/*
 * NFSMount: A native Haiku application for managing NFS shares.
 *
 * Copyright 2026, Kevin Adams <kevinadams05@gmail.com>
 * All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef NFS_MOUNT_EXPORT_PROTOCOL_H
#define NFS_MOUNT_EXPORT_PROTOCOL_H


#include <stddef.h>

#include <SupportDefs.h>


// ONC RPC (RFC 5531) and MOUNT protocol (RFC 1813 Appendix I) message
// building and parsing.
//
// This file deliberately knows nothing about sockets or about the Interface
// Kit: it only turns buffers into values and back. That keeps the wire
// format testable off-target -- the same functions can be driven against a
// real server from a plain POSIX harness -- and keeps the transport
// decisions in ExportBrowser.cpp.
//
// All integers on the wire are big-endian 4-byte quantities. Strings are
// a 4-byte length followed by the bytes, zero-padded up to a 4-byte
// boundary.

// Portmapper: program 100000, version 2, PMAPPROC_GETPORT is procedure 3.
static const uint32 kPortMapProgram		= 100000;
static const uint32 kPortMapVersion		= 2;
static const uint32 kPortMapProcGetPort	= 3;

// MOUNT: program 100005. Procedure 5 is EXPORT in both version 1 and 3.
static const uint32 kMountProgram		= 100005;
static const uint32 kMountVersion1		= 1;
static const uint32 kMountVersion3		= 3;
static const uint32 kMountProcExport	= 5;

// RFC 1813: dirpath is at most MNTPATHLEN, name at most MNTNAMLEN. Used to
// reject implausible lengths from a hostile or broken server.
static const uint32 kMountPathLength	= 1024;
static const uint32 kMountNameLength	= 255;


/*!	Called once per export found. \a clients is the server's group list,
	already flattened to a comma-separated string, and is empty when the
	server named no groups.
*/
typedef void (*RPCExportCallback)(void* cookie, const char* path,
	const char* clients);


// Big-endian 4-byte accessors.
void		RPCPutUInt32(uint8* buffer, uint32 value);
uint32		RPCGetUInt32(const uint8* buffer);

/*!	Builds a portmap GETPORT call asking where \a program version \a version
	listens for TCP.
*/
status_t	RPCBuildGetPortCall(uint32 program, uint32 version,
				uint8* buffer, size_t capacity, size_t* size);

/*!	Reads the port out of a GETPORT reply. A registered port of 0 means "not
	registered" and is returned as B_OK with \a port set to 0.
*/
status_t	RPCParseGetPortReply(const uint8* buffer, size_t size,
				uint16* port);

//! Builds a MOUNT EXPORT call. The procedure takes no arguments.
status_t	RPCBuildExportCall(uint32 mountVersion, uint8* buffer,
				size_t capacity, size_t* size);

/*!	Walks the export list in a MOUNT EXPORT reply, invoking \a callback once
	per entry.
*/
status_t	RPCParseExportReply(const uint8* buffer, size_t size,
				RPCExportCallback callback, void* cookie);


#endif	// NFS_MOUNT_EXPORT_PROTOCOL_H
