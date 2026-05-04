/*
 * Copyright 2026, Kevin Adams. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NFS_MOUNT_EDIT_SHARE_WINDOW_H
#define NFS_MOUNT_EDIT_SHARE_WINDOW_H


#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <MenuField.h>
#include <RadioButton.h>
#include <TextControl.h>
#include <Window.h>

#include <SupportDefs.h>


class EditShareWindow : public BWindow {
public:
								EditShareWindow(BRect frame,
									BWindow* target,
									const BMessage* share = NULL,
									int32 index = -1);
	virtual						~EditShareWindow();

	virtual	void				MessageReceived(BMessage* message);

private:
			void				_BuildLayout();
			void				_PopulateFields(const BMessage* share);
			bool				_ValidateFields();
			BMessage*			_BuildShareMessage();
			void				_UpdateVersionUI();

			BWindow*			fTarget;
			int32				fEditIndex;

			// Basic fields
			BMenuField*			fVersionField;
			BTextControl*		fNameField;
			BTextControl*		fServerField;
			BTextControl*		fExportField;
			BTextControl*		fMountPointField;
			BCheckBox*			fReadOnlyBox;
			BCheckBox*			fAutoMountBox;

			// Advanced fields
			BRadioButton*		fSoftRadio;
			BRadioButton*		fHardRadio;
			BTextControl*		fTimeoutField;
			BTextControl*		fRetransField;
			BTextControl*		fPortField;
			BMenuField*			fProtocolField;
			BTextControl*		fDirTimeField;
			BCheckBox*			fCacheMetadataBox;
			BCheckBox*			fEmulateXattrBox;

			BBox*				fAdvancedBox;

			// NFS v2 specific fields
			BTextControl*		fHostnameField;
			BTextControl*		fUIDField;
			BTextControl*		fGIDField;
			BBox*				fV2OptionsBox;
};


#endif
