/*
 * Copyright 2026, Kevin Adams. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "App.h"

#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include <Alert.h>
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


void
NFSMountApp::_AutoMount()
{
	int32 count = fSettings.CountShares();
	int32 mounted = 0;
	int32 failed = 0;
	BString errors;

	syslog(LOG_INFO, "NFSMount: auto-mount starting, %" B_PRId32 " share(s) configured", count);

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

		// Skip if already mounted
		if (ShareManager::IsMounted(mountPoint.String())) {
			syslog(LOG_INFO, "NFSMount: %s already mounted at %s, skipping",
				name.String(), mountPoint.String());
			mounted++;
			continue;
		}

		syslog(LOG_INFO, "NFSMount: mounting %s at %s",
			name.String(), mountPoint.String());
		status_t result = ShareManager::Mount(&share);
		if (result == B_OK) {
			syslog(LOG_INFO, "NFSMount: %s mounted successfully", name.String());
			mounted++;
		} else {
			syslog(LOG_ERR, "NFSMount: %s failed: %s",
				name.String(), strerror(result));
			failed++;
			BString error;
			error.SetToFormat("  %s: %s\n", name.String(),
				strerror(result));
			errors << error;
		}
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
