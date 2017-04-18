
/*
Copyright (c) 2009-2017 Maximus5
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

#ifndef COMMON_LVB_LEADING_BYTE
#define COMMON_LVB_LEADING_BYTE    0x0100 // Leading Byte of DBCS
#define COMMON_LVB_TRAILING_BYTE   0x0200 // Trailing Byte of DBCS
#endif

#ifndef CONSOLE_FULLSCREEN_HARDWARE
#define CONSOLE_FULLSCREEN_HARDWARE 2   // console owns the hardware
#endif

struct MY_CONSOLE_SCREEN_BUFFER_INFOEX
{
	ULONG      cbSize;
	COORD      dwSize;
	COORD      dwCursorPosition;
	WORD       wAttributes;
	SMALL_RECT srWindow;
	COORD      dwMaximumWindowSize;
	WORD       wPopupAttributes;
	BOOL       bFullscreenSupported;
	COLORREF   ColorTable[16];
};

struct MY_CONSOLE_READCONSOLE_CONTROL
{
    ULONG nLength;
    ULONG nInitialChars;
    ULONG dwCtrlWakeupMask;
    ULONG dwControlKeyState;
};

struct MY_CONSOLE_FONT_INFOEX
{
    ULONG cbSize;
    DWORD nFont;
    COORD dwFontSize;
    UINT FontFamily;
    UINT FontWeight;
    WCHAR FaceName[LF_FACESIZE];
};

extern MY_CONSOLE_FONT_INFOEX g_LastSetConsoleFont;

#ifdef _DEBUG
extern bool g_IgnoreSetLargeFont;
#endif

void SetConsoleFontSizeTo(HWND inConWnd, int inSizeY, int inSizeX, const wchar_t *asFontName, WORD anTextColors = 0, WORD anPopupColors = 0);

BOOL apiGetConsoleScreenBufferInfoEx(HANDLE hConsoleOutput, MY_CONSOLE_SCREEN_BUFFER_INFOEX* lpConsoleScreenBufferInfoEx);
BOOL apiSetConsoleScreenBufferInfoEx(HANDLE hConsoleOutput, MY_CONSOLE_SCREEN_BUFFER_INFOEX* lpConsoleScreenBufferInfoEx);

BOOL apiGetConsoleFontSize(HANDLE hOutput, int &SizeY, int &SizeX, wchar_t (&rsFontName)[LF_FACESIZE]); //Vista+ only!
BOOL apiSetConsoleFontSize(HANDLE hOutput, int inSizeY, int inSizeX, const wchar_t *asFontName); //Vista+ only!
BOOL apiFixFontSizeForBufferSize(HANDLE hOutput, COORD dwSize, char* pszUtfLog = NULL, int cchLogMax = 0);
BOOL apiInitConsoleFontSize(HANDLE hOutput); //Used in RM_ALTERNATIVE

BOOL apiPauseConsoleOutput(HWND hConWnd, bool bPause);
BOOL apiGetConsoleSelectionInfo(PCONSOLE_SELECTION_INFO lpConsoleSelectionInfo);

COORD MyGetLargestConsoleWindowSize(HANDLE hConsoleOutput);

class MSetConTextAttr
{
protected:
	HANDLE mh_ConOut;
	CONSOLE_SCREEN_BUFFER_INFO m_csbi;
public:
	MSetConTextAttr(HANDLE hConOut, WORD wAttributes);
	~MSetConTextAttr();
};
