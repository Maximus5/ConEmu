
/*
Copyright (c) 2012-2016 Maximus5
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

#include "../common/ConsoleAnnotation.h"
#include "../common/pluginW1900.hpp"

#include "../common/ConsoleTrueMod.h"
//typedef unsigned __int64 CECOLORFLAGS;
//static const CECOLORFLAGS
//	CECF_FG_24BIT      = 0x0000000000000001ULL,
//	CECF_BG_24BIT      = 0x0000000000000002ULL,
//	CECF_24BITMASK     = CECF_FG_24BIT|CECF_BG_24BIT,
//
//	CECF_FG_BOLD       = 0x1000000000000000ULL,
//	CECF_FG_ITALIC     = 0x2000000000000000ULL,
//	CECF_FG_UNDERLINE  = 0x4000000000000000ULL,
//	CECF_STYLEMASK     = CECF_FG_BOLD|CECF_FG_ITALIC|CECF_FG_UNDERLINE,
//
//	CECF_NONE          = 0;
//
//struct ConEmuColor
//{
//	CECOLORFLAGS Flags;
//	COLORREF ForegroundColor;
//	COLORREF BackgroundColor;
//};


#if 0
#define INDEXMASK 0x0000000F
#define COLORMASK 0x00FFFFFF

#define INDEXVALUE(x) ((x)&INDEXMASK)
#define COLORVALUE(x) ((x)&COLORMASK)
#endif

struct ExtAttributesParm
{
	size_t StructSize;
	HANDLE ConsoleOutput;
	ConEmuColor Attributes;
};

BOOL ExtGetAttributes(ExtAttributesParm* Info);
BOOL ExtSetAttributes(const ExtAttributesParm* Info);


typedef unsigned __int64 EXTREADWRITEFLAGS;
static const EXTREADWRITEFLAGS
	ewtf_FarClr  = 0x0000000000000001ULL,
	ewtf_CeClr   = 0x0000000000000002ULL,
	ewtf_AiClr   = 0x0000000000000003ULL,
	ewtf_Mask    = ewtf_FarClr|ewtf_CeClr|ewtf_AiClr,

	ewtf_Current = 0x0000000000000010ULL, // Use current color (may be extended) selected in console

	ewtf_WrapAt  = 0x0000000000000020ULL, // Force wrap at specific position (WrapAtCol)

	ewtf_Region  = 0x0000000000000040ULL, // Top/Bottom lines are defined, take into account when "\r\n"

	ewtf_DontWrap= 0x0000000000000080ULL, // tmux, status line

	ewtf_Commit  = 0x0000000000000100ULL, // Only for Write functions

	ewtf_NoBells = 0x0000000000000200ULL, // CECF_SuppressBells

	ewtf_None    = 0x0000000000000000ULL;

struct ExtWriteTextParm
{
	size_t StructSize;
	EXTREADWRITEFLAGS Flags;
	HANDLE ConsoleOutput;
	union
	{
		FarColor FARColor;
		ConEmuColor CEColor;
		AnnotationInfo AIColor;
	};
	const wchar_t* Buffer;
	DWORD NumberOfCharsToWrite;
	DWORD NumberOfCharsWritten;
	SHORT WrapAtCol; // 1-based. Used only when ewtf_WrapAt specified
	SHORT ScrolledRowsUp; // При достижении нижней границы экрана происходит скролл
	void* Private;   // ConEmu private usage !!! Must be NULL !!!
	RECT  Region;    // ewtf_Region, take into account when "\r\n"
};

BOOL ExtWriteText(ExtWriteTextParm* Info);

union ConEmuCharBuffer
{
	LPBYTE RAW;
	FAR_CHAR_INFO* FARBuffer;
	struct
	{
		CHAR_INFO* CEBuffer;
		union
		{
			ConEmuColor* CEColor;
			AnnotationInfo* AIColor;
		};
	};
	int BufferType; // ewtf_Mask

	void Inc(size_t Cells = 1);
};

struct ExtReadWriteOutputParm
{
	size_t StructSize;
	EXTREADWRITEFLAGS Flags;
	HANDLE ConsoleOutput;
	// Where
	COORD BufferSize;
	COORD BufferCoord;
	SMALL_RECT Region;
	// What
	ConEmuCharBuffer Buffer;
};

BOOL WINAPI ExtReadOutput(ExtReadWriteOutputParm* Info);

BOOL WINAPI ExtWriteOutput(ExtReadWriteOutputParm* Info);




#if 0
typedef unsigned __int64 EXTSETTEXTMODEFLAGS;
static const EXTSETTEXTMODEFLAGS
	estm_Clipping = 0x0000000000000001ULL, // wrap when cursor beyond dwSize.X instead of BufferWidth
	estm_None     = 0x0000000000000000ULL;

struct ExtSetTextMode
{
	size_t StructSize;
	EXTSETTEXTMODEFLAGS Flags;
	COORD dwSize;
};
#endif

typedef unsigned __int64 EXTFILLOUTPUTFLAGS;
static const EXTFILLOUTPUTFLAGS
	efof_Attribute = 0x0000000000000001ULL, // FillConsoleOutputAttribute
	efof_Character = 0x0000000000000002ULL, // FillConsoleOutputCharacter

	efof_Current   = 0x0000000000000010ULL, // Use current color (may be extended) selected in console
	efof_ResetExt  = 0x0000000000000020ULL, // Reset extended attributes

	efof_None      = 0x0000000000000000ULL;

struct ExtFillOutputParm
{
	size_t StructSize;
	EXTFILLOUTPUTFLAGS Flags;
	HANDLE ConsoleOutput;
	// Fill region with
	ConEmuColor FillAttr;
	wchar_t FillChar;
	// Where
	COORD Coord;
	DWORD Count;
};

BOOL ExtFillOutput(ExtFillOutputParm* Info);

typedef unsigned __int64 EXTSCROLLSCREENFLAGS;
static const EXTSCROLLSCREENFLAGS
	essf_Pad      = 0x0000000000000001ULL, // pad with spaces to right edge first
	essf_ExtOnly  = 0x0000000000000002ULL, // scroll extended attrs only (don't touch real console text/attr)

	essf_Current  = 0x0000000000000010ULL, // Use current color (may be extended) selected in console

	essf_Region   = 0x0000000000000020ULL, // Scroll region is defined. If not - scroll whole screen (visible buffer?)
	essf_Global   = 0x0000000000000080ULL, // Global coordinates are used

	essf_Commit   = 0x0000000000000100ULL,
	essf_None     = 0x0000000000000000ULL;

struct ExtScrollScreenParm
{
	size_t StructSize;
	EXTSCROLLSCREENFLAGS Flags;
	HANDLE ConsoleOutput;

	// (Dir < 0) - Scroll whole page up by n (default 1) lines. New lines are added at the bottom.
	// (Dir > 0) - Scroll whole page down by n (default 1) lines. New lines are added at the top.
	INT_PTR Dir;

	// Fill regions with
	ConEmuColor FillAttr;
	wchar_t FillChar;

	// essf_Region
	RECT Region;
};

BOOL ExtScrollLine(ExtScrollScreenParm* Info);
BOOL ExtScrollScreen(ExtScrollScreenParm* Info);


struct ExtCommitParm
{
	size_t StructSize;
	HANDLE ConsoleOutput;
};

BOOL ExtCommit(ExtCommitParm* Info);

void ExtCloseBuffers();
