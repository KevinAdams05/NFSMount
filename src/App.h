/*
 * Copyright 2026, Kevin Adams. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NFS_MOUNT_APP_H
#define NFS_MOUNT_APP_H


#include <Application.h>

#include "Settings.h"


class NFSMountApp : public BApplication {
public:
								NFSMountApp();
	virtual						~NFSMountApp();

	virtual	void				ReadyToRun();
	virtual	void				ArgvReceived(int32 argc, char** argv);
	virtual	void				AboutRequested();
	virtual	bool				QuitRequested();

			Settings*			GetSettings() { return &fSettings; }

private:
			void				_AutoMount();

			Settings			fSettings;
			bool				fAutoMode;
};


#endif
