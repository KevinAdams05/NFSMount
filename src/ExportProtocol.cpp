/*
 * NFSMount: A native Haiku application for managing NFS shares.
 *
 * Copyright 2026, Kevin Adams <kevinadams05@gmail.com>
 * All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include "ExportProtocol.h"

#include <string.h>


// RPC message fields (RFC 5531).
static const uint32 kRpcVersion			= 2;
static const uint32 kRpcCall			= 0;
static const uint32 kRpcReply			= 1;
static const uint32 kRpcMsgAccepted		= 0;
static const uint32 kRpcSuccess			= 0;
static const uint32 kRpcAuthNull		= 0;

// IANA protocol number, as the portmapper expects it.
static const uint32 kProtocolTCP		= 6;

// The transaction id only has to be unique among a client's outstanding
// calls. Every call here is synchronous -- send, then read the reply on the
// same connection -- so a counter is sufficient and avoids pulling in a
// random source. It starts at an arbitrary value purely so captures are
// easier to tell apart.
static uint32 sTransactionID = 0x4e46534d;	// 'NFSM'


/*!	Bounds-checked cursor over a reply buffer.

	Every read is checked against the end of the buffer, so a truncated or
	malicious reply produces B_BAD_DATA rather than reading past the end.
*/
struct XDRReader {
			const uint8*		data;
			size_t				size;
			size_t				offset;
			bool				valid;

								XDRReader(const uint8* buffer, size_t length)
									:
									data(buffer),
									size(length),
									offset(0),
									valid(true)
								{
								}

			uint32				ReadUInt32()
								{
									if (!valid || offset + 4 > size) {
										valid = false;
										return 0;
									}
									uint32 value = RPCGetUInt32(data
										+ offset);
									offset += 4;
									return value;
								}

			/*!	Reads an XDR string into \a target, which must hold
								\a targetSize bytes including the terminator.
								Skips the padding to the next 4-byte boundary.
							*/
			bool				ReadString(char* target, size_t targetSize,
									uint32 maximum)
								{
									uint32 length = ReadUInt32();
									if (!valid)
										return false;
									if (length > maximum
										|| offset + length > size) {
										valid = false;
										return false;
									}

									size_t copy = length;
									if (copy > targetSize - 1)
										copy = targetSize - 1;
									memcpy(target, data + offset, copy);
									target[copy] = '\0';

									// XDR pads strings out to a multiple of 4.
									size_t padded = (length + 3) & ~(size_t)3;
									if (offset + padded > size) {
										valid = false;
										return false;
									}
									offset += padded;
									return true;
								}
};


void
RPCPutUInt32(uint8* buffer, uint32 value)
{
	buffer[0] = (uint8)(value >> 24);
	buffer[1] = (uint8)(value >> 16);
	buffer[2] = (uint8)(value >> 8);
	buffer[3] = (uint8)value;
}


uint32
RPCGetUInt32(const uint8* buffer)
{
	return ((uint32)buffer[0] << 24) | ((uint32)buffer[1] << 16)
		| ((uint32)buffer[2] << 8) | (uint32)buffer[3];
}


/*!	Writes the header every RPC call starts with: transaction id, message
	type, RPC version, then the program/version/procedure being called, then
	null credentials and verifier.
*/
static status_t
BuildCallHeader(uint32 program, uint32 version, uint32 procedure,
	uint8* buffer, size_t capacity, size_t* offset)
{
	const size_t kHeaderSize = 10 * 4;
	if (capacity < kHeaderSize)
		return B_BUFFER_OVERFLOW;

	size_t at = 0;
	RPCPutUInt32(buffer + at, sTransactionID++);
	at += 4;
	RPCPutUInt32(buffer + at, kRpcCall);
	at += 4;
	RPCPutUInt32(buffer + at, kRpcVersion);
	at += 4;
	RPCPutUInt32(buffer + at, program);
	at += 4;
	RPCPutUInt32(buffer + at, version);
	at += 4;
	RPCPutUInt32(buffer + at, procedure);
	at += 4;

	// AUTH_NULL credentials and verifier: flavor 0, zero-length body. The
	// EXPORT procedure is public, so nothing stronger is needed.
	RPCPutUInt32(buffer + at, kRpcAuthNull);
	at += 4;
	RPCPutUInt32(buffer + at, 0);
	at += 4;
	RPCPutUInt32(buffer + at, kRpcAuthNull);
	at += 4;
	RPCPutUInt32(buffer + at, 0);
	at += 4;

	*offset = at;
	return B_OK;
}


/*!	Validates a reply header and leaves \a reader positioned at the results.

	Checks that this is a reply, that the server accepted it, and that the
	call succeeded, rejecting anything else as B_BAD_DATA. The transaction id
	is not matched: each call gets a fresh connection and a single
	synchronous reply, so there is no other response it could be confused
	with.
*/
static status_t
CheckReplyHeader(XDRReader& reader)
{
	reader.ReadUInt32();							// transaction id

	if (reader.ReadUInt32() != kRpcReply)
		return B_BAD_DATA;
	if (reader.ReadUInt32() != kRpcMsgAccepted)
		return B_BAD_DATA;

	// Verifier: flavor then an opaque body we do not need.
	reader.ReadUInt32();
	uint32 verifierLength = reader.ReadUInt32();
	if (!reader.valid)
		return B_BAD_DATA;
	size_t padded = (verifierLength + 3) & ~(size_t)3;
	if (reader.offset + padded > reader.size)
		return B_BAD_DATA;
	reader.offset += padded;

	if (reader.ReadUInt32() != kRpcSuccess)
		return B_BAD_DATA;

	return reader.valid ? B_OK : B_BAD_DATA;
}


status_t
RPCBuildGetPortCall(uint32 program, uint32 version, uint8* buffer,
	size_t capacity, size_t* size)
{
	size_t at = 0;
	status_t result = BuildCallHeader(kPortMapProgram, kPortMapVersion,
		kPortMapProcGetPort, buffer, capacity, &at);
	if (result != B_OK)
		return result;

	// struct pmap { prog; vers; prot; port; } -- port is 0 in a query.
	if (capacity < at + 16)
		return B_BUFFER_OVERFLOW;
	RPCPutUInt32(buffer + at, program);
	at += 4;
	RPCPutUInt32(buffer + at, version);
	at += 4;
	RPCPutUInt32(buffer + at, kProtocolTCP);
	at += 4;
	RPCPutUInt32(buffer + at, 0);
	at += 4;

	*size = at;
	return B_OK;
}


status_t
RPCParseGetPortReply(const uint8* buffer, size_t size, uint16* port)
{
	XDRReader reader(buffer, size);

	status_t result = CheckReplyHeader(reader);
	if (result != B_OK)
		return result;

	uint32 value = reader.ReadUInt32();
	if (!reader.valid)
		return B_BAD_DATA;
	if (value > 0xffff)
		return B_BAD_DATA;

	*port = (uint16)value;
	return B_OK;
}


status_t
RPCBuildExportCall(uint32 mountVersion, uint8* buffer, size_t capacity,
	size_t* size)
{
	size_t at = 0;
	status_t result = BuildCallHeader(kMountProgram, mountVersion,
		kMountProcExport, buffer, capacity, &at);
	if (result != B_OK)
		return result;

	// EXPORT takes no arguments.
	*size = at;
	return B_OK;
}


status_t
RPCParseExportReply(const uint8* buffer, size_t size,
	RPCExportCallback callback, void* cookie)
{
	XDRReader reader(buffer, size);

	status_t result = CheckReplyHeader(reader);
	if (result != B_OK)
		return result;

	// The export list is an XDR optional-linked-list: each node is preceded
	// by a boolean saying whether another node follows. RFC 1813:
	//
	//     struct exportnode { dirpath ex_dir; groups ex_groups;
	//         exports ex_next; };
	//     struct groupnode { name gr_name; groups gr_next; };
	//
	// so the shape is: while (more) { path; while (more) { group } }.
	while (true) {
		uint32 hasExport = reader.ReadUInt32();
		if (!reader.valid)
			return B_BAD_DATA;
		if (hasExport == 0)
			break;

		char path[kMountPathLength + 1];
		if (!reader.ReadString(path, sizeof(path), kMountPathLength))
			return B_BAD_DATA;

		// Flatten the group list into something displayable. A server that
		// exports to everyone typically reports a single "*" group.
		char clients[512];
		clients[0] = '\0';
		size_t clientsUsed = 0;

		while (true) {
			uint32 hasGroup = reader.ReadUInt32();
			if (!reader.valid)
				return B_BAD_DATA;
			if (hasGroup == 0)
				break;

			char group[kMountNameLength + 1];
			if (!reader.ReadString(group, sizeof(group), kMountNameLength))
				return B_BAD_DATA;

			size_t groupLength = strlen(group);
			size_t separator = clientsUsed > 0 ? 2 : 0;
			if (clientsUsed + separator + groupLength < sizeof(clients)) {
				if (separator > 0) {
					memcpy(clients + clientsUsed, ", ", 2);
					clientsUsed += 2;
				}
				memcpy(clients + clientsUsed, group, groupLength);
				clientsUsed += groupLength;
				clients[clientsUsed] = '\0';
			}
			// Groups beyond the buffer are dropped rather than truncating
			// mid-name; the field is informational only.
		}

		if (callback != NULL)
			callback(cookie, path, clients);
	}

	return B_OK;
}
