/*
 * NFSMount: A native Haiku application for managing NFS shares.
 *
 * Copyright 2026, Kevin Adams <kevinadams05@gmail.com>
 * All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef NFS_MOUNT_EXPORT_BROWSER_H
#define NFS_MOUNT_EXPORT_BROWSER_H


#include <ObjectList.h>
#include <String.h>
#include <SupportDefs.h>


/*!	One entry from a server's export list.

	\c path is the exported directory as the server names it, which is what
	goes in the share's Export field. \c clients is the server's own
	description of who may mount it -- RFC 1813 calls these names
	"implementation-specific and cannot be directly interpreted by clients",
	so it is shown to the user for information only and never parsed.
*/
struct NFSExport {
			BString				path;
			BString				clients;
};


typedef BObjectList<NFSExport, true> NFSExportList;


/*!	Asks an NFS server which file systems it exports.

	This is the MOUNT protocol's EXPORT procedure -- the same thing
	"showmount -e <server>" does. Two RPC calls over TCP:

	  1. portmap PMAPPROC_GETPORT (program 100000 v2, procedure 3) on port
	     111, asking where the MOUNT program lives.
	  2. MOUNTPROC3_EXPORT (program 100005 v3, procedure 5) on the port that
	     came back, which returns the export list.

	Both are defined by RFC 1813 Appendix I (MOUNT protocol) and RFC 5531
	(ONC RPC). Protocol numbers cross-checked against Haiku's own legacy NFS
	add-on, headers/.../file_systems/nfs/{pmap.h,mount.h}.

	Caveats worth knowing before relying on this:

	- The MOUNT protocol belongs to NFS v2/v3. An NFSv4-only server need not
	  run rpcbind or mountd at all, in which case there is nothing to ask and
	  Fetch() returns B_UNSUPPORTED. That is a normal answer, not a bug.
	- Every step is bounded by the caller's timeout. Nothing here may block
	  indefinitely: it is called from a worker thread on behalf of a window,
	  and an unreachable or firewalled server must not wedge the UI.
*/
class ExportBrowser {
public:
	/*!	Retrieves \a server's export list into \a exports.

		Blocking; call from a worker thread, never from a window thread.
		\a timeout bounds each network operation individually.

		Returns \c B_OK with \a exports filled (possibly empty -- a server
		may legitimately export nothing), \c B_UNSUPPORTED if the server
		answers but offers no MOUNT service, or a network error.
	*/
	static	status_t			Fetch(const char* server, bigtime_t timeout,
									NFSExportList& exports);

	/*!	Turns the status codes Fetch() can return into something worth
		showing a user, rather than "Error -2147459072".
	*/
	static	BString				ErrorString(status_t status);
};


#endif	// NFS_MOUNT_EXPORT_BROWSER_H
