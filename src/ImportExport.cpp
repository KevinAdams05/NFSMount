/*
 * Copyright 2026, Kevin Adams. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ImportExport.h"

#include <string.h>

#include <File.h>
#include <Messenger.h>
#include <Path.h>
#include <String.h>
#include <Window.h>

#include "Constants.h"
#include "Settings.h"


ImportExport::ImportExport(BWindow* target)
	:
	fTarget(target),
	fOpenPanel(NULL),
	fSavePanel(NULL)
{
	BMessenger messenger(target);

	BMessage openMsg(kMsgImportPanelDone);
	fOpenPanel = new BFilePanel(B_OPEN_PANEL, &messenger, NULL,
		B_FILE_NODE, false, &openMsg);
	fOpenPanel->Window()->SetTitle("Import NFSMount Shares");

	BMessage saveMsg(kMsgExportPanelDone);
	fSavePanel = new BFilePanel(B_SAVE_PANEL, &messenger, NULL,
		B_FILE_NODE, false, &saveMsg);
	fSavePanel->Window()->SetTitle("Export NFSMount Shares");
	fSavePanel->SetSaveText(BString("shares").Append(
		kExportFileExtension).String());
}


ImportExport::~ImportExport()
{
	delete fOpenPanel;
	delete fSavePanel;
}


void
ImportExport::ShowOpenPanel()
{
	if (fOpenPanel != NULL)
		fOpenPanel->Show();
}


void
ImportExport::ShowSavePanel()
{
	if (fSavePanel != NULL)
		fSavePanel->Show();
}


status_t
ImportExport::ExportTo(const entry_ref& directory, const char* filename,
	const Settings& settings)
{
	if (filename == NULL || *filename == '\0')
		return B_BAD_VALUE;

	// Resolve the target path: directory + filename. If the user
	// didn't add the .nfsmount extension, append it for them.
	BPath path;
	BEntry dirEntry(&directory);
	status_t result = dirEntry.GetPath(&path);
	if (result != B_OK)
		return result;

	BString name(filename);
	if (name.FindLast(kExportFileExtension)
			!= name.Length() - (int32)strlen(kExportFileExtension)) {
		name << kExportFileExtension;
	}
	path.Append(name.String());

	// Build the export message.
	BMessage exportMsg('NFSE');
	exportMsg.AddInt32(kExportFieldMagic, (int32)kExportMagic);
	exportMsg.AddInt32(kExportFieldVersion, kExportVersion);

	int32 count = settings.CountShares();
	for (int32 i = 0; i < count; i++) {
		BMessage share;
		if (settings.GetShare(i, &share) == B_OK)
			exportMsg.AddMessage(kExportFieldShares, &share);
	}

	BFile file(path.Path(),
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	result = file.InitCheck();
	if (result != B_OK)
		return result;

	return exportMsg.Flatten(&file);
}


status_t
ImportExport::ImportFrom(const entry_ref& ref, Settings& settings,
	int32* importedCount, int32* skippedCount)
{
	if (importedCount != NULL)
		*importedCount = 0;
	if (skippedCount != NULL)
		*skippedCount = 0;

	BFile file(&ref, B_READ_ONLY);
	status_t result = file.InitCheck();
	if (result != B_OK)
		return result;

	BMessage exportMsg;
	result = exportMsg.Unflatten(&file);
	if (result != B_OK)
		return result;

	// Validate header. We accept either an int32 or uint32 read of
	// the magic since AddInt32 was used on write but BMessage
	// stores the type tag separately.
	int32 magic = 0;
	if (exportMsg.FindInt32(kExportFieldMagic, &magic) != B_OK
		|| (uint32)magic != kExportMagic) {
		return B_BAD_TYPE;
	}

	int32 version = 0;
	if (exportMsg.FindInt32(kExportFieldVersion, &version) != B_OK)
		return B_BAD_TYPE;

	if (version > kExportVersion) {
		// File from a newer NFSMount than ours — we don't know
		// what fields it might carry. Refuse rather than silently
		// dropping data.
		return B_NOT_SUPPORTED;
	}

	// Build a set of existing share names so we can skip duplicates
	// instead of creating ambiguous entries.
	BMessage existingNames;
	int32 existingCount = settings.CountShares();
	for (int32 i = 0; i < existingCount; i++) {
		BMessage share;
		if (settings.GetShare(i, &share) != B_OK)
			continue;
		BString name;
		if (share.FindString(kFieldName, &name) == B_OK)
			existingNames.AddString(kFieldName, name.String());
	}

	// Append each share. The same name-collision check used by the
	// UI prevents accidental duplicates.
	type_code type;
	int32 shareCount = 0;
	exportMsg.GetInfo(kExportFieldShares, &type, &shareCount);

	for (int32 i = 0; i < shareCount; i++) {
		BMessage share;
		if (exportMsg.FindMessage(kExportFieldShares, i, &share) != B_OK)
			continue;

		BString name;
		share.FindString(kFieldName, &name);

		bool duplicate = false;
		int32 nameCount = 0;
		existingNames.GetInfo(kFieldName, &type, &nameCount);
		for (int32 j = 0; j < nameCount; j++) {
			BString existing;
			if (existingNames.FindString(kFieldName, j, &existing) == B_OK
				&& existing == name) {
				duplicate = true;
				break;
			}
		}

		if (duplicate) {
			if (skippedCount != NULL)
				(*skippedCount)++;
			continue;
		}

		if (settings.AddShare(&share) == B_OK) {
			existingNames.AddString(kFieldName, name.String());
			if (importedCount != NULL)
				(*importedCount)++;
		}
	}

	return B_OK;
}
