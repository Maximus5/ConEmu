﻿
/*
Copyright (c) 2009-2016 Maximus5
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
#include "version.h"

#include <commctrl.h>
#pragma warning(disable: 4091)
#include <shlobj.h>
#pragma warning(default: 4091)

#ifdef __GNUC__
#include "ShObjIdl_Part.h"
#endif // __GNUC__

#include "../common/Monitors.h"
#include "../common/MSetter.h"
#include "../common/StartupEnvDef.h"
#include "../common/WUser.h"
#include "../ConEmuCD/ExitCodes.h"
#include "../ConEmuCD/GuiHooks.h"
#include "../ConEmuPlugin/FarDefaultMacros.h"

#include "AboutDlg.h"
#include "Background.h"
#include "CmdHistory.h"
#include "ConEmu.h"
#include "ConEmuApp.h"
#include "ConEmuCtrl.h"
#include "DefaultTerm.h"
#include "DpiAware.h"
#include "DynDialog.h"
#include "FontMgr.h"
#include "HotkeyDlg.h"
#include "ImgButton.h"
#include "Inside.h"
#include "LngRc.h"
#include "LoadImg.h"
#include "Options.h"
#include "OptionsClass.h"
#include "OptionsFast.h"
#include "OptionsHelp.h"
#include "RealConsole.h"
#include "Recreate.h"
#include "SearchCtrl.h"
#include "SetCmdTask.h"
#include "SetColorPalette.h"
#include "SetDlgColors.h"
#include "SetDlgLists.h"
#include "SetPgAppear.h"
#include "SetPgApps.h"
#include "SetPgBackgr.h"
#include "SetPgBase.h"
#include "SetPgColors.h"
#include "SetPgComspec.h"
#include "SetPgConfirm.h"
#include "SetPgControls.h"
#include "SetPgCursor.h"
#include "SetPgDebug.h"
#include "SetPgDefTerm.h"
#include "SetPgEnvironment.h"
#include "SetPgFar.h"
#include "SetPgFarMacro.h"
#include "SetPgFeatures.h"
#include "SetPgFonts.h"
#include "SetPgHilight.h"
#include "SetPgInfo.h"
#include "SetPgIntegr.h"
#include "SetPgKeys.h"
#include "SetPgMarkCopy.h"
#include "SetPgPaste.h"
#include "SetPgSizePos.h"
#include "SetPgStartup.h"
#include "SetPgStatus.h"
#include "SetPgTabs.h"
#include "SetPgTaskbar.h"
#include "SetPgTasks.h"
#include "SetPgTransparent.h"
#include "SetPgUpdate.h"
#include "SetPgViews.h"
#include "Status.h"
#include "TabBar.h"
#include "TrayIcon.h"
#include "VConChild.h"
#include "VConGroup.h"
#include "VirtualConsole.h"

//#define CONEMU_ROOT_KEY L"Software\\ConEmu"

#define DEBUGSTRDPI(s) DEBUGSTR(s)

#define COUNTER_REFRESH 5000


#define FAILED_FONT_TIMEOUT 3000
#define FAILED_CONFONT_TIMEOUT 30000
#define FAILED_SELMOD_TIMEOUT 5000
#define CONTROL_FOUND_TIMEOUT 3000

//const WORD HostkeyCtrlIds[] = {cbHostWin, cbHostApps, cbHostLCtrl, cbHostRCtrl, cbHostLAlt, cbHostRAlt, cbHostLShift, cbHostRShift};
//const BYTE HostkeyVkIds[]   = {VK_LWIN,   VK_APPS,    VK_LCONTROL, VK_RCONTROL, VK_LMENU,   VK_RMENU,   VK_LSHIFT,    VK_RSHIFT};
//int upToFontHeight=0;
HWND ghOpWnd=NULL;

#ifdef _DEBUG
#define HEAPVAL HeapValidate(GetProcessHeap(), 0, NULL);
#else
#define HEAPVAL
#endif

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
	: mp_Dialog()
	, m_Pages()
	, mn_PagesCount()
	, mp_BgInfo()
	, m_ColorFormat(eRgbDec) // RRR GGG BBB (как показывать цвета на вкладке Colors)
	, mp_DpiAware()
	, mp_ImgBtn()
	, mn_LastChangingFontCtrlId()
	, mp_HelpPopup()
{
	// Prepare global pointers
	gpSetCls = this;
	gpSet = &m_Settings;

	GetOverallDpi();

	// Go
	isResetBasicSettings = false;
	isFastSetupDisabled = false;
	isDontCascade = false; // -nocascade or -dontcascade switch
	ibDisableSaveSettingsOnExit = false;
	m_ActivityLoggingType = glt_None; mn_ActivityCmdStartTick = 0;
	bForceBufferHeight = false; nForceBufferHeight = 1000; /* устанавливается в true, из ком.строки /BufferHeight */

	#ifdef SHOW_AUTOSCROLL
	AutoScroll = true;
	#endif

	isAllowDetach = 0;
	mb_ThemingEnabled = (gOSVer.dwMajorVersion >= 6 || (gOSVer.dwMajorVersion == 5 && gOSVer.dwMinorVersion >= 1));
	//isFullScreen = false;
	//ZeroStruct(m_QuakePrevSize);

	szSelectionModError[0] = 0;

	// Некоторые вещи нужно делать вне InitSettings, т.к. она может быть
	// вызвана потом из интерфейса диалога настроек
	wcscpy_c(ConfigPath, CONEMU_ROOT_KEY L"\\.Vanilla");
	ConfigName[0] = 0;

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

	SingleInstanceArg = sgl_Default;
	SingleInstanceShowHide = sih_None;
	gpSet->mb_CharSetWasSet = FALSE;
	mb_TabHotKeyRegistered = FALSE;
	//hMain = hExt = hFar = hTabs = hKeys = hColors = hCmdTasks = hViews = hInfo = hDebug = hUpdate = hSelection = NULL;
	m_LastActivePageId = thi_Last;
	hwndTip = NULL; hwndBalloon = NULL;
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
	memset(mn_Counter, 0, sizeof(mn_Counter));
	memset(mn_CounterMax, 0, sizeof(mn_CounterMax));
	memset(mn_FPS, 0, sizeof(mn_FPS)); mn_FPS_CUR_FRAME = 0;
	memset(mn_RFPS, 0, sizeof(mn_RFPS)); mn_RFPS_CUR_FRAME = 0;
	memset(mn_CounterTick, 0, sizeof(mn_CounterTick));
	memset(mn_KbdDelays, 0, sizeof(mn_KbdDelays)); mn_KbdDelayCounter = 0; mn_KBD_CUR_FRAME = -1;
	//hBgBitmap = NULL; bgBmp = MakeCoord(0,0); hBgDc = NULL;
	#ifndef APPDISTINCTBACKGROUND
	mb_BgLastFade = false;
	mp_Bg = NULL;
	mp_BgImgData = NULL;
	isBackgroundImageValid = false;
	mb_NeedBgUpdate = FALSE; //mb_WasVConBgImage = FALSE;
	ftBgModified.dwHighDateTime = ftBgModified.dwLowDateTime = nBgModifiedTick = 0;
	#else
	#endif
#if 0
	//120714 - аналогичные параметры работают в ConEmuC.exe, а в GUI они и не работали. убрал пока
	nAttachPID = 0; hAttachConWnd = NULL;
#endif
	nConFontError = 0;
	memset(&tiBalloon, 0, sizeof(tiBalloon));
	//mn_FadeMul = gpSet->mn_FadeHigh - gpSet->mn_FadeLow;
	//gpSet->mn_LastFadeSrc = gpSet->mn_LastFadeDst = -1;

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

	mn_MsgUpdateCounter = RegisterWindowMessage(L"ConEmuSettings::Counter"); mb_MsgUpdateCounter = FALSE;
	mn_MsgRecreateFont = RegisterWindowMessage(L"Settings::RecreateFont");
	mn_MsgLoadFontFromMain = RegisterWindowMessage(L"Settings::LoadFontNames");
	mn_ActivateTabMsg = RegisterWindowMessage(L"Settings::ActivateTab");

	mp_HelpPopup = new CEHelpPopup;

	// Settings pages definitions
	InitVars_Pages();

	// Hotkeys list
	CSetPgKeys::ReInitHotkeys();

	memset(&tiConFontBalloon, 0, sizeof(tiConFontBalloon));
	mb_IgnoreSelPage = false;
}

int CSettings::GetOverallDpi()
{
	// Must be called during initialization only
	CDpiAware::QueryDpiForMonitor(NULL, &_dpi_all);
	_ASSERTE(_dpi_all.Xdpi >= 96 && _dpi_all.Ydpi >= 96);
	_dpi.SetDpi(_dpi_all);
	return _dpi.Ydpi;
}

// This returns current dpi for main window
int CSettings::QueryDpi()
{
	return _dpi.Ydpi;
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

		CVConGuard VCon;
		for (int i = 0; CVConGroup::GetVCon(i, &VCon, true); i++)
		{
			nNewValue |= VCon->RCon()->GetConsoleKeyShortcuts();
		}

		*pnConsoleKeyShortcuts = nNewValue;
	}

	// __declspec(dllexport) DWORD gnHookedKeys[HookedKeysMaxCount] = {};
	DWORD *pnHookedKeys = (DWORD*)GetProcAddress(hLLKeyHookDll, "gnHookedKeys");
	if (pnHookedKeys)
	{
		DWORD *pn = pnHookedKeys;

		//WARNING("CConEmuCtrl:: Тут вообще наверное нужно по всем HotKeys проехаться, а не только по «избранным»");
		for (int i = 0;; i++)
		{
			const ConEmuHotKey* pHK = gpHotKeys->GetHotKeyPtr(i);
			if (!pHK)
				break;

			// chk_Local may fails to be registered by RegisterHotKey
			// so it's better to try both ways if possible
			if ((pHK->HkType == chk_Modifier) || (pHK->HkType == chk_Global))
				continue;

			DWORD VkMod = pHK->GetVkMod();

			if (!ConEmuHotKey::HasModifier(VkMod, VK_LWIN))
				continue;

			if (pHK->Enabled && !pHK->Enabled())
				continue;

			if (pHK->DontWinHook && pHK->DontWinHook(pHK))
				continue;

			DWORD nFlags = ConEmuHotKey::GetHotkey(VkMod);
			for (int i = 1; i <= 3; i++)
			{
				switch (ConEmuHotKey::GetModifier(VkMod, i))
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

		*pn = 0;
		_ASSERTE((pn - pnHookedKeys) < (HookedKeysMaxCount-1));
	}
}

void CSettings::InitVars_Pages()
{
	ConEmuSetupPages Pages[] =
	{
		// При добавлении вкладки нужно добавить OnInitDialog_XXX в pageOpProc
		{IDD_SPG_FONTS,       0, lng_SpgFonts,        thi_Fonts,        CSetPgFonts::Create},
		{IDD_SPG_SIZEPOS,     1, lng_SpgSizePos,      thi_SizePos,      CSetPgSizePos::Create},
		{IDD_SPG_APPEAR,      1, lng_SpgAppear,       thi_Appear,       CSetPgAppear::Create},
		{IDD_SPG_BACKGR,      1, lng_SpgBackgr,       thi_Backgr,       CSetPgBackgr::Create},
		{IDD_SPG_TABS,        1, lng_SpgTabBar,       thi_Tabs,         CSetPgTabs::Create},
		{IDD_SPG_CONFIRM,     1, lng_SpgConfirm,      thi_Confirm,      CSetPgConfirm::Create},
		{IDD_SPG_TASKBAR,     1, lng_SpgTaskBar,      thi_Taskbar,      CSetPgTaskbar::Create},
		{IDD_SPG_UPDATE,      1, lng_SpgUpdate,       thi_Update,       CSetPgUpdate::Create},
		{IDD_SPG_STARTUP,     0, lng_SpgStartup,      thi_Startup,      CSetPgStartup::Create},
		{IDD_SPG_TASKS,       1, lng_SpgTasks,        thi_Tasks,        CSetPgTasks::Create},
		{IDD_SPG_ENVIRONMENT, 1, lng_SpgEnvironment,  thi_Environment,  CSetPgEnvironment::Create},
		{IDD_SPG_FEATURES,    0, lng_SpgFeatures,     thi_Features,     CSetPgFeatures::Create},
		{IDD_SPG_CURSOR,      1, lng_SpgCursor,       thi_Cursor,       CSetPgCursor::Create},
		{IDD_SPG_COLORS,      1, lng_SpgColors,       thi_Colors,       CSetPgColors::Create},
		{IDD_SPG_TRANSPARENT, 1, lng_SpgTransparent,  thi_Transparent,  CSetPgTransparent::Create},
		{IDD_SPG_STATUSBAR,   1, lng_SpgStatusBar,    thi_Status,       CSetPgStatus::Create},
		{IDD_SPG_APPDISTINCT, 1, lng_SpgAppDistinct,  thi_Apps,         CSetPgApps::Create},
		{IDD_SPG_INTEGRATION, 0, lng_SpgIntegration,  thi_Integr,       CSetPgIntegr::Create},
		{IDD_SPG_DEFTERM,     1, lng_SpgDefTerm,      thi_DefTerm,      CSetPgDefTerm::Create},
		{IDD_SPG_COMSPEC,     1, lng_SpgComSpec,      thi_Comspec,      CSetPgComspec::Create},
		{IDD_SPG_KEYS,        0, lng_SpgKeys,         thi_Keys,         CSetPgKeys::Create},
		{IDD_SPG_CONTROLS,    1, lng_SpgControls,     thi_Controls,     CSetPgControls::Create},
		{IDD_SPG_MARKCOPY,    1, lng_SpgMarkCopy,     thi_MarkCopy,     CSetPgMarkCopy::Create},
		{IDD_SPG_PASTE,       1, lng_SpgPaste,        thi_Paste,        CSetPgPaste::Create},
		{IDD_SPG_HIGHLIGHT,   1, lng_SpgHighlight,    thi_Hilight,      CSetPgHilight::Create},
		{IDD_SPG_FEATURE_FAR, 0, lng_SpgFarManager,   thi_Far,          CSetPgFar::Create, true/*Collapsed*/},
		{IDD_SPG_FARMACRO,    1, lng_SpgFarMacros,    thi_FarMacro,     CSetPgFarMacro::Create},
		{IDD_SPG_VIEWS,       1, lng_SpgFarViews,     thi_Views,        CSetPgViews::Create},
		{IDD_SPG_INFO,        0, lng_SpgInfo,         thi_Info,         CSetPgInfo::Create, RELEASEDEBUGTEST(true,false)/*Collapsed in Release*/},
		{IDD_SPG_DEBUG,       1, lng_SpgDebug,        thi_Debug,        CSetPgDebug::Create},
		// End
		{},
	};

	#ifdef _DEBUG
	for (size_t i = 0; Pages[i].DialogID; i++)
	{
		_ASSERTE(Pages[i].PageIndex == (TabHwndIndex)i);
	}
	#endif

	mn_PagesCount = ((int)countof(Pages) - 1);
	_ASSERTE(mn_PagesCount>0 && Pages[mn_PagesCount].DialogID==0);
	_ASSERTE(mn_PagesCount == thi_Last);

	m_Pages = (ConEmuSetupPages*)malloc(sizeof(Pages));
	memmove(m_Pages, Pages, sizeof(Pages));
}

void CSettings::InitPageNames()
{
	if (!m_Pages)
	{
		_ASSERTE(m_Pages && (mn_PagesCount > 0));
		return;
	}

	for (int i = 0; i < mn_PagesCount; i++)
	{
		lstrcpyn(m_Pages[i].PageName, CLngRc::getRsrc(m_Pages[i].PageNameRsrc), countof(m_Pages[i].PageName));
	}
}

void CSettings::ReleaseHandles()
{
	//if (sTabCloseMacro) {free(sTabCloseMacro); sTabCloseMacro = NULL;}
	//if (sSafeFarCloseMacro) {free(sSafeFarCloseMacro); sSafeFarCloseMacro = NULL;}
	//if (sSaveAllMacro) {free(sSaveAllMacro); sSaveAllMacro = NULL;}
	//if (sRClickMacro) {free(sRClickMacro); sRClickMacro = NULL;}

	#ifndef APPDISTINCTBACKGROUND
	SafeDelete(mp_Bg);
	SafeFree(mp_BgImgData);
	#else
	SafeRelease(mp_BgInfo);
	#endif
}

CSettings::~CSettings()
{
	ReleaseHandles();

	//if (gpSet->psCmd) {free(gpSet->psCmd); gpSet->psCmd = NULL;}

	//if (gpSet->psCmdHistory) {free(gpSet->psCmdHistory); gpSet->psCmdHistory = NULL;}

	//if (gpSet->psCurCmd) {free(gpSet->psCurCmd); gpSet->psCurCmd = NULL;}

#if 0
	if (mh_Uxtheme!=NULL) { FreeLibrary(mh_Uxtheme); mh_Uxtheme = NULL; }
#endif

	CSetDlgColors::ReleaseHandles();

	//if (gpSet)
	//{
	//	delete gpSet;
	//	gpSet = NULL;
	//}

	gpHotKeys->ReleaseHotkeys();

	SafeFree(m_Pages);

	SafeDelete(mp_HelpPopup);
	SafeDelete(mp_Dialog);
	SafeDelete(mp_DpiAware);
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
	SetEnvironmentVariable(ENV_CONEMU_CONFIG_W, ConfigName);
}

bool CSettings::SetOption(LPCWSTR asName, int anValue)
{
	bool lbRc = true;

	if (!lstrcmpi(asName, L"CursorBlink"))
	{
		gpSet->AppStd.CursorActive.isBlinking = (anValue != 0);
		HWND hCursorPg = GetPage(thi_Cursor);
		if (hCursorPg)
			checkDlgButton(hCursorPg, cbCursorBlink, (anValue != 0));
		// Может выполняться в фоновом потоке!
		// При таком (ниже) запуске после старта курсор не отрисовывается в промпте
		// -basic -cmd cmd /k conemuc -guimacro status 0 2 -guimacro setoption cursorblink 0 & dir
	}
	else
	{
		lbRc = false;
	}

	return lbRc;
}

bool CSettings::SetOption(LPCWSTR asName, LPCWSTR asValue)
{
	bool lbRc = false;

	if (!lstrcmpi(asName, L"FarGotoEditorPath"))
	{
		wchar_t* pszNewVal = lstrdup(asValue);
		if (pszNewVal && *pszNewVal)
		{
			wchar_t* pszOld = (wchar_t*)InterlockedExchangePointer((PVOID*)&gpSet->sFarGotoEditor, pszNewVal);
			SafeFree(pszOld);
			HWND hHilightPg = GetPage(thi_Hilight);
			if (hHilightPg)
				SetDlgItemText(hHilightPg, lbGotoEditorCmd, gpSet->sFarGotoEditor);
			lbRc = true;
		}
	}
	else
	{
		_ASSERTE(FALSE && "Unsupported parameter name");
	}

	return lbRc;
}

void CSettings::SettingsLoaded(SettingsLoadedFlags slfFlags, LPCWSTR pszCmdLine /*= NULL*/)
{
	// Logging may be enabled in Settings permanently
	if (gpSet->isLogging())
	{
		gpConEmu->CreateLog();
	}

	_ASSERTE(gpLng != NULL);
	if (gpLng)
		gpLng->Reload();

	gpSet->PatchSizeSettings();

	if ((ghWnd == NULL) || (slfFlags & slf_OnResetReload))
	{
		gpConEmu->wndX = gpSet->_wndX;
		gpConEmu->wndY = gpSet->_wndY;
		gpConEmu->WndWidth.Raw = gpSet->wndWidth.Raw;
		gpConEmu->WndHeight.Raw = gpSet->wndHeight.Raw;
	}

	// Recheck dpi settings?
	if (ghWnd == NULL)
	{
		gpConEmu->GetInitialDpi(&_dpi);
		wchar_t szInfo[100];
		_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"DPI initialized to {%i,%i}\r\n", _dpi.Xdpi, _dpi.Ydpi);
		DEBUGSTRDPI(szInfo);
		LogString(szInfo, true, false);
	}

	#if 0
	// Не требуется, размер будет скорректирован в CreateMainWindow()
	if (slfFlags & (slf_NeedCreateVanilla|slf_DefaultSettings) && (gpSet->FontSizeY > 1))
	{
		// Under debug the default console width is set to 110x35,
		// that may be too large on small displays
		MONITORINFO mi = {};
		if (gpConEmu->FindInitialMonitor(&mi))
		{
			// Примерно, лишь бы сильно за рабочую область не вылезало...
			RECT rcCon = gpConEmu->CalcRect(CER_CONSOLE_ALL, mi.rcWork, CER_MONITOR);
			//RECT rcDC = gpConEmu->CalcRect(CER_BACK, mi.rcWork, CER_MONITOR);
			//RECT rcCon = MakeRect((rcDC.right - rcDC.left) / (gpSet->FontSizeX ? gpSet->FontSizeX : (gpSet->FontSizeY / 2)), (rcDC.bottom - rcDC.top) / gpSet->FontSizeY);
			if ((rcCon.right > 0) && (rcCon.bottom > 0)
				&& ((rcCon.right < gpSet->wndWidth.Value) || (rcCon.bottom < gpSet->wndHeight.Value)))
			{
				gpSet->wndWidth.Set(true, ss_Standard, min(gpSet->wndWidth.Value, rcCon.right));
				gpSet->wndHeight.Set(false, ss_Standard, min(gpSet->wndHeight.Value, rcCon.bottom));
				gpConEmu->WndWidth.Raw = gpSet->wndWidth.Raw;
				gpConEmu->WndHeight.Raw = gpSet->wndHeight.Raw;
			}
		}
	}
	#endif

	// Обработать 32/64 (найти tcc.exe и т.п.)
	FindComspec(&gpSet->ComSpec);

	// Зовем "FastConfiguration" (перед созданием новой/чистой конфигурации)
	if (slfFlags & slf_AllowFastConfig)
	{
		LPCWSTR pszDef = gpConEmu->GetDefaultTitle();
		//wchar_t szType[8];
		bool ReadOnly = false;
		SettingsStorage Storage = {};
		gpSet->GetSettingsType(Storage, ReadOnly);
		LPCWSTR pszConfig = gpSetCls->GetConfigName();

		CEStr szTitle;
		LPCWSTR pszFastCfgTitle = CLngRc::getRsrc(lng_DlgFastCfg/*"fast configuration"*/);
		if (pszConfig && *pszConfig)
			szTitle = lstrmerge(pszDef, L" ", pszFastCfgTitle, L" (", pszConfig, L")");
		else
			szTitle = lstrmerge(pszDef, L" ", pszFastCfgTitle);

		// Run "Fast configuration dialog" and apply some final defaults (if was Reset of new settings)
		CheckOptionsFast(szTitle, slfFlags);

		// Single instance?
		if (gpSet->isSingleInstance && (gpSetCls->SingleInstanceArg == sgl_Default))
		{
			if ((pszCmdLine && *pszCmdLine) || (gpConEmu->m_StartDetached == crb_On))
			{
				// Должен быть "sih_None" иначе существующая копия не запустит команду
				_ASSERTE(SingleInstanceShowHide == sih_None);
			}
			else
			{
				SingleInstanceShowHide = sih_ShowMinimize;
			}
		}
	}


	if (slfFlags & slf_NeedCreateVanilla)
	{
		if (!gpConEmu->IsResetBasicSettings())
		{
			gpSet->SaveSettings(TRUE/*abSilent*/);
		}
		else
		{
			// Must not be here when using "/basic" switch!
			_ASSERTE(!gpConEmu->IsResetBasicSettings());
		}
	}

	// Issue 1191: ConEmu was launched instead of explorer from taskbar pinned library icon
	bool bHasLibraries = IsWindows7;
	if (bHasLibraries)
	{
		CSetPgIntegr::UnregisterShellInvalids();
	}

	// Передернуть палитру затенения
	gpSet->ResetFadeColors();
	gpSet->GetColors(-1, TRUE);


	// Назначить корректные флаги для кнопок Win+Number и Win+Arrow
	gpHotKeys->UpdateNumberModifier();
	gpHotKeys->UpdateArrowModifier();


	// Проверить необходимость установки хуков
	//-- isKeyboardHooks();
	// При первом запуске - проверить, хотят ли включить автообновление?
	//-- CheckUpdatesWanted();



	// Стили окна
	if ((gpSet->_WindowMode != wmNormal)
		&& (gpSet->_WindowMode != wmMaximized)
		&& (gpSet->_WindowMode != wmFullScreen))
	{
		// Иначе окно вообще не отображается
		_ASSERTE(gpSet->_WindowMode!=0);
		gpSet->_WindowMode = wmNormal;
	}

	//if (rcTabMargins.top > 100) rcTabMargins.top = 100;
	//_ASSERTE((rcTabMargins.bottom == 0 || rcTabMargins.top == 0) && !rcTabMargins.left && !rcTabMargins.right);
	//rcTabMargins.left = rcTabMargins.right = 0;
	//int nTabHeight = rcTabMargins.top ? rcTabMargins.top : rcTabMargins.bottom;
	//if (nTabsLocation == 1)
	//{
	//	rcTabMargins.top = 0; rcTabMargins.bottom = nTabHeight;
	//}
	//else
	//{
	//	rcTabMargins.top = nTabHeight; rcTabMargins.bottom = 0;
	//}

	if (!gpSet->pHistory)
	{
		gpSet->pHistory = new CommandHistory(MAX_CMD_HISTORY);
	}

	MCHKHEAP

	if ((SingleInstanceArg == sgl_Default) && gpSet->isQuakeStyle)
	{
		_ASSERTE((SingleInstanceShowHide == sih_None) || (gpSet->isSingleInstance && (SingleInstanceShowHide == sih_ShowMinimize)));
		SingleInstanceArg = sgl_Enabled;
	}

	//wcscpy_c(gpSet->ComSpec.ConEmuDir, gpConEmu->ms_ConEmuDir);
	gpConEmu->InitComSpecStr(gpSet->ComSpec);
	// -- должно вообще вызываться в UpdateGuiInfoMapping
	//UpdateComspec(&gpSet->ComSpec);

	// Init custom palette for ColorSelection dialog
	CSetDlgColors::OnSettingsLoaded(gpSet->Colors);

	gpFontMgr->SettingsLoaded(slfFlags);

	// Стили окна
	// Т.к. вызывается из Settings::LoadSettings() то проверка на валидность уже не нужно, оставим только ассерт
	_ASSERTE(gpSet->_WindowMode == wmNormal || gpSet->_WindowMode == wmMaximized || gpSet->_WindowMode == wmFullScreen);

	if (ghWnd == NULL)
	{
		gpConEmu->WindowMode = (ConEmuWindowMode)gpSet->_WindowMode;
	}
	else
	{
		gpConEmu->SetWindowMode((ConEmuWindowMode)gpSet->_WindowMode);
	}

	if (!gpConEmu->InCreateWindow())
	{
		gpConEmu->OnGlobalSettingsChanged();
	}

	MCHKHEAP
}

void CSettings::SettingsPreSave()
{
	// Do not get data from LogFont if it was not created yet
	if (gpConEmu->mn_StartupFinished >= CConEmuMain::ss_Started)
	{
		gpFontMgr->SettingsPreSave();
	}

	// Макросы, совпадающие с "умолчательными" - не пишем
	if (gpSet->sRClickMacro && (lstrcmp(gpSet->sRClickMacro, gpSet->RClickMacroDefault(fmv_Default)) == 0 || lstrcmp(gpSet->sRClickMacro, gpSet->RClickMacroDefault(fmv_Lua)) == 0))
		SafeFree(gpSet->sRClickMacro);
	if (gpSet->sSafeFarCloseMacro && (lstrcmp(gpSet->sSafeFarCloseMacro, gpSet->SafeFarCloseMacroDefault(fmv_Default)) == 0 || lstrcmp(gpSet->sSafeFarCloseMacro, gpSet->SafeFarCloseMacroDefault(fmv_Lua)) == 0))
		SafeFree(gpSet->sSafeFarCloseMacro);
	if (gpSet->sTabCloseMacro && (lstrcmp(gpSet->sTabCloseMacro, gpSet->TabCloseMacroDefault(fmv_Default)) == 0 || lstrcmp(gpSet->sTabCloseMacro, gpSet->TabCloseMacroDefault(fmv_Lua)) == 0))
		SafeFree(gpSet->sTabCloseMacro);
	if (gpSet->sSaveAllMacro && (lstrcmp(gpSet->sSaveAllMacro, gpSet->SaveAllMacroDefault(fmv_Default)) == 0 || lstrcmp(gpSet->sSaveAllMacro, gpSet->SaveAllMacroDefault(fmv_Lua)) == 0))
		SafeFree(gpSet->sSaveAllMacro);
}



// Помножить размер на масштаб * dpi * юниты(-1)
LONG CSettings::EvalSize(LONG nSize, EvalSizeFlags Flags)
{
	if (nSize <= 0)
	{
		// Must not be used for "units"!
		// This must be used for evaluate "pixels" or sort of.
		// Positive values only...
		_ASSERTE(nSize >= 0);
		return 0;
	}

	LONG iMul = 1, iDiv = 1, iResult;

	// DPI текущего(!) монитора
	if ((Flags & esf_CanUseDpi) && gpSet->FontUseDpi)
	{
		int iDpi = (Flags & esf_Horizontal) ? _dpi.Xdpi : _dpi.Ydpi;
		if (iDpi > 0)
		{
			iMul *= iDpi;
			iDiv *= 96;
		}
		else
		{
			_ASSERTE(iDpi > 0);
		}
	}

	// Zooming, current, is not stored in the settings
	if (Flags & esf_CanUseZoom)
	{
		if (gpFontMgr->mn_FontZoomValue && (gpFontMgr->mn_FontZoomValue != CFontMgr::FontZoom100))
		{
			iMul *= gpFontMgr->mn_FontZoomValue;
			iDiv *= CFontMgr::FontZoom100;
		}
	}

	// Сейчас множитель-делитель должны быть >0
	_ASSERTE(iMul>0 && iDiv>0);

	// Units (char height) or pixels (cell height)?
	if ((Flags & esf_CanUseUnits) && (Flags & esf_Vertical)
		&& gpSet->FontUseUnits && (nSize > 0))
	{
		iMul = -iMul;
	}

	iResult = MulDiv(nSize, iMul, iDiv);
	return iResult;
}




// Find setting by typed name in the "Search" box
void CSettings::SearchForControls()
{
	HWND hSearchEdit = GetDlgItem(ghOpWnd, tOptionSearch);
	wchar_t* pszPart = GetDlgItemTextPtr(hSearchEdit, 0);
	if (!pszPart || !*pszPart)
	{
		SafeFree(pszPart);
		return;
	}

	TOOLINFO toolInfo = { 44, TTF_IDISHWND, ghOpWnd, (UINT_PTR)hSearchEdit }; //sizeof(toolInfo); -- need to work on Win2k and compile with Vista+
	SendMessage(hwndTip, TTM_DELTOOL, 0, (LPARAM)&toolInfo);

	size_t i, s, iTab, iCurTab;
	INT_PTR lFind = -1;
	HWND hSelTab = NULL, hCurTab = NULL, hCtrl = NULL;
	static HWND hLastTab, hLastCtrl;
	INT_PTR lLastListFind = -1;
	wchar_t szText[255] = L"", szClass[80];
	static wchar_t szLastText[255];

	#define ResetLastList() { lLastListFind = -1; szLastText[0] = 0; }
	#define ResetLastCtrl() { hLastCtrl = NULL; ResetLastList(); }

	SetCursor(LoadCursor(NULL,IDC_WAIT));

	for (i = 0; m_Pages[i].DialogID; i++)
	{
		if (m_Pages[i].hPage && IsWindowVisible(m_Pages[i].hPage))
		{
			hSelTab = m_Pages[i].hPage;
			iTab = i;
			break;
		}
	}

	if (!hSelTab)
	{
		_ASSERTE(hSelTab!=NULL);
		goto wrap;
	}

	if (hLastTab != hSelTab)
	{
		ResetLastCtrl();
	}

	for (s = 0; s <= 2; s++)
	{
		size_t iFrom = iTab, iTo = iTab + 1;

		if (s)
		{
			ResetLastCtrl();
		}

		if (s == 1)
		{
			iFrom = iTab+1; iTo = thi_Last;
		}
		else if (s == 2)
		{
			iFrom = 0; iTo = iTab;
		}

		for (i = iFrom; (i < iTo) && m_Pages[i].DialogID; i++)
		{
			if (m_Pages[i].hPage == NULL)
			{
				CSetPgBase::CreatePage(&(m_Pages[i]), ghOpWnd, mn_ActivateTabMsg, mp_DpiAware);
			}

			iCurTab = i;
			hCurTab = m_Pages[i].hPage;

			if (hCurTab == NULL)
			{
				_ASSERTE(hCurTab != NULL);
				continue;
			}

			_ASSERTE(hCtrl==NULL);

			if (hCurTab == hLastTab)
			{
				if (hLastCtrl && GetClassName(hLastCtrl, szClass, countof(szClass)))
				{
					if (lstrcmpi(szClass, L"ListBox") == 0)
					{
						INT_PTR iCount = SendMessage(hLastCtrl, LB_GETCOUNT, 0, 0);
						lLastListFind = SendMessage(hLastCtrl, LB_GETCURSEL, 0, 0);
						if ((iCount > 0) && (lLastListFind + 1) < iCount)
							hCtrl = hLastCtrl;
					}
					else if (lstrcmpi(szClass, L"SysListView32") == 0)
					{
						INT_PTR iCount = ListView_GetItemCount(hLastCtrl);
						lLastListFind = ListView_GetNextItem(hLastCtrl, -1, LVNI_SELECTED);
						if ((iCount > 0) && (lLastListFind + 1) < iCount)
							hCtrl = hLastCtrl;
					}
					else
					{
						ResetLastList();
					}
				}
				else
				{
					ResetLastCtrl();
				}
			}

			// If next item not available in the list
			if (!hCtrl)
			{
				hCtrl = FindWindowEx(hCurTab, hLastCtrl, NULL, NULL);
			}

			if (hCtrl == hLastCtrl)
				wcscpy_c(szText, szLastText);
			else if (!hCtrl || (hCtrl != hLastCtrl))
				lLastListFind = -1;

			while (hCtrl != NULL)
			{
				if ((GetWindowLong(hCtrl, GWL_STYLE) & WS_VISIBLE)
					&& GetClassName(hCtrl, szClass, countof(szClass)))
				{
					if (lstrcmpi(szClass, L"ListBox") == 0)
					{
						lFind = SendMessage(hCtrl, LB_FINDSTRING, lLastListFind, (LPARAM)pszPart);
						// LB_FINDSTRING search from begin of string, but may be "In string"?
						if (lFind < 0)
						{
							INT_PTR iCount = SendMessage(hCtrl, LB_GETCOUNT, 0, 0);
							for (INT_PTR i = lLastListFind+1; i < iCount; i++)
							{
								INT_PTR iLen = SendMessage(hCtrl, LB_GETTEXTLEN, i, 0);
								if (iLen >= (INT_PTR)countof(szText))
								{
									_ASSERTE(iLen < countof(szText));
								}
								else
								{
									SendMessage(hCtrl, LB_GETTEXT, i, (LPARAM)szText);
									if (StrStrI(szText, pszPart) != NULL)
									{
										lFind = i;
										break;
									}
								}
							}
						}

						if ((lFind >= 0) && (lFind > lLastListFind))
						{
							lLastListFind = lFind;
							SendMessage(hCtrl, LB_SETCURSEL, lFind, 0);
							INT_PTR iLen = SendMessage(hCtrl, LB_GETTEXTLEN, lFind, 0);
							if (iLen >= (INT_PTR)countof(szText))
							{
								_ASSERTE(iLen < countof(szText));
								lstrcpyn(szText, pszPart, countof(szText));
							}
							else
							{
								SendMessage(hCtrl, LB_GETTEXT, lFind, (LPARAM)szText);
							}
							break; // Found
						}
					} // End of "ListBox" processing
					else if (lstrcmpi(szClass, L"SysListView32") == 0)
					{
						LVITEM lvi = {LVIF_TEXT|LVIF_STATE|LVIF_PARAM};
						lvi.pszText = szText;
						INT_PTR iCount = ListView_GetItemCount(hCtrl);
						lFind = -1;

						for (INT_PTR i = lLastListFind+1; (i < iCount) && (lFind == -1); i++)
						{
							for (lvi.iSubItem = 0; lvi.iSubItem <= 32; lvi.iSubItem++)
							{
								lvi.cchTextMax = countof(szText);
								if (SendMessage(hCtrl, LVM_GETITEMTEXT, i, (LPARAM)&lvi) > 0)
								{
									if (StrStrI(szText, pszPart) != NULL)
									{
										lFind = i;
										break;
									}
								}
							}
						}

						if ((lFind >= 0) && (lFind > lLastListFind))
						{
							lLastListFind = lFind;
							ListView_SetItemState(hCtrl, lFind, LVIS_SELECTED, LVIS_SELECTED);
							ListView_SetSelectionMark(hCtrl, lFind);
							ListView_EnsureVisible(hCtrl, lFind, FALSE);
							break; // Found
						}
					} // End of "SysListView32" processing
					else if (((lstrcmpi(szClass, L"Button") == 0)
								|| (lstrcmpi(szClass, L"Static") == 0))
						&& GetWindowText(hCtrl, szText, countof(szText)) && *szText)
					{
						// The control's text may has (&) accelerator confusing the search
						wchar_t* p = wcschr(szText, L'&');
						while (p)
						{
							wmemmove(p, p+1, wcslen(p));
							p = wcschr(p+1, L'&');
						}

						if (StrStrI(szText, pszPart) != NULL)
							break; // Found
					} // End of "Button" and "Static" processing
					else if ((lstrcmpi(szClass, L"Edit") == 0)
						&& GetWindowText(hCtrl, szText, countof(szText)) && *szText)
					{
						// Process "Edit" fields too (search for user-typed string)
						if (StrStrI(szText, pszPart) != NULL)
							break; // Found
					} // End of "Edit" processing
				}

				hCtrl = FindWindowEx(hCurTab, hCtrl, NULL, NULL);
			}

			if (hCtrl)
				break;
		}

		if (hCtrl)
			break;
	}

	if (hCtrl != NULL)
	{
		// Активировать нужный таб
		if (hSelTab != hCurTab)
		{
			SelectTreeItem(GetDlgItem(ghOpWnd, tvSetupCategories), gpSetCls->m_Pages[iCurTab].hTI, true);
			//SendMessage(hCurTab, mn_ActivateTabMsg, 1, (LPARAM)&(m_Pages[i]));
			//ShowWindow(hCurTab, SW_SHOW);
			//ShowWindow(hSelTab, SW_HIDE);
		}

		// Показать хинт?
		static wchar_t szHint[2000];

		LONG wID = GetWindowLong(hCtrl, GWL_ID);

		if ((wID == -1) && (lstrcmpi(szClass, L"Static") == 0))
		{
			// If it is STATIC with IDC_STATIC - get next ctrl (edit/combo/so on)
			wID = GetWindowLong(FindWindowEx(hCurTab, hCtrl, NULL, NULL), GWL_ID);
		}

		szHint[0] = 0;

		if (wID != -1)
		{
			// Is there hint for this control?
			if (!CLngRc::getHint(wID, szHint, countof(szHint)))
				szHint[0] = 0;
		}

		if (!*szHint)
		{
			// Show text of original control.
			// szText contains text from previous label ("Static")
			wcscpy_c(szHint, szText);
		}

		wcscpy_c(szLastText, szText);

		HWND hBall = gpSetCls->hwndBalloon;
		TOOLINFO *pti = &gpSetCls->tiBalloon;

		pti->lpszText = szHint;

		if (gpSetCls->hwndTip) SendMessage(gpSetCls->hwndTip, TTM_ACTIVATE, FALSE, 0);

		SendMessage(hBall, TTM_UPDATETIPTEXT, 0, (LPARAM)pti);
		RECT rcControl; GetWindowRect(hCtrl, &rcControl);
		int ptx = /*bLeftAligh ?*/ (rcControl.left + 10) /*: (rcControl.right - 10)*/;
		int pty = rcControl.top + 10; //bLeftAligh ? rcControl.bottom : (rcControl.top + rcControl.bottom) / 2;
		if ((lstrcmpi(szClass, L"ListBox") == 0) && (lFind >= 0))
		{
			RECT rcItem = {};
			if (SendMessage(hCtrl, LB_GETITEMRECT, lFind, (LPARAM)&rcItem) != LB_ERR)
			{
				pty += rcItem.top;
			}
		}
		else if ((lstrcmpi(szClass, L"SysListView32") == 0) && (lFind >= 0))
		{
			RECT rcList = {};
			if (ListView_GetItemRect(hCtrl, lFind, &rcList, LVIR_LABEL))
			{
				pty += rcList.top;
			}
		}
		SendMessage(hBall, TTM_TRACKPOSITION, 0, MAKELONG(ptx,pty));
		SendMessage(hBall, TTM_TRACKACTIVATE, TRUE, (LPARAM)pti);
		SetTimer(hCurTab, BALLOON_MSG_TIMERID, CONTROL_FOUND_TIMEOUT, 0);
	}

wrap:
	// Запомнить
	hLastTab = hCurTab;
	hLastCtrl = hCtrl;

	SafeFree(pszPart);

	SetCursor(LoadCursor(NULL,IDC_ARROW));
}

LRESULT CSettings::OnInitDialog()
{
	//_ASSERTE(!hMain && !hColors && !hCmdTasks && !hViews && !hExt && !hFar && !hInfo && !hDebug && !hUpdate && !hSelection);
	//hMain = hExt = hFar = hTabs = hKeys = hViews = hColors = hCmdTasks = hInfo = hDebug = hUpdate = hSelection = NULL;
	_ASSERTE(m_Pages && (m_Pages[0].PageIndex==thi_Fonts) && !m_Pages[0].hPage /*...*/);
	ClearPages();

	CSetDlgColors::ReleaseHandles();

	_ASSERTE(mp_ImgBtn==NULL);
	SafeDelete(mp_ImgBtn);
	mp_ImgBtn = new CImgButtons(ghOpWnd, tOptionSearch, bSaveSettings);
	mp_ImgBtn->AddDonateButtons();

	if (mp_DpiAware)
	{
		mp_DpiAware->Attach(ghOpWnd, ghWnd, mp_Dialog);
	}

	EditIconHint_Set(ghOpWnd, GetDlgItem(ghOpWnd, tOptionSearch), true,
		CLngRc::getRsrc(lng_SearchCtrlF/*"Search (Ctrl+F)"*/),
		false, UM_SEARCH, bSaveSettings);
	EditIconHint_Subclass(ghOpWnd);

	RECT rcEdt = {}, rcBtn = {};
	if (GetWindowRect(GetDlgItem(ghOpWnd, tOptionSearch), &rcEdt))
	{
		MapWindowPoints(NULL, ghOpWnd, (LPPOINT)&rcEdt, 2);

		// Hate non-strict alignment...
		WORD nCtrls[] = {cbExportConfig};
		for (size_t i = 0; i < countof(nCtrls); i++)
		{
			HWND hBtn = GetDlgItem(ghOpWnd, nCtrls[i]);
			GetWindowRect(hBtn, &rcBtn);
			MapWindowPoints(NULL, ghOpWnd, (LPPOINT)&rcBtn, 2);
			SetWindowPos(hBtn, NULL, rcBtn.left, rcEdt.top-1, rcBtn.right-rcBtn.left, rcBtn.bottom-rcBtn.top, SWP_NOZORDER);
		}

		#if 0
		RECT rcMargins = {0, rcBtn.bottom+2};
		if (gpConEmu->ExtendWindowFrame(ghOpWnd, rcMargins))
		{
			//HDC hDc = GetDC(ghOpWnd);
			//RECT rcClient; GetClientRect(ghOpWnd, &rcClient);
			//rcClient.bottom = rcMargins.top;
			//FillRect(hDc, &rcClient, (HBRUSH)GetStockObject(BLACK_BRUSH));
			//ReleaseDC(ghOpWnd, hDc);
		}
		#endif
	}

	RegisterTipsFor(ghOpWnd);

	HMENU hSysMenu = GetSystemMenu(ghOpWnd, FALSE);
	InsertMenu(hSysMenu, 0, MF_BYPOSITION | MF_SEPARATOR, 0, 0);
	InsertMenu(hSysMenu, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED
	           | ((GetWindowLong(ghOpWnd,GWL_EXSTYLE)&WS_EX_TOPMOST) ? MF_CHECKED : 0),
	           ID_ALWAYSONTOP, _T("Al&ways on top..."));
	RegisterTabs();
	mn_LastChangingFontCtrlId = 0;
	wchar_t szTitle[MAX_PATH*2]; szTitle[0]=0;

	//wchar_t szType[8];
	SettingsStorage Storage = {};
	bool ReadOnly = false;
	gpSet->GetSettingsType(Storage, ReadOnly);
	if (ReadOnly || isResetBasicSettings)
	{
		EnableWindow(GetDlgItem(ghOpWnd, bSaveSettings), FALSE); // Сохранение запрещено
		if (isResetBasicSettings)
		{
			SetDlgItemText(ghOpWnd, bSaveSettings, CLngRc::getRsrc(lng_BasicSettings/*"Настройки по умолчанию"*/));
		}
	}

	if (isResetBasicSettings)
	{
		CEStr lsBracketed(L"<", CLngRc::getRsrc(lng_BasicSettings/*"Настройки по умолчанию"*/), L">");
		SetDlgItemText(ghOpWnd, tStorage, lsBracketed);
	}
	else if (lstrcmp(Storage.szType, CONEMU_CONFIGTYPE_REG) == 0)
	{
		wchar_t szStorage[MAX_PATH*2];
		wcscpy_c(szStorage, L"HKEY_CURRENT_USER\\");
		wcscat_c(szStorage, ConfigPath);
		SetDlgItemText(ghOpWnd, tStorage, szStorage);
	}
	else
	{
		SetDlgItemText(ghOpWnd, tStorage, gpConEmu->ConEmuXml());
	}

	LPCWSTR pszDlgTitle = CLngRc::getRsrc(lng_DlgSettings/*"Settings"*/);
	if (ConfigName[0])
		_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"%s %s (%s) %s", gpConEmu->GetDefaultTitle(), pszDlgTitle, ConfigName, Storage.szType);
	else
		_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"%s %s %s", gpConEmu->GetDefaultTitle(), pszDlgTitle, Storage.szType);

	SetWindowText(ghOpWnd, szTitle);
	MCHKHEAP
	{
		mb_IgnoreSelPage = true;
		TVINSERTSTRUCT ti = {TVI_ROOT, TVI_LAST, {{TVIF_TEXT}}};
		HWND hTree = GetDlgItem(ghOpWnd, tvSetupCategories);
		HTREEITEM hLastRoot = TVI_ROOT;
		bool bNeedExpand = true;
		for (size_t i = 0; m_Pages[i].DialogID; i++)
		{
			if ((m_Pages[i].DialogID == IDD_SPG_UPDATE) && !gpConEmu->isUpdateAllowed())
			{
				m_Pages[i].hTI = NULL;
				continue;
			}

			ti.hParent = m_Pages[i].Level ? hLastRoot : TVI_ROOT;
			ti.item.pszText = m_Pages[i].PageName;

			m_Pages[i].hTI = TreeView_InsertItem(hTree, &ti);

			_ASSERTE(m_Pages[i].hPage==NULL);
			m_Pages[i].hPage = NULL;

			if (m_Pages[i].Level == 0)
			{
				hLastRoot = m_Pages[i].hTI;
				bNeedExpand = !m_Pages[i].Collapsed;
			}
			else
			{
				if (bNeedExpand)
				{
					TreeView_Expand(hTree, hLastRoot, TVE_EXPAND);
				}
				// Only two levels in tree
				bNeedExpand = false;
			}
		}

		TreeView_SelectItem(GetDlgItem(ghOpWnd, tvSetupCategories), gpSetCls->m_Pages[0].hTI);

		HWND hPlace = GetDlgItem(ghOpWnd, tSetupPagePlace);
		ShowWindow(hPlace, SW_HIDE);

		mb_IgnoreSelPage = false;

		CSetPgBase::CreatePage(&(m_Pages[0]), ghOpWnd, mn_ActivateTabMsg, mp_DpiAware);

		apiShowWindow(m_Pages[0].hPage, SW_SHOW);
	}
	MCHKHEAP
	{
		CDpiAware::CenterDialog(ghOpWnd);
	}
	return 0;
}














void CSettings::CheckSelectionModifiers(HWND hWnd2)
{
	struct {
		WORD nCtrlID;
		LPCWSTR Descr;
		TabHwndIndex nDlgID;
		bool bEnabled;
		int nVkIdx;
		bool bIgnoreInFar;
		// Service
		BYTE Vk;
		HWND hPage;
	} Keys[] = {
		{lbCTSBlockSelection, L"Block selection", thi_MarkCopy, gpSet->isCTSSelectBlock, vkCTSVkBlock},
		{lbCTSTextSelection, L"Text selection", thi_MarkCopy, gpSet->isCTSSelectText, vkCTSVkText},
		{lbCTSClickPromptPosition, L"Prompt position", thi_Controls, gpSet->AppStd.isCTSClickPromptPosition!=0, vkCTSVkPromptClk, true},

		// Don't check it?
		// -- {lbFarGotoEditorVk, L"Highlight and goto", ..., gpSet->isFarGotoEditor},

		// Far manager only
		{lbLDragKey, L"Far Manager LDrag", thi_Far, (gpSet->isDragEnabled & DRAG_L_ALLOWED)!=0, vkLDragKey},
		{0},
	};

	bool bIsFar = CVConGroup::isFar(true);

	for (size_t i = 0; Keys[i].nCtrlID; i++)
	{
		Keys[i].hPage = GetPage(Keys[i].nDlgID);

		if (!bIsFar && (Keys[i].nCtrlID == lbLDragKey))
		{
			Keys[i].bEnabled = false;
			continue;
		}

		//GetListBoxByte(hDlg, nCtrlID[i], SettingsNS::szKeysAct, SettingsNS::nKeysAct, Vk[i]);
		DWORD VkMod = gpSet->GetHotkeyById(Keys[i].nVkIdx);
		_ASSERTE((VkMod & 0xFF) == VkMod); // One modifier only?
		Keys[i].Vk = (BYTE)(VkMod & 0xFF);
	}

	for (size_t i = 0; Keys[i+1].nCtrlID; i++)
	{
		if (!Keys[i].bEnabled)
			continue;

		for (size_t j = i+1; Keys[j].nCtrlID; j++)
		{
			if (!Keys[j].bEnabled)
				continue;

			if (((Keys[i].nDlgID == thi_Far) && Keys[j].bIgnoreInFar)
				|| ((Keys[j].nDlgID == thi_Far) && Keys[i].bIgnoreInFar))
				continue;

			if ((Keys[i].Vk == Keys[j].Vk)
				&& ((Keys[i].hPage == hWnd2) || (Keys[j].hPage == hWnd2))
				)
			{
				wchar_t szInfo[255];
				_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"You must set different\nmodifiers for\n<%s> and\n<%s>", Keys[i].Descr, Keys[j].Descr);
				HWND hDlg = hWnd2;
				WORD nID = (Keys[j].hPage == hWnd2) ? Keys[j].nCtrlID : Keys[i].nCtrlID;
				ShowErrorTip(szInfo, hDlg, nID, gpSetCls->szSelectionModError, countof(gpSetCls->szSelectionModError),
							 gpSetCls->hwndBalloon, &gpSetCls->tiBalloon, gpSetCls->hwndTip, FAILED_SELMOD_TIMEOUT, true);
				return;
			}
		}
	}
}

void CSettings::UpdateTextColorSettings(BOOL ChangeTextAttr /*= TRUE*/, BOOL ChangePopupAttr /*= TRUE*/, const AppSettings* apDistinct /*= NULL*/)
{
	// Обновить палитры
	gpSet->PaletteSetStdIndexes();

	// Обновить консоли
	if (ghWnd)
	{
		CVConGroup::OnUpdateTextColorSettings(ChangeTextAttr, ChangePopupAttr, apDistinct);
	}
}

// This is used if user choose palette from dropdown in the Settings dialog
// OR when GuiMacro Palette was called.
void CSettings::ChangeCurrentPalette(const ColorPalette* pPal, bool bChangeDropDown)
{
	if (!pPal)
	{
		_ASSERTE(pPal!=NULL);
		return;
	}

	if (!isMainThread())
	{
		gpConEmu->PostChangeCurPalette(pPal->pszName, bChangeDropDown, false);
		return;
	}

	if (gpSet->isLogging())
	{
		CEStr lsLog(L"Color Palette: `", pPal->pszName, L"` ChangeDropDown=", bChangeDropDown ? L"yes" : L"no");
		LogString(lsLog);
	}

	HWND hDlg = GetPage(thi_Colors);

	if (bChangeDropDown && hDlg)
	{
		CSetDlgLists::SelectStringExact(hDlg, lbDefaultColors, pPal->pszName);
	}

	uint nCount = countof(pPal->Colors);

	for (uint i = 0; i < nCount; i++)
	{
		gpSet->Colors[i] = pPal->Colors[i]; //-V108
	}

	BOOL bTextChanged = (gpSet->AppStd.nTextColorIdx != pPal->nTextColorIdx) || (gpSet->AppStd.nBackColorIdx != pPal->nBackColorIdx);
	BOOL bPopupChanged = (gpSet->AppStd.nPopTextColorIdx != pPal->nPopTextColorIdx) || (gpSet->AppStd.nPopBackColorIdx != pPal->nPopBackColorIdx);

	// We need to change consoles contents if TEXT attributes was changed
	if (bTextChanged || bPopupChanged)
	{
		wchar_t szLog[128];
		_wsprintf(szLog, SKIPCOUNT(szLog)
			L"Color Palette: Text {%u|%u}->{%u|%u} Popup {%u|%u}->{%u|%u}",
			gpSet->AppStd.nTextColorIdx, gpSet->AppStd.nBackColorIdx, pPal->nTextColorIdx, pPal->nBackColorIdx,
			gpSet->AppStd.nPopTextColorIdx, gpSet->AppStd.nPopBackColorIdx, pPal->nPopTextColorIdx, pPal->nPopBackColorIdx);
		LogString(szLog);

		gpSet->AppStd.nTextColorIdx = pPal->nTextColorIdx;
		gpSet->AppStd.nBackColorIdx = pPal->nBackColorIdx;
		gpSet->AppStd.nPopTextColorIdx = pPal->nPopTextColorIdx;
		gpSet->AppStd.nPopBackColorIdx = pPal->nPopBackColorIdx;

		UpdateTextColorSettings(bTextChanged, bPopupChanged);

		LogString(L"Color Palette: UpdateTextColorSettings finished");
	}

	LogString(L"Color Palette: Refreshing");

	gpSet->AppStd.nExtendColorIdx = pPal->nExtendColorIdx;
	gpSet->AppStd.isExtendColors = pPal->isExtendColors;

	if (hDlg && (hDlg == GetActivePage()))
	{
		CSetPgColors* pColorsPg;
		if (GetPageObj(pColorsPg))
			pColorsPg->OnInitDialog(hDlg, false);
	}

	if (ghWnd)
	{
		gpConEmu->InvalidateAll();
		gpConEmu->Update(true);
	}

	LogString(L"Color Palette: Finished");
}

// TODO: Move to CSetPgBase!!!
LRESULT CSettings::OnListBoxDblClk(HWND hWnd2, WPARAM wParam, LPARAM lParam)
{
	WORD wId = LOWORD(wParam);

	switch (wId)
	{
	case lbStatusAvailable:
		OnButtonClicked(hWnd2, cbStatusAddSelected, 0);
		break;
	case lbStatusSelected:
		OnButtonClicked(hWnd2, cbStatusDelSelected, 0);
		break;
	}

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
	if (gpFontMgr->szFontError[0])
	{
		gpFontMgr->szFontError[0] = 0;
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
			for (size_t i = 0; m_Pages[i].DialogID; i++)
			{
				if (p->itemNew.hItem == m_Pages[i].hTI)
				{
					if (m_Pages[i].hPage == NULL)
					{
						SetCursor(LoadCursor(NULL,IDC_WAIT));
						CSetPgBase::CreatePage(&(m_Pages[i]), ghOpWnd, mn_ActivateTabMsg, mp_DpiAware);
					}
					else
					{
						SendMessage(m_Pages[i].hPage, mn_ActivateTabMsg, 1, (LPARAM)&(m_Pages[i]));
						m_Pages[i].pPage->ProcessDpiChange(gpSetCls->mp_DpiAware);
					}
					ShowWindow(m_Pages[i].hPage, SW_SHOW);
					m_LastActivePageId = gpSetCls->m_Pages[i].PageIndex;
				}
				else if (p->itemOld.hItem == m_Pages[i].hTI)
				{
					hCurrent = m_Pages[i].hPage;
				}
			}
			if (hCurrent)
				ShowWindow(hCurrent, SW_HIDE);
		} // TVN_SELCHANGED
		break;
	}

	return 0;
}

/// IdShowPage is DialogID (IDD_SPG_FONTS, etc.)
void CSettings::Dialog(int IdShowPage /*= 0*/)
{
	if (!ghOpWnd || !IsWindow(ghOpWnd))
	{
		_ASSERTE(isMainThread());

		SetCursor(LoadCursor(NULL,IDC_WAIT));

		// Сначала обновить DC, чтобы некрасивостей не было
		gpConEmu->UpdateWindowChild(NULL);

		if (!gpSetCls->mp_DpiAware
			#ifndef _DEBUG
			&& CDpiAware::IsPerMonitorDpi()
			#endif
			)
		{
			gpSetCls->mp_DpiAware = new CDpiForDialog();
		}

		wchar_t szLog[80]; _wsprintf(szLog, SKIPCOUNT(szLog) L"Creating settings dialog, IdPage=%u", IdShowPage);
		LogString(szLog);

		gpSetCls->InitPageNames();

		SafeDelete(gpSetCls->mp_Dialog);
		//2009-05-03. Нам нужен НЕмодальный диалог
		gpSetCls->mp_Dialog = CDynDialog::ShowDialog(IDD_SETTINGS, NULL, wndOpProc, 0/*dwInitParam*/);

		if (!gpSetCls->mp_Dialog)
		{
			DisplayLastError(L"Can't create settings dialog!");
			goto wrap;
		}
		else
		{
			_ASSERTE(ghOpWnd == gpSetCls->mp_Dialog->mh_Dlg);
			ghOpWnd = gpSetCls->mp_Dialog->mh_Dlg;
		}
	}

	apiShowWindow(ghOpWnd, SW_SHOWNORMAL);

	TabHwndIndex showPage = thi_Last;
	if (IdShowPage)
		showPage = gpSetCls->GetPageIdByDialogId(IdShowPage);
	if ((showPage == thi_Last) && (gpSetCls->m_LastActivePageId != thi_Last))
		showPage = gpSetCls->m_LastActivePageId;

	if (showPage != thi_Last)
	{
		for (size_t i = 0; gpSetCls->m_Pages[i].DialogID; i++)
		{
			if (gpSetCls->m_Pages[i].PageIndex == showPage)
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

void CSettings::OnSettingsClosed()
{
	if (!ghOpWnd)
		return;

	gpConEmu->OnPanelViewSettingsChanged();

	gpConEmu->RegisterMinRestore(gpSet->IsHotkey(vkMinimizeRestore) || gpSet->IsHotkey(vkMinimizeRestor2));

	if (gpSet->m_isKeyboardHooks == 1)
		gpConEmu->RegisterHooks();
	else if (gpSet->m_isKeyboardHooks == 2)
		gpConEmu->UnRegisterHooks();

	UnregisterTabs();

	CSetDlgColors::ReleaseHandles();

	if (hwndTip)
	{
		DestroyWindow(hwndTip);
		hwndTip = NULL;
	}

	if (hwndBalloon)
	{
		DestroyWindow(hwndBalloon);
		hwndBalloon = NULL;
	}

	// mp_DpiAware and others are cleared in ClearPages()
	ClearPages();

	if (m_ActivityLoggingType != glt_None)
	{
		m_ActivityLoggingType = glt_None;
		gpConEmu->OnGlobalSettingsChanged();
	}

	gpConEmu->OnOurDialogClosed();

	ghOpWnd = NULL;
}

void CSettings::OnResetOrReload(bool abResetOnly, SettingsStorage* pXmlStorage /*= NULL*/)
{
	bool lbWasPos = false;
	RECT rcWnd = {};
	int nSel = -1;

	wchar_t* pszMsg = NULL;
	LPCWSTR pszWarning = L"\n\nWarning!!!\nAll your current settings will be lost!";
	if (pXmlStorage)
	{
		_ASSERTE(abResetOnly == false);
		pszMsg = lstrmerge(L"Confirm import settings from file:\n", pXmlStorage->pszFile ? pXmlStorage->pszFile : L"???", pszWarning);
	}
	else if (abResetOnly)
	{
		pszMsg = lstrmerge(L"Confirm reset settings to defaults", pszWarning);
	}
	else
	{
		pszMsg = lstrmerge(L"Confirm reload settings from ", gpSet->Type, pszWarning);
	}

	int nBtn = MsgBox(pszMsg, MB_YESNO|MB_ICONEXCLAMATION|MB_DEFBUTTON2, gpConEmu->GetDefaultTitle(), ghOpWnd);
	if (nBtn != IDYES)
		return;

	SetCursor(LoadCursor(NULL,IDC_WAIT));
	gpConEmu->Taskbar_SetProgressState(TBPF_INDETERMINATE);

	if (ghOpWnd && IsWindow(ghOpWnd))
	{
		lbWasPos = true;
		nSel = TabCtrl_GetCurSel(GetDlgItem(ghOpWnd, tabMain));
		GetWindowRect(ghOpWnd, &rcWnd);
		DestroyWindow(ghOpWnd);
	}
	_ASSERTE(ghOpWnd == NULL);

	// Сброс настроек на умолчания
	gpSet->InitSettings();

	// Почистить макросы и сбросить на умолчания
	CSetPgKeys::ReInitHotkeys();

	if (!abResetOnly)
	{
		// Если надо - загрузить из реестра/xml
		bool bNeedCreateVanilla = false;
		gpSet->LoadSettings(bNeedCreateVanilla, pXmlStorage);
	}
	else
	{
		// Иначе - какие-то настройки могут быть модифицированы, как для "Новой конфигурации"
		gpSet->IsConfigNew = true;
		gpSet->InitVanilla();
	}


	SettingsLoadedFlags slfFlags = slf_OnResetReload
		| (abResetOnly ? (slf_DefaultSettings|slf_AllowFastConfig) : slf_None);

	SettingsLoaded(slfFlags, NULL);

	if (lbWasPos && !ghOpWnd)
	{
		Dialog();
		TabCtrl_SetCurSel(GetDlgItem(ghOpWnd, tabMain), nSel);
	}

	SetCursor(LoadCursor(NULL,IDC_ARROW));
	gpConEmu->Taskbar_SetProgressState(TBPF_NOPROGRESS);

	MsgBox(L"Don't forget to Save your new settings", MB_ICONINFORMATION, gpConEmu->GetDefaultTitle(), ghOpWnd);
}

void CSettings::ExportSettings()
{
	wchar_t *pszDefPath = lstrdup(gpConEmu->ConEmuXml());
	wchar_t *pszSlash = pszDefPath ? wcsrchr(pszDefPath, L'\\') : NULL;
	if (pszSlash) *(pszSlash+1) = 0;

	wchar_t *pszFile = SelectFile(L"Export settings", L"*.xml", pszDefPath, ghOpWnd, L"XML files (*.xml)\0*.xml\0", sff_SaveNewFile);
	if (pszFile)
	{
		SetCursor(LoadCursor(NULL,IDC_WAIT));
		gpConEmu->Taskbar_SetProgressState(TBPF_INDETERMINATE);

		// Export using ".Vanilla" configuration!
		wchar_t* pszSaveName = NULL;
		if (ConfigName && *ConfigName)
		{
			pszSaveName = lstrdup(ConfigName);
			SetConfigName(L"");
		}

		SettingsStorage XmlStorage = {CONEMU_CONFIGTYPE_XML};
		XmlStorage.pszFile = pszFile;
		bool bOld = isResetBasicSettings; isResetBasicSettings = false;
		gpSet->SaveSettings(FALSE, &XmlStorage);
		isResetBasicSettings = bOld;
		SafeFree(pszFile);

		// Restore configuration name if any
		if (pszSaveName)
		{
			SetConfigName(pszSaveName);
			SafeFree(pszSaveName);
		}

		SetCursor(LoadCursor(NULL,IDC_ARROW));
		gpConEmu->Taskbar_SetProgressState(TBPF_NOPROGRESS);
	}

	SafeFree(pszDefPath);
}

void CSettings::ImportSettings()
{
	wchar_t *pszFile = SelectFile(L"Import settings", L"*.xml", NULL, ghOpWnd, L"XML files (*.xml)\0*.xml\0", sff_Default);
	if (pszFile)
	{
		// Import using ".Vanilla" configuration!
		wchar_t* pszSaveName = NULL;
		if (ConfigName && *ConfigName)
		{
			pszSaveName = lstrdup(ConfigName);
			SetConfigName(L"");
		}

		SettingsStorage XmlStorage = {CONEMU_CONFIGTYPE_XML};
		XmlStorage.pszFile = pszFile;

		OnResetOrReload(false, &XmlStorage);

		// Restore configuration name if any
		if (pszSaveName)
		{
			SetConfigName(pszSaveName);
			SafeFree(pszSaveName);
		}

		SafeFree(pszFile);
	}
}

INT_PTR CSettings::ProcessTipHelp(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	LPNMTTDISPINFO lpnmtdi = (LPNMTTDISPINFO)lParam;

	if (!lpnmtdi)
	{
		_ASSERTE(lpnmtdi);
		return 0;
	}

	// If your message handler sets the uFlags field of the NMTTDISPINFO structure to TTF_DI_SETITEM,
	// the ToolTip control stores the information and will not request it again.
	static wchar_t szHint[2000];

	_ASSERTE((lpnmtdi->uFlags & TTF_IDISHWND) == TTF_IDISHWND);

	if (mp_HelpPopup->mh_Popup)
	{
		POINT MousePos = {}; GetCursorPos(&MousePos);
		mp_HelpPopup->ShowItemHelp(0, (HWND)lpnmtdi->hdr.idFrom, MousePos);

		szHint[0] = 0;
		lpnmtdi->lpszText = szHint;
	}
	else
	{
		if (gpSet->isShowHelpTooltips)
		{
			mp_HelpPopup->GetItemHelp(0, (HWND)lpnmtdi->hdr.idFrom, szHint, countof(szHint));

			lpnmtdi->lpszText = szHint;
		}
		else
		{
			szHint[0] = 0;
		}
	}

	lpnmtdi->szText[0] = 0;

	return 0;
}

// DlgProc для окна настроек (IDD_SETTINGS)
INT_PTR CSettings::wndOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	_ASSERTE(ghOpWnd == hWnd2 || ghOpWnd == NULL);

	INT_PTR lRc = 0;
	if ((gpSetCls->mp_ImgBtn && gpSetCls->mp_ImgBtn->Process(hWnd2, messg, wParam, lParam, lRc))
		|| EditIconHint_Process(hWnd2, messg, wParam, lParam, lRc))
	{
		SetWindowLongPtr(hWnd2, DWLP_MSGRESULT, lRc);
		return TRUE;
	}

	PatchMsgBoxIcon(hWnd2, messg, wParam, lParam);

	switch (messg)
	{
		case WM_INITDIALOG:
			gpConEmu->OnOurDialogOpened();
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

			switch (HIWORD(wParam))
			{
				case BN_CLICKED:
				{
					switch (LOWORD(wParam))
					{
					case bSaveSettings:
						{
							if (/*ReadOnly || */gpSetCls->isResetBasicSettings)
							{
								//DisplayLastError(isResetBasicSettings ? L"Not allowed in '/Basic' mode" : L"Settings storage is read only", -1);
								DisplayLastError(L"Not allowed in '/Basic' mode", -1);
								return 0;
							}

							bool isShiftPressed = isPressed(VK_SHIFT);

							// были изменения в полях размера/положения?
							HWND hSizePosPg = gpSetCls->GetPage(thi_SizePos);
							if (hSizePosPg
								&& IsWindowEnabled(GetDlgItem(hSizePosPg, cbApplyPos)))
							{
								gpSetCls->OnButtonClicked(hSizePosPg, cbApplyPos, 0);
							}

							if (gpSet->SaveSettings())
							{
								if (!isShiftPressed)
									SendMessage(ghOpWnd,WM_COMMAND,IDOK,0);
							}
						}
						break;

					case bResetSettings:
					case bReloadSettings:
						gpSetCls->OnResetOrReload(LOWORD(wParam) == bResetSettings);
						break;
					case cbExportConfig:
						gpSetCls->ExportSettings();
						break;
					case bImportSettings:
						gpSetCls->ImportSettings();
						break;

					case IDCANCEL:
						{
							// Clear the ‘Filter’ field on the Keys&Macro page, for example
							CSetPgBase* pg = gpSetCls->GetActivePageObj();
							if (pg && !pg->QueryDialogCancel())
								break;
						}
					case IDOK:
					case IDCLOSE:
						// -- перенесено в WM_CLOSE
						//if (gpSet->isTabs==1) gpConEmu->ForceShowTabs(TRUE); else
						//if (gpSet->isTabs==0) gpConEmu->ForceShowTabs(FALSE); else
						//	gpConEmu->mp_TabBar->Update();
						//gpConEmu->OnPanelViewSettingsChanged();
						SendMessage(ghOpWnd, WM_CLOSE, 0, 0);
						break;

					default:
						gpSetCls->OnButtonClicked(hWnd2, wParam, lParam);
					}
				}
				break;

				case EN_CHANGE:
				{
					_ASSERTE(LOWORD(wParam)==tStorage && "EN_CHANGE is ignored in the ghOpWnd!");
					//gpSetCls->OnEditChanged(hWnd2, wParam, lParam);
				}
				break;

				case CBN_EDITCHANGE:
				case CBN_SELCHANGE:
				{
					_ASSERTE(HIWORD(wParam)!=CBN_EDITCHANGE && "CBN_EDITCHANGE is NOT expected in ghOpWnd!");
					_ASSERTE(HIWORD(wParam)!=CBN_SELCHANGE && "CBN_SELCHANGE is NOT expected in ghOpWnd!");
					//gpSetCls->OnComboBox(hWnd2, wParam, lParam);
				}
				break;
			} // switch (HIWORD(wParam))
			break; // case WM_COMMAND

		case UM_SEARCH:
			gpSetCls->SearchForControls();
			break;

		case WM_NOTIFY:
		{
			LPNMHDR phdr = (LPNMHDR)lParam;

			if (phdr->code == TTN_GETDISPINFO)
			{
				return gpSetCls->ProcessTipHelp(hWnd2, messg, wParam, lParam);
			}
			else switch (phdr->idFrom)
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
			gpSetCls->OnSettingsClosed();
			DestroyWindow(hWnd2);
		} break;
		case WM_DESTROY:
		{
			gpSetCls->OnSettingsClosed();
		} break;
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

			break;

		case WM_ACTIVATE:

			if (LOWORD(wParam) != 0)
				gpSetCls->RegisterTabs();
			else
				gpSetCls->UnregisterTabs();

			break;

		case HELP_WM_HELP:
			break;
		case WM_HELP:
			if ((wParam == 0) && (lParam != 0))
			{
				// Показать хинт?
				HELPINFO* hi = (HELPINFO*)lParam;
				if (hi->cbSize >= sizeof(HELPINFO))
				{
					switch (hi->iCtrlId)
					{
					case tCmdGroupCommands:
						// Some controls are processed personally
						ConEmuAbout::OnInfo_About(L"-new_console");
						break;
					default:
						gpSetCls->mp_HelpPopup->ShowItemHelp(hi);
					}
				}
			}
			return TRUE;

		default:
			if (gpSetCls->mp_DpiAware && gpSetCls->mp_DpiAware->ProcessDpiMessages(hWnd2, messg, wParam, lParam))
			{
				// Refresh the visible page and mark 'to be changed' others
				for (ConEmuSetupPages* p = gpSetCls->m_Pages; p->DialogID; p++)
				{
					if (p->pPage)
					{
						p->pPage->OnDpiChanged(gpSetCls->mp_DpiAware);
					}
				}

				return TRUE;
			}
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
		int nDpi = GetDialogDpi();
		if (nDpi >= 96 && _dpi_all.Ydpi >= 96)
		{
			pItem->itemHeight = 15 * nDpi / 96;
		}
		else
		{
			_ASSERTE(nDpi >= 96 && _dpi_all.Ydpi >= 96);
			pItem->itemHeight = 15;
		}
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
		int nDpi = GetDialogDpi();
		HFONT hFont = CreateFont(-8*nDpi/72, 0,0,0,(bAlmostMonospace==1)?FW_BOLD:FW_NORMAL,0,0,0,
		                         ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH,
		                         L"MS Shell Dlg");
		HFONT hOldF = (HFONT)SelectObject(pItem->hDC, hFont);
		DrawText(pItem->hDC, szText, _tcslen(szText), &rc, DT_LEFT|DT_VCENTER|DT_NOPREFIX);
		SelectObject(pItem->hDC, hOldF);
		DeleteObject(hFont);
	}

	return TRUE;
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
		CEStr szArg;
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
	HWND hDebugPg = NULL;
	if ((m_ActivityLoggingType != glt_Commands) || ((hDebugPg = GetPage(thi_Debug)) == NULL))
		return;

	_ASSERTE(abInput==TRUE || pResult!=NULL || (pInfo->hdr.nCmd==CECMD_LANGCHANGE || pInfo->hdr.nCmd==CECMD_GUICHANGED || pInfo->hdr.nCmd==CMD_FARSETCHANGED || pInfo->hdr.nCmd==CECMD_ONACTIVATION));

	LogCommandsData* pData = (LogCommandsData*)calloc(1,sizeof(LogCommandsData));

	if (!pData)
		return;

	pData->bInput = abInput;
	pData->bMainThread = (abInput == FALSE) && isMainThread();
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
		lstrcpyn(pData->szExtra, pInfo->NewCmd.GetCommand(), countof(pData->szExtra));
		break;
	case CECMD_GUIMACRO:
		lstrcpyn(pData->szExtra, pInfo->GuiMacro.sMacro, countof(pData->szExtra));
		break;
	case CMD_POSTMACRO:
		lstrcpyn(pData->szExtra, (LPCWSTR)pInfo->wData, countof(pData->szExtra));
		break;
	}

	PostMessage(hDebugPg, DBGMSG_LOG_ID, DBGMSG_LOG_CMD_MAGIC, (LPARAM)pData);
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

void CSettings::UpdateWindowMode(ConEmuWindowMode WndMode)
{
	_ASSERTE(WndMode==wmNormal || WndMode==wmMaximized || WndMode==wmFullScreen);

	if (gpSet->isQuakeStyle)
	{
		return;
	}

	if (gpSet->isUseCurrentSizePos)
	{
		gpSet->_WindowMode = WndMode;
	}

	HWND hSizePosPg = GetPage(thi_SizePos);
	if (hSizePosPg && gpSet->isUseCurrentSizePos)
	{
		checkRadioButton(hSizePosPg, rNormal, rFullScreen,
			(WndMode == wmFullScreen) ? rFullScreen
			: (WndMode == wmMaximized) ? rMaximized
			: rNormal);

	}
}

void CSettings::UpdatePos(int ax, int ay, bool bGetRect)
{
	int x = ax, y = ay;

	if (!gpConEmu->isFullScreen()
		&& !gpConEmu->isZoomed()
		&& !gpConEmu->isIconic()
		&& (gpConEmu->GetTileMode(false) == cwc_Current))
	{
		RECT rc; GetWindowRect(ghWnd, &rc);
		x = rc.left; y = rc.top;
	}

	if ((gpConEmu->wndX != x) || (gpConEmu->wndY != y))
	{
		if (gpConEmu->wndX != x)
			gpConEmu->wndX = x;
		if (gpConEmu->wndY != y)
			gpConEmu->wndY = y;
	}

	if (gpSet->isUseCurrentSizePos)
	{
		if (gpSet->_wndX != x)
			gpSet->_wndX = x;
		if (gpSet->_wndY != y)
			gpSet->_wndY = y;
	}

	HWND hSizePosPg = GetPage(thi_SizePos);
	if (hSizePosPg)
	{
		MSetter lIgnoreEdit(&CSetPgBase::mb_IgnoreEditChanged);
		SetDlgItemInt(hSizePosPg, tWndX, gpSet->isUseCurrentSizePos ? gpConEmu->wndX : gpSet->_wndX, TRUE);
		SetDlgItemInt(hSizePosPg, tWndY, gpSet->isUseCurrentSizePos ? gpConEmu->wndY : gpSet->_wndY, TRUE);
	}

	wchar_t szLabel[128];
	_wsprintf(szLabel, SKIPLEN(countof(szLabel)) L"UpdatePos A={%i,%i} C={%i,%i} S={%i,%i}", ax,ay, gpConEmu->wndX, gpConEmu->wndY, gpSet->_wndX, gpSet->_wndY);
	gpConEmu->LogWindowPos(szLabel);
}

void CSettings::UpdateSize(const CESize w, const CESize h)
{
	//Issue ???: Сохранять размер Quake?
	bool bIgnoreWidth = (gpSet->isQuakeStyle != 0) && (gpSet->_WindowMode != wmNormal);

	if (w.IsValid(true) && h.IsValid(false))
	{
		if ((w.Raw != gpConEmu->WndWidth.Raw) || (h.Raw != gpConEmu->WndHeight.Raw))
		{
			gpConEmu->WndWidth.Set(true, w.Style, w.Value);
			gpConEmu->WndHeight.Set(false, h.Style, h.Value);
		}

		if (gpSet->isUseCurrentSizePos && ((w.Raw != gpSet->wndWidth.Raw) || (h.Raw != gpSet->wndHeight.Raw)))
		{
			if (!bIgnoreWidth)
				gpSet->wndWidth.Set(true, w.Style, w.Value);
			gpSet->wndHeight.Set(false, h.Style, h.Value);
		}
	}

	CSetPgSizePos* pSizePosPg;
	if (GetPageObj(pSizePosPg))
		pSizePosPg->FillSizeControls(pSizePosPg->Dlg());

	wchar_t szLabel[128];
	CESize ws = {w.Raw};
	CESize hs = {h.Raw};
	_wsprintf(szLabel, SKIPLEN(countof(szLabel)) L"UpdateSize A={%s,%s} C={%s,%s} S={%s,%s}", ws.AsString(), hs.AsString(), gpConEmu->WndWidth.AsString(), gpConEmu->WndHeight.AsString(), gpSet->wndWidth.AsString(), gpSet->wndHeight.AsString());
	gpConEmu->LogWindowPos(szLabel);
}

void CSettings::UpdateFontInfo()
{
	CSetPgInfo* pPage;
	if (GetPageObj(pPage))
		pPage->FillFontInfo(pPage->Dlg());
}

void CSettings::PostUpdateCounters(bool bPosted)
{
	if (!bPosted)
	{
		if (!mb_MsgUpdateCounter)
		{
			mb_MsgUpdateCounter = TRUE;
			PostMessage(GetPage(thi_Info), mn_MsgUpdateCounter, 0, 0);
		}
		return;
	}

	wchar_t szTotal[256] = L"";

	if (mn_Freq!=0)
	{
		_ASSERTE(tPerfFPS == 0);

		for (INT_PTR nID = tPerfFPS; nID < tPerfLast; nID++)
		{
			wchar_t sTemp[64];

			i64 v = 0, v2 = 0, v3 = 0;

			if (nID == tPerfFPS || nID == tPerfInterval)
			{
				i64 *pFPS = NULL;
				UINT nCount = 0;

				if (nID == tPerfFPS)
				{
					pFPS = mn_FPS; nCount = countof(mn_FPS);
				}
				else
				{
					pFPS = mn_RFPS; nCount = countof(mn_RFPS);
				}

				i64 tmin, tmax;
				i64 imin = 0, imax = 0;
				tmax = tmin = pFPS[0];

				for (UINT i = 0; i < nCount; i++)
				{
					i64 vi = pFPS[i]; //-V108
					if (!vi) continue;

					if (vi < tmin) { tmin = vi; imin = i; }
					if (vi > tmax) { tmax = vi; imax = i; }
				}

				if ((tmax > tmin) && mn_Freq > 0)
				{
					_ASSERTE(imin!=imax);
					i64 iSamples = imax - imin;
					if (iSamples < 0)
						iSamples += nCount;
					v = iSamples * 10 * mn_Freq / (tmax - tmin);
				}
			}
			else if (nID == tPerfKeyboard)
			{
				v = v2 = mn_KbdDelays[0]; v3 = 0;

				size_t nCount = max(0, min(mn_KBD_CUR_FRAME, (int)countof(mn_KbdDelays)));
				for (size_t i = 0; i < nCount; i++)
				{
					i64 vi = mn_KbdDelays[i];
					// Skip too large values, they may be false detected
					if (vi <= 0 || vi >= 5000) continue;

					if (vi < v)
						v = vi;
					if (vi > v2 && vi < 5000)
						v2 = vi;
					v3 += vi; // avg
				}

				if (nCount > 0)
					v3 = v3 / nCount;
			}
			else
			{
				v = (10000*(__int64)mn_CounterMax[nID])/mn_Freq;
			}

			// WinApi's wsprintf can't do float/double, so we use integer arithmetics for FPS and others

			if (nID == tPerfKeyboard)
				_wsprintf(sTemp, SKIPLEN(countof(sTemp)) L"%u/%u/%u", (int)v, (int)v3, (int)v2);
			else
				_wsprintf(sTemp, SKIPLEN(countof(sTemp)) L"%u.%u", (int)(v/10), (int)(v%10));

			switch (nID)
			{
			case tPerfFPS:
				wcscat_c(szTotal, L"   FPS:"); break;
			case tPerfData:
				wcscat_c(szTotal, L"   Data:"); break;
			case tPerfRender:
				wcscat_c(szTotal, L"   Render:"); break;
			case tPerfBlt:
				wcscat_c(szTotal, L"   Blt:"); break;
			case tPerfInterval:
				wcscat_c(szTotal, L"   RPS:"); break;
			case tPerfKeyboard:
				wcscat_c(szTotal, L"   KBD:"); break;
			default:
				wcscat_c(szTotal, L"   ???:");
			}
			wcscat_c(szTotal, sTemp);
		} // if (mn_Freq!=0)
	}

	SetDlgItemText(GetPage(thi_Info), tPerfCounters, SkipNonPrintable(szTotal)); //-V107

	// Done, allow next show cycle
	mb_MsgUpdateCounter = FALSE;
}

void CSettings::Performance(UINT nID, BOOL bEnd)
{
	_ASSERTE(gbPerformance > tPerfLast);

	if (nID == gbPerformance)  //groupbox ctrl id
	{
		if (!isMainThread())
			return;

		if (ghOpWnd)
		{
			// Performance
			wchar_t sTemp[128];
			//Нихрена это не мегагерцы. Например на "AMD Athlon 64 X2 1999 MHz" здесь отображается "0.004 GHz"
			_wsprintf(sTemp, SKIPLEN(countof(sTemp)) L"Performance counters (%I64i)", ((i64)(mn_Freq/1000)));
			SetDlgItemText(GetPage(thi_Info), nID, sTemp);
			// Обновить сразу (значений еще нет)
			PostUpdateCounters(true);
		}

		return;
	}

	if (nID >= tPerfLast)
		return;

	i64 tick2 = 0, t;

	if (nID == tPerfFPS)
	{
		QueryPerformanceCounter((LARGE_INTEGER *)&tick2);
		mn_FPS[mn_FPS_CUR_FRAME] = tick2;
		mn_FPS_CUR_FRAME++;

		if (mn_FPS_CUR_FRAME >= (int)countof(mn_FPS)) mn_FPS_CUR_FRAME = 0;

		if (ghOpWnd)
			PostUpdateCounters(false);

		return;
	}

	if (nID == tPerfInterval)
	{
		QueryPerformanceCounter((LARGE_INTEGER *)&tick2);
		mn_RFPS[mn_RFPS_CUR_FRAME] = tick2;
		mn_RFPS_CUR_FRAME++;

		if (mn_RFPS_CUR_FRAME >= (int)countof(mn_RFPS))
			mn_RFPS_CUR_FRAME = 0;

		if (ghOpWnd)
			PostUpdateCounters(false);

		return;
	}

	if (nID == tPerfKeyboard)
	{
		QueryPerformanceCounter((LARGE_INTEGER *)&tick2);

		if (bEnd == (BOOL)-1)
		{
			// After tab switch for example (RCon changed);
			mn_KbdDelayCounter = 0;
		}
		else if (!bEnd)
		{
			// Must be called when user press something on the keyboard
			if (!mn_KbdDelayCounter)
				mn_KbdDelayCounter = tick2;
		}
		else if (mn_KbdDelayCounter && mn_Freq)
		{
			i64 iPrev = mn_KbdDelayCounter; mn_KbdDelayCounter = 0;
			// let eval ms the delay of console output is refreshed
			t = (tick2 - iPrev) * 1000 / mn_Freq;
			int idx = (InterlockedIncrement(&mn_KBD_CUR_FRAME) & (countof(mn_KbdDelays)-1));
			mn_KbdDelays[idx] = t;
			if (mn_KBD_CUR_FRAME < 0) mn_KBD_CUR_FRAME = countof(mn_KbdDelays)-1;
		}

		return;
	}

	if (!bEnd)
	{
		QueryPerformanceCounter((LARGE_INTEGER *)&(mn_Counter[nID]));
		return;
	}
	else if (!mn_Counter[nID] || !mn_Freq)
	{
		return;
	}

	QueryPerformanceCounter((LARGE_INTEGER *)&tick2);
	t = (tick2-mn_Counter[nID]);

	if ((t >= 0)
		&& ((mn_CounterMax[nID] < t)
	        || ((GetTickCount() - mn_CounterTick[nID]) > COUNTER_REFRESH)))
	{
		mn_CounterMax[nID] = t;
		mn_CounterTick[nID] = GetTickCount();

		if (ghOpWnd)
			PostUpdateCounters(false);
	}
}

DWORD CSettings::BalloonStyle()
{
	//грр, Issue 886, и подсказки нифига не видны
	//[HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced]
	//"EnableBalloonTips"=0

	DWORD nBalloonStyle = TTS_BALLOON;

	HKEY hk;
	if (0 == RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, KEY_READ, &hk))
	{
		DWORD nEnabled = 1;
		DWORD nSize = sizeof(nEnabled);
		if ((0 == RegQueryValueEx(hk, L"EnableBalloonTips", NULL, NULL, (LPBYTE)&nEnabled, &nSize)) && (nEnabled == 0))
		{
			nBalloonStyle = 0;
		}
		RegCloseKey(hk);
	}

	return nBalloonStyle;
}

void CSettings::RegisterTipsFor(HWND hChildDlg)
{
	//TODO: Actually, all dialogs have to be localized

	if (gpSetCls->hConFontDlg == hChildDlg)
	{
		if (!hwndConFontBalloon || !IsWindow(hwndConFontBalloon))
		{
			hwndConFontBalloon = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
			                                    BalloonStyle() | WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX | TTS_CLOSE,
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

		CDynDialog::LocalizeDialog(hChildDlg);

	}
	else
	{
		// Used for highlight found control, for example
		if (!hwndBalloon || !IsWindow(hwndBalloon))
		{
			hwndBalloon = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
			                             BalloonStyle() | WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
			                             CW_USEDEFAULT, CW_USEDEFAULT,
			                             CW_USEDEFAULT, CW_USEDEFAULT,
			                             ghOpWnd, NULL,
			                             g_hInstance, NULL);
			SetWindowPos(hwndBalloon, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
			// Set up tool information.
			// In this case, the "tool" is the entire parent window.
			tiBalloon.cbSize = 44; // был sizeof(TOOLINFO);
			tiBalloon.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
			tiBalloon.hwnd = GetPage(thi_Fonts);
			tiBalloon.hinst = g_hInstance;
			static wchar_t szAsterisk[] = L"*"; // eliminate GCC warning
			tiBalloon.lpszText = szAsterisk;
			tiBalloon.uId = (UINT_PTR)tiBalloon.hwnd;
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
			                         BalloonStyle() | WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
			                         CW_USEDEFAULT, CW_USEDEFAULT,
			                         CW_USEDEFAULT, CW_USEDEFAULT,
			                         ghOpWnd, NULL,
			                         g_hInstance, NULL);
			SetWindowPos(hwndTip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
			SendMessage(hwndTip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 30000);
		}

		// Register tips, if succeeded
		if (hwndTip)
			EnumChildWindows(hChildDlg, RegisterTipsForChild, (LPARAM)hChildDlg);
		else
			CDynDialog::LocalizeDialog(hChildDlg);

		// Final tooltip polishing
		if (hwndTip)
			SendMessage(hwndTip, TTM_SETMAXTIPWIDTH, 0, (LPARAM)EvalSize(300, esf_CanUseDpi|esf_Horizontal));
	}
}

BOOL CSettings::RegisterTipsForChild(HWND hChild, LPARAM lParam)
{
	HWND hChildDlg = (HWND)lParam;

	#if defined(_DEBUG)
	LONG wID = GetWindowLong(hChild, GWL_ID);
	if (wID == stCmdGroupCommands)
		wID = wID;
	#endif

	// Localize Control text
	CDynDialog::LocalizeControl(hChild, lParam);

	// Register tooltip by child HWND
	if (gpSetCls->hwndTip)
	{
		BOOL lbRc = FALSE;
		HWND hEdit = NULL;

		// Associate the ToolTip with the tool.
		TOOLINFO toolInfo = { 0 };
		toolInfo.cbSize = 44; //sizeof(toolInfo); -- need to work on Win2k and compile with Vista+
		GetWindowRect(hChild, &toolInfo.rect);
		MapWindowPoints(NULL, hChildDlg, (LPPOINT)&toolInfo.rect, 2);
		toolInfo.hwnd = hChildDlg;
		toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
		toolInfo.uId = (UINT_PTR)hChild;

		// Use CSettings::ProcessTipHelp dynamically
		toolInfo.lpszText = LPSTR_TEXTCALLBACK;

		lbRc = SendMessage(gpSetCls->hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
		hEdit = FindWindowEx(hChild, NULL, L"Edit", NULL);

		if (hEdit)
		{
			toolInfo.lpszText = LPSTR_TEXTCALLBACK;
			toolInfo.uId = (UINT_PTR)hEdit;

			GetWindowRect(hEdit, &toolInfo.rect);
			MapWindowPoints(NULL, hChildDlg, (LPPOINT)&toolInfo.rect, 2);
			toolInfo.hwnd = hChildDlg;

			lbRc = SendMessage(gpSetCls->hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
		}

		UNREFERENCED_PARAMETER(lbRc);
	}

	return TRUE; // Continue enumeration
}


// Вызывается из диалога настроек
void CSettings::RecreateFont(WORD wFromID)
{
	_ASSERTE(wFromID != (WORD)-1);
	gpFontMgr->RecreateFont(false, (wFromID == cbFontAsDeviceUnits || wFromID == cbFontMonitorDpi));

	if (ghOpWnd && (wFromID == tFontFace))
	{
		HWND hMainPg = gpSetCls->GetPage(thi_Fonts);
		if (hMainPg)
		{
			wchar_t szSize[10];
			_wsprintf(szSize, SKIPLEN(countof(szSize)) L"%i", gpSet->FontSizeY);
			SetDlgItemText(hMainPg, tFontSizeY, szSize);
		}
	}
}

void CSettings::ShowFontErrorTip(LPCTSTR asInfo)
{
	ShowErrorTip(asInfo, gpSetCls->GetPage(thi_Fonts), tFontFace, gpFontMgr->szFontError, countof(gpFontMgr->szFontError),
	             gpSetCls->hwndBalloon, &gpSetCls->tiBalloon, gpSetCls->hwndTip, FAILED_FONT_TIMEOUT);
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
		gpConEmu->SetDefaultCmd(L"far");
	}
	else
	{
		wchar_t* pszComspec = GetComspec(&gpSet->ComSpec);
		_ASSERTE(pszComspec!=NULL);
		gpConEmu->SetDefaultCmd(pszComspec ? pszComspec : L"cmd");
		SafeFree(pszComspec);
	}
}

RecreateActionParm CSettings::GetDefaultCreateAction()
{
	return IsMulti() ? cra_CreateTab : cra_CreateWindow;
}

bool CSettings::IsMulti()
{
	if (!gpSet->mb_isMulti)
	{
		// "SingleInstance" has more weight
		if (!IsSingleInstanceArg())
			return false;
		// Otherwise we'll get infinite loop
	}
	return true;
}

bool CSettings::IsSingleInstanceArg()
{
	if (SingleInstanceArg == sgl_Enabled)
		return true;

	if ((SingleInstanceArg == sgl_Default) && (gpSet->isSingleInstance || gpSet->isQuakeStyle))
		return true;

	return false;
}

bool CSettings::ResetCmdHistory(HWND hParent)
{
	if (IDYES != MsgBox(L"Do you want to clear current history?\nThis can not be undone!",
			MB_ICONEXCLAMATION|MB_YESNO|MB_DEFBUTTON2, gpConEmu->GetDefaultTitle(), hParent ? hParent : ghWnd))
	{
		return false;
	}

	gpSet->HistoryReset();

	return true;
}

void CSettings::SetSaveCmdHistory(bool bSaveHistory)
{
	gpSet->isSaveCmdHistory = bSaveHistory;

	// И сразу сохранить в настройках
	SettingsBase* reg = gpSet->CreateSettings(NULL);
	if (!reg)
	{
		_ASSERTE(reg!=NULL);
		return;
	}

	if (reg->OpenKey(gpSetCls->GetConfigPath(), KEY_WRITE))
	{
		reg->Save(L"SaveCmdHistory", gpSet->isSaveCmdHistory);
		reg->CloseKey();
	}

	delete reg;
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





// Показать в "Инфо" текущий режим консоли
void CSettings::UpdateConsoleMode(CRealConsole* pRCon)
{
	HWND hInfoPg = GetPage(thi_Info);
	if (hInfoPg && IsWindow(hInfoPg))
	{
		WORD nConInMode = 0, nConOutMode = 0;
		TermEmulationType Term = te_win32;
		BOOL bBracketedPaste = FALSE;
		pRCon->GetConsoleModes(nConInMode, nConOutMode, Term, bBracketedPaste);
		CEActiveAppFlags appFlags = pRCon->GetActiveAppFlags();

		wchar_t szFlags[128] = L"";
		switch (Term)
		{
		case te_win32:
			wcscpy_c(szFlags, L"win32"); break;
		case te_xterm:
			wcscpy_c(szFlags, L"xterm"); break;
		default:
			msprintf(szFlags, countof(szFlags), L"term=%u", Term);
		}
		if (bBracketedPaste)
			wcscat_c(szFlags, L"|BrPaste");
		if (appFlags & caf_Cygwin1)
			wcscat_c(szFlags, L"|cygwin");
		if (appFlags & caf_Msys1)
			wcscat_c(szFlags, L"|msys");
		if (appFlags & caf_Msys2)
			wcscat_c(szFlags, L"|msys2");
		if (appFlags & caf_Clink)
			wcscat_c(szFlags, L"|clink");

		wchar_t szInfo[255];
		_wsprintf(szInfo, SKIPLEN(countof(szInfo))
			L"Console states (In=x%02X, Out=x%02X, %s)",
			nConInMode, nConOutMode, szFlags);
		SetDlgItemText(hInfoPg, IDC_CONSOLE_STATES, szInfo);
	}
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
	HWND hViewsPg = GetPage(thi_Views);
	if (hViewsPg && IsWindow(hViewsPg))
	{
		if (abAppear != IsWindowEnabled(GetDlgItem(hViewsPg, bApplyViewSettings)))
			EnableWindow(GetDlgItem(hViewsPg, bApplyViewSettings), abAppear);
	}
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

	// Иначе - по настройкам ConEmu
	#ifndef APPDISTINCTBACKGROUND
	if (!isBackgroundImageValid)
		return false;
	#else
	CBackgroundInfo* pBgObject = apVCon ? apVCon->GetBackgroundObject() : mp_BgInfo;
	bool bBgExist = (pBgObject && pBgObject->GetBgImgData() != NULL);
	SafeRelease(pBgObject);
	if (!bBgExist)
		return false;
	#endif

	if (apVCon && (apVCon->isEditor || apVCon->isViewer))
	{
		return (gpSet->isShowBgImage == 1);
	}
	else
	{
		return (gpSet->isShowBgImage != 0);
	}
}

void CSettings::SetBgImageDarker(u8 newValue, bool bUpdate)
{
	if (/*newV < 256*/ newValue != gpSet->bgImageDarker)
	{
		gpSet->bgImageDarker = newValue;

		HWND hBgPg = GetPage(thi_Backgr);
		if (hBgPg)
		{
			SendDlgItemMessage(hBgPg, slDarker, TBM_SETPOS, (WPARAM) true, (LPARAM) gpSet->bgImageDarker);

			TCHAR tmp[10];
			_wsprintf(tmp, SKIPLEN(countof(tmp)) L"%u", (UINT)gpSet->bgImageDarker);
			SetDlgItemText(hBgPg, tDarker, tmp);
		}

		if (bUpdate)
		{
			// Картинку может установить и плагин
			if (gpSet->isShowBgImage && gpSet->sBgImage[0])
				LoadBackgroundFile(gpSet->sBgImage);

			NeedBackgroundUpdate();

			gpConEmu->Update(true);
		}
	}
}

CBackgroundInfo* CSettings::GetBackgroundObject()
{
	if (mp_BgInfo)
		mp_BgInfo->AddRef();
	return mp_BgInfo;
}

bool CSettings::LoadBackgroundFile(TCHAR *inPath, bool abShowErrors)
{
	bool lRes = false;

#ifdef APPDISTINCTBACKGROUND
	CBackgroundInfo* pNew = CBackgroundInfo::CreateBackgroundObject(inPath, abShowErrors);
	SafeRelease(mp_BgInfo);
	mp_BgInfo = pNew;
	lRes = (mp_BgInfo != NULL);
#else
	//_ASSERTE(isMainThread());
	if (!inPath || _tcslen(inPath)>=MAX_PATH)
	{
		if (abShowErrors)
			MBoxA(L"Invalid 'inPath' in Settings::LoadImageFrom");

		return false;
	}

	_ASSERTE(isMainThread());
	BY_HANDLE_FILE_INFORMATION inf = {0};
	BITMAPFILEHEADER* pBkImgData = NULL;

	if (wcspbrk(inPath, L"%\\.") == NULL)
	{
		// May be "Solid color"
		COLORREF clr = (COLORREF)-1;
		if (::GetColorRef(inPath, &clr))
		{
			pBkImgData = CreateSolidImage(clr, 128, 128);
		}
	}

	if (!pBkImgData)
	{
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

		pBkImgData = LoadImageEx(exPath, inf);
	}

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
#endif
	return lRes;
}


void CSettings::NeedBackgroundUpdate()
{
	#ifndef APPDISTINCTBACKGROUND
	mb_NeedBgUpdate = TRUE;
	#else
	CVConGuard VCon;
	for (INT_PTR i = 0; CVConGroup::GetVCon(i, &VCon); i++)
	{
		VCon->NeedBackgroundUpdate();
	}
	#endif
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
		RECT rcControl = {};
		HWND hCtrl = nCtrlID ? GetDlgItem(hDlg, nCtrlID) : NULL;
		GetWindowRect(hCtrl ? hCtrl : hDlg, &rcControl);
		int ptx = bLeftAligh ? (rcControl.left + 10) : (rcControl.right - 10);
		int pty = bLeftAligh ? rcControl.bottom : (rcControl.top + rcControl.bottom) / 2;
		SendMessage(hBall, TTM_TRACKPOSITION, 0, MAKELONG(ptx,pty));
		SendMessage(hBall, TTM_TRACKACTIVATE, TRUE, (LPARAM)pti);
		SetTimer(hDlg, BALLOON_MSG_TIMERID, nTimeout/*FAILED_FONT_TIMEOUT*/, 0);
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
	INT_PTR nRc = CDynDialog::ExecuteDialog(IDD_MORE_CONFONT, hParent, EditConsoleFontProc, 0);
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
bool CSettings::CheckConsoleFontFast(LPCWSTR asCheckName /*= NULL*/)
{
	// В ReactOS шрифт не меняется и в реестре не регистрируется
	if (gpStartEnv->bIsReactOS)
	{
		return true;
	}

	//wchar_t szCreatedFaceName[32] = {0};
	LOGFONT LF = gpSet->ConsoleFont;
	BOOL bCheckStarted = FALSE;
	DWORD nCheckResult = -1;
	DWORD nCheckWait = -1;

	gpSetCls->nConFontError = 0; //ConFontErr_NonSystem|ConFontErr_NonRegistry|ConFontErr_InvalidName;
	if (asCheckName && *asCheckName)
		wcscpy_c(LF.lfFaceName, asCheckName);

	HFONT hf = CreateFontIndirect(&LF);

	if (!hf)
	{
		gpSetCls->nConFontError = ConFontErr_InvalidName;
	}
	else
	{
		LPOUTLINETEXTMETRICW lpOutl = gpFontMgr->LoadOutline(NULL, hf);

		if (!lpOutl)
		{
			gpSetCls->nConFontError = ConFontErr_InvalidName;
		}
		else
		{
			LPCWSTR pszFamilyName = (LPCWSTR)lpOutl->otmpFamilyName;

			// Интересуют только TrueType (вроде только для TTF доступен lpOutl - проверить
			if (pszFamilyName[0] != L'@'
			        && (gbIsDBCS || CFontMgr::IsAlmostMonospace(pszFamilyName, &lpOutl->otmTextMetrics, lpOutl /*lpOutl->otmTextMetrics.tmMaxCharWidth, lpOutl->otmTextMetrics.tmAveCharWidth, lpOutl->otmTextMetrics.tmHeight*/))
			        && lpOutl->otmPanoseNumber.bProportion == PAN_PROP_MONOSPACED
			        && lstrcmpi(pszFamilyName, LF.lfFaceName) == 0
			  )
			{
				BOOL lbNonSystem = FALSE;

				// Нельзя использовать шрифты, которые зарегистрированы нами (для ConEmu). Они должны быть системными
				//for (std::vector<RegFont>::iterator iter = gpSetCls->m_RegFonts.begin(); !lbNonSystem && iter != gpSetCls->m_RegFonts.end(); ++iter)
				for (INT_PTR j = 0; j < gpFontMgr->m_RegFonts.size(); ++j)
				{
					const RegFont* iter = &(gpFontMgr->m_RegFonts[j]);

					if (!iter->bAlreadyInSystem &&
					        lstrcmpi(iter->szFontName, LF.lfFaceName) == 0)
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
		DeleteObject(hf);
	}

	// Если успешно - проверить зарегистрированность в реестре
	if (gpSetCls->nConFontError == 0)
	{
		if (!CheckConsoleFontRegistry(LF.lfFaceName))
			gpSetCls->nConFontError |= ConFontErr_NonRegistry;
	}

	// WinPE may not have "Lucida Console" preinstalled
	if (gpSetCls->nConFontError && !asCheckName && gpStartEnv && (gpStartEnv->bIsWinPE == 1) && (lstrcmpi(LF.lfFaceName, gsAltConFont) != 0))
	{
		DWORD errSave = gpSetCls->nConFontError;
		if (CheckConsoleFontFast(gsAltConFont))
		{
			// But has "Courier New"
			wcscpy_c(gpSet->ConsoleFont.lfFaceName, gsAltConFont);
			goto wrap;
		}
		gpSetCls->nConFontError = errSave;
	}

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

wrap:
	bConsoleFontChecked = (gpSetCls->nConFontError == 0);
	if (gpSet->isLogging())
	{
		wchar_t szInfo[128];
		_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"CheckConsoleFontFast(`%s`,`%s`) = %u",
			asCheckName ? asCheckName : L"NULL", LF.lfFaceName, gpSetCls->nConFontError);
		LogString(szInfo);
	}
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
			INT_PTR nIdx = -1;
			while ((iRc = RegEnumValue(hk, idx++, szId, &cchName, NULL, &dwType, (LPBYTE)szFont, &dwLen)) == 0)
			{
				szId[min(countof(szId)-1,cchName)] = 0; szFont[min(countof(szFont)-1,dwLen/2)] = 0;
				if (*szFont)
				{
					LPCWSTR pszFaceName = (szFont[0] == L'*') ? (szFont+1) : szFont;
					if (SendDlgItemMessage(ahDlg, tConsoleFontFace, CB_FINDSTRINGEXACT, -1, (LPARAM) pszFaceName)==-1)
					{
						nIdx = SendDlgItemMessage(ahDlg, tConsoleFontFace, CB_ADDSTRING, 0, (LPARAM) pszFaceName); //-V103
					}
				}
				cchName = countof(szId); dwLen = sizeof(szFont)-2;
			}
			UNREFERENCED_PARAMETER(nIdx);
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
		if (CSetDlgLists::SelectString(ahDlg, tConsoleFontFace, gpSet->ConsoleFont.lfFaceName)<0)
			SetDlgItemText(ahDlg, tConsoleFontFace, gpSet->ConsoleFont.lfFaceName);
	}

	return (gpSetCls->nConFontError == 0);
}

INT_PTR CSettings::EditConsoleFontProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch (messg)
	{
		case WM_NOTIFY:
			{
				LPNMHDR phdr = (LPNMHDR)lParam;

				if (phdr->code == TTN_GETDISPINFO)
				{
					return gpSetCls->ProcessTipHelp(hWnd2, messg, wParam, lParam);
				}

				break;
			}
		case WM_INITDIALOG:
		{
			SendMessage(hWnd2, WM_SETICON, ICON_BIG, (LPARAM)hClassIcon);
			SendMessage(hWnd2, WM_SETICON, ICON_SMALL, (LPARAM)hClassIconSm);

			gpSetCls->hConFontDlg = NULL; // пока не выставим - на смену в контролах не реагировать
			wchar_t temp[10];
			const DWORD* pnSizesSmall = NULL;
			uint nCount = CSetDlgLists::GetListItems(CSetDlgLists::eFSizesSmall, pnSizesSmall);
			for (uint i = 0; i < nCount; i++)
			{
				_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", pnSizesSmall[i]);
				SendDlgItemMessage(hWnd2, tConsoleFontSizeY, CB_ADDSTRING, 0, (LPARAM) temp);
				_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", (int)(pnSizesSmall[i]*3/2));
				SendDlgItemMessage(hWnd2, tConsoleFontSizeX, CB_ADDSTRING, 0, (LPARAM) temp);

				if ((LONG)pnSizesSmall[i] >= gpFontMgr->LogFont.lfHeight)
					break; // не допускаются шрифты больше, чем выбрано для основного шрифта!
			}

			_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", gpSet->ConsoleFont.lfHeight);
			CSetDlgLists::SelectStringExact(hWnd2, tConsoleFontSizeY, temp);
			_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", gpSet->ConsoleFont.lfWidth);
			CSetDlgLists::SelectStringExact(hWnd2, tConsoleFontSizeX, temp);

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

			if (wParam == BALLOON_MSG_TIMERID)
			{
				KillTimer(hWnd2, BALLOON_MSG_TIMERID);
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
						MsgBox(gpSetCls->sConFontError[0] ? gpSetCls->sConFontError : gpSetCls->CreateConFontError(NULL,NULL),
							MB_OK|MB_ICONSTOP, gpConEmu->GetDefaultTitle(), hWnd2);
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
						wchar_t szWorkDir[MAX_PATH+1];
						wcscpy_c(szWorkDir, gpConEmu->WorkDir());
						sei.lpDirectory = szWorkDir;
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

								if (CSetDlgLists::SelectString(hWnd2, tConsoleFontFace, gpSet->ConsoleFont.lfFaceName)<0)
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
						LPOUTLINETEXTMETRICW lpOutl = gpFontMgr->LoadOutline(NULL, hf);

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
								CSetDlgLists::SelectStringExact(hWnd2, tConsoleFontSizeX, temp);
							}

							if (lpOutl->otmTextMetrics.tmHeight != LF.lfHeight)
							{
								_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", lpOutl->otmTextMetrics.tmHeight);
								CSetDlgLists::SelectStringExact(hWnd2, tConsoleFontSizeY, temp);
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
	//	SetTimer(hConFontDlg, BALLOON_MSG_TIMERID, FAILED_FONT_TIMEOUT, 0);
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
	//for (std::vector<RegFont>::iterator iter = gpSetCls->m_RegFonts.begin(); iter != gpSetCls->m_RegFonts.end(); ++iter)
	for (INT_PTR j = 0; j < gpFontMgr->m_RegFonts.size(); ++j)
	{
		const RegFont* iter = &(gpFontMgr->m_RegFonts[j]);

		if (!iter->bAlreadyInSystem &&
		        lstrcmpi(iter->szFontName, lplf->lfFaceName) == 0)
		{
			return TRUE;
		}
	}

	// PAN_PROP_MONOSPACED - не дает правильного результата. Например 'MS Mincho' заявлен как моноширинный,
	// но совсем таковым не является. Кириллица у него дофига какая...
	// И только моноширинные!
	DWORD bAlmostMonospace = gpFontMgr->IsAlmostMonospace(lplf->lfFaceName, (LPTEXTMETRIC)lpntm /*lpntm->tmMaxCharWidth, lpntm->tmAveCharWidth, lpntm->tmHeight*/) ? 1 : 0;

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

	LPOUTLINETEXTMETRICW lpOutl = gpFontMgr->LoadOutline(NULL, hf);

	if (!lpOutl) return TRUE;  // Ошибка получения параметров шрифта

	if (lpOutl->otmPanoseNumber.bProportion != PAN_PROP_MONOSPACED  // шрифт не заявлен как моноширинный
	        || lstrcmpi((wchar_t*)lpOutl->otmpFamilyName, LF.lfFaceName)) // или алиас
	{
		free(lpOutl);
		return TRUE; // следующий шрифт
	}

	free(lpOutl); lpOutl = NULL;

	// Сравниваем с текущим, выбранным в настройке
	if (lstrcmpi(LF.lfFaceName, gpSet->ConsoleFont.lfFaceName) == 0)
		gpSetCls->nConFontError &= ~(DWORD)ConFontErr_NonSystem;

	INT_PTR nIdx = -1;
	if (hWnd2)
	{
		if (SendDlgItemMessage(hWnd2, tConsoleFontFace, CB_FINDSTRINGEXACT, -1, (LPARAM) LF.lfFaceName)==-1)
		{
			nIdx = SendDlgItemMessage(hWnd2, tConsoleFontFace, CB_ADDSTRING, 0, (LPARAM) LF.lfFaceName); //-V103
		}
	}
	UNREFERENCED_PARAMETER(nIdx);

	if (gpSetCls->sDefaultConFontName[0] == 0)
	{
		if (CheckConsoleFontRegistry(LF.lfFaceName))
			wcscpy_c(gpSetCls->sDefaultConFontName, LF.lfFaceName);
	}

	MCHKHEAP
	return TRUE;
	UNREFERENCED_PARAMETER(lpntm);
}

bool CSettings::isDialogMessage(MSG &Msg)
{
	if (((Msg.message == WM_KEYDOWN) || (Msg.message == WM_KEYUP))
		&& ((Msg.wParam == VK_UP) || (Msg.wParam == VK_DOWN)))
	{
		// Is it button? WM_GETDLGCODE?
		wchar_t szClass[64] = L""; GetClassName(Msg.hwnd, szClass, countof(szClass));
		if (wcscmp(szClass, L"Button") == 0)
		{
			CSetPgBase* pg = GetActivePageObj();
			if (pg && pg->SelectNextItem((Msg.wParam == VK_DOWN), (Msg.message == WM_KEYDOWN)))
			{
				return true;
			}
		}
	}

	if (IsDialogMessage(ghOpWnd, &Msg))
	{
		return true;
	}

	if (mp_HelpPopup && mp_HelpPopup->mh_Popup)
	{
		if (IsDialogMessage(mp_HelpPopup->mh_Popup, &Msg))
		{
			return true;
		}
	}

	return false;
}

const ConEmuSetupPages* CSettings::GetPageData(TabHwndIndex nPage)
{
	if (!m_Pages)
	{
		_ASSERTE(m_Pages!=NULL && "Pages list must be initialized already!");
		return NULL;
	}

	const ConEmuSetupPages* pData = NULL;

	_ASSERTE((thi_Fonts == (TabHwndIndex)0) && (nPage >= thi_Fonts) && (nPage < thi_Last));

	if ((nPage >= thi_Fonts) && (nPage < thi_Last))
	{
		if (m_Pages[nPage].PageIndex == nPage)
		{
			pData = &(m_Pages[nPage]);
		}
		else
		{
			_ASSERTE(m_Pages[nPage].PageIndex == nPage);

			for (const ConEmuSetupPages* p = m_Pages; p->DialogID; p++)
			{
				if (p->PageIndex == nPage)
				{
					pData = p;
					break;
				}
			}
		}
	}

	if (!pData)
	{
		_ASSERTE(FALSE && "Invalid nPage identifier!");
	}

	return pData;
}

TabHwndIndex CSettings::GetPageIdByDialogId(UINT DialogID)
{
	TabHwndIndex pageId = thi_Last;

	if (!ghOpWnd || !m_Pages)
	{
		_ASSERTE(ghOpWnd && m_Pages);
	}
	else
	{
		for (const ConEmuSetupPages* p = m_Pages; p->DialogID; p++)
		{
			if (p->DialogID == DialogID)
			{
				pageId = p->PageIndex;
				break;
			}
		}
	}

	return pageId;
}

HWND CSettings::GetActivePage()
{
	const ConEmuSetupPages* p = (m_LastActivePageId != thi_Last) ? GetPageData(m_LastActivePageId) : NULL;
	if (!p)
		return NULL;
	if (!p->hPage || !IsWindowVisible(p->hPage))
		return NULL;
	return p->hPage;
}

CSetPgBase* CSettings::GetActivePageObj()
{
	const ConEmuSetupPages* p = (m_LastActivePageId != thi_Last) ? GetPageData(m_LastActivePageId) : NULL;
	if (!p)
		return NULL;
	if (!p->hPage || !IsWindowVisible(p->hPage))
		return NULL;
	return p->pPage;
}

HWND CSettings::GetPage(TabHwndIndex nPage)
{
	const ConEmuSetupPages* p = GetPageData(nPage);
	if (!p)
		return NULL;
	return p->hPage;
}

CSetPgBase* CSettings::GetPageObj(TabHwndIndex nPage)
{
	const ConEmuSetupPages* p = GetPageData(nPage);
	if (!p)
		return NULL;
	return p->pPage;
}

TabHwndIndex CSettings::GetPageId(HWND hPage, ConEmuSetupPages** pp)
{
	TabHwndIndex pgId = thi_Last;

	if (ghOpWnd && m_Pages && hPage)
	{
		for (ConEmuSetupPages* p = m_Pages; p->DialogID; p++)
		{
			if (p->hPage == hPage)
			{
				pgId = p->PageIndex;
				if (pp)
					*pp = p;
				break;
			}
		}
	}

	return pgId;
}

TabHwndIndex CSettings::GetPageId(HWND hPage)
{
	return GetPageId(hPage, NULL);
}

void CSettings::ClearPages()
{
	if (!m_Pages)
	{
		_ASSERTE(m_Pages);
		return;
	}

	for (ConEmuSetupPages *p = m_Pages; p->DialogID; p++)
	{
		SafeDelete(p->pPage);
		p->hPage = NULL;
	}

	if (mp_DpiAware)
		mp_DpiAware->Detach();

	SafeDelete(mp_ImgBtn);
}

int CSettings::GetDialogDpi()
{
	if (mp_DpiAware)
		return mp_DpiAware->GetCurDpi().Ydpi;
	return _dpi.Ydpi;
}
