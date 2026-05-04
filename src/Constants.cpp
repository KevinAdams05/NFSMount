/*
 * Copyright 2026, Kevin Adams. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "Constants.h"


// Application
const char* const kAppSignature = "application/x-vnd.NFSMount";
const char* const kAppName = "NFSMount";

// NFS filesystem identifiers
const char* const kNFSFileSystem = "nfs";
const char* const kNFS4FileSystem = "nfs4";

// Settings file name
const char* const kSettingsFileName = "NFSMount";

// Default NFS4 mount options
const char* const kDefaultProtocol = "tcp";

// Settings field names
const char* const kFieldShares = "shares";
const char* const kFieldWindowFrame = "window_frame";
const char* const kFieldName = "name";
const char* const kFieldServer = "server";
const char* const kFieldExport = "export";
const char* const kFieldMountPoint = "mountPoint";
const char* const kFieldReadOnly = "readOnly";
const char* const kFieldAutoMount = "autoMount";
const char* const kFieldHard = "hard";
const char* const kFieldTimeout = "timeout";
const char* const kFieldRetrans = "retrans";
const char* const kFieldPort = "port";
const char* const kFieldProtocol = "protocol";
const char* const kFieldDirTime = "dirtime";
const char* const kFieldCacheMetadata = "cacheMetadata";
const char* const kFieldEmulateXattr = "emulateXattr";
const char* const kFieldNFSVersion = "nfsVersion";
const char* const kFieldHostname = "hostname";
const char* const kFieldUID = "uid";
const char* const kFieldGID = "gid";

// Auto-mount launch script name
const char* const kLaunchScriptName = "NFSMount";

// Export-file format
const uint32 kExportMagic = 'NFSE';
const int32 kExportVersion = 1;
const char* const kExportFieldMagic = "magic";
const char* const kExportFieldVersion = "version";
const char* const kExportFieldShares = "shares";
const char* const kExportFileExtension = ".nfsmount";
const char* const kExportMimeType = "application/x-vnd.NFSMount-export";
