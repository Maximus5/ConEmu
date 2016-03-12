
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
#include <commctrl.h>
#pragma warning(disable: 4091)
#include <shlobj.h>
#pragma warning(default: 4091)

#ifdef __GNUC__
#include "ShObjIdl_Part.h"
#endif // __GNUC__

#include "../common/Monitors.h"
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
#include "SetColorPalette.h"
#include "SetCmdTask.h"
#include "SetDlgLists.h"
#include "Status.h"
#include "TabBar.h"
#include "TrayIcon.h"
#include "VConChild.h"
#include "VConGroup.h"
#include "version.h"
#include "VirtualConsole.h"

//#define CONEMU_ROOT_KEY L"Software\\ConEmu"

#define CONEMU_HERE_CMD  L"{cmd} -cur_console:n"
#define CONEMU_HERE_POSH L"{powershell} -cur_console:n"

#define DEBUGSTRDPI(s) DEBUGSTR(s)

#define COUNTER_REFRESH 5000


#define BALLOON_MSG_TIMERID 101
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
	isAdvLogging = 0;
	m_ActivityLoggingType = glt_None; mn_ActivityCmdStartTick = 0;
	bForceBufferHeight = false; nForceBufferHeight = 1000; /* устанавливается в true, из ком.строки /BufferHeight */

	#ifdef SHOW_AUTOSCROLL
	AutoScroll = true;
	#endif

	// Шрифты
	//memset(m_Fonts, 0, sizeof(m_Fonts));
	//TODO: OLD - на переделку
	mn_FontZoomValue = FontZoom100;
	memset(&LogFont, 0, sizeof(LogFont));
	memset(&LogFont2, 0, sizeof(LogFont2));
	mn_FontWidth = mn_BorderFontWidth = 0; mn_FontHeight = 16; // сброшено будет в SettingsLoaded
	//gpSet->isFontAutoSize = false;
	mb_Name1Ok = mb_Name2Ok = false;
	mn_AutoFontWidth = mn_AutoFontHeight = -1;
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
	mb_StopRegisterFonts = FALSE;
	mb_IgnoreEditChanged = FALSE;
	gpSet->mb_CharSetWasSet = FALSE;
	mb_TabHotKeyRegistered = FALSE;
	//hMain = hExt = hFar = hTabs = hKeys = hColors = hCmdTasks = hViews = hInfo = hDebug = hUpdate = hSelection = NULL;
	mn_LastActivedTab = 0;
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
	mp_BgInfo = NULL;
	#endif
	m_ColorFormat = eRgbDec; // RRR GGG BBB (как показывать цвета на вкладке Colors)
	mp_DpiAware = NULL;
	mp_ImgBtn = NULL;
	mp_DlgDistinct2 = NULL;
	mp_DpiDistinct2 = NULL;
	ZeroStruct(mh_Font);
	mh_Font2 = NULL;
	ZeroStruct(m_tm);
	ZeroStruct(m_otm);
	ResetFontWidth();
#if 0
	//120714 - аналогичные параметры работают в ConEmuC.exe, а в GUI они и не работали. убрал пока
	nAttachPID = 0; hAttachConWnd = NULL;
#endif
	szFontError[0] = 0;
	nConFontError = 0;
	memset(&tiBalloon, 0, sizeof(tiBalloon));
	//mn_FadeMul = gpSet->mn_FadeHigh - gpSet->mn_FadeLow;
	//gpSet->mn_LastFadeSrc = gpSet->mn_LastFadeDst = -1;

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

	mn_MsgUpdateCounter = RegisterWindowMessage(L"ConEmuSettings::Counter"); mb_MsgUpdateCounter = FALSE;
	mn_MsgRecreateFont = RegisterWindowMessage(L"Settings::RecreateFont");
	mn_MsgLoadFontFromMain = RegisterWindowMessage(L"Settings::LoadFontNames");
	mn_ActivateTabMsg = RegisterWindowMessage(L"Settings::ActivateTab");

	mp_HelpPopup = new CEHelpPopup;

	mp_ActiveHotKey = NULL;

	// Горячие клавиши
	InitVars_Hotkeys();

	mp_Dialog = NULL;
	m_Pages = NULL;
	mn_PagesCount = 0;

	memset(&tiConFontBalloon, 0, sizeof(tiConFontBalloon));
	mf_MarkCopyPreviewProc = NULL;
	mb_IgnoreSelPage = false;

	// Вкладки-диалоги
	InitVars_Pages();
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

// Till now prcSuggested is not used
// However it may be needed for 'Auto sized' fonts
bool CSettings::RecreateFontByDpi(int dpiX, int dpiY, LPRECT prcSuggested)
{
	if ((_dpi.Xdpi == dpiX && _dpi.Ydpi == dpiY) || (dpiY < 72) || (dpiY > 960))
	{
		_ASSERTE(dpiY >= 72 && dpiY <= 960);
		return false;
	}

	_dpi.SetDpi(dpiX, dpiY);
	//Raster fonts???
	EvalLogfontSizes(LogFont, gpSet->FontSizeY, gpSet->FontSizeX);
	RecreateFont(-1);

	return true;
}

void CSettings::ReleaseHotkeys()
{
	for (int i = m_HotKeys.size() - 1; i >= 0; i--)
	{
		SafeFree(m_HotKeys[i].GuiMacro);
	}
	m_HotKeys.clear();
}

void CSettings::InitVars_Hotkeys()
{
	ReleaseHotkeys();

	// Горячие клавиши (умолчания)
	m_HotKeys.AllocateHotkeys();

	mp_ActiveHotKey = NULL;
}

// Called when user change hotkey or modifiers in Settings dialog
void CSettings::SetHotkeyVkMod(ConEmuHotKey *pHK, DWORD VkMod)
{
	if (!pHK)
	{
		_ASSERTE(pHK!=NULL);
		return;
	}

	// Usually, this is equal to "mp_ActiveHotKey->VkMod = VkMod"
	pHK->SetVkMod(VkMod);

	// Global? Need to re-register?
	if (pHK->HkType == chk_Local)
	{
		gpConEmu->GlobalHotKeyChanged();
	}
}

const ConEmuHotKey* CSettings::GetHotKeyPtr(int idx)
{
	const ConEmuHotKey* pHK = NULL;

	if (idx >= 0 && this)
	{
		if (idx < m_HotKeys.size())
		{
			pHK = &(m_HotKeys[idx]);
		}
		else
		{
			int iHotkeys = m_HotKeys.size();
			const CommandTasks* pCmd = gpSet->CmdTaskGet(idx - iHotkeys);
			if (pCmd)
			{
				_ASSERTE(pCmd->HotKey.HkType==chk_Task && pCmd->HotKey.GetTaskIndex()==(idx-iHotkeys));
				pHK = &pCmd->HotKey;
			}
		}
	}

	return pHK;
}

// pRCon may be NULL
const ConEmuHotKey* CSettings::GetHotKeyInfo(const ConEmuChord& VkState, bool bKeyDown, CRealConsole* pRCon)
{
	// На сами модификаторы - действий не вешается
	switch (VkState.Vk)
	{
	case VK_LWIN: case VK_RWIN:
	case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:
	case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
	case VK_MENU: case VK_LMENU: case VK_RMENU:
		return NULL;
	case 0:
		_ASSERTE(VkState.Vk!=0);
		return NULL;
	}

	const ConEmuHotKey* p = NULL;

	// Теперь бежим по mp_HotKeys и сравниваем требуемые модификаторы
	for (int i = 0;; i++)
	{
		const ConEmuHotKey* pi = GetHotKeyPtr(i);
		if (!pi)
			break;

		if (pi->HkType == chk_Modifier)
			continue;

		// Hotkey was not set in settings?
		if (!pi->Key.Vk)
			continue;

		// May be disabled by settings or context?
		if (pi->Enabled)
		{
			if (!pi->Enabled())
				continue;
		}

		// Do compare (chord keys are planned)
		if (!pi->Key.IsEqual(VkState))
			continue;

		// The function
		if (pi->fkey)
		{
			// Допускается ли этот хоткей в текущем контексте?
			if (pi->fkey(VkState, true, pi, pRCon))
			{
				p = pi;
				break; // Нашли
			}
		}
		else
		{
			// Хоткей должен знать, что он "делает"
			_ASSERTE(pi->fkey!=NULL);
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
			const ConEmuHotKey* pHK = GetHotKeyPtr(i);
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
		{IDD_SPG_MAIN,        0, L"Main",           thi_Main       /* OnInitDialog_Main */},
		{IDD_SPG_WNDSIZEPOS,  1, L"Size & Pos",     thi_SizePos    /* OnInitDialog_WndSizePos */},
		{IDD_SPG_SHOW,        1, L"Appearance",     thi_Show       /* OnInitDialog_Show */},
		{IDD_SPG_BACK,        1, L"Background",     thi_Backgr     /* OnInitDialog_Background */},
		{IDD_SPG_TABS,        1, L"Tab bar",        thi_Tabs       /* OnInitDialog_Tabs */},
		{IDD_SPG_CONFIRM,     1, L"Confirm",        thi_Confirm    /* OnInitDialog_Confirm */},
		{IDD_SPG_TASKBAR,     1, L"Task bar",       thi_Taskbar    /* OnInitDialog_Taskbar */},
		{IDD_SPG_UPDATE,      1, L"Update",         thi_Update     /* OnInitDialog_Update */},
		{IDD_SPG_STARTUP,     0, L"Startup",        thi_Startup    /* OnInitDialog_Startup */},
		{IDD_SPG_CMDTASKS,    1, L"Tasks",          thi_Tasks      /* OnInitDialog_CmdTasks */},
		{IDD_SPG_COMSPEC,     1, L"ComSpec",        thi_Comspec    /* OnInitDialog_Comspec */},
		{IDD_SPG_ENVIRONMENT, 1, L"Environment",    thi_Environment/* OnInitDialog_Environment */},
		{IDD_SPG_FEATURE,     0, L"Features",       thi_Ext        /* OnInitDialog_Ext */},
		{IDD_SPG_CURSOR,      1, L"Text cursor",    thi_Cursor     /* OnInitDialog_Cursor */},
		{IDD_SPG_COLORS,      1, L"Colors",         thi_Colors     /* OnInitDialog_Color */},
		{IDD_SPG_TRANSPARENT, 1, L"Transparency",   thi_Transparent/* OnInitDialog_Transparent */},
		{IDD_SPG_STATUSBAR,   1, L"Status bar",     thi_Status     /* OnInitDialog_Status */},
		{IDD_SPG_APPDISTINCT, 1, L"App distinct",   thi_Apps       /* OnInitDialog_CmdTasks */},
		{IDD_SPG_INTEGRATION, 0, L"Integration",    thi_Integr     /* OnInitDialog_Integr */},
		{IDD_SPG_DEFTERM,     1, L"Default term",   thi_DefTerm    /* OnInitDialog_DefTerm */},
		{IDD_SPG_KEYS,        0, L"Keys & Macro",   thi_Keys       /* OnInitDialog_Keys */},
		{IDD_SPG_CONTROL,     1, L"Controls",       thi_KeybMouse  /* OnInitDialog_Control */},
		{IDD_SPG_MARKCOPY,    1, L"Mark/Copy",      thi_MarkCopy   /* OnInitDialog_MarkCopy */},
		{IDD_SPG_PASTE,       1, L"Paste",          thi_Paste      /* OnInitDialog_Paste */},
		{IDD_SPG_HIGHLIGHT,   1, L"Highlight",      thi_Hilight    /* OnInitDialog_Hilight */},
		{IDD_SPG_FEATURE_FAR, 0, L"Far Manager",    thi_Far        /* OnInitDialog_Far */, true/*Collapsed*/},
		{IDD_SPG_FARMACRO,    1, L"Far macros",     thi_FarMacro   /* OnInitDialog_FarMacro */},
		{IDD_SPG_VIEWS,       1, L"Views",          thi_Views      /* OnInitDialog_Views */},
		{IDD_SPG_INFO,        0, L"Info",           thi_Info       /* OnInitDialog_Info */, RELEASEDEBUGTEST(true,false)/*Collapsed in Release*/},
		{IDD_SPG_DEBUG,       1, L"Debug",          thi_Debug      /* OnInitDialog_Debug */},
		// End
		{},
	};

	mn_PagesCount = ((int)countof(Pages) - 1);
	_ASSERTE(mn_PagesCount>0 && Pages[mn_PagesCount].PageID==0);

	m_Pages = (ConEmuSetupPages*)malloc(sizeof(Pages));
	memmove(m_Pages, Pages, sizeof(Pages));
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

	for (int i=0; i<MAX_FONT_STYLES; i++)
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

	ReleaseHotkeys();
	mp_ActiveHotKey = NULL;

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
		UnregisterShellInvalids();
	}

	// Передернуть палитру затенения
	gpSet->ResetFadeColors();
	gpSet->GetColors(-1, TRUE);


	// Назначить корректные флаги для кнопок Win+Number и Win+Arrow
	m_HotKeys.UpdateNumberModifier();
	m_HotKeys.UpdateArrowModifier();


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

	// Инициализация кастомной палитры для диалога выбора цвета
	memmove(acrCustClr, gpSet->Colors, sizeof(COLORREF)*16);

	/*
	LogFont.lfHeight = mn_FontHeight = gpSet->FontSizeY;
	LogFont.lfWidth = mn_FontWidth = gpSet->FontSizeX;
	*/
	EvalLogfontSizes(LogFont, gpSet->FontSizeY, gpSet->FontSizeX);
	lstrcpyn(LogFont.lfFaceName, gpSet->inFont, countof(LogFont.lfFaceName));
	lstrcpyn(LogFont2.lfFaceName, gpSet->inFont2, countof(LogFont2.lfFaceName));
	LogFont.lfQuality = gpSet->mn_AntiAlias;
	LogFont.lfWeight = gpSet->isBold ? FW_BOLD : FW_NORMAL;
	LogFont.lfCharSet = (BYTE)gpSet->mn_LoadFontCharSet;
	LogFont.lfItalic = gpSet->isItalic;

	if (slfFlags & slf_OnResetReload)
	{
		// Шрифт пере-создать сразу, его характеристики используются при ресайзе окна
		RecreateFont((WORD)-1);
	}
	else
	{
		_ASSERTE(ghWnd==NULL);
	}

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
		lstrcpyn(gpSet->inFont, LogFont.lfFaceName, countof(gpSet->inFont));
		lstrcpyn(gpSet->inFont2, LogFont2.lfFaceName, countof(gpSet->inFont2));
		#if 0
		// was #ifdef UPDATE_FONTSIZE_RECREATE
		gpSet->FontSizeY = LogFont.lfHeight;
		#endif
		gpSet->mn_LoadFontCharSet = LogFont.lfCharSet;
		gpSet->mn_AntiAlias = LogFont.lfQuality;
		gpSet->isBold = (LogFont.lfWeight >= FW_BOLD);
		gpSet->isItalic = (LogFont.lfItalic != 0);
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


// Сюда приходят значения из gpSet->FontSize[Y|X] или заданные пользователем через макрос
// Функция должна учесть:
// * текущий dpi (FontUseDpi);
// * масштаб (пока нету);
// * FontUseUnits (положительный или отрицательный LF.lfHeight)
void CSettings::EvalLogfontSizes(LOGFONT& LF, LONG lfHeight, LONG lfWidth)
{
	if (lfHeight == 0)
	{
		_ASSERTE(lfHeight != 0);
		lfHeight = gpSet->FontUseUnits ? 12 : 16;
	}

	LF.lfHeight = EvalSize(lfHeight, esf_Vertical|esf_CanUseUnits|esf_CanUseDpi|esf_CanUseZoom);
	LF.lfWidth  = lfWidth ? EvalSize(lfWidth, esf_Horizontal|esf_CanUseDpi|esf_CanUseZoom) : 0;
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
		if (mn_FontZoomValue && (mn_FontZoomValue != FontZoom100))
		{
			iMul *= mn_FontZoomValue;
			iDiv *= FontZoom100;
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

// Do NOT take into account current zoom value here!
// This must not be used for BDF or raster fonts, the result may be wrong.
LONG CSettings::EvalFontHeight(LPCWSTR lfFaceName, LONG lfHeight, BYTE nFontCharSet)
{
	if (!lfHeight || !*lfFaceName)
	{
		_ASSERTE(lfHeight != 0);
		return 0;
	}

	LONG CellHeight = 0;
	for (INT_PTR i = 0; i < m_FontHeights.size(); i++)
	{
		const FontHeightInfo& f = m_FontHeights[i];
		if ((f.lfHeight != lfHeight) || (f.lfCharSet != nFontCharSet))
			continue;
		if (lstrcmp(lfFaceName, f.lfFaceName) != 0)
			continue;
		CellHeight = f.CellHeight;
		break;
	}

	if (!CellHeight)
	{
		FontHeightInfo fi = {};
		TEXTMETRIC tm = {};
		SIZE sz = {};
		LOGFONT lf = LogFont;
		lstrcpyn(lf.lfFaceName, lfFaceName, countof(lf.lfFaceName));
		lstrcpyn(fi.lfFaceName, lfFaceName, countof(fi.lfFaceName));
		fi.lfHeight = lf.lfHeight = lfHeight;
		lf.lfWidth = 0;
		fi.lfCharSet = lf.lfCharSet = nFontCharSet;
		lf.lfPitchAndFamily = DEFAULT_PITCH | FF_MODERN;

		HDC hdc = CreateCompatibleDC(NULL);
		if (hdc)
		{
			HFONT hOld, f = CreateFontIndirect(&lf);
			if (f)
			{
				hOld = (HFONT)SelectObject(hdc, f);
				if (GetTextMetrics(hdc, &tm) && (tm.tmHeight > 0))
				{
					CellHeight = tm.tmHeight + tm.tmExternalLeading;
				}
				else if (GetTextExtentPoint(hdc, L"Yy", 2, &sz) && (sz.cy > 0))
				{
					CellHeight = sz.cy;
				}
				SelectObject(hdc, hOld);
				DeleteObject(f);
			}
			DeleteDC(hdc);
		}

		if (!CellHeight)
		{
			// Still unknown?
			_ASSERTE(CellHeight != 0);
			CellHeight = abs(lfHeight);
		}

		fi.CellHeight = CellHeight;
		m_FontHeights.push_back(fi);
	}

	return CellHeight;
}

LONG CSettings::EvalCellWidth()
{
	return (LONG)gpSet->FontSizeX3;
}

// в процентах (false) или mn_FontZoomValue (true)
LONG CSettings::GetZoom(bool bRaw /*= false*/)
{
	return bRaw ? mn_FontZoomValue : MulDiv(mn_FontZoomValue, 100, FontZoom100);
}


void CSettings::InitFont(LPCWSTR asFontName/*=NULL*/, int anFontHeight/*=-1*/, int anQuality/*=-1*/)
{
	lstrcpyn(LogFont.lfFaceName, (asFontName && *asFontName) ? asFontName : (*gpSet->inFont) ? gpSet->inFont : gsDefGuiFont, countof(LogFont.lfFaceName));
	if ((asFontName && *asFontName) || *gpSet->inFont)
		mb_Name1Ok = TRUE;

	if (anFontHeight && (anFontHeight != -1))
	{
		/*
		LogFont.lfHeight = mn_FontHeight = anFontHeight;
		LogFont.lfWidth = mn_FontWidth = 0;
		*/
		EvalLogfontSizes(LogFont, anFontHeight, 0);
	}
	else
	{
		/*
		LogFont.lfHeight = mn_FontHeight = gpSet->FontSizeY;
		LogFont.lfWidth = mn_FontWidth = gpSet->FontSizeX;
		*/
		EvalLogfontSizes(LogFont, gpSet->FontSizeY, gpSet->FontSizeX);
	}

	_ASSERTE(anQuality==-1 || anQuality==NONANTIALIASED_QUALITY || anQuality==ANTIALIASED_QUALITY || anQuality==CLEARTYPE_NATURAL_QUALITY);

	_ASSERTE(gpSet->mn_AntiAlias==NONANTIALIASED_QUALITY || gpSet->mn_AntiAlias==ANTIALIASED_QUALITY || gpSet->mn_AntiAlias==CLEARTYPE_NATURAL_QUALITY);
	LogFont.lfQuality = (anQuality!=-1) ? anQuality : gpSet->mn_AntiAlias;
	LogFont.lfWeight = gpSet->isBold ? FW_BOLD : FW_NORMAL;
	LogFont.lfCharSet = gpSet->mn_LoadFontCharSet;
	LogFont.lfItalic = gpSet->isItalic;

	lstrcpyn(LogFont2.lfFaceName, (*gpSet->inFont2) ? gpSet->inFont2 : gsDefGuiFont, countof(LogFont2.lfFaceName));
	if (*gpSet->inFont2)
		mb_Name2Ok = TRUE;

	//std::vector<RegFont>::iterator iter;

	if (!mb_Name1Ok)
	{
		for (int i = 0; !mb_Name1Ok && (i < 3); i++)
		{
			//for (iter = m_RegFonts.begin(); iter != m_RegFonts.end(); ++iter)
			for (INT_PTR j = 0; j < m_RegFonts.size(); ++j)
			{
				const RegFont* iter = &(m_RegFonts[j]);

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
		//for (iter = m_RegFonts.begin(); iter != m_RegFonts.end(); ++iter)
		for (INT_PTR j = 0; j < m_RegFonts.size(); ++j)
		{
			const RegFont* iter = &(m_RegFonts[j]);

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
	SaveFontSizes((mn_AutoFontWidth == -1), false);

	MCHKHEAP
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

	for (i = 0; m_Pages[i].PageID; i++)
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

		for (i = iFrom; (i < iTo) && m_Pages[i].PageID; i++)
		{
			if (m_Pages[i].hPage == NULL)
			{
				CreatePage(&(m_Pages[i]));
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

void CSettings::InvalidateCtrl(HWND hCtrl, BOOL bErase)
{
	::InvalidateRect(hCtrl, NULL, bErase);
}

LRESULT CSettings::OnInitDialog()
{
	//_ASSERTE(!hMain && !hColors && !hCmdTasks && !hViews && !hExt && !hFar && !hInfo && !hDebug && !hUpdate && !hSelection);
	//hMain = hExt = hFar = hTabs = hKeys = hViews = hColors = hCmdTasks = hInfo = hDebug = hUpdate = hSelection = NULL;
	_ASSERTE(m_Pages && (m_Pages[0].PageIndex==thi_Main) && !m_Pages[0].hPage /*...*/);
	ClearPages();

	_ASSERTE(mp_ImgBtn==NULL);
	SafeDelete(mp_ImgBtn);
	mp_ImgBtn = new CImgButtons(ghOpWnd, tOptionSearch, bSaveSettings);
	mp_ImgBtn->AddDonateButtons();

	if (mp_DpiAware)
	{
		mp_DpiAware->Attach(ghOpWnd, ghWnd, mp_Dialog);
	}

	gbLastColorsOk = FALSE;

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
	mp_ActiveHotKey = NULL;
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
			SetDlgItemText(ghOpWnd, bSaveSettings, L"Basic settings");
		}
	}

	if (isResetBasicSettings)
	{
		SetDlgItemText(ghOpWnd, tStorage, L"<Basic settings>");
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
		for (size_t i = 0; m_Pages[i].PageID; i++)
		{
			if ((m_Pages[i].PageID == IDD_SPG_UPDATE) && !gpConEmu->isUpdateAllowed())
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

		CreatePage(&(m_Pages[0]));

		apiShowWindow(m_Pages[0].hPage, SW_SHOW);
	}
	MCHKHEAP
	{
		CDpiAware::CenterDialog(ghOpWnd);
	}
	return 0;
}

LRESULT CSettings::OnInitDialog_Main(HWND hWnd2, bool abInitial)
{
	SetDlgItemText(hWnd2, tFontFace, LogFont.lfFaceName);
	SetDlgItemText(hWnd2, tFontFace2, LogFont2.lfFaceName);

	if (abInitial)
	{
		// Добавить шрифты рисованные ConEmu
		for (INT_PTR j = 0; j < m_RegFonts.size(); ++j)
		{
			const RegFont* iter = &(m_RegFonts[j]);

			if (iter->pCustom)
			{
				BOOL bMono = iter->pCustom->GetFont(0,0,0,0)->IsMonospace();

				int nIdx = SendDlgItemMessage(hWnd2, tFontFace, CB_ADDSTRING, 0, (LPARAM)iter->szFontName);
				SendDlgItemMessage(hWnd2, tFontFace, CB_SETITEMDATA, nIdx, bMono ? 1 : 0);

				nIdx = SendDlgItemMessage(hWnd2, tFontFace2, CB_ADDSTRING, 0, (LPARAM)iter->szFontName);
				SendDlgItemMessage(hWnd2, tFontFace2, CB_SETITEMDATA, nIdx, bMono ? 1 : 0);
			}
		}

		CSetDlgFonts::StartEnumFontsThread();

		CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, tFontSizeY), CSetDlgLists::eFSizesY, gpSet->FontSizeY, true);

		CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, tFontSizeX), CSetDlgLists::eFSizesX, gpSet->FontSizeX, true);
		CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, tFontSizeX2), CSetDlgLists::eFSizesX, gpSet->FontSizeX2, true);
		CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, tFontSizeX3), CSetDlgLists::eFSizesX, gpSet->FontSizeX3, true);

		CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbExtendFontBoldIdx), CSetDlgLists::eColorIdx, gpSet->AppStd.nFontBoldColor, false);
		CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbExtendFontItalicIdx), CSetDlgLists::eColorIdx, gpSet->AppStd.nFontItalicColor, false);
		CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbExtendFontNormalIdx), CSetDlgLists::eColorIdx, gpSet->AppStd.nFontNormalColor, false);

		CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, tFontCharset), CSetDlgLists::eCharSets, LogFont.lfCharSet, false);
	}
	else
	{
		//TODO: Обновить значения в списках?
	}

	checkDlgButton(hWnd2, cbExtendFonts, gpSet->AppStd.isExtendFonts);
	CSetDlgLists::EnableDlgItems(hWnd2, CSetDlgLists::eExtendFonts, gpSet->AppStd.isExtendFonts);

	checkDlgButton(hWnd2, cbFontAuto, gpSet->isFontAutoSize);

	MCHKHEAP

	checkRadioButton(hWnd2, rNoneAA, rCTAA,
		(LogFont.lfQuality == CLEARTYPE_NATURAL_QUALITY) ? rCTAA :
		(LogFont.lfQuality == ANTIALIASED_QUALITY) ? rStandardAA : rNoneAA);

	// 3d state - force center symbols in cells
	checkDlgButton(hWnd2, cbMonospace, BST(gpSet->isMonospace));

	checkDlgButton(hWnd2, cbBold, (LogFont.lfWeight == FW_BOLD) ? BST_CHECKED : BST_UNCHECKED);

	checkDlgButton(hWnd2, cbItalic, LogFont.lfItalic ? BST_CHECKED : BST_UNCHECKED);

	/* Alternative font, initially created for prettifying Far Manager borders */
	{
		checkDlgButton(hWnd2, cbFixFarBorders, BST(gpSet->isFixFarBorders));

		checkDlgButton(hWnd2, cbFont2AA, gpSet->isAntiAlias2 ? BST_CHECKED : BST_UNCHECKED);

		LPCWSTR cszFontRanges[] = {
			L"Far Manager borders: 2500-25C4;",
			L"Dashes and Borders: 2013-2015;2500-25C4;",
			L"Pseudographics: 2013-25C4;",
			L"CJK: 2E80-9FC3;AC00-D7A3;F900-FAFF;FE30-FE4F;FF01-FF60;FFE0-FFE6;",
			NULL
		};
		CEStr szCharRanges(gpSet->CreateCharRanges(gpSet->mpc_CharAltFontRanges));
		LPCWSTR pszCurrentRange = szCharRanges.ms_Val;
		bool bExist = false;

		HWND hCombo = GetDlgItem(hWnd2, tUnicodeRanges);
		SendDlgItemMessage(hWnd2, tUnicodeRanges, CB_RESETCONTENT, 0,0);

		// Fill our drop down with font ranges
		for (INT_PTR i = 0; cszFontRanges[i]; i++)
		{
			LPCWSTR pszRange = wcsstr(cszFontRanges[i], L": ");
			if (!pszRange) { _ASSERTE(pszRange); continue; }

			SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)cszFontRanges[i]);

			if (!bExist && (lstrcmpi(pszRange+2, pszCurrentRange) == 0))
			{
				pszCurrentRange = cszFontRanges[i];
				bExist = true;
			}
		}
		if (pszCurrentRange && *pszCurrentRange && !bExist)
			SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)pszCurrentRange);
		// And show current value
		SetWindowText(hCombo, pszCurrentRange ? pszCurrentRange : L"");
	}
	/* Alternative font ends */

	checkDlgButton(hWnd2, cbFontMonitorDpi, gpSet->FontUseDpi ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hWnd2, cbFontAsDeviceUnits, gpSet->FontUseUnits ? BST_CHECKED : BST_UNCHECKED);

	mn_LastChangingFontCtrlId = 0;
	return 0;
}

LRESULT CSettings::OnInitDialog_Background(HWND hWnd2, bool abInitial)
{
	TCHAR tmp[255];

	SetDlgItemText(hWnd2, tBgImage, gpSet->sBgImage);
	//checkDlgButton(hWnd2, rBgSimple, BST_CHECKED);

	checkDlgButton(hWnd2, rbBgReplaceIndexes, BST_CHECKED);
	FillBgImageColors(hWnd2);

	checkDlgButton(hWnd2, cbBgImage, BST(gpSet->isShowBgImage));

	_wsprintf(tmp, SKIPLEN(countof(tmp)) L"%i", gpSet->bgImageDarker);
	SendDlgItemMessage(hWnd2, tDarker, EM_SETLIMITTEXT, 3, 0);
	SetDlgItemText(hWnd2, tDarker, tmp);
	SendDlgItemMessage(hWnd2, slDarker, TBM_SETRANGE, (WPARAM) true, (LPARAM) MAKELONG(0, 255));
	SendDlgItemMessage(hWnd2, slDarker, TBM_SETPOS  , (WPARAM) true, (LPARAM) gpSet->bgImageDarker);

	//checkDlgButton(hWnd2, rBgUpLeft+(UINT)gpSet->bgOperation, BST_CHECKED);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbBgPlacement), CSetDlgLists::eBgOper, gpSet->bgOperation, false);

	checkDlgButton(hWnd2, cbBgAllowPlugin, BST(gpSet->isBgPluginAllowed));

	CSetDlgLists::EnableDlgItems(hWnd2, CSetDlgLists::eImgCtrls, gpSet->isShowBgImage);

	return 0;
}

LRESULT CSettings::OnInitDialog_Show(HWND hWnd2, bool abInitial)
{
	checkDlgButton(hWnd2, cbHideCaption, gpSet->isHideCaption);

	checkDlgButton(hWnd2, cbHideCaptionAlways, gpSet->isHideCaptionAlways());
	EnableWindow(GetDlgItem(hWnd2, cbHideCaptionAlways), !gpSet->isForcedHideCaptionAlways());

	// Quake style
	checkDlgButton(hWnd2, cbQuakeStyle, gpSet->isQuakeStyle ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hWnd2, cbQuakeAutoHide, (gpSet->isQuakeStyle == 2) ? BST_CHECKED : BST_UNCHECKED);
	// Show/Hide/Slide timeout
	SetDlgItemInt(hWnd2, tQuakeAnimation, gpSet->nQuakeAnimation, FALSE);

	EnableWindow(GetDlgItem(hWnd2, cbQuakeAutoHide), gpSet->isQuakeStyle);

	// Скрытие рамки
	SetDlgItemInt(hWnd2, tHideCaptionAlwaysFrame, gpSet->HideCaptionAlwaysFrame(), TRUE);
	SetDlgItemInt(hWnd2, tHideCaptionAlwaysDelay, gpSet->nHideCaptionAlwaysDelay, FALSE);
	SetDlgItemInt(hWnd2, tHideCaptionAlwaysDissapear, gpSet->nHideCaptionAlwaysDisappear, FALSE);

	// Child GUI applications
	checkDlgButton(hWnd2, cbHideChildCaption, gpSet->isHideChildCaption);

	checkDlgButton(hWnd2, cbEnhanceGraphics, gpSet->isEnhanceGraphics);

	//checkDlgButton(hWnd2, cbEnhanceButtons, gpSet->isEnhanceButtons);

	//checkDlgButton(hWnd2, cbAlwaysShowScrollbar, gpSet->isAlwaysShowScrollbar);
	checkRadioButton(hWnd2, rbScrollbarHide, rbScrollbarAuto, (gpSet->isAlwaysShowScrollbar==0) ? rbScrollbarHide : (gpSet->isAlwaysShowScrollbar==1) ? rbScrollbarShow : rbScrollbarAuto);
	SetDlgItemInt(hWnd2, tScrollAppearDelay, gpSet->nScrollBarAppearDelay, FALSE);
	SetDlgItemInt(hWnd2, tScrollDisappearDelay, gpSet->nScrollBarDisappearDelay, FALSE);

	checkDlgButton(hWnd2, cbAlwaysOnTop, gpSet->isAlwaysOnTop);

	#ifdef _DEBUG
	checkDlgButton(hWnd2, cbTabsInCaption, gpSet->isTabsInCaption);
	#else
	ShowWindow(GetDlgItem(hWnd2, cbTabsInCaption), SW_HIDE);
	#endif

	checkDlgButton(hWnd2, cbNumberInCaption, gpSet->isNumberInCaption);

	checkDlgButton(hWnd2, cbMultiCon, gpSet->mb_isMulti);
	checkDlgButton(hWnd2, cbMultiShowButtons, gpSet->isMultiShowButtons);
	checkDlgButton(hWnd2, cbMultiShowSearch, gpSet->isMultiShowSearch);

	checkDlgButton(hWnd2, cbSingleInstance, IsSingleInstanceArg());
	EnableDlgItem(hWnd2, cbSingleInstance, (gpSet->isQuakeStyle == 0));

	checkDlgButton(hWnd2, cbShowHelpTooltips, gpSet->isShowHelpTooltips);

	return 0;
}

LRESULT CSettings::OnInitDialog_Confirm(HWND hWnd2, bool abInitial)
{
	checkDlgButton(hWnd2, cbNewConfirm, gpSet->isMultiNewConfirm);
	checkDlgButton(hWnd2, cbDupConfirm, gpSet->isMultiDupConfirm);
	checkDlgButton(hWnd2, cbCloseWindowConfirm, (gpSet->nCloseConfirmFlags & Settings::cc_Window) ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hWnd2, cbCloseConsoleConfirm, (gpSet->nCloseConfirmFlags & Settings::cc_Console) ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hWnd2, cbConfirmCloseRunning, (gpSet->nCloseConfirmFlags & Settings::cc_Running) ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hWnd2, cbCloseEditViewConfirm, (gpSet->nCloseConfirmFlags & Settings::cc_FarEV) ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hWnd2, cbConfirmDetach, gpSet->isMultiDetachConfirm);
	checkDlgButton(hWnd2, cbShowWasHiddenMsg, gpSet->isDownShowHiddenMessage ? BST_UNCHECKED : BST_CHECKED);
	checkDlgButton(hWnd2, cbShowWasSetOnTopMsg, gpSet->isDownShowExOnTopMessage ? BST_UNCHECKED : BST_CHECKED);

	return 0;
}

LRESULT CSettings::OnInitDialog_Taskbar(HWND hWnd2, bool abInitial)
{
	checkDlgButton(hWnd2, cbMinToTray, gpSet->mb_MinToTray);
	EnableWindow(GetDlgItem(hWnd2, cbMinToTray), !gpConEmu->ForceMinimizeToTray);

	checkDlgButton(hWnd2, cbAlwaysShowTrayIcon, gpSet->isAlwaysShowTrayIcon());

	checkRadioButton(hWnd2, rbTaskbarBtnActive, rbTaskbarBtnHidden,
		(gpSet->m_isTabsOnTaskBar == 3) ? rbTaskbarBtnHidden :
		(gpSet->m_isTabsOnTaskBar == 2) ? rbTaskbarBtnWin7 :
		(gpSet->m_isTabsOnTaskBar == 1) ? rbTaskbarBtnAll
		: rbTaskbarBtnActive);
	checkDlgButton(hWnd2, cbTaskbarOverlay, gpSet->isTaskbarOverlay);
	checkDlgButton(hWnd2, cbTaskbarProgress, gpSet->isTaskbarProgress);

	//checkRadioButton(hWnd2, rbMultiLastClose, rbMultiLastTSA,
	//	gpSet->isMultiLeaveOnClose ? (gpSet->isMultiHideOnClose ? rbMultiLastTSA : rbMultiLastLeave) : rbMultiLastClose);
	checkDlgButton(hWnd2, cbCloseConEmuWithLastTab, (gpSet->isMultiLeaveOnClose == 0) ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hWnd2, cbCloseConEmuOnCrossClicking, (gpSet->isMultiLeaveOnClose != 1) ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hWnd2, cbMinimizeOnLastTabClose, (gpSet->isMultiLeaveOnClose && gpSet->isMultiHideOnClose != 0) ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hWnd2, cbHideOnLastTabClose, (gpSet->isMultiLeaveOnClose && gpSet->isMultiHideOnClose == 1) ? BST_CHECKED : BST_UNCHECKED);
	//
	EnableDlgItem(hWnd2, cbCloseConEmuOnCrossClicking, (gpSet->isMultiLeaveOnClose != 0));
	EnableDlgItem(hWnd2, cbMinimizeOnLastTabClose, (gpSet->isMultiLeaveOnClose != 0));
	EnableDlgItem(hWnd2, cbHideOnLastTabClose, (gpSet->isMultiLeaveOnClose != 0) && (gpSet->isMultiHideOnClose != 0));


	checkDlgButton(hWnd2, cbMinimizeOnLoseFocus, gpSet->mb_MinimizeOnLoseFocus);
	EnableWindow(GetDlgItem(hWnd2, cbMinimizeOnLoseFocus), (gpSet->isQuakeStyle == 0));


	checkRadioButton(hWnd2, rbMinByEscAlways, rbMinByEscNever,
		(gpSet->isMultiMinByEsc == 2) ? rbMinByEscEmpty : gpSet->isMultiMinByEsc ? rbMinByEscAlways : rbMinByEscNever);
	checkDlgButton(hWnd2, cbMapShiftEscToEsc, gpSet->isMapShiftEscToEsc);
	EnableWindow(GetDlgItem(hWnd2, cbMapShiftEscToEsc), (gpSet->isMultiMinByEsc == 1 /*Always*/));

	checkDlgButton(hWnd2, cbCmdTaskbarTasks, gpSet->isStoreTaskbarkTasks);
	checkDlgButton(hWnd2, cbCmdTaskbarCommands, gpSet->isStoreTaskbarCommands);
	EnableWindow(GetDlgItem(hWnd2, cbCmdTaskbarUpdate), (gnOsVer >= 0x601));

	return 0;
}

// IDD_SPG_WNDSIZEPOS / thi_SizePos
LRESULT CSettings::OnInitDialog_WndSizePos(HWND hWnd2, bool abInitial)
{
	_ASSERTE(GetPage(thi_SizePos) == hWnd2);

	checkDlgButton(hWnd2, cbAutoSaveSizePos, gpSet->isAutoSaveSizePos);

	checkDlgButton(hWnd2, cbUseCurrentSizePos, gpSet->isUseCurrentSizePos);

	ConEmuWindowMode wMode;
	if (gpSet->isQuakeStyle || !gpSet->isUseCurrentSizePos)
		wMode = (ConEmuWindowMode)gpSet->_WindowMode;
	else if (gpConEmu->isFullScreen())
		wMode = wmFullScreen;
	else if (gpConEmu->isZoomed())
		wMode = wmMaximized;
	else
		wMode = wmNormal;
	checkRadioButton(hWnd2, rNormal, rFullScreen,
		(wMode == wmFullScreen) ? rFullScreen
		: (wMode == wmMaximized) ? rMaximized
		: rNormal);

	SendDlgItemMessage(hWnd2, tWndWidth, EM_SETLIMITTEXT, 6, 0);
	SendDlgItemMessage(hWnd2, tWndHeight, EM_SETLIMITTEXT, 6, 0);

	UpdateSize(gpConEmu->WndWidth, gpConEmu->WndHeight);

	EnableWindow(GetDlgItem(hWnd2, cbApplyPos), FALSE);
	SendDlgItemMessage(hWnd2, tWndX, EM_SETLIMITTEXT, 6, 0);
	SendDlgItemMessage(hWnd2, tWndY, EM_SETLIMITTEXT, 6, 0);
	UpdatePosSizeEnabled(hWnd2);
	MCHKHEAP

	UpdatePos(gpConEmu->wndX, gpConEmu->wndY, true);

	checkRadioButton(hWnd2, rCascade, rFixed, gpSet->wndCascade ? rCascade : rFixed);
	SetDlgItemText(hWnd2, rFixed, gpSet->isQuakeStyle ? L"Free" : L"Fixed");
	SetDlgItemText(hWnd2, rCascade, gpSet->isQuakeStyle ? L"Centered" : L"Cascade");

	checkDlgButton(hWnd2, cbLongOutput, gpSet->AutoBufferHeight);
	TODO("Надо бы увеличить, но нужно сервер допиливать");
	SendDlgItemMessage(hWnd2, tLongOutputHeight, EM_SETLIMITTEXT, 5, 0);
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

	checkDlgButton(hWnd2, cbIntegralSize, !gpSet->mb_IntegralSize);

	checkDlgButton(hWnd2, cbRestore2ActiveMonitor, gpSet->isRestore2ActiveMon);

	checkDlgButton(hWnd2, cbSnapToDesktopEdges, gpSet->isSnapToDesktopEdges);

	return 0;
}

void CSettings::InitCursorCtrls(HWND hWnd2, const AppSettings* pApp)
{
	checkRadioButton(hWnd2, rCursorH, rCursorR, (rCursorH + pApp->CursorActive.CursorType));
	checkDlgButton(hWnd2, cbCursorColor, pApp->CursorActive.isColor);
	checkDlgButton(hWnd2, cbCursorBlink, pApp->CursorActive.isBlinking);
	checkDlgButton(hWnd2, cbCursorIgnoreSize, pApp->CursorActive.isFixedSize);
	SetDlgItemInt(hWnd2, tCursorFixedSize, pApp->CursorActive.FixedSize, FALSE);
	SetDlgItemInt(hWnd2, tCursorMinSize, pApp->CursorActive.MinSize, FALSE);

	checkDlgButton(hWnd2, cbInactiveCursor, pApp->CursorInactive.Used);
	checkRadioButton(hWnd2, rInactiveCursorH, rInactiveCursorR, (rInactiveCursorH + pApp->CursorInactive.CursorType));
	checkDlgButton(hWnd2, cbInactiveCursorColor, pApp->CursorInactive.isColor);
	checkDlgButton(hWnd2, cbInactiveCursorBlink, pApp->CursorInactive.isBlinking);
	checkDlgButton(hWnd2, cbInactiveCursorIgnoreSize, pApp->CursorInactive.isFixedSize);
	SetDlgItemInt(hWnd2, tInactiveCursorFixedSize, pApp->CursorInactive.FixedSize, FALSE);
	SetDlgItemInt(hWnd2, tInactiveCursorMinSize, pApp->CursorInactive.MinSize, FALSE);
}

LRESULT CSettings::OnInitDialog_Cursor(HWND hWnd2, bool abInitial)
{
	InitCursorCtrls(hWnd2, &gpSet->AppStd);

	return 0;
}

LRESULT CSettings::OnInitDialog_Startup(HWND hWnd2, bool abInitial)
{
	pageOpProc_Start(hWnd2, WM_INITDIALOG, 0, abInitial);

	return 0;
}

INT_PTR CSettings::pageOpProc_Start(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR iRc = 0;
	const wchar_t* csNoTask = L"<None>";
	#define MSG_SHOWTASKCONTENTS (WM_USER+64)

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
			BOOL bInitial = (lParam != 0); UNREFERENCED_PARAMETER(bInitial);

			checkRadioButton(hWnd2, rbStartSingleApp, rbStartLastTabs, rbStartSingleApp+gpSet->nStartType);

			SetDlgItemText(hWnd2, tCmdLine, gpSet->psStartSingleApp ? gpSet->psStartSingleApp : L"");

			// Признак "командного файла" - лидирующая @, в диалоге - не показываем
			SetDlgItemText(hWnd2, tStartTasksFile, gpSet->psStartTasksFile ? (*gpSet->psStartTasksFile == CmdFilePrefix ? (gpSet->psStartTasksFile+1) : gpSet->psStartTasksFile) : L"");

			int nGroup = 0;
			const CommandTasks* pGrp = NULL;
			SendDlgItemMessage(hWnd2, lbStartNamedTask, CB_RESETCONTENT, 0,0);
			SendDlgItemMessage(hWnd2, lbStartNamedTask, CB_ADDSTRING, 0, (LPARAM)csNoTask);
			// Fill tasks from settings
			while ((pGrp = gpSet->CmdTaskGet(nGroup++)))
				SendDlgItemMessage(hWnd2, lbStartNamedTask, CB_ADDSTRING, 0, (LPARAM)pGrp->pszName);
			// Select active task
			pGrp = gpSet->psStartTasksName ? gpSet->CmdTaskGetByName(gpSet->psStartTasksName) : NULL;
			if (!pGrp || !pGrp->pszName
				|| (CSetDlgLists::SelectStringExact(hWnd2, lbStartNamedTask, pGrp->pszName) <= 0))
			{
				if (gpSet->nStartType == (rbStartNamedTask - rbStartSingleApp))
				{
					// 0 -- csNoTask
					// Задачи с таким именем больше нет - прыгаем на "Командную строку"
					gpSet->nStartType = 0;
					checkRadioButton(hWnd2, rbStartSingleApp, rbStartLastTabs, rbStartSingleApp);
				}
			}

			SetDlgItemInt(hWnd2, tStartCreateDelay, gpSet->nStartCreateDelay, FALSE);

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
						// -- not supported yet
						ShowWindow(GetDlgItem(hWnd2, cbStartFarRestoreFolders), SW_HIDE);
						ShowWindow(GetDlgItem(hWnd2, cbStartFarRestoreEditors), SW_HIDE);
						//
						EnableWindow(GetDlgItem(hWnd2, tStartGroupCommands), (CB == rbStartNamedTask) || (CB == rbStartLastTabs));
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

							LPCWSTR pszFilter, pszTitle;
							if (CB==cbCmdLine)
							{
								pszFilter = L"Executables (*.exe)\0*.exe\0All files (*.*)\0*.*\0\0";
								pszTitle = L"Choose application";
							}
							else
							{
								pszFilter = L"Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0\0";
								pszTitle = L"Choose command file";
							}

							wchar_t* pszRet = SelectFile(pszTitle, temp, NULL, ghOpWnd, pszFilter, (CB==cbCmdLine)?sff_AutoQuote:sff_Default);

							if (pszRet)
							{
								SetDlgItemText(hWnd2, (CB==cbCmdLine)?tCmdLine:tStartTasksFile, pszRet);
								SafeFree(pszRet);
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
						} // tStartTasksFile
						break;
					case tStartCreateDelay:
						{
							BOOL bDelayOk = FALSE;
							UINT nNewDelay = GetDlgItemInt(hWnd2, tStartCreateDelay, &bDelayOk, FALSE);
							if (bDelayOk)
								gpSet->nStartCreateDelay = GetMinMax(nNewDelay, RUNQUEUE_CREATE_LAG_MIN, RUNQUEUE_CREATE_LAG_MAX);
							else
								gpSet->nStartCreateDelay = RUNQUEUE_CREATE_LAG_DEF;
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
							CSetDlgLists::GetSelectedString(hWnd2, lbStartNamedTask, &pszName);
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
				const CommandTasks* pTask = (nIdx >= -1) ? gpSet->CmdTaskGet(nIdx) : NULL;
				SetDlgItemText(hWnd2, tStartGroupCommands, pTask ? pTask->pszCommands : L"");
			}
			else
			{
				SetDlgItemText(hWnd2, tStartGroupCommands, L"");
			}
		}
		break;
	}

	return iRc;
}

LRESULT CSettings::OnInitDialog_Ext(HWND hWnd2, bool abInitial)
{
	checkDlgButton(hWnd2, cbAutoRegFonts, gpSet->isAutoRegisterFonts);

	checkDlgButton(hWnd2, cbDebugSteps, gpSet->isDebugSteps);

	checkDlgButton(hWnd2, cbMonitorConsoleLang, gpSet->isMonitorConsoleLang ? BST_CHECKED : BST_UNCHECKED);

	checkDlgButton(hWnd2, cbSleepInBackground, gpSet->isSleepInBackground);

	checkDlgButton(hWnd2, cbRetardInactivePanes, gpSet->isRetardInactivePanes);

	checkDlgButton(hWnd2, cbVisible, gpSet->isConVisible);

	checkDlgButton(hWnd2, cbUseInjects, gpSet->isUseInjects);

	checkDlgButton(hWnd2, cbProcessAnsi, gpSet->isProcessAnsi);

	checkDlgButton(hWnd2, cbAnsiLog, gpSet->isAnsiLog);
	SetDlgItemText(hWnd2, tAnsiLogPath, gpSet->pszAnsiLog ? gpSet->pszAnsiLog : L"");

	checkDlgButton(hWnd2, cbProcessNewConArg, gpSet->isProcessNewConArg);
	checkDlgButton(hWnd2, cbProcessCmdStart, gpSet->isProcessCmdStart);
	checkDlgButton(hWnd2, cbProcessCtrlZ, gpSet->isProcessCtrlZ);

	checkDlgButton(hWnd2, cbSuppressBells, gpSet->isSuppressBells);

	checkDlgButton(hWnd2, cbConsoleExceptionHandler, gpSet->isConsoleExceptionHandler);

	checkDlgButton(hWnd2, cbUseClink, gpSet->isUseClink() ? BST_CHECKED : BST_UNCHECKED);

	checkDlgButton(hWnd2, cbDosBox, gpConEmu->mb_DosBoxExists);
	// изменение пока запрещено
	// или чтобы ругнуться, если DosBox не установлен
	EnableWindow(GetDlgItem(hWnd2, cbDosBox), !gpConEmu->mb_DosBoxExists);
	//EnableWindow(GetDlgItem(hWnd2, bDosBoxSettings), FALSE); // изменение пока запрещено
	ShowWindow(GetDlgItem(hWnd2, bDosBoxSettings), SW_HIDE);

	checkDlgButton(hWnd2, cbDisableAllFlashing, gpSet->isDisableAllFlashing);

	checkDlgButton(hWnd2, cbFocusInChildWindows, gpSet->isFocusInChildWindows);

	return 0;
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
	EnableDlgItem(hWnd2, cbComspecUpdateEnv, (gpSet->ComSpec.csType!=cst_EnvVar));

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

	// Autorun (autoattach) with "cmd.exe" or "tcc.exe"
	pageOpProc_Integr(hWnd2, UM_RELOAD_AUTORUN, UM_RELOAD_AUTORUN, 0);

	return 0;
}

LRESULT CSettings::OnInitDialog_Environment(HWND hWnd2, bool abInitial)
{
	checkDlgButton(hWnd2, cbAddConEmu2Path, (gpSet->ComSpec.AddConEmu2Path & CEAP_AddConEmuExeDir) ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hWnd2, cbAddConEmuBase2Path, (gpSet->ComSpec.AddConEmu2Path & CEAP_AddConEmuBaseDir) ? BST_CHECKED : BST_UNCHECKED);

	SetDlgItemText(hWnd2, tSetCommands, gpSet->psEnvironmentSet ? gpSet->psEnvironmentSet : L"");

	return 0;
}

LRESULT CSettings::OnInitDialog_MarkCopy(HWND hWnd2, bool abInitial)
{
	checkDlgButton(hWnd2, cbCTSIntelligent, gpSet->isCTSIntelligent);
	wchar_t* pszExcept = gpSet->GetIntelligentExceptions();
	SetDlgItemText(hWnd2, tCTSIntelligentExceptions, pszExcept ? pszExcept : L"");
	SafeFree(pszExcept);

	checkDlgButton(hWnd2, cbCTSAutoCopy, gpSet->isCTSAutoCopy);
	checkDlgButton(hWnd2, cbCTSResetOnRelease, (gpSet->isCTSResetOnRelease && gpSet->isCTSAutoCopy));
	EnableDlgItem(hWnd2, cbCTSResetOnRelease, gpSet->isCTSAutoCopy);

	checkDlgButton(hWnd2, cbCTSIBeam, gpSet->isCTSIBeam);

	checkDlgButton(hWnd2, cbCTSEndOnTyping, (gpSet->isCTSEndOnTyping != 0));
	checkDlgButton(hWnd2, cbCTSEndOnKeyPress, (gpSet->isCTSEndOnTyping != 0) && gpSet->isCTSEndOnKeyPress);
	checkDlgButton(hWnd2, cbCTSEndCopyBefore, (gpSet->isCTSEndOnTyping == 1));
	checkDlgButton(hWnd2, cbCTSEraseBeforeReset, gpSet->isCTSEraseBeforeReset);
	EnableDlgItem(hWnd2, cbCTSEndOnKeyPress, gpSet->isCTSEndOnTyping!=0);
	EnableDlgItem(hWnd2, cbCTSEndCopyBefore, gpSet->isCTSEndOnTyping!=0);
	EnableDlgItem(hWnd2, cbCTSEraseBeforeReset, gpSet->isCTSEndOnTyping!=0);

	checkDlgButton(hWnd2, cbCTSFreezeBeforeSelect, gpSet->isCTSFreezeBeforeSelect);
	checkDlgButton(hWnd2, cbCTSBlockSelection, gpSet->isCTSSelectBlock);
	UINT VkMod = gpSet->GetHotkeyById(vkCTSVkBlock);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbCTSBlockSelection), CSetDlgLists::eKeysAct, VkMod, true);
	checkDlgButton(hWnd2, cbCTSTextSelection, gpSet->isCTSSelectText);
	VkMod = gpSet->GetHotkeyById(vkCTSVkText);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbCTSTextSelection), CSetDlgLists::eKeysAct, VkMod, true);
	VkMod = gpSet->GetHotkeyById(vkCTSVkAct);

	UINT idxBack = (gpSet->isCTSColorIndex & 0xF0) >> 4;
	UINT idxFore = (gpSet->isCTSColorIndex & 0xF);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbCTSForeIdx), CSetDlgLists::eColorIdx16, idxFore, false);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbCTSBackIdx), CSetDlgLists::eColorIdx16, idxBack, false);

	if (abInitial)
	{
		mf_MarkCopyPreviewProc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hWnd2, stCTSPreview), GWLP_WNDPROC, (LONG_PTR)MarkCopyPreviewProc);
	}

	checkDlgButton(hWnd2, cbCTSDetectLineEnd, gpSet->AppStd.isCTSDetectLineEnd);
	checkDlgButton(hWnd2, cbCTSBashMargin, gpSet->AppStd.isCTSBashMargin);
	checkDlgButton(hWnd2, cbCTSTrimTrailing, gpSet->AppStd.isCTSTrimTrailing);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbCTSEOL), CSetDlgLists::eCRLF, gpSet->AppStd.isCTSEOL, false);

	checkDlgButton(hWnd2, cbCTSShiftArrowStartSel, gpSet->AppStd.isCTSShiftArrowStart);

	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbCopyFormat), CSetDlgLists::eCopyFormat, gpSet->isCTSHtmlFormat, false);

	CheckSelectionModifiers(hWnd2);

	return 0;
}

LRESULT CSettings::OnInitDialog_Paste(HWND hWnd2, bool abInitial)
{
	checkDlgButton(hWnd2, cbClipShiftIns, gpSet->AppStd.isPasteAllLines);
	checkDlgButton(hWnd2, cbClipCtrlV, gpSet->AppStd.isPasteFirstLine);
	checkDlgButton(hWnd2, cbClipConfirmEnter, gpSet->isPasteConfirmEnter);
	checkDlgButton(hWnd2, cbClipConfirmLimit, (gpSet->nPasteConfirmLonger!=0));
	SetDlgItemInt(hWnd2, tClipConfirmLimit, gpSet->nPasteConfirmLonger, FALSE);

	return 0;
}

LRESULT CSettings::OnInitDialog_Hilight(HWND hWnd2, bool abInitial)
{
	// Hyperlinks & compiler errors
	checkDlgButton(hWnd2, cbFarGotoEditor, gpSet->isFarGotoEditor);
	UINT VkMod = gpSet->GetHotkeyById(vkFarGotoEditorVk);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbFarGotoEditorVk), CSetDlgLists::eKeysAct, VkMod, false);

	LPCWSTR ppszDefEditors[] = {
		HI_GOTO_EDITOR_FAR,    // Far Manager
		HI_GOTO_EDITOR_SCITE,  // SciTE (Scintilla)
		HI_GOTO_EDITOR_NPADP,  // Notepad++
		HI_GOTO_EDITOR_VIMW,   // Vim, official, using Windows paths
		HI_GOTO_EDITOR_SUBLM,  // Sublime text
		HI_GOTO_EDITOR_CMD,    // Just ‘start’ highlighted file via cmd.exe
		HI_GOTO_EDITOR_CMD_NC, // Just ‘start’ highlighted file via cmd.exe, same as prev. but without close confirmation
		NULL};
	FillCBList(GetDlgItem(hWnd2, lbGotoEditorCmd), abInitial, ppszDefEditors, gpSet->sFarGotoEditor);

	// Highlight full row/column under mouse cursor
	checkDlgButton(hWnd2, cbHighlightMouseRow, gpSet->isHighlightMouseRow);
	checkDlgButton(hWnd2, cbHighlightMouseCol, gpSet->isHighlightMouseCol);

	// This modifier (lbFarGotoEditorVk) is not checked
	// CheckSelectionModifiers(hWnd2);

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
		{lbCTSClickPromptPosition, L"Prompt position", thi_KeybMouse, gpSet->AppStd.isCTSClickPromptPosition!=0, vkCTSVkPromptClk, true},

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

LRESULT CSettings::OnInitDialog_Far(HWND hWnd2, bool abInitial)
{
	// Сначала - то что обновляется при активации вкладки

	// Списки
	UINT VkMod = gpSet->GetHotkeyById(vkLDragKey);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbLDragKey), CSetDlgLists::eKeys, VkMod, false);
	VkMod = gpSet->GetHotkeyById(vkRDragKey);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbRDragKey), CSetDlgLists::eKeys, VkMod, false);

	if (!abInitial)
		return 0;

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

	SetDlgItemText(hWnd2, tTabPanels, gpSet->szTabPanels);
	SetDlgItemText(hWnd2, tTabViewer, gpSet->szTabViewer);
	SetDlgItemText(hWnd2, tTabEditor, gpSet->szTabEditor);
	SetDlgItemText(hWnd2, tTabEditorMod, gpSet->szTabEditorModified);

	CheckSelectionModifiers(hWnd2);

	return 0;
}

LRESULT CSettings::OnInitDialog_FarMacro(HWND hWnd2, bool abInitial)
{
	_ASSERTE(gpSet->isRClickSendKey==0 || gpSet->isRClickSendKey==1 || gpSet->isRClickSendKey==2);

	checkDlgButton(hWnd2, cbRClick, gpSet->isRClickSendKey);
	checkDlgButton(hWnd2, cbSafeFarClose, gpSet->isSafeFarClose);

	struct MacroItem {
		int nDlgItem;
		LPCWSTR pszEditor;
		LPCWSTR pszVariants[6];
	} Macros[] = {
		{tRClickMacro, gpSet->RClickMacro(fmv_Default),
			{FarRClickMacroDefault2, FarRClickMacroDefault3, NULL}},
		{tSafeFarCloseMacro, gpSet->SafeFarCloseMacro(fmv_Default),
		{FarSafeCloseMacroDefault2, FarSafeCloseMacroDefault3, FarSafeCloseMacroDefaultD2, FarSafeCloseMacroDefaultD3, L"#Close(0)", NULL}},
		{tCloseTabMacro, gpSet->TabCloseMacro(fmv_Default),
			{FarTabCloseMacroDefault2, FarTabCloseMacroDefault3, NULL}},
		{tSaveAllMacro, gpSet->SaveAllMacro(fmv_Default),
			{FarSaveAllMacroDefault2, FarSaveAllMacroDefault3, NULL}},
		{0}
	};

	for (MacroItem* p = Macros; p->nDlgItem; p++)
	{
		HWND hCombo = GetDlgItem(hWnd2, p->nDlgItem);

		FillCBList(hCombo, abInitial, p->pszVariants, p->pszEditor);
	}

	return 0;
}

void CSettings::FillCBList(HWND hCombo, bool abInitial, LPCWSTR* ppszPredefined, LPCWSTR pszUser)
{
	bool bUser = (pszUser != NULL);

	if (abInitial)
	{
		// Variants
		if (ppszPredefined)
		{
			for (LPCWSTR* ppsz = ppszPredefined; *ppsz; ppsz++)
			{
				SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)*ppsz);
				if (pszUser && (lstrcmp(*ppsz, pszUser) == 0))
					bUser = false; // This is our predefined string
			}
		}

		// Add user defined string to list
		if (bUser)
		{
			SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)pszUser);
		}
	}

	if (pszUser && (abInitial || GetWindowTextLength(hCombo) == 0))
	{
		SetWindowText(hCombo, pszUser);
	}
}

void CSettings::FillHotKeysList(HWND hWnd2, BOOL abInitial)
{
	static UINT nLastShowType = rbHotkeysAll;
	UINT nShowType = rbHotkeysAll;
	if (IsChecked(hWnd2, rbHotkeysUser))
		nShowType = rbHotkeysUser;
	else if (IsChecked(hWnd2, rbHotkeysMacros))
		nShowType = rbHotkeysMacros;
	else if (IsChecked(hWnd2, rbHotkeysSystem))
		nShowType = rbHotkeysSystem;

	static bool bLastHideEmpties = false;
	bool bHideEmpties = (IsChecked(hWnd2, cbHotkeysAssignedOnly) == BST_CHECKED);

	// Населить список всеми хоткеями
	HWND hList = GetDlgItem(hWnd2, lbConEmuHotKeys);
	if (abInitial || (nLastShowType != nShowType) || (bLastHideEmpties != bHideEmpties))
	{
		ListView_DeleteAllItems(hList);
		abInitial = TRUE;
	}
	nLastShowType = nShowType;
	bLastHideEmpties = bHideEmpties;


	wchar_t szName[128], szDescr[512];
	//HWND hDetails = GetDlgItem(hWnd2, lbActivityDetails);
	LVITEM lvi = {LVIF_TEXT|LVIF_STATE|LVIF_PARAM};
	lvi.state = 0;
	lvi.stateMask = LVIS_SELECTED|LVIS_FOCUSED;
	lvi.pszText = szName;
	const ConEmuHotKey *ppHK = NULL;
	int ItemsCount = (int)ListView_GetItemCount(hList);
	int nItem = -1; // если -1 то будет добавлен новый

	// Если выбран режим "Показать только макросы"
	// то сначала отобразить пользовательские "Macro 00"
	// а после них все системные, которые используют Macro (для справки)
	size_t stepMax = (nShowType == rbHotkeysMacros) ? 1 : 0;
	for (size_t step = 0; step <= stepMax; step++)
	{
		int iHkIdx = 0;

		while (TRUE)
		{
			if (abInitial)
			{
				ppHK = GetHotKeyPtr(iHkIdx++);
				if (!ppHK)
					break; // кончились
				nItem = -1; // если -1 то будет добавлен новый

				switch (nShowType)
				{
				case rbHotkeysUser:
					if ((ppHK->HkType != chk_User) && (ppHK->HkType != chk_Global) && (ppHK->HkType != chk_Local)
						&& (ppHK->HkType != chk_Modifier) && (ppHK->HkType != chk_Modifier2) && (ppHK->HkType != chk_Task))
						continue;
					break;
				case rbHotkeysMacros:
					if (((step == 0) && (ppHK->HkType != chk_Macro))
						|| ((step > 0) && ((ppHK->HkType == chk_Macro) || !ppHK->GuiMacro)))
						continue;
					break;
				case rbHotkeysSystem:
					if ((ppHK->HkType != chk_System) && (ppHK->HkType != chk_ArrHost) && (ppHK->HkType != chk_NumHost))
						continue;
					break;
				default:
					; // OK
				}

				if (bHideEmpties)
				{
					if (ppHK->Key.IsEmpty())
						continue;
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
			case chk_Local:
				wcscpy_c(szName, L"Local"); break;
			case chk_User:
				wcscpy_c(szName, L"User"); break;
			case chk_Macro:
				_wsprintf(szName, SKIPLEN(countof(szName)) L"Macro %02i", ppHK->DescrLangID-vkGuiMacro01+1); break;
			case chk_Modifier:
			case chk_Modifier2:
				wcscpy_c(szName, L"Modifier"); break;
			case chk_NumHost:
			case chk_ArrHost:
				wcscpy_c(szName, L"System"); break;
			case chk_System:
				wcscpy_c(szName, L"System"); break;
			case chk_Task:
				wcscpy_c(szName, L"Task"); break;
			default:
				// Неизвестный тип!
				_ASSERTE(FALSE && "Unknown ppHK->HkType");
				//wcscpy_c(szName, L"???");
				continue;
			}

			if (nItem == -1)
			{
				lvi.iItem = ItemsCount + 1; // в конец
				lvi.lParam = (LPARAM)ppHK;
				nItem = ListView_InsertItem(hList, &lvi);
				//_ASSERTE(nItem==ItemsCount && nItem>=0);
				ItemsCount++;
			}
			if (abInitial)
			{
				ListView_SetItemState(hList, nItem, 0, LVIS_SELECTED|LVIS_FOCUSED);
			}

			ppHK->GetHotkeyName(szName);

			ListView_SetItemText(hList, nItem, klc_Hotkey, szName);

			if (ppHK->HkType == chk_Macro)
			{
				//wchar_t* pszBuf = EscapeString(true, ppHK->GuiMacro);
				//LPCWSTR pszMacro = pszBuf;
				LPCWSTR pszMacro = ppHK->GuiMacro;
				if (!pszMacro || !*pszMacro)
					pszMacro = L"<Not set>";
				ListView_SetItemText(hList, nItem, klc_Desc, (wchar_t*)pszMacro);
				//SafeFree(pszBuf);
			}
			else
			{
				ppHK->GetDescription(szDescr, countof(szDescr));
				ListView_SetItemText(hList, nItem, klc_Desc, szDescr);
			}
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
				case chk_Local:
				case chk_Task:
					pszLabel = L"Choose hotkey:";
					VkMod = pk->GetVkMod();
					bHotKeyEnabled = bKeyListEnabled = bModifiersEnabled = TRUE;
					break;
				case chk_Macro:
					pszLabel = L"Choose hotkey:";
					VkMod = pk->GetVkMod();
					bHotKeyEnabled = bKeyListEnabled = bModifiersEnabled = bMacroEnabled = TRUE;
					break;
				case chk_Modifier:
					pszLabel = L"Choose modifier:";
					VkMod = pk->GetVkMod();
					bKeyListEnabled = TRUE;
					break;
				case chk_Modifier2:
					pszLabel = L"Choose modifier:";
					VkMod = pk->GetVkMod();
					bModifiersEnabled = TRUE;
					break;
				case chk_NumHost:
				case chk_ArrHost:
					pszLabel = L"Choose modifiers:";
					VkMod = pk->GetVkMod();
					_ASSERTE(VkMod);
					bModifiersEnabled = TRUE;
					break;
				case chk_System:
					pszLabel = L"Predefined:";
					VkMod = pk->GetVkMod();
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

				//if (bMacroEnabled)
				//{
				//	pszDescription = pk->GuiMacro;
				//}
				//else
				//{
				//	if (!CLngRc::getHint(pk->DescrLangID, szDescTemp, countof(szDescTemp)))
				//		szDescTemp[0] = 0;
				//	pszDescription = szDescTemp;
				//}
				// -- use function
				pszDescription = pk->GetDescription(szDescTemp, countof(szDescTemp));

				//nVK = pk ? *pk->VkPtr : 0;
				//if ((pk->Type == 0) || (pk->Type == 2))
				BYTE vk = ConEmuHotKey::GetHotkey(VkMod);
				if (bHotKeyEnabled)
				{
					SetHotkeyField(hHk, vk);
					//SendMessage(hHk, HKM_SETHOTKEY,
					//	vk|((vk==VK_DELETE||vk==VK_UP||vk==VK_DOWN||vk==VK_LEFT||vk==VK_RIGHT
					//	||vk==VK_PRIOR||vk==VK_NEXT||vk==VK_HOME||vk==VK_END
					//	||vk==VK_INSERT) ? (HOTKEYF_EXT<<8) : 0), 0);

					// Warning! Если nVK не указан в SettingsNS::nKeysHot - nVK будет обнулен
					CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbHotKeyList), CSetDlgLists::eKeysHot, vk, false);
				}
				else if (bKeyListEnabled)
				{
					SetHotkeyField(hHk, 0);
					CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbHotKeyList), CSetDlgLists::eKeys, vk, false);
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
		MySetDlgItemText(hWnd2, tGuiMacro, pszDescription/*, bMacroEnabled*/);
		EnableWindow(GetDlgItem(hWnd2, cbGuiMacroHelp), (mp_ActiveHotKey!=NULL) && (bMacroEnabled || mp_ActiveHotKey->GuiMacro));
		if (!bHotKeyEnabled)
			SetHotkeyField(hHk, 0);
			//SendMessage(hHk, HKM_SETHOTKEY, 0, 0);
		if (!bKeyListEnabled)
			SendDlgItemMessage(hWnd2, lbHotKeyList, CB_RESETCONTENT, 0, 0);
		// Modifiers
		BOOL bEnabled = (mp_ActiveHotKey && (bModifiersEnabled==TRUE));
		BOOL bShow = (mp_ActiveHotKey && (mp_ActiveHotKey->HkType != chk_Modifier));
		for (int n = 0; n < 3; n++)
		{
			BYTE b = (bShow && VkMod) ? ConEmuHotKey::GetModifier(VkMod,n+1) : 0;
			//FillListBoxByte(hWnd2, lbHotKeyMod1+n, SettingsNS::Modifiers, b);
			CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbHotKeyMod1+n), CSetDlgLists::eModifiers, b, false);
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
				wchar_t szFull1[128], szFull2[128]; ;
				nCmp = lstrcmp(pHk1->GetHotkeyName(szFull1), pHk2->GetHotkeyName(szFull2));
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
					pszDescr1 = (pHk1->GuiMacro && *pHk1->GuiMacro) ? pHk1->GuiMacro : L"<Not set>";
				else
					pszDescr1 = pHk1->GetDescription(szBuf1, countof(szBuf1));

				if (pHk2->HkType == chk_Macro)
					pszDescr2 = (pHk2->GuiMacro && *pHk2->GuiMacro) ? pHk2->GuiMacro : L"<Not set>";
				else
					pszDescr2 = pHk2->GetDescription(szBuf2, countof(szBuf2));

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

void CSettings::setHotkeyCheckbox(HWND hDlg, WORD nCtrlId, int iHotkeyId, LPCWSTR pszFrom, LPCWSTR pszTo, bool bChecked)
{
	wchar_t szKeyFull[128] = L"";
	gpSet->GetHotkeyNameById(iHotkeyId, szKeyFull, false);
	if (szKeyFull[0] == 0)
	{
		EnableWindow(GetDlgItem(hDlg, nCtrlId), FALSE);
		checkDlgButton(hDlg, nCtrlId, BST_UNCHECKED);
	}
	else
	{
		if (pszFrom)
		{
			wchar_t* ptr = (wchar_t*)wcsstr(szKeyFull, pszFrom);
			if (ptr)
			{
				*ptr = 0;
				if (pszTo)
				{
					wcscat_c(szKeyFull, pszTo);
				}
			}
		}

		CEStr lsText(GetDlgItemTextPtr(hDlg, nCtrlId));
		LPCWSTR pszTail = lsText.IsEmpty() ? NULL : wcsstr(lsText, L" - ");
		if (pszTail)
		{
			CEStr lsNew(szKeyFull, pszTail);
			SetDlgItemText(hDlg, nCtrlId, lsNew);
		}

		checkDlgButton(hDlg, nCtrlId, bChecked);
	}
}

LRESULT CSettings::OnInitDialog_Control(HWND hWnd2, bool abInitial)
{
	checkDlgButton(hWnd2, cbEnableMouse, !gpSet->isDisableMouse);
	checkDlgButton(hWnd2, cbSkipActivation, gpSet->isMouseSkipActivation);
	checkDlgButton(hWnd2, cbSkipMove, gpSet->isMouseSkipMoving);
	checkDlgButton(hWnd2, cbActivateSplitMouseOver, gpSet->bActivateSplitMouseOver);

	checkDlgButton(hWnd2, cbInstallKeybHooks,
	               (gpSet->m_isKeyboardHooks == 1) ? BST_CHECKED :
	               ((gpSet->m_isKeyboardHooks == 0) ? BST_INDETERMINATE : BST_UNCHECKED));

	setHotkeyCheckbox(hWnd2, cbUseWinNumber, vkConsole_1, L"+1", L"+Numbers", gpSet->isUseWinNumber);
	setHotkeyCheckbox(hWnd2, cbUseWinArrows, vkWinLeft, L"+Left", L"+Arrows", gpSet->isUseWinArrows);

	checkDlgButton(hWnd2, cbUseWinTab, gpSet->isUseWinTab);

	checkDlgButton(hWnd2, cbSendAltTab, gpSet->isSendAltTab);
	checkDlgButton(hWnd2, cbSendAltEsc, gpSet->isSendAltEsc);
	checkDlgButton(hWnd2, cbSendAltPrintScrn, gpSet->isSendAltPrintScrn);
	checkDlgButton(hWnd2, cbSendPrintScrn, gpSet->isSendPrintScrn);
	checkDlgButton(hWnd2, cbSendCtrlEsc, gpSet->isSendCtrlEsc);

	checkDlgButton(hWnd2, cbFixAltOnAltTab, gpSet->isFixAltOnAltTab);

	checkDlgButton(hWnd2, cbSkipFocusEvents, gpSet->isSkipFocusEvents);

	// Prompt click
	checkDlgButton(hWnd2, cbCTSClickPromptPosition, gpSet->AppStd.isCTSClickPromptPosition);
	UINT VkMod = gpSet->GetHotkeyById(vkCTSVkPromptClk);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbCTSClickPromptPosition), CSetDlgLists::eKeysAct, VkMod, false);

	// Ctrl+BS - del left word
	setHotkeyCheckbox(hWnd2, cbCTSDeleteLeftWord, vkDeleteLeftWord, NULL, NULL, gpSet->AppStd.isCTSDeleteLeftWord);

	checkDlgButton(hWnd2, (gpSet->isCTSActMode==1)?rbCTSActAlways:rbCTSActBufferOnly, BST_CHECKED);
	VkMod = gpSet->GetHotkeyById(vkCTSVkAct);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbCTSActAlways), CSetDlgLists::eKeysAct, VkMod, false);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbCTSRBtnAction), CSetDlgLists::eClipAct, gpSet->isCTSRBtnAction, false);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbCTSMBtnAction), CSetDlgLists::eClipAct, gpSet->isCTSMBtnAction, false);

	CheckSelectionModifiers(hWnd2);

	return 0;
}

LRESULT CSettings::OnInitDialog_Keys(HWND hWnd2, bool abInitial)
{
	if (!abInitial)
	{
		FillHotKeysList(hWnd2, abInitial);
		return 0;
	}

	checkRadioButton(hWnd2, rbHotkeysAll, rbHotkeysMacros, rbHotkeysAll);

	for (INT_PTR i = m_HotKeys.size() - 1; i >= 0; i--)
	{
		ConEmuHotKey* p = &(m_HotKeys[i]);
		p->cchGuiMacroMax = p->GuiMacro ? (wcslen(p->GuiMacro)+1) : 0;
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
		LVCOLUMN col = {
			LVCF_WIDTH|LVCF_TEXT|LVCF_FMT, LVCFMT_LEFT,
			gpSetCls->EvalSize(60, esf_Horizontal|esf_CanUseDpi)};
		wchar_t szTitle[64]; col.pszText = szTitle;

		ListView_SetExtendedListViewStyleEx(hList,LVS_EX_FULLROWSELECT,LVS_EX_FULLROWSELECT);
		ListView_SetExtendedListViewStyleEx(hList,LVS_EX_LABELTIP|LVS_EX_INFOTIP,LVS_EX_LABELTIP|LVS_EX_INFOTIP);

		wcscpy_c(szTitle, L"Type");			ListView_InsertColumn(hList, klc_Type, &col);
		col.cx = gpSetCls->EvalSize(120, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"Hotkey");		ListView_InsertColumn(hList, klc_Hotkey, &col);
		col.cx = gpSetCls->EvalSize(300, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"Description");	ListView_InsertColumn(hList, klc_Desc, &col);
	}

	FillHotKeysList(hWnd2, abInitial);

	for (int i = 0; i < 3; i++)
	{
		BYTE b = 0;
		CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbHotKeyMod1+i), CSetDlgLists::eModifiers, b, false);
	}
	//FillListBoxByte(hWnd2, lbHotKeyMod1, SettingsNS::Modifiers, b);
	//FillListBoxByte(hWnd2, lbHotKeyMod2, SettingsNS::Modifiers, b);
	//FillListBoxByte(hWnd2, lbHotKeyMod3, SettingsNS::Modifiers, b);

	return 0;
}

LRESULT CSettings::OnInitDialog_Tabs(HWND hWnd2, bool abInitial)
{
	checkRadioButton(hWnd2, rbTabsAlways, rbTabsNone, (gpSet->isTabs == 2) ? rbTabsAuto : gpSet->isTabs ? rbTabsAlways : rbTabsNone);

	checkDlgButton(hWnd2, cbTabsLocationBottom, (gpSet->nTabsLocation == 1) ? BST_CHECKED : BST_UNCHECKED);

	checkDlgButton(hWnd2, cbOneTabPerGroup, gpSet->isOneTabPerGroup);

	checkDlgButton(hWnd2, cbTabSelf, gpSet->isTabSelf);

	checkDlgButton(hWnd2, cbTabRecent, gpSet->isTabRecent);

	checkDlgButton(hWnd2, cbTabLazy, gpSet->isTabLazy);

	checkDlgButton(hWnd2, cbHideInactiveConTabs, gpSet->bHideInactiveConsoleTabs);
	checkDlgButton(hWnd2, cbHideDisabledTabs, gpSet->bHideDisabledTabs);
	checkDlgButton(hWnd2, cbShowFarWindows, gpSet->bShowFarWindows);

	SetDlgItemText(hWnd2, tTabFontFace, gpSet->sTabFontFace);

	if (CSetDlgFonts::EnumFontsFinished())  // Если шрифты уже считаны
	{
		if (abInitial)
			OnInitDialog_CopyFonts(hWnd2, tTabFontFace, 0); // можно скопировать список с вкладки [thi_Main]
	}

	UINT nVal = gpSet->nTabFontHeight;
	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, tTabFontHeight), CSetDlgLists::eFSizesSmall, nVal, true);

	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, tTabFontCharset), CSetDlgLists::eCharSets, gpSet->nTabFontCharSet, false);

	checkDlgButton(hWnd2, cbMultiIterate, gpSet->isMultiIterate);

	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, tTabBarDblClickAction), CSetDlgLists::eTabBarDblClickActions, gpSet->nTabBarDblClickAction);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, tTabBtnDblClickAction), CSetDlgLists::eTabBtnDblClickActions, gpSet->nTabBtnDblClickAction);

	SetDlgItemText(hWnd2, tTabConsole, gpSet->szTabConsole);
	SetDlgItemText(hWnd2, tTabModifiedSuffix, gpSet->szTabModifiedSuffix);
	SetDlgItemInt(hWnd2, tTabFlashCounter, gpSet->nTabFlashChanged, TRUE);

	SetDlgItemText(hWnd2, tTabSkipWords, gpSet->pszTabSkipWords ? gpSet->pszTabSkipWords : L"");
	SetDlgItemInt(hWnd2, tTabLenMax, gpSet->nTabLenMax, FALSE);

	checkDlgButton(hWnd2, cbAdminShield, gpSet->isAdminShield() ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hWnd2, cbAdminSuffix, gpSet->isAdminSuffix() ? BST_CHECKED : BST_UNCHECKED);
	SetDlgItemText(hWnd2, tAdminSuffix, gpSet->szAdminTitleSuffix);

	return 0;
}

LRESULT CSettings::OnInitDialog_Status(HWND hWnd2, bool abInitial)
{
	SetDlgItemText(hWnd2, tStatusFontFace, gpSet->sStatusFontFace);

	if (CSetDlgFonts::EnumFontsFinished())  // Если шрифты уже считаны
		OnInitDialog_CopyFonts(hWnd2, tStatusFontFace, 0); // можно скопировать список с вкладки [thi_Main]

	UINT nVal = gpSet->nStatusFontHeight;
	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, tStatusFontHeight), CSetDlgLists::eFSizesSmall, nVal, true);

	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, tStatusFontCharset), CSetDlgLists::eCharSets, gpSet->nStatusFontCharSet, false);

	// Colors
	for (uint c = c35; c <= c37; c++)
		ColorSetEdit(hWnd2, c);

	checkDlgButton(hWnd2, cbStatusVertSep, (gpSet->isStatusBarFlags & csf_VertDelim) ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hWnd2, cbStatusHorzSep, (gpSet->isStatusBarFlags & csf_HorzDelim) ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hWnd2, cbStatusVertPad, (gpSet->isStatusBarFlags & csf_NoVerticalPad)==0 ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hWnd2, cbStatusSystemColors, (gpSet->isStatusBarFlags & csf_SystemColors) ? BST_CHECKED : BST_UNCHECKED);

	CSetDlgLists::EnableDlgItems(hWnd2, CSetDlgLists::eStatusColorIds, !(gpSet->isStatusBarFlags & csf_SystemColors));

	checkDlgButton(hWnd2, cbShowStatusBar, gpSet->isStatusBarShow);

	//for (size_t i = 0; i < countof(SettingsNS::StatusItems); i++)
	//{
	//	checkDlgButton(hWnd2, SettingsNS::StatusItems[i].nDlgID, !gpSet->isStatusColumnHidden[SettingsNS::StatusItems[i].stItem]);
	//}

	OnInitDialog_StatusItems(hWnd2);

	return 0;
}

void CSettings::OnInitDialog_StatusItems(HWND hWnd2)
{
	HWND hAvail = GetDlgItem(hWnd2, lbStatusAvailable); _ASSERTE(hAvail!=NULL);
	INT_PTR iMaxAvail = -1, iCurAvail = SendMessage(hAvail, LB_GETCURSEL, 0, 0);
	DEBUGTEST(INT_PTR iCountAvail = SendMessage(hAvail, LB_GETCOUNT, 0, 0));
	HWND hSeltd = GetDlgItem(hWnd2, lbStatusSelected); _ASSERTE(hSeltd!=NULL);
	INT_PTR iMaxSeltd = -1, iCurSeltd = SendMessage(hSeltd, LB_GETCURSEL, 0, 0);
	DEBUGTEST(INT_PTR iCountSeltd = SendMessage(hSeltd, LB_GETCOUNT, 0, 0));

	SendMessage(hAvail, LB_RESETCONTENT, 0, 0);
	SendMessage(hSeltd, LB_RESETCONTENT, 0, 0);

	StatusColInfo* pColumns = NULL;
	size_t nCount = CStatus::GetAllStatusCols(&pColumns);
	_ASSERTE(pColumns!=NULL);

	for (size_t i = 0; i < nCount; i++)
	{
		CEStatusItems nID = pColumns[i].nID;
		if ((nID == csi_Info) || (pColumns[i].sSettingName == NULL))
			continue;

		if (gpSet->isStatusColumnHidden[nID])
		{
			iMaxAvail = SendMessage(hAvail, LB_ADDSTRING, 0, (LPARAM)pColumns[i].sName);
			if (iMaxAvail >= 0)
				SendMessage(hAvail, LB_SETITEMDATA, iMaxAvail, nID);
		}
		else
		{
			iMaxSeltd = SendMessage(hSeltd, LB_ADDSTRING, 0, (LPARAM)pColumns[i].sName);
			if (iMaxSeltd >= 0)
				SendMessage(hSeltd, LB_SETITEMDATA, iMaxSeltd, nID);
		}
	}

	if (iCurAvail >= 0 && iMaxAvail >= 0)
		SendMessage(hAvail, LB_SETCURSEL, (iCurAvail <= iMaxAvail) ? iCurAvail : iMaxAvail, 0);
	if (iCurSeltd >= 0 && iMaxSeltd >= 0)
		SendMessage(hSeltd, LB_SETCURSEL, (iCurSeltd <= iMaxSeltd) ? iCurSeltd : iMaxSeltd, 0);
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

	if (bTextChanged || bPopupChanged)
	{
		gpSet->AppStd.nTextColorIdx = pPal->nTextColorIdx;
		gpSet->AppStd.nBackColorIdx = pPal->nBackColorIdx;
		gpSet->AppStd.nPopTextColorIdx = pPal->nPopTextColorIdx;
		gpSet->AppStd.nPopBackColorIdx = pPal->nPopBackColorIdx;
		// We need to change consoles contents if TEXT attributes was changed
		UpdateTextColorSettings(bTextChanged, bPopupChanged);
	}

	gpSet->AppStd.nExtendColorIdx = pPal->nExtendColorIdx;
	gpSet->AppStd.isExtendColors = pPal->isExtendColors;

	if (hDlg && (hDlg == GetActivePage()))
	{
		OnInitDialog_Color(hDlg, false);
	}

	if (ghWnd)
	{
		gpConEmu->InvalidateAll();
		gpConEmu->Update(true);
	}
}

LRESULT CSettings::OnInitDialog_Color(HWND hWnd2, bool abInitial)
{
	#if 0
	if (gpSetCls->EnableThemeDialogTextureF)
		gpSetCls->EnableThemeDialogTextureF(hWnd2, 6/*ETDT_ENABLETAB*/);
	#endif

	checkRadioButton(hWnd2, rbColorRgbDec, rbColorBgrHex, rbColorRgbDec + m_ColorFormat);

	for (int c = c0; c <= MAX_COLOR_EDT_ID; c++)
	{
		ColorSetEdit(hWnd2, c);
		InvalidateCtrl(GetDlgItem(hWnd2, c), TRUE);
	}

	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbConClrText), CSetDlgLists::eColorIdxTh, gpSet->AppStd.nTextColorIdx, true);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbConClrBack), CSetDlgLists::eColorIdxTh, gpSet->AppStd.nBackColorIdx, true);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbConClrPopText), CSetDlgLists::eColorIdxTh, gpSet->AppStd.nPopTextColorIdx, true);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbConClrPopBack), CSetDlgLists::eColorIdxTh, gpSet->AppStd.nPopBackColorIdx, true);

	//WARNING("Отладка...");
	//if (gpSet->AppStd.nPopTextColorIdx <= 15 || gpSet->AppStd.nPopBackColorIdx <= 15
	//	|| RELEASEDEBUGTEST(FALSE,TRUE))
	//{
	//	EnableWindow(GetDlgItem(hWnd2, lbConClrPopText), TRUE);
	//	EnableWindow(GetDlgItem(hWnd2, lbConClrPopBack), TRUE);
	//}

	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbExtendIdx), CSetDlgLists::eColorIdxSh, gpSet->AppStd.nExtendColorIdx, true);
	checkDlgButton(hWnd2, cbExtendColors, gpSet->AppStd.isExtendColors ? BST_CHECKED : BST_UNCHECKED);
	OnButtonClicked(hWnd2, cbExtendColors, 0);
	checkDlgButton(hWnd2, cbTrueColorer, gpSet->isTrueColorer ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hWnd2, cbFadeInactive, gpSet->isFadeInactive ? BST_CHECKED : BST_UNCHECKED);
	SetDlgItemInt(hWnd2, tFadeLow, gpSet->mn_FadeLow, FALSE);
	SetDlgItemInt(hWnd2, tFadeHigh, gpSet->mn_FadeHigh, FALSE);

	// Palette
	const ColorPalette* pPal;

	// Default colors
	if (!abInitial && gbLastColorsOk)
	{
		// активация уже загруженной вкладки, текущую палитру уже запомнили
	}
	else if ((pPal = gpSet->PaletteGet(-1)) != NULL)
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

	SendMessage(GetDlgItem(hWnd2, lbDefaultColors), CB_RESETCONTENT, 0, 0);
	SendDlgItemMessage(hWnd2, lbDefaultColors, CB_ADDSTRING, 0, (LPARAM)gLastColors.pszName);

	for (int i = 0; (pPal = gpSet->PaletteGet(i)) != NULL; i++)
	{
		SendDlgItemMessage(hWnd2, lbDefaultColors, CB_ADDSTRING, 0, (LPARAM)pPal->pszName);
	}

	// Find saved palette with current colors and attributes
	pPal = gpSet->PaletteFindCurrent(true);
	if (pPal)
		CSetDlgLists::SelectStringExact(hWnd2, lbDefaultColors, pPal->pszName);
	else
		SendDlgItemMessage(hWnd2, lbDefaultColors, CB_SETCURSEL, 0, 0);

	bool bBtnEnabled = pPal && !pPal->bPredefined; // Save & Delete buttons
	EnableWindow(GetDlgItem(hWnd2, cbColorSchemeSave), bBtnEnabled);
	EnableWindow(GetDlgItem(hWnd2, cbColorSchemeDelete), bBtnEnabled);

	gbLastColorsOk = TRUE;

	return 0;
}

LRESULT CSettings::OnInitDialog_Transparent(HWND hWnd2)
{
	// +Color key
	ColorSetEdit(hWnd2, c38);

	checkDlgButton(hWnd2, cbTransparent, (gpSet->nTransparent!=MAX_ALPHA_VALUE) ? BST_CHECKED : BST_UNCHECKED);
	SendDlgItemMessage(hWnd2, slTransparent, TBM_SETRANGE, (WPARAM) true, (LPARAM) MAKELONG(MIN_ALPHA_VALUE, MAX_ALPHA_VALUE));
	SendDlgItemMessage(hWnd2, slTransparent, TBM_SETPOS  , (WPARAM) true, (LPARAM) gpSet->nTransparent);
	checkDlgButton(hWnd2, cbTransparentSeparate, gpSet->isTransparentSeparate ? BST_CHECKED : BST_UNCHECKED);
	//EnableWindow(GetDlgItem(hWnd2, cbTransparentInactive), gpSet->isTransparentSeparate);
	//checkDlgButton(hWnd2, cbTransparentInactive, (gpSet->nTransparentInactive!=MAX_ALPHA_VALUE) ? BST_CHECKED : BST_UNCHECKED);
	EnableWindow(GetDlgItem(hWnd2, slTransparentInactive), gpSet->isTransparentSeparate);
	EnableWindow(GetDlgItem(hWnd2, stTransparentInactive), gpSet->isTransparentSeparate);
	EnableWindow(GetDlgItem(hWnd2, stOpaqueInactive), gpSet->isTransparentSeparate);
	SendDlgItemMessage(hWnd2, slTransparentInactive, TBM_SETRANGE, (WPARAM) true, (LPARAM) MAKELONG(MIN_INACTIVE_ALPHA_VALUE, MAX_ALPHA_VALUE));
	SendDlgItemMessage(hWnd2, slTransparentInactive, TBM_SETPOS  , (WPARAM) true, (LPARAM) gpSet->isTransparentSeparate ? gpSet->nTransparentInactive : gpSet->nTransparent);
	checkDlgButton(hWnd2, cbUserScreenTransparent, gpSet->isUserScreenTransparent ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hWnd2, cbColorKeyTransparent, gpSet->isColorKeyTransparent);

	return 0;
}

LRESULT CSettings::OnInitDialog_Tasks(HWND hWnd2, bool abForceReload)
{
	mb_IgnoreCmdGroupEdit = true;

	wchar_t szKey[128] = L"";
	const ConEmuHotKey* pDefCmdKey = NULL;
	if (!gpSet->GetHotkeyById(vkMultiCmd, &pDefCmdKey) || !pDefCmdKey)
		wcscpy_c(szKey, gsNoHotkey);
	else
		pDefCmdKey->GetHotkeyName(szKey, true);
	CEStr lsLabel(L"Default shell (", szKey, L")");
	SetDlgItemText(hWnd2, cbCmdGrpDefaultCmd, lsLabel);

	// Not implemented yet
	ShowWindow(GetDlgItem(hWnd2, cbCmdGrpToolbar), SW_HIDE);

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
	const CommandTasks* pGrp = NULL;
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

	return 0;
}

LRESULT CSettings::OnInitDialog_Integr(HWND hWnd2, bool abInitial)
{
	pageOpProc_Integr(hWnd2, WM_INITDIALOG, 0, abInitial);

	return 0;
}

LRESULT CSettings::OnInitDialog_DefTerm(HWND hWnd2, bool abInitial)
{
	// Default terminal apps
	CheckDlgButton(hWnd2, cbDefaultTerminal, gpSet->isSetDefaultTerminal);
	bool bLeaveInTSA = gpSet->isRegisterOnOsStartupTSA;
	bool bRegister = gpSet->isRegisterOnOsStartup || gpConEmu->mp_DefTrm->IsRegisteredOsStartup(NULL,0,&bLeaveInTSA);
	CheckDlgButton(hWnd2, cbDefaultTerminalStartup, bRegister);
	CheckDlgButton(hWnd2, cbDefaultTerminalTSA, bLeaveInTSA);
	EnableWindow(GetDlgItem(hWnd2, cbDefaultTerminalTSA), bRegister);
	CheckDlgButton(hWnd2, cbDefTermAgressive, gpSet->isRegisterAgressive);
	CheckDlgButton(hWnd2, cbDefaultTerminalNoInjects, gpSet->isDefaultTerminalNoInjects);
	CheckDlgButton(hWnd2, cbDefaultTerminalUseExisting, !gpSet->isDefaultTerminalNewWindow);
	CheckDlgButton(hWnd2, cbDefaultTerminalDebugLog, gpSet->isDefaultTerminalDebugLog);
	CheckRadioButton(hWnd2, rbDefaultTerminalConfAuto, rbDefaultTerminalConfNever, rbDefaultTerminalConfAuto+gpSet->nDefaultTerminalConfirmClose);
	wchar_t* pszApps = gpSet->GetDefaultTerminalApps();
	_ASSERTE(pszApps!=NULL);
	SetDlgItemText(hWnd2, tDefaultTerminal, pszApps);
	if (wcschr(pszApps, L',') != NULL && wcschr(pszApps, L'|') == NULL)
		Icon.ShowTrayIcon(L"List of hooked executables must be delimited with \"|\" but not commas", tsa_Default_Term);
	SafeFree(pszApps);

	return 0;
}

// Load current value of "HKCU\Software\Microsoft\Command Processor" : "AutoRun"
// (bClear==true) - remove from it our "... Cmd_Autorun.cmd ..." part
static wchar_t* LoadAutorunValue(HKEY hkCmd, bool bClear)
{
	size_t cchCmdMax = 65535;
	wchar_t *pszCmd = (wchar_t*)malloc(cchCmdMax*sizeof(*pszCmd));
	if (!pszCmd)
	{
		_ASSERTE(pszCmd!=NULL);
		return NULL;
	}

	_ASSERTE(hkCmd!=NULL);
	//HKEY hkCmd = NULL;
	//if (0 == RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Command Processor", 0, KEY_READ, &hkCmd))

	DWORD cbMax = (cchCmdMax-2) * sizeof(*pszCmd);
	if (0 == RegQueryValueEx(hkCmd, L"AutoRun", NULL, NULL, (LPBYTE)pszCmd, &cbMax))
	{
		pszCmd[cbMax>>1] = 0;

		if (bClear && *pszCmd)
		{
			// Просили почистить от "... Cmd_Autorun.cmd ..."
			wchar_t* pszFind = StrStrI(pszCmd, L"\\ConEmu\\Cmd_Autorun.cmd");
			if (pszFind)
			{
				// "... Cmd_Autorun.cmd ..." found, need to find possible start and end of our part ('&' separated)
				wchar_t* pszStart = pszFind;
				while ((pszStart > pszCmd) && (*(pszStart-1) != L'&'))
					pszStart--;

				const wchar_t* pszEnd = wcschr(pszFind, L'&');
				if (!pszEnd)
				{
					pszEnd = pszFind + _tcslen(pszFind);
				}
				else
				{
					while (*pszEnd == L'&')
						pszEnd++;
				}

				// Ok, There are another commands?
				if ((pszStart > pszCmd) || *pszEnd)
				{
					// Possibilities
					if (!*pszEnd)
					{
						// app1.exe && Cmd_Autorun.cmd
						while ((pszStart > pszCmd) && ((*(pszStart-1) == L'&') || (*(pszStart-1) == L' ')))
							pszStart--;
						_ASSERTE(pszStart > pszCmd); // Command to left is empty?
						*pszStart = 0; // just trim
					}
					else
					{
						// app1.exe && Cmd_Autorun.cmd & app2.exe
						// app1.exe & Cmd_Autorun.cmd && app2.exe
						// Cmd_Autorun.cmd & app2.exe
						if (pszStart == pszCmd)
						{
							pszEnd = SkipNonPrintable(pszEnd);
						}
						size_t cchLeft = _tcslen(pszEnd)+1;
						// move command (from right) to the 'Cmd_Autorun.cmd' place
						memmove(pszStart, pszEnd, cchLeft*sizeof(wchar_t));
					}
				}
				else
				{
					// No, we are alone
					*pszCmd = 0;
				}
			}
		}

		// Skip spaces?
		LPCWSTR pszChar = SkipNonPrintable(pszCmd);
		if (!pszChar || !*pszChar)
		{
			*pszCmd = 0;
		}
	}
	else
	{
		*pszCmd = 0;
	}

	// Done
	if (pszCmd && (*pszCmd == 0))
	{
		SafeFree(pszCmd);
	}
	return pszCmd;
}

INT_PTR CSettings::pageOpProc_Integr(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	static bool bSkipCbSel = FALSE;
	INT_PTR iRc = 0;

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
			bSkipCbSel = true;

			pageOpProc_Integr(hWnd2, UM_RELOAD_HERE_LIST, UM_RELOAD_HERE_LIST, 0);

			//-- moved to "ComSpec" page
			//pageOpProc_Integr(hWnd2, UM_RELOAD_AUTORUN, UM_RELOAD_AUTORUN, 0);

			// Возвращает NULL, если строка пустая
			wchar_t* pszCurInside = GetDlgItemTextPtr(hWnd2, cbInsideName);
			_ASSERTE((pszCurInside==NULL) || (*pszCurInside!=0));
			wchar_t* pszCurHere   = GetDlgItemTextPtr(hWnd2, cbHereName);
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
				SetDlgItemText(hWnd2, tInsideShell, CONEMU_HERE_POSH);
				//SetDlgItemText(hWnd2, tInsideIcon, szIcon);
				SetDlgItemText(hWnd2, tInsideIcon, L"powershell.exe");
				checkDlgButton(hWnd2, cbInsideSyncDir, gpConEmu->mp_Inside && gpConEmu->mp_Inside->mb_InsideSynchronizeCurDir);
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
				SetDlgItemText(hWnd2, tHereShell, CONEMU_HERE_CMD);
				SetDlgItemText(hWnd2, tHereIcon, szIcon);
			}

			bSkipCbSel = false;

			SafeFree(pszCurInside);
			SafeFree(pszCurHere);

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
					if (gpConEmu->mp_Inside)
					{
						gpConEmu->mp_Inside->mb_InsideSynchronizeCurDir = IsChecked(hWnd2, CB);
					}
					break;
				case bInsideRegister:
				case bInsideUnregister:
					ShellIntegration(hWnd2, ShellIntgr_Inside, CB==bInsideRegister);
					pageOpProc_Integr(hWnd2, UM_RELOAD_HERE_LIST, UM_RELOAD_HERE_LIST, 0);
					if (CB==bInsideUnregister)
						pageOpProc_Integr(hWnd2, WM_COMMAND, MAKELONG(cbInsideName,CBN_SELCHANGE), 0);
					break;
				case bHereRegister:
				case bHereUnregister:
					ShellIntegration(hWnd2, ShellIntgr_Here, CB==bHereRegister);
					pageOpProc_Integr(hWnd2, UM_RELOAD_HERE_LIST, UM_RELOAD_HERE_LIST, 0);
					if (CB==bHereUnregister)
						pageOpProc_Integr(hWnd2, WM_COMMAND, MAKELONG(cbHereName,CBN_SELCHANGE), 0);
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
					if (gpConEmu->mp_Inside)
					{
						SafeFree(gpConEmu->mp_Inside->ms_InsideSynchronizeCurDir);
						gpConEmu->mp_Inside->ms_InsideSynchronizeCurDir = GetDlgItemTextPtr(hWnd2, tInsideSyncDir);
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
											CEStr szArg;
											while (0 == NextArg(&psz, szArg, &pszPrev))
											{
												if (*szArg != L'/')
													continue;

												if ((lstrcmpi(szArg, L"/inside") == 0)
													|| (lstrcmpi(szArg, L"/here") == 0)
													)
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
							SafeFree(pszName);
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
			wchar_t* pszCurInside = GetDlgItemTextPtr(hWnd2, cbInsideName);
			_ASSERTE((pszCurInside==NULL) || (*pszCurInside!=0));
			wchar_t* pszCurHere   = GetDlgItemTextPtr(hWnd2, cbHereName);
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
				CSetDlgLists::SelectStringExact(hWnd2, cbInsideName, pszCurInside);

			SetDlgItemText(hWnd2, cbHereName, pszCurHere ? pszCurHere : L"");
			if (pszCurHere && *pszCurHere)
				CSetDlgLists::SelectStringExact(hWnd2, cbHereName, pszCurHere);

			bSkipCbSel = lbOldSkip;

			SafeFree(pszCurInside);
			SafeFree(pszCurHere);

			free(pszCmd);
		}
		break; // UM_RELOAD_HERE_LIST

	case UM_RELOAD_AUTORUN:
		if (wParam == UM_RELOAD_AUTORUN) // страховка
		{
			wchar_t *pszCmd = NULL;

			BOOL bForceNewWnd = IsChecked(hWnd2, cbCmdAutorunNewWnd);

			HKEY hkDir = NULL;
			if (0 == RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Command Processor", 0, KEY_READ, &hkDir))
			{
				pszCmd = LoadAutorunValue(hkDir, false);
				if (pszCmd && *pszCmd)
				{
					bForceNewWnd = (StrStrI(pszCmd, L"/GHWND=NEW") != NULL);
				}
				RegCloseKey(hkDir);
			}

			SetDlgItemText(hWnd2, tCmdAutoAttach, pszCmd ? pszCmd : L"");
			checkDlgButton(hWnd2, cbCmdAutorunNewWnd, bForceNewWnd);

			SafeFree(pszCmd);
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
		asCmd = CONEMU_HERE_CMD;

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
	//@="C:\\Program Files\\ConEmu\\ConEmu.exe /inside /config shell /cmd {powershell} -cur_console:n"

	//[HKEY_CURRENT_USER\Software\Classes\Directory\Background\shell\ConEmu inside]
	//"Icon"="C:\\Program Files\\ConEmu\\ConEmu.exe,1"

	//[HKEY_CURRENT_USER\Software\Classes\Directory\Background\shell\ConEmu inside\command]
	//@="C:\\Program Files\\ConEmu\\ConEmu.exe /inside /config shell /cmd {powershell} -cur_console:n"

	//[HKEY_CURRENT_USER\Software\Classes\Directory\shell\ConEmu inside]
	//"Icon"="C:\\Program Files\\ConEmu\\ConEmu.exe,1"

	//[HKEY_CURRENT_USER\Software\Classes\Directory\shell\ConEmu inside\command]
	//@="C:\\Program Files\\ConEmu\\ConEmu.exe /inside /config shell /dir \"%1\" /cmd {powershell} -cur_console:n"

	//[HKEY_CURRENT_USER\Software\Classes\Drive\shell\ConEmu inside]
	//"Icon"="C:\\Program Files\\ConEmu\\ConEmu.exe,1"

	//[HKEY_CURRENT_USER\Software\Classes\Drive\shell\ConEmu inside\command]
	//@="C:\\Program Files\\ConEmu\\ConEmu.exe /inside /config shell /dir \"%1\" /cmd {powershell} -cur_console:n"

	int iSucceeded = 0;
	bool bHasLibraries = IsWindows7;

	for (int i = 1; i <= 6; i++)
	{
		_wsprintf(pszCmd, SKIPLEN(cchMax) L"\"%s\" ", gpConEmu->ms_ConEmuExe);
		if (asOpt && *asOpt)
		{
			bool bQ = (wcschr(asOpt, L' ') != NULL);
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
			if (!bHasLibraries)
				continue;
			pszRoot = L"Software\\Classes\\LibraryFolder\\Background\\shell";
			break;
		case 4:
			pszRoot = L"Software\\Classes\\Directory\\shell";
			_wcscat_c(pszCmd, cchMax, L"/dir \"%1\" ");
			break;
		case 5:
			pszRoot = L"Software\\Classes\\Drive\\shell";
			_wcscat_c(pszCmd, cchMax, L"/dir \"%1\" ");
			break;
		case 6:
			// Issue 1191: ConEmu was launched instead of explorer from taskbar pinned library icon
			continue;
			//if (!bHasLibraries)
			//	continue;
			//pszRoot = L"Software\\Classes\\LibraryFolder\\shell";
			//_wcscat_c(pszCmd, cchMax, L"/dir \"%1\" ");
			//break;
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
			if (0 == RegCreateKeyEx(hkRoot, asName, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkConEmu, NULL))
			{
				// Если задана "иконка"
				if (asIcon)
					RegSetValueEx(hkConEmu, L"Icon", 0, REG_SZ, (LPBYTE)asIcon, (lstrlen(asIcon)+1)*sizeof(*asIcon));
				else
					RegDeleteValue(hkConEmu, L"Icon");

				// Команда
				HKEY hkCmd;
				if (0 == RegCreateKeyEx(hkConEmu, L"command", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkCmd, NULL))
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
	DeleteRegKeyRecursive(HKEY_CURRENT_USER, L"Software\\Classes\\LibraryFolder\\Background\\shell", asName);
	DeleteRegKeyRecursive(HKEY_CURRENT_USER, L"Software\\Classes\\LibraryFolder\\shell", asName);
}

// Issue 1191: ConEmu was launched instead of explorer from taskbar pinned library icon
void CSettings::UnregisterShellInvalids()
{
	HKEY hkDir;

	if (0 == RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Classes\\LibraryFolder\\shell", 0, KEY_READ, &hkDir))
	{
		int iOthers = 0;
		MArray<wchar_t*> lsNames;

		for (DWORD i = 0; i < 512; i++)
		{
			wchar_t szName[MAX_PATH+32] = {};
			wchar_t szCmd[MAX_PATH*4];
			DWORD cchMax = countof(szName) - 32;
			if (0 != RegEnumKeyEx(hkDir, i, szName, &cchMax, NULL, NULL, NULL, NULL))
				break;
			wchar_t* pszSlash = szName + _tcslen(szName);
			wcscat_c(szName, L"\\command");
			HKEY hkCmd = NULL;
			if (0 == RegOpenKeyEx(hkDir, szName, 0, KEY_READ, &hkCmd))
			{
				DWORD cbMax = sizeof(szCmd)-2;
				if (0 == RegQueryValueEx(hkCmd, NULL, NULL, NULL, (LPBYTE)szCmd, &cbMax))
				{
					szCmd[cbMax>>1] = 0;
					*pszSlash = 0;
					//LPCWSTR pszInside = StrStrI(szCmd, L"/inside");
					LPCWSTR pszConEmu = StrStrI(szCmd, L"conemu");
					if (pszConEmu)
						lsNames.push_back(lstrdup(szName));
					else
						iOthers++;
				}
				RegCloseKey(hkCmd);
			}
		}
		RegCloseKey(hkDir);


		wchar_t* pszName;
		while (lsNames.pop_back(pszName))
		{
			DeleteRegKeyRecursive(HKEY_CURRENT_USER, L"Software\\Classes\\LibraryFolder\\shell", pszName);
		}

		// If there is no other "commands" - try to delete "shell" subkey.
		// No worse if there are any other "non commands" - RegDeleteKey will do nothing.
		if ((iOthers == 0) && (0 == RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Classes\\LibraryFolder", 0, KEY_ALL_ACCESS, &hkDir)))
		{
			RegDeleteKey(hkDir, L"shell");
			RegCloseKey(hkDir);
		}
	}
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
				RegisterShell(szName, L"/here", SkipNonPrintable(szConfig), SkipNonPrintable(szShell), szIcon);
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
				wchar_t* pszCur = NULL;
				wchar_t* pszBuf = NULL;
				LPCWSTR  pszSet = L"";
				wchar_t szCmd[MAX_PATH+128];

				if (bEnabled)
				{
					pszCur = LoadAutorunValue(hkCmd, true);
					pszSet = pszCur;

					_wsprintf(szCmd, SKIPLEN(countof(szCmd)) L"\"%s\\Cmd_Autorun.cmd", gpConEmu->ms_ConEmuBaseDir);
					if (FileExists(szCmd+1))
					{
						wcscat_c(szCmd, bForceNewWnd ? L"\" \"/GHWND=NEW\"" : L"\"");

						if (pszCur == NULL)
						{
							pszSet = szCmd;
						}
						else
						{
							// Current "AutoRun" is not empty, need concatenate
							size_t cchAll = _tcslen(szCmd) + _tcslen(pszCur) + 5;
							pszBuf = (wchar_t*)malloc(cchAll*sizeof(*pszBuf));
							_ASSERTE(pszBuf);
							if (pszBuf)
							{
								_wcscpy_c(pszBuf, cchAll, szCmd);
								_wcscat_c(pszBuf, cchAll, L" & "); // conveyer next command indifferent to %errorlevel%
								_wcscat_c(pszBuf, cchAll, pszCur);
								// Ok, Set
								pszSet = pszBuf;
							}
						}
					}
					else
					{
						MsgBox(szCmd, MB_ICONSTOP, L"File not found", ghOpWnd);

						pszSet = pszCur ? pszCur : L"";
					}
				}
				else
				{
					pszCur = bForced ? NULL : LoadAutorunValue(hkCmd, true);
					pszSet = pszCur ? pszCur : L"";
				}

				DWORD cchLen = _tcslen(pszSet)+1;
				RegSetValueEx(hkCmd, L"AutoRun", 0, REG_SZ, (LPBYTE)pszSet, cchLen*sizeof(wchar_t));

				RegCloseKey(hkCmd);

				if (pszBuf && (pszBuf != pszCur) && (pszBuf != szCmd))
					free(pszBuf);
				SafeFree(pszCur);
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

	if (CSetDlgFonts::EnumFontsFinished())  // Если шрифты уже считаны
		OnInitDialog_CopyFonts(hWnd2, tThumbsFontName, tTilesFontName, 0); // можно скопировать список с вкладки [thi_Main]

	UINT nVal;

	nVal = gpSet->ThSet.Thumbs.nFontHeight;
	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, tThumbsFontSize), CSetDlgLists::eFSizesSmall, nVal, false);

	nVal = gpSet->ThSet.Tiles.nFontHeight;
	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, tTilesFontSize), CSetDlgLists::eFSizesSmall, nVal, false);

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
	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, tThumbMaxZoom), CSetDlgLists::eThumbMaxZoom, gpSet->ThSet.nMaxZoom, false);

	// Colors
	for(uint c = c32; c <= c34; c++)
		ColorSetEdit(hWnd2, c);

	nVal = gpSet->ThSet.crBackground.ColorIdx;
	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbThumbBackColorIdx), CSetDlgLists::eColorIdxTh, nVal, false);
	checkRadioButton(hWnd2, rbThumbBackColorIdx, rbThumbBackColorRGB,
	                 gpSet->ThSet.crBackground.UseIndex ? rbThumbBackColorIdx : rbThumbBackColorRGB);
	checkDlgButton(hWnd2, cbThumbPreviewBox, gpSet->ThSet.nPreviewFrame ? 1 : 0);
	nVal = gpSet->ThSet.crPreviewFrame.ColorIdx;
	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbThumbPreviewBoxColorIdx), CSetDlgLists::eColorIdxTh, nVal, false);
	checkRadioButton(hWnd2, rbThumbPreviewBoxColorIdx, rbThumbPreviewBoxColorRGB,
	                 gpSet->ThSet.crPreviewFrame.UseIndex ? rbThumbPreviewBoxColorIdx : rbThumbPreviewBoxColorRGB);
	checkDlgButton(hWnd2, cbThumbSelectionBox, gpSet->ThSet.nSelectFrame ? 1 : 0);
	nVal = gpSet->ThSet.crSelectFrame.ColorIdx;
	CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbThumbSelectionBoxColorIdx), CSetDlgLists::eColorIdxTh, nVal, false);
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

	return 0;
}

// tThumbsFontName, tTilesFontName, 0
void CSettings::OnInitDialog_CopyFonts(HWND hWnd2, int nList1, ...)
{
	DWORD bAlmostMonospace;
	int nIdx, nCount, i;
	wchar_t szFontName[128]; // не должно быть более 32
	HWND hMainPg = GetPage(thi_Main);
	nCount = SendDlgItemMessage(hMainPg, tFontFace, CB_GETCOUNT, 0, 0);

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
	va_end(argptr);


	for (i = 0; i < nCount; i++)
	{
		// Взять список шрифтов с главной страницы
		if (SendDlgItemMessage(hMainPg, tFontFace, CB_GETLBTEXT, i, (LPARAM) szFontName) > 0)
		{
			// Показывать [Raster Fonts WxH] смысла нет
			if (szFontName[0] == L'[' && !wcsncmp(szFontName+1, RASTER_FONTS_NAME, _tcslen(RASTER_FONTS_NAME)))
				continue;
			// В Thumbs & Tiles [bdf] пока не поддерживается
			int iLen = lstrlen(szFontName);
			if ((iLen > CE_BDF_SUFFIX_LEN) && !wcscmp(szFontName+iLen-CE_BDF_SUFFIX_LEN, CE_BDF_SUFFIX))
				continue;

			bAlmostMonospace = (DWORD)SendDlgItemMessage(hMainPg, tFontFace, CB_GETITEMDATA, i, 0);

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
		CSetDlgLists::SelectString(hWnd2, nCtrlIds[j], szFontName);
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

	LVCOLUMN col = {
		LVCF_WIDTH|LVCF_TEXT|LVCF_FMT, LVCFMT_LEFT,
		gpSetCls->EvalSize(60, esf_Horizontal|esf_CanUseDpi)};
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

	return 0;
}

LRESULT CSettings::OnInitDialog_Update(HWND hWnd2, bool abInitial)
{
	ConEmuUpdateSettings* p = &gpSet->UpdSet;

	SetDlgItemText(hWnd2, tUpdateVerLocation, p->UpdateVerLocation());

	checkDlgButton(hWnd2, cbUpdateCheckOnStartup, p->isUpdateCheckOnStartup);
	checkDlgButton(hWnd2, cbUpdateCheckHourly, p->isUpdateCheckHourly);
	checkDlgButton(hWnd2, cbUpdateConfirmDownload, !p->isUpdateConfirmDownload);
	checkRadioButton(hWnd2, rbUpdateStableOnly, rbUpdateLatestAvailable,
		(p->isUpdateUseBuilds==1) ? rbUpdateStableOnly : (p->isUpdateUseBuilds==3) ? rbUpdatePreview : rbUpdateLatestAvailable);

	checkDlgButton(hWnd2, cbUpdateInetTool, p->isUpdateInetTool);
	SetDlgItemText(hWnd2, tUpdateInetTool, p->GetUpdateInetToolCmd());

	checkDlgButton(hWnd2, cbUpdateUseProxy, p->isUpdateUseProxy);
	SetDlgItemText(hWnd2, tUpdateProxy, p->szUpdateProxy);
	SetDlgItemText(hWnd2, tUpdateProxyUser, p->szUpdateProxyUser);
	SetDlgItemText(hWnd2, tUpdateProxyPassword, p->szUpdateProxyPassword);

	OnButtonClicked(hWnd2, cbUpdateInetTool, 0); // Enable/Disable command field, button '...' and ‘Proxy’ fields

	int nPackage = p->UpdateDownloadSetup(); // 1-exe, 2-7zip
	checkRadioButton(hWnd2, rbUpdateUseExe, rbUpdateUseArc, (nPackage==1) ? rbUpdateUseExe : rbUpdateUseArc);
	wchar_t szCPU[4] = L"";
	SetDlgItemText(hWnd2, tUpdateExeCmdLine, p->UpdateExeCmdLine(szCPU));
	SetDlgItemText(hWnd2, tUpdateArcCmdLine, p->UpdateArcCmdLine());
	SetDlgItemText(hWnd2, tUpdatePostUpdateCmd, p->szUpdatePostUpdateCmd);
	EnableDlgItem(hWnd2, (nPackage==1) ? tUpdateArcCmdLine : tUpdateExeCmdLine, FALSE);
	// Show used or preferred installer bitness
	CEStr szFormat, szTitle; INT_PTR iLen;
	if ((iLen = GetString(hWnd2, rbUpdateUseExe, &szFormat.ms_Val)) > 0)
	{
		if (wcsstr(szFormat.ms_Val, L"%s") != NULL)
		{
			wchar_t* psz = szTitle.GetBuffer(iLen+4);
			if (psz)
			{
				_wsprintf(psz, SKIPLEN(iLen+4) szFormat.ms_Val, (nPackage == 1) ? szCPU : WIN3264TEST(L"x86",L"x64"));
				SetDlgItemText(hWnd2, rbUpdateUseExe, szTitle);
			}
		}
	}

	checkDlgButton(hWnd2, cbUpdateLeavePackages, p->isUpdateLeavePackages);
	SetDlgItemText(hWnd2, tUpdateDownloadPath, p->szUpdateDownloadPath);

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
	{
		pVCon->RCon()->UpdateCursorInfo();
	}
	UpdateFontInfo();
	if (pVCon)
	{
		UpdateConsoleMode(pVCon->RCon());
	}

	return 0;
}

//bool CSettings::GetColorRef(LPCWSTR pszText, COLORREF* pCR)
//{
//	if (!pszText || !*pszText)
//		return false;
//
//	bool result = false;
//	int r = 0, g = 0, b = 0;
//	const wchar_t *pch;
//	wchar_t *pchEnd;
//
//	if ((pszText[0] == L'#') || (pszText[0] == L'x' || pszText[0] == L'X') || (pszText[0] == L'0' && (pszText[1] == L'x' || pszText[1] == L'X')))
//	{
//		pch = (pszText[0] == L'0') ? (pszText+2) : (pszText+1);
//		// Считаем значение 16-ричным rgb кодом
//		pchEnd = NULL;
//		COLORREF clr = wcstoul(pch, &pchEnd, 16);
//		if (clr && (pszText[0] == L'#'))
//		{
//			// "#rrggbb", обменять местами rr и gg, нам нужен COLORREF (bbggrr)
//			clr = ((clr & 0xFF)<<16) | ((clr & 0xFF00)) | ((clr & 0xFF0000)>>16);
//		}
//		// Done
//		if (pchEnd && (pchEnd > (pszText+1)) && (clr <= 0xFFFFFF) && (*pCR != clr))
//		{
//			*pCR = clr;
//			result = true;
//		}
//	}
//	else
//	{
//		pch = (wchar_t*)wcspbrk(pszText, L"0123456789");
//		pchEnd = NULL;
//		r = pch ? wcstol(pch, &pchEnd, 10) : 0;
//		if (pchEnd && (pchEnd > pch))
//		{
//			pch = (wchar_t*)wcspbrk(pchEnd, L"0123456789");
//			pchEnd = NULL;
//			g = pch ? wcstol(pch, &pchEnd, 10) : 0;
//
//			if (pchEnd && (pchEnd > pch))
//			{
//				pch = (wchar_t*)wcspbrk(pchEnd, L"0123456789");
//				pchEnd = NULL;
//				b = pch ? wcstol(pch, &pchEnd, 10) : 0;
//			}
//
//			// decimal format of UltraEdit?
//			if ((r > 255) && !g && !b)
//			{
//				g = (r & 0xFF00) >> 8;
//				b = (r & 0xFF0000) >> 16;
//				r &= 0xFF;
//			}
//
//			// Достаточно ввода одной компоненты
//			if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255 && *pCR != RGB(r, g, b))
//			{
//				*pCR = RGB(r, g, b);
//				result = true;
//			}
//		}
//	}
//
//	return result;
//}

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
				NeedBackgroundUpdate();
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

		// Если таки изменился - обновим
		if (newBgColors && gpSet->nBgImageColors != newBgColors)
		{
			gpSet->nBgImageColors = newBgColors;
			NeedBackgroundUpdate();
			gpConEmu->Update(true);
		}

		break;
	} // case tBgImageColors:

	case tWndX:
	case tWndY:
		if (IsChecked(hWnd2, rNormal) == BST_CHECKED)
		{
			wchar_t *pVal = GetDlgItemTextPtr(hWnd2, TB);
			BOOL bValid = (pVal && isDigit(*pVal));
			EnableWindow(GetDlgItem(hWnd2, cbApplyPos), bValid);
			SafeFree(pVal);
		}
		break; // case tWndX: case tWndY:
	case tWndWidth:
	case tWndHeight:
		if (IsChecked(hWnd2, rNormal) == BST_CHECKED)
		{
			CESize sz = {0};
			wchar_t *pVal = GetDlgItemTextPtr(hWnd2, TB);
			BOOL bValid = (pVal && sz.SetFromString(false, pVal));
			EnableWindow(GetDlgItem(hWnd2, cbApplyPos), bValid);
			SafeFree(pVal);
		}
		break; // case tWndWidth: case tWndHeight:

	case tPadSize:
	{
		BOOL bPadOk = FALSE;
		UINT nNewPad = GetDlgItemInt(hWnd2, TB, &bPadOk, FALSE);

		if (nNewPad >= CENTERCONSOLEPAD_MIN && nNewPad <= CENTERCONSOLEPAD_MAX)
			gpSet->nCenterConsolePad = nNewPad;
		else if (nNewPad > CENTERCONSOLEPAD_MAX)
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
		break;
	}

	case tDarker:
	{
		DWORD newV;
		TCHAR tmp[10];
		GetDlgItemText(hWnd2, tDarker, tmp, countof(tmp));
		newV = _wtoi(tmp);

		if (newV < 256 && newV != gpSet->bgImageDarker)
		{
			SetBgImageDarker(newV, true);

			//gpSet->bgImageDarker = newV;
			//SendDlgItemMessage(hWnd2, slDarker, TBM_SETPOS, (WPARAM) true, (LPARAM) gpSet->bgImageDarker);

			//// Картинку может установить и плагин
			//if (gpSet->isShowBgImage && gpSet->sBgImage[0])
			//	LoadBackgroundFile(gpSet->sBgImage);
			//else
			//	NeedBackgroundUpdate();

			//gpConEmu->Update(true);
		}
		break;
	} //case tDarker:

	case tCursorFixedSize:
	case tInactiveCursorFixedSize:
	case tCursorMinSize:
	case tInactiveCursorMinSize:
	{
		if (OnEditChanged_Cursor(hWnd2, wParam, lParam, &gpSet->AppStd))
		{
			gpConEmu->Update(true);
		}
		break;
	} //case tCursorFixedSize, tInactiveCursorFixedSize, tCursorMinSize, tInactiveCursorMinSize


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
		UINT nHotKey = CHotKeyDialog::dlgGetHotkey(hWnd2, hkHotKeySelect, lbHotKeyList);

		if (mp_ActiveHotKey && mp_ActiveHotKey->CanChangeVK())
		{
			DWORD nCurMods = (CEHOTKEY_MODMASK & mp_ActiveHotKey->GetVkMod());
			if (!nCurMods)
				nCurMods = CEHOTKEY_NOMOD;

			SetHotkeyVkMod(mp_ActiveHotKey, nHotKey | nCurMods);

			FillHotKeysList(hWnd2, FALSE);
		}
		break;
	} // case hkHotKeySelect:

	case tTabSkipWords:
	{
		SafeFree(gpSet->pszTabSkipWords);
		gpSet->pszTabSkipWords = GetDlgItemTextPtr(hWnd2, TB);
		gpConEmu->mp_TabBar->Update(TRUE);
		break;
	}
	case tTabConsole:
	case tTabModifiedSuffix:
	case tTabPanels:
	case tTabViewer:
	case tTabEditor:
	case tTabEditorMod:
	{
		wchar_t temp[MAX_PATH]; temp[0] = 0;

		if (GetDlgItemText(hWnd2, TB, temp, countof(temp)) && temp[0])
		{
			temp[31] = 0; // страховка

			//03.04.2013, via gmail, просили не добавлять автоматом %s
			//if (wcsstr(temp, L"%s") || wcsstr(temp, L"%n"))
			{
				if (TB == tTabConsole)
					wcscpy_c(gpSet->szTabConsole, temp);
				else if (TB == tTabModifiedSuffix)
					lstrcpyn(gpSet->szTabModifiedSuffix, temp, countof(gpSet->szTabModifiedSuffix));
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

	case tTabFlashCounter:
	{
		GetDlgItemSigned(hWnd2, tTabFlashCounter, gpSet->nTabFlashChanged, -1, 0);
		break;
	} // case tTabFlashCounter:

	case tAdminSuffix:
	{
		wchar_t szNew[64];
		GetDlgItemText(hWnd2, tAdminSuffix, szNew, countof(szNew));
		if (lstrcmp(szNew, gpSet->szAdminTitleSuffix) != 0)
		{
			wcscpy_c(gpSet->szAdminTitleSuffix, szNew);
			gpConEmu->mp_TabBar->Update(TRUE);
		}
		break;
	} // case tAdminSuffix:

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
	case tUpdateInetTool:
		if (gpSet->UpdSet.isUpdateInetTool)
			GetString(hWnd2, TB, &gpSet->UpdSet.szUpdateInetTool);
		break;
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

			int iCur = CSetDlgLists::GetListboxCurSel(hWnd2, lbCmdTasks, true);
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
				CSetDlgLists::ListBoxMultiSel(hWnd2, lbCmdTasks, iCur);
				mb_IgnoreCmdGroupList = false;
			}

			SafeFree(pszName);
			SafeFree(pszCmds);
			SafeFree(pszGuiArgs);
		}
		break;
	/* *** Command groups *** */

	case tSetCommands:
		{
			size_t cchMax = gpSet->psEnvironmentSet ? (_tcslen(gpSet->psEnvironmentSet)+1) : 0;
			MyGetDlgItemText(hWnd2, tSetCommands, cchMax, gpSet->psEnvironmentSet);
		}
		break;

	case tDefaultTerminal:
		{
			wchar_t* pszApps = GetDlgItemTextPtr(hWnd2, tDefaultTerminal);
			if (!pszApps || !*pszApps)
			{
				SafeFree(pszApps);
				pszApps = lstrdup(DEFAULT_TERMINAL_APPS/*L"explorer.exe"*/);
				SetDlgItemText(hWnd2, tDefaultTerminal, pszApps);
			}
			gpSet->SetDefaultTerminalApps(pszApps);
			SafeFree(pszApps);
		}
		break;

	case tAnsiLogPath:
		{
			SafeFree(gpSet->pszAnsiLog);
			gpSet->pszAnsiLog = GetDlgItemTextPtr(hWnd2, tAnsiLogPath);
		}
		break;

	/* *** GUI Macro - hotkeys *** */
	case tGuiMacro:
		if (mp_ActiveHotKey && (mp_ActiveHotKey->HkType == chk_Macro))
		{
			MyGetDlgItemText(hWnd2, tGuiMacro, mp_ActiveHotKey->cchGuiMacroMax, mp_ActiveHotKey->GuiMacro);
			FillHotKeysList(hWnd2, FALSE);
		}
		break;


	case tQuakeAnimation:
		{
			WORD TB = LOWORD(wParam);
			BOOL lbOk = FALSE;
			UINT nNewVal = GetDlgItemInt(hWnd2, TB, &lbOk, FALSE);

			gpSet->nQuakeAnimation = (nNewVal <= QUAKEANIMATION_MAX) ? nNewVal : QUAKEANIMATION_MAX;
		}
		break;

	case tHideCaptionAlwaysFrame:
		{
			WORD TB = LOWORD(wParam);
			BOOL lbOk = FALSE;
			int nNewVal = GetDlgItemInt(hWnd2, TB, &lbOk, TRUE);

			if (lbOk && (gpSet->nHideCaptionAlwaysFrame != ((nNewVal < 0) ? 255 : (BYTE)nNewVal)))
			{
				gpSet->nHideCaptionAlwaysFrame = (nNewVal < 0) ? 255 : (BYTE)nNewVal;
				gpConEmu->OnHideCaption();
				gpConEmu->UpdateWindowRgn();
			}
		}
		break;
	case tHideCaptionAlwaysDelay:
	case tHideCaptionAlwaysDissapear:
	case tScrollAppearDelay:
	case tScrollDisappearDelay:
		if (HIWORD(wParam) == EN_CHANGE)
		{
			WORD TB = LOWORD(wParam);
			BOOL lbOk = FALSE;
			UINT nNewVal = GetDlgItemInt(hWnd2, TB, &lbOk, FALSE);

			if (lbOk)
			{
				switch (TB)
				{
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
				}
			}
		}
		break;

	/* *** Console text selections - intelligent exclusions *** */
	case tCTSIntelligentExceptions:
		if (HIWORD(wParam) == EN_CHANGE)
		{
			wchar_t* pszApps = GetDlgItemTextPtr(hWnd2, tCTSIntelligentExceptions);
			gpSet->SetIntelligentExceptions(pszApps);
			SafeFree(pszApps);
		}
		break;

	default:

		switch (GetPageId(hWnd2))
		{

		case thi_Views:
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
				} // switch (TB)
			} // if (bValOk)

			if (TB >= tc32 && TB <= tc38)
			{
				COLORREF color = 0;

				if (gpSetCls->GetColorById(TB - (tc0-c0), &color))
				{
					if (gpSetCls->GetColorRef(hWnd2, TB, &color))
					{
						if (gpSetCls->SetColorById(TB - (tc0-c0), color))
						{
							InvalidateCtrl(GetDlgItem(hWnd2, TB - (tc0-c0)), TRUE);
							// done
						}
					}
				}
			}

			break;
		} // case thi_Views:

		case thi_Colors:
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

						InvalidateCtrl(GetDlgItem(hWnd2, TB - (tc0-c0)), TRUE);
					}
				}
			}

			break;
		} // case thi_Colors:

		case thi_Status:
		{
			COLORREF color = 0;

			if ((TB >= tc35 && TB <= tc37)
				&& GetColorById(TB - (tc0-c0), &color))
			{
				if (GetColorRef(hWnd2, TB, &color))
				{
					if (SetColorById(TB - (tc0-c0), color))
					{
						InvalidateCtrl(GetDlgItem(hWnd2, TB - (tc0-c0)), TRUE);
					}
				}
			}

			break;
		} // case thi_Status:

		} // switch (GetPageId(hWnd2))

	// end of default:
	} // switch (TB)

	return 0;
}

bool CSettings::OnEditChanged_Cursor(HWND hWnd2, WPARAM wParam, LPARAM lParam, AppSettings* pApp)
{
	bool bChanged = false;
	WORD TB = LOWORD(wParam);

	switch (TB)
	{
		case tCursorFixedSize:
		case tInactiveCursorFixedSize:
		{
			BOOL lbOk = FALSE;
			UINT nNewVal = GetDlgItemInt(hWnd2, TB, &lbOk, FALSE);
			if (lbOk)
			{
				UINT nMinSize = (TB == tCursorFixedSize) ? CURSORSIZE_MIN : 0;
				UINT nMaxSize = CURSORSIZE_MAX;
				if ((nNewVal >= nMinSize) && (nNewVal <= nMaxSize))
				{
					CECursorType* pCur = (TB == tCursorFixedSize) ? &pApp->CursorActive : &pApp->CursorInactive;

					if (pCur->FixedSize != nNewVal)
					{
						pCur->FixedSize = nNewVal;
						bChanged = true;
					}
				}
			}

			break;
		} //case tCursorFixedSize, tInactiveCursorFixedSize:

		case tCursorMinSize:
		case tInactiveCursorMinSize:
		{
			BOOL lbOk = FALSE;
			UINT nNewVal = GetDlgItemInt(hWnd2, TB, &lbOk, FALSE);
			if (lbOk)
			{
				UINT nMinSize = (TB == tCursorMinSize) ? CURSORSIZEPIX_MIN : 0;
				UINT nMaxSize = CURSORSIZEPIX_MAX;
				if ((nNewVal >= nMinSize) && (nNewVal <= nMaxSize))
				{
					CECursorType* pCur = (TB == tCursorMinSize) ? &pApp->CursorActive : &pApp->CursorInactive;

					if (pCur->MinSize != nNewVal)
					{
						pCur->MinSize = nNewVal;
						bChanged = true;
					}
				}
			}

			break;
		} //case tCursorMinSize, tInactiveCursorMinSize:

		default:
			_ASSERTE(FALSE && "Unprocessed edit");
	}

	return bChanged;
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

	case tUnicodeRanges:
		// Do not required actually, the button "Apply" is enabled by default
		EnableWindow(GetDlgItem(hWnd2, cbUnicodeRangesApply), TRUE);
		if (HIWORD(wParam) == CBN_SELCHANGE)
			PostMessage(hWnd2, WM_COMMAND, cbUnicodeRangesApply, 0);
		break;

	case lbBgPlacement:
	{
		BYTE bg = 0;
		CSetDlgLists::GetListBoxItem(hWnd2, lbBgPlacement, CSetDlgLists::eBgOper, bg);
		gpSet->bgOperation = bg;
		gpSetCls->LoadBackgroundFile(gpSet->sBgImage, true);
		NeedBackgroundUpdate();
		gpConEmu->Update(true);
		break;
	}

	case lbLDragKey:
	{
		BYTE VkMod = 0;
		CSetDlgLists::GetListBoxItem(hWnd2, wId, CSetDlgLists::eKeys, VkMod);
		gpSet->SetHotkeyById(vkLDragKey, VkMod);
		break;
	}
	case lbRDragKey:
	{
		BYTE VkMod = 0;
		CSetDlgLists::GetListBoxItem(hWnd2, wId, CSetDlgLists::eKeys, VkMod);
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

		CVConGroup::OnUpdateFarSettings();
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
				//GetListBoxByte(hWnd2, wId, SettingsNS::KeysHot, vk);
				CSetDlgLists::GetListBoxItem(hWnd2, wId, CSetDlgLists::eKeysHot, vk);

				SetHotkeyField(GetDlgItem(hWnd2, hkHotKeySelect), vk);
				//SendDlgItemMessage(hWnd2, hkHotKeySelect, HKM_SETHOTKEY, vk|(vk==VK_DELETE ? (HOTKEYF_EXT<<8) : 0), 0);

				DWORD nMod = (CEHOTKEY_MODMASK & mp_ActiveHotKey->GetVkMod());
				if (nMod == 0)
				{
					// Если модификатора вообще не было - ставим Win
					BYTE b = VK_LWIN;
					CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbHotKeyMod1), CSetDlgLists::eModifiers, b, false);
					nMod = (VK_LWIN << 8);
				}
				SetHotkeyVkMod(mp_ActiveHotKey, ((DWORD)vk) | nMod);
			}
			else if (mp_ActiveHotKey->HkType == chk_Modifier)
			{
				CSetDlgLists::GetListBoxItem(hWnd2, wId, CSetDlgLists::eKeys, vk);
				SetHotkeyField(GetDlgItem(hWnd2, hkHotKeySelect), 0);
				SetHotkeyVkMod(mp_ActiveHotKey, vk);
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
			CSetDlgLists::GetListBoxItem(hWnd2, lbHotKeyMod1+i, CSetDlgLists::eModifiers, vk);
			BYTE vkChange = vk;

			// Некоторые модификаторы НЕ допустимы при регистрации глобальных хоткеев (ограничения WinAPI)
			if (mp_ActiveHotKey && (mp_ActiveHotKey->HkType == chk_Global || mp_ActiveHotKey->HkType == chk_Local))
			{
				switch (vk)
				{
				case VK_APPS:
					vkChange = 0; break;
				case VK_LMENU: case VK_RMENU:
					vkChange = VK_MENU; break;
				case VK_LCONTROL: case VK_RCONTROL:
					vkChange = VK_CONTROL; break;
				case VK_LSHIFT: case VK_RSHIFT:
					vkChange = VK_SHIFT; break;
				}
			}

			if (vkChange != vk)
			{
				vk = vkChange;
				CSetDlgLists::FillListBoxItems(GetDlgItem(hWnd2, lbHotKeyMod1+i), CSetDlgLists::eModifiers, vkChange, false);
			}

			if (vk)
				nModifers = ConEmuHotKey::SetModifier(nModifers, vk, false);
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
				m_HotKeys.UpdateNumberModifier();
			}
			else if (mp_ActiveHotKey->HkType == chk_ArrHost)
			{
				if (!nModifers)
					nModifers = (VK_LWIN << 8);
				// Для этой группы - модификаторы назначаются "чохом"
				_ASSERTE((nModifers & 0xFF) == 0); // тут данные в старших трех байтах
				gpSet->nHostkeyArrowModifier = (nModifers >> 8); // а тут в младших трех
				m_HotKeys.UpdateArrowModifier();
			}
			else //if (mp_ActiveHotKey->VkMod)
			{
				if (!nModifers)
					nModifers = CEHOTKEY_NOMOD;
				SetHotkeyVkMod(mp_ActiveHotKey, (cvk_VK_MASK & mp_ActiveHotKey->GetVkMod()) | nModifers);
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
			UINT val;
			INT_PTR nSel = SendDlgItemMessage(hWnd2, wId, CB_GETCURSEL, 0, 0);

			switch (wId)
			{
			case tTabFontFace:
				SendDlgItemMessage(hWnd2, wId, CB_GETLBTEXT, nSel, (LPARAM)gpSet->sTabFontFace);
				break;
			case tTabFontHeight:
				if (CSetDlgLists::GetListBoxItem(hWnd2, wId, CSetDlgLists::eFSizesSmall, val))
					gpSet->nTabFontHeight = val;
				break;
			case tTabFontCharset:
				if (CSetDlgLists::GetListBoxItem(hWnd2, wId, CSetDlgLists::eCharSets, val))
					gpSet->nTabFontCharSet = val;
				else
					gpSet->nTabFontCharSet = DEFAULT_CHARSET;
			}
		}
		gpConEmu->RecreateControls(true, false, true);
		break;
	} // tTabFontFace, tTabFontHeight, tTabFontCharset

	case tTabBarDblClickAction:
	case tTabBtnDblClickAction:
	{
		if (HIWORD(wParam) == CBN_SELCHANGE)
		{
			UINT val;
			INT_PTR nSel = SendDlgItemMessage(hWnd2, wId, CB_GETCURSEL, 0, 0);

			switch(wId)
			{
			case tTabBarDblClickAction:
				if (CSetDlgLists::GetListBoxItem(hWnd2, wId, CSetDlgLists::eTabBarDblClickActions, val))
					gpSet->nTabBarDblClickAction = val;
				else
					gpSet->nTabBarDblClickAction = TABBAR_DEFAULT_CLICK_ACTION;
				break;
			case tTabBtnDblClickAction:
				if (CSetDlgLists::GetListBoxItem(hWnd2, wId, CSetDlgLists::eTabBtnDblClickActions, val))
					gpSet->nTabBtnDblClickAction = val;
				else
					gpSet->nTabBtnDblClickAction = TABBTN_DEFAULT_CLICK_ACTION;
				break;
			}
		}
		break;
	} // tTabDblClickAction

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
		}
		else if (HIWORD(wParam) == CBN_SELCHANGE)
		{
			UINT val;
			INT_PTR nSel = SendDlgItemMessage(hWnd2, wId, CB_GETCURSEL, 0, 0);

			switch (wId)
			{
			case tStatusFontFace:
				SendDlgItemMessage(hWnd2, wId, CB_GETLBTEXT, nSel, (LPARAM)gpSet->sStatusFontFace);
				break;
			case tStatusFontHeight:
				if (CSetDlgLists::GetListBoxItem(hWnd2, wId, CSetDlgLists::eFSizesSmall, val))
					gpSet->nStatusFontHeight = val;
				break;
			case tStatusFontCharset:
				if (CSetDlgLists::GetListBoxItem(hWnd2, wId, CSetDlgLists::eCharSets, val))
					gpSet->nStatusFontCharSet = val;
				else
					gpSet->nStatusFontCharSet = DEFAULT_CHARSET;
			}
		}
		gpConEmu->RecreateControls(false, true, true);
		break;
	} // tStatusFontFace, tStatusFontHeight, tStatusFontCharset

	case lbCmdTasks:
	{
		if (mb_IgnoreCmdGroupList)
			break;

		mb_IgnoreCmdGroupEdit = true;
		const CommandTasks* pCmd = NULL;
		int* Items = NULL;
		int iSelCount = CSetDlgLists::GetListboxSelection(hWnd2, lbCmdTasks, Items);
		int iCur = (iSelCount == 1) ? Items[0] : -1;
		if (iCur >= 0)
			pCmd = gpSet->CmdTaskGet(iCur);
		BOOL lbEnable = FALSE;
		if (pCmd)
		{
			_ASSERTE(pCmd->pszName);
			wchar_t* pszNoBrk = lstrdup(!pCmd->pszName ? L"" : (pCmd->pszName[0] != TaskBracketLeft) ? pCmd->pszName : (pCmd->pszName+1));
			if (*pszNoBrk && (pszNoBrk[_tcslen(pszNoBrk)-1] == TaskBracketRight))
				pszNoBrk[_tcslen(pszNoBrk)-1] = 0;
			SetDlgItemText(hWnd2, tCmdGroupName, pszNoBrk);
			SafeFree(pszNoBrk);

			wchar_t szKey[128] = L"";
			SetDlgItemText(hWnd2, tCmdGroupKey, pCmd->HotKey.GetHotkeyName(szKey));

			SetDlgItemText(hWnd2, tCmdGroupGuiArg, pCmd->pszGuiArgs ? pCmd->pszGuiArgs : L"");
			SetDlgItemText(hWnd2, tCmdGroupCommands, pCmd->pszCommands ? pCmd->pszCommands : L"");

			checkDlgButton(hWnd2, cbCmdGrpDefaultNew, (pCmd->Flags & CETF_NEW_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
			checkDlgButton(hWnd2, cbCmdGrpDefaultCmd, (pCmd->Flags & CETF_CMD_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
			checkDlgButton(hWnd2, cbCmdGrpTaskbar, ((pCmd->Flags & CETF_NO_TASKBAR) == CETF_NONE) ? BST_CHECKED : BST_UNCHECKED);
			checkDlgButton(hWnd2, cbCmdGrpToolbar, (pCmd->Flags & CETF_ADD_TOOLBAR) ? BST_CHECKED : BST_UNCHECKED);

			lbEnable = TRUE;
		}
		else
		{
			SetDlgItemText(hWnd2, tCmdGroupName, L"");
			SetDlgItemText(hWnd2, tCmdGroupGuiArg, L"");
			SetDlgItemText(hWnd2, tCmdGroupCommands, L"");
		}
		//for (size_t i = 0; i < countof(SettingsNS::nTaskCtrlId); i++)
		//	EnableWindow(GetDlgItem(hWnd2, SettingsNS::nTaskCtrlId[i]), lbEnable);
		CSetDlgLists::EnableDlgItems(hWnd2, CSetDlgLists::eTaskCtrlId, lbEnable);
		mb_IgnoreCmdGroupEdit = false;
		delete[] Items;

		break;
	} // lbCmdTasks:


	case lbGotoEditorCmd:
	{
		if ((HIWORD(wParam) == CBN_EDITCHANGE) || (HIWORD(wParam) == CBN_SELCHANGE))
		{
			GetString(hWnd2, lbGotoEditorCmd, &gpSet->sFarGotoEditor, NULL, (HIWORD(wParam) == CBN_SELCHANGE));
		}
		break;
	} // lbGotoEditorCmd

	// Far macros:
	case tRClickMacro:
	case tSafeFarCloseMacro:
	case tCloseTabMacro:
	case tSaveAllMacro:
	{
		wchar_t** ppszMacro = NULL;
		LPCWSTR pszDefaultMacro = NULL;
		switch (wId)
		{
			case tRClickMacro:
				ppszMacro = &gpSet->sRClickMacro; pszDefaultMacro = gpSet->RClickMacroDefault(fmv_Default);
				break;
			case tSafeFarCloseMacro:
				ppszMacro = &gpSet->sSafeFarCloseMacro; pszDefaultMacro = gpSet->SafeFarCloseMacroDefault(fmv_Default);
				break;
			case tCloseTabMacro:
				ppszMacro = &gpSet->sTabCloseMacro; pszDefaultMacro = gpSet->TabCloseMacroDefault(fmv_Default);
				break;
			case tSaveAllMacro:
				ppszMacro = &gpSet->sSaveAllMacro; pszDefaultMacro = gpSet->SaveAllMacroDefault(fmv_Default);
				break;
		}

		if (HIWORD(wParam) == CBN_EDITCHANGE)
		{
			GetString(hWnd2, wId, ppszMacro, pszDefaultMacro, false);
		}
		else if (HIWORD(wParam) == CBN_SELCHANGE)
		{
			GetString(hWnd2, wId, ppszMacro, pszDefaultMacro, true);
		}
		break;
	} // case tRClickMacro, tSafeFarCloseMacro, tCloseTabMacro, tSaveAllMacro


	case lbStatusAvailable:
	case lbStatusSelected:
		break;


	default:
		switch (GetPageId(hWnd2))
		{

		case thi_Views:
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
					default:
						_ASSERTE(FALSE && "EditBox was not processed");
				}
			}
			else if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				UINT val;
				INT_PTR nSel = SendDlgItemMessage(hWnd2, wId, CB_GETCURSEL, 0, 0);

				switch (wId)
				{
					case lbThumbBackColorIdx:
						gpSet->ThSet.crBackground.ColorIdx = nSel;
						InvalidateCtrl(GetDlgItem(hWnd2, c32), TRUE);
						break;
					case lbThumbPreviewBoxColorIdx:
						gpSet->ThSet.crPreviewFrame.ColorIdx = nSel;
						InvalidateCtrl(GetDlgItem(hWnd2, c33), TRUE);
						break;
					case lbThumbSelectionBoxColorIdx:
						gpSet->ThSet.crSelectFrame.ColorIdx = nSel;
						InvalidateCtrl(GetDlgItem(hWnd2, c34), TRUE);
						break;
					case tThumbsFontName:
						SendDlgItemMessage(hWnd2, wId, CB_GETLBTEXT, nSel, (LPARAM)gpSet->ThSet.Thumbs.sFontName);
						break;
					case tThumbsFontSize:
						if (CSetDlgLists::GetListBoxItem(hWnd2, wId, CSetDlgLists::eFSizesSmall, val))
							gpSet->ThSet.Thumbs.nFontHeight = val;
						break;
					case tTilesFontName:
						SendDlgItemMessage(hWnd2, wId, CB_GETLBTEXT, nSel, (LPARAM)gpSet->ThSet.Tiles.sFontName);
						break;
					case tTilesFontSize:
						if (CSetDlgLists::GetListBoxItem(hWnd2, wId, CSetDlgLists::eFSizesSmall, val))
							gpSet->ThSet.Tiles.nFontHeight = val;
						break;
					case tThumbMaxZoom:
						gpSet->ThSet.nMaxZoom = max(100,((nSel+1)*100));
					default:
						_ASSERTE(FALSE && "ListBox was not processed");
				}
			}

			break;
		} // case thi_Views:

		case thi_Colors:
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
			else if (wId == lbDefaultColors)
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
					const ColorPalette* pPal = NULL;

					if (nIdx == 0)
						pPal = &gLastColors;
					else if ((pPal = gpSet->PaletteGet(nIdx-1)) == NULL)
						return 0; // неизвестный набор

					gpSetCls->ChangeCurrentPalette(pPal, false);

					return 0;
				}
				else
				{
					return 0;
				}
			}

			gpConEmu->Update(true);

			break;
		} // case thi_Colors:

		case thi_MarkCopy:
		{
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				switch (wId)
				{
				case lbCTSBlockSelection:
					{
						BYTE VkMod = 0;
						CSetDlgLists::GetListBoxItem(hWnd2, lbCTSBlockSelection, CSetDlgLists::eKeysAct, VkMod);
						gpSet->SetHotkeyById(vkCTSVkBlock, VkMod);
						CheckSelectionModifiers(hWnd2);
					} break;
				case lbCTSTextSelection:
					{
						BYTE VkMod = 0;
						CSetDlgLists::GetListBoxItem(hWnd2, lbCTSTextSelection, CSetDlgLists::eKeysAct, VkMod);
						gpSet->SetHotkeyById(vkCTSVkText, VkMod);
						CheckSelectionModifiers(hWnd2);
					} break;
				case lbCTSEOL:
					{
						BYTE eol = 0;
						CSetDlgLists::GetListBoxItem(hWnd2, lbCTSEOL, CSetDlgLists::eCRLF, eol);
						gpSet->AppStd.isCTSEOL = eol;
					} // lbCTSEOL
					break;
				case lbCopyFormat:
					{
						BYTE CopyFormat = 0;
						CSetDlgLists::GetListBoxItem(hWnd2, lbCopyFormat, CSetDlgLists::eCopyFormat, CopyFormat);
						gpSet->isCTSHtmlFormat = CopyFormat;
					} // lbCopyFormat
					break;
				case lbCTSForeIdx:
					{
						UINT nFore = 0;
						CSetDlgLists::GetListBoxItem(hWnd2, lbCTSForeIdx, CSetDlgLists::eColorIdx16, nFore);
						gpSet->isCTSColorIndex = (gpSet->isCTSColorIndex & 0xF0) | (nFore & 0xF);
						InvalidateRect(GetDlgItem(hWnd2, stCTSPreview), NULL, FALSE);
						gpConEmu->Update(true);
					} break;
				case lbCTSBackIdx:
					{
						UINT nBack = 0;
						CSetDlgLists::GetListBoxItem(hWnd2, lbCTSBackIdx, CSetDlgLists::eColorIdx16, nBack);
						gpSet->isCTSColorIndex = (gpSet->isCTSColorIndex & 0xF) | ((nBack & 0xF) << 4);
						InvalidateRect(GetDlgItem(hWnd2, stCTSPreview), NULL, FALSE);
						gpConEmu->Update(true);
					} break;
				default:
					_ASSERTE(FALSE && "ListBox was not processed");
				}
			} // if (HIWORD(wParam) == CBN_SELCHANGE)

			break;
		} // case thi_MarkCopy:

		case thi_Hilight:
		{
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				switch (wId)
				{
				case lbFarGotoEditorVk:
					{
						BYTE VkMod = 0;
						CSetDlgLists::GetListBoxItem(hWnd2, lbFarGotoEditorVk, CSetDlgLists::eKeysAct, VkMod);
						gpSet->SetHotkeyById(vkFarGotoEditorVk, VkMod);
					} break;
				default:
					_ASSERTE(FALSE && "ListBox was not processed");
				}
			}

			break;
		} // case thi_Hilight:

		case thi_KeybMouse:
		{
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				switch (wId)
				{
				case lbCTSClickPromptPosition:
					{
						BYTE VkMod = 0;
						CSetDlgLists::GetListBoxItem(hWnd2, lbCTSClickPromptPosition, CSetDlgLists::eKeysAct, VkMod);
						gpSet->SetHotkeyById(vkCTSVkPromptClk, VkMod);
						CheckSelectionModifiers(hWnd2);
					} break;
				case lbCTSActAlways:
					{
						BYTE VkMod = 0;
						CSetDlgLists::GetListBoxItem(hWnd2, lbCTSActAlways, CSetDlgLists::eKeysAct, VkMod);
						gpSet->SetHotkeyById(vkCTSVkAct, VkMod);
					} break;
				case lbCTSRBtnAction:
					{
						CSetDlgLists::GetListBoxItem(hWnd2, lbCTSRBtnAction, CSetDlgLists::eClipAct, gpSet->isCTSRBtnAction);
					} break;
				case lbCTSMBtnAction:
					{
						CSetDlgLists::GetListBoxItem(hWnd2, lbCTSMBtnAction, CSetDlgLists::eClipAct, gpSet->isCTSMBtnAction);
					} break;
				default:
					_ASSERTE(FALSE && "ListBox was not processed");
				}
			} // if (HIWORD(wParam) == CBN_SELCHANGE)

			break;
		} // case thi_KeybMouse:

		default:
		{
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				_ASSERTE(FALSE && "ListBox was not processed");
			}
		}

		} // switch (GetPageId(hWnd2))
	} // switch (wId)
	return 0;
}

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
					if (m_Pages[i].hPage == NULL)
					{
						SetCursor(LoadCursor(NULL,IDC_WAIT));
						CreatePage(&(m_Pages[i]));
					}
					else
					{
						SendMessage(m_Pages[i].hPage, mn_ActivateTabMsg, 1, (LPARAM)&(m_Pages[i]));
						ProcessDpiChange(&(m_Pages[i]));
					}
					ShowWindow(m_Pages[i].hPage, SW_SHOW);
					mn_LastActivedTab = gpSetCls->m_Pages[i].PageID;
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

	if (!IdShowPage && gpSetCls->mn_LastActivedTab)
		IdShowPage = gpSetCls->mn_LastActivedTab;

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

	gColorBoxMap.Reset();

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

	if (mh_CtlColorBrush)
	{
		DeleteObject(mh_CtlColorBrush);
		mh_CtlColorBrush = NULL;
	}

	// mp_DpiAware and others are cleared in ClearPages()
	ClearPages();
	mp_ActiveHotKey = NULL;
	gbLastColorsOk = FALSE;

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
	InitVars_Hotkeys();

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

					default:
						gpSetCls->OnButtonClicked(hWnd2, wParam, lParam);
					}
				}
				break;

				case EN_CHANGE:
				{
					gpSetCls->OnEditChanged(hWnd2, wParam, lParam);
				}
				break;

				case CBN_EDITCHANGE:
				case CBN_SELCHANGE:
				{
					gpSetCls->OnComboBox(hWnd2, wParam, lParam);
				}
				break;
			}
			break;

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
				for (ConEmuSetupPages* p = gpSetCls->m_Pages; p->PageID; p++)
				{
					if (p->hPage)
					{
						p->DpiChanged = true;
						if (IsWindowVisible(p->hPage))
						{
							gpSetCls->ProcessDpiChange(p);
						}
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

// Общая DlgProc на _все_ вкладки
INT_PTR CSettings::pageOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	static bool bSkipSelChange = false;

	ConEmuSetupPages* pPage = NULL;
	TabHwndIndex pgId = gpSetCls->GetPageId(hWnd2, &pPage);

	if (pPage && pPage->pDpiAware && pPage->pDpiAware->ProcessDpiMessages(hWnd2, messg, wParam, lParam))
	{
		return TRUE;
	}

	if ((messg == WM_INITDIALOG) || (messg == gpSetCls->mn_ActivateTabMsg))
	{
		if (!lParam)
		{
			_ASSERTE(lParam!=0);
			return 0;
		}

		ConEmuSetupPages* p = (ConEmuSetupPages*)lParam;
		bool bInitial = (messg == WM_INITDIALOG);

		if (bInitial)
		{
			_ASSERTE(p->PageIndex>=0 && p->hPage==NULL);
			p->hPage = hWnd2;

			HWND hPlace = GetDlgItem(ghOpWnd, tSetupPagePlace);
			RECT rcClient; GetWindowRect(hPlace, &rcClient);
			MapWindowPoints(NULL, ghOpWnd, (LPPOINT)&rcClient, 2);
			if (p->pDpiAware)
				p->pDpiAware->Attach(hWnd2, ghOpWnd, p->pDialog);
			MoveWindowRect(hWnd2, rcClient);
		}
		else
		{
			_ASSERTE(p->PageIndex>=0 && p->hPage==hWnd2);
			// обновить контролы страничек при активации вкладки
		}

		switch (p->PageID)
		{
		case IDD_SPG_MAIN:
			gpSetCls->OnInitDialog_Main(hWnd2, bInitial);
			break;
		case IDD_SPG_WNDSIZEPOS:
			{
			bool lbOld = bSkipSelChange; bSkipSelChange = true;
			gpSetCls->OnInitDialog_WndSizePos(hWnd2, bInitial);
			bSkipSelChange = lbOld;
			}
			break;
		case IDD_SPG_BACK:
			gpSetCls->OnInitDialog_Background(hWnd2, bInitial);
			break;
		case IDD_SPG_SHOW:
			gpSetCls->OnInitDialog_Show(hWnd2, bInitial);
			break;
		case IDD_SPG_CONFIRM:
			gpSetCls->OnInitDialog_Confirm(hWnd2, bInitial);
			break;
		case IDD_SPG_TASKBAR:
			gpSetCls->OnInitDialog_Taskbar(hWnd2, bInitial);
			break;
		case IDD_SPG_CURSOR:
			gpSetCls->OnInitDialog_Cursor(hWnd2, bInitial);
			break;
		case IDD_SPG_STARTUP:
			gpSetCls->OnInitDialog_Startup(hWnd2, bInitial);
			break;
		case IDD_SPG_FEATURE:
			gpSetCls->OnInitDialog_Ext(hWnd2, bInitial);
			break;
		case IDD_SPG_COMSPEC:
			gpSetCls->OnInitDialog_Comspec(hWnd2, bInitial);
			break;
		case IDD_SPG_ENVIRONMENT:
			gpSetCls->OnInitDialog_Environment(hWnd2, bInitial);
			break;
		case IDD_SPG_MARKCOPY:
			gpSetCls->OnInitDialog_MarkCopy(hWnd2, bInitial);
			break;
		case IDD_SPG_PASTE:
			gpSetCls->OnInitDialog_Paste(hWnd2, bInitial);
			break;
		case IDD_SPG_HIGHLIGHT:
			gpSetCls->OnInitDialog_Hilight(hWnd2, bInitial);
			break;
		case IDD_SPG_FEATURE_FAR:
			gpSetCls->OnInitDialog_Far(hWnd2, bInitial);
			break;
		case IDD_SPG_FARMACRO:
			gpSetCls->OnInitDialog_FarMacro(hWnd2, bInitial);
			break;
		case IDD_SPG_KEYS:
			{
			bool lbOld = bSkipSelChange; bSkipSelChange = true;
			gpSetCls->OnInitDialog_Keys(hWnd2, bInitial);
			bSkipSelChange = lbOld;
			}
			break;
		case IDD_SPG_CONTROL:
			gpSetCls->OnInitDialog_Control(hWnd2, bInitial);
			break;
		case IDD_SPG_TABS:
			gpSetCls->OnInitDialog_Tabs(hWnd2, bInitial);
			break;
		case IDD_SPG_STATUSBAR:
			gpSetCls->OnInitDialog_Status(hWnd2, bInitial);
			break;
		case IDD_SPG_COLORS:
			gpSetCls->OnInitDialog_Color(hWnd2, bInitial);
			break;
		case IDD_SPG_TRANSPARENT:
			if (bInitial)
				gpSetCls->OnInitDialog_Transparent(hWnd2);
			break;
		case IDD_SPG_CMDTASKS:
			{
			bool lbOld = bSkipSelChange; bSkipSelChange = true;
			gpSetCls->OnInitDialog_Tasks(hWnd2, bInitial);
			bSkipSelChange = lbOld;
			}
			break;
		case IDD_SPG_APPDISTINCT:
			gpSetCls->OnInitDialog_Apps(hWnd2, bInitial);
			break;
		case IDD_SPG_INTEGRATION:
			gpSetCls->OnInitDialog_Integr(hWnd2, bInitial);
			break;
		case IDD_SPG_DEFTERM:
			{
			bool lbOld = bSkipSelChange; bSkipSelChange = true;
			gpSetCls->OnInitDialog_DefTerm(hWnd2, bInitial);
			bSkipSelChange = lbOld;
			}
			break;
		case IDD_SPG_VIEWS:
			if (bInitial)
				gpSetCls->OnInitDialog_Views(hWnd2);
			break;
		case IDD_SPG_DEBUG:
			if (bInitial)
				gpSetCls->OnInitDialog_Debug(hWnd2);
			break;
		case IDD_SPG_UPDATE:
			gpSetCls->OnInitDialog_Update(hWnd2, bInitial);
			break;
		case IDD_SPG_INFO:
			if (bInitial)
				gpSetCls->OnInitDialog_Info(hWnd2);
			break;

		default:
			// Чтобы не забыть добавить вызов инициализации
			_ASSERTE(FALSE && "Unhandled PageID!");
		} // switch (((ConEmuSetupPages*)lParam)->PageID)

		if (bInitial)
		{
			gpSetCls->RegisterTipsFor(hWnd2);
		}
	}
	else if ((messg == WM_HELP) || (messg == HELP_WM_HELP))
	{
		_ASSERTE(messg == WM_HELP);
		return gpSetCls->wndOpProc(hWnd2, messg, wParam, lParam);
	}
	else if (pgId == thi_Apps)
	{
		// Страничка "App distinct" в некотором смысле особенная.
		// У многих контролов ИД дублируются с другими вкладками.
		return gpSetCls->pageOpProc_Apps(hWnd2, NULL, messg, wParam, lParam);
	}
	else if (pgId == thi_Integr)
	{
		return gpSetCls->pageOpProc_Integr(hWnd2, messg, wParam, lParam);
	}
	else if (pgId == thi_Startup)
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
				switch (HIWORD(wParam))
				{
				case BN_CLICKED:
					gpSetCls->OnButtonClicked(hWnd2, wParam, lParam);
					return 0;

				case EN_CHANGE:
					if (!bSkipSelChange)
						gpSetCls->OnEditChanged(hWnd2, wParam, lParam);
					return 0;

				case CBN_EDITCHANGE:
				case CBN_SELCHANGE/*LBN_SELCHANGE*/:
					if (!bSkipSelChange)
						gpSetCls->OnComboBox(hWnd2, wParam, lParam);
					return 0;

				case LBN_DBLCLK:
					gpSetCls->OnListBoxDblClk(hWnd2, wParam, lParam);
					return 0;

				case CBN_KILLFOCUS:
					if (gpSetCls->mn_LastChangingFontCtrlId && LOWORD(wParam) == gpSetCls->mn_LastChangingFontCtrlId)
					{
						_ASSERTE(pgId == thi_Main);
						PostMessage(hWnd2, gpSetCls->mn_MsgRecreateFont, gpSetCls->mn_LastChangingFontCtrlId, 0);
						gpSetCls->mn_LastChangingFontCtrlId = 0;
						return 0;
					}
					break;

				default:
					if (HIWORD(wParam) == 0xFFFF && LOWORD(wParam) == lbConEmuHotKeys)
					{
						gpSetCls->OnHotkeysNotify(hWnd2, wParam, 0);
					}
				} // switch (HIWORD(wParam))
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
				if ((pgId == thi_Backgr) && (HWND)lParam == GetDlgItem(hWnd2, slDarker))
				{
					int newV = SendDlgItemMessage(hWnd2, slDarker, TBM_GETPOS, 0, 0);

					if (newV != gpSet->bgImageDarker)
					{
						gpSetCls->SetBgImageDarker(newV, true);

						//gpSet->bgImageDarker = newV;
						//TCHAR tmp[10];
						//_wsprintf(tmp, SKIPLEN(countof(tmp)) L"%i", gpSet->bgImageDarker);
						//SetDlgItemText(hWnd2, tDarker, tmp);

						//// Картинку может установить и плагин
						//if (gpSet->isShowBgImage && gpSet->sBgImage[0])
						//	gpSetCls->LoadBackgroundFile(gpSet->sBgImage);
						//else
						//	gpSetCls->NeedBackgroundUpdate();

						//gpConEmu->Update(true);
					}
				}
				else if ((pgId == thi_Transparent) && (HWND)lParam == GetDlgItem(hWnd2, slTransparent))
				{
					int newV = SendDlgItemMessage(hWnd2, slTransparent, TBM_GETPOS, 0, 0);

					if (newV != gpSet->nTransparent)
					{
						checkDlgButton(hWnd2, cbTransparent, (newV!=MAX_ALPHA_VALUE) ? BST_CHECKED : BST_UNCHECKED);
						gpSet->nTransparent = newV;
						if (!gpSet->isTransparentSeparate)
							SendDlgItemMessage(hWnd2, slTransparentInactive, TBM_SETPOS, (WPARAM) true, (LPARAM) gpSet->nTransparent);
						gpConEmu->OnTransparent();
					}
				}
				else if ((pgId == thi_Transparent) && (HWND)lParam == GetDlgItem(hWnd2, slTransparentInactive))
				{
					int newV = SendDlgItemMessage(hWnd2, slTransparentInactive, TBM_GETPOS, 0, 0);

					if (gpSet->isTransparentSeparate && (newV != gpSet->nTransparentInactive))
					{
						//checkDlgButton(hWnd2, cbTransparentInactive, (newV!=MAX_ALPHA_VALUE) ? BST_CHECKED : BST_UNCHECKED);
						gpSet->nTransparentInactive = newV;
						gpConEmu->OnTransparent();
					}
				}
			} // WM_HSCROLL
			break;

		case WM_NOTIFY:
			{
				if (((NMHDR*)lParam)->code == TTN_GETDISPINFO)
				{
					return gpSetCls->ProcessTipHelp(hWnd2, messg, wParam, lParam);
				}
				else switch (((NMHDR*)lParam)->idFrom)
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

			if (wParam == BALLOON_MSG_TIMERID)
			{
				KillTimer(hWnd2, BALLOON_MSG_TIMERID);
				SendMessage(gpSetCls->hwndBalloon, TTM_TRACKACTIVATE, FALSE, (LPARAM)&gpSetCls->tiBalloon);
				SendMessage(gpSetCls->hwndTip, TTM_ACTIVATE, TRUE, 0);
			}
			break;

		default:
		{
			if (messg == gpSetCls->mn_MsgRecreateFont)
			{
				gpSetCls->RecreateFont(wParam);
			}
			else if (messg == gpSetCls->mn_MsgLoadFontFromMain)
			{
				if (pgId == thi_Views)
					gpSetCls->OnInitDialog_CopyFonts(hWnd2, tThumbsFontName, tTilesFontName, 0);
				else if (pgId == thi_Tabs)
					gpSetCls->OnInitDialog_CopyFonts(hWnd2, tTabFontFace, 0);
				else if (pgId == thi_Status)
					gpSetCls->OnInitDialog_CopyFonts(hWnd2, tStatusFontFace, 0);

			}
			else if (messg == gpSetCls->mn_MsgUpdateCounter)
			{
				gpSetCls->PostUpdateCounters(true);
			}
			else if (messg == DBGMSG_LOG_ID && pgId == thi_Debug)
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
		gpSetCls->RegisterTipsFor(hWnd2);
		return FALSE;

	case WM_NOTIFY:
		{
			LPNMHDR phdr = (LPNMHDR)lParam;

			if (phdr->code == TTN_GETDISPINFO)
			{
				return gpSetCls->ProcessTipHelp(hWnd2, messg, wParam, lParam);
			}

			break;
		}

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

	default:
		if (gpSetCls->mp_DpiDistinct2 && gpSetCls->mp_DpiDistinct2->ProcessDpiMessages(hWnd2, messg, wParam, lParam))
		{
			return TRUE;
		}
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
		DWORD nOverrideID;
		DWORD nCtrls[32];
	} DistinctControls[] = {
		{0, {rbAppDistinctElevatedOn, rbAppDistinctElevatedOff, rbAppDistinctElevatedIgnore, stAppDistinctName, tAppDistinctName}},
		{cbExtendFontsOverride, {cbExtendFonts, stExtendFontBoldIdx, lbExtendFontBoldIdx, stExtendFontItalicIdx, lbExtendFontItalicIdx, stExtendFontNormalIdx, lbExtendFontNormalIdx}},
		{cbCursorOverride, {
			rCursorV, rCursorH, rCursorB/**/, rCursorR/**/, cbCursorColor, cbCursorBlink, cbCursorIgnoreSize, tCursorFixedSize, stCursorFixedSize, tCursorMinSize, stCursorMinSize,
			cbInactiveCursor/**/, rInactiveCursorV/**/, rInactiveCursorH/**/, rInactiveCursorB/**/, rInactiveCursorR/**/, cbInactiveCursorColor/**/, cbInactiveCursorBlink/**/, cbInactiveCursorIgnoreSize/**/, tInactiveCursorFixedSize/**/, stInactiveCursorFixedSize, tInactiveCursorMinSize, stInactiveCursorMinSize,
		}},
		{cbColorsOverride, {lbColorsOverride}},
		{cbClipboardOverride, {
			gbCopyingOverride, cbCTSDetectLineEnd, cbCTSBashMargin, cbCTSTrimTrailing, stCTSEOL, lbCTSEOL,
			gbSelectingOverride, cbCTSShiftArrowStartSel,
			gbPastingOverride, cbClipShiftIns, cbClipCtrlV,
			gbPromptOverride, cbCTSClickPromptPosition, cbCTSDeleteLeftWord}},
		{cbBgImageOverride, {cbBgImage, tBgImage, bBgImage, lbBgPlacement}},
	};

	if (!hChild)
		hChild = GetDlgItem(hWnd2, IDD_SPG_APPDISTINCT2);

	if (!hChild)
	{
		if ((messg == WM_INITDIALOG) || (messg == mn_ActivateTabMsg))
		{
			LogString(L"Creating child dialog IDD_SPG_APPDISTINCT2");
			SafeDelete(mp_DlgDistinct2); // JIC

			mp_DlgDistinct2 = CDynDialog::ShowDialog(IDD_SPG_APPDISTINCT2, hWnd2, pageOpProc_AppsChild, 0/*dwInitParam*/);
			HWND hChild = mp_DlgDistinct2 ? mp_DlgDistinct2->mh_Dlg : NULL;

			if (!hChild)
			{
				EnableWindow(hWnd2, FALSE);
				MBoxAssert(hChild && "CreateDialogParam(IDD_SPG_APPDISTINCT2) failed");
				return 0;
			}
			SetWindowLongPtr(hChild, GWLP_ID, IDD_SPG_APPDISTINCT2);

			if (!mp_DpiDistinct2 && mp_DpiAware)
			{
				mp_DpiDistinct2 = new CDpiForDialog();
				mp_DpiDistinct2->Attach(hChild, hWnd2, mp_DlgDistinct2);
			}

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

			MoveWindowRect(hChild, rcPos);

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
			const ColorPalette* pPal;

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


			CSetDlgLists::FillListBox(hChild, lbExtendFontBoldIdx, CSetDlgLists::eColorIdx);
			CSetDlgLists::FillListBox(hChild, lbExtendFontItalicIdx, CSetDlgLists::eColorIdx);
			CSetDlgLists::FillListBox(hChild, lbExtendFontNormalIdx, CSetDlgLists::eColorIdx);
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
		const AppSettings* pApp = NULL;
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
	case WM_NOTIFY:
		{
			LPNMHDR phdr = (LPNMHDR)lParam;

			if (phdr->code == TTN_GETDISPINFO)
			{
				return gpSetCls->ProcessTipHelp(hWnd2, messg, wParam, lParam);
			}

			break;
		}
	case UM_FILL_CONTROLS:
		if ((((HWND)wParam) == hWnd2) && lParam) // arg check
		{
			const AppSettings* pApp = (const AppSettings*)lParam;

			checkRadioButton(hWnd2, rbAppDistinctElevatedOn, rbAppDistinctElevatedIgnore,
				(pApp->Elevated == 1) ? rbAppDistinctElevatedOn :
				(pApp->Elevated == 2) ? rbAppDistinctElevatedOff : rbAppDistinctElevatedIgnore);

			BYTE b;
			wchar_t temp[MAX_PATH];

			checkDlgButton(hChild, cbExtendFontsOverride, pApp->OverrideExtendFonts);
			checkDlgButton(hChild, cbExtendFonts, pApp->isExtendFonts);
			_wsprintf(temp, SKIPLEN(countof(temp))(pApp->nFontBoldColor<16) ? L"%2i" : L"None", pApp->nFontBoldColor);
			CSetDlgLists::SelectStringExact(hChild, lbExtendFontBoldIdx, temp);
			_wsprintf(temp, SKIPLEN(countof(temp))(pApp->nFontItalicColor<16) ? L"%2i" : L"None", pApp->nFontItalicColor);
			CSetDlgLists::SelectStringExact(hChild, lbExtendFontItalicIdx, temp);
			_wsprintf(temp, SKIPLEN(countof(temp))(pApp->nFontNormalColor<16) ? L"%2i" : L"None", pApp->nFontNormalColor);
			CSetDlgLists::SelectStringExact(hChild, lbExtendFontNormalIdx, temp);

			checkDlgButton(hChild, cbCursorOverride, pApp->OverrideCursor);
			InitCursorCtrls(hChild, pApp);

			checkDlgButton(hChild, cbColorsOverride, pApp->OverridePalette);
			CSetDlgLists::SelectStringExact(hChild, lbColorsOverride, pApp->szPaletteName);

			checkDlgButton(hChild, cbClipboardOverride, pApp->OverrideClipboard);
			//
			checkDlgButton(hChild, cbCTSDetectLineEnd, pApp->isCTSDetectLineEnd);
			checkDlgButton(hChild, cbCTSBashMargin, pApp->isCTSBashMargin);
			checkDlgButton(hChild, cbCTSTrimTrailing, pApp->isCTSTrimTrailing);
			b = pApp->isCTSEOL;
			CSetDlgLists::GetListBoxItem(hChild, lbCTSEOL, CSetDlgLists::eCRLF, b);
			//
			checkDlgButton(hChild, cbClipShiftIns, pApp->isPasteAllLines);
			checkDlgButton(hChild, cbClipCtrlV, pApp->isPasteFirstLine);
			//
			checkDlgButton(hChild, cbCTSClickPromptPosition, pApp->isCTSClickPromptPosition);
			//
			checkDlgButton(hChild, cbCTSDeleteLeftWord, pApp->isCTSDeleteLeftWord);


			checkDlgButton(hChild, cbBgImageOverride, pApp->OverrideBgImage);
			checkDlgButton(hChild, cbBgImage, BST(pApp->isShowBgImage));
			SetDlgItemText(hChild, tBgImage, pApp->sBgImage);
			b = pApp->nBgOperation;
			CSetDlgLists::GetListBoxItem(hChild, lbBgPlacement, CSetDlgLists::eBgOper, b);

		} // UM_FILL_CONTROLS
		break;
	case UM_DISTINCT_ENABLE:
		if (((HWND)wParam) == hWnd2) // arg check
		{
			_ASSERTE(hChild && IsWindow(hChild));

			WORD nID = (WORD)(lParam & 0xFFFF);
			bool bAllowed = false;

			const AppSettings* pApp = NULL;
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

				CSetDlgLists::EnableDlgItems(hDlg, DistinctControls[i].nCtrls, countof(DistinctControls[i].nCtrls), bEnabled);
			}

			InvalidateCtrl(hChild, FALSE);
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
				AppSettings* pApp = (iCur < 0) ? NULL : gpSet->GetAppSettingsPtr(iCur);
				_ASSERTE((iCur<0) || (pApp && pApp->AppNames));

				switch (CB)
				{
				case cbAppDistinctReload:
					{
						if (MsgBox(L"Warning! All unsaved changes will be lost!\n\nReload Apps from settings?",
								MB_YESNO|MB_ICONEXCLAMATION, gpConEmu->GetDefaultTitle(), ghOpWnd) != IDYES)
							break;

						// Перезагрузить App distinct
						gpSet->LoadAppsSettings(NULL, true);

						// Обновить список на экране
						OnInitDialog_Apps(hWnd2, true);
					} // cbAppDistinctReload
					break;
				case cbAppDistinctAdd:
					{
						int iCount = (int)SendDlgItemMessage(hWnd2, lbAppDistinct, LB_GETCOUNT, 0,0);
						AppSettings* pNew = gpSet->GetAppSettingsPtr(iCount, TRUE);
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

						const AppSettings* p = gpSet->GetAppSettingsPtr(iCur);
						if (!p)
							break;

						if (MsgBox(L"Delete application?", MB_YESNO|MB_ICONQUESTION, p->AppNames, ghOpWnd) != IDYES)
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
						// Обновить палитры
						gpSet->PaletteSetStdIndexes();
						// Обновить консоли (считаем, что меняется только TextAttr, Popup не трогать
						gpSetCls->UpdateTextColorSettings(TRUE, FALSE, pApp);
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
				case rCursorH:
				case rCursorV:
				case rCursorB:
				case rCursorR:
				case cbCursorColor:
				case cbCursorBlink:
				case cbCursorIgnoreSize:
				case cbInactiveCursor:
				case rInactiveCursorH:
				case rInactiveCursorV:
				case rInactiveCursorB:
				case rInactiveCursorR:
				case cbInactiveCursorColor:
				case cbInactiveCursorBlink:
				case cbInactiveCursorIgnoreSize:
					OnButtonClicked_Cursor(hChild, wParam, lParam, pApp);
					bRedraw = true;
					break;
				//case rCursorV:
				//case rCursorH:
				//case rCursorB:
				//	if (pApp)
				//	{
				//		pApp->isCursorType = (CB - rCursorH); // IsChecked(hChild, rCursorV);
				//		bRedraw = true;
				//	}
				//	break;
				//case cbCursorColor:
				//	if (pApp)
				//	{
				//		pApp->isCursorColor = IsChecked(hChild, CB);
				//		bRedraw = true;
				//	}
				//	break;
				//case cbCursorBlink:
				//	if (pApp)
				//	{
				//		pApp->isCursorBlink = IsChecked(hChild, CB);
				//		//if (!gpSet->AppStd.isCursorBlink) // если мигание отключается - то курсор может "замереть" в погашенном состоянии.
				//		//	gpConEmu->ActiveCon()->Invalidate();
				//		bRedraw = true;
				//	}
				//	break;
				//case cbBlockInactiveCursor:
				//	if (pApp)
				//	{
				//		pApp->isCursorBlockInactive = IsChecked(hChild, CB);
				//		bRedraw = true;
				//	}
				//	break;
				//case cbCursorIgnoreSize:
				//	if (pApp)
				//	{
				//		pApp->isCursorIgnoreSize = IsChecked(hChild, CB);
				//		bRedraw = true;
				//	}
				//	break;

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
				case cbCTSDeleteLeftWord:
					if (pApp)
					{
						pApp->isCTSDeleteLeftWord = IsChecked(hChild, CB);
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
				AppSettings* pApp = (iCur < 0) ? NULL : gpSet->GetAppSettingsPtr(iCur);
				_ASSERTE((iCur<0) || (pApp && pApp->AppNames));

				if (pApp)
				{
					switch (ID)
					{
					case tAppDistinctName:
						if (!bSkipEditChange)
						{
							AppSettings* pApp = gpSet->GetAppSettingsPtr(iCur);
							if (!pApp || !pApp->AppNames)
							{
								_ASSERTE(pApp && pApp->AppNames);
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
					case tInactiveCursorFixedSize:
					case tCursorMinSize:
					case tInactiveCursorMinSize:
						if (pApp)
						{
							bRedraw = OnEditChanged_Cursor(hChild, wParam, lParam, pApp);
						} //case tCursorFixedSize, tInactiveCursorFixedSize, tCursorMinSize, tInactiveCursorMinSize:
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

					const AppSettings* pApp = NULL;
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
					AppSettings* pApp = (iCur < 0) ? NULL : gpSet->GetAppSettingsPtr(iCur);
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
											pApp->SetPaletteName((nIdx == 0) ? L"" : pszText);

											_ASSERTE(iCur>=0 && iCur<gpSet->AppCount /*&& gpSet->AppColors*/);
											const ColorPalette* pPal = gpSet->PaletteGet(iPal);
											if (pPal)
											{
												//memmove(gpSet->AppColors[iCur]->Colors, pPal->Colors, sizeof(pPal->Colors));
												//gpSet->AppColors[iCur]->FadeInitialized = false;

												BOOL bTextAttr = (pApp->nTextColorIdx != pPal->nTextColorIdx) || (pApp->nBackColorIdx != pPal->nBackColorIdx);
												pApp->nTextColorIdx = pPal->nTextColorIdx;
												pApp->nBackColorIdx = pPal->nBackColorIdx;
												BOOL bPopupAttr = (pApp->nPopTextColorIdx != pPal->nPopTextColorIdx) || (pApp->nPopBackColorIdx != pPal->nPopBackColorIdx);
												pApp->nPopTextColorIdx = pPal->nPopTextColorIdx;
												pApp->nPopBackColorIdx = pPal->nPopBackColorIdx;
												pApp->isExtendColors = pPal->isExtendColors;
												pApp->nExtendColorIdx = pPal->nExtendColorIdx;
												if (bTextAttr || bPopupAttr)
												{
													gpSetCls->UpdateTextColorSettings(bTextAttr, bPopupAttr, pApp);
												}
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
								CSetDlgLists::GetListBoxItem(hChild, lbCTSEOL, CSetDlgLists::eCRLF, eol);
								pApp->isCTSEOL = eol;
							} // lbCTSEOL
							break;

						case lbBgPlacement:
							{
								BYTE bg = 0;
								CSetDlgLists::GetListBoxItem(hChild, lbBgPlacement, CSetDlgLists::eBgOper, bg);
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
	{
		// Tell all RCon-s that application settings were changed
		struct impl
		{
			static bool ResetAppId(CVirtualConsole* pVCon, LPARAM lParam)
			{
				pVCon->RCon()->ResetActiveAppSettingsId();
				return true;
			};
		};
		CVConGroup::EnumVCon(evf_All, impl::ResetAppId, 0);

		// And update VCon-s. We need to update consoles if only visible settings were changes...
		gpConEmu->Update(true);
	}

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

LRESULT CSettings::MarkCopyPreviewProc(HWND hCtrl, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRc = 0;
	PAINTSTRUCT ps = {};
	HBRUSH hbr;
	HDC hdc;

	uint idxCon = gpSet->AppStd.nBackColorIdx;
	if (idxCon > 15)
		idxCon = 0;
	uint idxBack = (gpSet->isCTSColorIndex & 0xF0) >> 4;
	uint idxFore = (gpSet->isCTSColorIndex & 0xF);
	RECT rcClient = {};

	switch (uMsg)
	{
	case WM_ERASEBKGND:
	case WM_PAINT:
		hbr = CreateSolidBrush(gpSet->Colors[idxCon]);
		GetClientRect(hCtrl, &rcClient);
		if (uMsg == WM_ERASEBKGND)
			hdc = (HDC)wParam;
		else
			hdc = BeginPaint(hCtrl, &ps);
		if (!hdc)
			goto wrap;
		FillRect(hdc, &rcClient, hbr);
		if (uMsg == WM_PAINT)
		{
			HFONT hOld = NULL;
			HFONT hNew = NULL;
			if (gpSetCls->mh_Font[0].iType == CEFONT_GDI)
				hNew = gpSetCls->mh_Font[0].hFont;
			else
				hNew = (HFONT)SendMessage(GetParent(hCtrl), WM_GETFONT, 0, 0);
			if (hNew)
				hOld = (HFONT)SelectObject(hdc, hNew);
			SetTextColor(hdc, gpSet->Colors[idxFore]);
			SetBkColor(hdc, gpSet->Colors[idxBack]);
			wchar_t szText[] = L" Selected text ";
			SIZE sz = {};
			GetTextExtentPoint32(hdc, szText, lstrlen(szText), &sz);
			RECT rcText = {sz.cx, sz.cy};
			OffsetRect(&rcText, max(0,(rcClient.right-rcClient.left-sz.cx)/2), max(0,(rcClient.bottom-rcClient.top-sz.cy)/2));
			DrawText(hdc, szText, -1, &rcText, DT_VCENTER|DT_CENTER|DT_SINGLELINE);
			if (hOld)
				SelectObject(hdc, hOld);
		}
		DeleteObject(hbr);
		if (uMsg == WM_PAINT)
			EndPaint(hCtrl, &ps);
		goto wrap;
	}

	if (gpSetCls->mf_MarkCopyPreviewProc)
		lRc = ::CallWindowProc(gpSetCls->mf_MarkCopyPreviewProc, hCtrl, uMsg, wParam, lParam);
	else
		lRc = ::DefWindowProc(hCtrl, uMsg, wParam, lParam);
wrap:
	return lRc;

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

void CSettings::UpdatePosSizeEnabled(HWND hWnd2)
{
	EnableWindow(GetDlgItem(hWnd2, tWndX), !gpConEmu->mp_Inside && !(gpSet->isQuakeStyle && gpSet->wndCascade));
	EnableWindow(GetDlgItem(hWnd2, tWndY), !(gpSet->isQuakeStyle || gpConEmu->mp_Inside));
	EnableWindow(GetDlgItem(hWnd2, tWndWidth), !gpConEmu->mp_Inside);
	EnableWindow(GetDlgItem(hWnd2, tWndHeight), !gpConEmu->mp_Inside);
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
		mb_IgnoreEditChanged = TRUE;
		SetDlgItemInt(hSizePosPg, tWndX, gpSet->isUseCurrentSizePos ? gpConEmu->wndX : gpSet->_wndX, TRUE);
		SetDlgItemInt(hSizePosPg, tWndY, gpSet->isUseCurrentSizePos ? gpConEmu->wndY : gpSet->_wndY, TRUE);
		mb_IgnoreEditChanged = FALSE;
	}

	wchar_t szLabel[128];
	_wsprintf(szLabel, SKIPLEN(countof(szLabel)) L"UpdatePos A={%i,%i} C={%i,%i} S={%i,%i}", ax,ay, gpConEmu->wndX, gpConEmu->wndY, gpSet->_wndX, gpSet->_wndY);
	gpConEmu->LogWindowPos(szLabel);
}

void CSettings::UpdateSize(const CESize w, const CESize h)
{
	bool bUserCurSize = gpSet->isUseCurrentSizePos;
	//Issue ???: Сохранять размер Quake?
	bool bIgnoreWidth = (gpSet->isQuakeStyle != 0) && (gpSet->_WindowMode != wmNormal);

	if (w.IsValid(true) && h.IsValid(false))
	{
		if ((w.Raw != gpConEmu->WndWidth.Raw) || (h.Raw != gpConEmu->WndHeight.Raw))
		{
			gpConEmu->WndWidth.Set(true, w.Style, w.Value);
			gpConEmu->WndHeight.Set(false, h.Style, h.Value);
		}

		if (bUserCurSize && ((w.Raw != gpSet->wndWidth.Raw) || (h.Raw != gpSet->wndHeight.Raw)))
		{
			if (!bIgnoreWidth)
				gpSet->wndWidth.Set(true, w.Style, w.Value);
			gpSet->wndHeight.Set(false, h.Style, h.Value);
		}
	}

	HWND hSizePosPg = GetPage(thi_SizePos);
	if (hSizePosPg)
	{
		mb_IgnoreEditChanged = TRUE;
		SetDlgItemText(hSizePosPg, tWndWidth, bUserCurSize ? gpConEmu->WndWidth.AsString() : gpSet->wndWidth.AsString());
		SetDlgItemText(hSizePosPg, tWndHeight, bUserCurSize ? gpConEmu->WndHeight.AsString() : gpSet->wndHeight.AsString());
		mb_IgnoreEditChanged = FALSE;

		// Во избежание недоразумений - запретим элементы размера для Max/Fullscreen
		BOOL bNormalChecked = IsChecked(hSizePosPg, rNormal);
		//for (size_t i = 0; i < countof(SettingsNS::nSizeCtrlId); i++)
		//{
		//	EnableWindow(GetDlgItem(hSizePosPg, SettingsNS::nSizeCtrlId[i]), bNormalChecked);
		//}
		CSetDlgLists::EnableDlgItems(hSizePosPg, CSetDlgLists::eSizeCtrlId, bNormalChecked);
	}

	wchar_t szLabel[128];
	CESize ws = {w.Raw};
	CESize hs = {h.Raw};
	_wsprintf(szLabel, SKIPLEN(countof(szLabel)) L"UpdateSize A={%s,%s} C={%s,%s} S={%s,%s}", ws.AsString(), hs.AsString(), gpConEmu->WndWidth.AsString(), gpConEmu->WndHeight.AsString(), gpSet->wndWidth.AsString(), gpSet->wndHeight.AsString());
	gpConEmu->LogWindowPos(szLabel);
}

void CSettings::UpdateFontInfo()
{
	HWND hInfoPg = GetPage(thi_Info);
	if (!hInfoPg) return;

	wchar_t szTemp[32];
	_wsprintf(szTemp, SKIPLEN(countof(szTemp)) L"%ix%ix%i", LogFont.lfHeight, LogFont.lfWidth, m_tm->tmAveCharWidth);
	SetDlgItemText(hInfoPg, tRealFontMain, szTemp);
	_wsprintf(szTemp, SKIPLEN(countof(szTemp)) L"%ix%i", LogFont2.lfHeight, LogFont2.lfWidth);
	SetDlgItemText(hInfoPg, tRealFontBorders, szTemp);
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
			tiBalloon.hwnd = GetPage(thi_Main);
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

		SaveFontSizes((mn_AutoFontWidth == -1), true);

		gpConEmu->Update(true);

		if (gpConEmu->WindowMode == wmNormal)
			CVConGroup::SyncWindowToConsole(); // -- функция пустая, игнорируется
		else
			CVConGroup::SyncConsoleToWindow();

		gpConEmu->ReSize();
	}

	if (ghOpWnd)
	{
		wchar_t szSize[10];
		_wsprintf(szSize, SKIPLEN(countof(szSize)) L"%i", gpSet->FontSizeY);
		SetDlgItemText(GetPage(thi_Main), tFontSizeY, szSize);
		UpdateFontInfo();
		ShowFontErrorTip(gpSetCls->szFontError);
	}

	gpConEmu->OnPanelViewSettingsChanged(TRUE);
}

// Вызывается из диалога настроек
void CSettings::RecreateFont(WORD wFromID)
{
	LOGFONT LF = {0};

	HWND hMainPg = GetPage(thi_Main);

	if ((wFromID == (WORD)-1) || (ghOpWnd == NULL))
	{
		LF = LogFont;
	}
	else
	{
		LF.lfOutPrecision = OUT_TT_PRECIS;
		LF.lfClipPrecision = CLIP_DEFAULT_PRECIS;
		LF.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
		GetDlgItemText(hMainPg, tFontFace, LF.lfFaceName, countof(LF.lfFaceName));
		gpSet->FontSizeY = GetNumber(hMainPg, tFontSizeY);
		gpSet->FontSizeX = GetNumber(hMainPg, tFontSizeX);
		gpSet->FontUseDpi = IsChecked(hMainPg, cbFontMonitorDpi);
		gpSet->FontUseUnits = IsChecked(hMainPg, cbFontAsDeviceUnits);
		EvalLogfontSizes(LF, gpSet->FontSizeY, gpSet->FontSizeX);
		LF.lfWeight = IsChecked(hMainPg, cbBold) ? FW_BOLD : FW_NORMAL;
		LF.lfItalic = IsChecked(hMainPg, cbItalic);
		LF.lfCharSet = gpSet->mn_LoadFontCharSet;

		if (gpSet->mb_CharSetWasSet)
		{
			UINT lfCharSet = DEFAULT_CHARSET;
			if (CSetDlgLists::GetListBoxItem(hMainPg, tFontCharset, CSetDlgLists::eCharSets, lfCharSet))
				LF.lfCharSet = LOBYTE(lfCharSet);
			else
				LF.lfCharSet = DEFAULT_CHARSET;
		}

		if (IsChecked(hMainPg, rNoneAA))
			LF.lfQuality = NONANTIALIASED_QUALITY;
		else if (IsChecked(hMainPg, rStandardAA))
			LF.lfQuality = ANTIALIASED_QUALITY;
		else if (IsChecked(hMainPg, rCTAA))
			LF.lfQuality = CLEARTYPE_NATURAL_QUALITY;

		GetDlgItemText(hMainPg, tFontFace2, LogFont2.lfFaceName, countof(LogFont2.lfFaceName));
		gpSet->FontSizeX2 = GetNumber(hMainPg, tFontSizeX2, FontDefWidthMin, FontDefWidthMax);
		gpSet->FontSizeX3 = GetNumber(hMainPg, tFontSizeX3, FontDefWidthMin, FontDefWidthMax);

		if (isAdvLogging)
		{
			char szInfo[128]; _wsprintfA(szInfo, SKIPLEN(countof(szInfo)) "AutoRecreateFont(H=%i, W=%i)", LF.lfHeight, LF.lfWidth);
			CVConGroup::LogString(szInfo);
		}
	}

	_ASSERTE(LF.lfWidth >= 0 && LF.lfHeight != 0);

	CEFONT hf = CreateFontIndirectMy(&LF);

	_ASSERTE(LF.lfWidth >= 0 && LF.lfHeight > 0);

	if (hf.IsSet())
	{
		// SaveFontSizes выполним после обновления LogFont, т.к. там зовется gpConEmu->OnPanelViewSettingsChanged
		CEFONT hOldF = mh_Font[0];

		LogFont = LF;

		mh_Font[0] = hf;
		if (hOldF != hf)
		{
			hOldF.Delete();
		}

		SaveFontSizes((mn_AutoFontWidth == -1), true);

		if (wFromID != (WORD)-1)
		{
			if (wFromID == cbFontAsDeviceUnits || wFromID == cbFontMonitorDpi)
				gpConEmu->RecreateControls(true, true, false);

			gpConEmu->Update(true);

			if (gpConEmu->WindowMode == wmNormal)
				CVConGroup::SyncWindowToConsole(); // -- функция пустая, игнорируется
			else
				CVConGroup::SyncConsoleToWindow();

			gpConEmu->ReSize();
		}
	}

	if (ghOpWnd && wFromID == tFontFace)
	{
		wchar_t szSize[10];
		_wsprintf(szSize, SKIPLEN(countof(szSize)) L"%i", gpSet->FontSizeY);
		SetDlgItemText(hMainPg, tFontSizeY, szSize);
	}

	if (ghOpWnd)
	{
		UpdateFontInfo();

		ShowFontErrorTip(gpSetCls->szFontError);
	}

	if (gpConEmu->mn_StartupFinished >= CConEmuMain::ss_Started)
	{
		gpConEmu->OnPanelViewSettingsChanged(TRUE);
	}
}

void CSettings::ShowFontErrorTip(LPCTSTR asInfo)
{
	ShowErrorTip(asInfo, gpSetCls->GetPage(thi_Main), tFontFace, gpSetCls->szFontError, countof(gpSetCls->szFontError),
	             gpSetCls->hwndBalloon, &gpSetCls->tiBalloon, gpSetCls->hwndTip, FAILED_FONT_TIMEOUT);
}

void CSettings::SaveFontSizes(bool bAuto, bool bSendChanges)
{
	// Even if font was created with FontUseUnits option (negative lfHeight)
	// CreateFontIndirectMy MUST return in the lfHeight & lfWidth ACTUAL
	// bounding rectangle, so we can just store them
	_ASSERTE(LogFont.lfWidth > 0 && LogFont.lfHeight);

	mn_FontWidth = LogFont.lfWidth;
	mn_FontHeight = LogFont.lfHeight;

	wchar_t szLog[120];
	_wsprintf(szLog, SKIPLEN(countof(szLog))
		L"Main font was created Face='%s' lfHeight=%i lfWidth=%i use-dpi=%u dpi=%i zoom=%i",
		LogFont.lfFaceName, LogFont.lfHeight, LogFont.lfWidth,
		(UINT)gpSet->FontUseDpi, _dpi.Ydpi, mn_FontZoomValue);
	LogString(szLog);

	if (bAuto)
	{
		mn_AutoFontWidth = mn_FontWidth;
		mn_AutoFontHeight = mn_FontHeight;
	}

	// Применить в Mapping (там заодно и палитра копируется)
	gpConEmu->OnPanelViewSettingsChanged(bSendChanges);
}

bool CSettings::MacroFontSetSizeInt(LOGFONT& LF, int nRelative/*0/1/2/3*/, int nValue/*+-1,+-2,... | 100%*/)
{
	bool bChanged = false;
	int nCurHeight = EvalSize(gpSet->FontSizeY, esf_Vertical|esf_CanUseZoom);
	int nNeedHeight = nCurHeight;
	int nNewZoomValue = mn_FontZoomValue;

	// The current defaults
	LF = LogFont;

	// The LogFont member does not contains "Units" (negative values)
	// So we need to reevaluate "current" font descriptor (the height actually)
	LOGFONT lfCur = {};
	EvalLogfontSizes(lfCur, gpSet->FontSizeY, gpSet->FontSizeX);

	switch (nRelative)
	{
	case 0:
		// Absolute
		if (nValue < 5)
		{
			_ASSERTE(nValue >= 5);
			gpConEmu->LogString(L"-- Skipped! Absolute value less than 5");
			return false;
		}
		// Set the new value
		nNeedHeight = nValue;
		break;

	case 1:
		// Relative
		if (nValue == 0)
		{
			_ASSERTE(nValue != 0);
			gpConEmu->LogString(L"-- Skipped! Relative value is zero");
			return false;
		}
		// Decrease/increate font height
		nNeedHeight += nValue;
		break;

	case 2:
	case 3:
		// Zoom value
		nNewZoomValue = (nRelative == 2) ? MulDiv(nValue, FontZoom100, 100) : nValue;
		if (nNewZoomValue < 10)
		{
			_ASSERTE(nNewZoomValue >= 10);
			gpConEmu->LogString(L"-- Skipped! Zoom value is too small");
			return false;
		}
		// Force font recreation, even if zoom value is the same
		bChanged = true;
		goto wrap;
	}

	if ((gpSet->FontSizeY <= 0) || (nNeedHeight <= 0))
	{
		_ASSERTE((gpSet->FontSizeY > 0) && (nNeedHeight >= 0));
		gpConEmu->LogString(L"-- Skipped! FontSizeY and nNeedHeight must be positive");
		return false;
	}

	// Eval new zoom value
	nNewZoomValue = MulDiv(nNeedHeight, FontZoom100, gpSet->FontSizeY);
	// If relative, let easy return to 100%
	if (nRelative == 1)
	{
		if (((mn_FontZoomValue > FontZoom100) && (nNewZoomValue < FontZoom100)) || ((mn_FontZoomValue < FontZoom100) && (nNewZoomValue > FontZoom100)))
		{
			nNewZoomValue = FontZoom100;
			bChanged = true;
		}
	}

wrap:
	// And set the Zoom value
	mn_FontZoomValue = nNewZoomValue;

	// Now we can set the font
	EvalLogfontSizes(LF, gpSet->FontSizeY, gpSet->FontSizeX);

	// Check the height
	if (!bChanged && (LF.lfHeight != lfCur.lfHeight))
		bChanged = true;
	#if _DEBUG
	if (!bChanged)
	{
		_ASSERTE(bChanged && "lfHeight must be changed");
		gpConEmu->LogString(L"-- Skipped! lfHeight was not changed");
	}
	#endif
	// Ready
	return bChanged;
}

// Вызов из GUI-макросов - увеличить/уменьшить шрифт, без изменения размера (в пикселях) окна
// Функция НЕ меняет высоту шрифта настройки и изменения не будут сохранены в xml/reg
// Здесь меняется только значение "зума"
bool CSettings::MacroFontSetSize(int nRelative/*0/1/2/3*/, int nValue/*+-1,+-2,... | 100%*/)
{
	wchar_t szLog[128];
	if (isAdvLogging)
	{
		_wsprintf(szLog, SKIPLEN(countof(szLog)) L"MacroFontSetSize(%i,%i)", nRelative, nValue);
		gpConEmu->LogString(szLog);
	}

	// Validation
	if (nRelative == 0)
	{
		// Absolute height
		if (nValue < 5)
		{
			gpConEmu->LogString(L"-- Skipped! Absolute value less than 5");
			return false;
		}
	}
	else if (nRelative == 1)
	{
		// Relative
		if (nValue == 0)
		{
			gpConEmu->LogString(L"-- Skipped! Relative value is zero");
			return false;
		}
	}
	else if ((nRelative == 2) || (nRelative == 3))
	{
		// Zoom value
		if (nValue < 1)
		{
			gpConEmu->LogString(L"-- Skipped! Zoom value is too small");
			return false;
		}
	}
	else
	{
		_ASSERTE(FALSE && "Invalid nRelative value");
		gpConEmu->LogString(L"-- Skipped! Unsupported nRelative value");
		return false;
	}

	int nNewHeight = LogFont.lfHeight; // Issue 1130
	bool bWasNotZoom100 = (mn_FontZoomValue != FontZoom100);
	LOGFONT LF = {};

	for (int nRetry = 0; nRetry < 10; nRetry++)
	{
		if (!MacroFontSetSizeInt(LF, nRelative/*0/1/2*/, nValue/*+-1,+-2,... | 100%*/))
		{
			break;
		}

		// Не должен стать менее 5 пунктов
		if (abs(LF.lfHeight) < 5)
		{
			gpConEmu->LogString(L"-- Failed! Created font height less than 5");
			return false;
		}

		CEFONT hf = CreateFontIndirectMy(&LF);

		// Успешно, только если:
		// шрифт изменился
		// или хотели поставить абсолютный размер
		// или был масштаб НЕ 100%, а стал 100% (гарантированный возврат к оригиналу)
		if (hf.IsSet()
			&& ((nRelative != 1)
				|| (LF.lfHeight != LogFont.lfHeight)
				|| (!bWasNotZoom100 && (mn_FontZoomValue == FontZoom100))))
		{
			// SaveFontSizes выполним после обновления LogFont, т.к. там зовется gpConEmu->OnPanelViewSettingsChanged
			CEFONT hOldF = mh_Font[0];

			LogFont = LF;

			mh_Font[0] = hf;
			hOldF.Delete();

			// Запомнить размер шрифта (AutoFontWidth/Height - может быть другим, он запоминается выше)
			SaveFontSizes(false, true);

			// Передернуть размер консоли
			gpConEmu->OnSize();
			// Передернуть флажки, что шрифт поменялся
			gpConEmu->Update(true);

			if (ghOpWnd)
			{
				gpSetCls->UpdateFontInfo();

				HWND hMainPg = GetPage(thi_Main);
				if (hMainPg)
				{
					wchar_t temp[16];
					_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", gpSet->FontSizeY);
					CSetDlgLists::SelectStringExact(hMainPg, tFontSizeY, temp);
					_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", gpSet->FontSizeX);
					CSetDlgLists::SelectStringExact(hMainPg, tFontSizeX, temp);
				}
			}

			if (gpConEmu->mp_Status)
				gpConEmu->mp_Status->UpdateStatusBar(true);

			_wsprintf(szLog, SKIPLEN(countof(szLog)) L"-- Succeeded! New font {'%s',%i,%i} was created", LF.lfFaceName, LF.lfHeight, LF.lfWidth, LF.lfHeight, LF.lfWidth);
			gpConEmu->LogString(szLog);

			return true;
		}

		hf.Delete();

		if (nRelative != 1)
		{
			_ASSERTE(FALSE && "Font creation failed?");
			gpConEmu->LogString(L"-- Failed? (nRelative==0)?");
			return false;
		}

		// Если пытаются изменить относительный размер, а шрифт не создался - попробовать следующий размер
		nValue += (nValue > 0) ? 1 : -1;
	}

	_wsprintf(szLog, SKIPLEN(countof(szLog)) L"-- Failed! New font {'%s',%i,%i} was not created", LF.lfFaceName, LF.lfHeight, LF.lfWidth, LF.lfHeight, LF.lfWidth);
	gpConEmu->LogString(szLog);

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
		SaveFontSizes(false, true);

		// Передернуть флажки, что шрифт поменялся
		gpConEmu->Update(true);
		return true;
	}

	return false;
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
	//for (std::vector<RegFont>::iterator iter = m_RegFonts.begin(); iter != m_RegFonts.end(); ++iter)
	for (INT_PTR j = 0; j < m_RegFonts.size(); ++j)
	{
		const RegFont* iter = &(m_RegFonts[j]);

		if (iter->pCustom && lstrcmp(lfFaceName, iter->szFontName)==0)
		{
			*ppCustom = iter->pCustom;
			*ppFont = (*ppCustom)->GetFont(iSize, bBold, bItalic, bUnderline);

			if (!*ppFont)
			{
				MBoxAssert(*ppFont != NULL);
			}

			return true; // [bdf] шрифт. ошибка определяется по (*ppFont==NULL)
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

		HWND hMainPg = GetPage(thi_Main);
		if (hMainPg)
			CSetDlgLists::SelectStringExact(hMainPg, tFontSizeX2, L"0");
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

	// Eval first to consider DPI and FontUseUnits options
	// Force the same height in pixels as main font
	EvalLogfontSizes(LogFont2, gpSet->FontSizeY, gpSet->FontSizeX2);

	// Font for pseudographics may differs a lot in height,
	// so, to avoid vertically-dashed frames...
	if (gpSet->CheckCharAltFont(ucBoxDblVert))
	{
		if ((LogFont.lfHeight > 0)
			|| ((LogFont2.lfHeight > 0) && (LogFont.lfHeight > LogFont2.lfHeight))
			)
		{
			LogFont2.lfHeight = LogFont.lfHeight;
		}
		else
		{
			_ASSERTE(LogFont.lfHeight > 0);
		}
	}

	// Font width was not defined?
	if (gpSet->FontSizeX2 <= 0)
	{
		// Use main font width
		LogFont2.lfWidth = inFont->lfWidth;
	}
	mn_BorderFontWidth = LogFont2.lfWidth;

	// To avoid dashed frames, alternative font was created with NONANTIALIASED_QUALITY
	// But now, due to user-defined ranges, the font may be used for arbitrary characters,
	// so it's better to give user control over this
	DWORD fdwQuality = (!gpSet->isAntiAlias2) ? NONANTIALIASED_QUALITY
		// so, if clear-type or other anti-aliasing is ON for main font...
		: (gpSet->mn_AntiAlias != NONANTIALIASED_QUALITY) ? gpSet->mn_AntiAlias
		// otherwise - use clear-type
		: CLEARTYPE_NATURAL_QUALITY;
	// No control over alternative font charset, use default
	DWORD fdwCharSet = DEFAULT_CHARSET;
	mh_Font2 = CEFONT(CreateFont(LogFont2.lfHeight, LogFont2.lfWidth, 0, 0, FW_NORMAL,
	                             0, 0, 0, fdwCharSet, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
	                             fdwQuality, 0, LogFont2.lfFaceName));

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

				// Lucida may be not installed too
				// So, try to create Lucida or Courier (we need font with 'frames')
				bool bCreated = false;
				LPCWSTR szAltNames[] = {gsDefGuiFont, gsAltGuiFont};
				for (size_t a = 0; a < countof(szAltNames); a++)
				{
					if (!a && lstrcmpi(LogFont2.lfFaceName, gsDefGuiFont) == 0)
						continue; // It was already failed...

					wcscpy_c(LogFont2.lfFaceName, szAltNames[a]);
					SelectObject(hDC, hOldF);
					mh_Font2.Delete();

					mh_Font2 = CEFONT(CreateFont(LogFont2.lfHeight, LogFont2.lfWidth, 0, 0, FW_NORMAL,
													0, 0, 0, fdwCharSet, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
													fdwQuality, 0, LogFont2.lfFaceName));
					hOldF = (HFONT)SelectObject(hDC, mh_Font2.hFont);
					wchar_t szFontFace2[32];

					if (GetTextFace(hDC, countof(szFontFace2), szFontFace2))
					{
						szFontFace2[31] = 0;

						// Проверяем что создалось, и ругаемся, если что...
						if (lstrcmpi(LogFont2.lfFaceName, szFontFace2) == 0)
						{
							bCreated = true;
							wcscat_c(szFontError, szFontFace);
							wcscat_c(szFontError, L"\nUsing: ");
							wcscat_c(szFontError, LogFont2.lfFaceName);
							break;
						}
					}
				}
				// Font not installed or available?
				if (!bCreated)
				{
					wcscat_c(szFontError, szAltNames[0]);
				}
			}
		}

		GetTextMetrics(hDC, m_tm+MAX_FONT_STYLES);

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
// bool CSettings::MacroFontSetSize(int nRelative/*0/1/2*/, int nValue/*+-1,+-2,... | 100%*/)
// -- пересоздание шрифта по изменению контрола окна настроек
// void CSettings::RecreateFont(WORD wFromID)
// -- подгонка шрифта под размер окна GUI (если включен флажок "Auto")
// bool CSettings::AutoRecreateFont(int nFontW, int nFontH)
CEFONT CSettings::CreateFontIndirectMy(LOGFONT *inFont)
{
	//ResetFontWidth(); -- перенесено вниз, после того, как убедимся в валидности шрифта
	//lfOutPrecision = OUT_RASTER_PRECIS,
	szFontError[0] = 0;

	HWND hMainPg = GetPage(thi_Main);

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

			if (hMainPg)
				checkDlgButton(hMainPg, cbFontAuto, BST_UNCHECKED);

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

				if (hMainPg)
				{
					wchar_t temp[32];
					_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", nRastHeight);
					CSetDlgLists::SelectStringExact(hMainPg, tFontSizeY, temp);
					_wsprintf(temp, SKIPLEN(countof(temp)) L"%i", nRastWidth);
					CSetDlgLists::SelectStringExact(hMainPg, tFontSizeX, temp);
					CSetDlgLists::SelectStringExact(hMainPg, tFontSizeX3, temp);
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
		#ifdef _DEBUG
		DumpFontMetrics(L"mh_Font", hDC, hFont);
		#endif

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

		for (int i=0; i<MAX_FONT_STYLES; i++)
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
		bool bAlmostMonospace = IsAlmostMonospace(inFont->lfFaceName, m_tm, m_otm[0] /*m_tm->tmMaxCharWidth, m_tm->tmAveCharWidth, m_tm->tmHeight*/);
		//if (m_tm->tmMaxCharWidth && m_tm->tmAveCharWidth && m_tm->tmHeight)
		//{
		//	int nRelativeDelta = (m_tm->tmMaxCharWidth - m_tm->tmAveCharWidth) * 100 / m_tm->tmHeight;
		//	// Если расхождение менее 15% высоты - считаем шрифт моноширинным
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

		// Лучше поставим AveCharWidth. MaxCharWidth для "условно моноширинного" Consolas почти равен высоте.
		if (gpSet->FontSizeX3 && ((int)gpSet->FontSizeX3 > FontDefWidthMin) && ((int)gpSet->FontSizeX3 <= FontDefWidthMax))
			inFont->lfWidth = EvalCellWidth();
		else
			inFont->lfWidth = m_tm->tmAveCharWidth;
		// Обновлять реальный размер шрифта в диалоге настройки не будем, были случаи, когда
		// tmHeight был меньше, чем запрашивалось, однако, если пытаться создать шрифт с этим "обновленным"
		// размером - в реале создавался совсем другой шрифт...
		inFont->lfHeight = m_tm->tmHeight;

		if (lbTM && m_tm->tmCharSet != DEFAULT_CHARSET)
		{
			inFont->lfCharSet = m_tm->tmCharSet;

			const ListBoxItem* pCharSets = NULL;
			uint nCount = CSetDlgLists::GetListItems(CSetDlgLists::eCharSets, pCharSets);
			for (uint i = 0; i < nCount; i++)
			{
				if (pCharSets[i].nValue == m_tm->tmCharSet)
				{
					SendDlgItemMessage(hMainPg, tFontCharset, CB_SETCURSEL, i, 0);
					break;
				}
			}
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

void CSettings::EnableDlgItem(HWND hParent, WORD nCtrlId, BOOL bEnabled)
{
#ifdef _DEBUG
	if (!hParent)
	{
		_ASSERTE(hParent!=NULL);
	}
	else if (!GetDlgItem(hParent,nCtrlId))
	{
		//_ASSERTE(hDlgItem!=NULL && "Control not found in hParent dlg");
		wchar_t szErr[128]; _wsprintf(szErr, SKIPLEN(countof(szErr)) L"EnableDlgItem failed\nControlID %u not found in hParent dlg", nCtrlId);
		MsgBox(szErr, MB_SYSTEMMODAL|MB_ICONSTOP, L"ConEmu settings", ghOpWnd);
	}
#endif

	EnableWindow(GetDlgItem(hParent, nCtrlId), bEnabled);
}

int CSettings::GetNumber(HWND hParent, WORD nCtrlId, int nMin /*= 0*/, int nMax /*= 0*/)
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
			nValue = _wtoi((szNumber[0]==L' ') ? (szNumber+1) : szNumber);
		// Validation?
		if (nMin < nMax)
			nValue = min(nMax,max(nMin,nValue));
	}

	return nValue;
}

INT_PTR CSettings::GetString(HWND hParent, WORD nCtrlId, wchar_t** ppszStr, LPCWSTR asNoDefault /*= NULL*/, bool abListBox /*= false*/)
{
	INT_PTR nSel = abListBox ? SendDlgItemMessage(hParent, nCtrlId, CB_GETCURSEL, 0, 0) : -1;

	INT_PTR nLen = abListBox
		? ((nSel >= 0) ? SendDlgItemMessage(hParent, nCtrlId, CB_GETLBTEXTLEN, nSel, 0) : 0)
		: SendDlgItemMessage(hParent, nCtrlId, WM_GETTEXTLENGTH, 0, 0);

	if (!ppszStr)
		return nLen;

	if (nLen<=0)
	{
		SafeFree(*ppszStr);
		return nLen;
	}

	wchar_t* pszNew = (TCHAR*)calloc(nLen+1, sizeof(TCHAR));
	if (!pszNew)
	{
		_ASSERTE(pszNew!=NULL);
	}
	else
	{
		if (abListBox)
		{
			if (nSel >= 0)
				SendDlgItemMessage(hParent, nCtrlId, CB_GETLBTEXT, nSel, (LPARAM)pszNew);
		}
		else
		{
			GetDlgItemText(hParent, nCtrlId, pszNew, nLen+1);
		}


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
			SafeFree(*ppszStr);
			SafeFree(pszNew);
			nLen = 0;
			// Reset (it is default value!)
			return nLen;
		}

		if (nLen > (*ppszStr ? (INT_PTR)_tcslen(*ppszStr) : 0))
		{
			if (*ppszStr) free(*ppszStr);
			*ppszStr = pszNew; pszNew = NULL;
		}
		else if (*ppszStr)
		{
			_wcscpy_c(*ppszStr, nLen+1, pszNew);
		}
		SafeFree(pszNew);
	}

	return nLen;
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
	if (LogFont.lfWidth <= 0)
	{
		// Сюда мы должны попадать только для примерных расчетов во время старта!
		_ASSERTE(LogFont.lfWidth>0 || gpConEmu->mn_StartupFinished<=CConEmuMain::ss_Starting);
		int iEvalWidth = FontHeight() * 10 / 18;
		if (iEvalWidth)
			return _abs(iEvalWidth);
		return 8;
	}

	_ASSERTE(gpSetCls->mn_FontWidth==LogFont.lfWidth);
	_ASSERTE((int)gpSetCls->mn_FontWidth > 0);
	return gpSetCls->mn_FontWidth;
}

LONG CSettings::FontCellWidth()
{
	// В mn_FontWidth сохраняется ширина шрифта с учетом FontSizeX3, поэтому возвращаем
	return FontWidth();
}

LONG CSettings::FontHeight()
{
	if (LogFont.lfHeight <= 0)
	{
		// Сюда мы должны попадать только для примерных расчетов во время старта!
		_ASSERTE(LogFont.lfHeight>0 || gpConEmu->mn_StartupFinished<=CConEmuMain::ss_Starting);
		int iEvalHeight = 0;
		if (gpSet->FontSizeY)
		{
			iEvalHeight = EvalSize(gpSet->FontSizeY, esf_Vertical|esf_CanUseUnits|esf_CanUseDpi|esf_CanUseZoom);
			if (iEvalHeight < 0)
				iEvalHeight = -iEvalHeight * 17 / 14;
		}
		if (iEvalHeight)
			return _abs(iEvalHeight);
		return 12;
	}

	_ASSERTE(gpSetCls->mn_FontHeight==LogFont.lfHeight);
	return gpSetCls->mn_FontHeight;
}

// Возможно скорректированный размер шрифта для выгрузки фрагмента в HTML
LONG CSettings::FontHeightHtml()
{
	if (LogFont.lfHeight <= 0)
	{
		_ASSERTE(LogFont.lfHeight>0);
		return FontHeight();
	}

	int iHeight, iLineGap = 0;

	if (m_otm[0] && (m_otm[0]->otmrcFontBox.top > 0))
	{
		_ASSERTE(((m_otm[0]->otmrcFontBox.top * 1.3) >= LogFont.lfHeight) && (m_otm[0]->otmrcFontBox.top <= LogFont.lfHeight));
		iHeight = m_otm[0]->otmrcFontBox.top;
		if ((m_otm[0]->otmTextMetrics.tmInternalLeading < (iHeight/2)) && (m_otm[0]->otmTextMetrics.tmInternalLeading > 1))
			iLineGap = m_otm[0]->otmTextMetrics.tmInternalLeading - 1;
	}
	else
	{
		_ASSERTE(gpSetCls->mn_FontHeight==LogFont.lfHeight);
		iHeight = gpSetCls->mn_FontHeight;
		if ((m_tm[0].tmInternalLeading < (iHeight/2)) && (m_tm[0].tmInternalLeading > 1))
			iLineGap = m_tm[0].tmInternalLeading - 1;
	}

	return (iHeight - iLineGap);
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

bool CSettings::FontMonospaced()
{
	if (mh_Font[0].iType == CEFONT_CUSTOM)
	{
		// BDF fonts are always treated as monospaced
		return true;
	}
	return IsAlmostMonospace(LogFont.lfFaceName, m_tm, m_otm[0]);
}

LPCWSTR CSettings::BorderFontFaceName()
{
	return LogFont2.lfFaceName;
}

// Returns real pixels
LONG CSettings::BorderFontWidth()
{
	if (!gpSet->isFixFarBorders)
	{
		return FontCellWidth();
	}
	_ASSERTE(LogFont2.lfWidth);
	_ASSERTE(gpSetCls->mn_BorderFontWidth==LogFont2.lfWidth);
	return gpSetCls->mn_BorderFontWidth;
}

BYTE CSettings::BorderFontCharSet()
{
	return m_tm[MAX_FONT_STYLES].tmCharSet;
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

// asFontFile may come from: ConEmu /fontfile <path-to-font-file>
// or from RegisterFontsDir when enumerating font folder...
BOOL CSettings::RegisterFont(LPCWSTR asFontFile, BOOL abDefault)
{
	// Обработка параметра /fontfile
	_ASSERTE(asFontFile && *asFontFile);

	if (mb_StopRegisterFonts) return FALSE;

	//for (std::vector<RegFont>::iterator iter = m_RegFonts.begin(); iter != m_RegFonts.end(); ++iter)
	for (INT_PTR j = 0; j < m_RegFonts.size(); ++j)
	{
		RegFont* iter = &(m_RegFonts[j]);

		if (StrCmpI(iter->szFontFile, asFontFile) == 0)
		{
			// Уже добавлено
			if (abDefault && iter->bDefault == FALSE)
				iter->bDefault = TRUE;

			return TRUE;
		}
	}

	RegFont rf = {abDefault};
	wchar_t szFullFontName[LF_FACESIZE] = L"";

	if (!GetFontNameFromFile(asFontFile, rf.szFontName, szFullFontName))
	{
		//DWORD dwErr = GetLastError();
		size_t cchLen = _tcslen(asFontFile)+100;
		wchar_t* psz=(wchar_t*)calloc(cchLen,sizeof(wchar_t));
		_wcscpy_c(psz, cchLen, L"Can't retrieve font family from file:\n");
		_wcscat_c(psz, cchLen, asFontFile);
		_wcscat_c(psz, cchLen, L"\nContinue?");
		int nBtn = MsgBox(psz, MB_OKCANCEL|MB_ICONSTOP, gpConEmu->GetDefaultTitle());
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

	//for (std::vector<RegFont>::iterator iter = m_RegFonts.begin(); iter != m_RegFonts.end(); ++iter)
	for (INT_PTR j = 0; j < m_RegFonts.size(); ++j)
	{
		const RegFont* iter = &(m_RegFonts[j]);

		// Это может быть другой тип шрифта (Liberation Mono Bold, Liberation Mono Regular, ...)
		if (lstrcmpi(iter->szFontName, rf.szFontName) == 0
			|| lstrcmpi(iter->szFontName, szFullFontName) == 0)
		{
			lbRegistered = iter->bAlreadyInSystem;
			lbOneOfFam = TRUE;
			//iFamIndex = iter - m_RegFonts.begin();
			iFamIndex = j;
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
			int nBtn = MsgBox(psz, MB_OKCANCEL|MB_ICONSTOP, gpConEmu->GetDefaultTitle());
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
		int nBtn = MsgBox(psz, MB_OKCANCEL|MB_ICONSTOP, gpConEmu->GetDefaultTitle());
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
	if (!gpSet->isAutoRegisterFonts || gpConEmu->DisableRegisterFonts)
		return; // Если поиск шрифтов не требуется

	// Сначала - регистрация шрифтов в папке программы
	RegisterFontsDir(gpConEmu->ms_ConEmuExeDir);

	// Если папка запуска отличается от папки программы
	if (lstrcmpW(gpConEmu->ms_ConEmuExeDir, gpConEmu->ms_ConEmuBaseDir))
		RegisterFontsDir(gpConEmu->ms_ConEmuBaseDir); // зарегистрировать шрифты и из базовой папки

	// Если папка запуска отличается от папки программы
	if (lstrcmpiW(gpConEmu->ms_ConEmuExeDir, gpConEmu->WorkDir()))
	{
		BOOL lbSkipCurDir = FALSE;
		wchar_t szFontsDir[MAX_PATH+1];

		if (SHGetSpecialFolderPath(ghWnd, szFontsDir, CSIDL_FONTS, FALSE))
		{
			// Oops, папка запуска совпала с системной папкой шрифтов?
			lbSkipCurDir = (lstrcmpiW(szFontsDir, gpConEmu->WorkDir()) == 0);
		}

		if (!lbSkipCurDir)
		{
			// зарегистрировать шрифты и из папки запуска
			RegisterFontsDir(gpConEmu->WorkDir());
		}
	}

	// Теперь можно смотреть, зарегистрировались ли какие-то шрифты... И выбрать из них подходящие
	// Это делается в InitFont
}

void CSettings::RegisterFontsDir(LPCWSTR asFromDir)
{
	if (!asFromDir || !*asFromDir)
		return;

	// Регистрация шрифтов в папке ConEmu
	WIN32_FIND_DATA fnd;
	wchar_t szFind[MAX_PATH*2]; wcscpy_c(szFind, asFromDir);
	wchar_t* pszSlash = szFind + lstrlenW(szFind);
	_ASSERTE(pszSlash > szFind);
	if (*(pszSlash-1) == L'\\')
		pszSlash--;
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
					int nBtn = MsgBox(psz, MB_OKCANCEL|MB_ICONSTOP, gpConEmu->GetDefaultTitle());
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
	//for(std::vector<RegFont>::iterator iter = m_RegFonts.begin();
	//        iter != m_RegFonts.end(); iter = m_RegFonts.erase(iter))
	while (m_RegFonts.size() > 0)
	{
		INT_PTR j = m_RegFonts.size()-1;
		RegFont* iter = &(m_RegFonts[j]);

		if (iter->pCustom)
			delete iter->pCustom;
		else
			RemoveFontResourceEx(iter->szFontFile, FR_PRIVATE, NULL);

		m_RegFonts.erase(j);
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
	wchar_t szRetValA[MAX_PATH];
	wchar_t szRetValU[MAX_PATH];
	DWORD dwRead;
	TT_OFFSET_TABLE ttOffsetTable;
	TT_TABLE_DIRECTORY tblDir;
	BOOL bFound = FALSE;

	// Dump found table item
	LogString(lpszFilePath);
	bool bDumpTable = RELEASEDEBUGTEST((gpSetCls->isAdvLogging !=0),true);
	wchar_t szDumpInfo[200];

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
			if (lstrcmpni(tblDir.szTag, "name", 4) == 0) //-V112
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
								if (ttRecord.uStringLength <= (LF_FACESIZE * sizeof(wchar_t)))
								{
									//f.Read(csTemp.GetBuffer(ttRecord.uStringLength + 1), ttRecord.uStringLength);
									//csTemp.ReleaseBuffer();
									char szName[(LF_FACESIZE+3)*sizeof(wchar_t)] = "";

									if (ReadFile(f, szName, ttRecord.uStringLength + sizeof(wchar_t), &(dwRead=0), NULL) && dwRead)
									{
										// Ensure even wchar_t would be Z-terminated
										szName[ttRecord.uStringLength] = 0; szName[ttRecord.uStringLength+1] = 0; szName[ttRecord.uStringLength+2] = 0;
										LPCWSTR pszFound = NULL;

										// Dump found table item
										if (bDumpTable)
										{
											_wsprintf(szDumpInfo, SKIPCOUNT(szDumpInfo) L"  Platf: %u Enc: %u Lang: %u Len: %u \"", ttRecord.uPlatformID, ttRecord.uEncodingID, ttRecord.uLanguageID, ttRecord.uStringLength);
											int iLen = lstrlen(szDumpInfo);
											for (DWORD i = 0; i < dwRead; i++)
											{
												szDumpInfo[iLen++] = (szName[i]) ? (wchar_t)szName[i] : L'.';
											}
											szDumpInfo[iLen++] = L'\"';
											szDumpInfo[iLen] = 0;
											//gpConEmu->LogString(szDumpInfo); -- below
										}

										// Process read item
										if ((
											((ttRecord.uPlatformID == 768) && (ttRecord.uEncodingID == 256))
											|| ((ttRecord.uPlatformID == 0) && ((ttRecord.uEncodingID == 0) || (ttRecord.uEncodingID == 768)))
											) && ((wchar_t*)szName)[0])
										{
											// Seems like it's a 1201 │ UTF-16 (Big endian)
											int j = 0;
											while (j < ttRecord.uStringLength)
											{
												szRetValU[j] = SWAPWORD(((wchar_t*)szName)[j]);
												j++;
											}
											szRetValU[j] = 0;
											if (szRetValU[0])
											{
												lbRc = TRUE;
												pszFound = szRetValU;
											}
										}
										else if ((
											((ttRecord.uPlatformID == 256) && (ttRecord.uEncodingID == 0))
											) && szName[0])
										{
											for (int j = ttRecord.uStringLength; j >= 0 && szName[j] == ' '; j--)
												szName[j] = 0;
											szName[ttRecord.uStringLength] = 0;

											if (szName[0])
											{
												MultiByteToWideChar(CP_ACP, 0, szName, -1, szRetValA, LF_FACESIZE+1);
												pszFound = szRetValA;
												lbRc = TRUE;
											}

											//break; -- continue, may be Unicode name may be found
										}

										if (bDumpTable)
										{
											if (pszFound)
											{
												wcscat_c(szDumpInfo, L" >> \"");
												wcscat_c(szDumpInfo, pszFound);
												wcscat_c(szDumpInfo, L"\"");
											}
											else
											{
												wcscat_c(szDumpInfo, L" - UNKNOWN FORMAT");
											}
											gpConEmu->LogString(szDumpInfo);
										}
									}
								}
							}

							//f.Seek(nPos, CFile::begin);
							SetFilePointer(f, nPos, 0, FILE_BEGIN);
						}
					}
				} // for (int i = 0; i < ttNTHeader.uNRCount; i++)
			}
		}
	}

	if (lbRc)
	{
		wcscpy_c(rsFontName, *szRetValA ? szRetValA : szRetValU);
		wcscpy_c(rsFullFontName, *szRetValU ? szRetValU : szRetValA);
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

	if (lstrcmpni(root.szTag, "OTTO", 4) != 0) //-V112
		goto wrap; // Не поддерживается


	for (WORD i = 0; i < root.NumTables; i++)
	{
		//f.Read(&tblDir, sizeof(TT_TABLE_DIRECTORY));
		if (ReadFile(f, &tbl, sizeof(tbl), &(dwRead=0), NULL) && dwRead)
		{
			if (lstrcmpni(tbl.szTag, "name", 4) == 0) //-V112
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

void CSettings::ResetFontWidth()
{
	memset(m_CharWidth, 0, sizeof(m_CharWidth));
	memset(m_CharABC, 0, sizeof(m_CharABC));
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

void CSettings::SetHotkeyField(HWND hHk, BYTE vk)
{
	SendMessage(hHk, HKM_SETHOTKEY,
				vk|((vk==VK_DELETE||vk==VK_UP||vk==VK_DOWN||vk==VK_LEFT||vk==VK_RIGHT
				||vk==VK_PRIOR||vk==VK_NEXT||vk==VK_HOME||vk==VK_END
				||vk==VK_INSERT) ? (HOTKEYF_EXT<<8) : 0), 0);
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
			        && (gbIsDBCS || IsAlmostMonospace(pszFamilyName, &lpOutl->otmTextMetrics, lpOutl /*lpOutl->otmTextMetrics.tmMaxCharWidth, lpOutl->otmTextMetrics.tmAveCharWidth, lpOutl->otmTextMetrics.tmHeight*/))
			        && lpOutl->otmPanoseNumber.bProportion == PAN_PROP_MONOSPACED
			        && lstrcmpi(pszFamilyName, LF.lfFaceName) == 0
			  )
			{
				BOOL lbNonSystem = FALSE;

				// Нельзя использовать шрифты, которые зарегистрированы нами (для ConEmu). Они должны быть системными
				//for (std::vector<RegFont>::iterator iter = gpSetCls->m_RegFonts.begin(); !lbNonSystem && iter != gpSetCls->m_RegFonts.end(); ++iter)
				for (INT_PTR j = 0; j < m_RegFonts.size(); ++j)
				{
					const RegFont* iter = &(m_RegFonts[j]);

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
	if (isAdvLogging)
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

				if ((LONG)pnSizesSmall[i] >= gpSetCls->LogFont.lfHeight)
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
	for (INT_PTR j = 0; j < gpSetCls->m_RegFonts.size(); ++j)
	{
		const RegFont* iter = &(gpSetCls->m_RegFonts[j]);

		if (!iter->bAlreadyInSystem &&
		        lstrcmpi(iter->szFontName, lplf->lfFaceName) == 0)
		{
			return TRUE;
		}
	}

	// PAN_PROP_MONOSPACED - не дает правильного результата. Например 'MS Mincho' заявлен как моноширинный,
	// но совсем таковым не является. Кириллица у него дофига какая...
	// И только моноширинные!
	DWORD bAlmostMonospace = IsAlmostMonospace(lplf->lfFaceName, (LPTEXTMETRIC)lpntm /*lpntm->tmMaxCharWidth, lpntm->tmAveCharWidth, lpntm->tmHeight*/) ? 1 : 0;

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

HWND CSettings::CreatePage(ConEmuSetupPages* p)
{
	if (mp_DpiAware)
	{
		if (!p->pDpiAware)
			p->pDpiAware = new CDpiForDialog();
		p->DpiChanged = false;
	}
	wchar_t szLog[80]; _wsprintf(szLog, SKIPCOUNT(szLog) L"Creating child dialog ID=%u", p->PageID);
	LogString(szLog);
	SafeDelete(p->pDialog);

	p->pDialog = CDynDialog::ShowDialog(p->PageID, ghOpWnd, pageOpProc, (LPARAM)p);
	p->hPage = p->pDialog ? p->pDialog->mh_Dlg : NULL;

	return p->hPage;
}

void CSettings::ProcessDpiChange(ConEmuSetupPages* p)
{
	if (!p->hPage || !p->pDpiAware || !mp_DpiAware)
		return;

	HWND hPlace = GetDlgItem(ghOpWnd, tSetupPagePlace);
	RECT rcClient; GetWindowRect(hPlace, &rcClient);
	MapWindowPoints(NULL, ghOpWnd, (LPPOINT)&rcClient, 2);

	p->DpiChanged = false;
	p->pDpiAware->SetDialogDPI(mp_DpiAware->GetCurDpi(), &rcClient);

	if ((p->PageID == thi_Apps) && mp_DpiDistinct2)
	{
		HWND hHolder = GetDlgItem(p->hPage, tAppDistinctHolder);
		RECT rcPos = {}; GetWindowRect(hHolder, &rcPos);
		MapWindowPoints(NULL, p->hPage, (LPPOINT)&rcPos, 2);

		mp_DpiDistinct2->SetDialogDPI(mp_DpiAware->GetCurDpi(), &rcPos);
	}
}

HWND CSettings::GetActivePage()
{
	HWND hPage = NULL;

	if (ghOpWnd && m_Pages)
	{
		for (const ConEmuSetupPages* p = m_Pages; p->PageID; p++)
		{
			if (p->PageID == mn_LastActivedTab)
			{
				if (IsWindowVisible(p->hPage))
					hPage = p->hPage;
				break;
			}
		}
	}

	return hPage;
}

HWND CSettings::GetPage(CSettings::TabHwndIndex nPage)
{
	HWND hPage = NULL;

	if (ghOpWnd && m_Pages && (nPage >= thi_Main) && (nPage < thi_Last))
	{
		for (const ConEmuSetupPages* p = m_Pages; p->PageID; p++)
		{
			if (p->PageIndex == nPage)
			{
				hPage = p->hPage;
				break;
			}
		}
	}

	return hPage;
}

CSettings::TabHwndIndex CSettings::GetPageId(HWND hPage, ConEmuSetupPages** pp)
{
	TabHwndIndex pgId = thi_Last;

	if (ghOpWnd && m_Pages && hPage)
	{
		for (ConEmuSetupPages* p = m_Pages; p->PageID; p++)
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

CSettings::TabHwndIndex CSettings::GetPageId(HWND hPage)
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

	if (mp_DpiDistinct2)
		mp_DpiDistinct2->Detach();
	SafeDelete(mp_DlgDistinct2);

	for (ConEmuSetupPages *p = m_Pages; p->PageID; p++)
	{
		if (p->pDpiAware)
		{
			p->pDpiAware->Detach();
			SafeDelete(p->pDpiAware);
		}
		SafeDelete(p->pDialog);
		p->hPage = NULL;
		p->DpiChanged = false;
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
