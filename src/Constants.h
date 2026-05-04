/*
 * Copyright 2026, Kevin Adams. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NFS_MOUNT_CONSTANTS_H
#define NFS_MOUNT_CONSTANTS_H


#include <SupportDefs.h>


// Application
extern const char* const kAppSignature;
extern const char* const kAppName;

// NFS filesystem identifiers
extern const char* const kNFSFileSystem;
extern const char* const kNFS4FileSystem;

// Settings file name (stored in B_USER_SETTINGS_DIRECTORY)
extern const char* const kSettingsFileName;

// NFS version constants
static const int32 kNFSVersion2 = 2;
static const int32 kNFSVersion4 = 4;
static const int32 kDefaultNFSVersion = 2;

// Default NFS4 mount options
static const int32 kDefaultTimeout = 60;
static const int32 kDefaultRetrans = 5;
static const int32 kDefaultPort = 2049;
extern const char* const kDefaultProtocol;
static const int32 kDefaultDirTime = 5;

// Default NFS2 mount options
static const int32 kDefaultUID = 0;
static const int32 kDefaultGID = 0;

// Mount status check interval (microseconds) -- every 10 seconds
static const bigtime_t kStatusCheckInterval = 10000000;

// Message codes
enum {
	kMsgShareSelected		= 'shsl',
	kMsgShareInvoked		= 'shin',
	kMsgMountShare			= 'mnts',
	kMsgUnmountShare		= 'umnt',
	kMsgAddShare			= 'adds',
	kMsgEditShare			= 'edts',
	kMsgRemoveShare			= 'rmvs',
	kMsgShareSaved			= 'svds',
	kMsgShareCancelled		= 'cncl',
	kMsgAutoMount			= 'auto',
	kMsgCheckStatus			= 'chks',
	kMsgRetryModeChanged	= 'rtmd',
	kMsgVersionChanged		= 'vrch',
};

// Settings field names
extern const char* const kFieldShares;
extern const char* const kFieldWindowFrame;
extern const char* const kFieldName;
extern const char* const kFieldServer;
extern const char* const kFieldExport;
extern const char* const kFieldMountPoint;
extern const char* const kFieldReadOnly;
extern const char* const kFieldAutoMount;
extern const char* const kFieldHard;
extern const char* const kFieldTimeout;
extern const char* const kFieldRetrans;
extern const char* const kFieldPort;
extern const char* const kFieldProtocol;
extern const char* const kFieldDirTime;
extern const char* const kFieldCacheMetadata;
extern const char* const kFieldEmulateXattr;
extern const char* const kFieldNFSVersion;
extern const char* const kFieldHostname;
extern const char* const kFieldUID;
extern const char* const kFieldGID;

// Auto-mount launch script name
extern const char* const kLaunchScriptName;


#endif
