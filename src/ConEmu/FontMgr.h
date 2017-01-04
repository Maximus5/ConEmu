
/*
Copyright (c) 2016-2017 Maximus5
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

#include "../common/MArray.h"
#include "../common/MFileMapping.h"
#include <commctrl.h>

#include "CustomFonts.h"
#include "DpiAware.h"
#include "FontInfo.h"
#include "FontPtr.h"
#include "HotkeyList.h"
#include "Options.h"
//#include "SetDlgButtons.h"
//#include "SetDlgColors.h"
//#include "SetDlgFonts.h"

class CFont;
class CFontMgr;

extern CFontMgr* gpFontMgr;

// When font size is used as character size (negative LF.lfHeight)
// we need to evaluate real font size... Only for vector fonts!
struct FontHeightInfo
{
	// In
	wchar_t lfFaceName[LF_FACESIZE];
	int     lfHeight;
	UINT    lfCharSet;
	// Out
	int     CellHeight;
};

// Temporarily registered fonts
struct RegFont
{
	BOOL    bDefault;             // Этот шрифт пользователь указал через /fontfile
	CustomFontFamily* pCustom;    // Для шрифтов, рисованных нами
	wchar_t szFontFile[MAX_PATH]; // полный путь
	wchar_t szFontName[32];       // Font Family
	BOOL    bUnicode;             // Юникодный?
	BOOL    bHasBorders;          // Имеет ли данный шрифт символы рамок
	BOOL    bAlreadyInSystem;     // Шрифт с таким именем уже был зарегистрирован в системе
};

class CFontMgr
{
public:
	CFontMgr();
	~CFontMgr();

private:
	friend class CSetDlgButtons;
	friend class CSetDlgColors;
	friend class CSetDlgFonts;
	friend class CSettings;

private:
	MArray<CFont*> m_FontPtrs;
protected:
	WARNING("Refactoring is required. m_FontPtrs must be filled from here, and CFont must not be created outside of CFontMgr");
	friend class CFont;
	void FontPtrRegister(CFont* p);
	void FontPtrUnregister(CFont* p);

public:
	static const int FontDefWidthMin; // 0
	static const int FontDefWidthMax; // 99
	static const int FontZoom100; // 10000

	static const wchar_t RASTER_FONTS_NAME[]; // L"Raster Fonts";
	static SIZE szRasterSizes[100]; // {{0,0}}; // {{16,8},{6,9},{8,9},{5,12},{7,12},{8,12},{16,12},{12,16},{10,18}};

public:
	WARNING("Make these protected?");
	CFontPtr  m_Font[MAX_FONT_STYLES];
	CFontPtr  m_Font2;

	CEStr   ms_FontError;

private:
	friend class CSetPgInfo;
	friend class CSetPgFonts;
	LONG mn_FontZoomValue; // 100% == 10000 (FontZoom100)
	CLogFont LogFont;
	LONG mn_AutoFontWidth, mn_AutoFontHeight; // размеры шрифтов, которые были запрошены при авторесайзе шрифта
	LONG mn_FontWidth, mn_FontHeight; // реальные размеры шрифтов

	BOOL mb_Name1Ok, mb_Name2Ok;

	// When font size is used as character size (negative LF.lfHeight)
	// we need to evaluate real font size... Only for vector fonts!
	MArray<FontHeightInfo> m_FontHeights;

	// Temporarily registered fonts
	MArray<RegFont> m_RegFonts;
	BOOL mb_StopRegisterFonts;

public:
	bool    AutoRecreateFont(int nFontW, int nFontH);
	BYTE    BorderFontCharSet();
	LPCWSTR BorderFontFaceName();
	LONG    BorderFontHeight();
	LONG    BorderFontWidth();
	bool    Create(CLogFont inFont, CFontPtr& rpFont, CustomFontFamily** ppCustom = NULL);
	bool    CreateOtherFont(const wchar_t* asFontName, CFontPtr& rpFont);
	LONG    EvalFontHeight(LPCWSTR lfFaceName, LONG lfHeight, BYTE nFontCharSet);
	void    EvalLogfontSizes(LOGFONT& LF, LONG lfHeight, LONG lfWidth);
	BOOL    FontBold();
	LONG    FontCellWidth();
	BYTE    FontCharSet();
	BOOL    FontClearType();
	LPCWSTR FontFaceName();
	LONG    FontHeight();
	LONG    FontHeightHtml();
	BOOL    FontItalic();
	bool    FontMonospaced();
	BYTE    FontQuality();
	LONG    FontWidth();
	BOOL    GetFontNameFromFile(LPCTSTR lpszFilePath, wchar_t (&rsFontName)[LF_FACESIZE], wchar_t (&rsFullFontName)[LF_FACESIZE]);
	BOOL    GetFontNameFromFile_BDF(LPCTSTR lpszFilePath, wchar_t (&rsFontName)[LF_FACESIZE], wchar_t (&rsFullFontName)[LF_FACESIZE]);
	BOOL    GetFontNameFromFile_OTF(LPCTSTR lpszFilePath, wchar_t (&rsFontName)[LF_FACESIZE], wchar_t (&rsFullFontName)[LF_FACESIZE]);
	BOOL    GetFontNameFromFile_TTF(LPCTSTR lpszFilePath, wchar_t (&rsFontName)[LF_FACESIZE], wchar_t (&rsFullFontName)[LF_FACESIZE]);
	void    GetMainLogFont(LOGFONT& lf);
	LONG    GetZoom(bool bRaw = false); // в процентах (false) или mn_FontZoomValue (true)
	void    InitFont(LPCWSTR asFontName=NULL, int anFontHeight=-1, int anQuality=-1);
	void    MacroFontSetName(LPCWSTR pszFontName, WORD anHeight /*= 0*/, WORD anWidth /*= 0*/);
	bool    MacroFontSetSize(int nRelative/*0/1/2/3*/, int nValue/*+-1,+-2,... | 100%*/);
	bool    QueryFont(CEFontStyles fontStyle, CVirtualConsole* pVCon, CFontPtr& rpFont);
	bool    RecreateFontByDpi(int dpiX, int dpiY, LPRECT prcSuggested);
	BOOL    RegisterFont(LPCWSTR asFontFile, BOOL abDefault);
	void    RegisterFonts();
	void    RegisterFontsDir(LPCWSTR asFromDir);
	void    UnregisterFonts();

public:
	static bool IsAlmostMonospace(LPCWSTR asFaceName, LPTEXTMETRIC lptm, LPOUTLINETEXTMETRIC lpotm = NULL);
	static LPOUTLINETEXTMETRIC LoadOutline(HDC hDC, HFONT hFont);
	static void DumpFontMetrics(LPCWSTR szType, CFontPtr& Font);

public:
	void    SettingsLoaded(SettingsLoadedFlags slfFlags);
	void    SettingsPreSave();

private:
	bool    CreateFontGroup(CLogFont inFont);
	LONG    EvalCellWidth();
	bool    FindCustomFont(LPCWSTR lfFaceName, int iSize, BOOL bBold, BOOL bItalic, BOOL bUnderline, CustomFontFamily** ppCustom, CustomFont** ppFont);
	bool    MacroFontSetSizeInt(LOGFONT& LF, int nRelative/*0/1/2/3*/, int nValue/*+-1,+-2,... | 100%*/);
	void    RecreateAlternativeFont();
	void    RecreateFont(bool abReset, bool abRecreateControls = false);
	void    ResetFontWidth();
	void    SaveFontSizes(bool bAuto, bool bSendChanges);
};
