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

	// Writes the launch_daemon job file at
	// ~/config/settings/launch/user/NFSMount and removes the legacy
	// boot/launch/NFSMount shell script if present. Safe to call on
	// every app launch — idempotent. The job runs NFSMount --auto when
	// the network becomes available, which mounts every share whose
	// per-share autoMount flag is set.
	static	status_t			InstallLaunchJob();
};


#endif
