
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

#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "Header.h"
#include <commctrl.h>
#include <shlobj.h>
//#include "../common/ConEmuCheck.h"
#include "Options.h"
#include "ConEmu.h"
#include "ConEmuApp.h"
#include "ConEmuCtrl.h"
#include "VConChild.h"
#include "VirtualConsole.h"
#include "VConGroup.h"
#include "RealConsole.h"
#include "TabBar.h"
#include "Background.h"
#include "TrayIcon.h"
#include "LoadImg.h"
#include "Status.h"
#include "Recreate.h"
#include "../ConEmuCD/GuiHooks.h"
#include "../ConEmuCD/ExitCodes.h"
#include "version.h"

//#define CONEMU_ROOT_KEY L"Software\\ConEmu"

#define DEBUGSTRFONT(s) DEBUGSTR(s)

#define COUNTER_REFRESH 5000

#define RASTER_FONTS_NAME L"Raster Fonts"
SIZE szRasterSizes[100] = {{0,0}}; // {{16,8},{6,9},{8,9},{5,12},{7,12},{8,12},{16,12},{12,16},{10,18}};
const wchar_t szRasterAutoError[] = L"Font auto size is not allowed for a fixed raster font size. Select 'Terminal' instead of '[Raster Fonts ...]'";

// Тут можно бы оставить "LF.lfHeight". При выборе другого шрифта - может меняться высота?
// Хотя, наверное все же лучше не включать "AI", а дать пользователю задать то, что хочется ему.
#define CurFontSizeY gpSet->FontSizeY/*LF.lfHeight*/ 		
#undef UPDATE_FONTSIZE_RECREATE


#define TEST_FONT_WIDTH_STRING_EN L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define TEST_FONT_WIDTH_STRING_RU L"АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"

#define FAILED_FONT_TIMERID 101
#define FAILED_FONT_TIMEOUT 3000
#define FAILED_CONFONT_TIMEOUT 30000
#define FAILED_SELMOD_TIMEOUT 5000

//const WORD HostkeyCtrlIds[] = {cbHostWin, cbHostApps, cbHostLCtrl, cbHostRCtrl, cbHostLAlt, cbHostRAlt, cbHostLShift, cbHostRShift};
//const BYTE HostkeyVkIds[]   = {VK_LWIN,   VK_APPS,    VK_LCONTROL, VK_RCONTROL, VK_LMENU,   VK_RMENU,   VK_LSHIFT,    VK_RSHIFT};
//int upToFontHeight=0;
HWND ghOpWnd=NULL;

#ifdef _DEBUG
#define HEAPVAL //HeapValidate(GetProcessHeap(), 0, NULL);
#else
#define HEAPVAL
#endif

//#define DEFAULT_FADE_LOW 0
//#define DEFAULT_FADE_HIGH 0xD0

//struct CONEMUDEFCOLORS
//{
//	const wchar_t* pszTitle;
//	DWORD dwDefColors[0x10];
//};
//
//const CONEMUDEFCOLORS DefColors[] =
//{
//	{
//		L"Default color scheme (Windows standard)", {
//			0x00000000, 0x00800000, 0x00008000, 0x00808000, 0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0,
//			0x00808080, 0x00ff0000, 0x0000ff00, 0x00ffff00, 0x000000ff, 0x00ff00ff, 0x0000ffff, 0x00ffffff
//		}
//	},
//	{
//		L"Gamma 1 (for use with dark monitors)", {
//			0x00000000, 0x00960000, 0x0000aa00, 0x00aaaa00, 0x000000aa, 0x00800080, 0x0000aaaa, 0x00c0c0c0,
//			0x00808080, 0x00ff0000, 0x0000ff00, 0x00ffff00, 0x000000ff, 0x00ff00ff, 0x0000ffff, 0x00ffffff
//		}
//	},
//	{
//		L"Murena scheme", {
//			0x00000000, 0x00644100, 0x00008000, 0x00808000, 0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0,
//			0x00808080, 0x00ff0000, 0x0076c587, 0x00ffff00, 0x00004bff, 0x00d78ce6, 0x0000ffff, 0x00ffffff
//		}
//	},
//	{
//		L"tc-maxx", {
//			0x00000000, RGB(11,27,59), RGB(0,128,0), RGB(0,90,135), RGB(106,7,28), RGB(128,0,128), RGB(128,128,0), RGB(40,150,177),
//			RGB(128,128,128), RGB(0,0,255), RGB(0,255,0), RGB(0,215,243), RGB(190,7,23), RGB(255,0,255), RGB(255,255,0), RGB(255,255,255)
//		}
//	},
//	{
//		L"Solarized (John Doe)", {
//			0x00362b00, 0x00423607, 0x00756e58, 0x00837b65, 0x002f32dc, 0x00c4716c, 0x00164bcb, 0x00d5e8ee,
//			0x00a1a193, 0x00d28b26, 0x00009985, 0x0098a12a, 0x00969483, 0x008236d3, 0x000089b5, 0x00e3f6fd		
//		}
//	},
//
//};
//const DWORD *dwDefColors = DefColors->dwDefColors;
//DWORD gdwLastColors[0x20] = {0};
BOOL  gbLastColorsOk = FALSE;
Settings::ColorPalette gLastColors = {};

#define MAX_COLOR_EDT_ID c31



namespace SettingsNS
{
	const WCHAR* szBgOper[] = {L"UpLeft", L"UpRight", L"DownLeft", L"DownRight", L"Stretch", L"Tile"};
	const DWORD  nBgOper[] =  {eUpLeft,   eUpRight,   eDownLeft,   eDownRight,   eStretch,   eTile};
	const WCHAR* szKeys[] = {L"<None>", L"Left Ctrl", L"Right Ctrl", L"Left Alt", L"Right Alt", L"Left Shift", L"Right Shift"};
	const DWORD  nKeys[] =  {0,         VK_LCONTROL,  VK_RCONTROL,   VK_LMENU,    VK_RMENU,     VK_LSHIFT,     VK_RSHIFT};
	const WCHAR* szModifiers[] = {L" ", L"Win",  L"Apps",  L"Ctrl", L"LCtrl", L"RCtrl",           L"Alt", L"LAlt", L"RAlt",     L"Shift", L"LShift", L"RShift"};
	const DWORD  nModifiers[] =  {0,    VK_LWIN, VK_APPS,  VK_CONTROL, VK_LCONTROL, VK_RCONTROL,  VK_MENU, VK_LMENU, VK_RMENU,  VK_SHIFT, VK_LSHIFT, VK_RSHIFT};
	const WCHAR* szKeysAct[] = {L"<Always>", L"Left Ctrl", L"Right Ctrl", L"Left Alt", L"Right Alt", L"Left Shift", L"Right Shift"};
	const DWORD  nKeysAct[] =  {0,         VK_LCONTROL,  VK_RCONTROL,   VK_LMENU,    VK_RMENU,     VK_LSHIFT,     VK_RSHIFT};
	const WCHAR* szKeysHot[] = {L"", L"Esc", L"Delete", L"Tab", L"Enter", L"Space", L"Backspace", L"Pause", L"Wheel Up", L"Wheel Down", L"Wheen Left", L"Wheel Right"};
	const DWORD  nKeysHot[] =  {0, VK_ESCAPE, VK_DELETE, VK_TAB, VK_RETURN, VK_SPACE, VK_BACK, VK_PAUSE, VK_WHEEL_UP, VK_WHEEL_DOWN, VK_WHEEL_LEFT, VK_WHEEL_RIGHT};
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
	const struct StatusBarIds { CEStatusItems stItem; UINT nDlgID; } StatusItems[] = 
	{
		{csi_ActiveProcess, cbStatusActiveProcess}, {csi_ConsoleTitle, cbStatusConsoleTitle},
		{csi_ActiveVCon, cbStatusActiveVCon}, {csi_NewVCon, cbStatusNewVCon}, {csi_CapsLock, cbStatusCapsLock}, {csi_NumLock, cbStatusNumLock},
		{csi_ScrollLock, cbStatusScrlLock}, {csi_InputLocale, cbStatusInputLang}, {csi_WindowPos, cbStatusWindowPos},
		{csi_WindowSize, cbStatusWindowSize}, {csi_WindowClient, cbStatusWindowClient}, {csi_WindowWork, cbStatusWindowWorkspace},
		{csi_ActiveBuffer, cbStatusActiveBuffer}, {csi_ConsolePos, cbStatusConsolePos}, {csi_ConsoleSize, cbStatusConsoleSize},
		{csi_BufferSize, cbStatusBufferSize}, {csi_CursorX, cbStatusCursorX}, {csi_CursorY, cbStatusCursorY},
		{csi_CursorSize, cbStatusCursorSize}, {csi_CursorInfo, cbStatusCursorInfo}, {csi_ConEmuPID, cbStatusGuiPID},
		{csi_ConEmuHWND, cbStatusGuiHwnd}, {csi_ConEmuView, cbStatusViewHwnd}, {csi_Server, cbStatusServer},
		{csi_ServerHWND, cbStatusRealHwnd}, {csi_Transparency, cbStatusTransparency}
	};
	const WCHAR* szCRLF[] = {L"CR+LF", L"LF", L"CR"};
	const DWORD  nCRLF[] =  {0, 1, 2};
	const WORD nSizeCtrlId[] = {tWndWidth, stWndWidth, tWndHeight, stWndHeight};
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

	#ifdef SHOW_AUTOSCROLL
	AutoScroll = true;
	#endif
	
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
	//ZeroStruct(m_QuakePrevSize);
	
	// Некоторые вещи нужно делать вне InitSettings, т.к. она может быть
	// вызвана потом из интерфейса диалога настроек
	wcscpy_c(ConfigPath, CONEMU_ROOT_KEY L"\\.Vanilla");
	ConfigName[0] = 0;
	//Type[0] = 0;
	//gpSet->psCmd = NULL; gpSet->psCurCmd = NULL;
	SetDefaultCmd(L"far");
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

	mh_FindDlg = NULL;
	
	// Теперь установим умолчания настроек	
	gpSet->InitSettings();
	
	SingleInstanceArg = false;
	SingleInstanceShowHide = sih_None;
	mb_StopRegisterFonts = FALSE;
	mb_IgnoreEditChanged = FALSE;
	mb_IgnoreTtfChange = TRUE;
	gpSet->mb_CharSetWasSet = FALSE;
	mb_TabHotKeyRegistered = FALSE;
	//hMain = hExt = hFar = hTabs = hKeys = hColors = hCmdTasks = hViews = hInfo = hDebug = hUpdate = hSelection = NULL;
	memset(mh_Tabs, 0, sizeof(mh_Tabs));
	hwndTip = NULL; hwndBalloon = NULL;
	mb_IgnoreCmdGroupEdit = mb_IgnoreCmdGroupList = false;
	hConFontDlg = NULL; hwndConFontBalloon = NULL; bShowConFontError = FALSE; sConFontError[0] = 0; bConsoleFontChecked = FALSE;
	if (gbIsDBCS)
	{
		wcscpy_c(sDefaultConFontName, gsDefConFont);
	}
	else
	{
		sDefaultConFontName[0] = 0;
	}
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
#if 0
	//120714 - аналогичные параметры работают в ConEmuC.exe, а в GUI они и не работали. убрал пока
	nAttachPID = 0; hAttachConWnd = NULL;
#endif
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
	m_HotKeys = gpSet->AllocateHotkeys();

	mp_ActiveHotKey = NULL;
}

// pRCon may be NULL
const ConEmuHotKey* CSettings::GetHotKeyInfo(DWORD VkMod, bool bKeyDown, CRealConsole* pRCon)
{
	DWORD vk = gpSet->GetHotkey(VkMod);
	// На сами модификаторы - действий не вешается
	if (vk == VK_LWIN || vk == VK_RWIN /*|| vk == VK_APPS*/
		|| vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT
		|| vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL
		|| vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU)
	{
		return NULL;
	}

	const ConEmuHotKey* p = NULL;

	DWORD nState = (VkMod & cvk_ALLMASK);

	// Теперь бежим по m_HotKeys и сравниваем требуемые модификаторы
	for (int i = 0; m_HotKeys[i].DescrLangID; i++)
	{
		if (m_HotKeys[i].HkType == chk_Modifier)
			continue;

		WARNING("ConEmuHotKey: Требуется заменить. Указатели вообще не нужны, вся инфа должна храниться в m_HotKeys");
		DWORD TestVkMod = m_HotKeys[i].VkMod;
		if (gpSet->GetHotkey(TestVkMod) != vk)
			continue; // Не совпадает сама кнопка

		DWORD TestMask = cvk_DISTINCT;
		DWORD TestValue = 0;

		for (int k = 1; k <= 3; k++)
		{
			switch (gpSet->GetModifier(TestVkMod, k))
			{
			case 0:
				break; // нету
			case VK_LWIN: case VK_RWIN: // RWin быть не должно по идее
				TestValue |= cvk_Win; break;
			case VK_APPS:
				TestValue |= cvk_Apps; break;

			case VK_LCONTROL:
				TestValue |= cvk_LCtrl; break;
			case VK_RCONTROL:
				TestValue |= cvk_RCtrl; break;
			case VK_CONTROL:
				TestValue |= cvk_Ctrl;
				TestValue &= ~(cvk_LCtrl|cvk_RCtrl);
				TestMask |= cvk_Ctrl;
				TestMask &= ~(cvk_LCtrl|cvk_RCtrl);
				break;

			case VK_LMENU:
				TestValue |= cvk_LAlt; break;
			case VK_RMENU:
				TestValue |= cvk_RAlt; break;
			case VK_MENU:
				TestValue |= cvk_Alt;
				TestValue &= ~(cvk_LAlt|cvk_RAlt);
				TestMask |= cvk_Alt;
				TestMask &= ~(cvk_LAlt|cvk_RAlt);
				break;

			case VK_LSHIFT:
				TestValue |= cvk_LShift; break;
			case VK_RSHIFT:
				TestValue |= cvk_RShift; break;
			case VK_SHIFT:
				TestValue |= cvk_Shift;
				TestValue &= ~(cvk_LShift|cvk_RShift);
				TestMask |= cvk_Shift;
				TestMask &= ~(cvk_LShift|cvk_RShift);
				break;

			#ifdef _DEBUG
			default:
				// Неизвестный!
				_ASSERTE(gpSet->GetModifier(TestVkMod, k)==0);
			#endif
			}
		}

		if ((nState & TestMask) != TestValue)
			continue;

		if (m_HotKeys[i].fkey)
		{
			// Допускается ли этот хоткей в текущем контексте?
			if (m_HotKeys[i].fkey(VkMod, true, (m_HotKeys+i), pRCon))
			{
				p = (m_HotKeys+i);
				break; // Нашли
			}
		}
		else
		{
			_ASSERTE(m_HotKeys[i].fkey!=NULL);
		}
	}

	// Некоторые комбинации нужно обрабатывать "на отпускание" во избежание глюков с интерфейсом
    if (p)
    {
    	// Поэтому проверяем, совпадает ли требование "нажатости"
    	if (p->OnKeyUp == bKeyDown)
    		p = ConEmuSkipHotKey;
    }

    return p;
}

bool CSettings::HasSingleWinHotkey()
{
	for (int i = 0; m_HotKeys[i].DescrLangID; i++)
	{
		if (m_HotKeys[i].HkType == chk_Modifier)
			continue;
		DWORD VkMod = m_HotKeys[i].VkMod;
		if (gpSet->GetModifier(VkMod, 1) == VK_LWIN)
		{
			// Win+<другой модификатор> вроде винда в приложение таки присылает?
        	if (gpSet->GetModifier(VkMod, 2) == 0)
        		return true; // А вот если больше модификаторов нет...
		}
	}
	return false;
}

void CSettings::UpdateWinHookSettings(HMODULE hLLKeyHookDll)
{
	BOOL *pbWinTabHook = (BOOL*)GetProcAddress(hLLKeyHookDll, "gbWinTabHook");
	BYTE *pnConsoleKeyShortcuts = (BYTE*)GetProcAddress(hLLKeyHookDll, "gnConsoleKeyShortcuts");

	if (pbWinTabHook)
		*pbWinTabHook = gpSet->isUseWinTab;

	if (pnConsoleKeyShortcuts)
	{
		BYTE nNewValue = 0;
		
		if (gpSet->isSendAltTab) nNewValue |= 1<<ID_ALTTAB;
		if (gpSet->isSendAltEsc) nNewValue |= 1<<ID_ALTESC;
		if (gpSet->isSendAltPrintScrn) nNewValue |= 1<<ID_ALTPRTSC;
		if (gpSet->isSendPrintScrn) nNewValue |= 1<<ID_PRTSC;
		if (gpSet->isSendCtrlEsc) nNewValue |= 1<<ID_CTRLESC;
		
		CVirtualConsole* pVCon;
		for (size_t i = 0;; i++)
		{
			pVCon = gpConEmu->GetVCon(i, true);
			if (!pVCon)
				break;
			nNewValue |= pVCon->RCon()->GetConsoleKeyShortcuts();
		}
		
		*pnConsoleKeyShortcuts = nNewValue;
	}

	// __declspec(dllexport) DWORD gnHookedKeys[HookedKeysMaxCount] = {};
	DWORD *pnHookedKeys = (DWORD*)GetProcAddress(hLLKeyHookDll, "gnHookedKeys");
	if (pnHookedKeys)
	{
		DWORD *pn = pnHookedKeys;

		//WARNING("CConEmuCtrl:: Тут вообще наверное нужно по всем HotKeys проехаться, а не только по «избранным»");
		for (int i = 0; m_HotKeys[i].DescrLangID; i++)
		{
			if ((m_HotKeys[i].HkType == chk_Modifier) || (m_HotKeys[i].HkType == chk_Global))
				continue;

			if (m_HotKeys[i].Enabled && !*m_HotKeys[i].Enabled)
				continue;

			DWORD VkMod = m_HotKeys[i].VkMod;

			if (gpSet->HasModifier(VkMod, VK_LWIN))
			{
				DWORD nFlags = gpSet->GetHotkey(VkMod);
				for (int i = 1; i <= 3; i++)
				{
					switch (gpSet->GetModifier(VkMod, i))
					{
					case 0:
						break;
					case VK_LWIN: case VK_RWIN:
						nFlags |= cvk_Win; break;
					case VK_APPS:
						nFlags |= cvk_Apps; break;
					case VK_CONTROL:
						nFlags |= cvk_Ctrl; break;
					case VK_LCONTROL:
						nFlags |= cvk_LCtrl|cvk_Ctrl; break;
					case VK_RCONTROL:
						nFlags |= cvk_RCtrl|cvk_Ctrl; break;
					case VK_MENU:
						nFlags |= cvk_Alt; break;
					case VK_LMENU:
						nFlags |= cvk_LAlt|cvk_Alt; break;
					case VK_RMENU:
						nFlags |= cvk_RAlt|cvk_Alt; break;
					case VK_SHIFT:
						nFlags |= cvk_Shift; break;
					case VK_LSHIFT:
						nFlags |= cvk_LShift|cvk_Shift; break;
					case VK_RSHIFT:
						nFlags |= cvk_RShift|cvk_Shift; break;
					}
				}

				*(pn++) = nFlags;
			}
		}
		
		//if (gpSet->isMulti)
		//{
		//	if (gpSet->HasModifier(gpSet->vmMultiNew, VK_LWIN))
		//		*(pn++) = gpSet->vmMultiNew;
		//	if (gpSet->HasModifier(gpSet->vmMultiNewShift, VK_LWIN))
		//		*(pn++) = gpSet->vmMultiNewShift;
		//	if (gpSet->HasModifier(gpSet->vmMultiNext, VK_LWIN))
		//		*(pn++) = gpSet->vmMultiNext;
		//	if (gpSet->HasModifier(gpSet->vmMultiNextShift, VK_LWIN))
		//		*(pn++) = gpSet->vmMultiNextShift;
		//	if (gpSet->HasModifier(gpSet->vmMultiRecreate, VK_LWIN))
		//		*(pn++) = gpSet->vmMultiRecreate;
		//	if (gpSet->HasModifier(gpSet->vmMultiBuffer, VK_LWIN))
		//		*(pn++) = gpSet->vmMultiBuffer;
		//	if (gpSet->HasModifier(gpSet->vmMultiClose, VK_LWIN))
		//		*(pn++) = gpSet->vmMultiClose;
		//	if (gpSet->HasModifier(gpSet->vmMultiCmd, VK_LWIN))
		//		*(pn++) = gpSet->vmMultiCmd;
		//}

		//if (gpSet->isUseWinArrows)
		//{
		//	*(pn++) = VK_LEFT;
		//	*(pn++) = VK_RIGHT;
		//	*(pn++) = VK_UP;
		//	*(pn++) = VK_DOWN;
		//}

		//if (gpSet->HasModifier(gpSet->vmCTSVkBlockStart, VK_LWIN))
		//	*(pn++) = gpSet->vmCTSVkBlockStart;
		//if (gpSet->HasModifier(gpSet->vmCTSVkTextStart, VK_LWIN))
		//	*(pn++) = gpSet->vmCTSVkTextStart;

		*pn = 0;
		_ASSERTE((pn - pnHookedKeys) < (HookedKeysMaxCount-1));
	}
}

void CSettings::InitVars_Pages()
{
	ConEmuSetupPages Pages[] = 
	{
		// При добавлении вкладки нужно добавить OnInitDialog_XXX в pageOpProc
		{IDD_SPG_MAIN,        0, L"Main",           thi_Main/*,    OnInitDialog_Main*/},
		{IDD_SPG_WNDSIZEPOS,  1, L"Size & Pos",     thi_SizePos/*, OnInitDialog_WndPosSize*/},
		{IDD_SPG_SHOW,  1, L"Appearance",     thi_Show/*, OnInitDialog_Show*/},
		{IDD_SPG_UPDATE,      1, L"Update",         thi_Update/*,  OnInitDialog_Update*/},
		{IDD_SPG_STARTUP,     0, L"Startup",        thi_Startup/*, OnInitDialog_Startup*/},
		{IDD_SPG_CMDTASKS,    1, L"Tasks",          thi_Tasks/*,   OnInitDialog_CmdTasks*/},
		{IDD_SPG_COMSPEC,     1, L"ComSpec",        thi_Comspec/*, OnInitDialog_Comspec*/},
		//{IDD_SPG_CMDOUTPUT, 1, L"Output",         thi_Output/*,  OnInitDialog_Output*/},
		{IDD_SPG_FEATURE,     0, L"Features",       thi_Ext/*,     OnInitDialog_Ext*/},
		{IDD_SPG_CURSOR,      1, L"Text cursor",    thi_Cursor/*,  OnInitDialog_Cursor*/},
		{IDD_SPG_COLORS,      1, L"Colors",         thi_Colors/*,  OnInitDialog_Color*/},
		{IDD_SPG_TRANSPARENT, 1, L"Transparency",   thi_Transparent/*, OnInitDialog_Transparent*/},
		{IDD_SPG_TABS,        1, L"Tabs",           thi_Tabs/*,    OnInitDialog_Tabs*/},
		{IDD_SPG_STATUSBAR,   1, L"Status bar",     thi_Status,/*  OnInitDialog_Status*/},
		{IDD_SPG_INTEGRATION, 1, L"Integration",    thi_Integr/*,  OnInitDialog_Integr*/},
		{IDD_SPG_APPDISTINCT, 1, L"App distinct",   thi_Apps/*,    OnInitDialog_CmdTasks*/},
		{IDD_SPG_KEYS,        0, L"Keys & Macro",   thi_Keys/*,    OnInitDialog_Keys*/},
		{IDD_SPG_CONTROL,  1, L"Controls",       thi_KeybMouse/*,OnInitDialog_Control*/},
		{IDD_SPG_SELECTION,   1, L"Mark & Paste",   thi_Selection/*OnInitDialog_Selection*/},
		{IDD_SPG_FEATURE_FAR, 0, L"Far Manager",    thi_Far/*,     OnInitDialog_Ext*/},
		{IDD_SPG_VIEWS,       1, L"Views",          thi_Views/*,   OnInitDialog_Views*/},
		{IDD_SPG_INFO,        0, L"Info",           thi_Info/*,    OnInitDialog_Info*/},
		{IDD_SPG_DEBUG,       1, L"Debug",          thi_Debug/*,   OnInitDialog_Debug*/},
		// End
		{},
	};
	#ifdef _DEBUG
	WARNING("LogFont.lfFaceName && LogFont2.lfFaceName, LogFont.lfHeight - subject to change");
	// !!! Main_Items пока вообще не используется !!!
	ConEmuSetupItem Main_Items[] =
	{
		{tFontFace, sit_FontsAndRaster, LogFont.lfFaceName, countof(LogFont.lfFaceName)},
		{tFontFace2, sit_Fonts, LogFont2.lfFaceName, countof(LogFont2.lfFaceName)},
		{tFontSizeY, sit_ULong, &CurFontSizeY, 0, sit_Byte, SettingsNS::FSizes+1, countof(SettingsNS::FSizes)-1},
		{tFontSizeX, sit_ULong, &gpSet->FontSizeX, 0, sit_Byte, SettingsNS::FSizes, countof(SettingsNS::FSizes)},
		{tFontSizeX2, sit_ULong, &gpSet->FontSizeX2, 0, sit_Byte, SettingsNS::FSizes, countof(SettingsNS::FSizes)},
		{tFontSizeX3, sit_ULong, &gpSet->FontSizeX3, 0, sit_Byte, SettingsNS::FSizes, countof(SettingsNS::FSizes)},
		{lbExtendFontBoldIdx, sit_Byte, &gpSet->AppStd.nFontBoldColor, 0, sit_FixString, SettingsNS::szColorIdx, countof(SettingsNS::szColorIdx)},
		{lbExtendFontItalicIdx, sit_Byte, &gpSet->AppStd.nFontItalicColor, 0, sit_FixString, SettingsNS::szColorIdx, countof(SettingsNS::szColorIdx)},
		{lbExtendFontNormalIdx, sit_Byte, &gpSet->AppStd.nFontNormalColor, 0, sit_FixString, SettingsNS::szColorIdx, countof(SettingsNS::szColorIdx)},
		{cbExtendFonts, sit_Bool, &gpSet->AppStd.isExtendFonts},
		// End
		{},
	};
	#endif

	m_Pages = (ConEmuSetupPages*)malloc(sizeof(Pages));
	memmove(m_Pages, Pages, sizeof(Pages));
	//ConEmuSetupPages* p = m_Pages;
	//for (int i = 0; i < countof(Pages); i++)
	//{
	//	// Если установлено как пакет MinGW - стандартное обновление через googlecode не работает.
	//	if ((Pages[i].PageID == IDD_SPG_UPDATE) && gpConEmu->mb_MingwMode)
	//		continue;
	//	*(p++) = Pages[i];
	//}
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
		mh_Font[i].Delete();

		if (m_otm[i]) {free(m_otm[i]); m_otm[i] = NULL;}
	}
	
	TODO("Очистить m_Fonts[Idx].hFonts");

	mh_Font2.Delete();
	
	//if (gpSet->psCmd) {free(gpSet->psCmd); gpSet->psCmd = NULL;}

	//if (gpSet->psCmdHistory) {free(gpSet->psCmdHistory); gpSet->psCmdHistory = NULL;}

	//if (gpSet->psCurCmd) {free(gpSet->psCurCmd); gpSet->psCurCmd = NULL;}

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
	SetEnvironmentVariable(ENV_CONEMUANSI_CONFIG_W, ConfigName);
}

void CSettings::SettingsLoaded()
{
	MCHKHEAP

	if (!SingleInstanceArg && gpSet->isQuakeStyle)
	{
		_ASSERTE(SingleInstanceShowHide == sih_None);
		SingleInstanceArg = true;
	}

	//wcscpy_c(gpSet->ComSpec.ConEmuDir, gpConEmu->ms_ConEmuDir);
	wcscpy_c(gpSet->ComSpec.ConEmuBaseDir, gpConEmu->ms_ConEmuBaseDir);
	UpdateComspec(&gpSet->ComSpec);
	// Обновить реестр на предмет поддержки UNC путей в cmd.exe
	if (gpSet->ComSpec.isAllowUncPaths)
	{
		UpdateComSpecUncSupport();
	}
	
	// Инициализация кастомной палитры для диалога выбора цвета
	memmove(acrCustClr, gpSet->Colors, sizeof(COLORREF)*16);

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
	// Т.к. вызывается из Settings::LoadSettings() то проверка на валидность уже не нужно, оставим только ассерт
	_ASSERTE(gpSet->_WindowMode == rNormal || gpSet->_WindowMode == rMaximized || gpSet->_WindowMode == rFullScreen);

	if (ghWnd == NULL)
	{
		gpConEmu->WindowMode = (ConEmuWindowMode)gpSet->_WindowMode;
	}
	else
	{
		gpConEmu->SetWindowMode((ConEmuWindowMode)gpSet->_WindowMode);
	}

	if (ghWnd == NULL)
	{
		gpConEmu->wndX = gpSet->_wndX;
		gpConEmu->wndY = gpSet->_wndY;
		gpConEmu->wndWidth = gpSet->_wndWidth;
		gpConEmu->wndHeight = gpSet->_wndHeight;
	}

	if (gpSet->wndCascade && (ghWnd == NULL))
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

		while (hPrev)
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

			gpConEmu->wndX = rcWnd.left + nShift;
			gpConEmu->wndY = rcWnd.top + nShift;
			break; // нашли, сдвинулись, выходим
		}
	}

	MCHKHEAP
}

void CSettings::SettingsPreSave()
{
	lstrcpyn(gpSet->inFont, LogFont.lfFaceName, countof(gpSet->inFont));
	lstrcpyn(gpSet->inFont2, LogFont2.lfFaceName, countof(gpSet->inFont2));
	#ifdef UPDATE_FONTSIZE_RECREATE
	gpSet->FontSizeY = LogFont.lfHeight;
	#endif
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

	ApplyStartupOptions();
}

void CSettings::ApplyStartupOptions()
{
	if (ghOpWnd && IsWindow(mh_Tabs[thi_Startup]))
	{
		//GetString(mh_Tabs[thi_Startup], tCmdLine, &gpSet->psStartSingleApp);
		ResetCmdArg();

		//TODO: пендюрки всякие, типа "Auto save/restore open tabs", "Far editor/viewer also"
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
	lstrcpyn(LogFont.lfFaceName, (asFontName && *asFontName) ? asFontName : (*gpSet->inFont) ? gpSet->inFont : gsDefGuiFont, countof(LogFont.lfFaceName));
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

	_ASSERTE(anQuality==-1 || anQuality==NONANTIALIASED_QUALITY || anQuality==ANTIALIASED_QUALITY || anQuality==CLEARTYPE_NATURAL_QUALITY);

	LogFont.lfQuality = (anQuality!=-1) ? anQuality : gpSet->mn_AntiAlias;
	LogFont.lfWeight = gpSet->isBold ? FW_BOLD : FW_NORMAL;
	LogFont.lfCharSet = gpSet->mn_LoadFontCharSet;
	LogFont.lfItalic = gpSet->isItalic;
	
	lstrcpyn(LogFont2.lfFaceName, (*gpSet->inFont2) ? gpSet->inFont2 : gsDefGuiFont, countof(LogFont2.lfFaceName));
	if (*gpSet->inFont2)
		mb_Name2Ok = TRUE;
	
	std::vector<RegFont>::iterator iter;

	if (!mb_Name1Ok)
	{
		for(int i = 0; !mb_Name1Ok && (i < 3); i++)
		{
			for (iter = m_RegFonts.begin(); iter != m_RegFonts.end(); ++iter)
			{
				switch (i)
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

	if (SendDlgItemMessage(gpSetCls->mh_Tabs[thi_Main], tFontFace, CB_FINDSTRINGEXACT, -1, (LPARAM) lplf->lfFaceName)==-1)
	{
		int nIdx;
		nIdx = SendDlgItemMessage(gpSetCls->mh_Tabs[thi_Main], tFontFace, CB_ADDSTRING, 0, (LPARAM) lplf->lfFaceName);
		SendDlgItemMessage(gpSetCls->mh_Tabs[thi_Main], tFontFace, CB_SETITEMDATA, nIdx, bAlmostMonospace);
		nIdx = SendDlgItemMessage(gpSetCls->mh_Tabs[thi_Main], tFontFace2, CB_ADDSTRING, 0, (LPARAM) lplf->lfFaceName);
		SendDlgItemMessage(gpSetCls->mh_Tabs[thi_Main], tFontFace2, CB_SETITEMDATA, nIdx, bAlmostMonospace);
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

	for (size_t sz=0; sz<countof(szRasterSizes) && szRasterSizes[sz].cy; sz++)
	{
		_wsprintf(szName, SKIPLEN(countof(szName)) L"[%s %ix%i]", RASTER_FONTS_NAME, szRasterSizes[sz].cx, szRasterSizes[sz].cy);
		int nIdx = SendDlgItemMessage(gpSetCls->mh_Tabs[thi_Main], tFontFace, CB_INSERTSTRING, sz, (LPARAM)szName);
		SendDlgItemMessage(gpSetCls->mh_Tabs[thi_Main], tFontFace, CB_SETITEMDATA, nIdx, 1);
	}

	GetDlgItemText(gpSetCls->mh_Tabs[thi_Main], tFontFace, szName, countof(szName));
	gpSetCls->SelectString(gpSetCls->mh_Tabs[thi_Main], tFontFace, szName);
	GetDlgItemText(gpSetCls->mh_Tabs[thi_Main], tFontFace2, szName, countof(szName));
	gpSetCls->SelectString(gpSetCls->mh_Tabs[thi_Main], tFontFace2, szName);
	SafeCloseHandle(gpSetCls->mh_EnumThread);
	_ASSERTE(gpSetCls->mh_EnumThread == NULL);

	// Если шустрый юзер успел переключиться на вкладку "Views" до оконачания
	// загрузки шрифтов - послать в диалог сообщение "Считать список из mh_Tabs[thi_Main]"
	if (ghOpWnd)
	{
		if (gpSetCls->mh_Tabs[thi_Views])
			PostMessage(gpSetCls->mh_Tabs[thi_Views], gpSetCls->mn_MsgLoadFontFromMain, 0, 0);
		if (gpSetCls->mh_Tabs[thi_Tabs])
			PostMessage(gpSetCls->mh_Tabs[thi_Tabs], gpSetCls->mn_MsgLoadFontFromMain, 0, 0);
		if (gpSetCls->mh_Tabs[thi_Status])
			PostMessage(gpSetCls->mh_Tabs[thi_Status], gpSetCls->mn_MsgLoadFontFromMain, 0, 0);
	}

	return 0;
}

LRESULT CSettings::OnInitDialog()
{
	//_ASSERTE(!hMain && !hColors && !hCmdTasks && !hViews && !hExt && !hFar && !hInfo && !hDebug && !hUpdate && !hSelection);
	//hMain = hExt = hFar = hTabs = hKeys = hViews = hColors = hCmdTasks = hInfo = hDebug = hUpdate = hSelection = NULL;
	_ASSERTE(!mh_Tabs[thi_Main] /*...*/);
	memset(mh_Tabs, 0, sizeof(mh_Tabs));

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

	wchar_t szType[8];
	bool ReadOnly = false;
	gpSet->GetSettingsType(szType, ReadOnly);
	if (ReadOnly)
		EnableWindow(GetDlgItem(ghOpWnd, bSaveSettings), FALSE); // Сохранение запрещено

	//if (nConfLen>(nStdLen+1))
	//	_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"%s Settings (%s) %s", gpConEmu->GetDefaultTitle(), (Config+nStdLen+1), szType);
	if (ConfigName[0])
		_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"%s Settings (%s) %s", gpConEmu->GetDefaultTitle(), ConfigName, szType);
	else
		_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"%s Settings %s", gpConEmu->GetDefaultTitle(), szType);

	SetWindowText(ghOpWnd, szTitle);
	MCHKHEAP
	{
		mb_IgnoreSelPage = true;
		TVINSERTSTRUCT ti = {TVI_ROOT, TVI_LAST, {{TVIF_TEXT}}};
		HWND hTree = GetDlgItem(ghOpWnd, tvSetupCategories);
		HTREEITEM hLastRoot = TVI_ROOT;
		bool bExpanded = false;
		for (size_t i = 0; m_Pages[i].PageID; i++)
		{
			if ((m_Pages[i].PageID == IDD_SPG_UPDATE) && gpConEmu->mb_MingwMode)
			{
				m_Pages[i].hTI = NULL;
				continue;
			}

			ti.hParent = m_Pages[i].Level ? hLastRoot : TVI_ROOT;
			ti.item.pszText = m_Pages[i].PageName;
			
			m_Pages[i].hTI = TreeView_InsertItem(hTree, &ti);
			
			_ASSERTE(mh_Tabs[m_Pages[i].PageIndex]==NULL);
			mh_Tabs[m_Pages[i].PageIndex] = NULL;

			if (!m_Pages[i].Level)
			{
				hLastRoot = m_Pages[i].hTI;
				bExpanded = false;
			}
			else if (!bExpanded)
			{
				TreeView_Expand(hTree, hLastRoot, TVE_EXPAND);
				bExpanded = true;
			}
		}

		HWND hPlace = GetDlgItem(ghOpWnd, tSetupPagePlace);
		//RECT rcClient; GetWindowRect(hPlace, &rcClient);
		//MapWindowPoints(NULL, ghOpWnd, (LPPOINT)&rcClient, 2);
		ShowWindow(hPlace, SW_HIDE);

		mb_IgnoreSelPage = false;

		mh_Tabs[m_Pages[0].PageIndex] = CreateDialogParam((HINSTANCE)GetModuleHandle(NULL),
			MAKEINTRESOURCE(m_Pages[0].PageID), ghOpWnd, pageOpProc, (LPARAM)&(m_Pages[0]));
		//MoveWindow(mh_Tabs[m_Pages[0].PageIndex], rcClient.left, rcClient.top, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top, 0);


		apiShowWindow(mh_Tabs[m_Pages[0].PageIndex], SW_SHOW);
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
	SetDlgItemText(mh_Tabs[thi_Main], tBgImageColors, tmp);
}

LRESULT CSettings::OnInitDialog_Main(HWND hWnd2)
{
	SetDlgItemText(hWnd2, tFontFace, LogFont.lfFaceName);
	SetDlgItemText(hWnd2, tFontFace2, LogFont2.lfFaceName);

	// Добавить шрифты рисованные ConEmu
	for (std::vector<RegFont>::iterator iter = m_RegFonts.begin(); iter != m_RegFonts.end(); ++iter)
	{
		if (iter->pCustom)
		{
			BOOL bMono = iter->pCustom->GetFont(0,0,0,0)->IsMonospace();

			int nIdx = SendDlgItemMessage(hWnd2, tFontFace, CB_ADDSTRING, 0, (LPARAM)iter->szFontName);
			SendDlgItemMessage(hWnd2, tFontFace, CB_SETITEMDATA, nIdx, bMono ? 1 : 0);

			nIdx = SendDlgItemMessage(hWnd2, tFontFace2, CB_ADDSTRING, 0, (LPARAM)iter->szFontName);
			SendDlgItemMessage(hWnd2, tFontFace2, CB_SETITEMDATA, nIdx, bMono ? 1 : 0);
		}
	}

	DWORD dwThId;
	mh_EnumThread = CreateThread(0,0,EnumFontsThread,0,0,&dwThId); // хэндл закроет сама нить
	{
		wchar_t temp[MAX_PATH];

		for (uint i=0; i < countof(SettingsNS::FSizes); i++)
		{
			_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", SettingsNS::FSizes[i]);

			if (i > 0)
				SendDlgItemMessage(hWnd2, tFontSizeY, CB_ADDSTRING, 0, (LPARAM) temp);

			SendDlgItemMessage(hWnd2, tFontSizeX, CB_ADDSTRING, 0, (LPARAM) temp);
			SendDlgItemMessage(hWnd2, tFontSizeX2, CB_ADDSTRING, 0, (LPARAM) temp);
			SendDlgItemMessage(hWnd2, tFontSizeX3, CB_ADDSTRING, 0, (LPARAM) temp);
		}

		SendMessage(GetDlgItem(hWnd2, lbExtendFontBoldIdx), CB_RESETCONTENT, 0, 0);
		SendMessage(GetDlgItem(hWnd2, lbExtendFontItalicIdx), CB_RESETCONTENT, 0, 0);
		SendMessage(GetDlgItem(hWnd2, lbExtendFontNormalIdx), CB_RESETCONTENT, 0, 0);
		for (uint i=0; i < countof(SettingsNS::szColorIdx); i++)
		{
			//_wsprintf(temp, SKIPLEN(countof(temp))(i==16) ? L"None" : L"%2i", i);
			SendDlgItemMessage(hWnd2, lbExtendFontBoldIdx, CB_ADDSTRING, 0, (LPARAM) SettingsNS::szColorIdx[i]);
			SendDlgItemMessage(hWnd2, lbExtendFontItalicIdx, CB_ADDSTRING, 0, (LPARAM) SettingsNS::szColorIdx[i]);
			SendDlgItemMessage(hWnd2, lbExtendFontNormalIdx, CB_ADDSTRING, 0, (LPARAM) SettingsNS::szColorIdx[i]);
		}

		checkDlgButton(hWnd2, cbExtendFonts, gpSet->AppStd.isExtendFonts);

		_wsprintf(temp, SKIPLEN(countof(temp))(gpSet->AppStd.nFontBoldColor<16) ? L"%2i" : L"None", gpSet->AppStd.nFontBoldColor);
		SelectStringExact(hWnd2, lbExtendFontBoldIdx, temp);
		_wsprintf(temp, SKIPLEN(countof(temp))(gpSet->AppStd.nFontItalicColor<16) ? L"%2i" : L"None", gpSet->AppStd.nFontItalicColor);
		SelectStringExact(hWnd2, lbExtendFontItalicIdx, temp);
		_wsprintf(temp, SKIPLEN(countof(temp))(gpSet->AppStd.nFontNormalColor<16) ? L"%2i" : L"None", gpSet->AppStd.nFontNormalColor);
		SelectStringExact(hWnd2, lbExtendFontNormalIdx, temp);

		checkDlgButton(hWnd2, cbFontAuto, gpSet->isFontAutoSize);

		_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", CurFontSizeY);
		//upToFontHeight = LogFont.lfHeight;
		SelectStringExact(hWnd2, tFontSizeY, temp);
		_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", gpSet->FontSizeX);
		SelectStringExact(hWnd2, tFontSizeX, temp);
		_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", gpSet->FontSizeX2);
		SelectStringExact(hWnd2, tFontSizeX2, temp);
		_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", gpSet->FontSizeX3);
		SelectStringExact(hWnd2, tFontSizeX3, temp);
	}

	FillListBoxCharSet(hWnd2, tFontCharset, LogFont.lfCharSet);

	//{
	//	_ASSERTE(countof(SettingsNS::nCharSets) == countof(SettingsNS::szCharSets));
	//	u8 num = 4; //-V112
	//	for (size_t i = 0; i < countof(SettingsNS::nCharSets); i++)
	//	{
	//		SendDlgItemMessageA(hWnd2, tFontCharset, CB_ADDSTRING, 0, (LPARAM) ChSets[i]);
	//		if (chSetsNums[i] == LogFont.lfCharSet) num = i;
	//	}
	//	SendDlgItemMessage(hWnd2, tFontCharset, CB_SETCURSEL, num, 0);
	//}
	
	MCHKHEAP
	SetDlgItemText(hWnd2, tBgImage, gpSet->sBgImage);
	//checkDlgButton(hWnd2, rBgSimple, BST_CHECKED);

	checkDlgButton(mh_Tabs[thi_Main], rbBgReplaceIndexes, BST_CHECKED);
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
	//SetDlgItemText(hWnd2, tBgImageColors, tmp);

	_wsprintf(tmp, SKIPLEN(countof(tmp)) L"%i", gpSet->bgImageDarker);
	SendDlgItemMessage(hWnd2, tDarker, EM_SETLIMITTEXT, 3, 0);
	SetDlgItemText(hWnd2, tDarker, tmp);
	SendDlgItemMessage(hWnd2, slDarker, TBM_SETRANGE, (WPARAM) true, (LPARAM) MAKELONG(0, 255));
	SendDlgItemMessage(hWnd2, slDarker, TBM_SETPOS  , (WPARAM) true, (LPARAM) gpSet->bgImageDarker);
	
	//checkDlgButton(hWnd2, rBgUpLeft+(UINT)gpSet->bgOperation, BST_CHECKED);
	BYTE b = gpSet->bgOperation;
	FillListBoxByte(hWnd2, lbBgPlacement, SettingsNS::szBgOper, SettingsNS::nBgOper, b);

	checkDlgButton(hWnd2, cbBgAllowPlugin, BST(gpSet->isBgPluginAllowed));

	checkDlgButton(hWnd2, cbBgImage, BST(gpSet->isShowBgImage));
	EnableWindow(GetDlgItem(hWnd2, tBgImage), gpSet->isShowBgImage);
	EnableWindow(GetDlgItem(hWnd2, bBgImage), gpSet->isShowBgImage);

	checkRadioButton(hWnd2, rNoneAA, rCTAA,
		(LogFont.lfQuality == CLEARTYPE_NATURAL_QUALITY) ? rCTAA :
		(LogFont.lfQuality == ANTIALIASED_QUALITY) ? rStandardAA : rNoneAA);


	// 3d state - force center symbols in cells
	checkDlgButton(hWnd2, cbMonospace, BST(gpSet->isMonospace));

	checkDlgButton(hWnd2, cbBold, (LogFont.lfWeight == FW_BOLD) ? BST_CHECKED : BST_UNCHECKED);

	checkDlgButton(hWnd2, cbItalic, LogFont.lfItalic ? BST_CHECKED : BST_UNCHECKED);

	checkDlgButton(hWnd2, cbFixFarBorders, BST(gpSet->isFixFarBorders));

	mn_LastChangingFontCtrlId = 0;
	RegisterTipsFor(hWnd2);
	return 0;
}

LRESULT CSettings::OnInitDialog_Show(HWND hWnd2, bool abInitial)
{
	checkDlgButton(hWnd2, cbMinToTray, gpSet->mb_MinToTray);
	EnableWindow(GetDlgItem(hWnd2, cbMinToTray), !gpConEmu->ForceMinimizeToTray);

	checkDlgButton(hWnd2, cbAlwaysShowTrayIcon, gpSet->isAlwaysShowTrayIcon);

	checkRadioButton(hWnd2, rbTaskbarBtnActive, rbTaskbarBtnHidden, 
		(gpSet->m_isTabsOnTaskBar == 3) ? rbTaskbarBtnHidden :
		(gpSet->m_isTabsOnTaskBar == 2) ? rbTaskbarBtnWin7 :
		(gpSet->m_isTabsOnTaskBar == 1) ? rbTaskbarBtnAll
		: rbTaskbarBtnActive);

	checkDlgButton(hWnd2, cbHideCaption, gpSet->isHideCaption);

	checkDlgButton(hWnd2, cbHideCaptionAlways, gpSet->isHideCaptionAlways());
	EnableWindow(GetDlgItem(hWnd2, cbHideCaptionAlways), !gpSet->isForcedHideCaptionAlways());

	SetDlgItemInt(hWnd2, tHideCaptionAlwaysFrame, gpSet->nHideCaptionAlwaysFrame, FALSE);
	SetDlgItemInt(hWnd2, tHideCaptionAlwaysDelay, gpSet->nHideCaptionAlwaysDelay, FALSE);
	SetDlgItemInt(hWnd2, tHideCaptionAlwaysDissapear, gpSet->nHideCaptionAlwaysDisappear, FALSE);

	checkDlgButton(hWnd2, cbEnhanceGraphics, gpSet->isEnhanceGraphics);
	
	//checkDlgButton(hWnd2, cbEnhanceButtons, gpSet->isEnhanceButtons);

	checkDlgButton(hWnd2, cbAlwaysShowScrollbar, gpSet->isAlwaysShowScrollbar);
	SetDlgItemInt(hWnd2, tScrollAppearDelay, gpSet->nScrollBarAppearDelay, FALSE);
	SetDlgItemInt(hWnd2, tScrollDisappearDelay, gpSet->nScrollBarDisappearDelay, FALSE);

	checkDlgButton(hWnd2, cbDesktopMode, gpSet->isDesktopMode);

	checkDlgButton(hWnd2, cbAlwaysOnTop, gpSet->isAlwaysOnTop);

	#ifdef _DEBUG
	checkDlgButton(hWnd2, cbTabsInCaption, gpSet->isTabsInCaption);
	#else
	ShowWindow(GetDlgItem(hWnd2, cbTabsInCaption), SW_HIDE);
	#endif

	checkDlgButton(hWnd2, cbNumberInCaption, gpSet->isNumberInCaption);

	RegisterTipsFor(hWnd2);
	return 0;
}

LRESULT CSettings::OnInitDialog_WndPosSize(HWND hWnd2, bool abInitial)
{
	_ASSERTE(mh_Tabs[thi_SizePos] == hWnd2);

	checkDlgButton(hWnd2, cbAutoSaveSizePos, gpSet->isAutoSaveSizePos);

	checkDlgButton(hWnd2, cbUseCurrentSizePos, gpSet->isUseCurrentSizePos);

	if (gpSet->isQuakeStyle || !gpSet->isUseCurrentSizePos)
		checkRadioButton(hWnd2, rNormal, rFullScreen, gpSet->_WindowMode);
	else if (gpConEmu->isFullScreen())
		checkRadioButton(hWnd2, rNormal, rFullScreen, rFullScreen);
	else if (gpConEmu->isZoomed())
		checkRadioButton(hWnd2, rNormal, rFullScreen, rMaximized);
	else
		checkRadioButton(hWnd2, rNormal, rFullScreen, rNormal);

	//swprintf_c(temp, L"%i", wndWidth);   SetDlgItemText(hWnd2, tWndWidth, temp);
	SendDlgItemMessage(hWnd2, tWndWidth, EM_SETLIMITTEXT, 3, 0);
	//swprintf_c(temp, L"%i", wndHeight);  SetDlgItemText(hWnd2, tWndHeight, temp);
	SendDlgItemMessage(hWnd2, tWndHeight, EM_SETLIMITTEXT, 3, 0);
	
	UpdateSize(gpConEmu->wndWidth, gpConEmu->wndHeight);

	EnableWindow(GetDlgItem(hWnd2, cbApplyPos), FALSE);
	SendDlgItemMessage(hWnd2, tWndX, EM_SETLIMITTEXT, 6, 0);
	SendDlgItemMessage(hWnd2, tWndY, EM_SETLIMITTEXT, 6, 0);
	MCHKHEAP

	UpdatePos(gpConEmu->wndX, gpConEmu->wndY, true);

	checkRadioButton(hWnd2, rCascade, rFixed, gpSet->wndCascade ? rCascade : rFixed);


	checkDlgButton(hWnd2, cbLongOutput, gpSet->AutoBufferHeight);
	TODO("Надо бы увеличить, но нужно сервер допиливать");
	SendDlgItemMessage(hWnd2, tLongOutputHeight, EM_SETLIMITTEXT, 4, 0);
	SetDlgItemInt(hWnd2, tLongOutputHeight, gpSet->DefaultBufferHeight, FALSE);
	//EnableWindow(GetDlgItem(hWnd2, tLongOutputHeight), gpSet->AutoBufferHeight);


	// 16bit Height
	if (abInitial)
	{
		SendDlgItemMessage(hWnd2, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"Auto");
		SendDlgItemMessage(hWnd2, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"25 lines");
		SendDlgItemMessage(hWnd2, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"28 lines");
		SendDlgItemMessage(hWnd2, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"43 lines");
		SendDlgItemMessage(hWnd2, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"50 lines");
	}
	SendDlgItemMessage(hWnd2, lbNtvdmHeight, CB_SETCURSEL, !gpSet->ntvdmHeight ? 0 :
	                   ((gpSet->ntvdmHeight == 25) ? 1 : ((gpSet->ntvdmHeight == 28) ? 2 : ((gpSet->ntvdmHeight == 43) ? 3 : 4))), 0); //-V112


	checkDlgButton(hWnd2, cbTryToCenter, gpSet->isTryToCenter);
	SetDlgItemInt(hWnd2, tPadSize, gpSet->nCenterConsolePad, FALSE);

	checkDlgButton(hWnd2, cbSnapToDesktopEdges, gpSet->isSnapToDesktopEdges);

	checkDlgButton(hWnd2, cbQuakeStyle, gpSet->isQuakeStyle ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hWnd2, cbQuakeAutoHide, (gpSet->isQuakeStyle == 2) ? BST_CHECKED : BST_UNCHECKED);
	EnableWindow(GetDlgItem(hWnd2, cbQuakeAutoHide), gpSet->isQuakeStyle);


	RegisterTipsFor(hWnd2);
	return 0;
}

LRESULT CSettings::OnInitDialog_Cursor(HWND hWnd2, BOOL abInitial)
{
	checkRadioButton(hWnd2, rCursorV, rCursorH, gpSet->AppStd.isCursorV ? rCursorV : rCursorH);

	checkDlgButton(hWnd2, cbCursorColor, gpSet->AppStd.isCursorColor);

	checkDlgButton(hWnd2, cbCursorBlink, gpSet->AppStd.isCursorBlink);

	checkDlgButton(hWnd2, cbBlockInactiveCursor, gpSet->AppStd.isCursorBlockInactive);

	checkDlgButton(hWnd2, cbCursorIgnoreSize, gpSet->AppStd.isCursorIgnoreSize);

	SetDlgItemInt(hWnd2, tCursorFixedSize, gpSet->AppStd.nCursorFixedSize, FALSE);
	SetDlgItemInt(hWnd2, tCursorMinSize, gpSet->AppStd.nCursorMinSize, FALSE);

	if (abInitial)
		RegisterTipsFor(hWnd2);
	return 0;
}

LRESULT CSettings::OnInitDialog_Startup(HWND hWnd2, BOOL abInitial)
{
	pageOpProc_Start(hWnd2, WM_INITDIALOG, 0, abInitial);

	if (abInitial)
		RegisterTipsFor(hWnd2);
	return 0;
}

INT_PTR CSettings::pageOpProc_Start(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR iRc = 0;
	const wchar_t* csNoTask = L"<None>";
	#define MSG_SHOWTASKCONTENTS (WM_USER+64)

	switch (messg)
	{
	case WM_INITDIALOG:
		{
			BOOL bInitial = (lParam != 0); UNREFERENCED_PARAMETER(bInitial);

			checkRadioButton(hWnd2, rbStartSingleApp, rbStartLastTabs, rbStartSingleApp+gpSet->nStartType);

			SetDlgItemText(hWnd2, tCmdLine, gpSet->psStartSingleApp ? gpSet->psStartSingleApp : L"");

			// Признак "командного файла" - лидирующая @, в диалоге - не показываем
			SetDlgItemText(hWnd2, tStartTasksFile, gpSet->psStartTasksFile ? (*gpSet->psStartTasksFile == CmdFilePrefix ? (gpSet->psStartTasksFile+1) : gpSet->psStartTasksFile) : L"");

			int nGroup = 0;
			const Settings::CommandTasks* pGrp = NULL;
			SendDlgItemMessage(hWnd2, lbStartNamedTask, CB_RESETCONTENT, 0,0);
			SendDlgItemMessage(hWnd2, lbStartNamedTask, CB_ADDSTRING, 0, (LPARAM)csNoTask);
			while ((pGrp = gpSet->CmdTaskGet(nGroup++)))
				SendDlgItemMessage(hWnd2, lbStartNamedTask, CB_ADDSTRING, 0, (LPARAM)pGrp->pszName);
			if (SelectStringExact(hWnd2, lbStartNamedTask, gpSet->psStartTasksName ? gpSet->psStartTasksName : L"") <= 0)
			{
				if (gpSet->nStartType == (rbStartNamedTask - rbStartSingleApp))
				{
					// 0 -- csNoTask
					// Задачи с таким именем больше нет - прыгаем на "Командную строку"
					gpSet->nStartType = 0;
					checkRadioButton(hWnd2, rbStartSingleApp, rbStartLastTabs, rbStartSingleApp);
				}
			}

			pageOpProc_Start(hWnd2, WM_COMMAND, rbStartSingleApp+gpSet->nStartType, 0);
		}
		break;
	case WM_COMMAND:
		{
			switch (HIWORD(wParam))
			{
			case BN_CLICKED:
				{
					WORD CB = LOWORD(wParam);
					switch (CB)
					{
					case rbStartSingleApp:
					case rbStartTasksFile:
					case rbStartNamedTask:
					case rbStartLastTabs:
						gpSet->nStartType = (CB - rbStartSingleApp);
						//
						EnableWindow(GetDlgItem(hWnd2, tCmdLine), (CB == rbStartSingleApp));
						EnableWindow(GetDlgItem(hWnd2, cbCmdLine), (CB == rbStartSingleApp));
						//
						EnableWindow(GetDlgItem(hWnd2, tStartTasksFile), (CB == rbStartTasksFile));
						EnableWindow(GetDlgItem(hWnd2, cbStartTasksFile), (CB == rbStartTasksFile));
						//
						EnableWindow(GetDlgItem(hWnd2, lbStartNamedTask), (CB == rbStartNamedTask));
						//
						EnableWindow(GetDlgItem(hWnd2, cbStartFarRestoreFolders), (CB == rbStartLastTabs));
						EnableWindow(GetDlgItem(hWnd2, cbStartFarRestoreEditors), (CB == rbStartLastTabs));
						// 
						EnableWindow(GetDlgItem(hWnd2, stCmdGroupCommands), (CB == rbStartNamedTask) || (CB == rbStartLastTabs));
						EnableWindow(GetDlgItem(hWnd2, tCmdGroupCommands), (CB == rbStartNamedTask) || (CB == rbStartLastTabs));
						// Task source
						pageOpProc_Start(hWnd2, MSG_SHOWTASKCONTENTS, CB, 0);
						break;
					case cbCmdLine:
					case cbStartTasksFile:
						{
							wchar_t temp[MAX_PATH+1], edt[MAX_PATH];
							if (!GetDlgItemText(hWnd2, (CB==cbCmdLine)?tCmdLine:tStartTasksFile, edt, countof(edt)))
								edt[0] = 0;
							ExpandEnvironmentStrings(edt, temp, countof(temp));
							OPENFILENAME ofn; memset(&ofn,0,sizeof(ofn));
							ofn.lStructSize=sizeof(ofn);
							ofn.hwndOwner = ghOpWnd;
							if (CB==cbCmdLine)
							{
								ofn.lpstrFilter = L"Executables (*.exe)\0*.exe\0All files (*.*)\0*.*\0\0";
								ofn.lpstrTitle = L"Choose application";
							}
							else
							{
								ofn.lpstrFilter = L"Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0\0";
								ofn.lpstrTitle = L"Choose command file";
							}
							ofn.lpstrFile = temp;
							ofn.nMaxFile = countof(temp);
							ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
										| OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST;

							if (GetOpenFileName(&ofn))
							{
								SetDlgItemText(hWnd2, (CB==cbCmdLine)?tCmdLine:tStartTasksFile, temp);
							}
						}
						break;
					}
				} // BN_CLICKED
				break;
			case EN_CHANGE:
				{
					switch (LOWORD(wParam))
					{
					case tCmdLine:
						GetString(hWnd2, tCmdLine, &gpSet->psStartSingleApp);
						break;
					case tStartTasksFile:
						{
							wchar_t* psz = NULL;
							INT_PTR nLen = GetString(hWnd2, tStartTasksFile, &psz);
							if ((nLen <= 0) || !psz || !*psz)
							{
								SafeFree(gpSet->psStartTasksFile);
							}
							else
							{
								LPCWSTR pszName = (*psz == CmdFilePrefix) ? (psz+1) : psz;
								SafeFree(gpSet->psStartTasksFile);
								gpSet->psStartTasksFile = (wchar_t*)calloc(nLen+2,sizeof(*gpSet->psStartTasksFile));
								*gpSet->psStartTasksFile = CmdFilePrefix;
								_wcscpy_c(gpSet->psStartTasksFile+1, nLen+1, pszName);
							}
							SafeFree(psz);
						}
						break;
					}
				} // EN_CHANGE
				break;
			case CBN_SELCHANGE:
				{
					switch (LOWORD(wParam))
					{
					case lbStartNamedTask:
						{
							wchar_t* pszName = NULL;
							GetSelectedString(hWnd2, lbStartNamedTask, &pszName);
							if (pszName)
							{
								if (lstrcmp(pszName, csNoTask) != 0)
								{
									SafeFree(gpSet->psStartTasksName);
									gpSet->psStartTasksName = pszName;
								}
							}
							else
							{
								SafeFree(pszName);
								// Задачи с таким именем больше нет - прыгаем на "Командную строку"
								gpSet->nStartType = 0;
								checkRadioButton(hWnd2, rbStartSingleApp, rbStartLastTabs, rbStartSingleApp);
								pageOpProc_Start(hWnd2, WM_COMMAND, rbStartSingleApp+gpSet->nStartType, 0);
							}

							// Показать содержимое задачи
							pageOpProc_Start(hWnd2, MSG_SHOWTASKCONTENTS, gpSet->nStartType+rbStartSingleApp, 0);
						}
						break;
					}
				}
				break;
			}
		} // WM_COMMAND
		break;
	case MSG_SHOWTASKCONTENTS:
		if ((wParam == rbStartLastTabs) || (wParam == rbStartNamedTask))
		{
			if ((gpSet->nStartType == (rbStartLastTabs-rbStartSingleApp)) || (gpSet->nStartType == (rbStartNamedTask-rbStartSingleApp)))
			{
				int nIdx = -2;
				if (gpSet->nStartType == (rbStartLastTabs-rbStartSingleApp))
				{
					nIdx = -1;
				}
				else
				{
					nIdx = SendDlgItemMessage(hWnd2, lbStartNamedTask, CB_GETCURSEL, 0, 0) - 1;
					if (nIdx == -1)
						nIdx = -2;
				}
				const Settings::CommandTasks* pTask = (nIdx >= -1) ? gpSet->CmdTaskGet(nIdx) : NULL;
				SetDlgItemText(hWnd2, tCmdGroupCommands, pTask ? pTask->pszCommands : L"");
			}
			else
			{
				SetDlgItemText(hWnd2, tCmdGroupCommands, L"");
			}
		}
		break;
	}

	return iRc;
}

LRESULT CSettings::OnInitDialog_Ext(HWND hWnd2)
{
	checkDlgButton(hWnd2, cbAutoRegFonts, gpSet->isAutoRegisterFonts);

	checkDlgButton(hWnd2, cbDebugSteps, gpSet->isDebugSteps);

	checkDlgButton(hWnd2, cbMonitorConsoleLang, gpSet->isMonitorConsoleLang ? BST_CHECKED : BST_UNCHECKED);

	//checkDlgButton(hWnd2, cbConsoleTextSelection, gpSet->isConsoleTextSelection);

	checkDlgButton(hWnd2, cbSleepInBackground, gpSet->isSleepInBackground);

	checkDlgButton(hWnd2, cbVisible, gpSet->isConVisible);

	checkDlgButton(hWnd2, cbUseInjects, gpSet->isUseInjects);

	checkDlgButton(hWnd2, cbProcessAnsi, gpSet->isProcessAnsi);

	checkDlgButton(hWnd2, cbUseClink, gpSet->isUseClink());

	checkDlgButton(hWnd2, cbDosBox, gpConEmu->mb_DosBoxExists);
	// изменение пока запрещено
	// или чтобы ругнуться, если DosBox не установлен
	EnableWindow(GetDlgItem(hWnd2, cbDosBox), !gpConEmu->mb_DosBoxExists);
	//EnableWindow(GetDlgItem(hWnd2, bDosBoxSettings), FALSE); // изменение пока запрещено
	ShowWindow(GetDlgItem(hWnd2, bDosBoxSettings), SW_HIDE);

	checkDlgButton(hWnd2, cbShowWasHiddenMsg, gpSet->isDownShowHiddenMessage ? BST_UNCHECKED : BST_CHECKED);

	checkDlgButton(hWnd2, cbDisableAllFlashing, gpSet->isDisableAllFlashing);


	#ifdef USEPORTABLEREGISTRY
	if (gpConEmu->mb_PortableRegExist)
	{
		checkDlgButton(hWnd2, cbPortableRegistry, gpSet->isPortableReg);
		EnableWindow(GetDlgItem(hWnd2, bPortableRegistrySettings), FALSE); // изменение пока запрещено
	}
	else
	#endif
	{
		EnableWindow(GetDlgItem(hWnd2, cbPortableRegistry), FALSE); // изменение пока запрещено
		EnableWindow(GetDlgItem(hWnd2, bPortableRegistrySettings), FALSE); // изменение пока запрещено
	}
	
	RegisterTipsFor(hWnd2);
	return 0;
}

void CSettings::UpdateComSpecUncSupport()
{
	// Обновить реестр на предмет поддержки UNC путей в cmd.exe
	SettingsRegistry UncChk;
	if (UncChk.OpenKey(HKEY_CURRENT_USER, L"Software\\Microsoft\\Command Processor", KEY_WRITE))
	{
		DWORD DisableUNCCheck = gpSet->ComSpec.isAllowUncPaths ? 1 : 0;
		UncChk.Save(L"DisableUNCCheck", (LPBYTE)&DisableUNCCheck, REG_DWORD, DisableUNCCheck);
	}
}

LRESULT CSettings::OnInitDialog_Comspec(HWND hWnd2, bool abInitial)
{
	_ASSERTE((rbComspecAuto+cst_Explicit)==rbComspecExplicit && (rbComspecAuto+cst_Cmd)==rbComspecCmd  && (rbComspecAuto+cst_EnvVar)==rbComspecEnvVar);
	checkRadioButton(hWnd2, rbComspecAuto, rbComspecExplicit, rbComspecAuto+gpSet->ComSpec.csType);

	SetDlgItemText(hWnd2, tComspecExplicit, gpSet->ComSpec.ComspecExplicit);
	SendDlgItemMessage(hWnd2, tComspecExplicit, EM_SETLIMITTEXT, countof(gpSet->ComSpec.ComspecExplicit)-1, 0);

	_ASSERTE((rbComspec_OSbit+csb_SameApp)==rbComspec_AppBit && (rbComspec_OSbit+csb_x32)==rbComspec_x32);
	checkRadioButton(hWnd2, rbComspec_OSbit, rbComspec_x32, rbComspec_OSbit+gpSet->ComSpec.csBits);

	checkDlgButton(hWnd2, cbComspecUpdateEnv, gpSet->ComSpec.isUpdateEnv ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hWnd2, cbAddConEmu2Path, gpSet->ComSpec.isAddConEmu2Path ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hWnd2, cbComspecUncPaths, gpSet->ComSpec.isAllowUncPaths ? BST_CHECKED : BST_UNCHECKED);

	// Cmd.exe output cp
	if (abInitial)
	{
		SendDlgItemMessage(hWnd2, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"Undefined");
		SendDlgItemMessage(hWnd2, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"Automatic");
		SendDlgItemMessage(hWnd2, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"Unicode (/U)");
		SendDlgItemMessage(hWnd2, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"OEM (/A)");
	}
	SendDlgItemMessage(hWnd2, lbCmdOutputCP, CB_SETCURSEL, gpSet->nCmdOutputCP, 0);

	RegisterTipsFor(hWnd2);
	return 0;
}

//LRESULT CSettings::OnInitDialog_Output(HWND hWnd2, bool abInitial)
//{
//	checkDlgButton(hWnd2, cbLongOutput, gpSet->AutoBufferHeight);
//	
//	wchar_t sz[16];
//	TODO("Надо бы увеличить, но нужно сервер допиливать");
//	SendDlgItemMessage(hWnd2, tLongOutputHeight, EM_SETLIMITTEXT, 5, 0);
//	SetDlgItemText(hWnd2, tLongOutputHeight, _ltow(gpSet->DefaultBufferHeight, sz, 10));
//	EnableWindow(GetDlgItem(hWnd2, tLongOutputHeight), gpSet->AutoBufferHeight);
//
//	// 16bit Height
//	if (abInitial)
//	{
//		SendDlgItemMessage(hWnd2, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"Auto");
//		SendDlgItemMessage(hWnd2, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"25 lines");
//		SendDlgItemMessage(hWnd2, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"28 lines");
//		SendDlgItemMessage(hWnd2, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"43 lines");
//		SendDlgItemMessage(hWnd2, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"50 lines");
//	}
//	SendDlgItemMessage(hWnd2, lbNtvdmHeight, CB_SETCURSEL, !gpSet->ntvdmHeight ? 0 :
//	                   ((gpSet->ntvdmHeight == 25) ? 1 : ((gpSet->ntvdmHeight == 28) ? 2 : ((gpSet->ntvdmHeight == 43) ? 3 : 4))), 0); //-V112
//	// Cmd.exe output cp
//	if (abInitial)
//	{
//		SendDlgItemMessage(hWnd2, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"Undefined");
//		SendDlgItemMessage(hWnd2, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"Automatic");
//		SendDlgItemMessage(hWnd2, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"Unicode");
//		SendDlgItemMessage(hWnd2, lbCmdOutputCP, CB_ADDSTRING, 0, (LPARAM) L"OEM");
//	}
//	SendDlgItemMessage(hWnd2, lbCmdOutputCP, CB_SETCURSEL, gpSet->nCmdOutputCP, 0);
//
//	RegisterTipsFor(hWnd2);
//	return 0;
//}

LRESULT CSettings::OnInitDialog_Selection(HWND hWnd2)
{
	checkRadioButton(hWnd2, rbCTSNever, rbCTSBufferOnly,
		(gpSet->isConsoleTextSelection == 0) ? rbCTSNever :
		(gpSet->isConsoleTextSelection == 1) ? rbCTSAlways : rbCTSBufferOnly);

	checkDlgButton(hWnd2, cbCTSAutoCopy, gpSet->isCTSAutoCopy);
	checkDlgButton(hWnd2, cbCTSEndOnTyping, gpSet->isCTSEndOnTyping);
	checkDlgButton(hWnd2, cbCTSEndOnKeyPress, (gpSet->isCTSEndOnTyping != 0) && gpSet->isCTSEndOnKeyPress);
	EnableWindow(GetDlgItem(hWnd2, cbCTSEndOnKeyPress), gpSet->isCTSEndOnTyping!=0);
	checkDlgButton(hWnd2, cbCTSFreezeBeforeSelect, gpSet->isCTSFreezeBeforeSelect);
	checkDlgButton(hWnd2, cbCTSBlockSelection, gpSet->isCTSSelectBlock);
	DWORD VkMod = gpSet->GetHotkeyById(vkCTSVkBlock);
	FillListBoxByte(hWnd2, lbCTSBlockSelection, SettingsNS::szKeysAct, SettingsNS::nKeysAct, VkMod);
	checkDlgButton(hWnd2, cbCTSTextSelection, gpSet->isCTSSelectText);
	VkMod = gpSet->GetHotkeyById(vkCTSVkText);
	FillListBoxByte(hWnd2, lbCTSTextSelection, SettingsNS::szKeysAct, SettingsNS::nKeysAct, VkMod);
	checkDlgButton(hWnd2, (gpSet->isCTSActMode==1)?rbCTSActAlways:rbCTSActBufferOnly, BST_CHECKED);
	VkMod = gpSet->GetHotkeyById(vkCTSVkAct);

	FillListBoxByte(hWnd2, lbCTSActAlways, SettingsNS::szKeysAct, SettingsNS::nKeysAct, VkMod);
	FillListBoxByte(hWnd2, lbCTSRBtnAction, SettingsNS::szClipAct, SettingsNS::nClipAct, gpSet->isCTSRBtnAction);
	FillListBoxByte(hWnd2, lbCTSMBtnAction, SettingsNS::szClipAct, SettingsNS::nClipAct, gpSet->isCTSMBtnAction);
	DWORD idxBack = (gpSet->isCTSColorIndex & 0xF0) >> 4;
	DWORD idxFore = (gpSet->isCTSColorIndex & 0xF);
	FillListBoxItems(GetDlgItem(hWnd2, lbCTSForeIdx), countof(SettingsNS::szColorIdx)-1,
		SettingsNS::szColorIdx, SettingsNS::nColorIdx, idxFore);
	FillListBoxItems(GetDlgItem(hWnd2, lbCTSBackIdx), countof(SettingsNS::szColorIdx)-1,
		SettingsNS::szColorIdx, SettingsNS::nColorIdx, idxBack);

	// Prompt click
	checkDlgButton(hWnd2, cbCTSClickPromptPosition, gpSet->AppStd.isCTSClickPromptPosition);
	VkMod = gpSet->GetHotkeyById(vkCTSVkPromptClk);
	FillListBoxByte(hWnd2, lbCTSClickPromptPosition, SettingsNS::szKeysAct, SettingsNS::nKeysAct, VkMod);

	checkDlgButton(hWnd2, cbCTSDetectLineEnd, gpSet->AppStd.isCTSDetectLineEnd);
	checkDlgButton(hWnd2, cbCTSBashMargin, gpSet->AppStd.isCTSBashMargin);
	checkDlgButton(hWnd2, cbCTSTrimTrailing, gpSet->AppStd.isCTSTrimTrailing);
	BYTE b = gpSet->AppStd.isCTSEOL;
	FillListBoxByte(hWnd2, lbCTSEOL, SettingsNS::szCRLF, SettingsNS::nCRLF, b);

	checkDlgButton(hWnd2, cbClipShiftIns, gpSet->AppStd.isPasteAllLines);
	checkDlgButton(hWnd2, cbClipCtrlV, gpSet->AppStd.isPasteFirstLine);
	checkDlgButton(hWnd2, cbClipConfirmEnter, gpSet->isPasteConfirmEnter);
	checkDlgButton(hWnd2, cbClipConfirmLimit, (gpSet->nPasteConfirmLonger!=0));
	SetDlgItemInt(hWnd2, tClipConfirmLimit, gpSet->nPasteConfirmLonger, FALSE);

	CheckSelectionModifiers();

	// тултипы
	RegisterTipsFor(hWnd2);
	return 0;
}

void CSettings::CheckSelectionModifiers()
{
	WORD nCtrlID[] = {lbCTSBlockSelection, lbCTSTextSelection, lbCTSClickPromptPosition/*, lbFarGotoEditorVk*/};
	bool bEnabled[] = {gpSet->isCTSSelectBlock, gpSet->isCTSSelectText, gpSet->AppStd.isCTSClickPromptPosition, gpSet->isFarGotoEditor};
	BYTE Vk[] = {0,0,0,0};
	LPCWSTR Descr[] = {L"Block selection", L"Text selection", L"Prompt position", L"Highlight and goto"};
	HWND hDlg = gpSetCls->mh_Tabs[thi_Selection];
	if (!hDlg)
		return;

	for (size_t i = 0; i < countof(nCtrlID); i++)
	{
		GetListBoxByte(hDlg, nCtrlID[i], SettingsNS::szKeysAct, SettingsNS::nKeysAct, Vk[i]);
	}

	for (size_t i = 0; i < (countof(nCtrlID)-1); i++)
	{
		if (!bEnabled[i])
			continue;

		for (size_t j = i+1; j < countof(nCtrlID); j++)
		{
			if (!bEnabled[j])
				continue;

			if (Vk[i] == Vk[j])
			{
				wchar_t szInfo[255];
				_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"You must set different\nmodifiers for\n<%s> and\n<%s>", Descr[i], Descr[j]);
				ShowErrorTip(szInfo, hDlg, nCtrlID[j], gpSetCls->szSelectionModError, countof(gpSetCls->szSelectionModError),
							 gpSetCls->hwndBalloon, &gpSetCls->tiBalloon, gpSetCls->hwndTip, FAILED_SELMOD_TIMEOUT, true);
				return;
			}
		}
	}
}

LRESULT CSettings::OnInitDialog_Far(HWND hWnd2, BOOL abInitial)
{
	#if 0
	if (gpSetCls->EnableThemeDialogTextureF)
		gpSetCls->EnableThemeDialogTextureF(hWnd2, 6/*ETDT_ENABLETAB*/);
	#endif

	// Сначала - то что обновляется при активации вкладки

	// Списки
	DWORD VkMod = gpSet->GetHotkeyById(vkLDragKey);
	FillListBoxByte(hWnd2, lbLDragKey, SettingsNS::szKeys, SettingsNS::nKeys, VkMod);
	VkMod = gpSet->GetHotkeyById(vkRDragKey);
	FillListBoxByte(hWnd2, lbRDragKey, SettingsNS::szKeys, SettingsNS::nKeys, VkMod);

	if (!abInitial)
		return 0;

	_ASSERTE(gpSet->isRClickSendKey==0 || gpSet->isRClickSendKey==1 || gpSet->isRClickSendKey==2);
	checkDlgButton(hWnd2, cbRClick, gpSet->isRClickSendKey);
	SetDlgItemText(hWnd2, tRClickMacro, gpSet->RClickMacro());

	checkDlgButton(hWnd2, cbSafeFarClose, gpSet->isSafeFarClose);
	SetDlgItemText(hWnd2, tSafeFarCloseMacro, gpSet->SafeFarCloseMacro());

	SetDlgItemText(hWnd2, tCloseTabMacro, gpSet->TabCloseMacro());
	
	SetDlgItemText(hWnd2, tSaveAllMacro, gpSet->SaveAllMacro());

	checkDlgButton(hWnd2, cbFARuseASCIIsort, gpSet->isFARuseASCIIsort);

	checkDlgButton(hWnd2, cbShellNoZoneCheck, gpSet->isShellNoZoneCheck);

	checkDlgButton(hWnd2, cbKeyBarRClick, gpSet->isKeyBarRClick);

	checkDlgButton(hWnd2, cbFarHourglass, gpSet->isFarHourglass);

	checkDlgButton(hWnd2, cbExtendUCharMap, gpSet->isExtendUCharMap);

	checkDlgButton(hWnd2, cbDragL, (gpSet->isDragEnabled & DRAG_L_ALLOWED) ? BST_CHECKED : BST_UNCHECKED);
    checkDlgButton(hWnd2, cbDragR, (gpSet->isDragEnabled & DRAG_R_ALLOWED) ? BST_CHECKED : BST_UNCHECKED);

    _ASSERTE(gpSet->isDropEnabled==0 || gpSet->isDropEnabled==1 || gpSet->isDropEnabled==2);
	checkDlgButton(hWnd2, cbDropEnabled, gpSet->isDropEnabled);

	checkDlgButton(hWnd2, cbDnDCopy, gpSet->isDefCopy);

	checkDlgButton(hWnd2, cbDropUseMenu, gpSet->isDropUseMenu);

	// Overlay
	checkDlgButton(hWnd2, cbDragImage, gpSet->isDragOverlay);

	checkDlgButton(hWnd2, cbDragIcons, gpSet->isDragShowIcons);

	checkDlgButton(hWnd2, cbRSelectionFix, gpSet->isRSelFix);

	checkDlgButton(hWnd2, cbDragPanel, gpSet->isDragPanel);
	checkDlgButton(hWnd2, cbDragPanelBothEdges, gpSet->isDragPanelBothEdges);

	_ASSERTE(gpSet->isDisableFarFlashing==0 || gpSet->isDisableFarFlashing==1 || gpSet->isDisableFarFlashing==2);
	checkDlgButton(hWnd2, cbDisableFarFlashing, gpSet->isDisableFarFlashing);

	RegisterTipsFor(hWnd2);
	return 0;
}

void CSettings::FillHotKeysList(HWND hWnd2, BOOL abInitial)
{
	if (!m_HotKeys)
	{
		_ASSERTE(m_HotKeys!=NULL);
		return;
	}

	static UINT nLastShowType = rbHotkeysAll;
	UINT nShowType = rbHotkeysAll;
	if (IsChecked(hWnd2, rbHotkeysUser))
		nShowType = rbHotkeysUser;
	else if (IsChecked(hWnd2, rbHotkeysMacros))
		nShowType = rbHotkeysMacros;
	else if (IsChecked(hWnd2, rbHotkeysSystem))
		nShowType = rbHotkeysSystem;

	// Населить список всеми хоткеями
	HWND hList = GetDlgItem(hWnd2, lbConEmuHotKeys);
	if (abInitial || (nLastShowType != nShowType))
	{
		ListView_DeleteAllItems(hList);
		abInitial = TRUE;
	}
	nLastShowType = nShowType;


	wchar_t szName[128], szDescr[512];
	//HWND hDetails = GetDlgItem(hWnd2, lbActivityDetails);
	LVITEM lvi = {LVIF_TEXT|LVIF_STATE|LVIF_PARAM};
	lvi.state = 0;
	lvi.stateMask = LVIS_SELECTED|LVIS_FOCUSED;
	lvi.pszText = szName;
	const ConEmuHotKey *ppHK = NULL;
	const ConEmuHotKey *ppNext = m_HotKeys;
	int ItemsCount = (int)ListView_GetItemCount(hList);
	int nItem = -1; // если -1 то будет добавлен новый
	//for (size_t i = 0; m_HotKeys[i].DescrLangID; i++)
	while (TRUE)
	{
		if (abInitial)
		{
			ppHK = ppNext; ppNext++;
			if (!ppHK->DescrLangID)
				break; // кончились
			nItem = -1; // если -1 то будет добавлен новый

			switch (nShowType)
			{
			case rbHotkeysUser:
				if ((ppHK->HkType != chk_User) && (ppHK->HkType != chk_Global)
					&& (ppHK->HkType != chk_Modifier) && (ppHK->HkType != chk_Modifier2))
					continue;
				break;
			case rbHotkeysMacros:
				if ((ppHK->HkType != chk_Macro) && !ppHK->GuiMacro)
					continue;
				break;
			case rbHotkeysSystem:
				if ((ppHK->HkType != chk_System) && (ppHK->HkType != chk_ArrHost) && (ppHK->HkType != chk_NumHost))
					continue;
				break;
			default:
				; // OK
			}
		}
		else
		{
			nItem++; // на старте было "-1"
			if (nItem >= ItemsCount)
				break; // кончились
			LVITEM lvf = {LVIF_PARAM, nItem};
			if (!ListView_GetItem(hList, &lvf))
			{
				_ASSERTE(ListView_GetItem(hList, &lvf));
				break;
			}
			ppHK = (const ConEmuHotKey*)lvf.lParam;
			if (!ppHK || !ppHK->DescrLangID)
			{
				_ASSERTE(ppHK && ppHK->DescrLangID);
				break;
			}
		}

		switch (ppHK->HkType)
		{
		case chk_Global:
			wcscpy_c(szName, L"Global"); break;
		case chk_User:
			wcscpy_c(szName, L"User"); break;
		case chk_Macro:
			_wsprintf(szName, SKIPLEN(countof(szName)) L"Macro %02i", ppHK->DescrLangID-vkGuMacro01+1); break;
		case chk_Modifier:
		case chk_Modifier2:
			wcscpy_c(szName, L"Modifier"); break;
		case chk_NumHost:
		case chk_ArrHost:
			wcscpy_c(szName, L"System"); break;
		case chk_System:
			wcscpy_c(szName, L"System"); break;
		default:
			// Неизвестный тип!
			_ASSERTE(ppHK->HkType == chk_User);
			//wcscpy_c(szName, L"???");
			continue;
		}
		
		if (nItem == -1)
		{
			lvi.iItem = ItemsCount + 1; // в конец
			lvi.lParam = (LPARAM)(m_HotKeys+ItemsCount);
			nItem = ListView_InsertItem(hList, &lvi);
			//_ASSERTE(nItem==ItemsCount && nItem>=0);
			ItemsCount++;
		}
		if (abInitial)
		{
			ListView_SetItemState(hList, nItem, 0, LVIS_SELECTED|LVIS_FOCUSED);
		}
		
		gpSet->GetHotkeyName(ppHK, szName);

		ListView_SetItemText(hList, nItem, klc_Hotkey, szName);
		
		if (ppHK->HkType == chk_Macro)
		{
			LPCWSTR pszMacro = ppHK->GuiMacro;
			if (!pszMacro || !*pszMacro)
				pszMacro = L"<Not set>";
			ListView_SetItemText(hList, nItem, klc_Desc, (wchar_t*)pszMacro);
		}
		else
		{
			ppHK->GetDescription(szDescr, countof(szDescr));
			//if (!LoadString(g_hInstance, ppHK->DescrLangID, szDescr, countof(szDescr)))
			//{
			//	if ((ppHK->HkType == chk_User) && ppHK->GuiMacro && *ppHK->GuiMacro)
			//		lstrcpyn(szDescr, ppHK->GuiMacro, countof(szDescr));
			//	else
			//		_wsprintf(szDescr, SKIPLEN(countof(szDescr)) L"%i", ppHK->DescrLangID);
			//}
			//else if ((ppHK->HkType == chk_User) && ppHK->GuiMacro && *ppHK->GuiMacro)
			//{
			//	int nLen = lstrlen(szDescr);
			//	if ((nLen + 64) < countof(szDescr))
			//	{
			//		wcscat_c(szDescr, L": ");
			//		nLen -= 2;
			//		lstrcpyn(szDescr + nLen, ppHK->GuiMacro, countof(szDescr) - nLen - 1);
			//	}
			//}
			ListView_SetItemText(hList, nItem, klc_Desc, szDescr);
		}
	}

	//ListView_SetSelectionMark(hList, -1);
	gpSet->CheckHotkeyUnique();
}

LRESULT CSettings::OnHotkeysNotify(HWND hWnd2, WPARAM wParam, LPARAM lParam)
{
	static bool bChangePosted = false;

	if (!lParam)
	{
		_ASSERTE(HIWORD(wParam) == 0xFFFF && LOWORD(wParam) == lbConEmuHotKeys);
		bChangePosted = false;

		HWND hHk = GetDlgItem(hWnd2, hkHotKeySelect);
		BOOL bHotKeyEnabled = FALSE, bKeyListEnabled = FALSE, bModifiersEnabled = FALSE, bMacroEnabled = FALSE;
		LPCWSTR pszLabel = L"Choose hotkey:";
		LPCWSTR pszDescription = L"";
		wchar_t szDescTemp[512];
		DWORD VkMod = 0;

		int iItem = (int)SendDlgItemMessage(hWnd2, lbConEmuHotKeys, LVM_GETNEXTITEM, -1, LVNI_SELECTED);

		if (iItem >= 0)
		{
			HWND hList = GetDlgItem(hWnd2, lbConEmuHotKeys);
			LVITEM lvi = {LVIF_PARAM, iItem};
			ConEmuHotKey* pk = NULL;
			if (ListView_GetItem(hList, &lvi))
				pk = (ConEmuHotKey*)lvi.lParam;
			if (pk && !(pk->DescrLangID /*&& (pk->VkMod || pk->HkType == chk_Macro)*/))
			{
				//_ASSERTE(pk->DescrLangID && (pk->VkMod || pk->HkType == chk_Macro));
				_ASSERTE(pk->DescrLangID);
				pk = NULL;
			}
			mp_ActiveHotKey = pk;

			if (pk)
			{
				//SetDlgItemText(hWnd2, stHotKeySelect, (pk->Type == 0) ? L"Choose hotkey:" : (pk->Type == 1) ?  L"Choose modifier:" : L"Choose modifiers:");
				switch (pk->HkType)
				{
				case chk_User:
				case chk_Global:
					pszLabel = L"Choose hotkey:";
					VkMod = pk->VkMod;
					bHotKeyEnabled = bKeyListEnabled = bModifiersEnabled = TRUE;
					break;
				case chk_Macro:
					pszLabel = L"Choose hotkey:";
					VkMod = pk->VkMod;
					bHotKeyEnabled = bKeyListEnabled = bModifiersEnabled = bMacroEnabled = TRUE;
					break;
				case chk_Modifier:
					pszLabel = L"Choose modifier:";
					VkMod = pk->VkMod;
					bKeyListEnabled = TRUE;
					break;
				case chk_Modifier2:
					pszLabel = L"Choose modifier:";
					VkMod = pk->VkMod;
					bModifiersEnabled = TRUE;
					break;
				case chk_NumHost:
				case chk_ArrHost:
					pszLabel = L"Choose modifiers:";
					_ASSERTE(pk->VkMod);
					VkMod = pk->VkMod;
					bModifiersEnabled = TRUE;
					break;
				case chk_System:
					pszLabel = L"Predefined:";
					VkMod = pk->VkMod;
					bHotKeyEnabled = bKeyListEnabled = bModifiersEnabled = 2;
					break;
				default:
					_ASSERTE(0 && "Unknown HkType");
					pszLabel = L"Unknown:";
				}
				//SetDlgItemText(hWnd2, stHotKeySelect, psz);

				//EnableWindow(GetDlgItem(hWnd2, stHotKeySelect), TRUE);
				//EnableWindow(GetDlgItem(hWnd2, lbHotKeyList), TRUE);
				//EnableWindow(hHk, (pk->Type == 0));

				if (bMacroEnabled)
				{
					pszDescription = pk->GuiMacro;
				}
				else
				{
					if (!LoadString(g_hInstance, pk->DescrLangID, szDescTemp, countof(szDescTemp)))
						szDescTemp[0] = 0;
					pszDescription = szDescTemp;
				}

				//nVK = pk ? *pk->VkPtr : 0;
				//if ((pk->Type == 0) || (pk->Type == 2))
				BYTE vk = gpSet->GetHotkey(VkMod);
				if (bHotKeyEnabled)
				{
					SendMessage(hHk, HKM_SETHOTKEY, 
						vk|((vk==VK_DELETE||vk==VK_UP||vk==VK_DOWN||vk==VK_LEFT||vk==VK_RIGHT
						||vk==VK_PRIOR||vk==VK_NEXT||vk==VK_HOME||vk==VK_END
						||vk==VK_INSERT) ? (HOTKEYF_EXT<<8) : 0), 0);
					// Warning! Если nVK не указан в SettingsNS::nKeysHot - nVK будет обнулен
					FillListBoxByte(hWnd2, lbHotKeyList, SettingsNS::szKeysHot, SettingsNS::nKeysHot, vk);
				}
				else if (bKeyListEnabled)
				{
					SendMessage(hHk, HKM_SETHOTKEY, 0, 0);
					FillListBoxByte(hWnd2, lbHotKeyList, SettingsNS::szKeys, SettingsNS::nKeys, vk);
				}
			}
		}
		else
		{
			mp_ActiveHotKey = NULL;
		}

		//if (!mp_ActiveHotKey)
		//{
		SetDlgItemText(hWnd2, stHotKeySelect, pszLabel);
		EnableWindow(GetDlgItem(hWnd2, stHotKeySelect), (bHotKeyEnabled || bKeyListEnabled || bModifiersEnabled));
		EnableWindow(GetDlgItem(hWnd2, lbHotKeyList), (bKeyListEnabled==TRUE));
		EnableWindow(hHk, (bHotKeyEnabled==TRUE));
		EnableWindow(GetDlgItem(hWnd2, stGuiMacro), (bMacroEnabled==TRUE));
		SetDlgItemText(hWnd2, stGuiMacro, bMacroEnabled ? L"GUI Macro:" : L"Description:");
		HWND hMacro = GetDlgItem(hWnd2, tGuiMacro);
		EnableWindow(hMacro, (mp_ActiveHotKey!=NULL));
		SendMessage(hMacro, EM_SETREADONLY, !bMacroEnabled, 0);
		MySetDlgItemText(hWnd2, tGuiMacro, pszDescription, bMacroEnabled);
		EnableWindow(GetDlgItem(hWnd2, cbGuiMacroHelp), (mp_ActiveHotKey!=NULL) && bMacroEnabled);
		if (!bHotKeyEnabled)
			SendMessage(hHk, HKM_SETHOTKEY, 0, 0);
		if (!bKeyListEnabled)
			SendDlgItemMessage(hWnd2, lbHotKeyList, CB_RESETCONTENT, 0, 0);
		// Modifiers
		BOOL bEnabled = (mp_ActiveHotKey && (bModifiersEnabled==TRUE));
		BOOL bShow = (mp_ActiveHotKey && (mp_ActiveHotKey->HkType != chk_Modifier));
		for (int n = 0; n < 3; n++)
		{
			BYTE b = (bShow && VkMod) ? gpSet->GetModifier(VkMod,n+1) : 0;
			FillListBoxByte(hWnd2, lbHotKeyMod1+n, SettingsNS::szModifiers, SettingsNS::nModifiers, b);
			EnableWindow(GetDlgItem(hWnd2, lbHotKeyMod1+n), bEnabled);
		}
		//for (size_t n = 0; n < countof(HostkeyCtrlIds); n++)
		//{
		//	BOOL bEnabled = (mp_ActiveHotKey && bModifiersEnabled);
		//	BOOL bChecked = bEnabled ? gpSet->HasModifier(VkMod, HostkeyVkIds[n]) : false;
		//	checkDlgButton(hWnd2, HostkeyCtrlIds[n], bChecked);
		//	EnableWindow(GetDlgItem(hWnd2, HostkeyCtrlIds[n]), bEnabled);
		//}
		//}
	}
	else switch (((NMHDR*)lParam)->code)
	{
	case LVN_ITEMCHANGED:
		{
			#ifdef _DEBUG
			LPNMLISTVIEW p = (LPNMLISTVIEW)lParam; UNREFERENCED_PARAMETER(p);
			#endif

			if (!bChangePosted)
			{
				bChangePosted = true;
				PostMessage(hWnd2, WM_COMMAND, MAKELONG(lbConEmuHotKeys,0xFFFF), 0);
			}
		} //LVN_ITEMCHANGED
		break;

	case LVN_COLUMNCLICK:
		{
			LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
			ListView_SortItems(GetDlgItem(hWnd2, lbConEmuHotKeys), HotkeysCompare, pnmv->iSubItem);
		} // LVN_COLUMNCLICK
		break;
	}
	return 0;
}

int CSettings::HotkeysCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	int nCmp = 0;
	ConEmuHotKey* pHk1 = (ConEmuHotKey*)lParam1;
	ConEmuHotKey* pHk2 = (ConEmuHotKey*)lParam2;

	if (pHk1 && pHk1->DescrLangID && pHk2 && pHk2->DescrLangID)
	{
		switch (lParamSort)
		{
		case 0:
			// Type
			nCmp =
				(pHk1->HkType < pHk2->HkType) ? -1 :
				(pHk1->HkType > pHk2->HkType) ? 1 :
				(pHk1 < pHk2) ? -1 :
				(pHk1 > pHk2) ? 1 :
				0;
			break;

		case 1:
			// Hotkey
			{
				wchar_t szFull1[128]; gpSet->GetHotkeyName(pHk1, szFull1);
				wchar_t szFull2[128]; gpSet->GetHotkeyName(pHk2, szFull2);
				nCmp = lstrcmp(szFull1, szFull2);
				if (nCmp == 0)
					nCmp = (pHk1 < pHk2) ? -1 : (pHk1 > pHk2) ? 1 : 0;
			}
			break;

		case 2:
			// Description
			{
				LPCWSTR pszDescr1, pszDescr2;
				wchar_t szBuf1[512], szBuf2[512];

				if (pHk1->HkType == chk_Macro)
				{
					pszDescr1 = (pHk1->GuiMacro && *pHk1->GuiMacro) ? pHk1->GuiMacro : L"<Not set>";
				}
				else
				{
					if (!LoadString(g_hInstance, pHk1->DescrLangID, szBuf1, countof(szBuf1)))
						_wsprintf(szBuf1, SKIPLEN(countof(szBuf1)) L"%i", pHk1->DescrLangID);
					pszDescr1 = szBuf1;
				}

				if (pHk2->HkType == chk_Macro)
				{
					pszDescr2 = (pHk2->GuiMacro && *pHk2->GuiMacro) ? pHk2->GuiMacro : L"<Not set>";
				}
				else
				{
					if (!LoadString(g_hInstance, pHk2->DescrLangID, szBuf2, countof(szBuf2)))
						_wsprintf(szBuf2, SKIPLEN(countof(szBuf2)) L"%i", pHk2->DescrLangID);
					pszDescr2 = szBuf2;
				}

				nCmp = lstrcmpi(pszDescr1, pszDescr2);
				if (nCmp == 0)
					nCmp = (pHk1 < pHk2) ? -1 : (pHk1 > pHk2) ? 1 : 0;
			}
			break;
		default:
			nCmp = (pHk1 < pHk2) ? -1 : (pHk1 > pHk2) ? 1 : 0;
		}
	}

	return nCmp;
}

LRESULT CSettings::OnInitDialog_Control(HWND hWnd2, BOOL abInitial)
{
	checkDlgButton(hWnd2, cbEnableMouse, !gpSet->isDisableMouse);
	checkDlgButton(hWnd2, cbSkipActivation, gpSet->isMouseSkipActivation);
	checkDlgButton(hWnd2, cbSkipMove, gpSet->isMouseSkipMoving);

	checkDlgButton(hWnd2, cbInstallKeybHooks,
	               (gpSet->m_isKeyboardHooks == 1) ? BST_CHECKED :
	               ((gpSet->m_isKeyboardHooks == 0) ? BST_INDETERMINATE : BST_UNCHECKED));
	
	checkDlgButton(hWnd2, cbUseWinNumber, gpSet->isUseWinNumber);
	checkDlgButton(hWnd2, cbUseWinTab, gpSet->isUseWinTab);
	checkDlgButton(hWnd2, cbUseWinArrows, gpSet->isUseWinArrows);

	checkDlgButton(hWnd2, cbSendAltTab, gpSet->isSendAltTab);
	checkDlgButton(hWnd2, cbSendAltEsc, gpSet->isSendAltEsc);
	checkDlgButton(hWnd2, cbSendAltPrintScrn, gpSet->isSendAltPrintScrn);
	checkDlgButton(hWnd2, cbSendPrintScrn, gpSet->isSendPrintScrn);
	checkDlgButton(hWnd2, cbSendCtrlEsc, gpSet->isSendCtrlEsc);

	checkDlgButton(hWnd2, cbFixAltOnAltTab, gpSet->isFixAltOnAltTab);

	checkDlgButton(hWnd2, cbSkipFocusEvents, gpSet->isSkipFocusEvents);

	// Не совсем выделение, но в том же диалоге
	checkDlgButton(hWnd2, cbFarGotoEditor, gpSet->isFarGotoEditor);
	DWORD VkMod = gpSet->GetHotkeyById(vkFarGotoEditorVk);
	FillListBoxByte(hWnd2, lbFarGotoEditorVk, SettingsNS::szKeysAct, SettingsNS::nKeysAct, VkMod);
	SetDlgItemText(hWnd2, tGotoEditorCmd, gpSet->sFarGotoEditor);

	RegisterTipsFor(hWnd2);
	return 0;
}

LRESULT CSettings::OnInitDialog_Keys(HWND hWnd2, BOOL abInitial)
{
	if (!abInitial)
	{
		FillHotKeysList(hWnd2, abInitial);
		return 0;
	}

	checkRadioButton(hWnd2, rbHotkeysAll, rbHotkeysMacros, rbHotkeysAll);

	for (int i = 0; m_HotKeys[i].DescrLangID; i++)
	{
		m_HotKeys[i].cchGuiMacroMax = m_HotKeys[i].GuiMacro ? (wcslen(m_HotKeys[i].GuiMacro)+1) : 0;
	}

	HWND hList = GetDlgItem(hWnd2, lbConEmuHotKeys);
	mp_ActiveHotKey = NULL;
	
	HWND hTip = ListView_GetToolTips(hList);
	SetWindowPos(hTip, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);

	// Убедиться, что поле клавиши идет поверх выпадающего списка
	//HWND hHk = GetDlgItem(hWnd2, hkHotKeySelect);
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

	FillHotKeysList(hWnd2, abInitial);
		
	BYTE b = 0;
	FillListBoxByte(hWnd2, lbHotKeyMod1, SettingsNS::szModifiers, SettingsNS::nModifiers, b);
	FillListBoxByte(hWnd2, lbHotKeyMod2, SettingsNS::szModifiers, SettingsNS::nModifiers, b);
	FillListBoxByte(hWnd2, lbHotKeyMod3, SettingsNS::szModifiers, SettingsNS::nModifiers, b);

	RegisterTipsFor(hWnd2);
	return 0;
}

LRESULT CSettings::OnInitDialog_Tabs(HWND hWnd2)
{
	checkDlgButton(hWnd2, cbMultiCon, gpSet->isMulti);
	checkDlgButton(hWnd2, cbNewConfirm, gpSet->isMultiNewConfirm);
	checkDlgButton(hWnd2, cbCloseConsoleConfirm, gpSet->isCloseConsoleConfirm);
	checkDlgButton(hWnd2, cbMultiLeaveOnClose, gpSet->isMultiLeaveOnClose);

	//checkDlgButton(hWnd2, cbTabs, gpSet->isTabs);
	checkRadioButton(hWnd2, rbTabsAlways, rbTabsNone, (gpSet->isTabs == 2) ? rbTabsAuto : gpSet->isTabs ? rbTabsAlways : rbTabsNone);

	checkDlgButton(hWnd2, cbTabsLocationBottom, (gpSet->nTabsLocation == 1) ? BST_CHECKED : BST_UNCHECKED);

	checkDlgButton(hWnd2, cbTabSelf, gpSet->isTabSelf);

	checkDlgButton(hWnd2, cbTabRecent, gpSet->isTabRecent);

	checkDlgButton(hWnd2, cbTabLazy, gpSet->isTabLazy);

	checkDlgButton(hWnd2, cbHideInactiveConTabs, gpSet->bHideInactiveConsoleTabs);
	checkDlgButton(hWnd2, cbHideDisabledTabs, gpSet->bHideDisabledTabs);
	checkDlgButton(hWnd2, cbShowFarWindows, gpSet->bShowFarWindows);

	SetDlgItemText(hWnd2, tTabFontFace, gpSet->sTabFontFace);

	if (gpSetCls->mh_EnumThread == NULL)  // Если шрифты уже считаны
		OnInitDialog_CopyFonts(hWnd2, tTabFontFace, 0); // можно скопировать список с вкладки mh_Tabs[thi_Main]

	DWORD nVal = gpSet->nTabFontHeight;
	FillListBoxInt(hWnd2, tTabFontHeight, SettingsNS::FSizesSmall, nVal);

	FillListBoxCharSet(hWnd2, tTabFontCharset, gpSet->nTabFontCharSet);

	checkDlgButton(hWnd2, cbMultiIterate, gpSet->isMultiIterate);


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
	//	checkDlgButton(hTabs, cbUseWinNumber, BST_CHECKED);

	SetDlgItemText(hWnd2, tTabConsole, gpSet->szTabConsole);
	SetDlgItemText(hWnd2, tTabSkipWords, gpSet->szTabSkipWords);
	SetDlgItemText(hWnd2, tTabPanels, gpSet->szTabPanels);
	SetDlgItemText(hWnd2, tTabViewer, gpSet->szTabViewer);
	SetDlgItemText(hWnd2, tTabEditor, gpSet->szTabEditor);
	SetDlgItemText(hWnd2, tTabEditorMod, gpSet->szTabEditorModified);
	SetDlgItemInt(hWnd2, tTabLenMax, gpSet->nTabLenMax, FALSE);

	checkRadioButton(hWnd2, rbAdminShield, rbAdminSuffix, gpSet->bAdminShield ? rbAdminShield : rbAdminSuffix);
	SetDlgItemText(hWnd2, tAdminSuffix, gpSet->szAdminTitleSuffix);

	RegisterTipsFor(hWnd2);
	return 0;
}

LRESULT CSettings::OnInitDialog_Status(HWND hWnd2, bool abInitial)
{
	SetDlgItemText(hWnd2, tStatusFontFace, gpSet->sStatusFontFace);

	if (gpSetCls->mh_EnumThread == NULL)  // Если шрифты уже считаны
		OnInitDialog_CopyFonts(hWnd2, tStatusFontFace, 0); // можно скопировать список с вкладки mh_Tabs[thi_Main]

	DWORD nVal = gpSet->nStatusFontHeight;
	FillListBoxInt(hWnd2, tStatusFontHeight, SettingsNS::FSizesSmall, nVal);

	FillListBoxCharSet(hWnd2, tStatusFontCharset, gpSet->nStatusFontCharSet);

	// Colors
	for (uint c = c35; c <= c37; c++)
		ColorSetEdit(hWnd2, c);

	checkDlgButton(hWnd2, cbStatusVertSep, (gpSet->isStatusBarFlags & csf_VertDelim) ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hWnd2, cbStatusHorzSep, (gpSet->isStatusBarFlags & csf_HorzDelim) ? BST_CHECKED : BST_UNCHECKED);

	checkDlgButton(hWnd2, cbShowStatusBar, gpSet->isStatusBarShow);
	for (size_t i = 0; i < countof(SettingsNS::StatusItems); i++)
	{
		checkDlgButton(hWnd2, SettingsNS::StatusItems[i].nDlgID, !gpSet->isStatusColumnHidden[SettingsNS::StatusItems[i].stItem]);
	}

	RegisterTipsFor(hWnd2);
	return 0;
}

void CSettings::UpdateTextColorSettings(BOOL ChangeTextAttr /*= TRUE*/, BOOL ChangePopupAttr /*= TRUE*/)
{
	// Обновить палитры
	gpSet->PaletteSetStdIndexes();

	// Обновить консоли
	gpConEmu->UpdateTextColorSettings(ChangeTextAttr, ChangePopupAttr);
}

LRESULT CSettings::OnInitDialog_Color(HWND hWnd2)
{
	#if 0
	if (gpSetCls->EnableThemeDialogTextureF)
		gpSetCls->EnableThemeDialogTextureF(hWnd2, 6/*ETDT_ENABLETAB*/);
	#endif

#define getR(inColorref) (byte)inColorref
#define getG(inColorref) (byte)(inColorref >> 8)
#define getB(inColorref) (byte)(inColorref >> 16)

	//wchar_t temp[MAX_PATH];
	DWORD nVal;

	for(uint c = c0; c <= MAX_COLOR_EDT_ID; c++)
		ColorSetEdit(hWnd2, c);

	nVal = gpSet->AppStd.nTextColorIdx;
	FillListBox(hWnd2, lbConClrText, SettingsNS::szColorIdxTh, SettingsNS::nColorIdxTh, nVal);
	nVal = gpSet->AppStd.nBackColorIdx;
	FillListBox(hWnd2, lbConClrBack, SettingsNS::szColorIdxTh, SettingsNS::nColorIdxTh, nVal);
	nVal = gpSet->AppStd.nPopTextColorIdx;
	FillListBox(hWnd2, lbConClrPopText, SettingsNS::szColorIdxTh, SettingsNS::nColorIdxTh, nVal);
	nVal = gpSet->AppStd.nPopBackColorIdx;
	FillListBox(hWnd2, lbConClrPopBack, SettingsNS::szColorIdxTh, SettingsNS::nColorIdxTh, nVal);

	//WARNING("Отладка...");
	//if (gpSet->AppStd.nPopTextColorIdx <= 15 || gpSet->AppStd.nPopBackColorIdx <= 15
	//	|| RELEASEDEBUGTEST(FALSE,TRUE))
	//{
	//	EnableWindow(GetDlgItem(hWnd2, lbConClrPopText), TRUE);
	//	EnableWindow(GetDlgItem(hWnd2, lbConClrPopBack), TRUE);
	//}

	nVal = gpSet->AppStd.nExtendColorIdx;
	FillListBox(hWnd2, lbExtendIdx, SettingsNS::szColorIdxSh, SettingsNS::nColorIdxSh, nVal);
	gpSet->AppStd.nExtendColorIdx = nVal;
	checkDlgButton(hWnd2, cbExtendColors, gpSet->AppStd.isExtendColors ? BST_CHECKED : BST_UNCHECKED);
	OnButtonClicked(hWnd2, cbExtendColors, 0);
	checkDlgButton(hWnd2, cbTrueColorer, gpSet->isTrueColorer ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hWnd2, cbFadeInactive, gpSet->isFadeInactive ? BST_CHECKED : BST_UNCHECKED);
	SetDlgItemInt(hWnd2, tFadeLow, gpSet->mn_FadeLow, FALSE);
	SetDlgItemInt(hWnd2, tFadeHigh, gpSet->mn_FadeHigh, FALSE);

	// Palette
	const Settings::ColorPalette* pPal;

	// Default colors
	//memmove(gdwLastColors, gpSet->Colors, sizeof(gdwLastColors));
	if ((pPal = gpSet->PaletteGet(-1)) != NULL)
	{
		memmove(&gLastColors, pPal, sizeof(gLastColors));
		if (gLastColors.pszName == NULL)
		{
			_ASSERTE(gLastColors.pszName!=NULL);
			gLastColors.pszName = (wchar_t*)L"<Current color scheme>";
		}
	}
	else
	{
		EnableWindow(hWnd2, FALSE);
		MBoxAssert(pPal && "PaletteGet(-1) failed");
		return 0;
	}

	INT_PTR iCurPalette = 0;
	bool bBtnEnabled = false;

	SendMessage(GetDlgItem(hWnd2, lbDefaultColors), CB_RESETCONTENT, 0, 0);
	SendDlgItemMessage(hWnd2, lbDefaultColors, CB_ADDSTRING, 0, (LPARAM)gLastColors.pszName);

	for (int i = 0; (pPal = gpSet->PaletteGet(i)) != NULL; i++)
	{
		SendDlgItemMessage(hWnd2, lbDefaultColors, CB_ADDSTRING, 0, (LPARAM)pPal->pszName);

		if (/*(iCurPalette == 0) -- покажем последнюю, чтобы приоритет пользовательским отдать
			&&*/ (memcmp(gLastColors.Colors, pPal->Colors, sizeof(pPal->Colors)) == 0)
			&& (gLastColors.isExtendColors == pPal->isExtendColors)
			&& (gLastColors.nExtendColorIdx == pPal->nExtendColorIdx))
		{
			iCurPalette = i+1;
			bBtnEnabled = !pPal->bPredefined;
		}
	}

	SendDlgItemMessage(hWnd2, lbDefaultColors, CB_SETCURSEL, iCurPalette, 0);

	EnableWindow(GetDlgItem(hWnd2, cbColorSchemeSave), bBtnEnabled);
	EnableWindow(GetDlgItem(hWnd2, cbColorSchemeDelete), bBtnEnabled);

	gbLastColorsOk = TRUE;

	RegisterTipsFor(hWnd2);
	return 0;
}

LRESULT CSettings::OnInitDialog_Transparent(HWND hWnd2)
{
	// +Color key
	ColorSetEdit(hWnd2, c38);

	checkDlgButton(hWnd2, cbTransparent, (gpSet->nTransparent!=255) ? BST_CHECKED : BST_UNCHECKED);
	SendDlgItemMessage(hWnd2, slTransparent, TBM_SETRANGE, (WPARAM) true, (LPARAM) MAKELONG(MIN_ALPHA_VALUE, 255));
	SendDlgItemMessage(hWnd2, slTransparent, TBM_SETPOS  , (WPARAM) true, (LPARAM) gpSet->nTransparent);
	checkDlgButton(hWnd2, cbTransparentSeparate, gpSet->isTransparentSeparate ? BST_CHECKED : BST_UNCHECKED);
	//EnableWindow(GetDlgItem(hWnd2, cbTransparentInactive), gpSet->isTransparentSeparate);
	//checkDlgButton(hWnd2, cbTransparentInactive, (gpSet->nTransparentInactive!=255) ? BST_CHECKED : BST_UNCHECKED);
	EnableWindow(GetDlgItem(hWnd2, slTransparentInactive), gpSet->isTransparentSeparate);
	EnableWindow(GetDlgItem(hWnd2, stTransparentInactive1), gpSet->isTransparentSeparate);
	EnableWindow(GetDlgItem(hWnd2, stTransparentInactive2), gpSet->isTransparentSeparate);
	SendDlgItemMessage(hWnd2, slTransparentInactive, TBM_SETRANGE, (WPARAM) true, (LPARAM) MAKELONG(MIN_INACTIVE_ALPHA_VALUE, 255));
	SendDlgItemMessage(hWnd2, slTransparentInactive, TBM_SETPOS  , (WPARAM) true, (LPARAM) gpSet->isTransparentSeparate ? gpSet->nTransparentInactive : gpSet->nTransparent);
	checkDlgButton(hWnd2, cbUserScreenTransparent, gpSet->isUserScreenTransparent ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hWnd2, cbColorKeyTransparent, gpSet->isColorKeyTransparent);

	RegisterTipsFor(hWnd2);
	return 0;
}

LRESULT CSettings::OnInitDialog_Tasks(HWND hWnd2, bool abForceReload)
{
	mb_IgnoreCmdGroupEdit = true;

	if (abForceReload)
	{
		int nTab = 4*4; // represent the number of quarters of the average character width for the font
		SendDlgItemMessage(hWnd2, lbCmdTasks, LB_SETTABSTOPS, 1, (LPARAM)&nTab);

		LONG_PTR nStyles = GetWindowLongPtr(GetDlgItem(hWnd2, lbCmdTasks), GWL_STYLE);
		if (!(nStyles & LBS_NOTIFY))
			SetWindowLongPtr(GetDlgItem(hWnd2, lbCmdTasks), GWL_STYLE, nStyles|LBS_NOTIFY);
	}

	// Сброс ранее загруженного списка (ListBox: lbCmdTasks)
	SendDlgItemMessage(hWnd2, lbCmdTasks, LB_RESETCONTENT, 0,0);

	//if (abForceReload)
	//{
	//	// Обновить группы команд
	//	gpSet->LoadCmdTasks(NULL, true);
	//}

	int nGroup = 0;
	wchar_t szItem[1024];
	const Settings::CommandTasks* pGrp = NULL;
	while ((pGrp = gpSet->CmdTaskGet(nGroup)))
	{
		_wsprintf(szItem, SKIPLEN(countof(szItem)) L"%i\t", nGroup+1);
		int nPrefix = lstrlen(szItem);
		lstrcpyn(szItem+nPrefix, pGrp->pszName, countof(szItem)-nPrefix);

		INT_PTR iIndex = SendDlgItemMessage(hWnd2, lbCmdTasks, LB_ADDSTRING, 0, (LPARAM)szItem);
		UNREFERENCED_PARAMETER(iIndex);
		//SendDlgItemMessage(hWnd2, lbCmdTasks, LB_SETITEMDATA, iIndex, (LPARAM)pGrp);

		nGroup++;
	}

	OnComboBox(hWnd2, MAKELONG(lbCmdTasks,LBN_SELCHANGE), 0);

	mb_IgnoreCmdGroupEdit = false;

	checkDlgButton(hWnd2, cbCmdTaskbarTasks, gpSet->isStoreTaskbarkTasks);
	checkDlgButton(hWnd2, cbCmdTaskbarCommands, gpSet->isStoreTaskbarCommands);
	EnableWindow(GetDlgItem(hWnd2, cbCmdTaskbarUpdate), (gnOsVer >= 0x601));

	RegisterTipsFor(hWnd2);
	return 0;
}

LRESULT CSettings::OnInitDialog_Apps(HWND hWnd2, bool abForceReload)
{
	//mn_AppsEnableControlsMsg = RegisterWindowMessage(L"ConEmu::AppsEnableControls");

	if (abForceReload)
	{
		int nTab[2] = {4*4, 7*4}; // represent the number of quarters of the average character width for the font
		SendDlgItemMessage(hWnd2, lbAppDistinct, LB_SETTABSTOPS, countof(nTab), (LPARAM)nTab);

		LONG_PTR nStyles = GetWindowLongPtr(GetDlgItem(hWnd2, lbAppDistinct), GWL_STYLE);
		if (!(nStyles & LBS_NOTIFY))
			SetWindowLongPtr(GetDlgItem(hWnd2, lbAppDistinct), GWL_STYLE, nStyles|LBS_NOTIFY);
	}
	
	pageOpProc_Apps(hWnd2, NULL, abForceReload ? WM_INITDIALOG : mn_ActivateTabMsg, 0, 0);

	if (abForceReload)
	{
		RegisterTipsFor(hWnd2);
	}
	return 0;
}

LRESULT CSettings::OnInitDialog_Integr(HWND hWnd2, BOOL abInitial)
{
	pageOpProc_Integr(hWnd2, WM_INITDIALOG, 0, abInitial);

	if (abInitial)
		RegisterTipsFor(hWnd2);
	return 0;
}

INT_PTR CSettings::pageOpProc_Integr(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	static bool bSkipCbSel = FALSE;
	INT_PTR iRc = 0;
	#define UM_RELOAD_HERE_LIST (WM_USER+32)
	#define UM_RELOAD_AUTORUN   (WM_USER+33)

	switch (messg)
	{
	case WM_INITDIALOG:
		{
			bSkipCbSel = true;

			pageOpProc_Integr(hWnd2, UM_RELOAD_HERE_LIST, UM_RELOAD_HERE_LIST, 0);
			pageOpProc_Integr(hWnd2, UM_RELOAD_AUTORUN, UM_RELOAD_AUTORUN, 0);

			// Возвращает NULL, если строка пустая
			wchar_t* pszCurInside = GetDlgItemText(hWnd2, cbInsideName);
			_ASSERTE((pszCurInside==NULL) || (*pszCurInside!=0));
			wchar_t* pszCurHere   = GetDlgItemText(hWnd2, cbHereName);
			_ASSERTE((pszCurHere==NULL) || (*pszCurHere!=0));

			wchar_t szIcon[MAX_PATH+32];
			_wsprintf(szIcon, SKIPLEN(countof(szIcon)) L"%s,0", gpConEmu->ms_ConEmuExe);

			if (pszCurInside)
			{
				bSkipCbSel = false;
				pageOpProc_Integr(hWnd2, WM_COMMAND, MAKELONG(cbInsideName,CBN_SELCHANGE), 0);
				bSkipCbSel = true;
			}
			else
			{
				SetDlgItemText(hWnd2, cbInsideName, L"ConEmu Inside");
				SetDlgItemText(hWnd2, tInsideConfig, L"shell");
				SetDlgItemText(hWnd2, tInsideShell, L"powershell -cur_console:n");
				//SetDlgItemText(hWnd2, tInsideIcon, szIcon);
				SetDlgItemText(hWnd2, tInsideIcon, L"powershell.exe");
				checkDlgButton(hWnd2, cbInsideSyncDir, gpConEmu->mb_InsideSynchronizeCurDir);
				SetDlgItemText(hWnd2, tInsideSyncDir, L""); // Auto
			}

			if (pszCurHere)
			{
				bSkipCbSel = false;
				pageOpProc_Integr(hWnd2, WM_COMMAND, MAKELONG(cbHereName,CBN_SELCHANGE), 0);
				bSkipCbSel = true;
			}
			else
			{
				SetDlgItemText(hWnd2, cbHereName, L"ConEmu Here");
				SetDlgItemText(hWnd2, tHereConfig, L"");
				SetDlgItemText(hWnd2, tHereShell, L"cmd -cur_console:n");
				SetDlgItemText(hWnd2, tHereIcon, szIcon);
			}

			bSkipCbSel = false;
		}
		break; // WM_INITDIALOG

	case WM_COMMAND:
		switch (HIWORD(wParam))
		{
		case BN_CLICKED:
			{
				WORD CB = LOWORD(wParam);
				switch (CB)
				{
				case cbInsideSyncDir:
					gpConEmu->mb_InsideSynchronizeCurDir = IsChecked(hWnd2, CB);
					break;
				case bInsideRegister:
				case bInsideUnregister:
					ShellIntegration(hWnd2, ShellIntgr_Inside, CB==bInsideRegister);
					pageOpProc_Integr(hWnd2, UM_RELOAD_HERE_LIST, UM_RELOAD_HERE_LIST, 0);
					break;
				case bHereRegister:
				case bHereUnregister:
					ShellIntegration(hWnd2, ShellIntgr_Here, CB==bHereRegister);
					pageOpProc_Integr(hWnd2, UM_RELOAD_HERE_LIST, UM_RELOAD_HERE_LIST, 0);
					break;
				case bCmdAutoRegister:
				case bCmdAutoUnregister:
				case bCmdAutoClear:
					ShellIntegration(hWnd2, ShellIntgr_CmdAuto, CB==bCmdAutoRegister, CB==bCmdAutoClear);
					pageOpProc_Integr(hWnd2, UM_RELOAD_AUTORUN, UM_RELOAD_AUTORUN, 0);
					break;
				}
			}
			break; // BN_CLICKED
		case EN_CHANGE:
			{
				WORD EB = LOWORD(wParam);
				switch (EB)
				{
				case tInsideSyncDir:
					if (gpConEmu->m_InsideIntegration)
					{
						SafeFree(gpConEmu->ms_InsideSynchronizeCurDir);
                        gpConEmu->ms_InsideSynchronizeCurDir = GetDlgItemText(hWnd2, tInsideSyncDir);
					}
					break;
				}
			}
			break; // EN_CHANGE
		case CBN_SELCHANGE:
			{
				WORD CB = LOWORD(wParam);
				switch (CB)
				{
				case cbInsideName:
				case cbHereName:
					if (!bSkipCbSel)
					{
						wchar_t *pszCfg = NULL, *pszIco = NULL, *pszFull = NULL, *pszDirSync = NULL;
						LPCWSTR pszCmd = NULL;
						INT_PTR iSel = SendDlgItemMessage(hWnd2, CB, CB_GETCURSEL, 0,0);
						if (iSel >= 0)
						{
							INT_PTR iLen = SendDlgItemMessage(hWnd2, CB, CB_GETLBTEXTLEN, iSel, 0);
							size_t cchMax = iLen+128;
							wchar_t* pszName = (wchar_t*)calloc(cchMax,sizeof(*pszName));
							if ((iLen > 0) && pszName)
							{
								_wcscpy_c(pszName, cchMax, L"Directory\\shell\\");
								SendDlgItemMessage(hWnd2, CB, CB_GETLBTEXT, iSel, (LPARAM)(pszName+_tcslen(pszName)));

								HKEY hkShell = NULL;
								if (0 == RegOpenKeyEx(HKEY_CLASSES_ROOT, pszName, 0, KEY_READ, &hkShell))
								{
									DWORD nType;
									DWORD nSize = MAX_PATH*2*sizeof(wchar_t);
									pszIco = (wchar_t*)calloc(nSize+2,1);
									if (0 != RegQueryValueEx(hkShell, L"Icon", NULL, &nType, (LPBYTE)pszIco, &nSize) || nType != REG_SZ)
										SafeFree(pszIco);
									HKEY hkCmd = NULL;
									if (0 == RegOpenKeyEx(hkShell, L"command", 0, KEY_READ, &hkCmd))
									{
										DWORD nSize = MAX_PATH*8*sizeof(wchar_t);
										pszFull = (wchar_t*)calloc(nSize+2,1);
										if (0 != RegQueryValueEx(hkCmd, NULL, NULL, &nType, (LPBYTE)pszFull, &nSize) || nType != REG_SZ)
										{
											SafeFree(pszIco);
										}
										else
										{
											LPCWSTR psz = pszFull;
											LPCWSTR pszPrev = pszFull;
											wchar_t szArg[MAX_PATH+1];
											while (0 == NextArg(&psz, szArg, &pszPrev))
											{
												if (*szArg != L'/')
													continue;

												if (lstrcmpi(szArg, L"/inside") == 0)
												{
													// Nop
												}
												else if (lstrcmpni(szArg, L"/inside=", 8) == 0)
												{
													pszDirSync = lstrdup(szArg+8); // may be empty!
												}
												else if (lstrcmpi(szArg, L"/config") == 0)
												{
													if (0 != NextArg(&psz, szArg))
														break;
													pszCfg = lstrdup(szArg);
												}
												else if (lstrcmpi(szArg, L"/dir") == 0)
												{
													if (0 != NextArg(&psz, szArg))
														break;
													_ASSERTE(lstrcmpi(szArg, L"%1")==0);
												}
												else //if (lstrcmpi(szArg, L"/cmd") == 0)
												{
													if (lstrcmpi(szArg, L"/cmd") == 0)
														pszCmd = psz;
													else
														pszCmd = pszPrev;
													break;
												}
											}
										}
										RegCloseKey(hkCmd);
									}
									RegCloseKey(hkShell);
								}
							}
						}

						SetDlgItemText(hWnd2, (CB==cbInsideName) ? tInsideConfig : tHereConfig,
							pszCfg ? pszCfg : L"");
						SetDlgItemText(hWnd2, (CB==cbInsideName) ? tInsideShell : tHereShell,
							pszCmd ? pszCmd : L"");
						SetDlgItemText(hWnd2, (CB==cbInsideName) ? tInsideIcon : tHereIcon,
							pszIco ? pszIco : L"");

						if (CB==cbInsideName)
						{
							SetDlgItemText(hWnd2, tInsideSyncDir, pszDirSync ? pszDirSync : L"");
							checkDlgButton(hWnd2, cbInsideSyncDir, (pszDirSync && *pszDirSync) ? BST_CHECKED : BST_UNCHECKED);
						}

						SafeFree(pszCfg);
						SafeFree(pszFull);
						SafeFree(pszIco);
						SafeFree(pszDirSync);
					}
					break;
				}
			}
			break; // CBN_SELCHANGE
		} // switch (HIWORD(wParam))
		break; // WM_COMMAND

	case UM_RELOAD_HERE_LIST:
		if (wParam == UM_RELOAD_HERE_LIST)
		{
			HKEY hkDir = NULL;
			size_t cchCmdMax = 65535;
			wchar_t* pszCmd = (wchar_t*)calloc(cchCmdMax,sizeof(*pszCmd));
			if (!pszCmd)
				break;

			// Возвращает NULL, если строка пустая
			wchar_t* pszCurInside = GetDlgItemText(hWnd2, cbInsideName);
			_ASSERTE((pszCurInside==NULL) || (*pszCurInside!=0));
			wchar_t* pszCurHere   = GetDlgItemText(hWnd2, cbHereName);
			_ASSERTE((pszCurHere==NULL) || (*pszCurHere!=0));

			bool lbOldSkip = bSkipCbSel; bSkipCbSel = true;

			SendDlgItemMessage(hWnd2, cbInsideName, CB_RESETCONTENT, 0, 0);
			SendDlgItemMessage(hWnd2, cbHereName, CB_RESETCONTENT, 0, 0);

			if (0 == RegOpenKeyEx(HKEY_CLASSES_ROOT, L"Directory\\shell", 0, KEY_READ, &hkDir))
			{
				for (DWORD i = 0; i < 512; i++)
				{
					wchar_t szName[MAX_PATH+32] = {};
					DWORD cchMax = countof(szName) - 32;
					if (0 != RegEnumKeyEx(hkDir, i, szName, &cchMax, NULL, NULL, NULL, NULL))
						break;
					wchar_t* pszSlash = szName + _tcslen(szName);
					wcscat_c(szName, L"\\command");
					HKEY hkCmd = NULL;
					if (0 == RegOpenKeyEx(hkDir, szName, 0, KEY_READ, &hkCmd))
					{
						DWORD cbMax = (cchCmdMax-2) * sizeof(*pszCmd);
						if (0 == RegQueryValueEx(hkCmd, NULL, NULL, NULL, (LPBYTE)pszCmd, &cbMax))
						{
							pszCmd[cbMax>>1] = 0;
							*pszSlash = 0;
							LPCWSTR pszInside = StrStrI(pszCmd, L"/inside");
							LPCWSTR pszConEmu = StrStrI(pszCmd, L"conemu");
							if (pszConEmu)
							{
								SendDlgItemMessage(hWnd2,
									pszInside ? cbInsideName : cbHereName,
									CB_ADDSTRING, 0, (LPARAM)szName);
								if ((pszInside ? pszCurInside : pszCurHere) == NULL)
								{
									if (pszInside)
										pszCurInside = lstrdup(szName);
									else
										pszCurHere = lstrdup(szName);
								}
							}
						}
						RegCloseKey(hkCmd);
					}
				}
				RegCloseKey(hkDir);
			}

			SetDlgItemText(hWnd2, cbInsideName, pszCurInside ? pszCurInside : L"");
			if (pszCurInside && *pszCurInside)
				SelectStringExact(hWnd2, cbInsideName, pszCurInside);

			SetDlgItemText(hWnd2, cbHereName, pszCurHere ? pszCurHere : L"");
			if (pszCurHere && *pszCurHere)
				SelectStringExact(hWnd2, cbHereName, pszCurHere);

			bSkipCbSel = lbOldSkip;

			SafeFree(pszCurInside);
			SafeFree(pszCurHere);

			free(pszCmd);
		}
		break; // UM_RELOAD_HERE_LIST

	case UM_RELOAD_AUTORUN:
		if (wParam == UM_RELOAD_AUTORUN) // страховка
		{
			size_t cchCmdMax = 65535;
			wchar_t *pszCmd = (wchar_t*)calloc(cchCmdMax,sizeof(*pszCmd));
			if (!pszCmd)
				break;

			BOOL bForceNewWnd = IsChecked(hWnd2, cbCmdAutorunNewWnd);

			HKEY hkDir = NULL;
			if (0 == RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Command Processor", 0, KEY_READ, &hkDir))
			{
				DWORD cbMax = (cchCmdMax-2) * sizeof(*pszCmd);
				if (0 == RegQueryValueEx(hkDir, L"AutoRun", NULL, NULL, (LPBYTE)pszCmd, &cbMax))
				{
					pszCmd[cbMax>>1] = 0;
					if (*pszCmd)
					{
						bForceNewWnd = (StrStrI(pszCmd, L"/GHWND=NEW") != NULL);
					}
				}
				else
					*pszCmd = 0;
				RegCloseKey(hkDir);
			}
			
			SetDlgItemText(hWnd2, tCmdAutoAttach, pszCmd);
			checkDlgButton(hWnd2, cbCmdAutorunNewWnd, bForceNewWnd);

			free(pszCmd);
		}
		break; // UM_RELOAD_AUTORUN
	}

	return iRc;
}

void CSettings::RegisterShell(LPCWSTR asName, LPCWSTR asOpt, LPCWSTR asConfig, LPCWSTR asCmd, LPCWSTR asIcon)
{
	if (!asName || !*asName)
		asName = L"ConEmu";

	if (!asCmd || !*asCmd)
		asCmd = L"cmd -cur_console:n";

	asCmd = SkipNonPrintable(asCmd);

	size_t cchMax = _tcslen(gpConEmu->ms_ConEmuExe)
		+ (asOpt ? (_tcslen(asOpt) + 3) : 0)
		+ (asConfig ? (_tcslen(asConfig) + 16) : 0)
		+ _tcslen(asCmd) + 32;
	wchar_t* pszCmd = (wchar_t*)malloc(cchMax*sizeof(*pszCmd));
	if (!pszCmd)
		return;

	//[HKEY_CURRENT_USER\Software\Classes\*\shell\ConEmu inside]
	//"Icon"="C:\\Program Files\\ConEmu\\ConEmu.exe,1"

	//[HKEY_CURRENT_USER\Software\Classes\*\shell\ConEmu inside\command]
	//@="C:\\Program Files\\ConEmu\\ConEmu.exe /inside /config shell /cmd powershell -cur_console:n"

	//[HKEY_CURRENT_USER\Software\Classes\Directory\Background\shell\ConEmu inside]
	//"Icon"="C:\\Program Files\\ConEmu\\ConEmu.exe,1"

	//[HKEY_CURRENT_USER\Software\Classes\Directory\Background\shell\ConEmu inside\command]
	//@="C:\\Program Files\\ConEmu\\ConEmu.exe /inside /config shell /cmd powershell -cur_console:n"

	//[HKEY_CURRENT_USER\Software\Classes\Directory\shell\ConEmu inside]
	//"Icon"="C:\\Program Files\\ConEmu\\ConEmu.exe,1"

	//[HKEY_CURRENT_USER\Software\Classes\Directory\shell\ConEmu inside\command]
	//@="C:\\Program Files\\ConEmu\\ConEmu.exe /inside /config shell /dir \"%1\" /cmd powershell -cur_console:n"

	//[HKEY_CURRENT_USER\Software\Classes\Drive\shell\ConEmu inside]
	//"Icon"="C:\\Program Files\\ConEmu\\ConEmu.exe,1"

	//[HKEY_CURRENT_USER\Software\Classes\Drive\shell\ConEmu inside\command]
	//@="C:\\Program Files\\ConEmu\\ConEmu.exe /inside /config shell /dir \"%1\" /cmd powershell -cur_console:n"

	int iSucceeded = 0;

	for (int i = 1; i <= 4; i++)
	{
		_wsprintf(pszCmd, SKIPLEN(cchMax) L"\"%s\" ", gpConEmu->ms_ConEmuExe);
		if (asOpt && *asOpt)
		{
			bool bQ = (wcschr(pszCmd, L' ') != NULL);
			if (bQ) _wcscat_c(pszCmd, cchMax, L"\"");
			_wcscat_c(pszCmd, cchMax, asOpt);
			_wcscat_c(pszCmd, cchMax, bQ ? L"\" " : L" ");
		}
		if (asConfig && *asConfig)
		{
			_wcscat_c(pszCmd, cchMax, L"/config \"");
			_wcscat_c(pszCmd, cchMax, asConfig);
			_wcscat_c(pszCmd, cchMax, L"\" ");
		}

		LPCWSTR pszRoot = NULL;
		switch (i)
		{
		case 1:
			pszRoot = L"Software\\Classes\\*\\shell";
			break;
		case 2:
			pszRoot = L"Software\\Classes\\Directory\\Background\\shell";
			break;
		case 3:
			pszRoot = L"Software\\Classes\\Directory\\shell";
			_wcscat_c(pszCmd, cchMax, L"/dir \"%1\" ");
			break;
		case 4:
			pszRoot = L"Software\\Classes\\Drive\\shell";
			_wcscat_c(pszCmd, cchMax, L"/dir \"%1\" ");
			break;
		}

		bool bCmdKeyExist = false;

		if (*asCmd == L'/')
		{
			bCmdKeyExist = (StrStrI(asCmd, L"/cmd ") != NULL);
		}

		if (!bCmdKeyExist)
			_wcscat_c(pszCmd, cchMax, L"/cmd ");
		_wcscat_c(pszCmd, cchMax, asCmd);

		HKEY hkRoot;
		if (0 == RegCreateKeyEx(HKEY_CURRENT_USER, pszRoot, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkRoot, NULL))
		{
			HKEY hkConEmu;
			if (0 == RegCreateKeyEx(hkRoot, asName, NULL, NULL, 0, KEY_ALL_ACCESS, NULL, &hkConEmu, NULL))
			{
				// Если задана "иконка"
				if (asIcon)
					RegSetValueEx(hkConEmu, L"Icon", 0, REG_SZ, (LPBYTE)asIcon, (lstrlen(asIcon)+1)*sizeof(*asIcon));
				else
					RegDeleteValue(hkConEmu, L"Icon");

				// Команда
				HKEY hkCmd;
				if (0 == RegCreateKeyEx(hkConEmu, L"command", NULL, NULL, 0, KEY_ALL_ACCESS, NULL, &hkCmd, NULL))
				{
					if (0 == RegSetValueEx(hkCmd, NULL, 0, REG_SZ, (LPBYTE)pszCmd, (lstrlen(pszCmd)+1)*sizeof(*pszCmd)))
						iSucceeded++;
					RegCloseKey(hkCmd);
				}

				RegCloseKey(hkConEmu);
			}
			RegCloseKey(hkRoot);
		}
	}

	free(pszCmd);
}

void CSettings::UnregisterShell(LPCWSTR asName)
{
	DeleteRegKeyRecursive(HKEY_CURRENT_USER, L"Software\\Classes\\Directory\\Background\\shell", asName);
	DeleteRegKeyRecursive(HKEY_CURRENT_USER, L"Software\\Classes\\Directory\\shell", asName);
	DeleteRegKeyRecursive(HKEY_CURRENT_USER, L"Software\\Classes\\Drive\\shell", asName);
	DeleteRegKeyRecursive(HKEY_CURRENT_USER, L"Software\\Classes\\*\\shell", asName);
}

bool CSettings::DeleteRegKeyRecursive(HKEY hRoot, LPCWSTR asParent, LPCWSTR asName)
{
	bool lbRc = false;
	HKEY hParent = NULL;
	HKEY hKey = NULL;

	if (!asName || !*asName || !hRoot)
		return false;

	if (asParent && *asParent)
	{
		if (0 != RegOpenKeyEx(hRoot, asParent, 0, KEY_ALL_ACCESS, &hParent))
			return false;
		hRoot = hParent;
	}

	if (0 == RegOpenKeyEx(hRoot, asName, 0, KEY_ALL_ACCESS, &hKey))
	{
		for (DWORD i = 0; i < 255; i++)
		{
			wchar_t szName[MAX_PATH];
			DWORD nMax = countof(szName);
			if (0 != RegEnumKeyEx(hKey, 0, szName, &nMax, 0, 0, 0, 0))
				break;

			if (!DeleteRegKeyRecursive(hKey, NULL, szName))
				break;
		}

		RegCloseKey(hKey);

		if (0 == RegDeleteKey(hRoot, asName))
			lbRc = true;
	}

	if (hParent)
		RegCloseKey(hParent);

	return lbRc;
}

void CSettings::ShellIntegration(HWND hDlg, CSettings::ShellIntegrType iMode, bool bEnabled, bool bForced /*= false*/)
{
	switch (iMode)
	{
	case ShellIntgr_Inside:
		{
			wchar_t szName[MAX_PATH] = {};
			GetDlgItemText(hDlg, cbInsideName, szName, countof(szName));
			if (bEnabled)
			{
				wchar_t szConfig[MAX_PATH] = {}, szShell[MAX_PATH] = {}, szIcon[MAX_PATH+16] = {}, szOpt[130] = {};
				GetDlgItemText(hDlg, tInsideConfig, szConfig, countof(szConfig));
				GetDlgItemText(hDlg, tInsideShell, szShell, countof(szShell));

				if (IsChecked(hDlg, cbInsideSyncDir))
				{
					wcscpy_c(szOpt, L"/inside=");
					int nOL = lstrlen(szOpt); _ASSERTE(nOL==8);
					GetDlgItemText(hDlg, tInsideSyncDir, szOpt+nOL, countof(szShell)-nOL);
					if (szOpt[8] == 0)
					{
						szOpt[0] = 0;
					}
				}

				//_wsprintf(szIcon, SKIPLEN(countof(szIcon)) L"%s,0", gpConEmu->ms_ConEmuExe);
				GetDlgItemText(hDlg, tInsideIcon, szIcon, countof(szIcon));
				RegisterShell(szName, szOpt[0] ? szOpt : L"/inside", SkipNonPrintable(szConfig), SkipNonPrintable(szShell), szIcon);
			}
			else if (*szName)
			{
				UnregisterShell(szName);
			}
		}
		break;
	case ShellIntgr_Here:
		{
			wchar_t szName[MAX_PATH] = {};
			GetDlgItemText(hDlg, cbHereName, szName, countof(szName));
			if (bEnabled)
			{
				wchar_t szConfig[MAX_PATH] = {}, szShell[MAX_PATH] = {}, szIcon[MAX_PATH+16];
				GetDlgItemText(hDlg, tHereConfig, szConfig, countof(szConfig));
				GetDlgItemText(hDlg, tHereShell, szShell, countof(szShell));
				//_wsprintf(szIcon, SKIPLEN(countof(szIcon)) L"%s,0", gpConEmu->ms_ConEmuExe);
				GetDlgItemText(hDlg, tHereIcon, szIcon, countof(szIcon));
				RegisterShell(szName, NULL, SkipNonPrintable(szConfig), SkipNonPrintable(szShell), szIcon);
			}
			else if (*szName)
			{
				UnregisterShell(szName);
			}
		}
		break;
	case ShellIntgr_CmdAuto:
		{
			BOOL bForceNewWnd = IsChecked(hDlg, cbCmdAutorunNewWnd);
				//checkDlgButton(, (StrStrI(pszCmd, L"/GHWND=NEW") != NULL));

			HKEY hkCmd = NULL;
			if (0 == RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Command Processor", 0, KEY_READ|KEY_WRITE, &hkCmd))
			{
				if (bEnabled)
				{
					wchar_t szCmd[MAX_PATH+128];
					_wsprintf(szCmd, SKIPLEN(countof(szCmd)) L"\"%s\\Cmd_Autorun.cmd", gpConEmu->ms_ConEmuBaseDir);
					if (FileExists(szCmd+1))
					{
						wcscat_c(szCmd, bForceNewWnd ? L"\" \"/GHWND=NEW\"" : L"\"");

						RegSetValueEx(hkCmd, L"AutoRun", 0, REG_SZ, (LPBYTE)szCmd, (lstrlen(szCmd)+1)*sizeof(szCmd[0]));
					}
					else
					{
						MessageBox(szCmd, MB_ICONSTOP, L"File not found", ghOpWnd);

						RegSetValueEx(hkCmd, L"AutoRun", 0, REG_SZ, (LPBYTE)L"", 1*sizeof(wchar_t));
					}
				}
				else
				{
					TODO("Если bForced==false - удалить только Cmd_Autorun.cmd, остальное - оставить");
					RegSetValueEx(hkCmd, L"AutoRun", 0, REG_SZ, (LPBYTE)L"", 1*sizeof(wchar_t));
				}

				RegCloseKey(hkCmd);
			}
		}
		break;
	}
}

LRESULT CSettings::OnInitDialog_Views(HWND hWnd2)
{
	CVConGuard VCon;
	CVConGroup::GetActiveVCon(&VCon);
	// пока выключим
	EnableWindow(GetDlgItem(hWnd2, bApplyViewSettings), VCon.VCon() ? VCon->IsPanelViews() : FALSE);

	SetDlgItemText(hWnd2, tThumbsFontName, gpSet->ThSet.Thumbs.sFontName);
	SetDlgItemText(hWnd2, tTilesFontName, gpSet->ThSet.Tiles.sFontName);

	if (gpSetCls->mh_EnumThread == NULL)  // Если шрифты уже считаны
		OnInitDialog_CopyFonts(hWnd2, tThumbsFontName, tTilesFontName, 0); // можно скопировать список с вкладки mh_Tabs[thi_Main]

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
	checkRadioButton(hWnd2, rbThumbBackColorIdx, rbThumbBackColorRGB,
	                 gpSet->ThSet.crBackground.UseIndex ? rbThumbBackColorIdx : rbThumbBackColorRGB);
	checkDlgButton(hWnd2, cbThumbPreviewBox, gpSet->ThSet.nPreviewFrame ? 1 : 0);
	nVal = gpSet->ThSet.crPreviewFrame.ColorIdx;
	FillListBox(hWnd2, lbThumbPreviewBoxColorIdx, SettingsNS::szColorIdxTh, SettingsNS::nColorIdxTh, nVal);
	checkRadioButton(hWnd2, rbThumbPreviewBoxColorIdx, rbThumbPreviewBoxColorRGB,
	                 gpSet->ThSet.crPreviewFrame.UseIndex ? rbThumbPreviewBoxColorIdx : rbThumbPreviewBoxColorRGB);
	checkDlgButton(hWnd2, cbThumbSelectionBox, gpSet->ThSet.nSelectFrame ? 1 : 0);
	nVal = gpSet->ThSet.crSelectFrame.ColorIdx;
	FillListBox(hWnd2, lbThumbSelectionBoxColorIdx, SettingsNS::szColorIdxTh, SettingsNS::nColorIdxTh, nVal);
	checkRadioButton(hWnd2, rbThumbSelectionBoxColorIdx, rbThumbSelectionBoxColorRGB,
	                 gpSet->ThSet.crSelectFrame.UseIndex ? rbThumbSelectionBoxColorIdx : rbThumbSelectionBoxColorRGB);

	if ((gpSet->ThSet.bLoadPreviews & 3) == 3)
		checkDlgButton(hWnd2, cbThumbLoadFiles, BST_CHECKED);
	else if ((gpSet->ThSet.bLoadPreviews & 3) == 1)
		checkDlgButton(hWnd2, cbThumbLoadFiles, BST_INDETERMINATE);
	else
		checkDlgButton(hWnd2, cbThumbLoadFiles, BST_UNCHECKED);

	checkDlgButton(hWnd2, cbThumbLoadFolders, gpSet->ThSet.bLoadFolders);
	SetDlgItemInt(hWnd2, tThumbLoadingTimeout, gpSet->ThSet.nLoadTimeout, FALSE);
	checkDlgButton(hWnd2, cbThumbUsePicView2, gpSet->ThSet.bUsePicView2);
	checkDlgButton(hWnd2, cbThumbRestoreOnStartup, gpSet->ThSet.bRestoreOnStartup);

	RegisterTipsFor(hWnd2);
	return 0;
}

// tThumbsFontName, tTilesFontName, 0
void CSettings::OnInitDialog_CopyFonts(HWND hWnd2, int nList1, ...)
{
	DWORD bAlmostMonospace;
	int nIdx, nCount, i;
	wchar_t szFontName[128]; // не должно быть более 32
	nCount = SendDlgItemMessage(gpSetCls->mh_Tabs[thi_Main], tFontFace, CB_GETCOUNT, 0, 0);

#ifdef _DEBUG
	GetDlgItemText(hWnd2, nList1, szFontName, countof(szFontName));
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
		if (SendDlgItemMessage(gpSetCls->mh_Tabs[thi_Main], tFontFace, CB_GETLBTEXT, i, (LPARAM) szFontName) > 0)
		{
			// Показывать [Raster Fonts WxH] смысла нет
			if (szFontName[0] == L'[' && !wcsncmp(szFontName+1, RASTER_FONTS_NAME, _tcslen(RASTER_FONTS_NAME)))
				continue;
			// В Thumbs & Tiles [bdf] пока не поддерживается
			int iLen = lstrlen(szFontName);
			if ((iLen > CE_BDF_SUFFIX_LEN) && !wcscmp(szFontName+iLen-CE_BDF_SUFFIX_LEN, CE_BDF_SUFFIX))
				continue;

			bAlmostMonospace = (DWORD)SendDlgItemMessage(gpSetCls->mh_Tabs[thi_Main], tFontFace, CB_GETITEMDATA, i, 0);

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
		GetDlgItemText(hWnd2, nCtrlIds[j], szFontName, countof(szFontName));
		gpSetCls->SelectString(hWnd2, nCtrlIds[j], szFontName);
	}
}

LRESULT CSettings::OnInitDialog_Debug(HWND hWnd2)
{
	//LVCOLUMN col ={LVCF_WIDTH|LVCF_TEXT|LVCF_FMT, LVCFMT_LEFT, 60};
	//wchar_t szTitle[64]; col.pszText = szTitle;

	checkDlgButton(hWnd2, rbActivityDisabled, BST_CHECKED);

	HWND hList = GetDlgItem(hWnd2, lbActivityLog);
	ListView_SetExtendedListViewStyleEx(hList,LVS_EX_FULLROWSELECT,LVS_EX_FULLROWSELECT);
	ListView_SetExtendedListViewStyleEx(hList,LVS_EX_LABELTIP|LVS_EX_INFOTIP,LVS_EX_LABELTIP|LVS_EX_INFOTIP);

	LVCOLUMN col ={LVCF_WIDTH|LVCF_TEXT|LVCF_FMT, LVCFMT_LEFT, 60};
	wchar_t szTitle[4]; col.pszText = szTitle;
	wcscpy_c(szTitle, L" ");		ListView_InsertColumn(hList, 0, &col);
	
	HWND hTip = ListView_GetToolTips(hList);
	SetWindowPos(hTip, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);


	//HWND hDetails = GetDlgItem(hWnd2, lbActivityDetails);
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

	//ShowWindow(GetDlgItem(hWnd2, gbInputActivity), SW_HIDE);
	//ShowWindow(GetDlgItem(hWnd2, lbInputActivity), SW_HIDE);


	//hList = GetDlgItem(hWnd2, lbInputActivity);
	//col.cx = 80;
	//wcscpy_c(szTitle, L"Type");			ListView_InsertColumn(hList, 0, &col);	col.cx = 120;
	//wcscpy_c(szTitle, L"Sent");			ListView_InsertColumn(hList, 1, &col);
	//wcscpy_c(szTitle, L"Received");		ListView_InsertColumn(hList, 2, &col);
	//wcscpy_c(szTitle, L"Description");	ListView_InsertColumn(hList, 3, &col);
	
	RegisterTipsFor(hWnd2);
	return 0;
}

LRESULT CSettings::OnInitDialog_Update(HWND hWnd2)
{
	ConEmuUpdateSettings* p = &gpSet->UpdSet;
	
	// Через интерфейс - не редактируется
	SetDlgItemText(hWnd2, tUpdateVerLocation, p->UpdateVerLocation());
	
	checkDlgButton(hWnd2, cbUpdateCheckOnStartup, p->isUpdateCheckOnStartup);
	checkDlgButton(hWnd2, cbUpdateCheckHourly, p->isUpdateCheckHourly);
	checkDlgButton(hWnd2, cbUpdateConfirmDownload, !p->isUpdateConfirmDownload);
	checkRadioButton(hWnd2, rbUpdateStableOnly, rbUpdateLatestAvailable, (p->isUpdateUseBuilds==1) ? rbUpdateStableOnly : rbUpdateLatestAvailable);
	checkRadioButton(hWnd2, rbUpdateUseExe, rbUpdateUseArc, (p->UpdateDownloadSetup()==1) ? rbUpdateUseExe : rbUpdateUseArc);
	
	checkDlgButton(hWnd2, cbUpdateUseProxy, p->isUpdateUseProxy);
	OnButtonClicked(hWnd2, cbUpdateUseProxy, 0); // Enable/Disable proxy fields
	SetDlgItemText(hWnd2, tUpdateProxy, p->szUpdateProxy);
	SetDlgItemText(hWnd2, tUpdateProxyUser, p->szUpdateProxyUser);
	SetDlgItemText(hWnd2, tUpdateProxyPassword, p->szUpdateProxyPassword);
	
	SetDlgItemText(hWnd2, tUpdateExeCmdLine, p->UpdateExeCmdLine());
	SetDlgItemText(hWnd2, tUpdateArcCmdLine, p->UpdateArcCmdLine());
	SetDlgItemText(hWnd2, tUpdatePostUpdateCmd, p->szUpdatePostUpdateCmd);
	
	checkDlgButton(hWnd2, cbUpdateLeavePackages, p->isUpdateLeavePackages);
	SetDlgItemText(hWnd2, tUpdateDownloadPath, p->szUpdateDownloadPath);

	RegisterTipsFor(hWnd2);
	return 0;
}

LRESULT CSettings::OnInitDialog_Info(HWND hWnd2)
{
	CVirtualConsole* pVCon = NULL;
	CVConGuard VCon;
	if (CVConGroup::GetActiveVCon(&VCon) >= 0)
		pVCon = VCon.VCon();

	SetDlgItemText(hWnd2, tCurCmdLine, GetCommandLine());
	// Performance
	Performance(gbPerformance, TRUE);
	gpConEmu->UpdateProcessDisplay(TRUE);
	gpConEmu->UpdateSizes();
	if (pVCon)
		pVCon->RCon()->UpdateCursorInfo();
	UpdateFontInfo();
	if (pVCon)
		UpdateConsoleMode(pVCon->RCon()->GetConsoleStates());
	RegisterTipsFor(hWnd2);
	return 0;
}

LRESULT CSettings::OnButtonClicked(HWND hWnd2, WPARAM wParam, LPARAM lParam)
{
	_ASSERTE(hWnd2!=NULL);
	WORD CB = LOWORD(wParam);

	switch (CB)
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

				// были изменения в полях размера/положения?
				if (mh_Tabs[thi_SizePos] && IsWindowEnabled(GetDlgItem(mh_Tabs[thi_SizePos], cbApplyPos)))
					OnButtonClicked(mh_Tabs[thi_SizePos], cbApplyPos, 0);

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
			PostMessage(hWnd2, gpSetCls->mn_MsgRecreateFont, wParam, 0);
			break;
		case rNormal:
		case rFullScreen:
		case rMaximized:
			if (gpSet->isQuakeStyle)
			{
				gpSet->_WindowMode = CB;
				RECT rcWnd = gpConEmu->GetDefaultRect();
				//gpConEmu->SetWindowMode((ConEmuWindowMode)CB);
				SetWindowPos(ghWnd, NULL, rcWnd.left, rcWnd.top, rcWnd.right-rcWnd.left, rcWnd.bottom-rcWnd.top, SWP_NOZORDER);
				apiSetForegroundWindow(ghOpWnd);
			}
			else
			{
				EnableWindow(GetDlgItem(hWnd2, cbApplyPos), TRUE);
				for (size_t i = 0; i < countof(SettingsNS::nSizeCtrlId); i++)
					EnableWindow(GetDlgItem(hWnd2, SettingsNS::nSizeCtrlId[i]), CB == rNormal);
			}
			break;
		case cbApplyPos:
			{
				if (!gpSet->isQuakeStyle)
				{
					if (IsChecked(hWnd2, rNormal) == BST_CHECKED)
					{
						int newX, newY;
						DWORD newW, newH;
						//wchar_t temp[MAX_PATH];
						//GetDlgItemText(hWnd2, tWndWidth, temp, countof(temp));  newW = klatoi(temp);
						//GetDlgItemText(hWnd2, tWndHeight, temp, countof(temp)); newH = klatoi(temp);
						BOOL lbOk;

						newW = GetDlgItemInt(hWnd2, tWndWidth, &lbOk, FALSE);
						if (!lbOk) newW = gpConEmu->wndWidth;
						newH = GetDlgItemInt(hWnd2, tWndHeight, &lbOk, FALSE);
						if (!lbOk) newH = gpConEmu->wndHeight;

						newX = (int)GetDlgItemInt(hWnd2, tWndX, &lbOk, TRUE);
						if (!lbOk) newX = gpConEmu->wndX;
						newY = (int)GetDlgItemInt(hWnd2, tWndY, &lbOk, TRUE);
						if (!lbOk) newY = gpConEmu->wndY;

						SetFocus(GetDlgItem(hWnd2, rNormal));

						if (gpConEmu->isZoomed() || gpConEmu->isIconic() || gpConEmu->isFullScreen())
							gpConEmu->SetWindowMode(wmNormal);

						SetWindowPos(ghWnd, NULL, newX, newY, 0,0, SWP_NOSIZE|SWP_NOZORDER);

						// Установить размер
						CVConGroup::SetAllConsoleWindowsSize(MakeCoord(newW, newH), /*true,*/ true, true);

						SetWindowPos(ghWnd, NULL, newX, newY, 0,0, SWP_NOSIZE|SWP_NOZORDER);
					}
					else if (IsChecked(hWnd2, rMaximized) == BST_CHECKED)
					{
						SetFocus(GetDlgItem(hWnd2, rMaximized));

						if (!gpConEmu->isZoomed())
							gpConEmu->SetWindowMode(wmMaximized);
					}
					else if (IsChecked(hWnd2, rFullScreen) == BST_CHECKED)
					{
						SetFocus(GetDlgItem(hWnd2, rFullScreen));

						if (!gpConEmu->isFullScreen())
							gpConEmu->SetWindowMode(wmFullScreen);
					}

					// Запомнить "идеальный" размер окна, выбранный пользователем
					gpConEmu->UpdateIdealRect(TRUE);
				}
				EnableWindow(GetDlgItem(hWnd2, cbApplyPos), FALSE);
				apiSetForegroundWindow(ghOpWnd);
			} // cbApplyPos
			break;
		case rCascade:
		case rFixed:
			gpSet->wndCascade = CB == rCascade;
			break;
		case cbUseCurrentSizePos:
			gpSet->isUseCurrentSizePos = IsChecked(hWnd2, cbUseCurrentSizePos);
			if (gpSet->isUseCurrentSizePos)
			{
				UpdateWindowMode(gpConEmu->WindowMode);
				UpdatePos(gpConEmu->wndX, gpConEmu->wndY, true);
				UpdateSize(gpConEmu->wndWidth, gpConEmu->wndHeight);
			}
			break;
		case cbAutoSaveSizePos:
			gpSet->isAutoSaveSizePos = IsChecked(hWnd2, cbAutoSaveSizePos);
			break;
		case cbFontAuto:
			gpSet->isFontAutoSize = IsChecked(hWnd2, cbFontAuto);

			if (gpSet->isFontAutoSize && LogFont.lfFaceName[0] == L'['
			        && !wcsncmp(LogFont.lfFaceName+1, RASTER_FONTS_NAME, _tcslen(RASTER_FONTS_NAME)))
			{
				gpSet->isFontAutoSize = false;
				checkDlgButton(hWnd2, cbFontAuto, BST_UNCHECKED);
				ShowFontErrorTip(szRasterAutoError);
			}

			break;
		case cbFixFarBorders:

			//gpSet->isFixFarBorders = !gpSet->isFixFarBorders;
			switch (IsChecked(hWnd2, cbFixFarBorders))
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
			gpSet->AppStd.isCursorColor = IsChecked(hWnd2,cbCursorColor);
			gpConEmu->Update(true);
			break;
		case cbCursorBlink:
			gpSet->AppStd.isCursorBlink = IsChecked(hWnd2,cbCursorBlink);
			if (!gpSet->AppStd.isCursorBlink) // если мигание отключается - то курсор может "замереть" в погашенном состоянии.
				CVConGroup::InvalidateAll();
			break;
		case cbMultiCon:
			gpSet->isMulti = IsChecked(hWnd2, cbMultiCon);
			break;
		case cbMultiIterate:
			gpSet->isMultiIterate = IsChecked(hWnd2, cbMultiIterate);
			break;
		case cbNewConfirm:
			gpSet->isMultiNewConfirm = IsChecked(hWnd2, cbNewConfirm);
			break;
		case cbLongOutput:
			gpSet->AutoBufferHeight = IsChecked(hWnd2, cbLongOutput);
			gpConEmu->UpdateFarSettings();
			EnableWindow(GetDlgItem(hWnd2, tLongOutputHeight), gpSet->AutoBufferHeight);
			break;
		case rbComspecAuto:
		case rbComspecEnvVar:
		case rbComspecCmd:
		case rbComspecExplicit:
			if (IsChecked(hWnd2, rbComspecExplicit))
				gpSet->ComSpec.csType = cst_Explicit;
			else if (IsChecked(hWnd2, rbComspecCmd))
				gpSet->ComSpec.csType = cst_Cmd;
			else if (IsChecked(hWnd2, rbComspecEnvVar))
				gpSet->ComSpec.csType = cst_EnvVar;
			else
				gpSet->ComSpec.csType = cst_AutoTccCmd;
			gpConEmu->OnGlobalSettingsChanged();
			break;
		case cbComspecExplicit:
			{
				wchar_t temp[MAX_PATH], edt[MAX_PATH];
				if (!GetDlgItemText(hWnd2, tComspecExplicit, edt, countof(edt)))
					edt[0] = 0;
				ExpandEnvironmentStrings(edt, temp, countof(temp));
				OPENFILENAME ofn; memset(&ofn,0,sizeof(ofn));
				ofn.lStructSize=sizeof(ofn);
				ofn.hwndOwner = ghOpWnd;
				ofn.lpstrFilter = L"Processors (cmd.exe,tcc.exe)\0cmd.exe;tcc.exe\0Executables (*.exe)\0*.exe\0\0";
				ofn.nFilterIndex = 1;
				ofn.lpstrFile = temp;
				ofn.nMaxFile = countof(temp);
				ofn.lpstrTitle = L"Choose command processor";
				ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
							| OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST;

				if (GetOpenFileName(&ofn))
				{
					bool bChanged = (lstrcmp(gpSet->ComSpec.ComspecExplicit, temp)!=0);
					SetDlgItemText(hWnd2, tComspecExplicit, temp);
					if (bChanged)
					{
						wcscpy_c(gpSet->ComSpec.ComspecExplicit, temp);
						gpSet->ComSpec.csType = cst_Explicit;
						checkRadioButton(hWnd2, rbComspecAuto, rbComspecExplicit, rbComspecExplicit);
						gpConEmu->OnGlobalSettingsChanged();
					}
				}
			} //cbComspecExplicit
			break;
		case cbComspecTest:
			{
				wchar_t* psz = GetComspec(&gpSet->ComSpec);
				MessageBox(ghOpWnd, psz ? psz : L"<NULL>", gpConEmu->GetDefaultTitle(), MB_ICONINFORMATION);
				SafeFree(psz);
			} // cbComspecTest
			break;
		case rbComspec_OSbit:
		case rbComspec_AppBit:
		case rbComspec_x32:
			if (IsChecked(hWnd2, rbComspec_x32))
				gpSet->ComSpec.csBits = csb_x32;
			else if (IsChecked(hWnd2, rbComspec_AppBit))
				gpSet->ComSpec.csBits = csb_SameApp;
			else
				gpSet->ComSpec.csBits = csb_SameOS;
			gpConEmu->OnGlobalSettingsChanged();
			break;
		case cbComspecUpdateEnv:
			gpSet->ComSpec.isUpdateEnv = IsChecked(hWnd2, cbComspecUpdateEnv);
			gpConEmu->OnGlobalSettingsChanged();
			break;
		case cbAddConEmu2Path:
			gpSet->ComSpec.isAddConEmu2Path = IsChecked(hWnd2, cbAddConEmu2Path);
			gpConEmu->OnGlobalSettingsChanged();
			break;
		case cbComspecUncPaths:
			gpSet->ComSpec.isAllowUncPaths = IsChecked(hWnd2, cbComspecUncPaths);
			UpdateComSpecUncSupport();
			break;
		case cbBold:
		case cbItalic:
			PostMessage(hWnd2, gpSetCls->mn_MsgRecreateFont, wParam, 0);
			break;
		case cbBgImage:
			{
				gpSet->isShowBgImage = IsChecked(hWnd2, cbBgImage);
				EnableWindow(GetDlgItem(hWnd2, tBgImage), gpSet->isShowBgImage);
				//EnableWindow(GetDlgItem(hWnd2, tDarker), gpSet->isShowBgImage);
				//EnableWindow(GetDlgItem(hWnd2, slDarker), gpSet->isShowBgImage);
				EnableWindow(GetDlgItem(hWnd2, bBgImage), gpSet->isShowBgImage);
				//EnableWindow(GetDlgItem(hWnd2, rBgUpLeft), gpSet->isShowBgImage);
				//EnableWindow(GetDlgItem(hWnd2, rBgStretch), gpSet->isShowBgImage);
				//EnableWindow(GetDlgItem(hWnd2, rBgTile), gpSet->isShowBgImage);
				BOOL lbNeedLoad = (mp_Bg == NULL);

				if (gpSet->isShowBgImage && gpSet->bgImageDarker == 0)
				{
					if (MessageBox(ghOpWnd,
								  L"Background image will NOT be visible\n"
								  L"while 'Darkening' is 0. Increase it?",
								  gpConEmu->GetDefaultTitle(), MB_YESNO|MB_ICONEXCLAMATION)!=IDNO)
					{
						gpSet->bgImageDarker = 0x46;
						SendDlgItemMessage(hWnd2, slDarker, TBM_SETPOS, (WPARAM) true, (LPARAM) gpSet->bgImageDarker);
						TCHAR tmp[10];
						_wsprintf(tmp, SKIPLEN(countof(tmp)) L"%i", gpSet->bgImageDarker);
						SetDlgItemText(hWnd2, tDarker, tmp);
						lbNeedLoad = TRUE;
					}
				}

				if (lbNeedLoad)
				{
					gpSetCls->LoadBackgroundFile(gpSet->sBgImage, true);
				}

				gpConEmu->Update(true);

			} // cbBgImage
			break;
#if 0
		case rBgUpLeft:
		case rBgStretch:
		case rBgTile:
			gpSet->bgOperation = (char)(CB - rBgUpLeft);
			gpSetCls->LoadBackgroundFile(gpSet->sBgImage, true);
			gpConEmu->Update(true);
			break;
#endif
		case cbBgAllowPlugin:
			gpSet->isBgPluginAllowed = IsChecked(hWnd2, cbBgAllowPlugin);
			NeedBackgroundUpdate();
			gpConEmu->Update(true);
			break;
		case cbRClick:
			gpSet->isRClickSendKey = IsChecked(hWnd2, cbRClick); //0-1-2
			break;
		case cbSafeFarClose:
			gpSet->isSafeFarClose = IsChecked(hWnd2, cbSafeFarClose);
			break;
		case cbMinToTray:
			gpSet->mb_MinToTray = IsChecked(hWnd2, cbMinToTray);
			break;
		case cbCloseConsoleConfirm:
			gpSet->isCloseConsoleConfirm = IsChecked(hWnd2, cbCloseConsoleConfirm);
			break;
		case cbAlwaysShowTrayIcon:
			gpSet->isAlwaysShowTrayIcon = IsChecked(hWnd2, cbAlwaysShowTrayIcon);
			Icon.SettingsChanged();
			break;
		case cbQuakeStyle:
		case cbQuakeAutoHide:
			{
				BYTE NewQuakeMode = IsChecked(hWnd2, cbQuakeStyle)
					? IsChecked(hWnd2, cbQuakeAutoHide) ? 2 : 1 : 0;
				ConEmuWindowMode NewWindowMode = 
					IsChecked(hWnd2, rMaximized) ? wmMaximized :
					IsChecked(hWnd2, rFullScreen) ? wmFullScreen : 
					wmNormal;
				// здесь меняются gpSet->isQuakeStyle, gpSet->isTryToCenter, gpSet->SetMinToTray
				gpConEmu->SetQuakeMode(NewQuakeMode, NewWindowMode, true);
			}
			break;
		case cbHideCaption:
			gpSet->isHideCaption = IsChecked(hWnd2, cbHideCaption);
			break;
		case cbHideCaptionAlways:
			gpSet->SetHideCaptionAlways(0!=IsChecked(hWnd2, cbHideCaptionAlways));

			if (gpSet->isHideCaptionAlways())
			{
				checkDlgButton(hWnd2, cbHideCaptionAlways, BST_CHECKED);
				//TODO("показать тултип, что скрытие обязательно при прозрачности");
			}
			EnableWindow(GetDlgItem(hWnd2, cbHideCaptionAlways), !gpSet->isForcedHideCaptionAlways());

			gpConEmu->OnHideCaption();
			apiSetForegroundWindow(ghOpWnd);
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
		case cbKeyBarRClick:
			gpSet->isKeyBarRClick = IsChecked(hWnd2, cbKeyBarRClick);
			break;
		case cbDragPanel:
			gpSet->isDragPanel = IsChecked(hWnd2, cbDragPanel);
			gpConEmu->OnSetCursor();
			break;
		case cbDragPanelBothEdges:
			gpSet->isDragPanelBothEdges = IsChecked(hWnd2, cbDragPanelBothEdges);
			gpConEmu->OnSetCursor();
			break;
		case cbTryToCenter:
			gpSet->isTryToCenter = IsChecked(hWnd2, cbTryToCenter);
			gpConEmu->OnSize(false);
			gpConEmu->InvalidateAll();
			break;
		case cbAlwaysShowScrollbar:
			gpSet->isAlwaysShowScrollbar = IsChecked(hWnd2, cbAlwaysShowScrollbar);
			if (!gpSet->isAlwaysShowScrollbar) gpConEmu->OnAlwaysShowScrollbar(false);
			if (gpConEmu->isZoomed() || gpConEmu->isFullScreen())
				gpConEmu->SyncConsoleToWindow();
			else
				CVConGroup::SetAllConsoleWindowsSize(MakeCoord(gpConEmu->wndWidth, gpConEmu->wndHeight), /*true,*/ true, true);
			if (gpSet->isAlwaysShowScrollbar) gpConEmu->OnAlwaysShowScrollbar(false);
			gpConEmu->ReSize();
			//gpConEmu->OnSize(true);
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
		case cbDropUseMenu:
			gpSet->isDropUseMenu = IsChecked(hWnd2, cbDropUseMenu);
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
		//case cbTabs:
		case rbTabsNone:
		case rbTabsAlways:
		case rbTabsAuto:

			if (IsChecked(hWnd2, rbTabsAuto))
			{
				gpSet->isTabs = 2;
			}
			else if (IsChecked(hWnd2, rbTabsAlways))
			{
				gpSet->isTabs = 1;
				gpConEmu->ForceShowTabs(TRUE);
			}
			else
			{
				gpSet->isTabs = 0;
				gpConEmu->ForceShowTabs(FALSE);
			}
			
			gpConEmu->mp_TabBar->Update();
			gpConEmu->UpdateWindowRgn();

			//switch (IsChecked(hWnd2, cbTabs))
			//{
			//	case BST_UNCHECKED:
			//		gpSet->isTabs = 0; break;
			//	case BST_CHECKED:
			//		gpSet->isTabs = 1; break;
			//	case BST_INDETERMINATE:
			//		gpSet->isTabs = 2; break;
			//}

			//TODO("Хорошо бы сразу видимость табов менять");
			////gpConEmu->mp_TabBar->Update(TRUE); -- это как-то неправильно работает.
			break;
		case cbTabsLocationBottom:
			gpSet->nTabsLocation = IsChecked(hWnd2, cbTabsLocationBottom);
			gpConEmu->OnSize();
			break;
		case cbTabSelf:
			gpSet->isTabSelf = IsChecked(hWnd2, cbTabSelf);
			break;
		case cbTabRecent:
			gpSet->isTabRecent = IsChecked(hWnd2, cbTabRecent);
			break;
		case cbTabLazy:
			gpSet->isTabLazy = IsChecked(hWnd2, cbTabLazy);
			break;
		//case cbTabsOnTaskBar:
		case rbTaskbarBtnActive:
		case rbTaskbarBtnAll:
		case rbTaskbarBtnWin7:
		case rbTaskbarBtnHidden:
			// 3state: BST_UNCHECKED/BST_CHECKED/BST_INDETERMINATE
			gpSet->m_isTabsOnTaskBar = IsChecked(hWnd2, rbTaskbarBtnAll) ? 1
				: IsChecked(hWnd2, rbTaskbarBtnWin7) ? 2
				: IsChecked(hWnd2, rbTaskbarBtnHidden) ? 3 : 0;
			if ((gpSet->m_isTabsOnTaskBar == 3) && !gpSet->mb_MinToTray)
			{
				gpSet->SetMinToTray(true);
			}
			gpConEmu->OnTaskbarSettingsChanged();
			break;
		case cbRSelectionFix:
			gpSet->isRSelFix = IsChecked(hWnd2, cbRSelectionFix);
			break;
		case cbEnableMouse:
			gpSet->isDisableMouse = IsChecked(hWnd2, cbEnableMouse) ? false : true;
			break;
		case cbSkipActivation:
			gpSet->isMouseSkipActivation = IsChecked(hWnd2, cbSkipActivation);
			break;
		case cbSkipMove:
			gpSet->isMouseSkipMoving = IsChecked(hWnd2, cbSkipMove);
			break;
		case cbMonitorConsoleLang:
			// "|2" reserved for "One layout for all consoles", always on
			gpSet->isMonitorConsoleLang = IsChecked(hWnd2, cbMonitorConsoleLang) ? 3 : 0;
			break;
		case cbSkipFocusEvents:
			gpSet->isSkipFocusEvents = IsChecked(hWnd2, cbSkipFocusEvents);
			break;
		case cbMonospace:
			{
				#ifdef _DEBUG
				BYTE cMonospaceNow = gpSet->isMonospace;
				#endif
				gpSet->isMonospace = IsChecked(hWnd2, cbMonospace);

				if (gpSet->isMonospace) gpSetCls->isMonospaceSelected = gpSet->isMonospace;

				mb_IgnoreEditChanged = TRUE;
				ResetFontWidth();
				gpConEmu->Update(true);
				mb_IgnoreEditChanged = FALSE;
			} // cbMonospace
			break;
		case cbExtendFonts:
			gpSet->AppStd.isExtendFonts = IsChecked(hWnd2, cbExtendFonts);
			gpConEmu->Update(true);
			break;
		//case cbAutoConHandle:
		//	isUpdConHandle = !isUpdConHandle;
		//	gpConEmu->Update(true);
		//	break;
		case rCursorH:
		case rCursorV:

			if (wParam == rCursorV)
				gpSet->AppStd.isCursorV = true;
			else
				gpSet->AppStd.isCursorV = false;

			gpConEmu->Update(true);
			break;
		case cbBlockInactiveCursor:
			gpSet->AppStd.isCursorBlockInactive = IsChecked(hWnd2, cbBlockInactiveCursor);
			CVConGroup::InvalidateAll();
			break;
		case cbCursorIgnoreSize:
			gpSet->AppStd.isCursorIgnoreSize = IsChecked(hWnd2, cbCursorIgnoreSize);
			CVConGroup::InvalidateAll();
			break;
		case bBgImage:
			{
				wchar_t temp[MAX_PATH], edt[MAX_PATH];
				if (!GetDlgItemText(hWnd2, tBgImage, edt, countof(edt)))
					edt[0] = 0;
				ExpandEnvironmentStrings(edt, temp, countof(temp));
				OPENFILENAME ofn; memset(&ofn,0,sizeof(ofn));
				ofn.lStructSize=sizeof(ofn);
				ofn.hwndOwner = ghOpWnd;
				ofn.lpstrFilter = L"All images (*.bmp,*.jpg,*.png)\0*.bmp;*.jpg;*.jpe;*.jpeg;*.png\0Bitmap images (*.bmp)\0*.bmp\0JPEG images (*.jpg)\0*.jpg;*.jpe;*.jpeg\0PNG images (*.png)\0*.png\0\0";
				ofn.nFilterIndex = 1;
				ofn.lpstrFile = temp;
				ofn.nMaxFile = countof(temp);
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
						SetDlgItemText(hWnd2, tBgImage, gpSet->sBgImage);
						gpConEmu->Update(true);
					}
				}
			} // bBgImage
			break;
		case cbVisible:
			gpSet->isConVisible = IsChecked(hWnd2, cbVisible);

			if (gpSet->isConVisible)
			{
				// Если показывать - то только текущую (иначе на экране мешанина консолей будет
				CVConGuard VCon;
				if (CVConGroup::GetActiveVCon(&VCon) >= 0)
					VCon->RCon()->ShowConsole(gpSet->isConVisible);
			}
			else
			{
				// А если скрывать - то все сразу
				for (int i=0; i<MAX_CONSOLE_COUNT; i++)
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
		case cbProcessAnsi:
			gpSet->isProcessAnsi = IsChecked(hWnd2, cbProcessAnsi);
			gpConEmu->OnGlobalSettingsChanged();
			break;
		case cbUseClink:
			gpSet->mb_UseClink = IsChecked(hWnd2, cbUseClink);
			if (gpSet->mb_UseClink && !gpSet->isUseClink())
			{
				checkDlgButton(hWnd2, cbUseClink, BST_UNCHECKED);
				MessageBox(L"Clink was not found in '%ConEmuBaseDir%\\clink'\nDownload and unpack clink files\nhttp://code.google.com/p/clink/", MB_ICONSTOP|MB_SYSTEMMODAL, NULL, ghOpWnd);
			}
			gpConEmu->OnGlobalSettingsChanged();
			break;
		case cbPortableRegistry:
			#ifdef USEPORTABLEREGISTRY
			gpSet->isPortableReg = IsChecked(hWnd2, cbPortableRegistry);
			// Проверить, готов ли к использованию
			if (!gpConEmu->PreparePortableReg())
			{
				gpSet->isPortableReg = false;
				checkDlgButton(hWnd2, cbPortableRegistry, BST_UNCHECKED);
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
		case cbSnapToDesktopEdges:
			gpSet->isSnapToDesktopEdges = IsChecked(hWnd2, cbSnapToDesktopEdges);
			if (gpSet->isSnapToDesktopEdges)
				gpConEmu->OnMoving();
			break;
		case cbAlwaysOnTop:
			gpSet->isAlwaysOnTop = IsChecked(hWnd2, cbAlwaysOnTop);
			gpConEmu->OnAlwaysOnTop();
			break;
		case cbSleepInBackground:
			gpSet->isSleepInBackground = IsChecked(hWnd2, cbSleepInBackground);
			CVConGroup::OnGuiFocused(TRUE);
			break;
		case cbDisableFarFlashing:
			gpSet->isDisableFarFlashing = IsChecked(hWnd2, cbDisableFarFlashing);
			break;
		case cbDisableAllFlashing:
			gpSet->isDisableAllFlashing = IsChecked(hWnd2, cbDisableAllFlashing);
			break;
		case cbShowWasHiddenMsg:
			gpSet->isDownShowHiddenMessage = IsChecked(hWnd2, cbShowWasHiddenMsg) ? false : true;
			break;
		case cbTabsInCaption:
			gpSet->isTabsInCaption = IsChecked(hWnd2, cbTabsInCaption);
			////RedrawWindow(ghWnd, NULL, NULL, RDW_UPDATENOW|RDW_FRAME);
			////gpConEmu->OnNcMessage(ghWnd, WM_NCPAINT, 0,0);
			//SendMessage(ghWnd, WM_NCACTIVATE, 0, 0);
			//SendMessage(ghWnd, WM_NCPAINT, 0, 0);
			gpConEmu->RedrawTabPanel();
			break;
		case cbNumberInCaption:
			gpSet->isNumberInCaption = IsChecked(hWnd2, cbNumberInCaption);
			gpConEmu->UpdateTitle();
			break;
		case rbAdminShield:
		case rbAdminSuffix:
			gpSet->bAdminShield = IsChecked(hWnd2, rbAdminShield);
			gpConEmu->mp_TabBar->Update(TRUE);
			break;
		case cbHideInactiveConTabs:
			gpSet->bHideInactiveConsoleTabs = IsChecked(hWnd2, cbHideInactiveConTabs);
			gpConEmu->mp_TabBar->Update(TRUE);
			break;
		case cbHideDisabledTabs:
			gpSet->bHideDisabledTabs = IsChecked(hWnd2, cbHideDisabledTabs);
			gpConEmu->mp_TabBar->Update(TRUE);
			break;
		case cbShowFarWindows:
			gpSet->bShowFarWindows = IsChecked(hWnd2, cbShowFarWindows);
			gpConEmu->mp_TabBar->Update(TRUE);
			break;
		case cbMultiLeaveOnClose:
			gpSet->isMultiLeaveOnClose = IsChecked(hWnd2, cbMultiLeaveOnClose);
			break;

		case cbGuiMacroHelp:
			gpConEmu->OnInfo_About(L"Macro");
			break;
			
		case cbUseWinArrows:
		case cbUseWinNumber:
		case cbUseWinTab:
			switch (CB)
			{
				case cbUseWinArrows:
					gpSet->isUseWinArrows = IsChecked(hWnd2, CB);
					break;
				case cbUseWinNumber:
					gpSet->isUseWinNumber = IsChecked(hWnd2, CB);
					break;
				case cbUseWinTab:
					gpSet->isUseWinTab = IsChecked(hWnd2, CB);
					break;
			}
			gpConEmu->UpdateWinHookSettings();
			break;

		//case cbUseWinNumber:
		//	gpSet->isUseWinNumber = IsChecked(hWnd2, cbUseWinNumber);
		//	if (hWnd2) checkDlgButton(hWnd2, cbUseWinNumberK, gpSet->isUseWinNumber ? BST_CHECKED : BST_UNCHECKED);
		//	gpConEmu->UpdateWinHookSettings();
		//	break;

		case cbSendAltTab:
		case cbSendAltEsc:
		case cbSendAltPrintScrn:
		case cbSendPrintScrn:
		case cbSendCtrlEsc:
			switch (CB)
			{
				case cbSendAltTab:
					gpSet->isSendAltTab = IsChecked(hWnd2, cbSendAltTab); break;
				case cbSendAltEsc:
					gpSet->isSendAltEsc = IsChecked(hWnd2, cbSendAltEsc); break;
				case cbSendAltPrintScrn:
					gpSet->isSendAltPrintScrn = IsChecked(hWnd2, cbSendAltPrintScrn); break;
				case cbSendPrintScrn:
					gpSet->isSendPrintScrn = IsChecked(hWnd2, cbSendPrintScrn); break;
				case cbSendCtrlEsc:
					gpSet->isSendCtrlEsc = IsChecked(hWnd2, cbSendCtrlEsc); break;
			}
			gpConEmu->UpdateWinHookSettings();
			break;

		case rbHotkeysAll:
		case rbHotkeysUser:
		case rbHotkeysSystem:
		case rbHotkeysMacros:
			gpSetCls->FillHotKeysList(hWnd2, TRUE);
			break;

			
		case cbInstallKeybHooks:
			switch (IsChecked(hWnd2,cbInstallKeybHooks))
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
				checkDlgButton(hWnd2, cbDosBox, BST_CHECKED);
				EnableWindow(GetDlgItem(hWnd2, cbDosBox), FALSE); // изменение пока запрещено
			}
			else
			{
				checkDlgButton(hWnd2, cbDosBox, BST_UNCHECKED);
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

			switch (IsChecked(hWnd2,CB))
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
			gpSet->AppStd.isExtendColors = IsChecked(hWnd2, cbExtendColors) == BST_CHECKED ? true : false;

			for(int i=16; i<32; i++) //-V112
				EnableWindow(GetDlgItem(hWnd2, tc0+i), gpSet->AppStd.isExtendColors);

			EnableWindow(GetDlgItem(hWnd2, lbExtendIdx), gpSet->AppStd.isExtendColors);

			if (lParam)
			{
				gpConEmu->Update(true);
			}

			break;
		case cbColorSchemeSave:
		case cbColorSchemeDelete:
			{
				HWND hList = GetDlgItem(hWnd2, lbDefaultColors);
				int nLen = GetWindowTextLength(hList);
				if (nLen < 1)
					break;
				wchar_t* pszName = (wchar_t*)malloc((nLen+1)*sizeof(wchar_t));
				GetWindowText(hList, pszName, nLen+1);
				if (*pszName != L'<')
				{
					if (CB == cbColorSchemeSave)
						gpSet->PaletteSaveAs(pszName);
					else
						gpSet->PaletteDelete(pszName);
				}
				// Поставить фокус в список, а то кнопки могут "задизэблиться"
				SetFocus(hList);
				HWND hCB = GetDlgItem(hWnd2, CB);
				SetWindowLongPtr(hCB, GWL_STYLE, GetWindowLongPtr(hCB, GWL_STYLE) & ~BS_DEFPUSHBUTTON);
				// Перетряхнуть
				OnInitDialog_Color(hWnd2);
			} // cbColorSchemeSave, cbColorSchemeDelete
			break;
		case cbTrueColorer:
			gpSet->isTrueColorer = IsChecked(hWnd2, cbTrueColorer);
			gpConEmu->UpdateFarSettings();
			gpConEmu->Update(true);
			break;
		case cbFadeInactive:
			gpSet->isFadeInactive = IsChecked(hWnd2, cbFadeInactive);
			CVConGroup::InvalidateAll();
			break;
		case cbTransparent:
			{
				int newV = gpSet->nTransparent;

				if (IsChecked(hWnd2, cbTransparent))
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
					SendDlgItemMessage(hWnd2, slTransparent, TBM_SETPOS, (WPARAM) true, (LPARAM)gpSet->nTransparent);
					if (!gpSet->isTransparentSeparate)
						SendDlgItemMessage(hWnd2, slTransparentInactive, TBM_SETPOS, (WPARAM) true, (LPARAM) gpSet->nTransparent);
					gpConEmu->OnTransparent();
				}
			} break;
		case cbTransparentSeparate:
			{
				gpSet->isTransparentSeparate = IsChecked(hWnd2, cbTransparentSeparate);
				//EnableWindow(GetDlgItem(hWnd2, cbTransparentInactive), gpSet->isTransparentSeparate);
				EnableWindow(GetDlgItem(hWnd2, slTransparentInactive), gpSet->isTransparentSeparate);
				EnableWindow(GetDlgItem(hWnd2, stTransparentInactive1), gpSet->isTransparentSeparate);
				EnableWindow(GetDlgItem(hWnd2, stTransparentInactive2), gpSet->isTransparentSeparate);
				//checkDlgButton(hWnd2, cbTransparentInactive, (gpSet->nTransparentInactive!=255) ? BST_CHECKED : BST_UNCHECKED);
				SendDlgItemMessage(hWnd2, slTransparentInactive, TBM_SETPOS, (WPARAM) true, (LPARAM) gpSet->isTransparentSeparate ? gpSet->nTransparentInactive : gpSet->nTransparent);
				gpConEmu->OnTransparent();
			} break;
		//case cbTransparentInactive:
		//	{
		//		int newV = gpSet->nTransparentInactive;
		//		if (IsChecked(hWnd2, cbTransparentInactive))
		//		{
		//			if (newV == 255) newV = 200;
		//		}
		//		else
		//		{
		//			newV = 255;
		//		}
		//		if (newV != gpSet->nTransparentInactive)
		//		{
		//			gpSet->nTransparentInactive = newV;
		//			SendDlgItemMessage(hWnd2, slTransparentInactive, TBM_SETPOS, (WPARAM) true, (LPARAM)gpSet->nTransparentInactive);
		//			//gpConEmu->OnTransparent(); -- смысла нет, ConEmu сейчас "активен"
		//		}
		//	} break;
		case cbUserScreenTransparent:
			{
				gpSet->isUserScreenTransparent = IsChecked(hWnd2, cbUserScreenTransparent);

				if (hWnd2) checkDlgButton(hWnd2, cbHideCaptionAlways, gpSet->isHideCaptionAlways() ? BST_CHECKED : BST_UNCHECKED);
				EnableWindow(GetDlgItem(hWnd2, cbHideCaptionAlways), !gpSet->isForcedHideCaptionAlways());

				gpConEmu->OnHideCaption(); // при прозрачности - обязательно скрытие заголовка + кнопки
				gpConEmu->UpdateWindowRgn();
			} break;
		case cbColorKeyTransparent:
			{
				gpSet->isColorKeyTransparent = IsChecked(hWnd2, cbColorKeyTransparent);
				gpConEmu->OnTransparent();
			} break;


		/* *** Text selections options *** */
		case rbCTSNever:
		case rbCTSAlways:
		case rbCTSBufferOnly:
			gpSet->isConsoleTextSelection = (CB==rbCTSNever) ? 0 : (CB==rbCTSAlways) ? 1 : 2;
			//checkDlgButton(gpSetCls->hExt, cbConsoleTextSelection, gpSet->isConsoleTextSelection);
			break;
		case rbCTSActAlways: case rbCTSActBufferOnly:
			gpSet->isCTSActMode = (CB==rbCTSActAlways) ? 1 : 2;
			break;
		case cbCTSFreezeBeforeSelect:
			gpSet->isCTSFreezeBeforeSelect = IsChecked(hWnd2,CB);
			break;
		case cbCTSAutoCopy:
			gpSet->isCTSAutoCopy = IsChecked(hWnd2,CB);
			break;
		case cbCTSEndOnTyping:
			gpSet->isCTSEndOnTyping = IsChecked(hWnd2,CB);
			EnableWindow(GetDlgItem(hWnd2, cbCTSEndOnKeyPress), gpSet->isCTSEndOnTyping!=0);
			checkDlgButton(hWnd2, cbCTSEndOnKeyPress, gpSet->isCTSEndOnKeyPress);
			break;
		case cbCTSEndOnKeyPress:
			gpSet->isCTSEndOnKeyPress = IsChecked(hWnd2,CB);
			break;
		case cbCTSBlockSelection:
			gpSet->isCTSSelectBlock = IsChecked(hWnd2,CB);
			CheckSelectionModifiers();
			break;
		case cbCTSTextSelection:
			gpSet->isCTSSelectText = IsChecked(hWnd2,CB);
			CheckSelectionModifiers();
			break;
		case cbCTSDetectLineEnd:
			gpSet->AppStd.isCTSDetectLineEnd = IsChecked(hWnd2, CB);
			break;
		case cbCTSBashMargin:
			gpSet->AppStd.isCTSBashMargin = IsChecked(hWnd2, CB);
			break;
		case cbCTSTrimTrailing:
			gpSet->AppStd.isCTSTrimTrailing = IsChecked(hWnd2, CB);
			break;
		case cbCTSClickPromptPosition:
			gpSet->AppStd.isCTSClickPromptPosition = IsChecked(hWnd2,CB);
			CheckSelectionModifiers();
			break;
		case cbClipShiftIns:
			gpSet->AppStd.isPasteAllLines = IsChecked(hWnd2,CB);
			break;
		case cbClipCtrlV:
			gpSet->AppStd.isPasteFirstLine = IsChecked(hWnd2,CB);
			break;
		case cbClipConfirmEnter:
			gpSet->isPasteConfirmEnter = IsChecked(hWnd2,CB);
			break;
		case cbClipConfirmLimit:
			if (IsChecked(hWnd2,CB))
			{
				gpSet->nPasteConfirmLonger = gpSet->nPasteConfirmLonger ? gpSet->nPasteConfirmLonger : 200;
			}
			else
			{
				gpSet->nPasteConfirmLonger = 0;
			}
			SetDlgItemInt(hWnd2, tClipConfirmLimit, gpSet->nPasteConfirmLonger, FALSE);
			EnableWindow(GetDlgItem(hWnd2, tClipConfirmLimit), (gpSet->nPasteConfirmLonger != 0));
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
			gpSet->UpdSet.isUpdateConfirmDownload = (IsChecked(hWnd2, CB) == BST_UNCHECKED);
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

		/* *** Command groups *** */
		case cbCmdTasksAdd:
		case cbCmdTasksDel:
		case cbCmdTasksUp:
		case cbCmdTasksDown:
		case cbCmdGroupApp:
		case cbCmdTasksParm:
		case cbCmdTasksDir:
		case cbCmdTasksReload:
		case cbCmdTaskbarTasks:
		case cbCmdTaskbarCommands:
		case cbCmdTaskbarUpdate:
			return OnButtonClicked_Tasks(hWnd2, wParam, lParam);
		/* *** Command groups *** */

		case bGotoEditorCmd:
			{
				wchar_t szPath[MAX_PATH+1] = {};
				wchar_t szInitialDir[MAX_PATH+1]; GetCurrentDirectory(countof(szInitialDir), szInitialDir);
				lstrcpyn(szPath, gpSet->sFarGotoEditor, countof(szPath));
				OPENFILENAME ofn = {sizeof(ofn)};
				ofn.hwndOwner = ghOpWnd;
				ofn.lpstrFilter = L"Executables (*.exe)\0*.exe\0\0";
				ofn.nFilterIndex = 1;
				ofn.lpstrFile = szPath;
				ofn.nMaxFile = countof(szPath);
				ofn.lpstrInitialDir = szInitialDir;
				ofn.lpstrTitle = L"Choose file editor";
				ofn.lpstrDefExt = L"exe";
				ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
					| OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY;

				if (GetSaveFileName(&ofn))
				{
					SetDlgItemText(hWnd2, tGotoEditorCmd, szPath);
				}
			}
			break;

		default:
		{
			//if (CB >= cbHostWin && CB <= cbHostRShift)
			//{
			//	//memset(gpSet->mn_HostModOk, 0, sizeof(gpSet->mn_HostModOk));
			//	DWORD nModifers = 0;
			//	//if (IsChecked(hWnd2, CB))
			//	//{
			//	//	nModifers = gpSet->XorModifier(nModifers, HostkeyCtrlId2Vk(CB));
			//	//}
			//	for (UINT i = 0; i < 3; i++)
			//	{
			//		BYTE vk = 0;
			//		GetListBoxByte(hWnd2,lbHotKeyMod1+i,SettingsNS::szModifiers,SettingsNS::nModifiers,vk);
			//		nModifers = gpSet->SetModifier(nModifers, vk, false);
			//		//if ((CB != HostkeyCtrlIds[i]) && IsChecked(hWnd2, HostkeyCtrlIds[i]))
			//		//	nModifers = gpSet->XorModifier(nModifers, HostkeyCtrlId2Vk(HostkeyCtrlIds[i]));
			//		//	//gpSet->CheckHostkeyModifier(HostkeyCtrlId2Vk(HostkeyCtrlIds[i]));
			//	}
			//	if (mp_ActiveHotKey && (mp_ActiveHotKey->HkType != chk_Modifier) && (mp_ActiveHotKey->HkType != chk_System))
			//	{
			//		if (mp_ActiveHotKey->VkModPtr)
			//		{
			//			*mp_ActiveHotKey->VkModPtr = (cvk_VK_MASK & *mp_ActiveHotKey->VkModPtr) | nModifers;
			//		}
			//		else if (mp_ActiveHotKey->HkType == chk_Hostkey)
			//		{
			//			// Для этой группы - модификаторы назначаются "чохом"
			//			for (int i = 0; m_HotKeys[i].DescrLangID; i++)
			//			{
			//				if (m_HotKeys[i].HkType == chk_Hostkey)
			//				{
			//					_ASSERTE(m_HotKeys[i].VkModPtr == NULL && m_HotKeys[i].VkMod != 0);
			//					if (m_HotKeys[i].VkMod)
			//					{
			//						m_HotKeys[i].VkMod = (cvk_VK_MASK & m_HotKeys[i].VkMod) | nModifers;
			//					}
			//				}
			//			}
			//		}
			//		else if (mp_ActiveHotKey->VkMod)
			//		{
			//			mp_ActiveHotKey->VkMod = (cvk_VK_MASK & mp_ActiveHotKey->VkMod) | nModifers;
			//		}
			//	}
			//	//gpSet->TrimHostkeys();
			//	WARNING("ConEmuHotKey: Убрать nMultiHotkeyModifier как пережиток");
			//	_ASSERTE((nModifers & 0xFF) == 0); // Модификаторы должны быть в старших 3-х байтах
			//	gpSet->nMultiHotkeyModifier = (nModifers >> 8); // а тут они хранятся в младших (так сложилось)
			//	//if (IsChecked(hWnd2, CB))
			//	//{
			//	//	gpSet->CheckHostkeyModifier(HostkeyCtrlId2Vk(CB));
			//	//	gpSet->TrimHostkeys();
			//	//}
			//	// Обновить, что осталось
			//	//gpSetCls->SetupHotkeyChecks(hWnd2);
			//	//gpSet->MakeHostkeyModifier();
			//} // if (CB >= cbHostWin && CB <= cbHostRShift)
			//else
			if (CB >= c32 && CB <= c34)
			{
				if (gpSetCls->ColorEditDialog(hWnd2, CB))
				{
					if (CB == c32)
					{
						gpSet->ThSet.crBackground.UseIndex = 0;
						checkRadioButton(hWnd2, rbThumbBackColorIdx, rbThumbBackColorRGB, rbThumbBackColorRGB);
					}
					else if (CB == c33)
					{
						gpSet->ThSet.crPreviewFrame.UseIndex = 0;
						checkRadioButton(hWnd2, rbThumbPreviewBoxColorIdx, rbThumbPreviewBoxColorRGB, rbThumbPreviewBoxColorRGB);
					}
					else if (CB == c34)
					{
						gpSet->ThSet.crSelectFrame.UseIndex = 0;
						checkRadioButton(hWnd2, rbThumbSelectionBoxColorIdx, rbThumbSelectionBoxColorRGB, rbThumbSelectionBoxColorRGB);
					}

					InvalidateRect(GetDlgItem(hWnd2, CB), 0, 1);
					// done
				}
			} // else if (CB >= c32 && CB <= c34)
			else if (CB >= c35 && CB <= c37)
			{
				if (ColorEditDialog(hWnd2, CB))
				{
					gpConEmu->mp_Status->UpdateStatusBar(true);
				}
			} // if (CB >= c35 && CB <= c37)
			else if (CB == c38)
			{
				if (ColorEditDialog(hWnd2, CB))
				{
					gpConEmu->OnTransparent();
				}
			} // if (CB == c38)
			else if (CB >= c0 && CB <= MAX_COLOR_EDT_ID)
			{
				if (ColorEditDialog(hWnd2, CB))
				{
					//gpConEmu->m_Back->Refresh();
					gpConEmu->Update(true);
				}
			} // else if (CB >= c0 && CB <= MAX_COLOR_EDT_ID)
			else if (hWnd2 && (hWnd2 == mh_Tabs[thi_Status]))
			{
				/* *** Status bar options *** */
				if (CB == cbShowStatusBar)
				{
					gpConEmu->StatusCommand(csc_ShowHide, IsChecked(hWnd2,CB) ? 1 : 2);
				}
				else if (CB == cbStatusVertSep)
				{
					if (IsChecked(hWnd2,CB))
						gpSet->isStatusBarFlags |= csf_VertDelim;
					else
						gpSet->isStatusBarFlags &= ~csf_VertDelim;
					gpConEmu->mp_Status->UpdateStatusBar(true);
				}
				else if (CB == cbStatusHorzSep)
				{
					if (IsChecked(hWnd2,CB))
						gpSet->isStatusBarFlags |= csf_HorzDelim;
					else
						gpSet->isStatusBarFlags &= ~csf_HorzDelim;
					gpConEmu->mp_Status->UpdateStatusBar(true);
				}
				else
				{
					for (size_t i = 0; i < countof(SettingsNS::StatusItems); i++)
					{
						if (CB == SettingsNS::StatusItems[i].nDlgID)
						{
							gpSet->isStatusColumnHidden[SettingsNS::StatusItems[i].stItem] = !IsChecked(hWnd2,CB);
							gpConEmu->mp_Status->UpdateStatusBar(true);
							break;
						}
					}
				}
				/* *** Status bar options *** */
			} // else if (hWnd2 && (hWnd2 == mh_Tabs[thi_Status]))

			else
			{
				_ASSERTE(FALSE && "Button click was not processed");
			}

		} // default:
	}

	return 0;
}

LRESULT CSettings::OnButtonClicked_Tasks(HWND hWnd2, WPARAM wParam, LPARAM lParam)
{
	WORD CB = LOWORD(wParam);

	switch (CB)
	{
	case cbCmdTasksAdd:
		{
			int iCount = (int)SendDlgItemMessage(hWnd2, lbCmdTasks, LB_GETCOUNT, 0,0);
			if (!gpSet->CmdTaskGet(iCount))
				gpSet->CmdTaskSet(iCount, L"", L"", L"");

        	OnInitDialog_Tasks(hWnd2, false);

        	SendDlgItemMessage(hWnd2, lbCmdTasks, LB_SETCURSEL, iCount, 0);
        	OnComboBox(hWnd2, MAKELONG(lbCmdTasks,LBN_SELCHANGE), 0);
		} // cbCmdTasksAdd
		break;

	case cbCmdTasksDel:
		{
			int iCur = (int)SendDlgItemMessage(hWnd2, lbCmdTasks, LB_GETCURSEL, 0,0);
			if (iCur < 0)
				break;

			const Settings::CommandTasks* p = gpSet->CmdTaskGet(iCur);
            if (!p)
            	break;

			bool bIsStartup = false;
			if (gpSet->psStartTasksName && p->pszName && (lstrcmpi(gpSet->psStartTasksName, p->pszName) == 0))
				bIsStartup = true;

			size_t cchMax = (p->pszName ? _tcslen(p->pszName) : 0) + 200;
			wchar_t* pszMsg = (wchar_t*)malloc(cchMax*sizeof(*pszMsg));
			if (!pszMsg)
				break;

			_wsprintf(pszMsg, SKIPLEN(cchMax) L"%sDelete command group %s?",
				bIsStartup ? L"Warning! You about to delete startup task!\n\n" : L"",
				p->pszName ? p->pszName : L"{???}");

			int nBtn = MessageBox(pszMsg, MB_YESNO|(bIsStartup ? MB_ICONEXCLAMATION : MB_ICONQUESTION), NULL, ghOpWnd);
			SafeFree(pszMsg);

            if (nBtn != IDYES)
            	break;

        	gpSet->CmdTaskSet(iCur, NULL, NULL, NULL);

			if (bIsStartup && gpSet->psStartTasksName)
				*gpSet->psStartTasksName = 0;

        	OnInitDialog_Tasks(hWnd2, false);

        	int iCount = (int)SendDlgItemMessage(hWnd2, lbCmdTasks, LB_GETCOUNT, 0,0);
        	SendDlgItemMessage(hWnd2, lbCmdTasks, LB_SETCURSEL, min(iCur,(iCount-1)), 0);
        	OnComboBox(hWnd2, MAKELONG(lbCmdTasks,LBN_SELCHANGE), 0);

		} // cbCmdTasksDel
		break;

	case cbCmdTasksUp:
	case cbCmdTasksDown:
		{
			int iCur, iChg;
			iCur = (int)SendDlgItemMessage(hWnd2, lbCmdTasks, LB_GETCURSEL, 0,0);
			if (iCur < 0)
				break;
			if (CB == cbCmdTasksUp)
			{
				if (!iCur)
					break;
            	iChg = iCur - 1;
        	}
        	else
        	{
        		iChg = iCur + 1;
        		if (iChg >= (int)SendDlgItemMessage(hWnd2, lbCmdTasks, LB_GETCOUNT, 0,0))
        			break;
        	}

        	if (!gpSet->CmdTaskXch(iCur, iChg))
        		break;

        	OnInitDialog_Tasks(hWnd2, false);

        	SendDlgItemMessage(hWnd2, lbCmdTasks, LB_SETCURSEL, iChg, 0);
		} // cbCmdTasksUp, cbCmdTasksDown
		break;

	case cbCmdGroupApp:
		{
			// Добавить команду в группу
			RConStartArgs args;
			args.aRecreate = cra_EditTab;
			int nDlgRc = gpConEmu->RecreateDlg(&args);

			if (nDlgRc == IDC_START)
			{
				wchar_t* pszCmd = args.CreateCommandLine();
				if (!pszCmd || !*pszCmd)
				{
					DisplayLastError(L"Can't compile command line for new tab\nAll fields are empty?", -1);
				}
				else
				{
					//SendDlgItemMessage(hWnd2, tCmdGroupCommands, EM_REPLACESEL, TRUE, (LPARAM)pszName);
					wchar_t* pszFull = GetDlgItemText(hWnd2, tCmdGroupCommands);
					if (!pszFull || !*pszFull)
					{
						SafeFree(pszFull);
						pszFull = pszCmd;
					}
					else
					{
						size_t cchLen = _tcslen(pszFull);
						size_t cchMax = cchLen + 7 + _tcslen(pszCmd);
						pszFull = (wchar_t*)realloc(pszFull, cchMax*sizeof(*pszFull));

						int nRN = 0;
						if (cchLen >= 2)
						{
							if (pszFull[cchLen-2] == L'\r' && pszFull[cchLen-1] == L'\n')
							{
								nRN++;
								if (cchLen >= 4)
								{
									if (pszFull[cchLen-4] == L'\r' && pszFull[cchLen-3] == L'\n')
									{
										nRN++;
									}
								}
							}
						}

						if (nRN < 2)
							_wcscat_c(pszFull, cchMax, nRN ? L"\r\n" : L"\r\n\r\n");

						_wcscat_c(pszFull, cchMax, pszCmd);
					}

					if (pszFull)
					{
						SetDlgItemText(hWnd2, tCmdGroupCommands, pszFull);
						OnEditChanged(hWnd2, MAKELPARAM(tCmdGroupCommands,EN_CHANGE), 0);
					}
					else
					{
						_ASSERTE(pszFull);
					}
					SafeFree(pszFull);
				}
				SafeFree(pszCmd);
			}
		}
		break;

	case cbCmdTasksParm:
		{
			// Добавить файл
			wchar_t temp[MAX_PATH+10] = {};
			OPENFILENAME ofn = {sizeof(ofn)};
			ofn.hwndOwner = ghOpWnd;
			ofn.lpstrFilter = L"All files (*.*)\0*.*\0Text files (*.txt,*.ini,*.log)\0*.txt;*.ini;*.log\0Executables (*.exe,*.com,*.bat,*.cmd)\0*.exe;*.com;*.bat;*.cmd\0Scripts (*.vbs,*.vbe,*.js,*.jse)\0*.vbs;*.vbe;*.js;*.jse\0\0";
			//ofn.lpstrFilter = L"All files (*.*)\0*.*\0\0";
			ofn.lpstrFile = temp+1;
			ofn.nMaxFile = countof(temp)-10;
			ofn.lpstrTitle = L"Choose file";
			ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
			            | OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST;

			if (GetOpenFileName(&ofn))
			{
				LPCWSTR pszName = temp+1;
				if (wcschr(pszName, L' '))
				{
					temp[0] = L'"';
					wcscat_c(temp, L"\"");
					pszName = temp;
				}
				else
				{
					temp[0] = L' ';
				}
				//wcscat_c(temp, L"\r\n\r\n");

				SendDlgItemMessage(hWnd2, tCmdGroupCommands, EM_REPLACESEL, TRUE, (LPARAM)pszName);
			}
		} // cbCmdTasksParm
		break;

	case cbCmdTasksDir:
		{
			TODO("Извлечь текущий каталог запуска");

			BROWSEINFO bi = {ghOpWnd};
			wchar_t szFolder[MAX_PATH+1] = {0};
			TODO("Извлечь текущий каталог запуска");
			bi.pszDisplayName = szFolder;
			wchar_t szTitle[100];
			bi.lpszTitle = wcscpy(szTitle, L"Choose tab startup directory");
			bi.ulFlags = BIF_EDITBOX | BIF_RETURNONLYFSDIRS | BIF_VALIDATE;
			bi.lpfn = CRecreateDlg::BrowseCallbackProc;
			bi.lParam = (LPARAM)szFolder;
			LPITEMIDLIST pRc = SHBrowseForFolder(&bi);

			if (pRc)
			{
				if (SHGetPathFromIDList(pRc, szFolder))
				{
					wchar_t szFull[MAX_PATH+32];
					bool bQuot = wcschr(szFolder, L' ') != NULL;
					wcscpy_c(szFull, bQuot ? L" \"-new_console:d:" : L" -new_console:d:");
					wcscat_c(szFull, szFolder);
					if (bQuot)
						wcscat_c(szFull, L"\"");
				
					SendDlgItemMessage(hWnd2, tCmdGroupCommands, EM_REPLACESEL, TRUE, (LPARAM)szFull);
				}

				CoTaskMemFree(pRc);
			}
		}
		break;

	case cbCmdTasksReload:
		{
			if (MessageBox(ghOpWnd, L"Warning! All unsaved changes will be lost!\n\nReload command groups from settings?",
					gpConEmu->GetDefaultTitle(), MB_YESNO|MB_ICONEXCLAMATION) != IDYES)
				break;

			// Обновить группы команд
			gpSet->LoadCmdTasks(NULL, true);

			// Обновить список на экране
			OnInitDialog_Tasks(hWnd2, true);
		} // cbCmdTasksReload
		break;

	case cbCmdTaskbarTasks:
		gpSet->isStoreTaskbarkTasks = IsChecked(hWnd2, CB);
		break;
	case cbCmdTaskbarCommands:
		gpSet->isStoreTaskbarCommands = IsChecked(hWnd2, CB);
		break;
	case cbCmdTaskbarUpdate:
		if (!gpSet->SaveCmdTasks(NULL))
		{
			LPCWSTR pszMsg = L"Can't save task list to settings!\r\nJump list may be not working!\r\nUpdate Windows 7 task list now?";
			if (MessageBox(ghOpWnd, pszMsg, gpConEmu->GetDefaultTitle(), MB_ICONEXCLAMATION|MB_YESNO|MB_DEFBUTTON2) != IDYES)
				break; // Обновлять таскбар не будем
		}
		UpdateWin7TaskList(true);
		break;
	}

	return 0;
}

// Возвращает TRUE, если значение распознано и отличается от старого
BOOL CSettings::GetColorRef(HWND hDlg, WORD TB, COLORREF* pCR)
{
	BOOL result = FALSE;
	int r = 0, g = 0, b = 0;
	wchar_t temp[MAX_PATH];
	wchar_t *pch, *pchEnd;

	if (!GetDlgItemText(hDlg, TB, temp, countof(temp)) || !*temp)
		return FALSE;

	if ((temp[0] == L'#') || (temp[0] == L'x' || temp[0] == L'X') || (temp[0] == L'0' && (temp[1] == L'x' || temp[1] == L'X')))
	{
		pch = (temp[0] == L'0') ? (temp+2) : (temp+1);
		// Считаем значение 16-ричным rgb кодом
		pchEnd = NULL;
		COLORREF clr = wcstoul(pch, &pchEnd, 16);
		if (clr && (temp[0] == L'#'))
		{
			// "#rrggbb", обменять местами rr и gg, нам нужен COLORREF (bbggrr)
			clr = ((clr & 0xFF)<<16) | ((clr & 0xFF00)) | ((clr & 0xFF0000)>>16);
		}
		// Done
		if (pchEnd && (pchEnd > (temp+1)) && (clr <= 0xFFFFFF) && (*pCR != clr))
		{
			*pCR = clr;
			result = TRUE;
		}
	}
	else
	{
		pch = (wchar_t*)wcspbrk(temp, L"0123456789");
		pchEnd = NULL;
		r = pch ? wcstol(pch, &pchEnd, 10) : 0;
		if (pchEnd && (pchEnd > pch))
		{
			pch = (wchar_t*)wcspbrk(pchEnd, L"0123456789");
			pchEnd = NULL;
			g = pch ? wcstol(pch, &pchEnd, 10) : 0;

			if (pchEnd && (pchEnd > pch))
			{
				pch = (wchar_t*)wcspbrk(pchEnd, L"0123456789");
				pchEnd = NULL;
				b = pch ? wcstol(pch, &pchEnd, 10) : 0;
			}

			// Достаточно ввода одной компоненты
			if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255 && *pCR != RGB(r, g, b))
			{
				*pCR = RGB(r, g, b);
				result = TRUE;
			}
		}
	}

	//TCHAR *sp1 = wcschr(temp, ' '), *sp2;

	//if (sp1 && *(sp1+1) && *(sp1+1) != ' ')
	//{
	//	sp2 = wcschr(sp1+1, ' ');

	//	if (sp2 && *(sp2+1) && *(sp2+1) != ' ')
	//	{
	//		*sp1 = 0;
	//		sp1++;
	//		*sp2 = 0;
	//		sp2++;
	//		r = klatoi(temp); g = klatoi(sp1), b = klatoi(sp2);

	//		if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255 && *pCR != RGB(r, g, b))
	//		{
	//			*pCR = RGB(r, g, b);
	//			result = TRUE;
	//		}
	//	}
	//}

	return result;
}

LRESULT CSettings::OnEditChanged(HWND hWnd2, WPARAM wParam, LPARAM lParam)
{
	_ASSERTE(hWnd2!=NULL);
	if (mb_IgnoreEditChanged)
		return 0;

	WORD TB = LOWORD(wParam);

	switch (TB)
	{
	case tBgImage:
	{
		wchar_t temp[MAX_PATH];
		GetDlgItemText(hWnd2, tBgImage, temp, countof(temp));

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
		GetDlgItemText(hWnd2, tBgImageColors, temp, countof(temp)-1);
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

	case tWndX:
	case tWndY:
	case tWndWidth:
	case tWndHeight:
	{
		if (IsChecked(hWnd2, rNormal) == BST_CHECKED)
			EnableWindow(GetDlgItem(hWnd2, cbApplyPos), TRUE);
		break;
	} // case tWndX: case tWndY: case tWndWidth: case tWndHeight:
	
	case tDarker:
	{
		DWORD newV;
		TCHAR tmp[10];
		GetDlgItemText(hWnd2, tDarker, tmp, countof(tmp));
		newV = klatoi(tmp);

		if (newV < 256 && newV != gpSet->bgImageDarker)
		{
			gpSet->bgImageDarker = newV;
			SendDlgItemMessage(hWnd2, slDarker, TBM_SETPOS, (WPARAM) true, (LPARAM) gpSet->bgImageDarker);

			// Картинку может установить и плагин
			if (gpSet->isShowBgImage && gpSet->sBgImage[0])
				LoadBackgroundFile(gpSet->sBgImage);
			else
				NeedBackgroundUpdate();

			gpConEmu->Update(true);
		}
		break;
	} //case tDarker:

	case tCursorFixedSize:
	{
		BOOL lbOk = FALSE;
		UINT nNewVal = GetDlgItemInt(hWnd2, tCursorFixedSize, &lbOk, FALSE);
		if (lbOk)
		{
			if ((nNewVal >= CURSORSIZE_MIN) && (nNewVal <= CURSORSIZE_MAX) && (gpSet->AppStd.nCursorFixedSize != nNewVal))
			{
				gpSet->AppStd.nCursorFixedSize = nNewVal;
				gpConEmu->Update(true);
			}
		}

		break;
	} //case tCursorFixedSize:
	
	case tCursorMinSize:
	{
		BOOL lbOk = FALSE;
		UINT nNewVal = GetDlgItemInt(hWnd2, tCursorMinSize, &lbOk, FALSE);
		if (lbOk)
		{
			if ((nNewVal >= CURSORSIZEPIX_MIN) && (nNewVal <= CURSORSIZEPIX_MAX) && (gpSet->AppStd.nCursorMinSize != nNewVal))
			{
				gpSet->AppStd.nCursorMinSize = nNewVal;
				gpConEmu->Update(true);
			}
		}

		break;
	} //case tCursorMinSize:
	
	case tLongOutputHeight:
	{
		BOOL lbOk = FALSE;
		wchar_t szTemp[16];
		UINT nNewVal = GetDlgItemInt(hWnd2, tLongOutputHeight, &lbOk, FALSE);

		if (lbOk)
		{
			if (nNewVal >= LONGOUTPUTHEIGHT_MIN && nNewVal <= LONGOUTPUTHEIGHT_MAX)
				gpSet->DefaultBufferHeight = nNewVal;
			else if (nNewVal > LONGOUTPUTHEIGHT_MAX)
				SetDlgItemInt(hWnd2, TB, gpSet->DefaultBufferHeight, FALSE);
		}
		else
		{
			SetDlgItemText(hWnd2, TB, _ltow(gpSet->DefaultBufferHeight, szTemp, 10));
		}
		break;
	} //case tLongOutputHeight:

	case tComspecExplicit:
	{
		GetDlgItemText(hWnd2, tComspecExplicit, gpSet->ComSpec.ComspecExplicit, countof(gpSet->ComSpec.ComspecExplicit));
		break;
	} //case tComspecExplicit:
	
	//case hkNewConsole:
	//case hkSwitchConsole:
	//case hkCloseConsole:
	//{
	//	UINT nHotKey = 0xFF & SendDlgItemMessage(hWnd2, TB, HKM_GETHOTKEY, 0, 0);

	//	if (TB == hkNewConsole)
	//		gpSet->icMultiNew = nHotKey;
	//	else if (TB == hkSwitchConsole)
	//		gpSet->icMultiNext = nHotKey;
	//	else if (TB == hkCloseConsole)
	//		gpSet->icMultiRecreate = nHotKey;

	//	// SendDlgItemMessage(hWnd2, hkMinimizeRestore, HKM_SETHOTKEY, gpSet->icMinimizeRestore, 0);
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
		if (mp_ActiveHotKey && mp_ActiveHotKey->CanChangeVK())
		{
			DWORD nCurMods = (CEHOTKEY_MODMASK & mp_ActiveHotKey->VkMod);
			if (!nCurMods)
				nCurMods = CEHOTKEY_NOMOD;
			mp_ActiveHotKey->VkMod = nHotKey | nCurMods;
			FillHotKeysList(hWnd2, FALSE);
		}
		break;
	} // case hkHotKeySelect:

	case tTabSkipWords:
	{
		GetDlgItemText(hWnd2, TB, gpSet->szTabSkipWords, countof(gpSet->szTabSkipWords));
		gpConEmu->mp_TabBar->Update(TRUE);
		break;
	}
	case tTabConsole:
	case tTabPanels:
	case tTabViewer:
	case tTabEditor:
	case tTabEditorMod:
	{
		wchar_t temp[MAX_PATH]; temp[0] = 0;

		if (GetDlgItemText(hWnd2, TB, temp, countof(temp)) && temp[0])
		{
			temp[31] = 0; // страховка

			if (wcsstr(temp, L"%s") || wcsstr(temp, L"%n"))
			{
				if (TB == tTabConsole)
					wcscpy_c(gpSet->szTabConsole, temp);
				else if (TB == tTabPanels)
					wcscpy_c(gpSet->szTabPanels, temp);
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
		DWORD n = GetDlgItemInt(hWnd2, tTabLenMax, &lbOk, FALSE);

		if (n > 10 && n < CONEMUTABMAX)
		{
			gpSet->nTabLenMax = n;
			gpConEmu->mp_TabBar->Update(TRUE);
		}
		break;
	} // case tTabLenMax:
	
	case tAdminSuffix:
	{
		GetDlgItemText(hWnd2, tAdminSuffix, gpSet->szAdminTitleSuffix, countof(gpSet->szAdminTitleSuffix));
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

	case tClipConfirmLimit:
	{
		BOOL lbValOk = FALSE;
		gpSet->nPasteConfirmLonger = GetDlgItemInt(hWnd2, tClipConfirmLimit, &lbValOk, FALSE);
		if (IsChecked(hWnd2, cbClipConfirmLimit) != (gpSet->nPasteConfirmLonger!=0))
			checkDlgButton(hWnd2, cbClipConfirmLimit, (gpSet->nPasteConfirmLonger!=0));
		if (lbValOk && (gpSet->nPasteConfirmLonger == 0))
		{
			SetFocus(GetDlgItem(hWnd2, cbClipConfirmLimit));
			EnableWindow(GetDlgItem(hWnd2, tClipConfirmLimit), FALSE);
		}
		break;
	} // case tClipConfirmLimit:

	
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


	/* *** Command groups *** */
	case tCmdGroupName:
	case tCmdGroupGuiArg:
	case tCmdGroupCommands:
		{
			if (mb_IgnoreCmdGroupEdit)
				break;

			int iCur = (int)SendDlgItemMessage(hWnd2, lbCmdTasks, LB_GETCURSEL, 0,0);
			if (iCur < 0)
				break;

			HWND hName = GetDlgItem(hWnd2, tCmdGroupName);
			HWND hGuiArg = GetDlgItem(hWnd2, tCmdGroupGuiArg);
			HWND hCmds = GetDlgItem(hWnd2, tCmdGroupCommands);
			size_t nNameLen = GetWindowTextLength(hName);
			wchar_t *pszName = (wchar_t*)calloc(nNameLen+1,sizeof(wchar_t));
			size_t nGuiLen = GetWindowTextLength(hGuiArg);
			wchar_t *pszGuiArgs = (wchar_t*)calloc(nGuiLen+1,sizeof(wchar_t));
			size_t nCmdsLen = GetWindowTextLength(hCmds);
			wchar_t *pszCmds = (wchar_t*)calloc(nCmdsLen+1,sizeof(wchar_t));

			if (pszName && pszCmds)
			{
				GetWindowText(hName, pszName, (int)(nNameLen+1));
				GetWindowText(hGuiArg, pszGuiArgs, (int)(nGuiLen+1));
				GetWindowText(hCmds, pszCmds, (int)(nCmdsLen+1));

				gpSet->CmdTaskSet(iCur, pszName, pszGuiArgs, pszCmds);

				mb_IgnoreCmdGroupList = true;
        		OnInitDialog_Tasks(hWnd2, false);
        		SendDlgItemMessage(hWnd2, lbCmdTasks, LB_SETCURSEL, iCur, 0);
        		mb_IgnoreCmdGroupList = false;
			}

			SafeFree(pszName);
			SafeFree(pszCmds);
		}
		break;
	/* *** Command groups *** */


	/* *** GUI Macro - hotkeys *** */
	case tGuiMacro:
		if (mp_ActiveHotKey && (mp_ActiveHotKey->HkType == chk_Macro))
		{
			MyGetDlgItemText(hWnd2, tGuiMacro, mp_ActiveHotKey->cchGuiMacroMax, mp_ActiveHotKey->GuiMacro, true);
			FillHotKeysList(hWnd2, FALSE);
		}
		break;


	case tGotoEditorCmd:
		{
			size_t cchMax = gpSet->sFarGotoEditor ? (lstrlen(gpSet->sFarGotoEditor)+1) : 0;
			MyGetDlgItemText(hWnd2, tGotoEditorCmd, cchMax, gpSet->sFarGotoEditor);
		}
		break;

	
	default:
	
		if (hWnd2 == mh_Tabs[thi_Views])
		{
			BOOL bValOk = FALSE;
			UINT nVal = GetDlgItemInt(hWnd2, TB, &bValOk, FALSE);

			if (bValOk)
			{
				switch (TB)
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
			
			if (TB >= tc32 && TB <= tc38)
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
		} // if (hWnd2 == mh_Tabs[thi_Views])
		else if (hWnd2 == mh_Tabs[thi_Colors])
		{
			COLORREF color = 0;

			if (TB == tFadeLow || TB == tFadeHigh)
			{
				BOOL lbOk = FALSE;
				UINT nVal = GetDlgItemInt(hWnd2, TB, &lbOk, FALSE);

				if (lbOk && nVal <= 255)
				{
					if (TB == tFadeLow)
						gpSet->mn_FadeLow = nVal;
					else
						gpSet->mn_FadeHigh = nVal;

					gpSet->ResetFadeColors();	
					//gpSet->mb_FadeInitialized = false;
					//gpSet->mn_LastFadeSrc = gpSet->mn_LastFadeDst = -1;
				}
			}
			else if (GetColorById(TB - (tc0-c0), &color))
			{
				if (GetColorRef(hWnd2, TB, &color))
				{
					if (SetColorById(TB - (tc0-c0), color))
					{
						gpConEmu->InvalidateAll();

						if (TB >= tc0 && TB <= tc31)
							gpConEmu->Update(true);

						InvalidateRect(GetDlgItem(hWnd2, TB - (tc0-c0)), 0, 1);
					}
				}
			}
		} // else if (hWnd2 == mh_Tabs[thi_Colors])
		else if (hWnd2 == mh_Tabs[thi_Status])
		{
			COLORREF color = 0;
			
			if ((TB >= tc35 && TB <= tc37)
				&& GetColorById(TB - (tc0-c0), &color))
			{
				if (GetColorRef(hWnd2, TB, &color))
				{
					if (SetColorById(TB - (tc0-c0), color))
					{
						InvalidateRect(GetDlgItem(hWnd2, TB - (tc0-c0)), 0, 1);
					}
				}
			}
		} // else if (hWnd2 == mh_Tabs[thi_Status])
		else if (hWnd2 == mh_Tabs[thi_Ext])
		{
			if (HIWORD(wParam) == EN_CHANGE)
			{
				WORD TB = LOWORD(wParam);
				BOOL lbOk = FALSE;
				UINT nNewVal = GetDlgItemInt(hWnd2, TB, &lbOk, FALSE);

				if (lbOk)
				{
					switch (TB)
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
					case tScrollAppearDelay:
						if (nNewVal >= SCROLLBAR_DELAY_MIN && nNewVal <= SCROLLBAR_DELAY_MAX)
							gpSet->nScrollBarAppearDelay = nNewVal;
						else if (nNewVal > SCROLLBAR_DELAY_MAX)
							SetDlgItemInt(hWnd2, tScrollAppearDelay, SCROLLBAR_DELAY_MAX, FALSE);
						break;
					case tScrollDisappearDelay:
						if (nNewVal >= SCROLLBAR_DELAY_MIN && nNewVal <= SCROLLBAR_DELAY_MAX)
							gpSet->nScrollBarDisappearDelay = nNewVal;
						else if (nNewVal > SCROLLBAR_DELAY_MAX)
							SetDlgItemInt(hWnd2, tScrollDisappearDelay, SCROLLBAR_DELAY_MAX, FALSE);
						break;
					case tPadSize:
						if (nNewVal >= CENTERCONSOLEPAD_MIN && nNewVal <= CENTERCONSOLEPAD_MAX)
							gpSet->nCenterConsolePad = nNewVal;
						else if (nNewVal > CENTERCONSOLEPAD_MAX)
							SetDlgItemInt(hWnd2, tPadSize, CENTERCONSOLEPAD_MAX, FALSE);
						// Если юзер ставит "бордюр" то нужно сразу включить опцию, чтобы он работал
						if (gpSet->nCenterConsolePad && !IsChecked(hWnd2, cbTryToCenter))
						{
							gpSet->isTryToCenter = true;
							checkDlgButton(hWnd2, cbTryToCenter, BST_CHECKED);
						}
						// Update window/console size
						if (gpSet->isTryToCenter)
							gpConEmu->OnSize();
					}
				}
			}
		} // else if (hWnd2 == mh_Tabs[thi_Ext])
		
	// end of default:
	} // switch (TB)

	return 0;
}

LRESULT CSettings::OnComboBox(HWND hWnd2, WPARAM wParam, LPARAM lParam)
{
	_ASSERTE(hWnd2!=NULL);
	WORD wId = LOWORD(wParam);
	
	switch (wId)
	{
	case tFontCharset:
	{
		gpSet->mb_CharSetWasSet = TRUE;
		PostMessage(hWnd2, gpSetCls->mn_MsgRecreateFont, wParam, 0);
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
			PostMessage(hWnd2, mn_MsgRecreateFont, wId, 0);
		else
			mn_LastChangingFontCtrlId = wId;
		break;
	}

	case lbBgPlacement:
	{
		BYTE bg = 0;
		GetListBoxByte(hWnd2,lbBgPlacement,SettingsNS::szBgOper,SettingsNS::nBgOper,bg);
		gpSet->bgOperation = bg;
		gpSetCls->LoadBackgroundFile(gpSet->sBgImage, true);
		gpConEmu->Update(true);
		break;
	}

	case lbLDragKey:
	{
		BYTE VkMod = 0;
		GetListBoxByte(hWnd2,wId,SettingsNS::szKeys,SettingsNS::nKeys,VkMod);
		gpSet->SetHotkeyById(vkLDragKey, VkMod);
		break;
	}
	case lbRDragKey:
	{
		BYTE VkMod = 0;
		GetListBoxByte(hWnd2,wId,SettingsNS::szKeys,SettingsNS::nKeys,VkMod);
		gpSet->SetHotkeyById(vkLDragKey, VkMod);
		break;
	}
	case lbNtvdmHeight:
	{
		INT_PTR num = SendDlgItemMessage(hWnd2, wId, CB_GETCURSEL, 0, 0);
		gpSet->ntvdmHeight = (num == 1) ? 25 : ((num == 2) ? 28 : ((num == 3) ? 43 : ((num == 4) ? 50 : 0))); //-V112
		break;
	}
	case lbCmdOutputCP:
	{
		gpSet->nCmdOutputCP = SendDlgItemMessage(hWnd2, wId, CB_GETCURSEL, 0, 0);

		if (gpSet->nCmdOutputCP == -1) gpSet->nCmdOutputCP = 0;

		gpConEmu->UpdateFarSettings();
		break;
	}
	case lbExtendFontNormalIdx:
	case lbExtendFontBoldIdx:
	case lbExtendFontItalicIdx:
	{
		if (wId == lbExtendFontNormalIdx)
			gpSet->AppStd.nFontNormalColor = GetNumber(hWnd2, wId);
		else if (wId == lbExtendFontBoldIdx)
			gpSet->AppStd.nFontBoldColor = GetNumber(hWnd2, wId);
		else if (wId == lbExtendFontItalicIdx)
			gpSet->AppStd.nFontItalicColor = GetNumber(hWnd2, wId);

		if (gpSet->AppStd.isExtendFonts)
			gpConEmu->Update(true);
		break;
	}

	case lbHotKeyList:
	{
		if (!mp_ActiveHotKey)
			break;

		BYTE vk = 0;

		//if (mp_ActiveHotKey && (mp_ActiveHotKey->Type == 0))
		//{
		//}
		//else
		//{
		//}

		//if (mp_ActiveHotKey && (mp_ActiveHotKey->Type == 0 || mp_ActiveHotKey->Type == 1) && mp_ActiveHotKey->VkPtr)
		if (mp_ActiveHotKey)
		{
			if (mp_ActiveHotKey->CanChangeVK())
			{
				GetListBoxByte(hWnd2, wId, SettingsNS::szKeysHot, SettingsNS::nKeysHot, vk);
				SendDlgItemMessage(hWnd2, hkHotKeySelect, HKM_SETHOTKEY, vk|(vk==VK_DELETE ? (HOTKEYF_EXT<<8) : 0), 0);
				DWORD nMod = (CEHOTKEY_MODMASK & mp_ActiveHotKey->VkMod);
				if (nMod == 0)
				{
					// Если модификатора вообще не было - ставим Win
					BYTE b = VK_LWIN;
					FillListBoxByte(hWnd2, lbHotKeyMod1, SettingsNS::szModifiers, SettingsNS::nModifiers, b);
					nMod = (VK_LWIN << 8);
				}
				mp_ActiveHotKey->VkMod = ((DWORD)vk) | nMod;
			}
			else if (mp_ActiveHotKey->HkType == chk_Modifier)
			{
				GetListBoxByte(hWnd2, wId, SettingsNS::szKeys, SettingsNS::nKeys, vk);
				SendDlgItemMessage(hWnd2, hkHotKeySelect, HKM_SETHOTKEY, 0, 0);
				mp_ActiveHotKey->VkMod = vk;
			}
			FillHotKeysList(hWnd2, FALSE);
		}

		break;
	} // lbHotKeyList

	case lbHotKeyMod1:
	case lbHotKeyMod2:
	case lbHotKeyMod3:
	{
		DWORD nModifers = 0;

		for (UINT i = 0; i < 3; i++)
		{
			BYTE vk = 0;
			GetListBoxByte(hWnd2,lbHotKeyMod1+i,SettingsNS::szModifiers,SettingsNS::nModifiers,vk);
			if (vk)
				nModifers = gpSet->SetModifier(nModifers, vk, false);
		}

		_ASSERTE((nModifers & 0xFF) == 0); // Модификаторы должны быть строго в старших 3-х байтах

		if (mp_ActiveHotKey && (mp_ActiveHotKey->HkType != chk_Modifier) && (mp_ActiveHotKey->HkType != chk_System))
		{
			//if (mp_ActiveHotKey->VkModPtr)
			//{
			//	*mp_ActiveHotKey->VkModPtr = (cvk_VK_MASK & *mp_ActiveHotKey->VkModPtr) | nModifers;
			//}
			//else 
			if (mp_ActiveHotKey->HkType == chk_NumHost)
			{
				if (!nModifers)
					nModifers = (VK_LWIN << 8);
				// Для этой группы - модификаторы назначаются "чохом"
				_ASSERTE((nModifers & 0xFF) == 0); // тут данные в старших трех байтах
				gpSet->nHostkeyNumberModifier = (nModifers >> 8); // а тут в младших трех
			}
			else if (mp_ActiveHotKey->HkType == chk_ArrHost)
			{
				if (!nModifers)
					nModifers = (VK_LWIN << 8);
				// Для этой группы - модификаторы назначаются "чохом"
				_ASSERTE((nModifers & 0xFF) == 0); // тут данные в старших трех байтах
				gpSet->nHostkeyArrowModifier = (nModifers >> 8); // а тут в младших трех
			}
			else //if (mp_ActiveHotKey->VkMod)
			{
				if (!nModifers)
					nModifers = CEHOTKEY_NOMOD;
				mp_ActiveHotKey->VkMod = (cvk_VK_MASK & mp_ActiveHotKey->VkMod) | nModifers;
			}
		}

		//if (mp_ActiveHotKey->HkType == chk_Hostkey)
		//{
		//	gpSet->nHostkeyModifier = (nModifers >> 8); // а тут они хранятся в младших (так сложилось)
		//}
		//else if (mp_ActiveHotKey->HkType == chk_Hostkey)
		//{
		//	gpSet->nHostkeyModifier = (nModifers >> 8); // а тут они хранятся в младших (так сложилось)
		//}

		FillHotKeysList(hWnd2, FALSE);

		break;
	} // lbHotKeyMod1, lbHotKeyMod2, lbHotKeyMod3

	case tTabFontFace:
	case tTabFontHeight:
	case tTabFontCharset:
	{
		if (HIWORD(wParam) == CBN_EDITCHANGE)
		{
			switch (wId)
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

			switch (wId)
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
	} // tTabFontFace, tTabFontHeight, tTabFontCharset

	case tStatusFontFace:
	case tStatusFontHeight:
	case tStatusFontCharset:
	{
		if (HIWORD(wParam) == CBN_EDITCHANGE)
		{
			switch (wId)
			{
			case tStatusFontFace:
				GetDlgItemText(hWnd2, wId, gpSet->sStatusFontFace, countof(gpSet->sStatusFontFace)); break;
			case tStatusFontHeight:
				gpSet->nStatusFontHeight = GetNumber(hWnd2, wId); break;
			}
			gpConEmu->mp_Status->UpdateStatusFont();
		}
		else if (HIWORD(wParam) == CBN_SELCHANGE)
		{
			INT_PTR nSel = SendDlgItemMessage(hWnd2, wId, CB_GETCURSEL, 0, 0);

			switch (wId)
			{
			case tStatusFontFace:
				SendDlgItemMessage(hWnd2, wId, CB_GETLBTEXT, nSel, (LPARAM)gpSet->sStatusFontFace);
				break;
			case tStatusFontHeight:
				if (nSel >= 0 && nSel < (INT_PTR)countof(SettingsNS::FSizesSmall))
					gpSet->nStatusFontHeight = SettingsNS::FSizesSmall[nSel];
				break;
			case tStatusFontCharset:
				if (nSel >= 0 && nSel < (INT_PTR)countof(SettingsNS::nCharSets))
					gpSet->nStatusFontCharSet = SettingsNS::nCharSets[nSel];
				else
					gpSet->nStatusFontCharSet = DEFAULT_CHARSET;
			}
			gpConEmu->mp_Status->UpdateStatusFont();
		}
		break;
	} // tStatusFontFace, tStatusFontHeight, tStatusFontCharset

	case lbCmdTasks:
	{
		if (mb_IgnoreCmdGroupList)
			break;

		mb_IgnoreCmdGroupEdit = true;
		const Settings::CommandTasks* pCmd = NULL;
		int iCur = (int)SendDlgItemMessage(hWnd2, lbCmdTasks, LB_GETCURSEL, 0,0);
		if (iCur >= 0)
			pCmd = gpSet->CmdTaskGet(iCur);
		if (pCmd)
		{
			_ASSERTE(pCmd->pszName);
			wchar_t* pszNoBrk = lstrdup(!pCmd->pszName ? L"" : (pCmd->pszName[0] != TaskBracketLeft) ? pCmd->pszName : (pCmd->pszName+1));
			if (*pszNoBrk && (pszNoBrk[_tcslen(pszNoBrk)-1] == TaskBracketRight))
				pszNoBrk[_tcslen(pszNoBrk)-1] = 0;
			SetDlgItemText(hWnd2, tCmdGroupName, pszNoBrk);
			SafeFree(pszNoBrk);
			SetDlgItemText(hWnd2, tCmdGroupGuiArg, pCmd->pszGuiArgs ? pCmd->pszGuiArgs : L"");
			SetDlgItemText(hWnd2, tCmdGroupCommands, pCmd->pszCommands ? pCmd->pszCommands : L"");
			EnableWindow(GetDlgItem(hWnd2, tCmdGroupName), TRUE);
			EnableWindow(GetDlgItem(hWnd2, tCmdGroupGuiArg), TRUE);
			EnableWindow(GetDlgItem(hWnd2, tCmdGroupCommands), TRUE);
			EnableWindow(GetDlgItem(hWnd2, cbCmdGroupApp), TRUE);
			EnableWindow(GetDlgItem(hWnd2, cbCmdTasksParm), TRUE);
		}
		else
		{
			SetDlgItemText(hWnd2, tCmdGroupName, L"");
			SetDlgItemText(hWnd2, tCmdGroupGuiArg, L"");
			SetDlgItemText(hWnd2, tCmdGroupCommands, L"");
			EnableWindow(GetDlgItem(hWnd2, tCmdGroupName), FALSE);
			EnableWindow(GetDlgItem(hWnd2, tCmdGroupGuiArg), FALSE);
			EnableWindow(GetDlgItem(hWnd2, tCmdGroupCommands), FALSE);
			EnableWindow(GetDlgItem(hWnd2, cbCmdGroupApp), FALSE);
			EnableWindow(GetDlgItem(hWnd2, cbCmdTasksParm), FALSE);
		}
		mb_IgnoreCmdGroupEdit = false;

		break;
	} // lbCmdTasks:




	default:
		if (hWnd2 == gpSetCls->mh_Tabs[thi_Views])
		{
			if (HIWORD(wParam) == CBN_EDITCHANGE)
			{
				switch (wId)
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

				switch (wId)
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
		} // else if (hWnd2 == gpSetCls->mh_Tabs[thi_Views])
		else if (hWnd2 == mh_Tabs[thi_Colors])
		{
			if (wId == lbExtendIdx)
			{
				gpSet->AppStd.nExtendColorIdx = SendDlgItemMessage(hWnd2, wId, CB_GETCURSEL, 0, 0);
			}
			else if (wId == lbConClrText)
			{
				gpSet->AppStd.nTextColorIdx = SendDlgItemMessage(hWnd2, wId, CB_GETCURSEL, 0, 0);
				if (gpSet->AppStd.nTextColorIdx != gpSet->AppStd.nBackColorIdx)
					UpdateTextColorSettings(TRUE, FALSE);
			}
			else if (wId == lbConClrBack)
			{
				gpSet->AppStd.nBackColorIdx = SendDlgItemMessage(hWnd2, wId, CB_GETCURSEL, 0, 0);
				if (gpSet->AppStd.nTextColorIdx != gpSet->AppStd.nBackColorIdx)
					UpdateTextColorSettings(TRUE, FALSE);
			}
			else if (wId == lbConClrPopText)
			{
				gpSet->AppStd.nPopTextColorIdx = SendDlgItemMessage(hWnd2, wId, CB_GETCURSEL, 0, 0);
				if (gpSet->AppStd.nPopTextColorIdx != gpSet->AppStd.nPopBackColorIdx)
					UpdateTextColorSettings(FALSE, TRUE);
			}
			else if (wId == lbConClrPopBack)
			{
				gpSet->AppStd.nPopBackColorIdx = SendDlgItemMessage(hWnd2, wId, CB_GETCURSEL, 0, 0);
				if (gpSet->AppStd.nPopTextColorIdx != gpSet->AppStd.nPopBackColorIdx)
					UpdateTextColorSettings(FALSE, TRUE);
			}
			else if (wId==lbDefaultColors)
			{
				HWND hList = GetDlgItem(hWnd2, lbDefaultColors);
				INT_PTR nIdx = SendMessage(hList, CB_GETCURSEL, 0, 0);

				// Save & Delete buttons
				{
					bool bEnabled = false;
					wchar_t* pszText = NULL;

					if (HIWORD(wParam) == CBN_EDITCHANGE)
					{
						INT_PTR nLen = GetWindowTextLength(hList);
						pszText = (nLen > 0) ? (wchar_t*)malloc((nLen+1)*sizeof(wchar_t)) : NULL;
						if (pszText)
							GetWindowText(hList, pszText, nLen+1);
					}
					else if ((HIWORD(wParam) == CBN_SELCHANGE) && nIdx > 0) // 0 -- current color scheme. ее удалять/сохранять "нельзя"
					{
						INT_PTR nLen = SendMessage(hList, CB_GETLBTEXTLEN, nIdx, 0);
						pszText = (nLen > 0) ? (wchar_t*)malloc((nLen+1)*sizeof(wchar_t)) : NULL;
						if (pszText)
							SendMessage(hList, CB_GETLBTEXT, nIdx, (LPARAM)pszText);
					}

					if (pszText)
					{
						bEnabled = (wcspbrk(pszText, L"<>") == NULL);
						SafeFree(pszText);
					}

					EnableWindow(GetDlgItem(hWnd2, cbColorSchemeSave), bEnabled);
					EnableWindow(GetDlgItem(hWnd2, cbColorSchemeDelete), bEnabled);
				}

				// Юзер выбрал в списке другую палитру
				if ((HIWORD(wParam) == CBN_SELCHANGE) && gbLastColorsOk)  // только если инициализация палитр завершилась
				{
					//const DWORD* pdwDefData = NULL;
					const Settings::ColorPalette* pPal = NULL;
					wchar_t temp[32];

					if (nIdx == 0)
						pPal = &gLastColors;
					//else if (nIdx >= 1 && nIdx <= (INT_PTR)countof(DefColors))
					//	pdwDefData = DefColors[nIdx-1].dwDefColors;
					//else
					else if ((pPal = gpSet->PaletteGet(nIdx-1)) == NULL)
						return 0; // неизвестный набор

					uint nCount = countof(pPal->Colors);

					for (uint i = 0; i < nCount; i++)
					{
						gpSet->Colors[i] = pPal->Colors[i]; //-V108
						_wsprintf(temp, SKIPLEN(countof(temp)) L"%i %i %i", getR(gpSet->Colors[i]), getG(gpSet->Colors[i]), getB(gpSet->Colors[i]));
						SetDlgItemText(hWnd2, 1100 + i, temp);
						InvalidateRect(GetDlgItem(hWnd2, c0+i), 0, 1);
					}

					DWORD nVal;

					nVal = pPal->nTextColorIdx;
					FillListBox(hWnd2, lbConClrText, SettingsNS::szColorIdxTh, SettingsNS::nColorIdxTh, nVal);
					nVal = pPal->nBackColorIdx;
					FillListBox(hWnd2, lbConClrBack, SettingsNS::szColorIdxTh, SettingsNS::nColorIdxTh, nVal);
					nVal = pPal->nPopTextColorIdx;
					FillListBox(hWnd2, lbConClrPopText, SettingsNS::szColorIdxTh, SettingsNS::nColorIdxTh, nVal);
					nVal = pPal->nPopBackColorIdx;
					FillListBox(hWnd2, lbConClrPopBack, SettingsNS::szColorIdxTh, SettingsNS::nColorIdxTh, nVal);

					BOOL bTextChanged = (gpSet->AppStd.nTextColorIdx != pPal->nTextColorIdx) || (gpSet->AppStd.nBackColorIdx != pPal->nBackColorIdx);
					BOOL bPopupChanged = (gpSet->AppStd.nPopTextColorIdx != pPal->nPopTextColorIdx) || (gpSet->AppStd.nPopBackColorIdx != pPal->nPopBackColorIdx);

					if (bTextChanged || bPopupChanged)
					{
						gpSet->AppStd.nTextColorIdx = pPal->nTextColorIdx;
						gpSet->AppStd.nBackColorIdx = pPal->nBackColorIdx;
						gpSet->AppStd.nPopTextColorIdx = pPal->nPopTextColorIdx;
						gpSet->AppStd.nPopBackColorIdx = pPal->nPopBackColorIdx;
						UpdateTextColorSettings(bTextChanged, bPopupChanged);
					}

					nVal = pPal->nExtendColorIdx;
					FillListBox(hWnd2, lbExtendIdx, SettingsNS::szColorIdxSh, SettingsNS::nColorIdxSh, nVal);
					gpSet->AppStd.nExtendColorIdx = nVal;
					gpSet->AppStd.isExtendColors = pPal->isExtendColors;
					checkDlgButton(hWnd2, cbExtendColors, pPal->isExtendColors ? BST_CHECKED : BST_UNCHECKED);
					OnButtonClicked(hWnd2, cbExtendColors, 0);
				}
				else return 0;
			}

			gpConEmu->Update(true);
		} // else if (hWnd2 == mh_Tabs[thi_Colors])
		else if (hWnd2 == mh_Tabs[thi_Selection])
		{
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				switch (wId)
				{
				case lbCTSBlockSelection:
					{
						BYTE VkMod = 0;
						GetListBoxByte(hWnd2, lbCTSBlockSelection, SettingsNS::szKeysAct, SettingsNS::nKeysAct, VkMod);
						gpSet->SetHotkeyById(vkCTSVkBlock, VkMod);
						CheckSelectionModifiers();
					} break;
				case lbCTSTextSelection:
					{
						BYTE VkMod = 0;
						GetListBoxByte(hWnd2, lbCTSTextSelection, SettingsNS::szKeysAct, SettingsNS::nKeysAct, VkMod);
						gpSet->SetHotkeyById(vkCTSVkText, VkMod);
						CheckSelectionModifiers();
					} break;
				case lbCTSActAlways:
					{
						BYTE VkMod = 0;
						GetListBoxByte(hWnd2, lbCTSActAlways, SettingsNS::szKeysAct, SettingsNS::nKeysAct, VkMod);
						gpSet->SetHotkeyById(vkCTSVkAct, VkMod);
					} break;
				case lbCTSEOL:
					{
						BYTE eol = 0;
						GetListBoxByte(hWnd2,lbCTSEOL,SettingsNS::szCRLF,SettingsNS::nCRLF,eol);
						gpSet->AppStd.isCTSEOL = eol;
					} // lbCTSEOL
					break;
				case lbCTSClickPromptPosition:
					{
						BYTE VkMod = 0;
						GetListBoxByte(hWnd2, lbCTSClickPromptPosition, SettingsNS::szKeysAct, SettingsNS::nKeysAct, VkMod);
						gpSet->SetHotkeyById(vkCTSVkPromptClk, VkMod);
						CheckSelectionModifiers();
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
						BYTE VkMod = 0;
						GetListBoxByte(hWnd2, lbFarGotoEditorVk, SettingsNS::szKeysAct, SettingsNS::nKeysAct, VkMod);
						gpSet->SetHotkeyById(vkFarGotoEditorVk, VkMod);
					} break;
				}
			} // if (HIWORD(wParam) == CBN_SELCHANGE)
		} // else if (hWnd2 == hSelection)
	} // switch (wId)
	return 0;
}

void CSettings::SelectTreeItem(HWND hTree, HTREEITEM hItem, bool bPost /*= false*/)
{
	HTREEITEM hParent = TreeView_GetParent(hTree, hItem);
	if (hParent)
	{
		TreeView_Expand(hTree, hParent, TVE_EXPAND);
	}
	
	if (!bPost)
		TreeView_SelectItem(hTree, hItem);
	else
		PostMessage(hTree, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hItem);
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
		{
			SelectTreeItem(hTree, nm.itemNew.hItem);
		}
		return 0;
	}
	else if ((LONG_PTR)phdr == 0x102)
	{
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
		{
			SelectTreeItem(hTree, nm.itemNew.hItem);
		}
		return 0;
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
					if (mh_Tabs[m_Pages[i].PageIndex] == NULL)
					{
						SetCursor(LoadCursor(NULL,IDC_WAIT));
						//HWND hPlace = GetDlgItem(ghOpWnd, tSetupPagePlace);
						//RECT rcClient; GetWindowRect(hPlace, &rcClient);
						//MapWindowPoints(NULL, ghOpWnd, (LPPOINT)&rcClient, 2);
						mh_Tabs[m_Pages[i].PageIndex] = CreateDialogParam((HINSTANCE)GetModuleHandle(NULL),
							MAKEINTRESOURCE(m_Pages[i].PageID), ghOpWnd, pageOpProc, (LPARAM)&(m_Pages[i]));
						//MoveWindow(mh_Tabs[m_Pages[i].PageIndex], rcClient.left, rcClient.top, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top, 0);
					}
					else
					{
						SendMessage(mh_Tabs[m_Pages[i].PageIndex], mn_ActivateTabMsg, 1, (LPARAM)&(m_Pages[i]));
					}
					ShowWindow(mh_Tabs[m_Pages[i].PageIndex], SW_SHOW);
				}
				else if (p->itemOld.hItem == m_Pages[i].hTI)
				{
					hCurrent = mh_Tabs[m_Pages[i].PageIndex];
				}
			}
			if (hCurrent)
				ShowWindow(hCurrent, SW_HIDE);
		} // TVN_SELCHANGED
		break;
	}

	return 0;
}

void CSettings::Dialog(int IdShowPage /*= 0*/)
{
	if (!ghOpWnd || !IsWindow(ghOpWnd))
	{
		SetCursor(LoadCursor(NULL,IDC_WAIT));

		// Сначала обновить DC, чтобы некрасивостей не было
		gpConEmu->UpdateWindowChild(NULL);

		//2009-05-03. DialogBox создает МОДАЛЬНЫЙ диалог, а нам нужен НЕмодальный
		HWND hOpt = CreateDialog(g_hInstance, MAKEINTRESOURCE(IDD_SETTINGS), NULL, wndOpProc);

		if (!hOpt)
		{
			DisplayLastError(L"Can't create settings dialog!");
			goto wrap;
		}
		else
		{
			_ASSERTE(ghOpWnd == hOpt);
			ghOpWnd = hOpt;
		}
	}

	apiShowWindow(ghOpWnd, SW_SHOWNORMAL);

	if (IdShowPage != 0)
	{
		for (size_t i = 0; gpSetCls->m_Pages[i].PageID; i++)
		{
			if (gpSetCls->m_Pages[i].PageID == IdShowPage)
			{
				//PostMessage(GetDlgItem(ghOpWnd, tvSetupCategories), TVM_SELECTITEM, TVGN_CARET, (LPARAM)gpSetCls->m_Pages[i].hTI);
				SelectTreeItem(GetDlgItem(ghOpWnd, tvSetupCategories), gpSetCls->m_Pages[i].hTI, true);
				break;
			}
		}
	}

	SetFocus(ghOpWnd);
wrap:
	return;
}

void CSettings::OnClose()
{
	ApplyStartupOptions();

	//if (gpSet->isTabs==1)
	//	gpConEmu->ForceShowTabs(TRUE);
	//else if (gpSet->isTabs==0)
	//	gpConEmu->ForceShowTabs(FALSE);
	////else
	//
	//gpConEmu->mp_TabBar->Update();
	//gpConEmu->UpdateWindowRgn();

	gpConEmu->OnPanelViewSettingsChanged();
	//gpConEmu->UpdateGuiInfoMapping();
	gpConEmu->RegisterMinRestore(gpSet->IsHotkey(vkMinimizeRestore) || gpSet->IsHotkey(vkMinimizeRestor2));

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
	switch (messg)
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
			//gpSetCls->hMain = gpSetCls->hExt = gpSetCls->hFar = gpSetCls->hKeys = gpSetCls->hTabs = gpSetCls->hColors = NULL;
			//gpSetCls->hCmdTasks = gpSetCls->hViews = gpSetCls->hInfo = gpSetCls->hDebug = gpSetCls->hUpdate = gpSetCls->hSelection = NULL;
			memset(gpSetCls->mh_Tabs, 0, sizeof(gpSetCls->mh_Tabs));
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

	if (wID == tFontFace || wID == tFontFace2 || wID == tThumbsFontName || wID == tTilesFontName
		|| wID == tTabFontFace || wID == tStatusFontFace)
	{
		MEASUREITEMSTRUCT *pItem = (MEASUREITEMSTRUCT*)lParam;
		pItem->itemHeight = 15; //pItem->itemHeight;
	}

	return TRUE;
}

INT_PTR CSettings::OnDrawFontItem(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	DWORD wID = wParam;

	if (wID == tFontFace || wID == tFontFace2 || wID == tThumbsFontName || wID == tTilesFontName
		|| wID == tTabFontFace || wID == tStatusFontFace)
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
		if (!lParam)
		{
			_ASSERTE(lParam!=0);
			return 0;
		}
		ConEmuSetupPages* p = (ConEmuSetupPages*)lParam;
		//_ASSERTE(((ConEmuSetupPages*)lParam)->hPage!=NULL && *((ConEmuSetupPages*)lParam)->hPage==NULL && ((ConEmuSetupPages*)lParam)->PageID!=0);
		_ASSERTE(p->PageIndex>=0 && p->PageIndex<countof(gpSetCls->mh_Tabs) && gpSetCls->mh_Tabs[p->PageIndex]==NULL);
		gpSetCls->mh_Tabs[p->PageIndex] = hWnd2;

		HWND hPlace = GetDlgItem(ghOpWnd, tSetupPagePlace);
		RECT rcClient; GetWindowRect(hPlace, &rcClient);
		MapWindowPoints(NULL, ghOpWnd, (LPPOINT)&rcClient, 2);
		MoveWindow(hWnd2, rcClient.left, rcClient.top, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top, 0);

		switch (((ConEmuSetupPages*)lParam)->PageID)
		{
		case IDD_SPG_MAIN:
			gpSetCls->OnInitDialog_Main(hWnd2);
			break;
		case IDD_SPG_WNDSIZEPOS:
			gpSetCls->OnInitDialog_WndPosSize(hWnd2, true);
			break;
		case IDD_SPG_SHOW:
			gpSetCls->OnInitDialog_Show(hWnd2, true);
			break;
		case IDD_SPG_CURSOR:
			gpSetCls->OnInitDialog_Cursor(hWnd2, TRUE);
			break;
		case IDD_SPG_STARTUP:
			gpSetCls->OnInitDialog_Startup(hWnd2, TRUE);
			break;
		case IDD_SPG_FEATURE:
			gpSetCls->OnInitDialog_Ext(hWnd2);
			break;
		case IDD_SPG_COMSPEC:
			gpSetCls->OnInitDialog_Comspec(hWnd2, true);
			break;
		//case IDD_SPG_CMDOUTPUT:
		//	gpSetCls->OnInitDialog_Output(hWnd2, true);
		//	break;
		case IDD_SPG_SELECTION:
			gpSetCls->OnInitDialog_Selection(hWnd2);
			break;
		case IDD_SPG_FEATURE_FAR:
			gpSetCls->OnInitDialog_Far(hWnd2, TRUE);
			break;
		case IDD_SPG_KEYS:
			{
			bool lbOld = bSkipSelChange; bSkipSelChange = true;
			gpSetCls->OnInitDialog_Keys(hWnd2, TRUE);
			bSkipSelChange = lbOld;
			}
			break;
		case IDD_SPG_CONTROL:
			gpSetCls->OnInitDialog_Control(hWnd2, TRUE);
			break;
		case IDD_SPG_TABS:
			gpSetCls->OnInitDialog_Tabs(hWnd2);
			break;
		case IDD_SPG_STATUSBAR:
			gpSetCls->OnInitDialog_Status(hWnd2, true);
			break;
		case IDD_SPG_COLORS:
			gpSetCls->OnInitDialog_Color(hWnd2);
			break;
		case IDD_SPG_TRANSPARENT:
			gpSetCls->OnInitDialog_Transparent(hWnd2);
			break;
		case IDD_SPG_CMDTASKS:
			{
			bool lbOld = bSkipSelChange; bSkipSelChange = true;
			gpSetCls->OnInitDialog_Tasks(hWnd2, true);
			bSkipSelChange = lbOld;
			}
			break;
		case IDD_SPG_APPDISTINCT:
			gpSetCls->OnInitDialog_Apps(hWnd2, true);
			break;
		case IDD_SPG_INTEGRATION:
			gpSetCls->OnInitDialog_Integr(hWnd2, true);
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
		ConEmuSetupPages* p = (ConEmuSetupPages*)lParam;
		//_ASSERTE(((ConEmuSetupPages*)lParam)->hPage!=NULL && *((ConEmuSetupPages*)lParam)->hPage==hWnd2 && ((ConEmuSetupPages*)lParam)->PageID!=0);
		_ASSERTE(p->PageIndex>=0 && p->PageIndex<countof(gpSetCls->mh_Tabs) && gpSetCls->mh_Tabs[p->PageIndex]!=NULL && gpSetCls->mh_Tabs[p->PageIndex]==hWnd2);

		// Здесь можно обновить контролы страничек при активации вкладки
		switch (p->PageID)
		{
		case IDD_SPG_MAIN:    /*gpSetCls->OnInitDialog_Main(hWnd2);*/   break;
		case IDD_SPG_WNDSIZEPOS:
			gpSetCls->OnInitDialog_WndPosSize(hWnd2, false);
			break;
		case IDD_SPG_SHOW:
			gpSetCls->OnInitDialog_Show(hWnd2, false);
			break;
		case IDD_SPG_CURSOR:
			gpSetCls->OnInitDialog_Cursor(hWnd2, FALSE);
			break;
		case IDD_SPG_STARTUP:
			gpSetCls->OnInitDialog_Startup(hWnd2, FALSE);
			break;
		case IDD_SPG_FEATURE: /*gpSetCls->OnInitDialog_Ext(hWnd2);*/    break;
		case IDD_SPG_COMSPEC:
			gpSetCls->OnInitDialog_Comspec(hWnd2, false);
			break;
		//case IDD_SPG_CMDOUTPUT:
		//	gpSetCls->OnInitDialog_Output(hWnd2, false);
		//	break;
		case IDD_SPG_SELECTION: /*gpSetCls->OnInitDialog_Selection(hWnd2);*/    break;
		case IDD_SPG_FEATURE_FAR:
			gpSetCls->OnInitDialog_Far(hWnd2, FALSE);
			break;
		case IDD_SPG_KEYS:
			{
			bool lbOld = bSkipSelChange; bSkipSelChange = true;
			gpSetCls->OnInitDialog_Keys(hWnd2, FALSE);
			bSkipSelChange = lbOld;
			}
			break;
		case IDD_SPG_CONTROL:
			gpSetCls->OnInitDialog_Control(hWnd2, FALSE);
			break;
		case IDD_SPG_TABS:    /*gpSetCls->OnInitDialog_Tabs(hWnd2);*/   break;
		case IDD_SPG_STATUSBAR:
			gpSetCls->OnInitDialog_Status(hWnd2, false);
			break;
		case IDD_SPG_COLORS:  /*gpSetCls->OnInitDialog_Color(hWnd2);*/  break;
		case IDD_SPG_TRANSPARENT:
			gpSetCls->OnInitDialog_Transparent(hWnd2);
			break;
		case IDD_SPG_CMDTASKS:
			gpSetCls->OnInitDialog_Tasks(hWnd2, false);
			break;
		case IDD_SPG_APPDISTINCT:
			gpSetCls->OnInitDialog_Apps(hWnd2, false);
			break;
		case IDD_SPG_INTEGRATION:
			gpSetCls->OnInitDialog_Integr(hWnd2, false);
			break;
		case IDD_SPG_VIEWS:   /*gpSetCls->OnInitDialog_Views(hWnd2);*/  break;
		case IDD_SPG_DEBUG:   /*gpSetCls->OnInitDialog_Debug(hWnd2);*/  break;
		case IDD_SPG_UPDATE:  /*gpSetCls->OnInitDialog_Update(hWnd2);*/ break;
		case IDD_SPG_INFO:    /*gpSetCls->OnInitDialog_Info(hWnd2);*/   break;

		default:
			// Чтобы не забыть добавить вызов инициализации
			_ASSERTE(((ConEmuSetupPages*)lParam)->PageID==IDD_SPG_MAIN);
		}
	}
	else if (gpSetCls->mh_Tabs[thi_Apps] && (hWnd2 == gpSetCls->mh_Tabs[thi_Apps]))
	{
		// Страничка "App distinct" в некотором смысле особенная.
		// У многих контролов ИД дублируются с другими вкладками.
		return gpSetCls->pageOpProc_Apps(hWnd2, NULL, messg, wParam, lParam);
	}
	else if (gpSetCls->mh_Tabs[thi_Integr] && (hWnd2 == gpSetCls->mh_Tabs[thi_Integr]))
	{
		return gpSetCls->pageOpProc_Integr(hWnd2, messg, wParam, lParam);
	}
	else if (gpSetCls->mh_Tabs[thi_Startup] && (hWnd2 == gpSetCls->mh_Tabs[thi_Startup]))
	{
		return gpSetCls->pageOpProc_Start(hWnd2, messg, wParam, lParam);
	}
	else
	// All other messages
	switch (messg)
	{
		#ifdef _DEBUG
		case WM_INITDIALOG:
			// Должно быть обработано выше
			_ASSERTE(messg!=WM_INITDIALOG);
			break;
		#endif

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
				else if (HIWORD(wParam) == CBN_EDITCHANGE || HIWORD(wParam) == CBN_SELCHANGE/*LBN_SELCHANGE*/)
				{
					if (!bSkipSelChange)
						gpSetCls->OnComboBox(hWnd2, wParam, lParam);
				}
				else if (HIWORD(wParam) == CBN_KILLFOCUS && gpSetCls->mn_LastChangingFontCtrlId && LOWORD(wParam) == gpSetCls->mn_LastChangingFontCtrlId)
				{
					_ASSERTE(hWnd2 == gpSetCls->mh_Tabs[thi_Main]);
					PostMessage(hWnd2, gpSetCls->mn_MsgRecreateFont, gpSetCls->mn_LastChangingFontCtrlId, 0);
					gpSetCls->mn_LastChangingFontCtrlId = 0;
				}
				else if (HIWORD(wParam) == 0xFFFF && LOWORD(wParam) == lbConEmuHotKeys)
				{
					gpSetCls->OnHotkeysNotify(hWnd2, wParam, NULL);
				}
			} // WM_COMMAND
			break;
		case WM_MEASUREITEM:
			return gpSetCls->OnMeasureFontItem(hWnd2, messg, wParam, lParam);
		case WM_DRAWITEM:
			return gpSetCls->OnDrawFontItem(hWnd2, messg, wParam, lParam);
		case WM_CTLCOLORSTATIC:
			{
				WORD wID = GetDlgCtrlID((HWND)lParam);

				if ((wID >= c0 && wID <= MAX_COLOR_EDT_ID) || (wID >= c32 && wID <= c38))
					return gpSetCls->ColorCtlStatic(hWnd2, wID, (HWND)lParam);

				return 0;
			} // WM_CTLCOLORSTATIC
		case WM_HSCROLL:
			{
				if (gpSetCls->mh_Tabs[thi_Main] && (HWND)lParam == GetDlgItem(gpSetCls->mh_Tabs[thi_Main], slDarker))
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
				else if (gpSetCls->mh_Tabs[thi_Transparent] && (HWND)lParam == GetDlgItem(gpSetCls->mh_Tabs[thi_Transparent], slTransparent))
				{
					int newV = SendDlgItemMessage(hWnd2, slTransparent, TBM_GETPOS, 0, 0);

					if (newV != gpSet->nTransparent)
					{
						checkDlgButton(gpSetCls->mh_Tabs[thi_Transparent], cbTransparent, (newV!=255) ? BST_CHECKED : BST_UNCHECKED);
						gpSet->nTransparent = newV;
						if (!gpSet->isTransparentSeparate)
							SendDlgItemMessage(hWnd2, slTransparentInactive, TBM_SETPOS, (WPARAM) true, (LPARAM) gpSet->nTransparent);
						gpConEmu->OnTransparent();
					}
				}
				else if (gpSetCls->mh_Tabs[thi_Transparent] && (HWND)lParam == GetDlgItem(gpSetCls->mh_Tabs[thi_Transparent], slTransparentInactive))
				{
					int newV = SendDlgItemMessage(hWnd2, slTransparentInactive, TBM_GETPOS, 0, 0);

					if (gpSet->isTransparentSeparate && (newV != gpSet->nTransparentInactive))
					{
						//checkDlgButton(gpSetCls->mh_Tabs[thi_Transparent], cbTransparentInactive, (newV!=255) ? BST_CHECKED : BST_UNCHECKED);
						gpSet->nTransparentInactive = newV;
						gpConEmu->OnTransparent();
					}
				}
			} // WM_HSCROLL
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
				KillTimer(gpSetCls->mh_Tabs[thi_Main], FAILED_FONT_TIMERID);
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
				if (hWnd2 == gpSetCls->mh_Tabs[thi_Views])
					gpSetCls->OnInitDialog_CopyFonts(hWnd2, tThumbsFontName, tTilesFontName, 0);
				else if (hWnd2 == gpSetCls->mh_Tabs[thi_Tabs])
					gpSetCls->OnInitDialog_CopyFonts(hWnd2, tTabFontFace, 0);
				else if (hWnd2 == gpSetCls->mh_Tabs[thi_Status])
					gpSetCls->OnInitDialog_CopyFonts(hWnd2, tStatusFontFace, 0);
					
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
					SetDlgItemText(gpSetCls->mh_Tabs[thi_Info], wParam+tPerfFPS, sTemp); //-V107
				} // if (gpSetCls->mn_Freq!=0)
			} // if (messg == gpSetCls->mn_MsgUpdateCounter)
			else if (messg == DBGMSG_LOG_ID && hWnd2 == gpSetCls->mh_Tabs[thi_Debug])
			{
				if (wParam == DBGMSG_LOG_SHELL_MAGIC)
				{
					bool lbOld = bSkipSelChange; bSkipSelChange = true;
					DebugLogShellActivity *pShl = (DebugLogShellActivity*)lParam;
					gpSetCls->debugLogShell(hWnd2, pShl);
					bSkipSelChange = lbOld;
				} // DBGMSG_LOG_SHELL_MAGIC
				else if (wParam == DBGMSG_LOG_INPUT_MAGIC)
				{
					bool lbOld = bSkipSelChange; bSkipSelChange = true;
					CESERVER_REQ_PEEKREADINFO* pInfo = (CESERVER_REQ_PEEKREADINFO*)lParam;
					gpSetCls->debugLogInfo(hWnd2, pInfo);
					bSkipSelChange = lbOld;
				} // DBGMSG_LOG_INPUT_MAGIC
				else if (wParam == DBGMSG_LOG_CMD_MAGIC)
				{
					bool lbOld = bSkipSelChange; bSkipSelChange = true;
					LogCommandsData* pCmd = (LogCommandsData*)lParam;
					gpSetCls->debugLogCommand(hWnd2, pCmd);
					bSkipSelChange = lbOld;
				} // DBGMSG_LOG_CMD_MAGIC
			} // if (messg == DBGMSG_LOG_ID && hWnd2 == gpSetCls->hDebug)
		} // default:
	}

	return 0;
}

INT_PTR CSettings::pageOpProc_AppsChild(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	static int nLastScrollPos = 0;

	switch (messg)
	{
	case WM_INITDIALOG:
		nLastScrollPos = 0;
		return FALSE;

	case WM_MOUSEWHEEL:
		{
			SHORT nDir = (SHORT)HIWORD(wParam);
			if (nDir)
			{
				pageOpProc_AppsChild(hWnd2, WM_VSCROLL, (nDir > 0) ? SB_PAGEUP : SB_PAGEDOWN, 0);
			}
		}
		return 0;

	case WM_VSCROLL:
		{
			int dx = 0, dy = 0;

			SCROLLINFO si = {sizeof(si)};
			si.fMask = SIF_POS|SIF_RANGE|SIF_PAGE;
			GetScrollInfo(hWnd2, SB_VERT, &si);

			int nPos = 0;
			SHORT nCode = (SHORT)LOWORD(wParam);
			if ((nCode == SB_THUMBPOSITION) || (nCode == SB_THUMBTRACK))
			{
				nPos = (SHORT)HIWORD(wParam);
			}
			else
			{
				nPos = si.nPos;
				int nDelta = 16; // Высота CheckBox'a
				RECT rcChild = {};
				if (GetWindowRect(GetDlgItem(hWnd2, cbExtendFontsOverride), &rcChild))
					nDelta = rcChild.bottom - rcChild.top;

				switch (nCode)
				{
				case SB_LINEDOWN:
				case SB_PAGEDOWN:
					nPos = min(si.nMax,si.nPos+nDelta);
					break;
				//case SB_PAGEDOWN:
				//	nPos = min(si.nMax,si.nPos+si.nPage);
				//	break;
				case SB_LINEUP:
				case SB_PAGEUP:
					nPos = max(si.nMin,si.nPos-nDelta);
					break;
				//case SB_PAGEUP:
				//	nPos = max(si.nMin,si.nPos-si.nPage);
				//	break;
				}
			}

			dy = nLastScrollPos - nPos;
			nLastScrollPos = nPos;

			si.fMask = SIF_POS;
			si.nPos = nPos;
			SetScrollInfo(hWnd2, SB_VERT, &si, TRUE);

			if (dy)
			{
				ScrollWindowEx(hWnd2, dx, dy, NULL, NULL, NULL, NULL, SW_SCROLLCHILDREN|SW_INVALIDATE|SW_ERASE);
			}
		}
		return FALSE;
	}

	HWND hParent = GetParent(hWnd2);
	return gpSetCls->pageOpProc_Apps(hParent, hWnd2, messg, wParam, lParam);
}

INT_PTR CSettings::pageOpProc_Apps(HWND hWnd2, HWND hChild, UINT messg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR iRc = 0;
	static bool bSkipSelChange = false, bSkipEditChange = false, bSkipEditSet = false;
	bool bRedraw = false, bRefill = false;

	#define UM_DISTINCT_ENABLE (WM_APP+32)
	#define UM_FILL_CONTROLS (WM_APP+33)

	static struct StrDistinctControls
	{
		WORD nOverrideID;
		WORD nCtrls[16];
	} DistinctControls[] = {
		{0, {rbAppDistinctElevatedOn, rbAppDistinctElevatedOff, rbAppDistinctElevatedIgnore, stAppDistinctName, tAppDistinctName}},
		{cbExtendFontsOverride, {cbExtendFonts, stExtendFontBoldIdx, lbExtendFontBoldIdx, stExtendFontItalicIdx, lbExtendFontItalicIdx, stExtendFontNormalIdx, lbExtendFontNormalIdx}},
		{cbCursorOverride, {rCursorV, rCursorH, cbCursorColor, cbCursorBlink, cbBlockInactiveCursor, cbCursorIgnoreSize, tCursorFixedSize, stCursorFixedSize, tCursorMinSize, stCursorMinSize}},
		{cbColorsOverride, {lbColorsOverride}},
		{cbClipboardOverride, {
			gbCopyingOverride, cbCTSDetectLineEnd, cbCTSBashMargin, cbCTSTrimTrailing, stCTSEOL, lbCTSEOL,
			gbPastingOverride, cbClipShiftIns, cbClipCtrlV,
			gbPromptOverride, cbCTSClickPromptPosition}},
		{cbBgImageOverride, {cbBgImage, tBgImage, bBgImage, lbBgPlacement}},
	};

	if (!hChild)
		hChild = GetDlgItem(hWnd2, IDD_SPG_APPDISTINCT2);

	if (!hChild)
	{
		if ((messg == WM_INITDIALOG) || (messg == mn_ActivateTabMsg))
		{
			hChild = CreateDialogParam((HINSTANCE)GetModuleHandle(NULL),
							MAKEINTRESOURCE(IDD_SPG_APPDISTINCT2), hWnd2, pageOpProc_AppsChild, NULL);
			if (!hChild)
			{
				EnableWindow(hWnd2, FALSE);
				MBoxAssert(hChild && "CreateDialogParam(IDD_SPG_APPDISTINCT2) failed");
				return 0;
			}
			SetWindowLongPtr(hChild, GWLP_ID, IDD_SPG_APPDISTINCT2);

			HWND hHolder = GetDlgItem(hWnd2, tAppDistinctHolder);
			RECT rcPos = {}; GetWindowRect(hHolder, &rcPos);
			MapWindowPoints(NULL, hWnd2, (LPPOINT)&rcPos, 2);
			POINT ptScroll = {};
			HWND hEnd = GetDlgItem(hChild,stAppDistinctBottom);
			MapWindowPoints(hEnd, hChild, &ptScroll, 1);
			ShowWindow(hEnd, SW_HIDE);

			SCROLLINFO si = {sizeof(si), SIF_ALL};
			si.nMax = ptScroll.y - (rcPos.bottom - rcPos.top);
			RECT rcChild = {}; GetWindowRect(GetDlgItem(hChild, DistinctControls[1].nOverrideID), &rcChild);
			si.nPage = rcChild.bottom - rcChild.top;
			SetScrollInfo(hChild, SB_VERT, &si, FALSE);

			MoveWindow(hChild, rcPos.left, rcPos.top, rcPos.right-rcPos.left, rcPos.bottom-rcPos.top, FALSE);

			ShowWindow(hHolder, SW_HIDE);
			ShowWindow(hChild, SW_SHOW);
		}

		//_ASSERTE(hChild && IsWindow(hChild));
	}



	if ((messg == WM_INITDIALOG) || (messg == mn_ActivateTabMsg))
	{
		bool lbOld = bSkipSelChange; bSkipSelChange = true;
		bool bForceReload = (messg == WM_INITDIALOG);

		if (bForceReload || !bSkipEditSet)
		{
			const Settings::ColorPalette* pPal;

			if ((pPal = gpSet->PaletteGet(-1)) != NULL)
			{
				memmove(&gLastColors, pPal, sizeof(gLastColors));
				if (gLastColors.pszName == NULL)
				{
					_ASSERTE(gLastColors.pszName!=NULL);
					gLastColors.pszName = (wchar_t*)L"<Current color scheme>";
				}
			}
			else
			{
				EnableWindow(hWnd2, FALSE);
				MBoxAssert(pPal && "PaletteGet(-1) failed");
				return 0;
			}

			SendMessage(GetDlgItem(hChild, lbColorsOverride), CB_RESETCONTENT, 0, 0);
			SendDlgItemMessage(hChild, lbColorsOverride, CB_ADDSTRING, 0, (LPARAM)gLastColors.pszName);
			for (int i = 0; (pPal = gpSet->PaletteGet(i)) != NULL; i++)
			{
				SendDlgItemMessage(hChild, lbColorsOverride, CB_ADDSTRING, 0, (LPARAM)pPal->pszName);
			}
			SendDlgItemMessage(hChild, lbColorsOverride, CB_SETCURSEL, 0/*iCurPalette*/, 0);


			SendMessage(GetDlgItem(hChild, lbExtendFontBoldIdx), CB_RESETCONTENT, 0, 0);
			SendMessage(GetDlgItem(hChild, lbExtendFontItalicIdx), CB_RESETCONTENT, 0, 0);
			SendMessage(GetDlgItem(hChild, lbExtendFontNormalIdx), CB_RESETCONTENT, 0, 0);
			for (uint i=0; i < countof(SettingsNS::szColorIdx); i++)
			{
				//_wsprintf(temp, SKIPLEN(countof(temp))(i==16) ? L"None" : L"%2i", i);
				SendDlgItemMessage(hChild, lbExtendFontBoldIdx, CB_ADDSTRING, 0, (LPARAM) SettingsNS::szColorIdx[i]);
				SendDlgItemMessage(hChild, lbExtendFontItalicIdx, CB_ADDSTRING, 0, (LPARAM) SettingsNS::szColorIdx[i]);
				SendDlgItemMessage(hChild, lbExtendFontNormalIdx, CB_ADDSTRING, 0, (LPARAM) SettingsNS::szColorIdx[i]);
			}
		}

		// Сброс ранее загруженного списка (ListBox: lbAppDistinct)
		SendDlgItemMessage(hWnd2, lbAppDistinct, LB_RESETCONTENT, 0,0);

		//if (abForceReload)
		//{
		//	// Обновить группы команд
		//	gpSet->LoadCmdTasks(NULL, true);
		//}

		int nApp = 0;
		wchar_t szItem[1024];
		const Settings::AppSettings* pApp = NULL;
		while ((pApp = gpSet->GetAppSettings(nApp)) && pApp->AppNames)
		{
			_wsprintf(szItem, SKIPLEN(countof(szItem)) L"%i\t%s\t", nApp+1,
				(pApp->Elevated == 1) ? L"E" : (pApp->Elevated == 2) ? L"N" : L"*");
			int nPrefix = lstrlen(szItem);
			lstrcpyn(szItem+nPrefix, pApp->AppNames, countof(szItem)-nPrefix);

			INT_PTR iIndex = SendDlgItemMessage(hWnd2, lbAppDistinct, LB_ADDSTRING, 0, (LPARAM)szItem);
			UNREFERENCED_PARAMETER(iIndex);
			//SendDlgItemMessage(hWnd2, lbAppDistinct, LB_SETITEMDATA, iIndex, (LPARAM)pApp);

			nApp++;
		}

		bSkipSelChange = lbOld;

		if (!bSkipSelChange)
		{
			pageOpProc_Apps(hWnd2, hChild, WM_COMMAND, MAKELONG(lbAppDistinct,LBN_SELCHANGE), 0);
		}

	}
	else switch (messg)
	{
	case UM_FILL_CONTROLS:
		if ((((HWND)wParam) == hWnd2) && lParam) // arg check
		{
			const Settings::AppSettings* pApp = (const Settings::AppSettings*)lParam;

			checkRadioButton(hWnd2, rbAppDistinctElevatedOn, rbAppDistinctElevatedIgnore,
				(pApp->Elevated == 1) ? rbAppDistinctElevatedOn :
				(pApp->Elevated == 2) ? rbAppDistinctElevatedOff : rbAppDistinctElevatedIgnore);

			BYTE b;
			wchar_t temp[MAX_PATH];

			checkDlgButton(hChild, cbExtendFontsOverride, pApp->OverrideExtendFonts);
			checkDlgButton(hChild, cbExtendFonts, pApp->isExtendFonts);
			_wsprintf(temp, SKIPLEN(countof(temp))(pApp->nFontBoldColor<16) ? L"%2i" : L"None", pApp->nFontBoldColor);
			SelectStringExact(hChild, lbExtendFontBoldIdx, temp);
			_wsprintf(temp, SKIPLEN(countof(temp))(pApp->nFontItalicColor<16) ? L"%2i" : L"None", pApp->nFontItalicColor);
			SelectStringExact(hChild, lbExtendFontItalicIdx, temp);
			_wsprintf(temp, SKIPLEN(countof(temp))(pApp->nFontNormalColor<16) ? L"%2i" : L"None", pApp->nFontNormalColor);
			SelectStringExact(hChild, lbExtendFontNormalIdx, temp);

			checkDlgButton(hChild, cbCursorOverride, pApp->OverrideCursor);
			checkRadioButton(hChild, rCursorV, rCursorH, pApp->isCursorV ? rCursorV : rCursorH);
			checkDlgButton(hChild, cbCursorColor, pApp->isCursorColor);
			checkDlgButton(hChild, cbCursorBlink, pApp->isCursorBlink);
			checkDlgButton(hChild, cbBlockInactiveCursor, pApp->isCursorBlockInactive);
			checkDlgButton(hChild, cbCursorIgnoreSize, pApp->isCursorIgnoreSize);
			SetDlgItemInt(hChild, tCursorFixedSize, pApp->nCursorFixedSize, FALSE);
			SetDlgItemInt(hChild, tCursorMinSize, pApp->nCursorMinSize, FALSE);

			checkDlgButton(hChild, cbColorsOverride, pApp->OverridePalette);
			SelectStringExact(hChild, lbColorsOverride, pApp->szPaletteName);

			checkDlgButton(hChild, cbClipboardOverride, pApp->OverrideClipboard);
			//
			checkDlgButton(hChild, cbCTSDetectLineEnd, pApp->isCTSDetectLineEnd);
			checkDlgButton(hChild, cbCTSBashMargin, pApp->isCTSBashMargin);
			checkDlgButton(hChild, cbCTSTrimTrailing, pApp->isCTSTrimTrailing);
			b = pApp->isCTSEOL;
			FillListBoxByte(hChild, lbCTSEOL, SettingsNS::szCRLF, SettingsNS::nCRLF, b);
			//
			checkDlgButton(hChild, cbClipShiftIns, pApp->isPasteAllLines);
			checkDlgButton(hChild, cbClipCtrlV, pApp->isPasteFirstLine);
			//
			checkDlgButton(hChild, cbCTSClickPromptPosition, pApp->isCTSClickPromptPosition);


			checkDlgButton(hChild, cbBgImageOverride, pApp->OverrideBgImage);
			checkDlgButton(hChild, cbBgImage, BST(pApp->isShowBgImage));
			SetDlgItemText(hChild, tBgImage, pApp->sBgImage);
			b = pApp->nBgOperation;
			FillListBoxByte(hChild, lbBgPlacement, SettingsNS::szBgOper, SettingsNS::nBgOper, b);

		} // UM_FILL_CONTROLS
		break;
	case UM_DISTINCT_ENABLE:
		if (((HWND)wParam) == hWnd2) // arg check
		{
			_ASSERTE(hChild && IsWindow(hChild));

			WORD nID = (WORD)(lParam & 0xFFFF);
			bool bAllowed = false;
			
			const Settings::AppSettings* pApp = NULL;
			int iCur = (int)SendDlgItemMessage(hWnd2, lbAppDistinct, LB_GETCURSEL, 0,0);
			if (iCur >= 0)
				pApp = gpSet->GetAppSettings(iCur);
			if (pApp && pApp->AppNames)
			{
				bAllowed = true;
			}
			
			for (size_t i = 0; i < countof(DistinctControls); i++)
			{
				if (nID && (nID != DistinctControls[i].nOverrideID))
					continue;

				BOOL bEnabled = bAllowed
					? (DistinctControls[i].nOverrideID ? IsChecked(hChild, DistinctControls[i].nOverrideID) : TRUE)
					: FALSE;

				HWND hDlg = DistinctControls[i].nOverrideID ? hChild : hWnd2;

				if (DistinctControls[i].nOverrideID)
				{
					EnableWindow(GetDlgItem(hDlg, DistinctControls[i].nOverrideID), bAllowed);
					if (!bAllowed)
						checkDlgButton(hDlg, DistinctControls[i].nOverrideID, BST_UNCHECKED);
				}

				_ASSERTE(DistinctControls[i].nCtrls[countof(DistinctControls[i].nCtrls)-1]==0 && "Overflow check of nCtrls[]")

				for (size_t j = 0; j < countof(DistinctControls[i].nCtrls) && DistinctControls[i].nCtrls[j]; j++)
				{
					EnableWindow(GetDlgItem(hDlg, DistinctControls[i].nCtrls[j]), bEnabled);
				}
			}

			InvalidateRect(hChild, NULL, FALSE);
		} // UM_DISTINCT_ENABLE
		break;
	case WM_COMMAND:
		{
			_ASSERTE(hChild && IsWindow(hChild));

			if (HIWORD(wParam) == BN_CLICKED)
			{
				BOOL bChecked;
				WORD CB = LOWORD(wParam);

				int iCur = bSkipSelChange ? -1 : (int)SendDlgItemMessage(hWnd2, lbAppDistinct, LB_GETCURSEL, 0,0);
				Settings::AppSettings* pApp = (iCur < 0) ? NULL : gpSet->GetAppSettingsPtr(iCur);
				_ASSERTE((iCur<0) || (pApp && pApp->AppNames));

				switch (CB)
				{
				case cbAppDistinctReload:
					{
						if (MessageBox(ghOpWnd, L"Warning! All unsaved changes will be lost!\n\nReload Apps from settings?",
								gpConEmu->GetDefaultTitle(), MB_YESNO|MB_ICONEXCLAMATION) != IDYES)
							break;

						// Перезагрузить App distinct
						gpSet->LoadAppSettings(NULL, true);

						// Обновить список на экране
						OnInitDialog_Apps(hWnd2, true);
					} // cbAppDistinctReload
					break;
				case cbAppDistinctAdd:
					{
						int iCount = (int)SendDlgItemMessage(hWnd2, lbAppDistinct, LB_GETCOUNT, 0,0);
						Settings::AppSettings* pNew = gpSet->GetAppSettingsPtr(iCount, TRUE);
						UNREFERENCED_PARAMETER(pNew);

						bool lbOld = bSkipSelChange; bSkipSelChange = true;

        				OnInitDialog_Apps(hWnd2, false);

        				SendDlgItemMessage(hWnd2, lbAppDistinct, LB_SETCURSEL, iCount, 0);

						bSkipSelChange = lbOld;

        				pageOpProc_Apps(hWnd2, hChild, WM_COMMAND, MAKELONG(lbAppDistinct,LBN_SELCHANGE), 0);
					} // cbAppDistinctAdd
					break;

				case cbAppDistinctDel:
					{
						int iCur = (int)SendDlgItemMessage(hWnd2, lbAppDistinct, LB_GETCURSEL, 0,0);
						if (iCur < 0)
							break;

						const Settings::AppSettings* p = gpSet->GetAppSettingsPtr(iCur);
						if (!p)
            				break;

						if (MessageBox(ghOpWnd, L"Delete application?", p->AppNames, MB_YESNO|MB_ICONQUESTION) != IDYES)
            				break;

        				gpSet->AppSettingsDelete(iCur);

						bool lbOld = bSkipSelChange; bSkipSelChange = true;

        				OnInitDialog_Apps(hWnd2, false);

        				int iCount = (int)SendDlgItemMessage(hWnd2, lbAppDistinct, LB_GETCOUNT, 0,0);
						bSkipSelChange = lbOld;
        				SendDlgItemMessage(hWnd2, lbAppDistinct, LB_SETCURSEL, min(iCur,(iCount-1)), 0);
        				pageOpProc_Apps(hWnd2, hChild, WM_COMMAND, MAKELONG(lbAppDistinct,LBN_SELCHANGE), 0);

					} // cbAppDistinctDel
					break;

				case cbAppDistinctUp:
				case cbAppDistinctDown:
					{
						int iCur, iChg;
						iCur = (int)SendDlgItemMessage(hWnd2, lbAppDistinct, LB_GETCURSEL, 0,0);
						if (iCur < 0)
							break;
						if (CB == cbAppDistinctUp)
						{
							if (!iCur)
								break;
            				iChg = iCur - 1;
        				}
        				else
        				{
        					iChg = iCur + 1;
        					if (iChg >= (int)SendDlgItemMessage(hWnd2, lbAppDistinct, LB_GETCOUNT, 0,0))
        						break;
        				}

        				if (!gpSet->AppSettingsXch(iCur, iChg))
        					break;

						bool lbOld = bSkipSelChange; bSkipSelChange = true;

        				OnInitDialog_Apps(hWnd2, false);

						bSkipSelChange = lbOld;

        				SendDlgItemMessage(hWnd2, lbAppDistinct, LB_SETCURSEL, iChg, 0);
					} // cbAppDistinctUp, cbAppDistinctDown
					break;

				case rbAppDistinctElevatedOn:
				case rbAppDistinctElevatedOff:
				case rbAppDistinctElevatedIgnore:
					if (pApp)
					{
						pApp->Elevated = IsChecked(hWnd2, rbAppDistinctElevatedOn) ? 1
							: IsChecked(hWnd2, rbAppDistinctElevatedOff) ? 2
							: 0;
						bRefill = bRedraw = true;
					}
					break;


				case cbColorsOverride:
					bChecked = IsChecked(hChild, CB);
					//EnableWindow(GetDlgItem(hChild, lbColorsOverride), bChecked);
					pageOpProc_Apps(hWnd2, hChild, UM_DISTINCT_ENABLE, (WPARAM)hWnd2, cbColorsOverride);
					if (pApp)
					{
						pApp->OverridePalette = bChecked;
						bRedraw = true;
					}
					break;

				case cbCursorOverride:
					bChecked = IsChecked(hChild, CB);
					//EnableWindow(GetDlgItem(hWnd2, rCursorV), bChecked);
					//EnableWindow(GetDlgItem(hWnd2, rCursorH), bChecked);
					//EnableWindow(GetDlgItem(hWnd2, cbCursorColor), bChecked);
					//EnableWindow(GetDlgItem(hWnd2, cbCursorBlink), bChecked);
					//EnableWindow(GetDlgItem(hWnd2, cbBlockInactiveCursor), bChecked);
					//EnableWindow(GetDlgItem(hWnd2, cbCursorIgnoreSize), bChecked);
					pageOpProc_Apps(hWnd2, hChild, UM_DISTINCT_ENABLE, (WPARAM)hWnd2, cbCursorOverride);
					if (pApp)
					{
						pApp->OverrideCursor = bChecked;
						bRedraw = true;
					}
					break;
				case rCursorV:
				case rCursorH:
					if (pApp)
					{
						pApp->isCursorV = IsChecked(hChild, rCursorV);
						bRedraw = true;
					}
					break;
				case cbCursorColor:
					if (pApp)
					{
						pApp->isCursorColor = IsChecked(hChild, CB);
						bRedraw = true;
					}
					break;
				case cbCursorBlink:
					if (pApp)
					{
						pApp->isCursorBlink = IsChecked(hChild, CB);
						//if (!gpSet->AppStd.isCursorBlink) // если мигание отключается - то курсор может "замереть" в погашенном состоянии.
						//	gpConEmu->ActiveCon()->Invalidate();
						bRedraw = true;
					}
					break;
				case cbBlockInactiveCursor:
					if (pApp)
					{
						pApp->isCursorBlockInactive = IsChecked(hChild, CB);
						bRedraw = true;
					}
					break;
				case cbCursorIgnoreSize:
					if (pApp)
					{
						pApp->isCursorIgnoreSize = IsChecked(hChild, CB);
						bRedraw = true;
					}
					break;

				case cbExtendFontsOverride:
					bChecked = IsChecked(hChild, CB);
					//EnableWindow(GetDlgItem(hWnd2, cbExtendFonts), bChecked);
					//EnableWindow(GetDlgItem(hWnd2, lbExtendFontBoldIdx), bChecked);
					//EnableWindow(GetDlgItem(hWnd2, lbExtendFontItalicIdx), bChecked);
					//EnableWindow(GetDlgItem(hWnd2, lbExtendFontNormalIdx), bChecked);
					pageOpProc_Apps(hWnd2, hChild, UM_DISTINCT_ENABLE, (WPARAM)hWnd2, cbExtendFontsOverride);
					if (!bSkipSelChange)
					{
						pApp->OverrideExtendFonts = IsChecked(hChild, CB);
						bRedraw = true;
					}
					break;
				case cbExtendFonts:
					if (pApp)
					{
						pApp->isExtendFonts = IsChecked(hChild, CB);
						bRedraw = true;
					}
					break;


				case cbClipboardOverride:
					bChecked = IsChecked(hChild, CB);
					//EnableWindow(GetDlgItem(hWnd2, cbClipShiftIns), bChecked);
					//EnableWindow(GetDlgItem(hWnd2, cbClipCtrlV), bChecked);
					pageOpProc_Apps(hWnd2, hChild, UM_DISTINCT_ENABLE, (WPARAM)hWnd2, cbClipboardOverride);
					if (!bSkipSelChange)
					{
						pApp->OverrideClipboard = IsChecked(hChild, CB);
					}
					break;
				case cbClipShiftIns:
					if (pApp)
					{
						pApp->isPasteAllLines = IsChecked(hChild, CB);
					}
					break;
				case cbClipCtrlV:
					if (pApp)
					{
						pApp->isPasteFirstLine = IsChecked(hChild, CB);
					}
					break;
				case cbCTSDetectLineEnd:
					if (pApp)
					{
						pApp->isCTSDetectLineEnd = IsChecked(hChild, CB);
					}
					break;
				case cbCTSBashMargin:
					if (pApp)
					{
						pApp->isCTSBashMargin = IsChecked(hChild, CB);
					}
					break;
				case cbCTSTrimTrailing:
					if (pApp)
					{
						pApp->isCTSTrimTrailing = IsChecked(hChild, CB);
					}
					break;
				case cbCTSClickPromptPosition:
					if (pApp)
					{
						pApp->isCTSClickPromptPosition = IsChecked(hChild, CB);
					}
					break;

				case cbBgImageOverride:
					bChecked = IsChecked(hChild, CB);
					pageOpProc_Apps(hWnd2, hChild, UM_DISTINCT_ENABLE, (WPARAM)hWnd2, cbBgImageOverride);
					if (!bSkipSelChange)
					{
						//pApp->OverrideBackground = IsChecked(hChild, CB);
					}
					break;
				case cbBgImage:
					if (pApp)
					{
						pApp->isShowBgImage = IsChecked(hChild, CB);
					}
					break;
				case bBgImage:
					if (pApp)
					{
						wchar_t temp[MAX_PATH], edt[MAX_PATH];
						if (!GetDlgItemText(hChild, tBgImage, edt, countof(edt)))
							edt[0] = 0;
						ExpandEnvironmentStrings(edt, temp, countof(temp));
						OPENFILENAME ofn; memset(&ofn,0,sizeof(ofn));
						ofn.lStructSize=sizeof(ofn);
						ofn.hwndOwner = ghOpWnd;
						ofn.lpstrFilter = L"All images (*.bmp,*.jpg,*.png)\0*.bmp;*.jpg;*.jpe;*.jpeg;*.png\0Bitmap images (*.bmp)\0*.bmp\0JPEG images (*.jpg)\0*.jpg;*.jpe;*.jpeg\0PNG images (*.png)\0*.png\0\0";
						ofn.nFilterIndex = 1;
						ofn.lpstrFile = temp;
						ofn.nMaxFile = countof(temp);
						ofn.lpstrTitle = L"Choose background image";
						ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
									| OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST;

						if (GetOpenFileName(&ofn))
						{
							TODO("LoadBackgroundFile");
							//if (LoadBackgroundFile(temp, true))
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
									wcscpy_c(pApp->sBgImage, L"%ConEmuDir%");
									wcscat_c(pApp->sBgImage, temp + _tcslen(gpConEmu->ms_ConEmuExeDir));
								}
								else
								{
									wcscpy_c(pApp->sBgImage, temp);
								}
								SetDlgItemText(hChild, tBgImage, pApp->sBgImage);
								gpConEmu->Update(true);
							}
						}
					} // bBgImage
					break;
				}	
			} // if (HIWORD(wParam) == BN_CLICKED)
			else if (HIWORD(wParam) == EN_CHANGE)
			{
				WORD ID = LOWORD(wParam);
				int iCur = bSkipSelChange ? -1 : (int)SendDlgItemMessage(hWnd2, lbAppDistinct, LB_GETCURSEL, 0,0);
				Settings::AppSettings* pApp = (iCur < 0) ? NULL : gpSet->GetAppSettingsPtr(iCur);
				_ASSERTE((iCur<0) || (pApp && pApp->AppNames));

				if (pApp)
				{
					switch (ID)
					{
					case tAppDistinctName:
						if (!bSkipEditChange)
						{
							Settings::AppSettings* pApp = gpSet->GetAppSettingsPtr(iCur);
							if (!pApp || !pApp->AppNames)
							{
								_ASSERTE(!pApp && pApp->AppNames);
							}
							else
							{
								wchar_t* pszApps = NULL;
								if (GetString(hWnd2, ID, &pszApps))
								{
									pApp->SetNames(pszApps);
									bRefill = bRedraw = true;
								}
								SafeFree(pszApps);
							}
						} // tAppDistinctName
						break;

					case tCursorFixedSize:
						if (pApp)
						{
							BOOL lbOk = FALSE;
							UINT nNewVal = GetDlgItemInt(hChild, tCursorFixedSize, &lbOk, FALSE);
							if (lbOk)
							{
								if ((nNewVal >= CURSORSIZE_MIN) && (nNewVal <= CURSORSIZE_MAX) && (pApp->nCursorFixedSize != nNewVal))
								{
									pApp->nCursorFixedSize = nNewVal;
									bRedraw = true;
								}
							}
						} //case tCursorFixedSize:
						break;

					case tCursorMinSize:
						if (pApp)
						{
							BOOL lbOk = FALSE;
							UINT nNewVal = GetDlgItemInt(hChild, tCursorMinSize, &lbOk, FALSE);
							if (lbOk)
							{
								if ((nNewVal >= CURSORSIZEPIX_MIN) && (nNewVal <= CURSORSIZEPIX_MAX) && (pApp->nCursorMinSize != nNewVal))
								{
									pApp->nCursorMinSize = nNewVal;
									bRedraw = true;
								}
							}
						} //case tCursorMinSize:
						break;

					case tBgImage:
						if (pApp)
						{
							wchar_t temp[MAX_PATH];
							GetDlgItemText(hChild, tBgImage, temp, countof(temp));

							if (wcscmp(temp, pApp->sBgImage))
							{
								TODO("LoadBackgroundFile");
								//if (LoadBackgroundFile(temp, true))
								{
									wcscpy_c(pApp->sBgImage, temp);
									gpConEmu->Update(true);
								}
							}
						} // tBgImage
                    	break;
					}
				}
			} // if (HIWORD(wParam) == EN_CHANGE)
			else if (HIWORD(wParam) == LBN_SELCHANGE)
			{
				WORD CB = LOWORD(wParam);

				if (CB == lbAppDistinct)
				{
					if (bSkipSelChange)
						break; // WM_COMMAND

					const Settings::AppSettings* pApp = NULL;
					//while ((pApp = gpSet->GetAppSettings(nApp)) && pApp->AppNames)
					int iCur = (int)SendDlgItemMessage(hWnd2, lbAppDistinct, LB_GETCURSEL, 0,0);
					if (iCur >= 0)
						pApp = gpSet->GetAppSettings(iCur);
					if (pApp && pApp->AppNames)
					{
						if (!bSkipEditSet)
						{
							bool lbCur = bSkipEditChange; bSkipEditChange = true;
							SetDlgItemText(hWnd2, tAppDistinctName, pApp->AppNames);
							bSkipEditChange = lbCur;
						}

						pageOpProc_Apps(hWnd2, hChild, UM_FILL_CONTROLS, (WPARAM)hWnd2, (LPARAM)pApp);
					}
					else
					{
						SetDlgItemText(hWnd2, tAppDistinctName, L"");
						checkRadioButton(hWnd2, rbAppDistinctElevatedOn, rbAppDistinctElevatedIgnore, rbAppDistinctElevatedIgnore);
					}
					
					bool lbOld = bSkipSelChange; bSkipSelChange = true;
					pageOpProc_Apps(hWnd2, hChild, UM_DISTINCT_ENABLE, (WPARAM)hWnd2, 0);
					bSkipSelChange = lbOld;
				} // if (CB == lbAppDistinct)
				else
				{
					int iCur = bSkipSelChange ? -1 : (int)SendDlgItemMessage(hWnd2, lbAppDistinct, LB_GETCURSEL, 0,0);
					Settings::AppSettings* pApp = (iCur < 0) ? NULL : gpSet->GetAppSettingsPtr(iCur);
					_ASSERTE((iCur<0) || (pApp && pApp->AppNames));

					if (pApp)
					{
						switch (CB)
						{
						case lbExtendFontBoldIdx:
						case lbExtendFontItalicIdx:
						case lbExtendFontNormalIdx:
							{
								if (CB == lbExtendFontNormalIdx)
									pApp->nFontNormalColor = GetNumber(hChild, CB);
								else if (CB == lbExtendFontBoldIdx)
									pApp->nFontBoldColor = GetNumber(hChild, CB);
								else if (CB == lbExtendFontItalicIdx)
									pApp->nFontItalicColor = GetNumber(hChild, CB);

								if (pApp->isExtendFonts)
									bRedraw = true;
							} // lbExtendFontBoldIdx, lbExtendFontItalicIdx, lbExtendFontNormalIdx
							break;

						case lbColorsOverride:
							{
								HWND hList = GetDlgItem(hChild, CB);
								INT_PTR nIdx = SendMessage(hList, CB_GETCURSEL, 0, 0);
								if (nIdx >= 0)
								{
									INT_PTR nLen = SendMessage(hList, CB_GETLBTEXTLEN, nIdx, 0);
									wchar_t* pszText = (nLen > 0) ? (wchar_t*)calloc((nLen+1),sizeof(wchar_t)) : NULL;
									if (pszText)
									{
										SendMessage(hList, CB_GETLBTEXT, nIdx, (LPARAM)pszText);
										int iPal = (nIdx == 0) ? -1 : gpSet->PaletteGetIndex(pszText);
										if ((nIdx == 0) || (iPal >= 0))
										{
											lstrcpyn(pApp->szPaletteName, (nIdx == 0) ? L"" : pszText, countof(pApp->szPaletteName));
											_ASSERTE(iCur>=0 && iCur<gpSet->AppCount && gpSet->AppColors);
											const Settings::ColorPalette* pPal = gpSet->PaletteGet(iPal);
											if (pPal)
											{
												memmove(gpSet->AppColors[iCur]->Colors, pPal->Colors, sizeof(pPal->Colors));
												BOOL bTextAttr = (pApp->nTextColorIdx != pPal->nTextColorIdx) || (pApp->nBackColorIdx != pPal->nBackColorIdx);
												pApp->nTextColorIdx = pPal->nTextColorIdx;
												pApp->nBackColorIdx = pPal->nBackColorIdx;
												BOOL bPopupAttr = (pApp->nPopTextColorIdx != pPal->nPopTextColorIdx) || (pApp->nPopBackColorIdx != pPal->nPopBackColorIdx);
												pApp->nPopTextColorIdx = pPal->nPopTextColorIdx;
												pApp->nPopBackColorIdx = pPal->nPopBackColorIdx;
												pApp->isExtendColors = pPal->isExtendColors;
												pApp->nExtendColorIdx = pPal->nExtendColorIdx;
												gpSet->AppColors[iCur]->FadeInitialized = false;
												if (bTextAttr || bPopupAttr)
													gpSetCls->UpdateTextColorSettings(bTextAttr, bPopupAttr);
												bRedraw = true;
											}
											else
											{
												_ASSERTE(pPal!=NULL);
											}
										}
									}
								}
							} // lbColorsOverride
							break;

						case lbCTSEOL:
							{
								BYTE eol = 0;
								GetListBoxByte(hChild,lbCTSEOL,SettingsNS::szCRLF,SettingsNS::nCRLF,eol);
								pApp->isCTSEOL = eol;
							} // lbCTSEOL
							break;

						case lbBgPlacement:
							{
								BYTE bg = 0;
								GetListBoxByte(hChild,lbBgPlacement,SettingsNS::szBgOper,SettingsNS::nBgOper,bg);
								pApp->nBgOperation = bg;
								TODO("LoadBackgroundFile");
								//gpSetCls->LoadBackgroundFile(gpSet->sBgImage, true);
								gpConEmu->Update(true);
							} // lbBgPlacement
							break;
						}
					}
				}
			} // if (HIWORD(wParam) == LBN_SELCHANGE)
		} // WM_COMMAND
		break;
	} // switch (messg)

	if (bRedraw)
		gpConEmu->Update(true); // в принципе, обновлять нужно если только настройки видимой консоли поменялись, но...

	if (bRefill)
	{
		bool lbOld = bSkipSelChange; bSkipSelChange = true;
		bool lbOldSet = bSkipEditSet; bSkipEditSet = true;
		INT_PTR iSel = SendDlgItemMessage(hWnd2, lbAppDistinct, LB_GETCURSEL, 0,0);
		gpSetCls->OnInitDialog_Apps(hWnd2, false);
		SendDlgItemMessage(hWnd2, lbAppDistinct, LB_SETCURSEL, iSel,0);
		bSkipSelChange = lbOld;
		bSkipEditSet = lbOldSet;
		bRefill = false;
	}

	return iRc;
}

void CSettings::debugLogShell(HWND hWnd2, DebugLogShellActivity *pShl)
{
	if (gpSetCls->m_ActivityLoggingType != glt_Processes)
	{
		_ASSERTE(gpSetCls->m_ActivityLoggingType == glt_Processes);
		return;
	}

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
	_ASSERTE(gpSetCls->m_ActivityLoggingType != glt_None);
	_ASSERTE(pszParamEx!=NULL && asFile && *asFile);

	HANDLE hFile = CreateFile(asFile, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER liSize = {};

	size_t cchMax = 32770;
	char *szBuf = (char*)malloc(cchMax);
	DWORD nRead = 0;

	if (hFile && hFile != INVALID_HANDLE_VALUE
		&& GetFileSizeEx(hFile, &liSize) && (liSize.QuadPart < 0xFFFFFF)
		&& ReadFile(hFile, szBuf, cchMax-3, &nRead, NULL) && nRead)
	{
		szBuf[nRead] = 0; szBuf[nRead+1] = 0; szBuf[nRead+2] = 0;
		CloseHandle(hFile); hFile = NULL;

		bool lbUnicode = false;
		LPCWSTR pszExt = PointToExt(asFile);
		size_t nAll = 0;
		wchar_t* pszNew = NULL;

		// Для расширений помимо ".bat" и ".cmd" - проверить содержимое
		if (lstrcmpi(pszExt, L".bat")!=0 && lstrcmpi(pszExt, L".cmd")!=0)
		{
			// например, ".tmp" файлы
			for (UINT i = 0; i < nRead; i++)
			{
				if (szBuf[i] == 0)
				{
					TODO("Может файл просто юникодный?");
					goto wrap;
				}
			}
		}

		nAll = (lstrlen(pszParamEx)+20) + nRead + 1 + 2*lstrlen(asFile);
		pszNew = (wchar_t*)realloc(pszParamEx, nAll*sizeof(wchar_t));
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
wrap:
	free(szBuf);
	if (hFile && hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
}

void CSettings::debugLogInfo(HWND hWnd2, CESERVER_REQ_PEEKREADINFO* pInfo)
{
	if (gpSetCls->m_ActivityLoggingType != glt_Input)
	{
		_ASSERTE(gpSetCls->m_ActivityLoggingType == glt_Input);
		return;
	}

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
	if ((m_ActivityLoggingType != glt_Commands) || (mh_Tabs[thi_Debug] == NULL))
		return;

	_ASSERTE(abInput==TRUE || pResult!=NULL || (pInfo->hdr.nCmd==CECMD_LANGCHANGE || pInfo->hdr.nCmd==CECMD_GUICHANGED || pInfo->hdr.nCmd==CMD_FARSETCHANGED));
		
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
	
	PostMessage(gpSetCls->mh_Tabs[thi_Debug], DBGMSG_LOG_ID, DBGMSG_LOG_CMD_MAGIC, (LPARAM)pData);
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
				size_t cchText = 65535;
				wchar_t *szText = (wchar_t*)malloc(cchText*sizeof(*szText));
				if (!szText)
					return 0;
				szText[0] = 0;
				if (gpSetCls->m_ActivityLoggingType == glt_Processes)
				{
					ListView_GetItemText(hList, p->iItem, lpc_App, szText, cchText);
					SetDlgItemText(hWnd2, ebActivityApp, szText);
					ListView_GetItemText(hList, p->iItem, lpc_Params, szText, cchText);
					SetDlgItemText(hWnd2, ebActivityParm, szText);
				}
				else if (gpSetCls->m_ActivityLoggingType == glt_Input)
				{
					ListView_GetItemText(hList, p->iItem, lic_Type, szText, cchText);
					_wcscat_c(szText, cchText, L" - ");
					int nLen = _tcslen(szText);
					ListView_GetItemText(hList, p->iItem, lic_Dup, szText+nLen, cchText-nLen);
					SetDlgItemText(hWnd2, ebActivityApp, szText);
					ListView_GetItemText(hList, p->iItem, lic_Event, szText, cchText);
					SetDlgItemText(hWnd2, ebActivityParm, szText);
				}
				else if (gpSetCls->m_ActivityLoggingType == glt_Commands)
				{
					ListView_GetItemText(hList, p->iItem, lcc_Pipe, szText, cchText);
					SetDlgItemText(hWnd2, ebActivityApp, szText);
					SetDlgItemText(hWnd2, ebActivityParm, L"");
				}
				else
				{
					SetDlgItemText(hWnd2, ebActivityApp, L"");
					SetDlgItemText(hWnd2, ebActivityParm, L"");
				}
				free(szText);
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

void CSettings::UpdateWindowMode(WORD WndMode)
{
	_ASSERTE(WndMode==rNormal || WndMode==rMaximized || WndMode==rFullScreen);

	if (gpSet->isQuakeStyle)
	{
		return;
	}

	if (gpSet->isUseCurrentSizePos)
	{
		gpSet->_WindowMode = WndMode;
	}

	if (ghOpWnd && mh_Tabs[thi_SizePos] && gpSet->isUseCurrentSizePos)
	{
		checkRadioButton(gpSetCls->mh_Tabs[thi_SizePos], rNormal, rFullScreen, WndMode);
	}
}

void CSettings::UpdatePos(int ax, int ay, bool bGetRect)
{
	int x = ax, y = ay;

	if (!gpConEmu->isFullScreen() && !gpConEmu->isZoomed())
	{
		if (!gpConEmu->isIconic())
		{
			RECT rc; GetWindowRect(ghWnd, &rc);
			x = rc.left; y = rc.top;
		}
	}

	if ((gpConEmu->wndX != x) || (gpConEmu->wndY != y))
	{
		gpConEmu->wndX = x;
		gpConEmu->wndY = y;
	}
	
	if (gpSet->isUseCurrentSizePos)
	{
		gpSet->_wndX = x;
		gpSet->_wndY = y;
	}

	if (ghOpWnd && mh_Tabs[thi_SizePos])
	{
		mb_IgnoreEditChanged = TRUE;
		SetDlgItemInt(mh_Tabs[thi_SizePos], tWndX, gpSet->isUseCurrentSizePos ? gpConEmu->wndX : gpSet->_wndX, TRUE);
		SetDlgItemInt(mh_Tabs[thi_SizePos], tWndY, gpSet->isUseCurrentSizePos ? gpConEmu->wndY : gpSet->_wndY, TRUE);
		mb_IgnoreEditChanged = FALSE;
	}

	if (isAdvLogging >= 2)
	{
		wchar_t szLabel[128];
		_wsprintf(szLabel, SKIPLEN(countof(szLabel)) L"UpdatePos A={%i,%i} C={%i,%i} S={%i,%i}", ax,ay, gpConEmu->wndX, gpConEmu->wndY, gpSet->_wndX, gpSet->_wndY);
		gpConEmu->LogWindowPos(szLabel);
	}
}

void CSettings::UpdateSize(UINT w, UINT h)
{
	bool bUserCurSize = gpSet->isUseCurrentSizePos;
	bool bIgnoreWidth = (gpSet->isQuakeStyle != 0) && (gpSet->_WindowMode != wmNormal);

	if (w>=MIN_CON_WIDTH && h>=MIN_CON_HEIGHT)
	{
		if (w!=gpConEmu->wndWidth || h!=gpConEmu->wndHeight)
		{
			gpConEmu->wndWidth = w;
			gpConEmu->wndHeight = h;
		}

		if (bUserCurSize && (w!=gpSet->_wndWidth || h!=gpSet->_wndHeight))
		{
			if (!bIgnoreWidth)
				gpSet->_wndWidth = w;
			gpSet->_wndHeight = h;
		}
	}

	if (ghOpWnd && mh_Tabs[thi_SizePos])
	{
		mb_IgnoreEditChanged = TRUE;
		SetDlgItemInt(mh_Tabs[thi_SizePos], tWndWidth, bUserCurSize ? gpConEmu->wndWidth : gpSet->_wndWidth, FALSE);
		SetDlgItemInt(mh_Tabs[thi_SizePos], tWndHeight, bUserCurSize ? gpConEmu->wndHeight : gpSet->_wndHeight, FALSE);
		mb_IgnoreEditChanged = FALSE;

		// Во избежание недоразумений - запретим элементы размера для Max/Fullscreen
		BOOL bNormalChecked = IsChecked(mh_Tabs[thi_SizePos], rNormal);
		for (size_t i = 0; i < countof(SettingsNS::nSizeCtrlId); i++)
		{
			EnableWindow(GetDlgItem(mh_Tabs[thi_SizePos], SettingsNS::nSizeCtrlId[i]), bNormalChecked);
		}
	}

	if (isAdvLogging >= 2)
	{
		wchar_t szLabel[128];
		_wsprintf(szLabel, SKIPLEN(countof(szLabel)) L"UpdateSize A={%i,%i} C={%i,%i} S={%i,%i}", w,h, gpConEmu->wndWidth, gpConEmu->wndHeight, gpSet->_wndWidth, gpSet->_wndHeight);
		gpConEmu->LogWindowPos(szLabel);
	}
}

// Пожалуй, не будем автоматически менять флажок "Monospace"
// Был вроде Issue, да и не всегда моноширность правильно определяется (DejaVu Sans Mono)
void CSettings::UpdateTTF(BOOL bNewTTF)
{
	if (mb_IgnoreTtfChange)
		return;

	if (GetSystemMetrics(SM_DBCSENABLED) != 0)
		return;
	
	if (!bNewTTF)
	{
		gpSet->isMonospace = bNewTTF ? 0 : isMonospaceSelected;

		if (mh_Tabs[thi_Main])
			checkDlgButton(mh_Tabs[thi_Main], cbMonospace, gpSet->isMonospace); // 3state
	}
}

void CSettings::UpdateFontInfo()
{
	if (!ghOpWnd || !mh_Tabs[thi_Info]) return;

	wchar_t szTemp[32];
	_wsprintf(szTemp, SKIPLEN(countof(szTemp)) L"%ix%ix%i", LogFont.lfHeight, LogFont.lfWidth, m_tm->tmAveCharWidth);
	SetDlgItemText(mh_Tabs[thi_Info], tRealFontMain, szTemp);
	_wsprintf(szTemp, SKIPLEN(countof(szTemp)) L"%ix%i", LogFont2.lfHeight, LogFont2.lfWidth);
	SetDlgItemText(mh_Tabs[thi_Info], tRealFontBorders, szTemp);
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
			SetDlgItemText(mh_Tabs[thi_Info], nID, sTemp);

			for (nID = tPerfFPS; mn_Freq && (nID <= tPerfInterval); nID++)
			{
				SendMessage(gpSetCls->mh_Tabs[thi_Info], mn_MsgUpdateCounter, nID-tPerfFPS, 0);
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
			PostMessage(gpSetCls->mh_Tabs[thi_Info], mn_MsgUpdateCounter, nID-tPerfFPS, 0);

		return;
	}

	if (nID == tPerfInterval)
	{
		i64 tick2 = 0;
		QueryPerformanceCounter((LARGE_INTEGER *)&tick2);
		mn_RFPS[mn_RFPS_CUR_FRAME] = tick2;
		mn_RFPS_CUR_FRAME++;

		if (mn_RFPS_CUR_FRAME >= (int)countof(mn_RFPS))
			mn_RFPS_CUR_FRAME = 0;

		if (ghOpWnd)
			PostMessage(gpSetCls->mh_Tabs[thi_Info], mn_MsgUpdateCounter, nID-tPerfFPS, 0);

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
			PostMessage(gpSetCls->mh_Tabs[thi_Info], mn_MsgUpdateCounter, nID-tPerfFPS, 0);
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
			tiBalloon.hwnd = mh_Tabs[thi_Main];
			tiBalloon.hinst = g_hInstance;
			static wchar_t szAsterisk[] = L"*"; // eliminate GCC warning
			tiBalloon.lpszText = szAsterisk;
			tiBalloon.uId = (UINT_PTR)mh_Tabs[thi_Main];
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
					toolInfo.hwnd = mh_Tabs[thi_Main];
					toolInfo.hinst = g_hInstance;
					toolInfo.lpszText = L"*";
					toolInfo.uId = (UINT_PTR)mh_Tabs[thi_Main];
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
		CVConGroup::LogString(szInfo);
	}

	CEFONT hf = CreateFontIndirectMy(&LF);

	if (hf.IsSet())
	{
		// SaveFontSizes выполним после обновления LogFont, т.к. там зовется gpConEmu->OnPanelViewSettingsChanged
		CEFONT hOldF = mh_Font[0];
		LogFont = LF;
		mh_Font[0] = hf;
		hOldF.Delete();
		SaveFontSizes(&LF, (mn_AutoFontWidth == -1), true);
		gpConEmu->Update(true);

		if (gpConEmu->WindowMode == wmNormal)
			gpConEmu->SyncWindowToConsole(); // -- функция пустая, игнорируется
		else
			gpConEmu->SyncConsoleToWindow();

		gpConEmu->ReSize();
	}

	if (ghOpWnd)
	{
		wchar_t szSize[10];
		_wsprintf(szSize, SKIPLEN(countof(szSize)) L"%i", CurFontSizeY);
		SetDlgItemText(mh_Tabs[thi_Main], tFontSizeY, szSize);
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
		GetDlgItemText(mh_Tabs[thi_Main], tFontFace, LF.lfFaceName, countof(LF.lfFaceName));
		LF.lfHeight = gpSet->FontSizeY = GetNumber(mh_Tabs[thi_Main], tFontSizeY);
		LF.lfWidth = gpSet->FontSizeX = GetNumber(mh_Tabs[thi_Main], tFontSizeX);
		LF.lfWeight = IsChecked(mh_Tabs[thi_Main], cbBold) ? FW_BOLD : FW_NORMAL;
		LF.lfItalic = IsChecked(mh_Tabs[thi_Main], cbItalic);
		LF.lfCharSet = gpSet->mn_LoadFontCharSet;

		if (gpSet->mb_CharSetWasSet)
		{
			//gpSet->mb_CharSetWasSet = FALSE;
			INT_PTR newCharSet = SendDlgItemMessage(mh_Tabs[thi_Main], tFontCharset, CB_GETCURSEL, 0, 0);

			if (newCharSet != CB_ERR && newCharSet >= 0 && newCharSet < (INT_PTR)countof(SettingsNS::nCharSets))
				LF.lfCharSet = SettingsNS::nCharSets[newCharSet];
			else
				LF.lfCharSet = DEFAULT_CHARSET;
		}

		if (IsChecked(mh_Tabs[thi_Main], rNoneAA))
			LF.lfQuality = NONANTIALIASED_QUALITY;
		else if (IsChecked(mh_Tabs[thi_Main], rStandardAA))
			LF.lfQuality = ANTIALIASED_QUALITY;
		else if (IsChecked(mh_Tabs[thi_Main], rCTAA))
			LF.lfQuality = CLEARTYPE_NATURAL_QUALITY;

		GetDlgItemText(mh_Tabs[thi_Main], tFontFace2, LogFont2.lfFaceName, countof(LogFont2.lfFaceName));
		gpSet->FontSizeX2 = GetNumber(mh_Tabs[thi_Main], tFontSizeX2);
		gpSet->FontSizeX3 = GetNumber(mh_Tabs[thi_Main], tFontSizeX3);

		if (isAdvLogging)
		{
			char szInfo[128]; _wsprintfA(szInfo, SKIPLEN(countof(szInfo)) "AutoRecreateFont(H=%i, W=%i)", LF.lfHeight, LF.lfWidth);
			CVConGroup::LogString(szInfo);
		}
	}

	CEFONT hf = CreateFontIndirectMy(&LF);

	if (hf.IsSet())
	{
		// SaveFontSizes выполним после обновления LogFont, т.к. там зовется gpConEmu->OnPanelViewSettingsChanged
		CEFONT hOldF = mh_Font[0];
		LogFont = LF;
		mh_Font[0] = hf;
		hOldF.Delete();
		SaveFontSizes(&LF, (mn_AutoFontWidth == -1), true);
		gpConEmu->Update(true);

		if (gpConEmu->WindowMode == wmNormal)
			gpConEmu->SyncWindowToConsole(); // -- функция пустая, игнорируется
		else
			gpConEmu->SyncConsoleToWindow();

		gpConEmu->ReSize();
	}

	if (ghOpWnd && wFromID == tFontFace)
	{
		wchar_t szSize[10];
		_wsprintf(szSize, SKIPLEN(countof(szSize)) L"%i", CurFontSizeY);
		SetDlgItemText(mh_Tabs[thi_Main], tFontSizeY, szSize);
	}

	if (ghOpWnd)
	{
		UpdateFontInfo();
	
		ShowFontErrorTip(gpSetCls->szFontError);
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
	ShowErrorTip(asInfo, gpSetCls->mh_Tabs[thi_Main], tFontFace, gpSetCls->szFontError, countof(gpSetCls->szFontError),
	             gpSetCls->hwndBalloon, &gpSetCls->tiBalloon, gpSetCls->hwndTip, FAILED_FONT_TIMEOUT);
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
		CEFONT hf = CreateFontIndirectMy(&LF);

		// Успешно, только если шрифт изменился, или хотели поставить абсолютный размер
		if (hf.IsSet() && ((nRelative == 0) || (LF.lfHeight != LogFont.lfHeight)))
		{
			// SaveFontSizes выполним после обновления LogFont, т.к. там зовется gpConEmu->OnPanelViewSettingsChanged
			CEFONT hOldF = mh_Font[0];
			LogFont = LF;
			mh_Font[0] = hf;
			hOldF.Delete();
			// Запомнить размер шрифта (AutoFontWidth/Height - может быть другим, он запоминается выше)
			SaveFontSizes(&LF, false, true);
			// Передернуть размер консоли
			gpConEmu->OnSize();
			// Передернуть флажки, что шрифт поменялся
			gpConEmu->Update(true);

			if (ghOpWnd)
			{
				gpSetCls->UpdateFontInfo();

				if (mh_Tabs[thi_Main])
				{
					wchar_t temp[16];
					_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", CurFontSizeY);
					SelectStringExact(mh_Tabs[thi_Main], tFontSizeY, temp);
					_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", gpSet->FontSizeX);
					SelectStringExact(mh_Tabs[thi_Main], tFontSizeX, temp);
				}
			}

			return true;
		}

		hf.Delete();

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
		CVConGroup::LogString(szInfo);
	}

	// Сразу запомним, какой размер просили в последний раз
	mn_AutoFontWidth = nFontW; mn_AutoFontHeight = nFontH;
	// Пытаемся создать новый шрифт
	LOGFONT LF = LogFont;
	LF.lfWidth = nFontW;
	LF.lfHeight = nFontH;
	CEFONT hf = CreateFontIndirectMy(&LF);

	if (hf.IsSet())
	{
		// SaveFontSizes выполним после обновления LogFont, т.к. там зовется gpConEmu->OnPanelViewSettingsChanged
		CEFONT hOldF = mh_Font[0];
		LogFont = LF;
		mh_Font[0] = hf;
		hOldF.Delete();
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

bool CSettings::FindCustomFont(LPCWSTR lfFaceName, int iSize, BOOL bBold, BOOL bItalic, BOOL bUnderline, CustomFontFamily** ppCustom, CustomFont** ppFont)
{
	*ppFont = NULL;
	*ppCustom = NULL;

	// Поиск по шрифтам рисованным ConEmu (bdf)
	for (std::vector<RegFont>::iterator iter = m_RegFonts.begin(); iter != m_RegFonts.end(); ++iter)
	{
		if (iter->pCustom && lstrcmp(lfFaceName, iter->szFontName)==0)
		{
			*ppCustom = iter->pCustom;
			*ppFont = (*ppCustom)->GetFont(iSize, bBold, bItalic, bUnderline);

			if (!*ppFont)
			{	
				MBoxAssert(*ppFont != NULL);
			}

			return true; // [bdf] шрифт. ошибка опрделяется по (*ppFont==NULL)
		}
	}

	return false; // НЕ [bdf] шрифт
}

void CSettings::RecreateBorderFont(const LOGFONT *inFont)
{
	mh_Font2.Delete();

	// если ширина шрифта стала больше ширины FixFarBorders - сбросить ширину FixFarBorders в 0
	if (gpSet->FontSizeX2 && (LONG)gpSet->FontSizeX2 < inFont->lfWidth)
	{
		gpSet->FontSizeX2 = 0;

		if (ghOpWnd && mh_Tabs[thi_Main])
			SelectStringExact(mh_Tabs[thi_Main], tFontSizeX2, L"0");
	}

	// Поиск по шрифтам рисованным ConEmu (bdf)
	CustomFont* pFont = NULL;
	CustomFontFamily* pCustom = NULL;
	if (FindCustomFont(LogFont2.lfFaceName, inFont->lfHeight,
				inFont->lfWeight >= FW_BOLD, inFont->lfItalic, inFont->lfUnderline,
				&pCustom, &pFont))
	{
		if (!pFont)
		{	
			MBoxAssert(pFont != NULL);
			return;
		}

		// OK
		mh_Font2.iType = CEFONT_CUSTOM;
		mh_Font2.pCustomFont = pFont;
		return;
	}

	wchar_t szFontFace[32];
	// лучше для ghWnd, может разные мониторы имеют разные параметры...
	HDC hScreenDC = GetDC(ghWnd); // GetDC(0);
	HDC hDC = CreateCompatibleDC(hScreenDC);
	ReleaseDC(ghWnd, hScreenDC);
	hScreenDC = NULL;
	MBoxAssert(hDC);
	HFONT hOldF = NULL;

	//int width = gpSet->FontSizeX2 ? gpSet->FontSizeX2 : inFont->lfWidth;
	LogFont2.lfWidth = mn_BorderFontWidth = gpSet->FontSizeX2 ? gpSet->FontSizeX2 : inFont->lfWidth;
	LogFont2.lfHeight = abs(inFont->lfHeight);
	// Иначе рамки прерывистыми получаются... поставил NONANTIALIASED_QUALITY
	mh_Font2 = CEFONT(CreateFont(LogFont2.lfHeight, LogFont2.lfWidth, 0, 0, FW_NORMAL,
	                             0, 0, 0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
	                             NONANTIALIASED_QUALITY/*ANTIALIASED_QUALITY*/, 0, LogFont2.lfFaceName));

	if (mh_Font2.IsSet())
	{
		hOldF = (HFONT)SelectObject(hDC, mh_Font2.hFont);

		if (GetTextFace(hDC, countof(szFontFace), szFontFace))
		{
			szFontFace[countof(szFontFace)-1] = 0; // гарантировано ASCII-Z

			// Проверяем, совпадает ли имя созданного шрифта с запрошенным?
			if (lstrcmpi(LogFont2.lfFaceName, szFontFace))
			{
				if (szFontError[0]) wcscat_c(szFontError, L"\n");

				int nCurLen = _tcslen(szFontError);
				_wsprintf(szFontError+nCurLen, SKIPLEN(countof(szFontError)-nCurLen)
				          L"Failed to create border font!\nRequested: %s\nCreated: ", LogFont2.lfFaceName);

				// Если запрашивалась Люцида - оставляем (хотя это уже облом, должна быть)
				if (lstrcmpi(LogFont2.lfFaceName, gsDefGuiFont) == 0)
				{
					// только запомним что было реально создано
					lstrcpyn(LogFont2.lfFaceName, szFontFace, countof(LogFont2.lfFaceName));
				}
				else
				{
					// Иначе - пробуем создать Люциду (нам нужен шрифт с рамками)
					wcscpy_c(LogFont2.lfFaceName, gsDefGuiFont);
					SelectObject(hDC, hOldF);
					mh_Font2.Delete();

					mh_Font2 = CEFONT(CreateFont(LogFont2.lfHeight, LogFont2.lfWidth, 0, 0, FW_NORMAL,
					                             0, 0, 0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
					                             NONANTIALIASED_QUALITY/*ANTIALIASED_QUALITY*/, 0, LogFont2.lfFaceName));
					hOldF = (HFONT)SelectObject(hDC, mh_Font2.hFont);
					wchar_t szFontFace2[32];

					if (GetTextFace(hDC, countof(szFontFace2), szFontFace2))
					{
						szFontFace2[31] = 0;

						// Проверяем что создалось, и ругаемся, если что...
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
		DumpFontMetrics(L"mh_Font2", hDC, mh_Font2.hFont);
		#endif

		SelectObject(hDC, hOldF);
	}

	DeleteDC(hDC);
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
CEFONT CSettings::CreateFontIndirectMy(LOGFONT *inFont)
{
	//ResetFontWidth(); -- перенесено вниз, после того, как убедимся в валидности шрифта
	//lfOutPrecision = OUT_RASTER_PRECIS,
	szFontError[0] = 0;

	// Поиск по шрифтам рисованным ConEmu
	CustomFont* pFont = NULL;
	CustomFontFamily* pCustom = NULL;
	if (FindCustomFont(inFont->lfFaceName, inFont->lfHeight,
				inFont->lfWeight >= FW_BOLD, inFont->lfItalic, inFont->lfUnderline,
				&pCustom, &pFont))
	{
		if (!pFont)
		{	
			MBoxAssert(pFont != NULL);
			return (HFONT)NULL;
		}
		// Получить реальные размеры шрифта (обновить inFont)
		pFont->GetBoundingBox(&inFont->lfWidth, &inFont->lfHeight);
		ResetFontWidth();
		if (ghOpWnd)
			UpdateTTF(!pFont->IsMonospace());

		CEFONT ceFont;
		ceFont.iType = CEFONT_CUSTOM;
		ceFont.pCustomFont = pFont;

		for (int i = 0; i < MAX_FONT_STYLES; i++)
		{
			SafeFree(m_otm[i]);
			if (i)
			{
				mh_Font[i].iType = CEFONT_CUSTOM;
				mh_Font[i].pCustomFont = pCustom->GetFont(inFont->lfHeight,
					(i & AI_STYLE_BOLD     ) ? TRUE : FALSE,
					(i & AI_STYLE_ITALIC   ) ? TRUE : FALSE,
					(i & AI_STYLE_UNDERLINE) ? TRUE : FALSE);
			}
		}

		RecreateBorderFont(inFont);

		return ceFont;
	}

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

			if (mh_Tabs[thi_Main])
				checkDlgButton(mh_Tabs[thi_Main], cbFontAuto, BST_UNCHECKED);

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

				if (ghOpWnd && mh_Tabs[thi_Main])
				{
					wchar_t temp[32];
					_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", nRastHeight);
					SelectStringExact(mh_Tabs[thi_Main], tFontSizeY, temp);
					_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", nRastWidth);
					SelectStringExact(mh_Tabs[thi_Main], tFontSizeX, temp);
					SelectStringExact(mh_Tabs[thi_Main], tFontSizeX3, temp);
				}
			}
		}

		inFont->lfCharSet = OEM_CHARSET;
		tmpFont = *inFont;
		wcscpy_c(tmpFont.lfFaceName, L"Terminal");
	}

	if (gpSet->isMonospace)
	{
		tmpFont.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
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

		// Лучше поставим AveCharWidth. MaxCharWidth для "условно моноширного" Consolas почти равен высоте.
		inFont->lfWidth = gpSet->FontSizeX3 ? gpSet->FontSizeX3 : m_tm->tmAveCharWidth;
		// Обновлять реальный размер шрифта в диалоге настройки не будем, были случаи, когда
		// tmHeight был меньше, чем запрашивалось, однако, если пытаться создать шрифт с этим "обновленным"
		// размером - в реале создавался совсем другой шрифт...
		inFont->lfHeight = m_tm->tmHeight;

		if (lbTM && m_tm->tmCharSet != DEFAULT_CHARSET)
		{
			inFont->lfCharSet = m_tm->tmCharSet;

			for (uint i = 0; i < countof(SettingsNS::nCharSets); i++)
			{
				if (SettingsNS::nCharSets[i] == m_tm->tmCharSet)
				{
					SendDlgItemMessage(mh_Tabs[thi_Main], tFontCharset, CB_SETCURSEL, i, 0);
					break;
				}
			}
		}

		if (ghOpWnd)
		{
			// устанавливать только при листании шрифта в настройке
			TODO("Или через GuiMacro?");
			// при кликах по самому флажку "Monospace" шрифт не пересоздается (CreateFont... не вызывается)
			UpdateTTF(!bAlmostMonospace);    //(m_tm->tmMaxCharWidth - m_tm->tmAveCharWidth)>2
		}

		for (int s = 1; s < MAX_FONT_STYLES; s++)
		{
			mh_Font[s].Delete();

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
			mh_Font[s] = CEFONT(CreateFontIndirect(&tmpFont));
			SelectObject(hDC, mh_Font[s].hFont);
			lbTM = GetTextMetrics(hDC, m_tm+s);
			//_ASSERTE(lbTM);
			lpOutl = LoadOutline(hDC, mh_Font[s].hFont);

			if (lpOutl)
			{
				if (m_otm[s]) free(m_otm[s]);

				m_otm[s] = lpOutl; lpOutl = NULL;
			}
		}

		SelectObject(hDC, hOldF);
		DeleteDC(hDC);

		RecreateBorderFont(inFont);
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

int CSettings::checkDlgButton(HWND hParent, WORD nCtrlId, UINT uCheck)
{
#ifdef _DEBUG
	HWND hCheckBox = NULL;
	if (!hParent)
	{
		_ASSERTE(hParent!=NULL);
	}
	else
	{
		hCheckBox = GetDlgItem(hParent, nCtrlId);
		if (!hCheckBox)
		{
			_ASSERTE(hCheckBox!=NULL && "Checkbox not found in hParent dlg");
		}
		else
		{
			_ASSERTE(uCheck==BST_UNCHECKED || uCheck==BST_CHECKED || (uCheck==BST_INDETERMINATE && (BS_3STATE&GetWindowLong(hCheckBox,GWL_STYLE))));
		}
	}
#endif
	// Аналог CheckDlgButton
	BOOL bRc = CheckDlgButton(hParent, nCtrlId, uCheck);
	return bRc;
}

int CSettings::checkRadioButton(HWND hParent, int nIDFirstButton, int nIDLastButton, int nIDCheckButton)
{
#ifdef _DEBUG
	if (!hParent)
	{
		_ASSERTE(hParent!=NULL);
	}
	else
	{
		_ASSERTE(GetDlgItem(hParent, nIDFirstButton) && "Checkbox not found in hParent dlg");
		_ASSERTE(GetDlgItem(hParent, nIDLastButton) && "Checkbox not found in hParent dlg");
		_ASSERTE(GetDlgItem(hParent, nIDCheckButton) && "Checkbox not found in hParent dlg");
	}
#endif
	// Аналог CheckRadioButton
	BOOL bRc = CheckRadioButton(hParent, nIDFirstButton, nIDLastButton, nIDCheckButton);
	return bRc;
}

// FALSE - выключена
// TRUE (BST_CHECKED) - включена
// BST_INDETERMINATE (2) - 3-d state
int CSettings::IsChecked(HWND hParent, WORD nCtrlId)
{
#ifdef _DEBUG
	if (!hParent)
	{
		_ASSERTE(hParent!=NULL);
	}
	else
	{
		HWND hCheckBox = GetDlgItem(hParent, nCtrlId);
		_ASSERTE(hCheckBox!=NULL && "Checkbox not found in hParent dlg");
	}
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
			nValue = 255; // 0xFF для gpSet->AppStd.nFontNormalColor, gpSet->AppStd.nFontBoldColor, gpSet->AppStd.nFontItalicColor;
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

INT_PTR CSettings::GetSelectedString(HWND hParent, WORD nListCtrlId, wchar_t** ppszStr)
{
	INT_PTR nCur = SendDlgItemMessage(hParent, nListCtrlId, CB_GETCURSEL, 0, 0);
	INT_PTR nLen = (nCur >= 0) ? SendDlgItemMessage(hParent, nListCtrlId, CB_GETLBTEXTLEN, nCur, 0) : -1;
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
			SendDlgItemMessage(hParent, nListCtrlId, CB_GETLBTEXT, nCur, (LPARAM)pszNew);

			if (*ppszStr)
			{
				if (lstrcmp(*ppszStr, pszNew) == 0)
				{
					free(pszNew);
					return nLen; // Изменений не было
				}
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
		wchar_t temp[MAX_PATH] = {};

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
	if (anBufferHeight==0)
	{
		SetDefaultCmd(L"far");
	}
	else
	{
		wchar_t* pszComspec = GetComspec(&gpSet->ComSpec);
		_ASSERTE(pszComspec!=NULL);
		SetDefaultCmd(pszComspec ? pszComspec : L"cmd");
		SafeFree(pszComspec);
	}
}

void CSettings::SetDefaultCmd(LPCWSTR asCmd)
{
	if (gpConEmu && gpConEmu->mb_MingwMode && gpConEmu->mb_MSysStartup)
	{
		wchar_t szSearch[MAX_PATH+32], *pszFile;
		wcscpy_c(szSearch, gpConEmu->ms_ConEmuExeDir);
		pszFile = wcsrchr(szSearch, L'\\');
		if (pszFile)
			*pszFile = 0;
		wcscat_c(szSearch, L"\\msys\\1.0\\bin\\sh.exe");
		if (!FileExists(szSearch))
		{
			wcscpy_c(szSearch, L"sh.exe");
		}

		_wsprintf(szDefCmd, SKIPLEN(countof(szDefCmd))
			(wcschr(szSearch, L' ') != NULL)
				? L"\"%s\" --login -i" /* -new_console:n" */
				: L"%s --login -i" /* -new_console:n" */,
			szSearch);
	}
	else
	{
		wcscpy_c(szDefCmd, asCmd ? asCmd : L"cmd");
	}
}

LPCTSTR CSettings::GetDefaultCmd()
{
	_ASSERTE(szDefCmd[0]!=0);
	return szDefCmd;
}

// Если ConEmu был запущен с ключом "/single /cmd xxx" то после окончания
// загрузки - сбросить команду, которая пришла из "/cmd" - загрузить настройку
void CSettings::ResetCmdArg()
{
	SingleInstanceArg = false;
	// Сбросить нужно только gpSet->psCurCmd, gpSet->psCmd не меняется - загружается только из настройки
	SafeFree(gpSet->psCurCmd);
}

void CSettings::GetMainLogFont(LOGFONT& lf)
{
	lf = LogFont;
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
	BOOL lbRegistered = FALSE, lbOneOfFam = FALSE; int iFamIndex = -1;

	for (std::vector<RegFont>::iterator iter = m_RegFonts.begin(); iter != m_RegFonts.end(); ++iter)
	{
		// Это может быть другой тип шрифта (Liberation Mono Bold, Liberation Mono Regular, ...)
		if (lstrcmpi(iter->szFontName, rf.szFontName) == 0
			|| lstrcmpi(iter->szFontName, szFullFontName) == 0)
		{
			lbRegistered = iter->bAlreadyInSystem;
			lbOneOfFam = TRUE;
			iFamIndex = iter - m_RegFonts.begin();
			break;
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
	wcscpy_c(rf.szFontFile, asFontFile);

	LPCTSTR pszDot = _tcsrchr(asFontFile, _T('.'));
	if (pszDot && lstrcmpi(pszDot, _T(".bdf"))==0)
	{
		WARNING("Не загружать шрифт полностью - только имена/заголовок, а то слишком накладно по времени. Загружать при первом вызове.");
		CustomFont* pFont = BDF_Load(asFontFile);
		if (!pFont)
		{
			size_t cchLen = _tcslen(asFontFile)+100;
			wchar_t* psz=(wchar_t*)calloc(cchLen,sizeof(wchar_t));
			_wcscpy_c(psz, cchLen, L"Can't load BDF font:\n");
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

		if (lbOneOfFam)
		{
			// Добавим в существующее семейство
			_ASSERTE(iFamIndex >= 0);
			m_RegFonts[iFamIndex].pCustom->AddFont(pFont);
			return TRUE;
		}

		rf.pCustom = new CustomFontFamily();
		rf.pCustom->AddFont(pFont);
		rf.bUnicode = pFont->HasUnicode();
		rf.bHasBorders = pFont->HasBorders();

		// Запомнить шрифт
		m_RegFonts.push_back(rf);
		return TRUE;
	}

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
				if (!pszDot || (lstrcmpi(pszDot, _T(".ttf")) && lstrcmpi(pszDot, _T(".otf")) /*&& lstrcmpi(pszDot, _T(".fon"))*/ && lstrcmpi(pszDot, _T(".bdf")) ))
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
		if (iter->pCustom)
			delete iter->pCustom;
		else
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

	if (!lstrcmpi(pszDot, _T(".bdf")))
		return GetFontNameFromFile_BDF(lpszFilePath, rsFontName, rsFullFontName);

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

// Retrieve Family name from BDF file
BOOL CSettings::GetFontNameFromFile_BDF(LPCTSTR lpszFilePath, wchar_t (&rsFontName)[LF_FACESIZE], wchar_t (&rsFullFontName)[LF_FACESIZE])
{
	if (!BDF_GetFamilyName(lpszFilePath, rsFontName))
		return FALSE;
	wcscat_c(rsFontName, CE_BDF_SUFFIX/*L" [BDF]"*/);
	lstrcpy(rsFullFontName, rsFontName);
	return TRUE;
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
	if (mh_Tabs[thi_Info] && IsWindow(mh_Tabs[thi_Info]))
	{
		wchar_t szInfo[255];
		_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"Console states (0x%X)", nMode);
		SetDlgItemText(mh_Tabs[thi_Info], IDC_CONSOLE_STATES, szInfo);
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
	_ASSERTE(hWnd2!=NULL);
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
	_ASSERTE(hWnd2!=NULL);
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
					CVConGuard VCon;
					const CEFAR_INFO_MAPPING *pfi = (CVConGroup::GetActiveVCon(&VCon) >= 0) ? VCon->RCon()->GetFarInfo() : NULL;

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
		} // if (c >= c32 && c <= c34)
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
	switch (nID)
	{
	case c32:
		*color = gpSet->ThSet.crBackground.ColorRGB;
		break;
	case c33:
		*color = gpSet->ThSet.crPreviewFrame.ColorRGB;
		break;
	case c34:
		*color = gpSet->ThSet.crSelectFrame.ColorRGB;
		break;
	case c35:
		*color = gpSet->nStatusBarBack;
		break;
	case c36:
		*color = gpSet->nStatusBarLight;
		break;
	case c37:
		*color = gpSet->nStatusBarDark;
		break;
	case c38:
		*color = gpSet->nColorKeyValue;
		break;

	default:
		if (nID <= c31)
			*color = gpSet->Colors[nID - c0];
		else
			return false;
	}
	
	return true;
}

bool CSettings::SetColorById(WORD nID, COLORREF color)
{
	switch (nID)
	{
	case c32:
		gpSet->ThSet.crBackground.ColorRGB = color;
		break;
	case c33:
		gpSet->ThSet.crPreviewFrame.ColorRGB = color;
		break;
	case c34:
		gpSet->ThSet.crSelectFrame.ColorRGB = color;
		break;
	case c35:
		gpSet->nStatusBarBack = color;
		gpConEmu->mp_Status->UpdateStatusBar(true);
		break;
	case c36:
		gpSet->nStatusBarLight = color;
		gpConEmu->mp_Status->UpdateStatusBar(true);
		break;
	case c37:
		gpSet->nStatusBarDark = color;
		gpConEmu->mp_Status->UpdateStatusBar(true);
		break;
	case c38:
		gpSet->nColorKeyValue = color;
		gpConEmu->OnTransparent();
		break;

	default:
		if (nID <= c31)
		{
			gpSet->Colors[nID - c0] = color;
			gpSet->mb_FadeInitialized = false;
		}
		else
			return false;
	}

	return true;
}

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

void CSettings::OnPanelViewAppeared(BOOL abAppear)
{
	if (mh_Tabs[thi_Views] && IsWindow(mh_Tabs[thi_Views]))
	{
		if (abAppear != IsWindowEnabled(GetDlgItem(mh_Tabs[thi_Views],bApplyViewSettings)))
			EnableWindow(GetDlgItem(mh_Tabs[thi_Views],bApplyViewSettings), abAppear);
	}
}

#ifndef APPDISTINCTBACKGROUND
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
#endif

// Должна вернуть true, если файл изменился
// Работает ТОЛЬКО с файлом. Данные плагинов обрабатываются в самом CVirtualConsole!
bool CSettings::PrepareBackground(CVirtualConsole* apVCon, HDC* phBgDc, COORD* pbgBmpSize)
{
	#ifdef APPDISTINCTBACKGROUND
	PRAGMA_ERROR("Пенесести основной код в CBackgroundFile!");
	#endif

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
		if ((gpSet->bgOperation == eUpLeft) || (gpSet->bgOperation == eUpRight)
			|| (gpSet->bgOperation == eDownLeft) || (gpSet->bgOperation == eDownRight))
		{
			// MemoryDC создается всегда по размеру картинки, т.е. изменение размера окна - игнорируется
		}
		else
		{
			RECT rcWnd, rcWork; GetClientRect(ghWnd, &rcWnd);
			WARNING("DoubleView: тут непонятно, какой и чей размер, видимо, нужно ветвиться, и хранить Background в самих VCon");
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
					// Сюда мы можем попасть только в случае eUpLeft/eUpRight/eDownLeft/eDownRight
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

#ifndef APPDISTINCTBACKGROUND
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
	
	return lRes;
}
#endif

void CSettings::NeedBackgroundUpdate()
{
	mb_NeedBgUpdate = TRUE;
}

// общая функция
void CSettings::ShowErrorTip(LPCTSTR asInfo, HWND hDlg, int nCtrlID, wchar_t* pszBuffer, int nBufferSize, HWND hBall, TOOLINFO *pti, HWND hTip, DWORD nTimeout, bool bLeftAligh)
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
		int ptx = bLeftAligh ? (rcControl.left + 10) : (rcControl.right - 10);
		int pty = bLeftAligh ? rcControl.bottom : (rcControl.top + rcControl.bottom) / 2;
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
	DWORD nRights = KEY_READ|WIN3264TEST((IsWindows64() ? KEY_WOW64_64KEY : 0),0);

	if (!RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Console\\TrueTypeFont",
	                  0, nRights, &hk))
	{
		wchar_t szId[32] = {0}, szFont[255]; DWORD dwLen, dwType; LONG iRc;

		if (gbIsDBCS)
		{
			DWORD idx = 0, cchName = countof(szId), dwLen = sizeof(szFont)-2;
			while ((iRc = RegEnumValue(hk, idx++, szId, &cchName, NULL, &dwType, (LPBYTE)szFont, &dwLen)) == 0)
			{
				szId[min(countof(szId)-1,cchName)] = 0; szFont[min(countof(szFont)-1,dwLen/2)] = 0;
				wchar_t* pszEnd;
				if (wcstoul(szId, &pszEnd, 10) && *szFont)
				{
					if (lstrcmpi((szFont[0] == L'*') ? (szFont+1) : szFont, asFaceName) == 0)
					{
						lbFound = true; break;
					}
				}
				cchName = countof(szId); dwLen = sizeof(szFont)-2;
			}
		}

		if (!lbFound)
		{
			for (DWORD i = 0; i < 20; i++)
			{
				szId[i] = L'0'; szId[i+1] = 0; wmemset(szFont, 0, 255);

				if (RegQueryValueExW(hk, szId, NULL, &dwType, (LPBYTE)szFont, &(dwLen = 255*2)))
					break;

				if (lstrcmpi(szFont, asFaceName) == 0)
				{
					lbFound = true; break;
				}
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
			        && (gbIsDBCS || IsAlmostMonospace(pszFamilyName, lpOutl->otmTextMetrics.tmMaxCharWidth, lpOutl->otmTextMetrics.tmAveCharWidth, lpOutl->otmTextMetrics.tmHeight))
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

	BOOL bCheckStarted = FALSE;
	DWORD nCheckResult = -1;
	DWORD nCheckWait = -1;

	if ((gpSetCls->nConFontError & ConFontErr_NonRegistry)
		|| (gbIsWine && gpSetCls->nConFontError))
	{
		wchar_t szCmd[MAX_PATH+64] = L"\"";
		wcscat_c(szCmd, gpConEmu->ms_ConEmuBaseDir);
		wchar_t* psz = szCmd + _tcslen(szCmd);
		_wcscpy_c(psz, 64, L"\\ConEmuC.exe");
		if (IsWindows64() && !FileExists(szCmd+1))
		{
			_wcscpy_c(psz, 64, L"\\ConEmuC64.exe");
		}
		wcscat_c(szCmd, L"\" /CheckUnicode");

		PROCESS_INFORMATION pi = {};
		STARTUPINFO si = {sizeof(si)};
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE; //RELEASEDEBUGTEST(SW_HIDE,SW_SHOW);

		#if 0
		AllocConsole();
		#endif

		bCheckStarted = CreateProcess(NULL, szCmd, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS|CREATE_DEFAULT_ERROR_MODE|CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
		if (bCheckStarted)
		{
			nCheckWait = WaitForSingleObject(pi.hProcess, 5000);
			if (nCheckWait == WAIT_OBJECT_0)
			{
				GetExitCodeProcess(pi.hProcess, &nCheckResult);

				if ((int)nCheckResult == CERR_UNICODE_CHK_OKAY)
				{
					gpSetCls->nConFontError = 0;
				}
			}
			else
			{
				TerminateProcess(pi.hProcess, 100);
			}

			#if 0
			wchar_t szDbg[1024];
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"Cmd:\n%s\nExitCode=%i", szCmd, nCheckResult);
			MBoxA(szDbg);
			FreeConsole();
			#endif
		}
	}

	bConsoleFontChecked = (gpSetCls->nConFontError == 0);
	return bConsoleFontChecked;
}

bool CSettings::CheckConsoleFont(HWND ahDlg)
{
	if (gbIsDBCS)
	{
		// В DBCS IsAlmostMonospace работает неправильно
		gpSetCls->CheckConsoleFontFast();

		// Заполнить список шрифтами из реестра
		HKEY hk;
		DWORD nRights = KEY_READ|WIN3264TEST((IsWindows64() ? KEY_WOW64_64KEY : 0),0);

		if (!RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Console\\TrueTypeFont",
						  0, nRights, &hk))
		{
			wchar_t szId[32] = {0}, szFont[255]; DWORD dwType; LONG iRc;

			DWORD idx = 0, cchName = countof(szId), dwLen = sizeof(szFont)-2;
			while ((iRc = RegEnumValue(hk, idx++, szId, &cchName, NULL, &dwType, (LPBYTE)szFont, &dwLen)) == 0)
			{
				szId[min(countof(szId)-1,cchName)] = 0; szFont[min(countof(szFont)-1,dwLen/2)] = 0;
				if (*szFont)
				{
					LPCWSTR pszFaceName = (szFont[0] == L'*') ? (szFont+1) : szFont;
					if (SendDlgItemMessage(ahDlg, tConsoleFontFace, CB_FINDSTRINGEXACT, -1, (LPARAM) pszFaceName)==-1)
					{
						int nIdx;
						nIdx = SendDlgItemMessage(ahDlg, tConsoleFontFace, CB_ADDSTRING, 0, (LPARAM) pszFaceName); //-V103
					}
				}
				cchName = countof(szId); dwLen = sizeof(szFont)-2;
			}
		}
	}
	else
	{
		gpSetCls->nConFontError = ConFontErr_NonSystem|ConFontErr_NonRegistry;

		bool bLoaded = false;

		if (ahDlg && (GetSystemMetrics(SM_DBCSENABLED) != 0))
		{
			// Chinese
			HKEY hk = NULL;
			DWORD nRights = KEY_READ|WIN3264TEST((IsWindows64() ? KEY_WOW64_64KEY : 0),0);
			if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Console\\TrueTypeFont", 0, nRights, &hk))
			{
				DWORD dwIndex = 0;
				wchar_t szName[255], szValue[255] = {};
				DWORD cchName = countof(szName), cbData = sizeof(szValue)-2, dwType;
				while (!RegEnumValue(hk, dwIndex++, szName, &cchName, NULL, &dwType, (LPBYTE)szValue, &cbData))
				{
            		if ((dwType == REG_DWORD) && *szName && *szValue)
            		{
            			SendDlgItemMessage(ahDlg, tConsoleFontFace, CB_ADDSTRING, 0, (LPARAM)szValue);
            			bLoaded = true;
            		}

					// Next
					cchName = countof(szName); cbData = sizeof(szValue);
					ZeroStruct(szValue);
				}
				RegCloseKey(hk);
			}
		}

		if (!bLoaded)
		{
			// Сначала загрузить имена шрифтов, установленных в систему
			HDC hdc = GetDC(NULL);
			EnumFontFamilies(hdc, (LPCTSTR) NULL, (FONTENUMPROC) EnumConFamCallBack, (LPARAM) ahDlg);
			DeleteDC(hdc);
		}

		// Проверить реестр
		if (CheckConsoleFontRegistry(gpSet->ConsoleFont.lfFaceName))
			gpSetCls->nConFontError &= ~(DWORD)ConFontErr_NonRegistry;
	}

	// Показать текущий шрифт
	if (ahDlg)
	{
		if (SelectString(ahDlg, tConsoleFontFace, gpSet->ConsoleFont.lfFaceName)<0)
			SetDlgItemText(ahDlg, tConsoleFontFace, gpSet->ConsoleFont.lfFaceName);
	}

	return (gpSetCls->nConFontError == 0);
}

INT_PTR CSettings::EditConsoleFontProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch (messg)
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

			if (gpConEmu->isVConExists(0))
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
						wcscpy_c(gpSet->ConsoleFont.lfFaceName, gpSetCls->sDefaultConFontName[0] ? gpSetCls->sDefaultConFontName : gsDefConFont);
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
					DWORD nRights = KEY_ALL_ACCESS|WIN3264TEST((IsWindows64() ? KEY_WOW64_64KEY : 0),0);

					if (!RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Console\\TrueTypeFont",
					                  0, nRights, &hk))
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
						sei.lpFile = WIN3264TEST(gpConEmu->ms_ConEmuC32Full,gpConEmu->ms_ConEmuC64Full);
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


void CSettings::FindTextDialog()
{
	if (mh_FindDlg && IsWindow(mh_FindDlg))
	{
		SetForegroundWindow(mh_FindDlg);
		return;
	}

	CVConGuard VCon;
	CRealConsole* pRCon = (CVConGroup::GetActiveVCon(&VCon) >= 0) ? VCon->RCon() : NULL;

	// Создаем диалог поиска только для консольных приложений
	if (!pRCon || (pRCon->GuiWnd() && !pRCon->isBufferHeight()) || !pRCon->GetView())
	{
		//DisplayLastError(L"No RealConsole, nothing to find");
		return;
	}

	gpConEmu->SkipOneAppsRelease(true);
	
	mh_FindDlg = CreateDialogParam((HINSTANCE)GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_FIND), ghWnd, findTextProc, NULL/*Param*/);
	if (!mh_FindDlg)
	{
		DisplayLastError(L"Can't create Find text dialog", GetLastError());
	}
}

void CSettings::UpdateFindDlgAlpha(bool abForce)
{
	if (!mh_FindDlg)
		return;

	int nAlpha = 255;

	if (gpSet->FindOptions.bTransparent)
	{
		POINT ptCur = {}; GetCursorPos(&ptCur);
		RECT rcWnd = {}; GetWindowRect(mh_FindDlg, &rcWnd);

		nAlpha = PtInRect(&rcWnd, ptCur) ? 254 : DEFAULT_FINDDLG_ALPHA;
	}

	static int nWasAlpha;
	if (!abForce && (nWasAlpha == nAlpha))
		return;

	nWasAlpha = nAlpha;
	gpConEmu->SetTransparent(mh_FindDlg, nAlpha);
}

INT_PTR CSettings::findTextProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch (messg)
	{
		case WM_INITDIALOG:
		{
			gpSetCls->mh_FindDlg = hWnd2;
			SendMessage(hWnd2, WM_SETICON, ICON_BIG, (LPARAM)hClassIcon);
			SendMessage(hWnd2, WM_SETICON, ICON_SMALL, (LPARAM)hClassIconSm);
			
			#if 0
			//if (IsDebuggerPresent())
			if (!gpSet->isAlwaysOnTop)
				SetWindowPos(hWnd2, HWND_NOTOPMOST, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE);
			#endif

			CVConGuard VCon;
			CRealConsole* pRCon = (CVConGroup::GetActiveVCon(&VCon) >= 0) ? VCon->RCon() : NULL;
			RECT rcWnd = {}; GetWindowRect(pRCon->GetView(), &rcWnd);
			SetWindowPos(hWnd2, gpSet->isAlwaysOnTop ? HWND_TOPMOST : HWND_TOP,
				rcWnd.left+gpSet->FontSizeY, rcWnd.top+gpSet->FontSizeY, 0,0,
				SWP_NOSIZE);
			gpSetCls->UpdateFindDlgAlpha(true);
			SetTimer(hWnd2, 101, 1000, NULL);

			SetClassLongPtr(hWnd2, GCLP_HICON, (LONG_PTR)hClassIcon);
			SetDlgItemText(hWnd2, tFindText, gpSet->FindOptions.pszText ? gpSet->FindOptions.pszText : L"");
			checkDlgButton(hWnd2, cbFindMatchCase, gpSet->FindOptions.bMatchCase);
			checkDlgButton(hWnd2, cbFindWholeWords, gpSet->FindOptions.bMatchWholeWords);
			checkDlgButton(hWnd2, cbFindFreezeConsole, gpSet->FindOptions.bFreezeConsole);
			#if 0
			checkDlgButton(hWnd2, cbFindHighlightAll, gpSet->FindOptions.bHighlightAll);
			#endif
			checkDlgButton(hWnd2, cbFindTransparent, gpSet->FindOptions.bTransparent);

			if (gpSet->FindOptions.pszText && *gpSet->FindOptions.pszText)
				SendDlgItemMessage(hWnd2, tFindText, EM_SETSEL, 0, lstrlen(gpSet->FindOptions.pszText));

			// Зовем всегда, чтобы инициализировать буфер для поиска как минимум
			gpConEmu->DoFindText(0);
			break;
		}

		//case WM_SYSCOMMAND:
		//	if (LOWORD(wParam) == ID_ALWAYSONTOP)
		//	{
		//		BOOL lbOnTopNow = GetWindowLong(ghOpWnd, GWL_EXSTYLE) & WS_EX_TOPMOST;
		//		SetWindowPos(ghOpWnd, lbOnTopNow ? HWND_NOTOPMOST : HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
		//		CheckMenuItem(GetSystemMenu(ghOpWnd, FALSE), ID_ALWAYSONTOP, MF_BYCOMMAND |
		//		              (lbOnTopNow ? MF_UNCHECKED : MF_CHECKED));
		//		SetWindowLongPtr(hWnd2, DWLP_MSGRESULT, 0);
		//		return 1;
		//	}
		//	break;

		case WM_MOUSEMOVE:
		case WM_NCMOUSEMOVE:
		case WM_TIMER:
			gpSetCls->UpdateFindDlgAlpha();
			break;

		case WM_COMMAND:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				int nDirection = 0;

				switch (LOWORD(wParam))
				{
				case IDCLOSE:
				case IDCANCEL:
					DestroyWindow(hWnd2);
					return 0;
				case cbFindMatchCase:
					gpSet->FindOptions.bMatchCase = IsChecked(hWnd2, cbFindMatchCase);
					break;
				case cbFindWholeWords:
					gpSet->FindOptions.bMatchWholeWords = IsChecked(hWnd2, cbFindWholeWords);
					break;
				case cbFindFreezeConsole:
					gpSet->FindOptions.bFreezeConsole = IsChecked(hWnd2, cbFindFreezeConsole);
					break;
				case cbFindHighlightAll:
					gpSet->FindOptions.bHighlightAll = IsChecked(hWnd2, cbFindHighlightAll);
					break;
				case cbFindTransparent:
					gpSet->FindOptions.bTransparent = IsChecked(hWnd2, cbFindTransparent);
					gpSetCls->UpdateFindDlgAlpha(true);
					return 0;
				case cbFindNext:
					nDirection = 1;
					break;
				case cbFindPrev:
					nDirection = -1;
					break;
				default:
					return 0;
				}

				if (gpSet->FindOptions.pszText && *gpSet->FindOptions.pszText)
					gpConEmu->DoFindText(nDirection);
			}
			else if (HIWORD(wParam) == EN_CHANGE || HIWORD(wParam) == CBN_EDITCHANGE || HIWORD(wParam) == CBN_SELCHANGE)
			{
				MyGetDlgItemText(hWnd2, tFindText, gpSet->FindOptions.cchTextMax, gpSet->FindOptions.pszText);
				if (gpSet->FindOptions.pszText && *gpSet->FindOptions.pszText)
					gpConEmu->DoFindText(0);
			}
			break;

		case WM_CLOSE:
			DestroyWindow(hWnd2);
			break;

		case WM_DESTROY:
			KillTimer(hWnd2, 101);
			gpSetCls->mh_FindDlg = NULL;
			gpSet->SaveFindOptions();
			gpConEmu->SkipOneAppsRelease(false);
			gpConEmu->DoEndFindText();
			break;

		default:
			return 0;
	}

	return 0;
}
