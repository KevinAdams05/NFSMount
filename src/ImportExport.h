/*
 * Copyright 2026, Kevin Adams. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NFS_MOUNT_IMPORT_EXPORT_H
#define NFS_MOUNT_IMPORT_EXPORT_H


#include <Entry.h>
#include <FilePanel.h>
#include <SupportDefs.h>


class BWindow;
class Settings;


// Wraps the Import / Export BFilePanel pair and handles serializing
// the share list to a flattened BMessage on disk.
//
// On-disk format (flattened BMessage):
//   uint32 "magic"   : 'NFSE'
//   int32  "version" : kExportVersion (currently 1)
//   BMessage[] "shares" : one entry per exported share, format
//                          identical to the share BMessages stored
//                          in Settings (so the existing field
//                          constants from Constants.h apply).
//
// Window-frame and other UI state are intentionally NOT exported.
class ImportExport {
public:
								ImportExport(BWindow* target);
								~ImportExport();

			void				ShowOpenPanel();
			void				ShowSavePanel();

			status_t			ExportTo(const entry_ref& directory,
									const char* filename,
									const Settings& settings);
			status_t			ImportFrom(const entry_ref& ref,
									Settings& settings,
									int32* importedCount,
									int32* skippedCount);

private:
			BWindow*			fTarget;
			BFilePanel*			fOpenPanel;
			BFilePanel*			fSavePanel;
};


#endif
