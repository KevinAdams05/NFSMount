/*
 * Copyright 2026, Kevin Adams. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NFS_MOUNT_SETTINGS_H
#define NFS_MOUNT_SETTINGS_H


#include <Message.h>
#include <Path.h>
#include <Rect.h>
#include <SupportDefs.h>


class Settings {
public:
								Settings();
								~Settings();

			status_t			Load();
			status_t			Save();

			BRect				WindowFrame() const;
			void				SetWindowFrame(BRect frame);

			int32				CountShares() const;
			status_t			GetShare(int32 index,
									BMessage* share) const;
			status_t			AddShare(const BMessage* share);
			status_t			UpdateShare(int32 index,
									const BMessage* share);
			status_t			RemoveShare(int32 index);

private:
			BMessage			fSettings;
			BPath				fSettingsPath;
};


#endif
