
/*
Copyright (c) 2009-2013 Maximus5
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

#undef VALIDATE_AND_DELAY_ON_TERMINATE

#ifdef _DEBUG
//  Раскомментировать, чтобы сразу после запуска процесса (conemuc.exe) показать MessageBox, чтобы прицепиться дебаггером
//	#define SHOW_STARTED_MSGBOX
//	#define SHOW_ALTERNATIVE_MSGBOX
//  #define SHOW_DEBUG_STARTED_MSGBOX
//	#define SHOW_COMSPEC_STARTED_MSGBOX
//	#define SHOW_SERVER_STARTED_MSGBOX
//  #define SHOW_STARTED_ASSERT
//  #define SHOW_STARTED_PRINT
//	#define SHOW_STARTED_PRINT_LITE
	#define SHOW_EXITWAITKEY_MSGBOX
//	#define SHOW_INJECT_MSGBOX
//	#define SHOW_INJECTREM_MSGBOX
//	#define SHOW_ATTACH_MSGBOX
//  #define SHOW_ROOT_STARTED
	#define WINE_PRINT_PROC_INFO
//	#define USE_PIPE_DEBUG_BOXES

//	#define DEBUG_ISSUE_623

//	#define VALIDATE_AND_DELAY_ON_TERMINATE

#elif defined(__GNUC__)
//  Раскомментировать, чтобы сразу после запуска процесса (conemuc.exe) показать MessageBox, чтобы прицепиться дебаггером
//  #define SHOW_STARTED_MSGBOX
#else
//
#endif

#define SHOWDEBUGSTR
#define DEBUGSTRCMD(x) DEBUGSTR(x)
#define DEBUGSTRFIN(x) DEBUGSTR(x)
#define DEBUGSTRCP(x) DEBUGSTR(x)

//#define SHOW_INJECT_MSGBOX

#include "ConEmuC.h"
#include "../ConEmu/version.h"
#include "../common/execute.h"
#include "../ConEmuHk/Injects.h"
#include "../common/MArray.h"
#include "../common/MMap.h"
#include "../common/RConStartArgs.h"
#include "../common/ConsoleAnnotation.h"
#include "../common/ConsoleRead.h"
#include "../common/WinConsole.h"
#include "../common/CmdLine.h"
#include "ConsoleHelp.h"
#include "Debugger.h"
#include "UnicodeTest.h"

#define FULL_STARTUP_ENV
#include "../common/StartupEnv.h"

#pragma comment(lib, "shlwapi.lib")


WARNING("Обязательно после запуска сделать apiSetForegroundWindow на GUI окно, если в фокусе консоль");
WARNING("Обязательно получить код и имя родительского процесса");

#ifdef USEPIPELOG
namespace PipeServerLogger
{
    Event g_events[BUFFER_SIZE];
    LONG g_pos = -1;
}
#endif

#ifdef _DEBUG
wchar_t gszDbgModLabel[6] = {0};
#endif

WARNING("!!!! Пока можно при появлении события запоминать текущий тик");
// и проверять его в RefreshThread. Если он не 0 - и дельта больше (100мс?)
// то принудительно перечитать консоль и сбросить тик в 0.


WARNING("Наверное все-же стоит производить периодические чтения содержимого консоли, а не только по событию");

WARNING("Стоит именно здесь осуществлять проверку живости GUI окна (если оно было). Ведь может быть запущен не far, а CMD.exe");

WARNING("Если GUI умер, или не подцепился по таймауту - показать консольное окно и наверное установить шрифт поболе");

WARNING("В некоторых случаях не срабатывает ни EVENT_CONSOLE_UPDATE_SIMPLE ни EVENT_CONSOLE_UPDATE_REGION");
// Пример. Запускаем cmd.exe. печатаем какую-то муйню в командной строке и нажимаем 'Esc'
// При Esc никаких событий ВООБЩЕ не дергается, а экран в консоли изменился!



FGetConsoleKeyboardLayoutName pfnGetConsoleKeyboardLayoutName = NULL;
FGetConsoleProcessList pfnGetConsoleProcessList = NULL;
FDebugActiveProcessStop pfnDebugActiveProcessStop = NULL;
FDebugSetProcessKillOnExit pfnDebugSetProcessKillOnExit = NULL;


/* Console Handles */
//MConHandle ghConIn ( L"CONIN$" );
MConHandle ghConOut(L"CONOUT$");

// Время ожидания завершения консольных процессов, когда юзер нажал крестик в КОНСОЛЬНОМ окне
// The system also displays this dialog box if the process does not respond within a certain time-out period 
// (5 seconds for CTRL_CLOSE_EVENT, and 20 seconds for CTRL_LOGOFF_EVENT or CTRL_SHUTDOWN_EVENT).
// Поэтому ждать пытаемся - не больше 4 сек
#define CLOSE_CONSOLE_TIMEOUT 4000

/*  Global  */
CEStartupEnv* gpStartEnv = NULL;
HMODULE ghOurModule = NULL; // ConEmuCD.dll
DWORD   gnSelfPID = 0;
BOOL    gbTerminateOnExit = FALSE;
//HANDLE  ghConIn = NULL, ghConOut = NULL;
HWND    ghConWnd = NULL;
HWND    ghConEmuWnd = NULL; // Root! window
HWND    ghConEmuWndDC = NULL; // ConEmu DC window
HWND    ghConEmuWndBack = NULL; // ConEmu Back window
DWORD   gnMainServerPID = 0;
DWORD   gnAltServerPID = 0;
BOOL    gbLogProcess = FALSE;
BOOL    gbWasBufferHeight = FALSE;
BOOL    gbNonGuiMode = FALSE;
DWORD   gnExitCode = 0;
HANDLE  ghRootProcessFlag = NULL;
HANDLE  ghExitQueryEvent = NULL; int nExitQueryPlace = 0, nExitPlaceStep = 0;
SetTerminateEventPlace gTerminateEventPlace = ste_None;
HANDLE  ghQuitEvent = NULL;
bool    gbQuit = false;
BOOL	gbInShutdown = FALSE;
BOOL	gbInExitWaitForKey = FALSE;
BOOL	gbStopExitWaitForKey = FALSE;
BOOL    gbCtrlBreakStopWaitingShown = FALSE;
BOOL	gbTerminateOnCtrlBreak = FALSE;
int     gnConfirmExitParm = 0; // 1 - CONFIRM, 2 - NOCONFIRM
BOOL    gbAlwaysConfirmExit = FALSE;
BOOL	gbAutoDisableConfirmExit = FALSE; // если корневой процесс проработал достаточно (10 сек) - будет сброшен gbAlwaysConfirmExit
BOOL    gbRootAliveLess10sec = FALSE; // корневой процесс проработал менее CHECK_ROOTOK_TIMEOUT
int     gbRootWasFoundInCon = 0;
AttachModeEnum gbAttachMode = am_None; // сервер запущен НЕ из conemu.exe (а из плагина, из CmdAutoAttach, или -new_console, или /GUIATTACH)
BOOL    gbAlienMode = FALSE;  // сервер НЕ является владельцем консоли (корневым процессом этого консольного окна)
BOOL    gbForceHideConWnd = FALSE;
DWORD   gdwMainThreadId = 0;
wchar_t* gpszRunCmd = NULL;
LPCWSTR gpszCheck4NeedCmd = NULL; // Для отладки
wchar_t gszComSpec[MAX_PATH+1] = {0};
BOOL    gbRunInBackgroundTab = FALSE;
BOOL    gbRunViaCmdExe = FALSE;
DWORD   gnImageSubsystem = 0, gnImageBits = 32;
//HANDLE  ghCtrlCEvent = NULL, ghCtrlBreakEvent = NULL;
//HANDLE ghHeap = NULL; //HeapCreate(HEAP_GENERATE_EXCEPTIONS, nMinHeapSize, 0);
#ifdef _DEBUG
size_t gnHeapUsed = 0, gnHeapMax = 0;
HANDLE ghFarInExecuteEvent;
#endif

RunMode gnRunMode = RM_UNDEFINED;

BOOL  gbDumpServerInitStatus = FALSE;
BOOL  gbNoCreateProcess = FALSE;
BOOL  gbDontInjectConEmuHk = FALSE;
int   gnCmdUnicodeMode = 0;
BOOL  gbUseDosBox = FALSE; HANDLE ghDosBoxProcess = NULL; DWORD gnDosBoxPID = 0;
UINT  gnPTYmode = 0; // 1 enable PTY, 2 - disable PTY (work as plain console), 0 - don't change
BOOL  gbRootIsCmdExe = TRUE;
BOOL  gbAttachFromFar = FALSE;
BOOL  gbAlternativeAttach = FALSE; // Подцепиться к существующей консоли, без внедрения в процесс ConEmuHk.dll
BOOL  gbSkipWowChange = FALSE;
BOOL  gbConsoleModeFlags = TRUE;
DWORD gnConsoleModeFlags = 0; //(ENABLE_QUICK_EDIT_MODE|ENABLE_INSERT_MODE);
WORD  gnDefTextColors = 0, gnDefPopupColors = 0; // Передаются через "/TA=..."
BOOL  gbVisibleOnStartup = FALSE;
OSVERSIONINFO gOSVer;
WORD gnOsVer = 0x500;
bool gbIsWine = false;
bool gbIsDBCS = false;


SrvInfo* gpSrv = NULL;

//#pragma pack(push, 1)
//CESERVER_CONSAVE* gpStoredOutput = NULL;
//#pragma pack(pop)
//MSection* gpcsStoredOutput = NULL;

//CmdInfo* gpSrv = NULL;

COORD gcrVisibleSize = {80,25}; // gcrBufferSize переименован в gcrVisibleSize
BOOL  gbParmVisibleSize = FALSE;
BOOL  gbParmBufSize = FALSE;
SHORT gnBufferHeight = 0;
SHORT gnBufferWidth = 0; // Определяется в MyGetConsoleScreenBufferInfo
wchar_t* gpszPrevConTitle = NULL;

MFileLog* gpLogSize = NULL;


BOOL gbInRecreateRoot = FALSE;


namespace InputLogger
{
	Event g_evt[BUFFER_INFO_SIZE];
	LONG g_evtidx = -1;
	LONG g_overflow = 0;
};



void ShutdownSrvStep(LPCWSTR asInfo, int nParm1 /*= 0*/, int nParm2 /*= 0*/, int nParm3 /*= 0*/, int nParm4 /*= 0*/)
{
#ifdef SHOW_SHUTDOWNSRV_STEPS
	static int nDbg = 0;
	if (!nDbg)
		nDbg = IsDebuggerPresent() ? 1 : 2;
	if (nDbg != 1)
		return;
	wchar_t szFull[512];
	msprintf(szFull, countof(szFull), L"%u:ConEmuC:PID=%u:TID=%u: ",
		GetTickCount(), GetCurrentProcessId(), GetCurrentThreadId());
	if (asInfo)
	{
		int nLen = lstrlen(szFull);
		msprintf(szFull+nLen, countof(szFull)-nLen, asInfo, nParm1, nParm2, nParm3, nParm4);
	}
	lstrcat(szFull, L"\n");
	OutputDebugString(szFull);
#endif
}


#if defined(__GNUC__)
extern "C" {
	BOOL WINAPI DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved);
};
#endif

//extern UINT_PTR gfnLoadLibrary;
//UINT gnMsgActivateCon = 0;
UINT gnMsgSwitchCon = 0;
UINT gnMsgHookedKey = 0;
//UINT gnMsgConsoleHookedKey = 0;

BOOL WINAPI DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch(ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		{
			ghOurModule = (HMODULE)hModule;

			DWORD nImageBits = WIN3264TEST(32,64), nImageSubsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
			GetImageSubsystem(nImageSubsystem,nImageBits);
			if (nImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI)
				ghConWnd = NULL;
			else
				ghConWnd = GetConEmuHWND(2);

			gnSelfPID = GetCurrentProcessId();
			ghWorkingModule = (u64)hModule;

			#ifdef _DEBUG
			HANDLE hProcHeap = GetProcessHeap();
			gAllowAssertThread = am_Pipe;
			#endif

			#ifdef SHOW_STARTED_MSGBOX
			if (!IsDebuggerPresent())
			{
				char szMsg[128]; msprintf(szMsg, countof(szMsg), WIN3264TEST("ConEmuCD.dll","ConEmuCD64.dll") " loaded, PID=%u, TID=%u", GetCurrentProcessId(), GetCurrentThreadId());
				MessageBoxA(NULL, szMsg, "ConEmu server" WIN3264TEST(""," x64"), 0);
			}
			#endif
			#ifdef _DEBUG
			DWORD dwConMode = -1;
			GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &dwConMode);
			#endif

			//_ASSERTE(ghHeap == NULL);
			//ghHeap = HeapCreate(HEAP_GENERATE_EXCEPTIONS, 200000, 0);
			HeapInitialize();
			
			/* *** DEBUG PURPOSES */
			gpStartEnv = LoadStartupEnv();
			/* *** DEBUG PURPOSES */

			// Поскольку процедура в принципе может быть кем-то перехвачена, сразу найдем адрес
			// iFindAddress = FindKernelAddress(pi.hProcess, pi.dwProcessId, &fLoadLibrary);
			//gfnLoadLibrary = (UINT_PTR)::GetProcAddress(::GetModuleHandle(L"kernel32.dll"), "LoadLibraryW");
			GetLoadLibraryAddress(); // Загрузить gfnLoadLibrary
			
			//#ifndef TESTLINK
			gpLocalSecurity = LocalSecurity();
			//gnMsgActivateCon = RegisterWindowMessage(CONEMUMSG_ACTIVATECON);
			gnMsgSwitchCon = RegisterWindowMessage(CONEMUMSG_SWITCHCON);
			gnMsgHookedKey = RegisterWindowMessage(CONEMUMSG_HOOKEDKEY);
			//gnMsgConsoleHookedKey = RegisterWindowMessage(CONEMUMSG_CONSOLEHOOKEDKEY);
			//#endif
			//wchar_t szSkipEventName[128];
			//_wsprintf(szSkipEventName, SKIPLEN(countof(szSkipEventName)) CEHOOKDISABLEEVENT, GetCurrentProcessId());
			//HANDLE hSkipEvent = OpenEvent(EVENT_ALL_ACCESS , FALSE, szSkipEventName);
			////BOOL lbSkipInjects = FALSE;

			//if (hSkipEvent)
			//{
			//	gbSkipInjects = (WaitForSingleObject(hSkipEvent, 0) == WAIT_OBJECT_0);
			//	CloseHandle(hSkipEvent);
			//}
			//else
			//{
			//	gbSkipInjects = FALSE;
			//}

			// Открыть мэппинг консоли и попытаться получить HWND GUI, PID сервера, и пр...
			if (ghConWnd)
			{
				MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> ConInfo;
				ConInfo.InitName(CECONMAPNAME, (DWORD)ghConWnd); //-V205
				CESERVER_CONSOLE_MAPPING_HDR *pInfo = ConInfo.Open();
				if (pInfo)
				{
					if (pInfo->cbSize >= sizeof(CESERVER_CONSOLE_MAPPING_HDR))
					{
						if (pInfo->hConEmuRoot && IsWindow(pInfo->hConEmuRoot))
						{
							ghConEmuWnd = pInfo->hConEmuRoot;
							if (pInfo->hConEmuWndDc && IsWindow(pInfo->hConEmuWndDc)
								&& pInfo->hConEmuWndBack && IsWindow(pInfo->hConEmuWndBack))
							{
								SetConEmuWindows(pInfo->hConEmuWndDc, pInfo->hConEmuWndBack);
							}
						}
						if (pInfo->nServerPID && pInfo->nServerPID != gnSelfPID)
						{
							gnMainServerPID = pInfo->nServerPID;
							gnAltServerPID = pInfo->nAltServerPID;
						}

						gbLogProcess = (pInfo->nLoggingType == glt_Processes);
						if (gbLogProcess)
						{
							wchar_t szExeName[MAX_PATH], szDllName[MAX_PATH]; szExeName[0] = szDllName[0] = 0;
							GetModuleFileName(NULL, szExeName, countof(szExeName));
							GetModuleFileName((HMODULE)hModule, szDllName, countof(szDllName));
							int ImageBits = 0, ImageSystem = 0;
							#ifdef _WIN64
							ImageBits = 64;
							#else
							ImageBits = 32;
							#endif
							CESERVER_REQ* pIn = ExecuteNewCmdOnCreate(pInfo, ghConWnd, eSrvLoaded,
								L"", szExeName, szDllName, NULL, NULL, NULL, NULL, 
								ImageBits, ImageSystem, 
								GetStdHandle(STD_INPUT_HANDLE), GetStdHandle(STD_OUTPUT_HANDLE), GetStdHandle(STD_ERROR_HANDLE));
							if (pIn)
							{
								CESERVER_REQ* pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
								ExecuteFreeResult(pIn);
								if (pOut) ExecuteFreeResult(pOut);
							}
						}

						ConInfo.CloseMap();
					}
					else
					{
						_ASSERTE(pInfo->cbSize == sizeof(CESERVER_CONSOLE_MAPPING_HDR));
					}
				}
			}

			//if (!gbSkipInjects && ghConWnd)
			//{
			//	InitializeConsoleInputSemaphore();
			//}
			
			
			//#ifdef _WIN64
			//DWORD nImageBits = 64, nImageSubsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
			//#else
			//DWORD nImageBits = 32, nImageSubsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
			//#endif
			//GetImageSubsystem(nImageSubsystem,nImageBits);

			//CShellProc sp;
			//if (sp.LoadGuiMapping())
			//{
			//	wchar_t szExeName[MAX_PATH+1]; //, szBaseDir[MAX_PATH+2];
			//	//BOOL lbDosBoxAllowed = FALSE;
			//	if (!GetModuleFileName(NULL, szExeName, countof(szExeName))) szExeName[0] = 0;
			//	CESERVER_REQ* pIn = sp.NewCmdOnCreate(
			//		gbSkipInjects ? eHooksLoaded : eInjectingHooks,
			//		L"", szExeName, GetCommandLineW(),
			//		NULL, NULL, NULL, NULL, // flags
			//		nImageBits, nImageSubsystem,
			//		GetStdHandle(STD_INPUT_HANDLE), GetStdHandle(STD_OUTPUT_HANDLE), GetStdHandle(STD_ERROR_HANDLE));
			//	if (pIn)
			//	{
			//		//HWND hConWnd = GetConEmuHWND(2);
			//		CESERVER_REQ* pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
			//		ExecuteFreeResult(pIn);
			//		if (pOut) ExecuteFreeResult(pOut);
			//	}
			//}

			//if (!gbSkipInjects)
			//{
			//	#ifdef _DEBUG
			//	wchar_t szModule[MAX_PATH+1]; szModule[0] = 0;
			//	#endif

			//	#ifdef SHOW_INJECT_MSGBOX
			//	wchar_t szDbgMsg[1024], szTitle[128];//, szModule[MAX_PATH];
			//	if (!GetModuleFileName(NULL, szModule, countof(szModule)))
			//		wcscpy_c(szModule, L"GetModuleFileName failed");
			//	_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuHk, PID=%u", GetCurrentProcessId());
			//	_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"SetAllHooks, ConEmuHk, PID=%u\n%s", GetCurrentProcessId(), szModule);
			//	MessageBoxW(NULL, szDbgMsg, szTitle, MB_SYSTEMMODAL);
			//	#endif
			//	gnRunMode = RM_APPLICATION;

			//	#ifdef _DEBUG
			//	//wchar_t szModule[MAX_PATH+1]; szModule[0] = 0;
			//	GetModuleFileName(NULL, szModule, countof(szModule));
			//	const wchar_t* pszName = PointToName(szModule);
			//	_ASSERTE((nImageSubsystem==IMAGE_SUBSYSTEM_WINDOWS_CUI) || (lstrcmpi(pszName, L"DosBox.exe")==0));
			//	//if (!lstrcmpi(pszName, L"far.exe") || !lstrcmpi(pszName, L"mingw32-make.exe"))
			//	//if (!lstrcmpi(pszName, L"as.exe"))
			//	//	MessageBoxW(NULL, L"as.exe loaded!", L"ConEmuHk", MB_SYSTEMMODAL);
			//	//else if (!lstrcmpi(pszName, L"cc1plus.exe"))
			//	//	MessageBoxW(NULL, L"cc1plus.exe loaded!", L"ConEmuHk", MB_SYSTEMMODAL);
			//	//else if (!lstrcmpi(pszName, L"mingw32-make.exe"))
			//	//	MessageBoxW(NULL, L"mingw32-make.exe loaded!", L"ConEmuHk", MB_SYSTEMMODAL);
			//	//if (!lstrcmpi(pszName, L"g++.exe"))
			//	//	MessageBoxW(NULL, L"g++.exe loaded!", L"ConEmuHk", MB_SYSTEMMODAL);
			//	//{
			//	#endif

			//	gbHooksWasSet = StartupHooks(ghOurModule);

			//	#ifdef _DEBUG
			//	//}
			//	#endif

			//	// Если NULL - значит это "Detached" консольный процесс, посылать "Started" в сервер смысла нет
			//	if (ghConWnd != NULL)
			//	{
			//		SendStarted();
			//		//#ifdef _DEBUG
			//		//// Здесь это приводит к обвалу _chkstk,
			//		//// похоже из-за того, что dll-ка загружена НЕ из известных модулей,
			//		//// а из специально сформированного блока памяти
			//		// -- в одной из функций, под локальные переменные выделялось слишком много памяти
			//		// -- переделал в malloc/free, все заработало
			//		//TestShellProcessor();
			//		//#endif
			//	}
			//}
			//else
			//{
			//	gbHooksWasSet = FALSE;
			//}
		}
		break;
		case DLL_PROCESS_DETACH:
		{
			ShutdownSrvStep(L"DLL_PROCESS_DETACH");
			//if (!gbSkipInjects && gbHooksWasSet)
			//{
			//	gbHooksWasSet = FALSE;
			//	ShutdownHooks();
			//}


			if ((gnRunMode == RM_APPLICATION) || (gnRunMode == RM_ALTSERVER))
			{
				SendStopped();
			}
			//else if (gnRunMode == RM_ALTSERVER)
			//{
			//	WARNING("RM_ALTSERVER тоже должен посылать уведомление в главный сервер о своем завершении");
			//	// Но пока - оставим, для отладки ситуации, когда процесс завершается аварийно (Kill).
			//	_ASSERTE(gnRunMode != RM_ALTSERVER && "AltServer must inform MainServer about self-termination");
			//}

			//#ifndef TESTLINK
			CommonShutdown();

			//ReleaseConsoleInputSemaphore();

			//#endif
			//if (ghHeap)
			//{
			//	HeapDestroy(ghHeap);
			//	ghHeap = NULL;
			//}
			HeapDeinitialize();

			ShutdownSrvStep(L"DLL_PROCESS_DETACH done");
		}
		break;
	}

	return TRUE;
}

//#if defined(CRTSTARTUP)
//extern "C" {
//	BOOL WINAPI _DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved);
//};
//
//BOOL WINAPI _DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
//{
//	DllMain(hDll, dwReason, lpReserved);
//	return TRUE;
//}
//#endif

#ifdef _DEBUG
void OnProcessCreatedDbg(BOOL bRc, DWORD dwErr, LPPROCESS_INFORMATION pProcessInformation, LPSHELLEXECUTEINFO pSEI)
{
	int iDbg = 0;
#ifdef SHOW_ROOT_STARTED
	if (bRc)
	{
		wchar_t szTitle[64], szMsg[128];
		_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuSrv, PID=%u", GetCurrentProcessId());
		_wsprintf(szMsg, SKIPLEN(countof(szMsg)) L"Root process started, PID=%u", pProcessInformation->dwProcessId);
		MessageBox(NULL,szMsg,szTitle,0);
	}
#endif
}
#endif

BOOL createProcess(BOOL abSkipWowChange, LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
	MWow64Disable wow;
	if (!abSkipWowChange)
		wow.Disable();

	wchar_t* pszOrigPath = NULL;

	// We need to check [HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths]
	// when starting our "root" or "comspec" command
	if (lpApplicationName == NULL)
	{
		LPCWSTR pszTemp = lpCommandLine;
		wchar_t szExe[MAX_PATH+1];
		if (NextArg(&pszTemp, szExe) == 0)
		{
			LPCWSTR pszName = PointToName(szExe);
			if (!pszName || !*pszName)
			{
				_ASSERTE(pszName && *pszName);
			}
			else
			{
				DWORD nRegFlags = KEY_READ;

				//Seems that key does not have both (32-bit and 64-bit) versions
				//if (IsWindows64())
				//{
				//}

				bool bFound = false;
				DWORD cchMax = 65537;
				wchar_t* pszValue = (wchar_t*)calloc(cchMax,sizeof(*pszValue));
				if (!pszValue)
				{
					SetLastError(ERROR_NOT_ENOUGH_MEMORY);
					return FALSE;
				}

				wchar_t szKey[MAX_PATH*2];
				wcscpy_c(szKey, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\");
				wcscat_c(szKey, pszName);

				HKEY hk;
				for (int i = 0; !bFound && (i <= 1); i++)
				{
					LONG lRc = RegOpenKeyEx(
						(!i) ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, szKey, 0,
						(!i) ? KEY_READ : nRegFlags, &hk);
					if (lRc != 0)
					{
						// Приложение не зарегистрировано
						continue;
					}

					DWORD nSize = (cchMax-2)*sizeof(*pszValue);
					lRc = RegQueryValueEx(hk, L"Path", NULL, NULL, (LPBYTE)pszValue, &nSize);
					if (lRc == 0)
					{
						bFound = true;

						if (*pszValue)
						{
							int nLen = lstrlen(pszValue);
							if (pszValue[nLen-1] != L';')
							{
								pszValue[nLen++] = L';';
								_ASSERTE(pszValue[nLen] == 0);
							}

							// Получить текущий PATH и добавить в его начало дополнительные пути
							DWORD cchMax = 65535;
							pszOrigPath = (wchar_t*)calloc(cchMax, sizeof(*pszOrigPath));
							if (!pszOrigPath)
							{
								RegCloseKey(hk);
								free(pszValue);
								SetLastError(ERROR_NOT_ENOUGH_MEMORY);
								return FALSE;
							}
							DWORD nEnvLen = GetEnvironmentVariable(L"PATH", pszOrigPath, cchMax);
							if (nEnvLen > 0)
							{
								wchar_t* pszNewPath = lstrmerge(pszValue, pszOrigPath);
								if (!pszNewPath)
								{
									RegCloseKey(hk);
									free(pszValue);
									SetLastError(ERROR_NOT_ENOUGH_MEMORY);
									return FALSE;
								}
								SetEnvironmentVariable(L"PATH", pszNewPath);
								free(pszNewPath);
							}
							else
							{
								SafeFree(pszOrigPath);
							}
						}
					}

					RegCloseKey(hk);
				}

				free(pszValue);
			}
		}
	}

#if defined(SHOW_STARTED_PRINT_LITE)
	DWORD nStartTick = GetTickCount();
	if (gnRunMode == RM_SERVER)
	{
		_printf("Starting root: ");
		_wprintf(lpCommandLine ? lpCommandLine : lpApplicationName ? lpApplicationName : L"<NULL>");
	}
#endif

	SetLastError(0);

	BOOL lbRc = CreateProcess(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	DWORD dwErr = GetLastError();

#if defined(SHOW_STARTED_PRINT_LITE)
	DWORD nStartDuration = GetTickCount() - nStartTick;
	if (gnRunMode == RM_SERVER)
	{
		if (lbRc)
			_printf("\nSuccess (%u ms), PID=%u\n\n", nStartDuration, lpProcessInformation->dwProcessId);
		else
			_printf("\nFailed (%u ms), Error code=%u(x%04X)\n", nStartDuration, dwErr, dwErr);
	}
#endif

	#ifdef _DEBUG
	OnProcessCreatedDbg(lbRc, dwErr, lpProcessInformation, NULL);
	#endif

	if (!abSkipWowChange)
	{
		wow.Restore();
	}

	if (pszOrigPath && *pszOrigPath)
	{
		SetEnvironmentVariable(L"PATH", pszOrigPath);
		free(pszOrigPath);
	}

	SetLastError(dwErr);
	return lbRc;
}

#if defined(__GNUC__)
extern "C" {
	int __stdcall ConsoleMain2(int anWorkMode/*0-Server,1-AltServer,2-Reserved*/);
};
#endif

// Возвращает текст с информацией о пути к сохраненному дампу
DWORD CreateDumpForReport(LPEXCEPTION_POINTERS ExceptionInfo, wchar_t (&szFullInfo)[1024]);
#include "../common/Dump.h"

LPTOP_LEVEL_EXCEPTION_FILTER gpfnPrevExFilter = NULL;
LONG WINAPI CreateDumpOnException(LPEXCEPTION_POINTERS ExceptionInfo)
{
	wchar_t szFull[1024] = L"";
	DWORD dwErr = CreateDumpForReport(ExceptionInfo, szFull);
	wchar_t szAdd[1200];
	wcscpy_c(szAdd, szFull);
	wcscat_c(szAdd, L"\r\n\r\nPress <Yes> to copy this text to clipboard\r\nand open project web page");
	wchar_t szTitle[100], szExe[MAX_PATH] = L"", *pszExeName;
	GetModuleFileName(NULL, szExe, countof(szExe));
	pszExeName = (wchar_t*)PointToName(szExe);
	if (pszExeName && lstrlen(pszExeName) > 63) pszExeName[63] = 0;
	_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"%s crashed, PID=%u", pszExeName ? pszExeName : L"<process>", GetCurrentProcessId());

	int nBtn = MessageBox(NULL, szAdd, szTitle, MB_YESNO|MB_ICONSTOP|MB_SYSTEMMODAL);
	if (nBtn == IDYES)
	{
		CopyToClipboard(szFull);
		ShellExecute(NULL, L"open", CEREPORTCRASH, NULL, NULL, SW_SHOWNORMAL);
	}

	LONG lExRc = EXCEPTION_EXECUTE_HANDLER;

	if (gpfnPrevExFilter)
	{
		// если фильтр уже был установлен перед нашим - будем звать его
		// все-равно уже свалились, на валидность адреса можно не проверяться
        lExRc = gpfnPrevExFilter(ExceptionInfo);
	}

	return lExRc;
}

void SetupCreateDumpOnException()
{
	_ASSERTE(gnRunMode == RM_ALTSERVER);

	// Far 3.x, telnet, Vim, etc.
	// В этих программах ConEmuCD.dll может загружаться для работы с альтернативными буферами и TrueColor
	if (!gpfnPrevExFilter && !IsDebuggerPresent())
	{
		// Сохраним, если фильтр уже был установлен - будем звать его из нашей функции
		gpfnPrevExFilter = SetUnhandledExceptionFilter(CreateDumpOnException);
	}
}


// Main entry point for ConEmuC.exe
int __stdcall ConsoleMain2(int anWorkMode/*0-Server&ComSpec,1-AltServer,2-Reserved*/)
{
	TODO("можно при ошибках показать консоль, предварительно поставив 80x25 и установив крупный шрифт");
	
	//WARNING("После создания AltServer - проверить в ConEmuCD все условия на RM_SERVER!!!");
	//_ASSERTE(anWorkMode==FALSE);

	//#ifdef _DEBUG
	//InitializeCriticalSection(&gcsHeap);
	//#endif

	if (!anWorkMode)
	{
		if (!IsDebuggerPresent())
		{
			// Наш exe-шник, gpfnPrevExFilter не нужен
			SetUnhandledExceptionFilter(CreateDumpOnException);
		}

		HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleMode(hOut, ENABLE_PROCESSED_OUTPUT|ENABLE_WRAP_AT_EOL_OUTPUT);

		WARNING("Этот атрибут нужно задавать в настройках GUI!");
		#if 0
		SetConsoleTextAttribute(hOut, 7);
		#endif
	}
	#if 0
	// Issue 1183, 1188, 1189: Exception filter вызывается (некорректно?) при обработке EMenu или закрытии NetBox
	else if (anWorkMode == 1)
	{
		// Far 3.x, telnet, Vim, etc.
		// В этих программах ConEmuCD.dll может загружаться для работы с альтернативными буферами и TrueColor
		if (!IsDebuggerPresent())
		{
			// Сохраним, если фильтр уже был установлен - будем звать его из нашей функции
			gpfnPrevExFilter = SetUnhandledExceptionFilter(CreateDumpOnException);
		}
	}
	#endif


	// На всякий случай - сбросим
	gnRunMode = RM_UNDEFINED;

	if (ghOurModule == NULL)
	{
		wchar_t szTitle[128]; _wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuHk, PID=%u", GetCurrentProcessId());
		MessageBox(NULL, L"ConsoleMain2. ghOurModule is NULL\nDllMain was not executed", szTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
		return CERR_DLLMAIN_SKIPPED;
	}

	gpSrv = (SrvInfo*)calloc(sizeof(SrvInfo),1);
	if (gpSrv)
	{
		gpSrv->InitFields();
		
		if (ghConEmuWnd)
		{
			GetWindowThreadProcessId(ghConEmuWnd, &gpSrv->dwGuiPID);
		}
	}

	RemoveOldComSpecC();

	//if (ghHeap == NULL)
	//{
	//	#ifdef _DEBUG
	//	_ASSERTE(ghHeap != NULL);
	//	#else
	//	wchar_t szTitle[128]; swprintf_c(szTitle, L"ConEmuHk, PID=%u", GetCurrentProcessId());
	//	MessageBox(NULL, L"ConsoleMain2. ghHeap is NULL", szTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
	//	#endif
	//	return CERR_NOTENOUGHMEM1;
	//}

#if defined(SHOW_STARTED_PRINT)
	BOOL lbDbgWrite; DWORD nDbgWrite; HANDLE hDbg; char szDbgString[255], szHandles[128];
	sprintf(szDbgString, "ConEmuC: PID=%u", GetCurrentProcessId());
	MessageBoxA(0, GetCommandLineA(), szDbgString, MB_SYSTEMMODAL);
	hDbg = CreateFile(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
	                  0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	sprintf(szHandles, "STD_OUTPUT_HANDLE(0x%08X) STD_ERROR_HANDLE(0x%08X) CONOUT$(0x%08X)",
	        (DWORD)GetStdHandle(STD_OUTPUT_HANDLE), (DWORD)GetStdHandle(STD_ERROR_HANDLE), (DWORD)hDbg);
	printf("ConEmuC: Printf: %s\n", szHandles);
	sprintf(szDbgString, "ConEmuC: STD_OUTPUT_HANDLE: %s\n", szHandles);
	lbDbgWrite = WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), szDbgString, lstrlenA(szDbgString), &nDbgWrite, NULL);
	sprintf(szDbgString, "ConEmuC: STD_ERROR_HANDLE:  %s\n", szHandles);
	lbDbgWrite = WriteFile(GetStdHandle(STD_ERROR_HANDLE), szDbgString, lstrlenA(szDbgString), &nDbgWrite, NULL);
	sprintf(szDbgString, "ConEmuC: CONOUT$: %s", szHandles);
	lbDbgWrite = WriteFile(hDbg, szDbgString, lstrlenA(szDbgString), &nDbgWrite, NULL);
	CloseHandle(hDbg);
	//sprintf(szDbgString, "ConEmuC: PID=%u", GetCurrentProcessId());
	//MessageBoxA(0, "Press Ok to continue", szDbgString, MB_SYSTEMMODAL);
#elif defined(SHOW_STARTED_PRINT_LITE)
	{
	wchar_t szPath[MAX_PATH]; GetModuleFileNameW(NULL, szPath, countof(szPath));
	wchar_t szDbgMsg[MAX_PATH*2];
	_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"%s started, PID=%u\n", PointToName(szPath), GetCurrentProcessId());
	_wprintf(szDbgMsg);
	gbDumpServerInitStatus = TRUE;
	}
#endif

	int iRc = 100;
	PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
	STARTUPINFOW si = {sizeof(STARTUPINFOW)};
	// ConEmuC должен быть максимально прозрачен для конечного процесса
	GetStartupInfo(&si);
	DWORD dwErr = 0, nWait = 0, nWaitExitEvent = -1, nWaitDebugExit = -1, nWaitComspecExit = -1;
	DWORD dwWaitGui = -1, dwWaitRoot = -1;
	BOOL lbRc = FALSE;
	//DWORD mode = 0;
	//BOOL lb = FALSE;
	//ghHeap = HeapCreate(HEAP_GENERATE_EXCEPTIONS, 200000, 0);
	memset(&gOSVer, 0, sizeof(gOSVer));
	gOSVer.dwOSVersionInfoSize = sizeof(gOSVer);
	GetVersionEx(&gOSVer);
	gnOsVer = ((gOSVer.dwMajorVersion & 0xFF) << 8) | (gOSVer.dwMinorVersion & 0xFF);

	gbIsWine = IsWine(); // В общем случае, на флажок ориентироваться нельзя. Это для информации.
	gbIsDBCS = IsDbcs();

	gpLocalSecurity = LocalSecurity();
	HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");

	// Хэндл консольного окна
	ghConWnd = GetConEmuHWND(2);
	gbVisibleOnStartup = IsWindowVisible(ghConWnd);
	// здесь действительно может быть NULL при запуска как detached comspec
	//_ASSERTE(ghConWnd!=NULL);
	//if (!ghConWnd)
	//{
	//	dwErr = GetLastError();
	//	_printf("ghConWnd==NULL, ErrCode=0x%08X\n", dwErr);
	//	iRc = CERR_GETCONSOLEWINDOW; goto wrap;
	//}
	// PID
	gnSelfPID = GetCurrentProcessId();
	gdwMainThreadId = GetCurrentThreadId();


	DWORD nCurrentPIDCount = 0, nCurrentPIDs[64] = {};
	if (hKernel)
	{
		pfnGetConsoleKeyboardLayoutName = (FGetConsoleKeyboardLayoutName)GetProcAddress(hKernel, "GetConsoleKeyboardLayoutNameW");
		pfnGetConsoleProcessList = (FGetConsoleProcessList)GetProcAddress(hKernel, "GetConsoleProcessList");
	}


	#ifdef _DEBUG
	if (ghConWnd)
	{
		// Это событие дергается в отладочной (мной поправленной) версии фара
		wchar_t szEvtName[64]; _wsprintf(szEvtName, SKIPLEN(countof(szEvtName)) L"FARconEXEC:%08X", (DWORD)ghConWnd);
		ghFarInExecuteEvent = CreateEvent(0, TRUE, FALSE, szEvtName);
	}
	#endif


	#if defined(SHOW_STARTED_MSGBOX) || defined(SHOW_COMSPEC_STARTED_MSGBOX)
	if (!IsDebuggerPresent())
	{
		wchar_t szTitle[100]; _wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuC Loaded (PID=%i)", gnSelfPID);
		const wchar_t* pszCmdLine = GetCommandLineW();
		MessageBox(NULL,pszCmdLine,szTitle,0);
	}
	#endif


	#ifdef SHOW_STARTED_ASSERT
	if (!IsDebuggerPresent())
	{
		_ASSERT(FALSE);
	}
	#endif


	wchar_t szSelfExe[MAX_PATH*2] = {}, szSelfDir[MAX_PATH+1] = {};
	DWORD nSelfLen = GetModuleFileNameW(NULL, szSelfExe, countof(szSelfExe));
	if (!nSelfLen || (nSelfLen >= countof(szSelfExe)))
	{
		_ASSERTE(FALSE && "GetModuleFileNameW(NULL) failed");
		szSelfExe[0] = 0;
	}
	else
	{
		lstrcpyn(szSelfDir, szSelfExe, countof(szSelfDir));
		wchar_t* pszSlash = wcsrchr(szSelfDir, L'\\');
		if (pszSlash)
			*pszSlash = 0;
		else
			szSelfDir[0] = 0;
	}


	PRINT_COMSPEC(L"ConEmuC started: %s\n", GetCommandLineW());
	nExitPlaceStep = 50;
	xf_check();

	#ifdef _DEBUG
	{
		wchar_t szCpInfo[128];
		DWORD nCP = GetConsoleOutputCP();
		_wsprintf(szCpInfo, SKIPLEN(countof(szCpInfo)) L"Current Output CP = %u\n", nCP);
		DEBUGSTRCP(szCpInfo);
	}
	#endif

	// Перенес сверху, т.к. "дебаггер" активируется в ParseCommandLine
	// Событие используется для всех режимов
	ghExitQueryEvent = CreateEvent(NULL, TRUE/*используется в нескольких нитях, manual*/, FALSE, NULL);

	LPCWSTR pszFullCmdLine = GetCommandLineW();
	wchar_t szDebugCmdLine[MAX_PATH];
	lstrcpyn(szDebugCmdLine, pszFullCmdLine ? pszFullCmdLine : L"", countof(szDebugCmdLine));

	if (anWorkMode)
	{
		// Alternative mode
		_ASSERTE(anWorkMode==1); // может еще и 2 появится - для StandAloneGui
		_ASSERTE(gnRunMode == RM_UNDEFINED);
		gnRunMode = RM_ALTSERVER;
		gbAttachMode = am_Simple;
		gnConfirmExitParm = 2;
		gbAlwaysConfirmExit = FALSE; gbAutoDisableConfirmExit = FALSE;
		gbNoCreateProcess = TRUE;
		gbAlienMode = TRUE;
		gpSrv->dwRootProcess = GetCurrentProcessId();
		gpSrv->hRootProcess = GetCurrentProcess();
		//gpSrv->dwGuiPID = ...;
		gpszRunCmd = (wchar_t*)calloc(1,2);
		CreateColorerHeader();
	}
	else if ((iRc = ParseCommandLine(pszFullCmdLine/*, &gpszRunCmd, &gbRunInBackgroundTab*/)) != 0)
	{
		goto wrap;
	}

	if ((gnRunMode == RM_UNDEFINED) && gpSrv->DbgInfo.bDebugProcess)
	{
		DWORD nDebugThread = WaitForSingleObject(gpSrv->DbgInfo.hDebugThread, INFINITE);
		_ASSERTE(nDebugThread == WAIT_OBJECT_0); UNREFERENCED_PARAMETER(nDebugThread);

		goto wrap;
	}

#ifdef _DEBUG
	if (gnRunMode == RM_SERVER)
	{
		// отладка для Wine
#ifdef USE_PIPE_DEBUG_BOXES
		gbPipeDebugBoxes = true;
#endif
		if (gbIsWine)
		{
			wchar_t szMsg[128];
			msprintf(szMsg, countof(szMsg), L"ConEmuC Started, Wine detected\r\nConHWND=x%08X(%u), PID=%u\r\nCmdLine: ",
				(DWORD)ghConWnd, (DWORD)ghConWnd, gnSelfPID);
			_wprintf(szMsg);
			_wprintf(GetCommandLineW());
			_wprintf(L"\r\n");
		}
	}
#endif

	_ASSERTE(!gpSrv->hRootProcessGui || (((DWORD)gpSrv->hRootProcessGui)!=0xCCCCCCCC && IsWindow(gpSrv->hRootProcessGui)));

	//#ifdef _DEBUG
	//CreateLogSizeFile();
	//#endif
	nExitPlaceStep = 100;
	xf_check();


	#ifdef SHOW_SERVER_STARTED_MSGBOX
	if ((gnRunMode == RM_SERVER || gnRunMode == RM_ALTSERVER) && !IsDebuggerPresent())
	{
		wchar_t szTitle[100]; _wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuC [Server] started (PID=%i)", gnSelfPID);
		const wchar_t* pszCmdLine = GetCommandLineW();
		MessageBox(NULL,pszCmdLine,szTitle,0);
	}
	#endif


	/* ***************************** */
	/* *** "Общая" инициализация *** */
	/* ***************************** */
	nExitPlaceStep = 150;

	if (!ghExitQueryEvent)
	{
		dwErr = GetLastError();
		_printf("CreateEvent() failed, ErrCode=0x%08X\n", dwErr);
		iRc = CERR_EXITEVENT; goto wrap;
	}
 
	ResetEvent(ghExitQueryEvent);
	ghQuitEvent = CreateEvent(NULL, TRUE/*используется в нескольких нитях, manual*/, FALSE, NULL);

	if (!ghQuitEvent)
	{
		dwErr = GetLastError();
		_printf("CreateEvent() failed, ErrCode=0x%08X\n", dwErr);
		iRc = CERR_EXITEVENT; goto wrap;
	}

	ResetEvent(ghQuitEvent);
	xf_check();

	// Дескрипторы
	//ghConIn  = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
	//            0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	//if ((HANDLE)ghConIn == INVALID_HANDLE_VALUE) {
	//    dwErr = GetLastError();
	//    _printf("CreateFile(CONIN$) failed, ErrCode=0x%08X\n", dwErr);
	//    iRc = CERR_CONINFAILED; goto wrap;
	//}
	// Дескрипторы
	//ghConOut = CreateFile(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
	//            0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (gnRunMode == RM_SERVER || gnRunMode == RM_ALTSERVER)
	{
		if ((HANDLE)ghConOut == INVALID_HANDLE_VALUE)
		{
			dwErr = GetLastError();
			_printf("CreateFile(CONOUT$) failed, ErrCode=0x%08X\n", dwErr);
			iRc = CERR_CONOUTFAILED; goto wrap;
		}

		if (pfnGetConsoleProcessList)
		{
			SetLastError(0);
			nCurrentPIDCount = pfnGetConsoleProcessList(nCurrentPIDs, countof(nCurrentPIDs));
			// Wine bug
			if (!nCurrentPIDCount)
			{
				DWORD nErr = GetLastError();
				_ASSERTE(nCurrentPIDCount || gbIsWine);
				wchar_t szDbgMsg[512], szFile[MAX_PATH] = {};
				GetModuleFileName(NULL, szFile, countof(szFile));
				msprintf(szDbgMsg, countof(szDbgMsg), L"%s: PID=%u: GetConsoleProcessList failed, code=%u\r\n", PointToName(szFile), gnSelfPID, nErr);
				_wprintf(szDbgMsg);
				pfnGetConsoleProcessList = NULL;
			}
		}
	}

	nExitPlaceStep = 200;
	//2009-05-30 попробуем без этого ?
	//SetHandleInformation(GetStdHandle(STD_INPUT_HANDLE), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
	//SetHandleInformation(GetStdHandle(STD_OUTPUT_HANDLE), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
	//SetHandleInformation(GetStdHandle(STD_ERROR_HANDLE), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
	//mode = 0;
	/*lb = GetConsoleMode(ghConIn, &mode);
	if (!(mode & ENABLE_MOUSE_INPUT)) {
		mode |= ENABLE_MOUSE_INPUT;
		lb = SetConsoleMode(ghConIn, mode);
	}*/

	//110131 - ставится в ConEmuC.exe, а это - dll-ка
	//// Обязательно, иначе по CtrlC мы свалимся
	//SetConsoleCtrlHandler((PHANDLER_ROUTINE)HandlerRoutine, true);
	//SetConsoleMode(ghConIn, 0);

	/* ******************************** */
	/* *** "Режимная" инициализация *** */
	/* ******************************** */
	if (gnRunMode == RM_SERVER || gnRunMode == RM_ALTSERVER)
	{
		_ASSERTE(anWorkMode == (gnRunMode == RM_ALTSERVER));
		if ((iRc = ServerInit(anWorkMode)) != 0)
		{
			nExitPlaceStep = 250;
			goto wrap;
		}
	}
	else
	{
		xf_check();

		if ((iRc = ComspecInit()) != 0)
		{
			nExitPlaceStep = 300;
			goto wrap;
		}
	}

	/* ********************************* */
	/* *** Запуск дочернего процесса *** */
	/* ********************************* */
#ifdef SHOW_STARTED_PRINT
	sprintf(szDbgString, "ConEmuC: PID=%u", GetCurrentProcessId());
	MessageBoxA(0, "Press Ok to continue", szDbgString, MB_SYSTEMMODAL);
#endif

	// CREATE_NEW_PROCESS_GROUP - низя, перестает работать Ctrl-C
	// Перед CreateProcess нужно ставить 0, иначе из-за антивирусов может наступить
	// timeout ожидания окончания процесса еще ДО выхода из CreateProcess
	if (!gbAttachMode)
		gpSrv->nProcessStartTick = 0;

	if (gbNoCreateProcess)
	{
		lbRc = TRUE; // Процесс уже запущен, просто цепляемся к ConEmu (GUI)
		pi.hProcess = gpSrv->hRootProcess;
		pi.dwProcessId = gpSrv->dwRootProcess;
	}
	else
	{
		_ASSERTE(gnRunMode != RM_ALTSERVER);
		nExitPlaceStep = 350;

		// Process environment variables
		wchar_t* pszExpandedCmd = ParseConEmuSubst(gpszRunCmd, true/*UpdateTitle*/);
		if (pszExpandedCmd)
		{
			free(gpszRunCmd);
			gpszRunCmd = pszExpandedCmd;
		}

		#ifdef _DEBUG
		if (ghFarInExecuteEvent && wcsstr(gpszRunCmd,L"far.exe"))
			ResetEvent(ghFarInExecuteEvent);
		#endif

		LPCWSTR pszCurDir = NULL;
		WARNING("The process handle must have the PROCESS_VM_OPERATION access right!");

		if (gbUseDosBox)
		{
			DosBoxHelp();
		}
		else if (gnRunMode == RM_SERVER && !gbRunViaCmdExe)
		{
			// Проверить, может пытаются запустить GUI приложение как вкладку в ConEmu?
			if (((si.dwFlags & STARTF_USESHOWWINDOW) && (si.wShowWindow == SW_HIDE))
				|| !(si.dwFlags & STARTF_USESHOWWINDOW) || (si.wShowWindow != SW_SHOWNORMAL))
			{
				//_ASSERTEX(si.wShowWindow != SW_HIDE); -- да, окно сервера (консоль) спрятана

				// Имеет смысл, только если окно хотят изначально спрятать
				const wchar_t *psz = gpszRunCmd, *pszStart;
				wchar_t szExe[MAX_PATH+1];
				if (NextArg(&psz, szExe, &pszStart) == 0)
				{
					MWow64Disable wow;
					if (!gbSkipWowChange) wow.Disable();
					
					DWORD RunImageSubsystem = 0, RunImageBits = 0, RunFileAttrs = 0;
					bool bSubSystem = GetImageSubsystem(szExe, RunImageSubsystem, RunImageBits, RunFileAttrs);

					if (!bSubSystem)
					{
						// szExe may be simple "notepad", we must seek for executable...
						wchar_t szFound[MAX_PATH+1];
						LPWSTR pszFile = NULL;
						// We are interesting only on ".exe" files,
						// supposing that other executable extensions can't be GUI applications
						DWORD n = SearchPath(NULL, szExe, L".exe", countof(szFound), szFound, &pszFile);
						if (n && (n < countof(szFound)))
							bSubSystem = GetImageSubsystem(szFound, RunImageSubsystem, RunImageBits, RunFileAttrs);
					}

					if (bSubSystem)
					{
						if (RunImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI)
						{
							si.dwFlags |= STARTF_USESHOWWINDOW;
							si.wShowWindow = SW_SHOWNORMAL;
						}
					}
				}
			}
		}

		// ConEmuC должен быть максимально прозрачен для конечного процесса
		WARNING("При компиляции gcc все равно прозрачно не получается");
		BOOL lbInheritHandle = (gnRunMode!=RM_SERVER);
		// Если не делать вставку ConEmuC.exe в промежуток между g++.exe и (as.exe или cc1plus.exe)
		// то все хорошо, если вставлять - то лезет куча ошибок вида
		// c:/gcc/mingw/bin/../libexec/gcc/mingw32/4.3.2/cc1plus.exe:1: error: stray '\220' in program
		// c:/gcc/mingw/bin/../libexec/gcc/mingw32/4.3.2/cc1plus.exe:1:4: warning: null character(s) ignored
		// c:/gcc/mingw/bin/../libexec/gcc/mingw32/4.3.2/cc1plus.exe:1: error: stray '\3' in program
		// c:/gcc/mingw/bin/../libexec/gcc/mingw32/4.3.2/cc1plus.exe:1:6: warning: null character(s) ignored
		// Возможно, проблема в наследовании pipe-ов, проверить бы... или в другом SecurityDescriptor.


		//MWow64Disable wow;
		////#ifndef _DEBUG
		//if (!gbSkipWowChange) wow.Disable();
		////#endif

		#ifdef _DEBUG
		LPCWSTR pszRunCmpApp = NULL;
		wchar_t szExeName[MAX_PATH+1];
		{
			LPCWSTR pszStart = gpszRunCmd;
			if (NextArg(&pszStart, szExeName) == 0)
			{
				if (FileExists(szExeName))
				{
					pszRunCmpApp = szExeName;
					pszRunCmpApp = NULL;
				}
			}
		}
		#endif

		LPSECURITY_ATTRIBUTES lpSec = LocalSecurity();
		//#ifdef _DEBUG
		//		lpSec = NULL;
		//#endif
		// Не будем разрешать наследование, если нужно - сделаем DuplicateHandle
		lbRc = createProcess(!gbSkipWowChange, NULL, gpszRunCmd, lpSec,lpSec, lbInheritHandle,
		                      NORMAL_PRIORITY_CLASS/*|CREATE_NEW_PROCESS_GROUP*/
		                      |CREATE_SUSPENDED/*((gnRunMode == RM_SERVER) ? CREATE_SUSPENDED : 0)*/,
		                      NULL, pszCurDir, &si, &pi);
		dwErr = GetLastError();

		if (!lbRc && (gnRunMode == RM_SERVER) && dwErr == ERROR_FILE_NOT_FOUND)
		{
			// Фикс для перемещения ConEmu.exe в подпапку фара. т.е. far.exe находится на одну папку выше
			if (szSelfExe[0] != 0)
			{
				wchar_t szSelf[MAX_PATH*2];
				wcscpy_c(szSelf, szSelfExe);

				wchar_t* pszSlash = wcsrchr(szSelf, L'\\');

				if (pszSlash)
				{
					*pszSlash = 0; // получили папку с exe-шником
					pszSlash = wcsrchr(szSelf, L'\\');

					if (pszSlash)
					{
						*pszSlash = 0; // получили родительскую папку
						pszCurDir = szSelf;
						SetCurrentDirectory(pszCurDir);
						// Пробуем еще раз, в родительской директории
						// Не будем разрешать наследование, если нужно - сделаем DuplicateHandle
						lbRc = createProcess(!gbSkipWowChange, NULL, gpszRunCmd, NULL,NULL, FALSE/*TRUE*/,
						                      NORMAL_PRIORITY_CLASS/*|CREATE_NEW_PROCESS_GROUP*/
						                      |CREATE_SUSPENDED/*((gnRunMode == RM_SERVER) ? CREATE_SUSPENDED : 0)*/,
						                      NULL, pszCurDir, &si, &pi);
						dwErr = GetLastError();
					}
				}
			}
		}

		//wow.Restore();

		if (lbRc) // && (gnRunMode == RM_SERVER))
		{
			nExitPlaceStep = 400;

			#ifdef SHOW_INJECT_MSGBOX
			wchar_t szDbgMsg[128], szTitle[128];
			swprintf_c(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuC, PID=%u", GetCurrentProcessId());
			szDbgMsg[0] = 0;
			#endif

			if (gbDontInjectConEmuHk)
			{
				#ifdef SHOW_INJECT_MSGBOX
				swprintf_c(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"ConEmuC, PID=%u\nConEmuHk injects skipped, PID=%u", GetCurrentProcessId(), pi.dwProcessId);
				#endif
			}
			else
			{
				TODO("Не только в сервере, но и в ComSpec, чтобы дочерние КОНСОЛЬНЫЕ процессы могли пользоваться редиректами");
				//""F:\VCProject\FarPlugin\ConEmu\Bugs\DOS\TURBO.EXE ""
				TODO("При выполнении DOS приложений - VirtualAllocEx(hProcess, обламывается!");
				TODO("В принципе - завелось, но в сочетании с Anamorphosis получается странное зацикливание far->conemu->anamorph->conemu");

				#ifdef SHOW_INJECT_MSGBOX
				swprintf_c(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"ConEmuC, PID=%u\nInjecting hooks into PID=%u", GetCurrentProcessId(), pi.dwProcessId);
				MessageBoxW(NULL, szDbgMsg, szTitle, MB_SYSTEMMODAL);
				#endif

				//BOOL gbLogProcess = FALSE;
				//TODO("Получить из мэппинга glt_Process");
				//#ifdef _DEBUG
				//gbLogProcess = TRUE;
				//#endif
				int iHookRc = -1;
				if (((gnImageSubsystem == IMAGE_SUBSYSTEM_DOS_EXECUTABLE) || (gnImageBits == 16))
					&& !gbUseDosBox)
				{
					// Если запускается ntvdm.exe - все-равно хук поставить не даст
					iHookRc = 0;
				}
				else
				{
					// Чтобы модуль хуков в корневом процессе знал, что оно корневое
					_ASSERTE(ghRootProcessFlag==NULL);
					wchar_t szEvtName[64];
					msprintf(szEvtName, countof(szEvtName), CECONEMUROOTPROCESS, pi.dwProcessId);
					ghRootProcessFlag = CreateEvent(LocalSecurity(), TRUE, TRUE, szEvtName);
					if (ghRootProcessFlag)
					{
						SetEvent(ghRootProcessFlag);
					}
					else
					{
						_ASSERTE(ghRootProcessFlag!=NULL);
					}

					// Теперь ставим хуки
					iHookRc = InjectHooks(pi, FALSE, gbLogProcess);
				}

				if (iHookRc != 0)
				{
					DWORD nErrCode = GetLastError();
					//_ASSERTE(iHookRc == 0); -- ассерт не нужен, есть MsgBox
					wchar_t szDbgMsg[255], szTitle[128];
					_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuC, PID=%u", GetCurrentProcessId());
					_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"ConEmuC.M, PID=%u\nInjecting hooks into PID=%u\nFAILED, code=%i:0x%08X", GetCurrentProcessId(), pi.dwProcessId, iHookRc, nErrCode);
					MessageBoxW(NULL, szDbgMsg, szTitle, MB_SYSTEMMODAL);
				}
				
				if (gbUseDosBox)
				{
					// Если запустился - то сразу добавим в список процессов (хотя он и не консольный)
					ghDosBoxProcess = pi.hProcess; gnDosBoxPID = pi.dwProcessId;
					//ProcessAdd(pi.dwProcessId);
				}
			}

			#ifdef SHOW_INJECT_MSGBOX
			wcscat_c(szDbgMsg, L"\nPress OK to resume started process");
			MessageBoxW(NULL, szDbgMsg, szTitle, MB_SYSTEMMODAL);
			#endif

			// Отпустить процесс (это корневой процесс консоли, например far.exe)
			ResumeThread(pi.hThread);
		}

		if (!lbRc && dwErr == 0x000002E4)
		{
			nExitPlaceStep = 450;
			// Допустимо только в режиме comspec - тогда запустится новая консоль
			_ASSERTE(gnRunMode != RM_SERVER);
			PRINT_COMSPEC(L"Vista+: The requested operation requires elevation (ErrCode=0x%08X).\n", dwErr);
			// Vista: The requested operation requires elevation.
			LPCWSTR pszCmd = gpszRunCmd;
			wchar_t szVerb[10], szExec[MAX_PATH+1];

			if (NextArg(&pszCmd, szExec) == 0)
			{
				SHELLEXECUTEINFO sei = {sizeof(SHELLEXECUTEINFO)};
				sei.hwnd = ghConEmuWnd;
				sei.fMask = SEE_MASK_NO_CONSOLE; //SEE_MASK_NOCLOSEPROCESS; -- смысла ждать завершения нет - процесс запускается в новой консоли
				wcscpy_c(szVerb, L"open"); sei.lpVerb = szVerb;
				sei.lpFile = szExec;
				sei.lpParameters = pszCmd;
				sei.lpDirectory = pszCurDir;
				sei.nShow = SW_SHOWNORMAL;
				MWow64Disable wow; wow.Disable();
				lbRc = ShellExecuteEx(&sei);
				dwErr = GetLastError();
				#ifdef _DEBUG
				OnProcessCreatedDbg(lbRc, dwErr, NULL, &sei);
				#endif
				wow.Restore();

				if (lbRc)
				{
					// OK
					pi.hProcess = NULL; pi.dwProcessId = 0;
					pi.hThread = NULL; pi.dwThreadId = 0;
					// т.к. запустилась новая консоль - подтверждение на закрытие этой точно не нужно
					DisableAutoConfirmExit();
					iRc = 0; goto wrap;
				}
			}
		}
	}

	if (!lbRc)
	{
		nExitPlaceStep = 900;

		PrintExecuteError(gpszRunCmd, dwErr);

		iRc = CERR_CREATEPROCESS; goto wrap;
	}

	if (gbAttachMode)
	{
		// мы цепляемся к уже существующему процессу:
		// аттач из фар плагина или запуск dos-команды в новой консоли через -new_console
		// в последнем случае отключение подтверждения закрытия однозначно некорректно
		// -- DisableAutoConfirmExit(); - низя
	}
	else
	{
		if (!gpSrv->nProcessStartTick) // Уже мог быть проинициализирован из cmd_CmdStartStop
			gpSrv->nProcessStartTick = GetTickCount();
	}

	if (pi.dwProcessId)
		AllowSetForegroundWindow(pi.dwProcessId);

#ifdef _DEBUG
	xf_validate(NULL);
#endif

	/* ************************ */
	/* *** Ожидание запуска *** */
	/* ************************ */

	// Чтобы не блокировать папку запуска
	if (szSelfDir[0])
		SetCurrentDirectory(szSelfDir);

	if (gnRunMode == RM_SERVER)
	{
		//DWORD dwWaitGui = -1;

		nExitPlaceStep = 500;
		gpSrv->hRootProcess  = pi.hProcess; pi.hProcess = NULL; // Required for Win2k
		gpSrv->hRootThread   = pi.hThread;  pi.hThread  = NULL;
		gpSrv->dwRootProcess = pi.dwProcessId;
		gpSrv->dwRootThread  = pi.dwThreadId;
		gpSrv->dwRootStartTime = GetTickCount();
		// Скорее всего процесс в консольном списке уже будет
		CheckProcessCount(TRUE);

		#ifdef _DEBUG
		if (gpSrv->nProcessCount && !gpSrv->DbgInfo.bDebuggerActive)
		{
			_ASSERTE(gpSrv->pnProcesses[gpSrv->nProcessCount-1]!=0);
		}
		#endif

		//if (pi.hProcess) SafeCloseHandle(pi.hProcess);
		//if (pi.hThread) SafeCloseHandle(pi.hThread);

		if (gpSrv->hConEmuGuiAttached)
		{
			DEBUGTEST(DWORD t1 = timeGetTime());
			
			dwWaitGui = WaitForSingleObject(gpSrv->hConEmuGuiAttached, 1000);

			#ifdef _DEBUG
			DWORD t2 = timeGetTime(), tDur = t2-t1;
			if (tDur > GUIATTACHEVENT_TIMEOUT)
			{
				_ASSERTE(tDur <= GUIATTACHEVENT_TIMEOUT);
			}
			#endif

			if (dwWaitGui == WAIT_OBJECT_0)
			{
				// GUI пайп готов
				_wsprintf(gpSrv->szGuiPipeName, SKIPLEN(countof(gpSrv->szGuiPipeName)) CEGUIPIPENAME, L".", (DWORD)ghConWnd); // был gnSelfPID //-V205
			}
		}

		// Ждем, пока в консоли не останется процессов (кроме нашего)
		TODO("Проверить, может ли так получиться, что CreateProcess прошел, а к консоли он не прицепился? Может, если процесс GUI");
		// "Подцепление" процесса к консоли наверное может задержать антивирус
		nWait = nWaitExitEvent = WaitForSingleObject(ghExitQueryEvent, CHECK_ANTIVIRUS_TIMEOUT);

		if (nWait != WAIT_OBJECT_0)  // Если таймаут
		{
			iRc = gpSrv->nProcessCount 
				+ (((gpSrv->nProcessCount==1) && gbUseDosBox && (WaitForSingleObject(ghDosBoxProcess,0)==WAIT_TIMEOUT)) ? 1 : 0);

			// И процессов в консоли все еще нет
			if (iRc == 1 && !gpSrv->DbgInfo.bDebuggerActive)
			{
				if (!gbInShutdown)
				{
					gbTerminateOnCtrlBreak = TRUE;
					gbCtrlBreakStopWaitingShown = TRUE;
					//_printf("Process was not attached to console. Is it GUI?\nCommand to be executed:\n");
					//_wprintf(gpszRunCmd);
					PrintExecuteError(gpszRunCmd, 0, L"Process was not attached to console. Is it GUI?\n");
					_printf("\n\nPress Ctrl+Break to stop waiting\n");

					while (!gbInShutdown && (nWait != WAIT_OBJECT_0))
					{
						nWait = nWaitExitEvent = WaitForSingleObject(ghExitQueryEvent, 250);

						if (nWaitExitEvent == WAIT_OBJECT_0)
						{
							dwWaitRoot = WaitForSingleObject(gpSrv->hRootProcess, 0);
							if (dwWaitRoot == WAIT_OBJECT_0)
							{
								gbTerminateOnCtrlBreak = FALSE;
								gbCtrlBreakStopWaitingShown = FALSE; // сбросим, чтобы ассерты не лезли
							}
							else
							{
								// Root process must be terminated at this point (can't start/initialize)
								_ASSERTE(dwWaitRoot == WAIT_OBJECT_0);
							}
						}

						if ((nWait != WAIT_OBJECT_0) && (gpSrv->nProcessCount > 1))
						{
							gbTerminateOnCtrlBreak = FALSE;
							gbCtrlBreakStopWaitingShown = FALSE; // сбросим, чтобы ассерты не лезли
							goto wait; // OK, переходим в основной цикл ожидания завершения
						}
					}
				}

				// Что-то при загрузке компа иногда все-таки не дожидается, когда процесс в консоли появится
				_ASSERTE(FALSE && gbTerminateOnCtrlBreak && gbInShutdown);
				iRc = CERR_PROCESSTIMEOUT; goto wrap;
			}
		}
	}
	else if (gnRunMode == RM_COMSPEC)
	{
		// В режиме ComSpec нас интересует завершение ТОЛЬКО дочернего процесса
		_ASSERTE(pi.dwProcessId!=0);
		gpSrv->dwRootProcess = pi.dwProcessId;
		gpSrv->dwRootThread = pi.dwThreadId;
	}

	/* *************************** */
	/* *** Ожидание завершения *** */
	/* *************************** */
wait:
#ifdef _DEBUG
	xf_validate(NULL);
#endif

#ifdef _DEBUG
	if (gnRunMode == RM_SERVER)
	{
		gbPipeDebugBoxes = false;
	}
#endif

	
	// Чтобы не блокировать папку запуска (на всякий случай, если на метку goto был)
	if (szSelfDir[0])
		SetCurrentDirectory(szSelfDir);


	if (gnRunMode == RM_ALTSERVER)
	{
		// Поскольку мы крутимся в этом же процессе - то завершения ждать глупо. Здесь другое поведение
		iRc = 0;
		goto AltServerDone;
	} // (gnRunMode == RM_ALTSERVER)
	else if (gnRunMode == RM_SERVER)
	{
		nExitPlaceStep = 550;
		// По крайней мере один процесс в консоли запустился. Ждем пока в консоли не останется никого кроме нас
		nWait = WAIT_TIMEOUT; nWaitExitEvent = -2;

		if (!gpSrv->DbgInfo.bDebuggerActive)
		{
			#ifdef _DEBUG
			while (nWait == WAIT_TIMEOUT)
			{
				nWait = nWaitExitEvent = WaitForSingleObject(ghExitQueryEvent, 100);
				// Что-то при загрузке компа иногда все-таки не дожидается, когда процесс в консоли появится
				_ASSERTE(!(gbCtrlBreakStopWaitingShown && (nWait != WAIT_TIMEOUT)));
			}
			#else
			nWait = nWaitExitEvent = WaitForSingleObject(ghExitQueryEvent, INFINITE);
			_ASSERTE(!(gbCtrlBreakStopWaitingShown && (nWait != WAIT_TIMEOUT)));
			#endif
			ShutdownSrvStep(L"ghExitQueryEvent was set");
		}
		else
		{
			// Перенес обработку отладочных событий в отдельную нить, чтобы случайно не заблокироваться с главной
			nWaitDebugExit = WaitForSingleObject(gpSrv->DbgInfo.hDebugThread, INFINITE);
			nWait = WAIT_OBJECT_0;
			//while (nWait == WAIT_TIMEOUT)
			//{
			//	ProcessDebugEvent();
			//	nWait = WaitForSingleObject(ghExitQueryEvent, 0);
			//}
			//gbAlwaysConfirmExit = TRUE;
			ShutdownSrvStep(L"DebuggerThread terminated");
		}

		#ifdef _DEBUG
		xf_validate(NULL);
		#endif


		// Получить ExitCode
		GetExitCodeProcess(gpSrv->hRootProcess, &gnExitCode);


		#ifdef _DEBUG
		if (nWait == WAIT_OBJECT_0)
		{
			DEBUGSTRFIN(L"*** FinalizeEvent was set!\n");
		}
		#endif

	} // (gnRunMode == RM_SERVER)
	else
	{
		nExitPlaceStep = 600;
		//HANDLE hEvents[3];
		//hEvents[0] = pi.hProcess;
		//hEvents[1] = ghCtrlCEvent;
		//hEvents[2] = ghCtrlBreakEvent;
		//WaitForSingleObject(pi.hProcess, INFINITE);
		#ifdef _DEBUG
		xf_validate(NULL);
		#endif

		DWORD dwWait;
		dwWait = nWaitComspecExit = WaitForSingleObject(pi.hProcess, INFINITE);

		#ifdef _DEBUG
		xf_validate(NULL);
		#endif

		// Получить ExitCode
		GetExitCodeProcess(pi.hProcess, &gnExitCode);

		#ifdef _DEBUG
		xf_validate(NULL);
		#endif

		// Сразу закрыть хэндлы
		if (pi.hProcess)
			SafeCloseHandle(pi.hProcess);

		if (pi.hThread)
			SafeCloseHandle(pi.hThread);

		#ifdef _DEBUG
		xf_validate(NULL);
		#endif
	} // (gnRunMode == RM_COMSPEC)

	/* ************************* */
	/* *** Завершение работы *** */
	/* ************************* */
	iRc = 0;
wrap:
	ShutdownSrvStep(L"Finalizing.1");

#if defined(SHOW_STARTED_PRINT_LITE)
	if (gnRunMode == RM_SERVER)
	{
		_printf("\n" WIN3264TEST("ConEmuC.exe","ConEmuC64.exe") " finalizing, PID=%u\n", GetCurrentProcessId());
	}
#endif

	#ifdef VALIDATE_AND_DELAY_ON_TERMINATE
	// Проверка кучи
	xf_validate(NULL);
	// Отлов изменения высоты буфера
	if (gnRunMode == RM_SERVER)
		Sleep(1000);
	#endif

	// К сожалению, HandlerRoutine может быть еще не вызван, поэтому
	// в самой процедуре ExitWaitForKey вставлена проверка флага gbInShutdown
	PRINT_COMSPEC(L"Finalizing. gbInShutdown=%i\n", gbInShutdown);
#ifdef SHOW_STARTED_MSGBOX
	MessageBox(GetConEmuHWND(2), L"Finalizing", (gnRunMode == RM_SERVER) ? L"ConEmuC.Server" : L"ConEmuC.ComSpec", 0);
#endif

	#ifdef VALIDATE_AND_DELAY_ON_TERMINATE
	xf_validate(NULL);
	#endif

	if (iRc == CERR_GUIMACRO_SUCCEEDED)
	{
		iRc = 0;
	}

	if (gnRunMode == RM_SERVER && gpSrv->hRootProcess)
		GetExitCodeProcess(gpSrv->hRootProcess, &gnExitCode);
	else if (pi.hProcess)
		GetExitCodeProcess(pi.hProcess, &gnExitCode);
	// Ассерт может быть если был запрос на аттач, который не удался
	_ASSERTE(gnExitCode!=STILL_ACTIVE || (iRc==CERR_ATTACHFAILED) || (iRc==CERR_RUNNEWCONSOLE));

	if (iRc && (gbAttachMode == am_Auto))
	{
		// Issue 1003: Non zero exit codes leads to problems in some applications...
		iRc = 0;
	}

	ShutdownSrvStep(L"Finalizing.2");

	if (!gbInShutdown  // только если юзер не нажал крестик в заголовке окна, или не удался /ATTACH (чтобы в консоль не гадить)
	        && ((iRc!=0 && iRc!=CERR_RUNNEWCONSOLE && iRc!=CERR_EMPTY_COMSPEC_CMDLINE
					&& iRc!=CERR_UNICODE_CHK_FAILED && iRc!=CERR_UNICODE_CHK_OKAY
					&& iRc!=CERR_GUIMACRO_SUCCEEDED && iRc!=CERR_GUIMACRO_FAILED
					&& iRc!=CERR_AUTOATTACH_NOT_ALLOWED && iRc!=CERR_ATTACHFAILED
					&& !(gnRunMode!=RM_SERVER && iRc==CERR_CREATEPROCESS))
				|| gbAlwaysConfirmExit)
	  )
	{
		// Чтобы не блокировать папку запуска (если вдруг ошибка была, и папку не меняли)
		if (szSelfDir[0])
			SetCurrentDirectory(szSelfDir);

		//#ifdef _DEBUG
		//if (!gbInShutdown)
		//	MessageBox(0, L"ExitWaitForKey", L"ConEmuC", MB_SYSTEMMODAL);
		//#endif

		BOOL lbProcessesLeft = FALSE, lbDontShowConsole = FALSE;
		DWORD nProcesses[10] = {};
		DWORD nProcCount = -1;

		if (pfnGetConsoleProcessList)
		{
			// консоль может не успеть среагировать на "закрытие" корневого процесса
			nProcCount = pfnGetConsoleProcessList(nProcesses, 10);

			if (nProcCount > 1)
			{
				DWORD nValid = 0;
				for (DWORD i = 0; i < nProcCount; i++)
				{
					if ((nProcesses[i] != gpSrv->dwRootProcess)
						#ifndef WIN64
						&& (nProcesses[i] != gpSrv->nNtvdmPID)
						#endif
						)
					{
						nValid++;
					}
				}

				lbProcessesLeft = (nValid > 1);
			}
		}

		LPCWSTR pszMsg = NULL;

		if (lbProcessesLeft)
		{
			pszMsg = L"\n\nPress Enter or Esc to exit...";
			lbDontShowConsole = gnRunMode != RM_SERVER;
		}
		else
		{
			if (gbRootWasFoundInCon == 1)
			{
				if (gbRootAliveLess10sec && (gnConfirmExitParm != 1))  // корневой процесс проработал менее CHECK_ROOTOK_TIMEOUT
				{
					static wchar_t szMsg[255];
					if (gnExitCode)
					{
						PrintExecuteError(gpszRunCmd, 0, L"\n");
					}
					_wsprintf(szMsg, SKIPLEN(countof(szMsg))
						L"\n\nConEmuC: Root process was alive less than 10 sec, ExitCode=%u.\nPress Enter or Esc to close console...",
						gnExitCode);
					pszMsg = szMsg;
				}
				else
					pszMsg = L"\n\nPress Enter or Esc to close console...";
			}
		}

		if (!pszMsg)  // Иначе - сообщение по умолчанию
		{
			pszMsg = L"\n\nPress Enter or Esc to close console, or wait...";

			#ifdef _DEBUG
			static wchar_t szDbgMsg[255];
			_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg))
			          L"\n\ngbInShutdown=%i, iRc=%i, gbAlwaysConfirmExit=%i, nExitQueryPlace=%i"
			          L"%s",
			          (int)gbInShutdown, iRc, (int)gbAlwaysConfirmExit, nExitQueryPlace,
			          pszMsg);
			pszMsg = szDbgMsg;
			#endif
		}

		gbInExitWaitForKey = TRUE;
		WORD vkKeys[3]; vkKeys[0] = VK_RETURN; vkKeys[1] = VK_ESCAPE; vkKeys[2] = 0;
		ExitWaitForKey(vkKeys, pszMsg, TRUE, lbDontShowConsole);

		UNREFERENCED_PARAMETER(nProcCount);
		UNREFERENCED_PARAMETER(nProcesses[0]);

		if (iRc == CERR_PROCESSTIMEOUT)
		{
			int nCount = gpSrv->nProcessCount;

			if (nCount > 1 || gpSrv->DbgInfo.bDebuggerActive)
			{
				// Процесс таки запустился!
				goto wait;
			}
		}
	}

	// На всякий случай - выставим событие
	if (ghExitQueryEvent)
	{
		_ASSERTE(gbTerminateOnCtrlBreak==FALSE);
		if (!nExitQueryPlace) nExitQueryPlace = 11+(nExitPlaceStep);

		SetTerminateEvent(ste_ConsoleMain);
	}

	// Завершение RefreshThread, InputThread, ServerThread
	if (ghQuitEvent) SetEvent(ghQuitEvent);

	ShutdownSrvStep(L"Finalizing.3");

#ifdef _DEBUG
	xf_validate(NULL);
#endif

	/* ***************************** */
	/* *** "Режимное" завершение *** */
	/* ***************************** */

	if (gnRunMode == RM_SERVER)
	{
		ServerDone(iRc, true);
		//MessageBox(0,L"Server done...",L"ConEmuC",0);
		SafeCloseHandle(gpSrv->DbgInfo.hDebugReady);
		SafeCloseHandle(gpSrv->DbgInfo.hDebugThread);
	}
	else if (gnRunMode == RM_COMSPEC)
	{
		ComspecDone(iRc);
		//MessageBox(0,L"Comspec done...",L"ConEmuC",0);
	}
	else if (gnRunMode == RM_APPLICATION)
	{
		SendStopped();
	}

	ShutdownSrvStep(L"Finalizing.4");

	/* ************************** */
	/* *** "Общее" завершение *** */
	/* ************************** */

	// Наверное, не нужно возвращать заголовок консоли в режиме сервера
	if (gnRunMode == RM_COMSPEC)
	{
		if (gpszPrevConTitle)
		{
			if (ghConWnd)
				SetTitle(false, gpszPrevConTitle);

			free(gpszPrevConTitle);
		}
	}

	SafeCloseHandle(ghRootProcessFlag);

	LogSize(NULL, "Shutdown");
	//ghConIn.Close();
	ghConOut.Close();
	SafeDelete(gpLogSize);

	//if (wpszLogSizeFile)
	//{
	//	//DeleteFile(wpszLogSizeFile);
	//	free(wpszLogSizeFile); wpszLogSizeFile = NULL;
	//}

#ifdef _DEBUG
	SafeCloseHandle(ghFarInExecuteEvent);
#endif

	if (gpszRunCmd) { delete gpszRunCmd; gpszRunCmd = NULL; }

	CommonShutdown();

	ShutdownSrvStep(L"Finalizing.5");

	// -> DllMain
	//if (ghHeap)
	//{
	//	HeapDestroy(ghHeap);
	//	ghHeap = NULL;
	//}

	// борьба с оптимизатором
	if (szDebugCmdLine[0] != 0)
	{
		int nLen = lstrlen(szDebugCmdLine);
		UNREFERENCED_PARAMETER(nLen);
	}

	// Если режим ComSpec - вернуть код возврата из запущенного процесса
	if (iRc == 0 && gnRunMode == RM_COMSPEC)
		iRc = gnExitCode;

#ifdef SHOW_STARTED_MSGBOX
	MessageBox(GetConEmuHWND(2), L"Exiting", (gnRunMode == RM_SERVER) ? L"ConEmuC.Server" : L"ConEmuC.ComSpec", 0);
#endif
	if (gpSrv)
	{
		gpSrv->FinalizeFields();
		free(gpSrv);
		gpSrv = NULL;
	}
AltServerDone:
	ShutdownSrvStep(L"Finalizing done");
	UNREFERENCED_PARAMETER(gpszCheck4NeedCmd);
	UNREFERENCED_PARAMETER(nWaitDebugExit);
	return iRc;
}

int WINAPI RequestLocalServer(/*[IN/OUT]*/RequestLocalServerParm* Parm)
{
	//_ASSERTE(FALSE && "ConEmuCD. Continue to RequestLocalServer");

	int iRc = 0;
	wchar_t szName[64];

	if (!Parm || (Parm->StructSize != sizeof(*Parm)))
	{
		iRc = CERR_CARGUMENT;
		goto wrap;
	}

	// Если больше ничего кроме регистрации событий нет
	if ((Parm->Flags & (slsf_GetFarCommitEvent|slsf_FarCommitForce|slsf_GetCursorEvent)) == Parm->Flags)
	{
		goto DoEvents;
	}

	Parm->pAnnotation = NULL;
	Parm->Flags &= ~slsf_PrevAltServerPID;

	// Хэндл обновим сразу
	if (Parm->Flags & slsf_SetOutHandle)
	{
		ghConOut.SetBufferPtr(Parm->ppConOutBuffer);
	}
	
	if (gnRunMode != RM_ALTSERVER)
	{
		#ifdef SHOW_ALTERNATIVE_MSGBOX
		if (!IsDebuggerPresent())
		{
			char szMsg[128]; msprintf(szMsg, countof(szMsg), "AltServer: " WIN3264TEST("ConEmuCD.dll","ConEmuCD64.dll") " loaded, PID=%u, TID=%u", GetCurrentProcessId(), GetCurrentThreadId());
			MessageBoxA(NULL, szMsg, "ConEmu AltServer" WIN3264TEST(""," x64"), 0);
		}
		#endif

		_ASSERTE(gpSrv == NULL);
		_ASSERTE(gnRunMode == RM_UNDEFINED);

		HWND hConEmu = GetConEmuHWND(TRUE);
		if (!hConEmu || !IsWindow(hConEmu))
		{
			iRc = CERR_GUI_NOT_FOUND;
			goto wrap;
		}

		// Need to block all requests to output buffer in other threads
		MSectionLockSimple csRead;
		if (gpSrv)
			csRead.Lock(&gpSrv->csReadConsoleInfo, LOCK_READOUTPUT_TIMEOUT);

		// Инициализировать gcrVisibleSize и прочие переменные
		CONSOLE_SCREEN_BUFFER_INFO sbi = {};
		// MyGetConsoleScreenBufferInfo пользовать нельзя - оно gpSrv и gnRunMode хочет
		if (GetConsoleScreenBufferInfo(ghConOut, &sbi))
		{
			gcrVisibleSize.X = sbi.srWindow.Right - sbi.srWindow.Left + 1;
			gcrVisibleSize.Y = sbi.srWindow.Bottom - sbi.srWindow.Top + 1;
			gbParmVisibleSize = FALSE;
			gnBufferHeight = (sbi.dwSize.Y == gcrVisibleSize.Y) ? 0 : sbi.dwSize.Y;
			gnBufferWidth = (sbi.dwSize.X == gcrVisibleSize.X) ? 0 : sbi.dwSize.X;
			gbParmBufSize = (gnBufferHeight != 0);
		}
		_ASSERTE(gcrVisibleSize.X>0 && gcrVisibleSize.X<=400 && gcrVisibleSize.Y>0 && gcrVisibleSize.Y<=300);
		csRead.Unlock();

		iRc = ConsoleMain2(1/*0-Server&ComSpec,1-AltServer,2-Reserved*/);

		if ((iRc == 0) && gpSrv && gpSrv->dwPrevAltServerPID)
		{
			Parm->Flags |= slsf_PrevAltServerPID;
			Parm->nPrevAltServerPID = gpSrv->dwPrevAltServerPID;
		}
	}

	// Если поток RefreshThread был "заморожен" при запуске другого сервера
	if (gpSrv->hFreezeRefreshThread)
	{
		SetEvent(gpSrv->hFreezeRefreshThread);
	}

	TODO("Инициализация TrueColor буфера - Parm->ppAnnotation");

DoEvents:
	if (Parm->Flags & slsf_GetFarCommitEvent)
	{
		if (gpSrv)
		{
			_ASSERTE(gpSrv->hFarCommitEvent != NULL); // Уже должно быть создано!
		}
		else
		{
			;
		}

		_wsprintf(szName, SKIPLEN(countof(szName)) CEFARWRITECMTEVENT, gnSelfPID);
		Parm->hFarCommitEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, szName);
		_ASSERTE(Parm->hFarCommitEvent!=NULL);

		if (Parm->Flags & slsf_FarCommitForce)
		{
			gpSrv->bFarCommitRegistered = TRUE;
		}
	}

	if (Parm->Flags & slsf_GetCursorEvent)
	{
		_ASSERTE(gpSrv->hCursorChangeEvent != NULL); // Уже должно быть создано!

		_wsprintf(szName, SKIPLEN(countof(szName)) CECURSORCHANGEEVENT, gnSelfPID);
		Parm->hCursorChangeEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, szName);
		_ASSERTE(Parm->hCursorChangeEvent!=NULL);

		gpSrv->bCursorChangeRegistered = TRUE;
	}

wrap:
	return iRc;
}

//#if defined(CRTSTARTUP)
//extern "C"{
//  BOOL WINAPI _DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved);
//};
//
//BOOL WINAPI mainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
//{
//  DllMain(hDll, dwReason, lpReserved);
//  return TRUE;
//}
//#endif

void PrintVersion()
{
	char szProgInfo[255];
	_wsprintfA(szProgInfo, SKIPLEN(countof(szProgInfo))
		"ConEmuC build %s %s. " CECOPYRIGHTSTRING_A "\n",
		CONEMUVERS, WIN3264TEST("x86","x64"));
	_printf(szProgInfo);
}

void Help()
{
	PrintVersion();
	_wprintf(pConsoleHelp);
	
	//_printf(
	//	    "This is a console part of ConEmu product.\n"
	//	    "Usage: ConEmuC [switches] [/U | /A] /C <command line, passed to %%COMSPEC%%>\n"
	//	    "   or: ConEmuC [switches] /ROOT <program with arguments, far.exe for example>\n"
	//	    "   or: ConEmuC /ATTACH /NOCMD\n"
	//		"   or: ConEmuC /ATTACH /[FAR]PID=<PID>\n"
	//	    "   or: ConEmuC /GUIMACRO <ConEmu GUI macro command>\n"
	//		"   or: ConEmuC /DEBUGPID=<Debugging PID>\n"
	//#ifdef _DEBUG
	//		"   or: ConEmuC /REGCONFONT=<FontName> -> RegisterConsoleFontHKLM\n"
	//#endif
	//	    "   or: ConEmuC /?\n"
	//	    "Switches:\n"
	//	    "     /[NO]CONFIRM    - [don't] confirm closing console on program termination\n"
	//	    "     /ATTACH         - auto attach to ConEmu GUI\n"
	//	    "     /NOCMD          - attach current (existing) console to GUI\n"
	//		"     /[FAR]PID=<PID> - use <PID> as root process\n"
	//	    "     /B{W|H|Z}       - define window width, height and buffer height\n"
	//#ifdef _DEBUG
	//		"     /BW=<WndX> /BH=<WndY> /BZ=<BufY>\n"
	//#endif
	//	    "     /F{N|W|H}    - define console font name, width, height\n"
	//#ifdef _DEBUG
	//		"     /FN=<ConFontName> /FH=<FontHeight> /FW=<FontWidth>\n"
	//#endif
	//	    "     /LOG[N]      - create (debug) log file, N is number from 0 to 3\n"
	//#ifdef _DEBUG
	//		"     /CINMODE==<hex:gnConsoleModeFlags>\n"
	//		"     /HIDE -> gbForceHideConWnd=TRUE\n"
	//		"     /GID=<ConEmu.exe PID>\n"
	//		"     /SETHOOKS=HP{16},PID{10},HT{16},TID{10},ForceGui\n"
	//		"     /INJECT=PID{10}\n"
	//		"     /DOSBOX -> use DosBox\n"
	//#endif
	//	    "\n"
	//	    "When you run application from ConEmu console, you may use\n"
	//        "  Switch: -new_console[:abch[N]rx[N]y[N]u[:user:pwd]]\n"
	//        "     a - RunAs shell verb (as Admin on Vista+, login/passw in Win2k and WinXP)\n"
	//        "     b - Create background tab\n"
	//        "     c - force enable 'Press Enter or Esc to close console' (default)\n"
	//        "     h<height> - i.e., h0 - turn buffer off, h9999 - switch to 9999 lines\n"
	//        "     l - lock console size, do not sync it to ConEmu window\n"
	//        "     n - disable 'Press Enter or Esc to close console'\n"
	//        "     r - run as restricted user\n"
	//        "     x<width>, y<height> - change size of visible area, use with 'l'\n"
	//        "     u - ConEmu choose user dialog\n"
	//        "     u:<user>:<pwd> - specify user/pwd in args, MUST BE LAST OPTION\n"
	//        "  Warning: Option 'Inject ConEmuHk' must be enabled in ConEmu settings!\n"
	//        "  Example: dir \"-new_console:bh9999c\" c:\\ /s\n");
}

void DosBoxHelp()
{
	_wprintf(pDosBoxHelp);
	//_printf(
	//	"Starting DosBox, You may use following default combinations in DosBox window\n"
	//	"ALT-ENTER     Switch to full screen and back.\n"
	//	"ALT-PAUSE     Pause emulation (hit ALT-PAUSE again to continue).\n"
	//	"CTRL-F1       Start the keymapper.\n"
	//	"CTRL-F4       Change between mounted floppy/CD images. Update directory cache \n"
	//	"              for all drives.\n"
	//	"CTRL-ALT-F5   Start/Stop creating a movie of the screen. (avi video capturing)\n"
	//	"CTRL-F5       Save a screenshot. (PNG format)\n"
	//	"CTRL-F6       Start/Stop recording sound output to a wave file.\n"
	//	"CTRL-ALT-F7   Start/Stop recording of OPL commands. (DRO format)\n"
	//	"CTRL-ALT-F8   Start/Stop the recording of raw MIDI commands.\n"
	//	"CTRL-F7       Decrease frameskip.\n"
	//	"CTRL-F8       Increase frameskip.\n"
	//	"CTRL-F9       Kill DOSBox.\n"
	//	"CTRL-F10      Capture/Release the mouse.\n"
	//	"CTRL-F11      Slow down emulation (Decrease DOSBox Cycles).\n"
	//	"CTRL-F12      Speed up emulation (Increase DOSBox Cycles).\n"
	//	"ALT-F12       Unlock speed (turbo button/fast forward).\n"
	//	"F11, ALT-F11  (machine=cga) change tint in NTSC output modes\n"
	//	"F11           (machine=hercules) cycle through amber, green, white colouring\n"
	//);
}

void PrintExecuteError(LPCWSTR asCmd, DWORD dwErr, LPCWSTR asSpecialInfo/*=NULL*/)
{
	if (asSpecialInfo)
	{
		if (*asSpecialInfo)
			_wprintf(asSpecialInfo);
	}
	else
	{
		wchar_t* lpMsgBuf = NULL;
		DWORD nFmtRc, nFmtErr = 0;

		if (dwErr == 5)
		{
			lpMsgBuf = (wchar_t*)LocalAlloc(LPTR, 128*sizeof(wchar_t));
			_wcscpy_c(lpMsgBuf, 128, L"Access is denied.\nThis may be cause of antiviral or file permissions denial.");
		}
		else
		{
			nFmtRc = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, dwErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&lpMsgBuf, 0, NULL);

			if (!nFmtRc)
				nFmtErr = GetLastError();
		}

		_printf("Can't create process, ErrCode=0x%08X, Description:\n", dwErr);
		_wprintf((lpMsgBuf == NULL) ? L"<Unknown error>" : lpMsgBuf);
		if (lpMsgBuf) LocalFree(lpMsgBuf);
		UNREFERENCED_PARAMETER(nFmtErr);
	}

	size_t nCchMax = MAX_PATH*2+32;
	wchar_t* lpInfo = (wchar_t*)calloc(nCchMax,sizeof(*lpInfo));
	if (lpInfo)
	{
		_wcscpy_c(lpInfo, nCchMax, L"\nCurrent directory:\n");
			_ASSERTE(nCchMax>=(MAX_PATH*2+32));
		GetCurrentDirectory(MAX_PATH*2, lpInfo+lstrlen(lpInfo));
		_wcscat_c(lpInfo, nCchMax, L"\n");
		_wprintf(lpInfo);
		free(lpInfo);
	}

	_printf("\nCommand to be executed:\n");
	_wprintf(asCmd ? asCmd : L"<null>");
	_printf("\n");
}

void CheckUnicodeMode()
{
	if (gnCmdUnicodeMode) return;

	wchar_t szValue[16] = {0};

	if (GetEnvironmentVariable(L"ConEmuOutput", szValue, sizeof(szValue)/sizeof(szValue[0])))
	{
		if (lstrcmpi(szValue, L"UNICODE") == 0)
			gnCmdUnicodeMode = 2;
		else if (lstrcmpi(szValue, L"ANSI") == 0)
			gnCmdUnicodeMode = 1;
	}
}

void RegisterConsoleFontHKLM(LPCWSTR pszFontFace)
{
	if (!pszFontFace || !*pszFontFace)
		return;

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
				RegSetValueExW(hk, szId, 0, REG_SZ, (LPBYTE)pszFontFace, (lstrlen(pszFontFace)+1)*2);
				break;
			}

			if (lstrcmpi(szFont, pszFontFace) == 0)
			{
				break; // он уже добавлен
			}
		}

		RegCloseKey(hk);
	}
}

int CheckAttachProcess()
{
	HWND hConEmu = GetConEmuHWND(TRUE);

	if (hConEmu && IsWindow(hConEmu))
	{
		// Console already attached
		// Ругаться наверное вообще не будем
		gbInShutdown = TRUE;
		return CERR_CARGUMENT;
	}

	BOOL lbArgsFailed = FALSE;
	wchar_t szFailMsg[512]; szFailMsg[0] = 0;

	if (gpSrv->hRootProcessGui)
	{
		if (!IsWindow(gpSrv->hRootProcessGui))
		{
			_wsprintf(szFailMsg, SKIPLEN(countof(szFailMsg)) L"Attach of GUI application was requested,\n"
				L"but required HWND(0x%08X) not found!", (DWORD)gpSrv->hRootProcessGui);
			lbArgsFailed = TRUE;
		}
		else
		{
			DWORD nPid; GetWindowThreadProcessId(gpSrv->hRootProcessGui, &nPid);
			if (!gpSrv->dwRootProcess || (gpSrv->dwRootProcess != nPid))
			{
				_wsprintf(szFailMsg, SKIPLEN(countof(szFailMsg)) L"Attach of GUI application was requested,\n"
					L"but PID(%u) of HWND(0x%08X) does not match Root(%u)!",
					nPid, (DWORD)gpSrv->hRootProcessGui, gpSrv->dwRootProcess);
				lbArgsFailed = TRUE;
			}
		}
	}
	else if (pfnGetConsoleProcessList==NULL)
	{
		wcscpy_c(szFailMsg, L"Attach to GUI was requested, but required WinXP or higher!");
		lbArgsFailed = TRUE;
		//_wprintf (GetCommandLineW());
		//_printf ("\n");
		//_ASSERTE(FALSE);
		//gbInShutdown = TRUE; // чтобы в консоль не гадить
		//return CERR_CARGUMENT;
	}
	else
	{
		DWORD nProcesses[20];
		DWORD nProcCount = pfnGetConsoleProcessList(nProcesses, 20);

		// 2 процесса, потому что это мы сами и минимум еще один процесс в этой консоли,
		// иначе смысла в аттаче нет
		if (nProcCount < 2)
		{
			wcscpy_c(szFailMsg, L"Attach to GUI was requested, but there is no console processes!");
			lbArgsFailed = TRUE;
			//_wprintf (GetCommandLineW());
			//_printf ("\n");
			//_ASSERTE(FALSE);
			//return CERR_CARGUMENT;
		}
		// не помню, зачем такая проверка была введена, но (nProcCount > 2) мешает аттачу.
		// в момент запуска сервера (/ATTACH /PID=n) еще жив родительский (/ATTACH /NOCMD)
		//// Если cmd.exe запущен из cmd.exe (в консоли уже больше двух процессов) - ничего не делать
		else if ((gpSrv->dwRootProcess != 0) || (nProcCount > 2))
		{
			BOOL lbRootExists = (gpSrv->dwRootProcess == 0);
			// И ругаться только под отладчиком
			wchar_t szProc[255] = {0}, szTmp[10]; //StringCchPrintf(szProc, countof(szProc), L"%i, %i, %i", nProcesses[0], nProcesses[1], nProcesses[2]);
			DWORD nFindId = 0;

			for(int n = ((int)nProcCount-1); n >= 0; n--)
			{
				if (szProc[0]) wcscat_c(szProc, L", ");

				_wsprintf(szTmp, SKIPLEN(countof(szTmp)) L"%i", nProcesses[n]);
				wcscat_c(szProc, szTmp);

				if (gpSrv->dwRootProcess)
				{
					if (!lbRootExists && nProcesses[n] == gpSrv->dwRootProcess)
						lbRootExists = TRUE;
				}
				else if ((nFindId == 0) && (nProcesses[n] != gnSelfPID))
				{
					// Будем считать его корневым.
					// Собственно, кого считать корневым не важно, т.к.
					// сервер не закроется до тех пор пока жив хотя бы один процесс
					nFindId = nProcesses[n];
				}
			}

			if ((gpSrv->dwRootProcess == 0) && (nFindId != 0))
			{
				gpSrv->dwRootProcess = nFindId;
				lbRootExists = TRUE;
			}

			if ((gpSrv->dwRootProcess != 0) && !lbRootExists)
			{
				_wsprintf(szFailMsg, SKIPLEN(countof(szFailMsg)) L"Attach to GUI was requested, but\n" L"root process (%u) does not exists", gpSrv->dwRootProcess);
				lbArgsFailed = TRUE;
			}
			else if ((gpSrv->dwRootProcess == 0) && (nProcCount > 2))
			{
				_wsprintf(szFailMsg, SKIPLEN(countof(szFailMsg)) L"Attach to GUI was requested, but\n" L"there is more than 2 console processes: %s\n", szProc);
				lbArgsFailed = TRUE;
			}

			//PRINT_COMSPEC(L"Attach to GUI was requested, but there is more then 2 console processes: %s\n", szProc);
			//_ASSERTE(FALSE);
			//return CERR_CARGUMENT;
		}
	}

	if (lbArgsFailed)
	{

		LPCWSTR pszCmdLine = GetCommandLineW(); if (!pszCmdLine) pszCmdLine = L"";

		int nCmdLen = lstrlen(szFailMsg) + lstrlen(pszCmdLine) + 16;
		wchar_t* pszMsg = (wchar_t*)malloc(nCmdLen*2);
		_wcscpy_c(pszMsg, nCmdLen, szFailMsg);
		_wcscat_c(pszMsg, nCmdLen, L"\n\n");
		_wcscat_c(pszMsg, nCmdLen, pszCmdLine);
		wchar_t szTitle[64]; _wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuC, PID=%u", GetCurrentProcessId());
		MessageBox(NULL, pszMsg, szTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
		free(pszMsg);
		gbInShutdown = TRUE;
		return CERR_CARGUMENT;
	}

	return 0; // OK
}

// Возвращает CERR_UNICODE_CHK_OKAY, если консоль поддерживает отображение
// юникодных символов. Иначе - CERR_UNICODE_CHK_FAILED
int CheckUnicodeFont()
{
	int iRc = CERR_UNICODE_CHK_FAILED;

	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	

	wchar_t szText[80] = UnicodeTestString;
	CHAR_INFO cWrite[80];
	CHAR_INFO cRead[80] = {};
	WORD aWrite[80], aRead[80] = {};
	wchar_t sAttrWrite[80] = {}, sAttrRead[80] = {}, sAttrBlock[80] = {};
	wchar_t szInfo[1024]; DWORD nTmp;
	wchar_t szCheck[80] = L"", szBlock[80] = L"";
	BOOL bInfo = FALSE, bWrite = FALSE, bRead = FALSE, bCheck = FALSE;
	DWORD nLen = lstrlen(szText), nWrite = 0, nRead = 0, nErr = 0;
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	_ASSERTE(nLen<=35); // ниже на 2 буфер множится

	wchar_t szMinor[2] = {MVV_4a[0], 0};
	//msprintf не умеет "%02u"
	_wsprintf(szInfo, SKIPLEN(countof(szInfo))
		L"ConEmu %02u%02u%02u%s %s\r\n"
		L"OS Version: %u.%u.%u (%u:%s)\r\n",
		MVV_1, MVV_2, MVV_3, szMinor, WIN3264TEST(L"x86",L"x64"),
		gOSVer.dwMajorVersion, gOSVer.dwMinorVersion, gOSVer.dwBuildNumber, gOSVer.dwPlatformId, gOSVer.szCSDVersion);
	WriteConsoleW(hOut, szInfo, lstrlen(szInfo), &nTmp, NULL);

	msprintf(szInfo, countof(szInfo), L"SM_IMMENABLED=%u, SM_DBCSENABLED=%u, ACP=%u, OEMCP=%u\r\n",
		GetSystemMetrics(SM_IMMENABLED), GetSystemMetrics(SM_DBCSENABLED), GetACP(), GetOEMCP());
	WriteConsoleW(hOut, szInfo, lstrlen(szInfo), &nTmp, NULL);

	msprintf(szInfo, countof(szInfo), L"ConHWND=0x%08X, Class=\"", (DWORD)ghConWnd);
	GetClassName(ghConWnd, szInfo+lstrlen(szInfo), 255);
	lstrcpyn(szInfo+lstrlen(szInfo), L"\"\r\n", 4);
	WriteConsoleW(hOut, szInfo, lstrlen(szInfo), &nTmp, NULL);

	struct FONT_INFOEX
	{
		ULONG cbSize;
		DWORD nFont;
		COORD dwFontSize;
		UINT  FontFamily;
		UINT  FontWeight;
		WCHAR FaceName[LF_FACESIZE];
	};
	typedef BOOL (WINAPI* GetCurrentConsoleFontEx_t)(HANDLE hConsoleOutput, BOOL bMaximumWindow, FONT_INFOEX* lpConsoleCurrentFontEx);
	HMODULE hKernel = GetModuleHandle(L"kernel32.dll");
	GetCurrentConsoleFontEx_t _GetCurrentConsoleFontEx = (GetCurrentConsoleFontEx_t)(hKernel ? GetProcAddress(hKernel,"GetCurrentConsoleFontEx") : NULL);
	if (!_GetCurrentConsoleFontEx)
	{
		lstrcpyn(szInfo, L"Console font info: Not available\r\n", countof(szInfo));
	}
	else
	{
		FONT_INFOEX info = {sizeof(info)};
		if (!_GetCurrentConsoleFontEx(hOut, FALSE, &info))
		{
			msprintf(szInfo, countof(szInfo), L"Console font info: Failed, code=%u\r\n", GetLastError());
		}
		else
		{
			info.FaceName[LF_FACESIZE-1] = 0;
			msprintf(szInfo, countof(szInfo), L"Console font info: %u, {%ux%u}, %u, %u, \"%s\"\r\n",
				info.nFont, info.dwFontSize.X, info.dwFontSize.Y, info.FontFamily, info.FontWeight,
				info.FaceName);
		}
	}
	WriteConsoleW(hOut, szInfo, lstrlen(szInfo), &nTmp, NULL);

	DWORD nCP = GetConsoleCP();
	DWORD nOutCP = GetConsoleOutputCP();
	CPINFOEX cpinfo = {};

	msprintf(szInfo, countof(szInfo), L"ConsoleCP=%u, ConsoleOutputCP=%u\r\n", nCP, nOutCP);
	WriteConsoleW(hOut, szInfo, lstrlen(szInfo), &nTmp, NULL);

	for (UINT i = 0; i <= 1; i++)
	{
		if (i && (nOutCP == nCP))
			break;

		if (!GetCPInfoEx(i ? nOutCP : nCP, 0, &cpinfo))
		{
			msprintf(szInfo, countof(szInfo), L"GetCPInfoEx(%u) failed, code=%u\r\n", nCP, GetLastError());
		}
		else
		{
			msprintf(szInfo, countof(szInfo),
				L"CP%u: Max=%u "
				L"Def=x%02X,x%02X UDef=x%X\r\n"
				L"  Lead=x%02X,x%02X,x%02X,x%02X,x%02X,x%02X,x%02X,x%02X,x%02X,x%02X,x%02X,x%02X\r\n"
				L"  Name=\"%s\"\r\n",
				cpinfo.CodePage, cpinfo.MaxCharSize,
				cpinfo.DefaultChar[0], cpinfo.DefaultChar[1], cpinfo.UnicodeDefaultChar,
				cpinfo.LeadByte[0], cpinfo.LeadByte[1], cpinfo.LeadByte[2], cpinfo.LeadByte[3], cpinfo.LeadByte[4], cpinfo.LeadByte[5],
				cpinfo.LeadByte[6], cpinfo.LeadByte[7], cpinfo.LeadByte[8], cpinfo.LeadByte[9], cpinfo.LeadByte[10], cpinfo.LeadByte[11],
				cpinfo.CodePageName);
		}
		WriteConsoleW(hOut, szInfo, lstrlen(szInfo), &nTmp, NULL);
    }


    WriteConsoleW(hOut, L"\r\nCheck ", 8, &nTmp, NULL);


	if ((bInfo = GetConsoleScreenBufferInfo(hOut, &csbi)) != FALSE)
	{
		for (DWORD i = 0; i < nLen; i++)
		{
			cWrite[i].Char.UnicodeChar = szText[i];
			aWrite[i] = 10 + (i % 6);
			cWrite[i].Attributes = aWrite[i];
			msprintf(sAttrWrite+i, 2, L"%X", aWrite[i]);
		}

		COORD /*crSize = {(SHORT)nLen,1},*/ cr0 = {0,0};
		//SMALL_RECT rcWrite = {csbi.dwCursorPosition.X, csbi.dwCursorPosition.Y, csbi.dwCursorPosition.X+(SHORT)nLen-1, csbi.dwCursorPosition.Y};
		if ((bWrite = WriteConsoleOutputCharacterW(hOut, szText, nLen, csbi.dwCursorPosition, &nWrite)) != FALSE)
        //if ((bWrite = WriteConsoleOutputW(hOut, cWrite, crSize, cr0, &rcWrite)) != FALSE)
		{
			if (((bRead = ReadConsoleOutputCharacterW(hOut, szCheck, nLen*2, csbi.dwCursorPosition, &nRead)) != FALSE)
				/*&& (nRead == nLen)*/)
			{
				bCheck = (memcmp(szText, szCheck, nLen*sizeof(szText[0])) == 0);
				if (bCheck)
				{
					iRc = CERR_UNICODE_CHK_OKAY;
				}
			}

			if (ReadConsoleOutputAttribute(hOut, aRead, nLen, csbi.dwCursorPosition, &nTmp))
			{
				for (UINT i = 0; i < nTmp; i++)
				{
					msprintf(sAttrRead+i, 2, L"%X", aRead[i] & 0xF);
				}
			}

			COORD crBufSize = {(SHORT)sizeof(cRead),1};
			SMALL_RECT rcRead = {csbi.dwCursorPosition.X, csbi.dwCursorPosition.Y, csbi.dwCursorPosition.X+(SHORT)nLen*2, csbi.dwCursorPosition.Y};
			if (ReadConsoleOutputW(hOut, cRead, crBufSize, cr0, &rcRead))
			{
				for (UINT i = 0; i < nTmp; i++)
				{
					szBlock[i] = cRead[i].Char.UnicodeChar;
					msprintf(sAttrBlock+i, 2, L"%X", cRead[i].Attributes & 0xF);
				}
			}
		}
	}

	//if (!bRead || !bCheck)
	{
		nErr = GetLastError();
		
		msprintf(szInfo, countof(szInfo),
			L"\r\n" // чтобы не затереть сам текст
			L"Text: %s\r\n"
			L"Read: %s\r\n"
			L"Blck: %s\r\n"
			L"Attr: %s\r\n"
			L"Read: %s\r\n"
			L"Blck: %s\r\n"
			L"Info: %u,%u,%u,%u,%u,%u\r\n"
			L"\r\n%s\r\n",
			szText, szCheck, szBlock, sAttrWrite, sAttrRead, sAttrBlock,
			nErr, bInfo, bWrite, nWrite, bRead, nRead,
			bCheck ? L"Unicode check succeeded" :
			bRead ? L"Unicode check FAILED!" : L"Unicode is not supported in this console!"
			);
		//MessageBoxW(NULL, szInfo, szTitle, MB_ICONEXCLAMATION|MB_ICONERROR);
	}
	//else
	//{
	//	lstrcpyn(szInfo, L"\r\nUnicode check succeeded\r\n", countof(szInfo));
	//}

	//WriteConsoleW(hOut, L"\r\n", 2, &nTmp, NULL);
	//WriteConsoleW(hOut, szCheck, nRead, &nTmp, NULL);

	//LPCWSTR pszText = bCheck ? L"\r\nUnicode check succeeded\r\n" : L"\r\nUnicode check FAILED!\r\n";
	WriteConsoleW(hOut, szInfo, lstrlen(szInfo), &nWrite, NULL);

	return iRc;
}

enum ConEmuStateCheck
{
	ec_None = 0,
	ec_IsConEmu,
	ec_IsTerm,
	ec_IsAnsi,
};

bool DoStateCheck(ConEmuStateCheck eStateCheck)
{
	bool bOn = false;

	switch (eStateCheck)
	{
	case ec_IsConEmu:
	case ec_IsAnsi:
		if (ghConWnd)
		{
			CESERVER_CONSOLE_MAPPING_HDR* pInfo = (CESERVER_CONSOLE_MAPPING_HDR*)malloc(sizeof(*pInfo));
			if (pInfo && LoadSrvMapping(ghConWnd, *pInfo))
			{
				_ASSERTE(pInfo->ComSpec.ConEmuExeDir[0] && pInfo->ComSpec.ConEmuBaseDir[0]);

				HWND hWnd = pInfo->hConEmuWndDc;
				if (hWnd && IsWindow(hWnd))
				{
					switch (eStateCheck)
					{
					case ec_IsConEmu:
						bOn = true;
						break;
					case ec_IsAnsi:
						bOn = ((pInfo->Flags & CECF_ProcessAnsi) != 0);
						break;
					default:
						;
					}
				}
			}
			SafeFree(pInfo);
		}
		break;
	case ec_IsTerm:
		bOn = isTerminalMode();
		break;
	default:
		_ASSERTE(FALSE && "Unsupported StateCheck code");
	}

	return bOn;
}

enum ConEmuExecAction
{
	ea_None = 0,
	ea_RegConFont, // RegisterConsoleFontHKLM
	ea_InjectHooks,
	ea_InjectRemote,
	ea_InjectDefTrm,
	ea_GuiMacro,
	ea_CheckUnicodeFont,
	ea_ExportCon,  // export env.vars to processes of active console
	ea_ExportGui,  // ea_ExportCon + ConEmu window
	ea_ExportAll,  // export env.vars to all opened tabs of current ConEmu window
};

int DoInjectHooks(LPWSTR asCmdArg)
{
	gbInShutdown = TRUE; // чтобы не возникло вопросов при выходе
	gnRunMode = RM_SETHOOK64;
	LPWSTR pszNext = asCmdArg;
	LPWSTR pszEnd = NULL;
	BOOL lbForceGui = FALSE;
	PROCESS_INFORMATION pi = {NULL};
	pi.hProcess = (HANDLE)wcstoul(pszNext, &pszEnd, 16);


	#ifdef SHOW_INJECT_MSGBOX
	wchar_t szDbgMsg[512], szTitle[128];
	PROCESSENTRY32 pinf;
	GetProcessInfo(nRemotePID, &pinf);
	swprintf_c(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuCD PID=%u", GetCurrentProcessId());
	swprintf_c(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"InjectsTo PID=%s {%s}\nConEmuCD PID=%u", asCmdArg ? asCmdArg : L"", pinf.szExeFile, GetCurrentProcessId());
	MessageBoxW(NULL, szDbgMsg, szTitle, MB_SYSTEMMODAL);
	#endif


	if (pi.hProcess && pszEnd && *pszEnd)
	{
		pszNext = pszEnd+1;
		pi.dwProcessId = wcstoul(pszNext, &pszEnd, 10);
	}

	if (pi.dwProcessId && pszEnd && *pszEnd)
	{
		pszNext = pszEnd+1;
		pi.hThread = (HANDLE)wcstoul(pszNext, &pszEnd, 16);
	}

	if (pi.hThread && pszEnd && *pszEnd)
	{
		pszNext = pszEnd+1;
		pi.dwThreadId = wcstoul(pszNext, &pszEnd, 10);
	}

	if (pi.dwThreadId && pszEnd && *pszEnd)
	{
		pszNext = pszEnd+1;
		lbForceGui = wcstoul(pszNext, &pszEnd, 10);
	}
	
	if (pi.hProcess && pi.hThread && pi.dwProcessId && pi.dwThreadId)
	{
		//BOOL gbLogProcess = FALSE;
		//TODO("Получить из мэппинга glt_Process");
		//#ifdef _DEBUG
		//gbLogProcess = TRUE;
		//#endif
		int iHookRc = InjectHooks(pi, lbForceGui, gbLogProcess);

		if (iHookRc == 0)
		{
			return CERR_HOOKS_WAS_SET;
		}

		// Ошибку (пока во всяком случае) лучше показать, для отлова возможных проблем
		DWORD nErrCode = GetLastError();
		//_ASSERTE(iHookRc == 0); -- ассерт не нужен, есть MsgBox
		wchar_t szDbgMsg[255], szTitle[128];
		_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuC, PID=%u", GetCurrentProcessId());
		_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"ConEmuC.X, PID=%u\nInjecting hooks into PID=%u\nFAILED, code=%i:0x%08X", GetCurrentProcessId(), pi.dwProcessId, iHookRc, nErrCode);
		MessageBoxW(NULL, szDbgMsg, szTitle, MB_SYSTEMMODAL);
	}
	else
	{
		//_ASSERTE(pi.hProcess && pi.hThread && pi.dwProcessId && pi.dwThreadId);
		wchar_t szDbgMsg[512], szTitle[128];
		_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuC, PID=%u", GetCurrentProcessId());
		_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"ConEmuC.X, PID=%u\nCmdLine parsing FAILED (%u,%u,%u,%u,%u)!\n%s",
			GetCurrentProcessId(), (DWORD)pi.hProcess, (DWORD)pi.hThread, pi.dwProcessId, pi.dwThreadId, lbForceGui, //-V205
			asCmdArg);
		MessageBoxW(NULL, szDbgMsg, szTitle, MB_SYSTEMMODAL);
	}

	return CERR_HOOKS_FAILED;
}

int DoInjectRemote(LPWSTR asCmdArg, bool abDefTermOnly)
{
	gbInShutdown = TRUE; // чтобы не возникло вопросов при выходе
	gnRunMode = RM_SETHOOK64;
	LPWSTR pszNext = asCmdArg;
	LPWSTR pszEnd = NULL;
	DWORD nRemotePID = wcstoul(pszNext, &pszEnd, 10);


	#ifdef SHOW_INJECTREM_MSGBOX
	wchar_t szDbgMsg[512], szTitle[128];
	PROCESSENTRY32 pinf;
	GetProcessInfo(nRemotePID, &pinf);
	swprintf_c(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuCD PID=%u", GetCurrentProcessId());
	swprintf_c(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"Hooking PID=%s {%s}\nConEmuCD PID=%u. Continue with injects?", asCmdArg ? asCmdArg : L"", pinf.szExeFile, GetCurrentProcessId());
	if (MessageBoxW(NULL, szDbgMsg, szTitle, MB_SYSTEMMODAL|MB_OKCANCEL) != IDOK)
	{
		return CERR_HOOKS_FAILED;
	}
	#endif

	
	if (nRemotePID)
	{
		#if defined(SHOW_ATTACH_MSGBOX)
		if (!IsDebuggerPresent())
		{
			wchar_t szTitle[100]; _wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuC (PID=%i) /INJECT", gnSelfPID);
			const wchar_t* pszCmdLine = GetCommandLineW();
			MessageBox(NULL,pszCmdLine,szTitle,MB_SYSTEMMODAL);
		}
		#endif

		// Preparing Events
		wchar_t szName[64]; HANDLE hEvent;
		if (!abDefTermOnly)
		{
			// When running in normal mode (NOT set up as default terminal)
			// we need full initialization procedure, not a light one when hooking explorer.exe
			_wsprintf(szName, SKIPLEN(countof(szName)) CEDEFAULTTERMHOOK, nRemotePID);
			hEvent = OpenEvent(EVENT_MODIFY_STATE|SYNCHRONIZE, FALSE, szName);
			if (hEvent)
			{
				ResetEvent(hEvent);
				CloseHandle(hEvent);
			}
		}
		// Creating as remote thread, need to determine MainThread?
		_wsprintf(szName, SKIPLEN(countof(szName)) CECONEMUROOTTHREAD, nRemotePID);
		hEvent = OpenEvent(EVENT_MODIFY_STATE|SYNCHRONIZE, FALSE, szName);
		if (hEvent)
		{
			ResetEvent(hEvent);
			CloseHandle(hEvent);
		}

		int iHookRc = -1;
		bool bAlreadyInjected = false;
		// Hey, may be ConEmuHk.dll is already loaded?
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, nRemotePID);
		MODULEENTRY32 mi = {sizeof(mi)};
		if (hSnap && Module32First(hSnap, &mi))
		{
			// 130829 - Let load newer(!) ConEmuHk.dll into target process.

			LPCWSTR pszConEmuHk = WIN3264TEST(L"conemuhk.", L"conemuhk64.");
			size_t nDllNameLen = lstrlen(pszConEmuHk);
			// Out preferred module name
			wchar_t szOurName[32] = {}, szVer[2] = {MVV_4a[0],0};
			_wsprintf(szOurName, SKIPLEN(countof(szOurName))
				CEDEFTERMDLLFORMAT /*L"ConEmuHk%s.%02u%02u%02u%s.dll"*/,
				WIN3264TEST(L"",L"64"), MVV_1, MVV_2, MVV_3, szVer);
			CharLowerBuff(szOurName, lstrlen(szOurName));

			// Go to enumeration
			wchar_t szName[64];
			do {
				LPCWSTR pszName = PointToName(mi.szModule);
				// Name of hooked module may be changed (copied to %APPDATA%)
				if (pszName && *pszName)
				{
					lstrcpyn(szName, pszName, countof(szName));
					CharLowerBuff(szName, lstrlen(szName));
					// ConEmuHk*.*.dll?
					if (wmemcmp(szName, pszConEmuHk, nDllNameLen) == 0
						&& wmemcmp(szName+lstrlen(szName)-4, L".dll", 4) == 0)
					{
						// Yes! ConEmuHk.dll already loaded into nRemotePID!
						// But what is the version? Let don't downgrade loaded version!
						if (lstrcmp(szName, szOurName) >= 0)
						{
							// OK, szName is newer or equal to our build
							iHookRc = 0;
							bAlreadyInjected = true;
						}
						// Stop enumeration
						break;
					}
				}
			} while (Module32Next(hSnap, &mi));

			// Check done
		}
		SafeCloseHandle(hSnap);


		// Go to hook
		if (iHookRc == -1)
		{
			iHookRc = InjectRemote(nRemotePID, abDefTermOnly);
		}

		if (iHookRc == 0)
		{
			return bAlreadyInjected ? CERR_HOOKS_WAS_ALREADY_SET : CERR_HOOKS_WAS_SET;
		}

		// Ошибку (пока во всяком случае) лучше показать, для отлова возможных проблем
		DWORD nErrCode = GetLastError();
		//_ASSERTE(iHookRc == 0); -- ассерт не нужен, есть MsgBox
		wchar_t szDbgMsg[255], szTitle[128];
		_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuC, PID=%u", GetCurrentProcessId());
		_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"ConEmuC.X, PID=%u\nInjecting remote into PID=%u\nFAILED, code=%i:0x%08X", GetCurrentProcessId(), nRemotePID, iHookRc, nErrCode);
		MessageBoxW(NULL, szDbgMsg, szTitle, MB_SYSTEMMODAL);
	}
	else
	{
		//_ASSERTE(pi.hProcess && pi.hThread && pi.dwProcessId && pi.dwThreadId);
		wchar_t szDbgMsg[512], szTitle[128];
		_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuC, PID=%u", GetCurrentProcessId());
		_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"ConEmuC.X, PID=%u\nCmdLine parsing FAILED (%u)!\n%s",
			GetCurrentProcessId(), nRemotePID,
			asCmdArg);
		MessageBoxW(NULL, szDbgMsg, szTitle, MB_SYSTEMMODAL);
	}

	return CERR_HOOKS_FAILED;
}

int DoExportEnv(LPCWSTR asCmdArg, ConEmuExecAction eExecAction, bool bSilent = false)
{
	int iRc = CERR_CARGUMENT;
	struct ProcInfo {
		DWORD nPID, nParentPID;
		DWORD_PTR Flags;
	};
	//ProcInfo* pList = NULL;
	MArray<ProcInfo> List;
	LPWSTR pszAllVars = NULL, pszSrc;
	CESERVER_REQ *pIn = NULL;
	const DWORD nSelfPID = GetCurrentProcessId();
	DWORD nTestPID, nParentPID;
	DWORD nSrvPID = 0;
	CESERVER_CONSOLE_MAPPING_HDR test = {};
	BOOL lbMapExist = false;
	HANDLE h;
	size_t cchMaxEnvLen = 0;
	wchar_t* pszBuffer;

	//_ASSERTE(FALSE && "Continue with exporting environment");

	#define ExpFailedPref "ConEmuC: can't export environment"

	if (!ghConWnd)
	{
		_ASSERTE(ghConWnd);
		if (!bSilent)
			_printf(ExpFailedPref ", ghConWnd was not set\n");
		goto wrap;
	}



	// Query current environment
	pszAllVars = GetEnvironmentStringsW();
	if (!pszAllVars || !*pszAllVars)
	{
		if (!bSilent)
			_printf(ExpFailedPref ", GetEnvironmentStringsW failed, code=%u\n", GetLastError());
		goto wrap;
	}

	// Parse variables block, determining MAX length
	pszSrc = pszAllVars;
	while (*pszSrc)
	{
		LPWSTR pszName = pszSrc;
		LPWSTR pszVal = pszName + lstrlen(pszName) + 1;
		pszSrc = pszVal;
	}
	cchMaxEnvLen = pszSrc - pszAllVars + 1;
	
	// Preparing buffer
	pIn = ExecuteNewCmd(CECMD_EXPORTVARS, sizeof(CESERVER_REQ_HDR)+cchMaxEnvLen*sizeof(wchar_t));
	if (!pIn)
	{
		if (!bSilent)
			_printf(ExpFailedPref ", pIn allocation failed\n");
		goto wrap;
	}
	pszBuffer = (wchar_t*)pIn->wData;

	asCmdArg = SkipNonPrintable(asCmdArg);

	//_ASSERTE(FALSE && "Continue to export");

	// Copy variables to buffer
	if (!asCmdArg || !*asCmdArg || (lstrcmp(asCmdArg, L"*")==0) || (lstrcmp(asCmdArg, L"\"*\"")==0)
		|| (wcsncmp(asCmdArg, L"* ", 2)==0) || (wcsncmp(asCmdArg, L"\"*\" ", 4)==0))
	{
		// transfer ALL variables, except of ConEmu's internals
		pszSrc = pszAllVars;
		while (*pszSrc)
		{
			// may be "=C:=C:\Program Files\..."
			LPWSTR pszEq = wcschr(pszSrc+1, L'=');
			if (!pszEq)
			{
				pszSrc = pszSrc + lstrlen(pszSrc) + 1;
				continue;
			}
			*pszEq = 0;
			LPWSTR pszName = pszSrc;
			LPWSTR pszVal = pszName + lstrlen(pszName) + 1;
			LPWSTR pszNext = pszVal + lstrlen(pszVal) + 1;
			if (IsExportEnvVarAllowed(pszName))
			{
				// Non ConEmu's internals, transfer it
				size_t cchAdd = pszNext - pszName;
				wmemmove(pszBuffer, pszName, cchAdd);
				pszBuffer += cchAdd;
				_ASSERTE(*pszBuffer == 0);
			}
			*pszEq = L'=';
			pszSrc = pszNext;
		}
	}
	else
	{
		wchar_t szTest[MAX_PATH+1];
		while (0==NextArg(&asCmdArg, szTest))
		{
			if (!*szTest || *szTest == L'*')
			{
				if (!bSilent)
					_printf(ExpFailedPref ", invalid name mask\n");
				goto wrap;
			}

			size_t cchLeft = cchMaxEnvLen - 1;
			// Loop through variable names
			pszSrc = pszAllVars;
			while (*pszSrc)
			{
				// may be "=C:=C:\Program Files\..."
				LPWSTR pszEq = wcschr(pszSrc+1, L'=');
				if (!pszEq)
				{
					pszSrc = pszSrc + lstrlen(pszSrc) + 1;
					continue;
				}
				*pszEq = 0;
				LPWSTR pszName = pszSrc;
				LPWSTR pszVal = pszName + lstrlen(pszName) + 1;
				LPWSTR pszNext = pszVal + lstrlen(pszVal) + 1;
				if (IsExportEnvVarAllowed(pszName)
					&& CompareFileMask(pszName, szTest)) // limited
				{
					// Non ConEmu's internals, transfer it
					size_t cchAdd = pszNext - pszName;
					if (cchAdd >= cchLeft)
					{
						*pszEq = L'=';
						if (!bSilent)
							_printf(ExpFailedPref ", too many variables\n");
						goto wrap;
					}
					wmemmove(pszBuffer, pszName, cchAdd);
					pszBuffer += cchAdd;
					_ASSERTE(*pszBuffer == 0);
					cchLeft -= cchAdd;
				}
				*pszEq = L'=';
				pszSrc = pszNext;
			}
		}
	}

	if (pszBuffer == (wchar_t*)pIn->wData)
	{
		if (!bSilent)
			_printf(ExpFailedPref ", nothing to export\n");
		goto wrap;
	}
	_ASSERTE(*pszBuffer==0 && *(pszBuffer-1)==0); // Must be ASCIIZZ
	_ASSERTE((INT_PTR)cchMaxEnvLen >= (pszBuffer - (wchar_t*)pIn->wData + 1));

	// Precise buffer size
	cchMaxEnvLen = pszBuffer - (wchar_t*)pIn->wData + 1;
	_ASSERTE(!(cchMaxEnvLen>>20));
	pIn->hdr.cbSize = sizeof(CESERVER_REQ_HDR)+(DWORD)cchMaxEnvLen*sizeof(wchar_t);

	// Find current server (even if no server or no GUI - apply environment to parent tree)
	lbMapExist = LoadSrvMapping(ghConWnd, test);
	if (lbMapExist)
	{
		_ASSERTE(test.ComSpec.ConEmuExeDir[0] && test.ComSpec.ConEmuBaseDir[0]);
		nSrvPID = test.nServerPID;
	}

	// Allocate memory for process tree
	//pList = (ProcInfo*)malloc(cchMax*sizeof(*pList));
	//if (!pList)
	if (!List.alloc(4096))
	{
		if (!bSilent)
			_printf(ExpFailedPref ", List allocation failed\n");
		goto wrap;
	}

	// Go, build tree (first step - query all running PIDs in the system)
	nParentPID = nSelfPID;
	h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (h && (h != INVALID_HANDLE_VALUE))
	{
		PROCESSENTRY32 PI = {sizeof(PI)};
		if (Process32First(h, &PI))
		{
			do {
				if (PI.th32ProcessID == nSelfPID)
				{
					// On the first step we'll get our parent process
					nParentPID = PI.th32ParentProcessID;
				}
				CharUpperBuff(PI.szExeFile, lstrlen(PI.szExeFile));
				LPCWSTR pszName = PointToName(PI.szExeFile);
				ProcInfo pi = {
					PI.th32ProcessID,
					PI.th32ParentProcessID,
					((lstrcmp(pszName, L"CONEMUC.EXE") == 0) || (lstrcmp(pszName, L"CONEMUC64.EXE") == 0)) ? 1 : 0
				};
				List.push_back(pi);
			} while (Process32Next(h, &PI));
		}

		CloseHandle(h);
	}

	// Send to parents tree
	if (!List.empty() && (nParentPID != nSelfPID))
	{
		DWORD nOurParentPID = nParentPID;
		// Loop while parent is found
		bool bParentFound = true;
		while (nParentPID != nSrvPID)
		{
			nTestPID = nParentPID;
			bParentFound = false;
			// find next parent
			INT_PTR i = 0;
			INT_PTR nCount = List.size();
			while (i < nCount)
			{
				const ProcInfo& pi = List[i];
				if (pi.nPID == nTestPID)
				{
					nParentPID = pi.nParentPID;
					if (pi.Flags & 1)
					{
						// ConEmuC - пропустить
						i = 0;
						nTestPID = nParentPID;
						continue;
					}
					bParentFound = true;
					break;
				}
				i++;
			}

			if (nTestPID == nSrvPID)
				break; // Fin, this is root server
			if (!bParentFound)
				break; // Fin, no more parents

			//_ASSERTE(nTestPID != nOurParentPID); - вполне себе может far->conemuc->cmd->conemuc/export

			// Apply environment
			CESERVER_REQ *pOut = ExecuteHkCmd(nTestPID, pIn, ghConWnd);

			if (!pOut && !bSilent)
			{
				_printf(ExpFailedPref " to PID=%u, check <Inject ConEmuHk>\n", nTestPID);
			}

			ExecuteFreeResult(pOut);
		}
	}

	// В корневом сервере тоже применить
	{
		CESERVER_REQ *pOut = ExecuteSrvCmd(nSrvPID, pIn, ghConWnd);

		if (!pOut)
		{
			if (!bSilent)
				_printf(ExpFailedPref " to PID=%u, root server was terminated?\n", nSrvPID);
		}
		else
		{
			ExecuteFreeResult(pOut);
		}
	}

	// Если просили во все табы - тогда досылаем и в GUI
	if ((eExecAction != ea_ExportCon) && lbMapExist && test.hConEmuRoot && IsWindow((HWND)test.hConEmuRoot))
	{
		if (eExecAction == ea_ExportAll)
		{
			pIn->hdr.nCmd = CECMD_EXPORTVARSALL;
		}
		else
		{
			_ASSERTE(pIn->hdr.nCmd == CECMD_EXPORTVARS);
		}

		ExecuteGuiCmd(ghConWnd, pIn, ghConWnd, TRUE);
	}

wrap:
	// Fin
	if (pszAllVars)
		FreeEnvironmentStringsW((LPWCH)pszAllVars);
	if (pIn)
		ExecuteFreeResult(pIn);

	return iRc;
}

int DoGuiMacro(LPCWSTR asCmdArg)
{
	// Все что в asCmdArg - выполнить в Gui
	int iRc = CERR_GUIMACRO_FAILED;
	int nLen = lstrlen(asCmdArg);
	//SetEnvironmentVariable(CEGUIMACRORETENVVAR, NULL);
	CESERVER_REQ *pIn = NULL, *pOut = NULL;
	pIn = ExecuteNewCmd(CECMD_GUIMACRO, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_GUIMACRO)+nLen*sizeof(wchar_t));
	lstrcpyW(pIn->GuiMacro.sMacro, asCmdArg);
	pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
	if (pOut)
	{
		if (pOut->GuiMacro.nSucceeded)
		{
			// Передать переменную в родительский процесс
			// Сработает только если включены хуки (Inject ConEmuHk)
			// И может игнорироваться некоторыми шеллами (если победить не удастся)
			DoExportEnv(CEGUIMACRORETENVVAR, ea_ExportCon, true/*bSilent*/);

			// Show macro result in StdOutput	
			_wprintf(pOut->GuiMacro.sMacro);
			iRc = CERR_GUIMACRO_SUCCEEDED; // OK
		}
		ExecuteFreeResult(pOut);
	}
	ExecuteFreeResult(pIn);
	return iRc;
}

int DoExecAction(ConEmuExecAction eExecAction, LPCWSTR asCmdArg /* rest of cmdline */)
{
	int iRc = CERR_CARGUMENT;

	switch (eExecAction)
	{
	case ea_RegConFont:
		{
			RegisterConsoleFontHKLM(asCmdArg);
			iRc = CERR_EMPTY_COMSPEC_CMDLINE;
			break;
		}
	case ea_InjectHooks:
		{
			iRc = DoInjectHooks((LPWSTR)asCmdArg);
			break;
		}
	case ea_InjectRemote:
	case ea_InjectDefTrm:
		{
			iRc = DoInjectRemote((LPWSTR)asCmdArg, (eExecAction == ea_InjectDefTrm));
			break;
		}
	case ea_GuiMacro:
		{
			iRc = DoGuiMacro(asCmdArg);
			break;
		}
	case ea_CheckUnicodeFont:
		{
			iRc = CheckUnicodeFont();
			break;
		}
	case ea_ExportCon:
	case ea_ExportGui:
	case ea_ExportAll:
		{
			iRc = DoExportEnv(asCmdArg, eExecAction);
			break;
		}
	default:
		_ASSERTE(FALSE && "Unsupported ExecAction code");
	}

	return iRc;
}

void SetWorkEnvVar()
{
	_ASSERTE(gnRunMode == RM_SERVER && !gbNoCreateProcess);
	wchar_t szPath[MAX_PATH*2] = L"";
	GetCurrentDirectory(countof(szPath), szPath);
	SetEnvironmentVariable(ENV_CONEMUWORKDIR_VAR_W, szPath);

	wchar_t szDrive[MAX_PATH];
	SetEnvironmentVariable(ENV_CONEMUWORKDRIVE_VAR_W, GetDrive(szPath, szDrive, countof(szDrive)));
	GetModuleFileName(ghOurModule, szPath, countof(szPath));
	SetEnvironmentVariable(ENV_CONEMUDRIVE_VAR_W, GetDrive(szPath, szDrive, countof(szDrive)));
}

// 1. Заменить подстановки вида: !ConEmuHWND!, !ConEmuDrawHWND!, !ConEmuBackHWND!, !ConEmuWorkDir!
// 2. Развернуть переменные окружения (PowerShell, например, не признает переменные в качестве параметров)
wchar_t* ParseConEmuSubst(LPCWSTR asCmd, bool bUpdateTitle /*= false*/)
{
	if (!asCmd || !*asCmd)
		return NULL;

	// Другие имена нет смысла передавать через "!" вместо "%"
	LPCWSTR szNames[] = {ENV_CONEMUHWND_VAR_W, ENV_CONEMUDRAW_VAR_W, ENV_CONEMUBACK_VAR_W, ENV_CONEMUWORKDIR_VAR_W};

#ifdef _DEBUG
	// Переменные уже должны быть определены!
	for (size_t i = 0; i < countof(szNames); ++i)
	{
		LPCWSTR pszName = szNames[i];
		wchar_t szDbg[MAX_PATH+1] = L"";
		GetEnvironmentVariable(pszName, szDbg, countof(szDbg));
		if (!*szDbg)
		{
			_ASSERTE(*szDbg && "Variables must be set already!");
		}
	}
#endif

	// Если ничего похожего нет, то и не дергаться
	wchar_t szFind[] = L"!ConEmu";
	bool bExclSubst = (StrStrI(asCmd, szFind) != NULL);
	if (!bExclSubst && (wcschr(asCmd, L'%') == NULL))
		return NULL;

	//_ASSERTE(FALSE && "Continue to ParseConEmuSubst");

	wchar_t* pszCmdCopy = NULL;

	if (bExclSubst)
	{
		wchar_t* pszCmdCopy = lstrdup(asCmd);
		if (!pszCmdCopy)
			return NULL; // Ошибка выделения памяти вообще-то

		for (size_t i = 0; i < countof(szNames); ++i)
		{
			wchar_t szName[64]; _wsprintf(szName, SKIPLEN(countof(szName)) L"!%s!", szNames[i]);
			size_t iLen = lstrlen(szName);

			wchar_t* pszStart = StrStrI(pszCmdCopy, szName);
			if (!pszStart)
				continue;
			while (pszStart)
			{
				pszStart[0] = L'%';
				pszStart[iLen-1] = L'%';

				pszStart = StrStrI(pszStart+iLen, szName);
			}
		}

		asCmd = pszCmdCopy;
	}

	wchar_t* pszExpand = ExpandEnvStr(asCmd);
	if (bUpdateTitle && pszExpand)
	{
		BOOL lbNeedCutStartEndQuot = (pszExpand[0] == L'"');
		UpdateConsoleTitle(pszExpand, /* IN/OUT */lbNeedCutStartEndQuot, false);
	}

	SafeFree(pszCmdCopy);
	return pszExpand;
}

BOOL SetTitle(bool bExpandVars, LPCWSTR lsTitle)
{
	wchar_t* pszExpanded = (bExpandVars && lsTitle) ? ParseConEmuSubst(lsTitle, false) : NULL;
	BOOL bRc = SetConsoleTitle(pszExpanded ? pszExpanded : lsTitle ? lsTitle : L"");
	SafeFree(pszExpanded);
	return bRc;
}

void UpdateConsoleTitle(LPCWSTR lsCmdLine, BOOL& lbNeedCutStartEndQuot, bool bExpandVars)
{
	// Сменим заголовок консоли
	if (*lsCmdLine == L'"')
	{
		if (lsCmdLine[1])
		{
			wchar_t *pszTitle = gpszRunCmd;
			wchar_t *pszEndQ = pszTitle + lstrlenW(pszTitle) - 1;

			if (pszEndQ > (pszTitle+1) && *pszEndQ == L'"'
			        && wcschr(pszTitle+1, L'"') == pszEndQ)
			{
				*pszEndQ = 0; pszTitle ++;
				bool lbCont = true;

				// "F:\Temp\1\ConsoleTest.exe ." - кавычки не нужны, после программы идут параметры
				if (lbCont && (*pszTitle != L'"') && ((*(pszEndQ-1) == L'.') ||(*(pszEndQ-1) == L' ')))
				{
					LPCWSTR pwszCopy = pszTitle;
					wchar_t szTemp[MAX_PATH+1];

					if (NextArg(&pwszCopy, szTemp) == 0)
					{
						// В полученном пути к файлу (исполняемому) не должно быть пробелов?
						if (!wcschr(szTemp, ' ') && IsFilePath(szTemp) && FileExists(szTemp))
						{
							lbCont = false;
							lbNeedCutStartEndQuot = TRUE;
						}
					}
				}

				// "C:\Program Files\FAR\far.exe" - кавычки нужны, иначе не запустится
				if (lbCont)
				{
					if (IsFilePath(pszTitle) && FileExists(pszTitle))
					{
						lbCont = false;
						lbNeedCutStartEndQuot = FALSE;
					}

					//DWORD dwFileAttr = GetFileAttributes(pszTitle);
					//if (dwFileAttr != INVALID_FILE_ATTRIBUTES && !(dwFileAttr & FILE_ATTRIBUTE_DIRECTORY))
					//	lbNeedCutStartEndQuot = FALSE;
					//else
					//	lbNeedCutStartEndQuot = TRUE;
				}
			}
			else
			{
				pszEndQ = NULL;
			}

			int nLen = 4096; //GetWindowTextLength(ghConWnd); -- KIS2009 гундит "Посылка оконного сообщения"...

			if (nLen > 0)
			{
				gpszPrevConTitle = (wchar_t*)calloc(nLen+1,2);

				if (gpszPrevConTitle)
				{
					if (!GetConsoleTitleW(gpszPrevConTitle, nLen+1))
					{
						free(gpszPrevConTitle); gpszPrevConTitle = NULL;
					}
				}
			}

			SetTitle(bExpandVars/*true*/, pszTitle);

			if (pszEndQ) *pszEndQ = L'"';

			return; // Done
		}
	}
	
	if (*lsCmdLine)
	{
		int nLen = 4096; //GetWindowTextLength(ghConWnd); -- KIS2009 гундит "Посылка оконного сообщения"...

		if (nLen > 0)
		{
			gpszPrevConTitle = (wchar_t*)calloc(nLen+1,2);

			if (gpszPrevConTitle)
			{
				if (!GetConsoleTitleW(gpszPrevConTitle, nLen+1))
				{
					free(gpszPrevConTitle); gpszPrevConTitle = NULL;
				}
			}
		}

		SetTitle(bExpandVars/*true*/, lsCmdLine);
	}
}

// Разбор параметров командной строки
int ParseCommandLine(LPCWSTR asCmdLine/*, wchar_t** psNewCmd, BOOL* pbRunInBackgroundTab*/)
{
	//if (!psNewCmd || !pbRunInBackgroundTab)
	//{
	//	_ASSERTE(psNewCmd && pbRunInBackgroundTab);
	//	return CERR_CARGUMENT;
	//}
	
	int iRc = 0;
	wchar_t szArg[MAX_PATH+1] = {0}, szExeTest[MAX_PATH+1];
	LPCWSTR pszArgStarts = NULL;
	//wchar_t szComSpec[MAX_PATH+1] = {0};
	//LPCWSTR pwszCopy = NULL;
	//wchar_t* psFilePart = NULL;
	//BOOL bViaCmdExe = TRUE;
	gbRunViaCmdExe = TRUE;
	gbRootIsCmdExe = TRUE;
	gbRunInBackgroundTab = FALSE;
	size_t nCmdLine = 0;
	LPCWSTR pwszStartCmdLine = asCmdLine;
	LPCWSTR lsCmdLine = asCmdLine;
	BOOL lbNeedCutStartEndQuot = FALSE;

	ConEmuStateCheck eStateCheck = ec_None;
	ConEmuExecAction eExecAction = ea_None;

	if (!lsCmdLine || !*lsCmdLine)
	{
		DWORD dwErr = GetLastError();
		_printf("GetCommandLineW failed! ErrCode=0x%08X\n", dwErr);
		return CERR_GETCOMMANDLINE;
	}

	#ifdef _DEBUG
	// Для отлова запуска дебаггера
	//_ASSERTE(wcsstr(lsCmdLine, L"/DEBUGPID=")==0);
	#endif

	gnRunMode = RM_UNDEFINED;

	BOOL lbAttachGuiApp = FALSE;

	while ((iRc = NextArg(&lsCmdLine, szArg, &pszArgStarts)) == 0)
	{
		xf_check();

		if ((szArg[0] == L'/' || szArg[0] == L'-')
		        && (szArg[1] == L'?' || ((szArg[1] & ~0x20) == L'H'))
		        && szArg[2] == 0)
		{
			Help();
			return CERR_HELPREQUESTED;
		}

		// Далее - требуется чтобы у аргумента был "/"
		if (szArg[0] != L'/')
			continue;

		#ifdef _DEBUG
		if (lstrcmpi(szArg, L"/DEBUGTRAP")==0)
		{
			int i, j = 1; j--; i = 1 / j;
		}
		else
		#endif

		if (wcsncmp(szArg, L"/REGCONFONT=", 12)==0)
		{
			eExecAction = ea_RegConFont;
			lsCmdLine = szArg+12;
			break;
		}
		else if (wcsncmp(szArg, L"/SETHOOKS=", 10) == 0)
		{
			eExecAction = ea_InjectHooks;
			lsCmdLine = szArg+10;
			break;
		}
		else if (wcsncmp(szArg, L"/INJECT=", 8) == 0)
		{
			eExecAction = ea_InjectRemote;
			lsCmdLine = szArg+8;
			break;
		}
		else if (wcsncmp(szArg, L"/DEFTRM=", 8) == 0)
		{
			eExecAction = ea_InjectDefTrm;
			lsCmdLine = szArg+8;
			break;
		}
		else if (lstrcmpi(szArg, L"/GUIMACRO")==0)
		{
			// Все что в lsCmdLine - выполнить в Gui
			eExecAction = ea_GuiMacro;
			break;
		}
		else if (lstrcmpi(szArg, L"/CHECKUNICODE")==0)
		{
			eExecAction = ea_CheckUnicodeFont;
			break;
		}
		else if (lstrcmpni(szArg, L"/EXPORT", 7)==0)
		{
			if (lstrcmpi(szArg, L"/EXPORT=ALL")==0 || lstrcmpi(szArg, L"/EXPORTALL")==0)
				eExecAction = ea_ExportAll;
			else if (lstrcmpi(szArg, L"/EXPORT=CON")==0 || lstrcmpi(szArg, L"/EXPORTCON")==0)
				eExecAction = ea_ExportCon;
			else
				eExecAction = ea_ExportGui;
			break;
		}
		else if (lstrcmpi(szArg, L"/IsConEmu")==0)
		{
			eStateCheck = ec_IsConEmu;
			break;
		}
		else if (lstrcmpi(szArg, L"/IsTerm")==0)
		{
			eStateCheck = ec_IsTerm;
			break;
		}
		else if (lstrcmpi(szArg, L"/IsAnsi")==0)
		{
			eStateCheck = ec_IsAnsi;
			break;
		}
		else if (wcscmp(szArg, L"/CONFIRM")==0)
		{
			TODO("уточнить, что нужно в gbAutoDisableConfirmExit");
			gnConfirmExitParm = 1;
			gbAlwaysConfirmExit = TRUE; gbAutoDisableConfirmExit = FALSE;
		}
		else if (wcscmp(szArg, L"/NOCONFIRM")==0)
		{
			gnConfirmExitParm = 2;
			gbAlwaysConfirmExit = FALSE; gbAutoDisableConfirmExit = FALSE;
		}
		else if (wcscmp(szArg, L"/ATTACH")==0)
		{
			#if defined(SHOW_ATTACH_MSGBOX)
			if (!IsDebuggerPresent())
			{
				wchar_t szTitle[100]; _wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuC (PID=%i) /ATTACH", gnSelfPID);
				const wchar_t* pszCmdLine = GetCommandLineW();
				MessageBox(NULL,pszCmdLine,szTitle,MB_SYSTEMMODAL);
			}
			#endif

			if (!gbAttachMode)
				gbAttachMode = am_Simple;
			gnRunMode = RM_SERVER;
		}
		else if (wcscmp(szArg, L"/AUTOATTACH")==0)
		{
			#if defined(SHOW_ATTACH_MSGBOX)
			if (!IsDebuggerPresent())
			{
				wchar_t szTitle[100]; _wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuC (PID=%i) /AUTOATTACH", gnSelfPID);
				const wchar_t* pszCmdLine = GetCommandLineW();
				MessageBox(NULL,pszCmdLine,szTitle,MB_SYSTEMMODAL);
			}
			#endif

			gnRunMode = RM_SERVER;
			gbAttachMode = am_Auto;
			gbAlienMode = TRUE;
			gbNoCreateProcess = TRUE;

			// Еще может быть "/GHWND=NEW" но оно ниже. Там ставится "gpSrv->bRequestNewGuiWnd=TRUE"

			//ConEmu autorun (c) Maximus5
			//Starting "%ConEmuPath%" in "Attach" mode (NewWnd=%FORCE_NEW_WND%)

			if (!IsAutoAttachAllowed())
			{
				if (ghConWnd && IsWindowVisible(ghConWnd))
				{
					_printf("AutoAttach was requested, but skipped\n");
				}
				DisableAutoConfirmExit();
				//_ASSERTE(FALSE && "AutoAttach was called while Update process is in progress?");
				return CERR_AUTOATTACH_NOT_ALLOWED;
			}
		}
		else if (wcsncmp(szArg, L"/GUIATTACH=", 11)==0)
		{
			#if defined(SHOW_ATTACH_MSGBOX)
			if (!IsDebuggerPresent())
			{
				wchar_t szTitle[100]; _wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuC (PID=%i) /GUIATTACH", gnSelfPID);
				const wchar_t* pszCmdLine = GetCommandLineW();
				MessageBox(NULL,pszCmdLine,szTitle,MB_SYSTEMMODAL);
			}
			#endif

			if (!gbAttachMode)
				gbAttachMode = am_Simple;
			lbAttachGuiApp = TRUE;
			wchar_t* pszEnd;
			HWND hAppWnd = (HWND)wcstoul(szArg+11, &pszEnd, 16);
			if (IsWindow(hAppWnd))
				gpSrv->hRootProcessGui = hAppWnd;
			gnRunMode = RM_SERVER;
		}
		else if (wcscmp(szArg, L"/NOCMD")==0)
		{
			gnRunMode = RM_SERVER;
			gbNoCreateProcess = TRUE;
			gbAlienMode = TRUE;
		}
		else if (wcsncmp(szArg, L"/PARENTFARPID=", 14)==0)
		{
			// Для режима RM_COMSPEC нужно будет сохранить "длинный вывод"
			wchar_t* pszEnd = NULL, *pszStart;
			pszStart = szArg+14;
			gpSrv->dwParentFarPID = wcstoul(pszStart, &pszEnd, 10);
		}
		else if (wcsncmp(szArg, L"/PID=", 5)==0 || wcsncmp(szArg, L"/FARPID=", 8)==0 || wcsncmp(szArg, L"/CONPID=", 8)==0)
		{
			gnRunMode = RM_SERVER;
			gbNoCreateProcess = TRUE;
			gbAlienMode = TRUE;
			wchar_t* pszEnd = NULL, *pszStart;

			if (wcsncmp(szArg, L"/FARPID=", 8)==0)
			{
				gbAttachFromFar = TRUE;
				gbRootIsCmdExe = FALSE;
				pszStart = szArg+8;
			}
			else if (wcsncmp(szArg, L"/CONPID=", 8)==0)
			{
				_ASSERTE(FALSE && "Continue to alternative attach mode");
				gbAlternativeAttach = TRUE;
				gbRootIsCmdExe = FALSE;
				pszStart = szArg+8;
			}
			else
			{
				pszStart = szArg+5;
			}

			gpSrv->dwRootProcess = wcstoul(pszStart, &pszEnd, 10);

			// -- 
			//if (gpSrv->dwRootProcess)
			//{
			//	_ASSERTE(gpSrv->hRootProcess==NULL); // Еще не должен был быть открыт
			//	gpSrv->hRootProcess = OpenProcess(MY_PROCESS_ALL_ACCESS, FALSE, gpSrv->dwRootProcess);
			//	if (gpSrv->hRootProcess == NULL)
			//	{
			//		gpSrv->hRootProcess = OpenProcess(SYNCHRONIZE|PROCESS_QUERY_INFORMATION, FALSE, gpSrv->dwRootProcess);
			//	}
			//}

			if (gbAlternativeAttach && gpSrv->dwRootProcess)
			{
				// Если процесс был запущен "с консольным окном"
				if (ghConWnd)
				{
					#ifdef _DEBUG
					SafeCloseHandle(ghFarInExecuteEvent);
					#endif
				}

				BOOL bAttach = FALSE;

				HMODULE hKernel = GetModuleHandle(L"kernel32.dll");
				AttachConsole_t AttachConsole_f = hKernel ? (AttachConsole_t)GetProcAddress(hKernel,"AttachConsole") : NULL;

				HWND hSaveCon = GetConsoleWindow();

				RetryAttach:

				if (AttachConsole_f)
				{
					// FreeConsole нужно дергать даже если ghConWnd уже NULL. Что-то в винде глючит и
					// AttachConsole вернет ERROR_ACCESS_DENIED, если FreeConsole не звать...
					FreeConsole();
					ghConWnd = NULL;

					// Issue 998: Need to wait, while real console will appears
					// gpSrv->hRootProcess еще не открыт
					HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, gpSrv->dwRootProcess);
					while (hProcess)
					{
						DWORD nConPid = 0;
						HWND hNewCon = FindWindowEx(NULL, NULL, RealConsoleClass, NULL);
						while (hNewCon)
						{
							if (GetWindowThreadProcessId(hNewCon, &nConPid) && (nConPid == gpSrv->dwRootProcess))
								break;
							hNewCon = FindWindowEx(NULL, hNewCon, RealConsoleClass, NULL);
						}

						if ((hNewCon != NULL) || (WaitForSingleObject(hProcess, 100) == WAIT_OBJECT_0))
							break;
					}
					SafeCloseHandle(hProcess);

					bAttach = AttachConsole_f(gpSrv->dwRootProcess);
				}
				else
				{
					SetLastError(ERROR_PROC_NOT_FOUND);
				}

				if (!bAttach)
				{
					DWORD nErr = GetLastError();
					size_t cchMsgMax = 10*MAX_PATH;
					wchar_t* pszMsg = (wchar_t*)calloc(cchMsgMax,sizeof(*pszMsg));
					wchar_t szTitle[MAX_PATH];
					HWND hFindConWnd = FindWindowEx(NULL, NULL, RealConsoleClass, NULL);
					DWORD nFindConPID = 0; if (hFindConWnd) GetWindowThreadProcessId(hFindConWnd, &nFindConPID);

					PROCESSENTRY32 piCon = {}, piRoot = {};
					GetProcessInfo(gpSrv->dwRootProcess, &piRoot);
					if (nFindConPID == gpSrv->dwRootProcess)
						piCon = piRoot;
					else if (nFindConPID)
						GetProcessInfo(nFindConPID, &piCon);

					if (hFindConWnd)
						GetWindowText(hFindConWnd, szTitle, countof(szTitle));
					else
						szTitle[0] = 0;

					_wsprintf(pszMsg, SKIPLEN(cchMsgMax)
						L"AttachConsole(PID=%u) failed, code=%u\n"
						L"[%u]: %s\n"
						L"Top console HWND=x%08X, PID=%u, %s\n%s\n---\n"
						L"Prev (self) console HWND=x%08X\n\n"
						L"Retry?",
						gpSrv->dwRootProcess, nErr,
						gpSrv->dwRootProcess, piRoot.szExeFile,
						(DWORD)hFindConWnd, nFindConPID, piCon.szExeFile, szTitle,
						(DWORD)hSaveCon
						);
					
					_wsprintf(szTitle, SKIPLEN(countof(szTitle)) WIN3264TEST(L"ConEmuC",L"ConEmuC64") L": PID=%u", GetCurrentProcessId());

					int nBtn = MessageBox(NULL, pszMsg, szTitle, MB_ICONSTOP|MB_SYSTEMMODAL|MB_RETRYCANCEL);

					free(pszMsg);

					if (nBtn == IDRETRY)
					{
						goto RetryAttach;
					}

					gbInShutdown = TRUE;
					gbAlwaysConfirmExit = FALSE;
					return CERR_CARGUMENT;
				}

				ghConWnd = GetConEmuHWND(2);
				gbVisibleOnStartup = IsWindowVisible(ghConWnd);
				#ifdef _DEBUG
				_ASSERTE(ghFarInExecuteEvent==NULL);
				_ASSERTE(ghConWnd!=NULL);
				#endif
			}
            else if (gpSrv->dwRootProcess == 0)
			{
				_printf("Attach to GUI was requested, but invalid PID specified:\n");
				_wprintf(GetCommandLineW());
				_printf("\n");
				_ASSERTE(FALSE);
				return CERR_CARGUMENT;
			}
		}
		else if (wcsncmp(szArg, L"/CINMODE=", 9)==0)
		{
			wchar_t* pszEnd = NULL, *pszStart = szArg+9;
			gnConsoleModeFlags = wcstoul(pszStart, &pszEnd, 16);
			// если передан 0 - включится (ENABLE_QUICK_EDIT_MODE|ENABLE_EXTENDED_FLAGS|ENABLE_INSERT_MODE)
			gbConsoleModeFlags = (gnConsoleModeFlags != 0);
		}
		else if (wcscmp(szArg, L"/HIDE")==0)
		{
			gbForceHideConWnd = TRUE;
		}
		else if (wcsncmp(szArg, L"/B", 2)==0)
		{
			wchar_t* pszEnd = NULL;

			if (wcsncmp(szArg, L"/BW=", 4)==0)
			{
				gcrVisibleSize.X = /*_wtoi(szArg+4);*/(SHORT)wcstol(szArg+4,&pszEnd,10); gbParmVisibleSize = TRUE;
			}
			else if (wcsncmp(szArg, L"/BH=", 4)==0)
			{
				gcrVisibleSize.Y = /*_wtoi(szArg+4);*/(SHORT)wcstol(szArg+4,&pszEnd,10); gbParmVisibleSize = TRUE;
			}
			else if (wcsncmp(szArg, L"/BZ=", 4)==0)
			{
				gnBufferHeight = /*_wtoi(szArg+4);*/(SHORT)wcstol(szArg+4,&pszEnd,10); gbParmBufSize = TRUE;
			}
			TODO("/BX для ширины буфера?");
		}
		else if (wcsncmp(szArg, L"/F", 2)==0)
		{
			wchar_t* pszEnd = NULL;

			if (wcsncmp(szArg, L"/FN=", 4)==0) //-V112
			{
				lstrcpynW(gpSrv->szConsoleFont, szArg+4, 32); //-V112
			}
			else if (wcsncmp(szArg, L"/FW=", 4)==0) //-V112
			{
				gpSrv->nConFontWidth = /*_wtoi(szArg+4);*/(SHORT)wcstol(szArg+4,&pszEnd,10);
			}
			else if (wcsncmp(szArg, L"/FH=", 4)==0) //-V112
			{
				gpSrv->nConFontHeight = /*_wtoi(szArg+4);*/(SHORT)wcstol(szArg+4,&pszEnd,10);
				//} else if (wcsncmp(szArg, L"/FF=", 4)==0) {
				//  lstrcpynW(gpSrv->szConsoleFontFile, szArg+4, MAX_PATH);
			}
		}
		else if (wcsncmp(szArg, L"/LOG",4)==0) //-V112
		{
			int nLevel = 0;
			if (szArg[4]==L'1') nLevel = 1; else if (szArg[4]>=L'2') nLevel = 2;

			CreateLogSizeFile(nLevel);
		}
		else if (wcsncmp(szArg, L"/GID=", 5)==0)
		{
			gnRunMode = RM_SERVER;
			wchar_t* pszEnd = NULL;
			gpSrv->dwGuiPID = wcstoul(szArg+5, &pszEnd, 10);

			if (gpSrv->dwGuiPID == 0)
			{
				_printf("Invalid GUI PID specified:\n");
				_wprintf(GetCommandLineW());
				_printf("\n");
				_ASSERTE(FALSE);
				return CERR_CARGUMENT;
			}
		}
		else if (wcsncmp(szArg, L"/AID=", 5)==0)
		{
			wchar_t* pszEnd = NULL;
			gpSrv->dwGuiAID = wcstoul(szArg+5, &pszEnd, 10);
		}
		else if (wcsncmp(szArg, L"/GHWND=", 7)==0)
		{
			gnRunMode = RM_SERVER;
			wchar_t* pszEnd = NULL;
			if (lstrcmpi(szArg+7, L"NEW") == 0)
			{
				gpSrv->hGuiWnd = NULL;
				gpSrv->bRequestNewGuiWnd = TRUE;
			}
			else
			{
				gpSrv->hGuiWnd = (HWND)wcstoul(szArg+7, &pszEnd, 16);

				if ((gpSrv->hGuiWnd) == NULL || !IsWindow(gpSrv->hGuiWnd))
				{
					_printf("Invalid GUI HWND specified:\n");
					_wprintf(GetCommandLineW());
					_printf("\n");
					_ASSERTE(FALSE);
					return CERR_CARGUMENT;
				}
			}
		}
		else if (wcsncmp(szArg, L"/TA=", 4)==0)
		{
			wchar_t* pszEnd = NULL;
        	DWORD nColors = wcstoul(szArg+4, &pszEnd, 16);
        	if (nColors)
        	{
        		DWORD nTextIdx = (nColors & 0xFF);
        		DWORD nBackIdx = ((nColors >> 8) & 0xFF);
        		DWORD nPopTextIdx = ((nColors >> 16) & 0xFF);
        		DWORD nPopBackIdx = ((nColors >> 24) & 0xFF);

        		if ((nTextIdx <= 15) && (nBackIdx <= 15) && (nTextIdx != nBackIdx))
        			gnDefTextColors = (WORD)(nTextIdx | (nBackIdx << 4));

        		if ((nPopTextIdx <= 15) && (nPopBackIdx <= 15) && (nPopTextIdx != nPopBackIdx))
        			gnDefPopupColors = (WORD)(nPopTextIdx | (nPopBackIdx << 4));

        		if (gnDefTextColors || gnDefPopupColors)
        		{
        			HANDLE hConOut = ghConOut;
        			BOOL bPassed = FALSE;

        			if (gnDefPopupColors && (gnOsVer >= 0x600))
        			{
						CONSOLE_SCREEN_BUFFER_INFO csbi0 = {};
						GetConsoleScreenBufferInfo(hConOut, &csbi0);

						MY_CONSOLE_SCREEN_BUFFER_INFOEX csbi = {sizeof(csbi)};
						if (apiGetConsoleScreenBufferInfoEx(hConOut, &csbi))
						{
							if ((!gnDefTextColors || (csbi.wAttributes = gnDefTextColors))
								&& (!gnDefPopupColors || (csbi.wPopupAttributes = gnDefPopupColors)))
							{
								bPassed = TRUE; // Менять не нужно, консоль соответствует
							}
							else
							{
								if (gnDefTextColors)
									csbi.wAttributes = gnDefTextColors;
								if (gnDefPopupColors)
									csbi.wPopupAttributes = gnDefPopupColors;

								_ASSERTE(FALSE && "Continue to SetConsoleScreenBufferInfoEx");

								// Vista/Win7. _SetConsoleScreenBufferInfoEx unexpectedly SHOWS console window
								//if (gnOsVer == 0x0601)
								//{
								//	RECT rcGui = {};
								//	if (gpSrv->hGuiWnd)
								//		GetWindowRect(gpSrv->hGuiWnd, &rcGui);
								//	//SetWindowPos(ghConWnd, HWND_BOTTOM, rcGui.left+3, rcGui.top+3, 0,0, SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOZORDER);
								//	SetWindowPos(ghConWnd, NULL, -30000, -30000, 0,0, SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOZORDER);
								//	ShowWindow(ghConWnd, SW_SHOWMINNOACTIVE);
								//	#ifdef _DEBUG
								//	ShowWindow(ghConWnd, SW_SHOWNORMAL);
								//	ShowWindow(ghConWnd, SW_HIDE);
								//	#endif
								//}

								bPassed = apiSetConsoleScreenBufferInfoEx(hConOut, &csbi);

								// Что-то Win7 хулиганит
								if (!gbVisibleOnStartup)
								{
									ShowWindow(ghConWnd, SW_HIDE);
								}
							}
						}
					}


					if (!bPassed && gnDefTextColors)
					{
        				SetConsoleTextAttribute(hConOut, gnDefTextColors);
    				}
        		}
        	}
		}
		else if (lstrcmpni(szArg, L"/DEBUGPID=", 10)==0)
		{
			//gnRunMode = RM_SERVER; -- не будем ставить, RM_UNDEFINED будет признаком того, что просто хотят дамп
			gbNoCreateProcess = gpSrv->DbgInfo.bDebugProcess = TRUE;
			wchar_t* pszEnd = NULL;
			//gpSrv->dwRootProcess = _wtol(szArg+10);
			gpSrv->dwRootProcess = wcstoul(szArg+10, &pszEnd, 10);

			if (gpSrv->dwRootProcess == 0)
			{
				_printf("Debug of process was requested, but invalid PID specified:\n");
				_wprintf(GetCommandLineW());
				_printf("\n");
				_ASSERTE(FALSE);
				return CERR_CARGUMENT;
			}

			if (pszEnd && (*pszEnd == L','))
			{
				gpSrv->DbgInfo.pDebugAttachProcesses = new MArray<DWORD>;
				while (pszEnd && (*pszEnd == L','))
				{
					DWORD nPID = wcstoul(pszEnd+1, &pszEnd, 10);
					if (nPID != 0)
						gpSrv->DbgInfo.pDebugAttachProcesses->push_back(nPID);
				}
			}
		}
		else if (lstrcmpi(szArg, L"/DEBUGEXE")==0 || lstrcmpi(szArg, L"/DEBUGTREE")==0)
		{
			wchar_t* pszLine = lstrdup(GetCommandLineW());
			if (!pszLine || !*pszLine)
			{
				_printf("Debug of process was requested, but GetCommandLineW failed\n");
				_ASSERTE(FALSE);
				return CERR_CARGUMENT;
			}

			gpSrv->DbgInfo.bDebugProcessTree = (lstrcmpi(szArg, L"/DEBUGTREE")==0);

			LPWSTR pszDebugCmd = wcsstr(pszLine, szArg);

			if (pszDebugCmd)
			{
				pszDebugCmd = (LPWSTR)SkipNonPrintable(pszDebugCmd + lstrlen(szArg));
			}

			if (!pszDebugCmd || !*pszDebugCmd)
			{
				_printf("Debug of process was requested, but command was not found\n");
				_ASSERTE(FALSE);
				return CERR_CARGUMENT;
			}

			/*
			STARTUPINFO si = {sizeof(si)};
			PROCESS_INFORMATION pi = {};

			if (!CreateProcess(NULL, pszDebugCmd, NULL, NULL, FALSE,
				NORMAL_PRIORITY_CLASS|CREATE_NEW_CONSOLE|DEBUG_ONLY_THIS_PROCESS,
				NULL, NULL, &si, &pi))
			{
				_printf("Debug of process was requested, but CreateProcess failed\n");
				_ASSERTE(FALSE);
				return CERR_CARGUMENT;
			}
			*/

			gbNoCreateProcess = gpSrv->DbgInfo.bDebugProcess = TRUE;
			gpSrv->DbgInfo.pszDebuggingCmdLine = pszDebugCmd;
			/*
			gpSrv->hRootProcess = pi.hProcess;
			gpSrv->hRootThread = pi.hThread;
			gpSrv->dwRootProcess = pi.dwProcessId;
			gpSrv->dwRootThread = pi.dwThreadId;
			gpSrv->dwRootStartTime = GetTickCount();
			*/

			break;
		}
		else if (lstrcmpi(szArg, L"/DUMP")==0)
		{
			gpSrv->DbgInfo.nDebugDumpProcess = 1;
		}
		else if (lstrcmpi(szArg, L"/MINIDUMP")==0 || (gpSrv->DbgInfo.bDebugProcess && lstrcmpi(szArg, L"/MINI")==0))
		{
			gpSrv->DbgInfo.nDebugDumpProcess = 2;
		}
		else if (lstrcmpi(szArg, L"/FULLDUMP")==0 || (gpSrv->DbgInfo.bDebugProcess && lstrcmpi(szArg, L"/FULL")==0))
		{
			gpSrv->DbgInfo.nDebugDumpProcess = 3;
		}
		else if (wcscmp(szArg, L"/A")==0 || wcscmp(szArg, L"/a")==0)
		{
			gnCmdUnicodeMode = 1;
		}
		else if (wcscmp(szArg, L"/U")==0 || wcscmp(szArg, L"/u")==0)
		{
			gnCmdUnicodeMode = 2;
		}
		// После этих аргументов - идет то, что передается в CreateProcess!
		else if (wcscmp(szArg, L"/ROOT")==0 || wcscmp(szArg, L"/root")==0)
		{
			gnRunMode = RM_SERVER; gbNoCreateProcess = FALSE;
			SetWorkEnvVar();
			break; // lsCmdLine уже указывает на запускаемую программу
		}
		else if (lstrcmpi(szArg, L"/NOINJECT")==0)
		{
			gbDontInjectConEmuHk = TRUE;
		}
		else if (lstrcmpi(szArg, L"/DOSBOX")==0)
		{
			gbUseDosBox = TRUE;
		}
		// После этих аргументов - идет то, что передается в COMSPEC (CreateProcess)!
		//if (wcscmp(szArg, L"/C")==0 || wcscmp(szArg, L"/c")==0 || wcscmp(szArg, L"/K")==0 || wcscmp(szArg, L"/k")==0) {
		else if (szArg[0] == L'/' && (((szArg[1] & ~0x20) == L'C') || ((szArg[1] & ~0x20) == L'K')))
		{
			gbNoCreateProcess = FALSE;

			if (szArg[2] == 0)  // "/c" или "/k"
				gnRunMode = RM_COMSPEC;

			if (gnRunMode == RM_UNDEFINED && szArg[4] == 0
			        && ((szArg[2] & ~0x20) == L'M') && ((szArg[3] & ~0x20) == L'D'))
			{
				_ASSERTE(FALSE && "'/cmd' obsolete switch. use /c, /k, /root");
				gnRunMode = RM_SERVER;
			}

			// Если тип работа до сих пор не определили - считаем что режим ComSpec
			// и команда начинается сразу после /c (может быть "cmd /cecho xxx")
			if (gnRunMode == RM_UNDEFINED)
			{
				gnRunMode = RM_COMSPEC;
				// Поддержка возможности "cmd /cecho xxx"
				lsCmdLine = SkipNonPrintable(pszArgStarts + 2);
			}

			if (gnRunMode == RM_COMSPEC)
			{
				gpSrv->bK = (szArg[1] & ~0x20) == L'K';
			}

			break; // lsCmdLine уже указывает на запускаемую программу
		}
	}


	// Some checks or actions
	if (eStateCheck || eExecAction)
	{
		int iFRc = CERR_CARGUMENT;

		if (eStateCheck)
		{
			bool bOn = DoStateCheck(eStateCheck);
			iFRc = bOn ? CERR_CHKSTATE_ON : CERR_CHKSTATE_OFF;
		}
		else if (eExecAction)
		{
			iFRc = DoExecAction(eExecAction, lsCmdLine);
		}

		// И сразу на выход
		gbInShutdown = TRUE;
		return iFRc;
	}


	if ((gnRunMode == RM_SERVER) && gpSrv->hGuiWnd)
	{
		ReloadGuiSettings(NULL);
	}

	// Issue 364, например, идет билд в VS, запускается CustomStep, в этот момент автоаттач нафиг не нужен
	// Теоретически, в Студии не должно бы быть запуска ConEmuC.exe, но он может оказаться в "COMSPEC", так что проверим.
	if (gbAttachMode && (gnRunMode == RM_SERVER) && (gpSrv->dwGuiPID == 0))
	{
		_ASSERTE(!gbAlternativeAttach && "Alternative mode must be already processed!");

		BOOL lbIsWindowVisible = FALSE;
		// Добавим проверку на telnet
		if (!ghConWnd || !(lbIsWindowVisible = IsWindowVisible(ghConWnd)) || isTerminalMode())
		{
			// Но это может быть все-таки наше окошко. Как проверить...
			// Найдем первый параметр
			LPCWSTR pszSlash = lsCmdLine ? wcschr(lsCmdLine, L'/') : NULL;
			if (pszSlash)
			{
				// И сравним с используемыми у нас. Возможно потом еще что-то добавить придется
				if (wmemcmp(pszSlash, L"/DEBUGPID=", 10) != 0)
					pszSlash = NULL;
			}
			if (pszSlash == NULL)
			{
				// Не наше окошко, выходим
				gbInShutdown = TRUE;
				return CERR_ATTACH_NO_CONWND;
			}
		}

		if (!gbAlternativeAttach && !gpSrv->dwRootProcess)
		{
			// Из батника убрал, покажем инфу тут
			PrintVersion();
			char szAutoRunMsg[128];
			_wsprintfA(szAutoRunMsg, SKIPLEN(countof(szAutoRunMsg)) "Starting attach autorun (NewWnd=%s)\n", gpSrv->bRequestNewGuiWnd ? "YES" : "NO");
			_printf(szAutoRunMsg);
		}
	}

	xf_check();

	if ((gnRunMode == RM_SERVER) || (gpSrv->DbgInfo.bDebugProcess && (gnRunMode == RM_UNDEFINED)))
	{
		if (gpSrv->DbgInfo.bDebugProcess)
		{
			// Запустить поток дебаггера и дождаться его завершения
			int iDbgRc = RunDebugger();
			return iDbgRc;
		}
		else if (gbNoCreateProcess && gbAttachMode)
		{
			// Проверить процессы в консоли, подобрать тот, который будем считать "корневым"
			int nChk = CheckAttachProcess();

			if (nChk != 0)
				return nChk;

			gpszRunCmd = (wchar_t*)calloc(1,2);

			if (!gpszRunCmd)
			{
				_printf("Can't allocate 1 wchar!\n");
				return CERR_NOTENOUGHMEM1;
			}

			gpszRunCmd[0] = 0;
			return 0;
		}
	}

	xf_check();

	if (iRc != 0)
	{
		if (iRc == CERR_CMDLINEEMPTY)
		{
			Help();
			_printf("\n\nParsing command line failed (/C argument not found):\n");
			_wprintf(GetCommandLineW());
			_printf("\n");
		}
		else
		{
			_printf("Parsing command line failed:\n");
			_wprintf(asCmdLine);
			_printf("\n");
		}

		return iRc;
	}

	if (gnRunMode == RM_UNDEFINED)
	{
		_printf("Parsing command line failed (/C argument not found):\n");
		_wprintf(GetCommandLineW());
		_printf("\n");
		_ASSERTE(FALSE);
		return CERR_CARGUMENT;
	}

	xf_check();

	if (gnRunMode == RM_COMSPEC)
	{
		// Может просили открыть новую консоль?
		int nArgLen = lstrlenA("-new_console");
		LPCWSTR pwszCopy = (wchar_t*)wcsstr(lsCmdLine, L"-new_console");

		// Если после -new_console идет пробел, или это вообще конец строки
		// 111211 - после -new_console: допускаются параметры
		if (pwszCopy
			&& ((pwszCopy > lsCmdLine) || (*(pwszCopy-1) == L' ') || (*(pwszCopy-1) == L'"'))
			&& (pwszCopy[nArgLen]==L' ' || pwszCopy[nArgLen]==L':' || pwszCopy[nArgLen]==0
		         || (pwszCopy[nArgLen]==L'"' || pwszCopy[nArgLen+1]==0)))
		{
			if (!ghConWnd)
			{
				// Недопустимо!
				_ASSERTE(ghConWnd != NULL);
			}
			else
			{
				xf_check();
				// тогда обрабатываем
				gpSrv->bNewConsole = TRUE;

				// По идее, должен запускаться в табе ConEmu (в существующей консоли), но если нет
				HWND hConEmu = ghConEmuWnd;
				if (!hConEmu || !IsWindow(hConEmu))
				{
					_ASSERTE(ghConEmuWnd!=NULL && IsWindow(ghConEmuWnd));
					// попытаться найти открытый ConEmu
					hConEmu = FindWindowEx(NULL, NULL, VirtualConsoleClassMain, NULL);
					if (hConEmu)
						gbNonGuiMode = TRUE; // Чтобы не пытаться выполнить SendStopped (ибо некому)
				}

				int iNewConRc = CERR_RUNNEWCONSOLE;

				DWORD nCmdLen = lstrlen(lsCmdLine);
				CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_NEWCMD, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_NEWCMD)+(nCmdLen*sizeof(wchar_t)));
				if (pIn)
				{
					pIn->NewCmd.hFromConWnd = ghConWnd;
					GetCurrentDirectory(countof(pIn->NewCmd.szCurDir), pIn->NewCmd.szCurDir);
					lstrcpyn(pIn->NewCmd.szCommand, lsCmdLine, nCmdLen+1);

					CESERVER_REQ* pOut = ExecuteGuiCmd(hConEmu, pIn, ghConWnd);
					if (pOut)
					{
						if (pOut->hdr.cbSize <= sizeof(pOut->hdr) || pOut->Data[0] == FALSE)
						{
							iNewConRc = CERR_RUNNEWCONSOLEFAILED;
						}
						ExecuteFreeResult(pOut);
					}
					else
					{
						_ASSERTE(pOut!=NULL);
						iNewConRc = CERR_RUNNEWCONSOLEFAILED;
					}
					ExecuteFreeResult(pIn);
				}
				else
				{
					iNewConRc = CERR_NOTENOUGHMEM1;
				}

				DisableAutoConfirmExit();
				return iNewConRc;
			}
		}

		//pwszCopy = lsCmdLine;
		//if ((iRc = NextArg(&pwszCopy, szArg)) != 0) {
		//    wprintf (L"Parsing command line failed:\n%s\n", lsCmdLine);
		//    return iRc;
		//}
		//pwszCopy = wcsrchr(szArg, L'\\'); if (!pwszCopy) pwszCopy = szArg;
		//#pragma warning( push )
		//#pragma warning(disable : 6400)
		//if (lstrcmpiW(pwszCopy, L"cmd")==0 || lstrcmpiW(pwszCopy, L"cmd.exe")==0) {
		//    gbRunViaCmdExe = FALSE; // уже указан командный процессор, cmd.exe в начало добавлять не нужно
		//}
		//#pragma warning( pop )
		//} else {
		//    gbRunViaCmdExe = FALSE; // командным процессором выступает сам ConEmuC (серверный режим)
	}

	LPCWSTR pszArguments4EnvVar = NULL;

	if (gnRunMode == RM_COMSPEC && (!lsCmdLine || !*lsCmdLine))
	{
		if (gpSrv->bK)
		{
			gbRunViaCmdExe = TRUE;
		}
		else
		{
			// В фаре могут повесить пустую ассоциацию на маску
			// *.ini -> "@" - тогда фар как бы ничего не делает при запуске этого файла, но ComSpec зовет...
			gbNonGuiMode = TRUE;
			DisableAutoConfirmExit();
			return CERR_EMPTY_COMSPEC_CMDLINE;
		}
	}
	else
	{
		BOOL bAlwaysConfirmExit = gbAlwaysConfirmExit, bAutoDisableConfirmExit = gbAutoDisableConfirmExit;

		gpszCheck4NeedCmd = lsCmdLine; // Для отладки

		gbRunViaCmdExe = IsNeedCmd((gnRunMode == RM_SERVER), lsCmdLine, &pszArguments4EnvVar, &lbNeedCutStartEndQuot, szExeTest, 
			gbRootIsCmdExe, bAlwaysConfirmExit, bAutoDisableConfirmExit);

		if (gnConfirmExitParm == 0)
		{
			gbAlwaysConfirmExit = bAlwaysConfirmExit;
			gbAutoDisableConfirmExit = bAutoDisableConfirmExit;
		}
	}

#ifndef WIN64

	// Команды вида: C:\Windows\SysNative\reg.exe Query "HKCU\Software\Far2"|find "Far"
	// Для них нельзя отключать редиректор (wow.Disable()), иначе SysNative будет недоступен
	if (IsWindows64())
	{
		LPCWSTR pszTest = lsCmdLine;
		wchar_t szApp[MAX_PATH+1];

		if (NextArg(&pszTest, szApp) == 0)
		{
			wchar_t szSysnative[MAX_PATH+32];
			int nLen = GetWindowsDirectory(szSysnative, MAX_PATH);

			if (nLen >= 2 && nLen < MAX_PATH)
			{
				if (szSysnative[nLen-1] != L'\\')
				{
					szSysnative[nLen++] = L'\\'; szSysnative[nLen] = 0;
				}

				lstrcatW(szSysnative, L"Sysnative\\");
				nLen = lstrlenW(szSysnative);
				int nAppLen = lstrlenW(szApp);

				if (nAppLen > nLen)
				{
					szApp[nLen] = 0;

					if (lstrcmpiW(szApp, szSysnative) == 0)
					{
						gbSkipWowChange = TRUE;
					}
				}
			}
		}
	}

#endif
	nCmdLine = lstrlenW(lsCmdLine);

	if (!gbRunViaCmdExe)
	{
		nCmdLine += 1; // только место под 0
		if (pszArguments4EnvVar && *szExeTest)
			nCmdLine += lstrlen(szExeTest)+3;
	}
	else
	{
		// Если определена ComSpecC - значит ConEmuC переопределил стандартный ComSpec
		//WARNING("TCC/ComSpec");
		if (!GetEnvironmentVariable(L"ComSpecC", gszComSpec, MAX_PATH) || gszComSpec[0] == 0)
			if (!GetEnvironmentVariable(L"ComSpec", gszComSpec, MAX_PATH) || gszComSpec[0] == 0)
				gszComSpec[0] = 0;

		if (gszComSpec[0] != 0)
		{
			// Только если это (случайно) не conemuc.exe
			//pwszCopy = wcsrchr(gszComSpec, L'\\'); if (!pwszCopy) pwszCopy = gszComSpec;
			LPCWSTR pwszCopy = PointToName(gszComSpec);
#ifndef __GNUC__
#pragma warning( push )
#pragma warning(disable : 6400)
#endif

			if (lstrcmpiW(pwszCopy, L"ConEmuC")==0 || lstrcmpiW(pwszCopy, L"ConEmuC.exe")==0
			        /*|| lstrcmpiW(pwszCopy, L"ConEmuC64")==0 || lstrcmpiW(pwszCopy, L"ConEmuC64.exe")==0*/)
				gszComSpec[0] = 0;

#ifndef __GNUC__
#pragma warning( pop )
#endif
		}

		// ComSpec/ComSpecC не определен, используем cmd.exe
		if (gszComSpec[0] == 0)
		{
			//WARNING("TCC/ComSpec");
			//if (!SearchPathW(NULL, L"cmd.exe", NULL, MAX_PATH, gszComSpec, &psFilePart))

			LPCWSTR pszFind = GetComspecFromEnvVar(gszComSpec, countof(gszComSpec));
            if (!pszFind || !wcschr(pszFind, L'\\') || !FileExists(pszFind))
			{
				_ASSERTE("cmd.exe not found!");
				_printf("Can't find cmd.exe!\n");
				return CERR_CMDEXENOTFOUND;
			}
		}

		nCmdLine += lstrlenW(gszComSpec)+15; // "/C", кавычки и возможный "/U"
	}

	size_t nCchLen = nCmdLine+1; // nCmdLine учитывает длинну lsCmdLine + gszComSpec + еще чуть-чуть на "/C" и прочее
	gpszRunCmd = (wchar_t*)calloc(nCchLen,2);

	if (!gpszRunCmd)
	{
		_printf("Can't allocate %i wchars!\n", (DWORD)nCmdLine);
		return CERR_NOTENOUGHMEM1;
	}

	// это нужно для смены заголовка консоли. при необходимости COMSPEC впишем ниже, после смены
	if (pszArguments4EnvVar && *szExeTest && !gbRunViaCmdExe)
	{
		gpszRunCmd[0] = L'"';
		_wcscat_c(gpszRunCmd, nCchLen, szExeTest);
		if (*pszArguments4EnvVar)
		{
			_wcscat_c(gpszRunCmd, nCchLen, L"\" ");
			_wcscat_c(gpszRunCmd, nCchLen, pszArguments4EnvVar);
		}
		else
		{
			_wcscat_c(gpszRunCmd, nCchLen, L"\"");
		}
	}
	else
	{
		_wcscpy_c(gpszRunCmd, nCchLen, lsCmdLine);
	}
	// !!! gpszRunCmd может поменяться ниже!

	// Сменим заголовок консоли
	UpdateConsoleTitle(lsCmdLine, /* IN/OUT */lbNeedCutStartEndQuot, true);

	// ====
	if (gbRunViaCmdExe)
	{
		CheckUnicodeMode();

		// -- для унификации - окавычиваем всегда
		//if (wcschr(gszComSpec, L' '))
		{
			gpszRunCmd[0] = L'"';
			_wcscpy_c(gpszRunCmd+1, nCchLen-1, gszComSpec);

			if (gnCmdUnicodeMode)
				_wcscat_c(gpszRunCmd, nCchLen, (gnCmdUnicodeMode == 2) ? L" /U" : L" /A");

			_wcscat_c(gpszRunCmd, nCchLen, gpSrv->bK ? L"\" /K " : L"\" /C ");
		}
		//else
		//{
		//	_wcscpy_c(gpszRunCmd, nCchLen, gszComSpec);

		//	if (gnCmdUnicodeMode)
		//		_wcscat_c(gpszRunCmd, nCchLen, (gnCmdUnicodeMode == 2) ? L" /U" : L" /A");

		//	_wcscat_c(gpszRunCmd, nCchLen, gpSrv->bK ? L" /K " : L" /C ");
		//}

		// Наверное можно положиться на фар, и не кавычить самостоятельно
		//BOOL lbNeedQuatete = FALSE;
		// Команды в cmd.exe лучше передавать так:
		// ""c:\program files\arc\7z.exe" -?"
		//int nLastChar = lstrlenW(lsCmdLine) - 1;
		//if (lsCmdLine[0] == L'"' && lsCmdLine[nLastChar] == L'"') {
		//	// Наверное можно положиться на фар, и не кавычить самостоятельно
		//	if (gnRunMode == RM_COMSPEC)
		//		lbNeedQuatete = FALSE;
		//	//if (lsCmdLine[1] == L'"' && lsCmdLine[2])
		//	//	lbNeedQuatete = FALSE; // уже
		//	//else if (wcschr(lsCmdLine+1, L'"') == (lsCmdLine+nLastChar))
		//	//	lbNeedQuatete = FALSE; // не требуется. внутри кавычек нет
		//}
		//if (lbNeedQuatete) { // надо
		//	lstrcatW(gpszRunCmd, L"\"" );
		//}
		// Собственно, командная строка
		_wcscat_c(gpszRunCmd, nCchLen, lsCmdLine);
		//if (lbNeedQuatete)
		//	lstrcatW(gpszRunCmd, L"\"" );
	}
	else if (lbNeedCutStartEndQuot)
	{
		// ""c:\arc\7z.exe -?"" - не запустится!
		_wcscpy_c(gpszRunCmd, nCchLen, lsCmdLine+1);
		wchar_t *pszEndQ = gpszRunCmd + lstrlenW(gpszRunCmd) - 1;
		_ASSERTE(pszEndQ && *pszEndQ == L'"');

		if (pszEndQ && *pszEndQ == L'"') *pszEndQ = 0;
	}
	
	// Теперь выкусить и обработать "-new_console" / "-cur_console"
	RConStartArgs args;
	args.pszSpecialCmd = gpszRunCmd;
	args.ProcessNewConArg();
	args.pszSpecialCmd = NULL; // Чтобы не разрушилась память отведенная в gpszRunCmd
	// Если указана рабочая папка
	if (args.pszStartupDir)
	{
		SetCurrentDirectory(args.pszStartupDir);
	}
	//
	gbRunInBackgroundTab = args.bBackgroundTab;
	if (args.bBufHeight)
	{
		TODO("gcrBufferSize - и ширину буфера");
		gnBufferHeight = args.nBufHeight;
		gbParmBufSize = TRUE;
	}
	// DosBox?
	if (args.bForceDosBox)
	{
		gbUseDosBox = TRUE;
	}
	// Overwrite mode in Prompt?
	if (args.bOverwriteMode)
	{
		gnConsoleModeFlags |= (ENABLE_INSERT_MODE << 16); // Mask
		gnConsoleModeFlags &= ~ENABLE_INSERT_MODE; // Turn bit OFF

		// Поскольку ключик указан через "-cur_console/-new_console"
		// смену режима нужно сделать сразу, т.к. функа зовется только для сервера
		ServerInitConsoleMode();
	}

#ifdef _DEBUG
	OutputDebugString(gpszRunCmd); OutputDebugString(L"\n");
#endif
	UNREFERENCED_PARAMETER(pwszStartCmdLine);
	_ASSERTE(pwszStartCmdLine==asCmdLine);
	return 0;
}

//int NextArg(LPCWSTR &asCmdLine, wchar_t* rsArg/*[MAX_PATH+1]*/)
//{
//    LPCWSTR psCmdLine = asCmdLine, pch = NULL;
//    wchar_t ch = *psCmdLine;
//    int nArgLen = 0;
//
//    while (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n') ch = *(++psCmdLine);
//    if (ch == 0) return CERR_CMDLINEEMPTY;
//
//    // аргумент начинается с "
//    if (ch == L'"') {
//        psCmdLine++;
//        pch = wcschr(psCmdLine, L'"');
//        if (!pch) return CERR_CMDLINE;
//        while (pch[1] == L'"') {
//            pch += 2;
//            pch = wcschr(pch, L'"');
//            if (!pch) return CERR_CMDLINE;
//        }
//        // Теперь в pch ссылка на последнюю "
//    } else {
//        // До конца строки или до первого пробела
//        //pch = wcschr(psCmdLine, L' ');
//        // 09.06.2009 Maks - обломался на: cmd /c" echo Y "
//        pch = psCmdLine;
//        while (*pch && *pch!=L' ' && *pch!=L'"') pch++;
//        //if (!pch) pch = psCmdLine + lstrlen(psCmdLine); // до конца строки
//    }
//
//    nArgLen = pch - psCmdLine;
//    if (nArgLen > MAX_PATH) return CERR_CMDLINE;
//
//    // Вернуть аргумент
//    memcpy(rsArg, psCmdLine, nArgLen*sizeof(wchar_t));
//    rsArg[nArgLen] = 0;
//
//    psCmdLine = pch;
//
//    // Finalize
//    ch = *psCmdLine; // может указывать на закрывающую кавычку
//    if (ch == L'"') ch = *(++psCmdLine);
//    while (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n') ch = *(++psCmdLine);
//    asCmdLine = psCmdLine;
//
//    return 0;
//}

//void CorrectConsolePos()
//{
//	RECT rcNew = {};
//	if (GetWindowRect(ghConWnd, &rcNew))
//	{
//		HMONITOR hMon = MonitorFromWindow(ghConWnd, MONITOR_DEFAULTTOPRIMARY);
//		MONITORINFO mi = {sizeof(mi)};
//		int nMaxX = 0, nMaxY = 0;
//		if (GetMonitorInfo(hMon, &mi))
//		{
//			int newW = (rcNew.right-rcNew.left), newH = (rcNew.bottom-rcNew.top);
//			int newX = rcNew.left, newY = rcNew.top;
//
//			if (newX < mi.rcWork.left)
//				newX = mi.rcWork.left;
//			else if (rcNew.right > mi.rcWork.right)
//				newX = max(mi.rcWork.left,(mi.rcWork.right-newW));
//
//			if (newY < mi.rcWork.top)
//				newY = mi.rcWork.top;
//			else if (rcNew.bottom > mi.rcWork.bottom)
//				newY = max(mi.rcWork.top,(mi.rcWork.bottom-newH));
//
//			if ((newX != rcNew.left) || (newY != rcNew.top))
//				SetWindowPos(ghConWnd, HWND_TOP, newX, newY,0,0, SWP_NOSIZE);
//		}
//	}
//}
//
//void EmergencyShow()
//{
//	if (ghConWnd)
//	{
//		CorrectConsolePos();
//
//		if (!IsWindowVisible(ghConWnd))
//		{
//			SetWindowPos(ghConWnd, HWND_NOTOPMOST, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE);
//			//SetWindowPos(ghConWnd, HWND_TOP, 50,50,0,0, SWP_NOSIZE);
//			apiShowWindowAsync(ghConWnd, SW_SHOWNORMAL);
//		}
//		else
//		{
//			// Снять TOPMOST
//			SetWindowPos(ghConWnd, HWND_NOTOPMOST, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE);
//		}
//
//		if (!IsWindowEnabled(ghConWnd))
//			EnableWindow(ghConWnd, true);
//	}
//}

// Проверить, что nPID это "ConEmuC.exe" или "ConEmuC64.exe"
bool IsMainServerPID(DWORD nPID)
{
	PROCESSENTRY32 Info;
	if (!GetProcessInfo(nPID, &Info))
		return false;
	if ((lstrcmpi(Info.szExeFile, L"ConEmuC.exe") == 0)
		|| (lstrcmpi(Info.szExeFile, L"ConEmuC64.exe") == 0))
	{
		return true;
	}
	return false;
}

int ExitWaitForKey(WORD* pvkKeys, LPCWSTR asConfirm, BOOL abNewLine, BOOL abDontShowConsole)
{
	int nKeyPressed = -1;

	// Чтобы ошибку было нормально видно
	if (!abDontShowConsole)
	{
		BOOL lbNeedVisible = FALSE;

		if (!ghConWnd) ghConWnd = GetConEmuHWND(2);

		if (ghConWnd)  // Если консоль была скрыта
		{
			WARNING("Если GUI жив - отвечает на запросы SendMessageTimeout - показывать консоль не нужно. Не красиво получается");

			if (!IsWindowVisible(ghConWnd))
			{
				BOOL lbGuiAlive = FALSE;

				if (ghConEmuWnd && IsWindow(ghConEmuWnd))
				{
					DWORD_PTR dwLRc = 0;

					if (SendMessageTimeout(ghConEmuWnd, WM_NULL, 0, 0, SMTO_ABORTIFHUNG, 1000, &dwLRc))
						lbGuiAlive = TRUE;
				}

				if (!lbGuiAlive && !IsWindowVisible(ghConWnd))
				{
					lbNeedVisible = TRUE;
					// не надо наверное... // поставить "стандартный" 80x25, или то, что было передано к ком.строке
					//SMALL_RECT rcNil = {0}; SetConsoleSize(0, gcrVisibleSize, rcNil, ":Exiting");
					//SetConsoleFontSizeTo(ghConWnd, 8, 12); // установим шрифт побольше
					//apiShowWindow(ghConWnd, SW_SHOWNORMAL); // и покажем окошко
					EmergencyShow(ghConWnd);
				}
			}
		}
	}

	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

	// Сначала почистить буфер
	INPUT_RECORD r = {0}; DWORD dwCount = 0;
	DWORD nPreFlush = 0, nPostFlush = 0, nPreQuit = 0;

	GetNumberOfConsoleInputEvents(hIn, &nPreFlush);

	FlushConsoleInputBuffer(hIn);

	PRINT_COMSPEC(L"Finalizing. gbInShutdown=%i\n", gbInShutdown);
	GetNumberOfConsoleInputEvents(hIn, &nPostFlush);

	if (gbInShutdown)
		goto wrap; // Event закрытия мог припоздниться

	SetConsoleMode(hOut, ENABLE_PROCESSED_OUTPUT|ENABLE_WRAP_AT_EOL_OUTPUT);

	//
	_wprintf(asConfirm);

	//if (lbNeedVisible)
	// Если окошко опять было скрыто - значит GUI часть жива, и опять отображаться не нужно
	//while (PeekConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &r, 1, &dwCount)) {
	//    if (dwCount)
	//        ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &r, 1, &dwCount);
	//    else
	//        Sleep(100);
	//    if (lbNeedVisible && !IsWindowVisible(ghConWnd)) {
	//        apiShowWindow(ghConWnd, SW_SHOWNORMAL); // и покажем окошко
	//    }
	while (TRUE)
	{
		if (gbStopExitWaitForKey)
		{
			// Был вызван HandlerRoutine(CLOSE)
			break;
		}

		if (!PeekConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &r, 1, &dwCount))
			dwCount = 0;

		if (!gbInExitWaitForKey)
		{
			if (gnRunMode == RM_SERVER)
			{
				int nCount = gpSrv->nProcessCount;

				if (nCount > 1)
				{
					// Теперь Peek, так что просто выходим
					//// ! Процесс таки запустился, закрываться не будем. Вернуть событие в буфер!
					//WriteConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &r, 1, &dwCount);
					break;
				}
			}

			if (gbInShutdown)
			{
				break;
			}
		}

		if (dwCount)
		{
			GetNumberOfConsoleInputEvents(hIn, &nPreQuit);

			if (ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &r, 1, &dwCount) && dwCount)
			{
				bool lbMatch = false;

				if (r.EventType == KEY_EVENT && r.Event.KeyEvent.bKeyDown)
				{
					if (pvkKeys)
					{
						for (int i = 0; !lbMatch && pvkKeys[i]; i++)
						{
							lbMatch = (r.Event.KeyEvent.wVirtualKeyCode == pvkKeys[i]);
						}
					}
					else
					{
						lbMatch = (r.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE);
					}
				}

				if (lbMatch)
				{
					nKeyPressed = r.Event.KeyEvent.wVirtualKeyCode;
					break;
				}
			}
		}

		Sleep(50);
	}

	//MessageBox(0,L"Debug message...............1",L"ConEmuC",0);
	//int nCh = _getch();
	if (abNewLine)
		_printf("\n");

wrap:
	#if defined(_DEBUG) && defined(SHOW_EXITWAITKEY_MSGBOX)
	wchar_t szTitle[128];
	_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuC[Srv]: PID=%u", GetCurrentProcessId());
	MessageBox(NULL, asConfirm ? asConfirm : L"???", szTitle, MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
	#endif
	return nKeyPressed;
}



// Используется в режиме RM_APPLICATION, чтобы не тормозить основной поток (жалобы на замедление запуска программ из батников)
DWORD gnSendStartedThread = 0;
HANDLE ghSendStartedThread = NULL;
DWORD WINAPI SendStartedThreadProc(LPVOID lpParameter)
{
	_ASSERTE(gnMainServerPID!=0 && gnMainServerPID!=GetCurrentProcessId() && "Main server PID must be determined!");

	CESERVER_REQ *pIn = (CESERVER_REQ*)lpParameter;
	_ASSERTE(pIn && (pIn->hdr.cbSize>=sizeof(pIn->hdr)) && (pIn->hdr.nCmd==CECMD_CMDSTARTSTOP));

	// Сам результат не интересует
	CESERVER_REQ *pSrvOut = ExecuteSrvCmd(gnMainServerPID, pIn, ghConWnd, TRUE/*async*/);

	ExecuteFreeResult(pSrvOut);
	ExecuteFreeResult(pIn);

	return 0;
}


void SetTerminateEvent(SetTerminateEventPlace eFrom)
{
	#ifdef DEBUG_ISSUE_623
	_ASSERTE((eFrom == ste_ProcessCountChanged) && "Calling SetTerminateEvent");
	#endif

	if (!gTerminateEventPlace)
		gTerminateEventPlace = eFrom;
	SetEvent(ghExitQueryEvent);
}


void SendStarted()
{
	WARNING("Подозрение, что слишком много вызовов при старте сервера. Неаккуратно");

	static bool bSent = false;

	if (bSent)
		return; // отсылать только один раз

	//crNewSize = gpSrv->sbi.dwSize;
	//_ASSERTE(crNewSize.X>=MIN_CON_WIDTH && crNewSize.Y>=MIN_CON_HEIGHT);
	HWND hConWnd = GetConEmuHWND(2);

	if (!gnSelfPID)
	{
		_ASSERTE(gnSelfPID!=0);
		gnSelfPID = GetCurrentProcessId();
	}

	if (!hConWnd)
	{
		// Это Detached консоль. Скорее всего запущен вместо COMSPEC
		_ASSERTE(gnRunMode == RM_COMSPEC);
		gbNonGuiMode = TRUE; // Не посылать ExecuteGuiCmd при выходе. Это не наша консоль
		return;
	}

	_ASSERTE(hConWnd == ghConWnd);
	ghConWnd = hConWnd;

	DWORD nMainServerPID = 0, nAltServerPID = 0, nGuiPID = 0;

	// Для ComSpec-а сразу можно проверить, а есть-ли сервер в этой консоли...
	if (gnRunMode /*== RM_COMSPEC*/ > RM_SERVER)
	{
		MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> ConsoleMap;
		ConsoleMap.InitName(CECONMAPNAME, (DWORD)hConWnd); //-V205
		const CESERVER_CONSOLE_MAPPING_HDR* pConsoleInfo = ConsoleMap.Open();

		//WCHAR sHeaderMapName[64];
		//StringCchPrintf(sHeaderMapName, countof(sHeaderMapName), CECONMAPNAME, (DWORD)hConWnd);
		//HANDLE hFileMapping = OpenFileMapping(FILE_MAP_READ/*|FILE_MAP_WRITE*/, FALSE, sHeaderMapName);
		//if (hFileMapping) {
		//	const CESERVER_CONSOLE_MAPPING_HDR* pConsoleInfo
		//		= (CESERVER_CONSOLE_MAPPING_HDR*)MapViewOfFile(hFileMapping, FILE_MAP_READ/*|FILE_MAP_WRITE*/,0,0,0);
		if (pConsoleInfo)
		{
			nMainServerPID = pConsoleInfo->nServerPID;
			nAltServerPID = pConsoleInfo->nAltServerPID;
			nGuiPID = pConsoleInfo->nGuiPID;

			if (pConsoleInfo->cbSize >= sizeof(CESERVER_CONSOLE_MAPPING_HDR))
			{
				if (pConsoleInfo->nLogLevel)
					CreateLogSizeFile(pConsoleInfo->nLogLevel);
			}

			//UnmapViewOfFile(pConsoleInfo);
			ConsoleMap.CloseMap();
		}

		//	CloseHandle(hFileMapping);
		//}

		if (nMainServerPID == 0)
		{
			gbNonGuiMode = TRUE; // Не посылать ExecuteGuiCmd при выходе. Это не наша консоль
			return; // Режим ComSpec, но сервера нет, соответственно, в GUI ничего посылать не нужно
		}
	}
	else
	{
		nGuiPID = gpSrv->dwGuiPID;
	}

	CESERVER_REQ *pIn = NULL, *pOut = NULL, *pSrvOut = NULL;
	int nSize = sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_STARTSTOP);
	pIn = ExecuteNewCmd(CECMD_CMDSTARTSTOP, nSize);

	if (pIn)
	{
		if (!GetModuleFileName(NULL, pIn->StartStop.sModuleName, countof(pIn->StartStop.sModuleName)))
			pIn->StartStop.sModuleName[0] = 0;
		#ifdef _DEBUG
		LPCWSTR pszFileName = PointToName(pIn->StartStop.sModuleName);
		#endif

		// Cmd/Srv режим начат
		switch (gnRunMode)
		{
		case RM_SERVER:
			pIn->StartStop.nStarted = sst_ServerStart;
			IsKeyboardLayoutChanged(&pIn->StartStop.dwKeybLayout);
			break;
		case RM_ALTSERVER:
			pIn->StartStop.nStarted = sst_AltServerStart;
			IsKeyboardLayoutChanged(&pIn->StartStop.dwKeybLayout);
			break;
		case RM_COMSPEC:
			pIn->StartStop.nParentFarPID = gpSrv->dwParentFarPID;
			pIn->StartStop.nStarted = sst_ComspecStart;
			break;
		default:
			pIn->StartStop.nStarted = sst_AppStart;
		}
		pIn->StartStop.hWnd = ghConWnd;
		pIn->StartStop.dwPID = gnSelfPID;
		#ifdef _WIN64
		pIn->StartStop.nImageBits = 64;
		#else
		pIn->StartStop.nImageBits = 32;
		#endif
		TODO("Ntvdm/DosBox -> 16");

		//pIn->StartStop.dwInputTID = (gnRunMode == RM_SERVER) ? gpSrv->dwInputThreadId : 0;
		if ((gnRunMode == RM_SERVER) || (gnRunMode == RM_ALTSERVER))
			pIn->StartStop.bUserIsAdmin = IsUserAdmin();

		// Перед запуском 16бит приложений нужно подресайзить консоль...
		gnImageSubsystem = 0;
		LPCWSTR pszTemp = gpszRunCmd;
		wchar_t lsRoot[MAX_PATH+1] = {0};

		if (gnRunMode == RM_SERVER && gpSrv->DbgInfo.bDebuggerActive)
		{
			// "Отладчик"
			gnImageSubsystem = 0x101;
			gbRootIsCmdExe = TRUE; // Чтобы буфер появился
		}
		else if (/*!gpszRunCmd &&*/ gbAttachFromFar)
		{
			// Аттач из фар-плагина
			gnImageSubsystem = 0x100;
		}
		else if (gpszRunCmd && ((0 == NextArg(&pszTemp, lsRoot))))
		{
			PRINT_COMSPEC(L"Starting: <%s>", lsRoot);

			MWow64Disable wow;
			if (!gbSkipWowChange) wow.Disable();
			
			DWORD nImageFileAttr = 0;
			if (!GetImageSubsystem(lsRoot, gnImageSubsystem, gnImageBits, nImageFileAttr))
				gnImageSubsystem = 0;

			PRINT_COMSPEC(L", Subsystem: <%i>\n", gnImageSubsystem);
			PRINT_COMSPEC(L"  Args: %s\n", pszTemp);
		}
		else
		{
			GetImageSubsystem(gnImageSubsystem, gnImageBits);
		}

		pIn->StartStop.nSubSystem = gnImageSubsystem;
		if ((gnImageSubsystem == IMAGE_SUBSYSTEM_DOS_EXECUTABLE) || (gbUseDosBox))
			pIn->StartStop.nImageBits = 16;
		else if (gnImageBits)
			pIn->StartStop.nImageBits = gnImageBits;

		pIn->StartStop.bRootIsCmdExe = gbRootIsCmdExe; //2009-09-14

		// НЕ MyGet..., а то можем заблокироваться...
		DWORD dwErr1 = 0;
		BOOL lbRc1;

		{
			HANDLE hOut;
			// Need to block all requests to output buffer in other threads
			MSectionLockSimple csRead; csRead.Lock(&gpSrv->csReadConsoleInfo, LOCK_READOUTPUT_TIMEOUT);

			if ((gnRunMode == RM_SERVER) || (gnRunMode == RM_ALTSERVER))
				hOut = (HANDLE)ghConOut;
			else
				hOut = GetStdHandle(STD_OUTPUT_HANDLE);

			lbRc1 = GetConsoleScreenBufferInfo(hOut, &pIn->StartStop.sbi);

			if (!lbRc1) dwErr1 = GetLastError();

			pIn->StartStop.crMaxSize = MyGetLargestConsoleWindowSize(hOut);
		}

		// Если (для ComSpec) указан параметр "-cur_console:h<N>"
		if (gbParmBufSize)
		{
			pIn->StartStop.bForceBufferHeight = TRUE;
			TODO("gcrBufferSize - и ширину буфера");
			pIn->StartStop.nForceBufferHeight = gnBufferHeight;
		}

		PRINT_COMSPEC(L"Starting %s mode (ExecuteGuiCmd started)\n", (RunMode==RM_SERVER) ? L"Server" : (RunMode==RM_ALTSERVER) ? L"AltServer" : L"ComSpec");

		// CECMD_CMDSTARTSTOP
		if (gnRunMode == RM_SERVER)
		{
			_ASSERTE(nGuiPID!=0 && gnRunMode==RM_SERVER);
			pIn->StartStop.hServerProcessHandle = (u64)(DWORD_PTR)DuplicateProcessHandle(nGuiPID);

			// послать CECMD_CMDSTARTSTOP/sst_ServerStart в GUI
			pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
		}
		else if (gnRunMode == RM_ALTSERVER)
		{
			// Подготовить хэндл своего процесса для MainServer
			pIn->StartStop.hServerProcessHandle = (u64)(DWORD_PTR)DuplicateProcessHandle(nMainServerPID);

			pSrvOut = ExecuteSrvCmd(nMainServerPID, pIn, ghConWnd);

			// MainServer должен был вернуть PID предыдущего AltServer (если он был)
			if (pSrvOut && (pSrvOut->DataSize() >= sizeof(CESERVER_REQ_STARTSTOPRET)))
			{
				gpSrv->dwPrevAltServerPID = pSrvOut->StartStopRet.dwPrevAltServerPID;
			}
			else
			{
				_ASSERTE(pSrvOut && (pSrvOut->DataSize() >= sizeof(CESERVER_REQ_STARTSTOPRET)) && "StartStopRet.dwPrevAltServerPID expected");
			}
		}
		else if (gnRunMode == RM_APPLICATION)
		{
			if (nMainServerPID == 0)
			{
				_ASSERTE(nMainServerPID && "Main Server must be detected already!");
			}
			else
			{
				// Сразу запомнить в глобальной переменной PID сервера
				gnMainServerPID = nMainServerPID;
				gnAltServerPID = nAltServerPID;
				// чтобы не тормозить основной поток (жалобы на замедление запуска программ из батников)
				ghSendStartedThread = CreateThread(NULL, 0, SendStartedThreadProc, pIn, 0, &gnSendStartedThread);
	  			DWORD nErrCode = ghSendStartedThread ? 0 : GetLastError();
	  			if (ghSendStartedThread == NULL)
	  			{
	  				_ASSERTE(ghSendStartedThread && L"SendStartedThreadProc creation failed!")
					pSrvOut = ExecuteSrvCmd(nMainServerPID, pIn, ghConWnd);
	  			}
	  			else
	  			{
	  				pIn = NULL; // Освободит сама SendStartedThreadProc
	  			}
	  			UNREFERENCED_PARAMETER(nErrCode);
  			}
  			_ASSERTE(pOut == NULL); // нада
		}
		else
		{
			WARNING("TODO: Может быть это тоже в главный сервер посылать?");
			_ASSERTE(nGuiPID!=0 && gnRunMode==RM_COMSPEC);

			pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
			// pOut должен содержать инфу, что должен сделать ComSpec
			// при завершении (вернуть высоту буфера)
			_ASSERTE(pOut!=NULL && "Prev buffer size must be returned!");
		}

		#if 0
		if (nServerPID && (nServerPID != gnSelfPID))
		{
			_ASSERTE(nServerPID!=0 && (gnRunMode==RM_ALTSERVER || gnRunMode==RM_COMSPEC));
			if ((gnRunMode == RM_ALTSERVER) || (gnRunMode == RM_SERVER))
			{
				pIn->StartStop.hServerProcessHandle = (u64)(DWORD_PTR)DuplicateProcessHandle(nServerPID);
			}

			WARNING("Optimize!!!");
			WARNING("Async");
			pSrvOut = ExecuteSrvCmd(nServerPID, pIn, ghConWnd);
			if (gnRunMode == RM_ALTSERVER)
			{
				if (pSrvOut && (pSrvOut->DataSize() >= sizeof(CESERVER_REQ_STARTSTOPRET)))
				{
					gpSrv->dwPrevAltServerPID = pSrvOut->StartStopRet.dwPrevAltServerPID;
				}
				else
				{
					_ASSERTE(pSrvOut && (pSrvOut->DataSize() >= sizeof(CESERVER_REQ_STARTSTOPRET)));
				}
			}
		}
		else
		{
			_ASSERTE(gnRunMode==RM_SERVER && (nServerPID && (nServerPID != gnSelfPID)) && "nServerPID MUST be known already!");
		}
		#endif

		#if 0
		WARNING("Только для RM_SERVER. Все остальные должны докладываться главному серверу, а уж он разберется");
		if (gnRunMode != RM_APPLICATION)
		{
			_ASSERTE(nGuiPID!=0 || gnRunMode==RM_SERVER);
			if ((gnRunMode == RM_ALTSERVER) || (gnRunMode == RM_SERVER))
			{
				pIn->StartStop.hServerProcessHandle = (u64)(DWORD_PTR)DuplicateProcessHandle(nGuiPID);
			}

			WARNING("Optimize!!!");
			pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
		}
		#endif


		
		
		PRINT_COMSPEC(L"Starting %s mode (ExecuteGuiCmd finished)\n",(RunMode==RM_SERVER) ? L"Server" : (RunMode==RM_ALTSERVER) ? L"AltServer" : L"ComSpec");

		if (pOut == NULL)
		{
			if (gnRunMode != RM_COMSPEC)
			{
				// для RM_APPLICATION будет pOut==NULL?
				_ASSERTE(gnRunMode == RM_ALTSERVER);
			}
			else
			{
				gbNonGuiMode = TRUE; // Не посылать ExecuteGuiCmd при выходе. Это не наша консоль
			}
		}
		else
		{
			bSent = true;
			BOOL  bAlreadyBufferHeight = pOut->StartStopRet.bWasBufferHeight;
			DWORD nGuiPID = pOut->StartStopRet.dwPID;
			ghConEmuWnd = pOut->StartStopRet.hWnd;
			SetConEmuWindows(pOut->StartStopRet.hWndDc, pOut->StartStopRet.hWndBack);
			if (gpSrv)
			{
				gpSrv->dwGuiPID = pOut->StartStopRet.dwPID;
				#ifdef _DEBUG
				DWORD dwPID; GetWindowThreadProcessId(ghConEmuWnd, &dwPID);
				_ASSERTE(ghConEmuWnd==NULL || dwPID==gpSrv->dwGuiPID);
				#endif
			}

			if ((gnRunMode == RM_SERVER) || (gnRunMode == RM_ALTSERVER))
			{
				if (gpSrv)
				{
					gpSrv->bWasDetached = FALSE;
				}
				else
				{
					_ASSERTE(gpSrv!=NULL);
				}
			}

			UpdateConsoleMapHeader();

			_ASSERTE(gnMainServerPID==0 || gnMainServerPID==pOut->StartStopRet.dwMainSrvPID || (gbAttachMode && gbAlienMode && (pOut->StartStopRet.dwMainSrvPID==gnSelfPID)));
			gnMainServerPID = pOut->StartStopRet.dwMainSrvPID;
			gnAltServerPID = pOut->StartStopRet.dwAltSrvPID;

			AllowSetForegroundWindow(nGuiPID);
			TODO("gnBufferHeight->gcrBufferSize");
			TODO("gcrBufferSize - и ширину буфера");
			gnBufferHeight  = (SHORT)pOut->StartStopRet.nBufferHeight;
			gbParmBufSize = TRUE;
			// gcrBufferSize переименован в gcrVisibleSize
			_ASSERTE(pOut->StartStopRet.nWidth && pOut->StartStopRet.nHeight);
			gcrVisibleSize.X = (SHORT)pOut->StartStopRet.nWidth;
			gcrVisibleSize.Y = (SHORT)pOut->StartStopRet.nHeight;
			gbParmVisibleSize = TRUE;			

			if ((gnRunMode == RM_SERVER) || (gnRunMode == RM_ALTSERVER))
			{
				// Если режим отладчика - принудительно включить прокрутку
				if (gpSrv->DbgInfo.bDebuggerActive && !gnBufferHeight)
				{
					_ASSERTE(gnRunMode != RM_ALTSERVER);
					gnBufferHeight = 9999;
				}

				SMALL_RECT rcNil = {0};
				SetConsoleSize(gnBufferHeight, gcrVisibleSize, rcNil, "::SendStarted");

				// Смена раскладки клавиатуры
				if ((gnRunMode != RM_ALTSERVER) && pOut->StartStopRet.bNeedLangChange)
				{
					#ifndef INPUTLANGCHANGE_SYSCHARSET
					#define INPUTLANGCHANGE_SYSCHARSET 0x0001
					#endif

					WPARAM wParam = INPUTLANGCHANGE_SYSCHARSET;
					TODO("Проверить на x64, не будет ли проблем с 0xFFFFFFFFFFFFFFFFFFFFF");
					LPARAM lParam = (LPARAM)(DWORD_PTR)pOut->StartStopRet.NewConsoleLang;
					SendMessage(ghConWnd, WM_INPUTLANGCHANGEREQUEST, wParam, lParam);
				}
			}
			else
			{
				// Может так получиться, что один COMSPEC запущен из другого.
				// 100628 - неактуально. COMSPEC сбрасывается в cmd.exe
				//if (bAlreadyBufferHeight)
				//	gpSrv->bNonGuiMode = TRUE; // Не посылать ExecuteGuiCmd при выходе - прокрутка должна остаться
				gbWasBufferHeight = bAlreadyBufferHeight;
			}

			//nNewBufferHeight = ((DWORD*)(pOut->Data))[0];
			//crNewSize.X = (SHORT)((DWORD*)(pOut->Data))[1];
			//crNewSize.Y = (SHORT)((DWORD*)(pOut->Data))[2];
			TODO("Если он запущен как COMSPEC - то к GUI никакого отношения иметь не должен");
			//if (rNewWindow.Right >= crNewSize.X) // размер был уменьшен за счет полосы прокрутки
			//    rNewWindow.Right = crNewSize.X-1;
			ExecuteFreeResult(pOut); //pOut = NULL;
			//gnBufferHeight = nNewBufferHeight;

		} // (pOut != NULL)

		ExecuteFreeResult(pIn);
		ExecuteFreeResult(pSrvOut);
	}
}

CESERVER_REQ* SendStopped(CONSOLE_SCREEN_BUFFER_INFO* psbi)
{
	int iHookRc = -1;
	if (gnRunMode == RM_ALTSERVER)
	{
		// сообщение о завершении будет посылать ConEmuHk.dll
		HMODULE hHooks = GetModuleHandle(WIN3264TEST(L"ConEmuHk.dll",L"ConEmuHk64.dll"));
		RequestLocalServer_t fRequestLocalServer = (RequestLocalServer_t)(hHooks ? GetProcAddress(hHooks, "RequestLocalServer") : NULL);
		if (fRequestLocalServer)
		{
			RequestLocalServerParm Parm = {sizeof(Parm), slsf_AltServerStopped};
			iHookRc = fRequestLocalServer(&Parm);
		}
		_ASSERTE((iHookRc == 0) && "SendStopped must be sent from ConEmuHk.dll");
		return NULL;
	}

	if (ghSendStartedThread)
	{
		_ASSERTE(gnRunMode!=RM_COMSPEC);
		// Чуть-чуть подождать, может все-таки успеет?
		DWORD nWait = WaitForSingleObject(ghSendStartedThread, 50);
		if (nWait == WAIT_TIMEOUT)
		{
			#ifndef __GNUC__
			#pragma warning( push )
			#pragma warning( disable : 6258 )
			#endif
			TerminateThread(ghSendStartedThread, 100);    // раз корректно не хочет...
			#ifndef __GNUC__
			#pragma warning( pop )
			#endif
		}

		HANDLE h = ghSendStartedThread; ghSendStartedThread = NULL;
		CloseHandle(h);
	}
	else
	{
		_ASSERTE(gnRunMode!=RM_APPLICATION);
	}

	CESERVER_REQ *pIn = NULL, *pOut = NULL;
	int nSize = sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_STARTSTOP);
	pIn = ExecuteNewCmd(CECMD_CMDSTARTSTOP,nSize);

	if (pIn)
	{
		switch (gnRunMode)
		{
		case RM_SERVER:
			// По идее, sst_ServerStop не посылается
			_ASSERTE(gnRunMode != RM_SERVER);
			pIn->StartStop.nStarted = sst_ServerStop;
			break;
		case RM_ALTSERVER:
			pIn->StartStop.nStarted = sst_AltServerStop;
			break;
		case RM_COMSPEC:
			pIn->StartStop.nStarted = sst_ComspecStop;
			pIn->StartStop.nOtherPID = gpSrv->dwRootProcess;
			pIn->StartStop.nParentFarPID = gpSrv->dwParentFarPID;
			break;
		default:
			pIn->StartStop.nStarted = sst_AppStop;
		}

		if (!GetModuleFileName(NULL, pIn->StartStop.sModuleName, countof(pIn->StartStop.sModuleName)))
			pIn->StartStop.sModuleName[0] = 0;

		pIn->StartStop.hWnd = ghConWnd;
		pIn->StartStop.dwPID = gnSelfPID;
		pIn->StartStop.nSubSystem = gnImageSubsystem;
		if ((gnImageSubsystem == IMAGE_SUBSYSTEM_DOS_EXECUTABLE) || (gbUseDosBox))
			pIn->StartStop.nImageBits = 16;
		else if (gnImageBits)
			pIn->StartStop.nImageBits = gnImageBits;
		else
		{
			#ifdef _WIN64
			pIn->StartStop.nImageBits = 64;
			#else
			pIn->StartStop.nImageBits = 32;
			#endif
		}
		pIn->StartStop.bWasBufferHeight = gbWasBufferHeight;

		if (psbi != NULL)
		{
			pIn->StartStop.sbi = *psbi;
		}
		else
		{
			// НЕ MyGet..., а то можем заблокироваться...
			// ghConOut может быть NULL, если ошибка произошла во время разбора аргументов
			GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &pIn->StartStop.sbi);
		}

		pIn->StartStop.crMaxSize = MyGetLargestConsoleWindowSize(GetStdHandle(STD_OUTPUT_HANDLE));

		if ((gnRunMode == RM_APPLICATION) || (gnRunMode == RM_ALTSERVER))
		{
			if (gnMainServerPID == 0)
			{
				_ASSERTE(gnMainServerPID!=0 && "MainServer PID must be determined");
			}
			else
			{
				pIn->StartStop.nOtherPID = (gnRunMode == RM_ALTSERVER) ? gpSrv->dwPrevAltServerPID : 0;
				pOut = ExecuteSrvCmd(gnMainServerPID, pIn, ghConWnd);
			}
		}
		else
		{
			PRINT_COMSPEC(L"Finalizing comspec mode (ExecuteGuiCmd started)\n",0);
			WARNING("Это надо бы совместить, но пока - нужно сначала передернуть главный сервер!");
			pOut = ExecuteSrvCmd(gnMainServerPID, pIn, ghConWnd);
			_ASSERTE(pOut!=NULL);
			ExecuteFreeResult(pOut);
			pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
			PRINT_COMSPEC(L"Finalizing comspec mode (ExecuteGuiCmd finished)\n",0);
		}

		ExecuteFreeResult(pIn); pIn = NULL;
	}

	return pOut;
}


WARNING("Добавить LogInput(INPUT_RECORD* pRec) но имя файла сделать 'ConEmuC-input-%i.log'");
void CreateLogSizeFile(int nLevel)
{
	if (gpLogSize) return;  // уже

	DWORD dwErr = 0;
	wchar_t szFile[MAX_PATH+64], *pszDot, *pszName, *pszDir = NULL;
	wchar_t szDir[MAX_PATH+16];

	if (!GetModuleFileName(NULL, szFile, MAX_PATH))
	{
		dwErr = GetLastError();
		_printf("GetModuleFileName failed! ErrCode=0x%08X\n", dwErr);
		return; // не удалось
	}

	pszName = (wchar_t*)PointToName(szFile);

	if ((pszDot = wcsrchr(pszName, L'.')) == NULL)
	{
		_printf("wcsrchr failed!\n", 0, szFile); //-V576
		return; // ошибка
	}

	if (pszName > szFile)
	{
		*(pszName-1) = 0;
		wcscpy_c(szDir, szFile);
		pszDir = wcsrchr(szDir, L'\\');
		if (!pszDir || (lstrcmpi(pszDir, L"\\ConEmu") != 0))
		{
			// Если ConEmuC.exe лежит НЕ в папке "ConEmu"
			pszDir = szDir; // писать в текущую папку или на десктоп
		}
		else
		{
			// Иначе - попытаться найти соответствующий файл GUI
			wcscat_c(szDir, WIN3264TEST(L"\\ConEmu.exe", L"\\ConEmu64.exe"));
			if (FileExists(szDir))
			{
				pszDir = szFile; // GUI лежит в той же папке, что и "сервер"
			}
			else
			{
				// На уровень выше?
				*pszDir = 0;
				wcscat_c(szDir, WIN3264TEST(L"\\ConEmu.exe", L"\\ConEmu64.exe"));
				if (FileExists(szDir))
				{
					*pszDir = 0;
					pszDir = szDir; // GUI лежит в родительской папке
				}
				else
				{
					pszDir = szFile; // GUI не нашли
				}
			}
		}
	}

	_wcscpy_c(pszDot, 16, L"-size");
	gpLogSize = new MFileLog(pszName, pszDir, GetCurrentProcessId());

	if (!gpLogSize)
		return;

	dwErr = (DWORD)gpLogSize->CreateLogFile();
	if (dwErr != 0)
	{
		_printf("Create console log file failed! ErrCode=0x%08X\n", dwErr, gpLogSize->GetLogFileName()); //-V576
		SafeDelete(gpLogSize);
		return;
	}

	gpLogSize->LogStartEnv(gpStartEnv);

	LogSize(NULL, "Startup");
}

void LogString(LPCSTR asText)
{
	if (!gpLogSize) return;

	char szInfo[255]; szInfo[0] = 0;
	LPCSTR pszThread = " <unknown thread>";
	DWORD dwId = GetCurrentThreadId();

	if (dwId == gdwMainThreadId)
		pszThread = " MainThread";
	else if (gpSrv->CmdServer.IsPipeThread(dwId))
		pszThread = " ServerThread";
	else if (dwId == gpSrv->dwRefreshThread)
		pszThread = " RefreshThread";
	//#ifdef USE_WINEVENT_SRV
	//else if (dwId == gpSrv->dwWinEventThread)
	//	pszThread = " WinEventThread";
	//#endif
	else if (gpSrv->InputServer.IsPipeThread(dwId))
		pszThread = " InputPipeThread";
	else if (gpSrv->DataServer.IsPipeThread(dwId))
		pszThread = " DataPipeThread";

	SYSTEMTIME st; GetLocalTime(&st);
	_wsprintfA(szInfo, SKIPLEN(countof(szInfo)) "%i:%02i:%02i.%03i ",
	           st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	int nCur = lstrlenA(szInfo);
	lstrcpynA(szInfo+nCur, asText ? asText : "", 255-nCur-3);
	//StringCchCatA(szInfo, countof(szInfo), "\r\n");
	gpLogSize->LogString(szInfo, false, pszThread);
}

void LogSize(COORD* pcrSize, LPCSTR pszLabel)
{
	if (!gpLogSize) return;

	CONSOLE_SCREEN_BUFFER_INFO lsbi = {{0,0}};
	// В дебажный лог помещаем реальный значения
	GetConsoleScreenBufferInfo(ghConOut ? ghConOut : GetStdHandle(STD_OUTPUT_HANDLE), &lsbi);
	char szInfo[192]; szInfo[0] = 0;
	LPCSTR pszThread = "<unknown thread>";
	DWORD dwId = GetCurrentThreadId();

	if (dwId == gdwMainThreadId)
		pszThread = "MainThread";
	else if (gpSrv->CmdServer.IsPipeThread(dwId))
		pszThread = "ServerThread";
	else if (dwId == gpSrv->dwRefreshThread)
		pszThread = "RefreshThread";
	//#ifdef USE_WINEVENT_SRV
	//else if (dwId == gpSrv->dwWinEventThread)
	//	pszThread = "WinEventThread";
	//#endif
	else if (gpSrv->InputServer.IsPipeThread(dwId))
		pszThread = "InputPipeThread";
	else if (gpSrv->DataServer.IsPipeThread(dwId))
		pszThread = "DataPipeThread";

	/*HDESK hDesk = GetThreadDesktop ( GetCurrentThreadId() );
	HDESK hInp = OpenInputDesktop ( 0, FALSE, GENERIC_READ );*/
	SYSTEMTIME st; GetLocalTime(&st);
	//char szMapSize[32]; szMapSize[0] = 0;
	//if (gpSrv->pConsoleMap->IsValid()) {
	//	StringCchPrintfA(szMapSize, countof(szMapSize), " CurMapSize={%ix%ix%i}",
	//		gpSrv->pConsoleMap->Ptr()->sbi.dwSize.X, gpSrv->pConsoleMap->Ptr()->sbi.dwSize.Y,
	//		gpSrv->pConsoleMap->Ptr()->sbi.srWindow.Bottom-gpSrv->pConsoleMap->Ptr()->sbi.srWindow.Top+1);
	//}

	if (pcrSize)
	{
		_wsprintfA(szInfo, SKIPLEN(countof(szInfo)) "%i:%02i:%02i.%03i CurSize={%ix%i} ChangeTo={%ix%i} %s %s",
		           st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
		           lsbi.dwSize.X, lsbi.dwSize.Y, pcrSize->X, pcrSize->Y, pszThread, (pszLabel ? pszLabel : ""));
	}
	else
	{
		_wsprintfA(szInfo, SKIPLEN(countof(szInfo)) "%i:%02i:%02i.%03i CurSize={%ix%i} %s %s",
		           st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
		           lsbi.dwSize.X, lsbi.dwSize.Y, pszThread, (pszLabel ? pszLabel : ""));
	}

	//if (hInp) CloseDesktop ( hInp );
	DWORD dwLen = 0;
	gpLogSize->LogString(szInfo, false, NULL, true);
}


void ProcessCountChanged(BOOL abChanged, UINT anPrevCount, MSectionLock *pCS)
{
	int nExitPlaceAdd = 2; // 2,3,4,5,6,7,8,9 +(nExitPlaceStep)
	bool bPrevCount2 = (anPrevCount>1);

	// Заблокировать, если этого еще не сделали
	MSectionLock CS;
	if (!pCS)
		CS.Lock(gpSrv->csProc);

#ifdef USE_COMMIT_EVENT
	// Если кто-то регистрировался как "ExtendedConsole.dll"
	// то проверить, а не свалился ли процесс его вызвавший
	if (gpSrv && gpSrv->nExtConsolePID)
	{
		DWORD nExtPID = gpSrv->nExtConsolePID;
		bool bExist = false;
		for (UINT i = 0; i < gpSrv->nProcessCount; i++)
		{
			if (gpSrv->pnProcesses[i] == nExtPID)
			{
				bExist = true; break;
			}
		}
		if (!bExist)
		{
			if (gpSrv->hExtConsoleCommit)
			{
				CloseHandle(gpSrv->hExtConsoleCommit);
				gpSrv->hExtConsoleCommit = NULL;
			}
			gpSrv->nExtConsolePID = 0;
		}
	}
#endif

#ifndef WIN64
	// Найти "ntvdm.exe"
	if (abChanged && !gpSrv->nNtvdmPID && !IsWindows64())
	{
		//BOOL lbFarExists = FALSE, lbTelnetExist = FALSE;

		if (gpSrv->nProcessCount > 1)
		{
			HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);

			if (hSnap != INVALID_HANDLE_VALUE)
			{
				PROCESSENTRY32 prc = {sizeof(PROCESSENTRY32)};

				if (Process32First(hSnap, &prc))
				{
					do
					{
						for (UINT i = 0; i < gpSrv->nProcessCount; i++)
						{
							if (prc.th32ProcessID != gnSelfPID
							        && prc.th32ProcessID == gpSrv->pnProcesses[i])
							{
								//if (IsFarExe(prc.szExeFile))
								//{
								//	lbFarExists = TRUE;
								//	//if (gpSrv->nProcessCount <= 2) // нужно проверить и ntvdm
								//	//	break; // возможно, в консоли еще есть и telnet?
								//}

								//#ifndef WIN64
								//else
								if (!gpSrv->nNtvdmPID && lstrcmpiW(prc.szExeFile, L"ntvdm.exe")==0)
								{
									gpSrv->nNtvdmPID = prc.th32ProcessID;
									break;
								}
								//#endif

								//// 23.04.2010 Maks - telnet нужно определять, т.к. у него проблемы с Ins и курсором
								//else if (lstrcmpiW(prc.szExeFile, L"telnet.exe")==0)
								//{
								//	lbTelnetExist = TRUE;
								//}
							}
						}

						//if (lbFarExists && lbTelnetExist
						//	#ifndef WIN64
						//        && gpSrv->nNtvdmPID
						//	#endif
						//    )
						//{
						//	break; // чтобы условие выхода внятнее было
						//}
					}
					while(Process32Next(hSnap, &prc));
				}

				CloseHandle(hSnap);
			}
		}

		//gpSrv->bTelnetActive = lbTelnetExist;
	}
#endif

	gpSrv->dwProcessLastCheckTick = GetTickCount();

	// Если корневой процесс проработал достаточно (10 сек), значит он живой и gbAlwaysConfirmExit можно сбросить
	// Если gbAutoDisableConfirmExit==FALSE - сброс подтверждение закрытия консоли не выполняется
	if (gbAlwaysConfirmExit  // если уже не сброшен
	        && gbAutoDisableConfirmExit // требуется ли вообще такая проверка (10 сек)
	        && anPrevCount > 1 // если в консоли был зафиксирован запущенный процесс
	        && gpSrv->hRootProcess) // и корневой процесс был вообще запущен
	{
		if ((gpSrv->dwProcessLastCheckTick - gpSrv->nProcessStartTick) > CHECK_ROOTOK_TIMEOUT)
		{
			_ASSERTE(gnConfirmExitParm==0);
			// эта проверка выполняется один раз
			gbAutoDisableConfirmExit = FALSE;
			// 10 сек. прошло, теперь необходимо проверить, а жив ли процесс?
			DWORD dwProcWait = WaitForSingleObject(gpSrv->hRootProcess, 0);

			if (dwProcWait == WAIT_OBJECT_0)
			{
				// Корневой процесс завершен, возможно, была какая-то проблема?
				gbRootAliveLess10sec = TRUE; // корневой процесс проработал менее 10 сек
			}
			else
			{
				// Корневой процесс все еще работает, считаем что все ок и подтверждения закрытия консоли не потребуется
				DisableAutoConfirmExit();
				//gpSrv->nProcessStartTick = GetTickCount() - 2*CHECK_ROOTSTART_TIMEOUT; // менять nProcessStartTick не нужно. проверка только по флажкам
			}
		}
	}

	if (gbRootWasFoundInCon == 0 && gpSrv->nProcessCount > 1 && gpSrv->hRootProcess && gpSrv->dwRootProcess)
	{
		if (WaitForSingleObject(gpSrv->hRootProcess, 0) == WAIT_OBJECT_0)
		{
			gbRootWasFoundInCon = 2; // в консоль процесс не попал, и уже завершился
		}
		else
		{
			for(UINT n = 0; n < gpSrv->nProcessCount; n++)
			{
				if (gpSrv->dwRootProcess == gpSrv->pnProcesses[n])
				{
					// Процесс попал в консоль
					gbRootWasFoundInCon = 1; break;
				}
			}
		}
	}

	// Только для x86. На x64 ntvdm.exe не бывает.
	#ifndef WIN64
	WARNING("gpSrv->bNtvdmActive нигде не устанавливается");
	if (gpSrv->nProcessCount == 2 && !gpSrv->bNtvdmActive && gpSrv->nNtvdmPID)
	{
		// Возможно было запущено 16битное приложение, а ntvdm.exe не выгружается при его закрытии
		// gnSelfPID не обязательно будет в gpSrv->pnProcesses[0]
		if ((gpSrv->pnProcesses[0] == gnSelfPID && gpSrv->pnProcesses[1] == gpSrv->nNtvdmPID)
		        || (gpSrv->pnProcesses[1] == gnSelfPID && gpSrv->pnProcesses[0] == gpSrv->nNtvdmPID))
		{
			// Послать в нашу консоль команду закрытия
			PostMessage(ghConWnd, WM_CLOSE, 0, 0);
		}
	}
	#endif

	WARNING("Если в консоли ДО этого были процессы - все условия вида 'gpSrv->nProcessCount == 1' обломаются");

	bool bForcedTo2 = false;
	DWORD nWaitDbg1 = -1, nWaitDbg2 = -1;
	// Похоже "пример" не соответствует условию, оставлю пока, для истории
	// -- Пример - запускаемся из фара. Количество процессов ИЗНАЧАЛЬНО - 5
	// -- cmd вываливается сразу (path not found)
	// -- количество процессов ОСТАЕТСЯ 5 и ни одно из ниже условий не проходит
	if (anPrevCount == 1 && gpSrv->nProcessCount == 1
		&& gpSrv->nProcessStartTick && gpSrv->dwProcessLastCheckTick
		&& ((gpSrv->dwProcessLastCheckTick - gpSrv->nProcessStartTick) > CHECK_ROOTSTART_TIMEOUT)
		&& (nWaitDbg1 = WaitForSingleObject(ghExitQueryEvent,0)) == WAIT_TIMEOUT
		// выходить можно только если корневой процесс завершился
		&& gpSrv->hRootProcess && ((nWaitDbg2 = WaitForSingleObject(gpSrv->hRootProcess,0)) != WAIT_TIMEOUT))
	{
		anPrevCount = 2; // чтобы сработало следующее условие
		bForcedTo2 = true;
		//2010-03-06 - установка флажка должна быть при старте сервера
		//if (!gbAlwaysConfirmExit) gbAlwaysConfirmExit = TRUE; // чтобы консоль не схлопнулась
	}

	if (anPrevCount > 1 && gpSrv->nProcessCount == 1)
	{
		if (gpSrv->pnProcesses[0] != gnSelfPID)
		{
			_ASSERTE(gpSrv->pnProcesses[0] == gnSelfPID);
		}
		else
		{
			#ifdef DEBUG_ISSUE_623
			_ASSERTE(FALSE && "Calling SetTerminateEvent");
			#endif

			// !!! Во время сильной загрузки процессора периодически
			// !!! случается, что ConEmu отваливается быстрее, чем в
			// !!! консоли появится фар. Обратить внимание на nPrevProcessedDbg[]
			// !!! в вызывающей функции. Откуда там появилось 2 процесса,
			// !!! и какого фига теперь стал только 1?
			_ASSERTE(gbTerminateOnCtrlBreak==FALSE);
			// !!! ****


			if (pCS)
				pCS->Unlock();
			else
				CS.Unlock();

			//2010-03-06 это не нужно, проверки делаются по другому
			//if (!gbAlwaysConfirmExit && (gpSrv->dwProcessLastCheckTick - gpSrv->nProcessStartTick) <= CHECK_ROOTSTART_TIMEOUT) {
			//	gbAlwaysConfirmExit = TRUE; // чтобы консоль не схлопнулась
			//}
			if (gbAlwaysConfirmExit && (gpSrv->dwProcessLastCheckTick - gpSrv->nProcessStartTick) <= CHECK_ROOTSTART_TIMEOUT)
				gbRootAliveLess10sec = TRUE; // корневой процесс проработал менее 10 сек

			if (bForcedTo2)
				nExitPlaceAdd = bPrevCount2 ? 3 : 4;
			else if (bPrevCount2)
				nExitPlaceAdd = 5;

			if (!nExitQueryPlace) nExitQueryPlace = nExitPlaceAdd/*2,3,4,5,6,7,8,9*/+(nExitPlaceStep);

			ShutdownSrvStep(L"All processes are terminated, SetEvent(ghExitQueryEvent)");
			SetTerminateEvent(ste_ProcessCountChanged);
		}
	}

	UNREFERENCED_PARAMETER(nWaitDbg1); UNREFERENCED_PARAMETER(nWaitDbg2); UNREFERENCED_PARAMETER(bForcedTo2);
}

BOOL ProcessAdd(DWORD nPID, MSectionLock *pCS /*= NULL*/)
{
	MSectionLock CS;
	if ((pCS == NULL) && (gpSrv->csProc != NULL))
	{
		pCS = &CS;
		CS.Lock(gpSrv->csProc);
	}
	
	UINT nPrevCount = gpSrv->nProcessCount;
	BOOL lbChanged = FALSE;
	_ASSERTE(nPID!=0);
	
	// Добавить процесс в список
	_ASSERTE(gpSrv->pnProcesses[0] == gnSelfPID);
	BOOL lbFound = FALSE;
	for (DWORD n = 0; n < nPrevCount; n++)
	{
		if (gpSrv->pnProcesses[n] == nPID)
		{
			lbFound = TRUE;
			break;
		}
	}
	if (!lbFound)
	{
		if (nPrevCount < gpSrv->nMaxProcesses)
		{
			pCS->RelockExclusive(200);
			gpSrv->pnProcesses[gpSrv->nProcessCount++] = nPID;
			lbChanged = TRUE;
		}
		else
		{
			_ASSERTE(nPrevCount < gpSrv->nMaxProcesses);
		}
	}
	
	return lbChanged;
}

BOOL ProcessRemove(DWORD nPID, UINT nPrevCount, MSectionLock *pCS /*= NULL*/)
{
	BOOL lbChanged = FALSE;

	MSectionLock CS;
	if ((pCS == NULL) && (gpSrv->csProc != NULL))
	{
		pCS = &CS;
		CS.Lock(gpSrv->csProc);
	}

	// Удалить процесс из списка
	_ASSERTE(gpSrv->pnProcesses[0] == gnSelfPID);
	DWORD nChange = 0;
	for (DWORD n = 0; n < nPrevCount; n++)
	{
		if (gpSrv->pnProcesses[n] == nPID)
		{
			pCS->RelockExclusive(200);
			lbChanged = TRUE;
			gpSrv->nProcessCount--;
			continue;
		}
		if (lbChanged)
		{
			gpSrv->pnProcesses[nChange] = gpSrv->pnProcesses[n];
		}
		nChange++;
	}

	return lbChanged;
}	

#ifdef _DEBUG
void DumpProcInfo(LPCWSTR sLabel, DWORD nCount, DWORD* pPID)
{
#ifdef WINE_PRINT_PROC_INFO
	DWORD nErr = GetLastError();
	wchar_t szDbgMsg[255];
	msprintf(szDbgMsg, countof(szDbgMsg), L"%s: Err=%u, Count=%u:", sLabel ? sLabel : L"", nErr, nCount);
	nCount = min(nCount,10);
	for (DWORD i = 0; pPID && (i < nCount); i++)
	{
		int nLen = lstrlen(szDbgMsg);
		msprintf(szDbgMsg+nLen, countof(szDbgMsg)-nLen, L" %u", pPID[i]);
	}
	wcscat_c(szDbgMsg, L"\r\n");
	_wprintf(szDbgMsg);
#endif
}
#define DUMP_PROC_INFO(s,n,p) //DumpProcInfo(s,n,p)
#else
#define DUMP_PROC_INFO(s,n,p)
#endif

BOOL CheckProcessCount(BOOL abForce/*=FALSE*/)
{
	//static DWORD dwLastCheckTick = GetTickCount();
	UINT nPrevCount = gpSrv->nProcessCount;
#ifdef _DEBUG
	DWORD nCurProcessesDbg[128] = {}; // для отладки, получение текущего состояния консоли
	DWORD nPrevProcessedDbg[128] = {}; // для отладки, запомнить предыдущее состояние консоли
	if (gpSrv->pnProcesses && gpSrv->nProcessCount)
		memmove(nPrevProcessedDbg, gpSrv->pnProcesses, min(countof(nPrevProcessedDbg),gpSrv->nProcessCount)*sizeof(*gpSrv->pnProcesses));
#endif

	if (gpSrv->nProcessCount <= 0)
	{
		abForce = TRUE;
	}

	if (!abForce)
	{
		DWORD dwCurTick = GetTickCount();

		if ((dwCurTick - gpSrv->dwProcessLastCheckTick) < (DWORD)CHECK_PROCESSES_TIMEOUT)
			return FALSE;
	}

	BOOL lbChanged = FALSE;
	MSectionLock CS; CS.Lock(gpSrv->csProc);

	if (gpSrv->nProcessCount == 0)
	{
		gpSrv->pnProcesses[0] = gnSelfPID;
		gpSrv->nProcessCount = 1;
	}

	if (gpSrv->DbgInfo.bDebuggerActive)
	{
		//if (gpSrv->hRootProcess) {
		//	if (WaitForSingleObject(gpSrv->hRootProcess, 0) == WAIT_OBJECT_0) {
		//		gpSrv->nProcessCount = 1;
		//		return TRUE;
		//	}
		//}
		//gpSrv->pnProcesses[1] = gpSrv->dwRootProcess;
		//gpSrv->nProcessCount = 2;
		return FALSE;
	}

	#ifdef _DEBUG
	bool bDumpProcInfo = gbIsWine;
	#endif

	bool bProcFound = false;

	if (pfnGetConsoleProcessList && (gpSrv->hRootProcessGui == NULL))
	{
		WARNING("Переделать, как-то слишком сложно получается");
		DWORD nCurCount = 0;
		nCurCount = pfnGetConsoleProcessList(gpSrv->pnProcessesGet, gpSrv->nMaxProcesses);
		#ifdef _DEBUG
		SetLastError(0);
		int nCurCountDbg = pfnGetConsoleProcessList(nCurProcessesDbg, countof(nCurProcessesDbg));
		DUMP_PROC_INFO(L"WinXP mode", nCurCountDbg, nCurProcessesDbg);
		#endif
		lbChanged = (gpSrv->nProcessCount != nCurCount);

		bProcFound = (nCurCount > 0);

		if (nCurCount == 0)
		{
			_ASSERTE(gbTerminateOnCtrlBreak==FALSE);

			// Похоже что сюда мы попадаем также при ошибке 0xC0000142 запуска root-process
			#ifdef _DEBUG
			_ASSERTE(FALSE && "(nCurCount==0)?");
			// Some diagnostics for debugger
			DWORD nErr;
			HWND hConWnd = GetConsoleWindow(); nErr = GetLastError();
			HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE); nErr = GetLastError();
			CONSOLE_SCREEN_BUFFER_INFO sbi = {}; BOOL bSbi = GetConsoleScreenBufferInfo(hOut, &sbi); nErr = GetLastError();
			DWORD nWait = WaitForSingleObject(gpSrv->hRootProcess, 0);
			GetExitCodeProcess(gpSrv->hRootProcess, &nErr);
			#endif

			// Это значит в Win7 свалился conhost.exe
			//if ((gnOsVer >= 0x601) && !gbIsWine)
			{
				#ifdef _DEBUG
				DWORD dwErr = GetLastError();
				#endif
				gpSrv->nProcessCount = 1;
				SetEvent(ghQuitEvent);

				if (!nExitQueryPlace) nExitQueryPlace = 1+(nExitPlaceStep);

				SetTerminateEvent(ste_CheckProcessCount);
				return TRUE;
			}
		}
		else
		{
			if (nCurCount > gpSrv->nMaxProcesses)
			{
				DWORD nSize = nCurCount + 100;
				DWORD* pnPID = (DWORD*)calloc(nSize, sizeof(DWORD));

				if (pnPID)
				{
					CS.RelockExclusive(200);
					nCurCount = pfnGetConsoleProcessList(pnPID, nSize);

					if (nCurCount > 0 && nCurCount <= nSize)
					{
						free(gpSrv->pnProcessesGet);
						gpSrv->pnProcessesGet = pnPID; pnPID = NULL;
						free(gpSrv->pnProcesses);
						gpSrv->pnProcesses = (DWORD*)calloc(nSize, sizeof(DWORD));
						_ASSERTE(nExitQueryPlace == 0 || nCurCount == 1);
						gpSrv->nProcessCount = nCurCount;
						gpSrv->nMaxProcesses = nSize;
					}

					if (pnPID)
						free(pnPID);
				}
			}
			
			// PID-ы процессов в консоли могут оказаться перемешаны. Нас же интересует gnSelfPID на первом месте
			gpSrv->pnProcesses[0] = gnSelfPID;
			DWORD nCorrect = 1, nMax = gpSrv->nMaxProcesses, nDosBox = 0;
			if (gbUseDosBox)
			{
				if (ghDosBoxProcess && WaitForSingleObject(ghDosBoxProcess, 0) == WAIT_TIMEOUT)
				{
					gpSrv->pnProcesses[nCorrect++] = gnDosBoxPID;
					nDosBox = 1;
				}
				else if (ghDosBoxProcess)
				{
					ghDosBoxProcess = NULL;
				}
			}
			for (DWORD n = 0; n < nCurCount && nCorrect < nMax; n++)
			{
				if (gpSrv->pnProcessesGet[n] != gnSelfPID)
				{
					if (gpSrv->pnProcesses[nCorrect] != gpSrv->pnProcessesGet[n])
					{
						gpSrv->pnProcesses[nCorrect] = gpSrv->pnProcessesGet[n];
						lbChanged = TRUE;
					}
					nCorrect++;
				}
			}
			nCurCount += nDosBox;


			if (nCurCount < gpSrv->nMaxProcesses)
			{
				// Сбросить в 0 ячейки со старыми процессами
				_ASSERTE(gpSrv->nProcessCount < gpSrv->nMaxProcesses);

				if (nCurCount < gpSrv->nProcessCount)
				{
					UINT nSize = sizeof(DWORD)*(gpSrv->nProcessCount - nCurCount);
					memset(gpSrv->pnProcesses + nCurCount, 0, nSize);
				}

				_ASSERTE(nCurCount>0);
				_ASSERTE(nExitQueryPlace == 0 || nCurCount == 1);
				gpSrv->nProcessCount = nCurCount;
			}

			if (!lbChanged)
			{
				UINT nSize = sizeof(DWORD)*min(gpSrv->nMaxProcesses,START_MAX_PROCESSES);

				#ifdef _DEBUG
				_ASSERTE(!IsBadWritePtr(gpSrv->pnProcessesCopy,nSize));
				_ASSERTE(!IsBadWritePtr(gpSrv->pnProcesses,nSize));
				#endif

				lbChanged = memcmp(gpSrv->pnProcessesCopy, gpSrv->pnProcesses, nSize) != 0;
				MCHKHEAP;

				if (lbChanged)
					memmove(gpSrv->pnProcessesCopy, gpSrv->pnProcesses, nSize);

				MCHKHEAP;
			}
		}
	}

	// Wine related
	//if (!pfnGetConsoleProcessList || gpSrv->hRootProcessGui)
	if (!bProcFound)
	{
		_ASSERTE(gpSrv->pnProcesses[0] == gnSelfPID);
		gpSrv->pnProcesses[0] = gnSelfPID;

		HANDLE hRootProcess = gpSrv->hRootProcess;

		if (hRootProcess)
		{
			if (WaitForSingleObject(hRootProcess, 0) == WAIT_OBJECT_0)
			{
				gpSrv->pnProcesses[1] = 0;
				lbChanged = gpSrv->nProcessCount != 1;
				gpSrv->nProcessCount = 1;
			}
			else
			{
				gpSrv->pnProcesses[1] = gpSrv->dwRootProcess;
				lbChanged = gpSrv->nProcessCount != 2;
				_ASSERTE(nExitQueryPlace == 0);
				gpSrv->nProcessCount = 2;
			}
		}
		else if (gpSrv->hRootProcessGui)
		{
			if (!IsWindow(gpSrv->hRootProcessGui))
			{
				// Process handle must be opened!
				_ASSERTE(gpSrv->hRootProcess != NULL);
				// Fin
				gpSrv->pnProcesses[1] = 0;
				lbChanged = gpSrv->nProcessCount != 1;
				gpSrv->nProcessCount = 1;
			}
		}

		DUMP_PROC_INFO(L"Win2k mode", gpSrv->nProcessCount, gpSrv->pnProcesses);
	}

	gpSrv->dwProcessLastCheckTick = GetTickCount();

	ProcessCountChanged(lbChanged, nPrevCount, &CS);


	return lbChanged;
}

int CALLBACK FontEnumProc(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, DWORD FontType, LPARAM lParam)
{
	if ((FontType & TRUETYPE_FONTTYPE) == TRUETYPE_FONTTYPE)
	{
		// OK, подходит
		wcscpy_c(gpSrv->szConsoleFont, lpelfe->elfLogFont.lfFaceName);
		return 0;
	}

	return TRUE; // ищем следующий фонт
}


BOOL cmd_SetSizeXXX_CmdStartedFinished(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;
	
		//case CECMD_SETSIZESYNC:
		//case CECMD_SETSIZENOSYNC:
		//case CECMD_CMDSTARTED:
		//case CECMD_CMDFINISHED:
		
	MCHKHEAP;
	int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_RETSIZE);
	*out = ExecuteNewCmd(0,nOutSize);

	if (*out == NULL) return FALSE;

	MCHKHEAP;

	// Need to block all requests to output buffer in other threads
	MSectionLockSimple csRead; csRead.Lock(&gpSrv->csReadConsoleInfo, LOCK_READOUTPUT_TIMEOUT);

	DWORD nTick1 = 0, nTick2 = 0, nTick3 = 0, nTick4 = 0, nTick5 = 0;
	DWORD nWasSetSize = -1;

	if (in.hdr.cbSize >= (sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_SETSIZE)))
	{
		USHORT nBufferHeight = 0;
		COORD  crNewSize = {0,0};
		SMALL_RECT rNewRect = {0};
		SHORT  nNewTopVisible = -1;
		//memmove(&nBufferHeight, in.Data, sizeof(USHORT));
		nBufferHeight = in.SetSize.nBufferHeight;

		if (nBufferHeight == -1)
		{
			// Для 'far /w' нужно оставить высоту буфера!
			if (in.SetSize.size.Y < gpSrv->sbi.dwSize.Y
			        && gpSrv->sbi.dwSize.Y > (gpSrv->sbi.srWindow.Bottom - gpSrv->sbi.srWindow.Top + 1))
			{
				nBufferHeight = gpSrv->sbi.dwSize.Y;
			}
			else
			{
				nBufferHeight = 0;
			}
		}

		//memmove(&crNewSize, in.Data+sizeof(USHORT), sizeof(COORD));
		crNewSize = in.SetSize.size;
		//memmove(&nNewTopVisible, in.Data+sizeof(USHORT)+sizeof(COORD), sizeof(SHORT));
		nNewTopVisible = in.SetSize.nSendTopLine;
		//memmove(&rNewRect, in.Data+sizeof(USHORT)+sizeof(COORD)+sizeof(SHORT), sizeof(SMALL_RECT));
		rNewRect = in.SetSize.rcWindow;
		MCHKHEAP;
		(*out)->hdr.nCmd = in.hdr.nCmd;
		// Все остальные поля заголовка уже заполнены в ExecuteNewCmd

		//#ifdef _DEBUG
		if (in.hdr.nCmd == CECMD_CMDFINISHED)
		{
			PRINT_COMSPEC(L"CECMD_CMDFINISHED, Set height to: %i\n", crNewSize.Y);
			DEBUGSTRCMD(L"\n!!! CECMD_CMDFINISHED !!!\n\n");
			// Вернуть нотификатор
			TODO("Смена режима рефреша консоли")
			//if (gpSrv->dwWinEventThread != 0)
			//	PostThreadMessage(gpSrv->dwWinEventThread, gpSrv->nMsgHookEnableDisable, TRUE, 0);
		}
		else if (in.hdr.nCmd == CECMD_CMDSTARTED)
		{
			PRINT_COMSPEC(L"CECMD_CMDSTARTED, Set height to: %i\n", nBufferHeight);
			DEBUGSTRCMD(L"\n!!! CECMD_CMDSTARTED !!!\n\n");
			// Отключить нотификатор
			TODO("Смена режима рефреша консоли")
			//if (gpSrv->dwWinEventThread != 0)
			//	PostThreadMessage(gpSrv->dwWinEventThread, gpSrv->nMsgHookEnableDisable, FALSE, 0);
		}

		//#endif

		if (in.hdr.nCmd == CECMD_CMDFINISHED)
		{
			// Сохранить данные ВСЕЙ консоли
			PRINT_COMSPEC(L"Storing long output\n", 0);
			CmdOutputStore();
			PRINT_COMSPEC(L"Storing long output (done)\n", 0);
		}

		MCHKHEAP;

		if (in.hdr.nCmd == CECMD_SETSIZESYNC)
		{
			ResetEvent(gpSrv->hAllowInputEvent);
			gpSrv->bInSyncResize = TRUE;
		}

		gpSrv->nTopVisibleLine = nNewTopVisible;
		nTick1 = GetTickCount();
		csRead.Unlock();
		WARNING("Если указан dwFarPID - это что-ли два раза подряд выполнится?");
		nWasSetSize = SetConsoleSize(nBufferHeight, crNewSize, rNewRect, ":CECMD_SETSIZESYNC");
		WARNING("!! Не может ли возникнуть конфликт с фаровским фиксом для убирания полос прокрутки?");
		nTick2 = GetTickCount();

		// вернуть блокировку
		csRead.Lock(&gpSrv->csReadConsoleInfo, LOCK_READOUTPUT_TIMEOUT);

		if (in.hdr.nCmd == CECMD_SETSIZESYNC)
		{
			CESERVER_REQ *pPlgIn = NULL, *pPlgOut = NULL;

			//TODO("Пока закомментарим, чтобы GUI реагировало побыстрее");
			if (in.SetSize.dwFarPID /*&& !nBufferHeight*/)
			{
				// Во избежание каких-то накладок FAR (по крайней мере с /w)
				// стал ресайзить панели только после дерганья мышкой над консолью

				// Need to block all requests to output buffer in other threads
				MSectionLockSimple csRead; csRead.Lock(&gpSrv->csReadConsoleInfo, LOCK_READOUTPUT_TIMEOUT);

				CONSOLE_SCREEN_BUFFER_INFO sc = {{0,0}};
				GetConsoleScreenBufferInfo(ghConOut, &sc);
				HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
				INPUT_RECORD r = {MOUSE_EVENT};
				r.Event.MouseEvent.dwMousePosition.X = sc.srWindow.Right-1;
				r.Event.MouseEvent.dwMousePosition.Y = sc.srWindow.Bottom-1;
				r.Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
				DWORD cbWritten = 0;
				WriteConsoleInput(hIn, &r, 1, &cbWritten);

				csRead.Unlock();

				// Команду можно выполнить через плагин FARа
				wchar_t szPipeName[128];
				_wsprintf(szPipeName, SKIPLEN(countof(szPipeName)) CEPLUGINPIPENAME, L".", in.SetSize.dwFarPID);
				//DWORD nHILO = ((DWORD)crNewSize.X) | (((DWORD)(WORD)crNewSize.Y) << 16);
				//pPlgIn = ExecuteNewCmd(CMD_SETSIZE, sizeof(CESERVER_REQ_HDR)+sizeof(nHILO));
				pPlgIn = ExecuteNewCmd(CMD_REDRAWFAR, sizeof(CESERVER_REQ_HDR));
				//pPlgIn->dwData[0] = nHILO;
				pPlgOut = ExecuteCmd(szPipeName, pPlgIn, 500, ghConWnd);

				if (pPlgOut) ExecuteFreeResult(pPlgOut);
			}

			SetEvent(gpSrv->hAllowInputEvent);
			gpSrv->bInSyncResize = FALSE;

			// ReloadFullConsoleInfo - передает управление в Refresh thread,
			// поэтому блокировку нужно предварительно снять
			csRead.Unlock();

			// Передернуть RefreshThread - перечитать консоль
			ReloadFullConsoleInfo(FALSE); // вызовет Refresh в Refresh thread

			// вернуть блокировку
			csRead.Lock(&gpSrv->csReadConsoleInfo, LOCK_READOUTPUT_TIMEOUT);
		}

		MCHKHEAP;

		if (in.hdr.nCmd == CECMD_CMDSTARTED)
		{
			// Восстановить текст скрытой (прокрученной вверх) части консоли
			CmdOutputRestore(false);
		}
	}

	MCHKHEAP;

	nTick3 = GetTickCount();

	// already blocked
	//csRead.Lock(&gpSrv->csReadConsoleInfo, LOCK_READOUTPUT_TIMEOUT);

	// Prepare result
	(*out)->SetSizeRet.crMaxSize = MyGetLargestConsoleWindowSize(ghConOut);

	nTick4 = GetTickCount();

	PCONSOLE_SCREEN_BUFFER_INFO psc = &((*out)->SetSizeRet.SetSizeRet);
	MyGetConsoleScreenBufferInfo(ghConOut, psc);

	nTick5 = GetTickCount();
	
	DWORD lnNextPacketId = ++gpSrv->nLastPacketID;
	(*out)->SetSizeRet.nNextPacketId = lnNextPacketId;

	//gpSrv->bForceFullSend = TRUE;
	SetEvent(gpSrv->hRefreshEvent);
	MCHKHEAP;
	lbRc = TRUE;

	UNREFERENCED_PARAMETER(nTick1);
	UNREFERENCED_PARAMETER(nTick2);
	UNREFERENCED_PARAMETER(nTick3);
	UNREFERENCED_PARAMETER(nTick4);
	UNREFERENCED_PARAMETER(nTick5);
	UNREFERENCED_PARAMETER(nWasSetSize);
	return lbRc;
}

#if 0
BOOL cmd_GetOutput(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;
	MSectionLock CS; CS.Lock(gpcsStoredOutput, FALSE);

	if (gpStoredOutput)
	{
		_ASSERTE(sizeof(CESERVER_CONSAVE_HDR) > sizeof(gpStoredOutput->hdr.hdr));
		DWORD nSize = sizeof(CESERVER_CONSAVE_HDR)
		              + min((int)gpStoredOutput->hdr.cbMaxOneBufferSize,
		                    (gpStoredOutput->hdr.sbi.dwSize.X*gpStoredOutput->hdr.sbi.dwSize.Y*2));
		*out = ExecuteNewCmd(CECMD_GETOUTPUT, nSize);
		if (*out)
		{
			memmove((*out)->Data, ((LPBYTE)gpStoredOutput) + sizeof(gpStoredOutput->hdr.hdr), nSize - sizeof(gpStoredOutput->hdr.hdr));
			lbRc = TRUE;
		}
	}
	else
	{
		*out = ExecuteNewCmd(CECMD_GETOUTPUT, sizeof(CESERVER_REQ_HDR));
		lbRc = (*out != NULL);
	}

	return lbRc;
}
#endif

BOOL cmd_Attach2Gui(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;
	
	// Может придти из Attach2Gui() плагина
	HWND hDc = Attach2Gui(ATTACH2GUI_TIMEOUT);

	if (hDc != NULL)
	{
		int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD);
		*out = ExecuteNewCmd(CECMD_ATTACH2GUI,nOutSize);

		if (*out != NULL)
		{
			// Чтобы не отображалась "Press any key to close console"
			DisableAutoConfirmExit();
			//
			(*out)->dwData[0] = (DWORD)hDc; //-V205 // Дескриптор окна
			lbRc = TRUE;

			gpSrv->bWasReattached = TRUE;
			if (gpSrv->hRefreshEvent)
			{
				SetEvent(gpSrv->hRefreshEvent);
			}
			else
			{
				_ASSERTE(gpSrv->hRefreshEvent!=NULL);
			}
		}
	}
	
	return lbRc;
}

BOOL cmd_FarLoaded(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;

	#if 0
	wchar_t szDbg[512], szExe[MAX_PATH], szTitle[512];
	GetModuleFileName(NULL, szExe, countof(szExe));
	_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"cmd_FarLoaded: %s", PointToName(szExe));
	_wsprintf(szDbg, SKIPLEN(countof(szDbg))
		L"cmd_FarLoaded was received\nServerPID=%u, name=%s\nFarPID=%u\nRootPID=%u\nDisablingConfirm=%s",
		GetCurrentProcessId(), PointToName(szExe), in.hdr.nSrcPID, gpSrv->dwRootProcess,
		((gbAutoDisableConfirmExit || (gnConfirmExitParm == 1)) && gpSrv->dwRootProcess == in.dwData[0]) ? L"YES" :
		gbAlwaysConfirmExit ? L"AlreadyOFF" : L"NO");
	MessageBox(NULL, szDbg, szTitle, MB_SYSTEMMODAL);
	#endif
	
	// gnConfirmExitParm==1 получается, когда консоль запускалась через "-new_console"
	// Если плагин фара загрузился - думаю можно отключить подтверждение закрытия консоли
	if ((gbAutoDisableConfirmExit || (gnConfirmExitParm == 1)) && gpSrv->dwRootProcess == in.dwData[0])
	{
		// FAR нормально запустился, считаем что все ок и подтверждения закрытия консоли не потребуется
		DisableAutoConfirmExit(TRUE);
	}
	
	int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD);
	*out = ExecuteNewCmd(CECMD_FARLOADED,nOutSize);
	if (*out != NULL)
	{
		(*out)->dwData[0] = GetCurrentProcessId();
		lbRc = TRUE;
	}

	return lbRc;
}

BOOL cmd_PostConMsg(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;
	
	HWND hSendWnd = (HWND)in.Msg.hWnd;

	if ((in.Msg.nMsg == WM_CLOSE) && (hSendWnd == ghConWnd))
	{
		// Чтобы при закрытии не _мелькало_ "Press Enter to Close console"
		gbInShutdown = TRUE;
	}

	// Info & Log
	if (in.Msg.nMsg == WM_INPUTLANGCHANGE || in.Msg.nMsg == WM_INPUTLANGCHANGEREQUEST)
	{
		WPARAM wParam = (WPARAM)in.Msg.wParam;
		LPARAM lParam = (LPARAM)in.Msg.lParam;
		unsigned __int64 l = lParam;
		
		#ifdef _DEBUG
		wchar_t szDbg[255];
		_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"ConEmuC: %s(0x%08X, %s, CP:%i, HKL:0x%08I64X)\n",
		          in.Msg.bPost ? L"PostMessage" : L"SendMessage", (DWORD)hSendWnd,
		          (in.Msg.nMsg == WM_INPUTLANGCHANGE) ? L"WM_INPUTLANGCHANGE" :
		          (in.Msg.nMsg == WM_INPUTLANGCHANGEREQUEST) ? L"WM_INPUTLANGCHANGEREQUEST" :
		          (in.Msg.nMsg == WM_SHOWWINDOW) ? L"WM_SHOWWINDOW" :
		          L"<Other message>",
		          (DWORD)wParam, l);
		DEBUGLOGLANG(szDbg);
		#endif

		if (gpLogSize)
		{
			char szInfo[255];
			_wsprintfA(szInfo, SKIPLEN(countof(szInfo)) "ConEmuC: %s(0x%08X, %s, CP:%i, HKL:0x%08I64X)",
			           in.Msg.bPost ? "PostMessage" : "SendMessage", (DWORD)hSendWnd, //-V205
			           (in.Msg.nMsg == WM_INPUTLANGCHANGE) ? "WM_INPUTLANGCHANGE" :
			           (in.Msg.nMsg == WM_INPUTLANGCHANGEREQUEST) ? "WM_INPUTLANGCHANGEREQUEST" :
			           "<Other message>",
			           (DWORD)wParam, l);
			LogString(szInfo);
		}
	}

	if (in.Msg.nMsg == WM_SHOWWINDOW)
	{
		DWORD lRc = 0;

		if (in.Msg.bPost)
			lRc = apiShowWindowAsync(hSendWnd, (int)(in.Msg.wParam & 0xFFFF));
		else
			lRc = apiShowWindow(hSendWnd, (int)(in.Msg.wParam & 0xFFFF));

		// Возвращаем результат
		DWORD dwErr = GetLastError();
		int nOutSize = sizeof(CESERVER_REQ_HDR) + 2*sizeof(DWORD);
		*out = ExecuteNewCmd(CECMD_POSTCONMSG,nOutSize);

		if (*out != NULL)
		{
			(*out)->dwData[0] = lRc;
			(*out)->dwData[1] = dwErr;
			lbRc = TRUE;
		}
	}
	else if (in.Msg.nMsg == WM_SIZE)
	{
		//
	}
	else if (in.Msg.bPost)
	{
		PostMessage(hSendWnd, in.Msg.nMsg, (WPARAM)in.Msg.wParam, (LPARAM)in.Msg.lParam);
	}
	else
	{
		LRESULT lRc = SendMessage(hSendWnd, in.Msg.nMsg, (WPARAM)in.Msg.wParam, (LPARAM)in.Msg.lParam);
		// Возвращаем результат
		int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(u64);
		*out = ExecuteNewCmd(CECMD_POSTCONMSG,nOutSize);

		if (*out != NULL)
		{
			(*out)->qwData[0] = lRc;
			lbRc = TRUE;
		}
	}
	
	return lbRc;
}

BOOL cmd_SaveAliases(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;
	
	//wchar_t* pszAliases; DWORD nAliasesSize;
	// Запомнить алиасы
	DWORD nNewSize = in.hdr.cbSize - sizeof(in.hdr);

	if (nNewSize > gpSrv->nAliasesSize)
	{
		MCHKHEAP;
		wchar_t* pszNew = (wchar_t*)calloc(nNewSize, 1);

		if (!pszNew)
			goto wrap;  // не удалось выдеить память

		MCHKHEAP;

		if (gpSrv->pszAliases) free(gpSrv->pszAliases);

		MCHKHEAP;
		gpSrv->pszAliases = pszNew;
		gpSrv->nAliasesSize = nNewSize;
	}

	if (nNewSize > 0 && gpSrv->pszAliases)
	{
		MCHKHEAP;
		memmove(gpSrv->pszAliases, in.Data, nNewSize);
		MCHKHEAP;
	}
	
wrap:
	return lbRc;
}

BOOL cmd_GetAliases(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;
	
	// Возвращаем запомненные алиасы
	int nOutSize = sizeof(CESERVER_REQ_HDR) + gpSrv->nAliasesSize;
	*out = ExecuteNewCmd(CECMD_GETALIASES,nOutSize);

	if (*out != NULL)
	{
		if (gpSrv->pszAliases && gpSrv->nAliasesSize)
		{
			memmove((*out)->Data, gpSrv->pszAliases, gpSrv->nAliasesSize);
		}
		lbRc = TRUE;
	}
	
	return lbRc;
}

BOOL cmd_SetDontClose(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;
	
	// После детача в фаре команда (например dir) схлопнется, чтобы
	// консоль неожиданно не закрылась...
	gbAutoDisableConfirmExit = FALSE;
	gbAlwaysConfirmExit = TRUE;
	
	return lbRc;
}

BOOL cmd_OnActivation(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;
	
	if (gpSrv /*->pConsole*/)
	{
		//gpSrv->pConsole->hdr.bConsoleActive = in.dwData[0];
		//gpSrv->pConsole->hdr.bThawRefreshThread = in.dwData[1];

		//if (gpLogSize)
		//{
		//	char szInfo[128];
		//	_wsprintfA(szInfo, SKIPLEN(countof(szInfo)) "ConEmuC: cmd_OnActivation(active=%u, speed=%s)", in.dwData[0], in.dwData[1] ? "high" : "low");
		//	LogString(szInfo);
		//}

		////gpSrv->pConsoleMap->SetFrom(&(gpSrv->pConsole->hdr));
		//UpdateConsoleMapHeader();

		//// Если консоль активировали - то принудительно перечитать ее содержимое
		//if (gpSrv->pConsole->hdr.bConsoleActive)


		// Принудить RefreshThread перечитать статус активности консоли
		gpSrv->nLastConsoleActiveTick = 0;


		if (gpSrv->dwAltServerPID && (gpSrv->dwAltServerPID != gnSelfPID))
		{
			CESERVER_REQ* pAltIn = ExecuteNewCmd(CECMD_ONACTIVATION, sizeof(CESERVER_REQ_HDR));
			if (pAltIn)
			{
				CESERVER_REQ* pAltOut = ExecuteSrvCmd(gpSrv->dwAltServerPID, pAltIn, ghConWnd);
				ExecuteFreeResult(pAltIn);
				ExecuteFreeResult(pAltOut);
			}
		}
		else
		{
			ReloadFullConsoleInfo(TRUE);
		}
	}
	
	return lbRc;
}

BOOL cmd_SetWindowPos(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;
	BOOL lbWndRc = FALSE, lbShowRc = FALSE;
	DWORD nErrCode[2] = {};

	lbWndRc = SetWindowPos(in.SetWndPos.hWnd, in.SetWndPos.hWndInsertAfter,
	             in.SetWndPos.X, in.SetWndPos.Y, in.SetWndPos.cx, in.SetWndPos.cy,
	             in.SetWndPos.uFlags);
	nErrCode[0] = lbWndRc ? 0 : GetLastError();

	if ((in.SetWndPos.uFlags & SWP_SHOWWINDOW) && !IsWindowVisible(in.SetWndPos.hWnd))
	{
		lbShowRc = ShowWindowAsync(in.SetWndPos.hWnd, SW_SHOW);
		nErrCode[1] = lbShowRc ? 0 : GetLastError();
	}
	
	// Результат
	int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD)*4;
	*out = ExecuteNewCmd(CECMD_SETWINDOWPOS,nOutSize);

	if (*out != NULL)
	{
		(*out)->dwData[0] = lbWndRc;
		(*out)->dwData[1] = nErrCode[0];
		(*out)->dwData[2] = lbShowRc;
		(*out)->dwData[3] = nErrCode[1];

		lbRc = TRUE;
	}

	return lbRc;
}

BOOL cmd_SetFocus(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE, lbForeRc = FALSE;
	HWND hFocusRc = NULL;
	
	if (in.setFocus.bSetForeground)
		lbForeRc = SetForegroundWindow(in.setFocus.hWindow);
	else
		hFocusRc = SetFocus(in.setFocus.hWindow);
	
	return lbRc;
}

BOOL cmd_SetParent(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE; //, lbForeRc = FALSE;

	HWND h = SetParent(in.setParent.hWnd, in.setParent.hParent);

	int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_SETPARENT);
	*out = ExecuteNewCmd(CECMD_SETPARENT,nOutSize);

	if (*out != NULL)
	{
		(*out)->setParent.hWnd = (HWND)GetLastError();
		(*out)->setParent.hParent = h;

		lbRc = TRUE;
	}

	return lbRc;
}

BOOL cmd_SetWindowRgn(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;
	
	MySetWindowRgn(&in.SetWndRgn);
	
	return lbRc;
}

BOOL cmd_DetachCon(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;

	//if (gOSVer.dwMajorVersion >= 6)
	//{
	//	if ((gcrVisibleSize.X <= 0) && (gcrVisibleSize.Y <= 0))
	//	{
	//		_ASSERTE((gcrVisibleSize.X > 0) && (gcrVisibleSize.Y > 0));
	//	}
	//	else
	//	{
	//		int curSizeY, curSizeX;
	//		wchar_t sFontName[LF_FACESIZE];
	//		HANDLE hOutput = (HANDLE)ghConOut;

	//		if (apiGetConsoleFontSize(hOutput, curSizeY, curSizeX, sFontName) && curSizeY && curSizeX)
	//		{
	//			COORD crLargest = MyGetLargestConsoleWindowSize(hOutput);
	//			HMONITOR hMon = MonitorFromWindow(ghConWnd, MONITOR_DEFAULTTOPRIMARY);
	//			MONITORINFO mi = {sizeof(mi)};
	//			int nMaxX = 0, nMaxY = 0;
	//			if (GetMonitorInfo(hMon, &mi))
	//			{
	//				nMaxX = mi.rcWork.right - mi.rcWork.left - 2*GetSystemMetrics(SM_CXSIZEFRAME) - GetSystemMetrics(SM_CYCAPTION);
	//				nMaxY = mi.rcWork.bottom - mi.rcWork.top - 2*GetSystemMetrics(SM_CYSIZEFRAME);
	//			}
	//			
	//			if ((nMaxX > 0) && (nMaxY > 0))
	//			{
	//				int nFontX = nMaxX  / gcrVisibleSize.X;
	//				int nFontY = nMaxY / gcrVisibleSize.Y;
	//				// Too large height?
	//				if (nFontY > 28)
	//				{
	//					nFontX = 28 * nFontX / nFontY;
	//					nFontY = 28;
	//				}

	//				// Evaluate default width for the font
	//				int nEvalX = EvaluateDefaultFontWidth(nFontY, sFontName);
	//				if (nEvalX > 0)
	//					nFontX = nEvalX;

	//				// Look in the registry?
	//				HKEY hk;
	//				DWORD nRegSize = 0, nLen;
	//				if (!RegOpenKeyEx(HKEY_CURRENT_USER, L"Console", 0, KEY_READ, &hk))
	//				{
	//					if (RegQueryValueEx(hk, L"FontSize", NULL, NULL, (LPBYTE)&nRegSize, &(nLen=sizeof(nRegSize))) || nLen!=sizeof(nRegSize))
	//						nRegSize = 0;
	//					RegCloseKey(hk);
	//				}
	//				if (!nRegSize && !RegOpenKeyEx(HKEY_CURRENT_USER, L"Console\\%SystemRoot%_system32_cmd.exe", 0, KEY_READ, &hk))
	//				{
	//					if (RegQueryValueEx(hk, L"FontSize", NULL, NULL, (LPBYTE)&nRegSize, &(nLen=sizeof(nRegSize))) || nLen!=sizeof(nRegSize))
	//						nRegSize = 0;
	//					RegCloseKey(hk);
	//				}
	//				if ((HIWORD(nRegSize) > curSizeY) && (HIWORD(nRegSize) < nFontY)
	//					&& (LOWORD(nRegSize) > curSizeX) && (LOWORD(nRegSize) < nFontX))
	//				{
	//					nFontY = HIWORD(nRegSize);
	//					nFontX = LOWORD(nRegSize);
	//				}

	//				if ((nFontX > curSizeX) || (nFontY > curSizeY))
	//				{
	//					apiSetConsoleFontSize(hOutput, nFontY, nFontX, sFontName);
	//				}
	//			}
	//		}
	//	}
	//}
	
	gpSrv->bWasDetached = TRUE;
	ghConEmuWnd = NULL;
	SetConEmuWindows(NULL, NULL);
	gpSrv->dwGuiPID = 0;
	UpdateConsoleMapHeader();

	HWND hGuiApp = NULL;
	if (in.DataSize() >= sizeof(DWORD))
	{
		hGuiApp = (HWND)in.dwData[0];
		if (hGuiApp && !IsWindow(hGuiApp))
			hGuiApp = NULL;
	}

	if (in.DataSize() >= 2*sizeof(DWORD) && in.dwData[1])
	{
		if (hGuiApp)
			PostMessage(hGuiApp, WM_CLOSE, 0, 0);
		PostMessage(ghConWnd, WM_CLOSE, 0, 0);
	}
	else
	{
		// Без мелькания консольного окошка почему-то пока не получается
		// Наверх выносится ConEmu вместо "отцепленного" GUI приложения
		EmergencyShow(ghConWnd);
	}

	DisableAutoConfirmExit(FALSE);

	SafeCloseHandle(gpSrv->hRootProcess);
	SafeCloseHandle(gpSrv->hRootThread);

	if (hGuiApp != NULL)
	{
		//apiShowWindow(ghConWnd, SW_SHOWMINIMIZED);
		apiSetForegroundWindow(hGuiApp);
		SetTerminateEvent(ste_CmdDetachCon);
	}

	int nOutSize = sizeof(CESERVER_REQ_HDR);
	*out = ExecuteNewCmd(CECMD_DETACHCON,nOutSize);
	lbRc = (*out != NULL);
	
	return lbRc;
}

BOOL cmd_CmdStartStop(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;
	
	MSectionLock CS; CS.Lock(gpSrv->csProc);
	
	UINT nPrevCount = gpSrv->nProcessCount;
	BOOL lbChanged = FALSE;

	_ASSERTE(in.StartStop.dwPID!=0);
	DWORD nPID = in.StartStop.dwPID;
	DWORD nPrevAltServerPID = gpSrv->dwAltServerPID;

	if (!gpSrv->nProcessStartTick && (gpSrv->dwRootProcess == in.StartStop.dwPID))
	{
		gpSrv->nProcessStartTick = GetTickCount();
	}

	MSectionLock CsAlt;
	if (((in.StartStop.nStarted == sst_AltServerStop) || (in.StartStop.nStarted == sst_AppStop))
		&& in.StartStop.dwPID && (in.StartStop.dwPID == gpSrv->dwAltServerPID))
	{
		// _ASSERTE могут приводить к ошибкам блокировки gpSrv->csProc в других потоках. Но ассертов быть не должно )
		_ASSERTE(GetCurrentThreadId() != gpSrv->dwRefreshThread);
		CsAlt.Lock(gpSrv->csAltSrv, TRUE, 1000);
	}
	
#ifdef _DEBUG
	wchar_t szDbg[128];

	switch (in.StartStop.nStarted)
	{
		case sst_ServerStart:
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"SRV received CECMD_CMDSTARTSTOP(ServerStart,%i,PID=%u)\n", in.hdr.nCreateTick, in.StartStop.dwPID);
			break;
		case sst_AltServerStart:
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"SRV received CECMD_CMDSTARTSTOP(AltServerStart,%i,PID=%u)\n", in.hdr.nCreateTick, in.StartStop.dwPID);
			break;
		case sst_ServerStop:
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"SRV received CECMD_CMDSTARTSTOP(ServerStop,%i,PID=%u)\n", in.hdr.nCreateTick, in.StartStop.dwPID);
			break;
		case sst_AltServerStop:
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"SRV received CECMD_CMDSTARTSTOP(AltServerStop,%i,PID=%u)\n", in.hdr.nCreateTick, in.StartStop.dwPID);
			break;
		case sst_ComspecStart:
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"SRV received CECMD_CMDSTARTSTOP(ComspecStart,%i,PID=%u)\n", in.hdr.nCreateTick, in.StartStop.dwPID);
			break;
		case sst_ComspecStop:
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"SRV received CECMD_CMDSTARTSTOP(ComspecStop,%i,PID=%u)\n", in.hdr.nCreateTick, in.StartStop.dwPID);
			break;
		case sst_AppStart:
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"SRV received CECMD_CMDSTARTSTOP(AppStart,%i,PID=%u)\n", in.hdr.nCreateTick, in.StartStop.dwPID);
			break;
		case sst_AppStop:
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"SRV received CECMD_CMDSTARTSTOP(AppStop,%i,PID=%u)\n", in.hdr.nCreateTick, in.StartStop.dwPID);
			break;
		case sst_App16Start:
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"SRV received CECMD_CMDSTARTSTOP(App16Start,%i,PID=%u)\n", in.hdr.nCreateTick, in.StartStop.dwPID);
			break;
		case sst_App16Stop:
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"SRV received CECMD_CMDSTARTSTOP(App16Stop,%i,PID=%u)\n", in.hdr.nCreateTick, in.StartStop.dwPID);
			break;
		default:
			// _ASSERTE могут приводить к ошибкам блокировки gpSrv->csProc в других потоках. Но ассертов быть не должно )
			_ASSERTE(in.StartStop.nStarted==sst_ServerStart && "Unknown StartStop code!");
	}

	DEBUGSTRCMD(szDbg);
#endif

	// _ASSERTE могут приводить к ошибкам блокировки gpSrv->csProc в других потоках. Но ассертов быть не должно )
	_ASSERTE(in.StartStop.dwPID!=0);


	// Переменные, чтобы позвать функцию AltServerWasStarted после отпускания CS.Unlock()
	bool AltServerChanged = false;
	DWORD nAltServerWasStarted = 0, nAltServerWasStopped = 0;
	DWORD nCurAltServerPID = gpSrv->dwAltServerPID;
	HANDLE hAltServerWasStarted = NULL;
	bool ForceThawAltServer = false;
	AltServerInfo info = {};


	if (in.StartStop.nStarted == sst_AltServerStart)
	{
		// Перевести нить монитора в режим ожидания завершения AltServer, инициализировать gpSrv->dwAltServerPID, gpSrv->hAltServer
		// _ASSERTE могут приводить к ошибкам блокировки gpSrv->csProc в других потоках. Но ассертов быть не должно )
		_ASSERTE(in.StartStop.hServerProcessHandle!=0);

		nAltServerWasStarted = in.StartStop.dwPID;
		hAltServerWasStarted = (HANDLE)(DWORD_PTR)in.StartStop.hServerProcessHandle;
		AltServerChanged = true;
		//ForceThawAltServer = false;
		//AltServerWasStarted(in.StartStop.dwPID, (HANDLE)(DWORD_PTR)in.StartStop.hServerProcessHandle);
	}
	else if ((in.StartStop.nStarted == sst_ComspecStart)
			|| (in.StartStop.nStarted == sst_AppStart))
	{
		// Добавить процесс в список
		// _ASSERTE могут приводить к ошибкам блокировки gpSrv->csProc в других потоках. Но ассертов быть не должно )
		_ASSERTE(gpSrv->pnProcesses[0] == gnSelfPID);
		lbChanged = ProcessAdd(nPID, &CS);
	}
	else if ((in.StartStop.nStarted == sst_AltServerStop)
			|| (in.StartStop.nStarted == sst_ComspecStop)
			|| (in.StartStop.nStarted == sst_AppStop))
	{
		// Issue 623: Не удалять из списка корневой процесс _сразу_, пусть он "отвалится" обычным образом
		if (nPID != gpSrv->dwRootProcess)
		{
			// Удалить процесс из списка
			// _ASSERTE могут приводить к ошибкам блокировки gpSrv->csProc в других потоках. Но ассертов быть не должно )
			_ASSERTE(gpSrv->pnProcesses[0] == gnSelfPID);
			lbChanged = ProcessRemove(nPID, nPrevCount, &CS);
		}
		else
		{
			if (in.StartStop.bWasSucceededInRead)
			{
				// В консоли был успешный вызов ReadConsole/ReadConsoleInput.
				// Отключить "Press enter to close console".
				if (gbAutoDisableConfirmExit && (gnConfirmExitParm == 0))
				{
					DisableAutoConfirmExit(FALSE);
				}
			}
		}

		DWORD nAltPID = (in.StartStop.nStarted == sst_ComspecStop) ? in.StartStop.nOtherPID : nPID;

		// Если это закрылся AltServer
		if (nAltPID)
		{
			bool bPrevFound = gpSrv->AltServers.Get(nAltPID, &info, true/*Remove*/);

			// _ASSERTE могут приводить к ошибкам блокировки gpSrv->csProc в других потоках. Но ассертов быть не должно )
			_ASSERTE(in.StartStop.nStarted==sst_ComspecStop || in.StartStop.nOtherPID==info.nPrevPID);

			// Сначала проверяем, не текущий ли альт.сервер закрывается
			if (gpSrv->dwAltServerPID && (nAltPID == gpSrv->dwAltServerPID))
			{
				// Поскольку текущий сервер завершается - то сразу сбросим PID (его морозить уже не нужно)
				nAltServerWasStopped = nAltPID;
				gpSrv->dwAltServerPID = 0;
				// Переключаемся на "старый" (если был)
				if (bPrevFound && info.nPrevPID)
				{
					// _ASSERTE могут приводить к ошибкам блокировки gpSrv->csProc в других потоках. Но ассертов быть не должно )
					_ASSERTE(info.hPrev!=NULL);
					// Перевести нить монитора в обычный режим, закрыть gpSrv->hAltServer
					// Активировать альтернативный сервер (повторно), отпустить его нити чтения
					AltServerChanged = true;
					nAltServerWasStarted = info.nPrevPID;
					hAltServerWasStarted = info.hPrev;
					ForceThawAltServer = true;
					//AltServerWasStarted(in.StartStop.nPrevAltServerPID, NULL, true);
				}
				else
				{
					// _ASSERTE могут приводить к ошибкам блокировки gpSrv->csProc в других потоках. Но ассертов быть не должно )
					_ASSERTE(info.hPrev==NULL);
					AltServerChanged = true;
					// Уведомить ГУЙ!
					//gpSrv->dwAltServerPID = 0;
					//SafeCloseHandle(gpSrv->hAltServer);
				}
			}
			else
			{
				// _ASSERTE могут приводить к ошибкам блокировки gpSrv->csProc в других потоках. Но ассертов быть не должно )
				_ASSERTE(((nPID == gpSrv->dwAltServerPID) || !gpSrv->dwAltServerPID || ((in.StartStop.nStarted != sst_AltServerStop) && (nPID != gpSrv->dwAltServerPID) && !bPrevFound)) && "Expected active alt.server!");
            }
		}
	}
	else
	{
		// _ASSERTE могут приводить к ошибкам блокировки gpSrv->csProc в других потоках. Но ассертов быть не должно )
		_ASSERTE(in.StartStop.nStarted==sst_AppStart || in.StartStop.nStarted==sst_AppStop || in.StartStop.nStarted==sst_ComspecStart || in.StartStop.nStarted==sst_ComspecStop);
	}
	
	// ***
	if (lbChanged)
		ProcessCountChanged(TRUE, nPrevCount, &CS);
	CS.Unlock();
	// ***

	// После Unlock-а, зовем функцию
	if (AltServerChanged)
	{
		if (nAltServerWasStarted)
		{
			AltServerWasStarted(nAltServerWasStarted, hAltServerWasStarted, ForceThawAltServer);
		}
		else if (nCurAltServerPID && (nPID == nCurAltServerPID))
		{
			if (gpSrv->hAltServerChanged)
			{
				// Чтобы не подраться между потоками - закрывать хэндл только в RefreshThread
				gpSrv->hCloseAltServer = gpSrv->hAltServer;
				gpSrv->dwAltServerPID = 0;
				gpSrv->hAltServer = NULL;
				// В RefreshThread ожидание хоть и небольшое (100мс), но лучше передернуть
				SetEvent(gpSrv->hAltServerChanged);
			}
			else
			{
				gpSrv->dwAltServerPID = 0;
				SafeCloseHandle(gpSrv->hAltServer);
				_ASSERTE(gpSrv->hAltServerChanged!=NULL);
			}
		}

		if (!ghConEmuWnd || !IsWindow(ghConEmuWnd))
		{
			_ASSERTE((ghConEmuWnd==NULL) && "ConEmu GUI was terminated? Invalid ghConEmuWnd");
		}
		else
		{
			CESERVER_REQ *pGuiIn = NULL, *pGuiOut = NULL;
			int nSize = sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_STARTSTOP);
			pGuiIn = ExecuteNewCmd(CECMD_CMDSTARTSTOP, nSize);

			if (!pGuiIn)
			{
				_ASSERTE(pGuiIn!=NULL && "Memory allocation failed");
			}
			else
			{
				pGuiIn->StartStop = in.StartStop;
				pGuiIn->StartStop.dwPID = nAltServerWasStarted ? nAltServerWasStarted : nAltServerWasStopped;
				pGuiIn->StartStop.hServerProcessHandle = NULL; // для GUI смысла не имеет
				pGuiIn->StartStop.nStarted = nAltServerWasStarted ? sst_AltServerStart : sst_AltServerStop;
				if (pGuiIn->StartStop.nStarted == sst_AltServerStop)
				{
					// Если это был последний процесс в консоли, то главный сервер тоже закрывается
					// Переоткрывать пайпы в ConEmu нельзя
					pGuiIn->StartStop.bMainServerClosing = gbQuit || (WaitForSingleObject(ghExitQueryEvent,0) == WAIT_OBJECT_0);
				}

				pGuiOut = ExecuteGuiCmd(ghConWnd, pGuiIn, ghConWnd);

				_ASSERTE(pGuiOut!=NULL && "Can not switch GUI to alt server?"); // успешное выполнение?
				ExecuteFreeResult(pGuiOut);
				ExecuteFreeResult(pGuiIn);
			}
		}
	}

	CsAlt.Unlock();

	int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_STARTSTOPRET);
	*out = ExecuteNewCmd(CECMD_CMDSTARTSTOP,nOutSize);

	if (*out != NULL)
	{
		(*out)->StartStopRet.bWasBufferHeight = (gnBufferHeight != 0);
		(*out)->StartStopRet.hWnd = ghConEmuWnd;
		(*out)->StartStopRet.hWndDc = ghConEmuWndDC;
		(*out)->StartStopRet.hWndBack = ghConEmuWndBack;
		(*out)->StartStopRet.dwPID = gpSrv->dwGuiPID;
		(*out)->StartStopRet.nBufferHeight = gnBufferHeight;
		(*out)->StartStopRet.nWidth = gpSrv->sbi.dwSize.X;
		(*out)->StartStopRet.nHeight = (gpSrv->sbi.srWindow.Bottom - gpSrv->sbi.srWindow.Top + 1);
		_ASSERTE(gnRunMode==RM_SERVER);
		(*out)->StartStopRet.dwMainSrvPID = GetCurrentProcessId();
		(*out)->StartStopRet.dwAltSrvPID = gpSrv->dwAltServerPID;
		if (in.StartStop.nStarted == sst_AltServerStart)
		{
			(*out)->StartStopRet.dwPrevAltServerPID = nPrevAltServerPID;
		}

		lbRc = TRUE;
	}
	
	return lbRc;
}

BOOL cmd_SetFarPID(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;

	WARNING("***ALT*** не нужно звать ConEmuC, если вызывающий процесс уже альт.сервер");

	gpSrv->nActiveFarPID = in.hdr.nSrcPID;
	UpdateConsoleMapHeader();

	return lbRc;
}

BOOL cmd_GuiChanged(CESERVER_REQ& in, CESERVER_REQ** out)
{
	// посылается в сервер, чтобы он обновил у себя ConEmuGuiMapping->CESERVER_CONSOLE_MAPPING_HDR
	BOOL lbRc = FALSE;

	if (gpSrv && gpSrv->pConsole)
	{
		ReloadGuiSettings(&in.GuiInfo);
	}

	return lbRc;
}

BOOL cmd_TerminatePid(CESERVER_REQ& in, CESERVER_REQ** out)
{
	// Прибить процесс (TerminateProcess)
	BOOL lbRc = FALSE;
	HANDLE hProcess = NULL;
	BOOL bNeedClose = FALSE;
	DWORD nErrCode = 0;

	if (gpSrv && gpSrv->pConsole)
	{
		if (in.dwData[0] == gpSrv->dwRootProcess)
		{
			hProcess = gpSrv->hRootProcess;
			bNeedClose = FALSE;
		}
	}

	if (!hProcess)
	{
		hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, in.dwData[0]);
		if (hProcess != NULL)
		{
			bNeedClose = TRUE;
		}
		else
		{
			nErrCode = GetLastError();
		}
	}

	if (hProcess != NULL)
	{
		if (TerminateProcess(hProcess, 100))
		{
			lbRc = TRUE;
		}
		else
		{
			nErrCode = GetLastError();
			if (!bNeedClose)
			{
				hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, in.dwData[0]);
				if (hProcess != NULL)
				{
					bNeedClose = TRUE;
					if (TerminateProcess(hProcess, 100))
						lbRc = TRUE;
				}
			}
		}
	}

	if (lbRc)
	{
		// Иначе если прихлопывается процесс - будет "Press enter to close console"
		DisableAutoConfirmExit();
	}

	int nOutSize = sizeof(CESERVER_REQ_HDR) + 2*sizeof(DWORD);
	*out = ExecuteNewCmd(CECMD_TERMINATEPID,nOutSize);

	if (*out != NULL)
	{
		(*out)->dwData[0] = lbRc;
		(*out)->dwData[1] = nErrCode;
	}

	if (hProcess && bNeedClose)
		CloseHandle(hProcess);
	return lbRc;
}

BOOL cmd_GuiAppAttached(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;

	if (!gpSrv)
	{
		_ASSERTE(gpSrv!=NULL);
	}
	else
	{
		_ASSERTEX(in.AttachGuiApp.nPID == gpSrv->dwRootProcess && gpSrv->dwRootProcess && in.AttachGuiApp.nPID);

		wchar_t szInfo[MAX_PATH*2];

		// Вызывается два раза. Первый (при запуске exe) ahGuiWnd==NULL, второй - после фактического создания окна
		if (gbAttachMode || (gpSrv->hRootProcessGui == NULL))
		{
			_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"GUI application (PID=%u) was attached to ConEmu:\n%s\n",
				in.AttachGuiApp.nPID, in.AttachGuiApp.sAppFileName);
			_wprintf(szInfo);
		}

		if (in.AttachGuiApp.hAppWindow && (gbAttachMode || (gpSrv->hRootProcessGui != in.AttachGuiApp.hAppWindow)))
		{
			wchar_t szTitle[MAX_PATH] = {};
			GetWindowText(in.AttachGuiApp.hAppWindow, szTitle, countof(szTitle));
			wchar_t szClass[MAX_PATH] = {};
			GetClassName(in.AttachGuiApp.hAppWindow, szClass, countof(szClass));
			_wsprintf(szInfo,  SKIPLEN(countof(szInfo))
				L"\nWindow (x%08X,Style=x%08X,Ex=x%X,Flags=x%X) was attached to ConEmu:\nTitle: \"%s\"\nClass: \"%s\"\n",
				(DWORD)in.AttachGuiApp.hAppWindow, in.AttachGuiApp.Styles.nStyle, in.AttachGuiApp.Styles.nStyleEx, in.AttachGuiApp.nFlags, szTitle, szClass);
			_wprintf(szInfo);
		}

		if (in.AttachGuiApp.hAppWindow == NULL)
		{
			gpSrv->hRootProcessGui = (HWND)-1;
		}
		else
		{
			gpSrv->hRootProcessGui = in.AttachGuiApp.hAppWindow;
		}
		// Смысла в подтверждении нет - GUI приложение в консоль ничего не выводит
		gbAlwaysConfirmExit = FALSE;
		CheckProcessCount(TRUE);
		lbRc = TRUE;
	}

	int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD);
	*out = ExecuteNewCmd(CECMD_ATTACHGUIAPP,nOutSize);

	if (*out != NULL)
	{
		(*out)->dwData[0] = lbRc;
	}

	return TRUE;
}

#ifdef USE_COMMIT_EVENT
BOOL cmd_RegExtConsole(CESERVER_REQ& in, CESERVER_REQ** out)
{
	PRAGMA_ERROR("CECMD_REGEXTCONSOLE is deprecated!");

	BOOL lbRc = FALSE;

	if (!gpSrv)
	{
		_ASSERTE(gpSrv!=NULL);
	}
	else
	{
		if (in.RegExtCon.hCommitEvent != NULL)
		{
			if (gpSrv->hExtConsoleCommit && gpSrv->hExtConsoleCommit != (HANDLE)in.RegExtCon.hCommitEvent)
				CloseHandle(gpSrv->hExtConsoleCommit);
			gpSrv->hExtConsoleCommit = in.RegExtCon.hCommitEvent;
			gpSrv->nExtConsolePID = in.hdr.nSrcPID;
		}
		else
		{
			if (gpSrv->hExtConsoleCommit == (HANDLE)in.RegExtCon.hCommitEvent)
			{
				CloseHandle(gpSrv->hExtConsoleCommit);
				gpSrv->hExtConsoleCommit = NULL;
				gpSrv->nExtConsolePID = 0;
			}
		}

		lbRc = TRUE;
	}

	int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD);
	*out = ExecuteNewCmd(CECMD_REGEXTCONSOLE, nOutSize);

	if (*out != NULL)
	{
		(*out)->dwData[0] = lbRc;
	}

	return TRUE;
}
#endif

BOOL cmd_FreezeAltServer(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;
	DWORD nPrevAltServer = 0;

	if (!gpSrv)
	{
		_ASSERTE(gpSrv!=NULL);
	}
	else
	{
		// dwData[0]=1-Freeze, 0-Thaw; dwData[1]=New Alt server PID
		if (in.dwData[0] == 1)
		{
			if (!gpSrv->hFreezeRefreshThread)
				gpSrv->hFreezeRefreshThread = CreateEvent(NULL, TRUE, FALSE, NULL);
			ResetEvent(gpSrv->hFreezeRefreshThread);
		}
		else
		{
			if (gpSrv->hFreezeRefreshThread)
				SetEvent(gpSrv->hFreezeRefreshThread);

			if (gnRunMode == RM_ALTSERVER)
			{
				// Уведомить GUI, что альт.сервер снова в работе!
				SendStarted();
			}
			else
			{
				_ASSERTE(gnRunMode == RM_ALTSERVER);
			}
		}

		lbRc = TRUE;
	}

	int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD)*2;
	*out = ExecuteNewCmd(CECMD_FREEZEALTSRV, nOutSize);

	if (*out != NULL)
	{
		(*out)->dwData[0] = lbRc;
		(*out)->dwData[1] = nPrevAltServer; // Reserved
	}

	return TRUE;
}

BOOL cmd_LoadFullConsoleData(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;
	//DWORD nPrevAltServer = 0;

	// В Win7 закрытие дескриптора в ДРУГОМ процессе - закрывает консольный буфер ПОЛНОСТЬЮ!!!
	// В итоге, буфер вывода telnet'а схлопывается!
	if (gpSrv->bReopenHandleAllowed)
	{
		ConOutCloseHandle();
	}

	// Need to block all requests to output buffer in other threads
	MSectionLockSimple csRead; csRead.Lock(&gpSrv->csReadConsoleInfo, LOCK_READOUTPUT_TIMEOUT);

	CONSOLE_SCREEN_BUFFER_INFO lsbi = {{0,0}};
	// !!! Нас интересует реальное положение дел в консоли,
	//     а не скорректированное функцией MyGetConsoleScreenBufferInfo
	if (!GetConsoleScreenBufferInfo(ghConOut, &lsbi))
	{
		//CS.RelockExclusive();
		//SafeFree(gpStoredOutput);
		return FALSE; // Не смогли получить информацию о консоли...
	}

	CESERVER_CONSAVE_MAP* pData = NULL;
	size_t cbReplySize = sizeof(CESERVER_CONSAVE_MAP) + (lsbi.dwSize.X * lsbi.dwSize.Y * sizeof(pData->Data[0]));
	*out = ExecuteNewCmd(CECMD_CONSOLEFULL, cbReplySize);

	if ((*out) != NULL)
	{
		pData = (CESERVER_CONSAVE_MAP*)*out;
		COORD BufSize = {lsbi.dwSize.X, lsbi.dwSize.Y};
		SMALL_RECT ReadRect = {0, 0, lsbi.dwSize.X-1, lsbi.dwSize.Y-1};

		lbRc = MyReadConsoleOutput(ghConOut, pData->Data, BufSize, ReadRect);

		if (lbRc)
		{
			pData->Succeeded = lbRc;
			pData->MaxCellCount = lsbi.dwSize.X * lsbi.dwSize.Y;
			static DWORD nLastFullIndex;
			pData->CurrentIndex = ++nLastFullIndex;

			// Еще раз считать информацию по консоли (курсор положение и прочее...)
			// За время чтения данных - они могли прокрутиться вверх
			CONSOLE_SCREEN_BUFFER_INFO lsbi2 = {{0,0}};
			if (GetConsoleScreenBufferInfo(ghConOut, &lsbi2))
			{
				// Обновим только курсор, а то юзер может получить черный экран, вместо ожидаемого тектса
				// Если во время "dir c:\ /s" запросить AltConsole - получаем черный экран.
				// Смысл в том, что пока читали строки НИЖЕ курсора - экран уже убежал.
				lsbi.dwCursorPosition = lsbi2.dwCursorPosition;
			}

			pData->info = lsbi;
		}
	}

	return lbRc;
}

BOOL cmd_SetFullScreen(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;
	//ghConOut

	typedef BOOL (WINAPI* SetConsoleDisplayMode_t)(HANDLE, DWORD, PCOORD);
	SetConsoleDisplayMode_t _SetConsoleDisplayMode = (SetConsoleDisplayMode_t)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "SetConsoleDisplayMode");

	size_t cbReplySize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD);
	*out = ExecuteNewCmd(CECMD_CONSOLEFULL, cbReplySize);

	// Need to block all requests to output buffer in other threads
	MSectionLockSimple csRead; csRead.Lock(&gpSrv->csReadConsoleInfo, LOCK_READOUTPUT_TIMEOUT);

	if ((*out) != NULL)
	{
		if (!_SetConsoleDisplayMode)
		{
			(*out)->FullScreenRet.bSucceeded = FALSE;
			(*out)->FullScreenRet.nErrCode = ERROR_INVALID_FUNCTION;
		}
		else
		{
			(*out)->FullScreenRet.bSucceeded = _SetConsoleDisplayMode(ghConOut, 1/*CONSOLE_FULLSCREEN_MODE*/, &(*out)->FullScreenRet.crNewSize);
			if (!(*out)->FullScreenRet.bSucceeded)
				(*out)->FullScreenRet.nErrCode = GetLastError();
		}
	}
	else
	{
		lbRc = FALSE;
	}

	return lbRc;
}

BOOL cmd_SetConColors(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;
	//ghConOut

	// Need to block all requests to output buffer in other threads
	MSectionLockSimple csRead; csRead.Lock(&gpSrv->csReadConsoleInfo, LOCK_READOUTPUT_TIMEOUT);

	size_t cbReplySize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_SETCONSOLORS);
	*out = ExecuteNewCmd(CECMD_CONSOLEFULL, cbReplySize);

	if ((*out) != NULL)
	{
		BOOL bOk = FALSE, bTextChanged = FALSE, bPopupChanged = FALSE, bNeedRepaint = TRUE;
		WORD OldText = 0x07, OldPopup = 0x3E;

		CONSOLE_SCREEN_BUFFER_INFO csbi5 = {};
		BOOL bCsbi5 = GetConsoleScreenBufferInfo(ghConOut, &csbi5);

		if (bCsbi5)
		{
			(*out)->SetConColor.ChangeTextAttr = TRUE;
			(*out)->SetConColor.NewTextAttributes = csbi5.wAttributes;
		}

		if (gnOsVer >= 0x600)
		{
			MY_CONSOLE_SCREEN_BUFFER_INFOEX csbi6 = {sizeof(csbi6)};
			if (apiGetConsoleScreenBufferInfoEx(ghConOut, &csbi6))
			{
				OldText = csbi6.wAttributes;
				OldPopup = csbi6.wPopupAttributes;
				bTextChanged = (csbi6.wAttributes != in.SetConColor.NewTextAttributes);
				bPopupChanged = in.SetConColor.ChangePopupAttr && (csbi6.wPopupAttributes != in.SetConColor.NewPopupAttributes);

				(*out)->SetConColor.ChangePopupAttr = TRUE;
				(*out)->SetConColor.NewPopupAttributes = OldPopup;

				if (!bTextChanged && !bPopupChanged)
				{
					bOk = TRUE;
				}
				else
				{
					csbi6.wAttributes = in.SetConColor.NewTextAttributes;
					csbi6.wPopupAttributes = in.SetConColor.NewPopupAttributes;
					if (bPopupChanged)
					{
						BOOL bIsVisible = IsWindowVisible(ghConWnd);
						bOk = apiSetConsoleScreenBufferInfoEx(ghConOut, &csbi6);
						bNeedRepaint = FALSE;
						if (!bIsVisible && IsWindowVisible(ghConWnd))
						{
							ShowWindow(ghConWnd, SW_HIDE);
						}
						if (bOk)
						{
							(*out)->SetConColor.NewTextAttributes = csbi6.wAttributes;
							(*out)->SetConColor.NewPopupAttributes = csbi6.wPopupAttributes;
						}
					}
					else
					{
						bOk = SetConsoleTextAttribute(ghConOut, in.SetConColor.NewTextAttributes);
						if (bOk)
						{
							(*out)->SetConColor.NewTextAttributes = csbi6.wAttributes;
						}
					}
				}
			}
		}
		else
		{
			if (bCsbi5)
			{
				OldText = csbi5.wAttributes;
				bTextChanged = (csbi5.wAttributes != in.SetConColor.NewTextAttributes);

				if (!bTextChanged)
				{
					bOk = TRUE;
				}
				else
				{
					bOk = SetConsoleTextAttribute(ghConOut, in.SetConColor.NewTextAttributes);
					if (bOk)
					{
						(*out)->SetConColor.NewTextAttributes = in.SetConColor.NewTextAttributes;
					}
				}
			}
		}

		if (bOk && bCsbi5 && bTextChanged && in.SetConColor.ReFillConsole && csbi5.dwSize.X && csbi5.dwSize.Y)
		{
			// Считать из консоли текущие атрибуты (построчно/поблочно)
			// И там, где они совпадают с OldText - заменить на in.SetConColor.NewTextAttributes
			DWORD nMaxLines = max(1,min((8000 / csbi5.dwSize.X),csbi5.dwSize.Y));
			WORD* pnAttrs = (WORD*)malloc(nMaxLines*csbi5.dwSize.X*sizeof(*pnAttrs));
			if (pnAttrs)
			{
				WORD NewText = in.SetConColor.NewTextAttributes;
				COORD crRead = {0,0};
				while (crRead.Y < csbi5.dwSize.Y)
				{
					DWORD nReadLn = min((int)nMaxLines,(csbi5.dwSize.Y-crRead.Y));
					DWORD nReady = 0;
					if (!ReadConsoleOutputAttribute(ghConOut, pnAttrs, nReadLn * csbi5.dwSize.X, crRead, &nReady))
						break;

					bool bStarted = false;
					COORD crFrom = crRead;
					//SHORT nLines = (SHORT)(nReady / csbi5.dwSize.X);
					DWORD i = 0, iStarted = 0, iWritten;
					while (i < nReady)
					{
						if (pnAttrs[i] == OldText)
						{
							if (!bStarted)
							{
								_ASSERT(crRead.X == 0);
								crFrom.Y = (SHORT)(crRead.Y + (i / csbi5.dwSize.X));
								crFrom.X = i % csbi5.dwSize.X;
								iStarted = i;
								bStarted = true;
							}
						}
						else
						{
							if (bStarted)
							{
								bStarted = false;
								if (iStarted < i)
									FillConsoleOutputAttribute(ghConOut, NewText, i - iStarted, crFrom, &iWritten);
							}
						}
						// Next cell checking
						i++;
					}
					// Если хвост остался
					if (bStarted && (iStarted < i))
					{
						FillConsoleOutputAttribute(ghConOut, NewText, i - iStarted, crFrom, &iWritten);
					}

					// Next block
					crRead.Y += (USHORT)nReadLn;
				}
				free(pnAttrs);
			}
		}

		(*out)->SetConColor.ReFillConsole = bOk;
	}
	else
	{
		lbRc = FALSE;
	}

	return lbRc;
}

BOOL cmd_SetConTitle(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;

	size_t cbReplySize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD);
	*out = ExecuteNewCmd(CECMD_SETCONTITLE, cbReplySize);

	if ((*out) != NULL)
	{
		(*out)->dwData[0] = SetTitle(false, (wchar_t*)in.wData);
	}
	else
	{
		lbRc = FALSE;
	}

	return lbRc;
}

BOOL cmd_AltBuffer(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = TRUE;
	CONSOLE_SCREEN_BUFFER_INFO lsbi = {};

	// Need to block all requests to output buffer in other threads
	MSectionLockSimple csRead; csRead.Lock(&gpSrv->csReadConsoleInfo, LOCK_READOUTPUT_TIMEOUT);

	// !!! Нас интересует реальное положение дел в консоли,
	//     а не скорректированное функцией MyGetConsoleScreenBufferInfo
	if (!GetConsoleScreenBufferInfo(ghConOut, &lsbi))
	{
		LogString("cmd_AltBuffer: GetConsoleScreenBufferInfo failed");
		lbRc = FALSE;
	}
	else
	{
		if (gpLogSize)
		{
			char szInfo[100];
			_wsprintfA(szInfo, SKIPLEN(countof(szInfo)) "cmd_AltBuffer: (%u) %s%s%s%s",
				in.AltBuf.BufferHeight,
				!in.AltBuf.AbFlags ? "AbFlags==0" :
				(in.AltBuf.AbFlags & abf_SaveContents) ? " abf_SaveContents" : "",
				(in.AltBuf.AbFlags & abf_RestoreContents) ? " abf_RestoreContents" : "",
				(in.AltBuf.AbFlags & abf_BufferOn) ? " abf_BufferOn" : "",
				(in.AltBuf.AbFlags & abf_BufferOff) ? " abf_BufferOff" : "");
			LogString(szInfo);
		}


		if (in.AltBuf.AbFlags & abf_SaveContents)
		{
			CmdOutputStore();
		}


		//cmd_SetSizeXXX_CmdStartedFinished(CESERVER_REQ& in, CESERVER_REQ** out)
		//CECMD_SETSIZESYNC
		if (in.AltBuf.AbFlags & (abf_BufferOn|abf_BufferOff))
		{
			CESERVER_REQ* pSizeOut = NULL;
			CESERVER_REQ* pSizeIn = ExecuteNewCmd(CECMD_SETSIZESYNC, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_SETSIZE));
			if (!pSizeIn)
			{
				lbRc = FALSE;
			}
			else
			{
				pSizeIn->hdr = in.hdr; // Как-бы фиктивно пришло из приложения
				pSizeIn->hdr.nCmd = CECMD_SETSIZESYNC;
				pSizeIn->hdr.cbSize = sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_SETSIZE);

				pSizeIn->SetSize.rcWindow.Left = pSizeIn->SetSize.rcWindow.Top = 0;
				pSizeIn->SetSize.rcWindow.Right = lsbi.srWindow.Right - lsbi.srWindow.Left;
				pSizeIn->SetSize.rcWindow.Bottom = lsbi.srWindow.Bottom - lsbi.srWindow.Top;
				pSizeIn->SetSize.size.X = lsbi.srWindow.Right - lsbi.srWindow.Left + 1;
				pSizeIn->SetSize.size.Y = lsbi.srWindow.Bottom - lsbi.srWindow.Top + 1;

				if (in.AltBuf.AbFlags & abf_BufferOff)
				{
					pSizeIn->SetSize.nBufferHeight = 0;
				}
				else
				{
					TODO("BufferWidth");
					pSizeIn->SetSize.nBufferHeight = (in.AltBuf.BufferHeight > 0) ? in.AltBuf.BufferHeight : 1000;
				}

				csRead.Unlock();

				if (!cmd_SetSizeXXX_CmdStartedFinished(*pSizeIn, &pSizeOut))
				{
					lbRc = FALSE;
				}
				else
				{
					gpSrv->bAltBufferEnabled = ((in.AltBuf.AbFlags & (abf_BufferOff|abf_SaveContents)) == (abf_BufferOff|abf_SaveContents));
				}
				
				ExecuteFreeResult(pSizeIn);
				ExecuteFreeResult(pSizeOut);
			}
		}


		if (in.AltBuf.AbFlags & abf_RestoreContents)
		{
			CmdOutputRestore(true/*Simple*/);
		}
	}

	size_t cbReplySize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_ALTBUFFER);
	*out = ExecuteNewCmd(CECMD_ALTBUFFER, cbReplySize);
	if ((*out) != NULL)
	{
		(*out)->AltBuf.AbFlags = in.AltBuf.AbFlags;
		TODO("BufferHeight");
		(*out)->AltBuf.BufferHeight = lsbi.dwSize.Y;
	}
	else
	{
		lbRc = FALSE;
	}

	return lbRc;
}

BOOL cmd_AltBufferState(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = TRUE;

	size_t cbReplySize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD);
	*out = ExecuteNewCmd(CECMD_ALTBUFFERSTATE, cbReplySize);
	if ((*out) != NULL)
	{
		(*out)->dwData[0] = gpSrv->bAltBufferEnabled;
	}
	else
	{
		lbRc = FALSE;
	}

	return lbRc;
}

BOOL cmd_ApplyExportVars(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = TRUE;

	ApplyExportEnvVar((LPCWSTR)in.wData);

	size_t cbReplySize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD);
	*out = ExecuteNewCmd(CECMD_EXPORTVARS, cbReplySize);
	if ((*out) != NULL)
	{
		(*out)->dwData[0] = TRUE;
	}
	else
	{
		lbRc = FALSE;
	}

	return lbRc;
}

bool ProcessAltSrvCommand(CESERVER_REQ& in, CESERVER_REQ** out, BOOL& lbRc)
{
	bool lbProcessed = false;
	// Если крутится альтернативный сервер - команду нужно выполнять в нем
	if (gpSrv->dwAltServerPID && (gpSrv->dwAltServerPID != gnSelfPID))
	{
		(*out) = ExecuteSrvCmd(gpSrv->dwAltServerPID, &in, ghConWnd);
		lbProcessed = ((*out) != NULL);
		lbRc = lbProcessed;
	}
	return lbProcessed;
}

BOOL ProcessSrvCommand(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc;
	MCHKHEAP;

	switch(in.hdr.nCmd)
	{
		//case CECMD_GETCONSOLEINFO:
		//case CECMD_REQUESTCONSOLEINFO:
		//{
		//	if (gpSrv->szGuiPipeName[0] == 0) { // Серверный пайп в CVirtualConsole уже должен быть запущен
		//		StringCchPrintf(gpSrv->szGuiPipeName, countof(gpSrv->szGuiPipeName), CEGUIPIPENAME, L".", (DWORD)ghConWnd); // был gnSelfPID
		//	}
		//	_ASSERT(ghConOut && ghConOut!=INVALID_HANDLE_VALUE);
		//	if (ghConOut==NULL || ghConOut==INVALID_HANDLE_VALUE)
		//		return FALSE;
		//	ReloadFullConsoleInfo(TRUE);
		//	MCHKHEAP;
		//	// На запрос из GUI (ProcessSrvCommand)
		//	if (in.hdr.nCmd == CECMD_GETCONSOLEINFO) {
		//		//*out = CreateConsoleInfo(NULL, (in.hdr.nCmd == CECMD_GETFULLINFO));
		//		int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_CONSOLE_MAPPING_HDR);
		//		*out = ExecuteNewCmd(0,nOutSize);
		//		(*out)->ConInfo = gpSrv->pConsole->info;
		//	}
		//	MCHKHEAP;
		//	lbRc = TRUE;
		//} break;
		
		//case CECMD_SETSIZE:
		case CECMD_SETSIZESYNC:
		case CECMD_SETSIZENOSYNC:
		case CECMD_CMDSTARTED:
		case CECMD_CMDFINISHED:
		{
			lbRc = cmd_SetSizeXXX_CmdStartedFinished(in, out);
		} break;
		#if 0
		case CECMD_GETOUTPUT:
		{
			lbRc = cmd_GetOutput(in, out);
		} break;
		#endif
		case CECMD_ATTACH2GUI:
		{
			lbRc = cmd_Attach2Gui(in, out);
		}  break;
		case CECMD_FARLOADED:
		{
			lbRc = cmd_FarLoaded(in, out);
		} break;
		case CECMD_POSTCONMSG:
		{
			lbRc = cmd_PostConMsg(in, out);
		} break;
		case CECMD_SAVEALIASES:
		{
			// Запомнить алиасы
			lbRc = cmd_SaveAliases(in, out);
		} break;
		case CECMD_GETALIASES:
		{
			// Возвращаем запомненные алиасы
			lbRc = cmd_GetAliases(in, out);
		} break;
		case CECMD_SETDONTCLOSE:
		{
			// После детача в фаре команда (например dir) схлопнется, чтобы
			// консоль неожиданно не закрылась...
			lbRc = cmd_SetDontClose(in, out);
		} break;
		case CECMD_ONACTIVATION:
		{
			lbRc = cmd_OnActivation(in, out);
		} break;
		case CECMD_SETWINDOWPOS:
		{
			//SetWindowPos(in.SetWndPos.hWnd, in.SetWndPos.hWndInsertAfter,
			//             in.SetWndPos.X, in.SetWndPos.Y, in.SetWndPos.cx, in.SetWndPos.cy,
			//             in.SetWndPos.uFlags);
			lbRc = cmd_SetWindowPos(in, out);
		} break;
		case CECMD_SETFOCUS:
		{
			lbRc = cmd_SetFocus(in, out);
		} break;
		case CECMD_SETPARENT:
		{
			lbRc = cmd_SetParent(in, out);
		} break;
		case CECMD_SETWINDOWRGN:
		{
			//MySetWindowRgn(&in.SetWndRgn);
			lbRc = cmd_SetWindowRgn(in, out);
		} break;
		case CECMD_DETACHCON:
		{
			lbRc = cmd_DetachCon(in, out);
		} break;
		case CECMD_CMDSTARTSTOP:
		{
			lbRc = cmd_CmdStartStop(in, out);
		} break;
		case CECMD_SETFARPID:
		{
			lbRc = cmd_SetFarPID(in, out);
		} break;
		case CECMD_GUICHANGED:
		{
			lbRc = cmd_GuiChanged(in, out);
		} break;
		case CECMD_TERMINATEPID:
		{
			lbRc = cmd_TerminatePid(in, out);
		} break;
		case CECMD_ATTACHGUIAPP:
		{
			lbRc = cmd_GuiAppAttached(in, out);
		} break;
		case CECMD_ALIVE:
		{
			*out = ExecuteNewCmd(CECMD_ALIVE, sizeof(CESERVER_REQ_HDR));
			lbRc = TRUE;
		} break;
		#ifdef USE_COMMIT_EVENT
		case CECMD_REGEXTCONSOLE:
		{
			lbRc = cmd_RegExtConsole(in, out);
		} break;
		#endif
		case CECMD_FREEZEALTSRV:
		{
			lbRc = cmd_FreezeAltServer(in, out);
		} break;
		case CECMD_CONSOLEFULL:
		{
			lbRc = cmd_LoadFullConsoleData(in, out);
		} break;
		case CECMD_SETFULLSCREEN:
		{
			lbRc = cmd_SetFullScreen(in, out);
		} break;
		case CECMD_SETCONCOLORS:
		{
			lbRc = cmd_SetConColors(in, out);
		} break;
		case CECMD_SETCONTITLE:
		{
			lbRc = cmd_SetConTitle(in, out);
		} break;
		case CECMD_ALTBUFFER:
		{
			// Если крутится альтернативный сервер - команду нужно выполнять в нем
			if (!ProcessAltSrvCommand(in, out, lbRc))
			{
				lbRc = cmd_AltBuffer(in, out);
			}
		} break;
		case CECMD_ALTBUFFERSTATE:
		{
			// Если крутится альтернативный сервер - команду нужно выполнять в нем
			if (!ProcessAltSrvCommand(in, out, lbRc))
			{
				lbRc = cmd_AltBufferState(in, out);
			}
		} break;
		case CECMD_EXPORTVARS:
		{
			lbRc = cmd_ApplyExportVars(in, out);
		} break;
		default:
		{
			// Отлов необработанных
			_ASSERTE(FALSE && "Command was not processed");
			lbRc = FALSE;
		}
	}

	if (gbInRecreateRoot) gbInRecreateRoot = FALSE;

	return lbRc;
}







// Действует аналогично функции WinApi (GetConsoleScreenBufferInfo), но в режиме сервера:
// 1. запрещает (то есть отменяет) максимизацию консольного окна
// 2. корректирует srWindow: сбрасывает горизонтальную прокрутку,
//    а если не обнаружен "буферный режим" - то и вертикальную.
BOOL MyGetConsoleScreenBufferInfo(HANDLE ahConOut, PCONSOLE_SCREEN_BUFFER_INFO apsc)
{
	BOOL lbRc = FALSE, lbSetRc = TRUE;

	//CSection cs(NULL,NULL);
	//MSectionLock CSCS;
	//if (gnRunMode == RM_SERVER)
	//CSCS.Lock(&gpSrv->cChangeSize);
	//cs.Enter(&gpSrv->csChangeSize, &gpSrv->ncsTChangeSize);

	if (gnRunMode == RM_SERVER && ghConEmuWnd && IsWindow(ghConEmuWnd))  // ComSpec окно менять НЕ ДОЛЖЕН!
	{
		// Если юзер случайно нажал максимизацию, когда консольное окно видимо - ничего хорошего не будет
		if (IsZoomed(ghConWnd))
		{
			SendMessage(ghConWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
			DWORD dwStartTick = GetTickCount();

			do
			{
				Sleep(20); // подождем чуть, но не больше секунды
			}
			while(IsZoomed(ghConWnd) && (GetTickCount()-dwStartTick)<=1000);

			Sleep(20); // и еще чуть-чуть, чтобы консоль прочухалась
			// Теперь нужно вернуть (вдруго он изменился) размер буфера консоли
			// Если этого не сделать - размер консоли нельзя УМЕНЬШИТЬ
			RECT rcConPos;
			GetWindowRect(ghConWnd, &rcConPos);
			MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, 1, 1, 1);

			if (gnBufferHeight == 0)
			{
				//specified width and height cannot be less than the width and height of the console screen buffer's window
				lbRc = SetConsoleScreenBufferSize(ghConOut, gcrVisibleSize);
			}
			else
			{
				// Начался ресайз для BufferHeight
				COORD crHeight = {gcrVisibleSize.X, gnBufferHeight};
				MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, 1, 1, 1);
				lbRc = SetConsoleScreenBufferSize(ghConOut, crHeight); // а не crNewSize - там "оконные" размеры
			}

			// И вернуть тот видимый прямоугольник, который был получен в последний раз (успешный раз)
			SetConsoleWindowInfo(ghConOut, TRUE, &gpSrv->sbi.srWindow);
		}
	}

	CONSOLE_SCREEN_BUFFER_INFO csbi = {{0,0}};
	lbRc = GetConsoleScreenBufferInfo(ahConOut, &csbi);
	// Issue 373: wmic
	if (lbRc)
	{
		TODO("Его надо и из ConEmu обновлять");
		if (csbi.dwSize.X && (gnBufferWidth != csbi.dwSize.X))
			gnBufferWidth = csbi.dwSize.X;
		else if (gnBufferWidth == (csbi.srWindow.Right - csbi.srWindow.Left + 1))
			gnBufferWidth = 0;
	}

	if (GetConsoleDisplayMode(&gpSrv->dwDisplayMode) && gpSrv->dwDisplayMode)
	{
		_ASSERTE(!csbi.srWindow.Left && !csbi.srWindow.Top);
		csbi.dwSize.X = csbi.srWindow.Right+1;
		csbi.dwSize.Y = csbi.srWindow.Bottom+1;
	}

	//
	_ASSERTE((csbi.srWindow.Bottom-csbi.srWindow.Top)<200);

	if (lbRc && (gnRunMode == RM_SERVER || gnRunMode == RM_ALTSERVER))  // ComSpec окно менять НЕ ДОЛЖЕН!
	{
		// Перенесено в SetConsoleSize
		//     if (gnBufferHeight) {
		//// Если мы знаем о режиме BufferHeight - можно подкорректировать размер (зачем это было сделано?)
		//         if (gnBufferHeight <= (csbi.dwMaximumWindowSize.Y * 12 / 10))
		//             gnBufferHeight = max(300, (SHORT)(csbi.dwMaximumWindowSize.Y * 12 / 10));
		//     }
		// Если прокрутки быть не должно - по возможности уберем ее, иначе при запуске FAR
		// запустится только в ВИДИМОЙ области
		BOOL lbNeedCorrect = FALSE; // lbNeedUpdateSrvMap = FALSE;

		#ifdef _DEBUG
		SHORT nWidth = (csbi.srWindow.Right - csbi.srWindow.Left + 1);
		SHORT nHeight = (csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
		#endif

		// Приложениям запрещено менять размер видимой области.
		// Размер буфера - могут менять, но не менее чем текущая видимая область
		if (gpSrv && !gpSrv->nRequestChangeSize && (gpSrv->crReqSizeNewSize.X > 0) && (gpSrv->crReqSizeNewSize.Y > 0))
		{
			COORD crMax = MyGetLargestConsoleWindowSize(ghConOut);
			// Это может случиться, если пользователь резко уменьшил разрешение экрана
			if (crMax.X > 0 && crMax.X < gpSrv->crReqSizeNewSize.X)
			{
				gpSrv->crReqSizeNewSize.X = crMax.X;
				TODO("Обновить gcrVisibleSize");
				//lbNeedUpdateSrvMap = TRUE; 
			}
			if (crMax.Y > 0 && crMax.Y < gpSrv->crReqSizeNewSize.Y)
			{
				gpSrv->crReqSizeNewSize.Y = crMax.Y;
				TODO("Обновить gcrVisibleSize");
				//lbNeedUpdateSrvMap = TRUE;
			}

			WARNING("Пока всякую коррекцию убрал. Ибо конфликтует с gpSrv->nRequestChangeSize.");
			//Видимо, нужно сравнивать не с gpSrv->crReqSizeNewSize, а с gcrVisibleSize
#if 0
			if (nWidth != gpSrv->crReqSizeNewSize.X && gpSrv->crReqSizeNewSize.X <= )
			{
				
				csbi.srWindow.Right = min(csbi.dwSize.X, gpSrv->crReqSizeNewSize.X)-1;
				csbi.srWindow.Left = max(0, (csbi.srWindow.Right-gpSrv->crReqSizeNewSize.X+1));
				lbNeedCorrect = TRUE;
			}
			
			if (nHeight != gpSrv->crReqSizeNewSize.Y)
			{
				csbi.srWindow.Bottom = min(csbi.dwSize.Y, gpSrv->crReqSizeNewSize.Y)-1;
				csbi.srWindow.Top = max(0, (csbi.srWindow.Bottom-gpSrv->crReqSizeNewSize.Y+1));
				lbNeedCorrect = TRUE;
			}

			if (lbNeedUpdateSrvMap)
				UpdateConsoleMapHeader();
#endif
		}

#if 0
		// Левая граница
		if (csbi.srWindow.Left > 0)
		{
			lbNeedCorrect = TRUE; csbi.srWindow.Left = 0;
		}

		// Максимальный размер консоли
		if (csbi.dwSize.X > csbi.dwMaximumWindowSize.X)
		{
			// Это может случиться, если пользователь резко уменьшил разрешение экрана
			// или консольное приложение значительно увеличило размер горизонтального буфера (Issue 373: падает при запуске wmic)
			lbNeedCorrect = TRUE;
			if (gpSrv 
				&& (gpSrv->crReqSizeNewSize.X > 0)
				&& gpSrv->crReqSizeNewSize.X <= csbi.dwMaximumWindowSize.X)
			{
				csbi.dwSize.X = gpSrv->crReqSizeNewSize.X;
			}
			else
			{
				csbi.dwSize.X = csbi.dwMaximumWindowSize.X;
			}
			csbi.srWindow.Right = (csbi.dwSize.X - 1);
		}

		if ((csbi.srWindow.Right+1) < csbi.dwSize.X)
		{
			lbNeedCorrect = TRUE; csbi.srWindow.Right = (csbi.dwSize.X - 1);
		}

		BOOL lbBufferHeight = FALSE;
		SHORT nHeight = (csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
		// 2010-11-19 заменил "12 / 10" на "2"
		// 2010-12-12 заменил (csbi.dwMaximumWindowSize.Y * 2) на (nHeight + 1)
		WARNING("Проверить, не будет ли глючить MyGetConsoleScreenBufferInfo если резко уменьшить размер экрана");

		if (csbi.dwSize.Y >= (nHeight + 1))
		{
			lbBufferHeight = TRUE;
		}
		else if (csbi.dwSize.Y > csbi.dwMaximumWindowSize.Y)
		{
			// Это может случиться, если пользователь резко уменьшил разрешение экрана
			lbNeedCorrect = TRUE; csbi.dwSize.Y = csbi.dwMaximumWindowSize.Y;
		}

		if (!lbBufferHeight)
		{
			_ASSERTE((csbi.srWindow.Bottom-csbi.srWindow.Top)<200);

			if (csbi.srWindow.Top > 0)
			{
				lbNeedCorrect = TRUE; csbi.srWindow.Top = 0;
			}

			if ((csbi.srWindow.Bottom+1) < csbi.dwSize.Y)
			{
				lbNeedCorrect = TRUE; csbi.srWindow.Bottom = (csbi.dwSize.Y - 1);
			}
		}
#endif

		WARNING("CorrectVisibleRect пока закомментарен, ибо все равно нифига не делает");

		//if (CorrectVisibleRect(&csbi))
		//	lbNeedCorrect = TRUE;
		if (lbNeedCorrect)
		{
			lbSetRc = SetConsoleWindowInfo(ghConOut, TRUE, &csbi.srWindow);
			_ASSERTE(lbSetRc);
			lbRc = GetConsoleScreenBufferInfo(ahConOut, &csbi);
		}
	}

	// Возвращаем (возможно) скорректированные данные
	*apsc = csbi;
	//cs.Leave();
	//CSCS.Unlock();
#ifdef _DEBUG
#endif

	if (lbRc && gbIsDBCS && csbi.dwSize.X && csbi.dwCursorPosition.X)
	{
		// Issue 577: Chinese display error on Chinese
		// -- GetConsoleScreenBufferInfo возвращает координаты курсора в DBCS, а нам нужен wchar_t !!!
		DWORD nCP = GetConsoleOutputCP();
		if ((nCP != CP_UTF8) && (nCP != CP_UTF7))
		{
			UINT MaxCharSize = 0;
			LPCSTR szLeads = GetCpInfoLeads(nCP, &MaxCharSize);

			if (szLeads && *szLeads && (MaxCharSize > 1))
			{
				_ASSERTE(MaxCharSize==2 || MaxCharSize==4);
				char szLine[512];
				COORD crRead = {0, csbi.dwCursorPosition.Y};
				//120830 - DBCS uses 2 or 4 cells per hieroglyph
				DWORD nRead = 0, cchMax = min((int)countof(szLine)-1, csbi.dwSize.X/* *cpinfo.MaxCharSize */);
				if (ReadConsoleOutputCharacterA(ahConOut, szLine, cchMax, crRead, &nRead) && nRead)
				{
					_ASSERTE(nRead<((int)countof(szLine)-1));
					szLine[nRead] = 0;
					char* pszEnd = szLine+min((int)nRead,csbi.dwCursorPosition.X);
					char* psz = szLine;
					int nXShift = 0;
					while (((psz = strpbrk(psz, szLeads)) != NULL) && (psz < pszEnd))
					{
						nXShift++;
						psz += MaxCharSize;
					}
					_ASSERTE(nXShift <= csbi.dwCursorPosition.X);
					apsc->dwCursorPosition.X = max(0,(csbi.dwCursorPosition.X - nXShift));
				}
			}
		}
	}

	return lbRc;
}



// BufferHeight  - высота БУФЕРА (0 - без прокрутки)
// crNewSize     - размер ОКНА (ширина окна == ширине буфера)
// rNewRect      - для (BufferHeight!=0) определяет new upper-left and lower-right corners of the window
BOOL SetConsoleSize(USHORT BufferHeight, COORD crNewSize, SMALL_RECT rNewRect, LPCSTR asLabel)
{
	_ASSERTE(ghConWnd);
	_ASSERTE(BufferHeight==0 || BufferHeight>crNewSize.Y); // Otherwise - it will be NOT a bufferheight...

	if (!ghConWnd) return FALSE;

//#ifdef _DEBUG
//	if (gnRunMode != RM_SERVER || !gpSrv->DbgInfo.bDebuggerActive)
//	{
//		BOOL bFarInExecute = WaitForSingleObject(ghFarInExecuteEvent, 0) == WAIT_OBJECT_0;
//		if (BufferHeight) {
//			if (!bFarInExecute) {
//				_ASSERTE(BufferHeight && bFarInExecute);
//			}
//		}
//	}
//#endif
	DWORD dwCurThId = GetCurrentThreadId();
	DWORD dwWait = 0;
	DWORD dwErr = 0;

	if ((gnRunMode == RM_SERVER) || (gnRunMode == RM_ALTSERVER))
	{
		// Запомним то, что последний раз установил сервер. пригодится
		gpSrv->nReqSizeBufferHeight = BufferHeight;
		gpSrv->crReqSizeNewSize = crNewSize;
		_ASSERTE(gpSrv->crReqSizeNewSize.X!=0);
		gpSrv->rReqSizeNewRect = rNewRect;
		gpSrv->sReqSizeLabel = asLabel;

		// Ресайз выполнять только в нити RefreshThread. Поэтому если нить другая - ждем...
		if (gpSrv->dwRefreshThread && dwCurThId != gpSrv->dwRefreshThread)
		{
			ResetEvent(gpSrv->hReqSizeChanged);
			gpSrv->nRequestChangeSize++;
			//dwWait = WaitForSingleObject(gpSrv->hReqSizeChanged, REQSIZE_TIMEOUT);
			// Ожидание, пока сработает RefreshThread
			HANDLE hEvents[2] = {ghQuitEvent, gpSrv->hReqSizeChanged};
			DWORD nSizeTimeout = REQSIZE_TIMEOUT;

			#ifdef _DEBUG
			if (IsDebuggerPresent())
				nSizeTimeout = INFINITE;
			#endif

			dwWait = WaitForMultipleObjects(2, hEvents, FALSE, nSizeTimeout);

			if (gpSrv->nRequestChangeSize > 0)
			{
				gpSrv->nRequestChangeSize --;
			}
			else
			{
				_ASSERTE(gpSrv->nRequestChangeSize>0);
			}

			if (dwWait == WAIT_OBJECT_0)
			{
				// ghQuitEvent !!
				return FALSE;
			}

			if (dwWait == (WAIT_OBJECT_0+1))
			{
				return gpSrv->bRequestChangeSizeResult;
			}

			// ?? Может быть стоит самим попробовать?
			return FALSE;
		}
	}

	MSectionLock RCS;
	if (gpSrv->pReqSizeSection && !RCS.Lock(gpSrv->pReqSizeSection, TRUE, 30000))
	{
		_ASSERTE(FALSE);
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (gpLogSize) LogSize(&crNewSize, asLabel);

	_ASSERTE(crNewSize.X>=MIN_CON_WIDTH && crNewSize.Y>=MIN_CON_HEIGHT);

	// Проверка минимального размера
	if (crNewSize.X</*4*/MIN_CON_WIDTH)
		crNewSize.X = /*4*/MIN_CON_WIDTH;

	if (crNewSize.Y</*3*/MIN_CON_HEIGHT)
		crNewSize.Y = /*3*/MIN_CON_HEIGHT;

	BOOL lbNeedChange = TRUE;
	CONSOLE_SCREEN_BUFFER_INFO csbi = {{0,0}};

	if (MyGetConsoleScreenBufferInfo(ghConOut, &csbi))
	{
		// Используется только при (gnBufferHeight == 0)
		lbNeedChange = (csbi.dwSize.X != crNewSize.X) || (csbi.dwSize.Y != crNewSize.Y)
			|| ((csbi.srWindow.Right - csbi.srWindow.Left + 1) != crNewSize.X)
			|| ((csbi.srWindow.Bottom - csbi.srWindow.Top + 1) != crNewSize.Y);
#ifdef _DEBUG

		if (!lbNeedChange)
			BufferHeight = BufferHeight;

#endif
	}


	// Minimum console size
	int curSizeY, curSizeX;
	wchar_t sFontName[LF_FACESIZE];
	bool bCanChangeFontSize = false; // Vista+ only
	if (apiGetConsoleFontSize(ghConOut, curSizeY, curSizeX, sFontName) && curSizeY && curSizeX)
	{
		bCanChangeFontSize = true;
		int nMinY = GetSystemMetrics(SM_CYMIN) - GetSystemMetrics(SM_CYSIZEFRAME) - GetSystemMetrics(SM_CYCAPTION);
		int nMinX = GetSystemMetrics(SM_CXMIN) - 2*GetSystemMetrics(SM_CXSIZEFRAME);
		if ((nMinX > 0) && (nMinY > 0))
		{
			// Теперь прикинуть, какой размер шрифта нам нужен
			int minSizeY = (nMinY / curSizeY);
			int minSizeX = (nMinX / curSizeX);
			if ((minSizeX > crNewSize.X) || (minSizeY > crNewSize.Y))
			{
				apiFixFontSizeForBufferSize(ghConOut, crNewSize);
				apiGetConsoleFontSize(ghConOut, curSizeY, curSizeX, sFontName);
			}
		}
	}


	BOOL lbRc = TRUE;
	RECT rcConPos = {0};
	COORD crMax = MyGetLargestConsoleWindowSize(ghConOut);

	// Если размер превышает допустимый - лучше ничего не делать,
	// иначе получается неприятный эффект при попытке AltEnter:
	// размер окна становится сильно больше чем был, но FullScreen НЕ включается
	//if (crMax.X && crNewSize.X > crMax.X)
	//	crNewSize.X = crMax.X;
	//if (crMax.Y && crNewSize.Y > crMax.Y)
	//	crNewSize.Y = crMax.Y;
	if ((crMax.X && crNewSize.X > crMax.X)
		|| (crMax.Y && crNewSize.Y > crMax.Y))
	{
		if (bCanChangeFontSize && apiFixFontSizeForBufferSize(ghConOut, crNewSize))
		{
			crMax = MyGetLargestConsoleWindowSize(ghConOut);
			if ((crMax.X && crNewSize.X > crMax.X)
				|| (crMax.Y && crNewSize.Y > crMax.Y))
			{
				lbRc = FALSE;
				goto wrap;
			}
		}
		else
		{
			lbRc = FALSE;
			goto wrap;
		}
	}

	// Делаем это ПОСЛЕ MyGetConsoleScreenBufferInfo, т.к. некоторые коррекции размера окна
	// она делает ориентируясь на gnBufferHeight
	gnBufferHeight = BufferHeight;
	// Размер видимой области (слишком большой?)
	_ASSERTE(crNewSize.X<=500 && crNewSize.Y<=300);
	gcrVisibleSize = crNewSize;
	
	if (gnRunMode == RM_SERVER || gnRunMode == RM_ALTSERVER)
		UpdateConsoleMapHeader(); // Обновить pConsoleMap.crLockedVisible

	if (gnBufferHeight)
	{
		// В режиме BufferHeight - высота ДОЛЖНА быть больше допустимого размера окна консоли
		// иначе мы запутаемся при проверках "буферный ли это режим"...
		if (gnBufferHeight <= (csbi.dwMaximumWindowSize.Y * 12 / 10))
			gnBufferHeight = max(300, (SHORT)(csbi.dwMaximumWindowSize.Y * 12 / 10));

		// В режиме cmd сразу уменьшим максимальный FPS
		gpSrv->dwLastUserTick = GetTickCount() - USER_IDLE_TIMEOUT - 1;
	}

	//RECT rcConPos = {0};
	GetWindowRect(ghConWnd, &rcConPos);
	//BOOL lbRc = TRUE;
	//DWORD nWait = 0;

	//if (gpSrv->hChangingSize) {
	//    nWait = WaitForSingleObject(gpSrv->hChangingSize, 300);
	//    _ASSERTE(nWait == WAIT_OBJECT_0);
	//    ResetEvent(gpSrv->hChangingSize);
	//}

	// case: simple mode
	if (BufferHeight == 0)
	{
		if (lbNeedChange)
		{
			// Если этого не сделать - размер консоли нельзя УМЕНЬШИТЬ
			//if (crNewSize.X < csbi.dwSize.X || crNewSize.Y < csbi.dwSize.Y)
			if (crNewSize.X <= (csbi.srWindow.Right-csbi.srWindow.Left) || crNewSize.Y <= (csbi.srWindow.Bottom-csbi.srWindow.Top))
			{
				//MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, 1, 1, 1);
				rNewRect.Left = 0; rNewRect.Top = 0;
				rNewRect.Right = min((crNewSize.X - 1),(csbi.srWindow.Right-csbi.srWindow.Left));
				rNewRect.Bottom = min((crNewSize.Y - 1),(csbi.srWindow.Bottom-csbi.srWindow.Top));

				if (!SetConsoleWindowInfo(ghConOut, TRUE, &rNewRect))
					MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, 1, 1, 1);
			}

			//specified width and height cannot be less than the width and height of the console screen buffer's window
			lbRc = SetConsoleScreenBufferSize(ghConOut, crNewSize);

			if (!lbRc)
				dwErr = GetLastError();

			//TODO: а если правый нижний край вылезет за пределы экрана?
			//WARNING("отключил для теста");
			//MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 1);
			rNewRect.Left = 0; rNewRect.Top = 0;
			rNewRect.Right = crNewSize.X - 1;
			rNewRect.Bottom = crNewSize.Y - 1;
			SetConsoleWindowInfo(ghConOut, TRUE, &rNewRect);
		}
	}
	else
	{
		// Начался ресайз для BufferHeight
		COORD crHeight = {crNewSize.X, BufferHeight};
		SMALL_RECT rcTemp = {0};

		if (!rNewRect.Top && !rNewRect.Bottom && !rNewRect.Right)
			rNewRect = csbi.srWindow;

		// Подправим будующую видимую область
		if (csbi.dwSize.Y == (csbi.srWindow.Bottom - csbi.srWindow.Top + 1))
		{
			// Прокрутки сейчас нет, оставляем .Top без изменений!
		}
		// При изменении высоты буфера (если он уже был включен), нужно скорректировать новую видимую область
		else if (rNewRect.Bottom >= (csbi.dwSize.Y - (csbi.srWindow.Bottom - csbi.srWindow.Top)))
		{
			// Считаем, что рабочая область прижата к низу экрана. Нужно подвинуть .Top
			int nBottomLines = (csbi.dwSize.Y - csbi.srWindow.Bottom - 1); // Сколько строк сейчас снизу от видимой области?
			SHORT nTop = BufferHeight - crNewSize.Y - nBottomLines;
			rNewRect.Top = (nTop > 0) ? nTop : 0;
			// .Bottom подправится ниже, перед последним SetConsoleWindowInfo
		}
		else
		{
			// Считаем, что верх рабочей области фиксирован, коррекция не требуется
		}

		// Если этого не сделать - размер консоли нельзя УМЕНЬШИТЬ
		if (crNewSize.X <= (csbi.srWindow.Right-csbi.srWindow.Left)
		        || crNewSize.Y <= (csbi.srWindow.Bottom-csbi.srWindow.Top))
		{
			//MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, 1, 1, 1);
			rcTemp.Left = 0;
			WARNING("А при уменшении высоты, тащим нижнюю границе окна вверх, Top глючить не будет?");
			rcTemp.Top = max(0,(csbi.srWindow.Bottom-crNewSize.Y+1));
			rcTemp.Right = min((crNewSize.X - 1),(csbi.srWindow.Right-csbi.srWindow.Left));
			rcTemp.Bottom = min((BufferHeight - 1),(rcTemp.Top+crNewSize.Y-1));//(csbi.srWindow.Bottom-csbi.srWindow.Top)); //-V592

			if (!SetConsoleWindowInfo(ghConOut, TRUE, &rcTemp))
				MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, 1, 1, 1);
		}

		//MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, 1, 1, 1);
		//WaitForSingleObject(ghConOut, 200);
		//Sleep(100);
		/*if (gOSVer.dwMajorVersion == 6 && gOSVer.dwMinorVersion == 1) {
			const SHORT nMaxBuf = 600;
			if (crHeight.Y >= nMaxBuf)
				crHeight.Y = nMaxBuf;
		}*/
		lbRc = SetConsoleScreenBufferSize(ghConOut, crHeight); // а не crNewSize - там "оконные" размеры

		if (!lbRc)
			dwErr = GetLastError();

		//}
		//окошко раздвигаем только по ширине!
		//RECT rcCurConPos = {0};
		//GetWindowRect(ghConWnd, &rcCurConPos); //X-Y новые, но высота - старая
		//MoveWindow(ghConWnd, rcCurConPos.left, rcCurConPos.top, GetSystemMetrics(SM_CXSCREEN), rcConPos.bottom-rcConPos.top, 1);
		// Последняя коррекция видимой области.
		// Левую граница - всегда 0 (горизонтальную прокрутку не поддерживаем)
		// Вертикальное положение - пляшем от rNewRect.Top
		rNewRect.Left = 0;
		rNewRect.Right = crHeight.X-1;
		rNewRect.Bottom = min((crHeight.Y-1), (rNewRect.Top+gcrVisibleSize.Y-1)); //-V592
		_ASSERTE((rNewRect.Bottom-rNewRect.Top)<200);
		SetConsoleWindowInfo(ghConOut, TRUE, &rNewRect);
	}

	//if (gpSrv->hChangingSize) { // во время запуска ConEmuC
	//    SetEvent(gpSrv->hChangingSize);
	//}

wrap:
	gpSrv->bRequestChangeSizeResult = lbRc;

	if ((gnRunMode == RM_SERVER) && gpSrv->hRefreshEvent)
	{
		//gpSrv->bForceFullSend = TRUE;
		SetEvent(gpSrv->hRefreshEvent);
	}

	//cs.Leave();
	//CSCS.Unlock();
	UNREFERENCED_PARAMETER(dwErr);
	return lbRc;
}

#if defined(__GNUC__)
extern "C" {
	BOOL WINAPI HandlerRoutine(DWORD dwCtrlType);
};
#endif

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
	//PRINT_COMSPEC(L"HandlerRoutine triggered. Event type=%i\n", dwCtrlType);
	if ((dwCtrlType == CTRL_CLOSE_EVENT)
		|| (dwCtrlType == CTRL_LOGOFF_EVENT)
		|| (dwCtrlType == CTRL_SHUTDOWN_EVENT))
	{
		PRINT_COMSPEC(L"Console about to be closed\n", 0);
		WARNING("Тут бы подождать немного, пока другие консольные процессы завершатся... а то таб закрывается раньше времени");
		gbInShutdown = TRUE;

		if (gbInExitWaitForKey)
			gbStopExitWaitForKey = TRUE;
		
		// Остановить отладчик, иначе отлаживаемый процесс тоже схлопнется
		if (gpSrv->DbgInfo.bDebuggerActive)
		{
			if (pfnDebugActiveProcessStop) pfnDebugActiveProcessStop(gpSrv->dwRootProcess);

			gpSrv->DbgInfo.bDebuggerActive = FALSE;
		}
		else
		{
			// trick to let ConsoleMain2() finish correctly
			ExitThread(1);
			//return TRUE;
		
			//#ifdef _DEBUG
			//wchar_t szTitle[128]; wsprintf(szTitle, L"ConEmuC, PID=%u", GetCurrentProcessId());
			////MessageBox(NULL, L"CTRL_CLOSE_EVENT in ConEmuC", szTitle, MB_SYSTEMMODAL);
			//#endif
			//DWORD nWait = WaitForSingleObject(ghExitQueryEvent, CLOSE_CONSOLE_TIMEOUT);
			//if (nWait == WAIT_OBJECT_0)
			//{
			//	#ifdef _DEBUG
			//	OutputDebugString(L"All console processes was terminated\n");
			//	#endif
			//}
			//else
			//{
			//	// Поскольку мы (сервер) сейчас свалимся, то нужно показать
			//	// консольное окно, раз в нем остались какие-то процессы
			//	EmergencyShow();
			//}
		}
	}
	else if ((dwCtrlType == CTRL_C_EVENT) || (dwCtrlType == CTRL_BREAK_EVENT))
	{
		if (gbTerminateOnCtrlBreak)
		{
			PRINT_COMSPEC(L"Ctrl+Break recieved, server will be terminated\n", 0);
			gbInShutdown = TRUE;
		}
		else if (gpSrv->DbgInfo.bDebugProcess)
		{
			DWORD nWait = WaitForSingleObject(gpSrv->hRootProcess, 0);
			#ifdef _DEBUG
			DWORD nErr = GetLastError();
			#endif
			if (nWait == WAIT_OBJECT_0)
			{
				_ASSERTE(gbTerminateOnCtrlBreak==FALSE);
				PRINT_COMSPEC(L"Ctrl+Break recieved, debgger will be stopped\n", 0);
				//if (pfnDebugActiveProcessStop) pfnDebugActiveProcessStop(gpSrv->dwRootProcess);
				//gpSrv->DbgInfo.bDebuggerActive = FALSE;
				//gbInShutdown = TRUE;
				SetTerminateEvent(ste_HandlerRoutine);
			}
			else
			{
				GenerateMiniDumpFromCtrlBreak();
			}
		}
	}

	return TRUE;
}

int GetProcessCount(DWORD *rpdwPID, UINT nMaxCount)
{
	if (!rpdwPID || (nMaxCount < 3))
	{
		_ASSERTE(rpdwPID && (nMaxCount >= 3));
		return gpSrv->nProcessCount;
	}


	UINT nRetCount = 0;

	rpdwPID[nRetCount++] = gnSelfPID;

	// Windows 7 and higher: there is "conhost.exe"
	if (gnOsVer >= 0x0601)
	{
		#if 0
		typedef BOOL (WINAPI* GetNamedPipeServerProcessId_t)(HANDLE Pipe,PULONG ServerProcessId);
		HMODULE hKernel = GetModuleHandle(L"kernel32.dll");
		GetNamedPipeServerProcessId_t GetNamedPipeServerProcessId_f = hKernel
			? (GetNamedPipeServerProcessId_t)GetProcAddress(hKernel, "GetNamedPipeServerProcessId") : NULL;
		HANDLE hOut; BOOL bSrv = FALSE; ULONG nSrvPid = 0;
		_ASSERTE(FALSE && "calling GetNamedPipeServerProcessId_f");
		if (GetNamedPipeServerProcessId_f)
		{
			hOut = (HANDLE)ghConOut;
			if (hOut)
			{
				bSrv = GetNamedPipeServerProcessId_f(hOut, &nSrvPid);
			}
		}
		#endif

		if (!gpSrv->nConhostPID)
		{
			// Найти порожденный conhost.exe
			HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (h && (h != INVALID_HANDLE_VALUE))
			{
				// Учтем альтернативные серверы (Far/Telnet/...)
				DWORD nSrvPID = gpSrv->dwMainServerPID ? gpSrv->dwMainServerPID : gnSelfPID;
				PROCESSENTRY32 PI = {sizeof(PI)};
				if (Process32First(h, &PI))
				{
					do {
						if ((PI.th32ParentProcessID == nSrvPID)
							&& (lstrcmpi(PI.szExeFile, L"conhost.exe") == 0))
						{
							gpSrv->nConhostPID = PI.th32ProcessID;
							break;
						}
					} while (Process32Next(h, &PI));
				}

				CloseHandle(h);
			}

			if (!gpSrv->nConhostPID)
				gpSrv->nConhostPID = (UINT)-1;
		}

		if (gpSrv->nConhostPID && (gpSrv->nConhostPID != (UINT)-1))
		{
			rpdwPID[nRetCount++] = gpSrv->nConhostPID;
		}
	}


	MSectionLock CS;
	UINT nCurCount = 0;
	if (CS.Lock(gpSrv->csProc, TRUE/*abExclusive*/, 200))
	{
		nCurCount = gpSrv->nProcessCount;

		for (INT_PTR i1 = (nCurCount-1); (i1 >= 0) && (nRetCount < nMaxCount); i1--)
		{
			DWORD PID = gpSrv->pnProcesses[i1];
			if (PID && PID != gnSelfPID)
			{
				rpdwPID[nRetCount++] = PID;
			}
		}
	}

	for (size_t i = nRetCount; i < nMaxCount; i++)
	{
		rpdwPID[i] = 0;
	}

	//if (nSize > nMaxCount)
	//{
	//	memset(rpdwPID, 0, sizeof(DWORD)*nMaxCount);
	//	rpdwPID[0] = gnSelfPID;

	//	for(int i1=0, i2=(nMaxCount-1); i1<(int)nSize && i2>0; i1++, i2--)
	//		rpdwPID[i2] = gpSrv->pnProcesses[i1]; //-V108

	//	nSize = nMaxCount;
	//}
	//else
	//{
	//	memmove(rpdwPID, gpSrv->pnProcesses, sizeof(DWORD)*nSize);

	//	for (UINT i=nSize; i<nMaxCount; i++)
	//		rpdwPID[i] = 0; //-V108
	//}

	_ASSERTE(rpdwPID[0]);
	return nRetCount;
}

#ifdef CRTPRINTF
WARNING("Можно облегчить... заменить на wvsprintf");
//void _printf(LPCSTR asFormat, ...)
//{
//    va_list argList; va_start(argList, an_StrResId);
//    char szError[2000]; -- только нужно учесть длину %s
//    wvsprintf(szError, asFormat, argList);
//}

void _printf(LPCSTR asFormat, DWORD dw1, DWORD dw2, LPCWSTR asAddLine)
{
	char szError[MAX_PATH];
	_wsprintfA(szError, SKIPLEN(countof(szError)) asFormat, dw1, dw2);
	_printf(szError);

	if (asAddLine)
	{
		_wprintf(asAddLine);
		_printf("\n");
	}
}

void _printf(LPCSTR asFormat, DWORD dwErr, LPCWSTR asAddLine)
{
	char szError[MAX_PATH];
	_wsprintfA(szError, SKIPLEN(countof(szError)) asFormat, dwErr);
	_printf(szError);
	_wprintf(asAddLine);
	_printf("\n");
}

void _printf(LPCSTR asFormat, DWORD dwErr)
{
	char szError[MAX_PATH];
	_wsprintfA(szError, SKIPLEN(countof(szError)) asFormat, dwErr);
	_printf(szError);
}

void _printf(LPCSTR asBuffer)
{
	if (!asBuffer) return;

	int nAllLen = lstrlenA(asBuffer);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwWritten = 0;
	WriteConsoleA(hOut, asBuffer, nAllLen, &dwWritten, 0);
}

#endif

void _wprintf(LPCWSTR asBuffer)
{
	if (!asBuffer) return;

	int nAllLen = lstrlenW(asBuffer);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwWritten = 0;
	WriteConsoleW(hOut, asBuffer, nAllLen, &dwWritten, 0);
	
	//UINT nOldCP = GetConsoleOutputCP();
	//char* pszOEM = (char*)malloc(nAllLen+1);
	//
	//if (pszOEM)
	//{
	//	WideCharToMultiByte(nOldCP,0, asBuffer, nAllLen, pszOEM, nAllLen, 0,0);
	//	pszOEM[nAllLen] = 0;
	//	WriteFile(hOut, pszOEM, nAllLen, &dwWritten, 0);
	//	free(pszOEM);
	//}
	//else
	//{
	//	WriteFile(hOut, asBuffer, nAllLen*2, &dwWritten, 0);
	//}
}

void DisableAutoConfirmExit(BOOL abFromFarPlugin)
{
	// Консоль моглы быть приаттачена к GUI в тот момент, когда сервер ожидает
	// от юзера подтверждение закрытия консоли
	if (!gbInExitWaitForKey)
	{
		_ASSERTE(gnConfirmExitParm==0 || abFromFarPlugin);
		gbAutoDisableConfirmExit = FALSE; gbAlwaysConfirmExit = FALSE;
		// менять nProcessStartTick не нужно. проверка только по флажкам
		//gpSrv->nProcessStartTick = GetTickCount() - 2*CHECK_ROOTSTART_TIMEOUT;
	}
}

//BOOL IsUserAdmin()
//{
//	OSVERSIONINFO osv = {sizeof(OSVERSIONINFO)};
//	GetVersionEx(&osv);
//	// Проверять нужно только для висты, чтобы на XP лишний "Щит" не отображался
//	if (osv.dwMajorVersion < 6)
//		return FALSE;
//
//	BOOL b;
//	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
//	PSID AdministratorsGroup;
//
//	b = AllocateAndInitializeSid(
//		&NtAuthority,
//		2,
//		SECURITY_BUILTIN_DOMAIN_RID,
//		DOMAIN_ALIAS_RID_ADMINS,
//		0, 0, 0, 0, 0, 0,
//		&AdministratorsGroup);
//	if (b)
//	{
//		if (!CheckTokenMembership(NULL, AdministratorsGroup, &b))
//		{
//			b = FALSE;
//		}
//		FreeSid(AdministratorsGroup);
//	}
//	return(b);
//}

bool IsKeyboardLayoutChanged(DWORD* pdwLayout)
{
	bool bChanged = false;

	if (pfnGetConsoleKeyboardLayoutName)
	{
		wchar_t szCurKeybLayout[32] = L"";

		//#ifdef _DEBUG
		//wchar_t szDbgKeybLayout[KL_NAMELENGTH/*==9*/];
		//BOOL lbGetRC = GetKeyboardLayoutName(szDbgKeybLayout); // -- не дает эффекта, поскольку "на процесс", а не на консоль
		//#endif
		// Возвращает строку в виде "00000419" -- может тут 16 цифр?
		if (pfnGetConsoleKeyboardLayoutName(szCurKeybLayout))
		{
			if (lstrcmpW(szCurKeybLayout, gpSrv->szKeybLayout))
			{
				#ifdef _DEBUG
				wchar_t szDbg[128];
				_wsprintf(szDbg, SKIPLEN(countof(szDbg))
				          L"ConEmuC: InputLayoutChanged (GetConsoleKeyboardLayoutName returns) '%s'\n",
				          szCurKeybLayout);
				OutputDebugString(szDbg);
				#endif

				if (gpLogSize)
				{
					char szInfo[128]; wchar_t szWide[128];
					_wsprintf(szWide, SKIPLEN(countof(szWide)) L"ConEmuC: ConsKeybLayout changed from %s to %s", gpSrv->szKeybLayout, szCurKeybLayout);
					WideCharToMultiByte(CP_ACP,0,szWide,-1,szInfo,128,0,0);
					LogString(szInfo);
				}

				// Сменился
				wcscpy_c(gpSrv->szKeybLayout, szCurKeybLayout);
				bChanged = true;
			}
		}
	}

	if (pdwLayout)
	{
		wchar_t *pszEnd = NULL; //szCurKeybLayout+8;
		//WARNING("BUGBUG: 16 цифр не вернет"); -- тут именно 8 цифт. Это LayoutNAME, а не string(HKL)
		// LayoutName: "00000409", "00010409", ...
		// А HKL от него отличается, так что передаем DWORD
		// HKL в x64 выглядит как: "0x0000000000020409", "0xFFFFFFFFF0010409"
		*pdwLayout = wcstoul(gpSrv->szKeybLayout, &pszEnd, 16);
	}

	return bChanged;
}

//WARNING("BUGBUG: x64 US-Dvorak"); - done
void CheckKeyboardLayout()
{
	DWORD dwLayout = 0;

	//WARNING("BUGBUG: 16 цифр не вернет"); -- тут именно 8 цифт. Это LayoutNAME, а не string(HKL)
	// LayoutName: "00000409", "00010409", ...
	// А HKL от него отличается, так что передаем DWORD
	// HKL в x64 выглядит как: "0x0000000000020409", "0xFFFFFFFFF0010409"

	if (IsKeyboardLayoutChanged(&dwLayout))
	{
		// Сменился, Отошлем в GUI
		CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_LANGCHANGE,sizeof(CESERVER_REQ_HDR)+sizeof(DWORD)); //-V119

		if (pIn)
		{
			//memmove(pIn->Data, &dwLayout, 4);
			pIn->dwData[0] = dwLayout;

			CESERVER_REQ* pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);

			ExecuteFreeResult(pOut);
			ExecuteFreeResult(pIn);
		}
	}
}


/*
LPVOID calloc(size_t nCount, size_t nSize)
{
	#ifdef _DEBUG
	//HeapValidate(ghHeap, 0, NULL);
	#endif

	size_t nWhole = nCount * nSize;
	_ASSERTE(nWhole>0);
	LPVOID ptr = HeapAlloc ( ghHeap, HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY, nWhole );

	#ifdef HEAP_LOGGING
	wchar_t szDbg[64];
	_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"%i: ALLOCATED   0x%08X..0x%08X   (%i bytes)\n", GetCurrentThreadId(), (DWORD)ptr, ((DWORD)ptr)+nWhole, nWhole);
	DEBUGSTR(szDbg);
	#endif

	#ifdef _DEBUG
	HeapValidate(ghHeap, 0, NULL);
	if (ptr)
	{
		gnHeapUsed += nWhole;
		if (gnHeapMax < gnHeapUsed)
			gnHeapMax = gnHeapUsed;
	}
	#endif

	return ptr;
}

void free(LPVOID ptr)
{
	if (ptr && ghHeap)
	{
		#ifdef _DEBUG
		//HeapValidate(ghHeap, 0, NULL);
		size_t nMemSize = HeapSize(ghHeap, 0, ptr);
		#endif
		#ifdef HEAP_LOGGING
		wchar_t szDbg[64];
		_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"%i: FREE BLOCK  0x%08X..0x%08X   (%i bytes)\n", GetCurrentThreadId(), (DWORD)ptr, ((DWORD)ptr)+nMemSize, nMemSize);
		DEBUGSTR(szDbg);
		#endif

		HeapFree ( ghHeap, 0, ptr );

		#ifdef _DEBUG
		HeapValidate(ghHeap, 0, NULL);
		if (gnHeapUsed > nMemSize)
			gnHeapUsed -= nMemSize;
		#endif
	}
}
*/

/* Используются как extern в ConEmuCheck.cpp */
/*
LPVOID _calloc(size_t nCount,size_t nSize) {
	return calloc(nCount,nSize);
}
LPVOID _malloc(size_t nCount) {
	return calloc(nCount,1);
}
void   _free(LPVOID ptr) {
	free(ptr);
}
*/

/*
void * __cdecl operator new(size_t _Size)
{
	void * p = calloc(_Size,1);
	return p;
}

void __cdecl operator delete(void *p)
{
	free(p);
}
*/
