
/*
Copyright (c) 2014-2017 Maximus5
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

#include <windows.h>

struct FindTextOptions
{
	size_t   cchTextMax;
	wchar_t* pszText;
	bool     bMatchCase;
	bool     bMatchWholeWords;
	bool     bFreezeConsole;
	bool     bHighlightAll;
	bool     bTransparent;
};


enum CECursorStyle
{
	cur_Horz         = 0x00,
	cur_Vert         = 0x01,
	cur_Block        = 0x02,
	cur_Rect         = 0x03,

	// Used for Min/Max
	cur_First        = cur_Horz,
	cur_Last         = cur_Rect,
};

union CECursorType
{
	struct
	{
		CECursorStyle  CursorType   : 6;
		unsigned int   isBlinking   : 1;
		unsigned int   isColor      : 1;
		unsigned int   isFixedSize  : 1;
		unsigned int   FixedSize    : 7;
		unsigned int   MinSize      : 7;
		// set to true for use distinct settings for Inactive cursor
		unsigned int   Used         : 1;
	};

	DWORD Raw;
};

enum TabStyle
{
	ts_VS2008 = 0,
	ts_Win8   = 1,
};

typedef DWORD SettingsLoadedFlags;
const SettingsLoadedFlags
	slf_NeedCreateVanilla = 0x0001, // Call gpSet->SaveSettings after initializing
	slf_AllowFastConfig   = 0x0002,
	slf_OnStartupLoad     = 0x0004,
	slf_OnResetReload     = 0x0008,
	slf_DefaultSettings   = 0x0010,
	slf_AppendTasks       = 0x0020,
	slf_DefaultTasks      = 0x0040, // if config is partial (no tasks, no command line)
	slf_RewriteExisting   = 0x0080,
	slf_None              = 0x0000
;

enum AdminTabStyle
{
	ats_Empty        = 0,
	ats_Shield       = 1,
	ats_ShieldSuffix = 3,
	ats_Disabled     = 4,
};
