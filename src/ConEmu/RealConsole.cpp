
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
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
//#define ALLOWUSEFARSYNCHRO

#include "Header.h"
#include <Tlhelp32.h>
#include "../common/ConEmuCheck.h"
#include "../common/farcolor.hpp"
#include "../common/RgnDetect.h"
#include "RealConsole.h"
#include "VirtualConsole.h"
#include "TabBar.h"
#include "ConEmu.h"
#include "ConEmuApp.h"
#include "ConEmuChild.h"
#include "ConEmuPipe.h"

#define DEBUGSTRDRAW(s) DEBUGSTR(s)
#define DEBUGSTRINPUT(s) //DEBUGSTR(s)
#define DEBUGSTRINPUTPIPE(s) //DEBUGSTR(s)
#define DEBUGSTRSIZE(s) //DEBUGSTR(s)
#define DEBUGSTRPROC(s) //DEBUGSTR(s)
#define DEBUGSTRCMD(s) //DEBUGSTR(s)
#define DEBUGSTRPKT(s) //DEBUGSTR(s)
#define DEBUGSTRCON(s) //DEBUGSTR(s)
#define DEBUGSTRLANG(s) //DEBUGSTR(s)// ; Sleep(2000)
#define DEBUGSTRLOG(s) //OutputDebugStringA(s)
#define DEBUGSTRALIVE(s) //DEBUGSTR(s)
#define DEBUGSTRTABS(s) //DEBUGSTR(s)
#define DEBUGSTRMACRO(s) //DEBUGSTR(s)

// Иногда не отрисовывается диалог поиска полностью - только бежит текущая сканируемая директория.
// Иногда диалог отрисовался, но часть до текста "..." отсутствует

//PRAGMA_ERROR("При попытке компиляции F:\\VCProject\\FarPlugin\\PPCReg\\compile.cmd - Enter в консоль не прошел");

WARNING("В каждой VCon создать буфер BYTE[256] для хранения распознанных клавиш (Ctrl,...,Up,PgDn,Add,и пр.");
WARNING("Нераспознанные можно помещать в буфер {VKEY,wchar_t=0}, в котором заполнять последний wchar_t по WM_CHAR/WM_SYSCHAR");
WARNING("При WM_(SYS)CHAR помещать wchar_t в начало, в первый незанятый VKEY");
WARNING("При нераспознном WM_KEYUP - брать(и убрать) wchar_t из этого буфера, послав в консоль UP");
TODO("А периодически - проводить процерку isKeyDown, и чистить буфер");
WARNING("при переключении на другую консоль (да наверное и в процессе просто нажатий - модификатор может быть изменен в самой программе) требуется проверка caps, scroll, num");
WARNING("а перед пересылкой символа/клавиши проверять нажат ли на клавиатуре Ctrl/Shift/Alt");

WARNING("Часто после разблокирования компьютера размер консоли изменяется (OK), но обновленное содержимое консоли не приходит в GUI - там остется обрезанная верхняя и нижняя строка");


//Курсор, его положение, размер консоли, измененный текст, и пр...

#define VCURSORWIDTH 2
#define HCURSORHEIGHT 2

#define Free SafeFree
#define Alloc calloc

#define Assert(V) if ((V)==FALSE) { TCHAR szAMsg[MAX_PATH*2]; wsprintf(szAMsg, _T("Assertion (%s) at\n%s:%i"), _T(#V), _T(__FILE__), __LINE__); Box(szAMsg); }

#ifdef _DEBUG
#define HEAPVAL MCHKHEAP
#else
#define HEAPVAL 
#endif

#ifdef _DEBUG
    #define FORCE_INVALIDATE_TIMEOUT 999999999
#else
    #define FORCE_INVALIDATE_TIMEOUT 300
#endif
#define SETSYNCSIZEAPPLYTIMEOUT 500
#define SETSYNCSIZEMAPPINGTIMEOUT 300
#define CONSOLEPROGRESSTIMEOUT 300
#define CONSOLEPROGRESSWARNTIMEOUT 2000 // поставил 2с, т.к. при минимизации консоль обновляется раз в секунду
#define CONSOLEINACTIVERGNTIMEOUT 500
#define SERVERCLOSETIMEOUT 2000

#ifndef INPUTLANGCHANGE_SYSCHARSET
#define INPUTLANGCHANGE_SYSCHARSET 0x0001
#endif

static BOOL gbInSendConEvent = FALSE;


const wchar_t gszAnalogues[32] = {
	32, 9786, 9787, 9829, 9830, 9827, 9824, 8226, 9688, 9675, 9689, 9794, 9792, 9834, 9835, 9788,
	9658, 9668, 8597, 8252,  182,  167, 9632, 8616, 8593, 8595, 8594, 8592, 8735, 8596, 9650, 9660
};

//static bool gbInTransparentAssert = false;

CRealConsole::CRealConsole(CVirtualConsole* apVCon)
{
	MCHKHEAP;

	SetConStatus(L"Initializing ConEmu..");

	PostMessage(ghWnd, WM_SETCURSOR, -1, -1);

    mp_VCon = apVCon;
    mp_Rgn = new CRgnDetect();
	mn_LastRgnFlags = -1;
    
    memset(Title,0,sizeof(Title)); memset(TitleCmp,0,sizeof(TitleCmp));
    mn_tabsCount = 0; ms_PanelTitle[0] = 0; mn_ActiveTab = 0;
	mn_MaxTabs = 20; mb_TabsWasChanged = FALSE;
	mp_tabs = (ConEmuTab*)Alloc(mn_MaxTabs, sizeof(ConEmuTab));
	_ASSERTE(mp_tabs!=NULL);
    //memset(&m_PacketQueue, 0, sizeof(m_PacketQueue));

    mn_FlushIn = mn_FlushOut = 0;

    mr_LeftPanel = mr_LeftPanelFull = mr_RightPanel = mr_RightPanel = MakeRect(-1,-1);
	mb_LeftPanel = mb_RightPanel = FALSE;

	mb_MouseButtonDown = FALSE;
	mb_BtnClicked = FALSE; mrc_BtnClickPos = MakeCoord(-1,-1);

	mcr_LastMouseEventPos = MakeCoord(-1,-1);

	//m_DetectedDialogs.Count = 0;
	//mn_DetectCallCount = 0;

    wcscpy(Title, L"ConEmu");
    wcscpy(TitleFull, Title);
    wcscpy(ms_PanelTitle, Title);
	mb_ForceTitleChanged = FALSE;
    mn_Progress = mn_PreWarningProgress = mn_LastShownProgress = -1; // Процентов нет
    mn_ConsoleProgress = mn_LastConsoleProgress = -1;
	mn_LastConProgrTick = mn_LastWarnCheckTick = 0;

    hPictureView = NULL; mb_PicViewWasHidden = FALSE;

    mh_MonitorThread = NULL; mn_MonitorThreadID = 0; 
    //mh_InputThread = NULL; mn_InputThreadID = 0;
    
    mp_sei = NULL;
    
    mn_ConEmuC_PID = 0; //mn_ConEmuC_Input_TID = 0;
	mh_ConEmuC = NULL; mh_ConEmuCInput = NULL; mb_UseOnlyPipeInput = FALSE;
	//mh_CreateRootEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	mb_InCreateRoot = FALSE;
    
    mb_NeedStartProcess = FALSE; mb_IgnoreCmdStop = FALSE;

    ms_ConEmuC_Pipe[0] = 0; ms_ConEmuCInput_Pipe[0] = 0; ms_VConServer_Pipe[0] = 0;
    mh_TermEvent = CreateEvent(NULL,TRUE/*MANUAL - используется в нескольких нитях!*/,FALSE,NULL); ResetEvent(mh_TermEvent);
    mh_MonitorThreadEvent = CreateEvent(NULL,TRUE,FALSE,NULL); //2009-09-09 Поставил Manual. Нужно для определения, что можно убирать флаг Detached
	mh_ApplyFinished = CreateEvent(NULL,TRUE,FALSE,NULL);
    //mh_EndUpdateEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
    //WARNING("mh_Sync2WindowEvent убрать");
    //mh_Sync2WindowEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
    //mh_ConChanged = CreateEvent(NULL,FALSE,FALSE,NULL);
    //mh_PacketArrived = CreateEvent(NULL,FALSE,FALSE,NULL);
    //mh_CursorChanged = NULL;
    //mb_Detached = FALSE;
    //m_Args.pszSpecialCmd = NULL; -- не требуется
    mb_FullRetrieveNeeded = FALSE;
    memset(&m_LastMouse, 0, sizeof(m_LastMouse));
	memset(&m_LastMouseGuiPos, 0, sizeof(m_LastMouseGuiPos));
    mb_DataChanged = FALSE;

    mn_ProgramStatus = 0; mn_FarStatus = 0; mn_Comspec4Ntvdm = 0;
    isShowConsole = false;
    //mb_ConsoleSelectMode = false;
    mn_SelectModeSkipVk = 0;
    mn_ProcessCount = 0; 
	mn_FarPID = 0; //mn_FarInputTID = 0;
	mn_InRecreate = 0; mb_ProcessRestarted = FALSE; mb_InCloseConsole = FALSE;
    mn_LastSetForegroundPID = 0;
    mh_ServerSemaphore = NULL;
    memset(mh_RConServerThreads, 0, sizeof(mh_RConServerThreads));
	mh_ActiveRConServerThread = NULL;
    memset(mn_RConServerThreadsId, 0, sizeof(mn_RConServerThreadsId));

    ZeroStruct(con); //WARNING! Содержит CriticalSection, поэтому сброс выполнять ПЕРЕД InitializeCriticalSection(&csCON);
	con.hInSetSize = CreateEvent(0,TRUE,TRUE,0);
    mb_BuferModeChangeLocked = FALSE;
	con.DefaultBufferHeight = gSet.bForceBufferHeight ? gSet.nForceBufferHeight : gSet.DefaultBufferHeight;

	ZeroStruct(m_ServerClosing);

	ZeroStruct(m_Args);

    mn_LastInvalidateTick = 0;

    //InitializeCriticalSection(&csPRC); ncsTPRC = 0;
    //InitializeCriticalSection(&csCON); con.ncsT = 0;
    //InitializeCriticalSection(&csPKT); ncsTPKT = 0;

    //mn_LastProcessedPkt = 0;

    hConWnd = NULL; 
	//hFileMapping = NULL; pConsoleData = NULL;
    mh_GuiAttached = NULL; 
    mn_Focused = -1;
    mn_LastVKeyPressed = 0;
    mh_LogInput = NULL; mpsz_LogInputFile = NULL; //mpsz_LogPackets = NULL; mn_LogPackets = 0;

	//mh_FileMapping = mh_FileMappingData = mh_FarFileMapping =
	//mh_FarAliveEvent = NULL;
    //mp_ConsoleInfo = NULL;
    //mp_ConsoleData = NULL;
	//mp_FarInfo = NULL;
    mn_LastConsoleDataIdx = mn_LastConsolePacketIdx = /*mn_LastFarReadIdx =*/ -1;
	mn_LastFarReadTick = 0;
	//ms_HeaderMapName[0] = ms_DataMapName[0] = 0;

	//mh_ColorMapping = NULL;
	//mp_ColorHdr = NULL;
	//mp_ColorData = NULL;
	mn_LastColorFarID = 0;

	//ms_ConEmuC_DataReady[0] = 0; mh_ConEmuC_DataReady = NULL;
	
    m_UseLogs = gSet.isAdvLogging;

    mb_PluginDetected = FALSE; mn_FarPID_PluginDetected = 0; //mn_Far_PluginInputThreadId = 0;

    lstrcpy(ms_Editor, L"edit ");
    MultiByteToWideChar(CP_ACP, 0, "редактирование ", -1, ms_EditorRus, 32);
    lstrcpy(ms_Viewer, L"view ");
    MultiByteToWideChar(CP_ACP, 0, "просмотр ", -1, ms_ViewerRus, 32);
    lstrcpy(ms_TempPanel, L"{Temporary panel");
    MultiByteToWideChar(CP_ACP, 0, "{Временная панель", -1, ms_TempPanelRus, 32);
	//lstrcpy(ms_NameTitle, L"Name");

    SetTabs(NULL,1); // Для начала - показывать вкладку Console, а там ФАР разберется

    PreInit(FALSE); // просто инициализировать переменные размеров...

	MCHKHEAP;
}

CRealConsole::~CRealConsole()
{
	DEBUGSTRCON(L"CRealConsole::~CRealConsole()\n");
#ifdef _DEBUG
	if (!gConEmu.isMainThread())
	{
		_ASSERTE(gConEmu.isMainThread());
	}
#endif

	if (gbInSendConEvent) {
		#ifdef _DEBUG
		_ASSERTE(gbInSendConEvent==FALSE);
		#endif
		Sleep(100);
	}


    StopThread();
    
    MCHKHEAP

    if (con.pConChar)
        { Free(con.pConChar); con.pConChar = NULL; }
    if (con.pConAttr)
        { Free(con.pConAttr); con.pConAttr = NULL; }
	if (con.hInSetSize)
		{ CloseHandle(con.hInSetSize); con.hInSetSize = NULL; }

	//// Требутся, т.к. сам объект делает SafeFree, а не Free
    //if (m_Args.pszSpecialCmd)
    //    { Free(m_Args.pszSpecialCmd); m_Args.pszSpecialCmd = NULL; }
    //if (m_Args.pszStartupDir)
    //    { Free(m_Args.pszStartupDir); m_Args.pszStartupDir = NULL; }


    SafeCloseHandle(mh_ConEmuC); mn_ConEmuC_PID = 0; //mn_ConEmuC_Input_TID = 0;
    SafeCloseHandle(mh_ConEmuCInput);
	m_ConDataChanged.Close();
    
    SafeCloseHandle(mh_ServerSemaphore);
    SafeCloseHandle(mh_GuiAttached);

    //DeleteCriticalSection(&csPRC);
    //DeleteCriticalSection(&csCON);
    //DeleteCriticalSection(&csPKT);
    
    
    if (mp_tabs) Free(mp_tabs);
    mp_tabs = NULL; mn_tabsCount = 0; mn_ActiveTab = 0; mn_MaxTabs = 0;

    // 
    CloseLogFiles();
    
    if (mp_sei) {
    	SafeCloseHandle(mp_sei->hProcess);
    	GlobalFree(mp_sei); mp_sei = NULL;
    }

	//CloseMapping();
	CloseMapHeader(); // CloseMapData() & CloseFarMapData() зовет сам CloseMapHeader
	CloseColorMapping(); // Colorer data
	
	if (mp_Rgn) {
		delete mp_Rgn;
		mp_Rgn = NULL;
	}
}

BOOL CRealConsole::PreCreate(RConStartArgs *args)
	//(BOOL abDetached, LPCWSTR asNewCmd /*= NULL*/, , BOOL abAsAdmin /*= FALSE*/)
{
    mb_NeedStartProcess = FALSE;

    if (args->pszSpecialCmd /*&& !m_Args.pszSpecialCmd*/) {
		SafeFree(m_Args.pszSpecialCmd);
        _ASSERTE(args->bDetached == FALSE);
        m_Args.pszSpecialCmd = _wcsdup(args->pszSpecialCmd);
        if (!m_Args.pszSpecialCmd)
            return FALSE;
    }
    if (args->pszStartupDir) {
		SafeFree(m_Args.pszStartupDir);
        m_Args.pszStartupDir = _wcsdup(args->pszStartupDir);
        if (!m_Args.pszStartupDir)
            return FALSE;
    }

    m_Args.bRunAsRestricted = args->bRunAsRestricted;
	m_Args.bRunAsAdministrator = args->bRunAsAdministrator;
	SafeFree(m_Args.pszUserName); //SafeFree(m_Args.pszUserPassword);
	//if (m_Args.hLogonToken) { CloseHandle(m_Args.hLogonToken); m_Args.hLogonToken = NULL; }
	if (args->pszUserName) {
		m_Args.pszUserName = _wcsdup(args->pszUserName);
		lstrcpy(m_Args.szUserPassword, args->szUserPassword);
		SecureZeroMemory(args->szUserPassword, sizeof(args->szUserPassword));
		//m_Args.pszUserPassword = _wcsdup(args->pszUserPassword ? args->pszUserPassword : L"");
		//m_Args.hLogonToken = args->hLogonToken; args->hLogonToken = NULL;
		if (!m_Args.pszUserName || !*m_Args.szUserPassword)
			return FALSE;
	}

    if (args->bDetached) {
        // Пока ничего не делаем - просто создается серверная нить
        if (!PreInit()) { //TODO: вообще-то PreInit() уже наверное вызван...
            return FALSE;
        }
        m_Args.bDetached = TRUE;
    } else
    if (gSet.nAttachPID) {
        // Attach - only once
        DWORD dwPID = gSet.nAttachPID; gSet.nAttachPID = 0;
        if (!AttachPID(dwPID)) {
            return FALSE;
        }
    } else {
        mb_NeedStartProcess = TRUE;
    }

    if (!StartMonitorThread()) {
        return FALSE;
    }

    return TRUE;
}


void CRealConsole::DumpConsole(HANDLE ahFile)
{
    DWORD dw = 0;
    if (con.pConChar && con.pConAttr) {
        WriteFile(ahFile, con.pConChar, con.nTextWidth * con.nTextHeight * 2, &dw, NULL);
        WriteFile(ahFile, con.pConAttr, con.nTextWidth * con.nTextHeight * 2, &dw, NULL);
    }
}

BOOL CRealConsole::SetConsoleSizeSrv(USHORT sizeX, USHORT sizeY, USHORT sizeBuffer, DWORD anCmdID/*=CECMD_SETSIZESYNC*/)
{
    if (!this) return FALSE;

    if (!hConWnd || ms_ConEmuC_Pipe[0] == 0) {
        // 19.06.2009 Maks - Она действительно может быть еще не создана
        //Box(_T("Console was not created (CRealConsole::SetConsoleSize)"));
		DEBUGSTRSIZE(L"SetConsoleSize skipped (!hConWnd || !ms_ConEmuC_Pipe)\n");
		if (gSet.isAdvLogging) LogString("SetConsoleSize skipped (!hConWnd || !ms_ConEmuC_Pipe)");
        return FALSE; // консоль пока не создана?
    }

    BOOL lbRc = FALSE;
    BOOL fSuccess = FALSE;
    DWORD dwRead = 0;
    int nInSize = sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_SETSIZE);
	int nOutSize = sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_RETSIZE);
    CESERVER_REQ lIn = {{nInSize}};
	CESERVER_REQ lOut = {{nOutSize}};
    SMALL_RECT rect = {0};

    _ASSERTE(anCmdID==CECMD_SETSIZESYNC || anCmdID==CECMD_CMDSTARTED || anCmdID==CECMD_CMDFINISHED);
	ExecutePrepareCmd(&lIn, anCmdID, lIn.hdr.cbSize);



    // Для режима BufferHeight нужно передать еще и видимый прямоугольник (нужна только верхняя координата?)
    if (con.bBufferHeight) {

        // case: buffer mode: change buffer
        CONSOLE_SCREEN_BUFFER_INFO sbi = con.m_sbi;

        sbi.dwSize.X = sizeX; // new
		sizeBuffer = BufferHeight(sizeBuffer); // Если нужно - скорректировать
		_ASSERTE(sizeBuffer > 0);
		sbi.dwSize.Y = sizeBuffer;

        rect.Top = sbi.srWindow.Top;
        rect.Left = sbi.srWindow.Left;
        rect.Right = rect.Left + sizeX - 1;
        rect.Bottom = rect.Top + sizeY - 1;
        if (rect.Right >= sbi.dwSize.X)
        {
            int shift = sbi.dwSize.X - 1 - rect.Right;
            rect.Left += shift;
            rect.Right += shift;
        }
        if (rect.Bottom >= sbi.dwSize.Y)
        {
            int shift = sbi.dwSize.Y - 1 - rect.Bottom;
            rect.Top += shift;
            rect.Bottom += shift;
        }

        // В size передаем видимую область
        //sizeY = TextHeight(); -- sizeY уже(!) должен содержать требуемую высоту видимой области!
	} else {
		_ASSERTE(sizeBuffer == 0);
	}

    _ASSERTE(sizeY>0 && sizeY<200);

    // Устанавливаем параметры для передачи в ConEmuC
    lIn.SetSize.nBufferHeight = sizeBuffer; //con.bBufferHeight ? gSet.Default BufferHeight : 0;
    lIn.SetSize.size.X = sizeX;
	lIn.SetSize.size.Y = sizeY;
	TODO("nTopVisibleLine должен передаваться при скролле, а не при ресайзе!");
    lIn.SetSize.nSendTopLine = (gSet.AutoScroll || !con.bBufferHeight) ? -1 : con.nTopVisibleLine;
    lIn.SetSize.rcWindow = rect;
	lIn.SetSize.dwFarPID = (con.bBufferHeight && !isFarBufferSupported()) ? 0 : GetFarPID();

	// Если размер менять не нужно - то и CallNamedPipe не делать
	//if (mp_ConsoleInfo) {

	// Если заблокирована верхняя видимая строка - выполнять строго
	if (anCmdID != CECMD_CMDFINISHED && lIn.SetSize.nSendTopLine == -1) {
		// иначе - проверяем текущее соответствие
		//CONSOLE_SCREEN_BUFFER_INFO sbi = mp_ConsoleInfo->sbi;
		bool lbSizeMatch = true;
		if (con.bBufferHeight) {
			if (con.m_sbi.dwSize.X != sizeX || con.m_sbi.dwSize.Y != sizeBuffer)
				lbSizeMatch = false;
			else if (con.m_sbi.srWindow.Top != rect.Top || con.m_sbi.srWindow.Bottom != rect.Bottom)
				lbSizeMatch = false;
		} else {
			if (con.m_sbi.dwSize.X != sizeX || con.m_sbi.dwSize.Y != sizeY)
				lbSizeMatch = false;
		}
		// fin
		if (lbSizeMatch && anCmdID != CECMD_CMDFINISHED)
			return TRUE; // менять ничего не нужно
	}
	//}

	if (gSet.isAdvLogging) {
		char szInfo[128];
		wsprintfA(szInfo, "%s(Cols=%i, Rows=%i, Buf=%i, Top=%i)",
			(anCmdID==CECMD_SETSIZESYNC) ? "CECMD_SETSIZESYNC" :
			(anCmdID==CECMD_CMDSTARTED) ? "CECMD_CMDSTARTED" :
			(anCmdID==CECMD_CMDFINISHED) ? "CECMD_CMDFINISHED" :
			"UnknownSizeCommand", sizeX, sizeY, sizeBuffer, lIn.SetSize.nSendTopLine);
		LogString(szInfo, TRUE);
	}

	#ifdef _DEBUG
	wchar_t szDbgCmd[128]; wsprintf(szDbgCmd, L"SetConsoleSize.CallNamedPipe(cx=%i, cy=%i, buf=%i, cmd=%i)\n",
		sizeX, sizeY, sizeBuffer, anCmdID);
	DEBUGSTRSIZE(szDbgCmd);
	#endif

    fSuccess = CallNamedPipe(ms_ConEmuC_Pipe, &lIn, lIn.hdr.cbSize, &lOut, lOut.hdr.cbSize, &dwRead, 500);

    if (!fSuccess || dwRead<(DWORD)nOutSize) {
		if (gSet.isAdvLogging) {
			char szInfo[128]; DWORD dwErr = GetLastError();
			wsprintfA(szInfo, "SetConsoleSizeSrv.CallNamedPipe FAILED!!! ErrCode=0x%08X, Bytes read=%i", dwErr, dwRead);
			LogString(szInfo);
		}
		DEBUGSTRSIZE(L"SetConsoleSize.CallNamedPipe FAILED!!!\n");
	} else {
		_ASSERTE(m_ConsoleMap.IsValid());

		bool bNeedApplyConsole = //mp_ConsoleInfo &&
			m_ConsoleMap.IsValid()
			&& (anCmdID == CECMD_SETSIZESYNC) 
			&& (mn_MonitorThreadID != GetCurrentThreadId());

		DEBUGSTRSIZE(L"SetConsoleSize.fSuccess == TRUE\n");
        if (lOut.hdr.nCmd != lIn.hdr.nCmd) {
			_ASSERTE(lOut.hdr.nCmd == lIn.hdr.nCmd);
			if (gSet.isAdvLogging) {
				char szInfo[128]; wsprintfA(szInfo, "SetConsoleSizeSrv FAILED!!! OutCmd(%i)!=InCmd(%i)", lOut.hdr.nCmd, lIn.hdr.nCmd);
				LogString(szInfo);
			}
		} else {
            //con.nPacketIdx = lOut.SetSizeRet.nNextPacketId;
			//mn_LastProcessedPkt = lOut.SetSizeRet.nNextPacketId;
            CONSOLE_SCREEN_BUFFER_INFO sbi = {{0,0}};
			int nBufHeight;
			//_ASSERTE(mp_ConsoleInfo);
			if (bNeedApplyConsole /*&& mp_ConsoleInfo->nCurDataMapIdx && (HWND)mp_ConsoleInfo->hConWnd*/) {
				// Если Apply еще не прошел - ждем
				DWORD nWait = -1;
				if (con.m_sbi.dwSize.X != sizeX || con.m_sbi.dwSize.Y != (sizeBuffer ? sizeBuffer : sizeY))
				{
					//// Проходит некоторое (короткое) время, пока обновится FileMapping в нашем процессе
					//_ASSERTE(mp_ConsoleInfo!=NULL);
					//COORD crCurSize = mp_ConsoleInfo->sbi.dwSize;
					//if (crCurSize.X != sizeX || crCurSize.Y != (sizeBuffer ? sizeBuffer : sizeY))
					//{
					//	DWORD dwStart = GetTickCount();
					//	do {
					//		Sleep(1);
					//		crCurSize = mp_ConsoleInfo->sbi.dwSize;
					//		if (crCurSize.X == sizeX && crCurSize.Y == (sizeBuffer ? sizeBuffer : sizeY))
					//			break;
					//	} while ((GetTickCount() - dwStart) < SETSYNCSIZEMAPPINGTIMEOUT);
					//}

					ResetEvent(mh_ApplyFinished);
					mn_LastConsolePacketIdx--;
					SetEvent(mh_MonitorThreadEvent);

					nWait = WaitForSingleObject(mh_ApplyFinished, SETSYNCSIZEAPPLYTIMEOUT);

					COORD crDebugCurSize = con.m_sbi.dwSize;
					if (crDebugCurSize.X != sizeX) {
						if (gSet.isAdvLogging) {
							char szInfo[128]; wsprintfA(szInfo, "SetConsoleSize FAILED!!! ReqSize={%ix%i}, OutSize={%ix%i}", sizeX, (sizeBuffer ? sizeBuffer : sizeY), crDebugCurSize.X, crDebugCurSize.Y);
							LogString(szInfo);
						}
						#ifdef _DEBUG
						_ASSERTE(crDebugCurSize.X == sizeX);
						#endif
					}
				}

				if (gSet.isAdvLogging){
					LogString(
					(nWait == (DWORD)-1) ?
						"SetConsoleSizeSrv: does not need wait" :
					(nWait != WAIT_OBJECT_0) ?
						"SetConsoleSizeSrv.WaitForSingleObject(mh_ApplyFinished) TIMEOUT!!!" :
						"SetConsoleSizeSrv.WaitForSingleObject(mh_ApplyFinished) succeded");
				}

				sbi = con.m_sbi;
				nBufHeight = con.nBufferHeight;
			} else {
		        sbi = lOut.SetSizeRet.SetSizeRet;
				nBufHeight = lIn.SetSize.nBufferHeight;
				if (gSet.isAdvLogging)
					LogString("SetConsoleSizeSrv.Not waiting for ApplyFinished");
			}

			WARNING("!!! Здесь часто возникают _ASSERT'ы. Видимо высота буфера меняется в другой нити и con.bBufferHeight просто не успевает?");
            if (con.bBufferHeight) {
                _ASSERTE((sbi.srWindow.Bottom-sbi.srWindow.Top)<200);

				if (gSet.isAdvLogging) {
					char szInfo[128]; wsprintfA(szInfo, "Current size: Cols=%i, Buf=%i", sbi.dwSize.X, sbi.dwSize.Y);
					LogString(szInfo, TRUE);
				}

				if (sbi.dwSize.X == sizeX && sbi.dwSize.Y == nBufHeight) {
                    lbRc = TRUE;
				} else {
					if (gSet.isAdvLogging) {
						char szInfo[128]; wsprintfA(szInfo, "SetConsoleSizeSrv FAILED! Ask={%ix%i}, Cur={%ix%i}, Ret={%ix%i}",
							sizeX, sizeY,
							sbi.dwSize.X, sbi.dwSize.Y,
							lOut.SetSizeRet.SetSizeRet.dwSize.X, lOut.SetSizeRet.SetSizeRet.dwSize.Y
							);
						LogString(szInfo);
					}
					lbRc = FALSE;
				}
            } else {
                if (sbi.dwSize.Y > 200) {
                    _ASSERTE(sbi.dwSize.Y<200);
                }

				if (gSet.isAdvLogging) {
					char szInfo[128]; wsprintfA(szInfo, "Current size: Cols=%i, Rows=%i", sbi.dwSize.X, sbi.dwSize.Y);
					LogString(szInfo, TRUE);
				}

				if (sbi.dwSize.X == sizeX && sbi.dwSize.Y == sizeY) {
					lbRc = TRUE;
				} else {
					if (gSet.isAdvLogging) {
						char szInfo[128]; wsprintfA(szInfo, "SetConsoleSizeSrv FAILED! Ask={%ix%i}, Cur={%ix%i}, Ret={%ix%i}",
							sizeX, sizeY,
							sbi.dwSize.X, sbi.dwSize.Y,
							lOut.SetSizeRet.SetSizeRet.dwSize.X, lOut.SetSizeRet.SetSizeRet.dwSize.Y
							);
						LogString(szInfo);
					}
					lbRc = FALSE;
				}
            }
            //if (sbi.dwSize.X == size.X && sbi.dwSize.Y == size.Y) {
            //    con.m_sbi = sbi;
            //    if (sbi.dwSize.X == con.m_sbi.dwSize.X && sbi.dwSize.Y == con.m_sbi.dwSize.Y) {
            //        SetEvent(mh_ConChanged);
            //    }
            //    lbRc = true;
            //}
            if (lbRc) { // Попробуем сразу менять nTextWidth/nTextHeight. Иначе синхронизация размеров консоли глючит...
				DEBUGSTRSIZE(L"SetConsoleSizeSrv.lbRc == TRUE\n");
                con.nChange2TextWidth = sbi.dwSize.X;
                con.nChange2TextHeight = con.bBufferHeight ? (sbi.srWindow.Bottom-sbi.srWindow.Top+1) : sbi.dwSize.Y;
				#ifdef _DEBUG
				if (con.bBufferHeight)
					_ASSERTE(con.nBufferHeight == sbi.dwSize.Y);
				#endif
				con.nBufferHeight = con.bBufferHeight ? sbi.dwSize.Y : 0;
                if (con.nChange2TextHeight > 200) {
                    _ASSERTE(con.nChange2TextHeight<=200);
                }
            }
        }
    }

	return lbRc;
}

BOOL CRealConsole::SetConsoleSize(USHORT sizeX, USHORT sizeY, USHORT sizeBuffer, DWORD anCmdID/*=CECMD_SETSIZESYNC*/)
{
    if (!this) return FALSE;
    //_ASSERTE(hConWnd && ms_ConEmuC_Pipe[0]);

    if (!hConWnd || ms_ConEmuC_Pipe[0] == 0) {
        // 19.06.2009 Maks - Она действительно может быть еще не создана
        //Box(_T("Console was not created (CRealConsole::SetConsoleSize)"));
		if (gSet.isAdvLogging) LogString("SetConsoleSize skipped (!hConWnd || !ms_ConEmuC_Pipe)");
		DEBUGSTRSIZE(L"SetConsoleSize skipped (!hConWnd || !ms_ConEmuC_Pipe)\n");
        return false; // консоль пока не создана?
    }
    
	HEAPVAL
    _ASSERTE(sizeX>=MIN_CON_WIDTH && sizeY>=MIN_CON_HEIGHT);
    
    if (sizeX</*4*/MIN_CON_WIDTH)
        sizeX = /*4*/MIN_CON_WIDTH;
    if (sizeY</*3*/MIN_CON_HEIGHT)
        sizeY = /*3*/MIN_CON_HEIGHT;

	_ASSERTE(con.bBufferHeight || (!con.bBufferHeight && !sizeBuffer));
	if (con.bBufferHeight && !sizeBuffer)
		sizeBuffer = BufferHeight(sizeBuffer);
	_ASSERTE(!con.bBufferHeight || (sizeBuffer >= sizeY));

	BOOL lbRc = FALSE;

	DWORD dwPID = GetFarPID();
	if (dwPID) {
		// Если это СТАРЫЙ FAR (нет Synchro) - может быть не ресайзить через плагин?
		// Хотя тут плюс в том, что хоть активация и идет чуть медленнее, но
		// возврат из ресайза получается строго после обновления консоли
		if (!mb_PluginDetected || dwPID != mn_FarPID_PluginDetected)
			dwPID = 0;
	}
	_ASSERTE(sizeY <= 200);
	/*if (!con.bBufferHeight && dwPID)
		lbRc = SetConsoleSizePlugin(sizeX, sizeY, sizeBuffer, anCmdID);
	else*/

	HEAPVAL;

	// Чтобы ВО время ресайза пакеты НЕ обрабатывались
	ResetEvent(con.hInSetSize); con.bInSetSize = TRUE;
	lbRc = SetConsoleSizeSrv(sizeX, sizeY, sizeBuffer, anCmdID);
	con.bInSetSize = FALSE; SetEvent(con.hInSetSize);


	HEAPVAL;
    if (lbRc && isActive() && !isNtvdm())
	{
        // update size info
        //if (!gSet.isFullScreen && !gConEmu.isZoomed() && !gConEmu.isIconic())
        if (gConEmu.isWindowNormal())
        {
			int nHeight = TextHeight();
			gSet.UpdateSize(sizeX, nHeight);
        }
    }


	HEAPVAL;
	DEBUGSTRSIZE(L"SetConsoleSize.finalizing\n");

    return lbRc;
}

// Изменить размер консоли по размеру окна (главного)
// prcNewWnd передается из CConEmuMain::OnSizing(WPARAM wParam, LPARAM lParam)
// для опережающего ресайза консоли (во избежание мелькания отрисовки панелей)
void CRealConsole::SyncConsole2Window(BOOL abNtvdmOff/*=FALSE*/, LPRECT prcNewWnd/*=NULL*/)
{
    if (!this)
        return;

    //2009-06-17 Попробуем так. Вроде быстрее и наверное ничего блокироваться не должно
    /*
    if (GetCurrentThreadId() != mn_MonitorThreadID) {
        RECT rcClient; GetClientRect(ghWnd, &rcClient);
        _ASSERTE(rcClient.right>250 && rcClient.bottom>200);

        // Посчитать нужный размер консоли
        RECT newCon = gConEmu.CalcRect(CER_CONSOLE, rcClient, CER_MAINCLIENT);

        if (newCon.right==TextWidth() && newCon.bottom==TextHeight())
            return; // размеры не менялись

        SetEvent(mh_Sync2WindowEvent);
        return;
    }
    */

    DEBUGLOGFILE("SyncConsoleToWindow\n");

    RECT rcClient;
	if (prcNewWnd == NULL)
	{
		GetClientRect(ghWnd, &rcClient);
	}
	else
	{
		rcClient = gConEmu.CalcRect(CER_MAINCLIENT, *prcNewWnd, CER_MAIN);
	}
    //_ASSERTE(rcClient.right>140 && rcClient.bottom>100);

    // Посчитать нужный размер консоли
	gConEmu.AutoSizeFont(rcClient, CER_MAINCLIENT);
	RECT newCon = gConEmu.CalcRect(abNtvdmOff ? CER_CONSOLE_NTVDMOFF : CER_CONSOLE, rcClient, CER_MAINCLIENT, mp_VCon);
    _ASSERTE(newCon.right>20 && newCon.bottom>6);

	// Во избежание лишних движений да и зацикливания...
	if (con.nTextWidth != newCon.right || con.nTextHeight != newCon.bottom)
	{
		if (gSet.isAdvLogging>=2) {
			char szInfo[128]; wsprintfA(szInfo, "SyncConsoleToWindow(Cols=%i, Rows=%i, Current={%i,%i})", newCon.right, newCon.bottom, con.nTextWidth, con.nTextHeight);
			LogString(szInfo);
		}

#ifdef _DEBUG
		if (newCon.right == 80) {
			newCon.right = newCon.right;
		}
#endif

		// Сразу поставим, чтобы в основной нити при синхронизации размер не слетел
		// Необходимо заблокировать RefreshThread, чтобы не вызывался ApplyConsoleInfo ДО ЗАВЕРШЕНИЯ SetConsoleSize
		con.bLockChange2Text = TRUE;
		con.nChange2TextWidth = newCon.right; con.nChange2TextHeight = newCon.bottom;

		SetConsoleSize(newCon.right, newCon.bottom, 0/*Auto*/);

		con.bLockChange2Text = FALSE;

		if (isActive() && gConEmu.isMainThread()) {
			// Сразу обновить DC чтобы скорректировать Width & Height
			mp_VCon->OnConsoleSizeChanged();
		}
	}
}

// Вызывается при аттаче (после детача), или после RunAs?
// sbi передавать не ссылкой, а копией, ибо та же память
BOOL CRealConsole::AttachConemuC(HWND ahConWnd, DWORD anConemuC_PID, CESERVER_REQ_STARTSTOP rStartStop, CESERVER_REQ_STARTSTOPRET* pRet)
{
    DWORD dwErr = 0;
    HANDLE hProcess = NULL;

	_ASSERTE(pRet!=NULL);

    // Процесс запущен через ShellExecuteEx под другим пользователем (Administrator)
	if (mp_sei) {
		// в некоторых случаях (10% на x64) hProcess не успевает заполниться (еще не отработала функция?)
		if (!mp_sei->hProcess) {
			DWORD dwStart = GetTickCount();
			DWORD dwDelay = 2000;
			do {
				Sleep(100);
			} while ((GetTickCount() - dwStart) < dwDelay);
			_ASSERTE(mp_sei->hProcess!=NULL);
		}
		// поехали
		if (mp_sei->hProcess) {
	    	hProcess = mp_sei->hProcess;
			mp_sei->hProcess = NULL; // более не требуется. хэндл закроется в другом месте
		}
    }
    // Иначе - отркрываем как обычно
    if (!hProcess)
    	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION|SYNCHRONIZE, FALSE, anConemuC_PID);
    if (!hProcess) {
        DisplayLastError(L"Can't open ConEmuC process! Attach is impossible!", dwErr = GetLastError());
        return FALSE;
    }

	//2010-03-03 переделано для аттача через пайп
	int nCurWidth = 0, nCurHeight = 0;
	BOOL bCurBufHeight = FALSE;
	GetConWindowSize(rStartStop.sbi, nCurWidth, nCurHeight, &bCurBufHeight);
	if (rStartStop.bRootIsCmdExe)
		bCurBufHeight = TRUE; //2010-06-09
	if (con.bBufferHeight != bCurBufHeight) {
		_ASSERTE(mb_BuferModeChangeLocked==FALSE);
		SetBufferHeightMode(bCurBufHeight, FALSE);
	}
	RECT rcWnd; GetClientRect(ghWnd, &rcWnd);
	gConEmu.AutoSizeFont(rcWnd, CER_MAINCLIENT);
	RECT rcCon = gConEmu.CalcRect(CER_CONSOLE, rcWnd, CER_MAINCLIENT);

	// Скорректировать sbi на новый, который БУДЕТ установлен после отработки сервером аттача
	rStartStop.sbi.dwSize.X = rcCon.right;
	rStartStop.sbi.srWindow.Left = 0; rStartStop.sbi.srWindow.Right = rcCon.right-1;
	if (bCurBufHeight) {
		// sbi.dwSize.Y не трогаем
		rStartStop.sbi.srWindow.Bottom = rStartStop.sbi.srWindow.Top + rcCon.bottom - 1;
	} else {
		rStartStop.sbi.dwSize.Y = rcCon.bottom;
		rStartStop.sbi.srWindow.Top = 0; rStartStop.sbi.srWindow.Bottom = rcCon.bottom - 1;
	}	

	con.m_sbi = rStartStop.sbi;

    //// Событие "изменения" консоли //2009-05-14 Теперь события обрабатываются в GUI, но прийти из консоли может изменение размера курсора
    //wsprintfW(ms_ConEmuC_Pipe, CE_CURSORUPDATE, mn_ConEmuC_PID);
    //mh_CursorChanged = CreateEvent ( NULL, FALSE, FALSE, ms_ConEmuC_Pipe );
    //if (!mh_CursorChanged) {
    //    ms_ConEmuC_Pipe[0] = 0;
    //    DisplayLastError(L"Can't create event!");
    //    return FALSE;
    //}

    SetHwnd(ahConWnd);
    ProcessUpdate(&anConemuC_PID, 1);

    mh_ConEmuC = hProcess;
    mn_ConEmuC_PID = anConemuC_PID;

    CreateLogFiles();

	// Инициализировать имена пайпов, событий, мэппингов и т.п.
	InitNames();
    //// Имя пайпа для управления ConEmuC
    //wsprintfW(ms_ConEmuC_Pipe, CESERVERPIPENAME, L".", mn_ConEmuC_PID);
    //wsprintfW(ms_ConEmuCInput_Pipe, CESERVERINPUTNAME, L".", mn_ConEmuC_PID);
    //MCHKHEAP

	// Открыть map с данными, он уже должен быть создан
	OpenMapHeader(TRUE);


    //SetConsoleSize(MakeCoord(TextWidth,TextHeight));
	// Командой - низя, т.к. сервер сейчас только что запустился и ждет GUI
    //SetConsoleSize(rcCon.right,rcCon.bottom);
	pRet->bWasBufferHeight = bCurBufHeight;
	pRet->hWnd = ghWnd;
	pRet->hWndDC = ghWndDC;
	pRet->dwPID = GetCurrentProcessId();
	pRet->nBufferHeight = bCurBufHeight ? rStartStop.sbi.dwSize.Y : 0;
	pRet->nWidth = rcCon.right;
	pRet->nHeight = rcCon.bottom;
	pRet->dwSrvPID = anConemuC_PID;
	pRet->bNeedLangChange = rcCon.bottom;
	pRet->bNeedLangChange = TRUE;
	TODO("Проверить на x64, не будет ли проблем с 0xFFFFFFFFFFFFFFFFFFFFF");
	pRet->NewConsoleLang = gConEmu.GetActiveKeyboardLayout();
	

    // Передернуть нить MonitorThread
    SetEvent(mh_MonitorThreadEvent);

    return TRUE;
}

BOOL CRealConsole::AttachPID(DWORD dwPID)
{
    TODO("AttachPID пока работать не будет");
    return FALSE;

#ifdef ALLOW_ATTACHPID
    #ifdef MSGLOGGER
        TCHAR szMsg[100]; wsprintf(szMsg, _T("Attach to process %i"), (int)dwPID);
        DEBUGSTRPROC(szMsg);
    #endif
    BOOL lbRc = AttachConsole(dwPID);
    if (!lbRc) {
        DEBUGSTRPROC(_T(" - failed\n"));
        BOOL lbFailed = TRUE;
        DWORD dwErr = GetLastError();
        if (/*dwErr==0x1F || dwErr==6 &&*/ dwPID == -1)
        {
            // Если ConEmu запускается из FAR'а батником - то родительский процесс - CMD.EXE, а он уже скорее всего закрыт. то есть подцепиться не удастся
            HWND hConsole = FindWindowEx(NULL,NULL,_T("ConsoleWindowClass"),NULL);
            if (hConsole && IsWindowVisible(hConsole)) {
                DWORD dwCurPID = 0;
                if (GetWindowThreadProcessId(hConsole,  &dwCurPID)) {
                    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS,FALSE,dwCurPID);
                    dwErr = GetLastError();
                    if (AttachConsole(dwCurPID))
                        lbFailed = FALSE;
                    else
                        dwErr = GetLastError();
                }
            }
        }
        if (lbFailed) {
            TCHAR szErr[255];
            wsprintf(szErr, _T("AttachConsole failed (PID=%i)!"), dwPID);
            DisplayLastError(szErr, dwErr);
            return FALSE;
        }
    }
    DEBUGSTRPROC(_T(" - OK"));

    TODO("InitHandler в GUI наверное уже и не нужен...");
    //InitHandlers(FALSE);

    // Попытаться дернуть плагин для установки шрифта.
    CConEmuPipe pipe;
    
    //DEBUGSTRPROC(_T("CheckProcesses\n"));
    //gConEmu.CheckProcesses(0,TRUE);
    
    if (pipe.Init(_T("DefFont.in.attach"), TRUE))
        pipe.Execute(CMD_DEFFONT);

    return TRUE;
#endif
}

//BOOL CRealConsole::FlushInputQueue(DWORD nTimeout /*= 500*/)
//{
//	if (!this) return FALSE;
//	
//	if (nTimeout > 1000) nTimeout = 1000;
//	DWORD dwStartTick = GetTickCount();
//	
//	mn_FlushOut = mn_FlushIn;
//	mn_FlushIn++;
//
//	_ASSERTE(mn_ConEmuC_Input_TID!=0);
//
//	TODO("Активной может быть нить ввода плагина фара а не сервера!");
//	
//	//TODO("Преверка зависания нити и ее перезапуск");
//	PostThreadMessage(mn_ConEmuC_Input_TID, INPUT_THREAD_ALIVE_MSG, mn_FlushIn, 0);
//	
//	while (mn_FlushOut != mn_FlushIn) {
//		if (WaitForSingleObject(mh_ConEmuC, 100) == WAIT_OBJECT_0)
//			break; // Процесс сервера завершился
//		
//		DWORD dwCurTick = GetTickCount();
//		DWORD dwDelta = dwCurTick - dwStartTick;
//		if (dwDelta > nTimeout) break;
//	}
//	
//	return (mn_FlushOut == mn_FlushIn);
//}

void CRealConsole::PostKeyPress(WORD vkKey, DWORD dwControlState, wchar_t wch, int ScanCode /*= -1*/)
{
	if (!this)
		return;

	if (ScanCode == -1)
		ScanCode = MapVirtualKey(vkKey, 0/*MAPVK_VK_TO_VSC*/);

	INPUT_RECORD r = {KEY_EVENT};

	r.Event.KeyEvent.bKeyDown = TRUE;
	r.Event.KeyEvent.wRepeatCount = 1;
	r.Event.KeyEvent.wVirtualKeyCode = vkKey;
	r.Event.KeyEvent.wVirtualScanCode = ScanCode;
	r.Event.KeyEvent.uChar.UnicodeChar = wch;
	r.Event.KeyEvent.dwControlKeyState = dwControlState;
	TODO("Может нужно в dwControlKeyState применять модификатор, если он и есть vkKey?");
	PostConsoleEvent(&r);


	r.Event.KeyEvent.bKeyDown = FALSE;
	r.Event.KeyEvent.dwControlKeyState = dwControlState;
	PostConsoleEvent(&r);
}

void CRealConsole::PostKeyUp(WORD vkKey, DWORD dwControlState, wchar_t wch, int ScanCode /*= -1*/)
{
	if (!this)
		return;

	if (ScanCode == -1)
		ScanCode = MapVirtualKey(vkKey, 0/*MAPVK_VK_TO_VSC*/);

	INPUT_RECORD r = {KEY_EVENT};

	r.Event.KeyEvent.bKeyDown = FALSE;
	r.Event.KeyEvent.wRepeatCount = 1;
	r.Event.KeyEvent.wVirtualKeyCode = vkKey;
	r.Event.KeyEvent.wVirtualScanCode = ScanCode;
	r.Event.KeyEvent.uChar.UnicodeChar = wch;
	r.Event.KeyEvent.dwControlKeyState = dwControlState;
	PostConsoleEvent(&r);
}

void CRealConsole::PostConsoleEvent(INPUT_RECORD* piRec)
{
	if (!this) return;
	if (mn_ConEmuC_PID == 0 || !m_ConsoleMap.IsValid())
		return; // Сервер еще не стартовал. События будут пропущены...

	//DWORD dwTID = 0;
	//#ifdef ALLOWUSEFARSYNCHRO
	//	if (isFar() && mn_FarInputTID) {
	//		dwTID = mn_FarInputTID;
	//	} else {
	//#endif
	//if (mn_ConEmuC_Input_TID == 0) // значит еще TID ввода не получили
	//	return;
	//dwTID = mn_ConEmuC_Input_TID;
	//#ifdef ALLOWUSEFARSYNCHRO
	//	}
	//#endif
	//if (dwTID == 0) {
	//	//_ASSERTE(dwTID!=0);
	//	gConEmu.DebugStep(L"ConEmu: Input thread id is NULL");
	//	return;
	//}
	
    if (piRec->EventType == MOUSE_EVENT) {
        #ifdef _DEBUG
        static DWORD nLastBtnState;
        #endif
        
        //WARNING!!! Тут проблема следующая.
        // Фаровский AltIns требует получения MOUSE_MOVE в той же координате, где прошел клик.
        //  Иначе граббинг начинается не с "кликнутой" а со следующей позиции.
        // В других случаях наблюдаются проблемы с кликами. Например, в диалоге
        //  UCharMap. При его минимизации, если кликнуть по кнопке минимизации
        //  и фар получит MOUSE_MOVE - то диалог закроется при отпускании кнопки мышки.
        
		//2010-07-12 обработка вынесена в CRealConsole::OnMouse с учетом GUI курсора по пикселам
        //if (piRec->Event.MouseEvent.dwEventFlags == MOUSE_MOVED)
        //{
        //    if (m_LastMouse.dwButtonState     == piRec->Event.MouseEvent.dwButtonState 
        //     && m_LastMouse.dwControlKeyState == piRec->Event.MouseEvent.dwControlKeyState
        //     && m_LastMouse.dwMousePosition.X == piRec->Event.MouseEvent.dwMousePosition.X
        //     && m_LastMouse.dwMousePosition.Y == piRec->Event.MouseEvent.dwMousePosition.Y)
        //    {
        //        //#ifdef _DEBUG
        //        //wchar_t szDbg[60];
        //        //wsprintf(szDbg, L"!!! Skipping ConEmu.Mouse event at: {%ix%i}\n", m_LastMouse.dwMousePosition.X, m_LastMouse.dwMousePosition.Y);
        //        //DEBUGSTRINPUT(szDbg);
        //        //#endif
        //        return; // Это событие лишнее. Движения мышки реально не было, кнопки не менялись
        //    }
        //    #ifdef _DEBUG
        //    if ((nLastBtnState&FROM_LEFT_1ST_BUTTON_PRESSED)) {
        //    	nLastBtnState = nLastBtnState;
        //    }
        //    #endif
        //}
        // Запомним
        m_LastMouse.dwMousePosition   = piRec->Event.MouseEvent.dwMousePosition;
        m_LastMouse.dwEventFlags      = piRec->Event.MouseEvent.dwEventFlags;
        m_LastMouse.dwButtonState     = piRec->Event.MouseEvent.dwButtonState;
        m_LastMouse.dwControlKeyState = piRec->Event.MouseEvent.dwControlKeyState;
        #ifdef _DEBUG
        nLastBtnState = piRec->Event.MouseEvent.dwButtonState;
        #endif

        //#ifdef _DEBUG
        //wchar_t szDbg[60];
        //wsprintf(szDbg, L"ConEmu.Mouse event at: {%ix%i}\n", m_LastMouse.dwMousePosition.X, m_LastMouse.dwMousePosition.Y);
        //DEBUGSTRINPUT(szDbg);
        //#endif
        
    } else if (piRec->EventType == KEY_EVENT) {
    	if (!piRec->Event.KeyEvent.wRepeatCount) {
    		_ASSERTE(piRec->Event.KeyEvent.wRepeatCount!=0);
    		piRec->Event.KeyEvent.wRepeatCount = 0;
		}
    }
    

    MSG64 msg = {NULL};
    
    if (PackInputRecord ( piRec, &msg )) {
		if (m_UseLogs>=2)
			LogInput(piRec);

    	_ASSERTE(msg.message!=0);
		//if (mb_UseOnlyPipeInput) {
		PostConsoleEventPipe(&msg);
		#ifdef _DEBUG
		if (gbInSendConEvent) {
			_ASSERTE(!gbInSendConEvent);
		}
		#endif
		//} else
		//// ERROR_INVALID_THREAD_ID == 1444 (0x5A4)
		//// On Vista PostThreadMessage failed with code 5, if target process created 'As administrator'
		//if (!PostThreadMessage(dwTID, msg.message, msg.wParam, msg.lParam)) {
		//	DWORD dwErr = GetLastError();
		//	if (dwErr == ERROR_ACCESS_DENIED/*5*/) {
		//		mb_UseOnlyPipeInput = TRUE;
		//		PostConsoleEventPipe(&msg);
		//	} else if (dwErr == ERROR_INVALID_THREAD_ID/*1444*/) {
		//		// In shutdown?
		//	} else {
		//		wchar_t szErr[100];
		//		wsprintfW(szErr, L"ConEmu: PostThreadMessage(%i) failed, code=0x%08X", dwTID, dwErr);
		//		gConEmu.DebugStep(szErr);
		//	}
		//}
    } else {
    	gConEmu.DebugStep(L"ConEmu: PackInputRecord failed!");
    }
}

//DWORD CRealConsole::InputThread(LPVOID lpParameter)
//{
//    CRealConsole* pRCon = (CRealConsole*)lpParameter;
//    
//    MSG msg;
//    while (GetMessage(&msg,0,0,0)) {
//    	if (msg.message == WM_QUIT) break;
//    	if (WaitForSingleObject(pRCon->mh_TermEvent, 0) == WAIT_OBJECT_0) break;
//
//    	if (msg.message == INPUT_THREAD_ALIVE_MSG) {
//    		pRCon->mn_FlushOut = msg.wParam;
//    		continue;
//    	
//    	} else {
//    	
//    		INPUT_RECORD r = {0};
//    		
//    		if (UnpackInputRecord(&msg, &r)) {
//    			pRCon->SendConsoleEvent(&r);
//    		}
//    	
//    	}
//    }
//    
//    return 0;
//}

DWORD CRealConsole::MonitorThread(LPVOID lpParameter)
{
    CRealConsole* pRCon = (CRealConsole*)lpParameter;

    if (pRCon->mb_NeedStartProcess)
	{
        _ASSERTE(pRCon->mh_ConEmuC==NULL);
        pRCon->mb_NeedStartProcess = FALSE;
        if (!pRCon->StartProcess())
		{
            DEBUGSTRPROC(L"### Can't start process\n");
            return 0;
        }

        // Если ConEmu был запущен с ключом "/single /cmd xxx" то после окончания
        // загрузки - сбросить команду, которая пришла из "/cmd" - загрузить настройку
        if (gSet.SingleInstanceArg)
        {
        	gSet.ResetCmdArg();
        }
    }
    
    //_ASSERTE(pRCon->mh_ConChanged!=NULL);
    // Пока нить запускалась - произошел "аттач" так что все нормально...
    //_ASSERTE(pRCon->mb_Detached || pRCon->mh_ConEmuC!=NULL);

    BOOL bDetached = pRCon->m_Args.bDetached;
    //TODO: а тут мы будем читать консоль...
    #define IDEVENT_TERM  0
    #define IDEVENV_MONITORTHREADEVENT 1
    //#define IDEVENT_SYNC2WINDOW 2
    //#define IDEVENT_CURSORCHANGED 4
    //#define IDEVENT_PACKETARRIVED 2
    #define IDEVENT_CONCLOSED 2
    #define EVENTS_COUNT (IDEVENT_CONCLOSED+1)
    HANDLE hEvents[EVENTS_COUNT];
    hEvents[IDEVENT_TERM] = pRCon->mh_TermEvent;
    //hEvents[IDEVENT_CONCHANGED] = pRCon->mh_ConChanged;
    hEvents[IDEVENV_MONITORTHREADEVENT] = pRCon->mh_MonitorThreadEvent; // Использовать, чтобы вызвать Update & Invalidate
    //hEvents[IDEVENT_SYNC2WINDOW] = pRCon->mh_Sync2WindowEvent;
    //hEvents[IDEVENT_CURSORCHANGED] = pRCon->mh_CursorChanged;
    //hEvents[IDEVENT_PACKETARRIVED] = pRCon->mh_PacketArrived;
    hEvents[IDEVENT_CONCLOSED] = pRCon->mh_ConEmuC;
    DWORD  nEvents = /*bDetached ? (IDEVENT_SYNC2WINDOW+1) :*/ countof(hEvents);
    if (hEvents[IDEVENT_CONCLOSED] == NULL) nEvents --;
    _ASSERT(EVENTS_COUNT==countof(hEvents)); // проверить размерность

    DWORD  nWait = 0;
    BOOL   bException = FALSE, bIconic = FALSE, /*bFirst = TRUE,*/ bActive = TRUE;

    DWORD nElapse = max(10,gSet.nMainTimerElapse);

	DWORD nLastFarPID = 0;
	bool bLastAlive = false, bLastAliveActive = false;
	bool lbForceUpdate = false;

	WARNING("Переделать ожидание на hDataReadyEvent, который выставляется в сервере?");

    
    TODO("Нить не завершается при F10 в фаре - процессы пока не инициализированы...")
    while (TRUE)
    {
        bActive = pRCon->isActive();
        
        if (bActive)
        	gSet.Performance(tPerfInterval, TRUE); // считается по своему

        if (hEvents[IDEVENT_CONCLOSED] == NULL && pRCon->mh_ConEmuC /*&& pRCon->mh_CursorChanged*/
			&& WaitForSingleObject(hEvents[IDEVENV_MONITORTHREADEVENT],0) == WAIT_OBJECT_0)
		{
            bDetached = FALSE;
            //hEvents[IDEVENT_CURSORCHANGED] = pRCon->mh_CursorChanged;
            hEvents[IDEVENT_CONCLOSED] = pRCon->mh_ConEmuC;
            nEvents = countof(hEvents);
        }

        
        bIconic = gConEmu.isIconic();

        // в минимизированном/неактивном режиме - сократить расходы
        nWait = WaitForMultipleObjects(nEvents, hEvents, FALSE, (bIconic || !bActive) ? max(1000,nElapse) : nElapse);
        _ASSERTE(nWait!=(DWORD)-1);

        if (nWait == IDEVENT_TERM || nWait == IDEVENT_CONCLOSED) {
            if (nWait == IDEVENT_CONCLOSED) {
                DEBUGSTRPROC(L"### ConEmuC.exe was terminated\n");
            }
            break; // требование завершения нити
        }
		// Это событие теперь ManualReset
		if (nWait == IDEVENV_MONITORTHREADEVENT
			|| WaitForSingleObject(hEvents[IDEVENV_MONITORTHREADEVENT],0) == WAIT_OBJECT_0)
		{
			ResetEvent(hEvents[IDEVENV_MONITORTHREADEVENT]);
		}

        // Проверим, что ConEmuC жив
        if (pRCon->mh_ConEmuC) {
            DWORD dwExitCode = 0;
            #ifdef _DEBUG
            BOOL fSuccess = 
            #endif
            GetExitCodeProcess(pRCon->mh_ConEmuC, &dwExitCode);
            if (dwExitCode!=STILL_ACTIVE) {
                pRCon->StopSignal();
                return 0;
            }
        }

        // Если консоль не должна быть показана - но ее кто-то отобразил 
        if (!pRCon->isShowConsole && !gSet.isConVisible)
        {
            /*if (foreWnd == hConWnd)
                apiSetForegroundWindow(ghWnd);*/
			bool bMonitorVisibility = true;
			#ifdef _DEBUG
			if ((GetKeyState(VK_SCROLL) & 1))
				bMonitorVisibility = false;
			#endif
            if (bMonitorVisibility && IsWindowVisible(pRCon->hConWnd))
                apiShowWindow(pRCon->hConWnd, SW_HIDE);
        }

        // Размер консоли меняем в том треде, в котором это требуется. Иначе можем заблокироваться при Update (InitDC)
        // Требуется изменение размеров консоли
        /*if (nWait == (IDEVENT_SYNC2WINDOW)) {
            pRCon->SetConsoleSize(pRCon->m_ReqSetSize);
            //SetEvent(pRCon->mh_ReqSetSizeEnd);
            //continue; -- и сразу обновим информацию о ней
        }*/
        

        DWORD dwT1 = GetTickCount();

        SAFETRY
        {
            //ResetEvent(pRCon->mh_EndUpdateEvent);
            
			// Тут и ApplyConsole вызывается
            //if (pRCon->mp_ConsoleInfo)
            if (pRCon->m_ConsoleMap.IsValid())
            {
				// Alive?
				DWORD nCurFarPID = pRCon->GetFarPID();
				if (!nCurFarPID || nLastFarPID != nCurFarPID)
				{
					//pRCon->mn_LastFarReadIdx = -1;
					pRCon->mn_LastFarReadTick = 0;
					nLastFarPID = nCurFarPID;
					// Переоткрывать мэппинг при смене PID фара
					// (из одного фара запустили другой, который закрыли и вернулись в первый)
					pRCon->OpenFarMapData();
				}

				bool bAlive = false;
				//PRAGMA_ERROR("Переделать на мэппинг для mp_FarInfo");
				//if (nCurFarPID && pRCon->mn_LastFarReadIdx != pRCon->mp_ConsoleInfo->nFarReadIdx) {
				if (nCurFarPID && pRCon->m_FarInfo.IsValid() && pRCon->m_FarAliveEvent.Open())
				{
					DWORD nCurTick = GetTickCount();
					// Чтобы опрос события не шел слишком часто.
					if (pRCon->mn_LastFarReadTick == 0 ||
						(nCurTick - pRCon->mn_LastFarReadTick) >= (FAR_ALIVE_TIMEOUT/2))
					{
						//if (WaitForSingleObject(pRCon->mh_FarAliveEvent, 0) == WAIT_OBJECT_0)
						if (pRCon->m_FarAliveEvent.Wait(0) == WAIT_OBJECT_0)
						{
							pRCon->mn_LastFarReadTick = nCurTick ? nCurTick : 1;
							bAlive = true; // живой
						}
						#ifdef _DEBUG
						else
						{
							pRCon->mn_LastFarReadTick = nCurTick - FAR_ALIVE_TIMEOUT - 1;
							bAlive = false; // занят
						}
						#endif
					}
					else
					{
						bAlive = true; // еще не успело протухнуть
					}
					//if (pRCon->mn_LastFarReadIdx != pRCon->mp_FarInfo->nFarReadIdx) {
					//	pRCon->mn_LastFarReadIdx = pRCon->mp_FarInfo->nFarReadIdx;
					//	pRCon->mn_LastFarReadTick = GetTickCount();
					//	DEBUGSTRALIVE(L"*** FAR ReadTick updated\n");
					//	bAlive = true;
					//}
				}
				else
				{
					bAlive = true; // если нет фаровского плагина, или это не фар
				}
				//if (!bAlive) {
				//	bAlive = pRCon->isAlive();
				//}
				if (pRCon->isActive())
				{
					WARNING("Тут нужно бы сравнивать с переменной, хранящейся в gConEmu, а не в этом instance RCon!");
					#ifdef _DEBUG
					if (!IsDebuggerPresent())
					{
						bool lbIsAliveDbg = pRCon->isAlive();
						if (lbIsAliveDbg != bAlive) {
							_ASSERTE(lbIsAliveDbg == bAlive);
						}
					}
					#endif
					if (bLastAlive != bAlive || !bLastAliveActive)
					{
						DEBUGSTRALIVE(bAlive ? L"MonitorThread: Alive changed to TRUE\n" : L"MonitorThread: Alive changed to FALSE\n");
						PostMessage(ghWnd, WM_SETCURSOR, -1, -1);
					}
					bLastAliveActive = true;
				}
				else
				{
					bLastAliveActive = false;
				}
				bLastAlive = bAlive;


				// Загрузить изменения из консоли
				//if ((HWND)pRCon->mp_ConsoleInfo->hConWnd && pRCon->mp_ConsoleInfo->nCurDataMapIdx
				//	&& pRCon->mp_ConsoleInfo->nPacketId
				//	&& pRCon->mn_LastConsolePacketIdx != pRCon->mp_ConsoleInfo->nPacketId)
				WARNING("!!! Если ожидание m_ConDataChanged будет перенесно выше - то тут нужно пользовать полученное выше значение !!!");
				if (!pRCon->m_ConDataChanged.Wait(0,TRUE))
				{
            		pRCon->ApplyConsoleInfo();
            	}
        	}
            
			bool bCheckStatesFindPanels = false, lbForceUpdateProgress = false;

			// Если консоль неактивна - CVirtualConsole::Update не вызывается и диалоги не детектятся. А это требуется.
			if ((!bActive || bIconic) && pRCon->con.bConsoleDataChanged)
			{
				DWORD nCurTick = GetTickCount();
				DWORD nDelta = nCurTick - pRCon->con.nLastInactiveRgnCheck;
				if (nDelta > CONSOLEINACTIVERGNTIMEOUT)
				{
					pRCon->con.nLastInactiveRgnCheck = nCurTick;
					// Если при старте ConEmu создано несколько консолей через '@'
					// то все кроме активной - не инициализированы (InitDC не вызывался),
					// что нужно делать в главной нити, LoadConsoleData об этом позаботится
					if (pRCon->mp_VCon->LoadConsoleData())
						bCheckStatesFindPanels = true;
				}
			}


			// обновить статусы, найти панели, и т.п.
			if (pRCon->mb_DataChanged || pRCon->mb_TabsWasChanged)
			{
				lbForceUpdate = true; // чтобы если консоль неактивна - не забыть при ее активации передернуть что нужно...
				pRCon->mb_TabsWasChanged = FALSE;
				pRCon->mb_DataChanged = FALSE;
				// Функция загружает ТОЛЬКО ms_PanelTitle, чтобы показать 
				// корректный текст в закладке, соответсвующей панелям
			    pRCon->CheckPanelTitle();
				// выполнит ниже CheckFarStates & FindPanels
				bCheckStatesFindPanels = true;
			}
			if (!bCheckStatesFindPanels)
			{
				// Если была обнаружена "ошибка" прогресса - проверить заново.
				// Это может также произойти при извлечении файла из архива через MA.
				// Проценты бегут (панелей нет), проценты исчезают, панели появляются, но
				// пока не произойдет хоть каких-нибудь изменений в консоли - статус не обновлялся.
				if (pRCon->mn_LastWarnCheckTick || pRCon->mn_FarStatus & (CES_WASPROGRESS|CES_OPER_ERROR))
					bCheckStatesFindPanels = true;
			}
			if (bCheckStatesFindPanels)
			{
				// Статусы mn_FarStatus & mn_PreWarningProgress
			    pRCon->CheckFarStates();
				// Если возможны панели - найти их в консоли,
				// заодно оттуда позовется CheckProgressInConsole
				pRCon->FindPanels();
			}
			if (pRCon->mn_ConsoleProgress == -1 && pRCon->mn_LastConsoleProgress >= 0)
			{
				// Пока бежит запаковка 7z - иногда попадаем в момент, когда на новой строке процентов еще нет
				DWORD nDelta = GetTickCount() - pRCon->mn_LastConProgrTick;
				if (nDelta >= CONSOLEPROGRESSTIMEOUT)
				{
					pRCon->mn_LastConsoleProgress = -1; pRCon->mn_LastConProgrTick = 0;
					lbForceUpdateProgress = true;
				}
			}


            if (pRCon->hConWnd) // Если знаем хэндл окна - 
                GetWindowText(pRCon->hConWnd, pRCon->TitleCmp, countof(pRCon->TitleCmp)-2);

			// возможно, требуется сбросить прогресс
			//bool lbCheckProgress = (pRCon->mn_PreWarningProgress != -1);

			if (pRCon->mb_ForceTitleChanged
                || wcscmp(pRCon->Title, pRCon->TitleCmp))
            {
				pRCon->mb_ForceTitleChanged = FALSE;
                pRCon->OnTitleChanged();
				lbForceUpdateProgress = false; // прогресс обновлен

            }
			else if (bActive)
			{
				// Если в консоли заголовок не менялся, но он отличается от заголовка в ConEmu
                if (wcscmp(pRCon->TitleFull, gConEmu.GetLastTitle(false)))
                    gConEmu.UpdateTitle(/*pRCon->TitleFull*/); // 100624 - заголовок сам перечитает
            }

            if (lbForceUpdateProgress)
			{
            	gConEmu.UpdateProgress();
            }
            
			//if (lbCheckProgress && pRCon->mn_LastShownProgress >= 0) {
			//	if (pRCon->GetProgress(NULL) != -1) {
			//		pRCon->OnTitleChanged();
			//	}
			//	//DWORD nDelta = GetTickCount() - pRCon->mn_LastProgressTick;
			//	//if (nDelta >= 500) {
			//	//}
			//}

			bool lbIsActive = pRCon->isActive();
			#ifdef _DEBUG
			if (pRCon->con.bDebugLocked)
				lbIsActive = false;
			#endif

			if (lbIsActive)
			{
				//2009-01-21 сомнительно, что здесь действительно нужно подресайзивать дочерние окна
				//if (lbForceUpdate) // размер текущего консольного окна был изменен
				//	gConEmu.OnSize(-1); // послать в главную нить запрос на обновление размера

				bool lbNeedRedraw = false;
				if ((nWait == (WAIT_OBJECT_0+1)) || lbForceUpdate)
				{
					//2010-05-18 lbForceUpdate вызывал CVirtualConsole::Update(abForce=true), что приводило к тормозам
					bool bForce = false; //lbForceUpdate;
					lbForceUpdate = false;
					gConEmu.m_Child->Validate(); // сбросить флажок
					if (pRCon->m_UseLogs>2) pRCon->LogString("mp_VCon->Update from CRealConsole::MonitorThread");
					if (pRCon->mp_VCon->Update(bForce))
						lbNeedRedraw = true;
				}
				else if (gSet.isCursorBlink)
				{
					// Возможно, настало время мигнуть курсором?
					bool lbNeedBlink = false;
					pRCon->mp_VCon->UpdateCursor(lbNeedBlink);
					// UpdateCursor Invalidate не зовет
					if (lbNeedBlink)
					{
						if (pRCon->m_UseLogs>2) pRCon->LogString("Invalidating from CRealConsole::MonitorThread.1");
						lbNeedRedraw = true;
					}
				}
				else if (((GetTickCount() - pRCon->mn_LastInvalidateTick) > FORCE_INVALIDATE_TIMEOUT))
				{
					DEBUGSTRDRAW(L"+++ Force invalidate by timeout\n");
					if (pRCon->m_UseLogs>2) pRCon->LogString("Invalidating from CRealConsole::MonitorThread.2");
					lbNeedRedraw = true;
				}
				// Если нужна отрисовка - дернем основную нить
				if (lbNeedRedraw)
				{
					//#ifndef _DEBUG
					//WARNING("******************");
					TODO("После этого двойная отрисовка не вызывается?");
					gConEmu.m_Child->Redraw();
					//#endif
					pRCon->mn_LastInvalidateTick = GetTickCount();
				}
			}

        } SAFECATCH {
            bException = TRUE;
        }


		// Чтобы не было слишком быстрой отрисовки (тогда процессор загружается на 100%)
		// делаем такой расчет задержки
        DWORD dwT2 = GetTickCount();
        DWORD dwD = max(10,(dwT2 - dwT1));
        nElapse = (DWORD)(nElapse*0.7 + dwD*0.3);
        if (nElapse > 1000) nElapse = 1000; // больше секунды - не ждать! иначе курсор мигать не будет

        if (bException)
		{
			bException = FALSE;
            #ifdef _DEBUG
            _ASSERT(FALSE);
            #endif
            pRCon->Box(_T("Exception triggered in CRealConsole::MonitorThread"));
        }

        //if (bActive)
        //	gSet.Performance(tPerfInterval, FALSE);

		if (pRCon->m_ServerClosing.nServerPID
			&& pRCon->m_ServerClosing.nServerPID == pRCon->mn_ConEmuC_PID
			&& (GetTickCount() - pRCon->m_ServerClosing.nRecieveTick) >= SERVERCLOSETIMEOUT)
		{
			// Видимо, сервер повис во время выхода?
			pRCon->isConsoleClosing(); // функция позовет TerminateProcess(mh_ConEmuC)
			nWait = IDEVENT_CONCLOSED;
			break;
		}
    }

	pRCon->StopSignal();

	// 2010-08-03 - перенес в StopThread, чтобы не задерживать закрытие ЭТОЙ нити
	//// Завершение серверных нитей этой консоли
	//DEBUGSTRPROC(L"About to terminate main server thread (MonitorThread)\n");
	//if (pRCon->ms_VConServer_Pipe[0]) // значит хотя бы одна нить была запущена
	//{   
	//    pRCon->StopSignal(); // уже должен быть выставлен, но на всякий случай
	//    //
	//    HANDLE hPipe = INVALID_HANDLE_VALUE;
	//    DWORD dwWait = 0;
	//    // Передернуть пайпы, чтобы нити сервера завершились
	//    for (int i=0; i<MAX_SERVER_THREADS; i++) {
	//        DEBUGSTRPROC(L"Touching our server pipe\n");
	//        HANDLE hServer = pRCon->mh_ActiveRConServerThread;
	//        hPipe = CreateFile(pRCon->ms_VConServer_Pipe,GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
	//        if (hPipe == INVALID_HANDLE_VALUE) {
	//            DEBUGSTRPROC(L"All pipe instances closed?\n");
	//            break;
	//        }
	//        DEBUGSTRPROC(L"Waiting server pipe thread\n");
	//        dwWait = WaitForSingleObject(hServer, 200); // пытаемся дождаться, пока нить завершится
	//        // Просто закроем пайп - его нужно было передернуть
	//        CloseHandle(hPipe);
	//        hPipe = INVALID_HANDLE_VALUE;
	//    }
	//    // Немного подождем, пока ВСЕ нити завершатся
	//    DEBUGSTRPROC(L"Checking server pipe threads are closed\n");
	//    WaitForMultipleObjects(MAX_SERVER_THREADS, pRCon->mh_RConServerThreads, TRUE, 500);
	//    for (int i=0; i<MAX_SERVER_THREADS; i++) {
	//        if (WaitForSingleObject(pRCon->mh_RConServerThreads[i],0) != WAIT_OBJECT_0) {
	//            DEBUGSTRPROC(L"### Terminating mh_RConServerThreads\n");
	//            TerminateThread(pRCon->mh_RConServerThreads[i],0);
	//        }
	//        CloseHandle(pRCon->mh_RConServerThreads[i]);
	//        pRCon->mh_RConServerThreads[i] = NULL;
	//    }
	//	pRCon->ms_VConServer_Pipe[0] = 0;
	//}
    
    // Finalize
    //SafeCloseHandle(pRCon->mh_MonitorThread);

    DEBUGSTRPROC(L"Leaving MonitorThread\n");
    
    return 0;
}

BOOL CRealConsole::PreInit(BOOL abCreateBuffers/*=TRUE*/)
{
	MCHKHEAP;

    // Инициализировать переменные m_sbi, m_ci, m_sel
    RECT rcWnd; GetClientRect(ghWnd, &rcWnd);
    // isBufferHeight использовать нельзя, т.к. con.m_sbi еще не инициализирован!
	bool bNeedBufHeight = (gSet.bForceBufferHeight && gSet.nForceBufferHeight>0)
		|| (!gSet.bForceBufferHeight && con.DefaultBufferHeight);
    if (bNeedBufHeight && !con.bBufferHeight)
	{
		MCHKHEAP;
        SetBufferHeightMode(TRUE);
		MCHKHEAP;
		_ASSERTE(con.DefaultBufferHeight>0);
        BufferHeight(con.DefaultBufferHeight);
    }

	MCHKHEAP;

    //if (con.bBufferHeight) {
    //  // скорректировать ширину окна на ширину появляющейся полосы прокрутки
    //  if (!gConEmu.ActiveCon()->RCon()->isBufferHeight())
    //      rcWnd.right -= GetSystemMetrics(SM_CXVSCROLL);
    //} else {
    //  // // скорректировать ширину окна на ширину убирающейся полосы прокрутки
    //  if (gConEmu.ActiveCon()->RCon()->isBufferHeight())
    //      rcWnd.right += GetSystemMetrics(SM_CXVSCROLL);
    //}
    RECT rcCon;
    if (gConEmu.isIconic())
        rcCon = MakeRect(gSet.wndWidth, gSet.wndHeight);
    else
        rcCon = gConEmu.CalcRect(CER_CONSOLE, rcWnd, CER_MAINCLIENT);
    _ASSERTE(rcCon.right!=0 && rcCon.bottom!=0);
    if (con.bBufferHeight)
	{
		_ASSERTE(con.DefaultBufferHeight>0);
        con.m_sbi.dwSize = MakeCoord(rcCon.right,con.DefaultBufferHeight);
    }
	else
	{
        con.m_sbi.dwSize = MakeCoord(rcCon.right,rcCon.bottom);
    }
    con.m_sbi.wAttributes = 7;
    con.m_sbi.srWindow.Right = rcCon.right-1; con.m_sbi.srWindow.Bottom = rcCon.bottom-1;
    con.m_sbi.dwMaximumWindowSize = con.m_sbi.dwSize;
    con.m_ci.dwSize = 15; con.m_ci.bVisible = TRUE;
    if (!InitBuffers(0))
        return FALSE;
    return TRUE;
}

BOOL CRealConsole::StartMonitorThread()
{
    BOOL lbRc = FALSE;

    _ASSERT(mh_MonitorThread==NULL);
    //_ASSERT(mh_InputThread==NULL);
    //_ASSERTE(mb_Detached || mh_ConEmuC!=NULL); -- процесс теперь запускаем в MonitorThread

	SetConStatus(L"Initializing ConEmu...");

    mh_MonitorThread = CreateThread(NULL, 0, MonitorThread, (LPVOID)this, 0, &mn_MonitorThreadID);
    
    //mh_InputThread = CreateThread(NULL, 0, InputThread, (LPVOID)this, 0, &mn_InputThreadID);

    if (mh_MonitorThread == NULL /*|| mh_InputThread == NULL*/)
	{
        DisplayLastError(_T("Can't create console thread!"));
    }
	else
	{
        //lbRc = SetThreadPriority(mh_MonitorThread, THREAD_PRIORITY_ABOVE_NORMAL);
        lbRc = TRUE;
    }

    return lbRc;
}

BOOL CRealConsole::StartProcess()
{
    BOOL lbRc = FALSE;

	SetConStatus(L"Preparing process startup line...");

    if (!mb_ProcessRestarted)
	{
        if (!PreInit())
            return FALSE;
    }

    
	mb_UseOnlyPipeInput = FALSE;
    
    if (mp_sei)
	{
    	SafeCloseHandle(mp_sei->hProcess);
    	GlobalFree(mp_sei); mp_sei = NULL;
    }

	//ResetEvent(mh_CreateRootEvent);
	mb_InCreateRoot = TRUE;
	mb_InCloseConsole = FALSE;
	ZeroStruct(m_ServerClosing);

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    wchar_t szInitConTitle[255];
	MCHKHEAP;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW|STARTF_USECOUNTCHARS|STARTF_USESIZE/*|STARTF_USEPOSITION*/;
    si.lpTitle = wcscpy(szInitConTitle, CEC_INITTITLE);
	// К сожалению, можно задать только размер БУФЕРА в символах.
	si.dwXCountChars = con.m_sbi.dwSize.X;
	si.dwYCountChars = con.m_sbi.dwSize.Y;
	// Размер окна нужно задавать в пикселях, а мы заранее не знаем сколько будет нужно...
	// Но можно задать хоть что-то, чтобы окошко сразу не разъехалось (в расчете на шрифт 4*6)...
	if (con.bBufferHeight)
	{
		si.dwXSize = 4 * con.m_sbi.dwSize.X + 2*GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXVSCROLL);
		si.dwYSize = 6 * con.nTextHeight + 2*GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYCAPTION);
	}
	else
	{
		si.dwXSize = 4 * con.m_sbi.dwSize.X + 2*GetSystemMetrics(SM_CXFRAME);
		si.dwYSize = 6 * con.m_sbi.dwSize.Y + 2*GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYCAPTION);
	}
	// Если просят "отладочный" режим - покажем окошко
    si.wShowWindow = gSet.isConVisible ? SW_SHOWNORMAL : SW_HIDE;
    //RECT rcDC; GetWindowRect(ghWndDC, &rcDC);
    //si.dwX = rcDC.left; si.dwY = rcDC.top;
    ZeroMemory( &pi, sizeof(pi) );
	MCHKHEAP;

    int nStep = (m_Args.pszSpecialCmd!=NULL) ? 2 : 1;
    wchar_t* psCurCmd = NULL;
    while (nStep <= 2)
    {
        MCHKHEAP;
        /*if (!*gSet.GetCmd()) {
            gSet.psCurCmd = _tcsdup(gSet.Buffer Height == 0 ? _T("far") : _T("cmd"));
            nStep ++;
        }*/

		MCHKHEAP;
        LPCWSTR lpszCmd = NULL;
        if (m_Args.pszSpecialCmd)
            lpszCmd = m_Args.pszSpecialCmd;
        else
            lpszCmd = gSet.GetCmd();

        int nLen = _tcslen(lpszCmd);
        TCHAR *pszSlash=NULL;
        nLen += _tcslen(gConEmu.ms_ConEmuCExe) + 280 + MAX_PATH;
		MCHKHEAP;
        psCurCmd = (wchar_t*)malloc(nLen*sizeof(wchar_t));
        _ASSERTE(psCurCmd);
        wcscpy(psCurCmd, L"\"");
        wcscat(psCurCmd, gConEmu.ms_ConEmuExe);
        pszSlash = wcsrchr(psCurCmd, _T('\\'));
        MCHKHEAP;
		wcscpy(pszSlash+1, gConEmu.ms_ConEmuCExeName);
		wcscat(pszSlash+1, L"\" ");
		//if (IsWindows64())
		//	wcscpy(pszSlash+1, L"ConEmuC64.exe\" ");
		//else
		//	wcscpy(pszSlash+1, L"ConEmuC.exe\" ");

		if (m_Args.bRunAsAdministrator)
		{
			m_Args.bDetached = TRUE;
			wcscat(pszSlash, L" /ATTACH ");
		}

        int nWndWidth = con.m_sbi.dwSize.X;
        int nWndHeight = con.m_sbi.dwSize.Y;
        GetConWindowSize(con.m_sbi, nWndWidth, nWndHeight);
        wsprintf(psCurCmd+wcslen(psCurCmd), 
        	L"/GID=%i /BW=%i /BH=%i /BZ=%i \"/FN=%s\" /FW=%i /FH=%i", 
            GetCurrentProcessId(), nWndWidth, nWndHeight, con.DefaultBufferHeight,
            //(con.bBufferHeight ? gSet.Default BufferHeight : 0), // пусть с буфером сервер разбирается
            gSet.ConsoleFont.lfFaceName, gSet.ConsoleFont.lfWidth, gSet.ConsoleFont.lfHeight);
        /*if (gSet.FontFile[0]) { --  РЕГИСТРАЦИЯ ШРИФТА НА КОНСОЛЬ НЕ РАБОТАЕТ!
            wcscat(psCurCmd, L" \"/FF=");
            wcscat(psCurCmd, gSet.FontFile);
            wcscat(psCurCmd, L"\"");
        }*/
        if (m_UseLogs) wcscat(psCurCmd, (m_UseLogs==3) ? L" /LOG3" : (m_UseLogs==2) ? L" /LOG2" : L" /LOG");
		if (!gSet.isConVisible) wcscat(psCurCmd, L" /HIDE");
        wcscat(psCurCmd, L" /ROOT ");
        wcscat(psCurCmd, lpszCmd);
        MCHKHEAP

		DWORD dwLastError = 0;

        #ifdef MSGLOGGER
        DEBUGSTRPROC(psCurCmd);DEBUGSTRPROC(_T("\n"));
        #endif
        
		if (!m_Args.bRunAsAdministrator)
		{
			LockSetForegroundWindow ( LSFW_LOCK );

			SetConStatus(L"Starting root process...");
			if (m_Args.pszUserName != NULL)
			{
				if (CreateProcessWithLogonW(m_Args.pszUserName, NULL, m_Args.szUserPassword,
					LOGON_WITH_PROFILE, NULL, psCurCmd,
					NORMAL_PRIORITY_CLASS|CREATE_DEFAULT_ERROR_MODE|CREATE_NEW_CONSOLE
					, NULL, m_Args.pszStartupDir, &si, &pi))
				//if (CreateProcessAsUser(m_Args.hLogonToken, NULL, psCurCmd, NULL, NULL, FALSE,
				//	NORMAL_PRIORITY_CLASS|CREATE_DEFAULT_ERROR_MODE|CREATE_NEW_CONSOLE
				//	, NULL, m_Args.pszStartupDir, &si, &pi))
				{
					lbRc = TRUE;
					mn_ConEmuC_PID = pi.dwProcessId;
				} else {
					dwLastError = GetLastError();
				}
				SecureZeroMemory(m_Args.szUserPassword, sizeof(m_Args.szUserPassword));

			} else if (m_Args.bRunAsRestricted)
			{
				HANDLE hToken = NULL, hTokenRest = NULL;
				lbRc = FALSE;
				if (OpenProcessToken(GetCurrentProcess(), 
						//TOKEN_ASSIGN_PRIMARY|TOKEN_DUPLICATE|TOKEN_QUERY|TOKEN_ADJUST_DEFAULT,
						TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_QUERY,
						&hToken))
				{
					enum WellKnownAuthorities {
						NullAuthority = 0, WorldAuthority, LocalAuthority, CreatorAuthority, 
						NonUniqueAuthority, NtAuthority, MandatoryLabelAuthority = 16
					};
					SID *pAdmSid = (SID*)calloc(sizeof(SID)+sizeof(DWORD)*2,1);
						pAdmSid->Revision = SID_REVISION;
						pAdmSid->SubAuthorityCount = 2;
						pAdmSid->IdentifierAuthority.Value[5] = NtAuthority;
						pAdmSid->SubAuthority[0] = SECURITY_BUILTIN_DOMAIN_RID;
						pAdmSid->SubAuthority[1] = DOMAIN_ALIAS_RID_ADMINS;
					SID_AND_ATTRIBUTES sidsToDisable[] = {
						{pAdmSid}
					};
					if (CreateRestrictedToken(hToken, DISABLE_MAX_PRIVILEGE, 
							countof(sidsToDisable), sidsToDisable, 
							0, NULL, 0, NULL, &hTokenRest))
					{
						if (CreateProcessAsUserW(hTokenRest, NULL, psCurCmd, NULL, NULL, FALSE,
								NORMAL_PRIORITY_CLASS|CREATE_DEFAULT_ERROR_MODE|CREATE_NEW_CONSOLE
								, NULL, m_Args.pszStartupDir, &si, &pi))
						{
							lbRc = TRUE;
							mn_ConEmuC_PID = pi.dwProcessId;
						}
						CloseHandle(hTokenRest); hTokenRest = NULL;
					} else {
						dwLastError = GetLastError();
					}
					free(pAdmSid);
					CloseHandle(hToken); hToken = NULL;
				} else {
					dwLastError = GetLastError();
				}
			} else {
				lbRc = CreateProcess(NULL, psCurCmd, NULL, NULL, FALSE, 
					NORMAL_PRIORITY_CLASS|CREATE_DEFAULT_ERROR_MODE|CREATE_NEW_CONSOLE
					//|CREATE_NEW_PROCESS_GROUP - низя! перестает срабатывать Ctrl-C
					, NULL, m_Args.pszStartupDir, &si, &pi);
				if (!lbRc)
				{
					dwLastError = GetLastError();
				} else {
					mn_ConEmuC_PID = pi.dwProcessId;
				}
			}
			DEBUGSTRPROC(_T("CreateProcess finished\n"));

			//if (m_Args.hLogonToken) { CloseHandle(m_Args.hLogonToken); m_Args.hLogonToken = NULL; }

			LockSetForegroundWindow ( LSFW_UNLOCK );

		} else {
			LPCWSTR pszCmd = psCurCmd;
			wchar_t szExec[MAX_PATH+1];
			if (NextArg(&pszCmd, szExec) != 0)
			{
				lbRc = FALSE;
				dwLastError = -1;
			}
			else
			{
                if (mp_sei)
				{
                	SafeCloseHandle(mp_sei->hProcess);
                	GlobalFree(mp_sei); mp_sei = NULL;
                }

                wchar_t szCurrentDirectory[MAX_PATH+1];
				if (m_Args.pszStartupDir)
					wcscpy(szCurrentDirectory, m_Args.pszStartupDir);
				else if (!GetCurrentDirectory(MAX_PATH+1, szCurrentDirectory))
                	szCurrentDirectory[0] = 0;
                
                int nWholeSize = sizeof(SHELLEXECUTEINFO)
                	+ sizeof(wchar_t) *
                	  ( 10 /* Verb */
                	  + wcslen(szExec)+2
                	  + ((pszCmd == NULL) ? 0 : (wcslen(pszCmd)+2))
                	  + wcslen(szCurrentDirectory) + 2
                	  );
				mp_sei = (SHELLEXECUTEINFO*)GlobalAlloc(GPTR, nWholeSize);
				mp_sei->hwnd = ghWnd;
				mp_sei->cbSize = sizeof(SHELLEXECUTEINFO);
				//mp_sei->hwnd = /*NULL; */ ghWnd; // почему я тут NULL ставил?
				mp_sei->fMask = SEE_MASK_NO_CONSOLE|SEE_MASK_NOCLOSEPROCESS;
				mp_sei->lpVerb = (wchar_t*)(mp_sei+1);
					wcscpy((wchar_t*)mp_sei->lpVerb, L"runas");
				mp_sei->lpFile = mp_sei->lpVerb + wcslen(mp_sei->lpVerb) + 2;
					wcscpy((wchar_t*)mp_sei->lpFile, szExec);
				mp_sei->lpParameters = mp_sei->lpFile + wcslen(mp_sei->lpFile) + 2;
					if (pszCmd)
					{
						*(wchar_t*)mp_sei->lpParameters = L' ';
						wcscpy((wchar_t*)(mp_sei->lpParameters+1), pszCmd);
					}
				mp_sei->lpDirectory = mp_sei->lpParameters + wcslen(mp_sei->lpParameters) + 2;
					if (szCurrentDirectory[0])
						wcscpy((wchar_t*)mp_sei->lpDirectory, szCurrentDirectory);
					else
						mp_sei->lpDirectory = NULL;
				//mp_sei->nShow = gSet.isConVisible ? SW_SHOWNORMAL : SW_HIDE;
				mp_sei->nShow = SW_SHOWMINIMIZED;
				SetConStatus(L"Starting root process as user...");
				lbRc = gConEmu.GuiShellExecuteEx(mp_sei, TRUE);
				// ошибку покажем дальше
				dwLastError = GetLastError();
			}
		}

        

        if (lbRc)
        {
			if (!m_Args.bRunAsAdministrator)
			{
				ProcessUpdate(&pi.dwProcessId, 1);

				AllowSetForegroundWindow(pi.dwProcessId);
			}
            apiSetForegroundWindow(ghWnd);

            DEBUGSTRPROC(_T("CreateProcess OK\n"));
            lbRc = TRUE;

            /*if (!AttachPID(pi.dwProcessId)) {
                DEBUGSTRPROC(_T("AttachPID failed\n"));
				SetEvent(mh_CreateRootEvent); mb_InCreateRoot = FALSE;
                return FALSE;
            }
            DEBUGSTRPROC(_T("AttachPID OK\n"));*/

            break; // OK, запустили
        } else {
            //Box("Cannot execute the command.");
            //DWORD dwLastError = GetLastError();
            DEBUGSTRPROC(_T("CreateProcess failed\n"));
            int nLen = _tcslen(psCurCmd);
            TCHAR* pszErr=(TCHAR*)Alloc(nLen+100,sizeof(TCHAR));
            
            if (0==FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, dwLastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                pszErr, 1024, NULL))
            {
                wsprintf(pszErr, _T("Unknown system error: 0x%x"), dwLastError);
            }
            
            nLen += _tcslen(pszErr);
            TCHAR* psz=(TCHAR*)Alloc(nLen+100,sizeof(TCHAR));
            int nButtons = MB_OK|MB_ICONEXCLAMATION|MB_SETFOREGROUND;
            
            _tcscpy(psz, _T("Cannot execute the command.\r\n"));
            _tcscat(psz, psCurCmd); _tcscat(psz, _T("\r\n"));
            _tcscat(psz, pszErr);
            if (m_Args.pszSpecialCmd == NULL)
            {
                if (psz[_tcslen(psz)-1]!=_T('\n')) _tcscat(psz, _T("\r\n"));
                if (!gSet.psCurCmd && StrStrI(gSet.GetCmd(), gSet.GetDefaultCmd())==NULL)
				{
                    _tcscat(psz, _T("\r\n\r\n"));
                    _tcscat(psz, _T("Do You want to simply start "));
                    _tcscat(psz, gSet.GetDefaultCmd());
                    _tcscat(psz, _T("?"));
                    nButtons |= MB_YESNO;
                }
            }
            MCHKHEAP
            //Box(psz);
            int nBrc = MessageBox(NULL, psz, _T("ConEmu"), nButtons);
            Free(psz); Free(pszErr);
            if (nBrc!=IDYES)
			{
                // ??? Может ведь быть НЕСКОЛЬКО консолей. Нельзя так разрушать основное окно!
                //gConEmu.Destroy();
				//SetEvent(mh_CreateRootEvent);
				mb_InCreateRoot = FALSE;
				CloseConsole();
                return FALSE;
            }
            // Выполнить стандартную команду...
            if (m_Args.pszSpecialCmd == NULL)
            {
                gSet.psCurCmd = _tcsdup(gSet.GetDefaultCmd());
            }
            nStep ++;
            MCHKHEAP
            if (psCurCmd) free(psCurCmd); psCurCmd = NULL;
        }
    }

    MCHKHEAP
    if (psCurCmd) free(psCurCmd); psCurCmd = NULL;
    MCHKHEAP

    //TODO: а делать ли это?
    SafeCloseHandle(pi.hThread); pi.hThread = NULL;
    //CloseHandle(pi.hProcess); pi.hProcess = NULL;
    mn_ConEmuC_PID = pi.dwProcessId;
    mh_ConEmuC = pi.hProcess; pi.hProcess = NULL;

	if (!m_Args.bRunAsAdministrator)
	{
		CreateLogFiles();
    
		//// Событие "изменения" консоль //2009-05-14 Теперь события обрабатываются в GUI, но прийти из консоли может изменение размера курсора
		//wsprintfW(ms_ConEmuC_Pipe, CE_CURSORUPDATE, mn_ConEmuC_PID);
		//mh_CursorChanged = CreateEvent ( NULL, FALSE, FALSE, ms_ConEmuC_Pipe );
        
		// Инициализировать имена пайпов, событий, мэппингов и т.п.
		InitNames();
		//// Имя пайпа для управления ConEmuC
		//wsprintfW(ms_ConEmuC_Pipe, CESERVERPIPENAME, L".", mn_ConEmuC_PID);
		//wsprintfW(ms_ConEmuCInput_Pipe, CESERVERINPUTNAME, L".", mn_ConEmuC_PID);
		//MCHKHEAP
	}

	//SetEvent(mh_CreateRootEvent);
	mb_InCreateRoot = FALSE;

    return lbRc;
}

// Инициализировать имена пайпов, событий, мэппингов и т.п.
// Только имена - реальных хэндлов еще может не быть!
void CRealConsole::InitNames()
{
	// Имя пайпа для управления ConEmuC
	wsprintfW(ms_ConEmuC_Pipe, CESERVERPIPENAME, L".", mn_ConEmuC_PID);
	wsprintfW(ms_ConEmuCInput_Pipe, CESERVERINPUTNAME, L".", mn_ConEmuC_PID);

	// Имя событие измененности данных в консоли
	m_ConDataChanged.InitName(CEDATAREADYEVENT, mn_ConEmuC_PID);
	//wsprintfW(ms_ConEmuC_DataReady, CEDATAREADYEVENT, mn_ConEmuC_PID);

	MCHKHEAP;

	m_GetDataPipe.InitName(L"ConEmu", CESERVERREADNAME, L".", mn_ConEmuC_PID);
}

// Если включена прокрутка - скорректировать индекс ячейки из экранных в буферные
COORD CRealConsole::ScreenToBuffer(COORD crMouse)
{
	if (!this)
		return crMouse;
	if (isBufferHeight())
	{
		crMouse.X += con.m_sbi.srWindow.Left;
		crMouse.Y += con.m_sbi.srWindow.Top;
	}
	return crMouse;
}


// x,y - экранные координаты
void CRealConsole::OnMouse(UINT messg, WPARAM wParam, int x, int y)
{
    #ifndef WM_MOUSEHWHEEL
    #define WM_MOUSEHWHEEL                  0x020E
    #endif

#ifdef _DEBUG
	wchar_t szDbg[60]; wsprintf(szDbg, L"RCon::MouseEvent at DC {%ix%i}\n", x,y);
	DEBUGSTRINPUT(szDbg);
#endif

    if (!this || !hConWnd)
        return;
        
	if (messg != WM_MOUSEMOVE)
	{
		mcr_LastMouseEventPos.X = mcr_LastMouseEventPos.Y = -1;
	}
        

    //BOOL lbStdMode = FALSE;
    //if (!con.bBufferHeight)
    //    lbStdMode = TRUE;
    if (isSelectionAllowed())
	{
    	if (messg == WM_LBUTTONDOWN)
		{
	    	// Начало обработки выделения
	    	if (OnMouseSelection(messg, wParam, x, y))
	    		return;
    	}
    }

	if (gSet.isCTSRBtnAction == 2 && (messg == WM_RBUTTONDOWN || messg == WM_RBUTTONUP))
	{
		if (messg == WM_RBUTTONUP) Paste();
		return;
	}
	if (gSet.isCTSMBtnAction == 2 && (messg == WM_MBUTTONDOWN || messg == WM_MBUTTONUP))
	{
		if (messg == WM_MBUTTONUP) Paste();
		return;
	}
    
	BOOL lbFarBufferSupported = isFarBufferSupported();
    if (con.bBufferHeight && !lbFarBufferSupported)
	{
        if (messg == WM_MOUSEWHEEL)
		{
            SHORT nDir = (SHORT)HIWORD(wParam);
            BOOL lbCtrl = isPressed(VK_CONTROL);
            if (nDir > 0)
			{
                OnScroll(lbCtrl ? SB_PAGEUP : SB_LINEUP);
                //OnScroll(SB_PAGEUP);
            }
			else if (nDir < 0)
			{
                OnScroll(lbCtrl ? SB_PAGEDOWN : SB_LINEDOWN);
                //OnScroll(SB_PAGEDOWN);
            }
        }
        
        if (!isConSelectMode())
        	return;
    }
    
    //if (isConSelectMode()) -- это неправильно. она реагирует и на фаровский граббер (чтобы D&D не взлетал)
    if (con.m_sel.dwFlags != 0)
	{
    	// Ручная обработка выделения, на консоль полагаться не следует...
    	OnMouseSelection(messg, wParam, x, y);
    	return;
    }

    
    // Если консоль в режиме с прокруткой - не посылать мышь в консоль
    // Иначе получаются казусы. Если во время выполнения команды (например "dir c: /s")
    // успеть дернуть мышкой - то при возврате в ФАР сразу пойдет фаровский драг
    if (isBufferHeight() && !lbFarBufferSupported)
    	return;
    	

	// Получить известные координаты символов
	COORD crMouse = ScreenToBuffer(mp_VCon->ClientToConsole(x,y));
	//if (isBufferHeight()) {
	//	crMouse.X += con.m_sbi.srWindow.Left;
	//	crMouse.Y += con.m_sbi.srWindow.Top;
	//}

	//2010-07-12 - перенес вниз
	//if (messg == WM_MOUSEMOVE /*&& mb_MouseButtonDown*/) {
	//	// Issue 172: ConEmu10020304: проблема с правым кликом на PanelTabs
	//	if (mcr_LastMouseEventPos.X == crMouse.X && mcr_LastMouseEventPos.Y == crMouse.Y)
	//		return; // не посылать в консоль MouseMove на том же месте
	//	mcr_LastMouseEventPos.X = crMouse.X; mcr_LastMouseEventPos.Y = crMouse.Y;
	//}

	INPUT_RECORD r; memset(&r, 0, sizeof(r));
    r.EventType = MOUSE_EVENT;
    
    // Mouse Buttons
    if (messg != WM_LBUTTONUP && (messg == WM_LBUTTONDOWN || messg == WM_LBUTTONDBLCLK || isPressed(VK_LBUTTON)))
        r.Event.MouseEvent.dwButtonState |= FROM_LEFT_1ST_BUTTON_PRESSED;
    if (messg != WM_RBUTTONUP && (messg == WM_RBUTTONDOWN || messg == WM_RBUTTONDBLCLK || isPressed(VK_RBUTTON)))
        r.Event.MouseEvent.dwButtonState |= RIGHTMOST_BUTTON_PRESSED;
    if (messg != WM_MBUTTONUP && (messg == WM_MBUTTONDOWN || messg == WM_MBUTTONDBLCLK || isPressed(VK_MBUTTON)))
        r.Event.MouseEvent.dwButtonState |= FROM_LEFT_2ND_BUTTON_PRESSED;
    if (messg != WM_XBUTTONUP && (messg == WM_XBUTTONDOWN || messg == WM_XBUTTONDBLCLK))
	{
        if ((HIWORD(wParam) & 0x0001/*XBUTTON1*/))
            r.Event.MouseEvent.dwButtonState |= 0x0008/*FROM_LEFT_3ND_BUTTON_PRESSED*/;
        else if ((HIWORD(wParam) & 0x0002/*XBUTTON2*/))
            r.Event.MouseEvent.dwButtonState |= 0x0010/*FROM_LEFT_4ND_BUTTON_PRESSED*/;
    }

	mb_MouseButtonDown = (r.Event.MouseEvent.dwButtonState 
		& (FROM_LEFT_1ST_BUTTON_PRESSED|FROM_LEFT_2ND_BUTTON_PRESSED|RIGHTMOST_BUTTON_PRESSED)) != 0;

    // Key modifiers
    if (GetKeyState(VK_CAPITAL) & 1)
        r.Event.MouseEvent.dwControlKeyState |= CAPSLOCK_ON;
    if (GetKeyState(VK_NUMLOCK) & 1)
        r.Event.MouseEvent.dwControlKeyState |= NUMLOCK_ON;
    if (GetKeyState(VK_SCROLL) & 1)
        r.Event.MouseEvent.dwControlKeyState |= SCROLLLOCK_ON;
    if (isPressed(VK_LMENU))
        r.Event.MouseEvent.dwControlKeyState |= LEFT_ALT_PRESSED;
    if (isPressed(VK_RMENU))
        r.Event.MouseEvent.dwControlKeyState |= RIGHT_ALT_PRESSED;
    if (isPressed(VK_LCONTROL))
        r.Event.MouseEvent.dwControlKeyState |= LEFT_CTRL_PRESSED;
    if (isPressed(VK_RCONTROL))
        r.Event.MouseEvent.dwControlKeyState |= RIGHT_CTRL_PRESSED;
    if (isPressed(VK_SHIFT))
        r.Event.MouseEvent.dwControlKeyState |= SHIFT_PRESSED;

    if (messg == WM_LBUTTONDBLCLK || messg == WM_RBUTTONDBLCLK || messg == WM_MBUTTONDBLCLK)
        r.Event.MouseEvent.dwEventFlags = DOUBLE_CLICK;
    else if (messg == WM_MOUSEMOVE)
        r.Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
    else if (messg == WM_MOUSEWHEEL)
	{
		if (m_UseLogs>=2)
		{
			char szDbgMsg[128]; wsprintfA(szDbgMsg, "WM_MOUSEWHEEL(wParam=0x%08X, x=%i, y=%i)", wParam, x, y);
			LogString(szDbgMsg);
		}
		
		WARNING("Если включен режим прокрутки - посылать команду пайпа, а не мышиное событие");
		// Иначе на 2008 server вообще не крутится
		
        r.Event.MouseEvent.dwEventFlags = MOUSE_WHEELED;
        SHORT nScroll = (SHORT)(((DWORD)wParam & 0xFFFF0000)>>16);
        if (nScroll<0) { if (nScroll>-120) nScroll=-120; } else { if (nScroll<120) nScroll=120; }
        if (nScroll<-120 || nScroll>120)
        	nScroll = ((SHORT)(nScroll / 120)) * 120;
        r.Event.MouseEvent.dwButtonState |= ((DWORD)(WORD)nScroll) << 16;
        //r.Event.MouseEvent.dwButtonState |= /*(0xFFFF0000 & wParam)*/ (nScroll > 0) ? 0x00780000 : 0xFF880000;
    } else if (messg == WM_MOUSEHWHEEL)
	{
		if (m_UseLogs>=2)
		{
			char szDbgMsg[128]; wsprintfA(szDbgMsg, "WM_MOUSEHWHEEL(wParam=0x%08X, x=%i, y=%i)", wParam, x, y);
			LogString(szDbgMsg);
		}
        r.Event.MouseEvent.dwEventFlags = 8; //MOUSE_HWHEELED
        SHORT nScroll = (SHORT)(((DWORD)wParam & 0xFFFF0000)>>16);
        if (nScroll<0) { if (nScroll>-120) nScroll=-120; } else { if (nScroll<120) nScroll=120; }
        if (nScroll<-120 || nScroll>120)
        	nScroll = ((SHORT)(nScroll / 120)) * 120;
        r.Event.MouseEvent.dwButtonState |= ((DWORD)(WORD)nScroll) << 16;
    }

	if (messg == WM_LBUTTONDOWN || messg == WM_RBUTTONDOWN || messg == WM_MBUTTONDOWN)
	{
		mb_BtnClicked = TRUE; mrc_BtnClickPos = crMouse;
	}

	if (messg == WM_MOUSEMOVE /*&& mb_MouseButtonDown*/)
	{
		// Issue 172: проблема с правым кликом на PanelTabs
		//if (mcr_LastMouseEventPos.X == crMouse.X && mcr_LastMouseEventPos.Y == crMouse.Y)
		//	return; // не посылать в консоль MouseMove на том же месте
		//mcr_LastMouseEventPos.X = crMouse.X; mcr_LastMouseEventPos.Y = crMouse.Y;
		// Проверять будем по пикселам, иначе AltIns начинает выделять со следующей позиции
		int nDeltaX = (m_LastMouseGuiPos.x > x) ? (m_LastMouseGuiPos.x - x) : (x - m_LastMouseGuiPos.x);
		int nDeltaY = (m_LastMouseGuiPos.y > y) ? (m_LastMouseGuiPos.y - y) : (y - m_LastMouseGuiPos.y);
		if (m_LastMouse.dwButtonState     == r.Event.MouseEvent.dwButtonState 
			&& m_LastMouse.dwControlKeyState == r.Event.MouseEvent.dwControlKeyState
			&& (nDeltaX <= 1 && nDeltaY <= 1))
			return; // не посылать в консоль MouseMove на том же месте
		if (mb_BtnClicked)
		{
			// Если после LBtnDown в ЭТУ же позицию не был послан MOUSE_MOVE - дослать в mrc_BtnClickPos
			if (mb_MouseButtonDown && (mrc_BtnClickPos.X != crMouse.X || mrc_BtnClickPos.Y != crMouse.Y))
			{
				r.Event.MouseEvent.dwMousePosition = mrc_BtnClickPos;
				PostConsoleEvent ( &r );
			}
			mb_BtnClicked = FALSE;
		}
		m_LastMouseGuiPos.x = x; m_LastMouseGuiPos.y = y;
		mcr_LastMouseEventPos.X = crMouse.X; mcr_LastMouseEventPos.Y = crMouse.Y;
	}



    // При БЫСТРОМ драге правой кнопкой мышки выделение в панели получается прерывистым. Исправим это.
    if (gSet.isRSelFix)
	{
        BOOL lbRBtnDrag = (r.Event.MouseEvent.dwButtonState & RIGHTMOST_BUTTON_PRESSED) == RIGHTMOST_BUTTON_PRESSED;
        if (con.bRBtnDrag && !lbRBtnDrag)
		{
            con.bRBtnDrag = FALSE;
        }
		else if (con.bRBtnDrag)
		{
        	#ifdef _DEBUG
            SHORT nXDelta = crMouse.X - con.crRBtnDrag.X;
            #endif
            SHORT nYDelta = crMouse.Y - con.crRBtnDrag.Y;
            if (nYDelta < -1 || nYDelta > 1)
			{
                // Если после предыдущего драга прошло более 1 строки
                SHORT nYstep = (nYDelta < -1) ? -1 : 1;
                SHORT nYend = crMouse.Y; // - nYstep;
                crMouse.Y = con.crRBtnDrag.Y + nYstep;
                // досылаем пропущенные строки
                while (crMouse.Y != nYend)
				{
                    #ifdef _DEBUG
                    wchar_t szDbg[60]; wsprintf(szDbg, L"+++ Add right button drag: {%ix%i}\n", crMouse.X, crMouse.Y);
                    DEBUGSTRINPUT(szDbg);
                    #endif
                    r.Event.MouseEvent.dwMousePosition = crMouse;
                    PostConsoleEvent ( &r );
                    crMouse.Y += nYstep;
                }
            }
        }
        if (lbRBtnDrag)
		{
            con.bRBtnDrag = TRUE;
            con.crRBtnDrag = crMouse;
        }
    }
    
    r.Event.MouseEvent.dwMousePosition = crMouse;

    if (mn_FarPID && mn_FarPID != mn_LastSetForegroundPID)
	{
        AllowSetForegroundWindow(mn_FarPID);
        mn_LastSetForegroundPID = mn_FarPID;
    }

    // Посылаем событие в консоль через ConEmuC
    PostConsoleEvent ( &r );
}

// x,y - экранные координаты
bool CRealConsole::OnMouseSelection(UINT messg, WPARAM wParam, int x, int y)
{
	if (TextWidth()<=1 || TextHeight()<=1)
	{
		_ASSERTE(TextWidth()>1 && TextHeight()>1);
		return false;
	}

	// Получить известные координаты символов
	COORD cr = mp_VCon->ClientToConsole(x,y);
	if (cr.X<0) cr.X = 0; else if (cr.X >= (int)TextWidth()) cr.X = TextWidth()-1;
	if (cr.Y<0) cr.Y = 0; else if (cr.Y >= (int)TextHeight()) cr.Y = TextHeight()-1;

	if (messg == WM_LBUTTONDOWN)
	{
		BOOL lbStreamSelection = FALSE;
		BYTE vkMod = 0; // Если удерживается модификатор - его нужно "отпустить" в консоль

		if (con.m_sel.dwFlags & (CONSOLE_TEXT_SELECTION|CONSOLE_BLOCK_SELECTION))
		{
			// Выделение запущено из меню
			lbStreamSelection = (con.m_sel.dwFlags & (CONSOLE_TEXT_SELECTION)) == CONSOLE_TEXT_SELECTION;
		}
		else
		{
			if (gSet.isCTSSelectBlock && gSet.isModifierPressed(gSet.isCTSVkBlock))
			{
				lbStreamSelection = FALSE; vkMod = gSet.isCTSVkBlock; // OK
			}
			else if (gSet.isCTSSelectText && gSet.isModifierPressed(gSet.isCTSVkText))
			{
				lbStreamSelection = TRUE; vkMod = gSet.isCTSVkText; // OK
			}
			else
			{
				return false; // модификатор не разрешен
			}
		}

		StartSelection(lbStreamSelection, cr.X, cr.Y, TRUE);

		if (vkMod)
		{
			// Но чтобы ФАР не запустил макрос (если есть макро на RAlt например...)
			if (vkMod == VK_CONTROL || vkMod == VK_LCONTROL || vkMod == VK_RCONTROL)
				PostKeyPress(VK_SHIFT, LEFT_CTRL_PRESSED, 0);
			else if (vkMod == VK_MENU || vkMod == VK_LMENU || vkMod == VK_RMENU)
				PostKeyPress(VK_SHIFT, LEFT_ALT_PRESSED, 0);
			else
				PostKeyPress(VK_CONTROL, SHIFT_PRESSED, 0);

			// "Отпустить" в консоли модификатор
			PostKeyUp(vkMod, 0, 0);
		}
		
		//con.m_sel.dwFlags = CONSOLE_SELECTION_IN_PROGRESS|CONSOLE_MOUSE_SELECTION;
		//
		//con.m_sel.dwSelectionAnchor = cr;
		//con.m_sel.srSelection.Left = con.m_sel.srSelection.Right = cr.X;
		//con.m_sel.srSelection.Top = con.m_sel.srSelection.Bottom = cr.Y;
		//
		//UpdateSelection();

		return true;

	}
	else if ((con.m_sel.dwFlags & CONSOLE_MOUSE_SELECTION) && (messg == WM_MOUSEMOVE || messg == WM_LBUTTONUP))
	{
		_ASSERTE(con.m_sel.dwFlags!=0);
		if (cr.X<0 || cr.X>=(int)TextWidth() || cr.Y<0 && cr.Y>=(int)TextHeight())
		{
			_ASSERTE(cr.X>=0 && cr.X<(int)TextWidth());
			_ASSERTE(cr.Y>=0 && cr.Y<(int)TextHeight());
			return false; // Ошибка в координатах
		}
		
		////con.m_sel.dwSelectionAnchor = cr;
		//if (cr.X < con.m_sel.dwSelectionAnchor.X) {
		//	con.m_sel.srSelection.Left = cr.X;
		//	con.m_sel.srSelection.Right = con.m_sel.dwSelectionAnchor.X;
		//} else {
		//	con.m_sel.srSelection.Left = con.m_sel.dwSelectionAnchor.X;
		//	con.m_sel.srSelection.Right = cr.X;
		//}
		//
		//if (cr.Y < con.m_sel.dwSelectionAnchor.Y) {
		//	con.m_sel.srSelection.Top = cr.Y;
		//	con.m_sel.srSelection.Bottom = con.m_sel.dwSelectionAnchor.Y;
		//} else {
		//	con.m_sel.srSelection.Top = con.m_sel.dwSelectionAnchor.Y;
		//	con.m_sel.srSelection.Bottom = cr.Y;
		//}
		//
		//UpdateSelection();
		
		ExpandSelection(cr.X, cr.Y);
		
		if (messg == WM_LBUTTONUP)
		{
			con.m_sel.dwFlags &= ~CONSOLE_MOUSE_SELECTION;
			//ReleaseCapture();
		}
		
		return true;
		
	} else if (con.m_sel.dwFlags && (messg == WM_RBUTTONUP || messg == WM_MBUTTONUP))
	{
		BYTE bAction = (messg == WM_RBUTTONUP) ? gSet.isCTSRBtnAction : gSet.isCTSMBtnAction;

		if (bAction == 1)
			DoSelectionCopy();
		else if (bAction == 2)
			Paste();

		return true;

	}

	return false;
}

void CRealConsole::StartSelection(BOOL abTextMode, SHORT anX/*=-1*/, SHORT anY/*=-1*/, BOOL abByMouse/*=FALSE*/)
{
	if (anX == -1 && anY == -1)
	{
		anX = con.m_sbi.dwCursorPosition.X;
		anY = con.m_sbi.dwCursorPosition.Y;
	}
	
	
	COORD cr = {anX,anY};
	if (cr.X<0 || cr.X>=(int)TextWidth() || cr.Y<0 && cr.Y>=(int)TextHeight())
	{
		_ASSERTE(cr.X>=0 && cr.X<(int)TextWidth());
		_ASSERTE(cr.Y>=0 && cr.Y<(int)TextHeight());
		return; // Ошибка в координатах
	}


	con.m_sel.dwFlags = CONSOLE_SELECTION_IN_PROGRESS
		| (abByMouse ? CONSOLE_MOUSE_SELECTION : 0)
		| (abTextMode ? CONSOLE_TEXT_SELECTION : CONSOLE_BLOCK_SELECTION);
	
	con.m_sel.dwSelectionAnchor = cr;
	con.m_sel.srSelection.Left = con.m_sel.srSelection.Right = cr.X;
	con.m_sel.srSelection.Top = con.m_sel.srSelection.Bottom = cr.Y;
	
	UpdateSelection();
}

void CRealConsole::ExpandSelection(SHORT anX/*=-1*/, SHORT anY/*=-1*/)
{
	_ASSERTE(con.m_sel.dwFlags!=0);
	
	COORD cr = {anX,anY};
	
	if (cr.X<0 || cr.X>=(int)TextWidth() || cr.Y<0 && cr.Y>=(int)TextHeight())
	{
		_ASSERTE(cr.X>=0 && cr.X<(int)TextWidth());
		_ASSERTE(cr.Y>=0 && cr.Y<(int)TextHeight());
		return; // Ошибка в координатах
	}

	BOOL lbStreamSelection = (con.m_sel.dwFlags & (CONSOLE_TEXT_SELECTION)) == CONSOLE_TEXT_SELECTION;
	
	if (!lbStreamSelection)
	{
		if (cr.X < con.m_sel.dwSelectionAnchor.X)
		{
			con.m_sel.srSelection.Left = cr.X;
			con.m_sel.srSelection.Right = con.m_sel.dwSelectionAnchor.X;
		} else {
			con.m_sel.srSelection.Left = con.m_sel.dwSelectionAnchor.X;
			con.m_sel.srSelection.Right = cr.X;
		}
	} else {
		if ((cr.Y > con.m_sel.dwSelectionAnchor.Y) 
			|| ((cr.Y == con.m_sel.dwSelectionAnchor.Y) && (cr.X > con.m_sel.dwSelectionAnchor.X)))
		{
			con.m_sel.srSelection.Left = con.m_sel.dwSelectionAnchor.X;
			con.m_sel.srSelection.Right = cr.X;
		} else {
			con.m_sel.srSelection.Left = cr.X;
			con.m_sel.srSelection.Right = con.m_sel.dwSelectionAnchor.X;
		}
	}

	if (cr.Y < con.m_sel.dwSelectionAnchor.Y)
	{
		con.m_sel.srSelection.Top = cr.Y;
		con.m_sel.srSelection.Bottom = con.m_sel.dwSelectionAnchor.Y;
	} else {
		con.m_sel.srSelection.Top = con.m_sel.dwSelectionAnchor.Y;
		con.m_sel.srSelection.Bottom = cr.Y;
	}
	
	UpdateSelection();
}

bool CRealConsole::DoSelectionCopy()
{
	if (!con.m_sel.dwFlags)
	{
		MBoxAssert(con.m_sel.dwFlags != 0);
		return false;
	}
	if (!con.pConChar)
	{
		MBoxAssert(con.pConChar != NULL);
		return false;
	}

	DWORD dwErr = 0;
	BOOL  lbRc = FALSE;
	bool  Result = false;

	BOOL lbStreamMode = (con.m_sel.dwFlags & CONSOLE_TEXT_SELECTION) == CONSOLE_TEXT_SELECTION;
	int nSelWidth = con.m_sel.srSelection.Right - con.m_sel.srSelection.Left;
	int nSelHeight = con.m_sel.srSelection.Bottom - con.m_sel.srSelection.Top;
	if (nSelWidth<0 || nSelHeight<0)
	{
		MBoxAssert(nSelWidth>=0 && nSelHeight>=0);
		return false;
	}
	nSelWidth++; nSelHeight++;
	int nCharCount = 0;
	if (!lbStreamMode)
	{
		nCharCount = ((nSelWidth+2/* "\r\n" */) * nSelHeight) - 2; // после последней строки "\r\n" не ставится
	}
	else
	{
		if (nSelHeight == 1)
		{
			nCharCount = nSelWidth;
		}
		else if (nSelHeight == 2)
		{
			// На первой строке - до конца строки, вторая строка - до окончания блока, + "\r\n"
			nCharCount = (con.nTextWidth - con.m_sel.srSelection.Left) + (con.m_sel.srSelection.Right + 1) + 2;
		}
		else
		{
			_ASSERTE(nSelHeight>2);
			// На первой строке - до конца строки, последняя строка - до окончания блока, + "\r\n"
			nCharCount = (con.nTextWidth - con.m_sel.srSelection.Left) + (con.m_sel.srSelection.Right + 1) + 2
				+ ((nSelHeight - 2) * (con.nTextWidth + 2)); // + серединка * (длину консоли + "\r\n")
		}
	}

	HGLOBAL hUnicode = NULL;

	hUnicode = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, (nCharCount+1)*sizeof(wchar_t));
	if (hUnicode == NULL)
	{
		MBoxAssert(hUnicode != NULL);
		return false;
	}
	wchar_t *pch = (wchar_t*)GlobalLock(hUnicode);
	if (!pch)
	{
		MBoxAssert(pch != NULL);
		GlobalFree(hUnicode);
	}

	// Заполнить данными
	if ((con.m_sel.srSelection.Left + nSelWidth) > con.nTextWidth)
	{
		_ASSERTE((con.m_sel.srSelection.Left + nSelWidth) <= con.nTextWidth);
		nSelWidth = con.nTextWidth - con.m_sel.srSelection.Left;
	}
	if ((con.m_sel.srSelection.Top + nSelHeight) > con.nTextHeight)
	{
		_ASSERTE((con.m_sel.srSelection.Top + nSelHeight) <= con.nTextHeight);
		nSelHeight = con.nTextHeight - con.m_sel.srSelection.Top;
	}
	nSelHeight--;
	if (!lbStreamMode)
	{
		for (int Y = 0; Y <= nSelHeight; Y++)
		{
			wchar_t* pszCon = con.pConChar + con.nTextWidth*(Y+con.m_sel.srSelection.Top) + con.m_sel.srSelection.Left;
			int nMaxX = nSelWidth - 1;
			//while (nMaxX > 0 && (pszCon[nMaxX] == ucSpace || pszCon[nMaxX] == ucNoBreakSpace))
			//	nMaxX--; // отрезать хвостовые пробелы - зачем их в буфер копировать?
			for (int X = 0; X <= nMaxX; X++)
				*(pch++) = *(pszCon++);
			// Добавить перевод строки
			if (Y < nSelHeight)
			{
				*(pch++) = L'\r'; *(pch++) = L'\n';
			}
		}
	} else {
		int nX1, nX2;
		//for (nY = rc.Top; nY <= rc.Bottom; nY++) {
		//	pnDst = pAttr + nWidth*nY;

		//	nX1 = (nY == rc.Top) ? rc.Left : 0;
		//	nX2 = (nY == rc.Bottom) ? rc.Right : (nWidth-1);

		//	for (nX = nX1; nX <= nX2; nX++) {
		//		pnDst[nX] = lcaSel;
		//	}
		//}
		for (int Y = 0; Y <= nSelHeight; Y++) {
			nX1 = (Y == 0) ? con.m_sel.srSelection.Left : 0;
			nX2 = (Y == nSelHeight) ? con.m_sel.srSelection.Right : (con.nTextWidth-1);

			wchar_t* pszCon = con.pConChar + con.nTextWidth*(Y+con.m_sel.srSelection.Top) + nX1;
			int nMaxX = nSelWidth - 1;
			for (int X = nX1; X <= nX2; X++)
				*(pch++) = *(pszCon++);
			// Добавить перевод строки
			if (Y < nSelHeight) {
				*(pch++) = L'\r'; *(pch++) = L'\n';
			}
		}
	}

	// Ready
	GlobalUnlock(hUnicode);


	// Открыть буфер обмена
	while (!(lbRc = OpenClipboard(ghWnd))) {
		dwErr = GetLastError();
		if (IDRETRY != DisplayLastError(L"OpenClipboard failed!", dwErr, MB_RETRYCANCEL|MB_ICONSTOP))
			return false;
	}

	lbRc = EmptyClipboard();

	// Установить данные
	Result = SetClipboardData(CF_UNICODETEXT, hUnicode);
	while (!Result) {
		dwErr = GetLastError();
		if (IDRETRY != DisplayLastError(L"SetClipboardData(CF_UNICODETEXT, ...) failed!", dwErr, MB_RETRYCANCEL|MB_ICONSTOP)) {
			GlobalFree(hUnicode); hUnicode = NULL;
			break;
		}

		Result = SetClipboardData(CF_UNICODETEXT, hUnicode);
	}

	lbRc = CloseClipboard();


	// Fin, Сбрасываем	
	if (Result) {
		con.m_sel.dwFlags = 0;
		UpdateSelection(); // обновить на экране
	}
	
	return Result;
}

// обновить на экране
void CRealConsole::UpdateSelection()
{
    TODO("Это корректно? Нужно обновить VCon");
	con.bConsoleDataChanged = TRUE; // А эта - при вызовах из CVirtualConsole
	mp_VCon->Update(true);
	gConEmu.m_Child->Redraw();
}

BOOL CRealConsole::OpenConsoleEventPipe()
{
	if (mh_ConEmuCInput && mh_ConEmuCInput!=INVALID_HANDLE_VALUE) {
		CloseHandle(mh_ConEmuCInput); mh_ConEmuCInput = NULL;
	}

	TODO("Если пайп с таким именем не появится в течении 10 секунд (минуты?) - закрыть VirtualConsole показав ошибку");

	// Try to open a named pipe; wait for it, if necessary. 
	int nSteps = 10;
	BOOL fSuccess;
	DWORD dwErr;

	while ((nSteps--) > 0) 
	{ 
		mh_ConEmuCInput = CreateFile( 
				ms_ConEmuCInput_Pipe,// pipe name 
				GENERIC_WRITE, 
				0,              // no sharing 
				NULL,           // default security attributes
				OPEN_EXISTING,  // opens existing pipe 
				0,              // default attributes 
				NULL);          // no template file 

		// Break if the pipe handle is valid. 
		if (mh_ConEmuCInput != INVALID_HANDLE_VALUE) {
			// The pipe connected; change to message-read mode. 
			DWORD dwMode = PIPE_READMODE_MESSAGE; 
			fSuccess = SetNamedPipeHandleState( 
				mh_ConEmuCInput,    // pipe handle 
				&dwMode,  // new pipe mode 
				NULL,     // don't set maximum bytes 
				NULL);    // don't set maximum time 
			if (!fSuccess) 
			{
				DEBUGSTRINPUT(L" - FAILED!\n");
				DWORD dwErr = GetLastError();
				SafeCloseHandle(mh_ConEmuCInput);
				//if (!IsDebuggerPresent())
				if (!isConsoleClosing())
					DisplayLastError(L"SetNamedPipeHandleState failed", dwErr);
				return FALSE;
			}
			return TRUE; 
		}

		// Exit if an error other than ERROR_PIPE_BUSY occurs. 
		dwErr = GetLastError();
		if (dwErr != ERROR_PIPE_BUSY) 
		{
			TODO("Подождать, пока появится пайп с таким именем, но только пока жив mh_ConEmuC");
			dwErr = WaitForSingleObject(mh_ConEmuC, 100);
			if (dwErr == WAIT_OBJECT_0)
			{
				DEBUGSTRINPUT(L"ConEmuC was closed. OpenPipe FAILED!\n");
				return FALSE;
			}
			if (!isConsoleClosing())
				break;
			continue;
			//DisplayLastError(L"Could not open pipe", dwErr);
			//return 0;
		}

		// All pipe instances are busy, so wait for 0.1 second.
		if (!WaitNamedPipe(ms_ConEmuCInput_Pipe, 100) ) 
		{
			dwErr = GetLastError();

			DWORD dwWait = WaitForSingleObject(mh_ConEmuC, 100);
			if (dwWait == WAIT_OBJECT_0) {
				DEBUGSTRINPUT(L"ConEmuC was closed. OpenPipe FAILED!\n");
				return FALSE;
			}

			if (!isConsoleClosing())
				DisplayLastError(L"WaitNamedPipe failed", dwErr); 
			return FALSE;
		}
	}

	if (mh_ConEmuCInput == NULL || mh_ConEmuCInput == INVALID_HANDLE_VALUE)
	{
		// Не дождались появления пайпа. Возможно, ConEmuC еще не запустился
		//DEBUGSTRINPUT(L" - mh_ConEmuCInput not found!\n");
		#ifdef _DEBUG
		DWORD dwTick1 = GetTickCount();
		struct ServerClosing sc1 = m_ServerClosing;
		#endif
		if (!isConsoleClosing())
		{
			#ifdef _DEBUG
			DWORD dwTick2 = GetTickCount();
			struct ServerClosing sc2 = m_ServerClosing;
			if (dwErr == 0x102)
			{
				TODO("Иногда трапится. Проверить m_ServerClosing.nServerPID. Может его выставлять при щелчке по крестику?");
				MyAssertTrap();
			}
			DWORD dwTick3 = GetTickCount();
			struct ServerClosing sc3 = m_ServerClosing;
			#endif
			DisplayLastError(L"mh_ConEmuCInput not found", dwErr);
		}
		return FALSE;
	}


	return FALSE;
}

void CRealConsole::PostConsoleEventPipe(MSG64 *pMsg)
{
    DWORD dwErr = 0, dwMode = 0;
    BOOL fSuccess = FALSE;

#ifdef _DEBUG
	if (gbInSendConEvent) {
		_ASSERTE(!gbInSendConEvent);
	}
#endif

    // Пайп есть. Проверим, что ConEmuC жив
    DWORD dwExitCode = 0;
    fSuccess = GetExitCodeProcess(mh_ConEmuC, &dwExitCode);
    if (dwExitCode!=STILL_ACTIVE) {
        //DisplayLastError(L"ConEmuC was terminated");
        return;
    }

    TODO("Если пайп с таким именем не появится в течении 10 секунд (минуты?) - закрыть VirtualConsole показав ошибку");
    if (mh_ConEmuCInput==NULL || mh_ConEmuCInput==INVALID_HANDLE_VALUE)
	{
        // Try to open a named pipe; wait for it, if necessary. 
        if (!OpenConsoleEventPipe())
        	return;
        //int nSteps = 10;
        //while ((nSteps--) > 0) 
        //{ 
        //  mh_ConEmuCInput = CreateFile( 
        //     ms_ConEmuCInput_Pipe,// pipe name 
        //     GENERIC_WRITE, 
        //     0,              // no sharing 
        //     NULL,           // default security attributes
        //     OPEN_EXISTING,  // opens existing pipe 
        //     0,              // default attributes 
        //     NULL);          // no template file 
        //
        //  // Break if the pipe handle is valid. 
        //  if (mh_ConEmuCInput != INVALID_HANDLE_VALUE) 
        //     break; 
        //
        //  // Exit if an error other than ERROR_PIPE_BUSY occurs. 
        //  dwErr = GetLastError();
        //  if (dwErr != ERROR_PIPE_BUSY) 
        //  {
        //    TODO("Подождать, пока появится пайп с таким именем, но только пока жив mh_ConEmuC");
        //    dwErr = WaitForSingleObject(mh_ConEmuC, 100);
        //    if (dwErr == WAIT_OBJECT_0) {
        //        return;
        //    }
        //    continue;
        //    //DisplayLastError(L"Could not open pipe", dwErr);
        //    //return 0;
        //  }
        //
        //  // All pipe instances are busy, so wait for 0.1 second.
        //  if (!WaitNamedPipe(ms_ConEmuCInput_Pipe, 100) ) 
        //  {
        //    dwErr = WaitForSingleObject(mh_ConEmuC, 100);
        //    if (dwErr == WAIT_OBJECT_0) {
        //        DEBUGSTRINPUT(L" - FAILED!\n");
        //        return;
        //    }
        //    //DisplayLastError(L"WaitNamedPipe failed"); 
        //    //return 0;
        //  }
        //}
        //if (mh_ConEmuCInput == NULL || mh_ConEmuCInput == INVALID_HANDLE_VALUE) {
        //    // Не дождались появления пайпа. Возможно, ConEmuC еще не запустился
        //    DEBUGSTRINPUT(L" - mh_ConEmuCInput not found!\n");
        //    return;
        //}
        //
        //// The pipe connected; change to message-read mode. 
        //dwMode = PIPE_READMODE_MESSAGE; 
        //fSuccess = SetNamedPipeHandleState( 
        //  mh_ConEmuCInput,    // pipe handle 
        //  &dwMode,  // new pipe mode 
        //  NULL,     // don't set maximum bytes 
        //  NULL);    // don't set maximum time 
        //if (!fSuccess) 
        //{
        //  DEBUGSTRINPUT(L" - FAILED!\n");
        //  DWORD dwErr = GetLastError();
        //  SafeCloseHandle(mh_ConEmuCInput);
        //  if (!IsDebuggerPresent())
        //    DisplayLastError(L"SetNamedPipeHandleState failed", dwErr);
        //  return;
        //}
    }
    
    //// Пайп есть. Проверим, что ConEmuC жив
    //dwExitCode = 0;
    //fSuccess = GetExitCodeProcess(mh_ConEmuC, &dwExitCode);
    //if (dwExitCode!=STILL_ACTIVE) {
    //    //DisplayLastError(L"ConEmuC was terminated");
    //    return;
    //}

#ifdef _DEBUG
	switch (pMsg->message) {
		case WM_KEYDOWN: case WM_SYSKEYDOWN:
			DEBUGSTRINPUTPIPE(L"ConEmu: Sending key down\n"); break;
		case WM_KEYUP: case WM_SYSKEYUP:
			DEBUGSTRINPUTPIPE(L"ConEmu: Sending key up\n"); break;
		default:
			DEBUGSTRINPUTPIPE(L"ConEmu: Sending input\n");
	}
#endif

	gbInSendConEvent = TRUE;

    DWORD dwSize = sizeof(MSG64), dwWritten;
    fSuccess = WriteFile ( mh_ConEmuCInput, pMsg, dwSize, &dwWritten, NULL);
    if (!fSuccess)
	{
    	dwErr = GetLastError();
		if (!isConsoleClosing())
		{
    		if (dwErr == 0x000000E8/*The pipe is being closed.*/)
			{
				fSuccess = GetExitCodeProcess(mh_ConEmuC, &dwExitCode);
				if (fSuccess && dwExitCode!=STILL_ACTIVE)
					goto wrap;
    			if (OpenConsoleEventPipe()) {
    				fSuccess = WriteFile ( mh_ConEmuCInput, pMsg, dwSize, &dwWritten, NULL);
    				if (fSuccess)
    					goto wrap; // таки ОК
    			}
    		}
			#ifdef _DEBUG
			//DisplayLastError(L"Can't send console event (pipe)", dwErr);
			wchar_t szDbg[128];
			wsprintf(szDbg, L"Can't send console event (pipe)", dwErr);
			gConEmu.DebugStep(szDbg);
			#endif
		}
		goto wrap;
    }
wrap:
	gbInSendConEvent = FALSE;
}

LRESULT CRealConsole::PostConsoleMessage(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRc = 0;
	bool bNeedCmd = isAdministrator() || (m_Args.pszUserName != NULL);

	if (nMsg == WM_INPUTLANGCHANGE || nMsg == WM_INPUTLANGCHANGEREQUEST)
		bNeedCmd = true;

	#ifdef _DEBUG
	if (nMsg == WM_INPUTLANGCHANGE || nMsg == WM_INPUTLANGCHANGEREQUEST)
	{
		wchar_t szDbg[255];
		const wchar_t* pszMsgID = (nMsg == WM_INPUTLANGCHANGE) ? L"WM_INPUTLANGCHANGE" : L"WM_INPUTLANGCHANGEREQUEST";
		const wchar_t* pszVia = bNeedCmd ? L"CmdExecute" : L"PostThreadMessage";
		wsprintf(szDbg, L"RealConsole: %s, CP:%i, HKL:0x%08I64X via %s\n",
			pszMsgID, (DWORD)wParam, (unsigned __int64)(DWORD_PTR)lParam, pszVia);
		DEBUGSTRLANG(szDbg);
	}
	#endif

	if (!bNeedCmd)
	{
		POSTMESSAGE(hWnd/*hConWnd*/, nMsg, wParam, lParam, FALSE);
	}
	else
	{
		CESERVER_REQ in;
		ExecutePrepareCmd(&in, CECMD_POSTCONMSG, sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_POSTMSG));
		// Собственно, аргументы
		in.Msg.bPost = TRUE;
		in.Msg.hWnd = hWnd;
		in.Msg.nMsg = nMsg;
		in.Msg.wParam = wParam;
		in.Msg.lParam = lParam;

		CESERVER_REQ *pOut = ExecuteSrvCmd(GetServerPID(), &in, ghWnd);
		if (pOut) ExecuteFreeResult(pOut);
	}
	return lRc;
}

void CRealConsole::StopSignal()
{
	DEBUGSTRCON(L"CRealConsole::StopSignal()\n");
    if (!this)
        return;

    if (mn_ProcessCount) {
        MSectionLock SPRC; SPRC.Lock(&csPRC, TRUE);
        m_Processes.clear();
        SPRC.Unlock();
        mn_ProcessCount = 0;
    }

    SetEvent(mh_TermEvent);

	if (!mn_InRecreate) {
		// Чтобы при закрытии не было попытка активировать 
		// другую вкладку ЭТОЙ консоли
		mn_tabsCount = 0; 
		// Очистка массива консолей и обновление вкладок
        gConEmu.OnVConTerminated(mp_VCon);
	}
}

void CRealConsole::StopThread(BOOL abRecreating)
{
#ifdef _DEBUG
    /*
        HeapValidate(mh_Heap, 0, NULL);
    */
#endif

    _ASSERT(abRecreating==mb_ProcessRestarted);

    DEBUGSTRPROC(L"Entering StopThread\n");

    // Сначала выставить флаги закрытия
    if (mh_MonitorThread) {
        // выставление флагов и завершение нити
        StopSignal(); //SetEvent(mh_TermEvent);
    }
    
    //if (mh_InputThread) {
    //	PostThreadMessage(mn_InputThreadID, WM_QUIT, 0, 0);
    //}
    
    
    // А теперь можно ждать завершения
    if (mh_MonitorThread) {
        if (WaitForSingleObject(mh_MonitorThread, 300) != WAIT_OBJECT_0) {
            DEBUGSTRPROC(L"### Main Thread wating timeout, terminating...\n");
            TerminateThread(mh_MonitorThread, 1);
        } else {
            DEBUGSTRPROC(L"Main Thread closed normally\n");
        }
        SafeCloseHandle(mh_MonitorThread);
    }


	// Завершение серверных нитей этой консоли
	DEBUGSTRPROC(L"About to terminate main server thread (MonitorThread)\n");
	if (ms_VConServer_Pipe[0]) // значит хотя бы одна нить была запущена
	{   
		StopSignal(); // уже должен быть выставлен, но на всякий случай
		//
		HANDLE hPipe = INVALID_HANDLE_VALUE;
		DWORD dwWait = 0;
		// Передернуть пайпы, чтобы нити сервера завершились
		for (int i=0; i<MAX_SERVER_THREADS; i++) {
			DEBUGSTRPROC(L"Touching our server pipe\n");
			HANDLE hServer = mh_ActiveRConServerThread;
			hPipe = CreateFile(ms_VConServer_Pipe,GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
			if (hPipe == INVALID_HANDLE_VALUE) {
				DEBUGSTRPROC(L"All pipe instances closed?\n");
				break;
			}
			DEBUGSTRPROC(L"Waiting server pipe thread\n");
			dwWait = WaitForSingleObject(hServer, 200); // пытаемся дождаться, пока нить завершится
			// Просто закроем пайп - его нужно было передернуть
			CloseHandle(hPipe);
			hPipe = INVALID_HANDLE_VALUE;
		}
		// Немного подождем, пока ВСЕ нити завершатся
		DEBUGSTRPROC(L"Checking server pipe threads are closed\n");
		WaitForMultipleObjects(MAX_SERVER_THREADS, mh_RConServerThreads, TRUE, 500);
		for (int i=0; i<MAX_SERVER_THREADS; i++) {
			if (WaitForSingleObject(mh_RConServerThreads[i],0) != WAIT_OBJECT_0) {
				DEBUGSTRPROC(L"### Terminating mh_RConServerThreads\n");
				TerminateThread(mh_RConServerThreads[i],0);
			}
			CloseHandle(mh_RConServerThreads[i]);
			mh_RConServerThreads[i] = NULL;
		}
		ms_VConServer_Pipe[0] = 0;
	}

    
    //if (mh_InputThread) {
    //    if (WaitForSingleObject(mh_InputThread, 300) != WAIT_OBJECT_0) {
    //        DEBUGSTRPROC(L"### Input Thread wating timeout, terminating...\n");
    //        TerminateThread(mh_InputThread, 1);
    //    } else {
    //        DEBUGSTRPROC(L"Input Thread closed normally\n");
    //    }
    //    SafeCloseHandle(mh_InputThread);
    //}
    
    if (!abRecreating) {
        SafeCloseHandle(mh_TermEvent);
        SafeCloseHandle(mh_MonitorThreadEvent);
        //SafeCloseHandle(mh_PacketArrived);
    }
    
    
    if (abRecreating) {
        hConWnd = NULL;
        mn_ConEmuC_PID = 0;
		//mn_ConEmuC_Input_TID = 0;
        SafeCloseHandle(mh_ConEmuC);
        SafeCloseHandle(mh_ConEmuCInput);
		m_ConDataChanged.Close();
		m_GetDataPipe.Close();
        // Имя пайпа для управления ConEmuC
        ms_ConEmuC_Pipe[0] = 0;
        ms_ConEmuCInput_Pipe[0] = 0;
		// Закрыть все мэппинги
		CloseMapHeader();
		CloseColorMapping();
    }
#ifdef _DEBUG
    /*
        HeapValidate(mh_Heap, 0, NULL);
    */
#endif

    DEBUGSTRPROC(L"Leaving StopThread\n");
}


void CRealConsole::Box(LPCTSTR szText)
{
#ifdef _DEBUG
    _ASSERT(FALSE);
#endif
    MessageBox(NULL, szText, _T("ConEmu"), MB_ICONSTOP);
}


BOOL CRealConsole::isBufferHeight()
{
    if (!this)
        return FALSE;

    return con.bBufferHeight;
}

BOOL CRealConsole::isConSelectMode()
{
    if (!this) return false;
    
    if (con.m_sel.dwFlags != 0)
    	return true;
    	
    // В Фаре может быть активен граббер (AltIns)
	if (con.m_ci.dwSize >= 40) // Попробуем его определять по высоте курсора.
		return true;
    
    return false;
}

BOOL CRealConsole::isDetached()
{
    if (this == NULL) return FALSE;
    if (!m_Args.bDetached) return FALSE;
    // Флажок ВООБЩЕ не сбрасываем - ориентируемся на хэндлы
    //_ASSERTE(!mb_Detached || (mb_Detached && (hConWnd==NULL)));
    return (mh_ConEmuC == NULL);
}

BOOL CRealConsole::isFar(BOOL abPluginRequired/*=FALSE*/)
{
    if (!this) return false;
    return GetFarPID(abPluginRequired)!=0;
}

BOOL CRealConsole::isWindowVisible()
{
	if (!this) return FALSE;
	if (!hConWnd) return FALSE;
	return IsWindowVisible(hConWnd);
}

LPCTSTR CRealConsole::GetTitle()
{
    // На старте mn_ProcessCount==0, а кнопку в тулбаре показывать уже нужно
    if (!this /*|| !mn_ProcessCount*/)
        return NULL;
    return TitleFull;
}

LRESULT CRealConsole::OnScroll(int nDirection)
{
	if (!this) return 0;
    // SB_LINEDOWN / SB_LINEUP / SB_PAGEDOWN / SB_PAGEUP
    WARNING("Переделать в команду пайпа");
	PostConsoleMessage(hConWnd, WM_VSCROLL, nDirection, NULL);
    return 0;
}

LRESULT CRealConsole::OnSetScrollPos(WPARAM wParam)
{
	if (!this) return 0;
	// SB_LINEDOWN / SB_LINEUP / SB_PAGEDOWN / SB_PAGEUP
	TODO("Переделать в команду пайпа"); // не критично. если консоль под админом - сообщение посылается через пайп в сервер, а он уже пересылает в консоль
	PostConsoleMessage(hConWnd, WM_VSCROLL, wParam, NULL);
	return 0;
}

void CRealConsole::OnKeyboard(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, const wchar_t *pszChars)
{
	if (!this) return;
    //LRESULT result = 0;
	_ASSERTE(pszChars!=NULL);

#ifdef _DEBUG
    if (wParam != VK_LCONTROL && wParam != VK_RCONTROL && wParam != VK_CONTROL &&
        wParam != VK_LSHIFT && wParam != VK_RSHIFT && wParam != VK_SHIFT &&
        wParam != VK_LMENU && wParam != VK_RMENU && wParam != VK_MENU &&
        wParam != VK_LWIN && wParam != VK_RWIN)
    {
        wParam = wParam;
    }
    if (wParam == VK_CONTROL || wParam == VK_LCONTROL || wParam == VK_RCONTROL || wParam == 'C')
	{
        if (messg == WM_KEYDOWN || messg == WM_KEYUP /*|| messg == WM_CHAR*/)
		{
			wchar_t szDbg[128];
            if (messg == WM_KEYDOWN)
                wsprintf(szDbg, L"WM_KEYDOWN(%i,0x%08X)\n", wParam, lParam);
            else //if (messg == WM_KEYUP)
                wsprintf(szDbg, L"WM_KEYUP(%i,0x%08X)\n", wParam, lParam);
            //else
            //    wsprintf(szDbg, L"WM_CHAR(%i,0x%08X)\n", wParam, lParam);
            DEBUGSTRINPUT(szDbg);
        }
    }
#endif

	TODO("Обработка Left/Right/Up/Down при выделении");
    if (con.m_sel.dwFlags && messg == WM_KEYDOWN 
    	&& ((wParam == VK_ESCAPE) || (wParam == VK_RETURN)
    		 || (wParam == VK_LEFT) || (wParam == VK_RIGHT) || (wParam == VK_UP) || (wParam == VK_DOWN))
    	)
    {
    	if ((wParam == VK_ESCAPE) || (wParam == VK_RETURN))
    	{
	    	if (wParam == VK_RETURN)
			{
	    		DoSelectionCopy();
	    	}
	    	mn_SelectModeSkipVk = wParam;
	    	con.m_sel.dwFlags = 0;
	        //mb_ConsoleSelectMode = false;
			UpdateSelection(); // обновить на экране
		}
		else
		{
			COORD cr; GetConsoleCursorPos(&cr);
			// Поправить
			cr.Y -= con.nTopVisibleLine;
			if (wParam == VK_LEFT)  { if (cr.X>0) cr.X--; } else
			if (wParam == VK_RIGHT) { if (cr.X<(con.nTextWidth-1)) cr.X++; } else
			if (wParam == VK_UP)    { if (cr.Y>0) cr.Y--; } else
			if (wParam == VK_DOWN)  { if (cr.Y<(con.nTextHeight-1)) cr.Y++; }
			
			// Теперь - двигаем
			BOOL bShift = isPressed(VK_SHIFT);
			if (!bShift)
			{
				BOOL lbStreamSelection = (con.m_sel.dwFlags & (CONSOLE_TEXT_SELECTION)) == CONSOLE_TEXT_SELECTION;
				StartSelection(lbStreamSelection, cr.X,cr.Y);
			}
			else
			{
				ExpandSelection(cr.X,cr.Y);
			}
		}
		return;
    }
    if (messg == WM_KEYUP && wParam == mn_SelectModeSkipVk)
	{
    	mn_SelectModeSkipVk = 0; // игнорируем отпускание, поскольку нажатие было на копирование/отмену
    	return;
    }
    if (messg == WM_KEYDOWN && mn_SelectModeSkipVk)
	{
    	mn_SelectModeSkipVk = 0; // при нажатии любой другой клавиши - сбросить флажок (во избежание)
    }
    if (con.m_sel.dwFlags)
	{
    	return; // В режиме выделения - в консоль ВООБЩЕ клавиатуру не посылать!
    }
    


    // Основная обработка 
    {
    
		if (wParam == VK_MENU && (messg == WM_KEYUP || messg == WM_SYSKEYUP) && gSet.isFixAltOnAltTab)
		{
			// При быстром нажатии Alt-Tab (переключение в другое окно)
			// в консоль проваливается {press Alt/release Alt}
			// В результате, может выполниться макрос, повешенный на Alt.
			if (GetForegroundWindow()!=ghWnd && GetFarPID())
			{
				if (/*isPressed(VK_MENU) &&*/ !isPressed(VK_CONTROL) && !isPressed(VK_SHIFT))
				{
					PostKeyPress(VK_CONTROL, LEFT_ALT_PRESSED, 0);
					//TODO("Вынести в отдельную функцию типа SendKeyPress(VK_TAB,0x22,'\x09')");
					//INPUT_RECORD r = {KEY_EVENT};
					//
					////WARNING("По хорошему, нужно не TAB посылать, а то если открыт QuickSearch - он закроется а фар перескочит на другую панель");
					//
					//r.Event.KeyEvent.bKeyDown = TRUE;
					//r.Event.KeyEvent.wVirtualKeyCode = VK_TAB;
					//r.Event.KeyEvent.wVirtualScanCode = MapVirtualKey(VK_TAB, 0/*MAPVK_VK_TO_VSC*/);
					//r.Event.KeyEvent.dwControlKeyState = 0x22;
					//r.Event.KeyEvent.uChar.UnicodeChar = 9;
					//PostConsoleEvent(&r);
					//
					////On Keyboard(hConWnd, WM_KEYUP, VK_TAB, 0);
					//r.Event.KeyEvent.bKeyDown = FALSE;
					//r.Event.KeyEvent.dwControlKeyState = 0x22; // было 0x20
					//PostConsoleEvent(&r);
				}
			}
		}
    
        //if (messg == WM_SYSKEYDOWN) 
        //    if (wParam == VK_INSERT && lParam & (1<<29)/*Бред. это 29-й бит, а не число 29*/)
        //        mb_ConsoleSelectMode = true;

        static bool isSkipNextAltUp = false;
        if (messg == WM_SYSKEYDOWN && wParam == VK_RETURN && lParam & (1<<29)
			&& !isPressed(VK_SHIFT))
        {
            if (gSet.isSendAltEnter)
            {
				INPUT_RECORD r = {KEY_EVENT};

				//On Keyboard(hConWnd, WM_KEYDOWN, VK_MENU, 0); -- Alt слать не нужно - он уже послан
				
				WARNING("А надо ли так заморачиваться?");

				//On Keyboard(hConWnd, WM_KEYDOWN, VK_RETURN, 0);
				r.Event.KeyEvent.bKeyDown = TRUE;
                r.Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
				r.Event.KeyEvent.wVirtualScanCode = /*28 на моей клавиатуре*/MapVirtualKey(VK_RETURN, 0/*MAPVK_VK_TO_VSC*/);
				r.Event.KeyEvent.dwControlKeyState = 0x22;
				r.Event.KeyEvent.uChar.UnicodeChar = pszChars[0];
				PostConsoleEvent(&r);
                
                //On Keyboard(hConWnd, WM_KEYUP, VK_RETURN, 0);
				r.Event.KeyEvent.bKeyDown = FALSE;
				r.Event.KeyEvent.dwControlKeyState = 0x20;
				PostConsoleEvent(&r);

                //On Keyboard(hConWnd, WM_KEYUP, VK_MENU, 0); -- Alt слать не нужно - он будет послан сам позже
            }
            else
            {
                //if (isPressed(VK_SHIFT))
                //    return;

				gConEmu.OnAltEnter();

                isSkipNextAltUp = true;
            }
        }
		//AltSpace - показать системное меню
        else if (!gSet.isSendAltSpace && (messg == WM_SYSKEYDOWN || messg == WM_SYSKEYUP)
        	&& wParam == VK_SPACE && lParam & (1<<29) && !isPressed(VK_SHIFT))
        {   // Нада, или системное меню будет недоступно
			if (messg == WM_SYSKEYUP) // Только по UP, чтобы не "булькало"
			{
				gConEmu.ShowSysmenu();
			}
        }
        else if (messg == WM_KEYUP && wParam == VK_MENU && isSkipNextAltUp) isSkipNextAltUp = false;
        else if (messg == WM_SYSKEYDOWN && wParam == VK_F9 && lParam & (1<<29)
			&& !isPressed(VK_SHIFT))
        {
			// AltF9
			// Чтобы у консоли не сносило крышу (FAR может выполнить макрос на Alt)
			if (gSet.isFixAltOnAltTab)
				PostKeyPress(VK_CONTROL, LEFT_ALT_PRESSED, 0);
			
			//INPUT_RECORD r = {KEY_EVENT};
			//r.Event.KeyEvent.bKeyDown = TRUE;
            //r.Event.KeyEvent.wVirtualKeyCode = VK_CONTROL;
			//r.Event.KeyEvent.wVirtualScanCode = MapVirtualKey(VK_CONTROL, 0/*MAPVK_VK_TO_VSC*/);
			//r.Event.KeyEvent.dwControlKeyState = 0x2A;
			//r.Event.KeyEvent.wRepeatCount = 1;
			//PostConsoleEvent(&r);
			//r.Event.KeyEvent.bKeyDown = FALSE;
			//r.Event.KeyEvent.dwControlKeyState = 0x22;
			//PostConsoleEvent(&r);
			
            //gConEmu.SetWindowMode((gConEmu.isZoomed()||(gSet.isFullScreen&&gConEmu.isWndNotFSMaximized)) ? rNormal : rMaximized);
            gConEmu.OnAltF9(TRUE);
            
        }
		else
		{
            INPUT_RECORD r = {KEY_EVENT};

            WORD nCaps = 1 & (WORD)GetKeyState(VK_CAPITAL);
            WORD nNum = 1 & (WORD)GetKeyState(VK_NUMLOCK);
            WORD nScroll = 1 & (WORD)GetKeyState(VK_SCROLL);
            WORD nLAlt = 0x8000 & (WORD)GetKeyState(VK_LMENU);
            WORD nRAlt = 0x8000 & (WORD)GetKeyState(VK_RMENU);
            WORD nLCtrl = 0x8000 & (WORD)GetKeyState(VK_LCONTROL);
            WORD nRCtrl = 0x8000 & (WORD)GetKeyState(VK_RCONTROL);
            WORD nShift = 0x8000 & (WORD)GetKeyState(VK_SHIFT);

            //if (messg == WM_CHAR || messg == WM_SYSCHAR) {
            //    if (((WCHAR)wParam) <= 32 || mn_LastVKeyPressed == 0)
            //        return; // это уже обработано
            //    r.Event.KeyEvent.bKeyDown = TRUE;
            //    r.Event.KeyEvent.uChar.UnicodeChar = (WCHAR)wParam;
            //    r.Event.KeyEvent.wRepeatCount = 1; TODO("0-15 ? Specifies the repeat count for the current message. The value is the number of times the keystroke is autorepeated as a result of the user holding down the key. If the keystroke is held long enough, multiple messages are sent. However, the repeat count is not cumulative.");
            //    r.Event.KeyEvent.wVirtualKeyCode = mn_LastVKeyPressed;
            //} else {
                mn_LastVKeyPressed = wParam & 0xFFFF;
                ////PostConsoleMessage(hConWnd, messg, wParam, lParam, FALSE);
                //if ((wParam >= VK_F1 && wParam <= /*VK_F24*/ VK_SCROLL) || wParam <= 32 ||
                //    (wParam >= VK_LSHIFT/*0xA0*/ && wParam <= /*VK_RMENU=0xA5*/ 0xB7 /*=VK_LAUNCH_APP2*/) ||
                //    (wParam >= VK_LWIN/*0x5B*/ && wParam <= VK_APPS/*0x5D*/) ||
                //    /*(wParam >= VK_NUMPAD0 && wParam <= VK_DIVIDE) ||*/ //TODO:
                //    (wParam >= VK_PRIOR/*0x21*/ && wParam <= VK_HELP/*0x2F*/) ||
                //    nLCtrl || nRCtrl ||
                //    ((nLAlt || nRAlt) && !(nLCtrl || nRCtrl || nShift) && (wParam >= VK_NUMPAD0/*0x60*/ && wParam <= VK_NUMPAD9/*0x69*/)) || // Ввод Alt-цифры при включенном NumLock
                //    FALSE)
                //{
                    r.Event.KeyEvent.wRepeatCount = 1; TODO("0-15 ? Specifies the repeat count for the current message. The value is the number of times the keystroke is autorepeated as a result of the user holding down the key. If the keystroke is held long enough, multiple messages are sent. However, the repeat count is not cumulative.");
                    r.Event.KeyEvent.wVirtualKeyCode = mn_LastVKeyPressed;
                    r.Event.KeyEvent.uChar.UnicodeChar = pszChars[0];
                    //if (!nLCtrl && !nRCtrl) {
                    //    if (wParam == VK_ESCAPE || wParam == VK_RETURN || wParam == VK_BACK || wParam == VK_TAB || wParam == VK_SPACE
                    //        || FALSE)
                    //        r.Event.KeyEvent.uChar.UnicodeChar = wParam;
                    //}
                //    mn_LastVKeyPressed = 0; // чтобы не обрабатывать WM_(SYS)CHAR
                //} else {
                //    return;
                //}
                r.Event.KeyEvent.bKeyDown = (messg == WM_KEYDOWN || messg == WM_SYSKEYDOWN);
            //}

            r.Event.KeyEvent.wVirtualScanCode = ((DWORD)lParam & 0xFF0000) >> 16; // 16-23 - Specifies the scan code. The value depends on the OEM.
            // 24 - Specifies whether the key is an extended key, such as the right-hand ALT and CTRL keys that appear on an enhanced 101- or 102-key keyboard. The value is 1 if it is an extended key; otherwise, it is 0.
            // 29 - Specifies the context code. The value is 1 if the ALT key is held down while the key is pressed; otherwise, the value is 0.
            // 30 - Specifies the previous key state. The value is 1 if the key is down before the message is sent, or it is 0 if the key is up.
            // 31 - Specifies the transition state. The value is 1 if the key is being released, or it is 0 if the key is being pressed.
            r.Event.KeyEvent.dwControlKeyState = 0;
            if (((DWORD)lParam & (DWORD)(1 << 24)) != 0)
                r.Event.KeyEvent.dwControlKeyState |= ENHANCED_KEY;
            if ((nCaps & 1) == 1)
                r.Event.KeyEvent.dwControlKeyState |= CAPSLOCK_ON;
            if ((nNum & 1) == 1)
                r.Event.KeyEvent.dwControlKeyState |= NUMLOCK_ON;
            if ((nScroll & 1) == 1)
                r.Event.KeyEvent.dwControlKeyState |= SCROLLLOCK_ON;
            if (nLAlt & 0x8000)
                r.Event.KeyEvent.dwControlKeyState |= LEFT_ALT_PRESSED;
            if (nRAlt & 0x8000)
                r.Event.KeyEvent.dwControlKeyState |= RIGHT_ALT_PRESSED;
            if (nLCtrl & 0x8000)
                r.Event.KeyEvent.dwControlKeyState |= LEFT_CTRL_PRESSED;
            if (nRCtrl & 0x8000)
                r.Event.KeyEvent.dwControlKeyState |= RIGHT_CTRL_PRESSED;
            if (nShift & 0x8000)
                r.Event.KeyEvent.dwControlKeyState |= SHIFT_PRESSED;


            #ifdef _DEBUG
            if (r.EventType == KEY_EVENT && r.Event.KeyEvent.bKeyDown &&
                r.Event.KeyEvent.wVirtualKeyCode == VK_F11)
            {
                DEBUGSTRINPUT(L"  ---  F11 sending\n");
            }
            #endif


			// -- заменено на перехват функции ScreenToClient
			//// Сделано (пока) только чтобы текстовое EMenu активировалось по центру консоли,
			//// а не в положении мыши (что смотрится отвратно - оно может сплющиться до 2-3 строк).
			//// Только при скрытой консоли.
			//RemoveFromCursor();

            
            if (mn_FarPID && mn_FarPID != mn_LastSetForegroundPID)
			{
                //DWORD dwFarPID = GetFarPID();
                //if (dwFarPID)
                AllowSetForegroundWindow(mn_FarPID);
                mn_LastSetForegroundPID = mn_FarPID;
            }
            
            PostConsoleEvent(&r);

			// нажатие клавиши может трансформироваться в последовательность нескольких символов...
			/*
			The expected behaviour would be (as it is in a cmd.exe session):
			- hit "^" -> see nothing
			- hit "^" again -> see ^^
			- hit "^" again -> see nothing
			- hit "^" again -> see ^^

			Alternatively:
			- hit "^" -> see nothing
			- hit any other alpha-numeric key, e.g. "k" -> see "^k"
			*/
			for (int i = 1; pszChars[i]; i++)
			{
				r.Event.KeyEvent.uChar.UnicodeChar = pszChars[i];
				PostConsoleEvent(&r);
			}

            //if (messg == WM_CHAR || messg == WM_SYSCHAR) {
            //    // И сразу посылаем отпускание
            //    r.Event.KeyEvent.bKeyDown = FALSE;
            //    PostConsoleEvent(&r);
            //}
        }
    }

    /*if (IsDebuggerPresent()) {
        if (hWnd ==ghWnd)
            DEBUGSTRINPUT(L"   focused ghWnd\n"); else
        if (hWnd ==hConWnd)
            DEBUGSTRINPUT(L"   focused hConWnd\n"); else
        if (hWnd ==ghWndDC)
            DEBUGSTRINPUT(L"   focused ghWndDC\n"); 
        else
            DEBUGSTRINPUT(L"   focused UNKNOWN\n"); 
    }*/

    return;
}

void CRealConsole::OnWinEvent(DWORD anEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    _ASSERTE(hwnd!=NULL);
    
	if (hConWnd == NULL && anEvent == EVENT_CONSOLE_START_APPLICATION && idObject == (LONG)mn_ConEmuC_PID) {
		SetConStatus(L"Waiting for console server...");
        SetHwnd ( hwnd );
	}

    _ASSERTE(hConWnd!=NULL && hwnd==hConWnd);
    
    //TODO("!!! Сделать обработку событий и население m_Processes");
    //
    //AddProcess(idobject), и удаление idObject из списка процессов
    // Не забыть, про обработку флажка Ntvdm
    
    TODO("При отцеплении от консоли NTVDM - можно прибить этот процесс");
    switch(anEvent)
    {
    case EVENT_CONSOLE_START_APPLICATION:
        //A new console process has started. 
        //The idObject parameter contains the process identifier of the newly created process. 
        //If the application is a 16-bit application, the idChild parameter is CONSOLE_APPLICATION_16BIT and idObject is the process identifier of the NTVDM session associated with the console.
        {   
            if (mn_InRecreate>=1)
                mn_InRecreate = 0; // корневой процесс успешно пересоздался
                
            //WARNING("Тут можно повиснуть, если нарваться на блокировку: процесс может быть добавлен и через сервер");
            //Process Add(idObject);
            
            // Если запущено 16битное приложение - необходимо повысить приоритет нашего процесса, иначе будут тормоза
			#ifndef WIN64
            _ASSERTE(CONSOLE_APPLICATION_16BIT==1);
            if (idChild == CONSOLE_APPLICATION_16BIT) {
				DEBUGSTRPROC(L"16 bit application STARTED\n");
				if (mn_Comspec4Ntvdm == 0) {
					// mn_Comspec4Ntvdm может быть еще не заполнен, если 16бит вызван из батника
					_ASSERTE(mn_Comspec4Ntvdm != 0);
				}
				if (!(mn_ProgramStatus & CES_NTVDM))
					mn_ProgramStatus |= CES_NTVDM;
				if (gOSVer.dwMajorVersion>5 || (gOSVer.dwMajorVersion==5 && gOSVer.dwMinorVersion>=1))
					mb_IgnoreCmdStop = TRUE;
                SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
            }
			#endif
        } break;
    case EVENT_CONSOLE_END_APPLICATION:
        //A console process has exited. 
        //The idObject parameter contains the process identifier of the terminated process.
        {
            //WARNING("Тут можно повиснуть, если нарваться на блокировку: процесс может быть удален и через сервер");
            //Process Delete(idObject);
            
            //
			#ifndef WIN64
            if (idChild == CONSOLE_APPLICATION_16BIT) {
                //gConEmu.gbPostUpdateWindowSize = true;
				DEBUGSTRPROC(L"16 bit application TERMINATED\n");
				WARNING("Не сбрасывать CES_NTVDM сразу. Еще не отработал возврат размера консоли!");
				if (mn_Comspec4Ntvdm == 0) {
					mn_ProgramStatus &= ~CES_NTVDM;
				}
				//2010-02-26 убрал. может прийти с задержкой и только создать проблемы
				//SyncConsole2Window(); // После выхода из 16bit режима хорошо бы отресайзить консоль по размеру GUI
                TODO("Вообще-то хорошо бы проверить, что 16бит не осталось в других консолях");
                SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
            }
			#endif
        } break;
    }
}


DWORD CRealConsole::RConServerThread(LPVOID lpvParam) 
{ 
    CRealConsole *pRCon = (CRealConsole*)lpvParam;
	CVirtualConsole *pVCon = pRCon->mp_VCon;
    BOOL fConnected = FALSE;
    DWORD dwErr = 0;
    HANDLE hPipe = NULL; 
    HANDLE hWait[2] = {NULL,NULL};
    DWORD dwTID = GetCurrentThreadId();

	MCHKHEAP
	_ASSERTE(pVCon!=NULL);
    _ASSERTE(pRCon->hConWnd!=NULL);
    _ASSERTE(pRCon->ms_VConServer_Pipe[0]!=0);
    _ASSERTE(pRCon->mh_ServerSemaphore!=NULL);
	_ASSERTE(pRCon->mh_TermEvent!=NULL);
    //wsprintf(pRCon->ms_VConServer_Pipe, CEGUIPIPENAME, L".", (DWORD)pRCon->hConWnd); //был mn_ConEmuC_PID

    // The main loop creates an instance of the named pipe and 
    // then waits for a client to connect to it. When the client 
    // connects, a thread is created to handle communications 
    // with that client, and the loop is repeated. 
    
    hWait[0] = pRCon->mh_TermEvent;
    hWait[1] = pRCon->mh_ServerSemaphore;

	MCHKHEAP

    // Пока не затребовано завершение консоли
    do {
        while (!fConnected)
        { 
            _ASSERTE(hPipe == NULL);

			// !!! Переносить проверку семафора ПОСЛЕ CreateNamedPipe нельзя, т.к. в этом случае
			//     нити дерутся и клиент не может подцепиться к серверу
			
			// Дождаться разрешения семафора, или закрытия консоли
			dwErr = WaitForMultipleObjects ( 2, hWait, FALSE, INFINITE );
			if (dwErr == WAIT_OBJECT_0) {
				return 0; // Консоль закрывается
			}

			MCHKHEAP
			for (int i=0; i<MAX_SERVER_THREADS; i++) {
				if (pRCon->mn_RConServerThreadsId[i] == dwTID) {
					pRCon->mh_ActiveRConServerThread = pRCon->mh_RConServerThreads[i]; break;
				}
			}

			_ASSERTE(gpNullSecurity);
            hPipe = CreateNamedPipe( 
                pRCon->ms_VConServer_Pipe, // pipe name 
                PIPE_ACCESS_DUPLEX,       // read/write access 
                PIPE_TYPE_MESSAGE |       // message type pipe 
                PIPE_READMODE_MESSAGE |   // message-read mode 
                PIPE_WAIT,                // blocking mode 
                PIPE_UNLIMITED_INSTANCES, // max. instances  
                PIPEBUFSIZE,              // output buffer size 
                PIPEBUFSIZE,              // input buffer size 
                0,                        // client time-out 
                gpNullSecurity);          // default security attribute 

			MCHKHEAP
            if (hPipe == INVALID_HANDLE_VALUE) 
            {
				dwErr = GetLastError();

				_ASSERTE(hPipe != INVALID_HANDLE_VALUE);
                //DisplayLastError(L"CreateNamedPipe failed"); 
                hPipe = NULL;
				// Разрешить этой или другой нити создать серверный пайп
				ReleaseSemaphore(hWait[1], 1, NULL);
                //Sleep(50);
                continue;
            }
			MCHKHEAP

			// Чтобы ConEmuC знал, что серверный пайп готов
			if (pRCon->mh_GuiAttached) {
        		SetEvent(pRCon->mh_GuiAttached);
        		SafeCloseHandle(pRCon->mh_GuiAttached);
   			}


            // Wait for the client to connect; if it succeeds, 
            // the function returns a nonzero value. If the function
            // returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 

            fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : ((dwErr = GetLastError()) == ERROR_PIPE_CONNECTED); 

			MCHKHEAP
            // сразу разрешить другой нити принять вызов
            ReleaseSemaphore(hWait[1], 1, NULL);

            // Консоль закрывается!
            if (WaitForSingleObject ( hWait[0], 0 ) == WAIT_OBJECT_0) {
                //FlushFileBuffers(hPipe); -- это не нужно, мы ничего не возвращали
                //DisconnectNamedPipe(hPipe); 
                SafeCloseHandle(hPipe);
                return 0;
            }

			MCHKHEAP
            if (fConnected)
            	break;
            else
                SafeCloseHandle(hPipe);
        }

        if (fConnected) {
            // сразу сбросим, чтобы не забыть
            fConnected = FALSE;
            //// разрешить другой нити принять вызов //2009-08-28 перенесено сразу после ConnectNamedPipe
            //ReleaseSemaphore(hWait[1], 1, NULL);
            
			MCHKHEAP
			if (gConEmu.isValid(pVCon)) {
				_ASSERTE(pVCon==pRCon->mp_VCon);
	            pRCon->ServerThreadCommand ( hPipe ); // При необходимости - записывает в пайп результат сама
			} else {
				_ASSERT(FALSE);
				// Проблема. VirtualConsole закрыта, а нить еще не завершилась! хотя должна была...
				SafeCloseHandle(hPipe);
				return 1;
			}
        }

		MCHKHEAP
        FlushFileBuffers(hPipe); 
        //DisconnectNamedPipe(hPipe); 
        SafeCloseHandle(hPipe);
    } // Перейти к открытию нового instance пайпа
    while (WaitForSingleObject ( pRCon->mh_TermEvent, 0 ) != WAIT_OBJECT_0);

    return 0; 
}

// Эта функция пайп не закрывает!
void CRealConsole::ServerThreadCommand(HANDLE hPipe)
{
    CESERVER_REQ in={{0}}, *pIn=NULL;
    DWORD cbRead = 0, cbWritten = 0, dwErr = 0;
    BOOL fSuccess = FALSE;
	#ifdef _DEBUG
	HANDLE lhConEmuC = mh_ConEmuC;
	#endif

	MCHKHEAP;

    // Send a message to the pipe server and read the response. 
    fSuccess = ReadFile( 
        hPipe,            // pipe handle 
        &in,              // buffer to receive reply
        sizeof(in),       // size of read buffer
        &cbRead,          // bytes read
        NULL);            // not overlapped 

    if (!fSuccess && ((dwErr = GetLastError()) != ERROR_MORE_DATA)) 
    {
		#ifdef _DEBUG
		// Если консоль закрывается - MonitorThread в ближайшее время это поймет
		DEBUGSTRPROC(L"!!! ReadFile(pipe) failed - console in close?\n");
		//DWORD dwWait = WaitForSingleObject ( mh_TermEvent, 0 );
		//if (dwWait == WAIT_OBJECT_0) return;
		//Sleep(1000);
		//if (lhConEmuC != mh_ConEmuC)
		//	dwWait = WAIT_OBJECT_0;
		//else
		//	dwWait = WaitForSingleObject ( mh_ConEmuC, 0 );
		//if (dwWait == WAIT_OBJECT_0) return;
		//_ASSERTE("ReadFile(pipe) failed"==NULL);
		#endif
        //CloseHandle(hPipe);
        return;
    }
    if (in.hdr.nVersion != CESERVER_REQ_VER) {
        gConEmu.ShowOldCmdVersion(in.hdr.nCmd, in.hdr.nVersion, in.hdr.nSrcPID==GetServerPID() ? 1 : 0);
        return;
    }
    _ASSERTE(in.hdr.cbSize>=sizeof(CESERVER_REQ_HDR) && cbRead>=sizeof(CESERVER_REQ_HDR));
    if (cbRead < sizeof(CESERVER_REQ_HDR) || /*in.hdr.cbSize < cbRead ||*/ in.hdr.nVersion != CESERVER_REQ_VER) {
        //CloseHandle(hPipe);
        return;
    }

    if (in.hdr.cbSize <= cbRead) {
        pIn = &in; // выделение памяти не требуется
    } else {
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
        if (nAllSize>0) {
            //CloseHandle(hPipe);
            return; // удалось считать не все данные
        }
    }

    
    UINT nDataSize = pIn->hdr.cbSize - sizeof(CESERVER_REQ_HDR);

    // Все данные из пайпа получены, обрабатываем команду и возвращаем (если нужно) результат

	//  //if (pIn->hdr.nCmd == CECMD_GETFULLINFO /*|| pIn->hdr.nCmd == CECMD_GETSHORTINFO*/) {
	//  if (pIn->hdr.nCmd == CECMD_GETCONSOLEINFO) {
	//  	_ASSERTE(pIn->hdr.nCmd != CECMD_GETCONSOLEINFO);
	//// только если мы НЕ в цикле ресайза. иначе не страшно, устаревшие пакеты пропустит PopPacket
	////if (!con.bInSetSize && !con.bBufferHeight && pIn->ConInfo.inf.sbi.dwSize.Y > 200) {
	////	_ASSERTE(con.bBufferHeight || pIn->ConInfo.inf.sbi.dwSize.Y <= 200);
	////}
	////#ifdef _DEBUG
	////wchar_t szDbg[255]; wsprintf(szDbg, L"GUI recieved %s, PktID=%i, Tick=%i\n", 
	////	(pIn->hdr.nCmd == CECMD_GETFULLINFO) ? L"CECMD_GETFULLINFO" : L"CECMD_GETSHORTINFO",
	////	pIn->ConInfo.inf.nPacketId, pIn->hdr.nCreateTick);
	//      //DEBUGSTRCMD(szDbg);
	////#endif
	//      ////ApplyConsoleInfo(pIn);
	//      //if (((LPVOID)&in)==((LPVOID)pIn)) {
	//      //    // Это фиксированная память - переменная (in)
	//      //    _ASSERTE(in.hdr.cbSize>0);
	//      //    // Для его обработки нужно создать копию памяти, которую освободит PopPacket
	//      //    pIn = (CESERVER_REQ*)calloc(in.hdr.cbSize,1);
	//      //    memmove(pIn, &in, in.hdr.cbSize);
	//      //}
	//      //PushPacket(pIn);
	//      //pIn = NULL;

	//  } else 

	if (pIn->hdr.nCmd == CECMD_CMDSTARTSTOP)
	{
        //
		DWORD nStarted = pIn->StartStop.nStarted;

		HWND  hWnd     = (HWND)pIn->StartStop.hWnd;
        #ifdef _DEBUG
		wchar_t szDbg[128];
		switch (nStarted) {
			case 0: wsprintf(szDbg, L"GUI received CECMD_CMDSTARTSTOP(ServerStart,%i)\n", pIn->hdr.nCreateTick); break;
			case 1: wsprintf(szDbg, L"GUI received CECMD_CMDSTARTSTOP(ServerStop,%i)\n", pIn->hdr.nCreateTick); break;
			case 2: wsprintf(szDbg, L"GUI received CECMD_CMDSTARTSTOP(ComspecStart,%i)\n", pIn->hdr.nCreateTick); break;
			case 3: wsprintf(szDbg, L"GUI received CECMD_CMDSTARTSTOP(ComspecStop,%i)\n", pIn->hdr.nCreateTick); break;
		}
		DEBUGSTRCMD(szDbg);
        #endif

        DWORD nPID     = pIn->StartStop.dwPID;
		DWORD nSubSystem = pIn->StartStop.nSubSystem;
		BOOL bRunViaCmdExe = pIn->StartStop.bRootIsCmdExe;
		BOOL bUserIsAdmin = pIn->StartStop.bUserIsAdmin;
		BOOL lbWasBuffer = pIn->StartStop.bWasBufferHeight;
		//DWORD nInputTID = pIn->StartStop.dwInputTID;

		_ASSERTE(sizeof(CESERVER_REQ_STARTSTOPRET) <= sizeof(CESERVER_REQ_STARTSTOP));
		pIn->hdr.cbSize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_STARTSTOPRET);
        
        if (nStarted == 0 || nStarted == 2) {
            // Сразу заполним результат
            pIn->StartStopRet.bWasBufferHeight = isBufferHeight(); // чтобы comspec знал, что буфер нужно будет отключить
			
			//DWORD nParentPID = 0;
			//if (nStarted == 2)
			//{
			//	ConProcess* pPrc = NULL;
			//	int i, nProcCount = GetProcesses(&pPrc);
			//	if (pPrc != NULL)
			//	{ 				
			//		for (i = 0; i < nProcCount; i++) {
			//			if (pPrc[i].ProcessID == nPID) {
			//				nParentPID = pPrc[i].ParentPID; break;
			//			}
			//		}
			//		if (nParentPID == 0) {
			//			_ASSERTE(nParentPID != 0);
			//		} else {
			//			BOOL lbFar = FALSE;
			//			for (i = 0; i < nProcCount; i++) {
			//				if (pPrc[i].ProcessID == nParentPID) {
			//					lbFar = pPrc[i].IsFar; break;
			//				}
			//			}
			//			if (!lbFar) {
			//				_ASSERTE(lbFar);
			//				nParentPID = 0;
			//			}
			//		}
			//		free(pPrc);
			//	}
			//}
			//pIn->StartStopRet.bWasBufferHeight = FALSE;// (nStarted == 2) && (nParentPID == 0); // comspec должен уведомить о завершении
			pIn->StartStopRet.hWnd = ghWnd;
			pIn->StartStopRet.hWndDC = ghWndDC;
			pIn->StartStopRet.dwPID = GetCurrentProcessId();
			pIn->StartStopRet.dwSrvPID = mn_ConEmuC_PID;
			pIn->StartStopRet.bNeedLangChange = FALSE;

			if (nStarted == 0) {
				//_ASSERTE(nInputTID);
				//_ASSERTE(mn_ConEmuC_Input_TID==0 || mn_ConEmuC_Input_TID==nInputTID);
				//mn_ConEmuC_Input_TID = nInputTID;
				//
				if (!m_Args.bRunAsAdministrator && bUserIsAdmin)
					m_Args.bRunAsAdministrator = TRUE;

				// Если один Layout на все консоли
				if ((gSet.isMonitorConsoleLang & 2) == 2) {
					// Команду - низя, нити еще не функционируют
					//	SwitchKeyboardLayout(INPUTLANGCHANGE_SYSCHARSET,gConEmu.GetActiveKeyboardLayout());
					pIn->StartStopRet.bNeedLangChange = TRUE;
					TODO("Проверить на x64, не будет ли проблем с 0xFFFFFFFFFFFFFFFFFFFFF");
					pIn->StartStopRet.NewConsoleLang = gConEmu.GetActiveKeyboardLayout();
				}


				// Теперь мы гарантированно знаем дескриптор окна консоли
				SetHwnd(hWnd);

				// Открыть мэппинги, выставить KeyboardLayout, и т.п.
				OnServerStarted();
			}

            AllowSetForegroundWindow(nPID);
            
			// 100627 - con.m_sbi.dwSize.Y более использовать некорректно ввиду "far/w"
            COORD cr16bit = {80,con.m_sbi.srWindow.Bottom-con.m_sbi.srWindow.Top+1};
			if (nSubSystem == 255) {
				if (gSet.ntvdmHeight && cr16bit.Y >= (int)gSet.ntvdmHeight) cr16bit.Y = gSet.ntvdmHeight;
				else if (cr16bit.Y>=50) cr16bit.Y = 50;
				else if (cr16bit.Y>=43) cr16bit.Y = 43;
				else if (cr16bit.Y>=28) cr16bit.Y = 28;
				else if (cr16bit.Y>=25) cr16bit.Y = 25;
			}

            // ComSpec started
            if (nStarted == 2) {
				// Устанавливается в TRUE если будет запущено 16битное приложение
				if (nSubSystem == 255) {
					DEBUGSTRCMD(L"16 bit application STARTED, aquired from CECMD_CMDSTARTSTOP\n");
					if (!(mn_ProgramStatus & CES_NTVDM))
						mn_ProgramStatus |= CES_NTVDM;
					mn_Comspec4Ntvdm = nPID;
					mb_IgnoreCmdStop = TRUE;
					
					SetConsoleSize(cr16bit.X, cr16bit.Y, 0, CECMD_CMDSTARTED);
					
					pIn->StartStopRet.nBufferHeight = 0;
					pIn->StartStopRet.nWidth = cr16bit.X;
					pIn->StartStopRet.nHeight = cr16bit.Y;
				} else {
					// но пока его нужно сбросить
					mb_IgnoreCmdStop = FALSE;

					// в ComSpec передаем именно то, что сейчас в gSet
					pIn->StartStopRet.nBufferHeight = gSet.DefaultBufferHeight;
					pIn->StartStopRet.nWidth = con.m_sbi.dwSize.X;
					pIn->StartStopRet.nHeight = con.m_sbi.dwSize.Y;

					if (gSet.DefaultBufferHeight && !isBufferHeight()) {
						WARNING("Тут наверное нужно бы заблокировать прием команды смена размера из сервера ConEmuC");
						#if 0
						con.m_sbi.dwSize.Y = gSet.DefaultBufferHeight;
						mb_BuferModeChangeLocked = TRUE;
						SetBufferHeightMode(TRUE, TRUE); // Сразу включаем, иначе команда неправильно сформируется
						SetConsoleSize(con.m_sbi.dwSize.X, TextHeight(), 0, CECMD_CMDSTARTED);
						mb_BuferModeChangeLocked = FALSE;
						#else
						//con.m_sbi.dwSize.Y = gSet.DefaultBufferHeight; -- не будем менять сразу, а то SetConsoleSize просто skip
						mb_BuferModeChangeLocked = TRUE;
						SetBufferHeightMode(TRUE, TRUE); // Сразу включаем, иначе команда неправильно сформируется
						SetConsoleSize(con.m_sbi.dwSize.X, TextHeight(), gSet.DefaultBufferHeight, CECMD_CMDSTARTED);
						mb_BuferModeChangeLocked = FALSE;
						#endif
					}
				}
            } else if (nStarted == 0) {
            	// Server
            	if (nSubSystem == 255) {
					pIn->StartStopRet.nBufferHeight = 0;
					pIn->StartStopRet.nWidth = cr16bit.X;
					pIn->StartStopRet.nHeight = cr16bit.Y;
            	} else {
					pIn->StartStopRet.nWidth = con.m_sbi.dwSize.X;
					//0x101 - запуск отладчика
					if (nSubSystem != 0x100  // 0x100 - Аттач из фар-плагина
						&& (con.bBufferHeight
							|| (con.DefaultBufferHeight && bRunViaCmdExe)))
					{
						_ASSERTE(m_Args.bDetached || con.DefaultBufferHeight == con.m_sbi.dwSize.Y || con.m_sbi.dwSize.Y == TextHeight());
						con.bBufferHeight = TRUE;
						con.nBufferHeight = max(con.m_sbi.dwSize.Y,con.DefaultBufferHeight);
						con.m_sbi.dwSize.Y = con.nBufferHeight; // Сразу обновить, иначе буфер может сброситься самопроизвольно
						pIn->StartStopRet.nBufferHeight = con.nBufferHeight;
						_ASSERTE(con.nTextHeight > 5);
						pIn->StartStopRet.nHeight = con.nTextHeight;
					} else {
						_ASSERTE(!con.bBufferHeight);
						pIn->StartStopRet.nBufferHeight = 0;
						pIn->StartStopRet.nHeight = con.m_sbi.dwSize.Y;
					}					
            	}

				SetBufferHeightMode((pIn->StartStopRet.nBufferHeight != 0), TRUE);
				con.nChange2TextWidth = pIn->StartStopRet.nWidth;
				con.nChange2TextHeight = pIn->StartStopRet.nHeight;
				if (con.nChange2TextHeight > 200) {
					_ASSERTE(con.nChange2TextHeight <= 200);
				}
            }

            // 23.06.2009 Maks - уберем пока. Должно работать в ApplyConsoleInfo
            //Process Add(nPID);

        } else {
            // 23.06.2009 Maks - уберем пока. Должно работать в ApplyConsoleInfo
            //Process Delete(nPID);

			// ComSpec stopped
            if (nStarted == 3) {
				BOOL lbNeedResizeWnd = FALSE;
				BOOL lbNeedResizeGui = FALSE;
				COORD crNewSize = {TextWidth(),TextHeight()};
				int nNewWidth=0, nNewHeight=0; BOOL bBufferHeight = FALSE;
				if ((mn_ProgramStatus & CES_NTVDM) == 0
					&& !(gSet.isFullScreen || gConEmu.isZoomed()))
				{
					pIn->StartStopRet.bWasBufferHeight = FALSE;
					// В некоторых случаях (comspec без консоли?) GetConsoleScreenBufferInfo обламывается
					if (pIn->StartStop.sbi.dwSize.X && pIn->StartStop.sbi.dwSize.Y) {
						if (GetConWindowSize(pIn->StartStop.sbi, nNewWidth, nNewHeight, &bBufferHeight)) {
							lbNeedResizeGui = (crNewSize.X != nNewWidth || crNewSize.Y != nNewHeight);
							if (bBufferHeight || crNewSize.X != nNewWidth || crNewSize.Y != nNewHeight) {
								//gConEmu.SyncWindowToConsole(); - его использовать нельзя. во первых это не главная нить, во вторых - размер pVCon может быть еще не изменен
								lbNeedResizeWnd = TRUE;
								crNewSize.X = nNewWidth;
								crNewSize.Y = nNewHeight;
								pIn->StartStopRet.bWasBufferHeight = TRUE;
							}
						}
					}
				}

				if (mb_IgnoreCmdStop || (mn_ProgramStatus & CES_NTVDM) == CES_NTVDM) {
					// Ветка активируется только в WinXP и выше
					// Было запущено 16битное приложение, сейчас запомненный размер консоли скорее всего 80x25
					// что не соответствует желаемому размеру при выходе из 16бит. Консоль нужно подресайзить
					// поз размер окна. Это сделает OnWinEvent.
					//SetBufferHeightMode(FALSE, TRUE);
					//PRAGMA_ERROR("Если не вызвать CECMD_CMDFINISHED не включатся WinEvents");
					mb_IgnoreCmdStop = FALSE; // наверное сразу сбросим, две подряд прийти не могут
					
    				DEBUGSTRCMD(L"16 bit application TERMINATED (aquired from CECMD_CMDFINISHED)\n");
                    //mn_ProgramStatus &= ~CES_NTVDM; -- сбросим после синхронизации размера консоли, чтобы не слетел
					if (lbWasBuffer) {
						SetBufferHeightMode(TRUE, TRUE); // Сразу выключаем, иначе команда неправильно сформируется
					}
    				SyncConsole2Window(TRUE); // После выхода из 16bit режима хорошо бы отресайзить консоль по размеру GUI
					if (mn_Comspec4Ntvdm && mn_Comspec4Ntvdm != nPID) {
						_ASSERTE(mn_Comspec4Ntvdm == nPID);
					}
					mn_ProgramStatus &= ~CES_NTVDM;
					lbNeedResizeWnd = FALSE;
					crNewSize.X = TextWidth();
					crNewSize.Y = TextHeight();
				} //else {
				// Восстановить размер через серверный ConEmuC
				mb_BuferModeChangeLocked = TRUE;
				con.m_sbi.dwSize.Y = crNewSize.Y;
				if (!lbWasBuffer) {
					SetBufferHeightMode(FALSE, TRUE); // Сразу выключаем, иначе команда неправильно сформируется
				}

				#ifdef _DEBUG
				wsprintf(szDbg, L"Returns normal window size begin at %i\n", GetTickCount());
				DEBUGSTRCMD(szDbg);
				#endif

				// Обязательно. Иначе сервер не узнает, что команда завершена
				SetConsoleSize(crNewSize.X, crNewSize.Y, 0, CECMD_CMDFINISHED);

				#ifdef _DEBUG
				wsprintf(szDbg, L"Finished returns normal window size begin at %i\n", GetTickCount());
				DEBUGSTRCMD(szDbg);
				#endif

#ifdef _DEBUG
		#ifdef WIN64
//				PRAGMA_ERROR("Есть подозрение, что после этого на Win7 x64 приходит старый пакет с буферной высотой и возникает уже некорректная синхронизация размера!");
		#endif
#endif
				// может nChange2TextWidth, nChange2TextHeight нужно использовать?
				
				if (lbNeedResizeGui) {
					RECT rcCon = MakeRect(nNewWidth, nNewHeight);
					RECT rcNew = gConEmu.CalcRect(CER_MAIN, rcCon, CER_CONSOLE);
					RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);
					if (gSet.isDesktopMode) {
						MapWindowPoints(NULL, gConEmu.mh_ShellWindow, (LPPOINT)&rcWnd, 2);
					}
					MOVEWINDOW ( ghWnd, rcWnd.left, rcWnd.top, rcNew.right, rcNew.bottom, 1);
				}
				mb_BuferModeChangeLocked = FALSE;
				//}
            } else {
            	// сюда мы попадать не должны!
            	_ASSERT(FALSE);
            }
        }
        
        // Отправляем
        fSuccess = WriteFile( 
            hPipe,        // handle to pipe 
            pIn,         // buffer to write from 
            pIn->hdr.cbSize,  // number of bytes to write 
            &cbWritten,   // number of bytes written 
            NULL);        // not overlapped I/O 

    }
    else if (pIn->hdr.nCmd == CECMD_GETGUIHWND)
    {
        DEBUGSTRCMD(L"GUI recieved CECMD_GETGUIHWND\n");
        CESERVER_REQ *pRet = ExecuteNewCmd(pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR) + 2*sizeof(DWORD));
        pRet->dwData[0] = (DWORD)ghWnd;
        pRet->dwData[1] = (DWORD)ghWndDC;
        // Отправляем
        fSuccess = WriteFile( 
            hPipe,        // handle to pipe 
            pRet,         // buffer to write from 
            pRet->hdr.cbSize,  // number of bytes to write 
            &cbWritten,   // number of bytes written 
            NULL);        // not overlapped I/O 
        ExecuteFreeResult(pRet);

    }
    else if (pIn->hdr.nCmd == CECMD_TABSCHANGED)
    {
        DEBUGSTRCMD(L"GUI recieved CECMD_TABSCHANGED\n");
        if (nDataSize == 0) {
            // ФАР закрывается
			if (pIn->hdr.nSrcPID == mn_FarPID) {
				mn_ProgramStatus &= ~CES_FARACTIVE;
				mn_FarPID_PluginDetected = mn_FarPID = 0;
				CloseFarMapData();
				if (isActive()) gConEmu.UpdateProcessDisplay(FALSE); // обновить PID в окне настройки
			}
            SetTabs(NULL, 1);
        } else {
            _ASSERTE(nDataSize>=4);
            _ASSERTE(((pIn->Tabs.nTabCount-1)*sizeof(ConEmuTab))==(nDataSize-sizeof(CESERVER_REQ_CONEMUTAB)));
			BOOL lbCanUpdate = TRUE;
			// Если выполняется макрос - изменение размеров окна FAR при авто-табах нежелательно
			if (pIn->Tabs.bMacroActive) {
				if (gSet.isTabs == 2) {
					lbCanUpdate = FALSE;
					// Нужно вернуть в плагин информацию, что нужно отложенное обновление
					CESERVER_REQ *pRet = ExecuteNewCmd(CECMD_TABSCHANGED, sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_CONEMUTAB_RET));
					if (pRet) {
						pRet->TabsRet.bNeedPostTabSend = TRUE;
						// Отправляем
						fSuccess = WriteFile( 
							hPipe,        // handle to pipe 
							pRet,         // buffer to write from 
							pRet->hdr.cbSize,  // number of bytes to write 
							&cbWritten,   // number of bytes written 
							NULL);        // not overlapped I/O 
						ExecuteFreeResult(pRet);
					}
				}
			}
			// Если включены автотабы - попытаться изменить размер консоли СРАЗУ (в самом FAR)
			// Но только если вызов SendTabs был сделан из основной нити фара (чтобы небыло потребности в Synchro)
			if (pIn->Tabs.bMainThread && lbCanUpdate && gSet.isTabs == 2) {
				TODO("расчитать новый размер, если сменилась видимость табов");
				bool lbCurrentActive = gConEmu.mp_TabBar->IsActive();
				bool lbNewActive = lbCurrentActive;
				// Если консолей более одной - видимость табов не изменится
				if (gConEmu.GetVCon(1) == NULL) {
					lbNewActive = (pIn->Tabs.nTabCount > 1);
				}

				if (lbCurrentActive != lbNewActive) {
					enum ConEmuMargins tTabAction = lbNewActive ? CEM_TABACTIVATE : CEM_TABDEACTIVATE;
					RECT rcConsole = gConEmu.CalcRect(CER_CONSOLE, gConEmu.GetIdealRect(), CER_MAIN, NULL, NULL, tTabAction);

					con.nChange2TextWidth = rcConsole.right;
					con.nChange2TextHeight = rcConsole.bottom;
					if (con.nChange2TextHeight > 200) {
						_ASSERTE(con.nChange2TextHeight <= 200);
					}

					gConEmu.m_Child->SetRedraw(FALSE);
					gConEmu.mp_TabBar->SetRedraw(FALSE);
					fSuccess = FALSE;

					// Нужно вернуть в плагин информацию, что нужно размер консоли
					CESERVER_REQ *pRet = ExecuteNewCmd(CECMD_TABSCHANGED, sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_CONEMUTAB_RET));
					if (pRet) {
						pRet->TabsRet.bNeedResize = TRUE;
						pRet->TabsRet.crNewSize.X = rcConsole.right;
						pRet->TabsRet.crNewSize.Y = rcConsole.bottom;
						// Отправляем
						fSuccess = WriteFile( 
							hPipe,        // handle to pipe 
							pRet,         // buffer to write from 
							pRet->hdr.cbSize,  // number of bytes to write 
							&cbWritten,   // number of bytes written 
							NULL);        // not overlapped I/O 
						ExecuteFreeResult(pRet);
					}

					if (fSuccess) { // Дождаться, пока из сервера придут изменения консоли
						WaitConsoleSize(rcConsole.bottom, 500);
					}
					gConEmu.m_Child->SetRedraw(TRUE);
				}
			}
			if (lbCanUpdate) {
				gConEmu.m_Child->Invalidate();
				SetTabs(pIn->Tabs.tabs, pIn->Tabs.nTabCount);
				gConEmu.mp_TabBar->SetRedraw(TRUE);
				gConEmu.m_Child->Redraw();
			}
        }

    }
    else if (pIn->hdr.nCmd == CECMD_GETOUTPUTFILE)
    {
        DEBUGSTRCMD(L"GUI recieved CECMD_GETOUTPUTFILE\n");
        _ASSERTE(nDataSize>=4);
        BOOL lbUnicode = pIn->OutputFile.bUnicode;

        CESERVER_REQ *pRet = NULL;
        int nSize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_OUTPUTFILE);
        pRet = (CESERVER_REQ*)calloc(nSize, 1);
		ExecutePrepareCmd(pRet, pIn->hdr.nCmd, nSize);

        pRet->OutputFile.bUnicode = lbUnicode;
        pRet->OutputFile.szFilePathName[0] = 0; // Сформирует PrepareOutputFile

        if (!PrepareOutputFile(lbUnicode, pRet->OutputFile.szFilePathName)) {
            pRet->OutputFile.szFilePathName[0] = 0;
        }

        // Отправляем
        fSuccess = WriteFile( 
            hPipe,        // handle to pipe 
            pRet,         // buffer to write from 
            pRet->hdr.cbSize,  // number of bytes to write 
            &cbWritten,   // number of bytes written 
            NULL);        // not overlapped I/O 
        free(pRet);

    }
    else if (pIn->hdr.nCmd == CECMD_LANGCHANGE)
    {
        DEBUGSTRLANG(L"GUI recieved CECMD_LANGCHANGE\n");
        _ASSERTE(nDataSize>=4);
		// LayoutName: "00000409", "00010409", ...
		// А HKL от него отличается, так что передаем DWORD
		// HKL в x64 выглядит как: "0x0000000000020409", "0xFFFFFFFFF0010409"
		DWORD dwName = pIn->dwData[0];

		gConEmu.OnLangChangeConsole(mp_VCon, dwName);

		//#ifdef _DEBUG
		////Sleep(2000);
		//WCHAR szMsg[255];
		//// --> Видимо именно это не "нравится" руслату. Нужно переправить Post'ом в основную нить
		//HKL hkl = GetKeyboardLayout(0);
		//wsprintf(szMsg, L"ConEmu: GetKeyboardLayout(0) on CECMD_LANGCHANGE after GetKeyboardLayout(0) = 0x%08I64X\n",
		//	(unsigned __int64)(DWORD_PTR)hkl);
		//DEBUGSTRLANG(szMsg);
		////Sleep(2000);
		//#endif
		//
		//wchar_t szName[10]; wsprintf(szName, L"%08X", dwName);
        //DWORD_PTR dwNewKeybLayout = (DWORD_PTR)LoadKeyboardLayout(szName, 0);
        //
		//#ifdef _DEBUG
		//DEBUGSTRLANG(L"ConEmu: Calling GetKeyboardLayout(0)\n");
		////Sleep(2000);
		//hkl = GetKeyboardLayout(0);
		//wsprintf(szMsg, L"ConEmu: GetKeyboardLayout(0) after LoadKeyboardLayout = 0x%08I64X\n",
		//	(unsigned __int64)(DWORD_PTR)hkl);
		//DEBUGSTRLANG(szMsg);
		////Sleep(2000);
		//#endif
		//
        //if ((gSet.isMonitorConsoleLang & 1) == 1) {
        //    if (con.dwKeybLayout != dwNewKeybLayout) {
        //        con.dwKeybLayout = dwNewKeybLayout;
		//		if (isActive()) {
        //            gConEmu.SwitchKeyboardLayout(dwNewKeybLayout);
        //
		//			#ifdef _DEBUG
		//			hkl = GetKeyboardLayout(0);
		//			wsprintf(szMsg, L"ConEmu: GetKeyboardLayout(0) after SwitchKeyboardLayout = 0x%08I64X\n",
		//				(unsigned __int64)(DWORD_PTR)hkl);
		//			DEBUGSTRLANG(szMsg);
		//			//Sleep(2000);
		//			#endif
		//		}
        //    }
        //}

    }
    else if (pIn->hdr.nCmd == CECMD_TABSCMD)
    {
        // 0: спрятать/показать табы, 1: перейти на следующую, 2: перейти на предыдущую, 3: commit switch
        DEBUGSTRCMD(L"GUI recieved CECMD_TABSCMD\n");
        _ASSERTE(nDataSize>=1);
        DWORD nTabCmd = pIn->Data[0];
        gConEmu.TabCommand(nTabCmd);

    }
    else if (pIn->hdr.nCmd == CECMD_RESOURCES)
    {
        DEBUGSTRCMD(L"GUI recieved CECMD_RESOURCES\n");
        _ASSERTE(nDataSize>=6);
        mb_PluginDetected = TRUE; // Запомним, что в фаре есть плагин (хотя фар может быть закрыт)
        mn_FarPID_PluginDetected = pIn->dwData[0];
		OpenFarMapData();

        if (isActive()) gConEmu.UpdateProcessDisplay(FALSE); // обновить PID в окне настройки
        //mn_Far_PluginInputThreadId      = pIn->dwData[1];
		
        
        //CheckColorMapping(mn_FarPID_PluginDetected);
        
        // 23.06.2009 Maks - уберем пока. Должно работать в ApplyConsoleInfo
        //Process Add(mn_FarPID_PluginDetected); // На всякий случай, вдруг он еще не в нашем списке?
        wchar_t* pszRes = (wchar_t*)(&(pIn->dwData[1])), *pszNext;
        if (*pszRes) {
            //EnableComSpec(mn_FarPID_PluginDetected, TRUE);
            //UpdateFarSettings(mn_FarPID_PluginDetected);
			wchar_t* pszItems[] = {ms_EditorRus,ms_ViewerRus,ms_TempPanelRus/*,ms_NameTitle*/};

			for (int i = 0; i < countof(pszItems); i++) {
				pszNext = pszRes + lstrlen(pszRes)+1;
				if (lstrlen(pszRes)>=30) pszRes[30] = 0;
				lstrcpy(pszItems[i], pszRes);
				if (i < 2) lstrcat(pszItems[i], L" ");
				pszRes = pszNext;
				if (*pszRes == 0)
					break;
			}

            //pszNext = pszRes + lstrlen(pszRes)+1;
            //if (lstrlen(pszRes)>=30) pszRes[30] = 0;
            //lstrcpy(ms_EditorRus, pszRes); lstrcat(ms_EditorRus, L" ");
            //pszRes = pszNext;

            //if (*pszRes) {
            //    pszNext = pszRes + lstrlen(pszRes)+1;
            //    if (lstrlen(pszRes)>=30) pszRes[30] = 0;
            //    lstrcpy(ms_ViewerRus, pszRes); lstrcat(ms_ViewerRus, L" ");
            //    pszRes = pszNext;

            //    if (*pszRes) {
            //        pszNext = pszRes + lstrlen(pszRes)+1;
            //        if (lstrlen(pszRes)>=31) pszRes[31] = 0;
            //        lstrcpy(ms_TempPanelRus, pszRes);
            //        pszRes = pszNext;
            //    }
            //}
        }

		UpdateFarSettings(mn_FarPID_PluginDetected);


	}
	else if (pIn->hdr.nCmd == CECMD_SETFOREGROUND)
	{
		AllowSetForegroundWindow(pIn->hdr.nSrcPID);
		apiSetForegroundWindow((HWND)pIn->qwData[0]);

	}
	else if (pIn->hdr.nCmd == CECMD_FLASHWINDOW)
	{
		UINT nFlash = RegisterWindowMessage(CONEMUMSG_FLASHWINDOW);
		WPARAM wParam = 0;
		if (pIn->Flash.bSimple) {
			wParam = (pIn->Flash.bInvert ? 2 : 1) << 25;
		} else {
			wParam = ((pIn->Flash.dwFlags & 0xF) << 24) | (pIn->Flash.uCount & 0xFFFFFF);
		}
		PostMessage(ghWnd, nFlash, wParam, (LPARAM)pIn->Flash.hWnd.u);

	}
	else if (pIn->hdr.nCmd == CECMD_REGPANELVIEW)
	{
		CESERVER_REQ Out;
		ExecutePrepareCmd(&Out, pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR)+sizeof(pIn->PVI));
		Out.PVI = pIn->PVI;

		if (Out.PVI.cbSize != sizeof(Out.PVI)) {
			Out.PVI.cbSize = 0; // ошибка версии?
		} else
		if (!mp_VCon->RegisterPanelView(&(Out.PVI))) {
			Out.PVI.cbSize = 0; // ошибка
		}
		
        // Отправляем ответ
        fSuccess = WriteFile( 
            hPipe,        // handle to pipe 
            &Out,         // buffer to write from 
            Out.hdr.cbSize,  // number of bytes to write 
            &cbWritten,   // number of bytes written 
            NULL);        // not overlapped I/O 
	
	}
	else if (pIn->hdr.nCmd == CECMD_SETBACKGROUND)
	{
		CESERVER_REQ Out;
		ExecutePrepareCmd(&Out, pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_SETBACKGROUNDRET));
		// Set background Image
		Out.BackgroundRet.nResult = mp_VCon->SetBackgroundImageData(&pIn->Background);
		//(pIn->Background.bEnabled ? (&pIn->Background.bmp) : NULL);
		fSuccess = WriteFile(hPipe, &Out, Out.hdr.cbSize, &cbWritten, NULL);
	}
	else if (pIn->hdr.nCmd == CECMD_ACTIVATECON)
	{
		CESERVER_REQ Out;
		ExecutePrepareCmd(&Out, pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_ACTIVATECONSOLE));
		// Activate current console
		_ASSERTE(hConWnd == (HWND)pIn->ActivateCon.hConWnd);
		if (gConEmu.Activate(mp_VCon))
			Out.ActivateCon.hConWnd = hConWnd;
		else
			Out.ActivateCon.hConWnd = NULL;
		fSuccess = WriteFile(hPipe, &Out, Out.hdr.cbSize, &cbWritten, NULL);
	}

    // Освободить память
    if (pIn && (LPVOID)pIn != (LPVOID)&in) {
        free(pIn); pIn = NULL;
    }

	MCHKHEAP;

    //CloseHandle(hPipe);
    return;
}

void CRealConsole::OnServerClosing(DWORD anSrvPID)
{
	if (anSrvPID == mn_ConEmuC_PID && mh_ConEmuC)
	{
		//int nCurProcessCount = m_Processes.size();
		//_ASSERTE(nCurProcessCount <= 1);
		m_ServerClosing.nRecieveTick = GetTickCount();
		m_ServerClosing.hServerProcess = mh_ConEmuC;
		m_ServerClosing.nServerPID = anSrvPID;
	}
	else
	{
		_ASSERTE(anSrvPID == mn_ConEmuC_PID);
	}
}

int CRealConsole::GetProcesses(ConProcess** ppPrc)
{
    if (mn_InRecreate) {
        DWORD dwCurTick = GetTickCount();
        DWORD dwDelta = dwCurTick - mn_InRecreate;
        if (dwDelta > CON_RECREATE_TIMEOUT) {
            mn_InRecreate = 0;
        } else if (ppPrc == NULL) {
            if (mn_InRecreate && !mb_ProcessRestarted && mh_ConEmuC) {
                DWORD dwExitCode = 0;
                if (GetExitCodeProcess(mh_ConEmuC, &dwExitCode) && dwExitCode != STILL_ACTIVE) {
                    RecreateProcessStart();
                }
            }
            return 1; // Чтобы во время Recreate GUI не захлопнулся
        }
    }


    // Если хотят узнать только количество процессов
    if (ppPrc == NULL || mn_ProcessCount == 0) {
        if (ppPrc) *ppPrc = NULL;
        return mn_ProcessCount;
    }

    MSectionLock SPRC; SPRC.Lock(&csPRC);
    
    int dwProcCount = m_Processes.size();

    if (dwProcCount > 0) {
        *ppPrc = (ConProcess*)calloc(dwProcCount, sizeof(ConProcess));
        _ASSERTE((*ppPrc)!=NULL);
        
        for (int i=0; i<dwProcCount; i++) {
            (*ppPrc)[i] = m_Processes[i];
        }
        
    } else {
        *ppPrc = NULL;
    }
    
    SPRC.Unlock();
    return dwProcCount;
}

DWORD CRealConsole::GetProgramStatus()
{
    if (!this)
        return 0;
    return mn_ProgramStatus;
}

DWORD CRealConsole::GetFarStatus()
{
    if (!this)
        return 0;
    if ((mn_ProgramStatus & CES_FARACTIVE) == 0)
        return 0;
    return mn_FarStatus;
}

DWORD CRealConsole::GetServerPID()
{
    if (!this)
        return 0;

	if (mb_InCreateRoot && !mn_ConEmuC_PID) {
		_ASSERTE(!mb_InCreateRoot);
		//if (GetCurrentThreadId() != mn_MonitorThreadID) {
		//	WaitForSingleObject(mh_CreateRootEvent, 30000); -- низя - DeadLock
		//}
	}

    return mn_ConEmuC_PID;
}

DWORD CRealConsole::GetFarPID(BOOL abPluginRequired/*=FALSE*/)
{
    if (!this)
        return 0;

    if (!mn_FarPID // Должен быть известен код PID
    	|| ((mn_ProgramStatus & CES_FARACTIVE) == 0) // выставлен флаг
    	|| ((mn_ProgramStatus & CES_NTVDM) == CES_NTVDM)) // Если активна 16бит программа - значит фар точно не доступен
        return 0;

    return abPluginRequired ? mn_FarPID_PluginDetected : mn_FarPID;
}

// Обновить статус запущенных программ
void CRealConsole::ProcessUpdateFlags(BOOL abProcessChanged)
{
    //Warning: Должен вызываться ТОЛЬКО из ProcessAdd/ProcessDelete, т.к. сам секцию не блокирует

    bool bIsFar=false, bIsTelnet=false, bIsCmd=false;
    DWORD dwPID = 0; //, dwInputTID = 0;
    
    // Наличие 16bit определяем ТОЛЬКО по WinEvent. Иначе не получится отсечь его завершение,
    // т.к. процесс ntvdm.exe не выгружается, а остается в памяти.
    bool bIsNtvdm = (mn_ProgramStatus & CES_NTVDM) == CES_NTVDM;

    std::vector<ConProcess>::reverse_iterator iter = m_Processes.rbegin();
    while (iter!=m_Processes.rend()) {
        // Корневой процесс ConEmuC не учитываем!
        if (iter->ProcessID != mn_ConEmuC_PID) {
            if (!bIsFar && iter->IsFar) bIsFar = true;
            if (!bIsTelnet && iter->IsTelnet) bIsTelnet = true;
            //if (!bIsNtvdm && iter->IsNtvdm) bIsNtvdm = true;
            if (!bIsFar && !bIsCmd && iter->IsCmd) bIsCmd = true;
            // 
			if (!dwPID && iter->IsFar) {
				dwPID = iter->ProcessID;
				//dwInputTID = iter->InputTID;
			}
        }
        iter++;
    }

    TODO("Однако, наверное cmd.exe может быть запущен и в 'фоне'? Например из Update");
    if (bIsCmd && bIsFar) { // Если в консоли запущен cmd.exe - значит (скорее всего?) фар выполняет команду
        bIsFar = false; dwPID = 0;
    }
        
    DWORD nNewProgramStatus = 0;
	if (bIsFar)
		nNewProgramStatus |= CES_FARACTIVE;
	if (bIsFar && bIsNtvdm)
		// 100627 -- считаем, что фар не запускает 16бит программные без cmd (%comspec%)
		bIsNtvdm = false;
	//#ifdef _DEBUG
	//else
	//	nNewProgramStatus = nNewProgramStatus;
	//#endif
    if (bIsTelnet)
		nNewProgramStatus |= CES_TELNETACTIVE;
    if (bIsNtvdm) // определяется выше как "(mn_ProgramStatus & CES_NTVDM) == CES_NTVDM"
        nNewProgramStatus |= CES_NTVDM;
	if (mn_ProgramStatus != nNewProgramStatus)
		mn_ProgramStatus = nNewProgramStatus;

    mn_ProcessCount = m_Processes.size();
    if (mn_FarPID != dwPID)
        AllowSetForegroundWindow(dwPID);
    mn_FarPID = dwPID;
    //TODO("Если сменился FAR - переоткрыть данные для Colorer - CheckColorMapping();");
	//mn_FarInputTID = dwInputTID;

    if (mn_ProcessCount == 0) {
        if (mn_InRecreate == 0) {
            StopSignal();
        } else if (mn_InRecreate == 1) {
            mn_InRecreate = 2;
        }
    }

    // Обновить список процессов в окне настроек и закладки
    if (abProcessChanged) {
        gConEmu.UpdateProcessDisplay(abProcessChanged);
        //2009-09-10
        //gConEmu.mp_TabBar->Refresh(mn_ProgramStatus & CES_FARACTIVE);
        gConEmu.mp_TabBar->Update();
    }
}

void CRealConsole::ProcessUpdate(const DWORD *apPID, UINT anCount)
{
	TODO("OPTIMIZE: хорошо бы от секции вообще избавиться, да и не всегда обновлять нужно...");
    MSectionLock SPRC; SPRC.Lock(&csPRC);

    BOOL lbRecreateOk = FALSE;
    if (mn_InRecreate && mn_ProcessCount == 0) {
        // Раз сюда что-то пришло, значит мы получили пакет, значит консоль запустилась
        lbRecreateOk = TRUE;
    }
    
	_ASSERTE(anCount<40);
	if (anCount>40) anCount = 40;
	DWORD PID[40]; memmove(PID, apPID, anCount*sizeof(DWORD));
    UINT i = 0;
    std::vector<ConProcess>::iterator iter;
    //BOOL bAlive = FALSE;
    BOOL bProcessChanged = FALSE, bProcessNew = FALSE, bProcessDel = FALSE;
    
    // поставить пометочку на все процессы, вдруг кто уже убился 
    iter = m_Processes.begin();
    while (iter != m_Processes.end()) { iter->inConsole = false; iter ++; }
    
    // Проверяем, какие процессы уже есть в нашем списке
    iter=m_Processes.begin();
    while (iter!=m_Processes.end()) {
        for (i = 0; i < anCount; i++) {
            if (PID[i] && PID[i] == iter->ProcessID) {
                iter->inConsole = true;
                PID[i] = 0; // Его добавлять уже не нужно, мы о нем знаем
                break;
            }    
        }
        iter++;
    }
    
    // Проверяем, есть ли изменения
    for (i = 0; i < anCount; i++) {
        if (PID[i]) {
            bProcessNew = TRUE; break;
        }
    }
    iter = m_Processes.begin();
    while (iter != m_Processes.end()) {
        if (iter->inConsole == false) {
            bProcessDel = TRUE; break;
        }
        iter ++;
    }


    

    // Теперь нужно добавить новый процесс
    if (bProcessNew || bProcessDel)
    {
        ConProcess cp;
        HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
        _ASSERTE(h!=INVALID_HANDLE_VALUE);
        if (h==INVALID_HANDLE_VALUE) {
            DWORD dwErr = GetLastError();
            wchar_t szError[255];
            wsprintf(szError, L"Can't create process snapshoot, ErrCode=0x%08X", dwErr);
            gConEmu.DebugStep(szError);
            
        } else {
            //Snapshoot создан, поехали

            // Перед добавлением нового - поставить пометочку на все процессы, вдруг кто уже убился 
            iter = m_Processes.begin();
            while (iter != m_Processes.end()) { iter->Alive = false; iter ++; }

            PROCESSENTRY32 p; memset(&p, 0, sizeof(p)); p.dwSize = sizeof(p);
            if (Process32First(h, &p)) 
            {
                do {
                    // Если он addPID - добавить в m_Processes
                    if (bProcessNew) {
                        for (i = 0; i < anCount; i++) {
                            if (PID[i] && PID[i] == p.th32ProcessID) {
                                if (!bProcessChanged) bProcessChanged = TRUE;
                                memset(&cp, 0, sizeof(cp));
                                cp.ProcessID = PID[i]; cp.ParentPID = p.th32ParentProcessID;
                                ProcessCheckName(cp, p.szExeFile); //far, telnet, cmd, conemuc, и пр.
                                cp.Alive = true;
                                cp.inConsole = true;

                                SPRC.RelockExclusive(300); // Заблокировать, если это еще не сделано
                                m_Processes.push_back(cp);
                            }
                        }
                    }
                    
                    // Перебираем запомненные процессы - поставить флажок Alive
                    // сохранить имя для тех процессов, которым ранее это сделать не удалось
                    iter = m_Processes.begin();
                    while (iter != m_Processes.end())
                    {
                        if (iter->ProcessID == p.th32ProcessID) {
                            iter->Alive = true;
                            if (!iter->NameChecked)
                            {
                                // пометить, что сменился список (определилось имя процесса)
                                if (!bProcessChanged) bProcessChanged = TRUE;
                                //far, telnet, cmd, conemuc, и пр.
                                ProcessCheckName(*iter, p.szExeFile);
                                // запомнить родителя
                                iter->ParentPID = p.th32ParentProcessID;
                            }
                        }
                        iter ++;
                    }
                    
                // Следущий процесс
                } while (Process32Next(h, &p));
            }
            
            // Закрыть shapshoot
            SafeCloseHandle(h);
        }
    }
    
    // Убрать процессы, которых уже нет
    iter = m_Processes.begin();
    while (iter != m_Processes.end())
    {
        if (!iter->Alive || !iter->inConsole) {
            if (!bProcessChanged) bProcessChanged = TRUE;

            SPRC.RelockExclusive(300); // Если уже нами заблокирован - просто вернет FALSE  
            iter = m_Processes.erase(iter);
        } else {
        	//if (mn_Far_PluginInputThreadId && mn_FarPID_PluginDetected
        	//    && iter->ProcessID == mn_FarPID_PluginDetected 
        	//    && iter->InputTID == 0)
        	//    iter->InputTID = mn_Far_PluginInputThreadId;
        	    
            iter ++;
        }
    }
    
    // Обновить статус запущенных программ, получить PID FAR'а, посчитать количество процессов в консоли
    ProcessUpdateFlags(bProcessChanged);

    // 
    if (lbRecreateOk)
        mn_InRecreate = 0;
}

//void CRealConsole::ProcessAdd(DWORD addPID)
//{
//    MSectionLock SPRC; SPRC.Lock(&csPRC);
//
//    BOOL lbRecreateOk = FALSE;
//    if (mn_InRecreate && mn_ProcessCount == 0) {
//        lbRecreateOk = TRUE;
//    }
//    
//    std::vector<ConProcess>::iterator iter;
//    BOOL bAlive = FALSE;
//    BOOL bProcessChanged = FALSE;
//    
//    // может он уже есть в списке?
//    iter=m_Processes.begin();
//    while (iter!=m_Processes.end()) {
//        if (addPID == iter->ProcessID) {
//            addPID = 0;
//            break;
//        }
//        iter++;
//    }
//
//    
//
//    // Теперь нужно добавить новый процесс
//    ConProcess cp;
//    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
//    _ASSERTE(h!=INVALID_HANDLE_VALUE);
//    if (h==INVALID_HANDLE_VALUE) {
//        DWORD dwErr = GetLastError();
//      if (addPID) {
//          // Просто добавить, раз не удалось снять Snapshoot
//          memset(&cp, 0, sizeof(cp));
//          cp.ProcessID = addPID; cp.RetryName = true;
//          wsprintf(cp.Name, L"Can't create snaphoot. ErrCode=0x%08X", dwErr);
//
//          SPRC.RelockExclusive(300);
//          m_Processes.push_back(cp);
//          SPRC.Unlock();
//      }
//        
//    } else {
//        //Snapshoot создан, поехали
//
//        // Перед добавлением нового - поставить пометочку на все процессы, вдруг кто уже убился 
//        iter = m_Processes.begin();
//        while (iter != m_Processes.end()) { iter->Alive = false; iter ++; }
//
//        PROCESSENTRY32 p; memset(&p, 0, sizeof(p)); p.dwSize = sizeof(p);
//        if (Process32First(h, &p)) 
//        {
//            do {
//                // Если он addPID - добавить в m_Processes
//                if (addPID && addPID == p.th32ProcessID) {
//                    if (!bProcessChanged) bProcessChanged = TRUE;
//                    memset(&cp, 0, sizeof(cp));
//                    cp.ProcessID = addPID; cp.ParentPID = p.th32ParentProcessID;
//                    ProcessCheckName(cp, p.szExeFile); //far, telnet, cmd, conemuc, и пр.
//                    cp.Alive = true;
//
//                  SPRC.RelockExclusive(300);
//                    m_Processes.push_back(cp);
//                    break;
//                }
//                
//                // Перебираем запомненные процессы - поставить флажок Alive
//                // сохранить имя для тех процессов, которым ранее это сделать не удалось
//                iter = m_Processes.begin();
//                while (iter != m_Processes.end())
//                {
//                    if (iter->ProcessID == p.th32ProcessID) {
//                        iter->Alive = true;
//                        if (!iter->NameChecked)
//                        {
//                            // пометить, что сменился список (определилось имя процесса)
//                            if (!bProcessChanged) bProcessChanged = TRUE;
//                            //far, telnet, cmd, conemuc, и пр.
//                            ProcessCheckName(*iter, p.szExeFile);
//                            // запомнить родителя
//                            iter->ParentPID = p.th32ParentProcessID;
//                        }
//                    }
//                    iter ++;
//                }
//                
//            // Следущий процесс
//            } while (Process32Next(h, &p));
//        }
//        
//        // Убрать процессы, которых уже нет
//        iter = m_Processes.begin();
//        while (iter != m_Processes.end())
//        {
//            if (!iter->Alive) {
//                if (!bProcessChanged) bProcessChanged = TRUE;
//              SPRC.RelockExclusive(300);
//                iter = m_Processes.erase(iter);
//            } else {
//                iter ++;
//            }
//        }
//        
//        // Закрыть shapshoot
//        SafeCloseHandle(h);
//    }
//    
//    // Обновить статус запущенных программ, получить PID FAR'а, посчитать количество процессов в консоли
//    ProcessUpdateFlags(bProcessChanged);
//
//    // 
//    if (lbRecreateOk)
//        mn_InRecreate = 0;
//}

//void CRealConsole::ProcessDelete(DWORD addPID)
//{
//    MSectionLock SPRC; SPRC.Lock(&csPRC, TRUE);
//
//    BOOL bProcessChanged = FALSE;
//    std::vector<ConProcess>::iterator iter=m_Processes.begin();
//    while (iter!=m_Processes.end()) {
//        if (addPID == iter->ProcessID) {
//            m_Processes.erase(iter);
//            bProcessChanged = TRUE;
//            break;
//        }
//        iter++;
//    }
//
//    // Обновить статус запущенных программ, получить PID FAR'а, посчитать количество процессов в консоли
//    ProcessUpdateFlags(bProcessChanged);
//
//  SPRC.Unlock();
//    
//    if (mn_InRecreate && mn_ProcessCount == 0 && !mb_ProcessRestarted) {
//        RecreateProcessStart();
//    }
//}

void CRealConsole::ProcessCheckName(struct ConProcess &ConPrc, LPWSTR asFullFileName)
{
    wchar_t* pszSlash = _tcsrchr(asFullFileName, _T('\\'));
    if (pszSlash) pszSlash++; else pszSlash=asFullFileName;
    int nLen = _tcslen(pszSlash);
    if (nLen>=63) pszSlash[63]=0;
    lstrcpyW(ConPrc.Name, pszSlash);

    ConPrc.IsFar = lstrcmpi(ConPrc.Name, _T("far.exe"))==0;

    ConPrc.IsNtvdm = lstrcmpi(ConPrc.Name, _T("ntvdm.exe"))==0;

    ConPrc.IsTelnet = lstrcmpi(ConPrc.Name, _T("telnet.exe"))==0;
    
    TODO("Тут главное не промахнуться, и не посчитать корневой conemuc, из которого запущен сам FAR, или который запустил плагин, чтобы GUI прицепился к этой консоли");
    ConPrc.IsCmd = lstrcmpi(ConPrc.Name, _T("cmd.exe"))==0 || lstrcmpi(ConPrc.Name, _T("conemuc.exe"))==0 || lstrcmpi(ConPrc.Name, _T("conemuc64.exe"))==0;

    ConPrc.NameChecked = true;
}

BOOL CRealConsole::WaitConsoleSize(UINT anWaitSize, DWORD nTimeout)
{
	BOOL lbRc = FALSE;
	CESERVER_REQ *pIn = NULL, *pOut = NULL;
	DWORD nStart = GetTickCount();
	DWORD nDelta = 0;
	BOOL lbBufferHeight = FALSE;
	int nNewWidth = 0, nNewHeight = 0;
	if (nTimeout > 10000) nTimeout = 10000;
	if (nTimeout == 0) nTimeout = 100;

	if (GetCurrentThreadId() == mn_MonitorThreadID) {
		_ASSERTE(GetCurrentThreadId() != mn_MonitorThreadID);
		return FALSE;
	}

	#ifdef _DEBUG
	wchar_t szDbg[128]; wsprintf(szDbg, L"CRealConsole::WaitConsoleSize(H=%i, Timeout=%i)\n", anWaitSize, nTimeout);
	DEBUGSTRTABS(szDbg);
	#endif

	WARNING("Вообще, команду в сервер может и не посылать? Сам справится? Просто проверять значения из FileMap");

	while (nDelta < nTimeout)
	{
		if (GetConWindowSize(con.m_sbi, nNewWidth, nNewHeight, &lbBufferHeight)) {
			if (nNewHeight == (int)anWaitSize)
				lbRc = TRUE;
		}

		//pIn = ExecuteNewCmd(CECMD_GETCONSOLEINFO, sizeof(CESERVER_REQ_HDR) + sizeof(DWORD));
		//if (pIn) {
		//	pIn->dwData[0] = anWaitSize;
		//	pOut = ExecuteSrvCmd(mn_ConEmuC_PID, pIn, ghWnd);
		//	if (pOut) {
		//		if (GetConWindowSize(pOut->ConInfo.sbi, nNewWidth, nNewHeight, &lbBufferHeight)) {
		//			if (nNewHeight == (int)anWaitSize)
		//				lbRc = TRUE;
		//		}
		//		
		//	}
		//	ExecuteFreeResult(pIn);
		//	ExecuteFreeResult(pOut);
		//}
		
		if (lbRc) break;

		SetEvent(mh_MonitorThreadEvent);
		Sleep(10);
		nDelta = GetTickCount() - nStart;
	}

	DEBUGSTRTABS(lbRc ? L"CRealConsole::WaitConsoleSize SUCCEEDED\n" : L"CRealConsole::WaitConsoleSize FAILED!!!\n");

	return lbRc;
}

// -- заменено на перехват функции ScreenToClient
//void CRealConsole::RemoveFromCursor()
//{
//	if (!this) return;
//	// 
//	if (gSet.isLockRealConsolePos) return;
//	// Сделано (пока) только чтобы текстовое EMenu активировалось по центру консоли,
//	// а не в положении мыши (что смотрится отвратно - оно может сплющиться до 2-3 строк).
//	// Только при скрытой консоли.
//	if (!isWindowVisible())
//	{  // просто подвинем скрытое окно консоли так, чтобы курсор был ВНЕ него
//		RECT con; POINT ptCur;
//		GetWindowRect(hConWnd, &con);
//		GetCursorPos(&ptCur);
//		short x = ptCur.x + 1;
//		short y = ptCur.y + 1;
//		if (con.left != x || con.top != y)
//			MOVEWINDOW(hConWnd, x, y, con.right - con.left + 1, con.bottom - con.top + 1, TRUE);
//	}
//}

BOOL CRealConsole::GetConWindowSize(const CONSOLE_SCREEN_BUFFER_INFO& sbi, int& nNewWidth, int& nNewHeight, BOOL* pbBufferHeight/*=NULL*/)
{
    // Функция возвращает размер ОКНА, то есть буфер может быть больше
    WARNING("проверить, может там где вызывается GetConWindowSize нужно включить режим BufferHeight?");

    nNewWidth  = sbi.dwSize.X; // sbi.srWindow.Right - sbi.srWindow.Left + 1;
    BOOL lbBufferHeight = con.bBufferHeight;
    // Проверка режимов прокрутки
    if (!lbBufferHeight) {
        if (sbi.dwSize.Y > sbi.dwMaximumWindowSize.Y)
        {
            lbBufferHeight = TRUE; // однозначное включение прокрутки
        }
    }
    if (lbBufferHeight) {
        if (sbi.srWindow.Top == 0
            && sbi.dwSize.Y == (sbi.srWindow.Bottom + 1)
            )
        {
            lbBufferHeight = FALSE; // однозначное вЫключение прокрутки
        }
    }

    // Теперь собственно размеры
    if (!lbBufferHeight) {
        nNewHeight =  sbi.dwSize.Y;
    } else {
        // Это может прийти во время смены размера
        if ((sbi.srWindow.Bottom - sbi.srWindow.Top + 1) < MIN_CON_HEIGHT)
            nNewHeight = con.nTextHeight;
        else
            nNewHeight = sbi.srWindow.Bottom - sbi.srWindow.Top + 1;
    }
    WARNING("Здесь нужно выполнить коррекцию, если nNewHeight велико - включить режим BufferHeight");

	if (pbBufferHeight)
		*pbBufferHeight = lbBufferHeight;

    _ASSERTE(nNewWidth>=MIN_CON_WIDTH && nNewHeight>=MIN_CON_HEIGHT);

    if (!nNewWidth || !nNewHeight) {
        Assert(nNewWidth && nNewHeight);
        return FALSE;
    }

    //if (nNewWidth < sbi.dwSize.X)
    //    nNewWidth = sbi.dwSize.X;

    return TRUE;
}

BOOL CRealConsole::InitBuffers(DWORD OneBufferSize)
{
    // Эта функция должна вызываться только в MonitorThread.
    // Тогда блокировка буфера не потребуется
    #ifdef _DEBUG
    DWORD dwCurThId = GetCurrentThreadId();
    #endif
    _ASSERTE(mn_MonitorThreadID==0 || dwCurThId==mn_MonitorThreadID);

    BOOL lbRc = FALSE;
    int nNewWidth = 0, nNewHeight = 0;

	MCHKHEAP;

    if (!GetConWindowSize(con.m_sbi, nNewWidth, nNewHeight))
        return FALSE;

    if (OneBufferSize) {
        if ((nNewWidth * nNewHeight * sizeof(*con.pConChar)) != OneBufferSize) {
            //// Это может случиться во время ресайза
            //nNewWidth = nNewWidth;
            _ASSERTE((nNewWidth * nNewHeight * sizeof(*con.pConChar)) == OneBufferSize);
		} else if (con.nTextWidth == nNewWidth && con.nTextHeight == nNewHeight) {
			// Не будем зря передергивать буферы и прочее
			if (con.pConChar!=NULL && con.pConAttr!=NULL && con.pDataCmp!=NULL) {
				return TRUE;
			}
		}
        //if ((nNewWidth * nNewHeight * sizeof(*con.pConChar)) != OneBufferSize)
        //    return FALSE;
    }

    // Если требуется увеличить или создать (первично) буфера
    if (!con.pConChar || (con.nTextWidth*con.nTextHeight) < (nNewWidth*nNewHeight))
    {
        MSectionLock sc; sc.Lock(&csCON, TRUE);

        MCHKHEAP;

        if (con.pConChar)
            { Free(con.pConChar); con.pConChar = NULL; }
        if (con.pConAttr)
            { Free(con.pConAttr); con.pConAttr = NULL; }
		if (con.pDataCmp)
			{ Free(con.pDataCmp); con.pDataCmp = NULL; }
		//if (con.pCmp)
		//	{ Free(con.pCmp); con.pCmp = NULL; }

        MCHKHEAP;

        con.pConChar = (TCHAR*)Alloc((nNewWidth * nNewHeight * 2), sizeof(*con.pConChar));
        con.pConAttr = (WORD*)Alloc((nNewWidth * nNewHeight * 2), sizeof(*con.pConAttr));
		con.pDataCmp = (CHAR_INFO*)Alloc((nNewWidth * nNewHeight)*sizeof(CHAR_INFO),1);
		//con.pCmp = (CESERVER_REQ_CONINFO_DATA*)Alloc((nNewWidth * nNewHeight)*sizeof(CHAR_INFO)+sizeof(CESERVER_REQ_CONINFO_DATA),1);

        sc.Unlock();

        _ASSERTE(con.pConChar!=NULL);
        _ASSERTE(con.pConAttr!=NULL);
		_ASSERTE(con.pDataCmp!=NULL);
		//_ASSERTE(con.pCmp!=NULL);

        MCHKHEAP

        lbRc = con.pConChar!=NULL && con.pConAttr!=NULL && con.pDataCmp!=NULL;
    } else if (con.nTextWidth!=nNewWidth || con.nTextHeight!=nNewHeight) {
        MCHKHEAP
        MSectionLock sc; sc.Lock(&csCON);
        memset(con.pConChar, 0, (nNewWidth * nNewHeight * 2) * sizeof(*con.pConChar));
        memset(con.pConAttr, 0, (nNewWidth * nNewHeight * 2) * sizeof(*con.pConAttr));
		memset(con.pDataCmp, 0, (nNewWidth * nNewHeight) * sizeof(CHAR_INFO));
		//memset(con.pCmp->Buf, 0, (nNewWidth * nNewHeight) * sizeof(CHAR_INFO));
        sc.Unlock();
        MCHKHEAP

        lbRc = TRUE;
    } else {
        lbRc = TRUE;
    }
    MCHKHEAP

    con.nTextWidth = nNewWidth;
    con.nTextHeight = nNewHeight;

    // чтобы передернулись положения панелей и прочие флаги
	mb_DataChanged = TRUE;

    //InitDC(false,true);

    return lbRc;
}

void CRealConsole::ShowConsole(int nMode) // -1 Toggle, 0 - Hide, 1 - Show
{
    if (this == NULL) return;
    if (!hConWnd) return;
    
    if (nMode == -1) {
        nMode = IsWindowVisible(hConWnd) ? 0 : 1;
    }
    
    if (nMode == 1)
    {
        isShowConsole = true;
        //apiShowWindow(hConWnd, SW_SHOWNORMAL);
        //if (setParent) SetParent(hConWnd, 0);
        RECT rcCon, rcWnd; GetWindowRect(hConWnd, &rcCon); GetWindowRect(ghWnd, &rcWnd);
        //if (!IsDebuggerPresent())
		TODO("Скорректировать позицию так, чтобы не вылезло за экран");
        if (SetOtherWindowPos(hConWnd, HWND_TOPMOST, 
            rcWnd.right-rcCon.right+rcCon.left, rcWnd.bottom-rcCon.bottom+rcCon.top,
            0,0, SWP_NOSIZE|SWP_SHOWWINDOW))
		{
			if (!IsWindowEnabled(hConWnd))
				EnableWindow(hConWnd, true); // Для админской консоли - не сработает.
			DWORD dwExStyle = GetWindowLong(hConWnd, GWL_EXSTYLE);
			#if 0
			DWORD dw1, dwErr;
			if ((dwExStyle & WS_EX_TOPMOST) == 0) {
				dw1 = SetWindowLong(hConWnd, GWL_EXSTYLE, dwExStyle|WS_EX_TOPMOST);
				dwErr = GetLastError();
				dwExStyle = GetWindowLong(hConWnd, GWL_EXSTYLE);
				if ((dwExStyle & WS_EX_TOPMOST) == 0) {
					SetOtherWindowPos(hConWnd, HWND_TOPMOST, 
						0,0,0,0, SWP_NOSIZE|SWP_NOMOVE);
				}
			}
			#endif
			// Issue 246. Возвращать фокус в ConEmu можно только если удалось установить
			// "OnTop" для RealConsole, иначе - RealConsole "всплывет" на заднем плане
			if ((dwExStyle & WS_EX_TOPMOST))
				SetFocus(ghWnd);
		//} else { //2010-06-05 Не требуется. SetOtherWindowPos выполнит команду в сервере при необходимости
		//	if (isAdministrator() || (m_Args.pszUserName != NULL)) {
		//		// Если оно запущено в Win7 as admin
		//        CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_SHOWCONSOLE, sizeof(CESERVER_REQ_HDR) + sizeof(DWORD));
		//		if (pIn) {
		//			pIn->dwData[0] = SW_SHOWNORMAL;
		//			CESERVER_REQ *pOut = ExecuteSrvCmd(GetServerPID(), pIn, ghWnd);
		//			if (pOut) ExecuteFreeResult(pOut);
		//			ExecuteFreeResult(pIn);
		//		}
		//	}
		}
        //if (setParent) SetParent(hConWnd, 0);
    }
    else
    {
        isShowConsole = false;
		ShowOtherWindow(hConWnd, SW_HIDE);
		////if (!gSet.isConVisible)
		//if (!apiShowWindow(hConWnd, SW_HIDE)) {
		//	if (isAdministrator() || (m_Args.pszUserName != NULL)) {
		//		// Если оно запущено в Win7 as admin
		//        CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_SHOWCONSOLE, sizeof(CESERVER_REQ_HDR) + sizeof(DWORD));
		//		if (pIn) {
		//			pIn->dwData[0] = SW_HIDE;
		//			CESERVER_REQ *pOut = ExecuteSrvCmd(GetServerPID(), pIn, ghWnd);
		//			if (pOut) ExecuteFreeResult(pOut);
		//			ExecuteFreeResult(pIn);
		//		}
		//	}
		//}
		////if (setParent) SetParent(hConWnd, setParent2 ? ghWnd : ghWndDC);
		////if (!gSet.isConVisible)
		////EnableWindow(hConWnd, false); -- наверное не нужно
        SetFocus(ghWnd);
    }
}

//void CRealConsole::CloseMapping()
//{
//	if (pConsoleData) {
//		UnmapViewOfFile(pConsoleData);
//		pConsoleData = NULL;
//	}
//	if (hFileMapping) {
//		CloseHandle(hFileMapping);
//		hFileMapping = NULL;
//	}
//}

void CRealConsole::OnServerStarted()
{
	//if (!mp_ConsoleInfo)
	if (!m_ConsoleMap.IsValid()) {
		// Инициализировать имена пайпов, событий, мэппингов и т.п.
		InitNames();
	
		// Открыть map с данными, теперь он уже должен быть создан
		OpenMapHeader();
	}

	// И атрибуты Colorer
	OpenColorMapping();

	// Возвращается через CESERVER_REQ_STARTSTOPRET
	//if ((gSet.isMonitorConsoleLang & 2) == 2) // Один Layout на все консоли
	//	SwitchKeyboardLayout(INPUTLANGCHANGE_SYSCHARSET,gConEmu.GetActiveKeyboardLayout());
}

void CRealConsole::SetHwnd(HWND ahConWnd)
{
	// Окно разрушено? Пересоздание консоли?
	if (hConWnd && !IsWindow(hConWnd)) {
		_ASSERTE(IsWindow(hConWnd));
		hConWnd = NULL;
	}

	// Мог быть уже вызван (AttachGui/ConsoleEvent/CMD_START)
	if (hConWnd != NULL) {
		if (hConWnd != ahConWnd) {
			if (m_ConsoleMap.IsValid()) {
				_ASSERTE(!m_ConsoleMap.IsValid());
			
				//CloseMapHeader(); // вдруг был подцеплен к другому окну? хотя не должен
				// OpenMapHeader() пока не зовем, т.к. map мог быть еще не создан
			}

			Assert(hConWnd == ahConWnd);
			return;
		}
	}

    hConWnd = ahConWnd;
    //if (mb_Detached && ahConWnd) // Не сбрасываем, а то нить может не успеть!
    //  mb_Detached = FALSE; // Сброс флажка, мы уже подключились
    
    //OpenColorMapping();
        
    mb_ProcessRestarted = FALSE; // Консоль запущена
    mb_InCloseConsole = FALSE;
	ZeroStruct(m_ServerClosing);

    if (ms_VConServer_Pipe[0] == 0) {
        wchar_t szEvent[64];

		if (!mh_GuiAttached) {
			wsprintfW(szEvent, CEGUIRCONSTARTED, (DWORD)hConWnd);
			//// Скорее всего событие в сервере еще не создано
			//mh_GuiAttached = OpenEvent(EVENT_MODIFY_STATE, FALSE, ms_VConServer_Pipe);
			//// Вроде, когда используется run as administrator - event открыть не получается?
			//if (!mh_GuiAttached) {
    		mh_GuiAttached = CreateEvent(gpNullSecurity, TRUE, FALSE, szEvent);
    		_ASSERTE(mh_GuiAttached!=NULL);
			//}
		}

        // Запустить серверный пайп
        wsprintf(ms_VConServer_Pipe, CEGUIPIPENAME, L".", (DWORD)hConWnd); //был mn_ConEmuC_PID
		if (!mh_ServerSemaphore)
			mh_ServerSemaphore = CreateSemaphore(NULL, 1, 1, NULL);

        for (int i=0; i<MAX_SERVER_THREADS; i++) {
			if (mh_RConServerThreads[i])
				continue;
            mn_RConServerThreadsId[i] = 0;
            mh_RConServerThreads[i] = CreateThread(NULL, 0, RConServerThread, (LPVOID)this, 0, &mn_RConServerThreadsId[i]);
            _ASSERTE(mh_RConServerThreads[i]!=NULL);
        }

        // чтобы ConEmuC знал, что мы готовы
	   //     if (mh_GuiAttached) {
	   //     	SetEvent(mh_GuiAttached);
				//Sleep(10);
	   //     	SafeCloseHandle(mh_GuiAttached);
	   //		}
    }

    if (gSet.isConVisible)
        ShowConsole(1); // установить консольному окну флаг AlwaysOnTop
    //else if (isAdministrator())
    //	ShowConsole(0); // В Win7 оно таки появляется видимым - проверка вынесена в ConEmuC

	// Перенесено в OnServerStarted
    //if ((gSet.isMonitorConsoleLang & 2) == 2) // Один Layout на все консоли
    //    SwitchKeyboardLayout(INPUTLANGCHANGE_SYSCHARSET,gConEmu.GetActiveKeyboardLayout());

    if (isActive()) {
        ghConWnd = hConWnd;
        // Чтобы можно было найти хэндл окна по хэндлу консоли
        SetWindowLongPtr(ghWnd, GWLP_USERDATA, (LONG_PTR)hConWnd);
    }
}

void CRealConsole::OnFocus(BOOL abFocused)
{
    if (!this) return;

    if ((mn_Focused == -1) ||
        ((mn_Focused == 0) && abFocused) ||
        ((mn_Focused == 1) && !abFocused))
    {
#ifdef _DEBUG
        if (abFocused) {
            DEBUGSTRINPUT(L"--Get focus\n")
        } else {
            DEBUGSTRINPUT(L"--Loose focus\n")
        }
#endif
		// Сразу, иначе по окончании PostConsoleEvent RCon может разрушиться?
		mn_Focused = abFocused ? 1 : 0;

        INPUT_RECORD r = {FOCUS_EVENT};
        r.Event.FocusEvent.bSetFocus = abFocused;
        PostConsoleEvent(&r);

    }
}

void CRealConsole::CreateLogFiles()
{
    if (!m_UseLogs || mh_LogInput) return; // уже

    DWORD dwErr = 0;
    wchar_t szFile[MAX_PATH+64], *pszDot;
    _ASSERTE(gConEmu.ms_ConEmuExe[0]);
    lstrcpyW(szFile, gConEmu.ms_ConEmuExe);
    if ((pszDot = wcsrchr(szFile, L'\\')) == NULL) {
        DisplayLastError(L"wcsrchr failed!");
        return; // ошибка
    }
    *pszDot = 0;

    //mpsz_LogPackets = (wchar_t*)calloc(pszDot - szFile + 64, 2);
    //lstrcpyW(mpsz_LogPackets, szFile);
    //wsprintf(mpsz_LogPackets+(pszDot-szFile), L"\\ConEmu-recv-%i-%%i.con", mn_ConEmuC_PID); // ConEmu-recv-<ConEmuC_PID>-<index>.con

    wsprintfW(pszDot, L"\\ConEmu-input-%i.log", mn_ConEmuC_PID);
    

    mh_LogInput = CreateFileW ( szFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (mh_LogInput == INVALID_HANDLE_VALUE) {
        mh_LogInput = NULL;
        dwErr = GetLastError();
		wchar_t szError[MAX_PATH*2];
        wsprintf(szError, L"Create log file failed! ErrCode=0x%08X\n%s\n", dwErr, szFile);
		MBoxA(szError);
        return;
    }

    mpsz_LogInputFile = _wcsdup(szFile);
    // OK, лог создали
}

void CRealConsole::LogString(LPCWSTR asText, BOOL abShowTime /*= FALSE*/)
{
    if (!this) return;
    if (!asText || !mh_LogInput) return;
    char chAnsi[255];
    WideCharToMultiByte(CP_ACP, 0, asText, -1, chAnsi, 254, 0,0);
    chAnsi[254] = 0;
    LogString(chAnsi, abShowTime);
}

void CRealConsole::LogString(LPCSTR asText, BOOL abShowTime /*= FALSE*/)
{
    if (!this) return;
    if (!asText) return;
	if (mh_LogInput)
	{
		DWORD dwLen;

		if (abShowTime)
		{
			SYSTEMTIME st; GetLocalTime(&st);
			char szTime[32];
			wsprintfA(szTime, "%i:%02i:%02i.%03i ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
			dwLen = strlen(szTime);
			WriteFile(mh_LogInput, szTime, dwLen, &dwLen, 0);
		}

		if ((dwLen = strlen(asText))>0)
			WriteFile(mh_LogInput, asText, dwLen, &dwLen, 0);

		WriteFile(mh_LogInput, "\r\n", 2, &dwLen, 0);

		FlushFileBuffers(mh_LogInput);
	}
	else
	{
		#ifdef _DEBUG
			DEBUGSTRLOG(asText); DEBUGSTRLOG("\n");
		#endif
	}
}

void CRealConsole::LogInput(INPUT_RECORD* pRec)
{
    if (!mh_LogInput || m_UseLogs<2) return;

    char szInfo[192] = {0};

    SYSTEMTIME st; GetLocalTime(&st);
    wsprintfA(szInfo, "%i:%02i:%02i.%03i ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    char *pszAdd = szInfo+strlen(szInfo);

    switch (pRec->EventType) {
    /*case FOCUS_EVENT: wsprintfA(pszAdd, "FOCUS_EVENT(%i)\r\n", pRec->Event.FocusEvent.bSetFocus);
        break;*/
    case MOUSE_EVENT: wsprintfA(pszAdd, "MOUSE_EVENT\r\n");
        {
            wsprintfA(pszAdd, "Mouse: {%ix%i} Btns:{", pRec->Event.MouseEvent.dwMousePosition.X, pRec->Event.MouseEvent.dwMousePosition.Y);
            pszAdd += strlen(pszAdd);
            if (pRec->Event.MouseEvent.dwButtonState & 1) strcat(pszAdd, "L");
            if (pRec->Event.MouseEvent.dwButtonState & 2) strcat(pszAdd, "R");
            if (pRec->Event.MouseEvent.dwButtonState & 4) strcat(pszAdd, "M1");
            if (pRec->Event.MouseEvent.dwButtonState & 8) strcat(pszAdd, "M2");
            if (pRec->Event.MouseEvent.dwButtonState & 0x10) strcat(pszAdd, "M3");
            pszAdd += strlen(pszAdd);
            if (pRec->Event.MouseEvent.dwButtonState & 0xFFFF0000)
            	wsprintfA(pszAdd, "x%04X", (pRec->Event.MouseEvent.dwButtonState & 0xFFFF0000)>>16);
            strcat(pszAdd, "} "); pszAdd += strlen(pszAdd);
            wsprintfA(pszAdd, "KeyState: 0x%08X ", pRec->Event.MouseEvent.dwControlKeyState);
            if (pRec->Event.MouseEvent.dwEventFlags & 0x01) strcat(pszAdd, "|MOUSE_MOVED");
            if (pRec->Event.MouseEvent.dwEventFlags & 0x02) strcat(pszAdd, "|DOUBLE_CLICK");
            if (pRec->Event.MouseEvent.dwEventFlags & 0x04) strcat(pszAdd, "|MOUSE_WHEELED");
            if (pRec->Event.MouseEvent.dwEventFlags & 0x08) strcat(pszAdd, "|MOUSE_HWHEELED");
            strcat(pszAdd, "\r\n");
            
        } break;
    case KEY_EVENT:
        {
            char chAcp; WideCharToMultiByte(CP_ACP, 0, &pRec->Event.KeyEvent.uChar.UnicodeChar, 1, &chAcp, 1, 0,0);
            /* */ wsprintfA(pszAdd, "%c %s count=%i, VK=%i, SC=%i, CH=%i, State=0x%08x %s\r\n",
                chAcp ? chAcp : ' ',
                pRec->Event.KeyEvent.bKeyDown ? "Down," : "Up,  ",
                pRec->Event.KeyEvent.wRepeatCount,
                pRec->Event.KeyEvent.wVirtualKeyCode,
                pRec->Event.KeyEvent.wVirtualScanCode,
                pRec->Event.KeyEvent.uChar.UnicodeChar,
                pRec->Event.KeyEvent.dwControlKeyState,
                (pRec->Event.KeyEvent.dwControlKeyState & ENHANCED_KEY) ?
                "<Enhanced>" : "");
        } break;
    }

    if (*pszAdd) {
        DWORD dwLen = 0;
        WriteFile(mh_LogInput, szInfo, strlen(szInfo), &dwLen, 0);
        FlushFileBuffers(mh_LogInput);
    }
}

void CRealConsole::CloseLogFiles()
{
    SafeCloseHandle(mh_LogInput);
    if (mpsz_LogInputFile) {
        //DeleteFile(mpsz_LogInputFile);
        free(mpsz_LogInputFile); mpsz_LogInputFile = NULL;
    }
    //if (mpsz_LogPackets) {
    //    //wchar_t szMask[MAX_PATH*2]; wcscpy(szMask, mpsz_LogPackets);
    //    //wchar_t *psz = wcsrchr(szMask, L'%');
    //    //if (psz) {
    //    //    wcscpy(psz, L"*.*");
    //    //    psz = wcsrchr(szMask, L'\\'); 
    //    //    if (psz) {
    //    //        psz++;
    //    //        WIN32_FIND_DATA fnd;
    //    //        HANDLE hFind = FindFirstFile(szMask, &fnd);
    //    //        if (hFind != INVALID_HANDLE_VALUE) {
    //    //            do {
    //    //                if ((fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0) {
    //    //                    wcscpy(psz, fnd.cFileName);
    //    //                    DeleteFile(szMask);
    //    //                }
    //    //            } while (FindNextFile(hFind, &fnd));
    //    //            FindClose(hFind);
    //    //        }
    //    //    }
    //    //}
    //    free(mpsz_LogPackets); mpsz_LogPackets = NULL;
    //}
}

//void CRealConsole::LogPacket(CESERVER_REQ* pInfo)
//{
//    if (!mpsz_LogPackets || m_UseLogs<3) return;
//
//    wchar_t szPath[MAX_PATH];
//    wsprintf(szPath, mpsz_LogPackets, ++mn_LogPackets);
//
//    HANDLE hFile = CreateFile(szPath, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
//    if (hFile != INVALID_HANDLE_VALUE) {
//        DWORD dwSize = 0;
//        WriteFile(hFile, pInfo, pInfo->hdr.cbSize, &dwSize, 0);
//        CloseHandle(hFile);
//    }
//}



// Послать в консоль запрос на закрытие
BOOL CRealConsole::RecreateProcess(RConStartArgs *args)
{
    if (!this)
        return false;
        
    _ASSERTE(hConWnd && mh_ConEmuC);

    if (!hConWnd || !mh_ConEmuC) {
        Box(_T("Console was not created (CRealConsole::SetConsoleSize)"));
        return false; // консоль пока не создана?
    }
    if (mn_InRecreate) {
        Box(_T("Console already in recreate..."));
        return false;
    }
    
    if (args->pszSpecialCmd && *args->pszSpecialCmd) {
        if (m_Args.pszSpecialCmd) Free(m_Args.pszSpecialCmd);
        int nLen = lstrlenW(args->pszSpecialCmd);
        m_Args.pszSpecialCmd = (wchar_t*)Alloc(nLen+1,2);
        if (!m_Args.pszSpecialCmd) {
            Box(_T("Can't allocate memory..."));
            return FALSE;
        }
        lstrcpyW(m_Args.pszSpecialCmd, args->pszSpecialCmd);
    }
    if (args->pszStartupDir) {
		if (m_Args.pszStartupDir) Free(m_Args.pszStartupDir);
        int nLen = lstrlenW(args->pszStartupDir);
        m_Args.pszStartupDir = (wchar_t*)Alloc(nLen+1,2);
        if (!m_Args.pszStartupDir)
            return FALSE;
        lstrcpyW(m_Args.pszStartupDir, args->pszStartupDir);
    }

	m_Args.bRunAsAdministrator = args->bRunAsAdministrator;

    //DWORD nWait = 0;

    mb_ProcessRestarted = FALSE;
    mn_InRecreate = GetTickCount();
    if (!mn_InRecreate) {
        DisplayLastError(L"GetTickCount failed");
        return false;
    }

    CloseConsole();

    // mb_InCloseConsole сбросим после того, как появится новое окно!
    //mb_InCloseConsole = FALSE;
    
	SetConStatus(L"Restarting process...");

    return true;
}

// И запустить ее заново
BOOL CRealConsole::RecreateProcessStart()
{
    bool lbRc = false;
    if (mn_InRecreate && mn_ProcessCount == 0 && !mb_ProcessRestarted) {
        mb_ProcessRestarted = TRUE;
        if ((mn_ProgramStatus & CES_NTVDM) == CES_NTVDM) {
        	mn_ProgramStatus = 0; mb_IgnoreCmdStop = FALSE;
        	// При пересоздании сбрасывается 16битный режим, нужно отресайзится
        	if (!PreInit())
        		return FALSE;
        } else {
        	mn_ProgramStatus = 0; mb_IgnoreCmdStop = FALSE;
        }
        
        StopThread(TRUE/*abRecreate*/);
        
        ResetEvent(mh_TermEvent);
        hConWnd = NULL;
        ms_VConServer_Pipe[0] = 0;
        SafeCloseHandle(mh_ServerSemaphore);
        SafeCloseHandle(mh_GuiAttached);

        if (!StartProcess()) {
            mn_InRecreate = 0;
            mb_ProcessRestarted = FALSE;
            
        } else {
            ResetEvent(mh_TermEvent);
            lbRc = StartMonitorThread();
        }
    }
    return lbRc;
}




///* ****************************************** */
///* Поиск диалогов и пометка "прозрачных" мест */
///* ****************************************** */
//
//// Найти правую границу
//bool CRealConsole::FindFrameRight_ByTop(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight)
//{
//	wchar_t wcMostRight = 0;
//	int n;
//	int nShift = nWidth*nFromY;
//
//	wchar_t wc = pChar[nShift+nFromX];
//
//	nMostRight = nFromX;
//
//	if (wc != ucBoxSinglDownRight && wc != ucBoxDblDownRight) {
//		// нетривиальная ситуация - возможно диалог на экране неполный
//		int nMostTop = nFromY;
//		if (FindFrameTop_ByLeft(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nMostTop))
//			nFromY = nMostTop; // этот диалог продолжается сверху
//	}
//
//	if (wc != ucBoxSinglDownRight && wc != ucBoxDblDownRight) {
//		wchar_t c;
//		// Придется попробовать идти просто до границы
//		if (wc == ucBoxSinglVert || wc == ucBoxSinglVertRight) {
//			while (++nMostRight < nWidth) {
//				c = pChar[nShift+nMostRight];
//				if (c == ucBoxSinglVert || c == ucBoxSinglVertLeft) {
//					nMostRight++; break;
//				}
//			}
//
//		} else if (wc == ucBoxDblDownRight) {
//			while (++nMostRight < nWidth) {
//				c = pChar[nShift+nMostRight];
//				if (c == ucBoxDblVert || c == ucBoxDblVertLeft || c == ucBoxDblVertSinglLeft) {
//					nMostRight++; break;
//				}
//			}
//
//		}
//
//	} else {
//		if (wc == ucBoxSinglDownRight) {
//			wcMostRight = ucBoxSinglDownLeft;
//		} else if (wc == ucBoxDblDownRight) {
//			wcMostRight = ucBoxDblDownLeft;
//		}
//
//		// Найти правую границу
//		while (++nMostRight < nWidth) {
//			n = nShift+nMostRight;
//			//if (pAttr[n].crBackColor != nBackColor)
//			//	break; // конец цвета фона диалога
//			if (pChar[n] == wcMostRight) {
//				nMostRight++;
//				break; // закрывающая угловая рамка
//			}
//		}
//	}
//
//	nMostRight--;
//	_ASSERTE(nMostRight<nWidth);
//	return (nMostRight > nFromX);
//}
//
//// Найти левуу границу
//bool CRealConsole::FindFrameLeft_ByTop(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostLeft)
//{
//	wchar_t wcMostLeft;
//	int n;
//	int nShift = nWidth*nFromY;
//
//	wchar_t wc = pChar[nShift+nFromX];
//
//	nMostLeft = nFromX;
//
//	if (wc != ucBoxSinglDownLeft && wc != ucBoxDblDownLeft) {
//		// нетривиальная ситуация - возможно диалог на экране неполный
//		int nMostTop = nFromY;
//		if (FindFrameTop_ByRight(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nMostTop))
//			nFromY = nMostTop; // этот диалог продолжается сверху
//	}
//
//	if (wc != ucBoxSinglDownLeft && wc != ucBoxDblDownLeft) {
//		wchar_t c;
//		// Придется попробовать идти просто до границы
//		if (wc == ucBoxSinglVert || wc == ucBoxSinglVertLeft) {
//			while (--nMostLeft >= 0) {
//				c = pChar[nShift+nMostLeft];
//				if (c == ucBoxSinglVert || c == ucBoxSinglVertRight) {
//					nMostLeft--; break;
//				}
//			}
//
//		} else if (wc == ucBoxDblDownRight) {
//			while (--nMostLeft >= 0) {
//				c = pChar[nShift+nMostLeft];
//				if (c == ucBoxDblVert || c == ucBoxDblVertLeft || c == ucBoxDblVertSinglLeft) {
//					nMostLeft--; break;
//				}
//			}
//
//		}
//
//	} else {
//		if (wc == ucBoxSinglDownLeft) {
//			wcMostLeft = ucBoxSinglDownRight;
//		} else if (wc == ucBoxDblDownLeft) {
//			wcMostLeft = ucBoxDblDownRight;
//		} else {
//			_ASSERTE(wc == ucBoxSinglDownLeft || wc == ucBoxDblDownLeft);
//			return false;
//		}
//
//		// Найти левую границу
//		while (--nMostLeft >= 0) {
//			n = nShift+nMostLeft;
//			//if (pAttr[n].crBackColor != nBackColor)
//			//	break; // конец цвета фона диалога
//			if (pChar[n] == wcMostLeft) {
//				nMostLeft--;
//				break; // закрывающая угловая рамка
//			}
//		}
//	}
//
//	nMostLeft++;
//	_ASSERTE(nMostLeft>=0);
//	return (nMostLeft < nFromX);
//}
//
//bool CRealConsole::FindFrameRight_ByBottom(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight)
//{
//	wchar_t wcMostRight;
//	int n;
//	int nShift = nWidth*nFromY;
//
//	wchar_t wc = pChar[nShift+nFromX];
//
//	nMostRight = nFromX;
//
//
//	if (wc == ucBoxSinglUpRight || wc == ucBoxSinglHorz || wc == ucBoxSinglUpHorz) {
//		wcMostRight = ucBoxSinglUpLeft;
//	} else if (wc == ucBoxDblUpRight || wc == ucBoxSinglUpDblHorz || wc == ucBoxDblHorz) {
//		wcMostRight = ucBoxDblUpLeft;
//	} else {
//		return false; // найти правый нижний угол мы не сможем
//	}
//
//	// Найти правую границу
//	while (++nMostRight < nWidth) {
//		n = nShift+nMostRight;
//		//if (pAttr[n].crBackColor != nBackColor)
//		//	break; // конец цвета фона диалога
//		if (pChar[n] == wcMostRight) {
//			nMostRight++;
//			break; // закрывающая угловая рамка
//		}
//	}
//
//	nMostRight--;
//	_ASSERTE(nMostRight<nWidth);
//	return (nMostRight > nFromX);
//}
//
//bool CRealConsole::FindFrameLeft_ByBottom(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostLeft)
//{
//	wchar_t wcMostLeft;
//	int n;
//	int nShift = nWidth*nFromY;
//
//	wchar_t wc = pChar[nShift+nFromX];
//
//	nMostLeft = nFromX;
//
//
//	if (wc == ucBoxSinglUpLeft || wc == ucBoxSinglHorz || wc == ucBoxSinglUpHorz) {
//		wcMostLeft = ucBoxSinglUpRight;
//	} else if (wc == ucBoxDblUpLeft || wc == ucBoxSinglUpDblHorz || wc == ucBoxDblHorz) {
//		wcMostLeft = ucBoxDblUpRight;
//	} else {
//		return false; // найти левый нижний угол мы не сможем
//	}
//
//	// Найти левую границу
//	while (--nMostLeft >= 0) {
//		n = nShift+nMostLeft;
//		//if (pAttr[n].crBackColor != nBackColor)
//		//	break; // конец цвета фона диалога
//		if (pChar[n] == wcMostLeft) {
//			nMostLeft--;
//			break; // закрывающая угловая рамка
//		}
//	}
//
//	nMostLeft++;
//	_ASSERTE(nMostLeft>=0);
//	return (nMostLeft < nFromX);
//}
//
//// Диалог без окантовки. Все просто - идем по рамке
//bool CRealConsole::FindDialog_TopLeft(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, BOOL &bMarkBorder)
//{
//	bMarkBorder = TRUE;
//	int nShift = nWidth*nFromY;
//	int nMostRightBottom;
//
//	// Найти правую границу по верху
//	nMostRight = nFromX;
//	FindFrameRight_ByTop(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nMostRight);
//	_ASSERTE(nMostRight<nWidth);
//
//	// Найти нижнюю границу
//	nMostBottom = nFromY;
//	FindFrameBottom_ByLeft(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nMostBottom);
//	_ASSERTE(nMostBottom<nHeight);
//
//	// Найти нижнюю границу по правой стороне
//	nMostRightBottom = nFromY;
//	if (FindFrameBottom_ByRight(pChar, pAttr, nWidth, nHeight, nMostRight, nFromY, nMostRightBottom)) {
//		_ASSERTE(nMostRightBottom<nHeight);
//		// Результатом считаем - наиБОЛЬШУЮ высоту
//		if (nMostRightBottom > nMostBottom)
//			nMostBottom = nMostRightBottom;
//	}
//
//	return true;
//}
//
//// Диалог без окантовки. Все просто - идем по рамке
//bool CRealConsole::FindDialog_TopRight(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, BOOL &bMarkBorder)
//{
//	bMarkBorder = TRUE;
//	int nShift = nWidth*nFromY;
//	int nX;
//
//	nMostRight = nFromX;
//	nMostBottom = nFromY;
//
//	// Найти левую границу по верху
//	if (FindFrameLeft_ByTop(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nX)) {
//		_ASSERTE(nX>=0);
//		nFromX = nX;
//	}
//
//	// Найти нижнюю границу
//	nMostBottom = nFromY;
//	FindFrameBottom_ByRight(pChar, pAttr, nWidth, nHeight, nMostRight, nFromY, nMostBottom);
//	_ASSERTE(nMostBottom<nHeight);
//
//	// найти левую границу по низу
//	if (FindFrameLeft_ByBottom(pChar, pAttr, nWidth, nHeight, nMostRight, nMostBottom, nX)) {
//		_ASSERTE(nX>=0);
//		if (nFromX > nX) nFromX = nX;
//	}
//
//	return true;
//}
//
//// Это может быть нижний кусок диалога
//// левая граница рамки
//// Диалог без окантовки, но начинается не с угла
//bool CRealConsole::FindDialog_Left(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, BOOL &bMarkBorder)
//{
//	bMarkBorder = TRUE;
//	//nBackColor = pAttr[nFromX+nWidth*nFromY].crBackColor; // но на всякий случай сохраним цвет фона для сравнения
//	wchar_t wcMostRight, wcMostBottom, wcMostRightBottom, wcMostTop, wcNotMostBottom1, wcNotMostBottom2;
//	int nShift = nWidth*nFromY;
//	int nMostTop, nY, nX;
//
//	wchar_t wc = pChar[nShift+nFromX];
//
//	if (wc == ucBoxSinglVert || wc == ucBoxSinglVertRight) {
//		wcMostRight = ucBoxSinglUpLeft; wcMostBottom = ucBoxSinglUpRight; wcMostRightBottom = ucBoxSinglUpLeft; wcMostTop = ucBoxSinglDownLeft;
//		// наткнулись на вертикальную линию на панели
//		if (wc == ucBoxSinglVert) {
//			wcNotMostBottom1 = ucBoxSinglUpHorz; wcNotMostBottom2 = ucBoxSinglUpDblHorz;
//			nMostBottom = nFromY;
//			while (++nMostBottom < nHeight) {
//				wc = pChar[nFromX+nMostBottom*nWidth];
//				if (wc == wcNotMostBottom1 || wc == wcNotMostBottom2)
//					return false;
//			}
//		}
//	} else {
//		wcMostRight = ucBoxDblUpLeft; wcMostBottom = ucBoxDblUpRight; wcMostRightBottom = ucBoxDblUpLeft; wcMostTop = ucBoxDblDownLeft;
//	}
//
//	// попытаться подняться вверх, может угол все-таки есть?
//	if (FindFrameTop_ByLeft(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nY)) {
//		_ASSERTE(nY >= 0);
//		nFromY = nY;
//	}
//
//	// Найти нижнюю границу
//	nMostBottom = nFromY;
//	FindFrameBottom_ByLeft(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nMostBottom);
//	_ASSERTE(nMostBottom<nHeight);
//
//	// Найти правую границу по верху
//	nMostRight = nFromX;
//	if (FindFrameRight_ByTop(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nX)) {
//		_ASSERTE(nX<nWidth);
//		nMostRight = nX;
//	}
//	// попробовать найти правую границу по низу
//	if (FindFrameRight_ByBottom(pChar, pAttr, nWidth, nHeight, nFromX, nMostBottom, nX)) {
//		_ASSERTE(nX<nWidth);
//		if (nX > nMostRight) nMostRight = nX;
//	}
//	_ASSERTE(nMostRight>=0);
//
//	// Попытаться подняться вверх по правой границе?
//	if (FindFrameTop_ByRight(pChar, pAttr, nWidth, nHeight, nMostRight, nFromY, nMostTop)) {
//		_ASSERTE(nMostTop>=0);
//		nFromY = nMostTop;
//	}
//
//	return true;
//}
//
//bool CRealConsole::FindFrameTop_ByLeft(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostTop)
//{
//	wchar_t c;
//	// Попытаемся подняться вверх вдоль правой рамки до угла
//	int nY = nFromY;
//	while (nY > 0)
//	{
//		c = pChar[(nY-1)*nWidth+nFromX];
//		if (c == ucBoxDblDownRight || c == ucBoxSinglDownRight // двойной и одинарный угол (левый верхний)
//			|| c == ucBoxDblVertRight || c == ucBoxDblVertSinglRight || c == ucBoxSinglVertRight
//			|| c == ucBoxDblVert || c == ucBoxSinglVert
//			) // пересечение (правая граница)
//		{
//			nY--; continue;
//		}
//		// Другие символы недопустимы
//		break;
//	}
//	_ASSERTE(nY >= 0);
//	nMostTop = nY;
//	return (nMostTop < nFromY);
//}
//
//bool CRealConsole::FindFrameTop_ByRight(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostTop)
//{
//	wchar_t c;
//	// Попытаемся подняться вверх вдоль правой рамки до угла
//	int nY = nFromY;
//	while (nY > 0)
//	{
//		c = pChar[(nY-1)*nWidth+nFromX];
//		if (c == ucBoxDblDownLeft || c == ucBoxSinglDownLeft // двойной и одинарный угол (правый верхний)
//			|| c == ucBoxDblVertLeft || c == ucBoxDblVertSinglLeft || c == ucBoxSinglVertLeft // пересечение (правая граница)
//			|| c == ucBoxDblVert || c == ucBoxSinglVert
//			|| (c >= ucBox25 && c <= ucBox75) || c == ucUpScroll || c == ucDnScroll) // полоса прокрутки может быть только справа
//		{
//			nY--; continue;
//		}
//		// Другие символы недопустимы
//		break;
//	}
//	_ASSERTE(nY >= 0);
//	nMostTop = nY;
//	return (nMostTop < nFromY);
//}
//
//bool CRealConsole::FindFrameBottom_ByRight(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostBottom)
//{
//	// Попытаемся спуститься вдоль правой рамки до угла
//	int nY = nFromY;
//	int nEnd = nHeight - 1;
//	wchar_t c; //, cd = ucBoxDblVert;
//	//// В случае с правой панелью в правом верхнем углу могут быть часы, а не собственно угол
//	//// "<=1" т.к. надо учесть возможность постоянного меню
//	//c = pChar[nY*nWidth+nFromX];
//	//if (nFromY <= 1 && nFromX == (nWidth-1)) {
//	//	if (isDigit(c)) {
//	//		cd = c;
//	//	}
//	//}
//	while (nY < nEnd)
//	{
//		c = pChar[(nY+1)*nWidth+nFromX];
//		if (c == ucBoxDblUpLeft || c == ucBoxSinglUpLeft // двойной и одинарный угол (правый нижний)
//			//|| c == cd // учесть часы на правой панели
//			|| c == ucBoxDblVertLeft || c == ucBoxDblVertSinglLeft || c == ucBoxSinglVertLeft // пересечение (правая граница)
//			|| c == ucBoxDblVert || c == ucBoxSinglVert
//			|| (c >= ucBox25 && c <= ucBox75) || c == ucUpScroll || c == ucDnScroll) // полоса прокрутки может быть только справа
//		{
//			nY++; continue;
//		}
//		// Другие символы недопустимы
//		break;
//	}
//	_ASSERTE(nY < nHeight);
//	nMostBottom = nY;
//	return (nMostBottom > nFromY);
//}
//
//bool CRealConsole::FindFrameBottom_ByLeft(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostBottom)
//{
//	// Попытаемся спуститься вдоль левой рамки до угла
//	int nY = nFromY;
//	int nEnd = nHeight - 1;
//	wchar_t c;
//	while (nY < nEnd)
//	{
//		c = pChar[(nY+1)*nWidth+nFromX];
//		if (c == ucBoxDblUpRight || c == ucBoxSinglUpRight // двойной и одинарный угол (левый нижний)
//			|| c == ucBoxDblVertRight || c == ucBoxDblVertSinglRight || c == ucBoxSinglVertRight
//			|| c == ucBoxDblVert || c == ucBoxSinglVert
//			) // пересечение (правая граница)
//		{
//			nY++; continue;
//		}
//		// Другие символы недопустимы
//		break;
//	}
//	_ASSERTE(nY < nHeight);
//	nMostBottom = nY;
//	return (nMostBottom > nFromY);
//}
//
//// Мы на правой границе рамки. Найти диалог слева
//bool CRealConsole::FindDialog_Right(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, BOOL &bMarkBorder)
//{
//	bMarkBorder = TRUE;
//	int nY = nFromY;
//	int nX = nFromX;
//	nMostRight = nFromX;
//	nMostBottom = nFromY; // сразу запомним
//
//	// Попытаемся подняться вверх вдоль правой рамки до угла
//	if (FindFrameTop_ByRight(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nY))
//		nFromY = nY;
//
//	// Попытаемся спуститься вдоль правой рамки до угла
//	if (FindFrameBottom_ByRight(pChar, pAttr, nWidth, nHeight, nFromX, nMostBottom, nY))
//		nMostBottom = nY;
//
//
//	// Теперь можно искать диалог
//	
//	// по верху
//	if (FindFrameLeft_ByTop(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nX))
//		nFromX = nX;
//
//	// по низу
//	if (FindFrameLeft_ByBottom(pChar, pAttr, nWidth, nHeight, nFromX, nMostBottom, nX))
//		if (nX < nFromX) nFromX = nX;
//
//	_ASSERTE(nFromX>=0 && nFromY>=0);
//
//	return true;
//}
//
//// Диалог может быть как слева, так и справа от вертикальной линии
//bool CRealConsole::FindDialog_Any(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, BOOL &bMarkBorder)
//{
//	wchar_t c;
//	wchar_t wc = pChar[nFromY*nWidth+nFromX];
//	// Так что сначала нужно подняться (или опуститься) по рамке до угла
//	for (int ii = 0; ii <= 1; ii++)
//	{
//		int nY = nFromY;
//		int nYEnd = (!ii) ? -1 : nHeight;
//		int nYStep = (!ii) ? -1 : 1;
//		wchar_t wcCorn1 = (!ii) ? ucBoxSinglDownLeft : ucBoxSinglUpLeft;
//		wchar_t wcCorn2 = (!ii) ? ucBoxDblDownLeft : ucBoxDblUpLeft;
//		wchar_t wcCorn3 = (!ii) ? ucBoxDblDownRight : ucBoxDblUpRight;
//		wchar_t wcCorn4 = (!ii) ? ucBoxSinglDownRight : ucBoxSinglUpRight;
//
//		// TODO: если можно - определим какой угол мы ищем (двойной/одинарный)
//		
//		// поехали
//		while (nY != nYEnd)
//		{
//			c = pChar[nY*nWidth+nFromX];
//			if (c == wcCorn1 || c == wcCorn2 // двойной и одинарный угол (правый верхний/нижний)
//				|| c == ucBoxDblVertLeft || c == ucBoxDblVertSinglLeft || c == ucBoxSinglVertLeft // пересечение (правая граница)
//				|| (c >= ucBox25 && c <= ucBox75) || c == ucUpScroll || c == ucDnScroll) // полоса прокрутки может быть только справа
//			{
//				if (FindDialog_Right(pChar, pAttr, nWidth, nHeight, nFromX, nY, nMostRight, nMostBottom, bMarkBorder)) {
//					nFromY = nY;
//					return true;
//				}
//				return false; // непонятно что...
//			}
//			if (c == wcCorn3 || c == wcCorn4 // двойной и одинарный угол (левый верхний/нижний)
//				|| c == ucBoxDblVertRight || c == ucBoxDblVertSinglRight || c == ucBoxSinglVertRight) // пересечение (правая граница)
//			{
//				if (FindDialog_Left(pChar, pAttr, nWidth, nHeight, nFromX, nY, nMostRight, nMostBottom, bMarkBorder)) {
//					nFromY = nY;
//					return true;
//				}
//				return false; // непонятно что...
//			}
//			if (c != wc) {
//				// Другие символы недопустимы
//				break;
//			}
//
//			// Сдвигаемся (вверх или вниз)
//			nY += nYStep;
//		}
//	}
//	return false;
//}
//
//bool CRealConsole::FindDialog_Inner(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY)
//{
//	// наткнулись на вертикальную линию на панели
//	int nShift = nWidth*nFromY;
//	wchar_t wc = pChar[nShift+nFromX];
//
//	if (wc != ucBoxSinglVert) {
//		_ASSERTE(wc == ucBoxSinglVert);
//		return false;
//	}
//
//	// Поверх панели может быть диалог... хорошо бы это учитывать
//
//	int nY = nFromY;
//	while (++nY < nHeight) {
//		wc = pChar[nFromX+nY*nWidth];
//		switch (wc)
//		{
//		// на панелях вертикальная линия может прерываться '}' (когда имя файла в колонку не влезает)
//		case ucBoxSinglVert:
//		case L'}':
//			continue;
//
//		// Если мы натыкаемся на угловой рамочный символ - значит это часть диалога. выходим
//		case ucBoxSinglUpRight:
//		case ucBoxSinglUpLeft:
//		case ucBoxSinglVertRight:
//		case ucBoxSinglVertLeft:
//			return false;
//
//		// достигли низа панели
//		case ucBoxSinglUpHorz:
//		case ucBoxSinglUpDblHorz:
//			nY++; // пометить все сверху (включая)
//		// иначе - прервать поиск и пометить все сверху (не включая)
//		default:
//			nY--;
//			{
//				// Пометить всю линию до верха (содержащую допустимые символы) как часть рамки
//				CharAttr* p = pAttr+(nWidth*nY+nFromX);
//				while (nY-- >= nFromY) {
//					//_ASSERTE(p->bDialog);
//					_ASSERTE(p >= pAttr);
//					p->bDialogVBorder = true;
//					p -= nWidth;
//				}
//				// мы могли начать не с верха панели
//				while (nY >= 0) {
//					wc = pChar[nFromX+nY*nWidth];
//					if (wc != ucBoxSinglVert && wc != ucBoxSinglDownHorz && wc != ucBoxSinglDownDblHorz)
//						break;
//					//_ASSERTE(p->bDialog);
//					_ASSERTE(p >= pAttr);
//					p->bDialogVBorder = true;
//					p -= nWidth;
//					nY --;
//				}
//			}
//			return true;
//		}
//	}
//
//	return false;
//}
//
//// Попытаемся найти рамку?
//bool CRealConsole::FindFrame_TopLeft(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nFrameX, int &nFrameY)
//{
//	// Попытаемся найти рамку?
//	nFrameX = -1; nFrameY = -1;
//	int nShift = nWidth*nFromY;
//	int nFindFrom = nShift+nFromX;
//	int nMaxAdd = min(5,(nWidth - nFromX - 1));
//	wchar_t wc;
//	// в этой же строке
//	for (int n = 1; n <= nMaxAdd; n++) {
//		wc = pChar[nFindFrom+n];
//		if (wc == ucBoxSinglDownRight || wc == ucBoxDblDownRight) {
//			nFrameX = nFromX+n; nFrameY = nFromY;
//			return true;
//		}
//	}
//	if (nFrameY == -1) {
//		// строкой ниже
//		nFindFrom = nShift+nWidth+nFromX;
//		for (int n = 0; n <= nMaxAdd; n++) {
//			wc = pChar[nFindFrom+n];
//			if (wc == ucBoxSinglDownRight || wc == ucBoxDblDownRight) {
//				nFrameX = nFromX+n; nFrameY = nFromY+1;
//				return true;
//			}
//		}
//	}
//	return false;
//}
//
//
//bool CRealConsole::ExpandDialogFrame(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int nFrameX, int nFrameY, int &nMostRight, int &nMostBottom)
//{
//	bool bExpanded = false;
//	int nStartRight = nMostRight;
//	int nStartBottom = nMostBottom;
//	// Теперь расширить nMostRight & nMostBottom на окантовку
//	int n, nShift = nWidth*nFromY;
//	wchar_t wc = pChar[nShift+nFromX];
//	DWORD nColor = pAttr[nShift+nFromX].crBackColor;
//
//	if (nFromX == nFrameX && nFromY == nFrameY) {
//		if (wc != ucBoxDblDownRight && wc != ucBoxSinglDownRight)
//			return false;
//
//		//Сначала нужно пройти вверх и влево
//		if (nFromY) { // Пробуем вверх
//			n = (nFromY-1)*nWidth+nFromX;
//			if (pAttr[n].crBackColor == nColor && (pChar[n] == L' ' || pChar[n] == ucNoBreakSpace)) {
//				nFromY--; bExpanded = true;
//			}
//		}
//		if (nFromX) { // Пробуем влево
//			int nMinMargin = nFromX-3; if (nMinMargin<0) nMinMargin = 0;
//			n = nFromY*nWidth+nFromX;
//			while (nFromX > nMinMargin) {
//				n--;
//				if (pAttr[n].crBackColor == nColor && (pChar[n] == L' ' || pChar[n] == ucNoBreakSpace)) {
//					nFromX--;
//				} else {
//					break;
//				}
//			}
//			bExpanded = (nFromX<nFrameX);
//		}
//		_ASSERTE(nFromX>=0 && nFromY>=0);
//	} else {
//		if (wc != ucSpace && wc != ucNoBreakSpace)
//			return false;
//	}
//
//	if (nMostRight < (nWidth-1)) {
//		int nMaxMargin = 3+(nFrameX - nFromX);
//		if (nMaxMargin > nWidth) nMaxMargin = nWidth;
//		int nFindFrom = nShift+nWidth+nMostRight+1;
//		n = 0;
//		wc = pChar[nShift+nFromX];
//
//		while (n < nMaxMargin) {
//			if (pAttr[nFindFrom].crBackColor != nColor || (pChar[nFindFrom] != L' ' && pChar[nFindFrom] != ucNoBreakSpace))
//				break;
//			n++; nFindFrom++;
//		}
//		if (n) {
//			nMostRight += n;
//			bExpanded = true;
//		}
//	}
//	_ASSERTE(nMostRight<nWidth);
//
//	// nMostBottom
//	if (nFrameY > nFromY && nMostBottom < (nHeight-1)) {
//		n = (nMostBottom+1)*nWidth+nFrameX;
//		if (pAttr[n].crBackColor == nColor && (pChar[n] == L' ' || pChar[n] == ucNoBreakSpace)) {
//			nMostBottom ++; bExpanded = true;
//		}
//	}
//	_ASSERTE(nMostBottom<nHeight);
//
//	return bExpanded;
//}
//
//// В идеале - сюда попадать не должны. Это может быть или кусок диалога
//// другая часть которого или скрыта, или вытащена за границы экрана
//bool CRealConsole::FindByBackground(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, BOOL &bMarkBorder)
//{
//	// Придется идти просто по цвету фона
//	// Это может быть диалог, рамка которого закрыта другим диалогом, 
//	// или вообще кусок диалога, у которого видна только часть рамки
//	DWORD nBackColor = pAttr[nFromX+nWidth*nFromY].crBackColor;
//	int n, nMostRightBottom, nShift = nWidth*nFromY;
//	// Найти правую границу
//	nMostRight = nFromX;
//	while (++nMostRight < nWidth) {
//		n = nShift+nMostRight;
//		if (pAttr[n].crBackColor != nBackColor)
//			break; // конец цвета фона диалога
//	}
//	nMostRight--;
//	_ASSERTE(nMostRight<nWidth);
//
//	wchar_t wc = pChar[nFromY*nWidth+nMostRight];
//	if (wc >= ucBoxSinglHorz && wc <= ucBoxDblVertHorz)
//	{
//		switch (wc)
//		{
//		case ucBoxSinglDownRight: case ucBoxDblDownRight:
//		case ucBoxSinglUpRight: case ucBoxDblUpRight:
//		case ucBoxSinglDownLeft: case ucBoxDblDownLeft:
//		case ucBoxSinglUpLeft: case ucBoxDblUpLeft:
//		case ucBoxDblVert: case ucBoxSinglVert:
//			{
//				DetectDialog(pChar, pAttr, nWidth, nHeight, nMostRight, nFromY);
//				if (pAttr[nShift+nFromX].bDialog)
//					return false; // все уже обработано
//			}
//		}
//	} else if (nMostRight && ((wc >= ucBox25 && wc <= ucBox75) || wc == ucUpScroll || wc == ucDnScroll)) {
//		int nX = nMostRight;
//		if (FindDialog_Right(pChar, pAttr, nWidth, nHeight, nX, nFromY, nMostRight, nMostBottom, bMarkBorder)) {
//			nFromX = nX;
//			return false;
//		}
//	}
//
//	// Найти нижнюю границу
//	nMostBottom = nFromY;
//	while (++nMostBottom < nHeight) {
//		n = nFromX+nMostBottom*nWidth;
//		if (pAttr[n].crBackColor != nBackColor)
//			break; // конец цвета фона диалога
//	}
//	nMostBottom--;
//	_ASSERTE(nMostBottom<nHeight);
//
//	// Найти нижнюю границу по правой стороне
//	nMostRightBottom = nFromY;
//	while (++nMostRightBottom < nHeight) {
//		n = nMostRight+nMostRightBottom*nWidth;
//		if (pAttr[n].crBackColor != nBackColor)
//			break; // конец цвета фона диалога
//	}
//	nMostRightBottom--;
//	_ASSERTE(nMostRightBottom<nHeight);
//
//	// Результатом считаем - наибольшую высоту
//	if (nMostRightBottom > nMostBottom)
//		nMostBottom = nMostRightBottom;
//
//	return true;
//}
//
//void CRealConsole::DetectDialog(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int nFromX, int nFromY, int *pnMostRight, int *pnMostBottom)
//{
//	if (nFromX >= nWidth || nFromY >= nHeight)
//	{
//		_ASSERTE(nFromX<nWidth);
//		_ASSERTE(nFromY<nHeight);
//		return;
//	}
//
//#ifdef _DEBUG
//	if (nFromX == 79 && nFromY == 1) {
//		nFromX = nFromX;
//	}
//#endif
//
//	// защита от переполнения стека (быть не должно)
//	if (mn_DetectCallCount >= 3) {
//		gbInTransparentAssert = true;
//		_ASSERTE(mn_DetectCallCount<3);
//		gbInTransparentAssert = false;
//		return;
//	}
//	
//	
//	/* *********************************************** */
//	/* После этой строки 'return' использовать нельзя! */
//	/* *********************************************** */
//	mn_DetectCallCount++;
//
//	wchar_t wc; //, wcMostRight, wcMostBottom, wcMostRightBottom, wcMostTop, wcNotMostBottom1, wcNotMostBottom2;
//	int nMostRight, nMostBottom; //, nMostRightBottom, nMostTop, nShift, n;
//	//DWORD nBackColor;
//	BOOL bMarkBorder = FALSE;
//
//	// Самое противное - детект диалога, который частично перекрыт другим диалогом
//
//	int nShift = nWidth*nFromY;
//	wc = pChar[nShift+nFromX];
//
//
//	WARNING("Доделать detect");
//	/*
//	Если нижний-левый угол диалога не нашли - он может быть закрыт другим диалогом?
//	попытаться найти правый-нижний угол?
//	*/
//
//
//	if (wc >= ucBoxSinglHorz && wc <= ucBoxDblVertHorz)
//	{
//		switch (wc)
//		{
//			// Верхний левый угол?
//			case ucBoxSinglDownRight: case ucBoxDblDownRight:
//			{
//				// Диалог без окантовки. Все просто - идем по рамке
//				if (!FindDialog_TopLeft(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nMostRight, nMostBottom, bMarkBorder))
//					goto fin;
//				goto done;
//			}
//			// Нижний левый угол?
//			case ucBoxSinglUpRight: case ucBoxDblUpRight:
//			{
//				// Сначала нужно будет подняться по рамке вверх
//				if (!FindDialog_TopLeft(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nMostRight, nMostBottom, bMarkBorder))
//					goto fin;
//				goto done;
//			}
//
//			// Верхний правый угол?
//			case ucBoxSinglDownLeft: case ucBoxDblDownLeft:
//			{
//				// Диалог без окантовки. Все просто - идем по рамке
//				if (!FindDialog_TopRight(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nMostRight, nMostBottom, bMarkBorder))
//					goto fin;
//				goto done;
//			}
//			// Нижний правый угол?
//			case ucBoxSinglUpLeft: case ucBoxDblUpLeft:
//			{
//				// Сначала нужно будет подняться по рамке вверх
//				if (!FindDialog_Right(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nMostRight, nMostBottom, bMarkBorder))
//					goto fin;
//				goto done;
//			}
//
//			case ucBoxDblVert: case ucBoxSinglVert:
//			{
//				// наткнулись на вертикальную линию на панели
//				if (wc == ucBoxSinglVert) {
//					if (FindDialog_Inner(pChar, pAttr, nWidth, nHeight, nFromX, nFromY))
//						goto fin;
//				}
//
//				// Диалог может быть как слева, так и справа от вертикальной линии
//				if (FindDialog_Any(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nMostRight, nMostBottom, bMarkBorder))
//					goto done;
//			}
//		}
//	}
//	
//	if (wc == ucSpace || wc == ucNoBreakSpace)
//	{
//		// Попытаемся найти рамку?
//		int nFrameX = -1, nFrameY = -1;
//		if (FindFrame_TopLeft(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nFrameX, nFrameY))
//		{
//			// Если угол нашли - ищем рамку по рамке :)
//			DetectDialog(pChar, pAttr, nWidth, nHeight, nFrameX, nFrameY, &nMostRight, &nMostBottom);
//			//// Теперь расширить nMostRight & nMostBottom на окантовку
//			//ExpandDialogFrame(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nFrameX, nFrameY, nMostRight, nMostBottom);
//			//
//			goto done;
//		}
//	}
//	
//
//	// Придется идти просто по цвету фона
//	// Это может быть диалог, рамка которого закрыта другим диалогом, 
//	// или вообще кусок диалога, у которого видна только часть рамки
//	if (!FindByBackground(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nMostRight, nMostBottom, bMarkBorder))
//		goto fin; // значит уже все пометили, или диалога нет
//
//
//
//done:
//#ifdef _DEBUG
//	if (nFromX<0 || nFromX>=nWidth || nMostRight<nFromX || nMostRight>=nWidth
//		|| nFromY<0 || nFromY>=nHeight || nMostBottom<nFromY || nMostBottom>=nHeight)
//	{
//		//_ASSERT(FALSE);
//		// Это происходит, если обновление внутренних буферов произошло ДО
//		// завершения обработки диалогов (быстрый драг панелей, диалогов, и т.п.)
//		goto fin;
//	}
//#endif
//	// Забить атрибуты
//	MarkDialog(pChar, pAttr, nWidth, nHeight, nFromX, nFromY, nMostRight, nMostBottom, bMarkBorder);
//
//	// Вернуть размеры, если просили
//	if (pnMostRight) *pnMostRight = nMostRight;
//	if (pnMostBottom) *pnMostBottom = nMostBottom;
//fin:
//	mn_DetectCallCount--;
//	_ASSERTE(mn_DetectCallCount>=0);
//	return;
//}
//
//void CRealConsole::MarkDialog(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int nX1, int nY1, int nX2, int nY2, BOOL bMarkBorder, BOOL bFindExterior /*= TRUE*/)
//{
//	if (nX1<0 || nX1>=nWidth || nX2<nX1 || nX2>=nWidth
//		|| nY1<0 || nY1>=nHeight || nY2<nY1 || nY2>=nHeight)
//	{
//		_ASSERTE(nX1>=0 && nX1<nWidth);  _ASSERTE(nX2>=0 && nX2<nWidth);
//		_ASSERTE(nY1>=0 && nY1<nHeight); _ASSERTE(nY2>=0 && nY2<nHeight);
//		return;
//	}
//
//	TODO("Занести координаты в новый массив прямоугольников, обнаруженных в консоли");
//	if (m_DetectedDialogs.Count < MAX_DETECTED_DIALOGS) {
//		int i = m_DetectedDialogs.Count++;
//		m_DetectedDialogs.Rects[i].Left = nX1;
//		m_DetectedDialogs.Rects[i].Top = nY1;
//		m_DetectedDialogs.Rects[i].Right = nX2;
//		m_DetectedDialogs.Rects[i].Bottom = nY2;
//		m_DetectedDialogs.bWasFrame[i] = bMarkBorder;
//	}
//
//#ifdef _DEBUG
//	if (nX1 == 57 && nY1 == 0) {
//		nX2 = nX2;
//	}
//#endif
//
//	if (bMarkBorder) {
//		pAttr[nY1 * nWidth + nX1].bDialogCorner = TRUE;
//		pAttr[nY1 * nWidth + nX2].bDialogCorner = TRUE;
//		pAttr[nY2 * nWidth + nX1].bDialogCorner = TRUE;
//		pAttr[nY2 * nWidth + nX2].bDialogCorner = TRUE;
//	}
//
//	for (int nY = nY1; nY <= nY2; nY++) {
//		int nShift = nY * nWidth + nX1;
//
//		if (bMarkBorder) {
//			pAttr[nShift].bDialogVBorder = TRUE;
//			pAttr[nShift+nX2-nX1].bDialogVBorder = TRUE;
//		}
//
//		for (int nX = nX1; nX <= nX2; nX++, nShift++) {
//			if (nY > 0 && nX >= 58) {
//				nX = nX;
//			}
//			pAttr[nShift].bDialog = TRUE;
//			pAttr[nShift].bTransparent = FALSE;
//		}
//
//		//if (bMarkBorder)
//		//	pAttr[nShift].bDialogVBorder = TRUE;
//	}
//
//	// Если помечается диалог по рамке - попытаться определить окантовку
//	if (bFindExterior && bMarkBorder) {
//		int nMostRight = nX2, nMostBottom = nY2;
//		int nNewX1 = nX1, nNewY1 = nY1;
//		if (ExpandDialogFrame(pChar, pAttr, nWidth, nHeight, nNewX1, nNewY1, nX1, nY1, nMostRight, nMostBottom)) {
//			_ASSERTE(nNewX1>=0 && nNewY1>=0);
//			//Optimize: помечать можно только окантовку - сам диалог уже помечен
//			MarkDialog(pChar, pAttr, nWidth, nHeight, nNewX1, nNewY1, nMostRight, nMostBottom, TRUE, FALSE);
//		}
//	}
//}
//

void CRealConsole::PrepareTransparent(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight)
{
	//if (!mp_ConsoleInfo)
	if (!pChar || !pAttr)
		return;

	CEFAR_INFO FI = {0};
	if (m_FarInfo.IsValid()) {
		//FI = *mp_FarInfo;
		m_FarInfo.GetTo(&FI);
	}

	#ifdef _DEBUG
	if (mb_LeftPanel && !mb_RightPanel && mr_LeftPanelFull.right > 120) {
		mb_LeftPanel = mb_LeftPanel;
	}
	#endif

	if (mb_LeftPanel) {
		FI.bFarLeftPanel = true;
		FI.FarLeftPanel.PanelRect = mr_LeftPanelFull;
	} else {
		FI.bFarLeftPanel = false;
	}
	if (mb_RightPanel) {
		FI.bFarRightPanel = true;
		FI.FarRightPanel.PanelRect = mr_RightPanelFull;
	} else {
		FI.bFarRightPanel = false;
	}

	FI.bViewerOrEditor = FALSE;
	if (!FI.bFarLeftPanel && !FI.bFarRightPanel) {
		// Если нет панелей - это может быть вьювер/редактор
		FI.bViewerOrEditor = (isViewer() || isEditor());
	}

	mp_Rgn->PrepareTransparent(&FI, mp_VCon->mp_Colors, &con.m_sbi, pChar, pAttr, nWidth, nHeight);

	#ifdef _DEBUG
	int nCount = mp_Rgn->GetDetectedDialogs(0,NULL,NULL);
	if (nCount == 1) {
		nCount = 1;
	}
	#endif
}
//void CRealConsole::PrepareTransparent(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight)
//{
//	if (gbInTransparentAssert)
//		return;
//
//	m_DetectedDialogs.Count = 0;
//
//	if (!mp_ConsoleInfo || !gSet.NeedDialogDetect())
//		return;
//
//	// !!! в буферном режиме - никакой прозрачности, но диалоги - детектим, пригодится при отрисовке
//
//	// В редакторах-вьюверах тоже нужен детект
//	//if (!mp_ConsoleInfo->FarInfo.bFarPanelAllowed)
//	//	return;
//	//if (nCurFarPID && pRCon->mn_LastFarReadIdx != pRCon->mp_ConsoleInfo->nFarReadIdx) {
//	//if (isPressed(VK_CONTROL) && isPressed(VK_SHIFT) && isPressed(VK_MENU))
//	//	return;
//
//	WARNING("Если информация в FarInfo не заполнена - брать умолчания!");
//	WARNING("Учитывать возможность наличия номеров окон, символа записи 'R', и по хорошему, ч/б режима");
//
//	//COLORREF crColorKey = gSet.ColorKey;
//	// реальный цвет, заданный в фаре
//	int nUserBackIdx = (mp_ConsoleInfo->FarInfo.nFarColors[COL_COMMANDLINEUSERSCREEN] & 0xF0) >> 4;
//	COLORREF crUserBack = mp_VCon->mp_Colors[nUserBackIdx];
//	int nMenuBackIdx = (mp_ConsoleInfo->FarInfo.nFarColors[COL_MENUTITLE] & 0xF0) >> 4;
//	COLORREF crMenuTitleBack = mp_VCon->mp_Colors[nMenuBackIdx];
//	
//	// Для детекта наличия PanelTabs
//	int nPanelTextBackIdx = (mp_ConsoleInfo->FarInfo.nFarColors[COL_PANELTEXT] & 0xF0) >> 4;
//	int nPanelTextForeIdx = mp_ConsoleInfo->FarInfo.nFarColors[COL_PANELTEXT] & 0xF;
//	
//	// При bUseColorKey Если панель погашена (или панели) то 
//	// 1. UserScreen под ним заменяется на crColorKey
//	// 2. а текст - на пробелы
//	// Проверять наличие KeyBar по настройкам (Keybar + CmdLine)
//	bool bShowKeyBar = (mp_ConsoleInfo->FarInfo.nFarInterfaceSettings & 8/*FIS_SHOWKEYBAR*/) != 0;
//	int nBottomLines = bShowKeyBar ? 2 : 1;
//	// Проверять наличие MenuBar по настройкам
//	// Или может быть меню сейчас показано?
//	// 1 - при видимом сейчас или постоянно меню
//	bool bAlwaysShowMenuBar = (mp_ConsoleInfo->FarInfo.nFarInterfaceSettings & 0x10/*FIS_ALWAYSSHOWMENUBAR*/) != 0;
//	int nTopLines = bAlwaysShowMenuBar ? 1 : 0;
//
//	// Проверка теперь в другом месте (по плагину), да и левый нижний угол могут закрыть диалогом...
//	//// Проверим, что фар загрузился
//	//if (bShowKeyBar) {
//	//	// в левом-нижнем углу должна быть цифра 1
//	//	if (pChar[nWidth*(nHeight-1)] != L'1')
//	//		return;
//	//	// соответствующего цвета
//	//	BYTE KeyBarNoColor = mp_ConsoleInfo->FarInfo.nFarColors[COL_KEYBARNUM];
//	//	if (pAttr[nWidth*(nHeight-1)].nBackIdx != ((KeyBarNoColor & 0xF0)>>4))
//	//		return;
//	//	if (pAttr[nWidth*(nHeight-1)].nForeIdx != (KeyBarNoColor & 0xF))
//	//		return;
//	//}
//
//	// Теперь информация о панелях хронически обновляется плагином
//	//if (mb_LeftPanel)
//	//	MarkDialog(pAttr, nWidth, nHeight, mr_LeftPanelFull.left, mr_LeftPanelFull.top, mr_LeftPanelFull.right, mr_LeftPanelFull.bottom);
//	//if (mb_RightPanel)
//	//	MarkDialog(pAttr, nWidth, nHeight, mr_RightPanelFull.left, mr_RightPanelFull.top, mr_RightPanelFull.right, mr_RightPanelFull.bottom);
//
//	// Пометить непрозрачными панели
//	RECT r;
//	bool lbLeftVisible = false, lbRightVisible = false, lbFullPanel = false;
//
//	// Хотя информация о панелях хронически обновляется плагином, но это могут и отключить,
//	// да и отрисовка на экране может несколько задержаться
//
//	if (mb_LeftPanel) {
//		lbLeftVisible = true;
//		r = mr_LeftPanelFull;
//	} else
//	// Но если часть панели скрыта диалогами - наш детект панели мог не сработать
//	if (mp_ConsoleInfo->FarInfo.bFarLeftPanel && mp_ConsoleInfo->FarInfo.FarLeftPanel.Visible) {
//		// В "буферном" режиме размер панелей намного больше экрана
//		lbLeftVisible = ConsoleRect2ScreenRect(mp_ConsoleInfo->FarInfo.FarLeftPanel.PanelRect, &r);
//	}
//	if (lbLeftVisible) {
//		if (r.right == (nWidth-1))
//			lbFullPanel = true; // значит правой быть не может
//		MarkDialog(pChar, pAttr, nWidth, nHeight, r.left, r.top, r.right, r.bottom, TRUE);
//		// Для детекта наличия PanelTabs
//		if (nHeight > (nBottomLines+r.bottom+1)) {
//			int nIdx = nWidth*(r.bottom+1)+r.right-1;
//			if (pChar[nIdx-1] == 9616 && pChar[nIdx] == L'+' && pChar[nIdx+1] == 9616
//				&& pAttr[nIdx].nBackIdx == nPanelTextBackIdx
//				&& pAttr[nIdx].nForeIdx == nPanelTextForeIdx)
//			{
//				MarkDialog(pChar, pAttr, nWidth, nHeight, r.left, r.bottom+1, r.right, r.bottom+1);
//			}
//		}
//	}
//
//	if (!lbFullPanel) {
//		if (mb_RightPanel) {
//			lbRightVisible = true;
//			r = mr_RightPanelFull;
//		} else
//		// Но если часть панели скрыта диалогами - наш детект панели мог не сработать
//		if (mp_ConsoleInfo->FarInfo.bFarRightPanel && mp_ConsoleInfo->FarInfo.FarRightPanel.Visible) {
//			// В "буферном" режиме размер панелей намного больше экрана
//			lbRightVisible = ConsoleRect2ScreenRect(mp_ConsoleInfo->FarInfo.FarRightPanel.PanelRect, &r);
//		}
//		if (lbRightVisible) {
//			MarkDialog(pChar, pAttr, nWidth, nHeight, r.left, r.top, r.right, r.bottom, TRUE);
//			// Для детекта наличия PanelTabs
//			if (nHeight > (nBottomLines+r.bottom+1)) {
//				int nIdx = nWidth*(r.bottom+1)+r.right-1;
//				if (pChar[nIdx-1] == 9616 && pChar[nIdx] == L'+' && pChar[nIdx+1] == 9616
//					&& pAttr[nIdx].nBackIdx == nPanelTextBackIdx
//					&& pAttr[nIdx].nForeIdx == nPanelTextForeIdx)
//				{
//					MarkDialog(pChar, pAttr, nWidth, nHeight, r.left, r.bottom+1, r.right, r.bottom+1);
//				}
//			}
//		}
//	}
//
//	if (!lbLeftVisible && !lbRightVisible) {
//		TODO("Хотя вообще-то детект диалогов делать все-таки нужно");
//		if (isPressed(VK_CONTROL) && isPressed(VK_SHIFT) && isPressed(VK_MENU))
//			return; // По CtrlAltShift - показать UserScreen (не делать его прозрачным)
//	}
//
//	// Может быть первая строка - меню? постоянное или текущее
//	if (bAlwaysShowMenuBar // всегда
//		|| (pAttr[0].crBackColor == crMenuTitleBack
//			&& (pChar[0] == L' ' && pChar[1] == L' ' && pChar[2] == L' ' && pChar[3] == L' ' && pChar[4] != L' '))
//			)
//	{
//		MarkDialog(pChar, pAttr, nWidth, nHeight, 0, 0, nWidth-1, 0);
//	}
//
//	WARNING("!!! Установку bTransparent делать во второй проход, когда все диалоги уже определены !!!");
//
//	if (mn_DetectCallCount != 0) {
//		gbInTransparentAssert = true;
//		_ASSERT(mn_DetectCallCount == 0);
//		gbInTransparentAssert = false;
//	}
//
//	wchar_t* pszDst = pChar;
//	CharAttr* pnDst = pAttr;
//	for (int nY = 0; nY < nHeight; nY++)
//	{
//		if (nY >= nTopLines && nY < (nHeight-nBottomLines))
//		{
//			// ! первый cell - резервируем для скрытия/показа панелей
//			int nX1 = 0;
//			int nX2 = nWidth-1; // по умолчанию - на всю ширину
//
//			if (!mb_LeftPanel && mb_RightPanel) {
//				// Погашена только левая панель
//				nX2 = mr_RightPanelFull.left-1;
//			} else if (mb_LeftPanel && !mb_RightPanel) {
//				// Погашена только правая панель
//				nX1 = mr_LeftPanelFull.right+1;
//			} else {
//				//Внимание! Панели могут быть, но они могут быть перекрыты PlugMenu!
//			}
//
//#ifdef _DEBUG
//			if (nY == 16) {
//				nY = nY;
//			}
//#endif
//
//			int nShift = nY*nWidth+nX1;
//			int nX = nX1;
//			while (nX <= nX2)
//			{
//				// Если еще не определен как поле диалога
//				
//				/*if (!pnDst[nX].bDialogVBorder) {
//					if (pszDst[nX] == ucBoxSinglDownLeft || pszDst[nX] == ucBoxDblDownLeft) {
//						// Это правый кусок диалога, который не полностью влез на экран
//						// Пометить "рамкой" до его низа
//						int nYY = nY;
//						int nXX = nX;
//						wchar_t wcWait = (pszDst[nX] == ucBoxSinglDownLeft) ? ucBoxSinglUpLeft : ucBoxDblUpLeft;
//						while (nYY++ < nHeight) {
//							if (!pnDst[nX].bDialog)
//								break;
//							pnDst[nXX].bDialogVBorder = TRUE;
//							if (pszDst[nXX] == wcWait)
//								break;
//							nXX += nWidth;
//						}
//					}
//
//					//Optimize:
//					if (isCharBorderLeftVertical(pszDst[nX])) {
//						DetectDialog(pChar, pAttr, nWidth, nHeight, nX, nY, &nX);
//						nX++; nShift++;
//						continue;
//					}
//				}*/
//				if (!pnDst[nX].bDialog) {
//					if (pnDst[nX].crBackColor != crUserBack) {
//						DetectDialog(pChar, pAttr, nWidth, nHeight, nX, nY);
//					}
//				} else if (!pnDst[nX].bDialogCorner) {
//					switch (pszDst[nX]) {
//						case ucBoxSinglDownRight:
//						case ucBoxSinglDownLeft:
//						case ucBoxSinglUpRight:
//						case ucBoxSinglUpLeft:
//						case ucBoxDblDownRight:
//						case ucBoxDblDownLeft:
//						case ucBoxDblUpRight:
//						case ucBoxDblUpLeft:
//							// Это правый кусок диалога, который не полностью влез на экран
//							// Пометить "рамкой" до его низа
//							DetectDialog(pChar, pAttr, nWidth, nHeight, nX, nY);
//					}
//				}
//				nX++; nShift++;
//			}
//		}
//		pszDst += nWidth;
//		pnDst += nWidth;
//	}
//
//	if (mn_DetectCallCount != 0) {
//		_ASSERT(mn_DetectCallCount == 0);
//	}
//
//
//	if (con.bBufferHeight)
//		return; // в буферном режиме - никакой прозрачности
//
//	// 0x0 должен быть непрозрачным
//	//pAttr[0].bDialog = TRUE;
//
//	pszDst = pChar;
//	pnDst = pAttr;
//	for (int nY = 0; nY < nHeight; nY++)
//	{
//		if (nY >= nTopLines && nY < (nHeight-nBottomLines))
//		{
//			// ! первый cell - резервируем для скрытия/показа панелей
//			int nX1 = 0;
//			int nX2 = nWidth-1; // по умолчанию - на всю ширину
//
//			// Все-таки, если панели приподняты - делаем UserScreen прозрачным
//			// Ведь остается возможность посмотреть его по CtrlAltShift
//			
//			//if (!mb_LeftPanel && mb_RightPanel) {
//			//	// Погашена только левая панель
//			//	nX2 = mr_RightPanelFull.left-1;
//			//} else if (mb_LeftPanel && !mb_RightPanel) {
//			//	// Погашена только правая панель
//			//	nX1 = mr_LeftPanelFull.right+1;
//			//} else {
//			//	//Внимание! Панели могут быть, но они могут быть перекрыты PlugMenu!
//			//}
//
//			WARNING("Во время запуска неприятно мелькает - пока не появятся панели - становится прозрачным");
//
//			int nShift = nY*nWidth+nX1;
//			int nX = nX1;
//			while (nX <= nX2)
//			{
//				// Если еще не определен как поле диалога
//				if (!pnDst[nX].bDialog) {
//					if (pnDst[nX].crBackColor == crUserBack) {
//						// помечаем прозрачным
//						pnDst[nX].bTransparent = TRUE;
//						//pnDst[nX].crBackColor = crColorKey;
//						//pszDst[nX] = L' ';
//					}
//				}
//				nX++; nShift++;
//			}
//		}
//		pszDst += nWidth;
//		pnDst += nWidth;
//	}
//
//	// Некрасиво...
//	//// 0x0 должен быть непрозрачным
//	//pAttr[0].bTransparent = FALSE;
//}

BOOL CRealConsole::IsConsoleDataChanged()
{
	if (!this) return FALSE;
#ifdef _DEBUG
	if (con.bDebugLocked)
		return FALSE;
#endif

	return con.bConsoleDataChanged;
}

// nWidth и nHeight это размеры, которые хочет получить VCon (оно могло еще не среагировать на изменения?
void CRealConsole::GetConsoleData(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight)
{
	if (!this) return;

    //DWORD cbDstBufSize = nWidth * nHeight * 2;
    DWORD cwDstBufSize = nWidth * nHeight;

    _ASSERT(nWidth != 0 && nHeight != 0);
    if (nWidth == 0 || nHeight == 0)
        return;
#ifdef _DEBUG
	if (con.bDebugLocked)
		return;
#endif

	con.bConsoleDataChanged = FALSE;
        
    // формирование умолчательных цветов, по атрибутам консоли
    //TODO("В принципе, это можно делать не всегда, а только при изменениях");
    int  nColorIndex = 0;
	bool bExtendColors = gSet.isExtendColors;
	BYTE nExtendColor = gSet.nExtendColor;
    bool bExtendFonts = gSet.isExtendFonts;
    BYTE nFontNormalColor = gSet.nFontNormalColor;
    BYTE nFontBoldColor = gSet.nFontBoldColor;
    BYTE nFontItalicColor = gSet.nFontItalicColor;
	//BOOL bUseColorKey = gSet.isColorKey  // Должен быть включен в настройке
	//	&& isFar(TRUE/*abPluginRequired*/) // в фаре загружен плагин (чтобы с цветами не проколоться)
	//	&& (mp_tabs && mn_tabsCount>0 && mp_tabs->Current) // Текущее окно - панели
	//	&& !(mb_LeftPanel && mb_RightPanel) // и хотя бы одна панель погашена
	//	&& (!con.m_ci.bVisible || con.m_ci.dwSize<30) // и сейчас НЕ включен режим граббера
	//	;
    CharAttr lca, lcaTableExt[0x100], lcaTableOrg[0x100], *lcaTable; // crForeColor, crBackColor, nFontIndex, nForeIdx, nBackIdx, crOrigForeColor, crOrigBackColor
        
    //COLORREF lcrForegroundColors[0x100], lcrBackgroundColors[0x100];
    //BYTE lnForegroundColors[0x100], lnBackgroundColors[0x100], lnFontByIndex[0x100];
	TODO("OPTIMIZE: В принципе, это можно делать не всегда, а только при изменениях");
    for (int nBack = 0; nBack <= 0xF; nBack++)
    {
    	for (int nFore = 0; nFore <= 0xF; nFore++, nColorIndex++)
    	{
			memset(&lca, 0, sizeof(lca));
    		lca.nForeIdx = nFore;
    		lca.nBackIdx = nBack;
	    	lca.crForeColor = lca.crOrigForeColor = mp_VCon->mp_Colors[lca.nForeIdx];
	    	lca.crBackColor = lca.crOrigBackColor = mp_VCon->mp_Colors[lca.nBackIdx];
	    	lcaTableOrg[nColorIndex] = lca;
	    	
			if (bExtendFonts)
			{
				if (nBack == nFontBoldColor) // nFontBoldColor may be -1, тогда мы сюда не попадаем
				{
					if (nFontNormalColor != 0xFF)
						lca.nBackIdx = nFontNormalColor;
					lca.nFontIndex = 1; //  Bold
					lca.crBackColor = lca.crOrigBackColor = mp_VCon->mp_Colors[lca.nBackIdx];
				}
				else if (nBack == nFontItalicColor) // nFontItalicColor may be -1, тогда мы сюда не попадаем
				{
					if (nFontNormalColor != 0xFF)
						lca.nBackIdx = nFontNormalColor;
					lca.nFontIndex = 2; // Italic
					lca.crBackColor = lca.crOrigBackColor = mp_VCon->mp_Colors[lca.nBackIdx];
				}
			}
	    	lcaTableExt[nColorIndex] = lca;
		}
    }
    lcaTable = lcaTableOrg;
    
    
    MSectionLock csData; csData.Lock(&csCON);
    HEAPVAL

    wchar_t wSetChar = L' ';
    CharAttr lcaDef;
    lcaDef = lcaTable[7]; // LtGray on Black
    //WORD    wSetAttr = 7;
    #ifdef _DEBUG
    wSetChar = (wchar_t)8776; //wSetAttr = 12;
    lcaDef = lcaTable[12]; // Red on Black
    #endif

    if (!con.pConChar || !con.pConAttr)
    {
        wmemset(pChar, wSetChar, cwDstBufSize);
        for (DWORD i = 0; i < cwDstBufSize; i++)
        	pAttr[i] = lcaDef;
        //wmemset((wchar_t*)pAttr, wSetAttr, cbDstBufSize);
    //} else if (nWidth == con.nTextWidth && nHeight == con.nTextHeight) {
    //    TODO("Во время ресайза консоль может подглючивать - отдает не то что нужно...");
    //    //_ASSERTE(*con.pConChar!=ucBoxDblVert);
    //    memmove(pChar, con.pConChar, cbDstBufSize);
    //    PRAGMA_ERROR("Это заменить на for");
    //    memmove(pAttr, con.pConAttr, cbDstBufSize);
    }
    else
    {
        TODO("Во время ресайза консоль может подглючивать - отдает не то что нужно...");
        //_ASSERTE(*con.pConChar!=ucBoxDblVert);

        int nYMax = min(nHeight,con.nTextHeight);
        wchar_t  *pszDst = pChar, *pszSrc = con.pConChar;
        CharAttr *pnDst = pAttr;
        WORD     *pnSrc = con.pConAttr;
        const AnnotationInfo *pcolSrc = NULL;
        const AnnotationInfo *pcolEnd = NULL;
        BOOL bUseColorData = FALSE;
        if (gSet.isTrueColorer && m_TrueColorerMap.IsValid() && mp_TrueColorerData)
        {
        	pcolSrc = mp_TrueColorerData;
        	pcolEnd = mp_TrueColorerData + m_TrueColorerMap.Ptr()->bufferSize;
        	bUseColorData = TRUE;
    	}
        DWORD cbDstLineSize = nWidth * 2;
        DWORD cnSrcLineLen = con.nTextWidth;
        DWORD cbSrcLineSize = cnSrcLineLen * 2;
		#ifdef _DEBUG
		if (con.nTextWidth != con.m_sbi.dwSize.X)
		{
			_ASSERTE(con.nTextWidth == con.m_sbi.dwSize.X);
		}
		#endif
        DWORD cbLineSize = min(cbDstLineSize,cbSrcLineSize);
        int nCharsLeft = max(0, (nWidth-con.nTextWidth));
        int nY, nX;
        BYTE attrBackLast = 0;
        //int nPrevDlgBorder = -1;

        // Собственно данные
        for (nY = 0; nY < nYMax; nY++)
        {
        	if (nY == 1) lcaTable = lcaTableExt;
        
        	// Текст
            memmove(pszDst, pszSrc, cbLineSize);
            if (nCharsLeft > 0)
                wmemset(pszDst+cnSrcLineLen, wSetChar, nCharsLeft);

            // Атрибуты
            DWORD atr = 0;
            for (nX = 0; nX < (int)cnSrcLineLen; nX++, pnSrc++, pcolSrc++)
            {
	            atr = (*pnSrc) & 0xFF; // интересут только нижний байт - там индексы цветов
				TODO("OPTIMIZE: lca = lcaTable[atr];");
	            lca = lcaTable[atr];

				TODO("OPTIMIZE: вынести проверку bExtendColors за циклы");
			    if (bExtendColors && nY)
			    {
			        if (lca.nBackIdx == nExtendColor)
			        {
			            lca.nBackIdx = attrBackLast; // фон нужно заменить на обычный цвет из соседней ячейки
			            lca.nForeIdx += 0x10;
				    	lca.crForeColor = lca.crOrigForeColor = mp_VCon->mp_Colors[lca.nForeIdx];
				    	lca.crBackColor = lca.crOrigBackColor = mp_VCon->mp_Colors[lca.nBackIdx];
			        }
			        else
			        {
			            attrBackLast = lca.nBackIdx; // запомним обычный цвет предыдущей ячейки
			        }
			    }
			    
			    // Colorer & Far - TrueMod
				TODO("OPTIMIZE: вынести проверку bUseColorData за циклы");
			    if (bUseColorData)
			    {
			    	if (pcolSrc >= pcolEnd)
			    	{
			    		bUseColorData = FALSE;
		    		}
		    		else
		    		{
						TODO("OPTIMIZE: доступ к битовым полям тяжело идет...");
				    	if (pcolSrc->fg_valid)
				    	{
				    		lca.nFontIndex = 0;
				    		lca.crForeColor = pcolSrc->fg_color;
				    		if (pcolSrc->bk_valid)
				    			lca.crBackColor = pcolSrc->bk_color;
				    	}
				    	else if (pcolSrc->bk_valid)
				    	{
				    		lca.nFontIndex = 0;
				    		lca.crBackColor = pcolSrc->bk_color;
			    		}
			    		// nFontIndex: 0 - normal, 1 - bold, 2 - italic, 3 - bold&italic,..., 4 - underline, ...
			    		if (pcolSrc->style)
			    			lca.nFontIndex = pcolSrc->style & 7;
		    		}
			    }
	            
				TODO("OPTIMIZE: lca = lcaTable[atr];");
	            pnDst[nX] = lca;
	            //memmove(pnDst, pnSrc, cbLineSize);
            }
            for (nX = cnSrcLineLen; nX < nWidth; nX++)
            {
            	pnDst[nX] = lcaDef;
            }
            
            // Far2 показывате красный 'A' в правом нижнем углу консоли
            // Этот ярко красный цвет фона может попасть в Extend Font Colors
            if (bExtendFonts && ((nY+1) == nYMax) && isFar()
            	&& (pszDst[nWidth-1] == L'A') && (atr == 0xCF))
            {
            	// Вернуть "родной" цвет и шрифт
            	pnDst[nWidth-1] = lcaTable[atr];
            }

            // Next line
			pszDst += nWidth; pszSrc += cnSrcLineLen; 
            pnDst += nWidth; //pnSrc += con.nTextWidth;
        }
        // Если вдруг запросили большую высоту, чем текущая в консоли - почистить низ
        for (nY = nYMax; nY < nHeight; nY++)
        {
            wmemset(pszDst, wSetChar, nWidth);
            pszDst += nWidth;

            //wmemset((wchar_t*)pnDst, wSetAttr, nWidth);
            for (nX = 0; nX < nWidth; nX++)
            {
            	pnDst[nX] = lcaDef;
            }
            pnDst += nWidth;
        }

		// Подготовить данные для Transparent
		// обнаружение диалогов нужно только при включенной прозрачности, 
		// или при пропорциональном шрифте
		// Даже если НЕ (gSet.NeedDialogDetect()) - нужно сбросить количество прямоугольников.
		PrepareTransparent(pChar, pAttr, nWidth, nHeight);

		if (mn_LastRgnFlags != mp_Rgn->GetFlags())
		{
			if (this->isActive())
				PostMessage(ghWnd, WM_SETCURSOR, -1, -1);
			mn_LastRgnFlags = mp_Rgn->GetFlags();
		}
		
		if (con.m_sel.dwFlags)
		{
			BOOL lbStreamMode = (con.m_sel.dwFlags & CONSOLE_TEXT_SELECTION) == CONSOLE_TEXT_SELECTION;
			SMALL_RECT rc = con.m_sel.srSelection;
			if (rc.Left<0) rc.Left = 0; else if (rc.Left>=nWidth) rc.Left = nWidth-1;
			if (rc.Top<0) rc.Top = 0; else if (rc.Top>=nHeight) rc.Top = nHeight-1;
			if (rc.Right<0) rc.Right = 0; else if (rc.Right>=nWidth) rc.Right = nWidth-1;
			if (rc.Bottom<0) rc.Bottom = 0; else if (rc.Bottom>=nHeight) rc.Bottom = nHeight-1;
			// для прямоугольника выделения сбрасываем прозрачность и ставим стандартный цвет выделения (lcaSel)
			//CharAttr lcaSel = lcaTable[gSet.isCTSColorIndex]; // Black on LtGray

			BYTE nForeIdx = (gSet.isCTSColorIndex & 0xF);
			COLORREF crForeColor = mp_VCon->mp_Colors[nForeIdx];
			BYTE nBackIdx = (gSet.isCTSColorIndex & 0xF0) >> 4;
			COLORREF crBackColor = mp_VCon->mp_Colors[nBackIdx];

			int nX1, nX2;
			// Block mode
			for (nY = rc.Top; nY <= rc.Bottom; nY++)
			{
				if (!lbStreamMode)
				{
					nX1 = rc.Left; nX2 = rc.Right;
				}
				else
				{
					nX1 = (nY == rc.Top) ? rc.Left : 0;
					nX2 = (nY == rc.Bottom) ? rc.Right : (nWidth-1);
				}

				pnDst = pAttr + nWidth*nY + nX1;
	            
				for (nX = nX1; nX <= nX2; nX++, pnDst++)
				{
            		//pnDst[nX] = lcaSel; -- чтобы не сбрасывать флаги рамок и диалогов - ставим по полям
					pnDst->crForeColor = pnDst->crOrigForeColor = crForeColor;
					pnDst->crBackColor = pnDst->crOrigBackColor = crBackColor;
					pnDst->nForeIdx = nForeIdx;
					pnDst->nBackIdx = nBackIdx;
					pnDst->bTransparent = FALSE;
					pnDst->ForeFont = 0;
				}
			}

			//} else {
			//	int nX1, nX2;
			//	for (nY = rc.Top; nY <= rc.Bottom; nY++) {
			//		pnDst = pAttr + nWidth*nY;

			//		nX1 = (nY == rc.Top) ? rc.Left : 0;
			//		nX2 = (nY == rc.Bottom) ? rc.Right : (nWidth-1);

			//		for (nX = nX1; nX <= nX2; nX++) {
			//			pnDst[nX] = lcaSel;
			//		}
			//	}
			//}
		}
    }
    // Если требуется показать "статус" - принудительно перебиваем первую видимую строку возвращаемого буфера
	if (ms_ConStatus[0])
	{
		int nLen = lstrlen(ms_ConStatus);
		wmemcpy(pChar, ms_ConStatus, nLen);
		if (nWidth>nLen)
			wmemset(pChar+nLen, L' ', nWidth-nLen);
		wmemset((wchar_t*)pAttr, 7, nWidth);
	}
	
	//FIN
    HEAPVAL
    csData.Unlock();
}

//#define PICVIEWMSG_SHOWWINDOW (WM_APP + 6)
//#define PICVIEWMSG_SHOWWINDOW_KEY 0x0101
//#define PICVIEWMSG_SHOWWINDOW_ASC 0x56731469

BOOL CRealConsole::ShowOtherWindow(HWND hWnd, int swShow)
{
	if ((IsWindowVisible(hWnd) == FALSE) == (swShow == SW_HIDE))
		return TRUE; // уже все сделано

	BOOL lbRc = apiShowWindow(hWnd, swShow);
	if (!lbRc)
	{
		DWORD dwErr = GetLastError();
		if (dwErr == 0)
		{
			if ((IsWindowVisible(hWnd) == FALSE) == (swShow == SW_HIDE))
				lbRc = TRUE;
			else
				dwErr = 5; // попробовать через сервер
		}

		if (dwErr == 5 /*E_access*/)
		{
			//PostConsoleMessage(hWnd, WM_SHOWWINDOW, SW_SHOWNA, 0);
			CESERVER_REQ in;
			ExecutePrepareCmd(&in, CECMD_POSTCONMSG, sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_POSTMSG));
			// Собственно, аргументы
			in.Msg.bPost = TRUE;
			in.Msg.hWnd = hWnd;
			in.Msg.nMsg = WM_SHOWWINDOW;
			in.Msg.wParam = swShow; //SW_SHOWNA;
			in.Msg.lParam = 0;

			CESERVER_REQ *pOut = ExecuteSrvCmd(GetServerPID(), &in, ghWnd);
			if (pOut) ExecuteFreeResult(pOut);
			lbRc = TRUE;
		}
		else if (!lbRc)
		{
			wchar_t szClass[64], szMessage[255];
			if (!GetClassName(hWnd, szClass, 63)) wsprintf(szClass, L"0x%08X", (DWORD)hWnd); else szClass[63] = 0;
			wsprintf(szMessage, L"Can't %s %s window!", 
				(swShow == SW_HIDE) ? L"hide" : L"show",
				szClass);
			DisplayLastError(szMessage, dwErr);
		}
	}
	return lbRc;
}

BOOL CRealConsole::SetOtherWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
	BOOL lbRc = SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
	if (!lbRc)
	{
		DWORD dwErr = GetLastError();
		if (dwErr == 5 /*E_access*/)
		{
			CESERVER_REQ in;
			ExecutePrepareCmd(&in, CECMD_SETWINDOWPOS, sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_SETWINDOWPOS));
			// Собственно, аргументы
			in.SetWndPos.hWnd = hWnd;
			in.SetWndPos.hWndInsertAfter = hWndInsertAfter;
			in.SetWndPos.X = X;
			in.SetWndPos.Y = Y;
			in.SetWndPos.cx = cx;
			in.SetWndPos.cy = cy;
			in.SetWndPos.uFlags = uFlags;

			CESERVER_REQ *pOut = ExecuteSrvCmd(GetServerPID(), &in, ghWnd);
			if (pOut) ExecuteFreeResult(pOut);
			lbRc = TRUE;
		}
		else
		{
			wchar_t szClass[64], szMessage[128];
			if (!GetClassName(hWnd, szClass, 63)) wsprintf(szClass, L"0x%08X", (DWORD)hWnd); else szClass[63] = 0;
			wsprintf(szMessage, L"SetWindowPos(%s) failed!", szClass);
			DisplayLastError(szMessage, dwErr);
		}
	}
	return lbRc;
}

BOOL CRealConsole::SetOtherWindowRgn(HWND hWnd, int nRects, LPRECT prcRects, BOOL bRedraw)
{
	BOOL lbRc = FALSE;
	CESERVER_REQ in;
	ExecutePrepareCmd(&in, CECMD_SETWINDOWRGN, sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_SETWINDOWRGN));
	// Собственно, аргументы
	in.SetWndRgn.hWnd = hWnd;
	if (nRects <= 0 || !prcRects)
	{
		_ASSERTE(nRects==0 || nRects==-1); // -1 means reset rgn and hide window
		in.SetWndRgn.nRectCount = nRects;
		in.SetWndRgn.bRedraw = bRedraw;		
	}
	else
	{
		in.SetWndRgn.nRectCount = nRects;
		in.SetWndRgn.bRedraw = bRedraw;
		memmove(in.SetWndRgn.rcRects, prcRects, nRects*sizeof(RECT));
	}

	CESERVER_REQ *pOut = ExecuteSrvCmd(GetServerPID(), &in, ghWnd);
	if (pOut) ExecuteFreeResult(pOut);
	lbRc = TRUE;
	
	return lbRc;
}

void CRealConsole::ShowHideViews(BOOL abShow)
{
	// т.к. apiShowWindow обломается, если окно создано от имени другого пользователя (или Run as admin)
	// то скрытие и отображение окна делаем другим способом
	HWND hPic = isPictureView();
	if (hPic)
	{
		if (abShow)
		{
			if (mb_PicViewWasHidden && !IsWindowVisible(hPic))
				ShowOtherWindow(hPic, SW_SHOWNA);
			mb_PicViewWasHidden = FALSE;

		}
		else
		{
			mb_PicViewWasHidden = TRUE;
			ShowOtherWindow(hPic, SW_HIDE);
		}
	}

	for (int p = 0; p <= 1; p++)
	{
		const PanelViewInit* pv = mp_VCon->GetPanelView(p==0);
		if (pv)
		{
			if (abShow)
			{
				if (pv->bVisible && !IsWindowVisible(pv->hWnd))
					ShowOtherWindow(pv->hWnd, SW_SHOWNA);

			}
			else
			{
				if (IsWindowVisible(pv->hWnd))
					ShowOtherWindow(pv->hWnd, SW_HIDE);
			}
		}
	}
}

void CRealConsole::OnActivate(int nNewNum, int nOldNum)
{
    if (!this)
        return;

    _ASSERTE(isActive());

    // Чтобы можно было найти хэндл окна по хэндлу консоли
    SetWindowLongPtr(ghWnd, GWLP_USERDATA, (LONG_PTR)hConWnd);
    ghConWnd = hConWnd;

	// Чтобы все в одном месте было
	OnGuiFocused(TRUE);
	//if (mp_ConsoleInfo) {
	//	PRAGMA_ERROR("Смену флажка нужно делать через команду сервера");
	//	mp_ConsoleInfo->bConsoleActive = TRUE;
	//}

    //if (mh_MonitorThread) SetThreadPriority(mh_MonitorThread, THREAD_PRIORITY_ABOVE_NORMAL);

    if ((gSet.isMonitorConsoleLang & 2) == 2) // Один Layout на все консоли
        SwitchKeyboardLayout(INPUTLANGCHANGE_SYSCHARSET,gConEmu.GetActiveKeyboardLayout());
    else if (con.dwKeybLayout && (gSet.isMonitorConsoleLang & 1) == 1) // Следить за Layout'ом в консоли
        gConEmu.SwitchKeyboardLayout(con.dwKeybLayout);

    WARNING("Не работало обновление заголовка");
    gConEmu.UpdateTitle(/*TitleFull*/); //100624 - сам перечитает

    UpdateScrollInfo();

    gConEmu.mp_TabBar->OnConsoleActivated(nNewNum+1/*, isBufferHeight()*/);
    gConEmu.mp_TabBar->Update();

    gConEmu.OnBufferHeight(); //con.bBufferHeight);

    gConEmu.UpdateProcessDisplay(TRUE);

	gSet.NeedBackgroundUpdate();

	ShowHideViews(TRUE);
	//HWND hPic = isPictureView();
	//if (hPic && mb_PicViewWasHidden) {
	//	if (!IsWindowVisible(hPic)) {
	//		if (!apiShowWindow(hPic, SW_SHOWNA)) {
	//			DisplayLastError(L"Can't show PictireView window!");
	//		}
	//	}
	//}
	//mb_PicViewWasHidden = FALSE;

	if (ghOpWnd && isActive())
		gSet.UpdateConsoleMode(con.m_dwConsoleMode);

	if (isActive())
	{
		gConEmu.OnSetCursor(-1,-1);
		gConEmu.UpdateWindowRgn();
	}
}

void CRealConsole::OnDeactivate(int nNewNum)
{
    if (!this) return;

	ShowHideViews(FALSE);
	//HWND hPic = isPictureView();
	//if (hPic && IsWindowVisible(hPic)) {
	//    mb_PicViewWasHidden = TRUE;
	//	if (!apiShowWindow(hPic, SW_HIDE)) {
	//		DisplayLastError(L"Can't hide PictuteView window!");
	//	}
	//}

    if (con.m_sel.dwFlags & CONSOLE_MOUSE_SELECTION) {
		con.m_sel.dwFlags &= ~CONSOLE_MOUSE_SELECTION;
		//ReleaseCapture();
	}

	// Чтобы все в одном месте было
	OnGuiFocused(FALSE);
	//if (mp_ConsoleInfo) {
	//	PRAGMA_ERROR("Смену флажка нужно делать через команду сервера");
	//	mp_ConsoleInfo->bConsoleActive = FALSE;
	//}

    //if (mh_MonitorThread) SetThreadPriority(mh_MonitorThread, THREAD_PRIORITY_NORMAL);
}

void CRealConsole::OnGuiFocused(BOOL abFocus)
{
	if (!this) return;

	// Если FALSE - сервер увеличивает интервал опроса консоли (GUI теряет фокус)
	BOOL lbThaw = abFocus || !gSet.isSleepInBackground;

	// Разрешит "заморозку" серверной нити и обновит hdr.bConsoleActive в мэппинге
	UpdateServerActive(lbThaw);
}

void CRealConsole::UpdateServerActive(BOOL abThaw)
{
	if (!this) return;

	BOOL fSuccess = FALSE;

	if (m_ConsoleMap.IsValid() && ms_ConEmuC_Pipe[0]) {
		BOOL lbNeedChange = FALSE;
		BOOL lbActive = isActive();

		if (m_ConsoleMap.Ptr()->bConsoleActive == lbActive && m_ConsoleMap.Ptr()->bThawRefreshThread == abThaw)
			lbNeedChange = FALSE;
		else
			lbNeedChange = TRUE;

		if (lbNeedChange) {
			int nInSize = sizeof(CESERVER_REQ_HDR)+sizeof(DWORD)*2;
			DWORD dwRead = 0;
			CESERVER_REQ lIn = {{nInSize}};
			lIn.dwData[0] = lbActive;
			lIn.dwData[1] = abThaw;
			
			ExecutePrepareCmd(&lIn, CECMD_ONACTIVATION, lIn.hdr.cbSize);

			fSuccess = CallNamedPipe(ms_ConEmuC_Pipe, &lIn, lIn.hdr.cbSize, &lIn, lIn.hdr.cbSize, &dwRead, 500);
		}
	}
}

// По переданному CONSOLE_SCREEN_BUFFER_INFO определяет, включена ли прокрутка
BOOL CRealConsole::BufferHeightTurnedOn(CONSOLE_SCREEN_BUFFER_INFO* psbi)
{
    BOOL lbTurnedOn = FALSE;
    TODO("!!! Скорректировать");

    if (psbi->dwSize.Y == (psbi->srWindow.Bottom - psbi->srWindow.Top + 1)) {
        // высота окна == высоте буфера, 
        lbTurnedOn = FALSE;
    } else if (con.m_sbi.dwSize.Y < (con.m_sbi.srWindow.Bottom-con.m_sbi.srWindow.Top+10)) {
        // Высота окна примерно равна высоте буфера
        lbTurnedOn = FALSE;
    } else if (con.m_sbi.dwSize.Y>(con.m_sbi.srWindow.Bottom-con.m_sbi.srWindow.Top+10)) {
        // Высота буфера 'намного' больше высоты окна
        lbTurnedOn = TRUE;
    }

    // однако, если высота слишком велика для отображения в GUI окне - нужно включить BufferHeight
    if (!lbTurnedOn) {
        //TODO: однако, если высота слишком велика для отображения в GUI окне - нужно включить BufferHeight
    }

    return lbTurnedOn;
}

void CRealConsole::UpdateScrollInfo()
{
    if (!isActive())
        return;

    if (!gConEmu.isMainThread()) {
        return;
    }

	// Не нужно? само спрячется
	if (!con.bBufferHeight) {
		return;
	}

    TODO("Как-то кэшировать нужно вызовы что-ли... и SetScrollInfo можно бы перенести в m_Back");
	static SHORT nLastHeight = 0, nLastVisible = 0, nLastTop = 0;

	if (nLastHeight == con.m_sbi.dwSize.Y
		&& nLastVisible == (con.m_sbi.srWindow.Bottom - con.m_sbi.srWindow.Top + 1)
		&& nLastTop == con.m_sbi.srWindow.Top)
		return; // не менялось

	nLastHeight = con.m_sbi.dwSize.Y;
	nLastVisible = (con.m_sbi.srWindow.Bottom - con.m_sbi.srWindow.Top + 1);
	nLastTop = con.m_sbi.srWindow.Top;

    int nCurPos = 0;
    //BOOL lbScrollRc = FALSE;
    SCROLLINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cbSize = sizeof(si);
    si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE; // | SIF_TRACKPOS;
	si.nPage = nLastVisible - 1;
	si.nPos = nLastTop;
	si.nMin = 0;
	si.nMax = nLastHeight;

	//// Если режим "BufferHeight" включен - получить из консольного окна текущее состояние полосы прокрутки
	//if (con.bBufferHeight) {
	//    lbScrollRc = GetScrollInfo(hConWnd, SB_VERT, &si);
	//} else {
	//    // Сбросываем параметры так, чтобы полоса не отображалась (все на 0)
	//}

    TODO("Нужно при необходимости 'всплыть' полосу прокрутки");
    nCurPos = SetScrollInfo(gConEmu.m_Back->mh_WndScroll, SB_VERT, &si, true);
}

// По con.m_sbi проверяет, включена ли прокрутка
BOOL CRealConsole::CheckBufferSize()
{
    bool lbForceUpdate = false;

    if (!this)
        return false;
    if (mb_BuferModeChangeLocked)
        return false;


    //if (con.m_sbi.dwSize.X>(con.m_sbi.srWindow.Right-con.m_sbi.srWindow.Left+1)) {
    //  DEBUGLOGFILE("Wrong screen buffer width\n");
    //  // Окошко консоли почему-то схлопнулось по горизонтали
    //  WARNING("пока убрал для теста");
    //  //MOVEWINDOW(hConWnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 1);
    //} else {

    // BufferHeight может меняться и из плагина, во время работы фара...
    BOOL lbTurnedOn = BufferHeightTurnedOn(&con.m_sbi);

    if ( !lbTurnedOn && con.bBufferHeight )
    {
        // может быть консольная программа увеличила буфер самостоятельно?
        // TODO: отключить прокрутку!!!
        SetBufferHeightMode(FALSE);

        UpdateScrollInfo();

        lbForceUpdate = true;
    } 
    else if ( lbTurnedOn && !con.bBufferHeight )
    {
        SetBufferHeightMode(TRUE);

        UpdateScrollInfo();

        lbForceUpdate = true;
    }

    //TODO: А если высота буфера вдруг сменилась из самой консольной программы?

    //if ((BufferHeight == 0) && (con.m_sbi.dwSize.Y>(con.m_sbi.srWindow.Bottom-con.m_sbi.srWindow.Top+1))) {
    //  TODO("это может быть консольная программа увеличила буфер самостоятельно!")
    //      DEBUGLOGFILE("Wrong screen buffer height\n");
    //  // Окошко консоли почему-то схлопнулось по вертикали
    //  WARNING("пока убрал для теста");
    //  //MOVEWINDOW(hConWnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 1);
    //}

    //TODO: Можно бы перенести в ConEmuC, если нужно будет
    //// При выходе из FAR -> CMD с BufferHeight - смена QuickEdit режима
    //DWORD mode = 0;
    //BOOL lb = FALSE;
    //if (BufferHeight) {
    //  //TODO: похоже, что для BufferHeight это вызывается постоянно?
    //  //lb = GetConsoleMode(hConIn(), &mode);
    //  mode = GetConsoleMode();

    //  if (con.m_sbi.dwSize.Y>(con.m_sbi.srWindow.Bottom-con.m_sbi.srWindow.Top+1)) {
    //      // Буфер больше высоты окна
    //      mode |= ENABLE_QUICK_EDIT_MODE|ENABLE_INSERT_MODE|ENABLE_EXTENDED_FLAGS;
    //  } else {
    //      // Буфер равен высоте окна (значит ФАР запустился)
    //      mode &= ~(ENABLE_QUICK_EDIT_MODE|ENABLE_INSERT_MODE);
    //      mode |= ENABLE_EXTENDED_FLAGS;
    //  }

    //  TODO("SetConsoleMode");
    //  //lb = SetConsoleMode(hConIn(), mode);
    //}

    return lbForceUpdate;
}

//// Должен вернуть: HIWORD==newBufferHeight, LOWORD=newBufferWidth
//LRESULT CRealConsole::OnConEmuCmd(BOOL abStarted, DWORD anConEmuC_PID)
//{
//  WORD newBufferHeight = TextHeight();
//  WORD newBufferWidth  = TextWidth();
//
//  if (abStarted) {
//      Process Add(anConEmuC_PID);
//
//      BufferHeight = TextHeight(); TODO("нужно вернуть реальный требуемый размер...");
//  } else {
//      Process Delete(anConEmuC_PID);
//
//      if ((mn_ProgramStatus & CES_CMDACTIVE) == 0)
//          BufferHeight = 0;
//  }
//
//  return MAKELONG(newBufferWidth,newBufferHeight);
//}


void CRealConsole::SetTabs(ConEmuTab* tabs, int tabsCount)
{
#ifdef _DEBUG
	wchar_t szDbg[128];
	wsprintf(szDbg, L"CRealConsole::SetTabs.  ItemCount=%i, PrevItemCount=%i\n", tabsCount, mn_tabsCount);
	DEBUGSTRTABS(szDbg);
#endif

    ConEmuTab* lpTmpTabs = NULL;

	const int nMaxTabName = countof(tabs->Name);
    
	// Табы нужно проверить и подготовить
    int nActiveTab = 0, i;
    if (tabs && tabsCount) {
		_ASSERTE(tabs->Type>1 || !tabs->Modified);

        //int nNewSize = sizeof(ConEmuTab)*tabsCount;
        //lpNewTabs = (ConEmuTab*)Alloc(nNewSize, 1);
        //if (!lpNewTabs)
        //    return;
        //memmove ( lpNewTabs, tabs, nNewSize );

        // иначе вместо "Panels" будет то что в заголовке консоли. Например "edit - doc1.doc"
		// это например, в процессе переключения закладок
        if (tabsCount > 1 && tabs[0].Type == 1/*WTYPE_PANELS*/ && tabs[0].Current)
            tabs[0].Name[0] = 0;

        if (tabsCount == 1) // принудительно. вдруг флажок не установлен
            tabs[0].Current = 1;
        
		// найти активную закладку
        for (i=(tabsCount-1); i>=0; i--) {
            if (tabs[i].Current) {
                nActiveTab = i; break;
            }
        }

        #ifdef _DEBUG
		// отладочно: имена закладок (редакторов/вьюверов) должны быть заданы!
        for (i=1; i<tabsCount; i++) {
            if (tabs[i].Name[0] == 0) {
                _ASSERTE(tabs[i].Name[0]!=0);
                //wcscpy(tabs[i].Name, L"ConEmu");
            }
        }
        #endif


    } else if (tabsCount == 1 && tabs == NULL) {
        lpTmpTabs = (ConEmuTab*)Alloc(sizeof(ConEmuTab),1);
        if (!lpTmpTabs)
            return;
		tabs = lpTmpTabs;
        tabs->Current = 1;
        tabs->Type = 1;
    }
	// Если запущено под "администратором"
	if (tabsCount && isAdministrator() && (gSet.bAdminShield || gSet.szAdminTitleSuffix[0])) {
		// В идеале - иконкой на закладке (если пользователь это выбрал)
		if (gSet.bAdminShield) {
			for (i=0; i<tabsCount; i++) {
				tabs[i].Type |= 0x100;
			}
		} else {
			// Иначе - суффиксом (если он задан)
			int nAddLen = lstrlen(gSet.szAdminTitleSuffix) + 1;
			for (i=0; i<tabsCount; i++) {
				if (tabs[i].Name[0]) {
					// Если есть место
					if (lstrlen(tabs[i].Name) < (int)(nMaxTabName + nAddLen))
						lstrcat(tabs[i].Name, gSet.szAdminTitleSuffix);
				}
			}
		}
	}

	if (tabsCount != mn_tabsCount)
		mb_TabsWasChanged = TRUE;
    
	MSectionLock SC;
	if (tabsCount > mn_MaxTabs) {
		SC.Lock(&msc_Tabs, TRUE);

		mn_tabsCount = 0; Free(mp_tabs); mp_tabs = NULL;
		mn_MaxTabs = tabsCount*2+10;
		mp_tabs = (ConEmuTab*)Alloc(mn_MaxTabs,sizeof(ConEmuTab));
		mb_TabsWasChanged = TRUE;

	} else {
		SC.Lock(&msc_Tabs, FALSE);
	}

	for (int i = 0; i < tabsCount; i++)
	{
		if (!mb_TabsWasChanged) {
			if (mp_tabs[i].Current != tabs[i].Current
				|| mp_tabs[i].Type != tabs[i].Type
				|| mp_tabs[i].Modified != tabs[i].Modified
				)
				mb_TabsWasChanged = TRUE;
			else if (wcscmp(mp_tabs[i].Name, tabs[i].Name)!=0)
				mb_TabsWasChanged = TRUE;
		}
		mp_tabs[i] = tabs[i];
	}
	mn_tabsCount = tabsCount; mn_ActiveTab = nActiveTab;
    
	SC.Unlock(); // больше не нужно

	//if (mb_TabsWasChanged && mp_tabs && mn_tabsCount) {
	//    CheckPanelTitle();
	//    CheckFarStates();
	//    FindPanels();
	//}
    
    // Передернуть gConEmu.mp_TabBar->..
	if (gConEmu.isValid(mp_VCon)) { // Во время создания консоли она еще не добавлена в список...
		// На время появления автотабов - отключалось
		gConEmu.mp_TabBar->SetRedraw(TRUE);
        gConEmu.mp_TabBar->Update();
	}
}

// Если такого таба нет - pTab НЕ ОБНУЛЯТЬ!!!
BOOL CRealConsole::GetTab(int tabIdx, /*OUT*/ ConEmuTab* pTab)
{
    if (!this)
        return FALSE;

	if (!mp_tabs || tabIdx<0 || tabIdx>=mn_tabsCount) {
		// На всякий случай, даже если табы не инициализированы, а просят первый -
		// вернем просто заголовок консоли
		if (tabIdx == 0) {
			pTab->Pos = 0; pTab->Current = 1; pTab->Type = 1; pTab->Modified = 0;
            int nMaxLen = max(countof(TitleFull) , countof(pTab->Name));
            lstrcpyn(pTab->Name, TitleFull, nMaxLen);
			return TRUE;
		}
        return FALSE;
	}
	// На время выполнения DOS-команд - только один таб
	if ((mn_ProgramStatus & CES_FARACTIVE) == 0 && tabIdx > 0)
		return FALSE;
    
    memmove(pTab, mp_tabs+tabIdx, sizeof(ConEmuTab));
    if (((mn_tabsCount == 1) || (mn_ProgramStatus & CES_FARACTIVE) == 0) && pTab->Type == 1) {
        if (TitleFull[0]) { // Если панель единственная - точно показываем заголовок консоли
            int nMaxLen = max(countof(TitleFull) , countof(pTab->Name));
            lstrcpyn(pTab->Name, TitleFull, nMaxLen);
        }
    }
    if (pTab->Name[0] == 0) {
        if (ms_PanelTitle[0]) { // скорее всего - это Panels?
        	// 03.09.2009 Maks -> min
            int nMaxLen = min(countof(ms_PanelTitle) , countof(pTab->Name));
            lstrcpyn(pTab->Name, ms_PanelTitle, nMaxLen);
			if (isAdministrator() && (gSet.bAdminShield || gSet.szAdminTitleSuffix[0])) {
				if (gSet.bAdminShield) {
					pTab->Type |= 0x100;
				} else {
					if (lstrlen(ms_PanelTitle) < (int)(countof(pTab->Name) + lstrlen(gSet.szAdminTitleSuffix) + 1))
						lstrcat(pTab->Name, gSet.szAdminTitleSuffix);
				}
			}
        } else if (mn_tabsCount == 1 && TitleFull[0]) { // Если панель единственная - точно показываем заголовок консоли
        	// 03.09.2009 Maks -> min
            int nMaxLen = min(countof(TitleFull) , countof(pTab->Name));
            lstrcpyn(pTab->Name, TitleFull, nMaxLen);
        }
    }
	if (tabIdx == 0 && isAdministrator() && gSet.bAdminShield) {
		pTab->Type |= 0x100;
	}
    wchar_t* pszAmp = pTab->Name;
    int nCurLen = lstrlenW(pTab->Name), nMaxLen = countof(pTab->Name)-1;
    while ((pszAmp = wcschr(pszAmp, L'&')) != NULL) {
        if (nCurLen >= (nMaxLen - 2)) {
            *pszAmp = L'_';
            pszAmp ++;
        } else {
			size_t nMove = nCurLen-(pszAmp-pTab->Name);
            wmemmove_s(pszAmp+1, nMove, pszAmp, nMove);
            nCurLen ++;
            pszAmp += 2;
        }
    }
    return TRUE;
}

int CRealConsole::GetTabCount()
{
    if (!this)
        return 0;

    if ((mn_ProgramStatus & CES_FARACTIVE) == 0)
    	return 1; // На время выполнения команд - ТОЛЬКО одна закладка

    return max(mn_tabsCount,1);
}

int CRealConsole::GetModifiedEditors()
{
	int nEditors = 0;
	ConEmuTab tab;

	for (int j = 0; TRUE; j++) {
		if (!GetTab(j, &tab))
			break;
		if (tab.Modified)
			nEditors ++;
	}

	return nEditors;
}

void CRealConsole::CheckPanelTitle()
{
#ifdef _DEBUG
	if (con.bDebugLocked)
		return;
#endif

    if (mp_tabs && mn_tabsCount) {
        if ((mn_ActiveTab >= 0 && mn_ActiveTab < mn_tabsCount) || mn_tabsCount == 1) {
            WCHAR szPanelTitle[CONEMUTABMAX];
            if (!GetWindowText(hConWnd, szPanelTitle, countof(ms_PanelTitle)-1))
                ms_PanelTitle[0] = 0;
            else if (szPanelTitle[0] == L'{' || szPanelTitle[0] == L'(')
                lstrcpy(ms_PanelTitle, szPanelTitle);
        }
    }
}

DWORD CRealConsole::CanActivateFarWindow(int anWndIndex)
{
    if (!this)
        return 0;
        
    DWORD dwPID = GetFarPID();
    if (!dwPID) {
        return -1; // консоль активируется без разбора по вкладкам (фара нет)
        //if (anWndIndex == mn_ActiveTab)
        //return 0;
    }

    // Если есть меню: ucBoxDblDownRight ucBoxDblHorz ucBoxDblHorz ucBoxDblHorz (первая или вторая строка консоли) - выходим
    // Если идет процесс (в заголовке консоли {n%}) - выходим
    // Если висит диалог - выходим (диалог обработает сам плагин)
    
    if (anWndIndex<0 || anWndIndex>=mn_tabsCount)
        return 0;

    // Добавил такую проверочку. По идее, у нас всегда должен быть актуальный номер текущего окна.
    if (mn_ActiveTab == anWndIndex)
        return (DWORD)-1; // Нужное окно уже выделено, лучше не дергаться...

    if (isPictureView(TRUE))
        return 0; // При наличии PictureView переключиться на другой таб этой консоли не получится

    if (!GetWindowText(hConWnd, TitleCmp, countof(TitleCmp)-2))
        TitleCmp[0] = 0;
	// Прогресс уже определился в другом месте
	if (GetProgress(NULL)>=0)
		return 0; // Идет копирование или какая-то другая операция
    //// Копирование в FAR: "{33%}..."
    ////2009-06-02: PPCBrowser показывает копирование так: "(33% 00:02:20)..."
    //if ((TitleCmp[0] == L'{' || TitleCmp[0] == L'(')
    //    && isDigit(TitleCmp[1]) &&
    //    ((TitleCmp[2] == L'%' /*&& TitleCmp[3] == L'}'*/) ||
    //     (isDigit(TitleCmp[2]) && TitleCmp[3] == L'%' /*&& TitleCmp[4] == L'}'*/) ||
    //     (isDigit(TitleCmp[2]) && isDigit(TitleCmp[3]) && TitleCmp[4] == L'%' /*&& TitleCmp[5] == L'}'*/))
    //   )
    //{
    //    // Идет копирование
    //    return 0;
    //}
    if (!con.pConChar || !con.nTextWidth || con.nTextHeight<2)
        return 0; // консоль не инициализирована, ловить нечего
    BOOL lbMenuActive = FALSE;
    if (mp_tabs && mn_ActiveTab >= 0 && mn_ActiveTab < mn_tabsCount)
    {   // Меню может быть только в панелях
        if (mp_tabs[mn_ActiveTab].Type == 1/*WTYPE_PANELS*/)
        {
            MSectionLock cs; cs.Lock(&csCON);
            if (con.pConChar) {
                if (con.pConChar[0] == L'R' || con.pConChar[0] == L'P') {
                    // Запись макроса. Запретим наверное переключаться?
                    lbMenuActive = TRUE;
                }
                else if (con.pConChar[0] == L' ' && con.pConChar[con.nTextWidth] == ucBoxDblVert) {
                    lbMenuActive = TRUE;
                } else if (con.pConChar[0] == L' ' && (con.pConChar[con.nTextWidth] == ucBoxDblDownRight || 
                    (con.pConChar[con.nTextWidth] == '[' 
                     && (con.pConChar[con.nTextWidth+1] >= L'0' && con.pConChar[con.nTextWidth+1] <= L'9'))))
                {
                    // Строка меню ВСЕГДА видна. Определим, активно ли меню.
                    for (int x=1; !lbMenuActive && x<con.nTextWidth; x++) {
                        if (con.pConAttr[x] != con.pConAttr[0]) // неактивное меню - не подсвечивается
                            lbMenuActive = TRUE;
                    }
                } else {
                    // Если строка меню ВСЕГДА не видна, а только всплывает
                    wchar_t* pszLine = con.pConChar + con.nTextWidth;
                    for (int x=1; !lbMenuActive && x<(con.nTextWidth-10); x++)
                    {
                        if (pszLine[x] == ucBoxDblDownRight && pszLine[x+1] == ucBoxDblHorz)
                            lbMenuActive = TRUE;
                    }
                }
            }
            cs.Unlock();
        }
    }
    
    // Если строка меню отображается всегда:
    //  0-строка начинается с "  " или с "R   " если идет запись макроса
    //  1-я строка ucBoxDblDownRight ucBoxDblHorz ucBoxDblHorz или "[0+1]" ucBoxDblHorz ucBoxDblHorz
    //  2-я строка ucBoxDblVert
    // Наличие активного меню определяем по количеству цветов в первой строке.
    // Неактивное меню отображается всегда одним цветом - в активном подсвечиваются хоткеи и выбранный пункт
    
    if (lbMenuActive)
        return 0;
        
    return dwPID;
}

BOOL CRealConsole::ActivateFarWindow(int anWndIndex)
{
    if (!this)
        return FALSE;
    

    DWORD dwPID = CanActivateFarWindow(anWndIndex);
	if (!dwPID) {
        return FALSE;
	} else if (dwPID == (DWORD)-1) {
        return TRUE; // Нужное окно уже выделено, лучше не дергаться...
	}


    BOOL lbRc = FALSE;

    CConEmuPipe pipe(dwPID, 100);
    if (pipe.Init(_T("CRealConsole::ActivateFarWindow")))
    {
		DWORD nData[2] = {anWndIndex,0};
		// Если в панелях висит QSearch - его нужно предварительно "снять"
		if (!mn_ActiveTab && (mp_Rgn->GetFlags() & FR_QSEARCH))
			nData[1] = TRUE;

        if (pipe.Execute(CMD_SETWINDOW, nData, 8))
        {
			WARNING("CMD_SETWINDOW по таймауту возвращает последнее считанное положение окон (gpTabs).");
			// То есть если переключение окна выполняется дольше 2х сек - возвратится предыдущее состояние

            DWORD cbBytesRead=0;
            //DWORD tabCount = 0, nInMacro = 0, nTemp = 0, nFromMainThread = 0;
            ConEmuTab* tabs = NULL;
			CESERVER_REQ_CONEMUTAB TabHdr;
			DWORD nHdrSize = sizeof(CESERVER_REQ_CONEMUTAB) - sizeof(TabHdr.tabs);
			//if (pipe.Read(&tabCount, sizeof(DWORD), &cbBytesRead) &&
			//	pipe.Read(&nInMacro, sizeof(DWORD), &nTemp) &&
			//	pipe.Read(&nFromMainThread, sizeof(DWORD), &nTemp)
			//	)
			if (pipe.Read(&TabHdr, nHdrSize, &cbBytesRead))
			{
                tabs = (ConEmuTab*)pipe.GetPtr(&cbBytesRead);
                _ASSERTE(cbBytesRead==(TabHdr.nTabCount*sizeof(ConEmuTab)));
                if (cbBytesRead == (TabHdr.nTabCount*sizeof(ConEmuTab))) {
                    SetTabs(tabs, TabHdr.nTabCount);
                    lbRc = TRUE;
                }
				MCHKHEAP;
            }

            pipe.Close();
        }
    }

    return lbRc;
}

BOOL CRealConsole::IsConsoleThread()
{
    if (!this) return FALSE;

    DWORD dwCurThreadId = GetCurrentThreadId();
    return dwCurThreadId == mn_MonitorThreadID;
}

void CRealConsole::SetForceRead()
{
    SetEvent(mh_MonitorThreadEvent);
}

// Вызывается из TabBar->ConEmu
void CRealConsole::ChangeBufferHeightMode(BOOL abBufferHeight)
{
	if (abBufferHeight && gOSVer.dwMajorVersion == 6 && gOSVer.dwMinorVersion == 1)
	{
		// Win7 BUGBUG: Issue 192: падение Conhost при turn bufferheight ON
		// http://code.google.com/p/conemu-maximus5/issues/detail?id=192
		return;
		//const SHORT nMaxBuf = 600;
		//if (nNewBufHeightSize > nMaxBuf && isFar())
		//	nNewBufHeightSize = nMaxBuf;
	}

	_ASSERTE(!mb_BuferModeChangeLocked);
	BOOL lb = mb_BuferModeChangeLocked; mb_BuferModeChangeLocked = TRUE;
	con.bBufferHeight = abBufferHeight;
	// Если при запуске было "conemu.exe /bufferheight 0 ..."
	if (abBufferHeight /*&& !con.nBufferHeight*/) {
		// Если пользователь меняет высоту буфера в диалоге настроек
		con.nBufferHeight = gSet.DefaultBufferHeight;
		if (con.nBufferHeight<300) con.nBufferHeight = max(300,con.nTextHeight*2);
	}
	USHORT nNewBufHeightSize = abBufferHeight ? con.nBufferHeight : 0;
	SetConsoleSize(TextWidth(), TextHeight(), nNewBufHeightSize, CECMD_SETSIZESYNC);
	mb_BuferModeChangeLocked = lb;
}

void CRealConsole::SetBufferHeightMode(BOOL abBufferHeight, BOOL abLock/*=FALSE*/)
{
    if (mb_BuferModeChangeLocked) {
        if (!abLock) {
            //_ASSERT(FALSE);
            return;
        }
    }

    con.bBufferHeight = abBufferHeight;
    if (isActive())
        gConEmu.OnBufferHeight(); //abBufferHeight);
}

HANDLE CRealConsole::PrepareOutputFileCreate(wchar_t* pszFilePathName)
{
    wchar_t szTemp[200];
    HANDLE hFile = NULL;
    if (GetTempPath(200, szTemp)) {
        if (GetTempFileName(szTemp, L"CEM", 0, pszFilePathName)) {
            hFile = CreateFile(pszFilePathName, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
            if (hFile == INVALID_HANDLE_VALUE) {
                pszFilePathName[0] = 0;
                hFile = NULL;
            }
        }
    }
    return hFile;
}

BOOL CRealConsole::PrepareOutputFile(BOOL abUnicodeText, wchar_t* pszFilePathName)
{
    BOOL lbRc = FALSE;
	CESERVER_REQ_HDR In = {0};
	const CESERVER_REQ *pOut = NULL;
	DWORD cbRead = 0;
	MPipe<CESERVER_REQ_HDR,CESERVER_REQ> Pipe;

	ExecutePrepareCmd(&In, CECMD_GETOUTPUT, sizeof(CESERVER_REQ_HDR));

	Pipe.InitName(L"ConEmu", L"%s", ms_ConEmuC_Pipe, 0);
	if (!Pipe.Transact(&In, In.cbSize, &pOut, &cbRead)) {
		MBoxA(Pipe.GetErrorText());
		return FALSE;
	}

	//HANDLE hPipe = NULL; 
	//
	//CESERVER_REQ *pOut = NULL;
	//BYTE cbReadBuf[512];
	//BOOL fSuccess;
	//DWORD cbRead, dwMode, dwErr;
	//
	//// Try to open a named pipe; wait for it, if necessary. 
	//while (1) 
	//{ 
	//    hPipe = CreateFile( 
	//        ms_ConEmuC_Pipe,// pipe name 
	//        GENERIC_READ |  // read and write access 
	//        GENERIC_WRITE, 
	//        0,              // no sharing 
	//        NULL,           // default security attributes
	//        OPEN_EXISTING,  // opens existing pipe 
	//        0,              // default attributes 
	//        NULL);          // no template file 
	//
	//    // Break if the pipe handle is valid. 
	//    if (hPipe != INVALID_HANDLE_VALUE) 
	//        break; 
	//
	//    // Exit if an error other than ERROR_PIPE_BUSY occurs. 
	//    dwErr = GetLastError();
	//    if (dwErr != ERROR_PIPE_BUSY) 
	//    {
	//        TODO("Подождать, пока появится пайп с таким именем, но только пока жив mh_ConEmuC");
	//        dwErr = WaitForSingleObject(mh_ConEmuC, 100);
	//        if (dwErr == WAIT_OBJECT_0)
	//            return FALSE;
	//        continue;
	//        //DisplayLastError(L"Could not open pipe", dwErr);
	//        //return 0;
	//    }
	//
	//    // All pipe instances are busy, so wait for 1 second.
	//    if (!WaitNamedPipe(ms_ConEmuC_Pipe, 1000) ) 
	//    {
	//        dwErr = WaitForSingleObject(mh_ConEmuC, 100);
	//        if (dwErr == WAIT_OBJECT_0) {
	//            DEBUGSTRCMD(L" - FAILED!\n");
	//            return FALSE;
	//        }
	//        //DisplayLastError(L"WaitNamedPipe failed"); 
	//        //return 0;
	//    }
	//} 
	//
	//// The pipe connected; change to message-read mode. 
	//dwMode = PIPE_READMODE_MESSAGE; 
	//fSuccess = SetNamedPipeHandleState( 
	//    hPipe,    // pipe handle 
	//    &dwMode,  // new pipe mode 
	//    NULL,     // don't set maximum bytes 
	//    NULL);    // don't set maximum time 
	//if (!fSuccess) 
	//{
	//    DEBUGSTRCMD(L" - FAILED!\n");
	//    DisplayLastError(L"SetNamedPipeHandleState failed");
	//    CloseHandle(hPipe);
	//    return 0;
	//}
	//
	//ExecutePrepareCmd((CESERVER_REQ*)&in, CECMD_GETOUTPUT, sizeof(CESERVER_REQ_HDR));
	//
	//// Send a message to the pipe server and read the response. 
	//fSuccess = TransactNamedPipe( 
	//    hPipe,                  // pipe handle 
	//    &in,                    // message to server
	//    in.nSize,               // message length 
	//    cbReadBuf,              // buffer to receive reply
	//    sizeof(cbReadBuf),      // size of read buffer
	//    &cbRead,                // bytes read
	//    NULL);                  // not overlapped 
	//
	//if (!fSuccess && (GetLastError() != ERROR_MORE_DATA)) 
	//{
	//    DEBUGSTRCMD(L" - FAILED!\n");
	//    //DisplayLastError(L"TransactNamedPipe failed"); 
	//    CloseHandle(hPipe);
	//    HANDLE hFile = PrepareOutputFileCreate(pszFilePathName);
	//    if (hFile)
	//        CloseHandle(hFile);
	//    return (hFile!=NULL);
	//}
	//
	//// Пока не выделяя память, просто указатель на буфер
	//pOut = (CESERVER_REQ*)cbReadBuf;
	//if (pOut->hdr.nVersion != CESERVER_REQ_VER) {
	//    gConEmu.ShowOldCmdVersion(pOut->hdr.nCmd, pOut->hdr.nVersion, pOut->hdr.nSrcPID==GetServerPID() ? 1 : 0);
	//    CloseHandle(hPipe);
	//    return 0;
	//}
	//
	//int nAllSize = pOut->hdr.cbSize;
	//if (nAllSize==0) {
	//    DEBUGSTRCMD(L" - FAILED!\n");
	//    DisplayLastError(L"Empty data recieved from server", 0);
	//    CloseHandle(hPipe);
	//    return 0;
	//}
	//if (nAllSize > (int)sizeof(cbReadBuf))
	//{
	//    pOut = (CESERVER_REQ*)calloc(nAllSize,1);
	//    _ASSERTE(pOut!=NULL);
	//    memmove(pOut, cbReadBuf, cbRead);
	//    _ASSERTE(pOut->hdr.nVersion==CESERVER_REQ_VER);
	//
	//    LPBYTE ptrData = ((LPBYTE)pOut)+cbRead;
	//    nAllSize -= cbRead;
	//
	//    while(nAllSize>0)
	//    { 
	//        //_tprintf(TEXT("%s\n"), chReadBuf);
	//
	//        // Break if TransactNamedPipe or ReadFile is successful
	//        if(fSuccess)
	//            break;
	//
	//        // Read from the pipe if there is more data in the message.
	//        fSuccess = ReadFile( 
	//            hPipe,      // pipe handle 
	//            ptrData,    // buffer to receive reply 
	//            nAllSize,   // size of buffer 
	//            &cbRead,    // number of bytes read 
	//            NULL);      // not overlapped 
	//
	//        // Exit if an error other than ERROR_MORE_DATA occurs.
	//        if( !fSuccess && (GetLastError() != ERROR_MORE_DATA)) 
	//            break;
	//        ptrData += cbRead;
	//        nAllSize -= cbRead;
	//    }
	//
	//    TODO("Может возникнуть ASSERT, если консоль была закрыта в процессе чтения");
	//    _ASSERTE(nAllSize==0);
	//}
	//
	//CloseHandle(hPipe);

    // Теперь pOut содержит данные вывода : CESERVER_CONSAVE

    HANDLE hFile = PrepareOutputFileCreate(pszFilePathName);
    lbRc = (hFile != NULL);

    if (pOut->hdr.nVersion == CESERVER_REQ_VER) {
        const CESERVER_CONSAVE* pSave = (CESERVER_CONSAVE*)pOut;
        UINT nWidth = pSave->hdr.sbi.dwSize.X;
        UINT nHeight = pSave->hdr.sbi.dwSize.Y;
        const wchar_t* pwszCur = pSave->Data;
        const wchar_t* pwszEnd = (const wchar_t*)(((LPBYTE)pOut)+pOut->hdr.cbSize);

        if (pOut->hdr.nVersion == CESERVER_REQ_VER && nWidth && nHeight && (pwszCur < pwszEnd)) {
            DWORD dwWritten;
            char *pszAnsi = NULL;
            const wchar_t* pwszRn = NULL;
            if (!abUnicodeText) {
                pszAnsi = (char*)calloc(nWidth+1,1);
            } else {
                WORD dwTag = 0xFEFF; //BOM
                WriteFile(hFile, &dwTag, 2, &dwWritten, 0);
            }

            BOOL lbHeader = TRUE;

            for (UINT y = 0; y < nHeight && pwszCur < pwszEnd; y++)
            {
                UINT nCurLen = 0;
                pwszRn = pwszCur + nWidth - 1;
                while (pwszRn >= pwszCur && *pwszRn == L' ') {
                    //*pwszRn = 0;
					pwszRn --;
                }
                nCurLen = pwszRn - pwszCur + 1;
                if (nCurLen > 0 || !lbHeader) { // Первые N строк если они пустые - не показывать
                    if (lbHeader) {
                        lbHeader = FALSE;
                    } else if (nCurLen == 0) {
                        // Если ниже строк нет - больше ничего не писать
                        pwszRn = pwszCur + nWidth;
                        while (pwszRn < pwszEnd && *pwszRn == L' ') pwszRn ++;
                        if (pwszRn >= pwszEnd) break; // Заполненных строк больше нет
                    }
                    if (abUnicodeText) {
                        if (nCurLen>0)
                            WriteFile(hFile, pwszCur, nCurLen*2, &dwWritten, 0);
                        WriteFile(hFile, L"\r\n", 4, &dwWritten, 0);
                    } else {
                        nCurLen = WideCharToMultiByte(CP_ACP, 0, pwszCur, nCurLen, pszAnsi, nWidth, 0,0);
                        if (nCurLen>0)
                            WriteFile(hFile, pszAnsi, nCurLen, &dwWritten, 0);
                        WriteFile(hFile, "\r\n", 2, &dwWritten, 0);
                    }
                }
                pwszCur += nWidth;
            }
        }
    }

    if (hFile != NULL && hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    //if (pOut && (LPVOID)pOut != (LPVOID)cbReadBuf)
    //    free(pOut);
    return lbRc;
}

void CRealConsole::SwitchKeyboardLayout(WPARAM wParam,DWORD_PTR dwNewKeyboardLayout)
{
    if (!this) return;
    if (ms_ConEmuC_Pipe[0] == 0) return;
    if (!hConWnd) return;

	#ifdef _DEBUG
	WCHAR szMsg[255];
	wsprintf(szMsg, L"CRealConsole::SwitchKeyboardLayout(CP:%i, HKL:0x%08I64X)\n",
		wParam, (unsigned __int64)(DWORD_PTR)dwNewKeyboardLayout);
	DEBUGSTRLANG(szMsg);
	#endif

    if (gSet.isAdvLogging > 1)
    {
	    WCHAR szInfo[255];
	    wsprintf(szInfo, L"CRealConsole::SwitchKeyboardLayout(CP:%i, HKL:0x%08I64X)",
			wParam, (unsigned __int64)(DWORD_PTR)dwNewKeyboardLayout);
    	LogString(szInfo);
	}
	
	// Сразу запомнить новое значение, чтобы не циклиться
	con.dwKeybLayout = dwNewKeyboardLayout;

    // В FAR при XLat делается так:
    //PostConsoleMessageW(hFarWnd,WM_INPUTLANGCHANGEREQUEST, INPUTLANGCHANGE_FORWARD, 0);
    PostConsoleMessage(hConWnd, WM_INPUTLANGCHANGEREQUEST, wParam, (LPARAM)dwNewKeyboardLayout);
}

void CRealConsole::Paste()
{
    if (!this) return;
    if (!hConWnd) return;
    
#ifndef RCON_INTERNAL_PASTE
    // Можно так
    PostConsoleMessage(hConWnd, WM_COMMAND, SC_PASTE_SECRET, 0);
#endif

#ifdef RCON_INTERNAL_PASTE
    // А можно и через наш сервер
    if (!OpenClipboard(NULL)) {
        MBox(_T("Can't open PC clipboard"));
        return;
    }
    HGLOBAL hglb = GetClipboardData(CF_UNICODETEXT);
    if (!hglb) {
        CloseClipboard();
        MBox(_T("Can't get CF_UNICODETEXT"));
        return;
    }
    wchar_t* lptstr = (wchar_t*)GlobalLock(hglb);
    if (!lptstr) {
        CloseClipboard();
        MBox(_T("Can't lock CF_UNICODETEXT"));
        return;
    }
    // Теперь сформируем пакет
    size_t nLen = lstrlenW(lptstr);
    if (nLen>0) {
        if (nLen>127) {
            TCHAR szMsg[128]; wsprintf(szMsg, L"Clipboard length is %i chars!\nContinue?", nLen);
            if (MessageBox(ghWnd, szMsg, gConEmu.GetTitle(), MB_OKCANCEL) != IDOK) {
                GlobalUnlock(hglb);
                CloseClipboard();
                return;
            }
        }
        
        INPUT_RECORD r = {KEY_EVENT};
        
        // Отправить в консоль все символы из: lptstr
        while (*lptstr)
        {
            r.Event.KeyEvent.bKeyDown = TRUE;
            r.Event.KeyEvent.uChar.UnicodeChar = *lptstr;
            r.Event.KeyEvent.wRepeatCount = 1; //TODO("0-15 ? Specifies the repeat count for the current message. The value is the number of times the keystroke is autorepeated as a result of the user holding down the key. If the keystroke is held long enough, multiple messages are sent. However, the repeat count is not cumulative.");
            r.Event.KeyEvent.wVirtualKeyCode = 0;

            //r.Event.KeyEvent.wVirtualScanCode = ((DWORD)lParam & 0xFF0000) >> 16; // 16-23 - Specifies the scan code. The value depends on the OEM.
            // 24 - Specifies whether the key is an extended key, such as the right-hand ALT and CTRL keys that appear on an enhanced 101- or 102-key keyboard. The value is 1 if it is an extended key; otherwise, it is 0.
            // 29 - Specifies the context code. The value is 1 if the ALT key is held down while the key is pressed; otherwise, the value is 0.
            // 30 - Specifies the previous key state. The value is 1 if the key is down before the message is sent, or it is 0 if the key is up.
            // 31 - Specifies the transition state. The value is 1 if the key is being released, or it is 0 if the key is being pressed.
            //r.Event.KeyEvent.dwControlKeyState = 0;
            
            PostConsoleEvent(&r);

            // И сразу посылаем отпускание
            r.Event.KeyEvent.bKeyDown = FALSE;
            PostConsoleEvent(&r);
        }
    }
    GlobalUnlock(hglb);
    CloseClipboard();
#endif
}

bool CRealConsole::isConsoleClosing()
{
	if (!gConEmu.isValid(this))
		return true;

	if (m_ServerClosing.nServerPID
		&& m_ServerClosing.nServerPID == mn_ConEmuC_PID
		&& (GetTickCount() - m_ServerClosing.nRecieveTick) >= SERVERCLOSETIMEOUT)
	{
		// Видимо, сервер повис во время выхода? Но проверим, вдруг он все-таки успел завершиться?
		if (WaitForSingleObject(mh_ConEmuC, 0))
		{
			_ASSERTE(m_ServerClosing.nServerPID==0);
			TerminateProcess(mh_ConEmuC, 100);
		}
		return true;
	}

	if ((hConWnd == NULL) || mb_InCloseConsole)
		return true;
	return false;
}

void CRealConsole::CloseConsole()
{
    if (!this) return;
    _ASSERTE(!mb_ProcessRestarted);
	if (hConWnd)
	{
		if (gSet.isSafeFarClose)
		{
			BOOL lbExecuted = FALSE;
			DWORD nFarPID = GetFarPID(TRUE/*abPluginRequired*/);
			if (nFarPID && isAlive())
			{
				CConEmuPipe pipe(nFarPID, CONEMUREADYTIMEOUT);
				if (pipe.Init(_T("CRealConsole::CloseConsole"), TRUE))
				{
					mb_InCloseConsole = TRUE;
					
					//DWORD cbWritten=0;
					gConEmu.DebugStep(_T("ConEmu: ACTL_QUIT"));
					//lbExecuted = pipe.Execute(CMD_QUITFAR);

					LPCWSTR pszMacro = gSet.sSafeFarCloseMacro;
					if (!pszMacro || !*pszMacro)
					{
						pszMacro = L"@$while (Dialog||Editor||Viewer||Menu||Disks||MainMenu||UserMenu||Other||Help) $if (Editor) ShiftF10 $else Esc $end $end  Esc  $if (Shell) F10 $if (Dialog) Enter $end $Exit $end  F10";
					}

					lbExecuted = pipe.Execute(CMD_POSTMACRO, pszMacro, (wcslen(pszMacro)+1)*2);

					gConEmu.DebugStep(NULL);
				}
			    
				if (lbExecuted)
		    		return;
			}
		}

		mb_InCloseConsole = TRUE;
        PostConsoleMessage(hConWnd, WM_CLOSE, 0, 0);
        
	} else {
		m_Args.bDetached = FALSE;
		if (mp_VCon)
			gConEmu.OnVConTerminated(mp_VCon);
	}
}

uint CRealConsole::TextWidth()
{ 
    _ASSERTE(this!=NULL);
    if (!this) return MIN_CON_WIDTH;
    if (con.nChange2TextWidth!=-1 && con.nChange2TextWidth!=0)
        return con.nChange2TextWidth;
    return con.nTextWidth;
}

uint CRealConsole::TextHeight()
{ 
    _ASSERTE(this!=NULL);
    if (!this) return MIN_CON_HEIGHT;
    uint nRet = 0;
    if (con.nChange2TextHeight!=-1 && con.nChange2TextHeight!=0)
        nRet = con.nChange2TextHeight;
    else
        nRet = con.nTextHeight;
    if (nRet > 200) {
        _ASSERTE(nRet<=200);
    }
    return nRet;
}

uint CRealConsole::BufferHeight(uint nNewBufferHeight/*=0*/)
{
	uint nBufferHeight = 0;
	if (con.bBufferHeight) {
		if (nNewBufferHeight) {
			nBufferHeight = nNewBufferHeight;
			con.nBufferHeight = nNewBufferHeight;
		} else if (con.nBufferHeight) {
			nBufferHeight = con.nBufferHeight;
		} else if (con.DefaultBufferHeight) {
			nBufferHeight = con.DefaultBufferHeight;
		} else {
			nBufferHeight = gSet.DefaultBufferHeight;
		}
	} else {
		// После выхода из буферного режима сбрасываем запомненную высоту, чтобы
		// в следующий раз установить высоту из настроек (gSet.DefaultBufferHeight)
		_ASSERTE(nNewBufferHeight == 0);
		con.nBufferHeight = 0;
	}
	return nBufferHeight;
}

bool CRealConsole::isActive()
{
    if (!this) return false;
    if (!mp_VCon) return false;
    return gConEmu.isActive(mp_VCon);
}

bool CRealConsole::isVisible()
{
    if (!this) return false;
    if (!mp_VCon) return false;
    return gConEmu.isVisible(mp_VCon);
}

void CRealConsole::CheckFarStates()
{
#ifdef _DEBUG
	if (con.bDebugLocked)
		return;
#endif

	DWORD nLastState = mn_FarStatus;
	DWORD nNewState = (mn_FarStatus & (~CES_FARFLAGS));

	if (GetFarPID() == 0)
	{
		nNewState = 0;
	}
	else
	{

		// Сначала пытаемся проверить статус по закладкам: если к нам из плагина пришло,
		// что активен "viewer/editor" - то однозначно ставим CES_VIEWER/CES_EDITOR
		if (mp_tabs && mn_ActiveTab >= 0 && mn_ActiveTab < mn_tabsCount)
		{
			if (mp_tabs[mn_ActiveTab].Type != 1)
			{
				if (mp_tabs[mn_ActiveTab].Type == 2)
					nNewState |= CES_VIEWER;
				else if (mp_tabs[mn_ActiveTab].Type == 3)
					nNewState |= CES_EDITOR;
			}
		}

		// Если по закладкам ничего не определили - значит может быть либо панель, либо
		// "viewer/editor" но при отсутствии плагина в фаре
		if ((nNewState & CES_FARFLAGS) == 0)
		{
			// пытаемся определить "viewer/editor" по заголовку окна консоли
			if (wcsncmp(Title, ms_Editor, lstrlen(ms_Editor))==0 || wcsncmp(Title, ms_EditorRus, lstrlen(ms_EditorRus))==0)
				nNewState |= CES_EDITOR;
			else if (wcsncmp(Title, ms_Viewer, lstrlen(ms_Viewer))==0 || wcsncmp(Title, ms_ViewerRus, lstrlen(ms_ViewerRus))==0)
				nNewState |= CES_VIEWER;
			else if (isFilePanel(true))
				nNewState |= CES_FILEPANEL;
		}

		// Смысл в том, чтобы не сбросить флажок CES_MAYBEPANEL если в панелях открыт диалог
		// Флажок сбрасывается только в редактроре/вьювере
		if ((nNewState & (CES_EDITOR | CES_VIEWER)) != 0)
			nNewState &= ~(CES_MAYBEPANEL|CES_WASPROGRESS|CES_OPER_ERROR); // При переключении в редактор/вьювер - сбросить CES_MAYBEPANEL

		if ((nNewState & CES_FILEPANEL) == CES_FILEPANEL)
		{
			nNewState |= CES_MAYBEPANEL; // Попали в панель - поставим флажок
			nNewState &= ~(CES_WASPROGRESS|CES_OPER_ERROR); // Значит СЕЙЧАС процесс копирования не идет
		}

		if (mn_Progress >= 0 && mn_Progress <= 100)
		{
			if (mn_ConsoleProgress == mn_Progress)
			{
				// При извлечении прогресса из текста консоли - Warning ловить смысла нет
				mn_PreWarningProgress = -1;
				nNewState &= ~CES_OPER_ERROR;
				nNewState |= CES_WASPROGRESS; // Пометить статус, что прогресс был
			}
			else
			{
				mn_PreWarningProgress = mn_Progress;
				if ((nNewState & CES_MAYBEPANEL) == CES_MAYBEPANEL)
					nNewState |= CES_WASPROGRESS; // Пометить статус, что прогресс был
				nNewState &= ~CES_OPER_ERROR;
			}
		} else if ((nNewState & (CES_WASPROGRESS|CES_MAYBEPANEL)) == (CES_WASPROGRESS|CES_MAYBEPANEL)
			&& mn_PreWarningProgress != -1)
		{
			if (mn_LastWarnCheckTick == 0)
			{
				mn_LastWarnCheckTick = GetTickCount();
			}
			else if ((mn_FarStatus & CES_OPER_ERROR) == CES_OPER_ERROR)
			{
				// для удобства отладки - флаг ошибки уже установлен
				_ASSERTE((nNewState & CES_OPER_ERROR) == CES_OPER_ERROR);
				nNewState |= CES_OPER_ERROR;
			}
			else
			{
				DWORD nDelta = GetTickCount() - mn_LastWarnCheckTick;
				if (nDelta > CONSOLEPROGRESSWARNTIMEOUT)
				{
					nNewState |= CES_OPER_ERROR;
					//mn_LastWarnCheckTick = 0;
				}
			}
		}
	}

	if (mn_Progress == -1 && mn_PreWarningProgress != -1)
	{
		if ((nNewState & CES_WASPROGRESS) == 0)
		{
			mn_PreWarningProgress = -1; mn_LastWarnCheckTick = 0;
			gConEmu.UpdateProgress();
		}
		else if (isFilePanel(true))
		{
			nNewState &= ~(CES_OPER_ERROR|CES_WASPROGRESS);
			mn_PreWarningProgress = -1; mn_LastWarnCheckTick = 0;
			gConEmu.UpdateProgress();
		}
	}

	if (nNewState != nLastState)
	{
		mn_FarStatus = nNewState;
		gConEmu.UpdateProcessDisplay(FALSE);
	}
}

short CRealConsole::CheckProgressInConsole(const wchar_t* pszCurLine)
{
	// Обработка прогресса NeroCMD и пр. консольных программ (если курсор находится в видимой области)


	//Плагин Update
	//"Downloading Far                                               99%"
	//NeroCMD
    //"012% ########.................................................................."
    //ChkDsk
    //"Completed: 25%"
    //Rar
    // ...       Vista x86\Vista x86.7z         6%
	
	int nIdx = 0;
	bool bAllowDot = false;
	short nProgress = -1;
	
	TODO("Хорошо бы и русские названия обрабатывать?");
	
	// Сначала проверим, может цифры идут в начале строки?
	if (pszCurLine[nIdx] == L' ' && isDigit(pszCurLine[nIdx+1]))
		nIdx++; // один лидирующий пробел перед цифрой
	else if (pszCurLine[nIdx] == L' ' && pszCurLine[nIdx+1] == L' ' && isDigit(pszCurLine[nIdx+2]))
		nIdx += 2; // два лидирующих пробела перед цифрой
	else if (!isDigit(pszCurLine[nIdx]))
	{
		// Строка начинается НЕ с цифры. Может начинается одним из известных префиксов (ChkDsk)?
		static int nRusLen = 0;
		static wchar_t szComplRus[32] = {0};
		if (!szComplRus[0]) {
			MultiByteToWideChar(CP_ACP,0,"Завершено:",-1,szComplRus,countof(szComplRus));
			nRusLen = lstrlen(szComplRus);
		}
		static int nEngLen = lstrlen(L"Completed:");

		if (!wcsncmp(pszCurLine, szComplRus, nRusLen)) {
			nIdx += nRusLen;
			if (pszCurLine[nIdx] == L' ') nIdx++;
			if (pszCurLine[nIdx] == L' ') nIdx++;
			bAllowDot = true;
		} else if (!wcsncmp(pszCurLine, L"Completed:", nEngLen)) {
			nIdx += nRusLen;
			if (pszCurLine[nIdx] == L' ') nIdx++;
			if (pszCurLine[nIdx] == L' ') nIdx++;
			bAllowDot = true;
		}

		// Известных префиксов не найдено, проверяем, может процент есть в конце строки?
		if (!nIdx)
		{
			//TODO("Не работает с одной цифрой");
			// Creating archive T:\From_Work\VMWare\VMWare.part006.rar                                                       
			// ...       Vista x86\Vista x86.7z         6%
			
			int i = con.nTextWidth - 1;
			// Откусить trailing spaces
			while (i>3 && pszCurLine[i] == L' ')
				i--;
			// Теперь, если дошли до '%' и перед ним - цифра
			if (i >= 3 && pszCurLine[i] == L'%' && isDigit(pszCurLine[i-1]))
			{
				//i -= 2;
				i--;
				// Две цифры перед '%'?
				if (isDigit(pszCurLine[i-1]))
					i--;
				// Три цифры допускается только для '100%'
				if (pszCurLine[i-1] == L'1' && !isDigit(pszCurLine[i-2]))
				{
					nIdx = i - 1;
				}
				// final check
				else if (!isDigit(pszCurLine[i-1]))
				{
					nIdx = i;
                }
				// Может ошибочно детектировать прогресс, если его ввести в строке запуска при погашенных панелях
				// Если в строке есть символ '>' - не прогресс
                while (i>=0)
                {
                	if (pszCurLine[i] == L'>')
                	{
                		nIdx = 0;
                		break;
                	}
                	i--;
                }
			}
		}
	}
	
	// Менять nProgress только если нашли проценты в строке с курсором
	if (isDigit(pszCurLine[nIdx]))
	{
		if (isDigit(pszCurLine[nIdx+1]) && isDigit(pszCurLine[nIdx+2]) 
			&& (pszCurLine[nIdx+3]==L'%' || (bAllowDot && pszCurLine[nIdx+3]==L'.')
			    || !wcsncmp(pszCurLine+nIdx+3,L" percent",8)))
		{
			nProgress = 100*(pszCurLine[nIdx] - L'0') + 10*(pszCurLine[nIdx+1] - L'0') + (pszCurLine[nIdx+2] - L'0');
		}
		else if (isDigit(pszCurLine[nIdx+1]) 
			&& (pszCurLine[nIdx+2]==L'%' || (bAllowDot && pszCurLine[nIdx+2]==L'.')
				|| !wcsncmp(pszCurLine+nIdx+2,L" percent",8)))
		{
			nProgress = 10*(pszCurLine[nIdx] - L'0') + (pszCurLine[nIdx+1] - L'0');
		}
		else if (pszCurLine[nIdx+1]==L'%' || (bAllowDot && pszCurLine[nIdx+1]==L'.')
				|| !wcsncmp(pszCurLine+nIdx+1,L" percent",8))
		{
			nProgress = (pszCurLine[nIdx] - L'0');
		}
	}
	
	if (nProgress != -1)
	{
		mn_LastConsoleProgress = nProgress;
		mn_LastConProgrTick = GetTickCount();
	}

	return nProgress;
}


// mn_Progress не меняет, результат возвращает
short CRealConsole::CheckProgressInTitle()
{
	// Обработка прогресса NeroCMD и пр. консольных программ (если курсор находится в видимой области)
	// выполняется в CheckProgressInConsole (-> mn_ConsoleProgress), вызывается из FindPanels 

	short nNewProgress = -1;

	int i = 0;
	wchar_t ch;

	// Wget [41%] http://....
	while ((ch = Title[i])!=0 && (ch == L'{' || ch == L'(' || ch == L'['))
		i++;

    //if (Title[0] == L'{' || Title[0] == L'(' || Title[0] == L'[') {
	if (Title[i])
	{
    	
		// Некоторые плагины/программы форматируют проценты по двум пробелам
    	if (Title[i] == L' ')
    	{
    		i++;
    		if (Title[i] == L' ')
    			i++;
    	}
    	
		// Теперь, если цифра - считываем проценты
        if (isDigit(Title[i]))
        {
            if (isDigit(Title[i+1]) && isDigit(Title[i+2]) 
                && (Title[i+3] == L'%' || (Title[i+3] == L'.' && isDigit(Title[i+4]) && Title[i+7] == L'%'))
                )
            {
                // По идее больше 100% быть не должно :)
                nNewProgress = 100*(Title[i] - L'0') + 10*(Title[i+1] - L'0') + (Title[i+2] - L'0');
            }
            else if (isDigit(Title[i+1]) 
                && (Title[i+2] == L'%' || (Title[i+2] == L'.' && isDigit(Title[i+3]) && Title[i+4] == L'%') )
                )
            {
                // 10 .. 99 %
                nNewProgress = 10*(Title[i] - L'0') + (Title[i+1] - L'0');
            }
            else if (Title[i+1] == L'%' || (Title[i+1] == L'.' && isDigit(Title[i+2]) && Title[i+3] == L'%'))
            {
                // 0 .. 9 %
                nNewProgress = (Title[i] - L'0');
            }
            _ASSERTE(nNewProgress<=100);
            if (nNewProgress > 100)
                nNewProgress = 100;
        }
    }

	return nNewProgress;
}

void CRealConsole::OnTitleChanged()
{
    if (!this) return;

#ifdef _DEBUG
	if (con.bDebugLocked)
		return;
#endif


    wcscpy(Title, TitleCmp);

    // Обработка прогресса операций
    //short nLastProgress = mn_Progress;
	short nNewProgress;

	TitleFull[0] = 0;
	nNewProgress = CheckProgressInTitle();
	if (nNewProgress == -1)
	{
		// mn_ConsoleProgress обновляется в FindPanels, должен быть уже вызван
		if (mn_ConsoleProgress != -1)
		{
    		// Обработка прогресса NeroCMD и пр. консольных программ
			// Если курсор находится в видимой области
    		nNewProgress = mn_ConsoleProgress;

			// Если в заголовке нет процентов (они есть только в консоли)
			// добавить их в наш заголовок
			wchar_t szPercents[5];
			_ltow(nNewProgress, szPercents, 10);
			lstrcatW(szPercents, L"%");
			if (!wcsstr(TitleCmp, szPercents))
			{
				TitleFull[0] = L'{';
				_ltow(nNewProgress, TitleFull+1, 10);
				lstrcatW(TitleFull, L"%} ");
			}
		}
    }
	wcscat(TitleFull, TitleCmp);
	// Обновляем на что нашли
	mn_Progress = nNewProgress;
	if (nNewProgress >= 0 && nNewProgress <= 100)
		mn_PreWarningProgress = nNewProgress;
	//SetProgress(nNewProgress);

	if (isAdministrator() && (gSet.bAdminShield || gSet.szAdminTitleSuffix))
	{
		if (!gSet.bAdminShield)
    		wcscat(TitleFull, gSet.szAdminTitleSuffix);
	}

	CheckFarStates();

    // иначе может среагировать на изменение заголовка ДО того,
    // как мы узнаем, что активировался редактор...
    TODO("Должно срабатывать и при запуске консольной программы!");
    if (Title[0] == L'{' || Title[0] == L'(')
        CheckPanelTitle();

	// заменил на GetProgress, т.к. он еще учитывает mn_PreWarningProgress
	nNewProgress = GetProgress(NULL);

    if (gConEmu.isActive(mp_VCon) && wcscmp(TitleFull, gConEmu.GetLastTitle(false)))
    {
        // Для активной консоли - обновляем заголовок. Прогресс обновится там же
		mn_LastShownProgress = nNewProgress;
		gConEmu.UpdateTitle(/*TitleFull*/); //100624 - сам перечитает // 20.09.2009 Maks - было TitleCmd
    }
    else if (mn_LastShownProgress != nNewProgress)
    {
        // Для НЕ активной консоли - уведомить главное окно, что у нас сменились проценты
		mn_LastShownProgress = nNewProgress;
        gConEmu.UpdateProgress();
    }
    gConEmu.mp_TabBar->Update(); // сменить заголовок закладки?
}

bool CRealConsole::isFilePanel(bool abPluginAllowed/*=false*/)
{
    if (!this) return false;

    if (Title[0] == 0) return false;

    //if (abPluginAllowed) {
	// "Viewer/Editor" вроде однозначно определяется, так почему только при abPluginAllowed
    if (isEditor() || isViewer())
         return false;
    //}

	// Если висят какие-либо диалоги - считаем что это НЕ панель
	DWORD dwFlags = mp_Rgn->GetFlags();
	if ((dwFlags & FR_FREEDLG_MASK) != 0)
		return false;

    // нужно для DragDrop
    if (_tcsncmp(Title, ms_TempPanel, _tcslen(ms_TempPanel)) == 0 || _tcsncmp(Title, ms_TempPanelRus, _tcslen(ms_TempPanelRus)) == 0)
        return true;

    if ((abPluginAllowed && Title[0]==_T('{')) ||
        (_tcsncmp(Title, _T("{\\\\"), 3)==0) ||
        (Title[0] == _T('{') && isDriveLetter(Title[1]) && Title[2] == _T(':') && Title[3] == _T('\\')))
    {
        TCHAR *Br = _tcsrchr(Title, _T('}'));
		if (Br && _tcsstr(Br, _T("} - Far"))) {
			if (mb_LeftPanel || mb_RightPanel)
				return true;
		}
    }
    //TCHAR *BrF = _tcschr(Title, '{'), *BrS = _tcschr(Title, '}'), *Slash = _tcschr(Title, '\\');
    //if (BrF && BrS && Slash && BrF == Title && (Slash == Title+1 || Slash == Title+3))
    //    return true;
    return false;
}

bool CRealConsole::isEditor()
{
    if (!this) return false;
    return GetFarStatus() & CES_EDITOR;
}

bool CRealConsole::isEditorModified()
{
	if (!this) return false;
	if (!isEditor()) return false;
	
	if (mp_tabs && mn_tabsCount) {
		for (int j = 0; j < mn_tabsCount; j++) {
			if (mp_tabs[j].Current) {
				if (mp_tabs[j].Type == /*Editor*/3) {
					return (mp_tabs[j].Modified != 0);
				}
				return false;
			}
		}
	}

	return false;
}

bool CRealConsole::isViewer()
{
    if (!this) return false;
    return GetFarStatus() & CES_VIEWER;
}

bool CRealConsole::isNtvdm()
{
    if (!this) return false;
    if (mn_ProgramStatus & CES_NTVDM) {
        // Наличие 16bit определяем ТОЛЬКО по WinEvent. Иначе не получится отсечь его завершение,
        // т.к. процесс ntvdm.exe не выгружается, а остается в памяти.
        return true;
        //if (mn_ProgramStatus & CES_FARFLAGS) {
        //  //mn_ActiveStatus &= ~CES_NTVDM;
        //} else if (isFilePanel()) {
        //  //mn_ActiveStatus &= ~CES_NTVDM;
        //} else {
        //  return true;
        //}
    }
    return false;
}

LPCWSTR CRealConsole::GetCmd()
{
	if (!this) return L"";
    if (m_Args.pszSpecialCmd)
        return m_Args.pszSpecialCmd;
    else
        return gSet.GetCmd();
}

LPCWSTR CRealConsole::GetDir()
{
	if (!this) return L"";
    if (m_Args.pszSpecialCmd)
        return m_Args.pszStartupDir;
    else
        return gConEmu.ms_ConEmuCurDir;
}

BOOL CRealConsole::GetUserPwd(const wchar_t** ppszUser, /*const wchar_t** ppszPwd,*/ BOOL* pbRestricted)
{
	if (m_Args.bRunAsRestricted) {
		*pbRestricted = TRUE;
		*ppszUser = /**ppszPwd =*/ NULL;
		return TRUE;
	}
	if (m_Args.pszUserName /*&& m_Args.pszUserPassword*/) {
		*ppszUser = m_Args.pszUserName;
		//*ppszPwd = m_Args.pszUserPassword;
		*pbRestricted = FALSE;
		return TRUE;
	}
	return FALSE;
}

short CRealConsole::GetProgress(BOOL *rpbError)
{
    if (!this) return -1;
	if (mn_Progress >= 0)
		return mn_Progress;
	if (mn_PreWarningProgress >= 0) {
		// mn_PreWarningProgress - это последнее значение прогресса (0..100)
		// по после завершения процесса - он может еще быть не сброшен
		if (rpbError) {
			//*rpbError = TRUE; --
			*rpbError = (mn_FarStatus & CES_OPER_ERROR) == CES_OPER_ERROR;
		}
		//if (mn_LastProgressTick != 0 && rpbError) {
		//	DWORD nDelta = GetTickCount() - mn_LastProgressTick;
		//	if (nDelta >= 1000) {
		//		if (rpbError) *rpbError = TRUE;
		//	}
		//}
		return mn_PreWarningProgress;
	}
	return -1;
}

//// установить переменную mn_Progress и mn_LastProgressTick
//void CRealConsole::SetProgress(short anProgress)
//{
//	mn_Progress = anProgress;
//	if (anProgress >= 0 && anProgress <= 100) {
//		mn_PreWarningProgress = anProgress;
//		mn_LastProgressTick = GetTickCount();
//	} else {
//		mn_LastProgressTick = 0;
//	}
//}

void CRealConsole::UpdateFarSettings(DWORD anFarPID/*=0*/)
{
    if (!this) return;
    
    DWORD dwFarPID = anFarPID ? anFarPID : GetFarPID();
    if (!dwFarPID) return;

    //int nLen = /*ComSpec\0*/ 8 + /*ComSpecC\0*/ 9 + 20 +  2*lstrlenW(gConEmu.ms_ConEmuExe);
    wchar_t szCMD[MAX_PATH+1];
    //wchar_t szData[MAX_PATH*4+64];

    // Если определена ComSpecC - значит переопределен стандартный ComSpec
    if (!GetEnvironmentVariable(L"ComSpecC", szCMD, MAX_PATH) || szCMD[0] == 0)
        if (!GetEnvironmentVariable(L"ComSpec", szCMD, MAX_PATH) || szCMD[0] == 0)
            szCMD[0] = 0;
    if (szCMD[0] != 0) {
        // Только если это (случайно) не conemuc.exe
        wchar_t* pwszCopy = wcsrchr(szCMD, L'\\'); if (!pwszCopy) pwszCopy = szCMD;
        #if !defined(__GNUC__)
        #pragma warning( push )
        #pragma warning(disable : 6400)
        #endif
        if (lstrcmpiW(pwszCopy, L"ConEmuC")==0 || lstrcmpiW(pwszCopy, L"ConEmuC.exe")==0
			|| lstrcmpiW(pwszCopy, L"ConEmuC64")==0 || lstrcmpiW(pwszCopy, L"ConEmuC64.exe")==0)
            szCMD[0] = 0;
        #if !defined(__GNUC__)
        #pragma warning( pop )
        #endif
    }

    // ComSpec/ComSpecC не определен, используем cmd.exe
    if (szCMD[0] == 0) {
        wchar_t* psFilePart = NULL;
        if (!SearchPathW(NULL, L"cmd.exe", NULL, MAX_PATH, szCMD, &psFilePart))
        {
            DisplayLastError(L"Can't find cmd.exe!\n", 0);
            return;
        }
    }
    
    // [MAX_PATH*4+64]
    FAR_REQ_SETENVVAR *pSetEnvVar = (FAR_REQ_SETENVVAR*)calloc(sizeof(FAR_REQ_SETENVVAR)+2*(MAX_PATH*4+64),1);
    wchar_t *szData = pSetEnvVar->szEnv;
    
    pSetEnvVar->bFARuseASCIIsort = gSet.isFARuseASCIIsort;
    pSetEnvVar->bShellNoZoneCheck = gSet.isShellNoZoneCheck;

    BOOL lbNeedQuot = (wcschr(gConEmu.ms_ConEmuCExe, L' ') != NULL);
    wchar_t* pszName = szData;
    lstrcpy(pszName, L"ComSpec");
    wchar_t* pszValue = pszName + lstrlenW(pszName) + 1;

	if (gSet.AutoBufferHeight) {
		if (lbNeedQuot) *(pszValue++) = L'"';
		lstrcpy(pszValue, gConEmu.ms_ConEmuCExe);
		if (lbNeedQuot) lstrcat(pszValue, L"\"");

		lbNeedQuot = (szCMD[0] != L'"') && (wcschr(szCMD, L' ') != NULL);
		pszName = pszValue + lstrlenW(pszValue) + 1;
		lstrcpy(pszName, L"ComSpecC");
		pszValue = pszName + lstrlenW(pszName) + 1;
	}
    if (lbNeedQuot) *(pszValue++) = L'"';
    lstrcpy(pszValue, szCMD);
    if (lbNeedQuot) lstrcat(pszValue, L"\"");
    pszName = pszValue + lstrlenW(pszValue) + 1;

	lstrcpy(pszName, L"ConEmuOutput");
	pszName = pszName + lstrlenW(pszName) + 1;
	lstrcpy(pszName, !gSet.nCmdOutputCP ? L"" : ((gSet.nCmdOutputCP == 1) ? L"AUTO" 
		: (((gSet.nCmdOutputCP == 2) ? L"UNICODE" : L"ANSI"))));
	pszName = pszName + lstrlenW(pszName) + 1;

    *(pszName++) = 0;
    *(pszName++) = 0;

    // Выполнить в плагине
    CConEmuPipe pipe(dwFarPID, 300);
    int nSize = sizeof(FAR_REQ_SETENVVAR)+2*(pszName - szData);
    if (pipe.Init(L"SetEnvVars", TRUE))
    	pipe.Execute(CMD_SETENVVAR, pSetEnvVar, nSize);
        //pipe.Execute(CMD_SETENVVAR, szData, 2*(pszName - szData));
        
    free(pSetEnvVar); pSetEnvVar = NULL;
}

HWND CRealConsole::FindPicViewFrom(HWND hFrom)
{
	// !!! PicView может быть несколько, по каждому на открытый ФАР
	HWND hPicView = NULL;

	//hPictureView = FindWindowEx(ghWnd, NULL, L"FarPictureViewControlClass", NULL);
	// Класс может быть как "FarPictureViewControlClass", так и "FarMultiViewControlClass"
	// А вот заголовок у них пока один
	while ((hPicView = FindWindowEx(hFrom, hPicView, NULL, L"PictureView")) != NULL) {
		// Проверить на принадлежность фару
		DWORD dwPID, dwTID;
		dwTID = GetWindowThreadProcessId ( hPicView, &dwPID );
		if (dwPID == mn_FarPID)
			break;
	}

	return hPicView;
}

// Заголовок окна для PictureView вообще может пользователем настраиваться, так что
// рассчитывать на него при определения "Просмотра" - нельзя
HWND CRealConsole::isPictureView(BOOL abIgnoreNonModal/*=FALSE*/)
{
    if (!this) return NULL;

    if (hPictureView && (!IsWindow(hPictureView) || !isFar())) {
        hPictureView = NULL; mb_PicViewWasHidden = FALSE;
        gConEmu.InvalidateAll();
    }

	// !!! PicView может быть несколько, по каждому на открытый ФАР
    if (!hPictureView) {
        //hPictureView = FindWindowEx(ghWnd, NULL, L"FarPictureViewControlClass", NULL);
        hPictureView = FindPicViewFrom(ghWnd);
        if (!hPictureView)
            //hPictureView = FindWindowEx(ghWndDC, NULL, L"FarPictureViewControlClass", NULL);
            hPictureView = FindPicViewFrom(ghWndDC);
        if (!hPictureView) { // FullScreen?
            //hPictureView = FindWindowEx(NULL, NULL, L"FarPictureViewControlClass", NULL);
            hPictureView = FindPicViewFrom(NULL);
        }
		// Принадлежность процессу фара уже проверила сама FindPicViewFrom
        //if (hPictureView) {
        //    // Проверить на принадлежность фару
        //    DWORD dwPID, dwTID;
        //    dwTID = GetWindowThreadProcessId ( hPictureView, &dwPID );
        //    if (dwPID != mn_FarPID) {
        //        hPictureView = NULL; mb_PicViewWasHidden = FALSE;
        //    }
        //}
    }

    
    if (hPictureView) {
        if (mb_PicViewWasHidden) {
            // Окошко было скрыто при переключении на другую консоль, но картинка еще активна
        } else if (!IsWindowVisible(hPictureView)) {
            // Если вызывали Help (F1) - окошко PictureView прячется (самим плагином), считаем что картинки нет
            hPictureView = NULL; mb_PicViewWasHidden = FALSE;
        }
    }

    if (mb_PicViewWasHidden && !hPictureView) mb_PicViewWasHidden = FALSE;

	if (hPictureView && abIgnoreNonModal) {
		wchar_t szClassName[128];
		if (GetClassName(hPictureView, szClassName, countof(szClassName))) {
			if (wcscmp(szClassName, L"FarMultiViewControlClass") == 0) {
				// Пока оно строго немодальное, но потом может быть по другому
				DWORD_PTR dwValue = GetWindowLongPtr(hPictureView, 0);
				if (dwValue != 0x200)
					return NULL;
			}
		}
	}

    return hPictureView;
}

HWND CRealConsole::ConWnd()
{
    if (!this) return NULL;
    return hConWnd;
}

// Найти панели, обновить mn_ConsoleProgress
void CRealConsole::FindPanels()
{
	TODO("Положение панелей можно бы узнавать из плагина");
	
	WARNING("!!! Нужно еще сохранять флажок 'Меню сейчас показано'");

#ifdef _DEBUG
	if (con.bDebugLocked)
		return;
#endif

    RECT rLeftPanel = MakeRect(-1,-1);
	RECT rLeftPanelFull = rLeftPanel;
	RECT rRightPanel = rLeftPanel;
	RECT rRightPanelFull = rLeftPanel;
	BOOL bLeftPanel = FALSE;
	BOOL bRightPanel = FALSE;
	BOOL bMayBePanels = FALSE;
	BOOL lbNeedUpdateSizes = FALSE;
	short nLastProgress = mn_ConsoleProgress;
	short nNewProgress;

	/*
	Имеем облом. При ресайзе панелей CtrlLeft/CtrlRight иногда сервер считывает
	содержимое консоли ДО окончания вывода в нее новой информации. В итоге верхняя
	часть считанного не соответствует нижней, что влечет ошибку
	определения панелей в CRealConsole::FindPanels - ошибочно считает, что
	Левая панель - полноэкранная :(
	*/

	WARNING("Добавить проверки по всем граням панелей, чтобы на них не было некорректных символов");
	// То есть на грани панели не было других диалогов (вертикальных/угловых бордюров поверх горизонтальной части панели)

	// функция проверяет (mn_ProgramStatus & CES_FARACTIVE) и т.п.
	if (isFar()) {
		// Если активен редактор или вьювер (или диалоги, копирование, и т.п.) - искать бессмысленно
		if ((mn_FarStatus & CES_NOTPANELFLAGS) == 0)
			bMayBePanels = TRUE; // только если нет
	}

    if (bMayBePanels && con.nTextHeight >= MIN_CON_HEIGHT && con.nTextWidth >= MIN_CON_WIDTH)
    {
        uint nY = 0;
		BOOL lbIsMenu = FALSE;
		if (con.pConChar[0] == L' ') {
			lbIsMenu = TRUE;
			for (int i=0; i<con.nTextWidth; i++) {
				if (con.pConChar[i]==ucBoxDblHorz || con.pConChar[i]==ucBoxDblDownRight || con.pConChar[i]==ucBoxDblDownLeft) {
					lbIsMenu = FALSE; break;
				}
			}
			if (lbIsMenu)
				nY ++; // скорее всего, первая строка - меню
		} else if ((con.pConChar[0] == L'R' || con.pConChar[0] == L'P') && con.pConChar[1] == L' ') {
			for (int i=1; i<con.nTextWidth; i++) {
				if (con.pConChar[i]==ucBoxDblHorz || con.pConChar[i]==ucBoxDblDownRight || con.pConChar[i]==ucBoxDblDownLeft) {
					lbIsMenu = FALSE; break;
				}
			}
			if (lbIsMenu)
				nY ++; // скорее всего, первая строка - меню, при включенной записи макроса
		}
            
        uint nIdx = nY*con.nTextWidth;

		// Левая панель
		BOOL bFirstCharOk = (nY == 0) 
			&& (
				(con.pConChar[0] == L'R' && (con.pConAttr[0] & 0x4F) == 0x4F) // символ записи макроса
				|| (con.pConChar[0] == L'P' && con.pConAttr[0] == 0x2F) // символ воспроизведения макроса
			);
		// из-за глюков индикации FAR2 пока вместо '[' - любой символ
		//if (( ((bFirstCharOk || con.pConChar[nIdx] == L'[') && (con.pConChar[nIdx+1]>=L'0' && con.pConChar[nIdx+1]<=L'9')) // открыто несколько редакторов/вьюверов
		if (( ((bFirstCharOk || con.pConChar[nIdx] != ucBoxDblDownRight) && (con.pConChar[nIdx+1]>=L'0' && con.pConChar[nIdx+1]<=L'9')) // открыто несколько редакторов/вьюверов
			|| ((bFirstCharOk || con.pConChar[nIdx] == ucBoxDblDownRight)
				&& (con.pConChar[nIdx+1] == ucBoxDblHorz || con.pConChar[nIdx+1] == ucBoxSinglDownDblHorz)) // доп.окон нет, только рамка
			) && con.pConChar[nIdx+con.nTextWidth] == ucBoxDblVert)
		{
			for (int i=2; !bLeftPanel && i<con.nTextWidth; i++) {
				if (con.pConChar[nIdx+i] == ucBoxDblDownLeft
					&& (con.pConChar[nIdx+i-1] == ucBoxDblHorz || con.pConChar[nIdx+i-1] == ucBoxSinglDownDblHorz)
					// МОЖЕТ быть закрыто AltHistory
					/*&& con.pConChar[nIdx+i+con.nTextWidth] == ucBoxDblVert*/)
				{
					uint nBottom = con.nTextHeight - 1;
					while (nBottom > 4) {
						if (con.pConChar[con.nTextWidth*nBottom] == ucBoxDblUpRight 
							/*&& con.pConChar[con.nTextWidth*nBottom+i] == ucBoxDblUpLeft*/)
						{
							rLeftPanel.left = 1;
							rLeftPanel.top = nY + 2;
							rLeftPanel.right = i-1;
							rLeftPanel.bottom = nBottom - 3;
							rLeftPanelFull.left = 0;
							rLeftPanelFull.top = nY;
							rLeftPanelFull.right = i;
							rLeftPanelFull.bottom = nBottom;
							bLeftPanel = TRUE;
							break;
						}
						nBottom --;
					}
				}
			}
		}

		// (Если есть левая панель и она не FullScreen) или левой панели нет вообще
		if ((bLeftPanel && (rLeftPanel.right+1) < con.nTextWidth) || !bLeftPanel)
		{
			if (bLeftPanel) {
				// Положение известно, нужно только проверить наличие
				if (con.pConChar[nIdx+rLeftPanel.right+2] == ucBoxDblDownRight
					/*&& con.pConChar[nIdx+rLeftPanel.right+2+con.nTextWidth] == ucBoxDblVert*/
					/*&& con.pConChar[nIdx+con.nTextWidth*2] == ucBoxDblVert*/
					/*&& con.pConChar[(rLeftPanel.bottom+3)*con.nTextWidth+rLeftPanel.right+2] == ucBoxDblUpRight*/
					&& con.pConChar[(rLeftPanel.bottom+4)*con.nTextWidth-1] == ucBoxDblUpLeft
					)
				{
					rRightPanel = rLeftPanel; // bottom & top берем из rLeftPanel
					rRightPanel.left = rLeftPanel.right+3;
					rRightPanel.right = con.nTextWidth-2;
					rRightPanelFull = rLeftPanelFull;
					rRightPanelFull.left = rLeftPanelFull.right+1;
					rRightPanelFull.right = con.nTextWidth-1;
					bRightPanel = TRUE;
				}
			}
			// Начиная с FAR2 build 1295 панели могут быть разной высоты
			if (!bRightPanel)
			{
				// нужно определить положение панели
				if (((con.pConChar[nIdx+con.nTextWidth-1]>=L'0' && con.pConChar[nIdx+con.nTextWidth-1]<=L'9') // справа часы
					|| con.pConChar[nIdx+con.nTextWidth-1] == ucBoxDblDownLeft) // или рамка
					&& con.pConChar[nIdx+con.nTextWidth*2-1] == ucBoxDblVert) // ну и правая граница панели
				{
					for (int i=con.nTextWidth-3; !bRightPanel && i>2; i--) {
						// ищем левую границу правой панели
						if (con.pConChar[nIdx+i] == ucBoxDblDownRight
							&& (con.pConChar[nIdx+i+1] == ucBoxDblHorz || con.pConChar[nIdx+i+1] == ucBoxSinglDownDblHorz)
							&& con.pConChar[nIdx+i+con.nTextWidth] == ucBoxDblVert)
						{
							uint nBottom = con.nTextHeight - 1;
							while (nBottom > 4) {
								if (/*con.pConChar[con.nTextWidth*nBottom+i] == ucBoxDblUpRight 
									&&*/ con.pConChar[con.nTextWidth*(nBottom+1)-1] == ucBoxDblUpLeft)
								{
									rRightPanel.left = i+1;
									rRightPanel.top = nY + 2;
									rRightPanel.right = con.nTextWidth-2;
									rRightPanel.bottom = nBottom - 3;
									rRightPanelFull.left = i;
									rRightPanelFull.top = nY;
									rRightPanelFull.right = con.nTextWidth-1;
									rRightPanelFull.bottom = nBottom;
									bRightPanel = TRUE;
									break;
								}
								nBottom --;
							}
						}
					}
				}
			}
		}
    }

#ifdef _DEBUG
	if (bLeftPanel && !bRightPanel && rLeftPanelFull.right > 120) {
		bLeftPanel = bLeftPanel;
	}
#endif

    if (isActive())
		lbNeedUpdateSizes = (memcmp(&mr_LeftPanel,&rLeftPanel,sizeof(mr_LeftPanel)) || memcmp(&mr_RightPanel,&rRightPanel,sizeof(mr_RightPanel)));
		
	mr_LeftPanel = rLeftPanel; mr_LeftPanelFull = rLeftPanelFull; mb_LeftPanel = bLeftPanel;
	mr_RightPanel = rRightPanel; mr_RightPanelFull = rRightPanelFull; mb_RightPanel = bRightPanel;
	if (bRightPanel || bLeftPanel)
		bMayBePanels = TRUE;
	else
		bMayBePanels = FALSE;


    nNewProgress = -1;
	// mn_ProgramStatus не катит. Хочется подхватывать прогресс из плагина Update, а там как раз CES_FARACTIVE
    //if (!abResetOnly && (mn_ProgramStatus & CES_FARACTIVE) == 0) {
	if (!bMayBePanels && (mn_FarStatus & CES_NOTPANELFLAGS) == 0) {
    	// Обработка прогресса NeroCMD и пр. консольных программ
		// Если курсор находится в видимой области
		int nY = con.m_sbi.dwCursorPosition.Y - con.m_sbi.srWindow.Top;
	    if (/*con.m_ci.bVisible && con.m_ci.dwSize -- Update прячет курсор?
			&&*/ con.m_sbi.dwCursorPosition.X >= 0 && con.m_sbi.dwCursorPosition.X < con.nTextWidth
			&& nY >= 0 && nY < con.nTextHeight)
        {
        	int nIdx = nY * con.nTextWidth;
        	
			// Обработка прогресса NeroCMD и пр. консольных программ (если курсор находится в видимой области)
        	nNewProgress = CheckProgressInConsole(con.pConChar+nIdx);

        }
    }
	
	if (mn_ConsoleProgress != nNewProgress || nLastProgress != nNewProgress /*|| mn_Progress != mn_ConsoleProgress*/)
	{
		// Запомнить, что получили из консоли
		mn_ConsoleProgress = nNewProgress;

		// Показать прогресс в заголовке
		mb_ForceTitleChanged = TRUE;
	}

	if (lbNeedUpdateSizes)
		gConEmu.UpdateSizes();
}

int CRealConsole::CoordInPanel(COORD cr)
{
	if (!this)
		return 0;

	if (CoordInRect(cr, mr_LeftPanel))
		return 1;
	if (CoordInRect(cr, mr_RightPanel))
		return 2;
	return 0;
}

BOOL CRealConsole::GetPanelRect(BOOL abRight, RECT* prc, BOOL abFull /*= FALSE*/)
{
	if (!this) {
		if (prc)
			*prc = MakeRect(-1,-1);
		return FALSE;
	}

	if (abRight) {
		if (prc)
			*prc = abFull ? mr_RightPanelFull : mr_RightPanel;
		if (mr_RightPanel.right <= mr_RightPanel.left)
			return FALSE;
	} else {
		if (prc)
			*prc = abFull ? mr_LeftPanelFull : mr_LeftPanel;
		if (mr_LeftPanel.right <= mr_LeftPanel.left)
			return FALSE;
	}

	return TRUE;
}

bool CRealConsole::isFarBufferSupported()
{
	if (!this)
		return false;

	if (!m_FarInfo.IsValid() 
		|| m_FarInfo.Ptr()->nFarPID != GetFarPID()
		|| !m_FarInfo.Ptr()->bBufferSupport)
		return false;

	return true;
}

bool CRealConsole::isSelectionAllowed()
{
	if (!this)
		return false;

	if (!con.pConChar || !con.pConAttr)
		return false; // Если данных консоли еще нет
		
	if (con.m_sel.dwFlags != 0)
		return true; // Если выделение было запущено через меню

	if (!gSet.isConsoleTextSelection)
		return false; // выделение мышкой запрещено в настройках
	else if (gSet.isConsoleTextSelection == 1)
		return true; // разрешено всегда
	else if (isBufferHeight())
	{
		// иначе - только в режиме с прокруткой
		DWORD nFarPid = 0;
		// Но в FAR2 появился новый ключик /w
		if (!isFarBufferSupported())
			return true;
	}

	//if ((con.m_dwConsoleMode & ENABLE_QUICK_EDIT_MODE) == ENABLE_QUICK_EDIT_MODE)
	//	return true;
	//if (mp_ConsoleInfo && isFar(TRUE)) {
	//	if ((mp_ConsoleInfo->FarInfo.nFarInterfaceSettings & 4/*FIS_MOUSE*/) == 0)
	//		return true;
	//}

	return false;
}

bool CRealConsole::isSelectionPresent()
{
	if (!this)
		return false;

	return (con.m_sel.dwFlags != 0);
}

bool CRealConsole::GetConsoleSelectionInfo(CONSOLE_SELECTION_INFO *sel)
{
	if (!this)
		return false;
	if (!isSelectionAllowed())
		return false;

	if (sel) {
		*sel = con.m_sel;
	}
	return (con.m_sel.dwFlags != 0);
	//return (con.m_sel.dwFlags & CONSOLE_SELECTION_NOT_EMPTY) == CONSOLE_SELECTION_NOT_EMPTY;
}

void CRealConsole::GetConsoleCursorInfo(CONSOLE_CURSOR_INFO *ci)
{
	if (!this) return;
	*ci = con.m_ci;
	// Если сейчас выделяется текст мышкой (ConEmu internal)
	// то курсор нужно переключить в половину знакоместа!
	if (isSelectionPresent()) {
		if (ci->dwSize < 50)
			ci->dwSize = 50;
	}
}

void CRealConsole::GetConsoleScreenBufferInfo(CONSOLE_SCREEN_BUFFER_INFO* sbi)
{
	if (!this) return;
	*sbi = con.m_sbi;
	
	if (con.m_sel.dwFlags) {
		// В режиме выделения - положение курсора ставим сами
		GetConsoleCursorPos(&(sbi->dwCursorPosition));
		
		//if (con.m_sel.dwSelectionAnchor.X == con.m_sel.srSelection.Left)
		//	sbi->dwCursorPosition.X = con.m_sel.srSelection.Right;
		//else
		//	sbi->dwCursorPosition.X = con.m_sel.srSelection.Left;
		//
		//if (con.m_sel.dwSelectionAnchor.Y == con.m_sel.srSelection.Top)
		//	sbi->dwCursorPosition.Y = con.m_sel.srSelection.Bottom;
		//else
		//	sbi->dwCursorPosition.Y = con.m_sel.srSelection.Top;
	}	
}

void CRealConsole::GetConsoleCursorPos(COORD *pcr)
{
	if (con.m_sel.dwFlags) {
		// В режиме выделения - положение курсора ставим сами
		if (con.m_sel.dwSelectionAnchor.X == con.m_sel.srSelection.Left)
			pcr->X = con.m_sel.srSelection.Right;
		else
			pcr->X = con.m_sel.srSelection.Left;

		if (con.m_sel.dwSelectionAnchor.Y == con.m_sel.srSelection.Top)
			pcr->Y = con.m_sel.srSelection.Bottom + con.nTopVisibleLine;
		else
			pcr->Y = con.m_sel.srSelection.Top + con.nTopVisibleLine;

	} else {
		*pcr = con.m_sbi.dwCursorPosition;
	}
}

// В дальнейшем надо бы возвращать значение для активного приложения
// По крайней мене в фаре мы можем проверить токены.
// В свойствах приложения проводником может быть установлен флажок "Run as administrator"
// Может быть соответствующий манифест...
// Хотя скорее всего это невозможно. В одной консоли не могут крутиться программы под разными аккаунтами
bool CRealConsole::isAdministrator()
{
	if (!this) return false;
	// 
	return m_Args.bRunAsAdministrator;
}

BOOL CRealConsole::isMouseButtonDown()
{
	if (!this) return FALSE;
	return mb_MouseButtonDown;
}

// Вызывается из CConEmuMain::OnLangChangeConsole в главной нити
void CRealConsole::OnConsoleLangChange(DWORD_PTR dwNewKeybLayout)
{
    if (con.dwKeybLayout != dwNewKeybLayout)
	{

	    if (gSet.isAdvLogging > 1)
	    {
	    	wchar_t szInfo[255];
			wsprintf(szInfo, L"CRealConsole::OnConsoleLangChange, Old=0x%08X, New=0x%08X",
				(DWORD)con.dwKeybLayout, (DWORD)dwNewKeybLayout);
	    	LogString(szInfo);
		}

        con.dwKeybLayout = dwNewKeybLayout;

		gConEmu.SwitchKeyboardLayout(dwNewKeybLayout);
		
		#ifdef _DEBUG
		WCHAR szMsg[255];
		HKL hkl = GetKeyboardLayout(0);
		wsprintf(szMsg, L"ConEmu: GetKeyboardLayout(0) after SwitchKeyboardLayout = 0x%08I64X\n",
			(unsigned __int64)(DWORD_PTR)hkl);
		DEBUGSTRLANG(szMsg);
		//Sleep(2000);
		#endif
    }
    else
    {
	    if (gSet.isAdvLogging > 1)
	    {
	    	wchar_t szInfo[255];
			wsprintf(szInfo, L"CRealConsole::OnConsoleLangChange skipped, con.dwKeybLayout already is 0x%08X",
				(DWORD)dwNewKeybLayout);
	    	LogString(szInfo);
		}
    }
}

DWORD CRealConsole::GetConsoleStates()
{
	if (!this) return 0;
	return con.m_dwConsoleMode;
}


void CRealConsole::CloseColorMapping()
{
	m_TrueColorerMap.CloseMap();
	//if (mp_ColorHdr) {
	//	UnmapViewOfFile(mp_ColorHdr);
	//mp_ColorHdr = NULL;
	mp_TrueColorerData = NULL;
	//}
	//if (mh_ColorMapping) {
	//	CloseHandle(mh_ColorMapping);
	//	mh_ColorMapping = NULL;
	//}

	mb_DataChanged = TRUE;
	mn_LastColorFarID = 0;
}

//void CRealConsole::CheckColorMapping(DWORD dwPID)
//{
//	if (!dwPID)
//		dwPID = GetFarPID();
//	if ((!dwPID && m_TrueColorerMap.IsValid()) || (dwPID != mn_LastColorFarID)) {
//		//CloseColorMapping();
//		if (!dwPID)
//			return;
//	}
//	
//	if (dwPID == mn_LastColorFarID)
//		return; // Для этого фара - наличие уже проверяли!
//		
//	mn_LastColorFarID = dwPID; // сразу запомним
//	
//}

void CRealConsole::OpenColorMapping()
{
	BOOL lbResult = FALSE;
	wchar_t szErr[512]; szErr[0] = 0;
	//wchar_t szMapName[512]; szErr[0] = 0;
	const AnnotationHeader *pHdr = NULL;

	_ASSERTE(hConWnd!=NULL);

	m_TrueColorerMap.InitName(AnnotationShareName, (DWORD)sizeof(AnnotationInfo), (DWORD)hConWnd);

	if (!m_TrueColorerMap.Open()) {
		lstrcpyn(szErr, m_TrueColorerMap.GetErrorText(), countof(szErr));
		goto wrap;
	}
	
	//_ASSERTE(mh_ColorMapping == NULL);

	//wsprintf(szMapName, AnnotationShareName, sizeof(AnnotationInfo), (DWORD)hConWnd);
	//mh_ColorMapping = OpenFileMapping(FILE_MAP_READ, FALSE, szMapName);
	//if (!mh_ColorMapping) {
	//	DWORD dwErr = GetLastError();
	//	wsprintf (szErr, L"ConEmu: Can't open colorer file mapping. ErrCode=0x%08X. %s", dwErr, szMapName);
	//	goto wrap;
	//}
	//
	//mp_ColorHdr = (AnnotationHeader*)MapViewOfFile(mh_ColorMapping, FILE_MAP_READ,0,0,0);
	//if (!mp_ColorHdr) {
	//	DWORD dwErr = GetLastError();
	//	wchar_t szErr[512];
	//	wsprintf (szErr, L"ConEmu: Can't map colorer info. ErrCode=0x%08X. %s", dwErr, szMapName);
	//	CloseHandle(mh_ColorMapping); mh_ColorMapping = NULL;
	//	goto wrap;
	//}
	
	pHdr = m_TrueColorerMap.Ptr();
	mp_TrueColorerData = (const AnnotationInfo*)(((LPBYTE)pHdr)+pHdr->struct_size);

	lbResult = TRUE;
wrap:
	if (!lbResult && szErr[0]) {
		gConEmu.DebugStep(szErr);
		#ifdef _DEBUG
		MBoxA(szErr);
		#endif
	}
	//return lbResult;
}

BOOL CRealConsole::OpenFarMapData()
{
	BOOL lbResult = FALSE;
	wchar_t szMapName[128], szErr[512]; szErr[0] = 0;
	//int nConInfoSize = sizeof(CESERVER_REQ_CONINFO_HDR);

	CloseFarMapData();

	_ASSERTE(m_FarInfo.IsValid() == FALSE);
	//_ASSERTE(mh_FarFileMapping == NULL);
	//_ASSERTE(mh_FarAliveEvent == NULL);

	DWORD dwErr = 0;
	DWORD nFarPID = GetFarPID(TRUE);
	if (!nFarPID)
		return FALSE;

	m_FarInfo.InitName(CEFARMAPNAME, nFarPID);
	//wsprintf(szMapName, CEFARMAPNAME, nFarPID);
	//mh_FarFileMapping = OpenFileMapping(FILE_MAP_READ/*|FILE_MAP_WRITE*/, FALSE, szMapName);
	//if (!mh_FarFileMapping) {
	//	dwErr = GetLastError();
	//	wsprintf (szErr, L"ConEmu: Can't open FAR data file mapping. ErrCode=0x%08X. %s", dwErr, szMapName);
	//	goto wrap;
	//}

	//mp_FarInfo = (CEFAR_INFO*)MapViewOfFile(mh_FarFileMapping, FILE_MAP_READ/*|FILE_MAP_WRITE*/,0,0,0);
	//if (!mp_FarInfo)
	if (!m_FarInfo.Open())
	{
		//DWORD dwErr = GetLastError();
		//wchar_t szErr[512];
		//wsprintf (szErr, L"ConEmu: Can't map FAR info. ErrCode=0x%08X. %s", dwErr, szMapName);
		lstrcpynW(szErr, m_FarInfo.GetErrorText(), countof(szErr));
		goto wrap;
	}

	if (m_FarInfo.Ptr()->nFarPID != nFarPID) {
		_ASSERTE(m_FarInfo.Ptr()->nFarPID != nFarPID);
		CloseFarMapData();
		wsprintf (szErr, L"ConEmu: Invalid FAR info format. %s", szMapName);
		goto wrap;
	}
	_ASSERTE(m_FarInfo.Ptr()->nProtocolVersion == CESERVER_REQ_VER);



	m_FarAliveEvent.InitName(CEFARALIVEEVENT, nFarPID);
	//wsprintf(szMapName, CEFARALIVEEVENT, nFarPID);
	//mh_FarAliveEvent = OpenEvent(EVENT_MODIFY_STATE|SYNCHRONIZE, FALSE, szMapName);
	//if (!mh_FarAliveEvent) {
	if (!m_FarAliveEvent.Open()) {
		dwErr = GetLastError();
		//if (mp_FarInfo) {
		if (m_FarInfo.IsValid()) {
			_ASSERTE(m_FarAliveEvent.GetHandle()!=NULL);
		}
	}

	lbResult = TRUE;
wrap:
	if (!lbResult && szErr[0]) {
		gConEmu.DebugStep(szErr);
		MBoxA(szErr);
	}
	return lbResult;
}

BOOL CRealConsole::OpenMapHeader(BOOL abFromAttach)
{
	BOOL lbResult = FALSE;
	wchar_t szErr[512]; szErr[0] = 0;
	//int nConInfoSize = sizeof(CESERVER_REQ_CONINFO_HDR);

	if (m_ConsoleMap.IsValid()) {
		if (hConWnd == (HWND)(m_ConsoleMap.Ptr()->hConWnd)) {
			_ASSERTE(m_ConsoleMap.Ptr() == NULL);
			return TRUE;
		}
	}
	
	//_ASSERTE(mh_FileMapping == NULL);

	//CloseMapData();

	m_ConsoleMap.InitName(CECONMAPNAME, (DWORD)hConWnd);

	if (!m_ConsoleMap.Open()) {
		lstrcpyn(szErr, m_ConsoleMap.GetErrorText(), countof(szErr));
		//wsprintf (szErr, L"ConEmu: Can't open console data file mapping. ErrCode=0x%08X. %s", dwErr, ms_HeaderMapName);
		goto wrap;
	}
	
	//wsprintf(ms_HeaderMapName, CECONMAPNAME, (DWORD)hConWnd);
	//mh_FileMapping = OpenFileMapping(FILE_MAP_READ/*|FILE_MAP_WRITE*/, FALSE, ms_HeaderMapName);
	//if (!mh_FileMapping) {
	//	DWORD dwErr = GetLastError();
	//	wsprintf (szErr, L"ConEmu: Can't open console data file mapping. ErrCode=0x%08X. %s", dwErr, ms_HeaderMapName);
	//	goto wrap;
	//}
	//
	//mp_ConsoleInfo = (CESERVER_REQ_CONINFO_HDR*)MapViewOfFile(mh_FileMapping, FILE_MAP_READ/*|FILE_MAP_WRITE*/,0,0,0);
	//if (!mp_ConsoleInfo) {
	//	DWORD dwErr = GetLastError();
	//	wchar_t szErr[512];
	//	wsprintf (szErr, L"ConEmu: Can't map console info. ErrCode=0x%08X. %s", dwErr, ms_HeaderMapName);
	//	goto wrap;
	//}

	if (!abFromAttach) {	
		if (m_ConsoleMap.Ptr()->nGuiPID != GetCurrentProcessId()) {
			_ASSERTE(m_ConsoleMap.Ptr()->nGuiPID == GetCurrentProcessId());
			WARNING("Наверное нужно будет передать в сервер код GUI процесса? В каком случае так может получиться?");
			//PRAGMA_ERROR("Передать через команду сервера новый GUI PID. Если пайп не готов сразу выйти");
		}
	}
	
	if (m_ConsoleMap.Ptr()->hConWnd && m_ConsoleMap.Ptr()->bDataReady) {
		// Только если MonitorThread еще не был запущен
		if (mn_MonitorThreadID == 0)
			ApplyConsoleInfo();
	}

	lbResult = TRUE;
wrap:
	if (!lbResult && szErr[0]) {
		gConEmu.DebugStep(szErr);
		MBoxA(szErr);
	}
	return lbResult;
}

//void CRealConsole::CloseMapData()
//{
//	if (mp_ConsoleData) {
//		UnmapViewOfFile(mp_ConsoleData);
//		mp_ConsoleData = NULL;
//		lstrcpy(ms_ConStatus, L"Console data was not opened!");
//	}
//	if (mh_FileMappingData) {
//		CloseHandle(mh_FileMappingData);
//		mh_FileMappingData = NULL;
//	}
//	mn_LastConsoleDataIdx = mn_LastConsolePacketIdx = /*mn_LastFarReadIdx =*/ -1;
//	mn_LastFarReadTick = 0;
//}

void CRealConsole::CloseFarMapData()
{
	m_FarInfo.CloseMap();
	//if (mp_FarInfo) {
	//	UnmapViewOfFile(mp_FarInfo);
	//	mp_FarInfo = NULL;
	//}
	//if (mh_FarFileMapping) {
	//	CloseHandle(mh_FarFileMapping);
	//	mh_FarFileMapping = NULL;
	//}

	m_FarAliveEvent.Close();
	//if (mh_FarAliveEvent) {
	//	CloseHandle(mh_FarAliveEvent);
	//	mh_FarAliveEvent = NULL;
	//}
}

//BOOL CRealConsole::ReopenMapData()
//{
//	BOOL lbRc = FALSE;
//	DWORD dwErr = 0;
//	DWORD nNewIndex = 0;
//	//DWORD nMaxSize = (con.m_sbi.dwMaximumWindowSize.X * con.m_sbi.dwMaximumWindowSize.Y * 2) * sizeof(CHAR_INFO);
//	wchar_t szErr[255]; szErr[0] = 0;
//	
//	_ASSERTE(mp_ConsoleInfo);
//	if (!mp_ConsoleInfo) {
//		lstrcpyW(szErr, L"ConEmu: RecreateMapData failed, mp_ConsoleInfo is NULL");
//		goto wrap;
//	}
//	
//	if (mp_ConsoleData)
//		CloseMapData();
//		
//
//	nNewIndex = mp_ConsoleInfo->nCurDataMapIdx;
//	if (!nNewIndex) {
//		_ASSERTE(nNewIndex);
//		lstrcpyW(szErr, L"ConEmu: mp_ConsoleInfo->nCurDataMapIdx is null");
//		goto wrap;
//	}
//	
//	wsprintf(ms_DataMapName, CECONMAPNAME L".%i", (DWORD)hConWnd, nNewIndex);
//
//	mh_FileMappingData = OpenFileMapping(FILE_MAP_READ, FALSE, ms_DataMapName);
//	if (!mh_FileMappingData) {
//		dwErr = GetLastError();
//		wsprintf (szErr, L"ConEmu: OpenFileMapping(%s) failed. ErrCode=0x%08X", ms_DataMapName, dwErr);
//		goto wrap;
//	}
//	
//	mp_ConsoleData = (CESERVER_REQ_CONINFO_DATA*)MapViewOfFile(mh_FileMappingData, FILE_MAP_READ,0,0,0);
//	if (!mp_ConsoleData) {
//		dwErr = GetLastError();
//		CloseHandle(mh_FileMappingData); mh_FileMappingData = NULL;
//		wsprintf (szErr, L"ConEmu: MapViewOfFile(%s) failed. ErrCode=0x%08X", ms_DataMapName, dwErr);
//		goto wrap;
//	}
//	
//	mn_LastConsoleDataIdx = nNewIndex;
//
//	ms_ConStatus[0] = 0; // сброс
//	
//	// Done
//	lbRc = TRUE;
//wrap:
//	if (!lbRc && szErr[0]) {
//		gConEmu.DebugStep(szErr);
//		MBoxA(szErr);
//	}
//	return lbRc;
//}

void CRealConsole::CloseMapHeader()
{
	CloseFarMapData();

	//CloseMapData();

	m_GetDataPipe.Close();
	m_ConsoleMap.CloseMap();
	//if (mp_ConsoleInfo) {
	//	UnmapViewOfFile(mp_ConsoleInfo);
	//	mp_ConsoleInfo = NULL;
	//}
	//if (mh_FileMapping) {
	//	CloseHandle(mh_FileMapping);
	//	mh_FileMapping = NULL;
	//}

	con.m_ci.bVisible = TRUE;
	con.m_ci.dwSize = 15;
	con.m_sbi.dwCursorPosition = MakeCoord(0,con.m_sbi.srWindow.Top);

	if (con.pConChar && con.pConAttr) {
		MSectionLock sc; sc.Lock(&csCON);
		DWORD OpeBufferSize = con.nTextWidth*con.nTextHeight*sizeof(wchar_t);
		memset(con.pConChar,0,OpeBufferSize);
		memset(con.pConAttr,0,OpeBufferSize);
	}

	mb_DataChanged = TRUE;
}

void CRealConsole::ApplyConsoleInfo()
{
    BOOL bBufRecreated = FALSE;
	BOOL lbChanged = FALSE;

#ifdef _DEBUG
	if (con.bDebugLocked)
		return;
#endif

	if (m_ServerClosing.nServerPID && m_ServerClosing.nServerPID == mn_ConEmuC_PID)
	{
		// Сервер уже закрывается. попытка считать данные из консоли может привести к зависанию!
		SetEvent(mh_ApplyFinished);
		return;
	}

	ResetEvent(mh_ApplyFinished);
    
	const CESERVER_REQ_CONINFO_INFO* pInfo = NULL;
	CESERVER_REQ_HDR cmd; ExecutePrepareCmd(&cmd, CECMD_CONSOLEDATA, sizeof(cmd));
	DWORD nOutSize = 0;
    
	if (!m_ConsoleMap.IsValid()) {
		_ASSERTE(m_ConsoleMap.IsValid());
	} else
	if (!m_GetDataPipe.Transact(&cmd, sizeof(cmd), (const CESERVER_REQ_HDR**)&pInfo, &nOutSize) || !pInfo) {
		#ifdef _DEBUG
		MBoxA(m_GetDataPipe.GetErrorText());
		#endif
	} else
	if (pInfo->cmd.cbSize < sizeof(CESERVER_REQ_CONINFO_INFO)) {
		_ASSERTE(pInfo->cmd.cbSize >= sizeof(CESERVER_REQ_CONINFO_INFO));
	} else
	//if (!mp_ConsoleInfo->hConWnd || !mp_ConsoleInfo->nCurDataMapIdx) {
	//	_ASSERTE(mp_ConsoleInfo->hConWnd && mp_ConsoleInfo->nCurDataMapIdx);
	//} else
	{
		//if (mn_LastConsoleDataIdx != mp_ConsoleInfo->nCurDataMapIdx) {
		//	ReopenMapData();
		//}
	    
		DWORD nPID = GetCurrentProcessId();
		DWORD nMapGuiPID = m_ConsoleMap.Ptr()->nGuiPID;
		if (nPID != nMapGuiPID) {
			// Если консоль запускалась как "-new_console" то nMapGuiPID может быть еще 0?
			// Хотя, это может случиться только если батник запущен из консоли НЕ прицепленной к GUI ConEmu.
			if (nMapGuiPID != 0)
			{
    			_ASSERTE(nMapGuiPID == nPID);
			}
    		//PRAGMA_ERROR("Передать через команду сервера новый GUI PID. Если пайп не готов сразу выйти");
    		//mp_ConsoleInfo->nGuiPID = nPID;
		}
		
		
		
	    
		#ifdef _DEBUG
		HWND hWnd = pInfo->hConWnd;
		_ASSERTE(hWnd!=NULL);
		_ASSERTE(hWnd==hConWnd);
		#endif
		//if (hConWnd != hWnd) {
		//    SetHwnd ( hWnd ); -- низя. Maps уже созданы!
		//}
		// 3
		// Здесь у нас реальные процессы консоли, надо обновиться
		ProcessUpdate(pInfo->nProcesses, countof(pInfo->nProcesses));

		// Теперь нужно открыть секцию - начинаем изменение переменных класса
		MSectionLock sc;

		// 4
		DWORD dwCiSize = pInfo->dwCiSize;
		if (dwCiSize != 0) {
			_ASSERTE(dwCiSize == sizeof(con.m_ci));
			if (memcmp(&con.m_ci, &pInfo->ci, sizeof(con.m_ci))!=0)
				lbChanged = TRUE;
			con.m_ci = pInfo->ci;
		}
		// 5, 6, 7
		con.m_dwConsoleCP = pInfo->dwConsoleCP;
		con.m_dwConsoleOutputCP = pInfo->dwConsoleOutputCP;
		if (con.m_dwConsoleMode != pInfo->dwConsoleMode) {
    		if (ghOpWnd && isActive())
    			gSet.UpdateConsoleMode(pInfo->dwConsoleMode);
		}
		con.m_dwConsoleMode = pInfo->dwConsoleMode;
		// 8
		DWORD dwSbiSize = pInfo->dwSbiSize;
		int nNewWidth = 0, nNewHeight = 0;
		if (dwSbiSize != 0) {
			MCHKHEAP
			_ASSERTE(dwSbiSize == sizeof(con.m_sbi));
			if (memcmp(&con.m_sbi, &pInfo->sbi, sizeof(con.m_sbi))!=0) {
				lbChanged = TRUE;
				if (isActive())
					gConEmu.UpdateCursorInfo(pInfo->sbi.dwCursorPosition);
			}
			con.m_sbi = pInfo->sbi;
			if (GetConWindowSize(con.m_sbi, nNewWidth, nNewHeight)) {
				if (con.bBufferHeight != (nNewHeight < con.m_sbi.dwSize.Y))
					SetBufferHeightMode(nNewHeight < con.m_sbi.dwSize.Y);
				//  TODO("Включить прокрутку? или оно само?");
				if (nNewWidth != con.nTextWidth || nNewHeight != con.nTextHeight) {
					#ifdef _DEBUG
					wchar_t szDbgSize[128]; wsprintf(szDbgSize, L"ApplyConsoleInfo.SizeWasChanged(cx=%i, cy=%i)\n", nNewWidth, nNewHeight);
					DEBUGSTRSIZE(szDbgSize);
					#endif

					bBufRecreated = TRUE; // Смена размера, буфер пересоздается
					//sc.Lock(&csCON, TRUE);
					//WARNING("может не заблокировалось?");
					InitBuffers(nNewWidth*nNewHeight*2);
				}
			}
			if (gSet.AutoScroll)
				con.nTopVisibleLine = con.m_sbi.srWindow.Top;
			MCHKHEAP
		}
	    
		//DWORD dwCharChanged = pInfo->ConInfo.RgnInfo.dwRgnInfoSize;
		//BOOL  lbDataRecv = FALSE;
		if (/*mp_ConsoleData &&*/ nNewWidth && nNewHeight)
		{
			_ASSERTE(nNewWidth == pInfo->crWindow.X && nNewHeight == pInfo->crWindow.Y);
			// 10
			//DWORD MaxBufferSize = pInfo->nCurDataMaxSize;
			//if (MaxBufferSize != 0) {
			
			//// Не будем гонять зря данные по пайпу, если изменений нет
			//if (mn_LastConsolePacketIdx != pInfo->nPacketId)

			// Если вместе с заголовком пришли измененные данные
			if (pInfo->nDataShift && pInfo->nDataCount)
			{
				mn_LastConsolePacketIdx = pInfo->nPacketId;

				DWORD CharCount = pInfo->nDataCount;
				#ifdef _DEBUG
				if (CharCount != (nNewWidth * nNewHeight)) {
					_ASSERTE(CharCount == (nNewWidth * nNewHeight));
				}
				#endif
        		DWORD OneBufferSize = CharCount * sizeof(wchar_t);

				CHAR_INFO *pData = (CHAR_INFO*) (((LPBYTE)pInfo) + pInfo->nDataShift);
	        
				// Проверка размера!
				DWORD nCalcCount = (pInfo->cmd.cbSize - pInfo->nDataShift) / sizeof(CHAR_INFO);
				if (nCalcCount != CharCount) {
					_ASSERTE(nCalcCount == CharCount);
					if (nCalcCount < CharCount)
						CharCount = nCalcCount;
				}

				//MSectionLock sc2; sc2.Lock(&csCON);
				sc.Lock(&csCON);

				MCHKHEAP;
				if (InitBuffers(OneBufferSize)) {
					if (LoadDataFromSrv(CharCount, pData))
						lbChanged = TRUE;
				}
				MCHKHEAP;
			}
		}
	    
		

		TODO("Во время ресайза консоль может подглючивать - отдает не то что нужно...");
		//_ASSERTE(*con.pConChar!=ucBoxDblVert);
	    
		// пока выполяется SetConsoleSizeSrv в другой нити Нельзя сбрасывать эти переменные!
		if (!con.bLockChange2Text)
		{
			con.nChange2TextWidth = -1;
			con.nChange2TextHeight = -1;
		}

		#ifdef _DEBUG
		wchar_t szCursorInfo[60];
		wsprintfW(szCursorInfo, L"Cursor (X=%i, Y=%i, Vis:%i, H:%i)\n", 
			con.m_sbi.dwCursorPosition.X, con.m_sbi.dwCursorPosition.Y,
			con.m_ci.bVisible, con.m_ci.dwSize);
		DEBUGSTRPKT(szCursorInfo);
		// Данные уже должны быть заполнены, и там не должно быть лажы
		if (con.pConChar) {
			BOOL lbDataValid = TRUE; uint n = 0;
			_ASSERTE(con.nTextWidth == con.m_sbi.dwSize.X);
			uint TextLen = con.nTextWidth * con.nTextHeight;
			while (n<TextLen) {
				if (con.pConChar[n] == 0) {
					lbDataValid = FALSE; break;
				} else if (con.pConChar[n] != L' ') {
					// 0 - может быть только для пробела. Иначе символ будет скрытым, чего по идее, быть не должно
					if (con.pConAttr[n] == 0) {
						lbDataValid = FALSE; break;
					}
				}
				n++;
			}
		}
		//_ASSERTE(lbDataValid);
		MCHKHEAP;
		#endif

		if (lbChanged) {
			// По con.m_sbi проверяет, включена ли прокрутка
			CheckBufferSize();
			MCHKHEAP;
		}

		sc.Unlock();
	}


	SetEvent(mh_ApplyFinished);

	if (lbChanged) {
		mb_DataChanged = TRUE; // Переменная используется внутри класса
		con.bConsoleDataChanged = TRUE; // А эта - при вызовах из CVirtualConsole
	}
}

BOOL CRealConsole::LoadDataFromSrv(DWORD CharCount, CHAR_INFO* pData)
{
	MCHKHEAP;
	BOOL lbScreenChanged = FALSE;

	wchar_t* lpChar = con.pConChar;
	WORD* lpAttr = con.pConAttr;
	CONSOLE_SELECTION_INFO sel;
	bool bSelectionPresent = GetConsoleSelectionInfo(&sel);
	#ifdef __GNUC__
	if (bSelectionPresent) bSelectionPresent = bSelectionPresent; // чтобы GCC не ругался
	#endif
	
	//const CESERVER_REQ_CONINFO_DATA* pData = NULL;
	//CESERVER_REQ_HDR cmd; ExecutePrepareCmd(&cmd, CECMD_CONSOLEDATA, sizeof(cmd));
	//DWORD nOutSize = 0;
    
	//if (!m_GetDataPipe.Transact(&cmd, sizeof(cmd), (const CESERVER_REQ_HDR**)&pData, &nOutSize) || !pData) {
	//	#ifdef _DEBUG
	//	MBoxA(m_GetDataPipe.GetErrorText());
	//	#endif
	//	return FALSE;
	//} else
	//if (pData->cmd.cbSize < sizeof(CESERVER_REQ_CONINFO_DATA)) {
	//	_ASSERTE(pData->cmd.cbSize >= sizeof(CESERVER_REQ_CONINFO_DATA));
	//	return FALSE;
	//}
	
	ms_ConStatus[0] = 0;

	//SAFETRY {
	//	// Теоретически, может возникнуть исключение при чтении? когда размер резко увеличивается (maximize)
	//	con.pCmp->crBufSize = mp_ConsoleData->crBufSize;
	//	if ((int)CharCount > (con.pCmp->crBufSize.X*con.pCmp->crBufSize.Y)) {
	//		_ASSERTE((int)CharCount <= (con.pCmp->crBufSize.X*con.pCmp->crBufSize.Y));
	//		CharCount = (con.pCmp->crBufSize.X*con.pCmp->crBufSize.Y);
	//	}
	//	memmove(con.pCmp->Buf, mp_ConsoleData->Buf, CharCount*sizeof(CHAR_INFO));
	//	MCHKHEAP;
	//} SAFECATCH {
	//	_ASSERT(FALSE);
	//}
	
	// Проверка размера!
	//DWORD nHdrSize = (((LPBYTE)pData->Buf)) - ((LPBYTE)pData);
	//DWORD nCalcCount = (pData->cmd.cbSize - nHdrSize) / sizeof(CHAR_INFO);
	//if (nCalcCount != CharCount) {
	//	_ASSERTE(nCalcCount == CharCount);
	//	if (nCalcCount < CharCount)
	//		CharCount = nCalcCount;		
	//}

	lbScreenChanged = memcmp(con.pDataCmp, pData, CharCount*sizeof(CHAR_INFO));
	if (lbScreenChanged)
	{
		//con.pCopy->crBufSize = con.pCmp->crBufSize;
		//memmove(con.pCopy->Buf, con.pCmp->Buf, CharCount*sizeof(CHAR_INFO));
		memmove(con.pDataCmp, pData, CharCount*sizeof(CHAR_INFO));
		MCHKHEAP;
		CHAR_INFO* lpCur = con.pDataCmp;

		//// Когда вернется возможность выделения - нужно сразу применять данные в атрибуты
		//_ASSERTE(!bSelectionPresent); -- не нужно. Все сделает GetConsoleData

		wchar_t ch;
		// Расфуговка буфера CHAR_INFO на текст и атрибуты
		for (DWORD n = 0; n < CharCount; n++, lpCur++) {
			TODO("OPTIMIZE: *(lpAttr++) = lpCur->Attributes;");
			*(lpAttr++) = lpCur->Attributes;

			TODO("OPTIMIZE: ch = lpCur->Char.UnicodeChar;");
			ch = lpCur->Char.UnicodeChar;
			//2009-09-25. Некоторые (старые?) программы умудряются засунуть в консоль символы (ASC<32)
			//            их нужно заменить на юникодные аналоги
			*(lpChar++) = (ch < 32) ? gszAnalogues[(WORD)ch] : ch;
		}
		MCHKHEAP
	}
	return lbScreenChanged;
}

bool CRealConsole::isAlive()
{
	if (!this) return false;

	if (GetFarPID()!=0 && mn_LastFarReadTick /*mn_LastFarReadIdx != (DWORD)-1*/) {
		bool lbAlive = false;
		DWORD nLastReadTick = mn_LastFarReadTick;
		if (nLastReadTick) {
			DWORD nCurTick = GetTickCount();
			DWORD nDelta = nCurTick - nLastReadTick;
			if (nDelta < FAR_ALIVE_TIMEOUT)
				lbAlive = true;
		}
		return lbAlive;
	}

	return true;
}

LPCWSTR CRealConsole::GetConStatus()
{
	return ms_ConStatus;
}

void CRealConsole::SetConStatus(LPCWSTR asStatus)
{
	lstrcpyn(ms_ConStatus, asStatus ? asStatus : L"", countof(ms_ConStatus));
	if (isActive()) {
		if (mp_VCon->Update(false))
			gConEmu.m_Child->Redraw();
	}
}

void CRealConsole::UpdateCursorInfo()
{
	if (!this) return;
	gConEmu.UpdateCursorInfo(con.m_sbi.dwCursorPosition);
}

// может отличаться от CVirtualConsole
bool CRealConsole::isCharBorderVertical(WCHAR inChar)
{
	if ((inChar != ucBoxDblHorz && inChar != ucBoxSinglHorz
			&& (inChar >= ucBoxSinglVert && inChar <= ucBoxDblVertHorz))
		|| (inChar >= ucBox25 && inChar <= ucBox75) || inChar == ucBox100
		|| inChar == ucUpScroll || inChar == ucDnScroll)
        return true;
    else
        return false;
}

bool CRealConsole::isCharBorderLeftVertical(WCHAR inChar)
{
	if (inChar < ucBoxSinglHorz || inChar > ucBoxDblVertHorz)
		return false; // чтобы лишних сравнений не делать

	if (inChar == ucBoxDblVert || inChar == ucBoxSinglVert 
		|| inChar == ucBoxDblDownRight || inChar == ucBoxSinglDownRight
		|| inChar == ucBoxDblVertRight || inChar == ucBoxDblVertSinglRight
		|| inChar == ucBoxSinglVertRight
		|| (inChar >= ucBox25 && inChar <= ucBox75) || inChar == ucBox100
		|| inChar == ucUpScroll || inChar == ucDnScroll)
		return true;
	else
		return false;
}

// может отличаться от CVirtualConsole
bool CRealConsole::isCharBorderHorizontal(WCHAR inChar)
{
	if (inChar == ucBoxSinglDownDblHorz || inChar == ucBoxSinglUpDblHorz
		|| inChar == ucBoxSinglDownHorz || inChar == ucBoxSinglUpHorz
		|| inChar == ucBoxDblHorz)
        return true;
    else
        return false;
}

bool CRealConsole::GetMaxConSize(COORD* pcrMaxConSize)
{
	bool bOk = false;
	//if (mp_ConsoleInfo)
	if (m_ConsoleMap.IsValid())
	{
		if (pcrMaxConSize)
			*pcrMaxConSize = m_ConsoleMap.Ptr()->crMaxConSize;
		bOk = true;
	}
	return bOk;
}

int CRealConsole::GetDetectedDialogs(int anMaxCount, SMALL_RECT* rc, DWORD* rf)
{
	if (!this) return 0;
 	return mp_Rgn->GetDetectedDialogs(anMaxCount, rc, rf);

	//int nCount = min(anMaxCount,m_DetectedDialogs.Count);
	//if (nCount>0) {
	//	if (rc)
	//		memmove(rc, m_DetectedDialogs.Rects, nCount*sizeof(SMALL_RECT));
	//	if (rb)
	//		memmove(rb, m_DetectedDialogs.bWasFrame, nCount*sizeof(bool));
	//}
	//return nCount;
}

const CRgnDetect* CRealConsole::GetDetector()
{
	return mp_Rgn;
}

// Преобразовать абсолютные координаты консоли в координаты нашего буфера
// (вычесть номер верхней видимой строки и скорректировать видимую высоту)
bool CRealConsole::ConsoleRect2ScreenRect(const RECT &rcCon, RECT *prcScr)
{
	if (!this) return false;
	*prcScr = rcCon;
	if (con.bBufferHeight && con.nTopVisibleLine) {
		prcScr->top -= con.nTopVisibleLine;
		prcScr->bottom -= con.nTopVisibleLine;
	}
	
	bool lbRectOK = true;

	if (prcScr->left == 0 && prcScr->right >= con.nTextWidth)
		prcScr->right = con.nTextWidth - 1;
	if (prcScr->left) {
		if (prcScr->left >= con.nTextWidth)
			return false;
		if (prcScr->right >= con.nTextWidth)
			prcScr->right = con.nTextWidth - 1;
	}

	if (prcScr->bottom < 0) {
		lbRectOK = false; // полностью уехал за границу вверх
	} else if (prcScr->top >= con.nTextHeight) {
		lbRectOK = false; // полностью уехал за границу вниз
	} else {
		// Скорректировать по видимому прямоугольнику
		if (prcScr->top < 0)
			prcScr->top = 0;
		if (prcScr->bottom >= con.nTextHeight)
			prcScr->bottom = con.nTextHeight - 1;
		lbRectOK = (prcScr->bottom > prcScr->top);
	}
	return lbRectOK;
}

void CRealConsole::PostMacro(LPCWSTR asMacro)
{
    if (!this || !asMacro || !*asMacro)
        return;
        
	DWORD nPID = GetFarPID(TRUE/*abPluginRequired*/);
	if (!nPID)
		return;

	#ifdef _DEBUG
	DEBUGSTRMACRO(asMacro); OutputDebugStringW(L"\n");
	#endif
	
    CConEmuPipe pipe(nPID, CONEMUREADYTIMEOUT);
    if (pipe.Init(_T("CRealConsole::PostMacro"), TRUE))
    {
        //DWORD cbWritten=0;
        gConEmu.DebugStep(_T("Macro: Waiting for result (10 sec)"));
        pipe.Execute(CMD_POSTMACRO, asMacro, (wcslen(asMacro)+1)*2);
        gConEmu.DebugStep(NULL);
    }
}

void CRealConsole::Detach()
{
	if (!this) return;
	ShowConsole(1);
	// Чтобы случайно не закрыть RealConsole?
	m_Args.bDetached = TRUE;
}

// Запустить Elevated копию фара с теми же папками на панелях
void CRealConsole::AdminDuplicate()
{
	if (!this) return;

	TODO("Запустить Elevated фар с параметрами - текущими путями");
}

const CEFAR_INFO* CRealConsole::GetFarInfo()
{
	if (!this) return NULL;
	return m_FarInfo.Ptr();
}

/*LPCWSTR CRealConsole::GetLngNameTime()
{
	if (!this) return NULL;
	return ms_NameTitle;
}*/

BOOL CRealConsole::InCreateRoot()
{
	return (mb_InCreateRoot && !mn_ConEmuC_PID);
}
