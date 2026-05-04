/*
 * Copyright 2026, Kevin Adams. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "App.h"

#include <stdio.h>
#include <string.h>

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
			mounted++;
			continue;
		}

		status_t result = ShareManager::Mount(&share);
		if (result == B_OK) {
			mounted++;
		} else {
			failed++;
			BString error;
			error.SetToFormat("  %s: %s\n", name.String(),
				strerror(result));
			errors << error;
		}
	}

	// If any mounts failed, show the main window with errors
	if (failed > 0) {
		BString message;
		message.SetToFormat(
			"Some NFS shares could not be mounted automatically:\n\n"
			"%s\n"
			"Check that the NFS servers are reachable.",
			errors.String());
		BAlert* alert = new BAlert("Auto-Mount", message.String(),
			"Open NFSMount", "Dismiss", NULL, B_WIDTH_AS_USUAL,
			B_WARNING_ALERT);

		if (alert->Go() == 0) {
			// Open the main window
			MainWindow* window = new MainWindow(&fSettings);
			window->Show();
			return;
		}
	}

	PostMessage(B_QUIT_REQUESTED);
}


int
main()
{
	NFSMountApp app;
	app.Run();
	return 0;
}
