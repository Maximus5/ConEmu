
/*
Copyright (c) 2009-2014 Maximus5
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

#ifdef _DEBUG
//  #define SHOW_GUIATTACH_START
	#define TEST_JP_LANS
#endif


#define SHOWDEBUGSTR

#define CHILD_DESK_MODE
#define REGPREPARE_EXTERNAL
//#define CATCH_TOPMOST_SET

#include "Header.h"
#include "AboutDlg.h"
#include <Tlhelp32.h>
#include <Shlobj.h>
#include "ShObjIdl_Part.h"
//#include <lm.h>
//#include "../common/ConEmuCheck.h"

#include "../common/execute.h"
#include "../common/MArray.h"
#include "../common/Monitors.h"
#include "../ConEmuCD/GuiHooks.h"
#include "../ConEmuCD/RegPrepare.h"
#include "Attach.h"
#include "ConEmu.h"
#include "ConEmuApp.h"
#include "ConEmuPipe.h"
#include "DefaultTerm.h"
#include "DragDrop.h"
#include "FindDlg.h"
#include "GestureEngine.h"
#include "Inside.h"
#include "LoadImg.h"
#include "Macro.h"
#include "Menu.h"
#include "options.h"
#include "RealBuffer.h"
#include "Recreate.h"
#include "RunQueue.h"
#include "Status.h"
#include "TabBar.h"
#include "TrayIcon.h"
#include "Update.h"
#include "VConChild.h"
#include "VConGroup.h"
#include "version.h"
#include "VirtualConsole.h"


#define DEBUGSTRSYS(s) //DEBUGSTR(s)
#define DEBUGSTRSIZE(s) //DEBUGSTR(s)
#define DEBUGSTRCONS(s) //DEBUGSTR(s)
#define DEBUGSTRTABS(s) //DEBUGSTR(s)
#define DEBUGSTRLANG(s) DEBUGSTR(s)// ; Sleep(2000)
#define DEBUGSTRMOUSE(s) //DEBUGSTR(s)
#define DEBUGSTRRCLICK(s) //DEBUGSTR(s)
#define DEBUGSTRKEY(s) //DEBUGSTR(s)
#define DEBUGSTRIME(s) //DEBUGSTR(s)
#define DEBUGSTRCHAR(s) //DEBUGSTR(s)
#define DEBUGSTRSETCURSOR(s) //OutputDebugString(s)
#define DEBUGSTRCONEVENT(s) //DEBUGSTR(s)
#define DEBUGSTRMACRO(s) //DEBUGSTR(s)
#define DEBUGSTRPANEL(s) //DEBUGSTR(s)
#define DEBUGSTRPANEL2(s) //DEBUGSTR(s)
#define DEBUGSTRFOREGROUND(s) //DEBUGSTR(s)
#define DEBUGSTRLLKB(s) //DEBUGSTR(s)
#define DEBUGSTRTIMER(s) //DEBUGSTR(s)
#define DEBUGSTRMSG(s) //DEBUGSTR(s)
#define DEBUGSTRMSG2(s) //DEBUGSTR(s)
#define DEBUGSTRANIMATE(s) //DEBUGSTR(s)
#define DEBUGSTRFOCUS(s) //DEBUGSTR(s)
#define DEBUGSTRSESS(s) DEBUGSTR(s)
#ifdef _DEBUG
//#define DEBUGSHOWFOCUS(s) DEBUGSTR(s)
#endif

//#define PROCESS_WAIT_START_TIME 1000

#define DRAG_DELTA 5
#define SPLITOVERCHECK_DELTA 5 // gpSet->isActivateSplitMouseOver()

#define PTDIFFTEST(C,D) PtDiffTest(C, ptCur.x, ptCur.y, D)
//(((abs(C.x-(short)LOWORD(lParam)))<D) && ((abs(C.y-(short)HIWORD(lParam)))<D))

#define _ASSERTEL(x) if (!(x)) { LogString(#x); _ASSERTE(x); }


#if defined(__GNUC__)
#define EXT_GNUC_LOG
#endif



#define HOTKEY_CTRLWINALTSPACE_ID 0x0201 // this is wParam for WM_HOTKEY
#define HOTKEY_SETFOCUSSWITCH_ID  0x0202 // this is wParam for WM_HOTKEY
#define HOTKEY_SETFOCUSGUI_ID     0x0203 // this is wParam for WM_HOTKEY
#define HOTKEY_SETFOCUSCHILD_ID   0x0204 // this is wParam for WM_HOTKEY
#define HOTKEY_CHILDSYSMENU_ID    0x0205 // this is wParam for WM_HOTKEY
#define HOTKEY_GLOBAL_START       0x1001 // this is wParam for WM_HOTKEY

#define RCLICKAPPSTIMEOUT 600
#define RCLICKAPPS_START 200 // начало отрисовки кружка вокруг курсора
#define RCLICKAPPSTIMEOUT_MAX 10000
#define RCLICKAPPSDELTA 3

#define TOUCH_DBLCLICK_DELTA 1000 // 1sec

#define FIXPOSAFTERSTYLE_DELTA 2500

//#define CONEMU_ANIMATE_DURATION 200
//#define CONEMU_ANIMATE_DURATION_MAX 5000

const wchar_t* gsHomePage    = CEHOMEPAGE;    //L"http://conemu-maximus5.googlecode.com";
const wchar_t* gsReportBug   = CEREPORTBUG;   //L"http://code.google.com/p/conemu-maximus5/issues/entry";
const wchar_t* gsReportCrash = CEREPORTCRASH; //L"http://code.google.com/p/conemu-maximus5/issues/entry";
const wchar_t* gsWhatsNew    = CEWHATSNEW;    //L"http://code.google.com/p/conemu-maximus5/wiki/Whats_New";

#define gsPortableApps_LauncherIni  L"\\..\\AppInfo\\Launcher\\ConEmuPortable.ini"
#define gsPortableApps_DefaultXml   L"\\..\\DefaultData\\settings\\ConEmu.xml"
#define gsPortableApps_ConEmuXmlDir L"\\..\\..\\Data\\settings"

static struct RegisteredHotKeys
{
	int DescrID;
	int RegisteredID; // wParam для WM_HOTKEY
	UINT VK, MOD;     // чтобы на изменение реагировать
	BOOL IsRegistered;
}
gRegisteredHotKeys[] = {
	{vkMinimizeRestore},
	{vkMinimizeRestor2},
	{vkGlobalRestore},
	{vkForceFullScreen},
};

namespace ConEmuMsgLogger
{
	MSG g_msg[BUFFER_SIZE];
	LONG g_msgidx = -1;

	Event g_pos[BUFFER_POS_SIZE];
	LONG g_posidx = -1;

	void LogPos(const MSG& msg, Source from)
	{
		#ifdef _DEBUG
		if (from != msgCanvas)
			return;
		#endif

		// Get next message index
		LONG i = _InterlockedIncrement(&g_posidx);
		// Write a message at this index
		Event& e = g_pos[i & (BUFFER_POS_SIZE - 1)]; // Wrap to buffer size

		e.hwnd = msg.hwnd;
		e.time = GetTickCount(); // msg.time == 0 скорее всего

		switch (msg.message)
		{
		case WM_WINDOWPOSCHANGING:
			e.msg = Event::WindowPosChanging;
			break;
		case WM_WINDOWPOSCHANGED:
			e.msg = Event::WindowPosChanged;
			break;
		case WM_MOVE:
			e.msg = Event::Move;
			break;
		case WM_SIZE:
			e.msg = Event::Size;
			break;
		case WM_MOVING:
			e.msg = Event::Moving;
			break;
		case WM_SIZING:
			e.msg = Event::Sizing;
			break;
		}

		switch (msg.message)
		{
		case WM_WINDOWPOSCHANGING:
		case WM_WINDOWPOSCHANGED:
			if (((WINDOWPOS*)msg.lParam)->flags & SWP_NOMOVE)
			{
				e.x = e.y = 0;
			}
			else
			{
				e.x = ((WINDOWPOS*)msg.lParam)->x;
				e.y = ((WINDOWPOS*)msg.lParam)->y;
			}
			if (((WINDOWPOS*)msg.lParam)->flags & SWP_NOSIZE)
			{
				e.w = e.h = 0;
			}
			else
			{
				e.w = ((WINDOWPOS*)msg.lParam)->cx;
				e.h = ((WINDOWPOS*)msg.lParam)->cy;
			}
			break;
		case WM_MOVE:
			e.x = (int)(short)LOWORD(msg.lParam);
			e.y = (int)(short)HIWORD(msg.lParam);
			e.w = e.h = 0;
			break;
		case WM_SIZE:
			e.x = e.y = 0;
			e.w = LOWORD(msg.lParam);
			e.h = HIWORD(msg.lParam);
			break;
		case WM_MOVING:
		case WM_SIZING:
			e.msg = Event::Sizing;
			e.x = ((LPRECT)msg.lParam)->left;
			e.y = ((LPRECT)msg.lParam)->top;
			e.w = ((LPRECT)msg.lParam)->right - e.x;
			e.h = ((LPRECT)msg.lParam)->bottom - e.y;
			break;
		}
	} // void LogPos(const MSG& msg)
}

const wchar_t gsFocusShellActivated[] = L"HSHELL_WINDOWACTIVATED";
const wchar_t gsFocusCheckTimer[] = L"TIMER_MAIN_ID";
const wchar_t gsFocusQuakeCheckTimer[] = L"TIMER_QUAKE_AUTOHIDE_ID";
#define QUAKE_FOCUS_CHECK_TIMER_DELAY 500

CConEmuMain::CConEmuMain()
{
	gpConEmu = this; // сразу!
	mb_FindBugMode = false;

	DEBUGTEST(mb_DestroySkippedInAssert=false);

	CVConGroup::Initialize();

	getForegroundWindow();

	_ASSERTE(gOSVer.dwMajorVersion>=5);
	//ZeroStruct(gOSVer);
	//gOSVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	//GetVersionEx(&gOSVer);

	mp_Log = NULL;
	InitializeCriticalSection(&mcs_Log);

	//HeapInitialize(); - уже
	//#define D(N) (1##N-100)

	_wsprintf(ms_ConEmuBuild, SKIPLEN(countof(ms_ConEmuBuild)) L"%02u%02u%02u%s", (MVV_1%100),MVV_2,MVV_3,RELEASEDEBUGTEST(_T(MVV_4a),_T("dbg")));
	wcscpy_c(ms_ConEmuDefTitle, L"ConEmu ");
	wcscat_c(ms_ConEmuDefTitle, ms_ConEmuBuild);
	wcscat_c(ms_ConEmuDefTitle, WIN3264TEST(L" [32]",L" [64]"));

	mp_Menu = new CConEmuMenu;
	mp_TabBar = NULL; /*m_Macro = NULL;*/ mp_Tip = NULL;
	mp_Status = new CStatus;
	mp_DefTrm = new CDefaultTerminal;
	mp_Inside = NULL;
	mp_Find = new CEFindDlg;
	mp_RunQueue = new CRunQueue();
	
	ms_ConEmuAliveEvent[0] = 0;	mb_AliveInitialized = FALSE;
	mh_ConEmuAliveEvent = NULL; mb_ConEmuAliveOwned = false; mn_ConEmuAliveEventErr = 0;
	mh_ConEmuAliveEventNoDir = NULL; mb_ConEmuAliveOwnedNoDir = false; mn_ConEmuAliveEventErrNoDir = 0;

	mn_MainThreadId = GetCurrentThreadId();
	//wcscpy_c(szConEmuVersion, L"?.?.?.?");
	
	bool bNeedConHostSearch = (gnOsVer == 0x0601);
	if (bNeedConHostSearch)
		InitializeCriticalSection(&m_LockConhostStart.cs);
	m_LockConhostStart.wait = false;

	mn_StartupFinished = ss_Starting;
	WindowMode = wmNormal;
	mb_InSetQuakeMode = false;
	changeFromWindowMode = wmNotChanging;
	//isRestoreFromMinimized = false;
	WindowStartMinimized = false;
	WindowStartTsa = false;
	WindowStartNoClose = false;
	ForceMinimizeToTray = false;
	mb_InCreateWindow = true;
	mb_InShowMinimized = false;
	mb_LastTransparentFocused = false;
	mh_MinFromMonitor = NULL;
	
	wndX = gpSet->_wndX; wndY = gpSet->_wndY;
	WndWidth = gpSet->wndWidth; WndHeight = gpSet->wndHeight;
	
	mn_QuakePercent = 0; // 0 - отключен
	DisableAutoUpdate = false;
	DisableKeybHooks = false;
	DisableAllMacro = false;
	DisableAllHotkeys = false;
	DisableSetDefTerm = false;
	DisableRegisterFonts = false;
	DisableCloseConfirm = false;
	//mn_SysMenuOpenTick = mn_SysMenuCloseTick = 0;
	//mb_PassSysCommand = false;
	mb_ExternalHidden = FALSE;
	ZeroStruct(mrc_StoredNormalRect);
	isWndNotFSMaximized = false;
	isQuakeMinimized = false;
	mb_StartDetached = FALSE;
	//mb_SkipSyncSize = false;
	isPiewUpdate = false; //true; --Maximus5
	gbPostUpdateWindowSize = false;
	hPictureView = NULL;  mrc_WndPosOnPicView = MakeRect(0,0);
	bPicViewSlideShow = false;
	dwLastSlideShowTick = 0;
	mh_ShellWindow = NULL; mn_ShellWindowPID = 0;
	mb_FocusOnDesktop = TRUE;
	cursor.x=0; cursor.y=0; Rcursor=cursor;
	mp_DragDrop = NULL;
	Title[0] = 0;
	TitleTemplate[0] = 0;
	mb_InTimer = FALSE;
	m_ProcCount = 0;
	mb_ProcessCreated = false; /*mn_StartTick = 0;*/ mb_WorkspaceErasedOnClose = false;
	mb_IgnoreSizeChange = false;
	mb_InCaptionChange = false;
	m_FixPosAfterStyle = 0;
	ZeroStruct(mrc_FixPosAfterStyle);
	mb_InImeComposition = false; mb_ImeMethodChanged = false;
	ZeroStruct(mr_Ideal);
	ZeroStruct(m_Pressed);
	mn_InResize = 0;
	mb_MouseCaptured = FALSE;
	mb_HotKeyRegistered = false;
	mh_LLKeyHookDll = NULL;
	mph_HookedGhostWnd = NULL;
	mh_LLKeyHook = NULL;
	mh_RightClickingBmp = NULL; mh_RightClickingDC = NULL; mb_RightClickingPaint = mb_RightClickingLSent = FALSE;
	m_RightClickingSize.x = m_RightClickingSize.y = m_RightClickingFrames = 0; m_RightClickingCurrent = -1;
	mh_RightClickingWnd = NULL; mb_RightClickingRegistered = FALSE;
	mb_WaitCursor = FALSE;
	mb_LastRgnWasNull = TRUE;
	mb_LockWindowRgn = FALSE;
	mb_LockShowWindow = FALSE;
	m_ForceShowFrame = fsf_Hide;
	mh_CursorWait = LoadCursor(NULL, IDC_WAIT);
	mh_CursorArrow = LoadCursor(NULL, IDC_ARROW);
	mh_CursorMove = LoadCursor(NULL, IDC_SIZEALL);
	mh_CursorIBeam = LoadCursor(NULL, IDC_IBEAM);
	mh_DragCursor = NULL;
	mh_CursorAppStarting = LoadCursor(NULL, IDC_APPSTARTING);
	// g_hInstance еще не инициализирован
	mh_SplitV = LoadCursor(GetModuleHandle(0), MAKEINTRESOURCE(IDC_SPLITV));
	mh_SplitH = LoadCursor(GetModuleHandle(0), MAKEINTRESOURCE(IDC_SPLITH));
	//ms_LogCreateProcess[0] = 0; mb_CreateProcessLogged = false;
	ZeroStruct(mouse);
	mouse.lastMMW=-1;
	mouse.lastMML=-1;
	GetCursorPos(&mouse.ptLastSplitOverCheck);
	ZeroStruct(session);
	ZeroStruct(m_TranslatedChars);
	//GetKeyboardState(m_KeybStates);
	//memset(m_KeybStates, 0, sizeof(m_KeybStates));
	m_ActiveKeybLayout = GetActiveKeyboardLayout();
	ZeroStruct(m_LayoutNames);
	//wchar_t szTranslatedChars[16];
	//HKL hkl = (HKL)GetActiveKeyboardLayout();
	//int nTranslatedChars = ToUnicodeEx(0, 0, m_KeybStates, szTranslatedChars, 15, 0, hkl);
	mn_LastPressedVK = 0;
	ZeroStruct(m_GuiInfo);
	m_GuiInfo.cbSize = sizeof(m_GuiInfo);
	//mh_RecreatePasswFont = NULL;
	mb_SkipOnFocus = false;
	mb_LastConEmuFocusState = false;
	mn_ForceTimerCheckLoseFocus = 0;
	mb_AllowAutoChildFocus = false;
	mb_IgnoreQuakeActivation = false;
	mn_LastQuakeShowHide = 0;
	mb_ScClosePending = false;
	mb_UpdateJumpListOnStartup = false;
	ZeroStruct(m_QuakePrevSize);
	m_TileMode = cwc_Current;
	ZeroStruct(m_JumpMonitor);

	mps_IconPath = NULL;

	#ifdef __GNUC__
	HMODULE hGdi32 = GetModuleHandle(L"gdi32.dll");
	GdiAlphaBlend = (AlphaBlend_t)(hGdi32 ? GetProcAddress(hGdi32, "GdiAlphaBlend") : NULL);
	#endif
	
	HMODULE hUser32 = GetModuleHandle(L"user32.dll");
	// GetLayeredWindowAttributes появился только в XP
	_GetLayeredWindowAttributes = (GetLayeredWindowAttributes_t)(hUser32 ? GetProcAddress(hUser32, "GetLayeredWindowAttributes") : NULL);
	#ifdef __GNUC__
	// SetLayeredWindowAttributes есть в Win2k, только про него не знает GCC
	SetLayeredWindowAttributes = (SetLayeredWindowAttributes_t)(hUser32 ? GetProcAddress(hUser32, "SetLayeredWindowAttributes") : NULL);
	#endif

	// IME support (WinXP or later)
	mh_Imm32 = NULL;
	_ImmSetCompositionFont = NULL;
	_ImmSetCompositionWindow = NULL;
	_ImmGetContext = NULL;
	if (gnOsVer >= 0x501)
	{
		mh_Imm32 = LoadLibrary(L"Imm32.dll");
		if (mh_Imm32)
		{
			_ImmSetCompositionFont = (ImmSetCompositionFontW_t)GetProcAddress(mh_Imm32, "ImmSetCompositionFontW");
			_ImmSetCompositionWindow = (ImmSetCompositionWindow_t)GetProcAddress(mh_Imm32, "ImmSetCompositionWindow");
			_ImmGetContext = (ImmGetContext_t)GetProcAddress(mh_Imm32, "ImmGetContext");
			if (!_ImmSetCompositionFont || !_ImmSetCompositionWindow || !_ImmGetContext)
			{
				_ASSERTE(_ImmSetCompositionFont && _ImmSetCompositionWindow && _ImmGetContext);
				_ImmSetCompositionFont = NULL;
				_ImmSetCompositionWindow = NULL;
				_ImmGetContext = NULL;
				FreeLibrary(mh_Imm32);
				mh_Imm32 = NULL;
			}
		}
	}


	// Попробуем "родное" из реестра?
	HKEY hk;
	ms_ComSpecInitial[0] = 0;
	if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment", 0, KEY_READ, &hk))
	{
		DWORD nSize = sizeof(ms_ComSpecInitial)-sizeof(wchar_t);
		if (RegQueryValueEx(hk, L"ComSpec", NULL, NULL, (LPBYTE)ms_ComSpecInitial, &nSize))
			ms_ComSpecInitial[0] = 0;
		RegCloseKey(hk);
	}
	if (!*ms_ComSpecInitial)
	{
		GetComspecFromEnvVar(ms_ComSpecInitial, countof(ms_ComSpecInitial));
	}
	_ASSERTE(*ms_ComSpecInitial);

	mpsz_ConEmuArgs = NULL;
	ms_ConEmuExe[0] = ms_ConEmuExeDir[0] = ms_ConEmuBaseDir[0] = ms_ConEmuWorkDir[0] = 0;
	//ms_ConEmuCExe[0] = 
	ms_ConEmuC32Full[0] = ms_ConEmuC64Full[0] = 0;
	ms_ConEmuXml[0] = ms_ConEmuIni[0] = ms_ConEmuChm[0] = 0;
	//ms_ConEmuCExeName[0] = 0;
	wchar_t *pszSlash = NULL;

	if (!GetModuleFileName(NULL, ms_ConEmuExe, MAX_PATH) || !(pszSlash = wcsrchr(ms_ConEmuExe, L'\\')))
	{
		DisplayLastError(L"GetModuleFileName failed");
		TerminateProcess(GetCurrentProcess(), 100);
		return;
	}

	// Запомнить текущую папку (на момент запуска)
	StoreWorkDir();

	//LoadVersionInfo(ms_ConEmuExe);
	// Папка программы
	wcscpy_c(ms_ConEmuExeDir, ms_ConEmuExe);
	pszSlash = wcsrchr(ms_ConEmuExeDir, L'\\');
	*pszSlash = 0;

	bool lbBaseFound = false;

	// Mingw/msys installation?
	m_InstallMode = cm_Normal; // проверка ниже
	wchar_t szFindFile[MAX_PATH+64];
	wcscpy_c(szFindFile, ms_ConEmuExeDir);
	pszSlash = wcsrchr(szFindFile, L'\\');
	if (pszSlash) *pszSlash = 0;
	pszSlash = szFindFile + _tcslen(szFindFile);
	wcscat_c(szFindFile, L"\\msys\\1.0\\bin\\sh.exe");
	if (FileExists(szFindFile))
	{
		m_InstallMode |= cm_MSysStartup;
	}
	else
	{
		// Git-bash mode
		*pszSlash = 0;
		wcscat_c(szFindFile, L"\\bin\\sh.exe");
		if (FileExists(szFindFile))
			m_InstallMode |= cm_MSysStartup;
	}

	// Сначала проверяем подпапку
	wcscpy_c(ms_ConEmuBaseDir, ms_ConEmuExeDir);
	wcscat_c(ms_ConEmuBaseDir, L"\\ConEmu");
	lbBaseFound = CheckBaseDir();


	if (!lbBaseFound)
	{
		// Если не нашли - то в той же папке, что и GUI
		wcscpy_c(ms_ConEmuBaseDir, ms_ConEmuExeDir);
		lbBaseFound = CheckBaseDir();
	}

	if (!lbBaseFound)
	{
		// Если все-равно не нашли - промерить, может это mingw?
		pszSlash = wcsrchr(ms_ConEmuExeDir, L'\\');
		if (pszSlash && (lstrcmpi(pszSlash, L"\\bin") == 0))
		{
			// -> $MINGW_ROOT/libexec/ConEmu
			size_t cch = pszSlash - ms_ConEmuExeDir;
			wmemmove(ms_ConEmuBaseDir, ms_ConEmuExeDir, cch);
			ms_ConEmuBaseDir[cch] = 0;
			wcscat_c(ms_ConEmuBaseDir, L"\\libexec\\ConEmu");
			// Last chance
			lbBaseFound = CheckBaseDir();
			if (lbBaseFound)
			{
				m_InstallMode |= cm_MinGW;
			}
			else
			{
				// Fail, revert to ExeDir
				wcscpy_c(ms_ConEmuBaseDir, ms_ConEmuExeDir);
			}
		}
	}

	if (isMingwMode() && isMSysStartup())
	{
		// This is example. Will be replaced with full path.
		gpSetCls->SetDefaultCmd(NULL /*L"sh.exe --login -i"*/);
	}

	if (!isMingwMode())
	{
		// PortableApps.com?
		LPCWSTR pszCheck[] = {
			gsPortableApps_LauncherIni /* L"\\..\\AppInfo\\Launcher\\ConEmuPortable.ini" */,
			gsPortableApps_DefaultXml /* L"\\..\\DefaultData\\settings\\ConEmu.xml"*/};
		for (size_t c = 0; c < countof(pszCheck); c++)
		{
			wcscpy_c(szFindFile, ms_ConEmuExeDir);
			wcscat_c(szFindFile, pszCheck[c]);
			if (!FileExists(szFindFile))
				goto NonPortable;
		}
		
		// Ищем где должен лежать файл настроек в структуре PortableApps
		wcscpy_c(ms_ConEmuXml, ms_ConEmuExeDir);
		wchar_t* pszCut = wcsrchr(ms_ConEmuXml, L'\\');
		if (pszCut)
		{
			*pszCut = 0;
			pszCut = wcsrchr(ms_ConEmuXml, L'\\');
			// gsPortableApps_ConEmuXml   L"\\..\\..\\Data\\settings"
			if (pszCut)
			{
				LPCWSTR pszAppendPath = gsPortableApps_ConEmuXmlDir;
				_ASSERTE(wcsncmp(pszAppendPath, L"\\..\\..\\D", 8)==0);
				*pszCut = 0;
				wcscat_c(ms_ConEmuXml, pszAppendPath+6);
				if (!DirectoryExists(ms_ConEmuXml))
					MyCreateDirectory(ms_ConEmuXml);
				wcscat_c(ms_ConEmuXml, L"\\ConEmu.xml");
				if (!FileExists(ms_ConEmuXml))
				{
					// Нужно скопировать дефолтный файл настроек?
					CopyFile(szFindFile, ms_ConEmuXml, FALSE);
				}
				// Turn it on
				m_InstallMode |= cm_PortableApps;
			}

			if (!(m_InstallMode & cm_PortableApps))
			{
				// Fail
				ms_ConEmuXml[0] = 0;
			}
		}


	}
	NonPortable:

	ZeroStruct(m_DbgInfo);
	if (IsDebuggerPresent())
	{
		wchar_t szDebuggers[32];
		if (GetEnvironmentVariable(ENV_CONEMU_BLOCKCHILDDEBUGGERS, szDebuggers, countof(szDebuggers)))
		{
			m_DbgInfo.bBlockChildrenDebuggers = (lstrcmp(szDebuggers, ENV_CONEMU_BLOCKCHILDDEBUGGERS_YES) == 0);
		}
	}
	// И сразу сбросить ее, чтобы не было мусора
	SetEnvironmentVariable(ENV_CONEMU_BLOCKCHILDDEBUGGERS, NULL);


	// Добавить в окружение переменную с папкой к ConEmu.exe
	SetEnvironmentVariable(ENV_CONEMUDIR_VAR_W, ms_ConEmuExeDir);
	SetEnvironmentVariable(ENV_CONEMUBASEDIR_VAR_W, ms_ConEmuBaseDir);
	SetEnvironmentVariable(ENV_CONEMUANSI_BUILD_W, ms_ConEmuBuild);
	SetEnvironmentVariable(ENV_CONEMUANSI_CONFIG_W, L"");
	// переменная "ConEmuArgs" заполняется в ConEmuApp.cpp:PrepareCommandLine

	wchar_t szDrive[MAX_PATH];
	SetEnvironmentVariable(ENV_CONEMUDRIVE_VAR_W, GetDrive(ms_ConEmuExeDir, szDrive, countof(szDrive)));
	

	// Ищем файл портабельных настроек. Сначала пробуем в BaseDir
	ConEmuXml();
	ConEmuIni();

	// Help-файл. Сначала попробуем в BaseDir
	wcscpy_c(ms_ConEmuChm, ms_ConEmuBaseDir); lstrcat(ms_ConEmuChm, L"\\ConEmu.chm");

	if (!FileExists(ms_ConEmuChm))
	{
		if (lstrcmp(ms_ConEmuBaseDir, ms_ConEmuExeDir))
		{
			wcscpy_c(ms_ConEmuChm, ms_ConEmuExeDir); lstrcat(ms_ConEmuChm, L"\\ConEmu.chm");

			if (!FileExists(ms_ConEmuChm))
				ms_ConEmuChm[0] = 0;
		}
		else
		{
			ms_ConEmuChm[0] = 0;
		}
	}

	wcscpy_c(ms_ConEmuC32Full, ms_ConEmuBaseDir);
	wcscat_c(ms_ConEmuC32Full, L"\\ConEmuC.exe");
	if (IsWindows64())
	{
		wcscpy_c(ms_ConEmuC64Full, ms_ConEmuBaseDir);
		wcscat_c(ms_ConEmuC64Full, L"\\ConEmuC64.exe");
		// Проверяем наличие
		if (!FileExists(ms_ConEmuC64Full))
			wcscpy_c(ms_ConEmuC64Full, ms_ConEmuC32Full);
		else if (!FileExists(ms_ConEmuC32Full))
			wcscpy_c(ms_ConEmuC32Full, ms_ConEmuC64Full);
	}
	else
	{
		wcscpy_c(ms_ConEmuC64Full, ms_ConEmuC32Full);
	}





	
	// DosBox (единственный способ запуска Dos-приложений в 64-битных OS)
	mb_DosBoxExists = CheckDosBoxExists();
	
	// Портабельный реестр для фара?
	mb_PortableRegExist = FALSE;
	ms_PortableReg[0] = ms_PortableRegHive[0] = ms_PortableRegHiveOrg[0] = ms_PortableMountKey[0] = 0;
	mb_PortableKeyMounted = FALSE;
	ms_PortableTempDir[0] = 0;
	mh_PortableMountRoot = mh_PortableRoot = NULL;
	CheckPortableReg();
	

	if (gOSVer.dwMajorVersion >= 6)
	{
		mb_IsUacAdmin = IsUserAdmin(); // Чтобы знать, может мы уже запущены под UAC админом?
	}
	else
	{
		mb_IsUacAdmin = FALSE;
	}
	
	//mh_Psapi = NULL;
	//GetModuleFileNameEx = (FGetModuleFileNameEx)GetProcAddress(GetModuleHandle(L"Kernel32.dll"), "GetModuleFileNameExW");
	//if (GetModuleFileNameEx == NULL)
	//{
	//	mh_Psapi = LoadLibrary(_T("psapi.dll"));
	//	if (mh_Psapi)
	//	    GetModuleFileNameEx = (FGetModuleFileNameEx)GetProcAddress(mh_Psapi, "GetModuleFileNameExW");
	//}

	#ifndef _WIN64
	mh_WinHook = NULL;
	#endif
	//mh_PopupHook = NULL;
	//mp_TaskBar2 = NULL;
	//mp_TaskBar3 = NULL;
	//mp_ VActive = NULL; mp_VCon1 = NULL; mp_VCon2 = NULL;
	//mb_CreatingActive = false;
	//memset(mp_VCon, 0, sizeof(mp_VCon));
	mp_AttachDlg = NULL;
	mp_RecreateDlg = NULL;

	// Dynamic messages
	mn__FirstAppMsg = WM_APP+10;
	m__AppMsgs.Init(128, true);
	// Go
	mn_MsgPostCreate = RegisterMessage("PostCreate");
	mn_MsgPostCopy = RegisterMessage("PostCopy");
	mn_MsgMyDestroy = RegisterMessage("MyDestroy");
	mn_MsgUpdateSizes = RegisterMessage("UpdateSizes");
	mn_MsgUpdateCursorInfo = RegisterMessage("UpdateCursorInfo");
	mn_MsgSetWindowMode = RegisterMessage("SetWindowMode");
	mn_MsgUpdateTitle = RegisterMessage("UpdateTitle");
	mn_MsgSrvStarted = RegisterMessage("SrvStarted"); //RegisterWindowMessage(CONEMUMSG_SRVSTARTED);
	mn_MsgUpdateScrollInfo = RegisterMessage("UpdateScrollInfo");
	mn_MsgUpdateTabs = RegisterMessage("UpdateTabs"); //RegisterWindowMessage(CONEMUMSG_UPDATETABS);
	mn_MsgOldCmdVer = RegisterMessage("OldCmdVer"); mb_InShowOldCmdVersion = FALSE;
	mn_MsgTabCommand = RegisterMessage("TabCommand");
	mn_MsgTabSwitchFromHook = RegisterMessage("CONEMUMSG_SWITCHCON",CONEMUMSG_SWITCHCON); //mb_InWinTabSwitch = FALSE;
	mn_MsgWinKeyFromHook = RegisterMessage("CONEMUMSG_HOOKEDKEY",CONEMUMSG_HOOKEDKEY);
	mn_PostConsoleResize = RegisterMessage("PostConsoleResize");
	mn_ConsoleLangChanged = RegisterMessage("ConsoleLangChanged");
	mn_MsgPostOnBufferHeight = RegisterMessage("PostOnBufferHeight");
	mn_MsgFlashWindow = RegisterMessage("CONEMUMSG_FLASHWINDOW",CONEMUMSG_FLASHWINDOW);
	mn_MsgPostAltF9 = RegisterMessage("PostAltF9");
	mn_MsgInitInactiveDC = RegisterMessage("InitInactiveDC");
	mn_MsgUpdateProcDisplay = RegisterMessage("UpdateProcDisplay");
	mn_MsgAutoSizeFont = RegisterMessage("AutoSizeFont");
	mn_MsgDisplayRConError = RegisterMessage("DisplayRConError");
	mn_MsgMacroFontSetName = RegisterMessage("MacroFontSetName");
	mn_MsgCreateViewWindow = RegisterMessage("CreateViewWindow");
	mn_MsgPostTaskbarActivate = RegisterMessage("PostTaskbarActivate"); mb_PostTaskbarActivate = FALSE;
	mn_MsgInitVConGhost = RegisterMessage("InitVConGhost");
	mn_MsgCreateCon = RegisterMessage("CreateCon");
	mn_MsgRequestUpdate = RegisterMessage("RequestUpdate");
	mn_MsgPanelViewMapCoord = RegisterMessage("CONEMUMSG_PNLVIEWMAPCOORD",CONEMUMSG_PNLVIEWMAPCOORD);
	mn_MsgTaskBarCreated = RegisterMessage("TaskbarCreated",L"TaskbarCreated");
	mn_MsgTaskBarBtnCreated = RegisterMessage("TaskbarButtonCreated",L"TaskbarButtonCreated");
	mn_MsgSheelHook = RegisterMessage("SHELLHOOK",L"SHELLHOOK");
	mn_MsgRequestRunProcess = RegisterMessage("RequestRunProcess");
	mn_MsgDeleteVConMainThread = RegisterMessage("DeleteVConMainThread");
	mn_MsgReqChangeCurPalette = RegisterMessage("ChangeCurrentPalette");
	mn_MsgMacroExecSync = RegisterMessage("MacroExecSync");
	mn_MsgActivateVCon = RegisterMessage("ActivateVCon");
}

bool CConEmuMain::isMingwMode()
{
	return (m_InstallMode & cm_MinGW) == cm_MinGW;
}

bool CConEmuMain::isUpdateAllowed()
{
	return !(m_InstallMode & (cm_MinGW));
}

bool CConEmuMain::isMSysStartup()
{
	return (m_InstallMode & cm_MSysStartup) == cm_MSysStartup;
}

void LogFocusInfo(LPCWSTR asInfo, int Level/*=1*/)
{
	if (!asInfo)
		return;

	if (Level <= gpSetCls->isAdvLogging)
	{
		gpConEmu->LogString(asInfo, TRUE);

	}

	#ifdef _DEBUG
	if ((Level == 1) || (Level <= gpSetCls->isAdvLogging))
	{
		wchar_t szFull[255];
		lstrcpyn(szFull, asInfo, countof(szFull)-3);
		wcscat_c(szFull, L"\n");
		DEBUGSTRFOCUS(szFull);
	}
	#endif
}

void CConEmuMain::StoreWorkDir(LPCWSTR asNewCurDir /*= NULL*/)
{
	if (asNewCurDir && (asNewCurDir[0] == L':'))
	{
		asNewCurDir = NULL; // Пути а-ля библиотеки - не поддерживаются при выполнении команд и приложений
	}

	if (asNewCurDir)
	{
		if (*asNewCurDir)
		{
			wcscpy_c(ms_ConEmuWorkDir, asNewCurDir);
		}
	}
	else
	{
		// Запомнить текущую папку (на момент запуска)
		DWORD nDirLen = GetCurrentDirectory(MAX_PATH, ms_ConEmuWorkDir);

		if (!nDirLen || nDirLen>MAX_PATH)
		{
			//ms_ConEmuWorkDir[0] = 0;
			wcscpy_c(ms_ConEmuWorkDir, ms_ConEmuExeDir);
		}
		else if (ms_ConEmuWorkDir[nDirLen-1] == L'\\')
		{
			ms_ConEmuWorkDir[nDirLen-1] = 0; // пусть будет БЕЗ слеша, для однообразия с ms_ConEmuExeDir
		}
	}

	wchar_t szDrive[MAX_PATH];
	SetEnvironmentVariable(ENV_CONEMUWORKDRIVE_VAR_W, GetDrive(ms_ConEmuWorkDir, szDrive, countof(szDrive)));
}

LPCWSTR CConEmuMain::WorkDir(LPCWSTR asOverrideCurDir /*= NULL*/)
{
	LPCWSTR pszWorkDir;
	if (asOverrideCurDir && *asOverrideCurDir)
	{
		pszWorkDir = asOverrideCurDir;
	}
	else
	{
		_ASSERTE(this && ms_ConEmuWorkDir[0]!=0);
		pszWorkDir = ms_ConEmuWorkDir;
	}

	if (pszWorkDir && (pszWorkDir[0] == L':'))
		pszWorkDir = L""; // Пути а-ля библиотеки - не поддерживаются при выполнении команд и приложений
	return pszWorkDir;
}

bool CConEmuMain::CheckRequiredFiles()
{
	wchar_t szPath[MAX_PATH+32];
	struct ReqFile {
		int  Bits;
		BOOL Req;
		wchar_t File[16];
	} Files[] = {
		{32, TRUE, L"ConEmuC.exe"},
		{64, TRUE, L"ConEmuC64.exe"},
		{32, TRUE, L"ConEmuCD.dll"},
		{64, TRUE, L"ConEmuCD64.dll"},
		{32, gpSet->isUseInjects, L"ConEmuHk.dll"},
		{64, gpSet->isUseInjects, L"ConEmuHk64.dll"},
	};
	
	wchar_t szRequired[128], szRecommended[128]; szRequired[0] = szRecommended[0] = 0;
	bool isWin64 = IsWindows64();
	int  nExeBits = WIN3264TEST(32,64);

	wcscpy_c(szPath, ms_ConEmuBaseDir);
	wcscat_c(szPath, L"\\");
	wchar_t* pszSlash = szPath + _tcslen(szPath);
	DWORD nFileSize;

	for (size_t i = 0; i < countof(Files); i++)
	{
		if (Files[i].Bits == 64)
		{
			if (!isWin64)
				continue; // 64битные файлы в 32битных ОС не нужны
		}

		_wcscpy_c(pszSlash, 32, Files[i].File);
		if (!FileExists(szPath, &nFileSize) || !nFileSize)
		{
			if (!Files[i].Req)
				continue;
			wchar_t* pszList = (Files[i].Bits == nExeBits) ? szRequired : szRecommended;
			if (*pszList)
				_wcscat_c(pszList, countof(szRequired), L", ");
			_wcscat_c(pszList, countof(szRequired), Files[i].File);
		}
	}

	if (*szRequired || *szRecommended)
	{
		size_t cchMax = _tcslen(szRequired) + _tcslen(szRecommended) + _tcslen(ms_ConEmuExe) + 255;
		wchar_t* pszMsg = (wchar_t*)calloc(cchMax, sizeof(*pszMsg));
		if (pszMsg)
		{
			_wcscpy_c(pszMsg, cchMax, *szRequired ? L"Critical error\n\n" : L"Warning\n\n");
			if (*szRequired)
			{
				_wcscat_c(pszMsg, cchMax, L"Required files not found!\n");
				_wcscat_c(pszMsg, cchMax, szRequired);
				_wcscat_c(pszMsg, cchMax, L"\n\n");
			}
			if (*szRecommended)
			{
				_wcscat_c(pszMsg, cchMax, L"Recommended files not found!\n");
				_wcscat_c(pszMsg, cchMax, szRecommended);
				_wcscat_c(pszMsg, cchMax, L"\n\n");
			}
			_wcscat_c(pszMsg, cchMax, L"ConEmu was started from:\n");
			_wcscat_c(pszMsg, cchMax, ms_ConEmuExe);
			_wcscat_c(pszMsg, cchMax, L"\n");
			if (*szRequired)
			{
				_wcscat_c(pszMsg, cchMax, L"\nConEmu will exits now");
			}
			MessageBox(NULL, pszMsg, GetDefaultTitle(), MB_SYSTEMMODAL|(*szRequired ? MB_ICONSTOP : MB_ICONWARNING));
			free(pszMsg);
		}
	}

	if (*szRequired)
		return false;
	return true; // Можно продолжать
}

bool CConEmuMain::CheckBaseDir()
{
	bool lbBaseFound = false;
	wchar_t szBaseFile[MAX_PATH+12];

	lstrcpyn(szBaseFile, ms_ConEmuBaseDir, countof(szBaseFile));
	wchar_t *pszSlash = szBaseFile + _tcslen(szBaseFile);
	lstrcpy(pszSlash, L"\\ConEmuC.exe");

	if (FileExists(szBaseFile))
	{
		lbBaseFound = true;
	}

	#ifdef WIN64
	if (!lbBaseFound)
	{
		lstrcpy(pszSlash, L"\\ConEmuC64.exe");

		if (FileExists(szBaseFile))
		{
			lbBaseFound = true;
		}
	}
	#endif

	return lbBaseFound;
}

bool CConEmuMain::SetConfigFile(LPCWSTR asFilePath, bool abWriteReq /*= false*/)
{
	LPCWSTR pszExt = asFilePath ? PointToExt(asFilePath) : NULL;
	int nLen = asFilePath ? lstrlen(asFilePath) : 0;

	if (!asFilePath || !*asFilePath || !pszExt || !*pszExt || (nLen > MAX_PATH))
	{
		DisplayLastError(L"Invalid file path specified in CConEmuMain::SetConfigFile", -1);
		return false;
	}

	wchar_t szPath[MAX_PATH+1] = L"";
	int nFileType = 0;

	if (lstrcmpi(pszExt, L".ini") == 0)
	{
		_ASSERTE(countof(ms_ConEmuIni)==countof(szPath));
		nFileType = 1;
	}
	else if (lstrcmpi(pszExt, L".xml") == 0)
	{
		_ASSERTE(countof(ms_ConEmuXml)==countof(szPath));
		nFileType = 0;
	}
	else
	{
		DisplayLastError(L"Unsupported file extension specified in CConEmuMain::SetConfigFile", -1);
		return false;
	}


	//The path must be already in full form, call GetFullPathName just for ensure (but it may return wrong path)
	_ASSERTE((asFilePath[1]==L':' && asFilePath[2]==L'\\') || (asFilePath[0]==L'\\' && asFilePath[1]==L'\\'));
	wchar_t* pszFilePart;
	nLen = GetFullPathName(asFilePath, countof(szPath), szPath, &pszFilePart);
	if ((nLen <= 0) || (nLen >= (INT_PTR)countof(szPath)))
	{
		DisplayLastError(L"Failed to expand specified file path in CConEmuMain::SetConfigFile");
		return false;
	}
	
	wchar_t* pszDirEnd = wcsrchr(szPath, L'\\');

	if (!pszDirEnd || (nLen > MAX_PATH))
	{
		DisplayLastError(L"Invalid file path specified in CConEmuMain::SetConfigFile (backslash not found)", -1);
		return false;
	}


	// Папки может не быть
	*pszDirEnd = 0;
	// Создадим, если нету
	MyCreateDirectory(szPath);
	*pszDirEnd = L'\\';

	// Нужно создать файл, если его нету.
	// Если просили доступ на запись - то однозначно CreateFile
	if (abWriteReq || !FileExists(szPath))
	{
		HANDLE hFile = CreateFile(szPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (!hFile || (hFile == INVALID_HANDLE_VALUE))
		{
			DWORD nErrCode = GetLastError();
			wchar_t szErrMsg[MAX_PATH*2];
			wcscpy_c(szErrMsg, L"Failed to create configuration file:\r\n");
			wcscat_c(szErrMsg, szPath);
			DisplayLastError(szErrMsg, nErrCode);
			return false;
		}
		CloseHandle(hFile);
	}

	if (nFileType == 1)
	{
		ms_ConEmuXml[0] = 0;
		wcscpy_c(ms_ConEmuIni, asFilePath);
	}
	else
	{
		ms_ConEmuIni[0] = 0;
		wcscpy_c(ms_ConEmuXml, asFilePath);
	}

	return true;
}

LPWSTR CConEmuMain::ConEmuXml()
{
	if (ms_ConEmuXml[0])
	{
		if (FileExists(ms_ConEmuXml))
			return ms_ConEmuXml;
	}

	TODO("Хорошо бы еще дать возможность пользователю использовать два файла - системный (предустановки) и пользовательский (настройки)");

	// Ищем файл портабельных настроек (возвращаем первый найденный, приоритет...)
	LPWSTR pszSearchXml[] = {
		ExpandEnvStr(L"%ConEmuDir%\\ConEmu.xml"),
		ExpandEnvStr(L"%ConEmuBaseDir%\\ConEmu.xml"),
		ExpandEnvStr(L"%APPDATA%\\ConEmu.xml"),
		NULL
	};

	for (size_t i = 0; pszSearchXml[i]; i++)
	{
		if (FileExists(pszSearchXml[i]))
		{
			wcscpy_c(ms_ConEmuXml, pszSearchXml[i]);
			goto fin;
		}
	}

	// Но если _создавать_ новый, то в BaseDir! Чтобы в корне не мусорить
	wcscpy_c(ms_ConEmuXml, ms_ConEmuBaseDir); wcscat_c(ms_ConEmuXml, L"\\ConEmu.xml");

fin:
	for (size_t i = 0; i < countof(pszSearchXml); i++)
	{
		SafeFree(pszSearchXml[i]);
	}
	return ms_ConEmuXml;
}

LPWSTR CConEmuMain::ConEmuIni()
{
	if (ms_ConEmuIni[0])
	{
		if (FileExists(ms_ConEmuIni))
			return ms_ConEmuIni;
	}

	TODO("Хорошо бы еще дать возможность пользователю использовать два файла - системный (предустановки) и пользовательский (настройки)");

	// Ищем файл портабельных настроек (возвращаем первый найденный, приоритет...)
	LPWSTR pszSearchIni[] = {
		ExpandEnvStr(L"%ConEmuDir%\\ConEmu.ini"),
		ExpandEnvStr(L"%ConEmuBaseDir%\\ConEmu.ini"),
		ExpandEnvStr(L"%APPDATA%\\ConEmu.ini"),
		NULL
	};

	for (size_t i = 0; pszSearchIni[i]; i++)
	{
		if (FileExists(pszSearchIni[i]))
		{
			wcscpy_c(ms_ConEmuIni, pszSearchIni[i]);
			goto fin;
		}
	}

	// Но если _создавать_ новый, то в BaseDir! Чтобы в корне не мусорить
	wcscpy_c(ms_ConEmuIni, ms_ConEmuBaseDir); wcscat_c(ms_ConEmuIni, L"\\ConEmu.ini");

fin:
	for (size_t i = 0; i < countof(pszSearchIni); i++)
	{
		SafeFree(pszSearchIni[i]);
	}
	return ms_ConEmuIni;
}

LPCWSTR CConEmuMain::ConEmuCExeFull(LPCWSTR asCmdLine/*=NULL*/)
{
	// Если OS - 32битная или в папке ConEmu был найден только один из "серверов"
	if (!IsWindows64() || !lstrcmp(ms_ConEmuC32Full, ms_ConEmuC64Full))
	{
		// Сразу вернуть
		return ms_ConEmuC32Full;
	}

	//LPCWSTR pszServer = ms_ConEmuC32Full;
	bool lbCmd = false, lbFound = false;
	int Bits = IsWindows64() ? 64 : 32;

	if (!asCmdLine || !*asCmdLine)
	{
		// Если строка запуска не указана - считаем, что запускается ComSpec
		lbCmd = true;
	}
	else
	{
		// Strip from cmdline some commands, wich ConEmuC can process internally
		ProcessSetEnvCmd(asCmdLine, false);

		// Проверить битность asCmdLine во избежание лишних запусков серверов для Inject
		// и корректной битности запускаемого процессора по настройке
		CmdArg szTemp;
		wchar_t* pszExpand = NULL;
		if (!FileExists(asCmdLine))
		{
			const wchar_t *psz = asCmdLine;
			if (NextArg(&psz, szTemp) == 0)
				asCmdLine = szTemp;
		}
		else
		{
			lbFound = true;
		}

		if (wcschr(asCmdLine, L'%'))
		{
			pszExpand = ExpandEnvStr(asCmdLine);
			if (pszExpand)
				asCmdLine = pszExpand;
		}

		// Если путь указан полностью - берем битность из него, иначе - проверяем "на cmd"
		if ((lstrcmpi(asCmdLine, L"cmd") == 0) || (lstrcmpi(asCmdLine, L"cmd.exe") == 0))
		{
			lbCmd = true;
		}
		else
		{
			LPCWSTR pszExt = PointToExt(asCmdLine);
			if (pszExt && (lstrcmpi(pszExt, L".exe") != 0) && (lstrcmpi(pszExt, L".com") != 0))
			{
				// Если указано расширение, и это не .exe и не .com - считаем, что запуск через ComProcessor
				lbCmd = true;
			}
			else
			{
				wchar_t szFind[MAX_PATH+1];
				wcscpy_c(szFind, asCmdLine);
				CharUpperBuff(szFind, lstrlen(szFind));
				// По хорошему, нужно бы проверить еще и начало на соответствие в "%WinDir%". Но это не критично.
				if (wcsstr(szFind, L"\\SYSNATIVE\\") || wcsstr(szFind, L"\\SYSWOW64\\"))
				{
					// Если "SysNative" - считаем что 32-bit, иначе, 64-битный сервер просто "не увидит" эту папку
					// С "SysWow64" все понятно, там только 32-битное
					Bits = 32;
				}
				else
				{
					MWow64Disable wow; wow.Disable();
					DWORD ImageSubsystem = 0, ImageBits = 0, FileAttrs = 0;
					lbFound = GetImageSubsystem(asCmdLine, ImageSubsystem, ImageBits, FileAttrs);
					// Если не нашли и путь не был указан
					// Даже если указан путь - это может быть путь к файлу без расширения. Его нужно "добавить".
					if (!lbFound /*&& !wcschr(asCmdLine, L'\\')*/)
					{
						LPCWSTR pszSearchFile = asCmdLine;
						LPCWSTR pszSlash = wcsrchr(asCmdLine, L'\\');
						wchar_t* pszSearchPath = NULL;
						if (pszSlash)
						{
							if ((pszSearchPath = lstrdup(asCmdLine)) != NULL)
							{
								pszSearchFile = pszSlash + 1;
								pszSearchPath[pszSearchFile - asCmdLine] = 0;
							}
						}

						// попытаемся найти
						wchar_t *pszFilePart;
						DWORD nLen = SearchPath(pszSearchPath, pszSearchFile, pszExt ? NULL : L".exe", countof(szFind), szFind, &pszFilePart);
						if (!nLen && !pszSearchPath)
						{
							wchar_t szRoot[MAX_PATH+1];
							wcscpy_c(szRoot, ms_ConEmuExeDir);
							wchar_t* pszSlash = wcsrchr(szRoot, L'\\');
							if (pszSlash)
								*pszSlash = 0;
							nLen = SearchPath(szRoot, pszSearchFile, pszExt ? NULL : L".exe", countof(szFind), szFind, &pszFilePart);
						}
						if (nLen && (nLen < countof(szFind)))
						{
							lbFound = GetImageSubsystem(szFind, ImageSubsystem, ImageBits, FileAttrs);
						}

						SafeFree(pszSearchPath);
					}

					if (lbFound)
						Bits = ImageBits;
				}
			}
		}

		SafeFree(pszExpand);
	}

	if (lbCmd)
	{
		if (gpSet->ComSpec.csBits == csb_SameApp)
			Bits = WIN3264TEST(32,64);
		else if (gpSet->ComSpec.csBits == csb_x32)
			Bits = 32;
	}

	return (Bits == 64) ? ms_ConEmuC64Full : ms_ConEmuC32Full;
}

BOOL CConEmuMain::Init()
{
	_ASSERTE(mp_TabBar == NULL);

	// Чтобы не блокировать папку запуска - CD
	SetCurrentDirectory(ms_ConEmuExeDir);

	// Только по настройке, а то дочерние процессы с тем же Affinity запускаются...
	// На тормоза - не влияет. Но вроде бы на многопроцессорных из-за глюков в железе могут быть ошибки подсчета производительности, если этого не сделать
	if (gpSet->nAffinity)
		SetProcessAffinityMask(GetCurrentProcess(), gpSet->nAffinity);

	ConEmuAbout::InitCommCtrls();

	//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
	//SetThreadAffinityMask(GetCurrentThread(), 1);
	/*DWORD dwErr = 0;
	HMODULE hInf = LoadLibrary(L"infis.dll");
	if (!hInf)
	dwErr = GetLastError();*/
	//!!!ICON
	LoadIcons();

	mp_Tip = new CToolTip();
	mp_TabBar = new TabBarClass();
	//m_Child = new CConEmuChild();
	//m_Back = new CConEmuBack();
	//m_Macro = new CConEmuMacro();
	//#pragma message("Win2k: EVENT_CONSOLE_START_APPLICATION, EVENT_CONSOLE_END_APPLICATION")
	//Нас интересуют только START и END. Все остальные события приходят от ConEmuC через серверный пайп
	//#if defined(__GNUC__)
	//HMODULE hUser32 = GetModuleHandle(L"user32.dll");
	//FSetWinEventHook SetWinEventHook = (FSetWinEventHook)GetProcAddress(hUser32, "SetWinEventHook");
	//#endif
#ifndef _WIN64
	// Интересует только запуск/останов 16битных приложений
	// NTVDM нету в 64битных версиях Windows
	if (!IsWindows64())
	{
		mh_WinHook = SetWinEventHook(EVENT_CONSOLE_START_APPLICATION/*EVENT_CONSOLE_CARET*/,EVENT_CONSOLE_END_APPLICATION,
	                             NULL, (WINEVENTPROC)CConEmuMain::WinEventProc, 0,0, WINEVENT_OUTOFCONTEXT);
	}
#endif
	//mh_PopupHook = SetWinEventHook(EVENT_SYSTEM_MENUPOPUPSTART,EVENT_SYSTEM_MENUPOPUPSTART,
	//    NULL, (WINEVENTPROC)CConEmuMain::WinEventProc, 0,0, WINEVENT_OUTOFCONTEXT);
	/*mh_Psapi = LoadLibrary(_T("psapi.dll"));
	if (mh_Psapi) {
	    GetModuleFileNameEx = (FGetModuleFileNameEx)GetProcAddress(mh_Psapi, "GetModuleFileNameExW");
	    if (GetModuleFileNameEx)
	        return TRUE;
	}*/
	/*DWORD dwErr = GetLastError();
	TCHAR szErr[255];
	_wsprintf(szErr, countof(szErr), _T("Can't initialize psapi!\r\nLastError = 0x%08x"), dwErr);
	MBoxA(szErr);
	return FALSE;*/

	InitFrameHolder();

	return TRUE;
}

bool CConEmuMain::IsResetBasicSettings()
{
	return gpSetCls->isResetBasicSettings;
}

bool CConEmuMain::IsFastSetupDisabled()
{
	return (gpSetCls->isResetBasicSettings || gpSetCls->isFastSetupDisabled);
}

bool CConEmuMain::IsAllowSaveSettingsOnExit()
{
	return !(gpSetCls->ibDisableSaveSettingsOnExit || gpSetCls->ibExitAfterDefTermSetup || IsResetBasicSettings());
}

void CConEmuMain::OnUseGlass(bool abEnableGlass)
{
	//CheckMenuItem(GetSysMenu(false), ID_USEGLASS, MF_BYCOMMAND | (abEnableGlass ? MF_CHECKED : MF_UNCHECKED));
}
void CConEmuMain::OnUseTheming(bool abEnableTheming)
{
	//CheckMenuItem(GetSysMenu(false), ID_USETHEME, MF_BYCOMMAND | (abEnableTheming ? MF_CHECKED : MF_UNCHECKED));
}
void CConEmuMain::OnUseDwm(bool abEnableDwm)
{
	//CheckMenuItem(GetSysMenu(false), ID_ISDWM, MF_BYCOMMAND | (abEnableDwm ? MF_CHECKED : MF_UNCHECKED));
}

// Например при вызове из диалога "Settings..." и нажатия кнопки "Apply" (Size & Pos)
bool CConEmuMain::SizeWindow(const CESize sizeW, const CESize sizeH)
{
	if (mp_Inside)
	{
		// В Inside - размер окна не меняется. Оно всегда подгоняется под родителя.
		return false;
	}

	ConEmuWindowMode wndMode = GetWindowMode();
	if (wndMode == wmMaximized || wndMode == wmFullScreen)
	{
		// Если окно развернуто - его размер таким образом (по консоли например) менять запрещается

		// Quake??? Разрешить?

		_ASSERTE(wndMode != wmMaximized && wndMode != wmFullScreen);
		return false;
	}

	// Установить размер окна по расчетным значениям

	RECT rcMargins = CalcMargins(CEM_FRAMECAPTION|CEM_SCROLL|CEM_STATUS|CEM_PAD|CEM_TAB);
	SIZE szPixelSize = GetDefaultSize(false, &sizeW, &sizeH);

	_ASSERTE(rcMargins.left>=0 && rcMargins.right>=0 && rcMargins.top>=0 && rcMargins.bottom>=0);

	int nWidth  = szPixelSize.cx + rcMargins.left + rcMargins.right;
	int nHeight = szPixelSize.cy + rcMargins.top + rcMargins.bottom;

	bool bSizeOK = false;

	WndWidth.Set(true, sizeW.Style, sizeW.Value);
	WndHeight.Set(false, sizeH.Style, sizeH.Value);

	if (SetWindowPos(ghWnd, NULL, 0,0, nWidth, nHeight, SWP_NOMOVE|SWP_NOZORDER))
	{
		bSizeOK = true;
	}

	return bSizeOK;
}

// bCells==true - вернуть расчетный консольный размер, bCells==false - размер в пикселях (только консоль, без рамок-отступов-табов-прочая)
// pSizeW и pSizeH указываются если нужно установить конкретные размеры
// hMon может быть указан при переносе окна между мониторами например
SIZE CConEmuMain::GetDefaultSize(bool bCells, const CESize* pSizeW/*=NULL*/, const CESize* pSizeH/*=NULL*/, HMONITOR hMon/*=NULL*/)
{
	//WARNING! Function must NOT call CalcRect to avoid cycling!

	_ASSERTE(mp_Inside == NULL); // Must not be called in "Inside"?

	SIZE sz = {80,25}; // This has no matter unless fatal errors

	CESize sizeW = {WndWidth.Raw};
	if (pSizeW && pSizeW->Value)
		sizeW.Set(true, pSizeW->Style, pSizeW->Value);
	CESize sizeH = {WndHeight.Raw};
	if (pSizeH && pSizeH->Value)
		sizeH.Set(false, pSizeH->Style, pSizeH->Value);

	// Шрифт
	int nFontWidth = gpSetCls->FontWidth();
	int nFontHeight = gpSetCls->FontHeight();
	if ((nFontWidth <= 0) || (nFontHeight <= 0))
	{
		Assert(nFontWidth>0 && nFontHeight>0);
		return sz;
	}



	RECT rcFrameMargin = CalcMargins(CEM_FRAMECAPTION|CEM_SCROLL|CEM_STATUS|CEM_PAD);
	int nFrameX = rcFrameMargin.left + rcFrameMargin.right;
	int nFrameY = rcFrameMargin.top + rcFrameMargin.bottom;
	RECT rcTabMargins = mp_TabBar->GetMargins();

	COORD conSize = {80, 25};

	// Если табы показываются всегда - сразу добавим их размер, чтобы размер консоли был заказанным
	bool bTabs = this->isTabsShown();
	int nShiftX = nFrameX + (bTabs ? (rcTabMargins.left+rcTabMargins.right) : 0);
	int nShiftY = nFrameY + (bTabs ? (rcTabMargins.top+rcTabMargins.bottom) : 0);

	ConEmuWindowMode wmCurMode = GetWindowMode();

	// Информация по монитору нам нужна только если размеры заданы в процентах
	MONITORINFO mi = {sizeof(mi)};
	if ((sizeW.Style == ss_Percents || sizeH.Style == ss_Percents)
		|| gpSet->isQuakeStyle
		|| (wmCurMode == wmMaximized || wmCurMode == wmFullScreen))
	{
		if (hMon != NULL)
		{
			GetMonitorInfoSafe(hMon, mi);
		}
		// по ghWnd - монитор не ищем (окно может быть свернуто), только по координатам
		else
		{
			RECT rc = {this->wndX, this->wndY, this->wndX, this->wndY};
			GetNearestMonitorInfo(&mi, NULL, &rc);
		}
	}

	//// >> rcWnd
	//rcWnd.left = this->wndX;
	//rcWnd.top = this->wndY;

	int nPixelWidth = 0, nPixelHeight = 0;

	// Calculate width
	if (sizeW.Style == ss_Pixels)
	{
		nPixelWidth = sizeW.Value;
	}
	else if (sizeW.Style == ss_Percents)
	{
		nPixelWidth = ((mi.rcWork.right - mi.rcWork.left) * sizeW.Value / 100) - nShiftX;
	}
	else if (sizeW.Value > 0)
	{
		conSize.X = sizeW.Value;
	}

	if (nPixelWidth <= 0)
		nPixelWidth = conSize.X * nFontWidth;

	//// >> rcWnd
	//rcWnd.right = this->wndX + nPixelWidth + nShiftX;

	// Calculate height
	if (sizeH.Style == ss_Pixels)
	{
		nPixelHeight = sizeH.Value;
	}
	else if (sizeH.Style == ss_Percents)
	{
		nPixelHeight = ((mi.rcWork.bottom - mi.rcWork.top) * sizeH.Value / 100) - nShiftY;
	}
	else if (sizeH.Value > 0)
	{
		conSize.Y = sizeH.Value;
	}
	
	if (nPixelHeight <= 0)
		nPixelHeight = conSize.Y * nFontHeight;


	// Для Quake & FS/Maximized нужно убедиться, что размеры НЕ превышают размеры монитора/рабочей области
	if (gpSet->isQuakeStyle || (wmCurMode == wmMaximized || wmCurMode == wmFullScreen))
	{
		if (mi.cbSize)
		{
			int nMaxWidth  = (wmCurMode == wmFullScreen) ? (mi.rcMonitor.right - mi.rcMonitor.left) : (mi.rcWork.right - mi.rcWork.left);
			int nMaxHeight = (wmCurMode == wmFullScreen) ? (mi.rcMonitor.bottom - mi.rcMonitor.top) : (mi.rcWork.bottom - mi.rcWork.top);
			// Maximized/Fullscreen - frame may come out of working area
			RECT rcFrameOnly = CalcMargins(CEM_FRAMEONLY);
			_ASSERTE(nFrameX>=0 && nFrameY>=0 && rcFrameOnly.left>=0 && rcFrameOnly.top>=0 && rcFrameOnly.right>=0 && rcFrameOnly.bottom>=0);
			// 131029 - Was nFrameX&nFrameY, which was less then required. Quake goes of the screen
			nMaxWidth += (rcFrameOnly.left + rcFrameOnly.right) - nShiftX;
			nMaxHeight += (rcFrameOnly.top + rcFrameOnly.bottom) - nShiftY;
			// Check it
			if (nPixelWidth > nMaxWidth)
				nPixelWidth = nMaxWidth;
			if (nPixelHeight > nMaxHeight)
				nPixelHeight = nMaxHeight;
		}
		else
		{
			_ASSERTE(mi.cbSize!=0);
		}
	}


	//// >> rcWnd
	//rcWnd.bottom = this->wndY + nPixelHeight + nShiftY;

	if (bCells)
	{
		sz.cx = nPixelWidth / nFontWidth;
		sz.cy = nPixelHeight / nFontHeight;
	}
	else
	{
		sz.cx = nPixelWidth;
		sz.cy = nPixelHeight;
	}

	return sz;
}

bool CConEmuMain::isTabsShown()
{
	if (gpSet->isTabs == 1)
		return true;
	if (mp_TabBar && mp_TabBar->IsTabsShown())
		return true;
	return false;
}

// Вызывается при старте программы, для вычисления mrc_Ideal - размера окна по умолчанию
RECT CConEmuMain::GetDefaultRect()
{
	RECT rcWnd = {};

	if (mp_Inside)
	{
		if (mp_Inside->GetInsideRect(&rcWnd))
		{
			WindowMode = wmNormal;

			this->wndX = rcWnd.left;
			this->wndY = rcWnd.top;
			RECT rcCon = CalcRect(CER_CONSOLE_ALL, rcWnd, CER_MAIN);
			// In the "Inside" mode we are interested only in "cells"
			this->WndWidth.Set(true, ss_Standard, rcCon.right);
			this->WndHeight.Set(false, ss_Standard, rcCon.bottom);

			OnMoving(&rcWnd);

			return rcWnd;
		}
	}

	Assert(gpSetCls->FontWidth() && gpSetCls->FontHeight());


	RECT rcFrameMargin = CalcMargins(CEM_FRAMECAPTION|CEM_SCROLL|CEM_STATUS|CEM_PAD);
	int nFrameX = rcFrameMargin.left + rcFrameMargin.right;
	int nFrameY = rcFrameMargin.top + rcFrameMargin.bottom;
	RECT rcTabMargins = mp_TabBar->GetMargins();

	// Если табы показываются всегда (или сейчас) - сразу добавим их размер, чтобы размер консоли был заказанным
	bool bTabs = this->isTabsShown();
	int nShiftX = nFrameX + (bTabs ? (rcTabMargins.left+rcTabMargins.right) : 0);
	int nShiftY = nFrameY + (bTabs ? (rcTabMargins.top+rcTabMargins.bottom) : 0);

	// Расчет размеров
	SIZE szConPixels = GetDefaultSize(false);

	// >> rcWnd
	rcWnd.left = this->wndX;
	rcWnd.top = this->wndY;
	rcWnd.right = this->wndX + szConPixels.cx + nShiftX;
	rcWnd.bottom = this->wndY + szConPixels.cy + nShiftY;


	// Go
	if (gpSet->isQuakeStyle)
	{
		HMONITOR hMon;
		POINT pt = {this->wndX+2*nFrameX,this->wndY+2*nFrameY};
		if (ghWnd)
		{
			RECT rcReal; GetWindowRect(ghWnd, &rcReal);
			pt.x = (rcReal.left+rcReal.right)/2;
			pt.y = (rcReal.top+rcReal.bottom)/2;
		}
		hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);

		MONITORINFO mi = {sizeof(mi)};
		GetMonitorInfoSafe(hMon, mi);

		bool bChange = false;

		// Если успешно - подгоняем по экрану
		if (mi.rcWork.right > mi.rcWork.left)
		{
			int nWidth = rcWnd.right - rcWnd.left;
			int nHeight = rcWnd.bottom - rcWnd.top;

			RECT rcFrameOnly = CalcMargins(CEM_FRAMECAPTION);

			switch (gpSet->_WindowMode)
			{
				case wmMaximized:
				{
					rcWnd.left = mi.rcWork.left - rcFrameOnly.left;
					rcWnd.right = mi.rcWork.right + rcFrameOnly.right;
					rcWnd.top = mi.rcWork.top - rcFrameOnly.top;
					rcWnd.bottom = rcWnd.top + nHeight;

					bChange = true;
					break;
				}

				case wmFullScreen:
				{
					rcWnd.left = mi.rcMonitor.left - rcFrameOnly.left;
					rcWnd.right = mi.rcMonitor.right + rcFrameOnly.right;
					rcWnd.top = mi.rcMonitor.top - rcFrameOnly.top;
					rcWnd.bottom = rcWnd.top + nHeight;

					bChange = true;
					break;
				}

				case wmNormal:
				{
					// Если выбран режим "Fixed" - разрешим задавать левую координату
					if (!gpSet->wndCascade)
						rcWnd.left = max(mi.rcWork.left,min(gpConEmu->wndX,(mi.rcWork.right - nWidth)));
					else
						rcWnd.left = max(mi.rcWork.left,((mi.rcWork.left + mi.rcWork.right - nWidth) / 2));
					rcWnd.right = min(mi.rcWork.right,(rcWnd.left + nWidth));
					rcWnd.top = mi.rcWork.top - rcFrameOnly.top;
					rcWnd.bottom = rcWnd.top + nHeight;

					bChange = true;
					break;
				}
			}
		}

		if (bChange)
		{
			ptFullScreenSize.x = rcWnd.right - rcWnd.left + 1;
			ptFullScreenSize.y = mi.rcMonitor.bottom - mi.rcMonitor.top + nFrameY;

			DEBUGTEST(RECT rcCon = CalcRect(CER_CONSOLE_ALL, rcWnd, CER_MAIN));
			//if (rcCon.right)
			//	this->wndWidth = rcCon.right;

			this->wndX = rcWnd.left;
			this->wndY = rcWnd.top;
		}
	}
	// При старте, если указан "Cascade" (то есть только при создании окна)
	else if (gpSet->wndCascade && !ghWnd)
	{
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

		//-- SM_CXSIZEFRAME/SM_CYSIZEFRAME fails in Windows 8
		//int nX = GetSystemMetrics(SM_CXSIZEFRAME), nY = GetSystemMetrics(SM_CYSIZEFRAME);
		RECT rcFrame = CalcMargins(CEM_FRAMEONLY, wmNormal);
		int nX = rcFrame.left, nY = rcFrame.top;
		int nWidth = rcWnd.right - rcWnd.left;
		int nHeight = rcWnd.bottom - rcWnd.top;
		// Теперь, если новый размер выходит за пределы экрана - сдвинуть в левый верхний угол
		BOOL lbX = ((rcWnd.left+nWidth)>(rcScreen.right+nX));
		BOOL lbY = ((rcWnd.top+nHeight)>(rcScreen.bottom+nY));

		if (lbX && lbY)
		{
			rcWnd = MakeRect(rcScreen.left,rcScreen.top,rcScreen.left+nWidth,rcScreen.top+nHeight);
		}
		else if (lbX)
		{
			rcWnd.left = rcScreen.right - nWidth; rcWnd.right = rcScreen.right;
		}
		else if (lbY)
		{
			rcWnd.top = rcScreen.bottom - nHeight; rcWnd.bottom = rcScreen.bottom;
		}

		if (rcWnd.left<(rcScreen.left-nX))
		{
			rcWnd.left=rcScreen.left-nX; rcWnd.right=rcWnd.left+nWidth;
		}

		if (rcWnd.top<(rcScreen.top-nX))
		{
			rcWnd.top=rcScreen.top-nX; rcWnd.bottom=rcWnd.top+nHeight;
		}

		if ((rcWnd.left+nWidth)>(rcScreen.right+nX))
		{
			rcWnd.left = max((rcScreen.left-nX),(rcScreen.right-nWidth));
			nWidth = min(nWidth, (rcScreen.right-rcWnd.left+2*nX));
			rcWnd.right = rcWnd.left + nWidth;
		}

		if ((rcWnd.top+nHeight)>(rcScreen.bottom+nY))
		{
			rcWnd.top = max((rcScreen.top-nY),(rcScreen.bottom-nHeight));
			nHeight = min(nHeight, (rcScreen.bottom-rcWnd.top+2*nY));
			rcWnd.bottom = rcWnd.top + nHeight;
		}

		// Скорректировать X/Y при каскаде
		this->wndX = rcWnd.left;
		this->wndY = rcWnd.top;
	}

	OnMoving(&rcWnd);

	return rcWnd;
}

RECT CConEmuMain::GetGuiClientRect()
{
	RECT rcClient = CalcRect(CER_MAINCLIENT);
	//RECT rcClient = {};
	//BOOL lbRc = ::GetClientRect(ghWnd, &rcClient); UNREFERENCED_PARAMETER(lbRc);
	return rcClient;
}

RECT CConEmuMain::GetVirtualScreenRect(BOOL abFullScreen)
{
	RECT rcScreen = MakeRect(800,600);
	int nMonitors = GetSystemMetrics(SM_CMONITORS);

	if (nMonitors > 1)
	{
		// Размер виртуального экрана по всем мониторам
		rcScreen.left = GetSystemMetrics(SM_XVIRTUALSCREEN); // may be <0
		rcScreen.top  = GetSystemMetrics(SM_YVIRTUALSCREEN);
		rcScreen.right = rcScreen.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
		rcScreen.bottom = rcScreen.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);

		// Хорошо бы исключить из рассмотрения Taskbar и прочие панели
		if (!abFullScreen)
		{
			RECT rcPrimaryWork;

			if (SystemParametersInfo(SPI_GETWORKAREA, 0, &rcPrimaryWork, 0))
			{
				if (rcPrimaryWork.left>0 && rcPrimaryWork.left<rcScreen.right)
					rcScreen.left = rcPrimaryWork.left;

				if (rcPrimaryWork.top>0 && rcPrimaryWork.top<rcScreen.bottom)
					rcScreen.top = rcPrimaryWork.top;

				if (rcPrimaryWork.right<rcScreen.right && rcPrimaryWork.right>rcScreen.left)
					rcScreen.right = rcPrimaryWork.right;

				if (rcPrimaryWork.bottom<rcScreen.bottom && rcPrimaryWork.bottom>rcScreen.top)
					rcScreen.bottom = rcPrimaryWork.bottom;
			}
		}
	}
	else
	{
		if (abFullScreen)
		{
			rcScreen = MakeRect(GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN));
		}
		else
		{
			SystemParametersInfo(SPI_GETWORKAREA, 0, &rcScreen, 0);
		}
	}

	if (rcScreen.right<=rcScreen.left || rcScreen.bottom<=rcScreen.top)
	{
		_ASSERTE(rcScreen.right>rcScreen.left && rcScreen.bottom>rcScreen.top);
		rcScreen = MakeRect(800,600);
	}

	return rcScreen;
}

void CConEmuMain::SetWindowStyle(DWORD anStyle)
{
	SetWindowStyle(ghWnd, anStyle);
}

void CConEmuMain::SetWindowStyle(HWND ahWnd, DWORD anStyle)
{
	LONG lRc = SetWindowLong(ahWnd, GWL_STYLE, anStyle);
	UNREFERENCED_PARAMETER(lRc);
}

// Эта функция расчитывает необходимые стили по текущим настройкам, а не возвращает GWL_STYLE
DWORD CConEmuMain::GetWindowStyle()
{
	DWORD style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

	if (mp_Inside)
	{
		style |= WS_CHILD|WS_SYSMENU;
	}
	else
	{
		//if (gpSet->isShowOnTaskBar) // ghWndApp
		//	style |= WS_POPUPWINDOW | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
		//else
		style |= WS_OVERLAPPEDWINDOW;
		style = FixWindowStyle(style);
		//if (gpSet->isTabsOnTaskBar() && gOSVer.dwMajorVersion <= 5)
		//{
		//	style |= WS_POPUP;
		//}

		#ifndef CHILD_DESK_MODE
		if (gpSet->isDesktopMode)
			style |= WS_POPUP;
		#endif

		//if (ghWnd) {
		//	if (gpSet->isHideCaptionAlways)
		//		style &= ~(WS_CAPTION/*|WS_THICKFRAME*/);
		//	else
		//		style |= (WS_CAPTION|/*WS_THICKFRAME|*/WS_MINIMIZEBOX|WS_MAXIMIZEBOX);
		//}
	}
	return style;
}

DWORD CConEmuMain::FixWindowStyle(DWORD dwStyle, ConEmuWindowMode wmNewMode /*= wmCurrent*/)
{
	/* WS_CAPTION == (WS_BORDER | WS_DLGFRAME) */

	if ((wmNewMode == wmCurrent) || (wmNewMode == wmNotChanging))
	{
		wmNewMode = WindowMode;
	}

	if (mp_Inside)
	{
		dwStyle &= ~(WS_CAPTION|WS_THICKFRAME);
		dwStyle |= WS_CHILD|WS_SYSMENU;
	}
	else if (gpSet->isCaptionHidden(wmNewMode))
	{
		//Win& & Quake - не работает "Slide up/down" если есть ThickFrame
		//if ((gpSet->isQuakeStyle == 0) // не для Quake. Для него нужна рамка, чтобы ресайзить
		if ((wmNewMode == wmFullScreen) || (wmNewMode == wmMaximized))
		{
			dwStyle &= ~(WS_CAPTION|WS_THICKFRAME);
		}
		else if ((m_ForceShowFrame == fsf_Show) || !gpSet->isFrameHidden())
		{
			dwStyle &= ~(WS_CAPTION);
			dwStyle |= WS_THICKFRAME;
		}
		else
		{
			dwStyle &= ~(WS_CAPTION|WS_THICKFRAME);
			dwStyle |= WS_DLGFRAME;
		}
	}
	else
	{
		/* WS_CAPTION == WS_BORDER | WS_DLGFRAME  */
		dwStyle |= WS_CAPTION|WS_THICKFRAME;
	}

	return dwStyle;
}

void CConEmuMain::SetWindowStyleEx(DWORD anStyleEx)
{
	SetWindowStyleEx(ghWnd, anStyleEx);
}

void CConEmuMain::SetWindowStyleEx(HWND ahWnd, DWORD anStyleEx)
{
	if (gpSetCls->isAdvLogging)
	{
		char szInfo[100];
		RECT rcWnd = {}; GetWindowRect(ghWnd, &rcWnd);
		_wsprintfA(szInfo, SKIPLEN(countof(szInfo))
			"SetWindowStyleEx(HWND=x%08X, StyleEx=x%08X)",
			(DWORD)(DWORD_PTR)ahWnd, anStyleEx);
		LogString(szInfo);
	}
	LONG lRc = SetWindowLong(ahWnd, GWL_EXSTYLE, anStyleEx);
	UNREFERENCED_PARAMETER(lRc);
}

// Эта функция расчитывает необходимые стили по текущим настройкам, а не возвращает GWL_STYLE_EX
DWORD CConEmuMain::GetWindowStyleEx()
{
	DWORD styleEx = 0;

	if (mp_Inside)
	{
		// ничего вроде не надо
	}
	else
	{
		if (gpSet->nTransparent < 255 /*&& !gpSet->isDesktopMode*/)
			styleEx |= WS_EX_LAYERED;

		if (gpSet->isAlwaysOnTop)
			styleEx |= WS_EX_TOPMOST;

		if (gpSet->isWindowOnTaskBar(true))
			styleEx |= WS_EX_APPWINDOW;

		#ifndef CHILD_DESK_MODE
		if (gpSet->isDesktopMode)
			styleEx |= WS_EX_TOOLWINDOW;
		#endif
	}

	return styleEx;
}

// Эта функция расчитывает необходимые стили по текущим настройкам, а не возвращает GWL_STYLE
DWORD CConEmuMain::GetWorkWindowStyle()
{
	DWORD style = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE;
	return style;
}

// Эта функция расчитывает необходимые стили по текущим настройкам, а не возвращает GWL_STYLE_EX
DWORD CConEmuMain::GetWorkWindowStyleEx()
{
	DWORD styleEx = 0;
	return styleEx;
}

bool CConEmuMain::CreateLog()
{
	_ASSERTE(gpSetCls->isAdvLogging && !mp_Log);

	if (mp_Log!=NULL)
	{
		_ASSERTE(mp_Log==NULL);
		return true; // создан уже
	}

	bool bRc = false;
	EnterCriticalSection(&mcs_Log);

	mp_Log = new MFileLog(L"ConEmu-gui", ms_ConEmuExeDir, GetCurrentProcessId());

	HRESULT hr = mp_Log ? mp_Log->CreateLogFile(L"ConEmu-gui", GetCurrentProcessId(), gpSetCls->isAdvLogging) : E_UNEXPECTED;
	if (hr != 0)
	{
		wchar_t szError[MAX_PATH*2];
		_wsprintf(szError, SKIPLEN(countof(szError)) L"Create log file failed! ErrCode=0x%08X\n%s\n", (DWORD)hr, mp_Log->GetLogFileName());
		MBoxA(szError);
		SafeDelete(mp_Log);
		// Сброс!
		gpSetCls->isAdvLogging = 0;
	}
	else
	{
		mp_Log->LogStartEnv(gpStartEnv);
		bRc = true;
	}

	LeaveCriticalSection(&mcs_Log);

	return bRc;
}

void CConEmuMain::LogString(LPCWSTR asInfo, bool abWriteTime /*= true*/, bool abWriteLine /*= true*/)
{
	if (!this || !mp_Log)
		return;

	mp_Log->LogString(asInfo, abWriteTime, NULL, abWriteLine);
}

void CConEmuMain::LogString(LPCSTR asInfo, bool abWriteTime /*= true*/, bool abWriteLine /*= true*/)
{
	if (!this || !mp_Log)
		return;

	mp_Log->LogString(asInfo, abWriteTime, NULL, abWriteLine);
}

BOOL CConEmuMain::CreateMainWindow()
{
	//if (!Init()) -- вызывается из WinMain
	//	return FALSE; // Ошибка уже показана

	if (_tcscmp(VirtualConsoleClass,VirtualConsoleClassMain)) //-V549
	{
		MBoxA(_T("Error: class names must be equal!"));
		return FALSE;
	}

	// 2009-06-11 Возможно, что CS_SAVEBITS приводит к глюкам отрисовки
	// банально непрорисовываются некоторые части экрана (драйвер видюхи глючит?)
	WNDCLASSEX wc = {sizeof(WNDCLASSEX), CS_DBLCLKS|CS_OWNDC/*|CS_SAVEBITS*/, CConEmuMain::MainWndProc, 0, 16,
	                 g_hInstance, hClassIcon, LoadCursor(NULL, IDC_ARROW),
	                 NULL /*(HBRUSH)COLOR_BACKGROUND*/,
	                 NULL, gsClassNameParent, hClassIconSm
	                };// | CS_DROPSHADOW

	WNDCLASSEX wcWork = {sizeof(WNDCLASSEX), CS_DBLCLKS|CS_OWNDC/*|CS_SAVEBITS*/, CConEmuMain::WorkWndProc, 0, 16,
	                 g_hInstance, hClassIcon, LoadCursor(NULL, IDC_ARROW),
	                 NULL /*(HBRUSH)COLOR_BACKGROUND*/,
	                 NULL, gsClassNameWork, hClassIconSm
	                };// | CS_DROPSHADOW

	WNDCLASSEX wcBack = {sizeof(WNDCLASSEX), CS_DBLCLKS|CS_OWNDC/*|CS_SAVEBITS*/, CConEmuChild::BackWndProc, 0, 16,
	                 g_hInstance, hClassIcon, LoadCursor(NULL, IDC_ARROW),
	                 NULL /*(HBRUSH)COLOR_BACKGROUND*/,
	                 NULL, gsClassNameBack, hClassIconSm
	                };// | CS_DROPSHADOW

	if (!RegisterClassEx(&wc) || !RegisterClassEx(&wcWork) || !RegisterClassEx(&wcBack))
	{
		DisplayLastError(L"Failed to register class names");
		return -1;
	}

	if (mp_Inside)
	{
		if (!mp_Inside->mh_InsideParentWND)
		{
			_ASSERTE(mp_Inside == NULL); // Must be cleared already!
			_ASSERTE(!mp_Inside->m_InsideIntegration || mp_Inside->mh_InsideParentWND);
			//m_InsideIntegration = ii_None;
			SafeDelete(mp_Inside);
		}
		else
		{
			DWORD nParentTID, nParentPID;
			nParentTID = GetWindowThreadProcessId(mp_Inside->mh_InsideParentWND, &nParentPID);
			_ASSERTE(nParentTID && nParentPID);
			BOOL bAttach = AttachThreadInput(GetCurrentThreadId(), nParentTID, TRUE);
			if (!bAttach)
			{
				DisplayLastError(L"Inside: AttachThreadInput() failed!");
			}
		}
	}

	DWORD styleEx = GetWindowStyleEx();
	DWORD style = GetWindowStyle();
	//	WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	////if (gpSet->isShowOnTaskBar) // ghWndApp
	////	style |= WS_POPUPWINDOW | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
	////else
	//style |= WS_OVERLAPPEDWINDOW;
	//if (gpSet->nTransparent < 255 /*&& !gpSet->isDesktopMode*/)
	//	styleEx |= WS_EX_LAYERED;
	//if (gpSet->isAlwaysOnTop)
	//	styleEx |= WS_EX_TOPMOST;
	////if (gpSet->isHideCaptionAlways) // сразу создать так почему-то не получается
	////	style &= ~(WS_CAPTION);
	int nWidth=CW_USEDEFAULT, nHeight=CW_USEDEFAULT;

	//WindowMode may be changed in SettingsLoaded
	//// In principle, this MUST be wmNormal on startup, even if started in Maximized/FullScreen/Iconic
	//_ASSERTE(WindowMode==wmNormal);

	// Расчет размеров окна в Normal режиме
	if ((this->WndWidth.Value && this->WndHeight.Value) || mp_Inside)
	{
		MBoxAssert(gpSetCls->FontWidth() && gpSetCls->FontHeight());
		//COORD conSize; conSize.X=gpSet->wndWidth; conSize.Y=gpSet->wndHeight;
		////int nShiftX = GetSystemMetrics(SM_CXSIZEFRAME)*2;
		////int nShiftY = GetSystemMetrics(SM_CYSIZEFRAME)*2 + (gpSet->isHideCaptionAlways ? 0 : GetSystemMetrics(SM_CYCAPTION));
		//RECT rcFrameMargin = CalcMargins(CEM_FRAME);
		//int nShiftX = rcFrameMargin.left + rcFrameMargin.right;
		//int nShiftY = rcFrameMargin.top + rcFrameMargin.bottom;
		//// Если табы показываются всегда - сразу добавим их размер, чтобы размер консоли был заказанным
		//nWidth  = conSize.X * gpSetCls->FontWidth() + nShiftX
		//	+ (this->isTabsShown() ? (gpSet->rcTabMargins.left+gpSet->rcTabMargins.right) : 0);
		//nHeight = conSize.Y * gpSetCls->FontHeight() + nShiftY
		//	+ (this->isTabsShown() ? (gpSet->rcTabMargins.top+gpSet->rcTabMargins.bottom) : 0);
		//mrc_Ideal = MakeRect(gpSet->wndX, gpSet->wndY, gpSet->wndX+nWidth, gpSet->wndY+nHeight);
		RECT rcWnd = GetDefaultRect();
		UpdateIdealRect(rcWnd);
		nWidth = rcWnd.right - rcWnd.left;
		nHeight = rcWnd.bottom - rcWnd.top;
	}
	else
	{
		_ASSERTE(this->WndWidth.Value && this->WndHeight.Value);
	}

	HWND hParent = mp_Inside ? mp_Inside->mh_InsideParentWND : ghWndApp;

	if (gpSetCls->isAdvLogging)
	{
		wchar_t szCreate[128];
		_wsprintf(szCreate, SKIPLEN(countof(szCreate)) L"Current display logical DPI=%u", gpSetCls->QueryDpi());
		LogString(szCreate);
		_wsprintf(szCreate, SKIPLEN(countof(szCreate)) L"Creating main window.%s%s%s Parent=x%08X X=%i Y=%i W=%i H=%i style=x%08X exStyle=x%08X Mode=%s",
			gpSet->isQuakeStyle ? L" Quake" : L"",
			this->mp_Inside ? L" Inside" : L"",
			gpSet->isDesktopMode ? L" Desktop" : L"",
			(DWORD)hParent,
			this->wndX, this->wndY, nWidth, nHeight, style, styleEx,
			GetWindowModeName(gpSet->isQuakeStyle ? (ConEmuWindowMode)gpSet->_WindowMode : WindowMode));
		LogString(szCreate);
	}

	//if (this->WindowMode == wmMaximized) style |= WS_MAXIMIZE;
	//style |= WS_VISIBLE;
	// cRect.right - cRect.left - 4, cRect.bottom - cRect.top - 4; -- все равно это было не правильно
	WARNING("На ноуте вылезает за пределы рабочей области");
	ghWnd = CreateWindowEx(styleEx, gsClassNameParent, gpSetCls->GetCmd(), style,
	                       gpConEmu->wndX, gpConEmu->wndY, nWidth, nHeight, hParent, NULL, (HINSTANCE)g_hInstance, NULL);

	if (!ghWnd)
	{
		//if (!ghWnd DC)
		MBoxA(_T("Can't create main window!"));

		return FALSE;
	}

	//if (gpSet->isHideCaptionAlways)
	//	OnHideCaption();
#ifdef _DEBUG
	DWORD style2 = GetWindowLongPtr(ghWnd, GWL_STYLE);
	DWORD styleEx2 = GetWindowLongPtr(ghWnd, GWL_EXSTYLE);
#endif
	OnTransparent();
	//if (gpConEmu->WindowMode == wmFullScreen || gpConEmu->WindowMode == wmMaximized)
	//	gpConEmu->SetWindowMode(gpConEmu->WindowMode);
	OnGlobalSettingsChanged();
	return TRUE;
}

BOOL CConEmuMain::CreateWorkWindow()
{
	HWND  hParent = ghWnd;
	RECT  rcClient = CalcRect(CER_WORKSPACE);
	DWORD styleEx = GetWorkWindowStyleEx();
	DWORD style = GetWorkWindowStyle();

	_ASSERTE(hParent!=NULL);

	ghWndWork = CreateWindowEx(styleEx, gsClassNameWork, L"ConEmu Workspace", style,
	                       rcClient.left, rcClient.top, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top,
	                       hParent, NULL, (HINSTANCE)g_hInstance, NULL);
	if (!ghWndWork)
	{
		DisplayLastError(L"Failed to create workspace window");
		return FALSE;
	}

	return TRUE;
}

bool CConEmuMain::SetParent(HWND hNewParent)
{
	HWND hCurrent = GetParent(ghWnd);
	if (hCurrent != hNewParent)
	{
		//::SetParent(ghWnd, NULL);
		::SetParent(ghWnd, hNewParent);
	}
	HWND hSet = GetParent(ghWnd);
	bool lbSuccess = (hSet == hNewParent) || (hNewParent == NULL && hSet == GetDesktopWindow());
	return lbSuccess;
}

void CConEmuMain::FillConEmuMainFont(ConEmuMainFont* pFont)
{
	// Параметры основного шрифта ConEmu
	lstrcpy(pFont->sFontName, gpSetCls->FontFaceName());
	pFont->nFontHeight = gpSetCls->FontHeight();
	pFont->nFontWidth = gpSetCls->FontWidth();
	pFont->nFontCellWidth = gpSet->FontSizeX3 ? gpSet->FontSizeX3 : gpSetCls->FontWidth();
	pFont->nFontQuality = gpSetCls->FontQuality();
	pFont->nFontCharSet = gpSetCls->FontCharSet();
	pFont->Bold = gpSetCls->FontBold();
	pFont->Italic = gpSetCls->FontItalic();
	lstrcpy(pFont->sBorderFontName, gpSetCls->BorderFontFaceName());
	pFont->nBorderFontWidth = gpSetCls->BorderFontWidth();
}


BOOL CConEmuMain::CheckDosBoxExists()
{
	BOOL lbExists = FALSE;
	wchar_t szDosBoxPath[MAX_PATH+32];
	wcscpy_c(szDosBoxPath, ms_ConEmuBaseDir);
	wchar_t* pszName = szDosBoxPath+_tcslen(szDosBoxPath);

	wcscpy_add(pszName, szDosBoxPath, L"\\DosBox\\DosBox.exe");
	if (FileExists(szDosBoxPath))
	{
		wcscpy_add(pszName, szDosBoxPath, L"\\DosBox\\DosBox.conf");
		lbExists = FileExists(szDosBoxPath);
	}

	return lbExists;
}

void CConEmuMain::CheckPortableReg()
{
#ifdef USEPORTABLEREGISTRY
	BOOL lbExists = FALSE;
	wchar_t szPath[MAX_PATH*2], szName[MAX_PATH];
	
	wcscpy_c(szPath, ms_ConEmuBaseDir);
	wcscat_c(szPath, L"\\Portable\\Portable.reg");
	if (_tcslen(szPath) >= countof(ms_PortableReg))
	{
		MBoxAssert(_tcslen(szPath) < countof(ms_PortableReg))
		return; // превышение длины
	}
	if (FileExists(szPath))
	{
		wcscpy_c(ms_PortableReg, szPath);
		mb_PortableRegExist = TRUE;
	}
	else
	{
		// Раз "Portable.reg" отсутствует - то и hive создавать/проверять не нужно
		return;
	}
	
	wchar_t* pszSID = NULL;
	wcscpy_c(szPath, ms_ConEmuBaseDir);
	wcscat_c(szPath, L"\\Portable\\");
	wcscpy_c(szName, L"Portable.");
	LPCWSTR pszSIDPtr = szName+_tcslen(szName);
	if (GetLogonSID (NULL, &pszSID))
	{
		if (_tcslen(pszSID) > 128)
			pszSID[127] = 0;
		wcscat_c(szName, pszSID);
		LocalFree(pszSID); pszSID = NULL;
	}
	else
	{
		wcscat_c(szName, L"S-Unknown");
	}
	
	if (gOSVer.dwMajorVersion <= 5)
	{
		_wsprintf(ms_PortableMountKey, SKIPLEN(countof(ms_PortableMountKey))
			L"%s.%s.%u", VIRTUAL_REGISTRY_GUID, pszSIDPtr, GetCurrentProcessId());
		mh_PortableMountRoot = HKEY_USERS;
	}
	else
	{
		ms_PortableMountKey[0] = 0;
		mh_PortableMountRoot = NULL;
	}
	
	wcscat_c(szPath, szName);
	wcscpy_c(ms_PortableRegHiveOrg, szPath);

	BOOL lbNeedTemp = FALSE;
	if ((_tcslen(szPath)+_tcslen(szName)) >= countof(ms_PortableRegHive))
		lbNeedTemp = TRUE;
	BOOL lbHiveExist = FileExists(szPath);
	
	// С сетевых и removable дисков монтировать не будем
	if (szPath[0] == L'\\' && szPath[1] == L'\\')
	{
		lbNeedTemp = TRUE;
	}
	else
	{
		wchar_t szDrive[MAX_PATH] = {0}, *pszSlash = wcschr(szPath, L'\\');
		if (pszSlash == NULL)
		{
			_ASSERTE(pszSlash!=NULL);
			lbNeedTemp = TRUE;
		}
		else
		{
			_wcscpyn_c(szDrive, MAX_PATH-1, szPath, (pszSlash-szPath)+1);
			DWORD nType = GetDriveType(szDrive);
			if (!nType || (nType & (DRIVE_CDROM|DRIVE_REMOTE|DRIVE_REMOVABLE)))
				lbNeedTemp = TRUE;
		}
	}

	// Если к файлу нельзя получить доступ на запись - тоже temp
	if (!lbNeedTemp)
	{
		HANDLE hFile = CreateFile(szPath, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			lbNeedTemp = TRUE;
		}
		else
		{
			CloseHandle(hFile);
			if (lbHiveExist && !lbNeedTemp)
				wcscpy_c(ms_PortableRegHive, szPath);
		}
	}
	else
	{
		wcscpy_c(ms_PortableRegHive, szPath);
	}
	if (lbNeedTemp)
	{
		wchar_t szTemp[MAX_PATH], szTempFile[MAX_PATH+1];
		int nLen = _tcslen(szName)+16;
		if (!GetTempPath(MAX_PATH-nLen, szTemp))
		{
			mb_PortableRegExist = FALSE;
			DisplayLastError(L"GetTempPath failed");
			return;
		}
		if (!GetTempFileName(szTemp, L"CEH", 0, szTempFile))
		{
			mb_PortableRegExist = FALSE;
			DisplayLastError(L"GetTempFileName failed");
			return;
		}
		// System create temp file, we needs directory
		DeleteFile(szTempFile);
		if (!CreateDirectory(szTempFile, NULL))
		{
			mb_PortableRegExist = FALSE;
			DisplayLastError(L"Create temp hive directory failed");
			return;
		}
		wcscat_c(szTempFile, L"\\");
		wcscpy_c(ms_PortableTempDir, szTempFile);
		wcscat_c(szTempFile, szName);
		if (lbHiveExist)
		{
			if (!CopyFile(szPath, szTempFile, FALSE))
			{
				mb_PortableRegExist = FALSE;
				DisplayLastError(L"Copy temp hive file failed");
				return;
			}
		}
		else
		{
			HANDLE hFile = CreateFile(szTempFile, GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				mb_PortableRegExist = FALSE;
				DisplayLastError(L"Create temp hive file failed");
				return;
			}
			else
			{
				CloseHandle(hFile);
			}
		}
		
		wcscpy_c(ms_PortableRegHive, szTempFile);
	}
#endif
}

bool CConEmuMain::PreparePortableReg()
{
#ifdef USEPORTABLEREGISTRY
	if (!mb_PortableRegExist)
		return false;
	if (mh_PortableRoot)
		return true; // уже
	
	bool lbRc = false;
	wchar_t szFullInfo[512];
	//typedef int (WINAPI* MountVirtualHive_t)(LPCWSTR asHive, PHKEY phKey, LPCWSTR asXPMountName, wchar_t* pszErrInfo, int cchErrInfoMax);
	MountVirtualHive_t MountVirtualHive = NULL;
	HMODULE hDll = LoadConEmuCD();
	if (!hDll)
	{
		DWORD dwErrCode = GetLastError();
		_wsprintf(szFullInfo, SKIPLEN(countof(szFullInfo))
			L"Portable registry will be NOT available\n\nLoadLibrary('%s') failed, ErrCode=0x%08X",
			#ifdef _WIN64
				L"ConEmuCD64.dll",
			#else
				L"ConEmuCD.dll",
			#endif
			dwErrCode);
	}
	else
	{
		MountVirtualHive = (MountVirtualHive_t)GetProcAddress(hDll, "MountVirtualHive");
		if (!MountVirtualHive)
		{
			_wsprintf(szFullInfo, SKIPLEN(countof(szFullInfo))
				L"Portable registry will be NOT available\n\nMountVirtualHive not found in '%s'",
				#ifdef _WIN64
					L"ConEmuCD64.dll"
				#else
					L"ConEmuCD.dll"
				#endif
				);
		}
		else
		{
			szFullInfo[0] = 0;
			_ASSERTE(ms_PortableMountKey[0]!=0 || (gOSVer.dwMajorVersion>=6));
			wchar_t szErrInfo[255]; szErrInfo[0] = 0;
			int lRc = MountVirtualHive(ms_PortableRegHive, &mh_PortableRoot,
					ms_PortableMountKey, szErrInfo, countof(szErrInfo), &mb_PortableKeyMounted);
			if (lRc != 0)
			{
				_wsprintf(szFullInfo, SKIPLEN(countof(szFullInfo))
					L"Portable registry will be NOT available\n\n%s\nMountVirtualHive result=%i",
					szErrInfo, lRc);
				ms_PortableMountKey[0] = 0;
			}
			else
			{
				lbRc = true;
			}
		}
	}
	
	if (!lbRc)
	{
		mb_PortableRegExist = FALSE;
	}

	// Обновить мэппинг
	OnGlobalSettingsChanged();
		
	if (!lbRc)
	{
		mb_PortableRegExist = FALSE; // сразу сбросим, раз не доступно

		wchar_t szFullMsg[1024];
		wcscpy_c(szFullMsg, szFullInfo);
		wcscat_c(szFullMsg, L"\n\nDo You want to continue without portable registry?");
		int nBtn = MessageBox(NULL, szFullMsg, GetDefaultTitle(),
			MB_ICONSTOP|MB_YESNO|MB_DEFBUTTON2);
		if (nBtn != IDYES)
			return false;
	}
	
	return true;
#else
	_ASSERTE(mb_PortableRegExist==FALSE);
	return true;
#endif
}

void CConEmuMain::FinalizePortableReg()
{
#ifdef USEPORTABLEREGISTRY
	TODO("Скопировать измененный куст - в папку Portable");
	// Portable.reg не трогаем - это как бы "шаблон" для куста

	// Сначала - закрыть хэндл, а то не получится демонтировать
	if (mh_PortableRoot)
	{
		RegCloseKey(mh_PortableRoot);
		mh_PortableRoot = NULL;
	}

	if (ms_PortableMountKey[0] && mb_PortableKeyMounted)
	{
		wchar_t szErrInfo[255];
		typedef int (WINAPI* UnMountVirtualHive_t)(LPCWSTR asXPMountName, wchar_t* pszErrInfo, int cchErrInfoMax);
		UnMountVirtualHive_t UnMountVirtualHive = NULL;
		HMODULE hDll = LoadConEmuCD();
		if (!hDll)
		{
			DWORD dwErrCode = GetLastError();
			_wsprintf(szErrInfo, SKIPLEN(countof(szErrInfo))
				L"Can't release portable key: 'HKU\\%s'\n\nLoadLibrary('%s') failed, ErrCode=0x%08X",
					ms_PortableMountKey,
				#ifdef _WIN64
					L"ConEmuCD64.dll",
				#else
					L"ConEmuCD.dll",
				#endif
				dwErrCode);
			MBoxA(szErrInfo);
		}
		else
		{
			UnMountVirtualHive = (UnMountVirtualHive_t)GetProcAddress(hDll, "UnMountVirtualHive");
			if (!UnMountVirtualHive)
			{
				_wsprintf(szErrInfo, SKIPLEN(countof(szErrInfo))
					L"Can't release portable key: 'HKU\\%s'\n\nUnMountVirtualHive not found in '%s'",
						ms_PortableMountKey,
					#ifdef _WIN64
						L"ConEmuCD64.dll"
					#else
						L"ConEmuCD.dll"
					#endif
					);
				MessageBox(NULL, szErrInfo, GetDefaultTitle(), MB_ICONSTOP|MB_SYSTEMMODAL);
			}
			else
			{
				szErrInfo[0] = 0;
				int lRc = UnMountVirtualHive(ms_PortableMountKey, szErrInfo, countof(szErrInfo));
				if (lRc != 0)
				{
					wchar_t szFullInfo[512];
					_wsprintf(szFullInfo, SKIPLEN(countof(szFullInfo))
						L"Can't release portable key: 'HKU\\%s'\n\n%s\nRc=%i",
						ms_PortableMountKey, szErrInfo, lRc);
					ms_PortableMountKey[0] = 0; // не пытаться повторно
					MessageBox(NULL, szFullInfo, GetDefaultTitle(), MB_ICONSTOP|MB_SYSTEMMODAL);
				}
			}
		}
	}
	ms_PortableMountKey[0] = 0; mb_PortableKeyMounted = FALSE; // или успешно, или не пытаться повторно

	if (ms_PortableTempDir[0])
	{
		wchar_t szTemp[MAX_PATH*2], *pszSlash;
		wcscpy_c(szTemp, ms_PortableTempDir);
		pszSlash = szTemp + _tcslen(szTemp) - 1;
		_ASSERTE(*pszSlash == L'\\');
		_wcscpy_c(pszSlash+1, MAX_PATH, _T("*.*"));
		WIN32_FIND_DATA fnd;
		HANDLE hFind = FindFirstFile(szTemp, &fnd);
		if (hFind && hFind != INVALID_HANDLE_VALUE)
		{
			do {
				if (!(fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					_wcscpy_c(pszSlash+1, MAX_PATH, fnd.cFileName);
					DeleteFile(szTemp);
				}
			} while (FindNextFile(hFind, &fnd));
			
			FindClose(hFind);
		}
		*pszSlash = 0;
		RemoveDirectory(szTemp);
		ms_PortableTempDir[0] = 0; // один раз
	}
#endif
}

void CConEmuMain::GetComSpecCopy(ConEmuComspec& ComSpec)
{
	_ASSERTE(m_GuiInfo.ComSpec.csType==gpSet->ComSpec.csType);
	_ASSERTE(m_GuiInfo.ComSpec.csBits==(m_DbgInfo.bBlockChildrenDebuggers ? csb_SameApp : gpSet->ComSpec.csBits));
	ComSpec = m_GuiInfo.ComSpec;
}

// Перед аттачем GUI приложения нужно создать мэппинг,
// чтобы оно могло легко узнать, куда нужно подцепляться
void CConEmuMain::CreateGuiAttachMapping(DWORD nGuiAppPID)
{
	m_GuiAttachMapping.CloseMap();
	m_GuiAttachMapping.InitName(CEGUIINFOMAPNAME, nGuiAppPID);

	if (m_GuiAttachMapping.Create())
	{
		m_GuiAttachMapping.SetFrom(&m_GuiInfo);
	}
}

// Установить пути в структуре
void CConEmuMain::InitComSpecStr(ConEmuComspec& ComSpec)
{
	wcscpy_c(ComSpec.ConEmuExeDir, ms_ConEmuExeDir);
	wcscpy_c(ComSpec.ConEmuBaseDir, ms_ConEmuBaseDir);
}

void CConEmuMain::UpdateGuiInfoMapping()
{
	m_GuiInfo.nProtocolVersion = CESERVER_REQ_VER;

	static DWORD ChangeNum = 0; ChangeNum++; if (!ChangeNum) ChangeNum = 1;
	m_GuiInfo.nChangeNum = ChangeNum;

	m_GuiInfo.hGuiWnd = ghWnd;
	m_GuiInfo.nGuiPID = GetCurrentProcessId();
	
	m_GuiInfo.nLoggingType = (ghOpWnd && gpSetCls->mh_Tabs[gpSetCls->thi_Debug]) ? gpSetCls->m_ActivityLoggingType : glt_None;
	m_GuiInfo.bUseInjects = (gpSet->isUseInjects ? 1 : 0) ; // ((gpSet->isUseInjects == BST_CHECKED) ? 1 : (gpSet->isUseInjects == BST_INDETERMINATE) ? 3 : 0);
	SetConEmuFlags(m_GuiInfo.Flags,CECF_UseTrueColor,(gpSet->isTrueColorer ? CECF_UseTrueColor : 0));
	SetConEmuFlags(m_GuiInfo.Flags,CECF_ProcessAnsi,(gpSet->isProcessAnsi ? CECF_ProcessAnsi : 0));
	SetConEmuFlags(m_GuiInfo.Flags,CECF_SuppressBells,(gpSet->isSuppressBells ? CECF_SuppressBells : 0));
	SetConEmuFlags(m_GuiInfo.Flags,CECF_ConExcHandler,(gpSet->isConsoleExceptionHandler ? CECF_ConExcHandler : 0));
	SetConEmuFlags(m_GuiInfo.Flags,CECF_ProcessNewCon,(gpSet->isProcessNewConArg ? CECF_ProcessNewCon : 0));
	// использовать расширение командной строки (ReadConsole). 0 - нет, 1 - старая версия (0.1.1), 2 - новая версия
	switch (gpSet->isUseClink())
	{
	case 1:
		SetConEmuFlags(m_GuiInfo.Flags,CECF_UseClink_Any,CECF_UseClink_1);
		break;
	case 2:
		SetConEmuFlags(m_GuiInfo.Flags,CECF_UseClink_Any,CECF_UseClink_2);
		break;
	default:
		_ASSERTE(gpSet->isUseClink()==0);
		SetConEmuFlags(m_GuiInfo.Flags,CECF_UseClink_Any,CECF_Empty);
	}

	SetConEmuFlags(m_GuiInfo.Flags,CECF_BlockChildDbg,(m_DbgInfo.bBlockChildrenDebuggers ? CECF_BlockChildDbg : 0));
	
	SetConEmuFlags(m_GuiInfo.Flags,CECF_SleepInBackg,(gpSet->isSleepInBackground ? CECF_SleepInBackg : 0));
	m_GuiInfo.bGuiActive = isMeForeground(true, true);
	{
	CVConGuard VCon;
	m_GuiInfo.hActiveCon = (GetActiveVCon(&VCon) >= 0) ? VCon->RCon()->ConWnd() : NULL;
	}
	m_GuiInfo.dwActiveTick = GetTickCount();

	mb_DosBoxExists = CheckDosBoxExists();
	SetConEmuFlags(m_GuiInfo.Flags,CECF_DosBox,(mb_DosBoxExists ? CECF_DosBox : 0));
	
	m_GuiInfo.isHookRegistry = (mb_PortableRegExist ? (gpSet->isPortableReg ? 3 : 1) : 0);
	wcscpy_c(m_GuiInfo.sHiveFileName, ms_PortableRegHive);
	m_GuiInfo.hMountRoot = mh_PortableMountRoot;
	wcscpy_c(m_GuiInfo.sMountKey, ms_PortableMountKey);
	
	wcscpy_c(m_GuiInfo.sConEmuExe, ms_ConEmuExe);
	//-- переехали в m_GuiInfo.ComSpec
	//wcscpy_c(m_GuiInfo.sConEmuDir, ms_ConEmuExeDir);
	//wcscpy_c(m_GuiInfo.sConEmuBaseDir, ms_ConEmuBaseDir);
	_wcscpyn_c(m_GuiInfo.sConEmuArgs, countof(m_GuiInfo.sConEmuArgs), mpsz_ConEmuArgs ? mpsz_ConEmuArgs : L"", countof(m_GuiInfo.sConEmuArgs));

	/* Default terminal begin */
	m_GuiInfo.bUseDefaultTerminal = gpSet->isSetDefaultTerminal;
	wchar_t szOpt[16] = {}; wchar_t* pszOpt = szOpt;
	LPCWSTR pszConfig = gpSetCls->GetConfigName();
	switch (gpSet->nDefaultTerminalConfirmClose)
	{
		case 0: break; // auto
		case 1: *(pszOpt++) = L'c'; break; // always
		case 2: *(pszOpt++) = L'n'; break; // never
	}
	if (gpSet->isDefaultTerminalNoInjects)
		*(pszOpt++) = L'i';
	if (gpSet->isDefaultTerminalNewWindow)
		*(pszOpt++) = L'N';
	_ASSERTE(pszOpt < (szOpt+countof(szOpt)));
	// Preparing arguments
	m_GuiInfo.sDefaultTermArg[0] = 0;
	if (pszConfig && *pszConfig)
	{
		wcscat_c(m_GuiInfo.sDefaultTermArg, L"/config \"");
		wcscat_c(m_GuiInfo.sDefaultTermArg, pszConfig);
		wcscat_c(m_GuiInfo.sDefaultTermArg, L"\" ");
	}
	if (*szOpt)
	{
		wcscat_c(m_GuiInfo.sDefaultTermArg, L"-new_console:");
		wcscat_c(m_GuiInfo.sDefaultTermArg, szOpt);
	}
	/* Default terminal end */

	// *********************
	// *** ComSpec begin ***
	// *********************
	{
		// Сначала - обработать что что поменяли (найти tcc и т.п.)
		SetEnvVarExpanded(L"ComSpec", ms_ComSpecInitial); // т.к. функции могут ориентироваться на окружение
		FindComspec(&gpSet->ComSpec);
		//wcscpy_c(gpSet->ComSpec.ConEmuDir, gpConEmu->ms_ConEmuDir);
		InitComSpecStr(gpSet->ComSpec);
		// установит переменную окружения ComSpec, если просили (isAddConEmu2Path)
		UpdateComspec(&gpSet->ComSpec, true/*но не трогать %PATH%*/);

		// Скопируем всю структуру сначала, потом будет "править" то что нужно
		m_GuiInfo.ComSpec = gpSet->ComSpec;

		// Теперь перенести в мэппинг, для информирования других процессов
		ComSpecType csType = gpSet->ComSpec.csType;
		// Если мы в режиме "отладки дерева процессов" - предпочитать ComSpec битности приложения!
		ComSpecBits csBits = m_DbgInfo.bBlockChildrenDebuggers ? csb_SameApp : gpSet->ComSpec.csBits;
		//m_GuiInfo.ComSpec.csType = gpSet->ComSpec.csType;
		//m_GuiInfo.ComSpec.csBits = gpSet->ComSpec.csBits;
		wchar_t szExpand[MAX_PATH] = {}, szFull[MAX_PATH] = {}, *pszFile;
		if (csType == cst_Explicit)
		{
			DWORD nLen;
			// Expand env.vars
			if (wcschr(gpSet->ComSpec.ComspecExplicit, L'%'))
			{
				nLen = ExpandEnvironmentStrings(gpSet->ComSpec.ComspecExplicit, szExpand, countof(szExpand));
				if (!nLen || (nLen>=countof(szExpand)))
				{
					MBoxAssert((nLen>0) && (nLen<countof(szExpand)) && "ExpandEnvironmentStrings(ComSpec)");
					szExpand[0] = 0;
				}
			}
			else
			{
				wcscpy_c(szExpand, gpSet->ComSpec.ComspecExplicit);
				nLen = lstrlen(szExpand);
			}
			// Expand relative paths. Note. Path MUST be specified with root
			// NOT recommended: "..\tcc\tcc.exe" this may lead to unpredictable results cause of CurrentDirectory
			// Allowed: "%ConEmuDir%\..\tcc\tcc.exe"
			if (*szExpand)
			{
				nLen = GetFullPathName(szExpand, countof(szFull), szFull, &pszFile);
				if (!nLen || (nLen>=countof(szExpand)))
				{
					MBoxAssert((nLen>0) && (nLen<countof(szExpand)) && "GetFullPathName(ComSpec)");
					szFull[0] = 0;
				}
				else if (!FileExists(szExpand))
				{
					szFull[0] = 0;
				}
			}
			else
			{
				szFull[0] = 0;
			}
			wcscpy_c(m_GuiInfo.ComSpec.ComspecExplicit, szFull);
			if (!*szFull && (csType == cst_Explicit))
			{
				csType = cst_AutoTccCmd;
			}
		}
		else
		{
			m_GuiInfo.ComSpec.ComspecExplicit[0] = 0; // избавимся от возможного мусора
		}

		//
		if (csType == cst_Explicit)
		{
			_ASSERTE(*szFull);
			wcscpy_c(m_GuiInfo.ComSpec.Comspec32, szFull);
			wcscpy_c(m_GuiInfo.ComSpec.Comspec64, szFull);
		}
		else
		{
			wcscpy_c(m_GuiInfo.ComSpec.Comspec32, gpSet->ComSpec.Comspec32);
			wcscpy_c(m_GuiInfo.ComSpec.Comspec64, gpSet->ComSpec.Comspec64);
		}
		//wcscpy_c(m_GuiInfo.ComSpec.ComspecInitial, gpConEmu->ms_ComSpecInitial);

		// finalization
		m_GuiInfo.ComSpec.csType = csType;
		m_GuiInfo.ComSpec.csBits = csBits;
		//m_GuiInfo.ComSpec.isUpdateEnv = gpSet->ComSpec.isUpdateEnv;
		//m_GuiInfo.ComSpec.isAddConEmu2Path = gpSet->ComSpec.isAddConEmu2Path;
	}
	// *******************
	// *** ComSpec end ***
	// *******************

	
	FillConEmuMainFont(&m_GuiInfo.MainFont);
	
	TODO("DosBox");
	//BOOL     bDosBox; // наличие DosBox
	//wchar_t  sDosBoxExe[MAX_PATH+1]; // полный путь к DosBox.exe
	//wchar_t  sDosBoxEnv[8192]; // команды загрузки (mount, и пр.)

	//if (mb_CreateProcessLogged)
	//	lstrcpy(m_GuiInfo.sLogCreateProcess, ms_LogCreateProcess);
	// sConEmuArgs уже заполнен в PrepareCommandLine
	
	BOOL lbMapIsValid = m_GuiInfoMapping.IsValid();
	
	if (!lbMapIsValid)
	{
		m_GuiInfoMapping.InitName(CEGUIINFOMAPNAME, GetCurrentProcessId());

		if (m_GuiInfoMapping.Create())
			lbMapIsValid = TRUE;
	}
	
	if (lbMapIsValid)
	{
		m_GuiInfoMapping.SetFrom(&m_GuiInfo);
	}
	#ifdef _DEBUG
	else
	{
		_ASSERTE(lbMapIsValid);
	}
	#endif
	
}

void CConEmuMain::UpdateGuiInfoMappingActive(bool bActive)
{
	CVConGuard VCon;
	HWND hActiveRCon = (GetActiveVCon(&VCon) >= 0) ? VCon->RCon()->ConWnd() : NULL;

	SetConEmuFlags(m_GuiInfo.Flags,CECF_SleepInBackg,(gpSet->isSleepInBackground ? CECF_SleepInBackg : 0));
	m_GuiInfo.bGuiActive = bActive;
	m_GuiInfo.hActiveCon = hActiveRCon;
	m_GuiInfo.dwActiveTick = GetTickCount();

	ConEmuGuiMapping* pData = m_GuiInfoMapping.Ptr();

	if (pData)
	{
		if ((((pData->Flags & CECF_SleepInBackg)!=0) != (gpSet->isSleepInBackground != FALSE))
			|| ((pData->bGuiActive!=FALSE) != (bActive!=FALSE))
			|| (pData->hActiveCon != hActiveRCon))
		{
			SetConEmuFlags(pData->Flags,CECF_SleepInBackg,(gpSet->isSleepInBackground ? CECF_SleepInBackg : 0));
			pData->bGuiActive = bActive;
			pData->hActiveCon = hActiveRCon;
			pData->dwActiveTick = m_GuiInfo.dwActiveTick;
		}
	}
}

HRGN CConEmuMain::CreateWindowRgn(bool abTestOnly/*=false*/)
{
	HRGN hRgn = NULL, hExclusion = NULL;

	TODO("DoubleView: Если видимы несколько консолей - нужно совместить регионы, или вообще отрубить, для простоты");

	hExclusion = CVConGroup::GetExclusionRgn(abTestOnly);

	if (abTestOnly && hExclusion)
	{
		_ASSERTE(hExclusion == (HRGN)1);
		return (HRGN)1;
	}

	WARNING("Установка любого НЕ NULL региона сбивает темы при отрисовке кнопок в заголовке");

	if (!gpSet->isQuakeStyle
		&& ((WindowMode == wmFullScreen)
			// Условие именно такое (для isZoomed) - здесь регион ставится на весь монитор
			|| ((WindowMode == wmMaximized) && (gpSet->isHideCaption || gpSet->isHideCaptionAlways()))))
	{
		if (abTestOnly)
			return (HRGN)1;

		ConEmuRect tFrom = (WindowMode == wmFullScreen) ? CER_FULLSCREEN : CER_MAXIMIZED;
		RECT rcScreen; // = CalcRect(tFrom, MakeRect(0,0), tFrom);
		RECT rcFrame = CalcMargins(CEM_FRAMECAPTION);

		MONITORINFO mi = {};
		HMONITOR hMon = GetNearestMonitor(&mi);
		
		if (hMon)
			rcScreen = (WindowMode == wmFullScreen) ? mi.rcMonitor : mi.rcWork;
		else
			rcScreen = CalcRect(tFrom, MakeRect(0,0), tFrom);

		// rcFrame, т.к. регион ставится относительно верхнего левого угла ОКНА
		hRgn = CreateWindowRgn(abTestOnly, false, rcFrame.left, rcFrame.top, rcScreen.right-rcScreen.left, rcScreen.bottom-rcScreen.top);
	}
	else if (!gpSet->isQuakeStyle
		&& (WindowMode == wmMaximized))
	{
		if (!hExclusion)
		{
			// Если прозрачных участков в консоли нет - ничего не делаем
		}
		else
		{
			// с FullScreen не совпадает. Нужно с учетом заголовка
			// сюда мы попадаем только когда заголовок НЕ скрывается
			RECT rcScreen = CalcRect(CER_FULLSCREEN, MakeRect(0,0), CER_FULLSCREEN);
			int nCX = GetSystemMetrics(SM_CXSIZEFRAME);
			int nCY = GetSystemMetrics(SM_CYSIZEFRAME);
			hRgn = CreateWindowRgn(abTestOnly, false, nCX, nCY, rcScreen.right-rcScreen.left, rcScreen.bottom-rcScreen.top);
		}
	}
	else
	{
		// Normal
		if (gpSet->isCaptionHidden())
		{
			if ((mn_QuakePercent != 0)
				|| !isMouseOverFrame()
				|| (gpSet->isQuakeStyle && (gpSet->HideCaptionAlwaysFrame() != 0)))
			{
				// Рамка невидима (мышка не над рамкой или заголовком)
				RECT rcClient = GetGuiClientRect();
				RECT rcFrame = CalcMargins(CEM_FRAMECAPTION);
				//_ASSERTE(!rcClient.left && !rcClient.top);

				bool bRoundTitle = (gOSVer.dwMajorVersion == 5) && gpSetCls->CheckTheming() && mp_TabBar->IsTabsShown();

				if (gpSet->isQuakeStyle)
				{
					if (mn_QuakePercent > 0)
					{
						int nPercent = (mn_QuakePercent > 100) ? 100 : (mn_QuakePercent == 1) ? 0 : mn_QuakePercent;
						int nQuakeHeight = (rcClient.bottom - rcClient.top + 1) * nPercent / 100;
						if (nQuakeHeight < 1)
						{
							nQuakeHeight = 1; // иначе регион не применится
							rcClient.right = rcClient.left + 1;
						}
						rcClient.bottom = min(rcClient.bottom, (rcClient.top+nQuakeHeight));
						bRoundTitle = false;
					}
					else
					{
						// Видимо все, но нужно "отрезать" верхнюю часть рамки
					}
				}

				int nFrame = gpSet->HideCaptionAlwaysFrame();
				bool bFullFrame = (nFrame < 0);
				if (bFullFrame)
				{
					nFrame = 0;
				}
				else
				{
					_ASSERTE(rcFrame.left>=nFrame);
					_ASSERTE(rcFrame.top>=nFrame);
				}


				// We need coordinates relative to upper-top corner of WINDOW (not client area)
				int rgnX = bFullFrame ? 0 : (rcFrame.left - nFrame);
				int rgnY = bFullFrame ? 0 : (rcFrame.top - nFrame);
				int rgnX2 = (rcFrame.left - rgnX) + (bFullFrame ?  rcFrame.right : nFrame);
				int rgnY2 = (rcFrame.top - rgnY) + (bFullFrame ? rcFrame.bottom : nFrame);
				if (gpSet->isQuakeStyle)
				{
					if (gpSet->_WindowMode != wmNormal)
					{
						// ConEmu window is maximized to fullscreen or work area
						// Need to cut left/top/bottom frame edges
						rgnX = rcFrame.left;
						rgnX2 = rcFrame.left - rcFrame.right;
					}
					// top - cut always
					rgnY = rcFrame.top;
				}
				int rgnWidth = rcClient.right + rgnX2;
				int rgnHeight = rcClient.bottom + rgnY2;

				bool bCreateRgn = (nFrame >= 0);


				if (gpSet->isQuakeStyle)
				{
					if (mn_QuakePercent)
					{
						if (mn_QuakePercent == 1)
						{
							rgnWidth = rgnHeight = 1;
						}
						else if (nFrame < 0)
						{
							nFrame = GetSystemMetrics(SM_CXSIZEFRAME);
						}
						bCreateRgn = true;
					}
					bRoundTitle = false;
				}


				if (bCreateRgn)
				{
					hRgn = CreateWindowRgn(abTestOnly, bRoundTitle, rgnX, rgnY, rgnWidth, rgnHeight);
				}
				else
				{
					_ASSERTE(hRgn == NULL);
					hRgn = NULL;
				}
			}
		}

		if (!hRgn && hExclusion)
		{
			// Таки нужно создать...
			bool bTheming = gpSetCls->CheckTheming();
			RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);
			hRgn = CreateWindowRgn(abTestOnly, bTheming,
			                       0,0, rcWnd.right-rcWnd.left, rcWnd.bottom-rcWnd.top);
		}
	}

	return hRgn;
}

HRGN CConEmuMain::CreateWindowRgn(bool abTestOnly/*=false*/,bool abRoundTitle/*=false*/,int anX, int anY, int anWndWidth, int anWndHeight)
{
	HRGN hRgn = NULL, hExclusion = NULL, hCombine = NULL;

	TODO("DoubleView: Если видимы несколько консолей - нужно совместить регионы, или вообще отрубить, для простоты");

	CVConGuard VCon;

	// Консоли может реально не быть на старте
	// или если GUI не закрывается при закрытии последнего таба
	if (CVConGroup::GetActiveVCon(&VCon) >= 0)
	{
		hExclusion = CVConGroup::GetExclusionRgn(abTestOnly);
	}
	

	if (abTestOnly && hExclusion)
	{
		_ASSERTE(hExclusion == (HRGN)1);
		return (HRGN)1;
	}

	if (abRoundTitle && anX>0 && anY>0)
	{
		int nPoint = 0;
		POINT ptPoly[20];
		ptPoly[nPoint++] = MakePoint(anX, anY+anWndHeight);
		ptPoly[nPoint++] = MakePoint(anX, anY+5);
		ptPoly[nPoint++] = MakePoint(anX+1, anY+3);
		ptPoly[nPoint++] = MakePoint(anX+3, anY+1);
		ptPoly[nPoint++] = MakePoint(anX+5, anY);
		ptPoly[nPoint++] = MakePoint(anX+anWndWidth-5, anY);
		ptPoly[nPoint++] = MakePoint(anX+anWndWidth-3, anY+1);
		ptPoly[nPoint++] = MakePoint(anX+anWndWidth-1, anY+4); //-V112
		ptPoly[nPoint++] = MakePoint(anX+anWndWidth, anY+6);
		ptPoly[nPoint++] = MakePoint(anX+anWndWidth, anY+anWndHeight);
		hRgn = CreatePolygonRgn(ptPoly, nPoint, WINDING);
	}
	else
	{
		hRgn = CreateRectRgn(anX, anY, anX+anWndWidth, anY+anWndHeight);
	}

	if (abTestOnly && (hRgn || hExclusion))
		return (HRGN)1;

	// Смотреть CombineRgn, OffsetRgn (для перемещения региона отрисовки в пространство окна)
	if (hExclusion)
	{
		if (hRgn)
		{
			//_ASSERTE(hRgn != NULL);
			//DeleteObject(hExclusion);
			//} else {
			//POINT ptShift = {0,0};
			//MapWindowPoints(ghWnd DC, NULL, &ptShift, 1);
			//RECT rcWnd = GetWindow
			RECT rcFrame = CalcMargins(CEM_FRAMECAPTION);

			#ifdef _DEBUG
			// CEM_TAB не учитывает центрирование клиентской части в развернутых режимах
			RECT rcTab = CalcMargins(CEM_TAB);
			#endif

			POINT ptClient = {0,0};
			TODO("Будет глючить на SplitScreen?");
			MapWindowPoints(VCon->GetView(), ghWnd, &ptClient, 1);

			HRGN hOffset = CreateRectRgn(0,0,0,0);
			int n1 = CombineRgn(hOffset, hExclusion, NULL, RGN_COPY); UNREFERENCED_PARAMETER(n1);
			int n2 = OffsetRgn(hOffset, rcFrame.left+ptClient.x, rcFrame.top+ptClient.y); UNREFERENCED_PARAMETER(n2);
			hCombine = CreateRectRgn(0,0,1,1);
			CombineRgn(hCombine, hRgn, hOffset, RGN_DIFF);
			DeleteObject(hRgn);
			DeleteObject(hOffset); hOffset = NULL;
			hRgn = hCombine; hCombine = NULL;
		}
	}

	return hRgn;
}

void CConEmuMain::Destroy()
{
	LogString(L"CConEmuMain::Destroy()");
	CVConGroup::StopSignalAll();

	if (gbInDisplayLastError)
	{
		LogString(L"-- Destroy skipped due to gbInDisplayLastError");
		return; // Чтобы не схлопывались окна с сообщениями об ошибках
	}

	#ifdef _DEBUG
	if (gbInMyAssertTrap)
	{
		LogString(L"-- Destroy skipped due to gbInMyAssertTrap");
		mb_DestroySkippedInAssert = true;
		return;
	}
	#endif

	if (ghWnd)
	{
		//HWND hWnd = ghWnd;
		//ghWnd = NULL;
		//DestroyWindow(hWnd); -- может быть вызывано из другой нити
		PostMessage(ghWnd, mn_MsgMyDestroy, GetCurrentThreadId(), 0);
	}
	else
	{
		LogString(L"-- Destroy skipped due to ghWnd");		
	}
}

CConEmuMain::~CConEmuMain()
{
	_ASSERTE(ghWnd==NULL || !IsWindow(ghWnd));
	MCHKHEAP;
	//ghWnd = NULL;

	SafeDelete(mp_DefTrm);

	bool bNeedConHostSearch = (gnOsVer == 0x0601);
	if (bNeedConHostSearch)
		DeleteCriticalSection(&m_LockConhostStart.cs);

	SafeDelete(mp_AttachDlg);
	SafeDelete(mp_RecreateDlg);

	CVConGroup::DestroyAllVCon();

	CVirtualConsole::ClearPartBrushes();

	#ifndef _WIN64
	if (mh_WinHook)
	{
		UnhookWinEvent(mh_WinHook);
		mh_WinHook = NULL;
	}
	#endif

	//if (mh_PopupHook) {
	//	UnhookWinEvent(mh_PopupHook);
	//	mh_PopupHook = NULL;
	//}

	SafeDelete(mp_DragDrop);

	//if (ProgressBars) {
	//    delete ProgressBars;
	//    ProgressBars = NULL;
	//}

	SafeDelete(mp_TabBar);

	SafeDelete(mp_Tip);

	SafeDelete(mp_Status);

	SafeDelete(mp_Menu);

	SafeDelete(mp_Inside);

	SafeDelete(mp_RunQueue);

	//if (m_Child)
	//{
	//	delete m_Child;
	//	m_Child = NULL;
	//}

	//if (m_Back)
	//{
	//	delete m_Back;
	//	m_Back = NULL;
	//}

	//if (m_Macro)
	//{
	//	delete m_Macro;
	//	m_Macro = NULL;
	//}

	//if (mh_RecreatePasswFont)
	//{
	//	DeleteObject(mh_RecreatePasswFont);
	//	mh_RecreatePasswFont = NULL;
	//}

	m_GuiServer.Stop(true);

	CVConGroup::Deinitialize();

	//DeleteCriticalSection(&mcs_ShellExecuteEx);
	
	// По идее, уже должен был быть вызван в OnDestroy
	FinalizePortableReg();

	LoadImageFinalize();

	SafeDelete(mp_Log);
	DeleteCriticalSection(&mcs_Log);

	_ASSERTE(mh_LLKeyHookDll==NULL);
	CommonShutdown();

	gpConEmu = NULL;
}




/* ****************************************************** */
/*                                                        */
/*                  Sizing methods                        */
/*                                                        */
/* ****************************************************** */

#ifdef colorer_func
{
	Sizing_Methods() {};
}
#endif

/*!!!static!!*/
void CConEmuMain::AddMargins(RECT& rc, const RECT& rcAddShift, BOOL abExpand/*=FALSE*/)
{
	if (!abExpand)
	{
		// подвинуть все грани на rcAddShift (left сдвигается на left, и т.п.)
		if (rcAddShift.left)
			rc.left += rcAddShift.left;

		if (rcAddShift.top)
			rc.top += rcAddShift.top;

		if (rcAddShift.right)
			rc.right -= rcAddShift.right;

		if (rcAddShift.bottom)
			rc.bottom -= rcAddShift.bottom;
	}
	else
	{
		// поправить только правую и нижнюю грань
		if (rcAddShift.right || rcAddShift.left)
			rc.right += rcAddShift.right + rcAddShift.left;

		if (rcAddShift.bottom || rcAddShift.top)
			rc.bottom += rcAddShift.bottom + rcAddShift.top;
	}
}

void CConEmuMain::AskChangeBufferHeight()
{
	CVConGuard VCon;
	if (GetActiveVCon(&VCon) < 0)
		return;
	CVirtualConsole *pVCon = VCon.VCon();
	CRealConsole *pRCon = pVCon->RCon();
	if (!pRCon) return;


	HWND hGuiClient = pRCon->GuiWnd();
	if (hGuiClient)
	{
		pRCon->ShowGuiClientInt(pRCon->isGuiForceConView());
		return;
	}


	// Win7 BUGBUG: Issue 192: падение Conhost при turn bufferheight ON
	// http://code.google.com/p/conemu-maximus5/issues/detail?id=192
	if (gOSVer.dwMajorVersion == 6 && gOSVer.dwMinorVersion == 1)
		return;


	BOOL lbBufferHeight = pRCon->isBufferHeight();

	int nBtn = 0;

	{
		DontEnable de;
		nBtn = MessageBox(ghWnd, lbBufferHeight ?
						  L"Do You want to turn bufferheight OFF?" :
						  L"Do You want to turn bufferheight ON?",
						  GetDefaultTitle(), MB_ICONQUESTION|MB_OKCANCEL);
	}

	if (nBtn != IDOK) return;

	#ifdef _DEBUG
	HANDLE hFarInExecuteEvent = NULL;

	if (!lbBufferHeight)
	{
		DWORD dwFarPID = pRCon->GetFarPID();

		if (dwFarPID)
		{
			// Это событие дергается в отладочной (мной поправленной) версии фара
			wchar_t szEvtName[64]; _wsprintf(szEvtName, SKIPLEN(countof(szEvtName)) L"FARconEXEC:%08X", (DWORD)pRCon->ConWnd());
			hFarInExecuteEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, szEvtName);

			if (hFarInExecuteEvent)  // Иначе ConEmuC вызовет _ASSERTE в отладочном режиме
				SetEvent(hFarInExecuteEvent);
		}
	}
	#endif

	
	pRCon->ChangeBufferHeightMode(!lbBufferHeight);


	#ifdef _DEBUG
	if (hFarInExecuteEvent)
		ResetEvent(hFarInExecuteEvent);
	#endif

	// Обновить на тулбаре статусы Scrolling(BufferHeight) & Alternative
	pRCon->OnBufferHeight();
}

void CConEmuMain::AskChangeAlternative()
{
	CVConGuard VCon;
	if (GetActiveVCon(&VCon) < 0)
		return;
	CVirtualConsole *pVCon = VCon.VCon();
	CRealConsole *pRCon = pVCon->RCon();
	if (!pRCon) return;


	HWND hGuiClient = pRCon->GuiWnd();
	if (hGuiClient)
	{
		pRCon->ShowGuiClientInt(!pRCon->isGuiVisible());
		return;
	}


	// Переключиться на альтернативный/основной буфер
	RealBufferType CurBuffer = pRCon->GetActiveBufferType();
	if (CurBuffer == rbt_Primary)
	{
		pRCon->SetActiveBuffer(rbt_Alternative);
	}
	else
	{
		pRCon->SetActiveBuffer(rbt_Primary);
	}

	// Обновить на тулбаре статусы Scrolling(BufferHeight) & Alternative
	pRCon->OnBufferHeight();
}

/*!!!static!!*/
// Функция расчитывает смещения (относительные)
// mg содержит битмаск, например (CEM_FRAMECAPTION|CEM_TAB|CEM_CLIENT)
RECT CConEmuMain::CalcMargins(DWORD/*enum ConEmuMargins*/ mg, ConEmuWindowMode wmNewMode /*= wmCurrent*/)
{
	_ASSERTE(this!=NULL);

	// -- функция должна работать только с главным окном
	// -- VCon тут влиять не должны
	//if (!apVCon)
	//	apVCon = gpConEmu->mp_ VActive;

	WARNING("CEM_SCROLL вообще удалить?");
	WARNING("Проверить, чтобы DC нормально центрировалось после удаления CEM_BACK");

	RECT rc = {};

	DEBUGTEST(FrameDrawStyle fdt = DrawType());
	ConEmuWindowMode wm = (wmNewMode == wmCurrent) ? WindowMode : wmNewMode;

	//if ((wmNewMode == wmCurrent) || (wmNewMode == wmNotChanging))
	//{
	//	wmNewMode = gpConEmu->WindowMode;
	//}

	if ((mg & ((DWORD)CEM_CLIENTSHIFT)) && (mp_Inside == NULL))
	{
		_ASSERTE(mg == (DWORD)CEM_CLIENTSHIFT); // Can not be combined with other flags!

		#if defined(CONEMU_TABBAR_EX)
		if ((fdt >= fdt_Aero) && gpSet->isTabsInCaption)
		{
			rc.top = gpConEmu->GetCaptionDragHeight() + gpConEmu->GetFrameHeight();
		}
		#endif
	}

	// Разница между размером всего окна и клиентской области окна (рамка + заголовок)
	if ((mg & ((DWORD)CEM_FRAMECAPTION)) && (mp_Inside == NULL))
	{
		// Только CEM_CAPTION считать нельзя
		_ASSERTE((mg & ((DWORD)CEM_FRAMECAPTION)) != CEM_CAPTION);

		// т.к. это первая обработка - можно ставить rc простым приравниванием
		_ASSERTE(rc.left==0 && rc.top==0 && rc.right==0 && rc.bottom==0);
		DWORD dwStyle = GetWindowStyle();
		if (wmNewMode != wmCurrent)
			dwStyle = FixWindowStyle(dwStyle, wmNewMode);
		DWORD dwStyleEx = GetWindowStyleEx();
		//static DWORD dwLastStyle, dwLastStyleEx;
		//static RECT rcLastRect;
		//if (dwLastStyle == dwStyle && dwLastStyleEx == dwStyleEx)
		//{
		//	rc = rcLastRect; // чтобы не дергать лишние расчеты
		//}
		//else
		//{
		const int nTestWidth = 100, nTestHeight = 100;
		RECT rcTest = MakeRect(nTestWidth,nTestHeight);

		bool bHideCaption = gpSet->isCaptionHidden(wmNewMode);

		// AdjustWindowRectEx НЕ должно вызываться в FullScreen. Глючит. А рамки с заголовком нет, расширять нечего.
		if (wm == wmFullScreen)
		{
			// Для FullScreen полностью убираются рамки и заголовок
			_ASSERTE(rc.left==0 && rc.top==0 && rc.right==0 && rc.bottom==0);
		}
		//#if defined(CONEMU_TABBAR_EX)
		//else if ((fdt >= fdt_Aero) && (wm == wmMaximized) && gpSet->isTabsInCaption)
		//{
		//	
		//}
		//#endif
		else if (AdjustWindowRectEx(&rcTest, dwStyle, FALSE, dwStyleEx))
		{
			rc.left = -rcTest.left;
			rc.right = rcTest.right - nTestWidth;
			rc.bottom = rcTest.bottom - nTestHeight;
			if ((mg & ((DWORD)CEM_CAPTION)) && !bHideCaption)
			{
				#if defined(CONEMU_TABBAR_EX)
				if ((fdt >= fdt_Aero) && gpSet->isTabsInCaption)
				{
					//-- NO additional space!
					//if (wm != wmMaximized)
					//	rc.top = gpConEmu->GetCaptionDragHeight() + rc.bottom;
					//else
					rc.top = rc.bottom + gpConEmu->GetCaptionDragHeight();
				}
				else
				#endif
				{
					rc.top = -rcTest.top;
				}
			}
			else
			{
				rc.top = rc.bottom;
			}
			_ASSERTE(rc.top >= 0 && rc.left >= 0 && rc.right >= 0 && rc.bottom >= 0);
		}
		else
		{
			_ASSERTE(FALSE);
			rc.left = rc.right = GetSystemMetrics(SM_CXSIZEFRAME);
			rc.bottom = GetSystemMetrics(SM_CYSIZEFRAME);
			rc.top = rc.bottom; // рамка

			if ((mg & ((DWORD)CEM_CAPTION)) && !bHideCaption) // если есть заголовок - добавим и его
			{
				#if defined(CONEMU_TABBAR_EX)
				if ((fdt >= fdt_Aero) && gpSet->isTabsInCaption)
				{
					//-- NO additional space!
					//if (wm != wmMaximized)
					//	rc.top += gpConEmu->GetCaptionDragHeight();
				}
				else
				#endif
				{
					rc.top += GetSystemMetrics(SM_CYCAPTION);
				}
			}
		}
	}

	// Сколько занимают табы (по идее меняется только rc.top)
	if (mg & ((DWORD)CEM_TAB))
	{
		if (ghWnd)
		{
			bool lbTabActive = ((mg & CEM_TAB_MASK) == CEM_TAB) ? gpConEmu->mp_TabBar->IsTabsActive()
			                   : ((mg & ((DWORD)CEM_TABACTIVATE)) == ((DWORD)CEM_TABACTIVATE));

			// Главное окно уже создано, наличие таба определено
			if (lbTabActive)  //TODO: + IsAllowed()?
			{
				RECT rcTab = gpConEmu->mp_TabBar->GetMargins();
				// !!! AddMargins использовать нельзя! он расчитан на вычитание
				//AddMargins(rc, rcTab, FALSE);
				_ASSERTE(rcTab.top==0 || rcTab.bottom==0);
				rc.top += rcTab.top;
				rc.bottom += rcTab.bottom;
			}// else { -- раз таба нет - значит дополнительные отступы не нужны

			//    rc = MakeRect(0,0); // раз таба нет - значит дополнительные отступы не нужны
			//}
		}
		else
		{
			// Иначе нужно смотреть по настройкам
			if (gpSet->isTabs == 1)
			{
				//RECT rcTab = gpSet->rcTabMargins; // умолчательные отступы таба
				RECT rcTab = mp_TabBar->GetMargins();
				// Сразу оба - быть не должны. Либо сверху, либо снизу.
				_ASSERTE(rcTab.top==0 || rcTab.bottom==0);

				//if (!gpSet->isTabFrame)
				//{

				// От таба остается только заголовок (закладки)
				//rc.left=0; rc.right=0; rc.bottom=0;
				if (gpSet->nTabsLocation == 1)
				{
					rc.bottom += rcTab.top ? rcTab.top : rcTab.bottom;
				}
				else
				{
					rc.top += rcTab.top ? rcTab.top : rcTab.bottom;
				}

				//}
				//else
				//{
				//	_ASSERTE(FALSE && "For deletion!");
				//	// !!! AddMargins использовать нельзя! он расчитан на вычитание
				//	//AddMargins(rc, rcTab, FALSE);
				//	rc.top += rcTab.top;
				//	rc.bottom += rcTab.bottom;
				//}
			}// else { -- раз таба нет - значит дополнительные отступы не нужны

			//    rc = MakeRect(0,0); // раз таба нет - значит дополнительные отступы не нужны
			//}
		}

		//#if defined(CONEMU_TABBAR_EX)
		#if 0
		if ((fdt >= fdt_Aero) && gpSet->isTabsInCaption)
		{
			if (wm != wmMaximized)
				rc.top += gpConEmu->GetCaptionDragHeight();
		}
		#endif
	}

	//    //case CEM_BACK:  // Отступы от краев окна фона (с прокруткой) до окна с отрисовкой (DC)
	//    //{
	//    //    if (apVCon && apVCon->RCon()->isBufferHeight()) { //TODO: а показывается ли сейчас прокрутка?
	//    //        rc = MakeRect(0,0,GetSystemMetrics(SM_CXVSCROLL),0);
	//    //    } else {
	//    //        rc = MakeRect(0,0); // раз прокрутки нет - значит дополнительные отступы не нужны
	//    //    }
	//    //}   break;
	//default:
	//    _ASSERTE(mg==CEM_FRAME || mg==CEM_TAB);
	//}

	//if (mg & ((DWORD)CEM_CLIENT))
	//{
	//	TODO("Переделать на ручной расчет, необходимо для DoubleView. Должен зависеть от apVCon");
	//	RECT rcDC; GetWindowRect(ghWnd DC, &rcDC);
	//	RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);
	//	RECT rcFrameTab = CalcMargins(CEM_FRAMETAB);
	//	// ставим результат
	//	rc.left += (rcDC.left - rcWnd.left - rcFrameTab.left);
	//	rc.top += (rcDC.top - rcWnd.top - rcFrameTab.top);
	//	rc.right -= (rcDC.right - rcWnd.right - rcFrameTab.right);
	//	rc.bottom -= (rcDC.bottom - rcWnd.bottom - rcFrameTab.bottom);
	//}

	if ((mg & ((DWORD)CEM_PAD)) && gpSet->isTryToCenter)
	{
		rc.left += gpSet->nCenterConsolePad;
		rc.top += gpSet->nCenterConsolePad;
		rc.right += gpSet->nCenterConsolePad;
		rc.bottom += gpSet->nCenterConsolePad;
	}

	if ((mg & ((DWORD)CEM_SCROLL)) && (gpSet->isAlwaysShowScrollbar == 1))
	{
		rc.right += GetSystemMetrics(SM_CXVSCROLL);
	}

	if ((mg & ((DWORD)CEM_STATUS)) && gpSet->isStatusBarShow)
	{
		TODO("Завистмость от темы/шрифта");
		rc.bottom += gpSet->StatusBarHeight();
	}

	return rc;
}

RECT CConEmuMain::CalcRect(enum ConEmuRect tWhat, CVirtualConsole* pVCon/*=NULL*/)
{
	_ASSERTE(ghWnd!=NULL);
	RECT rcMain = {};
	WINDOWPLACEMENT wpl = {sizeof(wpl)};
	int nGetStyle = 0;

	bool bNeedCalc = (isIconic() || mp_Menu->GetRestoreFromMinimized() || !IsWindowVisible(ghWnd));
	if (!bNeedCalc)
	{
		if (InMinimizing())
		{
			//_ASSERTE(!InMinimizing() || InQuakeAnimation()); -- вызывается при обновлении статусной строки, ну его...
			bNeedCalc = true;
		}
	}

	if (bNeedCalc)
	{
		nGetStyle = 1;

		if (gpSet->isQuakeStyle)
		{
			rcMain = GetDefaultRect();
			nGetStyle = 2;
		}
		else
		{
			// Win 8 Bug? Offered screen coordinates, but return - desktop workspace coordinates.
			// Thats why only for zoomed modes (need to detect "monitor"), however, this may be buggy too?
			if ((WindowMode == wmMaximized) || (WindowMode == wmFullScreen))
			{
				if (mrc_StoredNormalRect.right > mrc_StoredNormalRect.left && mrc_StoredNormalRect.bottom > mrc_StoredNormalRect.top)
				{
					rcMain = mrc_StoredNormalRect;
					nGetStyle = 3;
				}
				else
				{
					GetWindowPlacement(ghWnd, &wpl);
					rcMain = wpl.rcNormalPosition;
					nGetStyle = 4;
				}
			}
			else
			{
				GetWindowRect(ghWnd, &rcMain);
				nGetStyle = 5;
			}

			if (rcMain.left <= -32000 && rcMain.top <= -32000)
			{
				// -- when we call "DefWindowProc(hWnd, WM_SYSCOMMAND, wParam, lParam)"
				// -- uxtheme lose wpl.rcNormalPosition at this point
				//_ASSERTE(FALSE && "wpl.rcNormalPosition contains 'iconic' size!");

				if (mrc_StoredNormalRect.right > mrc_StoredNormalRect.left && mrc_StoredNormalRect.bottom > mrc_StoredNormalRect.top)
				{
					rcMain = mrc_StoredNormalRect;
					nGetStyle = 6;
				}
				else
				{
					_ASSERTE(FALSE && "mrc_StoredNormalRect was not saved yet, using GetDefaultRect()");

					rcMain = GetDefaultRect();
					nGetStyle = 7;
				}
			}

			// Если окно было свернуто из Maximized/FullScreen состояния
			if ((WindowMode == wmMaximized) || (WindowMode == wmFullScreen))
			{
				ConEmuRect t = (WindowMode == wmMaximized) ? CER_MAXIMIZED : CER_FULLSCREEN;
				rcMain = CalcRect(t, rcMain, t);
				nGetStyle += 10;
			}
		}
	}
	else
	{
		GetWindowRect(ghWnd, &rcMain);
	}

	if (tWhat == CER_MAIN)
	{
		return rcMain;
	}

	return CalcRect(tWhat, rcMain, CER_MAIN, pVCon);
}

// Return true, when rect was changed
bool CConEmuMain::FixWindowRect(RECT& rcWnd, DWORD nBorders /* enum of ConEmuBorders */, bool bPopupDlg /*= false*/)
{
	RECT rcStore = rcWnd;
	RECT rcWork;

	// When we live inside parent window - size must be strict
	if (!bPopupDlg && mp_Inside)
	{
		rcWork = GetDefaultRect();
		bool bChanged = (memcmp(&rcWork, &rcWnd, sizeof(rcWnd)) != 0);
		rcWnd = rcWork;
		return bChanged;
	}

	ConEmuWindowMode wndMode = GetWindowMode();

	if (!bPopupDlg && (wndMode == wmMaximized || wndMode == wmFullScreen))
	{
		nBorders |= CEB_TOP|CEB_LEFT|CEB_RIGHT|CEB_BOTTOM;
	}


	RECT rcFrame = CalcMargins(CEM_FRAMEONLY);
	

	// We prefer monitor, nearest to upper-left part of window
	//--RECT rcFind = {rcStore.left, rcStore.right, rcStore.left+(rcStore.right-rcStore.left)/3, rcStore.top+(rcStore.bottom-rcStore.top)/3};
	RECT rcFind = {rcStore.left+(rcFrame.left+1)*2, rcStore.top+(rcFrame.top+1)*2, rcStore.left+(rcFrame.left+1)*3, rcStore.top+(rcFrame.top+1)*3};
	MONITORINFO mi; GetNearestMonitor(&mi, &rcFind);
	// Get work area (may be FullScreen)
	rcWork = (wndMode == wmFullScreen) ? mi.rcMonitor : mi.rcWork;


	RECT rcInter = rcWnd;
	BOOL bIn = IntersectRect(&rcInter, &rcWork, &rcStore);

	if (bIn && ((EqualRect(&rcInter, &rcStore)) || (nBorders & CEB_ALLOW_PARTIAL)))
	{
		// Nothing must be changed
		return false;
	}

	if (nBorders & CEB_ALLOW_PARTIAL)
	{
		RECT rcFind2 = {rcStore.right-rcFrame.left-1, rcStore.bottom-rcFrame.top-1, rcStore.right, rcStore.bottom};
		MONITORINFO mi2; GetNearestMonitor(&mi2, &rcFind2);
		RECT rcInter2, rcInter3;
		if (IntersectRect(&rcInter2, &mi2.rcMonitor, &rcStore) || IntersectRect(&rcInter3, &mi.rcMonitor, &rcStore))
		{
			// It's partially visible, OK
			return false;
		}
	}

	// Calculate border sizes (even though one of left/top/right/bottom requested)
	RECT rcMargins = (nBorders & CEB_ALL) ? rcFrame : MakeRect(0,0);
	if (!(nBorders & CEB_LEFT))
		rcMargins.left = 0;
	if (!(nBorders & CEB_TOP))
		rcMargins.top = 0;
	if (!(nBorders & CEB_RIGHT))
		rcMargins.right = 0;
	if (!(nBorders & CEB_BOTTOM))
		rcMargins.bottom = 0;

	if (!IsRectEmpty(&rcMargins))
	{
		// May be work rect will cover our window without margins?
		RECT rcTest = rcWork;
		AddMargins(rcTest, rcMargins, TRUE);

		BOOL bIn2 = IntersectRect(&rcInter, &rcTest, &rcStore);
		if (bIn2 && EqualRect(&rcInter, &rcStore))
		{
			// Ok, it covers, nothing must be changed
			return false;
		}
	}

	bool bChanged = false;

	// We come here when at least part of ConEmu is "Out-of-screen"
	// "bIn == false" when ConEmu is totally "Out-of-screen"
	if (!bIn || !(nBorders & CEB_ALLOW_PARTIAL))
	{
		int nWidth = rcStore.right-rcStore.left;
		int nHeight = rcStore.bottom-rcStore.top;

		// All is bad. Windows is totally out of screen.
		rcWnd.left = max(rcWork.left-rcMargins.left,min(rcStore.left,rcWork.right-nWidth+rcMargins.right));
		rcWnd.top = max(rcWork.top-rcMargins.top,min(rcStore.top,rcWork.bottom-nHeight+rcMargins.bottom));

		// Add Width/Height. They may exceeds monitor in wmNormal.
		if ((wndMode == wmNormal) && !IsCantExceedMonitor())
		{
			rcWnd.right = rcWnd.left+nWidth;
			rcWnd.bottom = rcWnd.top+nHeight;

			// Ideally, we must detects most right/bottom monitor to limit our window
			if (!PtInRect(&rcWork, MakePoint(rcWnd.right, rcWnd.bottom)))
			{
				// Yes, right-bottom corner is out-of-screen
				RECT rcTest = {rcWnd.right, rcWnd.bottom, rcWnd.right, rcWnd.bottom};
				
				MONITORINFO mi2; GetNearestMonitor(&mi2, &rcTest);
				RECT rcWork2 = (wndMode == wmFullScreen) ? mi2.rcMonitor : mi2.rcWork;

				rcWnd.right = min(rcWork2.right+rcMargins.right,rcWnd.right);
				rcWnd.bottom = min(rcWork2.bottom+rcMargins.bottom,rcWnd.bottom);
			}
		}
		else
		{
			// Maximized/FullScreen. Window CAN'T exceeds active monitor!
			rcWnd.right = min(rcWork.right+rcMargins.right,rcWnd.left+rcStore.right-rcStore.left);
			rcWnd.bottom = min(rcWork.bottom+rcMargins.bottom,rcWnd.top+rcStore.bottom-rcStore.top);
		}

		bChanged = (memcmp(&rcWnd, &rcStore, sizeof(rcWnd)) != 0);
	}

	return bChanged;
}

// Для приблизительного расчета размеров - нужен только (размер главного окна)|(размер консоли)
// Для точного расчета размеров - требуется (размер главного окна) и (размер окна отрисовки) для корректировки
// на x64 возникают какие-то глюки с ",RECT rFrom,". Отладчик показывает мусор в rFrom,
// но тем не менее, после "RECT rc = rFrom;", rc получает правильные значения >:|
RECT CConEmuMain::CalcRect(enum ConEmuRect tWhat, const RECT &rFrom, enum ConEmuRect tFrom, CVirtualConsole* pVCon, enum ConEmuMargins tTabAction/*=CEM_TAB*/)
{
	_ASSERTE(this!=NULL);
	RECT rc = rFrom; // инициализация, если уж не получится...
	RECT rcShift = MakeRect(0,0);
	enum ConEmuRect tFromNow = tFrom;

	// tTabAction обязан включать и флаг CEM_TAB
	_ASSERTE((tTabAction & CEM_TAB)==CEM_TAB);

	WARNING("CEM_SCROLL вообще удалить?");
	WARNING("Проверить, чтобы DC нормально центрировалось после удаления CEM_BACK");

	//if (!pVCon)
	//	pVCon = gpConEmu->mp_ VActive;

	if (rFrom.left || rFrom.top)
	{
		if (rFrom.left >= rFrom.right || rFrom.top >= rFrom.bottom)
		{
			MBoxAssert(!(rFrom.left || rFrom.top));
		}
		else
		{
			// Это реально может произойти на втором мониторе.
			// Т.к. сюда передается Rect монитора,
			// полученный из CalcRect(CER_FULLSCREEN, MakeRect(0,0), CER_FULLSCREEN);
		}
	}

	switch (tFrom)
	{
		case CER_MAIN: // switch (tFrom)
			// Нужно отнять отступы рамки и заголовка!
		{
			// Это может быть, если сделали GetWindowRect для ghWnd, когда он isIconic!
			_ASSERTE((rc.left!=-32000 && rc.right!=-32000) && "Use CalcRect(CER_MAIN) instead of GetWindowRect() while IsIconic!");
			rcShift = CalcMargins(CEM_FRAMECAPTION);
			int nWidth = (rc.right-rc.left) - (rcShift.left+rcShift.right);
			int nHeight = (rc.bottom-rc.top) - (rcShift.top+rcShift.bottom);
			rc.left = 0;
			rc.top = 0; // Получили клиентскую область
			#if defined(CONEMU_TABBAR_EX)
			if (gpSet->isTabsInCaption)
			{
				rc.top = rcShift.top;
			}
			#endif
			rc.right = rc.left + nWidth;
			rc.bottom = rc.top + nHeight;
			tFromNow = CER_MAINCLIENT;
		}
		break;
		case CER_MAINCLIENT: // switch (tFrom)
		{
			//
		}
		break;
		case CER_DC: // switch (tFrom)
		{
			// Размер окна отрисовки в пикселах!
			//MBoxAssert(!(rFrom.left || rFrom.top));
			TODO("DoubleView");

			switch (tWhat)
			{
				case CER_MAIN:
				{
					//rcShift = CalcMargins(CEM_BACK);
					//AddMargins(rc, rcShift, TRUE/*abExpand*/);
					rcShift = CalcMargins(tTabAction|CEM_ALL_MARGINS);
					AddMargins(rc, rcShift, TRUE/*abExpand*/);
					//rcShift = CalcMargins(CEM_FRAME);
					//AddMargins(rc, rcShift, TRUE/*abExpand*/);
					WARNING("Неправильно получается при DoubleView");
					tFromNow = CER_MAINCLIENT;
				} break;
				case CER_MAINCLIENT:
				{
					//rcShift = CalcMargins(CEM_BACK);
					//AddMargins(rc, rcShift, TRUE/*abExpand*/);
					rcShift = CalcMargins(tTabAction);
					AddMargins(rc, rcShift, TRUE/*abExpand*/);
					WARNING("Неправильно получается при DoubleView");
					tFromNow = CER_MAINCLIENT;
				} break;
				case CER_TAB:
				{
					//rcShift = CalcMargins(CEM_BACK);
					//AddMargins(rc, rcShift, TRUE/*abExpand*/);
					_ASSERTE(tWhat!=CER_TAB); // вроде этого быть не должно?
					rcShift = CalcMargins(tTabAction);
					AddMargins(rc, rcShift, TRUE/*abExpand*/);
					WARNING("Неправильно получается при DoubleView");
					tFromNow = CER_MAINCLIENT;
				} break;
				case CER_WORKSPACE:
				{
					WARNING("Это tFrom. CER_WORKSPACE - не поддерживается (пока?)");
					_ASSERTE(tWhat!=CER_WORKSPACE);
				} break;
				case CER_BACK:
				{
					//rcShift = CalcMargins(CEM_BACK);
					//AddMargins(rc, rcShift, TRUE/*abExpand*/);
					rcShift = CalcMargins(tTabAction|CEM_CLIENT_MARGINS);
					//AddMargins(rc, rcShift, TRUE/*abExpand*/);
					rc.top += rcShift.top; rc.bottom += rcShift.top;
					_ASSERTE(rcShift.left == 0 && rcShift.right == 0 && rcShift.bottom == 0);
					WARNING("Неправильно получается при DoubleView");
					tFromNow = CER_MAINCLIENT;
				} break;
				default:
					_ASSERTE(FALSE && "Unexpected tWhat");
					break;
			}
		}
		_ASSERTE(rc.right>rc.left && rc.bottom>rc.top);
		return rc;

		case CER_CONSOLE_ALL: // switch (tFrom)
		case CER_CONSOLE_CUR: // switch (tFrom)
		{
			// Тут считаем БЕЗОТНОСИТЕЛЬНО SplitScreen.
			// Т.к. вызов, например, может идти из интерфейса диалога "Settings" и CVConGroup::SetAllConsoleWindowsSize

			// Размер консоли в символах!
			//MBoxAssert(!(rFrom.left || rFrom.top));
			_ASSERTE(tWhat!=CER_CONSOLE_ALL && tWhat!=CER_CONSOLE_CUR);
			//if (gpSetCls->FontWidth()==0) {
			//    MBoxAssert(pVCon!=NULL);
			//    pVCon->InitDC(false, true); // инициализировать ширину шрифта по умолчанию
			//}
			// ЭТО размер окна отрисовки DC
			rc = MakeRect((rFrom.right-rFrom.left) * gpSetCls->FontWidth(),
			              (rFrom.bottom-rFrom.top) * gpSetCls->FontHeight());

			RECT rcScroll = CalcMargins(CEM_SCROLL|CEM_PAD);
			AddMargins(rc, rcScroll, TRUE);

			if (tWhat != CER_DC)
				rc = CalcRect(tWhat, rc, CER_DC);

			// -- tFromNow = CER_BACK; -- менять не требуется, т.к. мы сразу на выход
		}
		_ASSERTE(rc.right>rc.left && rc.bottom>rc.top);
		return rc;

		case CER_FULLSCREEN: // switch (tFrom)
		case CER_MAXIMIZED: // switch (tFrom)
		case CER_RESTORE: // switch (tFrom)
		case CER_MONITOR: // switch (tFrom)
			// Например, таким способом можно получить размер развернутого окна на текущем мониторе
			// RECT rcMax = CalcRect(CER_MAXIMIZED, MakeRect(0,0), CER_MAXIMIZED);
			_ASSERTE((tFrom!=CER_FULLSCREEN && tFrom!=CER_MAXIMIZED && tFrom!=CER_RESTORE) || (tFrom==tWhat));
			break;
		case CER_BACK: // switch (tFrom)
			break;
		default:
			_ASSERTE(tFrom==CER_MAINCLIENT); // для отладки, сюда вроде попадать не должны
			break;
	};

	//// Теперь rc должен соответствовать CER_MAINCLIENT
	//RECT rcAddShift = MakeRect(0,0);

	// если мы дошли сюда - значит tFrom==CER_MAINCLIENT

	switch (tWhat)
	{
		case CER_TAB: // switch (tWhat)
		{
			_ASSERTE(tFrom==CER_MAINCLIENT);
			if (gpSet->nTabsLocation == 1)
			{
				int nStatusHeight = gpSet->isStatusBarShow ? gpSet->StatusBarHeight() : 0;
				rc.top = rc.bottom - nStatusHeight - mp_TabBar->GetTabbarHeight();
			}
			else
			{
				rc.bottom = rc.top + mp_TabBar->GetTabbarHeight();
			}
		} break;
		case CER_WORKSPACE: // switch (tWhat)
		{
			rcShift = CalcMargins(tTabAction|CEM_CLIENT_MARGINS/*|CEM_SCROLL*/);
			AddMargins(rc, rcShift);
		} break;
		case CER_BACK: // switch (tWhat)
		case CER_SCROLL: // switch (tWhat)
		case CER_DC: // switch (tWhat)
		case CER_CONSOLE_ALL: // switch (tWhat)
		case CER_CONSOLE_CUR: // switch (tWhat)
		case CER_CONSOLE_NTVDMOFF: // switch (tWhat)
		{
			rc = CVConGroup::CalcRect(tWhat, rc, tFromNow, pVCon, tTabAction);
			_ASSERTE(rc.right>rc.left && rc.bottom>rc.top);
			return rc;
		} break;

		case CER_MONITOR:    // switch (tWhat)
		case CER_FULLSCREEN: // switch (tWhat)
		case CER_MAXIMIZED:  // switch (tWhat)
		case CER_RESTORE:    // switch (tWhat)
			//case CER_CORRECTED:
		{
			//HMONITOR hMonitor = NULL;
			_ASSERTE(tFrom==tWhat || tFrom==CER_MAIN);

			DEBUGTEST(bool bIconic = isIconic());

			//if (tWhat != CER_CORRECTED)
			//    tFrom = tWhat;
			MONITORINFO mi = {sizeof(mi)};
			DEBUGTEST(HMONITOR hMonitor =) GetNearestMonitor(&mi, (tFrom==CER_MAIN) ? &rFrom : NULL);

			{
				//switch (tFrom) // --было
				switch (tWhat)
				{
					case CER_MONITOR:
					{
						rc = mi.rcWork;

					} break;
					case CER_FULLSCREEN:
					{
						rc = mi.rcMonitor;

					} break;
					case CER_MAXIMIZED:
					{
						rc = mi.rcWork;
						RECT rcFrame = CalcMargins(CEM_FRAMEONLY);
						// Скорректируем размер окна до видимого на мониторе (рамка при максимизации уезжает за пределы экрана)
						rc.left -= rcFrame.left;
						rc.right += rcFrame.right;
						rc.top -= rcFrame.top;
						rc.bottom += rcFrame.bottom;

						// Issue 828: When taskbar is auto-hidden
						APPBARDATA state = {sizeof(state)}; RECT rcTaskbar, rcMatch;
						while ((state.hWnd = FindWindowEx(NULL, state.hWnd, L"Shell_TrayWnd", NULL)) != NULL)
						{
							if (GetWindowRect(state.hWnd, &rcTaskbar)
								&& IntersectRect(&rcMatch, &rcTaskbar, &mi.rcMonitor))
							{
								break; // OK, taskbar match monitor
							}
						}
						// Ok, Is task-bar found on current monitor?
						if (state.hWnd)
						{
							LRESULT lState = SHAppBarMessage(ABM_GETSTATE, &state);
							if (lState & ABS_AUTOHIDE)
							{
								APPBARDATA pos = {sizeof(pos), state.hWnd};
								if (SHAppBarMessage(ABM_GETTASKBARPOS, &pos))
								{
									switch (pos.uEdge)
									{
										case ABE_LEFT:   rc.left   += 1; break;
										case ABE_RIGHT:  rc.right  -= 1; break;
										case ABE_TOP:    rc.top    += 1; break;
										case ABE_BOTTOM: rc.bottom -= 1; break;
									}
								}
							}
						}
						
					} break;
					case CER_RESTORE:
					{
						RECT rcNormal = {0};
						if (gpConEmu)
							rcNormal = gpConEmu->mrc_StoredNormalRect;
						int w = rcNormal.right - rcNormal.left;
						int h = rcNormal.bottom - rcNormal.top;
						if ((w > 0) && (h > 0))
						{
							rc = rcNormal;

							// Если после последней максимизации была изменена 
							// конфигурация мониторов - нужно поправить видимую область
							if (((rc.right + 30) <= mi.rcWork.left)
								|| ((rc.left + 30) >= mi.rcWork.right))
							{
								rc.left = mi.rcWork.left; rc.right = rc.left + w;
							}
							if (((rc.bottom + 30) <= mi.rcWork.top)
								|| ((rc.top + 30) >= mi.rcWork.bottom))
							{
								rc.top = mi.rcWork.top; rc.bottom = rc.top + h;
							}
						}
						else
						{
							rc = mi.rcWork;
						}
					} break;
					default:
					{
						_ASSERTE(tWhat==CER_FULLSCREEN || tWhat==CER_MAXIMIZED);
					}
				}
			}
			//else
			//{
			//	switch(tFrom)
			//	{
			//		case CER_MONITOR:
			//		{
			//			SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
			//		} break;
			//		case CER_FULLSCREEN:
			//		{
			//			rc = MakeRect(GetSystemMetrics(SM_CXFULLSCREEN),GetSystemMetrics(SM_CYFULLSCREEN));
			//		} break;
			//		case CER_MAXIMIZED:
			//		{
			//			rc = MakeRect(GetSystemMetrics(SM_CXMAXIMIZED),GetSystemMetrics(SM_CYMAXIMIZED));

			//			//120814 - ~WS_CAPTION
			//			//if (gpSet->isHideCaption && gpConEmu->mb_MaximizedHideCaption && !gpSet->isHideCaptionAlways())
			//			//	rc.top -= GetSystemMetrics(SM_CYCAPTION);

			//		} break;
			//		case CER_RESTORE:
			//		{
			//			RECT work;
			//			SystemParametersInfo(SPI_GETWORKAREA, 0, &work, 0);
			//			RECT rcNormal = {0};
			//			if (gpConEmu)
			//				rcNormal = gpConEmu->mrc_StoredNormalRect;
			//			int w = rcNormal.right - rcNormal.left;
			//			int h = rcNormal.bottom - rcNormal.top;
			//			if ((w > 0) && (h > 0))
			//			{
			//				rc = rcNormal;

			//				// Если после последней максимизации была изменена 
			//				// конфигурация мониторов - нужно поправить видимую область
			//				if (((rc.right + 30) <= work.left)
			//					|| ((rc.left + 30) >= work.right))
			//				{
			//					rc.left = work.left; rc.right = rc.left + w;
			//				}
			//				if (((rc.bottom + 30) <= work.top)
			//					|| ((rc.top + 30) >= work.bottom))
			//				{
			//					rc.top = work.top; rc.bottom = rc.top + h;
			//				}
			//			}
			//			else
			//			{
			//				WINDOWPLACEMENT wpl = {sizeof(WINDOWPLACEMENT)};
			//				if (ghWnd && GetWindowPlacement(ghWnd, &wpl))
			//					rc = wpl.rcNormalPosition;
			//				else
			//					rc = work;
			//			}
			//		} break;
			//		default:
			//			_ASSERTE(tFrom==CER_FULLSCREEN || tFrom==CER_MAXIMIZED);
			//	}
			//}

			//if (tWhat == CER_CORRECTED)
			//{
			//    RECT rcMon = rc;
			//    rc = rFrom;
			//    int nX = GetSystemMetrics(SM_CXSIZEFRAME), nY = GetSystemMetrics(SM_CYSIZEFRAME);
			//    int nWidth = rc.right-rc.left;
			//    int nHeight = rc.bottom-rc.top;
			//    static bool bFirstCall = true;
			//    if (bFirstCall) {
			//        if (gpSet->wndCascade && !gpSet->isDesktopMode) {
			//            BOOL lbX = ((rc.left+nWidth)>(rcMon.right+nX));
			//            BOOL lbY = ((rc.top+nHeight)>(rcMon.bottom+nY));
			//            {
			//                if (lbX && lbY) {
			//                    rc = MakeRect(rcMon.left,rcMon.top,rcMon.left+nWidth,rcMon.top+nHeight);
			//                } else if (lbX) {
			//                    rc.left = rcMon.right - nWidth; rc.right = rcMon.right;
			//                } else if (lbY) {
			//                    rc.top = rcMon.bottom - nHeight; rc.bottom = rcMon.bottom;
			//                }
			//            }
			//        }
			//        bFirstCall = false;
			//    }
			//	//2010-02-14 На многомониторных конфигурациях эти проверки нарушают
			//	//           требуемое положение (когда окно на обоих мониторах).
			//	//if (rc.left<(rcMon.left-nX)) {
			//	//	rc.left=rcMon.left-nX; rc.right=rc.left+nWidth;
			//	//}
			//	//if (rc.top<(rcMon.top-nX)) {
			//	//	rc.top=rcMon.top-nX; rc.bottom=rc.top+nHeight;
			//	//}
			//	//if ((rc.left+nWidth)>(rcMon.right+nX)) {
			//	//    rc.left = max((rcMon.left-nX),(rcMon.right-nWidth));
			//	//    nWidth = min(nWidth, (rcMon.right-rc.left+2*nX));
			//	//    rc.right = rc.left + nWidth;
			//	//}
			//	//if ((rc.top+nHeight)>(rcMon.bottom+nY)) {
			//	//    rc.top = max((rcMon.top-nY),(rcMon.bottom-nHeight));
			//	//    nHeight = min(nHeight, (rcMon.bottom-rc.top+2*nY));
			//	//    rc.bottom = rc.top + nHeight;
			//	//}
			//}
			_ASSERTE(rc.right>rc.left && rc.bottom>rc.top);
			return rc;
		} break;
		default:
			break;
	}

	//AddMargins(rc, rcAddShift);
	_ASSERTE(rc.right>rc.left && rc.bottom>rc.top);
	return rc; // Посчитали, возвращаем
}

#if 0
/*!!!static!!*/
// Получить размер (правый нижний угол) окна по его клиентской области и наоборот
RECT CConEmuMain::MapRect(RECT rFrom, BOOL bFrame2Client)
{
	_ASSERTE(this!=NULL);
	RECT rcShift = CalcMargins(CEM_FRAMECAPTION);

	if (bFrame2Client)
	{
		//rFrom.left -= rcShift.left;
		//rFrom.top -= rcShift.top;
		rFrom.right -= (rcShift.right+rcShift.left);
		rFrom.bottom -= (rcShift.bottom+rcShift.top);
	}
	else
	{
		//rFrom.left += rcShift.left;
		//rFrom.top += rcShift.top;
		rFrom.right += (rcShift.right+rcShift.left);
		rFrom.bottom += (rcShift.bottom+rcShift.top);
	}

	return rFrom;
}
#endif

bool CConEmuMain::ScreenToVCon(LPPOINT pt, CVirtualConsole** ppVCon)
{
	_ASSERTE(this!=NULL);
	CVirtualConsole* lpVCon = GetVConFromPoint(*pt);

	if (!lpVCon)
		return false;

#if 0
	HWND hView = lpVCon->GetView();
	ScreenToClient(hView, pt);
#endif

	if (ppVCon)
		*ppVCon = lpVCon;

	return true;
}

void CConEmuMain::SyncNtvdm()
{
	OnSize();
}

void CConEmuMain::AutoSizeFont(RECT arFrom, enum ConEmuRect tFrom)
{
	if (!gpSet->isFontAutoSize)
		return;

	// В 16бит режиме - не заморачиваться пока
	if (isNtvdm())
		return;

	if (!WndWidth.Value || !WndHeight.Value)
	{
		Assert(WndWidth.Value!=0 && WndHeight.Value!=0);
		return;
	}

	if ((WndWidth.Style != ss_Standard) || (WndHeight.Style != ss_Standard))
	{
		//TODO: Ниже WndWidth&WndHeight используется для расчета размера шрифта...
		Assert(WndWidth.Value!=0 && WndHeight.Value!=0);
		return;
	}

	//GO

	RECT rc = arFrom;

	if (tFrom == CER_MAIN)
	{
		rc = CalcRect(CER_WORKSPACE, arFrom, CER_MAIN);
	}
	else if (tFrom == CER_MAINCLIENT)
	{
		rc = CalcRect(CER_WORKSPACE, arFrom, CER_MAINCLIENT);
	}
	else
	{
		MBoxAssert(tFrom==CER_MAINCLIENT || tFrom==CER_MAIN);
		return;
	}

	// !!! Для CER_DC размер в rc.right
	int nFontW = (rc.right - rc.left) / WndWidth.Value;

	if (nFontW < 5) nFontW = 5;

	int nFontH = (rc.bottom - rc.top) / WndHeight.Value;

	if (nFontH < 8) nFontH = 8;

	gpSetCls->AutoRecreateFont(nFontW, nFontH);
}

void CConEmuMain::StoreIdealRect()
{
	if ((WindowMode != wmNormal) || mp_Inside)
		return;

	RECT rcWnd = CalcRect(CER_MAIN);
	UpdateIdealRect(rcWnd);
}

RECT CConEmuMain::GetIdealRect()
{
	RECT rcIdeal = mr_Ideal.rcIdeal;
	if (mp_Inside || (rcIdeal.right <= rcIdeal.left) || (rcIdeal.bottom <= rcIdeal.top))
	{
		rcIdeal = GetDefaultRect();
	}
	return rcIdeal;
}

// Если табов не было, создается новая консоль, появляются табы
// Размер окна ConEmu пытаемся НЕ менять, но размер КОНСОЛИ меняется
// Нужно ее запомнить, иначе при Minimize/Restore установится некорректная высота!
void CConEmuMain::OnTabbarActivated(bool bTabbarVisible)
{
	CVConGroup::SyncConsoleToWindow();
	StoreNormalRect(NULL);
}

void CConEmuMain::StoreNormalRect(RECT* prcWnd)
{
	mouse.bCheckNormalRect = false;

	// Обновить коордианты в gpSet, если требуется
	// Если сейчас окно в смене размера - игнорируем, размер запомнит SetWindowMode
	if ((WindowMode == wmNormal) && !mp_Inside && !isIconic())
	{
		if (prcWnd == NULL)
		{
			if (isFullScreen() || isZoomed() || isIconic())
				return;

			//131023 Otherwise, after tiling (Win+Left) 
			//       and Lose/Get focus (Alt+Tab,Alt+Tab)
			//       StoreNormalRect will be called unexpectedly
			// Don't call Estimate here? Avoid excess calculations?
			ConEmuWindowCommand CurTile = GetTileMode(false/*Estimate*/);
			if (CurTile != cwc_Current)
				return;
		}

		RECT rcNormal = {};
		if (prcWnd)
			rcNormal = *prcWnd;
		else
			GetWindowRect(ghWnd, &rcNormal);

		gpSetCls->UpdatePos(rcNormal.left, rcNormal.top);

		// 120720 - если окно сейчас тянут мышкой, то пока не обновлять mrc_StoredNormalRect,
		// иначе, если произошло Maximize при дотягивании до верхнего края экрана - то при
		// восстановлении окна получаем глюк позиционирования - оно прыгает заголовком за пределы.
		if (!isSizing())
			mrc_StoredNormalRect = rcNormal;

		if (CVConGroup::isVConExists(0))
		{
			// При ресайзе через окно настройки - mp_ VActive еще не перерисовался
			// так что и TextWidth/TextHeight не обновился
			//-- gpSetCls->UpdateSize(mp_ VActive->TextWidth, mp_ VActive->TextHeight);
			CESize w = {this->WndWidth.Raw};
			CESize h = {this->WndHeight.Raw};
			
			// Если хранятся размеры в ячейках - нужно позвать CVConGroup::AllTextRect()
			if ((w.Style == ss_Standard) || (h.Style == ss_Standard))
			{
				RECT rcAll = CVConGroup::AllTextRect();
				if (w.Style == ss_Standard)
					w.Set(true, ss_Standard, rcAll.right);
				if (h.Style == ss_Standard)
					h.Set(false, ss_Standard, rcAll.bottom);
			}

			// Размеры в процентах от монитора?
			if ((w.Style == ss_Percents) || (h.Style == ss_Percents))
			{
				MONITORINFO mi;
				GetNearestMonitorInfo(&mi, NULL, &rcNormal);

				if (w.Style == ss_Percents)
					w.Set(true, ss_Percents, (rcNormal.right-rcNormal.left)*100/(mi.rcWork.right - mi.rcWork.left) );
				if (h.Style == ss_Percents)
					h.Set(false, ss_Percents, (rcNormal.bottom-rcNormal.top)*100/(mi.rcWork.bottom - mi.rcWork.top) );
			}

			if (w.Style == ss_Pixels)
				w.Set(true, ss_Pixels, rcNormal.right-rcNormal.left);
			if (h.Style == ss_Pixels)
				h.Set(false, ss_Pixels, rcNormal.bottom-rcNormal.top);

			gpSetCls->UpdateSize(w, h);
		}
	}
}

BOOL CConEmuMain::ShowWindow(int anCmdShow, DWORD nAnimationMS /*= (DWORD)-1*/)
{
	if (mb_LockShowWindow)
	{
		return FALSE;
	}

	if (nAnimationMS == (DWORD)-1)
	{
		nAnimationMS = gpSet->isQuakeStyle ? gpSet->nQuakeAnimation : QUAKEANIMATION_DEF;
	}

	BOOL lbRc = FALSE;
	bool bUseApi = true;

	if (mb_InCreateWindow && (WindowStartTsa || (WindowStartMinimized && gpSet->isMinToTray())))
	{
		_ASSERTE(anCmdShow == SW_SHOWMINNOACTIVE || anCmdShow == SW_HIDE);
		_ASSERTE(::IsWindowVisible(ghWnd) == FALSE);
		anCmdShow = SW_HIDE;
	}


	// In Quake mode - animation proceeds with SetWindowRgn
	//   yes, it is possible to use Slide, but ConEmu may hold
	//   many child windows (outprocess), which don't support
	//   WM_PRINTCLIENT. So, our choice - "trim" window with RGN
	// When Caption is visible - Windows animates window itself.
	// Also, animation brakes transparency
	DWORD nStyleEx = GetWindowLong(ghWnd, GWL_EXSTYLE);
	if (!gpSet->isQuakeStyle && gpSet->isCaptionHidden() && !(nStyleEx & WS_EX_LAYERED) )
	{
		// Well, Caption is hidden, Windows does not animates our window.
		if (anCmdShow == SW_HIDE)
		{
			bUseApi = false; // можно AnimateWindow
		}
		else if (anCmdShow == SW_SHOWMAXIMIZED)
		{
			if (::IsZoomed(ghWnd)) // тут нужно "RAW" значение zoomed
				bUseApi = false; // можно AnimateWindow
		}
		else if (anCmdShow == SW_SHOWNORMAL)
		{
			if (!::IsZoomed(ghWnd) && !::IsIconic(ghWnd)) // тут нужно "RAW" значение zoomed
				bUseApi = false; // можно AnimateWindow
		}
	}

	bool bOldShowMinimized = mb_InShowMinimized;
	mb_InShowMinimized = (anCmdShow == SW_SHOWMINIMIZED) || (anCmdShow == SW_SHOWMINNOACTIVE);

	if (bUseApi)
	{
		bool b, bAllowPreserve = false;
		if (anCmdShow == SW_SHOWMAXIMIZED)
		{
			if (::IsZoomed(ghWnd)) // тут нужно "RAW" значение zoomed
				bAllowPreserve = true;
		}
		else if (anCmdShow == SW_SHOWNORMAL)
		{
			if (!::IsZoomed(ghWnd) && !::IsIconic(ghWnd)) // тут нужно "RAW" значение zoomed
				bAllowPreserve = true;
		}
		// Allowed?
		if (bAllowPreserve)
		{
			b = SetAllowPreserveClient(true);
		}

		//GO
		lbRc = apiShowWindow(ghWnd, anCmdShow);

		if (bAllowPreserve)
		{
			SetAllowPreserveClient(b);
		}
	}
	else
	{
		// Use animation only when Caption is hidden.
		// Otherwise - artifacts comes: while animation - theming does not applies
		// (Win7, Glass - caption style changes while animation)
		_ASSERTE(gpSet->isCaptionHidden());

		DWORD nFlags = AW_BLEND;
		if (anCmdShow == SW_HIDE)
			nFlags |= AW_HIDE;


		if (nAnimationMS > QUAKEANIMATION_MAX)
			nAnimationMS = QUAKEANIMATION_MAX;

		//if (nAnimationMS == 0)
		//	nAnimationMS = CONEMU_ANIMATE_DURATION;
		//else if (nAnimationMS > CONEMU_ANIMATE_DURATION_MAX)
		//	nAnimationMS = CONEMU_ANIMATE_DURATION_MAX;

		AnimateWindow(ghWnd, nAnimationMS, nFlags);
	}

	mb_InShowMinimized = bOldShowMinimized;

	return lbRc;
}

void CConEmuMain::QuakePrevSize::Save(const CESize& awndWidth, const CESize& awndHeight, const int& awndX, const int& awndY, const BYTE& anFrame, const ConEmuWindowMode& aWindowMode, const IdealRectInfo& arcIdealInfo)
{
	wndWidth = awndWidth;
	wndHeight = awndHeight;
	wndX = awndX;
	wndY = awndY;
	nFrame = anFrame;
	WindowMode = aWindowMode;
	rcIdealInfo = arcIdealInfo;
	//
	bWasSaved = true;
}

ConEmuWindowMode CConEmuMain::QuakePrevSize::Restore(CESize& rwndWidth, CESize& rwndHeight, int& rwndX, int& rwndY, BYTE& rnFrame, IdealRectInfo& rrcIdealInfo)
{
	rwndWidth = wndWidth;
	rwndHeight = wndHeight;
	rwndX = wndX;
	rwndY = wndY;
	rnFrame = nFrame;
	rrcIdealInfo = rcIdealInfo;
	return WindowMode;
}

bool CConEmuMain::SetQuakeMode(BYTE NewQuakeMode, ConEmuWindowMode nNewWindowMode /*= wmNotChanging*/, bool bFromDlg /*= false*/)
{
	if (mb_InSetQuakeMode)
	{
		_ASSERTE(mb_InSetQuakeMode==false);
		return false;
	}

	mb_InSetQuakeMode = true;

	BYTE bPrevStyle = gpSet->isQuakeStyle;
	gpSet->isQuakeStyle = NewQuakeMode;

	if (gpSet->isQuakeStyle && bFromDlg)
	{
		gpSet->isTryToCenter = true;
		gpSet->SetMinToTray(true);
	}

	if (NewQuakeMode && gpSet->isDesktopMode)  // этот режим с Desktop несовместим
	{
		gpSet->isDesktopMode = false;
		gpConEmu->OnDesktopMode();
	}

	HWND hWnd2 = ghOpWnd ? gpSetCls->mh_Tabs[CSettings::thi_Show] : NULL; // Страничка с настройками
	if (hWnd2)
	{
		EnableWindow(GetDlgItem(hWnd2, cbQuakeAutoHide), gpSet->isQuakeStyle);
		//EnableWindow(GetDlgItem(hWnd2, tHideCaptionAlwaysFrame), gpSet->isQuakeStyle); -- always enabled on "Appearance" page!
		//EnableWindow(GetDlgItem(hWnd2, stHideCaptionAlwaysFrame), gpSet->isQuakeStyle);
		EnableWindow(GetDlgItem(hWnd2, stQuakeAnimation), gpSet->isQuakeStyle);
		EnableWindow(GetDlgItem(hWnd2, tQuakeAnimation), gpSet->isQuakeStyle);
		gpSetCls->checkDlgButton(hWnd2, cbQuakeStyle, gpSet->isQuakeStyle!=0);
		gpSetCls->checkDlgButton(hWnd2, cbQuakeAutoHide, gpSet->isQuakeStyle==2);
		
		EnableWindow(GetDlgItem(hWnd2, cbSingleInstance), (gpSet->isQuakeStyle == 0));
		gpSetCls->checkDlgButton(hWnd2, cbSingleInstance, gpSetCls->IsSingleInstanceArg());

		gpSetCls->checkDlgButton(hWnd2, cbDesktopMode, gpSet->isDesktopMode);
	}

	hWnd2 = ghOpWnd ? gpSetCls->mh_Tabs[CSettings::thi_SizePos] : NULL; // Страничка с настройками
	if (hWnd2)
	{
		gpSetCls->checkDlgButton(hWnd2, cbTryToCenter, gpSet->isTryToCenter);
	}

	//ConEmuWindowMode nNewWindowMode = 
	//	IsChecked(hWnd2, rMaximized) ? wmMaximized :
	//	IsChecked(hWnd2, rFullScreen) ? wmFullScreen : 
	//	wmNormal;
	if (nNewWindowMode == wmNotChanging || nNewWindowMode == wmCurrent)
		nNewWindowMode = (ConEmuWindowMode)gpSet->_WindowMode;
	else
		gpSet->_WindowMode = nNewWindowMode;

	if (gpSet->isQuakeStyle && !bPrevStyle)
	{
		m_QuakePrevSize.Save(this->WndWidth, this->WndHeight, this->wndX, this->wndY, gpSet->nHideCaptionAlwaysFrame, this->WindowMode, mr_Ideal);
	}
	else if (!gpSet->isQuakeStyle && m_QuakePrevSize.bWasSaved)
	{
		nNewWindowMode = m_QuakePrevSize.Restore(this->WndWidth, this->WndHeight, this->wndX, this->wndY, gpSet->nHideCaptionAlwaysFrame, mr_Ideal);
	}


	HWND hTabsPg = ghOpWnd ? gpSetCls->mh_Tabs[CSettings::thi_Show] : NULL; // Страничка с настройками
	if (hTabsPg)
	{
		gpSetCls->checkDlgButton(hTabsPg, cbHideCaptionAlways, gpSet->isHideCaptionAlways() ? BST_CHECKED : BST_UNCHECKED);
		EnableWindow(GetDlgItem(hTabsPg, cbHideCaptionAlways), !gpSet->isForcedHideCaptionAlways());
	}


	RECT rcWnd = gpConEmu->GetDefaultRect();
	UNREFERENCED_PARAMETER(rcWnd);

	// При отключении Quake режима - сразу "подвинуть" окошко на "старое место"
	// Иначе оно уедет за границы экрана при вызове OnHideCaption
	if (!gpSet->isQuakeStyle && bPrevStyle)
	{
		//SetWindowPos(ghWnd, NULL, rcWnd.left, rcWnd.top, rcWnd.right-rcWnd.left, rcWnd.bottom-rcWnd.top, SWP_NOZORDER);
		m_QuakePrevSize.bWaitReposition = true;
		gpConEmu->UpdateIdealRect(rcWnd);
		mrc_StoredNormalRect = rcWnd;
	}

	gpConEmu->SetWindowMode(nNewWindowMode, TRUE);

	if (m_QuakePrevSize.bWaitReposition)
		m_QuakePrevSize.bWaitReposition = false;

	//if (hWnd2)
	//	SetDlgItemInt(hWnd2, tHideCaptionAlwaysFrame, gpSet->HideCaptionAlwaysFrame(), TRUE);
	if (ghOpWnd && gpSetCls->mh_Tabs[CSettings::thi_Show])
		SetDlgItemInt(gpSetCls->mh_Tabs[CSettings::thi_Show], tHideCaptionAlwaysFrame, gpSet->HideCaptionAlwaysFrame(), TRUE);

	// Save current rect, JIC
	StoreIdealRect();

	apiSetForegroundWindow(ghOpWnd ? ghOpWnd : ghWnd);

	mb_InSetQuakeMode = false;
	return true;
}

ConEmuWindowMode CConEmuMain::GetWindowMode()
{
	ConEmuWindowMode wndMode = gpSet->isQuakeStyle ? ((ConEmuWindowMode)gpSet->_WindowMode) : WindowMode;
	return wndMode;
}

LPCWSTR CConEmuMain::FormatTileMode(ConEmuWindowCommand Tile, wchar_t* pchBuf, size_t cchBufMax)
{
	switch (Tile)
	{
	case cwc_TileLeft:
		_wcscpy_c(pchBuf, cchBufMax, L"cwc_TileLeft"); break;
	case cwc_TileRight:
		_wcscpy_c(pchBuf, cchBufMax, L"cwc_TileRight"); break;
	case cwc_TileHeight:
		_wcscpy_c(pchBuf, cchBufMax, L"cwc_TileHeight"); break;
	default:
		_wsprintf(pchBuf, SKIPLEN(cchBufMax) L"%u", (UINT)Tile);
	}

	return pchBuf;
}

bool CConEmuMain::SetTileMode(ConEmuWindowCommand Tile)
{
	wchar_t szInfo[200], szName[32];

	if (gpSetCls->isAdvLogging)
	{
		_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"SetTileMode(%s)", FormatTileMode(Tile,szName,countof(szName)));
		gpConEmu->LogString(szInfo);
	}


	if (isIconic())
	{
		gpConEmu->LogString(L"SetTileMode SKIPPED due to isIconic");
		return false;
	}

	if (!IsSizePosFree())
	{
		_ASSERTE(FALSE && "Tiling not allowed in Quake and Inside modes");
		gpConEmu->LogString(L"SetTileMode SKIPPED due to Quake/Inside");
		return false;
	}

	if (Tile != cwc_TileLeft && Tile != cwc_TileRight)
	{
		_ASSERTE(Tile==cwc_TileLeft || Tile==cwc_TileRight);
		gpConEmu->LogString(L"SetTileMode SKIPPED due to invalid mode");
		return false;
	}

	MONITORINFO mi = {0};
	ConEmuWindowCommand CurTile = GetTileMode(true/*Estimate*/, &mi);
	ConEmuWindowMode wm = GetWindowMode();

	if (mi.cbSize)
	{
		bool bChange = false;
		RECT rcNewWnd = {};

		if ((wm == wmNormal) && (Tile == cwc_Current))
		{
			StoreNormalRect(NULL);
		}

		HMONITOR hMon = NULL;

		// When window is tiled to the right edge, and user press Win+Right
		// ConEmu must jump to next monitor and set tile to Left
		if ((CurTile == Tile) && (CurTile == cwc_TileLeft || CurTile == cwc_TileRight))
		{
			MONITORINFO nxt = {0};
			hMon = GetNextMonitorInfo(&nxt, &mi.rcWork, (CurTile == cwc_TileRight)/*Next*/);
			//131022 - if there is only one monitor - just switch Left/Right tiling...
			if (!hMon || !nxt.cbSize)
				hMon = GetNearestMonitorInfo(&nxt, NULL, &mi.rcWork);
			if (hMon && nxt.cbSize /* && memcmp(&mi.rcWork, &nxt.rcWork, sizeof(nxt.rcWork))*/)
			{
				memmove(&mi, &nxt, sizeof(nxt));
				Tile = (CurTile == cwc_TileRight) ? cwc_TileLeft : cwc_TileRight;
			}
		}
		else
		{
			if ((Tile == cwc_TileLeft) && (CurTile == cwc_TileRight))
			{
				rcNewWnd = GetDefaultRect();
				m_TileMode = Tile = cwc_Current;
				hMon = MonitorFromWindow(ghWnd, MONITOR_DEFAULTTONEAREST);
				JumpNextMonitor(ghWnd, hMon, false, rcNewWnd);
			}
			else if ((Tile == cwc_TileRight) && (CurTile == cwc_TileLeft))
			{
				rcNewWnd = GetDefaultRect();
				m_TileMode = Tile = cwc_Current;
				hMon = MonitorFromWindow(ghWnd, MONITOR_DEFAULTTONEAREST);
				JumpNextMonitor(ghWnd, hMon, true, rcNewWnd);
			}
		}


		if (Tile == cwc_Current)
		{
			// Вернуться из Tile-режима в нормальный
			rcNewWnd = GetDefaultRect();
			//bChange = true;
		}
		else if (Tile == cwc_TileLeft)
		{
			rcNewWnd.left = mi.rcWork.left;
			rcNewWnd.top = mi.rcWork.top;
			rcNewWnd.bottom = mi.rcWork.bottom;
			rcNewWnd.right = (mi.rcWork.left + mi.rcWork.right) >> 1;
			bChange = true;
		}
		else if (Tile == cwc_TileRight)
		{
			rcNewWnd.right = mi.rcWork.right;
			rcNewWnd.top = mi.rcWork.top;
			rcNewWnd.bottom = mi.rcWork.bottom;
			rcNewWnd.left = (mi.rcWork.left + mi.rcWork.right) >> 1;
			bChange = true;
		}

		if (bChange)
		{
			// Выйти из FullScreen/Maximized режима
			if (GetWindowMode() != wmNormal)
				SetWindowMode(wmNormal);

			// Сразу меняем, чтобы DefaultRect не слетел...
			m_TileMode = Tile;
			changeFromWindowMode = wmNormal;

			SetWindowPos(ghWnd, NULL, rcNewWnd.left, rcNewWnd.top, rcNewWnd.right-rcNewWnd.left, rcNewWnd.bottom-rcNewWnd.top, SWP_NOZORDER);
			
			changeFromWindowMode = wmNotChanging;
		}

		if (gpSetCls->isAdvLogging)
		{
			RECT rc = {}; GetWindowRect(ghWnd, &rc);
			ConEmuWindowCommand NewTile = GetTileMode(true/*Estimate*/, &mi);
			wchar_t szNewTile[32];
			_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"result of SetTileMode(%s) -> %u {%i,%i}-{%i,%i} x%08X -> %s {%i,%i}-{%i,%i}", 
				 FormatTileMode(Tile,szName,countof(szName)),
				 (UINT)bChange,
				 rcNewWnd.left, rcNewWnd.top, rcNewWnd.right, rcNewWnd.bottom,
				 (DWORD)(DWORD_PTR)hMon,
				 FormatTileMode(NewTile,szNewTile,countof(szNewTile)),
				 rc.left, rc.top, rc.right, rc.bottom
				 );
			gpConEmu->LogString(szInfo);
		}
	}

	return true;
}

ConEmuWindowCommand CConEmuMain::GetTileMode(bool Estimate, MONITORINFO* pmi/*=NULL*/)
{
	if (Estimate && IsSizePosFree())
	{
		_ASSERTE(IsWindowVisible(ghWnd) && !isFullScreen() && !isZoomed() && !isIconic());

		ConEmuWindowCommand CurTile = cwc_Current;

		MONITORINFO mi;
		GetNearestMonitorInfo(&mi, NULL, NULL, ghWnd);
		if (pmi)
			*pmi = mi;

		RECT rcWnd;
		GetWindowRect(ghWnd, &rcWnd);
		// _abs(x1-x2) <= 1 ?
		if ((rcWnd.right == mi.rcWork.right)
			&& (rcWnd.top == mi.rcWork.top)
			&& (rcWnd.bottom == mi.rcWork.bottom))
		{
			int nCenter = ((mi.rcWork.right + mi.rcWork.left) >> 1) - rcWnd.left;
			if (_abs(nCenter) <= 4)
			{
				CurTile = cwc_TileRight;
			}
		}
		else if ((rcWnd.left == mi.rcWork.left)
			&& (rcWnd.top == mi.rcWork.top)
			&& (rcWnd.bottom == mi.rcWork.bottom))
		{
			int nCenter = ((mi.rcWork.right + mi.rcWork.left) >> 1) - rcWnd.right;
			if (_abs(nCenter) <= 4)
			{
				CurTile = cwc_TileLeft;
			}
		}

		if (m_TileMode != CurTile)
		{
			// Сменился!
			wchar_t szTile[100], szNewTile[32], szOldTile[32];
			_wsprintf(szTile, SKIPLEN(countof(szTile)) L"Tile mode was changed externally: Our=%s, New=%s",
				FormatTileMode(m_TileMode,szOldTile,countof(szOldTile)), FormatTileMode(CurTile,szNewTile,countof(szNewTile)));
			LogString(szTile);

			m_TileMode = CurTile;
		}
	}

	return m_TileMode;
}

// В некоторых режимах менять положение/размер окна произвольно - нельзя,
// для Quake например окно должно быть прилеплено к верхней границе экрана
// в режиме Inside - размер окна вообще заблокирован и зависит от свободной области
bool CConEmuMain::IsSizeFree(ConEmuWindowMode CheckMode /*= wmFullScreen*/)
{
	// В некоторых режимах - нельзя
	if (mp_Inside)
		return false;

	// В режиме "Desktop" переходить в FullScreen - нельзя
	if (gpSet->isDesktopMode && (CheckMode == wmFullScreen))
		return false;

	// Размер И положение можно менять произвольно
	return true;
}
bool CConEmuMain::IsSizePosFree(ConEmuWindowMode CheckMode /*= wmFullScreen*/)
{
	// В некоторых режимах - нельзя
	if (gpSet->isQuakeStyle || mp_Inside)
		return false;

	// В режиме "Desktop" переходить в FullScreen - нельзя
	if (gpSet->isDesktopMode && (CheckMode == wmFullScreen))
		return false;

	// Размер И положение можно менять произвольно
	return true;
}
// В некоторых режимах - не должен выходить за пределы экрана
bool CConEmuMain::IsCantExceedMonitor()
{
	_ASSERTE(!mp_Inside);
	if (gpSet->isQuakeStyle)
		return true;
	return false;
}
bool CConEmuMain::IsPosLocked()
{
	// В некоторых режимах - двигать окно юзеру вообще нельзя
	if (mp_Inside)
		return true;

	// Положение - строго не ограничивается
	return false;
}

bool CConEmuMain::JumpNextMonitor(bool Next)
{
	if (mp_Inside || gpSet->isDesktopMode)
	{
		LogString(L"JumpNextMonitor skipped, not allowed in Inside/Desktop modes");
		return false;
	}

	wchar_t szInfo[100];
	RECT rcMain;

	HWND hJump = getForegroundWindow();
	if (!hJump)
	{
		Assert(hJump!=NULL);
		LogString(L"JumpNextMonitor skipped, no foreground window");
		return false;
	}
	DWORD nWndPID = 0; GetWindowThreadProcessId(hJump, &nWndPID);
	if (nWndPID != GetCurrentProcessId())
	{
		if (gpSetCls->isAdvLogging)
		{
			_wsprintf(szInfo, SKIPLEN(countof(szInfo))
				L"JumpNextMonitor skipped, not our window PID=%u, HWND=x%08X",
				nWndPID, (DWORD)hJump);
			LogString(szInfo);
		}
		return false;
	}

	if (hJump == ghWnd)
	{
		rcMain = CalcRect(CER_MAIN);
	}
	else
	{
		GetWindowRect(hJump, &rcMain);
	}

	return JumpNextMonitor(hJump, NULL, Next, rcMain);
}

bool CConEmuMain::JumpNextMonitor(HWND hJumpWnd, HMONITOR hJumpMon, bool Next, const RECT rcJumpWnd)
{
	wchar_t szInfo[100];
	RECT rcMain = {};
	bool bFullScreen = false;
	bool bMaximized = false;
	DWORD nStyles = GetWindowLong(hJumpWnd, GWL_STYLE);

	rcMain = rcJumpWnd;
	if (hJumpWnd == ghWnd)
	{
		//-- rcMain = CalcRect(CER_MAIN);
		bFullScreen = isFullScreen();
		bMaximized = bFullScreen ? false : isZoomed();
	}
	else
	{
		//-- GetWindowRect(hJumpWnd, &rcMain);
	}

	MONITORINFO mi = {};
	HMONITOR hNext = hJumpMon ? (GetMonitorInfoSafe(hJumpMon, mi) ? hJumpMon : NULL) : GetNextMonitorInfo(&mi, &rcMain, Next);
	if (!hNext)
	{
		if (gpSetCls->isAdvLogging)
		{
			_wsprintf(szInfo, SKIPLEN(countof(szInfo))
				L"JumpNextMonitor(%i) skipped, GetNextMonitorInfo({%i,%i}-{%i,%i}) returns NULL",
				(int)Next, rcMain.left, rcMain.top, rcMain.right, rcMain.bottom);
			LogString(szInfo);
		}
		return false;
	}
	else if (gpSetCls->isAdvLogging)
	{
		_wsprintf(szInfo, SKIPLEN(countof(szInfo))
			L"JumpNextMonitor(%i), GetNextMonitorInfo({%i,%i}-{%i,%i}) returns 0x%08X ({%i,%i}-{%i,%i})",
			(int)Next, rcMain.left, rcMain.top, rcMain.right, rcMain.bottom,
			(DWORD)(DWORD_PTR)hNext, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom);
		LogString(szInfo);
	}

	MONITORINFO miCur;
	DEBUGTEST(HMONITOR hCurMonitor = )
		GetNearestMonitor(&miCur, &rcMain);

	RECT rcNewMain = rcMain;

	if ((bFullScreen || bMaximized) && (hJumpWnd == ghWnd))
	{
		_ASSERTE(hJumpWnd == ghWnd);
		rcNewMain = CalcRect(bFullScreen ? CER_FULLSCREEN : CER_MAXIMIZED, mi.rcMonitor, CER_MAIN);
	}
	else
	{
		_ASSERTE(WindowMode==wmNormal || hJumpWnd!=ghWnd);

		#ifdef _DEBUG
		RECT rcPrevMon = bFullScreen ? miCur.rcMonitor : miCur.rcWork;
		RECT rcNextMon = bFullScreen ? mi.rcMonitor : mi.rcWork;
		#endif

		// Если мониторы различаются по разрешению или рабочей области - коррекция позиционирования
		int ShiftX = rcMain.left - miCur.rcWork.left;
		int ShiftY = rcMain.top - miCur.rcWork.top;

		int Width = (rcMain.right - rcMain.left);
		int Height = (rcMain.bottom - rcMain.top);

		// Если ширина или высота были заданы в процентах (от монитора)
		if ((WndWidth.Style == ss_Percents) || (WndHeight.Style == ss_Percents))
		{
			_ASSERTE(!gpSet->isQuakeStyle); // Quake?

			if ((WindowMode == wmNormal) && hNext)
			{
				RECT rcMargins = CalcMargins(CEM_FRAMECAPTION|CEM_SCROLL|CEM_STATUS|CEM_PAD|CEM_TAB);
				SIZE szNewSize = GetDefaultSize(false, &this->WndWidth, &this->WndHeight, hNext);
				if ((szNewSize.cx > 0) && (szNewSize.cy > 0))
				{
					if (WndWidth.Style == ss_Percents)
						Width = szNewSize.cx + rcMargins.left + rcMargins.right;
					if (WndHeight.Style == ss_Percents)
						Height = szNewSize.cy + rcMargins.top + rcMargins.bottom;
				}
				else
				{
					_ASSERTE((szNewSize.cx > 0) && (szNewSize.cy > 0));
				}
			}
		}


		if (ShiftX > 0)
		{
			int SpaceX = ((miCur.rcWork.right - miCur.rcWork.left) - Width);
			int NewSpaceX = ((mi.rcWork.right - mi.rcWork.left) - Width);
			if (SpaceX > 0)
			{
				if (NewSpaceX <= 0)
					ShiftX = 0;
				else
					ShiftX = ShiftX * NewSpaceX / SpaceX;
			}
		}

		if (ShiftY > 0)
		{
			int SpaceY = ((miCur.rcWork.bottom - miCur.rcWork.top) - Height);
			int NewSpaceY = ((mi.rcWork.bottom - mi.rcWork.top) - Height);
			if (SpaceY > 0)
			{
				if (NewSpaceY <= 0)
					ShiftY = 0;
				else
					ShiftY = ShiftY * NewSpaceY / SpaceY;
			}
		}

		rcNewMain.left = mi.rcWork.left + ShiftX;
		rcNewMain.top = mi.rcWork.top + ShiftY;
		rcNewMain.right = rcNewMain.left + Width;
		rcNewMain.bottom = rcNewMain.top + Height;
	}

	
	// Поскольку кнопки мы перехватываем - нужно предусмотреть
	// прыжки и для других окон, например окна настроек
	if (hJumpWnd == ghWnd)
	{
		// Коррекция (заранее)
		OnMoving(&rcNewMain);

		m_JumpMonitor.rcNewPos = rcNewMain;
		m_JumpMonitor.bInJump = true;
		m_JumpMonitor.bFullScreen = bFullScreen;
		m_JumpMonitor.bMaximized = bMaximized;

		GetWindowLong(ghWnd, GWL_STYLE);
		if (bFullScreen)
		{
			// Win8. Не хочет прыгать обратно
			SetWindowStyle(ghWnd, nStyles & ~WS_MAXIMIZE);
		}

		AutoSizeFont(rcNewMain, CER_MAIN);
		// Расчитать размер консоли по оптимальному WindowRect
		CVConGroup::PreReSize(WindowMode, rcNewMain);
	}


	// И перемещение
	SetWindowPos(hJumpWnd, NULL, rcNewMain.left, rcNewMain.top, rcNewMain.right-rcNewMain.left, rcNewMain.bottom-rcNewMain.top, SWP_NOZORDER/*|SWP_DRAWFRAME*/|SWP_NOCOPYBITS);
	//MoveWindow(hJumpWnd, rcNewMain.left, rcNewMain.top, rcNewMain.right-rcNewMain.left, rcNewMain.bottom-rcNewMain.top, FALSE);


	if (hJumpWnd == ghWnd)
	{
		if (bFullScreen)
		{
			// вернуть
			SetWindowStyle(ghWnd, nStyles);
			// Check it
			_ASSERTE(WindowMode == wmFullScreen);
		}

		m_JumpMonitor.bInJump = false;
	}

	return false;
}

// inMode: wmNormal, wmMaximized, wmFullScreen
bool CConEmuMain::SetWindowMode(ConEmuWindowMode inMode, BOOL abForce /*= FALSE*/, BOOL abFirstShow /*= FALSE*/)
{
	if (inMode != wmNormal && inMode != wmMaximized && inMode != wmFullScreen)
		inMode = wmNormal; // ошибка загрузки настроек?

	if (!isMainThread()) 
	{
		PostMessage(ghWnd, mn_MsgSetWindowMode, inMode, 0);
		return false;
	}

	//if (gpSet->isQuakeStyle && !mb_InSetQuakeMode)
	//{
	//	bool bQuake = SetQuakeMode(gpSet->isQuakeStyle
	//}

	if (gpSet->isDesktopMode)
	{
		if (inMode == wmFullScreen)
			inMode = (WindowMode != wmNormal) ? wmNormal : wmMaximized; // FullScreen на Desktop-е невозможен
	}

	wchar_t szInfo[128];

	if (gpSetCls->isAdvLogging)
	{
		_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"SetWindowMode(%s) begin", GetWindowModeName(inMode));
		LogString(szInfo);
	}

	if ((inMode != wmFullScreen) && (WindowMode == wmFullScreen))
	{
		// Отмена vkForceFullScreen
		DoForcedFullScreen(false);
	}

	#ifdef _DEBUG
	DWORD_PTR dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE);
	#endif

	if (isPictureView())
	{
		#ifndef _DEBUG
		//2009-04-22 Если открыт PictureView - лучше не дергаться...
		return false;
		#else
		_ASSERTE(FALSE && "Change WindowMode will be skipped in Release: isPictureView()");
		#endif
	}

	SetCursor(LoadCursor(NULL,IDC_WAIT));
	mp_Menu->SetPassSysCommand(true);

	//WindowPlacement -- использовать нельзя, т.к. он работает в координатах Workspace, а не Screen!
	RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);

	//bool canEditWindowSizes = false;
	bool lbRc = false;
	static bool bWasSetFullscreen = false;
	// Сброс флагов ресайза мышкой
	ResetSizingFlags(MOUSE_SIZING_BEGIN|MOUSE_SIZING_TODO);

	bool bNewFullScreen = (inMode == wmFullScreen) || (gpSet->isQuakeStyle && (gpSet->_WindowMode == wmFullScreen));

	if (bWasSetFullscreen && !bNewFullScreen)
	{
		if (mp_TaskBar2)
		{
			if (!gpSet->isDesktopMode)
				mp_TaskBar2->MarkFullscreenWindow(ghWnd, FALSE);

			bWasSetFullscreen = false;
		}
	}

	bool bIconic = isIconic();

	// When changing more to smth else than wmNormal, and current mode is wmNormal
	// save current window rect for futher usage (when restoring from wmMaximized for example)
	if ((inMode != wmNormal) && isWindowNormal() && !bIconic)
		StoreNormalRect(&rcWnd);

	// Коррекция для Quake/Inside/Desktop
	ConEmuWindowMode NewMode = IsSizePosFree(inMode) ? inMode : wmNormal;

	// И сразу запоминаем _новый_ режим
	changeFromWindowMode = WindowMode;
	mp_Menu->SetRestoreFromMinimized(bIconic);
	if (gpSet->isQuakeStyle)
	{
		gpSet->_WindowMode = inMode;
		WindowMode = wmNormal;
	}
	else
	{
		WindowMode = inMode;
	}


	CVConGuard VCon;
	GetActiveVCon(&VCon);
	//CRealConsole* pRCon = (gpSetCls->isAdvLogging!=0) ? (VCon.VCon() ? VCon.VCon()->RCon() : NULL) : NULL;

	if (gpSetCls->isAdvLogging)
	{
		_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"SetWindowMode(%s)", GetWindowModeName(inMode));
		LogString(szInfo);
	}

	OnHideCaption(); // inMode из параметров убрал, т.к. WindowMode уже изменен

	DEBUGTEST(BOOL bIsVisible = IsWindowVisible(ghWnd););

	//!!!
	switch (NewMode)
	{
		case wmNormal:
		{
			DEBUGLOGFILE("SetWindowMode(wmNormal)\n");
			//RECT consoleSize;

			RECT rcIdeal = (gpSet->isQuakeStyle) ? GetDefaultRect() : GetIdealRect();
			AutoSizeFont(rcIdeal, CER_MAIN);
			// Расчитать размер по оптимальному WindowRect
			if (!CVConGroup::PreReSize(inMode, rcIdeal))
			{
				mp_Menu->SetPassSysCommand(false);
				goto wrap;
			}

			//consoleSize = MakeRect(gpConEmu->wndWidth, gpConEmu->wndHeight);

			// Тут именно "::IsZoomed(ghWnd)"
			if (isIconic() || ::IsZoomed(ghWnd))
			{
				//apiShow Window(ghWnd, SW_SHOWNORMAL); // WM_SYSCOMMAND использовать не хочется...
				mb_IgnoreSizeChange = true;

				if (gpSet->isDesktopMode)
				{
					RECT rcNormal = CalcRect(CER_RESTORE, MakeRect(0,0), CER_RESTORE);
					DWORD_PTR dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE);
					if (dwStyle & WS_MAXIMIZE)
						SetWindowStyle(dwStyle&~WS_MAXIMIZE);
					SetWindowPos(ghWnd, HWND_TOP, 
						rcNormal.left, rcNormal.top, 
						rcNormal.right-rcNormal.left, rcNormal.bottom-rcNormal.top,
						SWP_NOCOPYBITS|SWP_SHOWWINDOW);
				}
				else if (IsWindowVisible(ghWnd))
				{
					if (gpSetCls->isAdvLogging) LogString("WM_SYSCOMMAND(SC_RESTORE)");

					DefWindowProc(ghWnd, WM_SYSCOMMAND, SC_RESTORE, 0); //2009-04-22 Было SendMessage
				}
				else
				{
					if (gpSetCls->isAdvLogging) LogString("ShowWindow(SW_SHOWNORMAL)");

					ShowWindow(SW_SHOWNORMAL);
				}

				//RePaint();
				mb_IgnoreSizeChange = false;

				//// Сбросить (заранее), вдруг оно isIconic?
				//if (mb_MaximizedHideCaption)
				//	mb_MaximizedHideCaption = FALSE;

				if (gpSetCls->isAdvLogging) LogString("OnSize(false).1");

				//OnSize(false); // подровнять ТОЛЬКО дочерние окошки
			}

			//// Сбросить (однозначно)
			//if (mb_MaximizedHideCaption)
			//	mb_MaximizedHideCaption = FALSE;

			RECT rcNew = GetDefaultRect();
			if (mp_Inside == NULL)
			{
				//130508 - интересует только ширина-высота
				//rcNew = CalcRect(CER_MAIN, consoleSize, CER_CONSOLE_ALL);
				rcNew.right -= rcNew.left; rcNew.bottom -= rcNew.top;
				rcNew.top = rcNew.left = 0;

				if ((mrc_StoredNormalRect.right > mrc_StoredNormalRect.left) && (mrc_StoredNormalRect.bottom > mrc_StoredNormalRect.top))
				{
					rcNew.left+=mrc_StoredNormalRect.left; rcNew.top+=mrc_StoredNormalRect.top;
					rcNew.right+=mrc_StoredNormalRect.left; rcNew.bottom+=mrc_StoredNormalRect.top;
				}
				else
				{
					MONITORINFO mi; GetNearestMonitor(&mi);
					rcNew.left+=mi.rcWork.left; rcNew.top+=mi.rcWork.top;
					rcNew.right+=mi.rcWork.left; rcNew.bottom+=mi.rcWork.top;
				}
			}

			#ifdef _DEBUG
			WINDOWPLACEMENT wpl = {sizeof(wpl)}; GetWindowPlacement(ghWnd, &wpl);
			#endif

			if (gpSetCls->isAdvLogging)
			{
				char szInfo[128]; wsprintfA(szInfo, "SetWindowPos(X=%i, Y=%i, W=%i, H=%i)", rcNew.left, rcNew.top, rcNew.right-rcNew.left, rcNew.bottom-rcNew.top);
				LogString(szInfo);
			}

			setWindowPos(NULL, rcNew.left, rcNew.top, rcNew.right-rcNew.left, rcNew.bottom-rcNew.top, SWP_NOZORDER);

			#ifdef _DEBUG
			GetWindowPlacement(ghWnd, &wpl);
			#endif

			gpSetCls->UpdateWindowMode(wmNormal);

			if (!IsWindowVisible(ghWnd))
			{
				ShowWindow((abFirstShow && WindowStartMinimized) ? SW_SHOWMINNOACTIVE : SW_SHOWNORMAL);
			}

			#ifdef _DEBUG
			GetWindowPlacement(ghWnd, &wpl);
			#endif

			// Если это во время загрузки - то до первого ShowWindow - isIconic возвращает FALSE
			// Нужен реальный IsZoomed (FullScreen теперь тоже Zoomed)
			if (!(abFirstShow && WindowStartMinimized) && (isIconic() || ::IsZoomed(ghWnd)))
			{
				ShowWindow(SW_SHOWNORMAL); // WM_SYSCOMMAND использовать не хочется...
				// что-то после AltF9, AltF9 уголки остаются не срезанными...
				//hRgn = CreateWindowRgn();
				//SetWindowRgn(ghWnd, hRgn, TRUE);
			}

			// Already resored, need to clear the flag to avoid incorrect sizing
			mp_Menu->SetRestoreFromMinimized(false);

			UpdateWindowRgn();
			OnSize(false); // подровнять ТОЛЬКО дочерние окошки

			//#ifdef _DEBUG
			//GetWindowPlacement(ghWnd, &wpl);
			//UpdateWindow(ghWnd);
			//#endif
		} break; //wmNormal

		case wmMaximized:
		{
			DEBUGLOGFILE("SetWindowMode(wmMaximized)\n");
			_ASSERTE(gpSet->isQuakeStyle==0); // Must not get here for Quake mode

			#ifdef _DEBUG
			RECT rcShift = CalcMargins(CEM_FRAMEONLY);
			_ASSERTE((rcShift.left==0 && rcShift.right==0 && rcShift.bottom==0 && rcShift.top==0) || !gpSet->isCaptionHidden());
			#endif

			#ifdef _DEBUG // было
			CalcRect(CER_MAXIMIZED, MakeRect(0,0), CER_MAXIMIZED);
			#endif
			RECT rcMax = CalcRect(CER_MAXIMIZED);

			WARNING("Может обломаться из-за максимального размера консоли");
			// в этом случае хорошо бы установить максимально возможный и отцентрировать ее в ConEmu
			if (!CVConGroup::PreReSize(inMode, rcMax))
			{
				mp_Menu->SetPassSysCommand(false);
				goto wrap;
			}

			//if (mb_MaximizedHideCaption || !::IsZoomed(ghWnd))
			if (bIconic || (changeFromWindowMode != wmMaximized))
			{
				mb_IgnoreSizeChange = true;

				InvalidateAll();

				if (!gpSet->isDesktopMode)
				{
					DEBUGTEST(WINDOWPLACEMENT wpl1 = {sizeof(wpl1)}; GetWindowPlacement(ghWnd, &wpl1););
					ShowWindow(SW_SHOWMAXIMIZED);
					DEBUGTEST(WINDOWPLACEMENT wpl2 = {sizeof(wpl2)}; GetWindowPlacement(ghWnd, &wpl2););
					if (changeFromWindowMode == wmFullScreen)
					{
						setWindowPos(HWND_TOP, 
							rcMax.left, rcMax.top, 
							rcMax.right-rcMax.left, rcMax.bottom-rcMax.top,
							SWP_NOCOPYBITS|SWP_SHOWWINDOW);
					}
				}
				else
				{
					/*WINDOWPLACEMENT wpl = {sizeof(WINDOWPLACEMENT)};
					GetWindowPlacement(ghWnd, &wpl);
					wpl.flags = 0;
					wpl.showCmd = SW_SHOWMAXIMIZED;
					wpl.ptMaxPosition.x = rcMax.left;
					wpl.ptMaxPosition.y = rcMax.top;
					SetWindowPlacement(ghWnd, &wpl);*/
					DWORD_PTR dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE);
					SetWindowStyle(dwStyle|WS_MAXIMIZE);
					SetWindowPos(ghWnd, HWND_TOP, 
						rcMax.left, rcMax.top, 
						rcMax.right-rcMax.left, rcMax.bottom-rcMax.top,
						SWP_NOCOPYBITS|SWP_SHOWWINDOW);
				}
				mb_IgnoreSizeChange = false;

				//RePaint();
				InvalidateAll();

				if (gpSetCls->isAdvLogging) LogString("OnSize(false).2");

				//OnSize(false); // консоль уже изменила свой размер

				gpSetCls->UpdateWindowMode(wmMaximized);
			} // if (!gpSet->isHideCaption)

			if (!IsWindowVisible(ghWnd))
			{
				mb_IgnoreSizeChange = true;
				ShowWindow((abFirstShow && WindowStartMinimized) ? SW_SHOWMINNOACTIVE : SW_SHOWMAXIMIZED);
				mb_IgnoreSizeChange = false;

				if (gpSetCls->isAdvLogging) LogString("OnSize(false).3");
			}

			// Already resored, need to clear the flag to avoid incorrect sizing
			mp_Menu->SetRestoreFromMinimized(false);

			OnSize(false); // консоль уже изменила свой размер

			UpdateWindowRgn();

		} break; //wmMaximized

		case wmFullScreen:
		{
			DEBUGLOGFILE("SetWindowMode(wmFullScreen)\n");
			_ASSERTE(gpSet->isQuakeStyle==0); // Must not get here for Quake mode

			RECT rcMax = CalcRect(CER_FULLSCREEN, MakeRect(0,0), CER_FULLSCREEN);

			WARNING("Может обломаться из-за максимального размера консоли");
			// в этом случае хорошо бы установить максимально возможный и отцентрировать ее в ConEmu
			if (!CVConGroup::PreReSize(inMode, rcMax))
			{
				mp_Menu->SetPassSysCommand(false);
				goto wrap;
			}

			//mb_isFullScreen = true;
			//WindowMode = inMode; // Запомним!

			//120820 - т.к. FullScreen теперь SW_SHOWMAXIMIZED, то здесь нужно смотреть на WindowMode
			isWndNotFSMaximized = (changeFromWindowMode == wmMaximized);
			
			RECT rcShift = CalcMargins(CEM_FRAMEONLY);
			_ASSERTE(rcShift.left==0 && rcShift.right==0 && rcShift.top==0 && rcShift.bottom==0);

			//GetCWShift(ghWnd, &rcShift); // Обновить, на всякий случай
			// Умолчания
			ptFullScreenSize.x = GetSystemMetrics(SM_CXSCREEN)+rcShift.left+rcShift.right;
			ptFullScreenSize.y = GetSystemMetrics(SM_CYSCREEN)+rcShift.top+rcShift.bottom;
			// которые нужно уточнить для текущего монитора!
			MONITORINFO mi = {};
			HMONITOR hMon = GetNearestMonitor(&mi);

			if (hMon)
			{
				ptFullScreenSize.x = (mi.rcMonitor.right-mi.rcMonitor.left)+rcShift.left+rcShift.right;
				ptFullScreenSize.y = (mi.rcMonitor.bottom-mi.rcMonitor.top)+rcShift.top+rcShift.bottom;
			}

			//120820 - Тут нужно проверять реальный IsZoomed
			if (isIconic() || !::IsZoomed(ghWnd))
			{
	 				mb_IgnoreSizeChange = true;
				//120820 для четкости, в FullScreen тоже ставим Maximized, а не Normal
				ShowWindow((abFirstShow && WindowStartMinimized) ? SW_SHOWMINNOACTIVE : SW_SHOWMAXIMIZED);

				//// Сбросить
				//if (mb_MaximizedHideCaption)
				//	mb_MaximizedHideCaption = FALSE;

				mb_IgnoreSizeChange = false;
				//120820 - лишняя перерисовка?
				//-- RePaint();
			}

			if (mp_TaskBar2)
			{
				if (!gpSet->isDesktopMode)
					mp_TaskBar2->MarkFullscreenWindow(ghWnd, TRUE);

				bWasSetFullscreen = true;
			}

			// for virtual screens mi.rcMonitor. may contains negative values...

			if (gpSetCls->isAdvLogging)
			{
				char szInfo[128]; wsprintfA(szInfo, "SetWindowPos(X=%i, Y=%i, W=%i, H=%i)", -rcShift.left+mi.rcMonitor.left,-rcShift.top+mi.rcMonitor.top, ptFullScreenSize.x,ptFullScreenSize.y);
				LogString(szInfo);
			}

			// Already resored, need to clear the flag to avoid incorrect sizing
			mp_Menu->SetRestoreFromMinimized(false);

			OnSize(false); // подровнять ТОЛЬКО дочерние окошки

			RECT rcFrame = CalcMargins(CEM_FRAMEONLY);
			// ptFullScreenSize содержит "скорректированный" размер (он больше монитора)
			UpdateWindowRgn(rcFrame.left, rcFrame.top,
				            mi.rcMonitor.right-mi.rcMonitor.left, mi.rcMonitor.bottom-mi.rcMonitor.top);
			setWindowPos(NULL,
			                -rcShift.left+mi.rcMonitor.left,-rcShift.top+mi.rcMonitor.top,
			                ptFullScreenSize.x,ptFullScreenSize.y,
			                SWP_NOZORDER);

			gpSetCls->UpdateWindowMode(wmFullScreen);

			if (!IsWindowVisible(ghWnd))
			{
				mb_IgnoreSizeChange = true;
				ShowWindow((abFirstShow && WindowStartMinimized) ? SW_SHOWMINNOACTIVE : SW_SHOWNORMAL);
				mb_IgnoreSizeChange = false;
				//WindowMode = inMode; // Запомним!

				if (gpSetCls->isAdvLogging) LogString("OnSize(false).5");

				OnSize(false); // подровнять ТОЛЬКО дочерние окошки
			}

			#ifdef _DEBUG
			bool bZoomed = ::IsZoomed(ghWnd);
			_ASSERTE(bZoomed);
			#endif

		} break; //wmFullScreen

	default:
		_ASSERTE(FALSE && "Unsupported mode");
	}

	if (gpSetCls->isAdvLogging) LogString("SetWindowMode done");

	//WindowMode = inMode; // Запомним!

	//if (VCon.VCon())
	//{
	//	VCon->RCon()->SyncGui2Window();
	//}

	//if (ghOpWnd && (gpSetCls->mh_Tabs[CSettings::thi_SizePos]))
	//{
	//	bool canEditWindowSizes = (inMode == wmNormal);
	//	HWND hSizePos = gpSetCls->mh_Tabs[CSettings::thi_SizePos];
	//	EnableWindow(GetDlgItem(hSizePos, tWndWidth), canEditWindowSizes);
	//	EnableWindow(GetDlgItem(hSizePos, tWndHeight), canEditWindowSizes);
	//}

	if (gpSetCls->isAdvLogging)
	{
		wchar_t szInfo[64];
		_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"SetWindowMode(%u) end", inMode);
		LogWindowPos(szInfo);
	}

	//Sync ConsoleToWindow(); 2009-09-10 А это вроде вообще не нужно - ресайз консоли уже сделан
	mp_Menu->SetPassSysCommand(false);
	lbRc = true;
wrap:
	if (!lbRc)
	{
		if (gpSetCls->isAdvLogging)
		{
			LogString(L"Failed to switch WindowMode");
		}
		_ASSERTE(lbRc && "Failed to switch WindowMode");
		WindowMode = changeFromWindowMode;
	}
	changeFromWindowMode = wmNotChanging;
	mp_Menu->SetRestoreFromMinimized(false);

	// В случае облома изменения размера консоли - может слететь признак
	// полноэкранности у панели задач. Вернем его...
	if (mp_TaskBar2)
	{
		bNewFullScreen = (WindowMode == wmFullScreen) || (gpSet->isQuakeStyle && (gpSet->_WindowMode == wmFullScreen));
		if (bWasSetFullscreen != bNewFullScreen)
		{
			if (!gpSet->isDesktopMode)
				mp_TaskBar2->MarkFullscreenWindow(ghWnd, bNewFullScreen);

			bWasSetFullscreen = bNewFullScreen;
		}
	}

	mp_TabBar->OnWindowStateChanged();

	#ifdef _DEBUG
	dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE);
	#endif

	// Transparency styles may was changed occasionally, check them
	OnTransparent();

	TODO("Что-то курсор иногда остается APPSTARTING...");
	SetCursor(LoadCursor(NULL,IDC_ARROW));
	//PostMessage(ghWnd, WM_SETCURSOR, -1, -1);
	return lbRc;
}

void CConEmuMain::ForceShowTabs(BOOL abShow)
{
	//if (!mp_ VActive)
	//  return;
	//2009-05-20 Раз это Force - значит на возможность получить табы из фара забиваем! Для консоли показывается "Console"
	BOOL lbTabsAllowed = abShow /*&& gpConEmu->mp_TabBar->IsAllowed()*/;

	if (abShow && !gpConEmu->mp_TabBar->IsTabsShown() && gpSet->isTabs && lbTabsAllowed)
	{
		gpConEmu->mp_TabBar->Activate(TRUE);
		//ConEmuTab tab; memset(&tab, 0, sizeof(tab));
		//tab.Pos=0;
		//tab.Current=1;
		//tab.Type = 1;
		//gpConEmu->mp_TabBar->Update(&tab, 1);
		//mp_ VActive->RCon()->SetTabs(&tab, 1);
		gpConEmu->mp_TabBar->Update();
		//gbPostUpdateWindowSize = true; // 2009-07-04 Resize выполняет сам TabBar
	}
	else if (!abShow)
	{
		gpConEmu->mp_TabBar->Deactivate(TRUE);
		//gbPostUpdateWindowSize = true; // 2009-07-04 Resize выполняет сам TabBar
	}

	// Иначе Gaps не отрисуются
	gpConEmu->InvalidateAll();
	UpdateWindowRgn();
	// При отключенных табах нужно показать "[n/n] " а при выключенных - спрятать
	UpdateTitle(); // сам перечитает
	// 2009-07-04 Resize выполняет сам TabBar
	//if (gbPostUpdateWindowSize) { // значит мы что-то поменяли
	//    ReSize();
	//    /*RECT rcNewCon; Get ClientRect(ghWnd, &rcNewCon);
	//    DCClientRect(&rcNewCon);
	//    MoveWindow(ghWnd DC, rcNewCon.left, rcNewCon.top, rcNewCon.right - rcNewCon.left, rcNewCon.bottom - rcNewCon.top, 0);
	//    dcWindowLast = rcNewCon;
	//
	//    if (gpSet->LogFont.lfWidth)
	//    {
	//        Sync ConsoleToWindow();
	//    }*/
	//}
}

bool CConEmuMain::isIconic(bool abRaw /*= false*/)
{
	bool bIconic = ::IsIconic(ghWnd);
	if (!abRaw && !bIconic)
	{
		bIconic = (gpSet->isQuakeStyle && isQuakeMinimized)
			// Don't assume "iconic" while creating window
			// otherwise "StoreNormalRect" will fails
			|| (!mb_InCreateWindow && !IsWindowVisible(ghWnd));
	}
	return bIconic;
}

bool CConEmuMain::isWindowNormal()
{
	#ifdef _DEBUG
	bool bZoomed = ::IsZoomed(ghWnd);
	bool bIconic = ::IsIconic(ghWnd);
	bool bInTransition = (changeFromWindowMode == wmNormal) || mb_InCreateWindow
		|| (m_JumpMonitor.bInJump && (m_JumpMonitor.bFullScreen || m_JumpMonitor.bMaximized))
		|| !IsWindowVisible(ghWnd);
	_ASSERTE((WindowMode == wmNormal) || bZoomed || bIconic || bInTransition);
	#endif

	if ((WindowMode != wmNormal) || !IsSizeFree())
		return false;

	if (::IsIconic(ghWnd))
		return false;

	return true;
}

bool CConEmuMain::isZoomed()
{
	#ifdef _DEBUG
	bool bZoomed = ::IsZoomed(ghWnd);
	bool bIconic = ::IsIconic(ghWnd);
	bool bInTransition = (changeFromWindowMode == wmNormal) || mb_InCreateWindow
		|| (m_JumpMonitor.bInJump && (m_JumpMonitor.bFullScreen || m_JumpMonitor.bMaximized))
		|| !IsWindowVisible(ghWnd);
	_ASSERTE((WindowMode == wmNormal) || bZoomed || bIconic || bInTransition);
	#endif

	if (WindowMode != wmMaximized)
		return false;
	if (::IsIconic(ghWnd))
		return false;

	return true;
}

bool CConEmuMain::isFullScreen()
{
	#ifdef _DEBUG
	bool bZoomed = ::IsZoomed(ghWnd);
	bool bIconic = ::IsIconic(ghWnd);
	bool bInTransition = (changeFromWindowMode == wmNormal) || mb_InCreateWindow
		|| (m_JumpMonitor.bInJump && (m_JumpMonitor.bFullScreen || m_JumpMonitor.bMaximized))
		|| !IsWindowVisible(ghWnd);
	_ASSERTE((WindowMode == wmNormal) || bZoomed || bIconic || bInTransition);
	#endif

	if (WindowMode != wmFullScreen)
		return false;
	if (::IsIconic(ghWnd))
		return false;

	return true;
}

// abCorrect2Ideal==TRUE приходит из TabBarClass::UpdatePosition(),
// это происходит при отображении/скрытии "автотабов"
// или при смене шрифта
void CConEmuMain::ReSize(BOOL abCorrect2Ideal /*= FALSE*/)
{
	if (isIconic())
		return;

	RECT client = GetGuiClientRect();

	#ifdef _DEBUG
	DWORD_PTR dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE);
	#endif

	if (abCorrect2Ideal)
	{
		if (isWindowNormal())
		{
			// Вобщем-то интересует только X,Y
			RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);

			// Пользователи жалуются на смену размера консоли

			#if 0
			// -- не годится. при запуске новой консоли и автопоказе табов
			// -- размер "CVConGroup::AllTextRect()" --> {0x0}
			RECT rcConsole = {}, rcCompWnd = {};
			TODO("DoubleView: нужно с учетом видимых консолией");
			if (isVConExists(0))
			{
				rcConsole = CVConGroup::AllTextRect();
				rcCompWnd = CalcRect(CER_MAIN, rcConsole, CER_CONSOLE_ALL);
				//AutoSizeFont(rcCompWnd, CER_MAIN);
			}
			else
			{
				rcCompWnd = rcWnd; // не менять?
			}
			#endif

			#if 1
			// Выполняем всегда, даже если размер уже соответсвует...
			// Без учета DoubleView/SplitScreen
			RECT rcIdeal = GetIdealRect();
			AutoSizeFont(rcIdeal, CER_MAIN);
			RECT rcConsole = CalcRect(CER_CONSOLE_ALL, rcIdeal, CER_MAIN);
			RECT rcCompWnd = CalcRect(CER_MAIN, rcConsole, CER_CONSOLE_ALL);
			#endif

			// При показе/скрытии табов высота консоли может "прыгать"
			// Ее нужно скорректировать. Поскольку идет реальный ресайз
			// главного окна - OnSize вызовается автоматически
			_ASSERTE(isMainThread());

			CVConGroup::PreReSize(WindowMode, rcCompWnd, CER_MAIN, true);

			//#ifdef _DEBUG
			//DebugStep(L"...Sleeping");
			//Sleep(300);
			//DebugStep(NULL);
			//#endif
			MoveWindow(ghWnd, rcWnd.left, rcWnd.top,
			           (rcCompWnd.right - rcCompWnd.left), (rcCompWnd.bottom - rcCompWnd.top), 1);
		}
		else
		{
			CVConGroup::PreReSize(WindowMode, client, CER_MAINCLIENT, true);
		}

		// Поправить! Может изменилось!
		client = GetGuiClientRect();
	}

#ifdef _DEBUG
	dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE);
#endif
	OnSize(true, isZoomed() ? SIZE_MAXIMIZED : SIZE_RESTORED,
	       client.right, client.bottom);

	if (abCorrect2Ideal)
	{
	}
}

//void CConEmuMain::ResizeChildren()
//{
//	CVirtualConsole* pVCon = gpConEmu->ActiveCon();
//	_ASSERTE(isMainThread());
//	if (!pVCon)
//	{
//		// По идее, не должно вызываться, если консолей вообще нет
//		_ASSERTE(pVCon!=NULL);
//		return;
//	}
//	CVConGuard guard(pVCon);
//
//	//RECT rc = gpConEmu->ConsoleOffsetRect();
//	RECT rcClient; Get ClientRect(ghWnd, &rcClient);
//	bool bTabsShown = gpConEmu->mp_TabBar->IsTabsShown();
//
//	static bool bTabsShownLast, bAlwaysScrollLast;
//	static RECT rcClientLast;
//
//	if (bTabsShownLast == bTabsShown && bAlwaysScrollLast == (gpSet->isAlwaysShowScrollbar == 1))
//	{
//		if (memcmp(&rcClient, &rcClientLast, sizeof(RECT))==0)
//		{
//			#if defined(EXT_GNUC_LOG)
//			char szDbg[255];
//			wsprintfA(szDbg, "  --  CConEmuBack::Resize() - exiting, (%i,%i,%i,%i)==(%i,%i,%i,%i)",
//				rcClient.left, rcClient.top, rcClient.right, rcClient.bottom,
//				rcClientLast.left, rcClientLast.top, rcClientLast.right, rcClientLast.bottom);
//
//			if (gpSetCls->isAdvLogging>1)
//				pVCon->RCon()->LogString(szDbg);
//			#endif
//
//			return; // ничего не менялось
//		}
//	}
//
//	rcClientLast = rcClient;
//	//memmove(&mrc_LastClient, &rcClient, sizeof(RECT)); // сразу запомним
//	bTabsShownLast = bTabsShown;
//	bAlwaysScrollLast = (gpSet->isAlwaysShowScrollbar == 1);
//	//RECT rcScroll; GetWindowRect(mh_WndScroll, &rcScroll);
//	RECT rc = gpConEmu->CalcRect(CER_BACK, rcClient, CER_MAINCLIENT);
//
//
//	#if defined(EXT_GNUC_LOG)
//	char szDbg[255]; wsprintfA(szDbg, "  --  CConEmuBack::Resize() - X=%i, Y=%i, W=%i, H=%i", rc.left, rc.top, 	rc.right - rc.left,	rc.bottom - rc.top);
//
//	if (gpSetCls->isAdvLogging>1)
//		pVCon->RCon()->LogString(szDbg);
//	#endif
//
//	// Это скрытое окно отрисовки. Оно должно соответствовать
//	// размеру виртуальной консоли. На это окно ориентируются плагины!
//	WARNING("DoubleView");
//	rc = gpConEmu->CalcRect(CER_SCROLL, rcClient, CER_MAINCLIENT);
//
//
//	#ifdef _DEBUG
//	if (rc.bottom != rcClient.bottom || rc.right != rcClient.right)
//	{
//		_ASSERTE(rc.bottom == rcClient.bottom && rc.right == rcClient.right);
//	}
//	#endif
//}

// Должна вернуть TRUE, если события мыши не нужно пропускать в консоль
BOOL CConEmuMain::TrackMouse()
{
	BOOL lbCapture = FALSE; // По умолчанию - мышь не перехватывать

	TODO("DoubleView: переделать на обработку в видимых консолях");
	CVConGuard VCon;
	if (GetActiveVCon(&VCon) >= 0)
	{
		if (VCon->TrackMouse())
			lbCapture = TRUE;
	}

	return lbCapture;
}

void CConEmuMain::OnAlwaysShowScrollbar(bool abSync /*= true*/)
{
	CVConGroup::OnAlwaysShowScrollbar(abSync);
}

void CConEmuMain::OnConsoleResize(BOOL abPosted/*=FALSE*/)
{
	TODO("На удаление. ConEmu не должен дергаться при смене размера ИЗ КОНСОЛИ");
	//MSetter lInConsoleResize(&mb_InConsoleResize);
	// Выполняться должно в нити окна, иначе можем повиснуть
	static bool lbPosted = false;
	abPosted = (mn_MainThreadId == GetCurrentThreadId());

	if (!abPosted)
	{
		if (gpSetCls->isAdvLogging)
			LogString("OnConsoleResize(abPosted==false)", TRUE);

		if (!lbPosted)
		{
			lbPosted = true; // чтобы post не накапливались
			//#ifdef _DEBUG
			//int nCurConWidth = (int)CVConGroup::TextWidth();
			//int nCurConHeight = (int)CVConGroup::TextHeight();
			//#endif
			PostMessage(ghWnd, mn_PostConsoleResize, 0,0);
		}

		return;
	}

	lbPosted = false;

	if (isIconic())
	{
		if (gpSetCls->isAdvLogging)
			LogString("OnConsoleResize ignored, because of iconic");

		return; // если минимизировано - ничего не делать
	}

	// Было ли реальное изменение размеров?
	BOOL lbSizingToDo  = (mouse.state & MOUSE_SIZING_TODO) == MOUSE_SIZING_TODO;
	bool lbIsSizing = isSizing();
	bool lbLBtnPressed = isPressed(VK_LBUTTON);

	if (lbIsSizing && !lbLBtnPressed)
	{
		// Сборс всех флагов ресайза мышкой
		ResetSizingFlags(MOUSE_SIZING_BEGIN|MOUSE_SIZING_TODO);
	}

	if (gpSetCls->isAdvLogging)
	{
		char szInfo[160]; wsprintfA(szInfo, "OnConsoleResize: mouse.state=0x%08X, SizingToDo=%i, IsSizing=%i, LBtnPressed=%i, gbPostUpdateWindowSize=%i",
		                            mouse.state, (int)lbSizingToDo, (int)lbIsSizing, (int)lbLBtnPressed, (int)gbPostUpdateWindowSize);
		LogString(szInfo, TRUE);
	}

	CVConGroup::OnConsoleResize(lbSizingToDo);
}

LRESULT CConEmuMain::OnSize(bool bResizeRCon/*=true*/, WPARAM wParam/*=0*/, WORD newClientWidth/*=(WORD)-1*/, WORD newClientHeight/*=(WORD)-1*/)
{
	LRESULT result = 0;
#ifdef _DEBUG
	RECT rcDbgSize; GetWindowRect(ghWnd, &rcDbgSize);
	wchar_t szSize[255]; _wsprintf(szSize, SKIPLEN(countof(szSize)) L"OnSize(%u, %i, %ix%i) Current window size (X=%i, Y=%i, W=%i, H=%i)\n",
	                               (int)bResizeRCon, (DWORD)wParam, (int)(short)newClientWidth, (int)(short)newClientHeight,
	                               rcDbgSize.left, rcDbgSize.top, (rcDbgSize.right-rcDbgSize.left), (rcDbgSize.bottom-rcDbgSize.top));
	DEBUGSTRSIZE(szSize);
#endif

	if (wParam == SIZE_MINIMIZED || isIconic())
	{
		return 0;
	}

	if (mb_IgnoreSizeChange)
	{
		// на время обработки WM_SYSCOMMAND
		return 0;
	}

	if (mn_MainThreadId != GetCurrentThreadId())
	{
		//MBoxAssert(mn_MainThreadId == GetCurrentThreadId());
		PostMessage(ghWnd, WM_SIZE, MAKELPARAM(wParam,(bResizeRCon?1:2)), MAKELONG(newClientWidth,newClientHeight));
		return 0;
	}

	UpdateWindowRgn();

#ifdef _DEBUG
	DWORD_PTR dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE);
#endif
	//if (mb_InResize) {
	//	_ASSERTE(!mb_InResize);
	//	PostMessage(ghWnd, WM_SIZE, wParam, MAKELONG(newClientWidth,newClientHeight));
	//	return 0;
	//}
	mn_InResize++;


	if (newClientWidth==(WORD)-1 || newClientHeight==(WORD)-1)
	{
		RECT rcClient = GetGuiClientRect();
		newClientWidth = rcClient.right;
		newClientHeight = rcClient.bottom;
	}

	//int nClientTop = 0;
	//#if defined(CONEMU_TABBAR_EX)
	//RECT rcFrame = CalcMargins(CEM_CLIENTSHIFT);
	//nClientTop = rcFrame.top;
	//#endif
	//RECT mainClient = MakeRect(0, nClientTop, newClientWidth, newClientHeight+nClientTop);
	RECT mainClient = CalcRect(CER_MAINCLIENT);
	RECT work = CalcRect(CER_WORKSPACE, mainClient, CER_MAINCLIENT);
	_ASSERTE(ghWndWork && GetParent(ghWndWork)==ghWnd); // пока расчитано на дочерний режим
	MoveWindow(ghWndWork, work.left, work.top, work.right-work.left, work.bottom-work.top, TRUE);

	// Запомнить "идеальный" размер окна, выбранный пользователем
	if (isSizing() && isWindowNormal())
	{
		//GetWindowRect(ghWnd, &mrc_Ideal);
		//UpdateIdealRect();
		StoreIdealRect();
	}

	if (gpConEmu->mp_TabBar->IsTabsActive())
		gpConEmu->mp_TabBar->Reposition();

	// Background - должен занять все клиентское место под тулбаром
	// Там же ресайзится ScrollBar
	//ResizeChildren();

	BOOL lbIsPicView = isPictureView();		UNREFERENCED_PARAMETER(lbIsPicView);

	if (bResizeRCon && (changeFromWindowMode == wmNotChanging) && (mn_InResize <= 1))
	{
		CVConGroup::SyncAllConsoles2Window(mainClient, CER_MAINCLIENT, true);
	}

	if (CVConGroup::isVConExists(0))
	{
		CVConGroup::ReSizePanes(mainClient);
	}

	InvalidateGaps();

	if (mn_InResize>0)
		mn_InResize--;

	#ifdef _DEBUG
	dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE);
	#endif
	return result;
}

LRESULT CConEmuMain::OnSizing(WPARAM wParam, LPARAM lParam)
{
	DEBUGSTRSIZE(L"CConEmuMain::OnSizing");

	LRESULT result = true;
#if defined(EXT_GNUC_LOG)
	char szDbg[255];
	wsprintfA(szDbg, "CConEmuMain::OnSizing(wParam=%i, L.Lo=%i, L.Hi=%i)\n",
	          wParam, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));

	if (gpSetCls->isAdvLogging)
		LogString(szDbg);

#endif
#ifndef _DEBUG

	if (isPictureView())
	{
		RECT *pRect = (RECT*)lParam; // с рамкой
		*pRect = mrc_WndPosOnPicView;
		//pRect->right = pRect->left + (mrc_WndPosOnPicView.right-mrc_WndPosOnPicView.left);
		//pRect->bottom = pRect->top + (mrc_WndPosOnPicView.bottom-mrc_WndPosOnPicView.top);
	}
	else
#endif
		if (mb_IgnoreSizeChange)
		{
			// на время обработки WM_SYSCOMMAND
		}
		else if (isNtvdm())
		{
			// не менять для 16бит приложений
		}
		else if (isWindowNormal())
		{
			RECT srctWindow;
			RECT wndSizeRect, restrictRect, calcRect;
			RECT *pRect = (RECT*)lParam; // с рамкой
			RECT rcCurrent; GetWindowRect(ghWnd, &rcCurrent);

			if ((mouse.state & (MOUSE_SIZING_BEGIN|MOUSE_SIZING_TODO))==MOUSE_SIZING_BEGIN
			        && isPressed(VK_LBUTTON))
			{
				SetSizingFlags(MOUSE_SIZING_TODO);
			}

			wndSizeRect = *pRect;
			// Для красивости рамки под мышкой
			LONG nWidth = gpSetCls->FontWidth(), nHeight = gpSetCls->FontHeight();

			if (nWidth && nHeight)
			{
				wndSizeRect.right += (nWidth-1)/2;
				wndSizeRect.bottom += (nHeight-1)/2;
			}

			bool bNeedFixSize = gpSet->isIntegralSize();

			// Рассчитать желаемый размер консоли
			//srctWindow = ConsoleSizeFromWindow(&wndSizeRect, true /* frameIncluded */);
			AutoSizeFont(wndSizeRect, CER_MAIN);
			srctWindow = CalcRect(CER_CONSOLE_ALL, wndSizeRect, CER_MAIN);

			// Минимально допустимые размеры консоли
			if (srctWindow.right < MIN_CON_WIDTH)
			{
				srctWindow.right = MIN_CON_WIDTH;
				bNeedFixSize = true;
			}

			if (srctWindow.bottom < MIN_CON_HEIGHT)
			{
				srctWindow.bottom = MIN_CON_HEIGHT;
				bNeedFixSize = true;
			}


			if (bNeedFixSize)
			{
				calcRect = CalcRect(CER_MAIN, srctWindow, CER_CONSOLE_ALL);
				#ifdef _DEBUG
				RECT rcRev = CalcRect(CER_CONSOLE_ALL, calcRect, CER_MAIN);
				_ASSERTE(rcRev.right==srctWindow.right && rcRev.bottom==srctWindow.bottom);
				#endif
				restrictRect.right = pRect->left + calcRect.right;
				restrictRect.bottom = pRect->top + calcRect.bottom;
				restrictRect.left = pRect->right - calcRect.right;
				restrictRect.top = pRect->bottom - calcRect.bottom;

				switch(wParam)
				{
					case WMSZ_RIGHT:
					case WMSZ_BOTTOM:
					case WMSZ_BOTTOMRIGHT:
						pRect->right = restrictRect.right;
						pRect->bottom = restrictRect.bottom;
						break;
					case WMSZ_LEFT:
					case WMSZ_TOP:
					case WMSZ_TOPLEFT:
						pRect->left = restrictRect.left;
						pRect->top = restrictRect.top;
						break;
					case WMSZ_TOPRIGHT:
						pRect->right = restrictRect.right;
						pRect->top = restrictRect.top;
						break;
					case WMSZ_BOTTOMLEFT:
						pRect->left = restrictRect.left;
						pRect->bottom = restrictRect.bottom;
						break;
				}
			}

			// При смене размера (пока ничего не делаем)
			if ((pRect->right - pRect->left) != (rcCurrent.right - rcCurrent.left)
			        || (pRect->bottom - pRect->top) != (rcCurrent.bottom - rcCurrent.top))
			{
				// Сразу подресайзить консоль, чтобы при WM_PAINT можно было отрисовать уже готовые данные
				TODO("DoubleView");
				//ActiveCon()->RCon()->SyncConsole2Window(FALSE, pRect);
				#ifdef _DEBUG
				wchar_t szSize[255]; _wsprintf(szSize, SKIPLEN(countof(szSize)) L"New window size (X=%i, Y=%i, W=%i, H=%i); Current size (X=%i, Y=%i, W=%i, H=%i)\n",
				                               pRect->left, pRect->top, (pRect->right-pRect->left), (pRect->bottom-pRect->top),
				                               rcCurrent.left, rcCurrent.top, (rcCurrent.right-rcCurrent.left), (rcCurrent.bottom-rcCurrent.top));
				DEBUGSTRSIZE(szSize);
				#endif
			}
		}

	if (gpSetCls->isAdvLogging)
		LogWindowPos(L"OnSizing.end");

	return result;
}

LRESULT CConEmuMain::OnMoving(LPRECT prcWnd /*= NULL*/, bool bWmMove /*= false*/)
{
	if (!gpSet->isSnapToDesktopEdges)
		return FALSE;

	if (isIconic() || isZoomed() || isFullScreen() || gpSet->isQuakeStyle)
		return FALSE;

	HMONITOR hMon;
	if (bWmMove && isPressed(VK_LBUTTON))
	{
		// Если двигаем мышкой - то дать возможность прыгнуть на монитор под мышкой
		POINT ptCur = {}; GetCursorPos(&ptCur);
		hMon = MonitorFromPoint(ptCur, MONITOR_DEFAULTTONEAREST);
	}
	else if (prcWnd)
	{
		hMon = MonitorFromRect(prcWnd, MONITOR_DEFAULTTONEAREST);
	}
	else
	{
		hMon = MonitorFromWindow(ghWnd, MONITOR_DEFAULTTONEAREST);
	}

	MONITORINFO mi = {sizeof(mi)};
	if (!GetMonitorInfo(hMon, &mi))
		return FALSE;

	RECT rcShift = {};
	if (gpSet->isHideCaptionAlways())
	{
		rcShift = CalcMargins(CEM_FRAMECAPTION);
		mi.rcWork.left -= rcShift.left;
		mi.rcWork.top -= rcShift.top;
		mi.rcWork.right += rcShift.right;
		mi.rcWork.bottom += rcShift.bottom;
	}

	RECT rcCur = {};
	if (prcWnd)
		rcCur = *prcWnd;
	else
		GetWindowRect(ghWnd, &rcCur);

	int nWidth = rcCur.right - rcCur.left;
	int nHeight = rcCur.bottom - rcCur.top;

	RECT rcWnd = {};
	rcWnd.left = max(mi.rcWork.left, min(rcCur.left, mi.rcWork.right - nWidth));
	rcWnd.top = max(mi.rcWork.top, min(rcCur.top, mi.rcWork.bottom - nHeight));
	rcWnd.right = min(mi.rcWork.right, rcWnd.left + nWidth);
	rcWnd.bottom = min(mi.rcWork.bottom, rcWnd.top + nHeight);
	
	if (memcmp(&rcWnd, &rcCur, sizeof(rcWnd)) == 0)
		return FALSE;

	if (prcWnd == NULL)
	{
		TODO("Desktop mode?");
		MoveWindow(ghWnd, rcWnd.left, rcWnd.right, nWidth+1, nHeight+1, TRUE);

		if (gpSetCls->isAdvLogging)
			LogWindowPos(L"OnMoving.end");
	}
	else
	{
		*prcWnd = rcWnd;
	}

	return TRUE;
}

void CConEmuMain::CheckTopMostState()
{
	static bool bInCheck = false;
	if (bInCheck)
		return;

	bInCheck = true;

	DWORD dwStyleEx = GetWindowLong(ghWnd, GWL_EXSTYLE);

	if (!gpSet->isAlwaysOnTop && ((dwStyleEx & WS_EX_TOPMOST) == WS_EX_TOPMOST))
	{
		if (gpSetCls->isAdvLogging)
		{
			char szInfo[200];
			RECT rcWnd = {}; GetWindowRect(ghWnd, &rcWnd);
			_wsprintfA(szInfo, SKIPLEN(countof(szInfo))
				"Some external program bring ConEmu OnTop: HWND=x%08X, StyleEx=x%08X, Rect={%i,%i}-{%i,%i}",
				(DWORD)ghWnd, dwStyleEx, rcWnd.left, rcWnd.top, rcWnd.right, rcWnd.bottom);
			LogString(szInfo);
		}

		if (IDYES == MsgBox(L"Some external program bring ConEmu OnTop\nRevert?", MB_SYSTEMMODAL|MB_ICONQUESTION|MB_YESNO))
		{
	        //SetWindowStyleEx(dwStyleEx & ~WS_EX_TOPMOST);
			OnAlwaysOnTop();
		}
		else
		{
			gpSet->isAlwaysOnTop = true;
		}
	}

	bInCheck = false;
}

// Про IntegralSize - смотреть CConEmuMain::OnSizing
LRESULT CConEmuMain::OnWindowPosChanged(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0; // DefWindowProc зовется в середине функции

	// Если нужно поправить параметры DWM
	gpConEmu->ExtendWindowFrame();

	//static int WindowPosStackCount = 0;
	WINDOWPOS *p = (WINDOWPOS*)lParam;
	DWORD dwStyle = GetWindowLong(ghWnd, GWL_STYLE);
	DWORD dwStyleEx = GetWindowLong(ghWnd, GWL_EXSTYLE);

	DEBUGTEST(WINDOWPLACEMENT wpl = {sizeof(wpl)}; GetWindowPlacement(ghWnd, &wpl););
	DEBUGTEST(WINDOWPOS ps = *p);

	if (gpSetCls->isAdvLogging >= 2)
	{
		wchar_t szInfo[255];
		wcscpy_c(szInfo, L"OnWindowPosChanged:");
		if (dwStyle & WS_MAXIMIZE) wcscat_c(szInfo, L" (zoomed)");
		if (dwStyle & WS_MINIMIZE) wcscat_c(szInfo, L" (iconic)");
		size_t cchLen = wcslen(szInfo);
		_wsprintf(szInfo+cchLen, SKIPLEN(countof(szInfo)-cchLen) L" x%08X x%08X (F:x%08X X:%i Y:%i W:%i H:%i)", dwStyle, dwStyleEx, p->flags, p->x, p->y, p->cx, p->cy);
		LogString(szInfo);
	}

	#ifdef _DEBUG
	static int cx, cy;

	if (!(p->flags & SWP_NOSIZE) && (cx != p->cx || cy != p->cy))
	{
		cx = p->cx; cy = p->cy;
	}

	// Отлов неожиданной установки "AlwaysOnTop"
	//static bool bWasTopMost = false;
	//_ASSERTE(((p->flags & SWP_NOZORDER) || (p->hwndInsertAfter!=HWND_TOPMOST)) && "OnWindowPosChanged");
	//_ASSERTE(((dwStyleEx & WS_EX_TOPMOST) == 0) && "OnWindowPosChanged");
	//bWasTopMost = ((dwStyleEx & WS_EX_TOPMOST)==WS_EX_TOPMOST);
	#endif

	wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_WINDOWPOSCHANGED ({%i-%i}x{%i-%i} Flags=0x%08X), style=0x%08X\n", p->x, p->y, p->cx, p->cy, p->flags, dwStyle);
	DEBUGSTRSIZE(szDbg);
	//WindowPosStackCount++;

	
	if (!gpSet->isAlwaysOnTop && ((dwStyleEx & WS_EX_TOPMOST) == WS_EX_TOPMOST))
	{
		CheckTopMostState();
		//_ASSERTE(((dwStyleEx & WS_EX_TOPMOST) == 0) && "Determined TopMost in OnWindowPosChanged");
		//SetWindowStyleEx(dwStyleEx & ~WS_EX_TOPMOST);
	}

	//if (WindowPosStackCount == 1)
	//{
	//	#ifdef _DEBUG
	//	bool bNoMove = (p->flags & SWP_NOMOVE); 
	//	bool bNoSize = (p->flags & SWP_NOSIZE);
	//	#endif

	//	if (gpConEmu->CorrectWindowPos(p))
	//	{
	//		MoveWindow(ghWnd, p->x, p->y, p->cx, p->cy, TRUE);
	//	}
	//}

	DEBUGTEST(WINDOWPOS ps1 = *p);

	// Иначе могут не вызваться события WM_SIZE/WM_MOVE
	result = DefWindowProc(hWnd, uMsg, wParam, lParam);

	DEBUGTEST(WINDOWPOS ps2 = *p);

	//WindowPosStackCount--;

	if (hWnd == ghWnd /*&& ghOpWnd*/)  //2009-05-08 запоминать wndX/wndY всегда, а не только если окно настроек открыто
	{
		if (!gpConEmu->mb_IgnoreSizeChange && !gpConEmu->isIconic())
		{
			RECT rc = gpConEmu->CalcRect(CER_MAIN);
			mp_Status->OnWindowReposition(&rc);

			if ((changeFromWindowMode == wmNotChanging) && isWindowNormal())
			{
				//131023 Otherwise, after tiling (Win+Left) 
				//       and Lose/Get focus (Alt+Tab,Alt+Tab)
				//       StoreNormalRect will be called unexpectedly
				// Don't call Estimate here? Avoid excess calculations?
				ConEmuWindowCommand CurTile = GetTileMode(false/*Estimate*/);
				if (CurTile == cwc_Current)
				{
					StoreNormalRect(&rc);
				}
			}

			if (gpConEmu->hPictureView)
			{
				gpConEmu->mrc_WndPosOnPicView = rc;
			}

			//else
			//{
			//	TODO("Доработать, когда будет ресайз PicView на лету");
			//	if (!(p->flags & SWP_NOSIZE))
			//	{
			//		RECT rcWnd = {0,0,p->cx,p->cy};
			//		ActiveCon()->RCon()->SyncConsole2Window(FALSE, &rcWnd);
			//	}
			//}
		}
		else
		{
			mp_Status->OnWindowReposition(NULL);
		}
	}
	else if (gpConEmu->hPictureView)
	{
		GetWindowRect(ghWnd, &gpConEmu->mrc_WndPosOnPicView);
	}

	// Если окошко сворачивалось кнопками Win+Down (Win7) то SC_MINIMIZE не приходит
	if (gpSet->isMinToTray() && isIconic(true) && IsWindowVisible(ghWnd))
	{
		Icon.HideWindowToTray();
	}

	// Issue 878: ConEmu - Putty: Can't select in putty when ConEmu change display
	if (IsWindowVisible(ghWnd) && !isIconic() && !isSizing())
	{
		CVConGroup::NotifyChildrenWindows();
	}

	//Логирование, что получилось
	if (gpSetCls->isAdvLogging)
	{
		LogWindowPos(L"WM_WINDOWPOSCHANGED.end");
	}

	return result;
}

// Про IntegralSize - смотреть CConEmuMain::OnSizing
LRESULT CConEmuMain::OnWindowPosChanging(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	WINDOWPOS *p = (WINDOWPOS*)lParam;
	DEBUGTEST(WINDOWPOS ps = *p);

	bool zoomed = ::IsZoomed(ghWnd);
	bool iconic = ::IsIconic(ghWnd);

	DWORD dwStyle = GetWindowLong(ghWnd, GWL_STYLE);
	DWORD dwStyleEx = GetWindowLong(ghWnd, GWL_EXSTYLE);

	if (m_JumpMonitor.bInJump)
	{
		if (m_JumpMonitor.bFullScreen || m_JumpMonitor.bMaximized)
			zoomed = true;
	}


	#ifdef _DEBUG
	DEBUGTEST(WINDOWPLACEMENT wpl = {sizeof(wpl)}; GetWindowPlacement(ghWnd, &wpl););
	_ASSERTE(zoomed == ((dwStyle & WS_MAXIMIZE) == WS_MAXIMIZE) || (zoomed && m_JumpMonitor.bInJump));
	_ASSERTE(iconic == ((dwStyle & WS_MINIMIZE) == WS_MINIMIZE));
	#endif

	// В процессе раскрытия/сворачивания могут меняться стили и вызвается ...Changing
	if (!gpSet->isQuakeStyle && (changeFromWindowMode != wmNotChanging))
	{
		if (!zoomed && ((WindowMode == wmMaximized) || (WindowMode == wmFullScreen)))
		{
			zoomed = true;
			// Это может быть, например, в случае смены стилей окна в процессе раскрытия (HideCaption)
			_ASSERTE(changeFromWindowMode==wmNormal || mb_InCreateWindow || iconic);
		}
		else if (zoomed && (WindowMode == wmNormal))
		{
			_ASSERTE((changeFromWindowMode!=wmNormal) && "Must be not zoomed!");
			zoomed = false;
		}
		else if (WindowMode != wmNormal)
		{
			_ASSERTE(zoomed && "Need to check 'zoomed' state");
		}
	}

	wchar_t szInfo[255];
	if (gpSetCls->isAdvLogging >= 2)
	{
		wcscpy_c(szInfo, L"OnWindowPosChanging:");
		if (zoomed) wcscat_c(szInfo, L" (zoomed)");
		if (iconic) wcscat_c(szInfo, L" (iconic)");
		size_t cchLen = wcslen(szInfo);
		_wsprintf(szInfo+cchLen, SKIPLEN(countof(szInfo)-cchLen) L" x%08X x%08X (F:x%08X X:%i Y:%i W:%i H:%i)", dwStyle, dwStyleEx, p->flags, p->x, p->y, p->cx, p->cy);
	}


	#ifdef _DEBUG
	{
		wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_WINDOWPOSCHANGING ({%i-%i}x{%i-%i} Flags=0x%08X) style=0x%08X%s\n", p->x, p->y, p->cx, p->cy, p->flags, dwStyle, zoomed ? L" (zoomed)" : L"");
		DEBUGSTRSIZE(szDbg);
		DWORD nCurTick = GetTickCount();
		static int cx, cy;

		if (!(p->flags & SWP_NOSIZE) && (cx != p->cx || cy != p->cy))
		{
			cx = p->cx; cy = p->cy;
		}

			#ifdef CATCH_TOPMOST_SET
			// Отлов неожиданной установки "AlwaysOnTop"
			static bool bWasTopMost = false;
			_ASSERTE(((p->flags & SWP_NOZORDER) || (p->hwndInsertAfter!=HWND_TOPMOST)) && "OnWindowPosChanging");
			_ASSERTE(((dwStyleEx & WS_EX_TOPMOST) == 0) && "OnWindowPosChanging");
			_ASSERTE((bWasTopMost || gpSet->isAlwaysOnTop || ((dwStyleEx & WS_EX_TOPMOST)==0)) && "TopMost mode detected in OnWindowPosChanging");
			bWasTopMost = ((dwStyleEx & WS_EX_TOPMOST)==WS_EX_TOPMOST);
			#endif
	}
	#endif

	if (m_FixPosAfterStyle)
	{
		DWORD nCurTick = GetTickCount();

		if ((nCurTick - m_FixPosAfterStyle) < FIXPOSAFTERSTYLE_DELTA)
		{
			#ifdef _DEBUG
			RECT rcDbgNow = {}; GetWindowRect(hWnd, &rcDbgNow);
			#endif
			p->flags &= ~(SWP_NOMOVE|SWP_NOSIZE);
			p->x = mrc_FixPosAfterStyle.left;
			p->y = mrc_FixPosAfterStyle.top;
			p->cx = mrc_FixPosAfterStyle.right - mrc_FixPosAfterStyle.left;
			p->cy = mrc_FixPosAfterStyle.bottom - mrc_FixPosAfterStyle.top;
		}
		m_FixPosAfterStyle = 0;
	}

	//120821 - Размер мог быть изменен извне (Win+Up например)
	//120830 - только если не "Minimized"
	if (!iconic
		&& !mb_InCaptionChange && !(p->flags & (SWP_NOSIZE|SWP_NOMOVE))
		&& (changeFromWindowMode == wmNotChanging) && !gpSet->isQuakeStyle)
	{
		// В этом случае нужно проверить, может быть требуется коррекция стилей окна?
		if (zoomed != (WindowMode == wmMaximized || WindowMode == wmFullScreen))
		{
			// В режимах Maximized/FullScreen должен быть WS_MAXIMIZED
			WindowMode = zoomed ? wmMaximized : wmNormal;
			// Установив WindowMode мы вызываем все необходимые действия
			// по смене стилей, обновлении регионов, и т.п.
			OnHideCaption();
		}
	}
	
	// -- // Если у нас режим скрытия заголовка (при максимизации/фулскрине)
	// При любой смене. Т.к. мы меняем WM_GETMINMAXINFO - нужно корректировать и размеры :(
	// Иначе возможны глюки
	if (!(p->flags & (SWP_NOSIZE|SWP_NOMOVE)) && !InMinimizing())
	{
		if (gpSet->isQuakeStyle)
		{
			// Разрешить менять и ширину окна Quake
			//CESize SaveWidth = {WndWidth.Raw};
			//bool tempChanged = false;

			RECT rc = GetDefaultRect();
			
			p->y = rc.top;

			TODO("разрешить изменение ширины в Quake!");
#if 0
			ConEmuWindowMode wmCurQuake = GetWindowMode();

			if (isSizing() && p->cx && (wmCurQuake == wmNormal))
			{
				// Поскольку оно центрируется (wndCascade)... изменение ширину нужно множить на 2
				RECT rcQNow = {}; GetWindowRect(hWnd, &rcQNow);
				int width = rcQNow.right - rcQNow.left;
				int shift = p->cx - width;
				// TODO: разрешить изменение ширины в Quake!
				if (shift)
				{
					//_ASSERTE(shift==0);
					#ifndef _DEBUG
					shift = 0;
					#endif
				}
				//WndWidth.Set(true, ss_Pixels, width + 2*shift);
				if (gpSet->wndCascade)
				{
					// Скорректировать координату!
					wndX = rc.left - shift;
					
					p->x -= shift;
					p->cx += shift;
				}
				else
				{
					//p->cx = rc.right - rc.left + shift*(gpSet->wndCascade ? 2 : 1); // + 1;
				}
			}
			else
#endif
			{
				p->x = rc.left;
				p->cx = rc.right - rc.left; // + 1;
			}


			//// Вернуть WndWidth, т.к. это было временно
			//if (tempChanged)
			//{
			//	WndWidth.Set(true, SaveWidth.Style, SaveWidth.Value);
			//}
		}
		else if (zoomed)
		{
			// Здесь может быть попытка перемещения на другой монитор. Нужно обрабатывать x/y/cx/cy!
			RECT rcNewMain = {p->x, p->y, p->x + p->cx, p->y + p->cy};
			RECT rc = CalcRect((WindowMode == wmFullScreen) ? CER_FULLSCREEN : CER_MAXIMIZED, rcNewMain, CER_MAIN);
			p->x = rc.left;
			p->y = rc.top;
			p->cx = rc.right - rc.left;
			p->cy = rc.bottom - rc.top;
		}
		else // normal only
		{
			_ASSERTE(GetWindowMode() == wmNormal);
			RECT rc = {p->x, p->y, p->x + p->cx, p->y + p->cy};
			if (FixWindowRect(rc, CEB_ALL|CEB_ALLOW_PARTIAL))
			{
				p->x = rc.left;
				p->y = rc.top;
				p->cx = rc.right - rc.left;
				p->cy = rc.bottom - rc.top;
			}
		}
	//#ifdef _DEBUG
	#if 0
		else if (isWindowNormal())
		{
			HMONITOR hMon = NULL;
			RECT rcNew = {}; GetWindowRect(ghWnd, &rcNew);
			if (!(p->flags & SWP_NOSIZE) && !(p->flags & SWP_NOMOVE))
			{
				// Меняются и размер и положение
				rcNew = MakeRect(p->x, p->y, p->x+p->cx, p->y+p->cy);
			}
			else if (!(p->flags & SWP_NOSIZE))
			{
				// Меняется только размер
				rcNew.right = rcNew.left + p->cx;
				rcNew.bottom = rcNew.top + p->cy;
			}
			else
			{
				// Меняется только положение
				rcNew = MakeRect(p->x, p->y, rcNew.right-rcNew.left, rcNew.bottom-rcNew.top);
			}

			hMon = MonitorFromRect(&rcNew, MONITOR_DEFAULTTONEAREST);
			MONITORINFO mi = {sizeof(mi)};
			if (GetMonitorInfo(hMon, &mi))
			{
				//if (WindowMode == wmFullScreen)
				//{
				//	p->x = mi.rcMonitor.left;
				//	p->y = mi.rcMonitor.top;
				//	p->cx = mi.rcMonitor.right-mi.rcMonitor.left;
				//	p->cy = mi.rcMonitor.bottom-mi.rcMonitor.top;
				//}
				//else
				//{
				//	p->x = mi.rcWork.left;
				//	p->y = mi.rcWork.top;
				//	p->cx = mi.rcWork.right-mi.rcWork.left;
				//	p->cy = mi.rcWork.bottom-mi.rcWork.top;
				//}
			}
			else
			{
				_ASSERTE(FALSE && "GetMonitorInfo failed");
			}

			//// Нужно скорректировать размеры, а то при смене разрешения монитора (в частности при повороте экрана) глюки лезут
			//p->flags |= (SWP_NOSIZE|SWP_NOMOVE);
			//// И обновить размер насильно
			//SetWindowMode(wmMaximized, TRUE);
		}
	#endif
	}

	//if (gpSet->isDontMinimize) {
	//	if ((p->flags & (0x8000|SWP_NOACTIVATE)) == (0x8000|SWP_NOACTIVATE)
	//		|| ((p->flags & (SWP_NOMOVE|SWP_NOSIZE)) == 0 && p->x < -30000 && p->y < -30000 )
	//		)
	//	{
	//		p->flags = SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE;
	//		p->hwndInsertAfter = HWND_BOTTOM;
	//		result = 0;
	//		if ((dwStyle & WS_MINIMIZE) == WS_MINIMIZE) {
	//			dwStyle &= ~WS_MINIMIZE;
	//			SetWindowStyle(dwStyle);
	//			gpConEmu->InvalidateAll();
	//		}
	//		break;
	//	}
	//}

	if (!(p->flags & SWP_NOSIZE)
	        && (hWnd == ghWnd) && !gpConEmu->mb_IgnoreSizeChange
	        && isWindowNormal())
	{
		if (!hPictureView)
		{
			TODO("Доработать, когда будет ресайз PicView на лету");
			RECT rcWnd = {0,0,p->cx,p->cy};
			CVConGroup::SyncAllConsoles2Window(rcWnd, CER_MAIN, true);
			//CVConGuard VCon;
			//CVirtualConsole* pVCon = (GetActiveVCon(&VCon) >= 0) ? VCon.VCon() : NULL;

			//if (pVCon && pVCon->RCon())
			//	pVCon->RCon()->SyncConsole2Window(FALSE, &rcWnd);
		}
	}

#if 0
	if (!(p->flags & SWP_NOMOVE) && gpSet->isQuakeStyle && (p->y > -30))
	{
		int nDbg = 0;
	}
#endif

	/*
	-- DWM, Glass --
	dwmm.cxLeftWidth = 0;
	dwmm.cxRightWidth = 0;
	dwmm.cyTopHeight = kClientRectTopOffset;
	dwmm.cyBottomHeight = 0;
	DwmExtendFrameIntoClientArea(hwnd, &dwmm);
	-- DWM, Glass --
	*/
	// Иначе не вызвутся события WM_SIZE/WM_MOVE
	result = DefWindowProc(hWnd, uMsg, wParam, lParam);
	p = (WINDOWPOS*)lParam;

	if (gpSetCls->isAdvLogging >= 2)
	{
		size_t cchLen = wcslen(szInfo);
		_wsprintf(szInfo+cchLen, SKIPLEN(countof(szInfo)-cchLen) L" --> (F:x%08X X:%i Y:%i W:%i H:%i)", p->flags, p->x, p->y, p->cx, p->cy);
		LogString(szInfo);
	}

	return result;
}

LRESULT CConEmuMain::OnQueryEndSession(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	wchar_t szSession[128];
	_wsprintf(szSession, SKIPLEN(countof(szSession)) L"Session End:%s%s%s\r\n",
		(lParam & ENDSESSION_CLOSEAPP) ? L" ENDSESSION_CLOSEAPP" : L"",
		(lParam & ENDSESSION_CRITICAL) ? L" ENDSESSION_CRITICAL" : L"",
		(lParam & ENDSESSION_LOGOFF)   ? L" ENDSESSION_LOGOFF" : L"");
	LogString(szSession, true, false);
	DEBUGSTRSESS(szSession);

	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CConEmuMain::OnSessionChanged(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	session.SessionChanged(wParam, lParam);

	wchar_t szInfo[128], szState[32];
	switch (LOWORD(wParam))
	{
		//0x1 The session identified by lParam was connected to the console terminal or RemoteFX session.
		case WTS_CONSOLE_CONNECT: wcscpy_c(szState, L"WTS_CONSOLE_CONNECT"); break;
		//0x2 The session identified by lParam was disconnected from the console terminal or RemoteFX session.
		case WTS_CONSOLE_DISCONNECT: wcscpy_c(szState, L"WTS_CONSOLE_DISCONNECT"); break;
		//0x3 The session identified by lParam was connected to the remote terminal.
		case WTS_REMOTE_CONNECT: wcscpy_c(szState, L"WTS_REMOTE_CONNECT"); break;
		//0x4 The session identified by lParam was disconnected from the remote terminal.
		case WTS_REMOTE_DISCONNECT: wcscpy_c(szState, L"WTS_REMOTE_DISCONNECT"); break;
		//0x5 A user has logged on to the session identified by lParam.
		case WTS_SESSION_LOGON: wcscpy_c(szState, L"WTS_SESSION_LOGON"); break;
		//0x6 A user has logged off the session identified by lParam.
		case WTS_SESSION_LOGOFF: wcscpy_c(szState, L"WTS_SESSION_LOGOFF"); break;
		//0x7 The session identified by lParam has been locked.
		case WTS_SESSION_LOCK: wcscpy_c(szState, L"WTS_SESSION_LOCK"); break;
		//0x8 The session identified by lParam has been unlocked.
		case WTS_SESSION_UNLOCK: wcscpy_c(szState, L"WTS_SESSION_UNLOCK"); break;
		//0x9 The session identified by lParam has changed its remote controlled status. To determine the status, call GetSystemMetrics and check the SM_REMOTECONTROL metric.
		case WTS_SESSION_REMOTE_CONTROL: wcscpy_c(szState, L"WTS_SESSION_REMOTE_CONTROL"); break;
		//0xA Reserved for future use.
		case 0xA/*WTS_SESSION_CREATE*/: wcscpy_c(szState, L"WTS_SESSION_CREATE"); break;
		//0xB Reserved for future use.
		case 0xB/*WTS_SESSION_TERMINATE*/: wcscpy_c(szState, L"WTS_SESSION_TERMINATE"); break;
		default: _wsprintf(szState, SKIPLEN(countof(szState)) L"x%08X", (DWORD)wParam);
	}
	_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"Session State (#%i): %s\r\n", (int)lParam, szState);
	LogString(szInfo, true, false);
	DEBUGSTRSESS(szInfo);

	return 0; // Return value ignored
}

void CConEmuMain::OnSizePanels(COORD cr)
{
	INPUT_RECORD r;
	int nRepeat = 0;
	wchar_t szKey[32];
	bool bShifted = (mouse.state & MOUSE_DRAGPANEL_SHIFT) && isPressed(VK_SHIFT);
	CRealConsole* pRCon = NULL;
	CVConGuard VCon;
	if (GetActiveVCon(&VCon) >= 0)
		pRCon = VCon->RCon();

	if (!pRCon)
	{
		mouse.state &= ~MOUSE_DRAGPANEL_ALL;
		return; // Некорректно, консоли нет
	}

	int nConWidth = pRCon->TextWidth();
	// Поскольку реакция на CtrlLeft/Right... появляется с задержкой - то
	// получение rcPanel - просто для проверки ее наличия!
	{
		RECT rcPanel;

		if (!pRCon->GetPanelRect((mouse.state & (MOUSE_DRAGPANEL_RIGHT|MOUSE_DRAGPANEL_SPLIT)), &rcPanel, TRUE))
		{
			// Во время изменения размера панелей соответствующий Rect может быть сброшен?
#ifdef _DEBUG
			if (mouse.state & MOUSE_DRAGPANEL_SPLIT)
			{
				DEBUGSTRPANEL2(L"PanelDrag: Skip of NO right panel\n");
			}
			else
			{
				DEBUGSTRPANEL2((mouse.state & MOUSE_DRAGPANEL_RIGHT) ? L"PanelDrag: Skip of NO right panel\n" : L"PanelDrag: Skip of NO left panel\n");
			}

#endif
			return;
		}
	}
	r.EventType = KEY_EVENT;
	r.Event.KeyEvent.dwControlKeyState = 0x128; // Потом добавить SHIFT_PRESSED, если нужно...
	r.Event.KeyEvent.wVirtualKeyCode = 0;

	// Сразу запомним изменение положения, чтобы не "колбасило" туда-сюда,
	// пока фар не отработает изменение положения

	if (mouse.state & MOUSE_DRAGPANEL_SPLIT)
	{
		//FAR BUGBUG: При наличии часов в заголовке консоли и нечетной ширине окна
		// слетает заголовок правой панели, если она уже 11 символов. Поставим минимум 12
		if (cr.X >= (nConWidth-13))
			cr.X = max((nConWidth-12),mouse.LClkCon.X);

		//rcPanel.left = mouse.LClkCon.X; -- делал для макро
		mouse.LClkCon.Y = cr.Y;

		if (cr.X < mouse.LClkCon.X)
		{
			r.Event.KeyEvent.wVirtualKeyCode = VK_LEFT;
			nRepeat = mouse.LClkCon.X - cr.X;
			mouse.LClkCon.X = cr.X; // max(cr.X, (mouse.LClkCon.X-1));
			wcscpy(szKey, L"CtrlLeft");
		}
		else if (cr.X > mouse.LClkCon.X)
		{
			r.Event.KeyEvent.wVirtualKeyCode = VK_RIGHT;
			nRepeat = cr.X - mouse.LClkCon.X;
			mouse.LClkCon.X = cr.X; // min(cr.X, (mouse.LClkCon.X+1));
			wcscpy(szKey, L"CtrlRight");
		}
	}
	else
	{
		//rcPanel.bottom = mouse.LClkCon.Y; -- делал для макро
		mouse.LClkCon.X = cr.X;

		if (cr.Y < mouse.LClkCon.Y)
		{
			r.Event.KeyEvent.wVirtualKeyCode = VK_UP;
			nRepeat = mouse.LClkCon.Y - cr.Y;
			mouse.LClkCon.Y = cr.Y; // max(cr.Y, (mouse.LClkCon.Y-1));
			wcscpy(szKey, bShifted ? L"CtrlShiftUp" : L"CtrlUp");
		}
		else if (cr.Y > mouse.LClkCon.Y)
		{
			r.Event.KeyEvent.wVirtualKeyCode = VK_DOWN;
			nRepeat = cr.Y - mouse.LClkCon.Y;
			mouse.LClkCon.Y = cr.Y; // min(cr.Y, (mouse.LClkCon.Y+1));
			wcscpy(szKey, bShifted ? L"CtrlShiftDown" : L"CtrlDown");
		}

		if (bShifted)
		{
			// Тогда разделение на левую и правую панели
			TODO("Активировать нужную, иначе будет меняться размер активной панели, а хотели другую...");
			r.Event.KeyEvent.dwControlKeyState |= SHIFT_PRESSED;
		}
	}

	if (r.Event.KeyEvent.wVirtualKeyCode)
	{
		// Макрос вызывается после отпускания кнопки мышки
		if (gpSet->isDragPanel == 2)
		{
			if (pRCon->isFar(TRUE))
			{
				mouse.LClkCon = cr;
				wchar_t szMacro[128]; szMacro[0] = L'@';

				if (pRCon->IsFarLua())
				{
					// "$Rep (%i)" -> "for RCounter= %i,1,-1 do Keys(\"%s\") end"
					if (nRepeat > 1)
						wsprintf(szMacro+1, L"for RCounter= %i,1,-1 do Keys(\"%s\") end", nRepeat, szKey);
					else
						wsprintf(szMacro+1, L"Keys(\"%s\")", szKey);
				}
				else
				{
					if (nRepeat > 1)
						wsprintf(szMacro+1, L"$Rep (%i) %s $End", nRepeat, szKey);
					else
						wcscpy(szMacro+1, szKey);
				}

				pRCon->PostMacro(szMacro);
			}
		}
		else
		{
#ifdef _DEBUG
			wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"PanelDrag: Sending '%s'\n", szKey);
			DEBUGSTRPANEL(szDbg);
#endif
			// Макросом не будем - велика вероятность второго вызова, когда еще не закончилась обработка первого макро
			// Поехали
			r.Event.KeyEvent.wVirtualScanCode = MapVirtualKey(r.Event.KeyEvent.wVirtualKeyCode, 0/*MAPVK_VK_TO_VSC*/);
			r.Event.KeyEvent.wRepeatCount = nRepeat; //-- repeat - что-то глючит...
			//while (nRepeat-- > 0)
			{
				r.Event.KeyEvent.bKeyDown = TRUE;
				pRCon->PostConsoleEvent(&r);
				r.Event.KeyEvent.bKeyDown = FALSE;
				r.Event.KeyEvent.wRepeatCount = 1;
				r.Event.KeyEvent.dwControlKeyState = 0x120; // "Отожмем Ctrl|Shift"
				pRCon->PostConsoleEvent(&r);
			}
		}
	}
	else
	{
		DEBUGSTRPANEL2(L"PanelDrag: Skip of NO key selected\n");
	}
}








/* ****************************************************** */
/*                                                        */
/*                  System Routines                       */
/*                                                        */
/* ****************************************************** */

#ifdef colorer_func
{
	System_Routines() {};
}
#endif

void CConEmuMain::OnActiveConWndStore(HWND hConWnd)
{
	// Для удобства внешних программ, плагинов и прочая
	// Запоминаем в UserData дескриптор "активного" окна консоли
	SetWindowLongPtr(ghWnd, GWLP_USERDATA, (LONG_PTR)hConWnd);

	// ghWndWork - планируется как Holder для виртуальных консолей,
	// причем он может быть как дочерним для ghWnd, так и отдельно
	// висящим, и делающим вид, что оно дочернее для ghWnd.
	if (ghWndWork && (ghWndWork != ghWnd))
	{	
		SetWindowLongPtr(ghWndWork, GWLP_USERDATA, (LONG_PTR)hConWnd);
	}

	UpdateGuiInfoMappingActive(isMeForeground(true,true));
}

BOOL CConEmuMain::Activate(CVirtualConsole* apVCon)
{
	BOOL lbRc;
	if (!isMainThread())
	{
		LRESULT lRc;
		if (apVCon->GuiWnd())
		{
			// We can lock, if Activate VCon is called from ChildGui.
			// That's why - PostMessage instead of SendMessage
			PostMessage(ghWnd, mn_MsgActivateVCon, 0, (LPARAM)apVCon);
			lRc = 0;
		}
		else
		{
			lRc = SendMessage(ghWnd, mn_MsgActivateVCon, 0, (LPARAM)apVCon);
		}
		lbRc = (lRc == (LRESULT)apVCon);
	}
	else
	{
		lbRc = CVConGroup::Activate(apVCon);
	}
	return lbRc;
}

void CConEmuMain::MoveActiveTab(CVirtualConsole* apVCon, bool bLeftward)
{
	CVConGroup::MoveActiveTab(apVCon, bLeftward);
}

//CVirtualConsole* CConEmuMain::ActiveCon()
//{
//	WARNING("На удаление");
//	CVConGuard VCon;
//	if (CVConGroup::GetActiveVCon(&VCon) < 0)
//		return NULL;
//	return VCon.VCon();
//	/*if (mn_ActiveCon >= countof(mp_VCon))
//	    mn_ActiveCon = -1;
//	if (mn_ActiveCon < 0)
//	    return NULL;
//	return mp_VCon[mn_ActiveCon];*/
//}

// 0 - based
int CConEmuMain::ActiveConNum()
{
	return CVConGroup::ActiveConNum();
}

int CConEmuMain::GetConCount()
{
	return CVConGroup::GetConCount();
}

void CConEmuMain::AttachToDialog()
{
	if (!mp_AttachDlg)
		mp_AttachDlg = new CAttachDlg;
	mp_AttachDlg->AttachDlg();
}

CRealConsole* CConEmuMain::AttachRequestedGui(LPCWSTR asAppFileName, DWORD anAppPID)
{
	wchar_t szLogInfo[MAX_PATH];

	if (gpSetCls->isAdvLogging!=0)
	{
		_wsprintf(szLogInfo, SKIPLEN(countof(szLogInfo)) L"AttachRequestedGui. AppPID=%u, FileName=", anAppPID);
		lstrcpyn(szLogInfo+_tcslen(szLogInfo), asAppFileName ? asAppFileName : L"<NULL>", 128);
		LogString(szLogInfo);
	}

	CRealConsole* pRCon = CVConGroup::AttachRequestedGui(asAppFileName, anAppPID);

	if (gpSetCls->isAdvLogging!=0)
	{
		wchar_t szRc[64];
		if (pRCon)
			_wsprintf(szRc, SKIPLEN(countof(szRc)) L"Succeeded. ServerPID=%u", pRCon->GetServerPID());
		else
			wcscpy_c(szRc, L"Rejected");
		_wsprintf(szLogInfo, SKIPLEN(countof(szLogInfo)) L"AttachRequestedGui. AppPID=%u. %s", anAppPID, szRc);
		LogString(szLogInfo);
	}

	return pRCon;
}

// Вернуть общее количество процессов по всем консолям
DWORD CConEmuMain::CheckProcesses()
{
	return CVConGroup::CheckProcesses();
}

bool CConEmuMain::ConActivateNext(BOOL abNext)
{
	return CVConGroup::ConActivateNext(abNext);
}

// nCon - zero-based index of console
bool CConEmuMain::ConActivate(int nCon)
{
	return CVConGroup::ConActivate(nCon);
}

// args должен быть выделен через "new"
// по завершении - на него будет вызван "delete"
void CConEmuMain::PostCreateCon(RConStartArgs *pArgs)
{
	_ASSERTE((pArgs->pszStartupDir == NULL) || (*pArgs->pszStartupDir != 0));
	PostMessage(ghWnd, mn_MsgCreateCon, mn_MsgCreateCon+1, (LPARAM)pArgs);
}

bool CConEmuMain::CreateWnd(RConStartArgs *args)
{
	if (!args || !args->pszSpecialCmd || !*args->pszSpecialCmd)
	{
		_ASSERTE(args && args->pszSpecialCmd && *args->pszSpecialCmd);
		return false;
	}

	_ASSERTE(args->aRecreate == cra_CreateWindow);

	BOOL bStart = FALSE;

	// Start new ConEmu.exe process with choosen arguments...
	STARTUPINFO si = {sizeof(si)};
	PROCESS_INFORMATION pi = {};
	wchar_t* pszCmdLine = NULL;
	LPCWSTR pszConfig = gpSetCls->GetConfigName();
	size_t cchMaxLen = _tcslen(ms_ConEmuExe)
		+ _tcslen(args->pszSpecialCmd)
		+ (pszConfig ? (_tcslen(pszConfig) + 32) : 0)
		+ 140; // на всякие флажки и -new_console
	if ((pszCmdLine = (wchar_t*)malloc(cchMaxLen*sizeof(*pszCmdLine))) == NULL)
	{
		_ASSERTE(pszCmdLine);
	}
	else
	{
		pszCmdLine[0] = L'"'; pszCmdLine[1] = 0;
		_wcscat_c(pszCmdLine, cchMaxLen, ms_ConEmuExe);
		_wcscat_c(pszCmdLine, cchMaxLen, L"\" ");
		if (pszConfig && *pszConfig)
		{
			_wcscat_c(pszCmdLine, cchMaxLen, L"/config \"");
			_wcscat_c(pszCmdLine, cchMaxLen, pszConfig);
			_wcscat_c(pszCmdLine, cchMaxLen, L"\" ");
		}
		_wcscat_c(pszCmdLine, cchMaxLen, L"/nosingle ");
		_wcscat_c(pszCmdLine, cchMaxLen, L"/cmd ");
		_wcscat_c(pszCmdLine, cchMaxLen, args->pszSpecialCmd);
		if ((args->RunAsAdministrator == crb_On) || (args->RunAsRestricted == crb_On) || args->pszUserName)
		{
			if (args->RunAsAdministrator == crb_On)
			{
				// Create ConEmu.exe with current credentials, implying elevation for the console
				_wcscat_c(pszCmdLine, cchMaxLen, L" -new_console:a");
			}
			else if (args->RunAsRestricted == crb_On)
			{
				_wcscat_c(pszCmdLine, cchMaxLen, L" -new_console:r");
			}
			//else if (args->pszUserName)
			//{
			//	_wcscat_c(pszCmdLine, cchMaxLen, L" -new_console:u:");
			//	_wcscat_c(pszCmdLine, cchMaxLen, args->pszUserName);
			//}
		}

		if ((args->RunAsAdministrator != crb_On) && (args->RunAsRestricted != crb_On) && (args->pszUserName && *args->pszUserName))
			bStart = CreateProcessWithLogonW(args->pszUserName, args->pszDomain, args->szUserPassword,
		                           LOGON_WITH_PROFILE, NULL, pszCmdLine,
		                           NORMAL_PRIORITY_CLASS|CREATE_DEFAULT_ERROR_MODE
		                           , NULL, args->pszStartupDir, &si, &pi);
		else
			bStart = CreateProcess(NULL, pszCmdLine, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, args->pszStartupDir, &si, &pi);

		if (!bStart)
		{
			DWORD nErr = GetLastError();
			DisplayLastError(pszCmdLine, nErr, MB_ICONSTOP, L"Failed to start new ConEmu window");
		}
		else
		{
			SafeCloseHandle(pi.hProcess);
			SafeCloseHandle(pi.hThread);
		}
	}

	return (bStart != FALSE);
}

CVirtualConsole* CConEmuMain::CreateCon(RConStartArgs *args, bool abAllowScripts /*= false*/, bool abForceCurConsole /*= false*/)
{
	_ASSERTE(args!=NULL);
	if (!gpConEmu->isMainThread())
	{
		// Создание VCon в фоновых потоках не допускается, т.к. здесь создаются HWND
		MBoxAssert(gpConEmu->isMainThread());
		return NULL;
	}

	CVirtualConsole* pVCon = CVConGroup::CreateCon(args, abAllowScripts, abForceCurConsole);

	return pVCon;
}

LPCWSTR CConEmuMain::ParseScriptLineOptions(LPCWSTR apszLine, bool* rpbAsAdmin, bool* rpbSetActive, size_t cchNameMax, wchar_t* rsName/*[MAX_RENAME_TAB_LEN]*/)
{
	if (!apszLine)
	{
		_ASSERTE(apszLine!=NULL);
		return NULL;
	}

	// !!! Don't Reset values (rpbSetActive, rpbAsAdmin, rsName)
	// !!! They may be defined in the other place

	// Go
	while (*apszLine == L'>' || *apszLine == L'*' /*|| *apszLine == L'?'*/ || *apszLine == L' ' || *apszLine == L'\t')
	{
		if (*apszLine == L'>' && rpbSetActive)
			*rpbSetActive = true;

		if (*apszLine == L'*' && rpbAsAdmin)
			*rpbAsAdmin = true;

		//if (*apszLine == L'?')
		//{
		//	LPCWSTR pszStart = apszLine+1;
		//	LPCWSTR pszEnd = wcschr(pszStart, L'?');
		//	if (!pszEnd) pszEnd = pszStart + _tcslen(pszStart);

		//	if (rsName && (pszEnd > pszStart) && (cchNameMax > 1))
		//	{
		//		UINT nLen = min((INT_PTR)cchNameMax, (pszEnd-pszStart+1));
		//		lstrcpyn(rsName, pszStart, nLen);
		//	}

		//	apszLine = pszEnd;
		//	if (!*pszEnd)
		//		break;
		//}

		apszLine++;
	}

	return apszLine;
}

// Возвращает указатель на АКТИВНУЮ консоль (при создании группы)
// apszScript содержит строки команд, разделенные \r\n
CVirtualConsole* CConEmuMain::CreateConGroup(LPCWSTR apszScript, bool abForceAsAdmin /*= false*/, LPCWSTR asStartupDir /*= NULL*/, const RConStartArgs *apDefArgs /*= NULL*/)
{
	CVirtualConsole* pVConResult = NULL;
	// Поехали
	wchar_t *pszDataW = lstrdup(apszScript);
	wchar_t *pszLine = pszDataW;
	wchar_t *pszNewLine = wcschr(pszLine, L'\n');
	CVirtualConsole *pSetActive = NULL, *pVCon = NULL, *pLastVCon = NULL;
	bool lbSetActive = false, lbOneCreated = false, lbRunAdmin = false;

	CVConGroup::OnCreateGroupBegin();

	while (*pszLine)
	{
		lbSetActive = false;
		lbRunAdmin = abForceAsAdmin;

		// Task pre-options, for example ">*cmd"
		pszLine = (wchar_t*)ParseScriptLineOptions(pszLine, &lbRunAdmin, &lbSetActive);

		if (pszNewLine)
		{
			*pszNewLine = 0;
			if ((pszNewLine > pszDataW) && (*(pszNewLine-1) == L'\r'))
				*(pszNewLine-1) = 0;
		}

		while (*pszLine == L' ') pszLine++;

		if (*pszLine)
		{
			while (pszLine[0] == L'/')
			{
				if (CSTR_EQUAL == CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE|SORT_STRINGSORT,
				                               pszLine, 14, L"/bufferheight ", 14))
				{
					pszLine += 14;

					while(*pszLine == L' ') pszLine++;

					wchar_t* pszEnd = NULL;
					long lBufHeight = wcstol(pszLine, &pszEnd, 10);
					gpSetCls->SetArgBufferHeight(lBufHeight);

					if (pszEnd) pszLine = pszEnd;
				}

				TODO("Когда появится ключ /mouse - добавить сюда обработку");

				if (CSTR_EQUAL == CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE|SORT_STRINGSORT,
				                               pszLine, 5, L"/cmd ", 5))
				{
					pszLine += 5;
				}

				while (*pszLine == L' ') pszLine++;
			}

			if (*pszLine)
			{
				RConStartArgs args;
				if (apDefArgs)
					args.AssignFrom(apDefArgs);

				args.pszSpecialCmd = lstrdup(pszLine);

				if (apDefArgs && apDefArgs->pszStartupDir && *apDefArgs->pszStartupDir)
					args.pszStartupDir = lstrdup(apDefArgs->pszStartupDir);
				else if (asStartupDir && *asStartupDir)
					args.pszStartupDir = lstrdup(asStartupDir);
				else
					SafeFree(args.pszStartupDir);

				if (lbRunAdmin) // don't reset one that may come from apDefArgs
					args.RunAsAdministrator = crb_On;

				pVCon = CreateCon(&args, false, true);

				if (!pVCon)
				{
					DisplayLastError(L"Can't create new virtual console!");

					if (!lbOneCreated)
					{
						//Destroy(); -- должна вызывающая функция
						goto wrap;
					}
				}
				else
				{
					lbOneCreated = TRUE;

					const RConStartArgs& modArgs = pVCon->RCon()->GetArgs();
					if (modArgs.ForegroungTab == crb_On)
						lbSetActive = true;

					pLastVCon = pVCon;
					if (lbSetActive && !pSetActive)
						pSetActive = pVCon;

					if (GetVCon((int)MAX_CONSOLE_COUNT-1))
						break; // Больше создать не получится
				}
			}

			// При создании группы консолей требуется обрабатывать сообщения,
			// иначе может возникнуть блокировка запускаемого сервера
			MSG Msg;
			while (PeekMessage(&Msg,0,0,0,PM_REMOVE))
			{
				ConEmuMsgLogger::Log(Msg, ConEmuMsgLogger::msgCommon);

				if (Msg.message == WM_QUIT)
					goto wrap;

				BOOL lbDlgMsg = isDialogMessage(Msg);
				if (!lbDlgMsg)
				{
					TranslateMessage(&Msg);
					DispatchMessage(&Msg);
				}
			}

			if (!ghWnd || !IsWindow(ghWnd))
				goto wrap;
		}

		if (!pszNewLine) break;

		pszLine = pszNewLine+1;

		if (!*pszLine) break;

		while ((*pszLine == L'\r') || (*pszLine == L'\n'))
			pszLine++; // пропустить все переводы строк

		pszNewLine = wcschr(pszLine, L'\n');
	}

	if (pSetActive)
	{
		Activate(pSetActive);
	}

	pVConResult = (pSetActive ? pSetActive : pLastVCon);
wrap:
	SafeFree(pszDataW);
	CVConGroup::OnCreateGroupEnd();
	return pVConResult;
}

void CConEmuMain::CreateGhostVCon(CVirtualConsole* apVCon)
{
	PostMessage(ghWnd, mn_MsgInitVConGhost, 0, (LPARAM)apVCon);
}

void CConEmuMain::UpdateActiveGhost(CVirtualConsole* apVCon)
{
	_ASSERTE(isActive(apVCon));
	if (mh_LLKeyHookDll && mph_HookedGhostWnd)
	{
		// Win7 и выше!
		if (IsWindows7 || !gpSet->isTabsOnTaskBar())
			*mph_HookedGhostWnd = NULL; //ghWndApp ? ghWndApp : ghWnd;
		else
			*mph_HookedGhostWnd = apVCon->GhostWnd();
	}
}

// Послать во все активные фары CMD_FARSETCHANGED
// Обновляются настройки: gpSet->isFARuseASCIIsort, gpSet->isShellNoZoneCheck;
void CConEmuMain::UpdateFarSettings()
{
	CVConGroup::OnUpdateFarSettings();
}

//void CConEmuMain::UpdateIdealRect(BOOL abAllowUseConSize/*=FALSE*/)
//{
//	// Запомнить "идеальный" размер окна, выбранный пользователем
//	if (isWindowNormal())
//	{
//		RECT rcWnd = {};
//		GetWindowRect(ghWnd, &rcWnd);
//		UpdateIdealRect(rcWnd);
//	}
//	else if (abAllowUseConSize)
//	{
//		//if (isVConExists(0))
//		//	return;
//
//		//RECT rcCon = CVConGroup::AllTextRect();
//		//RECT rcCon = CalcRect(CER_CONSOLE_ALL);
//		RECT rcCon = MakeRect(gpConEmu->wndWidth, gpConEmu->wndHeight);
//		RECT rcWnd = CalcRect(CER_MAIN, rcCon, CER_CONSOLE_ALL);
//		UpdateIdealRect(rcWnd);
//	}
//}

void CConEmuMain::UpdateInsideRect(RECT rcNewPos)
{
	RECT rcWnd = rcNewPos;

	gpConEmu->UpdateIdealRect(rcWnd);
	
	// Подвинуть
	SetWindowPos(ghWnd, HWND_TOP, rcWnd.left, rcWnd.top, rcWnd.right-rcWnd.left, rcWnd.bottom-rcWnd.top, 0);
}

void CConEmuMain::UpdateIdealRect(RECT rcNewIdeal)
{
#ifdef _DEBUG
	RECT rc = rcNewIdeal;
	_ASSERTE(rc.right>rc.left && rc.bottom>rc.top);
	if (memcmp(&mr_Ideal.rcIdeal, &rc, sizeof(rc)) == 0)
		return;
#endif

	mr_Ideal.rcIdeal = rcNewIdeal;
}

void CConEmuMain::UpdateTextColorSettings(BOOL ChangeTextAttr /*= TRUE*/, BOOL ChangePopupAttr /*= TRUE*/)
{
	CVConGroup::OnUpdateTextColorSettings(ChangeTextAttr, ChangePopupAttr);
}

void CConEmuMain::DebugStep(LPCWSTR asMsg, BOOL abErrorSeverity/*=FALSE*/)
{
	if (asMsg && *asMsg)
	{
		// Если ведется ЛОГ - выбросить в него
		LogString(asMsg);
	}

	if (ghWnd)
	{
		static bool bWasDbgStep, bWasDbgError;

		if (asMsg && *asMsg)
		{
			if (gpSet->isDebugSteps || abErrorSeverity)
			{
				bWasDbgStep = true;

				if (abErrorSeverity) bWasDbgError = true;

				SetWindowText(ghWnd, asMsg);
			}
		}
		else
		{
			// Обновит заголовок в соответствии с возможными процентами в неактивной консоли
			// и выполнит это в главной нити, если необходимо
			// abErrorSeverity проверяем, чтобы можно было насильно "повторить" текущий заголовок
			if (bWasDbgStep || abErrorSeverity)
			{
				bWasDbgStep = false;

				if (bWasDbgError)
				{
					bWasDbgError = false;
					return;
				}

				UpdateTitle();
			}
		}
	}
}

DWORD_PTR CConEmuMain::GetActiveKeyboardLayout()
{
	_ASSERTE(mn_MainThreadId!=0);
	DWORD_PTR dwActive = (DWORD_PTR)GetKeyboardLayout(mn_MainThreadId);
	return dwActive;
}

//LPTSTR CConEmuMain::GetTitleStart()
//{
//	//mn_ActiveStatus &= ~CES_CONALTERNATIVE; // сброс флага альтернативной консоли
//	return Title;
//}

LPCWSTR CConEmuMain::GetDefaultTitle()
{
	return ms_ConEmuDefTitle;
}

void CConEmuMain::SetTitleTemplate(LPCWSTR asTemplate)
{
	lstrcpyn(TitleTemplate, asTemplate ? asTemplate : L"", countof(TitleTemplate));
}

//LRESULT CConEmuMain::GuiShellExecuteEx(SHELLEXECUTEINFO* lpShellExecute, CVirtualConsole* apVCon)
//{
//	LRESULT lRc = 0;
//
//	if (!isMainThread())
//	{
//		GuiShellExecuteExArg* pArg = (GuiShellExecuteExArg*)calloc(1,sizeof(*pArg));
//		pArg->pVCon = apVCon;
//		pArg->lpShellExecute = lpShellExecute;
//		pArg->hReadyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
//
//		EnterCriticalSection(&mcs_ShellExecuteEx);
//		m_ShellExecuteQueue.push_back(pArg);
//		LeaveCriticalSection(&mcs_ShellExecuteEx);
//
//		PostMessage(ghWnd, mn_ShellExecuteEx, 0, 0);
//
//		if (ghWnd && IsWindow(ghWnd))
//		{
//			WaitForSingleObject(pArg->hReadyEvent, INFINITE);
//
//			lRc = pArg->bResult;
//		}
//
//		SafeCloseHandle(pArg->hReadyEvent);
//	}
//	else
//	{
//		_ASSERTE(FALSE && "GuiShellExecuteEx must not be called in Main thread!");
//	}
//
//	return lRc;
//}

//void CConEmuMain::GuiShellExecuteExQueue()
//{
//	if (mb_InShellExecuteQueue)
//	{
//		// Очередь уже обрабатывается!
//		return;
//	}
//
//	_ASSERTE(isMainThread()); // Должно выполняться в основном потоке!
//
//	mb_InShellExecuteQueue = true;
//
//	while (m_ShellExecuteQueue.size() != 0)
//	{
//		GuiShellExecuteExArg* pArg = NULL;
//
//		EnterCriticalSection(&mcs_ShellExecuteEx);
//		for (INT_PTR i = 0; i < m_ShellExecuteQueue.size(); i++)
//		{
//			if (m_ShellExecuteQueue[i]->bInProcess)
//			{
//				_ASSERTE(m_ShellExecuteQueue[i]->bInProcess == FALSE); // TRUE - должен быть убран из очереди!
//			}
//			else
//			{
//				m_ShellExecuteQueue[i]->bInProcess = TRUE;
//				pArg = m_ShellExecuteQueue[i];
//				m_ShellExecuteQueue.erase(i);
//				break;
//			}
//		}
//		LeaveCriticalSection(&mcs_ShellExecuteEx);
//
//		if (!pArg)
//		{
//			break; // очередь обработана?
//		}
//
//		SHELLEXECUTEINFO seiPre = *pArg->lpShellExecute;
//
//		BOOL lRc = ::ShellExecuteEx(pArg->lpShellExecute);
//
//		SHELLEXECUTEINFO seiPst = *pArg->lpShellExecute;
//		DWORD dwErr = (lRc == FALSE) ? GetLastError() : 0;
//
//		pArg->bResult = lRc;
//		pArg->dwErrCode = dwErr;		
//
//		if (lRc != FALSE)
//		{
//			if (pArg->lpShellExecute->fMask & SEE_MASK_NOCLOSEPROCESS)
//			{
//				// OK, но нам нужен хэндл запущенного процесса
//				_ASSERTE(seiPst.hProcess != NULL);
//				_ASSERTE(pArg->lpShellExecute->hProcess != NULL);
//			}
//		}
//		else // Ошибка
//		{
//			//120429 - если мы были в Recreate - то наверное не закрывать, пусть болванка висит?
//			if ((isValid(pArg->pVCon) && !pArg->pVCon->RCon()->InRecreate()))
//			{
//				pArg->pVCon->RCon()->CloseConsole(false, false);
//			}
//		}
//
//		SetEvent(pArg->hReadyEvent);
//
//		UNREFERENCED_PARAMETER(seiPre.cbSize);
//		UNREFERENCED_PARAMETER(seiPst.cbSize);
//		UNREFERENCED_PARAMETER(dwErr);
//	}
//
//	mb_InShellExecuteQueue = false;
//}

bool CConEmuMain::ExecuteProcessPrepare()
{
	TODO("Disable all global hooks for execution time?");
	return false;
}

void CConEmuMain::ExecuteProcessFinished(bool bOpt)
{
	if (bOpt)
	{
		TODO("Return all global hooks for execution time?");
	}
}

//BOOL CConEmuMain::HandlerRoutine(DWORD dwCtrlType)
//{
//    return (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT ? true : false);
//}

void CConEmuMain::SetWindowIcon(LPCWSTR asNewIcon)
{
	if (!asNewIcon || !*asNewIcon)
		return;

	HICON hOldClassIcon = hClassIcon, hOldClassIconSm = hClassIconSm;
	hClassIcon = NULL; hClassIconSm = NULL;
	SafeFree(gpConEmu->mps_IconPath);
	gpConEmu->mps_IconPath = ExpandEnvStr(asNewIcon);

	LoadIcons();

	if (hClassIcon)
	{
		SetClassLongPtr(ghWnd, GCLP_HICON, (LONG_PTR)hClassIcon);
		SetClassLongPtr(ghWnd, GCLP_HICONSM, (LONG_PTR)hClassIconSm);
		SendMessage(ghWnd, WM_SETICON, ICON_BIG, (LPARAM)hClassIcon);
		SendMessage(ghWnd, WM_SETICON, ICON_SMALL, (LPARAM)hClassIconSm);
	}
	else
	{
		hClassIcon = hOldClassIcon; hClassIconSm = hOldClassIconSm;
	}
}

// Нужно вызывать после загрузки настроек!
void CConEmuMain::LoadIcons()
{
	if (hClassIcon)
		return; // Уже загружены

	wchar_t *lpszExt = NULL;
	wchar_t szIconPath[MAX_PATH] = {};

	if (gpConEmu->mps_IconPath)
	{
		if (FileExists(gpConEmu->mps_IconPath))
		{
			wcscpy_c(szIconPath, gpConEmu->mps_IconPath);
		}
		else
		{
			wchar_t* pszFilePart;
			DWORD n = SearchPath(NULL, gpConEmu->mps_IconPath, NULL, countof(szIconPath), szIconPath, &pszFilePart);
			if (!n || (n >= countof(szIconPath)))
				szIconPath[0] = 0;
		}
	}
	else
	{
		lstrcpyW(szIconPath, ms_ConEmuExe);
		lpszExt = (wchar_t*)PointToExt(szIconPath);

		if (!lpszExt)
		{
			szIconPath[0] = 0;
		}
		else
		{
			_tcscpy(lpszExt, _T(".ico"));
			if (!FileExists(szIconPath))
				szIconPath[0]=0;
		}
	}

	if (szIconPath[0])
	{
		lpszExt = (wchar_t*)PointToExt(szIconPath);

		if (lpszExt && (lstrcmpi(lpszExt, L".ico") == 0))
		{
			hClassIcon = (HICON)LoadImage(0, szIconPath, IMAGE_ICON,
			                              GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR|LR_LOADFROMFILE);
			hClassIconSm = (HICON)LoadImage(0, szIconPath, IMAGE_ICON,
			                                GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR|LR_LOADFROMFILE);
        }
        else
        {
        	ExtractIconEx(szIconPath, 0, &hClassIcon, &hClassIconSm, 1);
        }
	}

	if (!hClassIcon)
	{
		szIconPath[0]=0;
		hClassIcon = (HICON)LoadImage(GetModuleHandle(0),
		                              MAKEINTRESOURCE(gpSet->nIconID), IMAGE_ICON,
		                              GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);

		if (hClassIconSm) DestroyIcon(hClassIconSm);

		hClassIconSm = (HICON)LoadImage(GetModuleHandle(0),
		                                MAKEINTRESOURCE(gpSet->nIconID), IMAGE_ICON,
		                                GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	}
}

//bool CConEmuMain::LoadVersionInfo(wchar_t* pFullPath)
//{
//	LPBYTE pBuffer=NULL;
//	wchar_t* pVersion=NULL;
//	//wchar_t* pDesc=NULL;
//	const wchar_t WSFI[] = L"StringFileInfo";
//	DWORD size = GetFileVersionInfoSizeW(pFullPath, &size);
//
//	if (!size) return false;
//
//	MCHKHEAP
//	pBuffer = new BYTE[size];
//	MCHKHEAP
//	GetFileVersionInfoW((wchar_t*)pFullPath, 0, size, pBuffer);
//	//Find StringFileInfo
//	DWORD ofs;
//
//	for(ofs = 92; ofs < size; ofs += *(WORD*)(pBuffer+ofs))
//		if (!lstrcmpiW((wchar_t*)(pBuffer+ofs+6), WSFI))
//			break;
//
//	if (ofs >= size)
//	{
//		delete pBuffer;
//		return false;
//	}
//
//	TCHAR *langcode;
//	langcode = (TCHAR*)(pBuffer + ofs + 42);
//	TCHAR blockname[48];
//	unsigned dsize;
//	_wsprintf(blockname, SKIPLEN(countof(blockname)) _T("\\%s\\%s\\FileVersion"), WSFI, langcode);
//
//	if (!VerQueryValue(pBuffer, blockname, (void**)&pVersion, &dsize))
//	{
//		pVersion = 0;
//	}
//	else
//	{
//		if (dsize>=31) pVersion[31]=0;
//
//		wcscpy(szConEmuVersion, pVersion);
//		pVersion = wcsrchr(szConEmuVersion, L',');
//
//		if (pVersion && wcscmp(pVersion, L", 0")==0)
//			*pVersion = 0;
//	}
//
//	delete[] pBuffer;
//	return true;
//}

void CConEmuMain::OnCopyingState()
{
	TODO("CConEmuMain::OnCopyingState()");
}

void CConEmuMain::PostDragCopy(BOOL abMove, BOOL abReceived/*=FALSE*/)
{
	if (!abReceived)
	{
		PostMessage(ghWnd, mn_MsgPostCopy, 0, (LPARAM)abMove);
	}
	else
	{
		//PostMacro(apszMacro);
		//free(apszMacro);
		CVConGuard VCon;
		if (GetActiveVCon(&VCon) >= 0)
		{
			VCon->RCon()->PostDragCopy(abMove);
		}
	}
}

void CConEmuMain::PostMacro(LPCWSTR asMacro)
{
	if (!asMacro || !*asMacro)
		return;

	CVConGuard VCon;
	if (GetActiveVCon(&VCon) >= 0)
	{
		VCon->RCon()->PostMacro(asMacro);
	}
	//#ifdef _DEBUG
	//DEBUGSTRMACRO(asMacro); OutputDebugStringW(L"\n");
	//#endif
	//
	//CConEmuPipe pipe(GetFarPID(), CONEMUREADYTIMEOUT);
	//if (pipe.Init(_T("CConEmuMain::PostMacro"), TRUE))
	//{
	//    //DWORD cbWritten=0;
	//    DebugStep(_T("Macro: Waiting for result (10 sec)"));
	//    pipe.Execute(CMD_POSTMACRO, asMacro, (_tcslen(asMacro)+1)*2);
	//    DebugStep(NULL);
	//}
}

void CConEmuMain::PostAutoSizeFont(int nRelative/*0/1*/, int nValue/*для nRelative==0 - высота, для ==1 - +-1, +-2,...*/)
{
	PostMessage(ghWnd, mn_MsgAutoSizeFont, (WPARAM)nRelative, (LPARAM)nValue);
}

void CConEmuMain::PostMacroFontSetName(wchar_t* pszFontName, WORD anHeight /*= 0*/, WORD anWidth /*= 0*/, BOOL abPosted)
{
	if (!abPosted)
	{
		wchar_t* pszDup = lstrdup(pszFontName);
		WPARAM wParam = (((DWORD)anHeight) << 16) | (anWidth);
		PostMessage(ghWnd, mn_MsgMacroFontSetName, wParam, (LPARAM)pszDup);
	}
	else
	{
		if (gpSetCls)
			gpSetCls->MacroFontSetName(pszFontName, anHeight, anWidth);
		free(pszFontName);
	}
}

void CConEmuMain::PostChangeCurPalette(LPCWSTR pszPalette, bool bChangeDropDown, bool abPosted)
{
	if (!abPosted)
	{
		// Synchronous!
		SendMessage(ghWnd, mn_MsgReqChangeCurPalette, (WPARAM)bChangeDropDown, (LPARAM)pszPalette);
	}
	else
	{
		const Settings::ColorPalette* pPal = gpSet->PaletteGetByName(pszPalette);
		gpSetCls->ChangeCurrentPalette(pPal, bChangeDropDown);
	}
}

LRESULT CConEmuMain::SyncExecMacro(WPARAM wParam, LPARAM lParam)
{
	LRESULT lRc;
	lRc = SendMessage(ghWnd, mn_MsgMacroExecSync, wParam, lParam);
	return lRc;
}

void CConEmuMain::PostDisplayRConError(CRealConsole* apRCon, wchar_t* pszErrMsg)
{
	//CVConGuard VCon(apRCon->VCon());
	//SendMessage(ghWnd, mn_MsgDisplayRConError, (WPARAM)apRCon, (LPARAM)pszErrMsg);

	if (RELEASEDEBUGTEST(gpConEmu->mb_FindBugMode,true))
	{
		//_ASSERTE(FALSE && "Strange 'ConEmuCInput not found' error");
		RaiseTestException();
	}

	PostMessage(ghWnd, mn_MsgDisplayRConError, (WPARAM)apRCon, (LPARAM)pszErrMsg);
}

bool CConEmuMain::PtDiffTest(POINT C, int aX, int aY, UINT D)
{
	//(((abs(C.x-(int)(short)LOWORD(lParam)))<D) && ((abs(C.y-(int)(short)HIWORD(lParam)))<D))
	int nX = C.x - aX;

	if (nX < 0) nX = -nX;

	if (nX > (int)D)
		return false;

	int nY = C.y - aY;

	if (nY < 0) nY = -nY;

	if (nY > (int)D)
		return false;

	return true;
}

void CConEmuMain::RegisterMinRestore(bool abRegister)
{
	wchar_t szErr[512];

	if (abRegister && !mp_Inside)
	{
		for (size_t i = 0; i < countof(gRegisteredHotKeys); i++)
		{
			//if (!gpSet->vmMinimizeRestore)

			const ConEmuHotKey* pHk = NULL;
			DWORD VkMod = gpSet->GetHotkeyById(gRegisteredHotKeys[i].DescrID, &pHk);
			UINT vk = ConEmuHotKey::GetHotkey(VkMod);
			if (!vk)
				continue;  // не просили
			UINT nMOD = ConEmuHotKey::GetHotKeyMod(VkMod);

			if (gRegisteredHotKeys[i].RegisteredID
					&& ((gRegisteredHotKeys[i].VK != vk) || (gRegisteredHotKeys[i].MOD != nMOD)))
			{
				UnregisterHotKey(ghWnd, gRegisteredHotKeys[i].RegisteredID);

				if (gpSetCls->isAdvLogging)
				{
					_wsprintf(szErr, SKIPLEN(countof(szErr)) L"UnregisterHotKey(ID=%u)", gRegisteredHotKeys[i].RegisteredID);
					LogString(szErr, TRUE);
				}

				gRegisteredHotKeys[i].RegisteredID = 0;
			}

			if (!gRegisteredHotKeys[i].RegisteredID)
			{
				wchar_t szKey[128];
				pHk->GetHotkeyName(szKey);

				BOOL bRegRc = RegisterHotKey(ghWnd, HOTKEY_GLOBAL_START+i, nMOD, vk);
				gRegisteredHotKeys[i].IsRegistered = bRegRc;
				DWORD dwErr = bRegRc ? 0 : GetLastError();

				if (gpSetCls->isAdvLogging)
				{
					_wsprintf(szErr, SKIPLEN(countof(szErr)) L"RegisterHotKey(ID=%u, %s, VK=%u, MOD=x%X) - %s, Code=%u", HOTKEY_GLOBAL_START+i, szKey, vk, nMOD, bRegRc ? L"OK" : L"FAILED", dwErr);
					LogString(szErr, TRUE);
				}

				if (bRegRc)
				{
					gRegisteredHotKeys[i].RegisteredID = HOTKEY_GLOBAL_START+i;
					gRegisteredHotKeys[i].VK = vk;
					gRegisteredHotKeys[i].MOD = nMOD;
				}
				else
				{
					if (isFirstInstance(true))
					{
						// -- При одновременном запуске двух копий - велики шансы, что они подерутся
						// -- наверное вообще не будем показывать ошибку
						// -- кроме того, isFirstInstance() не работает, если копия ConEmu.exe запущена под другим юзером
						wchar_t szName[128];

						if (!LoadString(g_hInstance, gRegisteredHotKeys[i].DescrID, szName, countof(szName)))
							_wsprintf(szName, SKIPLEN(countof(szName)) L"DescrID=%i", gRegisteredHotKeys[i].DescrID);

						_wsprintf(szErr, SKIPLEN(countof(szErr))
							L"Can't register hotkey for\n%s\n%s"
							L"%s, ErrCode=%u",
							szName,
							(dwErr == 1409) ? L"Hotkey already registered by another App\n" : L"",
							szKey, dwErr);
						Icon.ShowTrayIcon(szErr, tsa_Config_Error);
					}
				}
			}
		}
	}
	else
	{
		for (size_t i = 0; i < countof(gRegisteredHotKeys); i++)
		{
			if (gRegisteredHotKeys[i].RegisteredID)
			{
				UnregisterHotKey(ghWnd, gRegisteredHotKeys[i].RegisteredID);

				if (gpSetCls->isAdvLogging)
				{
					_wsprintf(szErr, SKIPLEN(countof(szErr)) L"UnregisterHotKey(ID=%u)", gRegisteredHotKeys[i].RegisteredID);
					LogString(szErr, TRUE);
				}

				gRegisteredHotKeys[i].RegisteredID = 0;
			}
		}
		//if (mn_MinRestoreRegistered)
		//{
		//	UnregisterHotKey(ghWnd, mn_MinRestoreRegistered);
		//	mn_MinRestoreRegistered = 0;
		//}
	}
}


static struct RegisteredHotKeys
//{
//	int DescrID;
//	int RegisteredID; // wParam для WM_HOTKEY
//	UINT VK, MOD;     // чтобы на изменение реагировать
//}
gActiveOnlyHotKeys[] = {
	{0, HOTKEY_CTRLWINALTSPACE_ID, VK_SPACE, MOD_CONTROL|MOD_WIN|MOD_ALT},
	//#ifdef Use_vkSwitchGuiFocus
	{vkSetFocusSwitch, HOTKEY_SETFOCUSSWITCH_ID},
	{vkSetFocusGui,    HOTKEY_SETFOCUSGUI_ID},
	{vkSetFocusChild,  HOTKEY_SETFOCUSCHILD_ID},
	{vkChildSystemMenu,HOTKEY_CHILDSYSMENU_ID},
	//#endif
};

// When hotkey changed in "Keys & Macro" page
void CConEmuMain::GlobalHotKeyChanged()
{
	if (isIconic())
	{
		return;
	}

	RegisterGlobalHotKeys(false);
	RegisterGlobalHotKeys(true);
}

void CConEmuMain::RegisterGlobalHotKeys(bool bRegister)
{
	if (bRegister == mb_HotKeyRegistered)
		return; // уже

	if (bRegister)
	{
		for (size_t i = 0; i < countof(gActiveOnlyHotKeys); i++)
		{
			DWORD id, vk, mod;

			// например, HOTKEY_CTRLWINALTSPACE_ID
			id = gActiveOnlyHotKeys[i].RegisteredID;

			if (gActiveOnlyHotKeys[i].DescrID == 0)
			{
				// VK_SPACE
				vk = gActiveOnlyHotKeys[i].VK;
				// MOD_CONTROL|MOD_WIN|MOD_ALT
				mod = gActiveOnlyHotKeys[i].MOD;
			}
			else
			{
				const ConEmuHotKey* pHk = NULL;
				DWORD VkMod = gpSet->GetHotkeyById(gActiveOnlyHotKeys[i].DescrID, &pHk);
				vk = ConEmuHotKey::GetHotkey(VkMod);
				if (!vk)
					continue;  // не просили
				mod = ConEmuHotKey::GetHotKeyMod(VkMod);
			}

			BOOL bRegRc = RegisterHotKey(ghWnd, id, mod, vk);
			gActiveOnlyHotKeys[i].IsRegistered = bRegRc;
			DWORD dwErr = bRegRc ? 0 : GetLastError();

			if (gpSetCls->isAdvLogging)
			{
				char szErr[512];
				_wsprintfA(szErr, SKIPLEN(countof(szErr)) "RegisterHotKey(ID=%u, VK=%u, MOD=x%X) - %s, Code=%u", id, vk, mod, bRegRc ? "OK" : "FAILED", dwErr);
				LogString(szErr, TRUE);
			}

			if (bRegRc)
			{
				mb_HotKeyRegistered = true;
			}
		}
	}
	else
	{
		for (size_t i = 0; i < countof(gActiveOnlyHotKeys); i++)
		{
			// например, HOTKEY_CTRLWINALTSPACE_ID
			DWORD id = gActiveOnlyHotKeys[i].RegisteredID;

			UnregisterHotKey(ghWnd, id);

			if (gpSetCls->isAdvLogging)
			{
				char szErr[128];
				_wsprintfA(szErr, SKIPLEN(countof(szErr)) "UnregisterHotKey(ID=%u)", id);
				LogString(szErr, TRUE);
			}
		}

		mb_HotKeyRegistered = false;
	}
}

void CConEmuMain::RegisterHotKeys()
{
	if (isIconic())
	{
		UnRegisterHotKeys();
		return;
	}

	RegisterGlobalHotKeys(true);

	if (!mh_LLKeyHook)
	{
		RegisterHooks();
	}
}

HMODULE CConEmuMain::LoadConEmuCD()
{
	if (!mh_LLKeyHookDll)
	{
		wchar_t szConEmuDll[MAX_PATH+32];
		lstrcpy(szConEmuDll, ms_ConEmuBaseDir);
#ifdef WIN64
		lstrcat(szConEmuDll, L"\\ConEmuCD64.dll");
#else
		lstrcat(szConEmuDll, L"\\ConEmuCD.dll");
#endif
		//wchar_t szSkipEventName[128];
		//_wsprintf(szSkipEventName, SKIPLEN(countof(szSkipEventName)) CEHOOKDISABLEEVENT, GetCurrentProcessId());
		//HANDLE hSkipEvent = CreateEvent(NULL, TRUE, TRUE, szSkipEventName);
		mh_LLKeyHookDll = LoadLibrary(szConEmuDll);
		//CloseHandle(hSkipEvent);

		if (mh_LLKeyHookDll)
			mph_HookedGhostWnd = (HWND*)GetProcAddress(mh_LLKeyHookDll, "ghActiveGhost");
	}

	if (!mh_LLKeyHookDll)
		mph_HookedGhostWnd = NULL;
	
	return mh_LLKeyHookDll;
}

void CConEmuMain::UpdateWinHookSettings()
{
	if (mh_LLKeyHookDll)
	{
		gpSetCls->UpdateWinHookSettings(mh_LLKeyHookDll);

		CVConGuard VCon;
		if (GetActiveVCon(&VCon) >= 0)
		{
			UpdateActiveGhost(VCon.VCon());
		}
	}
}

void CConEmuMain::RegisterHooks()
{
//	#ifndef _DEBUG
	// -- для GUI режима таки нужно
	//// Для WinXP это не было нужно
	//if (gOSVer.dwMajorVersion < 6)
	//{
	//	return;
	//}
//	#endif

	// Если Host-клавиша НЕ Win, или юзер не хочет переключаться Win+Number - хук не нужен
	//if (!gpSet->isUseWinNumber || !gpSet->IsHostkeySingle(VK_LWIN))
	if (!gpSetCls->HasSingleWinHotkey())
	{
		if (gpSetCls->isAdvLogging)
		{
			LogString("CConEmuMain::RegisterHooks() skipped, cause of !HasSingleWinHotkey()", TRUE);
		}

		UnRegisterHooks();

		return;
	}

	DWORD dwErr = 0;

	if (!mh_LLKeyHook)
	{
		// Проверяет, разрешил ли пользователь установку хуков.
		if (!gpSet->isKeyboardHooks())
		{
			if (gpSetCls->isAdvLogging)
			{
				LogString("CConEmuMain::RegisterHooks() skipped, cause of !isKeyboardHooks()", TRUE);
			}
		}
		else
		{
			if (!mh_LLKeyHookDll)
				LoadConEmuCD();
			//{
			//	wchar_t szConEmuDll[MAX_PATH+32];
			//	lstrcpy(szConEmuDll, ms_ConEmuBaseDir);
			//#ifdef WIN64
			//	lstrcat(szConEmuDll, L"\\ConEmuCD64.dll");
			//#else
			//	lstrcat(szConEmuDll, L"\\ConEmuCD.dll");
			//#endif
			//	wchar_t szSkipEventName[128];
			//	_wsprintf(szSkipEventName, SKIPLEN(countof(szSkipEventName)) CEHOOKDISABLEEVENT, GetCurrentProcessId());
			//	HANDLE hSkipEvent = CreateEvent(NULL, TRUE, TRUE, szSkipEventName);
			//	mh_LLKeyHookDll = LoadLibrary(szConEmuDll);
			//	CloseHandle(hSkipEvent);
			//}
			
			_ASSERTE(mh_LLKeyHook==NULL); // Из другого потока регистрация прошла?

			if (!mh_LLKeyHook && mh_LLKeyHookDll)
			{
				HOOKPROC pfnLLHK = (HOOKPROC)GetProcAddress(mh_LLKeyHookDll, "LLKeybHook");
				HHOOK *pKeyHook = (HHOOK*)GetProcAddress(mh_LLKeyHookDll, "ghKeyHook");
				HWND *pConEmuRoot = (HWND*)GetProcAddress(mh_LLKeyHookDll, "ghKeyHookConEmuRoot");
				//BOOL *pbWinTabHook = (BOOL*)GetProcAddress(mh_LLKeyHookDll, "gbWinTabHook");
				//HWND *pConEmuDc = (HWND*)GetProcAddress(mh_LLKeyHookDll, "ghConEmuWnd");

				if (pConEmuRoot)
					*pConEmuRoot = ghWnd;

				UpdateWinHookSettings();

				//if (pConEmuDc)
				//	*pConEmuDc = ghWnd DC;

				if (pfnLLHK)
				{
					// WH_KEYBOARD_LL - может быть только глобальной
					mh_LLKeyHook = SetWindowsHookEx(WH_KEYBOARD_LL, pfnLLHK, mh_LLKeyHookDll, 0);

					if (!mh_LLKeyHook)
					{
						dwErr = GetLastError();
						if (gpSetCls->isAdvLogging)
						{
							char szErr[128];
							_wsprintfA(szErr, SKIPLEN(countof(szErr)) "CConEmuMain::RegisterHooks() failed, Code=%u", dwErr);
							LogString(szErr, TRUE);
						}
						_ASSERTE(mh_LLKeyHook!=NULL);
					}
					else
					{
						if (pKeyHook) *pKeyHook = mh_LLKeyHook;
						if (gpSetCls->isAdvLogging)
						{
							LogString("CConEmuMain::RegisterHooks() succeeded", TRUE);
						}
					}
				}
			}
			else
			{
				if (gpSetCls->isAdvLogging)
				{
					LogString("CConEmuMain::RegisterHooks() failed, cause of !mh_LLKeyHookDll", TRUE);
				}
			}
		}
	}
	else
	{
		if (gpSetCls->isAdvLogging >= 2)
		{
			LogString("CConEmuMain::RegisterHooks() skipped, already was set", TRUE);
		}
	}
}

//BOOL CConEmuMain::LowLevelKeyHook(UINT nMsg, UINT nVkKeyCode)
//{
//    //if (nVkKeyCode == ' ' && gpSet->isSendAltSpace == 2 && gpSet->IsHostkeyPressed())
//	//{
//    //	ShowSysmenu();
//	//	return TRUE;
//	//}
//
//	if (!gpSet->isUseWinNumber || !gpSet->IsHostkeyPressed())
//		return FALSE;
//
//	// Теперь собственно обработка
//	if (nVkKeyCode >= '0' && nVkKeyCode <= '9')
//	{
//		if (nMsg == WM_KEYDOWN)
//		{
//			if (nVkKeyCode>='1' && nVkKeyCode<='9') // ##1..9
//				ConActivate(nVkKeyCode - '1');
//
//			else if (nVkKeyCode=='0') // #10.
//				ConActivate(9);
//		}
//
//		return TRUE;
//	}
//
//	return FALSE;
//}

void CConEmuMain::UnRegisterHotKeys(BOOL abFinal/*=FALSE*/)
{
	RegisterGlobalHotKeys(false);

	UnRegisterHooks(abFinal);
}

void CConEmuMain::UnRegisterHooks(BOOL abFinal/*=FALSE*/)
{
	if (mh_LLKeyHook)
	{
		UnhookWindowsHookEx(mh_LLKeyHook);
		mh_LLKeyHook = NULL;

		if (gpSetCls->isAdvLogging)
		{
			LogString("CConEmuMain::UnRegisterHooks() done", TRUE);
		}
	}

	if (abFinal)
	{
		if (mh_LLKeyHookDll)
		{
			FreeLibrary(mh_LLKeyHookDll);
			mh_LLKeyHookDll = NULL;
		}
	}
}

// Обработка WM_HOTKEY
void CConEmuMain::OnWmHotkey(WPARAM wParam)
{
	// Ctrl+Win+Alt+Space
	if (wParam == HOTKEY_CTRLWINALTSPACE_ID)
	{
		CtrlWinAltSpace();
	}
	// Win+Esc by default
	else if ((wParam == HOTKEY_SETFOCUSSWITCH_ID) || (wParam == HOTKEY_SETFOCUSGUI_ID) || (wParam == HOTKEY_SETFOCUSCHILD_ID))
	{
		SwitchGuiFocusOp FocusOp = 
			(wParam == HOTKEY_SETFOCUSSWITCH_ID) ? sgf_FocusSwitch :
			(wParam == HOTKEY_SETFOCUSGUI_ID) ? sgf_FocusGui :
			(wParam == HOTKEY_SETFOCUSCHILD_ID) ? sgf_FocusChild : sgf_None;
		OnSwitchGuiFocus(FocusOp);
	}
	else if (wParam == HOTKEY_CHILDSYSMENU_ID)
	{
		CVConGuard VCon;
		if (GetActiveVCon(&VCon) >= 0)
		{
			VCon->RCon()->ChildSystemMenu();
		}
	}
	else
	{
		//// vmMinimizeRestore -> Win+C
		//else if (gpConEmu->mn_MinRestoreRegistered && (int)wParam == gpConEmu->mn_MinRestoreRegistered)
		//{
		//	gpConEmu->DoMinimizeRestore();
		//}

		for (size_t i = 0; i < countof(gRegisteredHotKeys); i++)
		{
			if (gRegisteredHotKeys[i].RegisteredID && ((int)wParam == gRegisteredHotKeys[i].RegisteredID))
			{
				switch (gRegisteredHotKeys[i].DescrID)
				{
				case vkMinimizeRestore:
				case vkMinimizeRestor2:
					DoMinimizeRestore();
					break;
				case vkGlobalRestore:
					DoMinimizeRestore(sih_Show);
					break;
				case vkForceFullScreen:
					DoForcedFullScreen(true);
					break;
				}
				break;
			}
		}
	}
}

void CConEmuMain::CtrlWinAltSpace()
{
	CVConGuard VCon;
	if ((GetActiveVCon(&VCon) >= 0) && VCon->RCon())
	{
		VCon->RCon()->CtrlWinAltSpace(); // Toggle visibility
	}
}

void CConEmuMain::DeleteVConMainThread(CVirtualConsole* apVCon)
{
	// We can't use SendMessage because of server thread blocking
	DEBUGTEST(LRESULT lRc =)
		PostMessage(ghWnd, mn_MsgDeleteVConMainThread, 0, (LPARAM)apVCon);
	//if (!lRc)
	//{
	//	if (CVConGroup::isValid(apVCon))
	//	{
	//		apVCon->DeleteFromMainThread();
	//	}
	//}
}

// abRecreate: TRUE - пересоздать текущую, FALSE - создать новую
// abConfirm:  TRUE - показать диалог подтверждения
// abRunAs:    TRUE - под админом
void CConEmuMain::RecreateAction(RecreateActionParm aRecreate, BOOL abConfirm, BOOL abRunAs)
{
	FLASHWINFO fl = {sizeof(FLASHWINFO)}; fl.dwFlags = FLASHW_STOP; fl.hwnd = ghWnd;
	FlashWindowEx(&fl); // При многократных созданиях мигать начинает...
	RConStartArgs args;

	if (aRecreate == cra_RecreateTab)
	{
		CVConGuard VCon;
		if ((GetActiveVCon(&VCon) >= 0) && VCon->RCon())
		{
			const RConStartArgs& CurArgs = VCon->RCon()->GetArgs();
			args.AssignFrom(&CurArgs);
			//args.pszSpecialCmd = CurArgs.CreateCommandLine();
		}
	}

	args.aRecreate = aRecreate;
	args.RunAsAdministrator = abRunAs ? crb_On : crb_Off;

	WARNING("При переходе на новую обработку кнопок больше не нужно");
	//if (!abConfirm && isPressed(VK_SHIFT))
	//	abConfirm = TRUE;

	if ((args.aRecreate == cra_CreateTab) || (args.aRecreate == cra_CreateWindow))
	{
		// Создать новую консоль
		BOOL lbSlotFound = (args.aRecreate == cra_CreateWindow);

		if (args.aRecreate == cra_CreateWindow && gpSetCls->IsMulti())
			abConfirm = TRUE;

		if (args.aRecreate == cra_CreateTab)
		{
			//for (size_t i = 0; i < countof(mp_VCon); i++)
			//{
			//	if (!mp_VCon[i]) { lbSlotFound = TRUE; break; }
			//}
			// Проверяем по последней, т.к. "дырок" у нас быть не может
			if (!isVConExists(MAX_CONSOLE_COUNT-1))
				lbSlotFound = TRUE;
		}

		if (!lbSlotFound)
		{
			static bool bBoxShowed = false;

			if (!bBoxShowed)
			{
				bBoxShowed = true;
				FlashWindowEx(&fl); // При многократных созданиях мигать начинает...
				MBoxA(L"Maximum number of consoles was reached.");
				bBoxShowed = false;
			}

			FlashWindowEx(&fl); // При многократных созданиях мигать начинает...
			return;
		}

		if (abConfirm)
		{
			int nRc = RecreateDlg(&args);

			//int nRc = DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_RESTART), ghWnd, Recreate DlgProc, (LPARAM)&args);
			if (nRc != IDC_START)
				return;

			CVConGroup::Redraw();
		}

		if (args.aRecreate == cra_CreateTab)
		{
			//Собственно, запуск
			CreateCon(&args, true);
		}
		else
		{
			if (!args.pszSpecialCmd || !*args.pszSpecialCmd)
			{
				_ASSERTE((args.pszSpecialCmd && *args.pszSpecialCmd) || !abConfirm);
				args.pszSpecialCmd = lstrdup(gpSetCls->GetCmd());
			}

			if (!args.pszSpecialCmd || !*args.pszSpecialCmd)
			{
				_ASSERTE(args.pszSpecialCmd && *args.pszSpecialCmd);
			}
			else
			{
				// Start new ConEmu.exe process with choosen arguments...
				CreateWnd(&args);
			}
		}
	}
	else
	{
		// Restart or close console
		CVConGuard VCon;
		if ((GetActiveVCon(&VCon) >= 0) && VCon->RCon())
		{
			args.RunAsAdministrator = (abRunAs || VCon->RCon()->isAdministrator()) ? crb_On : crb_Off;

			if (abConfirm)
			{
				int nRc = RecreateDlg(&args);

				//int nRc = DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_RESTART), ghWnd, Recreate DlgProc, (LPARAM)&args);
				if (nRc == IDC_TERMINATE)
				{
					VCon->RCon()->CloseConsole(true, false);
					return;
				}

				if (nRc != IDC_START)
					return;
			}

			if (VCon.VCon())
			{
				VCon->Redraw();
				// Собственно, Recreate
				VCon->RCon()->RecreateProcess(&args);
			}
		}
	}

	SafeFree(args.pszSpecialCmd);
	SafeFree(args.pszStartupDir);
	SafeFree(args.pszUserName);
	//SafeFree(args.pszUserPassword);
}

int CConEmuMain::RecreateDlg(RConStartArgs* apArg)
{
	if (!mp_RecreateDlg)
		mp_RecreateDlg = new CRecreateDlg();

	int nRc = mp_RecreateDlg->RecreateDlg(apArg);

	return nRc;
}



BOOL CConEmuMain::RunSingleInstance(HWND hConEmuWnd /*= NULL*/, LPCWSTR apszCmd /*= NULL*/)
{
	BOOL lbAccepted = FALSE;
	LPCWSTR lpszCmd = apszCmd ? apszCmd : gpSetCls->GetCmd();

	if ((lpszCmd && *lpszCmd) || (gpSetCls->SingleInstanceShowHide != sih_None))
	{
		HWND ConEmuHwnd = hConEmuWnd ? hConEmuWnd : FindWindowExW(NULL, NULL, VirtualConsoleClassMain, NULL);
		MArray<HWND> hConEmuS;

		if (ConEmuHwnd)
		{
			if (hConEmuWnd)
			{
				hConEmuS.push_back(ConEmuHwnd);
			}
			else
			{
				// Сразу найдем список окон
				while (ConEmuHwnd)
				{
					hConEmuS.push_back(ConEmuHwnd);

					ConEmuHwnd = FindWindowExW(NULL, ConEmuHwnd, VirtualConsoleClassMain, NULL);
				}
			}
		}

		for (INT_PTR i = 0; i < hConEmuS.size(); i++)
		{
			ConEmuHwnd = hConEmuS[i];

			CESERVER_REQ *pIn = NULL, *pOut = NULL;
			int nCmdLen = lpszCmd ? lstrlenW(lpszCmd) : 1;
			int nSize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_NEWCMD) + (nCmdLen*sizeof(wchar_t));

			pIn = ExecuteNewCmd(CECMD_NEWCMD, nSize);

			if (pIn)
			{
				// "ShowHide" argument has priority before szCommand!!!
				// If need to run command in new tab of existing window,
				// "ShowHide" must be "sih_None"

				pIn->NewCmd.isAdvLogging = gpSetCls->isAdvLogging;

				pIn->NewCmd.ShowHide = gpSetCls->SingleInstanceShowHide;
				if (gpSetCls->SingleInstanceShowHide == sih_None)
				{
					if (gpConEmu->mb_StartDetached)
						pIn->NewCmd.ShowHide = sih_StartDetached;
				}
				else
				{
					_ASSERTE(gpConEmu->mb_StartDetached==FALSE);
				}

				//GetCurrentDirectory(countof(pIn->NewCmd.szCurDir), pIn->NewCmd.szCurDir);
				lstrcpyn(pIn->NewCmd.szCurDir, WorkDir(), countof(pIn->NewCmd.szCurDir));

				//GetModuleFileName(NULL, pIn->NewCmd.szConEmu, countof(pIn->NewCmd.szConEmu));
				wcscpy_c(pIn->NewCmd.szConEmu, ms_ConEmuExeDir);

				lstrcpyW(pIn->NewCmd.szCommand, lpszCmd ? lpszCmd : L"");
				DWORD dwPID = 0;

				if (GetWindowThreadProcessId(ConEmuHwnd, &dwPID))
					AllowSetForegroundWindow(dwPID);

				pOut = ExecuteGuiCmd(ConEmuHwnd, pIn, NULL);

				if (pOut && pOut->Data[0])
					lbAccepted = TRUE;
			}

			if (pIn) ExecuteFreeResult(pIn);

			if (pOut) ExecuteFreeResult(pOut);

			if (lbAccepted)
				break;

			// Попытаться со следующим окном (может быть запущено несколько экземпляров из разных путей
		}
	}

	return lbAccepted;
}

void CConEmuMain::ReportOldCmdVersion(DWORD nCmd, DWORD nVersion, int bFromServer, DWORD nFromProcess, u64 hFromModule, DWORD nBits)
{
	if (!isMainThread())
	{
		if (mb_InShowOldCmdVersion)
			return; // уже послано
		mb_InShowOldCmdVersion = TRUE;
		
		static CESERVER_REQ_HDR info = {};
		info.cbSize = sizeof(info);
		info.nCmd = nCmd;
		info.nVersion = nVersion;
		info.nSrcPID = nFromProcess;
		info.hModule = hFromModule;
		info.nBits = nBits;
		PostMessage(ghWnd, mn_MsgOldCmdVer, bFromServer, (LPARAM)&info);
		
		return;
	}

	// Один раз за сессию?
	static bool lbErrorShowed = false;
	if (lbErrorShowed) return;
	
	// Попытаемся получить информацию по процессу и модулю
	HANDLE h = NULL;
	PROCESSENTRY32 pi = {sizeof(pi)};
	MODULEENTRY32  mi = {sizeof(mi)};
	LPCTSTR pszCallPath = NULL;
	#ifdef _WIN64
	if (nBits == 64)
		h = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, nFromProcess);
	else
		h = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32, nFromProcess);
	#else
	h = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, nFromProcess);
	#endif

	wchar_t szSrcExe[MAX_PATH] = L"";

	if (h && h != INVALID_HANDLE_VALUE)
	{
		if (Module32First(h, &mi))
		{
			do {
				if (mi.th32ProcessID == nFromProcess)
				{
					if (!*szSrcExe)
					{
						LPCWSTR pszExePath = mi.szExePath[0] ? mi.szExePath : mi.szModule;
						LPCWSTR pszExt = PointToExt(pszExePath);
						if (lstrcmpi(pszExt, L".exe") == 0)
						{
							lstrcpyn(szSrcExe, pszExePath, countof(szSrcExe));
						}
					}

					if (!hFromModule || (mi.hModule == (HMODULE)hFromModule))
					{
						pszCallPath = mi.szExePath[0] ? mi.szExePath : mi.szModule;
						break;
					}
				}
				else
				{
					_ASSERTE(mi.th32ProcessID == nFromProcess);
				}
			} while (Module32Next(h, &mi));

			//if (!pszCallPath && pszExePath)
			//	pszCallPath = pszExePath;
		}
		CloseHandle(h);
	}

	if (!*szSrcExe)
	{
		h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (h && h != INVALID_HANDLE_VALUE)
		{
			if (Process32First(h, &pi))
			{
				do {
					if (pi.th32ProcessID == nFromProcess)
					{
						lstrcpyn(szSrcExe, pi.szExeFile, countof(szSrcExe));
						break;
					}
				} while (Process32Next(h, &pi));
			}
			CloseHandle(h);
		}
	}

	if (!pszCallPath && *szSrcExe)
	{
		LPCWSTR pszExeName = PointToName(szSrcExe);
		if (IsFarExe(pszExeName))
		{
			pszCallPath = L"(Check plugins in %FARHOME%\\Plugins folder)";
		}
	}

	lbErrorShowed = true;
	int nMaxLen = 255+(pszCallPath ? _tcslen(pszCallPath) : 0)+_tcslen(ms_ConEmuExe)+_tcslen(szSrcExe)+_tcslen(ms_ConEmuDefTitle);
	wchar_t *pszMsg = (wchar_t*)calloc(nMaxLen,sizeof(wchar_t));
	_wsprintf(pszMsg, SKIPLEN(nMaxLen)
		L"%s received wrong version packet!\n"
		L"%s\n\n"
		L"CommandID: %i, Version: %i, ReqVersion: %i, PID: %u\n"
		L"%s\n%s\n%s"
		L"Please check installation files",
		ms_ConEmuDefTitle, ms_ConEmuExe,
		nCmd, nVersion, CESERVER_REQ_VER, nFromProcess,
		szSrcExe, pszCallPath ? pszCallPath : L"", pszCallPath ? L"\n" : L""
		/*(bFromServer==1) ? L"ConEmuC*.exe" : (bFromServer==0) ? L"ConEmu*.dll and Far plugins" : L"ConEmuC*.exe and ConEmu*.dll"*/
		);
	MBox(pszMsg);
	free(pszMsg);
	mb_InShowOldCmdVersion = FALSE; // теперь можно показать еще одно...
}

bool CConEmuMain::LockConhostStart()
{
	// ConHost.exe появился в Windows 7. Но там он создается "от родительского csrss".
	// А вот в Win8 - уже все хорошо, он создается от корневого консольного процесса.
	bool bNeedConHostSearch = (gnOsVer == 0x0601);
	if (!bNeedConHostSearch)
		return false;
	EnterCriticalSection(&m_LockConhostStart.cs);
	if (m_LockConhostStart.wait)
	{
		// Если консоли создавать слишком быстро - может возникнуть проблема идентификации принадлежности процессов
		Sleep(250);
	}
	m_LockConhostStart.wait = true;
	return true;
}
void CConEmuMain::UnlockConhostStart()
{
	bool bNeedConHostSearch = (gnOsVer == 0x0601);
	if (!bNeedConHostSearch)
		return;
	LeaveCriticalSection(&m_LockConhostStart.cs);
}

void CConEmuMain::ReleaseConhostDelay()
{
	m_LockConhostStart.wait = false;
}

// Запуск отладки текущего GUI
bool CConEmuMain::StartDebugLogConsole()
{
	if (IsDebuggerPresent())
		return false; // УЖЕ!

	// Create process, with flag /Attach GetCurrentProcessId()
	// Sleep for sometimes, try InitHWND(hConWnd); several times
	WCHAR  szExe[MAX_PATH*2] = {0};
	bool lbRc = false;
	//DWORD nLen = 0;
	PROCESS_INFORMATION pi = {NULL};
	STARTUPINFO si = {sizeof(si)};
	DWORD dwSelfPID = GetCurrentProcessId();
	// "/ATTACH" - низя, а то заблокируемся при попытке подключения к "отлаживаемому" GUI
	_wsprintf(szExe, SKIPLEN(countof(szExe)) L"\"%s\" /DEBUGPID=%u /BW=80 /BH=25 /BZ=%u",
	          WIN3264TEST(ms_ConEmuC32Full,ms_ConEmuC64Full), dwSelfPID, LONGOUTPUTHEIGHT_MAX);

	#ifdef _DEBUG
	if (MessageBox(NULL, szExe, L"StartDebugLogConsole", MB_OKCANCEL|MB_SYSTEMMODAL) != IDOK)
		return false;
	#endif
	          
	if (!CreateProcess(NULL, szExe, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS|CREATE_NEW_CONSOLE, NULL,
	                  NULL, &si, &pi))
	{
		// Хорошо бы ошибку показать?
		DWORD dwErr = GetLastError();
		wchar_t szErr[128]; _wsprintf(szErr, SKIPLEN(countof(szErr)) L"Can't create debugger console! ErrCode=0x%08X", dwErr);
		MBoxA(szErr);
	}
	else
	{
		gbDebugLogStarted = TRUE;
		SafeCloseHandle(pi.hProcess);
		SafeCloseHandle(pi.hThread);
		lbRc = true;
	}

	return lbRc;
}

bool CConEmuMain::StartDebugActiveProcess()
{
	CVConGuard VCon;
	CRealConsole* pRCon = (CVConGroup::GetActiveVCon(&VCon) >= 0) ? VCon->RCon() : NULL;
	if (!pRCon)
		return false;
	DWORD dwPID = pRCon->GetActivePID();
	if (!dwPID)
		return false;

	// Create process, with flag /Attach GetCurrentProcessId()
	// Sleep for sometimes, try InitHWND(hConWnd); several times
	WCHAR  szExe[0x400] = {0};
	bool lbRc = false;
	//DWORD nLen = 0;
	PROCESS_INFORMATION pi = {NULL};
	STARTUPINFO si = {sizeof(si)};
	WARNING("Наверное лучше переделать на CreateCon...");
	si.dwFlags |= STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	DWORD dwSelfPID = GetCurrentProcessId();
	int W = pRCon->TextWidth();
	int H = pRCon->TextHeight();
	int nBits = GetProcessBits(dwPID);
	LPCWSTR pszServer = (nBits == 64) ? ms_ConEmuC64Full : ms_ConEmuC32Full;
	_wsprintf(szExe, SKIPLEN(countof(szExe)) L"\"%s\" /ATTACH /GID=%i /GHWND=%08X /BW=%i /BH=%i /BZ=%u /ROOT \"%s\" /DEBUGPID=%i ",
		pszServer, dwSelfPID, (DWORD)ghWnd, W, H, LONGOUTPUTHEIGHT_MAX, pszServer, dwPID);

	#ifdef _DEBUG
	if (MessageBox(NULL, szExe, L"StartDebugLogConsole", MB_OKCANCEL|MB_SYSTEMMODAL) != IDOK)
		return false;
	#endif
		
	if (!CreateProcess(NULL, szExe, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS|CREATE_NEW_CONSOLE, NULL,
		NULL, &si, &pi))
	{
		// Хорошо бы ошибку показать?
		DWORD dwErr = GetLastError();
		wchar_t szErr[128]; _wsprintf(szErr, SKIPLEN(countof(szErr)) L"Can't create debugger console! ErrCode=0x%08X", dwErr);
		MBoxA(szErr);
	}
	else
	{
		gbDebugLogStarted = TRUE;
		SafeCloseHandle(pi.hProcess);
		SafeCloseHandle(pi.hThread);
		lbRc = true;
	}

	return lbRc;
}

bool CConEmuMain::MemoryDumpActiveProcess()
{
	CVConGuard VCon;
	CRealConsole* pRCon = (CVConGroup::GetActiveVCon(&VCon) >= 0) ? VCon->RCon() : NULL;
	if (!pRCon)
		return false;
	DWORD dwPID = pRCon->GetActivePID();
	if (!dwPID)
		return false;

	// Create process, with flag /Attach GetCurrentProcessId()
	// Sleep for sometimes, try InitHWND(hConWnd); several times
	WCHAR  szExe[0x400] = {0};
	bool lbRc = false;
	//DWORD nLen = 0;
	PROCESS_INFORMATION pi = {NULL};
	STARTUPINFO si = {sizeof(si)};
	si.dwFlags |= STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOWNORMAL;
	int nBits = GetProcessBits(dwPID);
	LPCWSTR pszServer = (nBits == 64) ? ms_ConEmuC64Full : ms_ConEmuC32Full;
	_wsprintf(szExe, SKIPLEN(countof(szExe)) L"\"%s\" /DEBUGPID=%i /DUMP", pszServer, dwPID);

	//#ifdef _DEBUG
	//if (MessageBox(NULL, szExe, L"StartDebugLogConsole", MB_OKCANCEL|MB_SYSTEMMODAL) != IDOK)
	//	return;
	//#endif
		
	if (!CreateProcess(NULL, szExe, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS|CREATE_NEW_CONSOLE, NULL,
		NULL, &si, &pi))
	{
		// Хорошо бы ошибку показать?
		DWORD dwErr = GetLastError();
		wchar_t szErr[128]; _wsprintf(szErr, SKIPLEN(countof(szErr)) L"Can't create debugger console! ErrCode=0x%08X", dwErr);
		MBoxA(szErr);
	}
	else
	{
		gbDebugLogStarted = TRUE;
		SafeCloseHandle(pi.hProcess);
		SafeCloseHandle(pi.hThread);
		lbRc = true;
	}

	return lbRc;
}

void CConEmuMain::UpdateProcessDisplay(BOOL abForce)
{
	if (!ghOpWnd)
		return;

	if (!isMainThread())
	{
		PostMessage(ghWnd, mn_MsgUpdateProcDisplay, abForce, 0);
		return;
	}

	HWND hInfo = gpSetCls->mh_Tabs[gpSetCls->thi_Info];

	CVConGuard VCon;
	GetActiveVCon(&VCon);

	wchar_t szNo[32], szFlags[255]; szNo[0] = szFlags[0] = 0;
	DWORD nProgramStatus = VCon->RCon()->GetProgramStatus();
	DWORD nFarStatus = VCon->RCon()->GetFarStatus();
	if (nProgramStatus&CES_TELNETACTIVE) wcscat_c(szFlags, L"Telnet ");
	//CheckDlgButton(hInfo, cbsTelnetActive, (nProgramStatus&CES_TELNETACTIVE) ? BST_CHECKED : BST_UNCHECKED);
	if (nProgramStatus&CES_NTVDM) wcscat_c(szFlags, L"16bit ");
	//CheckDlgButton(hInfo, cbsNtvdmActive, (nProgramStatus&CES_NTVDM) ? BST_CHECKED : BST_UNCHECKED);
	if (nProgramStatus&CES_FARACTIVE) wcscat_c(szFlags, L"Far ");
	//CheckDlgButton(hInfo, cbsFarActive, (nProgramStatus&CES_FARACTIVE) ? BST_CHECKED : BST_UNCHECKED);
	if (nFarStatus&CES_FILEPANEL) wcscat_c(szFlags, L"Panels ");
	//CheckDlgButton(hInfo, cbsFilePanel, (nFarStatus&CES_FILEPANEL) ? BST_CHECKED : BST_UNCHECKED);
	if (nFarStatus&CES_EDITOR) wcscat_c(szFlags, L"Editor ");
	//CheckDlgButton(hInfo, cbsEditor, (nFarStatus&CES_EDITOR) ? BST_CHECKED : BST_UNCHECKED);
	if (nFarStatus&CES_VIEWER) wcscat_c(szFlags, L"Viewer ");
	//CheckDlgButton(hInfo, cbsViewer, (nFarStatus&CES_VIEWER) ? BST_CHECKED : BST_UNCHECKED);
	if (nFarStatus&CES_WASPROGRESS) wcscat_c(szFlags, L"%%Progress ");
	//CheckDlgButton(hInfo, cbsProgress, ((nFarStatus&CES_WASPROGRESS) /*|| VCon->RCon()->GetProgress(NULL)>=0*/) ? BST_CHECKED : BST_UNCHECKED);
	if (nFarStatus&CES_OPER_ERROR) wcscat_c(szFlags, L"%%Error ");
	//CheckDlgButton(hInfo, cbsProgressError, (nFarStatus&CES_OPER_ERROR) ? BST_CHECKED : BST_UNCHECKED);
	_wsprintf(szNo, SKIPLEN(countof(szNo)) L"%i/%i", VCon->RCon()->GetFarPID(), VCon->RCon()->GetFarPID(TRUE));

	if (hInfo)
	{
		SetDlgItemText(hInfo, tsTopPID, szNo);
		SetDlgItemText(hInfo, tsRConFlags, szFlags);
	}

	if (!abForce)
		return;

	MCHKHEAP;

	if (hInfo)
	{
		CVConGroup::OnUpdateProcessDisplay(hInfo);
	}

	MCHKHEAP
}

void CConEmuMain::UpdateCursorInfo(const CONSOLE_SCREEN_BUFFER_INFO* psbi, COORD crCursor, CONSOLE_CURSOR_INFO cInfo)
{
	if (psbi)
		mp_Status->OnConsoleChanged(psbi, &cInfo, false);
	else
		mp_Status->OnCursorChanged(&crCursor, &cInfo);

	if (!ghOpWnd || !gpSetCls->mh_Tabs[gpSetCls->thi_Info]) return;

	if (!isMainThread())
	{
		DWORD wParam = MAKELONG(crCursor.X, crCursor.Y);
		DWORD lParam = MAKELONG(cInfo.dwSize, cInfo.bVisible);
		PostMessage(ghWnd, mn_MsgUpdateCursorInfo, wParam, lParam);
		return;
	}

	TCHAR szCursor[64];
	_wsprintf(szCursor, SKIPLEN(countof(szCursor)) _T("%ix%i, %i %s"),
		(int)crCursor.X, (int)crCursor.Y,
		cInfo.dwSize, cInfo.bVisible ? L"vis" : L"hid");
	SetDlgItemText(gpSetCls->mh_Tabs[gpSetCls->thi_Info], tCursorPos, szCursor);
}

void CConEmuMain::UpdateSizes()
{
	POINT ptCur = {}; GetCursorPos(&ptCur);
	HWND hPoint = WindowFromPoint(ptCur);

	HWND hInfo = gpSetCls->mh_Tabs[gpSetCls->thi_Info];

	if (!ghOpWnd || !hInfo)
	{
		// Может курсор-сплиттер нужно убрать или поставить
		if (hPoint && ((hPoint == ghWnd) || (GetParent(hPoint) == ghWnd)))
		{
			PostMessage(ghWnd, WM_SETCURSOR, -1, -1);
		}
		return;
	}

	if (!isMainThread())
	{
		PostMessage(ghWnd, mn_MsgUpdateSizes, 0, 0);
		return;
	}

	// Может курсор-сплиттер нужно убрать или поставить
	if (hPoint && ((hPoint == ghWnd) || (GetParent(hPoint) == ghWnd)))
	{
		SendMessage(ghWnd, WM_SETCURSOR, -1, -1);
	}

	CVConGuard guard;
	GetActiveVCon(&guard);
	CVirtualConsole* pVCon = guard.VCon();

	if (pVCon)
	{
		pVCon->UpdateInfo();
	}
	else
	{
		SetDlgItemText(hInfo, tConSizeChr, _T("?"));
		SetDlgItemText(hInfo, tConSizePix, _T("?"));
		SetDlgItemText(hInfo, tPanelLeft, _T("?"));
		SetDlgItemText(hInfo, tPanelRight, _T("?"));
	}

	if (pVCon && pVCon->GetView())
	{
		RECT rcClient = pVCon->GetDcClientRect();
		TCHAR szSize[32]; _wsprintf(szSize, SKIPLEN(countof(szSize)) _T("%ix%i"), rcClient.right, rcClient.bottom);
		SetDlgItemText(hInfo, tDCSize, szSize);
	}
	else
	{
		SetDlgItemText(hInfo, tDCSize, L"<none>");
	}
}

void CConEmuMain::CheckNeedUpdateTitle(LPCWSTR asRConTitle)
{
	if (!asRConTitle)
		return;

	// Issue 1004: overflow of mn_MsgUpdateTitle when "/title" option was specified
	if (TitleTemplate[0] != 0)
		return;

	// Если в консоли заголовок не менялся, но он отличается от заголовка в ConEmu
	if (wcscmp(asRConTitle, GetLastTitle(false)))
		UpdateTitle();
}

// !!!Warning!!! Никаких return. в конце функции вызывается необходимый CheckProcesses
void CConEmuMain::UpdateTitle(/*LPCTSTR asNewTitle*/)
{
	if (GetCurrentThreadId() != mn_MainThreadId)
	{
		/*if (TitleCmp != asNewTitle) -- можем наколоться на многопоточности. Лучше получим повторно
		    wcscpy(TitleCmp, asNewTitle);*/
		PostMessage(ghWnd, mn_MsgUpdateTitle, 0, 0);
		return;
	}

	LPCTSTR pszNewTitle = NULL;

	if (TitleTemplate[0] != 0)
	{
		DWORD n = ExpandEnvironmentStrings(TitleTemplate, Title, countof(Title));
		if (n && (n < countof(Title)))
			pszNewTitle = Title;
		else
			pszNewTitle = GetDefaultTitle();
	}

	if (!pszNewTitle)
	{
		CVConGuard VCon;
		if ((GetActiveVCon(&VCon) >= 0) && VCon->RCon())
			pszNewTitle = VCon->RCon()->GetTitle();
	}

	if (!pszNewTitle)
	{
		//if ((pszNewTitle = mp_ VActive->RCon()->GetTitle()) == NULL)
		//	return;
		pszNewTitle = GetDefaultTitle();
	}


	if (pszNewTitle && (pszNewTitle != Title) && lstrcmpi(Title, pszNewTitle))
		lstrcpyn(Title, pszNewTitle, countof(Title));


	// SetWindowText(ghWnd, psTitle) вызывается здесь
	// Там же обновится L"[%i/%i] " если несколько консолей а табы отключены
	UpdateProgress(/*TRUE*/);
	Icon.UpdateTitle();
	// Под конец - проверить список процессов консоли
	CheckProcesses();
}

// Если в текущей консоли есть проценты - отображаются они
// Иначе - отображается максимальное значение процентов из всех консолей
void CConEmuMain::UpdateProgress()
{
	if (!this || (ghWnd == NULL))
	{
		_ASSERTE(this && ghWnd);
		return;
	}

	if (GetCurrentThreadId() != mn_MainThreadId)
	{
		/*чтобы не наколоться на многопоточности */
		PostMessage(ghWnd, mn_MsgUpdateTitle, 0, 0);
		return;
	}

	LPCWSTR psTitle = NULL;
	LPCWSTR pszFixTitle = GetLastTitle(true);
	wchar_t MultiTitle[MAX_TITLE_SIZE+30];
	MultiTitle[0] = 0;
	short nProgress = -1;
	BOOL bActiveHasProgress = FALSE;
	BOOL bNeedAddToTitle = FALSE;
	BOOL bWasError = FALSE;
	BOOL bWasIndeterminate = FALSE;

	CVConGroup::GetProgressInfo(&nProgress, &bActiveHasProgress, &bWasError, &bWasIndeterminate);

	mn_Progress = min(nProgress,100);

	if (!bActiveHasProgress)
	{
		if (!bNeedAddToTitle && (nProgress >= 0))
			bNeedAddToTitle = TRUE;
	}

	static short nLastProgress = -1;
	static BOOL  bLastProgressError = FALSE;
	static BOOL  bLastProgressIndeterminate = FALSE;

	if (nLastProgress != mn_Progress  || bLastProgressError != bWasError || bLastProgressIndeterminate != bWasIndeterminate)
	{
		HRESULT hr = S_OK;

		//if (mp_TaskBar3)
		//{
		if ((mn_Progress >= 0) && gpSet->isTaskbarProgress)
		{
			hr = Taskbar_SetProgressValue(mn_Progress);

			if (nLastProgress == -1 || bLastProgressError != bWasError || bLastProgressIndeterminate != bWasIndeterminate)
				hr = Taskbar_SetProgressState(bWasError ? TBPF_ERROR : bWasIndeterminate ? TBPF_INDETERMINATE : TBPF_NORMAL);
		}
		else
		{
			hr = Taskbar_SetProgressState(TBPF_NOPROGRESS);
		}
		//}

		// Запомнить последнее
		nLastProgress = mn_Progress;
		bLastProgressError = bWasError;
		bLastProgressIndeterminate = bWasIndeterminate;
		UNREFERENCED_PARAMETER(hr);
	}

	if ((mn_Progress >= 0) && bNeedAddToTitle)
	{
		psTitle = MultiTitle;
		wsprintf(MultiTitle+_tcslen(MultiTitle), L"{*%i%%} ", mn_Progress);
	}

	if (gpSetCls->IsMulti() && (gpSet->isNumberInCaption || !gpConEmu->mp_TabBar->IsTabsShown()))
	{
		int nCur = 1, nCount = 0;

		nCur = GetActiveVCon(NULL, &nCount);
		if (nCur < 0)
			nCur = (nCount > 0) ? 1 : 0;
		else
			nCur++;

		if (gpSet->isNumberInCaption || (nCount > 1))
		{
			psTitle = MultiTitle;
			wsprintf(MultiTitle+_tcslen(MultiTitle), L"[%i/%i] ", nCur, nCount);
		}
	}

	if (psTitle)
		wcscat(MultiTitle, pszFixTitle);
	else
		psTitle = pszFixTitle;

	SetWindowText(ghWnd, psTitle);

	// Задел на будущее
	if (ghWndApp)
		SetWindowText(ghWndApp, psTitle);
}

void CConEmuMain::StartForceShowFrame()
{
	SetKillTimer(false, TIMER_CAPTION_APPEAR_ID, 0);
	SetKillTimer(false, TIMER_CAPTION_DISAPPEAR_ID, 0);
	m_ForceShowFrame = fsf_Show;
	OnHideCaption();
	//UpdateWindowRgn();
}

void CConEmuMain::StopForceShowFrame()
{
	m_ForceShowFrame = fsf_Hide;
	SetKillTimer(false, TIMER_CAPTION_APPEAR_ID, 0);
	SetKillTimer(false, TIMER_CAPTION_DISAPPEAR_ID, 0);
	OnHideCaption();
	//UpdateWindowRgn();
}

void CConEmuMain::UpdateWindowRgn(int anX/*=-1*/, int anY/*=-1*/, int anWndWidth/*=-1*/, int anWndHeight/*=-1*/)
{
	if (mb_LockWindowRgn)
		return;

	HRGN hRgn = NULL;

	//if (gpSet->isHideCaptionAlways) {
	//	SetKillTimer(false, TIMER_CAPTION_APPEAR_ID, 0);
	//	SetKillTimer(false, TIMER_CAPTION_DISAPPEAR_ID, 0);
	//}

	if (m_ForceShowFrame == fsf_Show)
		hRgn = NULL; // Иначе при ресайзе получаются некрасивые (без XP Theme) кнопки в заголовке
	else if (anWndWidth != -1 && anWndHeight != -1)
		hRgn = CreateWindowRgn(false, false, anX, anY, anWndWidth, anWndHeight);
	else
		hRgn = CreateWindowRgn();

	if (hRgn)
	{
		mb_LastRgnWasNull = FALSE;
	}
	else
	{
		BOOL lbPrev = mb_LastRgnWasNull;
		mb_LastRgnWasNull = TRUE;

		if (lbPrev)
			return; // менять не нужно
	}

	if (gpSetCls->isAdvLogging)
	{
		char szInfo[128];
		RECT rcBox = {};
		int nRgn = hRgn ? GetRgnBox(hRgn, &rcBox) : NULLREGION;
		_wsprintfA(szInfo, SKIPLEN(countof(szInfo))
			nRgn ? "SetWindowRgn(0x%08X, <%u> {%i,%i}-{%i,%i})" : "SetWindowRgn(0x%08X, NULL)",
			ghWnd, nRgn, rcBox.left, rcBox.top, rcBox.right, rcBox.bottom);
		LogString(szInfo);
	}

	// Облом получается при двукратном SetWindowRgn(ghWnd, NULL, TRUE);
	// Причем после следующего ресайза - рамка приходит в норму
	bool b = SetDontPreserveClient(true);
	BOOL bRc = SetWindowRgn(ghWnd, hRgn, TRUE);
	DWORD dwErr = bRc ? 0 : GetLastError();
	SetDontPreserveClient(b);
	UNREFERENCED_PARAMETER(dwErr);
	UNREFERENCED_PARAMETER(bRc);
}

#ifndef _WIN64
VOID CConEmuMain::WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD anEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
	_ASSERTE(hwnd!=NULL);

	#ifdef _DEBUG
	switch(anEvent)
	{
		case EVENT_CONSOLE_START_APPLICATION:

			//A new console process has started.
			//The idObject parameter contains the process identifier of the newly created process.
			//If the application is a 16-bit application, the idChild parameter is CONSOLE_APPLICATION_16BIT and idObject is the process identifier of the NTVDM session associated with the console.
			if ((idChild == CONSOLE_APPLICATION_16BIT) && gpSetCls->isAdvLogging)
			{
				char szInfo[64]; wsprintfA(szInfo, "NTVDM started, PID=%i\n", idObject);
				gpConEmu->LogString(szInfo, TRUE);
			}

			#ifdef _DEBUG
			WCHAR szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"EVENT_CONSOLE_START_APPLICATION(HWND=0x%08X, PID=%i%s)\n", (DWORD)hwnd, idObject, (idChild == CONSOLE_APPLICATION_16BIT) ? L" 16bit" : L"");
			DEBUGSTRCONEVENT(szDbg);
			#endif

			break;
		case EVENT_CONSOLE_END_APPLICATION:

			//A console process has exited.
			//The idObject parameter contains the process identifier of the terminated process.
			if ((idChild == CONSOLE_APPLICATION_16BIT) && gpSetCls->isAdvLogging)
			{
				char szInfo[64]; wsprintfA(szInfo, "NTVDM stopped, PID=%i\n", idObject);
				gpConEmu->LogString(szInfo, TRUE);
			}

			#ifdef _DEBUG
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"EVENT_CONSOLE_END_APPLICATION(HWND=0x%08X, PID=%i%s)\n", (DWORD)hwnd, idObject,
			          (idChild == CONSOLE_APPLICATION_16BIT) ? L" 16bit" : L"");
			DEBUGSTRCONEVENT(szDbg);
			#endif

			break;
		case EVENT_CONSOLE_UPDATE_REGION: // 0x4002
		{
			//More than one character has changed.
			//The idObject parameter is a COORD structure that specifies the start of the changed region.
			//The idChild parameter is a COORD structure that specifies the end of the changed region.
			#ifdef _DEBUG
			COORD crStart, crEnd; memmove(&crStart, &idObject, sizeof(idObject)); memmove(&crEnd, &idChild, sizeof(idChild));
			WCHAR szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"EVENT_CONSOLE_UPDATE_REGION({%i, %i} - {%i, %i})\n", crStart.X,crStart.Y, crEnd.X,crEnd.Y);
			DEBUGSTRCONEVENT(szDbg);
			#endif
		} break;
		case EVENT_CONSOLE_UPDATE_SCROLL: //0x4004
		{
			//The console has scrolled.
			//The idObject parameter is the horizontal distance the console has scrolled.
			//The idChild parameter is the vertical distance the console has scrolled.
			#ifdef _DEBUG
			WCHAR szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"EVENT_CONSOLE_UPDATE_SCROLL(X=%i, Y=%i)\n", idObject, idChild);
			DEBUGSTRCONEVENT(szDbg);
			#endif
		} break;
		case EVENT_CONSOLE_UPDATE_SIMPLE: //0x4003
		{
			//A single character has changed.
			//The idObject parameter is a COORD structure that specifies the character that has changed.
			//Warning! В писании от  микрософта тут ошибка (после репорта была исправлена)
			//The idChild parameter specifies the character in the low word and the character attributes in the high word.
			#ifdef _DEBUG
			COORD crWhere; memmove(&crWhere, &idObject, sizeof(idObject));
			WCHAR ch = (WCHAR)LOWORD(idChild); WORD wA = HIWORD(idChild);
			WCHAR szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"EVENT_CONSOLE_UPDATE_SIMPLE({%i, %i} '%c'(\\x%04X) A=%i)\n", crWhere.X,crWhere.Y, ch, ch, wA);
			DEBUGSTRCONEVENT(szDbg);
			#endif
		} break;
		case EVENT_CONSOLE_CARET: //0x4001
		{
			//Warning! WinXPSP3. Это событие проходит ТОЛЬКО если консоль в фокусе.
			//         А с ConEmu она НИКОГДА не в фокусе, так что курсор не обновляется.
			//The console caret has moved.
			//The idObject parameter is one or more of the following values:
			//      CONSOLE_CARET_SELECTION or CONSOLE_CARET_VISIBLE.
			//The idChild parameter is a COORD structure that specifies the cursor's current position.
			#ifdef _DEBUG
			COORD crWhere; memmove(&crWhere, &idChild, sizeof(idChild));
			WCHAR szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"EVENT_CONSOLE_CARET({%i, %i} Sel=%c, Vis=%c\n", crWhere.X,crWhere.Y,
			                            ((idObject & CONSOLE_CARET_SELECTION)==CONSOLE_CARET_SELECTION) ? L'Y' : L'N',
			                            ((idObject & CONSOLE_CARET_VISIBLE)==CONSOLE_CARET_VISIBLE) ? L'Y' : L'N');
			DEBUGSTRCONEVENT(szDbg);
			#endif
		} break;
		case EVENT_CONSOLE_LAYOUT: //0x4005
		{
			//The console layout has changed.
			DEBUGSTRCONEVENT(L"EVENT_CONSOLE_LAYOUT\n");
		} break;
	}
	#endif

	// Интересуют только 16битные приложения.
	if (!(((anEvent == EVENT_CONSOLE_START_APPLICATION) || (anEvent == EVENT_CONSOLE_END_APPLICATION))
			&& (idChild == CONSOLE_APPLICATION_16BIT)))
		return;

	StartStopType sst = (anEvent == EVENT_CONSOLE_START_APPLICATION) ? sst_App16Start : sst_App16Stop;

	CVConGroup::OnDosAppStartStop(hwnd, sst, idChild);

}
#endif

/* ****************************************************** */
/*                                                        */
/*                      Painting                          */
/*                                                        */
/* ****************************************************** */

#ifdef colorer_func
{
	Painting() {};
}
#endif

void CConEmuMain::InitInactiveDC(CVirtualConsole* apVCon)
{
	PostMessage(ghWnd, mn_MsgInitInactiveDC, 0, (LPARAM)apVCon);
}

void CConEmuMain::Invalidate(LPRECT lpRect, BOOL bErase /*= TRUE*/)
{
	#ifdef _DEBUG
	static bool bSkip = false;
	if (bSkip)
		return;
	#endif
	::InvalidateRect(ghWnd, lpRect, bErase);
}

void CConEmuMain::InvalidateAll()
{
	Invalidate(NULL, TRUE);

	CVConGroup::InvalidateAll();

	CVConGroup::InvalidateGaps();

	if (mp_TabBar)
		mp_TabBar->Invalidate();

	//-- No need to invalidate status due to Invalidate(NULL) called above
	//if (mp_Status)
	//	mp_Status->InvalidateStatusBar();
}

void CConEmuMain::UpdateWindowChild(CVirtualConsole* apVCon)
{
	CVConGroup::UpdateWindowChild(apVCon);
}

//bool CConEmuMain::IsGlass()
//{
//	if (gOSVer.dwMajorVersion < 6)
//		return false;
//
//	if (!mh_DwmApi)
//	{
//		mh_DwmApi = LoadLibrary(L"dwmapi.dll");
//
//		if (!mh_DwmApi)
//			mh_DwmApi = (HMODULE)INVALID_HANDLE_VALUE;
//	}
//
//	if (mh_DwmApi != INVALID_HANDLE_VALUE)
//		return false;
//
//	if (!DwmIsCompositionEnabled && mh_DwmApi)
//	{
//		DwmIsCompositionEnabled = (FDwmIsCompositionEnabled)GetProcAddress(mh_DwmApi, "DwmIsCompositionEnabled");
//
//		if (!DwmIsCompositionEnabled)
//			return false;
//	}
//
//	BOOL composition_enabled = FALSE;
//	return DwmIsCompositionEnabled(&composition_enabled) == S_OK &&
//	       composition_enabled /*&& g_glass*/;
//}

//LRESULT CConEmuMain::OnNcMessage(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
//{
//	LRESULT lRc = 0;
//
//	if (hWnd != ghWnd)
//	{
//		lRc = DefWindowProc(hWnd, messg, wParam, lParam);
//	}
//	else
//	{
//		BOOL lbUseCaption = gpSet->isTabsInCaption && gpConEmu->mp_TabBar->IsActive();
//
//		switch(messg)
//		{
//			case WM_NCHITTEST:
//			{
//				lRc = DefWindowProc(hWnd, messg, wParam, lParam);
//
//				if (gpSet->isHideCaptionAlways() && !gpConEmu->mp_TabBar->IsShown() && lRc == HTTOP)
//					lRc = HTCAPTION;
//
//				break;
//			}
//			case WM_NCPAINT:
//			{
//				if (lbUseCaption)
//					lRc = gpConEmu->OnNcPaint((HRGN)wParam);
//				else
//					lRc = DefWindowProc(hWnd, messg, wParam, lParam);
//
//				break;
//			}
//			case WM_NCACTIVATE:
//			{
//				// Force paint our non-client area otherwise Windows will paint its own.
//				if (lbUseCaption)
//				{
//					// Тут все переделать нужно, когда табы новые будут
//					_ASSERTE(lbUseCaption==FALSE);
//					RedrawWindow(hWnd, NULL, NULL, RDW_UPDATENOW);
//				}
//				else
//				{
//					// При потере фокуса
//					if (!wParam)
//					{
//						CVirtualConsole* pVCon = ActiveCon();
//						CRealConsole* pRCon;
//						// Нужно схалтурить, чтобы при переходе фокуса в дочернее GUI приложение
//						// рамка окна самого ConEmu не стала неактивной (серой)
//						if (pVCon && ((pRCon = ActiveCon()->RCon()) != NULL) && pRCon->GuiWnd() && !pRCon->isBufferHeight())
//							wParam = TRUE;
//					}
//					lRc = DefWindowProc(hWnd, messg, wParam, lParam);
//				}
//
//				break;
//			}
//			case 0x31E: // WM_DWMCOMPOSITIONCHANGED:
//			{
//				if (lbUseCaption)
//				{
//					lRc = 0;
//					/*
//					DWMNCRENDERINGPOLICY policy = g_glass ? DWMNCRP_ENABLED : DWMNCRP_DISABLED;
//					DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY,
//					                    &policy, sizeof(DWMNCRENDERINGPOLICY));
//
//					SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
//					           SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
//					RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
//					*/
//				}
//				else
//					lRc = DefWindowProc(hWnd, messg, wParam, lParam);
//
//				break;
//			}
//			case 0xAE: // WM_NCUAHDRAWCAPTION:
//			case 0xAF: // WM_NCUAHDRAWFRAME:
//			{
//				lRc = lbUseCaption ? 0 : DefWindowProc(hWnd, messg, wParam, lParam);
//				break;
//			}
//			case WM_NCCALCSIZE:
//			{
//				NCCALCSIZE_PARAMS *pParms = NULL;
//				LPRECT pRect = NULL;
//				if (wParam) pParms = (NCCALCSIZE_PARAMS*)lParam; else pRect = (LPRECT)lParam;
//
//				lRc = DefWindowProc(hWnd, messg, wParam, lParam);
//				break;
//			}
//			default:
//				if (messg == WM_NCLBUTTONDOWN && wParam == HTSYSMENU)
//					GetSysMenu(); // Проверить корректность системного меню
//				lRc = DefWindowProc(hWnd, messg, wParam, lParam);
//		}
//	}
//
//	return lRc;
//}

//LRESULT CConEmuMain::OnNcPaint(HRGN hRgn)
//{
//	LRESULT lRc = 0, lMyRc = 0;
//	//HRGN hFrameRgn = hRgn;
//	RECT wr = {0}, dirty = {0}, dirty_box = {0};
//	GetWindowRect(ghWnd, &wr);
//
//	if (!hRgn || ((WPARAM)hRgn) == 1)
//	{
//		dirty = wr;
//		dirty.left = dirty.top = 0;
//	}
//	else
//	{
//		GetRgnBox(hRgn, &dirty_box);
//
//		if (!IntersectRect(&dirty, &dirty_box, &wr))
//			return 0;
//
//		OffsetRect(&dirty, -wr.left, -wr.top);
//	}
//
//	//hdc = GetWindowDC(hwnd);
//	//br = CreateSolidBrush(RGB(255,0,0));
//	//FillRect(hdc, &dirty, br);
//	//DeleteObject(br);
//	//ReleaseDC(hwnd, hdc);
//	int nXFrame = GetSystemMetrics(SM_CXSIZEFRAME);
//	int nYFrame = GetSystemMetrics(SM_CYSIZEFRAME);
//	int nPad = GetSystemMetrics(92/*SM_CXPADDEDBORDER*/);
//	int nXBtn = GetSystemMetrics(SM_CXSIZE);
//	int nYBtn = GetSystemMetrics(SM_CYSIZE);
//	int nYCaption = GetSystemMetrics(SM_CYCAPTION);
//	int nXMBtn = GetSystemMetrics(SM_CXSMSIZE);
//	int nYMBtn = GetSystemMetrics(SM_CYSMSIZE);
//	int nXIcon = GetSystemMetrics(SM_CXSMICON);
//	int nYIcon = GetSystemMetrics(SM_CYSMICON);
//	RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);
//	RECT rcUsing =
//	{
//		nXFrame + nXIcon, nYFrame,
//		(rcWnd.right-rcWnd.left) - nXFrame - 3*nXBtn,
//		nYFrame + nYCaption
//	};
//	//lRc = DefWindowProc(ghWnd, WM_NCPAINT, (WPARAM)hRgn, 0);
//	//HRGN hRect = CreateRectRgn(rcUsing.left,rcUsing.top,rcUsing.right,rcUsing.bottom);
//	//hFrameRgn = CreateRectRgn(0,0,1,1);
//	//int nRgn = CombineRgn(hFrameRgn, hRgn, hRect, RGN_XOR);
//	//if (nRgn == ERROR)
//	//{
//	//	DeleteObject(hFrameRgn);
//	//	hFrameRgn = hRgn;
//	//}
//	//else
//	{
//		// Рисуем
//		HDC hdc = NULL;
//		//hdc = GetDCEx(ghWnd, hRect, DCX_WINDOW|DCX_INTERSECTRGN);
//		//hRect = NULL; // system maintains this region
//		hdc = GetWindowDC(ghWnd);
//		mp_TabBar->PaintHeader(hdc, rcUsing);
//		ReleaseDC(ghWnd, hdc);
//	}
//	//if (hRect)
//	//	DeleteObject(hRect);
//	lMyRc = TRUE;
//	//lRc = DefWindowProc(ghWnd, WM_NCPAINT, (WPARAM)hFrameRgn, 0);
//	//
//	//if (hRgn != hFrameRgn)
//	//{
//	//	DeleteObject(hFrameRgn); hFrameRgn = NULL;
//	//}
//	return lMyRc ? lMyRc : lRc;
//}

bool CConEmuMain::isRightClickingPaint()
{
	return (bool)mb_RightClickingPaint;
}

void CConEmuMain::RightClickingPaint(HDC hdcIntVCon, CVirtualConsole* apVCon)
{
	//TODO: Если меняется PanelView - то "под кружочком" останется старый кусок
	if (hdcIntVCon == (HDC)INVALID_HANDLE_VALUE)
	{
		//if (mh_RightClickingWnd)
		//	apiShowWindow(mh_RightClickingWnd, SW_HIDE);
		return;
	}

	BOOL lbSucceeded = FALSE;

	CVConGuard VCon;
	if (GetActiveVCon(&VCon) >= 0)
	{
		if (!apVCon)
		{
			apVCon = VCon.VCon();
		}
		// По идее, кружок может появиться только в активной консоли
		_ASSERTE(apVCon == VCon.VCon());
	}


	HWND hView = apVCon ? apVCon->GetView() : NULL;

	if (!hView || gpSet->isDisableMouse)
	{
		DEBUGSTRRCLICK(L"RightClickingPaint: !hView || gpSet->isDisableMouse\n");
	}
	else
	{
		if (gpSet->isRClickTouchInvert())
		{
			// Длинный клик в режиме инверсии?
			lbSucceeded = FALSE;

			DEBUGSTRRCLICK(L"RightClickingPaint: gpSet->isRClickTouchInvert()\n");

			//if (!mb_RightClickingLSent && apVCon)
			//{
			//	mb_RightClickingLSent = TRUE;
			//	// Чтобы установить курсор в панелях точно под кликом
			//	// иначе получается некрасиво, что курсор прыгает только перед
			//	// появлением EMenu, а до этого (пока крутится "кружок") курсор не двигается.
			//	apVCon->RCon()->PostLeftClickSync(mouse.RClkDC);
			//	//apVCon->RCon()->OnMouse(WM_MOUSEMOVE, 0, mouse.RClkDC.X, mouse.RClkDC.Y, true);
			//	//WARNING("По хорошему, нужно дождаться пока мышь обработается");
			//	//apVCon->RCon()->PostMacro(L"MsLClick");
			//	//WARNING("!!! Заменить на CMD_LEFTCLKSYNC?");
			//}
		}
		else if ((gpSet->isRClickSendKey > 1) && (mouse.state & MOUSE_R_LOCKED)
			&& (m_RightClickingFrames > 0) && mh_RightClickingBmp)
		{
			//WORD nRDown = GetKeyState(VK_RBUTTON);
			//POINT ptCur; GetCursorPos(&ptCur);
			//ScreenToClient(hView, &ptCur);
			bool bRDown = isPressed(VK_RBUTTON);

			if (bRDown)
			{
				DWORD dwCurTick = TimeGetTime(); //GetTickCount();
				DWORD dwDelta = dwCurTick - mouse.RClkTick;

				if (dwDelta < RCLICKAPPS_START)
				{
					// Пока рисовать не начали
					lbSucceeded = TRUE;
				}
				// Если держали дольше 10сек - все назад
				else if (dwDelta > RCLICKAPPSTIMEOUT_MAX/*10сек*/)
				{
					lbSucceeded = FALSE;
				}
				else
				{
					lbSucceeded = TRUE;

					if (!mb_RightClickingLSent && apVCon)
					{
						mb_RightClickingLSent = TRUE;
						// Чтобы установить курсор в панелях точно под кликом
						// иначе получается некрасиво, что курсор прыгает только перед
						// появлением EMenu, а до этого (пока крутится "кружок") курсор не двигается.
						apVCon->RCon()->PostLeftClickSync(mouse.RClkDC);
						//apVCon->RCon()->OnMouse(WM_MOUSEMOVE, 0, mouse.RClkDC.X, mouse.RClkDC.Y, true);
						//WARNING("По хорошему, нужно дождаться пока мышь обработается");
						//apVCon->RCon()->PostMacro(L"MsLClick");
						//WARNING("!!! Заменить на CMD_LEFTCLKSYNC?");
					}

					// Прикинуть индекс фрейма
					int nIndex = (dwDelta - RCLICKAPPS_START) * m_RightClickingFrames / (RCLICKAPPSTIMEOUT - RCLICKAPPS_START);

					if (nIndex >= m_RightClickingFrames)
					{
						nIndex = (m_RightClickingFrames-1); // рисуем последний фрейм, мышку можно отпускать
						//-- SetKillTimer(false, TIMER_RCLICKPAINT, 0); // таймер понадобится для "скрытия" кружочка после RCLICKAPPSTIMEOUT_MAX
					}

					#ifdef _DEBUG
					wchar_t szDbg[128];
					_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"RightClickingPaint: Delta=%u, nIndex=%u, {%ix%i}\n", dwDelta, nIndex, Rcursor.x, Rcursor.y);
					DEBUGSTRRCLICK(szDbg);
					#endif

					//if (hdcIntVCon || (m_RightClickingCurrent != nIndex))
					{
						// Рисуем
						HDC hdcSelf = NULL;
						const wchar_t szRightClickingClass[] = L"ConEmu_RightClicking";
						//BOOL lbSelfDC = FALSE;

						if (!mb_RightClickingRegistered)
						{
							WNDCLASSEX wc = {sizeof(WNDCLASSEX), CS_OWNDC, CConEmuMain::RightClickingProc, 0, 0,
							                 g_hInstance, NULL, LoadCursor(NULL, IDC_ARROW),
							                 NULL, NULL, szRightClickingClass, NULL};
							if (!RegisterClassEx(&wc))
							{
								DisplayLastError(L"Regitser class failed");
							}
							else
							{
								mb_RightClickingRegistered = TRUE;
							}
						}

						int nHalf = m_RightClickingSize.y>>1;

						if (mb_RightClickingRegistered && (!mh_RightClickingWnd || !IsWindow(mh_RightClickingWnd)))
						{
							POINT pt = {Rcursor.x-nHalf, Rcursor.y-nHalf};
							MapWindowPoints(hView, ghWnd, &pt, 1);
							mh_RightClickingWnd = CreateWindow(szRightClickingClass, L"",
								WS_VISIBLE|WS_CHILD|WS_DISABLED|WS_CLIPSIBLINGS|WS_CLIPCHILDREN,
                                pt.x, pt.y, m_RightClickingSize.y, m_RightClickingSize.y,
                                ghWnd, (HMENU)9999, g_hInstance, NULL);
							SetWindowPos(mh_RightClickingWnd, HWND_TOP, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
						}
						else
						{
							if (!IsWindowVisible(mh_RightClickingWnd))
								apiShowWindow(mh_RightClickingWnd, SW_SHOWNORMAL);
						}

						if (mh_RightClickingWnd && ((hdcSelf = GetDC(mh_RightClickingWnd)) != NULL))
						{
							DEBUGSTRRCLICK(L"RightClickingPaint: Painting...\n");

							HDC hCompDC = CreateCompatibleDC(hdcSelf);
							HBITMAP hOld = (HBITMAP)SelectObject(hCompDC, mh_RightClickingBmp);

							BLENDFUNCTION bf = {AC_SRC_OVER,0,255,AC_SRC_ALPHA};

							if (hdcIntVCon)
							{
								// Если меняется содержимое консоли - его нужно "обновить" и в нашем "окошке" с кружочком
								BitBlt(hdcSelf, 0, 0, m_RightClickingSize.y, m_RightClickingSize.y,
									hdcIntVCon, Rcursor.x-nHalf, Rcursor.y-nHalf, SRCCOPY);
							}

							GdiAlphaBlend(hdcSelf, 0, 0, m_RightClickingSize.y, m_RightClickingSize.y,
								  hCompDC, nIndex*m_RightClickingSize.y, 0, m_RightClickingSize.y, m_RightClickingSize.y, bf);

							if (hOld && hCompDC)
								SelectObject(hCompDC, hOld);

							//if (/*lbSelfDC &&*/ hdcSelf)
							DeleteDC(hdcSelf);
						}
					}

					// Запомним фрейм, что рисовали в последний раз
					m_RightClickingCurrent = nIndex;
				}
			}
		}
		else
		{
			DEBUGSTRRCLICK(L"RightClickingPaint: Condition failed\n");
		}
	}

	if (!lbSucceeded)
	{
		StopRightClickingPaint();
	}
}

void CConEmuMain::StartRightClickingPaint()
{
	if (!mb_RightClickingPaint)
	{
		if (m_RightClickingFrames > 0 && mh_RightClickingBmp)
		{
			m_RightClickingCurrent = -1;
			mb_RightClickingPaint = TRUE;
			mb_RightClickingLSent = FALSE;
			SetKillTimer(true, TIMER_RCLICKPAINT, TIMER_RCLICKPAINT_ELAPSE);
		}
	}
}

void CConEmuMain::StopRightClickingPaint()
{
	DEBUGSTRRCLICK(L"StopRightClickingPaint\n");

	if (mh_RightClickingWnd)
	{
		DestroyWindow(mh_RightClickingWnd);
		mh_RightClickingWnd = NULL;
	}

	if (mb_RightClickingPaint)
	{
		mb_RightClickingPaint = FALSE;
		mb_RightClickingLSent = FALSE;
		SetKillTimer(false, TIMER_RCLICKPAINT, 0);
		m_RightClickingCurrent = -1;
		CVConGroup::InvalidateAll();
	}
}

// Смысл этого окошка в том, чтобы отрисоваться поверх возможного PanelView
LRESULT CConEmuMain::RightClickingProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch (messg)
	{
	case WM_CREATE:
		return 0; // allow
	case WM_PAINT:
		{
			// Если этого не сделать - в очереди "зависнет" WM_PAINT
			PAINTSTRUCT ps = {};
			BeginPaint(hWnd, &ps);
			//RECT rcClient; GetClientRect(hWnd, &rcClient);
			//FillRect(ps.hdc, &rcClient, (HBRUSH)GetStockObject(WHITE_BRUSH));
			EndPaint(hWnd, &ps);
		}
		return 0;
	case WM_ERASEBKGND:
		return 0;
	}

	return ::DefWindowProc(hWnd, messg, wParam, lParam);
}

//void CConEmuMain::OnPaintClient(HDC hdc/*, int width, int height*/)
//{
//	// Если "завис" PostUpdate
//	if (mp_TabBar->NeedPostUpdate())
//		mp_TabBar->Update();
//
//#ifdef _DEBUG
//	RECT rcDbgSize; GetWindowRect(ghWnd, &rcDbgSize);
//	wchar_t szSize[255]; _wsprintf(szSize, SKIPLEN(countof(szSize)) L"WM_PAINT -> Window size (X=%i, Y=%i, W=%i, H=%i)\n",
//	                               rcDbgSize.left, rcDbgSize.top, (rcDbgSize.right-rcDbgSize.left), (rcDbgSize.bottom-rcDbgSize.top));
//	DEBUGSTRSIZE(szSize);
//	static RECT rcDbgSize1;
//
//	if (memcmp(&rcDbgSize1, &rcDbgSize, sizeof(rcDbgSize1)))
//	{
//		rcDbgSize1 = rcDbgSize;
//	}
//#endif
//
//	//PaintGaps(hdc);
//}

void CConEmuMain::InvalidateGaps()
{
	if (isIconic())
		return;

	CVConGroup::InvalidateGaps();
}

//void CConEmuMain::PaintGaps(HDC hDC)
//{
//	CVConGroup::PaintGaps(hDC);
//}

//void CConEmuMain::PaintCon(HDC hPaintDC)
//{
//	//if (ProgressBars)
//	//    ProgressBars->OnTimer();
//
//	// Если "завис" PostUpdate
//	if (mp_TabBar->NeedPostUpdate())
//		mp_TabBar->Update();
//
//	RECT rcClient = {0};
//
//	if ('ghWnd DC')
//	{
//		Get ClientRect('ghWnd DC', &rcClient);
//		MapWindowPoints('ghWnd DC', ghWnd, (LPPOINT)&rcClient, 2);
//	}
//
//	// если mp_ VActive==NULL - будет просто выполнена заливка фоном.
//	mp_ VActive->PaintVCon(hPaintDC, rcClient);
//
//#ifdef _DEBUG
//	if ((GetKeyState(VK_SCROLL) & 1) && (GetKeyState(VK_CAPITAL) & 1))
//	{
//		DebugStep(L"ConEmu: Sleeping in PaintCon for 1s");
//		Sleep(1000);
//		DebugStep(NULL);
//	}
//#endif
//}

//void CConEmuMain::RePaint()
//{
//	mb_SkipSyncSize = true;
//
//	gpConEmu->mp_TabBar->RePaint();
//	//m_Back->RePaint();
//	HDC hDc = GetDC(ghWnd);
//	//mp_ VActive->PaintVCon(hDc); // если mp_ VActive==NULL - будет просто выполнена заливка фоном.
//	PaintGaps(hDc);
//	//PaintCon(hDc);
//	ReleaseDC(ghWnd, hDc);
//
//	CVConGroup::RePaint();
//
//	mb_SkipSyncSize = false;
//}

void CConEmuMain::Update(bool isForce /*= false*/)
{
	CVConGroup::Update(isForce);
}

/* ****************************************************** */
/*                                                        */
/*                 Status functions                       */
/*                                                        */
/* ****************************************************** */

#ifdef colorer_func
{
	Status_Functions() {};
}
#endif

DWORD CConEmuMain::GetFarPID(BOOL abPluginRequired/*=FALSE*/)
{
	DWORD dwPID = CVConGroup::GetFarPID(abPluginRequired);
	return dwPID;
}

LPCTSTR CConEmuMain::GetLastTitle(bool abUseDefault/*=true*/)
{
	if (!Title[0] && abUseDefault)
		return GetDefaultTitle();

	return Title;
}

LPCTSTR CConEmuMain::GetVConTitle(int nIdx)
{
	//if (nIdx < 0 || nIdx >= MAX_CONSOLE_COUNT)
	//	return NULL;

	CVConGuard VCon;
	if (!CVConGroup::GetVCon(nIdx, &VCon))
		return NULL;

	if (!VCon.VCon() || !VCon->RCon())
		return NULL;

	LPCWSTR pszTitle = VCon->RCon()->GetTitle();
	if (pszTitle == NULL)
	{
		_ASSERTE(pszTitle!=NULL);
		pszTitle = GetDefaultTitle();
	}
	return pszTitle;
}

// Возвращает индекс (0-based) активной консоли
int CConEmuMain::GetActiveVCon(CVConGuard* pVCon /*= NULL*/, int* pAllCount /*= NULL*/)
{
	return CVConGroup::GetActiveVCon(pVCon, pAllCount);
}

bool CConEmuMain::IsActiveConAdmin()
{
	CVConGuard VCon;
	bool bAdmin = (gpConEmu->GetActiveVCon(&VCon) >= 0)
		? VCon->RCon()->isAdministrator()
		: gpConEmu->mb_IsUacAdmin;
	return bAdmin;
}

// bFromCycle = true, для перебора в циклах (например, в CSettings::UpdateWinHookSettings), чтобы не вылезали ассерты
CVirtualConsole* CConEmuMain::GetVCon(int nIdx, bool bFromCycle /*= false*/)
{
	CVConGuard VCon;
	if (!CVConGroup::GetVCon(nIdx, &VCon))
	{
		_ASSERTE((nIdx>=0 && (nIdx<(int)MAX_CONSOLE_COUNT || (bFromCycle && nIdx==(int)MAX_CONSOLE_COUNT))));
		return NULL;
	}

	return VCon.VCon();
}

// 0 - такой консоли нет
// 1..MAX_CONSOLE_COUNT - "номер" консоли (1 based!)
int CConEmuMain::isVConValid(CVirtualConsole* apVCon)
{
	int nIdx = CVConGroup::GetVConIndex(apVCon);
	if (nIdx >= 0)
		return (nIdx+1);
	return 0;
}

CVirtualConsole* CConEmuMain::GetVConFromPoint(POINT ptScreen)
{
	CVConGuard VCon;
	if (!CVConGroup::GetVConFromPoint(ptScreen, &VCon))
		return NULL;

	return VCon.VCon();
}

bool CConEmuMain::isActive(CVirtualConsole* apVCon, bool abAllowGroup /*= true*/)
{
	return CVConGroup::isActive(apVCon, abAllowGroup);
}

bool CConEmuMain::isConSelectMode()
{
	return CVConGroup::isConSelectMode();
}

// Возвращает true если начат ShellDrag
bool CConEmuMain::isDragging()
{
	if ((mouse.state & (DRAG_L_STARTED | DRAG_R_STARTED)) == 0)
		return false;

	if (isConSelectMode())
	{
		mouse.state &= ~(DRAG_L_STARTED | DRAG_L_ALLOWED | DRAG_R_STARTED | DRAG_R_ALLOWED | MOUSE_DRAGPANEL_ALL);
		return false;
	}

	return true;
}

bool CConEmuMain::isFirstInstance(bool bFolderIgnore /*= false*/)
{
	if (!mb_AliveInitialized)
	{
		//создадим событие, чтобы не было проблем с ключем /SINGLE
		mb_AliveInitialized = TRUE;

		wchar_t* pszSlash;
		wchar_t szConfig[64]; _ASSERTE(gpSetCls->GetConfigName()!=NULL);
		// Trim config name to max 63 chars
		lstrcpyn(szConfig, gpSetCls->GetConfigName(), countof(szConfig));
		
		wchar_t szPath[MAX_PATH+1];
		DWORD cchMaxSize = MAX_PATH/*max event name len*/ - 2 - _tcslen(CEGUI_ALIVE_EVENT);
		if (*szConfig)
		{
			// When config name exists - append it (trimmed to 63 chars) + one separator-char
			cchMaxSize -= _tcslen(szConfig)+1;
			// Event name can contain any character except the backslash character (\).
			pszSlash = szConfig;
			while ((pszSlash = wcschr(pszSlash, L'\\')) != NULL)
			{
				*pszSlash = L'/';
			}
		}

		// Trim program folder path to available length
		lstrcpyn(szPath, ms_ConEmuBaseDir, cchMaxSize);
		CharLowerBuff(szPath, lstrlen(szPath));
		// Event name can contain any character except the backslash character (\).
		pszSlash = szPath;
		while ((pszSlash = wcschr(pszSlash, L'\\')) != NULL)
		{
			*pszSlash = L'/';
		}

		wcscpy_c(ms_ConEmuAliveEvent, CEGUI_ALIVE_EVENT);
		wcscat_c(ms_ConEmuAliveEvent, L"_");
		if (*szConfig)
		{
			wcscat_c(ms_ConEmuAliveEvent, L"_");
			wcscat_c(ms_ConEmuAliveEvent, szConfig);
		}
		wcscat_c(ms_ConEmuAliveEvent, szPath);

		//Events by default are created in "Local\\" kernel namespace.
		//GetUserName(ms_ConEmuAliveEvent+_tcslen(ms_ConEmuAliveEvent), &nSize);
		//WARNING("Event не работает, если conemu.exe запущен под другим пользователем");
		
		// Create event distinct by program folder and config name
		SetLastError(0);
		mh_ConEmuAliveEvent = CreateEvent(NULL, TRUE, TRUE, ms_ConEmuAliveEvent);
		mn_ConEmuAliveEventErr = GetLastError();
		_ASSERTE(mh_ConEmuAliveEvent!=NULL);

		// -- name can contain any character except the backslash character (\).

		mb_ConEmuAliveOwned = (mh_ConEmuAliveEvent != NULL) && (mn_ConEmuAliveEventErr != ERROR_ALREADY_EXISTS);

		// Теперь - глобальное
		SetLastError(0);
		mh_ConEmuAliveEventNoDir = CreateEvent(NULL, TRUE, TRUE, CEGUI_ALIVE_EVENT);
		mn_ConEmuAliveEventErrNoDir = GetLastError();
		mb_ConEmuAliveOwnedNoDir = (mh_ConEmuAliveEventNoDir != NULL) && (mn_ConEmuAliveEventErrNoDir != ERROR_ALREADY_EXISTS);
		_ASSERTE(mh_ConEmuAliveEventNoDir!=NULL);
	}


	// Попытка регистрации - по локальному событию. Ошибку показывать - только по глобальному.
	if (mh_ConEmuAliveEvent && !mb_ConEmuAliveOwned)
	{
		if (WaitForSingleObject(mh_ConEmuAliveEvent,0) == WAIT_TIMEOUT)
		{
			SetEvent(mh_ConEmuAliveEvent);
			mb_ConEmuAliveOwned = true;

			// Этот экземпляр становится "Основным" (другой, ранее бывший основным, был закрыт)
			RegisterMinRestore(true);
		}
	}

	if (mh_ConEmuAliveEventNoDir && !mb_ConEmuAliveOwnedNoDir)
	{
		if (WaitForSingleObject(mh_ConEmuAliveEventNoDir,0) == WAIT_TIMEOUT)
		{
			SetEvent(mh_ConEmuAliveEventNoDir);
			mb_ConEmuAliveOwnedNoDir = true;
		}
	}

	// 

	// Смотря что просили
	return bFolderIgnore ? mb_ConEmuAliveOwnedNoDir : mb_ConEmuAliveOwned;
}

bool CConEmuMain::isLBDown()
{
	return (mouse.state & DRAG_L_ALLOWED) == DRAG_L_ALLOWED;
}

bool CConEmuMain::isMainThread()
{
	DWORD dwTID = GetCurrentThreadId();
	return dwTID == mn_MainThreadId;
}

bool CConEmuMain::isMeForeground(bool abRealAlso/*=false*/, bool abDialogsAlso/*=true*/, HWND* phFore/*=NULL*/)
{
	if (!this) return false;

	static HWND hLastFore = NULL;
	static bool isMe = false;
	static bool bLastRealAlso, bLastDialogsAlso;
	HWND h = getForegroundWindow();
	if (phFore)
		*phFore = h;

	if ((h != hLastFore) || (bLastRealAlso != abRealAlso) || (bLastDialogsAlso != abDialogsAlso))
	{
		DWORD nForePID = 0;
		if (h)
			GetWindowThreadProcessId(h, &nForePID);
		isMe = (h != NULL)
			&& ((h == ghWnd)
				|| (abDialogsAlso && (nForePID == GetCurrentProcessId()))
					//((h == ghOpWnd)
					//|| (mp_AttachDlg && (mp_AttachDlg->GetHWND() == h))
					//|| (mp_RecreateDlg && (mp_RecreateDlg->GetHWND() == h))
					//|| (mp_Find->mh_FindDlg == h)))
				|| (mp_Inside && (h == mp_Inside->mh_InsideParentRoot)))
			;

		if (h && !isMe && abRealAlso)
		{
			if (CVConGroup::isOurWindow(h))
				isMe = true;
		}

		hLastFore = h;
		bLastRealAlso = abRealAlso;
		bLastDialogsAlso = abDialogsAlso;

		if (!isMe && nForePID && mp_DefTrm && gpSet->isSetDefaultTerminal && gpConEmu->isMainThread())
		{
			// If user want to use ConEmu as default terminal for CUI apps
			// we need to hook GUI applications (e.g. explorer)
			mp_DefTrm->CheckForeground(h, nForePID);
		}
	}

	return isMe;
}

bool CConEmuMain::isMouseOverFrame(bool abReal)
{
	// Если ресайзят за хвостик статус-бара - нефиг с рамкой играться
	if (mp_Status->IsStatusResizing())
	{
		_ASSERTE(m_ForceShowFrame == fsf_Hide); // не должно быть
		_ASSERTE(isSizing()); // флаг "ресайза" должен был быть выставлен
		return false;
	}

	if (m_ForceShowFrame && isSizing())
	{
		if (!isPressed(VK_LBUTTON))
		{
			EndSizing();
		}
		else
		{
			return true;
		}
	}

	bool bCurForceShow = false;

	if (abReal)
	{
		POINT ptMouse; GetCursorPos(&ptMouse);
		RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);
		
		const int nAdd = 2;
		const int nSub = 1;

		// чтобы область активации рамки была чуть побольше, увеличим регион
		rcWnd.left -= nAdd; rcWnd.right += nAdd; rcWnd.bottom += nAdd;
		if (gpSet->isQuakeStyle)
			rcWnd.top += nSub; // Don't activate on the TOP edge in Quake mode
		else
			rcWnd.top -= nAdd;

		if (PtInRect(&rcWnd, ptMouse))
		{
			RECT rcClient = GetGuiClientRect();

			// чтобы область активации рамки была чуть побольше, уменьшим регион
			rcClient.left += nSub; rcClient.right -= nSub; rcClient.bottom -= nSub;
			if (gpSet->isQuakeStyle)
				rcWnd.top -= nAdd;
			else
				rcClient.top += nSub;

			MapWindowPoints(ghWnd, NULL, (LPPOINT)&rcClient, 2);

			if (!PtInRect(&rcClient, ptMouse))
				bCurForceShow = true;
		}
	}
	else
	{
		bCurForceShow = (m_ForceShowFrame == fsf_Show);
	}

	return bCurForceShow;
}

bool CConEmuMain::isNtvdm(BOOL abCheckAllConsoles/*=FALSE*/)
{
	return CVConGroup::isNtvdm(abCheckAllConsoles);
}

bool CConEmuMain::isOurConsoleWindow(HWND hCon)
{
	return CVConGroup::isOurConsoleWindow(hCon);
}

bool CConEmuMain::isValid(CRealConsole* apRCon)
{
	return CVConGroup::isValid(apRCon);
}

bool CConEmuMain::isValid(CVirtualConsole* apVCon)
{
	return CVConGroup::isValid(apVCon);
}

bool CConEmuMain::isVConExists(int nIdx)
{
	return CVConGroup::isVConExists(nIdx);
}

bool CConEmuMain::isVConHWND(HWND hChild, CVConGuard* pVCon /*= NULL*/)
{
	return CVConGroup::isVConHWND(hChild, pVCon);
}

bool CConEmuMain::isViewer()
{
	return CVConGroup::isViewer();
}

bool CConEmuMain::isVisible(CVirtualConsole* apVCon)
{
	return CVConGroup::isVisible(apVCon);
}

//bool CConEmuMain::isChildWindowVisible()
//{
//	return CVConGroup::isChildWindowVisible();
//}

// Заголовок окна для PictureView вообще может пользователем настраиваться, так что
// рассчитывать на него при определения "Просмотра" - нельзя
bool CConEmuMain::isPictureView()
{
	return CVConGroup::isPictureView();
}

bool CConEmuMain::isProcessCreated()
{
	return mb_ProcessCreated;
}

bool CConEmuMain::isSizing(UINT nMouseMsg/*=0*/)
{
	// Юзер тащит мышкой рамку окна
	if ((mouse.state & MOUSE_SIZING_BEGIN) != MOUSE_SIZING_BEGIN)
		return false;

	// могло не сброситься, проверим
	if ((nMouseMsg==WM_NCLBUTTONUP) || !isPressed(VK_LBUTTON))
	{
		EndSizing(nMouseMsg);
		return false;
	}

	return true;
}

void CConEmuMain::BeginSizing(bool bFromStatusBar)
{
	// Перенес снизу, а то ассерты вылезают
	SetSizingFlags(MOUSE_SIZING_BEGIN);

	// When resizing by dragging status-bar corner
	if (bFromStatusBar)
	{
		// hide frame, if it was force-showed
		if (m_ForceShowFrame != fsf_Hide)
		{
			_ASSERTE(gpSet->isFrameHidden()); // m_ForceShowFrame may be set only when isFrameHidden

			StopForceShowFrame();
		}
	}

	if (gpSet->isHideCaptionAlways())
	{
		SetKillTimer(false, TIMER_CAPTION_DISAPPEAR_ID, 0);
		SetKillTimer(false, TIMER_CAPTION_APPEAR_ID, 0);
		// -- если уж пришел WM_NCLBUTTONDOWN - значит рамка уже показана?
		//gpConEmu->OnTimer(TIMER_CAPTION_APPEAR_ID,0);
		//UpdateWindowRgn();
	}
}

void CConEmuMain::SetSizingFlags(DWORD nSetFlags /*= MOUSE_SIZING_BEGIN*/)
{
	_ASSERTE((nSetFlags & (~(MOUSE_SIZING_BEGIN|MOUSE_SIZING_TODO))) == 0); // Допустимые флаги

	#ifdef _DEBUG
	if (!(gpConEmu->mouse.state & nSetFlags)) // For debug purposes (breakpoints)
	#endif
	gpConEmu->mouse.state |= nSetFlags;
}

void CConEmuMain::ResetSizingFlags(DWORD nDropFlags /*= MOUSE_SIZING_BEGIN|MOUSE_SIZING_TODO*/)
{
	_ASSERTE((nDropFlags & (~(MOUSE_SIZING_BEGIN|MOUSE_SIZING_TODO))) == 0); // Допустимые флаги

	#ifdef _DEBUG
	if ((gpConEmu->mouse.state & nDropFlags)) // For debug purposes (breakpoints)
	#endif
	mouse.state &= ~nDropFlags;
}

void CConEmuMain::EndSizing(UINT nMouseMsg/*=0*/)
{
	bool bApplyResize = false;

	if (mouse.state & MOUSE_SIZING_TODO)
	{
		// Если тянули мышкой - изменение размера консоли могло быть "отложено" до ее отпускания
		bApplyResize = true;
	}

	ResetSizingFlags(MOUSE_SIZING_BEGIN|MOUSE_SIZING_TODO);

	if (bApplyResize)
	{
		CVConGroup::SyncConsoleToWindow();
	}

	if (!isIconic())
	{
		if (m_TileMode != cwc_Current)
		{
			wchar_t szTile[100], szOldTile[32];
			_wsprintf(szTile, SKIPLEN(countof(szTile)) L"Tile mode was stopped on resizing: Was=%s, New=cwc_Current",
				FormatTileMode(m_TileMode,szOldTile,countof(szOldTile)));
			LogString(szTile);

			m_TileMode = cwc_Current;
		}

		// Сама разберется, что надо/не надо
		StoreNormalRect(NULL);
		// Теоретически, в некоторых случаях размер окна может (?) измениться с задержкой,
		// в следующем цикле таймера - проверить размер
		mouse.bCheckNormalRect = true;
	}

	if (IsWindowVisible(ghWnd) && !isIconic())
	{
		CVConGroup::NotifyChildrenWindows();
	}
}

// Сюда может придти только LOWORD от HKL
void CConEmuMain::SwitchKeyboardLayout(DWORD_PTR dwNewKeybLayout)
{
	if ((gpSet->isMonitorConsoleLang & 1) == 0)
	{
		if (gpSetCls->isAdvLogging > 1)
			LogString(L"CConEmuMain::SwitchKeyboardLayout skipped, cause of isMonitorConsoleLang==0");

		return;
	}

	#ifdef _DEBUG
	wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"CConEmuMain::SwitchKeyboardLayout(0x%08I64X)\n", (unsigned __int64)dwNewKeybLayout);
	DEBUGSTRLANG(szDbg);
	#endif
	
	HKL hKeyb[20] = {};
	UINT nCount, i;
	BOOL lbFound = FALSE;
	
	nCount = GetKeyboardLayoutList(countof(hKeyb), hKeyb);

	for (i = 0; !lbFound && i < nCount; i++)
	{
		if (hKeyb[i] == (HKL)dwNewKeybLayout)
			lbFound = TRUE;
	}

	WARNING("Похоже с другими раскладками будет глючить. US Dvorak?");

	for (i = 0; !lbFound && i < nCount; i++)
	{
		if ((((DWORD_PTR)hKeyb[i]) & 0xFFFF) == (dwNewKeybLayout & 0xFFFF))
		{
			lbFound = TRUE; dwNewKeybLayout = (DWORD_PTR)hKeyb[i];
		}
	}

	// Если не задана раскладка (только язык?) формируем по умолчанию
	if (!lbFound && (dwNewKeybLayout == (dwNewKeybLayout & 0xFFFF)))
		dwNewKeybLayout |= (dwNewKeybLayout << 16);

	// Может она сейчас и активна?
	if (dwNewKeybLayout != GetActiveKeyboardLayout())
	{
#ifdef _DEBUG
		_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"CConEmuMain::SwitchKeyboardLayout change to(0x%08I64X)\n", (unsigned __int64)dwNewKeybLayout);
		DEBUGSTRLANG(szDbg);
#endif

		if (gpSetCls->isAdvLogging > 1)
		{
			wchar_t szInfo[255];
			_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"CConEmuMain::SwitchKeyboardLayout, posting WM_INPUTLANGCHANGEREQUEST, WM_INPUTLANGCHANGE for 0x%08X",
			          (DWORD)dwNewKeybLayout);
			LogString(szInfo);
		}

		// Теперь переключаем раскладку
		PostMessage(ghWnd, WM_INPUTLANGCHANGEREQUEST, 0, (LPARAM)dwNewKeybLayout);
		PostMessage(ghWnd, WM_INPUTLANGCHANGE, 0, (LPARAM)dwNewKeybLayout);
	}
	else
	{
		if (gpSetCls->isAdvLogging > 1)
		{
			wchar_t szInfo[255];
			_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"CConEmuMain::SwitchKeyboardLayout skipped, cause of GetActiveKeyboardLayout()==0x%08X",
			          (DWORD)dwNewKeybLayout);
			LogString(szInfo);
		}
	}
}





/* ****************************************************** */
/*                                                        */
/*                        EVENTS                          */
/*                                                        */
/* ****************************************************** */

#ifdef colorer_func
{
	EVENTS() {};
}
#endif

// Обновить на тулбаре статусы Scrolling(BufferHeight) & Alternative
void CConEmuMain::OnBufferHeight() //BOOL abBufferHeight)
{
	if (!isMainThread())
	{
		PostMessage(ghWnd, mn_MsgPostOnBufferHeight, 0, 0);
		return;
	}

	CVirtualConsole* pVCon = NULL;
	CVConGuard VCon;
	if (GetActiveVCon(&VCon) >= 0)
		pVCon = VCon.VCon();


	BOOL lbBufferHeight = FALSE;
	BOOL lbAlternative = FALSE;

	if (pVCon)
	{
		mp_Status->OnConsoleBufferChanged(pVCon->RCon());

		lbBufferHeight = pVCon->RCon()->isBufferHeight();
		lbAlternative = pVCon->RCon()->isAlternative();
	}

	TrackMouse(); // спрятать или показать прокрутку, если над ней мышка

	mp_TabBar->OnBufferHeight(lbBufferHeight);
	mp_TabBar->OnAlternative(lbAlternative);
}


//bool CConEmuMain::DoClose()
//{
//	bool lbProceed = CVConGroup::OnScClose();
//
//	return lbProceed;
//}

// returns true if gpConEmu->Destroy() was called
bool CConEmuMain::OnScClose()
{
	gpConEmu->LogString(L"CVConGroup::OnScClose()");

	bool lbMsgConfirmed = false;

	// lbMsgConfirmed - был ли показан диалог подтверждения, или юзер не включил эту опцию
	if (!CVConGroup::OnCloseQuery(&lbMsgConfirmed))
		return false; // не закрывать

	return CVConGroup::DoCloseAllVCon(lbMsgConfirmed);
}

void CConEmuMain::SetScClosePending(bool bFlag)
{
	mb_ScClosePending = bFlag;
}

bool CConEmuMain::isCloseConfirmed()
{
	return (gpSet->isCloseConsoleConfirm && !DisableCloseConfirm) ? mb_ScClosePending : true;
}

BOOL CConEmuMain::setWindowPos(HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
	BOOL lbRc;

	bool bInCreate = InCreateWindow();
	bool bPrevIgnore = mb_IgnoreQuakeActivation;
	bool bQuake = gpSet->isQuakeStyle && !(gpSet->isDesktopMode || mp_Inside);

	if (bInCreate)
	{
		if (WindowStartTsa || (WindowStartMinimized && gpSet->isMinToTray()))
		{
			uFlags |= SWP_NOACTIVATE;
			uFlags &= ~SWP_SHOWWINDOW;
		}

		if (bQuake)
		{
			mb_IgnoreQuakeActivation = true;
		}
	}

	lbRc = SetWindowPos(ghWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);

	if (bInCreate && bQuake)
	{
		mb_IgnoreQuakeActivation = bPrevIgnore;
	}

	return lbRc;
}

LRESULT CConEmuMain::OnCreate(HWND hWnd, LPCREATESTRUCT lpCreate)
{
	_ASSERTE(ghWnd == hWnd); // Уже должно было быть выставлено (CConEmuMain::MainWndProc)
	ghWnd = hWnd; // ставим сразу, чтобы функции могли пользоваться

	if (gpSetCls->isAdvLogging)
	{
		char szInfo[200];
		RECT rcWnd = {}; GetWindowRect(ghWnd, &rcWnd);
		_wsprintfA(szInfo, SKIPLEN(countof(szInfo))
			"OnCreate: hWnd=x%08X, x=%i, y=%i, cx=%i, cy=%i, style=x%08X, exStyle=x%08X",
			(DWORD)(DWORD_PTR)hWnd, lpCreate->x, lpCreate->y, lpCreate->cx, lpCreate->cy, lpCreate->style, lpCreate->dwExStyle);
		LogString(szInfo);
	}

	OnTaskbarButtonCreated();

	//DWORD_PTR dwStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
	//if (gpSet->isHideCaptionAlways) {
	//	if ((dwStyle & (WS_CAPTION|WS_THICKFRAME)) != 0) {
	//		lpCreate->style &= ~(WS_CAPTION|WS_THICKFRAME);
	//		dwStyle = lpCreate->style;
	//		SetWindowStyle(dwStyle);
	//	}
	//}

	// lpCreate->cx/cy может содержать CW_USEDEFAULT
	//RECT rcWnd = CalcRect(CER_MAIN);
	//GetWindowRect(ghWnd, &rcWnd);

	// It's better to store current mrc_Ideal values now, after real window creation
	StoreIdealRect();
	//UpdateIdealRect();

	// Used for restoring from Maximized/Fullscreen/Iconic. Remember current Pos/Size.
	StoreNormalRect(NULL);

	// Win глючит? Просил создать БЕЗ WS_CAPTION, а создается С WS_CAPTION
	DWORD dwStyle = GetWindowLong(ghWnd, GWL_STYLE);
	DWORD dwNewStyle = FixWindowStyle(dwStyle, WindowMode);
	DEBUGTEST(WINDOWPLACEMENT wpl1 = {sizeof(wpl1)}; GetWindowPlacement(ghWnd, &wpl1););
	if (dwStyle != dwNewStyle)
	{
		//TODO: Проверить, чтобы в этот момент окошко не пыталось куда-то уехать
		SetWindowStyle(dwNewStyle);
	}
	DEBUGTEST(WINDOWPLACEMENT wpl2 = {sizeof(wpl2)}; GetWindowPlacement(ghWnd, &wpl2););


	Icon.LoadIcon(hWnd, gpSet->nIconID/*IDI_ICON1*/);
	// Позволяет реагировать на запросы FlashWindow из фара и запуск приложений
	HMODULE hUser32 = GetModuleHandle(L"user32.dll");
	FRegisterShellHookWindow fnRegisterShellHookWindow = NULL;

	if (hUser32) fnRegisterShellHookWindow = (FRegisterShellHookWindow)GetProcAddress(hUser32, "RegisterShellHookWindow");

	if (fnRegisterShellHookWindow) fnRegisterShellHookWindow(hWnd);

	_ASSERTE(ghConWnd==NULL && "ConWnd must not be created yet"); // оно еще не должно быть создано
	// Чтобы можно было найти хэндл окна по хэндлу консоли
	OnActiveConWndStore(NULL); // 31.03.2009 Maximus - только нихрена оно еще не создано!
	//m_Back->Create();

	//if (!m_Child->Create())
	//	return -1;

	//mn_StartTick = GetTickCount();
	//if (gpSet->isGUIpb && !ProgressBars) {
	//	ProgressBars = new CProgressBars(ghWnd, g_hInstance);
	//}
	//// Установить переменную среды с дескриптором окна
	//SetConEmuEnvVar('ghWnd DC');

	// Сформировать или обновить системное меню
	mp_Menu->GetSysMenu(TRUE);

	// Holder
	_ASSERTE(ghWndWork==NULL); // еще не должен был быть создан
	if (!ghWndWork && !CreateWorkWindow())
	{
		// Ошибка уже показана
		return -1;
	}

	//HMENU hwndMain = GetSysMenu(ghWnd, FALSE), hDebug = NULL;
	//InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_TOMONITOR, _T("Bring &here"));
	//InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_TOTRAY, TRAY_ITEM_HIDE_NAME/* L"Hide to &TSA" */);
	//InsertMenu(hwndMain, 0, MF_BYPOSITION, MF_SEPARATOR, 0);
	//InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_ABOUT, _T("&About / Help"));

	//if (ms_ConEmuChm[0])  //Показывать пункт только если есть conemu.chm
	//	InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_HELP, _T("&Help"));

	//InsertMenu(hwndMain, 0, MF_BYPOSITION, MF_SEPARATOR, 0);
	//hDebug = CreateDebugMenuPopup();
	//InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_POPUP | MF_ENABLED, (UINT_PTR)hDebug, _T("&Debug"));
	//PopulateEditMenuPopup(hwndMain);
	//InsertMenu(hwndMain, 0, MF_BYPOSITION, MF_SEPARATOR, 0);
	//InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED | (gpSet->isAlwaysOnTop ? MF_CHECKED : 0),
	//           ID_ALWAYSONTOP, _T("Al&ways on top"));
	//InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED | (gpSetCls->AutoScroll ? MF_CHECKED : 0),
	//           ID_AUTOSCROLL, _T("Auto scro&ll"));
	//InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_SETTINGS, _T("S&ettings..."));
	//InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_NEWCONSOLE, _T("&New console..."));

#ifdef _DEBUG
	dwStyle = GetWindowLong(ghWnd, GWL_STYLE);
#endif

	_ASSERTE(InCreateWindow() == true);

	if (gpSet->isTabs==1)  // "Табы всегда"
	{
		ForceShowTabs(TRUE); // Показать табы

		// Расчет высоты таббара был выполнен "примерно"
		if ((WindowMode == wmNormal) && !mp_Inside)
		{
			RECT rcWnd = GetDefaultRect();
			setWindowPos(NULL, rcWnd.left, rcWnd.top, rcWnd.right-rcWnd.left, rcWnd.bottom-rcWnd.top, SWP_NOZORDER);
			UpdateIdealRect(rcWnd);
		}
	}

	// Причесать размеры напоследок
	gpConEmu->ReSize(TRUE);

	//CreateCon();
	// Запустить серверную нить
	// 120122 - теперь через PipeServer
	if (!m_GuiServer.Start())
	{
		// Ошибка уже показана
		return -1;
	}

	return 0;
}

wchar_t* CConEmuMain::LoadConsoleBatch(LPCWSTR asSource, wchar_t** ppszStartupDir /*= NULL*/)
{
	wchar_t* pszDataW = NULL;

	if (*asSource == CmdFilePrefix)
	{
		// В качестве "команды" указан "пакетный файл" одновременного запуска нескольких консолей
		pszDataW = LoadConsoleBatch_File(asSource);
	}
	else if ((*asSource == TaskBracketLeft) || (lstrcmp(asSource, AutoStartTaskName) == 0))
	{
		// Имя задачи
		pszDataW = LoadConsoleBatch_Task(asSource, ppszStartupDir);
	}
	else if (*asSource == DropLnkPrefix)
	{
		// Сюда мы попадаем, если на ConEmu (или его ярлык)
		// набрасывают (в проводнике?) один или несколько других файлов/программ
		pszDataW = LoadConsoleBatch_Drops(asSource);
	}
	else
	{
		_ASSERTE(*asSource==CmdFilePrefix || *asSource==TaskBracketLeft);
	}

	return pszDataW;
}

wchar_t* CConEmuMain::LoadConsoleBatch_File(LPCWSTR asSource)
{
	wchar_t* pszDataW = NULL;

	if (asSource && (*asSource == CmdFilePrefix))
	{
		// В качестве "команды" указан "пакетный файл" одновременного запуска нескольких консолей
		HANDLE hFile = CreateFile(asSource+1, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

		if (!hFile || hFile == INVALID_HANDLE_VALUE)
		{
			DWORD dwErr = GetLastError();
			wchar_t szCurDir[MAX_PATH*2]; szCurDir[0] = 0; GetCurrentDirectory(countof(szCurDir), szCurDir);
			size_t cchMax = _tcslen(asSource)+100+_tcslen(szCurDir);
			wchar_t* pszErrMsg = (wchar_t*)calloc(cchMax,2);
			_wcscpy_c(pszErrMsg, cchMax, L"Can't open console batch file:\n" L"\x2018"/*‘*/);
			_wcscat_c(pszErrMsg, cchMax, asSource+1);
			_wcscat_c(pszErrMsg, cchMax, L"\x2019"/*’*/ L"\nCurrent directory:\n" L"\x2018"/*‘*/);
			_wcscat_c(pszErrMsg, cchMax, szCurDir);
			_wcscat_c(pszErrMsg, cchMax, L"\x2019"/*’*/);
			DisplayLastError(pszErrMsg, dwErr);
			free(pszErrMsg);
			//Destroy(); -- must caller
			return NULL;
		}

		DWORD nSize = GetFileSize(hFile, NULL);

		if (!nSize || nSize > (1<<20))
		{
			DWORD dwErr = GetLastError();
			CloseHandle(hFile);
			wchar_t* pszErrMsg = (wchar_t*)calloc(_tcslen(asSource)+100,2);
			lstrcpy(pszErrMsg, L"Console batch file is too large or empty:\n" L"\x2018"/*‘*/); lstrcat(pszErrMsg, asSource+1); lstrcat(pszErrMsg, L"\x2019"/*’*/);
			DisplayLastError(pszErrMsg, dwErr);
			free(pszErrMsg);
			//Destroy(); -- must caller
			return NULL;
		}

		char* pszDataA = (char*)calloc(nSize+4,1); //-V112
		_ASSERTE(pszDataA);
		DWORD nRead = 0;
		BOOL lbRead = ReadFile(hFile, pszDataA, nSize, &nRead, 0);
		DWORD dwErr = GetLastError();
		CloseHandle(hFile);

		if (!lbRead || nRead != nSize)
		{
			free(pszDataA);
			wchar_t* pszErrMsg = (wchar_t*)calloc(_tcslen(asSource)+100,2);
			lstrcpy(pszErrMsg, L"Reading console batch file failed:\n" L"\x2018"/*‘*/); lstrcat(pszErrMsg, asSource+1); lstrcat(pszErrMsg, L"\x2019"/*’*/);
			DisplayLastError(pszErrMsg, dwErr);
			free(pszErrMsg);
			//Destroy(); -- must caller
			return NULL;
		}

		// Опредлить код.страницу файла
		if (pszDataA[0] == '\xEF' && pszDataA[1] == '\xBB' && pszDataA[2] == '\xBF')
		{
			// UTF-8 BOM
			pszDataW = (wchar_t*)calloc(nSize+2,2);
			_ASSERTE(pszDataW);
			MultiByteToWideChar(CP_UTF8, 0, pszDataA+3, -1, pszDataW, nSize);
		}
		else if (pszDataA[0] == '\xFF' && pszDataA[1] == '\xFE')
		{
			// CP-1200 BOM
			pszDataW = lstrdup((wchar_t*)(pszDataA+2));
			_ASSERTE(pszDataW);
		}
		else
		{
			// Plain ANSI
			pszDataW = (wchar_t*)calloc(nSize+2,2);
			_ASSERTE(pszDataW);
			MultiByteToWideChar(CP_ACP, 0, pszDataA, -1, pszDataW, nSize+1);
		}

	}
	else
	{
		_ASSERTE(FALSE && "Invalid CmdFilePrefix");
	}

	return pszDataW;
}

wchar_t* CConEmuMain::LoadConsoleBatch_Drops(LPCWSTR asSource)
{
	wchar_t* pszDataW = NULL;

	if (asSource && (*asSource == DropLnkPrefix))
	{
		// Сюда мы попадаем, если на ConEmu (или его ярлык)
		// набрасывают (в проводнике?) один или несколько других файлов/программ
		asSource++;
		if (!*asSource)
		{
			DisplayLastError(L"Empty command", (DWORD)-1);
			return NULL;
		}

		// Считаем, что один файл (*.exe, *.cmd, ...) или ярлык (*.lnk)
		// это одна запускаемая консоль в ConEmu.
		CmdArg szPart;
		wchar_t szExe[MAX_PATH+1], szDir[MAX_PATH+1];
		HRESULT hr = S_OK;
		IShellLinkW* pShellLink = NULL;
		IPersistFile* pFile = NULL;
		if (StrStrI(asSource, L".lnk"))
		{
			hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&pShellLink);
			if (FAILED(hr) || !pShellLink)
			{
				DisplayLastError(L"Can't create IID_IShellLinkW", (DWORD)hr);
				return NULL;
			}
			hr = pShellLink->QueryInterface(IID_IPersistFile, (void**)&pFile);
			if (FAILED(hr) || !pFile)
			{
				DisplayLastError(L"Can't create IID_IPersistFile", (DWORD)hr);
				pShellLink->Release();
				return NULL;
			}
		}

		INT_PTR cchArguments = 32768;
		wchar_t* pszArguments = (wchar_t*)calloc(cchArguments,sizeof(pszArguments));
		if (!pszArguments)
		{
			SafeRelease(pShellLink);
			SafeRelease(pFile);
			return NULL;
		}
		
		// Поехали
		LPWSTR pszConsoles[MAX_CONSOLE_COUNT] = {};
		size_t cchLen, cchAllLen = 0, iCount = 0;
		while ((iCount < MAX_CONSOLE_COUNT) && (0 == NextArg(&asSource, szPart)))
		{
			if (lstrcmpi(PointToExt(szPart), L".lnk") == 0)
			{
				// Ярлык
				hr = pFile->Load(szPart, STGM_READ);
				if (SUCCEEDED(hr))
				{
					hr = pShellLink->GetPath(szExe, countof(szExe), NULL, 0);
					if (SUCCEEDED(hr) && *szExe)
					{
						hr = pShellLink->GetArguments(pszArguments, cchArguments);
						if (FAILED(hr))
							pszArguments[0] = 0;
						hr = pShellLink->GetWorkingDirectory(szDir, countof(szDir));
						if (FAILED(hr))
							szDir[0] = 0;

						cchLen = _tcslen(szExe)+3
							+ _tcslen(pszArguments)+1
							+ (*szDir ? (_tcslen(szDir)+32) : 0); // + "-new_console:d<Dir>
						pszConsoles[iCount] = (wchar_t*)malloc(cchLen*sizeof(wchar_t));
						_wsprintf(pszConsoles[iCount], SKIPLEN(cchLen) L"\"%s\"%s%s",
							Unquote(szExe), *pszArguments ? L" " : L"", pszArguments);
						if (*szDir)
						{
							_wcscat_c(pszConsoles[iCount], cchLen, L" \"-new_console:d");
							_wcscat_c(pszConsoles[iCount], cchLen, Unquote(szDir));
							_wcscat_c(pszConsoles[iCount], cchLen, L"\"");
						}
						iCount++;

						cchAllLen += cchLen+3;
					}
				}
			}
			else
			{
				cchLen = _tcslen(szPart) + 3;
				pszConsoles[iCount] = (wchar_t*)malloc(cchLen*sizeof(wchar_t));
				_wsprintf(pszConsoles[iCount], SKIPLEN(cchLen) L"\"%s\"", (LPCWSTR)szPart);
				iCount++;

				cchAllLen += cchLen+3;
			}
		}

		SafeFree(pszArguments);

		if (pShellLink)
			pShellLink->Release();
		if (pFile)
			pFile->Release();

		if (iCount == 0)
			return NULL;

		if (iCount == 1)
			return pszConsoles[0];

		// Теперь - собрать pszDataW
		pszDataW = (wchar_t*)malloc(cchAllLen*sizeof(*pszDataW));
		*pszDataW = 0;
		for (size_t i = 0; i < iCount; i++)
		{
			_wcscat_c(pszDataW, cchAllLen, pszConsoles[i]);
			_wcscat_c(pszDataW, cchAllLen, L"\r\n");
			free(pszConsoles[i]);
		}
	}
	else
	{
		_ASSERTE(FALSE && "Invalid DropLnkPrefix");
	}

	return pszDataW;
}

wchar_t* CConEmuMain::LoadConsoleBatch_Task(LPCWSTR asSource, wchar_t** ppszStartupDir /*= NULL*/)
{
	wchar_t* pszDataW = NULL;

	if (asSource && ((*asSource == TaskBracketLeft) || (lstrcmp(asSource, AutoStartTaskName) == 0)))
	{
		wchar_t szName[MAX_PATH]; lstrcpyn(szName, asSource, countof(szName));
		wchar_t* psz = wcschr(szName, TaskBracketRight);
		if (psz) psz[1] = 0;

		const Settings::CommandTasks* pGrp = NULL;
		if (lstrcmp(asSource, AutoStartTaskName) == 0)
		{
			pGrp = gpSet->CmdTaskGet(-1);
		}
		else
		{
			for (int i = 0; (pGrp = gpSet->CmdTaskGet(i)) != NULL; i++)
			{
				if (lstrcmpi(pGrp->pszName, szName) == 0)
				{
					break;
				}
			}
		}
		if (pGrp)
		{
			pszDataW = lstrdup(pGrp->pszCommands);
			if (ppszStartupDir && !*ppszStartupDir && pGrp->pszGuiArgs)
			{
				wchar_t* pszIcon = NULL;
				pGrp->ParseGuiArgs(ppszStartupDir, &pszIcon);

				if (pszIcon)
				{
					SetWindowIcon(pszIcon);
					SafeFree(pszIcon);
				}
			}
		}

		if (!pszDataW || !*pszDataW)
		{
			size_t cchMax = _tcslen(szName)+100;
			wchar_t* pszErrMsg = (wchar_t*)calloc(cchMax,sizeof(*pszErrMsg));
			_wsprintf(pszErrMsg, SKIPLEN(cchMax) L"Command group %s %s!\n"
				L"Choose your shell?",
				szName, pszDataW ? L"is empty" : L"not found");

			int nBtn = MsgBox(pszErrMsg, MB_YESNO|MB_ICONEXCLAMATION);

			SafeFree(pszErrMsg);
			SafeFree(pszDataW);

			if (nBtn == IDYES)
			{
				LPCWSTR pszDefCmd = gpSetCls->GetDefaultCmd();

				RConStartArgs args;
				args.aRecreate = cra_EditTab;
				if (pszDefCmd && *pszDefCmd)
					args.pszSpecialCmd = lstrdup(pszDefCmd);

				int nCreateRc = gpConEmu->RecreateDlg(&args);

				if (nCreateRc == IDC_START)
				{
					wchar_t* pszNewCmd = args.CreateCommandLine();
					return pszNewCmd;
					//return lstrdup(pszDefCmd);
				}
			}
			return NULL;
		}

	}
	else
	{
		_ASSERTE(FALSE && "Invalid task name");
	}

	return pszDataW;
}

void CConEmuMain::PostCreate(BOOL abReceived/*=FALSE*/)
{
	if (gpSetCls->isAdvLogging)
	{
		wchar_t sInfo[100];
		_wsprintf(sInfo, SKIPLEN(countof(sInfo)) L"PostCreate.%u.begin (LeaveOnClose=%u, HideOnClose=%u)",
			abReceived ? 2 : 1, (UINT)gpSet->isMultiLeaveOnClose, (UINT)gpSet->isMultiHideOnClose);
		LogString(sInfo);
	}

	mn_StartupFinished = abReceived ? ss_PostCreate2Called : ss_PostCreate1Called;

	// First ShowWindow forced to use nCmdShow. This may be weird...
	SkipOneShowWindow();

	if (!abReceived)
	{
		//if (gpConEmu->WindowMode == wmFullScreen || gpConEmu->WindowMode == wmMaximized) {
#ifdef MSGLOGGER
		WINDOWPLACEMENT wpl; memset(&wpl, 0, sizeof(wpl)); wpl.length = sizeof(wpl);
		GetWindowPlacement(ghWnd, &wpl);
#endif

		CheckActiveLayoutName();

		session.SetSessionNotification(true);

		if (gpSet->isHideCaptionAlways())
		{
			OnHideCaption();
		}

		if (gpSet->isDesktopMode)
			OnDesktopMode();

		SetWindowMode((ConEmuWindowMode)gpSet->_WindowMode, FALSE, TRUE);

		mb_InCreateWindow = false;

		PostMessage(ghWnd, mn_MsgPostCreate, 0, 0);

		if (!mh_RightClickingBmp)
		{
			mh_RightClickingBmp = (HBITMAP)LoadImage(g_hInstance, MAKEINTRESOURCE(IDB_RIGHTCLICKING),
			                      IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION);

			if (mh_RightClickingBmp)
			{
				BITMAP bi = {0};

				if (GetObject(mh_RightClickingBmp, sizeof(bi), &bi)
				        && bi.bmWidth > bi.bmHeight && bi.bmHeight > 0)
				{
					m_RightClickingSize.x = bi.bmWidth; m_RightClickingSize.y = bi.bmHeight;
				}
				else
				{
					m_RightClickingSize.x = 384; m_RightClickingSize.y = 16;
				}

				_ASSERTE(m_RightClickingSize.y>0);
				m_RightClickingFrames = m_RightClickingSize.x / m_RightClickingSize.y;
				// по идее, должно быть ровно
				_ASSERTE(m_RightClickingSize.x == m_RightClickingFrames * m_RightClickingSize.y);
			}
		}
	}
	else // (abReceived == true)
	{
		HWND hCurForeground = GetForegroundWindow();

		//-- Перенесено в Taskbar_Init();
		//HRESULT hr = S_OK;
		//hr = OleInitialize(NULL);  // как бы попробовать включать Ole только во время драга. кажется что из-за него глючит переключалка языка
		//CoInitializeEx(NULL, COINIT_MULTITHREADED);

		// Может быть уже вызван в SkipOneShowWindow, а может и нет
		Taskbar_Init();

		RegisterMinRestore(true);

		RegisterHotKeys();

		// Если вдруг оказалось, что хоткей (из ярлыка) назначился для
		// окна отличного от главного
		if (!gnWndSetHotkeyOk && gnWndSetHotkey)
		{
			if (ghWndApp)
			{
				SendMessage(ghWndApp, WM_SETHOTKEY, 0, 0);
			}
			SendMessage(ghWnd, WM_SETHOTKEY, gnWndSetHotkey, 0);
		}

		//TODO: Возможно, стоит отложить запуск секунд на 5, чтобы не мешать инициализации?
		if (gpSet->UpdSet.isUpdateCheckOnStartup && !DisableAutoUpdate)
			CheckUpdates(FALSE); // Не показывать сообщение "You are using latest available version"

		//SetWindowRgn(ghWnd, CreateWindowRgn(), TRUE);

		if (gpSetCls->szFontError[0] && !(gpStartEnv && ((gpStartEnv->bIsWinPE == 1) || (gpStartEnv->bIsWine == 1))))
		{
			MBoxA(gpSetCls->szFontError);
			gpSetCls->szFontError[0] = 0;
		}

		if (!gpSetCls->CheckConsoleFontFast())
		{
			DontEnable de;
			gpSetCls->EditConsoleFont(ghWnd);
		}
		
		// Если в ключе [HKEY_CURRENT_USER\Console] будут левые значения - то в Win7 могут
		// начаться страшные глюки :-) 
		// например, консольное окно будет "дырявое" - рамка есть, а содержимого - нет :-P
		gpSet->CheckConsoleSettings();

		if (gpSet->isShowBgImage)
		{
			gpSetCls->LoadBackgroundFile(gpSet->sBgImage);
		}

		// Перенес перед созданием консоли, чтобы не блокировать SendMessage
		// (mp_DragDrop->Register() может быть относительно длительной операцией)
		if (!mp_DragDrop)
		{
			// было 'ghWnd DC'. Попробуем на главное окно, было бы удобно
			// "бросать" на таб (с автоматической активацией консоли)
			mp_DragDrop = new CDragDrop();

			if (!mp_DragDrop->Register())
			{
				CDragDrop *p = mp_DragDrop; mp_DragDrop = NULL;
				delete p;
			}
		}

		// Может быть в настройке указано - всегда показывать иконку в TSA
		Icon.SettingsChanged();

		// Проверка уникальности хоткеев
		gpSet->CheckHotkeyUnique();


		if (!isVConExists(0) || !gpConEmu->mb_StartDetached)  // Консоль уже может быть создана, если пришел Attach из ConEmuC
		{
			// Если надо - подготовить портабельный реестр
			if (mb_PortableRegExist)
			{
				// Если реестр обломался, или юзер сказал "не продолжать"
				if (!gpConEmu->PreparePortableReg())
				{
					Destroy();
					return;
				}
			}


			BOOL lbCreated = FALSE;
			bool isScript = false;
			LPCWSTR pszCmd = gpSetCls->GetCmd(&isScript);
			_ASSERTE(pszCmd!=NULL && *pszCmd!=0); // Must be!

			if (isScript)
			{
				wchar_t* pszDataW = lstrdup(pszCmd);

				// Если передали "скрипт" (как бы содержимое Task вытянутое в строку)
				// Обработать - заменить "|||" на " \r\n"
				// Раньше здесь было '|' но заменил на "|||" во избежание неоднозначностей
				wchar_t* pszNext = pszDataW;
				while ((pszNext = wcsstr(pszNext, L"|||")) != NULL)
				{
					*(pszNext++) = L' ';
					*(pszNext++) = L'\r';
					*(pszNext++) = L'\n';
				}

				// GO
				if (!CreateConGroup(pszDataW, FALSE, NULL/*pszStartupDir*/))
				{
					Destroy();
					return;
				}

				SafeFree(pszDataW);

				// сбросить команду, которая пришла из "/cmdlist" - загрузить настройку
				gpSetCls->ResetCmdArg();

				lbCreated = TRUE;
			}
			else if ((*pszCmd == CmdFilePrefix || *pszCmd == TaskBracketLeft || lstrcmpi(pszCmd,AutoStartTaskName) == 0) && !gpConEmu->mb_StartDetached)
			{
				wchar_t* pszStartupDir = NULL;
				// В качестве "команды" указан "пакетный файл" или "группа команд" одновременного запуска нескольких консолей
				wchar_t* pszDataW = LoadConsoleBatch(pszCmd, &pszStartupDir);
				if (!pszDataW)
				{
					Destroy();
					return;
				}

				// GO
				if (!CreateConGroup(pszDataW, FALSE, pszStartupDir))
				{
					Destroy();
					return;
				}

				SafeFree(pszDataW);
				SafeFree(pszStartupDir);

				//// Если ConEmu был запущен с ключом "/single /cmd xxx" то после окончания
				//// загрузки - сбросить команду, которая пришла из "/cmd" - загрузить настройку
				//if (gpSetCls->SingleInstanceArg)
				//{
				//	gpSetCls->ResetCmdArg();
				//}

				lbCreated = TRUE;
			}

			if (!lbCreated && !gpConEmu->mb_StartDetached)
			{
				RConStartArgs args;
				args.Detached = gpConEmu->mb_StartDetached ? crb_On : crb_Off;

				if (args.Detached != crb_On)
				{
					args.pszSpecialCmd = lstrdup(gpSetCls->GetCmd());

					if (!CreateCon(&args, TRUE))
					{
						DisplayLastError(L"Can't create new virtual console!");
						Destroy();
						return;
					}
				}
			}
		}

		if (gpConEmu->mb_StartDetached)
			gpConEmu->mb_StartDetached = FALSE;  // действует только на первую консоль

		//// Может быть в настройке указано - всегда показывать иконку в TSA
		//Icon.SettingsChanged();

		//if (!mp_TaskBar2)
		//{
		//	// В PostCreate это выполняется дольше всего. По идее мешать не должно,
		//	// т.к. серверная нить уже запущена.
		//	hr = CoCreateInstance(CLSID_TaskbarList,NULL,CLSCTX_INPROC_SERVER,IID_ITaskbarList2,(void**)&mp_TaskBar2);
		//
		//	if (hr == S_OK && mp_TaskBar2)
		//	{
		//		hr = mp_TaskBar2->HrInit();
		//	}
		//
		//	if (hr != S_OK && mp_TaskBar2)
		//	{
		//		if (mp_TaskBar2) mp_TaskBar2->Release();
		//
		//		mp_TaskBar2 = NULL;
		//	}
		//}
		//
		//if (!mp_TaskBar3 && mp_TaskBar2)
		//{
		//	hr = mp_TaskBar2->QueryInterface(IID_ITaskbarList3, (void**)&mp_TaskBar3);
		//}

		//if (!mp_DragDrop)
		//{
		//	// было 'ghWnd DC'. Попробуем на главное окно, было бы удобно
		//	// "бросать" на таб (с автоматической активацией консоли)
		//	mp_DragDrop = new CDragDrop();

		//	if (!mp_DragDrop->Register())
		//	{
		//		CDragDrop *p = mp_DragDrop; mp_DragDrop = NULL;
		//		delete p;
		//	}
		//}

		//TODO terst
		WARNING("Если консоль не создана - handler не установится!")
		//SetConsoleCtrlHandler((PHANDLER_ROUTINE)CConEmuMain::HandlerRoutine, true);
		
		// Если фокус был у нас - вернуть его (на всякий случай, вдруг RealConsole какая забрала
		if (mp_Inside)
		{
			if ((hCurForeground == ghWnd) || (hCurForeground != mp_Inside->mh_InsideParentRoot))
				apiSetForegroundWindow(mp_Inside->mh_InsideParentRoot);
			SetWindowPos(ghWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
		}
		else if ((hCurForeground == ghWnd) && !WindowStartMinimized)
		{
			apiSetForegroundWindow(ghWnd);
		}

		if (WindowStartMinimized)
		{
			_ASSERTE(!WindowStartNoClose || (WindowStartTsa && WindowStartNoClose));

			if (WindowStartTsa || gpConEmu->ForceMinimizeToTray)
			{
				Icon.HideWindowToTray();

				if (WindowStartNoClose)
				{
					LogString(L"Set up {isMultiLeaveOnClose=1 && isMultiHideOnClose=1} due to WindowStartTsa & WindowStartNoClose");
					//_ASSERTE(FALSE && "Set up {isMultiLeaveOnClose=1 && isMultiHideOnClose=1}");
					gpSet->isMultiLeaveOnClose = 1;
					gpSet->isMultiHideOnClose = 1;
				}
			}
			else if (IsWindowVisible(ghWnd) && !isIconic())
			{
				PostMessage(ghWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
			}

			WindowStartMinimized = false;
		}

		UpdateWin7TaskList(mb_UpdateJumpListOnStartup);
		mb_UpdateJumpListOnStartup = false;

		//RegisterHotKeys();
		//SetParent(GetParent(GetShellWindow()));
		UINT n = SetKillTimer(true, TIMER_MAIN_ID, TIMER_MAIN_ELAPSE/*gpSet->nMainTimerElapse*/);
		#ifdef _DEBUG
		DWORD dw = GetLastError();
		#endif
		n = SetKillTimer(true, TIMER_CONREDRAW_ID, CON_REDRAW_TIMOUT*2);
		UNREFERENCED_PARAMETER(n);
		OnActivateSplitChanged();

		if (gpSet->isTaskbarShield && gpConEmu->mb_IsUacAdmin && gpSet->isWindowOnTaskBar())
		{
			// Bug in Win7? Sometimes after startup "As Admin" sheild does not appeears.
			SetKillTimer(true, TIMER_ADMSHIELD_ID, TIMER_ADMSHIELD_ELAPSE);
		}

		mp_DefTrm->PostCreated();
	}

	mn_StartupFinished = abReceived ? ss_PostCreate2Finished : ss_PostCreate1Finished;

	if (gpSetCls->isAdvLogging)
	{
		wchar_t szInfo[64];
		_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"PostCreate.%i.end", abReceived ? 2 : 1);
		LogWindowPos(szInfo);
	}
}

// Это не Post, а Send. Синхронное создание окна в главной нити
HWND CConEmuMain::PostCreateView(CConEmuChild* pChild)
{
	HWND hChild = NULL;
	if (GetCurrentThreadId() == mn_MainThreadId)
	{
		hChild = pChild->CreateView();
	}
	else
	{
		hChild = (HWND)SendMessage(ghWnd, mn_MsgCreateViewWindow, 0, (LPARAM)pChild);
	}
	return hChild;
}

LRESULT CConEmuMain::OnDestroy(HWND hWnd)
{
	LogString(L"CConEmuMain::OnDestroy()");

	session.SetSessionNotification(false);

	// Нужно проверить, вдруг окно закрылось без нашего ведома (InsideIntegration)
	CVConGroup::OnDestroyConEmu();


	if (mp_AttachDlg)
	{
		mp_AttachDlg->Close();
	}
	if (mp_RecreateDlg)
	{
		mp_RecreateDlg->Close();
	}
	
	// Выполняется однократно (само проверит)
	gpSet->SaveSettingsOnExit();

	// Делать обязательно перед ResetEvent(mh_ConEmuAliveEvent), чтобы у другого
	// экземпляра не возникло проблем с регистрацией hotkey
	RegisterMinRestore(false);
	
	FinalizePortableReg();

	if (mb_ConEmuAliveOwned && mh_ConEmuAliveEvent)
	{
		ResetEvent(mh_ConEmuAliveEvent); // Дадим другим процессам "завладеть" первенством
		SafeCloseHandle(mh_ConEmuAliveEvent);
		mb_ConEmuAliveOwned = FALSE;
	}

	if (mb_MouseCaptured)
	{
		ReleaseCapture();
		mb_MouseCaptured = FALSE;
	}

	//120122 - Теперь через PipeServer
	m_GuiServer.Stop(true);


	CVConGroup::DestroyAllVCon();


	#ifndef _WIN64
	if (mh_WinHook)
	{
		UnhookWinEvent(mh_WinHook);
		mh_WinHook = NULL;
	}
	#endif

	//if (mh_PopupHook) {
	//	UnhookWinEvent(mh_PopupHook);
	//	mh_PopupHook = NULL;
	//}

	if (mp_DragDrop)
	{
		delete mp_DragDrop;
		mp_DragDrop = NULL;
	}

	//if (ProgressBars) {
	//    delete ProgressBars;
	//    ProgressBars = NULL;
	//}
	Icon.RemoveTrayIcon(true);

	Taskbar_Release();
	//if (mp_TaskBar3)
	//{
	//	mp_TaskBar3->Release();
	//	mp_TaskBar3 = NULL;
	//}
	//if (mp_TaskBar2)
	//{
	//	mp_TaskBar2->Release();
	//	mp_TaskBar2 = NULL;
	//}

	//if (mh_InsideSysMenu)
	//	DestroyMenu(mh_InsideSysMenu);

	UnRegisterHotKeys(TRUE);
	//if (mh_DwmApi && mh_DwmApi != INVALID_HANDLE_VALUE)
	//{
	//	FreeLibrary(mh_DwmApi); mh_DwmApi = NULL;
	//	DwmIsCompositionEnabled = NULL;
	//}
	SetKillTimer(false, TIMER_MAIN_ID, 0);
	SetKillTimer(false, TIMER_CONREDRAW_ID, 0);
	SetKillTimer(false, TIMER_CAPTION_APPEAR_ID, 0);
	SetKillTimer(false, TIMER_CAPTION_DISAPPEAR_ID, 0);
	SetKillTimer(false, TIMER_QUAKE_AUTOHIDE_ID, 0);
	SetKillTimer(false, TIMER_ACTIVATESPLIT_ID, 0);
	PostQuitMessage(0);
	return 0;
}

LRESULT CConEmuMain::OnFlashWindow(WPARAM wParam, LPARAM lParam)
{
	DWORD nOpt = ((DWORD)wParam & 0x00F00000) >> 20;
	DWORD nFlags = ((DWORD)wParam & 0xFF000000) >> 24;
	DWORD nCount = (DWORD)wParam & 0xFFFFF;
	CVConGroup::OnFlashWindow(nOpt, nFlags, nCount, (HWND)lParam);

	return 0;
}

void CConEmuMain::DoFlashWindow(CESERVER_REQ_FLASHWINFO* pFlash, bool bFromMacro)
{
	WPARAM wParam = 0;

	if (pFlash->bSimple)
	{
		wParam = 0x00100000 | (pFlash->bInvert ? 0x00200000 : 0) | (bFromMacro ? 0x00400000 : 0);
	}
	else
	{
		wParam = ((pFlash->dwFlags & 0xFF) << 24) | (pFlash->uCount & 0xFFFFF) | (bFromMacro ? 0x00400000 : 0);
	}

	PostMessage(ghWnd, mn_MsgFlashWindow, wParam, (LPARAM)pFlash->hWnd.u);
}

void CConEmuMain::setFocus()
{
#ifdef _DEBUG
	DWORD nSelfPID = GetCurrentProcessId();
	wchar_t sFore[1024], sFocus[1024]; sFore[0] = sFocus[0] = 0;
	HWND hFocused = GetFocus();
	DWORD nFocusPID = 0; if (hFocused) { GetWindowThreadProcessId(hFocused, &nFocusPID); getWindowInfo(hFocused, sFocus); }
	HWND hFore = GetForegroundWindow();
	DWORD nForePID = 0; if (hFore) { GetWindowThreadProcessId(hFore, &nForePID); getWindowInfo(hFore, sFore); }
	bool bNeedFocus = false;
	if (hFocused != ghWnd)
		bNeedFocus = true;
	CVConGuard VCon;
	HWND hGuiWnd = (GetActiveVCon(&VCon) >= 0) ? VCon->GuiWnd() : NULL;

	// -- ассерт убрал, может возникать при переключении на консоль (табы)
	//_ASSERTE(hGuiWnd==NULL || !IsWindowVisible(hGuiWnd));
#endif

	SetFocus(ghWnd);
}

bool CConEmuMain::SetSkipOnFocus(bool abSkipOnFocus)
{
	bool bPrev = mb_SkipOnFocus;
	mb_SkipOnFocus = abSkipOnFocus;
	return bPrev;
}

void CConEmuMain::OnOurDialogOpened()
{
	mb_AllowAutoChildFocus = false;
}

void CConEmuMain::OnOurDialogClosed()
{
	CheckAllowAutoChildFocus((DWORD)-1);
}

bool CConEmuMain::isMenuActive()
{
	return (mp_Menu->GetTrackMenuPlace() != tmp_None);
}

bool CConEmuMain::CanSetChildFocus()
{
	if (isMenuActive())
		return false;
	return true;
}

void CConEmuMain::CheckAllowAutoChildFocus(DWORD nDeactivatedTID)
{
	if (!gpSet->isFocusInChildWindows)
	{
		mb_AllowAutoChildFocus = false;
		return;
	}

	bool bAllowAutoChildFocus = false;
	CVConGuard VCon;
	HWND hChild = NULL;

	if (gpConEmu->GetActiveVCon(&VCon) >= 0
		&& ((hChild = VCon->GuiWnd()) != NULL))
	{
		if (nDeactivatedTID == (DWORD)-1)
		{
			bAllowAutoChildFocus = true;
		}
		// просто так фокус в дочернее окно ставить нельзя
		// если переключать фокус в дочернее приложение по любому чиху
		// вообще не получается активировать окно ConEmu, открыть системное меню,
		// клацнуть по крестику в заголовке и т.п.
		else if (nDeactivatedTID)
		{
			DWORD nChildTID = GetWindowThreadProcessId(hChild, NULL);
			DWORD nConEmuTID = GetWindowThreadProcessId(ghWnd, NULL);

			if ((nDeactivatedTID != nChildTID)
				&& (nDeactivatedTID != nConEmuTID)) // это на всякий случай...
			{
				// gpConEmu->isMeForeground(true, true) -- уже дает неправильный результат, ConEmu считается активным
				bAllowAutoChildFocus = true;
			}
		}
	}

	mb_AllowAutoChildFocus = bAllowAutoChildFocus;
}

LRESULT CConEmuMain::OnFocus(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, LPCWSTR asMsgFrom /*= NULL*/, BOOL abForceChild /*= FALSE*/)
{
	// Чтобы избежать лишних вызовов по CtrlWinAltSpace при работе с GUI приложением
	if (mb_SkipOnFocus)
	{
		return 0;
	}

	bool lbSetFocus = false;
	#ifdef _DEBUG
	DWORD lnForceTimerCheckLoseFocus = mn_ForceTimerCheckLoseFocus;
	#endif
	
	WCHAR szDbg[128];

	LPCWSTR pszMsgName = L"Unknown";
	HWND hNewFocus = NULL;
	HWND hForeground = NULL;

	if (messg == WM_SETFOCUS)
	{
		lbSetFocus = true;
		_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_SETFOCUS(From=0x%08X)", (DWORD)wParam);
		LogFocusInfo(szDbg);
		pszMsgName = L"WM_SETFOCUS";
	}
	else if (messg == WM_ACTIVATE)
	{
		lbSetFocus = (LOWORD(wParam)==WA_ACTIVE) || (LOWORD(wParam)==WA_CLICKACTIVE);

		if (LOWORD(wParam)==WA_INACTIVE)
		{
			hForeground = GetForegroundWindow();
			hNewFocus = lParam ? (HWND)lParam : hForeground;
			if (CVConGroup::isOurWindow(hNewFocus))
			{
				lbSetFocus = true; // Считать, что фокус мы не теряем!

				// Клик мышкой в области дочернего GUI-приложения, запущенного в ConEmu
				// isPressed - не подходит
				if (!CVConGroup::InCreateGroup())
				{
					POINT ptCur = {}; GetCursorPos(&ptCur);
					for (size_t i = 0;; i++)
					{
						CVConGuard VCon;
						if (!CVConGroup::GetVCon(i, &VCon))
							break;
						if (!CVConGroup::isVisible(VCon.VCon()))
							continue;

						HWND hGuiWnd = VCon->GuiWnd();
						RECT rcChild = {}; GetWindowRect(hGuiWnd, &rcChild);
						if (PtInRect(&rcChild, ptCur))
						{
							Activate(VCon.VCon());
						}
					}
				}
			}
			else if (hNewFocus)
			{
				// Запускается новая консоль в режиме "администратора"?
				if (CVConGroup::isInCreateRoot())
				{
					wchar_t szClass[MAX_PATH];
					if (GetClassName(hNewFocus, szClass, countof(szClass)) && isConsoleClass(szClass))
					{
						lbSetFocus = true;
					}
				}
			}
		}

		if (!lbSetFocus && gpSet->isDesktopMode && mh_ShellWindow)
		{
			if (isPressed(VK_LBUTTON) || isPressed(VK_RBUTTON))
			{
				// При активации _Десктопа_ мышкой - запомнить, что ставить фокус в себя не нужно
				POINT ptCur; GetCursorPos(&ptCur);
				HWND hFromPoint = WindowFromPoint(ptCur);

				//if (hFromPoint == mh_ShellWindow) TODO: если сразу совпало - сразу выходим

				// mh_ShellWindow чисто для информации. Хоть родитель ConEmu и меняется на mh_ShellWindow
				// но проводник может перекинуть наше окно в другое (WorkerW или Progman)
				if (hFromPoint)
				{
					bool lbDesktopActive = false;
					wchar_t szClass[128];

					// Нужно учесть, что еще могут быть всякие бары, панели, и прочее, лежащие на десктопе
					while (hFromPoint)
					{
						if (GetClassName(hFromPoint, szClass, 127))
						{
							if (!wcscmp(szClass, L"WorkerW") || !wcscmp(szClass, L"Progman"))
							{
								DWORD dwPID;
								GetWindowThreadProcessId(hFromPoint, &dwPID);
								lbDesktopActive = (dwPID == mn_ShellWindowPID);
								break;
							}
						}

						// может таки что-то дочернее попалось?
						hFromPoint = GetParent(hFromPoint);
					}

					if (lbDesktopActive)
						mb_FocusOnDesktop = FALSE;
				}
			}
		}
		else if (lbSetFocus && gpSet->isQuakeStyle && isQuakeMinimized)
		{
			bool lbSkipQuakeActivation = false;
			if ((LOWORD(wParam) == WA_ACTIVE) && (HIWORD(wParam) != 0))
			{
				// Сюда мы попадаем если "активируется" кнопка на таскбаре, но без разворачивания окна
				// Пример. Наше окно единственное, жмем кнопку минимизации в тулбаре,
				// Quake сворачивается, но т.к. других окон нет - фокус "остается" в нашем
				// окне (в кнопке на таскбаре) и приходит WM_ACTIVATE.
				// Здесь его нужно игнорировать, иначе Quake нифига не "свернется"
				lbSkipQuakeActivation = true;
			}
			else if (mb_IgnoreQuakeActivation)
			{
				// Правый клик на иконке в TSA, тоже НЕ выезжать
				lbSkipQuakeActivation = true;
			}

			if (!lbSkipQuakeActivation)
			{
				// Обработать клик по кнопке на таскбаре
				DoMinimizeRestore(sih_ShowMinimize);
			}
		}

		if (LOWORD(wParam)==WA_ACTIVE)
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_ACTIVATE(From=0x%08X)", (DWORD)lParam);
		else
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_ACTIVATE.WA_INACTIVE(To=0x%08X) OurFocus=%i", (DWORD)lParam, lbSetFocus);
		LogFocusInfo(szDbg);

		pszMsgName = L"WM_ACTIVATE";
	}
	else if (messg == WM_ACTIVATEAPP)
	{
		lbSetFocus = (wParam!=0);
		if (!lbSetFocus)
		{
			hNewFocus = hForeground = GetForegroundWindow();
		}

		if (lbSetFocus)
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_ACTIVATEAPP.Activate(FromTID=%i)", (DWORD)lParam);
		else
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_ACTIVATEAPP.Deactivate(ToTID=%i) OurFocus=%i", (DWORD)lParam, lbSetFocus);
		LogFocusInfo(szDbg);

		if (!lbSetFocus)
		{
			if (CVConGroup::isOurWindow(hNewFocus))
			{
				lbSetFocus = true; // Считать, что фокус мы не теряем!
			}
		}
		else if (gpSet->isQuakeStyle)
		{
			if (!mb_IgnoreQuakeActivation)
			{
				DoMinimizeRestore(sih_Show);
			}
		}

		pszMsgName = L"WM_ACTIVATEAPP";
	}
	else if (messg == WM_KILLFOCUS)
	{
		pszMsgName = L"WM_KILLFOCUS";
		hForeground = GetForegroundWindow();
		hNewFocus = wParam ? (HWND)wParam : hForeground;

		if (CVConGroup::isOurWindow(hNewFocus))
		{
			lbSetFocus = true; // Считать, что фокус мы не теряем!
		}

		_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_KILLFOCUS(To=0x%08X) OurFocus=%i", (DWORD)wParam, lbSetFocus);
		LogFocusInfo(szDbg);

	}
	else
	{
		_ASSERTE(messg == 0 && asMsgFrom != NULL);
		if (asMsgFrom)
			pszMsgName = asMsgFrom;

		if (gpSet->isDesktopMode)
		{
			CheckFocus(pszMsgName);
		}

		if (gpSet->isDesktopMode || mp_Inside)
		{
			// В этих режимах игнорировать
			return 0;
		}

		hNewFocus = GetForegroundWindow();

		lbSetFocus = CVConGroup::isOurWindow(hNewFocus);

		if (mb_LastTransparentFocused != lbSetFocus)
		{
			OnTransparentSeparate(lbSetFocus);
		}

		if (!lbSetFocus && (asMsgFrom == gsFocusQuakeCheckTimer)
			&& mn_ForceTimerCheckLoseFocus
			//&& ((GetTickCount() - mn_ForceTimerCheckLoseFocus) >= QUAKE_FOCUS_CHECK_TIMER_DELAY)
			)
		{
			// сброс, однократная проверка для "Hide on focus lose"
			mn_ForceTimerCheckLoseFocus = 0;
		}
		else if (!lbSetFocus || mb_LastConEmuFocusState)
		{
			// Logging
			bool bNeedLog = RELEASEDEBUGTEST((gpSetCls->isAdvLogging>=2),true);
			if (bNeedLog)
			{
				HWND hFocus = GetFocus();
				wchar_t szInfo[128];
				_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"Focus event skipped, lbSetFocus=%u, mb_LastConEmuFocusState=%u, ghWnd=x%08X, hFore=x%08X, hFocus=x%08X", lbSetFocus, mb_LastConEmuFocusState, (DWORD)ghWnd, (DWORD)hNewFocus, (DWORD)hFocus);
				LogFocusInfo(szInfo, 2);
			}

			// Не дергаться
			return 0;
		}

		// Сюда мы доходим, если произошла какая-то ошибка, и ConEmu не получил
		// информации о том, что его окно было активировано. Нужно уведомить сервер.
		_ASSERTE(lbSetFocus || lnForceTimerCheckLoseFocus);
	}


	CheckFocus(pszMsgName);

	CVirtualConsole* pVCon = NULL;
	CVConGuard VCon;
	if (GetActiveVCon(&VCon) >= 0)
		pVCon = VCon.VCon();
		
	HWND hGuiWnd = pVCon ? pVCon->RCon()->GuiWnd() : NULL; // GUI режим (дочернее окно)

	// Если фокус "забрало" какое-либо дочернее окно в ConEmu (VideoRenderer - 'ActiveMovie Window')
	if (hNewFocus && hNewFocus != ghWnd && hNewFocus != hGuiWnd)
	{
		HWND hParent = hNewFocus;

		while ((hParent = GetParent(hNewFocus)) != NULL)
		{
			if (hParent == hGuiWnd)
				break;
			else if (hParent == ghWnd)
			{
				DWORD dwStyle = GetWindowLong(hNewFocus, GWL_STYLE);

				if ((dwStyle & (WS_POPUP|WS_OVERLAPPEDWINDOW|WS_DLGFRAME)) != 0)
					break; // Это диалог, не трогаем

				setFocus();
				hNewFocus = GetFocus();

				if (hNewFocus != ghWnd)
				{
					_ASSERTE(hNewFocus == ghWnd);
				}
				else
				{
					LogFocusInfo(L"Focus was returned to ConEmu");
				}

				lbSetFocus = (hNewFocus == ghWnd);
				break;
			}

			hNewFocus = hParent;
		}
	}

	if (!lbSetFocus && !hNewFocus && (isPressed(VK_LBUTTON) || isPressed(VK_RBUTTON)))
	{
		// Клик по таскбару, или еще кому...
		POINT ptCur = {}; GetCursorPos(&ptCur);
		HWND hUnderMouse = WindowFromPoint(ptCur);
		DWORD nPID = 0;
		if (hUnderMouse && GetWindowThreadProcessId(hUnderMouse, &nPID) && (nPID != GetCurrentProcessId()))
		{
			hNewFocus = hUnderMouse;
		}
	}

	// GUI client?
	if (!lbSetFocus && hNewFocus)
	{
		DWORD nNewPID = 0; GetWindowThreadProcessId(hNewFocus, &nNewPID);
		if (nNewPID == GetCurrentProcessId())
		{
			lbSetFocus = true;
		}
		else if (pVCon)
		{
			if (hNewFocus == pVCon->RCon()->ConWnd())
			{
				lbSetFocus = true;
			}
			else
			{
				
				DWORD nRConPID = pVCon->RCon()->GetActivePID();
				if (nNewPID == nRConPID)
					lbSetFocus = true;
			}
		}
	}



	/****************************************************************/
	/******      Done, Processing Activation/Deactivation      ******/
	/****************************************************************/

	UpdateGuiInfoMappingActive(lbSetFocus);

	mb_LastConEmuFocusState = lbSetFocus;
	LogFocusInfo(lbSetFocus ? L"mb_LastConEmuFocusState set to TRUE" : L"mb_LastConEmuFocusState set to FALSE");


	// Если для активного и НЕактивного окна назначены разные значения
	OnTransparentSeparate(lbSetFocus);

	//
	if (!lbSetFocus)
	{
		if (pVCon)
			FixSingleModifier(0, pVCon->RCon());
		mp_TabBar->SwitchRollback();
		UnRegisterHotKeys();
	}
	else if (!mb_HotKeyRegistered)
	{
		RegisterHotKeys();
	}

	if (pVCon && pVCon->RCon())
	{
		// Здесь будет вызван CRealConsole::UpdateServerActive

		// -- просто так abForceChild=TRUE здесь ставить нельзя
		// -- если переключать фокус в дочернее приложение по любому чиху
		// -- вообще не получается активировать окно ConEmu, открыть системное меню,
		// -- клацнуть по крестику в заголовке и т.п.
		// -- Поэтому abForceChild передается из вызывающей функции, которая "знает"

		pVCon->RCon()->OnGuiFocused(lbSetFocus, abForceChild);
	}

	if (gpSet->isMinimizeOnLoseFocus()
		&& (!lbSetFocus && !isMeForeground(true,true)) && !isIconic()
		&& IsWindowVisible(ghWnd))
	{
		wchar_t szClassName[255]; szClassName[0] = 0;
		bool bIsTaskbar = false, bIsTrayNotifyWnd = false;
		HWND h = hNewFocus;

		if (messg != 0)
		{
			while (h)
			{
				GetClassName(h, szClassName, countof(szClassName));
				if (lstrcmpi(szClassName, L"Shell_TrayWnd") == 0)
				{
					bIsTaskbar = true;
					break;
				}
				else if (lstrcmpi(szClassName, L"TrayNotifyWnd") == 0)
				{
					bIsTrayNotifyWnd = true;
				}
				h = GetParent(h);
			}
		}

		if (RELEASEDEBUGTEST(gpSetCls->isAdvLogging,true))
		{
			wchar_t szInfo[512];
			_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"MinimizeOnFocusLose: Class=%s, IsTaskbar=%u, IsTrayNotifyWnd=%u, Timer=%u",
				szClassName, bIsTaskbar, bIsTrayNotifyWnd, mn_ForceTimerCheckLoseFocus);
			LogFocusInfo(szInfo);
		}

		// Если активируется TaskBar - не дергаться.
		// Иначе получается бред при клике на иконку в TSA.
		if (!bIsTaskbar)
		{
			if (!mb_IgnoreQuakeActivation)
			{
				DoMinimizeRestore(sih_AutoHide);
			}
		}
		else if (!mn_ForceTimerCheckLoseFocus)
		{
			mn_ForceTimerCheckLoseFocus = GetTickCount();
			_ASSERTE(mn_ForceTimerCheckLoseFocus!=0);
			SetKillTimer(true, TIMER_QUAKE_AUTOHIDE_ID, TIMER_QUAKE_AUTOHIDE_ELAPSE);
		}
	}
	
	if (gpSet->isFadeInactive)
	{
		if (pVCon)
		{
			bool bForeground = lbSetFocus || isMeForeground();
			bool bLastFade = (pVCon!=NULL) ? pVCon->mb_LastFadeFlag : false;
			bool bNewFade = (gpSet->isFadeInactive && !bForeground && !isPictureView());

			if (bLastFade != bNewFade)
			{
				if (pVCon) pVCon->mb_LastFadeFlag = bNewFade;

				pVCon->Invalidate();
			}
		}

		mp_Status->UpdateStatusBar(true);
	}

	// Может настройки ControlPanel изменились?
	mouse.ReloadWheelScroll();

	if (lbSetFocus && pVCon && gpSet->isTabsOnTaskBar())
	{
		HWND hFore = GetForegroundWindow();
		if (hFore == ghWnd)
		{
			if (!mb_PostTaskbarActivate)
			{
				//mp_ VActive->OnTaskbarFocus();
				mb_PostTaskbarActivate = TRUE;
				PostMessage(ghWnd, mn_MsgPostTaskbarActivate, 0, 0);
			}
		}
	}

	if (gpSet->isSkipFocusEvents)
		return 0;

#ifdef MSGLOGGER
	/*if (messg == WM_ACTIVATE && wParam == WA_INACTIVE) {
	    WCHAR szMsg[128]; _wsprintf(szMsg, countof(szMsg), L"--Deactivating to 0x%08X\n", lParam);
	    DEBUGSTR(szMsg);
	}
	switch (messg) {
	    case WM_SETFOCUS:
	        {
	            DEBUGSTR(L"--Get focus\n");
	            //return 0;
	        } break;
	    case WM_KILLFOCUS:
	        {
	            DEBUGSTR(L"--Loose focus\n");
	        } break;
	}*/
#endif

	if (pVCon /*&& (messg == WM_SETFOCUS || messg == WM_KILLFOCUS)*/)
	{
		pVCon->RCon()->OnFocus(lbSetFocus);
	}

	return 0;
}

void CConEmuMain::StorePreMinimizeMonitor()
{
	// Запомним, на каком мониторе мы были до минимзации
	if (!isIconic())
	{
		mh_MinFromMonitor = MonitorFromWindow(ghWnd, MONITOR_DEFAULTTONEAREST);
	}
}

HMONITOR CConEmuMain::GetNearestMonitor(MONITORINFO* pmi /*= NULL*/, LPCRECT prcWnd /*= NULL*/)
{
	HMONITOR hMon = NULL;
	MONITORINFO mi = {sizeof(mi)};

	if (prcWnd)
	{
		hMon = GetNearestMonitorInfo(&mi, NULL, prcWnd);
	}
	else if (!ghWnd || (gpSet->isQuakeStyle && isIconic()))
	{
		_ASSERTE(gpConEmu->WndWidth.Value>0 && gpConEmu->WndHeight.Value>0);
		RECT rcEvalWnd = GetDefaultRect();
		hMon = GetNearestMonitorInfo(&mi, NULL, &rcEvalWnd);
	}
	else if (!mp_Menu->GetRestoreFromMinimized() && !isIconic())
	{
		hMon = GetNearestMonitorInfo(&mi, NULL, NULL, ghWnd);
	}
	else
	{
		_ASSERTE(!mp_Inside); // По идее, для "Inside" вызываться не должен

		//-- CalcRect использовать нельзя, он может ссылаться на GetNearestMonitor
		//RECT rcDefault = CalcRect(CER_MAIN);
		WINDOWPLACEMENT wpl = {sizeof(wpl)};
		GetWindowPlacement(ghWnd, &wpl);
		RECT rcDefault = wpl.rcNormalPosition;
		hMon = GetNearestMonitorInfo(&mi, mh_MinFromMonitor, &rcDefault);
	}

	// GetNearestMonitorInfo failed?
	_ASSERTE(hMon && mi.cbSize && !IsRectEmpty(&mi.rcMonitor) && !IsRectEmpty(&mi.rcWork));

	if (pmi)
	{
		*pmi = mi;
	}
	return hMon;
}

HMONITOR CConEmuMain::GetPrimaryMonitor(MONITORINFO* pmi /*= NULL*/)
{
	MONITORINFO mi = {sizeof(mi)};
	HMONITOR hMon = GetPrimaryMonitorInfo(&mi);

	// GetPrimaryMonitorInfo failed?
	_ASSERTE(hMon && mi.cbSize && !IsRectEmpty(&mi.rcMonitor) && !IsRectEmpty(&mi.rcWork));

	if (gpSetCls->isAdvLogging >= 2)
	{
		wchar_t szInfo[128];
		_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"  GetPrimaryMonitor=%u -> hMon=x%08X Work=({%i,%i}-{%i,%i}) Area=({%i,%i}-{%i,%i})",
			(hMon!=NULL), (DWORD)hMon,
			mi.rcWork.left, mi.rcWork.top, mi.rcWork.right, mi.rcWork.bottom,
            mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom);
        LogString(szInfo);
	}

	if (pmi)
	{
		*pmi = mi;
	}
	return hMon;
}

LRESULT CConEmuMain::OnGetMinMaxInfo(LPMINMAXINFO pInfo)
{
	bool bLog = RELEASEDEBUGTEST((gpSetCls->isAdvLogging >= 2),true);
	wchar_t szMinMax[255];
	RECT rcWnd;

	if (bLog)
	{
		GetWindowRect(ghWnd, &rcWnd);
		_wsprintf(szMinMax, SKIPLEN(countof(szMinMax)) L"OnGetMinMaxInfo[before] MaxSize={%i,%i}, MaxPos={%i,%i}, MinTrack={%i,%i}, MaxTrack={%i,%i}, Cur={%i,%i}-{%i,%i}\n",
	          pInfo->ptMaxSize.x, pInfo->ptMaxSize.y,
	          pInfo->ptMaxPosition.x, pInfo->ptMaxPosition.y,
	          pInfo->ptMinTrackSize.x, pInfo->ptMinTrackSize.y,
	          pInfo->ptMaxTrackSize.x, pInfo->ptMaxTrackSize.y,
			  rcWnd.left, rcWnd.top, rcWnd.right, rcWnd.bottom);
		if (gpSetCls->isAdvLogging >= 2)
			LogString(szMinMax, true, false);
		DEBUGSTRSIZE(szMinMax);
	}


	MONITORINFO mi = {sizeof(mi)}, prm = {sizeof(prm)};
	GetNearestMonitor(&mi);
	if (gnOsVer >= 0x600)
		prm = mi;
	else
		GetPrimaryMonitor(&prm);
		

	// *** Минимально допустимые размеры консоли
	RECT rcMin = MakeRect(MIN_CON_WIDTH,MIN_CON_HEIGHT);
	if (isVConExists(0))
		rcMin = CVConGroup::AllTextRect(true);
	RECT rcFrame = CalcRect(CER_MAIN, rcMin, CER_CONSOLE_ALL);
	pInfo->ptMinTrackSize.x = rcFrame.right;
	pInfo->ptMinTrackSize.y = rcFrame.bottom;


	if ((WindowMode == wmFullScreen) || (gpSet->isQuakeStyle && (gpSet->_WindowMode == wmFullScreen)))
	{
		if (pInfo->ptMaxTrackSize.x < ptFullScreenSize.x)
			pInfo->ptMaxTrackSize.x = ptFullScreenSize.x;

		if (pInfo->ptMaxTrackSize.y < ptFullScreenSize.y)
			pInfo->ptMaxTrackSize.y = ptFullScreenSize.y;

		pInfo->ptMaxPosition.x = 0;
		pInfo->ptMaxPosition.y = 0;
		pInfo->ptMaxSize.x = GetSystemMetrics(SM_CXFULLSCREEN);
		pInfo->ptMaxSize.y = GetSystemMetrics(SM_CYFULLSCREEN);

		//if (pInfo->ptMaxSize.x < ptFullScreenSize.x)
		//	pInfo->ptMaxSize.x = ptFullScreenSize.x;

		//if (pInfo->ptMaxSize.y < ptFullScreenSize.y)
		//	pInfo->ptMaxSize.y = ptFullScreenSize.y;
	}
	else if (gpSet->isQuakeStyle)
	{
		_ASSERTE(::IsZoomed(ghWnd) == FALSE); // Стиль WS_MAXIMIZE для Quake выставляться НЕ ДОЛЖЕН!
		// Поэтому здесь нас интересует только "Normal"

		RECT rcWork;
		if (gpSet->_WindowMode == wmFullScreen)
		{
			rcWork = mi.rcMonitor;
		}
		else
		{
			rcWork = mi.rcWork;
		}

		if (pInfo->ptMaxTrackSize.x < (rcWork.right - rcWork.left))
			pInfo->ptMaxTrackSize.x = (rcWork.right - rcWork.left);

		if (pInfo->ptMaxTrackSize.y < (rcWork.bottom - rcWork.top))
			pInfo->ptMaxTrackSize.y = (rcWork.bottom - rcWork.top);
	}
	else //if (WindowMode == wmMaximized)
	{
		RECT rcShift = CalcMargins(CEM_FRAMEONLY, (WindowMode == wmNormal) ? wmMaximized : WindowMode);

		if (gnOsVer >= 0x600)
		{
			pInfo->ptMaxPosition.x = (mi.rcWork.left - mi.rcMonitor.left) - rcShift.left;
			pInfo->ptMaxPosition.y = (mi.rcWork.top - mi.rcMonitor.top) - rcShift.top;

			pInfo->ptMaxSize.x = mi.rcWork.right - mi.rcWork.left + (rcShift.left + rcShift.right);
			pInfo->ptMaxSize.y = mi.rcWork.bottom - mi.rcWork.top + (rcShift.top + rcShift.bottom);
		}
		else
		{
			// Issue 721: WinXP, TaskBar on top of screen. Assuming, mi.rcWork as {0,0,...}
			pInfo->ptMaxPosition.x = /*mi.rcWork.left*/ - rcShift.left;
			pInfo->ptMaxPosition.y = /*mi.rcWork.top*/ - rcShift.top;

			pInfo->ptMaxSize.x = prm.rcWork.right - prm.rcWork.left + (rcShift.left + rcShift.right);
			pInfo->ptMaxSize.y = prm.rcWork.bottom - prm.rcWork.top + (rcShift.top + rcShift.bottom);
		}
	}


	// Что получилось...
	if (bLog)
	{
		_wsprintf(szMinMax, SKIPLEN(countof(szMinMax)) L"OnGetMinMaxInfo[after]  MaxSize={%i,%i}, MaxPos={%i,%i}, MinTrack={%i,%i}, MaxTrack={%i,%i}, Cur={%i,%i}-{%i,%i}\n",
	          pInfo->ptMaxSize.x, pInfo->ptMaxSize.y,
	          pInfo->ptMaxPosition.x, pInfo->ptMaxPosition.y,
	          pInfo->ptMinTrackSize.x, pInfo->ptMinTrackSize.y,
	          pInfo->ptMaxTrackSize.x, pInfo->ptMaxTrackSize.y,
			  rcWnd.left, rcWnd.top, rcWnd.right, rcWnd.bottom);
		if (gpSetCls->isAdvLogging >= 2)
			LogString(szMinMax, true, false);
		DEBUGSTRSIZE(szMinMax);
	}


	// If an application processes this message, it should return zero.
	return 0;
}

void CConEmuMain::OnHideCaption()
{
	if (m_JumpMonitor.bInJump)
	{
		if (gpSetCls->isAdvLogging > 1) LogString(L"OnHideCaption skipped due to m_JumpMonitor.bInJump");
		return;
	}

	mp_TabBar->OnCaptionHidden();

	//if (isZoomed())
	//{
	//	SetWindowMode(wmMaximized, TRUE);
	//}

	DEBUGTEST(bool bHideCaption = gpSet->isCaptionHidden());

	DWORD nStyle = GetWindowLong(ghWnd, GWL_STYLE);
	DWORD nNewStyle = FixWindowStyle(nStyle, WindowMode);
	DWORD nStyleEx = GetWindowLong(ghWnd, GWL_EXSTYLE);
	bool bNeedCorrect = (!isIconic() && (WindowMode != wmFullScreen)) && !m_QuakePrevSize.bWaitReposition;
	
	if (nNewStyle != nStyle)
	{
		if (gpSetCls->isAdvLogging)
		{
			wchar_t szInfo[128];
			_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"Changing main window 0x%08X style to 0x%08X", (DWORD)ghWnd, nNewStyle);
			gpConEmu->LogString(szInfo);
		}

		mb_IgnoreSizeChange = true;

		RECT rcClient = {}, rcBefore = {}, rcAfter = {};
		if (bNeedCorrect)
		{
			// AdjustWindowRectEx НЕ должно вызываться в FullScreen. Глючит. А рамки с заголовком нет, расширять нечего.
			_ASSERTE(!isFullScreen() || (WindowMode==wmNormal || WindowMode==wmMaximized));

			// Тут нужен "натуральный" ClientRect
			::GetClientRect(ghWnd, &rcClient);
			rcBefore = rcClient;
			MapWindowPoints(ghWnd, NULL, (LPPOINT)&rcBefore, 2);
			rcAfter = rcBefore;
			AdjustWindowRectEx(&rcBefore, nStyle, FALSE, nStyleEx);
			if (isZoomed())
			{
				int nCapY = GetSystemMetrics(SM_CYCAPTION);
				if (gpSet->isCaptionHidden())
					rcAfter.top -= nCapY;
				else
					rcAfter.top += nCapY;
			}
			AdjustWindowRectEx(&rcAfter, nNewStyle, FALSE, nStyleEx);

			// When switching from "No caption" to "Caption"
			if (WindowMode == wmNormal)
			{
				// we need to check window location, caption must not be "Out-of-screen"
				FixWindowRect(rcAfter, 0);
			}
		}

		//nStyle &= ~(WS_CAPTION|WS_THICKFRAME|WS_DLGFRAME);
		//SetWindowStyle(nStyle);
		//nStyle = FixWindowStyle(nStyle);
		//POINT ptBefore = {}; ClientToScreen(ghWnd, &ptBefore);

		if (bNeedCorrect && (rcBefore.left != rcAfter.left || rcBefore.top != rcAfter.top))
		{
			m_FixPosAfterStyle = GetTickCount();
			mrc_FixPosAfterStyle = rcAfter;
			//DWORD nFlags = SWP_NOSIZE|SWP_NOZORDER;
			/////nFlags |= SWP_DRAWFRAME;
			//SetWindowPos(ghWnd, NULL, rcAfter.left, rcAfter.top, 0,0, nFlags);
		}
		else
		{
			m_FixPosAfterStyle = 0;
		}

		mb_InCaptionChange = true;

		SetWindowStyle(nNewStyle);

		mb_InCaptionChange = false;

		//POINT ptAfter = {}; ClientToScreen(ghWnd, &ptAfter);
		//if ((!isIconic() && !isZoomed() && !isFullScreen())
		//	&& (ptBefore.x != ptAfter.x || ptBefore.y != ptAfter.y))
		//{
		//	RECT rcPos = {}; GetWindowRect(ghWnd, &rcPos);
		//	SetWindowPos(ghWnd, NULL, rcPos.left+ptBefore.x-ptAfter.x, rcPos.top+ptBefore.y-ptAfter.y, 0,0, SWP_NOSIZE|SWP_NOZORDER);
		//}
		//if (bNeedCorrect && (rcBefore.left != rcAfter.left || rcBefore.top != rcAfter.top))
		//{
		//	m_FixPosAfterStyle = GetTickCount();
		//	mrc_FixPosAfterStyle = rcAfter;
		//	//DWORD nFlags = SWP_NOSIZE|SWP_NOZORDER;
		//	/////nFlags |= SWP_DRAWFRAME;
		//	//SetWindowPos(ghWnd, NULL, rcAfter.left, rcAfter.top, 0,0, nFlags);
		//}
		//else
		//{
		//	m_FixPosAfterStyle = 0;
		//}
		mb_IgnoreSizeChange = false;

		//if (changeFromWindowMode == wmNotChanging)
		//{
		//	//TODO: Поменять на FALSE
		//	ReSize(TRUE);
		//}
	}

	if (IsWindowVisible(ghWnd))
	{
		// Refresh JIC
		RedrawFrame();
		// Status bar and others
		Invalidate(NULL, TRUE);
	}

	if (changeFromWindowMode == wmNotChanging)
	{
		UpdateWindowRgn();
		ReSize(FALSE);
	}

	//OnTransparent();
}

void CConEmuMain::OnGlobalSettingsChanged()
{
	gpConEmu->UpdateGuiInfoMapping();

	// и отослать в серверы
	CVConGroup::OnUpdateGuiInfoMapping(&m_GuiInfo);
	
	// И фары тоже уведомить
	gpConEmu->UpdateFarSettings();
}

void CConEmuMain::OnPanelViewSettingsChanged(BOOL abSendChanges/*=TRUE*/)
{
	// UpdateGuiInfoMapping будет вызван после завершения создания окна
	if (!InCreateWindow())
	{
		UpdateGuiInfoMapping();
	}
	
	// Заполнить цвета gpSet->ThSet.crPalette[16], gpSet->ThSet.crFadePalette[16]
	COLORREF *pcrNormal = gpSet->GetColors(-1, FALSE);
	COLORREF *pcrFade = gpSet->GetColors(-1, TRUE);

	for(int i=0; i<16; i++)
	{
		// через FOR чтобы с BitMask не наколоться
		gpSet->ThSet.crPalette[i] = (pcrNormal[i]) & 0xFFFFFF;
		gpSet->ThSet.crFadePalette[i] = (pcrFade[i]) & 0xFFFFFF;
	}

	gpSet->ThSet.nFontQuality = gpSetCls->FontQuality();
	
	// Параметры основного шрифта ConEmu
	FillConEmuMainFont(&gpSet->ThSet.MainFont);
	//lstrcpy(gpSet->ThSet.MainFont.sFontName, gpSet->FontFaceName());
	//gpSet->ThSet.MainFont.nFontHeight = gpSetCls->FontHeight();
	//gpSet->ThSet.MainFont.nFontWidth = gpSetCls->FontWidth();
	//gpSet->ThSet.MainFont.nFontCellWidth = gpSet->FontSizeX3 ? gpSet->FontSizeX3 : gpSetCls->FontWidth();
	//gpSet->ThSet.MainFont.nFontQuality = gpSet->FontQuality();
	//gpSet->ThSet.MainFont.nFontCharSet = gpSet->FontCharSet();
	//gpSet->ThSet.MainFont.Bold = gpSet->FontBold();
	//gpSet->ThSet.MainFont.Italic = gpSet->FontItalic();
	//lstrcpy(gpSet->ThSet.MainFont.sBorderFontName, gpSet->BorderFontFaceName());
	//gpSet->ThSet.MainFont.nBorderFontWidth = gpSet->BorderFontWidth();
	
	// Применить в мэппинг (используется в "Panel View" и "Background"-плагинах
	bool lbChanged = gpSetCls->m_ThSetMap.SetFrom(&gpSet->ThSet);

	if (abSendChanges && lbChanged)
	{
		// и отослать заинтересованным
		CVConGroup::OnPanelViewSettingsChanged();
	}
}

void CConEmuMain::OnTaskbarCreated()
{
	Icon.OnTaskbarCreated();

	// Т.к. создан TaskBar - нужно (пере)создать интерфейсы
	Taskbar_Release();
	Taskbar_Init();

	// И передернуть все TaskBarGhost
	CVConGroup::OnTaskbarCreated();

	// Dummy
	if (mp_DragDrop) mp_DragDrop->OnTaskbarCreated();
}

void CConEmuMain::OnTaskbarButtonCreated()
{
	// May be not initialized yet
	Taskbar_Init();

	if (!gpSet->isWindowOnTaskBar())
	{
		DWORD styleEx = GetWindowLong(ghWnd, GWL_EXSTYLE);
		if (styleEx & WS_EX_APPWINDOW)
		{
			styleEx &= ~WS_EX_APPWINDOW;
			SetWindowStyleEx(styleEx);
		}

		// Если оно вдруг таки показано на таскбаре
		Taskbar_DeleteTabXP(ghWnd);
	}
	else
	{
		if (gpConEmu->mb_IsUacAdmin)
		{
			// Bug in Win7? Sometimes after startup "As Admin" sheild does not appeears.
			SetKillTimer(true, TIMER_ADMSHIELD_ID, TIMER_ADMSHIELD_ELAPSE);
		}
	}
}

void CConEmuMain::OnDefaultTermChanged()
{
	if (!this || !mp_DefTrm)
		return;

	mp_DefTrm->OnHookedListChanged();
}

void CConEmuMain::OnTaskbarSettingsChanged()
{
	CVConGroup::OnTaskbarSettingsChanged();

	if (!gpSet->isTabsOnTaskBar())
		OnAllGhostClosed();

	DWORD styleEx = GetWindowLong(ghWnd, GWL_EXSTYLE);
	if (!gpSet->isWindowOnTaskBar())
	{
		if (styleEx & WS_EX_APPWINDOW)
		{
			styleEx &= ~WS_EX_APPWINDOW;
			SetWindowStyleEx(styleEx);
			// Если оно вдруг таки показано на таскбаре
			Taskbar_DeleteTabXP(ghWnd);
		}
	}
	else
	{
		if (!(styleEx & WS_EX_APPWINDOW))
		{
			styleEx |= WS_EX_APPWINDOW;
			SetWindowStyleEx(styleEx);
			// Показать на таскбаре
			Taskbar_AddTabXP(ghWnd);
		}
	}

	apiSetForegroundWindow(ghOpWnd ? ghOpWnd : ghWnd);
}

void CConEmuMain::DoFullScreen()
{
	if (mp_Inside)
	{
		_ASSERTE(FALSE && "Must not change mode in the Inside mode");
		return;
	}

	ConEmuWindowMode wm = GetWindowMode();

	if (wm != wmFullScreen)
		gpConEmu->SetWindowMode(wmFullScreen);
	else if (gpSet->isDesktopMode && (wm != wmNormal))
		gpConEmu->SetWindowMode(wmNormal);
	else
		gpConEmu->SetWindowMode(gpConEmu->isWndNotFSMaximized ? wmMaximized : wmNormal);
}

bool CConEmuMain::DoMaximizeWidthHeight(bool bWidth, bool bHeight)
{
	RECT rcWnd, rcNewWnd;
	GetWindowRect(ghWnd, &rcWnd);
	rcNewWnd = rcWnd;
	MONITORINFO mon = {sizeof(MONITORINFO)};

	if (bWidth)
	{
		// найти новую левую границу
		POINT pt = {rcWnd.left,((rcWnd.top+rcWnd.bottom)>>2)};
		HMONITOR hMonLeft = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

		if (!GetMonitorInfo(hMonLeft, &mon))
			return false;

		rcNewWnd.left = mon.rcWork.left;
		// найти новую правую границу
		pt.x = rcWnd.right;
		HMONITOR hMonRight = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

		if (hMonRight != hMonLeft)
			if (!GetMonitorInfo(hMonRight, &mon))
				return false;

		rcNewWnd.right = mon.rcWork.right;

		//// Скорректировать границы на ширину рамки
		//RECT rcFrame = CalcMargins(CEM_FRAMECAPTION);
		//rcNewWnd.left -= rcFrame.left;
		//rcNewWnd.right += rcFrame.right;
	}
	
	if (bHeight)
	{
		// найти новую верхнюю границу
		POINT pt = {((rcWnd.left+rcWnd.right)>>2),rcWnd.top};
		HMONITOR hMonTop = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

		if (!GetMonitorInfo(hMonTop, &mon))
			return false;

		rcNewWnd.top = mon.rcWork.top;
		// найти новую нижнюю границу
		pt.y = rcWnd.bottom;
		HMONITOR hMonBottom = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

		if (hMonBottom != hMonTop)
			if (!GetMonitorInfo(hMonBottom, &mon))
				return false;

		rcNewWnd.bottom = mon.rcWork.bottom;

		//// Скорректировать границы на ширину рамки
		//RECT rcFrame = CalcMargins(CEM_FRAMECAPTION);
		//rcNewWnd.top -= rcFrame.bottom; // т.к. в top учтена высота заголовка
		//rcNewWnd.bottom += rcFrame.bottom;
	}

	// Двигаем окошко
	if (rcNewWnd.left != rcWnd.left || rcNewWnd.right != rcWnd.right || rcNewWnd.top != rcWnd.top || rcNewWnd.bottom != rcWnd.bottom)
		MOVEWINDOW(ghWnd, rcNewWnd.left, rcNewWnd.top, rcNewWnd.right-rcNewWnd.left, rcNewWnd.bottom-rcNewWnd.top, 1);

	return true;
}

void CConEmuMain::DoMaximizeRestore()
{
	// abPosted - removed, all calls was as TRUE

	if (mp_Inside)
	{
		_ASSERTE(FALSE && "Must not change mode in the Inside mode");
		return;
	}

	// -- this will be done in SetWindowMode
	//StoreNormalRect(NULL); // Сама разберется, надо/не надо

	ConEmuWindowMode wm = GetWindowMode();

	gpConEmu->SetWindowMode((wm != wmMaximized) ? wmMaximized : wmNormal);
}

bool CConEmuMain::InCreateWindow()
{
	return mb_InCreateWindow;
}

bool CConEmuMain::InQuakeAnimation()
{
	return (mn_QuakePercent != 0);
}

BOOL CConEmuMain::EnumWindowsOverQuake(HWND hWnd, LPARAM lpData)
{
	DWORD nStyle = GetWindowLong(hWnd, GWL_STYLE);
	if (!(nStyle & WS_VISIBLE))
		return TRUE; // Не видимые окна не интересуют
	if (nStyle & WS_CHILD)
		return TRUE; // Внезапно "дочерние" - тоже
	DWORD nStyleEx = GetWindowLong(hWnd, GWL_EXSTYLE);
	if ((nStyleEx & WS_EX_TOOLWINDOW) != 0)
		return TRUE; // Палитры/тулбары - пропускаем

	DWORD nPID;
	GetWindowThreadProcessId(hWnd, &nPID);
	if (nPID == GetCurrentProcessId())
	{
		// Все, раз дошли до нашего окна - значит всех кто сверху уже перебрали
		// Но hWnd может быть нашим диалогом, так что прекращаем только на ghWnd
		return (hWnd != ghWnd);
	}

	WindowsOverQuake* pOur = (WindowsOverQuake*)lpData;
	RECT rcWnd = {}, rcOver = {};

	if (GetWindowRect(hWnd, &rcWnd))
	{
		if (IntersectRect(&rcOver, &pOur->rcWnd, &rcWnd))
		{
			// Окно лежит поверх нашего
			if (memcmp(&rcOver, pOur, sizeof(rcOver)) == 0)
			{
				pOur->iRc = NULLREGION;
				return FALSE; // Окно полностью скрыто
			}

			// Начинаются расчеты
			HRGN hWndRgn = CreateRectRgn(rcWnd.left, rcWnd.top, rcWnd.right, rcWnd.bottom);
			pOur->iRc = CombineRgn(pOur->hRgn, pOur->hRgn, hWndRgn, RGN_DIFF);
			DeleteObject(hWndRgn);

			_ASSERTE(pOur->iRc != ERROR);
			if (pOur->iRc == NULLREGION)
			{
				// Окно полностью скрыто
				return FALSE;
			}
		}
	}

	return TRUE;
}

UINT CConEmuMain::IsQuakeVisible()
{
	if (!IsWindowVisible(ghWnd))
		return 0;
	if (isIconic())
		return 0;

	RECT rcWnd = {}; GetWindowRect(ghWnd, &rcWnd);
	if (IsRectEmpty(&rcWnd))
	{
		_ASSERTE(FALSE && "rcWnd must not be empty");
		return 0;
	}

	int nFullSq = (rcWnd.right - rcWnd.left) * (rcWnd.bottom - rcWnd.top);
	if (nFullSq <= 0)
	{
		_ASSERTE(nFullSq > 0);
		return 0;
	}

	WindowsOverQuake rcFree = {rcWnd, NULL, SIMPLEREGION};
	rcFree.hRgn = CreateRectRgn(rcWnd.left, rcWnd.top, rcWnd.right, rcWnd.bottom);
	EnumWindows(EnumWindowsOverQuake, (LPARAM)&rcFree);

	UINT  nVisiblePart = 100; // в процентах
	
	// Если "Видимая область" окна стала менее VisibleLimit(%) - считаем что окно стало "не видимым"
	if (rcFree.iRc == NULLREGION)
	{
		nVisiblePart = 0;
	}
	else
	{
		int nLeftSq = 0; //(rcFree.right - rcFree.left) * (rcFree.bottom - rcFree.top);
		DWORD cbSize = GetRegionData(rcFree.hRgn, 0, NULL);
		if (cbSize > sizeof(PRGNDATA))
		{
			LPRGNDATA lpRgnData = (LPRGNDATA)calloc(cbSize,1);
			lpRgnData->rdh.dwSize = sizeof(lpRgnData->rdh);
			cbSize = GetRegionData(rcFree.hRgn, cbSize, lpRgnData);
			if (cbSize && (lpRgnData->rdh.iType == RDH_RECTANGLES))
			{
				LPRECT pRc = (LPRECT)lpRgnData->Buffer;
				for (DWORD i = 0; i < lpRgnData->rdh.nCount; i++)
				{
					nLeftSq += (pRc[i].right - pRc[i].left) * (pRc[i].bottom - pRc[i].top);
				}
			}
			else
			{
				_ASSERTE(lpRgnData->rdh.iType == RDH_RECTANGLES);
			}
			free(lpRgnData);

			// Видимость в процентах
			nVisiblePart = (nLeftSq * 100 / nFullSq);
			if (nVisiblePart > 100)
			{
				_ASSERTE(nVisiblePart <= 100);
				nVisiblePart = 100;
			}
			else if (nVisiblePart < 0)
			{
				_ASSERTE(nVisiblePart >= 0);
				nVisiblePart = 0;
			}
		}
		else
		{
			_ASSERTE(cbSize > sizeof(PRGNDATA));
		}
	}

	DeleteObject(rcFree.hRgn);

	return nVisiblePart;
}

bool CConEmuMain::InMinimizing()
{
	if (mb_InShowMinimized)
		return true;
	if (mp_Menu && mp_Menu->GetInScMinimize())
		return true;
	return false;
}

void CConEmuMain::DoMinimizeRestore(SingleInstanceShowHideType ShowHideType /*= sih_None*/)
{
	if (mp_Inside)
		return;

	static bool bInFunction = false;
	if (bInFunction)
		return;

	HWND hCurForeground = GetForegroundWindow();
	bool bIsForeground = CVConGroup::isOurWindow(hCurForeground);
	bool bIsIconic = isIconic();
	BOOL bVis = IsWindowVisible(ghWnd);

	wchar_t szInfo[120];
	_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"DoMinimizeRestore(%i) Fore=x%08X Our=%u Iconic=%u Vis=%u",
		ShowHideType, (DWORD)hCurForeground, bIsForeground, bIsIconic, bVis);
	LogFocusInfo(szInfo);

	if (ShowHideType == sih_QuakeShowHide)
	{
		if (bIsIconic)
		{
			if (mn_LastQuakeShowHide && gpSet->isMinimizeOnLoseFocus())
			{
				DWORD nDelay = GetTickCount() - mn_LastQuakeShowHide;
				if (nDelay < QUAKE_FOCUS_CHECK_TIMER_DELAY)
				{
					_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"DoMinimizeRestore skipped, delay was %u ms", nDelay);
					LogFocusInfo(szInfo);
					return;
				}
			}

			ShowHideType = sih_Show;
		}
		else
		{
			ShowHideType = sih_AutoHide; // дальше разберемся
		}
	}

	if (ShowHideType == sih_AutoHide)
	{
		if (gpSet->isQuakeStyle)
		{
			bool bMin2TSA = gpSet->isMinToTray();

			if (bMin2TSA)
				ShowHideType = sih_HideTSA;
			else
				ShowHideType = sih_Minimize;
		}
		else
		{
			ShowHideType = sih_Minimize;
		}
	}

	if ((ShowHideType == sih_HideTSA) || (ShowHideType == sih_Minimize))
	{
		// Some trick for clicks on taskbar and "Hide on focus lose"
		mn_ForceTimerCheckLoseFocus = 0;

		if (bIsIconic || !bVis)
		{
			return;
		}
	}

	bInFunction = true;

	// 1. Функция вызывается при нажатии глобально регистрируемого хоткея
	//    Win+C  -->  ShowHideType = sih_None
	// 2. При вызове из другого приложения 
	//    аргумент /single  --> ShowHideType = sih_Show
	// 3. При вызове из дрегого приложения
	//    аргумент /showhide     --> ShowHideType = sih_ShowMinimize
	//    аргумент /showhideTSA  --> ShowHideType = sih_ShowHideTSA
	SingleInstanceShowHideType cmd = sih_None;

	if (ShowHideType == sih_None)
	{
		// По настройкам
		ShowHideType = gpSet->isMinToTray() ? sih_ShowHideTSA : sih_ShowMinimize;
	}

	// Go
	if (ShowHideType == sih_Show)
	{
		cmd = sih_Show;
	}
	else if ((ShowHideType == sih_ShowMinimize)
		|| (ShowHideType == sih_Minimize)
		|| (ShowHideType == sih_ShowHideTSA)
		|| (ShowHideType == sih_HideTSA))
	{
		if ((bVis && (bIsForeground || gpSet->isAlwaysOnTop) && !bIsIconic)
			|| (ShowHideType == sih_HideTSA) || (ShowHideType == sih_Minimize))
		{
			// если видимо - спрятать
			cmd = (ShowHideType == sih_HideTSA) ? sih_ShowHideTSA : ShowHideType;
		}
		else
		{
			// Иначе - показать
			cmd = sih_Show;
		}
	}
	else
	{
		_ASSERTE(ShowHideType == sih_SetForeground);
		// Иначе - показываем (в зависимости от текущей видимости)
		if (IsWindowVisible(ghWnd) && (bIsForeground || !bIsIconic)) // (!bIsIconic) - окошко развернуто, надо свернуть
			cmd = sih_SetForeground;
		else
			cmd = sih_Show;
	}

	// Поехали
	int nQuakeMin = 1;
	int nQuakeShift = 10;
	//int nQuakeDelay = gpSet->nQuakeAnimation / nQuakeShift; // 20;
	bool bUseQuakeAnimation = false; //, bNeedHideTaskIcon = false;
	bool bMinToTray = gpSet->isMinToTray();
	if (gpSet->isQuakeStyle != 0)
	{
		// Если есть дочерние GUI окна - в них могут быть глюки с отрисовкой
		if ((gpSet->nQuakeAnimation > 0) && !CVConGroup::isChildWindowVisible())
		{
			bUseQuakeAnimation = true;
		}
	}

	DEBUGTEST(static DWORD nLastQuakeHide);

	// Logging
	if (RELEASEDEBUGTEST((gpSetCls->isAdvLogging>0),true))
	{
		switch (cmd)
		{
		case sih_None:
			LogFocusInfo(L"DoMinimizeRestore(sih_None)"); break;
		case sih_ShowMinimize:
			LogFocusInfo(L"DoMinimizeRestore(sih_ShowMinimize)"); break;
		case sih_ShowHideTSA:
			LogFocusInfo(L"DoMinimizeRestore(sih_ShowHideTSA)"); break;
		case sih_Show:
			LogFocusInfo(L"DoMinimizeRestore(sih_Show)"); break;
		case sih_SetForeground:
			LogFocusInfo(L"DoMinimizeRestore(sih_SetForeground)"); break;
		case sih_HideTSA:
			LogFocusInfo(L"DoMinimizeRestore(sih_HideTSA)"); break;
		case sih_Minimize:
			LogFocusInfo(L"DoMinimizeRestore(sih_Minimize)"); break;
		case sih_AutoHide:
			LogFocusInfo(L"DoMinimizeRestore(sih_AutoHide)"); break;
		case sih_QuakeShowHide:
			LogFocusInfo(L"DoMinimizeRestore(sih_QuakeShowHide)"); break;
		default:
			LogFocusInfo(L"DoMinimizeRestore(Unknown cmd)");
		}
	}

	if (cmd == sih_SetForeground)
	{
		apiSetForegroundWindow(ghWnd);
	}
	else if (cmd == sih_Show)
	{
		HWND hWndFore = hCurForeground;
		DEBUGTEST(DWORD nHideShowDelay = GetTickCount() - nLastQuakeHide);
		// Если активация идет кликом по кнопке на таскбаре (Quake без скрытия) то на WinXP
		// может быть глюк - двойная отрисовка раскрытия (WM_ACTIVATE, WM_SYSCOMMAND)
		bool bNoQuakeAnimation = false;

		bool bPrevInRestore = false;
		if (bIsIconic)
			bPrevInRestore = mp_Menu->SetRestoreFromMinimized(true);

		//apiSetForegroundWindow(ghWnd);

		if (gpSet->isFadeInactive)
		{
			CVConGuard VCon;
			if (GetActiveVCon(&VCon) >= 0)
			{
				// Чтобы при "Fade inactive" все сразу красиво отрисовалось
				int iCur = mn_QuakePercent; mn_QuakePercent = 100;
				VCon->Update();
				mn_QuakePercent = iCur;
			}
		}

		DEBUGTEST(bool bWasQuakeIconic = isQuakeMinimized);

		if (gpSet->isQuakeStyle /*&& !isMouseOverFrame()*/)
		{
			// Для Quake-style необходимо СНАЧАЛА сделать окно "невидимым" перед его разворачиваем или активацией
			if ((hWndFore == ghWnd) && !bIsIconic && bVis)
			{
				bNoQuakeAnimation = true;
				mn_QuakePercent = 0;
			}
			else
			{
				StopForceShowFrame();
				mn_QuakePercent = nQuakeMin;
				UpdateWindowRgn();
			}
		}
		else
			mn_QuakePercent = 0;

		if (Icon.isWindowInTray() || !IsWindowVisible(ghWnd))
		{
			mb_LockWindowRgn = TRUE;
			BOOL b = mb_LockShowWindow; mb_LockShowWindow = bUseQuakeAnimation;
			Icon.RestoreWindowFromTray(false, bUseQuakeAnimation);
			mb_LockShowWindow = b;
			mb_LockWindowRgn = FALSE;
			bIsIconic = isIconic();
			//bNeedHideTaskIcon = bUseQuakeAnimation;
		}
		
		// Здесь - интересует реальный IsIconic. Для isQuakeStyle может быть фейк
		if (::IsIconic(ghWnd))
		{
			DEBUGTEST(WINDOWPLACEMENT wpl = {sizeof(wpl)}; GetWindowPlacement(ghWnd, &wpl););
			bool b = mp_Menu->SetPassSysCommand(true);
			SendMessage(ghWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
			mp_Menu->SetPassSysCommand(b);
		}

		// Страховка, коррекция позиции для Quake
		if (gpSet->isQuakeStyle)
		{
			RECT rcWnd = GetDefaultRect();
			SetWindowPos(ghWnd, NULL, rcWnd.left, rcWnd.top, 0,0, SWP_NOZORDER|SWP_NOSIZE);
		}

		isQuakeMinimized = false; // теперь можно сбросить

		apiSetForegroundWindow(ghWnd);

		if (gpSet->isQuakeStyle /*&& !isMouseOverFrame()*/)
		{
			if (!bNoQuakeAnimation)
			{
				if (bUseQuakeAnimation)
				{
					DWORD nAnimationMS = gpSet->nQuakeAnimation; // (100 / nQuakeShift) * nQuakeDelay * 2;
					_ASSERTE(nAnimationMS > 0);
					//MinMax(nAnimationMS, CONEMU_ANIMATE_DURATION, CONEMU_ANIMATE_DURATION_MAX);
					MinMax(nAnimationMS, QUAKEANIMATION_MAX);

					DWORD nFlags = AW_SLIDE|AW_VER_POSITIVE|AW_ACTIVATE;

					// Need to hide window
					DEBUGTEST(RECT rcNow; ::GetWindowRect(ghWnd, &rcNow));
					RECT rcPlace = GetDefaultRect();
					if (::IsWindowVisible(ghWnd))
					{
						apiShowWindow(ghWnd, SW_HIDE);
					}
					// and place it "in place"
					int nWidth = rcPlace.right-rcPlace.left, nHeight = rcPlace.bottom-rcPlace.top;
					SetWindowPos(ghWnd, NULL, rcPlace.left, rcPlace.top, nWidth, nHeight, SWP_NOZORDER);

					mn_QuakePercent = 100;
					UpdateWindowRgn();

					// to enable animation
					AnimateWindow(ghWnd, nAnimationMS, nFlags);
				}
				else
				{
					DWORD nStartTick = GetTickCount();

					StopForceShowFrame();

					// Begin of animation
					_ASSERTE(mn_QuakePercent==0 || mn_QuakePercent==nQuakeMin);

					if (gpSet->nQuakeAnimation > 0)
					{
						int nQuakeStepDelay = gpSet->nQuakeAnimation / nQuakeShift; // 20;

						while (mn_QuakePercent < 100)
						{
							mn_QuakePercent += nQuakeShift;
							UpdateWindowRgn();
							RedrawWindow(ghWnd, NULL, NULL, RDW_UPDATENOW);

							DWORD nCurTick = GetTickCount();
							int nQuakeDelay = nQuakeStepDelay - ((int)(nCurTick - nStartTick));
							if (nQuakeDelay > 0)
							{
								Sleep(nQuakeDelay);
							}

							nStartTick = GetTickCount();
						}
					}
				}
			}
			if (mn_QuakePercent != 100)
			{
				mn_QuakePercent = 100;
				UpdateWindowRgn();
			}
		}
		mn_QuakePercent = 0; // 0 - отключен

		CVConGuard VCon;
		if (GetActiveVCon(&VCon) >= 0)
		{
			VCon->PostRestoreChildFocus();
			//gpConEmu->OnFocus(ghWnd, WM_ACTIVATEAPP, TRUE, 0, L"From DoMinimizeRestore(sih_Show)");
		}

		if (bIsIconic)
			mp_Menu->SetRestoreFromMinimized(bPrevInRestore);
	}
	else
	{
		// Минимизация
		_ASSERTE(((cmd == sih_ShowHideTSA) || (cmd == sih_ShowMinimize) || (cmd == sih_Minimize)) && "cmd must be determined!");

		isQuakeMinimized = true; // сразу включим, чтобы не забыть. Используется только при gpSet->isQuakeStyle

		if (bVis && !bIsIconic)
		{
			//UpdateIdealRect();
			StoreIdealRect();
		}

		if ((ghLastForegroundWindow != ghWnd) && (ghLastForegroundWindow != ghOpWnd))
		{
			// Фокус не там где надо - например, в дочернем GUI приложении
			if (CVConGroup::isOurWindow(ghLastForegroundWindow))
			{
				setFocus();
				apiSetForegroundWindow(ghWnd);
			}
			//TODO: Тут наверное нужно выйти и позвать Post для DoMinimizeRestore(cmd)
			//TODO: иначе при сворачивании не активируется "следующее" окно, фокус ввода
			//TODO: остается в дочернем Notepad (ввод текста идет в него)
		}

		if (gpSet->isQuakeStyle /*&& !isMouseOverFrame()*/)
		{
			if (bUseQuakeAnimation)
			{
				DWORD nAnimationMS = gpSet->nQuakeAnimation; // (100 / nQuakeShift) * nQuakeDelay * 2;
				_ASSERTE(nAnimationMS > 0);
				//MinMax(nAnimationMS, CONEMU_ANIMATE_DURATION, CONEMU_ANIMATE_DURATION_MAX);
				MinMax(nAnimationMS, QUAKEANIMATION_MAX);

				DWORD nFlags = AW_SLIDE|AW_VER_NEGATIVE|AW_HIDE;

				DEBUGTEST(BOOL bVs1 = ::IsWindowVisible(ghWnd));
				DEBUGTEST(RECT rc1; ::GetWindowRect(ghWnd, &rc1));
				AnimateWindow(ghWnd, nAnimationMS, nFlags);
				DEBUGTEST(BOOL bVs2 = ::IsWindowVisible(ghWnd));
				DEBUGTEST(RECT rc2; ::GetWindowRect(ghWnd, &rc2));
				DEBUGTEST(bVs1 = bVs2);

				if (!bMinToTray && (cmd != sih_ShowHideTSA))
				{
					// Если в трей не скрываем - то окошко нужно "вернуть на таскбар"
					StopForceShowFrame();
					mn_QuakePercent = 1;
					UpdateWindowRgn();
					apiShowWindow(ghWnd, SW_SHOWNOACTIVATE);
				}
				// Если на таскбаре отображаются "табы",
				// то после AnimateWindow(AW_HIDE) в Win8 иконка с таскбара не убирается
				else if (gpSet->isTabsOnTaskBar())
				{
					if (gpSet->isWindowOnTaskBar())
					{
						StopForceShowFrame();
						mn_QuakePercent = 1;
						UpdateWindowRgn();
						apiShowWindow(ghWnd, SW_SHOWNOACTIVATE);
						apiShowWindow(ghWnd, SW_HIDE);
					}
				}

			}
			else
			{
				DWORD nStartTick = GetTickCount();
				mn_QuakePercent = (gpSet->nQuakeAnimation > 0) ? (100 + nQuakeMin - nQuakeShift) : nQuakeMin;
				StopForceShowFrame();

				if (gpSet->nQuakeAnimation > 0)
				{
					int nQuakeStepDelay = gpSet->nQuakeAnimation / nQuakeShift; // 20;

					while (mn_QuakePercent > 0)
					{
						UpdateWindowRgn();
						RedrawWindow(ghWnd, NULL, NULL, RDW_UPDATENOW);
						//Sleep(nQuakeDelay);
						mn_QuakePercent -= nQuakeShift;

						DWORD nCurTick = GetTickCount();
						int nQuakeDelay = nQuakeStepDelay - ((int)(nCurTick - nStartTick));
						if (nQuakeDelay > 0)
						{
							Sleep(nQuakeDelay);
						}

						nStartTick = GetTickCount();
					}
				}
				else
				{
					UpdateWindowRgn();
				}
			}
		}
		mn_QuakePercent = 0; // 0 - отключен


		if (cmd == sih_ShowHideTSA)
		{
			mb_LockWindowRgn = TRUE;
			// Явно попросили в TSA спрятать
			Icon.HideWindowToTray();
			mb_LockWindowRgn = FALSE;
		}
		else if ((cmd == sih_ShowMinimize) || (ShowHideType == sih_Minimize))
		{
			if (gpSet->isQuakeStyle)
			{
				// Раз попали сюда - значит скрывать иконку с таскбара не хотят. Нужен изврат...

				// Найти окно "под" нами
				HWND hNext = NULL;
				if (hCurForeground && !bIsForeground)
				{
					// Вернуть фокус туда где он был до наших ексерсизов
					hNext = hCurForeground;
				}
				else
				{
					while ((hNext = FindWindowEx(NULL, hNext, NULL, NULL)) != NULL)
					{
						if (::IsWindowVisible(hNext))
						{
							// Доп условия, аналог Alt-Tab?
							DWORD nStylesEx = GetWindowLong(hNext, GWL_EXSTYLE);
							DEBUGTEST(DWORD nStyles = GetWindowLong(hNext, GWL_STYLE));
							if (!(nStylesEx & WS_EX_TOOLWINDOW))
							{
								break;
							}
						}
					}
				}
				// И задвинуть в зад
				SetWindowPos(ghWnd, NULL, -32000, -32000, 0,0, SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);
				apiSetForegroundWindow(hNext ? hNext : GetDesktopWindow());
			}
			else
			{
				// SC_MINIMIZE сам обработает (gpSet->isMinToTray || gpConEmu->ForceMinimizeToTray)
				SendMessage(ghWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
			}
		}

		DEBUGTEST(nLastQuakeHide = GetTickCount());
	}

	mn_LastQuakeShowHide = GetTickCount();
	LogFocusInfo(L"DoMinimizeRestore finished");

	bInFunction = false;
 }

void CConEmuMain::DoForcedFullScreen(bool bSet /*= true*/)
{
	static bool bWasSetTopMost = false;

	// определить возможность перейти в текстовый FullScreen
	if (!bSet)
	{
		// Снять флаг "OnTop", вернуть нормальные приоритеты процессам
		if (bWasSetTopMost)
		{
			gpSet->isAlwaysOnTop = false;
			OnAlwaysOnTop();
			bWasSetTopMost = false;
		}
		return;
	}

	if (IsHwFullScreenAvailable())
	{

		CVConGuard VCon;
		if (CVConGroup::GetActiveVCon(&VCon) >= 0)
		{
			//BOOL WINAPI SetConsoleDisplayMode(HANDLE hConsoleOutput, DWORD dwFlags, PCOORD lpNewScreenBufferDimensions);
			//if (!isIconic())
			//	SendMessage(ghWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);

			::ShowWindow(ghWnd, SW_SHOWMINNOACTIVE);

			bool bFRc = VCon->RCon()->SetFullScreen();

			if (bFRc)
				return;

			SendMessage(ghWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
		}
	}

	if (gpSet->isDesktopMode)
	{
		DisplayLastError(L"Can't set FullScreen in DesktopMode", -1);
		return;
	}

	TODO("Поднять приоритет процессов");
	
	// Установить AlwaysOnTop
	if (!gpSet->isAlwaysOnTop)
	{
		gpSet->isAlwaysOnTop = true;
		OnAlwaysOnTop();
		bWasSetTopMost = true;
	}

	if (!isMeForeground())
	{
		if (isIconic())
		{
			if (Icon.isWindowInTray() || !IsWindowVisible(ghWnd))
				Icon.RestoreWindowFromTray();
			else
				SendMessage(ghWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
		}
		//else
		//{
		//	//SendMessage(ghWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
		//	SendMessage(ghWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
		//}
	}

	SetWindowMode(wmFullScreen);

	if (!isMeForeground())
	{
		SwitchToThisWindow(ghWnd, FALSE);
	}
}

void CConEmuMain::OnSwitchGuiFocus(SwitchGuiFocusOp FocusOp)
{
	if (!((FocusOp > sgf_None) && (FocusOp < sgf_Last)))
	{
		Assert((FocusOp > sgf_None) && (FocusOp < sgf_Last));
		return;
	}

	CVConGuard VCon;
	//HWND hSet = ghWnd;

	if ((GetActiveVCon(&VCon) >= 0) && VCon->RCon()->GuiWnd())
	{
		bool bSetChild = (FocusOp == sgf_FocusChild);

		if (FocusOp == sgf_FocusSwitch)
		{
			DWORD nFocusPID = 0;
			HWND hGet = GetFocus();
			if (hGet)
			{
				GetWindowThreadProcessId(hGet, &nFocusPID);
			}

			if (nFocusPID == GetCurrentProcessId())
			{
				// Вернуть фокус в дочернее приложение
				bSetChild = true;
			}
		}

		if (bSetChild)
		{
			// Вернуть фокус в дочернее приложение
			VCon->RCon()->GuiWndFocusRestore();
			return;
		}
	}

	// Поставить фокус в ConEmu
	setFocus();
}

LRESULT CConEmuMain::OnKeyboard(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
#ifdef _DEBUG
	#if 0
	if (messg == WM_KEYDOWN || messg == WM_KEYUP)
	{
		if (wParam >= VK_F1 && wParam <= VK_F4)
		{
			if (messg == WM_KEYUP)
			{
				DWORD nFlags = 0;
				switch ((DWORD)wParam)
				{
				case VK_F1:
					nFlags = AW_SLIDE|AW_HIDE|AW_HOR_NEGATIVE|AW_VER_NEGATIVE;
					break;
				case VK_F2:
					nFlags = AW_SLIDE|AW_HOR_POSITIVE|AW_VER_POSITIVE;
					break;
				case VK_F3:
					nFlags = AW_BLEND|AW_HIDE|AW_CENTER;
					break;
				case VK_F4:
					nFlags = AW_BLEND|AW_CENTER;
					break;
				}
				BOOL b = AnimateWindow(ghWnd, 500, nFlags);
				DWORD nErr = GetLastError();
				wchar_t szErr[128];
				_wsprintf(szErr, SKIPLEN(countof(szErr)) L"AnimateWindow(%08x)=%u, Err=%u\n", nFlags, b, nErr);
				DEBUGSTRANIMATE(szErr);
			}
			return 0;
		}
	}
	#endif
#endif

	if (!m_GuiInfo.bGuiActive)
	{
		UpdateGuiInfoMappingActive(true);
	}

	if (mb_ImeMethodChanged)
	{
		if (messg == WM_SYSKEYDOWN || messg == WM_SYSKEYUP)
		{
			if (messg == WM_SYSKEYUP)
				mb_ImeMethodChanged = false;
			return 0;
		}
		else
		{
			mb_ImeMethodChanged = false;
		}
	}

	WORD bVK = (WORD)(wParam & 0xFF);
	//-- некорректно. сработало на LCtrl+LAlt
	//bool bAltGr = (messg == WM_KEYDOWN && bVK == VK_MENU);
	//_ASSERTE(!bAltGr);

	#ifdef _DEBUG
	wchar_t szDebug[255];
	_wsprintf(szDebug, SKIPLEN(countof(szDebug)) L"%s(VK=0x%02X, Scan=%i, lParam=0x%08X)\n",
	          (messg == WM_KEYDOWN) ? L"WM_KEYDOWN" :
	          (messg == WM_KEYUP) ? L"WM_KEYUP" :
	          (messg == WM_SYSKEYDOWN) ? L"WM_SYSKEYDOWN" :
	          (messg == WM_SYSKEYUP) ? L"WM_SYSKEYUP" :
	          L"<Unknown Message> ",
	          bVK, ((DWORD)lParam & 0xFF0000) >> 16, (DWORD)lParam);
	DEBUGSTRKEY(szDebug);
	#endif


	if (gpSetCls->isAdvLogging)
		CVConGroup::LogInput(messg, wParam, lParam);


#if 1
	// Works fine, but need to cache last pressed key, cause of need them in WM_KEYUP (send char to console)
	wchar_t szTranslatedChars[16] = {0};
	int nTranslatedChars = 0;
	MSG msgList[16] = {{hWnd,messg,wParam,lParam}};
	int iAddMsgList = 0, iDeadMsgIndex = 0;
	int iProcessDeadChars = 0;
	//static bool bWasDeadChar = false;
	//static LPARAM nDeadCharLParam = 0;
	MSG DeadCharMsg = {};

	if (messg == WM_KEYDOWN || messg == WM_SYSKEYDOWN)
	{
		_ASSERTE(sizeof(szTranslatedChars) == sizeof(m_TranslatedChars[0].szTranslatedChars));
		//BOOL lbDeadChar = FALSE;
		MSG msg, msg1;
		LPARAM lKey1ScanCode = (lParam & 0x00FF0000);
		DEBUGTEST(LPARAM lKey2ScanCode = 0);

		// 120722 - Spanish (for example) layout. Есть несколько клавиш для ввода
		// символов с разными Grave/Acute/Circumflex/Tilde/Diaeresis/Ring Above
		// Например, при вводе "~" и "a" система продуцирует следующую последовательность
		// WM_KEYDOWN, WM_DEADCHAR (~), WM_KEYUP, WM_KEYDOWN, WM_CHAR ("a" с "~"), WM_KEYUP
		// Причем только в том случае, если нет задержек в обработке этих сообщений.
		// При наличии задержки в обработке - получаем просто "a" без тильды.

		TODO("Проверить немецкую раскладку с '^'");

		while (nTranslatedChars < 15  // извлечь из буфера все последующие WM_CHAR & WM_SYSCHAR
		        && PeekMessage(&msg, 0,0,0, PM_NOREMOVE)
		      )
		{
			if (!(msg.message == WM_CHAR || msg.message == WM_SYSCHAR
					|| msg.message == WM_DEADCHAR || msg.message == WM_SYSDEADCHAR
					|| (iProcessDeadChars == 1 // был WM_DEADCHAR/WM_SYSDEADCHAR, ожидаем WM_KEYUP и WM_KEYDOWN
						&& ((msg.message == WM_KEYUP && (msg.lParam & 0x00FF0000) == lKey1ScanCode)
							|| (msg.message == WM_KEYDOWN /* следующая за DeadChar собственно буква */))
						)
					|| (iProcessDeadChars == 2 // был WM_DEADCHAR/WM_SYSDEADCHAR, теперь ожидаем WM_CHAR
						&& (msg.message == WM_CHAR || msg.message == WM_SYSCHAR)
						)
					))
				//|| (msg.lParam & 0xFF0000) != lKey1ScanCode /* совпадение скан-кода */) // 120722 - убрал
			{
				break;
			}

			if (GetMessage(&msg1, 0,0,0))  // убрать из буфера
			{
				ConEmuMsgLogger::Log(msg1, ConEmuMsgLogger::msgCommon);
				_ASSERTEL(msg1.message == msg.message && msg1.wParam == msg.wParam && msg1.lParam == msg.lParam);
				msgList[++iAddMsgList] = msg1;

				if (gpSetCls->isAdvLogging >= 4)
				{
					gpConEmu->LogMessage(msg1.hwnd, msg1.message, msg1.wParam, msg1.lParam);
				}

				if (msg.message == WM_CHAR || msg.message == WM_SYSCHAR)
				{
					szTranslatedChars[nTranslatedChars ++] = (wchar_t)msg1.wParam;
					if (iProcessDeadChars == 1)
					{
						_ASSERTEL(iProcessDeadChars == 2); // уже должно было быть выставлено в WM_KEYDOWN
						iProcessDeadChars = 2;
					}
				}
				else if (msg.message == WM_DEADCHAR || msg.message == WM_SYSDEADCHAR)
				{
					_ASSERTEL(iProcessDeadChars==0 && "Must be first entrance");
					//lbDeadChar = TRUE;
					iProcessDeadChars = 1;
					DeadCharMsg = msg;
				}
				else if (msg.message == WM_KEYDOWN)
				{
					if (iProcessDeadChars == 1)
					{
						iProcessDeadChars = 2;
						DEBUGTEST(lKey2ScanCode = (msg.lParam & 0x00FF0000));
						iDeadMsgIndex = iAddMsgList;
					}
				}

				// 120722 - Было DispatchMessage вместо TranslateMessage.
				// Требуется обработать сообщение
				TranslateMessage(&msg1);
				//DispatchMessage(&msg1);
			}
		}

		if (gpSetCls->isAdvLogging)
		{
			for (int i = 1; i <= iAddMsgList; i++)
			{
				CVConGroup::LogInput(msgList[i].message, msgList[i].wParam, msgList[i].lParam);
			}
		}

		if (iProcessDeadChars == 2)
		{
			_ASSERTEL(iDeadMsgIndex>0);
			_ASSERTEL(msgList[iDeadMsgIndex].message == messg);
			wParam = msgList[iDeadMsgIndex].wParam;
			lParam = msgList[iDeadMsgIndex].lParam;
			bVK = (WORD)(wParam & 0xFF);
		}

		//if (lbDeadChar)
		//{
		//	bWasDeadChar = true;
		//	nDeadCharLParam = lParam;
		//}
		//else
		//{
		//	bWasDeadChar = false;
		//}

		//if (lbDeadChar && nTranslatedChars)
		//{
		//	_ASSERTEL(FALSE && "Dead char does not produces szTranslatedChars");
		//	lbDeadChar = FALSE;
		//}

		memmove(m_TranslatedChars[bVK].szTranslatedChars, szTranslatedChars, sizeof(szTranslatedChars));
	}
	else
	{
		_ASSERTEL((messg == WM_KEYUP || messg == WM_SYSKEYUP) && "Unexpected msg");
		//if (messg == WM_KEYUP || messg == WM_SYSKEYUP)
		//{
		//	if (bWasDeadChar)
		//	{
		//		if ((nDeadCharLParam & 0xFF0000) == (lParam & 0xFF0000))
		//		{
		//			//DefWindowProc(hWnd, messg, wParam, lParam);
		//		}
		//		bWasDeadChar = false;
		//	}
		//}
		//else
		//{
		//	_ASSERTEL((messg == WM_KEYUP || messg == WM_SYSKEYUP) && "Unexpected msg");
		//}

		szTranslatedChars[0] = m_TranslatedChars[bVK].szTranslatedChars[0];
		szTranslatedChars[1] = 0;
		nTranslatedChars = szTranslatedChars[0] ? 1 : 0;
	}

#endif
#if 0
	/* Works, but has problems with dead chars */
	wchar_t szTranslatedChars[11] = {0};
	int nTranslatedChars = 0;

	if (!GetKeyboardState(m_KeybStates))
	{
		#ifdef _DEBUG
		DWORD dwErr = GetLastError();
		_ASSERTEL(FALSE);
		#endif
		static bool sbErrShown = false;

		if (!sbErrShown)
		{
			sbErrShown = true;
			DisplayLastError(L"GetKeyboardState failed!");
		}
	}
	else
	{
		HKL hkl = (HKL)GetActiveKeyboardLayout();
		UINT nVK = wParam & 0xFFFF;
		UINT nSC = ((DWORD)lParam & 0xFF0000) >> 16;
		WARNING("BUGBUG: похоже глючит в x64 на US-Dvorak");
		nTranslatedChars = ToUnicodeEx(nVK, nSC, m_KeybStates, szTranslatedChars, 10, 0, hkl);
		if (nTranslatedChars>0) szTranslatedChars[min(10,nTranslatedChars)] = 0; else szTranslatedChars[0] = 0;
	}

#endif
#if 0
	/* Invalid ? */
	wchar_t szTranslatedChars[16] = {0};

	if (!GetKeyboardState(m_KeybStates))
	{
#ifdef _DEBUG
		DWORD dwErr = GetLastError();
		_ASSERTEL(FALSE);
#endif
		static bool sbErrShown = false;

		if (!sbErrShown)
		{
			sbErrShown = true;
			DisplayLastError(L"GetKeyboardState failed!");
		}
	}

	//MSG smsg = {hWnd, messg, wParam, lParam};
	//TranslateMessage(&smsg);

	/*if (bVK == VK_SHIFT || bVK == VK_MENU || bVK == VK_CONTROL || bVK == VK_LWIN || bVK == VK_RWIN) {
		if (messg == WM_KEYDOWN || messg == WM_SYSKEYDOWN) {
			if (
		} else if (messg == WM_KEYUP || messg == WM_SYSKEYUP) {
		}
	}*/

	if (messg == WM_KEYDOWN || messg == WM_SYSKEYDOWN)
	{
		HKL hkl = (HKL)m_ActiveKeybLayout; //GetActiveKeyboardLayout();
		UINT nVK = wParam & 0xFFFF;
		UINT nSC = ((DWORD)lParam & 0xFF0000) >> 16;
		WARNING("BUGBUG: похоже глючит в x64 на US-Dvorak");
		int nTranslatedChars = ToUnicodeEx(nVK, nSC, m_KeybStates, szTranslatedChars, 15, 0, hkl);

		if (nTranslatedChars >= 0)
		{
			// 2 or more
			// Two or more characters were written to the buffer specified by pwszBuff.
			// The most common cause for this is that a dead-key character (accent or diacritic)
			// stored in the keyboard layout could not be combined with the specified virtual key
			// to form a single character. However, the buffer may contain more characters than the
			// return value specifies. When this happens, any extra characters are invalid and should be ignored.
			szTranslatedChars[min(15,nTranslatedChars)] = 0;
		}
		else if (nTranslatedChars == -1)
		{
			// The specified virtual key is a dead-key character (accent or diacritic).
			// This value is returned regardless of the keyboard layout, even if several
			// characters have been typed and are stored in the keyboard state. If possible,
			// even with Unicode keyboard layouts, the function has written a spacing version
			// of the dead-key character to the buffer specified by pwszBuff. For example, the
			// function writes the character SPACING ACUTE (0x00B4),
			// rather than the character NON_SPACING ACUTE (0x0301).
			szTranslatedChars[0] = 0;
		}
		else
		{
			// Invalid
			szTranslatedChars[0] = 0;
		}

		mn_LastPressedVK = bVK;
	}

#endif

	if (messg == WM_KEYDOWN && !mb_HotKeyRegistered)
		RegisterHotKeys(); // Win и прочее

	_ASSERTEL(messg != WM_CHAR && messg != WM_SYSCHAR);

	// Теперь обработаем некоторые "общие" хоткеи
	if (wParam == VK_ESCAPE)
	{
		if (mp_DragDrop->InDragDrop())
			return 0;

		static bool bEscPressed = false;

		if (messg != WM_KEYUP)
			bEscPressed = true;
		else if (!bEscPressed)
			return 0;
		else
			bEscPressed = false;
	}
	
	if ((wParam == VK_CAPITAL) || (wParam == VK_NUMLOCK) || (wParam == VK_SCROLL))
		mp_Status->OnKeyboardChanged();

	// Теперь - можно переслать в консоль
	CVConGuard VCon;
	if ((GetActiveVCon(&VCon) >= 0) && VCon->RCon())
	{
		_ASSERTEL(iProcessDeadChars==0 || iProcessDeadChars==1);
		VCon->RCon()->OnKeyboard(hWnd, messg, wParam, lParam, szTranslatedChars, iProcessDeadChars?&DeadCharMsg:NULL);
	}
	else if (((wParam & 0xFF) >= VK_WHEEL_FIRST) && ((wParam & 0xFF) <= VK_WHEEL_LAST))
	{
		// Такие коды с клавиатуры приходить не должны, а то для "мышки" ничего не останется
		_ASSERTEL(!(((wParam & 0xFF) >= VK_WHEEL_FIRST) && ((wParam & 0xFF) <= VK_WHEEL_LAST)));
	}
	else
	{
		// Если вдруг активной консоли нету (вообще?) но клавиши обработать все-равно нада
		// true - хоткей обработан
		if (!ProcessHotKeyMsg(messg, wParam, lParam, szTranslatedChars, NULL))
		{
			; // Keypress was not processed

			//// И напоследок
			//if (wParam == VK_ESCAPE)
			//{
			//	if (messg == WM_KEYUP)
			//	{
			//		DoMinimizeRestore(sih_Minimize);
			//	}
			//}
		}
	}

	return 0;
}

LRESULT CConEmuMain::OnKeyboardHook(DWORD VkMod)
{
	if (!VkMod)
		return 0;

	CVConGuard VCon;
	CRealConsole* pRCon = (GetActiveVCon(&VCon) >= 0) ? VCon->RCon() : NULL;

	const ConEmuHotKey* pHotKey = ProcessHotKey(VkMod, true/*bKeyDown*/, NULL, pRCon);

	if (pHotKey == ConEmuSkipHotKey)
	{
		// Если функция срабатывает на отпускание
		pHotKey = ProcessHotKey(VkMod, false/*bKeyDown*/, NULL, pRCon);
	}

	return 0;
}

void CConEmuMain::OnConsoleKey(WORD vk, LPARAM Mods)
{
	// Некоторые комбинации обрабатываются в самом ConEmu и сюда приходить по идее не должны
#if 0
	if (vk == VK_SPACE && (Mods == (Mods & (MOD_ALT|MOD_LALT|MOD_RALT))))
	{
		_ASSERTE(vk != VK_SPACE);
		if (!gpSet->isSendAltSpace)
		{
			ShowSysmenu();
			return;
		}
	}
	if (vk == VK_RETURN && (Mods == (Mods & (MOD_ALT|MOD_LALT|MOD_RALT))))
	{
		_ASSERTE(vk != VK_RETURN);
		if (!gpSet->isSendAltEnter)
		{
			DoFullScreen();
			return;
		}
	}
#endif

	// Все остальное - обычным образом
	CVConGuard VCon;
	CRealConsole* pRCon = (CVConGroup::GetActiveVCon(&VCon) >= 0) ? VCon->RCon() : NULL;
	if (pRCon)
	{
		INPUT_RECORD r = {KEY_EVENT};
		// Alt и прочих слать не нужно - он уже послан
		r.Event.KeyEvent.bKeyDown = TRUE;
		r.Event.KeyEvent.wRepeatCount = 1;
		r.Event.KeyEvent.wVirtualKeyCode = vk;
		r.Event.KeyEvent.wVirtualScanCode = /*28 на моей клавиатуре*/MapVirtualKey(vk, 0/*MAPVK_VK_TO_VSC*/);
		if (Mods & MOD_LALT)
			r.Event.KeyEvent.dwControlKeyState |= LEFT_ALT_PRESSED;
		if (Mods & MOD_RALT)
			r.Event.KeyEvent.dwControlKeyState |= RIGHT_ALT_PRESSED;
		if (Mods & MOD_LCONTROL)
			r.Event.KeyEvent.dwControlKeyState |= LEFT_CTRL_PRESSED;
		if (Mods & MOD_RCONTROL)
			r.Event.KeyEvent.dwControlKeyState |= RIGHT_CTRL_PRESSED;
		if (Mods & MOD_SHIFT)
			r.Event.KeyEvent.dwControlKeyState |= SHIFT_PRESSED;
		pRCon->PostConsoleEvent(&r);
		//On Keyboard(hConWnd, WM_KEYUP, VK_RETURN, 0);
		r.Event.KeyEvent.bKeyDown = FALSE;
		pRCon->PostConsoleEvent(&r);
	}
}

bool CConEmuMain::isInImeComposition()
{
	return mb_InImeComposition;
}

void CConEmuMain::UpdateImeComposition()
{
	if (!mh_Imm32)
		return;

	CVConGuard VCon;
	if (GetActiveVCon(&VCon) >= 0)
	{
		HWND hView = VCon->GetView();

		CONSOLE_CURSOR_INFO ci = {};
		CONSOLE_SCREEN_BUFFER_INFO sbi = {};
		CRealConsole* pRCon = VCon->RCon();
		pRCon->GetConsoleCursorInfo(&ci);
		pRCon->GetConsoleScreenBufferInfo(&sbi);
		COORD crVisual = pRCon->BufferToScreen(sbi.dwCursorPosition);
		crVisual.X = max(0,min(crVisual.X,(int)pRCon->TextWidth()));
		crVisual.Y = max(0,min(crVisual.Y,(int)pRCon->TextHeight()));
		POINT ptCurPos = VCon->ConsoleToClient(crVisual.X, crVisual.Y);

		MapWindowPoints(hView, ghWnd, &ptCurPos, 1);

		COMPOSITIONFORM cf = {CFS_POINT, ptCurPos};

		HIMC hImc = _ImmGetContext(ghWnd/*hView?*/);
		_ImmSetCompositionWindow(hImc, &cf);

		LOGFONT lf; gpSetCls->GetMainLogFont(lf);
		_ImmSetCompositionFont(hImc, &lf);
	}
}

LRESULT CConEmuMain::OnKeyboardIme(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	bool lbProcessed = false;
	wchar_t szDbgMsg[255];
	switch (messg)
	{
		case WM_IME_CHAR:
			{
				// Посылается в RealConsole (это ввод из окошка IME)
				_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"WM_IME_CHAR: char=%c, wParam=%u, lParam=0x%08X\n", (wchar_t)wParam, (DWORD)wParam, (DWORD)lParam);
				DEBUGSTRIME(szDbgMsg);
				CVConGuard VCon;
				if (GetActiveVCon(&VCon) >= 0)
				{
					VCon->RCon()->OnKeyboardIme(hWnd, messg, wParam, lParam);
				}
			}
			break;
		case WM_IME_COMPOSITION:
			_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"WM_IME_COMPOSITION: wParam=0x%08X, lParam=0x%08X\n", (DWORD)wParam, (DWORD)lParam);
			DEBUGSTRIME(szDbgMsg);
			UpdateImeComposition();
			//if (lParam & GCS_RESULTSTR) 
			//{
			//	HIMC hIMC = ImmGetContext(hWnd);
			//	if (hIMC)
			//	{
			//		DWORD dwSize;
			//		wchar_t* lpstr;
			//		// Get the size of the result string. 
			//		dwSize = ImmGetCompositionString(hIMC, GCS_RESULTSTR, NULL, 0);
			//		// increase buffer size for terminating null character,  
			//		dwSize += sizeof(wchar_t);
			//		lpstr = (wchar_t*)calloc(dwSize,1);
			//		if (lpstr)
			//		{
			//			// Get the result strings that is generated by IME into lpstr. 
			//			ImmGetCompositionString(hIMC, GCS_RESULTSTR, lpstr, dwSize);
			//			DEBUGSTRIME(L"GCS_RESULTSTR: ");
			//			DEBUGSTRIME(lpstr);
			//			// add this string into text buffer of application 
			//			free(lpstr);
			//		}
			//		ImmReleaseContext(hWnd, hIMC);
			//	}
			//}
			break;
		case WM_IME_COMPOSITIONFULL:
			_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"WM_IME_COMPOSITIONFULL: wParam=0x%08X, lParam=0x%08X\n", (DWORD)wParam, (DWORD)lParam);
			DEBUGSTRIME(szDbgMsg);
			break;
		case WM_IME_CONTROL:
			_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"WM_IME_CONTROL: wParam=0x%08X, lParam=0x%08X\n", (DWORD)wParam, (DWORD)lParam);
			DEBUGSTRIME(szDbgMsg);
			break;
		case WM_IME_ENDCOMPOSITION:
			_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"WM_IME_ENDCOMPOSITION: wParam=0x%08X, lParam=0x%08X\n", (DWORD)wParam, (DWORD)lParam);
			DEBUGSTRIME(szDbgMsg);
			// IME закончен. Обычные нажатия опять можно посылать в консоль
			mb_InImeComposition = false;
			break;
		case WM_IME_KEYDOWN:
			_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"WM_IME_KEYDOWN: wParam=0x%08X, lParam=0x%08X\n", (DWORD)wParam, (DWORD)lParam);
			DEBUGSTRIME(szDbgMsg);
			break;
		case WM_IME_KEYUP:
			_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"WM_IME_KEYUP: wParam=0x%08X, lParam=0x%08X\n", (DWORD)wParam, (DWORD)lParam);
			DEBUGSTRIME(szDbgMsg);
			break;
		case WM_IME_NOTIFY:
			_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"WM_IME_NOTIFY: wParam=0x%08X, lParam=0x%08X\n", (DWORD)wParam, (DWORD)lParam);
			DEBUGSTRIME(szDbgMsg);
			switch (wParam)
			{
				case IMN_SETOPENSTATUS:
					// Как-то странно, нажатие Alt` (в японской раскладке) посылает в консоль артефактный символ
					// (пересылает нажатие в консоль)
					mb_ImeMethodChanged = (wParam == IMN_SETOPENSTATUS) && isPressed(VK_MENU);
					break;
				//case 0xF:
				//	{
				//		wchar_t szInfo[1024];
				//		getWindowInfo((HWND)lParam, szInfo);
				//	}
				//	break;
			}
			break;
		case WM_IME_REQUEST:
			_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"WM_IME_REQUEST: wParam=0x%08X, lParam=0x%08X\n", (DWORD)wParam, (DWORD)lParam);
			DEBUGSTRIME(szDbgMsg);
			//if (wParam == IMR_QUERYCHARPOSITION)
			//{
			//	IMECHARPOSITION* p = (IMECHARPOSITION*)lParam;
			//	GetWindowRect(ghWnd DC, &p->rcDocument);
			//	p->cLineHeight = gpSetCls->FontHeight();
			//	if (mp_ VActive && mp_ VActive->RCon())
			//	{
			//		COORD crCur = {};
			//		mp_ VActive->RCon()->GetConsoleCursorPos(&crCur);
			//		p->pt = mp_ VActive->ConsoleToClient(crCur.X, crCur.Y);
			//	}
			//	lbProcessed = true;
			//	result = TRUE;
			//}
			break;
		case WM_IME_SELECT:
			_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"WM_IME_SELECT: wParam=0x%08X, lParam=0x%08X\n", (DWORD)wParam, (DWORD)lParam);
			DEBUGSTRIME(szDbgMsg);
			break;
		case WM_IME_SETCONTEXT:
			// If the application draws the composition window, 
			// the default IME window does not have to show its composition window.
			// In this case, the application must clear the ISC_SHOWUICOMPOSITIONWINDOW
			// value from the lParam parameter before passing the message to DefWindowProc
			// or ImmIsUIMessage. To display a certain user interface window, an application
			// should remove the corresponding value so that the IME will not display it.
			_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"WM_IME_SETCONTEXT: wParam=0x%08X, lParam=0x%08X\n", (DWORD)wParam, (DWORD)lParam);
			DEBUGSTRIME(szDbgMsg);
			break;
		case WM_IME_STARTCOMPOSITION:
			_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"WM_IME_STARTCOMPOSITION: wParam=0x%08X, lParam=0x%08X\n", (DWORD)wParam, (DWORD)lParam);
			DEBUGSTRIME(szDbgMsg);
			UpdateImeComposition();
			// Начало IME. Теперь нужно игнорировать обычные нажатия (не посылать в консоль)
			mb_InImeComposition = true;
			break;
		default:
			_ASSERTE(messg==0);
	}
	if (!lbProcessed)
		result = DefWindowProc(hWnd, messg, wParam, lParam);
	return result;
}

void CConEmuMain::AppendHKL(wchar_t* szInfo, size_t cchInfoMax, HKL* hKeyb, int nCount)
{
	INT_PTR nLen = _tcslen(szInfo);
	INT_PTR cchLen = cchInfoMax - nLen - 1;
	int i = 0;
	wchar_t* pszStr = szInfo + nLen;
	while ((i < nCount) && (cchLen > 40))
	{
		_wsprintf(pszStr, SKIPLEN(cchLen) WIN3264TEST(L"0x%08X",L"0x%08X%08X") L" ", WIN3264WSPRINT(hKeyb[i]));
		nLen = _tcslen(pszStr);
		cchLen -= nLen;
		pszStr += nLen;
		i++;
	}
}

void CConEmuMain::AppendRegisteredLayouts(wchar_t* szInfo, size_t cchInfoMax)
{
	INT_PTR nLen = _tcslen(szInfo);
	INT_PTR cchLen = cchInfoMax - nLen - 1;
	wchar_t* pszStr = szInfo + nLen;
	for (size_t i = 0; (i < countof(m_LayoutNames)) && (cchLen > 40); i++)
	{
		if (!m_LayoutNames[i].bUsed)
			continue;
		_wsprintf(pszStr, SKIPLEN(cchLen) L"{0x%08X," WIN3264TEST(L"0x%08X",L"0x%08X%08X") L"} ",
			m_LayoutNames[i].klName, WIN3264WSPRINT(m_LayoutNames[i].hkl));
		nLen = _tcslen(pszStr);
		cchLen -= nLen;
		pszStr += nLen;
	}
}

void CConEmuMain::CheckActiveLayoutName()
{
	wchar_t szLayout[KL_NAMELENGTH] = {0}, *pszEnd = NULL;
	GetKeyboardLayoutName(szLayout);
	HKL hkl = GetKeyboardLayout(0);
	HKL hKeyb[20] = {};
	UINT nCount = GetKeyboardLayoutList(countof(hKeyb), hKeyb);

	#ifdef TEST_JP_LANS
	wcscpy_c(szLayout, L"00000411");
	hkl = (HKL)0x04110411;
	nCount = 3; hKeyb[0] = (HKL)0x04110411; hKeyb[0] = (HKL)0xE0200411; hKeyb[0] = (HKL)0x04090409;
	#endif

	DWORD dwLayout = wcstoul(szLayout, &pszEnd, 16);

	wchar_t szInfo[200];
	_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"Layout=%s, HKL=" WIN3264TEST(L"0x%08X",L"0x%08X%08X") L", Count=%u\n", szLayout, WIN3264WSPRINT(hkl), nCount);
	AppendHKL(szInfo, countof(szInfo), hKeyb, nCount);

	int i, iUnused = -1;

	if (dwLayout)
	{
		for (i = 0; i < (int)countof(m_LayoutNames); i++)
		{
			if (!m_LayoutNames[i].bUsed)
			{
				if (iUnused == -1) iUnused = i;
				continue;
			}

			if (m_LayoutNames[i].klName == dwLayout)
			{
				iUnused = -2; // set "-2" to avoid assert lower
				break;
			}
		}
	}

	if (iUnused >= 0)
	{
		// Старшие слова совпадать не будут (если совпадают, то случайно)
		// А вот младшие - должны бы, или хотя бы "Primary Language ID". Но...
		// Issue 1050 #10: Layout=00020405, HKL=0xFFFFFFFFF00A0409. Weird...
		if ((LOWORD(hkl) & 0xFF) != (LOWORD(dwLayout) & 0xFF))
		{
			wcscat_c(szInfo, L"\nLOWORD(hkl)==LOWORD(dwLayout) -- Warning, strange layout ID");
			wchar_t* psz = szInfo; while (psz && (psz = wcschr(psz, L'\n'))) *psz = L' ';
			LogString(szInfo);
		}

		// So, always store "startup" layout and HKL for further reference...
		StoreLayoutName(iUnused, dwLayout, hkl);
	}
	else if (iUnused == -1)
	{
		// Startup keyboard layout must be detected
		wcscat_c(szInfo, L"\niUnused==-1");
		AssertMsg(szInfo);
	}
}

void CConEmuMain::StoreLayoutName(int iIdx, DWORD dwLayout, HKL hkl)
{
	_ASSERTE(iIdx>=0 && iIdx<countof(m_LayoutNames) && !m_LayoutNames[iIdx].bUsed);
	wchar_t szInfo[100];
	_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"StoreLayoutName(%i,x%08X," WIN3264TEST(L"0x%08X",L"0x%08X%08X") L")", iIdx, dwLayout, WIN3264WSPRINT(hkl));
	LogString(szInfo);

	// Check. This layout must not be stored already!
	for (size_t i = 0; i < countof(m_LayoutNames); i++)
	{
		if (m_LayoutNames[i].bUsed
			&& ((m_LayoutNames[i].klName == dwLayout)
				|| (m_LayoutNames[i].hkl == (DWORD_PTR)hkl)))
		{
			wchar_t szAssert[255];
			_wsprintf(szAssert, SKIPLEN(countof(szAssert)) L"Layout 0x%08X was already registered (hkl=" WIN3264TEST(L"0x%08X",L"0x%08X%08X") L")\nRegList: ", dwLayout, WIN3264WSPRINT(hkl));
			AppendRegisteredLayouts(szAssert, countof(szAssert));
			AssertMsg(szAssert);
		}
	}

	m_LayoutNames[iIdx].bUsed = TRUE;
	m_LayoutNames[iIdx].klName = dwLayout;
	m_LayoutNames[iIdx].hkl = (DWORD_PTR)hkl;
}

LRESULT CConEmuMain::OnLangChange(UINT messg, WPARAM wParam, LPARAM lParam)
{
	/*
	**********
	Вызовы идут в следующем порядке (WinXP SP3)
	**********
	En -> Ru : --> Слетает после этого!
	**********
	19:17:15.043(gui.4720) ConEmu: WM_INPUTLANGCHANGEREQUEST(CP:1, HKL:0x04190419)
	19:17:17.043(gui.4720) ConEmu: WM_INPUTLANGCHANGE(CP:204, HKL:0x04190419)
	19:17:19.043(gui.4720) ConEmu: GetKeyboardLayout(0) after DefWindowProc(WM_INPUTLANGCHANGE) = 0x04190419)
	19:17:21.043(gui.4720) ConEmu: GetKeyboardLayout(0) after DefWindowProc(WM_INPUTLANGCHANGEREQUEST) = 0x04190419)
	19:17:23.043(gui.4720) CRealConsole::SwitchKeyboardLayout(CP:1, HKL:0x04190419)
	19:17:25.044(gui.4720) RealConsole: WM_INPUTLANGCHANGEREQUEST, CP:1, HKL:0x04190419 via CmdExecute
	19:17:27.044(gui.3072) GUI recieved CECMD_LANGCHANGE
	19:17:29.044(gui.4720) ConEmu: GetKeyboardLayout(0) in OnLangChangeConsole after GetKeyboardLayout(0) = 0x04190419
	19:17:31.044(gui.4720) ConEmu: GetKeyboardLayout(0) in OnLangChangeConsole after GetKeyboardLayout(0) = 0x04190419
	--> Слетает после этого!
	'ConEmu.exe': Loaded 'C:\WINDOWS\system32\kbdru.dll'
	'ConEmu.exe': Unloaded 'C:\WINDOWS\system32\kbdru.dll'
	19:17:33.075(gui.4720) ConEmu: Calling GetKeyboardLayout(0)
	19:17:35.075(gui.4720) ConEmu: GetKeyboardLayout(0) after LoadKeyboardLayout = 0x04190419
	19:17:37.075(gui.4720) ConEmu: GetKeyboardLayout(0) after SwitchKeyboardLayout = 0x04190419
	**********
	Ru -> En : --> Слетает после этого!
	**********
	17:23:36.013(gui.3152) ConEmu: WM_INPUTLANGCHANGEREQUEST(CP:1, HKL:0x04090409)
	17:23:36.013(gui.3152) ConEmu: WM_INPUTLANGCHANGE(CP:0, HKL:0x04090409)
	17:23:36.013(gui.3152) ConEmu: GetKeyboardLayout(0) after DefWindowProc(WM_INPUTLANGCHANGE) = 0x04090409)
	17:23:36.013(gui.3152) ConEmu: GetKeyboardLayout(0) after DefWindowProc(WM_INPUTLANGCHANGEREQUEST) = 0x04090409)
	17:23:36.013(gui.3152) CRealConsole::SwitchKeyboardLayout(CP:1, HKL:0x04090409)
	17:23:36.013(gui.3152) RealConsole: WM_INPUTLANGCHANGEREQUEST, CP:1, HKL:0x04090409 via CmdExecute
	ConEmuC: PostMessage(WM_INPUTLANGCHANGEREQUEST, CP:1, HKL:0x04090409)
	The thread 'Win32 Thread' (0x3f0) has exited with code 1 (0x1).
	ConEmuC: InputLayoutChanged (GetConsoleKeyboardLayoutName returns) '00000409'
	17:23:38.013(gui.4460) GUI recieved CECMD_LANGCHANGE
	17:23:40.028(gui.4460) ConEmu: GetKeyboardLayout(0) on CECMD_LANGCHANGE after GetKeyboardLayout(0) = 0x04090409
	--> Слетает после этого!
	'ConEmu.exe': Loaded 'C:\WINDOWS\system32\kbdus.dll'
	'ConEmu.exe': Unloaded 'C:\WINDOWS\system32\kbdus.dll'
	17:23:42.044(gui.4460) ConEmu: Calling GetKeyboardLayout(0)
	17:23:44.044(gui.4460) ConEmu: GetKeyboardLayout(0) after LoadKeyboardLayout = 0x04090409
	17:23:46.044(gui.4460) ConEmu: GetKeyboardLayout(0) after SwitchKeyboardLayout = 0x04090409
	*/
	LRESULT result = 1;
#ifdef _DEBUG
	WCHAR szMsg[255];
	_wsprintf(szMsg, SKIPLEN(countof(szMsg)) L"ConEmu: %s(CP:%i, HKL:0x%08I64X)\n",
	          (messg == WM_INPUTLANGCHANGE) ? L"WM_INPUTLANGCHANGE" : L"WM_INPUTLANGCHANGEREQUEST",
	          (DWORD)wParam, (unsigned __int64)(DWORD_PTR)lParam);
	DEBUGSTRLANG(szMsg);
#endif

	if (gpSetCls->isAdvLogging > 1)
	{
		WCHAR szInfo[255];
		_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"CConEmuMain::OnLangChange: %s(CP:%i, HKL:0x%08I64X)",
		          (messg == WM_INPUTLANGCHANGE) ? L"WM_INPUTLANGCHANGE" : L"WM_INPUTLANGCHANGEREQUEST",
		          (DWORD)wParam, (unsigned __int64)(DWORD_PTR)lParam);
		LogString(szInfo);
	}

	/*
	wParam = 204, lParam = 0x04190419 - Russian
	wParam = 0,   lParam = 0x04090409 - US
	wParam = 0,   lParam = 0xfffffffff0020409 - US Dvorak
	wParam = 0,   lParam = 0xfffffffff01a0409 - US Dvorak left hand
	wParam = 0,   lParam = 0xfffffffff01b0409 - US Dvorak
	*/
	//POSTMESSAGE(ghConWnd, messg, wParam, lParam, FALSE);
	//mn_CurrentKeybLayout = lParam;
#ifdef _DEBUG
	wchar_t szBeforeChange[KL_NAMELENGTH] = {0}; GetKeyboardLayoutName(szBeforeChange);
#endif
	// Собственно смена
	result = DefWindowProc(ghWnd, messg, wParam, lParam);
	// Проверяем, что получилось
	wchar_t szAfterChange[KL_NAMELENGTH] = {0}; GetKeyboardLayoutName(szAfterChange);
	//if (wcscmp(szAfterChange, szBeforeChange)) -- практика показывает, что уже совпадают.
	//{
	wchar_t *pszEnd = szAfterChange+8;
	DWORD dwLayoutAfterChange = wcstoul(szAfterChange, &pszEnd, 16);
	// Запомнить, что получилось в m_LayoutNames
	int i, iUnused = -1;

	for (i = 0; i < (int)countof(m_LayoutNames); i++)
	{
		if (!m_LayoutNames[i].bUsed)
		{
			if (iUnused == -1) iUnused = i; continue;
		}

		if (m_LayoutNames[i].klName == dwLayoutAfterChange)
		{
			iUnused = -1; break;
		}
	}

	if (iUnused != -1)
	{
		DEBUGTEST(HKL hkl = GetKeyboardLayout(0));
		_ASSERTE((DWORD_PTR)hkl == (DWORD_PTR)lParam);
		StoreLayoutName(iUnused, dwLayoutAfterChange, (HKL)(DWORD_PTR)lParam);
	}

	//}
#ifdef _DEBUG
	HKL hkl = GetKeyboardLayout(0);
	_wsprintf(szMsg, SKIPLEN(countof(szMsg)) L"ConEmu: GetKeyboardLayout(0) after DefWindowProc(%s) = 0x%08I64X)\n",
	          (messg == WM_INPUTLANGCHANGE) ? L"WM_INPUTLANGCHANGE" : L"WM_INPUTLANGCHANGEREQUEST",
	          (unsigned __int64)(DWORD_PTR)hkl);
	DEBUGSTRLANG(szMsg);
#endif

	// в Win7x64 WM_INPUTLANGCHANGEREQUEST вообще не приходит, по крайней мере при переключении мышкой
	if (messg == WM_INPUTLANGCHANGEREQUEST || messg == WM_INPUTLANGCHANGE)
	{
		static UINT   nLastMsg;
		static LPARAM lLastPrm;

		CVConGuard VCon;
		CRealConsole* pRCon = (GetActiveVCon(&VCon) >= 0) ? VCon->RCon() : NULL;

		if (pRCon && !(nLastMsg == WM_INPUTLANGCHANGEREQUEST && messg == WM_INPUTLANGCHANGE && lLastPrm == lParam))
		{
			pRCon->SwitchKeyboardLayout(
			    (messg == WM_INPUTLANGCHANGEREQUEST) ? wParam : 0, lParam
			);
		}

		nLastMsg = messg;
		lLastPrm = lParam;

		// -- Эффекта не имеет
		//if (pRCon)
		//{
		//	HWND hGuiWnd = pRCon->GuiWnd();
		//	if (hGuiWnd)
		//		pRCon->PostConsoleMessage(hGuiWnd, messg, wParam, lParam);
		//}
	}

	m_ActiveKeybLayout = (DWORD_PTR)lParam;

	mp_Status->OnKeyboardChanged();

	//  if (isFar() && gpSet->isLangChangeWsPlugin)
	//  {
	//   //LONG lLastLang = GetWindowLong ( ghWnd DC, GWL_LANGCHANGE );
	//   //SetWindowLong ( ghWnd DC, GWL_LANGCHANGE, lParam );
	//
	//   /*if (lLastLang == lParam)
	//    return result;*/
	//
	//CConEmuPipe pipe(GetFarPID(), 10);
	//if (pipe.Init(_T("CConEmuMain::OnLangChange"), FALSE))
	//{
	//  if (pipe.Execute(CMD_LANGCHANGE, &lParam, sizeof(LPARAM)))
	//  {
	//      //gpConEmu->DebugStep(_T("ConEmu: Switching language (1 sec)"));
	//      // Подождем немножко, проверим что плагин живой
	//      /*DWORD dwWait = WaitForSingleObject(pipe.hEventAlive, CONEMUALIVETIMEOUT);
	//      if (dwWait == WAIT_OBJECT_0)*/
	//          return result;
	//  }
	//}
	//  }
	//  //POSTMESSAGE(ghConWnd, messg, wParam, lParam, FALSE);
	//  //SENDMESSAGE(ghConWnd, messg, wParam, lParam);
	//  //if (messg == WM_INPUTLANGCHANGEREQUEST)
	//  {
	//      //wParam Specifies the character set of the new locale.
	//      //lParam - HKL
	//      //ActivateKeyboardLayout((HKL)lParam, 0);
	//      //POSTMESSAGE(ghConWnd, messg, wParam, lParam, FALSE);
	//      //SENDMESSAGE(ghConWnd, messg, wParam, lParam);
	//  }
	//  if (messg == WM_INPUTLANGCHANGE)
	//  {
	//      //SENDMESSAGE(ghConWnd, WM_SETFOCUS, 0,0);
	//      //POSTMESSAGE(ghConWnd, WM_SETFOCUS, 0,0, TRUE);
	//      //POSTMESSAGE(ghWnd, WM_SETFOCUS, 0,0, TRUE);
	//  }
	return result;
}

// dwLayoutName содержит не HKL, а "имя" (i.e. "00030409") сконвертированное в DWORD
LRESULT CConEmuMain::OnLangChangeConsole(CVirtualConsole *apVCon, const DWORD adwLayoutName)
{
	if ((gpSet->isMonitorConsoleLang & 1) != 1)
		return 0;

	if (!isValid(apVCon))
		return 0;

	wchar_t szInfo[255];

	// bypass x64 debugger bug
	DWORD dwLayoutName = adwLayoutName;

	if (!dwLayoutName)
	{
		// Ubuntu, Wine. Get 0 here.
		_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"CConEmuMain::OnLangChangeConsole (0x%08X), Wine=%i", dwLayoutName, (int)gbIsWine);
		LogString(szInfo);
		return 0;
	}

	if (!isMainThread())
	{
		// --> не "нравится" руслату обращение к раскладкам из фоновых потоков. Поэтому выполяется в основном потоке
		if (gpSetCls->isAdvLogging > 1)
		{
			_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"CConEmuMain::OnLangChangeConsole (0x%08X), Posting to main thread", dwLayoutName);
			LogString(szInfo);
		}

		PostMessage(ghWnd, mn_ConsoleLangChanged, dwLayoutName, (LPARAM)apVCon);
		return 0;
	}
	else
	{
		if (gpSetCls->isAdvLogging > 1)
		{
			_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"CConEmuMain::OnLangChangeConsole (0x%08X), MainThread", dwLayoutName);
			LogString(szInfo);
		}
	}


	wchar_t szName[10]; _wsprintf(szName, SKIPLEN(countof(szName)) L"%08X", dwLayoutName);


	#ifdef _DEBUG
	//Sleep(2000);
	WCHAR szMsg[255];
	HKL hkl = GetKeyboardLayout(0);
	_wsprintf(szMsg, SKIPLEN(countof(szMsg))
		L"ConEmu: GetKeyboardLayout(0) in OnLangChangeConsole after GetKeyboardLayout(0) = "
		WIN3264TEST(L"0x%08X",L"0x%08X%08X") L"\n",
	    WIN3264WSPRINT((DWORD_PTR)hkl));
	DEBUGSTRLANG(szMsg);
	//Sleep(2000);
	#endif

	#ifdef _DEBUG
	DEBUGSTRLANG(szName);
	#endif

	DWORD_PTR dwNewKeybLayout = dwLayoutName; //(DWORD_PTR)LoadKeyboardLayout(szName, 0);
	HKL hKeyb[20] = {};
	UINT nCount, i, j, s, nErr;
	SetLastError(0);
	nCount = GetKeyboardLayoutList(countof(hKeyb), hKeyb);
	nErr = GetLastError();
	/*
	HKL:
	0x0000000004090409 - US
	0x0000000004190419 - Russian
	0xfffffffff0010409 - US - International
	0xfffffffff0020409 - US - Dvorak
	0xfffffffff01a0409 - US - Dvorak left hand
	0xfffffffff01b0409 - US - Dvorak right hand
	Layout (dwLayoutName):
	0x00010409 - US - Dvorak
	0x00030409 - US - Dvorak left hand
	0x00040409 - US - Dvorak right hand
	*/
	BOOL lbFound = FALSE;
	int iUnused = -1, iUnknownLeft = -1;

	// Check first, this layout may be found already
	for (i = 0; i < countof(m_LayoutNames); i++)
	{
		if (!m_LayoutNames[i].bUsed)
		{
			if (iUnused == -1) iUnused = i;

			continue;
		}

		if (m_LayoutNames[i].klName == dwLayoutName)
		{
			lbFound = TRUE;
			dwNewKeybLayout = m_LayoutNames[i].hkl;
			iUnused = -1; // запоминать не потребуется
			break;
		}
	}

	// LCIDToLocaleName(dwLayoutName, ...) 

	// s - number of items in (switch)
	for (s = 0; !lbFound && (s <= 5); s++)
	{
		for (i = 0; (i < nCount); i++)
		{
			// "классическая" раскладка, в которой ид раскладки совпадает с языком?
			if ((s == 0) && ((dwLayoutName & 0xFFFF) != dwLayoutName))
				continue;

			bool bIgnore = false;
			for (j = 0; j < countof(m_LayoutNames); j++)
			{
				if (!m_LayoutNames[j].bUsed)
					continue;

				if (m_LayoutNames[j].hkl == (DWORD_PTR)hKeyb[i])
				{
					bIgnore = true;
					break;
				}
			}
			if (bIgnore)
				continue;

			switch (s)
			{
			case 0:
				// это "классическая" раскладка, в которой ид раскладки совпадает с языком?
				lbFound = (((DWORD_PTR)hKeyb[i]) == (dwNewKeybLayout | (dwNewKeybLayout << 16)));
				break;
			case 1:
				// Issue 1035: dwLayoutName=0x00020409; hKeyb={0xfffffffff0010414,0xfffffffff0010409}
				// Любая (свободная/необработанная) раскладка для требуемого языка?
				lbFound = (HIWORD(hKeyb[i]) != LOWORD(hKeyb[i])) && (LOWORD(hKeyb[i]) == LOWORD(dwNewKeybLayout));
				//lbFound = ((((DWORD_PTR)hKeyb[i]) & 0xFFFFFFF) == dwNewKeybLayout);
				break;
			case 2:
				// По младшему слову
				lbFound = (LOWORD((DWORD_PTR)hKeyb[i]) == LOWORD(dwNewKeybLayout));
				break;
			case 3:
				// Если опять не нашли - то сравниваем старшее (HIWORD) слово раскладки
				lbFound = (HIWORD((DWORD_PTR)hKeyb[i]) == LOWORD(dwNewKeybLayout));
				break;
			case 4:
				// Last chance - Primary Language ID
				lbFound = ((LOWORD((DWORD_PTR)hKeyb[i]) & 0xFF) == (LOWORD(dwNewKeybLayout) & 0xFF));
				break;
			case 5:
				// If there is only "unlinked" HKL - choose it
				iUnknownLeft = 0;
				for (UINT i1 = 0; i1 < nCount; i1++)
				{
					bool bKnown = false;
					for (size_t i2 = 0; i2 < countof(m_LayoutNames); i2++)
					{
						if (!m_LayoutNames[i2].bUsed)
							continue;

						if (m_LayoutNames[i2].hkl == (DWORD_PTR)hKeyb[i1])
						{
							bKnown = true;
							break;
						}
					}
					if (!bKnown)
					{
						iUnknownLeft++;
					}
				}
				lbFound = (iUnknownLeft == 1);
				wsprintf(szInfo, L"hkl=0x%08X, dwLayout=" WIN3264TEST(L"0x%08X",L"0x%08X%08X") L" -- Warning, strange layout ID", dwLayoutName, WIN3264WSPRINT(hKeyb[i]));
				LogString(szInfo);
				break;
			//case 3:
			//	// Last change - check LOWORD of both HKL & Layout
			//	// Issue 1035: dwLayoutName=0x00020409; hKeyb={0xfffffffff0010414,0xfffffffff0010409}
			//	lbFound = (LOWORD((DWORD_PTR)hKeyb[i]) == LOWORD(dwNewKeybLayout));
			//	break;
			default:
				Assert(FALSE && "Invalid index");
			}

			if (lbFound)
			{
				dwNewKeybLayout = (DWORD_PTR)hKeyb[i];
				break;
			}
		}
	}

	//// Если не нашли, и это "классическая" раскладка, в которой ид раскладки совпадает с языком
	//if (!lbFound && ((dwLayoutName & 0xFFFF) == dwLayoutName))
	//{
	//	DWORD_PTR dwTest = dwNewKeybLayout | (dwNewKeybLayout << 16);

	//	for (i = 0; i < nCount; i++)
	//	{
	//		if (((DWORD_PTR)hKeyb[i]) == dwTest)
	//		{
	//			lbFound = TRUE;
	//			dwNewKeybLayout = (DWORD_PTR)hKeyb[i];
	//			break;
	//		}
	//	}

	//	// Если опять не нашли - то сравниваем старшее (HIWORD) слово раскладки
	//	if (!lbFound)
	//	{
	//		WORD wTest = LOWORD(dwNewKeybLayout);

	//		for (i = 0; i < nCount; i++)
	//		{
	//			if (HIWORD((DWORD_PTR)hKeyb[i]) == wTest)
	//			{
	//				lbFound = TRUE;
	//				dwNewKeybLayout = (DWORD_PTR)hKeyb[i];
	//				break;
	//			}
	//		}
	//	}
	//}

	//// Last change - check LOWORD of both HKL & Layout
	//// Issue 1035: dwLayoutName=0x00020409; hKeyb={0xfffffffff0010414,0xfffffffff0010409}
	//if (!lbFound)
	//{
	//	WORD wTest = LOWORD(dwNewKeybLayout);

	//	for (i = 0; i < nCount; i++)
	//	{
	//		if (LOWORD((DWORD_PTR)hKeyb[i]) == wTest)
	//		{
	//			lbFound = TRUE;
	//			dwNewKeybLayout = (DWORD_PTR)hKeyb[i];
	//			break;
	//		}
	//	}
	//}

	if ((!lbFound && gpSetCls->isAdvLogging) || (gpSetCls->isAdvLogging >= 2))
	{
		_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"Keyboard layout lookup (0x%08X) %s", dwLayoutName, lbFound ? L"Succeeded" : L"FAILED");
		LogString(szInfo);

		HKL hkl = GetKeyboardLayout(0);
		_wsprintf(szInfo, SKIPLEN(countof(szInfo))
			L"  Current keyboard layout:\r\n       " WIN3264TEST(L"0x%08X",L"0x%08X%08X") L"\n",
		    WIN3264WSPRINT((DWORD_PTR)hkl));
	    LogString(szInfo, false, false);

		LogString(L"  Installed keyboard layouts:\r\n", false, false);
    	for (i = 0; i < nCount; i++)
		{
			_wsprintf(szInfo, SKIPLEN(countof(szInfo))
				L"    %u: " WIN3264TEST(L"0x%08X",L"0x%08X%08X") L"\n",
			    i+1, WIN3264WSPRINT(hKeyb[i]));
		    LogString(szInfo, false, false);
		}
	}

	if (!lbFound)
	{
		_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"dwLayoutName not found: Count=%u,Left=%i,Err=%u,%08X\r\n", nCount, iUnknownLeft, nErr, dwLayoutName);
		AppendHKL(szInfo, countof(szInfo), hKeyb, nCount);

		// Загружать НОВЫЕ раскладки - некорректно (Issue 944)
		AssertMsg(szInfo);

		//wchar_t szLayoutName[9] = {0};
		//_wsprintf(szLayoutName, SKIPLEN(countof(szLayoutName)) L"%08X", dwLayoutName);

		//if (gpSetCls->isAdvLogging > 1)
		//{
		//	WCHAR szInfo[255];
		//	_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"CConEmuMain::OnLangChangeConsole -> LoadKeyboardLayout(0x%08X)", dwLayoutName);
		//	LogString(szInfo);
		//}

		//dwNewKeybLayout = (DWORD_PTR)LoadKeyboardLayout(szLayoutName, 0);

		//if (gpSetCls->isAdvLogging > 1)
		//{
		//	WCHAR szInfo[255];
		//	_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"CConEmuMain::OnLangChangeConsole -> LoadKeyboardLayout()=0x%08X", (DWORD)dwNewKeybLayout);
		//	LogString(szInfo);
		//}

		//lbFound = TRUE;
	}

	if (lbFound && iUnused != -1)
	{
		StoreLayoutName(iUnused, dwLayoutName, (HKL)dwNewKeybLayout);
	}

	UNREFERENCED_PARAMETER(nCount);

	//dwNewKeybLayout = (DWORD_PTR)hklNew;
	//for (i = 0; !lbFound && i < nCount; i++)
	//{
	//	if (hKeyb[i] == (HKL)dwNewKeybLayout)
	//		lbFound = TRUE;
	//}
	//WARNING("Похоже с другими раскладками будет глючить. US Dvorak?");
	//for (i = 0; !lbFound && i < nCount; i++)
	//{
	//	if ((((DWORD_PTR)hKeyb[i]) & 0xFFFF) == (dwNewKeybLayout & 0xFFFF))
	//	{
	//		lbFound = TRUE; dwNewKeybLayout = (DWORD_PTR)hKeyb[i];
	//	}
	//}
	//// Если не задана раскладка (только язык?) формируем по умолчанию
	//if (!lbFound && (dwNewKeybLayout == (dwNewKeybLayout & 0xFFFF)))
	//{
	//	dwNewKeybLayout |= (dwNewKeybLayout << 16);
	//}
#ifdef _DEBUG
	DEBUGSTRLANG(L"ConEmu: Calling GetKeyboardLayout(0)\n");
	//Sleep(2000);
	hkl = GetKeyboardLayout(0);
	_wsprintf(szMsg, SKIPLEN(countof(szMsg)) L"ConEmu: GetKeyboardLayout(0) after LoadKeyboardLayout = 0x%08I64X\n",
	          (unsigned __int64)(DWORD_PTR)hkl);
	DEBUGSTRLANG(szMsg);
	//Sleep(2000);
#endif

	if (isActive(apVCon))
	{
		apVCon->RCon()->OnConsoleLangChange(dwNewKeybLayout);
	}

	return 0;
}

void CConEmuMain::SetSkipMouseEvent(UINT nMsg1, UINT nMsg2, UINT nReplaceDblClk)
{
	mouse.nSkipEvents[0] = nMsg1 /*WM_LBUTTONDOWN*/;
	mouse.nSkipEvents[1] = nMsg2 /*WM_LBUTTONUP*/;
	mouse.nReplaceDblClk = nReplaceDblClk /*WM_LBUTTONDBLCLK*/;
}

bool CConEmuMain::PatchMouseEvent(UINT messg, POINT& ptCurClient, POINT& ptCurScreen, WPARAM wParam, bool& isPrivate)
{
	isPrivate = false;

	// Для этих сообщений, lParam - relative to the upper-left corner of the screen.
	if (messg == WM_MOUSEWHEEL || messg == WM_MOUSEHWHEEL)
		ScreenToClient(ghWnd, &ptCurClient);
	else // Для остальных lParam содержит клиентские координаты
		ClientToScreen(ghWnd, &ptCurScreen);

	if (mb_MouseCaptured
		|| (messg == WM_LBUTTONDOWN) || (messg == WM_RBUTTONDOWN) || (messg == WM_MBUTTONDOWN)
		)
	{
		HWND hChild = ::ChildWindowFromPointEx(ghWnd, ptCurClient, CWP_SKIPINVISIBLE|CWP_SKIPDISABLED|CWP_SKIPTRANSPARENT);

		CVConGuard VCon;
		if (isVConHWND(hChild, &VCon)/*(hChild != ghWnd)*/) // Это должна быть VCon
		{
			#ifdef _DEBUG
			wchar_t szClass[128]; GetClassName(hChild, szClass, countof(szClass));
			_ASSERTE(lstrcmp(szClass, VirtualConsoleClass)==0 && "This must be VCon DC window");
			#endif

			//bool bSkipThisEvent = false;

			// WARNING! Тут строго, без учета активности группы!
			if (VCon.VCon() && isVisible(VCon.VCon()) && !isActive(VCon.VCon(), false))
			{
				// по клику - активировать кликнутый сплит
				if ((messg == WM_LBUTTONDOWN) || (messg == WM_RBUTTONDOWN) || (messg == WM_MBUTTONDOWN))
				{
					Activate(VCon.VCon());
				}

				if (gpSet->isMouseSkipActivation)
				{
					if (messg == WM_LBUTTONDOWN)
					{
						SetSkipMouseEvent(WM_LBUTTONDOWN, WM_LBUTTONUP, WM_LBUTTONDBLCLK);
						//bSkipThisEvent = true;
					}
					else if (messg == WM_RBUTTONDOWN)
					{
						SetSkipMouseEvent(WM_RBUTTONDOWN, WM_RBUTTONUP, WM_RBUTTONDBLCLK);
						//bSkipThisEvent = true;
					}
					else if (messg == WM_MBUTTONDOWN)
					{
						SetSkipMouseEvent(WM_MBUTTONDOWN, WM_MBUTTONUP, WM_MBUTTONDBLCLK);
						//bSkipThisEvent = true;
					}
				}
			}


			// Если активны PanelView - они могут транслировать координаты
			POINT ptVConCoord = ptCurClient;
			::MapWindowPoints(ghWnd, hChild, &ptVConCoord, 1);
			HWND hPanelView = ::ChildWindowFromPointEx(hChild, ptVConCoord, CWP_SKIPINVISIBLE|CWP_SKIPDISABLED|CWP_SKIPTRANSPARENT);
			if (hPanelView && (hPanelView != hChild))
			{
				// LClick пропускать в консоль нельзя, чтобы не возникало глюков с позиционированием по клику...
				isPrivate = (messg == WM_LBUTTONDOWN);

				if (mb_MouseCaptured)
				{
					#ifdef _DEBUG
					GetClassName(hPanelView, szClass, countof(szClass));
					_ASSERTE(lstrcmp(szClass, ConEmuPanelViewClass)==0 && "This must be Thumbnail or Tile window");
					#endif

					::MapWindowPoints(hChild, hPanelView, &ptVConCoord, 1);

					DWORD_PTR lRc = (DWORD_PTR)-1;
					if (SendMessageTimeout(hPanelView, mn_MsgPanelViewMapCoord, MAKELPARAM(ptVConCoord.x,ptVConCoord.y), 0, SMTO_NORMAL, PNLVIEWMAPCOORD_TIMEOUT, &lRc) && (lRc != (DWORD_PTR)-1))
					{
						ptCurClient.x = LOWORD(lRc);
						ptCurClient.y = HIWORD(lRc);
						ptCurScreen = ptCurClient;
						ClientToScreen(ghWnd, &ptCurScreen);
					}
				}
			}
		}
	}

	if (messg == WM_LBUTTONDBLCLK)
	{
		mouse.LDblClkDC = ptCurClient;
		mouse.LDblClkTick = TimeGetTime();
	}
	else if ((mouse.lastMsg == WM_LBUTTONDBLCLK) && ((messg == WM_MOUSEMOVE) || (messg == WM_LBUTTONUP)))
	{
		// Тачпады и тачскрины.
		// При двойном тапе может получаться следующая фигня:
		//17:40:25.787(gui.4460) GUI::Mouse WM_ MOUSEMOVE at screen {603x239} x00000000
		//17:40:25.787(gui.4460) GUI::Mouse WM_ LBUTTONDOWN at screen {603x239} x00000001
		//17:40:25.787(gui.4460) GUI::Mouse WM_ LBUTTONUP at screen {603x239} x00000000
		//17:40:25.787(gui.4460) GUI::Mouse WM_ MOUSEMOVE at screen {603x239} x00000000
		//17:40:25.880(gui.4460) GUI::Mouse WM_ LBUTTONDBLCLK at screen {603x239} x00000001
		//17:40:25.880(gui.4460) GUI::Mouse WM_ MOUSEMOVE at screen {598x249} x00000001
		//17:40:25.927(gui.4460) GUI::Mouse WM_ LBUTTONUP at screen {598x249} x00000000
		//17:40:25.927(gui.4460) GUI::Mouse WM_ MOUSEMOVE at screen {598x249} x00000000
		// Т.е. место второго тапа немного смещено (95% случаев)
		// В итоге, винда присылает MOUSEMOVE & LBUTTONUP для этого смещения, что нежелательно,
		// т.к. Far, например, после двойного тапа по папке выполняет позиционирование
		// на файл в этой папке. Как если бы после DblClick был еще один лишний Click.
		DWORD dwDelta = TimeGetTime() - mouse.LDblClkTick;
		if (dwDelta <= TOUCH_DBLCLICK_DELTA)
		{
			if (messg == WM_MOUSEMOVE)
			{
				if ((wParam & (MK_LBUTTON|MK_RBUTTON|MK_MBUTTON)) == MK_LBUTTON)
				{
					return false; // Не пропускать это событие в консоль
				}
			}
			else
			{
				_ASSERTE(messg==WM_LBUTTONUP);
				// А тут мы скорректируем позицию, чтобы консоль думала, что "курсор не двигался"
				ptCurClient = mouse.LDblClkDC;
				ptCurScreen = ptCurClient;
				//MapWindowPoints(
				ClientToScreen(ghWnd, &ptCurScreen);
			}
		}
	}

	return true;
}

LRESULT CConEmuMain::OnMouse(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	// кто его знает, в каких координатах оно пришло...
	//short winX = GET_X_LPARAM(lParam);
	//short winY = GET_Y_LPARAM(lParam);

	if (gpSetCls->isAdvLogging)
		CVConGroup::LogInput(messg, wParam, lParam);

	if (isMenuActive())
	{
		if (mp_Tip)
		{
			mp_Tip->HideTip();
		}

		if (gpSetCls->isAdvLogging)
			LogString("Mouse event skipped due to (isMenuActive())");

		return 0;
	}

	LRESULT lRc = 0;

	#if defined(CONEMU_TABBAR_EX)
	if (mp_TabBar->OnMouse(hWnd, messg, wParam, lParam, lRc))
		return lRc;
	#endif


	//2010-05-20 все-таки будем ориентироваться на lParam, потому что
	//  только так ConEmuTh может передать корректные координаты
	//POINT ptCur = {-1, -1}; GetCursorPos(&ptCur);
	POINT ptCurClient = {(int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam)};
	POINT ptCurScreen = ptCurClient;

	// Коррекция координат или пропуск сообщений
	bool isPrivate = false;
	bool bContinue = PatchMouseEvent(messg, ptCurClient, ptCurScreen, wParam, isPrivate);

#ifdef _DEBUG
	wchar_t szDbg[128];
	_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"GUI::Mouse %s at screen {%ix%i} x%08X%s\n",
		(messg==WM_MOUSEMOVE) ? L"WM_MOUSEMOVE" :
		(messg==WM_LBUTTONDOWN) ? L"WM_LBUTTONDOWN" :
		(messg==WM_LBUTTONUP) ? L"WM_LBUTTONUP" :
		(messg==WM_LBUTTONDBLCLK) ? L"WM_LBUTTONDBLCLK" :
		(messg==WM_RBUTTONDOWN) ? L"WM_RBUTTONDOWN" :
		(messg==WM_RBUTTONUP) ? L"WM_RBUTTONUP" :
		(messg==WM_RBUTTONDBLCLK) ? L"WM_RBUTTONDBLCLK" :
		(messg==WM_MBUTTONDOWN) ? L"WM_MBUTTONDOWN" :
		(messg==WM_MBUTTONUP) ? L"WM_MBUTTONUP" :
		(messg==WM_MBUTTONDBLCLK) ? L"WM_MBUTTONDBLCLK" :
		(messg==0x020A) ? L"WM_MOUSEWHEEL" :
		(messg==0x020B) ? L"WM_XBUTTONDOWN" :
		(messg==0x020C) ? L"WM_XBUTTONUP" :
		(messg==0x020D) ? L"WM_XBUTTONDBLCLK" :
		(messg==0x020E) ? L"WM_MOUSEHWHEEL" :
		L"UnknownMsg",
		ptCurScreen.x,ptCurScreen.y,(DWORD)wParam,
		bContinue ? L"" : L" - SKIPPED!");
	DEBUGSTRMOUSE(szDbg);
#endif

	//gpSet->IsModifierPressed(vkWndDragKey, false))
	if ((messg == WM_LBUTTONDOWN)
		// 120820 - двигать можно только в "Normal"
		&& !(isZoomed() || isFullScreen() || gpSet->isQuakeStyle)
		&& ProcessHotKeyMsg(WM_KEYDOWN, VK_LBUTTON, 0, NULL, NULL))
	{
		mouse.state |= MOUSE_WINDOW_DRAG;
		GetCursorPos(&mouse.ptWndDragStart);
		GetWindowRect(ghWnd, &mouse.rcWndDragStart);
		SetCapture(ghWnd);
		SetCursor(LoadCursor(NULL,IDC_SIZEALL));
		return 0;
	}
	else if (mouse.state & MOUSE_WINDOW_DRAG)
	{
		if ((messg == WM_LBUTTONUP) || !isPressed(VK_LBUTTON))
		{
			mouse.state &= ~MOUSE_WINDOW_DRAG;
			ReleaseCapture();
		}
		else
		{
			POINT ptCurPos = {};
			GetCursorPos(&ptCurPos);
			RECT rcNew = {
				mouse.rcWndDragStart.left  + (ptCurPos.x - mouse.ptWndDragStart.x),
				mouse.rcWndDragStart.top   + (ptCurPos.y - mouse.ptWndDragStart.y)
			};
			rcNew.right = rcNew.left + (mouse.rcWndDragStart.right - mouse.rcWndDragStart.left);
			rcNew.bottom = rcNew.top + (mouse.rcWndDragStart.bottom - mouse.rcWndDragStart.top);

			OnMoving(&rcNew, true);

			TODO("Desktop mode?");
			MoveWindow(ghWnd, rcNew.left, rcNew.top, rcNew.right-rcNew.left, rcNew.bottom-rcNew.top, TRUE);
			//SetWindowPos(ghWnd, NULL, rcNew.left, rcNew.top, 0,0, SWP_NOSIZE|SWP_NOZORDER);
		}
		return 0;
	}


	// Обработать клики на StatusBar
	if (mp_Status->ProcessStatusMessage(hWnd, messg, wParam, lParam, ptCurClient, lRc))
		return lRc;

	if (!bContinue)
		return 0;

	if (mp_Inside && ((messg == WM_LBUTTONDOWN) || (messg == WM_RBUTTONDOWN)))
	{
		#ifdef _DEBUG
		HWND hFocus = GetFocus();
		HWND hFore = GetForegroundWindow();
		#endif
		apiSetForegroundWindow(ghWnd);
		SetFocus(ghWnd);
	}


	TODO("DoubleView. Хорошо бы колесико мышки перенаправлять в консоль под мышиным курором, а не в активную");
	RECT /*conRect = {0},*/ dcRect = {0};
	//GetWindowRect('ghWnd DC', &dcRect);
	CVirtualConsole* pVCon = NULL;
	CVConGuard VCon;

	if (CVConGroup::GetVConFromPoint(ptCurScreen, &VCon))
		pVCon = VCon.VCon();
	CRealConsole *pRCon = pVCon ? pVCon->RCon() : NULL;
	HWND hView = pVCon ? pVCon->GetView() : NULL;
	if (hView)
	{
		GetWindowRect(hView, &dcRect);
		MapWindowPoints(NULL, ghWnd, (LPPOINT)&dcRect, 2);
	}

	HWND hChild = ::ChildWindowFromPointEx(ghWnd, ptCurClient, CWP_SKIPINVISIBLE|CWP_SKIPDISABLED|CWP_SKIPTRANSPARENT);


	//enum DragPanelBorder dpb = DPB_NONE; //CConEmuMain::CheckPanelDrag(COORD crCon)
	if (((messg == WM_MOUSEWHEEL) || (messg == 0x020E/*WM_MOUSEHWHEEL*/)) && HIWORD(wParam))
	{
		BYTE vk = 0;
		if (messg == WM_MOUSEWHEEL)
			vk =  (((short)(WORD)HIWORD(wParam)) > 0) ? VK_WHEEL_UP : VK_WHEEL_DOWN;
		else if (messg == 0x020E/*WM_MOUSEHWHEEL*/)
			vk =  (((short)(WORD)HIWORD(wParam)) > 0) ? VK_WHEEL_RIGHT : VK_WHEEL_LEFT; // Если MSDN не врет - проверить не на чем

		// Зовем "виртуальное" кнопочное нажатие (если назначено)
		if (vk && ProcessHotKeyMsg(WM_KEYDOWN, vk, 0, NULL, pRCon))
		{
			// назначено, в консоль не пропускаем
			return 0;
		}
	}
	else if (messg >= WM_LBUTTONDOWN && messg <= WM_MBUTTONDBLCLK)
	{
		BYTE vk;
		if (messg >= WM_LBUTTONDOWN && messg <= WM_LBUTTONDBLCLK)
			vk = VK_LBUTTON;
		else if (messg >= WM_RBUTTONDOWN && messg <= WM_RBUTTONDBLCLK)
			vk = VK_RBUTTON;
		else
			vk = VK_MBUTTON;

		UpdateControlKeyState();

		bool bDn = (messg == WM_LBUTTONDOWN || messg == WM_RBUTTONDOWN || messg == WM_MBUTTONDOWN);
		bool bUp = (messg == WM_LBUTTONUP || messg == WM_RBUTTONUP || messg == WM_MBUTTONUP);

		const ConEmuHotKey* pHotKey = gpSetCls->GetHotKeyInfo(VkModFromVk(vk), bDn, pRCon);

		if (pHotKey)
		{
			if ((pHotKey != ConEmuSkipHotKey) && (bDn || bUp))
			{
				// Зовем "виртуальное" кнопочное нажатие
				ProcessHotKeyMsg(bDn ? WM_KEYDOWN : WM_KEYUP, vk, 0, NULL, pRCon);
			}

			// назначено, в консоль не пропускаем
			return 0;
		}
	}

	//BOOL lbMouseWasCaptured = mb_MouseCaptured;
	if (!mb_MouseCaptured)
	{
		// Если клик
		if ((hView && hChild == hView) &&
			(messg == WM_LBUTTONDOWN || messg == WM_RBUTTONDOWN || messg == WM_MBUTTONDOWN
			|| (isPressed(VK_LBUTTON) || isPressed(VK_RBUTTON) || isPressed(VK_MBUTTON))))
		{
			// В клиентской области (области отрисовки)
			//if (hView /*PtInRect(&dcRect, ptCurClient)*/)
			//{
			mb_MouseCaptured = TRUE;
			//TODO("После скрытия ViewPort наверное SetCapture нужно будет делать на ghWnd");
			SetCapture(ghWnd); // 100208 было 'ghWnd DC'
			//}
		}
	}
	else
	{
		// Все кнопки мышки отпущены - release
		if (!isPressed(VK_LBUTTON) && !isPressed(VK_RBUTTON) && !isPressed(VK_MBUTTON))
		{
			ReleaseCapture();
			mb_MouseCaptured = FALSE;

			if (pRCon && pRCon->isMouseButtonDown())
			{
				if (ptCurClient.x < dcRect.left)
					ptCurClient.x = dcRect.left;
				else if (ptCurClient.x >= dcRect.right)
					ptCurClient.x = dcRect.right-1;

				if (ptCurClient.y < dcRect.top)
					ptCurClient.y = dcRect.top;
				else if (ptCurClient.y >= dcRect.bottom)
					ptCurClient.y = dcRect.bottom-1;
			}
		}
	}

	if (!pVCon)
		return 0;

	if (gpSetCls->FontWidth()==0 || gpSetCls->FontHeight()==0)
		return 0;

	if ((messg==WM_LBUTTONUP || messg==WM_MOUSEMOVE) && (gpConEmu->mouse.state & MOUSE_SIZING_DBLCKL))
	{
		if (messg==WM_LBUTTONUP)
			gpConEmu->mouse.state &= ~MOUSE_SIZING_DBLCKL;

		return 0; //2009-04-22 После DblCkl по заголовку в консоль мог проваливаться LBUTTONUP
	}

	//-- Вообще, если курсор над полосой прокрутки - любые события мышки в консоль не нужно слать...
	//-- Но если мышка над скроллом - то обработкой Wheel занимается RealConsole и слать таки нужно...
	//if (messg == WM_MOUSEMOVE)
	if ((messg != WM_MOUSEWHEEL) && (messg != WM_MOUSEHWHEEL))
	{
		if (TrackMouse())
			return 0;
	}

	if (pRCon && pRCon->GuiWnd() && !pRCon->isBufferHeight())
	{
		// Эти сообщения нужно посылать специально, иначе не доходят
		if ((messg == WM_MOUSEWHEEL) || (messg == WM_MOUSEHWHEEL))
		{
			pRCon->PostConsoleMessage(pRCon->GuiWnd(), messg, wParam, lParam);
		}
		return 0;
	}

	if (pRCon && pRCon->isSelectionPresent()
	        && ((wParam & MK_LBUTTON) || messg == WM_LBUTTONUP))
	{
		ptCurClient.x -= dcRect.left; ptCurClient.y -= dcRect.top;
		pRCon->OnMouse(messg, wParam, ptCurClient.x, ptCurClient.y);
		return 0;
	}

	// Иначе в консоль проваливаются щелчки по незанятому полю таба...
	// все, что кроме - считаем кликами и они должны попадать в dcRect
	if (messg != WM_MOUSEMOVE && messg != WM_MOUSEWHEEL && messg != WM_MOUSEHWHEEL)
	{
		if (gpConEmu->mouse.state & MOUSE_SIZING_DBLCKL)
			gpConEmu->mouse.state &= ~MOUSE_SIZING_DBLCKL;

		if (hView == NULL /*!PtInRect(&dcRect, ptCur)*/)
		{
			DEBUGLOGFILE("Click outside of DC");
			return 0;
		}
	}

	// Переводим в клиентские (относительно hView) координаты
	ptCurClient.x -= dcRect.left; ptCurClient.y -= dcRect.top;
	//WARNING("Избавляться от ghConWnd. Должно быть обращение через классы");
	//::GetClientRect(ghConWnd, &conRect);
	COORD cr = pVCon->ClientToConsole(ptCurClient.x,ptCurClient.y);
	short conX = cr.X; //winX/gpSet->Log Font.lfWidth;
	short conY = cr.Y; //winY/gpSet->Log Font.lfHeight;

	if ((messg != WM_MOUSEWHEEL && messg != WM_MOUSEHWHEEL) && (conX<0 || conY<0))
	{
		DEBUGLOGFILE("Mouse outside of upper-left");
		return 0;
	}

	//* ****************************
	//* Если окно ConEmu не в фокусе - не слать к консоль движение мышки,
	//* иначе получается неприятная реакция пунктов меню и т.п.
	//* ****************************
	if (gpSet->isMouseSkipMoving && !isMeForeground(false,false))
	{
		DEBUGLOGFILE("ConEmu is not foreground window, mouse event skipped");
		return 0;
	}

	//* ****************************
	//* После получения WM_MOUSEACTIVATE опционально можно не пропускать
	//* клик в консоль, чтобы при активации ConEmu случайно не задеть
	//* (закрыть или отменить) висящий в FAR диалог
	//* ****************************
	if ((gpConEmu->mouse.nSkipEvents[0] && gpConEmu->mouse.nSkipEvents[0] == messg)
	        || (gpConEmu->mouse.nSkipEvents[1]
	            && (gpConEmu->mouse.nSkipEvents[1] == messg || messg == WM_MOUSEMOVE)))
	{
		if (gpConEmu->mouse.nSkipEvents[0] == messg)
		{
			gpConEmu->mouse.nSkipEvents[0] = 0;
			DEBUGSTRMOUSE(L"Skipping Mouse down\n");
		}
		else if (gpConEmu->mouse.nSkipEvents[1] == messg)
		{
			gpConEmu->mouse.nSkipEvents[1] = 0;
			DEBUGSTRMOUSE(L"Skipping Mouse up\n");
		}

#ifdef _DEBUG
		else if (messg == WM_MOUSEMOVE)
		{
			DEBUGSTRMOUSE(L"Skipping Mouse move\n");
		}

#endif
		DEBUGLOGFILE("ConEmu was not foreground window, mouse activation event skipped");
		return 0;
	}

	//* ****************************
	//* После получения WM_MOUSEACTIVATE и включенном gpSet->isMouseSkipActivation
	//* двойной клик переслать в консоль как одинарный, иначе после активации
	//* мышкой быстрый клик в том же месте будет обработан неправильно
	//* ****************************
	if (mouse.nReplaceDblClk)
	{
		if (!gpSet->isMouseSkipActivation)
		{
			mouse.nReplaceDblClk = 0;
		}
		else
		{
			if (messg == WM_LBUTTONDOWN && mouse.nReplaceDblClk == WM_LBUTTONDBLCLK)
			{
				mouse.nReplaceDblClk = 0;
			}
			else if (messg == WM_RBUTTONDOWN && mouse.nReplaceDblClk == WM_RBUTTONDBLCLK)
			{
				mouse.nReplaceDblClk = 0;
			}
			else if (messg == WM_MBUTTONDOWN && mouse.nReplaceDblClk == WM_MBUTTONDBLCLK)
			{
				mouse.nReplaceDblClk = 0;
			}
			else if (mouse.nReplaceDblClk == messg)
			{
				switch (mouse.nReplaceDblClk)
				{
					case WM_LBUTTONDBLCLK:
						messg = WM_LBUTTONDOWN;
						break;
					case WM_RBUTTONDBLCLK:
						messg = WM_RBUTTONDOWN;
						break;
					case WM_MBUTTONDBLCLK:
						messg = WM_MBUTTONDOWN;
						break;
				}

				mouse.nReplaceDblClk = 0;
			}
		}
	}

	// Forwarding сообщений в нить драга
	if (mp_DragDrop && mp_DragDrop->IsDragStarting())
	{
		if (mp_DragDrop->ForwardMessage(messg, wParam, lParam))
			return 0;
	}

	// -- поскольку выделялка у нас теперь своя - этого похоже не требуется. лишние движения окна
	// -- вызывают side-effects на некоторых системах. Issue 274: Окно реальной консоли позиционируется в неудобном месте
	/////*&& isPressed(VK_LBUTTON)*/) && // Если этого не делать - при выделении мышкой консоль может самопроизвольно прокрутиться
	//// Только при скрытой консоли. Иначе мы ConEmu видеть вообще не будем
	//if (pRCon && (/*pRCon->isBufferHeight() ||*/ !pRCon->isWindowVisible())
	//    && (messg == WM_LBUTTONDOWN || messg == WM_RBUTTONDOWN || messg == WM_LBUTTONDBLCLK || messg == WM_RBUTTONDBLCLK
	//        || (messg == WM_MOUSEMOVE && isPressed(VK_LBUTTON)))
	//    )
	//{
	//   // buffer mode: cheat the console window: adjust its position exactly to the cursor
	//   RECT win;
	//   GetWindowRect(ghWnd, &win);
	//   short x = win.left + ptCurClient.x - MulDiv(ptCurClient.x, conRect.right, klMax<uint>(1, pVCon->Width));
	//   short y = win.top + ptCurClient.y - MulDiv(ptCurClient.y, conRect.bottom, klMax<uint>(1, pVCon->Height));
	//   RECT con;
	//   GetWindowRect(ghConWnd, &con);
	//   if (con.left != x || con.top != y)
	//       MOVEWINDOW(ghConWnd, x, y, con.right - con.left + 1, con.bottom - con.top + 1, TRUE);
	//}

	if (!CVConGroup::isFar())
	{
		if (messg != WM_MOUSEMOVE) { DEBUGLOGFILE("FAR not active, all clicks forced to console"); }

		goto fin;
	}

	// Для этих сообщений, lParam - relative to the upper-left corner of the screen.
	if (messg == WM_MOUSEWHEEL || messg == WM_MOUSEHWHEEL)
		lParam = MAKELONG((short)ptCurScreen.x, (short)ptCurScreen.y);
	else
		lParam = MAKELONG((short)ptCurClient.x, (short)ptCurClient.y);

	// Запомним последнее мышиное событие (для PatchMouseEvent)
	mouse.lastMsg = messg;

	// Теперь можно обрабатывать мышку, и если нужно - слать ее в консоль
	if (messg == WM_MOUSEMOVE)
	{
		// ptCurClient уже преобразован в клиентские (относительно hView) координаты
		if (!OnMouse_Move(pVCon, hWnd, WM_MOUSEMOVE, wParam, lParam, ptCurClient, cr))
			return 0;
	}
	else
	{
		mouse.lastMMW=-1; mouse.lastMML=-1;

		if (messg == WM_LBUTTONDBLCLK)
		{
			// ptCurClient уже преобразован в клиентские (относительно hView) координаты
			if (!OnMouse_LBtnDblClk(pVCon, hWnd, messg, wParam, lParam, ptCurClient, cr))
				return 0;
		} // !!! Без else, т.к. теоретически функция может заменить клик на одинарный

		if (messg == WM_RBUTTONDBLCLK)
		{
			// ptCurClient уже преобразован в клиентские (относительно hView) координаты
			if (!OnMouse_RBtnDblClk(pVCon, hWnd, messg, wParam, lParam, ptCurClient, cr))
				return 0;
		} // !!! Без else, т.к. функция может заменить клик на одинарный

		// Теперь обрабатываем что осталось
		if (messg == WM_LBUTTONDOWN)
		{
			// ptCurClient уже преобразован в клиентские (относительно hView) координаты
			if (!OnMouse_LBtnDown(pVCon, hWnd, WM_LBUTTONDOWN, wParam, lParam, ptCurClient, cr))
				return 0;
		}
		else if (messg == WM_LBUTTONUP)
		{
			// ptCurClient уже преобразован в клиентские (относительно hView) координаты
			if (!OnMouse_LBtnUp(pVCon, hWnd, WM_LBUTTONUP, wParam, lParam, ptCurClient, cr))
				return 0;
		}
		else if (messg == WM_RBUTTONDOWN)
		{
			// ptCurClient уже преобразован в клиентские (относительно hView) координаты
			if (!OnMouse_RBtnDown(pVCon, hWnd, WM_RBUTTONDOWN, wParam, lParam, ptCurClient, cr))
				return 0;
		}
		else if (messg == WM_RBUTTONUP)
		{
			// ptCurClient уже преобразован в клиентские (относительно hView) координаты
			if (!OnMouse_RBtnUp(pVCon, hWnd, WM_RBUTTONUP, wParam, lParam, ptCurClient, cr))
				return 0;
		}
	}

#ifdef MSGLOGGER

	if (messg == WM_MOUSEMOVE)
		messg = WM_MOUSEMOVE;

#endif
fin:
	// Сюда мы попадаем если ни одна из функций (OnMouse_xxx) не запретила пересылку в консоль
	// иначе после (например) RBtnDown в консоль может СРАЗУ провалиться MOUSEMOVE
	// ptCurClient уже преобразован в клиентские (относительно hView) координаты
	mouse.lastMMW=wParam; mouse.lastMML=MAKELPARAM(ptCurClient.x, ptCurClient.y);
	
	// LClick из PanelViews пропускать в консоль нельзя, чтобы не возникало глюков с позиционированием по клику...
	if (!isPrivate)
	{
		// Теперь осталось послать событие в консоль
		if (pRCon)
			pRCon->OnMouse(messg, wParam, ptCurClient.x, ptCurClient.y);
	}

	return 0;
}

LRESULT CConEmuMain::OnMouse_Move(CVirtualConsole* pVCon, HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr)
{
	// WM_MOUSEMOVE вроде бы слишком часто вызывается даже при условии что курсор не двигается...
	if (wParam==mouse.lastMMW && lParam==mouse.lastMML
	        && (mouse.state & MOUSE_DRAGPANEL_ALL) == 0)
	{
		DEBUGSTRPANEL2(L"PanelDrag: Skip of wParam==mouse.lastMMW && lParam==mouse.lastMML\n");
		return 0;
	}

	mouse.lastMMW=wParam; mouse.lastMML=lParam;

	mp_Find->UpdateFindDlgAlpha();

	// мог не сброситься, проверим
	isSizing();

	// 18.03.2009 Maks - Если уже тащим - мышь не слать
	if (isDragging())
	{
		// может флажок случайно остался?
		if ((mouse.state & DRAG_L_STARTED) && ((wParam & MK_LBUTTON)==0))
			mouse.state &= ~(DRAG_L_STARTED | DRAG_L_ALLOWED);

		if ((mouse.state & DRAG_R_STARTED) && ((wParam & MK_RBUTTON)==0))
			mouse.state &= ~(DRAG_R_STARTED | DRAG_R_ALLOWED);

		// Случайно остался?
		mouse.state &= ~MOUSE_DRAGPANEL_ALL;

		if (mouse.state & (DRAG_L_STARTED | DRAG_R_STARTED))
		{
			DEBUGSTRPANEL2(L"PanelDrag: Skip of isDragging\n");
			return 0;
		}
	}
	else if ((mouse.state & (DRAG_L_ALLOWED | DRAG_R_ALLOWED)) != 0)
	{
		if (isConSelectMode())
		{
			mouse.state &= ~(DRAG_L_STARTED | DRAG_L_ALLOWED | DRAG_R_STARTED | DRAG_R_ALLOWED | MOUSE_DRAGPANEL_ALL);
		}
	}

	if (mouse.state & MOUSE_DRAGPANEL_ALL)
	{
		if (!gpSet->isDragPanel)
		{
			mouse.state &= ~MOUSE_DRAGPANEL_ALL;
			DEBUGSTRPANEL2(L"PanelDrag: Skip of isDragPanel==0\n");
		}
		else
		{
			if (!isPressed(VK_LBUTTON))
			{
				mouse.state &= ~MOUSE_DRAGPANEL_ALL;
				DEBUGSTRPANEL2(L"PanelDrag: Skip of LButton not pressed\n");
				return 0;
			}

			if (cr.X == mouse.LClkCon.X && cr.Y == mouse.LClkCon.Y)
			{
				DEBUGSTRPANEL2(L"PanelDrag: Skip of cr.X == mouse.LClkCon.X && cr.Y == mouse.LClkCon.Y\n");
				return 0;
			}

			if (gpSet->isDragPanel == 1)
				OnSizePanels(cr);

			// Мышь не слать...
			return 0;
		}
	}

	//TODO: вроде бы иногда isSizing() не сбрасывается?
	//? может так: if (gpSet->isDragEnabled & mouse.state)
	if (gpSet->isDragEnabled & ((mouse.state & (DRAG_L_ALLOWED|DRAG_R_ALLOWED))!=0)
	        && !isPictureView())
	{
		if (mp_DragDrop==NULL)
		{
			DebugStep(_T("DnD: Drag-n-Drop is null"));
		}
		else
		{
			mouse.bIgnoreMouseMove = true;
			BOOL lbDiffTest = PTDIFFTEST(cursor,DRAG_DELTA); // TRUE - если курсор НЕ двигался (далеко)

			if (!lbDiffTest && !isDragging() && !isSizing())
			{
				// 2009-06-19 Выполнялось сразу после "if (gpSet->isDragEnabled & (mouse.state & (DRAG_L_ALLOWED|DRAG_R_ALLOWED)))"
				if (mouse.state & MOUSE_R_LOCKED)
				{
					// чтобы при RightUp не ушел APPS
					mouse.state &= ~MOUSE_R_LOCKED;
					//PtDiffTest(C, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), D)
					char szLog[255];
					wsprintfA(szLog, "Right drag started, MOUSE_R_LOCKED cleared: cursor={%i-%i}, Rcursor={%i-%i}, Now={%i-%i}, MinDelta=%i",
					          (int)cursor.x, (int)cursor.y,
					          (int)Rcursor.x, (int)Rcursor.y,
					          (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam),
					          (int)DRAG_DELTA);
					LogString(szLog);
				}

				BOOL lbLeftDrag = (mouse.state & DRAG_L_ALLOWED) == DRAG_L_ALLOWED;
				LogString(lbLeftDrag ? "Left drag about to start" : "Right drag about to start");
				// Если сначала фокус был на файловой панели, но после LClick он попал на НЕ файловую - отменить ShellDrag
				bool bFilePanel = CVConGroup::isFilePanel();

				if (!bFilePanel)
				{
					DebugStep(_T("DnD: not file panel"));
					//isLBDown = false;
					mouse.state &= ~(DRAG_L_ALLOWED | DRAG_L_STARTED | DRAG_R_ALLOWED | DRAG_R_STARTED);
					mouse.bIgnoreMouseMove = false;
					//POSTMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( newX, newY ), TRUE);     //посылаем консоли отпускание
					pVCon->RCon()->OnMouse(WM_LBUTTONUP, wParam, ptCur.x, ptCur.x);
					//ReleaseCapture(); --2009-03-14
					return 0;
				}

				// Чтобы сам фар не дергался на MouseMove...
				//isLBDown = false;
				mouse.state &= ~(DRAG_L_ALLOWED | DRAG_L_STARTED | DRAG_R_STARTED); // флажок поставит сам CDragDrop::Drag() по наличию DRAG_R_ALLOWED
				// вроде валится, если перед Drag
				//POSTMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( newX, newY ), TRUE);     //посылаем консоли отпускание

				//TODO("Тут бы не посылать мышь в очередь, а передавать через аргумент команды GetDragInfo в плагин");
				//if (lbLeftDrag) { // сразу "отпустить" клавишу
				//	//POSTMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( mouse.LClkCon.X, mouse.LClkCon.Y ), TRUE);     //посылаем консоли отпускание
				//	pVCon->RCon()->OnMouse(WM_LBUTTONUP, wParam, mouse.LClkDC.X, mouse.LClkDC.Y);
				//} else {
				//	//POSTMESSAGE(ghConWnd, WM_LBUTTONDOWN, wParam, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);     //посылаем консоли отпускание
				//	pVCon->RCon()->OnMouse(WM_LBUTTONDOWN, wParam, mouse.RClkDC.X, mouse.RClkDC.Y);
				//	//POSTMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);     //посылаем консоли отпускание
				//	pVCon->RCon()->OnMouse(WM_LBUTTONUP, wParam, mouse.RClkDC.X, mouse.RClkDC.Y);
				//}
				//pVCon->RCon()->FlushInputQueue();

				// Иначе иногда срабатывает FAR'овский D'n'D
				//SENDMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( newX, newY ));     //посылаем консоли отпускание
				if (mp_DragDrop)
				{
					//COORD crMouse = pVCon->ClientToConsole(
					//	lbLeftDrag ? mouse.LClkDC.X : mouse.RClkDC.X,
					//	lbLeftDrag ? mouse.LClkDC.Y : mouse.RClkDC.Y);
					DebugStep(_T("DnD: Drag-n-Drop starting"));
					mp_DragDrop->Drag(!lbLeftDrag, lbLeftDrag ? mouse.LClkDC : mouse.RClkDC);
					DebugStep(Title); // вернуть заголовок
				}
				else
				{
					_ASSERTE(mp_DragDrop); // должно быть обработано выше
					DebugStep(_T("DnD: Drag-n-Drop is null"));
				}

				mouse.bIgnoreMouseMove = false;
				//#ifdef NEWMOUSESTYLE
				//newX = cursor.x; newY = cursor.y;
				//#else
				//newX = MulDiv(cursor.x, conRect.right, klMax<uint>(1, pVCon->Width));
				//newY = MulDiv(cursor.y, conRect.bottom, klMax<uint>(1, pVCon->Height));
				//#endif
				//if (lbLeftDrag)
				//  POSTMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( newX, newY ), TRUE);     //посылаем консоли отпускание
				//isDragProcessed=false; -- убрал, иначе при бросании в пассивную панель больших файлов дроп может вызваться еще раз???
				return 0;
			}
		}
	}
	else if (!gpSet->isDisableMouse && gpSet->isRClickSendKey && (mouse.state & MOUSE_R_LOCKED))
	{
		//Если двинули мышкой, а была включена опция RClick - не вызывать
		//контекстное меню - просто послать правый клик
		if (!PTDIFFTEST(Rcursor, RCLICKAPPSDELTA))
		{
			//isRBDown=false;
			mouse.state &= ~(DRAG_R_ALLOWED | DRAG_R_STARTED | MOUSE_R_LOCKED);

			if (mb_RightClickingPaint)
			{
				// Если начали рисовать Waiting вокруг курсора
				StopRightClickingPaint();
				_ASSERTE(mb_RightClickingPaint==FALSE);
			}

			char szLog[255];
			wsprintfA(szLog, "Mouse was moved, MOUSE_R_LOCKED cleared: Rcursor={%i-%i}, Now={%i-%i}, MinDelta=%i",
			          (int)Rcursor.x, (int)Rcursor.y,
			          (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam),
			          (int)RCLICKAPPSDELTA);
			LogString(szLog);
			//POSTMESSAGE(ghConWnd, WM_RBUTTONDOWN, 0, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);
			pVCon->RCon()->OnMouse(WM_RBUTTONDOWN, 0, mouse.RClkDC.X, mouse.RClkDC.Y);
		}

		return 0;
	}

	/*if (!isRBDown && (wParam==MK_RBUTTON)) {
	// Чтобы при выделении правой кнопкой файлы не пропускались
	if ((newY-RBDownNewY)>5) {// пока попробуем для режима сверху вниз
	for (short y=RBDownNewY;y<newY;y+=5)
	POSTMESSAGE(ghConWnd, WM_MOUSEMOVE, wParam, MAKELPARAM( RBDownNewX, y ), TRUE);
	}
	RBDownNewX=newX; RBDownNewY=newY;
	}*/

	if (mouse.bIgnoreMouseMove)
		return 0;

	return TRUE; // переслать в консоль
}

LRESULT CConEmuMain::OnMouse_LBtnDown(CVirtualConsole* pVCon, HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr)
{
	enum DragPanelBorder dpb = DPB_NONE;
	//if (isLBDown()) ReleaseCapture(); // Вдруг сглючило? --2009-03-14
	//isLBDown = false;
	mouse.state &= ~(DRAG_L_ALLOWED | DRAG_L_STARTED | MOUSE_DRAGPANEL_ALL);
	mouse.bIgnoreMouseMove = false;
	mouse.LClkCon = cr;
	mouse.LClkDC = MakeCoord(ptCur.x,ptCur.y);
	CRealConsole *pRCon = pVCon->RCon();

	if (!pRCon)  // если консоли нет - то и слать некуда
		return 0;

	dpb = CConEmuMain::CheckPanelDrag(cr);

	if (dpb != DPB_NONE)
	{
		if (dpb == DPB_SPLIT)
			mouse.state |= MOUSE_DRAGPANEL_SPLIT;
		else if (dpb == DPB_LEFT)
			mouse.state |= MOUSE_DRAGPANEL_LEFT;
		else if (dpb == DPB_RIGHT)
			mouse.state |= MOUSE_DRAGPANEL_RIGHT;

		// Если зажат шифт - в FAR2 меняется высота активной панели
		if (isPressed(VK_SHIFT))
		{
			mouse.state |= MOUSE_DRAGPANEL_SHIFT;

			if (dpb == DPB_LEFT)
			{
				PostMacro(L"@$If (!APanel.Left) Tab $End");
			}
			else if (dpb == DPB_RIGHT)
			{
				PostMacro(L"@$If (APanel.Left) Tab $End");
			}
		}

		// LBtnDown в консоль не посылаем, но попробуем послать MouseMove?
		// (иначе начинается драка с PanelTabs - он перехватывает и буферизирует всю клавиатуру)
		INPUT_RECORD r = {MOUSE_EVENT};
		r.Event.MouseEvent.dwMousePosition = mouse.LClkCon;
		r.Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
		pVCon->RCon()->PostConsoleEvent(&r);
		return 0;
	}

	if (!isConSelectMode() && CVConGroup::isFilePanel() && pVCon &&
	        pVCon->RCon()->CoordInPanel(mouse.LClkCon))
	{
		//SetCapture('ghWnd DC'); --2009-03-14
		cursor.x = (int)(short)LOWORD(lParam);
		cursor.y = (int)(short)HIWORD(lParam);
		//isLBDown=true;
		//isDragProcessed=false;
		CONSOLE_CURSOR_INFO ci;
		pVCon->RCon()->GetConsoleCursorInfo(&ci);

		if ((ci.bVisible && ci.dwSize>0)  // курсор должен быть видим, иначе это чье-то меню
		        && gpSet->isDragEnabled & DRAG_L_ALLOWED)
		{
			//if (!gpSet->nLDragKey || isPressed(gpSet->nLDragKey))
			// функция проверит нажат ли nLDragKey (допускает nLDragKey==0), а другие - не нажаты
			// то есть нажат SHIFT(==nLDragKey), а CTRL & ALT - НЕ нажаты
			if (gpSet->IsModifierPressed(vkLDragKey, true))
			{
				mouse.state = DRAG_L_ALLOWED;
			}
		}

		// иначе после LBtnDown в консоль может СРАЗУ провалиться MOUSEMOVE
		mouse.lastMMW=wParam; mouse.lastMML=lParam;
		//if (gpSet->is DnD) mouse.bIgnoreMouseMove = true;
		//POSTMESSAGE(ghConWnd, messg, wParam, MAKELPARAM( newX, newY ), FALSE); // было SEND
		pVCon->RCon()->OnMouse(messg, wParam, ptCur.x, ptCur.y);
		return 0;
	}

	return TRUE; // переслать в консоль
}
LRESULT CConEmuMain::OnMouse_LBtnUp(CVirtualConsole* pVCon, HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr)
{
	BOOL lbLDrag = (mouse.state & DRAG_L_STARTED) == DRAG_L_STARTED;

	if (mouse.state & MOUSE_DRAGPANEL_ALL)
	{
		if (gpSet->isDragPanel == 2)
		{
			OnSizePanels(cr);
		}

		mouse.state &= ~(DRAG_L_ALLOWED | DRAG_L_STARTED | MOUSE_DRAGPANEL_ALL);
		// Мышь не слать...
		return 0;
	}

	mouse.state &= ~(DRAG_L_ALLOWED | DRAG_L_STARTED | MOUSE_DRAGPANEL_ALL);

	if (lbLDrag)
		return 0; // кнопка уже "отпущена"

	if (mouse.bIgnoreMouseMove)
	{
		mouse.bIgnoreMouseMove = false;
		//#ifdef NEWMOUSESTYLE
		//newX = cursor.x; newY = cursor.y;
		//#else
		//newX = MulDiv(cursor.x, conRect.right, klMax<uint>(1, pVCon->Width));
		//newY = MulDiv(cursor.y, conRect.bottom, klMax<uint>(1, pVCon->Height));
		//#endif
		//POSTMESSAGE(ghConWnd, messg, wParam, MAKELPARAM( newX, newY ), FALSE);
		pVCon->RCon()->OnMouse(messg, wParam, cursor.x, cursor.y);
		return 0;
	}

	return TRUE; // переслать в консоль
}
LRESULT CConEmuMain::OnMouse_LBtnDblClk(CVirtualConsole* pVCon, HWND hWnd, UINT& messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr)
{
	CRealConsole *pRCon = pVCon->RCon();

	if (!pRCon)  // Если консоли нет - то слать некуда
		return 0;

	enum DragPanelBorder dpb = CConEmuMain::CheckPanelDrag(cr);

	if (dpb != DPB_NONE)
	{
		wchar_t szMacro[128]; szMacro[0] = 0;

		bool lbIsLua = pRCon->IsFarLua();

		if (dpb == DPB_SPLIT)
		{
			RECT rcRight; pRCon->GetPanelRect(TRUE, &rcRight, TRUE);
			int  nCenter = pRCon->TextWidth() / 2;

			if (lbIsLua)
			{
				if (nCenter < rcRight.left)
					_wsprintf(szMacro, SKIPLEN(countof(szMacro)) L"for RCounter= %i,1,-1 do Keys(\"CtrlLeft\") end", rcRight.left - nCenter);
				else if (nCenter > rcRight.left)
					_wsprintf(szMacro, SKIPLEN(countof(szMacro)) L"for RCounter= %i,1,-1 do Keys(\"CtrlRight\") end", nCenter - rcRight.left);
			}
			else
			{
				if (nCenter < rcRight.left)
					_wsprintf(szMacro, SKIPLEN(countof(szMacro)) L"@$Rep (%i) CtrlLeft $End", rcRight.left - nCenter);
				else if (nCenter > rcRight.left)
					_wsprintf(szMacro, SKIPLEN(countof(szMacro)) L"@$Rep (%i) CtrlRight $End", nCenter - rcRight.left);
			}
		}
		else
		{
			if (lbIsLua)
			{
				wsprintf(szMacro+1, L"for RCounter= %i,1,-1 do Keys(\"CtrlDown\") end", pRCon->TextHeight());
			}
			else
			{
				_wsprintf(szMacro, SKIPLEN(countof(szMacro)) L"@$Rep (%i) CtrlDown $End", pRCon->TextHeight());
			}
		}

		if (szMacro[0])
			pRCon->PostMacro(szMacro);

		return 0;
	}

	return TRUE; // переслать в консоль
}
LRESULT CConEmuMain::OnMouse_RBtnDown(CVirtualConsole* pVCon, HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr)
{
	Rcursor = ptCur;
	mouse.RClkCon = cr;
	mouse.RClkDC = MakeCoord(ptCur.x,ptCur.y);
	//isRBDown=false;
	mouse.state &= ~(DRAG_R_ALLOWED | DRAG_R_STARTED | MOUSE_R_LOCKED);
	mouse.bIgnoreMouseMove = false;

	if (gpSetCls->isAdvLogging)
	{
		char szLog[100];
		wsprintfA(szLog, "Right button down: Rcursor={%i-%i}", (int)Rcursor.x, (int)Rcursor.y);
		LogString(szLog);
	}

	//if (isFilePanel()) // Maximus5
	bool bSelect = false, bPanel = false, bActive = false, bCoord = false;

	if (!(bSelect = isConSelectMode())
	        && (bPanel = CVConGroup::isFilePanel())
	        && (bActive = (pVCon != NULL))
	        && (bCoord = pVCon->RCon()->CoordInPanel(mouse.RClkCon)))
	{
		if (gpSet->isDragEnabled & DRAG_R_ALLOWED)
		{
			//if (!gpSet->nRDragKey || isPressed(gpSet->nRDragKey)) {
			// функция проверит нажат ли nRDragKey (допускает nRDragKey==0), а другие - не нажаты
			// то есть нажат SHIFT(==nRDragKey), а CTRL & ALT - НЕ нажаты
			if (gpSet->IsModifierPressed(vkRDragKey, true))
			{
				mouse.state = DRAG_R_ALLOWED;

				if (gpSetCls->isAdvLogging) LogString("RightClick ignored of gpSet->nRDragKey pressed");

				return 0;
			}
		}

		// Если ничего лишнего не нажато!
		if (!gpSet->isDisableMouse && gpSet->isRClickSendKey && !(wParam&(MK_CONTROL|MK_LBUTTON|MK_MBUTTON|MK_SHIFT|MK_XBUTTON1|MK_XBUTTON2)))
		{
			//заведем таймер на .3
			//если больше - пошлем apps
			mouse.state |= MOUSE_R_LOCKED; mouse.bSkipRDblClk = false;
			char szLog[100];
			wsprintfA(szLog, "MOUSE_R_LOCKED was set: Rcursor={%i-%i}", (int)Rcursor.x, (int)Rcursor.y);
			LogString(szLog);
			mouse.RClkTick = TimeGetTime(); //GetTickCount();
			StartRightClickingPaint();
			return 0;
		}
		else
		{
			if (gpSetCls->isAdvLogging)
				LogString(
					gpSet->isDisableMouse ? "RightClick ignored of gpSet->isDisableMouse" :
				    !gpSet->isRClickSendKey ? "RightClick ignored of !gpSet->isRClickSendKey" :
				    "RightClick ignored of wParam&(MK_CONTROL|MK_LBUTTON|MK_MBUTTON|MK_SHIFT|MK_XBUTTON1|MK_XBUTTON2)"
				);
		}
	}
	else
	{
		if (gpSetCls->isAdvLogging)
			LogString(
			    bSelect ? "RightClick ignored of isConSelectMode" :
			    !bPanel ? "RightClick ignored of NOT isFilePanel" :
			    !bActive ? "RightClick ignored of NOT isFilePanel" :
			    !bCoord ? "RightClick ignored of NOT isFilePanel" :
			    "RightClick ignored, unknown cause"
			);
	}

	return TRUE; // переслать в консоль
}
LRESULT CConEmuMain::OnMouse_RBtnUp(CVirtualConsole* pVCon, HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr)
{
	// Сразу сбросить, если выставлялся
	StopRightClickingPaint();

	if (!gpSet->isDisableMouse && gpSet->isRClickSendKey && (mouse.state & MOUSE_R_LOCKED))
	{
		//isRBDown=false; // сразу сбросим!
		mouse.state &= ~(DRAG_R_ALLOWED | DRAG_R_STARTED | MOUSE_R_LOCKED);

		if (PTDIFFTEST(Rcursor,RCLICKAPPSDELTA))
		{
			//держали зажатой <.3
			//убьем таймер, кликнием правой кнопкой
			//KillTimer(hWnd, 1); -- Maximus5, таймер более не используется
			DWORD dwCurTick = TimeGetTime(); //GetTickCount();
			DWORD dwDelta=dwCurTick-mouse.RClkTick;

			// Если держали дольше .3с, но не слишком долго :)
			if ((gpSet->isRClickSendKey==1)
				|| (dwDelta>RCLICKAPPSTIMEOUT/*.3сек*/ && dwDelta<RCLICKAPPSTIMEOUT_MAX/*10000*/)
				// Или в режиме тачскрина - длинный _тап_
				|| (gpSet->isRClickTouchInvert() && (dwDelta<RCLICKAPPSTIMEOUT))
				)
			{
				if (!mb_RightClickingLSent && pVCon)
				{
					mb_RightClickingLSent = TRUE;
					// Чтобы установить курсор в панелях точно под кликом
					// иначе получается некрасиво, что курсор прыгает только перед
					// появлением EMenu, а до этого (пока крутится "кружок") курсор не двигается.
					pVCon->RCon()->PostLeftClickSync(mouse.RClkDC);
					//apVCon->RCon()->OnMouse(WM_MOUSEMOVE, 0, mouse.RClkDC.X, mouse.RClkDC.Y, true);
					//WARNING("По хорошему, нужно дождаться пока мышь обработается");
					//apVCon->RCon()->PostMacro(L"MsLClick");
					//WARNING("!!! Заменить на CMD_LEFTCLKSYNC?");
				}

				// А теперь можно и Apps нажать
				mouse.bSkipRDblClk=true; // чтобы пока FAR думает в консоль не проскочило мышиное сообщение
				//POSTMESSAGE(ghConWnd, WM_KEYDOWN, VK_APPS, 0, TRUE);
				DWORD dwFarPID = pVCon->RCon()->GetFarPID();

				if (dwFarPID)
				{
					AllowSetForegroundWindow(dwFarPID);
					COORD crMouse = pVCon->RCon()->ScreenToBuffer(
					                    pVCon->ClientToConsole(mouse.RClkDC.X, mouse.RClkDC.Y)
					                );
					CConEmuPipe pipe(GetFarPID(), CONEMUREADYTIMEOUT);

					if (pipe.Init(_T("CConEmuMain::EMenu"), TRUE))
					{
						//DWORD cbWritten=0;
						DebugStep(_T("EMenu: Waiting for result (10 sec)"));
						int nLen = 0;
						int nSize = sizeof(crMouse) + sizeof(wchar_t);

						if (gpSet->sRClickMacro && *gpSet->sRClickMacro)
						{
							nLen = _tcslen(gpSet->sRClickMacro);
							nSize += nLen*2;
							// -- заменено на перехват функции ScreenToClient
							//pVCon->RCon()->RemoveFromCursor();
						}

						LPBYTE pcbData = (LPBYTE)calloc(nSize,1);
						_ASSERTE(pcbData);
						memmove(pcbData, &crMouse, sizeof(crMouse));

						if (nLen)
							lstrcpy((wchar_t*)(pcbData+sizeof(crMouse)), gpSet->sRClickMacro);

						if (!pipe.Execute(CMD_EMENU, pcbData, nSize))
						{
							LogString("RightClicked, but pipe.Execute(CMD_EMENU) failed");
						}
						else
						{
							// OK
							if (gpSetCls->isAdvLogging)
							{
								char szInfo[255] = {0};
								lstrcpyA(szInfo, "RightClicked, pipe.Execute(CMD_EMENU) OK");

								if (gpSet->sRClickMacro && *gpSet->sRClickMacro)
								{
									lstrcatA(szInfo, ", Macro: "); int nLen = lstrlenA(szInfo);
									WideCharToMultiByte(CP_ACP,0, gpSet->sRClickMacro,-1, szInfo+nLen, countof(szInfo)-nLen-1, 0,0);
								}
								else
								{
									lstrcatA(szInfo, ", NoMacro");
								}

								LogString(szInfo);
							}
						}

						DebugStep(NULL);
						free(pcbData);
					}
					else
					{
						LogString("RightClicked, but pipe.Init() failed");
					}

					//INPUT_RECORD r = {KEY_EVENT};
					////pVCon->RCon()->On Keyboard(ghConWnd, WM_KEYDOWN, VK_APPS, (VK_APPS << 16) | (1 << 24));
					////pVCon->RCon()->On Keyboard(ghConWnd, WM_KEYUP, VK_APPS, (VK_APPS << 16) | (1 << 24));
					//r.Event.KeyEvent.bKeyDown = TRUE;
					//r.Event.KeyEvent.wVirtualKeyCode = VK_APPS;
					//r.Event.KeyEvent.wVirtualScanCode = /*28 на моей клавиатуре*/MapVirtualKey(VK_APPS, 0/*MAPVK_VK_TO_VSC*/);
					//r.Event.KeyEvent.dwControlKeyState = 0x120;
					//pVCon->RCon()->PostConsoleEvent(&r);
					////On Keyboard(hConWnd, WM_KEYUP, VK_RETURN, 0);
					//r.Event.KeyEvent.bKeyDown = FALSE;
					//r.Event.KeyEvent.dwControlKeyState = 0x120;
					//pVCon->RCon()->PostConsoleEvent(&r);
				}
				else
				{
					LogString("RightClicked, but FAR PID is 0");
				}

				return 0;
			}
			else
			{
				char szLog[255];
				// if ((gpSet->isRClickSendKey==1) || (dwDelta>RCLICKAPPSTIMEOUT && dwDelta<10000))
				lstrcpyA(szLog, "RightClicked, but condition failed: ");

				if (gpSet->isRClickSendKey!=1)
				{
					wsprintfA(szLog+lstrlenA(szLog), "((isRClickSendKey=%i)!=1)", (UINT)gpSet->isRClickSendKey);
				}
				else
				{
					wsprintfA(szLog+lstrlenA(szLog), "(isRClickSendKey==%i)", (UINT)gpSet->isRClickSendKey);
				}

				if (dwDelta <= RCLICKAPPSTIMEOUT)
				{
					wsprintfA(szLog+lstrlenA(szLog), ", ((Delay=%i)<=%i)", dwDelta, (int)RCLICKAPPSTIMEOUT);
				}
				else if (dwDelta >= 10000)
				{
					wsprintfA(szLog+lstrlenA(szLog), ", ((Delay=%i)>=10000)", dwDelta);
				}
				else
				{
					wsprintfA(szLog+lstrlenA(szLog), ", (Delay==%i)", dwDelta);
				}

				LogString(szLog);
			}
		}
		else
		{
			char szLog[100];
			wsprintfA(szLog, "RightClicked, but mouse was moved abs({%i-%i}-{%i-%i})>%i", Rcursor.x, Rcursor.y, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (int)RCLICKAPPSDELTA);
			LogString(szLog);
		}

		// иначе после RBtnDown в консоль может СРАЗУ провалиться MOUSEMOVE
		mouse.lastMMW=MK_RBUTTON|wParam; mouse.lastMML=lParam; // добавляем, т.к. мы в RButtonUp
		// Иначе нужно сначала послать WM_RBUTTONDOWN
		//POSTMESSAGE(ghConWnd, WM_RBUTTONDOWN, wParam, MAKELPARAM( newX, newY ), TRUE);
		pVCon->RCon()->OnMouse(WM_RBUTTONDOWN, wParam, ptCur.x, ptCur.y);
	}
	else
	{
		char szLog[255];
		// if (gpSet->isRClickSendKey && (mouse.state & MOUSE_R_LOCKED))
		//wsprintfA(szLog, "RightClicked, but condition failed (RCSK:%i, State:%u)", (int)gpSet->isRClickSendKey, (DWORD)mouse.state);
		lstrcpyA(szLog, "RightClicked, but condition failed: ");

		if (gpSet->isDisableMouse)
		{
			wsprintfA(szLog+lstrlenA(szLog), "((isDisableMouse=%i)==0)", (UINT)gpSet->isDisableMouse);
		}
		if (gpSet->isRClickSendKey==0)
		{
			wsprintfA(szLog+lstrlenA(szLog), "((isRClickSendKey=%i)==0)", (UINT)gpSet->isRClickSendKey);
		}
		else
		{
			wsprintfA(szLog+lstrlenA(szLog), "(isRClickSendKey==%i)", (UINT)gpSet->isRClickSendKey);
		}

		if ((mouse.state & MOUSE_R_LOCKED) == 0)
		{
			wsprintfA(szLog+lstrlenA(szLog), ", (((state=0x%X)&MOUSE_R_LOCKED)==0)", (DWORD)mouse.state);
		}
		else
		{
			wsprintfA(szLog+lstrlenA(szLog), ", (state==0x%X)", (DWORD)mouse.state);
		}

		LogString(szLog);
	}

	//isRBDown=false; // чтобы не осталось случайно
	mouse.state &= ~(DRAG_R_ALLOWED | DRAG_R_STARTED | MOUSE_R_LOCKED);
	return TRUE; // переслать в консоль
}
LRESULT CConEmuMain::OnMouse_RBtnDblClk(CVirtualConsole* pVCon, HWND hWnd, UINT& messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr)
{
	if (mouse.bSkipRDblClk)
	{
		mouse.bSkipRDblClk = false;
		return 0; // не обрабатывать, сейчас висит контекстное меню
	}

	//if (gpSet->isRClickSendKey) {
	//	// Заменить на одинарный клик, иначе может не подняться контекстное меню
	//	messg = WM_RBUTTONDOWN;
	//} -- хотя, раньше это всегда и делалось в любом случае...
	messg = WM_RBUTTONDOWN;
	return TRUE; // переслать в консоль
}

BOOL CConEmuMain::OnMouse_NCBtnDblClk(HWND hWnd, UINT& messg, WPARAM wParam, LPARAM lParam)
{
	// По DblClick на рамке - развернуть окно по горизонтали (вертикали) на весь монитор
	if (wParam == HTLEFT || wParam == HTRIGHT || wParam == HTTOP || wParam == HTBOTTOM)
	{
		if (WindowMode != wmNormal)
		{
			// Так быть не должно - рамка не должна быть видна в этих режимах
			// Для Quake - игнорировать
			_ASSERTE((WindowMode == wmNormal) || gpSet->isQuakeStyle);
			return FALSE;
		}

		DoMaximizeWidthHeight( (wParam == HTLEFT || wParam == HTRIGHT), (wParam == HTTOP || wParam == HTBOTTOM) );

		return TRUE;
	}

	return FALSE;
}

void CConEmuMain::SetDragCursor(HCURSOR hCur)
{
	mh_DragCursor = hCur;

	if (mh_DragCursor)
	{
		SetCursor(mh_DragCursor);
	}
	else
	{
		OnSetCursor();
	}
}

void CConEmuMain::SetWaitCursor(BOOL abWait)
{
	mb_WaitCursor = abWait;

	if (mb_WaitCursor)
		SetCursor(mh_CursorWait);
	else
		SetCursor(mh_CursorArrow);
}

void CConEmuMain::CheckFocus(LPCWSTR asFrom)
{
	HWND hCurForeground = GetForegroundWindow();
	static HWND hPrevForeground;

	if (hPrevForeground == hCurForeground)
		return;

	hPrevForeground = hCurForeground;
	DWORD dwPID = 0, dwTID = 0;

	#ifdef _DEBUG
	wchar_t szDbg[255], szClass[128];

	if (!hCurForeground || !GetClassName(hCurForeground, szClass, 127)) lstrcpy(szClass, L"<NULL>");
	#endif

	BOOL lbConEmuActive = (hCurForeground == ghWnd || (ghOpWnd && hCurForeground == ghOpWnd));
	BOOL lbLDown = isPressed(VK_LBUTTON);

	if (!lbConEmuActive)
	{
		if (!hCurForeground)
		{
			#ifdef _DEBUG
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"Foreground changed (%s). NewFore=0x00000000, LBtn=%i\n", asFrom, lbLDown);
			#endif
		}
		else
		{
			dwTID = GetWindowThreadProcessId(hCurForeground, &dwPID);
			// Получить информацию об активном треде
			GUITHREADINFO gti = {sizeof(GUITHREADINFO)};
			GetGUIThreadInfo(0/*dwTID*/, &gti);

			#ifdef _DEBUG
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"Foreground changed (%s). NewFore=0x%08X, Active=0x%08X, Focus=0x%08X, Class=%s, LBtn=%i\n", asFrom, (DWORD)hCurForeground, (DWORD)gti.hwndActive, (DWORD)gti.hwndFocus, szClass, lbLDown);
			#endif

			// mh_ShellWindow чисто для информации. Хоть родитель ConEmu и меняется на mh_ShellWindow
			// но проводник может перекинуть наше окно в другое (WorkerW или Progman)
			if (gpSet->isDesktopMode && mh_ShellWindow)
			{
				//HWND hShell = GetShellWindow(); // Progman
				bool lbDesktopActive = false;

				if (dwPID == mn_ShellWindowPID)  // дальше только для процесса Explorer.exe (который держит Desktop)
				{
					// При WinD активируется (Foreground) не Progman, а WorkerW
					// Тем не менее, фокус - передается в дочернее окно Progman
					if (hCurForeground == mh_ShellWindow)
					{
						lbDesktopActive = true;
					}
					else
					{
						wchar_t szClass[128];

						if (!GetClassName(hCurForeground, szClass, 127)) szClass[0] = 0;

						if (!wcscmp(szClass, L"WorkerW") || !wcscmp(szClass, L"Progman"))
							lbDesktopActive = true;

						//HWND hDesktop = GetDesktopWindow();
						//HWND hParent = GetParent(gti.hwndFocus);
						////	GetWindow(gti.hwndFocus, GW_OWNER);
						//while (hParent) {
						//	if (hParent == mh_ShellWindow) {
						//		lbDesktopActive = true;
						//		break;
						//	}
						//	hParent = GetParent(hParent);
						//	if (hParent == hDesktop) break;
						//}
					}
				}

				if (lbDesktopActive)
				{
					CVConGuard VCon;

					if (lbLDown)
					{
						// Кликнули мышкой для активации рабочего стола с иконками
						mb_FocusOnDesktop = FALSE; // запомним, что ConEmu активировать не нужно
					}
					else if (mb_FocusOnDesktop && (GetActiveVCon(&VCon) >= 0))
					{
						// Чтобы пользователю не приходилось вручную активировать ConEmu после WinD / WinM
						//apiSetForegroundWindow(ghWnd); // это скорее всего не сработает, т.к. фокус сейчас у другого процесса!
						// так что "активируем" мышкой
						COORD crOpaque = VCon->FindOpaqueCell();

						if (crOpaque.X<0 || crOpaque.Y<0)
						{
							DEBUGSTRFOREGROUND(L"Can't activate ConEmu on desktop. No opaque cell was found in VCon\n");
						}
						else
						{
							POINT pt = VCon->ConsoleToClient(crOpaque.X,crOpaque.Y);
							MapWindowPoints(VCon->GetView(), NULL, &pt, 1);
							HWND hAtPoint = WindowFromPoint(pt);

							if (hAtPoint != ghWnd)
							{
								#ifdef _DEBUG
								wchar_t szDbg[255], szClass[64];

								if (!hAtPoint || !GetClassName(hAtPoint, szClass, 63)) szClass[0] = 0;

								_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"Can't activate ConEmu on desktop. Opaque cell={%i,%i} screen={%i,%i}. WindowFromPoint=0x%08X (%s)\n",
								          crOpaque.X, crOpaque.Y, pt.x, pt.y, (DWORD)hAtPoint, szClass);
								DEBUGSTRFOREGROUND(szDbg);
								#endif
							}
							else
							{
								DEBUGSTRFOREGROUND(L"Activating ConEmu on desktop by mouse click\n");
								mouse.bForceSkipActivation = TRUE; // не пропускать этот клик в консоль!
								// Запомнить, где курсор сейчас. Вернуть надо будет
								POINT ptCur; GetCursorPos(&ptCur);
								SetCursorPos(pt.x,pt.y); // мышку обязательно "подвинуть", иначе mouse_event не сработает
								// "кликаем"
								mouse_event(MOUSEEVENTF_ABSOLUTE+MOUSEEVENTF_LEFTDOWN, pt.x,pt.y, 0,0);
								mouse_event(MOUSEEVENTF_ABSOLUTE+MOUSEEVENTF_LEFTUP, pt.x,pt.y, 0,0);
								// Вернуть курсор
								SetCursorPos(ptCur.x,ptCur.y);
								//
								//#ifdef _DEBUG -- очередь еще не обработана системой...
								//HWND hPost = GetForegroundWindow();
								//DEBUGSTRFOREGROUND((hPost==ghWnd) ? L"ConEmu on desktop activation Succeeded\n" : L"ConEmu on desktop activation FAILED\n");
								//#endif
							}
						}
					}
				}
			}
		}
	}
	else
	{
		#ifdef _DEBUG
		_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"Foreground changed (%s). NewFore=0x%08X, ConEmu has focus, LBtn=%i\n", asFrom, (DWORD)hCurForeground, lbLDown);
		#endif
		mb_FocusOnDesktop = TRUE;
	}

	UNREFERENCED_PARAMETER(dwTID);
	DEBUGSTRFOREGROUND(szDbg);
}

void CConEmuMain::CheckUpdates(BOOL abShowMessages)
{
	if (!gpConEmu->isUpdateAllowed())
		return;

	_ASSERTE(isMainThread());

	if (!gpUpd)
		gpUpd = new CConEmuUpdate;
	
	if (gpUpd)
		gpUpd->StartCheckProcedure(abShowMessages);
}

bool CConEmuMain::ReportUpdateConfirmation()
{
	bool lbRc = false;

	if (isMainThread())
	{
		if (gpUpd)
			lbRc = gpUpd->ShowConfirmation();
	}
	else
	{
		lbRc = (SendMessage(ghWnd, mn_MsgRequestUpdate, 1, 0) != 0);
	}

	return lbRc;
}

void CConEmuMain::ReportUpdateError()
{
	if (isMainThread())
	{
		if (gpUpd)
			gpUpd->ShowLastError();
	}
	else
	{
		PostMessage(ghWnd, mn_MsgRequestUpdate, 0, 0);
	}
}

void CConEmuMain::RequestExitUpdate()
{
	CConEmuUpdate::UpdateStep step = CConEmuUpdate::us_NotStarted;
	if (!gpUpd)
		return;

	step = gpUpd->InUpdate();
	if (step != CConEmuUpdate::us_ExitAndUpdate)
	{
		_ASSERTE(step == CConEmuUpdate::us_ExitAndUpdate);
		return;
	}

	// May be null, if update package was dropped on ConEmu icon
	if (ghWnd)
	{
		if (isMainThread())
		{
			UpdateProgress();
			PostScClose();
		}
		else
		{
			PostMessage(ghWnd, mn_MsgRequestUpdate, 2, 0);
		}
	}
}

void CConEmuMain::RequestPostUpdateTabs()
{
	PostMessage(ghWnd, mn_MsgUpdateTabs, 0, 0);
}

DWORD CConEmuMain::isSelectionModifierPressed(bool bAllowEmpty)
{
	DWORD nResult;

	if (m_Pressed.bChecked)
	{
		nResult = bAllowEmpty ? m_Pressed.nReadyToSel : m_Pressed.nReadyToSelNoEmpty;
	}
	else
	{
		DWORD nReadyToSel = 0, nReadyToSelNoEmpty = 0;
		bool bNoEmptyPressed = false, bEmptyAllowed = false;

		if (gpSet->isCTSSelectBlock)
		{
			gpSet->IsModifierPressed(vkCTSVkBlock, &bNoEmptyPressed, &bEmptyAllowed);
			if (bNoEmptyPressed)
				nReadyToSelNoEmpty = nReadyToSel = CONSOLE_BLOCK_SELECTION;
			else if (bEmptyAllowed)
				nReadyToSel = CONSOLE_BLOCK_SELECTION;
		}

		if (!nReadyToSelNoEmpty && gpSet->isCTSSelectText)
		{
			gpSet->IsModifierPressed(vkCTSVkText, &bNoEmptyPressed, &bEmptyAllowed);
			if (bNoEmptyPressed)
				nReadyToSelNoEmpty = nReadyToSel = CONSOLE_TEXT_SELECTION;
			else if (bEmptyAllowed && !nReadyToSel)
				nReadyToSel = CONSOLE_TEXT_SELECTION;
			_ASSERTE(nReadyToSel || (nReadyToSelNoEmpty == nReadyToSel));
		}

		// Store it
		m_Pressed.nReadyToSel = nReadyToSel;
		m_Pressed.nReadyToSelNoEmpty = nReadyToSelNoEmpty;
		m_Pressed.bChecked = TRUE;

		nResult = bAllowEmpty ? nReadyToSel : nReadyToSelNoEmpty;
	}

	return nResult;
}

void CConEmuMain::ForceSelectionModifierPressed(DWORD nValue)
{
	m_Pressed.nReadyToSel = nValue;
	m_Pressed.bChecked = TRUE;
}

enum DragPanelBorder CConEmuMain::CheckPanelDrag(COORD crCon)
{
	if (!gpSet->isDragPanel || isPictureView())
		return DPB_NONE;

	CVConGuard VCon;
	CRealConsole* pRCon = (GetActiveVCon(&VCon) >= 0) ? VCon->RCon() : NULL;

	if (!pRCon)
		return DPB_NONE;

	// Должен быть "Фар", "Панели", "Активен" (нет смысла пытаться дергать панели, если фар "висит")
	if (!pRCon->isFar() || !pRCon->isFilePanel(true) || !pRCon->isAlive())
		return DPB_NONE;

	// Если активен наш или ФАРовский граббер
	if (pRCon->isConSelectMode())
		return DPB_NONE;

	// Если удерживается модификатор запуска выделения текста
	if (gpConEmu->isSelectionModifierPressed(false))
		return DPB_NONE;

	//CONSOLE_CURSOR_INFO ci;
	//mp_ VActive->RCon()->GetConsoleCursorInfo(&ci);
	//   if (!ci.bVisible || ci.dwSize>40) // Курсор должен быть видим, и не в режиме граба
	//   	return DPB_NONE;
	// Теперь - можно проверить
	enum DragPanelBorder dpb = DPB_NONE;
	RECT rcPanel;
	
	//TODO("Сделаем все-таки драг влево-вправо хватанием за «промежуток» между рамками");
	//int nSplitWidth = gpSetCls->BorderFontWidth()/5;
	//if (nSplitWidth < 1) nSplitWidth = 1;
	

	if (VCon->RCon()->GetPanelRect(TRUE, &rcPanel, TRUE))
	{
		if (crCon.X == rcPanel.left && (rcPanel.top <= crCon.Y && crCon.Y <= rcPanel.bottom))
			dpb = DPB_SPLIT;
		else if (crCon.Y == rcPanel.bottom && (rcPanel.left <= crCon.X && crCon.X <= rcPanel.right))
			dpb = DPB_RIGHT;
	}

	if (dpb == DPB_NONE && VCon->RCon()->GetPanelRect(FALSE, &rcPanel, TRUE))
	{
		if (gpSet->isDragPanelBothEdges && crCon.X == rcPanel.right && (rcPanel.top <= crCon.Y && crCon.Y <= rcPanel.bottom))
			dpb = DPB_SPLIT;
		if (crCon.Y == rcPanel.bottom && (rcPanel.left <= crCon.X && crCon.X <= rcPanel.right))
			dpb = DPB_LEFT;
	}

	return dpb;
}

LRESULT CConEmuMain::OnSetCursor(WPARAM wParam, LPARAM lParam)
{
	DEBUGSTRSETCURSOR(lParam==-1 ? L"WM_SETCURSOR (int)" : L"WM_SETCURSOR");
#ifdef _DEBUG
	if (isPressed(VK_LBUTTON))
	{
		int nDbg = 0;
	}
#endif

	POINT ptCur; GetCursorPos(&ptCur);
	// Если сейчас идет trackPopupMenu - то на выход
	CVirtualConsole* pVCon = isMenuActive() ? NULL : GetVConFromPoint(ptCur);
	if (pVCon && !isActive(pVCon, false))
		pVCon = NULL;
	CRealConsole *pRCon = pVCon ? pVCon->RCon() : NULL;

	// Курсор НЕ над консолью?
	if (!pRCon)
	{
		// Status bar "Resize mark"?
		if (gpSet->isStatusBarShow && !gpSet->isStatusColumnHidden[csi_SizeGrip])
		{
			MapWindowPoints(NULL, ghWnd, &ptCur, 1);

			if (mp_Status->IsCursorOverResizeMark(ptCur))
			{
				LRESULT lResult = 0;
				if (mp_Status->ProcessStatusMessage(ghWnd, WM_SETCURSOR, 0,0, ptCur, lResult))
				{
					return TRUE;
				}
			}
		}

		// Если курсор НЕ над консолью - то курсор по умолчанию
		DEBUGSTRSETCURSOR(L" ---> skipped, not over VCon\n");
		return FALSE;
	}

	// В GUI режиме - не ломать курсор, заданный дочерним приложением
	HWND hGuiClient = pRCon->GuiWnd();
	if (pRCon && hGuiClient && pRCon->isGuiVisible())
	{
		DEBUGSTRSETCURSOR(L" ---> skipped (TRUE), GUI App Visible\n");
		return TRUE;
	}

	if (lParam == (LPARAM)-1)
	{ 
		RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);

		if (!PtInRect(&rcWnd, ptCur))
		{
			if (!isMeForeground())
			{
				DEBUGSTRSETCURSOR(L" ---> skipped, !isMeForeground()\n");
				return FALSE;
			}

			lParam = HTNOWHERE;
		}
		else
		{
			//CVirtualConsole* pVCon = GetVConFromPoint(ptCur); -- уже
			if (pVCon)
				lParam = HTCLIENT;
			else
				lParam = HTCAPTION;
		}

		wParam = pVCon ? (WPARAM)pVCon->GetView() : (WPARAM)ghWnd;
	}

	if (!(((HWND)wParam) == ghWnd || pVCon)
		|| isSizing()
		|| (LOWORD(lParam) != HTCLIENT && LOWORD(lParam) != HTNOWHERE))
	{
		/*if (gpSet->isHideCaptionAlways && !mb_InTrackSysMenu && !isSizing()
			&& (LOWORD(lParam) == HTTOP || LOWORD(lParam) == HTCAPTION))
		{
			SetCursor(mh_CursorMove);
			return TRUE;
		}*/
		DEBUGSTRSETCURSOR(L" ---> skipped, condition\n");
		return FALSE;
	}

	HCURSOR hCur = NULL;
	//LPCWSTR pszCurName = NULL;
	BOOL lbMeFore = TRUE;
	ExpandTextRangeType etr = etr_None;

	if (isMenuActive())
	{
		goto DefaultCursor;
	}

	if (LOWORD(lParam) == HTCLIENT && pVCon)
	{
		if (mouse.state & MOUSE_WINDOW_DRAG)
		{
			hCur = LoadCursor(NULL, IDC_SIZEALL);
		}
		// Если начат ShellDrag
		else if (mh_DragCursor && isDragging())
		{
			hCur = mh_DragCursor;
			DEBUGSTRSETCURSOR(L" ---> DragCursor\n");
		}
		else if (pRCon->isSelectionPresent())
		{
			if (gpSet->isCTSIBeam)
				hCur = mh_CursorIBeam; // LoadCursor(NULL, IDC_IBEAM);
		}
		else if ((etr = pRCon->GetLastTextRangeType()) != etr_None)
		{
			hCur = LoadCursor(NULL, IDC_HAND);
		}
		else if (mouse.state & MOUSE_DRAGPANEL_ALL)
		{
			if (mouse.state & MOUSE_DRAGPANEL_SPLIT)
			{
				hCur = mh_SplitH;
				DEBUGSTRSETCURSOR(L" ---> SplitH (dragging)\n");
			}
			else
			{
				hCur = mh_SplitV;
				DEBUGSTRSETCURSOR(L" ---> SplitV (dragging)\n");
			}
		}
		else
		{
			lbMeFore = isMeForeground();

			if (lbMeFore)
			{
				HWND hWndDC = pVCon ? pVCon->GetView() : NULL;
				if (pRCon && pRCon->isFar(FALSE) && hWndDC)  // Плагин не нужен, ФАР сам...
				{
					MapWindowPoints(NULL, hWndDC, &ptCur, 1);
					COORD crCon = pVCon->ClientToConsole(ptCur.x,ptCur.y);
					enum DragPanelBorder dpb = CheckPanelDrag(crCon);

					if (dpb == DPB_SPLIT)
					{
						hCur = mh_SplitH;
						DEBUGSTRSETCURSOR(L" ---> SplitH (allow)\n");
					}
					else if (dpb != DPB_NONE)
					{
						hCur = mh_SplitV;
						DEBUGSTRSETCURSOR(L" ---> SplitV (allow)\n");
					}
				}
			}

			// If mouse is used for selection, or specified modifier is pressed
			if (!hCur && lbMeFore && gpSet->isCTSIBeam)
			{
				// Хм, если выделение настроено "всегда" и без модификатора -
				// на экране всегда будет IBeam курсор, что может раздражать...
				// Поэтому - false
				if (isSelectionModifierPressed(false))
				{
					hCur = mh_CursorIBeam;
				}
			}
		}
	}

	if (!hCur && lbMeFore)
	{
		if (mb_WaitCursor)
		{
			hCur = mh_CursorWait;
			DEBUGSTRSETCURSOR(L" ---> CursorWait (mb_CursorWait)\n");
		}
		else if (gpSet->isFarHourglass)
		{
			if (pRCon)
			{
				BOOL lbAlive = pRCon->isAlive();
				if (!lbAlive)
				{
					hCur = mh_CursorAppStarting;
					DEBUGSTRSETCURSOR(L" ---> AppStarting (!pRCon->isAlive())\n");
				}
			}
		}
	}

DefaultCursor:
	if (!hCur)
	{
		hCur = mh_CursorArrow;
		DEBUGSTRSETCURSOR(L" ---> Arrow (default)\n");
	}

	SetCursor(hCur);
	return TRUE;
}

// true - если nPID запущен в одной из консолей
bool CConEmuMain::isConsolePID(DWORD nPID)
{
	return CVConGroup::isConsolePID(nPID);
}

LRESULT CConEmuMain::OnShellHook(WPARAM wParam, LPARAM lParam)
{
	/*
	wParam lParam
	HSHELL_GETMINRECT A pointer to a SHELLHOOKINFO structure.
	HSHELL_WINDOWACTIVATEED The HWND handle of the activated window.
	HSHELL_RUDEAPPACTIVATEED The HWND handle of the activated window.
	HSHELL_WINDOWREPLACING The HWND handle of the window replacing the top-level window.
	HSHELL_WINDOWREPLACED The HWND handle of the window being replaced.
	HSHELL_WINDOWCREATED The HWND handle of the window being created.
	HSHELL_WINDOWDESTROYED The HWND handle of the top-level window being destroyed.
	HSHELL_ACTIVATESHELLWINDOW Not used.
	HSHELL_TASKMAN Can be ignored.
	HSHELL_REDRAW The HWND handle of the window that needs to be redrawn.
	HSHELL_FLASH The HWND handle of the window that needs to be flashed.
	HSHELL_ENDTASK The HWND handle of the window that should be forced to exit.
	HSHELL_APPCOMMAND The APPCOMMAND which has been unhandled by the application or other hooks. See WM_APPCOMMAND and use the GET_APPCOMMAND_LPARAM macro to retrieve this parameter.
	*/
	switch(wParam)
	{
		case HSHELL_FLASH:
		{
			//HWND hCon = (HWND)lParam;
			//if (!hCon) return 0;
			//for (size_t i = 0; i < countof(mp_VCon); i++) {
			//    if (!mp_VCon[i]) continue;
			//    if (mp_VCon[i]->RCon()->ConWnd() == hCon) {
			//        FLASHWINFO fl = {sizeof(FLASHWINFO)};
			//        if (isMeForeground()) {
			//        	if (mp_VCon[i] != mp_ VActive) { // Только для неактивной консоли
			//                fl.dwFlags = FLASHW_STOP; fl.hwnd = ghWnd;
			//                FlashWindowEx(&fl); // Чтобы мигание не накапливалось
			//        		fl.uCount = 4; fl.dwFlags = FLASHW_ALL; fl.hwnd = ghWnd;
			//        		FlashWindowEx(&fl);
			//        	}
			//        } else {
			//        	fl.dwFlags = FLASHW_ALL|FLASHW_TIMERNOFG; fl.hwnd = ghWnd;
			//        	FlashWindowEx(&fl); // Помигать в GUI
			//        }
			//
			//        fl.dwFlags = FLASHW_STOP; fl.hwnd = hCon;
			//        FlashWindowEx(&fl);
			//        break;
			//    }
			//}
		}
		break;

		case HSHELL_WINDOWCREATED:
		{
			if (isMeForeground())
			{
				HWND hWnd = (HWND)lParam;

				if (!hWnd) return 0;

				DWORD dwPID = 0, dwParentPID = 0; // dwFarPID = 0;
				GetWindowThreadProcessId(hWnd, &dwPID);

				if (dwPID && dwPID != GetCurrentProcessId())
				{
					AllowSetForegroundWindow(dwPID);

					WARNING("Проверить, успевает ли диалог показаться, или потом для него ShowWindow зовется?");
					if (IsWindowVisible(hWnd))  // ? оно успело ?
					{
						bool lbConPID = isConsolePID(dwPID);

						PROCESSENTRY32 prc = {sizeof(PROCESSENTRY32)};
						if (!lbConPID)
						{
							// Получить PID родительского процесса этого окошка
							if (GetProcessInfo(dwPID, &prc))
							{
								dwParentPID = prc.th32ParentProcessID;

								lbConPID = (dwParentPID && isConsolePID(dwParentPID));
							}
						}

						if (lbConPID)
						{
							apiSetForegroundWindow(hWnd);
						}
					}

					//#ifdef _DEBUG
					//wchar_t szTitle[255], szClass[255], szMsg[1024];
					//GetWindowText(hWnd, szTitle, 254); GetClassName(hWnd, szClass, 254);
					//_wsprintf(szMsg, countof(szMsg), L"Window was created:\nTitle: %s\nClass: %s\nPID: %i", szTitle, szClass, dwPID);
					//MBox(szMsg);
					//#endif
				}
			}
		}
		break;
		
		#ifdef _DEBUG
		case HSHELL_ACTIVATESHELLWINDOW:
		{
			// Не вызывается
			wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"HSHELL_ACTIVATESHELLWINDOW(lParam=0x%08X)\n", (DWORD)lParam);
			DEBUGSTRFOREGROUND(szDbg);
		}
		break;
		#endif

		case HSHELL_WINDOWACTIVATED:
		{
			// Приходит позже чем WM_ACTIVATE(WA_INACTIVE), но пригодится, если CE был НЕ в фокусе
			#ifdef _DEBUG
			// Когда активируется Desktop - lParam == 0

			wchar_t szDbg[128], szClass[64];
			if (!lParam || !GetClassName((HWND)lParam, szClass, 63))
				wcscpy(szClass, L"<NULL>");

			BOOL lbLBtn = isPressed(VK_LBUTTON);
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"HSHELL_WINDOWACTIVATED(lParam=0x%08X, %s, %i)\n", (DWORD)lParam, szClass, lbLBtn);
			DEBUGSTRFOREGROUND(szDbg);
			#endif

			//CheckFocus(L"HSHELL_WINDOWACTIVATED");
			OnFocus(NULL, 0, 0, 0, gsFocusShellActivated);
		}
		break;
	}

	return 0;
}

void CConEmuMain::OnAlwaysOnTop()
{
	HWND hwndAfter = (gpSet->isAlwaysOnTop || gpSet->isDesktopMode) ? HWND_TOPMOST : HWND_NOTOPMOST;

	#ifdef CATCH_TOPMOST_SET
	_ASSERTE((hwndAfter!=HWND_TOPMOST) && "Setting TopMost mode - CConEmuMain::OnAlwaysOnTop()");
	#endif

	CheckMenuItem(gpConEmu->mp_Menu->GetSysMenu(), ID_ALWAYSONTOP, MF_BYCOMMAND |
	              (gpSet->isAlwaysOnTop ? MF_CHECKED : MF_UNCHECKED));
	SetWindowPos(ghWnd, hwndAfter, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);

	if (ghOpWnd && gpSet->isAlwaysOnTop)
	{
		SetWindowPos(ghOpWnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
		apiSetForegroundWindow(ghOpWnd);
	}
}

void CConEmuMain::OnDesktopMode()
{
	if (!this) return;

	Icon.SettingsChanged();

	if (WindowStartMinimized || ForceMinimizeToTray)
		return;

#ifndef CHILD_DESK_MODE
	DWORD dwStyleEx = GetWindowLong(ghWnd, GWL_EXSTYLE);
	DWORD dwNewStyleEx = dwStyleEx;
	DWORD dwStyle = GetWindowLong(ghWnd, GWL_STYLE);
	DWORD dwNewStyle = dwStyle;

	if (gpSet->isDesktopMode)
	{
		dwNewStyleEx |= WS_EX_TOOLWINDOW;
		dwNewStyle |= WS_POPUP;
	}
	else
	{
		dwNewStyleEx &= ~WS_EX_TOOLWINDOW;
		dwNewStyle &= ~WS_POPUP;
	}

	if (dwNewStyleEx != dwStyleEx || dwNewStyle != dwStyle)
	{
		SetWindowStyle(dwStyle);
		SetWindowStyleEx(dwStyleEx);
		SyncWindowToConsole(); // -- функция пустая, игнорируется
		UpdateWindowRgn();
	}

#endif
#ifdef CHILD_DESK_MODE
	HWND hDesktop = GetDesktopWindow();

	//HWND hProgman = FindWindowEx(hDesktop, NULL, L"Progman", L"Program Manager");
	//HWND hParent = NULL;gpSet->isDesktopMode ?  : GetDesktopWindow();

	OnTaskbarSettingsChanged();

	if (gpSet->isDesktopMode)
	{
		// Shell windows is FindWindowEx(hDesktop, NULL, L"Progman", L"Program Manager");
		HWND hShellWnd = GetShellWindow();
		DWORD dwShellPID = 0;

		if (hShellWnd)
			GetWindowThreadProcessId(hShellWnd, &dwShellPID);

		// But in Win7 it is not a real desktop holder :(
		if (gOSVer.dwMajorVersion >= 6)  // Vista too?
		{
			// В каких-то случаях (на каких-то темах?) иконки создаются в "Progman", а в одном из "WorkerW" классов
			// Все эти окна принадлежат одному процессу explorer.exe
			HWND hShell = FindWindowEx(hDesktop, NULL, L"WorkerW", NULL);

			while (hShell)
			{
				// У него должны быть дочерние окна
				if (IsWindowVisible(hShell) && FindWindowEx(hShell, NULL, NULL, NULL))
				{
					// Теоретически, эти окна должны принадлежать одному процессу (Explorer.exe)
					if (dwShellPID)
					{
						DWORD dwTestPID;
						GetWindowThreadProcessId(hShell, &dwTestPID);

						if (dwTestPID != dwShellPID)
						{
							hShell = FindWindowEx(hDesktop, hShell, L"WorkerW", NULL);
							continue;
						}
					}

					break;
				}

				hShell = FindWindowEx(hDesktop, hShell, L"WorkerW", NULL);
			}

			if (hShell)
				hShellWnd = hShell;
		}

		if (gpSet->isQuakeStyle)  // этот режим с Desktop несовместим
		{
			SetQuakeMode(0, (WindowMode == wmFullScreen) ? wmMaximized : wmNormal);
		}

		if (WindowMode == wmFullScreen)  // этот режим с Desktop несовместим
		{
			SetWindowMode(wmMaximized);
		}

		if (!hShellWnd)
		{
			gpSet->isDesktopMode = false;

			HWND hExt = gpSetCls->mh_Tabs[gpSetCls->thi_Ext];

			if (ghOpWnd && hExt)
			{
				CheckDlgButton(hExt, cbDesktopMode, BST_UNCHECKED);
			}
		}
		else
		{
			mh_ShellWindow = hShellWnd;
			GetWindowThreadProcessId(mh_ShellWindow, &mn_ShellWindowPID);
			RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);
			MapWindowPoints(NULL, mh_ShellWindow, (LPPOINT)&rcWnd, 2);
			//ShowWindow(SW_HIDE);
			//SetWindowPos(ghWnd, NULL, rcWnd.left,rcWnd.top,0,0, SWP_NOSIZE|SWP_NOZORDER);
			SetParent(mh_ShellWindow);
			SetWindowPos(ghWnd, NULL, rcWnd.left,rcWnd.top,0,0, SWP_NOSIZE|SWP_NOZORDER);
			SetWindowPos(ghWnd, HWND_TOPMOST, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE);
			//ShowWindow(SW_SHOW);
#ifdef _DEBUG
			RECT rcNow; GetWindowRect(ghWnd, &rcNow);
#endif
		}
	}

	if (!gpSet->isDesktopMode)
	{
		//dwStyle |= WS_POPUP;
		RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);
		RECT rcVirtual = GetVirtualScreenRect(TRUE);
		SetWindowPos(ghWnd, NULL, max(rcWnd.left,rcVirtual.left),max(rcWnd.top,rcVirtual.top),0,0, SWP_NOSIZE|SWP_NOZORDER);
		SetParent(hDesktop);
		SetWindowPos(ghWnd, NULL, max(rcWnd.left,rcVirtual.left),max(rcWnd.top,rcVirtual.top),0,0, SWP_NOSIZE|SWP_NOZORDER);
		OnAlwaysOnTop();

		if (ghOpWnd && !gpSet->isAlwaysOnTop)
			apiSetForegroundWindow(ghOpWnd);
	}

	//SetWindowStyle(dwStyle);
#endif
}

UINT_PTR CConEmuMain::SetKillTimer(bool bEnable, UINT nTimerID, UINT nTimerElapse)
{
	UINT_PTR nRc;
	if (bEnable)
		nRc = ::SetTimer(ghWnd, nTimerID, nTimerElapse, NULL);
	else
		nRc = ::KillTimer(ghWnd, nTimerID);
	return nRc;
}

LRESULT CConEmuMain::OnTimer(WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

#ifdef _DEBUG
	wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"ConEmu:MainTimer(%u)\n", wParam);
	DEBUGSTRTIMER(szDbg);
#endif

	//if (mb_InTimer) return 0; // чтобы ненароком два раза в одно событие не вошел (хотя не должен)
	mb_InTimer = TRUE;
	//result = gpConEmu->OnTimer(wParam, lParam);

#ifdef DEBUGSHOWFOCUS
	HWND hFocus = GetFocus();
	HWND hFore = GetForegroundWindow();
	static HWND shFocus, shFore;
	static wchar_t szWndInfo[1200];

	if (hFocus != shFocus)
	{
		_wsprintf(szWndInfo, SKIPLEN(countof(szWndInfo)) L"(Fore=0x%08X) Focus was changed to ", (DWORD)hFore);
		getWindowInfo(hFocus, szWndInfo+_tcslen(szWndInfo));
		wcscat(szWndInfo, L"\n");
		DEBUGSHOWFOCUS(szWndInfo);
		shFocus = hFocus;
	}

	if (hFore != shFore)
	{
		if (hFore != hFocus)
		{
			wcscpy(szWndInfo, L"Foreground window was changed to ");
			getWindowInfo(hFore, szWndInfo+_tcslen(szWndInfo));
			wcscat(szWndInfo, L"\n");
			DEBUGSHOWFOCUS(szWndInfo);
		}

		shFore = hFore;
	}

#endif

	CVirtualConsole* pVCon = NULL;
	CVConGuard VCon;
	if (GetActiveVCon(&VCon) >= 0)
		pVCon = VCon.VCon();

	switch (wParam)
	{
		case TIMER_MAIN_ID: // Период: 500 мс
			OnTimer_Main(pVCon);
			break; // case 0:

		case TIMER_CONREDRAW_ID: // Период: CON_REDRAW_TIMOUT*2
			OnTimer_ConRedraw(pVCon);
			break; // case 1:

		case TIMER_CAPTION_APPEAR_ID:
		case TIMER_CAPTION_DISAPPEAR_ID:
			OnTimer_FrameAppearDisappear(wParam);
			break;

		case TIMER_RCLICKPAINT:
			OnTimer_RClickPaint();
			break;

		case TIMER_ADMSHIELD_ID:
			OnTimer_AdmShield();
			break;

		case TIMER_RUNQUEUE_ID:
			mp_RunQueue->ProcessRunQueue(false);
			break;

		case TIMER_QUAKE_AUTOHIDE_ID:
			OnTimer_QuakeFocus();
			break;

		case TIMER_FAILED_TABBAR_ID:
			mp_TabBar->OnTimer(wParam);
			break;

		case TIMER_ACTIVATESPLIT_ID:
			OnTimer_ActivateSplit();
			break;
	}

	mb_InTimer = FALSE;
	return result;
}

void CConEmuMain::SetRunQueueTimer(bool bSet, UINT uElapse)
{
	static bool bLastSet = false;

	if (bSet)
	{
		if (!bLastSet)
			SetKillTimer(true, TIMER_RUNQUEUE_ID, uElapse);
	}
	else
	{
		SetKillTimer(false, TIMER_RUNQUEUE_ID, 0);
	}

	bLastSet = bSet;
}

void CConEmuMain::OnTimer_Main(CVirtualConsole* pVCon)
{
	#ifdef _DEBUG
	if (mb_DestroySkippedInAssert)
	{
		if (!gbInMyAssertTrap)
		{
			if (CVConGroup::GetConCount() == 0)
			{
				LogString(L"-- Destroy was skipped due to gbInMyAssertTrap, reposting");
				Destroy();
				return;
			}
			else
			{
				mb_DestroySkippedInAssert = false;
			}
		}
	}
	#endif

	//Maximus5. Hack - если какая-то зараза задизеблила окно
	if (!DontEnable::isDontEnable())
	{
		DWORD dwStyle = GetWindowLong(ghWnd, GWL_STYLE);

		if (dwStyle & WS_DISABLED)
			EnableWindow(ghWnd, TRUE);
	}

	HWND hForeWnd = NULL;
	bool bForeground = isMeForeground(false, true, &hForeWnd);
	if (bForeground && !m_GuiInfo.bGuiActive)
	{
		UpdateGuiInfoMappingActive(true);
	}

	DWORD dwStyleEx = GetWindowLong(ghWnd, GWL_EXSTYLE);
	#ifdef CATCH_TOPMOST_SET
	static bool bWasTopMost = false;
	_ASSERTE((bWasTopMost || gpSet->isAlwaysOnTop || ((dwStyleEx & WS_EX_TOPMOST)==0)) && "TopMost mode was set (WM_TIMER)");
	bWasTopMost = ((dwStyleEx & WS_EX_TOPMOST)==WS_EX_TOPMOST);
	#endif
	if (!gpSet->isAlwaysOnTop && ((dwStyleEx & WS_EX_TOPMOST)==WS_EX_TOPMOST))
	{
		CheckTopMostState();
	}

	mp_Status->OnTimer();

	CheckProcesses();
	TODO("Теперь это условие не работает. 1 - раньше это был сам ConEmu.exe");

	if (m_ProcCount == 0)
	{
		// При ошибках запуска консольного приложения хотя бы можно будет увидеть, что оно написало...
		if (mb_ProcessCreated)
		{
			OnAllVConClosed();
			// Once. Otherwise window we can't show window when "Auto hide" is on...
			mb_ProcessCreated = false;
			return;
		}
	}
	else
	{
		if (!mb_WorkspaceErasedOnClose)
			mb_WorkspaceErasedOnClose = false;
	}

	// Чтобы не возникало "зависаний/блокировок" в потоке чтения консоли - проверяем "живость" сервера
	// Кроме того, здесь проверяется "нужно ли скроллить консоль во время выделения мышкой"
	CVConGroup::OnRConTimerCheck();

	// TODO: поддержку SlideShow повесить на отдельный таймер
	BOOL lbIsPicView = isPictureView();

	if (bPicViewSlideShow)
	{
		DWORD dwTicks = GetTickCount();
		DWORD dwElapse = dwTicks - dwLastSlideShowTick;

		if (dwElapse > gpSet->nSlideShowElapse)
		{
			if (IsWindow(hPictureView) && pVCon)
			{
				//
				bPicViewSlideShow = false;
				SendMessage(pVCon->RCon()->ConWnd(), WM_KEYDOWN, VK_NEXT, 0x01510001);
				SendMessage(pVCon->RCon()->ConWnd(), WM_KEYUP, VK_NEXT, 0xc1510001);
				// Окно могло измениться?
				isPictureView();
				dwLastSlideShowTick = GetTickCount();
				bPicViewSlideShow = true;
			}
			else
			{
				hPictureView = NULL;
				bPicViewSlideShow = false;
			}
		}
	}

	if (mp_Inside)
	{
		mp_Inside->InsideParentMonitor();
	}

	//2009-04-22 - вроде не требуется
	/*if (lbIsPicView && !isPiewUpdate)
	{
	    // чтобы принудительно обновиться после закрытия PicView
	    isPiewUpdate = true;
	}*/

	if (!lbIsPicView && isPiewUpdate)
	{
		// После скрытия/закрытия PictureView нужно передернуть консоль - ее размер мог измениться
		isPiewUpdate = false;
		CVConGroup::SyncConsoleToWindow();
		InvalidateAll();
	}

	if (!isIconic())
	{
		// Был сдвиг окна? Проверка после отпускания кнопки мышки
		if ((mouse.state & MOUSE_SIZING_BEGIN) || mouse.bCheckNormalRect)
		{
			bool bIsSizing = isSizing();
			if (!bIsSizing)
			{
				StoreNormalRect(NULL); // Сама разберется, надо/не надо
			}
		}
		//// Было ли реальное изменение размеров?
		//BOOL lbSizingToDo  = (mouse.state & MOUSE_SIZING_TODO) == MOUSE_SIZING_TODO;

		//if (isSizing() && !isPressed(VK_LBUTTON)) {
		//    // Сборс всех флагов ресайза мышкой
		//    ResetSizingFlags(MOUSE_SIZING_BEGIN|MOUSE_SIZING_TODO);
		//}

		//TODO("возможно весь ресайз (кроме SyncNtvdm?) нужно перенести в нить консоли")
		//OnConsoleResize();

		// update scrollbar
		OnUpdateScrollInfo(TRUE);
	}
	else
	{
		if (mouse.bCheckNormalRect)
			mouse.bCheckNormalRect = false;
	}

	// режим полного скрытия заголовка
	if (gpSet->isCaptionHidden())
	{
		if (!bForeground)
		{
			if (m_ForceShowFrame)
			{
				StopForceShowFrame();
			}
		}
		else
		{
			// в Normal режиме при помещении мышки над местом, где должен быть
			// заголовок или рамка - показать их
			if (!isIconic() && (WindowMode == wmNormal))
			{
				TODO("Не наколоться бы с предыдущим статусом при ресайзе?");
				//static bool bPrevForceShow = false;
				bool bCurForceShow = isMouseOverFrame(true);

				if (bCurForceShow != (m_ForceShowFrame != fsf_Hide))
				{
					m_ForceShowFrame = bCurForceShow ? fsf_WaitShow : fsf_Hide;
					SetKillTimer(false, TIMER_CAPTION_APPEAR_ID, 0);
					SetKillTimer(false, TIMER_CAPTION_DISAPPEAR_ID, 0);
					WORD nID = bCurForceShow ? TIMER_CAPTION_APPEAR_ID : TIMER_CAPTION_DISAPPEAR_ID;
					DWORD nDelay = bCurForceShow ? gpSet->nHideCaptionAlwaysDelay : gpSet->nHideCaptionAlwaysDisappear;

					// Если просили показывать с задержкой - то по таймеру
					if (nDelay)
						SetKillTimer(true, nID, nDelay);
					else if (bCurForceShow)
						StartForceShowFrame();
					else
						StopForceShowFrame();
				}
			}
		}
	}

	if (pVCon)
	{
		bool bLastFade = pVCon->mb_LastFadeFlag;
		bool bNewFade = (gpSet->isFadeInactive && !bForeground && !lbIsPicView);

		// Это условие скорее всего никогда не выполнится, т.к.
		// смена Fade обрабатывается в WM_ACTIVATE/WM_SETFOCUS/WM_KILLFOCUS
		if (bLastFade != bNewFade)
		{
			pVCon->mb_LastFadeFlag = bNewFade;
			pVCon->Invalidate();
		}
	}

	if (mh_ConEmuAliveEvent && !mb_ConEmuAliveOwned)
		isFirstInstance(); // Заодно и проверит...

	#ifndef APPDISTINCTBACKGROUND
	// Если был изменен файл background
	if (gpSetCls->PollBackgroundFile())
	{
		gpConEmu->Update(true);
	}
	#endif

	if (isMenuActive() && mp_Tip)
	{
		POINT ptCur; GetCursorPos(&ptCur);
		HWND hPoint = WindowFromPoint(ptCur);
		if (hPoint)
		{
			#if 0
			wchar_t szWinInfo[1024];
			_wsprintf(szWinInfo, SKIPLEN(countof(szWinInfo)) L"WindowFromPoint(%i,%i) ", ptCur.x, ptCur.y);
			OutputDebugString(szWinInfo);
			getWindowInfo(hPoint, szWinInfo);
			wcscat_c(szWinInfo, L"\n");
			OutputDebugString(szWinInfo);
			#endif

			wchar_t szClass[128];
			if (GetClassName(hPoint, szClass, countof(szClass))
				&& (lstrcmp(szClass, VirtualConsoleClass) == 0 || lstrcmp(szClass, VirtualConsoleClassMain) == 0))
			{
				mp_Tip->HideTip();
			}
		}
	}

	// -- Замена на OnFocus
	//CheckFocus(L"TIMER_MAIN_ID");
	// Проверить, может ConEmu был активирован, а сервер нет?
	OnFocus(NULL, 0, 0, 0, gsFocusCheckTimer);

	if (!lbIsPicView && gpSet->UpdSet.isUpdateCheckHourly)
	{
		gpSet->UpdSet.CheckHourlyUpdate();
	}

}

void CConEmuMain::OnActivateSplitChanged()
{
	DWORD nTimeout = TIMER_ACTIVATESPLIT_ELAPSE;
	bool bActive = gpSet->isActivateSplitMouseOver();
	// Try to get system default timeout only if 'system' options is used
	if (gpSet->bActivateSplitMouseOver == 2)
	{
		if (!SystemParametersInfo(SPI_GETACTIVEWNDTRKTIMEOUT, 0, &nTimeout, 0))
			nTimeout = TIMER_ACTIVATESPLIT_ELAPSE;
		else if (nTimeout < 25)
			nTimeout = 25;
		else if (nTimeout > 2500)
			nTimeout = 2500;
	}
	SetKillTimer(bActive, TIMER_ACTIVATESPLIT_ID, nTimeout);
}

void CConEmuMain::OnTimer_ActivateSplit()
{
	if (!gpSet->isActivateSplitMouseOver())
	{
		SetKillTimer(false, TIMER_ACTIVATESPLIT_ID, 0);
		return;
	}

	if (isMenuActive())
	{
		return;
	}

	HWND hForeWnd = NULL;
	bool bForeground = isMeForeground(false, true, &hForeWnd);

	CVConGuard VConFromPoint;
	// bForeground - does not fit our needs (it ignores GUI children)
	// 130821 - Don't try to change focus during popup menu active!
	POINT ptCur; GetCursorPos(&ptCur);
	if (!PTDIFFTEST(mouse.ptLastSplitOverCheck, SPLITOVERCHECK_DELTA))
	{
		mouse.ptLastSplitOverCheck = ptCur;
		if (!isIconic() && (hForeWnd == ghWnd || CVConGroup::isOurGuiChildWindow(hForeWnd)))
		{
			// Курсор над ConEmu?
			RECT rcClient = CalcRect(CER_MAIN);
			// Проверяем, в какой из VCon попадает курсор?
			if (PtInRect(&rcClient, ptCur))
			{
				if (CVConGroup::GetVConFromPoint(ptCur, &VConFromPoint))
				{
					bool bActive = isActive(VConFromPoint.VCon(), false);
					if (!bActive)
					{
						CVConGuard VCon;
						// Если был активирован GUI CHILD - то фокус нужно сначала поставить в ConEmu
						if (!bForeground && (GetActiveVCon(&VCon) >= 0) && VCon->RCon()->GuiWnd())
						{
							apiSetForegroundWindow(ghWnd);
						}

						if (Activate(VConFromPoint.VCon()))
						{
							// Если был активен GUI CHILD - то ConEmu может НЕ активироваться
							if (!bForeground && !VConFromPoint.VCon()->RCon()->GuiWnd())
							{
								apiSetForegroundWindow(ghWnd);
							}
						}
					}
				}
			}
		}
	}

	UNREFERENCED_PARAMETER(bForeground);
}

void CConEmuMain::OnTimer_ConRedraw(CVirtualConsole* pVCon)
{
	if (pVCon && !isIconic())
	{
		pVCon->CheckPostRedraw();
	}
}

void CConEmuMain::OnTimer_FrameAppearDisappear(WPARAM wParam)
{
	SetKillTimer(false, wParam, 0);

	if (isMouseOverFrame(true))
		StartForceShowFrame();
	else
		StopForceShowFrame();
}

void CConEmuMain::OnTimer_RClickPaint()
{
	RightClickingPaint(NULL, NULL);
}

void CConEmuMain::OnTimer_AdmShield()
{
	static int nStep = 0;
	if (gpSet->isTaskbarShield)
	{
		Taskbar_UpdateOverlay();
		nStep++;
	}
	if ((nStep >= 5) || !gpSet->isTaskbarShield)
	{
		SetKillTimer(false, TIMER_ADMSHIELD_ID, 0);
	}
}

void CConEmuMain::OnTimer_QuakeFocus()
{
	if (mn_ForceTimerCheckLoseFocus)
	{
		if (RELEASEDEBUGTEST(gpSetCls->isAdvLogging,true))
		{
			wchar_t szInfo[80];
			DWORD nDelay = GetTickCount() - mn_ForceTimerCheckLoseFocus;
			_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"OnTimer_QuakeFocus, delay %u ms", nDelay);
			LogFocusInfo(szInfo);
		}

		OnFocus(NULL, 0, 0, 0, gsFocusQuakeCheckTimer);
	}
	else
	{
		LogFocusInfo(L"OnTimer_QuakeFocus skipped, already processed (TSA)");
	}

	SetKillTimer(false, TIMER_QUAKE_AUTOHIDE_ID, 0);
}

void CConEmuMain::OnTransparentSeparate(bool bSetFocus)
{
	if (gpSet->isTransparentSeparate
		&& (gpSet->nTransparent != gpSet->nTransparentInactive)
		&& gpSet->isTransparentAllowed())
	{
		// То нужно обновить "прозрачность"
		OnTransparent(true, bSetFocus);
	}
}

void CConEmuMain::OnTransparent()
{
	// Don't touch transparency during Quake-animation
	if (mn_QuakePercent != 0)
		return;

	bool bSetFocus = mp_Menu->GetRestoreFromMinimized();
	bool bFromFocus = bSetFocus;

	OnTransparent(bFromFocus, bSetFocus);
}

void CConEmuMain::OnTransparent(bool abFromFocus /*= false*/, bool bSetFocus /*= false*/)
{
	bool bForceLayered = false;
	bool bActive = abFromFocus ? bSetFocus : isMeForeground();
	bool bColorKey = gpSet->isColorKeyTransparent;
	UINT nAlpha = (bActive || !gpSet->isTransparentSeparate) ? gpSet->nTransparent : gpSet->nTransparentInactive;
	nAlpha = max((UINT)(bActive?MIN_ALPHA_VALUE:MIN_INACTIVE_ALPHA_VALUE),(UINT)min(nAlpha,255));

	if (((gpSet->nTransparent < 255)
			|| (gpSet->isTransparentSeparate && (gpSet->nTransparentInactive < 255))
			|| bColorKey)
		&& !gpSet->isTransparentAllowed())
	{
		nAlpha = 255; bColorKey = false;
	}
	else if (gpSet->isTransparentSeparate && (nAlpha == 255)
		&& ((gpSet->nTransparent < 255) || (gpSet->nTransparentInactive < 255)))
	{
		bForceLayered = true;
	}

	TODO("EnableBlurBehind: tabs and toolbar must be rewritten, all parts of GUI must be drawn with Alpha channel");
	#ifdef _DEBUG
	//EnableBlurBehind((nAlpha < 255));
	#endif

	// return true - when state was changes
	if (SetTransparent(ghWnd, nAlpha, bColorKey, gpSet->nColorKeyValue, bForceLayered))
	{
		// Запомнить
		mb_LastTransparentFocused = bActive;

		if (!isMenuActive())
		{
			OnSetCursor();
		}
	}
}

// return true - when state was changes
bool CConEmuMain::SetTransparent(HWND ahWnd, UINT anAlpha/*0..255*/, bool abColorKey /*= false*/, COLORREF acrColorKey /*= 0*/, bool abForceLayered /*= false*/)
{
	#ifdef __GNUC__
	if (!SetLayeredWindowAttributes)
	{
		_ASSERTE(SetLayeredWindowAttributes!=NULL);
		return false;
	}
	#endif

 	BOOL bNeedRedrawOp = FALSE;
	// Тут бы ветвиться по Active/Inactive, но это будет избыточно.
	// Проверка уже сделана в OnTransparent
	UINT nTransparent = max(MIN_INACTIVE_ALPHA_VALUE,min(anAlpha,255));
	DWORD dwExStyle = GetWindowLongPtr(ahWnd, GWL_EXSTYLE);
	bool lbChanged = false;

	if (ahWnd == ghWnd)
		mp_Status->OnTransparency();

	if ((nTransparent >= 255) && !abColorKey && !abForceLayered)
	{
		// Прозрачность отключается (полностью непрозрачный)
		//SetLayeredWindowAttributes(ahWnd, 0, 255, LWA_ALPHA);
		if ((dwExStyle & WS_EX_LAYERED) == WS_EX_LAYERED)
		{
			dwExStyle &= ~WS_EX_LAYERED;
			SetLayeredWindowAttributes(ahWnd, 0, 255, LWA_ALPHA);
			SetWindowStyleEx(ahWnd, dwExStyle);
			lbChanged = true;
			LogString("Transparency: Disabling WS_EX_LAYERED");
		}
		else
		{
			LogString("Transparency: WS_EX_LAYERED was already disabled");
		}
	}
	else
	{
		if ((dwExStyle & WS_EX_LAYERED) == 0)
		{
			dwExStyle |= WS_EX_LAYERED;
			SetWindowStyleEx(ahWnd, dwExStyle);
			bNeedRedrawOp = TRUE;
			lbChanged = true;
			LogString("Transparency: Enabling WS_EX_LAYERED");
		}
		else
		{
			LogString("Transparency: WS_EX_LAYERED was already enabled");
		}

		DWORD nNewFlags = (((nTransparent < 255) || abForceLayered) ? LWA_ALPHA : 0) | (abColorKey ? LWA_COLORKEY : 0);

		BYTE nCurAlpha = 0;
		DWORD nCurFlags = 0;
		COLORREF nCurColorKey = 0;
		BOOL bGet = FALSE;
		if (_GetLayeredWindowAttributes)
			bGet = _GetLayeredWindowAttributes(ahWnd, &nCurColorKey, &nCurAlpha, &nCurFlags);

		if (lbChanged
			|| (!bGet)
			|| (nCurAlpha != nTransparent) || (nCurFlags != nNewFlags)
			|| (abColorKey && (nCurColorKey != acrColorKey)))
		{
			lbChanged = true;
			BOOL bSet = SetLayeredWindowAttributes(ahWnd, acrColorKey, nTransparent, nNewFlags);
			if (gpSetCls->isAdvLogging)
			{
				wchar_t szInfo[128];
				DWORD nErr = bSet ? 0 : GetLastError();
				_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"Transparency: Set(0x%08X, 0x%08X, %u, 0x%X) -> %u:%u",
					(DWORD)ahWnd, acrColorKey, nTransparent, nNewFlags, bSet, nErr);
				LogString(szInfo);
			}
		}
		else
		{
			LogString(L"Transparency: Attributes were not changed");
		}

		// После смены стиля (не было альфа - появилась альфа) измененное окно "выносится наверх"
		// и принудительно перерисовывается. Если в этот момент видим диалог настроек - он затирается.
		if (bNeedRedrawOp)
		{
			HWND hWindows[] = {ghOpWnd, (mp_AttachDlg ? mp_AttachDlg->GetHWND() : NULL), mp_Find->mh_FindDlg};
			for (size_t i = 0; i < countof(hWindows); i++)
			{
				if (hWindows[i] && (hWindows[i] != ahWnd) && IsWindow(hWindows[i]))
				{
					// Ask the window and its children to repaint
					RedrawWindow(hWindows[i], NULL, NULL, RDW_ERASE|RDW_INVALIDATE|RDW_FRAME|RDW_ALLCHILDREN);
				}
			}
		}
	}

	return lbChanged;
}

// Вызовем UpdateScrollPos для АКТИВНОЙ консоли
LRESULT CConEmuMain::OnUpdateScrollInfo(BOOL abPosted/* = FALSE*/)
{
	if (!abPosted)
	{
		PostMessage(ghWnd, mn_MsgUpdateScrollInfo, 0, 0);
		return 0;
	}

	CVConGroup::OnUpdateScrollInfo();

	return 0;
}

// Чтобы при создании ПЕРВОЙ консоли на экране сразу можно было что-то нарисовать
void CConEmuMain::OnVConCreated(CVirtualConsole* apVCon, const RConStartArgs *args)
{
	SetScClosePending(false); // сброс

	// Если ConEmu не был закрыт после закрытия всех табов, и запущен новый таб...
	gpSet->ResetSavedOnExit();

	CVConGroup::OnVConCreated(apVCon, args);
}

// Зависит от настроек и того, как закрывали
bool CConEmuMain::isDestroyOnClose(bool ScCloseOnEmpty /*= false*/)
{
	CConEmuUpdate::UpdateStep step = gpUpd ? gpUpd->InUpdate() : CConEmuUpdate::us_NotStarted;

	bool bNeedDestroy = false;

	if (step == CConEmuUpdate::us_ExitAndUpdate)
	{
		bNeedDestroy = true; // Иначе облом при обновлении
	}
	else if (!gpSet->isMultiLeaveOnClose)
	{
		bNeedDestroy = true;
	}
	else if (gpSet->isMultiLeaveOnClose == 1)
	{
		bNeedDestroy = ScCloseOnEmpty;
	}
	else
	{
		// Сюда мы попадаем, если просили оставлять ConEmu только если
		// закрыта была вкладка, а не нажат "крестик" в заголовке
		_ASSERTE(gpSet->isMultiLeaveOnClose == 2);
		// mb_ScClosePending выставляется в true при закрытии крестиком
		// То есть, если нажали "крестик" - вызываем закрытие окна ConEmu
		bNeedDestroy = (mb_ScClosePending || ScCloseOnEmpty);
	}

	if (gpSetCls->isAdvLogging)
	{
		wchar_t sInfo[100];
		_wsprintf(sInfo, SKIPLEN(countof(sInfo)) L"isDestroyOnClose(%u) {%u,%u,%u} -> %u",
			(UINT)ScCloseOnEmpty,
			(UINT)step, (UINT)gpSet->isMultiLeaveOnClose, (UINT)mb_ScClosePending,
			bNeedDestroy);
		LogString(sInfo);
	}

	return bNeedDestroy;
}

void CConEmuMain::OnAllVConClosed()
{
	LogString(L"CConEmuMain::OnAllVConClosed()");
	ShutdownGuiStep(L"AllVConClosed");

	OnAllGhostClosed();

	Taskbar_SetShield(false);

	if (isDestroyOnClose())
	{
		Destroy();
	}
	else
	{
		if (gpSet->isMultiHideOnClose == 1)
		{
			if (IsWindowVisible(ghWnd))
			{
				Icon.HideWindowToTray();
			}
		}
		else if (gpSet->isMultiHideOnClose == 2)
		{
			if (!gpConEmu->isIconic())
				SendMessage(ghWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
		}

		if (!mb_WorkspaceErasedOnClose)
		{
			mb_WorkspaceErasedOnClose = true;
			UpdateTitle();
			InvalidateAll();
		}
	}
}

void CConEmuMain::OnGhostCreated(CVirtualConsole* apVCon, HWND ahGhost)
{
	// При появлении на таскбаре первой кнопки с консолью (WinXP и ниже)
	// нужно убрать с него кнопку самого приложения
	// 111127 на Vista тоже кнопку "убирать" нужно
	if (gpSet->isTabsOnTaskBar() && !IsWindows7)
	{
		DWORD curStyle = GetWindowLong(ghWnd, GWL_STYLE);
		DWORD curStyleEx = GetWindowLong(ghWnd, GWL_EXSTYLE);
		DWORD style = (curStyle | WS_POPUP);
		DWORD styleEx = (curStyleEx & ~WS_EX_APPWINDOW);
		if (style != curStyle || styleEx != curStyleEx)
		{
			SetWindowStyle(style);
			SetWindowStyleEx(styleEx);
		}
		Taskbar_DeleteTabXP(ghWnd);
	}
}

void CConEmuMain::OnAllGhostClosed()
{
	if (!gpSet->isWindowOnTaskBar(true))
		return;

	DWORD curStyle = GetWindowLong(ghWnd, GWL_STYLE);
	DWORD curStyleEx = GetWindowLong(ghWnd, GWL_EXSTYLE);
	DWORD style = (curStyle & ~WS_POPUP);
	DWORD styleEx = (curStyleEx | WS_EX_APPWINDOW);
	if (style != curStyle || styleEx != curStyleEx)
	{
		SetWindowStyle(style);
		SetWindowStyleEx(styleEx);

		Taskbar_AddTabXP(ghWnd);
	}
}

void CConEmuMain::OnRConStartedSuccess(CRealConsole* apRCon)
{
	// Note, apRCon MAY be NULL
	mb_ProcessCreated = true;

	if (apRCon != NULL)
	{
		SetScClosePending(false); // сброс
	}
}

void CConEmuMain::LogWindowPos(LPCWSTR asPrefix)
{
	if (gpSetCls->isAdvLogging)
	{
		wchar_t szInfo[255];
		RECT rcWnd = {}; GetWindowRect(ghWnd, &rcWnd);
		MONITORINFO mi;
		HMONITOR hMon = GetNearestMonitor(&mi);
		_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"%s: %s %s WindowMode=%u Rect={%i,%i}-{%i,%i} Mon(x%08X)=({%i,%i}-{%i,%i}),({%i,%i}-{%i,%i})",
			asPrefix ? asPrefix : L"WindowPos",
			::IsWindowVisible(ghWnd) ? L"Visible" : L"Hidden",
			::IsIconic(ghWnd) ? L"Iconic" : ::IsZoomed(ghWnd) ? L"Maximized" : L"Normal",
			gpSet->isQuakeStyle ? gpSet->_WindowMode : WindowMode,
			rcWnd.left, rcWnd.top, rcWnd.right, rcWnd.bottom,
			(DWORD)hMon, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom,
			mi.rcWork.left, mi.rcWork.top, mi.rcWork.right, mi.rcWork.bottom);
		LogString(szInfo);
	}
}

void CConEmuMain::LogMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (!this || !mp_Log || (gpSetCls->isAdvLogging < 4))
		return;

	char szLog[128];
	#define CASE_MSG(x) case x: lstrcpynA(szLog, #x, 64); break
	#define CASE_MSG2(x,n) case x: lstrcpynA(szLog, n, 64); break
	switch (uMsg)
	{
	CASE_MSG(WM_CREATE);
	CASE_MSG(WM_WTSSESSION_CHANGE);
	CASE_MSG(WM_SHOWWINDOW);
	CASE_MSG(WM_PAINT);
	CASE_MSG(WM_SETHOTKEY);
	CASE_MSG(WM_WINDOWPOSCHANGING);
	CASE_MSG(WM_WINDOWPOSCHANGED);
	CASE_MSG(WM_SIZE);
	CASE_MSG(WM_SIZING);
	CASE_MSG(WM_MOVE);
	CASE_MSG(WM_MOVING);
	CASE_MSG(WM_MOUSEMOVE);
	CASE_MSG(WM_MOUSEWHEEL);
	CASE_MSG(WM_LBUTTONDOWN);
	CASE_MSG(WM_LBUTTONUP);
	CASE_MSG(WM_LBUTTONDBLCLK);
	CASE_MSG(WM_RBUTTONDOWN);
	CASE_MSG(WM_RBUTTONUP);
	CASE_MSG(WM_RBUTTONDBLCLK);
	CASE_MSG(WM_KEYDOWN);
	CASE_MSG(WM_KEYUP);
	CASE_MSG(WM_CHAR);
	CASE_MSG(WM_DEADCHAR);
	CASE_MSG(WM_SYSKEYDOWN);
	CASE_MSG(WM_SYSKEYUP);
	CASE_MSG(WM_SYSCHAR);
	CASE_MSG(WM_SYSDEADCHAR);
	CASE_MSG2(0x0108,"WM_x0108");
	CASE_MSG2(0x0109,"WM_UNICHAR");
	CASE_MSG(WM_COMMAND);
	CASE_MSG(WM_SYSCOMMAND);
	CASE_MSG(WM_TIMER);
	CASE_MSG(WM_VSCROLL);
	CASE_MSG(WM_GETICON);
	CASE_MSG(WM_SETTEXT);
	CASE_MSG(WM_QUERYENDSESSION);
	CASE_MSG(WM_ENDSESSION);
	default:
		{
			LPCSTR pszReg = NULL;
			if ((uMsg >= WM_APP) && m__AppMsgs.Get(uMsg, &pszReg) && pszReg && *pszReg)
				_wsprintfA(szLog, SKIPLEN(countof(szLog)) "Msg%04X(%s)", uMsg, pszReg);
			else
				_wsprintfA(szLog, SKIPLEN(countof(szLog)) "Msg%04X(%u)", uMsg, uMsg);
		}
	}
	#undef CASE_MSG

	size_t cchLen = strlen(szLog);
	_wsprintfA(szLog+cchLen, SKIPLEN(countof(szLog)-cchLen)
		": HWND=x%08X," WIN3264TEST(" W=x%08X, L=x%08X", " W=x%08X%08X, L=x%08X%08X"),
		(DWORD)hWnd, WIN3264WSPRINT(wParam), WIN3264WSPRINT(lParam));

	LogString(szLog);
}

LRESULT CConEmuMain::MainWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	if (gpConEmu)
		gpConEmu->PreWndProc(messg);

	//if (messg == WM_CREATE)
	//{
	if (ghWnd == NULL)
		ghWnd = hWnd; // ставим сразу, чтобы функции могли пользоваться
	//else if ('ghWnd DC' == NULL)
	//	'ghWnd DC' = hWnd; // ставим сразу, чтобы функции могли пользоваться
	//}

	if (hWnd == ghWnd)
		result = gpConEmu->WndProc(hWnd, messg, wParam, lParam);
	else
		result = CConEmuChild::ChildWndProc(hWnd, messg, wParam, lParam);

	return result;
}

BOOL CConEmuMain::isDialogMessage(MSG &Msg)
{
	BOOL lbDlgMsg = FALSE;
	_ASSERTE(this!=NULL && this==gpConEmu);

	HWND hDlg = NULL;

	if (ghOpWnd && IsWindow(ghOpWnd))
	{
		lbDlgMsg = gpSetCls->isDialogMessage(Msg);
	}

	if (!lbDlgMsg && mp_AttachDlg && (hDlg = mp_AttachDlg->GetHWND()))
	{
		if (IsWindow(hDlg))
			lbDlgMsg = IsDialogMessage(hDlg, &Msg);
	}

	if (!lbDlgMsg && (hDlg = mp_Find->mh_FindDlg))
	{
		if (IsWindow(hDlg))
			lbDlgMsg = IsDialogMessage(hDlg, &Msg);
	}

	return lbDlgMsg;
}

bool CConEmuMain::isSkipNcMessage(const MSG& Msg)
{
	// When some GuiChild applications has focus (e.g. PuTTY)
	// Pressing Alt+F4 enexpectedly close all ConEmu's tabs instead of active (PuTTY) only
	if ((Msg.message == WM_SYSCOMMAND) && (Msg.wParam == SC_CLOSE))
	{
		// Message was posted by system?
		if (Msg.time)
		{
			CVConGuard VCon; HWND hGuiChild;
			if ((GetActiveVCon(&VCon) >= 0) && ((hGuiChild = VCon->GuiWnd()) != NULL))
			{
				VCon->RCon()->PostConsoleMessage(hGuiChild, Msg.message, Msg.wParam, Msg.lParam);
				return true;
			}
		}
	}

	return false;
}

// Window procedure for ghWndWork
LRESULT CConEmuMain::WorkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	_ASSERTE(ghWndWork==NULL || ghWndWork==hWnd);

	// Logger
	MSG msgStr = {hWnd, uMsg, wParam, lParam};
	ConEmuMsgLogger::Log(msgStr, ConEmuMsgLogger::msgWork);

	if (gpSetCls->isAdvLogging >= 4)
	{
		gpConEmu->LogMessage(hWnd, uMsg, wParam, lParam);
	}

	if (gpConEmu)
		gpConEmu->PreWndProc(uMsg);

	switch (uMsg)
	{
	case WM_CREATE:
		if (!ghWndWork)
			ghWndWork = hWnd;
		_ASSERTE(ghWndWork == hWnd);
		break;

	case WM_PAINT:
		{
			// По идее, видимых частей ghWndWork быть не должно, но если таки есть - зальем
			PAINTSTRUCT ps = {};
			BeginPaint(hWnd, &ps);
			_ASSERTE(ghWndWork == hWnd);

			CVConGroup::PaintGaps(ps.hdc);

			//int nAppId = -1;
			//CVConGuard VCon;
			//if ((gpConEmu->GetActiveVCon(&VCon) >= 0) && VCon->RCon())
			//{
			//	nAppId = VCon->RCon()->GetActiveAppSettingsId();
			//}

			//int nColorIdx = RELEASEDEBUGTEST(0/*Black*/,1/*Blue*/);
			//HBRUSH hBrush = CreateSolidBrush(gpSet->GetColors(nAppId, !gpConEmu->isMeForeground())[nColorIdx]);
			//if (hBrush)
			//{
			//	FillRect(ps.hdc, &ps.rcPaint, hBrush);

			//	DeleteObject(hBrush);
			//}

			EndPaint(hWnd, &ps);
		} // WM_PAINT
		break;

#ifdef _DEBUG
	case WM_MOVE:
		break;
	case WM_SIZE:
		break;
#endif

	default:
		result = DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return result;
}

UINT CConEmuMain::RegisterMessage(LPCSTR asLocal, LPCWSTR asGlobal)
{
	UINT nMsg;
	if (asGlobal)
		nMsg = RegisterWindowMessage(asGlobal);
	else
		nMsg = ++mn__FirstAppMsg;
	m__AppMsgs.Set(nMsg, asLocal);
	return nMsg;
}

UINT CConEmuMain::GetRegisteredMessage(LPCSTR asLocal)
{
	UINT nMsg = 0; LPCSTR pszMsg = NULL;
	if (m__AppMsgs.GetNext(NULL, &nMsg, &pszMsg))
	{
		while (nMsg)
		{
			if (pszMsg && lstrcmpA(pszMsg, asLocal) == 0)
				return nMsg;

			if (!m__AppMsgs.GetNext(&nMsg, &nMsg, &pszMsg))
				break;
		}
	}
	return RegisterMessage(asLocal);
}

// Speed up selection modifier checks
void CConEmuMain::PreWndProc(UINT messg)
{
	m_Pressed.bChecked = FALSE;
}

// Window procedure for ghWnd
// BUT! It may be called from child windows, e.g. for keyboard processing
LRESULT CConEmuMain::WndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	//MCHKHEAP

	// Logger
	MSG msgStr = {hWnd, messg, wParam, lParam};
	ConEmuMsgLogger::Log(msgStr, ConEmuMsgLogger::msgMain);

	if (gpSetCls->isAdvLogging >= 4)
	{
		LogMessage(hWnd, messg, wParam, lParam);
	}

	#ifdef _DEBUG
	if (messg == WM_TIMER || messg == WM_GETICON)
	{
		bool lbDbg1 = false;
	}
	else if (messg >= WM_APP)
	{
		bool lbDbg2 = false;

		wchar_t szDbg[127], szName[64];
		LPCSTR pszReg = NULL;
		if (m__AppMsgs.Get(messg, &pszReg) && pszReg && *pszReg)
			MultiByteToWideChar(CP_ACP, 0, pszReg, -1, szName, countof(szName));
		else
			_wsprintf(szName, SKIPLEN(countof(szName)) L"x%03X", messg);
		 _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WndProc(%i{%s},%i,%i)\n", messg, szName, (DWORD)wParam, (DWORD)lParam);
		DEBUGSTRMSG2(szDbg);
	}
	else
	{
		wchar_t szDbg[127]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WndProc(%i{x%03X},%i,%i)\n", messg, messg, (DWORD)wParam, (DWORD)lParam);
		DEBUGSTRMSG(szDbg);
	}
	#endif

	PatchMsgBoxIcon(hWnd, messg, wParam, lParam);

	if (messg == WM_SYSCHAR)  // Для пересылки в консоль не используется, но чтобы не пищало - необходимо
		return TRUE;

	if (this->ProcessNcMessage(hWnd, messg, wParam, lParam, result))
		return result;

	if (this->ProcessGestureMessage(hWnd, messg, wParam, lParam, result))
		return result;
		
	//if (messg == WM_CHAR)
	//  return TRUE;

	switch (messg)
	{
		case WM_CREATE:
			result = this->OnCreate(hWnd, (LPCREATESTRUCT)lParam);
			break;

		case WM_SETHOTKEY:
			gnWndSetHotkeyOk = wParam;
			result = ::DefWindowProc(hWnd, messg, wParam, lParam);
			break;

		case WM_WTSSESSION_CHANGE:
			result = OnSessionChanged(hWnd, messg, wParam, lParam);
			break;

		case WM_QUERYENDSESSION:
			result = OnQueryEndSession(hWnd, messg, wParam, lParam);
			break;

		case WM_NOTIFY:
		{
			#if !defined(CONEMU_TABBAR_EX)
			if (this->mp_TabBar)
				result = this->mp_TabBar->OnNotify((LPNMHDR)lParam);
			#endif

			break;
		}
		case WM_COMMAND:
		{
			if (this->mp_TabBar)
				this->mp_TabBar->OnCommand(wParam, lParam);

			result = 0;
			break;
		}
		case WM_INITMENUPOPUP:
			return this->mp_Menu->OnInitMenuPopup(hWnd, (HMENU)wParam, lParam);
		case WM_MENUSELECT:
			this->mp_Menu->OnMenuSelected((HMENU)lParam, LOWORD(wParam), HIWORD(wParam));
			return 0;
		case WM_MENURBUTTONUP:
			this->mp_Menu->OnMenuRClick((HMENU)lParam, (UINT)wParam);
			return 0;
		case WM_PRINTCLIENT:
			DefWindowProc(hWnd, messg, wParam, lParam);
			if (wParam && (hWnd == ghWnd) && gpSet->isStatusBarShow && (lParam & PRF_CLIENT))
			{
				OnPaint(hWnd, (HDC)wParam);

				//int nHeight = gpSet->StatusBarHeight();
				//RECT wr = CalcRect(CER_MAINCLIENT);
				//RECT rcStatus = {wr.left, wr.bottom - nHeight, wr.right, wr.bottom};
				//mp_Status->PaintStatus((HDC)wParam, rcStatus);
			}
			break;
		case WM_TIMER:
			result = this->OnTimer(wParam, lParam);
			break;
		case WM_SIZING:
		{
			RECT* pRc = (RECT*)lParam;
			wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_SIZING (Edge%i, {%i-%i}-{%i-%i})\n", (DWORD)wParam, pRc->left, pRc->top, pRc->right, pRc->bottom);

			if (!isIconic())
				result = this->OnSizing(wParam, lParam);

			size_t cchLen = wcslen(szDbg);
			_wsprintf(szDbg+cchLen, SKIPLEN(countof(szDbg)-cchLen) L" -> ({%i-%i}-{%i-%i})\n", pRc->left, pRc->top, pRc->right, pRc->bottom);
			LogString(szDbg, true, false);
			DEBUGSTRSIZE(szDbg);
		} break;

		case WM_MOVING:
		{
			RECT* pRc = (RECT*)lParam;
			if (pRc)
			{
				wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_MOVING ({%i-%i}-{%i-%i})", pRc->left, pRc->top, pRc->right, pRc->bottom);

				result = this->OnMoving(pRc, true);

				size_t cchLen = wcslen(szDbg);
				_wsprintf(szDbg+cchLen, SKIPLEN(countof(szDbg)-cchLen) L" -> ({%i-%i}-{%i-%i})", pRc->left, pRc->top, pRc->right, pRc->bottom);
				LogString(szDbg, true, false);
				DEBUGSTRSIZE(szDbg);
			}
		} break;

		case WM_SHOWWINDOW:
		{
			wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_SHOWWINDOW (Show=%i, Status=%i)\r\n", (DWORD)wParam, (DWORD)lParam);
			LogString(szDbg, true, false);
			DEBUGSTRSIZE(szDbg);
			result = DefWindowProc(hWnd, messg, wParam, lParam);
			if (wParam == FALSE)
			{
				if (!Icon.isWindowInTray() && !Icon.isHidingToTray() && !this->InQuakeAnimation())
				{
					//_ASSERTE(Icon.isHidingToTray());
					Icon.HideWindowToTray(gpSet->isDownShowHiddenMessage ? NULL : _T("ConEmu was hidden from some program"));
					this->mb_ExternalHidden = TRUE;
				}
			}
			else
			{
				if (this->mb_ExternalHidden)
					Icon.RestoreWindowFromTray(TRUE);
				this->mb_ExternalHidden = FALSE;
			}

			//Логирование, что получилось
			if (gpSetCls->isAdvLogging)
			{
				LogWindowPos(L"WM_SHOWWINDOW.end");
			}
		} break;

		case WM_SIZE:
		{
			#ifdef _DEBUG
			DWORD_PTR dwStyle = GetWindowLong(ghWnd, GWL_STYLE);
			wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_SIZE (Type:x%X, {%i, %i}) style=0x%08X\n", (DWORD)wParam, LOWORD(lParam), HIWORD(lParam), (DWORD)dwStyle);
			DEBUGSTRSIZE(szDbg);
			#endif

			//if (gpSet->isDontMinimize && wParam == SIZE_MINIMIZED) {
			//	result = 0;
			//	break;
			//}
			if (!isIconic())
			{
				WORD newClientWidth = LOWORD(lParam), newClientHeight = HIWORD(lParam);
				RECT rcShift = CalcMargins(CEM_CLIENTSHIFT);
				_ASSERTE(rcShift.left==0 && rcShift.bottom==0 && rcShift.right==0); // Only top shift allowed
				if ((UINT)rcShift.top > newClientHeight)
					newClientHeight -= rcShift.top;
				result = this->OnSize(HIWORD(wParam)!=2, LOWORD(wParam), newClientWidth, newClientHeight);
			}
		} break;
		case WM_MOVE:
		{
#ifdef _DEBUG
			wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_MOVE ({%i, %i})\n", (int)(SHORT)LOWORD(lParam), (int)(SHORT)HIWORD(lParam));
			DEBUGSTRSIZE(szDbg);
#endif
		} break;
		case WM_WINDOWPOSCHANGING:
		{
			result = this->OnWindowPosChanging(hWnd, messg, wParam, lParam);
		} break;
		//case WM_NCCALCSIZE:
		//	{
		//		NCCALCSIZE_PARAMS *pParms = NULL;
		//		LPRECT pRect = NULL;
		//		if (wParam) pParms = (NCCALCSIZE_PARAMS*)lParam; else pRect = (LPRECT)lParam;
		//		result = DefWindowProc(hWnd, messg, wParam, lParam);
		//		break;
		//	}
		case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO pInfo = (LPMINMAXINFO)lParam;
			result = this->OnGetMinMaxInfo(pInfo);
			break;
		}
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
			//case WM_CHAR: -- убрал. Теперь пользуем ToUnicodeEx.
			//case WM_SYSCHAR:
			result = this->OnKeyboard(hWnd, messg, wParam, lParam);

			if (messg == WM_SYSKEYUP || messg == WM_SYSKEYDOWN)
				result = TRUE;

			//if (messg == WM_SYSCHAR)
			//    return TRUE;
			break;
#ifdef _DEBUG
		case WM_CHAR:
		case WM_SYSCHAR:
		case WM_DEADCHAR:
		case WM_SYSDEADCHAR:
		{
			wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"%s(%i='%c', Scan=%i, lParam=0x%08X)\n",
				(messg == WM_CHAR) ? L"WM_CHAR" : (messg == WM_SYSCHAR) ? L"WM_SYSCHAR" : (messg == WM_SYSDEADCHAR) ? L"WM_DEADCHAR" : L"WM_DEADCHAR",
			                              (DWORD)wParam, (wchar_t)wParam, ((DWORD)lParam & 0xFF0000) >> 16, (DWORD)lParam);
			//_ASSERTE(FALSE && "WM_CHAR, WM_SYSCHAR, WM_DEADCHAR, WM_SYSDEADCHAR must be processed internally in CConEmuMain::OnKeyboard");
			this->DebugStep(L"WM_CHAR, WM_SYSCHAR, WM_DEADCHAR, WM_SYSDEADCHAR must be processed internally in CConEmuMain::OnKeyboard", TRUE);
			DEBUGSTRCHAR(szDbg);
		}
		break;
#endif

		case WM_ACTIVATE:
			LogString((wParam == WA_CLICKACTIVE) ? L"Window was activated by mouse click" : (wParam == WA_CLICKACTIVE) ? L"Window was activated somehow" : L"Window was deactivated");
			result = this->OnFocus(hWnd, messg, wParam, lParam);
			if (this->mb_AllowAutoChildFocus)
			{
				HWND hFore = getForegroundWindow();
				CVConGuard VCon;
				// Если это наш диалог - пропустить возврат фокуса!
				if (hFore && (hFore != ghWnd) && isMeForeground(false, true))
				{
					this->mb_AllowAutoChildFocus = false; // для отладчика
				}
				else if (this->CanSetChildFocus() && (this->GetActiveVCon(&VCon) >= 0) && VCon->RCon()->GuiWnd())
				{
					VCon->PostRestoreChildFocus();
				}
				this->mb_AllowAutoChildFocus = false; // однократно
			}
			result = DefWindowProc(hWnd, messg, wParam, lParam);
			break;
		case WM_ACTIVATEAPP:
			LogString(wParam ? L"Application activating" : L"Application deactivating");
			// просто так фокус в дочернее окно ставить нельзя
			// если переключать фокус в дочернее приложение по любому чиху
			// вообще не получается активировать окно ConEmu, открыть системное меню,
			// клацнуть по крестику в заголовке и т.п.
			if (wParam && lParam && gpSet->isFocusInChildWindows)
			{
				CheckAllowAutoChildFocus((DWORD)lParam);
			}
			result = this->OnFocus(hWnd, messg, wParam, lParam);
			result = DefWindowProc(hWnd, messg, wParam, lParam);
			break;
		case WM_KILLFOCUS:
			result = this->OnFocus(hWnd, messg, wParam, lParam);
			result = DefWindowProc(hWnd, messg, wParam, lParam);
			break;
		case WM_SETFOCUS:
			result = this->OnFocus(hWnd, messg, wParam, lParam);
			result = DefWindowProc(hWnd, messg, wParam, lParam); //-V519
			break;
		case WM_MOUSEACTIVATE:
			LogString(L"Activation by mouse");
			// просто так фокус в дочернее окно ставить нельзя
			// если переключать фокус в дочернее приложение по любому чиху
			// вообще не получается активировать окно ConEmu, открыть системное меню,
			// клацнуть по крестику в заголовке и т.п.
			if (wParam && lParam && gpSet->isFocusInChildWindows)
			{
				CheckAllowAutoChildFocus((DWORD)lParam);
			}

			//return MA_ACTIVATEANDEAT; -- ест все подряд, а LBUTTONUP пропускает :(
			this->mouse.nSkipEvents[0] = 0;
			this->mouse.nSkipEvents[1] = 0;

			if (this->mouse.bForceSkipActivation  // принудительная активация окна, лежащего на Desktop
			        || (gpSet->isMouseSkipActivation && LOWORD(lParam) == HTCLIENT
					&& !isMeForeground(false,false)))
			{
				this->mouse.bForceSkipActivation = FALSE; // Однократно
				POINT ptMouse = {0}; GetCursorPos(&ptMouse);
				//RECT  rcDC = {0}; GetWindowRect('ghWnd DC', &rcDC);
				//if (PtInRect(&rcDC, ptMouse))
				CVirtualConsole* pVCon = GetVConFromPoint(ptMouse);
				if (pVCon)
				{
					if (HIWORD(lParam) == WM_LBUTTONDOWN)
					{
						this->mouse.nSkipEvents[0] = WM_LBUTTONDOWN;
						this->mouse.nSkipEvents[1] = WM_LBUTTONUP;
						this->mouse.nReplaceDblClk = WM_LBUTTONDBLCLK;
					}
					else if (HIWORD(lParam) == WM_RBUTTONDOWN)
					{
						this->mouse.nSkipEvents[0] = WM_RBUTTONDOWN;
						this->mouse.nSkipEvents[1] = WM_RBUTTONUP;
						this->mouse.nReplaceDblClk = WM_RBUTTONDBLCLK;
					}
					else if (HIWORD(lParam) == WM_MBUTTONDOWN)
					{
						this->mouse.nSkipEvents[0] = WM_MBUTTONDOWN;
						this->mouse.nSkipEvents[1] = WM_MBUTTONUP;
						this->mouse.nReplaceDblClk = WM_MBUTTONDBLCLK;
					}
				}
			}

			if (this->mp_Inside)
			{
				apiSetForegroundWindow(ghWnd);
			}

			result = DefWindowProc(hWnd, messg, wParam, lParam);
			break;

		case WM_MOUSEMOVE:
		case WM_MOUSEWHEEL:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
		case WM_XBUTTONDBLCLK:
			result = this->OnMouse(hWnd, messg, wParam, lParam);
			break;
		case WM_CLOSE:
			_ASSERTE(FALSE && "WM_CLOSE is not called in normal behavior");
			this->OnScClose();
			result = 0;
			break;
		case WM_SYSCOMMAND:
			result = this->mp_Menu->OnSysCommand(hWnd, wParam, lParam);
			break;
		case WM_NCLBUTTONDOWN:
			// Note: При ресайзе WM_NCLBUTTONUP к сожалению не приходит
			// поэтому нужно запомнить, что был начат ресайз и при завершении
			// возможно выполнить дополнительные действия

			BeginSizing(false);

			result = DefWindowProc(hWnd, messg, wParam, lParam);
			break;

		case WM_NCLBUTTONDBLCLK:
			this->mouse.state |= MOUSE_SIZING_DBLCKL; // чтобы в консоль не провалился LBtnUp если окошко развернется

			if (this->OnMouse_NCBtnDblClk(hWnd, messg, wParam, lParam))
				result = 0;
			else
				result = DefWindowProc(hWnd, messg, wParam, lParam);

			break;

		case WM_TRAYNOTIFY:
			result = Icon.OnTryIcon(hWnd, messg, wParam, lParam);
			break;

		case WM_HOTKEY:
			this->OnWmHotkey(wParam);
			return 0;

		case WM_SETCURSOR:
			result = this->OnSetCursor(wParam, lParam);

			if (!result && (lParam != -1))
			{
				// Установка resize-курсоров на рамке, и т.п.
				result = DefWindowProc(hWnd, messg, wParam, lParam);
			}

			MCHKHEAP;
			// If an application processes this message, it should return TRUE to halt further processing or FALSE to continue.
			return result;
		case WM_DESTROY:
			result = this->OnDestroy(hWnd);
			break;
		case WM_IME_CHAR:
		case WM_IME_COMPOSITION:
		case WM_IME_COMPOSITIONFULL:
		case WM_IME_CONTROL:
		case WM_IME_ENDCOMPOSITION:
		case WM_IME_KEYDOWN:
		case WM_IME_KEYUP:
		case WM_IME_NOTIFY:
		case WM_IME_REQUEST:
		case WM_IME_SELECT:
		case WM_IME_SETCONTEXT:
		case WM_IME_STARTCOMPOSITION:
			result = this->OnKeyboardIme(hWnd, messg, wParam, lParam);
			break;
		case WM_INPUTLANGCHANGE:
		case WM_INPUTLANGCHANGEREQUEST:

			if (hWnd == ghWnd)
				result = this->OnLangChange(messg, wParam, lParam);
			else
				break;

			break;
			//case WM_NCHITTEST:
			//	{
			//		/*result = -1;
			//		if (gpSet->isHideCaptionAlways && gpSet->isTabs) {
			//			if (this->mp_TabBar->IsShown()) {
			//				HWND hTabBar = this->mp_TabBar->GetTabbar();
			//				RECT rcWnd; GetWindowRect(hTabBar, &rcWnd);
			//				TCHITTESTINFO tch = {{(int)(short)LOWORD(lParam),(int)(short)HIWORD(lParam)}};
			//				if (PtInRect(&rcWnd, tch.pt)) {
			//					// Преобразовать в относительные координаты
			//					tch.pt.x -= rcWnd.left; tch.pt.y -= rcWnd.top;
			//					LRESULT nTest = SendMessage(hTabBar, TCM_HITTEST, 0, (LPARAM)&tch);
			//					if (nTest == -1) {
			//						result = HTCAPTION;
			//					}
			//				}
			//			}
			//		}
			//		if (result == -1)*/
			//		result = DefWindowProc(hWnd, messg, wParam, lParam);
			//		if (gpSet->isHideCaptionAlways() && !this->mp_TabBar->IsShown() && result == HTTOP)
			//			result = HTCAPTION;
			//	} break;
		default:

			if (messg == this->mn_MsgPostCreate)
			{
				this->PostCreate(TRUE);
				return 0;
			}
			else if (messg == this->mn_MsgPostCopy)
			{
				this->PostDragCopy(lParam!=0, TRUE);
				return 0;
			}
			else if (messg == this->mn_MsgMyDestroy)
			{
				ShutdownGuiStep(L"DestroyWindow");
				//this->OnDestroy(hWnd);
				_ASSERTE(hWnd == ghWnd);
				DestroyWindow(hWnd);
				return 0;
			}
			else if (messg == this->mn_MsgUpdateSizes)
			{
				this->UpdateSizes();
				return 0;
			}
			else if (messg == this->mn_MsgUpdateCursorInfo)
			{
				COORD cr; cr.X = LOWORD(wParam); cr.Y = HIWORD(wParam);
				CONSOLE_CURSOR_INFO ci; ci.dwSize = LOWORD(lParam); ci.bVisible = HIWORD(lParam);
				this->UpdateCursorInfo(NULL, cr, ci);
				return 0;
			}
			else if (messg == this->mn_MsgSetWindowMode)
			{
				this->SetWindowMode((ConEmuWindowMode)wParam);
				return 0;
			}
			else if (messg == this->mn_MsgUpdateTitle)
			{
				this->UpdateTitle();
				return 0;
			}
			else if (messg == this->mn_MsgSrvStarted)
			{
				MsgSrvStartedArg *pArg = (MsgSrvStartedArg*)lParam;
				HWND hWndDC = NULL, hWndBack = NULL;
				//111002 - вернуть должен HWND окна отрисовки (дочернее окно ConEmu)

				DWORD nServerPID = pArg->nSrcPID;
				HWND  hWndCon = pArg->hConWnd;
				DWORD dwKeybLayout = pArg->dwKeybLayout;
				pArg->timeRecv = timeGetTime();

				DWORD t1, t2, t3; int iFound = -1;

				hWndDC = CVConGroup::DoSrvCreated(nServerPID, hWndCon, dwKeybLayout, t1, t2, t3, iFound, hWndBack);

				pArg->hWndDc = hWndDC;
				pArg->hWndBack = hWndBack;

				UNREFERENCED_PARAMETER(dwKeybLayout);
				UNREFERENCED_PARAMETER(hWndCon);

				pArg->timeFin = timeGetTime();
				if (hWndDC == NULL)
				{
					_ASSERTE(hWndDC!=NULL);
				}
				else
				{
					#ifdef _DEBUG
					DWORD nRecvDur = pArg->timeRecv - pArg->timeStart;
					DWORD nProcDur = pArg->timeFin - pArg->timeRecv;
					
					#define MSGSTARTED_TIMEOUT 10000
					if ((nRecvDur > MSGSTARTED_TIMEOUT) || (nProcDur > MSGSTARTED_TIMEOUT))
					{
						_ASSERTE((nRecvDur <= MSGSTARTED_TIMEOUT) && (nProcDur <= MSGSTARTED_TIMEOUT));
					}
					#endif
				}

				return (LRESULT)hWndDC;
			}
			else if (messg == this->mn_MsgUpdateScrollInfo)
			{
				return OnUpdateScrollInfo(TRUE);
			}
			else if (messg == this->mn_MsgUpdateTabs)
			{
				DEBUGSTRTABS(L"OnUpdateTabs\n");
				this->mp_TabBar->Update(TRUE);
				return 0;
			}
			else if (messg == this->mn_MsgOldCmdVer)
			{
				CESERVER_REQ_HDR* p = (CESERVER_REQ_HDR*)lParam;
				_ASSERTE(p->cbSize == sizeof(CESERVER_REQ_HDR));
				this->ReportOldCmdVersion(p->nCmd, p->nVersion, (int)wParam, p->nSrcPID, p->hModule, p->nBits);
				return 0;
			}
			else if (messg == this->mn_MsgTabCommand)
			{
				this->TabCommand((ConEmuTabCommand)(int)wParam);
				return 0;
			}
			else if (messg == this->mn_MsgTabSwitchFromHook)
			{
				this->TabCommand((wParam & cvk_Shift) ? ctc_SwitchPrev : ctc_SwitchNext);
				if (!gpSet->isTabLazy)
				{
					mb_InWinTabSwitch = FALSE;
					this->TabCommand(ctc_SwitchCommit);
				}
				else
				{
					mb_InWinTabSwitch = TRUE;
				}
				return 0;
			}
			else if (messg == this->mn_MsgWinKeyFromHook)
			{
				// Тут должно передавать ВСЕ модификаторы!
				DWORD VkMod = (DWORD)wParam;
				this->OnKeyboardHook(VkMod);
				//BOOL lbShift = (lParam!=0);
				//OnKeyboardHook(vk, lbShift);
				return 0;
			}
			#if 0
			else if (messg == this->mn_MsgConsoleHookedKey)
			{
				WORD vk = LOWORD(wParam);
				this->OnConsoleKey(vk, lParam);
				return 0;
			}
			#endif
			else if (messg == this->mn_MsgSheelHook)
			{
				this->OnShellHook(wParam, lParam);
				return 0;
			}
			//else if (messg == this->mn_ShellExecuteEx)
			//{
			//	this->GuiShellExecuteExQueue();
			//	return 0;
			//	//GuiShellExecuteExArg* pArg = (GuiShellExecuteExArg*)wParam;
			//	//if (pArg && (wParam != TRUE))
			//	//{
			//	//	_ASSERTE(pArg->lpShellExecute == (SHELLEXECUTEINFO*)lParam);
			//	//	result = this->GuiShellExecuteEx(pArg->lpShellExecute, pArg->pVCon);
			//	//	SafeFree(pArg);
			//	//}
			//	//return result;
			//}
			else if (messg == this->mn_PostConsoleResize)
			{
				this->OnConsoleResize(TRUE);
				return 0;
			}
			else if (messg == this->mn_ConsoleLangChanged)
			{
				this->OnLangChangeConsole((CVirtualConsole*)lParam, (DWORD)wParam);
				return 0;
			}
			else if (messg == this->mn_MsgPostOnBufferHeight)
			{
				CVConGuard VCon;
				if (this->GetActiveVCon(&VCon) >= 0)
				{
					VCon->RCon()->OnBufferHeight();
				}
				return 0;
				//} else if (messg == this->mn_MsgSetForeground) {
				//	apiSetForegroundWindow((HWND)lParam);
				//	return 0;
			}
			else if (messg == this->mn_MsgFlashWindow)
			{
				return OnFlashWindow(wParam, lParam);
			}
			else if (messg == this->mn_MsgPostAltF9)
			{
				DoMaximizeRestore();
				return 0;
			}
			else if (messg == this->mn_MsgInitInactiveDC)
			{
				CVirtualConsole* pVCon = (CVirtualConsole*)lParam;
				if (isValid(pVCon) && !isActive(pVCon))
				{
					CVConGuard guard(pVCon);
					pVCon->InitDC(true, true, NULL, NULL);
					pVCon->LoadConsoleData();
				}

				return 0;
			}
			else if (messg == this->mn_MsgUpdateProcDisplay)
			{
				this->UpdateProcessDisplay(wParam!=0);
				return 0;
			}
			else if (messg == this->mn_MsgAutoSizeFont)
			{
				gpSetCls->MacroFontSetSize((int)wParam, (int)lParam);
				return 0;
			}
			else if (messg == this->mn_MsgMacroFontSetName)
			{
				this->PostMacroFontSetName((wchar_t*)lParam, (WORD)(((DWORD)wParam & 0xFFFF0000)>>16), (WORD)(wParam & 0xFFFF), TRUE);
				return 0;
			}
			else if (messg == this->mn_MsgReqChangeCurPalette)
			{
				this->PostChangeCurPalette((LPCWSTR)lParam, (wParam!=0), true);
				return 0;
			}
			else if (messg == this->mn_MsgMacroExecSync)
			{
				return ConEmuMacro::ExecuteMacroSync(wParam, lParam);
			}
			else if (messg == this->mn_MsgDisplayRConError)
			{
				CRealConsole *pRCon = (CRealConsole*)wParam;
				wchar_t* pszErrMsg = (wchar_t*)lParam;

				if (this->isValid(pRCon))
				{
					CVConGuard VCon(pRCon->VCon());
					MessageBox(ghWnd, pszErrMsg, this->GetLastTitle(), MB_ICONSTOP|MB_SYSTEMMODAL);
				}

				free(pszErrMsg);
				return 0;
			}
			else if (messg == this->mn_MsgCreateViewWindow)
			{
				CConEmuChild* pChild = (CConEmuChild*)lParam;
				HWND hChild = pChild ? pChild->CreateView() : NULL;
				return (LRESULT)hChild;
			}
			else if (messg == this->mn_MsgPostTaskbarActivate)
			{
				mb_PostTaskbarActivate = FALSE;
				HWND hFore = GetForegroundWindow();
				CVConGuard VCon;
				if ((this->GetActiveVCon(&VCon) >= 0) && (hFore == ghWnd))
					VCon->OnTaskbarFocus();
				return 0;
			}
			else if (messg == this->mn_MsgInitVConGhost)
			{
				CVirtualConsole* pVCon = (CVirtualConsole*)lParam;
				if (this->isValid(pVCon))
					pVCon->InitGhost();
				return 0;
			}
			else if (messg == this->mn_MsgCreateCon)
			{
				_ASSERTE(wParam == this->mn_MsgCreateCon || wParam == (this->mn_MsgCreateCon+1));
				RConStartArgs *pArgs = (RConStartArgs*)lParam;
				CVirtualConsole* pVCon = CreateCon(pArgs, (wParam == (this->mn_MsgCreateCon+1)));
				UNREFERENCED_PARAMETER(pVCon);
				delete pArgs;
				return (LRESULT)pVCon;
			}
			else if (messg == this->mn_MsgRequestUpdate)
			{
				switch (wParam)
				{
				case 0:
					this->ReportUpdateError();
					return 0;
				case 1:
					return (LRESULT)this->ReportUpdateConfirmation();
				case 2:
					this->RequestExitUpdate();
					return 0;
				}
				return 0;
			}
			else if (messg == this->mn_MsgTaskBarCreated)
			{
				this->OnTaskbarCreated();
				result = DefWindowProc(hWnd, messg, wParam, lParam);
				return result;
			}
			else if (messg == this->mn_MsgTaskBarBtnCreated)
			{
				this->OnTaskbarButtonCreated();
				result = DefWindowProc(hWnd, messg, wParam, lParam);
				return result;
			}
			else if (messg == this->mn_MsgRequestRunProcess)
			{
				this->mp_RunQueue->ProcessRunQueue(true);
				return 0;
			}
			else if (messg == this->mn_MsgDeleteVConMainThread)
			{
				// Танцы с бубном, если последний Release() пришелся в фоновом потоке
				LRESULT lDelRc = 0;
				CVirtualConsole* pVCon = (CVirtualConsole*)lParam;
				if (CVConGroup::isValid(pVCon))
				{
					pVCon->DeleteFromMainThread();
					lDelRc = 1;
				}
				return lDelRc;
			}
			else if (messg == this->mn_MsgActivateVCon)
			{
				LRESULT lActivateRc = 0;
				CVirtualConsole* pVCon = (CVirtualConsole*)lParam;
				if (CVConGroup::isValid(pVCon))
				{
					BOOL bActivateRc = this->Activate(pVCon);
					if (bActivateRc)
						lActivateRc = (LRESULT)pVCon;
				}
				return lActivateRc;
			}

			//else if (messg == this->mn_MsgCmdStarted || messg == this->mn_MsgCmdStopped) {
			//  return this->OnConEmuCmd( (messg == this->mn_MsgCmdStarted), (HWND)wParam, (DWORD)lParam);
			//}


			if (messg)
			{
				// 0x0313 - Undocumented. When you right-click on a taskbar button,
				// Windows sends an undocumented message ($0313) to the corresponding
				// application window. The WPARAM is unused (zero) and the
				// LPARAM contains the mouse position in screen coordinates, in the usual
				// format. By default, WindowProc handles this message by popping up the
				// system menu at the given coordinates.
				if ((messg == WM_CONTEXTMENU) || (messg == 0x0313))
				{
					mp_Menu->SetTrackMenuPlace(tmp_System);
					mp_Tip->HideTip();
				}

				result = DefWindowProc(hWnd, messg, wParam, lParam);

				if ((messg == WM_CONTEXTMENU) || (messg == 0x0313))
				{
					mp_Menu->SetTrackMenuPlace(tmp_None);
					mp_Tip->HideTip();
				}
			}
	}

	return result;
}
