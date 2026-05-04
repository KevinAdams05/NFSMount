/*
 * Copyright 2026, Kevin Adams. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ShareItem.h"

#include <ColumnTypes.h>

#include "Constants.h"


ShareItem::ShareItem(const BMessage* share, int32 index)
	:
	BRow(),
	fIndex(index),
	fMounted(false)
{
	if (share != NULL) {
		share->FindString(kFieldName, &fName);
		share->FindString(kFieldServer, &fServer);
		share->FindString(kFieldExport, &fExport);
		share->FindString(kFieldMountPoint, &fMountPoint);
	}

	_SetFields();
}


ShareItem::~ShareItem()
{
}


void
ShareItem::Update(const BMessage* share)
{
	if (share == NULL)
		return;

	share->FindString(kFieldName, &fName);
	share->FindString(kFieldServer, &fServer);
	share->FindString(kFieldExport, &fExport);
	share->FindString(kFieldMountPoint, &fMountPoint);

	_SetFields();
}


void
ShareItem::UpdateStatus(bool mounted)
{
	fMounted = mounted;
	SetField(new BStringField(mounted ? "Mounted" : "Unmounted"),
		kStatusColumn);
}


void
ShareItem::_SetFields()
{
	SetField(new BStringField(fName.String()), kNameColumn);
	SetField(new BStringField(fServer.String()), kServerColumn);
	SetField(new BStringField(fExport.String()), kExportColumn);
	SetField(new BStringField(fMounted ? "Mounted" : "Unmounted"),
		kStatusColumn);
}
