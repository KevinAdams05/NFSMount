/*
 * Copyright 2026, Kevin Adams. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "App.h"

#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include <Alert.h>
#include <NetworkAddress.h>
#include <NetworkInterface.h>
#include <NetworkRoster.h>
#include <String.h>

#include "AboutBox.h"
#include "Constants.h"
#include "MainWindow.h"
#include "ShareManager.h"


NFSMountApp::NFSMountApp()
	:
	BApplication(kAppSignature),
	fAutoMode(false)
{
	fSettings.Load();
}


NFSMountApp::~NFSMountApp()
{
}


void
NFSMountApp::ReadyToRun()
{
	// Ensure the launch_daemon job file is in place with the current
	// app path. Idempotent — running every launch keeps the path
	// fresh if the app has been moved, and migrates legacy installs
	// that still have a boot/launch shell script from the pre-issue-#2
	// implementation.
	ShareManager::InstallLaunchJob();

	if (fAutoMode) {
		_AutoMount();
		return;
	}

	MainWindow* window = new MainWindow(&fSettings);
	window->Show();
}


void
NFSMountApp::ArgvReceived(int32 argc, char** argv)
{
	for (int32 i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--auto") == 0)
			fAutoMode = true;
	}
}


void
NFSMountApp::AboutRequested()
{
	show_about_window();
}


bool
NFSMountApp::QuitRequested()
{
	return true;
}


/*! Wait until at least one non-loopback interface has a configured
	address, up to `timeout`. launch_daemon's `network_available` event
	can fire before DHCP has actually acquired a lease (observed
	2026-06-04: two consecutive boots auto-mounted while DHCP_DISCOVER
	was still unanswered, and the nfs4 add-on's RPC connect failed with
	B_NAME_NOT_FOUND). Polling here closes that gap for the common
	case; the retry passes in _AutoMount() cover anything that slips
	through. Returns true if the network came up within the timeout. */
bool
NFSMountApp::_WaitForNetwork(bigtime_t timeout)
{
	const bigtime_t kStep = 1000000;
		// 1 s between checks
	bigtime_t waited = 0;

	while (true) {
		BNetworkRoster& roster = BNetworkRoster::Default();
		BNetworkInterface interface;
		uint32 cookie = 0;
		while (roster.GetNextInterface(&cookie, interface) == B_OK) {
			if ((interface.Flags() & IFF_LOOPBACK) != 0
				|| (interface.Flags() & IFF_UP) == 0) {
				continue;
			}

			int32 count = interface.CountAddresses();
			for (int32 i = 0; i < count; i++) {
				BNetworkInterfaceAddress address;
				if (interface.GetAddressAt(i, address) != B_OK)
					continue;
				// A link-local address (IPv4 169.254/16, IPv6 fe80::/10)
				// is auto-configured before DHCP hands out a real lease,
				// so it is not proof the network can route to the NFS
				// server. Treating it as "ready" is what let mounts fire
				// into a half-configured network (observed 2026-06-04).
				// Wait for a routable address instead.
				const BNetworkAddress& addr = address.Address();
				if (!addr.IsEmpty() && !addr.IsLinkLocal()) {
					if (waited > 0) {
						syslog(LOG_INFO, "NFSMount: network became "
							"available after %" B_PRIdBIGTIME " s",
							waited / 1000000);
					}
					return true;
				}
			}
		}

		if (waited >= timeout)
			break;
		snooze(kStep);
		waited += kStep;
	}

	syslog(LOG_WARNING, "NFSMount: no configured network interface "
		"after %" B_PRIdBIGTIME " s — attempting mounts anyway",
		timeout / 1000000);
	return false;
}


/*! One pass over all auto-mount shares. Returns the number of shares
	that failed with a *retryable* error (network/server not reachable
	yet); permanent errors (bad configuration etc.) are logged but not
	counted, so the caller doesn't waste retry passes on them. */
int32
NFSMountApp::_AutoMountPass(BString& errors)
{
	int32 count = fSettings.CountShares();
	int32 retryable = 0;

	for (int32 i = 0; i < count; i++) {
		BMessage share;
		if (fSettings.GetShare(i, &share) != B_OK)
			continue;

		bool autoMount = false;
		share.FindBool(kFieldAutoMount, &autoMount);
		if (!autoMount)
			continue;

		BString name;
		share.FindString(kFieldName, &name);

		BString mountPoint;
		share.FindString(kFieldMountPoint, &mountPoint);

		// Skip if already mounted (also makes retry passes cheap: every
		// share that succeeded earlier short-circuits here)
		if (ShareManager::IsMounted(mountPoint.String()))
			continue;

		// Pre-flight reachability probe. fs_mount_volume() has no timeout,
		// so handing it an unreachable or not-yet-routable server can wedge
		// the auto-mount thread (and has taken the machine off the network
		// at boot). A bounded TCP connect first means we only call the
		// kernel mount once the server actually answers; otherwise we count
		// it as retryable and let a later pass try again.
		const bigtime_t kProbeTimeout = 3 * 1000000LL;
		if (!ShareManager::IsServerReachable(&share, kProbeTimeout)) {
			syslog(LOG_INFO, "NFSMount: %s not reachable yet — deferring "
				"(will retry)", name.String());
			retryable++;
			continue;
		}

		syslog(LOG_INFO, "NFSMount: mounting %s at %s",
			name.String(), mountPoint.String());
		status_t result = ShareManager::Mount(&share);
		if (result == B_OK) {
			syslog(LOG_INFO, "NFSMount: %s mounted successfully",
				name.String());
			continue;
		}

		// The errors the nfs4 add-on returns when the server (or the
		// network, or the idmapper) isn't ready yet — all worth a
		// retry. Anything else (bad parameters, permission problems)
		// is permanent and retrying would just waste boot time.
		bool retry = result == B_NAME_NOT_FOUND
			|| result == B_TIMED_OUT || result == B_IO_ERROR
			|| result == ECONNREFUSED || result == ENETUNREACH
			|| result == EHOSTUNREACH;
		if (retry)
			retryable++;

		syslog(LOG_ERR, "NFSMount: %s failed: %s%s", name.String(),
			strerror(result), retry ? " (will retry)" : "");
		BString error;
		error.SetToFormat("  %s: %s\n", name.String(), strerror(result));
		errors << error;
	}

	return retryable;
}


void
NFSMountApp::_AutoMount()
{
	int32 count = fSettings.CountShares();
	BString errors;

	syslog(LOG_INFO, "NFSMount: auto-mount starting, %" B_PRId32
		" share(s) configured", count);

	// Don't fire mounts into a half-configured network (see
	// _WaitForNetwork). 30 s covers slow DHCP servers comfortably
	// while keeping the no-network worst case bounded.
	_WaitForNetwork(30 * 1000000LL);

	// Up to three passes: the first regular attempt plus two retries,
	// 5 s apart, for shares that failed with a transient error. This
	// absorbs whatever the network wait didn't catch — a server that
	// is itself still booting, or the idmapper losing its startup race
	// (its 500 ms grace period in EnsureIdmapperRunning is usually,
	// but not always, enough on a busy boot).
	const int32 kMaxPasses = 3;
	for (int32 pass = 1; pass <= kMaxPasses; pass++) {
		if (pass > 1) {
			syslog(LOG_INFO, "NFSMount: retry pass %" B_PRId32
				" of %" B_PRId32, pass, kMaxPasses);
			snooze(5 * 1000000LL);
		}

		errors.SetTo("");
		if (_AutoMountPass(errors) == 0)
			break;
	}

	// Final accounting for the syslog
	int32 mounted = 0;
	int32 failed = 0;
	for (int32 i = 0; i < count; i++) {
		BMessage share;
		if (fSettings.GetShare(i, &share) != B_OK)
			continue;
		bool autoMount = false;
		share.FindBool(kFieldAutoMount, &autoMount);
		if (!autoMount)
			continue;
		BString mountPoint;
		share.FindString(kFieldMountPoint, &mountPoint);
		if (ShareManager::IsMounted(mountPoint.String()))
			mounted++;
		else
			failed++;
	}

	syslog(LOG_INFO, "NFSMount: auto-mount complete — %" B_PRId32 " mounted, %" B_PRId32 " failed",
		mounted, failed);

	// Suppress the (void) on `errors` — it would only be shown to the
	// user via a modal dialog, but `--auto` runs during user-session
	// startup before there's anyone to click it. The previous version
	// did `BAlert::Go()` here, which blocks indefinitely on the boot
	// path — the entire NFSMount team would hang waiting for a click
	// that never comes, and (because the app is B_SINGLE_LAUNCH)
	// subsequent NFSMount launches would silently delegate to the hung
	// team and exit without doing anything.
	//
	// Per-share failures are already in the syslog above (LOG_ERR
	// level). Users notice unmounted shares via the file panel /
	// Tracker the same way they would for any non-auto mount, and can
	// open NFSMount manually to retry.
	(void)errors;

	PostMessage(B_QUIT_REQUESTED);
}


int
main()
{
	NFSMountApp app;
	app.Run();
	return 0;
}
