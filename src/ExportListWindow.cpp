/*
 * NFSMount: A native Haiku application for managing NFS shares.
 *
 * Copyright 2026, Kevin Adams <kevinadams05@gmail.com>
 * All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include "ExportListWindow.h"

#include <Button.h>
#include <LayoutBuilder.h>
#include <ListItem.h>
#include <ListView.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>

#include "Constants.h"


ExportListWindow::ExportListWindow(BWindow* parent, const char* server,
	const BMessage* exports, BMessenger target)
	:
	BWindow(BRect(0, 0, 420, 320), "Available shares", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS
			| B_CLOSE_ON_ESCAPE),
	fListView(NULL),
	fClientsLabel(NULL),
	fSelectButton(NULL),
	fTarget(target),
	fExports(*exports),
	fHasExports(false)
{
	// Modal against the Edit Share window only, so the rest of the
	// application stays usable while this is open.
	if (parent != NULL) {
		SetFeel(B_MODAL_SUBSET_WINDOW_FEEL);
		AddToSubset(parent);
	}

	BString heading("Shares exported by ");
	heading << server;
	BStringView* headingView = new BStringView("heading", heading.String());

	fListView = new BListView("exports", B_SINGLE_SELECTION_LIST);
	fListView->SetSelectionMessage(new BMessage(kMsgExportListSelection));
	fListView->SetInvocationMessage(new BMessage(kMsgExportListInvoked));

	BScrollView* scrollView = new BScrollView("scroller", fListView,
		0, false, true);

	// The group list is server-defined and not machine-readable (RFC 1813),
	// so it is shown purely as a hint about who may mount the share.
	fClientsLabel = new BStringView("clients", "");

	fSelectButton = new BButton("select", "Select",
		new BMessage(kMsgExportListInvoked));
	fSelectButton->SetEnabled(false);
	BButton* cancelButton = new BButton("cancel", "Cancel",
		new BMessage(B_QUIT_REQUESTED));

	const char* path;
	for (int32 i = 0; fExports.FindString(kFieldExportPath, i, &path) == B_OK;
			i++) {
		fListView->AddItem(new BStringItem(path));
	}

	fHasExports = fListView->CountItems() > 0;
	if (!fHasExports) {
		fListView->AddItem(new BStringItem(
			"The server did not report any exported shares."));
	}

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_WINDOW_SPACING)
		.Add(headingView)
		.Add(scrollView)
		.Add(fClientsLabel)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(cancelButton)
			.Add(fSelectButton)
		.End();

	SetDefaultButton(fSelectButton);
	if (parent != NULL)
		CenterIn(parent->Frame());
	else
		CenterOnScreen();
}


ExportListWindow::~ExportListWindow()
{
}


void
ExportListWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgExportListSelection:
			_UpdateClientsLabel();
			break;

		case kMsgExportListInvoked:
			_ChooseSelection();
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


/*!	Shows which clients the selected export is offered to.

	Informational only -- the names come straight from the server and are not
	interpreted.
*/
void
ExportListWindow::_UpdateClientsLabel()
{
	int32 selected = fListView->CurrentSelection();
	fSelectButton->SetEnabled(selected >= 0 && fHasExports);

	if (selected < 0) {
		fClientsLabel->SetText("");
		return;
	}

	const char* clients = NULL;
	BString text;
	if (fExports.FindString(kFieldExportClients, selected, &clients) == B_OK
		&& clients != NULL && *clients != '\0') {
		text << "Allowed clients: " << clients;
	}

	fClientsLabel->SetText(text.String());
}


/*!	Sends the chosen path back to the Edit Share window and closes. */
void
ExportListWindow::_ChooseSelection()
{
	int32 selected = fListView->CurrentSelection();
	if (selected < 0 || !fHasExports)
		return;

	const char* path = NULL;
	if (fExports.FindString(kFieldExportPath, selected, &path) != B_OK)
		return;

	BMessage chosen(kMsgExportSelected);
	chosen.AddString(kFieldExportPath, path);
	fTarget.SendMessage(&chosen);

	PostMessage(B_QUIT_REQUESTED);
}
