/*
 * Copyright 2026, Kevin Adams. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "EditShareWindow.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Alert.h>
#include <GroupView.h>
#include <LayoutBuilder.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <SeparatorView.h>
#include <String.h>
#include <StringView.h>

#include "Constants.h"


EditShareWindow::EditShareWindow(BRect frame, BWindow* target,
	const BMessage* share, int32 index)
	:
	BWindow(BRect(0, 0, 450, 480),
		index >= 0 ? "Edit NFS Share" : "Add NFS Share",
		B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS
			| B_CLOSE_ON_ESCAPE),
	fTarget(target),
	fEditIndex(index),
	fVersionField(NULL),
	fNameField(NULL),
	fServerField(NULL),
	fExportField(NULL),
	fMountPointField(NULL),
	fReadOnlyBox(NULL),
	fAutoMountBox(NULL),
	fSoftRadio(NULL),
	fHardRadio(NULL),
	fTimeoutField(NULL),
	fRetransField(NULL),
	fPortField(NULL),
	fProtocolField(NULL),
	fDirTimeField(NULL),
	fCacheMetadataBox(NULL),
	fEmulateXattrBox(NULL),
	fAdvancedTab(NULL),
	fAdvancedView(NULL),
	fHostnameField(NULL),
	fUIDField(NULL),
	fGIDField(NULL),
	fV2Tab(NULL),
	fV2View(NULL),
	fTabs(NULL)
{
	_BuildLayout();

	if (share != NULL)
		_PopulateFields(share);

	CenterOnScreen();
}


EditShareWindow::~EditShareWindow()
{
}


void
EditShareWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgRetryModeChanged:
		{
			// Enable retrans field only in soft mode
			bool soft = fSoftRadio->Value() == B_CONTROL_ON;
			fRetransField->SetEnabled(soft);
			break;
		}

		case kMsgVersionChanged:
			_UpdateVersionUI();
			break;

		case 'save':
		{
			if (!_ValidateFields())
				break;

			BMessage* shareMsg = _BuildShareMessage();
			BMessage notify(kMsgShareSaved);
			notify.AddMessage("share", shareMsg);
			notify.AddInt32("index", fEditIndex);
			delete shareMsg;

			fTarget->PostMessage(&notify);
			Quit();
			break;
		}

		case 'cncl':
			Quit();
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
EditShareWindow::_BuildLayout()
{
	// NFS version selector — sits above the tabs because it changes
	// which tabs are present.
	BPopUpMenu* versionMenu = new BPopUpMenu("version");
	BMenuItem* v2Item = new BMenuItem("NFSv2 (compatible)",
		new BMessage(kMsgVersionChanged));
	BMenuItem* v4Item = new BMenuItem("NFSv4",
		new BMessage(kMsgVersionChanged));
	versionMenu->AddItem(v2Item);
	versionMenu->AddItem(v4Item);
	v2Item->SetMarked(true);
	fVersionField = new BMenuField("version", "NFS version:", versionMenu);

	// ---- Basic tab ----
	fNameField = new BTextControl("name", "Name:", "", NULL);
	fServerField = new BTextControl("server", "Server (IP address):",
		"", NULL);
	fExportField = new BTextControl("export", "Export path:", "",
		NULL);
	fMountPointField = new BTextControl("mountpoint", "Mount point:",
		"/boot/home/", NULL);
	fReadOnlyBox = new BCheckBox("readonly", "Read only", NULL);
	fAutoMountBox = new BCheckBox("automount",
		"Mount automatically at login", NULL);

	// BCheckBox::MaxSize() defaults to the preferred (text-fit)
	// width, which in a vertical layout caps the column's max
	// width and locks horizontal resize. Override here so the
	// window stays freely resizable. Same fix below for the two
	// advanced-tab checkboxes.
	fReadOnlyBox->SetExplicitMaxSize(
		BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	fAutoMountBox->SetExplicitMaxSize(
		BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	BView* basicView = new BGroupView("basic", B_VERTICAL);
	BLayoutBuilder::Group<>((BGroupView*)basicView)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGrid()
			.AddTextControl(fNameField, 0, 0)
			.AddTextControl(fServerField, 0, 1)
			.AddTextControl(fExportField, 0, 2)
			.AddTextControl(fMountPointField, 0, 3)
			.SetColumnWeight(1, 10.0f)
		.End()
		.Add(fReadOnlyBox)
		.Add(fAutoMountBox)
		.AddGlue();

	// ---- Advanced tab (NFSv4) ----
	fSoftRadio = new BRadioButton("soft", "Soft",
		new BMessage(kMsgRetryModeChanged));
	fHardRadio = new BRadioButton("hard", "Hard",
		new BMessage(kMsgRetryModeChanged));
	fSoftRadio->SetValue(B_CONTROL_ON);

	BString defaultTimeout;
	defaultTimeout.SetToFormat("%" B_PRId32, kDefaultTimeout);
	fTimeoutField = new BTextControl("timeout", "Timeout (sec):",
		defaultTimeout.String(), NULL);

	BString defaultRetrans;
	defaultRetrans.SetToFormat("%" B_PRId32, kDefaultRetrans);
	fRetransField = new BTextControl("retrans", "Retries:",
		defaultRetrans.String(), NULL);

	BString defaultPort;
	defaultPort.SetToFormat("%" B_PRId32, kDefaultPort);
	fPortField = new BTextControl("port", "Port:",
		defaultPort.String(), NULL);

	BString defaultDirTime;
	defaultDirTime.SetToFormat("%" B_PRId32, kDefaultDirTime);
	fDirTimeField = new BTextControl("dirtime", "Dir cache (sec):",
		defaultDirTime.String(), NULL);

	BPopUpMenu* protoMenu = new BPopUpMenu("protocol");
	protoMenu->AddItem(new BMenuItem("tcp", NULL));
	protoMenu->AddItem(new BMenuItem("udp", NULL));
	protoMenu->ItemAt(0)->SetMarked(true);
	fProtocolField = new BMenuField("proto", "Protocol:", protoMenu);

	fCacheMetadataBox = new BCheckBox("cache", "Metadata cache", NULL);
	fCacheMetadataBox->SetValue(B_CONTROL_ON);
	fEmulateXattrBox = new BCheckBox("xattr",
		"Emulate named attributes", NULL);
	fCacheMetadataBox->SetExplicitMaxSize(
		BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	fEmulateXattrBox->SetExplicitMaxSize(
		BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	BGroupView* retryGroup = new BGroupView(B_HORIZONTAL);
	BLayoutBuilder::Group<>(retryGroup)
		.Add(fSoftRadio)
		.Add(fHardRadio)
		.AddGlue();

	fAdvancedView = new BGroupView("advanced", B_VERTICAL);
	BLayoutBuilder::Group<>((BGroupView*)fAdvancedView)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL)
			.Add(new BStringView("retrylabel", "Retry mode:"))
			.Add(retryGroup)
		.End()
		.AddGrid()
			.AddTextControl(fTimeoutField, 0, 0)
			.AddTextControl(fRetransField, 0, 1)
			.AddTextControl(fPortField, 0, 2)
			.Add(fProtocolField->CreateLabelLayoutItem(), 0, 3)
			.Add(fProtocolField->CreateMenuBarLayoutItem(), 1, 3)
			.AddTextControl(fDirTimeField, 0, 4)
			.SetColumnWeight(1, 10.0f)
		.End()
		.Add(fCacheMetadataBox)
		.Add(fEmulateXattrBox)
		.AddGlue();

	// ---- NFSv2 tab ----
	fHostnameField = new BTextControl("hostname", "Hostname:", "",
		NULL);
	fUIDField = new BTextControl("uid", "UID:", "0", NULL);
	fGIDField = new BTextControl("gid", "GID:", "0", NULL);

	fV2View = new BGroupView("v2options", B_VERTICAL);
	BLayoutBuilder::Group<>((BGroupView*)fV2View)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGrid()
			.AddTextControl(fHostnameField, 0, 0)
			.AddTextControl(fUIDField, 0, 1)
			.AddTextControl(fGIDField, 0, 2)
			.SetColumnWeight(1, 10.0f)
		.End()
		.AddGlue();

	// ---- Tabs ----
	fTabs = new BTabView("tabs", B_WIDTH_FROM_LABEL);

	BTab* basicTab = new BTab();
	fTabs->AddTab(basicView, basicTab);
	basicTab->SetLabel("Basic");

	fAdvancedTab = new BTab();
	fAdvancedTab->SetLabel("Advanced");
	fV2Tab = new BTab();
	fV2Tab->SetLabel("NFSv2");
	// Only one of {Advanced, NFSv2} is added at a time — see _UpdateVersionUI.

	// Buttons
	BButton* cancelButton = new BButton("cancel", "Cancel",
		new BMessage('cncl'));
	BButton* saveButton = new BButton("save", "Save",
		new BMessage('save'));
	saveButton->MakeDefault(true);

	// Main layout
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_WINDOW_INSETS)
		.AddGroup(B_HORIZONTAL)
			.Add(fVersionField->CreateLabelLayoutItem())
			.Add(fVersionField->CreateMenuBarLayoutItem())
			.AddGlue()
		.End()
		.Add(fTabs)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(cancelButton)
			.Add(saveButton)
		.End();

	// Establish the initial tab set based on the default version.
	_UpdateVersionUI();
}


void
EditShareWindow::_PopulateFields(const BMessage* share)
{
	BString value;
	int32 intValue;
	BString numStr;

	// NFS version
	int32 nfsVersion = kDefaultNFSVersion;
	share->FindInt32(kFieldNFSVersion, &nfsVersion);
	BPopUpMenu* versionMenu = dynamic_cast<BPopUpMenu*>(
		fVersionField->Menu());
	if (versionMenu != NULL) {
		const char* label = nfsVersion == kNFSVersion4
			? "NFSv4" : "NFSv2 (compatible)";
		BMenuItem* item = versionMenu->FindItem(label);
		if (item != NULL)
			item->SetMarked(true);
	}

	if (share->FindString(kFieldName, &value) == B_OK)
		fNameField->SetText(value.String());

	if (share->FindString(kFieldServer, &value) == B_OK)
		fServerField->SetText(value.String());

	if (share->FindString(kFieldExport, &value) == B_OK)
		fExportField->SetText(value.String());

	if (share->FindString(kFieldMountPoint, &value) == B_OK)
		fMountPointField->SetText(value.String());

	bool boolValue = false;
	if (share->FindBool(kFieldReadOnly, &boolValue) == B_OK)
		fReadOnlyBox->SetValue(boolValue ? B_CONTROL_ON : B_CONTROL_OFF);

	boolValue = false;
	if (share->FindBool(kFieldAutoMount, &boolValue) == B_OK)
		fAutoMountBox->SetValue(boolValue ? B_CONTROL_ON : B_CONTROL_OFF);

	// NFSv2 fields
	if (share->FindString(kFieldHostname, &value) == B_OK)
		fHostnameField->SetText(value.String());

	if (share->FindInt32(kFieldUID, &intValue) == B_OK) {
		numStr.SetToFormat("%" B_PRId32, intValue);
		fUIDField->SetText(numStr.String());
	}

	if (share->FindInt32(kFieldGID, &intValue) == B_OK) {
		numStr.SetToFormat("%" B_PRId32, intValue);
		fGIDField->SetText(numStr.String());
	}

	// NFSv4 advanced fields
	boolValue = false;
	if (share->FindBool(kFieldHard, &boolValue) == B_OK && boolValue) {
		fHardRadio->SetValue(B_CONTROL_ON);
		fRetransField->SetEnabled(false);
	}

	if (share->FindInt32(kFieldTimeout, &intValue) == B_OK) {
		numStr.SetToFormat("%" B_PRId32, intValue);
		fTimeoutField->SetText(numStr.String());
	}

	if (share->FindInt32(kFieldRetrans, &intValue) == B_OK) {
		numStr.SetToFormat("%" B_PRId32, intValue);
		fRetransField->SetText(numStr.String());
	}

	if (share->FindInt32(kFieldPort, &intValue) == B_OK) {
		numStr.SetToFormat("%" B_PRId32, intValue);
		fPortField->SetText(numStr.String());
	}

	if (share->FindString(kFieldProtocol, &value) == B_OK) {
		BPopUpMenu* menu = dynamic_cast<BPopUpMenu*>(
			fProtocolField->Menu());
		if (menu != NULL) {
			BMenuItem* item = menu->FindItem(value.String());
			if (item != NULL)
				item->SetMarked(true);
		}
	}

	if (share->FindInt32(kFieldDirTime, &intValue) == B_OK) {
		numStr.SetToFormat("%" B_PRId32, intValue);
		fDirTimeField->SetText(numStr.String());
	}

	boolValue = true;
	if (share->FindBool(kFieldCacheMetadata, &boolValue) == B_OK)
		fCacheMetadataBox->SetValue(
			boolValue ? B_CONTROL_ON : B_CONTROL_OFF);

	boolValue = false;
	if (share->FindBool(kFieldEmulateXattr, &boolValue) == B_OK)
		fEmulateXattrBox->SetValue(
			boolValue ? B_CONTROL_ON : B_CONTROL_OFF);

	_UpdateVersionUI();
}


bool
EditShareWindow::_ValidateFields()
{
	BString name(fNameField->Text());
	BString server(fServerField->Text());
	BString exportPath(fExportField->Text());
	BString mountPoint(fMountPointField->Text());

	if (name.IsEmpty()) {
		BAlert* alert = new BAlert("Validation",
			"Please enter a name for this share.",
			"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->Go();
		fNameField->MakeFocus(true);
		return false;
	}

	if (server.IsEmpty()) {
		BAlert* alert = new BAlert("Validation",
			"Please enter the NFS server address.",
			"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->Go();
		fServerField->MakeFocus(true);
		return false;
	}

	if (exportPath.IsEmpty() || exportPath.ByteAt(0) != '/') {
		BAlert* alert = new BAlert("Validation",
			"Export path must start with a forward slash (e.g. "
			"\"/volume1/share\").",
			"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->Go();
		fExportField->MakeFocus(true);
		return false;
	}

	if (mountPoint.IsEmpty() || mountPoint.ByteAt(0) != '/') {
		BAlert* alert = new BAlert("Validation",
			"Mount point must be an absolute path (e.g. "
			"\"/boot/home/NAS\").",
			"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->Go();
		fMountPointField->MakeFocus(true);
		return false;
	}

	// NFSv2 requires a hostname
	BMenuItem* versionItem = fVersionField->Menu()->FindMarked();
	bool isV4 = versionItem != NULL
		&& strcmp(versionItem->Label(), "NFSv4") == 0;

	if (!isV4) {
		BString hostname(fHostnameField->Text());
		if (hostname.IsEmpty()) {
			BAlert* alert = new BAlert("Validation",
				"Hostname is required for NFSv2. Enter the server's "
				"hostname (e.g. \"storage01\").",
				"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
			alert->Go();
			fHostnameField->MakeFocus(true);
			return false;
		}
	}

	return true;
}


BMessage*
EditShareWindow::_BuildShareMessage()
{
	BMessage* share = new BMessage('shar');

	// Determine selected NFS version
	BMenuItem* versionItem = fVersionField->Menu()->FindMarked();
	int32 nfsVersion = kDefaultNFSVersion;
	if (versionItem != NULL
		&& strcmp(versionItem->Label(), "NFSv4") == 0) {
		nfsVersion = kNFSVersion4;
	}
	share->AddInt32(kFieldNFSVersion, nfsVersion);

	share->AddString(kFieldName, fNameField->Text());
	share->AddString(kFieldServer, fServerField->Text());
	share->AddString(kFieldExport, fExportField->Text());
	share->AddString(kFieldMountPoint, fMountPointField->Text());
	share->AddBool(kFieldReadOnly,
		fReadOnlyBox->Value() == B_CONTROL_ON);
	share->AddBool(kFieldAutoMount,
		fAutoMountBox->Value() == B_CONTROL_ON);

	if (nfsVersion == kNFSVersion2) {
		// NFSv2 options
		share->AddString(kFieldHostname, fHostnameField->Text());
		share->AddInt32(kFieldUID, atoi(fUIDField->Text()));
		share->AddInt32(kFieldGID, atoi(fGIDField->Text()));
	} else {
		// NFSv4 advanced options
		share->AddBool(kFieldHard,
			fHardRadio->Value() == B_CONTROL_ON);
		share->AddInt32(kFieldTimeout, atoi(fTimeoutField->Text()));
		share->AddInt32(kFieldRetrans, atoi(fRetransField->Text()));
		share->AddInt32(kFieldPort, atoi(fPortField->Text()));

		BMenuItem* protoItem = fProtocolField->Menu()->FindMarked();
		if (protoItem != NULL)
			share->AddString(kFieldProtocol, protoItem->Label());
		else
			share->AddString(kFieldProtocol, kDefaultProtocol);

		share->AddInt32(kFieldDirTime, atoi(fDirTimeField->Text()));
		share->AddBool(kFieldCacheMetadata,
			fCacheMetadataBox->Value() == B_CONTROL_ON);
		share->AddBool(kFieldEmulateXattr,
			fEmulateXattrBox->Value() == B_CONTROL_ON);
	}

	return share;
}


void
EditShareWindow::_UpdateVersionUI()
{
	BMenuItem* item = fVersionField->Menu()->FindMarked();
	bool isV4 = item != NULL
		&& strcmp(item->Label(), "NFSv4") == 0;

	// Strip whichever extra tab is currently in the view (if any).
	// BTabView::RemoveTab returns the BTab and detaches the view —
	// it does not delete either, which is what we want, because we
	// reuse the same fAdvancedTab/fV2Tab objects and their views.
	for (int32 i = fTabs->CountTabs() - 1; i > 0; i--) {
		BTab* tab = fTabs->TabAt(i);
		if (tab == fAdvancedTab || tab == fV2Tab)
			fTabs->RemoveTab(i);
	}

	// Add the right one for the selected version. The view pointer
	// may not be currently parented (after RemoveTab it is detached),
	// so AddTab(view, tab) re-parents it cleanly.
	if (isV4) {
		fTabs->AddTab(fAdvancedView, fAdvancedTab);
	} else {
		fTabs->AddTab(fV2View, fV2Tab);
	}

	// Keep the user on the Basic tab when version flips, so the
	// content they're typing into doesn't vanish underneath them.
	if (fTabs->Selection() != 0)
		fTabs->Select(0);
}
