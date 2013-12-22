
/*
Copyright (c) 2009-2012 Maximus5
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


void ChangeScreenBufferSize(CONSOLE_SCREEN_BUFFER_INFO& sbi, SHORT VisibleX, SHORT VisibleY, SHORT BufferX, SHORT BufferY);
BOOL GetConWindowSize(const CONSOLE_SCREEN_BUFFER_INFO& sbi, int nCurWidth, int nCurHeight, DWORD nCurScroll, int* pnNewWidth, int* pnNewHeight, DWORD* pnScroll);
void SetConsoleFontSizeTo(HWND inConWnd, int inSizeY, int inSizeX, const wchar_t *asFontName, WORD anTextColors = 0, WORD anPopupColors = 0);
int EvaluateDefaultFontWidth(int inSizeY, const wchar_t *asFontName);
void EmergencyShow(HWND hConWnd);

BOOL apiGetConsoleScreenBufferInfoEx(HANDLE hConsoleOutput, MY_CONSOLE_SCREEN_BUFFER_INFOEX* lpConsoleScreenBufferInfoEx);
BOOL apiSetConsoleScreenBufferInfoEx(HANDLE hConsoleOutput, MY_CONSOLE_SCREEN_BUFFER_INFOEX* lpConsoleScreenBufferInfoEx);

BOOL apiGetConsoleFontSize(HANDLE hOutput, int &SizeY, int &SizeX, wchar_t (&rsFontName)[LF_FACESIZE]); //Vista+ only!
BOOL apiSetConsoleFontSize(HANDLE hOutput, int inSizeY, int inSizeX, const wchar_t *asFontName); //Vista+ only!
BOOL apiFixFontSizeForBufferSize(HANDLE hOutput, COORD dwSize);
