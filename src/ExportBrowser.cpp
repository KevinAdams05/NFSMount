/*
 * NFSMount: A native Haiku application for managing NFS shares.
 *
 * Copyright 2026, Kevin Adams <kevinadams05@gmail.com>
 * All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include "ExportBrowser.h"

#include <string.h>

#include <new>

#include <NetworkAddress.h>
#include <Socket.h>

#include "ExportProtocol.h"


// The portmapper always listens here; everything else is discovered.
static const uint16 kPortMapperPort = 111;

// Enough for a few hundred exports. A server offering more than this is
// outside anything this dialog could usefully display, and the cap keeps a
// malicious or broken server from making us allocate without bound.
static const size_t kMaxReplySize = 256 * 1024;


/*!	Collects one parsed export into the caller's list.

	Passed to RPCParseExportReply() so the protocol layer stays free of
	Interface Kit types.
*/
static void
AddExport(void* cookie, const char* path, const char* clients)
{
	NFSExportList* exports = static_cast<NFSExportList*>(cookie);

	NFSExport* entry = new (std::nothrow) NFSExport;
	if (entry == NULL)
		return;

	entry->path = path;
	entry->clients = clients;

	if (!exports->AddItem(entry))
		delete entry;
}


/*!	Sends one RPC message and reads the reply, both with TCP record marking.

	Record marking (RFC 5531 §11) frames each message as one or more
	fragments: a 4-byte big-endian header whose top bit marks the last
	fragment and whose low 31 bits give that fragment's length. Servers are
	free to split a reply, so the read loop reassembles until it sees the
	last-fragment bit.
*/
static status_t
SendAndReceive(BSocket& socket, const uint8* request, size_t requestSize,
	uint8* reply, size_t replyCapacity, size_t* replySize)
{
	uint8 header[4];
	RPCPutUInt32(header, 0x80000000 | (uint32)requestSize);

	ssize_t written = socket.Write(header, sizeof(header));
	if (written < 0)
		return (status_t)written;
	written = socket.Write(request, requestSize);
	if (written < 0)
		return (status_t)written;

	size_t total = 0;
	bool lastFragment = false;

	while (!lastFragment) {
		uint8 fragmentHeader[4];
		size_t got = 0;
		while (got < sizeof(fragmentHeader)) {
			ssize_t bytes = socket.Read(fragmentHeader + got,
				sizeof(fragmentHeader) - got);
			if (bytes <= 0)
				return bytes < 0 ? (status_t)bytes : B_IO_ERROR;
			got += bytes;
		}

		uint32 marker = RPCGetUInt32(fragmentHeader);
		lastFragment = (marker & 0x80000000) != 0;
		size_t fragmentSize = marker & 0x7fffffff;

		// Refuse absurd lengths rather than trusting the peer.
		if (fragmentSize > replyCapacity || total + fragmentSize > replyCapacity)
			return B_BUFFER_OVERFLOW;

		got = 0;
		while (got < fragmentSize) {
			ssize_t bytes = socket.Read(reply + total + got,
				fragmentSize - got);
			if (bytes <= 0)
				return bytes < 0 ? (status_t)bytes : B_IO_ERROR;
			got += bytes;
		}
		total += fragmentSize;
	}

	*replySize = total;
	return B_OK;
}


/*!	Opens a bounded TCP connection to \a server on \a port.

	BSocket::Connect honors the timeout and SetTimeout bounds the subsequent
	reads and writes, so no step here can hang the calling thread the way
	fs_mount_volume() can.
*/
static status_t
ConnectBounded(BSocket& socket, const char* server, uint16 port,
	bigtime_t timeout)
{
	BNetworkAddress address;
	status_t result = address.SetTo(server, port);
	if (result != B_OK)
		return result;

	result = socket.Connect(address, timeout);
	if (result != B_OK)
		return result;

	socket.SetTimeout(timeout);
	return B_OK;
}


/*!	Asks the portmapper which port serves \a program version \a version.

	Returns 0 in \a port (with B_OK) when the program is not registered --
	that is a valid answer, not an error, and the caller decides what to do
	about it.
*/
static status_t
QueryPort(const char* server, uint32 program, uint32 version,
	bigtime_t timeout, uint16* port)
{
	BSocket socket;
	status_t result = ConnectBounded(socket, server, kPortMapperPort,
		timeout);
	if (result != B_OK)
		return result;

	uint8 request[128];
	size_t requestSize = 0;
	result = RPCBuildGetPortCall(program, version, request, sizeof(request),
		&requestSize);
	if (result != B_OK)
		return result;

	uint8 reply[512];
	size_t replySize = 0;
	result = SendAndReceive(socket, request, requestSize, reply,
		sizeof(reply), &replySize);
	if (result != B_OK)
		return result;

	return RPCParseGetPortReply(reply, replySize, port);
}


status_t
ExportBrowser::Fetch(const char* server, bigtime_t timeout,
	NFSExportList& exports)
{
	if (server == NULL || *server == '\0')
		return B_BAD_VALUE;

	// Prefer MOUNT version 3, but fall back to version 1. Both define
	// procedure 5 as EXPORT returning the same export list, and some
	// servers register only the older version.
	uint32 mountVersion = kMountVersion3;
	uint16 mountPort = 0;

	status_t result = QueryPort(server, kMountProgram, kMountVersion3,
		timeout, &mountPort);
	if (result != B_OK)
		return result;

	if (mountPort == 0) {
		mountVersion = kMountVersion1;
		result = QueryPort(server, kMountProgram, kMountVersion1, timeout,
			&mountPort);
		if (result != B_OK)
			return result;
	}

	if (mountPort == 0) {
		// The server answered the portmapper query but offers no MOUNT
		// service. Typical of an NFSv4-only export, where the MOUNT
		// protocol no longer exists.
		return B_UNSUPPORTED;
	}

	BSocket socket;
	result = ConnectBounded(socket, server, mountPort, timeout);
	if (result != B_OK)
		return result;

	uint8 request[128];
	size_t requestSize = 0;
	result = RPCBuildExportCall(mountVersion, request, sizeof(request),
		&requestSize);
	if (result != B_OK)
		return result;

	uint8* reply = new (std::nothrow) uint8[kMaxReplySize];
	if (reply == NULL)
		return B_NO_MEMORY;

	size_t replySize = 0;
	result = SendAndReceive(socket, request, requestSize, reply,
		kMaxReplySize, &replySize);
	if (result == B_OK) {
		result = RPCParseExportReply(reply, replySize, &AddExport,
			&exports);
	}

	delete[] reply;
	return result;
}


BString
ExportBrowser::ErrorString(status_t status)
{
	BString message;

	switch (status) {
		case B_UNSUPPORTED:
			message = "The server did not offer a MOUNT service. Servers "
				"that only speak NFS version 4 cannot list their exports "
				"this way; type the export path manually.";
			break;
		case B_TIMED_OUT:
			message = "The server did not respond in time. Check that it is "
				"reachable and that port 111 is not blocked by a firewall.";
			break;
		case B_BUFFER_OVERFLOW:
			message = "The server returned more data than expected. Its "
				"export list could not be read.";
			break;
		case B_BAD_DATA:
			message = "The server's reply could not be understood. It may "
				"not be an NFS server.";
			break;
		case B_BAD_VALUE:
			message = "Enter a server address first.";
			break;
		default:
			message = "Could not reach the server: ";
			message << strerror(status);
			break;
	}

	return message;
}
