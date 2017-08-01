
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

enum ConEmuWindowMode
{
	wmCurrent = 0,
	wmNotChanging = -1,
	wmNormal = rNormal,
	wmMaximized = rMaximized,
	wmFullScreen = rFullScreen,
};

enum ConEmuQuakeMode
{
	quake_Disabled = 0,
	quake_Standard = 1,
	quake_HideOnLoseFocus = 2,
};

enum BackgroundOp
{
	eUpLeft = 0,
	eStretch = 1,
	eTile = 2,
	eUpRight = 3,
	eDownLeft = 4,
	eDownRight = 5,
	eFit = 6, // Stretch aspect ratio (Fit)
	eFill = 7, // Stretch aspect ratio (Fill)
	eCenter = 8,
	//
	eOpLast = eCenter,
};

enum StartupType
{
	start_Command = 0,
	start_File = 1, // @cmd task file
	start_Task = 2, // {named task}
	start_Auto = 3, // <auto saved task> (*StartupTask)
};

enum CEStatusFlags
{
	csf_VertDelim           = 0x00000001,
	csf_HorzDelim           = 0x00000002,
	csf_SystemColors        = 0x00000004,
	csf_NoVerticalPad       = 0x00000008,
};

enum CECopyMode
{
	cm_CopySel = 0, // Copy current selection (old bCopyAll==false)
	cm_CopyAll = 1, // Copy full buffer (old bCopyAll==true)
	cm_CopyVis = 2, // Copy visible screen area
	cm_CopyInt = 3, // Copy current selection into internal CEStr
};

enum CTSEndOnTyping
{
	ceot_Off = 0,
	ceot_CopyReset = 1,
	ceot_Reset = 2,
};

enum CTSCopyFormat
{
	CTSFormatText = 0,
	CTSFormatHtmlData = 1,   // ready to paste formatted text in word processing
	CTSFormatHtmlText = 2,   // RAW HTML copied to clipboard
	CTSFormatANSI = 3,       // ANSI sequences
	CTSFormatDefault = 0xFF, // default arg for functions, use gpSet->isCTSHtmlFormat
};

enum CEPasteMode
{
	pm_Standard  = 0, // Paste with possible "Return" keypresses
	pm_FirstLine = 1, // Paste only first line from the clipboard
	pm_OneLine   = 2, // Paste all lines from the clipboard, but delimit them with SPACES (cmd-line safe!)
};

// New columns may be inserted anywhere, all settings has certain names
enum CEStatusItems
{
	csi_Info = 0,

	csi_ActiveVCon,

	csi_CapsLock,
	csi_NumLock,
	csi_ScrollLock,
	csi_ViewLock,
	csi_InputLocale,
	csi_KeyHooks,
	csi_TermModes,
	csi_RConModes,

	csi_WindowPos,
	csi_WindowSize,
	csi_WindowClient,
	csi_WindowWork,
	csi_WindowBack,
	csi_WindowDC,
	csi_WindowStyle,
	csi_WindowStyleEx,
	csi_HwndFore,
	csi_HwndFocus,
	csi_Zoom,
	csi_DPI,

	csi_ActiveBuffer,
	csi_ConsolePos,
	csi_ConsoleSize,
	csi_BufferSize,
	csi_CursorX,
	csi_CursorY,
	csi_CursorSize,
	csi_CursorInfo,
	csi_ConEmuPID,
	csi_ConEmuHWND,
	csi_ConEmuView,
	csi_Server,
	csi_ServerHWND,
	csi_Transparency,

	csi_NewVCon,
	csi_SyncInside,
	csi_ActiveProcess,
	csi_ConsoleTitle,

	csi_Time,

	csi_SizeGrip,

	//
	csi_Last
};


enum CESizeStyle
{
	ss_Standard = 0,
	ss_Pixels   = 1,
	ss_Percents = 2,
};

union CESize
{
	DWORD Raw;

	struct
	{
		int             Value: 24;
		CESizeStyle     Style: 8;

		mutable wchar_t TempSZ[12];
	};

	const wchar_t* AsString() const;

	bool IsValid(bool IsWidth) const;

	bool Set(bool IsWidth, CESizeStyle NewStyle, int NewValue);
	void SetFromRaw(bool IsWidth, DWORD aRaw);
	bool SetFromString(bool IsWidth, const wchar_t* sValue);
};
