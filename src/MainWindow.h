/*
 * Copyright 2026, Kevin Adams. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NFS_MOUNT_MAIN_WINDOW_H
#define NFS_MOUNT_MAIN_WINDOW_H


#include <Button.h>
#include <ColumnListView.h>
#include <MessageRunner.h>
#include <StringView.h>
#include <Window.h>

#include <SupportDefs.h>

class Settings;
class ShareItem;


class MainWindow : public BWindow {
public:
								MainWindow(Settings* settings);
	virtual						~MainWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

private:
			void				_BuildLayout();
			void				_LoadShares();
			void				_UpdateButtons();
			void				_MountSelected();
			void				_UnmountSelected();
			void				_AddShare();
			void				_EditSelected();
			void				_RemoveSelected();
			void				_HandleShareSaved(BMessage* message);
			void				_CheckMountStatus();
			void				_SetStatus(const char* text);
			ShareItem*			_SelectedItem();

			Settings*			fSettings;
			BColumnListView*	fShareList;
			BButton*			fMountButton;
			BButton*			fUnmountButton;
			BButton*			fAddButton;
			BButton*			fEditButton;
			BButton*			fRemoveButton;
			BStringView*		fStatusBar;
			BMessageRunner*		fStatusChecker;
};


#endif
