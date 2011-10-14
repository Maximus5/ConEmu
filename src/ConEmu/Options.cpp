
/*
Copyright (c) 2009-2011 Maximus5
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
#include "version.h"


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

const u8 chSetsNums[] = {0, 178, 186, 136, 1, 238, 134, 161, 177, 129, 130, 77, 255, 204, 128, 2, 222, 162, 163};
const char *ChSets[] = {"ANSI", "Arabic", "Baltic", "Chinese Big 5", "Default", "East Europe",
                        "GB 2312", "Greek", "Hebrew", "Hangul", "Johab", "Mac", "OEM", "Russian", "Shiftjis",
                        "Symbol", "Thai", "Turkish", "Vietnamese"
                       };
const WORD HostkeyCtrlIds[] = {cbHostWin, cbHostApps, cbHostLCtrl, cbHostRCtrl, cbHostLAlt, cbHostRAlt, cbHostLShift, cbHostRShift};
//int upToFontHeight=0;
HWND ghOpWnd=NULL;

#ifdef _DEBUG
#define HEAPVAL //HeapValidate(GetProcessHeap(), 0, NULL);
#else
#define HEAPVAL
#endif

#define DEFAULT_FADE_LOW 0
#define DEFAULT_FADE_HIGH 0xD0


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



namespace Settings
{
	const WCHAR* szKeys[] = {L"<None>", L"Left Ctrl", L"Right Ctrl", L"Left Alt", L"Right Alt", L"Left Shift", L"Right Shift"};
	const DWORD  nKeys[] =  {0,         VK_LCONTROL,  VK_RCONTROL,   VK_LMENU,    VK_RMENU,     VK_LSHIFT,     VK_RSHIFT};
	const WCHAR* szKeysAct[] = {L"<Always>", L"Left Ctrl", L"Right Ctrl", L"Left Alt", L"Right Alt", L"Left Shift", L"Right Shift"};
	const DWORD  nKeysAct[] =  {0,         VK_LCONTROL,  VK_RCONTROL,   VK_LMENU,    VK_RMENU,     VK_LSHIFT,     VK_RSHIFT};
	const BYTE   FSizes[] = {0, 8, 9, 10, 11, 12, 14, 16, 18, 19, 20, 24, 26, 28, 30, 32, 34, 36, 40, 46, 50, 52, 72};
	const BYTE   FSizesSmall[] = {5, 6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 19, 20, 24, 26, 28, 30, 32};
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
	
};

#define FillListBox(hDlg,nDlgID,Items,Values,Value) \
	_ASSERTE(countof(Items) == countof(Values)); \
	FillListBoxItems(GetDlgItem(hDlg,nDlgID), countof(Items), Items, Values, Value)
#define FillListBoxByte(hDlg,nDlgID,Items,Values,Value) \
	_ASSERTE(countof(Items) == countof(Values)); { \
		DWORD dwVal = Value; \
		FillListBoxItems(GetDlgItem(hDlg,nDlgID), countof(Items), Items, Values, dwVal); \
		Value = dwVal; }

#define GetListBox(hDlg,nDlgID,Items,Values,Value) \
	_ASSERTE(countof(Items) == countof(Values)); \
	GetListBoxItem(GetDlgItem(hDlg,nDlgID), countof(Items), Items, Values, Value)
#define GetListBoxByte(hDlg,nDlgID,Items,Values,Value) \
	_ASSERTE(countof(Items) == countof(Values)); { \
		DWORD dwVal = Value; \
		GetListBoxItem(GetDlgItem(hDlg,nDlgID), countof(Items), Items, Values, dwVal); \
		Value = dwVal; }

#define SetThumbColor(s,rgb,idx,us) { (s).RawColor = 0; (s).ColorRGB = rgb; (s).ColorIdx = idx; (s).UseIndex = us; }
#define SetThumbSize(s,sz,x1,y1,x2,y2,ls,lp,fn,fs) { \
		(s).nImgSize = sz; (s).nSpaceX1 = x1; (s).nSpaceY1 = y1; (s).nSpaceX2 = x2; (s).nSpaceY2 = y2; \
		(s).nLabelSpacing = ls; (s).nLabelPadding = lp; wcscpy_c((s).sFontName,fn); (s).nFontHeight=fs; }
#define ThumbLoadSet(s,n) { \
		reg->Load(L"PanView." s L".ImgSize", n.nImgSize); \
		reg->Load(L"PanView." s L".SpaceX1", n.nSpaceX1); \
		reg->Load(L"PanView." s L".SpaceY1", n.nSpaceY1); \
		reg->Load(L"PanView." s L".SpaceX2", n.nSpaceX2); \
		reg->Load(L"PanView." s L".SpaceY2", n.nSpaceY2); \
		reg->Load(L"PanView." s L".LabelSpacing", n.nLabelSpacing); \
		reg->Load(L"PanView." s L".LabelPadding", n.nLabelPadding); \
		reg->Load(L"PanView." s L".FontName", n.sFontName, countof(n.sFontName)); \
		reg->Load(L"PanView." s L".FontHeight", n.nFontHeight); }
#define ThumbSaveSet(s,n) { \
		reg->Save(L"PanView." s L".ImgSize", n.nImgSize); \
		reg->Save(L"PanView." s L".SpaceX1", n.nSpaceX1); \
		reg->Save(L"PanView." s L".SpaceY1", n.nSpaceY1); \
		reg->Save(L"PanView." s L".SpaceX2", n.nSpaceX2); \
		reg->Save(L"PanView." s L".SpaceY2", n.nSpaceY2); \
		reg->Save(L"PanView." s L".LabelSpacing", n.nLabelSpacing); \
		reg->Save(L"PanView." s L".LabelPadding", n.nLabelPadding); \
		reg->Save(L"PanView." s L".FontName", n.sFontName); \
		reg->Save(L"PanView." s L".FontHeight", n.nFontHeight); }


CSettings::CSettings()
{
	gpSet = this; // сразу!
	//HeapInitialize();
	InitSettings();
	SingleInstanceArg = false;
	mb_StopRegisterFonts = FALSE;
	mb_IgnoreEditChanged = FALSE;
	mb_IgnoreTtfChange = TRUE;
	mb_CharSetWasSet = FALSE;
	mb_TabHotKeyRegistered = FALSE;
	hMain = hExt = hTabs = hKeys = hColors = hViews = hInfo = hDebug = NULL; hwndTip = NULL; hwndBalloon = NULL;
	hConFontDlg = NULL; hwndConFontBalloon = NULL; bShowConFontError = FALSE; sConFontError[0] = sDefaultConFontName[0] = 0; bConsoleFontChecked = FALSE;
	QueryPerformanceFrequency((LARGE_INTEGER *)&mn_Freq);
	memset(mn_Counter, 0, sizeof(*mn_Counter)*(tPerfInterval-gbPerformance));
	memset(mn_CounterMax, 0, sizeof(*mn_CounterMax)*(tPerfInterval-gbPerformance));
	memset(mn_FPS, 0, sizeof(mn_FPS)); mn_FPS_CUR_FRAME = 0;
	memset(mn_RFPS, 0, sizeof(mn_RFPS)); mn_RFPS_CUR_FRAME = 0;
	memset(mn_CounterTick, 0, sizeof(*mn_CounterTick)*(tPerfInterval-gbPerformance));
	//hBgBitmap = NULL; bgBmp = MakeCoord(0,0); hBgDc = NULL;
	isBackgroundImageValid = false;
	mb_NeedBgUpdate = FALSE; mb_WasVConBgImage = FALSE; mb_BgLastFade = false;
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
	mn_FadeMul = mn_FadeHigh - mn_FadeLow;
	mn_LastFadeSrc = mn_LastFadeDst = -1;

	try
	{
		GetStartupInfoW(&ourSI);
	}
	catch(...)
	{
		memset(&ourSI, 0, sizeof(ourSI));
	}

	mn_LastChangingFontCtrlId = 0;
	SetWindowThemeF = NULL;
	mh_Uxtheme = LoadLibrary(L"UxTheme.dll");

	if (mh_Uxtheme)
	{
		SetWindowThemeF = (SetWindowThemeT)GetProcAddress(mh_Uxtheme, "SetWindowTheme");
		EnableThemeDialogTextureF = (EnableThemeDialogTextureT)GetProcAddress(mh_Uxtheme, "EnableThemeDialogTexture");
		//if (SetWindowThemeF) { SetWindowThemeF(Progressbar1, L" ", L" "); }
	}

	mn_MsgUpdateCounter = RegisterWindowMessage(L"ConEmuSettings::Counter");
	mn_MsgRecreateFont = RegisterWindowMessage(L"CSettings::RecreateFont");
	mn_MsgLoadFontFromMain = RegisterWindowMessage(L"CSettings::LoadFontNames");
	mn_ActivateTabMsg = RegisterWindowMessage(L"CSettings::ActivateTab");
	mh_EnumThread = NULL;
	mh_CtlColorBrush = NULL;
	
	// Горячие клавиши
	TODO("Дополнить системные комбинации");
	WARNING("У nLDragKey,nRDragKey был тип DWORD");
	ConEmuHotKeys HotKeys[] = {
		// User (Keys)
		{vkMinimizeRestore, 0, &icMinimizeRestore},
		{vkMultiNew, 0, &icMultiNew},
		{vkMultiNewShift, 0, &icMultiNew},
		{vkMultiNext, 0, &icMultiNext},
		{vkMultiNextShift, 0, &icMultiNext},
		{vkMultiRecreate, 0, &icMultiRecreate},
		{vkMultiBuffer, 0, &icMultiBuffer},
		{vkMultiClose, 0, &icMultiClose},
		{vkMultiCmd, 0, &icMultiCmd},
		// User (Modifiers)
		{vkCTSVkBlock, 1, &isCTSVkBlock},      // модификатор запуска выделения мышкой
		{vkCTSVkText, 1, &isCTSVkText},       // модификатор запуска выделения мышкой
		{vkCTSVkAct, 1, &isCTSVkAct},        // модификатор разрешения действий правой и средней кнопки мышки
		{vkFarGotoEditorVk, 1, &isFarGotoEditorVk}, // модификатор для isFarGotoEditor
		{vkLDragKey, 1, &nLDragKey},         // модификатор драга левой кнопкой
		{vkRDragKey, 1, &nRDragKey},         // модификатор драга правой кнопкой
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
}

CSettings::~CSettings()
{
	for(int i=0; i<MAX_FONT_STYLES; i++)
	{
		if (mh_Font[i]) { DeleteObject(mh_Font[i]); mh_Font[i] = NULL; }

		if (m_otm[i]) {free(m_otm[i]); m_otm[i] = NULL;}
	}

	if (mh_Font2) { DeleteObject(mh_Font2); mh_Font2 = NULL; }

	if (psCmd) {free(psCmd); psCmd = NULL;}

	if (psCmdHistory) {free(psCmdHistory); psCmdHistory = NULL;}

	if (psCurCmd) {free(psCurCmd); psCurCmd = NULL;}

	if (sTabCloseMacro) {free(sTabCloseMacro); sTabCloseMacro = NULL;}

	if (sSaveAllMacro) {free(sSaveAllMacro); sSaveAllMacro = NULL;}

	if (sRClickMacro) {free(sRClickMacro); sRClickMacro = NULL;}

	if (mh_Uxtheme!=NULL) { FreeLibrary(mh_Uxtheme); mh_Uxtheme = NULL; }

	if (mh_CtlColorBrush) { DeleteObject(mh_CtlColorBrush); mh_CtlColorBrush = NULL; }

	if (mp_Bg) { delete mp_Bg; mp_Bg = NULL; }

	SafeFree(mp_BgImgData);
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

void CSettings::InitSettings()
{
	MCHKHEAP
//------------------------------------------------------------------------
///| Moved from CVirtualConsole |/////////////////////////////////////////
//------------------------------------------------------------------------
	wcscpy_c(ConfigPath, CONEMU_ROOT_KEY L"\\.Vanilla");
	ConfigName[0] = 0;
	Type[0] = 0;
	//FontFile[0] = 0;
	isAutoRegisterFonts = true;
	psCmd = NULL; psCurCmd = NULL; wcscpy_c(szDefCmd, L"far");
	psCmdHistory = NULL; nCmdHistorySize = 0;
	isMulti = true; icMultiNew = 'W'; icMultiNext = 'Q'; icMultiRecreate = 192/*VK_тильда*/; icMultiBuffer = 'A';
	icMinimizeRestore = 'C';
	icMultiClose = 0/*VK_DELETE*/; icMultiCmd = 'X'; isMultiAutoCreate = false; isMultiLeaveOnClose = false; isMultiIterate = true;
	isMultiNewConfirm = true; isUseWinNumber = true; isUseWinArrows = false; isUseWinTab = false; nMultiHotkeyModifier = VK_LWIN; TestHostkeyModifiers();
	m_isKeyboardHooks = 0;
	isFARuseASCIIsort = false; isFixAltOnAltTab = false; isShellNoZoneCheck = false;
	isFadeInactive = true; mn_FadeLow = DEFAULT_FADE_LOW; mn_FadeHigh = DEFAULT_FADE_HIGH; mb_FadeInitialized = false;
	mn_LastFadeSrc = mn_LastFadeDst = -1;
	//nFadeInactiveMask = 0xD0D0D0;
	// Logging
	isAdvLogging = 0;
	m_RealConLoggingType = glt_None;
	//wcscpy_c(szDumpPackets, L"c:\\temp\\ConEmuVCon-%i-%i.dat");
	nMainTimerElapse = 10;
	nMainTimerInactiveElapse = 1000;
	nAffinity = 0; // 0 - don't change default affinity
	//isAdvLangChange = true;
	isSkipFocusEvents = false;
	isSendAltEnter = isSendAltSpace = isSendAltTab = isSendAltEsc = isSendAltPrintScrn = isSendPrintScrn = isSendCtrlEsc = false;
	//isLangChangeWsPlugin = false;
	isMonitorConsoleLang = 3;
	DefaultBufferHeight = 1000; AutoBufferHeight = true;
	//FarSyncSize = true;
	nCmdOutputCP = 0;
	bForceBufferHeight = false; nForceBufferHeight = 1000; /* устанавливается в true, из ком.строки /BufferHeight */
	AutoScroll = true;

	// Шрифты
	memset(m_Fonts, 0, sizeof(m_Fonts));
	//TODO: OLD - на переделку
	memset(&LogFont, 0, sizeof(LogFont));
	memset(&LogFont2, 0, sizeof(LogFont2));
	LogFont.lfHeight = mn_FontHeight = FontSizeY = 16;
	LogFont.lfWidth = mn_FontWidth = FontSizeX = mn_BorderFontWidth = 0;
	FontSizeX2 = 0; FontSizeX3 = 0;
	LogFont.lfEscapement = LogFont.lfOrientation = 0;
	LogFont.lfWeight = FW_NORMAL;
	LogFont.lfItalic = LogFont.lfUnderline = LogFont.lfStrikeOut = FALSE;
	LogFont.lfCharSet = DEFAULT_CHARSET;
	LogFont.lfOutPrecision = OUT_TT_PRECIS;
	LogFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	LogFont.lfQuality = ANTIALIASED_QUALITY;
	LogFont.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
	wcscpy_c(LogFont.lfFaceName, L"Lucida Console");
	wcscpy_c(LogFont2.lfFaceName, L"Lucida Console");
	mb_Name1Ok = FALSE; mb_Name2Ok = FALSE;
	isTryToCenter = false;
	isAlwaysShowScrollbar = 2;
	isTabFrame = true;
	//isForceMonospace = false; isProportional = false;
	isMonospace = 1;
	isMinToTray = false; isAlwaysShowTrayIcon = false;
	memset(&rcTabMargins, 0, sizeof(rcTabMargins));
	isFontAutoSize = false; mn_AutoFontWidth = mn_AutoFontHeight = -1;
	ConsoleFont.lfHeight = 5;
	ConsoleFont.lfWidth = 3;
	wcscpy_c(ConsoleFont.lfFaceName, L"Lucida Console");
	{
		SettingsRegistry RegConColors, RegConDef;

		if (RegConColors.OpenKey(L"Console", KEY_READ))
		{
			RegConDef.OpenKey(HKEY_USERS, L".DEFAULT\\Console", KEY_READ);
			TCHAR ColorName[] = L"ColorTable00";
			bool  lbBlackFound = false;

			for(uint i = 0x10; i--;)
			{
				// L"ColorTableNN"
				ColorName[10] = i/10 + '0';
				ColorName[11] = i%10 + '0';

				if (!RegConColors.Load(ColorName, (LPBYTE)&(Colors[i]), sizeof(Colors[0])))
					if (!RegConDef.Load(ColorName, (LPBYTE)&(Colors[i]), sizeof(Colors[0])))
						Colors[i] = dwDefColors[i]; //-V108

				if (Colors[i] == 0)
				{
					if (!lbBlackFound)
						lbBlackFound = true;
					else if (lbBlackFound)
						Colors[i] = dwDefColors[i]; //-V108
				}

				Colors[i+0x10] = Colors[i]; // Умолчания
			}

			RegConDef.CloseKey();
			RegConColors.CloseKey();
		}
	}
	isTrueColorer = false;
	isExtendColors = false;
	nExtendColor = 14;
	isExtendFonts = false;
#ifdef _DEBUG
	isExtendFonts = true;
#endif
	nFontNormalColor = 1; nFontBoldColor = 12; nFontItalicColor = 13;
	//CheckTheming(); -- сейчас - нельзя. нужно дождаться, пока главное окно будет создано
	mb_ThemingEnabled = (gOSVer.dwMajorVersion >= 6 || (gOSVer.dwMajorVersion == 5 && gOSVer.dwMinorVersion >= 1));
//------------------------------------------------------------------------
///| Default settings |///////////////////////////////////////////////////
//------------------------------------------------------------------------
	isShowBgImage = 0;
	wcscpy_c(sBgImage, L"c:\\back.bmp");
	bgImageDarker = 255; // 0x46;
	//nBgImageColors = 1|2; синий,зеленый
	//nBgImageColors = 2; // только синий
	nBgImageColors = (DWORD)-1; // Получить цвет панелей из фара - иначе "1|2" == BgImageColorsDefaults.
	bgOperation = eUpLeft;
	isBgPluginAllowed = 1;
	nTransparent = 255;
	//isColorKey = false;
	//ColorKey = RGB(1,1,1);
	isUserScreenTransparent = false;
	isFixFarBorders = 1; isEnhanceGraphics = true;
	isPartBrush75 = 0xC8; isPartBrush50 = 0x96; isPartBrush25 = 0x5A;
	isPartBrushBlack = 32; //-V112
	isExtendUCharMap = true;
	isDownShowHiddenMessage = false;
	//memset(icFixFarBorderRanges, 0, sizeof(icFixFarBorderRanges));
	//wcscpy_c(mszCharRanges, L"2013-25C4");
	//icFixFarBorderRanges[0].bUsed = true; icFixFarBorderRanges[0].cBegin = 0x2013; icFixFarBorderRanges[0].cEnd = 0x25C4;
	//mpc_FixFarBorderValues = (bool*)calloc(65536,sizeof(bool));
	ParseCharRanges(L"2013-25C4", mpc_FixFarBorderValues);
	wndHeight = 25;
	ntvdmHeight = 0; // Подбирать автоматически
	wndWidth = 80;
	//WindowMode=rNormal; -- устанавливается в конструкторе CConEmuMain
	isFullScreen = false;
	isHideCaption = mb_HideCaptionAlways = false;
	nHideCaptionAlwaysFrame = 1; nHideCaptionAlwaysDelay = 2000; nHideCaptionAlwaysDisappear = 2000;
	isDesktopMode = false;
	isAlwaysOnTop = false;
	isSleepInBackground = false; // по умолчанию - не включать "засыпание в фоне".
	wndX = 0; wndY = 0; wndCascade = true; isAutoSaveSizePos = false;
	isConVisible = false; //isLockRealConsolePos = false;
	//WARNING("isUseInjects в релизе по умолчанию отключен");
	//#ifdef _DEBUG
	isUseInjects = true;
	//#else
	//isUseInjects = false;
	//#endif
	#ifdef USEPORTABLEREGISTRY
	isPortableReg = true; // включено по умолчанию
	#else
	isPortableReg = false;
	#endif
	nConInMode = (DWORD)-1; // по умолчанию, включится (ENABLE_QUICK_EDIT_MODE|ENABLE_EXTENDED_FLAGS|ENABLE_INSERT_MODE)
	nSlideShowElapse = 2500;
	nIconID = IDI_ICON1;
	isRClickSendKey = 2;
	sRClickMacro = NULL;
	wcscpy_c(szTabConsole, L"%s");
	//pszTabConsole = wcscpy_c(szTabPanels+_tcslen(szTabPanels)+1, L"Console");
	wcscpy_c(szTabEditor, L"[%s]");
	wcscpy_c(szTabEditorModified, L"[%s] *");
	/* */ wcscpy_c(szTabViewer, L"{%s}");
	nTabLenMax = 20;
	isSafeFarClose = true;
	sSafeFarCloseMacro = NULL; // если NULL - то используется макрос по умолчанию
	isCursorV = true;
	isCursorBlink = true;
	isCursorColor = false;
	isCursorBlockInactive = true;
	isConsoleTextSelection = 1; // Always
	isCTSSelectBlock = true; isCTSVkBlock = VK_LMENU; // по умолчанию - блок выделяется c LAlt
	isCTSSelectText = true; isCTSVkText = VK_LSHIFT; // а текст - при нажатом LShift
	isCTSActMode = 2; // BufferOnly
	isCTSVkAct = 0; // т.к. по умолчанию - только BufferOnly, то вообще без модификаторов
	isCTSRBtnAction = 3; // Auto (Выделения нет - Paste, Есть - Copy)
	isCTSMBtnAction = 0; // <None>
	isCTSColorIndex = 0xE0;
	isFarGotoEditor = true; isFarGotoEditorVk = VK_LCONTROL;
	isTabs = 1; isTabSelf = true; isTabRecent = true; isTabLazy = true;
	isTabsInCaption = false; //cbTabsInCaption
	wcscpy_c(sTabFontFace, L"Tahoma"); nTabFontCharSet = ANSI_CHARSET; nTabFontHeight = 16;
	sTabCloseMacro = sSaveAllMacro = NULL;
	nToolbarAddSpace = 0;
	//isVisualizer = false;
	//nVizNormal = 1; nVizFore = 15; nVizTab = 15; nVizEOL = 8; nVizEOF = 12;
	//cVizTab = 0x2192; cVizEOL = 0x2193; cVizEOF = 0x2640;
	isAllowDetach = 0;
	isCreateAppWindow = false;
	/*isScrollTitle = true;
	ScrollTitleLen = 22;*/
	wcscpy_c(szAdminTitleSuffix, L" (Admin)");
	bAdminShield = true;
	bHideInactiveConsoleTabs = false;
	isDisableMouse = false;
	isRSelFix = true; isMouseSkipActivation = true; isMouseSkipMoving = true;
	isFarHourglass = true; nFarHourglassDelay = 500;
	isDisableFarFlashing = false; isDisableAllFlashing = false;
	isDragEnabled = DRAG_L_ALLOWED; isDropEnabled = (BYTE)1; isDefCopy = true;
	nLDragKey = 0; nRDragKey = VK_LCONTROL;
	isDragOverlay = 1; isDragShowIcons = true;
	// изменение размера панелей мышкой
	isDragPanel = 2; // по умолчанию сделаем чтобы драгалось макросами (вдруг у юзера на Ctrl-Left/Right/Up/Down макросы висят... как бы конфуза не получилось)
	isDebugSteps = true;
	MCHKHEAP
	// Thumbnails
	memset(&ThSet, 0, sizeof(ThSet));
	ThSet.cbSize = sizeof(ThSet);
	// фон превьюшки: RGB или Index
	SetThumbColor(ThSet.crBackground, RGB(255,255,255), 16, FALSE);
	// серая рамка вокруг превьюшки
	ThSet.nPreviewFrame = 1;
	SetThumbColor(ThSet.crPreviewFrame, RGB(128,128,128), 8, TRUE);
	// рамка вокруг текущего элемента
	ThSet.nSelectFrame = 1;
	SetThumbColor(ThSet.crSelectFrame, RGB(192,192,192), 7, FALSE);
	/* теперь разнообразные размеры */
	// отступы для preview
	SetThumbSize(ThSet.Thumbs,96,1,1,5,20,2,0,L"Tahoma",14);
	// отступы для tiles
	SetThumbSize(ThSet.Tiles,48,4,4,172,4,4,1,L"Tahoma",14); //-V112
	// Прочие параметры загрузки
	ThSet.bLoadPreviews = 3;   // bitmask of {1=Thumbs, 2=Tiles}
	ThSet.bLoadFolders = true; // true - load infolder previews (only for Thumbs)
	ThSet.nLoadTimeout = 15;   // 15 sec
	ThSet.nMaxZoom = 600; // %%
	ThSet.bUsePicView2 = true;
	ThSet.bRestoreOnStartup = false;
	//// Пока не используется
	//DWORD nCacheFolderType; // юзер/программа/temp/и т.п.
	//wchar_t sCacheFolder[MAX_PATH];
	m_ThSetMap.InitName(CECONVIEWSETNAME, GetCurrentProcessId());

	if (!m_ThSetMap.Create())
	{
		MBoxA(m_ThSetMap.GetErrorText());
	}
	else
	{
		// Применить в Mapping (там заодно и палитра копируется)
		//m_ThSetMap.Set(&ThSet);
		//!! Это нужно делать после создания основного шрифта
		//gpConEmu->OnPanelViewSettingsChanged(FALSE);
	}
}

void CSettings::LoadSettings()
{
	MCHKHEAP
	FontSizeY = LogFont.lfHeight;
	TCHAR inFont[MAX_PATH], inFont2[MAX_PATH];
	wcscpy_c(inFont, LogFont.lfFaceName);
	wcscpy_c(inFont2, LogFont2.lfFaceName);
	//DWORD FontRangeCount = countof(m_Fonts);
	DWORD Quality = LogFont.lfQuality;
	//gpConEmu->WindowMode = rMaximized;
	mn_LoadFontCharSet = LogFont.lfCharSet;
	mb_CharSetWasSet = FALSE;
	bool isBold = (LogFont.lfWeight>=FW_BOLD), isItalic = (LogFont.lfItalic!=FALSE);

	//;; Q. В Windows Vista зависают другие консольные процессы.
	//	;; A. "Виноват" процесс ConIme.exe. Вроде бы он служит для ввода иероглифов
	//	;;    (китай и т.п.). Зачем он нужен, если ввод теперь идет в графическом окне?
	//	;;    Нужно запретить его автозапуск или вообще переименовать этот файл, например
	//	;;    в 'ConIme.ex1' (видимо это возможно только в безопасном режиме).
	//	;;    Запретить автозапуск: Внесите в реестр и перезагрузитесь
	if (gOSVer.dwMajorVersion == 6 && gOSVer.dwMinorVersion == 0)
	{
		CheckConIme();
	}

//------------------------------------------------------------------------
///| Loading from registry |//////////////////////////////////////////////
//------------------------------------------------------------------------
	SettingsBase* reg = CreateSettings();
	wcscpy_c(Type, reg->Type);

	BOOL lbOpened = FALSE, lbNeedCreateVanilla = FALSE;

	lbOpened = reg->OpenKey(ConfigPath, KEY_READ);
	// Поддержка старого стиля хранения (настройки БЕЗ имени конфига - лежали в корне ключа Software\ConEmu)
	if (!lbOpened && (ConfigName[0] == 0))
	{
		lbOpened = reg->OpenKey(CONEMU_ROOT_KEY, KEY_READ);
		lbNeedCreateVanilla = lbOpened;
	}

	if (lbOpened)
	{
		TCHAR ColorName[] = L"ColorTable00";

		for(uint i = countof(Colors)/*0x20*/; i--;)
		{
			ColorName[10] = i/10 + '0';
			ColorName[11] = i%10 + '0';
			reg->Load(ColorName, Colors[i]);
		}

		memmove(acrCustClr, Colors, sizeof(COLORREF)*16);
		reg->Load(L"ExtendColors", isExtendColors);
		reg->Load(L"ExtendColorIdx", nExtendColor);

		if (nExtendColor<0 || nExtendColor>15) nExtendColor=14;

		reg->Load(L"TrueColorerSupport", isTrueColorer);
		reg->Load(L"FadeInactive", isFadeInactive);
		reg->Load(L"FadeInactiveLow", mn_FadeLow);
		reg->Load(L"FadeInactiveHigh", mn_FadeHigh);

		if (mn_FadeHigh <= mn_FadeLow) { mn_FadeLow = DEFAULT_FADE_LOW; mn_FadeHigh = DEFAULT_FADE_HIGH; }
		mn_LastFadeSrc = mn_LastFadeDst = -1;

		reg->Load(L"ExtendFonts", isExtendFonts);
		reg->Load(L"ExtendFontNormalIdx", nFontNormalColor);

		if (nFontNormalColor<0 || nFontNormalColor>15) nFontNormalColor=1;

		reg->Load(L"ExtendFontBoldIdx", nFontBoldColor);

		if (nFontBoldColor<0 || nFontBoldColor>15) nFontBoldColor=12;

		reg->Load(L"ExtendFontItalicIdx", nFontItalicColor);

		if (nFontItalicColor<0 || nFontItalicColor>15) nFontItalicColor=13;

		// Debugging
		reg->Load(L"ConVisible", isConVisible);
		reg->Load(L"ConInMode", nConInMode);
		reg->Load(L"UseInjects", isUseInjects);
		#ifdef USEPORTABLEREGISTRY
		reg->Load(L"PortableReg", isPortableReg);
		#endif
		// Don't move invisible real console. This affects GUI eMenu.
		//reg->Load(L"LockRealConsolePos", isLockRealConsolePos);
		//reg->Load(L"DumpPackets", szDumpPackets);
		reg->Load(L"AutoRegisterFonts", isAutoRegisterFonts);

		if (reg->Load(L"FontName", inFont, countof(inFont)))
			mb_Name1Ok = TRUE;

		if (reg->Load(L"FontName2", inFont2, countof(inFont2)))
			mb_Name2Ok = TRUE;

		if (!mb_Name1Ok || !mb_Name2Ok)
			isAutoRegisterFonts = true;

		reg->Load(L"CmdLine", &psCmd);
		reg->Load(L"CmdLineHistory", &psCmdHistory); nCmdHistorySize = 0; HistoryCheck();
		reg->Load(L"Multi", isMulti);
		reg->Load(L"Multi.Modifier", nMultiHotkeyModifier); TestHostkeyModifiers();
		reg->Load(L"Multi.NewConsole", icMultiNew);
		reg->Load(L"Multi.Next", icMultiNext);
		reg->Load(L"Multi.Recreate", icMultiRecreate);
		reg->Load(L"Multi.Close", icMultiClose);
		reg->Load(L"Multi.CmdKey", icMultiCmd);
		reg->Load(L"Multi.NewConfirm", isMultiNewConfirm);
		reg->Load(L"Multi.Buffer", icMultiBuffer);
		reg->Load(L"Multi.UseArrows", isUseWinArrows);
		reg->Load(L"Multi.UseNumbers", isUseWinNumber);
		reg->Load(L"Multi.UseWinTab", isUseWinTab);
		reg->Load(L"Multi.AutoCreate", isMultiAutoCreate);
		reg->Load(L"Multi.LeaveOnClose", isMultiLeaveOnClose);
		reg->Load(L"Multi.Iterate", isMultiIterate);
		reg->Load(L"MinimizeRestore", icMinimizeRestore);

		reg->Load(L"KeyboardHooks", m_isKeyboardHooks); if (m_isKeyboardHooks>2) m_isKeyboardHooks = 0;

		reg->Load(L"FontAutoSize", isFontAutoSize);
		reg->Load(L"FontSize", FontSizeY);
		reg->Load(L"FontSizeX", FontSizeX);
		reg->Load(L"FontSizeX3", FontSizeX3);
		reg->Load(L"FontSizeX2", FontSizeX2);
		reg->Load(L"FontCharSet", mn_LoadFontCharSet); mb_CharSetWasSet = FALSE;
		reg->Load(L"Anti-aliasing", Quality);
		reg->Load(L"ConsoleFontName", ConsoleFont.lfFaceName, countof(ConsoleFont.lfFaceName));
		reg->Load(L"ConsoleFontWidth", ConsoleFont.lfWidth);
		reg->Load(L"ConsoleFontHeight", ConsoleFont.lfHeight);
		reg->Load(L"WindowMode", gpConEmu->WindowMode);
		reg->Load(L"HideCaption", isHideCaption);
		// грузим именно в mb_HideCaptionAlways, т.к. прозрачность сбивает темы в заголовке, поэтому возврат идет через isHideCaptionAlways()
		reg->Load(L"HideCaptionAlways", mb_HideCaptionAlways);
		reg->Load(L"HideCaptionAlwaysFrame", nHideCaptionAlwaysFrame);

		if (nHideCaptionAlwaysFrame > 10) nHideCaptionAlwaysFrame = 10;

		reg->Load(L"HideCaptionAlwaysDelay", nHideCaptionAlwaysDelay);

		if (nHideCaptionAlwaysDelay > 30000) nHideCaptionAlwaysDelay = 30000;

		reg->Load(L"HideCaptionAlwaysDisappear", nHideCaptionAlwaysDisappear);

		if (nHideCaptionAlwaysDisappear > 30000) nHideCaptionAlwaysDisappear = 30000;

		reg->Load(L"DownShowHiddenMessage", isDownShowHiddenMessage);

		reg->Load(L"ConWnd X", wndX); /*if (wndX<-10) wndX = 0;*/
		reg->Load(L"ConWnd Y", wndY); /*if (wndY<-10) wndY = 0;*/
		reg->Load(L"AutoSaveSizePos", isAutoSaveSizePos);
		// ЭТО не влияет на szDefCmd. Только прямое указание флажка "/BufferHeight N"
		// может сменить (умолчательную) команду запуска на "cmd" или "far"
		reg->Load(L"Cascaded", wndCascade);
		reg->Load(L"ConWnd Width", wndWidth); if (!wndWidth) wndWidth = 80; else if (wndWidth>1000) wndWidth = 1000;
		reg->Load(L"ConWnd Height", wndHeight); if (!wndHeight) wndHeight = 25; else if (wndHeight>500) wndHeight = 500;

		//TODO: Эти два параметра не сохраняются
		reg->Load(L"16bit Height", ntvdmHeight);

		if (ntvdmHeight!=0 && ntvdmHeight!=25 && ntvdmHeight!=28 && ntvdmHeight!=43 && ntvdmHeight!=50) ntvdmHeight = 25;

		reg->Load(L"DefaultBufferHeight", DefaultBufferHeight);

		if (DefaultBufferHeight < 300) DefaultBufferHeight = 300;

		reg->Load(L"AutoBufferHeight", AutoBufferHeight);
		//reg->Load(L"FarSyncSize", FarSyncSize);
		reg->Load(L"CmdOutputCP", nCmdOutputCP);
		reg->Load(L"CursorType", isCursorV);
		reg->Load(L"CursorColor", isCursorColor);
		reg->Load(L"CursorBlink", isCursorBlink);
		reg->Load(L"CursorBlockInactive", isCursorBlockInactive);

		reg->Load(L"ConsoleTextSelection", isConsoleTextSelection); if (isConsoleTextSelection>2) isConsoleTextSelection = 2;

		reg->Load(L"CTS.SelectBlock", isCTSSelectBlock);
		reg->Load(L"CTS.VkBlock", isCTSVkBlock);
		reg->Load(L"CTS.SelectText", isCTSSelectText);
		reg->Load(L"CTS.VkText", isCTSVkText);

		reg->Load(L"CTS.ActMode", isCTSActMode); if (!isCTSActMode || isCTSActMode>2) isCTSActMode = 2;

		reg->Load(L"CTS.VkAct", isCTSVkAct);

		reg->Load(L"CTS.RBtnAction", isCTSRBtnAction); if (isCTSRBtnAction>3) isCTSRBtnAction = 0;

		reg->Load(L"CTS.MBtnAction", isCTSMBtnAction); if (isCTSMBtnAction>3) isCTSMBtnAction = 0;

		reg->Load(L"CTS.ColorIndex", isCTSColorIndex); if ((isCTSColorIndex & 0xF) == ((isCTSColorIndex & 0xF0)>>4)) isCTSColorIndex = 0xE0;
		
		reg->Load(L"FarGotoEditor", isFarGotoEditor);
		reg->Load(L"FarGotoEditorVk", isFarGotoEditorVk);

		if (!reg->Load(L"FixFarBorders", isFixFarBorders))
			reg->Load(L"Experimental", isFixFarBorders);

		//mszCharRanges[0] = 0;

		TODO("Это вообще нужно будет расширить, определяя произвольное количество групп шрифтов");
		// max 100 ranges x 10 chars + a little ;)
		wchar_t szCharRanges[1024] = {};
		if (!reg->Load(L"FixFarBordersRanges", szCharRanges, countof(szCharRanges)))
		//{
		//	int n = 0, nMax = countof(icFixFarBorderRanges);
		//	wchar_t *pszRange = mszCharRanges, *pszNext = NULL;
		//	wchar_t cBegin, cEnd;
		//
		//	while(*pszRange && n < nMax)
		//	{
		//		cBegin = (wchar_t)wcstol(pszRange, &pszNext, 16);
		//
		//		if (!cBegin || cBegin == 0xFFFF || *pszNext != L'-') break;
		//
		//		pszRange = pszNext + 1;
		//		cEnd = (wchar_t)wcstol(pszRange, &pszNext, 16);
		//
		//		if (!cEnd || cEnd == 0xFFFF) break;
		//
		//		icFixFarBorderRanges[n].bUsed = true;
		//		icFixFarBorderRanges[n].cBegin = cBegin;
		//		icFixFarBorderRanges[n].cEnd = cEnd;
		//		n ++;
		//
		//		if (*pszNext != L';') break;
		//
		//		pszRange = pszNext + 1;
		//	}
		//
		//	for(; n < nMax; n++)
		//		icFixFarBorderRanges[n].bUsed = false;
		//}
		//else
		{
			wcscpy_c(szCharRanges, L"2013-25C4"); // default
		}
		ParseCharRanges(szCharRanges, mpc_FixFarBorderValues);

		reg->Load(L"ExtendUCharMap", isExtendUCharMap);
		reg->Load(L"PartBrush75", isPartBrush75); if (isPartBrush75<5) isPartBrush75=5; else if (isPartBrush75>250) isPartBrush75=250;
		reg->Load(L"PartBrush50", isPartBrush50); if (isPartBrush50<5) isPartBrush50=5; else if (isPartBrush50>250) isPartBrush50=250;
		reg->Load(L"PartBrush25", isPartBrush25); if (isPartBrush25<5) isPartBrush25=5; else if (isPartBrush25>250) isPartBrush25=250;

		reg->Load(L"PartBrushBlack", isPartBrushBlack);

		if (isPartBrush50>=isPartBrush75) isPartBrush50=isPartBrush75-10;

		if (isPartBrush25>=isPartBrush50) isPartBrush25=isPartBrush50-10;

		// Выделим в отдельную настройку
		reg->Load(L"EnhanceGraphics", isEnhanceGraphics);

		if (isFixFarBorders == 2 && !isEnhanceGraphics)
		{
			isFixFarBorders = 1;
			isEnhanceGraphics = true;
		}

		reg->Load(L"RightClick opens context menu", isRClickSendKey);
		reg->Load(L"RightClickMacro2", &sRClickMacro);
		
		reg->Load(L"AltEnter", isSendAltEnter);
		reg->Load(L"AltSpace", isSendAltSpace); //if (isSendAltSpace > 2) isSendAltSpace = 2; // когда-то был 3state, теперь - bool
		reg->Load(L"SendAltTab", isSendAltTab);
		reg->Load(L"SendAltEsc", isSendAltEsc);
		reg->Load(L"SendAltPrintScrn", isSendAltPrintScrn);
		reg->Load(L"SendPrintScrn", isSendPrintScrn);
		reg->Load(L"SendCtrlEsc", isSendCtrlEsc);
		
		reg->Load(L"Min2Tray", isMinToTray);
		reg->Load(L"AlwaysShowTrayIcon", isAlwaysShowTrayIcon);
		reg->Load(L"SafeFarClose", isSafeFarClose);
		reg->Load(L"SafeFarCloseMacro", &sSafeFarCloseMacro);
		reg->Load(L"FARuseASCIIsort", isFARuseASCIIsort);
		reg->Load(L"ShellNoZoneCheck", isShellNoZoneCheck);
		reg->Load(L"FixAltOnAltTab", isFixAltOnAltTab);
		reg->Load(L"BackGround Image show", isShowBgImage);

		if (isShowBgImage!=0 && isShowBgImage!=1 && isShowBgImage!=2) isShowBgImage = 0;

		reg->Load(L"BackGround Image", sBgImage, countof(sBgImage));
		reg->Load(L"bgImageDarker", bgImageDarker);
		reg->Load(L"bgImageColors", nBgImageColors);

		if (!nBgImageColors) nBgImageColors = (DWORD)-1; //1|2 == BgImageColorsDefaults;

		reg->Load(L"bgOperation", bgOperation);

		if (bgOperation!=eUpLeft && bgOperation!=eStretch && bgOperation!=eTile) bgOperation = 0;

		reg->Load(L"bgPluginAllowed", isBgPluginAllowed);

		if (isBgPluginAllowed!=0 && isBgPluginAllowed!=1 && isBgPluginAllowed!=2) isBgPluginAllowed = 1;

		reg->Load(L"AlphaValue", nTransparent);

		if (nTransparent < MIN_ALPHA_VALUE) nTransparent = MIN_ALPHA_VALUE;

		//reg->Load(L"UseColorKey", isColorKey);
		//reg->Load(L"ColorKey", ColorKey);
		reg->Load(L"UserScreenTransparent", isUserScreenTransparent);
		reg->Load(L"FontBold", isBold);
		reg->Load(L"FontItalic", isItalic);

		if (!reg->Load(L"Monospace", isMonospace))
		{
			// Compatibility. Пережитки прошлого, загрузить по "старым" именам параметров
			bool bForceMonospace = false, bProportional = false;
			reg->Load(L"ForceMonospace", bForceMonospace);
			reg->Load(L"Proportional", bProportional);
			isMonospace = bForceMonospace ? 2 : bProportional ? 0 : 1;
		}

		if (isMonospace > 2) isMonospace = 2;

		isMonospaceSelected = isMonospace ? isMonospace : 1; // запомнить, чтобы выбирать то что нужно при смене шрифта
		//reg->Load(L"Update Console handle", isUpdConHandle);
		reg->Load(L"DisableMouse", isDisableMouse);
		reg->Load(L"RSelectionFix", isRSelFix);
		reg->Load(L"MouseSkipActivation", isMouseSkipActivation);
		reg->Load(L"MouseSkipMoving", isMouseSkipMoving);
		reg->Load(L"FarHourglass", isFarHourglass);
		reg->Load(L"FarHourglassDelay", nFarHourglassDelay);
		reg->Load(L"Dnd", isDragEnabled);
		isDropEnabled = (BYTE)(isDragEnabled ? 1 : 0); // ранее "DndDrop" не было, поэтому ставим default
		reg->Load(L"DndLKey", nLDragKey);
		reg->Load(L"DndRKey", nRDragKey);
		reg->Load(L"DndDrop", isDropEnabled);
		reg->Load(L"DefCopy", isDefCopy);
		reg->Load(L"DragOverlay", isDragOverlay);
		reg->Load(L"DragShowIcons", isDragShowIcons);
		reg->Load(L"DebugSteps", isDebugSteps);

		reg->Load(L"DragPanel", isDragPanel); if (isDragPanel > 2) isDragPanel = 1;

		//reg->Load(L"GUIpb", isGUIpb);
		reg->Load(L"Tabs", isTabs);
		reg->Load(L"TabSelf", isTabSelf);
		reg->Load(L"TabLazy", isTabLazy);
		reg->Load(L"TabRecent", isTabRecent);

		if (!reg->Load(L"TabCloseMacro", &sTabCloseMacro) || (sTabCloseMacro && !*sTabCloseMacro)) { if (sTabCloseMacro) { free(sTabCloseMacro); sTabCloseMacro = NULL; } }

		reg->Load(L"TabFontFace", sTabFontFace, countof(sTabFontFace));
		reg->Load(L"TabFontCharSet", nTabFontCharSet);
		reg->Load(L"TabFontHeight", nTabFontHeight);

		if (!reg->Load(L"SaveAllEditors", &sSaveAllMacro))
		{
			sSaveAllMacro = lstrdup(L"@F2 $If (!Editor) $Exit $End %i0=-1; F12 %cur = CurPos; Home Down %s = Menu.Select(\" * \",3,2); $While (%s > 0) $If (%s == %i0) MsgBox(\"FAR SaveAll\",\"Asterisk in menuitem for already processed window\",0x10001) $Exit $End Enter $If (Editor) F2 $If (!Editor) $Exit $End $Else $If (!Viewer) $Exit $End $End %i0 = %s; F12 %s = Menu.Select(\" * \",3,2); $End $If (Menu && Title==\"Screens\") Home $Rep (%cur-1) Down $End Enter $End $Exit");
		}

		reg->Load(L"TabFrame", isTabFrame);
		reg->Load(L"TabMargins", rcTabMargins);
		reg->Load(L"ToolbarAddSpace", nToolbarAddSpace);

		if (nToolbarAddSpace<0 || nToolbarAddSpace>100) nToolbarAddSpace = 0;

		reg->Load(L"SlideShowElapse", nSlideShowElapse); // only PicView1
		reg->Load(L"IconID", nIconID);
		reg->Load(L"TabConsole", szTabConsole, countof(szTabConsole));
		//WCHAR* pszVert = wcschr(szTabPanels, L'|');
		//if (!pszVert) {
		//    if (_tcslen(szTabPanels)>54) szTabPanels[54] = 0;
		//    pszVert = szTabPanels + _tcslen(szTabPanels);
		//    wcscpy_c(pszVert+1, L"Console");
		//}
		//*pszVert = 0; pszTabConsole = pszVert+1;
		reg->Load(L"TabEditor", szTabEditor, countof(szTabEditor));
		reg->Load(L"TabEditorModified", szTabEditorModified, countof(szTabEditorModified));
		reg->Load(L"TabViewer", szTabViewer, countof(szTabViewer));

		reg->Load(L"TabLenMax", nTabLenMax); if (nTabLenMax < 10 || nTabLenMax >= CONEMUTABMAX) nTabLenMax = 20;

		/*reg->Load(L"ScrollTitle", isScrollTitle);
		reg->Load(L"ScrollTitleLen", ScrollTitleLen);*/
		reg->Load(L"AdminTitleSuffix", szAdminTitleSuffix, countof(szAdminTitleSuffix)); szAdminTitleSuffix[countof(szAdminTitleSuffix)-1] = 0;
		reg->Load(L"AdminShowShield", bAdminShield);
		reg->Load(L"HideInactiveConsoleTabs", bHideInactiveConsoleTabs);
		reg->Load(L"TryToCenter", isTryToCenter);

		reg->Load(L"ShowScrollbar", isAlwaysShowScrollbar); if (isAlwaysShowScrollbar > 2) isAlwaysShowScrollbar = 2;

		//reg->Load(L"CreateAppWindow", isCreateAppWindow);
		//reg->Load(L"AllowDetach", isAllowDetach);

		reg->Load(L"MainTimerElapse", nMainTimerElapse); if (nMainTimerElapse>1000) nMainTimerElapse = 1000;
		reg->Load(L"MainTimerInactiveElapse", nMainTimerInactiveElapse); if (nMainTimerInactiveElapse>10000) nMainTimerInactiveElapse = 10000;

		reg->Load(L"AffinityMask", nAffinity);
		//reg->Load(L"AdvLangChange", isAdvLangChange);
		reg->Load(L"SkipFocusEvents", isSkipFocusEvents);
		//reg->Load(L"LangChangeWsPlugin", isLangChangeWsPlugin);
		reg->Load(L"MonitorConsoleLang", isMonitorConsoleLang);
		reg->Load(L"DesktopMode", isDesktopMode);
		reg->Load(L"AlwaysOnTop", isAlwaysOnTop);
		reg->Load(L"SleepInBackground", isSleepInBackground);

		reg->Load(L"DisableFarFlashing", isDisableFarFlashing); if (isDisableFarFlashing>2) isDisableFarFlashing = 2;

		reg->Load(L"DisableAllFlashing", isDisableAllFlashing); if (isDisableAllFlashing>2) isDisableAllFlashing = 2;

		/* *********** Thumbnails and Tiles ************* */
		reg->Load(L"PanView.BackColor", ThSet.crBackground.RawColor);

		reg->Load(L"PanView.PFrame", ThSet.nPreviewFrame); if (ThSet.nPreviewFrame!=0 && ThSet.nPreviewFrame!=1) ThSet.nPreviewFrame = 1;

		reg->Load(L"PanView.PFrameColor", ThSet.crPreviewFrame.RawColor);

		reg->Load(L"PanView.SFrame", ThSet.nSelectFrame); if (ThSet.nSelectFrame!=0 && ThSet.nSelectFrame!=1) ThSet.nSelectFrame = 1;

		reg->Load(L"PanView.SFrameColor", ThSet.crSelectFrame.RawColor);
		/* теперь разнообразные размеры */
		ThumbLoadSet(L"Thumbs", ThSet.Thumbs);
		ThumbLoadSet(L"Tiles", ThSet.Tiles);
		// Прочие параметры загрузки
		reg->Load(L"PanView.LoadPreviews", ThSet.bLoadPreviews);
		reg->Load(L"PanView.LoadFolders", ThSet.bLoadFolders);
		reg->Load(L"PanView.LoadTimeout", ThSet.nLoadTimeout);
		reg->Load(L"PanView.MaxZoom", ThSet.nMaxZoom);
		reg->Load(L"PanView.UsePicView2", ThSet.bUsePicView2);
		reg->Load(L"PanView.RestoreOnStartup", ThSet.bRestoreOnStartup);
		reg->CloseKey();
	}

	// сервис больше не нужен
	delete reg;
	reg = NULL;

	LogFont.lfHeight = mn_FontHeight = FontSizeY;
	LogFont.lfWidth = mn_FontWidth = FontSizeX;
	lstrcpyn(LogFont.lfFaceName, inFont, countof(LogFont.lfFaceName));
	lstrcpyn(LogFont2.lfFaceName, inFont2, countof(LogFont2.lfFaceName));
	LogFont.lfQuality = Quality;
	LogFont.lfWeight = isBold ? FW_BOLD : FW_NORMAL;
	LogFont.lfCharSet = (BYTE) mn_LoadFontCharSet;
	LogFont.lfItalic = isItalic;

	if (lbNeedCreateVanilla)
	{
		SaveSettings(TRUE/*abSilent*/);
	}

	// Передернуть палитру затенения
	mb_FadeInitialized = false; GetColors(TRUE);
	// Применить в Mapping (там заодно и палитра копируется)
	//!! Это нужно делать после создания основного шрифта
	//gpConEmu->OnPanelViewSettingsChanged(FALSE);
	// Проверить необходимость установки хуков
	isKeyboardHooks();

	// Стили окна
	if (!gpConEmu->WindowMode)
	{
		// Иначе окно вообще не отображается
		_ASSERTE(gpConEmu->WindowMode!=0);
		gpConEmu->WindowMode = rNormal;
	}

	if (wndCascade)
	{
		// Сдвиг при каскаде
		int nShift = (GetSystemMetrics(SM_CYSIZEFRAME)+GetSystemMetrics(SM_CYCAPTION))*1.5;
		// Координаты и размер виртуальной рабочей области
		RECT rcScreen = MakeRect(800,600);
		int nMonitors = GetSystemMetrics(SM_CMONITORS);

		if (nMonitors > 1)
		{
			// Размер виртуального экрана по всем мониторам
			rcScreen.left = GetSystemMetrics(SM_XVIRTUALSCREEN); // may be <0
			rcScreen.top  = GetSystemMetrics(SM_YVIRTUALSCREEN);
			rcScreen.right = rcScreen.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
			rcScreen.bottom = rcScreen.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
			TODO("Хорошо бы исключить из рассмотрения Taskbar...");
		}
		else
		{
			SystemParametersInfo(SPI_GETWORKAREA, 0, &rcScreen, 0);
		}

		HWND hPrev = FindWindow(VirtualConsoleClassMain, NULL);

		while(hPrev)
		{
			/*if (Is Iconic(hPrev) || Is Zoomed(hPrev)) {
				hPrev = FindWindowEx(NULL, hPrev, VirtualConsoleClassMain, NULL);
				continue;
			}*/
			WINDOWPLACEMENT wpl = {sizeof(WINDOWPLACEMENT)}; // Workspace coordinates!!!

			if (!GetWindowPlacement(hPrev, &wpl))
			{
				break;
			}

			// Screen coordinates!
			RECT rcWnd; GetWindowRect(hPrev, &rcWnd);

			if (wpl.showCmd == SW_HIDE || !IsWindowVisible(hPrev)
			        || wpl.showCmd == SW_SHOWMINIMIZED || wpl.showCmd == SW_SHOWMAXIMIZED
			        /* Max в режиме скрытия заголовка */
			        || (wpl.rcNormalPosition.left<rcScreen.left || wpl.rcNormalPosition.top<rcScreen.top))
			{
				hPrev = FindWindowEx(NULL, hPrev, VirtualConsoleClassMain, NULL);
				continue;
			}

			wndX = rcWnd.left + nShift;
			wndY = rcWnd.top + nShift;
			break; // нашли, сдвинулись, выходим
		}
	}

	if (rcTabMargins.top > 100) rcTabMargins.top = 100;

	_ASSERTE(!rcTabMargins.bottom && !rcTabMargins.left && !rcTabMargins.right);
	rcTabMargins.bottom = rcTabMargins.left = rcTabMargins.right = 0;

	if (!psCmdHistory)
	{
		psCmdHistory = (wchar_t*)calloc(2,2);
	}

	//for(UINT n = 0; n < countof(icFixFarBorderRanges); n++)
	//{
	//	if (!icFixFarBorderRanges[n].bUsed) break;
	//
	//	for(WORD x = (WORD)(icFixFarBorderRanges[n].cBegin); x <= (WORD)(icFixFarBorderRanges[n].cEnd); x++)
	//		mpc_FixFarBorderValues[x] = true;
	//}

	/*if (wndWidth)
	    pVCon->TextWidth = wndWidth;
	if (wndHeight)
	    pVCon->TextHeight = wndHeight;*/
	/*if (wndHeight && wndWidth)
	{
	    COORD b = {wndWidth, wndHeight};
	  SetConsoleWindowSize(b,false); // Maximus5 - по аналогии с NightRoman
	  //MoveWindow(hConWnd, 0, 0, 1, 1, 0);
	  //SetConsoleScreenBufferSize(pVCon->hConOut(), b);
	  //MoveWindow(hConWnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 0);
	}*/

	// pVCon еще не создано!
	/*if (isShowBgImage && pVCon)
	    LoadImageFrom(pBgImage);*/
	//2009-06-07 Размер шрифта может быть задан в командной строке, так что создаем шрифт не здесь
	//InitFont();
	MCHKHEAP
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
BOOL CSettings::FontRangeLoad(SettingsBase* reg, int Idx)
{
	_ASSERTE(FALSE); // Не реализовано

	BOOL lbOpened = FALSE, lbNeedCreateVanilla = FALSE;
	wchar_t szFontKey[MAX_PATH+20];
	_wsprintf(szFontKey, SKIPLEN(countof(szFontKey)) L"%s\\Font%i", ConfigPath, (Idx+1));
	
	
	
	lbOpened = reg->OpenKey(szFontKey, KEY_READ);
	if (lbOpened)
	{
		TODO("Загрузить поля m_Fonts[Idx], очистив hFonts, не забыть про nLoadHeight/nLoadWidth");

		reg->CloseKey();
	}
	
	return lbOpened;
}
BOOL CSettings::FontRangeSave(SettingsBase* reg, int Idx)
{
	_ASSERTE(FALSE); // Не реализовано
	return FALSE;
}

void CSettings::InitFont(LPCWSTR asFontName/*=NULL*/, int anFontHeight/*=-1*/, int anQuality/*=-1*/)
{
	if (asFontName && *asFontName)
	{
		lstrcpynW(LogFont.lfFaceName, asFontName, countof(LogFont.lfFaceName));
		mb_Name1Ok = TRUE;
	}

	if (anFontHeight!=-1)
	{
		LogFont.lfHeight = mn_FontHeight = anFontHeight;
		LogFont.lfWidth = mn_FontWidth = 0;
	}

	if (anQuality!=-1)
	{
		LogFont.lfQuality = ANTIALIASED_QUALITY;
	}

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

void CSettings::UpdateMargins(RECT arcMargins)
{
	if (memcmp(&arcMargins, &rcTabMargins, sizeof(rcTabMargins))==0)
		return;

	rcTabMargins = arcMargins;
	SettingsBase* reg = CreateSettings();

	if (reg->OpenKey(ConfigPath, KEY_WRITE))
	{
		reg->Save(L"TabMargins", rcTabMargins);
		reg->CloseKey();
	}

	delete reg;
}

void CSettings::SaveSizePosOnExit()
{
	if (!this || !isAutoSaveSizePos)
		return;

	SettingsBase* reg = CreateSettings();

	if (reg->OpenKey(ConfigPath, KEY_WRITE))
	{
		reg->Save(L"ConWnd Width", wndWidth);
		reg->Save(L"ConWnd Height", wndHeight);
		reg->Save(L"16bit Height", ntvdmHeight);
		reg->Save(L"ConWnd X", wndX);
		reg->Save(L"ConWnd Y", wndY);
		reg->Save(L"Cascaded", wndCascade);
		reg->Save(L"AutoSaveSizePos", isAutoSaveSizePos);
		reg->CloseKey();
	}

	delete reg;
}

void CSettings::SaveConsoleFont()
{
	if (!this)
		return;

	SettingsBase* reg = CreateSettings();

	if (reg->OpenKey(ConfigPath, KEY_WRITE))
	{
		reg->Save(L"ConsoleFontName", ConsoleFont.lfFaceName);
		reg->Save(L"ConsoleFontWidth", ConsoleFont.lfWidth);
		reg->Save(L"ConsoleFontHeight", ConsoleFont.lfHeight);
		reg->CloseKey();
	}

	delete reg;
}

BOOL CSettings::SaveSettings(BOOL abSilent /*= FALSE*/)
{
	SettingsBase* reg = CreateSettings();

	if (reg->OpenKey(ConfigPath, KEY_WRITE, abSilent))
	{
		wcscpy_c(Type, reg->Type);
		TCHAR ColorName[] = L"ColorTable00";

		for(uint i = 0; i<countof(Colors)/*0x20*/; i++)
		{
			ColorName[10] = i/10 + '0';
			ColorName[11] = i%10 + '0';
			reg->Save(ColorName, (DWORD)Colors[i]);
		}

		reg->Save(L"ExtendColors", isExtendColors);
		reg->Save(L"ExtendColorIdx", nExtendColor);
		reg->Save(L"TrueColorerSupport", isTrueColorer);
		reg->Save(L"FadeInactive", isFadeInactive);
		reg->Save(L"FadeInactiveLow", mn_FadeLow);
		reg->Save(L"FadeInactiveHigh", mn_FadeHigh);
		/* таки сохраним, чтобы настройки переносить можно было */
		reg->Save(L"ExtendFonts", isExtendFonts);
		reg->Save(L"ExtendFontNormalIdx", nFontNormalColor);
		reg->Save(L"ExtendFontBoldIdx", nFontBoldColor);
		reg->Save(L"ExtendFontItalicIdx", nFontItalicColor);

		if (ghOpWnd && IsWindow(hMain))
		{
			int nLen = SendDlgItemMessage(hMain, tCmdLine, WM_GETTEXTLENGTH, 0, 0);
			if (nLen<=0)
			{
				if (psCmd) {free(psCmd); psCmd = NULL;}
			}
			else
			{
				if (nLen > (int)(psCmd ? _tcslen(psCmd) : 0))
				{
					if (psCmd) {free(psCmd); psCmd = NULL;}

					psCmd = (TCHAR*)calloc(nLen+1, sizeof(TCHAR));
				}

				GetDlgItemText(hMain, tCmdLine, psCmd, nLen+1);
			}
		}

		/*if (!isFullScreen && !gpConEmu->isZoomed() && !gpConEmu->isIconic())
		{
		    RECT rcPos; GetWindowRect(ghWnd, &rcPos);
		    wndX = rcPos.left;
		    wndY = rcPos.top;
		}*/
		reg->Save(L"ConVisible", isConVisible);
		reg->Save(L"ConInMode", nConInMode);
		
		//WARNING("Сохранение isUseInjects отключено принудительно");
		reg->Save(L"UseInjects", isUseInjects);
		#ifdef USEPORTABLEREGISTRY
		reg->Save(L"PortableReg", isPortableReg);
		#endif
		
		//reg->Save(L"LockRealConsolePos", isLockRealConsolePos);
		reg->Save(L"CmdLine", psCmd);

		if (psCmdHistory)
			reg->SaveMSZ(L"CmdLineHistory", psCmdHistory, nCmdHistorySize);

		reg->Save(L"Multi", isMulti);
		reg->Save(L"Multi.Modifier", nMultiHotkeyModifier);
		reg->Save(L"Multi.NewConsole", icMultiNew);
		reg->Save(L"Multi.Next", icMultiNext);
		reg->Save(L"Multi.Recreate", icMultiRecreate);
		reg->Save(L"Multi.Close", icMultiClose);
		reg->Save(L"Multi.CmdKey", icMultiCmd);
		reg->Save(L"Multi.NewConfirm", isMultiNewConfirm);
		reg->Save(L"Multi.Buffer", icMultiBuffer);
		reg->Save(L"Multi.UseArrows", isUseWinArrows);
		reg->Save(L"Multi.UseNumbers", isUseWinNumber);
		reg->Save(L"Multi.UseWinTab", isUseWinTab);
		reg->Save(L"Multi.AutoCreate", isMultiAutoCreate);
		reg->Save(L"Multi.LeaveOnClose", isMultiLeaveOnClose);
		reg->Save(L"Multi.Iterate", isMultiIterate);
		reg->Save(L"MinimizeRestore", icMinimizeRestore);
		_ASSERTE(m_isKeyboardHooks!=0);
		reg->Save(L"KeyboardHooks", m_isKeyboardHooks);
		reg->Save(L"FontName", LogFont.lfFaceName);
		reg->Save(L"FontName2", LogFont2.lfFaceName);
		bool lbTest = isAutoRegisterFonts; // Если в реестре настройка есть, или изменилось значение

		if (reg->Load(L"AutoRegisterFonts", lbTest) || isAutoRegisterFonts != lbTest)
			reg->Save(L"AutoRegisterFonts", isAutoRegisterFonts);

		reg->Save(L"BackGround Image show", isShowBgImage);
		reg->Save(L"BackGround Image", sBgImage);
		reg->Save(L"bgImageDarker", bgImageDarker);
		reg->Save(L"bgImageColors", nBgImageColors);
		reg->Save(L"bgOperation", bgOperation);
		reg->Save(L"bgPluginAllowed", isBgPluginAllowed);
		reg->Save(L"AlphaValue", nTransparent);
		//reg->Save(L"UseColorKey", isColorKey);
		//reg->Save(L"ColorKey", ColorKey);
		reg->Save(L"UserScreenTransparent", isUserScreenTransparent);
		reg->Save(L"FontAutoSize", isFontAutoSize);
		reg->Save(L"FontSize", LogFont.lfHeight);
		reg->Save(L"FontSizeX", FontSizeX);
		reg->Save(L"FontSizeX2", FontSizeX2);
		reg->Save(L"FontSizeX3", FontSizeX3);
		if (ghOpWnd == NULL)
		{
			reg->Save(L"FontCharSet", mn_LoadFontCharSet);
		}
		else
		{
			reg->Save(L"FontCharSet", mn_LoadFontCharSet = LogFont.lfCharSet); mb_CharSetWasSet = FALSE;
		}
		reg->Save(L"Anti-aliasing", LogFont.lfQuality);
		DWORD saveMode = (ghWnd == NULL) 
					? gpConEmu->WindowMode
					: (isFullScreen ? rFullScreen : gpConEmu->isZoomed() ? rMaximized : rNormal);
		reg->Save(L"WindowMode", saveMode);
		reg->Save(L"HideCaption", isHideCaption);
		reg->Save(L"HideCaptionAlways", mb_HideCaptionAlways);
		reg->Save(L"HideCaptionAlwaysFrame", nHideCaptionAlwaysFrame);
		reg->Save(L"HideCaptionAlwaysDelay", nHideCaptionAlwaysDelay);
		reg->Save(L"HideCaptionAlwaysDisappear", nHideCaptionAlwaysDisappear);
		reg->Save(L"DownShowHiddenMessage", isDownShowHiddenMessage);
		reg->Save(L"ConsoleFontName", ConsoleFont.lfFaceName);
		reg->Save(L"ConsoleFontWidth", ConsoleFont.lfWidth);
		reg->Save(L"ConsoleFontHeight", ConsoleFont.lfHeight);
		reg->Save(L"DefaultBufferHeight", DefaultBufferHeight);
		reg->Save(L"AutoBufferHeight", AutoBufferHeight);
		reg->Save(L"CmdOutputCP", nCmdOutputCP);
		reg->Save(L"CursorType", isCursorV);
		reg->Save(L"CursorColor", isCursorColor);
		reg->Save(L"CursorBlink", isCursorBlink);
		reg->Save(L"CursorBlockInactive", isCursorBlockInactive);
		reg->Save(L"ConsoleTextSelection", isConsoleTextSelection);
		reg->Save(L"CTS.SelectBlock", isCTSSelectBlock);
		reg->Save(L"CTS.VkBlock", isCTSVkBlock);
		reg->Save(L"CTS.SelectText", isCTSSelectText);
		reg->Save(L"CTS.VkText", isCTSVkText);
		reg->Save(L"CTS.ActMode", isCTSActMode);
		reg->Save(L"CTS.VkAct", isCTSVkAct);
		reg->Save(L"CTS.RBtnAction", isCTSRBtnAction);
		reg->Save(L"CTS.MBtnAction", isCTSMBtnAction);
		reg->Save(L"CTS.ColorIndex", isCTSColorIndex);
		
		reg->Save(L"FarGotoEditor", isFarGotoEditor);
		reg->Save(L"FarGotoEditorVk", isFarGotoEditorVk);
		
		reg->Save(L"FixFarBorders", isFixFarBorders);
		{
		wchar_t* pszCharRanges = CreateCharRanges(mpc_FixFarBorderValues);
		reg->Save(L"FixFarBordersRanges", pszCharRanges ? pszCharRanges : L"2013-25C4");
		if (pszCharRanges) free(pszCharRanges);
		}
		reg->Save(L"ExtendUCharMap", isExtendUCharMap);
		reg->Save(L"EnhanceGraphics", isEnhanceGraphics);
		reg->Save(L"PartBrush75", isPartBrush75);
		reg->Save(L"PartBrush50", isPartBrush50);
		reg->Save(L"PartBrush25", isPartBrush25);
		reg->Save(L"PartBrushBlack", isPartBrushBlack);
		reg->Save(L"RightClick opens context menu", isRClickSendKey);
		reg->Save(L"RightClickMacro2", sRClickMacro);
		reg->Save(L"AltEnter", isSendAltEnter);
		reg->Save(L"AltSpace", isSendAltSpace);
		reg->Save(L"SendAltTab", isSendAltTab);
		reg->Save(L"SendAltEsc", isSendAltEsc);
		reg->Save(L"SendAltPrintScrn", isSendAltPrintScrn);
		reg->Save(L"SendPrintScrn", isSendPrintScrn);
		reg->Save(L"SendCtrlEsc", isSendCtrlEsc);
		reg->Save(L"Min2Tray", isMinToTray);
		reg->Save(L"AlwaysShowTrayIcon", isAlwaysShowTrayIcon);
		reg->Save(L"SafeFarClose", isSafeFarClose);
		reg->Save(L"SafeFarCloseMacro", sSafeFarCloseMacro);
		reg->Save(L"FARuseASCIIsort", isFARuseASCIIsort);
		reg->Save(L"ShellNoZoneCheck", isShellNoZoneCheck);
		reg->Save(L"FixAltOnAltTab", isFixAltOnAltTab);
		reg->Save(L"DisableMouse", isDisableMouse);
		reg->Save(L"RSelectionFix", isRSelFix);
		reg->Save(L"MouseSkipActivation", isMouseSkipActivation);
		reg->Save(L"MouseSkipMoving", isMouseSkipMoving);
		reg->Save(L"FarHourglass", isFarHourglass);
		reg->Save(L"FarHourglassDelay", nFarHourglassDelay);
		reg->Save(L"Dnd", isDragEnabled);
		reg->Save(L"DndLKey", nLDragKey);
		reg->Save(L"DndRKey", nRDragKey);
		reg->Save(L"DndDrop", isDropEnabled);
		reg->Save(L"DefCopy", isDefCopy);
		reg->Save(L"DragOverlay", isDragOverlay);
		reg->Save(L"DragShowIcons", isDragShowIcons);
		reg->Save(L"DebugSteps", isDebugSteps);
		reg->Save(L"DragPanel", isDragPanel);
		//reg->Save(L"GUIpb", isGUIpb);
		reg->Save(L"Tabs", isTabs);
		reg->Save(L"TabSelf", isTabSelf);
		reg->Save(L"TabLazy", isTabLazy);
		reg->Save(L"TabRecent", isTabRecent);
		reg->Save(L"TabCloseMacro", sTabCloseMacro ? sTabCloseMacro : L"");
		reg->Save(L"TabFontFace", sTabFontFace);
		reg->Save(L"TabFontCharSet", nTabFontCharSet);
		reg->Save(L"TabFontHeight", nTabFontHeight);
		reg->Save(L"SaveAllEditors", sSaveAllMacro);
		reg->Save(L"TabFrame", isTabFrame);
		reg->Save(L"TabMargins", rcTabMargins);
		reg->Save(L"ToolbarAddSpace", nToolbarAddSpace);
		reg->Save(L"TabConsole", szTabConsole);
		reg->Save(L"TabEditor", szTabEditor);
		reg->Save(L"TabEditorModified", szTabEditorModified);
		reg->Save(L"TabViewer", szTabViewer);
		reg->Save(L"TabLenMax", nTabLenMax);
		reg->Save(L"AdminTitleSuffix", szAdminTitleSuffix);
		reg->Save(L"AdminShowShield", bAdminShield);
		reg->Save(L"HideInactiveConsoleTabs", bHideInactiveConsoleTabs);
		reg->Save(L"TryToCenter", isTryToCenter);
		reg->Save(L"ShowScrollbar", isAlwaysShowScrollbar);
		reg->Save(L"IconID", nIconID);
		reg->Save(L"FontBold", (bool)(LogFont.lfWeight == FW_BOLD));
		reg->Save(L"FontItalic", (bool)(LogFont.lfItalic != 0));
		//reg->Save(L"ForceMonospace", isForceMonospace);
		//reg->Save(L"Proportional", isProportional);
		reg->Save(L"Monospace", isMonospace);
		//reg->Save(L"Update Console handle", isUpdConHandle);
		reg->Save(L"ConWnd Width", wndWidth);
		reg->Save(L"ConWnd Height", wndHeight);
		reg->Save(L"16bit Height", ntvdmHeight);
		reg->Save(L"ConWnd X", wndX);
		reg->Save(L"ConWnd Y", wndY);
		reg->Save(L"Cascaded", wndCascade);
		reg->Save(L"AutoSaveSizePos", isAutoSaveSizePos);
		/*reg->Save(L"ScrollTitle", isScrollTitle);
		reg->Save(L"ScrollTitleLen", ScrollTitleLen);*/
		reg->Save(L"MainTimerElapse", nMainTimerElapse);
		reg->Save(L"MainTimerInactiveElapse", nMainTimerInactiveElapse);
		reg->Save(L"AffinityMask", nAffinity);
		reg->Save(L"SkipFocusEvents", isSkipFocusEvents);
		reg->Save(L"MonitorConsoleLang", isMonitorConsoleLang);
		reg->Save(L"DesktopMode", isDesktopMode);
		reg->Save(L"AlwaysOnTop", isAlwaysOnTop);
		reg->Save(L"SleepInBackground", isSleepInBackground);
		reg->Save(L"DisableFarFlashing", isDisableFarFlashing);
		reg->Save(L"DisableAllFlashing", isDisableAllFlashing);
		/* *********** Thumbnails and Tiles ************* */
		reg->Save(L"PanView.BackColor", ThSet.crBackground.RawColor);
		reg->Save(L"PanView.PFrame", ThSet.nPreviewFrame);
		reg->Save(L"PanView.PFrameColor", ThSet.crPreviewFrame.RawColor);
		reg->Save(L"PanView.SFrame", ThSet.nSelectFrame);
		reg->Save(L"PanView.SFrameColor", ThSet.crSelectFrame.RawColor);
		/* теперь разнообразные размеры */
		ThumbSaveSet(L"Thumbs", ThSet.Thumbs);
		ThumbSaveSet(L"Tiles", ThSet.Tiles);
		// Прочие параметры загрузки
		reg->Save(L"PanView.LoadPreviews", ThSet.bLoadPreviews);
		reg->Save(L"PanView.LoadFolders", ThSet.bLoadFolders);
		reg->Save(L"PanView.LoadTimeout", ThSet.nLoadTimeout);
		reg->Save(L"PanView.MaxZoom", ThSet.nMaxZoom);
		reg->Save(L"PanView.UsePicView2", ThSet.bUsePicView2);
		reg->Save(L"PanView.RestoreOnStartup", ThSet.bRestoreOnStartup);
		reg->CloseKey();
		delete reg;
		//if (isTabs==1) ForceShowTabs();
		//MessageBoxA(ghOpWnd, "Saved.", "Information", MB_ICONINFORMATION);
		return TRUE;
	}

	//}
	delete reg;

	// Вроде и показывать не нужно. Объект уже сам ругнулся
	//MessageBoxA(ghOpWnd, "Failed", "Information", MB_ICONERROR);
	return FALSE;
}


bool CSettings::ShowColorDialog(HWND HWndOwner, COLORREF *inColor)
{
	CHOOSECOLOR cc;                 // common dialog box structure
	// Вернул. IMHO - бред. Добавили Custom Color, а меняется ФОН окна!
	// Initialize CHOOSECOLOR
	ZeroMemory(&cc, sizeof(cc));
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = HWndOwner;
	cc.lpCustColors = (LPDWORD) acrCustClr;
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

	if (SendDlgItemMessage(gpSet->hMain, tFontFace, CB_FINDSTRINGEXACT, -1, (LPARAM) lplf->lfFaceName)==-1)
	{
		int nIdx;
		nIdx = SendDlgItemMessage(gpSet->hMain, tFontFace, CB_ADDSTRING, 0, (LPARAM) lplf->lfFaceName);
		SendDlgItemMessage(gpSet->hMain, tFontFace, CB_SETITEMDATA, nIdx, bAlmostMonospace);
		nIdx = SendDlgItemMessage(gpSet->hMain, tFontFace2, CB_ADDSTRING, 0, (LPARAM) lplf->lfFaceName);
		SendDlgItemMessage(gpSet->hMain, tFontFace2, CB_SETITEMDATA, nIdx, bAlmostMonospace);
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
		int nIdx = SendDlgItemMessage(gpSet->hMain, tFontFace, CB_INSERTSTRING, sz, (LPARAM)szName);
		SendDlgItemMessage(gpSet->hMain, tFontFace, CB_SETITEMDATA, nIdx, 1);
	}

	GetDlgItemText(gpSet->hMain, tFontFace, szName, MAX_PATH);
	gpSet->SelectString(gpSet->hMain, tFontFace, szName);
	GetDlgItemText(gpSet->hMain, tFontFace2, szName, MAX_PATH);
	gpSet->SelectString(gpSet->hMain, tFontFace2, szName);
	SafeCloseHandle(gpSet->mh_EnumThread);
	_ASSERTE(gpSet->mh_EnumThread == NULL);

	// Если шустрый юзер успел переключиться на вкладку "Views" до оконачания
	// загрузки шрифтов - послать в диалог сообщение "Считать список из hMain"
	if (ghOpWnd && gpSet->hViews)
	{
		PostMessage(gpSet->hViews, gpSet->mn_MsgLoadFontFromMain, 0, 0);
	}

	return 0;
}

LRESULT CSettings::OnInitDialog()
{
	_ASSERTE(!hMain && !hColors && !hViews && !hExt && !hInfo && !hDebug);
	hMain = hExt = hTabs = hKeys = hViews = hColors = hInfo = hDebug = NULL;
	gbLastColorsOk = FALSE;
	HMENU hSysMenu = GetSystemMenu(ghOpWnd, FALSE);
	InsertMenu(hSysMenu, 0, MF_BYPOSITION, MF_SEPARATOR, 0);
	InsertMenu(hSysMenu, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED
	           | ((GetWindowLong(ghOpWnd,GWL_EXSTYLE)&WS_EX_TOPMOST) ? MF_CHECKED : 0),
	           ID_ALWAYSONTOP, _T("Al&ways on top..."));
	RegisterTabs();
	mn_LastChangingFontCtrlId = 0;
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
	//	_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"%s Settings (%s) %s", gpConEmu->ms_ConEmuVer, (Config+nStdLen+1), pszType);
	if (ConfigName[0])
		_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"%s Settings (%s) %s", gpConEmu->ms_ConEmuVer, ConfigName, pszType);
	else
		_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"%s Settings %s", gpConEmu->ms_ConEmuVer, pszType);

	SetWindowText(ghOpWnd, szTitle);
	MCHKHEAP
	{
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

		HFONT hFont = CreateFont(nTabFontHeight/*TAB_FONT_HEIGTH*/, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, nTabFontCharSet /*ANSI_CHARSET*/, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, /* L"Tahoma" */ sTabFontFace);
		SendMessage(_hwndTab, WM_SETFONT, WPARAM(hFont), TRUE);

		RECT rcClient; GetWindowRect(_hwndTab, &rcClient);
		MapWindowPoints(NULL, ghOpWnd, (LPPOINT)&rcClient, 2);
		TabCtrl_AdjustRect(_hwndTab, FALSE, &rcClient);

		hMain = CreateDialog((HINSTANCE)GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDD_SPG_MAIN), ghOpWnd, mainOpProc);
		MoveWindow(hMain, rcClient.left, rcClient.top, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top, 0);


		apiShowWindow(hMain, SW_SHOW);
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
	DWORD nTest = nBgImageColors;
	wchar_t *pszTemp = tmp; tmp[0] = 0;

	if (nBgImageColors == (DWORD)-1)
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

LRESULT CSettings::OnInitDialog_Main()
{
	if (gpSet->EnableThemeDialogTextureF)
		gpSet->EnableThemeDialogTextureF(hMain, 6/*ETDT_ENABLETAB*/);

	//if (isUpdConHandle)
	//	CheckDlgButton(ghOpWnd, cbAutoConHandle, BST_CHECKED);

	SetDlgItemText(hMain, tFontFace, LogFont.lfFaceName);
	SetDlgItemText(hMain, tFontFace2, LogFont2.lfFaceName);
	DWORD dwThId;
	mh_EnumThread = CreateThread(0,0,EnumFontsThread,0,0,&dwThId); // хэндл закроет сама нить
	{
		wchar_t temp[MAX_PATH];

		for(uint i=0; i < countof(Settings::FSizes); i++)
		{
			_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", Settings::FSizes[i]);

			if (i > 0)
				SendDlgItemMessage(hMain, tFontSizeY, CB_ADDSTRING, 0, (LPARAM) temp);

			SendDlgItemMessage(hMain, tFontSizeX, CB_ADDSTRING, 0, (LPARAM) temp);
			SendDlgItemMessage(hMain, tFontSizeX2, CB_ADDSTRING, 0, (LPARAM) temp);
			SendDlgItemMessage(hMain, tFontSizeX3, CB_ADDSTRING, 0, (LPARAM) temp);
		}

		for(uint i=0; i <= 16; i++)
		{
			_wsprintf(temp, SKIPLEN(countof(temp))(i==16) ? L"None" : L"%2i", i);
			SendDlgItemMessage(hMain, lbExtendFontBoldIdx, CB_ADDSTRING, 0, (LPARAM) temp);
			SendDlgItemMessage(hMain, lbExtendFontItalicIdx, CB_ADDSTRING, 0, (LPARAM) temp);
			SendDlgItemMessage(hMain, lbExtendFontNormalIdx, CB_ADDSTRING, 0, (LPARAM) temp);
		}

		if (isExtendFonts) CheckDlgButton(hMain, cbExtendFonts, BST_CHECKED);

		_wsprintf(temp, SKIPLEN(countof(temp))(nFontBoldColor<16) ? L"%2i" : L"None", nFontBoldColor);
		SelectStringExact(hMain, lbExtendFontBoldIdx, temp);
		_wsprintf(temp, SKIPLEN(countof(temp))(nFontItalicColor<16) ? L"%2i" : L"None", nFontItalicColor);
		SelectStringExact(hMain, lbExtendFontItalicIdx, temp);
		_wsprintf(temp, SKIPLEN(countof(temp))(nFontNormalColor<16) ? L"%2i" : L"None", nFontNormalColor);
		SelectStringExact(hMain, lbExtendFontNormalIdx, temp);

		if (isFontAutoSize) CheckDlgButton(hMain, cbFontAuto, BST_CHECKED);

		_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", LogFont.lfHeight);
		//upToFontHeight = LogFont.lfHeight;
		SelectStringExact(hMain, tFontSizeY, temp);
		_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", FontSizeX);
		SelectStringExact(hMain, tFontSizeX, temp);
		_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", FontSizeX2);
		SelectStringExact(hMain, tFontSizeX2, temp);
		_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", FontSizeX3);
		SelectStringExact(hMain, tFontSizeX3, temp);
	}
	{
		_ASSERTE(countof(chSetsNums) == countof(ChSets));
		u8 num = 4; //-V112

		for(uint i=0; i < countof(ChSets); i++)
		{
			SendDlgItemMessageA(hMain, tFontCharset, CB_ADDSTRING, 0, (LPARAM) ChSets[i]);

			if (chSetsNums[i] == LogFont.lfCharSet) num = i;
		}

		SendDlgItemMessage(hMain, tFontCharset, CB_SETCURSEL, num, 0);
	}
	
	MCHKHEAP
	SetDlgItemText(hMain, tCmdLine, psCmd ? psCmd : L"");
	SetDlgItemText(hMain, tBgImage, sBgImage);
	//CheckDlgButton(hMain, rBgSimple, BST_CHECKED);

	FillBgImageColors();

	TCHAR tmp[255];
	//DWORD nTest = nBgImageColors;
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

	_wsprintf(tmp, SKIPLEN(countof(tmp)) L"%i", bgImageDarker);
	SendDlgItemMessage(hMain, tDarker, EM_SETLIMITTEXT, 3, 0);
	SetDlgItemText(hMain, tDarker, tmp);
	SendDlgItemMessage(hMain, slDarker, TBM_SETRANGE, (WPARAM) true, (LPARAM) MAKELONG(0, 255));
	SendDlgItemMessage(hMain, slDarker, TBM_SETPOS  , (WPARAM) true, (LPARAM) bgImageDarker);
	CheckDlgButton(hMain, rBgUpLeft+(UINT)bgOperation, BST_CHECKED);

	if (isBgPluginAllowed) CheckDlgButton(hMain, cbBgAllowPlugin, (isBgPluginAllowed == 1) ? BST_CHECKED : BST_INDETERMINATE);

	if (isShowBgImage)
	{
		CheckDlgButton(hMain, cbBgImage, (isShowBgImage == 1) ? BST_CHECKED : BST_INDETERMINATE);
	}
	else
	{
		EnableWindow(GetDlgItem(hMain, tBgImage), false);
		//EnableWindow(GetDlgItem(hMain, tDarker), false);
		//EnableWindow(GetDlgItem(hMain, slDarker), false);
		EnableWindow(GetDlgItem(hMain, bBgImage), false);
		//EnableWindow(GetDlgItem(hMain, rBgUpLeft), false);
		//EnableWindow(GetDlgItem(hMain, rBgStretch), false);
		//EnableWindow(GetDlgItem(hMain, rBgTile), false);
	}

	switch(LogFont.lfQuality)
	{
		case NONANTIALIASED_QUALITY:
			CheckDlgButton(hMain, rNoneAA, BST_CHECKED);
			break;
		case ANTIALIASED_QUALITY:
			CheckDlgButton(hMain, rStandardAA, BST_CHECKED);
			break;
		case CLEARTYPE_NATURAL_QUALITY:
			CheckDlgButton(hMain, rCTAA, BST_CHECKED);
			break;
	}

	if (isCursorColor) CheckDlgButton(hMain, cbCursorColor, BST_CHECKED);

	if (isCursorBlink) CheckDlgButton(hMain, cbCursorBlink, BST_CHECKED);

	CheckDlgButton(hMain, cbBlockInactiveCursor, isCursorBlockInactive ? BST_CHECKED : BST_UNCHECKED);

	if (isCursorV)
		CheckDlgButton(hMain, rCursorV, BST_CHECKED);
	else
		CheckDlgButton(hMain, rCursorH, BST_CHECKED);

	if (isMonospace)  // 3d state - force center symbols in cells
		CheckDlgButton(hMain, cbMonospace, isMonospace==2 ? BST_INDETERMINATE : BST_CHECKED);

	if (LogFont.lfWeight == FW_BOLD) CheckDlgButton(hMain, cbBold, BST_CHECKED);

	if (LogFont.lfItalic)            CheckDlgButton(hMain, cbItalic, BST_CHECKED);

	if (isFixFarBorders)
		CheckDlgButton(hMain, cbFixFarBorders, (isFixFarBorders == 1) ? BST_CHECKED : BST_INDETERMINATE);

	if (isFullScreen)
		CheckRadioButton(hMain, rNormal, rFullScreen, rFullScreen);
	else if (gpConEmu->isZoomed())
		CheckRadioButton(hMain, rNormal, rFullScreen, rMaximized);
	else
		CheckRadioButton(hMain, rNormal, rFullScreen, rNormal);

	//swprintf_c(temp, L"%i", wndWidth);   SetDlgItemText(hMain, tWndWidth, temp);
	SendDlgItemMessage(hMain, tWndWidth, EM_SETLIMITTEXT, 3, 0);
	//swprintf_c(temp, L"%i", wndHeight);  SetDlgItemText(hMain, tWndHeight, temp);
	SendDlgItemMessage(hMain, tWndHeight, EM_SETLIMITTEXT, 3, 0);
	UpdateSize(wndWidth, wndHeight);
	EnableWindow(GetDlgItem(hMain, cbApplyPos), FALSE);
	SendDlgItemMessage(hMain, tWndX, EM_SETLIMITTEXT, 6, 0);
	SendDlgItemMessage(hMain, tWndY, EM_SETLIMITTEXT, 6, 0);
	MCHKHEAP

	if (!isFullScreen && !gpConEmu->isZoomed())
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
			wndX = rc.left; wndY = rc.top;
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

	UpdatePos(wndX, wndY);
	CheckDlgButton(hMain, wndCascade ? rCascade : rFixed, BST_CHECKED);

	if (isAutoSaveSizePos) CheckDlgButton(hMain, cbAutoSaveSizePos, BST_CHECKED);

	mn_LastChangingFontCtrlId = 0;
	RegisterTipsFor(hMain);
	return 0;
}

LRESULT CSettings::OnInitDialog_Ext()
{
	if (gpSet->EnableThemeDialogTextureF)
		gpSet->EnableThemeDialogTextureF(hExt, 6/*ETDT_ENABLETAB*/);

	if (isRClickSendKey) CheckDlgButton(hExt, cbRClick, (isRClickSendKey==1) ? BST_CHECKED : BST_INDETERMINATE);

	if (isMinToTray) CheckDlgButton(hExt, cbMinToTray, BST_CHECKED);

	if (isAlwaysShowTrayIcon) CheckDlgButton(hExt, cbAlwaysShowTrayIcon, BST_CHECKED);

	if (isAutoRegisterFonts) CheckDlgButton(hExt, cbAutoRegFonts, BST_CHECKED);

	if (isDebugSteps) CheckDlgButton(hExt, cbDebugSteps, BST_CHECKED);
	
	if (isHideCaption) CheckDlgButton(hExt, cbHideCaption, BST_CHECKED);

	if (isHideCaptionAlways()) CheckDlgButton(hExt, cbHideCaptionAlways, BST_CHECKED);

	if (isEnhanceGraphics) CheckDlgButton(hExt, cbEnhanceGraphics, BST_CHECKED);

	if (isSafeFarClose) CheckDlgButton(hExt, cbSafeFarClose, BST_CHECKED);

	if (isFARuseASCIIsort) CheckDlgButton(hExt, cbFARuseASCIIsort, BST_CHECKED);

	if (isShellNoZoneCheck) CheckDlgButton(hExt, cbShellNoZoneCheck, BST_CHECKED);

	if (isFixAltOnAltTab) CheckDlgButton(hExt, cbFixAltOnAltTab, BST_CHECKED);

	if (isFarHourglass) CheckDlgButton(hExt, cbFarHourglass, BST_CHECKED);

	if (isExtendUCharMap) CheckDlgButton(hExt, cbExtendUCharMap, BST_CHECKED);

	if (isDragEnabled)
	{
		//CheckDlgButton(hExt, cbDragEnabled, BST_CHECKED);
		if (isDragEnabled & DRAG_L_ALLOWED) CheckDlgButton(hExt, cbDragL, BST_CHECKED);

		if (isDragEnabled & DRAG_R_ALLOWED) CheckDlgButton(hExt, cbDragR, BST_CHECKED);
	}

	if (isDropEnabled) CheckDlgButton(hExt, cbDropEnabled, (isDropEnabled==1) ? BST_CHECKED : BST_INDETERMINATE);

	if (isDefCopy) CheckDlgButton(hExt, cbDnDCopy, BST_CHECKED);

	// Списки
	FillListBoxByte(hExt, lbLDragKey, Settings::szKeys, Settings::nKeys, nLDragKey);
	FillListBoxByte(hExt, lbRDragKey, Settings::szKeys, Settings::nKeys, nRDragKey);

	// Overlay
	if (isDragOverlay) CheckDlgButton(hExt, cbDragImage, (isDragOverlay==1) ? BST_CHECKED : BST_INDETERMINATE);

	if (isDragShowIcons) CheckDlgButton(hExt, cbDragIcons, BST_CHECKED);

	if (isRSelFix)
		CheckDlgButton(hExt, cbRSelectionFix, BST_CHECKED);

	if (isMouseSkipActivation)
		CheckDlgButton(hExt, cbSkipActivation, BST_CHECKED);

	if (isMouseSkipMoving)
		CheckDlgButton(hExt, cbSkipMove, BST_CHECKED);

	if (isMonitorConsoleLang)
		CheckDlgButton(hExt, cbMonitorConsoleLang, BST_CHECKED);

	if (isSkipFocusEvents)
		CheckDlgButton(hExt, cbSkipFocusEvents, BST_CHECKED);

	CheckDlgButton(hExt, cbConsoleTextSelection, isConsoleTextSelection);

	if (isMulti)
		CheckDlgButton(hExt, cbMultiCon, BST_CHECKED);

	if (isMultiNewConfirm)
		CheckDlgButton(hExt, cbNewConfirm, BST_CHECKED);

	if (AutoBufferHeight)
		CheckDlgButton(hExt, cbLongOutput, BST_CHECKED);

	wchar_t sz[16];
	SendDlgItemMessage(hExt, tLongOutputHeight, EM_SETLIMITTEXT, 5, 0);
	SetDlgItemText(hExt, tLongOutputHeight, _ltow(gpSet->DefaultBufferHeight, sz, 10));
	EnableWindow(GetDlgItem(hExt, tLongOutputHeight), AutoBufferHeight);
	// 16bit Height
	SendDlgItemMessage(hExt, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"Auto");
	SendDlgItemMessage(hExt, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"25 lines");
	SendDlgItemMessage(hExt, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"28 lines");
	SendDlgItemMessage(hExt, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"43 lines");
	SendDlgItemMessage(hExt, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"50 lines");
	SendDlgItemMessage(hExt, lbNtvdmHeight, CB_SETCURSEL, !ntvdmHeight ? 0 :
	                   ((ntvdmHeight == 25) ? 1 : ((ntvdmHeight == 28) ? 2 : ((ntvdmHeight == 43) ? 3 : 4))), 0); //-V112
	// Cmd.exe output cp
	SendDlgItemMessage(hExt, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"Undefined");
	SendDlgItemMessage(hExt, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"Automatic");
	SendDlgItemMessage(hExt, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"Unicode");
	SendDlgItemMessage(hExt, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"OEM");
	SendDlgItemMessage(hExt, lbCmdOutputCP, CB_SETCURSEL, nCmdOutputCP, 0);
	//
	CheckDlgButton(hExt, cbDragPanel, isDragPanel);
	CheckDlgButton(hExt, cbTryToCenter, isTryToCenter);
	CheckDlgButton(hExt, cbAlwaysShowScrollbar, isAlwaysShowScrollbar);

	if (isDesktopMode) CheckDlgButton(hExt, cbDesktopMode, BST_CHECKED);

	if (isAlwaysOnTop)  CheckDlgButton(hExt, cbAlwaysOnTop, BST_CHECKED);

	if (isSleepInBackground) CheckDlgButton(hExt, cbSleepInBackground, BST_CHECKED);

	if (isDisableFarFlashing) CheckDlgButton(hExt, cbDisableFarFlashing, isDisableFarFlashing);

	if (isDisableAllFlashing) CheckDlgButton(hExt, cbDisableAllFlashing, isDisableAllFlashing);

	if (isConVisible)
		CheckDlgButton(hExt, cbVisible, BST_CHECKED);

	//#ifndef _DEBUG
	//WARNING("isUseInjects отключен принудительно");
	//if (!isUseInjects)
	//	EnableWindow(GetDlgItem(hExt, cbUseInjects), FALSE); // Сохранение запрещено
	//#endif
	if (isUseInjects)
		CheckDlgButton(hExt, cbUseInjects, BST_CHECKED);

	if (gpConEmu->mb_DosBoxExists)
	{
		CheckDlgButton(hExt, cbDosBox, BST_CHECKED);
		EnableWindow(GetDlgItem(hExt, cbDosBox), FALSE); // изменение пока запрещено
	}
	else
	{
		CheckDlgButton(hExt, cbDosBox, BST_UNCHECKED);
		EnableWindow(GetDlgItem(hExt, cbDosBox), TRUE); // чтобы ругнуться, если DosBox не установлен
	}
	EnableWindow(GetDlgItem(hExt, bDosBoxSettings), FALSE); // изменение пока запрещено

	#ifdef USEPORTABLEREGISTRY
	if (gpConEmu->mb_PortableRegExist)
	{
		if (isPortableReg)
			CheckDlgButton(hExt, cbPortableRegistry, BST_CHECKED);
		EnableWindow(GetDlgItem(hExt, bPortableRegistrySettings), FALSE); // изменение пока запрещено
	}
	else
	#endif
	{
		EnableWindow(GetDlgItem(hExt, cbPortableRegistry), FALSE); // изменение пока запрещено
		EnableWindow(GetDlgItem(hExt, bPortableRegistrySettings), FALSE); // изменение пока запрещено
	}
	

	//if (isLockRealConsolePos) CheckDlgButton(hExt, cbLockRealConsolePos, BST_CHECKED);
#ifdef _DEBUG

	if (isTabsInCaption) CheckDlgButton(hExt, cbTabsInCaption, BST_CHECKED);

#else
	ShowWindow(GetDlgItem(hExt, cbTabsInCaption), SW_HIDE);
#endif
	RegisterTipsFor(hExt);
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
		wcscat_c(szName, L"Ins"); break;
	case VK_DELETE:
		wcscat_c(szName, L"Del"); break;
	case VK_HOME:
		wcscat_c(szName, L"Home"); break;
	case VK_END:
		wcscat_c(szName, L"End"); break;
	case VK_PAUSE:
		wcscat_c(szName, L"Pause"); break;

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

void CSettings::FillHotKeysList()
{
	HWND hList = GetDlgItem(hKeys, lbConEmuHotKeys);

	// Населить список всеми хоткеями
	if (m_HotKeys)
	{
		ListView_DeleteAllItems(hList);
	
		wchar_t szName[128], szFull[512];
		//HWND hDetails = GetDlgItem(hWnd2, lbActivityDetails);
		LVITEM lvi = {LVIF_TEXT|LVIF_STATE};
		lvi.state = 0;
		lvi.stateMask = LVIS_SELECTED|LVIS_FOCUSED;
		lvi.pszText = szName;
		for (size_t i = 0; m_HotKeys[i].DescrLangID; i++)
		{
			wcscpy_c(szName, (m_HotKeys[i].Type == 1) ? L"Modifier" : m_HotKeys[i].VkPtr ? L"User" : L"System");
			lvi.iItem = i + 1; // в конец
			int nItem = ListView_InsertItem(hList, &lvi);
			ListView_SetItemState(hList, nItem, 0, LVIS_SELECTED|LVIS_FOCUSED);
			
			szFull[0] = 0;
			
			DWORD nModifier = (m_HotKeys[i].Type == 1) ? 0 : m_HotKeys[i].VkPtr ? (DWORD)-1/*nMultiHotkeyModifier*/ : m_HotKeys[i].Modifier;
			if (nModifier == (DWORD)-1)
			{
				//nModifier = nMultiHotkeyModifier; // для Win-Number
				wcscpy_c(szFull, L"«Host»");
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
}

LRESULT CSettings::OnInitDialog_Keys()
{
	if (gpSet->EnableThemeDialogTextureF)
		gpSet->EnableThemeDialogTextureF(hKeys, 6/*ETDT_ENABLETAB*/);

	HWND hList = GetDlgItem(hKeys, lbConEmuHotKeys);
	
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

	FillHotKeysList();	
		
	SetupHotkeyChecks(hKeys);

	if (gpSet->isUseWinArrows)
		CheckDlgButton(hKeys, cbUseWinArrows, BST_CHECKED);
	if (gpSet->isUseWinNumber)
		CheckDlgButton(hKeys, cbUseWinNumberK, BST_CHECKED);
	if (gpSet->isUseWinTab)
		CheckDlgButton(hKeys, cbUseWinTab, BST_CHECKED);

	CheckDlgButton(hKeys, cbInstallKeybHooks,
	               (m_isKeyboardHooks == 1) ? BST_CHECKED :
	               ((m_isKeyboardHooks == 0) ? BST_INDETERMINATE : BST_UNCHECKED));
	
	if (isSendAltEnter) CheckDlgButton(hKeys, cbSendAltEnter, BST_CHECKED);
	if (isSendAltSpace) CheckDlgButton(hKeys, cbSendAltSpace, BST_CHECKED);
	if (isSendAltTab) CheckDlgButton(hKeys, cbSendAltTab, BST_CHECKED);
	if (isSendAltEsc) CheckDlgButton(hKeys, cbSendAltEsc, BST_CHECKED);
	if (isSendAltPrintScrn) CheckDlgButton(hKeys, cbSendAltPrintScrn, BST_CHECKED);
	if (isSendPrintScrn) CheckDlgButton(hKeys, cbSendPrintScrn, BST_CHECKED);
	if (isSendCtrlEsc) CheckDlgButton(hKeys, cbSendCtrlEsc, BST_CHECKED);

	RegisterTipsFor(hKeys);
	return 0;
}

LRESULT CSettings::OnInitDialog_Tabs()
{
	if (gpSet->EnableThemeDialogTextureF)
		gpSet->EnableThemeDialogTextureF(hTabs, 6/*ETDT_ENABLETAB*/);

	if (isTabs)
		CheckDlgButton(hTabs, cbTabs, (isTabs==1) ? BST_CHECKED : BST_INDETERMINATE);

	if (isTabSelf)
		CheckDlgButton(hTabs, cbTabSelf, BST_CHECKED);

	if (isTabRecent)
		CheckDlgButton(hTabs, cbTabRecent, BST_CHECKED);

	if (isTabLazy)
		CheckDlgButton(hTabs, cbTabLazy, BST_CHECKED);

	if (gpSet->bHideInactiveConsoleTabs)
		CheckDlgButton(hTabs, cbHideInactiveConTabs, BST_CHECKED);

	if (isMulti)
		CheckDlgButton(hTabs, cbMultiCon, BST_CHECKED);

	if (isMultiNewConfirm)
		CheckDlgButton(hTabs, cbNewConfirm, BST_CHECKED);

	if (AutoBufferHeight)
		CheckDlgButton(hTabs, cbLongOutput, BST_CHECKED);

	wchar_t sz[16];
	SendDlgItemMessage(hTabs, tLongOutputHeight, EM_SETLIMITTEXT, 5, 0);
	SetDlgItemText(hTabs, tLongOutputHeight, _ltow(gpSet->DefaultBufferHeight, sz, 10));
	EnableWindow(GetDlgItem(hTabs, tLongOutputHeight), AutoBufferHeight);
	// 16bit Height
	SendDlgItemMessage(hTabs, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"Auto");
	SendDlgItemMessage(hTabs, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"25 lines");
	SendDlgItemMessage(hTabs, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"28 lines");
	SendDlgItemMessage(hTabs, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"43 lines");
	SendDlgItemMessage(hTabs, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"50 lines");
	SendDlgItemMessage(hTabs, lbNtvdmHeight, CB_SETCURSEL, !ntvdmHeight ? 0 :
	                   ((ntvdmHeight == 25) ? 1 : ((ntvdmHeight == 28) ? 2 : ((ntvdmHeight == 43) ? 3 : 4))), 0); //-V112
	// Cmd.exe output cp
	SendDlgItemMessage(hTabs, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"Undefined");
	SendDlgItemMessage(hTabs, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"Automatic");
	SendDlgItemMessage(hTabs, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"Unicode");
	SendDlgItemMessage(hTabs, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"OEM");
	SendDlgItemMessage(hTabs, lbCmdOutputCP, CB_SETCURSEL, nCmdOutputCP, 0);
	//
	//SetupHotkeyChecks(hTabs);
	SendDlgItemMessage(hTabs, hkNewConsole, HKM_SETRULES, HKCOMB_A|HKCOMB_C|HKCOMB_CA|HKCOMB_S|HKCOMB_SA|HKCOMB_SC|HKCOMB_SCA, 0);
	SendDlgItemMessage(hTabs, hkNewConsole, HKM_SETHOTKEY, gpSet->icMultiNew, 0);
	SendDlgItemMessage(hTabs, hkSwitchConsole, HKM_SETRULES, HKCOMB_A|HKCOMB_C|HKCOMB_CA|HKCOMB_S|HKCOMB_SA|HKCOMB_SC|HKCOMB_SCA, 0);
	SendDlgItemMessage(hTabs, hkSwitchConsole, HKM_SETHOTKEY, gpSet->icMultiNext, 0);
	SendDlgItemMessage(hTabs, hkCloseConsole, HKM_SETRULES, HKCOMB_A|HKCOMB_C|HKCOMB_CA|HKCOMB_S|HKCOMB_SA|HKCOMB_SC|HKCOMB_SCA, 0);
	SendDlgItemMessage(hTabs, hkCloseConsole, HKM_SETHOTKEY, gpSet->icMultiRecreate, 0);
	SendDlgItemMessage(hTabs, hkDelConsole, HKM_SETRULES, HKCOMB_A|HKCOMB_C|HKCOMB_CA|HKCOMB_S|HKCOMB_SA|HKCOMB_SC|HKCOMB_SCA, 0);
	SendDlgItemMessage(hTabs, hkDelConsole, HKM_SETHOTKEY, gpSet->icMultiClose, 0);
	SendDlgItemMessage(hTabs, hkStartCmd, HKM_SETRULES, HKCOMB_A|HKCOMB_C|HKCOMB_CA|HKCOMB_S|HKCOMB_SA|HKCOMB_SC|HKCOMB_SCA, 0);
	SendDlgItemMessage(hTabs, hkStartCmd, HKM_SETHOTKEY, gpSet->icMultiCmd, 0);
	SendDlgItemMessage(hTabs, hkMinRestore, HKM_SETRULES, HKCOMB_A|HKCOMB_C|HKCOMB_CA|HKCOMB_S|HKCOMB_SA|HKCOMB_SC|HKCOMB_SCA, 0);
	SendDlgItemMessage(hTabs, hkMinRestore, HKM_SETHOTKEY, gpSet->icMinimizeRestore, 0);
	SendDlgItemMessage(hTabs, hkBufferMode, HKM_SETRULES, HKCOMB_A|HKCOMB_C|HKCOMB_CA|HKCOMB_S|HKCOMB_SA|HKCOMB_SC|HKCOMB_SCA, 0);
	SendDlgItemMessage(hTabs, hkBufferMode, HKM_SETHOTKEY, gpSet->icMultiBuffer, 0);
	// SendDlgItemMessage(hTabs, hkMinimizeRestore, HKM_SETHOTKEY, gpSet->icMinimizeRestore, 0);
	if (gpSet->isUseWinNumber)
		CheckDlgButton(hTabs, cbUseWinNumber, BST_CHECKED);

	SetDlgItemText(hTabs, tTabConsole, gpSet->szTabConsole);
	SetDlgItemText(hTabs, tTabViewer, gpSet->szTabViewer);
	SetDlgItemText(hTabs, tTabEditor, gpSet->szTabEditor);
	SetDlgItemText(hTabs, tTabEditorMod, gpSet->szTabEditorModified);
	SetDlgItemInt(hTabs, tTabLenMax, gpSet->nTabLenMax, FALSE);

	if (gpSet->bAdminShield)
		CheckDlgButton(hTabs, cbAdminShield, BST_CHECKED);

	SetDlgItemText(hTabs, tAdminSuffix, gpSet->szAdminTitleSuffix);
	EnableWindow(GetDlgItem(hTabs, tAdminSuffix), !gpSet->bAdminShield);
	RegisterTipsFor(hTabs);
	return 0;
}

LRESULT CSettings::OnInitDialog_Color()
{
	if (gpSet->EnableThemeDialogTextureF)
		gpSet->EnableThemeDialogTextureF(hColors, 6/*ETDT_ENABLETAB*/);

#define getR(inColorref) (byte)inColorref
#define getG(inColorref) (byte)(inColorref >> 8)
#define getB(inColorref) (byte)(inColorref >> 16)

	//wchar_t temp[MAX_PATH];

	for(uint c = c0; c <= MAX_COLOR_EDT_ID; c++)
		ColorSetEdit(hColors, c);

	DWORD nVal = nExtendColor;
	FillListBoxItems(GetDlgItem(hColors, lbExtendIdx), countof(Settings::szColorIdxSh),
	                 Settings::szColorIdxSh, Settings::nColorIdxSh, nVal);
	nExtendColor = nVal;
	CheckDlgButton(hColors, cbExtendColors, isExtendColors ? BST_CHECKED : BST_UNCHECKED);
	OnColorButtonClicked(cbExtendColors, 0);
	CheckDlgButton(hColors, cbTrueColorer, isTrueColorer ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hColors, cbFadeInactive, isFadeInactive ? BST_CHECKED : BST_UNCHECKED);
	SetDlgItemInt(hColors, tFadeLow, mn_FadeLow, FALSE);
	SetDlgItemInt(hColors, tFadeHigh, mn_FadeHigh, FALSE);
	// Default colors
	memmove(gdwLastColors, Colors, sizeof(gdwLastColors));
	SendDlgItemMessage(hColors, lbDefaultColors, CB_ADDSTRING, 0, (LPARAM) L"<Current color scheme>");

	//SendDlgItemMessage(hColors, lbDefaultColors, CB_ADDSTRING, 0, (LPARAM) L"Default color sheme (Windows standard)");
	//SendDlgItemMessage(hColors, lbDefaultColors, CB_ADDSTRING, 0, (LPARAM) L"Gamma 1 (for use with dark monitors)");
	for(uint i=0; i<countof(DefColors); i++)
		SendDlgItemMessage(hColors, lbDefaultColors, CB_ADDSTRING, 0, (LPARAM) DefColors[i].pszTitle);

	SendDlgItemMessage(hColors, lbDefaultColors, CB_SETCURSEL, 0, 0);
	gbLastColorsOk = TRUE;
	SendDlgItemMessage(hColors, slTransparent, TBM_SETRANGE, (WPARAM) true, (LPARAM) MAKELONG(MIN_ALPHA_VALUE, 255));
	SendDlgItemMessage(hColors, slTransparent, TBM_SETPOS  , (WPARAM) true, (LPARAM) nTransparent);
	CheckDlgButton(hColors, cbTransparent, (nTransparent!=255) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hColors, cbUserScreenTransparent, isUserScreenTransparent ? BST_CHECKED : BST_UNCHECKED);
	RegisterTipsFor(hColors);
	return 0;
}

LRESULT CSettings::OnInitDialog_Views()
{
	if (gpSet->EnableThemeDialogTextureF)
		gpSet->EnableThemeDialogTextureF(hViews, 6/*ETDT_ENABLETAB*/);

	// пока выключим
	EnableWindow(GetDlgItem(hViews, bApplyViewSettings), gpConEmu->ActiveCon()->IsPanelViews());
	SetDlgItemText(hViews, tThumbsFontName, ThSet.Thumbs.sFontName);
	SetDlgItemText(hViews, tTilesFontName, ThSet.Tiles.sFontName);

	if (gpSet->mh_EnumThread == NULL)  // Если шрифты уже считаны
		OnInitDialog_ViewsFonts(); // можно скопировать список с вкладки hMain

	DWORD nVal;
	wchar_t temp[MAX_PATH];

	for(uint i=0; i < countof(Settings::FSizesSmall); i++)
	{
		_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", Settings::FSizesSmall[i]);
		SendDlgItemMessage(hViews, tThumbsFontSize, CB_ADDSTRING, 0, (LPARAM) temp);
		SendDlgItemMessage(hViews, tTilesFontSize, CB_ADDSTRING, 0, (LPARAM) temp);
	}

	_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", ThSet.Thumbs.nFontHeight);
	SelectStringExact(hViews, tThumbsFontSize, temp);
	_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", ThSet.Tiles.nFontHeight);
	SelectStringExact(hViews, tTilesFontSize, temp);
	SetDlgItemInt(hViews, tThumbsImgSize, ThSet.Thumbs.nImgSize, FALSE);
	SetDlgItemInt(hViews, tThumbsX1, ThSet.Thumbs.nSpaceX1, FALSE);
	SetDlgItemInt(hViews, tThumbsY1, ThSet.Thumbs.nSpaceY1, FALSE);
	SetDlgItemInt(hViews, tThumbsX2, ThSet.Thumbs.nSpaceX2, FALSE);
	SetDlgItemInt(hViews, tThumbsY2, ThSet.Thumbs.nSpaceY2, FALSE);
	SetDlgItemInt(hViews, tThumbsSpacing, ThSet.Thumbs.nLabelSpacing, FALSE);
	SetDlgItemInt(hViews, tThumbsPadding, ThSet.Thumbs.nLabelPadding, FALSE);
	SetDlgItemInt(hViews, tTilesImgSize, ThSet.Tiles.nImgSize, FALSE);
	SetDlgItemInt(hViews, tTilesX1, ThSet.Tiles.nSpaceX1, FALSE);
	SetDlgItemInt(hViews, tTilesY1, ThSet.Tiles.nSpaceY1, FALSE);
	SetDlgItemInt(hViews, tTilesX2, ThSet.Tiles.nSpaceX2, FALSE);
	SetDlgItemInt(hViews, tTilesY2, ThSet.Tiles.nSpaceY2, FALSE);
	SetDlgItemInt(hViews, tTilesSpacing, ThSet.Tiles.nLabelSpacing, FALSE);
	SetDlgItemInt(hViews, tTilesPadding, ThSet.Tiles.nLabelPadding, FALSE);
	FillListBoxItems(GetDlgItem(hViews, tThumbMaxZoom), countof(Settings::szThumbMaxZoom),
	                 Settings::szThumbMaxZoom, Settings::nThumbMaxZoom, ThSet.nMaxZoom);

	// Colors
	for(uint c = c32; c <= c34; c++)
		ColorSetEdit(hViews, c);

	nVal = ThSet.crBackground.ColorIdx;
	FillListBoxItems(GetDlgItem(hViews, lbThumbBackColorIdx), countof(Settings::szColorIdxTh),
	                 Settings::szColorIdxTh, Settings::nColorIdxTh, nVal);
	CheckRadioButton(hViews, rbThumbBackColorIdx, rbThumbBackColorRGB,
	                 ThSet.crBackground.UseIndex ? rbThumbBackColorIdx : rbThumbBackColorRGB);
	CheckDlgButton(hViews, cbThumbPreviewBox, ThSet.nPreviewFrame ? 1 : 0);
	nVal = ThSet.crPreviewFrame.ColorIdx;
	FillListBoxItems(GetDlgItem(hViews, lbThumbPreviewBoxColorIdx), countof(Settings::szColorIdxTh),
	                 Settings::szColorIdxTh, Settings::nColorIdxTh, nVal);
	CheckRadioButton(hViews, rbThumbPreviewBoxColorIdx, rbThumbPreviewBoxColorRGB,
	                 ThSet.crPreviewFrame.UseIndex ? rbThumbPreviewBoxColorIdx : rbThumbPreviewBoxColorRGB);
	CheckDlgButton(hViews, cbThumbSelectionBox, ThSet.nSelectFrame ? 1 : 0);
	nVal = ThSet.crSelectFrame.ColorIdx;
	FillListBoxItems(GetDlgItem(hViews, lbThumbSelectionBoxColorIdx), countof(Settings::szColorIdxTh),
	                 Settings::szColorIdxTh, Settings::nColorIdxTh, nVal);
	CheckRadioButton(hViews, rbThumbSelectionBoxColorIdx, rbThumbSelectionBoxColorRGB,
	                 ThSet.crSelectFrame.UseIndex ? rbThumbSelectionBoxColorIdx : rbThumbSelectionBoxColorRGB);

	if ((ThSet.bLoadPreviews & 3) == 3)
		CheckDlgButton(hViews, cbThumbLoadFiles, BST_CHECKED);
	else if ((ThSet.bLoadPreviews & 3) == 1)
		CheckDlgButton(hViews, cbThumbLoadFiles, BST_INDETERMINATE);

	CheckDlgButton(hViews, cbThumbLoadFolders, ThSet.bLoadFolders);
	SetDlgItemInt(hViews, tThumbLoadingTimeout, ThSet.nLoadTimeout, FALSE);
	CheckDlgButton(hViews, cbThumbUsePicView2, ThSet.bUsePicView2);
	CheckDlgButton(hViews, cbThumbRestoreOnStartup, ThSet.bRestoreOnStartup);
	RegisterTipsFor(hViews);
	return 0;
}

LRESULT CSettings::OnInitDialog_ViewsFonts()
{
	DWORD bAlmostMonospace;
	int nIdx, nCount, i;
	wchar_t szFontName[128]; // не должно быть более 32
	nCount = SendDlgItemMessage(gpSet->hMain, tFontFace, CB_GETCOUNT, 0, 0);

	for(i = 0; i < nCount; i++)
	{
		if (SendDlgItemMessage(gpSet->hMain, tFontFace, CB_GETLBTEXT, i, (LPARAM) szFontName) > 0)
		{
			bAlmostMonospace = (DWORD)SendDlgItemMessage(gpSet->hMain, tFontFace, CB_GETITEMDATA, i, 0);
			nIdx = SendDlgItemMessage(gpSet->hViews, tThumbsFontName, CB_ADDSTRING, 0, (LPARAM) szFontName);
			SendDlgItemMessage(gpSet->hViews, tThumbsFontName, CB_SETITEMDATA, nIdx, bAlmostMonospace);
			nIdx = SendDlgItemMessage(gpSet->hViews, tTilesFontName, CB_ADDSTRING, 0, (LPARAM) szFontName);
			SendDlgItemMessage(gpSet->hViews, tTilesFontName, CB_SETITEMDATA, nIdx, bAlmostMonospace);
		}
	}

	GetDlgItemText(gpSet->hViews, tThumbsFontName, szFontName, 128);
	gpSet->SelectString(gpSet->hViews, tThumbsFontName, szFontName);
	GetDlgItemText(gpSet->hViews, tTilesFontName, szFontName, 128);
	gpSet->SelectString(gpSet->hViews, tTilesFontName, szFontName);
	return TRUE;
}

LRESULT CSettings::OnInitDialog_Info()
{
	if (gpSet->EnableThemeDialogTextureF)
		gpSet->EnableThemeDialogTextureF(hInfo, 6/*ETDT_ENABLETAB*/);

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

LRESULT CSettings::OnInitDialog_Debug()
{
	if (gpSet->EnableThemeDialogTextureF)
		gpSet->EnableThemeDialogTextureF(hDebug, 6/*ETDT_ENABLETAB*/);

	//if (isDebugSteps) CheckDlgButton(hExt, cbDebugSteps, BST_CHECKED);
		
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
	SetWindowPos(hTip, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);


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

LRESULT CSettings::OnButtonClicked(WPARAM wParam, LPARAM lParam)
{
	WORD CB = LOWORD(wParam);

	switch(CB)
	{
		case IDOK:
		case IDCANCEL:
		case IDCLOSE:
			// -- перенесено в WM_CLOSE
			//if (isTabs==1) gpConEmu->ForceShowTabs(TRUE); else
			//if (isTabs==0) gpConEmu->ForceShowTabs(FALSE); else
			//	gpConEmu->mp_TabBar->Update();
			//gpConEmu->OnPanelViewSettingsChanged();
			SendMessage(ghOpWnd, WM_CLOSE, 0, 0);
			break;
		case bSaveSettings:

			if (IsWindowEnabled(GetDlgItem(hMain, cbApplyPos)))  // были изменения в полях размера/положения
				OnButtonClicked(cbApplyPos, 0);

			if (SaveSettings())
				SendMessage(ghOpWnd,WM_COMMAND,IDOK,0);

			break;
		case rNoneAA:
		case rStandardAA:
		case rCTAA:
			PostMessage(hMain, gpSet->mn_MsgRecreateFont, wParam, 0);
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

				if (gpConEmu->isZoomed() || gpConEmu->isIconic() || isFullScreen)
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

				if (!isFullScreen)
					gpConEmu->SetWindowMode(rFullScreen);
			}

			// Запомнить "идеальный" размер окна, выбранный пользователем
			gpConEmu->UpdateIdealRect(TRUE);
			EnableWindow(GetDlgItem(hMain, cbApplyPos), FALSE);
			apiSetForegroundWindow(ghOpWnd);
			break;
		case rCascade:
		case rFixed:
			wndCascade = CB == rCascade;
			break;
		case cbAutoSaveSizePos:
			isAutoSaveSizePos = IsChecked(hMain, cbAutoSaveSizePos);
			break;
		case cbFontAuto:
			isFontAutoSize = IsChecked(hMain, cbFontAuto);

			if (isFontAutoSize && LogFont.lfFaceName[0] == L'['
			        && !wcsncmp(LogFont.lfFaceName+1, RASTER_FONTS_NAME, _tcslen(RASTER_FONTS_NAME)))
			{
				isFontAutoSize = false;
				CheckDlgButton(hMain, cbFontAuto, BST_UNCHECKED);
				ShowFontErrorTip(szRasterAutoError);
			}

			break;
		case cbFixFarBorders:

			//isFixFarBorders = !isFixFarBorders;
			switch(IsChecked(hMain, cbFixFarBorders))
			{
				case BST_UNCHECKED:
					isFixFarBorders = 0; break;
				case BST_CHECKED:
					isFixFarBorders = 1; break;
				case BST_INDETERMINATE:
					isFixFarBorders = 2; break;
			}

			gpConEmu->Update(true);
			break;
		case cbCursorColor:
			isCursorColor = IsChecked(hMain,cbCursorColor);
			gpConEmu->Update(true);
			break;
		case cbCursorBlink:
			isCursorBlink = IsChecked(hMain,cbCursorBlink);
			break;
		case cbMultiCon:
			isMulti = IsChecked(hTabs, cbMultiCon);
			break;
		case cbNewConfirm:
			isMultiNewConfirm = IsChecked(hTabs, cbNewConfirm);
			break;
		case cbLongOutput:
			AutoBufferHeight = IsChecked(hTabs, cbLongOutput);
			gpConEmu->UpdateFarSettings();
			EnableWindow(GetDlgItem(hTabs, tLongOutputHeight), AutoBufferHeight);
			break;
		case cbBold:
		case cbItalic:
		{
			PostMessage(hMain, gpSet->mn_MsgRecreateFont, wParam, 0);
		}
		break;
		case cbBgImage:
		{
			isShowBgImage = IsChecked(hMain, cbBgImage);
			EnableWindow(GetDlgItem(hMain, tBgImage), isShowBgImage);
			//EnableWindow(GetDlgItem(hMain, tDarker), isShowBgImage);
			//EnableWindow(GetDlgItem(hMain, slDarker), isShowBgImage);
			EnableWindow(GetDlgItem(hMain, bBgImage), isShowBgImage);
			//EnableWindow(GetDlgItem(hMain, rBgUpLeft), isShowBgImage);
			//EnableWindow(GetDlgItem(hMain, rBgStretch), isShowBgImage);
			//EnableWindow(GetDlgItem(hMain, rBgTile), isShowBgImage);
			BOOL lbNeedLoad = (mp_Bg == NULL);

			if (isShowBgImage && bgImageDarker == 0)
			{
				if (MessageBox(ghOpWnd,
				              L"Background image will NOT be visible\n"
				              L"while 'Darkening' is 0. Increase it?",
				              gpConEmu->ms_ConEmuVer, MB_YESNO|MB_ICONEXCLAMATION)!=IDNO)
				{
					bgImageDarker = 0x46;
					SendDlgItemMessage(hMain, slDarker, TBM_SETPOS  , (WPARAM) true, (LPARAM) bgImageDarker);
					TCHAR tmp[10];
					_wsprintf(tmp, SKIPLEN(countof(tmp)) L"%i", gpSet->bgImageDarker);
					SetDlgItemText(hMain, tDarker, tmp);
					lbNeedLoad = TRUE;
				}
			}

			if (lbNeedLoad)
			{
				gpSet->LoadBackgroundFile(gpSet->sBgImage, true);
			}

			gpConEmu->Update(true);
		}
		break;
		case rBgUpLeft:
		case rBgStretch:
		case rBgTile:
		{
			bgOperation = (char)(CB - rBgUpLeft);
			gpSet->LoadBackgroundFile(gpSet->sBgImage, true);
			gpConEmu->Update(true);
		}
		break;
		case cbBgAllowPlugin:
		{
			isBgPluginAllowed = IsChecked(hMain, cbBgAllowPlugin);
			NeedBackgroundUpdate();
			gpConEmu->Update(true);
		}
		break;
		case cbRClick:
			isRClickSendKey = IsChecked(hExt, cbRClick); //0-1-2
			break;
		case cbSafeFarClose:
			isSafeFarClose = IsChecked(hExt, cbSafeFarClose);
			break;
		case cbMinToTray:
			isMinToTray = IsChecked(hExt, cbMinToTray);
			break;
		case cbAlwaysShowTrayIcon:
			isAlwaysShowTrayIcon = IsChecked(hExt, cbAlwaysShowTrayIcon);
			Icon.SettingsChanged();
			break;
		case cbHideCaption:
			isHideCaption = IsChecked(hExt, cbHideCaption);
			break;
		case cbHideCaptionAlways:
			mb_HideCaptionAlways = IsChecked(hExt, cbHideCaptionAlways);

			if (isHideCaptionAlways())
			{
				CheckDlgButton(hExt, cbHideCaptionAlways, BST_CHECKED);
				TODO("показать тултип, что скрытие обязательно при прозрачности");
			}

			gpConEmu->OnHideCaption();
			break;
		case bHideCaptionSettings:
			DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_MORE_HIDE), ghOpWnd, hideOpProc);
			break;
		case cbConsoleTextSelection:
			isConsoleTextSelection = IsChecked(hExt, cbConsoleTextSelection);
			break;
		case bCTSSettings:
			DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_MORE_SELECTION), ghOpWnd, selectionOpProc);
			break;
		case cbFARuseASCIIsort:
			isFARuseASCIIsort = IsChecked(hExt, cbFARuseASCIIsort);
			gpConEmu->UpdateFarSettings();
			break;
		case cbShellNoZoneCheck:
			isShellNoZoneCheck = IsChecked(hExt, cbShellNoZoneCheck);
			gpConEmu->UpdateFarSettings();
			break;
		case cbDragPanel:
			isDragPanel = IsChecked(hExt, cbDragPanel);
			gpConEmu->OnSetCursor();
			break;
		case cbTryToCenter:
			isTryToCenter = IsChecked(hExt, cbTryToCenter);
			gpConEmu->OnSize(-1);
			gpConEmu->InvalidateAll();
			break;
		case cbAlwaysShowScrollbar:
			isAlwaysShowScrollbar = IsChecked(hExt, cbAlwaysShowScrollbar);
			gpConEmu->OnSize(-1);
			gpConEmu->m_Back->TrackMouse();
			gpConEmu->InvalidateAll();
			break;
		case cbFarHourglass:
			isFarHourglass = IsChecked(hExt, cbFarHourglass);
			gpConEmu->OnSetCursor();
			break;
		case cbExtendUCharMap:
			isExtendUCharMap = IsChecked(hExt, cbExtendUCharMap);
			gpConEmu->Update(true);
			break;
		case cbFixAltOnAltTab:
			isFixAltOnAltTab = IsChecked(hExt, cbFixAltOnAltTab);
			break;
		case cbAutoRegFonts:
			isAutoRegisterFonts = IsChecked(hExt, cbAutoRegFonts);
			break;
		case cbDebugSteps:
			isDebugSteps = IsChecked(hExt, cbDebugSteps);
			break;
		case cbDragL:
		case cbDragR:
			isDragEnabled =
			    (IsChecked(hExt, cbDragL) ? DRAG_L_ALLOWED : 0) |
			    (IsChecked(hExt, cbDragR) ? DRAG_R_ALLOWED : 0);
			break;
		case cbDropEnabled:
			isDropEnabled = IsChecked(hExt, cbDropEnabled);
			break;
		case cbDnDCopy:
			isDefCopy = IsChecked(hExt, cbDnDCopy) == BST_CHECKED;
			break;
		case cbDragImage:
			isDragOverlay = IsChecked(hExt, cbDragImage);
			break;
		case cbDragIcons:
			isDragShowIcons = IsChecked(hExt, cbDragIcons) == BST_CHECKED;
			break;
		case cbEnhanceGraphics: // Progressbars and scrollbars
			isEnhanceGraphics = IsChecked(hExt, cbEnhanceGraphics);
			gpConEmu->Update(true);
			break;
		case cbTabs:

			switch(IsChecked(hTabs, cbTabs))
			{
				case BST_UNCHECKED:
					isTabs = 0; break;
				case BST_CHECKED:
					isTabs = 1; break;
				case BST_INDETERMINATE:
					isTabs = 2; break;
			}

			TODO("Хорошо бы сразу видимость табов менять");
			//gpConEmu->mp_TabBar->Update(TRUE); -- это как-то неправильно работает.
			break;
		case cbTabSelf:
			isTabSelf = IsChecked(hTabs, cbTabSelf);
			break;
		case cbTabRecent:
			isTabRecent = IsChecked(hTabs, cbTabRecent);
			break;
		case cbTabLazy:
			isTabLazy = IsChecked(hTabs, cbTabLazy);
			break;
		case cbRSelectionFix:
			isRSelFix = IsChecked(hExt, cbRSelectionFix);
			break;
		case cbSkipActivation:
			isMouseSkipActivation = IsChecked(hExt, cbSkipActivation);
			break;
		case cbSkipMove:
			isMouseSkipMoving = IsChecked(hExt, cbSkipMove);
			break;
		case cbMonitorConsoleLang:
			isMonitorConsoleLang = IsChecked(hExt, cbMonitorConsoleLang);
			break;
		case cbSkipFocusEvents:
			isSkipFocusEvents = IsChecked(hExt, cbSkipFocusEvents);
			break;
		case cbMonospace:
		{
			BYTE cMonospaceNow = isMonospace;
			isMonospace = IsChecked(hMain, cbMonospace);

			if (isMonospace) isMonospaceSelected = isMonospace;

			mb_IgnoreEditChanged = TRUE;
			ResetFontWidth();
			gpConEmu->Update(true);
			mb_IgnoreEditChanged = FALSE;
		} break;
		case cbExtendFonts:
		{
			isExtendFonts = IsChecked(hMain, cbExtendFonts);
			gpConEmu->Update(true);
		} break;
		//case cbAutoConHandle:
		//	isUpdConHandle = !isUpdConHandle;
		//	gpConEmu->Update(true);
		//	break;
		case rCursorH:
		case rCursorV:

			if (wParam == rCursorV)
				isCursorV = true;
			else
				isCursorV = false;

			gpConEmu->Update(true);
			break;
		case cbBlockInactiveCursor:
			isCursorBlockInactive = IsChecked(hMain, cbBlockInactiveCursor);
			gpConEmu->ActiveCon()->Invalidate();
			break;
		case bBgImage:
		{
			wchar_t temp[MAX_PATH];
			GetDlgItemText(hMain, tBgImage, temp, MAX_PATH);
			OPENFILENAME ofn; memset(&ofn,0,sizeof(ofn));
			ofn.lStructSize=sizeof(ofn);
			ofn.hwndOwner = ghOpWnd;
			ofn.lpstrFilter = L"Bitmap images (*.bmp)\0*.bmp\0\0";
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
					wcscpy_c(sBgImage, temp);
					SetDlgItemText(hMain, tBgImage, temp);
					gpConEmu->Update(true);
				}
			}
		}
		break;
		case cbVisible:
			isConVisible = IsChecked(hExt, cbVisible);

			if (isConVisible)
			{
				// Если показывать - то только текущую (иначе на экране мешанина консолей будет
				gpConEmu->ActiveCon()->RCon()->ShowConsole(isConVisible);
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
			//	isLockRealConsolePos = IsChecked(hExt, cbLockRealConsolePos);
			//	break;
		case cbUseInjects:
			isUseInjects = IsChecked(hExt, cbUseInjects);
			gpConEmu->OnGlobalSettingsChanged();
			break;
		case cbPortableRegistry:
			#ifdef USEPORTABLEREGISTRY
			isPortableReg = IsChecked(hExt, cbPortableRegistry);
			// Проверить, готов ли к использованию
			if (!gpConEmu->PreparePortableReg())
			{
				isPortableReg = false;
				CheckDlgButton(hExt, cbPortableRegistry, BST_UNCHECKED);
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
			isDesktopMode = IsChecked(hExt, cbDesktopMode);
			gpConEmu->OnDesktopMode();
			break;
		case cbAlwaysOnTop:
			isAlwaysOnTop = IsChecked(hExt, cbAlwaysOnTop);
			gpConEmu->OnAlwaysOnTop();
			break;
		case cbSleepInBackground:
			isSleepInBackground = IsChecked(hExt, cbSleepInBackground);
			gpConEmu->ActiveCon()->RCon()->OnGuiFocused(TRUE);
			break;
		case cbDisableFarFlashing:
			isDisableFarFlashing = IsChecked(hExt, cbDisableFarFlashing);
			break;
		case cbDisableAllFlashing:
			isDisableAllFlashing = IsChecked(hExt, cbDisableAllFlashing);
			break;
		case cbTabsInCaption:
			isTabsInCaption = IsChecked(hExt, cbTabsInCaption);
			//RedrawWindow(ghWnd, NULL, NULL, RDW_UPDATENOW|RDW_FRAME);
			//gpConEmu->OnNcMessage(ghWnd, WM_NCPAINT, 0,0);
			SendMessage(ghWnd, WM_NCACTIVATE, 0, 0);
			SendMessage(ghWnd, WM_NCPAINT, 0, 0);
			break;
		case cbAdminShield:
			gpSet->bAdminShield = IsChecked(hTabs, cbAdminShield);
			EnableWindow(GetDlgItem(hTabs, tAdminSuffix), !gpSet->bAdminShield);
			gpConEmu->mp_TabBar->Update(TRUE);
			break;
		case cbHideInactiveConTabs:
			gpSet->bHideInactiveConsoleTabs = IsChecked(hTabs, cbHideInactiveConTabs);
			gpConEmu->mp_TabBar->Update(TRUE);
			break;
			
		case cbUseWinArrows:
		case cbUseWinNumberK:
		case cbUseWinTab:
			switch (CB)
			{
				case cbUseWinArrows:
					gpSet->isUseWinArrows = IsChecked(hKeys, CB);
					break;
				case cbUseWinNumberK:
					gpSet->isUseWinNumber = IsChecked(hKeys, CB);
					if (hTabs) CheckDlgButton(hTabs, cbUseWinNumber, gpSet->isUseWinNumber ? BST_CHECKED : BST_UNCHECKED);
					break;
				case cbUseWinTab:
					gpSet->isUseWinTab = IsChecked(hKeys, CB);
					break;
			}
			gpConEmu->UpdateWinHookSettings();
			break;

		case cbUseWinNumber:
			gpSet->isUseWinNumber = IsChecked(hTabs, cbUseWinNumber);
			if (hKeys) CheckDlgButton(hKeys, cbUseWinNumberK, gpSet->isUseWinNumber ? BST_CHECKED : BST_UNCHECKED);
			gpConEmu->UpdateWinHookSettings();
			break;

		case cbSendAltEnter:
			// хуков не требует
			isSendAltEnter = IsChecked(hKeys, cbSendAltEnter);
			break;
		case cbSendAltSpace:
			// хуков не требует
			isSendAltSpace = IsChecked(hKeys, cbSendAltSpace);
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
				CheckDlgButton(hExt, cbDosBox, BST_CHECKED);
				EnableWindow(GetDlgItem(hExt, cbDosBox), FALSE); // изменение пока запрещено
			}
			else
			{
				CheckDlgButton(hExt, cbDosBox, BST_UNCHECKED);
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

		default:

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
				gpSet->SetupHotkeyChecks(hKeys);
				gpSet->MakeHostkeyModifier();
			}
	}

	return 0;
}

LRESULT CSettings::OnColorButtonClicked(WPARAM wParam, LPARAM lParam)
{
	WORD CB = LOWORD(wParam);

	switch(wParam)
	{
		case cbExtendColors:
			isExtendColors = IsChecked(hColors, cbExtendColors) == BST_CHECKED ? true : false;

			for(int i=16; i<32; i++) //-V112
				EnableWindow(GetDlgItem(hColors, tc0+i), isExtendColors);

			EnableWindow(GetDlgItem(hColors, lbExtendIdx), isExtendColors);

			if (lParam)
			{
				gpConEmu->Update(true);
			}

			break;
		case cbTrueColorer:
			isTrueColorer = IsChecked(hColors, cbTrueColorer);
			gpConEmu->UpdateFarSettings();
			gpConEmu->Update(true);
			break;
		case cbFadeInactive:
			isFadeInactive = IsChecked(hColors, cbFadeInactive);
			gpConEmu->ActiveCon()->Invalidate();
			break;
		case cbTransparent:
		{
			int newV = nTransparent;

			if (IsChecked(hColors, cbTransparent))
			{
				if (newV == 255) newV = 200;
			}
			else
			{
				newV = 255;
			}

			if (newV != nTransparent)
			{
				nTransparent = newV;
				SendDlgItemMessage(hColors, slTransparent, TBM_SETPOS, (WPARAM) true, (LPARAM)nTransparent);
				gpConEmu->OnTransparent();
			}
		} break;
		case cbUserScreenTransparent:
		{
			isUserScreenTransparent = IsChecked(hColors, cbUserScreenTransparent);

			if (hExt) CheckDlgButton(hExt, cbHideCaptionAlways, isHideCaptionAlways() ? BST_CHECKED : BST_UNCHECKED);

			gpConEmu->OnHideCaption(); // при прозрачности - обязательно скрытие заголовка + кнопки
			gpConEmu->UpdateWindowRgn();
		} break;
		default:

			if (CB >= c0 && CB <= MAX_COLOR_EDT_ID)
			{
				if (ColorEditDialog(hColors, CB))
				{
					gpConEmu->m_Back->Refresh();
					gpConEmu->Update(true);
				}
			}
	}

	return 0;
}

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

LRESULT CSettings::OnColorEditChanged(WPARAM wParam, LPARAM lParam)
{
	WORD TB = LOWORD(wParam);
	COLORREF color = 0;

	if (GetColorById(TB - (tc0-c0), &color))
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
	else if (TB == tFadeLow || TB == tFadeHigh)
	{
		BOOL lbOk = FALSE;
		UINT nVal = GetDlgItemInt(hColors, TB, &lbOk, FALSE);

		if (lbOk && nVal <= 255)
		{
			if (TB == tFadeLow)
				mn_FadeLow = nVal;
			else
				mn_FadeHigh = nVal;

			mb_FadeInitialized = false;
			mn_LastFadeSrc = mn_LastFadeDst = -1;
		}
	}

	return 0;
}

LRESULT CSettings::OnEditChanged(WPARAM wParam, LPARAM lParam)
{
	if (mb_IgnoreEditChanged)
		return 0;

	WORD TB = LOWORD(wParam);

	if (TB == tBgImage)
	{
		wchar_t temp[MAX_PATH];
		GetDlgItemText(hMain, tBgImage, temp, MAX_PATH);

		if (wcscmp(temp, sBgImage))
		{
			if (LoadBackgroundFile(temp, true))
			{
				wcscpy_c(sBgImage, temp);
				gpConEmu->Update(true);
			}
		}
	}
	else if (TB == tBgImageColors)
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
		if (newBgColors && nBgImageColors != newBgColors)
		{
			nBgImageColors = newBgColors;
			gpConEmu->Update(true);
		}
	}
	else if ((TB == tWndWidth || TB == tWndHeight) && IsChecked(hMain, rNormal) == BST_CHECKED)
	{
		EnableWindow(GetDlgItem(hMain, cbApplyPos), TRUE);
	}
	else if (TB == tDarker)
	{
		DWORD newV;
		TCHAR tmp[10];
		GetDlgItemText(hMain, tDarker, tmp, 10);
		newV = klatoi(tmp);

		if (newV < 256 && newV != bgImageDarker)
		{
			bgImageDarker = newV;
			SendDlgItemMessage(hMain, slDarker, TBM_SETPOS, (WPARAM) true, (LPARAM) bgImageDarker);

			// Картинку может установить и плагин
			if (isShowBgImage && sBgImage[0])
				LoadBackgroundFile(sBgImage);
			else
				NeedBackgroundUpdate();

			gpConEmu->Update(true);
		}
	}
	else if (TB == tLongOutputHeight)
	{
		BOOL lbOk = FALSE;
		wchar_t szTemp[16];
		UINT nNewVal = GetDlgItemInt(hTabs, tLongOutputHeight, &lbOk, FALSE);

		if (lbOk)
		{
			if (nNewVal >= 300 && nNewVal <= 9999)
				DefaultBufferHeight = nNewVal;
			else if (nNewVal > 9999)
				SetDlgItemText(hTabs, TB, _ltow(DefaultBufferHeight, szTemp, 10));
		}
		else
		{
			SetDlgItemText(hTabs, TB, _ltow(DefaultBufferHeight, szTemp, 10));
		}
	}
	else if (TB == hkNewConsole || TB == hkSwitchConsole || TB == hkCloseConsole)
	{
		UINT nHotKey = 0xFF & SendDlgItemMessage(hTabs, TB, HKM_GETHOTKEY, 0, 0);

		if (TB == hkNewConsole)
			gpSet->icMultiNew = nHotKey;
		else if (TB == hkSwitchConsole)
			gpSet->icMultiNext = nHotKey;
		else if (TB == hkCloseConsole)
			gpSet->icMultiRecreate = nHotKey;

		// SendDlgItemMessage(hTabs, hkMinimizeRestore, HKM_SETHOTKEY, gpSet->icMinimizeRestore, 0);
	}
	else if (TB == tTabConsole || TB == tTabViewer || TB == tTabEditor || TB == tTabEditorMod)
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
	}
	else if (TB == tTabLenMax)
	{
		BOOL lbOk = FALSE;
		DWORD n = GetDlgItemInt(hTabs, tTabLenMax, &lbOk, FALSE);

		if (n > 10 && n < CONEMUTABMAX)
		{
			gpSet->nTabLenMax = n;
			gpConEmu->mp_TabBar->Update(TRUE);
		}
	}
	else if (TB == tAdminSuffix)
	{
		GetDlgItemText(hTabs, tAdminSuffix, gpSet->szAdminTitleSuffix, countof(gpSet->szAdminTitleSuffix));
		gpConEmu->mp_TabBar->Update(TRUE);
	}

	return 0;
}

LRESULT CSettings::OnColorComboBox(WPARAM wParam, LPARAM lParam)
{
	WORD wId = LOWORD(wParam);

	if (wId == lbExtendIdx)
	{
		nExtendColor = SendDlgItemMessage(hColors, wId, CB_GETCURSEL, 0, 0);
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
				Colors[i] = pdwDefData[i]; //-V108
				_wsprintf(temp, SKIPLEN(countof(temp)) L"%i %i %i", getR(Colors[i]), getG(Colors[i]), getB(Colors[i]));
				SetDlgItemText(hColors, 1100 + i, temp);
				InvalidateRect(GetDlgItem(hColors, c0+i), 0, 1);
			}
		}
		else return 0;
	}

	gpConEmu->Update(true);
	return 0;
}

LRESULT CSettings::OnComboBox(WPARAM wParam, LPARAM lParam)
{
	WORD wId = LOWORD(wParam);

	if (wId == tFontCharset)
	{
		mb_CharSetWasSet = TRUE;
		PostMessage(hMain, gpSet->mn_MsgRecreateFont, wParam, 0);
	}
	else if (wId == tFontFace || wId == tFontFace2 ||
	        wId == tFontSizeY || wId == tFontSizeX ||
	        wId == tFontSizeX2 || wId == tFontSizeX3)
	{
		if (HIWORD(wParam) == CBN_SELCHANGE)
			PostMessage(hMain, mn_MsgRecreateFont, wId, 0);
		else
			mn_LastChangingFontCtrlId = wId;
	}
	else if (wId == lbLDragKey)
	{
		GetListBoxByte(hExt,wId,Settings::szKeys,Settings::nKeys,nLDragKey);
	}
	else if (wId == lbRDragKey)
	{
		GetListBoxByte(hExt,wId,Settings::szKeys,Settings::nKeys,nRDragKey);
	}
	else if (wId == lbNtvdmHeight)
	{
		INT_PTR num = SendDlgItemMessage(hTabs, wId, CB_GETCURSEL, 0, 0);
		ntvdmHeight = (num == 1) ? 25 : ((num == 2) ? 28 : ((num == 3) ? 43 : ((num == 4) ? 50 : 0))); //-V112
	}
	else if (wId == lbCmdOutputCP)
	{
		nCmdOutputCP = SendDlgItemMessage(hTabs, wId, CB_GETCURSEL, 0, 0);

		if (nCmdOutputCP == -1) nCmdOutputCP = 0;

		gpConEmu->UpdateFarSettings();
	}
	else if (wId == lbExtendFontNormalIdx || wId == lbExtendFontBoldIdx || wId == lbExtendFontItalicIdx)
	{
		if (wId == lbExtendFontNormalIdx)
			nFontNormalColor = GetNumber(hMain, wId);
		else if (wId == lbExtendFontBoldIdx)
			nFontBoldColor = GetNumber(hMain, wId);
		else if (wId == lbExtendFontItalicIdx)
			nFontItalicColor = GetNumber(hMain, wId);

		if (isExtendFonts)
			gpConEmu->Update(true);
	}

	return 0;
}

LRESULT CSettings::OnTab(LPNMHDR phdr)
{
	if (gpSet->szFontError[0])
	{
		gpSet->szFontError[0] = 0;
		SendMessage(hwndBalloon, TTM_TRACKACTIVATE, FALSE, (LPARAM)&tiBalloon);
		SendMessage(hwndTip, TTM_ACTIVATE, TRUE, 0);
	}

	switch(phdr->code)
	{
		case TCN_SELCHANGE:
		{
			int nSel = TabCtrl_GetCurSel(phdr->hwndFrom);
			HWND* phCurrent = NULL;
			UINT  nDlgRc = 0;
			DLGPROC dlgProc = NULL;
			BOOL bJustCreated = FALSE;

			if (nSel==0)
			{
				phCurrent = &hMain;
			}
			else if (nSel==1)
			{
				phCurrent = &hExt;
				nDlgRc = IDD_SPG_FEATURE;
				dlgProc = extOpProc;
			}
			else if (nSel==2)
			{
				phCurrent = &hKeys;
				nDlgRc = IDD_SPG_KEYS;
				dlgProc = keysOpProc;
			}
			else if (nSel==3)
			{
				phCurrent = &hTabs;
				nDlgRc = IDD_SPG_TABS;
				dlgProc = tabsOpProc;
			}
			else if (nSel==4) //-V112
			{
				phCurrent = &hColors;
				nDlgRc = IDD_SPG_COLORS;
				dlgProc = colorOpProc;
			}
			else if (nSel==5)
			{
				phCurrent = &hViews;
				nDlgRc = IDD_SPG_VIEWS;
				dlgProc = viewsOpProc;
			}
			else if (nSel==6)
			{
				phCurrent = &hDebug;
				nDlgRc = IDD_SPG_DEBUG;
				dlgProc = debugOpProc;
			}
			else
			{
				phCurrent = &hInfo;
				nDlgRc = IDD_SPG_INFO;
				dlgProc = infoOpProc;
			}

			if (*phCurrent == NULL && nDlgRc && dlgProc)
			{
				SetCursor(LoadCursor(NULL,IDC_WAIT));
				HWND _hwndTab = GetDlgItem(ghOpWnd, tabMain);
				RECT rcClient; GetWindowRect(_hwndTab, &rcClient);
				MapWindowPoints(NULL, ghOpWnd, (LPPOINT)&rcClient, 2);
				TabCtrl_AdjustRect(_hwndTab, FALSE, &rcClient);
				*phCurrent = CreateDialog((HINSTANCE)GetModuleHandle(NULL),
				                          MAKEINTRESOURCE(nDlgRc), ghOpWnd, dlgProc);
				MoveWindow(*phCurrent, rcClient.left, rcClient.top, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top, 0);
				bJustCreated = TRUE;
			}

			if (*phCurrent != NULL)
			{
				if (!bJustCreated)
				{
					SendMessage(*phCurrent, mn_ActivateTabMsg, 0, 0);
				}
				
				apiShowWindow(*phCurrent, SW_SHOW);

				if (*phCurrent != hMain)   apiShowWindow(hMain,   SW_HIDE);

				if (*phCurrent != hExt)    apiShowWindow(hExt,    SW_HIDE);
				
				if (*phCurrent != hKeys)   apiShowWindow(hKeys,   SW_HIDE);

				if (*phCurrent != hTabs)   apiShowWindow(hTabs,   SW_HIDE);

				if (*phCurrent != hColors) apiShowWindow(hColors, SW_HIDE);

				if (*phCurrent != hViews)  apiShowWindow(hViews,  SW_HIDE);

				if (*phCurrent != hDebug)  apiShowWindow(hDebug,  SW_HIDE);
				
				if (*phCurrent != hInfo)   apiShowWindow(hInfo,   SW_HIDE);

				SetFocus(*phCurrent);
			}
		}
		break;
	}

	return 0;
}

void CSettings::Dialog()
{
	SetCursor(LoadCursor(NULL,IDC_WAIT));

	// Сначала обновить DC, чтобы некрасивостей не было
	gpConEmu->UpdateWindowChild(NULL);

	//2009-05-03. DialogBox создает МОДАЛЬНЫЙ Диалог
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

	if (m_isKeyboardHooks == 1)
		gpConEmu->RegisterHoooks();
	else if (gpSet->m_isKeyboardHooks == 2)
		gpConEmu->UnRegisterHoooks();
}

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
			gpSet->OnInitDialog();
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
				gpSet->OnButtonClicked(wParam, lParam);
			}
			else if (HIWORD(wParam) == EN_CHANGE)
			{
				gpSet->OnEditChanged(wParam, lParam);
			}
			else if (HIWORD(wParam) == CBN_EDITCHANGE || HIWORD(wParam) == CBN_SELCHANGE)
			{
				gpSet->OnComboBox(wParam, lParam);
			}

			break;
		case WM_NOTIFY:
		{
			LPNMHDR phdr = (LPNMHDR)lParam;

			if (phdr->idFrom == tabMain)
				gpSet->OnTab(phdr);
		} break;
		case WM_CLOSE:
		{
			gpSet->OnClose();
			DestroyWindow(hWnd2);
		} break;
		case WM_DESTROY:
			gpSet->UnregisterTabs();

			if (gpSet->hwndTip) {DestroyWindow(gpSet->hwndTip); gpSet->hwndTip = NULL;}

			if (gpSet->hwndBalloon) {DestroyWindow(gpSet->hwndBalloon); gpSet->hwndBalloon = NULL;}

			if (gpSet->mh_CtlColorBrush) { DeleteObject(gpSet->mh_CtlColorBrush); gpSet->mh_CtlColorBrush = NULL; }

			//EndDialog(hWnd2, TRUE);
			ghOpWnd = NULL;
			gpSet->hMain = gpSet->hExt = gpSet->hKeys = gpSet->hTabs = gpSet->hColors = NULL;
			gpSet->hViews = gpSet->hInfo = gpSet->hDebug = NULL;
			gbLastColorsOk = FALSE;
			break;
		case WM_HOTKEY:

			if (wParam == 0x101)
			{
				// Переключиться на следующий таб
				int nCur = SendDlgItemMessage(ghOpWnd, tabMain, TCM_GETCURSEL,0,0);
				int nAll = SendDlgItemMessage(ghOpWnd, tabMain, TCM_GETITEMCOUNT,0,0);

				nCur ++; if (nCur>=nAll) nCur = 0;

				SendDlgItemMessage(ghOpWnd, tabMain, TCM_SETCURSEL,nCur,0);
				NMHDR hdr = {GetDlgItem(ghOpWnd, tabMain),tabMain,TCN_SELCHANGE};
				gpSet->OnTab(&hdr);
			}
			else if (wParam == 0x102)
			{
				// Переключиться на предыдущий таб
				int nCur = SendDlgItemMessage(ghOpWnd, tabMain, TCM_GETCURSEL,0,0);
				int nAll = SendDlgItemMessage(ghOpWnd, tabMain, TCM_GETITEMCOUNT,0,0);

				nCur --; if (nCur<0) nCur = nAll - 1;

				SendDlgItemMessage(ghOpWnd, tabMain, TCM_SETCURSEL,nCur,0);
				NMHDR hdr = {GetDlgItem(ghOpWnd, tabMain),tabMain,TCN_SELCHANGE};
				gpSet->OnTab(&hdr);
			}

		case WM_ACTIVATE:

			if (LOWORD(wParam) != 0)
				gpSet->RegisterTabs();
			else
				gpSet->UnregisterTabs();

			break;
		default:
			return 0;
	}

	return 0;
}

INT_PTR CSettings::OnMeasureFontItem(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	DWORD wID = wParam;

	if (wID == tFontFace || wID == tFontFace2 || wID == tThumbsFontName || wID == tTilesFontName)
	{
		MEASUREITEMSTRUCT *pItem = (MEASUREITEMSTRUCT*)lParam;
		pItem->itemHeight = 15; //pItem->itemHeight;
	}

	return TRUE;
}

INT_PTR CSettings::OnDrawFontItem(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	DWORD wID = wParam;

	if (wID == tFontFace || wID == tFontFace2 || wID == tThumbsFontName || wID == tTilesFontName)
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

INT_PTR CSettings::mainOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch(messg)
	{
		case WM_INITDIALOG:
			_ASSERTE(gpSet->hMain==NULL || gpSet->hMain==hWnd2);
			gpSet->hMain = hWnd2;
			gpSet->OnInitDialog_Main();
			break;
		case WM_MEASUREITEM:
			return gpSet->OnMeasureFontItem(hWnd2, messg, wParam, lParam);
		case WM_DRAWITEM:
			return gpSet->OnDrawFontItem(hWnd2, messg, wParam, lParam);
		case WM_HSCROLL:
		{
			if (gpSet->hMain && (HWND)lParam == GetDlgItem(gpSet->hMain, slDarker))
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
						gpSet->LoadBackgroundFile(gpSet->sBgImage);
					else
						gpSet->NeedBackgroundUpdate();

					gpConEmu->Update(true);
				}
			}
		}
		break;
		case WM_COMMAND:

			if (HIWORD(wParam) == BN_CLICKED)
			{
				gpSet->OnButtonClicked(wParam, lParam);
			}
			else if (HIWORD(wParam) == EN_CHANGE)
			{
				gpSet->OnEditChanged(wParam, lParam);
			}
			else if (HIWORD(wParam) == CBN_EDITCHANGE || HIWORD(wParam) == CBN_SELCHANGE)
			{
				gpSet->OnComboBox(wParam, lParam);
			}
			else if (HIWORD(wParam) == CBN_KILLFOCUS && gpSet->mn_LastChangingFontCtrlId && LOWORD(wParam) == gpSet->mn_LastChangingFontCtrlId)
			{
				PostMessage(gpSet->hMain, gpSet->mn_MsgRecreateFont, gpSet->mn_LastChangingFontCtrlId, 0);
				gpSet->mn_LastChangingFontCtrlId = 0;
			}

			break;
		case WM_TIMER:

			if (wParam == FAILED_FONT_TIMERID)
			{
				KillTimer(gpSet->hMain, FAILED_FONT_TIMERID);
				SendMessage(gpSet->hwndBalloon, TTM_TRACKACTIVATE, FALSE, (LPARAM)&gpSet->tiBalloon);
				SendMessage(gpSet->hwndTip, TTM_ACTIVATE, TRUE, 0);
			}

		default:

			if (messg == gpSet->mn_MsgRecreateFont)
			{
				gpSet->RecreateFont(wParam);
			}

			return 0;
	}

	return 0;
}

INT_PTR CSettings::extOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch(messg)
	{
		case WM_INITDIALOG:
			_ASSERTE(gpSet->hExt==NULL || gpSet->hExt==hWnd2);
			gpSet->hExt = hWnd2;
			gpSet->OnInitDialog_Ext();
			break;
		case WM_COMMAND:

			if (HIWORD(wParam) == BN_CLICKED)
			{
				gpSet->OnButtonClicked(wParam, lParam);
			}
			else if (HIWORD(wParam) == EN_CHANGE)
			{
				gpSet->OnEditChanged(wParam, lParam);
			}
			else if (HIWORD(wParam) == CBN_EDITCHANGE || HIWORD(wParam) == CBN_SELCHANGE)
			{
				gpSet->OnComboBox(wParam, lParam);
			}

			break;
		default:
			return 0;
	}

	return 0;
}

INT_PTR CSettings::keysOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	static bool bSkipSelChange = false;
	switch (messg)
	{
		case WM_INITDIALOG:
			_ASSERTE(gpSet->hKeys==NULL || gpSet->hKeys==hWnd2);
			gpSet->hKeys = hWnd2;
			bSkipSelChange = true;
			gpSet->OnInitDialog_Keys();
			bSkipSelChange = false;
			break;
		case WM_COMMAND:

			if (HIWORD(wParam) == BN_CLICKED)
			{
				gpSet->OnButtonClicked(wParam, lParam);
			}
			else if (HIWORD(wParam) == EN_CHANGE)
			{
				gpSet->OnEditChanged(wParam, lParam);
			}
			else if (HIWORD(wParam) == CBN_EDITCHANGE || HIWORD(wParam) == CBN_SELCHANGE)
			{
				gpSet->OnComboBox(wParam, lParam);
			}

			break;
		default:
			if (messg == gpSet->mn_ActivateTabMsg)
			{
				gpSet->FillHotKeysList(); // перезаполнить кнопки
			}
			return 0;
	}

	return 0;
}

INT_PTR CSettings::tabsOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch(messg)
	{
		case WM_INITDIALOG:
			_ASSERTE(gpSet->hTabs==NULL || gpSet->hTabs==hWnd2);
			gpSet->hTabs = hWnd2;
			gpSet->OnInitDialog_Tabs();
			break;
		case WM_COMMAND:

			if (HIWORD(wParam) == BN_CLICKED)
			{
				gpSet->OnButtonClicked(wParam, lParam);
			}
			else if (HIWORD(wParam) == EN_CHANGE)
			{
				gpSet->OnEditChanged(wParam, lParam);
			}
			else if (HIWORD(wParam) == CBN_EDITCHANGE || HIWORD(wParam) == CBN_SELCHANGE)
			{
				gpSet->OnComboBox(wParam, lParam);
			}

			break;
		default:
			return 0;
	}

	return 0;
}

INT_PTR CSettings::colorOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch(messg)
	{
		case WM_INITDIALOG:
			_ASSERTE(gpSet->hColors==NULL || gpSet->hColors==hWnd2);
			gpSet->hColors = hWnd2;
			gpSet->OnInitDialog_Color();
			break;
		case WM_CTLCOLORSTATIC:
		{
			WORD wID = GetDlgCtrlID((HWND)lParam);

			if (wID >= c0 && wID <= MAX_COLOR_EDT_ID)
				return gpSet->ColorCtlStatic(hWnd2, wID, (HWND)lParam);

			return 0;
		}
		case WM_COMMAND:

			if (HIWORD(wParam) == BN_CLICKED)
			{
				gpSet->OnColorButtonClicked(wParam, lParam);
			}
			else if (HIWORD(wParam) == EN_CHANGE)
			{
				gpSet->OnColorEditChanged(wParam, lParam);
			}
			else if (HIWORD(wParam) == CBN_EDITCHANGE || HIWORD(wParam) == CBN_SELCHANGE)
			{
				gpSet->OnColorComboBox(wParam, lParam);
			}

			break;
		case WM_HSCROLL:
		{
			if (gpSet->hColors && (HWND)lParam == GetDlgItem(gpSet->hColors, slTransparent))
			{
				int newV = SendDlgItemMessage(hWnd2, slTransparent, TBM_GETPOS, 0, 0);

				if (newV != gpSet->nTransparent)
				{
					CheckDlgButton(gpSet->hColors, cbTransparent, (newV!=255) ? BST_CHECKED : BST_UNCHECKED);
					gpSet->nTransparent = newV;
					gpConEmu->OnTransparent();
				}
			}
		}
		break;
		default:
			return 0;
	}

	return 0;
}

INT_PTR CSettings::viewsOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch(messg)
	{
		case WM_INITDIALOG:
			_ASSERTE(gpSet->hViews==NULL || gpSet->hViews==hWnd2);
			gpSet->hViews = hWnd2;
			gpSet->OnInitDialog_Views();
			break;
		case WM_MEASUREITEM:
			return gpSet->OnMeasureFontItem(hWnd2, messg, wParam, lParam);
		case WM_DRAWITEM:
			return gpSet->OnDrawFontItem(hWnd2, messg, wParam, lParam);
		case WM_CTLCOLORSTATIC:
		{
			WORD wID = GetDlgCtrlID((HWND)lParam);

			if (wID >= c32 && wID <= c34)
				return gpSet->ColorCtlStatic(hWnd2, wID, (HWND)lParam);

			return 0;
		}
		case WM_COMMAND:
		{
			WORD wId = LOWORD(wParam);

			if (HIWORD(wParam) == BN_CLICKED)
			{
				switch(wId)
				{
					case bApplyViewSettings:
						gpConEmu->OnPanelViewSettingsChanged();
						//gpConEmu->UpdateGuiInfoMapping();
						break;
					case cbThumbLoadFiles:

						switch(IsChecked(hWnd2,wId))
						{
							case BST_CHECKED:       gpSet->ThSet.bLoadPreviews = 3; break;
							case BST_INDETERMINATE: gpSet->ThSet.bLoadPreviews = 1; break;
							default: gpSet->ThSet.bLoadPreviews = 0;
						}

						break;
					case cbThumbLoadFolders:
						gpSet->ThSet.bLoadFolders = IsChecked(hWnd2, wId);
						break;
					case cbThumbUsePicView2:
						gpSet->ThSet.bUsePicView2 = IsChecked(hWnd2, wId);
						break;
					case cbThumbRestoreOnStartup:
						gpSet->ThSet.bRestoreOnStartup = IsChecked(hWnd2, wId);
						break;
					case cbThumbPreviewBox:
						gpSet->ThSet.nPreviewFrame = IsChecked(hWnd2, wId);
						break;
					case cbThumbSelectionBox:
						gpSet->ThSet.nSelectFrame = IsChecked(hWnd2, wId);
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
					default:

						if (wId >= c32 && wId <= c34)
						{
							if (gpSet->ColorEditDialog(hWnd2, wId))
							{
								if (wId == c32)
								{
									gpSet->ThSet.crBackground.UseIndex = 0;
									CheckRadioButton(hWnd2, rbThumbBackColorIdx, rbThumbBackColorRGB, rbThumbBackColorRGB);
								}
								else if (wId == c33)
								{
									gpSet->ThSet.crPreviewFrame.UseIndex = 0;
									CheckRadioButton(hWnd2, rbThumbPreviewBoxColorIdx, rbThumbPreviewBoxColorRGB, rbThumbPreviewBoxColorRGB);
								}
								else if (wId == c34)
								{
									gpSet->ThSet.crSelectFrame.UseIndex = 0;
									CheckRadioButton(hWnd2, rbThumbSelectionBoxColorIdx, rbThumbSelectionBoxColorRGB, rbThumbSelectionBoxColorRGB);
								}

								InvalidateRect(GetDlgItem(hWnd2, wId), 0, 1);
								// done
							}
						}
				}
			}
			else if (HIWORD(wParam) == EN_CHANGE)
			{
				BOOL bValOk = FALSE;
				UINT nVal = GetDlgItemInt(hWnd2, wId, &bValOk, FALSE);

				if (bValOk)
				{
					switch(wId)
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

				if (wId >= tc32 && wId <= tc34)
				{
					COLORREF color = 0;

					if (gpSet->GetColorById(wId - (tc0-c0), &color))
					{
						if (gpSet->GetColorRef(hWnd2, wId, &color))
						{
							if (gpSet->SetColorById(wId - (tc0-c0), color))
							{
								InvalidateRect(GetDlgItem(hWnd2, wId - (tc0-c0)), 0, 1);
								// done
							}
						}
					}
				}
			}
			else if (HIWORD(wParam) == CBN_EDITCHANGE)
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

						if (nSel>=0 && nSel<countof(Settings::FSizesSmall))
							gpSet->ThSet.Thumbs.nFontHeight = Settings::FSizesSmall[nSel];

						break;
					case tTilesFontName:
						SendDlgItemMessage(hWnd2, wId, CB_GETLBTEXT, nSel, (LPARAM)gpSet->ThSet.Tiles.sFontName);
						break;
					case tTilesFontSize:

						if (nSel>=0 && nSel<countof(Settings::FSizesSmall))
							gpSet->ThSet.Tiles.nFontHeight = Settings::FSizesSmall[nSel];

						break;
					case tThumbMaxZoom:
						gpSet->ThSet.nMaxZoom = max(100,((nSel+1)*100));
				}
			}
		}
		break;
		default:

			if (messg == gpSet->mn_MsgLoadFontFromMain)
			{
				gpSet->OnInitDialog_ViewsFonts();
			}

			return 0;
	}

	return 0;
}

INT_PTR CSettings::infoOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch(messg)
	{
		case WM_INITDIALOG:
			_ASSERTE(gpSet->hInfo==NULL || gpSet->hInfo==hWnd2);
			gpSet->hInfo = hWnd2;
			gpSet->OnInitDialog_Info();
			break;
		default:

			if (messg == gpSet->mn_MsgUpdateCounter)
			{
				wchar_t sTemp[64];

				if (gpSet->mn_Freq!=0)
				{
					i64 v = 0;

					if (wParam == (tPerfFPS-tPerfFPS) || wParam == (tPerfInterval-tPerfFPS))
					{
						i64 *pFPS = NULL;
						UINT nCount = 0;

						if (wParam == (tPerfFPS-tPerfFPS))
						{
							pFPS = gpSet->mn_FPS; nCount = countof(gpSet->mn_FPS);
						}
						else
						{
							pFPS = gpSet->mn_RFPS; nCount = countof(gpSet->mn_RFPS);
						}

						i64 tmin = 0, tmax = 0;
						tmin = pFPS[0];

						for(UINT i = 0; i < nCount; i++)
						{
							if (pFPS[i] < tmin) tmin = pFPS[i]; //-V108

							if (pFPS[i] > tmax) tmax = pFPS[i]; //-V108
						}

						if (tmax > tmin)
							v = ((__int64)200)* gpSet->mn_Freq / (tmax - tmin);
					}
					else
					{
						v = (10000*(__int64)gpSet->mn_CounterMax[wParam])/gpSet->mn_Freq;
					}

					// WinApi не умеет float/double
					_wsprintf(sTemp, SKIPLEN(countof(sTemp)) L"%u.%u", (int)(v/10), (int)(v%10));
					SetDlgItemText(gpSet->hInfo, wParam+tPerfFPS, sTemp); //-V107
				}

				return TRUE;
			}

			return 0;
	}

	return 0;
}

INT_PTR CSettings::debugOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	static bool bSkipSelChange = false;
	switch(messg)
	{
		case WM_INITDIALOG:
			_ASSERTE(gpSet->hDebug==NULL || gpSet->hDebug==hWnd2);
			gpSet->hDebug = hWnd2;
			gpSet->OnInitDialog_Debug();
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
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
					gpSet->OnSaveActivityLogFile(GetDlgItem(hWnd2, lbActivityLog));
				} // cbActivitySaveAs
				break;
			case rbActivityDisabled:
			case rbActivityShell:
			case rbActivityInput:
				{
					//PRAGMA_ERROR("m_RealConLoggingType");
					HWND hList = GetDlgItem(hWnd2, lbActivityLog);
					//HWND hDetails = GetDlgItem(hWnd2, lbActivityDetails);
					gpSet->m_RealConLoggingType =
						(LOWORD(wParam) == rbActivityShell) ? glt_Processes :
						(LOWORD(wParam) == rbActivityInput) ? glt_Input :
						glt_None;
					ListView_DeleteAllItems(hList);
					for (int c = 0; (c <= 40) && ListView_DeleteColumn(hList, 0); c++);
					//ListView_DeleteAllItems(hDetails);
					//for (int c = 0; (c <= 40) && ListView_DeleteColumn(hDetails, 0); c++);
					SetDlgItemText(hWnd2, ebActivityApp, L"");
					SetDlgItemText(hWnd2, ebActivityParm, L"");
					
					if (gpSet->m_RealConLoggingType == glt_Processes)
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

						//col.cx = 60;
						//wcscpy_c(szTitle, L"Item");		ListView_InsertColumn(hDetails, 0, &col);
						//col.cx = 380;
						//wcscpy_c(szTitle, L"Details");	ListView_InsertColumn(hDetails, 1, &col);
						//LVITEM lvi = {LVIF_TEXT|LVIF_STATE};
						//lvi.state = lvi.stateMask = LVIS_SELECTED|LVIS_FOCUSED; lvi.pszText = szTitle;
						//wcscpy_c(szTitle, L"AppName");
						//ListView_InsertItem(hDetails, &lvi); lvi.iItem++;
						//wcscpy_c(szTitle, L"CmdLine");
						//ListView_InsertItem(hDetails, &lvi); lvi.iItem++;
						////wcscpy_c(szTime, L"Details");
						////ListView_InsertItem(hDetails, &lvi);

					}
					else if (gpSet->m_RealConLoggingType == glt_Input)
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
			} // WM_COMMAND
			break;
		case WM_NOTIFY:
			{
				if (((NMHDR*)lParam)->idFrom == lbActivityLog && !bSkipSelChange)
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
								if (gpSet->m_RealConLoggingType == glt_Processes)
								{
									ListView_GetItemText(hList, p->iItem, lpc_App, szText, countof(szText));
									SetDlgItemText(hWnd2, ebActivityApp, szText);
									ListView_GetItemText(hList, p->iItem, lpc_Params, szText, countof(szText));
									SetDlgItemText(hWnd2, ebActivityParm, szText);
								}
								else if (gpSet->m_RealConLoggingType == glt_Input)
								{
									ListView_GetItemText(hList, p->iItem, lic_Type, szText, countof(szText));
									wcscat_c(szText, L" - ");
									int nLen = _tcslen(szText);
									ListView_GetItemText(hList, p->iItem, lic_Dup, szText+nLen, countof(szText)-nLen);
									SetDlgItemText(hWnd2, ebActivityApp, szText);
									ListView_GetItemText(hList, p->iItem, lic_Event, szText, countof(szText));
									SetDlgItemText(hWnd2, ebActivityParm, szText);
								}
								else
								{
									SetDlgItemText(hWnd2, ebActivityApp, L"");
									SetDlgItemText(hWnd2, ebActivityParm, L"");
								}
							}
						} //LVN_ODSTATECHANGED
						break;
					}
				}
				return 0;
			} // WM_NOTIFY
			break;
		case DBGMSG_LOG_ID:
			if (wParam == DBGMSG_LOG_SHELL_MAGIC)
			{
				bSkipSelChange = true;
				DebugLogShellActivity *pShl = (DebugLogShellActivity*)lParam;
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
				if (pShl->pszFile)
					ListView_SetItemText(hList, nItem, lpc_App, (wchar_t*)pShl->pszFile);
				SetDlgItemText(hWnd2, ebActivityApp, (wchar_t*)(pShl->pszFile ? pShl->pszFile : L""));
				//ListView_SetItemText(hDetails, 0, 1, (wchar_t*)(pShl->pszFile ? pShl->pszFile : L""));
				if (pShl->pszParam)
					ListView_SetItemText(hList, nItem, lpc_Params, (wchar_t*)pShl->pszParam);
				SetDlgItemText(hWnd2, ebActivityParm, (wchar_t*)(pShl->pszParam ? pShl->pszParam : L""));
				//ListView_SetItemText(hDetails, 1, 1, (wchar_t*)(pShl->pszParam ? pShl->pszParam : L""));
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
				bSkipSelChange = false;
			} // DBGMSG_LOG_SHELL_MAGIC
			else if (wParam == DBGMSG_LOG_INPUT_MAGIC)
			{
				bSkipSelChange = true;
				CESERVER_REQ_PEEKREADINFO* pInfo = (CESERVER_REQ_PEEKREADINFO*)lParam;
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
					ListView_SetItemText(hList, nItem, lic_Dup, L"1");
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
				bSkipSelChange = false;
			}
			break; // DBGMSG_LOG_ID

		default:
			return 0;
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
	LVITEM lvi = {0};
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

bool CSettings::isKeyboardHooks()
{
//	#ifndef _DEBUG
	// Для WinXP это не было нужно
	if (gOSVer.dwMajorVersion < 6)
	{
		return false;
	}

//	#endif

	if (m_isKeyboardHooks == 0)
	{
		// Вопрос пользователю еще не задавали (это на старте, окно еще и не создано)
		int nBtn = MessageBox(NULL,
		                      L"Do You want to use Win-Number combination for \n"
		                      L"switching between consoles (Multi Console feature)? \n\n"
		                      L"If You choose 'Yes' - ConEmu will install keyboard hook. \n"
		                      L"So, You must allow that in antiviral software (such as AVP). \n\n"
		                      L"You can change behavior later via Settings->Features->\n"
		                      L"'Install keyboard hooks (Vista & Win7)' check box, or\n"
		                      L"'KeyboardHooks' value in ConEmu settings (registry or xml)."
		                      , gpConEmu->ms_ConEmuVer, MB_YESNOCANCEL|MB_ICONQUESTION);

		if (nBtn == IDCANCEL)
		{
			m_isKeyboardHooks = 2; // NO
		}
		else
		{
			m_isKeyboardHooks = (nBtn == IDYES) ? 1 : 2;
			SettingsBase* reg = CreateSettings();

			if (!reg)
			{
				_ASSERTE(reg!=NULL);
			}
			else
			{
				if (reg->OpenKey(ConfigPath, KEY_WRITE))
				{
					reg->Save(L"KeyboardHooks", m_isKeyboardHooks);
					reg->CloseKey();
				}

				delete reg;
			}
		}
	}

	return (m_isKeyboardHooks == 1);
}

bool CSettings::IsHostkey(WORD vk)
{
	for(int i=0; i < 15 && mn_HostModOk[i]; i++)
		if (mn_HostModOk[i] == vk)
			return true;

	return false;
}

// Если есть vk - заменить на vkNew
void CSettings::ReplaceHostkey(BYTE vk, BYTE vkNew)
{
	for(int i = 0; i < 15; i++)
	{
		if (gpSet->mn_HostModOk[i] == vk)
		{
			gpSet->mn_HostModOk[i] = vkNew;
			return;
		}
	}
}

void CSettings::AddHostkey(BYTE vk)
{
	for(int i = 0; i < 15; i++)
	{
		if (gpSet->mn_HostModOk[i] == vk)
			break; // уже есть

		if (!gpSet->mn_HostModOk[i])
		{
			gpSet->mn_HostModOk[i] = vk; // добавить
			break;
		}
	}
}

BYTE CSettings::CheckHostkeyModifier(BYTE vk)
{
	// Если передан VK_NULL
	if (!vk)
		return 0;

	switch(vk)
	{
		case VK_LWIN: case VK_RWIN:

			if (gpSet->IsHostkey(VK_RWIN))
				ReplaceHostkey(VK_RWIN, VK_LWIN);

			vk = VK_LWIN; // Сохраняем только Левый-Win
			break;
		case VK_APPS:
			break; // Это - ок
		case VK_LSHIFT:

			if (gpSet->IsHostkey(VK_RSHIFT) || gpSet->IsHostkey(VK_SHIFT))
			{
				vk = VK_SHIFT;
				ReplaceHostkey(VK_RSHIFT, VK_SHIFT);
			}

			break;
		case VK_RSHIFT:

			if (gpSet->IsHostkey(VK_LSHIFT) || gpSet->IsHostkey(VK_SHIFT))
			{
				vk = VK_SHIFT;
				ReplaceHostkey(VK_LSHIFT, VK_SHIFT);
			}

			break;
		case VK_SHIFT:

			if (gpSet->IsHostkey(VK_LSHIFT))
				ReplaceHostkey(VK_LSHIFT, VK_SHIFT);
			else if (gpSet->IsHostkey(VK_RSHIFT))
				ReplaceHostkey(VK_RSHIFT, VK_SHIFT);

			break;
		case VK_LMENU:

			if (gpSet->IsHostkey(VK_RMENU) || gpSet->IsHostkey(VK_MENU))
			{
				vk = VK_MENU;
				ReplaceHostkey(VK_RMENU, VK_MENU);
			}

			break;
		case VK_RMENU:

			if (gpSet->IsHostkey(VK_LMENU) || gpSet->IsHostkey(VK_MENU))
			{
				vk = VK_MENU;
				ReplaceHostkey(VK_LMENU, VK_MENU);
			}

			break;
		case VK_MENU:

			if (gpSet->IsHostkey(VK_LMENU))
				ReplaceHostkey(VK_LMENU, VK_MENU);
			else if (gpSet->IsHostkey(VK_RMENU))
				ReplaceHostkey(VK_RMENU, VK_MENU);

			break;
		case VK_LCONTROL:

			if (gpSet->IsHostkey(VK_RCONTROL) || gpSet->IsHostkey(VK_CONTROL))
			{
				vk = VK_CONTROL;
				ReplaceHostkey(VK_RCONTROL, VK_CONTROL);
			}

			break;
		case VK_RCONTROL:

			if (gpSet->IsHostkey(VK_LCONTROL) || gpSet->IsHostkey(VK_CONTROL))
			{
				vk = VK_CONTROL;
				ReplaceHostkey(VK_LCONTROL, VK_CONTROL);
			}

			break;
		case VK_CONTROL:

			if (gpSet->IsHostkey(VK_LCONTROL))
				ReplaceHostkey(VK_LCONTROL, VK_CONTROL);
			else if (gpSet->IsHostkey(VK_RCONTROL))
				ReplaceHostkey(VK_RCONTROL, VK_CONTROL);

			break;
	}

	// Добавить в список входящих в Host
	AddHostkey(vk);
	// Вернуть (возможно измененный) VK
	return vk;
}

bool CSettings::TestHostkeyModifiers()
{
	memset(mn_HostModOk, 0, sizeof(mn_HostModOk));
	memset(mn_HostModSkip, 0, sizeof(mn_HostModSkip));

	if (!nMultiHotkeyModifier)
		nMultiHotkeyModifier = VK_LWIN;

	BYTE vk;
	vk = (nMultiHotkeyModifier & 0xFF);
	CheckHostkeyModifier(vk);
	vk = (nMultiHotkeyModifier & 0xFF00) >> 8;
	CheckHostkeyModifier(vk);
	vk = (nMultiHotkeyModifier & 0xFF0000) >> 16;
	CheckHostkeyModifier(vk);
	vk = (nMultiHotkeyModifier & 0xFF000000) >> 24;
	CheckHostkeyModifier(vk);
	// Однако, допустимо не более 3-х клавиш (больше смысла не имеет)
	TrimHostkeys();
	// Сформировать (возможно скорректированную) маску HostKey
	bool lbChanged = MakeHostkeyModifier();
	return lbChanged;
}

bool CSettings::MakeHostkeyModifier()
{
	bool lbChanged = false;
	// Сформировать (возможно скорректированную) маску HostKey
	DWORD nNew = 0;

	if (gpSet->mn_HostModOk[0])
		nNew |= gpSet->mn_HostModOk[0];

	if (gpSet->mn_HostModOk[1])
		nNew |= ((DWORD)(gpSet->mn_HostModOk[1])) << 8;

	if (gpSet->mn_HostModOk[2])
		nNew |= ((DWORD)(gpSet->mn_HostModOk[2])) << 16;

	if (gpSet->mn_HostModOk[3])
		nNew |= ((DWORD)(gpSet->mn_HostModOk[3])) << 24;

	TODO("!!! Добавить в mn_HostModSkip те VK, которые отсутствуют в mn_HostModOk");

	if (gpSet->nMultiHotkeyModifier != nNew)
	{
		gpSet->nMultiHotkeyModifier = nNew;
		lbChanged = true;
	}

	return lbChanged;
}

// Оставить в mn_HostModOk только 3 VK
void CSettings::TrimHostkeys()
{
	if (gpSet->mn_HostModOk[0] == 0)
		return;

	int i = 0;

	while (++i < 15 && gpSet->mn_HostModOk[i])
		;

	// Если вдруг задали более 3-х модификаторов - обрезать, оставив 3 последних
	if (i > 3)
	{
		if (i >= countof(gpSet->mn_HostModOk))
		{
			_ASSERTE(i < countof(gpSet->mn_HostModOk));
			i = countof(gpSet->mn_HostModOk) - 1;
		}
		memmove(gpSet->mn_HostModOk, gpSet->mn_HostModOk+i-3, 3); //-V112
	}

	memset(gpSet->mn_HostModOk+3, 0, sizeof(gpSet->mn_HostModOk)-3);
}

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

bool CSettings::isHostkeySingleLR(WORD vk, WORD vkC, WORD vkL, WORD vkR)
{
	if (vk == vkC)
	{
		bool bLeft  = isPressed(vkL);
		bool bRight = isPressed(vkR);

		if (bLeft && !bRight)
			return (nMultiHotkeyModifier == vkL);

		if (bRight && !bLeft)
			return (nMultiHotkeyModifier == vkR);

		// нажатие обоих шифтов - игнорируем
		return false;
	}

	if (vk == vkL)
		return (nMultiHotkeyModifier == vkL);

	if (vk == vkR)
		return (nMultiHotkeyModifier == vkR);

	return false;
}

bool CSettings::IsHostkeySingle(WORD vk)
{
	if (nMultiHotkeyModifier > 0xFF)
		return false; // в Host-комбинации больше одной клавиши

	if (vk == VK_LWIN || vk == VK_RWIN)
		return (nMultiHotkeyModifier == VK_LWIN);

	if (vk == VK_APPS)
		return (nMultiHotkeyModifier == VK_APPS);

	if (vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT)
		return isHostkeySingleLR(vk, VK_SHIFT, VK_LSHIFT, VK_RSHIFT);

	if (vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL)
		return isHostkeySingleLR(vk, VK_CONTROL, VK_LCONTROL, VK_RCONTROL);

	if (vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU)
		return isHostkeySingleLR(vk, VK_MENU, VK_LMENU, VK_RMENU);

	return false;
}

// набор флагов MOD_xxx для RegisterHotKey
UINT CSettings::GetHostKeyMod()
{
	UINT nMOD = 0;

	for(int i=0; i < 15 && mn_HostModOk[i]; i++)
	{
		switch(mn_HostModOk[i])
		{
			case VK_LWIN: case VK_RWIN:
				nMOD |= MOD_WIN;
				break;
			case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
				nMOD |= MOD_CONTROL;
				break;
			case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:
				nMOD |= MOD_SHIFT;
				break;
			case VK_MENU: case VK_LMENU: case VK_RMENU:
				nMOD |= MOD_ALT;
				break;
		}
	}

	if (!nMOD)
		nMOD = MOD_WIN;

	return nMOD;
}

WORD CSettings::GetPressedHostkey()
{
	_ASSERTE(mn_HostModOk[0]!=0);

	if (mn_HostModOk[0] == VK_LWIN)
	{
		if (isPressed(VK_LWIN))
			return VK_LWIN;

		if (isPressed(VK_RWIN))
			return VK_RWIN;
	}

	if (!isPressed(mn_HostModOk[0]))
	{
		_ASSERT(FALSE);
		return 0;
	}

	// Для правых-левых - возвращаем общий, т.к. именно он приходит в WM_KEYUP
	if (mn_HostModOk[0] == VK_LSHIFT || mn_HostModOk[0] == VK_RSHIFT)
		return VK_SHIFT;

	if (mn_HostModOk[0] == VK_LMENU || mn_HostModOk[0] == VK_RMENU)
		return VK_MENU;

	if (mn_HostModOk[0] == VK_LCONTROL || mn_HostModOk[0] == VK_RCONTROL)
		return VK_CONTROL;

	return mn_HostModOk[0];
}

bool CSettings::IsHostkeyPressed()
{
	if (mn_HostModOk[0] == 0)
	{
		_ASSERTE(mn_HostModOk[0]!=0);
		mn_HostModOk[0] = VK_LWIN;
		return isPressed(VK_LWIN) || isPressed(VK_RWIN);
	}

	// Не более 3-х модификаторов + кнопка
	_ASSERTE(mn_HostModOk[4] == 0);
	for(int i = 0; i < 4 && mn_HostModOk[i]; i++) //-V112
	{
		if (mn_HostModOk[i] == VK_LWIN)
		{
			if (!(isPressed(VK_LWIN) || isPressed(VK_RWIN)))
				return false;
		}
		else if (!isPressed(mn_HostModOk[i]))
		{
			return false;
		}
	}

	// Не более 3-х модификаторов + кнопка
	for(int j = 0; j < 4 && mn_HostModSkip[j]; j++) //-V112
	{
		if (isPressed(mn_HostModSkip[j]))
			return false;
	}

	return true;
}

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
//			gpSet->RegisterTipsFor(hWnd2);
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

INT_PTR CSettings::hideOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch(messg)
	{
		case WM_INITDIALOG:
		{
			SendMessage(hWnd2, WM_SETICON, ICON_BIG, (LPARAM)hClassIcon);
			SendMessage(hWnd2, WM_SETICON, ICON_SMALL, (LPARAM)hClassIconSm);

			SetDlgItemInt(hWnd2, tHideCaptionAlwaysFrame, gpSet->nHideCaptionAlwaysFrame, FALSE);
			SetDlgItemInt(hWnd2, tHideCaptionAlwaysDelay, gpSet->nHideCaptionAlwaysDelay, FALSE);
			SetDlgItemInt(hWnd2, tHideCaptionAlwaysDissapear, gpSet->nHideCaptionAlwaysDisappear, FALSE);
			gpSet->RegisterTipsFor(hWnd2);
			CenterMoreDlg(hWnd2);
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

				if (TB == IDOK || TB == IDCANCEL)
					EndDialog(hWnd2, TB);
			}
			else if (HIWORD(wParam) == EN_CHANGE)
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

			break;
	}

	return 0;
}

INT_PTR CSettings::selectionOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch(messg)
	{
		case WM_INITDIALOG:
		{
			SendMessage(hWnd2, WM_SETICON, ICON_BIG, (LPARAM)hClassIcon);
			SendMessage(hWnd2, WM_SETICON, ICON_SMALL, (LPARAM)hClassIconSm);

			//SetDlgItemInt (hWnd2, tHideCaptionAlwaysFrame, gpSet->nHideCaptionAlwaysFrame, FALSE);
			//SetDlgItemInt (hWnd2, tHideCaptionAlwaysDelay, gpSet->nHideCaptionAlwaysDelay, FALSE);
			//SetDlgItemInt (hWnd2, tHideCaptionAlwaysDissapear, gpSet->nHideCaptionAlwaysDisappear, FALSE);
			if (!gpSet->isConsoleTextSelection) gpSet->isConsoleTextSelection = 2;

			CheckDlgButton(hWnd2, (gpSet->isConsoleTextSelection==1)?rbCTSAlways:rbCTSBufferOnly, BST_CHECKED);
			CheckDlgButton(hWnd2, cbCTSBlockSelection, gpSet->isCTSSelectBlock);
			FillListBoxByte(hWnd2, lbCTSBlockSelection, Settings::szKeys, Settings::nKeys, gpSet->isCTSVkBlock);
			CheckDlgButton(hWnd2, cbCTSTextSelection, gpSet->isCTSSelectText);
			FillListBoxByte(hWnd2, lbCTSTextSelection, Settings::szKeys, Settings::nKeys, gpSet->isCTSVkText);
			CheckDlgButton(hWnd2, (gpSet->isCTSActMode==1)?rbCTSActAlways:rbCTSActBufferOnly, BST_CHECKED);
			FillListBoxByte(hWnd2, lbCTSActAlways, Settings::szKeysAct, Settings::nKeysAct, gpSet->isCTSVkAct);
			FillListBoxByte(hWnd2, lbCTSRBtnAction, Settings::szClipAct, Settings::nClipAct, gpSet->isCTSRBtnAction);
			FillListBoxByte(hWnd2, lbCTSMBtnAction, Settings::szClipAct, Settings::nClipAct, gpSet->isCTSMBtnAction);
			DWORD idxBack = (gpSet->isCTSColorIndex & 0xF0) >> 4;
			DWORD idxFore = (gpSet->isCTSColorIndex & 0xF);
			FillListBoxItems(GetDlgItem(hWnd2, lbCTSForeIdx), countof(Settings::szColorIdx)-1,
			                 Settings::szColorIdx, Settings::nColorIdx, idxFore);
			FillListBoxItems(GetDlgItem(hWnd2, lbCTSBackIdx), countof(Settings::szColorIdx)-1,
			                 Settings::szColorIdx, Settings::nColorIdx, idxBack);
			// Не совсем выделение, но в том же диалоге
			CheckDlgButton(hWnd2, cbFarGotoEditor, gpSet->isFarGotoEditor);
			FillListBoxByte(hWnd2, lbFarGotoEditorVk, Settings::szKeysAct, Settings::nKeysAct, gpSet->isFarGotoEditorVk);
			// тултипы
			gpSet->RegisterTipsFor(hWnd2);
			CenterMoreDlg(hWnd2);
		}
		break;
		//case WM_GETICON:

		//	if (wParam!=ICON_BIG)
		//	{
		//		SetWindowLongPtr(hWnd2, DWLP_MSGRESULT, (LRESULT)hClassIconSm);
		//		return 1;
		//	}

		//	return 0;
		case WM_COMMAND:

			if (HIWORD(wParam) == BN_CLICKED)
			{
				WORD CB = LOWORD(wParam);

				switch(CB)
				{
					case IDOK: case IDCANCEL:
						EndDialog(hWnd2, CB);
						break;
					case rbCTSAlways: case rbCTSBufferOnly:
						gpSet->isConsoleTextSelection = (CB==rbCTSAlways) ? 1 : 2;
						CheckDlgButton(gpSet->hExt, cbConsoleTextSelection, gpSet->isConsoleTextSelection);
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
				}
			}
			else if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				WORD CB = LOWORD(wParam);

				switch(CB)
				{
					case lbCTSBlockSelection:
					{
						GetListBoxByte(hWnd2, lbCTSBlockSelection, Settings::szKeys, Settings::nKeys, gpSet->isCTSVkBlock);
					} break;
					case lbCTSTextSelection:
					{
						GetListBoxByte(hWnd2, lbCTSTextSelection, Settings::szKeys, Settings::nKeys, gpSet->isCTSVkText);
					} break;
					case lbCTSActAlways:
					{
						GetListBoxByte(hWnd2, lbCTSActAlways, Settings::szKeysAct, Settings::nKeysAct, gpSet->isCTSVkAct);
					} break;
					case lbCTSRBtnAction:
					{
						GetListBoxByte(hWnd2, lbCTSRBtnAction, Settings::szClipAct, Settings::nClipAct, gpSet->isCTSRBtnAction);
					} break;
					case lbCTSMBtnAction:
					{
						GetListBoxByte(hWnd2, lbCTSMBtnAction, Settings::szClipAct, Settings::nClipAct, gpSet->isCTSMBtnAction);
					} break;
					case lbCTSForeIdx:
					{
						DWORD nFore = 0;
						GetListBoxItem(GetDlgItem(hWnd2, lbCTSForeIdx), countof(Settings::szColorIdx)-1,
						               Settings::szColorIdx, Settings::nColorIdx, nFore);
						gpSet->isCTSColorIndex = (gpSet->isCTSColorIndex & 0xF0) | (nFore & 0xF);
						gpConEmu->Update(true);
					} break;
					case lbCTSBackIdx:
					{
						DWORD nBack = 0;
						GetListBoxItem(GetDlgItem(hWnd2, lbCTSBackIdx), countof(Settings::szColorIdx)-1,
						               Settings::szColorIdx, Settings::nColorIdx, nBack);
						gpSet->isCTSColorIndex = (gpSet->isCTSColorIndex & 0xF) | ((nBack & 0xF) << 4);
						gpConEmu->Update(true);
					} break;
					case lbFarGotoEditorVk:
					{
						GetListBoxByte(hWnd2, lbFarGotoEditorVk, Settings::szKeysAct, Settings::nKeysAct, gpSet->isFarGotoEditorVk);
					} break;
				}
			}

			break;
	}

	return 0;
}

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

	if (w!=wndWidth || h!=wndHeight)
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
		isMonospace = bNewTTF ? 0 : isMonospaceSelected;

		if (hMain)
			CheckDlgButton(hMain, cbMonospace, isMonospace); // 3state
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
				SendMessage(gpSet->hInfo, mn_MsgUpdateCounter, nID-tPerfFPS, 0);
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
			PostMessage(gpSet->hInfo, mn_MsgUpdateCounter, nID-tPerfFPS, 0);

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
			PostMessage(gpSet->hInfo, mn_MsgUpdateCounter, nID-tPerfFPS, 0);

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
			PostMessage(gpSet->hInfo, mn_MsgUpdateCounter, nID-tPerfFPS, 0);
		}
	}
}

void CSettings::RegisterTipsFor(HWND hChildDlg)
{
	if (gpSet->hConFontDlg == hChildDlg)
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

			if (LoadString(g_hInstance, wID, szText, 0x200))
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
		char szInfo[128]; sprintf_s(szInfo, "MacroFontSetName('%s', H=%i, W=%i)", LF.lfFaceName, LF.lfHeight, LF.lfWidth);
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

		if (!isFullScreen && !gpConEmu->isZoomed())
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
		ShowFontErrorTip(gpSet->szFontError);
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
	LF.lfOutPrecision = OUT_TT_PRECIS;
	LF.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	LF.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
	GetDlgItemText(hMain, tFontFace, LF.lfFaceName, countof(LF.lfFaceName));
	LF.lfHeight = FontSizeY = GetNumber(hMain, tFontSizeY);
	LF.lfWidth = FontSizeX = GetNumber(hMain, tFontSizeX);
	LF.lfWeight = IsChecked(hMain, cbBold) ? FW_BOLD : FW_NORMAL;
	LF.lfItalic = IsChecked(hMain, cbItalic);
	LF.lfCharSet = mn_LoadFontCharSet;

	if (mb_CharSetWasSet)
	{
		//mb_CharSetWasSet = FALSE;
		INT_PTR newCharSet = SendDlgItemMessage(hMain, tFontCharset, CB_GETCURSEL, 0, 0);

		if (newCharSet != CB_ERR && newCharSet >= 0 && newCharSet < (INT_PTR)countof(chSetsNums))
			LF.lfCharSet = chSetsNums[newCharSet];
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
	FontSizeX2 = GetNumber(hMain, tFontSizeX2);
	FontSizeX3 = GetNumber(hMain, tFontSizeX3);

	if (isAdvLogging)
	{
		char szInfo[128]; sprintf_s(szInfo, "AutoRecreateFont(H=%i, W=%i)", LF.lfHeight, LF.lfWidth);
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

		if (!isFullScreen && !gpConEmu->isZoomed())
			gpConEmu->SyncWindowToConsole();
		else
			gpConEmu->SyncConsoleToWindow();

		gpConEmu->ReSize();
	}

	if (wFromID == tFontFace)
	{
		wchar_t szSize[10];
		_wsprintf(szSize, SKIPLEN(countof(szSize)) L"%i", LF.lfHeight);
		SetDlgItemText(hMain, tFontSizeY, szSize);
	}

	UpdateFontInfo();

	if (ghOpWnd)
	{
		ShowFontErrorTip(gpSet->szFontError);
		//tiBalloon.lpszText = gpSet->szFontError;
		//if (gpSet->szFontError[0]) {
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
	ShowErrorTip(asInfo, gpSet->hMain, tFontFace, gpSet->szFontError, countof(gpSet->szFontError),
	             gpSet->hwndBalloon, &gpSet->tiBalloon, gpSet->hwndTip, FAILED_FONT_TIMEOUT);
	//if (!asInfo)
	//	gpSet->szFontError[0] = 0;
	//else if (asInfo != gpSet->szFontError)
	//	lstrcpyn(gpSet->szFontError, asInfo, countof(gpSet->szFontError));
	//tiBalloon.lpszText = gpSet->szFontError;
	//if (gpSet->szFontError[0]) {
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
	if (FontSizeX && FontSizeY)
		LF.lfWidth = LogFont.lfWidth * FontSizeY / FontSizeX;
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
				gpSet->UpdateFontInfo();

				if (hMain)
				{
					wchar_t temp[16];
					_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", LogFont.lfHeight);
					SelectStringExact(hMain, tFontSizeY, temp);
					_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", FontSizeX);
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
		char szInfo[128]; sprintf_s(szInfo, "AutoRecreateFont(H=%i, W=%i)", nFontH, nFontW);
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
		if (isFontAutoSize)
		{
			isFontAutoSize = false;

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
				inFont->lfHeight = FontSizeY = nRastHeight;
				inFont->lfWidth = nRastWidth;
				FontSizeX = FontSizeX3 = nRastWidth;

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
		// Для пропорциональных шрифтов имеет смысл сохранять в реестре оптимальный lfWidth (это FontSizeX3)
		ZeroStruct(m_tm);
		BOOL lbTM = GetTextMetrics(hDC, m_tm);

		if (!lbTM && !bRasterFont)
		{
			// Считаем, что шрифт НЕ валиден!!!
			dwFontErr = GetLastError();
			SelectObject(hDC, hOldF);
			DeleteDC(hDC);
			_wsprintf(gpSet->szFontError, SKIPLEN(countof(gpSet->szFontError)) L"GetTextMetrics failed for non Raster font '%s'", inFont->lfFaceName);

			if (dwFontErr)
			{
				int nCurLen = _tcslen(gpSet->szFontError);
				_wsprintf(gpSet->szFontError+nCurLen, SKIPLEN(countof(gpSet->szFontError)-nCurLen)
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
					wcsncpy_s(inFont->lfFaceName, szFontFace, countof(inFont->lfFaceName)); inFont->lfFaceName[countof(inFont->lfFaceName)-1] = 0;
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

		//inFont->lfWidth = FontSizeX3 ? FontSizeX3 : m_tm->tmMaxCharWidth;
		// Лучше поставим AveCharWidth. MaxCharWidth для "условно моноширного" Consolas почти равен высоте.
		inFont->lfWidth = FontSizeX3 ? FontSizeX3 : m_tm->tmAveCharWidth;
		//} else {
		//	// Если указан FontSizeX3 (это принудительная ширина знакоместа)
		//    inFont->lfWidth = FontSizeX3 ? FontSizeX3 : m_tm->tmAveCharWidth;
		//}
		inFont->lfHeight = m_tm->tmHeight; TODO("Здесь нужно обновить реальный размер шрифта в диалоге настройки!");

		if (lbTM && m_tm->tmCharSet != DEFAULT_CHARSET)
		{
			inFont->lfCharSet = m_tm->tmCharSet;

			for(uint i=0; i < countof(ChSets); i++)
			{
				if (chSetsNums[i] == m_tm->tmCharSet)
				{
					SendDlgItemMessage(hMain, tFontCharset, CB_SETCURSEL, i, 0);
					break;
				}
			}
		}

		// если ширина шрифта стала больше ширины FixFarBorders - сбросить его в 0
		if (FontSizeX2 && (LONG)FontSizeX2 < inFont->lfWidth)
		{
			FontSizeX2 = 0;

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

		//int width = FontSizeX2 ? FontSizeX2 : inFont->lfWidth;
		LogFont2.lfWidth = mn_BorderFontWidth = FontSizeX2 ? FontSizeX2 : inFont->lfWidth;
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
			nValue = 255; // 0xFF для nFontNormalColor, nFontBoldColor, nFontItalicColor;
		else
			nValue = klatoi((szNumber[0]==L' ') ? (szNumber+1) : szNumber);
	}

	return nValue;
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

int CSettings::SelectStringExact(HWND hParent, WORD nCtrlId, LPCWSTR asText)
{
	if (!hParent)  // был ghOpWnd. теперь может быть вызван и для других диалогов!
		return -1;

#ifdef _DEBUG
	HWND hChild = GetDlgItem(hParent, nCtrlId);
	_ASSERTE(hChild!=NULL);
#endif
	int nIdx = SendDlgItemMessage(hParent, nCtrlId, CB_FINDSTRINGEXACT, -1, (LPARAM)asText);

	if (nIdx < 0)
	{
		int nCount = SendDlgItemMessage(hParent, nCtrlId, CB_GETCOUNT, 0, 0);
		int nNewVal = _wtol(asText);
		wchar_t temp[MAX_PATH];

		for(int i = 0; i < nCount; i++)
		{
			if (!SendDlgItemMessage(hParent, nCtrlId, CB_GETLBTEXT, i, (LPARAM)temp)) break;

			int nCurVal = _wtol(temp);

			if (nCurVal == nNewVal)
			{
				nIdx = i;
				break;
			}
			else if (nCurVal > nNewVal)
			{
				nIdx = SendDlgItemMessage(hParent, nCtrlId, CB_INSERTSTRING, i, (LPARAM) asText);
				break;
			}
		}

		if (nIdx < 0)
			nIdx = SendDlgItemMessage(hParent, nCtrlId, CB_INSERTSTRING, 0, (LPARAM) asText);
	}

	if (nIdx >= 0)
		SendDlgItemMessage(hParent, nCtrlId, CB_SETCURSEL, nIdx, 0);
	else
		SetDlgItemText(hParent, nCtrlId, asText);

	return nIdx;
}

// "Умолчательная" высота буфера.
// + ConEmu стартует в буферном режиме
// + команда по умолчанию (если не задана в реестре или ком.строке) будет "cmd", а не "far"
void CSettings::SetArgBufferHeight(int anBufferHeight)
{
	_ASSERTE(anBufferHeight>=0);
	//if (anBufferHeight>=0) DefaultBufferHeight = anBufferHeight;
	//ForceBufferHeight = (DefaultBufferHeight != 0);
	bForceBufferHeight = true;
	nForceBufferHeight = anBufferHeight;
	wcscpy_c(szDefCmd, (anBufferHeight==0) ? L"far" : L"cmd");
}

LPCTSTR CSettings::GetDefaultCmd()
{
	return szDefCmd;
}

void CSettings::ResetCmdArg()
{
	SingleInstanceArg = false;
	// Сбросить нужно только psCurCmd, psCmd не меняется - загружается только из настройки
	SafeFree(psCurCmd);
	//SettingsBase* reg = CreateSettings();
	//if (reg->OpenKey(Config, KEY_READ))
	//{
	//	reg->Load(L"CmdLine", &psCmd);
	//    reg->CloseKey();
	//}
	//delete reg;
}

LPCTSTR CSettings::GetCmd()
{
	if (psCurCmd && *psCurCmd)
		return psCurCmd;

	if (psCmd && *psCmd)
		return psCmd;

	SafeFree(psCurCmd); // впринципе, эта строка скорее всего не нужна, но на всякий случай...
	// Хорошо бы более корректно определить версию фара, но это не всегда просто
	// Например x64 файл сложно обработать в x86 ConEmu.
	DWORD nFarSize = 0;

	if (lstrcmpi(szDefCmd, L"far") == 0)
	{
		// Ищем фар. (1) В папке ConEmu, (2) в текущей директории, (2) на уровень вверх от папки ConEmu
		wchar_t szFar[MAX_PATH*2], *pszSlash;
		szFar[0] = L'"';
		wcscpy_add(1, szFar, gpConEmu->ms_ConEmuExeDir); // Теперь szFar содержит путь запуска программы
		pszSlash = szFar + _tcslen(szFar);
		_ASSERTE(pszSlash > szFar);
		BOOL lbFound = FALSE;

		// (1) В папке ConEmu
		if (!lbFound)
		{
			wcscpy_add(pszSlash, szFar, L"\\Far.exe");

			if (FileExists(szFar+1, &nFarSize))
				lbFound = TRUE;
		}

		// (2) в текущей директории
		if (!lbFound && lstrcmpi(gpConEmu->ms_ConEmuCurDir, gpConEmu->ms_ConEmuExeDir))
		{
			szFar[0] = L'"';
			wcscpy_add(1, szFar, gpConEmu->ms_ConEmuCurDir);
			wcscat_add(1, szFar, L"\\Far.exe");

			if (FileExists(szFar+1, &nFarSize))
				lbFound = TRUE;
		}

		// (3) на уровень вверх
		if (!lbFound)
		{
			szFar[0] = L'"';
			wcscpy_add(1, szFar, gpConEmu->ms_ConEmuExeDir);
			pszSlash = szFar + _tcslen(szFar);
			*pszSlash = 0;
			pszSlash = wcsrchr(szFar, L'\\');

			if (pszSlash)
			{
				wcscpy_add(pszSlash+1, szFar, L"Far.exe");

				if (FileExists(szFar+1, &nFarSize))
					lbFound = TRUE;
			}
		}

		if (lbFound)
		{
			// 110124 - нафиг, если пользователю надо - сам или параметр настроит, или реестр
			//// far чаще всего будет установлен в "Program Files", поэтому для избежания проблем - окавычиваем
			//// Пока тупо - если far.exe > 1200K - считаем, что это Far2
			//wcscat_c(szFar, (nFarSize>1228800) ? L"\" /w" : L"\"");
			wcscat_c(szFar, L"\"");
		}
		else
		{
			// Если Far.exe не найден рядом с ConEmu - запустить cmd.exe
			wcscpy_c(szFar, L"cmd");
		}

		// Finally - Result
		psCurCmd = lstrdup(szFar);
	}
	else
	{
		// Simple Copy
		psCurCmd = lstrdup(szDefCmd);
	}

	return psCurCmd;
}

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

	_ASSERTE(mn_FontWidth==LogFont.lfWidth);
	return mn_FontWidth;
}

LONG CSettings::FontHeight()
{
	if (!LogFont.lfHeight)
	{
		_ASSERTE(LogFont.lfHeight!=0);
		return 12;
	}

	_ASSERTE(mn_FontHeight==LogFont.lfHeight);
	return mn_FontHeight;
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
	_ASSERTE(mn_BorderFontWidth==LogFont2.lfWidth);
	return mn_BorderFontWidth;
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
		int nBtn = MessageBox(NULL, psz, gpConEmu->ms_ConEmuVer, MB_OKCANCEL|MB_ICONSTOP);
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
			LPOUTLINETEXTMETRICW lpOutl = gpSet->LoadOutline(NULL, hf);

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
			LPOUTLINETEXTMETRICW lpOutl = gpSet->LoadOutline(NULL, hf);

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
		int nBtn = MessageBox(NULL, psz, gpConEmu->ms_ConEmuVer, MB_OKCANCEL|MB_ICONSTOP);
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
			
			LPOUTLINETEXTMETRICW lpOutl = gpSet->LoadOutline(NULL, hf);
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
			LPOUTLINETEXTMETRICW lpOutl = gpSet->LoadOutline(NULL, hf);

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
	if (!isAutoRegisterFonts)
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
					int nBtn = MessageBox(NULL, psz, gpConEmu->ms_ConEmuVer, MB_OKCANCEL|MB_ICONSTOP);
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
					case 2:
						if (iSubFamOffset == -1)
							continue;
						iOffset = iSubFamOffset; iLen = iSubFamLength;
						break;
					}
					if (SetFilePointer(f, tbl.Offset + nam.StringOffset + iOffset, NULL, FILE_BEGIN)==INVALID_SET_FILE_POINTER)
						break;
					if (iLen >= sizeof(szData))
					{
						_ASSERTE(iLen < sizeof(szData));
						iLen = sizeof(szData)-1;
					}
					if (!ReadFile(f, szData, iLen, &(dwRead=0), NULL) || (dwRead != iLen))
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

void CSettings::HistoryCheck()
{
	if (!psCmdHistory || !*psCmdHistory)
	{
		nCmdHistorySize = 0;
	}
	else
	{
		const wchar_t* psz = psCmdHistory;

		while(*psz)
			psz += _tcslen(psz)+1;

		if (psz == psCmdHistory)
			nCmdHistorySize = 0;
		else
			nCmdHistorySize = (psz - psCmdHistory + 1)*sizeof(wchar_t);
	}
}

void CSettings::HistoryAdd(LPCWSTR asCmd)
{
	if (!asCmd || !*asCmd)
		return;

	if (psCmd && lstrcmp(psCmd, asCmd)==0)
		return;

	if (psCurCmd && lstrcmp(psCurCmd, asCmd)==0)
		return;

	HEAPVAL
	wchar_t *pszNewHistory, *psz;
	int nCount = 0;
	DWORD nCchNewSize = (nCmdHistorySize>>1) + _tcslen(asCmd) + 2;
	DWORD nNewSize = nCchNewSize*2;
	pszNewHistory = (wchar_t*)malloc(nNewSize);

	//wchar_t* pszEnd = pszNewHistory + nNewSize/sizeof(wchar_t);
	if (!pszNewHistory) return;

	_wcscpy_c(pszNewHistory, nCchNewSize, asCmd);
	psz = pszNewHistory + _tcslen(pszNewHistory) + 1;
	nCount++;

	if (psCmdHistory)
	{
		wchar_t* pszOld = psCmdHistory;
		int nLen;
		HEAPVAL;

		while(nCount < MAX_CMD_HISTORY && *pszOld /*&& psz < pszEnd*/)
		{
			const wchar_t *pszCur = pszOld;
			pszOld += _tcslen(pszOld) + 1;

			if (lstrcmp(pszCur, asCmd) == 0)
				continue;

			_wcscpy_c(psz, nCchNewSize-(psz-pszNewHistory), pszCur);
			psz += (nLen = (_tcslen(psz)+1));
			nCount ++;
		}
	}

	*psz = 0;
	HEAPVAL;
	free(psCmdHistory);
	psCmdHistory = pszNewHistory;
	nCmdHistorySize = (psz - pszNewHistory + 1)*sizeof(wchar_t);
	HEAPVAL;
	// И сразу сохранить в настройках
	SettingsBase* reg = CreateSettings();

	if (reg->OpenKey(ConfigPath, KEY_WRITE))
	{
		HEAPVAL;
		reg->SaveMSZ(L"CmdLineHistory", psCmdHistory, nCmdHistorySize);
		HEAPVAL;
		reg->CloseKey();
	}

	delete reg;
}

LPCWSTR CSettings::HistoryGet()
{
	if (psCmdHistory && *psCmdHistory)
		return psCmdHistory;

	return NULL;
}

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

// например, L"2013-25C3,25C4"
// Возвращает 0 - в случае успеха,
// при ошибке - индекс (1-based) ошибочного символа в asRanges
// -1 - ошибка выделения памяти
int CSettings::ParseCharRanges(LPCWSTR asRanges, BYTE (&Chars)[0x10000], BYTE abValue /*= TRUE*/)
{
	if (!asRanges)
	{
		_ASSERTE(asRanges!=NULL);
		return -1;
	}
	
	int iRc = 0;
	int n = 0, nMax = _tcslen(asRanges);
	wchar_t *pszCopy = lstrdup(asRanges);
	if (!pszCopy)
	{
		_ASSERTE(pszCopy!=NULL);
		return -1;
	}
	wchar_t *pszRange = pszCopy;
	wchar_t *pszNext = NULL;
	UINT cBegin, cEnd;
	
	memset(Chars, 0, sizeof(Chars));

	while(*pszRange && n < nMax)
	{
		cBegin = (UINT)wcstol(pszRange, &pszNext, 16);
		if (!cBegin || (cBegin > 0xFFFF))
		{
			iRc = (int)(pszRange - asRanges);
			goto wrap;
		}
			
		switch (*pszNext)
		{
		case L';':
		case 0:
			cEnd = cBegin;
			break;
		case L'-':
		case L' ':
			pszRange = pszNext + 1;
			cEnd = (UINT)wcstol(pszRange, &pszNext, 16);
			if ((cEnd < cBegin) || (cEnd > 0xFFFF))
			{
				iRc = (int)(pszRange - asRanges);
				goto wrap;
			}
			break;
		default:
			iRc = (int)(pszNext - asRanges);
			goto wrap;
		}

		for (UINT i = cBegin; i <= cEnd; i++)
			Chars[i] = abValue;
		
		if (*pszNext != L';') break;
		pszRange = pszNext + 1;
	}
	
	iRc = 0; // ok
wrap:
	if (pszCopy)
		free(pszCopy);
	return iRc;
}

// caller must free(result)
wchar_t* CSettings::CreateCharRanges(BYTE (&Chars)[0x10000])
{
	size_t nMax = 1024;
	wchar_t* pszRanges = (wchar_t*)calloc(nMax,sizeof(*pszRanges));
	if (!pszRanges)
	{
		_ASSERTE(pszRanges!=NULL);
		return NULL;
	}
	
	wchar_t* psz = pszRanges;
	wchar_t* pszEnd = pszRanges + nMax;
	UINT c = 0;
	_ASSERTE((countof(Chars)-1) == 0xFFFF);
	while (c < countof(Chars))
	{
		if (Chars[c])
		{
			if ((psz + 10) >= pszEnd)
			{
				// Слишком длинный блок
				_ASSERTE((psz + 10) < pszEnd);
				break;
			}
			
			UINT cBegin = (c++);
			UINT cEnd = cBegin;
			
			while (c < countof(Chars) && Chars[c])
			{
				cEnd = (c++);
			}
			
			if (cBegin == cEnd)
			{
				wsprintf(psz, L"%04X;", cBegin);
			}
			else
			{
				wsprintf(psz, L"%04X-%04X;", cBegin, cEnd);
			}
			psz += _tcslen(psz);
		}
		else
		{
			c++;
		}
	}
	
	return pszRanges;
}

bool CSettings::isCharBorder(wchar_t inChar)
{
	return mpc_FixFarBorderValues[(WORD)inChar];
}

void CSettings::ResetFontWidth()
{
	memset(CharWidth, 0, sizeof(*CharWidth)*0x10000);
	memset(CharABC, 0, sizeof(*CharABC)*0x10000);
}

BOOL CSettings::CheckConIme()
{
	long  lbStopWarning = FALSE;
	DWORD dwValue=1;
	SettingsBase* reg = CreateSettings();

	// БЕЗ имени конфигурации!
	if (reg->OpenKey(CONEMU_ROOT_KEY, KEY_READ))
	{
		if (!reg->Load(_T("StopWarningConIme"), lbStopWarning))
			lbStopWarning = FALSE;

		reg->CloseKey();
	}

	if (!lbStopWarning)
	{
		HKEY hk = NULL;

		if (0 == RegOpenKeyEx(HKEY_CURRENT_USER, L"Console", 0, KEY_READ, &hk))
		{
			DWORD dwType = REG_DWORD, nSize = sizeof(DWORD);

			if (0 != RegQueryValueEx(hk, L"LoadConIme", 0, &dwType, (LPBYTE)&dwValue, &nSize))
				dwValue = 1;

			RegCloseKey(hk);

			if (dwValue!=0)
			{
				if (IDCANCEL==MessageBox(0,
				                        L"Unwanted value of 'LoadConIme' registry parameter!\r\n"
				                        L"Press 'Cancel' to stop this message.\r\n"
				                        L"Take a look at 'FAQ-ConEmu.txt'.\r\n"
				                        L"You may simply import file 'Disable_ConIme.reg'\r\n"
				                        L"located in 'ConEmu.Addons' folder.",
				                        gpConEmu->ms_ConEmuVer,MB_OKCANCEL|MB_ICONEXCLAMATION))
					lbStopWarning = TRUE;
			}
		}
		else
		{
			if (IDCANCEL==MessageBox(0,
			                        L"Can't determine a value of 'LoadConIme' registry parameter!\r\n"
			                        L"Press 'Cancel' to stop this message.\r\n"
			                        L"Take a look at 'FAQ-ConEmu.txt'",
			                        gpConEmu->ms_ConEmuVer,MB_OKCANCEL|MB_ICONEXCLAMATION))
				lbStopWarning = TRUE;
		}

		if (lbStopWarning)
		{
			// БЕЗ имени конфигурации!
			if (reg->OpenKey(CONEMU_ROOT_KEY, KEY_WRITE))
			{
				reg->Save(_T("StopWarningConIme"), lbStopWarning);
				reg->CloseKey();
			}
		}
	}

	delete reg;
	return TRUE;
}

void CSettings::CheckConsoleSettings()
{
	// Если в ключе [HKEY_CURRENT_USER\Console] будут левые значения - то в Win7 могут
	// начаться страшные глюки :-)
	// например, консольное окно будет "дырявое" - рамка есть, а содержимого - нет :-P
	DWORD nFullScr = 0, nFullScrEmu = 0, nSize;
	HKEY hkCon, hkEmu;
	if (!RegOpenKeyEx(HKEY_CURRENT_USER, L"Console", 0, KEY_ALL_ACCESS, &hkCon))
	{
		if (RegQueryValueEx(hkCon, L"FullScreen", NULL, NULL, (LPBYTE)&nFullScr, &(nSize=sizeof(nFullScr))))
			nFullScr = 0;
		if (!RegOpenKeyEx(hkCon, CEC_INITTITLE, 0, KEY_ALL_ACCESS, &hkEmu))
		{
			if (RegQueryValueEx(hkEmu, L"FullScreen", NULL, NULL, (LPBYTE)&nFullScrEmu, &(nSize=sizeof(nFullScrEmu))))
				nFullScrEmu = 0;
			RegCloseKey(hkEmu);
		}
		else
		{
			nFullScrEmu = nFullScr;
		}
		RegCloseKey(hkCon);
	}
	
	if (nFullScr || nFullScrEmu)
	{
		if (gOSVer.dwMajorVersion >= 6)
		{
			wchar_t szWarning[512];
			_wsprintf(szWarning, SKIPLEN(countof(szWarning)) 
				L"Warning!\n"
				L"Dangerous values detected in yours registry\n\n"
				L"Please check [HKEY_CURRENT_USER\\Console] and [HKEY_CURRENT_USER\\Console\\ConEmu] keys\n\n"
				L"\"FullScreen\" recommended value is dword:00000000\n\n"
				L"Current value is dword:%08X",
				nFullScr ? nFullScr : nFullScrEmu);
			MBoxA(szWarning);
		}
	}
}

SettingsBase* CSettings::CreateSettings()
{
#ifndef __GNUC__
	SettingsBase* pReg = NULL;
	BOOL lbXml = FALSE;
	DWORD dwAttr = -1;
	LPWSTR pszXmlFile = gpConEmu->ConEmuXml();

	if (pszXmlFile && *pszXmlFile)
	{
		dwAttr = GetFileAttributes(pszXmlFile);

		if (dwAttr != (DWORD)-1 && !(dwAttr & FILE_ATTRIBUTE_DIRECTORY))
			lbXml = TRUE;
	}

	if (lbXml)
	{
		pReg = new SettingsXML();

		if (!((SettingsXML*)pReg)->IsXmlAllowed())
		{
			// Если MSXml.DomDocument не зарегистрирован
			pszXmlFile[0] = 0;
			lbXml = FALSE;
		}
	}

	if (!lbXml)
		pReg = new SettingsRegistry();

	return pReg;
#else
	return new SettingsRegistry();
#endif
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

			if (c == c32) ptc = &ThSet.crBackground;
			else if (c == c33) ptc = &ThSet.crPreviewFrame;
			else ptc = &ThSet.crSelectFrame;

			//
			if (ptc->UseIndex)
			{
				if (ptc->ColorIdx >= 0 && ptc->ColorIdx <= 15)
				{
					cr = Colors[ptc->ColorIdx];
				}
				else
				{
					const CEFAR_INFO_MAPPING *pfi = gpConEmu->ActiveCon()->RCon()->GetFarInfo();

					if (pfi && pfi->cbSize>=sizeof(CEFAR_INFO_MAPPING))
					{
						cr = Colors[(pfi->nFarColors[col_PanelText] & 0xF0)>>4];
					}
					else
					{
						cr = Colors[1];
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
			gpSet->GetColorById(c, &cr);
		}

		mh_CtlColorBrush = CreateSolidBrush(cr);
		return (INT_PTR)mh_CtlColorBrush;
	}

	return 0;
}

bool CSettings::GetColorById(WORD nID, COLORREF* color)
{
	if (nID <= c31)
		*color = Colors[nID - c0];
	else if (nID == c32)
		*color = ThSet.crBackground.ColorRGB;
	else if (nID == c33)
		*color = ThSet.crPreviewFrame.ColorRGB;
	else if (nID == c34)
		*color = ThSet.crSelectFrame.ColorRGB;
	else
		return false;

	return true;
}

bool CSettings::SetColorById(WORD nID, COLORREF color)
{
	if (nID <= c31)
	{
		Colors[nID - c0] = color;
		mb_FadeInitialized = false;
	}
	else if (nID == c32)
		ThSet.crBackground.ColorRGB = color;
	else if (nID == c33)
		ThSet.crPreviewFrame.ColorRGB = color;
	else if (nID == c34)
		ThSet.crSelectFrame.ColorRGB = color;
	else
		return false;

	return true;
}

bool CSettings::isHideCaptionAlways()
{
	return mb_HideCaptionAlways || (!mb_HideCaptionAlways && isUserScreenTransparent);
}

COLORREF* CSettings::GetColors(BOOL abFade)
{
	if (!abFade || !isFadeInactive)
		return Colors;

	if (!mb_FadeInitialized)
	{
		mn_LastFadeSrc = mn_LastFadeDst = -1;
		if (((int)mn_FadeHigh - (int)mn_FadeLow) < 64)
		{
			mn_FadeLow = DEFAULT_FADE_LOW; mn_FadeHigh = DEFAULT_FADE_HIGH;
		}

		mn_FadeMul = mn_FadeHigh - mn_FadeLow;
		mb_FadeInitialized = true;

		for(int i=0; i<countof(ColorsFade); i++)
		{
			ColorsFade[i] = GetFadeColor(Colors[i]);
		}
	}

	return ColorsFade;
}

COLORREF CSettings::GetFadeColor(COLORREF cr)
{
	if (!isFadeInactive)
		return cr;
		
	if (cr == mn_LastFadeSrc)
		return mn_LastFadeDst;

	MYCOLORREF mcr, mcrFade = {0}; mcr.color = cr;

	if (!mb_FadeInitialized)
	{
		GetColors(TRUE);
	}

	mcrFade.rgbRed = GetFadeColorItem(mcr.rgbRed);
	mcrFade.rgbGreen = GetFadeColorItem(mcr.rgbGreen);
	mcrFade.rgbBlue = GetFadeColorItem(mcr.rgbBlue);

	mn_LastFadeSrc = cr;
	mn_LastFadeDst = mcrFade.color;
	
	return mcrFade.color;
}

BYTE CSettings::GetFadeColorItem(BYTE c)
{
	DWORD nRc;

	switch(c)
	{
		case 0:
			return mn_FadeLow;
		case 255:
			return mn_FadeHigh;
		default:
			nRc = ((((DWORD)c) + mn_FadeLow) * mn_FadeHigh) >> 8;

			if (nRc >= 255)
			{
				//_ASSERTE(nRc <= 255); -- такие (mn_FadeLow&mn_FadeHigh) пользователь в настройке мог задать
				return 255;
			}

			return (BYTE)nRc;
	}
}

bool CSettings::NeedDialogDetect()
{
	// 100331 теперь оно нужно и для PanelView
	return true;
	//return (isUserScreenTransparent || !isMonospace);
}

void CSettings::FillListBoxItems(HWND hList, uint nItems, const WCHAR** pszItems, const DWORD* pnValues, DWORD& nValue)
{
	_ASSERTE(hList!=NULL);
	uint num = 0;

	for(uint i=0; i<nItems; i++)
	{
		SendMessage(hList, CB_ADDSTRING, 0, (LPARAM) pszItems[i]); //-V108

		if (pnValues[i] == nValue) num = i; //-V108
	}

	if (!num) nValue = 0;  // если код неизвестен?

	SendMessage(hList, CB_SETCURSEL, num, 0);
}

void CSettings::GetListBoxItem(HWND hList, uint nItems, const WCHAR** pszItems, const DWORD* pnValues, DWORD& nValue)
{
	_ASSERTE(hList!=NULL);
	INT_PTR num = SendMessage(hList, CB_GETCURSEL, 0, 0);

	//int nKeyCount = countof(Settings::szKeys);
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
//	if (isCTSSelectBlock && isCTSVkBlock && isPressed(isCTSVkBlock))
//		return true;
//	if (isCTSSelectText && isCTSVkText && isPressed(isCTSVkText))
//		return true;
//	return false;
//}

bool CSettings::isModifierPressed(DWORD vk)
{
	// если НЕ 0 - должен быть нажат
	if (vk)
	{
		if (!isPressed(vk))
			return false;
	}

	// но другие модификаторы нажаты быть не должны!
	if (vk != VK_SHIFT && vk != VK_LSHIFT && vk != VK_RSHIFT)
	{
		if (isPressed(VK_SHIFT))
			return false;
	}

	if (vk != VK_MENU && vk != VK_LMENU && vk != VK_RMENU)
	{
		if (isPressed(VK_MENU))
			return false;
	}

	if (vk != VK_CONTROL && vk != VK_LCONTROL && vk != VK_RCONTROL)
	{
		if (isPressed(VK_CONTROL))
			return false;
	}

	// Можно
	return true;
}

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

	if (isShowBgImage && sBgImage[0] && (GetTickCount() - nBgModifiedTick) > BACKGROUND_FILE_POLL)
	{
		WIN32_FIND_DATA fnd = {0};
		HANDLE hFind = FindFirstFile(sBgImage, &fnd);

		if (hFind != INVALID_HANDLE_VALUE)
		{
			if (fnd.ftLastWriteTime.dwHighDateTime != ftBgModified.dwHighDateTime
			        || fnd.ftLastWriteTime.dwLowDateTime != ftBgModified.dwLowDateTime)
			{
				//NeedBackgroundUpdate(); -- поставит LoadBackgroundFile, если у него получится файл открыть
				lbChanged = LoadBackgroundFile(sBgImage, false);
				nBgModifiedTick = GetTickCount();
			}

			FindClose(hFind);
		}
	}

	return lbChanged;
}

// Должна вернуть true, если файл изменился
bool CSettings::PrepareBackground(HDC* phBgDc, COORD* pbgBmpSize)
{
	/*
	    HBITMAP  hBgBitmap;
	    COORD    bgBmp;
	    HDC      hBgDc;
	    FILETIME ftBgModified;
	    WCHAR sBgImage[MAX_PATH];
	    char isShowBgImage;
		bool isBackgroundImageValid;
		u8 bgImageDarker;
		DWORD nBgImageColors;
	*/
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
	if (!mb_WasVConBgImage)
	{
		if (bgOperation == eUpLeft)
		{
			// MemoryDC создается всегда по размеру картинки, т.е. изменение размера окна - игнорируется
		}
		else
		{
			RECT rcWnd, rcWork; GetClientRect(ghWnd, &rcWnd);
			rcWork = gpConEmu->CalcRect(CER_WORKSPACE, rcWnd, CER_MAINCLIENT);

			// Смотрим дальше
			if (bgOperation == eStretch)
			{
				// Строго по размеру клиентской (точнее Workspace) области окна
				lMaxBgWidth = rcWork.right - rcWork.left;
				lMaxBgHeight = rcWork.bottom - rcWork.top;
			}
			else if (bgOperation == eTile)
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
		BackgroundOp op = (BackgroundOp)bgOperation;
		BOOL lbImageExist = (mp_BgImgData != NULL);
		BOOL lbVConImage = FALSE;
		LONG lBgWidth = 0, lBgHeight = 0;
		CVirtualConsole* pVCon = gpConEmu->ActiveCon();

		//MSectionLock SBK;
		if (pVCon && isBgPluginAllowed)
		{
			//SBK.Lock(&pVCon->csBkImgData);
			if (pVCon->HasBackgroundImage(&lBgWidth, &lBgHeight)
			        && lBgWidth && lBgHeight)
			{
				lbVConImage = lbImageExist = TRUE;
			}
		}

		mb_WasVConBgImage = lbVConImage;

		if (lbImageExist)
		{
			if (!mp_Bg)
				mp_Bg = new CBackground;

			mb_BgLastFade = (!bIsForeground && gpSet->isFadeInactive);
			TODO("Переделать, ориентироваться только на размер картинки - неправильно");
			TODO("DoubleView - скорректировать X,Y");

			if (lbVConImage)
			{
				if (lMaxBgWidth && lMaxBgHeight)
				{
					lBgWidth = lMaxBgWidth;
					lBgHeight = lMaxBgHeight;
				}

				if (!mp_Bg->CreateField(lBgWidth, lBgHeight) ||
				        !pVCon->PutBackgroundImage(mp_Bg, 0,0, lBgWidth, lBgHeight))
				{
					delete mp_Bg;
					mp_Bg = NULL;
				}
			}
			else
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
	if (isBgPluginAllowed && apVCon && apVCon->HasBackgroundImage(NULL,NULL))
	{
		if (apVCon->isEditor || apVCon->isViewer)
			return (isBgPluginAllowed == 1);

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
			MBoxA(L"Invalid 'inPath' in CSettings::LoadImageFrom");

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

	bool lRes = false;
	BITMAPFILEHEADER* pBkImgData = NULL;
	HANDLE hFile = CreateFile(exPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, NULL);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		BY_HANDLE_FILE_INFORMATION inf = {0};

		//LARGE_INTEGER nFileSize;
		//if (GetFileSizeEx(hFile, &nFileSize) && nFileSize.HighPart == 0
		if (GetFileInformationByHandle(hFile, &inf) && inf.nFileSizeHigh == 0
		        && inf.nFileSizeLow >= (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO))) //-V119
		{
			pBkImgData = (BITMAPFILEHEADER*)malloc(inf.nFileSizeLow);

			if (pBkImgData && ReadFile(hFile, pBkImgData, inf.nFileSizeLow, &inf.nFileSizeLow, NULL))
			{
				char *pBuf = (char*)pBkImgData;

				if (pBuf[0] == 'B' && pBuf[1] == 'M' && *(u32*)(pBuf + 0x0A) >= 0x36 && *(u32*)(pBuf + 0x0A) <= 0x436 && *(u32*)(pBuf + 0x0E) == 0x28 && !pBuf[0x1D] && !*(u32*)(pBuf + 0x1E))
				{
					ftBgModified = inf.ftLastWriteTime;
					nBgModifiedTick = GetTickCount();
					NeedBackgroundUpdate();
					_ASSERTE(gpConEmu->isMainThread());
					//MSectionLock SBG; SBG.Lock(&mcs_BgImgData);
					SafeFree(mp_BgImgData);
					isBackgroundImageValid = true;
					mp_BgImgData = pBkImgData;
					lRes = true;
				}
			}
		}

		SafeCloseHandle(hFile);
	}

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
	//    	PRAGMA_ERROR("Перенести код в CSettings::CreateBackgroundImage и переделать на AlphaBlend");
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
	//                    if (klstricmp(sBgImage, inPath))
	//                    {
	//                        lRes = true;
	//                        _tcscpy(sBgImage, inPath);
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
	//                            bArray[i].r = klMulDivU32(bArray[i].r, bgImageDarker, 255);
	//                            bArray[i].g = klMulDivU32(bArray[i].g, bgImageDarker, 255);
	//                            bArray[i].b = klMulDivU32(bArray[i].b, bgImageDarker, 255);
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
//	PRAGMA_ERROR("Доделать CSettings::CreateBackgroundImage");
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
	wchar_t szCreatedFaceName[32] = {0};
	LOGFONT LF = ConsoleFont;
	gpSet->nConFontError = 0; //ConFontErr_NonSystem|ConFontErr_NonRegistry|ConFontErr_InvalidName;
	HFONT hf = CreateFontIndirect(&LF);

	if (!hf)
	{
		gpSet->nConFontError = ConFontErr_InvalidName;
	}
	else
	{
		LPOUTLINETEXTMETRICW lpOutl = gpSet->LoadOutline(NULL, hf);

		if (!lpOutl)
		{
			gpSet->nConFontError = ConFontErr_InvalidName;
		}
		else
		{
			LPCWSTR pszFamilyName = (LPCWSTR)lpOutl->otmpFamilyName;

			// Интересуют только TrueType (вроде только для TTF доступен lpOutl - проверить
			if (pszFamilyName[0] != L'@'
			        && IsAlmostMonospace(pszFamilyName, lpOutl->otmTextMetrics.tmMaxCharWidth, lpOutl->otmTextMetrics.tmAveCharWidth, lpOutl->otmTextMetrics.tmHeight)
			        && lpOutl->otmPanoseNumber.bProportion == PAN_PROP_MONOSPACED
			        && lstrcmpi(pszFamilyName, ConsoleFont.lfFaceName) == 0
			  )
			{
				BOOL lbNonSystem = FALSE;

				// Нельзя использовать шрифты, которые зарегистрированы нами (для ConEmu). Они должны быть системными
				for (std::vector<RegFont>::iterator iter = gpSet->m_RegFonts.begin(); !lbNonSystem && iter != gpSet->m_RegFonts.end(); ++iter)
				{
					if (!iter->bAlreadyInSystem &&
					        lstrcmpi(iter->szFontName, ConsoleFont.lfFaceName) == 0)
						lbNonSystem = TRUE;
				}

				if (lbNonSystem)
					gpSet->nConFontError = ConFontErr_NonSystem;
			}
			else
			{
				gpSet->nConFontError = ConFontErr_NonSystem;
			}

			free(lpOutl);
		}
	}

	// Если успешно - проверить зарегистрированность в реестре
	if (gpSet->nConFontError == 0)
	{
		if (!CheckConsoleFontRegistry(gpSet->ConsoleFont.lfFaceName))
			gpSet->nConFontError |= ConFontErr_NonRegistry;
	}

	bConsoleFontChecked = (gpSet->nConFontError == 0);
	return bConsoleFontChecked;
}

bool CSettings::CheckConsoleFont(HWND ahDlg)
{
	gpSet->nConFontError = ConFontErr_NonSystem|ConFontErr_NonRegistry;
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
		gpSet->nConFontError &= ~(DWORD)ConFontErr_NonRegistry;

	return (gpSet->nConFontError == 0);
}

INT_PTR CSettings::EditConsoleFontProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch(messg)
	{
		case WM_INITDIALOG:
		{
			SendMessage(hWnd2, WM_SETICON, ICON_BIG, (LPARAM)hClassIcon);
			SendMessage(hWnd2, WM_SETICON, ICON_SMALL, (LPARAM)hClassIconSm);

			gpSet->hConFontDlg = NULL; // пока не выставим - на смену в контролах не реагировать
			wchar_t temp[10];

			for(uint i=0; i < countof(Settings::FSizesSmall); i++)
			{
				_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", Settings::FSizesSmall[i]);
				SendDlgItemMessage(hWnd2, tConsoleFontSizeY, CB_ADDSTRING, 0, (LPARAM) temp);
				_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", (int)(Settings::FSizesSmall[i]*3/2));
				SendDlgItemMessage(hWnd2, tConsoleFontSizeX, CB_ADDSTRING, 0, (LPARAM) temp);

				if (Settings::FSizesSmall[i] >= gpSet->LogFont.lfHeight)
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
				EnableWindow(GetDlgItem(hWnd2, bConFontAdd2HKLM), (gpSet->nConFontError&ConFontErr_NonRegistry)!=0);
			}

			// BCM_SETSHIELD = 5644
			if (gOSVer.dwMajorVersion >= 6)
				SendDlgItemMessage(hWnd2, bConFontAdd2HKLM, 5644/*BCM_SETSHIELD*/, 0, TRUE);

			// запускаем user-mode
			gpSet->hConFontDlg = hWnd2;
			gpSet->RegisterTipsFor(hWnd2);
			CenterMoreDlg(hWnd2);

			if (gpConEmu->ActiveCon())
				EnableWindow(GetDlgItem(hWnd2, tConsoleFontConsoleNote), TRUE);

			// Если есть ошибка - показать
			gpSet->bShowConFontError = (gpSet->nConFontError != 0);
		}
		break;
		case WM_DESTROY:

			if (gpSet->hwndConFontBalloon) {DestroyWindow(gpSet->hwndConFontBalloon); gpSet->hwndConFontBalloon = NULL;}

			gpSet->hConFontDlg = NULL;
			break;
		case WM_TIMER:

			if (wParam == FAILED_FONT_TIMERID)
			{
				KillTimer(hWnd2, FAILED_FONT_TIMERID);
				SendMessage(gpSet->hwndConFontBalloon, TTM_TRACKACTIVATE, FALSE, (LPARAM)&gpSet->tiConFontBalloon);
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
					if (gpSet->nConFontError == ConFontErr_NonRegistry && gpSet->CheckConsoleFontRegistry(gpSet->ConsoleFont.lfFaceName))
					{
						gpSet->nConFontError &= ~(DWORD)ConFontErr_NonRegistry;
					}

					if (gpSet->nConFontError)
					{
						_ASSERTE(gpSet->nConFontError==0);
						MessageBox(hWnd2, gpSet->sConFontError[0] ? gpSet->sConFontError : gpSet->CreateConFontError(NULL,NULL), gpConEmu->ms_ConEmuVer, MB_OK|MB_ICONSTOP);
						return 0;
					}

					gpSet->SaveConsoleFont(); // Сохранить шрифт в настройке
					EndDialog(hWnd2, IDOK);
				}
				else if (TB == IDCANCEL || TB == bConFontCancel)
				{
					if (!gpSet->bConsoleFontChecked)
					{
						wcscpy_c(gpSet->ConsoleFont.lfFaceName, gpSet->sDefaultConFontName[0] ? gpSet->sDefaultConFontName : L"Lucida Console");
						gpSet->ConsoleFont.lfHeight = 5; gpSet->ConsoleFont.lfWidth = 3;
					}

					EndDialog(hWnd2, IDCANCEL);
				}
				else if (TB == bConFontAdd2HKLM)
				{
					// Добавить шрифт в HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Console\TrueTypeFont
					gpSet->ShowConFontErrorTip(NULL);
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
							DWORD nWait = WaitForSingleObject(sei.hProcess, 30000);
							CloseHandle(sei.hProcess);

							if (gpSet->CheckConsoleFontRegistry(gpSet->ConsoleFont.lfFaceName))
							{
								lbFontJustRegistered = lbFound = true; // OK, добавили
							}
						}
					}

					if (lbFound)
					{
						SetFocus(GetDlgItem(hWnd2, tConsoleFontFace));
						EnableWindow(GetDlgItem(hWnd2, bConFontAdd2HKLM), FALSE);
						gpSet->nConFontError &= ~(DWORD)ConFontErr_NonRegistry;

						if (lbFontJustRegistered)
						{
							// Если шрифт только что зарегистрировали - его нельзя использовать до перезагрузки компьютера
							if (lbFontJustRegistered && gpSet->sDefaultConFontName[0])
							{
								wcscpy_c(gpSet->ConsoleFont.lfFaceName, gpSet->sDefaultConFontName);

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
				gpSet->nConFontError = 0; //ConFontErr_NonSystem|ConFontErr_NonRegistry|ConFontErr_InvalidName;
				int nIdx = SendDlgItemMessage(hWnd2, tConsoleFontFace, CB_FINDSTRINGEXACT, -1, (LPARAM)LF.lfFaceName); //-V103

				if (nIdx < 0)
				{
					gpSet->nConFontError = ConFontErr_NonSystem;
				}
				else
				{
					HFONT hf = CreateFontIndirect(&LF);

					if (!hf)
					{
						EnableWindow(GetDlgItem(hWnd2, bConFontOK), FALSE);
						gpSet->nConFontError = ConFontErr_InvalidName;
					}
					else
					{
						LPOUTLINETEXTMETRICW lpOutl = gpSet->LoadOutline(NULL, hf);

						if (!lpOutl)
						{
							// Ошибка
							gpSet->nConFontError = ConFontErr_InvalidName;
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
								gpSet->nConFontError |= ConFontErr_InvalidName;
						}

						DeleteObject(hf);
					}
				}

				if (gpSet->nConFontError == 0)
				{
					// Осталось проверить регистрацию в реестре
					wcscpy_c(gpSet->ConsoleFont.lfFaceName, LF.lfFaceName);
					gpSet->ConsoleFont.lfHeight = LF.lfHeight;
					gpSet->ConsoleFont.lfWidth = LF.lfWidth;
					bool lbRegChecked = CheckConsoleFontRegistry(gpSet->ConsoleFont.lfFaceName);

					if (!lbRegChecked) gpSet->nConFontError |= ConFontErr_NonRegistry;

					EnableWindow(GetDlgItem(hWnd2, bConFontOK), lbRegChecked);
					EnableWindow(GetDlgItem(hWnd2, bConFontAdd2HKLM), !lbRegChecked);
				}
				else
				{
					EnableWindow(GetDlgItem(hWnd2, bConFontOK), FALSE);
					EnableWindow(GetDlgItem(hWnd2, bConFontAdd2HKLM), FALSE);
				}

				ShowConFontErrorTip(gpSet->CreateConFontError(LF.lfFaceName, szCreatedFaceName));
			}

			break;
		case WM_ACTIVATE:

			if (LOWORD(wParam) == WA_INACTIVE)
				ShowConFontErrorTip(NULL);
			else if (gpSet->bShowConFontError)
			{
				gpSet->bShowConFontError = FALSE;
				ShowConFontErrorTip(gpSet->CreateConFontError(NULL,NULL));
			}

			break;
	}

	return 0;
}

void CSettings::ShowConFontErrorTip(LPCTSTR asInfo)
{
	ShowErrorTip(asInfo, gpSet->hConFontDlg, tConsoleFontFace, gpSet->sConFontError, countof(gpSet->sConFontError),
	             gpSet->hwndConFontBalloon, &gpSet->tiConFontBalloon, NULL, FAILED_CONFONT_TIMEOUT);
	//if (!asInfo)
	//	gpSet->sConFontError[0] = 0;
	//else if (asInfo != gpSet->sConFontError)
	//	lstrcpyn(gpSet->sConFontError, asInfo, countof(gpSet->sConFontError));
	//tiConFontBalloon.lpszText = gpSet->sConFontError;
	//if (gpSet->sConFontError[0]) {
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

	SendMessage(gpSet->hwndConFontBalloon, TTM_SETTITLE, TTI_WARNING, (LPARAM)(asReqFont ? asReqFont : ConsoleFont.lfFaceName));
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
	for (std::vector<RegFont>::iterator iter = gpSet->m_RegFonts.begin(); iter != gpSet->m_RegFonts.end(); ++iter)
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

	LPOUTLINETEXTMETRICW lpOutl = gpSet->LoadOutline(NULL, hf);

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
		gpSet->nConFontError &= ~(DWORD)ConFontErr_NonSystem;

	if (hWnd2)
	{
		if (SendDlgItemMessage(hWnd2, tConsoleFontFace, CB_FINDSTRINGEXACT, -1, (LPARAM) LF.lfFaceName)==-1)
		{
			int nIdx;
			nIdx = SendDlgItemMessage(hWnd2, tConsoleFontFace, CB_ADDSTRING, 0, (LPARAM) LF.lfFaceName); //-V103
		}
	}

	if (gpSet->sDefaultConFontName[0] == 0)
	{
		if (CheckConsoleFontRegistry(LF.lfFaceName))
			wcscpy_c(gpSet->sDefaultConFontName, LF.lfFaceName);
	}

	MCHKHEAP
	return TRUE;
	UNREFERENCED_PARAMETER(lpntm);
}
