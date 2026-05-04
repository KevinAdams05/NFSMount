/*
 * Copyright 2026, Kevin Adams. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ShareManager.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <Alert.h>
#include <Application.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Roster.h>
#include <fs_volume.h>

#include "Constants.h"


status_t
ShareManager::Mount(const BMessage* share)
{
	if (share == NULL)
		return B_BAD_VALUE;

	BString mountPoint;
	share->FindString(kFieldMountPoint, &mountPoint);
	if (mountPoint.IsEmpty())
		return B_BAD_VALUE;

	// Create the mount point directory if it does not exist
	status_t result = CreateMountPoint(mountPoint.String());
	if (result != B_OK)
		return result;

	// Check if already mounted
	if (IsMounted(mountPoint.String()))
		return B_OK;

	// Build the parameter string for the selected NFS version
	BString params = BuildParameterString(share);
	if (params.IsEmpty())
		return B_BAD_VALUE;

	// Determine mount flags
	bool readOnly = false;
	share->FindBool(kFieldReadOnly, &readOnly);
	uint32 flags = readOnly ? B_MOUNT_READ_ONLY : 0;

	// Select filesystem based on NFS version
	int32 nfsVersion = kDefaultNFSVersion;
	share->FindInt32(kFieldNFSVersion, &nfsVersion);
	const char* fsName = nfsVersion == kNFSVersion4
		? kNFS4FileSystem : kNFSFileSystem;

	// Mount via the kernel API
	dev_t device = fs_mount_volume(mountPoint.String(), NULL,
		fsName, flags, params.String());

	if (device < 0)
		return (status_t)device;

	// Verify the mount actually succeeded — the kernel may accept the
	// mount call but the NFS4 add-on can fail internally (e.g. connection
	// refused, bad export path). Check that a new filesystem appeared.
	if (!IsMounted(mountPoint.String())) {
		// The mount call returned success but no filesystem appeared.
		// Try to clean up and report the failure.
		fs_unmount_volume(mountPoint.String(), 0);
		return B_IO_ERROR;
	}

	return B_OK;
}


status_t
ShareManager::Unmount(const char* mountPoint)
{
	if (mountPoint == NULL)
		return B_BAD_VALUE;

	if (!IsMounted(mountPoint))
		return B_OK;

	return fs_unmount_volume(mountPoint, 0);
}


bool
ShareManager::IsMounted(const char* mountPoint)
{
	if (mountPoint == NULL)
		return false;

	struct stat mountStat;
	if (stat(mountPoint, &mountStat) != 0)
		return false;

	// Compare device IDs with the parent directory.
	// If they differ, a separate filesystem is mounted here.
	BPath parentPath(mountPoint);
	if (parentPath.GetParent(&parentPath) != B_OK)
		return false;

	struct stat parentStat;
	if (stat(parentPath.Path(), &parentStat) != 0)
		return false;

	return mountStat.st_dev != parentStat.st_dev;
}


BString
ShareManager::BuildParameterString(const BMessage* share)
{
	if (share == NULL)
		return BString();

	int32 nfsVersion = kDefaultNFSVersion;
	share->FindInt32(kFieldNFSVersion, &nfsVersion);

	if (nfsVersion == kNFSVersion4)
		return BuildV4ParameterString(share);

	return BuildV2ParameterString(share);
}


BString
ShareManager::BuildV2ParameterString(const BMessage* share)
{
	BString params;
	if (share == NULL)
		return params;

	BString server;
	BString exportPath;
	BString hostname;
	share->FindString(kFieldServer, &server);
	share->FindString(kFieldExport, &exportPath);
	share->FindString(kFieldHostname, &hostname);

	if (server.IsEmpty() || exportPath.IsEmpty()
		|| hostname.IsEmpty()) {
		return params;
	}

	// Format: "nfs:ip:export_path,hostname=name[,uid=N][,gid=N]"
	params << "nfs:" << server << ":" << exportPath;
	params << ",hostname=" << hostname;

	int32 uid = kDefaultUID;
	share->FindInt32(kFieldUID, &uid);
	if (uid != kDefaultUID) {
		BString temp;
		temp.SetToFormat(",uid=%" B_PRId32, uid);
		params << temp;
	}

	int32 gid = kDefaultGID;
	share->FindInt32(kFieldGID, &gid);
	if (gid != kDefaultGID) {
		BString temp;
		temp.SetToFormat(",gid=%" B_PRId32, gid);
		params << temp;
	}

	return params;
}


BString
ShareManager::BuildV4ParameterString(const BMessage* share)
{
	BString params;
	if (share == NULL)
		return params;

	BString server;
	BString exportPath;
	share->FindString(kFieldServer, &server);
	share->FindString(kFieldExport, &exportPath);

	if (server.IsEmpty() || exportPath.IsEmpty())
		return params;

	// Format: "server:export_path [options]"
	params << server << ":" << exportPath;

	// Retry mode
	bool hard = false;
	share->FindBool(kFieldHard, &hard);
	if (hard)
		params << " hard";

	// Timeout (only add if non-default)
	int32 timeout = kDefaultTimeout;
	share->FindInt32(kFieldTimeout, &timeout);
	if (timeout != kDefaultTimeout) {
		BString temp;
		temp.SetToFormat(" timeo=%" B_PRId32, timeout);
		params << temp;
	}

	// Retransmissions
	int32 retrans = kDefaultRetrans;
	share->FindInt32(kFieldRetrans, &retrans);
	if (retrans != kDefaultRetrans) {
		BString temp;
		temp.SetToFormat(" retrans=%" B_PRId32, retrans);
		params << temp;
	}

	// Port
	int32 port = kDefaultPort;
	share->FindInt32(kFieldPort, &port);
	if (port != kDefaultPort) {
		BString temp;
		temp.SetToFormat(" port=%" B_PRId32, port);
		params << temp;
	}

	// Protocol
	BString protocol;
	share->FindString(kFieldProtocol, &protocol);
	if (!protocol.IsEmpty()
		&& protocol.ICompare(kDefaultProtocol) != 0) {
		params << " proto=" << protocol;
	}

	// Directory cache time
	int32 dirTime = kDefaultDirTime;
	share->FindInt32(kFieldDirTime, &dirTime);
	if (dirTime != kDefaultDirTime) {
		BString temp;
		temp.SetToFormat(" dirtime=%" B_PRId32, dirTime);
		params << temp;
	}

	// Metadata cache
	bool cacheMetadata = true;
	share->FindBool(kFieldCacheMetadata, &cacheMetadata);
	if (!cacheMetadata)
		params << " noac";

	// Named attribute emulation
	bool emulateXattr = false;
	share->FindBool(kFieldEmulateXattr, &emulateXattr);
	if (emulateXattr)
		params << " xattr-emu";

	return params;
}


status_t
ShareManager::CreateMountPoint(const char* path)
{
	if (path == NULL)
		return B_BAD_VALUE;

	return create_directory(path, 0755);
}


status_t
ShareManager::InstallAutoMount()
{
	// Find the launch directory
	BPath launchPath;
	find_directory(B_USER_SETTINGS_DIRECTORY, &launchPath);
	launchPath.Append("boot/launch");

	// Create the directory if needed
	status_t result = create_directory(launchPath.Path(), 0755);
	if (result != B_OK)
		return result;

	launchPath.Append(kLaunchScriptName);

	// Get our own executable path
	app_info appInfo;
	be_app->GetAppInfo(&appInfo);
	BPath appPath(&appInfo.ref);

	// Write the launch script
	BFile file(launchPath.Path(),
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	result = file.InitCheck();
	if (result != B_OK)
		return result;

	BString script;
	script.SetToFormat("#!/bin/sh\n\"%s\" --auto\n", appPath.Path());
	ssize_t written = file.Write(script.String(), script.Length());
	if (written < 0)
		return (status_t)written;

	// Make executable
	mode_t permissions = 0755;
	file.SetPermissions(permissions);

	return B_OK;
}


status_t
ShareManager::RemoveAutoMount()
{
	BPath launchPath;
	find_directory(B_USER_SETTINGS_DIRECTORY, &launchPath);
	launchPath.Append("boot/launch");
	launchPath.Append(kLaunchScriptName);

	BEntry entry(launchPath.Path());
	if (!entry.Exists())
		return B_OK;

	return entry.Remove();
}
