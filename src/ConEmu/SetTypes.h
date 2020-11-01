
/*
Copyright (c) 2014-present Maximus5
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

#include "../common/defines.h"

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
		// only for inactive cursor, if true - use inactive style when cursor is invisible
		unsigned int   Invisible    : 1;
	};

	DWORD Raw; // 32 bits
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

// Allow or not to convert pasted Windows path into Posix notation
typedef BYTE PosixPasteMode;
const PosixPasteMode
	pxm_Convert    = 1,  // always try to convert
	pxm_Intact     = 2,  // never convert
	pxm_Auto       = 0   // autoselect on certain conditions and m_Args.pszMntRoot value
;

typedef BYTE PasteLinesMode;
const PasteLinesMode
	plm_Default    = 1,
	plm_MultiLine  = 2,
	plm_SingleLine = 3,
	plm_FirstLine  = 4,
	plm_Nothing    = 0
;


enum CESizeStyle
{
	ss_Standard = 0,
	ss_Pixels = 1,
	ss_Percents = 2,
};

union CESize
{
	DWORD Raw;

	struct
	{
		int         Value : 24;
		CESizeStyle Style : 8;

		wchar_t TempSZ[12];
	};

	const wchar_t* AsString()
	{
		switch (Style)
		{
		case ss_Pixels:
			swprintf_c(TempSZ, L"%ipx", Value);
			break;
		case ss_Percents:
			swprintf_c(TempSZ, L"%i%%", Value);
			break;
			//case ss_Standard:
		default:
			swprintf_c(TempSZ, L"%i", Value);
		}
		return TempSZ;
	};

	bool IsValid(bool IsWidth) const
	{
		bool bValid;
		switch (Style)
		{
		case ss_Percents:
			bValid = (Value >= 1 && Value <= 100);
			break;
		case ss_Pixels:
			// Treat width/height as values for font size 4x2 (minimal)
			if (IsWidth)
				bValid = (Value >= (MIN_CON_WIDTH * 4));
			else
				bValid = (Value >= (MIN_CON_HEIGHT * 2));
			break;
		default:
			if (IsWidth)
				bValid = (Value >= MIN_CON_WIDTH);
			else
				bValid = (Value >= MIN_CON_HEIGHT);
		}
		return bValid;
	};

	bool Set(bool IsWidth, CESizeStyle NewStyle, int NewValue)
	{
		if (NewStyle == ss_Standard)
		{
			int nDef = IsWidth ? 80 : 25;
			int nMax = IsWidth ? 1000 : 500;
			if (NewValue <= 0) NewValue = nDef; else if (NewValue > nMax) NewValue = nMax;
		}
		else if (NewStyle == ss_Percents)
		{
			int nDef = IsWidth ? 50 : 30;
			int nMax = 100;
			if (NewValue <= 0) NewValue = nDef; else if (NewValue > nMax) NewValue = nMax;
		}

		if (!NewValue)
		{
			// Size can't be empty
			_ASSERTE(NewValue);  // -V571
			// Fail
			return false;
		}

		Value = NewValue;
		Style = NewStyle;
		return true;
	};

	void SetFromRaw(bool IsWidth, DWORD aRaw)
	{
		CESize v; v.Raw = aRaw;
		if (v.Style == ss_Standard || v.Style == ss_Pixels || v.Style == ss_Percents)
		{
			this->Set(IsWidth, v.Style, v.Value);
		}
	};

	bool SetFromString(bool IsWidth, const wchar_t* sValue)
	{
		if (!sValue || !*sValue)
			return false;
		wchar_t* pszEnd = nullptr;
		// Try to convert
		int NewValue = wcstol(sValue, &pszEnd, 10);
		if (!NewValue)
			return false;

		CESizeStyle NewStyle = ss_Standard;
		if (pszEnd)
		{
			switch (*SkipNonPrintable(pszEnd))
			{
			case L'%':
				NewStyle = ss_Percents; break;
			case L'p':
				NewStyle = ss_Pixels; break;
			}
		}
		// Done
		return Set(IsWidth, NewStyle, NewValue);
	};
};
