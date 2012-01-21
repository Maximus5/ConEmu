
//     bHideCaptionSettings    "Choose frame width, appearance and disappearance delays"


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


// cbFARuseASCIIsort, cbFixAltOnAltTab

#define SHOWDEBUGSTR

#include "Header.h"
#include <commctrl.h>
#include <shlobj.h>
//#include "../common/ConEmuCheck.h"
#include "Options.h"
#include "ConEmu.h"
#include "ConEmuChild.h"
#include "VirtualConsole.h"
#include "RealConsole.h"
#include "TabBar.h"
#include "Background.h"
#include "TrayIcon.h"
#include "LoadImg.h"
#include "version.h"

//#define CONEMU_ROOT_KEY L"Software\\ConEmu"

#define DEBUGSTRFONT(s) DEBUGSTR(s)

#define COUNTER_REFRESH 5000
#define BACKGROUND_FILE_POLL 5000

#define RASTER_FONTS_NAME L"Raster Fonts"
SIZE szRasterSizes[100] = {{0,0}}; // {{16,8},{6,9},{8,9},{5,12},{7,12},{8,12},{16,12},{12,16},{10,18}};
const wchar_t szRasterAutoError[] = L"Font auto size is not allowed for a fixed raster font size. Select 'Terminal' instead of '[Raster Fonts ...]'";

#define TEST_FONT_WIDTH_STRING_EN L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define TEST_FONT_WIDTH_STRING_RU L"АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"

#define FAILED_FONT_TIMERID 101
#define FAILED_FONT_TIMEOUT 3000
#define FAILED_CONFONT_TIMEOUT 30000

const WORD HostkeyCtrlIds[] = {cbHostWin, cbHostApps, cbHostLCtrl, cbHostRCtrl, cbHostLAlt, cbHostRAlt, cbHostLShift, cbHostRShift};
//int upToFontHeight=0;
HWND ghOpWnd=NULL;

#ifdef _DEBUG
#define HEAPVAL //HeapValidate(GetProcessHeap(), 0, NULL);
#else
#define HEAPVAL
#endif

//#define DEFAULT_FADE_LOW 0
//#define DEFAULT_FADE_HIGH 0xD0

struct CONEMUDEFCOLORS
{
	const wchar_t* pszTitle;
	DWORD dwDefColors[0x10];
};

const CONEMUDEFCOLORS DefColors[] =
{
	{
		L"Default color scheme (Windows standard)", {
			0x00000000, 0x00800000, 0x00008000, 0x00808000, 0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0,
			0x00808080, 0x00ff0000, 0x0000ff00, 0x00ffff00, 0x000000ff, 0x00ff00ff, 0x0000ffff, 0x00ffffff
		}
	},
	{
		L"Gamma 1 (for use with dark monitors)", {
			0x00000000, 0x00960000, 0x0000aa00, 0x00aaaa00, 0x000000aa, 0x00800080, 0x0000aaaa, 0x00c0c0c0,
			0x00808080, 0x00ff0000, 0x0000ff00, 0x00ffff00, 0x000000ff, 0x00ff00ff, 0x0000ffff, 0x00ffffff
		}
	},
	{
		L"Murena scheme", {
			0x00000000, 0x00644100, 0x00008000, 0x00808000, 0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0,
			0x00808080, 0x00ff0000, 0x0076c587, 0x00ffff00, 0x00004bff, 0x00d78ce6, 0x0000ffff, 0x00ffffff
		}
	},
	{
		L"tc-maxx", {
			0x00000000, RGB(11,27,59), RGB(0,128,0), RGB(0,90,135), RGB(106,7,28), RGB(128,0,128), RGB(128,128,0), RGB(40,150,177),
			RGB(128,128,128), RGB(0,0,255), RGB(0,255,0), RGB(0,215,243), RGB(190,7,23), RGB(255,0,255), RGB(255,255,0), RGB(255,255,255)
		}
	},

};
const DWORD *dwDefColors = DefColors->dwDefColors;
DWORD gdwLastColors[0x10] = {0};
BOOL  gbLastColorsOk = FALSE;

#define MAX_COLOR_EDT_ID c31



namespace SettingsNS
{
	const WCHAR* szKeys[] = {L"<None>", L"Left Ctrl", L"Right Ctrl", L"Left Alt", L"Right Alt", L"Left Shift", L"Right Shift"};
	const DWORD  nKeys[] =  {0,         VK_LCONTROL,  VK_RCONTROL,   VK_LMENU,    VK_RMENU,     VK_LSHIFT,     VK_RSHIFT};
	const WCHAR* szKeysAct[] = {L"<Always>", L"Left Ctrl", L"Right Ctrl", L"Left Alt", L"Right Alt", L"Left Shift", L"Right Shift"};
	const DWORD  nKeysAct[] =  {0,         VK_LCONTROL,  VK_RCONTROL,   VK_LMENU,    VK_RMENU,     VK_LSHIFT,     VK_RSHIFT};
	const WCHAR* szKeysHot[] = {L"", L"Esc", L"Delete", L"Tab", L"Enter", L"Space", L"Backspace"};
	const DWORD  nKeysHot[] =  {0, VK_ESCAPE, VK_DELETE, VK_TAB, VK_RETURN, VK_SPACE, VK_BACK};
	const DWORD  FSizes[] = {0, 8, 9, 10, 11, 12, 14, 16, 18, 19, 20, 24, 26, 28, 30, 32, 34, 36, 40, 46, 50, 52, 72};
	const DWORD  FSizesSmall[] = {5, 6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 19, 20, 24, 26, 28, 30, 32};
	const WCHAR* szClipAct[] = {L"<None>", L"Copy", L"Paste", L"Auto"};
	const DWORD  nClipAct[] =  {0,         1,       2,        3};
	const WCHAR* szColorIdx[] = {L" 0", L" 1", L" 2", L" 3", L" 4", L" 5", L" 6", L" 7", L" 8", L" 9", L"10", L"11", L"12", L"13", L"14", L"15", L"None"};
	const DWORD  nColorIdx[] = {    0,     1,     2,     3,     4,     5,     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,    16};
	const WCHAR* szColorIdxSh[] = {L"# 0", L"# 1", L"# 2", L"# 3", L"# 4", L"# 5", L"# 6", L"# 7", L"# 8", L"# 9", L"#10", L"#11", L"#12", L"#13", L"#14", L"#15"};
	const DWORD  nColorIdxSh[] =  {    0,      1,      2,      3,      4,      5,      6,      7,      8,      9,     10,     11,     12,     13,     14,     15};
	const WCHAR* szColorIdxTh[] = {L"# 0", L"# 1", L"# 2", L"# 3", L"# 4", L"# 5", L"# 6", L"# 7", L"# 8", L"# 9", L"#10", L"#11", L"#12", L"#13", L"#14", L"#15", L"Auto"};
	const DWORD  nColorIdxTh[] =  {    0,      1,      2,      3,      4,      5,      6,      7,      8,      9,     10,     11,     12,     13,     14,     15,    16};
	const WCHAR* szThumbMaxZoom[] = {L"100%",L"200%",L"300%",L"400%",L"500%",L"600%"};
	const DWORD  nThumbMaxZoom[] = {100,200,300,400,500,600};
	const DWORD  nCharSets[] = {0, 178, 186, 136, 1, 238, 134, 161, 177, 129, 130, 77, 255, 204, 128, 2, 222, 162, 163};
	const WCHAR* szCharSets[] = {L"ANSI", L"Arabic", L"Baltic", L"Chinese Big 5", L"Default", L"East Europe",
		L"GB 2312", L"Greek", L"Hebrew", L"Hangul", L"Johab", L"Mac", L"OEM", L"Russian", L"Shiftjis",
		L"Symbol", L"Thai", L"Turkish", L"Vietnamese"
	};
};

#define FillListBox(hDlg,nDlgID,Items,Values,Value) \
	_ASSERTE(countof(Items) == countof(Values)); \
	FillListBoxItems(GetDlgItem(hDlg,nDlgID), countof(Items), Items, Values, Value)
#define FillListBoxInt(hDlg,nDlgID,Values,Value) \
	FillListBoxItems(GetDlgItem(hDlg,nDlgID), countof(Values), NULL, Values, Value)
#define FillListBoxByte(hDlg,nDlgID,Items,Values,Value) \
	_ASSERTE(countof(Items) == countof(Values)); { \
		DWORD dwVal = Value; \
		FillListBoxItems(GetDlgItem(hDlg,nDlgID), countof(Items), Items, Values, dwVal); \
		Value = dwVal; }
#define FillListBoxCharSet(hDlg,nDlgID,Value) \
	{ \
		_ASSERTE(countof(SettingsNS::nCharSets) == countof(SettingsNS::szCharSets)); \
		u8 num = 4; /*индекс DEFAULT_CHARSET*/ \
		for (size_t i = 0; i < countof(SettingsNS::nCharSets); i++) \
		{ \
			SendDlgItemMessageW(hDlg, nDlgID, CB_ADDSTRING, 0, (LPARAM)SettingsNS::szCharSets[i]); \
			if (SettingsNS::nCharSets[i] == Value) num = i; \
		} \
		SendDlgItemMessage(hDlg, nDlgID, CB_SETCURSEL, num, 0); \
	}


#define GetListBox(hDlg,nDlgID,Items,Values,Value) \
	_ASSERTE(countof(Items) == countof(Values)); \
	GetListBoxItem(GetDlgItem(hDlg,nDlgID), countof(Items), Items, Values, Value)
#define GetListBoxByte(hDlg,nDlgID,Items,Values,Value) \
	_ASSERTE(countof(Items) == countof(Values)); { \
		DWORD dwVal = Value; \
		GetListBoxItem(GetDlgItem(hDlg,nDlgID), countof(Items), Items, Values, dwVal); \
		Value = dwVal; }

#define BST(v) (int)(v & 3) // BST_UNCHECKED/BST_CHECKED/BST_INDETERMINATE

//#define SetThumbColor(s,rgb,idx,us) { (s).RawColor = 0; (s).ColorRGB = rgb; (s).ColorIdx = idx; (s).UseIndex = us; }
//#define SetThumbSize(s,sz,x1,y1,x2,y2,ls,lp,fn,fs) { 
//		(s).nImgSize = sz; (s).nSpaceX1 = x1; (s).nSpaceY1 = y1; (s).nSpaceX2 = x2; (s).nSpaceY2 = y2; 
//		(s).nLabelSpacing = ls; (s).nLabelPadding = lp; wcscpy_c((s).sFontName,fn); (s).nFontHeight=fs; }
//#define ThumbLoadSet(s,n) { 
//		reg->Load(L"PanView." s L".ImgSize", n.nImgSize); 
//		reg->Load(L"PanView." s L".SpaceX1", n.nSpaceX1); 
//		reg->Load(L"PanView." s L".SpaceY1", n.nSpaceY1); 
//		reg->Load(L"PanView." s L".SpaceX2", n.nSpaceX2); 
//		reg->Load(L"PanView." s L".SpaceY2", n.nSpaceY2); 
//		reg->Load(L"PanView." s L".LabelSpacing", n.nLabelSpacing); 
//		reg->Load(L"PanView." s L".LabelPadding", n.nLabelPadding); 
//		reg->Load(L"PanView." s L".FontName", n.sFontName, countof(n.sFontName)); 
//		reg->Load(L"PanView." s L".FontHeight", n.nFontHeight); }
//#define ThumbSaveSet(s,n) { 
//		reg->Save(L"PanView." s L".ImgSize", n.nImgSize); 
//		reg->Save(L"PanView." s L".SpaceX1", n.nSpaceX1); 
//		reg->Save(L"PanView." s L".SpaceY1", n.nSpaceY1); 
//		reg->Save(L"PanView." s L".SpaceX2", n.nSpaceX2); 
//		reg->Save(L"PanView." s L".SpaceY2", n.nSpaceY2); 
//		reg->Save(L"PanView." s L".LabelSpacing", n.nLabelSpacing); 
//		reg->Save(L"PanView." s L".LabelPadding", n.nLabelPadding); 
//		reg->Save(L"PanView." s L".FontName", n.sFontName); 
//		reg->Save(L"PanView." s L".FontHeight", n.nFontHeight); }


CSettings::CSettings()
{
	gpSetCls = this; // сразу!
	
	gpSet = &m_Settings;

	isAdvLogging = 0;
	m_ActivityLoggingType = glt_None; mn_ActivityCmdStartTick = 0;
	bForceBufferHeight = false; nForceBufferHeight = 1000; /* устанавливается в true, из ком.строки /BufferHeight */
	AutoScroll = true;
	
	// Шрифты
	//memset(m_Fonts, 0, sizeof(m_Fonts));
	//TODO: OLD - на переделку
	memset(&LogFont, 0, sizeof(LogFont));
	memset(&LogFont2, 0, sizeof(LogFont2));
	//gpSet->isFontAutoSize = false;
	mn_AutoFontWidth = mn_AutoFontHeight = -1;
	isAllowDetach = 0;
	mb_ThemingEnabled = (gOSVer.dwMajorVersion >= 6 || (gOSVer.dwMajorVersion == 5 && gOSVer.dwMinorVersion >= 1));
	//isFullScreen = false;
	isMonospaceSelected = 0;
	
	// Некоторые вещи нужно делать вне InitSettings, т.к. она может быть
	// вызвана потом из интерфейса диалога настроек
	wcscpy_c(ConfigPath, CONEMU_ROOT_KEY L"\\.Vanilla");
	ConfigName[0] = 0;
	//Type[0] = 0;
	//gpSet->psCmd = NULL; gpSet->psCurCmd = NULL;
	wcscpy_c(szDefCmd, L"far");
	//gpSet->psCmdHistory = NULL; gpSet->nCmdHistorySize = 0;
	
	m_ThSetMap.InitName(CECONVIEWSETNAME, GetCurrentProcessId());
	if (!m_ThSetMap.Create())
	{
		MBoxA(m_ThSetMap.GetErrorText());
	}
	else
	{
		// Применить в Mapping (там заодно и палитра копируется)
		//m_ThSetMap.Set(&gpSet->ThSet);
		//!! Это нужно делать после создания основного шрифта
		//gpConEmu->OnPanelViewSettingsChanged(FALSE);
	}

	
	// Теперь установим умолчания настроек	
	gpSet->InitSettings();
	
	SingleInstanceArg = false;
	mb_StopRegisterFonts = FALSE;
	mb_IgnoreEditChanged = FALSE;
	mb_IgnoreTtfChange = TRUE;
	gpSet->mb_CharSetWasSet = FALSE;
	mb_TabHotKeyRegistered = FALSE;
	hMain = hExt = hFar = hTabs = hKeys = hColors = hViews = hInfo = hDebug = hUpdate = hSelection = NULL; hwndTip = NULL; hwndBalloon = NULL;
	hConFontDlg = NULL; hwndConFontBalloon = NULL; bShowConFontError = FALSE; sConFontError[0] = sDefaultConFontName[0] = 0; bConsoleFontChecked = FALSE;
	QueryPerformanceFrequency((LARGE_INTEGER *)&mn_Freq);
	memset(mn_Counter, 0, sizeof(*mn_Counter)*(tPerfInterval-gbPerformance));
	memset(mn_CounterMax, 0, sizeof(*mn_CounterMax)*(tPerfInterval-gbPerformance));
	memset(mn_FPS, 0, sizeof(mn_FPS)); mn_FPS_CUR_FRAME = 0;
	memset(mn_RFPS, 0, sizeof(mn_RFPS)); mn_RFPS_CUR_FRAME = 0;
	memset(mn_CounterTick, 0, sizeof(*mn_CounterTick)*(tPerfInterval-gbPerformance));
	//hBgBitmap = NULL; bgBmp = MakeCoord(0,0); hBgDc = NULL;
	isBackgroundImageValid = false;
	mb_NeedBgUpdate = FALSE; //mb_WasVConBgImage = FALSE;
	mb_BgLastFade = false;
	ftBgModified.dwHighDateTime = ftBgModified.dwLowDateTime = nBgModifiedTick = 0;
	mp_Bg = NULL; mp_BgImgData = NULL;
	ZeroStruct(mh_Font);
	mh_Font2 = NULL;
	ZeroStruct(m_tm);
	ZeroStruct(m_otm);
	ResetFontWidth();
	nAttachPID = 0; hAttachConWnd = NULL;
	memset(&ourSI, 0, sizeof(ourSI));
	ourSI.cb = sizeof(ourSI);
	szFontError[0] = 0;
	nConFontError = 0;
	memset(&tiBalloon, 0, sizeof(tiBalloon));
	//mn_FadeMul = gpSet->mn_FadeHigh - gpSet->mn_FadeLow;
	//gpSet->mn_LastFadeSrc = gpSet->mn_LastFadeDst = -1;

	try
	{
		GetStartupInfoW(&ourSI);
	}
	catch(...)
	{
		memset(&ourSI, 0, sizeof(ourSI));
	}

	mn_LastChangingFontCtrlId = 0;

	#if 0
	SetWindowThemeF = NULL;
	mh_Uxtheme = LoadLibrary(L"UxTheme.dll");

	if (mh_Uxtheme)
	{
		SetWindowThemeF = (SetWindowThemeT)GetProcAddress(mh_Uxtheme, "SetWindowTheme");
		EnableThemeDialogTextureF = (EnableThemeDialogTextureT)GetProcAddress(mh_Uxtheme, "EnableThemeDialogTexture");
		//if (SetWindowThemeF) { SetWindowThemeF(Progressbar1, L" ", L" "); }
	}
	#endif

	mn_MsgUpdateCounter = RegisterWindowMessage(L"ConEmuSettings::Counter");
	mn_MsgRecreateFont = RegisterWindowMessage(L"Settings::RecreateFont");
	mn_MsgLoadFontFromMain = RegisterWindowMessage(L"Settings::LoadFontNames");
	mn_ActivateTabMsg = RegisterWindowMessage(L"Settings::ActivateTab");
	mh_EnumThread = NULL;
	mh_CtlColorBrush = NULL;

	// Горячие клавиши
	InitVars_Hotkeys();

	// Вкладки-диалоги
	InitVars_Pages();
}

void CSettings::InitVars_Hotkeys()
{	
	// Горячие клавиши
	TODO("Дополнить системные комбинации");
	WARNING("У gpSet->nLDragKey,gpSet->nRDragKey был тип DWORD");
	ConEmuHotKeys HotKeys[] = {
		// User (Keys)
		{vkMinimizeRestore, 0, &gpSet->icMinimizeRestore},
		{vkMultiNew, 0, &gpSet->icMultiNew},
		{vkMultiNewShift, 0, &gpSet->icMultiNew},
		{vkMultiNext, 0, &gpSet->icMultiNext},
		{vkMultiNextShift, 0, &gpSet->icMultiNext},
		{vkMultiRecreate, 0, &gpSet->icMultiRecreate},
		{vkMultiBuffer, 0, &gpSet->icMultiBuffer},
		{vkMultiClose, 0, &gpSet->icMultiClose},
		{vkMultiCmd, 0, &gpSet->icMultiCmd},
		// User (Modifiers)
		{vkCTSVkBlock, 1, &gpSet->isCTSVkBlock},      // модификатор запуска выделения мышкой
		{vkCTSVkText, 1, &gpSet->isCTSVkText},       // модификатор запуска выделения мышкой
		{vkCTSVkAct, 1, &gpSet->isCTSVkAct},        // модификатор разрешения действий правой и средней кнопки мышки
		{vkFarGotoEditorVk, 1, &gpSet->isFarGotoEditorVk}, // модификатор для gpSet->isFarGotoEditor
		{vkLDragKey, 1, &gpSet->nLDragKey},         // модификатор драга левой кнопкой
		{vkRDragKey, 1, &gpSet->nRDragKey},         // модификатор драга правой кнопкой
		// System (predefined, fixed)
		{vkWinAltP, 0, NULL, 'P', MAKEMODIFIER2(VK_LWIN,VK_MENU)}, // Settings
		{vkWinAltSpace, 0, NULL, VK_SPACE, MAKEMODIFIER2(VK_LWIN,VK_MENU)}, // System menu
		{vkAltF9, 0, NULL, VK_F9, VK_MENU}, // System menu
		{vkCtrlWinAltSpace, 0, NULL, VK_SPACE, MAKEMODIFIER3(VK_CONTROL,VK_LWIN,VK_MENU)}, // Show real console
		{vkAltEnter, 0, NULL, VK_RETURN, VK_MENU}, // Full screen
		{vkCtrlWinEnter, 0, NULL, VK_RETURN, MAKEMODIFIER2(VK_LWIN,VK_CONTROL)},
		{vkAltSpace, 0, NULL, VK_SPACE, VK_MENU}, // System menu
		{vkCtrlUp, 0, NULL, VK_UP, VK_CONTROL}, // Buffer scroll
		{vkCtrlDown, 0, NULL, VK_DOWN, VK_CONTROL}, // Buffer scroll
		{vkCtrlPgUp, 0, NULL, VK_PRIOR, VK_CONTROL}, // Buffer scroll
		{vkCtrlPgDn, 0, NULL, VK_NEXT, VK_CONTROL}, // Buffer scroll
		{vkCtrlTab, 0, NULL, VK_TAB, VK_CONTROL}, // Tab switch
		{vkCtrlShiftTab, 0, NULL, VK_TAB, MAKEMODIFIER2(VK_CONTROL,VK_SHIFT)}, // Tab switch
		{vkCtrlTab_Left, 0, NULL, VK_LEFT, VK_CONTROL}, // Tab switch
		{vkCtrlTab_Up, 0, NULL, VK_UP, VK_CONTROL}, // Tab switch
		{vkCtrlTab_Right, 0, NULL, VK_RIGHT, VK_CONTROL}, // Tab switch
		{vkCtrlTab_Down, 0, NULL, VK_DOWN, VK_CONTROL}, // Tab switch
		{vkWinLeft, 0, NULL, VK_LEFT, (DWORD)-1}, // Decrease window width
		{vkWinRight, 0, NULL, VK_RIGHT, (DWORD)-1}, // Increase window width
		{vkWinUp, 0, NULL, VK_UP, (DWORD)-1}, // Decrease window height
		{vkWinDown, 0, NULL, VK_DOWN, (DWORD)-1}, // Increase window height
		// Console activate by number
		{vkConsole_1, 0, NULL, '1', (DWORD)-1},
		{vkConsole_2, 0, NULL, '2', (DWORD)-1},
		{vkConsole_3, 0, NULL, '3', (DWORD)-1},
		{vkConsole_4, 0, NULL, '4', (DWORD)-1},
		{vkConsole_5, 0, NULL, '5', (DWORD)-1},
		{vkConsole_6, 0, NULL, '6', (DWORD)-1},
		{vkConsole_7, 0, NULL, '7', (DWORD)-1},
		{vkConsole_8, 0, NULL, '8', (DWORD)-1},
		{vkConsole_9, 0, NULL, '9', (DWORD)-1},
		{vkConsole_10, 0, NULL, '0', (DWORD)-1},
		{vkConsole_11, 0, NULL, VK_F11, (DWORD)-1},
		{vkConsole_12, 0, NULL, VK_F12, (DWORD)-1},
		// End
		{},
	};
	m_HotKeys = (ConEmuHotKeys*)malloc(sizeof(HotKeys));
	memmove(m_HotKeys, HotKeys, sizeof(HotKeys));
	mp_ActiveHotKey = NULL;
}

void CSettings::InitVars_Pages()
{
	ConEmuSetupPages Pages[] = 
	{
		// При добавлении вкладки нужно добавить OnInitDialog_XXX в pageOpProc
		{IDD_SPG_MAIN,        L"Main",     &hMain/*,    OnInitDialog_Main*/},
		{IDD_SPG_FEATURE,     L"Features", &hExt/*,     OnInitDialog_Ext*/},
		{IDD_SPG_SELECTION,   L"Text selection", &hSelection /**/},
		{IDD_SPG_FEATURE_FAR, L"Far Manager", &hFar/*,     OnInitDialog_Ext*/},
		{IDD_SPG_KEYS,        L"Keys",     &hKeys/*,    OnInitDialog_Keys*/},
		{IDD_SPG_TABS,        L"Tabs",     &hTabs/*,    OnInitDialog_Tabs*/},
		{IDD_SPG_COLORS,      L"Colors",   &hColors/*,  OnInitDialog_Color*/},
		{IDD_SPG_VIEWS,       L"Views",    &hViews/*,   OnInitDialog_Views*/},
		{IDD_SPG_DEBUG,       L"Debug",    &hDebug/*,   OnInitDialog_Debug*/},
		{IDD_SPG_UPDATE,      L"Update",   &hUpdate/*,  OnInitDialog_Update*/},
		{IDD_SPG_INFO,        L"Info",     &hInfo/*,    OnInitDialog_Info*/},
		// End
		{},
	};
	WARNING("LogFont.lfFaceName && LogFont2.lfFaceName, LogFont.lfHeight - subject to change");
	ConEmuSetupItem Main_Items[] =
	{
		{tFontFace, sit_FontsAndRaster, LogFont.lfFaceName, countof(LogFont.lfFaceName)},
		{tFontFace, sit_Fonts, LogFont2.lfFaceName, countof(LogFont2.lfFaceName)},
		{tFontSizeY, sit_ULong, &LogFont.lfHeight, 0, sit_Byte, SettingsNS::FSizes+1, countof(SettingsNS::FSizes)-1},
		{tFontSizeX, sit_ULong, &gpSet->FontSizeX, 0, sit_Byte, SettingsNS::FSizes, countof(SettingsNS::FSizes)},
		{tFontSizeX2, sit_ULong, &gpSet->FontSizeX2, 0, sit_Byte, SettingsNS::FSizes, countof(SettingsNS::FSizes)},
		{tFontSizeX3, sit_ULong, &gpSet->FontSizeX3, 0, sit_Byte, SettingsNS::FSizes, countof(SettingsNS::FSizes)},
		{lbExtendFontBoldIdx, sit_Byte, &gpSet->nFontBoldColor, 0, sit_FixString, SettingsNS::szColorIdx, countof(SettingsNS::szColorIdx)},
		{lbExtendFontItalicIdx, sit_Byte, &gpSet->nFontItalicColor, 0, sit_FixString, SettingsNS::szColorIdx, countof(SettingsNS::szColorIdx)},
		{lbExtendFontNormalIdx, sit_Byte, &gpSet->nFontNormalColor, 0, sit_FixString, SettingsNS::szColorIdx, countof(SettingsNS::szColorIdx)},
		{cbExtendFonts, sit_Bool, &gpSet->isExtendFonts},
		// End
		{},
	};

	m_Pages = (ConEmuSetupPages*)malloc(sizeof(Pages));
	memmove(m_Pages, Pages, sizeof(Pages));
}

void CSettings::ReleaseHandles()
{
	//if (sTabCloseMacro) {free(sTabCloseMacro); sTabCloseMacro = NULL;}
	//if (sSafeFarCloseMacro) {free(sSafeFarCloseMacro); sSafeFarCloseMacro = NULL;}
	//if (sSaveAllMacro) {free(sSaveAllMacro); sSaveAllMacro = NULL;}
	//if (sRClickMacro) {free(sRClickMacro); sRClickMacro = NULL;}

	if (mp_Bg) { delete mp_Bg; mp_Bg = NULL; }

	SafeFree(mp_BgImgData);
}

CSettings::~CSettings()
{
	ReleaseHandles();

	for(int i=0; i<MAX_FONT_STYLES; i++)
	{
		if (mh_Font[i]) { DeleteObject(mh_Font[i]); mh_Font[i] = NULL; }

		if (m_otm[i]) {free(m_otm[i]); m_otm[i] = NULL;}
	}
	
	TODO("Очистить m_Fonts[Idx].hFonts");

	if (mh_Font2) { DeleteObject(mh_Font2); mh_Font2 = NULL; }
	
	if (gpSet->psCmd) {free(gpSet->psCmd); gpSet->psCmd = NULL;}

	if (gpSet->psCmdHistory) {free(gpSet->psCmdHistory); gpSet->psCmdHistory = NULL;}

	if (gpSet->psCurCmd) {free(gpSet->psCurCmd); gpSet->psCurCmd = NULL;}

#if 0
	if (mh_Uxtheme!=NULL) { FreeLibrary(mh_Uxtheme); mh_Uxtheme = NULL; }
#endif

	if (mh_CtlColorBrush) { DeleteObject(mh_CtlColorBrush); mh_CtlColorBrush = NULL; }

	//if (gpSet)
	//{
	//	delete gpSet;
	//	gpSet = NULL;
	//}
	if (m_HotKeys)
	{
		free(m_HotKeys);
		m_HotKeys = NULL;
		mp_ActiveHotKey = NULL;
	}
	if (m_Pages)
	{
		free(m_Pages);
		m_Pages = NULL;
	}
}

LPCWSTR CSettings::GetConfigPath()
{
	return ConfigPath;
}

LPCWSTR CSettings::GetConfigName()
{
	return ConfigName;
}

void CSettings::SetConfigName(LPCWSTR asConfigName)
{
	if (asConfigName && *asConfigName)
	{
		_wcscpyn_c(ConfigName, countof(ConfigName), asConfigName, countof(ConfigName));
		ConfigName[countof(ConfigName)-1] = 0; //checkpoint
		wcscpy_c(ConfigPath, CONEMU_ROOT_KEY L"\\");
		wcscat_c(ConfigPath, ConfigName);
	}
	else
	{
		wcscpy_c(ConfigPath, CONEMU_ROOT_KEY L"\\.Vanilla");
		ConfigName[0] = 0;
	}
}

void CSettings::SettingsLoaded()
{
	MCHKHEAP
	
	memmove(acrCustClr, gpSet->GetColors(FALSE), sizeof(COLORREF)*16);

	LogFont.lfHeight = mn_FontHeight = gpSet->FontSizeY;
	LogFont.lfWidth = mn_FontWidth = gpSet->FontSizeX;
	lstrcpyn(LogFont.lfFaceName, gpSet->inFont, countof(LogFont.lfFaceName));
	lstrcpyn(LogFont2.lfFaceName, gpSet->inFont2, countof(LogFont2.lfFaceName));
	LogFont.lfQuality = gpSet->mn_AntiAlias;
	LogFont.lfWeight = gpSet->isBold ? FW_BOLD : FW_NORMAL;
	LogFont.lfCharSet = (BYTE)gpSet->mn_LoadFontCharSet;
	LogFont.lfItalic = gpSet->isItalic;
	
	isMonospaceSelected = gpSet->isMonospace ? gpSet->isMonospace : 1; // запомнить, чтобы выбирать то что нужно при смене шрифта

	// Стили окна
	if (!gpSet->WindowMode)
	{
		// Иначе окно вообще не отображается
		_ASSERTE(gpSet->WindowMode!=0);
		gpSet->WindowMode = rNormal;
	}
	if (ghWnd == NULL)
	{
		gpConEmu->WindowMode = gpSet->WindowMode;
	}
	else
	{
		gpConEmu->SetWindowMode(gpSet->WindowMode);
	}

	MCHKHEAP
}

void CSettings::SettingsPreSave()
{
	lstrcpyn(gpSet->inFont, LogFont.lfFaceName, countof(gpSet->inFont));
	lstrcpyn(gpSet->inFont2, LogFont2.lfFaceName, countof(gpSet->inFont2));
	gpSet->FontSizeY = LogFont.lfHeight;
	gpSet->mn_LoadFontCharSet = LogFont.lfCharSet;
	gpSet->mn_AntiAlias = LogFont.lfQuality;
	gpSet->isBold = (LogFont.lfWeight >= FW_BOLD);
	gpSet->isItalic = (LogFont.lfItalic != 0);

	// Макросы, совпадающие с "умолчательными" - не пишем
	if (gpSet->sRClickMacro && lstrcmp(gpSet->sRClickMacro, gpSet->RClickMacroDefault()) == 0)
		SafeFree(gpSet->sRClickMacro);
	if (gpSet->sSafeFarCloseMacro && lstrcmp(gpSet->sSafeFarCloseMacro, gpSet->SafeFarCloseMacroDefault()) == 0)
		SafeFree(gpSet->sSafeFarCloseMacro);
	if (gpSet->sTabCloseMacro && lstrcmp(gpSet->sTabCloseMacro, gpSet->TabCloseMacroDefault()) == 0)
		SafeFree(gpSet->sTabCloseMacro);
	if (gpSet->sSaveAllMacro && lstrcmp(gpSet->sSaveAllMacro, gpSet->SaveAllMacroDefault()) == 0)
		SafeFree(gpSet->sSaveAllMacro);
	
	if (ghOpWnd && IsWindow(hMain))
	{
		GetString(hMain, tCmdLine, &gpSet->psCmd);
	}
}

/*
[HKEY_CURRENT_USER\Software\ConEmu\.Vanilla\Fonts\Font3]
"Title"="CJK"
"Used"=hex:01 ; Для быстрого включения/отключения группы
"CharRange"="2E80-2EFF;3000-303F;3200-4DB5;4E00-9FC3;F900-FAFF;FE30-FE4F;"
"FontFace"="Arial Unicode MS"
"Width"=dword:00000000
"Height"=dword:00000009
"Charset"=hex:00
"Bold"=hex:00
"Italic"=hex:00
"Anti-aliasing"=hex:04
*/
//BOOL CSettings::FontRangeLoad(SettingsBase* reg, int Idx)
//{
//	_ASSERTE(FALSE); // Не реализовано
//
//	BOOL lbOpened = FALSE, lbNeedCreateVanilla = FALSE;
//	wchar_t szFontKey[MAX_PATH+20];
//	_wsprintf(szFontKey, SKIPLEN(countof(szFontKey)) L"%s\\Font%i", ConfigPath, (Idx+1));
//	
//	
//	
//	lbOpened = reg->OpenKey(szFontKey, KEY_READ);
//	if (lbOpened)
//	{
//		TODO("Загрузить поля m_Fonts[Idx], очистив hFonts, не забыть про nLoadHeight/nLoadWidth");
//
//		reg->CloseKey();
//	}
//	
//	return lbOpened;
//}
//BOOL CSettings::FontRangeSave(SettingsBase* reg, int Idx)
//{
//	_ASSERTE(FALSE); // Не реализовано
//	return FALSE;
//}

void CSettings::InitFont(LPCWSTR asFontName/*=NULL*/, int anFontHeight/*=-1*/, int anQuality/*=-1*/)
{
	lstrcpyn(LogFont.lfFaceName, (asFontName && *asFontName) ? asFontName : (*gpSet->inFont) ? gpSet->inFont : L"Lucida Console", countof(LogFont.lfFaceName));
	if ((asFontName && *asFontName) || *gpSet->inFont)
		mb_Name1Ok = TRUE;
		
	if (anFontHeight!=-1)
	{
		LogFont.lfHeight = mn_FontHeight = anFontHeight;
		LogFont.lfWidth = mn_FontWidth = 0;
	}
	else
	{
		LogFont.lfHeight = mn_FontHeight = gpSet->FontSizeY;
		LogFont.lfWidth = mn_FontWidth = gpSet->FontSizeX;
	}

	LogFont.lfQuality = (anQuality!=-1) ? ANTIALIASED_QUALITY : gpSet->mn_AntiAlias;
	LogFont.lfWeight = gpSet->isBold ? FW_BOLD : FW_NORMAL;
	LogFont.lfCharSet = gpSet->mn_LoadFontCharSet;
	LogFont.lfItalic = gpSet->isItalic;
	
	lstrcpyn(LogFont2.lfFaceName, (*gpSet->inFont2) ? gpSet->inFont2 : L"Lucida Console", countof(LogFont2.lfFaceName));
	if (*gpSet->inFont2)
		mb_Name2Ok = TRUE;
	
	std::vector<RegFont>::iterator iter;

	if (!mb_Name1Ok)
	{
		for(int i = 0; !mb_Name1Ok && (i < 3); i++)
		{
			for (iter = m_RegFonts.begin(); iter != m_RegFonts.end(); ++iter)
			{
				switch(i)
				{
					case 0:

						if (!iter->bDefault || !iter->bUnicode) continue;

						break;
					case 1:

						if (!iter->bDefault) continue;

						break;
					case 2:

						if (!iter->bUnicode) continue;

						break;
					default:
						break;
				}

				lstrcpynW(LogFont.lfFaceName, iter->szFontName, countof(LogFont.lfFaceName));
				mb_Name1Ok = TRUE;
				break;
			}
		}
	}

	if (!mb_Name2Ok)
	{
		for (iter = m_RegFonts.begin(); iter != m_RegFonts.end(); ++iter)
		{
			if (iter->bHasBorders)
			{
				lstrcpynW(LogFont2.lfFaceName, iter->szFontName, countof(LogFont2.lfFaceName));
				mb_Name2Ok = TRUE;
				break;
			}
		}
	}

	mh_Font[0] = CreateFontIndirectMy(&LogFont);
	//2009-06-07 Реальный размер созданного шрифта мог измениться
	SaveFontSizes(&LogFont, (mn_AutoFontWidth == -1), false);
	// Перенесено в SaveFontSizes
	//// Применить в Mapping (там заодно и палитра копируется)
	//gpConEmu->OnPanelViewSettingsChanged(FALSE);
	MCHKHEAP
}

bool CSettings::ShowColorDialog(HWND HWndOwner, COLORREF *inColor)
{
	CHOOSECOLOR cc;                 // common dialog box structure
	// Вернул. IMHO - бред. Добавили Custom Color, а меняется ФОН окна!
	// Initialize CHOOSECOLOR
	ZeroMemory(&cc, sizeof(cc));
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = HWndOwner;
	cc.lpCustColors = (LPDWORD)acrCustClr;
	cc.rgbResult = *inColor;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;

	if (ChooseColor(&cc))
	{
		*inColor = cc.rgbResult;
		return true;
	}

	return false;
}

int CSettings::EnumFamCallBack(LPLOGFONT lplf, LPNEWTEXTMETRIC lpntm, DWORD FontType, LPVOID aFontCount)
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

	DWORD bAlmostMonospace = IsAlmostMonospace(lplf->lfFaceName, lpntm->tmMaxCharWidth, lpntm->tmAveCharWidth, lpntm->tmHeight) ? 1 : 0;

	if (SendDlgItemMessage(gpSetCls->hMain, tFontFace, CB_FINDSTRINGEXACT, -1, (LPARAM) lplf->lfFaceName)==-1)
	{
		int nIdx;
		nIdx = SendDlgItemMessage(gpSetCls->hMain, tFontFace, CB_ADDSTRING, 0, (LPARAM) lplf->lfFaceName);
		SendDlgItemMessage(gpSetCls->hMain, tFontFace, CB_SETITEMDATA, nIdx, bAlmostMonospace);
		nIdx = SendDlgItemMessage(gpSetCls->hMain, tFontFace2, CB_ADDSTRING, 0, (LPARAM) lplf->lfFaceName);
		SendDlgItemMessage(gpSetCls->hMain, tFontFace2, CB_SETITEMDATA, nIdx, bAlmostMonospace);
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

int CSettings::EnumFontCallBackEx(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, DWORD FontType, LPARAM lParam)
{
	UINT sz = 0;
	LONG nHeight = lpelfe->elfLogFont.lfHeight;

	if (nHeight < 8)
		return TRUE; // такие мелкие - не интересуют

	LONG nWidth  = lpelfe->elfLogFont.lfWidth;
	UINT nMaxCount = countof(szRasterSizes);

	while(sz<nMaxCount && szRasterSizes[sz].cy)
	{
		if (szRasterSizes[sz].cx == nWidth && szRasterSizes[sz].cy == nHeight)
			return TRUE; // Этот размер уже добавили

		sz++;
	}

	if (sz >= nMaxCount)
		return FALSE; // место кончилось

	szRasterSizes[sz].cx = nWidth; szRasterSizes[sz].cy = nHeight;
	return TRUE;
	UNREFERENCED_PARAMETER(lpelfe);
	UNREFERENCED_PARAMETER(lpntme);
	UNREFERENCED_PARAMETER(FontType);
	UNREFERENCED_PARAMETER(lParam);
}

DWORD CSettings::EnumFontsThread(LPVOID apArg)
{
	HDC hdc = GetDC(NULL);
	int aFontCount[] = { 0, 0, 0 };
	wchar_t szName[MAX_PATH];
	// Сначала загрузить имена шрифтов, установленных в систему (или зарегистрированных нами)
	EnumFontFamilies(hdc, (LPCTSTR) NULL, (FONTENUMPROC) EnumFamCallBack, (LPARAM) aFontCount);
	// Теперь - загрузить размеры установленных терминальных шрифтов (aka Raster fonts)
	LOGFONT term = {0}; term.lfCharSet = OEM_CHARSET; wcscpy_c(term.lfFaceName, L"Terminal");
	//szRasterSizes[0].cx = szRasterSizes[0].cy = 0;
	memset(szRasterSizes, 0, sizeof(szRasterSizes));
	EnumFontFamiliesEx(hdc, &term, (FONTENUMPROCW) EnumFontCallBackEx, 0/*LPARAM*/, 0);
	UINT nMaxCount = countof(szRasterSizes);

	for(UINT i = 0; i<(nMaxCount-1) && szRasterSizes[i].cy; i++)
	{
		UINT k = i;

		for(UINT j = i+1; j<nMaxCount && szRasterSizes[j].cy; j++)
		{
			if (szRasterSizes[j].cy < szRasterSizes[k].cy)
				k = j;
			else if (szRasterSizes[j].cy == szRasterSizes[k].cy
			        && szRasterSizes[j].cx < szRasterSizes[k].cx)
				k = j;
		}

		if (k != i)
		{
			SIZE sz = szRasterSizes[k];
			szRasterSizes[k] = szRasterSizes[i];
			szRasterSizes[i] = sz;
		}
	}

	DeleteDC(hdc);

	for(UINT sz=0; sz<countof(szRasterSizes) && szRasterSizes[sz].cy; sz++)
	{
		_wsprintf(szName, SKIPLEN(countof(szName)) L"[%s %ix%i]", RASTER_FONTS_NAME, szRasterSizes[sz].cx, szRasterSizes[sz].cy);
		int nIdx = SendDlgItemMessage(gpSetCls->hMain, tFontFace, CB_INSERTSTRING, sz, (LPARAM)szName);
		SendDlgItemMessage(gpSetCls->hMain, tFontFace, CB_SETITEMDATA, nIdx, 1);
	}

	GetDlgItemText(gpSetCls->hMain, tFontFace, szName, MAX_PATH);
	gpSetCls->SelectString(gpSetCls->hMain, tFontFace, szName);
	GetDlgItemText(gpSetCls->hMain, tFontFace2, szName, MAX_PATH);
	gpSetCls->SelectString(gpSetCls->hMain, tFontFace2, szName);
	SafeCloseHandle(gpSetCls->mh_EnumThread);
	_ASSERTE(gpSetCls->mh_EnumThread == NULL);

	// Если шустрый юзер успел переключиться на вкладку "Views" до оконачания
	// загрузки шрифтов - послать в диалог сообщение "Считать список из hMain"
	if (ghOpWnd)
	{
		if (gpSetCls->hViews)
			PostMessage(gpSetCls->hViews, gpSetCls->mn_MsgLoadFontFromMain, 0, 0);
		if (gpSetCls->hTabs)
			PostMessage(gpSetCls->hTabs, gpSetCls->mn_MsgLoadFontFromMain, 0, 0);
	}

	return 0;
}

LRESULT CSettings::OnInitDialog()
{
	_ASSERTE(!hMain && !hColors && !hViews && !hExt && !hFar && !hInfo && !hDebug && !hUpdate && !hSelection);
	hMain = hExt = hFar = hTabs = hKeys = hViews = hColors = hInfo = hDebug = hUpdate = hSelection = NULL;
	gbLastColorsOk = FALSE;

	RegisterTipsFor(ghOpWnd);

	HMENU hSysMenu = GetSystemMenu(ghOpWnd, FALSE);
	InsertMenu(hSysMenu, 0, MF_BYPOSITION, MF_SEPARATOR, 0);
	InsertMenu(hSysMenu, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED
	           | ((GetWindowLong(ghOpWnd,GWL_EXSTYLE)&WS_EX_TOPMOST) ? MF_CHECKED : 0),
	           ID_ALWAYSONTOP, _T("Al&ways on top..."));
	RegisterTabs();
	mn_LastChangingFontCtrlId = 0;
	mp_ActiveHotKey = NULL;
	wchar_t szTitle[MAX_PATH*2]; szTitle[0]=0;
	const wchar_t* pszType = L"[reg]";
	//int nConfLen = _tcslen(Config);
	//int nStdLen = strlen("Software\\ConEmu");
#ifndef __GNUC__
	HANDLE hFile = NULL;
	LPWSTR pszXmlFile = gpConEmu->ConEmuXml();

	if (pszXmlFile && *pszXmlFile)
	{
		hFile = CreateFile(pszXmlFile, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,
		                   NULL, OPEN_EXISTING, 0, 0);

		// XML-файл есть
		if (hFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hFile); hFile = NULL;
			pszType = L"[xml]";
			// Проверим, сможем ли мы в него записать
			hFile = CreateFile(pszXmlFile, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
			                   NULL, OPEN_EXISTING, 0, 0);

			if (hFile != INVALID_HANDLE_VALUE)
			{
				CloseHandle(hFile); hFile = NULL; // OK
			}
			else
			{
				EnableWindow(GetDlgItem(ghOpWnd, bSaveSettings), FALSE); // Сохранение запрещено
			}
		}
	}

#endif

	//if (nConfLen>(nStdLen+1))
	//	_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"%s Settings (%s) %s", gpConEmu->GetDefaultTitle(), (Config+nStdLen+1), pszType);
	if (ConfigName[0])
		_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"%s Settings (%s) %s", gpConEmu->GetDefaultTitle(), ConfigName, pszType);
	else
		_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"%s Settings %s", gpConEmu->GetDefaultTitle(), pszType);

	SetWindowText(ghOpWnd, szTitle);
	MCHKHEAP
	{
		#if 0
		TCITEM tie;
		wchar_t szTitle[32];
		HWND _hwndTab = GetDlgItem(ghOpWnd, tabMain);
		tie.mask = TCIF_TEXT;
		tie.iImage = -1;
		tie.pszText = szTitle;

		wcscpy_c(szTitle, L"Main");		TabCtrl_InsertItem(_hwndTab, 0, &tie);
		wcscpy_c(szTitle, L"Features");	TabCtrl_InsertItem(_hwndTab, 1, &tie);
		wcscpy_c(szTitle, L"Keys");		TabCtrl_InsertItem(_hwndTab, 2, &tie);
		wcscpy_c(szTitle, L"Tabs");		TabCtrl_InsertItem(_hwndTab, 3, &tie);
		wcscpy_c(szTitle, L"Colors");	TabCtrl_InsertItem(_hwndTab, 4, &tie); //-V112
		wcscpy_c(szTitle, L"Views");	TabCtrl_InsertItem(_hwndTab, 5, &tie);
		wcscpy_c(szTitle, L"Debug");	TabCtrl_InsertItem(_hwndTab, 6, &tie);
		wcscpy_c(szTitle, L"Info");		TabCtrl_InsertItem(_hwndTab, 7, &tie);

		HFONT hFont = CreateFont(gpSet->nTabFontHeight/*TAB_FONT_HEIGTH*/, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, gpSet->nTabFontCharSet /*ANSI_CHARSET*/, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, /* L"Tahoma" */ gpSet->sTabFontFace);
		SendMessage(_hwndTab, WM_SETFONT, WPARAM(hFont), TRUE);

		RECT rcClient; GetWindowRect(_hwndTab, &rcClient);
		MapWindowPoints(NULL, ghOpWnd, (LPPOINT)&rcClient, 2);
		TabCtrl_AdjustRect(_hwndTab, FALSE, &rcClient);

		#else
		mb_IgnoreSelPage = true;
		TVINSERTSTRUCT ti = {TVI_ROOT, TVI_LAST, {{TVIF_TEXT}}};
		HWND hTree = GetDlgItem(ghOpWnd, tvSetupCategories);
		for (size_t i = 0; m_Pages[i].PageID; i++)
		{
			ti.item.pszText = m_Pages[i].PageName;
			m_Pages[i].hTI = TreeView_InsertItem(hTree, &ti);
			*m_Pages[i].hPage = NULL;
		}

		HWND hPlace = GetDlgItem(ghOpWnd, tSetupPagePlace);
		RECT rcClient; GetWindowRect(hPlace, &rcClient);
		MapWindowPoints(NULL, ghOpWnd, (LPPOINT)&rcClient, 2);
		ShowWindow(hPlace, SW_HIDE);
		#endif
		mb_IgnoreSelPage = false;

		*m_Pages[0].hPage = CreateDialogParam((HINSTANCE)GetModuleHandle(NULL),
			MAKEINTRESOURCE(m_Pages[0].PageID), ghOpWnd, pageOpProc, (LPARAM)&(m_Pages[0]));
		MoveWindow(*m_Pages[0].hPage, rcClient.left, rcClient.top, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top, 0);


		apiShowWindow(*m_Pages[0].hPage, SW_SHOW);
	}
	MCHKHEAP
	{
		RECT rect;
		GetWindowRect(ghOpWnd, &rect);

		BOOL lbCentered = FALSE;
		HMONITOR hMon = MonitorFromWindow(ghOpWnd, MONITOR_DEFAULTTONEAREST);

		if (hMon)
		{
			MONITORINFO mi; mi.cbSize = sizeof(mi);

			if (GetMonitorInfo(hMon, &mi))
			{
				lbCentered = TRUE;
				MoveWindow(ghOpWnd,
				(mi.rcWork.left+mi.rcWork.right-rect.right+rect.left)/2,
				(mi.rcWork.top+mi.rcWork.bottom-rect.bottom+rect.top)/2,
				rect.right - rect.left, rect.bottom - rect.top, false);
			}
		}

		if (!lbCentered)
			MoveWindow(ghOpWnd, GetSystemMetrics(SM_CXSCREEN)/2 - (rect.right - rect.left)/2, GetSystemMetrics(SM_CYSCREEN)/2 - (rect.bottom - rect.top)/2, rect.right - rect.left, rect.bottom - rect.top, false);
	}
	return 0;
}

void CSettings::FillBgImageColors()
{
	TCHAR tmp[255];
	DWORD nTest = gpSet->nBgImageColors;
	wchar_t *pszTemp = tmp; tmp[0] = 0;

	if (gpSet->nBgImageColors == (DWORD)-1)
	{
		*(pszTemp++) = L'*';
	}
	else for (int idx = 0; nTest && idx < 16; idx++)
	{
		if (nTest & 1)
		{
			if (pszTemp != tmp)
			{
				*pszTemp++ = L' ';
				*pszTemp = 0;
			}

			_wsprintf(pszTemp, SKIPLEN(countof(tmp)-(pszTemp-tmp)) L"#%i", idx);
			pszTemp += _tcslen(pszTemp);
		}

		nTest = nTest >> 1;
	}

	*pszTemp = 0;
	SetDlgItemText(hMain, tBgImageColors, tmp);
}

LRESULT CSettings::OnInitDialog_Main(HWND hWnd2)
{
	#if 0
	if (gpSetCls->EnableThemeDialogTextureF)
		gpSetCls->EnableThemeDialogTextureF(hMain, 6/*ETDT_ENABLETAB*/);
	#endif

	SetDlgItemText(hMain, tFontFace, LogFont.lfFaceName);
	SetDlgItemText(hMain, tFontFace2, LogFont2.lfFaceName);
	DWORD dwThId;
	mh_EnumThread = CreateThread(0,0,EnumFontsThread,0,0,&dwThId); // хэндл закроет сама нить
	{
		wchar_t temp[MAX_PATH];

		for (uint i=0; i < countof(SettingsNS::FSizes); i++)
		{
			_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", SettingsNS::FSizes[i]);

			if (i > 0)
				SendDlgItemMessage(hMain, tFontSizeY, CB_ADDSTRING, 0, (LPARAM) temp);

			SendDlgItemMessage(hMain, tFontSizeX, CB_ADDSTRING, 0, (LPARAM) temp);
			SendDlgItemMessage(hMain, tFontSizeX2, CB_ADDSTRING, 0, (LPARAM) temp);
			SendDlgItemMessage(hMain, tFontSizeX3, CB_ADDSTRING, 0, (LPARAM) temp);
		}

		for (uint i=0; i < countof(SettingsNS::szColorIdx); i++)
		{
			//_wsprintf(temp, SKIPLEN(countof(temp))(i==16) ? L"None" : L"%2i", i);
			SendDlgItemMessage(hMain, lbExtendFontBoldIdx, CB_ADDSTRING, 0, (LPARAM) SettingsNS::szColorIdx[i]);
			SendDlgItemMessage(hMain, lbExtendFontItalicIdx, CB_ADDSTRING, 0, (LPARAM) SettingsNS::szColorIdx[i]);
			SendDlgItemMessage(hMain, lbExtendFontNormalIdx, CB_ADDSTRING, 0, (LPARAM) SettingsNS::szColorIdx[i]);
		}

		if (gpSet->isExtendFonts) CheckDlgButton(hMain, cbExtendFonts, BST_CHECKED);

		_wsprintf(temp, SKIPLEN(countof(temp))(gpSet->nFontBoldColor<16) ? L"%2i" : L"None", gpSet->nFontBoldColor);
		SelectStringExact(hMain, lbExtendFontBoldIdx, temp);
		_wsprintf(temp, SKIPLEN(countof(temp))(gpSet->nFontItalicColor<16) ? L"%2i" : L"None", gpSet->nFontItalicColor);
		SelectStringExact(hMain, lbExtendFontItalicIdx, temp);
		_wsprintf(temp, SKIPLEN(countof(temp))(gpSet->nFontNormalColor<16) ? L"%2i" : L"None", gpSet->nFontNormalColor);
		SelectStringExact(hMain, lbExtendFontNormalIdx, temp);

		if (gpSet->isFontAutoSize) CheckDlgButton(hMain, cbFontAuto, BST_CHECKED);

		_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", LogFont.lfHeight);
		//upToFontHeight = LogFont.lfHeight;
		SelectStringExact(hMain, tFontSizeY, temp);
		_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", gpSet->FontSizeX);
		SelectStringExact(hMain, tFontSizeX, temp);
		_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", gpSet->FontSizeX2);
		SelectStringExact(hMain, tFontSizeX2, temp);
		_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", gpSet->FontSizeX3);
		SelectStringExact(hMain, tFontSizeX3, temp);
	}

	FillListBoxCharSet(hMain, tFontCharset, LogFont.lfCharSet);

	//{
	//	_ASSERTE(countof(SettingsNS::nCharSets) == countof(SettingsNS::szCharSets));
	//	u8 num = 4; //-V112
	//	for (size_t i = 0; i < countof(SettingsNS::nCharSets); i++)
	//	{
	//		SendDlgItemMessageA(hMain, tFontCharset, CB_ADDSTRING, 0, (LPARAM) ChSets[i]);
	//		if (chSetsNums[i] == LogFont.lfCharSet) num = i;
	//	}
	//	SendDlgItemMessage(hMain, tFontCharset, CB_SETCURSEL, num, 0);
	//}
	
	MCHKHEAP
	SetDlgItemText(hMain, tCmdLine, gpSet->psCmd ? gpSet->psCmd : L"");
	SetDlgItemText(hMain, tBgImage, gpSet->sBgImage);
	//CheckDlgButton(hMain, rBgSimple, BST_CHECKED);

	FillBgImageColors();

	TCHAR tmp[255];
	//DWORD nTest = gpSet->nBgImageColors;
	//wchar_t *pszTemp = tmp; tmp[0] = 0;

	//for(int idx = 0; nTest && idx < 16; idx++)
	//{
	//	if (nTest & 1)
	//	{
	//		if (pszTemp != tmp)
	//		{
	//			*pszTemp++ = L' ';
	//			*pszTemp = 0;
	//		}

	//		_wsprintf(pszTemp, SKIPLEN(countof(tmp)-(pszTemp-tmp)) L"#%i", idx);
	//		pszTemp += _tcslen(pszTemp);
	//	}

	//	nTest = nTest >> 1;
	//}

	//*pszTemp = 0;
	//SetDlgItemText(hMain, tBgImageColors, tmp);

	_wsprintf(tmp, SKIPLEN(countof(tmp)) L"%i", gpSet->bgImageDarker);
	SendDlgItemMessage(hMain, tDarker, EM_SETLIMITTEXT, 3, 0);
	SetDlgItemText(hMain, tDarker, tmp);
	SendDlgItemMessage(hMain, slDarker, TBM_SETRANGE, (WPARAM) true, (LPARAM) MAKELONG(0, 255));
	SendDlgItemMessage(hMain, slDarker, TBM_SETPOS  , (WPARAM) true, (LPARAM) gpSet->bgImageDarker);
	CheckDlgButton(hMain, rBgUpLeft+(UINT)gpSet->bgOperation, BST_CHECKED);

	CheckDlgButton(hMain, cbBgAllowPlugin, BST(gpSet->isBgPluginAllowed));

	CheckDlgButton(hMain, cbBgImage, BST(gpSet->isShowBgImage));
	EnableWindow(GetDlgItem(hMain, tBgImage), gpSet->isShowBgImage);
	EnableWindow(GetDlgItem(hMain, bBgImage), gpSet->isShowBgImage);

	CheckRadioButton(hWnd2, rNoneAA, rCTAA,
		(LogFont.lfQuality == CLEARTYPE_NATURAL_QUALITY) ? rCTAA :
		(LogFont.lfQuality == ANTIALIASED_QUALITY) ? rStandardAA : rNoneAA);

	CheckDlgButton(hMain, cbCursorColor, gpSet->isCursorColor);

	CheckDlgButton(hMain, cbCursorBlink, gpSet->isCursorBlink);

	CheckDlgButton(hMain, cbBlockInactiveCursor, gpSet->isCursorBlockInactive);

	CheckRadioButton(hWnd2, rCursorV, rCursorH, gpSet->isCursorV ? rCursorV : rCursorH);

	// 3d state - force center symbols in cells
	CheckDlgButton(hMain, cbMonospace, BST(gpSet->isMonospace));

	CheckDlgButton(hMain, cbBold, (LogFont.lfWeight == FW_BOLD) ? BST_CHECKED : BST_UNCHECKED);

	CheckDlgButton(hMain, cbItalic, LogFont.lfItalic ? BST_CHECKED : BST_UNCHECKED);

	CheckDlgButton(hMain, cbFixFarBorders, BST(gpSet->isFixFarBorders));

	if (gpConEmu->isFullScreen())
		CheckRadioButton(hMain, rNormal, rFullScreen, rFullScreen);
	else if (gpConEmu->isZoomed())
		CheckRadioButton(hMain, rNormal, rFullScreen, rMaximized);
	else
		CheckRadioButton(hMain, rNormal, rFullScreen, rNormal);

	//swprintf_c(temp, L"%i", wndWidth);   SetDlgItemText(hMain, tWndWidth, temp);
	SendDlgItemMessage(hMain, tWndWidth, EM_SETLIMITTEXT, 3, 0);
	//swprintf_c(temp, L"%i", wndHeight);  SetDlgItemText(hMain, tWndHeight, temp);
	SendDlgItemMessage(hMain, tWndHeight, EM_SETLIMITTEXT, 3, 0);
	UpdateSize(gpSet->wndWidth, gpSet->wndHeight);
	EnableWindow(GetDlgItem(hMain, cbApplyPos), FALSE);
	SendDlgItemMessage(hMain, tWndX, EM_SETLIMITTEXT, 6, 0);
	SendDlgItemMessage(hMain, tWndY, EM_SETLIMITTEXT, 6, 0);
	MCHKHEAP

	if (!gpConEmu->isFullScreen() && !gpConEmu->isZoomed())
	{
		//EnableWindow(GetDlgItem(hMain, tWndWidth), true);
		//EnableWindow(GetDlgItem(hMain, tWndHeight), true);
		//EnableWindow(GetDlgItem(hMain, tWndX), true);
		//EnableWindow(GetDlgItem(hMain, tWndY), true);
		//EnableWindow(GetDlgItem(hMain, rFixed), true);
		//EnableWindow(GetDlgItem(hMain, rCascade), true);

		if (!gpConEmu->isIconic())
		{
			RECT rc; GetWindowRect(ghWnd, &rc);
			gpSet->wndX = rc.left; gpSet->wndY = rc.top;
		}
	}
	//else
	//{
	//	EnableWindow(GetDlgItem(hMain, tWndWidth), false);
	//	EnableWindow(GetDlgItem(hMain, tWndHeight), false);
	//	EnableWindow(GetDlgItem(hMain, tWndX), false);
	//	EnableWindow(GetDlgItem(hMain, tWndY), false);
	//	EnableWindow(GetDlgItem(hMain, rFixed), false);
	//	EnableWindow(GetDlgItem(hMain, rCascade), false);
	//}

	UpdatePos(gpSet->wndX, gpSet->wndY);
	CheckRadioButton(hMain, rCascade, rFixed, gpSet->wndCascade ? rCascade : rFixed);

	CheckDlgButton(hMain, cbAutoSaveSizePos, gpSet->isAutoSaveSizePos);

	mn_LastChangingFontCtrlId = 0;
	RegisterTipsFor(hMain);
	return 0;
}

LRESULT CSettings::OnInitDialog_Ext(HWND hWnd2)
{
	#if 0
	if (gpSetCls->EnableThemeDialogTextureF)
		gpSetCls->EnableThemeDialogTextureF(hWnd2, 6/*ETDT_ENABLETAB*/);
	#endif

	CheckDlgButton(hWnd2, cbMinToTray, gpSet->isMinToTray);

	CheckDlgButton(hWnd2, cbAlwaysShowTrayIcon, gpSet->isAlwaysShowTrayIcon);

	CheckDlgButton(hWnd2, cbAutoRegFonts, gpSet->isAutoRegisterFonts);

	CheckDlgButton(hWnd2, cbDebugSteps, gpSet->isDebugSteps);

	CheckDlgButton(hWnd2, cbEnhanceGraphics, gpSet->isEnhanceGraphics);
	
	CheckDlgButton(hWnd2, cbEnhanceButtons, gpSet->isEnhanceButtons);

	CheckDlgButton(hWnd2, cbFixAltOnAltTab, gpSet->isFixAltOnAltTab);

	CheckDlgButton(hWnd2, cbSkipActivation, gpSet->isMouseSkipActivation);

	CheckDlgButton(hWnd2, cbSkipMove, gpSet->isMouseSkipMoving);

	CheckDlgButton(hWnd2, cbMonitorConsoleLang, gpSet->isMonitorConsoleLang);

	CheckDlgButton(hWnd2, cbSkipFocusEvents, gpSet->isSkipFocusEvents);

	//CheckDlgButton(hWnd2, cbConsoleTextSelection, gpSet->isConsoleTextSelection);

	CheckDlgButton(hWnd2, cbTryToCenter, gpSet->isTryToCenter);

	CheckDlgButton(hWnd2, cbAlwaysShowScrollbar, gpSet->isAlwaysShowScrollbar);

	CheckDlgButton(hWnd2, cbDesktopMode, gpSet->isDesktopMode);

	CheckDlgButton(hWnd2, cbAlwaysOnTop, gpSet->isAlwaysOnTop);

	CheckDlgButton(hWnd2, cbSleepInBackground, gpSet->isSleepInBackground);

	CheckDlgButton(hWnd2, cbDisableAllFlashing, gpSet->isDisableAllFlashing);

	CheckDlgButton(hWnd2, cbVisible, gpSet->isConVisible);

	//#ifndef _DEBUG
	//WARNING("isUseInjects отключен принудительно");
	//if (!gpSet->isUseInjects)
	//	EnableWindow(GetDlgItem(hWnd2, cbUseInjects), FALSE); // Сохранение запрещено
	//#endif
	CheckDlgButton(hWnd2, cbUseInjects, gpSet->isUseInjects);

	CheckDlgButton(hWnd2, cbDosBox, gpConEmu->mb_DosBoxExists);
	// изменение пока запрещено
	// или чтобы ругнуться, если DosBox не установлен
	EnableWindow(GetDlgItem(hWnd2, cbDosBox), !gpConEmu->mb_DosBoxExists);
	//EnableWindow(GetDlgItem(hWnd2, bDosBoxSettings), FALSE); // изменение пока запрещено
	ShowWindow(GetDlgItem(hWnd2, bDosBoxSettings), SW_HIDE);

	#ifdef USEPORTABLEREGISTRY
	if (gpConEmu->mb_PortableRegExist)
	{
		if (gpSet->isPortableReg)
			CheckDlgButton(hWnd2, cbPortableRegistry, BST_CHECKED);
		EnableWindow(GetDlgItem(hWnd2, bPortableRegistrySettings), FALSE); // изменение пока запрещено
	}
	else
	#endif
	{
		EnableWindow(GetDlgItem(hWnd2, cbPortableRegistry), FALSE); // изменение пока запрещено
		EnableWindow(GetDlgItem(hWnd2, bPortableRegistrySettings), FALSE); // изменение пока запрещено
	}
	
	#ifdef _DEBUG
	CheckDlgButton(hWnd2, cbTabsInCaption, gpSet->isTabsInCaption);
	#else
	ShowWindow(GetDlgItem(hWnd2, cbTabsInCaption), SW_HIDE);
	#endif


	CheckDlgButton(hWnd2, cbHideCaption, gpSet->isHideCaption);

	CheckDlgButton(hWnd2, cbHideCaptionAlways, gpSet->isHideCaptionAlways());

	SetDlgItemInt(hWnd2, tHideCaptionAlwaysFrame, gpSet->nHideCaptionAlwaysFrame, FALSE);
	SetDlgItemInt(hWnd2, tHideCaptionAlwaysDelay, gpSet->nHideCaptionAlwaysDelay, FALSE);
	SetDlgItemInt(hWnd2, tHideCaptionAlwaysDissapear, gpSet->nHideCaptionAlwaysDisappear, FALSE);

	RegisterTipsFor(hWnd2);
	return 0;
}

LRESULT CSettings::OnInitDialog_Selection(HWND hWnd2)
{
	CheckRadioButton(hWnd2, rbCTSNever, rbCTSBufferOnly,
		(gpSet->isConsoleTextSelection == 0) ? rbCTSNever :
		(gpSet->isConsoleTextSelection == 1) ? rbCTSAlways : rbCTSBufferOnly);

	CheckDlgButton(hWnd2, cbCTSBlockSelection, gpSet->isCTSSelectBlock);
	FillListBoxByte(hWnd2, lbCTSBlockSelection, SettingsNS::szKeys, SettingsNS::nKeys, gpSet->isCTSVkBlock);
	CheckDlgButton(hWnd2, cbCTSTextSelection, gpSet->isCTSSelectText);
	FillListBoxByte(hWnd2, lbCTSTextSelection, SettingsNS::szKeys, SettingsNS::nKeys, gpSet->isCTSVkText);
	CheckDlgButton(hWnd2, (gpSet->isCTSActMode==1)?rbCTSActAlways:rbCTSActBufferOnly, BST_CHECKED);
	FillListBoxByte(hWnd2, lbCTSActAlways, SettingsNS::szKeysAct, SettingsNS::nKeysAct, gpSet->isCTSVkAct);
	FillListBoxByte(hWnd2, lbCTSRBtnAction, SettingsNS::szClipAct, SettingsNS::nClipAct, gpSet->isCTSRBtnAction);
	FillListBoxByte(hWnd2, lbCTSMBtnAction, SettingsNS::szClipAct, SettingsNS::nClipAct, gpSet->isCTSMBtnAction);
	DWORD idxBack = (gpSet->isCTSColorIndex & 0xF0) >> 4;
	DWORD idxFore = (gpSet->isCTSColorIndex & 0xF);
	FillListBoxItems(GetDlgItem(hWnd2, lbCTSForeIdx), countof(SettingsNS::szColorIdx)-1,
		SettingsNS::szColorIdx, SettingsNS::nColorIdx, idxFore);
	FillListBoxItems(GetDlgItem(hWnd2, lbCTSBackIdx), countof(SettingsNS::szColorIdx)-1,
		SettingsNS::szColorIdx, SettingsNS::nColorIdx, idxBack);
	// Не совсем выделение, но в том же диалоге
	CheckDlgButton(hWnd2, cbFarGotoEditor, gpSet->isFarGotoEditor);
	FillListBoxByte(hWnd2, lbFarGotoEditorVk, SettingsNS::szKeysAct, SettingsNS::nKeysAct, gpSet->isFarGotoEditorVk);

	// тултипы
	RegisterTipsFor(hWnd2);
	return 0;
}

LRESULT CSettings::OnInitDialog_Far(HWND hWnd2, BOOL abInitial)
{
	#if 0
	if (gpSetCls->EnableThemeDialogTextureF)
		gpSetCls->EnableThemeDialogTextureF(hWnd2, 6/*ETDT_ENABLETAB*/);
	#endif

	// Сначала - то что обновляется при активации вкладки

	// Списки
	FillListBoxByte(hWnd2, lbLDragKey, SettingsNS::szKeys, SettingsNS::nKeys, gpSet->nLDragKey);
	FillListBoxByte(hWnd2, lbRDragKey, SettingsNS::szKeys, SettingsNS::nKeys, gpSet->nRDragKey);

	if (!abInitial)
		return 0;

	if (gpSet->isRClickSendKey) CheckDlgButton(hWnd2, cbRClick, (gpSet->isRClickSendKey==1) ? BST_CHECKED : BST_INDETERMINATE);
	SetDlgItemText(hWnd2, tRClickMacro, gpSet->RClickMacro());

	if (gpSet->isSafeFarClose) CheckDlgButton(hWnd2, cbSafeFarClose, BST_CHECKED);
	SetDlgItemText(hWnd2, tSafeFarCloseMacro, gpSet->SafeFarCloseMacro());

	SetDlgItemText(hWnd2, tCloseTabMacro, gpSet->TabCloseMacro());
	
	SetDlgItemText(hWnd2, tSaveAllMacro, gpSet->SaveAllMacro());

	if (gpSet->isFARuseASCIIsort) CheckDlgButton(hWnd2, cbFARuseASCIIsort, BST_CHECKED);

	if (gpSet->isShellNoZoneCheck) CheckDlgButton(hWnd2, cbShellNoZoneCheck, BST_CHECKED);

	if (gpSet->isFarHourglass) CheckDlgButton(hWnd2, cbFarHourglass, BST_CHECKED);

	if (gpSet->isExtendUCharMap) CheckDlgButton(hWnd2, cbExtendUCharMap, BST_CHECKED);

	if (gpSet->isDragEnabled)
	{
		//CheckDlgButton(hWnd2, cbDragEnabled, BST_CHECKED);
		if (gpSet->isDragEnabled & DRAG_L_ALLOWED) CheckDlgButton(hWnd2, cbDragL, BST_CHECKED);

		if (gpSet->isDragEnabled & DRAG_R_ALLOWED) CheckDlgButton(hWnd2, cbDragR, BST_CHECKED);
	}

	if (gpSet->isDropEnabled) CheckDlgButton(hWnd2, cbDropEnabled, (gpSet->isDropEnabled==1) ? BST_CHECKED : BST_INDETERMINATE);

	if (gpSet->isDefCopy) CheckDlgButton(hWnd2, cbDnDCopy, BST_CHECKED);

	// Overlay
	if (gpSet->isDragOverlay) CheckDlgButton(hWnd2, cbDragImage, (gpSet->isDragOverlay==1) ? BST_CHECKED : BST_INDETERMINATE);

	if (gpSet->isDragShowIcons) CheckDlgButton(hWnd2, cbDragIcons, BST_CHECKED);

	if (gpSet->isRSelFix)
		CheckDlgButton(hWnd2, cbRSelectionFix, BST_CHECKED);

	CheckDlgButton(hWnd2, cbDragPanel, gpSet->isDragPanel);

	if (gpSet->isDisableFarFlashing) CheckDlgButton(hWnd2, cbDisableFarFlashing, gpSet->isDisableFarFlashing);

	RegisterTipsFor(hWnd2);
	return 0;
}

void CSettings::GetVkKeyName(BYTE vk, wchar_t (&szName)[128])
{
	szName[0] = 0;

	switch (vk)
	{
	case VK_LWIN:
	case VK_RWIN:
		wcscat_c(szName, L"Win"); break;
	case VK_CONTROL:
		wcscat_c(szName, L"Ctrl"); break;
	case VK_LCONTROL:
		wcscat_c(szName, L"LCtrl"); break;
	case VK_RCONTROL:
		wcscat_c(szName, L"RCtrl"); break;
	case VK_MENU:
		wcscat_c(szName, L"Alt"); break;
	case VK_LMENU:
		wcscat_c(szName, L"LAlt"); break;
	case VK_RMENU:
		wcscat_c(szName, L"RAlt"); break;
	case VK_SHIFT:
		wcscat_c(szName, L"Shift"); break;
	case VK_LSHIFT:
		wcscat_c(szName, L"LShift"); break;
	case VK_RSHIFT:
		wcscat_c(szName, L"RShift"); break;
	case VK_APPS:
		wcscat_c(szName, L"Apps"); break;
	case VK_LEFT:
		wcscat_c(szName, L"Left"); break;
	case VK_RIGHT:
		wcscat_c(szName, L"Right"); break;
	case VK_UP:
		wcscat_c(szName, L"Up"); break;
	case VK_DOWN:
		wcscat_c(szName, L"Down"); break;
	case VK_PRIOR:
		wcscat_c(szName, L"PgUp"); break;
	case VK_NEXT:
		wcscat_c(szName, L"PgDn"); break;
	case VK_SPACE:
		wcscat_c(szName, L"Space"); break;
	case VK_TAB:
		wcscat_c(szName, L"Tab"); break;
	case VK_ESCAPE:
		wcscat_c(szName, L"Esc"); break;
	case VK_INSERT:
		wcscat_c(szName, L"Insert"); break;
	case VK_DELETE:
		wcscat_c(szName, L"Delete"); break;
	case VK_HOME:
		wcscat_c(szName, L"Home"); break;
	case VK_END:
		wcscat_c(szName, L"End"); break;
	case VK_PAUSE:
		wcscat_c(szName, L"Pause"); break;
	case VK_RETURN:
		wcscat_c(szName, L"Enter"); break;
	case VK_BACK:
		wcscat_c(szName, L"Backspace"); break;

	default:
		if (vk >= VK_F1 && vk <= VK_F24)
		{
			_wsprintf(szName, SKIPLEN(countof(szName)) L"F%u", (DWORD)vk-VK_F1+1);
		}
		else if ((vk >= (BYTE)'A' && vk <= (BYTE)'Z') || (vk >= (BYTE)'0' && vk <= (BYTE)'9'))
		{
			szName[0] = vk;
			szName[1] = 0;
		}
		else
		{
			szName[0] = MapVirtualKey(vk, MAPVK_VK_TO_CHAR);
			szName[1] = 0;
			//BYTE States[256] = {};
			//// Скорее всго не сработает
			//if (!ToUnicode(vk, 0, States, szName, countof(szName), 0))
			//	_wsprintf(szName, SKIPLEN(countof(szName)) L"<%u>", (DWORD)vk);
			// есть еще if (!GetKeyNameText((LONG)(DWORD)*m_HotKeys[i].VkPtr, szName, countof(szName)))
		}
	}
}

void CSettings::FillHotKeysList(BOOL abInitial)
{
	if (!m_HotKeys)
	{
		_ASSERTE(m_HotKeys!=NULL);
		return;
	}

	// Населить список всеми хоткеями
	HWND hList = GetDlgItem(hKeys, lbConEmuHotKeys);
	if (abInitial)
	{
		ListView_DeleteAllItems(hList);
	}


	wchar_t szName[128], szFull[512];
	//HWND hDetails = GetDlgItem(hWnd2, lbActivityDetails);
	LVITEM lvi = {LVIF_TEXT|LVIF_STATE|LVIF_PARAM};
	lvi.state = 0;
	lvi.stateMask = LVIS_SELECTED|LVIS_FOCUSED;
	lvi.pszText = szName;
	for (size_t i = 0; m_HotKeys[i].DescrLangID; i++)
	{
		wcscpy_c(szName, (m_HotKeys[i].Type == 1) ? L"Modifier" : m_HotKeys[i].VkPtr ? L"User" : L"System");
		int nItem = -1;
		if (!abInitial)
		{
			int Items = (int)ListView_GetItemCount(hList);
			for (int j = 0; j < Items; j++)
			{
				LVITEM lvf = {LVIF_PARAM, j};
				if (!ListView_GetItem(hList, &lvf))
				{
					_ASSERTE(ListView_GetItem(hList, &lvf));
					break;
				}
				if (lvf.lParam == (LPARAM)(m_HotKeys+i))
				{
					nItem = j;
					break;
				}
			}
			_ASSERTE(nItem!=-1);
		}
		if (nItem == -1)
		{
			lvi.iItem = i + 1; // в конец
			lvi.lParam = (LPARAM)(m_HotKeys+i);
			nItem = ListView_InsertItem(hList, &lvi);
		}
		if (abInitial)
		{
			ListView_SetItemState(hList, nItem, 0, LVIS_SELECTED|LVIS_FOCUSED);
		}
		
		szFull[0] = 0;
		
		DWORD nModifier = (m_HotKeys[i].Type == 1) ? 0 : m_HotKeys[i].VkPtr ? (DWORD)-1/*nMultiHotkeyModifier*/ : m_HotKeys[i].Modifier;
		if (nModifier == (DWORD)-1)
		{
			//nModifier = nMultiHotkeyModifier; // для Win-Number
			wcscpy_c(szFull, L"\xAB"/*"«"*/ L"Host" L"\xBB"/*L"»"*/);
			if ((m_HotKeys[i].DescrLangID == vkMultiNextShift) || (m_HotKeys[i].DescrLangID == vkMultiNewShift))
				wcscat_c(szFull, L"-Shift");
		}
		else
		{
			if ((m_HotKeys[i].DescrLangID == vkMultiNextShift) || (m_HotKeys[i].DescrLangID == vkMultiNewShift))
			{
				if (nModifier & VK_SHIFT)
					nModifier &= ~VK_SHIFT;
				else
					nModifier |= VK_SHIFT;
			}
			
			while (nModifier)
			{
				LONG vk = (LONG)(nModifier & 0xFF);
				// Не более 3-х модификаторов!
				nModifier = (nModifier & 0xFFFF00) >> 8;
				GetVkKeyName(vk, szName);
				if (szFull[0])
					wcscat_c(szFull, L"-");
				wcscat_c(szFull, szName);
			}
		}
		
		szName[0] = 0;
		if (m_HotKeys[i].VkPtr)
		{
			TODO("Для списков там может быть что-то типа <Auto>");
			if (*m_HotKeys[i].VkPtr)
			{
				GetVkKeyName(*m_HotKeys[i].VkPtr, szName);
			}
		}
		else if (m_HotKeys[i].Vk)
		{
			GetVkKeyName(m_HotKeys[i].Vk, szName);
		}
		
		if (szName[0])
		{
			if (szFull[0])
				wcscat_c(szFull, L"-");
			wcscat_c(szFull, szName);
		}
		else
		{
			wcscpy_c(szFull, L"<None>");
		}
		ListView_SetItemText(hList, nItem, klc_Hotkey, szFull);
		
		if (!LoadString(g_hInstance, m_HotKeys[i].DescrLangID, szName, countof(szName)))
			_wsprintf(szName, SKIPLEN(countof(szName)) L"%i", m_HotKeys[i].DescrLangID);
		ListView_SetItemText(hList, nItem, klc_Desc, szName);
	}

	//ListView_SetSelectionMark(hList, -1);
}

LRESULT CSettings::OnHotkeysNotify(HWND hWnd2, WPARAM wParam, LPARAM lParam)
{
	switch (((NMHDR*)lParam)->code)
	{
	case LVN_ITEMCHANGED:
		{
			LPNMLISTVIEW p = (LPNMLISTVIEW)lParam;
			HWND hHk = GetDlgItem(hWnd2, hkHotKeySelect);
			if ((p->uNewState & LVIS_SELECTED) && (p->iItem >= 0))
			{
				HWND hList = GetDlgItem(hWnd2, lbConEmuHotKeys);
				LVITEM lvi = {LVIF_PARAM, p->iItem};
				ConEmuHotKeys* pk = NULL;
				if (ListView_GetItem(hList, &lvi))
					pk = (ConEmuHotKeys*)lvi.lParam;
				if (pk && !(pk->DescrLangID && pk->VkPtr))
					pk = NULL;
				mp_ActiveHotKey = pk;

				if (pk)
				{
					SetDlgItemText(hWnd2, stHotKeySelect, (pk->Type == 0) ? L"Choose hotkey:" : L"Choose modifier:");
					EnableWindow(GetDlgItem(hWnd2, stHotKeySelect), TRUE);
					EnableWindow(GetDlgItem(hWnd2, lbHotKeyList), TRUE);
					EnableWindow(hHk, (pk->Type == 0));

					UINT nVK = pk ? *pk->VkPtr : 0;
					if (pk->Type == 0)
					{
						SendMessage(hHk, HKM_SETHOTKEY, nVK==VK_DELETE ? (VK_DELETE|(HOTKEYF_EXT<<8)) : nVK, 0);
						// Warning! Если nVK не указан в SettingsNS::nKeysHot - nVK будет обнулен
						FillListBoxByte(hWnd2, lbHotKeyList, SettingsNS::szKeysHot, SettingsNS::nKeysHot, nVK);
					}
					else
					{
						SendMessage(hHk, HKM_SETHOTKEY, 0, 0);
						FillListBoxByte(hWnd2, lbHotKeyList, SettingsNS::szKeys, SettingsNS::nKeys, nVK);
					}
				}
			}
			else
			{
				mp_ActiveHotKey = NULL;
			}

			if (!mp_ActiveHotKey)
			{
				SetDlgItemText(hWnd2, stHotKeySelect, L"Choose hotkey:");
				EnableWindow(GetDlgItem(hWnd2, stHotKeySelect), FALSE);
				EnableWindow(GetDlgItem(hWnd2, lbHotKeyList), FALSE);
				EnableWindow(hHk, FALSE);
				SendMessage(hHk, HKM_SETHOTKEY, 0, 0);
				SendDlgItemMessage(hWnd2, lbHotKeyList, CB_RESETCONTENT, 0, 0);
			}
		} //LVN_ITEMCHANGED
		break;
	}
	return 0;
}

LRESULT CSettings::OnInitDialog_Keys(HWND hWnd2, BOOL abInitial)
{
	#if 0
	if (gpSetCls->EnableThemeDialogTextureF)
		gpSetCls->EnableThemeDialogTextureF(hKeys, 6/*ETDT_ENABLETAB*/);
	#endif

	if (!abInitial)
	{
		FillHotKeysList(abInitial);
		return 0;
	}

	HWND hList = GetDlgItem(hKeys, lbConEmuHotKeys);
	mp_ActiveHotKey = NULL;
	
	HWND hTip = ListView_GetToolTips(hList);
	SetWindowPos(hTip, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);

	// Убедиться, что поле клавиши идет поверх выпадающего списка
	HWND hHk = GetDlgItem(hWnd2, hkHotKeySelect);
	SendDlgItemMessage(hWnd2, hkHotKeySelect, HKM_SETRULES, HKCOMB_A|HKCOMB_C|HKCOMB_CA|HKCOMB_S|HKCOMB_SA|HKCOMB_SC|HKCOMB_SCA, 0);

	// Создать колонки
	{
		LVCOLUMN col ={LVCF_WIDTH|LVCF_TEXT|LVCF_FMT, LVCFMT_LEFT, 60};
		wchar_t szTitle[64]; col.pszText = szTitle;

		ListView_SetExtendedListViewStyleEx(hList,LVS_EX_FULLROWSELECT,LVS_EX_FULLROWSELECT);
		ListView_SetExtendedListViewStyleEx(hList,LVS_EX_LABELTIP|LVS_EX_INFOTIP,LVS_EX_LABELTIP|LVS_EX_INFOTIP);
		
		wcscpy_c(szTitle, L"Type");			ListView_InsertColumn(hList, klc_Type, &col);
		col.cx = 120;
		wcscpy_c(szTitle, L"Hotkey");		ListView_InsertColumn(hList, klc_Hotkey, &col);
		col.cx = 300;
		wcscpy_c(szTitle, L"Description");	ListView_InsertColumn(hList, klc_Desc, &col);
	}

	FillHotKeysList(abInitial);
		
	SetupHotkeyChecks(hKeys);

	if (gpSet->isUseWinArrows)
		CheckDlgButton(hKeys, cbUseWinArrows, BST_CHECKED);
	if (gpSet->isUseWinNumber)
		CheckDlgButton(hKeys, cbUseWinNumber, BST_CHECKED);
	if (gpSet->isUseWinTab)
		CheckDlgButton(hKeys, cbUseWinTab, BST_CHECKED);

	CheckDlgButton(hKeys, cbInstallKeybHooks,
	               (gpSet->m_isKeyboardHooks == 1) ? BST_CHECKED :
	               ((gpSet->m_isKeyboardHooks == 0) ? BST_INDETERMINATE : BST_UNCHECKED));
	
	if (gpSet->isSendAltEnter) CheckDlgButton(hKeys, cbSendAltEnter, BST_CHECKED);
	if (gpSet->isSendAltSpace) CheckDlgButton(hKeys, cbSendAltSpace, BST_CHECKED);
	if (gpSet->isSendAltTab) CheckDlgButton(hKeys, cbSendAltTab, BST_CHECKED);
	if (gpSet->isSendAltEsc) CheckDlgButton(hKeys, cbSendAltEsc, BST_CHECKED);
	if (gpSet->isSendAltPrintScrn) CheckDlgButton(hKeys, cbSendAltPrintScrn, BST_CHECKED);
	if (gpSet->isSendPrintScrn) CheckDlgButton(hKeys, cbSendPrintScrn, BST_CHECKED);
	if (gpSet->isSendCtrlEsc) CheckDlgButton(hKeys, cbSendCtrlEsc, BST_CHECKED);

	RegisterTipsFor(hKeys);
	return 0;
}

LRESULT CSettings::OnInitDialog_Tabs(HWND hWnd2)
{
	CheckDlgButton(hWnd2, cbTabs, gpSet->isTabs);

	CheckDlgButton(hWnd2, cbTabSelf, gpSet->isTabSelf);

	CheckDlgButton(hWnd2, cbTabRecent, gpSet->isTabRecent);

	CheckDlgButton(hWnd2, cbTabLazy, gpSet->isTabLazy);

	CheckDlgButton(hWnd2, cbTabsOnTaskBar, gpSet->m_isTabsOnTaskBar);

	CheckDlgButton(hWnd2, cbHideInactiveConTabs, gpSet->bHideInactiveConsoleTabs);
	CheckDlgButton(hWnd2, cbHideDisabledTabs, gpSet->bHideDisabledTabs);
	CheckDlgButton(hWnd2, cbShowFarWindows, gpSet->bShowFarWindows);

	SetDlgItemText(hWnd2, tTabFontFace, gpSet->sTabFontFace);

	if (gpSetCls->mh_EnumThread == NULL)  // Если шрифты уже считаны
		OnInitDialog_CopyFonts(hWnd2, tTabFontFace, 0); // можно скопировать список с вкладки hMain

	DWORD nVal = gpSet->nTabFontHeight;
	FillListBoxInt(hWnd2, tTabFontHeight, SettingsNS::FSizesSmall, nVal);

	FillListBoxCharSet(hWnd2, tTabFontCharset, gpSet->nTabFontCharSet);

	CheckDlgButton(hWnd2, cbMultiCon, gpSet->isMulti);

	CheckDlgButton(hWnd2, cbMultiIterate, gpSet->isMultiIterate);

	CheckDlgButton(hWnd2, cbNewConfirm, gpSet->isMultiNewConfirm);

	CheckDlgButton(hWnd2, cbLongOutput, gpSet->AutoBufferHeight);

	wchar_t sz[16];
	SendDlgItemMessage(hWnd2, tLongOutputHeight, EM_SETLIMITTEXT, 5, 0);
	SetDlgItemText(hWnd2, tLongOutputHeight, _ltow(gpSet->DefaultBufferHeight, sz, 10));
	EnableWindow(GetDlgItem(hWnd2, tLongOutputHeight), gpSet->AutoBufferHeight);
	// 16bit Height
	SendDlgItemMessage(hWnd2, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"Auto");
	SendDlgItemMessage(hWnd2, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"25 lines");
	SendDlgItemMessage(hWnd2, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"28 lines");
	SendDlgItemMessage(hWnd2, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"43 lines");
	SendDlgItemMessage(hWnd2, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"50 lines");
	SendDlgItemMessage(hWnd2, lbNtvdmHeight, CB_SETCURSEL, !gpSet->ntvdmHeight ? 0 :
	                   ((gpSet->ntvdmHeight == 25) ? 1 : ((gpSet->ntvdmHeight == 28) ? 2 : ((gpSet->ntvdmHeight == 43) ? 3 : 4))), 0); //-V112
	// Cmd.exe output cp
	SendDlgItemMessage(hWnd2, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"Undefined");
	SendDlgItemMessage(hWnd2, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"Automatic");
	SendDlgItemMessage(hWnd2, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"Unicode");
	SendDlgItemMessage(hWnd2, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"OEM");
	SendDlgItemMessage(hWnd2, lbCmdOutputCP, CB_SETCURSEL, gpSet->nCmdOutputCP, 0);
	//
	//SetupHotkeyChecks(hTabs);
	//SendDlgItemMessage(hTabs, hkNewConsole, HKM_SETRULES, HKCOMB_A|HKCOMB_C|HKCOMB_CA|HKCOMB_S|HKCOMB_SA|HKCOMB_SC|HKCOMB_SCA, 0);
	//SendDlgItemMessage(hTabs, hkNewConsole, HKM_SETHOTKEY, gpSet->icMultiNew, 0);
	//SendDlgItemMessage(hTabs, hkSwitchConsole, HKM_SETRULES, HKCOMB_A|HKCOMB_C|HKCOMB_CA|HKCOMB_S|HKCOMB_SA|HKCOMB_SC|HKCOMB_SCA, 0);
	//SendDlgItemMessage(hTabs, hkSwitchConsole, HKM_SETHOTKEY, gpSet->icMultiNext, 0);
	//SendDlgItemMessage(hTabs, hkCloseConsole, HKM_SETRULES, HKCOMB_A|HKCOMB_C|HKCOMB_CA|HKCOMB_S|HKCOMB_SA|HKCOMB_SC|HKCOMB_SCA, 0);
	//SendDlgItemMessage(hTabs, hkCloseConsole, HKM_SETHOTKEY, gpSet->icMultiRecreate, 0);
	//SendDlgItemMessage(hTabs, hkDelConsole, HKM_SETRULES, HKCOMB_A|HKCOMB_C|HKCOMB_CA|HKCOMB_S|HKCOMB_SA|HKCOMB_SC|HKCOMB_SCA, 0);
	//SendDlgItemMessage(hTabs, hkDelConsole, HKM_SETHOTKEY, gpSet->icMultiClose, 0);
	//SendDlgItemMessage(hTabs, hkStartCmd, HKM_SETRULES, HKCOMB_A|HKCOMB_C|HKCOMB_CA|HKCOMB_S|HKCOMB_SA|HKCOMB_SC|HKCOMB_SCA, 0);
	//SendDlgItemMessage(hTabs, hkStartCmd, HKM_SETHOTKEY, gpSet->icMultiCmd, 0);
	//SendDlgItemMessage(hTabs, hkMinRestore, HKM_SETRULES, HKCOMB_A|HKCOMB_C|HKCOMB_CA|HKCOMB_S|HKCOMB_SA|HKCOMB_SC|HKCOMB_SCA, 0);
	//SendDlgItemMessage(hTabs, hkMinRestore, HKM_SETHOTKEY, gpSet->icMinimizeRestore, 0);
	//SendDlgItemMessage(hTabs, hkBufferMode, HKM_SETRULES, HKCOMB_A|HKCOMB_C|HKCOMB_CA|HKCOMB_S|HKCOMB_SA|HKCOMB_SC|HKCOMB_SCA, 0);
	//SendDlgItemMessage(hTabs, hkBufferMode, HKM_SETHOTKEY, gpSet->icMultiBuffer, 0);
	// SendDlgItemMessage(hTabs, hkMinimizeRestore, HKM_SETHOTKEY, gpSet->icMinimizeRestore, 0);
	//if (gpSet->isUseWinNumber)
	//	CheckDlgButton(hTabs, cbUseWinNumber, BST_CHECKED);

	SetDlgItemText(hWnd2, tTabConsole, gpSet->szTabConsole);
	SetDlgItemText(hWnd2, tTabViewer, gpSet->szTabViewer);
	SetDlgItemText(hWnd2, tTabEditor, gpSet->szTabEditor);
	SetDlgItemText(hWnd2, tTabEditorMod, gpSet->szTabEditorModified);
	SetDlgItemInt(hWnd2, tTabLenMax, gpSet->nTabLenMax, FALSE);

	CheckRadioButton(hWnd2, rbAdminShield, rbAdminSuffix, gpSet->bAdminShield ? rbAdminShield : rbAdminSuffix);
	SetDlgItemText(hWnd2, tAdminSuffix, gpSet->szAdminTitleSuffix);

	RegisterTipsFor(hWnd2);
	return 0;
}

LRESULT CSettings::OnInitDialog_Color(HWND hWnd2)
{
	#if 0
	if (gpSetCls->EnableThemeDialogTextureF)
		gpSetCls->EnableThemeDialogTextureF(hColors, 6/*ETDT_ENABLETAB*/);
	#endif

#define getR(inColorref) (byte)inColorref
#define getG(inColorref) (byte)(inColorref >> 8)
#define getB(inColorref) (byte)(inColorref >> 16)

	//wchar_t temp[MAX_PATH];

	for(uint c = c0; c <= MAX_COLOR_EDT_ID; c++)
		ColorSetEdit(hColors, c);

	DWORD nVal = gpSet->nExtendColor;
	FillListBox(hColors, lbExtendIdx, SettingsNS::szColorIdxSh, SettingsNS::nColorIdxSh, nVal);
	gpSet->nExtendColor = nVal;
	CheckDlgButton(hColors, cbExtendColors, gpSet->isExtendColors ? BST_CHECKED : BST_UNCHECKED);
	OnButtonClicked(hColors, cbExtendColors, 0);
	CheckDlgButton(hColors, cbTrueColorer, gpSet->isTrueColorer ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hColors, cbFadeInactive, gpSet->isFadeInactive ? BST_CHECKED : BST_UNCHECKED);
	SetDlgItemInt(hColors, tFadeLow, gpSet->mn_FadeLow, FALSE);
	SetDlgItemInt(hColors, tFadeHigh, gpSet->mn_FadeHigh, FALSE);
	// Default colors
	memmove(gdwLastColors, gpSet->Colors, sizeof(gdwLastColors));
	SendDlgItemMessage(hColors, lbDefaultColors, CB_ADDSTRING, 0, (LPARAM) L"<Current color scheme>");

	//SendDlgItemMessage(hColors, lbDefaultColors, CB_ADDSTRING, 0, (LPARAM) L"Default color sheme (Windows standard)");
	//SendDlgItemMessage(hColors, lbDefaultColors, CB_ADDSTRING, 0, (LPARAM) L"Gamma 1 (for use with dark monitors)");
	for(uint i=0; i<countof(DefColors); i++)
		SendDlgItemMessage(hColors, lbDefaultColors, CB_ADDSTRING, 0, (LPARAM) DefColors[i].pszTitle);

	SendDlgItemMessage(hColors, lbDefaultColors, CB_SETCURSEL, 0, 0);
	gbLastColorsOk = TRUE;
	SendDlgItemMessage(hColors, slTransparent, TBM_SETRANGE, (WPARAM) true, (LPARAM) MAKELONG(MIN_ALPHA_VALUE, 255));
	SendDlgItemMessage(hColors, slTransparent, TBM_SETPOS  , (WPARAM) true, (LPARAM) gpSet->nTransparent);
	CheckDlgButton(hColors, cbTransparent, (gpSet->nTransparent!=255) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hColors, cbUserScreenTransparent, gpSet->isUserScreenTransparent ? BST_CHECKED : BST_UNCHECKED);
	RegisterTipsFor(hColors);
	return 0;
}

LRESULT CSettings::OnInitDialog_Views(HWND hWnd2)
{
	// пока выключим
	EnableWindow(GetDlgItem(hWnd2, bApplyViewSettings), gpConEmu->ActiveCon()->IsPanelViews());

	SetDlgItemText(hWnd2, tThumbsFontName, gpSet->ThSet.Thumbs.sFontName);
	SetDlgItemText(hWnd2, tTilesFontName, gpSet->ThSet.Tiles.sFontName);

	if (gpSetCls->mh_EnumThread == NULL)  // Если шрифты уже считаны
		OnInitDialog_CopyFonts(hWnd2, tThumbsFontName, tTilesFontName, 0); // можно скопировать список с вкладки hMain

	DWORD nVal;

	nVal = gpSet->ThSet.Thumbs.nFontHeight;
	FillListBoxInt(hWnd2, tThumbsFontSize, SettingsNS::FSizesSmall, nVal);

	nVal = gpSet->ThSet.Tiles.nFontHeight;
	FillListBoxInt(hWnd2, tTilesFontSize, SettingsNS::FSizesSmall, nVal);

	//wchar_t temp[MAX_PATH];
	//for (uint i=0; i < countof(SettingsNS::FSizesSmall); i++)
	//{
	//	_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", SettingsNS::FSizesSmall[i]);
	//	SendDlgItemMessage(hWnd2, tThumbsFontSize, CB_ADDSTRING, 0, (LPARAM) temp);
	//	SendDlgItemMessage(hWnd2, tTilesFontSize, CB_ADDSTRING, 0, (LPARAM) temp);
	//}
	//_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", gpSet->ThSet.Thumbs.nFontHeight);
	//SelectStringExact(hWnd2, tThumbsFontSize, temp);
	//_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", gpSet->ThSet.Tiles.nFontHeight);
	//SelectStringExact(hWnd2, tTilesFontSize, temp);

	SetDlgItemInt(hWnd2, tThumbsImgSize, gpSet->ThSet.Thumbs.nImgSize, FALSE);
	SetDlgItemInt(hWnd2, tThumbsX1, gpSet->ThSet.Thumbs.nSpaceX1, FALSE);
	SetDlgItemInt(hWnd2, tThumbsY1, gpSet->ThSet.Thumbs.nSpaceY1, FALSE);
	SetDlgItemInt(hWnd2, tThumbsX2, gpSet->ThSet.Thumbs.nSpaceX2, FALSE);
	SetDlgItemInt(hWnd2, tThumbsY2, gpSet->ThSet.Thumbs.nSpaceY2, FALSE);
	SetDlgItemInt(hWnd2, tThumbsSpacing, gpSet->ThSet.Thumbs.nLabelSpacing, FALSE);
	SetDlgItemInt(hWnd2, tThumbsPadding, gpSet->ThSet.Thumbs.nLabelPadding, FALSE);
	SetDlgItemInt(hWnd2, tTilesImgSize, gpSet->ThSet.Tiles.nImgSize, FALSE);
	SetDlgItemInt(hWnd2, tTilesX1, gpSet->ThSet.Tiles.nSpaceX1, FALSE);
	SetDlgItemInt(hWnd2, tTilesY1, gpSet->ThSet.Tiles.nSpaceY1, FALSE);
	SetDlgItemInt(hWnd2, tTilesX2, gpSet->ThSet.Tiles.nSpaceX2, FALSE);
	SetDlgItemInt(hWnd2, tTilesY2, gpSet->ThSet.Tiles.nSpaceY2, FALSE);
	SetDlgItemInt(hWnd2, tTilesSpacing, gpSet->ThSet.Tiles.nLabelSpacing, FALSE);
	SetDlgItemInt(hWnd2, tTilesPadding, gpSet->ThSet.Tiles.nLabelPadding, FALSE);
	FillListBox(hWnd2, tThumbMaxZoom, SettingsNS::szThumbMaxZoom, SettingsNS::nThumbMaxZoom, gpSet->ThSet.nMaxZoom);

	// Colors
	for(uint c = c32; c <= c34; c++)
		ColorSetEdit(hWnd2, c);

	nVal = gpSet->ThSet.crBackground.ColorIdx;
	FillListBox(hWnd2, lbThumbBackColorIdx, SettingsNS::szColorIdxTh, SettingsNS::nColorIdxTh, nVal);
	CheckRadioButton(hWnd2, rbThumbBackColorIdx, rbThumbBackColorRGB,
	                 gpSet->ThSet.crBackground.UseIndex ? rbThumbBackColorIdx : rbThumbBackColorRGB);
	CheckDlgButton(hWnd2, cbThumbPreviewBox, gpSet->ThSet.nPreviewFrame ? 1 : 0);
	nVal = gpSet->ThSet.crPreviewFrame.ColorIdx;
	FillListBox(hWnd2, lbThumbPreviewBoxColorIdx, SettingsNS::szColorIdxTh, SettingsNS::nColorIdxTh, nVal);
	CheckRadioButton(hWnd2, rbThumbPreviewBoxColorIdx, rbThumbPreviewBoxColorRGB,
	                 gpSet->ThSet.crPreviewFrame.UseIndex ? rbThumbPreviewBoxColorIdx : rbThumbPreviewBoxColorRGB);
	CheckDlgButton(hWnd2, cbThumbSelectionBox, gpSet->ThSet.nSelectFrame ? 1 : 0);
	nVal = gpSet->ThSet.crSelectFrame.ColorIdx;
	FillListBox(hWnd2, lbThumbSelectionBoxColorIdx, SettingsNS::szColorIdxTh, SettingsNS::nColorIdxTh, nVal);
	CheckRadioButton(hWnd2, rbThumbSelectionBoxColorIdx, rbThumbSelectionBoxColorRGB,
	                 gpSet->ThSet.crSelectFrame.UseIndex ? rbThumbSelectionBoxColorIdx : rbThumbSelectionBoxColorRGB);

	if ((gpSet->ThSet.bLoadPreviews & 3) == 3)
		CheckDlgButton(hWnd2, cbThumbLoadFiles, BST_CHECKED);
	else if ((gpSet->ThSet.bLoadPreviews & 3) == 1)
		CheckDlgButton(hWnd2, cbThumbLoadFiles, BST_INDETERMINATE);

	CheckDlgButton(hWnd2, cbThumbLoadFolders, gpSet->ThSet.bLoadFolders);
	SetDlgItemInt(hWnd2, tThumbLoadingTimeout, gpSet->ThSet.nLoadTimeout, FALSE);
	CheckDlgButton(hWnd2, cbThumbUsePicView2, gpSet->ThSet.bUsePicView2);
	CheckDlgButton(hWnd2, cbThumbRestoreOnStartup, gpSet->ThSet.bRestoreOnStartup);

	RegisterTipsFor(hWnd2);
	return 0;
}

// tThumbsFontName, tTilesFontName, 0
void CSettings::OnInitDialog_CopyFonts(HWND hWnd2, int nList1, ...)
{
	DWORD bAlmostMonospace;
	int nIdx, nCount, i;
	wchar_t szFontName[128]; // не должно быть более 32
	nCount = SendDlgItemMessage(gpSetCls->hMain, tFontFace, CB_GETCOUNT, 0, 0);

#ifdef _DEBUG
	GetDlgItemText(hWnd2, nList1, szFontName, 128);
#endif

	int nCtrls = 1;
	int nCtrlIds[10] = {nList1};
	va_list argptr;
	va_start(argptr, nList1);
	int nNext = va_arg( argptr, int );
	while (nNext)
	{
		nCtrlIds[nCtrls++] = nNext;
		nNext = va_arg( argptr, int );
	}


	for (i = 0; i < nCount; i++)
	{
		// Взять список шрифтов с главной страницы
		if (SendDlgItemMessage(gpSetCls->hMain, tFontFace, CB_GETLBTEXT, i, (LPARAM) szFontName) > 0)
		{
			// Показывать [Raster Fonts WxH] смысла нет
			if (szFontName[0] == L'[' && !wcsncmp(szFontName+1, RASTER_FONTS_NAME, _tcslen(RASTER_FONTS_NAME)))
				continue;

			bAlmostMonospace = (DWORD)SendDlgItemMessage(gpSetCls->hMain, tFontFace, CB_GETITEMDATA, i, 0);

			// Теперь создаем новые строки
			for (int j = 0; j < nCtrls; j++)
			{
				nIdx = SendDlgItemMessage(hWnd2, nCtrlIds[j], CB_ADDSTRING, 0, (LPARAM) szFontName);
				SendDlgItemMessage(hWnd2, nCtrlIds[j], CB_SETITEMDATA, nIdx, bAlmostMonospace);
			}
		}
	}

	for (int j = 0; j < nCtrls; j++)
	{
		GetDlgItemText(hWnd2, nCtrlIds[j], szFontName, 128);
		gpSetCls->SelectString(hWnd2, nCtrlIds[j], szFontName);
	}
}

LRESULT CSettings::OnInitDialog_Debug(HWND hWnd2)
{
	#if 0
	if (gpSetCls->EnableThemeDialogTextureF)
		gpSetCls->EnableThemeDialogTextureF(hDebug, 6/*ETDT_ENABLETAB*/);
	#endif

	//LVCOLUMN col ={LVCF_WIDTH|LVCF_TEXT|LVCF_FMT, LVCFMT_LEFT, 60};
	//wchar_t szTitle[64]; col.pszText = szTitle;

	CheckDlgButton(hDebug, rbActivityDisabled, BST_CHECKED);

	HWND hList = GetDlgItem(hDebug, lbActivityLog);
	ListView_SetExtendedListViewStyleEx(hList,LVS_EX_FULLROWSELECT,LVS_EX_FULLROWSELECT);
	ListView_SetExtendedListViewStyleEx(hList,LVS_EX_LABELTIP|LVS_EX_INFOTIP,LVS_EX_LABELTIP|LVS_EX_INFOTIP);

	LVCOLUMN col ={LVCF_WIDTH|LVCF_TEXT|LVCF_FMT, LVCFMT_LEFT, 60};
	wchar_t szTitle[4]; col.pszText = szTitle;
	wcscpy_c(szTitle, L" ");		ListView_InsertColumn(hList, 0, &col);
	
	HWND hTip = ListView_GetToolTips(hList);
	SetWindowPos(hTip, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);


	//HWND hDetails = GetDlgItem(hDebug, lbActivityDetails);
	//ListView_SetExtendedListViewStyleEx(hDetails,LVS_EX_FULLROWSELECT,LVS_EX_FULLROWSELECT);
	//ListView_SetExtendedListViewStyleEx(hDetails,LVS_EX_LABELTIP|LVS_EX_INFOTIP,LVS_EX_LABELTIP|LVS_EX_INFOTIP);
	//wcscpy_c(szTitle, L" ");		ListView_InsertColumn(hDetails, 0, &col);
	//col.cx = 60;
	//wcscpy_c(szTitle, L"Item");		ListView_InsertColumn(hDetails, 0, &col);
	//col.cx = 380;
	//wcscpy_c(szTitle, L"Details");	ListView_InsertColumn(hDetails, 1, &col);
	//wchar_t szTime[128];
	//LVITEM lvi = {LVIF_TEXT|LVIF_STATE};
	//lvi.state = lvi.stateMask = LVIS_SELECTED|LVIS_FOCUSED; lvi.pszText = szTime;
	//wcscpy_c(szTime, L"AppName");
	//ListView_InsertItem(hDetails, &lvi); lvi.iItem++;
	//wcscpy_c(szTime, L"CmdLine");
	//ListView_InsertItem(hDetails, &lvi); lvi.iItem++;
	////wcscpy_c(szTime, L"Details");
	////ListView_InsertItem(hDetails, &lvi);
	//hTip = ListView_GetToolTips(hDetails);
	//SetWindowPos(hTip, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);

	//ShowWindow(GetDlgItem(hDebug, gbInputActivity), SW_HIDE);
	//ShowWindow(GetDlgItem(hDebug, lbInputActivity), SW_HIDE);


	//hList = GetDlgItem(hDebug, lbInputActivity);
	//col.cx = 80;
	//wcscpy_c(szTitle, L"Type");			ListView_InsertColumn(hList, 0, &col);	col.cx = 120;
	//wcscpy_c(szTitle, L"Sent");			ListView_InsertColumn(hList, 1, &col);
	//wcscpy_c(szTitle, L"Received");		ListView_InsertColumn(hList, 2, &col);
	//wcscpy_c(szTitle, L"Description");	ListView_InsertColumn(hList, 3, &col);
	
	RegisterTipsFor(hDebug);
	return 0;
}

LRESULT CSettings::OnInitDialog_Update(HWND hWnd2)
{
	_ASSERTE(hUpdate==hWnd2);
	
	ConEmuUpdateSettings* p = &gpSet->UpdSet;
	
	// Через интерфейс - не редактируется
	SetDlgItemText(hWnd2, tUpdateVerLocation, p->UpdateVerLocation());
	
	CheckDlgButton(hWnd2, cbUpdateCheckOnStartup, p->isUpdateCheckOnStartup);
	CheckDlgButton(hWnd2, cbUpdateCheckHourly, p->isUpdateCheckHourly);
	CheckDlgButton(hWnd2, cbUpdateConfirmDownload, p->isUpdateConfirmDownload);
	CheckRadioButton(hWnd2, rbUpdateStableOnly, rbUpdateLatestAvailable, (p->isUpdateUseBuilds==1) ? rbUpdateStableOnly : rbUpdateLatestAvailable);
	CheckRadioButton(hWnd2, rbUpdateUseExe, rbUpdateUseArc, (p->UpdateDownloadSetup()==1) ? rbUpdateUseExe : rbUpdateUseArc);
	
	CheckDlgButton(hWnd2, cbUpdateUseProxy, p->isUpdateUseProxy);
	SetDlgItemText(hWnd2, tUpdateProxy, p->szUpdateProxy);
	SetDlgItemText(hWnd2, tUpdateProxyUser, p->szUpdateProxyUser);
	SetDlgItemText(hWnd2, tUpdateProxyPassword, p->szUpdateProxyPassword);
	
	SetDlgItemText(hWnd2, tUpdateExeCmdLine, p->UpdateExeCmdLine());
	SetDlgItemText(hWnd2, tUpdateArcCmdLine, p->UpdateArcCmdLine());
	SetDlgItemText(hWnd2, tUpdatePostUpdateCmd, p->szUpdatePostUpdateCmd);
	
	CheckDlgButton(hWnd2, cbUpdateLeavePackages, p->isUpdateLeavePackages);
	SetDlgItemText(hWnd2, tUpdateDownloadPath, p->szUpdateDownloadPath);

	RegisterTipsFor(hWnd2);
	return 0;
}

LRESULT CSettings::OnInitDialog_Info(HWND hWnd2)
{
	#if 0
	if (gpSetCls->EnableThemeDialogTextureF)
		gpSetCls->EnableThemeDialogTextureF(hInfo, 6/*ETDT_ENABLETAB*/);
	#endif

	SetDlgItemText(hInfo, tCurCmdLine, GetCommandLine());
	// Performance
	Performance(gbPerformance, TRUE);
	gpConEmu->UpdateProcessDisplay(TRUE);
	gpConEmu->UpdateSizes();
	gpConEmu->ActiveCon()->RCon()->UpdateCursorInfo();
	UpdateFontInfo();
	UpdateConsoleMode(gpConEmu->ActiveCon()->RCon()->GetConsoleStates());
	RegisterTipsFor(hInfo);
	return 0;
}

LRESULT CSettings::OnButtonClicked(HWND hWnd2, WPARAM wParam, LPARAM lParam)
{
	WORD CB = LOWORD(wParam);

	switch(CB)
	{
		case IDOK:
		case IDCANCEL:
		case IDCLOSE:
			// -- перенесено в WM_CLOSE
			//if (gpSet->isTabs==1) gpConEmu->ForceShowTabs(TRUE); else
			//if (gpSet->isTabs==0) gpConEmu->ForceShowTabs(FALSE); else
			//	gpConEmu->mp_TabBar->Update();
			//gpConEmu->OnPanelViewSettingsChanged();
			SendMessage(ghOpWnd, WM_CLOSE, 0, 0);
			break;
		case bSaveSettings:
			{
				bool isShiftPressed = isPressed(VK_SHIFT);

				if (IsWindowEnabled(GetDlgItem(hMain, cbApplyPos)))  // были изменения в полях размера/положения
					OnButtonClicked(NULL, cbApplyPos, 0);

				if (gpSet->SaveSettings())
				{
					if (!isShiftPressed)
						SendMessage(ghOpWnd,WM_COMMAND,IDOK,0);
				}
			}
			break;
		case rNoneAA:
		case rStandardAA:
		case rCTAA:
			PostMessage(hMain, gpSetCls->mn_MsgRecreateFont, wParam, 0);
			break;
		case rNormal:
		case rFullScreen:
		case rMaximized:
			//gpConEmu->SetWindowMode(wParam);
			EnableWindow(GetDlgItem(hMain, cbApplyPos), TRUE);
			//EnableWindow(GetDlgItem(hMain, tWndWidth), CB == rNormal);
			//EnableWindow(GetDlgItem(hMain, tWndHeight), CB == rNormal);
			//EnableWindow(GetDlgItem(hMain, tWndX), CB == rNormal);
			//EnableWindow(GetDlgItem(hMain, tWndY), CB == rNormal);
			//EnableWindow(GetDlgItem(hMain, rFixed), CB == rNormal);
			//EnableWindow(GetDlgItem(hMain, rCascade), CB == rNormal);
			break;
		case cbApplyPos:

			if (IsChecked(hMain, rNormal) == BST_CHECKED)
			{
				DWORD newX, newY;
				wchar_t temp[MAX_PATH];
				GetDlgItemText(hMain, tWndWidth, temp, MAX_PATH);  newX = klatoi(temp);
				GetDlgItemText(hMain, tWndHeight, temp, MAX_PATH); newY = klatoi(temp);
				SetFocus(GetDlgItem(hMain, rNormal));

				if (gpConEmu->isZoomed() || gpConEmu->isIconic() || gpConEmu->isFullScreen())
					gpConEmu->SetWindowMode(rNormal);

				// Установить размер
				TODO("DoubleView: непонятно что делать. То ли на два делить?");
				gpConEmu->SetConsoleWindowSize(MakeCoord(newX, newY), true, NULL);
			}
			else if (IsChecked(hMain, rMaximized) == BST_CHECKED)
			{
				SetFocus(GetDlgItem(hMain, rMaximized));

				if (!gpConEmu->isZoomed())
					gpConEmu->SetWindowMode(rMaximized);
			}
			else if (IsChecked(hMain, rFullScreen) == BST_CHECKED)
			{
				SetFocus(GetDlgItem(hMain, rFullScreen));

				if (!gpConEmu->isFullScreen())
					gpConEmu->SetWindowMode(rFullScreen);
			}

			// Запомнить "идеальный" размер окна, выбранный пользователем
			gpConEmu->UpdateIdealRect(TRUE);
			EnableWindow(GetDlgItem(hMain, cbApplyPos), FALSE);
			apiSetForegroundWindow(ghOpWnd);
			break;
		case rCascade:
		case rFixed:
			gpSet->wndCascade = CB == rCascade;
			break;
		case cbAutoSaveSizePos:
			gpSet->isAutoSaveSizePos = IsChecked(hMain, cbAutoSaveSizePos);
			break;
		case cbFontAuto:
			gpSet->isFontAutoSize = IsChecked(hMain, cbFontAuto);

			if (gpSet->isFontAutoSize && LogFont.lfFaceName[0] == L'['
			        && !wcsncmp(LogFont.lfFaceName+1, RASTER_FONTS_NAME, _tcslen(RASTER_FONTS_NAME)))
			{
				gpSet->isFontAutoSize = false;
				CheckDlgButton(hMain, cbFontAuto, BST_UNCHECKED);
				ShowFontErrorTip(szRasterAutoError);
			}

			break;
		case cbFixFarBorders:

			//gpSet->isFixFarBorders = !gpSet->isFixFarBorders;
			switch(IsChecked(hMain, cbFixFarBorders))
			{
				case BST_UNCHECKED:
					gpSet->isFixFarBorders = 0; break;
				case BST_CHECKED:
					gpSet->isFixFarBorders = 1; break;
				case BST_INDETERMINATE:
					gpSet->isFixFarBorders = 2; break;
			}

			gpConEmu->Update(true);
			break;
		case cbCursorColor:
			gpSet->isCursorColor = IsChecked(hMain,cbCursorColor);
			gpConEmu->Update(true);
			break;
		case cbCursorBlink:
			gpSet->isCursorBlink = IsChecked(hMain,cbCursorBlink);
			if (!gpSet->isCursorBlink) // если мигание отключается - то курсор может "замереть" в погашенном состоянии.
				gpConEmu->ActiveCon()->Invalidate();
			break;
		case cbMultiCon:
			gpSet->isMulti = IsChecked(hTabs, cbMultiCon);
			break;
		case cbMultiIterate:
			gpSet->isMultiIterate = IsChecked(hTabs, cbMultiIterate);
			break;
		case cbNewConfirm:
			gpSet->isMultiNewConfirm = IsChecked(hTabs, cbNewConfirm);
			break;
		case cbLongOutput:
			gpSet->AutoBufferHeight = IsChecked(hTabs, cbLongOutput);
			gpConEmu->UpdateFarSettings();
			EnableWindow(GetDlgItem(hTabs, tLongOutputHeight), gpSet->AutoBufferHeight);
			break;
		case cbBold:
		case cbItalic:
		{
			PostMessage(hMain, gpSetCls->mn_MsgRecreateFont, wParam, 0);
		}
		break;
		case cbBgImage:
		{
			gpSet->isShowBgImage = IsChecked(hMain, cbBgImage);
			EnableWindow(GetDlgItem(hMain, tBgImage), gpSet->isShowBgImage);
			//EnableWindow(GetDlgItem(hMain, tDarker), gpSet->isShowBgImage);
			//EnableWindow(GetDlgItem(hMain, slDarker), gpSet->isShowBgImage);
			EnableWindow(GetDlgItem(hMain, bBgImage), gpSet->isShowBgImage);
			//EnableWindow(GetDlgItem(hMain, rBgUpLeft), gpSet->isShowBgImage);
			//EnableWindow(GetDlgItem(hMain, rBgStretch), gpSet->isShowBgImage);
			//EnableWindow(GetDlgItem(hMain, rBgTile), gpSet->isShowBgImage);
			BOOL lbNeedLoad = (mp_Bg == NULL);

			if (gpSet->isShowBgImage && gpSet->bgImageDarker == 0)
			{
				if (MessageBox(ghOpWnd,
				              L"Background image will NOT be visible\n"
				              L"while 'Darkening' is 0. Increase it?",
				              gpConEmu->GetDefaultTitle(), MB_YESNO|MB_ICONEXCLAMATION)!=IDNO)
				{
					gpSet->bgImageDarker = 0x46;
					SendDlgItemMessage(hMain, slDarker, TBM_SETPOS  , (WPARAM) true, (LPARAM) gpSet->bgImageDarker);
					TCHAR tmp[10];
					_wsprintf(tmp, SKIPLEN(countof(tmp)) L"%i", gpSet->bgImageDarker);
					SetDlgItemText(hMain, tDarker, tmp);
					lbNeedLoad = TRUE;
				}
			}

			if (lbNeedLoad)
			{
				gpSetCls->LoadBackgroundFile(gpSet->sBgImage, true);
			}

			gpConEmu->Update(true);
		}
		break;
		case rBgUpLeft:
		case rBgStretch:
		case rBgTile:
		{
			gpSet->bgOperation = (char)(CB - rBgUpLeft);
			gpSetCls->LoadBackgroundFile(gpSet->sBgImage, true);
			gpConEmu->Update(true);
		}
		break;
		case cbBgAllowPlugin:
		{
			gpSet->isBgPluginAllowed = IsChecked(hMain, cbBgAllowPlugin);
			NeedBackgroundUpdate();
			gpConEmu->Update(true);
		}
		break;
		case cbRClick:
			gpSet->isRClickSendKey = IsChecked(hWnd2, cbRClick); //0-1-2
			break;
		case cbSafeFarClose:
			gpSet->isSafeFarClose = IsChecked(hWnd2, cbSafeFarClose);
			break;
		case cbMinToTray:
			gpSet->isMinToTray = IsChecked(hWnd2, cbMinToTray);
			break;
		case cbAlwaysShowTrayIcon:
			gpSet->isAlwaysShowTrayIcon = IsChecked(hWnd2, cbAlwaysShowTrayIcon);
			Icon.SettingsChanged();
			break;
		case cbHideCaption:
			gpSet->isHideCaption = IsChecked(hWnd2, cbHideCaption);
			break;
		case cbHideCaptionAlways:
			gpSet->mb_HideCaptionAlways = IsChecked(hWnd2, cbHideCaptionAlways);

			if (gpSet->isHideCaptionAlways())
			{
				CheckDlgButton(hWnd2, cbHideCaptionAlways, BST_CHECKED);
				TODO("показать тултип, что скрытие обязательно при прозрачности");
			}

			gpConEmu->OnHideCaption();
			break;
		//case bHideCaptionSettings:
		//	DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_MORE_HIDE), ghOpWnd, hideOpProc);
		//	break;
		//case cbConsoleTextSelection:
		//	gpSet->isConsoleTextSelection = IsChecked(hWnd2, cbConsoleTextSelection);
		//	break;
		//case bCTSSettings:
		//	DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_SPG_SELECTION), ghOpWnd, selectionOpProc);
		//	break;
		case cbFARuseASCIIsort:
			gpSet->isFARuseASCIIsort = IsChecked(hWnd2, cbFARuseASCIIsort);
			gpConEmu->UpdateFarSettings();
			break;
		case cbShellNoZoneCheck:
			gpSet->isShellNoZoneCheck = IsChecked(hWnd2, cbShellNoZoneCheck);
			gpConEmu->UpdateFarSettings();
			break;
		case cbDragPanel:
			gpSet->isDragPanel = IsChecked(hWnd2, cbDragPanel);
			gpConEmu->OnSetCursor();
			break;
		case cbTryToCenter:
			gpSet->isTryToCenter = IsChecked(hWnd2, cbTryToCenter);
			gpConEmu->OnSize(-1);
			gpConEmu->InvalidateAll();
			break;
		case cbAlwaysShowScrollbar:
			gpSet->isAlwaysShowScrollbar = IsChecked(hWnd2, cbAlwaysShowScrollbar);
			gpConEmu->OnAlwaysShowScrollbar();
			gpConEmu->OnSize(-1);
			gpConEmu->InvalidateAll();
			break;
		case cbFarHourglass:
			gpSet->isFarHourglass = IsChecked(hWnd2, cbFarHourglass);
			gpConEmu->OnSetCursor();
			break;
		case cbExtendUCharMap:
			gpSet->isExtendUCharMap = IsChecked(hWnd2, cbExtendUCharMap);
			gpConEmu->Update(true);
			break;
		case cbFixAltOnAltTab:
			gpSet->isFixAltOnAltTab = IsChecked(hWnd2, cbFixAltOnAltTab);
			break;
		case cbAutoRegFonts:
			gpSet->isAutoRegisterFonts = IsChecked(hWnd2, cbAutoRegFonts);
			break;
		case cbDebugSteps:
			gpSet->isDebugSteps = IsChecked(hWnd2, cbDebugSteps);
			break;
		case cbDragL:
		case cbDragR:
			gpSet->isDragEnabled =
			    (IsChecked(hWnd2, cbDragL) ? DRAG_L_ALLOWED : 0) |
			    (IsChecked(hWnd2, cbDragR) ? DRAG_R_ALLOWED : 0);
			break;
		case cbDropEnabled:
			gpSet->isDropEnabled = IsChecked(hWnd2, cbDropEnabled);
			break;
		case cbDnDCopy:
			gpSet->isDefCopy = IsChecked(hWnd2, cbDnDCopy) == BST_CHECKED;
			break;
		case cbDragImage:
			gpSet->isDragOverlay = IsChecked(hWnd2, cbDragImage);
			break;
		case cbDragIcons:
			gpSet->isDragShowIcons = IsChecked(hWnd2, cbDragIcons) == BST_CHECKED;
			break;
		case cbEnhanceGraphics: // Progressbars and scrollbars
			gpSet->isEnhanceGraphics = IsChecked(hWnd2, cbEnhanceGraphics);
			gpConEmu->Update(true);
			break;
		case cbEnhanceButtons: // Buttons, CheckBoxes and RadioButtons
			gpSet->isEnhanceButtons = IsChecked(hWnd2, cbEnhanceButtons);
			gpConEmu->Update(true);
			break;
		case cbTabs:

			switch(IsChecked(hTabs, cbTabs))
			{
				case BST_UNCHECKED:
					gpSet->isTabs = 0; break;
				case BST_CHECKED:
					gpSet->isTabs = 1; break;
				case BST_INDETERMINATE:
					gpSet->isTabs = 2; break;
			}

			TODO("Хорошо бы сразу видимость табов менять");
			//gpConEmu->mp_TabBar->Update(TRUE); -- это как-то неправильно работает.
			break;
		case cbTabSelf:
			gpSet->isTabSelf = IsChecked(hTabs, cbTabSelf);
			break;
		case cbTabRecent:
			gpSet->isTabRecent = IsChecked(hTabs, cbTabRecent);
			break;
		case cbTabLazy:
			gpSet->isTabLazy = IsChecked(hTabs, cbTabLazy);
			break;
		case cbTabsOnTaskBar:
			// 3state: BST_UNCHECKED/BST_CHECKED/BST_INDETERMINATE
			gpSet->m_isTabsOnTaskBar = IsChecked(hTabs, cbTabsOnTaskBar);
			gpConEmu->OnTaskbarSettingsChanged();
			break;
		case cbRSelectionFix:
			gpSet->isRSelFix = IsChecked(hWnd2, cbRSelectionFix);
			break;
		case cbSkipActivation:
			gpSet->isMouseSkipActivation = IsChecked(hWnd2, cbSkipActivation);
			break;
		case cbSkipMove:
			gpSet->isMouseSkipMoving = IsChecked(hWnd2, cbSkipMove);
			break;
		case cbMonitorConsoleLang:
			gpSet->isMonitorConsoleLang = IsChecked(hWnd2, cbMonitorConsoleLang);
			break;
		case cbSkipFocusEvents:
			gpSet->isSkipFocusEvents = IsChecked(hWnd2, cbSkipFocusEvents);
			break;
		case cbMonospace:
		{
			#ifdef _DEBUG
			BYTE cMonospaceNow = gpSet->isMonospace;
			#endif
			gpSet->isMonospace = IsChecked(hMain, cbMonospace);

			if (gpSet->isMonospace) gpSetCls->isMonospaceSelected = gpSet->isMonospace;

			mb_IgnoreEditChanged = TRUE;
			ResetFontWidth();
			gpConEmu->Update(true);
			mb_IgnoreEditChanged = FALSE;
		} break;
		case cbExtendFonts:
		{
			gpSet->isExtendFonts = IsChecked(hMain, cbExtendFonts);
			gpConEmu->Update(true);
		} break;
		//case cbAutoConHandle:
		//	isUpdConHandle = !isUpdConHandle;
		//	gpConEmu->Update(true);
		//	break;
		case rCursorH:
		case rCursorV:

			if (wParam == rCursorV)
				gpSet->isCursorV = true;
			else
				gpSet->isCursorV = false;

			gpConEmu->Update(true);
			break;
		case cbBlockInactiveCursor:
			gpSet->isCursorBlockInactive = IsChecked(hMain, cbBlockInactiveCursor);
			gpConEmu->ActiveCon()->Invalidate();
			break;
		case bBgImage:
		{
			wchar_t temp[MAX_PATH], edt[MAX_PATH];
			if (!GetDlgItemText(hMain, tBgImage, edt, MAX_PATH))
				edt[0] = 0;
			ExpandEnvironmentStrings(edt, temp, countof(temp));
			OPENFILENAME ofn; memset(&ofn,0,sizeof(ofn));
			ofn.lStructSize=sizeof(ofn);
			ofn.hwndOwner = ghOpWnd;
			ofn.lpstrFilter = L"All images (*.bmp,*.jpg,*.png)\0*.bmp;*.jpg;*.jpe;*.jpeg;*.png\0Bitmap images (*.bmp)\0*.bmp\0JPEG images (*.jpg)\0*.jpg;*.jpe;*.jpeg\0PNG images (*.png)\0*.png\0\0";
			ofn.nFilterIndex = 1;
			ofn.lpstrFile = temp;
			ofn.nMaxFile = MAX_PATH;
			ofn.lpstrTitle = L"Choose background image";
			ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
			            | OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST;

			if (GetOpenFileName(&ofn))
			{
				if (LoadBackgroundFile(temp, true))
				{
					bool bUseEnvVar = false;
					size_t nEnvLen = _tcslen(gpConEmu->ms_ConEmuExeDir);
					if (_tcslen(temp) > nEnvLen && temp[nEnvLen] == L'\\')
					{
						temp[nEnvLen] = 0;
						if (lstrcmpi(temp, gpConEmu->ms_ConEmuExeDir) == 0)
							bUseEnvVar = true;
						temp[nEnvLen] = L'\\';
					}
					if (bUseEnvVar)
					{
						wcscpy_c(gpSet->sBgImage, L"%ConEmuDir%");
						wcscat_c(gpSet->sBgImage, temp + _tcslen(gpConEmu->ms_ConEmuExeDir));
					}
					else
					{
						wcscpy_c(gpSet->sBgImage, temp);
					}
					SetDlgItemText(hMain, tBgImage, gpSet->sBgImage);
					gpConEmu->Update(true);
				}
			}
		}
		break;
		case cbVisible:
			gpSet->isConVisible = IsChecked(hWnd2, cbVisible);

			if (gpSet->isConVisible)
			{
				// Если показывать - то только текущую (иначе на экране мешанина консолей будет
				gpConEmu->ActiveCon()->RCon()->ShowConsole(gpSet->isConVisible);
			}
			else
			{
				// А если скрывать - то все сразу
				for(int i=0; i<MAX_CONSOLE_COUNT; i++)
				{
					CVirtualConsole *pCon = gpConEmu->GetVCon(i);

					if (pCon) pCon->RCon()->ShowConsole(FALSE);
				}
			}

			apiSetForegroundWindow(ghOpWnd);
			break;
			//case cbLockRealConsolePos:
			//	isLockRealConsolePos = IsChecked(hWnd2, cbLockRealConsolePos);
			//	break;
		case cbUseInjects:
			gpSet->isUseInjects = IsChecked(hWnd2, cbUseInjects);
			gpConEmu->OnGlobalSettingsChanged();
			break;
		case cbPortableRegistry:
			#ifdef USEPORTABLEREGISTRY
			gpSet->isPortableReg = IsChecked(hWnd2, cbPortableRegistry);
			// Проверить, готов ли к использованию
			if (!gpConEmu->PreparePortableReg())
			{
				gpSet->isPortableReg = false;
				CheckDlgButton(hWnd2, cbPortableRegistry, BST_UNCHECKED);
			}
			else
			{
				gpConEmu->OnGlobalSettingsChanged();
			}
			#endif
			break;
		case bRealConsoleSettings:
			EditConsoleFont(ghOpWnd);
			break;
		case cbDesktopMode:
			gpSet->isDesktopMode = IsChecked(hWnd2, cbDesktopMode);
			gpConEmu->OnDesktopMode();
			break;
		case cbAlwaysOnTop:
			gpSet->isAlwaysOnTop = IsChecked(hWnd2, cbAlwaysOnTop);
			gpConEmu->OnAlwaysOnTop();
			break;
		case cbSleepInBackground:
			gpSet->isSleepInBackground = IsChecked(hWnd2, cbSleepInBackground);
			gpConEmu->ActiveCon()->RCon()->OnGuiFocused(TRUE);
			break;
		case cbDisableFarFlashing:
			gpSet->isDisableFarFlashing = IsChecked(hWnd2, cbDisableFarFlashing);
			break;
		case cbDisableAllFlashing:
			gpSet->isDisableAllFlashing = IsChecked(hWnd2, cbDisableAllFlashing);
			break;
		case cbTabsInCaption:
			gpSet->isTabsInCaption = IsChecked(hWnd2, cbTabsInCaption);
			////RedrawWindow(ghWnd, NULL, NULL, RDW_UPDATENOW|RDW_FRAME);
			////gpConEmu->OnNcMessage(ghWnd, WM_NCPAINT, 0,0);
			//SendMessage(ghWnd, WM_NCACTIVATE, 0, 0);
			//SendMessage(ghWnd, WM_NCPAINT, 0, 0);
			gpConEmu->RedrawTabPanel();
			break;
		case rbAdminShield:
		case rbAdminSuffix:
			gpSet->bAdminShield = IsChecked(hTabs, rbAdminShield);
			gpConEmu->mp_TabBar->Update(TRUE);
			break;
		case cbHideInactiveConTabs:
			gpSet->bHideInactiveConsoleTabs = IsChecked(hTabs, cbHideInactiveConTabs);
			gpConEmu->mp_TabBar->Update(TRUE);
			break;
		case cbHideDisabledTabs:
			gpSet->bHideDisabledTabs = IsChecked(hTabs, cbHideDisabledTabs);
			gpConEmu->mp_TabBar->Update(TRUE);
			break;
		case cbShowFarWindows:
			gpSet->bShowFarWindows = IsChecked(hTabs, cbShowFarWindows);
			gpConEmu->mp_TabBar->Update(TRUE);
			break;
			
		case cbUseWinArrows:
		case cbUseWinNumber:
		case cbUseWinTab:
			switch (CB)
			{
				case cbUseWinArrows:
					gpSet->isUseWinArrows = IsChecked(hKeys, CB);
					break;
				case cbUseWinNumber:
					gpSet->isUseWinNumber = IsChecked(hKeys, CB);
					//if (hTabs) CheckDlgButton(hTabs, cbUseWinNumber, gpSet->isUseWinNumber ? BST_CHECKED : BST_UNCHECKED);
					break;
				case cbUseWinTab:
					gpSet->isUseWinTab = IsChecked(hKeys, CB);
					break;
			}
			gpConEmu->UpdateWinHookSettings();
			break;

		//case cbUseWinNumber:
		//	gpSet->isUseWinNumber = IsChecked(hTabs, cbUseWinNumber);
		//	if (hKeys) CheckDlgButton(hKeys, cbUseWinNumberK, gpSet->isUseWinNumber ? BST_CHECKED : BST_UNCHECKED);
		//	gpConEmu->UpdateWinHookSettings();
		//	break;

		case cbSendAltEnter:
			// хуков не требует
			gpSet->isSendAltEnter = IsChecked(hKeys, cbSendAltEnter);
			break;
		case cbSendAltSpace:
			// хуков не требует
			gpSet->isSendAltSpace = IsChecked(hKeys, cbSendAltSpace);
			break;
			
		case cbSendAltTab:
		case cbSendAltEsc:
		case cbSendAltPrintScrn:
		case cbSendPrintScrn:
		case cbSendCtrlEsc:
			switch (CB)
			{
				case cbSendAltTab:
					gpSet->isSendAltTab = IsChecked(hKeys, cbSendAltTab); break;
				case cbSendAltEsc:
					gpSet->isSendAltEsc = IsChecked(hKeys, cbSendAltEsc); break;
				case cbSendAltPrintScrn:
					gpSet->isSendAltPrintScrn = IsChecked(hKeys, cbSendAltPrintScrn); break;
				case cbSendPrintScrn:
					gpSet->isSendPrintScrn = IsChecked(hKeys, cbSendPrintScrn); break;
				case cbSendCtrlEsc:
					gpSet->isSendCtrlEsc = IsChecked(hKeys, cbSendCtrlEsc); break;
			}
			gpConEmu->UpdateWinHookSettings();
			break;
			
		case cbInstallKeybHooks:
			switch (IsChecked(hKeys,cbInstallKeybHooks))
			{
					// Разрешено
				case BST_CHECKED: gpSet->m_isKeyboardHooks = 1; gpConEmu->RegisterHoooks(); break;
					// Запрещено
				case BST_UNCHECKED: gpSet->m_isKeyboardHooks = 2; gpConEmu->UnRegisterHoooks(); break;
					// Запрос при старте
				case BST_INDETERMINATE: gpSet->m_isKeyboardHooks = 0; break;
			}

			break;
		case cbDosBox:
			if (gpConEmu->mb_DosBoxExists)
			{
				CheckDlgButton(hWnd2, cbDosBox, BST_CHECKED);
				EnableWindow(GetDlgItem(hWnd2, cbDosBox), FALSE); // изменение пока запрещено
			}
			else
			{
				CheckDlgButton(hWnd2, cbDosBox, BST_UNCHECKED);
				size_t nMaxCCH = MAX_PATH*3;
				wchar_t* pszErrInfo = (wchar_t*)malloc(nMaxCCH*sizeof(wchar_t));
				_wsprintf(pszErrInfo, SKIPLEN(nMaxCCH) L"DosBox is not installed!\n"
						L"\n"
						L"DosBox files must be located here:"
						L"%s\\DosBox\\"
						L"\n"
						L"1. Copy files DOSBox.exe, SDL.dll, SDL_net.dll\n"
						L"2. Create of modify configuration file DOSBox.conf",
						gpConEmu->ms_ConEmuBaseDir);
			}
			break;
		case bApplyViewSettings:
			gpConEmu->OnPanelViewSettingsChanged();
			//gpConEmu->UpdateGuiInfoMapping();
			break;
		case cbThumbLoadFiles:

			switch(IsChecked(hWnd2,CB))
			{
				case BST_CHECKED:       gpSet->ThSet.bLoadPreviews = 3; break;
				case BST_INDETERMINATE: gpSet->ThSet.bLoadPreviews = 1; break;
				default: gpSet->ThSet.bLoadPreviews = 0;
			}

			break;
		case cbThumbLoadFolders:
			gpSet->ThSet.bLoadFolders = IsChecked(hWnd2, CB);
			break;
		case cbThumbUsePicView2:
			gpSet->ThSet.bUsePicView2 = IsChecked(hWnd2, CB);
			break;
		case cbThumbRestoreOnStartup:
			gpSet->ThSet.bRestoreOnStartup = IsChecked(hWnd2, CB);
			break;
		case cbThumbPreviewBox:
			gpSet->ThSet.nPreviewFrame = IsChecked(hWnd2, CB);
			break;
		case cbThumbSelectionBox:
			gpSet->ThSet.nSelectFrame = IsChecked(hWnd2, CB);
			break;
		case rbThumbBackColorIdx: case rbThumbBackColorRGB:
			gpSet->ThSet.crBackground.UseIndex = IsChecked(hWnd2, rbThumbBackColorIdx);
			InvalidateRect(GetDlgItem(hWnd2, c32), 0, 1);
			break;
		case rbThumbPreviewBoxColorIdx: case rbThumbPreviewBoxColorRGB:
			gpSet->ThSet.crPreviewFrame.UseIndex = IsChecked(hWnd2, rbThumbPreviewBoxColorIdx);
			InvalidateRect(GetDlgItem(hWnd2, c33), 0, 1);
			break;
		case rbThumbSelectionBoxColorIdx: case rbThumbSelectionBoxColorRGB:
			gpSet->ThSet.crSelectFrame.UseIndex = IsChecked(hWnd2, rbThumbSelectionBoxColorIdx);
			InvalidateRect(GetDlgItem(hWnd2, c34), 0, 1);
			break;

		case cbActivityReset:
			{
				ListView_DeleteAllItems(GetDlgItem(hWnd2, lbActivityLog));
				//wchar_t szText[2]; szText[0] = 0;
				//HWND hDetails = GetDlgItem(hWnd2, lbActivityDetails);
				//ListView_SetItemText(hDetails, 0, 1, szText);
				//ListView_SetItemText(hDetails, 1, 1, szText);								
				SetDlgItemText(hWnd2, ebActivityApp, L"");
				SetDlgItemText(hWnd2, ebActivityParm, L"");
			} // cbActivityReset
			break;
		case cbActivitySaveAs:
			{
				gpSetCls->OnSaveActivityLogFile(GetDlgItem(hWnd2, lbActivityLog));
			} // cbActivitySaveAs
			break;
		case rbActivityDisabled:
		case rbActivityShell:
		case rbActivityInput:
		case rbActivityCmd:
			{
				//PRAGMA_ERROR("m_ActivityLoggingType");
				HWND hList = GetDlgItem(hWnd2, lbActivityLog);
				//HWND hDetails = GetDlgItem(hWnd2, lbActivityDetails);
				gpSetCls->m_ActivityLoggingType =
					(LOWORD(wParam) == rbActivityShell) ? glt_Processes :
					(LOWORD(wParam) == rbActivityInput) ? glt_Input :
					(LOWORD(wParam) == rbActivityCmd)   ? glt_Commands :
					glt_None;
				ListView_DeleteAllItems(hList);
				for (int c = 0; (c <= 40) && ListView_DeleteColumn(hList, 0); c++);
				//ListView_DeleteAllItems(hDetails);
				//for (int c = 0; (c <= 40) && ListView_DeleteColumn(hDetails, 0); c++);
				SetDlgItemText(hWnd2, ebActivityApp, L"");
				SetDlgItemText(hWnd2, ebActivityParm, L"");
				
				if (gpSetCls->m_ActivityLoggingType == glt_Processes)
				{
					LVCOLUMN col ={LVCF_WIDTH|LVCF_TEXT|LVCF_FMT, LVCFMT_LEFT, 60};
					wchar_t szTitle[64]; col.pszText = szTitle;

					ListView_SetExtendedListViewStyleEx(hList,LVS_EX_FULLROWSELECT,LVS_EX_FULLROWSELECT);
					ListView_SetExtendedListViewStyleEx(hList,LVS_EX_LABELTIP|LVS_EX_INFOTIP,LVS_EX_LABELTIP|LVS_EX_INFOTIP);
					
					wcscpy_c(szTitle, L"Time");		ListView_InsertColumn(hList, lpc_Time, &col);
					col.cx = 55; col.fmt = LVCFMT_RIGHT;
					wcscpy_c(szTitle, L"PPID");		ListView_InsertColumn(hList, lpc_PPID, &col);
					col.cx = 60; col.fmt = LVCFMT_LEFT;
					wcscpy_c(szTitle, L"Func");		ListView_InsertColumn(hList, lpc_Func, &col);
					col.cx = 50;
					wcscpy_c(szTitle, L"Oper");		ListView_InsertColumn(hList, lpc_Oper, &col);
					col.cx = 40;
					wcscpy_c(szTitle, L"Bits");		ListView_InsertColumn(hList, lpc_Bits, &col);
					wcscpy_c(szTitle, L"Syst");		ListView_InsertColumn(hList, lpc_System, &col);
					col.cx = 120;
					wcscpy_c(szTitle, L"App");		ListView_InsertColumn(hList, lpc_App, &col);
					wcscpy_c(szTitle, L"Params");	ListView_InsertColumn(hList, lpc_Params, &col);
					//wcscpy_c(szTitle, L"CurDir");	ListView_InsertColumn(hList, 7, &col);
					col.cx = 120;
					wcscpy_c(szTitle, L"Flags");	ListView_InsertColumn(hList, lpc_Flags, &col);
					col.cx = 80;
					wcscpy_c(szTitle, L"StdIn");	ListView_InsertColumn(hList, lpc_StdIn, &col);
					wcscpy_c(szTitle, L"StdOut");	ListView_InsertColumn(hList, lpc_StdOut, &col);
					wcscpy_c(szTitle, L"StdErr");	ListView_InsertColumn(hList, lpc_StdErr, &col);

				}
				else if (gpSetCls->m_ActivityLoggingType == glt_Input)
				{
					LVCOLUMN col ={LVCF_WIDTH|LVCF_TEXT|LVCF_FMT, LVCFMT_LEFT, 60};
					wchar_t szTitle[64]; col.pszText = szTitle;

					ListView_SetExtendedListViewStyleEx(hList,LVS_EX_FULLROWSELECT,LVS_EX_FULLROWSELECT);
					ListView_SetExtendedListViewStyleEx(hList,LVS_EX_LABELTIP|LVS_EX_INFOTIP,LVS_EX_LABELTIP|LVS_EX_INFOTIP);
					
					wcscpy_c(szTitle, L"Time");		ListView_InsertColumn(hList, lic_Time, &col);
					col.cx = 50;
					wcscpy_c(szTitle, L"Type");		ListView_InsertColumn(hList, lic_Type, &col);
					col.cx = 50;
					wcscpy_c(szTitle, L"##");		ListView_InsertColumn(hList, lic_Dup, &col);
					col.cx = 300;
					wcscpy_c(szTitle, L"Event");	ListView_InsertColumn(hList, lic_Event, &col);

				}
				else if (gpSetCls->m_ActivityLoggingType == glt_Commands)
				{
					mn_ActivityCmdStartTick = timeGetTime();

					LVCOLUMN col ={LVCF_WIDTH|LVCF_TEXT|LVCF_FMT, LVCFMT_LEFT, 60};
					wchar_t szTitle[64]; col.pszText = szTitle;

					ListView_SetExtendedListViewStyleEx(hList,LVS_EX_FULLROWSELECT,LVS_EX_FULLROWSELECT);
					ListView_SetExtendedListViewStyleEx(hList,LVS_EX_LABELTIP|LVS_EX_INFOTIP,LVS_EX_LABELTIP|LVS_EX_INFOTIP);
					
					col.cx = 50;
					wcscpy_c(szTitle, L"In/Out");	ListView_InsertColumn(hList, lcc_InOut, &col);
					col.cx = 70;
					wcscpy_c(szTitle, L"Time");		ListView_InsertColumn(hList, lcc_Time, &col);
					col.cx = 60;
					wcscpy_c(szTitle, L"Duration");	ListView_InsertColumn(hList, lcc_Duration, &col);
					col.cx = 50;
					wcscpy_c(szTitle, L"Cmd");		ListView_InsertColumn(hList, lcc_Command, &col);
					wcscpy_c(szTitle, L"Size");		ListView_InsertColumn(hList, lcc_Size, &col);
					wcscpy_c(szTitle, L"PID");		ListView_InsertColumn(hList, lcc_PID, &col);
					col.cx = 300;
					wcscpy_c(szTitle, L"Pipe");		ListView_InsertColumn(hList, lcc_Pipe, &col);
					wcscpy_c(szTitle, L"Extra");	ListView_InsertColumn(hList, lcc_Extra, &col);

				}
				else
				{
					LVCOLUMN col ={LVCF_WIDTH|LVCF_TEXT|LVCF_FMT, LVCFMT_LEFT, 60};
					wchar_t szTitle[4]; col.pszText = szTitle;
					wcscpy_c(szTitle, L" ");		ListView_InsertColumn(hList, 0, &col);
					//ListView_InsertColumn(hDetails, 0, &col);
				}
				ListView_DeleteAllItems(GetDlgItem(hWnd2, lbActivityLog));
				
				gpConEmu->OnGlobalSettingsChanged();
			}; // rbActivityShell
			break;

		case cbExtendColors:
			gpSet->isExtendColors = IsChecked(hColors, cbExtendColors) == BST_CHECKED ? true : false;

			for(int i=16; i<32; i++) //-V112
				EnableWindow(GetDlgItem(hColors, tc0+i), gpSet->isExtendColors);

			EnableWindow(GetDlgItem(hColors, lbExtendIdx), gpSet->isExtendColors);

			if (lParam)
			{
				gpConEmu->Update(true);
			}

			break;
		case cbTrueColorer:
			gpSet->isTrueColorer = IsChecked(hColors, cbTrueColorer);
			gpConEmu->UpdateFarSettings();
			gpConEmu->Update(true);
			break;
		case cbFadeInactive:
			gpSet->isFadeInactive = IsChecked(hColors, cbFadeInactive);
			gpConEmu->ActiveCon()->Invalidate();
			break;
		case cbTransparent:
			{
				int newV = gpSet->nTransparent;

				if (IsChecked(hColors, cbTransparent))
				{
					if (newV == 255) newV = 200;
				}
				else
				{
					newV = 255;
				}

				if (newV != gpSet->nTransparent)
				{
					gpSet->nTransparent = newV;
					SendDlgItemMessage(hColors, slTransparent, TBM_SETPOS, (WPARAM) true, (LPARAM)gpSet->nTransparent);
					gpConEmu->OnTransparent();
				}
			} break;
		case cbUserScreenTransparent:
			{
				gpSet->isUserScreenTransparent = IsChecked(hColors, cbUserScreenTransparent);

				if (hWnd2) CheckDlgButton(hWnd2, cbHideCaptionAlways, gpSet->isHideCaptionAlways() ? BST_CHECKED : BST_UNCHECKED);

				gpConEmu->OnHideCaption(); // при прозрачности - обязательно скрытие заголовка + кнопки
				gpConEmu->UpdateWindowRgn();
			} break;


		/* *** Text selections options *** */
		case rbCTSNever:
		case rbCTSAlways:
		case rbCTSBufferOnly:
			gpSet->isConsoleTextSelection = (CB==rbCTSNever) ? 0 : (CB==rbCTSAlways) ? 1 : 2;
			//CheckDlgButton(gpSetCls->hExt, cbConsoleTextSelection, gpSet->isConsoleTextSelection);
			break;
		case rbCTSActAlways: case rbCTSActBufferOnly:
			gpSet->isCTSActMode = (CB==rbCTSActAlways) ? 1 : 2;
			break;
		case cbCTSBlockSelection:
			gpSet->isCTSSelectBlock = IsChecked(hWnd2,CB);
			break;
		case cbCTSTextSelection:
			gpSet->isCTSSelectText = IsChecked(hWnd2,CB);
			break;
		case cbFarGotoEditor:
			gpSet->isFarGotoEditor = IsChecked(hWnd2,CB);
			break;
		/* *** Text selections options *** */


		/* *** Update settings *** */
		case cbUpdateCheckOnStartup:
			gpSet->UpdSet.isUpdateCheckOnStartup = IsChecked(hWnd2, CB);
			break;
		case cbUpdateCheckHourly:
			gpSet->UpdSet.isUpdateCheckHourly = IsChecked(hWnd2, CB);
			break;
		case cbUpdateConfirmDownload:
			gpSet->UpdSet.isUpdateConfirmDownload = IsChecked(hWnd2, CB);
			break;
		case rbUpdateStableOnly:
		case rbUpdateLatestAvailable:
			gpSet->UpdSet.isUpdateUseBuilds = IsChecked(hWnd2, rbUpdateStableOnly) ? 1 : 2;
			break;
		case cbUpdateUseProxy:
			gpSet->UpdSet.isUpdateUseProxy = IsChecked(hWnd2, CB);
			{
				UINT nItems[] = {stUpdateProxy, tUpdateProxy, stUpdateProxyUser, tUpdateProxyUser, stUpdateProxyPassword, tUpdateProxyPassword};
				for (size_t i = 0; i < countof(nItems); i++)
				{
					HWND hItem = GetDlgItem(hWnd2, nItems[i]);
					if (!hItem)
					{
						_ASSERTE(GetDlgItem(hWnd2, nItems[i])!=NULL);
						continue;
					}
					EnableWindow(hItem, gpSet->UpdSet.isUpdateUseProxy);
				}
			}
			break;
		case cbUpdateLeavePackages:
			gpSet->UpdSet.isUpdateLeavePackages = IsChecked(hWnd2, CB);
			break;
		case cbUpdateArcCmdLine:
			{
				wchar_t szArcExe[MAX_PATH] = {};
				OPENFILENAME ofn = {sizeof(ofn)};
				ofn.hwndOwner = ghOpWnd;
				ofn.lpstrFilter = L"7-Zip or WinRar\0WinRAR.exe;UnRAR.exe;Rar.exe;7zg.exe;7z.exe\0Exe files (*.exe)\0*.exe\0\0";
				ofn.nFilterIndex = 1;

				ofn.lpstrFile = szArcExe;
				ofn.nMaxFile = countof(szArcExe);
				ofn.lpstrTitle = L"Choose 7-Zip or WinRar location";
				ofn.lpstrDefExt = L"exe";
				ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
					| OFN_FILEMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;

				if (GetOpenFileName(&ofn))
				{
					size_t nMax = _tcslen(szArcExe)+128;
					wchar_t *pszNew = (wchar_t*)calloc(nMax,sizeof(*pszNew));
					_wsprintf(pszNew, SKIPLEN(nMax) L"\"%s\"  x -y \"%%1\"", szArcExe);
					SetDlgItemText(hWnd2, tUpdateArcCmdLine, pszNew);
					//if (gpSet->UpdSet.szUpdateArcCmdLine && lstrcmp(gpSet->UpdSet.szUpdateArcCmdLine, gpSet->UpdSet.szUpdateArcCmdLineDef) == 0)
					//	SafeFree(gpSet->UpdSet.szUpdateArcCmdLine);
					SafeFree(pszNew);
				}
			}
			break;
		case cbUpdateDownloadPath:
			{
				wchar_t szStorePath[MAX_PATH] = {};
				wchar_t szInitial[MAX_PATH+1];
				ExpandEnvironmentStrings(gpSet->UpdSet.szUpdateDownloadPath, szInitial, countof(szInitial));
				OPENFILENAME ofn = {sizeof(ofn)};
				ofn.hwndOwner = ghOpWnd;
				ofn.lpstrFilter = L"Packages\0ConEmuSetup.*.exe;ConEmu.*.7z\0\0";
				ofn.nFilterIndex = 1;
				wcscpy_c(szStorePath, L"ConEmuSetup.exe");
				ofn.lpstrFile = szStorePath;
				ofn.nMaxFile = countof(szStorePath);
				ofn.lpstrInitialDir = szInitial;
				ofn.lpstrTitle = L"Choose download path";
				ofn.lpstrDefExt = L"";
				ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
					| OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY;

				if (GetSaveFileName(&ofn))
				{
					wchar_t *pszSlash = wcsrchr(szStorePath, L'\\');
					if (pszSlash)
					{
						*pszSlash = 0;
						SetDlgItemText(hWnd2, tUpdateDownloadPath, szStorePath);
					}
				}
			}
			break;
		/* *** Update settings *** */


		default:
		{
			if (CB >= cbHostWin && CB <= cbHostRShift)
			{
				memset(gpSet->mn_HostModOk, 0, sizeof(gpSet->mn_HostModOk));

				for(UINT i = 0; i < countof(HostkeyCtrlIds); i++)
				{
					if (IsChecked(hKeys, HostkeyCtrlIds[i]))
						gpSet->CheckHostkeyModifier(HostkeyCtrlId2Vk(HostkeyCtrlIds[i]));
				}

				gpSet->TrimHostkeys();

				if (IsChecked(hKeys, CB))
				{
					gpSet->CheckHostkeyModifier(HostkeyCtrlId2Vk(CB));
					gpSet->TrimHostkeys();
				}

				// Обновить, что осталось
				gpSetCls->SetupHotkeyChecks(hKeys);
				gpSet->MakeHostkeyModifier();
			} // if (CB >= cbHostWin && CB <= cbHostRShift)
			else if (CB >= c32 && CB <= c34)
			{
				if (gpSetCls->ColorEditDialog(hWnd2, CB))
				{
					if (CB == c32)
					{
						gpSet->ThSet.crBackground.UseIndex = 0;
						CheckRadioButton(hWnd2, rbThumbBackColorIdx, rbThumbBackColorRGB, rbThumbBackColorRGB);
					}
					else if (CB == c33)
					{
						gpSet->ThSet.crPreviewFrame.UseIndex = 0;
						CheckRadioButton(hWnd2, rbThumbPreviewBoxColorIdx, rbThumbPreviewBoxColorRGB, rbThumbPreviewBoxColorRGB);
					}
					else if (CB == c34)
					{
						gpSet->ThSet.crSelectFrame.UseIndex = 0;
						CheckRadioButton(hWnd2, rbThumbSelectionBoxColorIdx, rbThumbSelectionBoxColorRGB, rbThumbSelectionBoxColorRGB);
					}

					InvalidateRect(GetDlgItem(hWnd2, CB), 0, 1);
					// done
				}
			} // else if (CB >= c32 && CB <= c34)
			else if (CB >= c0 && CB <= MAX_COLOR_EDT_ID)
			{
				if (ColorEditDialog(hColors, CB))
				{
					//gpConEmu->m_Back->Refresh();
					gpConEmu->Update(true);
				}
			} // else if (CB >= c0 && CB <= MAX_COLOR_EDT_ID)

		} // default:
	}

	return 0;
}

//LRESULT CSettings::OnColorButtonClicked(HWND hWnd2, WPARAM wParam, LPARAM lParam)
//{
//	WORD CB = LOWORD(wParam);
//
//	switch(wParam)
//	{
//		case cbExtendColors:
//			gpSet->isExtendColors = IsChecked(hColors, cbExtendColors) == BST_CHECKED ? true : false;
//
//			for(int i=16; i<32; i++) //-V112
//				EnableWindow(GetDlgItem(hColors, tc0+i), gpSet->isExtendColors);
//
//			EnableWindow(GetDlgItem(hColors, lbExtendIdx), gpSet->isExtendColors);
//
//			if (lParam)
//			{
//				gpConEmu->Update(true);
//			}
//
//			break;
//		case cbTrueColorer:
//			gpSet->isTrueColorer = IsChecked(hColors, cbTrueColorer);
//			gpConEmu->UpdateFarSettings();
//			gpConEmu->Update(true);
//			break;
//		case cbFadeInactive:
//			gpSet->isFadeInactive = IsChecked(hColors, cbFadeInactive);
//			gpConEmu->ActiveCon()->Invalidate();
//			break;
//		case cbTransparent:
//		{
//			int newV = gpSet->nTransparent;
//
//			if (IsChecked(hColors, cbTransparent))
//			{
//				if (newV == 255) newV = 200;
//			}
//			else
//			{
//				newV = 255;
//			}
//
//			if (newV != gpSet->nTransparent)
//			{
//				gpSet->nTransparent = newV;
//				SendDlgItemMessage(hColors, slTransparent, TBM_SETPOS, (WPARAM) true, (LPARAM)gpSet->nTransparent);
//				gpConEmu->OnTransparent();
//			}
//		} break;
//		case cbUserScreenTransparent:
//		{
//			gpSet->isUserScreenTransparent = IsChecked(hColors, cbUserScreenTransparent);
//
//			if (hWnd2) CheckDlgButton(hWnd2, cbHideCaptionAlways, gpSet->isHideCaptionAlways() ? BST_CHECKED : BST_UNCHECKED);
//
//			gpConEmu->OnHideCaption(); // при прозрачности - обязательно скрытие заголовка + кнопки
//			gpConEmu->UpdateWindowRgn();
//		} break;
//		default:
//
//			if (CB >= c0 && CB <= MAX_COLOR_EDT_ID)
//			{
//				if (ColorEditDialog(hColors, CB))
//				{
//					gpConEmu->m_Back->Refresh();
//					gpConEmu->Update(true);
//				}
//			}
//	}
//
//	return 0;
//}

BOOL CSettings::GetColorRef(HWND hDlg, WORD TB, COLORREF* pCR)
{
	BOOL result = FALSE;
	int r, g, b;
	wchar_t temp[MAX_PATH];
	GetDlgItemText(hDlg, TB, temp, MAX_PATH);
	TCHAR *sp1 = wcschr(temp, ' '), *sp2;

	if (sp1 && *(sp1+1) && *(sp1+1) != ' ')
	{
		sp2 = wcschr(sp1+1, ' ');

		if (sp2 && *(sp2+1) && *(sp2+1) != ' ')
		{
			*sp1 = 0;
			sp1++;
			*sp2 = 0;
			sp2++;
			r = klatoi(temp); g = klatoi(sp1), b = klatoi(sp2);

			if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255 && *pCR != RGB(r, g, b))
			{
				*pCR = RGB(r, g, b);
				result = TRUE;
				//gpConEmu->Update(true);
				//InvalidateRect(GetDlgItem(hColors, TB - (tc0-c0)), 0, 1);
			}
		}
	}

	return result;
}

//LRESULT CSettings::OnColorEditChanged(HWND hWnd2, WPARAM wParam, LPARAM lParam)
//{
//	WORD TB = LOWORD(wParam);
//	COLORREF color = 0;
//
//	if (GetColorById(TB - (tc0-c0), &color))
//	{
//		if (GetColorRef(hColors, TB, &color))
//		{
//			if (SetColorById(TB - (tc0-c0), color))
//			{
//				gpConEmu->InvalidateAll();
//
//				if (TB >= tc0 && TB <= tc31)
//					gpConEmu->Update(true);
//
//				InvalidateRect(GetDlgItem(hColors, TB - (tc0-c0)), 0, 1);
//			}
//		}
//	}
//	else if (TB == tFadeLow || TB == tFadeHigh)
//	{
//		BOOL lbOk = FALSE;
//		UINT nVal = GetDlgItemInt(hColors, TB, &lbOk, FALSE);
//
//		if (lbOk && nVal <= 255)
//		{
//			if (TB == tFadeLow)
//				gpSet->mn_FadeLow = nVal;
//			else
//				gpSet->mn_FadeHigh = nVal;
//
//			gpSet->mb_FadeInitialized = false;
//			gpSet->mn_LastFadeSrc = gpSet->mn_LastFadeDst = -1;
//		}
//	}
//
//	return 0;
//}

LRESULT CSettings::OnEditChanged(HWND hWnd2, WPARAM wParam, LPARAM lParam)
{
	if (mb_IgnoreEditChanged)
		return 0;

	WORD TB = LOWORD(wParam);

	switch (TB)
	{
	case tBgImage:
	{
		wchar_t temp[MAX_PATH];
		GetDlgItemText(hMain, tBgImage, temp, MAX_PATH);

		if (wcscmp(temp, gpSet->sBgImage))
		{
			if (LoadBackgroundFile(temp, true))
			{
				wcscpy_c(gpSet->sBgImage, temp);
				gpConEmu->Update(true);
			}
		}
		break;
	} // case tBgImage:
	
	case tBgImageColors:
	{
		wchar_t temp[128] = {0};
		GetDlgItemText(hMain, tBgImageColors, temp, countof(temp)-1);
		DWORD newBgColors = 0;

		for (wchar_t* pc = temp; *pc; pc++)
		{
			if (*pc == L'*')
			{
				newBgColors = (DWORD)-1;
				break;
			}

			if (*pc == L'#')
			{
				if (isDigit(pc[1]))
				{
					pc++;
					// Получить индекс цвета (0..15)
					int nIdx = *pc - L'0';

					if (nIdx == 1 && isDigit(pc[1]))
					{
						pc++;
						nIdx = nIdx*10 + (*pc - L'0');
					}

					if (nIdx >= 0 && nIdx <= 15)
					{
						newBgColors |= (1 << nIdx);
					}
				}
			}
		}

		// Если таки изменлся - обновим
		if (newBgColors && gpSet->nBgImageColors != newBgColors)
		{
			gpSet->nBgImageColors = newBgColors;
			gpConEmu->Update(true);
		}
		
		break;
	} // case tBgImageColors:

	case tWndWidth:
	case tWndHeight:
	{
		if (IsChecked(hMain, rNormal) == BST_CHECKED)
			EnableWindow(GetDlgItem(hMain, cbApplyPos), TRUE);
		break;
	} // case tWndWidth: case tWndHeight:
	
	case tDarker:
	{
		DWORD newV;
		TCHAR tmp[10];
		GetDlgItemText(hMain, tDarker, tmp, 10);
		newV = klatoi(tmp);

		if (newV < 256 && newV != gpSet->bgImageDarker)
		{
			gpSet->bgImageDarker = newV;
			SendDlgItemMessage(hMain, slDarker, TBM_SETPOS, (WPARAM) true, (LPARAM) gpSet->bgImageDarker);

			// Картинку может установить и плагин
			if (gpSet->isShowBgImage && gpSet->sBgImage[0])
				LoadBackgroundFile(gpSet->sBgImage);
			else
				NeedBackgroundUpdate();

			gpConEmu->Update(true);
		}
		break;
	} //case tDarker:
	
	case tLongOutputHeight:
	{
		BOOL lbOk = FALSE;
		wchar_t szTemp[16];
		UINT nNewVal = GetDlgItemInt(hTabs, tLongOutputHeight, &lbOk, FALSE);

		if (lbOk)
		{
			if (nNewVal >= 300 && nNewVal <= 9999)
				gpSet->DefaultBufferHeight = nNewVal;
			else if (nNewVal > 9999)
				SetDlgItemText(hTabs, TB, _ltow(gpSet->DefaultBufferHeight, szTemp, 10));
		}
		else
		{
			SetDlgItemText(hTabs, TB, _ltow(gpSet->DefaultBufferHeight, szTemp, 10));
		}
		break;
	} //case tLongOutputHeight:
	
	//case hkNewConsole:
	//case hkSwitchConsole:
	//case hkCloseConsole:
	//{
	//	UINT nHotKey = 0xFF & SendDlgItemMessage(hTabs, TB, HKM_GETHOTKEY, 0, 0);

	//	if (TB == hkNewConsole)
	//		gpSet->icMultiNew = nHotKey;
	//	else if (TB == hkSwitchConsole)
	//		gpSet->icMultiNext = nHotKey;
	//	else if (TB == hkCloseConsole)
	//		gpSet->icMultiRecreate = nHotKey;

	//	// SendDlgItemMessage(hTabs, hkMinimizeRestore, HKM_SETHOTKEY, gpSet->icMinimizeRestore, 0);
	//	break;
	//} // case hkNewConsole: case hkSwitchConsole: case hkCloseConsole:

	case hkHotKeySelect:
	{
		UINT nHotKey = 0xFF & SendDlgItemMessage(hWnd2, TB, HKM_GETHOTKEY, 0, 0);
		bool bList = false;
		for (size_t i = 0; i < countof(SettingsNS::nKeysHot); i++)
		{
			if (SettingsNS::nKeysHot[i] == nHotKey)
			{
				SendDlgItemMessage(hWnd2, lbHotKeyList, CB_SETCURSEL, i, 0);
				bList = true;
				break;
			}
		}
		if (!bList)
			SendDlgItemMessage(hWnd2, lbHotKeyList, CB_SETCURSEL, 0, 0);
		if (mp_ActiveHotKey && (mp_ActiveHotKey->Type == 0) && mp_ActiveHotKey->VkPtr)
		{
			*mp_ActiveHotKey->VkPtr = (BYTE)nHotKey;
			FillHotKeysList(FALSE);
		}
		break;
	} // case hkHotKeySelect:
		
	case tTabConsole:
	case tTabViewer:
	case tTabEditor:
	case tTabEditorMod:
	{
		wchar_t temp[MAX_PATH]; temp[0] = 0;

		if (GetDlgItemText(hTabs, TB, temp, MAX_PATH) && temp[0])
		{
			temp[31] = 0; // страховка

			if (wcsstr(temp, L"%s"))
			{
				if (TB == tTabConsole)
					wcscpy_c(gpSet->szTabConsole, temp);
				else if (TB == tTabViewer)
					wcscpy_c(gpSet->szTabViewer, temp);
				else if (TB == tTabEditor)
					wcscpy_c(gpSet->szTabEditor, temp);
				else if (tTabEditorMod)
					wcscpy_c(gpSet->szTabEditorModified, temp);

				gpConEmu->mp_TabBar->Update(TRUE);
			}
		}
		break;
	} // case tTabConsole: case tTabViewer: case tTabEditor: case tTabEditorMod:
	
	case tTabLenMax:
	{
		BOOL lbOk = FALSE;
		DWORD n = GetDlgItemInt(hTabs, tTabLenMax, &lbOk, FALSE);

		if (n > 10 && n < CONEMUTABMAX)
		{
			gpSet->nTabLenMax = n;
			gpConEmu->mp_TabBar->Update(TRUE);
		}
		break;
	} // case tTabLenMax:
	
	case tAdminSuffix:
	{
		GetDlgItemText(hTabs, tAdminSuffix, gpSet->szAdminTitleSuffix, countof(gpSet->szAdminTitleSuffix));
		gpConEmu->mp_TabBar->Update(TRUE);
		break;
	} // case tAdminSuffix:
	
	case tRClickMacro:
	{
		GetString(hWnd2, tRClickMacro, &gpSet->sRClickMacro, gpSet->RClickMacroDefault());
		break;
	} // case tRClickMacro:
	
	case tSafeFarCloseMacro:
	{
		GetString(hWnd2, tSafeFarCloseMacro, &gpSet->sSafeFarCloseMacro, gpSet->SafeFarCloseMacroDefault());
		break;
	} // case tSafeFarCloseMacro:
	
	case tCloseTabMacro:
	{
		GetString(hWnd2, tCloseTabMacro, &gpSet->sTabCloseMacro, gpSet->TabCloseMacroDefault());
		break;
	} // case tCloseTabMacro:
	
	case tSaveAllMacro:
	{
		GetString(hWnd2, tSaveAllMacro, &gpSet->sSaveAllMacro, gpSet->SaveAllMacroDefault());
		break;
	} // case tSaveAllMacro:

	
	/* *** Update settings *** */
	case tUpdateProxy:
		GetString(hWnd2, TB, &gpSet->UpdSet.szUpdateProxy);
		break;
	case tUpdateProxyUser:
		GetString(hWnd2, TB, &gpSet->UpdSet.szUpdateProxyUser);
		break;
	case tUpdateProxyPassword:
		GetString(hWnd2, TB, &gpSet->UpdSet.szUpdateProxyPassword);
		break;
	case tUpdateDownloadPath:
		GetString(hWnd2, TB, &gpSet->UpdSet.szUpdateDownloadPath);
		break;
	case tUpdateExeCmdLine:
		GetString(hWnd2, TB, &gpSet->UpdSet.szUpdateExeCmdLine, gpSet->UpdSet.szUpdateExeCmdLineDef);
		break;
	case tUpdateArcCmdLine:
		GetString(hWnd2, TB, &gpSet->UpdSet.szUpdateArcCmdLine, gpSet->UpdSet.szUpdateArcCmdLineDef);
		break;
	case tUpdatePostUpdateCmd:
		GetString(hWnd2, TB, &gpSet->UpdSet.szUpdatePostUpdateCmd);
		break;
	/* *** Update settings *** */
	
	default:
	
		if (hWnd2 == hViews)
		{
			BOOL bValOk = FALSE;
			UINT nVal = GetDlgItemInt(hWnd2, TB, &bValOk, FALSE);

			if (bValOk)
			{
				switch(TB)
				{
					case tThumbLoadingTimeout:
						gpSet->ThSet.nLoadTimeout = nVal; break;
						//
					case tThumbsImgSize:
						gpSet->ThSet.Thumbs.nImgSize = nVal; break;
						//
					case tThumbsX1:
						gpSet->ThSet.Thumbs.nSpaceX1 = nVal; break;
					case tThumbsY1:
						gpSet->ThSet.Thumbs.nSpaceY1 = nVal; break;
					case tThumbsX2:
						gpSet->ThSet.Thumbs.nSpaceX2 = nVal; break;
					case tThumbsY2:
						gpSet->ThSet.Thumbs.nSpaceY2 = nVal; break;
						//
					case tThumbsSpacing:
						gpSet->ThSet.Thumbs.nLabelSpacing = nVal; break;
					case tThumbsPadding:
						gpSet->ThSet.Thumbs.nLabelPadding = nVal; break;
						// ****************
					case tTilesImgSize:
						gpSet->ThSet.Tiles.nImgSize = nVal; break;
						//
					case tTilesX1:
						gpSet->ThSet.Tiles.nSpaceX1 = nVal; break;
					case tTilesY1:
						gpSet->ThSet.Tiles.nSpaceY1 = nVal; break;
					case tTilesX2:
						gpSet->ThSet.Tiles.nSpaceX2 = nVal; break;
					case tTilesY2:
						gpSet->ThSet.Tiles.nSpaceY2 = nVal; break;
						//
					case tTilesSpacing:
						gpSet->ThSet.Tiles.nLabelSpacing = nVal; break;
					case tTilesPadding:
						gpSet->ThSet.Tiles.nLabelPadding = nVal; break;
				}
			}
			
			if (TB >= tc32 && TB <= tc34)
			{
				COLORREF color = 0;

				if (gpSetCls->GetColorById(TB - (tc0-c0), &color))
				{
					if (gpSetCls->GetColorRef(hWnd2, TB, &color))
					{
						if (gpSetCls->SetColorById(TB - (tc0-c0), color))
						{
							InvalidateRect(GetDlgItem(hWnd2, TB - (tc0-c0)), 0, 1);
							// done
						}
					}
				}
			}
		} // if (hWnd2 == hViews)
		else if (hWnd2 == hColors)
		{
			COLORREF color = 0;

			if (TB == tFadeLow || TB == tFadeHigh)
			{
				BOOL lbOk = FALSE;
				UINT nVal = GetDlgItemInt(hColors, TB, &lbOk, FALSE);

				if (lbOk && nVal <= 255)
				{
					if (TB == tFadeLow)
						gpSet->mn_FadeLow = nVal;
					else
						gpSet->mn_FadeHigh = nVal;

					gpSet->mb_FadeInitialized = false;
					gpSet->mn_LastFadeSrc = gpSet->mn_LastFadeDst = -1;
				}
			}
			else if (GetColorById(TB - (tc0-c0), &color))
			{
				if (GetColorRef(hColors, TB, &color))
				{
					if (SetColorById(TB - (tc0-c0), color))
					{
						gpConEmu->InvalidateAll();

						if (TB >= tc0 && TB <= tc31)
							gpConEmu->Update(true);

						InvalidateRect(GetDlgItem(hColors, TB - (tc0-c0)), 0, 1);
					}
				}
			}
		} // else if (hWnd2 == hColors)
		else if (hWnd2 == hExt)
		{
			if (HIWORD(wParam) == EN_CHANGE)
			{
				WORD TB = LOWORD(wParam);
				BOOL lbOk = FALSE;
				UINT nNewVal = GetDlgItemInt(hWnd2, TB, &lbOk, FALSE);

				if (lbOk)
				{
					switch(TB)
					{
					case tHideCaptionAlwaysFrame:
						gpSet->nHideCaptionAlwaysFrame = nNewVal;
						gpConEmu->UpdateWindowRgn();
						break;
					case tHideCaptionAlwaysDelay:
						gpSet->nHideCaptionAlwaysDelay = nNewVal;
						break;
					case tHideCaptionAlwaysDissapear:
						gpSet->nHideCaptionAlwaysDisappear = nNewVal;
						break;
					}
				}
			}
		} // else if (hWnd2 == hExt)
		
	// end of default:
	} // switch (TB)

	return 0;
}

//LRESULT CSettings::OnColorComboBox(HWND hWnd2, WPARAM wParam, LPARAM lParam)
//{
//	WORD wId = LOWORD(wParam);
//
//	if (wId == lbExtendIdx)
//	{
//		gpSet->nExtendColor = SendDlgItemMessage(hColors, wId, CB_GETCURSEL, 0, 0);
//	}
//	else if (wId==lbDefaultColors)
//	{
//		if (gbLastColorsOk)  // только если инициализация палитр завершилась
//		{
//			const DWORD* pdwDefData = NULL;
//			wchar_t temp[32];
//			INT_PTR nIdx = SendDlgItemMessage(hColors, lbDefaultColors, CB_GETCURSEL, 0, 0);
//
//			if (nIdx == 0)
//				pdwDefData = gdwLastColors;
//			else if (nIdx >= 1 && nIdx <= (INT_PTR)countof(DefColors))
//				pdwDefData = DefColors[nIdx-1].dwDefColors;
//			else
//				return 0; // неизвестный набор
//
//			uint nCount = countof(DefColors->dwDefColors);
//
//			for(uint i = 0; i < nCount; i++)
//			{
//				gpSet->Colors[i] = pdwDefData[i]; //-V108
//				_wsprintf(temp, SKIPLEN(countof(temp)) L"%i %i %i", getR(gpSet->Colors[i]), getG(gpSet->Colors[i]), getB(gpSet->Colors[i]));
//				SetDlgItemText(hColors, 1100 + i, temp);
//				InvalidateRect(GetDlgItem(hColors, c0+i), 0, 1);
//			}
//		}
//		else return 0;
//	}
//
//	gpConEmu->Update(true);
//	return 0;
//}

LRESULT CSettings::OnComboBox(HWND hWnd2, WPARAM wParam, LPARAM lParam)
{
	WORD wId = LOWORD(wParam);
	
	switch (wId)
	{
	case tFontCharset:
	{
		gpSet->mb_CharSetWasSet = TRUE;
		PostMessage(hMain, gpSetCls->mn_MsgRecreateFont, wParam, 0);
		break;
	}

	case tFontFace:
	case tFontFace2:
	case tFontSizeY:
	case tFontSizeX:
	case tFontSizeX2:
	case tFontSizeX3:
	{
		if (HIWORD(wParam) == CBN_SELCHANGE)
			PostMessage(hMain, mn_MsgRecreateFont, wId, 0);
		else
			mn_LastChangingFontCtrlId = wId;
		break;
	}

	case lbLDragKey:
	{
		GetListBoxByte(hWnd2,wId,SettingsNS::szKeys,SettingsNS::nKeys,gpSet->nLDragKey);
		break;
	}
	case lbRDragKey:
	{
		GetListBoxByte(hWnd2,wId,SettingsNS::szKeys,SettingsNS::nKeys,gpSet->nRDragKey);
		break;
	}
	case lbNtvdmHeight:
	{
		INT_PTR num = SendDlgItemMessage(hTabs, wId, CB_GETCURSEL, 0, 0);
		gpSet->ntvdmHeight = (num == 1) ? 25 : ((num == 2) ? 28 : ((num == 3) ? 43 : ((num == 4) ? 50 : 0))); //-V112
		break;
	}
	case lbCmdOutputCP:
	{
		gpSet->nCmdOutputCP = SendDlgItemMessage(hTabs, wId, CB_GETCURSEL, 0, 0);

		if (gpSet->nCmdOutputCP == -1) gpSet->nCmdOutputCP = 0;

		gpConEmu->UpdateFarSettings();
		break;
	}
	case lbExtendFontNormalIdx:
	case lbExtendFontBoldIdx:
	case lbExtendFontItalicIdx:
	{
		if (wId == lbExtendFontNormalIdx)
			gpSet->nFontNormalColor = GetNumber(hMain, wId);
		else if (wId == lbExtendFontBoldIdx)
			gpSet->nFontBoldColor = GetNumber(hMain, wId);
		else if (wId == lbExtendFontItalicIdx)
			gpSet->nFontItalicColor = GetNumber(hMain, wId);

		if (gpSet->isExtendFonts)
			gpConEmu->Update(true);
		break;
	}

	case lbHotKeyList:
	{
		if (!mp_ActiveHotKey)
			break;

		BYTE vk = 0;

		if (mp_ActiveHotKey && (mp_ActiveHotKey->Type == 0))
		{
			GetListBoxByte(hWnd2, wId, SettingsNS::szKeysHot, SettingsNS::nKeysHot, vk);
			SendDlgItemMessage(hWnd2, hkHotKeySelect, HKM_SETHOTKEY, vk|(vk==VK_DELETE ? (HOTKEYF_EXT<<8) : 0), 0);
		}
		else
		{
			GetListBoxByte(hWnd2, wId, SettingsNS::szKeys, SettingsNS::nKeys, vk);
			SendDlgItemMessage(hWnd2, hkHotKeySelect, HKM_SETHOTKEY, 0, 0);
		}

		if (mp_ActiveHotKey && (mp_ActiveHotKey->Type == 0 || mp_ActiveHotKey->Type == 1) && mp_ActiveHotKey->VkPtr)
		{
			*mp_ActiveHotKey->VkPtr = vk;
			FillHotKeysList(FALSE);
		}

		break;
	}

	case tTabFontFace:
	case tTabFontHeight:
	case tTabFontCharset:
	{
		if (HIWORD(wParam) == CBN_EDITCHANGE)
		{
			switch(wId)
			{
			case tTabFontFace:
				GetDlgItemText(hWnd2, wId, gpSet->sTabFontFace, countof(gpSet->sTabFontFace)); break;
			case tTabFontHeight:
				gpSet->nTabFontHeight = GetNumber(hWnd2, wId); break;
			}
		}
		else if (HIWORD(wParam) == CBN_SELCHANGE)
		{
			INT_PTR nSel = SendDlgItemMessage(hWnd2, wId, CB_GETCURSEL, 0, 0);

			switch(wId)
			{
			case tTabFontFace:
				SendDlgItemMessage(hWnd2, wId, CB_GETLBTEXT, nSel, (LPARAM)gpSet->sTabFontFace);
				break;
			case tTabFontHeight:
				if (nSel >= 0 && nSel < (INT_PTR)countof(SettingsNS::FSizesSmall))
					gpSet->nTabFontHeight = SettingsNS::FSizesSmall[nSel];
				break;
			case tTabFontCharset:
				if (nSel >= 0 && nSel < (INT_PTR)countof(SettingsNS::nCharSets))
					gpSet->nTabFontCharSet = SettingsNS::nCharSets[nSel];
				else
					gpSet->nTabFontCharSet = DEFAULT_CHARSET;
			}
		}
		gpConEmu->mp_TabBar->UpdateTabFont();
		break;
	}


	default:
		if (hWnd2 == gpSetCls->hViews)
		{
			if (HIWORD(wParam) == CBN_EDITCHANGE)
			{
				switch(wId)
				{
					case tThumbsFontName:
						GetDlgItemText(hWnd2, wId, gpSet->ThSet.Thumbs.sFontName, countof(gpSet->ThSet.Thumbs.sFontName)); break;
					case tThumbsFontSize:
						gpSet->ThSet.Thumbs.nFontHeight = GetNumber(hWnd2, wId); break;
					case tTilesFontName:
						GetDlgItemText(hWnd2, wId, gpSet->ThSet.Tiles.sFontName, countof(gpSet->ThSet.Tiles.sFontName)); break;
					case tTilesFontSize:
						gpSet->ThSet.Tiles.nFontHeight = GetNumber(hWnd2, wId); break;
				}
			}
			else if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				INT_PTR nSel = SendDlgItemMessage(hWnd2, wId, CB_GETCURSEL, 0, 0);

				switch(wId)
				{
					case lbThumbBackColorIdx:
						gpSet->ThSet.crBackground.ColorIdx = nSel;
						InvalidateRect(GetDlgItem(hWnd2, c32), 0, 1);
						break;
					case lbThumbPreviewBoxColorIdx:
						gpSet->ThSet.crPreviewFrame.ColorIdx = nSel;
						InvalidateRect(GetDlgItem(hWnd2, c33), 0, 1);
						break;
					case lbThumbSelectionBoxColorIdx:
						gpSet->ThSet.crSelectFrame.ColorIdx = nSel;
						InvalidateRect(GetDlgItem(hWnd2, c34), 0, 1);
						break;
					case tThumbsFontName:
						SendDlgItemMessage(hWnd2, wId, CB_GETLBTEXT, nSel, (LPARAM)gpSet->ThSet.Thumbs.sFontName);
						break;
					case tThumbsFontSize:

						if (nSel>=0 && nSel<(INT_PTR)countof(SettingsNS::FSizesSmall))
							gpSet->ThSet.Thumbs.nFontHeight = SettingsNS::FSizesSmall[nSel];

						break;
					case tTilesFontName:
						SendDlgItemMessage(hWnd2, wId, CB_GETLBTEXT, nSel, (LPARAM)gpSet->ThSet.Tiles.sFontName);
						break;
					case tTilesFontSize:

						if (nSel>=0 && nSel<(INT_PTR)countof(SettingsNS::FSizesSmall))
							gpSet->ThSet.Tiles.nFontHeight = SettingsNS::FSizesSmall[nSel];

						break;
					case tThumbMaxZoom:
						gpSet->ThSet.nMaxZoom = max(100,((nSel+1)*100));
				}
			}
		} // else if (hWnd2 == gpSetCls->hViews)
		else if (hWnd2 == hColors)
		{
			if (wId == lbExtendIdx)
			{
				gpSet->nExtendColor = SendDlgItemMessage(hColors, wId, CB_GETCURSEL, 0, 0);
			}
			else if (wId==lbDefaultColors)
			{
				if (gbLastColorsOk)  // только если инициализация палитр завершилась
				{
					const DWORD* pdwDefData = NULL;
					wchar_t temp[32];
					INT_PTR nIdx = SendDlgItemMessage(hColors, lbDefaultColors, CB_GETCURSEL, 0, 0);

					if (nIdx == 0)
						pdwDefData = gdwLastColors;
					else if (nIdx >= 1 && nIdx <= (INT_PTR)countof(DefColors))
						pdwDefData = DefColors[nIdx-1].dwDefColors;
					else
						return 0; // неизвестный набор

					uint nCount = countof(DefColors->dwDefColors);

					for(uint i = 0; i < nCount; i++)
					{
						gpSet->Colors[i] = pdwDefData[i]; //-V108
						_wsprintf(temp, SKIPLEN(countof(temp)) L"%i %i %i", getR(gpSet->Colors[i]), getG(gpSet->Colors[i]), getB(gpSet->Colors[i]));
						SetDlgItemText(hColors, 1100 + i, temp);
						InvalidateRect(GetDlgItem(hColors, c0+i), 0, 1);
					}
				}
				else return 0;
			}

			gpConEmu->Update(true);
		} // else if (hWnd2 == hColors)
		else if (hWnd2 == hSelection)
		{
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				switch(wId)
				{
				case lbCTSBlockSelection:
					{
						GetListBoxByte(hWnd2, lbCTSBlockSelection, SettingsNS::szKeys, SettingsNS::nKeys, gpSet->isCTSVkBlock);
					} break;
				case lbCTSTextSelection:
					{
						GetListBoxByte(hWnd2, lbCTSTextSelection, SettingsNS::szKeys, SettingsNS::nKeys, gpSet->isCTSVkText);
					} break;
				case lbCTSActAlways:
					{
						GetListBoxByte(hWnd2, lbCTSActAlways, SettingsNS::szKeysAct, SettingsNS::nKeysAct, gpSet->isCTSVkAct);
					} break;
				case lbCTSRBtnAction:
					{
						GetListBoxByte(hWnd2, lbCTSRBtnAction, SettingsNS::szClipAct, SettingsNS::nClipAct, gpSet->isCTSRBtnAction);
					} break;
				case lbCTSMBtnAction:
					{
						GetListBoxByte(hWnd2, lbCTSMBtnAction, SettingsNS::szClipAct, SettingsNS::nClipAct, gpSet->isCTSMBtnAction);
					} break;
				case lbCTSForeIdx:
					{
						DWORD nFore = 0;
						GetListBoxItem(GetDlgItem(hWnd2, lbCTSForeIdx), countof(SettingsNS::szColorIdx)-1,
							SettingsNS::szColorIdx, SettingsNS::nColorIdx, nFore);
						gpSet->isCTSColorIndex = (gpSet->isCTSColorIndex & 0xF0) | (nFore & 0xF);
						gpConEmu->Update(true);
					} break;
				case lbCTSBackIdx:
					{
						DWORD nBack = 0;
						GetListBoxItem(GetDlgItem(hWnd2, lbCTSBackIdx), countof(SettingsNS::szColorIdx)-1,
							SettingsNS::szColorIdx, SettingsNS::nColorIdx, nBack);
						gpSet->isCTSColorIndex = (gpSet->isCTSColorIndex & 0xF) | ((nBack & 0xF) << 4);
						gpConEmu->Update(true);
					} break;
				case lbFarGotoEditorVk:
					{
						GetListBoxByte(hWnd2, lbFarGotoEditorVk, SettingsNS::szKeysAct, SettingsNS::nKeysAct, gpSet->isFarGotoEditorVk);
					} break;
				}
			} // if (HIWORD(wParam) == CBN_SELCHANGE)
		} // else if (hWnd2 == hSelection)
	} // switch (wId)
	return 0;
}

LRESULT CSettings::OnPage(LPNMHDR phdr)
{
	if (gpSetCls->szFontError[0])
	{
		gpSetCls->szFontError[0] = 0;
		SendMessage(hwndBalloon, TTM_TRACKACTIVATE, FALSE, (LPARAM)&tiBalloon);
		SendMessage(hwndTip, TTM_ACTIVATE, TRUE, 0);
	}

	if ((LONG_PTR)phdr == 0x101)
	{
		// Переключиться на следующий таб
		#if 1
		HWND hTree = GetDlgItem(ghOpWnd, tvSetupCategories);
		NMTREEVIEW nm = {{hTree, tvSetupCategories, TVN_SELCHANGED}};
		nm.itemOld.hItem = TreeView_GetSelection(hTree);
		if (!nm.itemOld.hItem)
			nm.itemOld.hItem = TreeView_GetRoot(hTree);
		nm.itemNew.hItem = TreeView_GetNextSibling(hTree, nm.itemOld.hItem);
		if (!nm.itemNew.hItem)
			nm.itemNew.hItem = TreeView_GetRoot(hTree);
		//return OnPage((LPNMHDR)&nm);
		if (nm.itemNew.hItem)
			TreeView_SelectItem(hTree, nm.itemNew.hItem);
		return 0;
		#else
		int nCur = SendDlgItemMessage(ghOpWnd, tabMain, TCM_GETCURSEL,0,0);
		int nAll = SendDlgItemMessage(ghOpWnd, tabMain, TCM_GETITEMCOUNT,0,0);

		nCur ++; if (nCur>=nAll) nCur = 0;

		SendDlgItemMessage(ghOpWnd, tabMain, TCM_SETCURSEL,nCur,0);
		NMHDR hdr = {GetDlgItem(ghOpWnd, tabMain),tabMain,TCN_SELCHANGE};
		return OnPage(&hdr);
		#endif
	}
	else if ((LONG_PTR)phdr == 0x102)
	{
		#if 1
		HWND hTree = GetDlgItem(ghOpWnd, tvSetupCategories);
		NMTREEVIEW nm = {{hTree, tvSetupCategories, TVN_SELCHANGED}};
		nm.itemOld.hItem = TreeView_GetSelection(hTree);
		if (!nm.itemOld.hItem)
			nm.itemOld.hItem = TreeView_GetLastVisible(hTree);
		nm.itemNew.hItem = TreeView_GetPrevSibling(hTree, nm.itemOld.hItem);
		if (!nm.itemNew.hItem)
			nm.itemNew.hItem = TreeView_GetRoot(hTree);
		//return OnPage((LPNMHDR)&nm);
		if (nm.itemNew.hItem)
			TreeView_SelectItem(hTree, nm.itemNew.hItem);
		return 0;
		#else
		// Переключиться на предыдущий таб
		int nCur = SendDlgItemMessage(ghOpWnd, tabMain, TCM_GETCURSEL,0,0);
		int nAll = SendDlgItemMessage(ghOpWnd, tabMain, TCM_GETITEMCOUNT,0,0);

		nCur --; if (nCur<0) nCur = nAll - 1;

		SendDlgItemMessage(ghOpWnd, tabMain, TCM_SETCURSEL,nCur,0);
		NMHDR hdr = {GetDlgItem(ghOpWnd, tabMain),tabMain,TCN_SELCHANGE};
		return OnPage(&hdr);
		#endif
	}


	switch (phdr->code)
	{
		case TVN_SELCHANGED:
		{
			LPNMTREEVIEW p = (LPNMTREEVIEW)phdr;
			HWND hTree = GetDlgItem(ghOpWnd, tvSetupCategories);
			if (p->itemOld.hItem && p->itemOld.hItem != p->itemNew.hItem)
				TreeView_SetItemState(hTree, p->itemOld.hItem, 0, TVIS_BOLD);
			if (p->itemNew.hItem)
				TreeView_SetItemState(hTree, p->itemNew.hItem, TVIS_BOLD, TVIS_BOLD);

			if (mb_IgnoreSelPage)
				return 0;
			HWND hCurrent = NULL;
			for (size_t i = 0; m_Pages[i].PageID; i++)
			{
				if (p->itemNew.hItem == m_Pages[i].hTI)
				{
					if (*m_Pages[i].hPage == NULL)
					{
						SetCursor(LoadCursor(NULL,IDC_WAIT));
						HWND hPlace = GetDlgItem(ghOpWnd, tSetupPagePlace);
						RECT rcClient; GetWindowRect(hPlace, &rcClient);
						MapWindowPoints(NULL, ghOpWnd, (LPPOINT)&rcClient, 2);
						*m_Pages[i].hPage = CreateDialogParam((HINSTANCE)GetModuleHandle(NULL),
							MAKEINTRESOURCE(m_Pages[i].PageID), ghOpWnd, pageOpProc, (LPARAM)&(m_Pages[i]));
						MoveWindow(*m_Pages[i].hPage, rcClient.left, rcClient.top, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top, 0);
					}
					else
					{
						SendMessage(*m_Pages[i].hPage, mn_ActivateTabMsg, 1, (LPARAM)&(m_Pages[i]));
					}
					ShowWindow(*m_Pages[i].hPage, SW_SHOW);
				}
				else if (p->itemOld.hItem == m_Pages[i].hTI)
				{
					hCurrent = *m_Pages[i].hPage;
				}
			}
			if (hCurrent)
				ShowWindow(hCurrent, SW_HIDE);
		} // TVN_SELCHANGED
		break;
	}

	return 0;
}

void CSettings::Dialog()
{
	SetCursor(LoadCursor(NULL,IDC_WAIT));

	// Сначала обновить DC, чтобы некрасивостей не было
	gpConEmu->UpdateWindowChild(NULL);

	//2009-05-03. DialogBox создает МОДАЛЬНЫЙ диалог, а нам нужен НЕмодальный
	HWND hOpt = CreateDialog(g_hInstance, MAKEINTRESOURCE(IDD_SETTINGS), NULL, wndOpProc);

	if (!hOpt)
	{
		DisplayLastError(L"Can't create settings dialog!");
	}
	else
	{
		apiShowWindow(hOpt, SW_SHOWNORMAL);
		SetFocus(hOpt);
	}
}

void CSettings::OnClose()
{
	if (gpSet->isTabs==1) gpConEmu->ForceShowTabs(TRUE); else if (gpSet->isTabs==0) gpConEmu->ForceShowTabs(FALSE); else

		gpConEmu->mp_TabBar->Update();

	gpConEmu->OnPanelViewSettingsChanged();
	//gpConEmu->UpdateGuiInfoMapping();
	gpConEmu->RegisterMinRestore(gpSet->icMinimizeRestore != 0);

	if (gpSet->m_isKeyboardHooks == 1)
		gpConEmu->RegisterHoooks();
	else if (gpSet->m_isKeyboardHooks == 2)
		gpConEmu->UnRegisterHoooks();
}

void CSettings::OnResetOrReload(BOOL abResetSettings)
{
	BOOL lbWasPos = FALSE;
	RECT rcWnd = {};
	int nSel = -1;
	
	int nBtn = MessageBox(ghOpWnd,
		abResetSettings ? L"Confirm reset settings to defaults" : L"Confirm reload settings from xml/registry",
		gpConEmu->GetDefaultTitle(), MB_YESNO|MB_ICONEXCLAMATION|MB_DEFBUTTON2);
	if (nBtn != IDYES)
		return;
	
	if (ghOpWnd && IsWindow(ghOpWnd))
	{
		lbWasPos = TRUE;
		nSel = TabCtrl_GetCurSel(GetDlgItem(ghOpWnd, tabMain));
		GetWindowRect(ghOpWnd, &rcWnd);
		DestroyWindow(ghOpWnd);
	}
	_ASSERTE(ghOpWnd == NULL);
	
	if (abResetSettings)
		gpSet->InitSettings();
	else
		gpSet->LoadSettings();
	
	RecreateFont(0);
	
	if (lbWasPos && !ghOpWnd)
	{
		Dialog();
		TabCtrl_SetCurSel(GetDlgItem(ghOpWnd, tabMain), nSel);
	}
}

// DlgProc для окна настроек (IDD_SETTINGS)
INT_PTR CSettings::wndOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch(messg)
	{
		case WM_INITDIALOG:
			ghOpWnd = hWnd2;
			SendMessage(hWnd2, WM_SETICON, ICON_BIG, (LPARAM)hClassIcon);
			SendMessage(hWnd2, WM_SETICON, ICON_SMALL, (LPARAM)hClassIconSm);
#ifdef _DEBUG

			//if (IsDebuggerPresent())
			if (!gpSet->isAlwaysOnTop)
				SetWindowPos(ghOpWnd, HWND_NOTOPMOST, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE);

#endif
			SetClassLongPtr(hWnd2, GCLP_HICON, (LONG_PTR)hClassIcon);
			gpSetCls->OnInitDialog();
			break;
		case WM_SYSCOMMAND:

			if (LOWORD(wParam) == ID_ALWAYSONTOP)
			{
				BOOL lbOnTopNow = GetWindowLong(ghOpWnd, GWL_EXSTYLE) & WS_EX_TOPMOST;
				SetWindowPos(ghOpWnd, lbOnTopNow ? HWND_NOTOPMOST : HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
				CheckMenuItem(GetSystemMenu(ghOpWnd, FALSE), ID_ALWAYSONTOP, MF_BYCOMMAND |
				              (lbOnTopNow ? MF_UNCHECKED : MF_CHECKED));
				SetWindowLongPtr(hWnd2, DWLP_MSGRESULT, 0);
				return 1;
			}

			break;
		//case WM_GETICON:

		//	if (wParam==ICON_BIG)
		//	{
		//		#ifdef _DEBUG
		//		ICONINFO inf = {0}; BITMAP bi = {0};
		//		if (GetIconInfo(hClassIcon, &inf))
		//		{
		//			GetObject(inf.hbmColor, sizeof(bi), &bi);
		//			if (inf.hbmMask) DeleteObject(inf.hbmMask);
		//			if (inf.hbmColor) DeleteObject(inf.hbmColor);
		//		}
		//		#endif
		//		SetWindowLongPtr(hWnd2, DWL_MSGRESULT, (LRESULT)hClassIcon);
		//		return 1;
		//	}
		//	else
		//	{
		//		#ifdef _DEBUG
		//		ICONINFO inf = {0}; BITMAP bi = {0};
		//		if (GetIconInfo(hClassIconSm, &inf))
		//		{
		//			GetObject(inf.hbmColor, sizeof(bi), &bi);
		//			if (inf.hbmMask) DeleteObject(inf.hbmMask);
		//			if (inf.hbmColor) DeleteObject(inf.hbmColor);
		//		}
		//		#endif
		//		SetWindowLongPtr(hWnd2, DWLP_MSGRESULT, (LRESULT)hClassIconSm);
		//		return 1;
		//	}

		//	return 0;
		case WM_COMMAND:

			if (HIWORD(wParam) == BN_CLICKED)
			{
				if (LOWORD(wParam) == bResetSettings || LOWORD(wParam) == bReloadSettings)
				{
					gpSetCls->OnResetOrReload(LOWORD(wParam) == bResetSettings);
				}
				else
				{
					gpSetCls->OnButtonClicked(hWnd2, wParam, lParam);
				}
			}
			else if (HIWORD(wParam) == EN_CHANGE)
			{
				gpSetCls->OnEditChanged(hWnd2, wParam, lParam);
			}
			else if (HIWORD(wParam) == CBN_EDITCHANGE || HIWORD(wParam) == CBN_SELCHANGE)
			{
				gpSetCls->OnComboBox(hWnd2, wParam, lParam);
			}

			break;
		case WM_NOTIFY:
		{
			LPNMHDR phdr = (LPNMHDR)lParam;

			switch (phdr->idFrom)
			{
			#if 0
			case tabMain:
			#else
			case tvSetupCategories:
			#endif
				gpSetCls->OnPage(phdr);
				break;
			}
		} break;
		case WM_CLOSE:
		{
			gpSetCls->OnClose();
			DestroyWindow(hWnd2);
		} break;
		case WM_DESTROY:
			gpSetCls->UnregisterTabs();

			if (gpSetCls->hwndTip) {DestroyWindow(gpSetCls->hwndTip); gpSetCls->hwndTip = NULL;}

			if (gpSetCls->hwndBalloon) {DestroyWindow(gpSetCls->hwndBalloon); gpSetCls->hwndBalloon = NULL;}

			if (gpSetCls->mh_CtlColorBrush) { DeleteObject(gpSetCls->mh_CtlColorBrush); gpSetCls->mh_CtlColorBrush = NULL; }

			//EndDialog(hWnd2, TRUE);
			ghOpWnd = NULL;
			gpSetCls->hMain = gpSetCls->hExt = gpSetCls->hFar = gpSetCls->hKeys = gpSetCls->hTabs = gpSetCls->hColors = NULL;
			gpSetCls->hViews = gpSetCls->hInfo = gpSetCls->hDebug = gpSetCls->hUpdate = gpSetCls->hSelection = NULL;
			gpSetCls->mp_ActiveHotKey = NULL;
			gbLastColorsOk = FALSE;
			break;
		case WM_HOTKEY:

			if (wParam == 0x101)
			{
				// Переключиться на следующий таб
				gpSetCls->OnPage((LPNMHDR)wParam);
			}
			else if (wParam == 0x102)
			{
				// Переключиться на предыдущий таб
				gpSetCls->OnPage((LPNMHDR)wParam);
			}

		case WM_ACTIVATE:

			if (LOWORD(wParam) != 0)
				gpSetCls->RegisterTabs();
			else
				gpSetCls->UnregisterTabs();

			break;
		default:
			return 0;
	}

	return 0;
}

INT_PTR CSettings::OnMeasureFontItem(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	DWORD wID = wParam;

	if (wID == tFontFace || wID == tFontFace2 || wID == tThumbsFontName || wID == tTilesFontName || wID == tTabFontFace)
	{
		MEASUREITEMSTRUCT *pItem = (MEASUREITEMSTRUCT*)lParam;
		pItem->itemHeight = 15; //pItem->itemHeight;
	}

	return TRUE;
}

INT_PTR CSettings::OnDrawFontItem(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	DWORD wID = wParam;

	if (wID == tFontFace || wID == tFontFace2 || wID == tThumbsFontName || wID == tTilesFontName || wID == tTabFontFace)
	{
		DRAWITEMSTRUCT *pItem = (DRAWITEMSTRUCT*)lParam;
		wchar_t szText[128]; szText[0] = 0;
		SendDlgItemMessage(hWnd2, wID, CB_GETLBTEXT, pItem->itemID, (LPARAM)szText);
		DWORD bAlmostMonospace = (DWORD)SendDlgItemMessage(hWnd2, wID, CB_GETITEMDATA, pItem->itemID, 0);
		COLORREF crText, crBack;

		if (!(pItem->itemState & ODS_SELECTED))
		{
			crText = GetSysColor(COLOR_WINDOWTEXT);
			crBack = GetSysColor(COLOR_WINDOW);
		}
		else
		{
			crText = GetSysColor(COLOR_HIGHLIGHTTEXT);
			crBack = GetSysColor(COLOR_HIGHLIGHT);
		}

		SetTextColor(pItem->hDC, crText);
		SetBkColor(pItem->hDC, crBack);
		RECT rc = pItem->rcItem;
		HBRUSH hBr = CreateSolidBrush(crBack);
		FillRect(pItem->hDC, &rc, hBr);
		DeleteObject(hBr);
		rc.left++;
		HFONT hFont = CreateFont(8, 0,0,0,(bAlmostMonospace==1)?FW_BOLD:FW_NORMAL,0,0,0,
		                         ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH,
		                         L"MS Sans Serif");
		HFONT hOldF = (HFONT)SelectObject(pItem->hDC, hFont);
		DrawText(pItem->hDC, szText, _tcslen(szText), &rc, DT_LEFT|DT_VCENTER|DT_NOPREFIX);
		SelectObject(pItem->hDC, hOldF);
		DeleteObject(hFont);
	}

	return TRUE;
}

// Общая DlgProc на _все_ вкладки
INT_PTR CSettings::pageOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	static bool bSkipSelChange = false;

	if (messg == WM_INITDIALOG)
	{
		//_ASSERTE(gpSetCls->hMain==NULL || gpSetCls->hMain==hWnd2);
		_ASSERTE(((ConEmuSetupPages*)lParam)->hPage!=NULL && *((ConEmuSetupPages*)lParam)->hPage==NULL && ((ConEmuSetupPages*)lParam)->PageID!=0);
		*((ConEmuSetupPages*)lParam)->hPage = hWnd2;
		switch (((ConEmuSetupPages*)lParam)->PageID)
		{
		case IDD_SPG_MAIN:
			gpSetCls->OnInitDialog_Main(hWnd2);
			break;
		case IDD_SPG_FEATURE:
			gpSetCls->OnInitDialog_Ext(hWnd2);
			break;
		case IDD_SPG_SELECTION:
			gpSetCls->OnInitDialog_Selection(hWnd2);
			break;
		case IDD_SPG_FEATURE_FAR:
			gpSetCls->OnInitDialog_Far(hWnd2, TRUE);
			break;
		case IDD_SPG_KEYS:
			bSkipSelChange = true;
			gpSetCls->OnInitDialog_Keys(hWnd2, TRUE);
			bSkipSelChange = false;
			break;
		case IDD_SPG_TABS:
			gpSetCls->OnInitDialog_Tabs(hWnd2);
			break;
		case IDD_SPG_COLORS:
			gpSetCls->OnInitDialog_Color(hWnd2);
			break;
		case IDD_SPG_VIEWS:
			gpSetCls->OnInitDialog_Views(hWnd2);
			break;
		case IDD_SPG_DEBUG:
			gpSetCls->OnInitDialog_Debug(hWnd2);
			break;
		case IDD_SPG_UPDATE:
			gpSetCls->OnInitDialog_Update(hWnd2);
			break;
		case IDD_SPG_INFO:
			gpSetCls->OnInitDialog_Info(hWnd2);
			break;

		default:
			// Чтобы не забыть добавить вызов инициализации
			_ASSERTE(((ConEmuSetupPages*)lParam)->PageID==IDD_SPG_MAIN);
		}
	}
	else if (messg == gpSetCls->mn_ActivateTabMsg)
	{
		_ASSERTE(((ConEmuSetupPages*)lParam)->hPage!=NULL && *((ConEmuSetupPages*)lParam)->hPage==hWnd2 && ((ConEmuSetupPages*)lParam)->PageID!=0);
		// Здесь можно обновить контролы страничек при активации вкладки
		switch (((ConEmuSetupPages*)lParam)->PageID)
		{
		case IDD_SPG_MAIN:    /*gpSetCls->OnInitDialog_Main(hWnd2);*/   break;
		case IDD_SPG_FEATURE: /*gpSetCls->OnInitDialog_Ext(hWnd2);*/    break;
		case IDD_SPG_SELECTION: /*gpSetCls->OnInitDialog_Selection(hWnd2);*/    break;
		case IDD_SPG_FEATURE_FAR:
			gpSetCls->OnInitDialog_Far(hWnd2, FALSE);
			break;
		case IDD_SPG_KEYS:
			bSkipSelChange = true;
			gpSetCls->OnInitDialog_Keys(hWnd2, FALSE);
			bSkipSelChange = false;
			break;
		case IDD_SPG_TABS:    /*gpSetCls->OnInitDialog_Tabs(hWnd2);*/   break;
		case IDD_SPG_COLORS:  /*gpSetCls->OnInitDialog_Color(hWnd2);*/  break;
		case IDD_SPG_VIEWS:   /*gpSetCls->OnInitDialog_Views(hWnd2);*/  break;
		case IDD_SPG_DEBUG:   /*gpSetCls->OnInitDialog_Debug(hWnd2);*/  break;
		case IDD_SPG_UPDATE:  /*gpSetCls->OnInitDialog_Update(hWnd2);*/ break;
		case IDD_SPG_INFO:    /*gpSetCls->OnInitDialog_Info(hWnd2);*/   break;

		default:
			// Чтобы не забыть добавить вызов инициализации
			_ASSERTE(((ConEmuSetupPages*)lParam)->PageID==IDD_SPG_MAIN);
		}
	}
	else
	// All other messages
	switch (messg)
	{
		case WM_INITDIALOG:
			break;
		case WM_COMMAND:
		{
			if (HIWORD(wParam) == BN_CLICKED)
			{
				gpSetCls->OnButtonClicked(hWnd2, wParam, lParam);
			}
			else if (HIWORD(wParam) == EN_CHANGE)
			{
				gpSetCls->OnEditChanged(hWnd2, wParam, lParam);
			}
			else if (HIWORD(wParam) == CBN_EDITCHANGE || HIWORD(wParam) == CBN_SELCHANGE)
			{
				gpSetCls->OnComboBox(hWnd2, wParam, lParam);
			}
			else if (HIWORD(wParam) == CBN_KILLFOCUS && gpSetCls->mn_LastChangingFontCtrlId && LOWORD(wParam) == gpSetCls->mn_LastChangingFontCtrlId)
			{
				PostMessage(gpSetCls->hMain, gpSetCls->mn_MsgRecreateFont, gpSetCls->mn_LastChangingFontCtrlId, 0);
				gpSetCls->mn_LastChangingFontCtrlId = 0;
			}
		}
		break;
		case WM_MEASUREITEM:
			return gpSetCls->OnMeasureFontItem(hWnd2, messg, wParam, lParam);
		case WM_DRAWITEM:
			return gpSetCls->OnDrawFontItem(hWnd2, messg, wParam, lParam);
		case WM_CTLCOLORSTATIC:
		{
			WORD wID = GetDlgCtrlID((HWND)lParam);

			if ((wID >= c0 && wID <= MAX_COLOR_EDT_ID) || (wID >= c32 && wID <= c34))
				return gpSetCls->ColorCtlStatic(hWnd2, wID, (HWND)lParam);

			return 0;
		}
		case WM_HSCROLL:
		{
			if (gpSetCls->hMain && (HWND)lParam == GetDlgItem(gpSetCls->hMain, slDarker))
			{
				int newV = SendDlgItemMessage(hWnd2, slDarker, TBM_GETPOS, 0, 0);

				if (newV != gpSet->bgImageDarker)
				{
					gpSet->bgImageDarker = newV;
					TCHAR tmp[10];
					_wsprintf(tmp, SKIPLEN(countof(tmp)) L"%i", gpSet->bgImageDarker);
					SetDlgItemText(hWnd2, tDarker, tmp);

					// Картинку может установить и плагин
					if (gpSet->isShowBgImage && gpSet->sBgImage[0])
						gpSetCls->LoadBackgroundFile(gpSet->sBgImage);
					else
						gpSetCls->NeedBackgroundUpdate();

					gpConEmu->Update(true);
				}
			}
			else if (gpSetCls->hColors && (HWND)lParam == GetDlgItem(gpSetCls->hColors, slTransparent))
			{
				int newV = SendDlgItemMessage(hWnd2, slTransparent, TBM_GETPOS, 0, 0);

				if (newV != gpSet->nTransparent)
				{
					CheckDlgButton(gpSetCls->hColors, cbTransparent, (newV!=255) ? BST_CHECKED : BST_UNCHECKED);
					gpSet->nTransparent = newV;
					gpConEmu->OnTransparent();
				}
			}
		}
		break;
		
		case WM_NOTIFY:
		{
			switch (((NMHDR*)lParam)->idFrom)
			{
			case lbActivityLog:
				if (!bSkipSelChange)
					return gpSetCls->OnActivityLogNotify(hWnd2, wParam, lParam);
				break;
			case lbConEmuHotKeys:
				if (!bSkipSelChange)
					return gpSetCls->OnHotkeysNotify(hWnd2, wParam, lParam);
				break;
			}
			return 0;
		} // WM_NOTIFY
		break;
		
		case WM_TIMER:

			if (wParam == FAILED_FONT_TIMERID)
			{
				KillTimer(gpSetCls->hMain, FAILED_FONT_TIMERID);
				SendMessage(gpSetCls->hwndBalloon, TTM_TRACKACTIVATE, FALSE, (LPARAM)&gpSetCls->tiBalloon);
				SendMessage(gpSetCls->hwndTip, TTM_ACTIVATE, TRUE, 0);
			}

		default:
		{
			if (messg == gpSetCls->mn_MsgRecreateFont)
			{
				gpSetCls->RecreateFont(wParam);
			}
			else if (messg == gpSetCls->mn_MsgLoadFontFromMain)
			{
				if (hWnd2 == gpSetCls->hViews)
					gpSetCls->OnInitDialog_CopyFonts(hWnd2, tThumbsFontName, tTilesFontName, 0);
				else if (hWnd2 == gpSetCls->hTabs)
					gpSetCls->OnInitDialog_CopyFonts(hWnd2, tTabFontFace, 0);
					
			}
			else if (messg == gpSetCls->mn_MsgUpdateCounter)
			{
				wchar_t sTemp[64];

				if (gpSetCls->mn_Freq!=0)
				{
					i64 v = 0;

					if (wParam == (tPerfFPS-tPerfFPS) || wParam == (tPerfInterval-tPerfFPS))
					{
						i64 *pFPS = NULL;
						UINT nCount = 0;

						if (wParam == (tPerfFPS-tPerfFPS))
						{
							pFPS = gpSetCls->mn_FPS; nCount = countof(gpSetCls->mn_FPS);
						}
						else
						{
							pFPS = gpSetCls->mn_RFPS; nCount = countof(gpSetCls->mn_RFPS);
						}

						i64 tmin = 0, tmax = 0;
						tmin = pFPS[0];

						for(UINT i = 0; i < nCount; i++)
						{
							if (pFPS[i] < tmin) tmin = pFPS[i]; //-V108

							if (pFPS[i] > tmax) tmax = pFPS[i]; //-V108
						}

						if (tmax > tmin)
							v = ((__int64)200)* gpSetCls->mn_Freq / (tmax - tmin);
					}
					else
					{
						v = (10000*(__int64)gpSetCls->mn_CounterMax[wParam])/gpSetCls->mn_Freq;
					}

					// WinApi не умеет float/double
					_wsprintf(sTemp, SKIPLEN(countof(sTemp)) L"%u.%u", (int)(v/10), (int)(v%10));
					SetDlgItemText(gpSetCls->hInfo, wParam+tPerfFPS, sTemp); //-V107
				} // if (gpSetCls->mn_Freq!=0)
			} // if (messg == gpSetCls->mn_MsgUpdateCounter)
			else if (messg == DBGMSG_LOG_ID && hWnd2 == gpSetCls->hDebug)
			{
				if (wParam == DBGMSG_LOG_SHELL_MAGIC)
				{
					bSkipSelChange = true;
					DebugLogShellActivity *pShl = (DebugLogShellActivity*)lParam;
					gpSetCls->debugLogShell(hWnd2, pShl);
					bSkipSelChange = false;
				} // DBGMSG_LOG_SHELL_MAGIC
				else if (wParam == DBGMSG_LOG_INPUT_MAGIC)
				{
					bSkipSelChange = true;
					CESERVER_REQ_PEEKREADINFO* pInfo = (CESERVER_REQ_PEEKREADINFO*)lParam;
					gpSetCls->debugLogInfo(hWnd2, pInfo);
					bSkipSelChange = false;
				} // DBGMSG_LOG_INPUT_MAGIC
				else if (wParam == DBGMSG_LOG_CMD_MAGIC)
				{
					bSkipSelChange = true;
					LogCommandsData* pCmd = (LogCommandsData*)lParam;
					gpSetCls->debugLogCommand(hWnd2, pCmd);
					bSkipSelChange = false;
				} // DBGMSG_LOG_CMD_MAGIC
			} // if (messg == DBGMSG_LOG_ID && hWnd2 == gpSetCls->hDebug)
		} // default:
	}

	return 0;
}

void CSettings::debugLogShell(HWND hWnd2, DebugLogShellActivity *pShl)
{
	SYSTEMTIME st; GetLocalTime(&st);
	wchar_t szTime[128]; _wsprintf(szTime, SKIPLEN(countof(szTime)) L"%02i:%02i:%02i", st.wHour, st.wMinute, st.wSecond);
	HWND hList = GetDlgItem(hWnd2, lbActivityLog);
	//HWND hDetails = GetDlgItem(hWnd2, lbActivityDetails);
	LVITEM lvi = {LVIF_TEXT|LVIF_STATE};
	lvi.state = lvi.stateMask = LVIS_SELECTED|LVIS_FOCUSED;
	lvi.pszText = szTime;
	int nItem = ListView_InsertItem(hList, &lvi);
	//
	ListView_SetItemText(hList, nItem, lpc_Func, (wchar_t*)pShl->szFunction);
	_wsprintf(szTime, SKIPLEN(countof(szTime)) L"%u:%u", pShl->nParentPID, pShl->nParentBits);
	ListView_SetItemText(hList, nItem, lpc_PPID, szTime);
	if (pShl->pszAction)
		ListView_SetItemText(hList, nItem, lpc_Oper, (wchar_t*)pShl->pszAction);
	if (pShl->nImageBits)
	{
		_wsprintf(szTime, SKIPLEN(countof(szTime)) L"%u", pShl->nImageBits);
		ListView_SetItemText(hList, nItem, lpc_Bits, szTime);
	}
	if (pShl->nImageSubsystem)
	{
		_wsprintf(szTime, SKIPLEN(countof(szTime)) L"%u", pShl->nImageSubsystem);
		ListView_SetItemText(hList, nItem, lpc_System, szTime);
	}

	wchar_t* pszParamEx = lstrdup(pShl->pszParam);
	LPCWSTR pszAppFile = NULL;
	if (pShl->pszFile)
	{
		ListView_SetItemText(hList, nItem, lpc_App, pShl->pszFile);
		LPCWSTR pszExt = PointToExt(pShl->pszFile);
		if (pszExt && (!lstrcmpi(pszExt, L".bat") || !lstrcmpi(pszExt, L".cmd")))
			debugLogShellText(pszParamEx, (pszAppFile = pShl->pszFile));
	}
	SetDlgItemText(hWnd2, ebActivityApp, (wchar_t*)(pShl->pszFile ? pShl->pszFile : L""));

	if (pShl->pszParam && *pShl->pszParam)
	{
		LPCWSTR pszNext = pShl->pszParam;
		wchar_t szArg[MAX_PATH+1];
		while (0 == NextArg(&pszNext, szArg))
		{
			if (!*szArg || (*szArg == L'-') || (*szArg == L'/'))
				continue;
			LPCWSTR pszExt = PointToExt(szArg);
			TODO("наверное еще и *.tmp файлы подхватить, вроде они при компиляции ресурсов в VC гоняются");
			if (pszExt && (!lstrcmpi(pszExt, L".bat") || !lstrcmpi(pszExt, L".cmd") /*|| !lstrcmpi(pszExt, L".tmp")*/)
				&& (!pszAppFile || (lstrcmpi(szArg, pszAppFile) != 0)))
			{
				debugLogShellText(pszParamEx, szArg);
			}
			else if (szArg[0] == L'@' && szArg[2] == L':' && szArg[3] == L'\\')
			{
				debugLogShellText(pszParamEx, szArg+1);
			}
		}
	}
	if (pszParamEx)
		ListView_SetItemText(hList, nItem, lpc_Params, pszParamEx);
	SetDlgItemText(hWnd2, ebActivityParm, (wchar_t*)(pszParamEx ? pszParamEx : L""));
	if (pszParamEx && pszParamEx != pShl->pszParam)
		free(pszParamEx);

	//TODO: CurDir?

	szTime[0] = 0;
	if (pShl->nShellFlags)
		_wsprintf(szTime+_tcslen(szTime), SKIPLEN(32) L"Sh:0x%04X ", pShl->nShellFlags); //-V112
	if (pShl->nCreateFlags)
		_wsprintf(szTime+_tcslen(szTime), SKIPLEN(32) L"Cr:0x%04X ", pShl->nCreateFlags); //-V112
	if (pShl->nStartFlags)
		_wsprintf(szTime+_tcslen(szTime), SKIPLEN(32) L"St:0x%04X ", pShl->nStartFlags); //-V112
	if (pShl->nShowCmd)
		_wsprintf(szTime+_tcslen(szTime), SKIPLEN(32) L"Sw:%u ", pShl->nShowCmd); //-V112
	ListView_SetItemText(hList, nItem, lpc_Flags, szTime);

	if (pShl->hStdIn)
	{
		_wsprintf(szTime, SKIPLEN(countof(szTime)) L"0x%08X", pShl->hStdIn);
		ListView_SetItemText(hList, nItem, lpc_StdIn, szTime);
	}
	if (pShl->hStdOut)
	{
		_wsprintf(szTime, SKIPLEN(countof(szTime)) L"0x%08X", pShl->hStdOut);
		ListView_SetItemText(hList, nItem, lpc_StdOut, szTime);
	}
	if (pShl->hStdErr)
	{
		_wsprintf(szTime, SKIPLEN(countof(szTime)) L"0x%08X", pShl->hStdErr);
		ListView_SetItemText(hList, nItem, lpc_StdErr, szTime);
	}
	if (pShl->pszAction)
		free(pShl->pszAction);
	if (pShl->pszFile)
		free(pShl->pszFile);
	if (pShl->pszParam)
		free(pShl->pszParam);
	free(pShl);
}

void CSettings::debugLogShellText(wchar_t* &pszParamEx, LPCWSTR asFile)
{
	_ASSERTE(pszParamEx!=NULL && asFile && *asFile);

	HANDLE hFile = CreateFile(asFile, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER liSize = {};
	char szBuf[32770]; DWORD nRead = 0;
	if (hFile && hFile != INVALID_HANDLE_VALUE
		&& GetFileSizeEx(hFile, &liSize) && (liSize.QuadPart < 0xFFFFFF)
		&& ReadFile(hFile, szBuf, sizeof(szBuf)-3, &nRead, NULL) && nRead)
	{
		szBuf[nRead] = 0; szBuf[nRead+1] = 0; szBuf[nRead+2] = 0;
		CloseHandle(hFile);
		bool lbUnicode = false;
		LPCWSTR pszExt = PointToExt(asFile);
		// Для расширений помимо ".bat" и ".cmd" - проверить содержимое
		if (lstrcmpi(pszExt, L".bat")!=0 && lstrcmpi(pszExt, L".cmd")!=0)
		{
			// например, ".tmp" файлы
			for (UINT i = 0; i < nRead; i++)
			{
				if (szBuf[i] == 0)
				{
					TODO("Может файл просто юникодный?");
					return;
				}
			}
		}

		size_t nAll = (lstrlen(pszParamEx)+20) + nRead + 1 + 2*lstrlen(asFile);
		wchar_t* pszNew = (wchar_t*)realloc(pszParamEx, nAll*sizeof(wchar_t));
		if (pszNew)
		{
			_wcscat_c(pszNew, nAll, L"\r\n\r\n>>>");
			_wcscat_c(pszNew, nAll, asFile);
			_wcscat_c(pszNew, nAll, L"\r\n");
			if (lbUnicode)
				_wcscat_c(pszNew, nAll, (wchar_t*)szBuf);
			else
				MultiByteToWideChar(CP_OEMCP, 0, szBuf, nRead+1, pszNew+lstrlen(pszNew), nRead+1);
			_wcscat_c(pszNew, nAll, L"\r\n<<<");
			_wcscat_c(pszNew, nAll, asFile);
			pszParamEx = pszNew;
		}
	}
}

void CSettings::debugLogInfo(HWND hWnd2, CESERVER_REQ_PEEKREADINFO* pInfo)
{
	for (UINT nIdx = 0; nIdx < pInfo->nCount; nIdx++)
	{
		const INPUT_RECORD *pr = pInfo->Buffer+nIdx;
		SYSTEMTIME st; GetLocalTime(&st);
		wchar_t szTime[255]; _wsprintf(szTime, SKIPLEN(countof(szTime)) L"%02i:%02i:%02i", st.wHour, st.wMinute, st.wSecond);
		HWND hList = GetDlgItem(hWnd2, lbActivityLog);
		LVITEM lvi = {LVIF_TEXT|LVIF_STATE};
		lvi.state = lvi.stateMask = LVIS_SELECTED|LVIS_FOCUSED;
		lvi.pszText = szTime;
		static INPUT_RECORD LastLogEvent1; static char LastLogEventType1; static UINT LastLogEventDup1;
		static INPUT_RECORD LastLogEvent2; static char LastLogEventType2; static UINT LastLogEventDup2;
		if (LastLogEventType1 == pInfo->cPeekRead &&
			memcmp(&LastLogEvent1, pr, sizeof(LastLogEvent1)) == 0)
		{
			LastLogEventDup1 ++;
			_wsprintf(szTime, SKIPLEN(countof(szTime)) L"%u", LastLogEventDup1);
			ListView_SetItemText(hList, 0, lic_Dup, szTime); // верхний
			//free(pr);
			continue; // дубли - не показывать? только если прошло время?
		}
		if (LastLogEventType2 == pInfo->cPeekRead &&
			memcmp(&LastLogEvent2, pr, sizeof(LastLogEvent2)) == 0)
		{
			LastLogEventDup2 ++;
			_wsprintf(szTime, SKIPLEN(countof(szTime)) L"%u", LastLogEventDup2);
			ListView_SetItemText(hList, 1, lic_Dup, szTime); // верхний
			//free(pr);
			continue; // дубли - не показывать? только если прошло время?
		}
		int nItem = ListView_InsertItem(hList, &lvi);
		if (LastLogEventType1 && LastLogEventType1 != pInfo->cPeekRead)
		{
			LastLogEvent2 = LastLogEvent1; LastLogEventType2 = LastLogEventType1; LastLogEventDup2 = LastLogEventDup1;
		}
		LastLogEventType1 = pInfo->cPeekRead;
		memmove(&LastLogEvent1, pr, sizeof(LastLogEvent1));
		LastLogEventDup1 = 1;

		_wcscpy_c(szTime, countof(szTime), L"1");
		ListView_SetItemText(hList, nItem, lic_Dup, szTime);
		//
		szTime[0] = (wchar_t)pInfo->cPeekRead; szTime[1] = L'.'; szTime[2] = 0;
		if (pr->EventType == MOUSE_EVENT)
		{
			wcscat_c(szTime, L"Mouse");
			ListView_SetItemText(hList, nItem, lic_Type, szTime);
			const MOUSE_EVENT_RECORD *rec = &pr->Event.MouseEvent;
			_wsprintf(szTime, SKIPLEN(countof(szTime))
				L"[%d,%d], Btn=0x%08X (%c%c%c%c%c), Ctrl=0x%08X (%c%c%c%c%c - %c%c%c%c), Flgs=0x%08X (%s)",
				rec->dwMousePosition.X,
				rec->dwMousePosition.Y,
				rec->dwButtonState,
				(rec->dwButtonState&FROM_LEFT_1ST_BUTTON_PRESSED?L'L':L'l'),
				(rec->dwButtonState&RIGHTMOST_BUTTON_PRESSED?L'R':L'r'),
				(rec->dwButtonState&FROM_LEFT_2ND_BUTTON_PRESSED?L'2':L' '),
				(rec->dwButtonState&FROM_LEFT_3RD_BUTTON_PRESSED?L'3':L' '),
				(rec->dwButtonState&FROM_LEFT_4TH_BUTTON_PRESSED?L'4':L' '),
				rec->dwControlKeyState,
				(rec->dwControlKeyState&LEFT_CTRL_PRESSED?L'C':L'c'),
				(rec->dwControlKeyState&LEFT_ALT_PRESSED?L'A':L'a'),
				(rec->dwControlKeyState&SHIFT_PRESSED?L'S':L's'),
				(rec->dwControlKeyState&RIGHT_ALT_PRESSED?L'A':L'a'),
				(rec->dwControlKeyState&RIGHT_CTRL_PRESSED?L'C':L'c'),
				(rec->dwControlKeyState&ENHANCED_KEY?L'E':L'e'),
				(rec->dwControlKeyState&CAPSLOCK_ON?L'C':L'c'),
				(rec->dwControlKeyState&NUMLOCK_ON?L'N':L'n'),
				(rec->dwControlKeyState&SCROLLLOCK_ON?L'S':L's'),
				rec->dwEventFlags,
				(rec->dwEventFlags==0?L"(Click)":
				(rec->dwEventFlags==DOUBLE_CLICK?L"(DblClick)":
				(rec->dwEventFlags==MOUSE_MOVED?L"(Moved)":
				(rec->dwEventFlags==MOUSE_WHEELED?L"(Wheel)":
				(rec->dwEventFlags==0x0008/*MOUSE_HWHEELED*/?L"(HWheel)":L"")))))
				);
			if (rec->dwEventFlags==MOUSE_WHEELED  || rec->dwEventFlags==0x0008/*MOUSE_HWHEELED*/)
			{
				int nLen = _tcslen(szTime);
				_wsprintf(szTime+nLen, SKIPLEN(countof(szTime)-nLen)
					L" (Delta=%d)",HIWORD(rec->dwButtonState));
			}
			ListView_SetItemText(hList, nItem, lic_Event, szTime);
		}
		else if (pr->EventType == KEY_EVENT)
		{
			wcscat_c(szTime, L"Key");
			ListView_SetItemText(hList, nItem, lic_Type, szTime);
			_wsprintf(szTime, SKIPLEN(countof(szTime)) L"%c(%i) VK=%i, SC=%i, U=%c(x%04X), ST=x%08X",
				pr->Event.KeyEvent.bKeyDown ? L'D' : L'U',
				pr->Event.KeyEvent.wRepeatCount,
				pr->Event.KeyEvent.wVirtualKeyCode, pr->Event.KeyEvent.wVirtualScanCode,
				pr->Event.KeyEvent.uChar.UnicodeChar ? pr->Event.KeyEvent.uChar.UnicodeChar : L' ',
				pr->Event.KeyEvent.uChar.UnicodeChar,
				pr->Event.KeyEvent.dwControlKeyState);
			ListView_SetItemText(hList, nItem, lic_Event, szTime);
		}
		else if (pr->EventType == FOCUS_EVENT)
		{
			wcscat_c(szTime, L"Focus");
			ListView_SetItemText(hList, nItem, lic_Type, szTime);
			_wsprintf(szTime, SKIPLEN(countof(szTime)) L"%u", (DWORD)pr->Event.FocusEvent.bSetFocus);
			ListView_SetItemText(hList, nItem, lic_Event, szTime);
		}
		else if (pr->EventType == WINDOW_BUFFER_SIZE_EVENT)
		{
			wcscat_c(szTime, L"Buffer");
			ListView_SetItemText(hList, nItem, lic_Type, szTime);
			_wsprintf(szTime, SKIPLEN(countof(szTime)) L"%ix%i", (int)pr->Event.WindowBufferSizeEvent.dwSize.X, (int)pr->Event.WindowBufferSizeEvent.dwSize.Y);
			ListView_SetItemText(hList, nItem, lic_Event, szTime);
		}
		else if (pr->EventType == MENU_EVENT)
		{
			wcscat_c(szTime, L"Menu");
			ListView_SetItemText(hList, nItem, lic_Type, szTime);
			_wsprintf(szTime, SKIPLEN(countof(szTime)) L"%u", (DWORD)pr->Event.MenuEvent.dwCommandId);
			ListView_SetItemText(hList, nItem, lic_Event, szTime);
		}
		else
		{
			_wsprintf(szTime+2, SKIPLEN(countof(szTime)-2) L"%u", (DWORD)pr->EventType);
			ListView_SetItemText(hList, nItem, lic_Type, szTime);
		}
	}
	free(pInfo);
}

void CSettings::debugLogCommand(CESERVER_REQ* pInfo, BOOL abInput, DWORD anTick, DWORD anDur, LPCWSTR asPipe, CESERVER_REQ* pResult/*=NULL*/)
{
	if ((m_ActivityLoggingType != glt_Commands) || (hDebug == NULL))
		return;

	_ASSERTE(abInput==TRUE || pResult!=NULL || (pInfo->hdr.nCmd==CECMD_LANGCHANGE));
		
	LogCommandsData* pData = (LogCommandsData*)calloc(1,sizeof(LogCommandsData));
	
	if (!pData)
		return;

	pData->bInput = abInput;
	pData->bMainThread = (abInput == FALSE) && gpConEmu->isMainThread();
	pData->nTick = anTick - mn_ActivityCmdStartTick;
	pData->nDur = anDur;
	pData->nCmd = pInfo->hdr.nCmd;
	pData->nSize = pInfo->hdr.cbSize;
	pData->nPID = abInput ? pInfo->hdr.nSrcPID : pResult ? pResult->hdr.nSrcPID : 0;
	LPCWSTR pszName = asPipe ? PointToName(asPipe) : NULL;
	lstrcpyn(pData->szPipe, pszName ? pszName : L"", countof(pData->szPipe));
	switch (pInfo->hdr.nCmd)
	{
	case CECMD_POSTCONMSG:
		_wsprintf(pData->szExtra, SKIPLEN(countof(pData->szExtra))
			L"HWND=x%08X, Msg=%u, wParam=" WIN3264TEST(L"x%08X",L"x%08X%08X") L", lParam=" WIN3264TEST(L"x%08X",L"x%08X%08X") L": ",
			pInfo->Msg.hWnd, pInfo->Msg.nMsg, WIN3264WSPRINT(pInfo->Msg.wParam), WIN3264WSPRINT(pInfo->Msg.lParam));
		GetClassName(pInfo->Msg.hWnd, pData->szExtra+lstrlen(pData->szExtra), countof(pData->szExtra)-lstrlen(pData->szExtra));
		break;
	case CECMD_NEWCMD:
		lstrcpyn(pData->szExtra, pInfo->NewCmd.szCommand, countof(pData->szExtra));
		break;
	case CECMD_GUIMACRO:
		lstrcpyn(pData->szExtra, pInfo->GuiMacro.sMacro, countof(pData->szExtra));
		break;
	case CMD_POSTMACRO:
		lstrcpyn(pData->szExtra, (LPCWSTR)pInfo->wData, countof(pData->szExtra));
		break;
	}
	
	PostMessage(gpSetCls->hDebug, DBGMSG_LOG_ID, DBGMSG_LOG_CMD_MAGIC, (LPARAM)pData);
}

void CSettings::debugLogCommand(HWND hWnd2, LogCommandsData* apData)
{
	if (!apData)
		return;
	
	/*
		struct LogCommandsData
		{
			BOOL  bInput, bMainThread;
			DWORD nTick, nDur, nCmd, nSize, nPID;
			wchar_t szPipe[64];
		};
	*/

	wchar_t szText[128]; //_wsprintf(szTime, SKIPLEN(countof(szTime)) L"%02i:%02i:%02i", st.wHour, st.wMinute, st.wSecond);
	HWND hList = GetDlgItem(hWnd2, lbActivityLog);
	
	wcscpy_c(szText, apData->bInput ? L"In" : L"Out");

	LVITEM lvi = {LVIF_TEXT|LVIF_STATE};
	lvi.state = lvi.stateMask = LVIS_SELECTED|LVIS_FOCUSED;
	lvi.pszText = szText;
	int nItem = ListView_InsertItem(hList, &lvi);
	
	TODO("Проверить округления в CPP");
	int nMin = apData->nTick / 60000; apData->nTick -= nMin*60000;
	int nSec = apData->nTick / 1000;
	int nMS = apData->nTick % 1000;
	_wsprintf(szText, SKIPLEN(countof(szText)) L"%02i:%02i:%03i", nMin, nSec, nMS);
	ListView_SetItemText(hList, nItem, lcc_Time, szText);
	
	_wsprintf(szText, SKIPLEN(countof(szText)) apData->bInput ? L"" : L"%u", apData->nDur);
	ListView_SetItemText(hList, nItem, lcc_Duration, szText);
	
	_wsprintf(szText, SKIPLEN(countof(szText)) L"%u", apData->nCmd);
	ListView_SetItemText(hList, nItem, lcc_Command, szText);
	
	_wsprintf(szText, SKIPLEN(countof(szText)) L"%u", apData->nSize);
	ListView_SetItemText(hList, nItem, lcc_Size, szText);
	
	_wsprintf(szText, SKIPLEN(countof(szText)) apData->nPID ? L"%u" : L"", apData->nPID);
	ListView_SetItemText(hList, nItem, lcc_PID, szText);
	
	ListView_SetItemText(hList, nItem, lcc_Pipe, apData->szPipe);
	
	free(apData);
}

LRESULT CSettings::OnActivityLogNotify(HWND hWnd2, WPARAM wParam, LPARAM lParam)
{
	switch (((NMHDR*)lParam)->code)
	{
	case LVN_ITEMCHANGED:
		{
			LPNMLISTVIEW p = (LPNMLISTVIEW)lParam;
			if ((p->uNewState & LVIS_SELECTED) && (p->iItem >= 0))
			{
				HWND hList = GetDlgItem(hWnd2, lbActivityLog);
				//HWND hDetails = GetDlgItem(hWnd2, lbActivityDetails);
				wchar_t szText[65535]; szText[0] = 0;
				if (gpSetCls->m_ActivityLoggingType == glt_Processes)
				{
					ListView_GetItemText(hList, p->iItem, lpc_App, szText, countof(szText));
					SetDlgItemText(hWnd2, ebActivityApp, szText);
					ListView_GetItemText(hList, p->iItem, lpc_Params, szText, countof(szText));
					SetDlgItemText(hWnd2, ebActivityParm, szText);
				}
				else if (gpSetCls->m_ActivityLoggingType == glt_Input)
				{
					ListView_GetItemText(hList, p->iItem, lic_Type, szText, countof(szText));
					wcscat_c(szText, L" - ");
					int nLen = _tcslen(szText);
					ListView_GetItemText(hList, p->iItem, lic_Dup, szText+nLen, countof(szText)-nLen);
					SetDlgItemText(hWnd2, ebActivityApp, szText);
					ListView_GetItemText(hList, p->iItem, lic_Event, szText, countof(szText));
					SetDlgItemText(hWnd2, ebActivityParm, szText);
				}
				else if (gpSetCls->m_ActivityLoggingType == glt_Commands)
				{
					ListView_GetItemText(hList, p->iItem, lcc_Pipe, szText, countof(szText));
					SetDlgItemText(hWnd2, ebActivityApp, szText);
					SetDlgItemText(hWnd2, ebActivityParm, L"");
				}
				else
				{
					SetDlgItemText(hWnd2, ebActivityApp, L"");
					SetDlgItemText(hWnd2, ebActivityParm, L"");
				}
			}
		} //LVN_ITEMCHANGED
		break;
	}
	return 0;
}

void CSettings::OnSaveActivityLogFile(HWND hListView)
{
	wchar_t szLogFile[MAX_PATH];
	OPENFILENAME ofn; memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hwndOwner = ghOpWnd;
	ofn.lpstrFilter = _T("Log files (*.csv)\0*.csv\0\0");
	ofn.nFilterIndex = 1;

	ofn.lpstrFile = szLogFile; szLogFile[0] = 0;
	ofn.nMaxFile = countof(szLogFile);
	ofn.lpstrTitle = L"Save shell and processes log...";
	ofn.lpstrDefExt = L"csv";
	ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
	            | OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;

	if (!GetSaveFileName(&ofn))
		return;

	HANDLE hFile = CreateFile(szLogFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, 0, NULL);
	if (!hFile || hFile == INVALID_HANDLE_VALUE)
	{
		DisplayLastError(L"Create log file failed!");
		return;
	}

	int nMaxText = 262144;
	wchar_t* pszText = (wchar_t*)malloc(nMaxText*sizeof(wchar_t));
	LVCOLUMN lvc = {LVCF_TEXT};
	//LVITEM lvi = {0};
	int nColumnCount = 0;
	DWORD nWritten = 0, nLen;

	for (;;nColumnCount++)
	{
		if (nColumnCount)
			WriteFile(hFile, L";", 2, &nWritten, NULL);

		lvc.pszText = pszText;
		lvc.cchTextMax = nMaxText;
		if (!ListView_GetColumn(hListView, nColumnCount, &lvc))
			break;

		nLen = _tcslen(pszText)*2;
		if (nLen)
			WriteFile(hFile, pszText, nLen, &nWritten, NULL);
	}
	WriteFile(hFile, L"\r\n", 2*sizeof(wchar_t), &nWritten, NULL); //-V112

	if (nColumnCount > 0)
	{
		INT_PTR nItems = ListView_GetItemCount(hListView); //-V220
		for (INT_PTR i = 0; i < nItems; i++)
		{
			for (int c = 0; c < nColumnCount; c++)
			{
				if (c)
					WriteFile(hFile, L";", 2, &nWritten, NULL);
				pszText[0] = 0;
				ListView_GetItemText(hListView, i, c, pszText, nMaxText);
				nLen = _tcslen(pszText)*2;
				if (nLen)
					WriteFile(hFile, pszText, nLen, &nWritten, NULL);
			}
			WriteFile(hFile, L"\r\n", 2*sizeof(wchar_t), &nWritten, NULL); //-V112
		}
	}

	free(pszText);
	CloseHandle(hFile);
}

//void CSettings::CenterDialog(HWND hWnd2)
//{
//	RECT rcParent, rc;
//	GetWindowRect(ghOpWnd, &rcParent);
//	GetWindowRect(hWnd2, &rc);
//	MoveWindow(hWnd2,
//		(rcParent.left+rcParent.right-rc.right+rc.left)/2,
//		(rcParent.top+rcParent.bottom-rc.bottom+rc.top)/2,
//		rc.right - rc.left, rc.bottom - rc.top, TRUE);
//}

//bool CSettings::isKeyboardHooks()
//{
////	#ifndef _DEBUG
//	// Для WinXP это не было нужно
//	if (gOSVer.dwMajorVersion < 6)
//	{
//		return false;
//	}
//
////	#endif
//
//	if (gpSet->m_isKeyboardHooks == 0)
//	{
//		// Вопрос пользователю еще не задавали (это на старте, окно еще и не создано)
//		int nBtn = MessageBox(NULL,
//		                      L"Do You want to use Win-Number combination for \n"
//		                      L"switching between consoles (Multi Console feature)? \n\n"
//		                      L"If You choose 'Yes' - ConEmu will install keyboard hook. \n"
//		                      L"So, You must allow that in antiviral software (such as AVP). \n\n"
//		                      L"You can change behavior later via Settings->Features->\n"
//		                      L"'Install keyboard hooks (Vista & Win7)' check box, or\n"
//		                      L"'KeyboardHooks' value in ConEmu settings (registry or xml)."
//		                      , gpConEmu->GetDefaultTitle(), MB_YESNOCANCEL|MB_ICONQUESTION);
//
//		if (nBtn == IDCANCEL)
//		{
//			gpSet->m_isKeyboardHooks = 2; // NO
//		}
//		else
//		{
//			gpSet->m_isKeyboardHooks = (nBtn == IDYES) ? 1 : 2;
//			SettingsBase* reg = CreateSettings();
//
//			if (!reg)
//			{
//				_ASSERTE(reg!=NULL);
//			}
//			else
//			{
//				if (reg->OpenKey(ConfigPath, KEY_WRITE))
//				{
//					reg->Save(L"KeyboardHooks", gpSet->m_isKeyboardHooks);
//					reg->CloseKey();
//				}
//
//				delete reg;
//			}
//		}
//	}
//
//	return (gpSet->m_isKeyboardHooks == 1);
//}

//bool CSettings::IsHostkey(WORD vk)
//{
//	for(int i=0; i < 15 && mn_HostModOk[i]; i++)
//		if (mn_HostModOk[i] == vk)
//			return true;
//
//	return false;
//}

//// Если есть vk - заменить на vkNew
//void CSettings::ReplaceHostkey(BYTE vk, BYTE vkNew)
//{
//	for(int i = 0; i < 15; i++)
//	{
//		if (gpSet->mn_HostModOk[i] == vk)
//		{
//			gpSet->mn_HostModOk[i] = vkNew;
//			return;
//		}
//	}
//}
//
//void CSettings::AddHostkey(BYTE vk)
//{
//	for(int i = 0; i < 15; i++)
//	{
//		if (gpSet->mn_HostModOk[i] == vk)
//			break; // уже есть
//
//		if (!gpSet->mn_HostModOk[i])
//		{
//			gpSet->mn_HostModOk[i] = vk; // добавить
//			break;
//		}
//	}
//}
//
//BYTE CSettings::CheckHostkeyModifier(BYTE vk)
//{
//	// Если передан VK_NULL
//	if (!vk)
//		return 0;
//
//	switch(vk)
//	{
//		case VK_LWIN: case VK_RWIN:
//
//			if (gpSet->IsHostkey(VK_RWIN))
//				ReplaceHostkey(VK_RWIN, VK_LWIN);
//
//			vk = VK_LWIN; // Сохраняем только Левый-Win
//			break;
//		case VK_APPS:
//			break; // Это - ок
//		case VK_LSHIFT:
//
//			if (gpSet->IsHostkey(VK_RSHIFT) || gpSet->IsHostkey(VK_SHIFT))
//			{
//				vk = VK_SHIFT;
//				ReplaceHostkey(VK_RSHIFT, VK_SHIFT);
//			}
//
//			break;
//		case VK_RSHIFT:
//
//			if (gpSet->IsHostkey(VK_LSHIFT) || gpSet->IsHostkey(VK_SHIFT))
//			{
//				vk = VK_SHIFT;
//				ReplaceHostkey(VK_LSHIFT, VK_SHIFT);
//			}
//
//			break;
//		case VK_SHIFT:
//
//			if (gpSet->IsHostkey(VK_LSHIFT))
//				ReplaceHostkey(VK_LSHIFT, VK_SHIFT);
//			else if (gpSet->IsHostkey(VK_RSHIFT))
//				ReplaceHostkey(VK_RSHIFT, VK_SHIFT);
//
//			break;
//		case VK_LMENU:
//
//			if (gpSet->IsHostkey(VK_RMENU) || gpSet->IsHostkey(VK_MENU))
//			{
//				vk = VK_MENU;
//				ReplaceHostkey(VK_RMENU, VK_MENU);
//			}
//
//			break;
//		case VK_RMENU:
//
//			if (gpSet->IsHostkey(VK_LMENU) || gpSet->IsHostkey(VK_MENU))
//			{
//				vk = VK_MENU;
//				ReplaceHostkey(VK_LMENU, VK_MENU);
//			}
//
//			break;
//		case VK_MENU:
//
//			if (gpSet->IsHostkey(VK_LMENU))
//				ReplaceHostkey(VK_LMENU, VK_MENU);
//			else if (gpSet->IsHostkey(VK_RMENU))
//				ReplaceHostkey(VK_RMENU, VK_MENU);
//
//			break;
//		case VK_LCONTROL:
//
//			if (gpSet->IsHostkey(VK_RCONTROL) || gpSet->IsHostkey(VK_CONTROL))
//			{
//				vk = VK_CONTROL;
//				ReplaceHostkey(VK_RCONTROL, VK_CONTROL);
//			}
//
//			break;
//		case VK_RCONTROL:
//
//			if (gpSet->IsHostkey(VK_LCONTROL) || gpSet->IsHostkey(VK_CONTROL))
//			{
//				vk = VK_CONTROL;
//				ReplaceHostkey(VK_LCONTROL, VK_CONTROL);
//			}
//
//			break;
//		case VK_CONTROL:
//
//			if (gpSet->IsHostkey(VK_LCONTROL))
//				ReplaceHostkey(VK_LCONTROL, VK_CONTROL);
//			else if (gpSet->IsHostkey(VK_RCONTROL))
//				ReplaceHostkey(VK_RCONTROL, VK_CONTROL);
//
//			break;
//	}
//
//	// Добавить в список входящих в Host
//	AddHostkey(vk);
//	// Вернуть (возможно измененный) VK
//	return vk;
//}
//
//bool CSettings::TestHostkeyModifiers()
//{
//	memset(mn_HostModOk, 0, sizeof(mn_HostModOk));
//	memset(mn_HostModSkip, 0, sizeof(mn_HostModSkip));
//
//	if (!nMultiHotkeyModifier)
//		nMultiHotkeyModifier = VK_LWIN;
//
//	BYTE vk;
//	vk = (nMultiHotkeyModifier & 0xFF);
//	CheckHostkeyModifier(vk);
//	vk = (nMultiHotkeyModifier & 0xFF00) >> 8;
//	CheckHostkeyModifier(vk);
//	vk = (nMultiHotkeyModifier & 0xFF0000) >> 16;
//	CheckHostkeyModifier(vk);
//	vk = (nMultiHotkeyModifier & 0xFF000000) >> 24;
//	CheckHostkeyModifier(vk);
//	// Однако, допустимо не более 3-х клавиш (больше смысла не имеет)
//	TrimHostkeys();
//	// Сформировать (возможно скорректированную) маску HostKey
//	bool lbChanged = MakeHostkeyModifier();
//	return lbChanged;
//}
//
//bool CSettings::MakeHostkeyModifier()
//{
//	bool lbChanged = false;
//	// Сформировать (возможно скорректированную) маску HostKey
//	DWORD nNew = 0;
//
//	if (gpSet->mn_HostModOk[0])
//		nNew |= gpSet->mn_HostModOk[0];
//
//	if (gpSet->mn_HostModOk[1])
//		nNew |= ((DWORD)(gpSet->mn_HostModOk[1])) << 8;
//
//	if (gpSet->mn_HostModOk[2])
//		nNew |= ((DWORD)(gpSet->mn_HostModOk[2])) << 16;
//
//	if (gpSet->mn_HostModOk[3])
//		nNew |= ((DWORD)(gpSet->mn_HostModOk[3])) << 24;
//
//	TODO("!!! Добавить в mn_HostModSkip те VK, которые отсутствуют в mn_HostModOk");
//
//	if (gpSet->nMultiHotkeyModifier != nNew)
//	{
//		gpSet->nMultiHotkeyModifier = nNew;
//		lbChanged = true;
//	}
//
//	return lbChanged;
//}
//
//// Оставить в mn_HostModOk только 3 VK
//void CSettings::TrimHostkeys()
//{
//	if (gpSet->mn_HostModOk[0] == 0)
//		return;
//
//	int i = 0;
//
//	while (++i < 15 && gpSet->mn_HostModOk[i])
//		;
//
//	// Если вдруг задали более 3-х модификаторов - обрезать, оставив 3 последних
//	if (i > 3)
//	{
//		if (i >= countof(gpSet->mn_HostModOk))
//		{
//			_ASSERTE(i < countof(gpSet->mn_HostModOk));
//			i = countof(gpSet->mn_HostModOk) - 1;
//		}
//		memmove(gpSet->mn_HostModOk, gpSet->mn_HostModOk+i-3, 3); //-V112
//	}
//
//	memset(gpSet->mn_HostModOk+3, 0, sizeof(gpSet->mn_HostModOk)-3);
//}

void CSettings::SetupHotkeyChecks(HWND hWnd2)
{
	bool b;
	CheckDlgButton(hWnd2, cbHostWin, gpSet->IsHostkey(VK_LWIN));
	CheckDlgButton(hWnd2, cbHostApps, gpSet->IsHostkey(VK_APPS));
	b = gpSet->IsHostkey(VK_SHIFT);
	CheckDlgButton(hWnd2, cbHostLShift, b || gpSet->IsHostkey(VK_LSHIFT));
	CheckDlgButton(hWnd2, cbHostRShift, b || gpSet->IsHostkey(VK_RSHIFT));
	b = gpSet->IsHostkey(VK_MENU);
	CheckDlgButton(hWnd2, cbHostLAlt, b || gpSet->IsHostkey(VK_LMENU));
	CheckDlgButton(hWnd2, cbHostRAlt, b || gpSet->IsHostkey(VK_RMENU));
	b = gpSet->IsHostkey(VK_CONTROL);
	CheckDlgButton(hWnd2, cbHostLCtrl, b || gpSet->IsHostkey(VK_LCONTROL));
	CheckDlgButton(hWnd2, cbHostRCtrl, b || gpSet->IsHostkey(VK_RCONTROL));
}

BYTE CSettings::HostkeyCtrlId2Vk(WORD nID)
{
	switch(nID)
	{
		case cbHostWin:
			return VK_LWIN;
		case cbHostApps:
			return VK_APPS;
		case cbHostLShift:
			return VK_LSHIFT;
		case cbHostRShift:
			return VK_RSHIFT;
		case cbHostLAlt:
			return VK_LMENU;
		case cbHostRAlt:
			return VK_RMENU;
		case cbHostLCtrl:
			return VK_LCONTROL;
		case cbHostRCtrl:
			return VK_RCONTROL;
	}

	return 0;
}

//bool CSettings::isHostkeySingleLR(WORD vk, WORD vkC, WORD vkL, WORD vkR)
//{
//	if (vk == vkC)
//	{
//		bool bLeft  = isPressed(vkL);
//		bool bRight = isPressed(vkR);
//
//		if (bLeft && !bRight)
//			return (nMultiHotkeyModifier == vkL);
//
//		if (bRight && !bLeft)
//			return (nMultiHotkeyModifier == vkR);
//
//		// нажатие обоих шифтов - игнорируем
//		return false;
//	}
//
//	if (vk == vkL)
//		return (nMultiHotkeyModifier == vkL);
//
//	if (vk == vkR)
//		return (nMultiHotkeyModifier == vkR);
//
//	return false;
//}
//
//bool CSettings::IsHostkeySingle(WORD vk)
//{
//	if (nMultiHotkeyModifier > 0xFF)
//		return false; // в Host-комбинации больше одной клавиши
//
//	if (vk == VK_LWIN || vk == VK_RWIN)
//		return (nMultiHotkeyModifier == VK_LWIN);
//
//	if (vk == VK_APPS)
//		return (nMultiHotkeyModifier == VK_APPS);
//
//	if (vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT)
//		return isHostkeySingleLR(vk, VK_SHIFT, VK_LSHIFT, VK_RSHIFT);
//
//	if (vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL)
//		return isHostkeySingleLR(vk, VK_CONTROL, VK_LCONTROL, VK_RCONTROL);
//
//	if (vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU)
//		return isHostkeySingleLR(vk, VK_MENU, VK_LMENU, VK_RMENU);
//
//	return false;
//}
//
//// набор флагов MOD_xxx для RegisterHotKey
//UINT CSettings::GetHostKeyMod()
//{
//	UINT nMOD = 0;
//
//	for(int i=0; i < 15 && mn_HostModOk[i]; i++)
//	{
//		switch(mn_HostModOk[i])
//		{
//			case VK_LWIN: case VK_RWIN:
//				nMOD |= MOD_WIN;
//				break;
//			case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
//				nMOD |= MOD_CONTROL;
//				break;
//			case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:
//				nMOD |= MOD_SHIFT;
//				break;
//			case VK_MENU: case VK_LMENU: case VK_RMENU:
//				nMOD |= MOD_ALT;
//				break;
//		}
//	}
//
//	if (!nMOD)
//		nMOD = MOD_WIN;
//
//	return nMOD;
//}
//
//WORD CSettings::GetPressedHostkey()
//{
//	_ASSERTE(mn_HostModOk[0]!=0);
//
//	if (mn_HostModOk[0] == VK_LWIN)
//	{
//		if (isPressed(VK_LWIN))
//			return VK_LWIN;
//
//		if (isPressed(VK_RWIN))
//			return VK_RWIN;
//	}
//
//	if (!isPressed(mn_HostModOk[0]))
//	{
//		_ASSERT(FALSE);
//		return 0;
//	}
//
//	// Для правых-левых - возвращаем общий, т.к. именно он приходит в WM_KEYUP
//	if (mn_HostModOk[0] == VK_LSHIFT || mn_HostModOk[0] == VK_RSHIFT)
//		return VK_SHIFT;
//
//	if (mn_HostModOk[0] == VK_LMENU || mn_HostModOk[0] == VK_RMENU)
//		return VK_MENU;
//
//	if (mn_HostModOk[0] == VK_LCONTROL || mn_HostModOk[0] == VK_RCONTROL)
//		return VK_CONTROL;
//
//	return mn_HostModOk[0];
//}
//
//bool CSettings::IsHostkeyPressed()
//{
//	if (mn_HostModOk[0] == 0)
//	{
//		_ASSERTE(mn_HostModOk[0]!=0);
//		mn_HostModOk[0] = VK_LWIN;
//		return isPressed(VK_LWIN) || isPressed(VK_RWIN);
//	}
//
//	// Не более 3-х модификаторов + кнопка
//	_ASSERTE(mn_HostModOk[4] == 0);
//	for(int i = 0; i < 4 && mn_HostModOk[i]; i++) //-V112
//	{
//		if (mn_HostModOk[i] == VK_LWIN)
//		{
//			if (!(isPressed(VK_LWIN) || isPressed(VK_RWIN)))
//				return false;
//		}
//		else if (!isPressed(mn_HostModOk[i]))
//		{
//			return false;
//		}
//	}
//
//	// Не более 3-х модификаторов + кнопка
//	for(int j = 0; j < 4 && mn_HostModSkip[j]; j++) //-V112
//	{
//		if (isPressed(mn_HostModSkip[j]))
//			return false;
//	}
//
//	return true;
//}

//INT_PTR CSettings::multiOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
//{
//	switch (messg)
//	{
//	case WM_INITDIALOG:
//		{
//			gpSet->SetupHotkeyChecks(hWnd2);
//
//			SendDlgItemMessage(hWnd2, hkNewConsole, HKM_SETRULES, HKCOMB_A|HKCOMB_C|HKCOMB_CA|HKCOMB_S|HKCOMB_SA|HKCOMB_SC|HKCOMB_SCA, 0);
//			SendDlgItemMessage(hWnd2, hkNewConsole, HKM_SETHOTKEY, gpSet->icMultiNew, 0);
//			SendDlgItemMessage(hWnd2, hkSwitchConsole, HKM_SETRULES, HKCOMB_A|HKCOMB_C|HKCOMB_CA|HKCOMB_S|HKCOMB_SA|HKCOMB_SC|HKCOMB_SCA, 0);
//			SendDlgItemMessage(hWnd2, hkSwitchConsole, HKM_SETHOTKEY, gpSet->icMultiNext, 0);
//			SendDlgItemMessage(hWnd2, hkCloseConsole, HKM_SETRULES, HKCOMB_A|HKCOMB_C|HKCOMB_CA|HKCOMB_S|HKCOMB_SA|HKCOMB_SC|HKCOMB_SCA, 0);
//			SendDlgItemMessage(hWnd2, hkCloseConsole, HKM_SETHOTKEY, gpSet->icMultiRecreate, 0);
//
//			if (gpSet->isUseWinNumber) CheckDlgButton(hWnd2, cbUseWinNumber, BST_CHECKED);
//
//			SetDlgItemText(hWnd2, tTabConsole, gpSet->szTabConsole);
//			SetDlgItemText(hWnd2, tTabViewer, gpSet->szTabViewer);
//			SetDlgItemText(hWnd2, tTabEditor, gpSet->szTabEditor);
//			SetDlgItemText(hWnd2, tTabEditorMod, gpSet->szTabEditorModified);
//
//			SetDlgItemInt(hWnd2, tTabLenMax, gpSet->nTabLenMax, FALSE);
//
//			if (gpSet->bAdminShield) CheckDlgButton(hWnd2, cbAdminShield, BST_CHECKED);
//			SetDlgItemText(hWnd2, tAdminSuffix, gpSet->szAdminTitleSuffix);
//			EnableWindow(GetDlgItem(hWnd2, tAdminSuffix), !gpSet->bAdminShield);
//
//			if (gpSet->bHideInactiveConsoleTabs) CheckDlgButton(hWnd2, cbHideInactiveConTabs, BST_CHECKED);
//
//			gpSetCls->RegisterTipsFor(hWnd2);
//			CenterMoreDlg(hWnd2);
//		}
//		break;
//
//	case WM_GETICON:
//		if (wParam!=ICON_BIG) {
//			SetWindowLongPtr(hWnd2, DWLP_MSGRESULT, (LRESULT)hClassIconSm);
//			return 1;
//		}
//		break;
//
//	case WM_COMMAND:
//		if (HIWORD(wParam) == BN_CLICKED)
//		{
//			WORD TB = LOWORD(wParam);
//			if (TB == IDOK || TB == IDCANCEL) {
//				EndDialog(hWnd2, TB);
//				return 1;
//			}
//
//			if (TB >= cbHostWin && TB <= cbHostRShift)
//			{
//				memset(gpSet->mn_HostModOk, 0, sizeof(gpSet->mn_HostModOk));
//				for (UINT i = 0; i < countof(HostkeyCtrlIds); i++) {
//					if (IsChecked(hWnd2, HostkeyCtrlIds[i]))
//						gpSet->CheckHostkeyModifier(HostkeyCtrlId2Vk(HostkeyCtrlIds[i]));
//				}
//				gpSet->TrimHostkeys();
//				if (IsChecked(hWnd2, TB)) {
//					gpSet->CheckHostkeyModifier(HostkeyCtrlId2Vk(TB));
//					gpSet->TrimHostkeys();
//				}
//				// Обновить, что осталось
//				gpSet->SetupHotkeyChecks(hWnd2);
//				gpSet->MakeHostkeyModifier();
//			} else
//			if (TB == cbAdminShield) {
//				gpSet->bAdminShield = IsChecked(hWnd2, cbAdminShield);
//				EnableWindow(GetDlgItem(hWnd2, tAdminSuffix), !gpSet->bAdminShield);
//				gpConEmu->mp_TabBar->Update(TRUE);
//			} else
//			if (TB == cbHideInactiveConTabs) {
//				gpSet->bHideInactiveConsoleTabs = IsChecked(hWnd2, cbHideInactiveConTabs);
//				gpConEmu->mp_TabBar->Update(TRUE);
//			} else
//			if (TB == cbUseWinNumber) {
//				gpSet->isUseWinNumber = IsChecked(hWnd2, cbUseWinNumber);
//			}
//
//		} else if (HIWORD(wParam) == EN_CHANGE)	{
//			WORD TB = LOWORD(wParam);
//			if (TB == hkNewConsole || TB == hkSwitchConsole || TB == hkCloseConsole) {
//				UINT nHotKey = 0xFF & SendDlgItemMessage(hWnd2, TB, HKM_GETHOTKEY, 0, 0);
//				if (TB == hkNewConsole)
//					gpSet->icMultiNew = nHotKey;
//				else if (TB == hkSwitchConsole)
//					gpSet->icMultiNext = nHotKey;
//				else if (TB == hkCloseConsole)
//					gpSet->icMultiRecreate = nHotKey;
//			} else
//			if (TB == tTabConsole || TB == tTabViewer || TB == tTabEditor || TB == tTabEditorMod) {
//				wchar_t temp[MAX_PATH]; temp[0] = 0;
//				if (GetDlgItemText(hWnd2, TB, temp, MAX_PATH) && temp[0]) {
//					temp[31] = 0; // страховка
//					if (wcsstr(temp, L"%s")) {
//						if (TB == tTabConsole)
//							lstrcpy(gpSet->szTabConsole, temp);
//						else if (TB == tTabViewer)
//							lstrcpy(gpSet->szTabViewer, temp);
//						else if (TB == tTabEditor)
//							lstrcpy(gpSet->szTabEditor, temp);
//						else if (tTabEditorMod)
//							lstrcpy(gpSet->szTabEditorModified, temp);
//						gpConEmu->mp_TabBar->Update(TRUE);
//					}
//				}
//			} else
//			if (TB == tTabLenMax) {
//				BOOL lbOk = FALSE;
//				DWORD n = GetDlgItemInt(hWnd2, tTabLenMax, &lbOk, FALSE);
//				if (n > 10 && n < CONEMUTABMAX) {
//					gpSet->nTabLenMax = n;
//					gpConEmu->mp_TabBar->Update(TRUE);
//				}
//			} else
//			if (TB == tAdminSuffix) {
//				GetDlgItemText(hWnd2, tAdminSuffix, gpSet->szAdminTitleSuffix, countof(gpSet->szAdminTitleSuffix));
//				gpConEmu->mp_TabBar->Update(TRUE);
//			}
//		}
//		break;
//
//	}
//	return 0;
//}

//INT_PTR CSettings::hideOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
//{
//	switch(messg)
//	{
//		case WM_INITDIALOG:
//		{
//			SendMessage(hWnd2, WM_SETICON, ICON_BIG, (LPARAM)hClassIcon);
//			SendMessage(hWnd2, WM_SETICON, ICON_SMALL, (LPARAM)hClassIconSm);
//
//			SetDlgItemInt(hWnd2, tHideCaptionAlwaysFrame, gpSet->nHideCaptionAlwaysFrame, FALSE);
//			SetDlgItemInt(hWnd2, tHideCaptionAlwaysDelay, gpSet->nHideCaptionAlwaysDelay, FALSE);
//			SetDlgItemInt(hWnd2, tHideCaptionAlwaysDissapear, gpSet->nHideCaptionAlwaysDisappear, FALSE);
//			gpSetCls->RegisterTipsFor(hWnd2);
//			CenterMoreDlg(hWnd2);
//		}
//		break;
//		//case WM_GETICON:
//
//		//	if (wParam==ICON_BIG)
//		//	{
//		//		/*SetWindowLong(hWnd2, DWL_MSGRESULT, (LRESULT)hClassIcon);
//		//		return 1;*/
//		//	}
//		//	else
//		//	{
//		//		SetWindowLongPtr(hWnd2, DWLP_MSGRESULT, (LRESULT)hClassIconSm);
//		//		return 1;
//		//	}
//
//		//	return 0;
//		case WM_COMMAND:
//
//			if (HIWORD(wParam) == BN_CLICKED)
//			{
//				WORD TB = LOWORD(wParam);
//
//				if (TB == IDOK || TB == IDCANCEL)
//					EndDialog(hWnd2, TB);
//			}
//			else if (HIWORD(wParam) == EN_CHANGE)
//			{
//				WORD TB = LOWORD(wParam);
//				BOOL lbOk = FALSE;
//				UINT nNewVal = GetDlgItemInt(hWnd2, TB, &lbOk, FALSE);
//
//				if (lbOk)
//				{
//					switch(TB)
//					{
//						case tHideCaptionAlwaysFrame:
//							gpSet->nHideCaptionAlwaysFrame = nNewVal;
//							gpConEmu->UpdateWindowRgn();
//							break;
//						case tHideCaptionAlwaysDelay:
//							gpSet->nHideCaptionAlwaysDelay = nNewVal;
//							break;
//						case tHideCaptionAlwaysDissapear:
//							gpSet->nHideCaptionAlwaysDisappear = nNewVal;
//							break;
//					}
//				}
//			}
//
//			break;
//	}
//
//	return 0;
//}

//INT_PTR CSettings::selectionOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
//{
//	switch(messg)
//	{
//		case WM_INITDIALOG:
//		{
//			SendMessage(hWnd2, WM_SETICON, ICON_BIG, (LPARAM)hClassIcon);
//			SendMessage(hWnd2, WM_SETICON, ICON_SMALL, (LPARAM)hClassIconSm);
//
//			//SetDlgItemInt (hWnd2, tHideCaptionAlwaysFrame, gpSet->nHideCaptionAlwaysFrame, FALSE);
//			//SetDlgItemInt (hWnd2, tHideCaptionAlwaysDelay, gpSet->nHideCaptionAlwaysDelay, FALSE);
//			//SetDlgItemInt (hWnd2, tHideCaptionAlwaysDissapear, gpSet->nHideCaptionAlwaysDisappear, FALSE);
//			if (!gpSet->isConsoleTextSelection) gpSet->isConsoleTextSelection = 2;
//
//			CheckDlgButton(hWnd2, (gpSet->isConsoleTextSelection==1)?rbCTSAlways:rbCTSBufferOnly, BST_CHECKED);
//			CheckDlgButton(hWnd2, cbCTSBlockSelection, gpSet->isCTSSelectBlock);
//			FillListBoxByte(hWnd2, lbCTSBlockSelection, SettingsNS::szKeys, SettingsNS::nKeys, gpSet->isCTSVkBlock);
//			CheckDlgButton(hWnd2, cbCTSTextSelection, gpSet->isCTSSelectText);
//			FillListBoxByte(hWnd2, lbCTSTextSelection, SettingsNS::szKeys, SettingsNS::nKeys, gpSet->isCTSVkText);
//			CheckDlgButton(hWnd2, (gpSet->isCTSActMode==1)?rbCTSActAlways:rbCTSActBufferOnly, BST_CHECKED);
//			FillListBoxByte(hWnd2, lbCTSActAlways, SettingsNS::szKeysAct, SettingsNS::nKeysAct, gpSet->isCTSVkAct);
//			FillListBoxByte(hWnd2, lbCTSRBtnAction, SettingsNS::szClipAct, SettingsNS::nClipAct, gpSet->isCTSRBtnAction);
//			FillListBoxByte(hWnd2, lbCTSMBtnAction, SettingsNS::szClipAct, SettingsNS::nClipAct, gpSet->isCTSMBtnAction);
//			DWORD idxBack = (gpSet->isCTSColorIndex & 0xF0) >> 4;
//			DWORD idxFore = (gpSet->isCTSColorIndex & 0xF);
//			FillListBoxItems(GetDlgItem(hWnd2, lbCTSForeIdx), countof(SettingsNS::szColorIdx)-1,
//			                 SettingsNS::szColorIdx, SettingsNS::nColorIdx, idxFore);
//			FillListBoxItems(GetDlgItem(hWnd2, lbCTSBackIdx), countof(SettingsNS::szColorIdx)-1,
//			                 SettingsNS::szColorIdx, SettingsNS::nColorIdx, idxBack);
//			// Не совсем выделение, но в том же диалоге
//			CheckDlgButton(hWnd2, cbFarGotoEditor, gpSet->isFarGotoEditor);
//			FillListBoxByte(hWnd2, lbFarGotoEditorVk, SettingsNS::szKeysAct, SettingsNS::nKeysAct, gpSet->isFarGotoEditorVk);
//			// тултипы
//			gpSetCls->RegisterTipsFor(hWnd2);
//			CenterMoreDlg(hWnd2);
//		}
//		break;
//		//case WM_GETICON:
//
//		//	if (wParam!=ICON_BIG)
//		//	{
//		//		SetWindowLongPtr(hWnd2, DWLP_MSGRESULT, (LRESULT)hClassIconSm);
//		//		return 1;
//		//	}
//
//		//	return 0;
//		case WM_COMMAND:
//
//			if (HIWORD(wParam) == BN_CLICKED)
//			{
//				WORD CB = LOWORD(wParam);
//
//				switch(CB)
//				{
//					case IDOK: case IDCANCEL:
//						EndDialog(hWnd2, CB);
//						break;
//					//case rbCTSNever:
//					//case rbCTSAlways:
//					//case rbCTSBufferOnly:
//					//	gpSet->isConsoleTextSelection = (CB==rbCTSNever) ? 0 : (CB==rbCTSAlways) ? 1 : 2;
//					//	//CheckDlgButton(gpSetCls->hExt, cbConsoleTextSelection, gpSet->isConsoleTextSelection);
//					//	break;
//					//case rbCTSActAlways: case rbCTSActBufferOnly:
//					//	gpSet->isCTSActMode = (CB==rbCTSActAlways) ? 1 : 2;
//					//	break;
//					//case cbCTSBlockSelection:
//					//	gpSet->isCTSSelectBlock = IsChecked(hWnd2,CB);
//					//	break;
//					//case cbCTSTextSelection:
//					//	gpSet->isCTSSelectText = IsChecked(hWnd2,CB);
//					//	break;
//					//case cbFarGotoEditor:
//					//	gpSet->isFarGotoEditor = IsChecked(hWnd2,CB);
//					//	break;
//				}
//			}
//			else if (HIWORD(wParam) == CBN_SELCHANGE)
//			{
//				WORD CB = LOWORD(wParam);
//
//				switch(CB)
//				{
//					case lbCTSBlockSelection:
//					{
//						GetListBoxByte(hWnd2, lbCTSBlockSelection, SettingsNS::szKeys, SettingsNS::nKeys, gpSet->isCTSVkBlock);
//					} break;
//					case lbCTSTextSelection:
//					{
//						GetListBoxByte(hWnd2, lbCTSTextSelection, SettingsNS::szKeys, SettingsNS::nKeys, gpSet->isCTSVkText);
//					} break;
//					case lbCTSActAlways:
//					{
//						GetListBoxByte(hWnd2, lbCTSActAlways, SettingsNS::szKeysAct, SettingsNS::nKeysAct, gpSet->isCTSVkAct);
//					} break;
//					case lbCTSRBtnAction:
//					{
//						GetListBoxByte(hWnd2, lbCTSRBtnAction, SettingsNS::szClipAct, SettingsNS::nClipAct, gpSet->isCTSRBtnAction);
//					} break;
//					case lbCTSMBtnAction:
//					{
//						GetListBoxByte(hWnd2, lbCTSMBtnAction, SettingsNS::szClipAct, SettingsNS::nClipAct, gpSet->isCTSMBtnAction);
//					} break;
//					case lbCTSForeIdx:
//					{
//						DWORD nFore = 0;
//						GetListBoxItem(GetDlgItem(hWnd2, lbCTSForeIdx), countof(SettingsNS::szColorIdx)-1,
//						               SettingsNS::szColorIdx, SettingsNS::nColorIdx, nFore);
//						gpSet->isCTSColorIndex = (gpSet->isCTSColorIndex & 0xF0) | (nFore & 0xF);
//						gpConEmu->Update(true);
//					} break;
//					case lbCTSBackIdx:
//					{
//						DWORD nBack = 0;
//						GetListBoxItem(GetDlgItem(hWnd2, lbCTSBackIdx), countof(SettingsNS::szColorIdx)-1,
//						               SettingsNS::szColorIdx, SettingsNS::nColorIdx, nBack);
//						gpSet->isCTSColorIndex = (gpSet->isCTSColorIndex & 0xF) | ((nBack & 0xF) << 4);
//						gpConEmu->Update(true);
//					} break;
//					case lbFarGotoEditorVk:
//					{
//						GetListBoxByte(hWnd2, lbFarGotoEditorVk, SettingsNS::szKeysAct, SettingsNS::nKeysAct, gpSet->isFarGotoEditorVk);
//					} break;
//				}
//			}
//
//			break;
//	}
//
//	return 0;
//}

void CSettings::UpdatePos(int x, int y)
{
	TCHAR temp[32];
	gpSet->wndX = x;
	gpSet->wndY = y;

	if (ghOpWnd)
	{
		mb_IgnoreEditChanged = TRUE;
		_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", gpSet->wndX);
		SetDlgItemText(hMain, tWndX, temp);
		_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", gpSet->wndY);
		SetDlgItemText(hMain, tWndY, temp);
		mb_IgnoreEditChanged = FALSE;
	}
}

void CSettings::UpdateSize(UINT w, UINT h)
{
	TCHAR temp[32];

	if (w<29 || h<9)
		return;

	if (w!=gpSet->wndWidth || h!=gpSet->wndHeight)
	{
		gpSet->wndWidth = w;
		gpSet->wndHeight = h;
	}

	if (ghOpWnd)
	{
		mb_IgnoreEditChanged = TRUE;
		_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", gpSet->wndWidth);
		SetDlgItemText(hMain, tWndWidth, temp);
		_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", gpSet->wndHeight);
		SetDlgItemText(hMain, tWndHeight, temp);
		mb_IgnoreEditChanged = FALSE;
	}
}

// Пожалуй, не будем автоматически менять флажок "Monospace"
// Был вроде Issue, да и не всегда моноширность правильно определяется (DejaVu Sans Mono)
void CSettings::UpdateTTF(BOOL bNewTTF)
{
	if (mb_IgnoreTtfChange) return;
	
	if (!bNewTTF)
	{
		gpSet->isMonospace = bNewTTF ? 0 : isMonospaceSelected;

		if (hMain)
			CheckDlgButton(hMain, cbMonospace, gpSet->isMonospace); // 3state
	}
}

void CSettings::UpdateFontInfo()
{
	if (!ghOpWnd || !hInfo) return;

	wchar_t szTemp[32];
	_wsprintf(szTemp, SKIPLEN(countof(szTemp)) L"%ix%ix%i", LogFont.lfHeight, LogFont.lfWidth, m_tm->tmAveCharWidth);
	SetDlgItemText(hInfo, tRealFontMain, szTemp);
	_wsprintf(szTemp, SKIPLEN(countof(szTemp)) L"%ix%i", LogFont2.lfHeight, LogFont2.lfWidth);
	SetDlgItemText(hInfo, tRealFontBorders, szTemp);
}

void CSettings::Performance(UINT nID, BOOL bEnd)
{
	if (nID == gbPerformance)  //groupbox ctrl id
	{
		if (!gpConEmu->isMainThread())
			return;

		if (ghOpWnd)
		{
			// Performance
			wchar_t sTemp[128];
			//Нихрена это не мегагерцы. Например на "AMD Athlon 64 X2 1999 MHz" здесь отображается "0.004 GHz"
			//swprintf(sTemp, L"Performance counters (%.3f GHz)", ((double)(mn_Freq/1000)/1000000));
			_wsprintf(sTemp, SKIPLEN(countof(sTemp)) L"Performance counters (%I64i)", ((i64)(mn_Freq/1000)));
			SetDlgItemText(hInfo, nID, sTemp);

			for(nID=tPerfFPS; mn_Freq && nID<=tPerfInterval; nID++)
			{
				SendMessage(gpSetCls->hInfo, mn_MsgUpdateCounter, nID-tPerfFPS, 0);
			}
		}

		return;
	}

	if (nID<tPerfFPS || nID>tPerfInterval)
		return;

	if (nID == tPerfFPS)
	{
		i64 tick2 = 0;
		QueryPerformanceCounter((LARGE_INTEGER *)&tick2);
		mn_FPS[mn_FPS_CUR_FRAME] = tick2;
		mn_FPS_CUR_FRAME++;

		if (mn_FPS_CUR_FRAME >= (int)countof(mn_FPS)) mn_FPS_CUR_FRAME = 0;

		if (ghOpWnd)
			PostMessage(gpSetCls->hInfo, mn_MsgUpdateCounter, nID-tPerfFPS, 0);

		return;
	}

	if (nID == tPerfInterval)
	{
		i64 tick2 = 0;
		QueryPerformanceCounter((LARGE_INTEGER *)&tick2);
		mn_RFPS[mn_RFPS_CUR_FRAME] = tick2;
		mn_RFPS_CUR_FRAME++;

		if (mn_RFPS_CUR_FRAME >= (int)countof(mn_RFPS)) mn_RFPS_CUR_FRAME = 0;

		if (ghOpWnd)
			PostMessage(gpSetCls->hInfo, mn_MsgUpdateCounter, nID-tPerfFPS, 0);

		return;
	}

	if (!bEnd)
	{
		QueryPerformanceCounter((LARGE_INTEGER *)&(mn_Counter[nID-tPerfFPS]));
		return;
	}
	else if (!mn_Counter[nID-tPerfFPS] || !mn_Freq)
	{
		return;
	}

	i64 tick2;
	QueryPerformanceCounter((LARGE_INTEGER *)&tick2);
	i64 t = (tick2-mn_Counter[nID-tPerfFPS]);

	if (mn_CounterMax[nID-tPerfFPS]<t ||
	        (GetTickCount()-mn_CounterTick[nID-tPerfFPS])>COUNTER_REFRESH)
	{
		mn_CounterMax[nID-tPerfFPS] = t;
		mn_CounterTick[nID-tPerfFPS] = GetTickCount();

		if (ghOpWnd)
		{
			PostMessage(gpSetCls->hInfo, mn_MsgUpdateCounter, nID-tPerfFPS, 0);
		}
	}
}

void CSettings::RegisterTipsFor(HWND hChildDlg)
{
	if (gpSetCls->hConFontDlg == hChildDlg)
	{
		if (!hwndConFontBalloon || !IsWindow(hwndConFontBalloon))
		{
			hwndConFontBalloon = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
			                                    WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON | TTS_NOPREFIX | TTS_CLOSE,
			                                    CW_USEDEFAULT, CW_USEDEFAULT,
			                                    CW_USEDEFAULT, CW_USEDEFAULT,
			                                    ghOpWnd, NULL,
			                                    g_hInstance, NULL);
			SetWindowPos(hwndConFontBalloon, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
			// Set up tool information.
			// In this case, the "tool" is the entire parent window.
			tiConFontBalloon.cbSize = 44; // был sizeof(TOOLINFO);
			tiConFontBalloon.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
			tiConFontBalloon.hwnd = hChildDlg;
			tiConFontBalloon.hinst = g_hInstance;
			static wchar_t szAsterisk[] = L"*"; // eliminate GCC warning
			tiConFontBalloon.lpszText = szAsterisk;
			tiConFontBalloon.uId = (UINT_PTR)hChildDlg;
			GetClientRect(ghOpWnd, &tiConFontBalloon.rect);
			// Associate the ToolTip with the tool window.
			SendMessage(hwndConFontBalloon, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO) &tiConFontBalloon);
			// Allow multiline
			SendMessage(hwndConFontBalloon, TTM_SETMAXTIPWIDTH, 0, (LPARAM)300);
		}
	}
	else
	{
		if (!hwndBalloon || !IsWindow(hwndBalloon))
		{
			hwndBalloon = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
			                             WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON | TTS_NOPREFIX,
			                             CW_USEDEFAULT, CW_USEDEFAULT,
			                             CW_USEDEFAULT, CW_USEDEFAULT,
			                             ghOpWnd, NULL,
			                             g_hInstance, NULL);
			SetWindowPos(hwndBalloon, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
			// Set up tool information.
			// In this case, the "tool" is the entire parent window.
			tiBalloon.cbSize = 44; // был sizeof(TOOLINFO);
			tiBalloon.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
			tiBalloon.hwnd = hMain;
			tiBalloon.hinst = g_hInstance;
			static wchar_t szAsterisk[] = L"*"; // eliminate GCC warning
			tiBalloon.lpszText = szAsterisk;
			tiBalloon.uId = (UINT_PTR)hMain;
			GetClientRect(ghOpWnd, &tiBalloon.rect);
			// Associate the ToolTip with the tool window.
			SendMessage(hwndBalloon, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO) &tiBalloon);
			// Allow multiline
			SendMessage(hwndBalloon, TTM_SETMAXTIPWIDTH, 0, (LPARAM)300);
		}

		// Create the ToolTip.
		if (!hwndTip || !IsWindow(hwndTip))
		{
			hwndTip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
			                         WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON | TTS_NOPREFIX,
			                         CW_USEDEFAULT, CW_USEDEFAULT,
			                         CW_USEDEFAULT, CW_USEDEFAULT,
			                         ghOpWnd, NULL,
			                         g_hInstance, NULL);
			SetWindowPos(hwndTip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
			SendMessage(hwndTip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 30000);
		}

		if (!hwndTip) return;  // не смогли создать

		HWND hChild = NULL, hEdit = NULL;
		BOOL lbRc = FALSE;
		TCHAR szText[0x200];

		while((hChild = FindWindowEx(hChildDlg, hChild, NULL, NULL)) != NULL)
		{
			LONG wID = GetWindowLong(hChild, GWL_ID);

			if (wID == -1) continue;

			if (LoadString(g_hInstance, wID, szText, countof(szText)))
			{
				// Associate the ToolTip with the tool.
				TOOLINFO toolInfo = { 0 };
				toolInfo.cbSize = 44; //sizeof(toolInfo);
				//GetWindowRect(hChild, &toolInfo.rect); MapWindowPoints(NULL, hChildDlg, (LPPOINT)&toolInfo.rect, 2);
				GetClientRect(hChild, &toolInfo.rect);
				toolInfo.hwnd = hChild; //hChildDlg;
				toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
				toolInfo.uId = (UINT_PTR)hChild;
				toolInfo.lpszText = szText;
				//toolInfo.hinst = g_hInstance;
				//toolInfo.lpszText = (LPTSTR)wID;
				lbRc = SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
				hEdit = FindWindowEx(hChild, NULL, L"Edit", NULL);

				if (hEdit)
				{
					toolInfo.uId = (UINT_PTR)hEdit;
					lbRc = SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
				}

				/*if (wID == tFontFace) {
					toolInfo.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
					toolInfo.hwnd = hMain;
					toolInfo.hinst = g_hInstance;
					toolInfo.lpszText = L"*";
					toolInfo.uId = (UINT_PTR)hMain;
					GetClientRect (ghOpWnd, &toolInfo.rect);
					// Associate the ToolTip with the tool window.
					SendMessage(hwndBalloon, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &toolInfo);
				}*/
			}
		}

		SendMessage(hwndTip, TTM_SETMAXTIPWIDTH, 0, (LPARAM)300);
	}
}

void CSettings::MacroFontSetName(LPCWSTR pszFontName, WORD anHeight /*= 0*/, WORD anWidth /*= 0*/)
{
	LOGFONT LF = LogFont;
	if (pszFontName && *pszFontName)
		wcscpy_c(LF.lfFaceName, pszFontName);
	if (anHeight)
	{
		LF.lfHeight = anHeight;
		LF.lfWidth = anWidth;
	}
	else
	{
		LF.lfWidth = 0;
	}

	if (isAdvLogging)
	{
		char szInfo[128]; _wsprintfA(szInfo, SKIPLEN(countof(szInfo)) "MacroFontSetName('%s', H=%i, W=%i)", LF.lfFaceName, LF.lfHeight, LF.lfWidth);
		gpConEmu->ActiveCon()->RCon()->LogString(szInfo);
	}

	HFONT hf = CreateFontIndirectMy(&LF);

	if (hf)
	{
		// SaveFontSizes выполним после обновления LogFont, т.к. там зовется gpConEmu->OnPanelViewSettingsChanged
		HFONT hOldF = mh_Font[0];
		LogFont = LF;
		mh_Font[0] = hf;
		DeleteObject(hOldF);
		SaveFontSizes(&LF, (mn_AutoFontWidth == -1), true);
		gpConEmu->Update(true);

		if (!gpConEmu->isFullScreen() && !gpConEmu->isZoomed())
			gpConEmu->SyncWindowToConsole();
		else
			gpConEmu->SyncConsoleToWindow();

		gpConEmu->ReSize();
	}

	if (ghOpWnd)
	{
		wchar_t szSize[10];
		_wsprintf(szSize, SKIPLEN(countof(szSize)) L"%i", LF.lfHeight);
		SetDlgItemText(hMain, tFontSizeY, szSize);
		UpdateFontInfo();
		ShowFontErrorTip(gpSetCls->szFontError);
	}

	gpConEmu->OnPanelViewSettingsChanged(TRUE);
}

// Вызывается из диалога настроек
void CSettings::RecreateFont(WORD wFromID)
{
	if (wFromID == tFontFace
	        || wFromID == tFontSizeY
	        || wFromID == tFontSizeX
	        || wFromID == tFontCharset
	        || wFromID == cbBold
	        || wFromID == cbItalic
	        || wFromID == rNoneAA
	        || wFromID == rStandardAA
	        || wFromID == rCTAA
	  )
		mb_IgnoreTtfChange = FALSE;

	LOGFONT LF = {0};
	
	if (ghOpWnd == NULL)
	{
		LF = LogFont;
	}
	else
	{
		LF.lfOutPrecision = OUT_TT_PRECIS;
		LF.lfClipPrecision = CLIP_DEFAULT_PRECIS;
		LF.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
		GetDlgItemText(hMain, tFontFace, LF.lfFaceName, countof(LF.lfFaceName));
		LF.lfHeight = gpSet->FontSizeY = GetNumber(hMain, tFontSizeY);
		LF.lfWidth = gpSet->FontSizeX = GetNumber(hMain, tFontSizeX);
		LF.lfWeight = IsChecked(hMain, cbBold) ? FW_BOLD : FW_NORMAL;
		LF.lfItalic = IsChecked(hMain, cbItalic);
		LF.lfCharSet = gpSet->mn_LoadFontCharSet;

		if (gpSet->mb_CharSetWasSet)
		{
			//gpSet->mb_CharSetWasSet = FALSE;
			INT_PTR newCharSet = SendDlgItemMessage(hMain, tFontCharset, CB_GETCURSEL, 0, 0);

			if (newCharSet != CB_ERR && newCharSet >= 0 && newCharSet < (INT_PTR)countof(SettingsNS::nCharSets))
				LF.lfCharSet = SettingsNS::nCharSets[newCharSet];
			else
				LF.lfCharSet = DEFAULT_CHARSET;
		}

		if (IsChecked(hMain, rNoneAA))
			LF.lfQuality = NONANTIALIASED_QUALITY;
		else if (IsChecked(hMain, rStandardAA))
			LF.lfQuality = ANTIALIASED_QUALITY;
		else if (IsChecked(hMain, rCTAA))
			LF.lfQuality = CLEARTYPE_NATURAL_QUALITY;

		GetDlgItemText(hMain, tFontFace2, LogFont2.lfFaceName, countof(LogFont2.lfFaceName));
		gpSet->FontSizeX2 = GetNumber(hMain, tFontSizeX2);
		gpSet->FontSizeX3 = GetNumber(hMain, tFontSizeX3);

		if (isAdvLogging)
		{
			char szInfo[128]; _wsprintfA(szInfo, SKIPLEN(countof(szInfo)) "AutoRecreateFont(H=%i, W=%i)", LF.lfHeight, LF.lfWidth);
			gpConEmu->ActiveCon()->RCon()->LogString(szInfo);
		}
	}

	HFONT hf = CreateFontIndirectMy(&LF);

	if (hf)
	{
		// SaveFontSizes выполним после обновления LogFont, т.к. там зовется gpConEmu->OnPanelViewSettingsChanged
		HFONT hOldF = mh_Font[0];
		LogFont = LF;
		mh_Font[0] = hf;
		DeleteObject(hOldF);
		SaveFontSizes(&LF, (mn_AutoFontWidth == -1), true);
		gpConEmu->Update(true);

		if (!gpConEmu->isFullScreen() && !gpConEmu->isZoomed())
			gpConEmu->SyncWindowToConsole();
		else
			gpConEmu->SyncConsoleToWindow();

		gpConEmu->ReSize();
	}

	if (ghOpWnd && wFromID == tFontFace)
	{
		wchar_t szSize[10];
		_wsprintf(szSize, SKIPLEN(countof(szSize)) L"%i", LF.lfHeight);
		SetDlgItemText(hMain, tFontSizeY, szSize);
	}

	if (ghOpWnd)
	{
		UpdateFontInfo();
	
		ShowFontErrorTip(gpSetCls->szFontError);
		//tiBalloon.lpszText = gpSetCls->szFontError;
		//if (gpSetCls->szFontError[0]) {
		//	SendMessage(hwndTip, TTM_ACTIVATE, FALSE, 0);
		//	SendMessage(hwndBalloon, TTM_UPDATETIPTEXT, 0, (LPARAM)&tiBalloon);
		//	RECT rcControl; GetWindowRect(GetDlgItem(hMain, tFontFace), &rcControl);
		//	int ptx = rcControl.right - 10;
		//	int pty = (rcControl.top + rcControl.bottom) / 2;
		//	SendMessage(hwndBalloon, TTM_TRACKPOSITION, 0, MAKELONG(ptx,pty));
		//	SendMessage(hwndBalloon, TTM_TRACKACTIVATE, TRUE, (LPARAM)&tiBalloon);
		//	SetTimer(hMain, FAILED_FONT_TIMERID, FAILED_FONT_TIMEOUT, 0);
		//} else {
		//	SendMessage(hwndBalloon, TTM_TRACKACTIVATE, FALSE, (LPARAM)&tiBalloon);
		//	SendMessage(hwndTip, TTM_ACTIVATE, TRUE, 0);
		//}
	}

	//if (wFromID == rNoneAA || wFromID == rStandardAA || wFromID == rCTAA)
	//{
	gpConEmu->OnPanelViewSettingsChanged(TRUE);
	//	//gpConEmu->UpdateGuiInfoMapping();
	//}

	mb_IgnoreTtfChange = TRUE;
}

void CSettings::ShowFontErrorTip(LPCTSTR asInfo)
{
	ShowErrorTip(asInfo, gpSetCls->hMain, tFontFace, gpSetCls->szFontError, countof(gpSetCls->szFontError),
	             gpSetCls->hwndBalloon, &gpSetCls->tiBalloon, gpSetCls->hwndTip, FAILED_FONT_TIMEOUT);
	//if (!asInfo)
	//	gpSetCls->szFontError[0] = 0;
	//else if (asInfo != gpSetCls->szFontError)
	//	lstrcpyn(gpSetCls->szFontError, asInfo, countof(gpSetCls->szFontError));
	//tiBalloon.lpszText = gpSetCls->szFontError;
	//if (gpSetCls->szFontError[0]) {
	//	SendMessage(hwndTip, TTM_ACTIVATE, FALSE, 0);
	//	SendMessage(hwndBalloon, TTM_UPDATETIPTEXT, 0, (LPARAM)&tiBalloon);
	//	RECT rcControl; GetWindowRect(GetDlgItem(hMain, tFontFace), &rcControl);
	//	int ptx = rcControl.right - 10;
	//	int pty = (rcControl.top + rcControl.bottom) / 2;
	//	SendMessage(hwndBalloon, TTM_TRACKPOSITION, 0, MAKELONG(ptx,pty));
	//	SendMessage(hwndBalloon, TTM_TRACKACTIVATE, TRUE, (LPARAM)&tiBalloon);
	//	SetTimer(hMain, FAILED_FONT_TIMERID, FAILED_FONT_TIMEOUT, 0);
	//} else {
	//	SendMessage(hwndBalloon, TTM_TRACKACTIVATE, FALSE, (LPARAM)&tiBalloon);
	//	SendMessage(hwndTip, TTM_ACTIVATE, TRUE, 0);
	//}
}

void CSettings::SaveFontSizes(LOGFONT *pCreated, bool bAuto, bool bSendChanges)
{
	mn_FontWidth = pCreated->lfWidth;
	mn_FontHeight = pCreated->lfHeight;

	if (bAuto)
	{
		mn_AutoFontWidth = pCreated->lfWidth;
		mn_AutoFontHeight = pCreated->lfHeight;
	}

	// Применить в Mapping (там заодно и палитра копируется)
	gpConEmu->OnPanelViewSettingsChanged(bSendChanges);
}

// Вызов из GUI-макросов - увеличить/уменьшить шрифт, без изменения размера (в пикселях) окна
bool CSettings::MacroFontSetSize(int nRelative/*+1/-2*/, int nValue/*1,2,...*/)
{
	// Пытаемся создать новый шрифт
	LOGFONT LF = LogFont;

	if (nRelative == 0)
	{
		// По абсолютному значению (высота шрифта)
		if (nValue < 5)
			return false;

		LF.lfHeight = nValue;
	}
	//else if (nRelative == -1)
	//{
	//	// уменьшить шрифт
	//	LF.lfHeight -= nValue;
	//}
	else if (nRelative == 1)
	{
		if (nValue == 0)
			return false;

		// уменьшить/увеличить шрифт
		LF.lfHeight += nValue;
	}

	// Не должен стать менее 5 пунктов
	if (LF.lfHeight < 5)
		LF.lfHeight = 5;

	// Если задана ширина - подкорректировать
	if (gpSet->FontSizeX && gpSet->FontSizeY)
		LF.lfWidth = LogFont.lfWidth * gpSet->FontSizeY / gpSet->FontSizeX;
	else
		LF.lfWidth = 0;

	if (LF.lfHeight == LogFont.lfHeight && ((LF.lfWidth == LogFont.lfWidth) || (LF.lfWidth == 0)))
		return false;

	for(int nRetry = 0; nRetry < 10; nRetry++)
	{
		HFONT hf = CreateFontIndirectMy(&LF);

		// Успешно, только если шрифт изменился, или хотели поставить абсолютный размер
		if (hf && ((nRelative == 0) || (LF.lfHeight != LogFont.lfHeight)))
		{
			// SaveFontSizes выполним после обновления LogFont, т.к. там зовется gpConEmu->OnPanelViewSettingsChanged
			HFONT hOldF = mh_Font[0];
			LogFont = LF;
			mh_Font[0] = hf;
			DeleteObject(hOldF);
			// Запомнить размер шрифта (AutoFontWidth/Height - может быть другим, он запоминается выше)
			SaveFontSizes(&LF, false, true);
			// Передернуть размер консоли
			gpConEmu->OnSize();
			// Передернуть флажки, что шрифт поменялся
			gpConEmu->Update(true);

			if (ghOpWnd)
			{
				gpSetCls->UpdateFontInfo();

				if (hMain)
				{
					wchar_t temp[16];
					_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", LogFont.lfHeight);
					SelectStringExact(hMain, tFontSizeY, temp);
					_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", gpSet->FontSizeX);
					SelectStringExact(hMain, tFontSizeX, temp);
				}
			}

			return true;
		}

		if (hf)
		{
			DeleteObject(hf);
			hf = NULL;
		}

		if (nRelative == 0)
			return 0;

		// Если пытаются изменить относительный размер, а шрифт не создался - попробовать следующий размер
		//if (nRelative == -1)
		//	LF.lfHeight -= nValue; // уменьшить шрифт
		//else
		if (nRelative == 1)
			LF.lfHeight += nValue; // уменьшить/увеличить шрифт
		else
			return false;

		// Не должен стать менее 5 пунктов
		if (LF.lfHeight < 5)
			return false;

		// Если задана ширина - подкорректировать
		if (LogFont.lfWidth && LogFont.lfHeight)
			LF.lfWidth = LogFont.lfWidth * LF.lfHeight / LogFont.lfHeight;
	}

	return false;
}

// Вызывается при включенном gpSet->isFontAutoSize: подгонка размера шрифта
// под размер окна, без изменения размера в символах
bool CSettings::AutoRecreateFont(int nFontW, int nFontH)
{
	if (mn_AutoFontWidth == nFontW && mn_AutoFontHeight == nFontH)
		return false; // ничего не делали

	if (isAdvLogging)
	{
		char szInfo[128]; _wsprintfA(szInfo, SKIPLEN(countof(szInfo)) "AutoRecreateFont(H=%i, W=%i)", nFontH, nFontW);
		gpConEmu->ActiveCon()->RCon()->LogString(szInfo);
	}

	// Сразу запомним, какой размер просили в последний раз
	mn_AutoFontWidth = nFontW; mn_AutoFontHeight = nFontH;
	// Пытаемся создать новый шрифт
	LOGFONT LF = LogFont;
	LF.lfWidth = nFontW;
	LF.lfHeight = nFontH;
	HFONT hf = CreateFontIndirectMy(&LF);

	if (hf)
	{
		// SaveFontSizes выполним после обновления LogFont, т.к. там зовется gpConEmu->OnPanelViewSettingsChanged
		HFONT hOldF = mh_Font[0];
		LogFont = LF;
		mh_Font[0] = hf;
		DeleteObject(hOldF);
		// Запомнить размер шрифта (AutoFontWidth/Height - может быть другим, он запоминается выше)
		SaveFontSizes(&LF, false, true);
		// Передернуть флажки, что шрифт поменялся
		gpConEmu->Update(true);
		return true;
	}

	return false;
}

bool CSettings::IsAlmostMonospace(LPCWSTR asFaceName, int tmMaxCharWidth, int tmAveCharWidth, int tmHeight)
{
	// Некоторые шрифты (Consolas) достаточно странные. Заявлены как моноширные (PAN_PROP_MONOSPACED),
	// похожи на моноширные, но tmMaxCharWidth у них очень широкий (иероглифы что-ли?)
	if (lstrcmp(asFaceName, L"Consolas") == 0)
		return true;

	// у Arial'а например MaxWidth слишком большой (в два и более раз больше ВЫСОТЫ шрифта)
	bool bAlmostMonospace = false;

	if (tmMaxCharWidth && tmAveCharWidth && tmHeight)
	{
		int nRelativeDelta = (tmMaxCharWidth - tmAveCharWidth) * 100 / tmHeight;

		// Если расхождение менее 16% высоты - считаем шрифт моноширным
		// Увеличил до 16%. Win7, Courier New, 6x4
		if (nRelativeDelta <= 16)
			bAlmostMonospace = true;

		//if (abs(m_tm->tmMaxCharWidth - m_tm->tmAveCharWidth)<=2)
		//{ -- это была попытка прикинуть среднюю ширину по английским буквам
		//  -- не нужно, т.к. затевалось из-за проблем с ClearType на больших размерах
		//  -- шрифтов, а это лечится аргументом pDX в TextOut
		//	int nTestLen = _tcslen(TEST_FONT_WIDTH_STRING_EN);
		//	SIZE szTest = {0,0};
		//	if (GetTextExtentPoint32(hDC, TEST_FONT_WIDTH_STRING_EN, nTestLen, &szTest)) {
		//		int nAveWidth = (szTest.cx + nTestLen - 1) / nTestLen;
		//		if (nAveWidth > m_tm->tmAveCharWidth || nAveWidth > m_tm->tmMaxCharWidth)
		//			m_tm->tmMaxCharWidth = m_tm->tmAveCharWidth = nAveWidth;
		//	}
		//}
	}
	else
	{
		_ASSERTE(tmMaxCharWidth);
		_ASSERTE(tmAveCharWidth);
		_ASSERTE(tmHeight);
	}

	return bAlmostMonospace;
}

// Создать шрифт для отображения символов в диалоге плагина UCharMap
HFONT CSettings::CreateOtherFont(const wchar_t* asFontName)
{
	LOGFONT otherLF = {LogFont.lfHeight};
	otherLF.lfWeight = FW_NORMAL;
	otherLF.lfCharSet = DEFAULT_CHARSET;
	otherLF.lfQuality = LogFont.lfQuality;
	wcscpy_c(otherLF.lfFaceName, asFontName);
	HFONT hf = CreateFontIndirect(&otherLF);
	return hf;
}

// Вызывается из
// -- первичная инициализация
// void CSettings::InitFont(LPCWSTR asFontName/*=NULL*/, int anFontHeight/*=-1*/, int anQuality/*=-1*/)
// -- смена шрифта из фара (через Gui Macro "FontSetName")
// void CSettings::MacroFontSetName(LPCWSTR pszFontName, WORD anHeight /*= 0*/, WORD anWidth /*= 0*/) 
// -- смена _размера_ шрифта из фара (через Gui Macro "FontSetSize")
// bool CSettings::MacroFontSetSize(int nRelative/*+1/-2*/, int nValue/*1,2,...*/)
// -- пересоздание шрифта по изменению контрола окна настроек
// void CSettings::RecreateFont(WORD wFromID)
// -- подгонка шрифта под размер окна GUI (если включен флажок "Auto")
// bool CSettings::AutoRecreateFont(int nFontW, int nFontH)
HFONT CSettings::CreateFontIndirectMy(LOGFONT *inFont)
{
	//ResetFontWidth(); -- перенесено вниз, после того, как убедимся в валидности шрифта
	//lfOutPrecision = OUT_RASTER_PRECIS,
	szFontError[0] = 0;
	HFONT hFont = NULL;
	static int nRastNameLen = _tcslen(RASTER_FONTS_NAME);
	int nRastHeight = 0, nRastWidth = 0;
	bool bRasterFont = false;
	LOGFONT tmpFont = *inFont;
	LPOUTLINETEXTMETRIC lpOutl = NULL;

	if (inFont->lfFaceName[0] == L'[' && wcsncmp(inFont->lfFaceName+1, RASTER_FONTS_NAME, nRastNameLen) == 0)
	{
		if (gpSet->isFontAutoSize)
		{
			gpSet->isFontAutoSize = false;

			if (hMain) CheckDlgButton(hMain, cbFontAuto, BST_UNCHECKED);

			ShowFontErrorTip(szRasterAutoError);
		}

		wchar_t *pszEnd = NULL;
		wchar_t *pszSize = inFont->lfFaceName + nRastNameLen + 2;
		nRastWidth = wcstol(pszSize, &pszEnd, 10);

		if (nRastWidth && pszEnd && *pszEnd == L'x')
		{
			pszSize = pszEnd + 1;
			nRastHeight = wcstol(pszSize, &pszEnd, 10);

			if (nRastHeight)
			{
				bRasterFont = true;
				inFont->lfHeight = gpSet->FontSizeY = nRastHeight;
				inFont->lfWidth = nRastWidth;
				gpSet->FontSizeX = gpSet->FontSizeX3 = nRastWidth;

				if (ghOpWnd && hMain)
				{
					wchar_t temp[32];
					_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", nRastHeight);
					SelectStringExact(hMain, tFontSizeY, temp);
					_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", nRastWidth);
					SelectStringExact(hMain, tFontSizeX, temp);
					SelectStringExact(hMain, tFontSizeX3, temp);
				}
			}
		}

		inFont->lfCharSet = OEM_CHARSET;
		tmpFont = *inFont;
		wcscpy_c(tmpFont.lfFaceName, L"Terminal");
	}

	hFont = CreateFontIndirect(&tmpFont);
	
	
	wchar_t szFontFace[32];
	// лучше для ghWnd, может разные мониторы имеют разные параметры...
	HDC hScreenDC = GetDC(ghWnd); // GetDC(0);
	HDC hDC = CreateCompatibleDC(hScreenDC);
	ReleaseDC(ghWnd, hScreenDC);
	hScreenDC = NULL;
	MBoxAssert(hDC);

	if (hFont)
	{
		DWORD dwFontErr = 0;
		SetLastError(0);
		HFONT hOldF = (HFONT)SelectObject(hDC, hFont);
		dwFontErr = GetLastError();
		// Для пропорциональных шрифтов имеет смысл сохранять в реестре оптимальный lfWidth (это gpSet->FontSizeX3)
		ZeroStruct(m_tm);
		BOOL lbTM = GetTextMetrics(hDC, m_tm);

		if (!lbTM && !bRasterFont)
		{
			// Считаем, что шрифт НЕ валиден!!!
			dwFontErr = GetLastError();
			SelectObject(hDC, hOldF);
			DeleteDC(hDC);
			_wsprintf(gpSetCls->szFontError, SKIPLEN(countof(gpSetCls->szFontError)) L"GetTextMetrics failed for non Raster font '%s'", inFont->lfFaceName);

			if (dwFontErr)
			{
				int nCurLen = _tcslen(gpSetCls->szFontError);
				_wsprintf(gpSetCls->szFontError+nCurLen, SKIPLEN(countof(gpSetCls->szFontError)-nCurLen)
				          L"\r\nErrorCode = 0x%08X", dwFontErr);
			}
			
			DeleteObject(hFont);

			return NULL;
		}

		// Теперь - можно и reset сделать
		ResetFontWidth();

		for(int i=0; i<MAX_FONT_STYLES; i++)
		{
			if (m_otm[i]) {free(m_otm[i]); m_otm[i] = NULL;}
		}

		if (!lbTM)
		{
			_ASSERTE(lbTM);
		}

		if (bRasterFont)
		{
			m_tm->tmHeight = nRastHeight;
			m_tm->tmAveCharWidth = m_tm->tmMaxCharWidth = nRastWidth;
		}

		lpOutl = LoadOutline(hDC, NULL/*hFont*/); // шрифт УЖЕ выбран в DC

		if (lpOutl)
		{
			m_otm[0] = lpOutl; lpOutl = NULL;
		}
		else
		{
			dwFontErr = GetLastError();
		}

		if (GetTextFace(hDC, countof(szFontFace), szFontFace))
		{
			if (!bRasterFont)
			{
				szFontFace[31] = 0;

				if (lstrcmpi(inFont->lfFaceName, szFontFace))
				{
					int nCurLen = _tcslen(szFontError);
					_wsprintf(szFontError+nCurLen, SKIPLEN(countof(szFontError)-nCurLen)
					          L"Failed to create main font!\nRequested: %s\nCreated: %s\n", inFont->lfFaceName, szFontFace);
					lstrcpyn(inFont->lfFaceName, szFontFace, countof(inFont->lfFaceName)); inFont->lfFaceName[countof(inFont->lfFaceName)-1] = 0;
					wcscpy_c(tmpFont.lfFaceName, inFont->lfFaceName);
				}
			}
		}

		// у Arial'а например MaxWidth слишком большой (в два и более раз больше ВЫСОТЫ шрифта)
		bool bAlmostMonospace = IsAlmostMonospace(inFont->lfFaceName, m_tm->tmMaxCharWidth, m_tm->tmAveCharWidth, m_tm->tmHeight);
		//if (m_tm->tmMaxCharWidth && m_tm->tmAveCharWidth && m_tm->tmHeight)
		//{
		//	int nRelativeDelta = (m_tm->tmMaxCharWidth - m_tm->tmAveCharWidth) * 100 / m_tm->tmHeight;
		//	// Если расхождение менее 15% высоты - считаем шрифт моноширным
		//	if (nRelativeDelta < 15)
		//		bAlmostMonospace = true;

		//	//if (abs(m_tm->tmMaxCharWidth - m_tm->tmAveCharWidth)<=2)
		//	//{ -- это была попытка прикинуть среднюю ширину по английским буквам
		//	//  -- не нужно, т.к. затевалось из-за проблем с ClearType на больших размерах
		//	//  -- шрифтов, а это лечится аргументом pDX в TextOut
		//	//	int nTestLen = _tcslen(TEST_FONT_WIDTH_STRING_EN);
		//	//	SIZE szTest = {0,0};
		//	//	if (GetTextExtentPoint32(hDC, TEST_FONT_WIDTH_STRING_EN, nTestLen, &szTest)) {
		//	//		int nAveWidth = (szTest.cx + nTestLen - 1) / nTestLen;
		//	//		if (nAveWidth > m_tm->tmAveCharWidth || nAveWidth > m_tm->tmMaxCharWidth)
		//	//			m_tm->tmMaxCharWidth = m_tm->tmAveCharWidth = nAveWidth;
		//	//	}
		//	//}
		//} else {
		//	_ASSERTE(m_tm->tmMaxCharWidth);
		//	_ASSERTE(m_tm->tmAveCharWidth);
		//	_ASSERTE(m_tm->tmHeight);
		//}

		//if (isForceMonospace) {
		//Maximus - у Arial'а например MaxWidth слишком большой
		if (m_tm->tmMaxCharWidth > (m_tm->tmHeight * 15 / 10))
			m_tm->tmMaxCharWidth = m_tm->tmHeight; // иначе зашкалит - текст очень сильно разъедется

		//inFont->lfWidth = gpSet->FontSizeX3 ? gpSet->FontSizeX3 : m_tm->tmMaxCharWidth;
		// Лучше поставим AveCharWidth. MaxCharWidth для "условно моноширного" Consolas почти равен высоте.
		inFont->lfWidth = gpSet->FontSizeX3 ? gpSet->FontSizeX3 : m_tm->tmAveCharWidth;
		//} else {
		//	// Если указан gpSet->FontSizeX3 (это принудительная ширина знакоместа)
		//    inFont->lfWidth = gpSet->FontSizeX3 ? gpSet->FontSizeX3 : m_tm->tmAveCharWidth;
		//}
		inFont->lfHeight = m_tm->tmHeight; TODO("Здесь нужно обновить реальный размер шрифта в диалоге настройки!");

		if (lbTM && m_tm->tmCharSet != DEFAULT_CHARSET)
		{
			inFont->lfCharSet = m_tm->tmCharSet;

			for (uint i = 0; i < countof(SettingsNS::nCharSets); i++)
			{
				if (SettingsNS::nCharSets[i] == m_tm->tmCharSet)
				{
					SendDlgItemMessage(hMain, tFontCharset, CB_SETCURSEL, i, 0);
					break;
				}
			}
		}

		// если ширина шрифта стала больше ширины FixFarBorders - сбросить его в 0
		if (gpSet->FontSizeX2 && (LONG)gpSet->FontSizeX2 < inFont->lfWidth)
		{
			gpSet->FontSizeX2 = 0;

			if (ghOpWnd && hMain)
				SelectStringExact(hMain, tFontSizeX2, L"0");
		}

		if (ghOpWnd)
		{
			// устанавливать только при листании шрифта в настройке
			TODO("Или через GuiMacro?");
			// при кликах по самому флажку "Monospace" шрифт не пересоздается (CreateFont... не вызывается)
			UpdateTTF(!bAlmostMonospace);    //(m_tm->tmMaxCharWidth - m_tm->tmAveCharWidth)>2
		}

		for(int s = 1; s < MAX_FONT_STYLES; s++)
		{
			if (mh_Font[s]) { DeleteObject(mh_Font[s]); mh_Font[s] = NULL; }

			if (s & AI_STYLE_BOLD)
			{
				tmpFont.lfWeight = (inFont->lfWeight == FW_NORMAL) ? FW_BOLD : FW_NORMAL;
			}
			else
			{
				tmpFont.lfWeight = inFont->lfWeight;
			}

			tmpFont.lfItalic = (s & AI_STYLE_ITALIC) ? !inFont->lfItalic : inFont->lfItalic;
			tmpFont.lfUnderline = (s & AI_STYLE_UNDERLINE) ? !inFont->lfUnderline : inFont->lfUnderline;
			mh_Font[s] = CreateFontIndirect(&tmpFont);
			SelectObject(hDC, mh_Font[s]);
			lbTM = GetTextMetrics(hDC, m_tm+s);
			//_ASSERTE(lbTM);
			lpOutl = LoadOutline(hDC, mh_Font[s]);

			if (lpOutl)
			{
				if (m_otm[s]) free(m_otm[s]);

				m_otm[s] = lpOutl; lpOutl = NULL;
			}
		}

		SelectObject(hDC, hOldF);

		if (mh_Font2) { DeleteObject(mh_Font2); mh_Font2 = NULL; }

		//int width = gpSet->FontSizeX2 ? gpSet->FontSizeX2 : inFont->lfWidth;
		LogFont2.lfWidth = mn_BorderFontWidth = gpSet->FontSizeX2 ? gpSet->FontSizeX2 : inFont->lfWidth;
		LogFont2.lfHeight = abs(inFont->lfHeight);
		// Иначе рамки прерывистыми получаются... поставил NONANTIALIASED_QUALITY
		mh_Font2 = CreateFont(LogFont2.lfHeight, LogFont2.lfWidth, 0, 0, FW_NORMAL,
		                      0, 0, 0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		                      NONANTIALIASED_QUALITY/*ANTIALIASED_QUALITY*/, 0, LogFont2.lfFaceName);

		if (mh_Font2)
		{
			hOldF = (HFONT)SelectObject(hDC, mh_Font2);

			if (GetTextFace(hDC, countof(szFontFace), szFontFace))
			{
				szFontFace[31] = 0;

				if (lstrcmpi(LogFont2.lfFaceName, szFontFace))
				{
					if (szFontError[0]) wcscat_c(szFontError, L"\n");

					int nCurLen = _tcslen(szFontError);
					_wsprintf(szFontError+nCurLen, SKIPLEN(countof(szFontError)-nCurLen)
					          L"Failed to create border font!\nRequested: %s\nCreated: ", LogFont2.lfFaceName);

					if (lstrcmpi(LogFont2.lfFaceName, L"Lucida Console") == 0)
					{
						lstrcpyn(LogFont2.lfFaceName, szFontFace, countof(LogFont2.lfFaceName));
					}
					else
					{
						wcscpy_c(LogFont2.lfFaceName, L"Lucida Console");
						SelectObject(hDC, hOldF);
						DeleteObject(mh_Font2);
						mh_Font2 = CreateFont(LogFont2.lfHeight, LogFont2.lfWidth, 0, 0, FW_NORMAL,
						                      0, 0, 0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
						                      NONANTIALIASED_QUALITY/*ANTIALIASED_QUALITY*/, 0, LogFont2.lfFaceName);
						hOldF = (HFONT)SelectObject(hDC, mh_Font2);
						wchar_t szFontFace2[32];

						if (GetTextFace(hDC, countof(szFontFace2), szFontFace2))
						{
							szFontFace2[31] = 0;

							if (lstrcmpi(LogFont2.lfFaceName, szFontFace2) != 0)
							{
								wcscat_c(szFontError, szFontFace2);
							}
							else
							{
								wcscat_c(szFontError, szFontFace);
								wcscat_c(szFontError, L"\nUsing: Lucida Console");
							}
						}
					}
				}
			}

#ifdef _DEBUG
			DumpFontMetrics(L"mh_Font2", hDC, mh_Font2);
#endif
			SelectObject(hDC, hOldF);
		}

		DeleteDC(hDC);
	}

	return hFont;
}

LPOUTLINETEXTMETRIC CSettings::LoadOutline(HDC hDC, HFONT hFont)
{
	BOOL lbSelfDC = FALSE;

	if (!hDC)
	{
		HDC hScreenDC = GetDC(0);
		hDC = CreateCompatibleDC(hScreenDC);
		lbSelfDC = TRUE;
		ReleaseDC(0, hScreenDC);
	}

	HFONT hOldF = NULL;

	if (hFont)
	{
		hOldF = (HFONT)SelectObject(hDC, hFont);
	}

	LPOUTLINETEXTMETRIC pOut = NULL;
	UINT nSize = GetOutlineTextMetrics(hDC, 0, NULL);

	if (nSize)
	{
		pOut = (LPOUTLINETEXTMETRIC)calloc(nSize,1);

		if (pOut)
		{
			pOut->otmSize = nSize;

			if (!GetOutlineTextMetricsW(hDC, nSize, pOut))
			{
				free(pOut); pOut = NULL;
			}
			else
			{
				pOut->otmpFamilyName = (PSTR)(((LPBYTE)pOut) + (DWORD_PTR)pOut->otmpFamilyName);
				pOut->otmpFaceName = (PSTR)(((LPBYTE)pOut) + (DWORD_PTR)pOut->otmpFaceName);
				pOut->otmpStyleName = (PSTR)(((LPBYTE)pOut) + (DWORD_PTR)pOut->otmpStyleName);
				pOut->otmpFullName = (PSTR)(((LPBYTE)pOut) + (DWORD_PTR)pOut->otmpFullName);
			}
		}
	}

	if (hFont)
	{
		SelectObject(hDC, hOldF);
	}

	if (lbSelfDC)
	{
		DeleteDC(hDC);
	}

	return pOut;
}

void CSettings::DumpFontMetrics(LPCWSTR szType, HDC hDC, HFONT hFont, LPOUTLINETEXTMETRIC lpOutl)
{
	wchar_t szFontFace[32], szFontDump[255];
	TEXTMETRIC ltm;

	if (!hFont)
	{
		_wsprintf(szFontDump, SKIPLEN(countof(szFontDump)) L"*** gpSet->%s: WAS NOT CREATED!\n", szType);
	}
	else
	{
		SelectObject(hDC, hFont); // вернуть шрифт должна вызывающая функция!
		GetTextMetrics(hDC, &ltm);
		GetTextFace(hDC, countof(szFontFace), szFontFace);
		_wsprintf(szFontDump, SKIPLEN(countof(szFontDump)) L"*** gpSet->%s: '%s', Height=%i, Ave=%i, Max=%i, Over=%i, Angle*10=%i\n",
		          szType, szFontFace, ltm.tmHeight, ltm.tmAveCharWidth, ltm.tmMaxCharWidth, ltm.tmOverhang,
		          lpOutl ? lpOutl->otmItalicAngle : 0);
	}

	DEBUGSTRFONT(szFontDump);
}

// FALSE - выключена
// TRUE (BST_CHECKED) - включена
// BST_INDETERMINATE (2) - 3-d state
int CSettings::IsChecked(HWND hParent, WORD nCtrlId)
{
#ifdef _DEBUG
	HWND hChild = GetDlgItem(hParent, nCtrlId);
	_ASSERTE(hChild!=NULL);
#endif
	// Аналог IsDlgButtonChecked
	int nChecked = SendDlgItemMessage(hParent, nCtrlId, BM_GETCHECK, 0, 0);
	_ASSERTE(nChecked==0 || nChecked==1 || nChecked==2);

	if (nChecked!=0 && nChecked!=1 && nChecked!=2)
		nChecked = 0;

	return nChecked;
}

int CSettings::GetNumber(HWND hParent, WORD nCtrlId)
{
#ifdef _DEBUG
	HWND hChild = GetDlgItem(hParent, nCtrlId);
	_ASSERTE(hChild!=NULL);
#endif
	int nValue = 0;
	wchar_t szNumber[32] = {0};

	if (GetDlgItemText(hParent, nCtrlId, szNumber, countof(szNumber)))
	{
		if (!wcscmp(szNumber, L"None"))
			nValue = 255; // 0xFF для gpSet->nFontNormalColor, gpSet->nFontBoldColor, gpSet->nFontItalicColor;
		else
			nValue = klatoi((szNumber[0]==L' ') ? (szNumber+1) : szNumber);
	}

	return nValue;
}

INT_PTR CSettings::GetString(HWND hParent, WORD nCtrlId, wchar_t** ppszStr, LPCWSTR asNoDefault /*= NULL*/)
{
	INT_PTR nLen = SendDlgItemMessage(hParent, nCtrlId, WM_GETTEXTLENGTH, 0, 0);
	if (!ppszStr)
		return nLen;
	
	if (nLen<=0)
	{
		if (*ppszStr) {free(*ppszStr); *ppszStr = NULL;}
	}
	else
	{
		wchar_t* pszNew = (TCHAR*)calloc(nLen+1, sizeof(TCHAR));
		if (!pszNew)
		{
			_ASSERTE(pszNew!=NULL);
		}
		else
		{
			GetDlgItemText(hParent, nCtrlId, pszNew, nLen+1);

			if (*ppszStr)
			{
				if (lstrcmp(*ppszStr, pszNew) == 0)
				{
					free(pszNew);
					return nLen; // Изменений не было
				}
			}

			// Значение "по умолчанию" не запоминаем
			if (asNoDefault && lstrcmp(pszNew, asNoDefault) == 0)
			{
				SafeFree(pszNew);
			}

			if (nLen > (*ppszStr ? (INT_PTR)_tcslen(*ppszStr) : 0))
			{
				if (*ppszStr) free(*ppszStr);
				*ppszStr = pszNew; pszNew = NULL;
			}
			else
			{
				_wcscpy_c(*ppszStr, nLen+1, pszNew);
				SafeFree(pszNew);
			}
		}
	}
	
	return nLen;
}

int CSettings::SelectString(HWND hParent, WORD nCtrlId, LPCWSTR asText)
{
	if (!hParent)  // был ghOpWnd. теперь может быть вызван и для других диалогов!
		return -1;

#ifdef _DEBUG
	HWND hChild = GetDlgItem(hParent, nCtrlId);
	_ASSERTE(hChild!=NULL);
#endif
	// Осуществляет поиск по _началу_ (!) строки
	int nIdx = SendDlgItemMessage(hParent, nCtrlId, CB_SELECTSTRING, -1, (LPARAM)asText);
	return nIdx;
}

// Если nCtrlId==0 - hParent==hList
int CSettings::SelectStringExact(HWND hParent, WORD nCtrlId, LPCWSTR asText)
{
	if (!hParent)  // был ghOpWnd. теперь может быть вызван и для других диалогов!
		return -1;

	HWND hList = nCtrlId ? GetDlgItem(hParent, nCtrlId) : hParent;
	_ASSERTE(hList!=NULL);

	int nIdx = SendMessage(hList, CB_FINDSTRINGEXACT, -1, (LPARAM)asText);

	if (nIdx < 0)
	{
		int nCount = SendMessage(hList, CB_GETCOUNT, 0, 0);
		int nNewVal = _wtol(asText);
		wchar_t temp[MAX_PATH];

		for(int i = 0; i < nCount; i++)
		{
			if (!SendMessage(hList, CB_GETLBTEXT, i, (LPARAM)temp)) break;

			int nCurVal = _wtol(temp);

			if (nCurVal == nNewVal)
			{
				nIdx = i;
				break;
			}
			else if (nCurVal > nNewVal)
			{
				nIdx = SendMessage(hList, CB_INSERTSTRING, i, (LPARAM) asText);
				break;
			}
		}

		if (nIdx < 0)
			nIdx = SendMessage(hList, CB_INSERTSTRING, 0, (LPARAM) asText);
	}

	if (nIdx >= 0)
		SendMessage(hList, CB_SETCURSEL, nIdx, 0);
	else
		SetWindowText(hList, asText);

	return nIdx;
}

// "Умолчательная" высота буфера.
// + ConEmu стартует в буферном режиме
// + команда по умолчанию (если не задана в реестре или ком.строке) будет "cmd", а не "far"
void CSettings::SetArgBufferHeight(int anBufferHeight)
{
	_ASSERTE(anBufferHeight>=0);
	//if (anBufferHeight>=0) gpSet->DefaultBufferHeight = anBufferHeight;
	//ForceBufferHeight = (gpSet->DefaultBufferHeight != 0);
	bForceBufferHeight = true;
	nForceBufferHeight = anBufferHeight;
	wcscpy_c(szDefCmd, (anBufferHeight==0) ? L"far" : L"cmd");
}

LPCTSTR CSettings::GetDefaultCmd()
{
	_ASSERTE(szDefCmd[0]!=0);
	return szDefCmd;
}

void CSettings::ResetCmdArg()
{
	SingleInstanceArg = false;
	// Сбросить нужно только gpSet->psCurCmd, gpSet->psCmd не меняется - загружается только из настройки
	SafeFree(gpSet->psCurCmd);
	//SettingsBase* reg = CreateSettings();
	//if (reg->OpenKey(Config, KEY_READ))
	//{
	//	reg->Load(L"CmdLine", &gpSet->psCmd);
	//    reg->CloseKey();
	//}
	//delete reg;
}

//LPCTSTR CSettings::GetCmd()
//{
//	if (gpSet->psCurCmd && *gpSet->psCurCmd)
//		return gpSet->psCurCmd;
//
//	if (gpSet->psCmd && *gpSet->psCmd)
//		return gpSet->psCmd;
//
//	SafeFree(gpSet->psCurCmd); // впринципе, эта строка скорее всего не нужна, но на всякий случай...
//	// Хорошо бы более корректно определить версию фара, но это не всегда просто
//	// Например x64 файл сложно обработать в x86 ConEmu.
//	DWORD nFarSize = 0;
//
//	if (lstrcmpi(gpSet->szDefCmd, L"far") == 0)
//	{
//		// Ищем фар. (1) В папке ConEmu, (2) в текущей директории, (2) на уровень вверх от папки ConEmu
//		wchar_t szFar[MAX_PATH*2], *pszSlash;
//		szFar[0] = L'"';
//		wcscpy_add(1, szFar, gpConEmu->ms_ConEmuExeDir); // Теперь szFar содержит путь запуска программы
//		pszSlash = szFar + _tcslen(szFar);
//		_ASSERTE(pszSlash > szFar);
//		BOOL lbFound = FALSE;
//
//		// (1) В папке ConEmu
//		if (!lbFound)
//		{
//			wcscpy_add(pszSlash, szFar, L"\\Far.exe");
//
//			if (FileExists(szFar+1, &nFarSize))
//				lbFound = TRUE;
//		}
//
//		// (2) в текущей директории
//		if (!lbFound && lstrcmpi(gpConEmu->ms_ConEmuCurDir, gpConEmu->ms_ConEmuExeDir))
//		{
//			szFar[0] = L'"';
//			wcscpy_add(1, szFar, gpConEmu->ms_ConEmuCurDir);
//			wcscat_add(1, szFar, L"\\Far.exe");
//
//			if (FileExists(szFar+1, &nFarSize))
//				lbFound = TRUE;
//		}
//
//		// (3) на уровень вверх
//		if (!lbFound)
//		{
//			szFar[0] = L'"';
//			wcscpy_add(1, szFar, gpConEmu->ms_ConEmuExeDir);
//			pszSlash = szFar + _tcslen(szFar);
//			*pszSlash = 0;
//			pszSlash = wcsrchr(szFar, L'\\');
//
//			if (pszSlash)
//			{
//				wcscpy_add(pszSlash+1, szFar, L"Far.exe");
//
//				if (FileExists(szFar+1, &nFarSize))
//					lbFound = TRUE;
//			}
//		}
//
//		if (lbFound)
//		{
//			// 110124 - нафиг, если пользователю надо - сам или параметр настроит, или реестр
//			//// far чаще всего будет установлен в "Program Files", поэтому для избежания проблем - окавычиваем
//			//// Пока тупо - если far.exe > 1200K - считаем, что это Far2
//			//wcscat_c(szFar, (nFarSize>1228800) ? L"\" /w" : L"\"");
//			wcscat_c(szFar, L"\"");
//		}
//		else
//		{
//			// Если Far.exe не найден рядом с ConEmu - запустить cmd.exe
//			wcscpy_c(szFar, L"cmd");
//		}
//
//		// Finally - Result
//		gpSet->psCurCmd = lstrdup(szFar);
//	}
//	else
//	{
//		// Simple Copy
//		gpSet->psCurCmd = lstrdup(gpSet->szDefCmd);
//	}
//
//	return gpSet->psCurCmd;
//}

LPCWSTR CSettings::FontFaceName()
{
	return LogFont.lfFaceName;
}

LONG CSettings::FontWidth()
{
	if (!LogFont.lfWidth)
	{
		_ASSERTE(LogFont.lfWidth!=0);
		return 8;
	}

	_ASSERTE(gpSetCls->mn_FontWidth==LogFont.lfWidth);
	return gpSetCls->mn_FontWidth;
}

LONG CSettings::FontHeight()
{
	if (!LogFont.lfHeight)
	{
		_ASSERTE(LogFont.lfHeight!=0);
		return 12;
	}

	_ASSERTE(gpSetCls->mn_FontHeight==LogFont.lfHeight);
	return gpSetCls->mn_FontHeight;
}

BOOL CSettings::FontBold()
{
	return LogFont.lfWeight>400;
}

BOOL CSettings::FontItalic()
{
	return LogFont.lfItalic!=0;
}

BOOL CSettings::FontClearType()
{
	return (LogFont.lfQuality!=NONANTIALIASED_QUALITY);
}

BYTE CSettings::FontQuality()
{
	return LogFont.lfQuality;
}

LPCWSTR CSettings::BorderFontFaceName()
{
	return LogFont2.lfFaceName;
}

LONG CSettings::BorderFontWidth()
{
	_ASSERTE(LogFont2.lfWidth);
	_ASSERTE(gpSetCls->mn_BorderFontWidth==LogFont2.lfWidth);
	return gpSetCls->mn_BorderFontWidth;
}

BYTE CSettings::FontCharSet()
{
	return LogFont.lfCharSet;
}

void CSettings::RegisterTabs()
{
	if (!mb_TabHotKeyRegistered)
	{
		if (RegisterHotKey(ghOpWnd, 0x101, MOD_CONTROL, VK_TAB))
			mb_TabHotKeyRegistered = TRUE;

		RegisterHotKey(ghOpWnd, 0x102, MOD_CONTROL|MOD_SHIFT, VK_TAB);
	}
}

void CSettings::UnregisterTabs()
{
	if (mb_TabHotKeyRegistered)
	{
		UnregisterHotKey(ghOpWnd, 0x101);
		UnregisterHotKey(ghOpWnd, 0x102);
	}

	mb_TabHotKeyRegistered = FALSE;
}

// Если asFontFile НЕ NULL - значит его пользователь указал через /fontfile
BOOL CSettings::RegisterFont(LPCWSTR asFontFile, BOOL abDefault)
{
	// Обработка параметра /fontfile
	_ASSERTE(asFontFile && *asFontFile);

	if (mb_StopRegisterFonts) return FALSE;

	for (std::vector<RegFont>::iterator iter = m_RegFonts.begin(); iter != m_RegFonts.end(); ++iter)
	{
		if (StrCmpI(iter->szFontFile, asFontFile) == 0)
		{
			// Уже добавлено
			if (abDefault && iter->bDefault == FALSE) iter->bDefault = TRUE;

			return TRUE;
		}
	}

	RegFont rf = {abDefault};
	wchar_t szFullFontName[LF_FACESIZE];

	if (!GetFontNameFromFile(asFontFile, rf.szFontName, szFullFontName))
	{
		//DWORD dwErr = GetLastError();
		size_t cchLen = _tcslen(asFontFile)+100;
		wchar_t* psz=(wchar_t*)calloc(cchLen,sizeof(wchar_t));
		_wcscpy_c(psz, cchLen, L"Can't retrieve font family from file:\n");
		_wcscat_c(psz, cchLen, asFontFile);
		_wcscat_c(psz, cchLen, L"\nContinue?");
		int nBtn = MessageBox(NULL, psz, gpConEmu->GetDefaultTitle(), MB_OKCANCEL|MB_ICONSTOP);
		free(psz);

		if (nBtn == IDCANCEL)
		{
			mb_StopRegisterFonts = TRUE;
			return FALSE;
		}

		return TRUE; // продолжить со следующим файлом
	}
	else if (rf.szFontName[0] == 1 && rf.szFontName[1] == 0)
	{
		return TRUE;
	}

	// Проверить, может такой шрифт уже зарегистрирован в системе
	BOOL lbRegistered = FALSE, lbOneOfFam = FALSE;

	for (std::vector<RegFont>::iterator iter = m_RegFonts.begin(); iter != m_RegFonts.end(); ++iter)
	{
		// Это может быть другой тип шрифта (Liberation Mono Bold, Liberation Mono Regular, ...)
		if (lstrcmpi(iter->szFontName, rf.szFontName) == 0
			|| lstrcmpi(iter->szFontName, szFullFontName) == 0)
		{
			lbRegistered = iter->bAlreadyInSystem; lbOneOfFam = TRUE; break;
		}
	}

	if (!lbOneOfFam)
	{
		// Проверяем, может в системе уже зарегистрирован такой шрифт?
		LOGFONT LF = {0};
		LF.lfOutPrecision = OUT_TT_PRECIS; LF.lfClipPrecision = CLIP_DEFAULT_PRECIS; LF.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
		wcscpy_c(LF.lfFaceName, rf.szFontName); LF.lfHeight = 10; LF.lfWeight = FW_NORMAL;
		HFONT hf = CreateFontIndirect(&LF);
		BOOL lbFail = FALSE;

		if (hf)
		{
			LPOUTLINETEXTMETRICW lpOutl = gpSetCls->LoadOutline(NULL, hf);

			if (lpOutl)
			{
				if (lstrcmpi((wchar_t*)lpOutl->otmpFamilyName, rf.szFontName) == 0)
					lbRegistered = TRUE;
				else
					lbFail = TRUE;

				free(lpOutl);
			}

			DeleteObject(hf);
		}

		if ((!hf || lbFail) && (lstrcmp(rf.szFontName, szFullFontName) != 0))
		{
			wcscpy_c(LF.lfFaceName, szFullFontName);
			hf = CreateFontIndirect(&LF);
			LPOUTLINETEXTMETRICW lpOutl = gpSetCls->LoadOutline(NULL, hf);

			if (lpOutl)
			{
				if (lstrcmpi((wchar_t*)lpOutl->otmpFamilyName, rf.szFontName) == 0)
				{
					// Таки создавать нужно по полному имени
					wcscpy_c(rf.szFontName, szFullFontName);
					lbRegistered = TRUE;
				}

				free(lpOutl);
			}

			DeleteObject(hf);
		}
	}

	// Запомним, что такое имя шрифта в системе уже есть, но зарегистрируем. Может в этом файле какие-то модификации...
	rf.bAlreadyInSystem = lbRegistered;

	if (!AddFontResourceEx(asFontFile, FR_PRIVATE, NULL))  //ADD fontname; by Mors
	{
		size_t cchLen = _tcslen(asFontFile)+100;
		wchar_t* psz=(wchar_t*)calloc(cchLen,sizeof(wchar_t));
		_wcscpy_c(psz, cchLen, L"Can't register font:\n");
		_wcscat_c(psz, cchLen, asFontFile);
		_wcscat_c(psz, cchLen, L"\nContinue?");
		int nBtn = MessageBox(NULL, psz, gpConEmu->GetDefaultTitle(), MB_OKCANCEL|MB_ICONSTOP);
		free(psz);

		if (nBtn == IDCANCEL)
		{
			mb_StopRegisterFonts = TRUE;
			return FALSE;
		}

		return TRUE; // продолжить со следующим файлом
	}

	wcscpy_c(rf.szFontFile, asFontFile);
	// Теперь его нужно добавить в вектор независимо от успешности определения рамок
	// будет нужен RemoveFontResourceEx(asFontFile, FR_PRIVATE, NULL);
	// Определить наличие рамок и "юникодности" шрифта
	HDC hdc = CreateCompatibleDC(0);

	if (hdc)
	{
		BOOL lbFail = FALSE;
		HFONT hf = CreateFont(18, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
		                      OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		                      NONANTIALIASED_QUALITY/*ANTIALIASED_QUALITY*/, 0,
		                      rf.szFontName);

		wchar_t szDbg[1024]; szDbg[0] = 0;
		if (hf)
		{
			
			LPOUTLINETEXTMETRICW lpOutl = gpSetCls->LoadOutline(NULL, hf);
			if (lpOutl)
			{
				if (lstrcmpi((wchar_t*)lpOutl->otmpFamilyName, rf.szFontName) != 0)
				{
					
					_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"!!! RegFont failed: '%s'. Req: %s, Created: %s\n",
						asFontFile, rf.szFontName, (wchar_t*)lpOutl->otmpFamilyName);
					lbFail = TRUE;
				}
				free(lpOutl);
			}
		}

		// Попробовать по полному имени?
		if ((!hf || lbFail) && (lstrcmp(rf.szFontName, szFullFontName) != 0))
		{
			if (hf)
				DeleteObject(hf);
			hf = CreateFont(18, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
		                      OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		                      NONANTIALIASED_QUALITY/*ANTIALIASED_QUALITY*/, 0,
		                      szFullFontName);
			LPOUTLINETEXTMETRICW lpOutl = gpSetCls->LoadOutline(NULL, hf);

			if (lpOutl)
			{
				if (lstrcmpi((wchar_t*)lpOutl->otmpFamilyName, szFullFontName) == 0)
				{
					// Таки создавать нужно по полному имени
					wcscpy_c(rf.szFontName, szFullFontName);
					lbFail = FALSE;
				}

				free(lpOutl);
			}
		}

		// При обломе шрифт таки зарегистрируем, но как умолчание чего-либо брать не будем
		if (hf && lbFail)
		{
			DeleteObject(hf);
			hf = NULL;
		}
		if (lbFail && *szDbg)
		{
			// Показать в отладчике что стряслось
			OutputDebugString(szDbg);
		}

		if (hf)
		{
			HFONT hOldF = (HFONT)SelectObject(hdc, hf);
			LPGLYPHSET pSets = NULL;
			DWORD nSize = GetFontUnicodeRanges(hdc, NULL);

			if (nSize)
			{
				pSets = (LPGLYPHSET)calloc(nSize,1);

				if (pSets)
				{
					pSets->cbThis = nSize;

					if (GetFontUnicodeRanges(hdc, pSets))
					{
						rf.bUnicode = (pSets->flAccel != 1/*GS_8BIT_INDICES*/);

						// Поиск рамок
						if (rf.bUnicode)
						{
							for(DWORD r = 0; r < pSets->cRanges; r++)
							{
								if (pSets->ranges[r].wcLow < ucBoxDblDownRight
								        && (pSets->ranges[r].wcLow + pSets->ranges[r].cGlyphs - 1) > ucBoxDblDownRight)
								{
									rf.bHasBorders = TRUE; break;
								}
							}
						}
						else
						{
							_ASSERTE(rf.bUnicode);
						}
					}

					free(pSets);
				}
			}

			SelectObject(hdc, hOldF);
			DeleteObject(hf);
		}

		DeleteDC(hdc);
	}

	// Запомнить шрифт
	m_RegFonts.push_back(rf);
	return TRUE;
}

void CSettings::RegisterFonts()
{
	if (!gpSet->isAutoRegisterFonts)
		return; // Если поиск шрифтов не требуется

	// Сначала - регистрация шрифтов в папке программы
	RegisterFontsInt(gpConEmu->ms_ConEmuExeDir);

	// Если папка запуска отличается от папки программы
	if (lstrcmpW(gpConEmu->ms_ConEmuExeDir, gpConEmu->ms_ConEmuBaseDir))
		RegisterFontsInt(gpConEmu->ms_ConEmuBaseDir); // зарегистрировать шрифты и из базовой папки

	// Если папка запуска отличается от папки программы
	if (lstrcmpiW(gpConEmu->ms_ConEmuExeDir, gpConEmu->ms_ConEmuCurDir))
	{
		BOOL lbSkipCurDir = FALSE;
		wchar_t szFontsDir[MAX_PATH+1];

		if (SHGetSpecialFolderPath(ghWnd, szFontsDir, CSIDL_FONTS, FALSE))
		{
			// Oops, папка запуска совпала с системной папкой шрифтов?
			lbSkipCurDir = (lstrcmpiW(szFontsDir, gpConEmu->ms_ConEmuCurDir) == 0);
		}

		if (!lbSkipCurDir)
		{
			// зарегистрировать шрифты и из папки запуска
			RegisterFontsInt(gpConEmu->ms_ConEmuCurDir);
		}
	}

	// Теперь можно смотреть, зарегистрились ли какие-то шрифты... И выбрать из них подходящие
	// Это делается в InitFont
}

void CSettings::RegisterFontsInt(LPCWSTR asFromDir)
{
	// Регистрация шрифтов в папке ConEmu
	WIN32_FIND_DATA fnd;
	wchar_t szFind[MAX_PATH*2]; wcscpy_c(szFind, asFromDir); // БЕЗ завершающего слеша!
	wchar_t* pszSlash = szFind + lstrlenW(szFind);
	wcscpy_add(pszSlash, szFind, L"\\*.*");
	HANDLE hFind = FindFirstFile(szFind, &fnd);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				pszSlash[1] = 0;

				TCHAR* pszDot = _tcsrchr(fnd.cFileName, _T('.'));
				// Неизвестные расширения - пропускаем
				TODO("Register *.fon font files"); // Формат шрифта разобран в ImpEx
				if (!pszDot || (lstrcmpi(pszDot, _T(".ttf")) && lstrcmpi(pszDot, _T(".otf")) /*&& lstrcmpi(pszDot, _T(".fon"))*/))
					continue;

				if ((_tcslen(fnd.cFileName)+_tcslen(szFind)) >= MAX_PATH)
				{
					size_t cchLen = _tcslen(fnd.cFileName)+100;
					wchar_t* psz=(wchar_t*)calloc(cchLen,sizeof(wchar_t));
					_wcscpy_c(psz, cchLen, L"Too long full pathname for font:\n");
					_wcscat_c(psz, cchLen, fnd.cFileName);
					int nBtn = MessageBox(NULL, psz, gpConEmu->GetDefaultTitle(), MB_OKCANCEL|MB_ICONSTOP);
					free(psz);

					if (nBtn == IDCANCEL) break;
					continue;
				}


				wcscat_c(szFind, fnd.cFileName);

				// Возвращает FALSE если произошла ошибка и юзер сказал "Не продолжать"
				if (!RegisterFont(szFind, FALSE))
					break;
			}
		}
		while(FindNextFile(hFind, &fnd));

		FindClose(hFind);
	}
}

void CSettings::UnregisterFonts()
{
	for(std::vector<RegFont>::iterator iter = m_RegFonts.begin();
	        iter != m_RegFonts.end(); iter = m_RegFonts.erase(iter))
	{
		RemoveFontResourceEx(iter->szFontFile, FR_PRIVATE, NULL);
	}
}

BOOL CSettings::GetFontNameFromFile(LPCTSTR lpszFilePath, wchar_t (&rsFontName)[LF_FACESIZE], wchar_t (&rsFullFontName)[LF_FACESIZE])
{
	LPCTSTR pszDot = _tcsrchr(lpszFilePath, _T('.'));
	// Неизвестные расширения - пропускаем
	if (!pszDot)
		return FALSE;

	if (!lstrcmpi(pszDot, _T(".ttf")))
		return GetFontNameFromFile_TTF(lpszFilePath, rsFontName, rsFullFontName);

	if (!lstrcmpi(pszDot, _T(".otf")))
		return GetFontNameFromFile_OTF(lpszFilePath, rsFontName, rsFullFontName);

	TODO("*.fon files");

	return FALSE;
}

#define SWAPWORD(x)		MAKEWORD(HIBYTE(x), LOBYTE(x))
#define SWAPLONG(x)		MAKELONG(SWAPWORD(HIWORD(x)), SWAPWORD(LOWORD(x)))

BOOL CSettings::GetFontNameFromFile_TTF(LPCTSTR lpszFilePath, wchar_t (&rsFontName)[LF_FACESIZE], wchar_t (&rsFullFontName)[LF_FACESIZE])
{
	struct TT_OFFSET_TABLE
	{
		USHORT	uMajorVersion;
		USHORT	uMinorVersion;
		USHORT	uNumOfTables;
		USHORT	uSearchRange;
		USHORT	uEntrySelector;
		USHORT	uRangeShift;
	};
	struct TT_TABLE_DIRECTORY
	{
		char	szTag[4];			//table name //-V112
		ULONG	uCheckSum;			//Check sum
		ULONG	uOffset;			//Offset from beginning of file
		ULONG	uLength;			//length of the table in bytes
	};
	struct TT_NAME_TABLE_HEADER
	{
		USHORT	uFSelector;			//format selector. Always 0
		USHORT	uNRCount;			//Name Records count
		USHORT	uStorageOffset;		//Offset for strings storage, from start of the table
	};
	struct TT_NAME_RECORD
	{
		USHORT	uPlatformID;
		USHORT	uEncodingID;
		USHORT	uLanguageID;
		USHORT	uNameID;
		USHORT	uStringLength;
		USHORT	uStringOffset;	//from start of storage area
	};
	
	BOOL lbRc = FALSE;
	HANDLE f = NULL;
	wchar_t szRetVal[MAX_PATH];
	DWORD dwRead;
	TT_OFFSET_TABLE ttOffsetTable;
	TT_TABLE_DIRECTORY tblDir;
	BOOL bFound = FALSE;

	//if (f.Open(lpszFilePath, CFile::modeRead|CFile::shareDenyWrite)){
	if ((f = CreateFile(lpszFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
		goto wrap;

	//f.Read(&ttOffsetTable, sizeof(TT_OFFSET_TABLE));
	if (!ReadFile(f, &ttOffsetTable, sizeof(TT_OFFSET_TABLE), &(dwRead=0), NULL) || (dwRead != sizeof(TT_OFFSET_TABLE)))
		goto wrap;

	ttOffsetTable.uNumOfTables = SWAPWORD(ttOffsetTable.uNumOfTables);
	ttOffsetTable.uMajorVersion = SWAPWORD(ttOffsetTable.uMajorVersion);
	ttOffsetTable.uMinorVersion = SWAPWORD(ttOffsetTable.uMinorVersion);

	//check is this is a true type font and the version is 1.0
	if (ttOffsetTable.uMajorVersion != 1 || ttOffsetTable.uMinorVersion != 0)
		goto wrap;


	for (int i = 0; i < ttOffsetTable.uNumOfTables; i++)
	{
		//f.Read(&tblDir, sizeof(TT_TABLE_DIRECTORY));
		if (ReadFile(f, &tblDir, sizeof(TT_TABLE_DIRECTORY), &(dwRead=0), NULL) && dwRead)
		{
			//strncpy(szRetVal, tblDir.szTag, 4); szRetVal[4] = 0;
			//if (lstrcmpi(szRetVal, L"name") == 0)
			//if (memcmp(tblDir.szTag, "name", 4) == 0)
			if (strnicmp(tblDir.szTag, "name", 4) == 0) //-V112
			{
				bFound = TRUE;
				tblDir.uLength = SWAPLONG(tblDir.uLength);
				tblDir.uOffset = SWAPLONG(tblDir.uOffset);
				break;
			}
		}
	}

	if (bFound)
	{
		if (SetFilePointer(f, tblDir.uOffset, NULL, FILE_BEGIN)!=INVALID_SET_FILE_POINTER)
		{
			TT_NAME_TABLE_HEADER ttNTHeader;

			//f.Read(&ttNTHeader, sizeof(TT_NAME_TABLE_HEADER));
			if (ReadFile(f, &ttNTHeader, sizeof(TT_NAME_TABLE_HEADER), &(dwRead=0), NULL) && dwRead)
			{
				ttNTHeader.uNRCount = SWAPWORD(ttNTHeader.uNRCount);
				ttNTHeader.uStorageOffset = SWAPWORD(ttNTHeader.uStorageOffset);
				TT_NAME_RECORD ttRecord;
				bFound = FALSE;

				for (int i = 0; i < ttNTHeader.uNRCount; i++)
				{
					//f.Read(&ttRecord, sizeof(TT_NAME_RECORD));
					if (ReadFile(f, &ttRecord, sizeof(TT_NAME_RECORD), &(dwRead=0), NULL) && dwRead)
					{
						ttRecord.uNameID = SWAPWORD(ttRecord.uNameID);

						if (ttRecord.uNameID == 1)
						{
							ttRecord.uStringLength = SWAPWORD(ttRecord.uStringLength);
							ttRecord.uStringOffset = SWAPWORD(ttRecord.uStringOffset);
							//int nPos = f.GetPosition();
							DWORD nPos = SetFilePointer(f, 0, 0, FILE_CURRENT);

							//f.Seek(tblDir.uOffset + ttRecord.uStringOffset + ttNTHeader.uStorageOffset, CFile::begin);
							if (SetFilePointer(f, tblDir.uOffset + ttRecord.uStringOffset + ttNTHeader.uStorageOffset, 0, FILE_BEGIN)!=INVALID_SET_FILE_POINTER)
							{
								if ((ttRecord.uStringLength + 1) < 33)
								{
									//f.Read(csTemp.GetBuffer(ttRecord.uStringLength + 1), ttRecord.uStringLength);
									//csTemp.ReleaseBuffer();
									char szName[MAX_PATH]; szName[ttRecord.uStringLength + 1] = 0;

									if (ReadFile(f, szName, ttRecord.uStringLength + 1, &(dwRead=0), NULL) && dwRead)
									{
										//if (csTemp.GetLength() > 0){
										if (szName[0])
										{
											for (int j = ttRecord.uStringLength; j >= 0 && szName[j] == ' '; j--)
												szName[j] = 0;
											szName[ttRecord.uStringLength] = 0;

											if (szName[0])
											{
												MultiByteToWideChar(CP_ACP, 0, szName, -1, szRetVal, LF_FACESIZE); //-V112
												szRetVal[31] = 0;
												lbRc = TRUE;
											}

											break;
										}
									}
								}
							}

							//f.Seek(nPos, CFile::begin);
							SetFilePointer(f, nPos, 0, FILE_BEGIN);
						}
					}
				}
			}
		}
	}

	if (lbRc)
	{
		wcscpy_c(rsFontName, szRetVal);
		wcscpy_c(rsFullFontName, szRetVal);
	}
	
wrap:
	if (f && (f != INVALID_HANDLE_VALUE))
		CloseHandle(f);
	return lbRc;
}

// Retrieve Family name from OTF file
BOOL CSettings::GetFontNameFromFile_OTF(LPCTSTR lpszFilePath, wchar_t (&rsFontName)[LF_FACESIZE], wchar_t (&rsFullFontName)[LF_FACESIZE])
{
	struct OTF_ROOT
	{
		char  szTag[4]; // 0x00010000 или 'OTTO'
		WORD  NumTables;
		WORD  SearchRange;
		WORD  EntrySelector;
		WORD  RangeShift;
	};
	struct OTF_TABLE
	{
		char  szTag[4]; // нас интересует только 'name'
		DWORD CheckSum;
		DWORD Offset; // <== начало таблицы, от начала файла
		DWORD Length;
	};
	struct OTF_NAME_TABLE
	{
		WORD  Format; // = 0
		WORD  Count;
		WORD  StringOffset; // <== начало строк, в байтах, от начала таблицы
	};
	struct OTF_NAME_RECORD
	{
		WORD  PlatformID;
		WORD  EncodingID;
		WORD  LanguageID;
		WORD  NameID; // нас интересует 4=Full font name, или (1+' '+2)=(Font Family name + Font Subfamily name)
		WORD  Length; // in BYTES
		WORD  Offset; // in BYTES from start of storage area
	};
	
	//-- можно вернуть так, чтобы "по тихому" пропустить этот файл
	//rsFontName[0] = 1;
	//rsFontName[1] = 0;
	
	
	BOOL lbRc = FALSE;
	HANDLE f = NULL;
	wchar_t szFullName[MAX_PATH] = {}, szName[128] = {}, szSubName[128] = {};
	char szData[MAX_PATH] = {};
	int iFullOffset = -1, iFamOffset = -1, iSubFamOffset = -1;
	int iFullLength = -1, iFamLength = -1, iSubFamLength = -1;
	DWORD dwRead;
	OTF_ROOT root;
	OTF_TABLE tbl;
	OTF_NAME_TABLE nam;
	OTF_NAME_RECORD namRec;
	BOOL bFound = FALSE;

	if ((f = CreateFile(lpszFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
		goto wrap;

	//f.Read(&ttOffsetTable, sizeof(TT_OFFSET_TABLE));
	if (!ReadFile(f, &root, sizeof(root), &(dwRead=0), NULL) || (dwRead != sizeof(root)))
		goto wrap;
		
	root.NumTables = SWAPWORD(root.NumTables);
	
	if (strnicmp(root.szTag, "OTTO", 4) != 0) //-V112
		goto wrap; // Не поддерживается


	for (WORD i = 0; i < root.NumTables; i++)
	{
		//f.Read(&tblDir, sizeof(TT_TABLE_DIRECTORY));
		if (ReadFile(f, &tbl, sizeof(tbl), &(dwRead=0), NULL) && dwRead)
		{
			if (strnicmp(tbl.szTag, "name", 4) == 0) //-V112
			{
				bFound = TRUE;
				tbl.Length = SWAPLONG(tbl.Length);
				tbl.Offset = SWAPLONG(tbl.Offset);
				break;
			}
		}
	}

	if (bFound)
	{
		if (SetFilePointer(f, tbl.Offset, NULL, FILE_BEGIN)!=INVALID_SET_FILE_POINTER)
		{
			if (ReadFile(f, &nam, sizeof(nam), &(dwRead=0), NULL) && dwRead)
			{
				nam.Format = SWAPWORD(nam.Format);
				nam.Count = SWAPWORD(nam.Count);
				nam.StringOffset = SWAPWORD(nam.StringOffset);
				if (nam.Format != 0 || !nam.Count)
					goto wrap; // Неизвестный формат
				
				bFound = FALSE;

				for (int i = 0; i < nam.Count; i++)
				{
					if (ReadFile(f, &namRec, sizeof(namRec), &(dwRead=0), NULL) && dwRead)
					{
						namRec.NameID = SWAPWORD(namRec.NameID);
						namRec.Offset = SWAPWORD(namRec.Offset);
						namRec.Length = SWAPWORD(namRec.Length);

						switch (namRec.NameID)
						{
						case 1:
							iFamOffset = namRec.Offset;
							iFamLength = namRec.Length;
							break;
						case 2:
							iSubFamOffset = namRec.Offset;
							iSubFamLength = namRec.Length;
							break;
						case 4:
							iFullOffset = namRec.Offset;
							iFullLength = namRec.Length;
							break;
						}
					}

					if (iFamOffset != -1 && iSubFamOffset != -1 && iFullOffset != -1)
						break;
				}

				for (int n = 0; n < 3; n++)	
				{
					int iOffset, iLen;
					switch (n)
					{
					case 0:
						if (iFullOffset == -1)
							continue;
						iOffset = iFullOffset; iLen = iFullLength;
						break;
					case 1:
						if (iFamOffset == -1)
							continue;
						iOffset = iFamOffset; iLen = iFamLength;
						break;
					//case 2:
					default:
						if (iSubFamOffset == -1)
							continue;
						iOffset = iSubFamOffset; iLen = iSubFamLength;
						//break;
					}
					if (SetFilePointer(f, tbl.Offset + nam.StringOffset + iOffset, NULL, FILE_BEGIN)==INVALID_SET_FILE_POINTER)
						break;
					if (iLen >= (int)sizeof(szData))
					{
						_ASSERTE(iLen < (int)sizeof(szData));
						iLen = sizeof(szData)-1;
					}
					if (!ReadFile(f, szData, iLen, &(dwRead=0), NULL) || (dwRead != (DWORD)iLen))
						break;
					
					switch (n)
					{
					case 0:
						MultiByteToWideChar(CP_ACP, 0, szData, iLen, szFullName, countof(szFullName)-1);
						lbRc = TRUE;
						break;
					case 1:
						MultiByteToWideChar(CP_ACP, 0, szData, iLen, szName, countof(szName)-1);
						lbRc = TRUE;
						break;
					case 2:
						MultiByteToWideChar(CP_ACP, 0, szData, iLen, szSubName, countof(szSubName)-1);
						break;
					}
				}
			}
		}
	}

	if (lbRc)
	{
		if (!*szFullName)
		{
			// Если полное имя в файле не указано - сформируем сами
			wcscpy_c(szFullName, szName);
			if (*szSubName)
			{
				wcscat_c(szFullName, L" ");
				wcscat_c(szFullName, szSubName);
			}
		}

		szFullName[LF_FACESIZE-1] = 0;
		szName[LF_FACESIZE-1] = 0;
		
		if (szName[0] != 0)
		{
			wcscpy_c(rsFontName, *szName ? szName : szFullName);
		}
		
		if (szFullName[0] != 0)
		{
			wcscpy_c(rsFullFontName, szFullName);
		}
		else
		{
			_ASSERTE(szFullName[0] != 0);
			lbRc = FALSE;
		}
	}
	
wrap:
	if (f && (f != INVALID_HANDLE_VALUE))
		CloseHandle(f);
	return lbRc;
}

//void CSettings::HistoryCheck()
//{
//	if (!gpSet->psCmdHistory || !*gpSet->psCmdHistory)
//	{
//		gpSet->nCmdHistorySize = 0;
//	}
//	else
//	{
//		const wchar_t* psz = gpSet->psCmdHistory;
//
//		while(*psz)
//			psz += _tcslen(psz)+1;
//
//		if (psz == gpSet->psCmdHistory)
//			gpSet->nCmdHistorySize = 0;
//		else
//			gpSet->nCmdHistorySize = (psz - gpSet->psCmdHistory + 1)*sizeof(wchar_t);
//	}
//}

//void CSettings::HistoryAdd(LPCWSTR asCmd)
//{
//	if (!asCmd || !*asCmd)
//		return;
//
//	if (gpSet->psCmd && lstrcmp(gpSet->psCmd, asCmd)==0)
//		return;
//
//	if (gpSet->psCurCmd && lstrcmp(gpSet->psCurCmd, asCmd)==0)
//		return;
//
//	HEAPVAL
//	wchar_t *pszNewHistory, *psz;
//	int nCount = 0;
//	DWORD nCchNewSize = (gpSet->nCmdHistorySize>>1) + _tcslen(asCmd) + 2;
//	DWORD nNewSize = nCchNewSize*2;
//	pszNewHistory = (wchar_t*)malloc(nNewSize);
//
//	//wchar_t* pszEnd = pszNewHistory + nNewSize/sizeof(wchar_t);
//	if (!pszNewHistory) return;
//
//	_wcscpy_c(pszNewHistory, nCchNewSize, asCmd);
//	psz = pszNewHistory + _tcslen(pszNewHistory) + 1;
//	nCount++;
//
//	if (gpSet->psCmdHistory)
//	{
//		wchar_t* pszOld = gpSet->psCmdHistory;
//		int nLen;
//		HEAPVAL;
//
//		while(nCount < MAX_CMD_HISTORY && *pszOld /*&& psz < pszEnd*/)
//		{
//			const wchar_t *pszCur = pszOld;
//			pszOld += _tcslen(pszOld) + 1;
//
//			if (lstrcmp(pszCur, asCmd) == 0)
//				continue;
//
//			_wcscpy_c(psz, nCchNewSize-(psz-pszNewHistory), pszCur);
//			psz += (nLen = (_tcslen(psz)+1));
//			nCount ++;
//		}
//	}
//
//	*psz = 0;
//	HEAPVAL;
//	free(gpSet->psCmdHistory);
//	gpSet->psCmdHistory = pszNewHistory;
//	gpSet->nCmdHistorySize = (psz - pszNewHistory + 1)*sizeof(wchar_t);
//	HEAPVAL;
//	// И сразу сохранить в настройках
//	SettingsBase* reg = CreateSettings();
//
//	if (reg->OpenKey(ConfigPath, KEY_WRITE))
//	{
//		HEAPVAL;
//		reg->SaveMSZ(L"CmdLineHistory", gpSet->psCmdHistory, gpSet->nCmdHistorySize);
//		HEAPVAL;
//		reg->CloseKey();
//	}
//
//	delete reg;
//}

//LPCWSTR CSettings::HistoryGet()
//{
//	if (gpSet->psCmdHistory && *gpSet->psCmdHistory)
//		return gpSet->psCmdHistory;
//
//	return NULL;
//}

// Показать в "Инфо" текущий режим консоли
void CSettings::UpdateConsoleMode(DWORD nMode)
{
	if (hInfo && IsWindow(hInfo))
	{
		wchar_t szInfo[255];
		_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"Console states (0x%X)", nMode);
		SetDlgItemText(hInfo, IDC_CONSOLE_STATES, szInfo);
	}
}

//// например, L"2013-25C3,25C4"
//// Возвращает 0 - в случае успеха,
//// при ошибке - индекс (1-based) ошибочного символа в asRanges
//// -1 - ошибка выделения памяти
//int CSettings::ParseCharRanges(LPCWSTR asRanges, BYTE (&Chars)[0x10000], BYTE abValue /*= TRUE*/)
//{
//	if (!asRanges)
//	{
//		_ASSERTE(asRanges!=NULL);
//		return -1;
//	}
//	
//	int iRc = 0;
//	int n = 0, nMax = _tcslen(asRanges);
//	wchar_t *pszCopy = lstrdup(asRanges);
//	if (!pszCopy)
//	{
//		_ASSERTE(pszCopy!=NULL);
//		return -1;
//	}
//	wchar_t *pszRange = pszCopy;
//	wchar_t *pszNext = NULL;
//	UINT cBegin, cEnd;
//	
//	memset(Chars, 0, sizeof(Chars));
//
//	while(*pszRange && n < nMax)
//	{
//		cBegin = (UINT)wcstol(pszRange, &pszNext, 16);
//		if (!cBegin || (cBegin > 0xFFFF))
//		{
//			iRc = (int)(pszRange - asRanges);
//			goto wrap;
//		}
//			
//		switch (*pszNext)
//		{
//		case L';':
//		case 0:
//			cEnd = cBegin;
//			break;
//		case L'-':
//		case L' ':
//			pszRange = pszNext + 1;
//			cEnd = (UINT)wcstol(pszRange, &pszNext, 16);
//			if ((cEnd < cBegin) || (cEnd > 0xFFFF))
//			{
//				iRc = (int)(pszRange - asRanges);
//				goto wrap;
//			}
//			break;
//		default:
//			iRc = (int)(pszNext - asRanges);
//			goto wrap;
//		}
//
//		for (UINT i = cBegin; i <= cEnd; i++)
//			Chars[i] = abValue;
//		
//		if (*pszNext != L';') break;
//		pszRange = pszNext + 1;
//	}
//	
//	iRc = 0; // ok
//wrap:
//	if (pszCopy)
//		free(pszCopy);
//	return iRc;
//}
//
//// caller must free(result)
//wchar_t* CSettings::CreateCharRanges(BYTE (&Chars)[0x10000])
//{
//	size_t nMax = 1024;
//	wchar_t* pszRanges = (wchar_t*)calloc(nMax,sizeof(*pszRanges));
//	if (!pszRanges)
//	{
//		_ASSERTE(pszRanges!=NULL);
//		return NULL;
//	}
//	
//	wchar_t* psz = pszRanges;
//	wchar_t* pszEnd = pszRanges + nMax;
//	UINT c = 0;
//	_ASSERTE((countof(Chars)-1) == 0xFFFF);
//	while (c < countof(Chars))
//	{
//		if (Chars[c])
//		{
//			if ((psz + 10) >= pszEnd)
//			{
//				// Слишком длинный блок
//				_ASSERTE((psz + 10) < pszEnd);
//				break;
//			}
//			
//			UINT cBegin = (c++);
//			UINT cEnd = cBegin;
//			
//			while (c < countof(Chars) && Chars[c])
//			{
//				cEnd = (c++);
//			}
//			
//			if (cBegin == cEnd)
//			{
//				wsprintf(psz, L"%04X;", cBegin);
//			}
//			else
//			{
//				wsprintf(psz, L"%04X-%04X;", cBegin, cEnd);
//			}
//			psz += _tcslen(psz);
//		}
//		else
//		{
//			c++;
//		}
//	}
//	
//	return pszRanges;
//}

//bool CSettings::isCharBorder(wchar_t inChar)
//{
//	return mpc_FixFarBorderValues[(WORD)inChar];
//}

void CSettings::ResetFontWidth()
{
	memset(CharWidth, 0, sizeof(*CharWidth)*0x10000);
	memset(CharABC, 0, sizeof(*CharABC)*0x10000);
}


typedef long(WINAPI* ThemeFunction_t)();

bool CSettings::CheckTheming()
{
	static bool bChecked = false;

	if (bChecked)
		return mb_ThemingEnabled;

	bChecked = true;
	mb_ThemingEnabled = false;

	if (gOSVer.dwMajorVersion >= 6 || (gOSVer.dwMajorVersion == 5 && gOSVer.dwMinorVersion >= 1))
	{
		ThemeFunction_t fIsAppThemed = NULL;
		ThemeFunction_t fIsThemeActive = NULL;
		HMODULE hUxTheme = GetModuleHandle(L"UxTheme.dll");

		if (hUxTheme)
		{
			fIsAppThemed = (ThemeFunction_t)GetProcAddress(hUxTheme, "IsAppThemed");
			fIsThemeActive = (ThemeFunction_t)GetProcAddress(hUxTheme, "IsThemeActive");

			if (fIsAppThemed && fIsThemeActive)
			{
				long llThemed = fIsAppThemed();
				long llActive = fIsThemeActive();

				if (llThemed && llActive)
					mb_ThemingEnabled = true;
			}
		}
	}

	return mb_ThemingEnabled;
}

void CSettings::ColorSetEdit(HWND hWnd2, WORD c)
{
	WORD tc = (tc0-c0) + c;
	SendDlgItemMessage(hWnd2, tc, EM_SETLIMITTEXT, 11, 0);
	COLORREF cr = 0;
	GetColorById(c, &cr);
	wchar_t temp[16];
	_wsprintf(temp, SKIPLEN(countof(temp)) L"%i %i %i", getR(cr), getG(cr), getB(cr));
	SetDlgItemText(hWnd2, tc, temp);
}

bool CSettings::ColorEditDialog(HWND hWnd2, WORD c)
{
	bool bChanged = false;
	COLORREF color = 0;
	GetColorById(c, &color);
	wchar_t temp[16];
	COLORREF colornew = color;

	if (ShowColorDialog(ghOpWnd, &colornew) && colornew != color)
	{
		SetColorById(c, colornew);
		_wsprintf(temp, SKIPLEN(countof(temp)) L"%i %i %i", getR(colornew), getG(colornew), getB(colornew));
		SetDlgItemText(hWnd2, c + (tc0-c0), temp);
		InvalidateRect(GetDlgItem(hWnd2, c), 0, 1);
		bChanged = true;
	}

	return bChanged;
}

INT_PTR CSettings::ColorCtlStatic(HWND hWnd2, WORD c, HWND hItem)
{
	if (GetDlgItem(hWnd2, c) == hItem)
	{
		if (mh_CtlColorBrush) DeleteObject(mh_CtlColorBrush);

		COLORREF cr = 0;

		if (c >= c32 && c <= c34)
		{
			ThumbColor *ptc = NULL;

			if (c == c32) ptc = &gpSet->ThSet.crBackground;
			else if (c == c33) ptc = &gpSet->ThSet.crPreviewFrame;
			else ptc = &gpSet->ThSet.crSelectFrame;

			//
			if (ptc->UseIndex)
			{
				if (ptc->ColorIdx >= 0 && ptc->ColorIdx <= 15)
				{
					cr = gpSet->Colors[ptc->ColorIdx];
				}
				else
				{
					const CEFAR_INFO_MAPPING *pfi = gpConEmu->ActiveCon()->RCon()->GetFarInfo();

					if (pfi && pfi->cbSize>=sizeof(CEFAR_INFO_MAPPING))
					{
						cr = gpSet->Colors[(pfi->nFarColors[col_PanelText] & 0xF0)>>4];
					}
					else
					{
						cr = gpSet->Colors[1];
					}
				}
			}
			else
			{
				cr = ptc->ColorRGB;
			}
		}
		else
		{
			gpSetCls->GetColorById(c, &cr);
		}

		mh_CtlColorBrush = CreateSolidBrush(cr);
		return (INT_PTR)mh_CtlColorBrush;
	}

	return 0;
}

bool CSettings::GetColorById(WORD nID, COLORREF* color)
{
	if (nID <= c31)
		*color = gpSet->Colors[nID - c0];
	else if (nID == c32)
		*color = gpSet->ThSet.crBackground.ColorRGB;
	else if (nID == c33)
		*color = gpSet->ThSet.crPreviewFrame.ColorRGB;
	else if (nID == c34)
		*color = gpSet->ThSet.crSelectFrame.ColorRGB;
	else
		return false;

	return true;
}

bool CSettings::SetColorById(WORD nID, COLORREF color)
{
	if (nID <= c31)
	{
		gpSet->Colors[nID - c0] = color;
		gpSet->mb_FadeInitialized = false;
	}
	else if (nID == c32)
		gpSet->ThSet.crBackground.ColorRGB = color;
	else if (nID == c33)
		gpSet->ThSet.crPreviewFrame.ColorRGB = color;
	else if (nID == c34)
		gpSet->ThSet.crSelectFrame.ColorRGB = color;
	else
		return false;

	return true;
}

//bool CSettings::isHideCaptionAlways()
//{
//	return gpSet->mb_HideCaptionAlways || (!gpSet->mb_HideCaptionAlways && gpSet->isUserScreenTransparent);
//}

//COLORREF* CSettings::GetColors(BOOL abFade)
//{
//	if (!abFade || !gpSet->isFadeInactive)
//		return gpSet->Colors;
//
//	if (!mb_FadeInitialized)
//	{
//		gpSet->mn_LastFadeSrc = gpSet->mn_LastFadeDst = -1;
//		if (((int)gpSet->mn_FadeHigh - (int)gpSet->mn_FadeLow) < 64)
//		{
//			gpSet->mn_FadeLow = DEFAULT_FADE_LOW; gpSet->mn_FadeHigh = DEFAULT_FADE_HIGH;
//		}
//
//		mn_FadeMul = gpSet->mn_FadeHigh - gpSet->mn_FadeLow;
//		mb_FadeInitialized = true;
//
//		for(int i=0; i<countof(ColorsFade); i++)
//		{
//			ColorsFade[i] = GetFadeColor(gpSet->Colors[i]);
//		}
//	}
//
//	return ColorsFade;
//}
//
//COLORREF CSettings::GetFadeColor(COLORREF cr)
//{
//	if (!gpSet->isFadeInactive)
//		return cr;
//		
//	if (cr == gpSet->mn_LastFadeSrc)
//		return gpSet->mn_LastFadeDst;
//
//	MYCOLORREF mcr, mcrFade = {0}; mcr.color = cr;
//
//	if (!mb_FadeInitialized)
//	{
//		GetColors(TRUE);
//	}
//
//	mcrFade.rgbRed = GetFadeColorItem(mcr.rgbRed);
//	mcrFade.rgbGreen = GetFadeColorItem(mcr.rgbGreen);
//	mcrFade.rgbBlue = GetFadeColorItem(mcr.rgbBlue);
//
//	gpSet->mn_LastFadeSrc = cr;
//	gpSet->mn_LastFadeDst = mcrFade.color;
//	
//	return mcrFade.color;
//}

//BYTE CSettings::GetFadeColorItem(BYTE c)
//{
//	DWORD nRc;
//
//	switch(c)
//	{
//		case 0:
//			return gpSet->mn_FadeLow;
//		case 255:
//			return gpSet->mn_FadeHigh;
//		default:
//			nRc = ((((DWORD)c) + gpSet->mn_FadeLow) * gpSet->mn_FadeHigh) >> 8;
//
//			if (nRc >= 255)
//			{
//				//_ASSERTE(nRc <= 255); -- такие (gpSet->mn_FadeLow&gpSet->mn_FadeHigh) пользователь в настройке мог задать
//				return 255;
//			}
//
//			return (BYTE)nRc;
//	}
//}

//bool CSettings::NeedDialogDetect()
//{
//	// 100331 теперь оно нужно и для PanelView
//	return true;
//	//return (gpSet->isUserScreenTransparent || !gpSet->isMonospace);
//}

void CSettings::FillListBoxItems(HWND hList, uint nItems, const WCHAR** pszItems, const DWORD* pnValues, DWORD& nValue, BOOL abExact /*= FALSE*/)
{
	_ASSERTE(hList!=NULL);
	uint num = 0;
	wchar_t szNumber[32];

	SendMessage(hList, CB_RESETCONTENT, 0, 0);

	for (uint i = 0; i < nItems; i++)
	{
		if (pszItems)
		{
			SendMessage(hList, CB_ADDSTRING, 0, (LPARAM) pszItems[i]); //-V108
		}
		else
		{
			_wsprintf(szNumber, SKIPLEN(countof(szNumber)) L"%i", pnValues[i]);
			SendMessage(hList, CB_ADDSTRING, 0, (LPARAM)szNumber);
		}

		if (pnValues[i] == nValue) num = i; //-V108
	}

	if (abExact)
	{
		_wsprintf(szNumber, SKIPLEN(countof(szNumber)) L"%i", nValue);
		SelectStringExact(hList, 0, szNumber);
	}
	else
	{
		if (!num)
			nValue = 0;  // если код неизвестен?
		SendMessage(hList, CB_SETCURSEL, num, 0);
	}
}

void CSettings::GetListBoxItem(HWND hList, uint nItems, const WCHAR** pszItems, const DWORD* pnValues, DWORD& nValue)
{
	_ASSERTE(hList!=NULL);
	INT_PTR num = SendMessage(hList, CB_GETCURSEL, 0, 0);

	//int nKeyCount = countof(SettingsNS::szKeys);
	if (num>=0 && num<(int)nItems)
	{
		nValue = pnValues[num]; //-V108
	}
	else
	{
		nValue = 0;

		if (num)  // Invalid index?
			SendMessage(hList, CB_SETCURSEL, num=0, 0);
	}
}

void CSettings::CenterMoreDlg(HWND hWnd2)
{
	RECT rcParent, rc;
	GetWindowRect(ghOpWnd ? ghOpWnd : ghWnd, &rcParent);
	GetWindowRect(hWnd2, &rc);
	MoveWindow(hWnd2,
	           (rcParent.left+rcParent.right-rc.right+rc.left)/2,
	           (rcParent.top+rcParent.bottom-rc.bottom+rc.top)/2,
	           rc.right - rc.left, rc.bottom - rc.top, TRUE);
}

//bool CSettings::isSelectionModifierPressed()
//{
//	if (gpSet->isCTSSelectBlock && gpSet->isCTSVkBlock && isPressed(gpSet->isCTSVkBlock))
//		return true;
//	if (gpSet->isCTSSelectText && gpSet->isCTSVkText && isPressed(gpSet->isCTSVkText))
//		return true;
//	return false;
//}

//bool CSettings::isModifierPressed(DWORD vk)
//{
//	// если НЕ 0 - должен быть нажат
//	if (vk)
//	{
//		if (!isPressed(vk))
//			return false;
//	}
//
//	// но другие модификаторы нажаты быть не должны!
//	if (vk != VK_SHIFT && vk != VK_LSHIFT && vk != VK_RSHIFT)
//	{
//		if (isPressed(VK_SHIFT))
//			return false;
//	}
//
//	if (vk != VK_MENU && vk != VK_LMENU && vk != VK_RMENU)
//	{
//		if (isPressed(VK_MENU))
//			return false;
//	}
//
//	if (vk != VK_CONTROL && vk != VK_LCONTROL && vk != VK_RCONTROL)
//	{
//		if (isPressed(VK_CONTROL))
//			return false;
//	}
//
//	// Можно
//	return true;
//}

void CSettings::OnPanelViewAppeared(BOOL abAppear)
{
	if (hViews && IsWindow(hViews))
	{
		if (abAppear != IsWindowEnabled(GetDlgItem(hViews,bApplyViewSettings)))
			EnableWindow(GetDlgItem(hViews,bApplyViewSettings), abAppear);
	}
}

bool CSettings::PollBackgroundFile()
{
	bool lbChanged = false;

	if (gpSet->isShowBgImage && gpSet->sBgImage[0] && (GetTickCount() - nBgModifiedTick) > BACKGROUND_FILE_POLL)
	{
		WIN32_FIND_DATA fnd = {0};
		HANDLE hFind = FindFirstFile(gpSet->sBgImage, &fnd);

		if (hFind != INVALID_HANDLE_VALUE)
		{
			if (fnd.ftLastWriteTime.dwHighDateTime != ftBgModified.dwHighDateTime
			        || fnd.ftLastWriteTime.dwLowDateTime != ftBgModified.dwLowDateTime)
			{
				//NeedBackgroundUpdate(); -- поставит LoadBackgroundFile, если у него получится файл открыть
				lbChanged = LoadBackgroundFile(gpSet->sBgImage, false);
				nBgModifiedTick = GetTickCount();
			}

			FindClose(hFind);
		}
	}

	return lbChanged;
}

// Должна вернуть true, если файл изменился
// Работает ТОЛЬКО с файлом. Данные плагинов обрабатываются в самом CVirtualConsole!
bool CSettings::PrepareBackground(HDC* phBgDc, COORD* pbgBmpSize)
{
	bool lbForceUpdate = false;
	LONG lMaxBgWidth = 0, lMaxBgHeight = 0;
	bool bIsForeground = gpConEmu->isMeForeground(false);

	if (!mb_NeedBgUpdate)
	{
		if ((mb_BgLastFade == bIsForeground && gpSet->isFadeInactive)
		        || (!gpSet->isFadeInactive && mb_BgLastFade))
		{
			NeedBackgroundUpdate();
		}
	}

	PollBackgroundFile();

	if (mp_Bg == NULL)
	{
		NeedBackgroundUpdate();
	}

	// Если это НЕ плагиновая подложка - необходимо проверить размер требуемой картинки
	// -- здесь - всегда только файловая подложка
	//if (!mb_WasVConBgImage)
	{
		if (gpSet->bgOperation == eUpLeft)
		{
			// MemoryDC создается всегда по размеру картинки, т.е. изменение размера окна - игнорируется
		}
		else
		{
			RECT rcWnd, rcWork; GetClientRect(ghWnd, &rcWnd);
			rcWork = gpConEmu->CalcRect(CER_WORKSPACE, rcWnd, CER_MAINCLIENT);

			// Смотрим дальше
			if (gpSet->bgOperation == eStretch)
			{
				// Строго по размеру клиентской (точнее Workspace) области окна
				lMaxBgWidth = rcWork.right - rcWork.left;
				lMaxBgHeight = rcWork.bottom - rcWork.top;
			}
			else if (gpSet->bgOperation == eTile)
			{
				// Max между клиентской (точнее Workspace) областью окна и размером текущего монитора
				// Окно может быть растянуто на несколько мониторов, т.е. размер клиентской области может быть больше
				HMONITOR hMon = MonitorFromWindow(ghWnd, MONITOR_DEFAULTTONEAREST);
				MONITORINFO mon = {sizeof(MONITORINFO)};
				GetMonitorInfo(hMon, &mon);
				//
				lMaxBgWidth = klMax(rcWork.right - rcWork.left,mon.rcMonitor.right - mon.rcMonitor.left);
				lMaxBgHeight = klMax(rcWork.bottom - rcWork.top,mon.rcMonitor.bottom - mon.rcMonitor.top);
			}

			if (mp_Bg)
			{
				if (mp_Bg->bgSize.X != lMaxBgWidth || mp_Bg->bgSize.Y != lMaxBgHeight)
					NeedBackgroundUpdate();
			}
			else
			{
				NeedBackgroundUpdate();
			}
		}
	}

	if (mb_NeedBgUpdate)
	{
		mb_NeedBgUpdate = FALSE;
		lbForceUpdate = true;
		_ASSERTE(gpConEmu->isMainThread());
		//MSectionLock SBG; SBG.Lock(&mcs_BgImgData);
		//BITMAPFILEHEADER* pImgData = mp_BgImgData;
		BackgroundOp op = (BackgroundOp)gpSet->bgOperation;
		BOOL lbImageExist = (mp_BgImgData != NULL);
		//BOOL lbVConImage = FALSE;
		//LONG lBgWidth = 0, lBgHeight = 0;
		//CVirtualConsole* pVCon = gpConEmu->ActiveCon();

		////MSectionLock SBK;
		//if (apVCon && gpSet->isBgPluginAllowed)
		//{
		//	//SBK.Lock(&apVCon->csBkImgData);
		//	if (apVCon->HasBackgroundImage(&lBgWidth, &lBgHeight)
		//	        && lBgWidth && lBgHeight)
		//	{
		//		lbVConImage = lbImageExist = TRUE;
		//	}
		//}

		//mb_WasVConBgImage = lbVConImage;

		if (lbImageExist)
		{
			if (!mp_Bg)
				mp_Bg = new CBackground;

			mb_BgLastFade = (!bIsForeground && gpSet->isFadeInactive);
			TODO("Переделать, ориентироваться только на размер картинки - неправильно");
			TODO("DoubleView - скорректировать X,Y");

			//if (lbVConImage)
			//{
			//	if (lMaxBgWidth && lMaxBgHeight)
			//	{
			//		lBgWidth = lMaxBgWidth;
			//		lBgHeight = lMaxBgHeight;
			//	}

			//	if (!mp_Bg->CreateField(lBgWidth, lBgHeight) ||
			//	        !apVCon->PutBackgroundImage(mp_Bg, 0,0, lBgWidth, lBgHeight))
			//	{
			//		delete mp_Bg;
			//		mp_Bg = NULL;
			//	}
			//}
			//else
			{
				BITMAPINFOHEADER* pBmp = (BITMAPINFOHEADER*)(mp_BgImgData+1);

				if (!lMaxBgWidth || !lMaxBgHeight)
				{
					// Сюда мы можем попасть только в случае eUpLeft
					lMaxBgWidth = pBmp->biWidth;
					lMaxBgHeight = pBmp->biHeight;
				}

				if (!mp_Bg->CreateField(lMaxBgWidth, lMaxBgHeight) ||
				        !mp_Bg->FillBackground(mp_BgImgData, 0,0, lMaxBgWidth, lMaxBgHeight, op, mb_BgLastFade))
				{
					delete mp_Bg;
					mp_Bg = NULL;
				}
			}
		}
		else
		{
			delete mp_Bg;
			mp_Bg = NULL;
		}
	}

	if (mp_Bg)
	{
		*phBgDc = mp_Bg->hBgDc;
		*pbgBmpSize = mp_Bg->bgSize;
	}
	else
	{
		*phBgDc = NULL;
		*pbgBmpSize = MakeCoord(0,0);
	}

	return lbForceUpdate;
}

bool CSettings::IsBackgroundEnabled(CVirtualConsole* apVCon)
{
	// Если плагин фара установил свой фон
	if (gpSet->isBgPluginAllowed && apVCon && apVCon->HasBackgroundImage(NULL,NULL))
	{
		if (apVCon->isEditor || apVCon->isViewer)
			return (gpSet->isBgPluginAllowed == 1);

		return true;
	}

	// Иначе - по настрокам ConEmu
	if (!isBackgroundImageValid)
		return false;

	if (apVCon && (apVCon->isEditor || apVCon->isViewer))
	{
		return (gpSet->isShowBgImage == 1);
	}
	else
	{
		return (gpSet->isShowBgImage != 0);
	}
}

TODO("LoadImage может загрузить и jpg, а ручное преобразование лучше заменить на AlphaBlend");
bool CSettings::LoadBackgroundFile(TCHAR *inPath, bool abShowErrors)
{
	//_ASSERTE(gpConEmu->isMainThread());
	if (!inPath || _tcslen(inPath)>=MAX_PATH)
	{
		if (abShowErrors)
			MBoxA(L"Invalid 'inPath' in Settings::LoadImageFrom");

		return false;
	}

	TCHAR exPath[MAX_PATH + 2];

	if (!ExpandEnvironmentStrings(inPath, exPath, MAX_PATH))
	{
		if (abShowErrors)
		{
			wchar_t szError[MAX_PATH*2];
			DWORD dwErr = GetLastError();
			_wsprintf(szError, SKIPLEN(countof(szError)) L"Can't expand environment strings:\r\n%s\r\nError code=0x%08X\r\nImage loading failed",
			          inPath, dwErr);
			MBoxA(szError);
		}

		return false;
	}

	_ASSERTE(gpConEmu->isMainThread());
	bool lRes = false;
	BY_HANDLE_FILE_INFORMATION inf = {0};
	BITMAPFILEHEADER* pBkImgData = LoadImageEx(exPath, inf);
	if (pBkImgData)
	{
		ftBgModified = inf.ftLastWriteTime;
		nBgModifiedTick = GetTickCount();
		NeedBackgroundUpdate();
		//MSectionLock SBG; SBG.Lock(&mcs_BgImgData);
		SafeFree(mp_BgImgData);
		isBackgroundImageValid = true;
		mp_BgImgData = pBkImgData;
		lRes = true;
	}
	
	//HANDLE hFile = CreateFile(exPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, NULL);
	//if (hFile != INVALID_HANDLE_VALUE)
	//{
	//	BY_HANDLE_FILE_INFORMATION inf = {0};
	//	//LARGE_INTEGER nFileSize;
	//	//if (GetFileSizeEx(hFile, &nFileSize) && nFileSize.HighPart == 0
	//	if (GetFileInformationByHandle(hFile, &inf) && inf.nFileSizeHigh == 0
	//	        && inf.nFileSizeLow >= (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO))) //-V119
	//	{
	//		pBkImgData = (BITMAPFILEHEADER*)malloc(inf.nFileSizeLow);
	//		if (pBkImgData && ReadFile(hFile, pBkImgData, inf.nFileSizeLow, &inf.nFileSizeLow, NULL))
	//		{
	//			char *pBuf = (char*)pBkImgData;
	//			if (pBuf[0] == 'B' && pBuf[1] == 'M' && *(u32*)(pBuf + 0x0A) >= 0x36 && *(u32*)(pBuf + 0x0A) <= 0x436 && *(u32*)(pBuf + 0x0E) == 0x28 && !pBuf[0x1D] && !*(u32*)(pBuf + 0x1E))
	//			{
	//				ftBgModified = inf.ftLastWriteTime;
	//				nBgModifiedTick = GetTickCount();
	//				NeedBackgroundUpdate();
	//				_ASSERTE(gpConEmu->isMainThread());
	//				//MSectionLock SBG; SBG.Lock(&mcs_BgImgData);
	//				SafeFree(mp_BgImgData);
	//				isBackgroundImageValid = true;
	//				mp_BgImgData = pBkImgData;
	//				lRes = true;
	//			}
	//		}
	//	}
	//	SafeCloseHandle(hFile);
	//}

	//klFile file;
	//if (file.Open(exPath))
	//{
	//    char File[101];
	//    file.Read(File, 100);
	//    char *pBuf = File;
	//    if (pBuf[0] == 'B' && pBuf[1] == 'M' && *(u32*)(pBuf + 0x0A) >= 0x36 && *(u32*)(pBuf + 0x0A) <= 0x436 && *(u32*)(pBuf + 0x0E) == 0x28 && !pBuf[0x1D] && !*(u32*)(pBuf + 0x1E))
	//        //if (*(u16*)pBuf == 'MB' && *(u32*)(pBuf + 0x0A) >= 0x36)
	//    {
	//
	//    	PRAGMA_ERROR("Перенести код в Settings::CreateBackgroundImage и переделать на AlphaBlend");
	//
	//        const HDC hScreenDC = GetDC(0);
	//        HDC hNewBgDc = CreateCompatibleDC(hScreenDC);
	//        HBITMAP hNewBgBitmap;
	//        if (hNewBgDc)
	//        {
	//            if ((hNewBgBitmap = (HBITMAP)LoadImage(NULL, exPath, IMAGE_BITMAP,0,0,LR_LOADFROMFILE)) != NULL)
	//            {
	//                if (hBgBitmap) { DeleteObject(hBgBitmap); hBgBitmap=NULL; }
	//                if (hBgDc) { DeleteDC(hBgDc); hBgDc=NULL; }
	//                hBgDc = hNewBgDc;
	//                hBgBitmap = hNewBgBitmap;
	//                if (SelectObject(hBgDc, hBgBitmap))
	//                {
	//                    isBackgroundImageValid = true;
	//                    bgBmp.X = *(u32*)(pBuf + 0x12);
	//                    bgBmp.Y = *(i32*)(pBuf + 0x16);
	//                    // Ровняем на границу 4-х пикселов (WinXP SP2)
	//                    int nCxFixed = ((bgBmp.X+3)>>2)<<2;
	//                    if (klstricmp(gpSet->sBgImage, inPath))
	//                    {
	//                        lRes = true;
	//                        _tcscpy(gpSet->sBgImage, inPath);
	//                    }
	//                    struct bColor
	//                    {
	//                        u8 b;
	//                        u8 g;
	//                        u8 r;
	//                    };
	//                    MCHKHEAP
	//                        //GetDIBits памяти не хватает
	//                    bColor *bArray = new bColor[(nCxFixed+10) * bgBmp.Y];
	//                    MCHKHEAP
	//                    BITMAPINFO bInfo; memset(&bInfo, 0, sizeof(bInfo));
	//                    bInfo.bmiHeader.biSize = sizeof(BITMAPINFO);
	//                    bInfo.bmiHeader.biWidth = nCxFixed/*bgBmp.X*/;
	//                    bInfo.bmiHeader.biHeight = bgBmp.Y;
	//                    bInfo.bmiHeader.biPlanes = 1;
	//                    bInfo.bmiHeader.biBitCount = 24;
	//                    bInfo.bmiHeader.biCompression = BI_RGB;
	//                    MCHKHEAP
	//                    if (!GetDIBits(hBgDc, hBgBitmap, 0, bgBmp.Y, bArray, &bInfo, DIB_RGB_COLORS))
	//                        //MBoxA(L"!"); //Maximus5 - Да, это очень информативно
	//                        MBoxA(L"!GetDIBits");
	//                    MCHKHEAP
	//                    for (int y=0; y<bgBmp.Y; y++)
	//                    {
	//                        int i = y*nCxFixed;
	//                        for (int x=0; x<bgBmp.X; x++, i++)
	//                        //for (int i = 0; i < bgBmp.X * bgBmp.Y; i++)
	//                        {
	//                            bArray[i].r = klMulDivU32(bArray[i].r, gpSet->bgImageDarker, 255);
	//                            bArray[i].g = klMulDivU32(bArray[i].g, gpSet->bgImageDarker, 255);
	//                            bArray[i].b = klMulDivU32(bArray[i].b, gpSet->bgImageDarker, 255);
	//                        }
	//                    }
	//                    MCHKHEAP
	//                    if (!SetDIBits(hBgDc, hBgBitmap, 0, bgBmp.Y, bArray, &bInfo, DIB_RGB_COLORS))
	//                        MBoxA(L"!SetDIBits");
	//                    MCHKHEAP
	//                    delete[] bArray;
	//                    MCHKHEAP
	//                }
	//            }
	//            else
	//                DeleteDC(hNewBgDc);
	//        }
	//        ReleaseDC(0, hScreenDC);
	//    } else {
	//        if (abShowErrors)
	//            MBoxA(L"Only BMP files supported as background!");
	//    }
	//    file.Close();
	//}
	return lRes;
}

void CSettings::NeedBackgroundUpdate()
{
	mb_NeedBgUpdate = TRUE;
}

//CBackground* CSettings::CreateBackgroundImage(const BITMAPFILEHEADER* apBkImgData)
//{
//	PRAGMA_ERROR("Доделать Settings::CreateBackgroundImage");
//}

// общая функция
void CSettings::ShowErrorTip(LPCTSTR asInfo, HWND hDlg, int nCtrlID, wchar_t* pszBuffer, int nBufferSize, HWND hBall, TOOLINFO *pti, HWND hTip, DWORD nTimeout)
{
	if (!asInfo)
		pszBuffer[0] = 0;
	else if (asInfo != pszBuffer)
		lstrcpyn(pszBuffer, asInfo, nBufferSize);

	pti->lpszText = pszBuffer;

	if (pszBuffer[0])
	{
		if (hTip) SendMessage(hTip, TTM_ACTIVATE, FALSE, 0);

		SendMessage(hBall, TTM_UPDATETIPTEXT, 0, (LPARAM)pti);
		RECT rcControl; GetWindowRect(GetDlgItem(hDlg, nCtrlID), &rcControl);
		int ptx = rcControl.right - 10;
		int pty = (rcControl.top + rcControl.bottom) / 2;
		SendMessage(hBall, TTM_TRACKPOSITION, 0, MAKELONG(ptx,pty));
		SendMessage(hBall, TTM_TRACKACTIVATE, TRUE, (LPARAM)pti);
		SetTimer(hDlg, FAILED_FONT_TIMERID, nTimeout/*FAILED_FONT_TIMEOUT*/, 0);
	}
	else
	{
		SendMessage(hBall, TTM_TRACKACTIVATE, FALSE, (LPARAM)pti);

		if (hTip) SendMessage(hTip, TTM_ACTIVATE, TRUE, 0);
	}
}



/* ********************************************** */
/*         обработка шрифта в RealConsole         */
/* ********************************************** */

bool CSettings::EditConsoleFont(HWND hParent)
{
	hConFontDlg = NULL; nConFontError = 0;
	int nRc = DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_MORE_CONFONT), hParent, EditConsoleFontProc); //-V103
	hConFontDlg = NULL;
	return (nRc == IDOK);
}

bool CSettings::CheckConsoleFontRegistry(LPCWSTR asFaceName)
{
	bool lbFound = false;
	HKEY hk;

	if (!RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Console\\TrueTypeFont",
	                  0, KEY_READ, &hk))
	{
		wchar_t szId[32] = {0}, szFont[255]; DWORD dwLen, dwType;

		for(DWORD i = 0; i <20; i++)
		{
			szId[i] = L'0'; szId[i+1] = 0; wmemset(szFont, 0, 255);

			if (RegQueryValueExW(hk, szId, NULL, &dwType, (LPBYTE)szFont, &(dwLen = 255*2)))
				break;

			if (lstrcmpi(szFont, asFaceName) == 0)
			{
				lbFound = true; break;
			}
		}

		RegCloseKey(hk);
	}

	return lbFound;
}

// Вызывается при запуске ConEmu для быстрой проверки шрифта
// EnumFontFamilies не вызывается, т.к. занимает время
bool CSettings::CheckConsoleFontFast()
{
	//wchar_t szCreatedFaceName[32] = {0};
	LOGFONT LF = gpSet->ConsoleFont;
	gpSetCls->nConFontError = 0; //ConFontErr_NonSystem|ConFontErr_NonRegistry|ConFontErr_InvalidName;
	HFONT hf = CreateFontIndirect(&LF);

	if (!hf)
	{
		gpSetCls->nConFontError = ConFontErr_InvalidName;
	}
	else
	{
		LPOUTLINETEXTMETRICW lpOutl = gpSetCls->LoadOutline(NULL, hf);

		if (!lpOutl)
		{
			gpSetCls->nConFontError = ConFontErr_InvalidName;
		}
		else
		{
			LPCWSTR pszFamilyName = (LPCWSTR)lpOutl->otmpFamilyName;

			// Интересуют только TrueType (вроде только для TTF доступен lpOutl - проверить
			if (pszFamilyName[0] != L'@'
			        && IsAlmostMonospace(pszFamilyName, lpOutl->otmTextMetrics.tmMaxCharWidth, lpOutl->otmTextMetrics.tmAveCharWidth, lpOutl->otmTextMetrics.tmHeight)
			        && lpOutl->otmPanoseNumber.bProportion == PAN_PROP_MONOSPACED
			        && lstrcmpi(pszFamilyName, gpSet->ConsoleFont.lfFaceName) == 0
			  )
			{
				BOOL lbNonSystem = FALSE;

				// Нельзя использовать шрифты, которые зарегистрированы нами (для ConEmu). Они должны быть системными
				for (std::vector<RegFont>::iterator iter = gpSetCls->m_RegFonts.begin(); !lbNonSystem && iter != gpSetCls->m_RegFonts.end(); ++iter)
				{
					if (!iter->bAlreadyInSystem &&
					        lstrcmpi(iter->szFontName, gpSet->ConsoleFont.lfFaceName) == 0)
						lbNonSystem = TRUE;
				}

				if (lbNonSystem)
					gpSetCls->nConFontError = ConFontErr_NonSystem;
			}
			else
			{
				gpSetCls->nConFontError = ConFontErr_NonSystem;
			}

			free(lpOutl);
		}
	}

	// Если успешно - проверить зарегистрированность в реестре
	if (gpSetCls->nConFontError == 0)
	{
		if (!CheckConsoleFontRegistry(gpSet->ConsoleFont.lfFaceName))
			gpSetCls->nConFontError |= ConFontErr_NonRegistry;
	}

	bConsoleFontChecked = (gpSetCls->nConFontError == 0);
	return bConsoleFontChecked;
}

bool CSettings::CheckConsoleFont(HWND ahDlg)
{
	gpSetCls->nConFontError = ConFontErr_NonSystem|ConFontErr_NonRegistry;
	// Сначала загрузить имена шрифтов, установленных в систему
	HDC hdc = GetDC(NULL);
	EnumFontFamilies(hdc, (LPCTSTR) NULL, (FONTENUMPROC) EnumConFamCallBack, (LPARAM) ahDlg);
	DeleteDC(hdc);

	// Показать текущий шрифт
	if (ahDlg)
	{
		if (SelectString(ahDlg, tConsoleFontFace, gpSet->ConsoleFont.lfFaceName)<0)
			SetDlgItemText(ahDlg, tConsoleFontFace, gpSet->ConsoleFont.lfFaceName);
	}

	// Проверить реестр
	if (CheckConsoleFontRegistry(gpSet->ConsoleFont.lfFaceName))
		gpSetCls->nConFontError &= ~(DWORD)ConFontErr_NonRegistry;

	return (gpSetCls->nConFontError == 0);
}

INT_PTR CSettings::EditConsoleFontProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch(messg)
	{
		case WM_INITDIALOG:
		{
			SendMessage(hWnd2, WM_SETICON, ICON_BIG, (LPARAM)hClassIcon);
			SendMessage(hWnd2, WM_SETICON, ICON_SMALL, (LPARAM)hClassIconSm);

			gpSetCls->hConFontDlg = NULL; // пока не выставим - на смену в контролах не реагировать
			wchar_t temp[10];

			for (uint i = 0; i < countof(SettingsNS::FSizesSmall); i++)
			{
				_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", SettingsNS::FSizesSmall[i]);
				SendDlgItemMessage(hWnd2, tConsoleFontSizeY, CB_ADDSTRING, 0, (LPARAM) temp);
				_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", (int)(SettingsNS::FSizesSmall[i]*3/2));
				SendDlgItemMessage(hWnd2, tConsoleFontSizeX, CB_ADDSTRING, 0, (LPARAM) temp);

				if ((LONG)SettingsNS::FSizesSmall[i] >= gpSetCls->LogFont.lfHeight)
					break; // не допускаются шрифты больше, чем выбрано для основного шрифта!
			}

			_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", gpSet->ConsoleFont.lfHeight);
			SelectStringExact(hWnd2, tConsoleFontSizeY, temp);
			_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", gpSet->ConsoleFont.lfWidth);
			SelectStringExact(hWnd2, tConsoleFontSizeX, temp);

			// Показать текущий шрифт и проверить его
			if (CheckConsoleFont(hWnd2))
			{
				EnableWindow(GetDlgItem(hWnd2, bConFontOK), TRUE);
				EnableWindow(GetDlgItem(hWnd2, bConFontAdd2HKLM), FALSE);
			}
			else
			{
				EnableWindow(GetDlgItem(hWnd2, bConFontOK), FALSE);
				EnableWindow(GetDlgItem(hWnd2, bConFontAdd2HKLM), (gpSetCls->nConFontError&ConFontErr_NonRegistry)!=0);
			}

			// BCM_SETSHIELD = 5644
			if (gOSVer.dwMajorVersion >= 6)
				SendDlgItemMessage(hWnd2, bConFontAdd2HKLM, 5644/*BCM_SETSHIELD*/, 0, TRUE);

			// запускаем user-mode
			gpSetCls->hConFontDlg = hWnd2;
			gpSetCls->RegisterTipsFor(hWnd2);
			CenterMoreDlg(hWnd2);

			if (gpConEmu->ActiveCon())
				EnableWindow(GetDlgItem(hWnd2, tConsoleFontConsoleNote), TRUE);

			// Если есть ошибка - показать
			gpSetCls->bShowConFontError = (gpSetCls->nConFontError != 0);
		}
		break;
		case WM_DESTROY:

			if (gpSetCls->hwndConFontBalloon) {DestroyWindow(gpSetCls->hwndConFontBalloon); gpSetCls->hwndConFontBalloon = NULL;}

			gpSetCls->hConFontDlg = NULL;
			break;
		case WM_TIMER:

			if (wParam == FAILED_FONT_TIMERID)
			{
				KillTimer(hWnd2, FAILED_FONT_TIMERID);
				SendMessage(gpSetCls->hwndConFontBalloon, TTM_TRACKACTIVATE, FALSE, (LPARAM)&gpSetCls->tiConFontBalloon);
			}

			break;
		//case WM_GETICON:

		//	if (wParam==ICON_BIG)
		//	{
		//		/*SetWindowLong(hWnd2, DWL_MSGRESULT, (LRESULT)hClassIcon);
		//		return 1;*/
		//	}
		//	else
		//	{
		//		SetWindowLongPtr(hWnd2, DWLP_MSGRESULT, (LRESULT)hClassIconSm);
		//		return 1;
		//	}

		//	return 0;
		case WM_COMMAND:

			if (HIWORD(wParam) == BN_CLICKED)
			{
				WORD TB = LOWORD(wParam);

				if (TB == IDOK)
					return 0;
				else if (TB == bConFontOK)
				{
					// На всякий случай, повторно считаем поля диалога
					GetDlgItemText(hWnd2, tConsoleFontFace, gpSet->ConsoleFont.lfFaceName, countof(gpSet->ConsoleFont.lfFaceName));
					gpSet->ConsoleFont.lfHeight = GetNumber(hWnd2, tConsoleFontSizeY);
					gpSet->ConsoleFont.lfWidth = GetNumber(hWnd2, tConsoleFontSizeX);

					// Проверка валидности
					if (gpSetCls->nConFontError == ConFontErr_NonRegistry && gpSetCls->CheckConsoleFontRegistry(gpSet->ConsoleFont.lfFaceName))
					{
						gpSetCls->nConFontError &= ~(DWORD)ConFontErr_NonRegistry;
					}

					if (gpSetCls->nConFontError)
					{
						_ASSERTE(gpSetCls->nConFontError==0);
						MessageBox(hWnd2, gpSetCls->sConFontError[0] ? gpSetCls->sConFontError : gpSetCls->CreateConFontError(NULL,NULL), gpConEmu->GetDefaultTitle(), MB_OK|MB_ICONSTOP);
						return 0;
					}

					gpSet->SaveConsoleFont(); // Сохранить шрифт в настройке
					EndDialog(hWnd2, IDOK);
				}
				else if (TB == IDCANCEL || TB == bConFontCancel)
				{
					if (!gpSetCls->bConsoleFontChecked)
					{
						wcscpy_c(gpSet->ConsoleFont.lfFaceName, gpSetCls->sDefaultConFontName[0] ? gpSetCls->sDefaultConFontName : L"Lucida Console");
						gpSet->ConsoleFont.lfHeight = 5; gpSet->ConsoleFont.lfWidth = 3;
					}

					EndDialog(hWnd2, IDCANCEL);
				}
				else if (TB == bConFontAdd2HKLM)
				{
					// Добавить шрифт в HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Console\TrueTypeFont
					gpSetCls->ShowConFontErrorTip(NULL);
					EnableWindow(GetDlgItem(hWnd2, tConsoleFontHklmNote), TRUE);
					wchar_t szFaceName[32] = {0};
					bool lbFontJustRegistered = false;
					bool lbFound = false;
					GetDlgItemText(hWnd2, tConsoleFontFace, szFaceName, countof(szFaceName));
					HKEY hk;

					if (!RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Console\\TrueTypeFont",
					                  0, KEY_ALL_ACCESS, &hk))
					{
						wchar_t szId[32] = {0}, szFont[255]; DWORD dwLen, dwType;

						for(DWORD i = 0; i <20; i++)
						{
							szId[i] = L'0'; szId[i+1] = 0; wmemset(szFont, 0, 255);

							if (RegQueryValueExW(hk, szId, NULL, &dwType, (LPBYTE)szFont, &(dwLen = 255*2)))
							{
								if (!RegSetValueExW(hk, szId, 0, REG_SZ, (LPBYTE)szFaceName, (_tcslen(szFaceName)+1)*2))
								{
									lbFontJustRegistered = lbFound = true; // OK, добавили
								}

								break;
							}

							if (lstrcmpi(szFont, szFaceName) == 0)
							{
								lbFound = true; break; // он уже добавлен
							}
						}

						RegCloseKey(hk);
					}

					// Если не удалось (нет прав) то попробовать запустить ConEmuC.exe под админом (Vista+)
					if (!lbFound && gOSVer.dwMajorVersion >= 6)
					{
						wchar_t szCommandLine[MAX_PATH];
						SHELLEXECUTEINFO sei = {sizeof(SHELLEXECUTEINFO)};
						sei.hwnd = hWnd2;
						sei.fMask = SEE_MASK_NO_CONSOLE|SEE_MASK_NOCLOSEPROCESS|SEE_MASK_NOASYNC;
						sei.lpVerb = L"runas";
						sei.lpFile = gpConEmu->ms_ConEmuCExeFull;
						_wsprintf(szCommandLine, SKIPLEN(countof(szCommandLine)) L" \"/REGCONFONT=%s\"", szFaceName);
						sei.lpParameters = szCommandLine;
						sei.lpDirectory = gpConEmu->ms_ConEmuCurDir;
						sei.nShow = SW_SHOWMINIMIZED;
						BOOL lbRunAsRc = ::ShellExecuteEx(&sei);

						if (!lbRunAsRc)
						{
							//DisplayLastError( IsWindows64()
							//	? L"Can't start ConEmuC64.exe, console font registration failed!"
							//	: L"Can't start ConEmuC.exe, console font registration failed!");
#ifdef WIN64
							DisplayLastError(L"Can't start ConEmuC64.exe, console font registration failed!");
#else
							DisplayLastError(L"Can't start ConEmuC.exe, console font registration failed!");
#endif
						}
						else
						{
							DWORD nWait = WaitForSingleObject(sei.hProcess, 30000); UNREFERENCED_PARAMETER(nWait);
							CloseHandle(sei.hProcess);

							if (gpSetCls->CheckConsoleFontRegistry(gpSet->ConsoleFont.lfFaceName))
							{
								lbFontJustRegistered = lbFound = true; // OK, добавили
							}
						}
					}

					if (lbFound)
					{
						SetFocus(GetDlgItem(hWnd2, tConsoleFontFace));
						EnableWindow(GetDlgItem(hWnd2, bConFontAdd2HKLM), FALSE);
						gpSetCls->nConFontError &= ~(DWORD)ConFontErr_NonRegistry;

						if (lbFontJustRegistered)
						{
							// Если шрифт только что зарегистрировали - его нельзя использовать до перезагрузки компьютера
							if (lbFontJustRegistered && gpSetCls->sDefaultConFontName[0])
							{
								wcscpy_c(gpSet->ConsoleFont.lfFaceName, gpSetCls->sDefaultConFontName);

								if (SelectString(hWnd2, tConsoleFontFace, gpSet->ConsoleFont.lfFaceName)<0)
									SetDlgItemText(hWnd2, tConsoleFontFace, gpSet->ConsoleFont.lfFaceName);

								EnableWindow(GetDlgItem(hWnd2, bConFontOK), TRUE);
								SetFocus(GetDlgItem(hWnd2, bConFontOK));
							}
						}
						else
						{
							EnableWindow(GetDlgItem(hWnd2, bConFontOK), TRUE);
							SetFocus(GetDlgItem(hWnd2, bConFontOK));
						}
					}
				}

				return 0;
			}
			else if (HIWORD(wParam) == CBN_EDITCHANGE || HIWORD(wParam) == CBN_SELCHANGE)
			{
				PostMessage(hWnd2, (WM_APP+47), wParam, lParam);
			}

			break;
		case(WM_APP+47):

			if (HIWORD(wParam) == CBN_EDITCHANGE || HIWORD(wParam) == CBN_SELCHANGE)
			{
				wchar_t szCreatedFaceName[32] = {0};
				WORD TB = LOWORD(wParam);
				LOGFONT LF = {0};
				LF.lfOutPrecision = OUT_TT_PRECIS;
				LF.lfClipPrecision = CLIP_DEFAULT_PRECIS;
				LF.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
				GetDlgItemText(hWnd2, tConsoleFontFace, LF.lfFaceName, countof(LF.lfFaceName));
				LF.lfHeight = GetNumber(hWnd2, tConsoleFontSizeY);

				if (TB != tConsoleFontSizeY)
					LF.lfWidth = GetNumber(hWnd2, tConsoleFontSizeX);

				LF.lfWeight = FW_NORMAL;
				gpSetCls->nConFontError = 0; //ConFontErr_NonSystem|ConFontErr_NonRegistry|ConFontErr_InvalidName;
				int nIdx = SendDlgItemMessage(hWnd2, tConsoleFontFace, CB_FINDSTRINGEXACT, -1, (LPARAM)LF.lfFaceName); //-V103

				if (nIdx < 0)
				{
					gpSetCls->nConFontError = ConFontErr_NonSystem;
				}
				else
				{
					HFONT hf = CreateFontIndirect(&LF);

					if (!hf)
					{
						EnableWindow(GetDlgItem(hWnd2, bConFontOK), FALSE);
						gpSetCls->nConFontError = ConFontErr_InvalidName;
					}
					else
					{
						LPOUTLINETEXTMETRICW lpOutl = gpSetCls->LoadOutline(NULL, hf);

						if (!lpOutl)
						{
							// Ошибка
							gpSetCls->nConFontError = ConFontErr_InvalidName;
						}
						else
						{
							wcscpy_c(szCreatedFaceName, (wchar_t*)lpOutl->otmpFamilyName);
							wchar_t temp[10];

							if (TB != tConsoleFontSizeX)
							{
								_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", lpOutl->otmTextMetrics.tmAveCharWidth);
								SelectStringExact(hWnd2, tConsoleFontSizeX, temp);
							}

							if (lpOutl->otmTextMetrics.tmHeight != LF.lfHeight)
							{
								_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", lpOutl->otmTextMetrics.tmHeight);
								SelectStringExact(hWnd2, tConsoleFontSizeY, temp);
							}

							free(lpOutl); lpOutl = NULL;

							if (lstrcmpi(szCreatedFaceName, LF.lfFaceName))
								gpSetCls->nConFontError |= ConFontErr_InvalidName;
						}

						DeleteObject(hf);
					}
				}

				if (gpSetCls->nConFontError == 0)
				{
					// Осталось проверить регистрацию в реестре
					wcscpy_c(gpSet->ConsoleFont.lfFaceName, LF.lfFaceName);
					gpSet->ConsoleFont.lfHeight = LF.lfHeight;
					gpSet->ConsoleFont.lfWidth = LF.lfWidth;
					bool lbRegChecked = CheckConsoleFontRegistry(gpSet->ConsoleFont.lfFaceName);

					if (!lbRegChecked) gpSetCls->nConFontError |= ConFontErr_NonRegistry;

					EnableWindow(GetDlgItem(hWnd2, bConFontOK), lbRegChecked);
					EnableWindow(GetDlgItem(hWnd2, bConFontAdd2HKLM), !lbRegChecked);
				}
				else
				{
					EnableWindow(GetDlgItem(hWnd2, bConFontOK), FALSE);
					EnableWindow(GetDlgItem(hWnd2, bConFontAdd2HKLM), FALSE);
				}

				ShowConFontErrorTip(gpSetCls->CreateConFontError(LF.lfFaceName, szCreatedFaceName));
			}

			break;
		case WM_ACTIVATE:

			if (LOWORD(wParam) == WA_INACTIVE)
				ShowConFontErrorTip(NULL);
			else if (gpSetCls->bShowConFontError)
			{
				gpSetCls->bShowConFontError = FALSE;
				ShowConFontErrorTip(gpSetCls->CreateConFontError(NULL,NULL));
			}

			break;
	}

	return 0;
}

void CSettings::ShowConFontErrorTip(LPCTSTR asInfo)
{
	ShowErrorTip(asInfo, gpSetCls->hConFontDlg, tConsoleFontFace, gpSetCls->sConFontError, countof(gpSetCls->sConFontError),
	             gpSetCls->hwndConFontBalloon, &gpSetCls->tiConFontBalloon, NULL, FAILED_CONFONT_TIMEOUT);
	//if (!asInfo)
	//	gpSetCls->sConFontError[0] = 0;
	//else if (asInfo != gpSetCls->sConFontError)
	//	lstrcpyn(gpSetCls->sConFontError, asInfo, countof(gpSetCls->sConFontError));
	//tiConFontBalloon.lpszText = gpSetCls->sConFontError;
	//if (gpSetCls->sConFontError[0]) {
	//	SendMessage(hwndConFontBalloon, TTM_UPDATETIPTEXT, 0, (LPARAM)&tiConFontBalloon);
	//	RECT rcControl; GetWindowRect(GetDlgItem(hConFontDlg, tConsoleFontFace), &rcControl);
	//	int ptx = rcControl.right - 10;
	//	int pty = (rcControl.top + rcControl.bottom) / 2;
	//	SendMessage(hwndConFontBalloon, TTM_TRACKPOSITION, 0, MAKELONG(ptx,pty));
	//	SendMessage(hwndConFontBalloon, TTM_TRACKACTIVATE, TRUE, (LPARAM)&tiConFontBalloon);
	//	SetTimer(hConFontDlg, FAILED_FONT_TIMERID, FAILED_FONT_TIMEOUT, 0);
	//} else {
	//	SendMessage(hwndConFontBalloon, TTM_TRACKACTIVATE, FALSE, (LPARAM)&tiConFontBalloon);
	//}
}

LPCWSTR CSettings::CreateConFontError(LPCWSTR asReqFont/*=NULL*/, LPCWSTR asGotFont/*=NULL*/)
{
	sConFontError[0] = 0;

	if (!nConFontError)
		return NULL;

	SendMessage(gpSetCls->hwndConFontBalloon, TTM_SETTITLE, TTI_WARNING, (LPARAM)(asReqFont ? asReqFont : gpSet->ConsoleFont.lfFaceName));
	wcscpy_c(sConFontError, L"Console font test failed!\n");
	//wcscat_c(sConFontError, asReqFont ? asReqFont : ConsoleFont.lfFaceName);
	//wcscat_c(sConFontError, L"\n");

	if ((nConFontError & ConFontErr_InvalidName))
	{
		if (asReqFont && asGotFont)
		{
			int nCurLen = _tcslen(sConFontError);
			_wsprintf(sConFontError+nCurLen, SKIPLEN(countof(sConFontError)-nCurLen)
			          L"Requested: %s\nCreated: %s\n", asReqFont , asGotFont);
		}
		else
		{
			wcscat_c(sConFontError, L"Invalid font face name!\n");
		}
	}

	if ((nConFontError & ConFontErr_NonSystem))
		wcscat_c(sConFontError, L"Font is non public or non Unicode\n");

	if ((nConFontError & ConFontErr_NonRegistry))
		wcscat_c(sConFontError, L"Font is not registered for use in console\n");

	sConFontError[_tcslen(sConFontError)-1] = 0;
	return sConFontError;
}

int CSettings::EnumConFamCallBack(LPLOGFONT lplf, LPNEWTEXTMETRIC lpntm, DWORD FontType, LPVOID aFontCount)
{
	MCHKHEAP
	HWND hWnd2 = (HWND)aFontCount;

	// Интересуют только TrueType
	if ((FontType & TRUETYPE_FONTTYPE) == 0)
		return TRUE;

	if (lplf->lfFaceName[0] == L'@')
		return TRUE; // Alias?

	// Нельзя использовать шрифты, которые зарегистрированы нами (для ConEmu). Они должны быть системными
	for (std::vector<RegFont>::iterator iter = gpSetCls->m_RegFonts.begin(); iter != gpSetCls->m_RegFonts.end(); ++iter)
	{
		if (!iter->bAlreadyInSystem &&
		        lstrcmpi(iter->szFontName, lplf->lfFaceName) == 0)
			return TRUE;
	}

	// PAN_PROP_MONOSPACED - не дает правильного результата. Например 'MS Mincho' заявлен как моноширный,
	// но совсем таковым не является. Кириллица у него дофига какая...
	// И только моноширные!
	DWORD bAlmostMonospace = IsAlmostMonospace(lplf->lfFaceName, lpntm->tmMaxCharWidth, lpntm->tmAveCharWidth, lpntm->tmHeight) ? 1 : 0;

	if (!bAlmostMonospace)
		return TRUE;

	// Проверяем, реальное ли это имя. Или просто алиас?
	LOGFONT LF = {0};
	LF.lfHeight = 10; LF.lfWidth = 0; LF.lfWeight = 0; LF.lfItalic = 0;
	LF.lfOutPrecision = OUT_TT_PRECIS;
	LF.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	LF.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
	wcscpy_c(LF.lfFaceName, lplf->lfFaceName);
	HFONT hf = CreateFontIndirect(&LF);

	if (!hf) return TRUE;  // не получилось создать

	LPOUTLINETEXTMETRICW lpOutl = gpSetCls->LoadOutline(NULL, hf);

	if (!lpOutl) return TRUE;  // Ошибка получения параметров шрифта

	if (lpOutl->otmPanoseNumber.bProportion != PAN_PROP_MONOSPACED  // шрифт не заявлен как моноширный
	        || lstrcmpi((wchar_t*)lpOutl->otmpFamilyName, LF.lfFaceName)) // или алиас
	{
		free(lpOutl);
		return TRUE; // следущий шрифт
	}

	free(lpOutl); lpOutl = NULL;

	// Сравниваем с текущим, выбранным в настройке
	if (lstrcmpi(LF.lfFaceName, gpSet->ConsoleFont.lfFaceName) == 0)
		gpSetCls->nConFontError &= ~(DWORD)ConFontErr_NonSystem;

	if (hWnd2)
	{
		if (SendDlgItemMessage(hWnd2, tConsoleFontFace, CB_FINDSTRINGEXACT, -1, (LPARAM) LF.lfFaceName)==-1)
		{
			int nIdx;
			nIdx = SendDlgItemMessage(hWnd2, tConsoleFontFace, CB_ADDSTRING, 0, (LPARAM) LF.lfFaceName); //-V103
		}
	}

	if (gpSetCls->sDefaultConFontName[0] == 0)
	{
		if (CheckConsoleFontRegistry(LF.lfFaceName))
			wcscpy_c(gpSetCls->sDefaultConFontName, LF.lfFaceName);
	}

	MCHKHEAP
	return TRUE;
	UNREFERENCED_PARAMETER(lpntm);
}

//bool CSettings::NeedCreateAppWindow()
//{
//	// Пока что, окно для Application нужно создавать только для XP и ниже
//	// в том случае, если на таскбаре отображаются кнопки запущенных консолей
//	// Это для того, чтобы при Alt-Tab не светилась "лишняя" иконка главного окна
//	if ((gOSVer.dwMajorVersion == 5) && isTabsOnTaskBar())
//		return TRUE;
//	return FALSE;
//}

//bool CSettings::isTabsOnTaskBar()
//{
//	if (gpSet->isDesktopMode)
//		return false;
//	if ((gpSet->m_isTabsOnTaskBar == 1) || (((BYTE)gpSet->isTabsOnTaskBar() > 1) && (gOSVer.dwMajorVersion >= 6)))
//		return true;
//	return false;
//}
