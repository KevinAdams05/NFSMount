/*
 * Copyright 2026, Kevin Adams. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "MainWindow.h"

#include <stdio.h>

#include <Alert.h>
#include <Application.h>
#include <ColumnTypes.h>
#include <Entry.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <Messenger.h>
#include <String.h>

#include "AboutBox.h"
#include "Constants.h"
#include "EditShareWindow.h"
#include "ImportExport.h"
#include "Settings.h"
#include "ShareItem.h"
#include "ShareManager.h"


MainWindow::MainWindow(Settings* settings)
	:
	BWindow(settings->WindowFrame(), kAppName, B_TITLED_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS),
	fSettings(settings),
	fMenuBar(NULL),
	fImportItem(NULL),
	fExportItem(NULL),
	fMountMenuItem(NULL),
	fUnmountMenuItem(NULL),
	fEditMenuItem(NULL),
	fRemoveMenuItem(NULL),
	fShareList(NULL),
	fMountButton(NULL),
	fUnmountButton(NULL),
	fAddButton(NULL),
	fEditButton(NULL),
	fRemoveButton(NULL),
	fStatusBar(NULL),
	fStatusChecker(NULL),
	fImportExport(NULL)
{
	_BuildMenuBar();
	_BuildLayout();
	_LoadShares();
	_UpdateButtons();

	// Window size limits are derived from the layout via
	// B_AUTO_UPDATE_SIZE_LIMITS — see the BWindow flags above.
	// Children that would otherwise cap the layout's max width
	// (status bar, etc.) explicitly set their own MaxSize.

	// Start periodic mount status check
	BMessage checkMsg(kMsgCheckStatus);
	fStatusChecker = new BMessageRunner(BMessenger(this), &checkMsg,
		kStatusCheckInterval);

	fImportExport = new ImportExport(this);
}


MainWindow::~MainWindow()
{
	delete fStatusChecker;
	delete fImportExport;
}


void
MainWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgShareSelected:
			_UpdateButtons();
			{
				ShareItem* item = _SelectedItem();
				if (item != NULL) {
					BString status;
					status.SetToFormat("%s — %s",
						item->MountPoint(),
						item->IsMounted() ? "Mounted" : "Unmounted");
					_SetStatus(status.String());
				} else {
					_SetStatus("");
				}
			}
			break;

		case kMsgShareInvoked:
			_EditSelected();
			break;

		case kMsgMountShare:
			_MountSelected();
			break;

		case kMsgUnmountShare:
			_UnmountSelected();
			break;

		case kMsgAddShare:
			_AddShare();
			break;

		case kMsgEditShare:
			_EditSelected();
			break;

		case kMsgRemoveShare:
			_RemoveSelected();
			break;

		case kMsgShareSaved:
			_HandleShareSaved(message);
			break;

		case kMsgCheckStatus:
			_CheckMountStatus();
			break;

		case kMsgImportShares:
			_StartImport();
			break;

		case kMsgExportShares:
			_StartExport();
			break;

		case kMsgImportPanelDone:
		case kMsgExportPanelDone:
			_HandleImportExport(message);
			break;

		case kMsgShowAbout:
			show_about_window();
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
MainWindow::QuitRequested()
{
	fSettings->SetWindowFrame(Frame());
	fSettings->Save();

	// The auto-mount launch_daemon job is now installed once at
	// app startup (App::ReadyToRun → ShareManager::InstallLaunchJob)
	// and stays installed permanently; per-share autoMount flags
	// gate which shares it acts on. No install/remove needed here.

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
MainWindow::_BuildMenuBar()
{
	fMenuBar = new BMenuBar("menubar");

	BMenu* fileMenu = new BMenu("File");
	fImportItem = new BMenuItem("Import shares" B_UTF8_ELLIPSIS,
		new BMessage(kMsgImportShares), 'O');
	fExportItem = new BMenuItem("Export shares" B_UTF8_ELLIPSIS,
		new BMessage(kMsgExportShares), 'S');
	fileMenu->AddItem(fImportItem);
	fileMenu->AddItem(fExportItem);
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem("Quit",
		new BMessage(B_QUIT_REQUESTED), 'Q'));

	BMenu* editMenu = new BMenu("Edit");
	editMenu->AddItem(new BMenuItem("Add" B_UTF8_ELLIPSIS,
		new BMessage(kMsgAddShare), 'N'));
	fEditMenuItem = new BMenuItem("Edit" B_UTF8_ELLIPSIS,
		new BMessage(kMsgEditShare), 'E');
	editMenu->AddItem(fEditMenuItem);
	fRemoveMenuItem = new BMenuItem("Remove",
		new BMessage(kMsgRemoveShare), B_DELETE, 0);
	editMenu->AddItem(fRemoveMenuItem);
	editMenu->AddSeparatorItem();
	fMountMenuItem = new BMenuItem("Mount",
		new BMessage(kMsgMountShare), 'M');
	editMenu->AddItem(fMountMenuItem);
	fUnmountMenuItem = new BMenuItem("Unmount",
		new BMessage(kMsgUnmountShare), 'U');
	editMenu->AddItem(fUnmountMenuItem);

	BMenu* helpMenu = new BMenu("Help");
	helpMenu->AddItem(new BMenuItem("About NFSMount" B_UTF8_ELLIPSIS,
		new BMessage(kMsgShowAbout)));

	fMenuBar->AddItem(fileMenu);
	fMenuBar->AddItem(editMenu);
	fMenuBar->AddItem(helpMenu);
}


void
MainWindow::_BuildLayout()
{
	// Column list view
	fShareList = new BColumnListView("sharelist", 0);
	fShareList->SetSelectionMessage(
		new BMessage(kMsgShareSelected));
	fShareList->SetInvocationMessage(
		new BMessage(kMsgShareInvoked));

	fShareList->AddColumn(
		new BStringColumn("Name", 120, 50, 300, B_TRUNCATE_END), kNameColumn);
	fShareList->AddColumn(
		new BStringColumn("Server", 130, 50, 300, B_TRUNCATE_END),
		kServerColumn);
	fShareList->AddColumn(
		new BStringColumn("Export", 150, 50, 400, B_TRUNCATE_MIDDLE),
		kExportColumn);
	fShareList->AddColumn(
		new BStringColumn("Status", 80, 60, 120, B_TRUNCATE_END),
		kStatusColumn);

	// Buttons
	fMountButton = new BButton("mount", "Mount",
		new BMessage(kMsgMountShare));
	fUnmountButton = new BButton("unmount", "Unmount",
		new BMessage(kMsgUnmountShare));
	fAddButton = new BButton("add", "Add" B_UTF8_ELLIPSIS,
		new BMessage(kMsgAddShare));
	fEditButton = new BButton("edit", "Edit" B_UTF8_ELLIPSIS,
		new BMessage(kMsgEditShare));
	fRemoveButton = new BButton("remove", "Remove",
		new BMessage(kMsgRemoveShare));

	// Status bar. Default BStringView::MaxSize() returns the preferred
	// (text-fit) width — in a vertical layout that caps the entire
	// window's max width. Override with B_SIZE_UNLIMITED so the
	// window can be widened freely.
	fStatusBar = new BStringView("statusbar", "");
	fStatusBar->SetExplicitMinSize(BSize(100, B_SIZE_UNSET));
	fStatusBar->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	// Layout: menu bar at the very top (no insets), main content
	// below with the standard window insets.
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(fMenuBar)
		.AddGroup(B_VERTICAL)
			.SetInsets(B_USE_WINDOW_INSETS)
			.Add(fShareList, 10.0f)
			.AddGroup(B_HORIZONTAL)
				.Add(fMountButton)
				.Add(fUnmountButton)
				.AddGlue()
				.Add(fAddButton)
				.Add(fEditButton)
				.Add(fRemoveButton)
			.End()
			.Add(fStatusBar)
		.End();

	// Keyboard shortcuts are provided by the menu items; nothing
	// extra needed here.
}


void
MainWindow::_LoadShares()
{
	// Clear existing rows
	while (fShareList->CountRows() > 0) {
		BRow* row = fShareList->RowAt(0);
		fShareList->RemoveRow(row);
		delete row;
	}

	// Load from settings
	int32 count = fSettings->CountShares();
	for (int32 i = 0; i < count; i++) {
		BMessage share;
		if (fSettings->GetShare(i, &share) == B_OK) {
			ShareItem* item = new ShareItem(&share, i);
			fShareList->AddRow(item);

			// Check current mount status
			BString mountPoint;
			share.FindString(kFieldMountPoint, &mountPoint);
			item->UpdateStatus(
				ShareManager::IsMounted(mountPoint.String()));
		}
	}
}


void
MainWindow::_UpdateButtons()
{
	ShareItem* item = _SelectedItem();
	bool hasSelection = item != NULL;
	bool isMounted = hasSelection && item->IsMounted();

	fMountButton->SetEnabled(hasSelection && !isMounted);
	fUnmountButton->SetEnabled(hasSelection && isMounted);
	fEditButton->SetEnabled(hasSelection);
	fRemoveButton->SetEnabled(hasSelection && !isMounted);

	_UpdateMenuItems();
}


void
MainWindow::_UpdateMenuItems()
{
	ShareItem* item = _SelectedItem();
	bool hasSelection = item != NULL;
	bool isMounted = hasSelection && item->IsMounted();

	fMountMenuItem->SetEnabled(hasSelection && !isMounted);
	fUnmountMenuItem->SetEnabled(hasSelection && isMounted);
	fEditMenuItem->SetEnabled(hasSelection);
	fRemoveMenuItem->SetEnabled(hasSelection && !isMounted);

	// Export is only meaningful if there's at least one share.
	fExportItem->SetEnabled(fSettings->CountShares() > 0);
}


void
MainWindow::_MountSelected()
{
	ShareItem* item = _SelectedItem();
	if (item == NULL)
		return;

	BMessage share;
	if (fSettings->GetShare(item->Index(), &share) != B_OK)
		return;

	_SetStatus("Mounting...");

	status_t result = ShareManager::Mount(&share);
	if (result != B_OK) {
		BString message;
		if (result == B_IO_ERROR) {
			int32 nfsVersion = kDefaultNFSVersion;
			share.FindInt32(kFieldNFSVersion, &nfsVersion);

			if (nfsVersion == kNFSVersion4) {
				message.SetToFormat(
					"Mount of \"%s\" failed — the NFSv4 server "
					"did not respond.\n\n"
					"Common causes:\n"
					"  \xe2\x80\xa2 NFSv4 is not enabled on the server\n"
					"  \xe2\x80\xa2 The server is unreachable (check "
					"network)\n"
					"  \xe2\x80\xa2 The export path is wrong (try the "
					"path relative to the NFS pseudoroot)\n"
					"  \xe2\x80\xa2 Port 2049 is blocked by a firewall\n"
					"\nSee /var/log/syslog for kernel-level details.",
					item->Name());
			} else {
				message.SetToFormat(
					"Mount of \"%s\" failed — the NFS server did "
					"not respond.\n\n"
					"Common causes:\n"
					"  \xe2\x80\xa2 NFS is not enabled on the server\n"
					"  \xe2\x80\xa2 The server IP is unreachable\n"
					"  \xe2\x80\xa2 The export path is wrong\n"
					"  \xe2\x80\xa2 The hostname parameter is incorrect"
					"\n\n"
					"See /var/log/syslog for kernel-level details.",
					item->Name());
			}
		} else {
			message.SetToFormat(
				"Could not mount \"%s\".\n\n"
				"Error: %s (%" B_PRId32 ")\n\n"
				"Check that the NFS server is reachable and the "
				"export path is correct. See /var/log/syslog for "
				"details.",
				item->Name(), strerror(result), (int32)result);
		}
		BAlert* alert = new BAlert("Mount Failed", message.String(),
			"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go();

		_SetStatus("Mount failed.");
	} else {
		item->UpdateStatus(true);
		fShareList->InvalidateRow(item);

		BString status;
		status.SetToFormat("Mounted at %s", item->MountPoint());
		_SetStatus(status.String());
	}

	_UpdateButtons();
}


void
MainWindow::_UnmountSelected()
{
	ShareItem* item = _SelectedItem();
	if (item == NULL || !item->IsMounted())
		return;

	status_t result = ShareManager::Unmount(item->MountPoint());
	if (result != B_OK) {
		BString message;
		message.SetToFormat(
			"Could not unmount \"%s\".\n\n%s\n\n"
			"Make sure no files on the share are open.",
			item->Name(), strerror(result));
		BAlert* alert = new BAlert("Unmount Failed",
			message.String(), "OK", NULL, NULL, B_WIDTH_AS_USUAL,
			B_STOP_ALERT);
		alert->Go();
	} else {
		item->UpdateStatus(false);
		fShareList->InvalidateRow(item);
		_SetStatus("Unmounted.");
	}

	_UpdateButtons();
}


void
MainWindow::_AddShare()
{
	EditShareWindow* window = new EditShareWindow(Frame(), this);
	window->Show();
}


void
MainWindow::_EditSelected()
{
	ShareItem* item = _SelectedItem();
	if (item == NULL)
		return;

	BMessage share;
	if (fSettings->GetShare(item->Index(), &share) != B_OK)
		return;

	EditShareWindow* window = new EditShareWindow(Frame(), this,
		&share, item->Index());
	window->Show();
}


void
MainWindow::_RemoveSelected()
{
	ShareItem* item = _SelectedItem();
	if (item == NULL)
		return;

	if (item->IsMounted()) {
		BAlert* alert = new BAlert("Remove Share",
			"This share is currently mounted. Unmount it first "
			"before removing.",
			"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_INFO_ALERT);
		alert->Go();
		return;
	}

	BString message;
	message.SetToFormat("Remove the share \"%s\"?", item->Name());
	BAlert* alert = new BAlert("Remove Share", message.String(),
		"Cancel", "Remove", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	alert->SetShortcut(0, B_ESCAPE);

	if (alert->Go() != 1)
		return;

	fSettings->RemoveShare(item->Index());
	fSettings->Save();
	_LoadShares();
	_UpdateButtons();
	_SetStatus("Share removed.");
}


void
MainWindow::_HandleShareSaved(BMessage* message)
{
	BMessage share;
	int32 index = -1;

	if (message->FindMessage("share", &share) != B_OK)
		return;

	message->FindInt32("index", &index);

	if (index >= 0)
		fSettings->UpdateShare(index, &share);
	else
		fSettings->AddShare(&share);

	fSettings->Save();

	_LoadShares();
	_UpdateButtons();
	_SetStatus(index >= 0 ? "Share updated." : "Share added.");
}


void
MainWindow::_CheckMountStatus()
{
	int32 count = fShareList->CountRows();
	for (int32 i = 0; i < count; i++) {
		ShareItem* item = dynamic_cast<ShareItem*>(
			fShareList->RowAt(i));
		if (item == NULL)
			continue;

		bool wasMounted = item->IsMounted();
		bool isMounted = ShareManager::IsMounted(item->MountPoint());

		if (wasMounted != isMounted) {
			item->UpdateStatus(isMounted);
			fShareList->InvalidateRow(item);
		}
	}

	// Update buttons in case status changed
	_UpdateButtons();
}


void
MainWindow::_SetStatus(const char* text)
{
	fStatusBar->SetText(text);
}


ShareItem*
MainWindow::_SelectedItem()
{
	return dynamic_cast<ShareItem*>(fShareList->CurrentSelection());
}


void
MainWindow::_StartImport()
{
	if (fImportExport != NULL)
		fImportExport->ShowOpenPanel();
}


void
MainWindow::_StartExport()
{
	if (fImportExport != NULL)
		fImportExport->ShowSavePanel();
}


void
MainWindow::_HandleImportExport(BMessage* message)
{
	if (fImportExport == NULL)
		return;

	if (message->what == kMsgExportPanelDone) {
		entry_ref dirRef;
		BString filename;
		if (message->FindRef("directory", &dirRef) != B_OK
			|| message->FindString("name", &filename) != B_OK) {
			return;
		}

		status_t result = fImportExport->ExportTo(dirRef,
			filename.String(), *fSettings);
		if (result != B_OK) {
			BString text;
			text.SetToFormat("Could not export shares.\n\n%s",
				strerror(result));
			BAlert* alert = new BAlert("Export Failed",
				text.String(), "OK", NULL, NULL, B_WIDTH_AS_USUAL,
				B_STOP_ALERT);
			alert->Go();
			_SetStatus("Export failed.");
		} else {
			_SetStatus("Shares exported.");
		}
		return;
	}

	// kMsgImportPanelDone
	entry_ref ref;
	if (message->FindRef("refs", &ref) != B_OK)
		return;

	int32 importedCount = 0;
	int32 skippedCount = 0;
	status_t result = fImportExport->ImportFrom(ref, *fSettings,
		&importedCount, &skippedCount);
	if (result != B_OK) {
		BString text;
		text.SetToFormat("Could not import shares.\n\n%s",
			strerror(result));
		BAlert* alert = new BAlert("Import Failed", text.String(),
			"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go();
		_SetStatus("Import failed.");
		return;
	}

	fSettings->Save();

	_LoadShares();
	_UpdateButtons();

	BString status;
	if (skippedCount > 0) {
		status.SetToFormat("Imported %" B_PRId32 " share(s); "
			"%" B_PRId32 " skipped (already exist by name).",
			importedCount, skippedCount);
	} else {
		status.SetToFormat("Imported %" B_PRId32 " share(s).",
			importedCount);
	}
	_SetStatus(status.String());
}
