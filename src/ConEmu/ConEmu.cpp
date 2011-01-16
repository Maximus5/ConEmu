
/*
Copyright (c) 2009-2010 Maximus5
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


#define SHOWDEBUGSTR

#define CHILD_DESK_MODE

#include "Header.h"
#include <Tlhelp32.h>
#include <Shlobj.h>
#include <lm.h>
//#include "../common/ConEmuCheck.h"
#include "VirtualConsole.h"
#include "options.h"
#include "DragDrop.h"
#include "TrayIcon.h"
#include "ConEmuChild.h"
#include "ConEmu.h"
#include "ConEmuApp.h"
#include "tabbar.h"
#include "ConEmuPipe.h"

#define DEBUGSTRSYS(s) //DEBUGSTR(s)
#define DEBUGSTRSIZE(s) //DEBUGSTR(s)
#define DEBUGSTRCONS(s) //DEBUGSTR(s)
#define DEBUGSTRTABS(s) //DEBUGSTR(s)
#define DEBUGSTRLANG(s) //DEBUGSTR(s)// ; Sleep(2000)
#define DEBUGSTRMOUSE(s) //DEBUGSTR(s)
#define DEBUGSTRKEY(s) DEBUGSTR(s)
#define DEBUGSTRCHAR(s) //DEBUGSTR(s)
#define DEBUGSTRSETCURSOR(s) //DEBUGSTR(s)
#define DEBUGSTRCONEVENT(s) //DEBUGSTR(s)
#define DEBUGSTRMACRO(s) //DEBUGSTR(s)
#define DEBUGSTRPANEL(s) //DEBUGSTR(s)
#define DEBUGSTRPANEL2(s) //DEBUGSTR(s)
#define DEBUGSTRFOCUS(s) //DEBUGSTR(s)
#define DEBUGSTRFOREGROUND(s) //DEBUGSTR(s)
#define DEBUGSTRLLKB(s) //DEBUGSTR(s)
#ifdef _DEBUG
	//#define DEBUGSHOWFOCUS(s) DEBUGSTR(s)
#endif

#define PROCESS_WAIT_START_TIME 1000

#define PTDIFFTEST(C,D) PtDiffTest(C, LOWORD(lParam), HIWORD(lParam), D)
//(((abs(C.x-LOWORD(lParam)))<D) && ((abs(C.y-HIWORD(lParam)))<D))


// COM TaskBarList interface support
#ifdef __GNUC__
	#include "ShObjIdl_Part.h"
	const CLSID CLSID_TaskbarList = {0x56FDF344, 0xFD6D, 0x11d0, {0x95, 0x8A, 0x00, 0x60, 0x97, 0xC9, 0xA0, 0x90}};
	const IID IID_ITaskbarList3 = {0xea1afb91, 0x9e28, 0x4b86, {0x90, 0xe9, 0x9e, 0x9f, 0x8a, 0x5e, 0xef, 0xaf}};
	const IID IID_ITaskbarList2 = {0x602D4995, 0xB13A, 0x429b, {0xA6, 0x6E, 0x19, 0x35, 0xE4, 0x4F, 0x43, 0x17}};
#else
	#include <ShObjIdl.h>
	#ifndef __ITaskbarList3_INTERFACE_DEFINED__
		#undef __shobjidl_h__
		#include "ShObjIdl_Part.h"
		const IID IID_ITaskbarList3 = {0xea1afb91, 0x9e28, 0x4b86, {0x90, 0xe9, 0x9e, 0x9f, 0x8a, 0x5e, 0xef, 0xaf}};
	#endif
#endif


#if defined(__GNUC__)
#define EXT_GNUC_LOG
#endif


#define TIMER_MAIN_ID 0
#define TIMER_CONREDRAW_ID 1
#define TIMER_CAPTION_APPEAR_ID 3
#define TIMER_CAPTION_DISAPPEAR_ID 4
#define TIMER_RCLICKPAINT 5
#define TIMER_RCLICKPAINT_ELAPSE 20

#define RCLICKAPPSTIMEOUT 600
#define RCLICKAPPS_START 200 // начало отрисовки кружка вокруг курсора
#define RCLICKAPPSTIMEOUT_MAX 10000
#define RCLICKAPPSDELTA 3


CConEmuMain::CConEmuMain()
{
	mp_TabBar = NULL;
	ms_ConEmuAliveEvent[0] = 0;	mb_AliveInitialized = FALSE; mh_ConEmuAliveEvent = NULL; mb_ConEmuAliveOwned = FALSE;

    mn_MainThreadId = GetCurrentThreadId();
    wcscpy(szConEmuVersion, L"?.?.?.?");
    WindowMode=rNormal; mb_PassSysCommand = false; change2WindowMode = -1;
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
    Title[0] = 0; TitleCmp[0] = 0; /*MultiTitle[0] = 0;*/ mn_Progress = -1;
    mb_InTimer = FALSE;
    //mb_InClose = FALSE;
    //memset(m_ProcList, 0, 1000*sizeof(DWORD)); 
    m_ProcCount=0;
    mb_ProcessCreated = FALSE; mn_StartTick = 0;
    mb_IgnoreSizeChange = false;
    //mn_CurrentKeybLayout = (DWORD_PTR)GetKeyboardLayout(0);
    mn_GuiServerThreadId = 0; mh_GuiServerThread = NULL; mh_GuiServerThreadTerminate = NULL;
    //mpsz_RecreateCmd = NULL;
    mh_RecreateDlgKeyHook = NULL; mb_SkipAppsInRecreate = FALSE;
	ZeroStruct(mrc_Ideal);
	mn_InResize = 0;
	mb_MaximizedHideCaption = FALSE;
	mb_InRestore = FALSE;
	mb_MouseCaptured = FALSE;
	mb_HotKeyRegistered = FALSE;
	mn_MinRestoreRegistered = 0; mn_MinRestore_VK = mn_MinRestore_MOD = 0;
	mh_LLKeyHookDll = NULL;
	mh_LLKeyHook = NULL;
	mh_DwmApi = NULL;
	mh_RightClickingBmp = NULL; mh_RightClickingDC = NULL;
	m_RightClickingSize.x = m_RightClickingSize.y = m_RightClickingFrames = 0; m_RightClickingCurrent = -1;
	mb_WaitCursor = FALSE;
	mb_InTrackSysMenu = FALSE;
	mb_LastRgnWasNull = TRUE;
	mb_CaptionWasRestored = FALSE; mb_ForceShowFrame = FALSE;
	mh_CursorWait = LoadCursor(NULL, IDC_WAIT);
	mh_CursorArrow = LoadCursor(NULL, IDC_ARROW);
	mh_CursorMove = LoadCursor(NULL, IDC_SIZEALL);
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

	mpsz_ConEmuArgs = NULL;
    ms_ConEmuExe[0] = ms_ConEmuExeDir[0] = ms_ConEmuBaseDir[0] = 0;
	ms_ConEmuCExe[0] = ms_ConEmuCExeFull[0] = 0;
	ms_ConEmuXml[0] = ms_ConEmuChm[0] = 0;
	//ms_ConEmuCExeName[0] = 0;

    wchar_t *pszSlash = NULL;
    if (!GetModuleFileName(NULL, ms_ConEmuExe, MAX_PATH) || !(pszSlash = wcsrchr(ms_ConEmuExe, L'\\')))
	{
        DisplayLastError(L"GetModuleFileName failed");
        TerminateProcess(GetCurrentProcess(), 100);
        return;
    }
    LoadVersionInfo(ms_ConEmuExe);
	// Папка программы
	lstrcpy(ms_ConEmuExeDir, ms_ConEmuExe);
	pszSlash = wcsrchr(ms_ConEmuExeDir, L'\\');
	*pszSlash = 0;

	bool lbBaseFound = false;
	wchar_t szBaseFile[MAX_PATH+12];
	lstrcpy(szBaseFile, ms_ConEmuExeDir);
	// Сначала проверяем подпапку
	pszSlash = szBaseFile + lstrlen(szBaseFile);
	lstrcpy(pszSlash, L"\\ConEmu\\ConEmuC.exe");
	if (FileExists(szBaseFile))
	{
		lstrcpy(ms_ConEmuBaseDir, ms_ConEmuExeDir);
		lstrcat(ms_ConEmuBaseDir, L"\\ConEmu");
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
		lstrcpy(ms_ConEmuBaseDir, ms_ConEmuExeDir);
	}


    // Добавить в окружение переменную с папкой к ConEmu.exe
	SetEnvironmentVariable(L"ConEmuDir", ms_ConEmuExeDir);
	SetEnvironmentVariable(L"ConEmuBaseDir", ms_ConEmuBaseDir);


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
	lstrcpy(ms_ConEmuChm, ms_ConEmuBaseDir); lstrcat(ms_ConEmuChm, L"\\ConEmu.chm");
	if (!FileExists(ms_ConEmuChm))
	{
		if (lstrcmp(ms_ConEmuBaseDir, ms_ConEmuExeDir))
		{
			lstrcpy(ms_ConEmuChm, ms_ConEmuExeDir); lstrcat(ms_ConEmuChm, L"\\ConEmu.chm");
			if (!FileExists(ms_ConEmuChm))
				ms_ConEmuChm[0] = 0;
		}
		else
		{
			ms_ConEmuChm[0] = 0;
		}
	}


	lstrcpy(ms_ConEmuCExeFull, ms_ConEmuBaseDir);
	pszSlash = ms_ConEmuCExeFull + lstrlen(ms_ConEmuCExeFull);
	#ifdef WIN64
	lstrcpy(pszSlash, L"\\ConEmuC64.exe");
	if (!FileExists(ms_ConEmuCExeFull))
	#endif
	{
		lstrcpy(pszSlash, L"\\ConEmuC.exe");
	}
	
	// Если ConEmu.exe запущен с сетевого ресурса -  Сетевые пути не менять
	if (ms_ConEmuExe[0] == L'\\' /*|| wcschr(ms_ConEmuExe, L' ') == NULL*/)
	{
		lstrcpy(ms_ConEmuCExe, ms_ConEmuCExeFull);
	}
	else
	{
		wchar_t* pszShort = GetShortFileNameEx(ms_ConEmuCExeFull);
		if (pszShort)
		{
			wcscpy(ms_ConEmuCExe, pszShort);
			free(pszShort);
		}
		else
		{
			wcscpy(ms_ConEmuCExe, ms_ConEmuCExeFull);
		}
		//pszSlash = ms_ConEmuCExe + lstrlen(ms_ConEmuCExe);
		//if (*(pszSlash-1) != L'\\')
		//{
		//	*(pszSlash++) = L'\\'; *pszSlash = 0;
		//}
	}
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


    memset(&m_osv,0,sizeof(m_osv));	
    m_osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&m_osv);
	if (m_osv.dwMajorVersion >= 6)
	{
		mb_IsUacAdmin = IsUserAdmin(); // Чтобы знать, может мы уже запущены под UAC админом?
	}
	else
	{
		mb_IsUacAdmin = FALSE;
	}


    mh_WinHook = NULL;
	//mh_PopupHook = NULL;
    mp_TaskBar2 = NULL;
	mp_TaskBar3 = NULL;

    mp_VActive = NULL; mp_VCon1 = NULL; mp_VCon2 = NULL; mb_CreatingActive = false;
    memset(mp_VCon, 0, sizeof(mp_VCon));
    
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
	mn_MsgActivateCon = RegisterWindowMessage(CONEMUMSG_ACTIVATECON);
	mn_MsgUpdateProcDisplay = ++nAppMsg;
	//// В Win7x64 WM_INPUTLANGCHANGEREQUEST не приходит (по крайней мере при переключении мышкой)
	//wmInputLangChange = WM_INPUTLANGCHANGE;
}

LPWSTR CConEmuMain::ConEmuXml()
{
	if (ms_ConEmuXml[0])
	{
		if (FileExists(ms_ConEmuXml))
			return ms_ConEmuXml;
	}

	// Ищем файл портабельных настроек. Сначала пробуем в BaseDir
    lstrcpy(ms_ConEmuXml, ms_ConEmuBaseDir); lstrcat(ms_ConEmuXml, L"\\ConEmu.xml");
	if (!FileExists(ms_ConEmuXml))
	{
		if (lstrcmp(ms_ConEmuBaseDir, ms_ConEmuExeDir))
		{
			lstrcpy(ms_ConEmuXml, ms_ConEmuExeDir); lstrcat(ms_ConEmuXml, L"\\ConEmu.xml");
		}
	}

	return ms_ConEmuXml;
}

BOOL CConEmuMain::Init()
{
	_ASSERTE(mp_TabBar == NULL);
	mp_TabBar = new TabBarClass();
	m_Child = new CConEmuChild();
	m_Back = new CConEmuBack();

    //#pragma message("Win2k: EVENT_CONSOLE_START_APPLICATION, EVENT_CONSOLE_END_APPLICATION")
    //Нас интересуют только START и END. Все остальные события приходят от ConEmuC через серверный пайп
    //#if defined(__GNUC__)
    //HMODULE hUser32 = GetModuleHandle(L"user32.dll");
    //FSetWinEventHook SetWinEventHook = (FSetWinEventHook)GetProcAddress(hUser32, "SetWinEventHook");
    //#endif
    mh_WinHook = SetWinEventHook(EVENT_CONSOLE_START_APPLICATION/*EVENT_CONSOLE_CARET*/,EVENT_CONSOLE_END_APPLICATION,
        NULL, (WINEVENTPROC)CConEmuMain::WinEventProc, 0,0, WINEVENT_OUTOFCONTEXT);
	
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
    wsprintf(szErr, _T("Can't initialize psapi!\r\nLastError = 0x%08x"), dwErr);
    MBoxA(szErr);
    return FALSE;*/

    return TRUE;
}

RECT CConEmuMain::GetDefaultRect()
{
	int nWidth, nHeight;
	RECT rcWnd;
	MBoxAssert(gSet.FontWidth() && gSet.FontHeight());

	COORD conSize; conSize.X=gSet.wndWidth; conSize.Y=gSet.wndHeight;
	//int nShiftX = GetSystemMetrics(SM_CXSIZEFRAME)*2;
	//int nShiftY = GetSystemMetrics(SM_CYSIZEFRAME)*2 + (gSet.isHideCaptionAlways ? 0 : GetSystemMetrics(SM_CYCAPTION));
	RECT rcFrameMargin = CalcMargins(CEM_FRAME);
	int nShiftX = rcFrameMargin.left + rcFrameMargin.right;
	int nShiftY = rcFrameMargin.top + rcFrameMargin.bottom;

	// Если табы показываются всегда - сразу добавим их размер, чтобы размер консоли был заказанным
	nWidth  = conSize.X * gSet.FontWidth() + nShiftX 
		+ ((gSet.isTabs == 1) ? (gSet.rcTabMargins.left+gSet.rcTabMargins.right) : 0);
	nHeight = conSize.Y * gSet.FontHeight() + nShiftY 
		+ ((gSet.isTabs == 1) ? (gSet.rcTabMargins.top+gSet.rcTabMargins.bottom) : 0);

	rcWnd = MakeRect(gSet.wndX, gSet.wndY, gSet.wndX+nWidth, gSet.wndY+nHeight);

	if (gSet.wndCascade) {
		RECT rcScreen = MakeRect(800,600);
		int nMonitors = GetSystemMetrics(SM_CMONITORS);
		if (nMonitors > 1) {
			// Размер виртуального экрана по всем мониторам
			rcScreen.left = GetSystemMetrics(SM_XVIRTUALSCREEN); // may be <0
			rcScreen.top  = GetSystemMetrics(SM_YVIRTUALSCREEN);
			rcScreen.right = rcScreen.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
			rcScreen.bottom = rcScreen.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
			TODO("Хорошо бы исключить из рассмотрения Taskbar...");
		} else {
			SystemParametersInfo(SPI_GETWORKAREA, 0, &rcScreen, 0);
		}

		//RECT rcDefault = gConEmu.GetDefaultRect();
		int nX = GetSystemMetrics(SM_CXSIZEFRAME), nY = GetSystemMetrics(SM_CYSIZEFRAME);
		int nWidth = rcWnd.right - rcWnd.left;
		int nHeight = rcWnd.bottom - rcWnd.top;
		// Теперь, если новый размер выходит за пределы экрана - сдвинуть в левый верхний угол
		BOOL lbX = ((rcWnd.left+nWidth)>(rcScreen.right+nX));
		BOOL lbY = ((rcWnd.top+nHeight)>(rcScreen.bottom+nY));

		if (lbX && lbY) {
			rcWnd = MakeRect(rcScreen.left,rcScreen.top,rcScreen.left+nWidth,rcScreen.top+nHeight);
		} else if (lbX) {
			rcWnd.left = rcScreen.right - nWidth; rcWnd.right = rcScreen.right;
		} else if (lbY) {
			rcWnd.top = rcScreen.bottom - nHeight; rcWnd.bottom = rcScreen.bottom;
		}

		if (rcWnd.left<(rcScreen.left-nX)) {
			rcWnd.left=rcScreen.left-nX; rcWnd.right=rcWnd.left+nWidth;
		}
		if (rcWnd.top<(rcScreen.top-nX)) {
			rcWnd.top=rcScreen.top-nX; rcWnd.bottom=rcWnd.top+nHeight;
		}
		if ((rcWnd.left+nWidth)>(rcScreen.right+nX)) {
			rcWnd.left = max((rcScreen.left-nX),(rcScreen.right-nWidth));
			nWidth = min(nWidth, (rcScreen.right-rcWnd.left+2*nX));
			rcWnd.right = rcWnd.left + nWidth;
		}
		if ((rcWnd.top+nHeight)>(rcScreen.bottom+nY)) {
			rcWnd.top = max((rcScreen.top-nY),(rcScreen.bottom-nHeight));
			nHeight = min(nHeight, (rcScreen.bottom-rcWnd.top+2*nY));
			rcWnd.bottom = rcWnd.top + nHeight;
		}

		// Скорректировать X/Y при каскаде
		gSet.wndX = rcWnd.left;
		gSet.wndY = rcWnd.top;
	}

	return rcWnd;
}

RECT CConEmuMain::GetVirtualScreenRect(BOOL abFullScreen)
{
	RECT rcScreen = MakeRect(800,600);
	int nMonitors = GetSystemMetrics(SM_CMONITORS);
	if (nMonitors > 1) {
		// Размер виртуального экрана по всем мониторам
		rcScreen.left = GetSystemMetrics(SM_XVIRTUALSCREEN); // may be <0
		rcScreen.top  = GetSystemMetrics(SM_YVIRTUALSCREEN);
		rcScreen.right = rcScreen.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
		rcScreen.bottom = rcScreen.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
		// Хорошо бы исключить из рассмотрения Taskbar и прочие панели
		if (!abFullScreen) {
			RECT rcPrimaryWork;
			if (SystemParametersInfo(SPI_GETWORKAREA, 0, &rcPrimaryWork, 0)) {
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
	} else {
		if (abFullScreen) {
			rcScreen = MakeRect(GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN));
		} else {
			SystemParametersInfo(SPI_GETWORKAREA, 0, &rcScreen, 0);
		}
	}
	if (rcScreen.right<=rcScreen.left || rcScreen.bottom<=rcScreen.top) {
		_ASSERTE(rcScreen.right>rcScreen.left && rcScreen.bottom>rcScreen.top);
		rcScreen = MakeRect(800,600);
	}
	return rcScreen;
}

// Эта функция расчитывает необходимые стили по текущим настройкам, а не возвращает GWL_STYLE
DWORD CConEmuMain::GetWindowStyle()
{
	DWORD style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

	//if (gSet.isShowOnTaskBar) // ghWndApp
	//	style |= WS_POPUPWINDOW | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
	//else
	style |= WS_OVERLAPPEDWINDOW;

#ifndef CHILD_DESK_MODE
	if (gSet.isDesktopMode)
		style |= WS_POPUP;
#endif

	//if (ghWnd) {
	//	if (gSet.isHideCaptionAlways)
	//		style &= ~(WS_CAPTION/*|WS_THICKFRAME*/);
	//	else
	//		style |= (WS_CAPTION|/*WS_THICKFRAME|*/WS_MINIMIZEBOX|WS_MAXIMIZEBOX);
	//}

	return style;
}

// Эта функция расчитывает необходимые стили по текущим настройкам, а не возвращает GWL_STYLE_EX
DWORD CConEmuMain::GetWindowStyleEx()
{
	DWORD_PTR styleEx = 0;

	if (gSet.nTransparent < 255 /*&& !gSet.isDesktopMode*/)
		styleEx |= WS_EX_LAYERED;

	if (gSet.isAlwaysOnTop)
		styleEx |= WS_EX_TOPMOST;

#ifndef CHILD_DESK_MODE
	if (gSet.isDesktopMode)
		styleEx |= WS_EX_TOOLWINDOW;
#endif

	return styleEx;
}

BOOL CConEmuMain::CreateMainWindow()
{
	if (!Init())
		return FALSE; // Ошибка уже показана

	if (_tcscmp(VirtualConsoleClass,VirtualConsoleClassMain))
	{
		MBoxA(_T("Error: class names must be equal!"));
		return FALSE;
	}

	// 2009-06-11 Возможно, что CS_SAVEBITS приводит к глюкам отрисовки
	// банально непрорисовываются некоторые части экрана (драйвер видюхи глючит?)
	WNDCLASSEX wc = {sizeof(WNDCLASSEX), CS_DBLCLKS|CS_OWNDC/*|CS_SAVEBITS*/, CConEmuMain::MainWndProc, 0, 16, 
		g_hInstance, hClassIcon, LoadCursor(NULL, IDC_ARROW), 
		NULL /*(HBRUSH)COLOR_BACKGROUND*/, 
		NULL, szClassNameParent, hClassIconSm};// | CS_DROPSHADOW
	if (!RegisterClassEx(&wc))
		return -1;

	DWORD styleEx = GetWindowStyleEx();
	DWORD style = GetWindowStyle();
		//	WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
		////if (gSet.isShowOnTaskBar) // ghWndApp
		////	style |= WS_POPUPWINDOW | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
		////else
		//style |= WS_OVERLAPPEDWINDOW;

		//if (gSet.nTransparent < 255 /*&& !gSet.isDesktopMode*/)
		//	styleEx |= WS_EX_LAYERED;
		//if (gSet.isAlwaysOnTop)
		//	styleEx |= WS_EX_TOPMOST;
		////if (gSet.isHideCaptionAlways) // сразу создать так почему-то не получается
		////	style &= ~(WS_CAPTION);
	int nWidth=CW_USEDEFAULT, nHeight=CW_USEDEFAULT;

	// Расчет размеров окна в Normal режиме
	if (gSet.wndWidth && gSet.wndHeight)
	{
		MBoxAssert(gSet.FontWidth() && gSet.FontHeight());

		//COORD conSize; conSize.X=gSet.wndWidth; conSize.Y=gSet.wndHeight;
		////int nShiftX = GetSystemMetrics(SM_CXSIZEFRAME)*2;
		////int nShiftY = GetSystemMetrics(SM_CYSIZEFRAME)*2 + (gSet.isHideCaptionAlways ? 0 : GetSystemMetrics(SM_CYCAPTION));
		//RECT rcFrameMargin = CalcMargins(CEM_FRAME);
		//int nShiftX = rcFrameMargin.left + rcFrameMargin.right;
		//int nShiftY = rcFrameMargin.top + rcFrameMargin.bottom;

		//// Если табы показываются всегда - сразу добавим их размер, чтобы размер консоли был заказанным
		//nWidth  = conSize.X * gSet.FontWidth() + nShiftX 
		//	+ ((gSet.isTabs == 1) ? (gSet.rcTabMargins.left+gSet.rcTabMargins.right) : 0);
		//nHeight = conSize.Y * gSet.FontHeight() + nShiftY 
		//	+ ((gSet.isTabs == 1) ? (gSet.rcTabMargins.top+gSet.rcTabMargins.bottom) : 0);

		//mrc_Ideal = MakeRect(gSet.wndX, gSet.wndY, gSet.wndX+nWidth, gSet.wndY+nHeight);

		mrc_Ideal = GetDefaultRect();
		nWidth = mrc_Ideal.right - mrc_Ideal.left;
		nHeight = mrc_Ideal.bottom - mrc_Ideal.top;
	}

	//if (gConEmu.WindowMode == rMaximized) style |= WS_MAXIMIZE;
	//style |= WS_VISIBLE;
	// cRect.right - cRect.left - 4, cRect.bottom - cRect.top - 4; -- все равно это было не правильно
	ghWnd = CreateWindowEx(styleEx, szClassNameParent, gSet.GetCmd(), style, 
		gSet.wndX, gSet.wndY, nWidth, nHeight, ghWndApp, NULL, (HINSTANCE)g_hInstance, NULL);
	if (!ghWnd)
	{
		if (!ghWndDC) MBoxA(_T("Can't create main window!"));
		return FALSE;
	}
	//if (gSet.isHideCaptionAlways)
	//	OnHideCaption();
	#ifdef _DEBUG
	DWORD style2 = GetWindowLongPtr(ghWnd, GWL_STYLE);
	DWORD styleEx2 = GetWindowLongPtr(ghWnd, GWL_EXSTYLE);
	#endif
	OnTransparent();
	//if (gConEmu.WindowMode == rFullScreen || gConEmu.WindowMode == rMaximized)
	//	gConEmu.SetWindowMode(gConEmu.WindowMode);


	UpdateGuiInfoMapping();
	

	return TRUE;
}

void CConEmuMain::UpdateGuiInfoMapping()
{
	ConEmuGuiInfo ceInfo = {sizeof(ConEmuGuiInfo)};
	ceInfo.hGuiWnd = ghWnd;
	wchar_t *pszSlash = wcsrchr(ms_ConEmuExe, L'\\');
	*pszSlash = 0;
	lstrcpy(ceInfo.sConEmuDir, ms_ConEmuExe); // SetEnvironmentVariable(L"ConEmuDir", ceInfo.sConEmuDir);
	*pszSlash = L'\\';
	lstrcpyn(ceInfo.sConEmuArgs, mpsz_ConEmuArgs ? mpsz_ConEmuArgs : L"", countof(ceInfo.sConEmuArgs)); // Это идет в мэппинг, для информации

	//if (mb_CreateProcessLogged)
	//	lstrcpy(ceInfo.sLogCreateProcess, ms_LogCreateProcess);

	// sConEmuArgs уже заполнен в PrepareCommandLine
	m_GuiInfoMapping.InitName(CEGUIINFOMAPNAME, GetCurrentProcessId());
	if (m_GuiInfoMapping.Create())
	{
		m_GuiInfoMapping.SetFrom(&ceInfo);
	}
#ifdef _DEBUG
	else
	{
		_ASSERTE(FALSE);
	}
#endif
}

HRGN CConEmuMain::CreateWindowRgn(bool abTestOnly/*=false*/)
{
	HRGN hRgn = NULL, hExclusion = NULL;

	//if (isIconic())
	//	return NULL;
	if (mp_VActive/* && abTestOnly*/) {
		hExclusion = mp_VActive->GetExclusionRgn(true);
		if (abTestOnly && hExclusion) {
			_ASSERTE(hExclusion == (HRGN)1);
			return (HRGN)1;
		}
	}

	WARNING("Установка любого НЕ NULL региона сбивает темы при отрисовке кнопок в заголовке");

	if ((gSet.isFullScreen || (isZoomed() && (gSet.isHideCaption || gSet.isHideCaptionAlways()))) 
		&& !mb_InRestore) 
	{
		if (abTestOnly)
			return (HRGN)1;

		RECT rcScreen = CalcRect(CER_FULLSCREEN, MakeRect(0,0), CER_FULLSCREEN);
		RECT rcFrame = CalcMargins(CEM_FRAME);

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
		if (gSet.isHideCaptionAlways())
		{
			if (!isMouseOverFrame())
			{
				// Рамка невидима (мышка не над рамкой или заголовком)
				RECT rcClient; GetClientRect(ghWnd, &rcClient);
				RECT rcFrame = CalcMargins(CEM_FRAME);

				_ASSERTE(!rcClient.left && !rcClient.top);
				hRgn = CreateWindowRgn(abTestOnly, gSet.CheckTheming() && mp_TabBar->IsShown(),
					rcFrame.left-gSet.nHideCaptionAlwaysFrame,
					rcFrame.top-gSet.nHideCaptionAlwaysFrame,
					rcClient.right+2*gSet.nHideCaptionAlwaysFrame,
					rcClient.bottom+2*gSet.nHideCaptionAlwaysFrame);
			}
		}
		if (!hRgn && hExclusion)
		{
			// Таки нужно создать...
			bool bTheming = gSet.CheckTheming();
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
		ptPoly[nPoint++] = MakePoint(anX+anWndWidth-1, anY+4);
		ptPoly[nPoint++] = MakePoint(anX+anWndWidth, anY+6);
		ptPoly[nPoint++] = MakePoint(anX+anWndWidth, anY+anWndHeight);

		hRgn = CreatePolygonRgn(ptPoly, nPoint, WINDING);
	}
	else
	{
		hRgn = CreateRectRgn (anX, anY, anX+anWndWidth, anY+anWndHeight);
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
			//MapWindowPoints(ghWndDC, NULL, &ptShift, 1);
			//RECT rcWnd = GetWindow
			RECT rcFrame = CalcMargins(CEM_FRAME);
			#ifdef _DEBUG
			// CEM_TAB не учитывает центрирование клиентской части в развернутых режимах
			RECT rcTab = CalcMargins(CEM_TAB);
			#endif
			POINT ptClient = {0,0};
			MapWindowPoints(ghWndDC, ghWnd, &ptClient, 1);

			HRGN hOffset = CreateRectRgn(0,0,0,0);
			int n1 = CombineRgn ( hOffset, hExclusion, NULL, RGN_COPY);
			int n2 = OffsetRgn ( hOffset, rcFrame.left+ptClient.x, rcFrame.top+ptClient.y );

			hCombine = CreateRectRgn(0,0,1,1);
			CombineRgn ( hCombine, hRgn, hOffset, RGN_DIFF);
			DeleteObject(hRgn);
			DeleteObject(hOffset); hOffset = NULL;
			hRgn = hCombine; hCombine = NULL;
		}
	}


	return hRgn;
}

void CConEmuMain::Destroy()
{
    for (int i=0; i<MAX_CONSOLE_COUNT; i++) {
        if (mp_VCon[i]) {
            mp_VCon[i]->RCon()->StopSignal();
        }
    }

    if (ghWnd) {
        //HWND hWnd = ghWnd;
        //ghWnd = NULL;
        //DestroyWindow(hWnd); -- может быть вызывано из другой нити
        PostMessage(ghWnd, mn_MsgMyDestroy, GetCurrentThreadId(), 0);
    }
}

CConEmuMain::~CConEmuMain()
{
    for (int i=0; i<MAX_CONSOLE_COUNT; i++) {
        if (mp_VCon[i]) {
			CVirtualConsole* p = mp_VCon[i];
			mp_VCon[i] = NULL;
            delete p;
        }
    }

	CVirtualConsole::ClearPartBrushes();


    if (mh_WinHook) {
        UnhookWinEvent(mh_WinHook);
        mh_WinHook = NULL;
    }
	//if (mh_PopupHook) {
	//	UnhookWinEvent(mh_PopupHook);
	//	mh_PopupHook = NULL;
	//}

    if (mp_DragDrop) {
        delete mp_DragDrop;
        mp_DragDrop = NULL;
    }
    //if (ProgressBars) {
    //    delete ProgressBars;
    //    ProgressBars = NULL;
    //}
    
	if (mp_TabBar) {
		delete mp_TabBar;
		mp_TabBar = NULL;
	}
	if (m_Child) {
		delete m_Child;
		m_Child = NULL;
	}
	if (m_Back) {
		delete m_Back;
		m_Back = NULL;
	}

	_ASSERTE(mh_LLKeyHookDll==NULL);

    CommonShutdown();
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
    {	// подвинуть все грани на rcAddShift (left сдвигается на left, и т.п.)
        if (rcAddShift.left)
            rc.left += rcAddShift.left;
        if (rcAddShift.top)
            rc.top += rcAddShift.top;
        if (rcAddShift.right)
            rc.right -= rcAddShift.right;
        if (rcAddShift.bottom)
            rc.bottom -= rcAddShift.bottom;
    } else {
    	// поправить только правую и нижнюю грань
        if (rcAddShift.right || rcAddShift.left)
            rc.right += rcAddShift.right + rcAddShift.left;
        if (rcAddShift.bottom || rcAddShift.top)
            rc.bottom += rcAddShift.bottom + rcAddShift.top;
    }
}

void CConEmuMain::AskChangeBufferHeight()
{
	// Win7 BUGBUG: Issue 192: падение Conhost при turn bufferheight ON
	// http://code.google.com/p/conemu-maximus5/issues/detail?id=192
	if (gOSVer.dwMajorVersion == 6 && gOSVer.dwMinorVersion == 1)
		return;

	CRealConsole *pRCon = ActiveCon()->RCon();
	if (!pRCon) return;
	BOOL lbBufferHeight = pRCon->isBufferHeight();
	BOOL b = gbDontEnable; gbDontEnable = TRUE;
	int nBtn = MessageBox(ghWnd, lbBufferHeight ? 
			L"Do You want to turn bufferheight OFF?" :
			L"Do You want to turn bufferheight ON?",
			L"ConEmu", MB_ICONQUESTION|MB_OKCANCEL);
	gbDontEnable = b;
	if (nBtn != IDOK) return;

#ifdef _DEBUG
	HANDLE hFarInExecuteEvent = NULL;
	if (!lbBufferHeight) {
		DWORD dwFarPID = pRCon->GetFarPID();
		if (dwFarPID) {
			// Это событие дергается в отладочной (мной поправленной) версии фара
			wchar_t szEvtName[64]; wsprintf(szEvtName, L"FARconEXEC:%08X", (DWORD)pRCon->ConWnd());
			hFarInExecuteEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, szEvtName);
			if (hFarInExecuteEvent) // Иначе ConEmuC вызовет _ASSERTE в отладочном режиме
				SetEvent(hFarInExecuteEvent);
		}
	}
#endif

	pRCon->ChangeBufferHeightMode(!lbBufferHeight);

#ifdef _DEBUG
	if (hFarInExecuteEvent)
		ResetEvent(hFarInExecuteEvent);
#endif

	OnBufferHeight();
}

/*!!!static!!*/
// Функция расчитывает смещения (относительные)
// mg содержит битмаск, например (CEM_FRAME|CEM_TAB|CEM_CLIENT)
RECT CConEmuMain::CalcMargins(DWORD/*enum ConEmuMargins*/ mg, CVirtualConsole* apVCon)
{
    RECT rc = {0,0,0,0};
    
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
			//	+ (gSet.isHideCaptionAlways ? 0 : GetSystemMetrics(SM_CYCAPTION));
		}
		//}
    }
    
    
    // Сколько занимают табы (по идее меняется только rc.top)
    if (mg & ((DWORD)CEM_TAB))
    {
        if (ghWnd)
        {
			bool lbTabActive = ((mg & CEM_TAB_MASK) == CEM_TAB) ? gConEmu.mp_TabBar->IsActive() 
				: ((mg & ((DWORD)CEM_TABACTIVATE)) == ((DWORD)CEM_TABACTIVATE));
				
            // Главное окно уже создано, наличие таба определено
            if (lbTabActive) //TODO: + IsAllowed()?
            {
                RECT rcTab = gConEmu.mp_TabBar->GetMargins();
                AddMargins(rc, rcTab, FALSE);
            }// else { -- раз таба нет - значит дополнительные отступы не нужны
            //    rc = MakeRect(0,0); // раз таба нет - значит дополнительные отступы не нужны
            //}
        }
        else
        {
            // Иначе нужно смотреть по настройкам
            if (gSet.isTabs == 1)
            {
                RECT rcTab = gSet.rcTabMargins; // умолчательные отступы таба
                if (!gSet.isTabFrame)
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
    //    //    if (gConEmu.mp_VActive && gConEmu.mp_VActive->RCon()->isBufferHeight()) { //TODO: а показывается ли сейчас прокрутка?
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
    //	RECT rcDC; GetWindowRect(ghWndDC, &rcDC);
    //	RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);
    //	RECT rcFrameTab = CalcMargins(CEM_FRAMETAB);
    //	// ставим результат
    //	rc.left += (rcDC.left - rcWnd.left - rcFrameTab.left);
    //	rc.top += (rcDC.top - rcWnd.top - rcFrameTab.top);
    //	rc.right -= (rcDC.right - rcWnd.right - rcFrameTab.right);
    //	rc.bottom -= (rcDC.bottom - rcWnd.bottom - rcFrameTab.bottom);
    //}
    
    if ((mg & ((DWORD)CEM_SCROLL)) && (gSet.isAlwaysShowScrollbar == 1))
    {
    	rc.right += GetSystemMetrics(SM_CXVSCROLL);
    }
    
    return rc;
}

/*!!!static!!*/
// Для приблизительного расчета размеров - нужен только (размер главного окна)|(размер консоли)
// Для точного расчета размеров - требуется (размер главного окна) и (размер окна отрисовки) для корректировки
// на x64 возникают какие-то глюки с ",RECT rFrom,". Отладчик показывает мусор в rFrom,
// но тем не менее, после "RECT rc = rFrom;", rc получает правильные значения >:|
RECT CConEmuMain::CalcRect(enum ConEmuRect tWhat, const RECT &rFrom, enum ConEmuRect tFrom, CVirtualConsole* pVCon, RECT* prDC/*=NULL*/, enum ConEmuMargins tTabAction/*=CEM_TAB*/)
{
    RECT rc = rFrom; // инициализация, если уж не получится...
    RECT rcShift = MakeRect(0,0);
	enum ConEmuRect tFromNow = tFrom;

	if (!pVCon)
		pVCon = gConEmu.mp_VActive;
    
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
            {   // Размер окна отрисовки в пикселах!
                //MBoxAssert(!(rFrom.left || rFrom.top));
				TODO("DoubleView");
                
                switch (tWhat)
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
		    {   // Размер консоли в символах!
                //MBoxAssert(!(rFrom.left || rFrom.top));
                _ASSERTE(tWhat!=CER_CONSOLE);
                
                //if (gSet.FontWidth()==0) {
                //    MBoxAssert(gConEmu.mp_VActive!=NULL);
                //    gConEmu.mp_VActive->InitDC(false, true); // инициализировать ширину шрифта по умолчанию
                //}
                
                // ЭТО размер окна отрисовки DC
                rc = MakeRect((rFrom.right-rFrom.left) * gSet.FontWidth(),
					(rFrom.bottom-rFrom.top) * gSet.FontHeight());
                
                if (tWhat != CER_DC)
                    rc = CalcRect(tWhat, rc, CER_DC);

				// -- tFromNow = CER_BACK; -- менять не требуется, т.к. мы сразу на выход
            }
            return rc;
        case CER_FULLSCREEN: // switch (tFrom)
        case CER_MAXIMIZED: // switch (tFrom)
			// Например, таким способом можно получить размер развернутого окна на текущем мониторе
			// RECT rcMax = CalcRect(CER_MAXIMIZED, MakeRect(0,0), CER_MAXIMIZED);
        	_ASSERTE((tFrom!=CER_FULLSCREEN && tFrom!=CER_MAXIMIZED) || (tFrom==tWhat));
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
    
    switch (tWhat)
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
			//         if (gSet.FontWidth()==0 || gSet.FontHeight()==0)
			//             gConEmu.mp_VActive->InitDC(false, true); // инициализировать ширину шрифта по умолчанию
			//rc.right ++;
			//int nShift = (gSet.FontWidth() - 1) / 2; if (nShift < 1) nShift = 1;
			//rc.right += nShift;
			// Если есть вкладки
			//if (rcShift.top || rcShift.bottom || )
			//nShift = (gSet.FontWidth() - 1) / 2; if (nShift < 1) nShift = 1;

            if (tWhat != CER_CONSOLE_NTVDMOFF && pVCon && pVCon->RCon() && pVCon->RCon()->isNtvdm())
            {
                // NTVDM устанавливает ВЫСОТУ экранного буфера... в 25/28/43/50 строк
                // путем округления текущей высоты (то есть если до запуска 16bit
                // было 27 строк, то скорее всего будет установлена высота в 28 строк)
                RECT rc1 = MakeRect(gConEmu.mp_VActive->TextWidth*gSet.FontWidth(), gConEmu.mp_VActive->TextHeight*gSet.FontHeight());
                	//gSet.ntvdmHeight/*mp_VActive->TextHeight*/*gSet.FontHeight());
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
                if (nS>=0) {
                    rcShift.top = nS / 2;
                    rcShift.bottom = nS - rcShift.top;
                } else {
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
				int nW = (rc.right - rc.left + 1) / gSet.FontWidth();
				int nH = (rc.bottom - rc.top) / gSet.FontHeight();
                rc.left = 0; rc.top = 0; rc.right = nW; rc.bottom = nH;

				//2010-01-19
				if (gSet.isFontAutoSize)
				{
					if (gSet.wndWidth && rc.right > (LONG)gSet.wndWidth)
						rc.right = gSet.wndWidth;
					if (gSet.wndHeight && rc.bottom > (LONG)gSet.wndHeight)
						rc.bottom = gSet.wndHeight;
				}

				#ifdef _DEBUG
                _ASSERTE(rc.bottom>=5);
				#endif
				
				// Возможно, что в RealConsole выставлен большой шрифт, который помешает установке этого размера
				if (gConEmu.mp_VActive)
				{
					CRealConsole* pRCon = gConEmu.mp_VActive->RCon();
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
        case CER_FULLSCREEN: // switch (tWhat)
        case CER_MAXIMIZED: // switch (tWhat)
        //case CER_CORRECTED:
        {
            HMONITOR hMonitor = NULL;

            if (ghWnd)
                hMonitor = MonitorFromWindow ( ghWnd, MONITOR_DEFAULTTOPRIMARY );
            else
                hMonitor = MonitorFromRect ( &rFrom, MONITOR_DEFAULTTOPRIMARY );

            //if (tWhat != CER_CORRECTED)
            //    tFrom = tWhat;

            MONITORINFO mi; mi.cbSize = sizeof(mi);
            if (GetMonitorInfo(hMonitor, &mi))
            {
                switch (tFrom)
                {
                case CER_FULLSCREEN:
                    rc = mi.rcMonitor;
                    break;
                case CER_MAXIMIZED:
					{
						rc = mi.rcWork;
						RECT rcFrame = CalcMargins(CEM_FRAME);
						// Скорректируем размер окна до видимого на мониторе (рамка при максимизации уезжает за пределы экрана)
						rc.left -= rcFrame.left;
						rc.right += rcFrame.right;
						if (gSet.isHideCaption || gSet.isHideCaptionAlways())
							rc.top -= rcFrame.top;
						else
							rc.top -= rcFrame.bottom; // top включает и заголовок, а это нам не нужно
						rc.bottom += rcFrame.bottom;

						//if (gSet.isHideCaption && gConEmu.mb_MaximizedHideCaption && !gSet.isHideCaptionAlways)
						//	rc.top -= GetSystemMetrics(SM_CYCAPTION);
						//// Скорректируем размер окна до видимого на мониторе (рамка при максимизации уезжает за пределы экрана)
						//rc.left -= GetSystemMetrics(SM_CXSIZEFRAME);
						//rc.right += GetSystemMetrics(SM_CXSIZEFRAME);
						//rc.top -= GetSystemMetrics(SM_CYSIZEFRAME);
						//rc.bottom += GetSystemMetrics(SM_CYSIZEFRAME);
					}
                    break;
                default:
                    _ASSERTE(tFrom==CER_FULLSCREEN || tFrom==CER_MAXIMIZED);
                }
            }
            else
            {
                switch (tFrom)
                {
                case CER_FULLSCREEN:
                    rc = MakeRect(GetSystemMetrics(SM_CXFULLSCREEN),GetSystemMetrics(SM_CYFULLSCREEN));
                    break;
                case CER_MAXIMIZED:
                    rc = MakeRect(GetSystemMetrics(SM_CXMAXIMIZED),GetSystemMetrics(SM_CYMAXIMIZED));
                    if (gSet.isHideCaption && gConEmu.mb_MaximizedHideCaption && !gSet.isHideCaptionAlways())
                    	rc.top -= GetSystemMetrics(SM_CYCAPTION);
                    break;
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
			//        if (gSet.wndCascade && !gSet.isDesktopMode) {
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

/*!!!static!!*/
// Получить размер (правый нижний угол) окна по его клиентской области и наоборот
RECT CConEmuMain::MapRect(RECT rFrom, BOOL bFrame2Client)
{
    RECT rcShift = CalcMargins(CEM_FRAME);
    if (bFrame2Client) {
        //rFrom.left -= rcShift.left;
        //rFrom.top -= rcShift.top;
        rFrom.right -= (rcShift.right+rcShift.left);
        rFrom.bottom -= (rcShift.bottom+rcShift.top);
    } else {
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
//    GetClientRect(inWnd, &cRect); // The left and top members are zero. The right and bottom members contain the width and height of the window.
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
//  if (gConEmu.mp_TabBar->IsActive())
//      rect = gConEmu.mp_TabBar->GetMargins();
//
//  /*rect.top = gConEmu.mp_TabBar->IsActive()?gConEmu.mp_TabBar->Height():0;
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
//      GetClientRect(ghWnd, &rect);
//  if (gConEmu.mp_TabBar->IsActive()) {
//      RECT mr = gConEmu.mp_TabBar->GetMargins();
//      //rect.top += gConEmu.mp_TabBar->Height();
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
//  if (!gSet.Log Font.lfWidth || !gSet.Log Font.lfHeight) {
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
//        GetClientRect(ghWnd, &rect);
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
//      / gSet.LogFont.lfWidth;
//    size.Y = (rect.bottom - rect.top - (frameIncluded ? cwShift.y : 0) - consoleRect.top - consoleRect.bottom)
//      / gSet.LogFont.lfHeight;
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
//    rect.bottom = consoleSize.Y * gSet.LogFont.lfHeight + (rectInWindow ? cwShift.y : 0) + offsetRect.top + offsetRect.bottom;
//    rect.right = consoleSize.X * gSet.LogFont.lfWidth + (rectInWindow ? cwShift.x : 0) + offsetRect.left + offsetRect.right;
//    #ifdef MSGLOGGER
//        char szDbg[100]; wsprintfA(szDbg, "   WindowSizeFromConsole={%i,%i}\n", rect.right,rect.bottom);
//        DEBUGLOGFILE(szDbg);
//    #endif
//    return rect;
//}

// size in columns and lines
void CConEmuMain::SetConsoleWindowSize(const COORD& size, bool updateInfo)
{
    // Это не совсем корректно... ntvdm.exe не выгружается после выхода из 16бит приложения
    if (isNtvdm()) {
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

    if (isPictureView()) {
        isPiewUpdate = true;
        return;
    }

    // update size info
    // !!! Это вроде делает консоль
    WARNING("updateInfo убить");
    /*if (updateInfo && !gSet.isFullScreen && !isZoomed() && !isIconic())
    {
        gSet.UpdateSize(size.X, size.Y);
    }*/

    RECT rcCon = MakeRect(size.X,size.Y);
    if (mp_VActive) {
        if (!mp_VActive->RCon()->SetConsoleSize(size.X,size.Y))
            rcCon = MakeRect(mp_VActive->TextWidth,mp_VActive->TextHeight);
    }
    RECT rcWnd = CalcRect(CER_MAIN, rcCon, CER_CONSOLE);

    RECT wndR; GetWindowRect(ghWnd, &wndR); // текущий XY

    MOVEWINDOW ( ghWnd, wndR.left, wndR.top, rcWnd.right, rcWnd.bottom, 1);
}

// Изменить размер консоли по размеру окна (главного)
void CConEmuMain::SyncConsoleToWindow()
{
    if (mb_SkipSyncSize || isNtvdm() || !mp_VActive)
        return;

	_ASSERTE(mn_InResize <= 1);

    #ifdef _DEBUG
    if (change2WindowMode!=(DWORD)-1) {
        _ASSERTE(change2WindowMode==(DWORD)-1);
    }
    #endif

    mp_VActive->RCon()->SyncConsole2Window();

    /*
    DEBUGLOGFILE("SyncConsoleToWindow\n");

    RECT rcClient; GetClientRect(ghWnd, &rcClient);

    // Посчитать нужный размер консоли
    RECT newCon = CalcRect(CER_CONSOLE, rcClient, CER_MAINCLIENT);

    mp_VActive->SetConsoleSize(MakeCoord(newCon.right, newCon.bottom));
    */

    /*if (!isZoomed() && !isIconic() && !gSet.isFullScreen)
        gSet.UpdateSize(mp_VActive->TextWidth, mp_VActive->TextHeight);*/

    /*
    // Посчитать нужный размер консоли
    //COORD newConSize = ConsoleSizeFromWindow();
    // Получить текущий размер консольного окна
    CONSOLE_SCREEN_BUFFER_INFO inf; memset(&inf, 0, sizeof(inf));
    //GetConsoleScreenBufferInfo(mp_VActive->hConOut(), &inf);

    // Если нужно менять - ...
    if (newCon.right != (inf.srWindow.Right-inf.srWindow.Left+1) ||
        newCon.bottom != (inf.srWindow.Bottom-inf.srWindow.Top+1))
    {
        SetConsoleWindowSize(MakeCoord(newCon.right,newCon.bottom), true);
        if (mp_VActive)
            mp_VActive->InitDC(false);
    }
    */
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

	if (mp_VActive->TextWidth == 80) {
		int nDbg = mp_VActive->TextWidth;
	}

    #endif

	CRealConsole* pRCon = mp_VActive->RCon();
	if (pRCon && (mp_VActive->TextWidth != pRCon->TextWidth() || mp_VActive->TextHeight != pRCon->TextHeight())) {
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

	if (gSet.isAdvLogging) {
		char szInfo[128]; wsprintfA(szInfo, "SyncWindowToConsole(Cols=%i, Rows=%i)", mp_VActive->TextWidth, mp_VActive->TextHeight);
		mp_VActive->RCon()->LogString(szInfo, TRUE);
	}

    gSet.UpdateSize(mp_VActive->TextWidth, mp_VActive->TextHeight);

    MOVEWINDOW ( ghWnd, wndR.left, wndR.top, rcWnd.right, rcWnd.bottom, 1);
    
    //RECT consoleRect = ConsoleOffsetRect();

    //#ifdef MSGLOGGER
    //    char szDbg[100]; wsprintfA(szDbg, "   mp_VActive:Size={%i,%i}\n", mp_VActive->Width,mp_VActive->Height);
    //    DEBUGLOGFILE(szDbg);
    //#endif
    //
    //MOVEWINDOW ( ghWnd, wndR.left, wndR.top, mp_VActive->Width + cwShift.x + consoleRect.left + consoleRect.right, mp_VActive->Height + cwShift.y + consoleRect.top + consoleRect.bottom, 1);
}

void CConEmuMain::AutoSizeFont(const RECT &rFrom, enum ConEmuRect tFrom)
{
	if (gSet.isFontAutoSize) {
		// В 16бит режиме - не заморачиваться пока
		if (!gConEmu.isNtvdm()) {
			if (!gSet.wndWidth || !gSet.wndHeight) {
				MBoxAssert(gSet.wndWidth!=0 && gSet.wndHeight!=0);

			} else {
				RECT rc = rFrom;

				if (tFrom == CER_MAIN) {
					rc = CalcRect(CER_DC, rFrom, CER_MAIN);

				} else if (tFrom == CER_MAINCLIENT) {
					rc = CalcRect(CER_DC, rFrom, CER_MAINCLIENT);

				} else {
					MBoxAssert(tFrom==CER_MAINCLIENT || tFrom==CER_MAIN);
					return;
				}

				// !!! Для CER_DC размер в rc.right
				int nFontW = (rc.right - rc.left) / gSet.wndWidth;
					if (nFontW < 5) nFontW = 5;
				int nFontH = (rc.bottom - rc.top) / gSet.wndHeight;
					if (nFontH < 8) nFontH = 8;
				gSet.AutoRecreateFont(nFontW, nFontH);
			}
		}
	}
}

bool CConEmuMain::SetWindowMode(uint inMode, BOOL abForce)
{
	if (inMode != rNormal && inMode != rMaximized && inMode != rFullScreen)
		inMode = rNormal; // ошибка загрузки настроек?

    if (!isMainThread()) {
        PostMessage(ghWnd, mn_MsgSetWindowMode, inMode, 0);
        return false;
    }
    
    if (inMode == rFullScreen && gSet.isDesktopMode)
    	inMode = gSet.isFullScreen ? rNormal : rMaximized; // FullScreen на Desktop-е невозможен

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
    RECT consoleSize = MakeRect(gSet.wndWidth, gSet.wndHeight);
    bool canEditWindowSizes = false;
    bool lbRc = false;
	static bool bWasSetFullscreen = false;

    change2WindowMode = inMode;
	// Сброс флагов ресайза мышкой
	mouse.state &= ~(MOUSE_SIZING_BEGIN|MOUSE_SIZING_TODO);

	if (bWasSetFullscreen && inMode != rFullScreen) {
		if (mp_TaskBar2) {
			if (!gSet.isDesktopMode)
				mp_TaskBar2->MarkFullscreenWindow(ghWnd, FALSE);
			bWasSetFullscreen = false;
		}
	}

	CRealConsole* pRCon = (gSet.isAdvLogging!=0) ? ActiveCon()->RCon() : NULL;
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
			RECT rcCon = CalcRect(CER_CONSOLE, mrc_Ideal, CER_MAIN);
			if (!rcCon.right || !rcCon.bottom) { rcCon.right = gSet.wndWidth; rcCon.bottom = gSet.wndHeight; }
            if (mp_VActive && !mp_VActive->RCon()->SetConsoleSize(rcCon.right, rcCon.bottom)) {
				if (pRCon) pRCon->LogString("!!!SetConsoleSize FAILED!!!");
                mb_PassSysCommand = false;
                goto wrap;
            }

			//mb_InRestore = TRUE; 
			//HRGN hRgn = CreateWindowRgn();
			//SetWindowRgn(ghWnd, hRgn, TRUE);
			//mb_InRestore = FALSE;

            if (isIconic() || (isZoomed() && !mb_MaximizedHideCaption)) {
                //apiShowWindow(ghWnd, SW_SHOWNORMAL); // WM_SYSCOMMAND использовать не хочется...
                mb_IgnoreSizeChange = TRUE;
				if (IsWindowVisible(ghWnd)) {
					if (pRCon && gSet.isAdvLogging) pRCon->LogString("WM_SYSCOMMAND(SC_RESTORE)");
                    DefWindowProc(ghWnd, WM_SYSCOMMAND, SC_RESTORE, 0); //2009-04-22 Было SendMessage
				} else {
					if (pRCon && gSet.isAdvLogging) pRCon->LogString("ShowWindow(SW_SHOWNORMAL)");
                    apiShowWindow(ghWnd, SW_SHOWNORMAL);
				}
                //RePaint();
                mb_IgnoreSizeChange = FALSE;
				// Сбросить (заранее), вдруг оно isIconic?
				if (mb_MaximizedHideCaption)
					mb_MaximizedHideCaption = FALSE;
				if (pRCon && gSet.isAdvLogging) pRCon->LogString("OnSize(-1)");
                OnSize(-1); // подровнять ТОЛЬКО дочерние окошки
            }
			// Сбросить (однозначно)
			if (mb_MaximizedHideCaption)
				mb_MaximizedHideCaption = FALSE;

            RECT rcNew = CalcRect(CER_MAIN, consoleSize, CER_CONSOLE);
            //int nWidth = rcNew.right-rcNew.left;
            //int nHeight = rcNew.bottom-rcNew.top;
            rcNew.left+=gSet.wndX; rcNew.top+=gSet.wndY;
            rcNew.right+=gSet.wndX; rcNew.bottom+=gSet.wndY;

			// 2010-02-14 Проверку делаем ТОЛЬКО при загрузке настроек и включенном каскаде
            //// Параметры именно такие, результат - просто подгонка rcNew под рабочую область текущего монитора
            //rcNew = CalcRect(CER_CORRECTED, rcNew, CER_MAXIMIZED);

            #ifdef _DEBUG
            WINDOWPLACEMENT wpl; memset(&wpl,0,sizeof(wpl)); wpl.length = sizeof(wpl);
            GetWindowPlacement(ghWnd, &wpl);
            #endif
            
			if (pRCon && gSet.isAdvLogging) {
				char szInfo[128]; wsprintfA(szInfo, "SetWindowPos(X=%i, Y=%i, W=%i, H=%i)", rcNew.left, rcNew.top, rcNew.right-rcNew.left, rcNew.bottom-rcNew.top);
				pRCon->LogString(szInfo);
			}
            SetWindowPos(ghWnd, NULL, rcNew.left, rcNew.top, rcNew.right-rcNew.left, rcNew.bottom-rcNew.top, SWP_NOZORDER);
            
            #ifdef _DEBUG
            GetWindowPlacement(ghWnd, &wpl);
            #endif

            if (ghOpWnd)
                CheckRadioButton(gSet.hMain, rNormal, rFullScreen, rNormal);
            gSet.isFullScreen = false;

            if (!IsWindowVisible(ghWnd))
                apiShowWindow(ghWnd, SW_SHOWNORMAL);
            #ifdef _DEBUG
            GetWindowPlacement(ghWnd, &wpl);
            #endif
            
            // Если это во время загрузки - то до первого ShowWindow - isIconic возвращает FALSE
            if (isIconic() || isZoomed()) {
                apiShowWindow(ghWnd, SW_SHOWNORMAL); // WM_SYSCOMMAND использовать не хочется...
                // что-то после AltF9, AltF9 уголки остаются не срезанными...
                //hRgn = CreateWindowRgn();
				//SetWindowRgn(ghWnd, hRgn, TRUE);
            }

			UpdateWindowRgn();

            #ifdef _DEBUG
            GetWindowPlacement(ghWnd, &wpl);
            UpdateWindow(ghWnd);
            #endif
        } break;
    case rMaximized:
        {
            DEBUGLOGFILE("SetWindowMode(rMaximized)\n");

            // Обновить коордианты в gSet, если требуется
            if (!gSet.isFullScreen && !isZoomed() && !isIconic())
            {
                gSet.UpdatePos(rcWnd.left, rcWnd.top);
                if (mp_VActive)
                    gSet.UpdateSize(mp_VActive->TextWidth, mp_VActive->TextHeight);
            }

			if (!gSet.isHideCaption && !gSet.isHideCaptionAlways())
			{
				RECT rcMax = CalcRect(CER_MAXIMIZED, MakeRect(0,0), CER_MAXIMIZED);
				AutoSizeFont(rcMax, CER_MAIN);
				RECT rcCon = CalcRect(CER_CONSOLE, rcMax, CER_MAIN);
				if (mp_VActive && !mp_VActive->RCon()->SetConsoleSize(rcCon.right,rcCon.bottom))
				{
					if (pRCon) pRCon->LogString("!!!SetConsoleSize FAILED!!!");
					mb_PassSysCommand = false;
					goto wrap;
				}

				if (!isZoomed()) {
					mb_IgnoreSizeChange = TRUE;
					InvalidateAll();
					apiShowWindow(ghWnd, SW_SHOWMAXIMIZED);
					/*WINDOWPLACEMENT wpl = {sizeof(WINDOWPLACEMENT)};
					GetWindowPlacement(ghWnd, &wpl);
					wpl.flags = 0;
					wpl.showCmd = SW_SHOWMAXIMIZED;
					wpl.ptMaxPosition.x = rcMax.left;
					wpl.ptMaxPosition.y = rcMax.top;
					SetWindowPlacement(ghWnd, &wpl);*/
					mb_IgnoreSizeChange = FALSE;
					RePaint();
					if (pRCon && gSet.isAdvLogging) pRCon->LogString("OnSize(-1).2");
					OnSize(-1); // консоль уже изменила свой размер
				}

				if (ghOpWnd)
					CheckRadioButton(gSet.hMain, rNormal, rFullScreen, rMaximized);
				gSet.isFullScreen = false;

				if (!IsWindowVisible(ghWnd)) {
					mb_IgnoreSizeChange = TRUE;
					apiShowWindow(ghWnd, SW_SHOWMAXIMIZED);
					mb_IgnoreSizeChange = FALSE;
					if (pRCon && gSet.isAdvLogging) pRCon->LogString("OnSize(-1).3");
					OnSize(-1); // консоль уже изменила свой размер
				}

				UpdateWindowRgn();

			} // if (!gSet.isHideCaption)
			else
			{ // (gSet.isHideCaption)
				if (!isZoomed() || (gSet.isFullScreen || isIconic()) || abForce)
				{
					// Обновить коордианты в gSet, если требуется
					if (!gSet.isFullScreen && !isZoomed() && !isIconic())
					{
						gSet.UpdatePos(rcWnd.left, rcWnd.top);
						if (mp_VActive)
							gSet.UpdateSize(mp_VActive->TextWidth, mp_VActive->TextHeight);
					}

					mb_MaximizedHideCaption = TRUE;

					RECT rcMax = CalcRect(CER_MAXIMIZED, MakeRect(0,0), CER_MAXIMIZED);
					AutoSizeFont(rcMax, CER_MAIN);
					RECT rcCon = CalcRect(CER_CONSOLE, rcMax, CER_MAIN);
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
					MONITORINFO mi; memset(&mi, 0, sizeof(mi)); mi.cbSize = sizeof(mi);
					HMONITOR hMon = MonitorFromWindow(ghWnd, MONITOR_DEFAULTTONEAREST);
					if (hMon) {
						if (GetMonitorInfo(hMon, &mi)) {
							ptFullScreenSize.x = (mi.rcWork.right-mi.rcWork.left)+rcShift.left+rcShift.right;
							ptFullScreenSize.y = (mi.rcWork.bottom-mi.rcWork.top)+rcShift.top+rcShift.bottom;
						}
					}

					// Тут нужен "чистый" ::IsZoomed
					if (isIconic() || ::IsZoomed(ghWnd)) {
						// Если окно свернуто или "реально" максимизировано - показать нормальным
						mb_IgnoreSizeChange = TRUE;
						DWORD dwStyle = GetWindowLong(ghWnd, GWL_STYLE);
						if ((dwStyle & WS_MINIMIZE)) {
							apiShowWindow(ghWnd, SW_SHOWNORMAL);
						}
						if ((dwStyle & (WS_MINIMIZE|WS_MAXIMIZE)) != 0) {
							dwStyle &= ~(WS_MINIMIZE|WS_MAXIMIZE);
							SetWindowLong(ghWnd, GWL_STYLE, dwStyle);
						}
						//apiShowWindow(ghWnd, SW_SHOWNORMAL);
						// Сбросить
						_ASSERTE(mb_MaximizedHideCaption);
						mb_IgnoreSizeChange = FALSE;
						RePaint();
					}

					if (mp_TaskBar2) {
						if (!gSet.isDesktopMode)
							mp_TaskBar2->MarkFullscreenWindow(ghWnd, FALSE);
						bWasSetFullscreen = true;
					}

					// for virtual screens mi.rcWork. may contains negative values...

					if (pRCon && gSet.isAdvLogging) {
						char szInfo[128]; wsprintfA(szInfo, "SetWindowPos(X=%i, Y=%i, W=%i, H=%i)", -rcShift.left+mi.rcWork.left,-rcShift.top+mi.rcWork.top, ptFullScreenSize.x,ptFullScreenSize.y);
						pRCon->LogString(szInfo);
					}

					/* */ SetWindowPos(ghWnd, NULL,
						-rcShift.left+mi.rcWork.left,-rcShift.top+mi.rcWork.top,
						ptFullScreenSize.x,ptFullScreenSize.y,
						SWP_NOZORDER);

					if (ghOpWnd)
						CheckRadioButton(gSet.hMain, rNormal, rMaximized, rMaximized);
				}
				gSet.isFullScreen = false;

				if (!IsWindowVisible(ghWnd)) {
					mb_IgnoreSizeChange = TRUE;
					apiShowWindow(ghWnd, SW_SHOWNORMAL);
					mb_IgnoreSizeChange = FALSE;
					if (pRCon && gSet.isAdvLogging) pRCon->LogString("OnSize(-1).3");
					OnSize(-1);  // консоль уже изменила свой размер
				}

				UpdateWindowRgn();

			} // (gSet.isHideCaption)
        } break;

    case rFullScreen:
        DEBUGLOGFILE("SetWindowMode(rFullScreen)\n");
        if (!gSet.isFullScreen || (isZoomed() || isIconic()))
        {
            // Обновить коордианты в gSet, если требуется
            if (!gSet.isFullScreen && !isZoomed() && !isIconic())
            {
                gSet.UpdatePos(rcWnd.left, rcWnd.top);
                if (mp_VActive)
                    gSet.UpdateSize(mp_VActive->TextWidth, mp_VActive->TextHeight);
            }

            RECT rcMax = CalcRect(CER_FULLSCREEN, MakeRect(0,0), CER_FULLSCREEN);
			AutoSizeFont(rcMax, CER_MAINCLIENT);
            RECT rcCon = CalcRect(CER_CONSOLE, rcMax, CER_MAINCLIENT);
            if (mp_VActive && !mp_VActive->RCon()->SetConsoleSize(rcCon.right,rcCon.bottom))
            {
				if (pRCon) pRCon->LogString("!!!SetConsoleSize FAILED!!!");
                mb_PassSysCommand = false;
                goto wrap;
            }

            gSet.isFullScreen = true;
            isWndNotFSMaximized = isZoomed();
            
            RECT rcShift = CalcMargins(CEM_FRAME);
            //GetCWShift(ghWnd, &rcShift); // Обновить, на всякий случай

            // Умолчания
            ptFullScreenSize.x = GetSystemMetrics(SM_CXSCREEN)+rcShift.left+rcShift.right;
            ptFullScreenSize.y = GetSystemMetrics(SM_CYSCREEN)+rcShift.top+rcShift.bottom;
            // которые нужно уточнить для текущего монитора!
            MONITORINFO mi; memset(&mi, 0, sizeof(mi)); mi.cbSize = sizeof(mi);
            HMONITOR hMon = MonitorFromWindow(ghWnd, MONITOR_DEFAULTTONEAREST);
            if (hMon) {
                if (GetMonitorInfo(hMon, &mi)) {
                    ptFullScreenSize.x = (mi.rcMonitor.right-mi.rcMonitor.left)+rcShift.left+rcShift.right;
                    ptFullScreenSize.y = (mi.rcMonitor.bottom-mi.rcMonitor.top)+rcShift.top+rcShift.bottom;
                }
            }

            if (isIconic() || isZoomed()) {
                mb_IgnoreSizeChange = TRUE;
                apiShowWindow(ghWnd, SW_SHOWNORMAL);
				// Сбросить
				if (mb_MaximizedHideCaption)
					mb_MaximizedHideCaption = FALSE;
                mb_IgnoreSizeChange = FALSE;
                RePaint();
            }

			if (mp_TaskBar2) {
				if (!gSet.isDesktopMode)
					mp_TaskBar2->MarkFullscreenWindow(ghWnd, TRUE);
				bWasSetFullscreen = true;
			}

            // for virtual screens mi.rcMonitor. may contains negative values...

			if (pRCon && gSet.isAdvLogging) {
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
                CheckRadioButton(gSet.hMain, rNormal, rFullScreen, rFullScreen);
        }
        if (!IsWindowVisible(ghWnd)) {
            mb_IgnoreSizeChange = TRUE;
            apiShowWindow(ghWnd, SW_SHOWNORMAL);
            mb_IgnoreSizeChange = FALSE;
			if (pRCon && gSet.isAdvLogging) pRCon->LogString("OnSize(-1).3");
            OnSize(-1);  // консоль уже изменила свой размер
        }
        break;
    }


	if (pRCon && gSet.isAdvLogging) pRCon->LogString("SetWindowMode done");
    
    WindowMode = inMode; // Запомним!

    canEditWindowSizes = inMode == rNormal;
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
		if (bWasSetFullscreen != gSet.isFullScreen)
		{
			if (!gSet.isDesktopMode)
				mp_TaskBar2->MarkFullscreenWindow(ghWnd, gSet.isFullScreen);
			bWasSetFullscreen = gSet.isFullScreen;
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
    BOOL lbTabsAllowed = abShow /*&& gConEmu.mp_TabBar->IsAllowed()*/;

    if (abShow && !gConEmu.mp_TabBar->IsShown() && gSet.isTabs && lbTabsAllowed)
    {
        gConEmu.mp_TabBar->Activate();
        //ConEmuTab tab; memset(&tab, 0, sizeof(tab));
        //tab.Pos=0;
        //tab.Current=1;
        //tab.Type = 1;
        //gConEmu.mp_TabBar->Update(&tab, 1);
        //mp_VActive->RCon()->SetTabs(&tab, 1);
        gConEmu.mp_TabBar->Update();
        //gbPostUpdateWindowSize = true; // 2009-07-04 Resize выполняет сам TabBar
    }
    else if (!abShow)
    {
        gConEmu.mp_TabBar->Deactivate();
        //gbPostUpdateWindowSize = true; // 2009-07-04 Resize выполняет сам TabBar
    }


	// Иначе Gaps не отрисуются
	gConEmu.InvalidateAll();

	UpdateWindowRgn();

    // При отключенных табах нужно показать "[n/n] " а при выключенных - спрятать
    UpdateTitle(); // сам перечитает

	// 2009-07-04 Resize выполняет сам TabBar
    //if (gbPostUpdateWindowSize) { // значит мы что-то поменяли
    //    ReSize();
    //    /*RECT rcNewCon; GetClientRect(ghWnd, &rcNewCon);
    //    DCClientRect(&rcNewCon);
    //    MoveWindow(ghWndDC, rcNewCon.left, rcNewCon.top, rcNewCon.right - rcNewCon.left, rcNewCon.bottom - rcNewCon.top, 0);
    //    dcWindowLast = rcNewCon;
    //    
    //    if (gSet.LogFont.lfWidth)
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

void CConEmuMain::ReSize(BOOL abCorrect2Ideal /*= FALSE*/)
{
    if (isIconic())
        return;

	RECT client; GetClientRect(ghWnd, &client);

	#ifdef _DEBUG
	DWORD_PTR dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE);
	#endif

	if (abCorrect2Ideal)
	{

		if (!isZoomed() && !gSet.isFullScreen)
		{
			// Выполняем всегда, даже если размер уже соответсвует...
			RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);
			AutoSizeFont(mrc_Ideal, CER_MAIN);
			RECT rcConsole = CalcRect(CER_CONSOLE, mrc_Ideal, CER_MAIN);
			RECT rcCompWnd = CalcRect(CER_MAIN, rcConsole, CER_CONSOLE);
			// При показе/скрытии табов высота консоли может "прыгать"
			// Ее нужно скорректировать. Поскольку идет реальный ресайз
			// главного окна - OnSize вызовается автоматически
			_ASSERTE(isMainThread());

			m_Child->SetRedraw(FALSE);
			if (mp_VActive && mp_VActive->RCon())
				mp_VActive->RCon()->SetConsoleSize(rcConsole.right, rcConsole.bottom, 0, CECMD_SETSIZESYNC);
			m_Child->SetRedraw(TRUE);
			m_Child->Redraw();

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

			m_Child->SetRedraw(FALSE);
			mp_VActive->RCon()->SetConsoleSize(rcConsole.right, rcConsole.bottom, 0, CECMD_SETSIZESYNC);
			m_Child->SetRedraw(TRUE);
			m_Child->Redraw();
		}
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

void CConEmuMain::OnConsoleResize(BOOL abPosted/*=FALSE*/)
{
	//MSetter lInConsoleResize(&mb_InConsoleResize);

	// Выполняться должно в нити окна, иначе можем повиснуть
	static bool lbPosted = false;
	abPosted = (mn_MainThreadId == GetCurrentThreadId());
	if (!abPosted)
	{
		if (gSet.isAdvLogging)
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
		if (gSet.isAdvLogging)
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
    
	if (gSet.isAdvLogging)
	{
		char szInfo[160]; wsprintfA(szInfo, "OnConsoleResize: mouse.state=0x%08X, SizingToDo=%i, IsSizing=%i, LBtnPressed=%i, gbPostUpdateWindowSize=%i", 
			mouse.state, (int)lbSizingToDo, (int)lbIsSizing, (int)lbLBtnPressed, (int)gbPostUpdateWindowSize);
		mp_VActive->RCon()->LogString(szInfo, TRUE);
	}
    

    //COORD c = ConsoleSizeFromWindow();
    RECT client; GetClientRect(ghWnd, &client);
    // Проверим, вдруг не отработал isIconic
    if (client.bottom > 10 )
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
        
		if (gSet.isAdvLogging)
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
                if (!gSet.isFullScreen && !isZoomed() && !lbSizingToDo)
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
			//if (lbSizingToDo && !gSet.isFullScreen && !isZoomed() && !isIconic()) {
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

bool CConEmuMain::CorrectWindowPos(WINDOWPOS *wp)
{
	return false;
}

LRESULT CConEmuMain::OnSize(WPARAM wParam, WORD newClientWidth, WORD newClientHeight)
{
    LRESULT result = 0;
    
	#ifdef _DEBUG
	RECT rcDbgSize; GetWindowRect(ghWnd, &rcDbgSize);
	wchar_t szSize[255]; swprintf_s(szSize, L"OnSize(%i, %ix%i) Current window size (X=%i, Y=%i, W=%i, H=%i)\n",
		wParam, (int)(short)newClientWidth, (int)(short)newClientHeight,
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

    if (newClientWidth==(WORD)-1 || newClientHeight==(WORD)-1)
    {
        RECT rcClient; GetClientRect(ghWnd, &rcClient);
        newClientWidth = rcClient.right;
        newClientHeight = rcClient.bottom;
    }

	// Запомнить "идеальный" размер окна, выбранный пользователем
	if (isSizing() && !gSet.isFullScreen && !isZoomed() && !isIconic())
	{
		GetWindowRect(ghWnd, &mrc_Ideal);
	}

    if (gConEmu.mp_TabBar->IsActive())
        gConEmu.mp_TabBar->UpdateWidth();

    // Background - должен занять все клиентское место под тулбаром
    // Там же ресайзится ScrollBar
    m_Back->Resize();
    
    #ifdef _DEBUG
    BOOL lbIsPicView = 
    #endif
    isPictureView();

    if (wParam != (DWORD)-1 && change2WindowMode == (DWORD)-1 && mn_InResize <= 1)
    {
        SyncConsoleToWindow();
    }
    
    RECT mainClient = MakeRect(newClientWidth,newClientHeight);

	RECT dcSize = CalcRect(CER_DC, mainClient, CER_MAINCLIENT);

	RECT client = CalcRect(CER_DC, mainClient, CER_MAINCLIENT, NULL, &dcSize);

	WARNING("Вынести в CalcRect");
    RECT rcNewCon; memset(&rcNewCon,0,sizeof(rcNewCon));
    if (mp_VActive && mp_VActive->Width && mp_VActive->Height)
    {
        if ((gSet.isTryToCenter && (isZoomed() || gSet.isFullScreen))
			|| isNtvdm())
        {
            rcNewCon.left = (client.right+client.left-(int)mp_VActive->Width)/2;
            rcNewCon.top = (client.bottom+client.top-(int)mp_VActive->Height)/2;
        }

        if (rcNewCon.left<client.left) rcNewCon.left=client.left;
        if (rcNewCon.top<client.top) rcNewCon.top=client.top;

        rcNewCon.right = rcNewCon.left + mp_VActive->Width;
        rcNewCon.bottom = rcNewCon.top + mp_VActive->Height;
        
        if (rcNewCon.right>client.right) rcNewCon.right=client.right;
        if (rcNewCon.bottom>client.bottom) rcNewCon.bottom=client.bottom;
        
    }
    else
    {
        rcNewCon = client;
    }

	bool lbPosChanged = false;
	RECT rcCurCon; GetClientRect(ghWndDC, &rcCurCon);
	MapWindowPoints(ghWndDC, ghWnd, (LPPOINT)&rcCurCon, 2);
	lbPosChanged = memcmp(&rcCurCon, &rcNewCon, sizeof(RECT))!=0;

	if (lbPosChanged)
	{
	    // Двигаем/ресайзим окошко DC
		MoveWindow(ghWndDC, rcNewCon.left, rcNewCon.top, rcNewCon.right - rcNewCon.left, rcNewCon.bottom - rcNewCon.top, 1);
		m_Child->Invalidate();
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
        wParam, LOWORD(lParam), HIWORD(lParam));
    if (gSet.isAdvLogging) 
    	mp_VActive->RCon()->LogString(szDbg);
    #endif


    #ifndef _DEBUG
    if (isPictureView())
    {
        RECT *pRect = (RECT*)lParam; // с рамкой
        *pRect = mrc_WndPosOnPicView;
        //pRect->right = pRect->left + (mrc_WndPosOnPicView.right-mrc_WndPosOnPicView.left);
        //pRect->bottom = pRect->top + (mrc_WndPosOnPicView.bottom-mrc_WndPosOnPicView.top);
    } else
    #endif
    if (mb_IgnoreSizeChange)
    {
        // на время обработки WM_SYSCOMMAND
    }
    else if (isNtvdm())
    {
        // не менять для 16бит приложений
    }
    else if (!gSet.isFullScreen && !isZoomed())
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
        LONG nWidth = gSet.FontWidth(), nHeight = gSet.FontHeight();
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
			wchar_t szSize[255]; swprintf_s(szSize, L"New window size (X=%i, Y=%i, W=%i, H=%i); Current size (X=%i, Y=%i, W=%i, H=%i)\n",
				pRect->left, pRect->top, (pRect->right-pRect->left), (pRect->bottom-pRect->top),
				rcCurrent.left, rcCurrent.top, (rcCurrent.right-rcCurrent.left), (rcCurrent.bottom-rcCurrent.top));
			DEBUGSTRSIZE(szSize);
			#endif
		}
    }

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
		if (gSet.isDragPanel == 2)
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
			wchar_t szDbg[128]; wsprintf(szDbg, L"PanelDrag: Sending '%s'\n", szKey);
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
    for (int i=0; mp_VActive && i<MAX_CONSOLE_COUNT; i++)
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
    /*if (mn_ActiveCon >= MAX_CONSOLE_COUNT)
        mn_ActiveCon = -1;
    if (mn_ActiveCon < 0)
        return NULL;
    return mp_VCon[mn_ActiveCon];*/
}

// 0 - based
int CConEmuMain::ActiveConNum()
{
    int nActive = -1;
    for (int i=0; mp_VActive && i<MAX_CONSOLE_COUNT; i++)
    {
        if (mp_VCon[i] == mp_VActive)
        {
            nActive = i; break;
        }
    }
    return nActive;
}

BOOL CConEmuMain::AttachRequested(HWND ahConWnd, CESERVER_REQ_STARTSTOP pStartStop, CESERVER_REQ_STARTSTOPRET* pRet)
{
    int i;
    CVirtualConsole* pCon = NULL;

	_ASSERTE(pStartStop.dwPID!=0);

    // Может быть какой-то VCon ждет аттача?
    for (i = 0; !pCon && i<MAX_CONSOLE_COUNT; i++)
    {
		if (mp_VCon[i])
		{
			CRealConsole* pRCon = mp_VCon[i]->RCon();
			if (pRCon)
			{
				if (pRCon->isDetached())
					pCon = mp_VCon[i];
				if (pRCon->GetServerPID() == pStartStop.dwPID)
				{
					//_ASSERTE(pRCon->GetServerPID() != pStartStop.dwPID);
					pCon = mp_VCon[i];
					break;
				}
			}
		}
    }
    // Если не нашли - определим, можно ли добавить новую консоль?
    if (!pCon)
    {
		RConStartArgs args; args.bDetached = TRUE;
        if ((pCon = CreateCon(&args)) == NULL)
            return FALSE;
    }

    // Пытаемся подцепить консоль
    if (!pCon->RCon()->AttachConemuC(ahConWnd, pStartStop.dwPID, pStartStop, pRet))
        return FALSE;

    // OK
    return TRUE;
}

// Вернуть общее количество процессов по всем консолям
DWORD CConEmuMain::CheckProcesses()
{
    DWORD dwAllCount = 0;
    //mn_ActiveStatus &= ~CES_PROGRAMS;
    for (int j=0; j<MAX_CONSOLE_COUNT; j++)
    {
        if (mp_VCon[j] == NULL) continue;

        int nCount = mp_VCon[j]->RCon()->GetProcesses(NULL);
        if (nCount)
            dwAllCount += nCount;
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
    for (j=0; j<=1; j++)
    {
        if (abNext)
        {
            if (j == 0)
            {
                n1 = nActive+1; n2 = MAX_CONSOLE_COUNT; n3 = 1;
            }
            else
            {
                n1 = 0; n2 = nActive; n3 = 1;
            }
            if (n1>=n2) continue;
        }
        else
        {
            if (j == 0) {
                n1 = nActive-1; n2 = -1; n3 = -1;
            }
            else
            {
                n1 = MAX_CONSOLE_COUNT-1; n2 = nActive; n3 = -1;
            }
            if (n1<=n2) continue;
        }

        for (i=n1; i!=n2 && i>=0 && i<MAX_CONSOLE_COUNT; i+=n3)
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

    
    if (nCon>=0 && nCon<MAX_CONSOLE_COUNT)
    {
        CVirtualConsole* pCon = mp_VCon[nCon];
        if (pCon == NULL)
            return false; // консоль с этим номером не была создана!
        if (pCon == mp_VActive)
            return true; // уже
        bool lbSizeOK = true;
        int nOldConNum = ActiveConNum();

        // Спрятать PictureView, или еще чего...
        if (mp_VActive) mp_VActive->RCon()->OnDeactivate(nCon);
        
        // ПЕРЕД переключением на новую консоль - обновить ее размеры
        if (mp_VActive)
        {
            int nOldConWidth = mp_VActive->RCon()->TextWidth();
            int nOldConHeight = mp_VActive->RCon()->TextHeight();
            int nNewConWidth = pCon->RCon()->TextWidth();
            int nNewConHeight = pCon->RCon()->TextHeight();
            if (nOldConWidth != nNewConWidth || nOldConHeight != nNewConHeight)
            {
                lbSizeOK = pCon->RCon()->SetConsoleSize(nOldConWidth,nOldConHeight);
            }
        }

        mp_VActive = pCon;

        pCon->RCon()->OnActivate(nCon, nOldConNum);
        
        if (!lbSizeOK)
            SyncWindowToConsole();
        m_Child->Invalidate();
    }
    return false;
}

CVirtualConsole* CConEmuMain::CreateCon(RConStartArgs *args)
{
    CVirtualConsole* pCon = NULL;
    for (int i=0; i<MAX_CONSOLE_COUNT; i++)
    {
        if (!mp_VCon[i])
        {
			CVirtualConsole* pOldActive = mp_VActive;
			mb_CreatingActive = true;
            pCon = CVirtualConsole::CreateVCon(args);
			mb_CreatingActive = false;
            if (pCon)
            {
                if (pOldActive) pOldActive->RCon()->OnDeactivate(i);
                mp_VCon[i] = pCon;
                mp_VActive = pCon;
                pCon->RCon()->OnActivate(i, ActiveConNum());
                
                //mn_ActiveCon = i;

                //Update(true);
            }
            break;
        }
    }
    return pCon;
}

void CConEmuMain::UpdateFarSettings()
{
	for (int i = 0; i<MAX_CONSOLE_COUNT; i++)
	{
		if (mp_VCon[i] == NULL) continue;
		CRealConsole* pRCon = mp_VCon[i]->RCon();
		if (pRCon)
			pRCon->UpdateFarSettings();
		//DWORD dwFarPID = pRCon->GetFarPID();
		//if (!dwFarPID) continue;
		//pRCon->EnableComSpec(dwFarPID, gSet.AutoBufferHeight);
	}
}

void CConEmuMain::UpdateIdealRect(BOOL abAllowUseConSize/*=FALSE*/)
{
	// Запомнить "идеальный" размер окна, выбранный пользователем
	if (!gSet.isFullScreen && !isZoomed() && !isIconic())
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
			
			if (gSet.isDebugSteps || abErrorSeverity)
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

LPTSTR CConEmuMain::GetTitleStart()
{
    //mn_ActiveStatus &= ~CES_CONALTERNATIVE; // сброс флага альтернативной консоли
    return Title;
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
			int nBtn = MessageBox(ghWnd, L"Debugger active!\nShellExecuteEx(runas) my fails, when VC IDE\ncatches Microsoft C++ exceptions.\nContinue?", L"ConEmu", MB_ICONASTERISK|MB_YESNO|MB_DEFBUTTON2);
			gbDontEnable = b;
			if (nBtn != IDYES)
				return (FALSE);
		}*/
		lRc = ::ShellExecuteEx(lpShellExecute);
		
		if (abAllowAsync && lRc == 0)
		{
			mp_VActive->RCon()->CloseConsole();
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

    TCHAR szIconPath[MAX_PATH] = {0};
    lstrcpyW(szIconPath, ms_ConEmuExe);
        
    TCHAR *lpszExt = _tcsrchr(szIconPath, _T('.'));
    if (!lpszExt)
    {
        szIconPath[0] = 0;
    }
    else
    {
        _tcscpy(lpszExt, _T(".ico"));
        DWORD dwAttr = GetFileAttributes(szIconPath);
        if (dwAttr==(DWORD)-1 || (dwAttr & FILE_ATTRIBUTE_DIRECTORY))
            szIconPath[0]=0;
    }
    
    if (szIconPath[0])
    {
        hClassIcon = (HICON)LoadImage(0, szIconPath, IMAGE_ICON, 
            GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR|LR_LOADFROMFILE);
        hClassIconSm = (HICON)LoadImage(0, szIconPath, IMAGE_ICON, 
            GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR|LR_LOADFROMFILE);
    }
    if (!hClassIcon)
    {
        szIconPath[0]=0;
        
        hClassIcon = (HICON)LoadImage(GetModuleHandle(0), 
            MAKEINTRESOURCE(gSet.nIconID), IMAGE_ICON, 
            GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
            
        if (hClassIconSm) DestroyIcon(hClassIconSm);
        hClassIconSm = (HICON)LoadImage(GetModuleHandle(0), 
            MAKEINTRESOURCE(gSet.nIconID), IMAGE_ICON, 
            GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    }
}

bool CConEmuMain::LoadVersionInfo(wchar_t* pFullPath)
{
    LPBYTE pBuffer=NULL;
    wchar_t* pVersion=NULL;
    //wchar_t* pDesc=NULL;
    
    const wchar_t WSFI[] = L"StringFileInfo";

    DWORD size = GetFileVersionInfoSizeW(pFullPath, &size);
    if(!size) return false;
    MCHKHEAP
    pBuffer = new BYTE[size];
    MCHKHEAP
    GetFileVersionInfoW((wchar_t*)pFullPath, 0, size, pBuffer);

    //Find StringFileInfo
    DWORD ofs;
    for(ofs = 92; ofs < size; ofs += *(WORD*)(pBuffer+ofs) )
        if(!lstrcmpiW((wchar_t*)(pBuffer+ofs+6), WSFI))
            break;
    if(ofs >= size)
    {
        delete pBuffer;
        return false;
    }
    TCHAR *langcode;
    langcode = (TCHAR*)(pBuffer + ofs + 42);

    TCHAR blockname[48];
    unsigned dsize;

    wsprintf(blockname, _T("\\%s\\%s\\FileVersion"), WSFI, langcode);
    if(!VerQueryValue(pBuffer, blockname, (void**)&pVersion, &dsize))
    {
        pVersion = 0;
    }
    else
    {
       if (dsize>=31) pVersion[31]=0;
       wcscpy(szConEmuVersion, pVersion);
       pVersion = wcsrchr(szConEmuVersion, L',');
       if (pVersion && wcscmp(pVersion, L", 0")==0)
           *pVersion = 0;
    }
    
    delete[] pBuffer;
    
    return true;
}

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
    //    pipe.Execute(CMD_POSTMACRO, asMacro, (wcslen(asMacro)+1)*2);
    //    DebugStep(NULL);
    //}
}

bool CConEmuMain::PtDiffTest(POINT C, int aX, int aY, UINT D)
{
    //(((abs(C.x-LOWORD(lParam)))<D) && ((abs(C.y-HIWORD(lParam)))<D))
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
		if (!gSet.icMinimizeRestore)
			return; // не просили

		UINT nMOD = gSet.GetHostKeyMod();
		
		if (mn_MinRestoreRegistered
			&& (mn_MinRestore_VK != gSet.icMinimizeRestore || nMOD != mn_MinRestore_MOD))
		{
			UnregisterHotKey(ghWnd, mn_MinRestoreRegistered);
			mn_MinRestoreRegistered = 0;
		}
		
		if (!mn_MinRestoreRegistered)
		{
			if (RegisterHotKey(ghWnd, 0x1001, nMOD, (UINT)gSet.icMinimizeRestore))
			{
				mn_MinRestoreRegistered = 0x1001;
				mn_MinRestore_VK = gSet.icMinimizeRestore;
				mn_MinRestore_MOD = nMOD;
			}
			else if (isFirstInstance())
			{
				// -- При одновременном запуске двух копий - велики шансы, что они подерутся
				// -- наверное вообще не будем показывать ошибку
				// -- кроме того, isFirstInstance() не работает, если копия ConEmu.exe запущена под другим юзером
				#ifdef _DEBUG
				wchar_t szErr[128]; DWORD dwErr = GetLastError();
				wsprintfW(szErr, L"Can't register Minimize/Restore hotkey, ErrCode=0x%08X", dwErr);
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
	if (!mb_HotKeyRegistered)
	{
		if (RegisterHotKey(ghWnd, 0x201, MOD_CONTROL|MOD_WIN|MOD_ALT, VK_SPACE))
		{
			mb_HotKeyRegistered = TRUE;
		}
	}

	if (!mh_LLKeyHook)
	{
		RegisterHoooks();
	}
}

void CConEmuMain::RegisterHoooks()
{
//	#ifndef _DEBUG
	// Для WinXP это не было нужно
	if (gOSVer.dwMajorVersion < 6)
	{
		return;
	}
//	#endif

	// Если Host-клавиша НЕ Win, или юзер не хочет переключаться Win+Number - хук не нужен
	if (!gSet.isUseWinNumber || !gSet.IsHostkeySingle(VK_LWIN))
	{
		UnRegisterHoooks();
		return;
	}

	DWORD dwErr = 0;

	if (!mh_LLKeyHook)
	{
		// Проверяет, разрешил ли пользователь установку хуков.
		if (gSet.isKeyboardHooks())
		{
			if (!mh_LLKeyHookDll)
			{
				wchar_t szConEmuHkDll[MAX_PATH+32];
				lstrcpy(szConEmuHkDll, ms_ConEmuBaseDir);
				#ifdef WIN64
					lstrcat(szConEmuHkDll, L"\\ConEmuHk64.dll");
				#else
					lstrcat(szConEmuHkDll, L"\\ConEmuHk.dll");
				#endif
				mh_LLKeyHookDll = LoadLibrary(szConEmuHkDll);
			}
			if (!mh_LLKeyHook && mh_LLKeyHookDll)
			{
				HOOKPROC pfnLLHK = (HOOKPROC)GetProcAddress(mh_LLKeyHookDll, "LLKeybHook");
				HHOOK *pKeyHook = (HHOOK*)GetProcAddress(mh_LLKeyHookDll, "ghKeyHook");
				HWND *pConEmuRoot = (HWND*)GetProcAddress(mh_LLKeyHookDll, "ghConEmuRoot");
				HWND *pConEmuDc = (HWND*)GetProcAddress(mh_LLKeyHookDll, "ghConEmuWnd");

				if (pConEmuRoot)
					*pConEmuRoot = ghWnd;
				if (pConEmuDc)
					*pConEmuDc = ghWndDC;

				if (pfnLLHK)
				{
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
//    //if (nVkKeyCode == ' ' && gSet.isSendAltSpace == 2 && gSet.IsHostkeyPressed())
//	//{
//    //	ShowSysmenu();
//	//	return TRUE;
//	//}
//
//	if (!gSet.isUseWinNumber || !gSet.IsHostkeyPressed())
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
		UnregisterHotKey(ghWnd, 0x201);
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
    if (!mp_VActive)
    {
    	//MBox(L"CtrlWinAltSpace: mp_VActive==NULL");
    	return;
	}
    
    static DWORD dwLastSpaceTick = 0;
    if ((dwLastSpaceTick-GetTickCount())<1000)
    {
        //if (hWnd == ghWndDC) MBoxA(_T("Space bounce recieved from DC")) else
        //if (hWnd == ghWnd) MBoxA(_T("Space bounce recieved from MainWindow")) else
        //if (hWnd == gConEmu.m_Back->mh_WndBack) MBoxA(_T("Space bounce recieved from BackWindow")) else
        //if (hWnd == gConEmu.m_Back->mh_WndScroll) MBoxA(_T("Space bounce recieved from ScrollBar")) else
        MBoxA(_T("Space bounce recieved from unknown window"));
        return;
    }
    dwLastSpaceTick = GetTickCount();

    //MBox(L"CtrlWinAltSpace: Toggle");
    mp_VActive->RCon()->ShowConsole(-1); // Toggle visibility
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

	if (!abConfirm && isPressed(VK_SHIFT))
		abConfirm = TRUE;
    
    if (!abRecreate)
    {
        // Создать новую консоль
        BOOL lbSlotFound = FALSE;
        for (int i=0; i<MAX_CONSOLE_COUNT; i++)
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
        	int nRc = RecreateDlg((LPARAM)&args);
            //BOOL b = gbDontEnable;
            //gbDontEnable = TRUE;
            //int nRc = DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_RESTART), ghWnd, Recreate DlgProc, (LPARAM)&args);
            //gbDontEnable = b;
            if (nRc != IDC_START)
                return;
			m_Child->Redraw();
        }
        //Собственно, запуск
        CreateCon(&args);
        
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
				int nRc = RecreateDlg((LPARAM)&args);
	            //BOOL b = gbDontEnable;
	            //gbDontEnable = TRUE;
	            //int nRc = DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_RESTART), ghWnd, Recreate DlgProc, (LPARAM)&args);
	            //gbDontEnable = b;
	            if (nRc == IDC_TERMINATE)
	            {
	                mp_VActive->RCon()->CloseConsole();
	                return;
	            }
	            if (nRc != IDC_START)
	                return;
            }
			m_Child->Redraw();
            // Собственно, Recreate
            mp_VActive->RCon()->RecreateProcess(&args);
        }
    }
    
    SafeFree(args.pszSpecialCmd);
	SafeFree(args.pszStartupDir);
	SafeFree(args.pszUserName);
	//SafeFree(args.pszUserPassword);
}

int CConEmuMain::RecreateDlg(LPARAM lParam)
{
    BOOL b = gbDontEnable;
    gbDontEnable = TRUE;
    
 
	if (isPressed(VK_APPS))
	{
		// Игнорировать одно следующее VK_APPS
		mb_SkipAppsInRecreate = TRUE;
		mh_RecreateDlgKeyHook = SetWindowsHookEx(WH_GETMESSAGE, RecreateDlgKeyHook, NULL, GetCurrentThreadId() );
	}
    
    
    int nRc = DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_RESTART), ghWnd, RecreateDlgProc, lParam);
    
    if (mh_RecreateDlgKeyHook)
    {
    	UnhookWindowsHookEx(mh_RecreateDlgKeyHook);
    	mh_RecreateDlgKeyHook = NULL;
    }
    
    gbDontEnable = b;
    return nRc;
}



BOOL CConEmuMain::RunSingleInstance()
{
	BOOL lbAccepted = FALSE;
	LPCWSTR lpszCmd = gSet.GetCmd();
	if (lpszCmd && *lpszCmd)
	{
		HWND ConEmuHwnd = FindWindowExW(NULL, NULL, VirtualConsoleClassMain, NULL);
		if (ConEmuHwnd)
		{
			CESERVER_REQ *pIn = NULL, *pOut = NULL;
			int nCmdLen = lstrlenW(lpszCmd);
			int nSize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_NEWCMD);
			if (nCmdLen >= MAX_PATH)
			{
				nSize += (nCmdLen - MAX_PATH + 2) * 2;
			}
			pIn = (CESERVER_REQ*)calloc(nSize,1);
			if (pIn)
			{
				ExecutePrepareCmd(pIn, CECMD_NEWCMD, nSize);
				lstrcpyW(pIn->NewCmd.szCommand, lpszCmd);

				DWORD dwPID = 0;
				if (GetWindowThreadProcessId(ConEmuHwnd, &dwPID))
					AllowSetForegroundWindow(dwPID);

				pOut = ExecuteGuiCmd(ConEmuHwnd, pIn, NULL);
				if (pOut && pOut->Data[0])
					lbAccepted = TRUE;
			}
			if (pIn) {free(pIn); pIn = NULL;}
			if (pOut) ExecuteFreeResult(pOut);
		}
	}
	return lbAccepted;
}

int CConEmuMain::BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    if (uMsg==BFFM_INITIALIZED)
    {
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
    }
	return 0;
}

LRESULT CConEmuMain::RecreateDlgKeyHook(int code, WPARAM wParam, LPARAM lParam)
{
	if (code >= 0)
	{
		if (gConEmu.mb_SkipAppsInRecreate && lParam)
		{
			LPMSG pMsg = (LPMSG)lParam;
			if (pMsg->message == WM_CONTEXTMENU)
			{
				pMsg->message = WM_NULL;
				gConEmu.mb_SkipAppsInRecreate = FALSE;
				return FALSE; // Skip one Apps
			}
		}
	}

	return CallNextHookEx(gConEmu.mh_RecreateDlgKeyHook, code, wParam, lParam);
}

INT_PTR CConEmuMain::RecreateDlgProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	#define UM_USER_CONTROLS (WM_USER+121)
    switch (messg)
    {
    case WM_INITDIALOG:
        {
            BOOL lbRc = FALSE;
            //#ifdef _DEBUG
            //SetWindowPos(ghOpWnd, HWND_NOTOPMOST, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE);
            //#endif

			// Если на момент открытия диалога был нажат Apps - пропустить его "отжатие"
            
            LPCWSTR pszCmd = gConEmu.ActiveCon()->RCon()->GetCmd();
            int nId = SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_FINDSTRINGEXACT, -1, (LPARAM)pszCmd);
            if (nId < 0) SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_INSERTSTRING, 0, (LPARAM)pszCmd);
            LPCWSTR pszSystem = gSet.GetCmd();
            if (pszSystem != pszCmd)
            {
                int nId = SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_FINDSTRINGEXACT, -1, (LPARAM)pszSystem);
                if (nId < 0) SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_INSERTSTRING, 0, (LPARAM)pszSystem);
            }
			pszSystem = gSet.HistoryGet();
			if (pszSystem)
			{
				while (*pszSystem)
				{
					int nId = SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_FINDSTRINGEXACT, -1, (LPARAM)pszSystem);
					if (nId < 0) SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_INSERTSTRING, -1, (LPARAM)pszSystem);
					pszSystem += lstrlen(pszSystem)+1;
				}
			}
            SetDlgItemText(hDlg, IDC_RESTART_CMD, pszCmd);
            
            SetDlgItemText(hDlg, IDC_STARTUP_DIR, gConEmu.ActiveCon()->RCon()->GetDir());
            //EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_DIR), FALSE);
            //#ifndef _DEBUG
            //EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_DIR), FALSE);
            //#endif
            
            const wchar_t *pszUser/*, *pszPwd*/; BOOL bResticted;
            int nChecked = rbCurrentUser;
			wchar_t szCurUser[MAX_PATH]; DWORD nUserNameLen = countof(szCurUser);
			if (!GetUserName(szCurUser, &nUserNameLen)) szCurUser[0] = 0;
			wchar_t szRbCaption[MAX_PATH+32];
			lstrcpy(szRbCaption, L"Run as current &user: "); lstrcat(szRbCaption, szCurUser);
			SetDlgItemText(hDlg, rbCurrentUser, szRbCaption);
            if (gConEmu.ActiveCon()->RCon()->GetUserPwd(&pszUser, /*&pszPwd,*/ &bResticted))
            {
            	nChecked = rbAnotherUser;
            	if (bResticted)
            	{
	            	CheckDlgButton(hDlg, cbRunAsRestricted, BST_CHECKED);
            	}
            	else
            	{
					lstrcpyn(szCurUser, pszUser, MAX_PATH);
            		SetDlgItemText(hDlg, tRunAsPassword, L"");
            	}
            }
			SetDlgItemText(hDlg, tRunAsUser, szCurUser);
            CheckRadioButton(hDlg, rbCurrentUser, rbAnotherUser, nChecked);
            RecreateDlgProc(hDlg, UM_USER_CONTROLS, 0, 0);
            

			RConStartArgs* pArgs = (RConStartArgs*)lParam;
			_ASSERTE(pArgs);
			SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)lParam);

	
			if (gConEmu.m_osv.dwMajorVersion < 6)
			{
				// В XP и ниже это просто RunAs - с возможностью ввода имени пользователя и пароля
				//apiShowWindow(GetDlgItem(hDlg, cbRunAsAdmin), SW_HIDE);
				SetDlgItemTextA(hDlg, cbRunAsAdmin, "&Run as..."); //GCC hack. иначе не собирается
				// И уменьшить длину
                RECT rcBox; GetWindowRect(GetDlgItem(hDlg, cbRunAsAdmin), &rcBox);
                SetWindowPos(GetDlgItem(hDlg, cbRunAsAdmin), NULL, 0, 0, (rcBox.right-rcBox.left)/2, rcBox.bottom-rcBox.top,
                	SWP_NOMOVE|SWP_NOZORDER);
			}
			else if (gConEmu.mb_IsUacAdmin || (pArgs && pArgs->bRunAsAdministrator))
			{
				CheckDlgButton(hDlg, cbRunAsAdmin, BST_CHECKED);
				if (gConEmu.mb_IsUacAdmin) // Только в Vista+ если GUI уже запущен под админом
				{
					EnableWindow(GetDlgItem(hDlg, cbRunAsAdmin), FALSE);
				}
				else if (gConEmu.m_osv.dwMajorVersion < 6)
				{
					RecreateDlgProc(hDlg, WM_COMMAND, cbRunAsAdmin, 0);
				}
			}
			//}


            SetClassLongPtr(hDlg, GCLP_HICON, (LONG)hClassIcon);
            if (pArgs->bRecreate)
            {
                //GCC hack. иначе не собирается
                SetDlgItemTextA(hDlg, IDC_RESTART_MSG, "About to recreate console");
                SendDlgItemMessage(hDlg, IDC_RESTART_ICON, STM_SETICON, (WPARAM)LoadIcon(NULL,IDI_EXCLAMATION), 0);
                
                lbRc = TRUE;
            }
            else
            {
                //GCC hack. иначе не собирается
                SetDlgItemTextA(hDlg, IDC_RESTART_MSG, "Create new console");
                SendDlgItemMessage(hDlg, IDC_RESTART_ICON, STM_SETICON, (WPARAM)LoadIcon(NULL,IDI_QUESTION), 0);
                POINT pt = {0,0};
                MapWindowPoints(GetDlgItem(hDlg, IDC_TERMINATE), hDlg, &pt, 1);
                DestroyWindow(GetDlgItem(hDlg, IDC_TERMINATE));
                SetWindowPos(GetDlgItem(hDlg, IDC_START), NULL, pt.x, pt.y, 0,0, SWP_NOSIZE|SWP_NOZORDER);
                SetDlgItemText(hDlg, IDC_START, L"&Start");
                DestroyWindow(GetDlgItem(hDlg, IDC_WARNING));
                
                // Выровнять флажок по кнопке
				RECT rcBtn; GetWindowRect(GetDlgItem(hDlg, IDC_START), &rcBtn);
                RECT rcBox; GetWindowRect(GetDlgItem(hDlg, cbRunAsAdmin), &rcBox);
                pt.x = pt.x - (rcBox.right - rcBox.left) - 5;
                //MapWindowPoints(NULL, hDlg, (LPPOINT)&rcBox, 1);
                pt.y += ((rcBtn.bottom-rcBtn.top) - (rcBox.bottom-rcBox.top))/2;
                SetWindowPos(GetDlgItem(hDlg, cbRunAsAdmin), NULL, pt.x, pt.y, 0,0, SWP_NOSIZE|SWP_NOZORDER);
                
                SetFocus(GetDlgItem(hDlg, IDC_RESTART_CMD));
            }
            
            RECT rect;
            GetWindowRect(hDlg, &rect);
            RECT rcParent;
            GetWindowRect(ghWnd, &rcParent);
            MoveWindow(hDlg,
                (rcParent.left+rcParent.right-rect.right+rect.left)/2,
                (rcParent.top+rcParent.bottom-rect.bottom+rect.top)/2,
                rect.right - rect.left, rect.bottom - rect.top, false);
                
            return lbRc;
        }
        
    case WM_CTLCOLORSTATIC:
        if (GetDlgItem(hDlg, IDC_WARNING) == (HWND)lParam)
        {
            SetTextColor((HDC)wParam, 255);
            HBRUSH hBrush = GetSysColorBrush(COLOR_3DFACE);
            SetBkMode((HDC)wParam, TRANSPARENT);
            return (BOOL)hBrush;
        }
        break;

    case WM_GETICON:
        if (wParam==ICON_BIG)
        {
            /*SetWindowLong(hWnd2, DWL_MSGRESULT, (LRESULT)hClassIcon);
            return 1;*/
        }
        else
        {
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LRESULT)hClassIconSm);
            return 1;
        }
        return 0;
        
    case UM_USER_CONTROLS:
    	{
            if (SendDlgItemMessage(hDlg, rbCurrentUser, BM_GETCHECK, 0, 0))
            {
            	EnableWindow(GetDlgItem(hDlg, cbRunAsRestricted), TRUE);
            	//BOOL lbText = SendDlgItemMessage(hDlg, cbRunAsRestricted, BM_GETCHECK, 0, 0) == 0;
            	EnableWindow(GetDlgItem(hDlg, tRunAsUser), FALSE);
            	EnableWindow(GetDlgItem(hDlg, tRunAsPassword), FALSE);
            }
            else
            {
				if (SendDlgItemMessage(hDlg, tRunAsUser, CB_GETCOUNT, 0, 0) == 0)
				{
					DWORD dwLevel = 3, dwEntriesRead = 0, dwTotalEntries = 0, dwResumeHandle = 0;
					NET_API_STATUS nStatus;

					USER_INFO_3 *info = NULL;
					nStatus = ::NetUserEnum(NULL, dwLevel, FILTER_NORMAL_ACCOUNT, (PBYTE*) & info, 
						MAX_PREFERRED_LENGTH, &dwEntriesRead, &dwTotalEntries, &dwResumeHandle);
					if (nStatus == NERR_Success)
					{
						for (DWORD i = 0; i < dwEntriesRead; ++i)
						{
							// usri3_logon_server	"\\*"	wchar_t *
							if ((info[i].usri3_flags & UF_ACCOUNTDISABLE) == 0)
								SendDlgItemMessage(hDlg, tRunAsUser, CB_ADDSTRING, 0, (LPARAM)info[i].usri3_name);
						}
						::NetApiBufferFree(info);
					}
					else
					{
						// Добавить хотя бы текущего
						wchar_t szCurUser[MAX_PATH];
						if (GetWindowText(GetDlgItem(hDlg, tRunAsUser), szCurUser, countof(szCurUser)))
							SendDlgItemMessage(hDlg, tRunAsUser, CB_ADDSTRING, 0, (LPARAM)szCurUser);
					}
				}

            	EnableWindow(GetDlgItem(hDlg, cbRunAsRestricted), FALSE);
            	EnableWindow(GetDlgItem(hDlg, tRunAsUser), TRUE);
            	EnableWindow(GetDlgItem(hDlg, tRunAsPassword), TRUE);
            }
			if (wParam == rbAnotherUser)
				SetFocus(GetDlgItem(hDlg, tRunAsUser));
    	}
    	return 0;

    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            switch (LOWORD(wParam))
            {
                case IDC_CHOOSE:
                {
                    wchar_t *pszFilePath = NULL;
                    int nLen = MAX_PATH*2;
                    pszFilePath = (wchar_t*)calloc(nLen+1,2);
                    if (!pszFilePath) return 1;
                    
                    OPENFILENAME ofn; memset(&ofn,0,sizeof(ofn));
                    ofn.lStructSize=sizeof(ofn);
                    ofn.hwndOwner = hDlg;
                    ofn.lpstrFilter = _T("Executables (*.exe)\0*.exe\0\0");
                    ofn.nFilterIndex = 1;
                    ofn.lpstrFile = pszFilePath;
                    ofn.nMaxFile = nLen;
                    ofn.lpstrTitle = _T("Choose program to run");
                    ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
                            | OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST;
                    if (GetOpenFileName(&ofn))
                        SetDlgItemText(hDlg, IDC_RESTART_CMD, pszFilePath);

                    SafeFree(pszFilePath); 
                    return 1;
                }
                
                case IDC_CHOOSE_DIR:
                {
                	BROWSEINFO bi = {ghWnd};
                	wchar_t szFolder[MAX_PATH+1] = {0};
                	GetDlgItemText(hDlg, IDC_STARTUP_DIR, szFolder, countof(szFolder));
                	bi.pszDisplayName = szFolder;
                	wchar_t szTitle[100];
                	bi.lpszTitle = wcscpy(szTitle, L"Choose startup directory");
                	bi.ulFlags = BIF_EDITBOX | BIF_RETURNONLYFSDIRS | BIF_VALIDATE;
                	bi.lpfn = BrowseCallbackProc;
                	bi.lParam = (LPARAM)szFolder;
                	
                	LPITEMIDLIST pRc = SHBrowseForFolder(&bi);
                	if (pRc)
                	{
                		if (SHGetPathFromIDList(pRc, szFolder))
                		{
                			SetDlgItemText(hDlg, IDC_STARTUP_DIR, szFolder);
						}

                		CoTaskMemFree(pRc);
                	}
                	return 1;
                }
                
                case cbRunAsAdmin:
                {
                	// BCM_SETSHIELD = 5644
               		BOOL bRunAs = SendDlgItemMessage(hDlg, cbRunAsAdmin, BM_GETCHECK, 0, 0);
                	if (gOSVer.dwMajorVersion >= 6)
                	{
                		SendDlgItemMessage(hDlg, IDC_START, 5644/*BCM_SETSHIELD*/, 0, bRunAs);
            		}
            		if (bRunAs)
            		{
			            CheckRadioButton(hDlg, rbCurrentUser, rbAnotherUser, rbCurrentUser);
			            CheckDlgButton(hDlg, cbRunAsRestricted, BST_UNCHECKED);
			            RecreateDlgProc(hDlg, UM_USER_CONTROLS, 0, 0);
            		}
                	return 1;
                }
                
                case rbCurrentUser:
                case rbAnotherUser:
                case cbRunAsRestricted:
                {
                	RecreateDlgProc(hDlg, UM_USER_CONTROLS, LOWORD(wParam), 0);
                	return 1;
                }
                    
                case IDC_START:
                {
					RConStartArgs* pArgs = (RConStartArgs*)GetWindowLongPtr(hDlg, DWLP_USER);
					_ASSERTE(pArgs);
					
					SafeFree(pArgs->pszUserName);
					//SafeFree(pArgs->pszUserPassword);
					if (SendDlgItemMessage(hDlg, rbAnotherUser, BM_GETCHECK, 0, 0))
					{
						pArgs->bRunAsRestricted = FALSE;
						pArgs->pszUserName = GetDlgItemText(hDlg, tRunAsUser);
						if (pArgs->pszUserName)
						{
							//pArgs->pszUserPassword = GetDlgItemText(hDlg, tRunAsPassword);
							// Попытаться проверить правильность введенного пароля и возможность запуска
							if (!pArgs->CheckUserToken(GetDlgItem(hDlg, tRunAsPassword)))
								return 1;
						}
					}
					else
					{
						pArgs->bRunAsRestricted = SendDlgItemMessage(hDlg, cbRunAsRestricted, BM_GETCHECK, 0, 0);
					}

					// Command
					_ASSERTE(pArgs->pszSpecialCmd==NULL);
					pArgs->pszSpecialCmd = GetDlgItemText(hDlg, IDC_RESTART_CMD);
					if (pArgs->pszSpecialCmd)
						gSet.HistoryAdd(pArgs->pszSpecialCmd);
                    
					// StartupDir
					_ASSERTE(pArgs->pszStartupDir==NULL);
					pArgs->pszStartupDir = GetDlgItemText(hDlg, IDC_STARTUP_DIR);
                    
                    // Vista+ (As Admin...)
					pArgs->bRunAsAdministrator = SendDlgItemMessage(hDlg, cbRunAsAdmin, BM_GETCHECK, 0, 0);
					
                    EndDialog(hDlg, IDC_START);
                    return 1;
                }
                case IDC_TERMINATE:
                    EndDialog(hDlg, IDC_TERMINATE);
                    return 1;
                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return 1;
            }
        }
        break;
    default:
        return 0;
    }
    return 0;
}

void CConEmuMain::ShowOldCmdVersion(DWORD nCmd, DWORD nVersion, int bFromServer)
{
    if (!isMainThread())
    {
        if (mb_InShowOldCmdVersion)
            return; // уже послано
        mb_InShowOldCmdVersion = TRUE;
        PostMessage(ghWnd, mn_MsgOldCmdVer, (nCmd & 0xFFFF) | (((DWORD)(unsigned short)bFromServer) << 16), nVersion);
        return;
    }

	static bool lbErrorShowed = false;
	if (lbErrorShowed) return;
	lbErrorShowed = true;

    wchar_t szMsg[255];
    wsprintf(szMsg, L"ConEmu received old version packet!\nCommandID: %i, Version: %i, ReqVersion: %i\nPlease check %s",
    	nCmd, nVersion, CESERVER_REQ_VER,
    	(bFromServer==1) ? L"ConEmuC*.exe" : (bFromServer==0) ? L"ConEmu*.dll" : L"ConEmuC*.exe and ConEmu*.dll");
    MBox(szMsg);

    mb_InShowOldCmdVersion = FALSE; // теперь можно показать еще одно...
}

LRESULT CConEmuMain::OnInitMenuPopup(HWND hWnd, HMENU hMenu, LPARAM lParam)
{
	DefWindowProc(hWnd, WM_INITMENUPOPUP, (WPARAM)hMenu, lParam);
	
	if (HIWORD(lParam))
	{
		BOOL bSelectionExist = ActiveCon()->RCon()->isSelectionPresent();
    	EnableMenuItem(hMenu, ID_CON_COPY, MF_BYCOMMAND | (bSelectionExist?MF_ENABLED:MF_GRAYED));
    	
    	CheckMenuItem(hMenu, ID_DEBUG_SHOWRECTS, MF_BYCOMMAND|(gbDebugShowRects ? MF_CHECKED : MF_UNCHECKED));

    	#ifdef _DEBUG
		wchar_t szText[128];
		MENUITEMINFO mi = {sizeof(MENUITEMINFO)};
		mi.fMask = MIIM_STRING|MIIM_STATE;
		bool bLogged = false, bAllowed = false;
		CRealConsole* pRCon = mp_VActive->RCon();
		if (pRCon)
		{
			bLogged = pRCon->IsLogShellStarted();
			bAllowed = (pRCon->GetFarPID(TRUE) != 0);
		}
		lstrcpy(szText, bLogged ? _T("Disable &shell log") : _T("Enable &shell log..."));
		mi.dwTypeData = szText;
		mi.fState = bAllowed ? MFS_ENABLED : MFS_GRAYED;
		SetMenuItemInfo(hMenu, ID_MONITOR_SHELLACTIVITY, FALSE, &mi);
		#endif
	}
	
	return 0;
}

void CConEmuMain::ShowSysmenu(HWND Wnd, int x, int y)
{
	if (!Wnd)
		Wnd = ghWnd;
	if ((x == -32000) || (y == -32000))
	{
		RECT rect, cRect;
		GetWindowRect(ghWnd, &rect);
		GetClientRect(ghWnd, &cRect);
		WINDOWINFO wInfo;   GetWindowInfo(ghWnd, &wInfo);
		int nTabShift =
			((gSet.isHideCaptionAlways() || gSet.isFullScreen) && gConEmu.mp_TabBar->IsShown()) 
				? gConEmu.mp_TabBar->GetTabbarHeight() : 0;

		if (x == -32000)
			x = rect.right - cRect.right - wInfo.cxWindowBorders;
		if (y == -32000)
			y = rect.bottom - cRect.bottom - wInfo.cyWindowBorders + nTabShift;
	}
    bool iconic = isIconic();
    bool zoomed = isZoomed();
    bool visible = IsWindowVisible(Wnd);
    int style = GetWindowLong(Wnd, GWL_STYLE);

    HMENU systemMenu = GetSystemMenu(Wnd, false);
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

    SendMessage(Wnd, WM_INITMENU, (WPARAM)systemMenu, 0);
    SendMessage(Wnd, WM_INITMENUPOPUP, (WPARAM)systemMenu, MAKELPARAM(0, true));

    // Переехало в OnMenuPopup
	//BOOL bSelectionExist = ActiveCon()->RCon()->isSelectionPresent();
    //EnableMenuItem(systemMenu, ID_CON_COPY, MF_BYCOMMAND | (bSelectionExist?MF_ENABLED:MF_GRAYED));
    
    SetActiveWindow(Wnd);

	mb_InTrackSysMenu = TRUE;
    int command = TrackPopupMenu(systemMenu, TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, x, y, 0, Wnd, NULL);
	mb_InTrackSysMenu = FALSE;

    if (Icon.isWindowInTray)
        switch(command)
        {
        case SC_RESTORE:
        case SC_MOVE:
        case SC_SIZE:
        case SC_MINIMIZE:
        case SC_MAXIMIZE:
            SendMessage(Wnd, WM_TRAYNOTIFY, 0, WM_LBUTTONDOWN);
            break;
        }

    if (command)
        PostMessage(Wnd, WM_SYSCOMMAND, (WPARAM)command, 0);
}

void CConEmuMain::StartDebugLogConsole()
{
	if (IsDebuggerPresent())
		return; // УЖЕ!

	// Create process, with flag /Attach GetCurrentProcessId()
	// Sleep for sometimes, try InitHWND(hConWnd); several times
	WCHAR  szExe[0x200] = {0};
	BOOL lbRc = FALSE;
	
	//DWORD nLen = 0;
	PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
	STARTUPINFO si; memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	DWORD dwSelfPID = GetCurrentProcessId();
	
	// "/ATTACH" - низя, а то заблокируемся при попытке подключения к "отлаживаемому" GUI
	wsprintf(szExe, L"\"%s\" /DEBUGPID=%i /BW=80 /BH=25 /BZ=1000", 
		ms_ConEmuCExe, dwSelfPID);
	
	if (!CreateProcess(NULL, szExe, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS|CREATE_NEW_CONSOLE, NULL,
			NULL, &si, &pi))
	{
		// Хорошо бы ошибку показать?
		DWORD dwErr = GetLastError();
		wchar_t szErr[128]; wsprintf(szErr, L"Can't create debugger console! ErrCode=0x%08X", dwErr);
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
//	for (int i = 0; i < MAX_CONSOLE_COUNT; i++)
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

    wchar_t szNo[32];
    DWORD nProgramStatus = mp_VActive->RCon()->GetProgramStatus();
    DWORD nFarStatus = mp_VActive->RCon()->GetFarStatus();
    CheckDlgButton(gSet.hInfo, cbsTelnetActive, (nProgramStatus&CES_TELNETACTIVE) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(gSet.hInfo, cbsNtvdmActive, (nProgramStatus&CES_NTVDM) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(gSet.hInfo, cbsFarActive, (nProgramStatus&CES_FARACTIVE) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(gSet.hInfo, cbsFilePanel, (nFarStatus&CES_FILEPANEL) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(gSet.hInfo, cbsEditor, (nFarStatus&CES_EDITOR) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(gSet.hInfo, cbsViewer, (nFarStatus&CES_VIEWER) ? BST_CHECKED : BST_UNCHECKED);
    wsprintfW(szNo, L"%i/%i", mp_VActive->RCon()->GetFarPID(), mp_VActive->RCon()->GetFarPID(TRUE));
    SetDlgItemText(gSet.hInfo, tsTopPID, szNo);
	CheckDlgButton(gSet.hInfo, cbsProgress, ((nFarStatus&CES_WASPROGRESS) /*|| mp_VActive->RCon()->GetProgress(NULL)>=0*/) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(gSet.hInfo, cbsProgressError, (nFarStatus&CES_OPER_ERROR) ? BST_CHECKED : BST_UNCHECKED);

    if (!abForce)
        return;

    MCHKHEAP
    SendDlgItemMessage(gSet.hInfo, lbProcesses, LB_RESETCONTENT, 0, 0);
    wchar_t temp[MAX_PATH];
    for (int j=0; j<MAX_CONSOLE_COUNT; j++)
    {
        if (mp_VCon[j] == NULL) continue;

        ConProcess* pPrc = NULL;
        int nCount = mp_VCon[j]->RCon()->GetProcesses(&pPrc);
        for (int i=0; i<nCount; i++)
        {
            if (mp_VCon[j] == mp_VActive)
                _tcscpy(temp, _T("(*) "));
            else
                temp[0] = 0;

            swprintf(temp+_tcslen(temp), _T("[%i.%i] %s - PID:%i"),
                j+1, i, pPrc[i].Name, pPrc[i].ProcessID);
            SendDlgItemMessage(gSet.hInfo, lbProcesses, LB_ADDSTRING, 0, (LPARAM)temp);
        }
        if (pPrc) { free(pPrc); pPrc = NULL; }
    }
    MCHKHEAP
}

void CConEmuMain::UpdateCursorInfo(COORD crCursor)
{
	if (!ghOpWnd || !gSet.hInfo) return;

	if (!isMainThread())
	{
		DWORD wParam = MAKELONG(crCursor.X, crCursor.Y);
		PostMessage(ghWnd, mn_MsgUpdateCursorInfo, wParam, 0);
		return;
	}

	TCHAR szCursor[32]; wsprintf(szCursor, _T("%ix%i"), (int)crCursor.X, (int)crCursor.Y);
	SetDlgItemText(gSet.hInfo, tCursorPos, szCursor);
}

void CConEmuMain::UpdateSizes()
{
    if (!ghOpWnd || !gSet.hInfo)
    {
    	// Может курсор-сплиттер нужно убрать или поставить
    	PostMessage(ghWnd, WM_SETCURSOR, -1, -1);
        return;
    }
    
    if (!isMainThread())
    {
        PostMessage(ghWnd, mn_MsgUpdateSizes, 0, 0);
        return;
    }

	// Может курсор-сплиттер нужно убрать или поставить
	SendMessage(ghWnd, WM_SETCURSOR, -1, -1);
    
    if (mp_VActive)
    {
        mp_VActive->UpdateInfo();
    }
    else
    {
        SetDlgItemText(gSet.hInfo, tConSizeChr, _T("?"));
        SetDlgItemText(gSet.hInfo, tConSizePix, _T("?"));
        SetDlgItemText(gSet.hInfo, tPanelLeft, _T("?"));
        SetDlgItemText(gSet.hInfo, tPanelRight, _T("?"));
    }
    RECT rcClient; GetClientRect(ghWndDC, &rcClient);
    TCHAR szSize[32]; wsprintf(szSize, _T("%ix%i"), rcClient.right, rcClient.bottom);
    SetDlgItemText(gSet.hInfo, tDCSize, szSize);
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

	LPCTSTR asNewTitle = NULL;
	if (mp_VActive && mp_VActive->RCon())
		asNewTitle = mp_VActive->RCon()->GetTitle();
	if (!asNewTitle)
	{
    	//if ((asNewTitle = mp_VActive->RCon()->GetTitle()) == NULL)
    		return;
	}
    
    wcscpy(Title, asNewTitle);

	// SetWindowText(ghWnd, psTitle) вызывается здесь
    // Там же обновится L"[%i/%i] " если несколько консолей а табы отключены
    UpdateProgress(/*TRUE*/);

    Icon.UpdateTitle();

    // Под конец - проверить список процессов консоли
    CheckProcesses();
}

// Если в текущей консоли есть проценты - отображаются они
// Иначе - отображается максимальное значение процентов из всех консолей
void CConEmuMain::UpdateProgress(/*BOOL abUpdateTitle*/)
{
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
    
    short nProgress = -1, n;
    BOOL bActiveHasProgress = FALSE;
	BOOL bWasError = FALSE;
    if ((nProgress = mp_VActive->RCon()->GetProgress(&bWasError)) >= 0)
	{
        mn_Progress = nProgress;
        bActiveHasProgress = TRUE;
    }
	// нас интересует возможное наличие ошибки во всех остальных консолях
    for (UINT i = 0; i < MAX_CONSOLE_COUNT; i++)
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
        if (mp_TaskBar3)
		{
            if (mn_Progress >= 0)
			{
                hr = mp_TaskBar3->SetProgressValue(ghWnd, mn_Progress, 100);
                if (nLastProgress == -1 || bLastProgressError != bWasError)
					hr = mp_TaskBar3->SetProgressState(ghWnd, bWasError ? TBPF_ERROR : TBPF_NORMAL);
            }
			else
			{
                hr = mp_TaskBar3->SetProgressState(ghWnd, TBPF_NOPROGRESS);
            }
        }
        // Запомнить последнее
        nLastProgress = mn_Progress;
		bLastProgressError = bWasError;
    }
    if (mn_Progress >= 0 && !bActiveHasProgress)
	{
        psTitle = MultiTitle;
        wsprintf(MultiTitle+lstrlen(MultiTitle), L"{*%i%%} ", mn_Progress);
    }

    if (gSet.isMulti && !gConEmu.mp_TabBar->IsShown())
	{
        int nCur = 1, nCount = 0;
        for (int n=0; n<MAX_CONSOLE_COUNT; n++)
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
            wsprintf(MultiTitle+lstrlen(MultiTitle), L"[%i/%i] ", nCur, nCount);
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

	//if (gSet.isHideCaptionAlways) {
	//	KillTimer(ghWnd, TIMER_CAPTION_APPEAR_ID);
	//	KillTimer(ghWnd, TIMER_CAPTION_DISAPPEAR_ID);
	//}
	
	if (mb_ForceShowFrame)
		hRgn = NULL; // Иначе при ресайзе получаются некрасивые (без XP Theme) кнопки в заголовке
	else if (anWndWidth != -1 && anWndHeight != -1)
		hRgn = CreateWindowRgn(false, false, anX, anY, anWndWidth, anWndHeight);
	else
		hRgn = CreateWindowRgn();

	if (hRgn) {
		mb_LastRgnWasNull = FALSE;
	} else {
		BOOL lbPrev = mb_LastRgnWasNull;
		mb_LastRgnWasNull = TRUE;
		if (lbPrev)
			return; // менять не нужно
	}

	// Облом получается при двукратном SetWindowRgn(ghWnd, NULL, TRUE);
	// Причем после следующего ресайза - рамка приходит в норму

	SetWindowRgn(ghWnd, hRgn, TRUE);
}

VOID CConEmuMain::WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD anEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    _ASSERTE(hwnd!=NULL);
    // Процесс может запуститься и ДО того, как закончится инициализация хэндла окна!

	// Не помогает
	//if (anEvent == EVENT_SYSTEM_MENUPOPUPSTART) {
	//	if (gConEmu.isMeForeground())
	//	{
	//		//SetForegroundWindow(hwnd);
	//		DWORD dwPID = 0;
	//		GetWindowThreadProcessId(hwnd, &dwPID);
	//		AllowSetForegroundWindow(dwPID);
	//		HWND hParent = GetAncestor(hwnd, GA_PARENT); // Это Desktop
	//		if (hParent) {
	//			GetWindowThreadProcessId(hParent, &dwPID);
	//			AllowSetForegroundWindow(dwPID);
	//		}
	//	}
	//	//hwnd это похоже окошко самого меню, ClassName=#32768 (диалог)
	//	//#ifdef _DEBUG
	//	//wchar_t szClass[128] = {0}, szTitle[128] = {0};
	//	//HWND hParent = GetAncestor(hwnd, GA_PARENT); // Это Desktop
	//	//GetClassName(hParent, szClass, 128); GetWindowText(hParent, szTitle, 128);
	//	//WCHAR szDbg[512]; wsprintfW(szDbg, L"EVENT_SYSTEM_MENUPOPUPSTART(HWND=0x%08X, object=0x%08X, child=0x%08X)\nClass: %s, Title: %s\n\n", hwnd, idObject, idChild, szClass, szTitle);
	//	//OutputDebugString(szDbg);
	//	//#endif
	//	return;
	//}

#ifdef _DEBUG
    switch(anEvent)
    {
    case EVENT_CONSOLE_START_APPLICATION:
        //A new console process has started. 
        //The idObject parameter contains the process identifier of the newly created process. 
        //If the application is a 16-bit application, the idChild parameter is CONSOLE_APPLICATION_16BIT and idObject is the process identifier of the NTVDM session associated with the console.
		if ((idChild == CONSOLE_APPLICATION_16BIT) && gSet.isAdvLogging) {
			char szInfo[64]; wsprintfA(szInfo, "NTVDM started, PID=%i\n", idObject);
			gConEmu.mp_VActive->RCon()->LogString(szInfo, TRUE);
		}

        #ifdef _DEBUG
        WCHAR szDbg[128]; wsprintfW(szDbg, L"EVENT_CONSOLE_START_APPLICATION(HWND=0x%08X, PID=%i%s)\n", hwnd, idObject, (idChild == CONSOLE_APPLICATION_16BIT) ? L" 16bit" : L"");
        DEBUGSTRCONEVENT(szDbg);
        #endif
        break;

    case EVENT_CONSOLE_END_APPLICATION:
        //A console process has exited. 
        //The idObject parameter contains the process identifier of the terminated process.
		if ((idChild == CONSOLE_APPLICATION_16BIT) && gSet.isAdvLogging) {
			char szInfo[64]; wsprintfA(szInfo, "NTVDM stopped, PID=%i\n", idObject);
			gConEmu.mp_VActive->RCon()->LogString(szInfo, TRUE);
		}

        #ifdef _DEBUG
        wsprintfW(szDbg, L"EVENT_CONSOLE_END_APPLICATION(HWND=0x%08X, PID=%i%s)\n", hwnd, idObject, 
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
        WCHAR szDbg[128]; wsprintfW(szDbg, L"EVENT_CONSOLE_UPDATE_REGION({%i, %i} - {%i, %i})\n", crStart.X,crStart.Y, crEnd.X,crEnd.Y);
        DEBUGSTRCONEVENT(szDbg);
        #endif
        } break;
    case EVENT_CONSOLE_UPDATE_SCROLL: //0x4004
        {
        //The console has scrolled.
        //The idObject parameter is the horizontal distance the console has scrolled. 
        //The idChild parameter is the vertical distance the console has scrolled.
        #ifdef _DEBUG
        WCHAR szDbg[128]; wsprintfW(szDbg, L"EVENT_CONSOLE_UPDATE_SCROLL(X=%i, Y=%i)\n", idObject, idChild);
        DEBUGSTRCONEVENT(szDbg);
        #endif
        } break;
    case EVENT_CONSOLE_UPDATE_SIMPLE: //0x4003
        {
        //A single character has changed.
        //The idObject parameter is a COORD structure that specifies the character that has changed.
        //Warning! В писании от  микрософта тут ошибка!
        //The idChild parameter specifies the character in the low word and the character attributes in the high word.
        #ifdef _DEBUG
        COORD crWhere; memmove(&crWhere, &idObject, sizeof(idObject));
        WCHAR ch = (WCHAR)LOWORD(idChild); WORD wA = HIWORD(idChild);
        WCHAR szDbg[128]; wsprintfW(szDbg, L"EVENT_CONSOLE_UPDATE_SIMPLE({%i, %i} '%c'(\\x%04X) A=%i)\n", crWhere.X,crWhere.Y, ch, ch, wA);
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
        WCHAR szDbg[128]; wsprintfW(szDbg, L"EVENT_CONSOLE_CARET({%i, %i} Sel=%c, Vis=%c\n", crWhere.X,crWhere.Y, 
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

    BOOL lbProcessed = FALSE, lbWaitingExist = FALSE;
	for (int k = 0; k < 2 && !lbProcessed; k++)
	{
		for (int i = 0; i < MAX_CONSOLE_COUNT; i++) {
			if (!gConEmu.mp_VCon[i]) continue;

			// Запускаемые через "-new_console" цепляются через CECMD_ATTACH2GUI, а не через WinEvent
			if (gConEmu.mp_VCon[i]->RCon()->isDetached())
				continue;

			if (!k && gConEmu.mp_VCon[i]->RCon()->InCreateRoot()) {
				lbWaitingExist = TRUE;
				continue;
			}

			LONG nSrvPID = (LONG)gConEmu.mp_VCon[i]->RCon()->GetServerPID();
			#ifdef _DEBUG
			if (nSrvPID == 0) {
				_ASSERTE(nSrvPID != 0);
			}
			#endif

			HWND hRConWnd = gConEmu.mp_VCon[i]->RCon()->ConWnd();
			if (
				(hRConWnd == hwnd) ||
				(hRConWnd == NULL && anEvent == EVENT_CONSOLE_START_APPLICATION && idObject == nSrvPID)
				)
			{
				gConEmu.mp_VCon[i]->RCon()->OnWinEvent(anEvent, hwnd, idObject, idChild, dwEventThread, dwmsEventTime);
				lbProcessed = TRUE;
				break;
			}
		}
		if (!lbWaitingExist)
			break;
		if (!lbProcessed)
			Sleep(100);
	}
    //// Если событие "Запущен процесс" пришло ДО того, как в VirtualConsole определился
    //// хэндл консольного окна - передать событие в тот VirtualConsole, в котором
    //// mn_ConEmuC_PID == idObject
    //if (!lbProcessed && anEvent == EVENT_CONSOLE_START_APPLICATION && idObject) {
    //    // Warning. В принципе, за время выполнения этой процедуры mp_VCon[i]->hConWnd мог уже проинициализироваться
    //    for (int i=0; i<MAX_CONSOLE_COUNT; i++) {
    //        if (!gConEmu.mp_VCon[i]) continue;
    //        if (gConEmu.mp_VCon[i]->RCon()->ConWnd() == hwnd ||
    //            gConEmu.mp_VCon[i]->RCon()->GetServerPID() == (DWORD)idObject)
    //        {
    //            gConEmu.mp_VCon[i]->RCon()->OnWinEvent(anEvent, hwnd, idObject, idChild, dwEventThread, dwmsEventTime);
    //            lbProcessed = TRUE;
    //            break;
    //        }
    //    }
    //}
        
    //switch(anEvent)
    //{
    //case EVENT_CONSOLE_START_APPLICATION:
    //    //#pragma message("Win2k: CONSOLE_APPLICATION_16BIT")
    //    if (idChild == CONSOLE_APPLICATION_16BIT) {
    //        DWORD ntvdmPID = idObject;
    //        for (size_t i=0; i<gConEmu.m_Processes.size(); i++) {
    //            DWORD dwPID = gConEmu.m_Processes[i].ProcessID;
    //            if (dwPID == ntvdmPID) {
    //                gConEmu.mn_ActiveStatus |= CES_NTVDM;
    //                //TODO: их могут запускать и в разных консолях...
    //                SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    //            }
    //        }
    //    }
    //    break;
    //case EVENT_CONSOLE_END_APPLICATION:
    //    if (idChild == CONSOLE_APPLICATION_16BIT) {
    //        DWORD ntvdmPID = idObject;
    //        for (size_t i=0; i<gConEmu.m_Processes.size(); i++) {
    //            DWORD dwPID = gConEmu.m_Processes[i].ProcessID;
    //            if (dwPID == ntvdmPID) {
    //                gConEmu.gbPostUpdateWindowSize = true;
    //                gConEmu.mn_ActiveStatus &= ~CES_NTVDM;
    //                SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
    //            }
    //        }
    //    }
    //    break;
    //}
}


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
	if (!this || !apVCon) return;
	TODO("После добавления второго viewport'а потребуется доработка");
	m_Child->Invalidate();
}

void CConEmuMain::InvalidateAll()
{
    InvalidateRect(ghWnd, NULL, TRUE);
    m_Child->Invalidate();
    m_Back->Invalidate();
    gConEmu.mp_TabBar->Invalidate();
}

bool CConEmuMain::IsGlass()
{
	if (gOSVer.dwMajorVersion < 6)
		return false;

	if (!mh_DwmApi)
	{
		mh_DwmApi = LoadLibrary(L"dwmapi.dll");
		if (!mh_DwmApi)
			mh_DwmApi = (HMODULE)INVALID_HANDLE_VALUE;
	}
	if (mh_DwmApi != INVALID_HANDLE_VALUE)
		return false;
	if (!DwmIsCompositionEnabled && mh_DwmApi)
	{
		DwmIsCompositionEnabled = (FDwmIsCompositionEnabled)GetProcAddress(mh_DwmApi, "DwmIsCompositionEnabled");
		if (!DwmIsCompositionEnabled)
			return false;
	}

	BOOL composition_enabled = FALSE;
	return DwmIsCompositionEnabled(&composition_enabled) == S_OK &&
		composition_enabled /*&& g_glass*/;
}

LRESULT CConEmuMain::OnNcMessage(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRc = 0;
	
	if (hWnd != ghWnd)
	{
		lRc = DefWindowProc(hWnd, messg, wParam, lParam);
	}
	else
	{
		BOOL lbUseCaption = gSet.isTabsInCaption && gConEmu.mp_TabBar->IsActive();
		
		switch (messg)
		{
			case WM_NCHITTEST:
			{
				lRc = DefWindowProc(hWnd, messg, wParam, lParam);
				if (gSet.isHideCaptionAlways() && !gConEmu.mp_TabBar->IsShown() && lRc == HTTOP)
					lRc = HTCAPTION;
				break;
			}
			case WM_NCPAINT:
			{
				if (lbUseCaption)
					lRc = gConEmu.OnNcPaint((HRGN)wParam);
				else
					lRc = DefWindowProc(hWnd, messg, wParam, lParam);
				break;
			}
		    case WM_NCACTIVATE:
		    {
				// Force paint our non-client area otherwise Windows will paint its own.
				if (lbUseCaption)
					RedrawWindow(hWnd, NULL, NULL, RDW_UPDATENOW);
				else
					lRc = DefWindowProc(hWnd, messg, wParam, lParam);
				break;
			}
			case 0x31E: // WM_DWMCOMPOSITIONCHANGED:
			{
				if (lbUseCaption)
				{
					lRc = 0;
					/*
					DWMNCRENDERINGPOLICY policy = g_glass ? DWMNCRP_ENABLED : DWMNCRP_DISABLED;
					DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY,
					                    &policy, sizeof(DWMNCRENDERINGPOLICY));

					SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
					           SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
					RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
					*/
				}
				else
					lRc = DefWindowProc(hWnd, messg, wParam, lParam);
				break;
			}
		    case 0xAE: // WM_NCUAHDRAWCAPTION:
		    case 0xAF: // WM_NCUAHDRAWFRAME:
		    {
				lRc = lbUseCaption ? 0 : DefWindowProc(hWnd, messg, wParam, lParam);
				break;
			}
			case WM_NCCALCSIZE:
			{
				NCCALCSIZE_PARAMS *pParms = NULL;
				LPRECT pRect = NULL;
				if (wParam) pParms = (NCCALCSIZE_PARAMS*)lParam; else pRect = (LPRECT)lParam;
				lRc = DefWindowProc(hWnd, messg, wParam, lParam);
				break;
			}
			default:
				lRc = DefWindowProc(hWnd, messg, wParam, lParam);
		}
	}
	return lRc;
}

LRESULT CConEmuMain::OnNcPaint(HRGN hRgn)
{
	LRESULT lRc = 0, lMyRc = 0;
	//HRGN hFrameRgn = hRgn;

	RECT wr = {0}, dirty = {0}, dirty_box = {0};
	GetWindowRect(ghWnd, &wr);
	if (!hRgn || ((WPARAM)hRgn) == 1)
	{
		dirty = wr;
		dirty.left = dirty.top = 0;
	}
	else
	{
		GetRgnBox(hRgn, &dirty_box);
		if (!IntersectRect(&dirty, &dirty_box, &wr))
			return 0;
		OffsetRect(&dirty, -wr.left, -wr.top);
	}

	//hdc = GetWindowDC(hwnd);
	//br = CreateSolidBrush(RGB(255,0,0));
	//FillRect(hdc, &dirty, br);
	//DeleteObject(br);
	//ReleaseDC(hwnd, hdc);
	
	int nXFrame = GetSystemMetrics(SM_CXSIZEFRAME);
	int nYFrame = GetSystemMetrics(SM_CYSIZEFRAME);
	int nPad = GetSystemMetrics(92/*SM_CXPADDEDBORDER*/);
	int nXBtn = GetSystemMetrics(SM_CXSIZE);
	int nYBtn = GetSystemMetrics(SM_CYSIZE);
	int nYCaption = GetSystemMetrics(SM_CYCAPTION);

	int nXMBtn = GetSystemMetrics(SM_CXSMSIZE);
	int nYMBtn = GetSystemMetrics(SM_CYSMSIZE);
	int nXIcon = GetSystemMetrics(SM_CXSMICON);
	int nYIcon = GetSystemMetrics(SM_CYSMICON);

	RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);

	RECT rcUsing =
	{
		nXFrame + nXIcon, nYFrame, 
		(rcWnd.right-rcWnd.left) - nXFrame - 3*nXBtn,
		nYFrame + nYCaption
	};

	//lRc = DefWindowProc(ghWnd, WM_NCPAINT, (WPARAM)hRgn, 0);

	//HRGN hRect = CreateRectRgn(rcUsing.left,rcUsing.top,rcUsing.right,rcUsing.bottom);
	//hFrameRgn = CreateRectRgn(0,0,1,1);
	//int nRgn = CombineRgn(hFrameRgn, hRgn, hRect, RGN_XOR);
	//if (nRgn == ERROR)
	//{
	//	DeleteObject(hFrameRgn);
	//	hFrameRgn = hRgn;
	//}
	//else
	{
		// Рисуем
		HDC hdc = NULL;
		//hdc = GetDCEx(ghWnd, hRect, DCX_WINDOW|DCX_INTERSECTRGN);
		//hRect = NULL; // system maintains this region
		hdc = GetWindowDC(ghWnd);
		mp_TabBar->PaintHeader(hdc, rcUsing);
		ReleaseDC(ghWnd, hdc);
	}
	//if (hRect)
	//	DeleteObject(hRect);
	lMyRc = TRUE;

	//lRc = DefWindowProc(ghWnd, WM_NCPAINT, (WPARAM)hFrameRgn, 0);
	//
	//if (hRgn != hFrameRgn)
	//{
	//	DeleteObject(hFrameRgn); hFrameRgn = NULL;
	//}
	return lMyRc ? lMyRc : lRc;
}

bool CConEmuMain::isRightClickingPaint()
{
	return (bool)mb_RightClickingPaint;
}

void CConEmuMain::RightClickingPaint(HDC hdc /*= NULL*/)
{
	BOOL lbSucceeded = FALSE;
	
	if (gSet.isRClickSendKey > 1 && (mouse.state & MOUSE_R_LOCKED)
		&& m_RightClickingFrames > 0 && mh_RightClickingBmp)
	{
		WORD nRDown = GetKeyState(VK_RBUTTON);
		POINT ptCur; GetCursorPos(&ptCur);
		ScreenToClient(ghWnd, &ptCur);
		
		if ((nRDown & 0x8000))
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

				// Прикинуть индекс фрейма
				int nIndex = (dwDelta - RCLICKAPPS_START) * m_RightClickingFrames / (RCLICKAPPSTIMEOUT - RCLICKAPPS_START);
				if (nIndex >= m_RightClickingFrames)
				{
					nIndex = (m_RightClickingFrames-1); // рисуем последний фрейм, мышку можно отпускать
					//-- KillTimer(ghWnd, TIMER_RCLICKPAINT); // таймер понадобится для "скрытия" кружочка после RCLICKAPPSTIMEOUT_MAX
				}

				if (hdc || (m_RightClickingCurrent != nIndex))
				{
					// Рисуем
					BOOL lbSelfDC = FALSE;
					if (!hdc)
					{
						hdc = GetDC(ghWnd); lbSelfDC = TRUE;
					}
					
					HDC hCompDC = CreateCompatibleDC(hdc);
					HBITMAP hOld = (HBITMAP)SelectObject(hCompDC, mh_RightClickingBmp);
					
					int nHalf = m_RightClickingSize.y>>1;
					//BitBlt(hdc, Rcursor.x-nHalf, Rcursor.y-nHalf, m_RightClickingSize.y, m_RightClickingSize.y,
					//	hCompDC, nIndex*m_RightClickingSize.y, 0, SRCCOPY);
					BLENDFUNCTION bf = {AC_SRC_OVER,0,255,AC_SRC_ALPHA};
					GdiAlphaBlend(hdc, Rcursor.x-nHalf, Rcursor.y-nHalf, m_RightClickingSize.y, m_RightClickingSize.y,
						hCompDC, nIndex*m_RightClickingSize.y, 0, m_RightClickingSize.y, m_RightClickingSize.y, bf);

					
					if (hOld && hCompDC)
						SelectObject(hCompDC, hOld);
					
					if (lbSelfDC && hdc)
						DeleteDC(hdc);
				}

				// Запомним фрейм, что рисовали в последний раз
				m_RightClickingCurrent = nIndex;
			}
		}
	}
	
	if (!lbSucceeded)
	{
		StopRightClickingPaint();
	}
}

void CConEmuMain::StartRightClickingPaint()
{
	if (!mb_RightClickingPaint && m_RightClickingFrames > 0 && mh_RightClickingBmp)
	{
		m_RightClickingCurrent = -1;
		mb_RightClickingPaint = TRUE;
		SetTimer(ghWnd, TIMER_RCLICKPAINT, TIMER_RCLICKPAINT_ELAPSE, NULL);
	}
}

void CConEmuMain::StopRightClickingPaint()
{
	if (mb_RightClickingPaint)
	{
		mb_RightClickingPaint = FALSE;
		KillTimer(ghWnd, TIMER_RCLICKPAINT);
		m_RightClickingCurrent = -1;
		Invalidate(ActiveCon());
	}
}

LRESULT CConEmuMain::OnPaint(WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

	#ifdef _DEBUG
	RECT rcDbgSize; GetWindowRect(ghWnd, &rcDbgSize);
	wchar_t szSize[255]; swprintf_s(szSize, L"WM_PAINT -> Window size (X=%i, Y=%i, W=%i, H=%i)\n",
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
	//RECT rcClient; GetClientRect(ghWnd, &rcClient);
	//RECT rcTabMargins = CalcMargins(CEM_TAB);
	//AddMargins(rcClient, rcTabMargins, FALSE);

	//HDC hdc = GetDC(ghWnd);

    PaintGaps(ps.hdc);

	PaintCon(ps.hdc);
	//PaintCon(hdc);
	
	if (mb_RightClickingPaint)
	{
		// Нарисует кружочек, или сбросит таймер, если кнопку отпустили
		RightClickingPaint(ps.hdc);
	}

    EndPaint(ghWnd, &ps);
    //result = DefWindowProc(ghWnd, WM_PAINT, wParam, lParam);

	//ReleaseDC(ghWnd, hdc);

	//ValidateRect(ghWnd, &rcClient);

    return result;
}

void CConEmuMain::PaintGaps(HDC hDC)
{
	if (hDC==NULL)
		hDC = GetDC(ghWnd); // Главное окно!

	int
	#ifdef _DEBUG
		mn_ColorIdx = 1; // Blue
	#else
		mn_ColorIdx = 0; // Black
	#endif
	HBRUSH hBrush = CreateSolidBrush(gSet.GetColors(isMeForeground())[mn_ColorIdx]);

	RECT rcClient; GetClientRect(ghWnd, &rcClient); // Клиентская часть главного окна

	//RECT rcMargins = CalcMargins(CEM_TAB); // Откусить площадь, занятую строкой табов
	//AddMargins(rcClient, rcMargins, FALSE);
	//// На старте при /max - ghWndDC еще не изменил свое положение
	////RECT offsetRect; GetClientRect(ghWndDC, &offsetRect);
	//RECT rcWndClient; GetClientRect(ghWnd, &rcWndClient);
	//RECT rcCalcCon = gConEmu.CalcRect(CER_BACK, rcWndClient, CER_MAINCLIENT);
	//RECT rcCon = gConEmu.CalcRect(CER_CONSOLE, rcCalcCon, CER_BACK);
	// -- работает не правильно - не учитывает центрирование в Maximized
	//RECT offsetRect = gConEmu.CalcRect(CER_BACK, rcCon, CER_CONSOLE);

	/*
	RECT rcClient = {0};
	if (ghWndDC) {
		GetClientRect(ghWndDC, &rcClient);
		MapWindowPoints(ghWndDC, ghWnd, (LPPOINT)&rcClient, 2);
	}
	*/

	RECT dcSize = CalcRect(CER_DC, rcClient, CER_MAINCLIENT);
	RECT client = CalcRect(CER_DC, rcClient, CER_MAINCLIENT, NULL, &dcSize);


	WARNING("Вынести в CalcRect");
    RECT offsetRect; memset(&offsetRect,0,sizeof(offsetRect));
    if (mp_VActive && mp_VActive->Width && mp_VActive->Height)
    {
        if ((gSet.isTryToCenter && (isZoomed() || gSet.isFullScreen))
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
	GdiFlush();
#endif

	// right
	rect.left = offsetRect.right;
	rect.bottom = rcClient.bottom;
	if (!IsRectEmpty(&rect))
		FillRect(hDC, &rect, hBrush);
#ifdef _DEBUG
	GdiFlush();
#endif

	// left
	rect.left = 0;
	rect.right = offsetRect.left;
	rect.bottom = rcClient.bottom;
	if (!IsRectEmpty(&rect))
		FillRect(hDC, &rect, hBrush);
#ifdef _DEBUG
	GdiFlush();
#endif

	// bottom
	rect.left = 0;
	rect.right = rcClient.right;
	rect.top = offsetRect.bottom;
	rect.bottom = rcClient.bottom;
	if (!IsRectEmpty(&rect))
		FillRect(hDC, &rect, hBrush);
#ifdef _DEBUG
	GdiFlush();
#endif

	DeleteObject(hBrush);
}

void CConEmuMain::PaintCon(HDC hPaintDC)
{
    //if (ProgressBars)
    //    ProgressBars->OnTimer();

    // Если "завис" PostUpdate
    if (mp_TabBar->NeedPostUpdate())
    	mp_TabBar->Update();

	RECT rcClient = {0};
	if (ghWndDC) {
		GetClientRect(ghWndDC, &rcClient);
		MapWindowPoints(ghWndDC, ghWnd, (LPPOINT)&rcClient, 2);
	}

	// если mp_VActive==NULL - будет просто выполнена заливка фоном.
    mp_VActive->Paint(hPaintDC, rcClient);

#ifdef _DEBUG
	if ((GetKeyState(VK_SCROLL) & 1) && (GetKeyState(VK_CAPITAL) & 1)) {
		DebugStep(L"ConEmu: Sleeping in PaintCon for 1s");
		Sleep(1000);
		DebugStep(NULL);
	}
#endif
}

void CConEmuMain::RePaint()
{
    gConEmu.mp_TabBar->RePaint();
    m_Back->RePaint();
	HDC hDc = GetDC(ghWnd);
    //mp_VActive->Paint(hDc); // если mp_VActive==NULL - будет просто выполнена заливка фоном.
	PaintGaps(hDc);
	PaintCon(hDc);
	ReleaseDC(ghWnd, hDc);
}

void CConEmuMain::Update(bool isForce /*= false*/)
{
    if (isForce)
    {
        for (int i=0; i<MAX_CONSOLE_COUNT; i++)
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

DWORD CConEmuMain::GetFarPID()
{
    DWORD dwPID = 0;
    if (mp_VActive && mp_VActive->RCon())
        dwPID = mp_VActive->RCon()->GetFarPID();
    return dwPID;
}

LPCTSTR CConEmuMain::GetLastTitle(bool abUseDefault/*=true*/)
{
    if (!Title[0] && abUseDefault)
        return _T("ConEmu");
    return Title;
}

LPCTSTR CConEmuMain::GetVConTitle(int nIdx)
{
    if (nIdx<0 || nIdx>=MAX_CONSOLE_COUNT)
        return NULL;
    if (!mp_VCon[nIdx] || !mp_VCon[nIdx]->RCon())
        return NULL;
    return mp_VCon[nIdx]->RCon()->GetTitle();
}

CVirtualConsole* CConEmuMain::GetVCon(int nIdx)
{
    if (nIdx<0 || nIdx>=MAX_CONSOLE_COUNT)
    {
    	_ASSERTE(nIdx>=0 && nIdx<MAX_CONSOLE_COUNT);
        return NULL;
    }
    return mp_VCon[nIdx];
}

CVirtualConsole* CConEmuMain::GetVConFromPoint(POINT ptScreen)
{
	TODO("Доработать для DoubleView");
	HWND hView = ghWndDC;
	RECT rcView; GetWindowRect(hView, &rcView);
	if (!PtInRect(&rcView, ptScreen))
		return NULL;
	return ActiveCon();
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
    return mp_VActive->RCon()->isFilePanel(abPluginAllowed);
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
		GetUserName(ms_ConEmuAliveEvent+lstrlen(ms_ConEmuAliveEvent), &nSize);
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
			if (gSet.icMinimizeRestore)
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

bool CConEmuMain::isFar()
{
    if (!mp_VActive) return false;
    return mp_VActive->RCon()->isFar();
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
        isMe = (h != NULL) && (h == ghWnd || h == ghOpWnd);
        if (h && !isMe && abRealAlso)
        {
            for (int i=0; i<MAX_CONSOLE_COUNT; i++)
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
			RECT rcClient; GetClientRect(ghWnd, &rcClient);
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

bool CConEmuMain::isNtvdm()
{
    if (!mp_VActive) return false;
    return mp_VActive->RCon()->isNtvdm();
}

bool CConEmuMain::isValid(CRealConsole* apRCon)
{
	if (!apRCon)
		return false;

	for (int i=0; i<MAX_CONSOLE_COUNT; i++)
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

    for (int i=0; i<MAX_CONSOLE_COUNT; i++)
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
        
    TODO("После добавления второго viewport'а потребуется коррекция");
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
    if (gSet.isFullScreen || isZoomed() || isIconic())
        return false;
    return true;
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

    if (mp_VActive && mp_VActive->RCon())
        hPictureView = mp_VActive->RCon()->isPictureView();
    else
        hPictureView = NULL;
    

    lbRc = hPictureView!=NULL;

    // Если вызывали Help (F1) - окошко PictureView прячется
    if (hPictureView && !IsWindowVisible(hPictureView))
    {
        lbRc = false;
        hPictureView = NULL;
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

bool CConEmuMain::isSizing()
{
    // Юзер тащит мышкой рамку окна
    return (mouse.state & MOUSE_SIZING_BEGIN) == MOUSE_SIZING_BEGIN;
}

// Сюда может придти только LOWORD от HKL
void CConEmuMain::SwitchKeyboardLayout(DWORD_PTR dwNewKeybLayout)
{
    if ((gSet.isMonitorConsoleLang & 1) == 0)
    {
	    if (gSet.isAdvLogging > 1)
	    	mp_VActive->RCon()->LogString(L"CConEmuMain::SwitchKeyboardLayout skipped, cause of isMonitorConsoleLang==0");
        return;
    }

	#ifdef _DEBUG
	wchar_t szDbg[128]; wsprintfW(szDbg, L"CConEmuMain::SwitchKeyboardLayout(0x%08I64X)\n", (unsigned __int64)dwNewKeybLayout);
	DEBUGSTRLANG(szDbg);
	#endif

    HKL hKeyb[20]; UINT nCount, i; BOOL lbFound = FALSE;
    nCount = GetKeyboardLayoutList ( countof(hKeyb), hKeyb );
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
		wsprintfW(szDbg, L"CConEmuMain::SwitchKeyboardLayout change to(0x%08I64X)\n", (unsigned __int64)dwNewKeybLayout);
		DEBUGSTRLANG(szDbg);
		#endif

	    if (gSet.isAdvLogging > 1)
	    {
	    	wchar_t szInfo[255];
			wsprintf(szInfo, L"CConEmuMain::SwitchKeyboardLayout, posting WM_INPUTLANGCHANGEREQUEST, WM_INPUTLANGCHANGE for 0x%08X",
				(DWORD)dwNewKeybLayout);
	    	mp_VActive->RCon()->LogString(szInfo);
		}
		
        // Теперь переключаем раскладку
        PostMessage ( ghWnd, WM_INPUTLANGCHANGEREQUEST, 0, (LPARAM)dwNewKeybLayout );
        PostMessage ( ghWnd, WM_INPUTLANGCHANGE, 0, (LPARAM)dwNewKeybLayout );
    }
    else
    {
	    if (gSet.isAdvLogging > 1)
	    {
	    	wchar_t szInfo[255];
			wsprintf(szInfo, L"CConEmuMain::SwitchKeyboardLayout skipped, cause of GetActiveKeyboardLayout()==0x%08X",
				(DWORD)dwNewKeybLayout);
	    	mp_VActive->RCon()->LogString(szInfo);
		}
    }
}

void CConEmuMain::TabCommand(UINT nTabCmd)
{
    if (!isMainThread())
	{
        PostMessage(ghWnd, mn_MsgTabCommand, nTabCmd, 0);
        return;
    }

    switch (nTabCmd)
	{
        case 0:
            {
                if (gConEmu.mp_TabBar->IsShown())
                    gSet.isTabs = 0;
                else
                    gSet.isTabs = 1;
                gConEmu.ForceShowTabs ( gSet.isTabs == 1 );
            } break;
        case 1:
            {
                gConEmu.mp_TabBar->SwitchNext();
            } break;
        case 2:
            {
                gConEmu.mp_TabBar->SwitchPrev();
            } break;
        case 3:
            {
                gConEmu.mp_TabBar->SwitchCommit();
            } break;
    };
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

void CConEmuMain::OnBufferHeight() //BOOL abBufferHeight)
{
	if (!gConEmu.isMainThread())
	{
		PostMessage(ghWnd, mn_MsgPostOnBufferHeight, 0, 0);
		return;
	}

	BOOL lbBufferHeight = mp_VActive->RCon()->isBufferHeight();
    gConEmu.m_Back->TrackMouse(); // спрятать или показать прокрутку, если над ней мышка
    gConEmu.mp_TabBar->OnBufferHeight(lbBufferHeight);
}

TODO("И вообще, похоже это событие не вызывается");
LRESULT CConEmuMain::OnClose(HWND hWnd)
{
	// Если все-таки вызовется - имитировать SC_CLOSE
	OnSysCommand(hWnd, SC_CLOSE, 0);

    //_ASSERTE(FALSE);
    //Icon.Delete(); - перенес в WM_DESTROY
    //mb_InClose = TRUE;
    //if (ghConWnd && IsWindow(ghConWnd)) {
    //    mp_VActive->RCon()->CloseConsole();
    //} else {
    //    Destroy();
    //}
    //mb_InClose = FALSE;
    return 0;
}

BOOL CConEmuMain::OnCloseQuery()
{
	int nEditors = 0, nProgress = 0, i;
	for (i=(MAX_CONSOLE_COUNT-1); i>=0; i--)
	{
		CRealConsole* pRCon = NULL;
		ConEmuTab tab = {0};
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
		lstrcpy(szText, L"Close confirmation.\r\n\r\n"); pszText = szText+lstrlen(szText);
		if (nProgress) { wsprintf(pszText, L"Incomplete operations: %i\r\n", nProgress); pszText += lstrlen(pszText); }
		if (nEditors) { wsprintf(pszText, L"Unsaved editor windows: %i\r\n", nEditors); pszText += lstrlen(pszText); }
		lstrcpy(pszText, L"\r\nProceed with shutdown?");
		int nBtn = MessageBoxW(ghWnd, szText, L"ConEmu", MB_OKCANCEL|MB_ICONEXCLAMATION);
		if (nBtn != IDOK)
			return FALSE; // не закрывать
	}
	
	return TRUE; // можно
}

//// Вызывается из ConEmuC, когда он запускается в режиме ComSpec
//LRESULT CConEmuMain::OnConEmuCmd(BOOL abStarted, HWND ahConWnd, DWORD anConEmuC_PID)
//{
//  for (int i=0; i<MAX_CONSOLE_COUNT; i++) {
//      if (mp_VCon[i] == NULL || mp_VCon[i]->RCon() == NULL) continue;
//      if (mp_VCon[i]->RCon()->hConWnd != ahConWnd) continue;
//
//      return mp_VCon[i]->RCon()->OnConEmuCmd(abStarted, anConEmuC_PID);
//  }
//
//  return 0;
//}

HMENU CConEmuMain::CreateDebugMenuPopup()
{
    HMENU hDebug = CreatePopupMenu();
    AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_CON_TOGGLE_VISIBLE, _T("&Real console"));
    AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_CONPROP, _T("&Properties..."));
    AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_DUMPCONSOLE, _T("&Dump screen..."));
    AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_DEBUGGUI, _T("Debug &log (GUI)"));
    #ifdef _DEBUG
    AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_MONITOR_SHELLACTIVITY, _T("Enable &shell log..."));
    #endif
    AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_DEBUG_SHOWRECTS, _T("Show debug rec&ts"));
    return hDebug;
}

void CConEmuMain::PopulateEditMenuPopup(HMENU hMenu)
{
    InsertMenu(hMenu, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_CON_PASTE, _T("&Paste"));
    InsertMenu(hMenu, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_CON_COPY, _T("Cop&y"));
	InsertMenu(hMenu, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_CON_MARKTEXT, _T("Mar&k text"));
    InsertMenu(hMenu, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_CON_MARKBLOCK, _T("Mark &block"));
}

LRESULT CConEmuMain::OnCreate(HWND hWnd, LPCREATESTRUCT lpCreate)
{
    ghWnd = hWnd; // ставим сразу, чтобы функции могли пользоваться

	//DWORD_PTR dwStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
	//if (gSet.isHideCaptionAlways) {
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


    Icon.LoadIcon(hWnd, gSet.nIconID/*IDI_ICON1*/);
    
    
    // Позволяет реагировать на запросы FlashWindow из фара и запуск приложений
    HMODULE hUser32 = GetModuleHandle(L"user32.dll");
    FRegisterShellHookWindow fnRegisterShellHookWindow = NULL;
    if (hUser32) fnRegisterShellHookWindow = (FRegisterShellHookWindow)GetProcAddress(hUser32, "RegisterShellHookWindow");
    if (fnRegisterShellHookWindow) fnRegisterShellHookWindow ( hWnd );
    
    
    // Чтобы можно было найти хэндл окна по хэндлу консоли
    SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)ghConWnd); // 31.03.2009 Maximus - только нихрена оно еще не создано!

    m_Back->Create();

    if (!m_Child->Create())
        return -1;

    mn_StartTick = GetTickCount();


    //if (gSet.isGUIpb && !ProgressBars) {
    //	ProgressBars = new CProgressBars(ghWnd, g_hInstance);
    //}

    // Установить переменную среды с дескриптором окна
    SetConEmuEnvVar(ghWndDC);


    HMENU hwndMain = GetSystemMenu(ghWnd, FALSE), hDebug = NULL;
    InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_TOTRAY, TRAY_ITEM_HIDE_NAME/* L"Hide to &TSA" */);
    InsertMenu(hwndMain, 0, MF_BYPOSITION, MF_SEPARATOR, 0);
    InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_ABOUT, _T("&About"));
    if (ms_ConEmuChm[0]) //Показывать пункт только если есть conemu.chm
    	InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_HELP, _T("&Help"));
    InsertMenu(hwndMain, 0, MF_BYPOSITION, MF_SEPARATOR, 0);
    hDebug = CreateDebugMenuPopup();
    InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_POPUP | MF_ENABLED, (UINT_PTR)hDebug, _T("&Debug"));
    PopulateEditMenuPopup(hwndMain);
    InsertMenu(hwndMain, 0, MF_BYPOSITION, MF_SEPARATOR, 0);
    InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED | (gSet.isAlwaysOnTop ? MF_CHECKED : 0),
        ID_ALWAYSONTOP, _T("Al&ways on top"));
    InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED | (gSet.AutoScroll ? MF_CHECKED : 0),
        ID_AUTOSCROLL, _T("Auto scro&ll"));
    InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_SETTINGS, _T("S&ettings..."));

	#ifdef _DEBUG
	DWORD_PTR dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE);
	#endif

    if (gSet.isTabs==1) // "Табы всегда"
        ForceShowTabs(TRUE); // Показать табы

    //CreateCon();
    
    // Запустить серверную нить
    mh_GuiServerThreadTerminate = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (mh_GuiServerThreadTerminate) ResetEvent(mh_GuiServerThreadTerminate);
    mh_GuiServerThread = CreateThread(NULL, 0, GuiServerThread, (LPVOID)this, 0, &mn_GuiServerThreadId);
    
    return 0;
}

void CConEmuMain::PostCreate(BOOL abRecieved/*=FALSE*/)
{
    if (!abRecieved)
	{
        //if (gConEmu.WindowMode == rFullScreen || gConEmu.WindowMode == rMaximized) {
        #ifdef MSGLOGGER
        WINDOWPLACEMENT wpl; memset(&wpl, 0, sizeof(wpl)); wpl.length = sizeof(wpl);
        GetWindowPlacement(ghWnd, &wpl);
        #endif

		if (gSet.isHideCaptionAlways())
		{
			OnHideCaption();
		}

		if (gSet.isDesktopMode)
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
		
		if (gSet.icMinimizeRestore)
			RegisterMinRestore(true);
	
		//SetWindowRgn(ghWnd, CreateWindowRgn(), TRUE);

		if (gSet.szFontError[0])
		{
			MBoxA(gSet.szFontError);
			gSet.szFontError[0] = 0;
		}

		if (!gSet.CheckConsoleFontFast())
		{
			gbDontEnable = TRUE;
			gSet.EditConsoleFont(ghWnd);
			gbDontEnable = FALSE;
		}

		if (gSet.isShowBgImage)
		    gSet.LoadBackgroundFile(gSet.sBgImage);

        if (mp_VActive == NULL || !gConEmu.mb_StartDetached) // Консоль уже может быть создана, если пришел Attach из ConEmuC
		{
        	BOOL lbCreated = FALSE;
        	LPCWSTR pszCmd = gSet.GetCmd();
        	if (*pszCmd == L'@' && !gConEmu.mb_StartDetached)
			{
        		// В качестве "команды" указан "пакетный файл" одновременного запуска нескольких консолей
        		HANDLE hFile = CreateFile(pszCmd+1, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
        		if (!hFile)
				{
        			DWORD dwErr = GetLastError();
        			wchar_t* pszErrMsg = (wchar_t*)calloc(lstrlen(pszCmd)+100,2);
        			lstrcpy(pszErrMsg, L"Can't open console batch file:\n\xAB"/*«*/); lstrcat(pszErrMsg, pszCmd+1); lstrcat(pszErrMsg, L"\xBB"/*»*/);
	                DisplayLastError(pszErrMsg, dwErr);
	                free(pszErrMsg);
	                Destroy();
	                return;
        		}
        		DWORD nSize = GetFileSize(hFile, NULL);
        		if (!nSize || nSize > (1<<20))
				{
        			DWORD dwErr = GetLastError();
        			CloseHandle(hFile);
        			wchar_t* pszErrMsg = (wchar_t*)calloc(lstrlen(pszCmd)+100,2);
        			lstrcpy(pszErrMsg, L"Console batch file is too large or empty:\n\xAB"/*«*/); lstrcat(pszErrMsg, pszCmd+1); lstrcat(pszErrMsg, L"\xBB"/*»*/);
	                DisplayLastError(pszErrMsg, dwErr);
	                free(pszErrMsg);
	                Destroy();
	                return;
        		}
        		char* pszDataA = (char*)calloc(nSize+4,1);
        		_ASSERTE(pszDataA);
        		DWORD nRead = 0;
        		BOOL lbRead = ReadFile(hFile, pszDataA, nSize, &nRead, 0);
        		DWORD dwErr = GetLastError();
        		CloseHandle(hFile);
        		if (!lbRead || nRead != nSize)
				{
        			free(pszDataA);
        			wchar_t* pszErrMsg = (wchar_t*)calloc(lstrlen(pszCmd)+100,2);
        			lstrcpy(pszErrMsg, L"Reading console batch file failed:\n\xAB"/*«*/); lstrcat(pszErrMsg, pszCmd+1); lstrcat(pszErrMsg, L"\xBB"/*»*/);
	                DisplayLastError(pszErrMsg, dwErr);
	                free(pszErrMsg);
	                Destroy();
	                return;
        		}
        		// Опредлить код.страницу файла
        		wchar_t* pszDataW = NULL; BOOL lbNeedFreeW = FALSE;
        		if (pszDataA[0] == 0xEF && pszDataA[1] == 0xBB && pszDataA[2] == 0xBF)
				{
        			// UTF-8 BOM
        			pszDataW = (wchar_t*)calloc(nSize+2,2); lbNeedFreeW = TRUE;
        			_ASSERTE(pszDataW);
        			MultiByteToWideChar(CP_UTF8, 0, pszDataA+3, -1, pszDataW, nSize);
        		}
				else if (pszDataA[0] == 0xFF && pszDataA[1] == 0xFE)
				{
        			// CP-1200 BOM
        			pszDataW = (wchar_t*)(pszDataA+2);
        		}
				else
				{
        			// Plain ANSI
        			pszDataW = (wchar_t*)calloc(nSize+2,2); lbNeedFreeW = TRUE;
        			_ASSERTE(pszDataW);
        			MultiByteToWideChar(CP_ACP, 0, pszDataA, -1, pszDataW, nSize+1);
        		}
        		// Поехали
        		wchar_t *pszLine = pszDataW;
        		wchar_t *pszNewLine = wcschr(pszLine, L'\r');
        		CVirtualConsole *pSetActive = NULL, *pCon = NULL;
        		BOOL lbSetActive = FALSE, lbOneCreated = FALSE, lbRunAdmin = FALSE;
        		while (*pszLine)
				{
        			lbSetActive = lbRunAdmin = FALSE;
        			while (*pszLine == L'>' || *pszLine == L'*' || *pszLine == L' ' || *pszLine == L'\t')
					{
        				if (*pszLine == L'>') lbSetActive = TRUE;
        				if (*pszLine == L'*') lbRunAdmin = TRUE;
	        			pszLine++;
    	    		}
        			
        			if (pszNewLine) *pszNewLine = 0;
        			while (*pszLine == L' ') pszLine++;
        			
        			if (*pszLine)
					{
        				while (pszLine[0] == L'/')
        				{
        					if (CSTR_EQUAL == CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE|SORT_STRINGSORT,
        							pszLine, 14, L"/bufferheight ", 14))
							{
								pszLine += 14;
								while (*pszLine == L' ') pszLine++;
								wchar_t* pszEnd = NULL;
								long lBufHeight = wcstol(pszLine, &pszEnd, 10);
								gSet.SetArgBufferHeight ( lBufHeight );
								if (pszEnd) pszLine = pszEnd;
							}

							TODO("Когда появился ключ /mouse - добавить сюда обработку");
							
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
							args.pszSpecialCmd = _wcsdup(pszLine);
							args.bRunAsAdministrator = lbRunAdmin;

							pCon = CreateCon(&args);

				            if (!pCon)
							{
				                DisplayLastError(L"Can't create new virtual console!");
				                if (!lbOneCreated) {
					                Destroy();
					                return;
				                }
				            }
							else
							{
				            	lbOneCreated = TRUE;
				            	if (lbSetActive && !pSetActive)
				            		pSetActive = pCon;
				            		
				            	if (GetVCon(MAX_CONSOLE_COUNT-1))
				            		break; // Больше создать не получится
		            		}
		            	}

						MSG Msg;
						while (PeekMessage(&Msg,0,0,0,PM_REMOVE))
						{
							if (Msg.message == WM_QUIT)
								return;
							BOOL lbDlgMsg = FALSE;
							if (ghOpWnd)
							{
								if (IsWindow(ghOpWnd))
									lbDlgMsg = IsDialogMessage(ghOpWnd, &Msg);
							}
							if (!lbDlgMsg)
							{
								TranslateMessage(&Msg);
								DispatchMessage(&Msg);
							}
						}
						if (!ghWnd || !IsWindow(ghWnd))
							return;
	            	}
	            	
					if (!pszNewLine) break;
	            	pszLine = pszNewLine+1;
	            	if (!*pszLine) break;
	            	if (*pszLine == L'\n') pszLine++;
	            	pszNewLine = wcschr(pszLine, L'\r');
        		}
        		if (pSetActive)
				{
        			Activate(pSetActive);
        		}
        		if (pszDataW && lbNeedFreeW) free(pszDataW); pszDataW = NULL;
        		if (pszDataA) free(pszDataA); pszDataA = NULL;

				// Если ConEmu был запущен с ключом "/single /cmd xxx" то после окончания
				// загрузки - сбросить команду, которая пришла из "/cmd" - загрузить настройку
				if (gSet.SingleInstanceArg)
				{
        			gSet.ResetCmdArg();
				}

				lbCreated = TRUE;
        	}
        	
        	if (!lbCreated)
        	{
				RConStartArgs args; args.bDetached = gConEmu.mb_StartDetached;
	            if (!CreateCon(&args))
				{
	                DisplayLastError(L"Can't create new virtual console!");
	                Destroy();
	                return;
	            }
            }
        }
        if (gConEmu.mb_StartDetached) gConEmu.mb_StartDetached = FALSE; // действует только на первую консоль
        

		// Может быть в настройке указано - всегда показывать иконку в TSA
		Icon.SettingsChanged();


        HRESULT hr = S_OK;
        hr = OleInitialize (NULL); // как бы попробовать включать Ole только во время драга. кажется что из-за него глючит переключалка языка
        //CoInitializeEx(NULL, COINIT_MULTITHREADED);

         
		if (!mp_TaskBar2)
		{
			// В PostCreate это выполняется дольше всего. По идее мешать не должно,
			// т.к. серверная нить уже запущена.
			hr = CoCreateInstance(CLSID_TaskbarList,NULL,CLSCTX_INPROC_SERVER,IID_ITaskbarList2,(void**)&mp_TaskBar2);
			if (hr == S_OK && mp_TaskBar2)
			{
				hr = mp_TaskBar2->HrInit();
			}
			if (hr != S_OK && mp_TaskBar2)
			{
				if (mp_TaskBar2) mp_TaskBar2->Release();
				mp_TaskBar2 = NULL;
			}
		}
        if (!mp_TaskBar3 && mp_TaskBar2)
		{
			hr = mp_TaskBar2->QueryInterface(IID_ITaskbarList3, (void**)&mp_TaskBar3);
        }

        if (!mp_DragDrop)
		{
        	// было ghWndDC. Попробуем на главное окно, было бы удобно 
        	// "бросать" на таб (с автоматической активацией консоли)
            mp_DragDrop = new CDragDrop();
            if (!mp_DragDrop->Register())
			{
            	CDragDrop *p = mp_DragDrop; mp_DragDrop = NULL;
            	delete p;
            }
        }
        //TODO terst
        WARNING("Если консоль не создана - handler не установится!")

        //SetConsoleCtrlHandler((PHANDLER_ROUTINE)CConEmuMain::HandlerRoutine, true);

        apiSetForegroundWindow(ghWnd);
        
        RegisterHotKeys();

        //SetParent(ghWnd, GetParent(GetShellWindow()));

        UINT n = SetTimer(ghWnd, TIMER_MAIN_ID, 500/*gSet.nMainTimerElapse*/, NULL);
        #ifdef _DEBUG
        DWORD dw = GetLastError();
        #endif
        n = 0;
        n = SetTimer(ghWnd, TIMER_CONREDRAW_ID, CON_REDRAW_TIMOUT*2, NULL);
    }
}

LRESULT CConEmuMain::OnDestroy(HWND hWnd)
{
	gSet.SaveSizePosOnExit();
	
	// Делать обязательно перед ResetEvent(mh_ConEmuAliveEvent), чтобы у другого
	// экземпляра не возникло проблем с регистрацией hotkey
	RegisterMinRestore(false);

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

    if (mh_GuiServerThread)
    {
        SetEvent(mh_GuiServerThreadTerminate);
        
        wchar_t szServerPipe[MAX_PATH];
        _ASSERTE(ghWnd!=NULL);
        wsprintf(szServerPipe, CEGUIPIPENAME, L".", (DWORD)ghWnd);
        
        HANDLE hPipe = CreateFile(szServerPipe,GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
        if (hPipe == INVALID_HANDLE_VALUE)
        {
            DEBUGSTR(L"All pipe instances closed?\n");
        }
        else
        {
            DEBUGSTR(L"Waiting server pipe thread\n");
            #ifdef _DEBUG
            DWORD dwWait = 
            #endif
            WaitForSingleObject(mh_GuiServerThread, 200); // пытаемся дождаться, пока нить завершится
            // Просто закроем пайп - его нужно было передернуть
            CloseHandle(hPipe);
            hPipe = INVALID_HANDLE_VALUE;
        }
        // Если нить еще не завершилась - прибить
        if (WaitForSingleObject(mh_GuiServerThread,0) != WAIT_OBJECT_0)
        {
            DEBUGSTR(L"### Terminating mh_ServerThread\n");
            TerminateThread(mh_GuiServerThread,0);
        }
        SafeCloseHandle(mh_GuiServerThread);
        SafeCloseHandle(mh_GuiServerThreadTerminate);
    }

    for (int i=0; i<MAX_CONSOLE_COUNT; i++)
    {
        if (mp_VCon[i])
        {
            delete mp_VCon[i]; mp_VCon[i] = NULL;
        }
    }


    if (mh_WinHook)
    {
        UnhookWinEvent(mh_WinHook);
        mh_WinHook = NULL;
    }
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
    
	if (mp_TaskBar3)
	{
		mp_TaskBar3->Release();
		mp_TaskBar3 = NULL;
	}
	if (mp_TaskBar2)
	{
		mp_TaskBar2->Release();
		mp_TaskBar2 = NULL;
	}
    
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
    if (gSet.isDisableFarFlashing && mp_VActive->RCon()->GetFarPID(FALSE)) {
    	if (gSet.isDisableFarFlashing == 1)
    		return 0;
    	else
    		lbFlashSimple = true;
	} else
    if (gSet.isDisableAllFlashing) {
    	if (gSet.isDisableAllFlashing == 1)
    		return 0;
    	else
    		lbFlashSimple = true;
	}

        
    BOOL lbRc = FALSE;
    for (int i = 0; i<MAX_CONSOLE_COUNT; i++) {
        if (!mp_VCon[i]) continue;
        
        if (mp_VCon[i]->RCon()->ConWnd() == hCon) {
            FLASHWINFO fl = {sizeof(FLASHWINFO)};
            
            if (isMeForeground()) {
            	if (mp_VCon[i] != mp_VActive) { // Только для неактивной консоли
                    fl.dwFlags = FLASHW_STOP; fl.hwnd = ghWnd;
                    FlashWindowEx(&fl); // Чтобы мигание не накапливалось
                    
            		fl.uCount = 3; fl.dwFlags = lbFlashSimple ? FLASHW_ALL : FLASHW_TRAY; fl.hwnd = ghWnd;
            		FlashWindowEx(&fl);
            	}
            } else {
            	if (lbFlashSimple) {
            		fl.uCount = 3; fl.dwFlags = FLASHW_TRAY;
            	} else {
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

LRESULT CConEmuMain::OnFocus(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
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
		wsprintf(szDbg, L"WM_SETFOCUS(From=0x%08X)\n", (DWORD)wParam);
		DEBUGSTRFOCUS(szDbg);
		#endif
		pszMsgName = L"WM_SETFOCUS";
	}
	else if (messg == WM_ACTIVATE)
	{
        lbSetFocus = (LOWORD(wParam)==WA_ACTIVE) || (LOWORD(wParam)==WA_CLICKACTIVE);
		if (!lbSetFocus && gSet.isDesktopMode && mh_ShellWindow)
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
		#ifdef _DEBUG
		if (lbSetFocus)
			wsprintf(szDbg, L"WM_ACTIVATE(From=0x%08X)\n", (DWORD)lParam);
		else
			wsprintf(szDbg, L"WM_ACTIVATE.WA_INACTIVE(To=0x%08X)\n", (DWORD)lParam);
		DEBUGSTRFOCUS(szDbg);
		#endif
		pszMsgName = L"WM_ACTIVATE";
	}
	else if (messg == WM_ACTIVATEAPP)
	{
        lbSetFocus = (wParam!=0);
		#ifdef _DEBUG
		if (lbSetFocus)
			wsprintf(szDbg, L"WM_ACTIVATEAPP.Activate(FromTID=%i)\n", (DWORD)lParam);
		else
			wsprintf(szDbg, L"WM_ACTIVATEAPP.Deactivate(ToTID=%i)\n", (DWORD)lParam);
		DEBUGSTRFOCUS(szDbg);
		#endif
		pszMsgName = L"WM_ACTIVATEAPP";
	}
	else if (messg == WM_KILLFOCUS)
	{
		#ifdef _DEBUG
		wsprintf(szDbg, L"WM_KILLFOCUS(To=0x%08X)\n", (DWORD)wParam);
		DEBUGSTRFOCUS(szDbg);
		#endif
		pszMsgName = L"WM_KILLFOCUS";
		hNewFocus = (HWND)wParam;
	}

	CheckFocus(pszMsgName);

	// Если фокус "забрало" какое-либо дочернее окно в ConEmu (VideoRenderer - 'ActiveMovie Window')
	if (hNewFocus && hNewFocus != ghWnd)
	{
		HWND hParent = hNewFocus;
		while ((hParent = GetParent(hNewFocus)) != NULL)
		{
			if (hParent == ghWnd)
			{
				DWORD dwStyle = GetWindowLong(hNewFocus, GWL_STYLE);
				if ((dwStyle & (WS_POPUP|WS_OVERLAPPEDWINDOW|WS_DLGFRAME)) != 0)
					break; // Это диалог, не трогаем

				SetFocus(ghWnd);
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


    if (!lbSetFocus)
	{
        gConEmu.mp_TabBar->SwitchRollback();
        
        UnRegisterHotKeys();
    }

	if (mp_VActive && mp_VActive->RCon())
		mp_VActive->RCon()->OnGuiFocused(lbSetFocus);
    
    if (gSet.isFadeInactive && mp_VActive)
	{
    	bool bForeground = lbSetFocus || isMeForeground();
		bool bLastFade = (mp_VActive!=NULL) ? mp_VActive->mb_LastFadeFlag : false;
		bool bNewFade = (gSet.isFadeInactive && !bForeground && !isPictureView());
		if (bLastFade != bNewFade)
		{
			if (mp_VActive) mp_VActive->mb_LastFadeFlag = bNewFade;
			m_Child->Invalidate();
		}
	}
    
    if (gSet.isSkipFocusEvents)
        return 0;
        
#ifdef MSGLOGGER
    /*if (messg == WM_ACTIVATE && wParam == WA_INACTIVE) {
        WCHAR szMsg[128]; wsprintf(szMsg, L"--Deactivating to 0x%08X\n", lParam);
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

    if (mp_VActive /*&& (messg == WM_SETFOCUS || messg == WM_KILLFOCUS)*/) {
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
	wsprintf(szMinMax, L"OnGetMinMaxInfo[before] MaxSize={%i,%i}, MaxPos={%i,%i}, MinTrack={%i,%i}, MaxTrack={%i,%i}\n",
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

    //pInfo->ptMinTrackSize.x = srctWindow.X * (gSet.Log Font.lfWidth ? gSet.Log Font.lfWidth : 4)
    //  + p.x + shiftRect.left + shiftRect.right;

    //pInfo->ptMinTrackSize.y = srctWindow.Y * (gSet.Log Font.lfHeight ? gSet.Log Font.lfHeight : 6)
    //  + p.y + shiftRect.top + shiftRect.bottom;

    if (gSet.isFullScreen) {
		if (pInfo->ptMaxTrackSize.x < ptFullScreenSize.x)
			pInfo->ptMaxTrackSize.x = ptFullScreenSize.x;
		if (pInfo->ptMaxTrackSize.y < ptFullScreenSize.y)
			pInfo->ptMaxTrackSize.y = ptFullScreenSize.y;
		if (pInfo->ptMaxSize.x < ptFullScreenSize.x)
			pInfo->ptMaxSize.x = ptFullScreenSize.x;
		if (pInfo->ptMaxSize.y < ptFullScreenSize.y)
			pInfo->ptMaxSize.y = ptFullScreenSize.y;
    }
    
    if (gSet.isHideCaption && !gSet.isHideCaptionAlways()) {
    	pInfo->ptMaxPosition.y -= GetSystemMetrics(SM_CYCAPTION);
    	//pInfo->ptMaxSize.y += GetSystemMetrics(SM_CYCAPTION);
    }

	#ifdef _DEBUG
	wsprintf(szMinMax, L"OnGetMinMaxInfo[after]  MaxSize={%i,%i}, MaxPos={%i,%i}, MinTrack={%i,%i}, MaxTrack={%i,%i}\n",
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

	if (isZoomed()) {
		SetWindowMode(rMaximized, TRUE);
	}

	UpdateWindowRgn();

	//DWORD_PTR dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE);
	//if (gSet.isHideCaptionAlways)
	//	dwStyle &= ~(WS_CAPTION/*|WS_THICKFRAME*/);
	//else
	//	dwStyle |= (WS_CAPTION|/*WS_THICKFRAME|*/WS_MINIMIZEBOX|WS_MAXIMIZEBOX);
	//mb_SkipSyncSize = TRUE;
	//SetWindowLongPtr(ghWnd, GWL_STYLE, dwStyle);
	//OnSize(-1);
	//mp_TabBar->OnCaptionHidden();
	//mb_SkipSyncSize = FALSE;
	//if (!gSet.isFullScreen && !isZoomed() && !isIconic()) {
	//	RECT rcCon = MakeRect(gSet.wndWidth,gSet.wndHeight);
	//	RECT rcWnd = CalcRect(CER_MAIN, rcCon, CER_CONSOLE); // размеры окна
	//	RECT wndR; GetWindowRect(ghWnd, &wndR); // текущий XY
	//	if (gSet.isAdvLogging) {
	//		char szInfo[128]; wsprintfA(szInfo, "OnHideCaption(Cols=%i, Rows=%i)", gSet.wndWidth,gSet.wndHeight);
	//		mp_VActive->RCon()->LogString(szInfo);
	//	}
	//	MOVEWINDOW ( ghWnd, wndR.left, wndR.top, rcWnd.right, rcWnd.bottom, 1);

	//	GetWindowRect(ghWnd, &mrc_Ideal);
	//}
}

void CConEmuMain::OnPanelViewSettingsChanged(BOOL abSendChanges/*=TRUE*/)
{
	// Заполнить цвета gSet.ThSet.crPalette[16], gSet.ThSet.crFadePalette[16]
	COLORREF *pcrNormal = gSet.GetColors(FALSE);
	COLORREF *pcrFade = gSet.GetColors(TRUE);
	for (int i=0; i<16; i++)
	{
		// через FOR чтобы с BitMask не наколоться
		gSet.ThSet.crPalette[i] = (pcrNormal[i]) & 0xFFFFFF;
		gSet.ThSet.crFadePalette[i] = (pcrFade[i]) & 0xFFFFFF;
	}

	gSet.ThSet.nFontQuality = gSet.FontQuality();

	// Параметры основного шрифта ConEmu
	lstrcpy(gSet.ThSet.MainFont.sFontName, gSet.FontFaceName());
	gSet.ThSet.MainFont.nFontHeight = gSet.FontHeight();
	gSet.ThSet.MainFont.nFontWidth = gSet.FontWidth();
	gSet.ThSet.MainFont.nFontCellWidth = gSet.FontSizeX3 ? gSet.FontSizeX3 : gSet.FontWidth();
	gSet.ThSet.MainFont.nFontQuality = gSet.FontQuality();
	gSet.ThSet.MainFont.nFontCharSet = gSet.FontCharSet();
	gSet.ThSet.MainFont.Bold = gSet.FontBold();
	gSet.ThSet.MainFont.Italic = gSet.FontItalic();
	lstrcpy(gSet.ThSet.MainFont.sBorderFontName, gSet.BorderFontFaceName());
    gSet.ThSet.MainFont.nBorderFontWidth = gSet.BorderFontWidth();

	// Применить в мэппинг
	bool lbChanged = gSet.m_ThSetMap.SetFrom(&gSet.ThSet);

	if (abSendChanges && lbChanged)
	{
		// и отослать заинтересованным
		for (int i=0; i<MAX_CONSOLE_COUNT; i++)
		{
			if (mp_VCon[i]) {
				mp_VCon[i]->OnPanelViewSettingsChanged();
			}
		}
	}
}

void CConEmuMain::OnAltEnter()
{
	if (!gSet.isFullScreen)
		gConEmu.SetWindowMode(rFullScreen);
	else if (gSet.isDesktopMode && (gSet.isFullScreen || gConEmu.isZoomed()))
		gConEmu.SetWindowMode(rNormal);
	else
		gConEmu.SetWindowMode(gConEmu.isWndNotFSMaximized ? rMaximized : rNormal);
}

void CConEmuMain::OnAltF9(BOOL abPosted/*=FALSE*/)
{
	if (!abPosted) {
		PostMessage(ghWnd, gConEmu.mn_MsgPostAltF9, 0, 0);
		return;
	}

	gConEmu.SetWindowMode((gConEmu.isZoomed()||(gSet.isFullScreen/*&&gConEmu.isWndNotFSMaximized*/)) ? rNormal : rMaximized);
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
			PostMessage(ghWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
		else
			PostMessage(ghWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
	}
}

LRESULT CConEmuMain::OnKeyboard(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	WORD bVK = (WORD)(wParam & 0xFF);

#ifdef _DEBUG
	wchar_t szDebug[255];
	wsprintf(szDebug, L"%s(VK=0x%02X, Scan=%i, lParam=0x%08X)\n",
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
		while (nTranslatedChars < 15 // извлечь из буфера все последующие WM_CHAR & WM_SYSCHAR
			&& PeekMessage(&msg, 0,0,0, PM_NOREMOVE)
			)
		{
			if (!(msg.message == WM_CHAR || msg.message == WM_SYSCHAR || msg.message == WM_DEADCHAR)
				|| (msg.lParam & 0xFF0000) != (lParam & 0xFF0000) /* совпадение скан-кода */)
				break;

			if (GetMessage(&msg1, 0,0,0)) // убрать из буфера
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
	if (!GetKeyboardState(m_KeybStates)) {
		#ifdef _DEBUG
		DWORD dwErr = GetLastError();
		_ASSERTE(FALSE);
		#endif
		static bool sbErrShown = false;
		if (!sbErrShown) {
			sbErrShown = true;
			DisplayLastError(L"GetKeyboardState failed!");
		}
	} else {
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

	if (!GetKeyboardState(m_KeybStates)) {
		#ifdef _DEBUG
		DWORD dwErr = GetLastError();
		_ASSERTE(FALSE);
		#endif
		static bool sbErrShown = false;
		if (!sbErrShown) {
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
		if (nTranslatedChars >= 0) {
			// 2 or more
			// Two or more characters were written to the buffer specified by pwszBuff.
			// The most common cause for this is that a dead-key character (accent or diacritic)
			// stored in the keyboard layout could not be combined with the specified virtual key
			// to form a single character. However, the buffer may contain more characters than the
			// return value specifies. When this happens, any extra characters are invalid and should be ignored. 
			szTranslatedChars[min(15,nTranslatedChars)] = 0;
		} else if (nTranslatedChars == -1) {
			// The specified virtual key is a dead-key character (accent or diacritic).
			// This value is returned regardless of the keyboard layout, even if several
			// characters have been typed and are stored in the keyboard state. If possible,
			// even with Unicode keyboard layouts, the function has written a spacing version
			// of the dead-key character to the buffer specified by pwszBuff. For example, the
			// function writes the character SPACING ACUTE (0x00B4), 
			// rather than the character NON_SPACING ACUTE (0x0301).
			szTranslatedChars[0] = 0;
		} else {
			// Invalid
			szTranslatedChars[0] = 0;
		}

		mn_LastPressedVK = bVK;
	}
#endif






  
    if (messg == WM_KEYDOWN && !mb_HotKeyRegistered)
    	RegisterHotKeys(); // CtrlWinAltSpace

    if (messg == WM_KEYDOWN || messg == WM_KEYUP)
    {
        if (wParam == VK_PAUSE && !isPressed(VK_CONTROL))
        {
            if (isPictureView() && !IsWindowUnicode(hPictureView))
            {
                if (messg == WM_KEYUP)
                {
                    bPicViewSlideShow = !bPicViewSlideShow;
                    if (bPicViewSlideShow)
                    {
                        if (gSet.nSlideShowElapse<=500) gSet.nSlideShowElapse=500;
                        dwLastSlideShowTick = GetTickCount() - gSet.nSlideShowElapse;
                    }
                }
                return 0;
            }
        }
        else if (bPicViewSlideShow)
        {
            //KillTimer(hWnd, 3);
            if (wParam==0xbd/* -_ */ || wParam==0xbb/* =+ */)
            {
                if (messg == WM_KEYDOWN)
                {
                    if (wParam==0xbb)
                    {
                        gSet.nSlideShowElapse = 1.2 * gSet.nSlideShowElapse;
                    }
                    else
                    {
                        gSet.nSlideShowElapse = gSet.nSlideShowElapse / 1.2;
                        if (gSet.nSlideShowElapse<=500) gSet.nSlideShowElapse=500;
                    }
                }
                return 0;
            }
            else
            {
                bPicViewSlideShow = false; // отмена слайдшоу
            }
        }
    }

    // Прокрутка в "буферном" режиме
    if (gConEmu.mp_VActive->RCon()->isBufferHeight() && !gConEmu.mp_VActive->RCon()->isFarBufferSupported()
		&& (messg == WM_KEYDOWN || messg == WM_KEYUP) &&
        (wParam == VK_DOWN || wParam == VK_UP || wParam == VK_NEXT || wParam == VK_PRIOR) &&
        isPressed(VK_CONTROL)
       )
    {
        if (messg != WM_KEYDOWN || !mp_VActive)
            return 0;
            
        switch(wParam)
        {
        case VK_DOWN:
            return mp_VActive->RCon()->OnScroll(SB_LINEDOWN);
        case VK_UP:
            return mp_VActive->RCon()->OnScroll(SB_LINEUP);
        case VK_NEXT:
            return mp_VActive->RCon()->OnScroll(SB_PAGEDOWN);
        case VK_PRIOR:
            return mp_VActive->RCon()->OnScroll(SB_PAGEUP);
        }
        return 0;
    }
    
    //CtrlWinAltSpace
    WARNING("В висте, блин, не приходим сообщение на первое нажатие Space. Только на второе???");
    //TODO: Переделать на HotKey
    if (messg == WM_KEYDOWN && wParam == VK_SPACE && isPressed(VK_CONTROL) && isPressed(VK_LWIN) && isPressed(VK_MENU))
    {
    	CtrlWinAltSpace();
        
        return 0;
    }
    
    // Tabs
    if (/*gSet.isTabs &&*/ gSet.isTabSelf && /*gConEmu.mp_TabBar->IsShown() &&*/
        (
         ((messg == WM_KEYDOWN || messg == WM_KEYUP) 
           && (wParam == VK_TAB 
               || (gConEmu.mp_TabBar->IsInSwitch() 
                   && (wParam == VK_UP || wParam == VK_DOWN || wParam == VK_LEFT || wParam == VK_RIGHT))))
         //|| (messg == WM_CHAR && wParam == VK_TAB)
        ))
    {
		//2010-01-19 почему-то был закоментарен !isPressed(VK_MENU), в итоге в FAR не проваливалось кнопкосочетание для детача
        if (isPressed(VK_CONTROL) && !isPressed(VK_MENU) && !isPressed(VK_LWIN) && !isPressed(VK_RWIN))
        {
            if (gConEmu.mp_TabBar->OnKeyboard(messg, wParam, lParam))
                return 0;
        }
    }
    // !!! Запрос на переключение мог быть инициирован из плагина
    if (messg == WM_KEYUP && (wParam == VK_CONTROL || wParam == VK_LCONTROL || wParam == VK_RCONTROL))
	{/*&& gConEmu.mp_TabBar->IsShown()*/
		if (gSet.isTabSelf || (gSet.isTabLazy && gConEmu.mp_TabBar->IsInSwitch()))
		{
			gConEmu.mp_TabBar->SwitchCommit(); // Если переключения не было - ничего не делает
			// В фар отпускание кнопки таки пропустим
		}
    }

    // MultiConsole
    static bool sb_SkipMulticonChar = false;
	static DWORD sn_SkipMulticonVk[2] = {0,0};
	static bool sb_SkipSingleHostkey = false;
    static UINT sm_SkipSingleHostkey; static WPARAM sw_SkipSingleHostkey; static LPARAM sl_SkipSingleHostkey;
    //bool lbLWin = false, lbRWin = false;
    TODO("gSet.nMultiHotkeyModifier - байты содержат VK_[L|R]CONTROL, VK_[L|R]MENU, VK_[L|R]SHIFT, VK_APPS, VK_LWIN");
    TODO("gSet.icMultiBuffer - хоткей для включения-отключения режима буфера - AskChangeBufferHeight()");
    //if (gSet.isMulti && wParam && ((lbLWin = isPressed(VK_LWIN)) || (lbRWin = isPressed(VK_RWIN)) || sb_SkipMulticonChar)) {
    if ((sb_SkipMulticonChar && (messg == WM_KEYUP || messg == WM_SYSKEYUP))
		//|| (wParam==' ' && gSet.IsHostkeyPressed()) // показать системное меню
    	|| (gSet.isMulti && wParam
	        &&
	    	(wParam==gSet.icMultiNext || wParam==gSet.icMultiNew || wParam==gSet.icMultiRecreate
	    	|| (gSet.isUseWinNumber && wParam>='0' && wParam<='9') // активировать консоль по номеру
			|| (gSet.isUseWinNumber && (wParam==VK_F11 || wParam==VK_F12))) // KeyDown для этого не проходит, но на всякий случай
	        &&
	    	gSet.IsHostkeyPressed())
	    )
    {
        if (messg == WM_KEYDOWN || messg == WM_SYSKEYDOWN /*&& (lbLWin || lbRWin) && (wParam != VK_LWIN && wParam != VK_RWIN)*/)
		{
            if (wParam==gSet.icMultiNext || wParam==gSet.icMultiNew || wParam==gSet.icMultiRecreate
            	|| (gSet.isUseWinNumber && wParam>='0' && wParam<='9')
				|| (gSet.isUseWinNumber && (wParam==VK_F11 || wParam==VK_F12)) // KeyDown для этого не проходит, но на всякий случай
                )
            {
                // Запомнить, что не нужно пускать в консоль
                sb_SkipMulticonChar = true;
                sn_SkipMulticonVk[0] = gSet.GetPressedHostkey(); // lbLWin ? VK_LWIN : VK_RWIN;
                sn_SkipMulticonVk[1] = lParam & 0xFF0000; // Specifies the scan code. The value depends on the OEM.
				// И в консоль не слать модификатор
				sb_SkipSingleHostkey = false;

                // Теперь собственно обработка
                if (wParam>='1' && wParam<='9') // ##1..9
                    ConActivate(wParam - '1');
                    
                else if (wParam=='0') // #10.
                    ConActivate(9);
                    
				else if (wParam == gSet.icMultiNext /* L'Q' */) // Win-Q
				{
					bool lbReverse = isPressed(VK_SHIFT);
					if (lbReverse
						) {
						if (gSet.IsHostkey(VK_SHIFT))
							lbReverse = false;
						//else if (!isPressed(VK_LSHIFT) || !isPressed(VK_RSHIFT)) -- не помню что хотел сказать, но это сбрасывало lbReverse
						//	lbReverse = false;
					}
                    ConActivateNext(lbReverse ? FALSE : TRUE);
                    
				}
				else if (wParam == gSet.icMultiNew /* L'W' */) // Win-W
				{
                    // Создать новую консоль
                    Recreate ( FALSE, gSet.isMultiNewConfirm );
                    
                }
				else if (wParam == gSet.icMultiRecreate /* L'~' */) // Win-~
				{
                    Recreate ( TRUE, TRUE );

                }
                return 0;
			}
        //} else if (messg == WM_CHAR) {
        //    if (sn_SkipMulticonVk[1] == (lParam & 0xFF0000))
        //        return 0; // не пропускать букву в консоль
        }
		else if (messg == WM_KEYUP || messg == WM_SYSKEYUP)
		{
            //if (wParam == ' ')
			//{
            //	ShowSysmenu();
			//	return 0;
			//} else
			if (/*(lbLWin || lbRWin) &&*/ (wParam==VK_F11 || wParam==VK_F12))
			{
				ConActivate(wParam - VK_F11 + 10);
				return 0;
			//} else if (wParam == VK_LWIN || wParam == VK_RWIN) {
            }
			else if (sn_SkipMulticonVk[1] == (lParam & 0xFF0000))
			{
                sn_SkipMulticonVk[1] = 0;
                sb_SkipMulticonChar = (sn_SkipMulticonVk[0] != 0) || (sn_SkipMulticonVk[1] != 0);
                return 0;
			}
			else //if (gSet.IsHostkey(wParam)) {
			{
                if (sn_SkipMulticonVk[0] == wParam)
				{
                    sn_SkipMulticonVk[0] = 0;
                    sb_SkipMulticonChar = (sn_SkipMulticonVk[0] != 0) || (sn_SkipMulticonVk[1] != 0);
                    return 0;
                }
            }
        }
    }
	if (gSet.IsHostkeySingle(wParam) /*|| (wParam == VK_APPS && gSet.IsHostkey(VK_APPS))*/)
	{
		if (messg == WM_KEYDOWN || messg == WM_SYSKEYDOWN)
		{
			sb_SkipSingleHostkey = true; sm_SkipSingleHostkey = messg; sw_SkipSingleHostkey = wParam; sl_SkipSingleHostkey = lParam;
			// Если после нее не будет нажат Hotkey - пошлем в консоль
			return 0;
		}
	}
	if (sb_SkipSingleHostkey && mp_VActive)
	{
		/*if (wParam != VK_APPS)*/
		{
			sb_SkipSingleHostkey = false;
			if (mp_VActive->RCon())
				mp_VActive->RCon()->OnKeyboard(hWnd, sm_SkipSingleHostkey, sw_SkipSingleHostkey, sl_SkipSingleHostkey, L"");
		}
	}
	// После хоткея создания консоли - KEYUP мог "уйти" в открытый диалог, 
	// поэтому чистим "игнорируемые" кнопки при любом следующем нажатии
	if (sb_SkipMulticonChar && (messg == WM_KEYDOWN || messg == WM_SYSKEYDOWN))
	{
		sn_SkipMulticonVk[0] = sn_SkipMulticonVk[1] = 0;
		sb_SkipMulticonChar = false;
	}
	if (sb_SkipSingleHostkey && (messg == WM_KEYDOWN || messg == WM_SYSKEYDOWN))
	{
		sb_SkipSingleHostkey = false;
	}


	
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
	
	if (isPressed(VK_LWIN) || isPressed(VK_RWIN))
	{
		if (wParam == ' ' && isPressed(VK_MENU) && !isPressed(VK_CONTROL) && !isPressed(VK_SHIFT))
		{
			if (messg == WM_SYSKEYUP)
				ShowSysmenu();
			return 0;
		}
		// WinAltEnter зарезервиновано 7-кой под запуск MediaPlayer
		else if (wParam == VK_RETURN && isPressed(VK_CONTROL) && !isPressed(VK_MENU) && !isPressed(VK_SHIFT))
		{
			if (messg == WM_KEYUP)
				OnAltEnter();
			return 0;
		}
		else if (wParam == 'P' && isPressed(VK_MENU) && !isPressed(VK_CONTROL) && !isPressed(VK_SHIFT))
		{
			if (messg == WM_SYSKEYUP)
				OnSysCommand(ghWnd, ID_SETTINGS, 0);
			return 0;
		}
		TODO("Обработать другие комбинации. WinAltEnter, WinAltF9");
		TODO("Возможно WinAltEnter вообще нужно регистрить глобально в системе, чтобы иметь Emergency popup?");
	}


	// Теперь - можно переслать в консоль	
    if (mp_VActive)
	{
		//#ifdef _DEBUG
		//if (wParam == VK_LEFT) {
		//	if (messg == WM_KEYDOWN)
		//		OutputDebugString(L"VK_LEFT pressed\n");
		//	else if (messg == WM_KEYUP)
		//		OutputDebugString(L"VK_LEFT released\n");
		//}
		//#endif

		mp_VActive->RCon()->OnKeyboard(hWnd, messg, wParam, lParam, szTranslatedChars);
    }

    return 0;
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
    wsprintf(szMsg, L"ConEmu: %s(CP:%i, HKL:0x%08I64X)\n",
        (messg == WM_INPUTLANGCHANGE) ? L"WM_INPUTLANGCHANGE" : L"WM_INPUTLANGCHANGEREQUEST",
        (DWORD)wParam, (unsigned __int64)(DWORD_PTR)lParam);
    DEBUGSTRLANG(szMsg);
    #endif
    
    if (gSet.isAdvLogging > 1)
    {
	    WCHAR szInfo[255];
	    wsprintf(szInfo, L"CConEmuMain::OnLangChange: %s(CP:%i, HKL:0x%08I64X)",
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
	for (i = 0; i < countof(m_LayoutNames); i++)
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
    wsprintf(szMsg, L"ConEmu: GetKeyboardLayout(0) after DefWindowProc(%s) = 0x%08I64X)\n",
		(messg == WM_INPUTLANGCHANGE) ? L"WM_INPUTLANGCHANGE" : L"WM_INPUTLANGCHANGEREQUEST",
		(unsigned __int64)(DWORD_PTR)hkl);
    DEBUGSTRLANG(szMsg);
    #endif



	// в Win7x64 WM_INPUTLANGCHANGEREQUEST вообще не приходит, по крайней мере при переключении мышкой
	if (messg == WM_INPUTLANGCHANGEREQUEST || messg == WM_INPUTLANGCHANGE)
	{
		static UINT   nLastMsg;
		static LPARAM lLastPrm;

		if (!(nLastMsg == WM_INPUTLANGCHANGEREQUEST && messg == WM_INPUTLANGCHANGE && lLastPrm == lParam))
		{
			mp_VActive->RCon()->SwitchKeyboardLayout(
				(messg == WM_INPUTLANGCHANGEREQUEST) ? wParam : 0, lParam
				);
		}

		nLastMsg = messg;
		lLastPrm = lParam;
	}

	m_ActiveKeybLayout = (DWORD_PTR)lParam;
    
  //  if (isFar() && gSet.isLangChangeWsPlugin)
  //  {
     //   //LONG lLastLang = GetWindowLong ( ghWndDC, GWL_LANGCHANGE );
     //   //SetWindowLong ( ghWndDC, GWL_LANGCHANGE, lParam );
     //   
     //   /*if (lLastLang == lParam)
        //    return result;*/
     //   
        //CConEmuPipe pipe(GetFarPID(), 10);
        //if (pipe.Init(_T("CConEmuMain::OnLangChange"), FALSE))
        //{
        //  if (pipe.Execute(CMD_LANGCHANGE, &lParam, sizeof(LPARAM)))
        //  {
        //      //gConEmu.DebugStep(_T("ConEmu: Switching language (1 sec)"));
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
	if ((gSet.isMonitorConsoleLang & 1) != 1)
		return 0;

	if (!isValid(apVCon))
		return 0;
	
	if (!isMainThread())
	{
	    if (gSet.isAdvLogging > 1)
	    {
		    WCHAR szInfo[255];
		    wsprintf(szInfo, L"CConEmuMain::OnLangChangeConsole (0x%08X), Posting to main thread", dwLayoutName);
	    	mp_VActive->RCon()->LogString(szInfo);
		}
	
		PostMessage(ghWnd, mn_ConsoleLangChanged, dwLayoutName, (LPARAM)apVCon);
		return 0;
	}
	else
	{
	    if (gSet.isAdvLogging > 1)
	    {
		    WCHAR szInfo[255];
		    wsprintf(szInfo, L"CConEmuMain::OnLangChangeConsole (0x%08X), MainThread", dwLayoutName);
	    	mp_VActive->RCon()->LogString(szInfo);
		}
	}

	#ifdef _DEBUG
	//Sleep(2000);
	WCHAR szMsg[255];
	// --> Видимо именно это не "нравится" руслату. Нужно переправить Post'ом в основную нить
	HKL hkl = GetKeyboardLayout(0);
	wsprintf(szMsg, L"ConEmu: GetKeyboardLayout(0) in OnLangChangeConsole after GetKeyboardLayout(0) = 0x%08I64X\n",
		(unsigned __int64)(DWORD_PTR)hkl);
	DEBUGSTRLANG(szMsg);
	//Sleep(2000);
	#endif

	wchar_t szName[10]; wsprintf(szName, L"%08X", dwLayoutName);
	#ifdef _DEBUG
	DEBUGSTRLANG(szName);
	#endif
	// --> Тут слетает!
    DWORD_PTR dwNewKeybLayout = dwLayoutName; //(DWORD_PTR)LoadKeyboardLayout(szName, 0);

	HKL hKeyb[20]; UINT nCount, i;
	nCount = GetKeyboardLayoutList ( countof(hKeyb), hKeyb );
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
	
	for (i = 0; !lbFound && i < countof(m_LayoutNames); i++)
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
		for (i = 0; !lbFound && i < nCount; i++)
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
		wsprintfW(szLayoutName, L"%08X", dwLayoutName);
		
	    if (gSet.isAdvLogging > 1)
	    {
		    WCHAR szInfo[255];
		    wsprintf(szInfo, L"CConEmuMain::OnLangChangeConsole -> LoadKeyboardLayout(0x%08X)", dwLayoutName);
	    	mp_VActive->RCon()->LogString(szInfo);
		}
		
		dwNewKeybLayout = (DWORD_PTR)LoadKeyboardLayout(szLayoutName, 0);
		
	    if (gSet.isAdvLogging > 1)
	    {
		    WCHAR szInfo[255];
		    wsprintf(szInfo, L"CConEmuMain::OnLangChangeConsole -> LoadKeyboardLayout()=0x%08X", (DWORD)dwNewKeybLayout);
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
	wsprintf(szMsg, L"ConEmu: GetKeyboardLayout(0) after LoadKeyboardLayout = 0x%08I64X\n",
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

LRESULT CConEmuMain::OnMouse(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	// кто его знает, в каких координатах оно пришло...
    //short winX = GET_X_LPARAM(lParam);
    //short winY = GET_Y_LPARAM(lParam);

	TODO("DoubleView. Хорошо бы колесико мышки перенаправлять в консоль под мышиным курором, а не в активную");
	RECT conRect = {0}, dcRect = {0};
	GetWindowRect(ghWndDC, &dcRect);
	//MapWindowPoints(NULL, ghWnd, (LPPOINT)&dcRect, 2);

	//2010-05-20 все-таки будем ориентироваться на lParam, потому что
	//  только так ConEmuTh может передать корректные координаты
	//POINT ptCur = {-1, -1}; GetCursorPos(&ptCur);
	POINT ptCur = {LOWORD(lParam), HIWORD(lParam)};
	HWND hChild = ::ChildWindowFromPointEx(ghWnd, ptCur, CWP_SKIPINVISIBLE|CWP_SKIPDISABLED|CWP_SKIPTRANSPARENT);

	// Для этих сообщений, lParam - relative to the upper-left corner of the screen.
	if (messg != WM_MOUSEWHEEL && messg != WM_MOUSEHWHEEL)
		ClientToScreen(ghWnd, &ptCur);

	//enum DragPanelBorder dpb = DPB_NONE; //CConEmuMain::CheckPanelDrag(COORD crCon)

#ifdef _DEBUG
	if (messg == WM_MOUSEWHEEL)
	{
		messg = WM_MOUSEWHEEL;
	}
#endif

	//BOOL lbMouseWasCaptured = mb_MouseCaptured;
	if (!mb_MouseCaptured)
	{
		// Если клик
		if (hChild == NULL &&
			(isPressed(VK_LBUTTON) || isPressed(VK_RBUTTON) || isPressed(VK_MBUTTON)))
		{
			// В клиентской области (области отрисовки)
			if (PtInRect(&dcRect, ptCur))
			{
				mb_MouseCaptured = TRUE;
				//TODO("После скрытия ViewPort наверное SetCapture нужно будет делать на ghWnd");
				SetCapture(ghWnd); // 100208 было ghWndDC
			}
		}
	}
	else
	{
		// Все кнопки мышки отпущены - release
		if (!isPressed(VK_LBUTTON) && !isPressed(VK_RBUTTON) && !isPressed(VK_MBUTTON))
		{
			ReleaseCapture();
			mb_MouseCaptured = FALSE;

			if (mp_VActive->RCon()->isMouseButtonDown())
			{
				if (ptCur.x < dcRect.left)
					ptCur.x = dcRect.left;
				else if (ptCur.x >= dcRect.right)
					ptCur.x = dcRect.right-1;

				if (ptCur.y < dcRect.top)
					ptCur.y = dcRect.top;
				else if (ptCur.y >= dcRect.bottom)
					ptCur.y = dcRect.bottom-1;
			}
		}
	}
	
    if (!mp_VActive)
        return 0;

    if (gSet.FontWidth()==0 || gSet.FontHeight()==0)
        return 0;

#ifdef _DEBUG
	wchar_t szDbg[60]; wsprintf(szDbg, L"GUI::MouseEvent at screen {%ix%i}\n", ptCur.x,ptCur.y);
	DEBUGSTRMOUSE(szDbg);
#endif


    if ((messg==WM_LBUTTONUP || messg==WM_MOUSEMOVE) && (gConEmu.mouse.state & MOUSE_SIZING_DBLCKL))
    {
        if (messg==WM_LBUTTONUP)
            gConEmu.mouse.state &= ~MOUSE_SIZING_DBLCKL;
        return 0; //2009-04-22 После DblCkl по заголовку в консоль мог проваливаться LBUTTONUP
    }

    if (messg == WM_MOUSEMOVE)
    {
        if (m_Back->TrackMouse())
            return 0;
    }

	

	if (mp_VActive->RCon()->isSelectionPresent()
		&& ((wParam & MK_LBUTTON) || messg == WM_LBUTTONUP))
	{
		ptCur.x -= dcRect.left; ptCur.y -= dcRect.top;
		mp_VActive->RCon()->OnMouse(messg, wParam, ptCur.x, ptCur.y);
		return 0;
	}


    // Иначе в консоль проваливаются щелчки по незанятому полю таба...
	// все, что кроме - считаем кликами и они должны попадать в dcRect
    if (messg != WM_MOUSEMOVE && messg != WM_MOUSEWHEEL && messg != WM_MOUSEHWHEEL)
    {
        if (gConEmu.mouse.state & MOUSE_SIZING_DBLCKL)
            gConEmu.mouse.state &= ~MOUSE_SIZING_DBLCKL;

        if (!PtInRect(&dcRect, ptCur))
        {
            DEBUGLOGFILE("Click outside of DC");
            return 0;
        }
    }

	// Переводим в клиентские координаты
	ptCur.x -= dcRect.left; ptCur.y -= dcRect.top;
    GetClientRect(ghConWnd, &conRect);

    COORD cr = mp_VActive->ClientToConsole(ptCur.x,ptCur.y);
    short conX = cr.X; //winX/gSet.Log Font.lfWidth;
    short conY = cr.Y; //winY/gSet.Log Font.lfHeight;

    if ((messg != WM_MOUSEWHEEL && messg != WM_MOUSEHWHEEL) && (conY<0 || conY<0))
    {
		DEBUGLOGFILE("Mouse outside of upper-left");
        return 0;
    }
    
    //* ****************************
    //* Если окно ConEmu не в фокусе - не слать к консоль движение мышки,
    //* иначе получается неприятная реакция пунктов меню и т.п.
    //* ****************************
    if (gSet.isMouseSkipMoving && GetForegroundWindow() != ghWnd)
    {
    	DEBUGLOGFILE("ConEmu is not foreground window, mouse event skipped");
    	return 0;
    }

    //* ****************************
    //* После получения WM_MOUSEACTIVATE опционально можно не пропускать
    //* клик в консоль, чтобы при активации ConEmu случайно не задеть
    //* (закрыть или отменить) висящий в FAR диалог
    //* ****************************
    if ((gConEmu.mouse.nSkipEvents[0] && gConEmu.mouse.nSkipEvents[0] == messg)
        || (gConEmu.mouse.nSkipEvents[1] 
	        && (gConEmu.mouse.nSkipEvents[1] == messg || messg == WM_MOUSEMOVE)))
    {
		if (gConEmu.mouse.nSkipEvents[0] == messg)
		{
			gConEmu.mouse.nSkipEvents[0] = 0;
			DEBUGSTRMOUSE(L"Skipping Mouse down\n");
		}
		else if (gConEmu.mouse.nSkipEvents[1] == messg)
		{
			gConEmu.mouse.nSkipEvents[1] = 0;
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
    //* После получения WM_MOUSEACTIVATE и включенном gSet.isMouseSkipActivation
    //* двойной клик переслать в консоль как одинарный, иначе после активации
    //* мышкой быстрый клик в том же месте будет обработан неправильно
    //* ****************************
    if (mouse.nReplaceDblClk)
    {
		if (!gSet.isMouseSkipActivation)
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
		if (mp_DragDrop->ForwardMessage(hWnd, messg, wParam, lParam))
			return 0;
	}

	// -- поскольку выделялка у нас теперь своя - этого похоже не требуется. лишние движения окна 
	// -- вызывают side-effects на некоторых системах. Issue 274: Окно реальной консоли позиционируется в неудобном месте
    /////*&& isPressed(VK_LBUTTON)*/) && // Если этого не делать - при выделении мышкой консоль может самопроизвольно прокрутиться
    //CRealConsole* pRCon = mp_VActive->RCon();
    //// Только при скрытой консоли. Иначе мы ConEmu видеть вообще не будем
    //if (pRCon && (/*pRCon->isBufferHeight() ||*/ !pRCon->isWindowVisible())
    //    && (messg == WM_LBUTTONDOWN || messg == WM_RBUTTONDOWN || messg == WM_LBUTTONDBLCLK || messg == WM_RBUTTONDBLCLK
    //        || (messg == WM_MOUSEMOVE && isPressed(VK_LBUTTON)))
    //    )
    //{
    //   // buffer mode: cheat the console window: adjust its position exactly to the cursor
    //   RECT win;
    //   GetWindowRect(ghWnd, &win);
    //   short x = win.left + ptCur.x - MulDiv(ptCur.x, conRect.right, klMax<uint>(1, mp_VActive->Width));
    //   short y = win.top + ptCur.y - MulDiv(ptCur.y, conRect.bottom, klMax<uint>(1, mp_VActive->Height));
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
    

	// Теперь можно обрабатывать мышку, и если нужно - слать ее в консоль
    if (messg == WM_MOUSEMOVE)
    {
		if (!OnMouse_Move(hWnd, WM_MOUSEMOVE, wParam, lParam, ptCur, cr))
			return 0;

    }
    else
    {
        mouse.lastMMW=-1; mouse.lastMML=-1;

		if (messg == WM_LBUTTONDBLCLK)
		{
			if (!OnMouse_LBtnDblClk(hWnd, messg, wParam, lParam, ptCur, cr))
				return 0;

		} // !!! Без else, т.к. теоретически функция может заменить клик на одинарный
        
        if (messg == WM_RBUTTONDBLCLK)
		{
			if (!OnMouse_RBtnDblClk(hWnd, messg, wParam, lParam, ptCur, cr))
				return 0;

        } // !!! Без else, т.к. функция может заменить клик на одинарный

        
        // Теперь обрабатываем что осталось
        if (messg == WM_LBUTTONDOWN)
        {
			if (!OnMouse_LBtnDown(hWnd, WM_LBUTTONDOWN, wParam, lParam, ptCur, cr))
				return 0;

        }
        else if (messg == WM_LBUTTONUP)
        {
			if (!OnMouse_LBtnUp(hWnd, WM_LBUTTONUP, wParam, lParam, ptCur, cr))
				return 0;

        }
        else if (messg == WM_RBUTTONDOWN)
        {
			if (!OnMouse_RBtnDown(hWnd, WM_RBUTTONDOWN, wParam, lParam, ptCur, cr))
				return 0;

        }
        else if (messg == WM_RBUTTONUP)
        {
			if (!OnMouse_RBtnUp(hWnd, WM_RBUTTONUP, wParam, lParam, ptCur, cr))
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
	mouse.lastMMW=wParam; mouse.lastMML=MAKELPARAM( ptCur.x, ptCur.y );

    // Теперь осталось послать событие в консоль
    CRealConsole *pRCon;
    if (mp_VActive && (pRCon = mp_VActive->RCon()))
    	pRCon->OnMouse(messg, wParam, ptCur.x, ptCur.y);
    return 0;
}

LRESULT CConEmuMain::OnMouse_Move(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr)
{
	// WM_MOUSEMOVE вроде бы слишком часто вызывается даже при условии что курсор не двигается...
	if (wParam==mouse.lastMMW && lParam==mouse.lastMML
		&& (mouse.state & MOUSE_DRAGPANEL_ALL) == 0)
	{
		DEBUGSTRPANEL2(L"PanelDrag: Skip of wParam==mouse.lastMMW && lParam==mouse.lastMML\n");
		return 0;
	}
	mouse.lastMMW=wParam; mouse.lastMML=lParam;

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
		if (!gSet.isDragPanel)
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
			if (cr.X == mouse.LClkCon.X && cr.Y == mouse.LClkCon.Y) {
				DEBUGSTRPANEL2(L"PanelDrag: Skip of cr.X == mouse.LClkCon.X && cr.Y == mouse.LClkCon.Y\n");
				return 0;
			}

			if (gSet.isDragPanel == 1)
				OnSizePanels(cr);

			// Мышь не слать...
			return 0;
		}        	
	}


	//TODO: вроде бы иногда isSizing() не сбрасывается?
	//? может так: if (gSet.isDragEnabled & mouse.state)
	if (gSet.isDragEnabled & ((mouse.state & (DRAG_L_ALLOWED|DRAG_R_ALLOWED))!=0)
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
				// 2009-06-19 Выполнялось сразу после "if (gSet.isDragEnabled & (mouse.state & (DRAG_L_ALLOWED|DRAG_R_ALLOWED)))"
				if (mouse.state & MOUSE_R_LOCKED)
				{
					// чтобы при RightUp не ушел APPS
					mouse.state &= ~MOUSE_R_LOCKED;

					//PtDiffTest(C, LOWORD(lParam), HIWORD(lParam), D)
					char szLog[255];
					wsprintfA(szLog, "Right drag started, MOUSE_R_LOCKED cleared: cursor={%i-%i}, Rcursor={%i-%i}, Now={%i-%i}, MinDelta=%i",
						(int)cursor.x, (int)cursor.y,
						(int)Rcursor.x, (int)Rcursor.y,
						(int)LOWORD(lParam), (int)HIWORD(lParam),
						(int)DRAG_DELTA);
					mp_VActive->RCon()->LogString(szLog);
				}

				BOOL lbLeftDrag = (mouse.state & DRAG_L_ALLOWED) == DRAG_L_ALLOWED;

				mp_VActive->RCon()->LogString(lbLeftDrag ? "Left drag about to start" : "Right drag about to start");

				// Если сначала фокус был на файловой панели, но после LClick он попал на НЕ файловую - отменить ShellDrag
				bool bFilePanel = isFilePanel();
				if (!bFilePanel)
				{
					DebugStep(_T("DnD: not file panel"));
					//isLBDown = false; 
					mouse.state &= ~(DRAG_L_ALLOWED | DRAG_L_STARTED | DRAG_R_ALLOWED | DRAG_R_STARTED);
					mouse.bIgnoreMouseMove = false;
					//POSTMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( newX, newY ), TRUE);     //посылаем консоли отпускание
					mp_VActive->RCon()->OnMouse(WM_LBUTTONUP, wParam, ptCur.x, ptCur.x);
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
				//	mp_VActive->RCon()->OnMouse(WM_LBUTTONUP, wParam, mouse.LClkDC.X, mouse.LClkDC.Y);
				//} else {
				//	//POSTMESSAGE(ghConWnd, WM_LBUTTONDOWN, wParam, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);     //посылаем консоли отпускание
				//	mp_VActive->RCon()->OnMouse(WM_LBUTTONDOWN, wParam, mouse.RClkDC.X, mouse.RClkDC.Y);
				//	//POSTMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);     //посылаем консоли отпускание
				//	mp_VActive->RCon()->OnMouse(WM_LBUTTONUP, wParam, mouse.RClkDC.X, mouse.RClkDC.Y);
				//}
				//mp_VActive->RCon()->FlushInputQueue();

				// Иначе иногда срабатывает FAR'овский D'n'D
				//SENDMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( newX, newY ));     //посылаем консоли отпускание
				if (mp_DragDrop)
				{
					//COORD crMouse = mp_VActive->ClientToConsole(
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
				//newX = MulDiv(cursor.x, conRect.right, klMax<uint>(1, mp_VActive->Width));
				//newY = MulDiv(cursor.y, conRect.bottom, klMax<uint>(1, mp_VActive->Height));
				//#endif
				//if (lbLeftDrag)
				//  POSTMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( newX, newY ), TRUE);     //посылаем консоли отпускание
				//isDragProcessed=false; -- убрал, иначе при бросании в пассивную панель больших файлов дроп может вызваться еще раз???
				return 0;
			}
		}
	}
	else if (gSet.isRClickSendKey && (mouse.state & MOUSE_R_LOCKED))
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
				(int)LOWORD(lParam), (int)HIWORD(lParam),
				(int)RCLICKAPPSDELTA);
			mp_VActive->RCon()->LogString(szLog);

			//POSTMESSAGE(ghConWnd, WM_RBUTTONDOWN, 0, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);
			mp_VActive->RCon()->OnMouse(WM_RBUTTONDOWN, 0, mouse.RClkDC.X, mouse.RClkDC.Y);
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

LRESULT CConEmuMain::OnMouse_LBtnDown(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr)
{
	enum DragPanelBorder dpb = DPB_NONE;

	//if (isLBDown()) ReleaseCapture(); // Вдруг сглючило? --2009-03-14
	//isLBDown = false;
	mouse.state &= ~(DRAG_L_ALLOWED | DRAG_L_STARTED | MOUSE_DRAGPANEL_ALL);
	mouse.bIgnoreMouseMove = false;
	mouse.LClkCon = cr;
	mouse.LClkDC = MakeCoord(ptCur.x,ptCur.y);

	CRealConsole *pRCon = mp_VActive->RCon();
	if (!pRCon) // если консоли нет - то и слать некуда
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
		mp_VActive->RCon()->PostConsoleEvent ( &r );
		return 0;
	}

	if (!isConSelectMode() && isFilePanel() && mp_VActive &&
		mp_VActive->RCon()->CoordInPanel(mouse.LClkCon))
	{
		//SetCapture(ghWndDC); --2009-03-14
		cursor.x = LOWORD(lParam);
		cursor.y = HIWORD(lParam); 
		//isLBDown=true;
		//isDragProcessed=false;
		CONSOLE_CURSOR_INFO ci;
		mp_VActive->RCon()->GetConsoleCursorInfo(&ci);
		if ((ci.bVisible && ci.dwSize>0) // курсор должен быть видим, иначе это чье-то меню
			&& gSet.isDragEnabled & DRAG_L_ALLOWED)
		{
			//if (!gSet.nLDragKey || isPressed(gSet.nLDragKey))
			// функция проверит нажат ли nLDragKey (допускает nLDragKey==0), а другие - не нажаты
			// то есть нажат SHIFT(==nLDragKey), а CTRL & ALT - НЕ нажаты
			if (gSet.isModifierPressed(gSet.nLDragKey))
			{
				mouse.state = DRAG_L_ALLOWED;
			}
		}

		// иначе после LBtnDown в консоль может СРАЗУ провалиться MOUSEMOVE
		mouse.lastMMW=wParam; mouse.lastMML=lParam;

		//if (gSet.is DnD) mouse.bIgnoreMouseMove = true;
		//POSTMESSAGE(ghConWnd, messg, wParam, MAKELPARAM( newX, newY ), FALSE); // было SEND
		mp_VActive->RCon()->OnMouse(messg, wParam, ptCur.x, ptCur.y);
		return 0;
	}

	return TRUE; // переслать в консоль
}
LRESULT CConEmuMain::OnMouse_LBtnUp(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr)
{
	BOOL lbLDrag = (mouse.state & DRAG_L_STARTED) == DRAG_L_STARTED;

	if (mouse.state & MOUSE_DRAGPANEL_ALL)
	{
		if (gSet.isDragPanel == 2)
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
		//newX = MulDiv(cursor.x, conRect.right, klMax<uint>(1, mp_VActive->Width));
		//newY = MulDiv(cursor.y, conRect.bottom, klMax<uint>(1, mp_VActive->Height));
		//#endif
		//POSTMESSAGE(ghConWnd, messg, wParam, MAKELPARAM( newX, newY ), FALSE);
		mp_VActive->RCon()->OnMouse(messg, wParam, cursor.x, cursor.y);
		return 0;
	}

	return TRUE; // переслать в консоль
}
LRESULT CConEmuMain::OnMouse_LBtnDblClk(HWND hWnd, UINT& messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr)
{
	
	CRealConsole *pRCon = mp_VActive->RCon();
	if (!pRCon) // Если консоли нет - то слать некуда
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
				wsprintf(szMacro, L"@$Rep (%i) CtrlLeft $End", rcRight.left - nCenter);
			else if (nCenter > rcRight.left)
				wsprintf(szMacro, L"@$Rep (%i) CtrlRight $End", nCenter - rcRight.left);
		}
		else
		{
			wsprintf(szMacro, L"@$Rep (%i) CtrlDown $End", pRCon->TextHeight());
		}
		if (szMacro[0])
			PostMacro(szMacro);
		return 0;
	}

	return TRUE; // переслать в консоль
}
LRESULT CConEmuMain::OnMouse_RBtnDown(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr)
{
	Rcursor.x = LOWORD(lParam);
	Rcursor.y = HIWORD(lParam);
	mouse.RClkCon = cr;
	mouse.RClkDC = MakeCoord(ptCur.x,ptCur.y);
	//isRBDown=false;
	mouse.state &= ~(DRAG_R_ALLOWED | DRAG_R_STARTED | MOUSE_R_LOCKED);
	mouse.bIgnoreMouseMove = false;

	if (gSet.isAdvLogging)
	{
		char szLog[100];
		wsprintfA(szLog, "Right button down: Rcursor={%i-%i}", (int)Rcursor.x, (int)Rcursor.y);
		mp_VActive->RCon()->LogString(szLog);
	}

	//if (isFilePanel()) // Maximus5
	bool bSelect = false, bPanel = false, bActive = false, bCoord = false;
	if (!(bSelect = isConSelectMode())
		&& (bPanel = isFilePanel())
		&& (bActive = (mp_VActive != NULL))
		&& (bCoord = mp_VActive->RCon()->CoordInPanel(mouse.RClkCon)))
	{
		if (gSet.isDragEnabled & DRAG_R_ALLOWED)
		{
			//if (!gSet.nRDragKey || isPressed(gSet.nRDragKey)) {
			// функция проверит нажат ли nRDragKey (допускает nRDragKey==0), а другие - не нажаты
			// то есть нажат SHIFT(==nRDragKey), а CTRL & ALT - НЕ нажаты
			if (gSet.isModifierPressed(gSet.nRDragKey))
			{
				mouse.state = DRAG_R_ALLOWED;
				if (gSet.isAdvLogging) mp_VActive->RCon()->LogString("RightClick ignored of gSet.nRDragKey pressed");
				return 0;
			}
		}
		
		// Если ничего лишнего не нажато!
		if (gSet.isRClickSendKey && !(wParam&(MK_CONTROL|MK_LBUTTON|MK_MBUTTON|MK_SHIFT|MK_XBUTTON1|MK_XBUTTON2)))
		{
			//заведем таймер на .3
			//если больше - пошлем apps
			mouse.state |= MOUSE_R_LOCKED; mouse.bSkipRDblClk = false;

			char szLog[100];
			wsprintfA(szLog, "MOUSE_R_LOCKED was set: Rcursor={%i-%i}", (int)Rcursor.x, (int)Rcursor.y);
			mp_VActive->RCon()->LogString(szLog);

			mouse.RClkTick = TimeGetTime(); //GetTickCount();
			StartRightClickingPaint();
			return 0;
		}
		else
		{
			if (gSet.isAdvLogging)
				mp_VActive->RCon()->LogString(
					!gSet.isRClickSendKey ? "RightClick ignored of !gSet.isRClickSendKey" :
					"RightClick ignored of wParam&(MK_CONTROL|MK_LBUTTON|MK_MBUTTON|MK_SHIFT|MK_XBUTTON1|MK_XBUTTON2)"
				);
		}
	}
	else
	{
		if (gSet.isAdvLogging)
			mp_VActive->RCon()->LogString(
				bSelect ? "RightClick ignored of isConSelectMode" :
				!bPanel ? "RightClick ignored of NOT isFilePanel" :
				!bActive ? "RightClick ignored of NOT isFilePanel" :
				!bCoord ? "RightClick ignored of NOT isFilePanel" :
				"RightClick ignored, unknown cause"
			);
	}

	return TRUE; // переслать в консоль
}
LRESULT CConEmuMain::OnMouse_RBtnUp(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr)
{
	// Сразу сбросить, если выставлялся
	StopRightClickingPaint();
	
	if (gSet.isRClickSendKey && (mouse.state & MOUSE_R_LOCKED))
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
			if ((gSet.isRClickSendKey==1) ||
				(dwDelta>RCLICKAPPSTIMEOUT/*.3сек*/ && dwDelta<RCLICKAPPSTIMEOUT_MAX/*10000*/))
			{
				//// Сначала выделить файл под курсором
				////POSTMESSAGE(ghConWnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);
				//mp_VActive->RCon()->OnMouse(WM_LBUTTONDOWN, MK_LBUTTON, mouse.RClkDC.X, mouse.RClkDC.Y);
				////POSTMESSAGE(ghConWnd, WM_LBUTTONUP, 0, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);
				//mp_VActive->RCon()->OnMouse(WM_LBUTTONUP, 0, mouse.RClkDC.X, mouse.RClkDC.Y);
				//
				//mp_VActive->RCon()->FlushInputQueue();

				//mp_VActive->Update(true);
				//INVALIDATE(); //InvalidateRect(HDCWND, NULL, FALSE);

				// А теперь можно и Apps нажать
				mouse.bSkipRDblClk=true; // чтобы пока FAR думает в консоль не проскочило мышиное сообщение
				//POSTMESSAGE(ghConWnd, WM_KEYDOWN, VK_APPS, 0, TRUE);

				DWORD dwFarPID = mp_VActive->RCon()->GetFarPID();
				if (dwFarPID)
				{
					AllowSetForegroundWindow(dwFarPID);

					//if (gSet.sRClickMacro && *gSet.sRClickMacro) {
					//    //// Сначала выделить файл под курсором
					//    ////POSTMESSAGE(ghConWnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);
					//    //mp_VActive->RCon()->OnMouse(WM_LBUTTONDOWN, MK_LBUTTON, mouse.RClkDC.X, mouse.RClkDC.Y);
					//    ////POSTMESSAGE(ghConWnd, WM_LBUTTONUP, 0, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);
					//    //mp_VActive->RCon()->OnMouse(WM_LBUTTONUP, 0, mouse.RClkDC.X, mouse.RClkDC.Y);
					//    //
					//    //mp_VActive->RCon()->FlushInputQueue();
					//
					//    // Если юзер задал свой макрос - выполняем его
					//    PostMacro(gSet.sRClickMacro);
					//} else {

					COORD crMouse = mp_VActive->RCon()->ScreenToBuffer(
							mp_VActive->ClientToConsole(mouse.RClkDC.X, mouse.RClkDC.Y)
						);
					

					CConEmuPipe pipe(GetFarPID(), CONEMUREADYTIMEOUT);
					if (pipe.Init(_T("CConEmuMain::EMenu"), TRUE))
					{
						//DWORD cbWritten=0;
						DebugStep(_T("EMenu: Waiting for result (10 sec)"));

						int nLen = 0;
						int nSize = sizeof(crMouse) + sizeof(wchar_t);
						if (gSet.sRClickMacro && *gSet.sRClickMacro)
						{
							nLen = lstrlen(gSet.sRClickMacro);
							nSize += nLen*2;
							// -- заменено на перехват функции ScreenToClient
							//mp_VActive->RCon()->RemoveFromCursor();
						}
						LPBYTE pcbData = (LPBYTE)calloc(nSize,1);
						_ASSERTE(pcbData);

						memmove(pcbData, &crMouse, sizeof(crMouse));
						if (nLen)
							lstrcpy((wchar_t*)(pcbData+sizeof(crMouse)), gSet.sRClickMacro);

						if (!pipe.Execute(CMD_EMENU, pcbData, nSize))
						{
							mp_VActive->RCon()->LogString("RightClicked, but pipe.Execute(CMD_EMENU) failed");
						}
						else
						{
							// OK
							if (gSet.isAdvLogging)
							{
								char szInfo[255] = {0};
								lstrcpyA(szInfo, "RightClicked, pipe.Execute(CMD_EMENU) OK");
								if (gSet.sRClickMacro && *gSet.sRClickMacro)
								{
									lstrcatA(szInfo, ", Macro: "); int nLen = lstrlenA(szInfo);
									WideCharToMultiByte(CP_ACP,0, gSet.sRClickMacro,-1, szInfo+nLen, countof(szInfo)-nLen-1, 0,0);
								}
								else
								{
									lstrcatA(szInfo, ", NoMacro");
								}
								mp_VActive->RCon()->LogString(szInfo);
							}
						}
						DebugStep(NULL);

						free(pcbData);

					}
					else
					{
						mp_VActive->RCon()->LogString("RightClicked, but pipe.Init() failed");
					}

					//INPUT_RECORD r = {KEY_EVENT};
					////mp_VActive->RCon()->On Keyboard(ghConWnd, WM_KEYDOWN, VK_APPS, (VK_APPS << 16) | (1 << 24));
					////mp_VActive->RCon()->On Keyboard(ghConWnd, WM_KEYUP, VK_APPS, (VK_APPS << 16) | (1 << 24));
					//r.Event.KeyEvent.bKeyDown = TRUE;
					//r.Event.KeyEvent.wVirtualKeyCode = VK_APPS;
					//r.Event.KeyEvent.wVirtualScanCode = /*28 на моей клавиатуре*/MapVirtualKey(VK_APPS, 0/*MAPVK_VK_TO_VSC*/);
					//r.Event.KeyEvent.dwControlKeyState = 0x120;
					//mp_VActive->RCon()->PostConsoleEvent(&r);

					////On Keyboard(hConWnd, WM_KEYUP, VK_RETURN, 0);
					//r.Event.KeyEvent.bKeyDown = FALSE;
					//r.Event.KeyEvent.dwControlKeyState = 0x120;
					//mp_VActive->RCon()->PostConsoleEvent(&r);
				}
				else
				{
					mp_VActive->RCon()->LogString("RightClicked, but FAR PID is 0");
				}
				return 0;
			}
			else
			{
				char szLog[255];
				// if ((gSet.isRClickSendKey==1) || (dwDelta>RCLICKAPPSTIMEOUT && dwDelta<10000))
				lstrcpyA(szLog, "RightClicked, but condition failed: ");
				if (gSet.isRClickSendKey!=1)
				{
					wsprintfA(szLog+lstrlenA(szLog), "((isRClickSendKey=%i)!=1)", (UINT)gSet.isRClickSendKey);
				}
				else
				{
					wsprintfA(szLog+lstrlenA(szLog), "(isRClickSendKey==%i)", (UINT)gSet.isRClickSendKey);
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
				mp_VActive->RCon()->LogString(szLog);
			}
		}
		else
		{
			char szLog[100];
			wsprintfA(szLog, "RightClicked, but mouse was moved abs({%i-%i}-{%i-%i})>%i", (int)Rcursor.x, (int)Rcursor.y, (int)LOWORD(lParam), (int)HIWORD(lParam), (int)RCLICKAPPSDELTA);
			mp_VActive->RCon()->LogString(szLog);
		}

		// иначе после RBtnDown в консоль может СРАЗУ провалиться MOUSEMOVE
		mouse.lastMMW=MK_RBUTTON|wParam; mouse.lastMML=lParam; // добавляем, т.к. мы в RButtonUp

		// Иначе нужно сначала послать WM_RBUTTONDOWN
		//POSTMESSAGE(ghConWnd, WM_RBUTTONDOWN, wParam, MAKELPARAM( newX, newY ), TRUE);
		mp_VActive->RCon()->OnMouse(WM_RBUTTONDOWN, wParam, ptCur.x, ptCur.y);
	}
	else
	{
		char szLog[255];
		// if (gSet.isRClickSendKey && (mouse.state & MOUSE_R_LOCKED))
		//wsprintfA(szLog, "RightClicked, but condition failed (RCSK:%i, State:%u)", (int)gSet.isRClickSendKey, (DWORD)mouse.state);
		lstrcpyA(szLog, "RightClicked, but condition failed: ");
		if (gSet.isRClickSendKey==0)
		{
			wsprintfA(szLog+lstrlenA(szLog), "((isRClickSendKey=%i)==0)", (UINT)gSet.isRClickSendKey);
		}
		else
		{
			wsprintfA(szLog+lstrlenA(szLog), "(isRClickSendKey==%i)", (UINT)gSet.isRClickSendKey);
		}
		if ((mouse.state & MOUSE_R_LOCKED) == 0)
		{
			wsprintfA(szLog+lstrlenA(szLog), ", (((state=0x%X)&MOUSE_R_LOCKED)==0)", (DWORD)mouse.state);
		}
		else
		{
			wsprintfA(szLog+lstrlenA(szLog), ", (state==0x%X)", (DWORD)mouse.state);
		}
		mp_VActive->RCon()->LogString(szLog);
	}
	//isRBDown=false; // чтобы не осталось случайно
	mouse.state &= ~(DRAG_R_ALLOWED | DRAG_R_STARTED | MOUSE_R_LOCKED);

	return TRUE; // переслать в консоль
}
LRESULT CConEmuMain::OnMouse_RBtnDblClk(HWND hWnd, UINT& messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr)
{
	if (mouse.bSkipRDblClk)
	{
		mouse.bSkipRDblClk = false;
		return 0; // не обрабатывать, сейчас висит контекстное меню
	}
	
	//if (gSet.isRClickSendKey) {
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
		if (isZoomed() || gSet.isFullScreen)
		{
			// Так быть не должно - рамка не должна быть видна в этих режимах
			_ASSERTE(!isZoomed() && !gSet.isFullScreen);
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
			MOVEWINDOW ( ghWnd, rcNewWnd.left, rcNewWnd.top, rcNewWnd.right-rcNewWnd.left, rcNewWnd.bottom-rcNewWnd.top, 1);
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
			wsprintf(szDbg, L"Foreground changed (%s). NewFore=0x00000000, LBtn=%i\n", asFrom, lbLDown);
			#endif
		}
		else
		{
			dwTID = GetWindowThreadProcessId(hCurForeground, &dwPID);

			// Получить информацию об активном треде
			GUITHREADINFO gti = {sizeof(GUITHREADINFO)};
			GetGUIThreadInfo(0/*dwTID*/, &gti);

			#ifdef _DEBUG
			wsprintf(szDbg, L"Foreground changed (%s). NewFore=0x%08X, Active=0x%08X, Focus=0x%08X, Class=%s, LBtn=%i\n", asFrom, (DWORD)hCurForeground, (DWORD)gti.hwndActive, (DWORD)gti.hwndFocus, szClass, lbLDown);
			#endif

			// mh_ShellWindow чисто для информации. Хоть родитель ConEmu и меняется на mh_ShellWindow
			// но проводник может перекинуть наше окно в другое (WorkerW или Progman)
			if (gSet.isDesktopMode && mh_ShellWindow)
			{
				
				//HWND hShell = GetShellWindow(); // Progman
				bool lbDesktopActive = false;
				if (dwPID == mn_ShellWindowPID) // дальше только для процесса Explorer.exe (который держит Desktop)
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
							MapWindowPoints(ghWndDC, NULL, &pt, 1);
							HWND hAtPoint = WindowFromPoint(pt);
							if (hAtPoint != ghWnd)
							{
								#ifdef _DEBUG
								wchar_t szDbg[255], szClass[64];
								if (!hAtPoint || !GetClassName(hAtPoint, szClass, 63)) szClass[0] = 0;
								wsprintf(szDbg, L"Can't activate ConEmu on desktop. Opaque cell={%i,%i} screen={%i,%i}. WindowFromPoint=0x%08X (%s)\n",
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
								mouse_event ( MOUSEEVENTF_ABSOLUTE+MOUSEEVENTF_LEFTDOWN, pt.x,pt.y, 0,0);
								mouse_event ( MOUSEEVENTF_ABSOLUTE+MOUSEEVENTF_LEFTUP, pt.x,pt.y, 0,0);
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
		wsprintf(szDbg, L"Foreground changed (%s). NewFore=0x%08X, ConEmu has focus, LBtn=%i\n", asFrom, (DWORD)hCurForeground, lbLDown);
		#endif

		mb_FocusOnDesktop = TRUE;
	}

	DEBUGSTRFOREGROUND(szDbg);
}

enum DragPanelBorder CConEmuMain::CheckPanelDrag(COORD crCon)
{
	if (!gSet.isDragPanel || isPictureView())
		return DPB_NONE;

	CRealConsole* pRCon = mp_VActive->RCon();
	if (!pRCon)
		return DPB_NONE;
	if (!pRCon->isFar() || !pRCon->isFilePanel(true))
		return DPB_NONE;

	// Если активен наш или ФАРовский граббер
	if (pRCon->isConSelectMode())
		return DPB_NONE;
	// Если удерживается модификатор запуска драга
	if ((gSet.isCTSSelectBlock && gSet.isCTSVkBlock && gSet.isModifierPressed(gSet.isCTSVkBlock))
		|| (gSet.isCTSSelectText && gSet.isCTSVkText && gSet.isModifierPressed(gSet.isCTSVkText)))
		return DPB_NONE;
		
	//CONSOLE_CURSOR_INFO ci;
	//mp_VActive->RCon()->GetConsoleCursorInfo(&ci);
	//   if (!ci.bVisible || ci.dwSize>40) // Курсор должен быть видим, и не в режиме граба
	//   	return DPB_NONE;

	// Теперь - можно проверить	
	enum DragPanelBorder dpb = DPB_NONE;
	RECT rcPanel;
	if (mp_VActive->RCon()->GetPanelRect(TRUE, &rcPanel, TRUE))
	{
		if (crCon.X == rcPanel.left && (rcPanel.top <= crCon.Y && crCon.Y <= rcPanel.bottom))
			dpb = DPB_SPLIT;
		else if (crCon.Y == rcPanel.bottom && (rcPanel.left <= crCon.X && crCon.X <= rcPanel.right))
			dpb = DPB_RIGHT;
	}
	if (dpb == DPB_NONE && mp_VActive->RCon()->GetPanelRect(FALSE, &rcPanel, TRUE))
	{
		if (crCon.Y == rcPanel.bottom && (rcPanel.left <= crCon.X && crCon.X <= rcPanel.right))
			dpb = DPB_LEFT;
	}
	return dpb;
}

LRESULT CConEmuMain::OnSetCursor(WPARAM wParam, LPARAM lParam)
{
	POINT ptCur; GetCursorPos(&ptCur);

	if (lParam == (LPARAM)-1)
	{
		RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);
		if (!PtInRect(&rcWnd, ptCur))
		{
			if (!isMeForeground())
				return FALSE;
			lParam = HTNOWHERE;
		}
		else
		{
			GetWindowRect(ghWndDC, &rcWnd);
			if (PtInRect(&rcWnd, ptCur))
				lParam = HTCLIENT;
			else
				lParam = HTCAPTION;
		}
		wParam = (WPARAM)ghWnd;
	}

	if (((HWND)wParam) != ghWnd || isSizing()
		|| (LOWORD(lParam) != HTCLIENT && LOWORD(lParam) != HTNOWHERE))
	{
		/*if (gSet.isHideCaptionAlways && !mb_InTrackSysMenu && !isSizing()
			&& (LOWORD(lParam) == HTTOP || LOWORD(lParam) == HTCAPTION))
		{
			SetCursor(mh_CursorMove);
			return TRUE;
		}*/
		return FALSE;
	}
	
	HCURSOR hCur = NULL;

	DEBUGSTRSETCURSOR(L"WM_SETCURSOR");

	BOOL lbMeFore = TRUE;

	if (LOWORD(lParam) == HTCLIENT && mp_VActive)
	{
		if (mh_DragCursor && isDragging())
		{
			hCur = mh_DragCursor;
		}
		else if (mouse.state & MOUSE_DRAGPANEL_ALL)
		{
			if (mouse.state & MOUSE_DRAGPANEL_SPLIT)
				hCur = mh_SplitH;
			else
				hCur = mh_SplitV;
		}
		else
		{
			lbMeFore = isMeForeground();
			if (lbMeFore)
			{
				CRealConsole *pRCon = mp_VActive->RCon();
				if (pRCon && pRCon->isFar(FALSE)) // Плагин не нужен, ФАР сам...
				{
					MapWindowPoints(NULL, ghWndDC, &ptCur, 1);
					COORD crCon = mp_VActive->ClientToConsole(ptCur.x,ptCur.y);
					enum DragPanelBorder dpb = CheckPanelDrag(crCon);
					if (dpb == DPB_SPLIT)
						hCur = mh_SplitH;
					else if (dpb != DPB_NONE)
						hCur = mh_SplitV;
				}
			}
		}
	}

	if (!hCur && lbMeFore)
	{
		if (mb_WaitCursor)
		{
			hCur = mh_CursorWait;
			DEBUGSTRSETCURSOR(L" ---> CursorWait\n");
		}
		else if (gSet.isFarHourglass)
		{
			CRealConsole *pRCon = NULL;
			if (mp_VActive)
				pRCon = mp_VActive->RCon();
			if (pRCon)
			{
				BOOL lbAlive = pRCon->isAlive();
				
				if (!lbAlive)
				{
					hCur = mh_CursorAppStarting;
					DEBUGSTRSETCURSOR(L" ---> AppStarting\n");
				}
			}
		}
	}
	if (!hCur)
	{
		hCur = mh_CursorArrow;
		DEBUGSTRSETCURSOR(L" ---> Arrow\n");
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
    switch (wParam)
    {
    case HSHELL_FLASH:
        {
            //HWND hCon = (HWND)lParam;
            //if (!hCon) return 0;
            //for (int i = 0; i<MAX_CONSOLE_COUNT; i++) {
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
                    
                    if (IsWindowVisible(hWnd)) // ? оно успело ?
                    {
                    	// Получить PID родительского процесса этого окошка
                    	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
                        if (hSnap != INVALID_HANDLE_VALUE)
                        {
                            PROCESSENTRY32 prc = {sizeof(PROCESSENTRY32)};
                            if (Process32First(hSnap, &prc))
                            {
                                do {
                                    if (prc.th32ProcessID == dwPID)
                                    {
                                        dwParentPID = prc.th32ParentProcessID;
                                        break;
                                    }
                                } while (Process32Next(hSnap, &prc));
                            }
                            CloseHandle(hSnap);
                        }
                    	
                    	for (int i = 0; i<MAX_CONSOLE_COUNT; i++)
                    	{
                    		if (mp_VCon[i] == NULL || mp_VCon[i]->RCon() == NULL) continue;
                    		dwFarPID = mp_VCon[i]->RCon()->GetFarPID();
                    		if (!dwFarPID) continue;
                    		
                    		if (dwPID == dwFarPID || dwParentPID == dwFarPID) // MSDN Topics
                    		{
                    			apiSetForegroundWindow(hWnd);
                    			break;
                    		}
                    	}
                    }
                
                    //#ifdef _DEBUG
                    //wchar_t szTitle[255], szClass[255], szMsg[1024];
                    //GetWindowText(hWnd, szTitle, 254); GetClassName(hWnd, szClass, 254);
                    //wsprintf(szMsg, L"Window was created:\nTitle: %s\nClass: %s\nPID: %i", szTitle, szClass, dwPID);
                    //MBox(szMsg);
                    //#endif
                }
            }
        }
        break;
#ifdef _DEBUG
    case HSHELL_ACTIVATESHELLWINDOW:
    	{	// Не вызывается
    		#ifdef _DEBUG
    		wchar_t szDbg[128]; wsprintf(szDbg, L"HSHELL_ACTIVATESHELLWINDOW(lParam=0x%08X)\n", (DWORD)lParam);
    		DEBUGSTRFOREGROUND(szDbg);
    		#endif
    	}
    	break;
#endif
	case HSHELL_WINDOWACTIVATED:
		{	// Приходит позже чем WM_ACTIVATE(WA_INACTIVE), но пригодится, если CE был НЕ в фокусе

			#ifdef _DEBUG
			// Когда активируется Desktop - lParam == 0
			wchar_t szDbg[128], szClass[64]; if (!lParam || !GetClassName((HWND)lParam, szClass, 63)) wcscpy(szClass, L"<NULL>");
			BOOL lbLBtn = isPressed(VK_LBUTTON);
			wsprintf(szDbg, L"HSHELL_WINDOWACTIVATED(lParam=0x%08X, %s, %i)\n", (DWORD)lParam, szClass, lbLBtn);
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
    CheckMenuItem(GetSystemMenu(ghWnd, FALSE), ID_ALWAYSONTOP, MF_BYCOMMAND |
        (gSet.isAlwaysOnTop ? MF_CHECKED : MF_UNCHECKED));
	SetWindowPos(ghWnd, (gSet.isAlwaysOnTop || gSet.isDesktopMode) ? HWND_TOPMOST : HWND_NOTOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
	if (ghOpWnd && gSet.isAlwaysOnTop)
	{
		SetWindowPos(ghOpWnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
		apiSetForegroundWindow(ghOpWnd);
	}
}

void CConEmuMain::OnDesktopMode()
{
	if (!this) return;
	
#ifndef CHILD_DESK_MODE
	DWORD dwStyleEx = GetWindowLong(ghWnd, GWL_EXSTYLE);
	DWORD dwNewStyleEx = dwStyleEx;
	DWORD dwStyle = GetWindowLong(ghWnd, GWL_STYLE);
	DWORD dwNewStyle = dwStyle;

	if (gSet.isDesktopMode)
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
	//HWND hParent = NULL;gSet.isDesktopMode ?  : GetDesktopWindow();
	
	if (gSet.isDesktopMode)
	{
		// Shell windows is FindWindowEx(hDesktop, NULL, L"Progman", L"Program Manager");
		HWND hShellWnd = GetShellWindow();
		DWORD dwShellPID = 0;
		if (hShellWnd)
			GetWindowThreadProcessId(hShellWnd, &dwShellPID);
		// But in Win7 it is not a real desktop holder :(
		if (gOSVer.dwMajorVersion >= 6) // Vista too?
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

		if (gSet.isFullScreen) // этот режим с Desktop несовместим
			SetWindowMode(rMaximized);

		if (!hShellWnd)
		{
			gSet.isDesktopMode = false;
			if (ghOpWnd && gSet.hExt)
				CheckDlgButton(gSet.hExt, cbDesktopMode, BST_UNCHECKED);
		}
		else
		{
			mh_ShellWindow = hShellWnd;
			GetWindowThreadProcessId(mh_ShellWindow, &mn_ShellWindowPID);
			RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);
			MapWindowPoints(NULL, mh_ShellWindow, (LPPOINT)&rcWnd, 2);
			//apiShowWindow(ghWnd, SW_HIDE);
			//SetWindowPos(ghWnd, NULL, rcWnd.left,rcWnd.top,0,0, SWP_NOSIZE|SWP_NOZORDER);
			SetParent(ghWnd, mh_ShellWindow);
			SetWindowPos(ghWnd, NULL, rcWnd.left,rcWnd.top,0,0, SWP_NOSIZE|SWP_NOZORDER);
			SetWindowPos(ghWnd, HWND_TOPMOST, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE);
			//apiShowWindow(ghWnd, SW_SHOW);
			#ifdef _DEBUG
			RECT rcNow; GetWindowRect(ghWnd, &rcNow);
			#endif
		}
	}

	if (!gSet.isDesktopMode)
	{

		//dwStyle |= WS_POPUP;
		RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);
		RECT rcVirtual = GetVirtualScreenRect(TRUE);
		
		SetWindowPos(ghWnd, NULL, max(rcWnd.left,rcVirtual.left),max(rcWnd.top,rcVirtual.top),0,0, SWP_NOSIZE|SWP_NOZORDER);

		SetParent(ghWnd, hDesktop);

		SetWindowPos(ghWnd, NULL, max(rcWnd.left,rcVirtual.left),max(rcWnd.top,rcVirtual.top),0,0, SWP_NOSIZE|SWP_NOZORDER);
		
		OnAlwaysOnTop();
		if (ghOpWnd && !gSet.isAlwaysOnTop)
			apiSetForegroundWindow(ghOpWnd);
	}
	
	//SetWindowLong(ghWnd, GWL_STYLE, dwStyle);
#endif
}

LRESULT CConEmuMain::OnSysCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	#ifdef _DEBUG
	wchar_t szDbg[128]; wsprintf(szDbg, L"OnSysCommand (%i(0x%X), %i)\n", wParam, wParam, lParam);
	DEBUGSTRSIZE(szDbg);
	#endif

    LRESULT result = 0;
    switch(LOWORD(wParam))
    {
    case ID_SETTINGS:
        if (ghOpWnd && IsWindow(ghOpWnd))
        {
            if (!apiShowWindow ( ghOpWnd, SW_SHOWNORMAL ))
                DisplayLastError(L"Can't show settings window");
            SetFocus ( ghOpWnd );
            break; // А то открывались несколько окон диалогов :)
        }
        //DialogBox((HINSTANCE)GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SPG_MAIN), 0, CSettings::wndOpProc);
        CSettings::Dialog();
        return 0;
        //break;
    case ID_CON_PASTE:
        mp_VActive->RCon()->Paste();
        return 0;
    case ID_CON_COPY:
        mp_VActive->RCon()->DoSelectionCopy();
        return 0;
    case ID_CON_MARKBLOCK:
    case ID_CON_MARKTEXT:
    	mp_VActive->RCon()->StartSelection(LOWORD(wParam) == ID_CON_MARKTEXT);
        return 0;
    case ID_AUTOSCROLL:
        gSet.AutoScroll = !gSet.AutoScroll;
        CheckMenuItem(GetSystemMenu(ghWnd, FALSE), ID_AUTOSCROLL, MF_BYCOMMAND |
            (gSet.AutoScroll ? MF_CHECKED : MF_UNCHECKED));
        return 0;
    case ID_ALWAYSONTOP:
    	gSet.isAlwaysOnTop = !gSet.isAlwaysOnTop;
    	OnAlwaysOnTop();
    	if (ghOpWnd && gSet.hExt)
		{
    		CheckDlgButton(gSet.hExt, cbAlwaysOnTop, gSet.isAlwaysOnTop ? BST_CHECKED : BST_UNCHECKED);
    	}
        return 0;
    case ID_DUMPCONSOLE:
        if (mp_VActive)
            mp_VActive->DumpConsole();
        return 0;
        //break;
    case ID_DEBUGGUI:
    	StartDebugLogConsole();
    	return 0;
    case ID_MONITOR_SHELLACTIVITY:
    	{
    		CRealConsole* pRCon = mp_VActive->RCon();
    		if (pRCon)
    			pRCon->LogShellStartStop();
	    	//if (!mb_CreateProcessLogged)
	    	//	StartLogCreateProcess();
	    	//else
	    	//	StopLogCreateProcess();
		}
    	return 0;
    case ID_DEBUG_SHOWRECTS:
    	gbDebugShowRects = !gbDebugShowRects;
    	InvalidateAll();
    	return 0;
    case ID_CON_TOGGLE_VISIBLE:
        if (mp_VActive)
            mp_VActive->RCon()->ShowConsole(-1); // Toggle visibility
        return 0;
    case ID_HELP:
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
    	return 0;
    case ID_ABOUT:
        {
            WCHAR szTitle[255];
			const wchar_t *pszBits =
			#ifdef WIN64
				L"x64"
			#else
				L"x86"
			#endif
				;

            #ifdef _DEBUG
            wsprintf(szTitle, L"About ConEmu (%s [DEBUG] %s)", szConEmuVersion, pszBits);
            #else
            wsprintf(szTitle, L"About ConEmu (%s %s)", szConEmuVersion, pszBits);
            #endif

            BOOL b = gbDontEnable; gbDontEnable = TRUE;
            MSGBOXPARAMS mb = {sizeof(MSGBOXPARAMS), ghWnd, g_hInstance, pHelp, szTitle, 
            	MB_USERICON, MAKEINTRESOURCE(IMAGE_ICON), NULL, NULL, LANG_NEUTRAL};
            MessageBoxIndirectW(&mb);
            //MessageBoxW(ghWnd, pHelp, szTitle, MB_ICONQUESTION);
            gbDontEnable = b;
        }
        return 0;
        //break;
    case ID_TOTRAY:
    	if (IsWindowVisible(ghWnd))
        	Icon.HideWindowToTray();
        else
        	Icon.RestoreWindowFromTray();
        return 0;
        //break;
    case ID_CONPROP:
        #ifdef MSGLOGGER
        {
            HMENU hMenu = GetSystemMenu(ghConWnd, FALSE);
            MENUITEMINFO mii; TCHAR szText[255];
            for (int i=0; i<15; i++)
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
                        for (int i=0; i<15; i++) {
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
                } else
				break;
            }
        }
        #endif
        POSTMESSAGE(ghConWnd, WM_SYSCOMMAND, SC_PROPERTIES_SECRET/*65527*/, 0, TRUE);
        return 0;
        //break;
    }

    switch(wParam)
    {
    case SC_MAXIMIZE_SECRET:
        SetWindowMode(rMaximized);
        break;
    case SC_RESTORE_SECRET:
        SetWindowMode(rNormal);
        break;
    case SC_CLOSE:
        //Icon.Delete();
        //SENDMESSAGE(ghConWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
        //SENDMESSAGE(ghConWnd ? ghConWnd : ghWnd, WM_CLOSE, 0, 0); // ?? фар не ловит сообщение, ExitFAR не вызываются
        {
            int nConCount = 0, nDetachedCount = 0;
			//int nEditors = 0, nProgress = 0, i;
			//for (i=(MAX_CONSOLE_COUNT-1); i>=0; i--) {
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
			//	lstrcpy(szText, L"Close confirmation.\r\n\r\n"); pszText = szText+lstrlen(szText);
			//	if (nProgress) { wsprintf(pszText, L"Incomplete operations: %i\r\n", nProgress); pszText += lstrlen(pszText); }
			//	if (nEditors) { wsprintf(pszText, L"Unsaved editor windows: %i\r\n", nEditors); pszText += lstrlen(pszText); }
			//	lstrcpy(pszText, L"\r\nProceed with shutdown?");
			//	int nBtn = MessageBoxW(ghWnd, szText, L"ConEmu", MB_OKCANCEL|MB_ICONEXCLAMATION);
			//	if (nBtn != IDOK)
			//		return 0; // не закрывать
			//}
			if (!gConEmu.OnCloseQuery())
				return 0; // не закрывать

            for (int i=(MAX_CONSOLE_COUNT-1); i>=0; i--)
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
                        mp_VCon[i]->RCon()->CloseConsole();
                    }
                }
            }

			gSet.SaveSizePosOnExit();

            if (nConCount == 0)
            {
                if (nDetachedCount > 0)
                {
                    if (MessageBox(ghWnd, L"ConEmu is waiting for console attach.\nIt was started in 'Detached' mode.\nDo You want to cancel waiting?",
                        L"ConEmu", MB_YESNO|MB_ICONQUESTION) != IDYES)
                    return result;
                }
                Destroy();
            }
        }
        break;

    case SC_MAXIMIZE:
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
    case SC_RESTORE:
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
    case SC_MINIMIZE:
		DEBUGSTRSYS(L"OnSysCommand(SC_MINIMIZE)\n");
        if (gSet.isMinToTray)
        {
            Icon.HideWindowToTray();
            break;
        }
        //if (gSet.isDontMinimize) {
        //	SetWindowPos(ghWnd, HWND_BOTTOM, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
        //	break;
        //}
        result = DefWindowProc(hWnd, WM_SYSCOMMAND, wParam, lParam);
        break;

    default:
        if (wParam != 0xF100)
        {
			#ifdef _DEBUG
			wchar_t szDbg[64]; wsprintf(szDbg, L"OnSysCommand(%i)\n", wParam);
			DEBUGSTRSYS(szDbg);
			#endif
            // иначе это приводит к потере фокуса и активации невидимой консоли,
            // перехвате стрелок клавиатуры, и прочей фигни...
            if (wParam<0xF000)
            {
                POSTMESSAGE(ghConWnd, WM_SYSCOMMAND, wParam, lParam, FALSE);
            }
            result = DefWindowProc(hWnd, WM_SYSCOMMAND, wParam, lParam);
        }
    }
    return result;
}

WARNING("Частота хождения таймера в винде оставляет желать... нужно от него избавляться и по возможности переносить все в нити");
LRESULT CConEmuMain::OnTimer(WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;
    
    //if (mb_InTimer) return 0; // чтобы ненароком два раза в одно событие не вошел (хотя не должен)
    mb_InTimer = TRUE;
    //result = gConEmu.OnTimer(wParam, lParam);

#ifdef DEBUGSHOWFOCUS
	HWND hFocus = GetFocus();
	HWND hFore = GetForegroundWindow();
	static HWND shFocus, shFore;
	static wchar_t szWndInfo[1200];
	if (hFocus != shFocus)
	{
		wsprintf(szWndInfo, L"(Fore=0x%08X) Focus was changed to ", (DWORD)hFore);
		getWindowInfo(hFocus, szWndInfo+wcslen(szWndInfo));
		wcscat(szWndInfo, L"\n");
		DEBUGSHOWFOCUS(szWndInfo);
		shFocus = hFocus;
	}
	if (hFore != shFore)
	{
		if (hFore != hFocus)
		{
			wcscpy(szWndInfo, L"Foreground window was changed to ");
			getWindowInfo(hFore, szWndInfo+wcslen(szWndInfo));
			wcscat(szWndInfo, L"\n");
			DEBUGSHOWFOCUS(szWndInfo);
		}
		shFore = hFore;
	}
#endif

    switch (wParam)
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
	                Destroy();
	                break;
	            }
	        }
	        else if (!mb_ProcessCreated && m_ProcCount>=1)
	        {
	            if ((GetTickCount() - mn_StartTick)>PROCESS_WAIT_START_TIME)
	                mb_ProcessCreated = TRUE;
	        }


	        // TODO: поддержку SlideShow повесить на отдельный таймер
	        BOOL lbIsPicView = isPictureView();
	        if (bPicViewSlideShow)
	        {
	            DWORD dwTicks = GetTickCount();
	            DWORD dwElapse = dwTicks - dwLastSlideShowTick;
	            if (dwElapse > gSet.nSlideShowElapse)
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
	        {   // После скрытия/закрытия PictureView нужно передернуть консоль - ее размер мог измениться
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
	            mp_VActive->RCon()->UpdateScrollInfo();
	        }

			// режим полного скрытия заголовка
			if (gSet.isHideCaptionAlways())
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
					if (!isIconic() && !isZoomed() && !gSet.isFullScreen)
					{
						TODO("Не наколоться бы с предыдущим статусом при ресайзе?");
						//static bool bPrevForceShow = false;
						BOOL bCurForceShow = isMouseOverFrame(true);
						if (bCurForceShow != mb_CaptionWasRestored)
						{
							mb_CaptionWasRestored = bCurForceShow;
							//if (gSet.nHideCaptionAlwaysDelay && bCurForceShow) {
							KillTimer(ghWnd, TIMER_CAPTION_APPEAR_ID); KillTimer(ghWnd, TIMER_CAPTION_DISAPPEAR_ID);
							WORD nID = bCurForceShow ? TIMER_CAPTION_APPEAR_ID : TIMER_CAPTION_DISAPPEAR_ID;
							DWORD nDelay = bCurForceShow ? gSet.nHideCaptionAlwaysDelay : gSet.nHideCaptionAlwaysDisappear;
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
				bool bNewFade = (gSet.isFadeInactive && !bForeground && !lbIsPicView);
				if (bLastFade != bNewFade)
				{
					mp_VActive->mb_LastFadeFlag = bNewFade;
					m_Child->Invalidate();
				}
			}

			if (mh_ConEmuAliveEvent && !mb_ConEmuAliveOwned)
				isFirstInstance(); // Заодно и проверит...

			// Если был изменен файл background
			if (gSet.PollBackgroundFile())
			{
				gConEmu.Update(true);
			}

			CheckFocus(L"TIMER_MAIN_ID");

	    } break; // case 0:

	case TIMER_CONREDRAW_ID: // Период: CON_REDRAW_TIMOUT*2
		{
	        if (!isIconic())
	        {
	        	m_Child->CheckPostRedraw();
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
			RightClickingPaint();
		} break;
    }

    mb_InTimer = FALSE;

    return result;
}

void CConEmuMain::OnTransparent()
{
	TODO("CConEmuMain::OnTransparentColorKey()");
	
	BOOL bNeedRedrawOp = FALSE;
	UINT nTransparent = max(MIN_ALPHA_VALUE,gSet.nTransparent);
	DWORD dwExStyle = GetWindowLongPtr(ghWnd, GWL_EXSTYLE);
	if (nTransparent == 255 /*&& !gSet.isColorKey*/)
	{
		// Прозрачность отключается (полностью непрозрачный)
		//SetLayeredWindowAttributes(ghWnd, 0, 255, LWA_ALPHA);
		if ((dwExStyle & WS_EX_LAYERED) == WS_EX_LAYERED)
		{
			dwExStyle &= ~WS_EX_LAYERED;
			SetLayeredWindowAttributes(ghWnd, 0, 255, LWA_ALPHA);
			SetWindowLongPtr(ghWnd, GWL_EXSTYLE, dwExStyle);
		}
	}
	else
	{
		if ((dwExStyle & WS_EX_LAYERED) == 0)
		{
			dwExStyle |= WS_EX_LAYERED;
			SetWindowLongPtr(ghWnd, GWL_EXSTYLE, dwExStyle);
			bNeedRedrawOp = TRUE;
		}
		
		SetLayeredWindowAttributes(ghWnd,
			0/*gSet.ColorKey*/, nTransparent, 
			((nTransparent<255) ? LWA_ALPHA : 0) /*| (gSet.isColorKey ? LWA_COLORKEY : 0)*/);
		
		if (bNeedRedrawOp && ghOpWnd)
		{
			// Ask the window and its children to repaint
			RedrawWindow(ghOpWnd, 
			             NULL, 
			             NULL, 
			             RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
		}
	}
	
	OnSetCursor();
}

// Вызовем UpdateScrollPos для АКТИВНОЙ консоли
LRESULT CConEmuMain::OnUpdateScrollInfo(BOOL abPosted/* = FALSE*/)
{
    if (!abPosted)
	{
        PostMessage(ghWnd, mn_MsgUpdateScrollInfo, 0, (LPARAM)mp_VCon);
        return 0;
    }

    if (!mp_VActive)
        return 0;
    mp_VActive->RCon()->UpdateScrollInfo();
    return 0;
}

// Чтобы при создании ПЕРВОЙ консоли на экране сразу можно было что-то нарисовать
void CConEmuMain::OnVConCreated(CVirtualConsole* apVCon)
{
	if (!mp_VActive || mb_CreatingActive)
		mp_VActive = apVCon;
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

    for (int i=0; i<MAX_CONSOLE_COUNT; i++)
    {
        if (mp_VCon[i] == apVCon)
        {

			// Сначала нужно обновить закладки, иначе в закрываемой консоли
			// может быть несколько вкладок и вместо активации другой консоли
			// будет попытка активировать другую вкладку закрываемой консоли
			//gConEmu.mp_TabBar->Update(TRUE); -- а и не сможет он другую активировать, т.к. RCon вернет FALSE

			// Эта комбинация должна активировать предыдущую консоль (если активна текущая)
			if (gSet.isTabRecent && apVCon == mp_VActive)
			{
				if (gConEmu.GetVCon(1)) {
					gConEmu.mp_TabBar->SwitchRollback();
					gConEmu.mp_TabBar->SwitchNext();
					gConEmu.mp_TabBar->SwitchCommit();
				}
			}

			// Теперь можно очистить переменную массива
			mp_VCon[i] = NULL;


            WARNING("Вообще-то это нужно бы в CriticalSection закрыть. Несколько консолей может одновременно закрыться");
            if (mp_VActive == apVCon)
            {
                for (int j=(i-1); j>=0; j--)
                {
                    if (mp_VCon[j])
                    {
                        ConActivate(j);
                        break;
                    }
                }
                if (mp_VActive == apVCon)
                {
                    for (int j=(i+1); j<MAX_CONSOLE_COUNT; j++)
                    {
                        if (mp_VCon[j])
                        {
                            ConActivate(j);
                            break;
                        }
                    }
                }
            }
            for (int j=(i+1); j<MAX_CONSOLE_COUNT; j++)
            {
                mp_VCon[j-1] = mp_VCon[j];
            }
			mp_VCon[MAX_CONSOLE_COUNT-1] = NULL;
            if (mp_VActive == apVCon)
                mp_VActive = NULL;
            delete apVCon;
            break;
        }
    }
    // Теперь перетряхнуть заголовок (табы могут быть отключены и в заголовке отображается количество консолей)
    UpdateTitle(); // сам перечитает
    //
    gConEmu.mp_TabBar->Update(); // Иначе не будет обновлены закладки
	// А теперь можно обновить активную закладку
    gConEmu.mp_TabBar->OnConsoleActivated(ActiveConNum()+1/*, FALSE*/);
    return 0;
}

DWORD CConEmuMain::GuiServerThread(LPVOID lpvParam)
{ 
    BOOL fConnected = FALSE;
    DWORD dwErr = 0;
    HANDLE hPipe = NULL; 
    #ifdef _DEBUG
    DWORD dwTID = GetCurrentThreadId();
    #endif
    wchar_t szServerPipe[MAX_PATH];

	MCHKHEAP;

    _ASSERTE(ghWnd!=NULL);
    wsprintf(szServerPipe, CEGUIPIPENAME, L".", (DWORD)ghWnd);

    // The main loop creates an instance of the named pipe and 
    // then waits for a client to connect to it. When the client 
    // connects, a thread is created to handle communications 
    // with that client, and the loop is repeated.
    
	MCHKHEAP;

    // Пока окно не закрыто
    do {
        while (!fConnected)
        { 
            _ASSERTE(hPipe == NULL);

            hPipe = CreateNamedPipe( 
                szServerPipe,             // pipe name 
                PIPE_ACCESS_DUPLEX,       // read/write access 
                PIPE_TYPE_MESSAGE |       // message type pipe 
                PIPE_READMODE_MESSAGE |   // message-read mode 
                PIPE_WAIT,                // blocking mode 
                PIPE_UNLIMITED_INSTANCES, // max. instances  
                PIPEBUFSIZE,              // output buffer size 
                PIPEBUFSIZE,              // input buffer size 
                0,                        // client time-out 
                gpNullSecurity);          // default security attribute 

            _ASSERTE(hPipe != INVALID_HANDLE_VALUE);

            if (hPipe == INVALID_HANDLE_VALUE) 
            {
                //DisplayLastError(L"CreateNamedPipe failed"); 
                hPipe = NULL;
                Sleep(50);
                continue;
            }

			MCHKHEAP;

            // Wait for the client to connect; if it succeeds, 
            // the function returns a nonzero value. If the function
            // returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 

            fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : ((dwErr = GetLastError()) == ERROR_PIPE_CONNECTED); 

            if ((dwErr = WaitForSingleObject ( gConEmu.mh_GuiServerThreadTerminate, 0 )) == WAIT_OBJECT_0)
            {
                SafeCloseHandle(hPipe);
                return 0; // GUI закрывается
            }
            
            if (!fConnected)
                SafeCloseHandle(hPipe);
        }

        if (fConnected)
        {
            // сразу сбросим, чтобы не забыть
            fConnected = FALSE;

            gConEmu.GuiServerThreadCommand ( hPipe ); // При необходимости - записывает в пайп результат сама
        }

        FlushFileBuffers(hPipe); 
        //DisconnectNamedPipe(hPipe); 
        SafeCloseHandle(hPipe);
    } // Перейти к открытию нового instance пайпа
    while (WaitForSingleObject ( gConEmu.mh_GuiServerThreadTerminate, 0 ) != WAIT_OBJECT_0);

    return 0; 
}

// Эта функция пайп не закрывает!
void CConEmuMain::GuiServerThreadCommand(HANDLE hPipe)
{
    CESERVER_REQ in={{0}}, *pIn=NULL;
    DWORD cbRead = 0, cbWritten = 0, dwErr = 0;
    BOOL fSuccess = FALSE;

    // Send a message to the pipe server and read the response. 
    fSuccess = ReadFile( 
        hPipe,            // pipe handle 
        &in,              // buffer to receive reply
        sizeof(in),       // size of read buffer
        &cbRead,          // bytes read
        NULL);            // not overlapped 

    if (!fSuccess && ((dwErr = GetLastError()) != ERROR_MORE_DATA)) 
    {
        _ASSERTE("ReadFile(pipe) failed"==NULL || (dwErr==ERROR_BROKEN_PIPE && cbRead==0));
        //CloseHandle(hPipe);
        return;
    }
    if (in.hdr.nVersion != CESERVER_REQ_VER)
    {
        gConEmu.ShowOldCmdVersion(in.hdr.nCmd, in.hdr.nVersion, -1);
        return;
    }
    _ASSERTE(in.hdr.cbSize>=sizeof(CESERVER_REQ_HDR) && cbRead>=sizeof(CESERVER_REQ_HDR));
    if (cbRead < sizeof(CESERVER_REQ_HDR) || /*in.hdr.cbSize < cbRead ||*/ in.hdr.nVersion != CESERVER_REQ_VER)
    {
        //CloseHandle(hPipe);
        return;
    }

    if (in.hdr.cbSize <= cbRead)
    {
        pIn = &in; // выделение памяти не требуется
    }
    else
    {
        int nAllSize = in.hdr.cbSize;
        pIn = (CESERVER_REQ*)calloc(nAllSize,1);
        _ASSERTE(pIn!=NULL);
        memmove(pIn, &in, cbRead);
        _ASSERTE(pIn->hdr.nVersion==CESERVER_REQ_VER);

        LPBYTE ptrData = ((LPBYTE)pIn)+cbRead;
        nAllSize -= cbRead;

        while(nAllSize>0)
        { 
            //_tprintf(TEXT("%s\n"), chReadBuf);

            // Break if TransactNamedPipe or ReadFile is successful
            if(fSuccess)
                break;

            // Read from the pipe if there is more data in the message.
            fSuccess = ReadFile( 
                hPipe,      // pipe handle 
                ptrData,    // buffer to receive reply 
                nAllSize,   // size of buffer 
                &cbRead,    // number of bytes read 
                NULL);      // not overlapped 

            // Exit if an error other than ERROR_MORE_DATA occurs.
            if( !fSuccess && ((dwErr = GetLastError()) != ERROR_MORE_DATA)) 
                break;
            ptrData += cbRead;
            nAllSize -= cbRead;
        }

        TODO("Может возникнуть ASSERT, если консоль была закрыта в процессе чтения");
        _ASSERTE(nAllSize==0);
        if (nAllSize>0)
        {
            //CloseHandle(hPipe);
            return; // удалось считать не все данные
        }
    }

    #ifdef _DEBUG
    UINT nDataSize = pIn->hdr.cbSize - sizeof(CESERVER_REQ_HDR);
    #endif
    // Все данные из пайпа получены, обрабатываем команду и возвращаем (если нужно) результат
    
    if (pIn->hdr.nCmd == CECMD_NEWCMD)
    {
    	// Приходит из другой копии ConEmu.exe, когда она запущена с ключом /single
        DEBUGSTR(L"GUI recieved CECMD_NEWCMD\n");
    
        pIn->Data[0] = FALSE;
        pIn->hdr.cbSize = sizeof(CESERVER_REQ_HDR) + 1;

        if (isIconic())
            SendMessage(ghWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
        apiSetForegroundWindow(ghWnd);

		RConStartArgs args; args.pszSpecialCmd = pIn->NewCmd.szCommand;
        CVirtualConsole* pCon = CreateCon(&args);
		args.pszSpecialCmd = NULL;
        if (pCon)
        {
            pIn->Data[0] = TRUE;
        }
        
        // Отправляем
        fSuccess = WriteFile( 
            hPipe,        // handle to pipe 
            pIn,         // buffer to write from 
            pIn->hdr.cbSize,  // number of bytes to write 
            &cbWritten,   // number of bytes written 
            NULL);        // not overlapped I/O 

    }
    else if (pIn->hdr.nCmd == CECMD_TABSCMD)
    {
        // 0: спрятать/показать табы, 1: перейти на следующую, 2: перейти на предыдущую, 3: commit switch
        DEBUGSTR(L"GUI recieved CECMD_TABSCMD\n");
        _ASSERTE(nDataSize>=1);
        DWORD nTabCmd = pIn->Data[0];
        TabCommand(nTabCmd);

	}
	else if (pIn->hdr.nCmd == CECMD_ATTACH2GUI)
	{
		// Получен запрос на Attach из сервера
		if (AttachRequested(pIn->StartStop.hWnd, pIn->StartStop, &(pIn->StartStopRet)))
		{
			fSuccess = WriteFile(hPipe, pIn, pIn->hdr.cbSize, &cbWritten, NULL);
		}

	}
	else if (pIn->hdr.nCmd == CECMD_CMDSTARTSTOP)
	{
		if (pIn->dwData[0] == 1)
		{
			// Запущен процесс сервера
			HWND hConWnd = (HWND)pIn->dwData[1];
			_ASSERTE(hConWnd && IsWindow(hConWnd));
			//LRESULT l = 0;
			DWORD_PTR dwRc = 0;
			
			//2010-05-21 Поскольку это критично - лучше ждать до упора, хотя может быть DeadLock?
			//l = SendMessageTimeout(ghWnd, gConEmu.mn_MsgSrvStarted, (WPARAM)hConWnd, pIn->hdr.nSrcPID,
			//	SMTO_BLOCK, 5000, &dwRc);
			dwRc = SendMessage(ghWnd, gConEmu.mn_MsgSrvStarted, (WPARAM)hConWnd, pIn->hdr.nSrcPID);

			pIn->dwData[0] = (DWORD)ghWnd;
			pIn->dwData[1] = (DWORD)ghWndDC;
			
			//pIn->dwData[0] = (l == 0) ? 0 : 1;
		}
		else if (pIn->dwData[0] == 101)
		{
			// Процесс сервера завершается
			CRealConsole* pRCon = NULL;
			for (int i = 0; i < MAX_CONSOLE_COUNT; i++)
			{
				if (mp_VCon[i] && mp_VCon[i]->RCon() && mp_VCon[i]->RCon()->GetServerPID() == pIn->hdr.nSrcPID)
				{
					pRCon = mp_VCon[i]->RCon();
					break;
				}
			}
			if (pRCon)
				pRCon->OnServerClosing(pIn->hdr.nSrcPID);
			pIn->dwData[0] = 1;
		}
		else
		{
			pIn->dwData[0] = 0;
		}
		pIn->hdr.cbSize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD);
        // Отправляем
        fSuccess = WriteFile( 
            hPipe,        // handle to pipe 
            pIn,         // buffer to write from 
            pIn->hdr.cbSize,  // number of bytes to write 
            &cbWritten,   // number of bytes written 
            NULL);        // not overlapped I/O 
		
	}

    // Освободить память
    if (pIn && (LPVOID)pIn != (LPVOID)&in)
    {
        free(pIn); pIn = NULL;
    }

    return;
}

//void CConEmuMain::PostSetBackground(CVirtualConsole* apVCon, CESERVER_REQ_SETBACKGROUND* apImgData)
//{
//	PostMessage(ghWnd, mn_MsgPostSetBackground, (WPARAM)apVCon, (LPARAM)apImgData);
//}

LRESULT CConEmuMain::MainWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	if (messg == WM_CREATE)
	{
		if (ghWnd == NULL)
			ghWnd = hWnd; // ставим сразу, чтобы функции могли пользоваться
		else if (ghWndDC == NULL)
			ghWndDC = hWnd; // ставим сразу, чтобы функции могли пользоваться
	}

	if (hWnd == ghWnd)
		result = gConEmu.WndProc(hWnd, messg, wParam, lParam);
	else if (hWnd == ghWndDC)
		result = gConEmu.m_Child->ChildWndProc(hWnd, messg, wParam, lParam);
	else if (messg)
		result = DefWindowProc(hWnd, messg, wParam, lParam);

	return result;
}

LRESULT CConEmuMain::WndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    //MCHKHEAP

    #ifdef _DEBUG
    wchar_t szDbg[127]; wsprintfW(szDbg, L"WndProc(%i{x%03X},%i,%i)\n", messg, messg, wParam, lParam);
    //OutputDebugStringW(szDbg);
    #endif

    if (messg == WM_SYSCHAR) // Вернул. Для пересылки в консоль не используется, но чтобы не пищало - необходимо
        return TRUE;
    //if (messg == WM_CHAR)
    //  return TRUE;

    switch (messg)
    {
	case WM_CREATE:
		result = gConEmu.OnCreate(hWnd, (LPCREATESTRUCT)lParam);
		break;

    case WM_NOTIFY:
    {
		if (gConEmu.mp_TabBar)
			result = gConEmu.mp_TabBar->OnNotify((LPNMHDR)lParam);
        break;
    }

    case WM_COMMAND:
    {
		if (gConEmu.mp_TabBar)
			gConEmu.mp_TabBar->OnCommand(wParam, lParam);
        result = 0;
        break;
    }
    
    case WM_INITMENUPOPUP:
    {
    	return gConEmu.OnInitMenuPopup(hWnd, (HMENU)wParam, lParam);
    }
    
    case WM_ERASEBKGND:
        //return 0;
        return 1; //2010-10-05
        
    case WM_NCPAINT:
    case WM_NCACTIVATE:
    case WM_NCCALCSIZE:
    case WM_NCHITTEST:
    case 0x31E: // WM_DWMCOMPOSITIONCHANGED:
    case 0xAE: // WM_NCUAHDRAWCAPTION:
    case 0xAF: // WM_NCUAHDRAWFRAME:
        //result = gConEmu.OnNcPaint((HRGN)wParam);
        result = gConEmu.OnNcMessage(hWnd, messg, wParam, lParam);
        break;

    case WM_PAINT:
        result = gConEmu.OnPaint(wParam, lParam);
        break;
    
    case WM_TIMER:
		result = gConEmu.OnTimer(wParam, lParam);
        break;

    case WM_SIZING:
		{
			RECT* pRc = (RECT*)lParam;
			wchar_t szDbg[128]; wsprintf(szDbg, L"WM_SIZING (Edge%i, {%i-%i}-{%i-%i})\n", wParam, pRc->left, pRc->top, pRc->right, pRc->bottom);
			DEBUGSTRSIZE(szDbg);

			if (!isIconic())
				result = gConEmu.OnSizing(wParam, lParam);
		} break;


#ifdef _DEBUG
	case WM_SHOWWINDOW:
		{
			wchar_t szDbg[128]; wsprintf(szDbg, L"WM_SHOWWINDOW (Show=%i, Status=%i)\n", wParam, lParam);
			DEBUGSTRSIZE(szDbg);
			result = DefWindowProc(hWnd, messg, wParam, lParam);
		} break;
#endif

    case WM_SIZE:
		{
			#ifdef _DEBUG
			DWORD dwStyle = GetWindowLong(ghWnd, GWL_STYLE);
			wchar_t szDbg[128]; wsprintf(szDbg, L"WM_SIZE (Type:%i, {%i-%i}) style=0x%08X\n", wParam, LOWORD(lParam), HIWORD(lParam), dwStyle);
			DEBUGSTRSIZE(szDbg);
			#endif
			//if (gSet.isDontMinimize && wParam == SIZE_MINIMIZED) {
			//	result = 0;
			//	break;
			//}
			if (!isIconic())
				result = gConEmu.OnSize(wParam, LOWORD(lParam), HIWORD(lParam));
		} break;

    case WM_MOVE:
		{
			#ifdef _DEBUG
			wchar_t szDbg[128]; wsprintf(szDbg, L"WM_MOVE ({%i-%i})\n", (int)(SHORT)LOWORD(lParam), (int)(SHORT)HIWORD(lParam));
			DEBUGSTRSIZE(szDbg);
			#endif
		} break;

	case WM_WINDOWPOSCHANGING:
		{
			WINDOWPOS *p = (WINDOWPOS*)lParam;
			DWORD dwStyle = GetWindowLong(ghWnd, GWL_STYLE);
			#ifdef _DEBUG
			wchar_t szDbg[128]; wsprintf(szDbg, L"WM_WINDOWPOSCHANGING ({%i-%i}x{%i-%i} Flags=0x%08X) style=0x%08X\n", p->x, p->y, p->cx, p->cy, p->flags, dwStyle);
			DEBUGSTRSIZE(szDbg);

			static int cx, cy;
			if (!(p->flags & SWP_NOSIZE) && (cx != p->cx || cy != p->cy))
			{
				cx = p->cx; cy = p->cy;
			}
			#endif

			//if (gSet.isDontMinimize) {
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
			//			gConEmu.InvalidateAll();
			//		}
			//		break;
			//	}
			//}

			if (!(p->flags & SWP_NOSIZE)
				&& (hWnd == ghWnd) && !gConEmu.mb_IgnoreSizeChange 
				&& !gSet.isFullScreen && !isZoomed() && !isIconic())
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
			result = DefWindowProc(hWnd, messg, wParam, lParam);
			p = (WINDOWPOS*)lParam;
		} break;

	case WM_WINDOWPOSCHANGED:
		{
			static int WindowPosStackCount = 0;
			WINDOWPOS *p = (WINDOWPOS*)lParam;
			DWORD dwStyle = GetWindowLong(ghWnd, GWL_STYLE);

			#ifdef _DEBUG
			static int cx, cy;
			if (!(p->flags & SWP_NOSIZE) && (cx != p->cx || cy != p->cy))
			{
				cx = p->cx; cy = p->cy;
			}
			#endif

			wchar_t szDbg[128]; wsprintf(szDbg, L"WM_WINDOWPOSCHANGED ({%i-%i}x{%i-%i} Flags=0x%08X), style=0x%08X\n", p->x, p->y, p->cx, p->cy, p->flags, dwStyle);
			DEBUGSTRSIZE(szDbg);
			WindowPosStackCount++;
			if (WindowPosStackCount == 1) {
				bool bNoMove = (p->flags & SWP_NOMOVE);
				bool bNoSize = (p->flags & SWP_NOSIZE);
				if (gConEmu.CorrectWindowPos(p)) {
					MoveWindow(ghWnd, p->x, p->y, p->cx, p->cy, TRUE);
				}
			}
			// Иначе могут не вызваться события WM_SIZE/WM_MOVE
			result = DefWindowProc(hWnd, messg, wParam, lParam);
			WindowPosStackCount--;

			if (hWnd == ghWnd /*&& ghOpWnd*/) //2009-05-08 запоминать wndX/wndY всегда, а не только если окно настроек открыто
			{
				if (!gConEmu.mb_IgnoreSizeChange && !gSet.isFullScreen && !isZoomed() && !isIconic())
				{
					RECT rc; GetWindowRect(ghWnd, &rc);
					gSet.UpdatePos(rc.left, rc.top);
					if (hPictureView)
					{
						mrc_WndPosOnPicView = rc;
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
			else if (hPictureView)
			{
				GetWindowRect(ghWnd, &mrc_WndPosOnPicView);
			}
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
            result = gConEmu.OnGetMinMaxInfo(pInfo);
            break;
        }

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    //case WM_CHAR: -- убрал. Теперь пользуем ToUnicodeEx.
    //case WM_SYSCHAR:
        result = gConEmu.OnKeyboard(hWnd, messg, wParam, lParam);
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
			wchar_t szDbg[128]; wsprintf(szDbg, L"%s(%i='%c', Scan=%i, lParam=0x%08X)\n", 
				(messg == WM_CHAR) ? L"WM_CHAR" : (messg == WM_SYSCHAR) ? L"WM_SYSCHAR" : L"WM_DEADCHAR",
				wParam, (wchar_t)wParam, ((DWORD)lParam & 0xFF0000) >> 16, (DWORD)lParam);
			DEBUGSTRCHAR(szDbg);
		}
		break;
#endif

    case WM_ACTIVATE:
        #ifdef MSGLOGGER
        result = gConEmu.OnFocus(hWnd, messg, wParam, lParam);
        result = DefWindowProc(hWnd, messg, wParam, lParam);
        break;
        #endif
    case WM_ACTIVATEAPP:
        #ifdef MSGLOGGER
        result = gConEmu.OnFocus(hWnd, messg, wParam, lParam);
        result = DefWindowProc(hWnd, messg, wParam, lParam);
        break;
        #endif
    case WM_KILLFOCUS:
        #ifdef MSGLOGGER
        result = gConEmu.OnFocus(hWnd, messg, wParam, lParam);
        result = DefWindowProc(hWnd, messg, wParam, lParam);
        break;
        #endif
    case WM_SETFOCUS:
        result = gConEmu.OnFocus(hWnd, messg, wParam, lParam);
        result = DefWindowProc(hWnd, messg, wParam, lParam);
        break;

    case WM_MOUSEACTIVATE:
    	//return MA_ACTIVATEANDEAT; -- ест все подряд, а LBUTTONUP пропускает :(
		gConEmu.mouse.nSkipEvents[0] = 0;
		gConEmu.mouse.nSkipEvents[1] = 0;
		if (gConEmu.mouse.bForceSkipActivation // принудительная активация окна, лежащего на Desktop
			|| (gSet.isMouseSkipActivation && LOWORD(lParam) == HTCLIENT && GetForegroundWindow() != ghWnd))
		{
			gConEmu.mouse.bForceSkipActivation = FALSE; // Однократно

			POINT ptMouse = {0}; GetCursorPos(&ptMouse);
			RECT  rcDC = {0}; GetWindowRect(ghWndDC, &rcDC);
			if (PtInRect(&rcDC, ptMouse))
			{
            	if (HIWORD(lParam) == WM_LBUTTONDOWN)
            	{
            		gConEmu.mouse.nSkipEvents[0] = WM_LBUTTONDOWN;
            		gConEmu.mouse.nSkipEvents[1] = WM_LBUTTONUP;
            		gConEmu.mouse.nReplaceDblClk = WM_LBUTTONDBLCLK;
            	}
            	else if (HIWORD(lParam) == WM_RBUTTONDOWN)
            	{
            		gConEmu.mouse.nSkipEvents[0] = WM_RBUTTONDOWN;
            		gConEmu.mouse.nSkipEvents[1] = WM_RBUTTONUP;
            		gConEmu.mouse.nReplaceDblClk = WM_RBUTTONDBLCLK;
            	}
            	else if (HIWORD(lParam) == WM_MBUTTONDOWN)
            	{
            		gConEmu.mouse.nSkipEvents[0] = WM_MBUTTONDOWN;
            		gConEmu.mouse.nSkipEvents[1] = WM_MBUTTONUP;
            		gConEmu.mouse.nReplaceDblClk = WM_MBUTTONDBLCLK;
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
        result = gConEmu.OnMouse(hWnd, messg, wParam, lParam);
        break;

    case WM_CLOSE:
        result = gConEmu.OnClose(hWnd);
        break;

    case WM_SYSCOMMAND:
        result = gConEmu.OnSysCommand(hWnd, wParam, lParam);
        break;

    case WM_NCLBUTTONDOWN:
        // При ресайзе WM_NCLBUTTONUP к сожалению не приходит
        gConEmu.mouse.state |= MOUSE_SIZING_BEGIN;
		if (gSet.isHideCaptionAlways())
		{
			KillTimer(ghWnd, TIMER_CAPTION_DISAPPEAR_ID);
			gConEmu.OnTimer(TIMER_CAPTION_APPEAR_ID,0);
			UpdateWindowRgn();
		}
        result = DefWindowProc(hWnd, messg, wParam, lParam);
        break;

    case WM_NCLBUTTONDBLCLK:
        gConEmu.mouse.state |= MOUSE_SIZING_DBLCKL; // чтобы в консоль не провалился LBtnUp если окошко развернется
        if (gConEmu.OnMouse_NCBtnDblClk(hWnd, messg, wParam, lParam))
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
		if (wParam == 0x201)
		{
			gConEmu.CtrlWinAltSpace();
		}
		else if (gConEmu.mn_MinRestoreRegistered && wParam == gConEmu.mn_MinRestoreRegistered)
		{
			gConEmu.OnMinimizeRestore();
		}
		return 0;

	case WM_SETCURSOR:
		result = gConEmu.OnSetCursor(wParam, lParam);
		if (!result)
			result = DefWindowProc(hWnd, messg, wParam, lParam);
		MCHKHEAP;
		// If an application processes this message, it should return TRUE to halt further processing or FALSE to continue.
		return result;

    case WM_DESTROY:
        result = gConEmu.OnDestroy(hWnd);
        break;
    
    case WM_IME_NOTIFY:
        break;
    case WM_INPUTLANGCHANGE:
    case WM_INPUTLANGCHANGEREQUEST:
        if (hWnd == ghWnd)
            result = gConEmu.OnLangChange(messg, wParam, lParam);
        else
            break;
        break;

	//case WM_NCHITTEST:
	//	{
	//		/*result = -1;
	//		if (gSet.isHideCaptionAlways && gSet.isTabs) {
	//			if (gConEmu.mp_TabBar->IsShown()) {
	//				HWND hTabBar = gConEmu.mp_TabBar->GetTabbar();
	//				RECT rcWnd; GetWindowRect(hTabBar, &rcWnd);
	//				TCHITTESTINFO tch = {{LOWORD(lParam),HIWORD(lParam)}};
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
	//		if (gSet.isHideCaptionAlways() && !gConEmu.mp_TabBar->IsShown() && result == HTTOP)
	//			result = HTCAPTION;
	//	} break;
        
    default:
        if (messg == gConEmu.mn_MsgPostCreate)
		{
            gConEmu.PostCreate(TRUE);
            return 0;
        }
		else if (messg == gConEmu.mn_MsgPostCopy)
		{
            gConEmu.PostCopy((wchar_t*)lParam, TRUE);
            return 0;
        }
		else if (messg == gConEmu.mn_MsgMyDestroy)
		{
            gConEmu.OnDestroy(hWnd);
            return 0;
        }
		else if (messg == gConEmu.mn_MsgUpdateSizes)
		{
            gConEmu.UpdateSizes();
            return 0;
		}
		else if (messg == gConEmu.mn_MsgUpdateCursorInfo)
		{
			COORD cr; cr.X = LOWORD(wParam); cr.Y = HIWORD(wParam);
			gConEmu.UpdateCursorInfo(cr);
			return 0;
        }
		else if (messg == gConEmu.mn_MsgSetWindowMode)
		{
            gConEmu.SetWindowMode(wParam);
            return 0;
        }
		else if (messg == gConEmu.mn_MsgUpdateTitle)
		{
            //gConEmu.UpdateTitle(TitleCmp);
            gConEmu.UpdateTitle(/*mp_VActive->RCon()->GetTitle()*/);
            return 0;
        //} else if (messg == gConEmu.mn_MsgAttach) {
        //    return gConEmu.AttachRequested ( (HWND)wParam, (DWORD)lParam );
		}
		else if (messg == gConEmu.mn_MsgSrvStarted)
		{
			gConEmu.WinEventProc(NULL, EVENT_CONSOLE_START_APPLICATION, (HWND)wParam, lParam, 0, 0, 0);
			return 0;
        }
		else if (messg == gConEmu.mn_MsgVConTerminated)
		{

			#ifdef _DEBUG
				wchar_t szDbg[200];
				lstrcpy(szDbg, L"OnVConTerminated");
				CVirtualConsole* pCon = (CVirtualConsole*)lParam;
				for (int i = 0; pCon && i < MAX_CONSOLE_COUNT; i++) {
					if (pCon == mp_VCon[i]) {
						ConEmuTab tab = {0};
						pCon->RCon()->GetTab(0, &tab);
						tab.Name[128] = 0; // чтобы не вылезло из szDbg
						wsprintf(szDbg+lstrlen(szDbg), L": #%i: %s", i+1, tab.Name);
						break;						
					}
				}
				lstrcat(szDbg, L"\n");
				DEBUGSTRCONS(szDbg);
			#endif

            return gConEmu.OnVConTerminated ( (CVirtualConsole*)lParam, TRUE );
        }
		else if (messg == gConEmu.mn_MsgUpdateScrollInfo)
		{
            return OnUpdateScrollInfo(TRUE);
        }
		else if (messg == gConEmu.mn_MsgUpdateTabs)
		{
			DEBUGSTRTABS(L"OnUpdateTabs\n");
            gConEmu.mp_TabBar->Update(TRUE);
            return 0;
        }
		else if (messg == gConEmu.mn_MsgOldCmdVer)
		{
            gConEmu.ShowOldCmdVersion(wParam & 0xFFFF, lParam, (SHORT)((wParam & 0xFFFF0000)>>16));
            return 0;
        }
		else if (messg == gConEmu.mn_MsgTabCommand)
		{
            gConEmu.TabCommand(wParam);
            return 0;
        }
		else if (messg == gConEmu.mn_MsgSheelHook)
		{
            gConEmu.OnShellHook(wParam, lParam);
            return 0;
		}
		else if (messg == gConEmu.mn_ShellExecuteEx)
		{
			return gConEmu.GuiShellExecuteEx((SHELLEXECUTEINFO*)lParam, wParam);
		}
		else if (messg == gConEmu.mn_PostConsoleResize)
		{
			gConEmu.OnConsoleResize(TRUE);
			return 0;
		}
		else if (messg == gConEmu.mn_ConsoleLangChanged)
		{
			gConEmu.OnLangChangeConsole((CVirtualConsole*)lParam, (DWORD)wParam);
			return 0;
		}
		else if (messg == gConEmu.mn_MsgPostOnBufferHeight)
		{
			gConEmu.OnBufferHeight();
			return 0;
		//} else if (messg == gConEmu.mn_MsgSetForeground) {
		//	apiSetForegroundWindow((HWND)lParam);
		//	return 0;
		}
		else if (messg == gConEmu.mn_MsgFlashWindow)
		{
			return OnFlashWindow((wParam & 0xFF000000) >> 24, wParam & 0xFFFFFF, (HWND)lParam);
		}
		else if (messg == gConEmu.mn_MsgPostAltF9)
		{
			OnAltF9(TRUE);
			return 0;
		}
		else if (messg == gConEmu.mn_MsgActivateCon)
		{
			if (wParam>=1 && wParam<=MAX_CONSOLE_COUNT)
			{
				gConEmu.ConActivate((UINT)wParam-1);
			}
			//if (gSet.IsHostkeySingle(VK_LWIN))
			//{
			//	return gConEmu.LowLevelKeyHook((UINT)wParam, (UINT)lParam);				
			//}
			return 0;
		//} else if (messg == gConEmu.mn_MsgPostSetBackground) {
		//	if (isValid((CVirtualConsole*)wParam))
		//	{
		//		((CVirtualConsole*)wParam)->SetBackgroundImageData((CESERVER_REQ_SETBACKGROUND*)lParam);
		//	} else {
		//		// Поскольку консоль уже была закрыта - нужно просто освободить данные
		//		CESERVER_REQ_SETBACKGROUND* p = (CESERVER_REQ_SETBACKGROUND*)lParam;
		//		free(p);
		//	}
		}
		else if (messg == gConEmu.mn_MsgInitInactiveDC)
		{
			if (isValid((CVirtualConsole*)lParam)
				&& !isActive((CVirtualConsole*)lParam))
			{
				((CVirtualConsole*)lParam)->InitDC(true, true);
				((CVirtualConsole*)lParam)->LoadConsoleData();
			}
		}
		else if (messg == gConEmu.mn_MsgUpdateProcDisplay)
		{
			gConEmu.UpdateProcessDisplay(wParam!=0);
		}

        //else if (messg == gConEmu.mn_MsgCmdStarted || messg == gConEmu.mn_MsgCmdStopped) {
        //  return gConEmu.OnConEmuCmd( (messg == gConEmu.mn_MsgCmdStarted), (HWND)wParam, (DWORD)lParam);
        //}

        if (messg) result = DefWindowProc(hWnd, messg, wParam, lParam);
    }
    return result;
}
