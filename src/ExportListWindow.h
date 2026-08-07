/*
 * NFSMount: A native Haiku application for managing NFS shares.
 *
 * Copyright 2026, Kevin Adams <kevinadams05@gmail.com>
 * All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef NFS_MOUNT_EXPORT_LIST_WINDOW_H
#define NFS_MOUNT_EXPORT_LIST_WINDOW_H


#include <Messenger.h>
#include <Window.h>

class BButton;
class BListView;
class BStringView;


/*!	Shows the export list a server returned and lets one be picked.

	Opened by the Edit Share window once ExportBrowser has finished, and
	closes as soon as a choice is made or cancelled. The chosen path is sent
	back to the opener as kMsgExportSelected with a "path" string.
*/
class ExportListWindow : public BWindow {
public:
								ExportListWindow(BWindow* parent,
									const char* server,
									const BMessage* exports,
									BMessenger target);
	virtual						~ExportListWindow();

	virtual	void				MessageReceived(BMessage* message);

private:
			void				_UpdateClientsLabel();
			void				_ChooseSelection();

private:
			BListView*			fListView;
			BStringView*		fClientsLabel;
			BButton*			fSelectButton;
			BMessenger			fTarget;
			BMessage			fExports;

			// BListView has no enabled state, so the "server returned
			// nothing" placeholder is tracked here instead of by
			// disabling the list.
			bool				fHasExports;
};


#endif	// NFS_MOUNT_EXPORT_LIST_WINDOW_H
