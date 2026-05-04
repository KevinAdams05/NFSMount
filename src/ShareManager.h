/*
 * Copyright 2026, Kevin Adams. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NFS_MOUNT_SHARE_MANAGER_H
#define NFS_MOUNT_SHARE_MANAGER_H


#include <Message.h>
#include <String.h>
#include <SupportDefs.h>


class ShareManager {
public:
	static	status_t			Mount(const BMessage* share);
	static	status_t			Unmount(const char* mountPoint);
	static	bool				IsMounted(const char* mountPoint);
	static	BString				BuildParameterString(
									const BMessage* share);
	static	BString				BuildV2ParameterString(
									const BMessage* share);
	static	BString				BuildV4ParameterString(
									const BMessage* share);
	static	status_t			CreateMountPoint(const char* path);
	static	status_t			InstallAutoMount();
	static	status_t			RemoveAutoMount();
};


#endif
