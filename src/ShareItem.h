/*
 * Copyright 2026, Kevin Adams. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NFS_MOUNT_SHARE_ITEM_H
#define NFS_MOUNT_SHARE_ITEM_H


#include <ColumnListView.h>
#include <Message.h>
#include <String.h>
#include <SupportDefs.h>


class ShareItem : public BRow {
public:
								ShareItem(const BMessage* share,
									int32 index);
	virtual						~ShareItem();

			void				Update(const BMessage* share);
			void				UpdateStatus(bool mounted);

			int32				Index() const { return fIndex; }
			void				SetIndex(int32 index) { fIndex = index; }
			const char*			MountPoint() const
									{ return fMountPoint.String(); }
			const char*			Name() const { return fName.String(); }
			bool				IsMounted() const { return fMounted; }

private:
			void				_SetFields();

			int32				fIndex;
			BString				fName;
			BString				fServer;
			BString				fExport;
			BString				fMountPoint;
			bool				fMounted;
};


// Column indices
enum {
	kNameColumn		= 0,
	kServerColumn	= 1,
	kExportColumn	= 2,
	kStatusColumn	= 3,
};


#endif
