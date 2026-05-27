/*
 * Copyright 2026, Kevin Adams. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "Settings.h"

#include <File.h>
#include <FindDirectory.h>

#include "Constants.h"


Settings::Settings()
	:
	fSettings('NFSM')
{
	find_directory(B_USER_SETTINGS_DIRECTORY, &fSettingsPath);
	fSettingsPath.Append(kSettingsFileName);
}


Settings::~Settings()
{
}


status_t
Settings::Load()
{
	BFile file(fSettingsPath.Path(), B_READ_ONLY);
	status_t result = file.InitCheck();
	if (result != B_OK)
		return result;

	result = fSettings.Unflatten(&file);
	if (result != B_OK)
		fSettings.MakeEmpty();

	return result;
}


status_t
Settings::Save()
{
	BFile file(fSettingsPath.Path(),
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t result = file.InitCheck();
	if (result != B_OK)
		return result;

	return fSettings.Flatten(&file);
}


BRect
Settings::WindowFrame() const
{
	BRect frame;
	if (fSettings.FindRect(kFieldWindowFrame, &frame) != B_OK)
		frame.Set(80, 80, 700, 410);

	return frame;
}


void
Settings::SetWindowFrame(BRect frame)
{
	if (fSettings.HasRect(kFieldWindowFrame))
		fSettings.ReplaceRect(kFieldWindowFrame, frame);
	else
		fSettings.AddRect(kFieldWindowFrame, frame);
}


int32
Settings::CountShares() const
{
	int32 count = 0;
	type_code type;
	fSettings.GetInfo(kFieldShares, &type, &count);
	return count;
}


status_t
Settings::GetShare(int32 index, BMessage* share) const
{
	if (share == NULL)
		return B_BAD_VALUE;

	return fSettings.FindMessage(kFieldShares, index, share);
}


status_t
Settings::AddShare(const BMessage* share)
{
	if (share == NULL)
		return B_BAD_VALUE;

	return fSettings.AddMessage(kFieldShares, share);
}


status_t
Settings::UpdateShare(int32 index, const BMessage* share)
{
	if (share == NULL)
		return B_BAD_VALUE;

	return fSettings.ReplaceMessage(kFieldShares, index, share);
}


status_t
Settings::RemoveShare(int32 index)
{
	return fSettings.RemoveData(kFieldShares, index);
}
