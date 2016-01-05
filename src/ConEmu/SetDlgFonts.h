
/*
Copyright (c) 2014-2015 Maximus5
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

class CSetDlgFonts
{
protected:
	static const wchar_t RASTER_FONTS_NAME[]; // L"Raster Fonts";
	static const wchar_t szRasterAutoError[]; // L"Font auto size is not allowed for a fixed raster font size. Select 'Terminal' instead of '[Raster Fonts ...]'";
	static SIZE szRasterSizes[100]; // {{0,0}}; // {{16,8},{6,9},{8,9},{5,12},{7,12},{8,12},{16,12},{12,16},{10,18}};

	static const int FontDefWidthMin; // 0
	static const int FontDefWidthMax; // 99

	static const int FontZoom100; // 10000

	static const wchar_t TEST_FONT_WIDTH_STRING_EN[]; // L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	static const wchar_t TEST_FONT_WIDTH_STRING_RU[]; // L"АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"

	static int CALLBACK EnumFamCallBack(LPLOGFONT lplf, LPNEWTEXTMETRIC lpntm, DWORD FontType, LPVOID aFontCount);
	static int CALLBACK EnumFontCallBackEx(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, DWORD FontType, LPARAM lParam);

	static bool StartEnumFontsThread();
	static bool EnumFontsFinished();

	static bool IsAlmostMonospace(LPCWSTR asFaceName, LPTEXTMETRIC lptm, LPOUTLINETEXTMETRIC lpotm = NULL);
	static LPOUTLINETEXTMETRIC LoadOutline(HDC hDC, HFONT hFont);
	static void DumpFontMetrics(LPCWSTR szType, HDC hDC, HFONT hFont, LPOUTLINETEXTMETRIC lpOutl = NULL);

private:
	static HANDLE mh_EnumThread;
	static DWORD  mn_EnumThreadId;
	static bool   mb_EnumThreadFinished;
	static DWORD CALLBACK EnumFontsThread(LPVOID apArg);
};
