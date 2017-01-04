
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

#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "Header.h"

#include "OptionsClass.h"
#include "SetDlgFonts.h"
#include "SetDlgLists.h"

#include "../common/WThreads.h"

#define DEBUGSTRFONT(s) //DEBUGSTR(s)

//const wchar_t CSetDlgFonts::TEST_FONT_WIDTH_STRING_EN[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
//const wchar_t CSetDlgFonts::TEST_FONT_WIDTH_STRING_RU[] = L"АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ";

HANDLE CSetDlgFonts::mh_EnumThread = NULL;
DWORD  CSetDlgFonts::mn_EnumThreadId = 0;
bool   CSetDlgFonts::mb_EnumThreadFinished = false;

int CSetDlgFonts::EnumFamCallBack(LPLOGFONT lplf, LPNEWTEXTMETRIC lpntm, DWORD FontType, LPVOID aFontCount)
{
	MCHKHEAP
	int far * aiFontCount = (int far *) aFontCount;

	if (!ghOpWnd)
		return FALSE;

	// Record the number of raster, TrueType, and vector
	// fonts in the font-count array.

	if (FontType & RASTER_FONTTYPE)
	{
		aiFontCount[0]++;
#ifdef _DEBUG
		OutputDebugString(L"Raster font: "); OutputDebugString(lplf->lfFaceName); OutputDebugString(L"\n");
#endif
	}
	else if (FontType & TRUETYPE_FONTTYPE)
	{
		aiFontCount[2]++;
	}
	else
	{
		aiFontCount[1]++;
	}

	DWORD bAlmostMonospace = CFontMgr::IsAlmostMonospace(lplf->lfFaceName, (LPTEXTMETRIC)lpntm /*lpntm->tmMaxCharWidth, lpntm->tmAveCharWidth, lpntm->tmHeight*/) ? 1 : 0;

	HWND hFontsPg = gpSetCls->GetPage(thi_Fonts);
	if (SendDlgItemMessage(hFontsPg, tFontFace, CB_FINDSTRINGEXACT, -1, (LPARAM) lplf->lfFaceName)==-1)
	{
		int nIdx;
		nIdx = SendDlgItemMessage(hFontsPg, tFontFace, CB_ADDSTRING, 0, (LPARAM) lplf->lfFaceName);
		SendDlgItemMessage(hFontsPg, tFontFace, CB_SETITEMDATA, nIdx, bAlmostMonospace);
		nIdx = SendDlgItemMessage(hFontsPg, tFontFace2, CB_ADDSTRING, 0, (LPARAM) lplf->lfFaceName);
		SendDlgItemMessage(hFontsPg, tFontFace2, CB_SETITEMDATA, nIdx, bAlmostMonospace);
	}

	MCHKHEAP
	return TRUE;
	//if (aiFontCount[0] || aiFontCount[1] || aiFontCount[2])
	//    return TRUE;
	//else
	//    return FALSE;
	UNREFERENCED_PARAMETER(lplf);
	UNREFERENCED_PARAMETER(lpntm);
}

int CSetDlgFonts::EnumFontCallBackEx(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, DWORD FontType, LPARAM lParam)
{
	UINT sz = 0;
	LONG nHeight = lpelfe->elfLogFont.lfHeight;

	if (nHeight < 8)
		return TRUE; // такие мелкие - не интересуют

	LONG nWidth  = lpelfe->elfLogFont.lfWidth;
	UINT nMaxCount = countof(CFontMgr::szRasterSizes);

	while(sz<nMaxCount && CFontMgr::szRasterSizes[sz].cy)
	{
		if (CFontMgr::szRasterSizes[sz].cx == nWidth && CFontMgr::szRasterSizes[sz].cy == nHeight)
			return TRUE; // Этот размер уже добавили

		sz++;
	}

	if (sz >= nMaxCount)
		return FALSE; // место кончилось

	CFontMgr::szRasterSizes[sz].cx = nWidth; CFontMgr::szRasterSizes[sz].cy = nHeight;
	return TRUE;
	UNREFERENCED_PARAMETER(lpelfe);
	UNREFERENCED_PARAMETER(lpntme);
	UNREFERENCED_PARAMETER(FontType);
	UNREFERENCED_PARAMETER(lParam);
}

DWORD CSetDlgFonts::EnumFontsThread(LPVOID apArg)
{
	HDC hdc = GetDC(NULL);
	int aFontCount[] = { 0, 0, 0 };
	wchar_t szName[MAX_PATH];
	// Сначала загрузить имена шрифтов, установленных в систему (или зарегистрированных нами)
	EnumFontFamilies(hdc, (LPCTSTR) NULL, (FONTENUMPROC) EnumFamCallBack, (LPARAM) aFontCount);
	// Теперь - загрузить размеры установленных терминальных шрифтов (aka Raster fonts)
	LOGFONT term = {0}; term.lfCharSet = OEM_CHARSET; wcscpy_c(term.lfFaceName, L"Terminal");
	//CFontMgr::szRasterSizes[0].cx = CFontMgr::szRasterSizes[0].cy = 0;
	memset(CFontMgr::szRasterSizes, 0, sizeof(CFontMgr::szRasterSizes));
	EnumFontFamiliesEx(hdc, &term, (FONTENUMPROCW) EnumFontCallBackEx, 0/*LPARAM*/, 0);
	UINT nMaxCount = countof(CFontMgr::szRasterSizes);

	for(UINT i = 0; i<(nMaxCount-1) && CFontMgr::szRasterSizes[i].cy; i++)
	{
		UINT k = i;

		for(UINT j = i+1; j<nMaxCount && CFontMgr::szRasterSizes[j].cy; j++)
		{
			if (CFontMgr::szRasterSizes[j].cy < CFontMgr::szRasterSizes[k].cy)
				k = j;
			else if (CFontMgr::szRasterSizes[j].cy == CFontMgr::szRasterSizes[k].cy
					&& CFontMgr::szRasterSizes[j].cx < CFontMgr::szRasterSizes[k].cx)
				k = j;
		}

		if (k != i)
		{
			SIZE sz = CFontMgr::szRasterSizes[k];
			CFontMgr::szRasterSizes[k] = CFontMgr::szRasterSizes[i];
			CFontMgr::szRasterSizes[i] = sz;
		}
	}

	DeleteDC(hdc);

	HWND hFontsPg = gpSetCls->GetPage(thi_Fonts);
	if (hFontsPg)
	{
		for (size_t sz=0; sz<countof(CFontMgr::szRasterSizes) && CFontMgr::szRasterSizes[sz].cy; sz++)
		{
			_wsprintf(szName, SKIPLEN(countof(szName)) L"[%s %ix%i]", CFontMgr::RASTER_FONTS_NAME, CFontMgr::szRasterSizes[sz].cx, CFontMgr::szRasterSizes[sz].cy);
			int nIdx = SendDlgItemMessage(hFontsPg, tFontFace, CB_INSERTSTRING, sz, (LPARAM)szName);
			SendDlgItemMessage(hFontsPg, tFontFace, CB_SETITEMDATA, nIdx, 1);
		}

		GetDlgItemText(hFontsPg, tFontFace, szName, countof(szName));
		CSetDlgLists::SelectString(hFontsPg, tFontFace, szName);
		GetDlgItemText(hFontsPg, tFontFace2, szName, countof(szName));
		CSetDlgLists::SelectString(hFontsPg, tFontFace2, szName);
	}

	SafeCloseHandle(mh_EnumThread);
	_ASSERTE(mh_EnumThread == NULL);

	mb_EnumThreadFinished = true;

	// Если шустрый юзер успел переключиться на вкладку "Views" до окончания
	// загрузки шрифтов - послать в диалог сообщение "Считать список из [thi_Fonts]"
	HWND hViewsPg = gpSetCls->GetPage(thi_Views);
	if (hViewsPg)
		PostMessage(hViewsPg, gpSetCls->mn_MsgLoadFontFromMain, 0, 0);
	HWND hTabsPg = gpSetCls->GetPage(thi_Tabs);
	if (hTabsPg)
		PostMessage(hTabsPg, gpSetCls->mn_MsgLoadFontFromMain, 0, 0);
	HWND hStatusPg = gpSetCls->GetPage(thi_Status);
	if (hStatusPg)
		PostMessage(hStatusPg, gpSetCls->mn_MsgLoadFontFromMain, 0, 0);

	return 0;
}

bool CSetDlgFonts::StartEnumFontsThread()
{
	_ASSERTE(mh_EnumThread == NULL);
	mb_EnumThreadFinished = false;
	mh_EnumThread = apiCreateThread(EnumFontsThread, NULL, &mn_EnumThreadId, "EnumFontsThread"); // хэндл закроет сама нить
	return (mh_EnumThread != NULL);
}

bool CSetDlgFonts::EnumFontsFinished()
{
	return mb_EnumThreadFinished;
}
