/*
 * Copyright 2026, Kevin Adams. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "AboutBox.h"

#include <AboutWindow.h>


// MIT license text — embedded verbatim so the About box is self-
// contained. Kept in sync with the LICENSE file at the project root.
static const char* const kMITLicenseText =
	"Copyright (c) 2026 Kevin Adams\n"
	"\n"
	"Permission is hereby granted, free of charge, to any person "
	"obtaining a copy of this software and associated documentation "
	"files (the \"Software\"), to deal in the Software without "
	"restriction, including without limitation the rights to use, "
	"copy, modify, merge, publish, distribute, sublicense, and/or "
	"sell copies of the Software, and to permit persons to whom the "
	"Software is furnished to do so, subject to the following "
	"conditions:\n"
	"\n"
	"The above copyright notice and this permission notice shall be "
	"included in all copies or substantial portions of the Software."
	"\n\n"
	"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY "
	"KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE "
	"WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR "
	"PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR "
	"COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER "
	"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, "
	"ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE "
	"USE OR OTHER DEALINGS IN THE SOFTWARE.";


// Acknowledgements for libraries we link against. Haiku's own kits
// (be, tracker, columnlistview, localestub, root) are MIT-licensed
// and ship with the OS, so they don't strictly require attribution
// in dependent apps — but listing them is a courtesy.
static const char* const kThirdPartyText =
	"NFSMount is built with the Haiku Interface Kit, Storage Kit, "
	"and Tracker (BFilePanel, BColumnListView), all distributed "
	"under the MIT License as part of the Haiku operating system.\n"
	"\n"
	"The C++ runtime (libstdc++, libgcc) is provided by GCC under "
	"the GPL with the GCC Runtime Library Exception, which permits "
	"its use in non-GPL applications such as this one.\n"
	"\n"
	"No third-party code is statically embedded in NFSMount.";


void
show_about_window()
{
	BAboutWindow* about = new BAboutWindow("NFSMount",
		"application/x-vnd.NFSMount");

	about->SetVersion("0.0.5");

	about->AddDescription(
		"A native Haiku application for managing NFS network shares.\n"
		"\n"
		"Mount, unmount, save, and auto-mount NFSv4 (and NFSv2) "
		"shares through a graphical interface instead of typing "
		"terminal commands every session.");

	// AddCopyright takes the *first* copyright year. Haiku's About
	// dialog widens it to a "first-current" range automatically;
	// when first == current it just shows the single year.
	about->AddCopyright(2026, "Kevin Adams");

	const char* websites[] = {
		"https://github.com/KevinAdams05/HaikuTools/tree/main/NFSMount",
		NULL
	};
	about->AddText("Source code", websites);

	const char* licenseLines[] = { kMITLicenseText, NULL };
	about->AddText("License", licenseLines);

	const char* thanksLines[] = { kThirdPartyText, NULL };
	about->AddText("Acknowledgements", thanksLines);

	about->Show();
}
