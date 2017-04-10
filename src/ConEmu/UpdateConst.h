
/*
Copyright (c) 2011-2017 Maximus5
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#define CV_STABLE  L"stable"
#define CV_PREVIEW L"preview"
#define CV_DEVEL   L"alpha"
#define CV_Stable  L"stable"
#define CV_Preview L"preview"
#define CV_Devel   L"alpha"

#define sectionConEmuStable  L"ConEmu_Stable_2"
#define sectionConEmuPreview L"ConEmu_Preview_2"
#define sectionConEmuDevel   L"ConEmu_Devel_2"

#define szWhatsNewLabel L"Whats new (project wiki page)"

#define szRetryVersionIniCheck \
	L"ConEmu is unable to load current version information from servers.\n" \
	L"You may either check and download new versions manually from\n" \
	CEDOWNLPAGE /* http://www.fosshub.com/ConEmu.html */ L"\n" \
	L"or let ConEmu retry the check.\n"

#define szRetryPackageDownload \
	L"ConEmu is unable to download update package.\n" \
	L"You may either download new versions manually from\n" \
	CEDOWNLPAGE /* http://www.fosshub.com/ConEmu.html */ L"\n" \
	L"or let ConEmu retry the downloading.\n"
