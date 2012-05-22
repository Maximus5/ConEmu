
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

#define HIDE_USE_EXCEPTION_INFO
#ifdef _DEBUG
//  #define SHOW_GUIATTACH_START
#endif


#define SHOWDEBUGSTR

#define CHILD_DESK_MODE
#define REGPREPARE_EXTERNAL

#include "Header.h"
#include "About.h"
#include <Tlhelp32.h>
#include <Shlobj.h>
//#include <lm.h>
//#include "../common/ConEmuCheck.h"
#include "VirtualConsole.h"
#include "RealBuffer.h"
#include "options.h"
#include "DragDrop.h"
#include "TrayIcon.h"
#include "VConChild.h"
#include "GestureEngine.h"
#include "ConEmu.h"
#include "ConEmuApp.h"
#include "tabbar.h"
#include "ConEmuPipe.h"
#include "version.h"
#include "Macro.h"
#include "Attach.h"
#include "Recreate.h"
#include "Update.h"
#include "LoadImg.h"
#include "../ConEmuCD/RegPrepare.h"
#include "../ConEmuCD/GuiHooks.h"
#include "../common/execute.h"
//#ifdef __GNUC__
#include "ShObjIdl_Part.h"
//#endif

#define DEBUGSTRSYS(s) //DEBUGSTR(s)
#define DEBUGSTRSIZE(s) //DEBUGSTR(s)
#define DEBUGSTRCONS(s) //DEBUGSTR(s)
#define DEBUGSTRTABS(s) //DEBUGSTR(s)
#define DEBUGSTRLANG(s) //DEBUGSTR(s)// ; Sleep(2000)
#define DEBUGSTRMOUSE(s) DEBUGSTR(s)
#define DEBUGSTRRCLICK(s) DEBUGSTR(s)
#define DEBUGSTRKEY(s) //DEBUGSTR(s)
#define DEBUGSTRIME(s) //DEBUGSTR(s)
#define DEBUGSTRCHAR(s) //DEBUGSTR(s)
#define DEBUGSTRSETCURSOR(s) //OutputDebugString(s)
#define DEBUGSTRCONEVENT(s) //DEBUGSTR(s)
#define DEBUGSTRMACRO(s) //DEBUGSTR(s)
#define DEBUGSTRPANEL(s) //DEBUGSTR(s)
#define DEBUGSTRPANEL2(s) //DEBUGSTR(s)
#define DEBUGSTRFOCUS(s) //DEBUGSTR(s)
#define DEBUGSTRFOREGROUND(s) //DEBUGSTR(s)
#define DEBUGSTRLLKB(s) //DEBUGSTR(s)
#define DEBUGSTRTIMER(s) //DEBUGSTR(s)
#ifdef _DEBUG
//#define DEBUGSHOWFOCUS(s) DEBUGSTR(s)
#endif

//#define PROCESS_WAIT_START_TIME 1000

#define PTDIFFTEST(C,D) PtDiffTest(C, ptCur.x, ptCur.y, D)
//(((abs(C.x-(short)LOWORD(lParam)))<D) && ((abs(C.y-(short)HIWORD(lParam)))<D))



#if defined(__GNUC__)
#define EXT_GNUC_LOG
#endif


#define TIMER_MAIN_ID 0
#define TIMER_CONREDRAW_ID 1
#define TIMER_CAPTION_APPEAR_ID 3
#define TIMER_CAPTION_DISAPPEAR_ID 4
#define TIMER_RCLICKPAINT 5
#define TIMER_RCLICKPAINT_ELAPSE 20

#define HOTKEY_CTRLWINALTSPACE_ID 0x0201 // this is wParam for WM_HOTKEY
#define HOTKEY_MINRESTORE_ID      0x1001 // this is wParam for WM_HOTKEY

#define RCLICKAPPSTIMEOUT 600
#define RCLICKAPPS_START 200 // начало отрисовки кружка вокруг курсора
#define RCLICKAPPSTIMEOUT_MAX 10000
#define RCLICKAPPSDELTA 3

#define TOUCH_DBLCLICK_DELTA 1000 // 1sec

const wchar_t* gsHomePage = L"http://conemu-maximus5.googlecode.com";
const wchar_t* gsReportBug = L"http://code.google.com/p/conemu-maximus5/issues/entry";


CConEmuMain::CConEmuMain()
{
	gpConEmu = this; // сразу!

	_ASSERTE(gOSVer.dwMajorVersion>=5);
	//memset(&gOSVer,0,sizeof(gOSVer));
	//gOSVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	//GetVersionEx(&gOSVer);

	mb_CommCtrlsInitialized = false;
	mh_AboutDlg = NULL;

	//HeapInitialize(); - уже
	//#define D(N) (1##N-100)
	_wsprintf(ms_ConEmuVer, SKIPLEN(countof(ms_ConEmuVer)) L"ConEmu %02u%02u%02u%s", (MVV_1%100),MVV_2,MVV_3,_T(MVV_4a));
	mp_TabBar = NULL; /*m_Macro = NULL;*/ mp_Tip = NULL;
	ms_ConEmuAliveEvent[0] = 0;	mb_AliveInitialized = FALSE; mh_ConEmuAliveEvent = NULL; mb_ConEmuAliveOwned = FALSE;
	mn_MainThreadId = GetCurrentThreadId();
	//wcscpy_c(szConEmuVersion, L"?.?.?.?");
	WindowMode = rNormal; WindowStartMinimized = false; ForceMinimizeToTray = false;
	DisableAutoUpdate = false;
	mb_PassSysCommand = false; change2WindowMode = -1;
	mb_isFullScreen = false;
	mb_ExternalHidden = FALSE;
	memset(&mrc_StoredNormalRect, 0, sizeof(mrc_StoredNormalRect));
	isWndNotFSMaximized = false;
	mb_StartDetached = FALSE;
	mb_SkipSyncSize = false;
	isPiewUpdate = false; //true; --Maximus5
	gbPostUpdateWindowSize = false;
	hPictureView = NULL;  mrc_WndPosOnPicView = MakeRect(0,0);
	bPicViewSlideShow = false;
	dwLastSlideShowTick = 0;
	mh_ShellWindow = NULL; mn_ShellWindowPID = 0;
	mb_FocusOnDesktop = TRUE;
	cursor.x=0; cursor.y=0; Rcursor=cursor;
	m_LastConSize = MakeCoord(0,0);
	mp_DragDrop = NULL;
	//mb_InConsoleResize = FALSE;
	//ProgressBars = NULL;
	//cBlinkShift=0;
	mh_DebugPopup = mh_EditPopup = mh_ActiveVConPopup = mh_TerminateVConPopup = mh_VConListPopup = mh_HelpPopup = NULL;
	Title[0] = 0; TitleCmp[0] = 0; /*MultiTitle[0] = 0;*/ mn_Progress = -1;
	mb_InTimer = FALSE;
	//mb_InClose = FALSE;
	//memset(m_ProcList, 0, 1000*sizeof(DWORD));
	m_ProcCount=0;
	mb_ProcessCreated = FALSE; /*mn_StartTick = 0;*/ mb_WorkspaceErasedOnClose = FALSE;
	mb_IgnoreSizeChange = false;
	//mb_IgnoreStoreNormalRect = false;
	//mn_CurrentKeybLayout = (DWORD_PTR)GetKeyboardLayout(0);
	//mpsz_RecreateCmd = NULL;
	mb_InImeComposition = false; mb_ImeMethodChanged = false;
	ZeroStruct(mrc_Ideal);
	mn_InResize = 0;
	mb_MaximizedHideCaption = FALSE;
	mb_InRestore = FALSE;
	mb_MouseCaptured = FALSE;
	mb_HotKeyRegistered = FALSE;
	mn_MinRestoreRegistered = 0; mn_MinRestore_VK = mn_MinRestore_MOD = 0;
	mh_LLKeyHookDll = NULL;
	mph_HookedGhostWnd = NULL;
	mh_LLKeyHook = NULL;
	//mh_DwmApi = NULL; DwmIsCompositionEnabled = NULL;
	mh_RightClickingBmp = NULL; mh_RightClickingDC = NULL; mb_RightClickingPaint = mb_RightClickingLSent = FALSE;
	m_RightClickingSize.x = m_RightClickingSize.y = m_RightClickingFrames = 0; m_RightClickingCurrent = -1;
	mh_RightClickingWnd = NULL; mb_RightClickingRegistered = FALSE;
	mb_WaitCursor = FALSE;
	//mb_InTrackSysMenu = FALSE;
	mn_TrackMenuPlace = tmp_None;
	mb_LastRgnWasNull = TRUE;
	mb_CaptionWasRestored = FALSE; mb_ForceShowFrame = FALSE;
	mh_CursorWait = LoadCursor(NULL, IDC_WAIT);
	mh_CursorArrow = LoadCursor(NULL, IDC_ARROW);
	mh_CursorMove = LoadCursor(NULL, IDC_SIZEALL);
	mh_DragCursor = NULL;
	mh_CursorAppStarting = LoadCursor(NULL, IDC_APPSTARTING);
	// g_hInstance еще не инициализирован
	mh_SplitV = LoadCursor(GetModuleHandle(0), MAKEINTRESOURCE(IDC_SPLITV));
	mh_SplitH = LoadCursor(GetModuleHandle(0), MAKEINTRESOURCE(IDC_SPLITH));
	//ms_LogCreateProcess[0] = 0; mb_CreateProcessLogged = false;
	memset(&mouse, 0, sizeof(mouse));
	mouse.lastMMW=-1;
	mouse.lastMML=-1;
	memset(m_TranslatedChars, 0, sizeof(m_TranslatedChars));
	//GetKeyboardState(m_KeybStates);
	//memset(m_KeybStates, 0, sizeof(m_KeybStates));
	m_ActiveKeybLayout = GetActiveKeyboardLayout();
	memset(m_LayoutNames, 0, sizeof(m_LayoutNames));
	//wchar_t szTranslatedChars[16];
	//HKL hkl = (HKL)GetActiveKeyboardLayout();
	//int nTranslatedChars = ToUnicodeEx(0, 0, m_KeybStates, szTranslatedChars, 15, 0, hkl);
	mn_LastPressedVK = 0;
	memset(&m_GuiInfo, 0, sizeof(m_GuiInfo));
	m_GuiInfo.cbSize = sizeof(m_GuiInfo);
	//mh_RecreatePasswFont = NULL;
	mb_SkipOnFocus = FALSE;

	mps_IconPath = NULL;

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
	ms_ConEmuExe[0] = ms_ConEmuExeDir[0] = ms_ConEmuBaseDir[0] = 0;
	//ms_ConEmuCExe[0] = 
	ms_ConEmuC32Full[0] = ms_ConEmuC64Full[0] = 0;
	ms_ConEmuXml[0] = ms_ConEmuChm[0] = 0;
	//ms_ConEmuCExeName[0] = 0;
	wchar_t *pszSlash = NULL;

	if (!GetModuleFileName(NULL, ms_ConEmuExe, MAX_PATH) || !(pszSlash = wcsrchr(ms_ConEmuExe, L'\\')))
	{
		DisplayLastError(L"GetModuleFileName failed");
		TerminateProcess(GetCurrentProcess(), 100);
		return;
	}

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

	//LoadVersionInfo(ms_ConEmuExe);
	// Папка программы
	wcscpy_c(ms_ConEmuExeDir, ms_ConEmuExe);
	pszSlash = wcsrchr(ms_ConEmuExeDir, L'\\');
	*pszSlash = 0;
	bool lbBaseFound = false;
	wchar_t szBaseFile[MAX_PATH+12];
	wcscpy_c(szBaseFile, ms_ConEmuExeDir);
	// Сначала проверяем подпапку
	pszSlash = szBaseFile + _tcslen(szBaseFile);
	lstrcpy(pszSlash, L"\\ConEmu\\ConEmuC.exe");

	if (FileExists(szBaseFile))
	{
		wcscpy_c(ms_ConEmuBaseDir, ms_ConEmuExeDir);
		wcscat_c(ms_ConEmuBaseDir, L"\\ConEmu");
		lbBaseFound = true;
	}

#ifdef WIN64

	if (!lbBaseFound)
	{
		lstrcpy(pszSlash, L"\\ConEmu\\ConEmuC64.exe");

		if (FileExists(szBaseFile))
		{
			lstrcpy(ms_ConEmuBaseDir, ms_ConEmuExeDir);
			lstrcat(ms_ConEmuBaseDir, L"\\ConEmu");
			lbBaseFound = true;
		}
	}

#endif

	if (!lbBaseFound)
	{
		wcscpy_c(ms_ConEmuBaseDir, ms_ConEmuExeDir);
	}

	// Добавить в окружение переменную с папкой к ConEmu.exe
	SetEnvironmentVariable(L"ConEmuDir", ms_ConEmuExeDir);
	SetEnvironmentVariable(L"ConEmuBaseDir", ms_ConEmuBaseDir);
	// переменная "ConEmuArgs" заполняется в ConEmuApp.cpp:PrepareCommandLine

	// Ищем файл портабельных настроек. Сначала пробуем в BaseDir
	ConEmuXml();
	//lstrcpy(ms_ConEmuXml, ms_ConEmuBaseDir); lstrcat(ms_ConEmuXml, L"\\ConEmu.xml");
	//if (!FileExists(ms_ConEmuXml))
	//{
	//	if (lstrcmp(ms_ConEmuBaseDir, ms_ConEmuExeDir))
	//	{
	//		lstrcpy(ms_ConEmuXml, ms_ConEmuExeDir); lstrcat(ms_ConEmuXml, L"\\ConEmu.xml");
	//	}
	//}
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

	//// Если ConEmu.exe запущен с сетевого ресурса -  Сетевые пути не менять
	//if (ms_ConEmuExe[0] == L'\\' /*|| wcschr(ms_ConEmuExe, L' ') == NULL*/)
	//{
	//	wcscpy_c(ms_ConEmuCExe, ms_ConEmuCExeFull);
	//}
	//else
	//{
	//	wchar_t* pszShort = GetShortFileNameEx(ms_ConEmuCExeFull);

	//	if (pszShort)
	//	{
	//		wcscpy_c(ms_ConEmuCExe, pszShort);
	//		free(pszShort);
	//	}
	//	else
	//	{
	//		wcscpy_c(ms_ConEmuCExe, ms_ConEmuCExeFull);
	//	}

	//	//pszSlash = ms_ConEmuCExe + _tcslen(ms_ConEmuCExe);
	//	//if (*(pszSlash-1) != L'\\')
	//	//{
	//	//	*(pszSlash++) = L'\\'; *pszSlash = 0;
	//	//}
	//}

	//WARNING("Убрать ConEmuC64.exe - научиться работать с редиректором");
	//if (IsWindows64())
	//{
	//	wcscpy(pszSlash, L"ConEmuC64.exe");
	//	if (!FileExists(ms_ConEmuCExe)) // попробовать "ConEmuC.exe"
	//	{
	//		wcscpy(pszSlash, L"ConEmuC.exe");
	//		if (!FileExists(ms_ConEmuCExe)) // вернуть 64 взад, раз 32 не нашли
	//			wcscpy(pszSlash, L"ConEmuC64.exe");
	//	}
	//}
	//else
	//{
	//	wcscpy(pszSlash, L"ConEmuC.exe");
	//}
	//wcscpy(ms_ConEmuCExeName, pszSlash);
	// Запомнить текущую папку (на момент запуска)
	DWORD nDirLen = GetCurrentDirectory(MAX_PATH, ms_ConEmuCurDir);

	if (!nDirLen || nDirLen>MAX_PATH)
	{
		ms_ConEmuCurDir[0] = 0;
	}
	else if (ms_ConEmuCurDir[nDirLen-1] == L'\\')
	{
		ms_ConEmuCurDir[nDirLen-1] = 0; // пусть будет БЕЗ слеша, для однообразия с ms_ConEmuExeDir
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
	mp_VActive = NULL; mp_VCon1 = NULL; mp_VCon2 = NULL; mb_CreatingActive = false;
	memset(mp_VCon, 0, sizeof(mp_VCon));
	mp_AttachDlg = NULL;
	mp_RecreateDlg = NULL;
	UINT nAppMsg = WM_APP+10;
	mn_MsgPostCreate = ++nAppMsg;
	mn_MsgPostCopy = ++nAppMsg;
	mn_MsgMyDestroy = ++nAppMsg;
	mn_MsgUpdateSizes = ++nAppMsg;
	mn_MsgUpdateCursorInfo = ++nAppMsg;
	mn_MsgSetWindowMode = ++nAppMsg;
	mn_MsgUpdateTitle = ++nAppMsg;
	//mn_MsgAttach = RegisterWindowMessage(CONEMUMSG_ATTACH);
	mn_MsgSrvStarted = ++nAppMsg; //RegisterWindowMessage(CONEMUMSG_SRVSTARTED);
	mn_MsgVConTerminated = ++nAppMsg;
	mn_MsgUpdateScrollInfo = ++nAppMsg;
	mn_MsgUpdateTabs = RegisterWindowMessage(CONEMUMSG_UPDATETABS);
	mn_MsgOldCmdVer = ++nAppMsg; mb_InShowOldCmdVersion = FALSE;
	mn_MsgTabCommand = ++nAppMsg;
	mn_MsgTabSwitchFromHook = RegisterWindowMessage(CONEMUMSG_SWITCHCON); //mb_InWinTabSwitch = FALSE;
	mn_MsgWinKeyFromHook = RegisterWindowMessage(CONEMUMSG_HOOKEDKEY);
	//mn_MsgConsoleHookedKey = RegisterWindowMessage(CONEMUMSG_CONSOLEHOOKEDKEY);
	mn_MsgSheelHook = RegisterWindowMessage(L"SHELLHOOK");
	mn_ShellExecuteEx = ++nAppMsg;
	mn_PostConsoleResize = ++nAppMsg;
	mn_ConsoleLangChanged = ++nAppMsg;
	mn_MsgPostOnBufferHeight = ++nAppMsg;
	//mn_MsgSetForeground = RegisterWindowMessage(CONEMUMSG_SETFOREGROUND);
	mn_MsgFlashWindow = RegisterWindowMessage(CONEMUMSG_FLASHWINDOW);
	mn_MsgPostAltF9 = ++nAppMsg;
	//mn_MsgPostSetBackground = ++nAppMsg;
	mn_MsgInitInactiveDC = ++nAppMsg;
	//mn_MsgActivateCon = RegisterWindowMessage(CONEMUMSG_ACTIVATECON);
	mn_MsgUpdateProcDisplay = ++nAppMsg;
	mn_MsgAutoSizeFont = ++nAppMsg;
	mn_MsgDisplayRConError = ++nAppMsg;
	mn_MsgMacroFontSetName = ++nAppMsg;
	mn_MsgCreateViewWindow = ++nAppMsg;
	mn_MsgPostTaskbarActivate = ++nAppMsg; mb_PostTaskbarActivate = FALSE;
	mn_MsgInitVConGhost = ++nAppMsg;
	mn_MsgCreateCon = ++nAppMsg;
	mn_MsgRequestUpdate = ++nAppMsg;
	mn_MsgTaskBarCreated = RegisterWindowMessage(L"TaskbarCreated");
	mn_MsgPanelViewMapCoord = RegisterWindowMessage(CONEMUMSG_PNLVIEWMAPCOORD);
	//// В Win7x64 WM_INPUTLANGCHANGEREQUEST не приходит (по крайней мере при переключении мышкой)
	//wmInputLangChange = WM_INPUTLANGCHANGE;

	InitFrameHolder();
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

LPWSTR CConEmuMain::ConEmuXml()
{
	if (ms_ConEmuXml[0])
	{
		if (FileExists(ms_ConEmuXml))
			return ms_ConEmuXml;
	}

	// Ищем файл портабельных настроек. Сначала пробуем в BaseDir
	wcscpy_c(ms_ConEmuXml, ms_ConEmuBaseDir); wcscat_c(ms_ConEmuXml, L"\\ConEmu.xml");

	if (!FileExists(ms_ConEmuXml))
	{
		if (lstrcmp(ms_ConEmuBaseDir, ms_ConEmuExeDir))
		{
			wcscpy_c(ms_ConEmuXml, ms_ConEmuExeDir); lstrcat(ms_ConEmuXml, L"\\ConEmu.xml");
		}
	}

	return ms_ConEmuXml;
}

LPCWSTR CConEmuMain::ConEmuCExeFull(LPCWSTR asCmdLine/*=NULL*/)
{
	// Если OS - 32битная или в папке ConEmu был найден только один из "серверов"
	if (!IsWindows64() || !lstrcmp(ms_ConEmuC32Full, ms_ConEmuC64Full))
	{
		// Сразу вернуть
		return ms_ConEmuC32Full;
	}

	LPCWSTR pszServer = ms_ConEmuC32Full;
	bool lbCmd = false, lbFound = false;
	int Bits = IsWindows64() ? 64 : 32;

	if (!asCmdLine || !*asCmdLine)
	{
		// Если строка запуска не указана - считаем, что запускается ComSpec
		lbCmd = true;
	}
	else
	{
		// Проверить битность asCmdLine во избежание лишних запусков серверов для Inject
		// и корректной битности запускаемого процессора по настройке
		wchar_t szTemp[MAX_PATH+1], szExpand[MAX_PATH+1];
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
			DWORD nLen = ExpandEnvironmentStrings(asCmdLine, szExpand, countof(szExpand));
			if (nLen && (nLen < countof(szExpand)))
				asCmdLine = szExpand;
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
					if (!lbFound && !wcschr(asCmdLine, L'\\'))
					{
						// попытаемся найти
						wchar_t *pszFilePart;
						DWORD nLen = SearchPath(NULL, asCmdLine, pszExt ? NULL : L".exe", countof(szFind), szFind, &pszFilePart);
						if (!nLen)
						{
							wchar_t szRoot[MAX_PATH+1];
							wcscpy_c(szRoot, ms_ConEmuExeDir);
							wchar_t* pszSlash = wcsrchr(szRoot, L'\\');
							if (pszSlash)
								*pszSlash = 0;
							nLen = SearchPath(szRoot, asCmdLine, pszExt ? NULL : L".exe", countof(szFind), szFind, &pszFilePart);
						}
						if (nLen && (nLen < countof(szFind)))
						{
							lbFound = GetImageSubsystem(szFind, ImageSubsystem, ImageBits, FileAttrs);
						}
					}

					if (lbFound)
						Bits = ImageBits;
				}
			}
		}
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

void CConEmuMain::InitCommCtrls()
{
	if (mb_CommCtrlsInitialized)
		return;

	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC   = ICC_COOL_CLASSES|ICC_BAR_CLASSES|ICC_TAB_CLASSES|ICC_PROGRESS_CLASS;
	InitCommonControlsEx(&icex);

	mb_CommCtrlsInitialized = true;
}

BOOL CConEmuMain::Init()
{
	_ASSERTE(mp_TabBar == NULL);

	// Только по настройке, а то дочерние процессы с тем же Affinity запускаются...
	// На тормоза - не влияет. Но вроде бы на многопроцессорных из-за глюков в железе могут быть ошибки подсчета производительности, если этого не сделать
	if (gpSet->nAffinity)
		SetProcessAffinityMask(GetCurrentProcess(), gpSet->nAffinity);

	InitCommCtrls();

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
	return TRUE;
}

void CConEmuMain::OnUseGlass(bool abEnableGlass)
{
	//CheckMenuItem(GetSystemMenu(ghWnd, false), ID_USEGLASS, MF_BYCOMMAND | (abEnableGlass ? MF_CHECKED : MF_UNCHECKED));
}
void CConEmuMain::OnUseTheming(bool abEnableTheming)
{
	//CheckMenuItem(GetSystemMenu(ghWnd, false), ID_USETHEME, MF_BYCOMMAND | (abEnableTheming ? MF_CHECKED : MF_UNCHECKED));
}
void CConEmuMain::OnUseDwm(bool abEnableDwm)
{
	//CheckMenuItem(GetSystemMenu(ghWnd, false), ID_ISDWM, MF_BYCOMMAND | (abEnableDwm ? MF_CHECKED : MF_UNCHECKED));
}

// Вызывается при старте программы, для вычисления mrc_Ideal - размера окна по умолчанию
RECT CConEmuMain::GetDefaultRect()
{
	int nWidth, nHeight;
	RECT rcWnd;
	MBoxAssert(gpSetCls->FontWidth() && gpSetCls->FontHeight());
	COORD conSize; conSize.X=gpSet->wndWidth; conSize.Y=gpSet->wndHeight;
	//int nShiftX = GetSystemMetrics(SM_CXSIZEFRAME)*2;
	//int nShiftY = GetSystemMetrics(SM_CYSIZEFRAME)*2 + (gpSet->isHideCaptionAlways ? 0 : GetSystemMetrics(SM_CYCAPTION));
	RECT rcFrameMargin = CalcMargins(CEM_FRAME);
	int nShiftX = rcFrameMargin.left + rcFrameMargin.right;
	int nShiftY = rcFrameMargin.top + rcFrameMargin.bottom;
	// Если табы показываются всегда - сразу добавим их размер, чтобы размер консоли был заказанным
	nWidth  = conSize.X * gpSetCls->FontWidth() + nShiftX
	          + ((gpSet->isTabs == 1) ? (gpSet->rcTabMargins.left+gpSet->rcTabMargins.right) : 0);
	nHeight = conSize.Y * gpSetCls->FontHeight() + nShiftY
	          + ((gpSet->isTabs == 1) ? (gpSet->rcTabMargins.top+gpSet->rcTabMargins.bottom) : 0);
	rcWnd = MakeRect(gpSet->wndX, gpSet->wndY, gpSet->wndX+nWidth, gpSet->wndY+nHeight);

	if (gpSet->wndCascade)
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

		int nX = GetSystemMetrics(SM_CXSIZEFRAME), nY = GetSystemMetrics(SM_CYSIZEFRAME);
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
		gpSet->wndX = rcWnd.left;
		gpSet->wndY = rcWnd.top;
	}

	return rcWnd;
}

RECT CConEmuMain::GetGuiClientRect()
{
	RECT rcClient = {};
	BOOL lbRc = ::GetClientRect(ghWnd, &rcClient); UNREFERENCED_PARAMETER(lbRc);
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

HMENU CConEmuMain::GetSystemMenu(BOOL abInitial /*= FALSE*/)
{
	HMENU hwndMain = ::GetSystemMenu(ghWnd, FALSE);
	MENUITEMINFO mi = {sizeof(mi), MIIM_DATA};
	wchar_t szText[255];
	mi.dwTypeData = szText;
	mi.cch = countof(szText);

	// В результате работы некоторых недобросовествных программ может сбиваться настроенное системное меню
	if (!GetMenuItemInfo(hwndMain, ID_NEWCONSOLE, FALSE, &mi))
	{
		InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_TOMONITOR, _T("Bring &here"));
		InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_TOTRAY, TRAY_ITEM_HIDE_NAME/* L"Hide to &TSA" */);
		InsertMenu(hwndMain, 0, MF_BYPOSITION, MF_SEPARATOR, 0);
		
		//InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_ABOUT, _T("&About"));
		if (mh_HelpPopup) DestroyMenu(mh_HelpPopup);
		mh_HelpPopup = CreateHelpMenuPopup();
		InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_POPUP | MF_ENABLED, (UINT_PTR)mh_HelpPopup, _T("Hel&p"));
		//if (ms_ConEmuChm[0])  //Показывать пункт только если есть conemu.chm
		//	InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_HELP, _T("&Help"));

		// --------------------
		InsertMenu(hwndMain, 0, MF_BYPOSITION, MF_SEPARATOR, 0);
		
		if (mh_DebugPopup) DestroyMenu(mh_DebugPopup);
		mh_DebugPopup = CreateDebugMenuPopup();
		InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_POPUP | MF_ENABLED, (UINT_PTR)mh_DebugPopup, _T("&Debug"));
		
		if (mh_EditPopup) DestroyMenu(mh_EditPopup);
		mh_EditPopup = CreateEditMenuPopup(NULL);
		InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_POPUP | MF_ENABLED, (UINT_PTR)mh_EditPopup, _T("Ed&it"));
		
		// --------------------
		InsertMenu(hwndMain, 0, MF_BYPOSITION, MF_SEPARATOR, 0);
		
		if (mh_VConListPopup) DestroyMenu(mh_VConListPopup);
		mh_VConListPopup = CreateVConListPopupMenu(mh_VConListPopup, TRUE/*abFirstTabOnly*/);
		InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_POPUP | MF_ENABLED, (UINT_PTR)mh_VConListPopup, _T("Console &list"));
		
		if (mh_ActiveVConPopup) DestroyMenu(mh_ActiveVConPopup);
		if (mh_TerminateVConPopup) { DestroyMenu(mh_TerminateVConPopup); mh_TerminateVConPopup = NULL; }
		mh_ActiveVConPopup = CreateVConPopupMenu(NULL, NULL, FALSE, mh_TerminateVConPopup);
		InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_POPUP | MF_ENABLED, (UINT_PTR)mh_ActiveVConPopup, _T("Acti&ve console"));
		
		// --------------------
		InsertMenu(hwndMain, 0, MF_BYPOSITION, MF_SEPARATOR, 0);
		InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED | (gpSet->isAlwaysOnTop ? MF_CHECKED : 0),
			ID_ALWAYSONTOP, _T("Al&ways on top"));
		InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED | (gpSetCls->AutoScroll ? MF_CHECKED : 0),
			ID_AUTOSCROLL, _T("Auto scro&ll"));
		InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_SETTINGS, _T("S&ettings..."));
		InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, IDM_ATTACHTO, _T("Attach t&o..."));
		InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_NEWCONSOLE, _T("&New console..."));
	}

	return hwndMain;
}

// Эта функция расчитывает необходимые стили по текущим настройкам, а не возвращает GWL_STYLE
DWORD_PTR CConEmuMain::GetWindowStyle()
{
	DWORD_PTR style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	//if (gpSet->isShowOnTaskBar) // ghWndApp
	//	style |= WS_POPUPWINDOW | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
	//else
	style |= WS_OVERLAPPEDWINDOW;
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
	return style;
}

// Эта функция расчитывает необходимые стили по текущим настройкам, а не возвращает GWL_STYLE_EX
DWORD_PTR CConEmuMain::GetWindowStyleEx()
{
	DWORD_PTR styleEx = WS_EX_APPWINDOW;

	if (gpSet->nTransparent < 255 /*&& !gpSet->isDesktopMode*/)
		styleEx |= WS_EX_LAYERED;

	if (gpSet->isAlwaysOnTop)
		styleEx |= WS_EX_TOPMOST;

	if (gpSet->isTabsOnTaskBar() && !IsWindows7)
	{
		styleEx &= ~WS_EX_APPWINDOW;
		//styleEx |= WS_EX_TOOLWINDOW;
	}

#ifndef CHILD_DESK_MODE

	if (gpSet->isDesktopMode)
		styleEx |= WS_EX_TOOLWINDOW;

#endif
	return styleEx;
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

	if (!RegisterClassEx(&wc))
		return -1;

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

	// Расчет размеров окна в Normal режиме
	if (gpSet->wndWidth && gpSet->wndHeight)
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
		//	+ ((gpSet->isTabs == 1) ? (gpSet->rcTabMargins.left+gpSet->rcTabMargins.right) : 0);
		//nHeight = conSize.Y * gpSetCls->FontHeight() + nShiftY
		//	+ ((gpSet->isTabs == 1) ? (gpSet->rcTabMargins.top+gpSet->rcTabMargins.bottom) : 0);
		//mrc_Ideal = MakeRect(gpSet->wndX, gpSet->wndY, gpSet->wndX+nWidth, gpSet->wndY+nHeight);
		mrc_Ideal = GetDefaultRect();
		nWidth = mrc_Ideal.right - mrc_Ideal.left;
		nHeight = mrc_Ideal.bottom - mrc_Ideal.top;
	}

	//if (gpConEmu->WindowMode == rMaximized) style |= WS_MAXIMIZE;
	//style |= WS_VISIBLE;
	// cRect.right - cRect.left - 4, cRect.bottom - cRect.top - 4; -- все равно это было не правильно
	WARNING("На ноуте вылезает за пределы рабочей области");
	ghWnd = CreateWindowEx(styleEx, gsClassNameParent, gpSet->GetCmd(), style,
	                       gpSet->wndX, gpSet->wndY, nWidth, nHeight, ghWndApp, NULL, (HINSTANCE)g_hInstance, NULL);

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
	//if (gpConEmu->WindowMode == rFullScreen || gpConEmu->WindowMode == rMaximized)
	//	gpConEmu->SetWindowMode(gpConEmu->WindowMode);
	UpdateGuiInfoMapping();
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
	_ASSERTE(m_GuiInfo.ComSpec.csType==gpSet->ComSpec.csType && m_GuiInfo.ComSpec.csBits==gpSet->ComSpec.csBits);
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

void CConEmuMain::UpdateGuiInfoMapping()
{
	m_GuiInfo.nProtocolVersion = CESERVER_REQ_VER;
	m_GuiInfo.hGuiWnd = ghWnd;
	m_GuiInfo.nGuiPID = GetCurrentProcessId();
	
	m_GuiInfo.nLoggingType = (ghOpWnd && gpSetCls->mh_Tabs[gpSetCls->thi_Debug]) ? gpSetCls->m_ActivityLoggingType : glt_None;
	m_GuiInfo.bUseInjects = (gpSet->isUseInjects ? 1 : 0);
	m_GuiInfo.bUseTrueColor = gpSet->isTrueColorer;
	m_GuiInfo.bProcessAnsi = (gpSet->isProcessAnsi ? 1 : 0);

	mb_DosBoxExists = CheckDosBoxExists();
	m_GuiInfo.bDosBox = mb_DosBoxExists;
	
	m_GuiInfo.isHookRegistry = (mb_PortableRegExist ? (gpSet->isPortableReg ? 3 : 1) : 0);
	wcscpy_c(m_GuiInfo.sHiveFileName, ms_PortableRegHive);
	m_GuiInfo.hMountRoot = mh_PortableMountRoot;
	wcscpy_c(m_GuiInfo.sMountKey, ms_PortableMountKey);
	
	wcscpy_c(m_GuiInfo.sConEmuExe, ms_ConEmuExe);
	wcscpy_c(m_GuiInfo.sConEmuDir, ms_ConEmuExeDir);
	wcscpy_c(m_GuiInfo.sConEmuBaseDir, ms_ConEmuBaseDir);
	_wcscpyn_c(m_GuiInfo.sConEmuArgs, countof(m_GuiInfo.sConEmuArgs), mpsz_ConEmuArgs ? mpsz_ConEmuArgs : L"", countof(m_GuiInfo.sConEmuArgs));

	// *********************
	// *** ComSpec begin ***
	// *********************
	{
		// Сначала - обработать что что поменяли (найти tcc и т.п.)
		SetEnvVarExpanded(L"ComSpec", ms_ComSpecInitial); // т.к. функции могут ориентироваться на окружение
		FindComspec(&gpSet->ComSpec);
		UpdateComspec(&gpSet->ComSpec); // установит переменную окружения, если просили

		// Теперь перенести в мэппинг, для информирования других процессов
		ComSpecType csType = gpSet->ComSpec.csType;
		ComSpecBits csBits = gpSet->ComSpec.csBits;
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
		m_GuiInfo.ComSpec.isUpdateEnv = gpSet->ComSpec.isUpdateEnv;
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

HRGN CConEmuMain::CreateWindowRgn(bool abTestOnly/*=false*/)
{
	HRGN hRgn = NULL, hExclusion = NULL;

	TODO("DoubleView: Если видимы несколько консолей - нужно совместить регионы, или вообще отрубить, для простоты");

	//if (isIconic())
	//	return NULL;
	if (mp_VActive/* && abTestOnly*/)
	{
		hExclusion = mp_VActive->GetExclusionRgn(true);

		if (abTestOnly && hExclusion)
		{
			_ASSERTE(hExclusion == (HRGN)1);
			return (HRGN)1;
		}
	}

	WARNING("Установка любого НЕ NULL региона сбивает темы при отрисовке кнопок в заголовке");

	if ((mb_isFullScreen || (isZoomed() && (gpSet->isHideCaption || gpSet->isHideCaptionAlways())))
	        && !mb_InRestore)
	{
		if (abTestOnly)
			return (HRGN)1;

		ConEmuRect tFrom = mb_isFullScreen ? CER_FULLSCREEN : CER_MAXIMIZED;
		RECT rcScreen; // = CalcRect(tFrom, MakeRect(0,0), tFrom);
		RECT rcFrame = CalcMargins(CEM_FRAME);
		/*
		ConEmuRect tFrom = mb_isFullScreen ? CER_FULLSCREEN : CER_MAXIMIZED;
		RECT rcScreen = CalcRect(tFrom, MakeRect(0,0), tFrom);
		hRgn = CreateWindowRgn(abTestOnly, false, rcFrame.left, rcFrame.top, rcScreen.right-rcScreen.left, rcScreen.bottom-rcScreen.top);
		*/
		HMONITOR hMon = MonitorFromWindow(ghWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi = {sizeof(mi)};
		if (GetMonitorInfo(hMon, &mi))
			rcScreen = mb_isFullScreen ? mi.rcMonitor : mi.rcWork;
		else
			rcScreen = CalcRect(tFrom, MakeRect(0,0), tFrom);
		// rcFrame, т.к. регион ставится относительно верхнего левого угла ОКНА
		hRgn = CreateWindowRgn(abTestOnly, false, rcFrame.left, rcFrame.top, rcScreen.right-rcScreen.left, rcScreen.bottom-rcScreen.top);
	}
	else if (isZoomed() && !mb_InRestore)
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
		if (gpSet->isHideCaptionAlways())
		{
			if (!isMouseOverFrame())
			{
				// Рамка невидима (мышка не над рамкой или заголовком)
				RECT rcClient = GetGuiClientRect();
				RECT rcFrame = CalcMargins(CEM_FRAME);
				_ASSERTE(!rcClient.left && !rcClient.top);
				hRgn = CreateWindowRgn(abTestOnly, gpSetCls->CheckTheming() && mp_TabBar->IsTabsShown(),
				                       rcFrame.left-gpSet->nHideCaptionAlwaysFrame,
				                       rcFrame.top-gpSet->nHideCaptionAlwaysFrame,
				                       rcClient.right+2*gpSet->nHideCaptionAlwaysFrame,
				                       rcClient.bottom+2*gpSet->nHideCaptionAlwaysFrame);
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

	if (mp_VActive)
	{
		hExclusion = mp_VActive->GetExclusionRgn(abTestOnly);

		if (abTestOnly && hExclusion)
		{
			_ASSERTE(hExclusion == (HRGN)1);
			return (HRGN)1;
		}
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
			RECT rcFrame = CalcMargins(CEM_FRAME);
#ifdef _DEBUG
			// CEM_TAB не учитывает центрирование клиентской части в развернутых режимах
			RECT rcTab = CalcMargins(CEM_TAB);
#endif
			POINT ptClient = {0,0};
			MapWindowPoints(mp_VActive->GetView(), ghWnd, &ptClient, 1);
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
	for (size_t i = 0; i < countof(mp_VCon); i++)
	{
		if (mp_VCon[i])
		{
			mp_VCon[i]->RCon()->StopSignal();
		}
	}

	if (gbInDisplayLastError)
		return; // Чтобы не схлопывались окна с сообщениями об ошибках

	#ifdef _DEBUG
	if (gbInMyAssertTrap)
		return;
	#endif

	if (ghWnd)
	{
		//HWND hWnd = ghWnd;
		//ghWnd = NULL;
		//DestroyWindow(hWnd); -- может быть вызывано из другой нити
		PostMessage(ghWnd, mn_MsgMyDestroy, GetCurrentThreadId(), 0);
	}
}

CConEmuMain::~CConEmuMain()
{
	_ASSERTE(ghWnd==NULL || !IsWindow(ghWnd));
	//ghWnd = NULL;

	if (mp_AttachDlg)
	{
		delete mp_AttachDlg;
		mp_AttachDlg = NULL;
	}
	if (mp_RecreateDlg)
	{
		delete mp_RecreateDlg;
		mp_RecreateDlg = NULL;
	}

	for (size_t i = 0; i < countof(mp_VCon); i++)
	{
		if (mp_VCon[i])
		{
			CVirtualConsole* p = mp_VCon[i];
			mp_VCon[i] = NULL;
			p->Release();
		}
	}

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

	if (mp_DragDrop)
	{
		delete mp_DragDrop;
		mp_DragDrop = NULL;
	}

	//if (ProgressBars) {
	//    delete ProgressBars;
	//    ProgressBars = NULL;
	//}

	if (mp_TabBar)
	{
		delete mp_TabBar;
		mp_TabBar = NULL;
	}

	if (mp_Tip)
	{
		delete mp_Tip;
		mp_Tip = NULL;
	}

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

	if (mh_DebugPopup)
	{
		DestroyMenu(mh_DebugPopup);
		mh_DebugPopup = NULL;
	}
	
	if (mh_EditPopup)
	{
		DestroyMenu(mh_EditPopup);
		mh_EditPopup = NULL;
	}
	
	if (mh_TerminateVConPopup)
	{
		DestroyMenu(mh_TerminateVConPopup);
		mh_TerminateVConPopup = NULL;
	}
	
	if (mh_ActiveVConPopup)
	{
		DestroyMenu(mh_ActiveVConPopup);
		mh_ActiveVConPopup = NULL;
	}
	
	//if (mh_RecreatePasswFont)
	//{
	//	DeleteObject(mh_RecreatePasswFont);
	//	mh_RecreatePasswFont = NULL;
	//}

	m_GuiServer.Stop(true);
	
	// По идее, уже должен был быть вызван в OnDestroy
	FinalizePortableReg();

	LoadImageFinalize();

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
void CConEmuMain::AddMargins(RECT& rc, RECT& rcAddShift, BOOL abExpand/*=FALSE*/)
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
	CVirtualConsole *pVCon = ActiveCon();
	CRealConsole *pRCon = pVCon->RCon();
	if (!pRCon) return;


	HWND hGuiClient = pRCon->GuiWnd();
	if (hGuiClient)
	{
		int nNewShow = (GetWindowLongPtr(hGuiClient, GWL_STYLE) & WS_VISIBLE) ? SW_HIDE : SW_SHOW;
		pRCon->ShowOtherWindow(hGuiClient, nNewShow);
		pVCon->Invalidate();
		return;
	}


	// Win7 BUGBUG: Issue 192: падение Conhost при turn bufferheight ON
	// http://code.google.com/p/conemu-maximus5/issues/detail?id=192
	if (gOSVer.dwMajorVersion == 6 && gOSVer.dwMinorVersion == 1)
		return;

	CVConGuard guard(pVCon);

	BOOL lbBufferHeight = pRCon->isBufferHeight();

	BOOL b = gbDontEnable; gbDontEnable = TRUE;
	int nBtn = MessageBox(ghWnd, lbBufferHeight ?
						  L"Do You want to turn bufferheight OFF?" :
						  L"Do You want to turn bufferheight ON?",
						  GetDefaultTitle(), MB_ICONQUESTION|MB_OKCANCEL);
	gbDontEnable = b;

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
	OnBufferHeight();
}

void CConEmuMain::AskChangeAlternative()
{
	CVirtualConsole *pVCon = ActiveCon();
	CRealConsole *pRCon = pVCon->RCon();
	if (!pRCon || pRCon->GuiWnd())
	{
		return;
	}

	CVConGuard guard(pVCon);

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
	OnBufferHeight();
}

/*!!!static!!*/
// Функция расчитывает смещения (относительные)
// mg содержит битмаск, например (CEM_FRAME|CEM_TAB|CEM_CLIENT)
RECT CConEmuMain::CalcMargins(DWORD/*enum ConEmuMargins*/ mg, CVirtualConsole* apVCon)
{
	_ASSERTE(this!=NULL);
	if (!apVCon)
		apVCon = gpConEmu->mp_VActive;

	RECT rc = {};

	// Разница между размером всего окна и клиентской области окна (рамка + заголовок)
	if (mg & ((DWORD)CEM_FRAME))
	{
		// т.к. это первая обработка - можно ставить rc простым приравниванием
		_ASSERTE(rc.left==0 && rc.top==0 && rc.right==0 && rc.bottom==0);
		DWORD dwStyle = GetWindowStyle();
		DWORD dwStyleEx = GetWindowStyleEx();
		//static DWORD dwLastStyle, dwLastStyleEx;
		//static RECT rcLastRect;
		//if (dwLastStyle == dwStyle && dwLastStyleEx == dwStyleEx)
		//{
		//	rc = rcLastRect; // чтобы не дергать лишние расчеты
		//}
		//else
		//{
		RECT rcTest = MakeRect(100,100);

		if (AdjustWindowRectEx(&rcTest, dwStyle, FALSE, dwStyleEx))
		{
			rc.left = -rcTest.left;
			rc.top = -rcTest.top;
			rc.right = rcTest.right - 100;
			rc.bottom = rcTest.bottom - 100;
			//dwLastStyle = dwStyle; dwLastStyleEx = dwStyleEx;
			//rcLastRect = rc;
		}
		else
		{
			_ASSERTE(FALSE);
			rc.left = rc.right = GetSystemMetrics(SM_CXSIZEFRAME);
			rc.bottom = GetSystemMetrics(SM_CYSIZEFRAME);
			rc.top = rc.bottom + GetSystemMetrics(SM_CYCAPTION);
			//	+ (gpSet->isHideCaptionAlways ? 0 : GetSystemMetrics(SM_CYCAPTION));
		}

		//}
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
				AddMargins(rc, rcTab, FALSE);
			}// else { -- раз таба нет - значит дополнительные отступы не нужны

			//    rc = MakeRect(0,0); // раз таба нет - значит дополнительные отступы не нужны
			//}
		}
		else
		{
			// Иначе нужно смотреть по настройкам
			if (gpSet->isTabs == 1)
			{
				RECT rcTab = gpSet->rcTabMargins; // умолчательные отступы таба

				if (!gpSet->isTabFrame)
				{
					// От таба остается только заголовок (закладки)
					//rc.left=0; rc.right=0; rc.bottom=0;
					rc.top += rcTab.top;
				}
				else
				{
					AddMargins(rc, rcTab, FALSE);
				}
			}// else { -- раз таба нет - значит дополнительные отступы не нужны

			//    rc = MakeRect(0,0); // раз таба нет - значит дополнительные отступы не нужны
			//}
		}
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

	if ((mg & ((DWORD)CEM_SCROLL)) && (gpSet->isAlwaysShowScrollbar == 1))
	{
		rc.right += GetSystemMetrics(SM_CXVSCROLL);
	}

	return rc;
}

RECT CConEmuMain::CalcRect(enum ConEmuRect tWhat, CVirtualConsole* pVCon/*=NULL*/)
{
	_ASSERTE(ghWnd!=NULL);
	RECT rcMain = {};
	if (isIconic())
	{
		WINDOWPLACEMENT wpl = {sizeof(wpl)};
		GetWindowPlacement(ghWnd, &wpl);
		TODO("Если окно было свернуто из Maximized состояние? Нужно брать не rcNormalPosition а Maximized?");
		rcMain = wpl.rcNormalPosition;
	}
	else
	{
		GetWindowRect(ghWnd, &rcMain);
	}

	return CalcRect(tWhat, rcMain, CER_MAIN, pVCon);
}

// Для приблизительного расчета размеров - нужен только (размер главного окна)|(размер консоли)
// Для точного расчета размеров - требуется (размер главного окна) и (размер окна отрисовки) для корректировки
// на x64 возникают какие-то глюки с ",RECT rFrom,". Отладчик показывает мусор в rFrom,
// но тем не менее, после "RECT rc = rFrom;", rc получает правильные значения >:|
RECT CConEmuMain::CalcRect(enum ConEmuRect tWhat, const RECT &rFrom, enum ConEmuRect tFrom, CVirtualConsole* pVCon, RECT* prDC/*=NULL*/, enum ConEmuMargins tTabAction/*=CEM_TAB*/)
{
	_ASSERTE(this!=NULL);
	RECT rc = rFrom; // инициализация, если уж не получится...
	RECT rcShift = MakeRect(0,0);
	enum ConEmuRect tFromNow = tFrom;

	if (!pVCon)
		pVCon = gpConEmu->mp_VActive;

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

	switch(tFrom)
	{
		case CER_MAIN: // switch (tFrom)
			// Нужно отнять отступы рамки и заголовка!
		{
			rcShift = CalcMargins(CEM_FRAME);
			rc.right = (rFrom.right-rFrom.left) - (rcShift.left+rcShift.right);
			rc.bottom = (rFrom.bottom-rFrom.top) - (rcShift.top+rcShift.bottom);
			rc.left = 0;
			rc.top = 0; // Получили клиентскую область
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

			switch(tWhat)
			{
				case CER_MAIN:
				{
					//rcShift = CalcMargins(CEM_BACK);
					//AddMargins(rc, rcShift, TRUE/*abExpand*/);
					rcShift = CalcMargins(tTabAction|CEM_FRAME|CEM_SCROLL);
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
					WARNING("CER_WORKSPACE - не сделано вообще");
					_ASSERTE(tWhat!=CER_WORKSPACE);
				} break;
				case CER_BACK:
				{
					//rcShift = CalcMargins(CEM_BACK);
					//AddMargins(rc, rcShift, TRUE/*abExpand*/);
					rcShift = CalcMargins(tTabAction);
					//AddMargins(rc, rcShift, TRUE/*abExpand*/);
					rc.top += rcShift.top; rc.bottom += rcShift.top;
					_ASSERTE(rcShift.left == 0 && rcShift.right == 0 && rcShift.bottom == 0);
					WARNING("Неправильно получается при DoubleView");
					tFromNow = CER_MAINCLIENT;
				} break;
				default:
					break;
			}
		}
		return rc;
		case CER_CONSOLE: // switch (tFrom)
		{
			// Размер консоли в символах!
			//MBoxAssert(!(rFrom.left || rFrom.top));
			_ASSERTE(tWhat!=CER_CONSOLE);
			//if (gpSetCls->FontWidth()==0) {
			//    MBoxAssert(pVCon!=NULL);
			//    pVCon->InitDC(false, true); // инициализировать ширину шрифта по умолчанию
			//}
			// ЭТО размер окна отрисовки DC
			rc = MakeRect((rFrom.right-rFrom.left) * gpSetCls->FontWidth(),
			              (rFrom.bottom-rFrom.top) * gpSetCls->FontHeight());

			if (tWhat != CER_DC)
				rc = CalcRect(tWhat, rc, CER_DC);

			// -- tFromNow = CER_BACK; -- менять не требуется, т.к. мы сразу на выход
		}
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

	// Теперь rc должен соответствовать CER_MAINCLIENT
	RECT rcAddShift = MakeRect(0,0);

	if (prDC)
	{
		// Если передали реальный размер окна отрисовки - нужно посчитать дополнительные сдвиги
		RECT rcCalcDC = CalcRect(CER_DC, rFrom, CER_MAINCLIENT, NULL /*prDC*/);
		// расчетный НЕ ДОЛЖЕН быть меньше переданного
#ifdef MSGLOGGER
		_ASSERTE((rcCalcDC.right - rcCalcDC.left)>=(prDC->right - prDC->left));
		_ASSERTE((rcCalcDC.bottom - rcCalcDC.top)>=(prDC->bottom - prDC->top));
#endif

		// считаем доп.сдвиги. ТОЧНО
		if ((rcCalcDC.right - rcCalcDC.left)!=(prDC->right - prDC->left))
		{
			rcAddShift.left = (rcCalcDC.right - rcCalcDC.left - (prDC->right - prDC->left))/2;
			rcAddShift.right = rcCalcDC.right - rcCalcDC.left - rcAddShift.left;
		}

		if ((rcCalcDC.bottom - rcCalcDC.top)!=(prDC->bottom - prDC->top))
		{
			rcAddShift.top = (rcCalcDC.bottom - rcCalcDC.top - (prDC->bottom - prDC->top))/2;
			rcAddShift.bottom = rcCalcDC.bottom - rcCalcDC.top - rcAddShift.top;
		}
	}

	// если мы дошли сюда - значит tFrom==CER_MAINCLIENT

	switch(tWhat)
	{
		case CER_TAB: // switch (tWhat)
		{
			// Отступы ДО таба могут появиться только от корректировки
		} break;
		case CER_WORKSPACE: // switch (tWhat)
		{
			rcShift = CalcMargins(tTabAction|CEM_SCROLL);
			AddMargins(rc, rcShift);
		} break;
		case CER_BACK: // switch (tWhat)
		{
			TODO("DoubleView");
			rcShift = CalcMargins(tTabAction|CEM_SCROLL);
			AddMargins(rc, rcShift);
		} break;
		case CER_SCROLL: // switch (tWhat)
		{
			rcShift = CalcMargins(tTabAction);
			AddMargins(rc, rcShift);
			rc.left = rc.right - GetSystemMetrics(SM_CXVSCROLL);
			return rc; // Иначе внизу еще будет коррекция по DC (rcAddShift)
		} break;
		case CER_DC: // switch (tWhat)
		case CER_CONSOLE: // switch (tWhat)
		case CER_CONSOLE_NTVDMOFF: // switch (tWhat)
		{
			if (tFromNow == CER_MAINCLIENT)
			{
				// Учесть высоту закладок (табов)
				rcShift = CalcMargins(tTabAction|CEM_SCROLL);
				AddMargins(rc, rcShift);
			}
			else if (tFromNow == CER_BACK || tFromNow == CER_WORKSPACE)
			{
				TODO("Расчет для DoubleView");
			}
			else
			{
				// Другие значения - не допускаются
				_ASSERTE(tFromNow == CER_MAINCLIENT);
			}

			//// Для корректного деления на размер знакоместа...
			//         if (gpSetCls->FontWidth()==0 || gpSetCls->FontHeight()==0)
			//             pVCon->InitDC(false, true); // инициализировать ширину шрифта по умолчанию
			//rc.right ++;
			//int nShift = (gpSetCls->FontWidth() - 1) / 2; if (nShift < 1) nShift = 1;
			//rc.right += nShift;
			// Если есть вкладки
			//if (rcShift.top || rcShift.bottom || )
			//nShift = (gpSetCls->FontWidth() - 1) / 2; if (nShift < 1) nShift = 1;

			if (tWhat != CER_CONSOLE_NTVDMOFF && pVCon && pVCon->RCon() && pVCon->RCon()->isNtvdm())
			{
				// NTVDM устанавливает ВЫСОТУ экранного буфера... в 25/28/43/50 строк
				// путем округления текущей высоты (то есть если до запуска 16bit
				// было 27 строк, то скорее всего будет установлена высота в 28 строк)
				RECT rc1 = MakeRect(pVCon->TextWidth*gpSetCls->FontWidth(), pVCon->TextHeight*gpSetCls->FontHeight());

				//gpSet->ntvdmHeight /* pVCon->TextHeight */ * gpSetCls->FontHeight());
				if (rc1.bottom > (rc.bottom - rc.top))
					rc1.bottom = (rc.bottom - rc.top); // Если размер вылез за текущий - обрежем снизу :(

				int nS = rc.right - rc.left - rc1.right;

				if (nS>=0)
				{
					rcShift.left = nS / 2;
					rcShift.right = nS - rcShift.left;
				}
				else
				{
					rcShift.left = 0;
					rcShift.right = -nS;
				}

				nS = rc.bottom - rc.top - rc1.bottom;

				if (nS>=0)
				{
					rcShift.top = nS / 2;
					rcShift.bottom = nS - rcShift.top;
				}
				else
				{
					rcShift.top = 0;
					rcShift.bottom = -nS;
				}

				AddMargins(rc, rcShift);
			}

			// Если нужен размер консоли в символах сразу делим и выходим
			if (tWhat == CER_CONSOLE || tWhat == CER_CONSOLE_NTVDMOFF)
			{
				//2009-07-09 - ClientToConsole использовать нельзя, т.к. после его
				//  приближений высота может получиться больше Ideal, а ширина - меньше
				int nW = (rc.right - rc.left + 1) / gpSetCls->FontWidth();
				int nH = (rc.bottom - rc.top) / gpSetCls->FontHeight();
				rc.left = 0; rc.top = 0; rc.right = nW; rc.bottom = nH;

				//2010-01-19
				if (gpSet->isFontAutoSize)
				{
					if (gpSet->wndWidth && rc.right > (LONG)gpSet->wndWidth)
						rc.right = gpSet->wndWidth;

					if (gpSet->wndHeight && rc.bottom > (LONG)gpSet->wndHeight)
						rc.bottom = gpSet->wndHeight;
				}

#ifdef _DEBUG
				_ASSERTE(rc.bottom>=5);
#endif

				// Возможно, что в RealConsole выставлен большой шрифт, который помешает установке этого размера
				if (pVCon)
				{
					CRealConsole* pRCon = pVCon->RCon();

					if (pRCon)
					{
						COORD crMaxConSize = {0,0};

						if (pRCon->GetMaxConSize(&crMaxConSize))
						{
							if (rc.right > crMaxConSize.X)
								rc.right = crMaxConSize.X;

							if (rc.bottom > crMaxConSize.Y)
								rc.bottom = crMaxConSize.Y;
						}
					}
				}

				return rc;
			}
		} break;
		case CER_MONITOR:    // switch (tWhat)
		case CER_FULLSCREEN: // switch (tWhat)
		case CER_MAXIMIZED:  // switch (tWhat)
		case CER_RESTORE:    // switch (tWhat)
			//case CER_CORRECTED:
		{
			HMONITOR hMonitor = NULL;

			if (ghWnd)
				hMonitor = MonitorFromWindow(ghWnd, MONITOR_DEFAULTTOPRIMARY);
			else
				hMonitor = MonitorFromRect(&rFrom, MONITOR_DEFAULTTOPRIMARY);

			//if (tWhat != CER_CORRECTED)
			//    tFrom = tWhat;
			MONITORINFO mi; mi.cbSize = sizeof(mi);

			if (GetMonitorInfo(hMonitor, &mi))
			{
				switch(tFrom)
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
						RECT rcFrame = CalcMargins(CEM_FRAME);
						// Скорректируем размер окна до видимого на мониторе (рамка при максимизации уезжает за пределы экрана)
						rc.left -= rcFrame.left;
						rc.right += rcFrame.right;

						if (gpSet->isHideCaption || gpSet->isHideCaptionAlways())
							rc.top -= rcFrame.top;
						else
							rc.top -= rcFrame.bottom; // top включает и заголовок, а это нам не нужно

						rc.bottom += rcFrame.bottom;
						//if (gpSet->isHideCaption && gpConEmu->mb_MaximizedHideCaption && !gpSet->isHideCaptionAlways)
						//	rc.top -= GetSystemMetrics(SM_CYCAPTION);
						//// Скорректируем размер окна до видимого на мониторе (рамка при максимизации уезжает за пределы экрана)
						//rc.left -= GetSystemMetrics(SM_CXSIZEFRAME);
						//rc.right += GetSystemMetrics(SM_CXSIZEFRAME);
						//rc.top -= GetSystemMetrics(SM_CYSIZEFRAME);
						//rc.bottom += GetSystemMetrics(SM_CYSIZEFRAME);
						
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
						_ASSERTE(tFrom==CER_FULLSCREEN || tFrom==CER_MAXIMIZED);
				}
			}
			else
			{
				switch(tFrom)
				{
					case CER_MONITOR:
					{
						SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
					} break;
					case CER_FULLSCREEN:
					{
						rc = MakeRect(GetSystemMetrics(SM_CXFULLSCREEN),GetSystemMetrics(SM_CYFULLSCREEN));
					} break;
					case CER_MAXIMIZED:
					{
						rc = MakeRect(GetSystemMetrics(SM_CXMAXIMIZED),GetSystemMetrics(SM_CYMAXIMIZED));

						if (gpSet->isHideCaption && gpConEmu->mb_MaximizedHideCaption && !gpSet->isHideCaptionAlways())
							rc.top -= GetSystemMetrics(SM_CYCAPTION);

					} break;
					case CER_RESTORE:
					{
						RECT work;
						SystemParametersInfo(SPI_GETWORKAREA, 0, &work, 0);
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
							if (((rc.right + 30) <= work.left)
								|| ((rc.left + 30) >= work.right))
							{
								rc.left = work.left; rc.right = rc.left + w;
							}
							if (((rc.bottom + 30) <= work.top)
								|| ((rc.top + 30) >= work.bottom))
							{
								rc.top = work.top; rc.bottom = rc.top + h;
							}
						}
						else
						{
							WINDOWPLACEMENT wpl = {sizeof(WINDOWPLACEMENT)};
							if (ghWnd && GetWindowPlacement(ghWnd, &wpl))
								rc = wpl.rcNormalPosition;
							else
								rc = work;
						}
					} break;
					default:
						_ASSERTE(tFrom==CER_FULLSCREEN || tFrom==CER_MAXIMIZED);
				}
			}

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
			return rc;
		} break;
		default:
			break;
	}

	AddMargins(rc, rcAddShift);
	return rc; // Посчитали, возвращаем
}

POINT CConEmuMain::CalcTabMenuPos(CVirtualConsole* apVCon)
{
	POINT ptCur = {};
	if (apVCon)
	{
		RECT rcWnd;
		if (mp_TabBar && mp_TabBar->IsTabsShown())
		{
			mp_TabBar->GetActiveTabRect(&rcWnd);
			ptCur.x = rcWnd.left;
			ptCur.y = rcWnd.bottom;
		}
		else
		{
			GetWindowRect(mp_VActive->GetView(), &rcWnd);
			ptCur.x = rcWnd.left;
			ptCur.y = rcWnd.top;
		}
	}
	return ptCur;
}

/*!!!static!!*/
// Получить размер (правый нижний угол) окна по его клиентской области и наоборот
RECT CConEmuMain::MapRect(RECT rFrom, BOOL bFrame2Client)
{
	_ASSERTE(this!=NULL);
	RECT rcShift = CalcMargins(CEM_FRAME);

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

bool CConEmuMain::ScreenToVCon(LPPOINT pt, CVirtualConsole** ppVCon)
{
	_ASSERTE(this!=NULL);
	CVirtualConsole* lpVCon = GetVConFromPoint(*pt);

	if (!lpVCon)
		return false;

	HWND hView = lpVCon->GetView();
	ScreenToClient(hView, pt);

	if (ppVCon)
		*ppVCon = lpVCon;

	return true;
}

//// returns difference between window size and client area size of inWnd in outShift->x, outShift->y
//void CConEmuMain::GetCWShift(HWND inWnd, POINT *outShift)
//{
//    RECT cRect;
//
//    GetCWShift ( inWnd, &cRect );
//
//    outShift->x = cRect.right  - cRect.left;
//    outShift->y = cRect.bottom - cRect.top;
//}
//
//// returns margins between window frame and client area
//void CConEmuMain::GetCWShift(HWND inWnd, RECT *outShift)
//{
//    RECT cRect, wRect;
//    Get ClientRect(inWnd, &cRect); // The left and top members are zero. The right and bottom members contain the width and height of the window.
//    MapWindowPoints(inWnd, NULL, (LPPOINT)&cRect, 2);
//    GetWindowRect(inWnd, &wRect); // screen coordinates of the upper-left and lower-right corners of the window
//    outShift->top = wRect.top - cRect.top;          // <0
//    outShift->left = wRect.left - cRect.left;       // <0
//    outShift->bottom = wRect.bottom - cRect.bottom; // >0
//    outShift->right = wRect.right - cRect.right;    // >0
//}

//// Вернуть отступы со всех сторон от краев клиентской части главного окна до краев окна отрисовки
//RECT CConEmuMain::ConsoleOffsetRect()
//{
//    RECT rect; memset(&rect, 0, sizeof(rect));
//
//  if (gpConEmu->mp_TabBar->IsActive())
//      rect = gpConEmu->mp_TabBar->GetMargins();
//
//  /*rect.top = gpConEmu->mp_TabBar->IsActive()?gpConEmu->mp_TabBar->Height():0;
//    rect.left = 0;
//    rect.bottom = 0;
//    rect.right = 0;*/
//
//  return rect;
//}

//// Положение дочернего окна отрисовки
//RECT CConEmuMain::DCClientRect(RECT* pClient/*=NULL*/)
//{
//    RECT rect;
//  if (pClient)
//      rect = *pClient;
//  else
//      Get ClientRect(ghWnd, &rect);
//  if (gpConEmu->mp_TabBar->IsActive()) {
//      RECT mr = gpConEmu->mp_TabBar->GetMargins();
//      //rect.top += gpConEmu->mp_TabBar->Height();
//      rect.top += mr.top;
//      rect.left += mr.left;
//      rect.right -= mr.right;
//      rect.bottom -= mr.bottom;
//  }
//
//  if (pClient)
//      *pClient = rect;
//    return rect;
//}

//// returns console size in columns and lines calculated from current window size
//// rectInWindow - если true - с рамкой, false - только клиент
//COORD CConEmuMain::ConsoleSizeFromWindow(RECT* arect /*= NULL*/, bool frameIncluded /*= false*/, bool alreadyClient /*= false*/)
//{
//    COORD size;
//
//  if (!gpSet->Log Font.lfWidth || !gpSet->Log Font.lfHeight) {
//      MBoxAssert(FALSE);
//      // размер шрифта еще не инициализирован! вернем текущий размер консоли! TODO:
//      CONSOLE_SCREEN_BUFFER_INFO inf; memset(&inf, 0, sizeof(inf));
//      GetConsoleScreenBufferInfo(mp_VActive->hConOut(), &inf);
//      size = inf.dwSize;
//      return size;
//  }
//
//    RECT rect, consoleRect;
//    if (arect == NULL)
//    {
//      frameIncluded = false;
//        Get ClientRect(ghWnd, &rect);
//      consoleRect = ConsoleOffsetRect();
//    }
//    else
//    {
//        rect = *arect;
//      if (alreadyClient)
//          memset(&consoleRect, 0, sizeof(consoleRect));
//      else
//          consoleRect = ConsoleOffsetRect();
//    }
//
//    size.X = (rect.right - rect.left - (frameIncluded ? cwShift.x : 0) - consoleRect.left - consoleRect.right)
//      / gpSet->LogFont.lfWidth;
//    size.Y = (rect.bottom - rect.top - (frameIncluded ? cwShift.y : 0) - consoleRect.top - consoleRect.bottom)
//      / gpSet->LogFont.lfHeight;
//    #ifdef MSGLOGGER
//        char szDbg[100]; wsprintfA(szDbg, "   ConsoleSizeFromWindow={%i,%i}\n", size.X, size.Y);
//        DEBUGLOGFILE(szDbg);
//    #endif
//    return size;
//}

//// return window size in pixels calculated from console size
//RECT CConEmuMain::WindowSizeFromConsole(COORD consoleSize, bool rectInWindow /*= false*/, bool clientOnly /*= false*/)
//{
//    RECT rect;
//    rect.top = 0;
//    rect.left = 0;
//    RECT offsetRect;
//  if (clientOnly)
//      memset(&offsetRect, 0, sizeof(RECT));
//  else
//      offsetRect = ConsoleOffsetRect();
//    rect.bottom = consoleSize.Y * gpSet->LogFont.lfHeight + (rectInWindow ? cwShift.y : 0) + offsetRect.top + offsetRect.bottom;
//    rect.right = consoleSize.X * gpSet->LogFont.lfWidth + (rectInWindow ? cwShift.x : 0) + offsetRect.left + offsetRect.right;
//    #ifdef MSGLOGGER
//        char szDbg[100]; wsprintfA(szDbg, "   WindowSizeFromConsole={%i,%i}\n", rect.right,rect.bottom);
//        DEBUGLOGFILE(szDbg);
//    #endif
//    return rect;
//}

// size in columns and lines
void CConEmuMain::SetConsoleWindowSize(const COORD& size, bool updateInfo, CVirtualConsole* apVCon)
{
	if (!apVCon)
		apVCon = mp_VActive;

	// Это не совсем корректно... ntvdm.exe не выгружается после выхода из 16бит приложения
	if (isNtvdm())
	{
		//if (size.X == 80 && size.Y>25 && lastSize1.X != size.X && size.Y == lastSize1.Y) {
		TODO("Ntvdm почему-то не всегда устанавливает ВЫСОТУ консоли в 25/28/50 символов...")
		//}
		return; // запрет изменения размеров консоли для 16бит приложений
	}

#ifdef MSGLOGGER
	char szDbg[100]; wsprintfA(szDbg, "SetConsoleWindowSize({%i,%i},%i)\n", size.X, size.Y, updateInfo);
	DEBUGLOGFILE(szDbg);
#endif
	m_LastConSize = size;

	if (isPictureView())
	{
		isPiewUpdate = true;
		return;
	}

	// update size info
	// !!! Это вроде делает консоль
	WARNING("updateInfo убить");
	/*if (updateInfo && !mb_isFullScreen && !isZoomed() && !isIconic())
	{
	    gpSet->UpdateSize(size.X, size.Y);
	}*/
	RECT rcCon = MakeRect(size.X,size.Y);

	if (apVCon)
	{
		if (!apVCon->RCon()->SetConsoleSize(size.X,size.Y))
			rcCon = MakeRect(apVCon->TextWidth,apVCon->TextHeight);
	}

	RECT rcWnd = CalcRect(CER_MAIN, rcCon, CER_CONSOLE);
	RECT wndR; GetWindowRect(ghWnd, &wndR); // текущий XY
	MOVEWINDOW(ghWnd, wndR.left, wndR.top, rcWnd.right, rcWnd.bottom, 1);
}

// Изменить размер консоли по размеру окна (главного)
void CConEmuMain::SyncConsoleToWindow()
{
	if (mb_SkipSyncSize || isNtvdm() || !mp_VActive)
		return;

	_ASSERTE(mn_InResize <= 1);

#ifdef _DEBUG
	if (change2WindowMode!=(DWORD)-1)
	{
		_ASSERTE(change2WindowMode==(DWORD)-1);
	}
#endif

	mp_VActive->RCon()->SyncConsole2Window();
}

void CConEmuMain::SyncNtvdm()
{
	//COORD sz = {mp_VActive->TextWidth, mp_VActive->TextHeight};
	//SetConsoleWindowSize(sz, false);
	OnSize();
}

// Установить размер основного окна по текущему размеру mp_VActive
void CConEmuMain::SyncWindowToConsole()
{
	DEBUGLOGFILE("SyncWindowToConsole\n");

	if (mb_SkipSyncSize || !mp_VActive)
		return;

#ifdef _DEBUG
	_ASSERTE(GetCurrentThreadId() == mn_MainThreadId);
	if (mp_VActive->TextWidth == 80)
	{
		int nDbg = mp_VActive->TextWidth;
	}
#endif

	CRealConsole* pRCon = mp_VActive->RCon();

	if (pRCon && (mp_VActive->TextWidth != pRCon->TextWidth() || mp_VActive->TextHeight != pRCon->TextHeight()))
	{
		_ASSERTE(FALSE);
		mp_VActive->Update();
	}

	RECT rcDC = mp_VActive->GetRect();
	/*MakeRect(mp_VActive->Width, mp_VActive->Height);
	if (mp_VActive->Width == 0 || mp_VActive->Height == 0) {
	rcDC = MakeRect(mp_VActive->winSize.X, mp_VActive->winSize.Y);
	}*/
	//_ASSERTE(rcDC.right>250 && rcDC.bottom>200);
	RECT rcWnd = CalcRect(CER_MAIN, rcDC, CER_DC); // размеры окна
	//GetCWShift(ghWnd, &cwShift);
	RECT wndR; GetWindowRect(ghWnd, &wndR); // текущий XY

	if (gpSetCls->isAdvLogging)
	{
		char szInfo[128]; wsprintfA(szInfo, "SyncWindowToConsole(Cols=%i, Rows=%i)", mp_VActive->TextWidth, mp_VActive->TextHeight);
		mp_VActive->RCon()->LogString(szInfo, TRUE);
	}

	gpSetCls->UpdateSize(mp_VActive->TextWidth, mp_VActive->TextHeight);
	MOVEWINDOW(ghWnd, wndR.left, wndR.top, rcWnd.right, rcWnd.bottom, 1);
}

void CConEmuMain::AutoSizeFont(const RECT &rFrom, enum ConEmuRect tFrom)
{
	if (gpSet->isFontAutoSize)
	{
		// В 16бит режиме - не заморачиваться пока
		if (!gpConEmu->isNtvdm())
		{
			if (!gpSet->wndWidth || !gpSet->wndHeight)
			{
				MBoxAssert(gpSet->wndWidth!=0 && gpSet->wndHeight!=0);
			}
			else
			{
				RECT rc = rFrom;

				if (tFrom == CER_MAIN)
				{
					rc = CalcRect(CER_DC, rFrom, CER_MAIN);
				}
				else if (tFrom == CER_MAINCLIENT)
				{
					rc = CalcRect(CER_DC, rFrom, CER_MAINCLIENT);
				}
				else
				{
					MBoxAssert(tFrom==CER_MAINCLIENT || tFrom==CER_MAIN);
					return;
				}

				// !!! Для CER_DC размер в rc.right
				int nFontW = (rc.right - rc.left) / gpSet->wndWidth;

				if (nFontW < 5) nFontW = 5;

				int nFontH = (rc.bottom - rc.top) / gpSet->wndHeight;

				if (nFontH < 8) nFontH = 8;

				gpSetCls->AutoRecreateFont(nFontW, nFontH);
			}
		}
	}
}

void CConEmuMain::StoreNormalRect(RECT* prcWnd)
{
	// Обновить коордианты в gpSet, если требуется
	// Если сейчас окно в смене размера - игнорируем, размер запомнит SetWindowMode
	if ((change2WindowMode == (DWORD)-1) && !mb_isFullScreen && !isZoomed() && !isIconic())
	{
		if (prcWnd)
			mrc_StoredNormalRect = *prcWnd;
		else
			GetWindowRect(ghWnd, &mrc_StoredNormalRect);

		gpSetCls->UpdatePos(mrc_StoredNormalRect.left, mrc_StoredNormalRect.top);

		if (mp_VActive)
		{
			// При ресайзе через окно настройки - mp_VActive еще не перерисовался
			// так что и TextWidth/TextHeight не обновился
			//-- gpSetCls->UpdateSize(mp_VActive->TextWidth, mp_VActive->TextHeight);
			if (mp_VActive->RCon())
			{
				gpSetCls->UpdateSize(mp_VActive->RCon()->TextWidth(), mp_VActive->RCon()->TextHeight());
			}
		}
	}
}

BOOL CConEmuMain::ShowWindow(int anCmdShow)
{
#if 0
	STARTUPINFO si = {sizeof(si)};
	GetStartupInfo(&si);
#endif

	BOOL lbRc = apiShowWindow(ghWnd, anCmdShow);

#if 0
	if (anCmdShow == SW_SHOWNORMAL)
	{
		if (((gpSet->isHideCaption || gpSet->isHideCaptionAlways()) && isZoomed()))
		{
			// Если в свойствах ярлыка указано "Maximized"/"Iconic" - то первый ShowWindow ИГНОРИРУЕТСЯ
			_ASSERTE(si.wShowWindow == SW_SHOWMAXIMIZED);
			lbRc = apiShowWindow(ghWnd, anCmdShow);
		}
	}
#endif

	return lbRc;
}

bool CConEmuMain::SetWindowMode(uint inMode, BOOL abForce)
{
	if (inMode != rNormal && inMode != rMaximized && inMode != rFullScreen)
		inMode = rNormal; // ошибка загрузки настроек?

	if (!isMainThread())
	{
		PostMessage(ghWnd, mn_MsgSetWindowMode, inMode, 0);
		return false;
	}

	if (inMode == rFullScreen && gpSet->isDesktopMode)
		inMode = (mb_isFullScreen || isZoomed()) ? rNormal : rMaximized; // FullScreen на Desktop-е невозможен

#ifdef _DEBUG
	DWORD_PTR dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE);
#endif
#ifndef _DEBUG

	//2009-04-22 Если открыт PictureView - лучше не дергаться...
	if (isPictureView())
		return false;

#endif
	SetCursor(LoadCursor(NULL,IDC_WAIT));
	mb_PassSysCommand = true;
	//WindowPlacement -- использовать нельзя, т.к. он работает в координатах Workspace, а не Screen!
	RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);
	RECT consoleSize = MakeRect(gpSet->wndWidth, gpSet->wndHeight);
	bool canEditWindowSizes = false;
	bool lbRc = false;
	static bool bWasSetFullscreen = false;
	change2WindowMode = inMode;
	// Сброс флагов ресайза мышкой
	mouse.state &= ~(MOUSE_SIZING_BEGIN|MOUSE_SIZING_TODO);

	if (bWasSetFullscreen && inMode != rFullScreen)
	{
		if (mp_TaskBar2)
		{
			if (!gpSet->isDesktopMode)
				mp_TaskBar2->MarkFullscreenWindow(ghWnd, FALSE);

			bWasSetFullscreen = false;
		}
	}

	CRealConsole* pRCon = (gpSetCls->isAdvLogging!=0) ? ActiveCon()->RCon() : NULL;

	if (pRCon) pRCon->LogString((inMode==rNormal) ? "SetWindowMode(rNormal)" :
		                           (inMode==rMaximized) ? "SetWindowMode(rMaximized)" :
		                           (inMode==rFullScreen) ? "SetWindowMode(rFullScreen)" : "SetWindowMode(INVALID)",
		                           TRUE);

	//!!!
	switch(inMode)
	{
		case rNormal:
		{
			DEBUGLOGFILE("SetWindowMode(rNormal)\n");
			AutoSizeFont(mrc_Ideal, CER_MAIN);
			// Расчитать размер по оптимальному WindowRect
			RECT rcCon = CalcRect(CER_CONSOLE, mrc_Ideal, CER_MAIN, mp_VActive);

			if (!rcCon.right || !rcCon.bottom) { rcCon.right = gpSet->wndWidth; rcCon.bottom = gpSet->wndHeight; }

			if (mp_VActive && !mp_VActive->RCon()->SetConsoleSize(rcCon.right, rcCon.bottom))
			{
				if (pRCon) pRCon->LogString("!!!SetConsoleSize FAILED!!!");

				mb_PassSysCommand = false;
				goto wrap;
			}

			//mb_InRestore = TRUE;
			//HRGN hRgn = CreateWindowRgn();
			//SetWindowRgn(ghWnd, hRgn, TRUE);
			//mb_InRestore = FALSE;

			if (isIconic() || (isZoomed() && !mb_MaximizedHideCaption))
			{
				//apiShow Window(ghWnd, SW_SHOWNORMAL); // WM_SYSCOMMAND использовать не хочется...
				mb_IgnoreSizeChange = TRUE;

				if (gpSet->isDesktopMode)
				{
					RECT rcNormal = CalcRect(CER_RESTORE, MakeRect(0,0), CER_RESTORE);
					DWORD_PTR dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE);
					if (dwStyle & WS_MAXIMIZE)
						SetWindowLong(ghWnd, GWL_STYLE, (dwStyle&~WS_MAXIMIZE));
					SetWindowPos(ghWnd, HWND_TOP, 
						rcNormal.left, rcNormal.top, 
						rcNormal.right-rcNormal.left, rcNormal.bottom-rcNormal.top,
						SWP_NOCOPYBITS|SWP_SHOWWINDOW);
				}
				else if (IsWindowVisible(ghWnd))
				{
					if (pRCon && gpSetCls->isAdvLogging) pRCon->LogString("WM_SYSCOMMAND(SC_RESTORE)");

					DefWindowProc(ghWnd, WM_SYSCOMMAND, SC_RESTORE, 0); //2009-04-22 Было SendMessage
				}
				else
				{
					if (pRCon && gpSetCls->isAdvLogging) pRCon->LogString("ShowWindow(SW_SHOWNORMAL)");

					ShowWindow(SW_SHOWNORMAL);
				}

				//RePaint();
				mb_IgnoreSizeChange = FALSE;

				// Сбросить (заранее), вдруг оно isIconic?
				if (mb_MaximizedHideCaption)
					mb_MaximizedHideCaption = FALSE;

				if (pRCon && gpSetCls->isAdvLogging) pRCon->LogString("OnSize(-1)");

				OnSize(-1); // подровнять ТОЛЬКО дочерние окошки
			}

			// Сбросить (однозначно)
			if (mb_MaximizedHideCaption)
				mb_MaximizedHideCaption = FALSE;

			RECT rcNew = CalcRect(CER_MAIN, consoleSize, CER_CONSOLE);
			//int nWidth = rcNew.right-rcNew.left;
			//int nHeight = rcNew.bottom-rcNew.top;
			rcNew.left+=gpSet->wndX; rcNew.top+=gpSet->wndY;
			rcNew.right+=gpSet->wndX; rcNew.bottom+=gpSet->wndY;
			// 2010-02-14 Проверку делаем ТОЛЬКО при загрузке настроек и включенном каскаде
			//// Параметры именно такие, результат - просто подгонка rcNew под рабочую область текущего монитора
			//rcNew = CalcRect(CER_CORRECTED, rcNew, CER_MAXIMIZED);
#ifdef _DEBUG
			WINDOWPLACEMENT wpl; memset(&wpl,0,sizeof(wpl)); wpl.length = sizeof(wpl);
			GetWindowPlacement(ghWnd, &wpl);
#endif

			if (pRCon && gpSetCls->isAdvLogging)
			{
				char szInfo[128]; wsprintfA(szInfo, "SetWindowPos(X=%i, Y=%i, W=%i, H=%i)", rcNew.left, rcNew.top, rcNew.right-rcNew.left, rcNew.bottom-rcNew.top);
				pRCon->LogString(szInfo);
			}

			SetWindowPos(ghWnd, NULL, rcNew.left, rcNew.top, rcNew.right-rcNew.left, rcNew.bottom-rcNew.top, SWP_NOZORDER);
#ifdef _DEBUG
			GetWindowPlacement(ghWnd, &wpl);
#endif

			if (ghOpWnd)
				CheckRadioButton(gpSetCls->mh_Tabs[gpSetCls->thi_Main], rNormal, rFullScreen, rNormal);

			mb_isFullScreen = false;

			if (!IsWindowVisible(ghWnd))
				ShowWindow(SW_SHOWNORMAL);

			#ifdef _DEBUG
			GetWindowPlacement(ghWnd, &wpl);
			#endif

			// Если это во время загрузки - то до первого ShowWindow - isIconic возвращает FALSE
			if (isIconic() || isZoomed())
			{
				ShowWindow(SW_SHOWNORMAL); // WM_SYSCOMMAND использовать не хочется...
				// что-то после AltF9, AltF9 уголки остаются не срезанными...
				//hRgn = CreateWindowRgn();
				//SetWindowRgn(ghWnd, hRgn, TRUE);
			}

			UpdateWindowRgn();
			//#ifdef _DEBUG
			//GetWindowPlacement(ghWnd, &wpl);
			//UpdateWindow(ghWnd);
			//#endif
		} break;
		case rMaximized:
		{
			DEBUGLOGFILE("SetWindowMode(rMaximized)\n");

			// Обновить коордианты в gpSet, если требуется
			if (!mb_isFullScreen && !isZoomed() && !isIconic())
				StoreNormalRect(&rcWnd);

			if (!gpSet->isHideCaption && !gpSet->isHideCaptionAlways())
			{
				RECT rcMax = CalcRect(CER_MAXIMIZED, MakeRect(0,0), CER_MAXIMIZED);
				AutoSizeFont(rcMax, CER_MAIN);
				RECT rcCon = CalcRect(CER_CONSOLE, rcMax, CER_MAIN);

				WARNING("Может обломаться из-за максимального размера консоли");
				// в этом случае хорошо бы установить максимально возможный и отцентрировать ее в ConEmu
				if (mp_VActive && !mp_VActive->RCon()->SetConsoleSize(rcCon.right,rcCon.bottom))
				{
					if (pRCon) pRCon->LogString("!!!SetConsoleSize FAILED!!!");

					mb_PassSysCommand = false;
					goto wrap;
				}

				if (!isZoomed())
				{
					mb_IgnoreSizeChange = TRUE;
					InvalidateAll();
					if (!gpSet->isDesktopMode)
					{
						ShowWindow(SW_SHOWMAXIMIZED);
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
						SetWindowLong(ghWnd, GWL_STYLE, (dwStyle|WS_MAXIMIZE));
						SetWindowPos(ghWnd, HWND_TOP, 
							rcMax.left, rcMax.top, 
							rcMax.right-rcMax.left, rcMax.bottom-rcMax.top,
							SWP_NOCOPYBITS|SWP_SHOWWINDOW);
					}
					mb_IgnoreSizeChange = FALSE;
					RePaint();

					if (pRCon && gpSetCls->isAdvLogging) pRCon->LogString("OnSize(-1).2");

					OnSize(-1); // консоль уже изменила свой размер
				}

				if (ghOpWnd)
					CheckRadioButton(gpSetCls->mh_Tabs[gpSetCls->thi_Main], rNormal, rFullScreen, rMaximized);

				mb_isFullScreen = false;

				if (!IsWindowVisible(ghWnd))
				{
					mb_IgnoreSizeChange = TRUE;
					ShowWindow(SW_SHOWMAXIMIZED);
					mb_IgnoreSizeChange = FALSE;

					if (pRCon && gpSetCls->isAdvLogging) pRCon->LogString("OnSize(-1).3");

					OnSize(-1); // консоль уже изменила свой размер
				}

				UpdateWindowRgn();
			} // if (!gpSet->isHideCaption)
			else
			{
				// (gpSet->isHideCaption)
				if (!isZoomed() || (mb_isFullScreen || isIconic()) || abForce)
				{
					mb_MaximizedHideCaption = TRUE;
					RECT rcMax = CalcRect(CER_MAXIMIZED, MakeRect(0,0), CER_MAXIMIZED);
					AutoSizeFont(rcMax, CER_MAIN);
					RECT rcCon = CalcRect(CER_CONSOLE, rcMax, CER_MAIN);

					WARNING("Может обломаться из-за максимального размера консоли");
					// в этом случае хорошо бы установить максимально возможный и отцентрировать ее в ConEmu
					if (mp_VActive && !mp_VActive->RCon()->SetConsoleSize(rcCon.right,rcCon.bottom))
					{
						if (pRCon) pRCon->LogString("!!!SetConsoleSize FAILED!!!");

						mb_PassSysCommand = false;
						goto wrap;
					}

					RECT rcShift = CalcMargins(CEM_FRAME);
					//GetCWShift(ghWnd, &rcShift); // Обновить, на всякий случай
					// Умолчания
					ptFullScreenSize.x = GetSystemMetrics(SM_CXSCREEN)+rcShift.left+rcShift.right;
					ptFullScreenSize.y = GetSystemMetrics(SM_CYSCREEN)+rcShift.top+rcShift.bottom;
					// которые нужно уточнить для текущего монитора!
					MONITORINFO mi = {sizeof(mi)};
					HMONITOR hMon = MonitorFromWindow(ghWnd, MONITOR_DEFAULTTONEAREST);

					if (hMon)
					{
						if (GetMonitorInfo(hMon, &mi))
						{
							ptFullScreenSize.x = (mi.rcWork.right-mi.rcWork.left)+rcShift.left+rcShift.right;
							ptFullScreenSize.y = (mi.rcWork.bottom-mi.rcWork.top)+rcShift.top+rcShift.bottom;
						}
					}

					// Тут нужен "чистый" ::IsZoomed
					if (isIconic() || ::IsZoomed(ghWnd))
					{
						// Если окно свернуто или "реально" максимизировано - показать нормальным
						mb_IgnoreSizeChange = TRUE;
						DWORD dwStyle = GetWindowLong(ghWnd, GWL_STYLE);

						if ((dwStyle & WS_MINIMIZE))
						{
							ShowWindow(SW_SHOWNORMAL);
						}

						if ((dwStyle & (WS_MINIMIZE|WS_MAXIMIZE)) != 0)
						{
							dwStyle &= ~(WS_MINIMIZE|WS_MAXIMIZE);
							SetWindowLong(ghWnd, GWL_STYLE, dwStyle);
						}

						//ShowWindow(SW_SHOWNORMAL);
						// Сбросить
						_ASSERTE(mb_MaximizedHideCaption);
						mb_IgnoreSizeChange = FALSE;
						RePaint();
					}

					if (mp_TaskBar2)
					{
						if (!gpSet->isDesktopMode)
							mp_TaskBar2->MarkFullscreenWindow(ghWnd, FALSE);

						bWasSetFullscreen = true;
					}

					// for virtual screens mi.rcWork. may contains negative values...

					if (pRCon && gpSetCls->isAdvLogging)
					{
						char szInfo[128]; wsprintfA(szInfo, "SetWindowPos(X=%i, Y=%i, W=%i, H=%i)", -rcShift.left+mi.rcWork.left,-rcShift.top+mi.rcWork.top, ptFullScreenSize.x,ptFullScreenSize.y);
						pRCon->LogString(szInfo);
					}

					/* */ SetWindowPos(ghWnd, NULL,
					                   -rcShift.left+mi.rcWork.left,-rcShift.top+mi.rcWork.top,
					                   ptFullScreenSize.x,ptFullScreenSize.y,
					                   SWP_NOZORDER);

					if (ghOpWnd)
						CheckRadioButton(gpSetCls->mh_Tabs[gpSetCls->thi_Main], rNormal, rMaximized, rMaximized);
				}

				mb_isFullScreen = false;

				if (!IsWindowVisible(ghWnd))
				{
					mb_IgnoreSizeChange = TRUE;
					ShowWindow(SW_SHOWNORMAL);
					mb_IgnoreSizeChange = FALSE;

					if (pRCon && gpSetCls->isAdvLogging) pRCon->LogString("OnSize(-1).3");

					OnSize(-1);  // консоль уже изменила свой размер
				}

				UpdateWindowRgn();
			} // (gpSet->isHideCaption)
		} break;
		case rFullScreen:
			DEBUGLOGFILE("SetWindowMode(rFullScreen)\n");

			// Обновить коордианты в gpSet, если требуется
			if (!mb_isFullScreen && !isZoomed() && !isIconic())
				StoreNormalRect(&rcWnd);

			if (!mb_isFullScreen || (isZoomed() || isIconic()))
			{
				RECT rcMax = CalcRect(CER_FULLSCREEN, MakeRect(0,0), CER_FULLSCREEN);
				AutoSizeFont(rcMax, CER_MAINCLIENT);
				RECT rcCon = CalcRect(CER_CONSOLE, rcMax, CER_MAINCLIENT);

				WARNING("Может обломаться из-за максимального размера консоли");
				// в этом случае хорошо бы установить максимально возможный и отцентрировать ее в ConEmu
				if (mp_VActive && !mp_VActive->RCon()->SetConsoleSize(rcCon.right,rcCon.bottom))
				{
					if (pRCon) pRCon->LogString("!!!SetConsoleSize FAILED!!!");

					mb_PassSysCommand = false;
					goto wrap;
				}

				mb_isFullScreen = true;
				isWndNotFSMaximized = isZoomed();
				RECT rcShift = CalcMargins(CEM_FRAME);
				//GetCWShift(ghWnd, &rcShift); // Обновить, на всякий случай
				// Умолчания
				ptFullScreenSize.x = GetSystemMetrics(SM_CXSCREEN)+rcShift.left+rcShift.right;
				ptFullScreenSize.y = GetSystemMetrics(SM_CYSCREEN)+rcShift.top+rcShift.bottom;
				// которые нужно уточнить для текущего монитора!
				MONITORINFO mi; memset(&mi, 0, sizeof(mi)); mi.cbSize = sizeof(mi);
				HMONITOR hMon = MonitorFromWindow(ghWnd, MONITOR_DEFAULTTONEAREST);

				if (hMon)
				{
					if (GetMonitorInfo(hMon, &mi))
					{
						ptFullScreenSize.x = (mi.rcMonitor.right-mi.rcMonitor.left)+rcShift.left+rcShift.right;
						ptFullScreenSize.y = (mi.rcMonitor.bottom-mi.rcMonitor.top)+rcShift.top+rcShift.bottom;
					}
				}

				if (isIconic() || isZoomed())
				{
					mb_IgnoreSizeChange = TRUE;
					ShowWindow(SW_SHOWNORMAL);

					// Сбросить
					if (mb_MaximizedHideCaption)
						mb_MaximizedHideCaption = FALSE;

					mb_IgnoreSizeChange = FALSE;
					RePaint();
				}

				if (mp_TaskBar2)
				{
					if (!gpSet->isDesktopMode)
						mp_TaskBar2->MarkFullscreenWindow(ghWnd, TRUE);

					bWasSetFullscreen = true;
				}

				// for virtual screens mi.rcMonitor. may contains negative values...

				if (pRCon && gpSetCls->isAdvLogging)
				{
					char szInfo[128]; wsprintfA(szInfo, "SetWindowPos(X=%i, Y=%i, W=%i, H=%i)", -rcShift.left+mi.rcMonitor.left,-rcShift.top+mi.rcMonitor.top, ptFullScreenSize.x,ptFullScreenSize.y);
					pRCon->LogString(szInfo);
				}

				RECT rcFrame = CalcMargins(CEM_FRAME);
				// ptFullScreenSize содержит "скорректированный" размер (он больше монитора)
				UpdateWindowRgn(rcFrame.left, rcFrame.top,
				                mi.rcMonitor.right-mi.rcMonitor.left, mi.rcMonitor.bottom-mi.rcMonitor.top);
				/* */ SetWindowPos(ghWnd, NULL,
				                   -rcShift.left+mi.rcMonitor.left,-rcShift.top+mi.rcMonitor.top,
				                   ptFullScreenSize.x,ptFullScreenSize.y,
				                   SWP_NOZORDER);

				if (ghOpWnd)
					CheckRadioButton(gpSetCls->mh_Tabs[gpSetCls->thi_Main], rNormal, rFullScreen, rFullScreen);
			}

			if (!IsWindowVisible(ghWnd))
			{
				mb_IgnoreSizeChange = TRUE;
				ShowWindow(SW_SHOWNORMAL);
				mb_IgnoreSizeChange = FALSE;

				if (pRCon && gpSetCls->isAdvLogging) pRCon->LogString("OnSize(-1).3");

				OnSize(-1);  // консоль уже изменила свой размер
			}

			break;
	}

	if (pRCon && gpSetCls->isAdvLogging) pRCon->LogString("SetWindowMode done");

	WindowMode = inMode; // Запомним!
	canEditWindowSizes = inMode == rNormal;

	if (ActiveCon())
	{
		ActiveCon()->RCon()->SyncGui2Window();
	}

	if (ghOpWnd)
	{
		EnableWindow(GetDlgItem(ghOpWnd, tWndWidth), canEditWindowSizes);
		EnableWindow(GetDlgItem(ghOpWnd, tWndHeight), canEditWindowSizes);
	}

	//SyncConsoleToWindow(); 2009-09-10 А это вроде вообще не нужно - ресайз консоли уже сделан
	mb_PassSysCommand = false;
	lbRc = true;
wrap:
	mb_InRestore = FALSE;

	// В случае облома изменения размера консоли - может слететь признак
	// полноэкранности у панели задач. Вернем его...
	if (mp_TaskBar2)
	{
		if (bWasSetFullscreen != mb_isFullScreen)
		{
			if (!gpSet->isDesktopMode)
				mp_TaskBar2->MarkFullscreenWindow(ghWnd, mb_isFullScreen);

			bWasSetFullscreen = mb_isFullScreen;
		}
	}

	mp_TabBar->OnWindowStateChanged();
#ifdef _DEBUG
	dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE);
#endif
	change2WindowMode = -1;
	TODO("Что-то курсор иногда остается APPSTARTING...");
	SetCursor(LoadCursor(NULL,IDC_ARROW));
	//PostMessage(ghWnd, WM_SETCURSOR, -1, -1);
	return lbRc;
}

void CConEmuMain::ForceShowTabs(BOOL abShow)
{
	//if (!mp_VActive)
	//  return;
	//2009-05-20 Раз это Force - значит на возможность получить табы из фара забиваем! Для консоли показывается "Console"
	BOOL lbTabsAllowed = abShow /*&& gpConEmu->mp_TabBar->IsAllowed()*/;

	if (abShow && !gpConEmu->mp_TabBar->IsTabsShown() && gpSet->isTabs && lbTabsAllowed)
	{
		gpConEmu->mp_TabBar->Activate();
		//ConEmuTab tab; memset(&tab, 0, sizeof(tab));
		//tab.Pos=0;
		//tab.Current=1;
		//tab.Type = 1;
		//gpConEmu->mp_TabBar->Update(&tab, 1);
		//mp_VActive->RCon()->SetTabs(&tab, 1);
		gpConEmu->mp_TabBar->Update();
		//gbPostUpdateWindowSize = true; // 2009-07-04 Resize выполняет сам TabBar
	}
	else if (!abShow)
	{
		gpConEmu->mp_TabBar->Deactivate();
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
	//        SyncConsoleToWindow();
	//    }*/
	//}
}

bool CConEmuMain::isIconic()
{
	bool bIconic = ::IsIconic(ghWnd);
	return bIconic;
}

bool CConEmuMain::isZoomed()
{
	TODO("возможно mb_InRestore можно заменить на change2WindowMode");

	if (mb_InRestore)
		return false;

	bool bZoomed = (mb_MaximizedHideCaption && !::IsIconic(ghWnd)) || ::IsZoomed(ghWnd);
	return bZoomed;
}

bool CConEmuMain::isFullScreen()
{
	if (mb_isFullScreen)
	{
		if (isZoomed())
		{
			_ASSERTE(mb_isFullScreen==false && isZoomed());
			return false;
		}
	}
	return mb_isFullScreen;
}

// abCorrect2Ideal==TRUE приходит из TabBarClass::UpdatePosition(),
// это происходит при отображении/скрытии "автотабов"
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
		if (!isZoomed() && !mb_isFullScreen)
		{
			// Вобщем-то интересует только X,Y
			RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);

			// Пользователи жалуются на смену размера консоли
			#if 1
			RECT rcConsole = {}, rcCompWnd = {};
			TODO("DoubleView: нужно с учетом видимых консолией");
			if (mp_VActive)
			{
				rcConsole.right = mp_VActive->TextWidth;
				rcConsole.bottom = mp_VActive->TextHeight;
				rcCompWnd = CalcRect(CER_MAIN, rcConsole, CER_CONSOLE);
				AutoSizeFont(rcCompWnd, CER_MAIN);
			}
			else
			{
				rcCompWnd = rcWnd; // не менять?
			}

			#else
			// Выполняем всегда, даже если размер уже соответсвует...
			AutoSizeFont(mrc_Ideal, CER_MAIN);
			RECT rcConsole = CalcRect(CER_CONSOLE, mrc_Ideal, CER_MAIN);
			RECT rcCompWnd = CalcRect(CER_MAIN, rcConsole, CER_CONSOLE);
			#endif

			// При показе/скрытии табов высота консоли может "прыгать"
			// Ее нужно скорректировать. Поскольку идет реальный ресайз
			// главного окна - OnSize вызовается автоматически
			_ASSERTE(isMainThread());

			//m_Child->SetRedraw(FALSE);
			TODO("DoubleView: запретить во всех видимых");
			if (mp_VActive)
			{
				mp_VActive->SetRedraw(FALSE);

				if (mp_VActive->RCon())
					mp_VActive->RCon()->SetConsoleSize(rcConsole.right, rcConsole.bottom, 0, CECMD_SETSIZESYNC);

				TODO("DoubleView: во всех видимых");
				mp_VActive->SetRedraw(TRUE);
				mp_VActive->Redraw();
			}

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
			AutoSizeFont(client, CER_MAINCLIENT);
			RECT rcConsole = CalcRect(CER_CONSOLE, client, CER_MAINCLIENT);
			TODO("DoubleView: во всех видимых");
			mp_VActive->SetRedraw(FALSE);
			mp_VActive->RCon()->SetConsoleSize(rcConsole.right, rcConsole.bottom, 0, CECMD_SETSIZESYNC);
			mp_VActive->SetRedraw(TRUE);
			mp_VActive->Redraw();
		}

		// Поправить! Может изменилось!
		client = GetGuiClientRect();
	}

#ifdef _DEBUG
	dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE);
#endif
	OnSize(isZoomed() ? SIZE_MAXIMIZED : SIZE_RESTORED,
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
	if (ActiveCon()->TrackMouse())
		lbCapture = TRUE;

	return lbCapture;
}

void CConEmuMain::OnAlwaysShowScrollbar()
{
	for (size_t i = 0; i < countof(mp_VCon); i++)
	{
		if (mp_VCon[i])
			mp_VCon[i]->OnAlwaysShowScrollbar();
	}
}

void CConEmuMain::OnConsoleResize(BOOL abPosted/*=FALSE*/)
{
	//MSetter lInConsoleResize(&mb_InConsoleResize);
	// Выполняться должно в нити окна, иначе можем повиснуть
	static bool lbPosted = false;
	abPosted = (mn_MainThreadId == GetCurrentThreadId());

	if (!abPosted)
	{
		if (gpSetCls->isAdvLogging)
			mp_VActive->RCon()->LogString("OnConsoleResize(abPosted==false)", TRUE);

		if (!lbPosted)
		{
			lbPosted = true; // чтобы post не накапливались
#ifdef _DEBUG
			int nCurConWidth = (int)mp_VActive->RCon()->TextWidth();
			int nCurConHeight = (int)mp_VActive->RCon()->TextHeight();
#endif
			PostMessage(ghWnd, mn_PostConsoleResize, 0,0);
		}

		return;
	}

	lbPosted = false;

	if (isIconic())
	{
		if (gpSetCls->isAdvLogging)
			mp_VActive->RCon()->LogString("OnConsoleResize ignored, because of iconic");

		return; // если минимизировано - ничего не делать
	}

	// Было ли реальное изменение размеров?
	BOOL lbSizingToDo  = (mouse.state & MOUSE_SIZING_TODO) == MOUSE_SIZING_TODO;
	bool lbIsSizing = isSizing();
	bool lbLBtnPressed = isPressed(VK_LBUTTON);

	if (lbIsSizing && !lbLBtnPressed)
	{
		// Сборс всех флагов ресайза мышкой
		mouse.state &= ~(MOUSE_SIZING_BEGIN|MOUSE_SIZING_TODO);
	}

	if (gpSetCls->isAdvLogging)
	{
		char szInfo[160]; wsprintfA(szInfo, "OnConsoleResize: mouse.state=0x%08X, SizingToDo=%i, IsSizing=%i, LBtnPressed=%i, gbPostUpdateWindowSize=%i",
		                            mouse.state, (int)lbSizingToDo, (int)lbIsSizing, (int)lbLBtnPressed, (int)gbPostUpdateWindowSize);
		mp_VActive->RCon()->LogString(szInfo, TRUE);
	}

	//COORD c = ConsoleSizeFromWindow();
	RECT client = GetGuiClientRect();

	// Проверим, вдруг не отработал isIconic
	if (client.bottom > 10)
	{
		AutoSizeFont(client, CER_MAINCLIENT);
		RECT c = CalcRect(CER_CONSOLE, client, CER_MAINCLIENT);
		// чтобы не насиловать консоль лишний раз - реальное измененение ее размеров только
		// при отпускании мышкой рамки окна
		BOOL lbSizeChanged = FALSE;
		int nCurConWidth = (int)mp_VActive->RCon()->TextWidth();
		int nCurConHeight = (int)mp_VActive->RCon()->TextHeight();

		if (mp_VActive)
		{
			lbSizeChanged = (c.right != nCurConWidth || c.bottom != nCurConHeight);
		}

		if (gpSetCls->isAdvLogging)
		{
			char szInfo[160]; wsprintfA(szInfo, "OnConsoleResize: lbSizeChanged=%i, client={{%i,%i},{%i,%i}}, CalcCon={%i,%i}, CurCon={%i,%i}",
			                            lbSizeChanged, client.left, client.top, client.right, client.bottom,
			                            c.right, c.bottom, nCurConWidth, nCurConHeight);
			mp_VActive->RCon()->LogString(szInfo);
		}

		if (!isSizing() &&
		        (lbSizingToDo /*после реального ресайза мышкой*/ ||
		         gbPostUpdateWindowSize /*после появления/скрытия табов*/ ||
		         lbSizeChanged /*или размер в виртуальной консоли не совпадает с расчетным*/))
		{
			gbPostUpdateWindowSize = false;

			if (isNtvdm())
			{
				SyncNtvdm();
			}
			else
			{
				if (!mb_isFullScreen && !isZoomed() && !lbSizingToDo)
					SyncWindowToConsole();
				else
					SyncConsoleToWindow();

				OnSize(0, client.right, client.bottom);
			}

			//_ASSERTE(mp_VActive!=NULL);
			if (mp_VActive)
			{
				m_LastConSize = MakeCoord(mp_VActive->TextWidth,mp_VActive->TextHeight);
			}

			// Запомнить "идеальный" размер окна, выбранный пользователем
			if (lbSizingToDo)
				UpdateIdealRect();

			//if (lbSizingToDo && !mb_isFullScreen && !isZoomed() && !isIconic()) {
			//	GetWindowRect(ghWnd, &mrc_Ideal);
			//}
		}
		else if (mp_VActive
		        && (m_LastConSize.X != (int)mp_VActive->TextWidth
		            || m_LastConSize.Y != (int)mp_VActive->TextHeight))
		{
			// По идее, сюда мы попадаем только для 16-бит приложений
			if (isNtvdm())
				SyncNtvdm();

			m_LastConSize = MakeCoord(mp_VActive->TextWidth,mp_VActive->TextHeight);
		}
	}
}

//bool CConEmuMain::CorrectWindowPos(WINDOWPOS *wp)
//{
//	bool lbChanged = false;
//
//	// wp->flags != (SWP_NOMOVE|SWP_NOSIZE)
//	if ((gpSet->isHideCaption || gpSet->isHideCaptionAlways()) && ((wp->flags&3)!=3) && isZoomed())
//	{
//		RECT rcShift = CalcMargins(CEM_FRAME);
//		MONITORINFO mi = {sizeof(mi)};
//		HMONITOR hMon = MonitorFromWindow(ghWnd, MONITOR_DEFAULTTONEAREST);
//
//		if ((wp->x != (-rcShift.left+mi.rcWork.left)) || (wp->y != (-rcShift.top+mi.rcWork.top)))
//		{
//			wp->x = (-rcShift.left+mi.rcWork.left);
//			wp->y = (-rcShift.top+mi.rcWork.top);
//			TODO("Наверное и окошко подвинуть нада?");
//			lbChanged = true;
//		}
//
//		//ptFullScreenSize.x,ptFullScreenSize.y,
//	}
//	return lbChanged;
//}

LRESULT CConEmuMain::OnSize(WPARAM wParam, WORD newClientWidth, WORD newClientHeight)
{
	LRESULT result = 0;
#ifdef _DEBUG
	RECT rcDbgSize; GetWindowRect(ghWnd, &rcDbgSize);
	wchar_t szSize[255]; _wsprintf(szSize, SKIPLEN(countof(szSize)) L"OnSize(%i, %ix%i) Current window size (X=%i, Y=%i, W=%i, H=%i)\n",
	                               (DWORD)wParam, (int)(short)newClientWidth, (int)(short)newClientHeight,
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
		PostMessage(ghWnd, WM_SIZE, wParam, MAKELONG(newClientWidth,newClientHeight));
		return 0;
	}

#ifdef _DEBUG
	DWORD_PTR dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE);
#endif
	//if (mb_InResize) {
	//	_ASSERTE(!mb_InResize);
	//	PostMessage(ghWnd, WM_SIZE, wParam, MAKELONG(newClientWidth,newClientHeight));
	//	return 0;
	//}
	mn_InResize++;

	CVirtualConsole* pVCon = mp_VActive;
	CVConGuard guard(pVCon);

	if (newClientWidth==(WORD)-1 || newClientHeight==(WORD)-1)
	{
		RECT rcClient = GetGuiClientRect();
		newClientWidth = rcClient.right;
		newClientHeight = rcClient.bottom;
	}

	// Запомнить "идеальный" размер окна, выбранный пользователем
	if (isSizing() && !mb_isFullScreen && !isZoomed() && !isIconic())
	{
		GetWindowRect(ghWnd, &mrc_Ideal);
	}

	if (gpConEmu->mp_TabBar->IsTabsActive())
		gpConEmu->mp_TabBar->UpdateWidth();

	// Background - должен занять все клиентское место под тулбаром
	// Там же ресайзится ScrollBar
	//ResizeChildren();

	BOOL lbIsPicView = isPictureView();		UNREFERENCED_PARAMETER(lbIsPicView);

	if (wParam != (DWORD)-1 && change2WindowMode == (DWORD)-1 && mn_InResize <= 1)
	{
		SyncConsoleToWindow();
	}

	RECT mainClient = MakeRect(newClientWidth,newClientHeight);
	RECT dcSize = CalcRect(CER_DC, mainClient, CER_MAINCLIENT);
	RECT client = CalcRect(CER_DC, mainClient, CER_MAINCLIENT, NULL, &dcSize);
	WARNING("Вынести в CalcRect");
	RECT rcNewCon = {};

	if (gpSet->isAlwaysShowScrollbar == 1)
		client.right += GetSystemMetrics(SM_CXVSCROLL);

	if (pVCon && pVCon->Width && pVCon->Height)
	{
		if (pVCon->GuiWnd() && pVCon->RCon()->isGuiOverCon())
		{
			// Если работает в режиме "GUI во вкладке" - занять всю доступную область
			rcNewCon = dcSize;
		}
		else
		{
			// Иначе - "консольную" область возможно придется отцентрировать (по настройке)
			if ((gpSet->isTryToCenter && (isZoomed() || mb_isFullScreen))
					|| isNtvdm())
			{
				rcNewCon.left = (client.right + client.left - (int)pVCon->Width)/2;
				rcNewCon.top = (client.bottom + client.top - (int)pVCon->Height)/2;
			}

			if (rcNewCon.left<client.left) rcNewCon.left=client.left;

			if (rcNewCon.top<client.top) rcNewCon.top=client.top;

			rcNewCon.right = rcNewCon.left + pVCon->Width + ((gpSet->isAlwaysShowScrollbar == 1) ? GetSystemMetrics(SM_CXVSCROLL) : 0);
			rcNewCon.bottom = rcNewCon.top + pVCon->Height;

			if (rcNewCon.right>client.right) rcNewCon.right=client.right;

			if (rcNewCon.bottom>client.bottom) rcNewCon.bottom=client.bottom;
		}
	}
	else
	{
		rcNewCon = client;
	}

	bool lbPosChanged = false;
	RECT rcCurCon = {};

	WARNING("DoubleView и не только: Переделать. Ресайз должен жить в ConEmuChild/VConGroup!");
	for (size_t i = 0; i < countof(mp_VCon); i++)
	{
		if (mp_VCon[i] && (mp_VCon[i] != pVCon))
		{
			HWND hWndDC = mp_VCon[i]->GetView();
			if (hWndDC)
			{
				WARNING("DoubleView и не только: Переделать. Ресайз должен жить в ConEmuChild!");
				GetWindowRect(hWndDC, &rcCurCon);
				MapWindowPoints(NULL, ghWnd, (LPPOINT)&rcCurCon, 2);
				// Тут нас интересует только X/Y
				lbPosChanged = memcmp(&rcCurCon, &rcNewCon, sizeof(POINT))!=0;

				if (lbPosChanged)
				{
					// Двигаем окошко DC
					SetWindowPos(hWndDC, NULL, rcNewCon.left, rcNewCon.top, 0,0, SWP_NOSIZE|SWP_NOZORDER);
				}
			}
		}
	}

	HWND hWndDC = pVCon ? pVCon->GetView() : NULL;
	if (hWndDC)
	{
		WARNING("DoubleView и не только: Переделать. Ресайз должен жить в ConEmuChild!");
		GetWindowRect(hWndDC, &rcCurCon);
		MapWindowPoints(NULL, ghWnd, (LPPOINT)&rcCurCon, 2);
		lbPosChanged = memcmp(&rcCurCon, &rcNewCon, sizeof(RECT))!=0;

		if (lbPosChanged)
		{
			// Двигаем/ресайзим окошко DC
			MoveWindow(hWndDC, rcNewCon.left, rcNewCon.top, rcNewCon.right - rcNewCon.left, rcNewCon.bottom - rcNewCon.top, 1);
			pVCon->Invalidate();
		}
	}

	if (mn_InResize>0)
		mn_InResize--;

	#ifdef _DEBUG
	dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE);
	#endif
	return result;
}

LRESULT CConEmuMain::OnSizing(WPARAM wParam, LPARAM lParam)
{
	LRESULT result = true;
#if defined(EXT_GNUC_LOG)
	char szDbg[255];
	wsprintfA(szDbg, "CConEmuMain::OnSizing(wParam=%i, L.Lo=%i, L.Hi=%i)\n",
	          wParam, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));

	if (gpSetCls->isAdvLogging)
		mp_VActive->RCon()->LogString(szDbg);

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
		else if (!mb_isFullScreen && !isZoomed())
		{
			RECT srctWindow;
			RECT wndSizeRect, restrictRect, calcRect;
			RECT *pRect = (RECT*)lParam; // с рамкой
			RECT rcCurrent; GetWindowRect(ghWnd, &rcCurrent);

			if ((mouse.state & (MOUSE_SIZING_BEGIN|MOUSE_SIZING_TODO))==MOUSE_SIZING_BEGIN
			        && isPressed(VK_LBUTTON))
			{
				mouse.state |= MOUSE_SIZING_TODO;
			}

			wndSizeRect = *pRect;
			// Для красивости рамки под мышкой
			LONG nWidth = gpSetCls->FontWidth(), nHeight = gpSetCls->FontHeight();

			if (nWidth && nHeight)
			{
				wndSizeRect.right += (nWidth-1)/2;
				wndSizeRect.bottom += (nHeight-1)/2;
			}

			// Рассчитать желаемый размер консоли
			//srctWindow = ConsoleSizeFromWindow(&wndSizeRect, true /* frameIncluded */);
			AutoSizeFont(wndSizeRect, CER_MAIN);
			srctWindow = CalcRect(CER_CONSOLE, wndSizeRect, CER_MAIN);

			// Минимально допустимые размеры консоли
			if (srctWindow.right<28) srctWindow.right=28;

			if (srctWindow.bottom<9)  srctWindow.bottom=9;

			/*if ((srctWindowLast.X != srctWindow.X
			    || srctWindowLast.Y != srctWindow.Y)
			    && !mb_FullWindowDrag)
			{
			    SetConsoleWindowSize(srctWindow, true);
			    srctWindowLast = srctWindow;
			}*/
			//RECT consoleRect = ConsoleOffsetRect();
			//wndSizeRect = WindowSizeFromConsole(srctWindow, true /* rectInWindow */);
			calcRect = CalcRect(CER_MAIN, srctWindow, CER_CONSOLE);
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

	return result;
}

LRESULT CConEmuMain::OnWindowPosChanged(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0; // DefWindowProc зовется в середине функции

	// Если нужно поправить параметры DWM
	gpConEmu->ExtendWindowFrame();

	//static int WindowPosStackCount = 0;
	WINDOWPOS *p = (WINDOWPOS*)lParam;
	DWORD dwStyle = GetWindowLong(ghWnd, GWL_STYLE);

	#ifdef _DEBUG
	static int cx, cy;

	if (!(p->flags & SWP_NOSIZE) && (cx != p->cx || cy != p->cy))
	{
		cx = p->cx; cy = p->cy;
	}
	#endif

	wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_WINDOWPOSCHANGED ({%i-%i}x{%i-%i} Flags=0x%08X), style=0x%08X\n", p->x, p->y, p->cx, p->cy, p->flags, dwStyle);
	DEBUGSTRSIZE(szDbg);
	//WindowPosStackCount++;

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

	// Иначе могут не вызваться события WM_SIZE/WM_MOVE
	result = DefWindowProc(hWnd, uMsg, wParam, lParam);

	//WindowPosStackCount--;

	if (hWnd == ghWnd /*&& ghOpWnd*/)  //2009-05-08 запоминать wndX/wndY всегда, а не только если окно настроек открыто
	{
		if (!gpConEmu->mb_IgnoreSizeChange && !gpConEmu->isFullScreen() && !gpConEmu->isZoomed() && !gpConEmu->isIconic())
		{
			RECT rc; GetWindowRect(ghWnd, &rc);
			//gpSet->UpdatePos(rc.left, rc.top);
			gpConEmu->StoreNormalRect(&rc);

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
	}
	else if (gpConEmu->hPictureView)
	{
		GetWindowRect(ghWnd, &gpConEmu->mrc_WndPosOnPicView);
	}

	// Если окошко сворачивалось кнопками Win+Down (Win7) то SC_MINIMIZE не приходит
	if ((gpSet->isMinToTray || gpConEmu->ForceMinimizeToTray) && isIconic() && IsWindowVisible(ghWnd))
	{
		Icon.HideWindowToTray();
	}

	return result;
}

LRESULT CConEmuMain::OnWindowPosChanging(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	WINDOWPOS *p = (WINDOWPOS*)lParam;

	#ifdef _DEBUG
	DWORD dwStyle = GetWindowLong(ghWnd, GWL_STYLE);
	wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_WINDOWPOSCHANGING ({%i-%i}x{%i-%i} Flags=0x%08X) style=0x%08X\n", p->x, p->y, p->cx, p->cy, p->flags, dwStyle);
	DEBUGSTRSIZE(szDbg);
	static int cx, cy;

	if (!(p->flags & SWP_NOSIZE) && (cx != p->cx || cy != p->cy))
	{
		cx = p->cx; cy = p->cy;
	}
	#endif
	
	// Если у нас режим скрытия заголовка (при максимизации)
	if (!(p->flags & (SWP_NOSIZE|SWP_NOMOVE)) && (change2WindowMode == (DWORD)-1) && mb_MaximizedHideCaption && isZoomed())
	{
		// Нужно скорректировать размеры, а то при смене разрешения монитора (в частности при повороте экрана) глюки лезут
		p->flags |= (SWP_NOSIZE|SWP_NOMOVE);
		// И обновить размер насильно
		SetWindowMode(rMaximized, TRUE);
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
	//			SetWindowLong(ghWnd, GWL_STYLE, dwStyle);
	//			gpConEmu->InvalidateAll();
	//		}
	//		break;
	//	}
	//}

	if (!(p->flags & SWP_NOSIZE)
	        && (hWnd == ghWnd) && !gpConEmu->mb_IgnoreSizeChange
	        && !mb_isFullScreen && !isZoomed() && !isIconic())
	{
		if (!hPictureView)
		{
			TODO("Доработать, когда будет ресайз PicView на лету");
			RECT rcWnd = {0,0,p->cx,p->cy};
			CVirtualConsole* pVCon = ActiveCon();

			if (pVCon && pVCon->RCon())
				pVCon->RCon()->SyncConsole2Window(FALSE, &rcWnd);
		}
	}

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

	return result;
}

void CConEmuMain::OnSizePanels(COORD cr)
{
	INPUT_RECORD r;
	int nRepeat = 0;
	wchar_t szKey[32];
	bool bShifted = (mouse.state & MOUSE_DRAGPANEL_SHIFT) && isPressed(VK_SHIFT);
	CRealConsole* pRCon = mp_VActive->RCon();

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

				if (nRepeat > 1)
					wsprintf(szMacro+1, L"$Rep (%i) %s $End", nRepeat, szKey);
				else
					wcscpy(szMacro+1, szKey);

				PostMacro(szMacro);
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

BOOL CConEmuMain::Activate(CVirtualConsole* apVCon)
{
	if (!isValid(apVCon))
		return FALSE;

	BOOL lbRc = FALSE;

	for (size_t i = 0; i < countof(mp_VCon); i++)
	{
		if (mp_VCon[i] == apVCon)
		{
			ConActivate(i);
			lbRc = (mp_VActive == apVCon);
			break;
		}
	}

	return lbRc;
}

CVirtualConsole* CConEmuMain::ActiveCon()
{
	return mp_VActive;
	/*if (mn_ActiveCon >= countof(mp_VCon))
	    mn_ActiveCon = -1;
	if (mn_ActiveCon < 0)
	    return NULL;
	return mp_VCon[mn_ActiveCon];*/
}

// 0 - based
int CConEmuMain::ActiveConNum()
{
	int nActive = -1;

	for (size_t i = 0; i < countof(mp_VCon); i++)
	{
		if (mp_VCon[i] == mp_VActive)
		{
			nActive = i; break;
		}
	}

	return nActive;
}

int CConEmuMain::GetConCount()
{
	int nCount = 0;

	for (size_t i = 0; i < countof(mp_VCon); i++)
	{
		if (mp_VCon[i])
			nCount++;
		else
			break;
	}

	return nCount;
}

BOOL CConEmuMain::AttachRequested(HWND ahConWnd, const CESERVER_REQ_STARTSTOP* pStartStop, CESERVER_REQ_STARTSTOPRET* pRet)
{
	CVirtualConsole* pVCon = NULL;
	CRealConsole* pRCon = NULL;
	_ASSERTE(pStartStop->dwPID!=0);

	// Может быть какой-то VCon ждет аттача?
	if (!pVCon)
	{
		for (size_t i = 0; i < countof(mp_VCon); i++)
		{
			if (mp_VCon[i] && (pRCon = mp_VCon[i]->RCon()) != NULL)
			{
				if (pRCon->GetServerPID() == pStartStop->dwPID)
				{
					//_ASSERTE(pRCon->GetServerPID() != pStartStop.dwPID);
					pVCon = mp_VCon[i];
					break;
				}
			}
		}
	}

	if (!pVCon)
	{
		for (size_t i = 0; i < countof(mp_VCon); i++)
		{
			if (mp_VCon[i] && (pRCon = mp_VCon[i]->RCon()) != NULL)
			{
				if (pRCon->isDetached())
				{
					pVCon = mp_VCon[i];
					break;
				}
			}
		}
	}

	// Если не нашли - определим, можно ли добавить новую консоль?
	if (!pVCon)
	{
		RConStartArgs* pArgs = new RConStartArgs;
		pArgs->bDetached = TRUE;
		pArgs->bBackgroundTab = pStartStop->bRunInBackgroundTab;

		// т.к. это приходит из серверного потока - зовем в главном
		pVCon = (CVirtualConsole*)SendMessage(ghWnd, mn_MsgCreateCon, mn_MsgCreateCon, (LPARAM)pArgs);
		if (pVCon && !isValid(pVCon))
		{
			_ASSERTE(isValid(pVCon));
			pVCon = NULL;
		}
		//if ((pVCon = CreateCon(&args)) == NULL)
		//	return FALSE;
	}

	// Пытаемся подцепить консоль
	if (!pVCon->RCon()->AttachConemuC(ahConWnd, pStartStop->dwPID, pStartStop, pRet))
		return FALSE;

	// OK
	return TRUE;
}

CRealConsole* CConEmuMain::AttachRequestedGui(LPCWSTR asAppFileName, DWORD anAppPID)
{
	CRealConsole* pRCon;

	for (size_t i = 0; i < countof(mp_VCon); i++)
	{
		if (mp_VCon[i] && (pRCon = mp_VCon[i]->RCon()) != NULL)
		{
			if (pRCon->GuiAppAttachAllowed(asAppFileName, anAppPID))
				return pRCon;
		}
	}
	
	return NULL;
}

// Вернуть общее количество процессов по всем консолям
DWORD CConEmuMain::CheckProcesses()
{
	DWORD dwAllCount = 0;

	//mn_ActiveStatus &= ~CES_PROGRAMS;
	for (size_t j = 0; j < countof(mp_VCon); j++)
	{
		if (mp_VCon[j])
		{
			int nCount = mp_VCon[j]->RCon()->GetProcesses(NULL);
			if (nCount)
				dwAllCount += nCount;
		}
	}

	//if (mp_VActive) {
	//    mn_ActiveStatus |= mp_VActive->RCon()->GetProgramStatus();
	//}
	m_ProcCount = dwAllCount;
	return dwAllCount;
}

bool CConEmuMain::ConActivateNext(BOOL abNext)
{
	int nActive = ActiveConNum(), i, j, n1, n2, n3;

	for (j = 0; j <= 1; j++)
	{
		if (abNext)
		{
			if (j == 0)
			{
				n1 = nActive+1; n2 = countof(mp_VCon); n3 = 1;
			}
			else
			{
				n1 = 0; n2 = nActive; n3 = 1;
			}

			if (n1>=n2) continue;
		}
		else
		{
			if (j == 0)
			{
				n1 = nActive-1; n2 = -1; n3 = -1;
			}
			else
			{
				n1 = countof(mp_VCon)-1; n2 = nActive; n3 = -1;
			}

			if (n1<=n2) continue;
		}

		for (i = n1; i != n2 && i >= 0 && i < (int)countof(mp_VCon); i+=n3)
		{
			if (mp_VCon[i])
			{
				return ConActivate(i);
			}
		}
	}

	return false;
}

// nCon - zero-based index of console
bool CConEmuMain::ConActivate(int nCon)
{
	FLASHWINFO fl = {sizeof(FLASHWINFO)}; fl.dwFlags = FLASHW_STOP; fl.hwnd = ghWnd;
	FlashWindowEx(&fl); // При многократных созданиях мигать начинает...

	if (nCon >= 0 && nCon < (int)countof(mp_VCon))
	{
		CVirtualConsole* pVCon = mp_VCon[nCon];

		if (pVCon == NULL)
		{
			if (gpSet->isMultiAutoCreate)
			{
				// Создать новую default-консоль
				Recreate(FALSE, FALSE, FALSE);
				return true; // создана новая консоль
			}

			return false; // консоль с этим номером не была создана!
		}

		if (pVCon == mp_VActive)
		{
			// Итерация табов
			int nTabCount;
			CRealConsole *pRCon;

			// При последовательном нажатии "Win+<Number>" - крутить табы активной консоли
			if (gpSet->isMultiIterate
			        && ((pRCon = mp_VActive->RCon()) != NULL)
			        && ((nTabCount = pRCon->GetTabCount())>1))
			{
				int nActive = pRCon->GetActiveTab()+1;

				if (nActive >= nTabCount)
					nActive = 0;

				if (pRCon->CanActivateFarWindow(nActive))
					pRCon->ActivateFarWindow(nActive);
			}

			return true; // уже
		}

		bool lbSizeOK = true;
		int nOldConNum = ActiveConNum();

		CVirtualConsole* pOldActive = mp_VActive;

		// Спрятать PictureView, или еще чего...
		if (mp_VActive && mp_VActive->RCon())
		{
			mp_VActive->RCon()->OnDeactivate(nCon);
		}

		// ПЕРЕД переключением на новую консоль - обновить ее размеры
		if (mp_VActive)
		{
			int nOldConWidth = mp_VActive->RCon()->TextWidth();
			int nOldConHeight = mp_VActive->RCon()->TextHeight();
			int nNewConWidth = pVCon->RCon()->TextWidth();
			int nNewConHeight = pVCon->RCon()->TextHeight();

			if (nOldConWidth != nNewConWidth || nOldConHeight != nNewConHeight)
			{
				lbSizeOK = pVCon->RCon()->SetConsoleSize(nOldConWidth,nOldConHeight);
			}
		}

		mp_VActive = pVCon;
		pVCon->RCon()->OnActivate(nCon, nOldConNum);

		if (!lbSizeOK)
			SyncWindowToConsole();

		// Теперь можно показать активную
		//ShowWindow(mp_VActive->GetView(), SW_SHOW);
		mp_VActive->ShowView(SW_SHOW);
		// и спрятать деактивированную
		if (pOldActive && (pOldActive != mp_VActive) && !pOldActive->isVisible())
			pOldActive->ShowView(SW_HIDE);
			//ShowWindow(pOldActive->GetView(), SW_HIDE);

		Invalidate(mp_VActive);
	}

	return false;
}

// args должен быть выделен через "new"
// по завершении - на него будет вызван "delete"
void CConEmuMain::PostCreateCon(RConStartArgs *pArgs)
{
	_ASSERTE((pArgs->pszStartupDir == NULL) || (*pArgs->pszStartupDir != 0));
	PostMessage(ghWnd, mn_MsgCreateCon, mn_MsgCreateCon+1, (LPARAM)pArgs);
}

CVirtualConsole* CConEmuMain::CreateCon(RConStartArgs *args, BOOL abAllowScripts /*= FALSE*/)
{
	_ASSERTE(args!=NULL);
	if (!gpConEmu->isMainThread())
	{
		// Создание VCon в фоновых потоках не допускается, т.к. здесь создаются HWND
		MBoxAssert(gpConEmu->isMainThread());
		return NULL;
	}

	CVirtualConsole* pVCon = NULL;

	if (args->pszSpecialCmd)
		args->ProcessNewConArg();

	wchar_t* pszScript = NULL; //, szScript[MAX_PATH];

	if (args->pszSpecialCmd && (*args->pszSpecialCmd == CmdFilePrefix || *args->pszSpecialCmd == TaskBracketLeft))
	{
		if (!abAllowScripts)
		{
			DisplayLastError(L"Console script are not supported here!", -1);
			return NULL;
		}

		// В качестве "команды" указан "пакетный файл" или "группа команд" одновременного запуска нескольких консолей
		wchar_t* pszDataW = LoadConsoleBatch(args->pszSpecialCmd);
		if (!pszDataW)
			return NULL;

		// GO
		pVCon = CreateConGroup(pszDataW, args->bRunAsAdministrator);

		SafeFree(pszDataW);
		return pVCon;
	}

	for (size_t i = 0; i < countof(mp_VCon); i++)
	{
		if (mp_VCon[i] && mp_VCon[i]->RCon() && mp_VCon[i]->RCon()->isDetached())
		{
			// isDetached() means, that ConEmu.exe was started with "/detached" flag
			// so, it is safe to close "dummy" console, that was created on GUI startup
			mp_VCon[i]->RCon()->CloseConsole(false, false);
		}

		if (!mp_VCon[i])
		{
			CVirtualConsole* pOldActive = mp_VActive;
			mb_CreatingActive = true;
			pVCon = CVirtualConsole::CreateVCon(args);
			mb_CreatingActive = false;

			BOOL lbInBackground = args->bBackgroundTab && (pOldActive != NULL);

			if (pVCon)
			{
				if (!lbInBackground && pOldActive && pOldActive->RCon())
				{
					pOldActive->RCon()->OnDeactivate(i);
				}

				mp_VCon[i] = pVCon;
				
				if (!lbInBackground)
				{
					mp_VActive = pVCon;
				}
				else
				{
					_ASSERTE(mp_VActive==pOldActive);
				}
				
				pVCon->InitGhost();

				if (!lbInBackground)
				{
					pVCon->RCon()->OnActivate(i, ActiveConNum());

					//mn_ActiveCon = i;
					//Update(true);

					TODO("DoubleView: показать на неактивной?");
					// Теперь можно показать активную
					mp_VActive->ShowView(SW_SHOW);
					//ShowWindow(mp_VActive->GetView(), SW_SHOW);
					// и спрятать деактивированную
					if (pOldActive && (pOldActive != mp_VActive) && !pOldActive->isVisible())
						pOldActive->ShowView(SW_HIDE);
						//ShowWindow(pOldActive->GetView(), SW_HIDE);
				}
			}

			break;
		}
	}

	return pVCon;
}

// Возвращает указатель на АКТИВНУЮ консоль (при создании группы)
// apszScript содержит строки команд, разделенные \r\n
CVirtualConsole* CConEmuMain::CreateConGroup(LPCWSTR apszScript, BOOL abForceAsAdmin /*= FALSE*/)
{
	CVirtualConsole* pVConResult = NULL;
	// Поехали
	wchar_t *pszDataW = lstrdup(apszScript);
	wchar_t *pszLine = pszDataW;
	wchar_t *pszNewLine = wcschr(pszLine, L'\n');
	CVirtualConsole *pSetActive = NULL, *pVCon = NULL, *pLastVCon = NULL;
	BOOL lbSetActive = FALSE, lbOneCreated = FALSE, lbRunAdmin = FALSE;

	while (*pszLine)
	{
		lbSetActive = FALSE;
		lbRunAdmin = abForceAsAdmin;

		while (*pszLine == L'>' || *pszLine == L'*' || *pszLine == L' ' || *pszLine == L'\t')
		{
			if (*pszLine == L'>') lbSetActive = TRUE;

			if (*pszLine == L'*') lbRunAdmin = TRUE;

			pszLine++;
		}

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
				args.pszSpecialCmd = lstrdup(pszLine);
				args.bRunAsAdministrator = lbRunAdmin;
				pVCon = CreateCon(&args);

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

					pLastVCon = pVCon;
					if (lbSetActive && !pSetActive)
						pSetActive = pVCon;

					if (GetVCon((int)countof(mp_VCon)-1))
						break; // Больше создать не получится
				}
			}

			// При создании группы консолей требуется обрабатывать сообщения,
			// иначе может возникнуть блокировка запускаемого сервера
			MSG Msg;
			while (PeekMessage(&Msg,0,0,0,PM_REMOVE))
			{
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
	return pVConResult;
}

void CConEmuMain::CreateGhostVCon(CVirtualConsole* apVCon)
{
	PostMessage(ghWnd, mn_MsgInitVConGhost, 0, (LPARAM)apVCon);
}

void CConEmuMain::UpdateActiveGhost(CVirtualConsole* apVCon)
{
	_ASSERTE(apVCon == mp_VActive);
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
	for (size_t i = 0; i < countof(mp_VCon); i++)
	{
		if (mp_VCon[i] == NULL) continue;

		CRealConsole* pRCon = mp_VCon[i]->RCon();

		if (pRCon)
			pRCon->UpdateFarSettings();

		//DWORD dwFarPID = pRCon->GetFarPID();
		//if (!dwFarPID) continue;
		//pRCon->EnableComSpec(dwFarPID, gpSet->AutoBufferHeight);
	}
}

void CConEmuMain::UpdateIdealRect(BOOL abAllowUseConSize/*=FALSE*/)
{
	// Запомнить "идеальный" размер окна, выбранный пользователем
	if (!mb_isFullScreen && !isZoomed() && !isIconic())
	{
		GetWindowRect(ghWnd, &mrc_Ideal);
	}
	else if (abAllowUseConSize)
	{
		CRealConsole* pRCon = mp_VActive->RCon();

		if (pRCon)
		{
			RECT rcCon = MakeRect(pRCon->TextWidth(),pRCon->TextHeight());
			RECT rcWnd = CalcRect(CER_MAIN, rcCon, CER_CONSOLE);
			mrc_Ideal = rcWnd;
		}
	}
}

void CConEmuMain::DebugStep(LPCWSTR asMsg, BOOL abErrorSeverity/*=FALSE*/)
{
	if (ghWnd)
	{
		static bool bWasDbgStep, bWasDbgError;

		if (asMsg && *asMsg)
		{
			// Если ведется ЛОГ - выбросить в него
			mp_VActive->RCon()->LogString(asMsg);

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
	return ms_ConEmuVer;
}

LRESULT CConEmuMain::GuiShellExecuteEx(SHELLEXECUTEINFO* lpShellExecute, BOOL abAllowAsync)
{
	LRESULT lRc = 0;

	if (!isMainThread())
	{
		if (abAllowAsync)
			lRc = PostMessage(ghWnd, mn_ShellExecuteEx, abAllowAsync, (LPARAM)lpShellExecute);
		else
			lRc = SendMessage(ghWnd, mn_ShellExecuteEx, abAllowAsync, (LPARAM)lpShellExecute);
	}
	else
	{
		/*if (IsDebuggerPresent()) { -- не требуется. был баг с памятью
			BOOL b = gbDontEnable; gbDontEnable = TRUE;
			int nBtn = MessageBox(ghWnd, L"Debugger active!\nShellExecuteEx(runas) my fails, when VC IDE\ncatches Microsoft C++ exceptions.\nContinue?", GetDefaultTitle(), MB_ICONASTERISK|MB_YESNO|MB_DEFBUTTON2);
			gbDontEnable = b;
			if (nBtn != IDYES)
				return (FALSE);
		}*/
		lRc = ::ShellExecuteEx(lpShellExecute);

		//120429 - если мы были в Recreate - то наверное не закрывать, пусть болванка висит?
		if (abAllowAsync && (lRc == 0) && !mp_VActive->RCon()->InRecreate())
		{
			mp_VActive->RCon()->CloseConsole(false, false);
		}
	}

	return lRc;
}

//BOOL CConEmuMain::HandlerRoutine(DWORD dwCtrlType)
//{
//    return (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT ? true : false);
//}

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

void CConEmuMain::PostCopy(wchar_t* apszMacro, BOOL abRecieved/*=FALSE*/)
{
	if (!abRecieved)
	{
		PostMessage(ghWnd, mn_MsgPostCopy, 0, (LPARAM)apszMacro);
	}
	else
	{
		PostMacro(apszMacro);
		free(apszMacro);
	}
}

void CConEmuMain::PostMacro(LPCWSTR asMacro)
{
	if (!asMacro || !*asMacro)
		return;

	mp_VActive->RCon()->PostMacro(asMacro);
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

void CConEmuMain::PostDisplayRConError(CRealConsole* mp_RCon, wchar_t* pszErrMsg)
{
	PostMessage(ghWnd, mn_MsgDisplayRConError, (WPARAM)mp_VCon, (LPARAM)pszErrMsg);
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
	if (abRegister)
	{
		//if (!gpSet->vmMinimizeRestore)
		if (!gpSet->IsHotkey(vkMinimizeRestore))
			return; // не просили

		UINT nMOD = gpSet->GetHotKeyMod(gpSet->GetHotkeyById(vkMinimizeRestore));

		if (mn_MinRestoreRegistered
		        && (mn_MinRestore_VK != gpSet->GetHotkeyById(vkMinimizeRestore) || nMOD != mn_MinRestore_MOD))
		{
			UnregisterHotKey(ghWnd, mn_MinRestoreRegistered);
			mn_MinRestoreRegistered = 0;
		}

		if (!mn_MinRestoreRegistered)
		{
			UINT vk = gpSet->GetHotkey(gpSet->GetHotkeyById(vkMinimizeRestore));
			if (RegisterHotKey(ghWnd, HOTKEY_MINRESTORE_ID, nMOD, vk))
			{
				mn_MinRestoreRegistered = HOTKEY_MINRESTORE_ID;
				mn_MinRestore_VK = vk;
				mn_MinRestore_MOD = nMOD;
			}
			else if (isFirstInstance())
			{
				// -- При одновременном запуске двух копий - велики шансы, что они подерутся
				// -- наверное вообще не будем показывать ошибку
				// -- кроме того, isFirstInstance() не работает, если копия ConEmu.exe запущена под другим юзером
#ifdef _DEBUG
				wchar_t szErr[128]; DWORD dwErr = GetLastError();
				_wsprintf(szErr, SKIPLEN(countof(szErr)) L"Can't register Minimize/Restore hotkey, ErrCode=0x%08X", dwErr);
				//MBoxA(szErr);
#endif
			}
		}
	}
	else
	{
		if (mn_MinRestoreRegistered)
		{
			UnregisterHotKey(ghWnd, mn_MinRestoreRegistered);
			mn_MinRestoreRegistered = 0;
		}
	}
}

void CConEmuMain::RegisterHotKeys()
{
	if (isIconic())
	{
		UnRegisterHotKeys();
		return;
	}

	if (!mb_HotKeyRegistered)
	{
		if (RegisterHotKey(ghWnd, HOTKEY_CTRLWINALTSPACE_ID, MOD_CONTROL|MOD_WIN|MOD_ALT, VK_SPACE))
		{
			mb_HotKeyRegistered = TRUE;
		}
	}

	if (!mh_LLKeyHook)
	{
		RegisterHoooks();
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

		UpdateActiveGhost(mp_VActive);
	}
}

void CConEmuMain::RegisterHoooks()
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
		UnRegisterHoooks();
		return;
	}

	DWORD dwErr = 0;

	if (!mh_LLKeyHook)
	{
		// Проверяет, разрешил ли пользователь установку хуков.
		if (gpSet->isKeyboardHooks())
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
						_ASSERTE(mh_LLKeyHook!=NULL);
					}
					else
					{
						if (pKeyHook) *pKeyHook = mh_LLKeyHook;
					}
				}
			}
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
	if (mb_HotKeyRegistered)
	{
		UnregisterHotKey(ghWnd, HOTKEY_CTRLWINALTSPACE_ID);
		mb_HotKeyRegistered = FALSE;
	}

	UnRegisterHoooks(abFinal);
}

void CConEmuMain::UnRegisterHoooks(BOOL abFinal/*=FALSE*/)
{
	if (mh_LLKeyHook)
	{
		UnhookWindowsHookEx(mh_LLKeyHook);
		mh_LLKeyHook = NULL;
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

void CConEmuMain::CtrlWinAltSpace()
{
	if (mp_VActive && mp_VActive->RCon())
		mp_VActive->RCon()->CtrlWinAltSpace(); // Toggle visibility
}

// abRecreate: TRUE - пересоздать текущую, FALSE - создать новую
// abConfirm:  TRUE - показать диалог подтверждения
// abRunAs:    TRUE - под админом
void CConEmuMain::Recreate(BOOL abRecreate, BOOL abConfirm, BOOL abRunAs)
{
	FLASHWINFO fl = {sizeof(FLASHWINFO)}; fl.dwFlags = FLASHW_STOP; fl.hwnd = ghWnd;
	FlashWindowEx(&fl); // При многократных созданиях мигать начинает...
	RConStartArgs args;
	args.bRecreate = abRecreate;
	args.bRunAsAdministrator = abRunAs;

	WARNING("При переходе на новую обработку кнопок больше не нужно");
	//if (!abConfirm && isPressed(VK_SHIFT))
	//	abConfirm = TRUE;

	if (!abRecreate)
	{
		// Создать новую консоль
		BOOL lbSlotFound = FALSE;

		for (size_t i = 0; i < countof(mp_VCon); i++)
		{
			if (!mp_VCon[i]) { lbSlotFound = TRUE; break; }
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

			//BOOL b = gbDontEnable;
			//gbDontEnable = TRUE;
			//int nRc = DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_RESTART), ghWnd, Recreate DlgProc, (LPARAM)&args);
			//gbDontEnable = b;
			if (nRc != IDC_START)
				return;

			mp_VActive->Redraw();
		}

		//Собственно, запуск
		CreateCon(&args, TRUE);
	}
	else
	{
		// Restart or close console
		int nActive = ActiveConNum();

		if (nActive >=0)
		{
			args.bRunAsAdministrator = abRunAs || mp_VActive->RCon()->isAdministrator();

			if (abConfirm)
			{
				int nRc = RecreateDlg(&args);

				//BOOL b = gbDontEnable;
				//gbDontEnable = TRUE;
				//int nRc = DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_RESTART), ghWnd, Recreate DlgProc, (LPARAM)&args);
				//gbDontEnable = b;
				if (nRc == IDC_TERMINATE)
				{
					mp_VActive->RCon()->CloseConsole(true, false);
					return;
				}

				if (nRc != IDC_START)
					return;
			}

			if (mp_VActive)
			{
				mp_VActive->Redraw();
				// Собственно, Recreate
				mp_VActive->RCon()->RecreateProcess(&args);
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



BOOL CConEmuMain::RunSingleInstance()
{
	BOOL lbAccepted = FALSE;
	LPCWSTR lpszCmd = gpSet->GetCmd();

	if (lpszCmd && *lpszCmd)
	{
		HWND ConEmuHwnd = FindWindowExW(NULL, NULL, VirtualConsoleClassMain, NULL);

		if (ConEmuHwnd)
		{
			CESERVER_REQ *pIn = NULL, *pOut = NULL;
			int nCmdLen = lstrlenW(lpszCmd);
			int nSize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_NEWCMD) + (nCmdLen*sizeof(wchar_t));

			pIn = ExecuteNewCmd(CECMD_NEWCMD, nSize);

			if (pIn)
			{
				GetCurrentDirectory(countof(pIn->NewCmd.szCurDir), pIn->NewCmd.szCurDir);

				lstrcpyW(pIn->NewCmd.szCommand, lpszCmd);
				DWORD dwPID = 0;

				if (GetWindowThreadProcessId(ConEmuHwnd, &dwPID))
					AllowSetForegroundWindow(dwPID);

				pOut = ExecuteGuiCmd(ConEmuHwnd, pIn, NULL);

				if (pOut && pOut->Data[0])
					lbAccepted = TRUE;
			}

			if (pIn) ExecuteFreeResult(pIn);

			if (pOut) ExecuteFreeResult(pOut);
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
	if (h && h != INVALID_HANDLE_VALUE)
	{
		LPCTSTR pszExePath = NULL;
		if (Module32First(h, &mi))
		{
			do {
				if (mi.th32ProcessID == nFromProcess)
				{
					if (!pszExePath)
						pszExePath = mi.szExePath[0] ? mi.szExePath : mi.szModule;

					if (!hFromModule || (mi.hModule == (HMODULE)hFromModule))
					{
						pszCallPath = mi.szExePath[0] ? mi.szExePath : mi.szModule;
						break;
					}
				}
			} while (Module32Next(h, &mi));
			if (!pszCallPath && pszExePath)
				pszCallPath = pszExePath;
		}
		CloseHandle(h);
	}
	if (!pszCallPath)
	{
		h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (h && h != INVALID_HANDLE_VALUE)
		{
			if (Process32First(h, &pi))
			{
				do {
					if (pi.th32ProcessID == nFromProcess)
					{
						pszCallPath = pi.szExeFile;
						break;
					}
				} while (Process32Next(h, &pi));
			}
			CloseHandle(h);
		}
		pszCallPath = _T("");
	}

	lbErrorShowed = true;
	int nMaxLen = 255+_tcslen(pszCallPath);
	wchar_t *pszMsg = (wchar_t*)calloc(nMaxLen,sizeof(wchar_t));
	_wsprintf(pszMsg, SKIPLEN(nMaxLen)
		L"ConEmu received wrong version packet!\nCommandID: %i, Version: %i, ReqVersion: %i, PID: %u\nPlease check %s\n%s",
		nCmd, nVersion, CESERVER_REQ_VER, nFromProcess,
		(bFromServer==1) ? L"ConEmuC*.exe" : (bFromServer==0) ? L"ConEmu*.dll and Far plugins" : L"ConEmuC*.exe and ConEmu*.dll",
		pszCallPath);
	MBox(pszMsg);
	free(pszMsg);
	mb_InShowOldCmdVersion = FALSE; // теперь можно показать еще одно...
}

LRESULT CConEmuMain::OnInitMenuPopup(HWND hWnd, HMENU hMenu, LPARAM lParam)
{
	// Уже должен быть выставлен тип меню, иначе не будут всплывать подсказки для пунктов меню
	_ASSERTE(mn_TrackMenuPlace != tmp_None);

	DefWindowProc(hWnd, WM_INITMENUPOPUP, (WPARAM)hMenu, lParam);

	if (HIWORD(lParam))
	{
		_ASSERTE(mn_TrackMenuPlace == tmp_System);

		BOOL bSelectionExist = FALSE;

		CVirtualConsole* pVCon = ActiveCon();
		if (pVCon && pVCon->RCon())
			bSelectionExist = pVCon->RCon()->isSelectionPresent();

		//EnableMenuItem(hMenu, ID_CON_COPY, MF_BYCOMMAND | (bSelectionExist?MF_ENABLED:MF_GRAYED));
		if (mh_EditPopup)
		{
			TODO("Проверить, сработает ли, если mh_EditPopup уже был вставлен в SystemMenu?");
			CreateEditMenuPopup(pVCon, mh_EditPopup);
		}
		else
		{
			_ASSERTE(mh_EditPopup!=NULL);
		}
		
		if (mh_VConListPopup)
		{
			CreateVConListPopupMenu(mh_VConListPopup, TRUE/*abFirstTabOnly*/);
		}
		else
		{
			_ASSERTE(mh_VConListPopup!=NULL);
		}
		
		if (mh_ActiveVConPopup)
		{
			CreateVConPopupMenu(NULL, mh_ActiveVConPopup, FALSE, mh_TerminateVConPopup);
		}
		else
		{
			_ASSERTE(mh_ActiveVConPopup!=NULL);
		}
		
		
		CheckMenuItem(hMenu, ID_DEBUG_SHOWRECTS, MF_BYCOMMAND|(gbDebugShowRects ? MF_CHECKED : MF_UNCHECKED));
		//#ifdef _DEBUG
		//		wchar_t szText[128];
		//		MENUITEMINFO mi = {sizeof(MENUITEMINFO)};
		//		mi.fMask = MIIM_STRING|MIIM_STATE;
		//		bool bLogged = false, bAllowed = false;
		//		CRealConsole* pRCon = mp_VActive ? mp_VActive->RCon() : NULL;
		//
		//		if (pRCon)
		//		{
		//			bLogged = pRCon->IsLogShellStarted();
		//			bAllowed = (pRCon->GetFarPID(TRUE) != 0);
		//		}
		//
		//		lstrcpy(szText, bLogged ? _T("Disable &shell log") : _T("Enable &shell log..."));
		//		mi.dwTypeData = szText;
		//		mi.fState = bAllowed ? MFS_ENABLED : MFS_GRAYED;
		//		SetMenuItemInfo(hMenu, ID_MONITOR_SHELLACTIVITY, FALSE, &mi);
		//#endif
	}

	return 0;
}

int CConEmuMain::trackPopupMenu(TrackMenuPlace place, HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, RECT *prcRect)
{
	mp_Tip->HideTip();
	mn_TrackMenuPlace = place;

	int cmd = TrackPopupMenu(hMenu, uFlags, x, y, nReserved, hWnd, prcRect);

	mn_TrackMenuPlace = tmp_None;

	mp_Tip->HideTip();

	return cmd;
}

void CConEmuMain::ShowMenuHint(HMENU hMenu, WORD nID, WORD nFlags)
{
	if (nID && (nID != MF_SEPARATOR) && !(nFlags & MF_POPUP))
	{
		//POINT pt; GetCursorPos(&pt);
		RECT rcMenuItem = {};
		BOOL lbMenuItemPos = FALSE;
		UINT nMenuID = 0;
		for (int i = 0; i < 100; i++)
		{
			nMenuID = GetMenuItemID(hMenu, i);
			if (nMenuID == nID)
			{
				lbMenuItemPos = GetMenuItemRect(ghWnd, hMenu, i, &rcMenuItem);
				break;
			}
		}
		if (lbMenuItemPos)
		{
			POINT pt = {rcMenuItem.left + (rcMenuItem.bottom - rcMenuItem.top)*2, rcMenuItem.bottom};
			//pt.x = rcMenuItem.left; //(rcMenuItem.left + rcMenuItem.right) >> 1;
			//pt.y = rcMenuItem.bottom;
			TCHAR szText[0x200];
			if (LoadString(g_hInstance, nMenuID, szText, countof(szText)))
			{
				mp_Tip->ShowTip(ghWnd, ghWnd, szText, TRUE, pt, g_hInstance);
				return;
			}
		}
	}

	mp_Tip->HideTip();
}

void CConEmuMain::ShowKeyBarHint(HMENU hMenu, WORD nID, WORD nFlags)
{
	if (nID && (nID != MF_SEPARATOR) && !(nFlags & MF_POPUP))
	{
		CVirtualConsole* pVCon = ActiveCon();
		if (pVCon && pVCon->RCon())
			pVCon->RCon()->ShowKeyBarHint(nID);
	}
}

void CConEmuMain::ShowSysmenu(int x, int y)
{
	//if (!Wnd)
	//	Wnd = ghWnd;

	if ((x == -32000) || (y == -32000))
	{
		RECT rect, cRect;
		GetWindowRect(ghWnd, &rect);
		cRect = GetGuiClientRect();
		WINDOWINFO wInfo;   GetWindowInfo(ghWnd, &wInfo);
		int nTabShift =
		    ((gpSet->isHideCaptionAlways() || mb_isFullScreen) && gpConEmu->mp_TabBar->IsTabsShown())
		    ? gpConEmu->mp_TabBar->GetTabbarHeight() : 0;

		if (x == -32000)
			x = rect.right - cRect.right - wInfo.cxWindowBorders;

		if (y == -32000)
			y = rect.bottom - cRect.bottom - wInfo.cyWindowBorders + nTabShift;
	}

	bool iconic = isIconic();
	bool zoomed = isZoomed();
	bool visible = IsWindowVisible(ghWnd);
	int style = GetWindowLong(ghWnd, GWL_STYLE);
	HMENU systemMenu = GetSystemMenu();

	if (!systemMenu)
		return;

	EnableMenuItem(systemMenu, SC_RESTORE,
	               MF_BYCOMMAND | ((visible && (iconic || zoomed)) ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(systemMenu, SC_MOVE,
	               MF_BYCOMMAND | ((visible && !(iconic || zoomed)) ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(systemMenu, SC_SIZE,
	               MF_BYCOMMAND | ((visible && (!(iconic || zoomed) && (style & WS_SIZEBOX))) ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(systemMenu, SC_MINIMIZE,
	               MF_BYCOMMAND | ((visible && (!iconic && (style & WS_MINIMIZEBOX))) ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(systemMenu, SC_MAXIMIZE,
	               MF_BYCOMMAND | ((visible && (!zoomed && (style & WS_MAXIMIZEBOX))) ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(systemMenu, ID_TOTRAY, MF_BYCOMMAND | MF_ENABLED);

	mn_TrackMenuPlace = tmp_System;
	SendMessage(ghWnd, WM_INITMENU, (WPARAM)systemMenu, 0);
	SendMessage(ghWnd, WM_INITMENUPOPUP, (WPARAM)systemMenu, MAKELPARAM(0, true));

	// Переехало в OnMenuPopup
	//BOOL bSelectionExist = ActiveCon()->RCon()->isSelectionPresent();
	//EnableMenuItem(systemMenu, ID_CON_COPY, MF_BYCOMMAND | (bSelectionExist?MF_ENABLED:MF_GRAYED));
	SetActiveWindow(ghWnd);
	//mb_InTrackSysMenu = TRUE;
	int command = trackPopupMenu(tmp_System, systemMenu, TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, x, y, 0, ghWnd, NULL);
	//mb_InTrackSysMenu = FALSE;

	if (Icon.isWindowInTray())
		switch(command)
		{
			case SC_RESTORE:
			case SC_MOVE:
			case SC_SIZE:
			case SC_MINIMIZE:
			case SC_MAXIMIZE:
				SendMessage(ghWnd, WM_TRAYNOTIFY, 0, WM_LBUTTONDOWN);
				break;
		}

	if (command)
		PostMessage(ghWnd, WM_SYSCOMMAND, (WPARAM)command, 0);
}

// Запуск отладки текущего GUI
void CConEmuMain::StartDebugLogConsole()
{
	if (IsDebuggerPresent())
		return; // УЖЕ!

	// Create process, with flag /Attach GetCurrentProcessId()
	// Sleep for sometimes, try InitHWND(hConWnd); several times
	WCHAR  szExe[MAX_PATH*2] = {0};
	BOOL lbRc = FALSE;
	//DWORD nLen = 0;
	PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
	STARTUPINFO si; memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	DWORD dwSelfPID = GetCurrentProcessId();
	// "/ATTACH" - низя, а то заблокируемся при попытке подключения к "отлаживаемому" GUI
	_wsprintf(szExe, SKIPLEN(countof(szExe)) L"\"%s\" /DEBUGPID=%u /BW=80 /BH=25 /BZ=%u",
	          WIN3264TEST(ms_ConEmuC32Full,ms_ConEmuC64Full), dwSelfPID, LONGOUTPUTHEIGHT_MAX);

	#ifdef _DEBUG
	if (MessageBox(NULL, szExe, L"StartDebugLogConsole", MB_OKCANCEL|MB_SYSTEMMODAL) != IDOK)
		return;
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
		lbRc = TRUE;
	}
}

void CConEmuMain::StartDebugActiveProcess()
{
	CRealConsole* pRCon = ActiveCon()->RCon();
	if (!pRCon)
		return;
	DWORD dwPID = pRCon->GetActivePID();
	if (!dwPID)
		return;

	// Create process, with flag /Attach GetCurrentProcessId()
	// Sleep for sometimes, try InitHWND(hConWnd); several times
	WCHAR  szExe[0x400] = {0};
	BOOL lbRc = FALSE;
	//DWORD nLen = 0;
	PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
	STARTUPINFO si = {sizeof(si)};
	WARNING("Наверное лучше переделать на CreateCon...");
	si.dwFlags |= STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	DWORD dwSelfPID = GetCurrentProcessId();
	int W = pRCon->TextWidth();
	int H = pRCon->TextHeight();
	int nBits = GetProcessBits(dwPID);
	LPCWSTR pszServer = (nBits == 64) ? ms_ConEmuC64Full : ms_ConEmuC32Full;
	_wsprintf(szExe, SKIPLEN(countof(szExe)) L"\"%s\" /ATTACH /GID=%i /BW=%i /BH=%i /BZ=%u /ROOT \"%s\" /DEBUGPID=%i ",
		pszServer, dwSelfPID, W, H, LONGOUTPUTHEIGHT_MAX, pszServer, dwPID);

	#ifdef _DEBUG
	if (MessageBox(NULL, szExe, L"StartDebugLogConsole", MB_OKCANCEL|MB_SYSTEMMODAL) != IDOK)
		return;
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
		lbRc = TRUE;
	}
}

//void CConEmuMain::StartLogCreateProcess()
//{
//    OPENFILENAME ofn; memset(&ofn,0,sizeof(ofn));
//    ofn.lStructSize=sizeof(ofn);
//    ofn.hwndOwner = ghWnd;
//    ofn.lpstrFilter = _T("Log files (*.log)\0*.log\0\0");
//    ofn.nFilterIndex = 1;
//    ofn.lpstrFile = ms_LogCreateProcess;
//    ofn.nMaxFile = MAX_PATH;
//    ofn.lpstrTitle = L"Log CreateProcess...";
//    ofn.lpstrDefExt = L"log";
//    ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
//            | OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
//    if (!GetSaveFileName(&ofn))
//        return;
//
//	mb_CreateProcessLogged = true;
//	UpdateLogCreateProcess();
//}
//
//void CConEmuMain::StopLogCreateProcess()
//{
//	mb_CreateProcessLogged = false;
//	UpdateLogCreateProcess();
//}
//
//void CConEmuMain::UpdateLogCreateProcess()
//{
//	UpdateGuiInfoMapping();
//
//	for (size_t i = 0; i < countof(mp_VCon); i++)
//	{
//		if (mp_VCon[i] == NULL)
//			continue;
//
//		DWORD nFarPID = mp_VCon[i]->RCon()->GetFarPID(TRUE);
//		if (nFarPID)
//		{
//			// Выполнить в плагине
//			CConEmuPipe pipe(nFarPID, 300);
//			if (pipe.Init(L"LogShell", TRUE))
//    			pipe.Execute(CMD_LOG_SHELL);
//		}
//	}
//}

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

	wchar_t szNo[32], szFlags[255]; szNo[0] = szFlags[0] = 0;
	DWORD nProgramStatus = mp_VActive->RCon()->GetProgramStatus();
	DWORD nFarStatus = mp_VActive->RCon()->GetFarStatus();
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
	//CheckDlgButton(hInfo, cbsProgress, ((nFarStatus&CES_WASPROGRESS) /*|| mp_VActive->RCon()->GetProgress(NULL)>=0*/) ? BST_CHECKED : BST_UNCHECKED);
	if (nFarStatus&CES_OPER_ERROR) wcscat_c(szFlags, L"%%Error ");
	//CheckDlgButton(hInfo, cbsProgressError, (nFarStatus&CES_OPER_ERROR) ? BST_CHECKED : BST_UNCHECKED);
	_wsprintf(szNo, SKIPLEN(countof(szNo)) L"%i/%i", mp_VActive->RCon()->GetFarPID(), mp_VActive->RCon()->GetFarPID(TRUE));

	if (hInfo)
	{
		SetDlgItemText(hInfo, tsTopPID, szNo);
		SetDlgItemText(hInfo, tsRConFlags, szFlags);
	}

	if (!abForce)
		return;

	MCHKHEAP;

	if (hInfo)
		SendDlgItemMessage(hInfo, lbProcesses, LB_RESETCONTENT, 0, 0);

	wchar_t temp[MAX_PATH];

	for (size_t j = 0; j < countof(mp_VCon); j++)
	{
		if (mp_VCon[j] == NULL) continue;

		ConProcess* pPrc = NULL;
		int nCount = mp_VCon[j]->RCon()->GetProcesses(&pPrc);

		for(int i=0; i<nCount; i++)
		{
			if (mp_VCon[j] == mp_VActive)
				_tcscpy(temp, _T("(*) "));
			else
				temp[0] = 0;

			swprintf(temp+_tcslen(temp), _T("[%i.%i] %s - PID:%i"),
			         j+1, i, pPrc[i].Name, pPrc[i].ProcessID);
			if (hInfo)
				SendDlgItemMessage(hInfo, lbProcesses, LB_ADDSTRING, 0, (LPARAM)temp);
		}

		if (pPrc) { free(pPrc); pPrc = NULL; }
	}

	MCHKHEAP
}

void CConEmuMain::UpdateCursorInfo(COORD crCursor, CONSOLE_CURSOR_INFO cInfo)
{
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

	CVirtualConsole* pVCon = mp_VActive;
	CVConGuard guard(pVCon);

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

	if (mp_VActive && mp_VActive->RCon())
		pszNewTitle = mp_VActive->RCon()->GetTitle();

	if (!pszNewTitle)
	{
		//if ((pszNewTitle = mp_VActive->RCon()->GetTitle()) == NULL)
		//	return;
		pszNewTitle = GetDefaultTitle();
	}

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
	if (!this)
		return;

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
	short nUpdateProgress = gpUpd ? gpUpd->GetUpdateProgress() : -1;
	short n;
	BOOL bActiveHasProgress = FALSE;
	BOOL bWasError = FALSE;

	if (mp_VActive)
	{
		if (!isValid(mp_VActive))
		{
			_ASSERTE(isValid(mp_VActive));
		}
		else if ((nProgress = mp_VActive->RCon()->GetProgress(&bWasError)) >= 0)
		{
			mn_Progress = max(nProgress, nUpdateProgress);
			bActiveHasProgress = TRUE;
		}
	}

	if (!bActiveHasProgress && nUpdateProgress >= 0)
	{
		nProgress = max(nProgress, nUpdateProgress);
	}

	// нас интересует возможное наличие ошибки во всех остальных консолях
	for (size_t i = 0; i < countof(mp_VCon); i++)
	{
		if (mp_VCon[i])
		{
			BOOL bCurError = FALSE;
			n = mp_VCon[i]->RCon()->GetProgress(&bCurError);

			if (bCurError)
				bWasError = TRUE;

			if (!bActiveHasProgress && n > nProgress)
				nProgress = n;
		}
	}

	if (!bActiveHasProgress)
	{
		mn_Progress = min(nProgress,100);
	}

	static short nLastProgress = -1;
	static BOOL  bLastProgressError = FALSE;

	if (nLastProgress != mn_Progress  || bLastProgressError != bWasError)
	{
		HRESULT hr = S_OK;

		//if (mp_TaskBar3)
		//{
		if (mn_Progress >= 0)
		{
			hr = Taskbar_SetProgressValue(mn_Progress);

			if (nLastProgress == -1 || bLastProgressError != bWasError)
				hr = Taskbar_SetProgressState(bWasError ? TBPF_ERROR : TBPF_NORMAL);
		}
		else
		{
			hr = Taskbar_SetProgressState(TBPF_NOPROGRESS);
		}
		//}

		// Запомнить последнее
		nLastProgress = mn_Progress;
		bLastProgressError = bWasError;
	}

	if (mn_Progress >= 0 && !bActiveHasProgress)
	{
		psTitle = MultiTitle;
		wsprintf(MultiTitle+_tcslen(MultiTitle), L"{*%i%%} ", mn_Progress);
	}

	if (gpSet->isMulti && !gpConEmu->mp_TabBar->IsTabsShown())
	{
		int nCur = 1, nCount = 0;

		for (size_t n = 0; n < countof(mp_VCon); n++)
		{
			if (mp_VCon[n])
			{
				nCount ++;

				if (mp_VActive == mp_VCon[n])
					nCur = n+1;
			}
		}

		if (nCount > 1)
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

void CConEmuMain::UpdateWindowRgn(int anX/*=-1*/, int anY/*=-1*/, int anWndWidth/*=-1*/, int anWndHeight/*=-1*/)
{
	HRGN hRgn = NULL;

	//if (gpSet->isHideCaptionAlways) {
	//	KillTimer(ghWnd, TIMER_CAPTION_APPEAR_ID);
	//	KillTimer(ghWnd, TIMER_CAPTION_DISAPPEAR_ID);
	//}

	if (mb_ForceShowFrame)
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

	// Облом получается при двукратном SetWindowRgn(ghWnd, NULL, TRUE);
	// Причем после следующего ресайза - рамка приходит в норму
	SetWindowRgn(ghWnd, hRgn, TRUE);
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
				gpConEmu->mp_VActive->RCon()->LogString(szInfo, TRUE);
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
				gpConEmu->mp_VActive->RCon()->LogString(szInfo, TRUE);
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

	for (size_t i = 0; i < countof(gpConEmu->mp_VCon); i++)
	{
		if (!gpConEmu->mp_VCon[i]) continue;

		// Запускаемые через "-new_console" цепляются через CECMD_ATTACH2GUI, а не через WinEvent
		// 111211 - "-new_console" теперь передается в GUI и исполняется в нем
		if (gpConEmu->mp_VCon[i]->RCon()->isDetached() || !gpConEmu->mp_VCon[i]->RCon()->isServerCreated())
			continue;

		HWND hRConWnd = gpConEmu->mp_VCon[i]->RCon()->ConWnd();
		if (hRConWnd == hwnd)
		{
			StartStopType sst = (anEvent == EVENT_CONSOLE_START_APPLICATION) ? sst_App16Start : sst_App16Stop;
			gpConEmu->mp_VCon[i]->RCon()->OnDosAppStartStop(sst, idChild);
			break;
		}
	}
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

void CConEmuMain::Invalidate(CVirtualConsole* apVCon)
{
	if (!this || !apVCon || !apVCon->isVisible()) return;

	apVCon->Invalidate();
}

void CConEmuMain::InvalidateAll()
{
	InvalidateRect(ghWnd, NULL, TRUE);
	for (size_t i = 0; i < countof(mp_VCon); i++)
	{
		if (mp_VCon[i] && mp_VCon[i]->isVisible())
			mp_VCon[i]->Invalidate();
	}
	//m_Back->Invalidate();
	gpConEmu->mp_TabBar->Invalidate();
}

void CConEmuMain::UpdateWindowChild(CVirtualConsole* apVCon)
{
	if (apVCon)
	{
		if (apVCon->isVisible())
			UpdateWindow(apVCon->GetView());
	}
	else
	{
		for (size_t i = 0; i < countof(mp_VCon); i++)
		{
			if (mp_VCon[i] && mp_VCon[i]->isVisible())
				UpdateWindow(mp_VCon[i]->GetView());
		}
	}
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
//					GetSystemMenu(); // Проверить корректность системного меню
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

	if (!apVCon)
		apVCon = mp_VActive;

	// По идее, кружок может появиться только в активной консоли
	_ASSERTE(apVCon == mp_VActive);

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
		else if (gpSet->isRClickSendKey > 1 && (mouse.state & MOUSE_R_LOCKED)
			&& m_RightClickingFrames > 0 && mh_RightClickingBmp)
		{
			//WORD nRDown = GetKeyState(VK_RBUTTON);
			//POINT ptCur; GetCursorPos(&ptCur);
			//ScreenToClient(hView, &ptCur);
			bool bRDown = isPressed(VK_RBUTTON);

			if (bRDown)
				//&& PtDiffTest(Rcursor, ptCur.x, ptCur.y, RCLICKAPPSDELTA))
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
						//-- KillTimer(ghWnd, TIMER_RCLICKPAINT); // таймер понадобится для "скрытия" кружочка после RCLICKAPPSTIMEOUT_MAX
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
			SetTimer(ghWnd, TIMER_RCLICKPAINT, TIMER_RCLICKPAINT_ELAPSE, NULL);
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
		KillTimer(ghWnd, TIMER_RCLICKPAINT);
		m_RightClickingCurrent = -1;
		Invalidate(ActiveCon());
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

void CConEmuMain::OnPaintClient(HDC hdc, int width, int height)
{
	// Если "завис" PostUpdate
	if (mp_TabBar->NeedPostUpdate())
		mp_TabBar->Update();

#ifdef _DEBUG
	RECT rcDbgSize; GetWindowRect(ghWnd, &rcDbgSize);
	wchar_t szSize[255]; _wsprintf(szSize, SKIPLEN(countof(szSize)) L"WM_PAINT -> Window size (X=%i, Y=%i, W=%i, H=%i)\n",
	                               rcDbgSize.left, rcDbgSize.top, (rcDbgSize.right-rcDbgSize.left), (rcDbgSize.bottom-rcDbgSize.top));
	DEBUGSTRSIZE(szSize);
	static RECT rcDbgSize1;

	if (memcmp(&rcDbgSize1, &rcDbgSize, sizeof(rcDbgSize1)))
	{
		rcDbgSize1 = rcDbgSize;
	}
#endif

	PaintGaps(hdc);
}

#if 0
LRESULT CConEmuMain::OnPaint(WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	// Если "завис" PostUpdate
	if (mp_TabBar->NeedPostUpdate())
		mp_TabBar->Update();

#ifdef _DEBUG
	RECT rcDbgSize; GetWindowRect(ghWnd, &rcDbgSize);
	wchar_t szSize[255]; _wsprintf(szSize, SKIPLEN(countof(szSize)) L"WM_PAINT -> Window size (X=%i, Y=%i, W=%i, H=%i)\n",
	                               rcDbgSize.left, rcDbgSize.top, (rcDbgSize.right-rcDbgSize.left), (rcDbgSize.bottom-rcDbgSize.top));
	DEBUGSTRSIZE(szSize);
	static RECT rcDbgSize1;

	if (memcmp(&rcDbgSize1, &rcDbgSize, sizeof(rcDbgSize1)))
	{
		rcDbgSize1 = rcDbgSize;
	}
#endif


	PAINTSTRUCT ps;
	#ifdef _DEBUG
	HDC hDc =
	#endif
	BeginPaint(ghWnd, &ps);
	//RECT rcClient; Get ClientRect(ghWnd, &rcClient);
	//RECT rcTabMargins = CalcMargins(CEM_TAB);
	//AddMargins(rcClient, rcTabMargins, FALSE);
	//HDC hdc = GetDC(ghWnd);
	PaintGaps(ps.hdc);
	//PaintCon(ps.hdc);

	// Отрисовка идет в hView
	//if (mb_RightClickingPaint)
	//{
	//	// Нарисует кружочек, или сбросит таймер, если кнопку отпустили
	//	RightClickingPaint(ps.hdc);
	//}

	EndPaint(ghWnd, &ps);
	//result = DefWindowProc(ghWnd, WM_PAINT, wParam, lParam);
	//ReleaseDC(ghWnd, hdc);
	//ValidateRect(ghWnd, &rcClient);
	return result;
}
#endif

void CConEmuMain::PaintGaps(HDC hDC)
{
	bool lbReleaseDC = false;

	if (hDC == NULL)
	{
		hDC = GetDC(ghWnd); // Главное окно!
		lbReleaseDC = true;
	}

	int
#ifdef _DEBUG
	nColorIdx = 1; // Blue
#else
	nColorIdx = 0; // Black
#endif
	HBRUSH hBrush = CreateSolidBrush(gpSet->GetColors(-1, !isMeForeground())[nColorIdx]);

	RECT rcClient = GetGuiClientRect(); // Клиентская часть главного окна

	HWND hView = mp_VActive ? mp_VActive->GetView() : NULL;

	if (!hView || !IsWindowVisible(hView))
	{
		FillRect(hDC, &rcClient, hBrush);
	}
	else
	{
		TODO("DoubleView: заливать с учетом, что видимых окон - два, и может быть промежуток между ними");

		//RECT rcMargins = CalcMargins(CEM_TAB); // Откусить площадь, занятую строкой табов
		//AddMargins(rcClient, rcMargins, FALSE);
		//// На старте при /max - ghWnd DC еще не изменил свое положение
		////RECT offsetRect; Get ClientRect(ghWnd DC, &offsetRect);
		//RECT rcWndClient; Get ClientRect(ghWnd, &rcWndClient);
		//RECT rcCalcCon = gpConEmu->CalcRect(CER_BACK, rcWndClient, CER_MAINCLIENT);
		//RECT rcCon = gpConEmu->CalcRect(CER_CONSOLE, rcCalcCon, CER_BACK);
		// -- работает не правильно - не учитывает центрирование в Maximized
		//RECT offsetRect = gpConEmu->CalcRect(CER_BACK, rcCon, CER_CONSOLE);
		/*
		RECT rcClient = {0};
		if (ghWnd DC) {
			Get ClientRect(ghWnd DC, &rcClient);
			MapWindowPoints(ghWnd DC, ghWnd, (LPPOINT)&rcClient, 2);
		}
		*/
		RECT dcSize = CalcRect(CER_DC, rcClient, CER_MAINCLIENT);
		RECT client = CalcRect(CER_DC, rcClient, CER_MAINCLIENT, NULL, &dcSize);
		WARNING("Вынести в CalcRect");
		RECT offsetRect; memset(&offsetRect,0,sizeof(offsetRect));

		if (mp_VActive && mp_VActive->Width && mp_VActive->Height)
		{
			if ((gpSet->isTryToCenter && (isZoomed() || mb_isFullScreen))
					|| isNtvdm())
			{
				offsetRect.left = (client.right+client.left-(int)mp_VActive->Width)/2;
				offsetRect.top = (client.bottom+client.top-(int)mp_VActive->Height)/2;
			}

			if (offsetRect.left<client.left) offsetRect.left=client.left;

			if (offsetRect.top<client.top) offsetRect.top=client.top;

			offsetRect.right = offsetRect.left + mp_VActive->Width;
			offsetRect.bottom = offsetRect.top + mp_VActive->Height;

			if (offsetRect.right>client.right) offsetRect.right=client.right;

			if (offsetRect.bottom>client.bottom) offsetRect.bottom=client.bottom;
		}
		else
		{
			offsetRect = client;
		}

		// paint gaps between console and window client area with first color
		RECT rect;
		//TODO:!!!
		// top
		rect = rcClient;
		rect.bottom = offsetRect.top;

		if (!IsRectEmpty(&rect))
			FillRect(hDC, &rect, hBrush);

		#ifdef _DEBUG
		//GdiFlush();
		#endif
		// right
		rect.left = offsetRect.right;
		rect.bottom = rcClient.bottom;

		if (!IsRectEmpty(&rect))
			FillRect(hDC, &rect, hBrush);

		#ifdef _DEBUG
		//GdiFlush();
		#endif
		// left
		rect.left = 0;
		rect.right = offsetRect.left;
		rect.bottom = rcClient.bottom;

		if (!IsRectEmpty(&rect))
			FillRect(hDC, &rect, hBrush);

		#ifdef _DEBUG
		//GdiFlush();
		#endif
		// bottom
		rect.left = 0;
		rect.right = rcClient.right;
		rect.top = offsetRect.bottom;
		rect.bottom = rcClient.bottom;

		if (!IsRectEmpty(&rect))
			FillRect(hDC, &rect, hBrush);

		#ifdef _DEBUG
		//GdiFlush();
		#endif
		DeleteObject(hBrush);
	}

	if (lbReleaseDC)
		ReleaseDC(ghWnd, hDC);
}

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
//	// если mp_VActive==NULL - будет просто выполнена заливка фоном.
//	mp_VActive->Paint(hPaintDC, rcClient);
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

void CConEmuMain::RePaint()
{
	gpConEmu->mp_TabBar->RePaint();
	//m_Back->RePaint();
	HDC hDc = GetDC(ghWnd);
	//mp_VActive->Paint(hDc); // если mp_VActive==NULL - будет просто выполнена заливка фоном.
	PaintGaps(hDc);
	//PaintCon(hDc);
	ReleaseDC(ghWnd, hDc);

	for (size_t i = 0; i < countof(mp_VCon); i++)
	{
		if (mp_VCon[i] && mp_VCon[i]->isVisible())
		{
			CVirtualConsole* pVCon = mp_VCon[i];
			CVConGuard guard(pVCon);
			HWND hView = pVCon->GetView();
			if (hView)
			{
				hDc = GetDC(hView);
				RECT rcClient = pVCon->GetDcClientRect();
				pVCon->Paint(hDc, rcClient);
				ReleaseDC(ghWnd, hDc);
			}
		}
	}
}

void CConEmuMain::Update(bool isForce /*= false*/)
{
	if (isForce)
	{
		for (size_t i = 0; i < countof(mp_VCon); i++)
		{
			if (mp_VCon[i])
				mp_VCon[i]->OnFontChanged();
		}
	}

	CVirtualConsole::ClearPartBrushes();

	if (mp_VActive)
	{
		mp_VActive->Update(isForce);
		//InvalidateAll();
	}
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
	DWORD dwPID = 0;

	if (mp_VActive && mp_VActive->RCon())
		dwPID = mp_VActive->RCon()->GetFarPID(abPluginRequired);

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
	if (nIdx < 0 || nIdx >= (int)countof(mp_VCon))
		return NULL;

	if (!mp_VCon[nIdx] || !mp_VCon[nIdx]->RCon())
		return NULL;

	LPCWSTR pszTitle = mp_VCon[nIdx]->RCon()->GetTitle();
	if (pszTitle == NULL)
	{
		_ASSERTE(pszTitle!=NULL);
		pszTitle = GetDefaultTitle();
	}
	return pszTitle;
}

// Возвращает индекс (0-based) активной консоли
int CConEmuMain::GetActiveVCon()
{
	if (mp_VActive)
	{
		for (size_t i = 0; i < countof(mp_VCon); i++)
		{
			if (mp_VCon[i] == mp_VActive)
				return i;
		}
	}
	return -1;
}

CVirtualConsole* CConEmuMain::GetVCon(int nIdx)
{
	if (nIdx < 0 || nIdx >= (int)countof(mp_VCon))
	{
		_ASSERTE(nIdx>=0 && nIdx<(int)countof(mp_VCon));
		return NULL;
	}

	return mp_VCon[nIdx];
}

// 0 - такой консоли нет
// 1..MAX_CONSOLE_COUNT - "номер" консоли (1 based!)
int CConEmuMain::IsVConValid(CVirtualConsole* apVCon)
{
	if (!apVCon)
		return 0;
	for (size_t i = 0; i < countof(mp_VCon); i++)
	{
		if (mp_VCon[i] == apVCon)
			return (i+1);
	}
	return 0;
}

CVirtualConsole* CConEmuMain::GetVConFromPoint(POINT ptScreen)
{
	CVirtualConsole* pVCon;

	for (size_t i = 0; i < countof(mp_VCon); i++)
	{
		if ((pVCon = mp_VCon[i]) != NULL && pVCon->isVisible())
		{
			
			HWND hView = pVCon->GetView();
			if (hView)
			{
				RECT rcView; GetWindowRect(hView, &rcView);

				if (PtInRect(&rcView, ptScreen))
					return pVCon;
			}
			else
			{
				_ASSERTE(pVCon->GetView()!=NULL);
			}
		}
	}

	return NULL;
}

bool CConEmuMain::isActive(CVirtualConsole* apVCon)
{
	if (!this || !apVCon)
		return false;

	if (apVCon == mp_VActive)
		return true;

	return false;
}

bool CConEmuMain::isConSelectMode()
{
	//TODO: По курсору, что-ли попробовать определять?
	//return gb_ConsoleSelectMode;
	if (mp_VActive)
		return mp_VActive->RCon()->isConSelectMode();

	return false;
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

bool CConEmuMain::isFilePanel(bool abPluginAllowed/*=false*/)
{
	if (!mp_VActive) return false;

	bool lbIsPanels = mp_VActive->RCon()->isFilePanel(abPluginAllowed);
	return lbIsPanels;
}

bool CConEmuMain::isFirstInstance()
{
	if (!mb_AliveInitialized)
	{
		mb_AliveInitialized = TRUE;
		// создадим событие, чтобы не было проблем с ключем /SINGLE
		lstrcpy(ms_ConEmuAliveEvent, CEGUI_ALIVE_EVENT);
		DWORD nSize = MAX_PATH;
		// Добавим имя текущего юзера. Нам не нужны конфликты при наличии нескольких юзеров.
		GetUserName(ms_ConEmuAliveEvent+_tcslen(ms_ConEmuAliveEvent), &nSize);
		WARNING("Event не работает, если conemu.exe запущен под другим пользователем");
		mh_ConEmuAliveEvent = CreateEvent(NULL, TRUE, TRUE, ms_ConEmuAliveEvent);
		nSize = GetLastError();

		// имя пользователя теоретически может содержать символы, которые недопустимы в имени Event
		if (!mh_ConEmuAliveEvent /* || nSize == ERROR_PATH_NOT_FOUND */)
		{
			lstrcpy(ms_ConEmuAliveEvent, CEGUI_ALIVE_EVENT);
			mh_ConEmuAliveEvent = CreateEvent(NULL, TRUE, TRUE, ms_ConEmuAliveEvent);
			nSize = GetLastError();
		}

		mb_ConEmuAliveOwned = mh_ConEmuAliveEvent && (nSize!=ERROR_ALREADY_EXISTS);
	}

	if (mh_ConEmuAliveEvent && !mb_ConEmuAliveOwned)
	{
		if (WaitForSingleObject(mh_ConEmuAliveEvent,0) == WAIT_TIMEOUT)
		{
			SetEvent(mh_ConEmuAliveEvent);
			mb_ConEmuAliveOwned = TRUE;

			// Этот экземпляр становится "Основным" (другой, ранее бывший основным, был закрыт)
			if (gpSet->IsHotkey(vkMinimizeRestore))
				RegisterMinRestore(true);
		}
	}

	return mb_ConEmuAliveOwned;
}

bool CConEmuMain::isEditor()
{
	if (!mp_VActive) return false;

	return mp_VActive->RCon()->isEditor();
}

bool CConEmuMain::isFar(bool abPluginRequired/*=false*/)
{
	if (!mp_VActive) return false;

	return mp_VActive->RCon()->isFar(abPluginRequired);
}

// Если ли фар где-то?
bool CConEmuMain::isFarExist(CEFarWindowType anWindowType/*=fwt_Any*/, LPWSTR asName/*=NULL*/, CVConGuard* rpVCon/*=NULL*/)
{
	bool bFound = false, bLocked = false;
	CVConGuard VCon;

	if (rpVCon)
		*rpVCon = NULL;

	for (INT_PTR i = -1; i < (INT_PTR)countof(mp_VCon); i++)
	{
		if (i == -1)
			VCon = mp_VActive;
		else
			VCon = mp_VCon[i];

		if (VCon.VCon())
		{
			// Это фар?
			CRealConsole* pRCon = VCon->RCon();
			if (pRCon && pRCon->isFar(anWindowType & fwt_PluginRequired))
			{
				// Ищем что-то конкретное?
				if (!(anWindowType & (fwt_TypeMask|fwt_Elevated|fwt_NonElevated|fwt_Modal|fwt_NonModal|fwt_ActivateFound)) && !(asName && *asName))
				{
					bFound = true;
					break;
				}

				if (!(anWindowType & (fwt_TypeMask|fwt_ActivateFound)) && !(asName && *asName))
				{
					CEFarWindowType t = pRCon->GetActiveTabType();

					// Этот Far Elevated?
					if ((anWindowType & fwt_Elevated) && !(t & fwt_Elevated))
						continue;
					// В табе устанавливается флаг fwt_Elevated
					// fwt_NonElevated используется только как аргумент поиска
					if ((anWindowType & fwt_NonElevated) && (t & fwt_Elevated))
						continue;

					// Модальное окно?
					WARNING("Нужно еще учитывать <модальность> заблокированным диалогом, или меню, или еще чем-либо!");
					if ((anWindowType & fwt_Modal) && !(t & fwt_Modal))
						continue;
					// В табе устанавливается флаг fwt_Modal
					// fwt_NonModal используется только как аргумент поиска
					if ((anWindowType & fwt_NonModal) && (t & fwt_Modal))
						continue;

					bFound = true;
					break;
				}
				else
				{
					// Нужны доп.проверки окон фара
					ConEmuTab tab;
					LPCWSTR pszNameOnly = asName ? PointToName(asName) : NULL;
					if (pszNameOnly)
					{
						// Обработаем как обратные (в PointToName), так и прямые слеши
						// Это может быть актуально при переходе на ошибку/гиперссылку
						LPCWSTR pszSlash = wcsrchr(pszNameOnly, L'/');
						if (pszSlash)
							pszNameOnly = pszSlash+1;
					}

					for (int j = 0; !bFound; j++)
					{
						if (!pRCon->GetTab(j, &tab))
							break;

						if ((tab.Type & fwt_TypeMask) != (anWindowType & fwt_TypeMask))
							continue;

						// Этот Far Elevated?
						if ((anWindowType & fwt_Elevated) && !(tab.Type & fwt_Elevated))
							continue;
						// В табе устанавливается флаг fwt_Elevated
						// fwt_NonElevated используется только как аргумент поиска
						if ((anWindowType & fwt_NonElevated) && (tab.Type & fwt_Elevated))
							continue;

						// Модальное окно?
						WARNING("Нужно еще учитывать <модальность> заблокированным диалогом, или меню, или еще чем-либо!");
						if ((anWindowType & fwt_Modal) && !(tab.Type & fwt_Modal))
							continue;
						// В табе устанавливается флаг fwt_Modal
						// fwt_NonModal используется только как аргумент поиска
						if ((anWindowType & fwt_NonModal) && (tab.Type & fwt_Modal))
							continue;

						// Если ищем конкретный редактор/вьювер
						if (asName && *asName)
						{
							if (lstrcmpi(tab.Name, asName) == 0)
							{
								bFound = true;
							}
							else if ((pszNameOnly != asName) && (lstrcmpi(PointToName(tab.Name), pszNameOnly) == 0))
							{
								bFound = true;
							}
						}
						else
						{
							bFound = true;
						}


						if (bFound)
						{
							if (anWindowType & fwt_ActivateFound)
							{
								if (pRCon->ActivateFarWindow(j))
								{
									gpConEmu->Activate(VCon.VCon());
									bLocked = false;
								}
								else
								{
									bLocked = true;
								}
							}

							break;
						}
					}
				}
			}
		}
	}

	// Нашли?
	if (bFound)
	{
		if (rpVCon)
		{
			*rpVCon = VCon.VCon();
			if (bLocked)
				bFound = false;
		}
	}

	return bFound;
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

bool CConEmuMain::isMeForeground(bool abRealAlso)
{
	if (!this) return false;

	static HWND hLastFore = NULL;
	static bool isMe = false;
	HWND h = GetForegroundWindow();

	if (h != hLastFore)
	{
		isMe = (h != NULL)
			&& (((h == ghWnd) || (h == ghOpWnd)
				|| (mp_AttachDlg && (mp_AttachDlg->GetHWND() == h)))
			|| (mp_RecreateDlg && (mp_RecreateDlg->GetHWND() == h)))
			|| (gpSetCls->mh_FindDlg == h);

		if (h && !isMe && abRealAlso)
		{
			for (size_t i = 0; i < countof(mp_VCon); i++)
			{
				if (mp_VCon[i])
				{
					if (h == mp_VCon[i]->RCon()->ConWnd())
					{
						isMe = true; break;
					}
				}
			}
		}

		hLastFore = h;
	}

	return isMe;
}

bool CConEmuMain::isMouseOverFrame(bool abReal)
{
	if (mb_CaptionWasRestored && isSizing())
	{
		if (!isPressed(VK_LBUTTON))
		{
			mouse.state &= ~MOUSE_SIZING_BEGIN;
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
		// чтобы область активации рамки была чуть побольше
		rcWnd.left--; rcWnd.right++; rcWnd.bottom++;

		if (PtInRect(&rcWnd, ptMouse))
		{
			RECT rcClient = GetGuiClientRect();
			// чтобы область активации рамки была чуть побольше
			rcClient.left++; rcClient.right--; rcClient.bottom--;
			MapWindowPoints(ghWnd, NULL, (LPPOINT)&rcClient, 2);

			if (!PtInRect(&rcClient, ptMouse))
				bCurForceShow = true;
		}
	}
	else
	{
		bCurForceShow = mb_ForceShowFrame;
	}

	return bCurForceShow;
}

bool CConEmuMain::isNtvdm(BOOL abCheckAllConsoles/*=FALSE*/)
{
	if (mp_VActive && mp_VActive->RCon())
	{
		if (mp_VActive->RCon()->isNtvdm())
			return true;
	}

	for (size_t i = 0; i < countof(mp_VCon); i++)
	{
		if (mp_VCon[i] && mp_VCon[i] != mp_VActive && mp_VCon[i]->RCon())
		{
			if (mp_VCon[i]->RCon()->isNtvdm())
				return true;
		}
	}

	return false;
}

bool CConEmuMain::IsOurConsoleWindow(HWND hCon)
{
	if (!hCon)
		return false;
	
	for (size_t i = 0; i < countof(mp_VCon); i++)
	{
		if (mp_VCon[i] && mp_VCon[i]->RCon())
		{
			if (mp_VCon[i]->RCon()->ConWnd() == hCon)
				return true;
		}
	}
	
	return false;
}

bool CConEmuMain::isValid(CRealConsole* apRCon)
{
	if (!apRCon)
		return false;

	for (size_t i = 0; i < countof(mp_VCon); i++)
	{
		if (mp_VCon[i] && apRCon == mp_VCon[i]->RCon())
			return true;
	}

	return false;
}

bool CConEmuMain::isValid(CVirtualConsole* apVCon)
{
	if (!apVCon)
		return false;

	for (size_t i = 0; i < countof(mp_VCon); i++)
	{
		if (apVCon == mp_VCon[i])
			return true;
	}

	return false;
}

bool CConEmuMain::isViewer()
{
	if (!mp_VActive) return false;

	return mp_VActive->RCon()->isViewer();
}

bool CConEmuMain::isVisible(CVirtualConsole* apVCon)
{
	if (!this || !apVCon)
		return false;

	TODO("После добавления второго DoubleView потребуется коррекция");

	if (apVCon == mp_VActive || apVCon == mp_VCon1 || apVCon == mp_VCon2)
		return true;

	return false;
}

bool CConEmuMain::isWindowNormal()
{
	if (change2WindowMode != (DWORD)-1)
	{
		return (change2WindowMode == rNormal);
	}

	if (mb_isFullScreen || isZoomed() || isIconic())
		return false;

	return true;
}

bool CConEmuMain::isChildWindow()
{
	for (size_t i = 0; i < countof(mp_VCon); i++)
	{
		if (mp_VCon[i] && isVisible(mp_VCon[i]))
		{
			if (mp_VCon[i]->RCon())
			{
				if (mp_VCon[i]->RCon()->GuiWnd() || mp_VCon[i]->RCon()->isPictureView())
					return true;
			}
		}
	}

	return false;
}

// Заголовок окна для PictureView вообще может пользователем настраиваться, так что
// рассчитывать на него при определения "Просмотра" - нельзя
bool CConEmuMain::isPictureView()
{
	bool lbRc = false;

	if (hPictureView && (!IsWindow(hPictureView) || !isFar()))
	{
		InvalidateAll();
		hPictureView = NULL;
	}

	bool lbPrevPicView = (hPictureView != NULL);

	for (size_t i = 0; !lbRc && i < countof(mp_VCon); i++)
	{
		CVirtualConsole* pVCon = mp_VCon[i];
		if (!pVCon || !isVisible(pVCon) || !pVCon->RCon())
			continue;

		hPictureView = pVCon->RCon()->isPictureView();

		lbRc = hPictureView!=NULL;

		// Если вызывали Help (F1) - окошко PictureView прячется
		if (hPictureView && !IsWindowVisible(hPictureView))
		{
			lbRc = false;
			hPictureView = NULL;
		}
	}

	if (bPicViewSlideShow && !hPictureView)
	{
		bPicViewSlideShow=false;
	}

	if (lbRc && !lbPrevPicView)
	{
		GetWindowRect(ghWnd, &mrc_WndPosOnPicView);
	}
	else if (!lbRc)
	{
		memset(&mrc_WndPosOnPicView, 0, sizeof(mrc_WndPosOnPicView));
	}

	return lbRc;
}

bool CConEmuMain::isProcessCreated()
{
	return (mb_ProcessCreated != FALSE);
}

bool CConEmuMain::isSizing()
{
	// Юзер тащит мышкой рамку окна
	return (mouse.state & MOUSE_SIZING_BEGIN) == MOUSE_SIZING_BEGIN;
}

// Сюда может придти только LOWORD от HKL
void CConEmuMain::SwitchKeyboardLayout(DWORD_PTR dwNewKeybLayout)
{
	if ((gpSet->isMonitorConsoleLang & 1) == 0)
	{
		if (gpSetCls->isAdvLogging > 1)
			mp_VActive->RCon()->LogString(L"CConEmuMain::SwitchKeyboardLayout skipped, cause of isMonitorConsoleLang==0");

		return;
	}

#ifdef _DEBUG
	wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"CConEmuMain::SwitchKeyboardLayout(0x%08I64X)\n", (unsigned __int64)dwNewKeybLayout);
	DEBUGSTRLANG(szDbg);
#endif
	HKL hKeyb[20]; UINT nCount, i; BOOL lbFound = FALSE;
	nCount = GetKeyboardLayoutList(countof(hKeyb), hKeyb);

	for(i = 0; !lbFound && i < nCount; i++)
	{
		if (hKeyb[i] == (HKL)dwNewKeybLayout)
			lbFound = TRUE;
	}

	WARNING("Похоже с другими раскладками будет глючить. US Dvorak?");

	for(i = 0; !lbFound && i < nCount; i++)
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
			mp_VActive->RCon()->LogString(szInfo);
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
			mp_VActive->RCon()->LogString(szInfo);
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

	BOOL lbBufferHeight = mp_VActive->RCon()->isBufferHeight();
	BOOL lbAlternative = mp_VActive->RCon()->isAlternative();

	TrackMouse(); // спрятать или показать прокрутку, если над ней мышка

	mp_TabBar->OnBufferHeight(lbBufferHeight);
	mp_TabBar->OnAlternative(lbAlternative);
}


LRESULT CConEmuMain::OnClose(HWND hWnd)
{
	TODO("И вообще, похоже это событие не вызывается");
	_ASSERTE("CConEmuMain::OnClose"==NULL);

	// Если все-таки вызовется - имитировать SC_CLOSE
	OnSysCommand(hWnd, SC_CLOSE, 0);
	//_ASSERTE(FALSE);
	//Icon.Delete(); - перенес в WM_DESTROY
	//mb_InClose = TRUE;
	//if (ghConWnd && IsWindow(ghConWnd)) {
	//    mp_VActive->RCon()->CloseConsole(false, false);
	//} else {
	//    Destroy();
	//}
	//mb_InClose = FALSE;
	return 0;
}

BOOL CConEmuMain::OnCloseQuery()
{
	int nEditors = 0, nProgress = 0, i;

	for (i = ((int)countof(mp_VCon)-1); i >= 0; i--)
	{
		CRealConsole* pRCon = NULL;
		//ConEmuTab tab = {0};

		if (mp_VCon[i] && (pRCon = mp_VCon[i]->RCon())!=NULL)
		{
			// Прогрессы (копирование, удаление, и т.п.)
			if (pRCon->GetProgress(NULL) != -1)
				nProgress ++;

			// Несохраненные редакторы
			int n = pRCon->GetModifiedEditors();

			if (n)
				nEditors += n;
		}
	}

	if (nProgress || nEditors)
	{
		wchar_t szText[255], *pszText;
		wcscpy_c(szText, L"Close confirmation.\r\n\r\n"); pszText = szText+_tcslen(szText);

		if (nProgress) { _wsprintf(pszText, SKIPLEN(countof(szText)-(pszText-szText)) L"Incomplete operations: %i\r\n", nProgress); pszText += _tcslen(pszText); }

		if (nEditors) { _wsprintf(pszText, SKIPLEN(countof(szText)-(pszText-szText)) L"Unsaved editor windows: %i\r\n", nEditors); pszText += _tcslen(pszText); }

		lstrcpy(pszText, L"\r\nProceed with shutdown?");
		int nBtn = MessageBoxW(ghWnd, szText, GetDefaultTitle(), MB_OKCANCEL|MB_ICONEXCLAMATION);

		if (nBtn != IDOK)
			return FALSE; // не закрывать
	}

	#ifdef _DEBUG
	if (gbInMyAssertTrap)
		return FALSE;
	#endif

	// Чтобы мог сработать таймер закрытия
	OnRConStartedSuccess(NULL);
	return TRUE; // можно
}

HMENU CConEmuMain::CreateDebugMenuPopup()
{
	HMENU hDebug = CreatePopupMenu();
	AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_CON_TOGGLE_VISIBLE, _T("&Real console"));
	AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_CONPROP, _T("&Properties..."));
	AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_DUMPCONSOLE, _T("&Dump screen..."));
	AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_LOADDUMPCONSOLE, _T("&Load screen dump..."));
	AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_DEBUGGUI, _T("Debug &log (GUI)"));
	AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_DEBUGCON, _T("Debug &active process"));
//#ifdef _DEBUG
//	AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_MONITOR_SHELLACTIVITY, _T("Enable &shell log..."));
//#endif
	AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_DEBUG_SHOWRECTS, _T("Show debug rec&ts"));
	return hDebug;
}

HMENU CConEmuMain::CreateVConListPopupMenu(HMENU ahExist, BOOL abFirstTabOnly)
{
	HMENU h = ahExist ? ahExist : CreatePopupMenu();
	wchar_t szText[128];
	const int nMaxStrLen = 32;

	BOOL lbActiveVCon = FALSE;
	int nActiveCmd = -1; // DWORD MAKELONG(WORD wLow,WORD wHigh);
	CVirtualConsole* pVCon = NULL;
	DWORD nAddFlags = 0;
	
	if (ahExist)
	{
		while (DeleteMenu(ahExist, 0, MF_BYPOSITION))
			;
	}
	
	for (int V = 0; (pVCon = GetVCon(V))!=NULL; V++)
	{
		if ((lbActiveVCon = isActive(pVCon)))
			nActiveCmd = MAKELONG(1, V+1);
		nAddFlags = 0; //(lbActiveVCon ? MF_DEFAULT : 0);
		CRealConsole* pRCon = pVCon->RCon();
		if (!pRCon)
		{
			wsprintf(szText, L"%i: VConsole", V+1);
			AppendMenu(h, MF_STRING|nAddFlags, MAKELONG(1, V+1), szText);
		}
		else
		{
			ConEmuTab tab = {};
			int R = 0;
			if (!pRCon->GetTab(R, &tab))
			{
				wsprintf(szText, L"%i: RConsole", V+1);
				AppendMenu(h, MF_STRING|nAddFlags, MAKELONG(1, V+1), szText);
			}
			else
			{
				do
				{
					nAddFlags = 0/*((lbActiveVCon && (R==0)) ? MF_DEFAULT : 0)*/
						| ((lbActiveVCon && (abFirstTabOnly || pRCon->GetActiveTab() == R)) ? MF_CHECKED : MF_UNCHECKED)
						#if 0
						| ((tab->Flags() & etfDisabled) ? (MF_DISABLED|MF_GRAYED) : 0)
						#endif
						;
					int nLen = lstrlen(tab.Name/*.Ptr()*/);
					if (!R)
						wsprintf(szText, L"%i: ", V+1);
					else
						wcscpy_c(szText, L"      ");
					if (nLen <= nMaxStrLen)
					{
						wcscat_c(szText, tab.Name/*.Ptr()*/);
					}
					else
					{
						int nCurLen = lstrlen(szText);
						_ASSERTE((nCurLen+10)<nMaxStrLen);
						if ((tab.Type & fwt_TypeMask) == fwt_Panels)
						{
							lstrcpyn(szText+nCurLen, tab.Name/*.Ptr()*/, nMaxStrLen-1-nCurLen);
						}
						else
						{
							szText[nCurLen++] = L'\x2026'; szText[nCurLen] = 0;
							lstrcpyn(szText+nCurLen, tab.Name+nLen-nMaxStrLen, nMaxStrLen-1-nCurLen);
						}
						wcscat_c(szText, L"\x2026"); //...
					}
					AppendMenu(h, MF_STRING|nAddFlags, MAKELONG(R+1, V+1), szText);
				} while (!abFirstTabOnly && pRCon->GetTab(++R, &tab));
			}
		}
	}

	if (nActiveCmd != -1 && !abFirstTabOnly)
	{
		MENUITEMINFO mi = {sizeof(mi), MIIM_STATE|MIIM_ID};
		mi.wID = nActiveCmd;
		GetMenuItemInfo(h, nActiveCmd, FALSE, &mi);
		mi.fState |= MF_DEFAULT;
		SetMenuItemInfo(h, nActiveCmd, FALSE, &mi);
	}
	
	return h;
}

HMENU CConEmuMain::CreateVConPopupMenu(CVirtualConsole* apVCon, HMENU ahExist, BOOL abAddNew, HMENU& hTerminate)
{
	//BOOL lbEnabled = TRUE;
	HMENU hMenu = ahExist;
	
	if (!apVCon)
		apVCon = ActiveCon();

	if (!hTerminate)
		hTerminate = CreatePopupMenu();
	
	if (!hMenu)
	{	
		hMenu = CreatePopupMenu();
		
		/*
        MENUITEM "&Close",                      IDM_CLOSE
        MENUITEM "Detach",                      IDM_DETACH
        MENUITEM "&Terminate",                  IDM_TERMINATE
        MENUITEM SEPARATOR
        MENUITEM "&Restart",                    IDM_RESTART
        MENUITEM "Restart as...",               IDM_RESTARTAS
        MENUITEM SEPARATOR
        MENUITEM "New console...",              IDM_NEW
        MENUITEM SEPARATOR
        MENUITEM "&Save",                       IDM_SAVE
        MENUITEM "Save &all",                   IDM_SAVEALL
		*/
		
		TODO("Добавить пункт IDM_ADMIN_DUPLICATE");
		
		AppendMenu(hMenu, MF_STRING | MF_ENABLED,     IDM_CLOSE,     L"&Close");
		AppendMenu(hMenu, MF_STRING | MF_ENABLED,     IDM_DETACH,    L"Detach");
		AppendMenu(hTerminate, MF_STRING | MF_ENABLED, IDM_TERMINATECON, L"&Console");
		AppendMenu(hTerminate, MF_SEPARATOR, 0, L"");
		AppendMenu(hTerminate, MF_STRING | MF_ENABLED, IDM_TERMINATEPRC, L"&Active process");
		AppendMenu(hMenu, MF_POPUP | MF_ENABLED, (UINT_PTR)hTerminate, L"&Terminate");
		AppendMenu(hMenu, MF_SEPARATOR, 0, L"");
		AppendMenu(hMenu, MF_STRING | MF_ENABLED,     IDM_RESTART,   L"&Restart");
		AppendMenu(hMenu, MF_STRING | MF_ENABLED,     IDM_RESTARTAS, L"Restart as...");
		if (abAddNew)
		{
			AppendMenu(hMenu, MF_SEPARATOR, 0, L"");
			AppendMenu(hMenu, MF_STRING | MF_ENABLED, ID_NEWCONSOLE, L"New console...");
			AppendMenu(hMenu, MF_STRING | MF_ENABLED, IDM_ATTACHTO,  L"Attach to...");
		}
		AppendMenu(hMenu, MF_SEPARATOR, 0, L"");
		AppendMenu(hMenu, MF_STRING | MF_ENABLED,     IDM_SAVE,      L"&Save");
		AppendMenu(hMenu, MF_STRING | MF_ENABLED,     IDM_SAVEALL,   L"Save &all");
	}

	if (apVCon)
	{
		bool lbIsFar = apVCon->RCon()->isFar(TRUE/* abPluginRequired */)!=FALSE;
		#ifdef _DEBUG
		bool lbIsPanels = lbIsFar && apVCon->RCon()->isFilePanel(false/* abPluginAllowed */)!=FALSE;
		#endif
		bool lbIsEditorModified = lbIsFar && apVCon->RCon()->isEditorModified()!=FALSE;
		bool lbHaveModified = lbIsFar && apVCon->RCon()->GetModifiedEditors()!=0;
		bool lbCanCloseTab = apVCon->RCon()->CanCloseTab();

		if (lbHaveModified)
		{
			if (!gpSet->sSaveAllMacro || !*gpSet->sSaveAllMacro)
				lbHaveModified = false;
		}

		EnableMenuItem(hMenu, IDM_CLOSE, MF_BYCOMMAND | (lbCanCloseTab ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(hMenu, IDM_DETACH, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hTerminate, IDM_TERMINATECON, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hTerminate, IDM_TERMINATEPRC, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_RESTART, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_RESTARTAS, MF_BYCOMMAND | MF_ENABLED);
		//EnableMenuItem(hMenu, IDM_ADMIN_DUPLICATE, MF_BYCOMMAND | (lbIsPanels ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(hMenu, IDM_SAVE, MF_BYCOMMAND | (lbIsEditorModified ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(hMenu, IDM_SAVEALL, MF_BYCOMMAND | (lbHaveModified ? MF_ENABLED : MF_GRAYED));
	}
	else
	{
		EnableMenuItem(hMenu, IDM_CLOSE, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_DETACH, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hTerminate, IDM_TERMINATECON, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hTerminate, IDM_TERMINATEPRC, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_RESTART, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_RESTARTAS, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_SAVE, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_SAVEALL, MF_BYCOMMAND | MF_GRAYED);
	}

	return hMenu;
}

//void CConEmuMain::PopulateEditMenuPopup(HMENU hMenu)
HMENU CConEmuMain::CreateEditMenuPopup(CVirtualConsole* apVCon, HMENU ahExist /*= NULL*/)
{
	if (!apVCon)
		apVCon = ActiveCon();
		
	BOOL lbEnabled = TRUE;
	BOOL lbSelectionExist = FALSE;
	if (apVCon && apVCon->RCon())
	{
		if (apVCon->RCon()->GuiWnd() && !apVCon->RCon()->isBufferHeight())
			lbEnabled = FALSE; // Если видимо дочернее графическое окно - выделение смысла не имеет
		// Нужно ли серить пункт "Copy"
		lbSelectionExist = lbEnabled && ActiveCon()->RCon()->isSelectionPresent();
	}

	HMENU hMenu = ahExist;
	
	if (!hMenu)
	{	
		hMenu = CreatePopupMenu();
		AppendMenu(hMenu, MF_STRING | (lbEnabled?MF_ENABLED:MF_GRAYED), ID_CON_MARKBLOCK, _T("Mark &block"));
		AppendMenu(hMenu, MF_STRING | (lbEnabled?MF_ENABLED:MF_GRAYED), ID_CON_MARKTEXT, _T("Mar&k text"));
		AppendMenu(hMenu, MF_STRING | (lbSelectionExist?MF_ENABLED:MF_GRAYED), ID_CON_COPY, _T("Cop&y"));
		AppendMenu(hMenu, MF_STRING | (lbEnabled?MF_ENABLED:MF_GRAYED), ID_CON_PASTE, _T("&Paste"));
		AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
		AppendMenu(hMenu, MF_STRING | (lbEnabled?MF_ENABLED:MF_GRAYED), ID_CON_FIND, _T("&Find text..."));
	}
	else
	{
		EnableMenuItem(hMenu, ID_CON_MARKBLOCK, MF_BYCOMMAND | (lbEnabled?MF_ENABLED:MF_GRAYED));
		EnableMenuItem(hMenu, ID_CON_MARKTEXT, MF_BYCOMMAND | (lbEnabled?MF_ENABLED:MF_GRAYED));
		EnableMenuItem(hMenu, ID_CON_COPY, MF_BYCOMMAND | (lbSelectionExist?MF_ENABLED:MF_GRAYED));
		EnableMenuItem(hMenu, ID_CON_PASTE, MF_BYCOMMAND | (lbEnabled?MF_ENABLED:MF_GRAYED));
		EnableMenuItem(hMenu, ID_CON_FIND, MF_BYCOMMAND | (lbEnabled?MF_ENABLED:MF_GRAYED));
	}
	
	return hMenu;
}

HMENU CConEmuMain::CreateHelpMenuPopup()
{
	HMENU hHelp = CreatePopupMenu();
	if (gpUpd && gpUpd->InUpdate())
		AppendMenu(hHelp, MF_STRING | MF_ENABLED, ID_STOPUPDATE, _T("&Stop updates checking"));
	else
		AppendMenu(hHelp, MF_STRING | MF_ENABLED, ID_CHECKUPDATE, _T("&Check for updates"));
	
	AppendMenu(hHelp, MF_STRING | MF_ENABLED, ID_HOMEPAGE, _T("&Visit home page"));
	AppendMenu(hHelp, MF_STRING | MF_ENABLED, ID_REPORTBUG, _T("&Report a bug..."));
	
	if (ms_ConEmuChm[0])  //Показывать пункт только если есть conemu.chm
		AppendMenu(hHelp, MF_STRING | MF_ENABLED, ID_HELP, _T("&Help"));

	AppendMenu(hHelp, MF_SEPARATOR, 0, NULL);	
	AppendMenu(hHelp, MF_STRING | MF_ENABLED, ID_ABOUT, _T("&About"));
	
	return hHelp;
}

LRESULT CConEmuMain::OnCreate(HWND hWnd, LPCREATESTRUCT lpCreate)
{
	ghWnd = hWnd; // ставим сразу, чтобы функции могли пользоваться

	//DWORD_PTR dwStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
	//if (gpSet->isHideCaptionAlways) {
	//	if ((dwStyle & (WS_CAPTION|WS_THICKFRAME)) != 0) {
	//		lpCreate->style &= ~(WS_CAPTION|WS_THICKFRAME);
	//		dwStyle = lpCreate->style;
	//		SetWindowLongPtr(hWnd, GWL_STYLE, dwStyle);
	//	}
	//}

	if (!mrc_Ideal.right)
	{
		// lpCreate->cx/cy может содержать CW_USEDEFAULT
		GetWindowRect(ghWnd, &mrc_Ideal);
	}

	Icon.LoadIcon(hWnd, gpSet->nIconID/*IDI_ICON1*/);
	// Позволяет реагировать на запросы FlashWindow из фара и запуск приложений
	HMODULE hUser32 = GetModuleHandle(L"user32.dll");
	FRegisterShellHookWindow fnRegisterShellHookWindow = NULL;

	if (hUser32) fnRegisterShellHookWindow = (FRegisterShellHookWindow)GetProcAddress(hUser32, "RegisterShellHookWindow");

	if (fnRegisterShellHookWindow) fnRegisterShellHookWindow(hWnd);

	// Чтобы можно было найти хэндл окна по хэндлу консоли
	SetWindowLongPtr(ghWnd, GWLP_USERDATA, (LONG_PTR)ghConWnd); // 31.03.2009 Maximus - только нихрена оно еще не создано!
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
	GetSystemMenu(TRUE);

	//HMENU hwndMain = GetSystemMenu(ghWnd, FALSE), hDebug = NULL;
	//InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_TOMONITOR, _T("Bring &here"));
	//InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_TOTRAY, TRAY_ITEM_HIDE_NAME/* L"Hide to &TSA" */);
	//InsertMenu(hwndMain, 0, MF_BYPOSITION, MF_SEPARATOR, 0);
	//InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_ABOUT, _T("&About"));

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
	DWORD_PTR dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE);
#endif

	if (gpSet->isTabs==1)  // "Табы всегда"
		ForceShowTabs(TRUE); // Показать табы

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

wchar_t* CConEmuMain::LoadConsoleBatch(LPCWSTR asSource)
{
	wchar_t* pszDataW = NULL;

	if (*asSource == CmdFilePrefix)
	{
		// В качестве "команды" указан "пакетный файл" одновременного запуска нескольких консолей
		HANDLE hFile = CreateFile(asSource+1, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

		if (!hFile || hFile == INVALID_HANDLE_VALUE)
		{
			DWORD dwErr = GetLastError();
			wchar_t szCurDir[MAX_PATH*2]; szCurDir[0] = 0; GetCurrentDirectory(countof(szCurDir), szCurDir);
			size_t cchMax = _tcslen(asSource)+100+_tcslen(szCurDir);
			wchar_t* pszErrMsg = (wchar_t*)calloc(cchMax,2);
			_wcscpy_c(pszErrMsg, cchMax, L"Can't open console batch file:\n\xAB"/*«*/);
			_wcscat_c(pszErrMsg, cchMax, asSource+1);
			_wcscat_c(pszErrMsg, cchMax, L"\xBB"/*»*/ L"\nCurrent directory:\n\xAB"/*«*/);
			_wcscat_c(pszErrMsg, cchMax, szCurDir);
			_wcscat_c(pszErrMsg, cchMax, L"\xBB"/*»*/);
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
			lstrcpy(pszErrMsg, L"Console batch file is too large or empty:\n\xAB"/*«*/); lstrcat(pszErrMsg, asSource+1); lstrcat(pszErrMsg, L"\xBB"/*»*/);
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
			lstrcpy(pszErrMsg, L"Reading console batch file failed:\n\xAB"/*«*/); lstrcat(pszErrMsg, asSource+1); lstrcat(pszErrMsg, L"\xBB"/*»*/);
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
	else if ((*asSource == TaskBracketLeft) || (lstrcmp(asSource, AutoStartTaskName) == 0))
	{
		wchar_t szName[MAX_PATH]; lstrcpyn(szName, asSource, countof(szName));
		wchar_t* psz = wcschr(szName, TaskBracketRight);
		if (psz) psz[1] = 0;

		const Settings::CommandTasks* pGrp;
		for (int i = 0; (pGrp = gpSet->CmdTaskGet(i)) != NULL; i++)
		{
			if (lstrcmpi(pGrp->pszName, szName) == 0)
			{
				pszDataW = lstrdup(pGrp->pszCommands);
				break;
			}
		}

		if (!pszDataW)
		{
			wchar_t* pszErrMsg = (wchar_t*)calloc(_tcslen(szName)+100,2);
			lstrcpy(pszErrMsg, L"Command group "); lstrcat(pszErrMsg, szName); lstrcat(pszErrMsg, L" not found");
			DisplayLastError(pszErrMsg, -1);
			return NULL;
		}

	}
	else
	{
		_ASSERTE(*asSource==CmdFilePrefix || *asSource==TaskBracketLeft);
	}

	return pszDataW;
}

void CConEmuMain::PostCreate(BOOL abRecieved/*=FALSE*/)
{
	// First ShowWindow forced to use nCmdShow. This may be weird...
	SkipOneShowWindow();

	if (!abRecieved)
	{
		//if (gpConEmu->WindowMode == rFullScreen || gpConEmu->WindowMode == rMaximized) {
#ifdef MSGLOGGER
		WINDOWPLACEMENT wpl; memset(&wpl, 0, sizeof(wpl)); wpl.length = sizeof(wpl);
		GetWindowPlacement(ghWnd, &wpl);
#endif

		if (gpSet->isHideCaptionAlways())
		{
			OnHideCaption();
		}

		if (gpSet->isDesktopMode)
			OnDesktopMode();

		SetWindowMode(WindowMode);

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
	else
	{
		HWND hCurForeground = GetForegroundWindow();

		//-- Перенесено в Taskbar_Init();
		//HRESULT hr = S_OK;
		//hr = OleInitialize(NULL);  // как бы попробовать включать Ole только во время драга. кажется что из-за него глючит переключалка языка
		//CoInitializeEx(NULL, COINIT_MULTITHREADED);

		// Может быть уже вызван в SkipOneShowWindow, а может и нет
		Taskbar_Init();

		if (gpSet->IsHotkey(vkMinimizeRestore))
			RegisterMinRestore(true);

		RegisterHotKeys();

		//TODO: Возможно, стоит отложить запуск секунд на 5, чтобы не мешать инициализации?
		if (gpSet->UpdSet.isUpdateCheckOnStartup && !DisableAutoUpdate)
			CheckUpdates(FALSE); // Не показывать сообщение "You are using latest available version"

		//SetWindowRgn(ghWnd, CreateWindowRgn(), TRUE);

		if (gpSetCls->szFontError[0])
		{
			MBoxA(gpSetCls->szFontError);
			gpSetCls->szFontError[0] = 0;
		}

		if (!gpSetCls->CheckConsoleFontFast())
		{
			gbDontEnable = TRUE;
			gpSetCls->EditConsoleFont(ghWnd);
			gbDontEnable = FALSE;
		}
		
		// Если в ключе [HKEY_CURRENT_USER\Console] будут левые значения - то в Win7 могут
		// начаться страшные глюки :-) 
		// например, консольное окно будет "дырявое" - рамка есть, а содержимого - нет :-P
		gpSet->CheckConsoleSettings();

		if (gpSet->isShowBgImage)
			gpSetCls->LoadBackgroundFile(gpSet->sBgImage);

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

		if (mp_VActive == NULL || !gpConEmu->mb_StartDetached)  // Консоль уже может быть создана, если пришел Attach из ConEmuC
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
			LPCWSTR pszCmd = gpSet->GetCmd();

			if ((*pszCmd == CmdFilePrefix || *pszCmd == TaskBracketLeft) && !gpConEmu->mb_StartDetached)
			{
				// В качестве "команды" указан "пакетный файл" или "группа команд" одновременного запуска нескольких консолей
				wchar_t* pszDataW = LoadConsoleBatch(pszCmd);
				if (!pszDataW)
				{
					Destroy();
					return;
				}

				// GO
				if (!CreateConGroup(pszDataW))
				{
					Destroy();
					return;
				}

				SafeFree(pszDataW);

				// Если ConEmu был запущен с ключом "/single /cmd xxx" то после окончания
				// загрузки - сбросить команду, которая пришла из "/cmd" - загрузить настройку
				if (gpSetCls->SingleInstanceArg)
				{
					gpSetCls->ResetCmdArg();
				}

				lbCreated = TRUE;
			}

			if (!lbCreated)
			{
				RConStartArgs args;
				args.bDetached = gpConEmu->mb_StartDetached;

				if (!args.bDetached)
					args.pszSpecialCmd = lstrdup(gpSet->GetCmd());

				if (!CreateCon(&args))
				{
					DisplayLastError(L"Can't create new virtual console!");
					Destroy();
					return;
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
		if ((hCurForeground == ghWnd) && !WindowStartMinimized)
		{
			apiSetForegroundWindow(ghWnd);
		}

		if (WindowStartMinimized)
		{
			if (IsWindowVisible(ghWnd) && !isIconic())
				PostMessage(ghWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
			WindowStartMinimized = false;
		}

		//RegisterHotKeys();
		//SetParent(GetParent(GetShellWindow()));
		UINT n = SetTimer(ghWnd, TIMER_MAIN_ID, 500/*gpSet->nMainTimerElapse*/, NULL);
		#ifdef _DEBUG
		DWORD dw = GetLastError();
		#endif
		n = SetTimer(ghWnd, TIMER_CONREDRAW_ID, CON_REDRAW_TIMOUT*2, NULL);
		UNREFERENCED_PARAMETER(n);
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
	if (mp_AttachDlg)
	{
		mp_AttachDlg->Close();
	}
	if (mp_RecreateDlg)
	{
		mp_RecreateDlg->Close();
	}
	
	gpSet->SaveSizePosOnExit();
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
	m_GuiServer.Stop();


	for (size_t i = 0; i < countof(mp_VCon); i++)
	{
		if (mp_VCon[i])
		{
			CVirtualConsole* p = mp_VCon[i];
			mp_VCon[i] = NULL;
			p->Release();
		}
	}

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
	Icon.RemoveTrayIcon();

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

	UnRegisterHotKeys(TRUE);
	//if (mh_DwmApi && mh_DwmApi != INVALID_HANDLE_VALUE)
	//{
	//	FreeLibrary(mh_DwmApi); mh_DwmApi = NULL;
	//	DwmIsCompositionEnabled = NULL;
	//}
	KillTimer(ghWnd, TIMER_MAIN_ID);
	KillTimer(ghWnd, TIMER_CONREDRAW_ID);
	KillTimer(ghWnd, TIMER_CAPTION_APPEAR_ID);
	KillTimer(ghWnd, TIMER_CAPTION_DISAPPEAR_ID);
	PostQuitMessage(0);
	return 0;
}

LRESULT CConEmuMain::OnFlashWindow(DWORD nFlags, DWORD nCount, HWND hCon)
{
	if (!hCon) return 0;

	bool lbFlashSimple = false;

	// Достало. Настройка полного отключения флэшинга
	if (gpSet->isDisableFarFlashing && mp_VActive->RCon()->GetFarPID(FALSE))
	{
		if (gpSet->isDisableFarFlashing == 1)
			return 0;
		else
			lbFlashSimple = true;
	}
	else if (gpSet->isDisableAllFlashing)
	{
		if (gpSet->isDisableAllFlashing == 1)
			return 0;
		else
			lbFlashSimple = true;
	}

	LRESULT lbRc = FALSE;

	for (size_t i = 0; i < countof(mp_VCon); i++)
	{
		if (!mp_VCon[i]) continue;

		if (mp_VCon[i]->RCon()->ConWnd() == hCon)
		{
			FLASHWINFO fl = {sizeof(FLASHWINFO)};

			if (isMeForeground())
			{
				if (mp_VCon[i] != mp_VActive)    // Только для неактивной консоли
				{
					fl.dwFlags = FLASHW_STOP; fl.hwnd = ghWnd;
					FlashWindowEx(&fl); // Чтобы мигание не накапливалось
					fl.uCount = 3; fl.dwFlags = lbFlashSimple ? FLASHW_ALL : FLASHW_TRAY; fl.hwnd = ghWnd;
					FlashWindowEx(&fl);
				}
			}
			else
			{
				if (lbFlashSimple)
				{
					fl.uCount = 3; fl.dwFlags = FLASHW_TRAY;
				}
				else
				{
					fl.dwFlags = FLASHW_ALL|FLASHW_TIMERNOFG;
				}

				fl.hwnd = ghWnd;
				FlashWindowEx(&fl); // Помигать в GUI
			}

			//fl.dwFlags = FLASHW_STOP; fl.hwnd = hCon; -- не требуется, т.к. это хучится
			//FlashWindowEx(&fl);
			break;
		}
	}

	return lbRc;
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
#endif
	SetFocus(ghWnd);
}

void CConEmuMain::SetSkipOnFocus(BOOL abSkipOnFocus)
{
	mb_SkipOnFocus = abSkipOnFocus;
}

LRESULT CConEmuMain::OnFocus(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	// Чтобы избежать лишних вызовов по CtrlWinAltSpace при работе с GUI приложением
	if (mb_SkipOnFocus)
	{
		return 0;
	}

	BOOL lbSetFocus = FALSE;
#ifdef _DEBUG
	WCHAR szDbg[128];
#endif
	LPCWSTR pszMsgName = L"Unknown";
	HWND hNewFocus = NULL;

	if (messg == WM_SETFOCUS)
	{
		lbSetFocus = TRUE;
#ifdef _DEBUG
		_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_SETFOCUS(From=0x%08X)\n", (DWORD)wParam);
		DEBUGSTRFOCUS(szDbg);
#endif
		pszMsgName = L"WM_SETFOCUS";
	}
	else if (messg == WM_ACTIVATE)
	{
		lbSetFocus = (LOWORD(wParam)==WA_ACTIVE) || (LOWORD(wParam)==WA_CLICKACTIVE);

		if (LOWORD(wParam)==WA_INACTIVE)
			hNewFocus = lParam ? (HWND)lParam : GetForegroundWindow();

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
					while(hFromPoint)
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

		#ifdef _DEBUG
		if (lbSetFocus)
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_ACTIVATE(From=0x%08X)\n", (DWORD)lParam);
		else
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_ACTIVATE.WA_INACTIVE(To=0x%08X)\n", (DWORD)lParam);
		DEBUGSTRFOCUS(szDbg);
		#endif

		pszMsgName = L"WM_ACTIVATE";
	}
	else if (messg == WM_ACTIVATEAPP)
	{
		lbSetFocus = (wParam!=0);
		if (!lbSetFocus)
			hNewFocus = GetForegroundWindow();

		#ifdef _DEBUG
		if (lbSetFocus)
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_ACTIVATEAPP.Activate(FromTID=%i)\n", (DWORD)lParam);
		else
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_ACTIVATEAPP.Deactivate(ToTID=%i)\n", (DWORD)lParam);
		DEBUGSTRFOCUS(szDbg);
		#endif

		pszMsgName = L"WM_ACTIVATEAPP";
	}
	else if (messg == WM_KILLFOCUS)
	{
#ifdef _DEBUG
		_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_KILLFOCUS(To=0x%08X)\n", (DWORD)wParam);
		DEBUGSTRFOCUS(szDbg);
#endif
		pszMsgName = L"WM_KILLFOCUS";
		hNewFocus = wParam ? (HWND)wParam : GetForegroundWindow();
	}

	CheckFocus(pszMsgName);

	HWND hGuiWnd = mp_VActive ? mp_VActive->RCon()->GuiWnd() : NULL; // GUI режим (дочернее окно)

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
#ifdef _DEBUG

				if (hNewFocus != ghWnd)
				{
					_ASSERTE(hNewFocus == ghWnd);
				}
				else
				{
					DEBUGSTRFOCUS(L"Focus was returned to ConEmu\n");
				}

#endif
				lbSetFocus = (hNewFocus == ghWnd);
				break;
			}

			hNewFocus = hParent;
		}
	}

	// GUI client?
	if (!lbSetFocus && hNewFocus)
	{
		DWORD nNewPID = 0; GetWindowThreadProcessId(hNewFocus, &nNewPID);
		if (nNewPID == GetCurrentProcessId())
		{
			lbSetFocus = TRUE;
		}
		else if (mp_VActive)
		{
			if (hNewFocus == mp_VActive->RCon()->ConWnd())
			{
				lbSetFocus = TRUE;
			}
			else
			{
				
				DWORD nRConPID = mp_VActive->RCon()->GetActivePID();
				if (nNewPID == nRConPID)
					lbSetFocus = TRUE;
			}
		}
	}

	if (!lbSetFocus)
	{
		if (mp_VActive)
			FixSingleModifier(0, mp_VActive->RCon());
		mp_TabBar->SwitchRollback();
		UnRegisterHotKeys();
	}
	else if (!mb_HotKeyRegistered)
	{
		RegisterHotKeys();
	}

	if (mp_VActive && mp_VActive->RCon())
		mp_VActive->RCon()->OnGuiFocused(lbSetFocus, (messg == WM_ACTIVATEAPP));

	if (gpSet->isFadeInactive && mp_VActive)
	{
		bool bForeground = lbSetFocus || isMeForeground();
		bool bLastFade = (mp_VActive!=NULL) ? mp_VActive->mb_LastFadeFlag : false;
		bool bNewFade = (gpSet->isFadeInactive && !bForeground && !isPictureView());

		if (bLastFade != bNewFade)
		{
			if (mp_VActive) mp_VActive->mb_LastFadeFlag = bNewFade;

			mp_VActive->Invalidate();
		}
	}

	if (lbSetFocus && mp_VActive && gpSet->isTabsOnTaskBar())
	{
		HWND hFore = GetForegroundWindow();
		if (hFore == ghWnd)
		{
			if (!mb_PostTaskbarActivate)
			{
				//mp_VActive->OnTaskbarFocus();
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

	if (mp_VActive /*&& (messg == WM_SETFOCUS || messg == WM_KILLFOCUS)*/)
	{
		mp_VActive->RCon()->OnFocus(lbSetFocus);
	}

	return 0;
}

LRESULT CConEmuMain::OnGetMinMaxInfo(LPMINMAXINFO pInfo)
{
	LRESULT result = 0;
	//RECT rcFrame = CalcMargins(CEM_FRAME);
	//POINT p = cwShift;
	//RECT shiftRect = ConsoleOffsetRect();
	//RECT shiftRect = ConsoleOffsetRect();
#ifdef _DEBUG
	wchar_t szMinMax[255];
	_wsprintf(szMinMax, SKIPLEN(countof(szMinMax)) L"OnGetMinMaxInfo[before] MaxSize={%i,%i}, MaxPos={%i,%i}, MinTrack={%i,%i}, MaxTrack={%i,%i}\n",
	          pInfo->ptMaxSize.x, pInfo->ptMaxSize.y,
	          pInfo->ptMaxPosition.x, pInfo->ptMaxPosition.y,
	          pInfo->ptMinTrackSize.x, pInfo->ptMinTrackSize.y,
	          pInfo->ptMaxTrackSize.x, pInfo->ptMaxTrackSize.y);
	DEBUGSTRSIZE(szMinMax);
#endif
	// Минимально допустимые размеры консоли
	//COORD srctWindow; srctWindow.X=28; srctWindow.Y=9;
	RECT rcFrame = CalcRect(CER_MAIN, MakeRect(28,9), CER_CONSOLE);
	pInfo->ptMinTrackSize.x = rcFrame.right;
	pInfo->ptMinTrackSize.y = rcFrame.bottom;

	//pInfo->ptMinTrackSize.x = srctWindow.X * (gpSet->Log Font.lfWidth ? gpSet->Log Font.lfWidth : 4)
	//  + p.x + shiftRect.left + shiftRect.right;

	//pInfo->ptMinTrackSize.y = srctWindow.Y * (gpSet->Log Font.lfHeight ? gpSet->Log Font.lfHeight : 6)
	//  + p.y + shiftRect.top + shiftRect.bottom;

	if (mb_isFullScreen)
	{
		if (pInfo->ptMaxTrackSize.x < ptFullScreenSize.x)
			pInfo->ptMaxTrackSize.x = ptFullScreenSize.x;

		if (pInfo->ptMaxTrackSize.y < ptFullScreenSize.y)
			pInfo->ptMaxTrackSize.y = ptFullScreenSize.y;

		if (pInfo->ptMaxSize.x < ptFullScreenSize.x)
			pInfo->ptMaxSize.x = ptFullScreenSize.x;

		if (pInfo->ptMaxSize.y < ptFullScreenSize.y)
			pInfo->ptMaxSize.y = ptFullScreenSize.y;
	}

	if (gpSet->isHideCaption && !gpSet->isHideCaptionAlways())
	{
		int yCapt = GetSystemMetrics(SM_CYCAPTION);
		pInfo->ptMaxPosition.y -= yCapt;
		//if (!mb_isFullScreen)
		//	pInfo->ptMaxSize.y += yCapt;
		//HMONITOR hMon = MonitorFromWindow(ghWnd, MONITOR_DEFAULTTOPRIMARY);
		//MONITORINFO mi = {sizeof(mi)};
		//GetMonitorInfo(hMon, &mi);
		if (pInfo->ptMaxTrackSize.y < (pInfo->ptMaxSize.y + yCapt))
			pInfo->ptMaxTrackSize.y = (pInfo->ptMaxSize.y + yCapt);
	}

#ifdef _DEBUG
	_wsprintf(szMinMax, SKIPLEN(countof(szMinMax)) L"OnGetMinMaxInfo[after]  MaxSize={%i,%i}, MaxPos={%i,%i}, MinTrack={%i,%i}, MaxTrack={%i,%i}\n",
	          pInfo->ptMaxSize.x, pInfo->ptMaxSize.y,
	          pInfo->ptMaxPosition.x, pInfo->ptMaxPosition.y,
	          pInfo->ptMinTrackSize.x, pInfo->ptMinTrackSize.y,
	          pInfo->ptMaxTrackSize.x, pInfo->ptMaxTrackSize.y);
	DEBUGSTRSIZE(szMinMax);
#endif
	return result;
}

void CConEmuMain::OnHideCaption()
{
	mp_TabBar->OnCaptionHidden();

	if (isZoomed())
	{
		SetWindowMode(rMaximized, TRUE);
	}

	UpdateWindowRgn();
	//DWORD_PTR dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE);
	//if (gpSet->isHideCaptionAlways)
	//	dwStyle &= ~(WS_CAPTION/*|WS_THICKFRAME*/);
	//else
	//	dwStyle |= (WS_CAPTION|/*WS_THICKFRAME|*/WS_MINIMIZEBOX|WS_MAXIMIZEBOX);
	//mb_SkipSyncSize = TRUE;
	//SetWindowLongPtr(ghWnd, GWL_STYLE, dwStyle);
	//OnSize(-1);
	//mp_TabBar->OnCaptionHidden();
	//mb_SkipSyncSize = FALSE;
	//if (!mb_isFullScreen && !isZoomed() && !isIconic()) {
	//	RECT rcCon = MakeRect(gpSet->wndWidth,gpSet->wndHeight);
	//	RECT rcWnd = CalcRect(CER_MAIN, rcCon, CER_CONSOLE); // размеры окна
	//	RECT wndR; GetWindowRect(ghWnd, &wndR); // текущий XY
	//	if (gpSetCls->isAdvLogging) {
	//		char szInfo[128]; wsprintfA(szInfo, "OnHideCaption(Cols=%i, Rows=%i)", gpSet->wndWidth,gpSet->wndHeight);
	//		mp_VActive->RCon()->LogString(szInfo);
	//	}
	//	MOVEWINDOW ( ghWnd, wndR.left, wndR.top, rcWnd.right, rcWnd.bottom, 1);
	//	GetWindowRect(ghWnd, &mrc_Ideal);
	//}
}

void CConEmuMain::OnGlobalSettingsChanged()
{
	gpConEmu->UpdateGuiInfoMapping();

	// и отослать в серверы
	for (size_t i = 0; i < countof(mp_VCon); i++)
	{
		if (mp_VCon[i] && mp_VCon[i]->RCon())
		{
			mp_VCon[i]->RCon()->UpdateGuiInfoMapping(&m_GuiInfo);
		}
	}
	
	// И фары тоже уведомить
	gpConEmu->UpdateFarSettings();
}

void CConEmuMain::OnPanelViewSettingsChanged(BOOL abSendChanges/*=TRUE*/)
{
	gpConEmu->UpdateGuiInfoMapping();
	
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
		for (size_t i = 0; i < countof(mp_VCon); i++)
		{
			if (mp_VCon[i])
			{
				mp_VCon[i]->OnPanelViewSettingsChanged();
			}
		}
	}
}

void CConEmuMain::OnTaskbarSettingsChanged()
{
	for (size_t i = 0; i < countof(mp_VCon); i++)
	{
		if (mp_VCon[i])
			mp_VCon[i]->OnTaskbarSettingsChanged();
	}

	if (!gpSet->isTabsOnTaskBar())
		OnAllGhostClosed();

	apiSetForegroundWindow(ghOpWnd ? ghOpWnd : ghWnd);
}

void CConEmuMain::OnAltEnter()
{
	if (!mb_isFullScreen)
		gpConEmu->SetWindowMode(rFullScreen);
	else if (gpSet->isDesktopMode && (mb_isFullScreen || gpConEmu->isZoomed()))
		gpConEmu->SetWindowMode(rNormal);
	else
		gpConEmu->SetWindowMode(gpConEmu->isWndNotFSMaximized ? rMaximized : rNormal);
}

void CConEmuMain::OnAltF9(BOOL abPosted/*=FALSE*/)
{
	if (!abPosted)
	{
		PostMessage(ghWnd, gpConEmu->mn_MsgPostAltF9, 0, 0);
		return;
	}

	gpConEmu->SetWindowMode((gpConEmu->isZoomed()||(mb_isFullScreen/*&&gpConEmu->isWndNotFSMaximized*/)) ? rNormal : rMaximized);
}

void CConEmuMain::OnMinimizeRestore()
{
	if (isMeForeground())
	{
		PostMessage(ghWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
	}
	else
	{
		if (isIconic())
		{
			if (Icon.isWindowInTray() || !IsWindowVisible(ghWnd))
				Icon.RestoreWindowFromTray();
			else
				PostMessage(ghWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
		}
		else
		{
			PostMessage(ghWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
		}
	}
}

LRESULT CConEmuMain::OnKeyboard(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
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

#if 1
	// Works fine, but need to cache last pressed key, cause of need them in WM_KEYUP (send char to console)
	wchar_t szTranslatedChars[16] = {0};
	int nTranslatedChars = 0;

	if (messg == WM_KEYDOWN || messg == WM_SYSKEYDOWN)
	{
		_ASSERTE(sizeof(szTranslatedChars) == sizeof(m_TranslatedChars[0].szTranslatedChars));
		BOOL lbDeadChar = FALSE;
		MSG msg, msg1;

		while (nTranslatedChars < 15  // извлечь из буфера все последующие WM_CHAR & WM_SYSCHAR
		        && PeekMessage(&msg, 0,0,0, PM_NOREMOVE)
		      )
		{
			if (!(msg.message == WM_CHAR || msg.message == WM_SYSCHAR || msg.message == WM_DEADCHAR)
			        || (msg.lParam & 0xFF0000) != (lParam & 0xFF0000) /* совпадение скан-кода */)
				break;

			if (GetMessage(&msg1, 0,0,0))  // убрать из буфера
			{
				_ASSERTE(msg1.message == msg.message && msg1.wParam == msg.wParam && msg1.lParam == msg.lParam);

				if (msg.message != WM_DEADCHAR)
					szTranslatedChars[nTranslatedChars ++] = (wchar_t)msg1.wParam;
				else
					lbDeadChar = TRUE;

				// Требуется обработать сообщение
				DispatchMessage(&msg1);
			}
		}

		if (lbDeadChar && nTranslatedChars)
			lbDeadChar = FALSE;

		memmove(m_TranslatedChars[bVK].szTranslatedChars, szTranslatedChars, sizeof(szTranslatedChars));
	}
	else
	{
		szTranslatedChars[0] = m_TranslatedChars[bVK].szTranslatedChars[0];
		szTranslatedChars[1] = 0;
		nTranslatedChars = (szTranslatedChars[0] == 0) ? 0 : 1;
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
		_ASSERTE(FALSE);
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
		_ASSERTE(FALSE);
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

	_ASSERTE(messg != WM_CHAR && messg != WM_SYSCHAR);

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

	// Теперь - можно переслать в консоль
	if (mp_VActive && mp_VActive->RCon())
	{
		mp_VActive->RCon()->OnKeyboard(hWnd, messg, wParam, lParam, szTranslatedChars);
	}
	else if (((wParam & 0xFF) >= VK_WHEEL_FIRST) && ((wParam & 0xFF) <= VK_WHEEL_LAST))
	{
		// Такие коды с клавиатуры приходить не должны, а то для "мышки" ничего не останется
		_ASSERTE(!(((wParam & 0xFF) >= VK_WHEEL_FIRST) && ((wParam & 0xFF) <= VK_WHEEL_LAST)));
	}
	else
	{
		// Если вдруг активной консоли нету (вообще?) но клавиши обработать все-равно нада
		ProcessHotKeyMsg(messg, wParam, lParam, szTranslatedChars, NULL);
	}

	return 0;
}

LRESULT CConEmuMain::OnKeyboardHook(DWORD VkMod)
{
	if (!VkMod)
		return 0;

	CRealConsole* pRCon = mp_VActive ? mp_VActive->RCon() : NULL;
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
			OnAltEnter();
			return;
		}
	}

	// Все остальное - обычным образом
	CRealConsole* pRCon = ActiveCon() ? ActiveCon()->RCon() : NULL;
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

LRESULT CConEmuMain::OnKeyboardIme(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	bool lbProcessed = false;
	wchar_t szDbgMsg[255];
	switch (messg)
	{
		case WM_IME_CHAR:
			_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"WM_IME_CHAR: char=%c, wParam=%u, lParam=0x%08X\n", (wchar_t)wParam, (DWORD)wParam, (DWORD)lParam);
			DEBUGSTRIME(szDbgMsg);
			if (mp_VActive)
			{
				mp_VActive->RCon()->OnKeyboardIme(hWnd, messg, wParam, lParam);
			}
			break;
		case WM_IME_COMPOSITION:
			_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"WM_IME_COMPOSITION: wParam=0x%08X, lParam=0x%08X\n", (DWORD)wParam, (DWORD)lParam);
			DEBUGSTRIME(szDbgMsg);
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
			//	if (mp_VActive && mp_VActive->RCon())
			//	{
			//		COORD crCur = {};
			//		mp_VActive->RCon()->GetConsoleCursorPos(&crCur);
			//		p->pt = mp_VActive->ConsoleToClient(crCur.X, crCur.Y);
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
		mp_VActive->RCon()->LogString(szInfo);
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

	for(i = 0; i < (int)countof(m_LayoutNames); i++)
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
		m_LayoutNames[iUnused].bUsed = TRUE;
		m_LayoutNames[iUnused].klName = dwLayoutAfterChange;
		m_LayoutNames[iUnused].hkl = lParam;
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

		CRealConsole* pRCon = mp_VActive->RCon();

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
LRESULT CConEmuMain::OnLangChangeConsole(CVirtualConsole *apVCon, DWORD dwLayoutName)
{
	if ((gpSet->isMonitorConsoleLang & 1) != 1)
		return 0;

	if (!isValid(apVCon))
		return 0;

	if (!isMainThread())
	{
		if (gpSetCls->isAdvLogging > 1)
		{
			WCHAR szInfo[255];
			_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"CConEmuMain::OnLangChangeConsole (0x%08X), Posting to main thread", dwLayoutName);
			mp_VActive->RCon()->LogString(szInfo);
		}

		PostMessage(ghWnd, mn_ConsoleLangChanged, dwLayoutName, (LPARAM)apVCon);
		return 0;
	}
	else
	{
		if (gpSetCls->isAdvLogging > 1)
		{
			WCHAR szInfo[255];
			_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"CConEmuMain::OnLangChangeConsole (0x%08X), MainThread", dwLayoutName);
			mp_VActive->RCon()->LogString(szInfo);
		}
	}

#ifdef _DEBUG
	//Sleep(2000);
	WCHAR szMsg[255];
	// --> Видимо именно это не "нравится" руслату. Нужно переправить Post'ом в основную нить
	HKL hkl = GetKeyboardLayout(0);
	_wsprintf(szMsg, SKIPLEN(countof(szMsg)) L"ConEmu: GetKeyboardLayout(0) in OnLangChangeConsole after GetKeyboardLayout(0) = 0x%08I64X\n",
	          (unsigned __int64)(DWORD_PTR)hkl);
	DEBUGSTRLANG(szMsg);
	//Sleep(2000);
#endif
	wchar_t szName[10]; _wsprintf(szName, SKIPLEN(countof(szName)) L"%08X", dwLayoutName);
#ifdef _DEBUG
	DEBUGSTRLANG(szName);
#endif
	// --> Тут слетает!
	DWORD_PTR dwNewKeybLayout = dwLayoutName; //(DWORD_PTR)LoadKeyboardLayout(szName, 0);
	HKL hKeyb[20]; UINT nCount, i;
	nCount = GetKeyboardLayoutList(countof(hKeyb), hKeyb);
	/*
	HKL:
	0x0000000004090409 - US
	0x0000000004190419 - Russian
	0xfffffffff0020409 - US - Dvorak
	0xfffffffff01a0409 - US - Dvorak left hand
	0xfffffffff01b0409 - US - Dvorak right hand
	Layout (dwLayoutName):
	0x00010409 - US - Dvorak
	0x00030409 - US - Dvorak left hand
	0x00040409 - US - Dvorak right hand
	*/
	BOOL lbFound = FALSE;
	int iUnused = -1;

	for(i = 0; !lbFound && i < countof(m_LayoutNames); i++)
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

	// Если не нашли, и это "классическая" раскладка, в которой ид раскладки совпадает с языком
	if (!lbFound && ((dwLayoutName & 0xFFFF) == dwLayoutName))
	{
		DWORD_PTR dwTest = dwNewKeybLayout | (dwNewKeybLayout << 16);

		for(i = 0; !lbFound && i < nCount; i++)
		{
			if (((DWORD_PTR)hKeyb[i]) == dwTest)
			{
				lbFound = TRUE;
				dwNewKeybLayout = (DWORD_PTR)hKeyb[i];
				break;
			}
		}
	}

	if (!lbFound)
	{
		wchar_t szLayoutName[9] = {0};
		_wsprintf(szLayoutName, SKIPLEN(countof(szLayoutName)) L"%08X", dwLayoutName);

		if (gpSetCls->isAdvLogging > 1)
		{
			WCHAR szInfo[255];
			_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"CConEmuMain::OnLangChangeConsole -> LoadKeyboardLayout(0x%08X)", dwLayoutName);
			mp_VActive->RCon()->LogString(szInfo);
		}

		dwNewKeybLayout = (DWORD_PTR)LoadKeyboardLayout(szLayoutName, 0);

		if (gpSetCls->isAdvLogging > 1)
		{
			WCHAR szInfo[255];
			_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"CConEmuMain::OnLangChangeConsole -> LoadKeyboardLayout()=0x%08X", (DWORD)dwNewKeybLayout);
			mp_VActive->RCon()->LogString(szInfo);
		}

		lbFound = TRUE;
	}

	if (lbFound && iUnused != -1)
	{
		m_LayoutNames[iUnused].bUsed = TRUE;
		m_LayoutNames[iUnused].klName = dwLayoutName;
		m_LayoutNames[iUnused].hkl = dwNewKeybLayout;
	}

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

bool CConEmuMain::PatchMouseEvent(UINT messg, POINT& ptCurClient, POINT& ptCurScreen, WPARAM wParam, bool& isPrivate)
{
	isPrivate = false;

	// Для этих сообщений, lParam - relative to the upper-left corner of the screen.
	if (messg == WM_MOUSEWHEEL || messg == WM_MOUSEHWHEEL)
		ScreenToClient(ghWnd, &ptCurClient);
	else // Для остальных lParam содержит клиентские координаты
		ClientToScreen(ghWnd, &ptCurScreen);

	if (mb_MouseCaptured || (messg == WM_LBUTTONDOWN))
	{
		HWND hChild = ::ChildWindowFromPointEx(ghWnd, ptCurClient, CWP_SKIPINVISIBLE|CWP_SKIPDISABLED|CWP_SKIPTRANSPARENT);
		if (hChild && (hChild != ghWnd)) // Это должна быть VCon
		{
			#ifdef _DEBUG
			wchar_t szClass[128]; GetClassName(hChild, szClass, countof(szClass));
			_ASSERTE(lstrcmp(szClass, VirtualConsoleClass)==0 && "This must be VCon DC window");
			#endif

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

	if (mn_TrackMenuPlace != tmp_None && mp_Tip)
	{
		mp_Tip->HideTip();
	}

	//2010-05-20 все-таки будем ориентироваться на lParam, потому что
	//  только так ConEmuTh может передать корректные координаты
	//POINT ptCur = {-1, -1}; GetCursorPos(&ptCur);
	POINT ptCurClient = {(int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam)};
	POINT ptCurScreen = ptCurClient;

	// Коррекция координат или пропуск сообщений
	bool isPrivate = false;
	bool bSkipEvent = PatchMouseEvent(messg, ptCurClient, ptCurScreen, wParam, isPrivate);

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
		bSkipEvent ? L"" : L" - SKIPPED!");
	DEBUGSTRMOUSE(szDbg);
#endif

	if (!bSkipEvent)
		return 0;

	TODO("DoubleView. Хорошо бы колесико мышки перенаправлять в консоль под мышиным курором, а не в активную");
	RECT conRect = {0}, dcRect = {0};
	//GetWindowRect('ghWnd DC', &dcRect);
	CVirtualConsole* pVCon = GetVConFromPoint(ptCurScreen);
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

	if (messg == WM_MOUSEMOVE)
	{
		if (TrackMouse())
			return 0;
	}

	if (pRCon && pRCon->GuiWnd() && !pRCon->isBufferHeight())
	{
		//pRCon->PostConsoleMessage(pRCon->GuiWnd(), messg, wParam, lParam);
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
	WARNING("Избавляться от ghConWnd. Должно быть обращение через классы");
	::GetClientRect(ghConWnd, &conRect);
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
	if (gpSet->isMouseSkipMoving && GetForegroundWindow() != ghWnd)
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
				switch(mouse.nReplaceDblClk)
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

	if (!isFar())
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

	gpSetCls->UpdateFindDlgAlpha();

	// мог не сброситься, проверим
	if (isSizing())
	{
		if (!isPressed(VK_LBUTTON))
			mouse.state &= ~MOUSE_SIZING_BEGIN;
	}

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
					pVCon->RCon()->LogString(szLog);
				}

				BOOL lbLeftDrag = (mouse.state & DRAG_L_ALLOWED) == DRAG_L_ALLOWED;
				pVCon->RCon()->LogString(lbLeftDrag ? "Left drag about to start" : "Right drag about to start");
				// Если сначала фокус был на файловой панели, но после LClick он попал на НЕ файловую - отменить ShellDrag
				bool bFilePanel = isFilePanel();

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
			pVCon->RCon()->LogString(szLog);
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

	if (!isConSelectMode() && isFilePanel() && pVCon &&
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

		if (dpb == DPB_SPLIT)
		{
			RECT rcRight; pRCon->GetPanelRect(TRUE, &rcRight, TRUE);
			int  nCenter = pRCon->TextWidth() / 2;

			if (nCenter < rcRight.left)
				_wsprintf(szMacro, SKIPLEN(countof(szMacro)) L"@$Rep (%i) CtrlLeft $End", rcRight.left - nCenter);
			else if (nCenter > rcRight.left)
				_wsprintf(szMacro, SKIPLEN(countof(szMacro)) L"@$Rep (%i) CtrlRight $End", nCenter - rcRight.left);
		}
		else
		{
			_wsprintf(szMacro, SKIPLEN(countof(szMacro)) L"@$Rep (%i) CtrlDown $End", pRCon->TextHeight());
		}

		if (szMacro[0])
			PostMacro(szMacro);

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
		pVCon->RCon()->LogString(szLog);
	}

	//if (isFilePanel()) // Maximus5
	bool bSelect = false, bPanel = false, bActive = false, bCoord = false;

	if (!(bSelect = isConSelectMode())
	        && (bPanel = isFilePanel())
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

				if (gpSetCls->isAdvLogging) pVCon->RCon()->LogString("RightClick ignored of gpSet->nRDragKey pressed");

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
			pVCon->RCon()->LogString(szLog);
			mouse.RClkTick = TimeGetTime(); //GetTickCount();
			StartRightClickingPaint();
			return 0;
		}
		else
		{
			if (gpSetCls->isAdvLogging)
				pVCon->RCon()->LogString(
					gpSet->isDisableMouse ? "RightClick ignored of gpSet->isDisableMouse" :
				    !gpSet->isRClickSendKey ? "RightClick ignored of !gpSet->isRClickSendKey" :
				    "RightClick ignored of wParam&(MK_CONTROL|MK_LBUTTON|MK_MBUTTON|MK_SHIFT|MK_XBUTTON1|MK_XBUTTON2)"
				);
		}
	}
	else
	{
		if (gpSetCls->isAdvLogging)
			pVCon->RCon()->LogString(
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

				//// Сначала выделить файл под курсором
				////POSTMESSAGE(ghConWnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);
				//pVCon->RCon()->OnMouse(WM_LBUTTONDOWN, MK_LBUTTON, mouse.RClkDC.X, mouse.RClkDC.Y);
				////POSTMESSAGE(ghConWnd, WM_LBUTTONUP, 0, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);
				//pVCon->RCon()->OnMouse(WM_LBUTTONUP, 0, mouse.RClkDC.X, mouse.RClkDC.Y);
				//
				//pVCon->RCon()->FlushInputQueue();
				//pVCon->Update(true);
				//INVALIDATE(); //InvalidateRect(HDCWND, NULL, FALSE);
				// А теперь можно и Apps нажать
				mouse.bSkipRDblClk=true; // чтобы пока FAR думает в консоль не проскочило мышиное сообщение
				//POSTMESSAGE(ghConWnd, WM_KEYDOWN, VK_APPS, 0, TRUE);
				DWORD dwFarPID = pVCon->RCon()->GetFarPID();

				if (dwFarPID)
				{
					AllowSetForegroundWindow(dwFarPID);
					//if (gpSet->sRClickMacro && *gpSet->sRClickMacro) {
					//    //// Сначала выделить файл под курсором
					//    ////POSTMESSAGE(ghConWnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);
					//    //pVCon->RCon()->OnMouse(WM_LBUTTONDOWN, MK_LBUTTON, mouse.RClkDC.X, mouse.RClkDC.Y);
					//    ////POSTMESSAGE(ghConWnd, WM_LBUTTONUP, 0, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);
					//    //pVCon->RCon()->OnMouse(WM_LBUTTONUP, 0, mouse.RClkDC.X, mouse.RClkDC.Y);
					//    //
					//    //pVCon->RCon()->FlushInputQueue();
					//
					//    // Если юзер задал свой макрос - выполняем его
					//    PostMacro(gpSet->sRClickMacro);
					//} else {
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
							pVCon->RCon()->LogString("RightClicked, but pipe.Execute(CMD_EMENU) failed");
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

								pVCon->RCon()->LogString(szInfo);
							}
						}

						DebugStep(NULL);
						free(pcbData);
					}
					else
					{
						pVCon->RCon()->LogString("RightClicked, but pipe.Init() failed");
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
					pVCon->RCon()->LogString("RightClicked, but FAR PID is 0");
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

				pVCon->RCon()->LogString(szLog);
			}
		}
		else
		{
			char szLog[100];
			wsprintfA(szLog, "RightClicked, but mouse was moved abs({%i-%i}-{%i-%i})>%i", Rcursor.x, Rcursor.y, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (int)RCLICKAPPSDELTA);
			pVCon->RCon()->LogString(szLog);
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

		pVCon->RCon()->LogString(szLog);
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
		if (isZoomed() || mb_isFullScreen)
		{
			// Так быть не должно - рамка не должна быть видна в этих режимах
			_ASSERTE(!isZoomed() && !mb_isFullScreen);
			return FALSE;
		}

		RECT rcWnd, rcNewWnd;
		GetWindowRect(ghWnd, &rcWnd);
		rcNewWnd = rcWnd;
		MONITORINFO mon = {sizeof(MONITORINFO)};

		if (wParam == HTLEFT || wParam == HTRIGHT)
		{
			// найти новую левую границу
			POINT pt = {rcWnd.left,((rcWnd.top+rcWnd.bottom)>>2)};
			HMONITOR hMonLeft = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

			if (!GetMonitorInfo(hMonLeft, &mon))
				return FALSE;

			rcNewWnd.left = mon.rcWork.left;
			// найти новую правую границу
			pt.x = rcWnd.right;
			HMONITOR hMonRight = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

			if (hMonRight != hMonLeft)
				if (!GetMonitorInfo(hMonRight, &mon))
					return FALSE;

			rcNewWnd.right = mon.rcWork.right;
			// Скорректировать границы на ширину рамки
			RECT rcFrame = CalcMargins(CEM_FRAME);
			rcNewWnd.left -= rcFrame.left;
			rcNewWnd.right += rcFrame.right;
		}
		else if (wParam == HTTOP || wParam == HTBOTTOM)
		{
			// найти новую верхнюю границу
			POINT pt = {((rcWnd.left+rcWnd.right)>>2),rcWnd.top};
			HMONITOR hMonTop = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

			if (!GetMonitorInfo(hMonTop, &mon))
				return FALSE;

			rcNewWnd.top = mon.rcWork.top;
			// найти новую нижнюю границу
			pt.y = rcWnd.bottom;
			HMONITOR hMonBottom = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

			if (hMonBottom != hMonTop)
				if (!GetMonitorInfo(hMonBottom, &mon))
					return FALSE;

			rcNewWnd.bottom = mon.rcWork.bottom;
			// Скорректировать границы на ширину рамки
			RECT rcFrame = CalcMargins(CEM_FRAME);
			rcNewWnd.top -= rcFrame.bottom; // т.к. в top учтена высота заголовка
			rcNewWnd.bottom += rcFrame.bottom;
		}

		// Двигаем окошко
		if (rcNewWnd.left != rcWnd.left || rcNewWnd.right != rcWnd.right || rcNewWnd.top != rcWnd.top || rcNewWnd.bottom != rcWnd.bottom)
			MOVEWINDOW(ghWnd, rcNewWnd.left, rcNewWnd.top, rcNewWnd.right-rcNewWnd.left, rcNewWnd.bottom-rcNewWnd.top, 1);

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
					if (lbLDown)
					{
						// Кликнули мышкой для активации рабочего стола с иконками
						mb_FocusOnDesktop = FALSE; // запомним, что ConEmu активировать не нужно
					}
					else if (mb_FocusOnDesktop)
					{
						// Чтобы пользователю не приходилось вручную активировать ConEmu после WinD / WinM
						//apiSetForegroundWindow(ghWnd); // это скорее всего не сработает, т.к. фокус сейчас у другого процесса!
						// так что "активируем" мышкой
						COORD crOpaque = mp_VActive->FindOpaqueCell();

						if (crOpaque.X<0 || crOpaque.Y<0)
						{
							DEBUGSTRFOREGROUND(L"Can't activate ConEmu on desktop. No opaque cell was found in VCon\n");
						}
						else
						{
							POINT pt = mp_VActive->ConsoleToClient(crOpaque.X,crOpaque.Y);
							MapWindowPoints(mp_VActive->GetView(), NULL, &pt, 1);
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

	DEBUGSTRFOREGROUND(szDbg);
}

void CConEmuMain::CheckUpdates(BOOL abShowMessages)
{
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

	if (isMainThread())
	{
		UpdateProgress();
		PostMessage(ghWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
	}
	else
	{
		PostMessage(ghWnd, mn_MsgRequestUpdate, 2, 0);
	}
}

enum DragPanelBorder CConEmuMain::CheckPanelDrag(COORD crCon)
{
	if (!gpSet->isDragPanel || isPictureView())
		return DPB_NONE;

	CRealConsole* pRCon = mp_VActive->RCon();

	if (!pRCon)
		return DPB_NONE;

	// Должен быть "Фар", "Панели", "Активен" (нет смысла пытаться дергать панели, если фар "висит")
	if (!pRCon->isFar() || !pRCon->isFilePanel(true) || !pRCon->isAlive())
		return DPB_NONE;

	// Если активен наш или ФАРовский граббер
	if (pRCon->isConSelectMode())
		return DPB_NONE;

	// Если удерживается модификатор запуска граббера
	if ((gpSet->isCTSSelectBlock && gpSet->IsModifierPressed(vkCTSVkBlock, false))
	        || (gpSet->isCTSSelectText && gpSet->IsModifierPressed(vkCTSVkText, false)))
		return DPB_NONE;

	//CONSOLE_CURSOR_INFO ci;
	//mp_VActive->RCon()->GetConsoleCursorInfo(&ci);
	//   if (!ci.bVisible || ci.dwSize>40) // Курсор должен быть видим, и не в режиме граба
	//   	return DPB_NONE;
	// Теперь - можно проверить
	enum DragPanelBorder dpb = DPB_NONE;
	RECT rcPanel;
	
	//TODO("Сделаем все-таки драг влево-вправо хватанием за «промежуток» между рамками");
	//int nSplitWidth = gpSetCls->BorderFontWidth()/5;
	//if (nSplitWidth < 1) nSplitWidth = 1;
	

	if (mp_VActive->RCon()->GetPanelRect(TRUE, &rcPanel, TRUE))
	{
		if (crCon.X == rcPanel.left && (rcPanel.top <= crCon.Y && crCon.Y <= rcPanel.bottom))
			dpb = DPB_SPLIT;
		else if (crCon.Y == rcPanel.bottom && (rcPanel.left <= crCon.X && crCon.X <= rcPanel.right))
			dpb = DPB_RIGHT;
	}

	if (dpb == DPB_NONE && mp_VActive->RCon()->GetPanelRect(FALSE, &rcPanel, TRUE))
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
	CVirtualConsole* pVCon = GetVConFromPoint(ptCur);
	CRealConsole *pRCon = pVCon ? pVCon->RCon() : NULL;
	if (!pRCon)
	{
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

	if (LOWORD(lParam) == HTCLIENT && pVCon)
	{
		// Если начат ShellDrag
		if (mh_DragCursor && isDragging())
		{
			hCur = mh_DragCursor;
			DEBUGSTRSETCURSOR(L" ---> DragCursor\n");
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

	if (!hCur)
	{
		hCur = mh_CursorArrow;
		DEBUGSTRSETCURSOR(L" ---> Arrow (default)\n");
	}

	SetCursor(hCur);
	return TRUE;
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
			//        	if (mp_VCon[i] != mp_VActive) { // Только для неактивной консоли
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

				DWORD dwPID = 0, dwParentPID = 0, dwFarPID = 0;
				GetWindowThreadProcessId(hWnd, &dwPID);

				if (dwPID && dwPID != GetCurrentProcessId())
				{
					AllowSetForegroundWindow(dwPID);

					if (IsWindowVisible(hWnd))  // ? оно успело ?
					{
						// Получить PID родительского процесса этого окошка
						HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);

						if (hSnap != INVALID_HANDLE_VALUE)
						{
							PROCESSENTRY32 prc = {sizeof(PROCESSENTRY32)};

							if (Process32First(hSnap, &prc))
							{
								do
								{
									if (prc.th32ProcessID == dwPID)
									{
										dwParentPID = prc.th32ParentProcessID;
										break;
									}
								}
								while(Process32Next(hSnap, &prc));
							}

							CloseHandle(hSnap);
						}

						for (size_t i = 0; i < countof(mp_VCon); i++)
						{
							if (mp_VCon[i] == NULL || mp_VCon[i]->RCon() == NULL) continue;

							dwFarPID = mp_VCon[i]->RCon()->GetFarPID();

							if (!dwFarPID) continue;

							if (dwPID == dwFarPID || dwParentPID == dwFarPID)  // MSDN Topics
							{
								apiSetForegroundWindow(hWnd);
								break;
							}
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
#ifdef _DEBUG
			wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"HSHELL_ACTIVATESHELLWINDOW(lParam=0x%08X)\n", (DWORD)lParam);
			DEBUGSTRFOREGROUND(szDbg);
#endif
		}
		break;
#endif
		case HSHELL_WINDOWACTIVATED:
		{
			// Приходит позже чем WM_ACTIVATE(WA_INACTIVE), но пригодится, если CE был НЕ в фокусе
#ifdef _DEBUG
			// Когда активируется Desktop - lParam == 0

			wchar_t szDbg[128], szClass[64]; if (!lParam || !GetClassName((HWND)lParam, szClass, 63)) wcscpy(szClass, L"<NULL>");

			BOOL lbLBtn = isPressed(VK_LBUTTON);
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"HSHELL_WINDOWACTIVATED(lParam=0x%08X, %s, %i)\n", (DWORD)lParam, szClass, lbLBtn);
			DEBUGSTRFOREGROUND(szDbg);
#endif
			CheckFocus(L"HSHELL_WINDOWACTIVATED");
		}
		break;
	}

	return 0;
}

void CConEmuMain::OnAlwaysOnTop()
{
	CheckMenuItem(gpConEmu->GetSystemMenu(), ID_ALWAYSONTOP, MF_BYCOMMAND |
	              (gpSet->isAlwaysOnTop ? MF_CHECKED : MF_UNCHECKED));
	SetWindowPos(ghWnd, (gpSet->isAlwaysOnTop || gpSet->isDesktopMode) ? HWND_TOPMOST : HWND_NOTOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);

	if (ghOpWnd && gpSet->isAlwaysOnTop)
	{
		SetWindowPos(ghOpWnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
		apiSetForegroundWindow(ghOpWnd);
	}
}

void CConEmuMain::OnDesktopMode()
{
	if (!this) return;

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
		SetWindowLong(ghWnd, GWL_STYLE, dwStyle);
		SetWindowLong(ghWnd, GWL_EXSTYLE, dwStyleEx);
		SyncWindowToConsole();
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

			while(hShell)
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

		if (mb_isFullScreen)  // этот режим с Desktop несовместим
			SetWindowMode(rMaximized);

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

	//SetWindowLong(ghWnd, GWL_STYLE, dwStyle);
#endif
}

INT_PTR CConEmuMain::aboutProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	static struct {LPCWSTR Title; LPCWSTR Text;} Pages[] =
	{
		{L"About", pAbout},
		{L"Command line", pCmdLine},
		{L"Contributors", pAboutContributors},
		{L"License", pAboutLicense},
	};

	switch (messg)
	{
		case WM_INITDIALOG:
		{
			gpConEmu->mh_AboutDlg = hWnd2;

			WCHAR szTitle[255];
			LPCWSTR pszBits = WIN3264TEST(L"x86",L"x64");
			LPCWSTR pszDebug = L"";
			#ifdef _DEBUG
			pszDebug = L"[DEBUG] ";
			#endif
			_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"About ConEmu (%02u%02u%02u%s %s%s)", 
				(MVV_1%100),MVV_2,MVV_3,_T(MVV_4a), pszDebug, pszBits);
			SetWindowText(hWnd2, szTitle);

			if (hClassIcon)
			{
				SendMessage(hWnd2, WM_SETICON, ICON_BIG, (LPARAM)hClassIcon);
				SendMessage(hWnd2, WM_SETICON, ICON_SMALL, (LPARAM)hClassIconSm);
				SetClassLongPtr(hWnd2, GCLP_HICON, (LONG_PTR)hClassIcon);
			}

			SetDlgItemText(hWnd2, stConEmuAbout, pAboutTitle);
			SetDlgItemText(hWnd2, stConEmuUrl, gsHomePage);

			HWND hTab = GetDlgItem(hWnd2, tbAboutTabs);

			for (size_t i = 0; i < countof(Pages); i++)
			{
				TCITEM tie = {};
				tie.mask = TCIF_TEXT;
				tie.pszText = (LPWSTR)Pages[i].Title;
				TabCtrl_InsertItem(hTab, i, &tie);
			}

			SetDlgItemText(hWnd2, tAboutText, Pages[0].Text);

			break;
		}

		case WM_CTLCOLORSTATIC:
			if (GetDlgItem(hWnd2, stConEmuUrl) == (HWND)lParam)
			{
				SetTextColor((HDC)wParam, GetSysColor(COLOR_HOTLIGHT));
				HBRUSH hBrush = GetSysColorBrush(COLOR_3DFACE);
				SetBkMode((HDC)wParam, TRANSPARENT);
				return (INT_PTR)hBrush;
			}
			else
			{
				SetTextColor((HDC)wParam, GetSysColor(COLOR_WINDOWTEXT));
				HBRUSH hBrush = GetSysColorBrush(COLOR_3DFACE);
				SetBkMode((HDC)wParam, TRANSPARENT);
				return (INT_PTR)hBrush;
			}
			break;

		case WM_SETCURSOR:
			{
				if (((HWND)wParam) == GetDlgItem(hWnd2, stConEmuUrl))
				{
					SetCursor(LoadCursor(NULL, IDC_HAND));
					SetWindowLongPtr(hWnd2, DWLP_MSGRESULT, TRUE);
					return TRUE;
				}
				return FALSE;
			}
			break;

		case WM_COMMAND:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				switch (LOWORD(wParam))
				{
					case IDOK:
					case IDCANCEL:
					case IDCLOSE:
						aboutProc(hWnd2, WM_CLOSE, 0, 0);
						return 1;
					case stConEmuUrl:
						gpConEmu->OnInfo_HomePage();
						return 1;
				}
			}
			break;

		case WM_NOTIFY:
		{
			LPNMHDR nmhdr = (LPNMHDR)lParam;
			if ((nmhdr->code == TCN_SELCHANGE) && (nmhdr->idFrom == tbAboutTabs))
			{
				int iPage = TabCtrl_GetCurSel(nmhdr->hwndFrom);
				if ((iPage >= 0) && (iPage < countof(Pages)))
					SetDlgItemText(hWnd2, tAboutText, Pages[iPage].Text);
			}
			break;
		}

		case WM_CLOSE:
			//if (ghWnd == NULL)
			EndDialog(hWnd2, IDOK);
			//else
			//	DestroyWindow(hWnd2);
			break;

		case WM_DESTROY:
			gpConEmu->mh_AboutDlg = NULL;
			break;

		default:
			return FALSE;
	}

	return FALSE;
}

void CConEmuMain::OnInfo_About()
{
	InitCommCtrls();

	bool bOk = false;

	//if (ghWnd)
	//{
	//	HWND hAbout = NULL;
	//	if (mh_AboutDlg && IsWindow(mh_AboutDlg))
	//	{
	//		hAbout = mh_AboutDlg;
	//		apiShowWindow(hAbout, SW_SHOWNORMAL);
	//		SetFocus(hAbout);
	//	}
	//	else
	//	{
	//		hAbout = CreateDialog(g_hInstance, MAKEINTRESOURCE(IDD_ABOUT), NULL, aboutProc);
	//	}
	//	bOk = (hAbout != NULL);
	//}
	//else
	{
		BOOL b = gbDontEnable; gbDontEnable = TRUE;
		INT_PTR iRc = DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_ABOUT), ghWnd, aboutProc);
		gbDontEnable = b;
		bOk = (iRc != 0 && iRc != -1);
	}

	if (!bOk)
	{
		WCHAR szTitle[255];
		LPCWSTR pszBits = WIN3264TEST(L"x86",L"x64");
		LPCWSTR pszDebug = L"";
		#ifdef _DEBUG
		pszDebug = L"[DEBUG] ";
		#endif
		_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"About ConEmu (%02u%02u%02u%s %s%s)", 
			(MVV_1%100),MVV_2,MVV_3,_T(MVV_4a), pszDebug, pszBits);
		BOOL b = gbDontEnable; gbDontEnable = TRUE;
		MSGBOXPARAMS mb = {sizeof(MSGBOXPARAMS), ghWnd, g_hInstance,
			pAbout,
			szTitle,
			MB_USERICON, MAKEINTRESOURCE(IMAGE_ICON), NULL, NULL, LANG_NEUTRAL
		};
		MessageBoxIndirectW(&mb);
		//MessageBoxW(ghWnd, pHelp, szTitle, MB_ICONQUESTION);
		gbDontEnable = b;
	}
}

void CConEmuMain::OnInfo_Help()
{
	static HMODULE hhctrl = NULL;

	if (!hhctrl) hhctrl = GetModuleHandle(L"hhctrl.ocx");

	if (!hhctrl) hhctrl = LoadLibrary(L"hhctrl.ocx");

	if (hhctrl)
	{
		typedef BOOL (WINAPI* HTMLHelpW_t)(HWND hWnd, LPCWSTR pszFile, INT uCommand, INT dwData);
		HTMLHelpW_t fHTMLHelpW = (HTMLHelpW_t)GetProcAddress(hhctrl, "HtmlHelpW");

		if (fHTMLHelpW)
		{
			wchar_t szHelpFile[MAX_PATH*2];
			lstrcpy(szHelpFile, ms_ConEmuChm);
			//wchar_t* pszSlash = wcsrchr(szHelpFile, L'\\');
			//if (pszSlash) pszSlash++; else pszSlash = szHelpFile;
			//lstrcpy(pszSlash, L"ConEmu.chm");
			// lstrcat(szHelpFile, L::/Intro.htm");
			#define HH_HELP_CONTEXT 0x000F
			#define HH_DISPLAY_TOC  0x0001
			//fHTMLHelpW(NULL /*чтобы окно не блокировалось*/, szHelpFile, HH_HELP_CONTEXT, contextID);
			fHTMLHelpW(NULL /*чтобы окно не блокировалось*/, szHelpFile, HH_DISPLAY_TOC, 0);
		}
	}
}

void CConEmuMain::OnInfo_HomePage()
{
	DWORD shellRc = (DWORD)(INT_PTR)ShellExecute(ghWnd, L"open", gsHomePage, NULL, NULL, SW_SHOWNORMAL);
	if (shellRc <= 32)
	{
		DisplayLastError(L"ShellExecute failed", shellRc);
	}
}

void CConEmuMain::OnInfo_ReportBug()
{
	DWORD shellRc = (DWORD)(INT_PTR)ShellExecute(ghWnd, L"open", gsReportBug, NULL, NULL, SW_SHOWNORMAL);
	if (shellRc <= 32)
	{
		DisplayLastError(L"ShellExecute failed", shellRc);
	}
}

LRESULT CConEmuMain::OnSysCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
#ifdef _DEBUG
	wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"OnSysCommand (%i(0x%X), %i)\n", (DWORD)wParam, (DWORD)wParam, (DWORD)lParam);
	DEBUGSTRSIZE(szDbg);
#endif
	LRESULT result = 0;

	if (wParam >= IDM_VCON_FIRST && wParam <= IDM_VCON_LAST)
	{
		int nNewV = ((int)HIWORD(wParam))-1;
		int nNewR = ((int)LOWORD(wParam))-1; UNREFERENCED_PARAMETER(nNewR);
		
		CVirtualConsole* pVCon = GetVCon(nNewV);
		if (pVCon)
		{
			// -- в SysMenu показываются только консоли (редакторов/вьюверов там нет)
			//CRealConsole* pRCon = pVCon->RCon();
			//if (pRCon)
			//{
			//	//if (pRCon->CanActivateFarWindow(nNewR))
			//	pRCon->ActivateFarWindow(nNewR);
			//}
			if (!isActive(pVCon))
				Activate(pVCon);
			//else
			//	UpdateTabs();
		}
		return 0;
	}

	//switch(LOWORD(wParam))
	switch (wParam)
	{
		case ID_NEWCONSOLE:
			// Создать новую консоль
			Recreate(FALSE, gpSet->isMultiNewConfirm || isPressed(VK_SHIFT));
			return 0;
			
		case IDM_ATTACHTO:
			if (!mp_AttachDlg)
				mp_AttachDlg = new CAttachDlg;
			mp_AttachDlg->AttachDlg();
			return 0;
			
		case ID_SETTINGS:
			CSettings::Dialog();
			return 0;
			
		case ID_CON_PASTE:
			mp_VActive->RCon()->Paste();
			return 0;

		case ID_CON_FIND:
			gpSetCls->FindTextDialog();
			return 0;

		case ID_CON_COPY:
			mp_VActive->RCon()->DoSelectionCopy();
			return 0;
			
		case ID_CON_MARKBLOCK:
		case ID_CON_MARKTEXT:
			mp_VActive->RCon()->StartSelection(LOWORD(wParam) == ID_CON_MARKTEXT);
			return 0;
			
		case ID_AUTOSCROLL:
			gpSetCls->AutoScroll = !gpSetCls->AutoScroll;
			CheckMenuItem(gpConEmu->GetSystemMenu(), ID_AUTOSCROLL, MF_BYCOMMAND |
			              (gpSetCls->AutoScroll ? MF_CHECKED : MF_UNCHECKED));
			return 0;
			
		case ID_ALWAYSONTOP:
			{
				gpSet->isAlwaysOnTop = !gpSet->isAlwaysOnTop;
				OnAlwaysOnTop();

				HWND hExt = gpSetCls->mh_Tabs[gpSetCls->thi_Ext];

				if (ghOpWnd && hExt)
				{
					CheckDlgButton(hExt, cbAlwaysOnTop, gpSet->isAlwaysOnTop ? BST_CHECKED : BST_UNCHECKED);
				}
			}
			return 0;
			
		case ID_DUMPCONSOLE:

			if (mp_VActive)
				mp_VActive->DumpConsole();

			return 0;

		case ID_LOADDUMPCONSOLE:

			if (mp_VActive)
				mp_VActive->LoadDumpConsole();

			return 0;
			
		case ID_DEBUGGUI:
			StartDebugLogConsole();
			return 0;
			
		case ID_DEBUGCON:
			StartDebugActiveProcess();
			return 0;
			
		//case ID_MONITOR_SHELLACTIVITY:
		//{
		//	CRealConsole* pRCon = mp_VActive->RCon();

		//	if (pRCon)
		//		pRCon->LogShellStartStop();

		//	//if (!mb_CreateProcessLogged)
		//	//	StartLogCreateProcess();
		//	//else
		//	//	StopLogCreateProcess();
		//}
		//return 0;
		
		case ID_DEBUG_SHOWRECTS:
			gbDebugShowRects = !gbDebugShowRects;
			InvalidateAll();
			return 0;
			
		case ID_CON_TOGGLE_VISIBLE:

			if (mp_VActive)
				mp_VActive->RCon()->ShowConsoleOrGuiClient(-1); // Toggle visibility

			return 0;
			
		case ID_HELP:
		{
			gpConEmu->OnInfo_Help();
			return 0;
		} // case ID_HELP:
		
		case ID_HOMEPAGE:
		{
			gpConEmu->OnInfo_HomePage();
			return 0;
		}
		
		case ID_REPORTBUG:
		{
			gpConEmu->OnInfo_ReportBug();
			return 0;
		}
		
		case ID_CHECKUPDATE:
			gpConEmu->CheckUpdates(TRUE);
			return 0;
		
		case ID_STOPUPDATE:
			if (gpUpd)
				gpUpd->StopChecking();
			return 0;
		
		case ID_ABOUT:
		{
			gpConEmu->OnInfo_About();
			return 0;
		}

		case ID_TOMONITOR:
		{
			if (!IsWindowVisible(ghWnd))
				Icon.RestoreWindowFromTray();
			POINT ptCur = {}; GetCursorPos(&ptCur);
			HMONITOR hMon = MonitorFromPoint(ptCur, MONITOR_DEFAULTTOPRIMARY);
			MONITORINFO mi = {sizeof(mi)};
			GetMonitorInfo(hMon, &mi);
			SetWindowPos(ghWnd, HWND_TOP, mi.rcWork.left, mi.rcWork.top, 0,0, SWP_NOSIZE);
			return 0;
		}

		case ID_TOTRAY:
			if (IsWindowVisible(ghWnd))
				Icon.HideWindowToTray();
			else
				Icon.RestoreWindowFromTray();

			return 0;
			
		case ID_CONPROP:
		{
			#ifdef MSGLOGGER
			{
				HMENU hMenu = ::GetSystemMenu(ghConWnd, FALSE);
				MENUITEMINFO mii; TCHAR szText[255];

				for(int i=0; i<15; i++)
				{
					memset(&mii, 0, sizeof(mii));
					mii.cbSize = sizeof(mii); mii.dwTypeData=szText; mii.cch=255;
					mii.fMask = MIIM_ID|MIIM_STRING|MIIM_SUBMENU;

					if (GetMenuItemInfo(hMenu, i, TRUE, &mii))
					{
						mii.cbSize = sizeof(mii);

						if (mii.hSubMenu)
						{
							MENUITEMINFO mic;

							for(int i=0; i<15; i++)
							{
								memset(&mic, 0, sizeof(mic));
								mic.cbSize = sizeof(mic); mic.dwTypeData=szText; mic.cch=255;
								mic.fMask = MIIM_ID|MIIM_STRING;

								if (GetMenuItemInfo(mii.hSubMenu, i, TRUE, &mic))
								{
									mic.cbSize = sizeof(mic);
								}
								else
								{
									break;
								}
							}
						}
					}
					else
						break;
				}
			}
			#endif
			//POSTMESSAGE(ghConWnd, WM_SYSCOMMAND, SC_PROPERTIES_SECRET/*65527*/, 0, TRUE);
			if (mp_VActive && mp_VActive->RCon())
				mp_VActive->RCon()->ShowPropertiesDialog();
			return 0;
		} // case ID_CONPROP:

		case SC_MAXIMIZE_SECRET:
			SetWindowMode(rMaximized);
			break;
			
		case SC_RESTORE_SECRET:
			SetWindowMode(rNormal);
			break;
			
		case SC_CLOSE:
		{
			//Icon.Delete();
			//SENDMESSAGE(ghConWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
			//SENDMESSAGE(ghConWnd ? ghConWnd : ghWnd, WM_CLOSE, 0, 0); // ?? фар не ловит сообщение, ExitFAR не вызываются
			int nConCount = 0, nDetachedCount = 0;

			//int nEditors = 0, nProgress = 0, i;
			//for (i=((int)countof(mp_VCon)-1); i>=0; i--) {
			//	CRealConsole* pRCon = NULL;
			//	ConEmuTab tab = {0};
			//	if (mp_VCon[i] && (pRCon = mp_VCon[i]->RCon())!=NULL) {
			//		// Прогрессы (копирование, удаление, и т.п.)
			//		if (pRCon->GetProgress(NULL) != -1)
			//			nProgress ++;
			//
			//		// Несохраненные редакторы
			//		int n = pRCon->GetModifiedEditors();
			//		if (n)
			//			nEditors += n;
			//	}
			//}
			//if (nProgress || nEditors) {
			//	wchar_t szText[255], *pszText;
			//	lstrcpy(szText, L"Close confirmation.\r\n\r\n"); pszText = szText+_tcslen(szText);
			//	if (nProgress) { _wsprintf(pszText, SKIPLEN(countof(pszText)) L"Incomplete operations: %i\r\n", nProgress); pszText += _tcslen(pszText); }
			//	if (nEditors) { _wsprintf(pszText, SKIPLEN(countof(pszText)) L"Unsaved editor windows: %i\r\n", nEditors); pszText += _tcslen(pszText); }
			//	lstrcpy(pszText, L"\r\nProceed with shutdown?");
			//	int nBtn = MessageBoxW(ghWnd, szText, GetDefaultTitle(), MB_OKCANCEL|MB_ICONEXCLAMATION);
			//	if (nBtn != IDOK)
			//		return 0; // не закрывать
			//}
			if (!gpConEmu->OnCloseQuery())
				return 0; // не закрывать

			// Сохраним размер перед закрытием консолей, а то они могут напакостить и "вернуть" старый размер
			gpSet->SaveSizePosOnExit();
				
			for (int i = (int)(countof(mp_VCon)-1); i >= 0; i--)
			{
				if (mp_VCon[i] && mp_VCon[i]->RCon())
				{
					if (mp_VCon[i]->RCon()->isDetached())
					{
						nDetachedCount ++;
						continue;
					}

					nConCount ++;

					if (mp_VCon[i]->RCon()->ConWnd())
					{
						mp_VCon[i]->RCon()->CloseConsole(false, false);
					}
				}
			}

			if (nConCount == 0)
			{
				if (nDetachedCount > 0)
				{
					if (MessageBox(ghWnd, L"ConEmu is waiting for console attach.\nIt was started in 'Detached' mode.\nDo You want to cancel waiting?",
					              GetDefaultTitle(), MB_YESNO|MB_ICONQUESTION) != IDYES)
						return result;
				}

				Destroy();
			}
			break;
		} // case SC_CLOSE:
		
		case SC_MAXIMIZE:
		{
			DEBUGSTRSYS(L"OnSysCommand(SC_MAXIMIZE)\n");

			if (!mb_PassSysCommand)
			{
				#ifndef _DEBUG
				if (isPictureView())
					break;
				#endif
				
				SetWindowMode(rMaximized);
			}
			else
			{
				result = DefWindowProc(hWnd, WM_SYSCOMMAND, wParam, lParam);
			}

			break;
		} // case SC_MAXIMIZE:
		
		case SC_RESTORE:
		{
			DEBUGSTRSYS(L"OnSysCommand(SC_RESTORE)\n");

			if (!mb_PassSysCommand)
			{
				#ifndef _DEBUG
				if (!isIconic() && isPictureView())
					break;
				#endif

				if (!SetWindowMode(isIconic() ? WindowMode : rNormal))
					result = DefWindowProc(hWnd, WM_SYSCOMMAND, wParam, lParam);
			}
			else
			{
				result = DefWindowProc(hWnd, WM_SYSCOMMAND, wParam, lParam);
			}

			break;
		} // case SC_RESTORE:
		
		case SC_MINIMIZE:
		{
			DEBUGSTRSYS(L"OnSysCommand(SC_MINIMIZE)\n");

			// Если "фокус" в дочернем Gui приложении - нужно перед скрытием ConEmu "поднять" его
			if (mp_VActive && mp_VActive->RCon())
			{
				if (mp_VActive->RCon()->GuiWnd())
					apiSetForegroundWindow(ghWnd);
			}

			if (gpSet->isMinToTray || gpConEmu->ForceMinimizeToTray)
			{
				Icon.HideWindowToTray();
				break;
			}

			//if (gpSet->isDontMinimize) {
			//	SetWindowPos(ghWnd, HWND_BOTTOM, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
			//	break;
			//}
			result = DefWindowProc(hWnd, WM_SYSCOMMAND, wParam, lParam);
			break;
		} // case SC_MINIMIZE:
		
		default:
		{
			if (wParam >= IDM_VCONCMD_FIRST && wParam <= IDM_VCONCMD_LAST)
			{
				ActiveCon()->ExecPopupMenuCmd((int)(DWORD)wParam);
				result = 0; 
			}
			else if (wParam != 0xF100)
			{
				#ifdef _DEBUG
				wchar_t szDbg[64]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"OnSysCommand(%i)\n", (DWORD)wParam);
				DEBUGSTRSYS(szDbg);
				#endif

				// иначе это приводит к потере фокуса и активации невидимой консоли,
				// перехвате стрелок клавиатуры, и прочей фигни...
				if (wParam<0xF000)
				{
					POSTMESSAGE(ghConWnd, WM_SYSCOMMAND, wParam, lParam, FALSE);
				}

				if (wParam == SC_SYSMENUPOPUP_SECRET)
				{
					mn_TrackMenuPlace = tmp_System;
					mp_Tip->HideTip();
				}

				result = DefWindowProc(hWnd, WM_SYSCOMMAND, wParam, lParam);

				if (wParam == SC_SYSMENUPOPUP_SECRET)
				{
					mn_TrackMenuPlace = tmp_None;
					mp_Tip->HideTip();
				}
			}
		} // default:
	}

	return result;
}

WARNING("Частота хождения таймера в винде оставляет желать... нужно от него избавляться и по возможности переносить все в нити");
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

	switch(wParam)
	{
		case TIMER_MAIN_ID: // Период: 500 мс
		{
			//Maximus5. Hack - если какая-то зараза задизеблила окно
			if (!gbDontEnable)
			{
				DWORD dwStyle = GetWindowLong(ghWnd, GWL_STYLE);

				if (dwStyle & WS_DISABLED)
					EnableWindow(ghWnd, TRUE);
			}

			bool bForeground = isMeForeground();
			CheckProcesses();
			TODO("Теперь это условие не работает. 1 - раньше это был сам ConEmu.exe");

			if (m_ProcCount == 0)
			{
				// При ошибках запуска консольного приложения хотя бы можно будет увидеть, что оно написало...
				if (mb_ProcessCreated)
				{
					OnAllVConClosed();
					break;
				}
			}
			else
			{
				//if (!mb_ProcessCreated && m_ProcCount>=1) --> OnRConStartedSuccess
				//{
				//	if ((GetTickCount() - mn_StartTick)>PROCESS_WAIT_START_TIME)
				//		mb_ProcessCreated = TRUE;
				//}

				if (!mb_WorkspaceErasedOnClose)
					mb_WorkspaceErasedOnClose = FALSE;
			}

			// TODO: поддержку SlideShow повесить на отдельный таймер
			BOOL lbIsPicView = isPictureView();

			if (bPicViewSlideShow)
			{
				DWORD dwTicks = GetTickCount();
				DWORD dwElapse = dwTicks - dwLastSlideShowTick;

				if (dwElapse > gpSet->nSlideShowElapse)
				{
					if (IsWindow(hPictureView))
					{
						//
						bPicViewSlideShow = false;
						SendMessage(ghConWnd, WM_KEYDOWN, VK_NEXT, 0x01510001);
						SendMessage(ghConWnd, WM_KEYUP, VK_NEXT, 0xc1510001);
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
				SyncConsoleToWindow();
				//INVALIDATE(); //InvalidateRect(HDCWND, NULL, FALSE);
				InvalidateAll();
			}

			if (!isIconic())
			{
				//// Было ли реальное изменение размеров?
				//BOOL lbSizingToDo  = (mouse.state & MOUSE_SIZING_TODO) == MOUSE_SIZING_TODO;

				//if (isSizing() && !isPressed(VK_LBUTTON)) {
				//    // Сборс всех флагов ресайза мышкой
				//    mouse.state &= ~(MOUSE_SIZING_BEGIN|MOUSE_SIZING_TODO);
				//}

				//TODO("возможно весь ресайз (кроме SyncNtvdm?) нужно перенести в нить консоли")
				//OnConsoleResize();

				// update scrollbar
				OnUpdateScrollInfo(TRUE);
			}

			// режим полного скрытия заголовка
			if (gpSet->isHideCaptionAlways())
			{
				if (!bForeground)
				{
					if (mb_ForceShowFrame)
					{
						mb_ForceShowFrame = FALSE;
						KillTimer(ghWnd, TIMER_CAPTION_APPEAR_ID); KillTimer(ghWnd, TIMER_CAPTION_DISAPPEAR_ID);
						UpdateWindowRgn();
					}
				}
				else
				{
					// в Normal режиме при помещении мышки над местом, где должен быть
					// заголовок или рамка - показать их
					if (!isIconic() && !isZoomed() && !mb_isFullScreen)
					{
						TODO("Не наколоться бы с предыдущим статусом при ресайзе?");
						//static bool bPrevForceShow = false;
						BOOL bCurForceShow = isMouseOverFrame(true);

						if (bCurForceShow != mb_CaptionWasRestored)
						{
							mb_CaptionWasRestored = bCurForceShow;
							//if (gpSet->nHideCaptionAlwaysDelay && bCurForceShow) {
							KillTimer(ghWnd, TIMER_CAPTION_APPEAR_ID); KillTimer(ghWnd, TIMER_CAPTION_DISAPPEAR_ID);
							WORD nID = bCurForceShow ? TIMER_CAPTION_APPEAR_ID : TIMER_CAPTION_DISAPPEAR_ID;
							DWORD nDelay = bCurForceShow ? gpSet->nHideCaptionAlwaysDelay : gpSet->nHideCaptionAlwaysDisappear;

							if (nDelay)
								SetTimer(ghWnd, nID, nDelay, NULL);
							else
								UpdateWindowRgn();

							//} else {
							//	UpdateWindowRgn();
							//}
						}
					}
				}
			}

			if (mp_VActive)
			{
				bool bLastFade = mp_VActive->mb_LastFadeFlag;
				bool bNewFade = (gpSet->isFadeInactive && !bForeground && !lbIsPicView);

				// Это условие скорее всего никогда не выполнится, т.к.
				// смена Fade обрабатывается в WM_ACTIVATE/WM_SETFOCUS/WM_KILLFOCUS
				if (bLastFade != bNewFade)
				{
					mp_VActive->mb_LastFadeFlag = bNewFade;
					Invalidate(mp_VActive);
				}
			}

			if (mh_ConEmuAliveEvent && !mb_ConEmuAliveOwned)
				isFirstInstance(); // Заодно и проверит...

			// Если был изменен файл background
			if (gpSetCls->PollBackgroundFile())
			{
				gpConEmu->Update(true);
			}

			if (mn_TrackMenuPlace != tmp_None && mp_Tip)
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

			CheckFocus(L"TIMER_MAIN_ID");

			if (!lbIsPicView && gpSet->UpdSet.isUpdateCheckHourly)
			{
				gpSet->UpdSet.CheckHourlyUpdate();
			}

		} break; // case 0:
		case TIMER_CONREDRAW_ID: // Период: CON_REDRAW_TIMOUT*2
		{
			if (mp_VActive && !isIconic())
			{
				mp_VActive->CheckPostRedraw();
			}
		} break; // case 1:
		case TIMER_CAPTION_APPEAR_ID:
		case TIMER_CAPTION_DISAPPEAR_ID:
		{
			KillTimer(ghWnd, wParam);
			mb_ForceShowFrame = isMouseOverFrame(true);
			UpdateWindowRgn();
		} break;
		case TIMER_RCLICKPAINT:
		{
			RightClickingPaint(NULL, NULL);
		} break;
	}

	mb_InTimer = FALSE;
	return result;
}

void CConEmuMain::OnTransparent()
{
	UINT nAlpha = max(MIN_ALPHA_VALUE,min(gpSet->nTransparent,255));
	if ((nAlpha < 255) && isPictureView())
		nAlpha = 255;

	// return true - when state was changes
	if (SetTransparent(ghWnd, nAlpha))
	{
		OnSetCursor();
	}
}

// return true - when state was changes
bool CConEmuMain::SetTransparent(HWND ahWnd, UINT anAlpha/*0..255*/)
{
	#ifdef __GNUC__
	if (!SetLayeredWindowAttributes)
	{
		_ASSERTE(SetLayeredWindowAttributes!=NULL);
		return false;
	}
	#endif

	BOOL bNeedRedrawOp = FALSE;
	UINT nTransparent = max(MIN_ALPHA_VALUE,min(anAlpha,255));
	DWORD dwExStyle = GetWindowLongPtr(ahWnd, GWL_EXSTYLE);
	bool lbChanged = false;

	if (nTransparent >= 255)
	{
		// Прозрачность отключается (полностью непрозрачный)
		//SetLayeredWindowAttributes(ahWnd, 0, 255, LWA_ALPHA);
		if ((dwExStyle & WS_EX_LAYERED) == WS_EX_LAYERED)
		{
			dwExStyle &= ~WS_EX_LAYERED;
			SetLayeredWindowAttributes(ahWnd, 0, 255, LWA_ALPHA);
			SetWindowLongPtr(ahWnd, GWL_EXSTYLE, dwExStyle);
			lbChanged = true;
		}
	}
	else
	{
		if ((dwExStyle & WS_EX_LAYERED) == 0)
		{
			dwExStyle |= WS_EX_LAYERED;
			SetWindowLongPtr(ahWnd, GWL_EXSTYLE, dwExStyle);
			bNeedRedrawOp = TRUE;
			lbChanged = true;
		}

		DWORD nNewFlags = LWA_ALPHA;

		BYTE nCurAlpha = 0;
		DWORD nCurFlags = 0;
		if (lbChanged
			|| (!_GetLayeredWindowAttributes || !(_GetLayeredWindowAttributes(ahWnd, NULL, &nCurAlpha, &nCurFlags)))
			|| (nCurAlpha != nTransparent) || (nCurFlags != nNewFlags))
		{
			lbChanged = true;
			SetLayeredWindowAttributes(ahWnd, 0, nTransparent, nNewFlags);
		}

		// После смены стиля (не было альфа - появилась альфа) измененное окно "выносится наверх"
		// и принудительно перерисовывается. Если в этот момент видим диалог настроек - он затирается.
		if (bNeedRedrawOp)
		{
			HWND hWindows[] = {ghOpWnd, (mp_AttachDlg ? mp_AttachDlg->GetHWND() : NULL), gpSetCls->mh_FindDlg};
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
		PostMessage(ghWnd, mn_MsgUpdateScrollInfo, 0, (LPARAM)mp_VCon);
		return 0;
	}

	for (size_t i = 0; i < countof(mp_VCon); i++)
	{
		if (mp_VCon[i] && mp_VCon[i]->isVisible())
		{
			if (mp_VCon[i]->RCon())
				mp_VCon[i]->RCon()->UpdateScrollInfo();
		}
	}

	return 0;
}

// Чтобы при создании ПЕРВОЙ консоли на экране сразу можно было что-то нарисовать
void CConEmuMain::OnVConCreated(CVirtualConsole* apVCon, const RConStartArgs *args)
{
	if (!mp_VActive || (mb_CreatingActive && !args->bBackgroundTab))
	{
		mp_VActive = apVCon;

		// Теперь можно показать созданную консоль
		apiShowWindow(mp_VActive->GetView(), SW_SHOW);
	}
}

void CConEmuMain::OnAllVConClosed()
{
	OnAllGhostClosed();

	Taskbar_SetShield(false);

	if (!gpSet->isMultiLeaveOnClose)
	{
		Destroy();
	}
	else if (!mb_WorkspaceErasedOnClose)
	{
		mb_WorkspaceErasedOnClose = TRUE;
		UpdateTitle();
		InvalidateAll();
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
			SetWindowLong(ghWnd, GWL_STYLE, style);
			SetWindowLong(ghWnd, GWL_EXSTYLE, styleEx);
			Taskbar_DeleteTabXP(ghWnd);
		}
	}
}

void CConEmuMain::OnAllGhostClosed()
{
	DWORD curStyle = GetWindowLong(ghWnd, GWL_STYLE);
	DWORD curStyleEx = GetWindowLong(ghWnd, GWL_EXSTYLE);
	DWORD style = (curStyle & ~WS_POPUP);
	DWORD styleEx = (curStyleEx | WS_EX_APPWINDOW);
	if (style != curStyle || styleEx != curStyleEx)
	{
		SetWindowLong(ghWnd, GWL_STYLE, style);
		SetWindowLong(ghWnd, GWL_EXSTYLE, styleEx);
		Taskbar_AddTabXP(ghWnd);
	}
}

void CConEmuMain::OnRConStartedSuccess(CRealConsole* apRCon)
{
	// Note, apRCon MAY be NULL
	mb_ProcessCreated = TRUE;
}

LRESULT CConEmuMain::OnVConTerminated(CVirtualConsole* apVCon, BOOL abPosted /*= FALSE*/)
{
	_ASSERTE(apVCon);

	if (!apVCon)
		return 0;

	if (!abPosted)
	{
		PostMessage(ghWnd, mn_MsgVConTerminated, 0, (LPARAM)apVCon);
		return 0;
	}

	for (size_t i = 0; i < countof(mp_VCon); i++)
	{
		if (mp_VCon[i] == apVCon)
		{
			// Сначала нужно обновить закладки, иначе в закрываемой консоли
			// может быть несколько вкладок и вместо активации другой консоли
			// будет попытка активировать другую вкладку закрываемой консоли
			//gpConEmu->mp_TabBar->Update(TRUE); -- а и не сможет он другую активировать, т.к. RCon вернет FALSE

			// Эта комбинация должна активировать предыдущую консоль (если активна текущая)
			if (gpSet->isTabRecent && apVCon == mp_VActive)
			{
				if (gpConEmu->GetVCon(1))
				{
					gpConEmu->mp_TabBar->SwitchRollback();
					gpConEmu->mp_TabBar->SwitchNext();
					gpConEmu->mp_TabBar->SwitchCommit();
				}
			}

			// Теперь можно очистить переменную массива
			mp_VCon[i] = NULL;
			WARNING("Вообще-то это нужно бы в CriticalSection закрыть. Несколько консолей может одновременно закрыться");

			if (mp_VActive == apVCon)
			{
				for(int j=(i-1); j>=0; j--)
				{
					if (mp_VCon[j])
					{
						ConActivate(j);
						break;
					}
				}

				if (mp_VActive == apVCon)
				{
					for (size_t j = (i+1); j < countof(mp_VCon); j++)
					{
						if (mp_VCon[j])
						{
							ConActivate(j);
							break;
						}
					}
				}
			}

			for (size_t j = (i+1); j < countof(mp_VCon); j++)
			{
				mp_VCon[j-1] = mp_VCon[j];
			}

			mp_VCon[countof(mp_VCon)-1] = NULL;

			if (mp_VActive == apVCon)
				mp_VActive = NULL;

			apVCon->Release();
			break;
		}
	}

	// Теперь перетряхнуть заголовок (табы могут быть отключены и в заголовке отображается количество консолей)
	UpdateTitle(); // сам перечитает
	//
	gpConEmu->mp_TabBar->Update(); // Иначе не будет обновлены закладки
	// А теперь можно обновить активную закладку
	gpConEmu->mp_TabBar->OnConsoleActivated(ActiveConNum()/*, FALSE*/);
	return 0;
}

//void CConEmuMain::PostSetBackground(CVirtualConsole* apVCon, CESERVER_REQ_SETBACKGROUND* apImgData)
//{
//	PostMessage(ghWnd, mn_MsgPostSetBackground, (WPARAM)apVCon, (LPARAM)apImgData);
//}

LRESULT CConEmuMain::MainWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	//if (messg == WM_CREATE)
	//{
	if (ghWnd == NULL)
		ghWnd = hWnd; // ставим сразу, чтобы функции могли пользоваться
	//else if ('ghWnd DC' == NULL)
	//	'ghWnd DC' = hWnd; // ставим сразу, чтобы функции могли пользоваться
	//}

	if (hWnd == ghWnd)
		result = gpConEmu->WndProc(hWnd, messg, wParam, lParam);
	else //if (hWnd == 'ghWnd DC')
		result = CConEmuChild::ChildWndProc(hWnd, messg, wParam, lParam);
	//else if (messg)
	//	result = DefWindowProc(hWnd, messg, wParam, lParam);

	return result;
}

BOOL CConEmuMain::isDialogMessage(MSG &Msg)
{
	BOOL lbDlgMsg = FALSE;
	_ASSERTE(this!=NULL && this==gpConEmu);

	HWND hDlg = NULL;

	if (ghOpWnd)
	{
		if (IsWindow(ghOpWnd))
			lbDlgMsg = IsDialogMessage(ghOpWnd, &Msg);
	}

	if (!lbDlgMsg && mp_AttachDlg && (hDlg = mp_AttachDlg->GetHWND()))
	{
		if (IsWindow(hDlg))
			lbDlgMsg = IsDialogMessage(hDlg, &Msg);
	}

	if (!lbDlgMsg && (hDlg = gpSetCls->mh_FindDlg))
	{
		if (IsWindow(hDlg))
			lbDlgMsg = IsDialogMessage(hDlg, &Msg);
	}

	return lbDlgMsg;
}

LRESULT CConEmuMain::WndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	//MCHKHEAP
#ifdef _DEBUG
	if (messg == WM_TIMER || messg == WM_GETICON)
	{
		bool lbDbg1 = false;
	}
	else if (messg >= WM_APP)
	{
		bool lbDbg2 = false;
	}
	else
	{
		//wchar_t szDbg[127]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WndProc(%i{x%03X},%i,%i)\n", messg, messg, (DWORD)wParam, (DWORD)lParam);
		//OutputDebugStringW(szDbg);
	}
#endif

	if (messg == WM_SYSCHAR)  // Вернул. Для пересылки в консоль не используется, но чтобы не пищало - необходимо
		return TRUE;

	if (gpConEmu->ProcessNcMessage(hWnd, messg, wParam, lParam, result))
		return result;

	if (gpConEmu->ProcessGestureMessage(hWnd, messg, wParam, lParam, result))
		return result;
		
	//if (messg == WM_CHAR)
	//  return TRUE;

	switch (messg)
	{
		case WM_CREATE:
			result = gpConEmu->OnCreate(hWnd, (LPCREATESTRUCT)lParam);
			break;
		case WM_NOTIFY:
		{
			if (gpConEmu->mp_TabBar)
				result = gpConEmu->mp_TabBar->OnNotify((LPNMHDR)lParam);

			break;
		}
		case WM_COMMAND:
		{
			if (gpConEmu->mp_TabBar)
				gpConEmu->mp_TabBar->OnCommand(wParam, lParam);

			result = 0;
			break;
		}
		case WM_INITMENUPOPUP:
		{
			return gpConEmu->OnInitMenuPopup(hWnd, (HMENU)wParam, lParam);
		}
		case WM_MENUSELECT:
		{
			switch (gpConEmu->mn_TrackMenuPlace)
			{
			case tmp_Cmd:
				if (gpConEmu->mp_TabBar)
					gpConEmu->mp_TabBar->OnMenuSelected((HMENU)lParam, LOWORD(wParam), HIWORD(wParam));
				return 0;
			case tmp_System:
			case tmp_VCon:
				gpConEmu->ShowMenuHint((HMENU)lParam, LOWORD(wParam), HIWORD(wParam));
				return 0;
			case tmp_KeyBar:
				gpConEmu->ShowKeyBarHint((HMENU)lParam, LOWORD(wParam), HIWORD(wParam));
				return 0;
			case tmp_None:
				break;
			}
			// Else ...
			return 0;
		}
		case WM_ERASEBKGND:
			//return 0;
			return 1; //2010-10-05
		//case WM_NCPAINT:
		//case WM_NCACTIVATE:
		//case WM_NCCALCSIZE:
		//case WM_NCHITTEST:
		//case 0x31E: // WM_DWMCOMPOSITIONCHANGED:
		//case 0xAE: // WM_NCUAHDRAWCAPTION:
		//case 0xAF: // WM_NCUAHDRAWFRAME:
		//	//result = gpConEmu->OnNcPaint((HRGN)wParam);
		//	result = gpConEmu->OnNcMessage(hWnd, messg, wParam, lParam);
		//	break;
		//case WM_PAINT:
		//	result = gpConEmu->OnPaint(wParam, lParam);
		//	break;
		case WM_TIMER:
			result = gpConEmu->OnTimer(wParam, lParam);
			break;
		case WM_SIZING:
		{
			RECT* pRc = (RECT*)lParam;
			wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_SIZING (Edge%i, {%i-%i}-{%i-%i})\n", (DWORD)wParam, pRc->left, pRc->top, pRc->right, pRc->bottom);
			DEBUGSTRSIZE(szDbg);

			if (!isIconic())
				result = gpConEmu->OnSizing(wParam, lParam);
		} break;
//#ifdef _DEBUG
		case WM_SHOWWINDOW:
		{
			wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_SHOWWINDOW (Show=%i, Status=%i)\n", (DWORD)wParam, (DWORD)lParam);
			DEBUGSTRSIZE(szDbg);
			result = DefWindowProc(hWnd, messg, wParam, lParam);
			if (wParam == FALSE)
			{
				if (!Icon.isWindowInTray() && !Icon.isHidingToTray())
				{
					//_ASSERTE(Icon.isHidingToTray());
					Icon.HideWindowToTray(gpSet->isDownShowHiddenMessage ? NULL : _T("ConEmu was hidden from some program"));
					gpConEmu->mb_ExternalHidden = TRUE;
				}
			}
			else
			{
				if (gpConEmu->mb_ExternalHidden)
					Icon.RestoreWindowFromTray(TRUE);
				gpConEmu->mb_ExternalHidden = FALSE;
			}
		} break;
//#endif
		case WM_SIZE:
		{
#ifdef _DEBUG
			DWORD_PTR dwStyle = GetWindowLong(ghWnd, GWL_STYLE);
			wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_SIZE (Type:%i, {%i-%i}) style=0x%08X\n", (DWORD)wParam, LOWORD(lParam), HIWORD(lParam), (DWORD)dwStyle);
			DEBUGSTRSIZE(szDbg);
#endif

			//if (gpSet->isDontMinimize && wParam == SIZE_MINIMIZED) {
			//	result = 0;
			//	break;
			//}
			if (!isIconic())
				result = gpConEmu->OnSize(wParam, LOWORD(lParam), HIWORD(lParam));
		} break;
		case WM_MOVE:
		{
#ifdef _DEBUG
			wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_MOVE ({%i-%i})\n", (int)(SHORT)LOWORD(lParam), (int)(SHORT)HIWORD(lParam));
			DEBUGSTRSIZE(szDbg);
#endif
		} break;
		case WM_WINDOWPOSCHANGING:
		{
			result = gpConEmu->OnWindowPosChanging(hWnd, messg, wParam, lParam);
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
			result = gpConEmu->OnGetMinMaxInfo(pInfo);
			break;
		}
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
			//case WM_CHAR: -- убрал. Теперь пользуем ToUnicodeEx.
			//case WM_SYSCHAR:
			result = gpConEmu->OnKeyboard(hWnd, messg, wParam, lParam);

			if (messg == WM_SYSKEYUP || messg == WM_SYSKEYDOWN)
				result = TRUE;

			//if (messg == WM_SYSCHAR)
			//    return TRUE;
			break;
#ifdef _DEBUG
		case WM_CHAR:
		case WM_SYSCHAR:
		case WM_DEADCHAR:
		{
			wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"%s(%i='%c', Scan=%i, lParam=0x%08X)\n",
			                              (messg == WM_CHAR) ? L"WM_CHAR" : (messg == WM_SYSCHAR) ? L"WM_SYSCHAR" : L"WM_DEADCHAR",
			                              (DWORD)wParam, (wchar_t)wParam, ((DWORD)lParam & 0xFF0000) >> 16, (DWORD)lParam);
			DEBUGSTRCHAR(szDbg);
		}
		break;
#endif
		case WM_ACTIVATE:
#ifdef MSGLOGGER
			result = gpConEmu->OnFocus(hWnd, messg, wParam, lParam);
			result = DefWindowProc(hWnd, messg, wParam, lParam);
			break;
#endif
		case WM_ACTIVATEAPP:
#ifdef MSGLOGGER
			result = gpConEmu->OnFocus(hWnd, messg, wParam, lParam);
			result = DefWindowProc(hWnd, messg, wParam, lParam);
			break;
#endif
		case WM_KILLFOCUS:
#ifdef MSGLOGGER
			result = gpConEmu->OnFocus(hWnd, messg, wParam, lParam);
			result = DefWindowProc(hWnd, messg, wParam, lParam);
			break;
#endif
		case WM_SETFOCUS:
			result = gpConEmu->OnFocus(hWnd, messg, wParam, lParam);
			result = DefWindowProc(hWnd, messg, wParam, lParam); //-V519
			break;
		case WM_MOUSEACTIVATE:
			//return MA_ACTIVATEANDEAT; -- ест все подряд, а LBUTTONUP пропускает :(
			gpConEmu->mouse.nSkipEvents[0] = 0;
			gpConEmu->mouse.nSkipEvents[1] = 0;

			if (gpConEmu->mouse.bForceSkipActivation  // принудительная активация окна, лежащего на Desktop
			        || (gpSet->isMouseSkipActivation && LOWORD(lParam) == HTCLIENT && GetForegroundWindow() != ghWnd))
			{
				gpConEmu->mouse.bForceSkipActivation = FALSE; // Однократно
				POINT ptMouse = {0}; GetCursorPos(&ptMouse);
				//RECT  rcDC = {0}; GetWindowRect('ghWnd DC', &rcDC);
				//if (PtInRect(&rcDC, ptMouse))
				CVirtualConsole* pVCon = GetVConFromPoint(ptMouse);
				if (pVCon)
				{
					if (HIWORD(lParam) == WM_LBUTTONDOWN)
					{
						gpConEmu->mouse.nSkipEvents[0] = WM_LBUTTONDOWN;
						gpConEmu->mouse.nSkipEvents[1] = WM_LBUTTONUP;
						gpConEmu->mouse.nReplaceDblClk = WM_LBUTTONDBLCLK;
					}
					else if (HIWORD(lParam) == WM_RBUTTONDOWN)
					{
						gpConEmu->mouse.nSkipEvents[0] = WM_RBUTTONDOWN;
						gpConEmu->mouse.nSkipEvents[1] = WM_RBUTTONUP;
						gpConEmu->mouse.nReplaceDblClk = WM_RBUTTONDBLCLK;
					}
					else if (HIWORD(lParam) == WM_MBUTTONDOWN)
					{
						gpConEmu->mouse.nSkipEvents[0] = WM_MBUTTONDOWN;
						gpConEmu->mouse.nSkipEvents[1] = WM_MBUTTONUP;
						gpConEmu->mouse.nReplaceDblClk = WM_MBUTTONDBLCLK;
					}
				}
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
			result = gpConEmu->OnMouse(hWnd, messg, wParam, lParam);
			break;
		case WM_CLOSE:
			result = gpConEmu->OnClose(hWnd);
			break;
		case WM_SYSCOMMAND:
			result = gpConEmu->OnSysCommand(hWnd, wParam, lParam);
			break;
		case WM_NCLBUTTONDOWN:
			// При ресайзе WM_NCLBUTTONUP к сожалению не приходит
			gpConEmu->mouse.state |= MOUSE_SIZING_BEGIN;

			if (gpSet->isHideCaptionAlways())
			{
				KillTimer(ghWnd, TIMER_CAPTION_DISAPPEAR_ID);
				gpConEmu->OnTimer(TIMER_CAPTION_APPEAR_ID,0);
				UpdateWindowRgn();
			}

			result = DefWindowProc(hWnd, messg, wParam, lParam);
			break;
		case WM_NCLBUTTONDBLCLK:
			gpConEmu->mouse.state |= MOUSE_SIZING_DBLCKL; // чтобы в консоль не провалился LBtnUp если окошко развернется

			if (gpConEmu->OnMouse_NCBtnDblClk(hWnd, messg, wParam, lParam))
				result = 0;
			else
				result = DefWindowProc(hWnd, messg, wParam, lParam);

			break;
		case WM_NCRBUTTONUP:

			if (wParam == HTCLOSE)
			{
				Icon.HideWindowToTray();
			}

			break;
		case WM_TRAYNOTIFY:
			result = Icon.OnTryIcon(hWnd, messg, wParam, lParam);
			break;
		case WM_HOTKEY:

			// Ctrl+Win+Alt+Space
			if (wParam == HOTKEY_CTRLWINALTSPACE_ID)
			{
				gpConEmu->CtrlWinAltSpace();
			}
			// vmMinimizeRestore -> Win+C
			else if (gpConEmu->mn_MinRestoreRegistered && (int)wParam == gpConEmu->mn_MinRestoreRegistered)
			{
				gpConEmu->OnMinimizeRestore();
			}

			return 0;
		case WM_SETCURSOR:
			result = gpConEmu->OnSetCursor(wParam, lParam);

			if (!result && (lParam != -1))
			{
				// Установка resize-курсоров на рамке, и т.п.
				result = DefWindowProc(hWnd, messg, wParam, lParam);
			}

			MCHKHEAP;
			// If an application processes this message, it should return TRUE to halt further processing or FALSE to continue.
			return result;
		case WM_DESTROY:
			result = gpConEmu->OnDestroy(hWnd);
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
			result = gpConEmu->OnKeyboardIme(hWnd, messg, wParam, lParam);
			break;
		case WM_INPUTLANGCHANGE:
		case WM_INPUTLANGCHANGEREQUEST:

			if (hWnd == ghWnd)
				result = gpConEmu->OnLangChange(messg, wParam, lParam);
			else
				break;

			break;
			//case WM_NCHITTEST:
			//	{
			//		/*result = -1;
			//		if (gpSet->isHideCaptionAlways && gpSet->isTabs) {
			//			if (gpConEmu->mp_TabBar->IsShown()) {
			//				HWND hTabBar = gpConEmu->mp_TabBar->GetTabbar();
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
			//		if (gpSet->isHideCaptionAlways() && !gpConEmu->mp_TabBar->IsShown() && result == HTTOP)
			//			result = HTCAPTION;
			//	} break;
		default:

			if (messg == gpConEmu->mn_MsgPostCreate)
			{
				gpConEmu->PostCreate(TRUE);
				return 0;
			}
			else if (messg == gpConEmu->mn_MsgPostCopy)
			{
				gpConEmu->PostCopy((wchar_t*)lParam, TRUE);
				return 0;
			}
			else if (messg == gpConEmu->mn_MsgMyDestroy)
			{
				//gpConEmu->OnDestroy(hWnd);
				DestroyWindow(hWnd);
				return 0;
			}
			else if (messg == gpConEmu->mn_MsgUpdateSizes)
			{
				gpConEmu->UpdateSizes();
				return 0;
			}
			else if (messg == gpConEmu->mn_MsgUpdateCursorInfo)
			{
				COORD cr; cr.X = LOWORD(wParam); cr.Y = HIWORD(wParam);
				CONSOLE_CURSOR_INFO ci; ci.dwSize = LOWORD(lParam); ci.bVisible = HIWORD(lParam);
				gpConEmu->UpdateCursorInfo(cr, ci);
				return 0;
			}
			else if (messg == gpConEmu->mn_MsgSetWindowMode)
			{
				gpConEmu->SetWindowMode(wParam);
				return 0;
			}
			else if (messg == gpConEmu->mn_MsgUpdateTitle)
			{
				//gpConEmu->UpdateTitle(TitleCmp);
				gpConEmu->UpdateTitle(/*mp_VActive->RCon()->GetTitle()*/);
				return 0;
				//} else if (messg == gpConEmu->mn_MsgAttach) {
				//    return gpConEmu->AttachRequested ( (HWND)wParam, (DWORD)lParam );
			}
			else if (messg == gpConEmu->mn_MsgSrvStarted)
			{
				MsgSrvStartedArg *pArg = (MsgSrvStartedArg*)lParam;
				HWND hWndDC = NULL;
				//111002 - вернуть должен HWND окна отрисовки (дочернее окно ConEmu)

				DWORD nServerPID = pArg->nSrcPID;
				HWND  hWndCon = pArg->hConWnd;
				pArg->timeRecv = timeGetTime();

				#ifdef _DEBUG
				DWORD t1, t2, t3; int iFound = -1;
				#endif

				//gpConEmu->WinEventProc(NULL, EVENT_CONSOLE_START_APPLICATION, hWndCon, (LONG)nServerPID, 0, 0, 0);
				for (size_t i = 0; i < countof(gpConEmu->mp_VCon); i++)
				{
					CVirtualConsole* pVCon = gpConEmu->mp_VCon[i];
					CVConGuard guard(pVCon);
					CRealConsole* pRCon;
					if (pVCon && ((pRCon = pVCon->RCon()) != NULL) && pRCon->isServerCreated())
					{
						if (pRCon->GetServerPID() == nServerPID)
						{
							DEBUGTEST(iFound=i);
							DEBUGTEST(t1=timeGetTime());
							
							pRCon->OnServerStarted(hWndCon, nServerPID);
							
							DEBUGTEST(t2=timeGetTime());
							
							hWndDC = pVCon->GetView();

							DEBUGTEST(t3=timeGetTime());
							break;
						}
					}
				}
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
			else if (messg == gpConEmu->mn_MsgVConTerminated)
			{
#ifdef _DEBUG
				wchar_t szDbg[200];
				lstrcpy(szDbg, L"OnVConTerminated");
				CVirtualConsole* pVCon = (CVirtualConsole*)lParam;

				if (pVCon)
				{
					for (size_t i = 0; i < countof(mp_VCon); i++)
					{
						if (pVCon == mp_VCon[i])
						{
							ConEmuTab tab = {0};
							pVCon->RCon()->GetTab(0, &tab);
							tab.Name[128] = 0; // чтобы не вылезло из szDbg
							wsprintf(szDbg+_tcslen(szDbg), L": #%i: %s", i+1, tab.Name);
							break;
						}
					}
				}

				lstrcat(szDbg, L"\n");
				DEBUGSTRCONS(szDbg);
#endif
				return gpConEmu->OnVConTerminated((CVirtualConsole*)lParam, TRUE);
			}
			else if (messg == gpConEmu->mn_MsgUpdateScrollInfo)
			{
				return OnUpdateScrollInfo(TRUE);
			}
			else if (messg == gpConEmu->mn_MsgUpdateTabs)
			{
				DEBUGSTRTABS(L"OnUpdateTabs\n");
				gpConEmu->mp_TabBar->Update(TRUE);
				return 0;
			}
			else if (messg == gpConEmu->mn_MsgOldCmdVer)
			{
				CESERVER_REQ_HDR* p = (CESERVER_REQ_HDR*)lParam;
				_ASSERTE(p->cbSize == sizeof(CESERVER_REQ_HDR));
				gpConEmu->ReportOldCmdVersion(p->nCmd, p->nVersion, (int)wParam, p->nSrcPID, p->hModule, p->nBits);
				return 0;
			}
			else if (messg == gpConEmu->mn_MsgTabCommand)
			{
				gpConEmu->TabCommand((ConEmuTabCommand)(int)wParam);
				return 0;
			}
			else if (messg == gpConEmu->mn_MsgTabSwitchFromHook)
			{
				gpConEmu->TabCommand((wParam & cvk_Shift) ? ctc_SwitchPrev : ctc_SwitchNext);
				if (!gpSet->isTabLazy)
				{
					mb_InWinTabSwitch = FALSE;
					gpConEmu->TabCommand(ctc_SwitchCommit);
				}
				else
				{
					mb_InWinTabSwitch = TRUE;
				}
				return 0;
			}
			else if (messg == gpConEmu->mn_MsgWinKeyFromHook)
			{
				// Тут должно передавать ВСЕ модификаторы!
				DWORD VkMod = (DWORD)wParam;
				gpConEmu->OnKeyboardHook(VkMod);
				//BOOL lbShift = (lParam!=0);
				//OnKeyboardHook(vk, lbShift);
				return 0;
			}
			#if 0
			else if (messg == gpConEmu->mn_MsgConsoleHookedKey)
			{
				WORD vk = LOWORD(wParam);
				gpConEmu->OnConsoleKey(vk, lParam);
				return 0;
			}
			#endif
			else if (messg == gpConEmu->mn_MsgSheelHook)
			{
				gpConEmu->OnShellHook(wParam, lParam);
				return 0;
			}
			else if (messg == gpConEmu->mn_ShellExecuteEx)
			{
				return gpConEmu->GuiShellExecuteEx((SHELLEXECUTEINFO*)lParam, wParam!=0);
			}
			else if (messg == gpConEmu->mn_PostConsoleResize)
			{
				gpConEmu->OnConsoleResize(TRUE);
				return 0;
			}
			else if (messg == gpConEmu->mn_ConsoleLangChanged)
			{
				gpConEmu->OnLangChangeConsole((CVirtualConsole*)lParam, (DWORD)wParam);
				return 0;
			}
			else if (messg == gpConEmu->mn_MsgPostOnBufferHeight)
			{
				gpConEmu->OnBufferHeight();
				return 0;
				//} else if (messg == gpConEmu->mn_MsgSetForeground) {
				//	apiSetForegroundWindow((HWND)lParam);
				//	return 0;
			}
			else if (messg == gpConEmu->mn_MsgFlashWindow)
			{
				return OnFlashWindow((DWORD)(wParam & 0xFF000000) >> 24, (DWORD)wParam & 0xFFFFFF, (HWND)lParam);
			}
			else if (messg == gpConEmu->mn_MsgPostAltF9)
			{
				OnAltF9(TRUE);
				return 0;
			}
			else if (messg == gpConEmu->mn_MsgInitInactiveDC)
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
			else if (messg == gpConEmu->mn_MsgUpdateProcDisplay)
			{
				gpConEmu->UpdateProcessDisplay(wParam!=0);
				return 0;
			}
			else if (messg == gpConEmu->mn_MsgAutoSizeFont)
			{
				gpSetCls->MacroFontSetSize((int)wParam, (int)lParam);
				return 0;
			}
			else if (messg == gpConEmu->mn_MsgMacroFontSetName)
			{
				gpConEmu->PostMacroFontSetName((wchar_t*)lParam, (WORD)(((DWORD)wParam & 0xFFFF0000)>>16), (WORD)(wParam & 0xFFFF), TRUE);
				return 0;
			}
			else if (messg == gpConEmu->mn_MsgDisplayRConError)
			{
				CRealConsole *pRCon = (CRealConsole*)wParam;
				wchar_t* pszErrMsg = (wchar_t*)lParam;

				if (gpConEmu->isValid(pRCon))
					MessageBox(ghWnd, pszErrMsg, gpConEmu->GetLastTitle(), MB_ICONSTOP|MB_SYSTEMMODAL);

				free(pszErrMsg);
				return 0;
			}
			else if (messg == gpConEmu->mn_MsgCreateViewWindow)
			{
				CConEmuChild* pChild = (CConEmuChild*)lParam;
				HWND hChild = pChild ? pChild->CreateView() : NULL;
				return (LRESULT)hChild;
			}
			else if (messg == gpConEmu->mn_MsgPostTaskbarActivate)
			{
				mb_PostTaskbarActivate = FALSE;
				HWND hFore = GetForegroundWindow();
				if (gpConEmu->mp_VActive && hFore == ghWnd)
					gpConEmu->mp_VActive->OnTaskbarFocus();
				return 0;
			}
			else if (messg == gpConEmu->mn_MsgInitVConGhost)
			{
				CVirtualConsole* pVCon = (CVirtualConsole*)lParam;
				if (gpConEmu->isValid(pVCon))
					pVCon->InitGhost();
				return 0;
			}
			else if (messg == gpConEmu->mn_MsgCreateCon)
			{
				_ASSERTE(wParam == gpConEmu->mn_MsgCreateCon || wParam == (gpConEmu->mn_MsgCreateCon+1));
				RConStartArgs *pArgs = (RConStartArgs*)lParam;
				CVirtualConsole* pVCon = CreateCon(pArgs, (wParam == (gpConEmu->mn_MsgCreateCon+1)));
				UNREFERENCED_PARAMETER(pVCon);
				delete pArgs;
				return (LRESULT)pVCon;
			}
			else if (messg == gpConEmu->mn_MsgRequestUpdate)
			{
				switch (wParam)
				{
				case 0:
					gpConEmu->ReportUpdateError();
					return 0;
				case 1:
					return (LRESULT)gpConEmu->ReportUpdateConfirmation();
				case 2:
					gpConEmu->RequestExitUpdate();
					return 0;
				}
				return 0;
			}
			else if (messg == gpConEmu->mn_MsgTaskBarCreated)
			{
				Icon.OnTaskbarCreated();
				if (mp_DragDrop)
					mp_DragDrop->OnTaskbarCreated();
			}

			//else if (messg == gpConEmu->mn_MsgCmdStarted || messg == gpConEmu->mn_MsgCmdStopped) {
			//  return gpConEmu->OnConEmuCmd( (messg == gpConEmu->mn_MsgCmdStarted), (HWND)wParam, (DWORD)lParam);
			//}


			if (messg)
			{
				if (messg == WM_CONTEXTMENU)
				{
					mn_TrackMenuPlace = tmp_System;
					mp_Tip->HideTip();
				}

				result = DefWindowProc(hWnd, messg, wParam, lParam);

				if (messg == WM_CONTEXTMENU)
				{
					mn_TrackMenuPlace = tmp_None;
					mp_Tip->HideTip();
				}
			}
	}

	return result;
}
