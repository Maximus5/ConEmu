
/*
Copyright (c) 2009-2015 Maximus5
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
//	#define SHOW_ADMIN_STARTED_MSGBOX
//	#define SHOW_MAIN_MSGBOX
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
//	#define SHOW_ROOT_STARTED
//	#define SHOW_ASYNC_STARTED
	#define WINE_PRINT_PROC_INFO
//	#define USE_PIPE_DEBUG_BOXES
//	#define SHOW_SETCONTITLE_MSGBOX

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
#define DEBUGSTARTSTOPBOX(x) //MessageBox(NULL, x, WIN3264TEST(L"ConEmuC",L"ConEmuC64"), MB_ICONINFORMATION|MB_SYSTEMMODAL)
#define DEBUGSTRFIN(x) DEBUGSTR(x)
#define DEBUGSTRCP(x) DEBUGSTR(x)

//#define SHOW_INJECT_MSGBOX

#include "ConEmuC.h"
#include "../common/CmdLine.h"
#include "../common/ConsoleAnnotation.h"
#include "../common/ConsoleMixAttr.h"
#include "../common/ConsoleRead.h"
#include "../common/EmergencyShow.h"
#include "../common/execute.h"
#include "../common/HkFunc.h"
#include "../common/MArray.h"
#include "../common/MMap.h"
#include "../common/MSectionSimple.h"
#include "../common/MWow64Disable.h"
#include "../common/ProcessSetEnv.h"
#include "../common/ProcessData.h"
#include "../common/RConStartArgs.h"
#include "../common/SetEnvVar.h"
#include "../common/StartupEnvEx.h"
#include "../common/WConsole.h"
#include "../common/WFiles.h"
#include "../common/WThreads.h"
#include "../common/WUser.h"
#include "../ConEmu/version.h"
#include "../ConEmuHk/Injects.h"
#include "ConProcess.h"
#include "ConsoleHelp.h"
#include "Debugger.h"
#include "DownloaderCall.h"
#include "UnicodeTest.h"


#ifndef __GNUC__
#pragma comment(lib, "shlwapi.lib")
#endif


WARNING("Обязательно после запуска сделать apiSetForegroundWindow на GUI окно, если в фокусе консоль");
WARNING("Обязательно получить код и имя родительского процесса");

#ifdef USEPIPELOG
namespace PipeServerLogger
{
	Event g_events[BUFFER_SIZE];
	LONG g_pos = -1;
}
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
FGetConsoleDisplayMode pfnGetConsoleDisplayMode = NULL;


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
wchar_t gsModuleName[32] = L"";
BOOL    gbTerminateOnExit = FALSE;
bool    gbPrefereSilentMode = false;
//HANDLE  ghConIn = NULL, ghConOut = NULL;
HWND    ghConWnd = NULL;
DWORD   gnConEmuPID = 0; // PID of ConEmu[64].exe (ghConEmuWnd)
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
#define EPS_WAITING4PROCESS  550
#define EPS_ROOTPROCFINISHED 560
SetTerminateEventPlace gTerminateEventPlace = ste_None;
HANDLE  ghQuitEvent = NULL;
bool    gbQuit = false;
BOOL    gbInShutdown = FALSE;
BOOL    gbInExitWaitForKey = FALSE;
BOOL    gbStopExitWaitForKey = FALSE;
BOOL    gbCtrlBreakStopWaitingShown = FALSE;
BOOL    gbTerminateOnCtrlBreak = FALSE;
BOOL    gbPrintRetErrLevel = FALSE; // Вывести в StdOut код завершения процесса (RM_COMSPEC в основном)
bool    gbSkipHookersCheck = false;
int     gnConfirmExitParm = 0; // 1 - CONFIRM, 2 - NOCONFIRM
BOOL    gbAlwaysConfirmExit = FALSE;
BOOL    gbAutoDisableConfirmExit = FALSE; // если корневой процесс проработал достаточно (10 сек) - будет сброшен gbAlwaysConfirmExit
BOOL    gbRootAliveLess10sec = FALSE; // корневой процесс проработал менее CHECK_ROOTOK_TIMEOUT
int     gbRootWasFoundInCon = 0;
BOOL    gbComspecInitCalled = FALSE;
AttachModeEnum gbAttachMode = am_None; // сервер запущен НЕ из conemu.exe (а из плагина, из CmdAutoAttach, или -new_console, или /GUIATTACH, или /ADMIN)
BOOL    gbAlienMode = FALSE;  // сервер НЕ является владельцем консоли (корневым процессом этого консольного окна)
BOOL    gbDefTermCall = FALSE; // сервер запущен из DefTerm приложения (*.vshost.exe), конcоль может быть скрыта
BOOL    gbCreatingHiddenConsole = FALSE; // Используется для "тихого" открытия окна RealConsole из *.vshost.exe
BOOL    gbForceHideConWnd = FALSE;
DWORD   gdwMainThreadId = 0;
wchar_t* gpszRunCmd = NULL;
wchar_t* gpszRootExe = NULL; // may be set with '/ROOTEXE' switch if used with '/TRMPID'. full path to root exe
wchar_t* gpszForcedTitle = NULL;
CProcessEnvCmd* gpSetEnv = NULL;
LPCWSTR gpszCheck4NeedCmd = NULL; // Для отладки
wchar_t gszComSpec[MAX_PATH+1] = {0};
bool    gbRunInBackgroundTab = false;
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
BOOL  gbAsyncRun = FALSE;
int   gnCmdUnicodeMode = 0;
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
bool gbIsDownloading = false;


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

#ifdef _DEBUG
wchar_t* gpszPrevConTitle = NULL;
#endif

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


//extern UINT_PTR gfnLoadLibrary;
//UINT gnMsgActivateCon = 0;
UINT gnMsgSwitchCon = 0;
UINT gnMsgHookedKey = 0;
//UINT gnMsgConsoleHookedKey = 0;

#if defined(__GNUC__)
extern "C"
#endif
BOOL WINAPI DllMain(HINSTANCE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		{
			ghOurModule = (HMODULE)hModule;
			ghWorkingModule = (u64)hModule;

			DWORD nImageBits = WIN3264TEST(32,64), nImageSubsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
			GetImageSubsystem(nImageSubsystem,nImageBits);
			if (nImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI)
				ghConWnd = NULL;
			else
				ghConWnd = GetConEmuHWND(2);

			gnSelfPID = GetCurrentProcessId();
			gfnSearchAppPaths = SearchAppPaths;

			wchar_t szExeName[MAX_PATH] = L"", szDllName[MAX_PATH] = L"";
			GetModuleFileName(NULL, szExeName, countof(szExeName));
			GetModuleFileName((HMODULE)hModule, szDllName, countof(szDllName));
			if (IsConsoleServer(PointToName(szExeName)))
				wcscpy_c(gsModuleName, WIN3264TEST(L"ConEmuC",L"ConEmuC64"));
			else
				wcscpy_c(gsModuleName, WIN3264TEST(L"ConEmuCD",L"ConEmuCD64"));

			#ifdef _DEBUG
			HANDLE hProcHeap = GetProcessHeap();
			gAllowAssertThread = am_Pipe;
			#endif

			#ifdef SHOW_STARTED_MSGBOX
			if (!IsDebuggerPresent())
			{
				char szMsg[128] = ""; msprintf(szMsg, countof(szMsg), WIN3264TEST("ConEmuCD.dll","ConEmuCD64.dll") " loaded, PID=%u, TID=%u", GetCurrentProcessId(), GetCurrentThreadId());
				char szFile[MAX_PATH] = ""; GetModuleFileNameA(NULL, szFile, countof(szFile));
				MessageBoxA(NULL, szMsg, PointToName(szFile), 0);
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
			gpStartEnv = LoadStartupEnvEx::Create();
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
				ConInfo.InitName(CECONMAPNAME, LODWORD(ghConWnd)); //-V205
				CESERVER_CONSOLE_MAPPING_HDR *pInfo = ConInfo.Open();
				if (pInfo)
				{
					if (pInfo->cbSize >= sizeof(CESERVER_CONSOLE_MAPPING_HDR))
					{
						if (pInfo->hConEmuRoot && IsWindow(pInfo->hConEmuRoot))
						{
							SetConEmuWindows(pInfo->hConEmuRoot, pInfo->hConEmuWndDc, pInfo->hConEmuWndBack);
						}
						if (pInfo->nServerPID && pInfo->nServerPID != gnSelfPID)
						{
							gnMainServerPID = pInfo->nServerPID;
							gnAltServerPID = pInfo->nAltServerPID;
						}

						gbLogProcess = (pInfo->nLoggingType == glt_Processes);
						if (gbLogProcess)
						{
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

			#ifdef _DEBUG
			if ((gnRunMode == RM_SERVER) && (nExitPlaceStep == EPS_WAITING4PROCESS/*550*/))
			{
				// Это происходило после Ctrl+C если не был установлен HandlerRoutine
				// Ни _ASSERT ни DebugBreak здесь позвать уже не получится - все закрывается и игнорируется
				OutputDebugString(L"!!! Server was abnormally terminated !!!\n");
				LogString("!!! Server was abnormally terminated !!!\n");
			}
			#endif

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

//#if defined(GNUCRTSTARTUP)
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

#ifdef SHOW_ASYNC_STARTED
	if (gbAsyncRun)
	{
		_ASSERTE(FALSE && "Async startup requested");
	}
#endif

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
	CEStr fnDescr(lstrmerge(L"createProcess App={", lpApplicationName, L"} Cmd={", lpCommandLine, L"}"));
	LogFunction(fnDescr);

	MWow64Disable wow;
	if (!abSkipWowChange)
		wow.Disable();

	// %PATHS% from [HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths]
	// must be already processed in IsNeedCmd >> FileExistsSearch >> SearchAppPaths

	DWORD nStartTick = GetTickCount();

#if defined(SHOW_STARTED_PRINT_LITE)
	if (gnRunMode == RM_SERVER)
	{
		_printf("Starting root: ");
		_wprintf(lpCommandLine ? lpCommandLine : lpApplicationName ? lpApplicationName : L"<NULL>");
	}
#endif

	SetLastError(0);

	BOOL lbRc = CreateProcess(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	DWORD dwErr = GetLastError();
	DWORD nStartDuration = GetTickCount() - nStartTick;

	wchar_t szRunRc[80];
	if (lbRc)
		_wsprintf(szRunRc, SKIPCOUNT(szRunRc) L"Succeeded (%u ms) PID=%u", nStartDuration, lpProcessInformation->dwProcessId);
	else
		_wsprintf(szRunRc, SKIPCOUNT(szRunRc) L"Failed (%u ms) Code=%u(x%04X)", nStartDuration, dwErr, dwErr);
	LogFunction(szRunRc);

#if defined(SHOW_STARTED_PRINT_LITE)
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

	SetLastError(dwErr);
	return lbRc;
}

// Возвращает текст с информацией о пути к сохраненному дампу
// DWORD CreateDumpForReport(LPEXCEPTION_POINTERS ExceptionInfo, wchar_t (&szFullInfo)[1024], LPWSTR pszComment = NULL);
#include "../common/Dump.h"

bool CopyToClipboard(LPCWSTR asText)
{
	if (!asText)
		return false;

	bool bCopied = false;

	if (OpenClipboard(NULL))
	{
		DWORD cch = lstrlen(asText);
		HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (cch + 1) * sizeof(*asText));
		if (hglbCopy)
		{
			wchar_t* lptstrCopy = (wchar_t*)GlobalLock(hglbCopy);
			if (lptstrCopy)
			{
				_wcscpy_c(lptstrCopy, cch+1, asText);
				GlobalUnlock(hglbCopy);

				EmptyClipboard();
				bCopied = (SetClipboardData(CF_UNICODETEXT, hglbCopy) != NULL);
			}
		}

		CloseClipboard();
	}

	return bCopied;
}

LPTOP_LEVEL_EXCEPTION_FILTER gpfnPrevExFilter = NULL;
bool gbCreateDumpOnExceptionInstalled = false;
LONG WINAPI CreateDumpOnException(LPEXCEPTION_POINTERS ExceptionInfo)
{
	bool bKernelTrap = (gnInReadConsoleOutput > 0);
	wchar_t szExcptInfo[1024] = L"";
	wchar_t szDmpFile[MAX_PATH+64] = L"";
	wchar_t szAdd[2000];

	DWORD dwErr = CreateDumpForReport(ExceptionInfo, szExcptInfo, szDmpFile);

	szAdd[0] = 0;

	if (bKernelTrap)
	{
		wcscat_c(szAdd, L"Due to Microsoft kernel bug the crash was occurred\r\n");
		wcscat_c(szAdd, CEMSBUGWIKI /* http://conemu.github.io/en/MicrosoftBugs.html */);
		wcscat_c(szAdd, L"\r\n\r\n" L"The only possible workaround: enabling ‘Inject ConEmuHk’\r\n");
		wcscat_c(szAdd, CEHOOKSWIKI /* http://conemu.github.io/en/ConEmuHk.html */);
		wcscat_c(szAdd, L"\r\n\r\n");
	}

	wcscat_c(szAdd, szExcptInfo);

	if (szDmpFile[0])
	{
		wcscat_c(szAdd, L"\r\n\r\n" L"Memory dump was saved to\r\n");
		wcscat_c(szAdd, szDmpFile);

		if (!bKernelTrap)
		{
			wcscat_c(szAdd, L"\r\n\r\n" L"Please Zip it and send to developer (via DropBox etc.)\r\n");
			wcscat_c(szAdd, CEREPORTCRASH /* http://conemu.github.io/en/Issues.html... */);
		}
	}

	wcscat_c(szAdd, L"\r\n\r\nPress <Yes> to copy this text to clipboard");
	if (!bKernelTrap)
	{
		wcscat_c(szAdd, L"\r\nand open project web page");
	}

	// Message title
	wchar_t szTitle[100], szExe[MAX_PATH] = L"", *pszExeName;
	GetModuleFileName(NULL, szExe, countof(szExe));
	pszExeName = (wchar_t*)PointToName(szExe);
	if (pszExeName && lstrlen(pszExeName) > 63) pszExeName[63] = 0;
	_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"%s crashed, PID=%u", pszExeName ? pszExeName : L"<process>", GetCurrentProcessId());

	DWORD nMsgFlags = MB_YESNO|MB_ICONSTOP|MB_SYSTEMMODAL
		| (bKernelTrap ? MB_DEFBUTTON2 : 0);

	int nBtn = MessageBox(NULL, szAdd, szTitle, nMsgFlags);
	if (nBtn == IDYES)
	{
		CopyToClipboard(szAdd);
		if (!bKernelTrap)
		{
			ShellExecute(NULL, L"open", CEREPORTCRASH, NULL, NULL, SW_SHOWNORMAL);
		}
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
	// По умолчанию - фильтр в AltServer не включается, но в настройках ConEmu есть опция
	// gpSet->isConsoleExceptionHandler --> CECF_ConExcHandler
	_ASSERTE((gnRunMode == RM_ALTSERVER) && (gpSrv->pConsole && (gpSrv->pConsole->hdr.Flags & CECF_ConExcHandler)));

	// Far 3.x, telnet, Vim, etc.
	// В этих программах ConEmuCD.dll может загружаться для работы с альтернативными буферами и TrueColor
	if (!gpfnPrevExFilter && !IsDebuggerPresent())
	{
		// Сохраним, если фильтр уже был установлен - будем звать его из нашей функции
		gpfnPrevExFilter = SetUnhandledExceptionFilter(CreateDumpOnException);
		gbCreateDumpOnExceptionInstalled = true;
	}
}

#ifdef SHOW_SERVER_STARTED_MSGBOX
void ShowServerStartedMsgBox()
{
	wchar_t szTitle[100]; _wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuC [Server] started (PID=%i)", gnSelfPID);
	const wchar_t* pszCmdLine = GetCommandLineW();
	MessageBox(NULL,pszCmdLine,szTitle,0);
}
#endif


bool CheckAndWarnHookers()
{
	if (gbSkipHookersCheck)
	{
		if (gpLogSize) gpLogSize->LogString(L"CheckAndWarnHookers skipped due to /SKIPHOOKERS switch");
		return false;
	}

	bool bHooked = false;
	struct CheckModules {
		LPCWSTR Title, File;
	} modules [] = {
		{L"MacType", WIN3264TEST(L"MacType.dll", L"MacType64.dll")},
		{L"AnsiCon", WIN3264TEST(L"ANSI32.dll", L"ANSI64.dll")},
		{NULL}
	};
	CEStr szMessage;
	wchar_t szPath[MAX_PATH+16] = L"", szAddress[32] = L"";
	LPCWSTR pszTitle = NULL, pszName = NULL;
	HMODULE hModule = NULL;
	bool bConOutChecked = false, bRedirected = false;
	//BOOL bColorChanged = FALSE;
	CONSOLE_SCREEN_BUFFER_INFO sbi = {};
	HANDLE hOut = NULL;

	for (INT_PTR i = 0; modules[i].Title; i++)
	{
		pszTitle = modules[i].Title;
		pszName = modules[i].File;

		hModule = GetModuleHandle(pszName);
		if (hModule)
		{
			bHooked = true;
			if (!GetModuleFileName(hModule, szPath, countof(szPath)))
			{
				wcscpy_c(szPath, pszName); // Must not get here, but show a name at least on errors
			}

			if (!bConOutChecked)
			{
				bConOutChecked = true;
				hOut = GetStdHandle(STD_OUTPUT_HANDLE);
				GetConsoleScreenBufferInfo(hOut, &sbi);
				bRedirected = IsOutputRedirected();

				#if 0
				// Useless in most cases, most of users are running cmd.exe
				// but it clears all existing text attributes on start
				if (!bRedirected)
					bColorChanged = SetConsoleTextAttribute(hOut, 12); // LightRed on Black
				#endif
			}

			_wsprintf(szAddress, SKIPCOUNT(szAddress) WIN3264TEST(L"0x%08X",L"0x%08X%08X"), WIN3264WSPRINT((DWORD_PTR)hModule));
			szMessage = lstrmerge(
				L"WARNING! The ", pszTitle, L"'s hooks are detected at ", szAddress, L"\r\n"
				L"         ");
			LPCWSTR pszTail = L"\r\n"
				L"         Please add ConEmuC.exe and ConEmuC64.exe\r\n"
				L"         to the exclusion list to avoid crashes!\r\n"
				L"         " CEMACTYPEWARN L"\r\n\r\n";
			_wprintf(szMessage);
			_wprintf(szPath);
			_wprintf(pszTail);
		}
	}

	#if 0
	// If we've set warning colors - return original ones
	if (bColorChanged)
		SetConsoleTextAttribute(hOut, sbi.wAttributes);
	#endif

	return bHooked;
}




// Main entry point for ConEmuC.exe
#if defined(__GNUC__)
extern "C"
#endif
int __stdcall ConsoleMain2(int anWorkMode/*0-Server&ComSpec,1-AltServer,2-Reserved*/)
{
	TODO("можно при ошибках показать консоль, предварительно поставив 80x25 и установив крупный шрифт");

	#if defined(SHOW_MAIN_MSGBOX) || defined(SHOW_ADMIN_STARTED_MSGBOX)
	bool bShowWarn = false;
	#if defined(SHOW_MAIN_MSGBOX)
	if (!IsDebuggerPresent()) bShowWarn = true;
	#endif
	#if defined(SHOW_ADMIN_STARTED_MSGBOX)
	if (IsUserAdmin()) bShowWarn = true;
	#endif
	if (bShowWarn)
	{
		char szMsg[MAX_PATH+128]; msprintf(szMsg, countof(szMsg), WIN3264TEST("ConEmuCD.dll","ConEmuCD64.dll") " loaded, PID=%u, TID=%u\r\n", GetCurrentProcessId(), GetCurrentThreadId());
		int nMsgLen = lstrlenA(szMsg);
		GetModuleFileNameA(NULL, szMsg+nMsgLen, countof(szMsg)-nMsgLen);
		MessageBoxA(NULL, szMsg, "ConEmu server" WIN3264TEST(""," x64"), 0);
	}
	#endif

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
			gbCreateDumpOnExceptionInstalled = true;
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
			gbCreateDumpOnExceptionInstalled = true;
		}
	}
	#endif


	// На всякий случай - сбросим
	gnRunMode = RM_UNDEFINED;

	// Check linker fails!
	if (ghOurModule == NULL)
	{
		wchar_t szTitle[128]; _wsprintf(szTitle, SKIPLEN(countof(szTitle)) WIN3264TEST(L"ConEmuCD",L"ConEmuCD64") L", PID=%u", GetCurrentProcessId());
		MessageBox(NULL, L"ConsoleMain2: ghOurModule is NULL\nDllMain was not executed", szTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
		return CERR_DLLMAIN_SKIPPED;
	}

	gpSrv = (SrvInfo*)calloc(sizeof(SrvInfo),1);
	if (gpSrv)
	{
		gpSrv->InitFields();

		if (ghConEmuWnd)
		{
			GetWindowThreadProcessId(ghConEmuWnd, &gnConEmuPID);
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
	_wsprintfA(szDbgString, SKIPLEN(szDbgString) "ConEmuC: PID=%u", GetCurrentProcessId());
	MessageBoxA(0, GetCommandLineA(), szDbgString, MB_SYSTEMMODAL);
	hDbg = CreateFile(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
	                  0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	_wsprintfA(szHandles, SKIPLEN(szHandles) "STD_OUTPUT_HANDLE(0x%08X) STD_ERROR_HANDLE(0x%08X) CONOUT$(0x%08X)",
	        (DWORD)GetStdHandle(STD_OUTPUT_HANDLE), (DWORD)GetStdHandle(STD_ERROR_HANDLE), (DWORD)hDbg);
	printf("ConEmuC: Printf: %s\n", szHandles);
	_wsprintfA(szDbgString, SKIPLEN(szDbgString) "ConEmuC: STD_OUTPUT_HANDLE: %s\n", szHandles);
	lbDbgWrite = WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), szDbgString, lstrlenA(szDbgString), &nDbgWrite, NULL);
	_wsprintfA(szDbgString, SKIPLEN(szDbgString) "ConEmuC: STD_ERROR_HANDLE:  %s\n", szHandles);
	lbDbgWrite = WriteFile(GetStdHandle(STD_ERROR_HANDLE), szDbgString, lstrlenA(szDbgString), &nDbgWrite, NULL);
	_wsprintfA(szDbgString, SKIPLEN(szDbgString) "ConEmuC: CONOUT$: %s", szHandles);
	lbDbgWrite = WriteFile(hDbg, szDbgString, lstrlenA(szDbgString), &nDbgWrite, NULL);
	CloseHandle(hDbg);
	//_wsprintfA(szDbgString, SKIPLEN(szDbgString) "ConEmuC: PID=%u", GetCurrentProcessId());
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
	GetOsVersionInformational(&gOSVer);
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
		pfnGetConsoleDisplayMode = (FGetConsoleDisplayMode)GetProcAddress(hKernel, "GetConsoleDisplayMode");
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
		_ASSERTE(!gbCreateDumpOnExceptionInstalled);
		_ASSERTE(gbAttachMode==am_None);
		if (!(gbAttachMode & am_Modes))
			gbAttachMode |= am_Simple;
		gnConfirmExitParm = 2;
		gbAlwaysConfirmExit = FALSE; gbAutoDisableConfirmExit = FALSE;
		gbNoCreateProcess = TRUE;
		gbAlienMode = TRUE;
		gpSrv->dwRootProcess = GetCurrentProcessId();
		gpSrv->hRootProcess = GetCurrentProcess();
		//gnConEmuPID = ...;
		gpszRunCmd = (wchar_t*)calloc(1,2);
		CreateColorerHeader();
	}
	else if ((iRc = ParseCommandLine(pszFullCmdLine/*, &gpszRunCmd, &gbRunInBackgroundTab*/)) != 0)
	{
		goto wrap;
	}

	if (gbInShutdown)
		goto wrap;

	// По идее, при вызове дебаггера ParseCommandLine сразу должна послать на выход.
	_ASSERTE(!(gpSrv->DbgInfo.bDebuggerActive || gpSrv->DbgInfo.bDebugProcess || gpSrv->DbgInfo.bDebugProcessTree))


	if (gnRunMode == RM_SERVER)
	{
		// Until the root process is not terminated - set to STILL_ACTIVE
		gnExitCode = STILL_ACTIVE;

		#ifdef _DEBUG
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
		#endif

		// Warn about external hookers
		CheckAndWarnHookers();
	}

	_ASSERTE(!gpSrv->hRootProcessGui || (((DWORD)gpSrv->hRootProcessGui)!=0xCCCCCCCC && IsWindow(gpSrv->hRootProcessGui)));

	//#ifdef _DEBUG
	//CreateLogSizeFile();
	//#endif
	nExitPlaceStep = 100;
	xf_check();


	#ifdef SHOW_SERVER_STARTED_MSGBOX
	if ((gnRunMode == RM_SERVER || gnRunMode == RM_ALTSERVER) && !IsDebuggerPresent() && gbNoCreateProcess)
	{
		ShowServerStartedMsgBox();
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
	_wsprintfA(szDbgString, SKIPLEN(szDbgString) "ConEmuC: PID=%u", GetCurrentProcessId());
	MessageBoxA(0, "Press Ok to continue", szDbgString, MB_SYSTEMMODAL);
#endif

	// CREATE_NEW_PROCESS_GROUP - низя, перестает работать Ctrl-C
	// Перед CreateProcess нужно ставить 0, иначе из-за антивирусов может наступить
	// timeout ожидания окончания процесса еще ДО выхода из CreateProcess
	if (!(gbAttachMode & am_Modes))
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
		wchar_t* pszExpandedCmd = ParseConEmuSubst(gpszRunCmd);
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
				CmdArg szExe;
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
		CmdArg szExeName;
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
			_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"%s PID=%u", gsModuleName, GetCurrentProcessId());
			szDbgMsg[0] = 0;
			#endif

			if (gbDontInjectConEmuHk)
			{
				#ifdef SHOW_INJECT_MSGBOX
				_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"%s PID=%u\nConEmuHk injects skipped, PID=%u", gsModuleName, GetCurrentProcessId(), pi.dwProcessId);
				#endif
			}
			else
			{
				TODO("Не только в сервере, но и в ComSpec, чтобы дочерние КОНСОЛЬНЫЕ процессы могли пользоваться редиректами");
				//""F:\VCProject\FarPlugin\ConEmu\Bugs\DOS\TURBO.EXE ""
				TODO("При выполнении DOS приложений - VirtualAllocEx(hProcess, обламывается!");
				TODO("В принципе - завелось, но в сочетании с Anamorphosis получается странное зацикливание far->conemu->anamorph->conemu");

				#ifdef SHOW_INJECT_MSGBOX
				_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"%s PID=%u\nInjecting hooks into PID=%u", gsModuleName, GetCurrentProcessId(), pi.dwProcessId);
				MessageBoxW(NULL, szDbgMsg, szTitle, MB_SYSTEMMODAL);
				#endif

				//BOOL gbLogProcess = FALSE;
				//TODO("Получить из мэппинга glt_Process");
				//#ifdef _DEBUG
				//gbLogProcess = TRUE;
				//#endif
				CINJECTHK_EXIT_CODES iHookRc = CIH_GeneralError/*-1*/;
				if (((gnImageSubsystem == IMAGE_SUBSYSTEM_DOS_EXECUTABLE) || (gnImageBits == 16))
					&& !gbUseDosBox)
				{
					// Если запускается ntvdm.exe - все-равно хук поставить не даст
					iHookRc = CIH_OK/*0*/;
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
					iHookRc = InjectHooks(pi, gbLogProcess);
				}

				if (iHookRc != CIH_OK/*0*/)
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
			wchar_t szVerb[10];
			CmdArg szExec;

			if (NextArg(&pszCmd, szExec) == 0)
			{
				SHELLEXECUTEINFO sei = {sizeof(SHELLEXECUTEINFO)};
				sei.hwnd = ghConEmuWnd;
				sei.fMask = SEE_MASK_NO_CONSOLE; //SEE_MASK_NOCLOSEPROCESS; -- смысла ждать завершения нет - процесс запускается в новой консоли
				wcscpy_c(szVerb, L"open"); sei.lpVerb = szVerb;
				sei.lpFile = szExec.ms_Arg;
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

	if ((gbAttachMode & am_Modes))
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
	if (gnRunMode != RM_ALTSERVER && szSelfDir[0])
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
				_wsprintf(gpSrv->szGuiPipeName, SKIPLEN(countof(gpSrv->szGuiPipeName)) CEGUIPIPENAME, L".", LODWORD(ghConWnd)); // был gnSelfPID //-V205
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
	if (gnRunMode != RM_ALTSERVER && szSelfDir[0])
		SetCurrentDirectory(szSelfDir);


	if (gnRunMode == RM_ALTSERVER)
	{
		// Поскольку мы крутимся в этом же процессе - то завершения ждать глупо. Здесь другое поведение
		iRc = 0;
		goto AltServerDone;
	} // (gnRunMode == RM_ALTSERVER)
	else if (gnRunMode == RM_SERVER)
	{
		nExitPlaceStep = EPS_WAITING4PROCESS/*550*/;
		// По крайней мере один процесс в консоли запустился. Ждем пока в консоли не останется никого кроме нас
		nWait = WAIT_TIMEOUT; nWaitExitEvent = -2;

		_ASSERTE(!gpSrv->DbgInfo.bDebuggerActive);

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

		#ifdef _DEBUG
		xf_validate(NULL);
		#endif


		// Получить ExitCode
		GetExitCodeProcess(gpSrv->hRootProcess, &gnExitCode);

		nExitPlaceStep = EPS_ROOTPROCFINISHED/*560*/;

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

		DWORD nWaitMS = gbAsyncRun ? 0 : INFINITE;
		nWaitComspecExit = WaitForSingleObject(pi.hProcess, nWaitMS);

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

	if ((iRc == CERR_GUIMACRO_SUCCEEDED)
		|| (iRc == CERR_DOWNLOAD_SUCCEEDED))
	{
		iRc = 0;
	}

	if (gnRunMode == RM_SERVER && gpSrv->hRootProcess)
		GetExitCodeProcess(gpSrv->hRootProcess, &gnExitCode);
	else if (pi.hProcess)
		GetExitCodeProcess(pi.hProcess, &gnExitCode);
	// Ассерт может быть если был запрос на аттач, который не удался
	_ASSERTE(gnExitCode!=STILL_ACTIVE || (iRc==CERR_ATTACHFAILED) || (iRc==CERR_RUNNEWCONSOLE) || gbAsyncRun);

	// Log exit code
	if (((gnRunMode == RM_SERVER && gpSrv->hRootProcess) ? gpSrv->dwRootProcess : pi.dwProcessId) != 0)
	{
		wchar_t szInfo[80];
		LPCWSTR pszName = (gnRunMode == RM_SERVER && gpSrv->hRootProcess) ? L"Shell" : L"Process";
		DWORD nPID = (gnRunMode == RM_SERVER && gpSrv->hRootProcess) ? gpSrv->dwRootProcess : pi.dwProcessId;
		if (gnExitCode >= 0x80000000)
			_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"\n%s PID=%u ExitCode=%u (%i) {x%08X}", pszName, nPID, gnExitCode, (int)gnExitCode, gnExitCode);
		else
			_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"\n%s PID=%u ExitCode=%u {x%08X}", pszName, nPID, gnExitCode, gnExitCode);
		LogFunction(szInfo+1);

		if (gbPrintRetErrLevel)
		{
			wcscat_c(szInfo, L"\n");
			_wprintf(szInfo);
		}
	}

	if (iRc && (gbAttachMode & am_Auto))
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
					&& iRc!=CERR_WRONG_GUI_VERSION
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
		_ASSERTE(iRc==CERR_RUNNEWCONSOLE || gbComspecInitCalled);
		if (gbComspecInitCalled)
		{
			ComspecDone(iRc);
		}
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

	#ifdef _DEBUG
	#if 0
	if (gnRunMode == RM_COMSPEC)
	{
		if (gpszPrevConTitle)
		{
			if (ghConWnd)
				SetTitle(gpszPrevConTitle);

		}
	}
	#endif
	SafeFree(gpszPrevConTitle);
	#endif

	SafeCloseHandle(ghRootProcessFlag);

	LogSize(NULL, 0, "Shutdown");
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

	SafeFree(gpszRunCmd);
	SafeFree(gpszForcedTitle);

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
	UNREFERENCED_PARAMETER(nWaitComspecExit);
#if 0
	if (gnRunMode == RM_SERVER)
	{
		xf_dump();
	}
#endif
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

		HWND hConEmu = GetConEmuHWND(1/*Gui Main window*/);
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

	// See definition in "ConEmuCD/ConsoleHelp.h"
	_wprintf(pConsoleHelp);
	_wprintf(pNewConsoleHelp);

	// Don't ask keypress before exit
	gbInShutdown = TRUE;
}

void DosBoxHelp()
{
	// See definition in "ConEmuCD/ConsoleHelp.h"
	_wprintf(pDosBoxHelp);
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
		if (gpStartEnv && gpStartEnv->pszWorkDir)
		{
			_wcscat_c(lpInfo, nCchMax, gpStartEnv->pszWorkDir);
		}
		else
		{
			_ASSERTE(gpStartEnv && gpStartEnv->pszWorkDir);
			GetCurrentDirectory(MAX_PATH*2, lpInfo+lstrlen(lpInfo));
		}
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

		for (DWORD i = 0; i <20; i++)
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
	LogFunction(L"CheckAttachProcess");

	int liArgsFailed = 0;
	wchar_t szFailMsg[512]; szFailMsg[0] = 0;
	DWORD nProcesses[20] = {};
	DWORD nProcCount;
	BOOL lbRootExists = FALSE;
	wchar_t szProc[255] = {}, szTmp[10] = {};
	DWORD nFindId;

	if (gpSrv->hRootProcessGui)
	{
		if (!IsWindow(gpSrv->hRootProcessGui))
		{
			_wsprintf(szFailMsg, SKIPLEN(countof(szFailMsg)) L"Attach of GUI application was requested,\n"
				L"but required HWND(0x%08X) not found!", LODWORD(gpSrv->hRootProcessGui));
			liArgsFailed = 1;
			// will return CERR_CARGUMENT
		}
		else
		{
			DWORD nPid; GetWindowThreadProcessId(gpSrv->hRootProcessGui, &nPid);
			if (!gpSrv->dwRootProcess || (gpSrv->dwRootProcess != nPid))
			{
				_wsprintf(szFailMsg, SKIPLEN(countof(szFailMsg)) L"Attach of GUI application was requested,\n"
					L"but PID(%u) of HWND(0x%08X) does not match Root(%u)!",
					nPid, LODWORD(gpSrv->hRootProcessGui), gpSrv->dwRootProcess);
				liArgsFailed = 2;
				// will return CERR_CARGUMENT
			}
		}
	}
	else if (pfnGetConsoleProcessList==NULL)
	{
		wcscpy_c(szFailMsg, L"Attach to console app was requested, but required WinXP or higher!");
		liArgsFailed = 3;
		// will return CERR_CARGUMENT
	}
	else
	{
		nProcCount = pfnGetConsoleProcessList(nProcesses, 20);

		if ((nProcCount == 1) && gbCreatingHiddenConsole)
		{
			// Подождать, пока вызвавший процесс прицепится к нашей созданной консоли
			DWORD nStart = GetTickCount(), nMaxDelta = 30000, nDelta = 0;
			while (nDelta < nMaxDelta)
			{
				Sleep(100);
				nProcCount = pfnGetConsoleProcessList(nProcesses, 20);
				if (nProcCount > 1)
					break;
				nDelta = (GetTickCount() - nStart);
			}
		}

		// 2 процесса, потому что это мы сами и минимум еще один процесс в этой консоли,
		// иначе смысла в аттаче нет
		if (nProcCount < 2)
		{
			wcscpy_c(szFailMsg, L"Attach to console app was requested, but there is no console processes!");
			liArgsFailed = 4;
			//will return CERR_CARGUMENT
		}
		// не помню, зачем такая проверка была введена, но (nProcCount > 2) мешает аттачу.
		// в момент запуска сервера (/ATTACH /PID=n) еще жив родительский (/ATTACH /NOCMD)
		//// Если cmd.exe запущен из cmd.exe (в консоли уже больше двух процессов) - ничего не делать
		else if ((gpSrv->dwRootProcess != 0) || (nProcCount > 2))
		{
			lbRootExists = (gpSrv->dwRootProcess == 0);
			// И ругаться только под отладчиком
			nFindId = 0;

			for (int n = ((int)nProcCount-1); n >= 0; n--)
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
				liArgsFailed = 5;
				//will return CERR_CARGUMENT
			}
			else if ((gpSrv->dwRootProcess == 0) && (nProcCount > 2))
			{
				_wsprintf(szFailMsg, SKIPLEN(countof(szFailMsg)) L"Attach to GUI was requested, but\n" L"there is more than 2 console processes: %s\n", szProc);
				liArgsFailed = 6;
				//will return CERR_CARGUMENT
			}
		}
	}

	if (liArgsFailed)
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

// Возвращает CERR_UNICODE_CHK_OKAY(142), если консоль поддерживает отображение
// юникодных символов. Иначе - CERR_UNICODE_CHK_FAILED(141)
// This function is called by: ConEmuC.exe /CHECKUNICODE
int CheckUnicodeFont()
{
	int iRc = CERR_UNICODE_CHK_FAILED;

	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);


	wchar_t szText[80] = UnicodeTestString;
	wchar_t szColor[32] = ColorTestString;
	CHAR_INFO cWrite[80];
	CHAR_INFO cRead[80] = {};
	WORD aWrite[80], aRead[80] = {};
	wchar_t sAttrWrite[80] = {}, sAttrRead[80] = {}, sAttrBlock[80] = {};
	wchar_t szInfo[1024]; DWORD nTmp;
	wchar_t szCheck[80] = L"", szBlock[80] = L"";
	BOOL bInfo = FALSE, bWrite = FALSE, bRead = FALSE, bCheck = FALSE;
	DWORD nLen = lstrlen(szText), nWrite = 0, nRead = 0, nErr = 0;
	WORD nDefColor = 7;
	DWORD nColorLen = lstrlen(szColor);
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	_ASSERTE(nLen<=35); // ниже на 2 буфер множится

	wchar_t szMinor[8] = L""; lstrcpyn(szMinor, _T(MVV_4a), countof(szMinor));
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

	msprintf(szInfo, countof(szInfo), L"ConHWND=0x%08X, Class=\"", LODWORD(ghConWnd));
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


	// Simlify checking of ConEmu's "colorization"
	if (GetConsoleScreenBufferInfo(hOut, &csbi))
		nDefColor = csbi.wAttributes;
	WriteConsoleW(hOut, L"\r\n", 2, &nTmp, NULL);
	WORD nColor = 7;
	for (DWORD n = 0; n < nColorLen; n++)
	{
		SetConsoleTextAttribute(hOut, nColor);
		WriteConsoleW(hOut, szColor+n, 1, &nTmp, NULL);
		nColor++; if (nColor == 16) nColor = 7;
	}
	WriteConsoleW(hOut, L"\r\n", 2, &nTmp, NULL);


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

		COORD cr0 = {0,0};
		if ((bWrite = WriteConsoleOutputCharacterW(hOut, szText, nLen, csbi.dwCursorPosition, &nWrite)) != FALSE)
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

// ConEmuC -OsVerInfo
int OsVerInfo()
{
	OSVERSIONINFOEX osv = {sizeof(osv)};
	GetOsVersionInformational((OSVERSIONINFO*)&osv);

	UINT DBCS = IsDbcs();
	UINT HWFS = IsHwFullScreenAvailable();
	UINT W5fam = IsWin5family();
	UINT WXPSP1 = IsWinXPSP1();
	UINT W6 = IsWin6();
	UINT W7 = IsWin7();
	UINT W10 = IsWin10();
	UINT Wx64 = IsWindows64();
	UINT WINE = IsWine();
	UINT WPE = IsWinPE();
	UINT TELNET = isTerminalMode();

	wchar_t szInfo[200];
	_wsprintf(szInfo, SKIPCOUNT(szInfo)
		L"OS version information\n"
		L"%u.%u build %u SP%u.%u suite=x%04X type=%u\n"
		L"W5fam=%u WXPSP1=%u W6=%u W7=%u W10=%u Wx64=%u\n"
		L"HWFS=%u DBCS=%u WINE=%u WPE=%u TELNET=%u\n",
		osv.dwMajorVersion, osv.dwMinorVersion, osv.dwBuildNumber, osv.wServicePackMajor, osv.wServicePackMinor, osv.wSuiteMask, osv.wProductType,
		W5fam, WXPSP1, W6, W7, W10, Wx64, HWFS,
		DBCS, WINE, WPE, TELNET);
	_wprintf(szInfo);

	return MAKEWORD(osv.dwMinorVersion, osv.dwMajorVersion);
}

enum ConEmuStateCheck
{
	ec_None = 0,
	ec_IsConEmu,
	ec_IsTerm,
	ec_IsAnsi,
	ec_IsAdmin,
};

bool DoStateCheck(ConEmuStateCheck eStateCheck)
{
	LogFunction(L"DoStateCheck");

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
	case ec_IsAdmin:
		bOn = IsUserAdmin();
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
	ea_OsVerInfo,
	ea_ExportCon,  // export env.vars to processes of active console
	ea_ExportTab,  // ea_ExportCon + ConEmu window
	ea_ExportGui,  // export env.vars to ConEmu window
	ea_ExportAll,  // export env.vars to all opened tabs of current ConEmu window
	ea_Download,   // after "/download" switch may be unlimited pairs of {"url","file"},{"url","file"},...
	ea_ParseArgs,  // debug test of NextArg function... print args to STDOUT
	ea_ErrorLevel, // return specified errorlevel
	ea_OutEcho,    // echo "string" with ANSI processing
	ea_OutType,    // print file contents with ANSI processing
	ea_StoreCWD,   // store current console work dir
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


	#ifdef SHOW_INJECT_MSGBOX
	wchar_t szDbgMsg[512], szTitle[128];
	PROCESSENTRY32 pinf;
	GetProcessInfo(pi.dwProcessId, &pinf);
	_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuCD PID=%u", GetCurrentProcessId());
	_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"InjectsTo PID=%s {%s}\nConEmuCD PID=%u", asCmdArg ? asCmdArg : L"", pinf.szExeFile, GetCurrentProcessId());
	MessageBoxW(NULL, szDbgMsg, szTitle, MB_SYSTEMMODAL);
	#endif


	if (pi.hProcess && pi.hThread && pi.dwProcessId && pi.dwThreadId)
	{
		// Аргумент abForceGui не использовался
		CINJECTHK_EXIT_CODES iHookRc = InjectHooks(pi, /*lbForceGui,*/ gbLogProcess);

		if (iHookRc == CIH_OK/*0*/)
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
			GetCurrentProcessId(), LODWORD(pi.hProcess), LODWORD(pi.hThread), pi.dwProcessId, pi.dwThreadId, lbForceGui, //-V205
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
	_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuCD PID=%u", GetCurrentProcessId());
	_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"Hooking PID=%s {%s}\nConEmuCD PID=%u. Continue with injects?", asCmdArg ? asCmdArg : L"", pinf.szExeFile, GetCurrentProcessId());
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
			wchar_t szTitle[100]; _wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"%s PID=%u /INJECT", gsModuleName, gnSelfPID);
			const wchar_t* pszCmdLine = GetCommandLineW();
			MessageBox(NULL,pszCmdLine,szTitle,MB_SYSTEMMODAL);
		}
		#endif

		// Go to hook
		// InjectRemote waits for thread termination
		DWORD nErrCode = 0;
		CINFILTRATE_EXIT_CODES iHookRc = InjectRemote(nRemotePID, abDefTermOnly, &nErrCode);

		if (iHookRc == CIR_OK/*0*/ || iHookRc == CIR_AlreadyInjected/*1*/)
		{
			return iHookRc ? CERR_HOOKS_WAS_ALREADY_SET : CERR_HOOKS_WAS_SET;
		}

		CProcessData processes;
		CEStr lsName, lsPath;
		processes.GetProcessName(nRemotePID, lsName.GetBuffer(MAX_PATH), MAX_PATH, lsPath.GetBuffer(MAX_PATH*2), MAX_PATH*2, NULL);

		DWORD nSelfPID = GetCurrentProcessId();
		PROCESSENTRY32 self = {sizeof(self)}, parent = {sizeof(parent)};
		// Not optimal, needs refactoring
		if (GetProcessInfo(nSelfPID, &self))
			GetProcessInfo(self.th32ParentProcessID, &parent);

		// Ошибку (пока во всяком случае) лучше показать, для отлова возможных проблем
		//_ASSERTE(iHookRc == 0); -- ассерт не нужен, есть MsgBox
		wchar_t szDbgMsg[255], szTitle[128];
		_wsprintf(szTitle, SKIPLEN(countof(szTitle))
			L"%s, PID=%u", gsModuleName, nSelfPID);
		_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg))
			L"%s.X, PID=%u\n"
			L"Injecting remote into PID=%u from ParentPID=%u\n"
			L"FAILED, code=%i:0x%08X\r\n",
			gsModuleName, nSelfPID,
			nRemotePID, self.th32ParentProcessID, iHookRc, nErrCode);

		CEStr lsError(lstrmerge(szDbgMsg,
			L"Process: ",
			lsPath.IsEmpty() ? lsName.IsEmpty() ? L"<Unknown>" : lsName.ms_Arg : lsPath.ms_Arg,
			L"\n" L"Parent: ", parent.szExeFile));

		MessageBoxW(NULL, lsError, szTitle, MB_SYSTEMMODAL);
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

// GCC error: template argument for 'template<class _Ty> class MArray' uses local type
struct ProcInfo {
	DWORD nPID, nParentPID;
	DWORD_PTR Flags;
	WCHAR szExeFile[64];
};

int DoExportEnv(LPCWSTR asCmdArg, ConEmuExecAction eExecAction, bool bSilent = false)
{
	int iRc = CERR_CARGUMENT;
	//ProcInfo* pList = NULL;
	MArray<ProcInfo> List;
	LPWSTR pszAllVars = NULL, pszSrc;
	int iVarCount = 0;
	CESERVER_REQ *pIn = NULL;
	const DWORD nSelfPID = GetCurrentProcessId();
	DWORD nTestPID, nParentPID;
	DWORD nSrvPID = 0;
	CESERVER_CONSOLE_MAPPING_HDR test = {};
	BOOL lbMapExist = false;
	HANDLE h;
	size_t cchMaxEnvLen = 0;
	wchar_t* pszBuffer;
	CEStr szTmpPart;
	LPCWSTR pszTmpPartStart;
	LPCWSTR pszCmdArg;

	//_ASSERTE(FALSE && "Continue with exporting environment");

	#define ExpFailedPref WIN3264TEST("ConEmuC","ConEmuC64") ": can't export environment"

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

	pszCmdArg = SkipNonPrintable(asCmdArg);

	//_ASSERTE(FALSE && "Continue to export");

	// Copy variables to buffer
	if (!pszCmdArg || !*pszCmdArg || (lstrcmp(pszCmdArg, L"*")==0) || (lstrcmp(pszCmdArg, L"\"*\"")==0)
		|| (wcsncmp(pszCmdArg, L"* ", 2)==0) || (wcsncmp(pszCmdArg, L"\"*\" ", 4)==0))
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
				iVarCount++;
			}
			*pszEq = L'=';
			pszSrc = pszNext;
		}
	}
	else while (0==NextArg(&pszCmdArg, szTmpPart, &pszTmpPartStart))
	{
		if ((pszTmpPartStart > asCmdArg) && (*(pszTmpPartStart-1) != L'"'))
		{
			// Unless the argument name was surrounded by double-quotes
			// replace commas with spaces, this allows more intuitive
			// way to run something like this:
			// ConEmuC -export=ALL SSH_AGENT_PID,SSH_AUTH_SOCK
			wchar_t* pszComma = szTmpPart.ms_Arg;
			while ((pszComma = (wchar_t*)wcspbrk(pszComma, L",;")) != NULL)
			{
				*pszComma = L' ';
			}
		}
		LPCWSTR pszPart = szTmpPart;
		CmdArg szTest;
		while (0==NextArg(&pszPart, szTest))
		{
			if (!*szTest || *szTest == L'*')
			{
				if (!bSilent)
					_printf(ExpFailedPref ", name masks can't be quoted\n");
				goto wrap;
			}

			bool bFound = false;
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
					iVarCount++;
					pszBuffer += cchAdd;
					_ASSERTE(*pszBuffer == 0);
					cchLeft -= cchAdd;
					bFound = true;
				}
				*pszEq = L'=';
				pszSrc = pszNext;

				// continue scan, only if it is a mask
				if (bFound && !wcschr(szTest, L'*'))
					break;
			} // end of 'while (*pszSrc)'

			if (!bFound && !wcschr(szTest, L'*'))
			{
				// To avoid "nothing to export"
				size_t cchAdd = lstrlen(szTest) + 1;
				if (cchAdd < cchLeft)
				{
					// Copy "Name\0\0"
					wmemmove(pszBuffer, szTest, cchAdd);
					iVarCount++;
					cchAdd++; // We need second zero after a Name
					pszBuffer += cchAdd;
					cchLeft -= cchAdd;
					_ASSERTE(*pszBuffer == 0);
				}
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
	// Don't do snapshot if only GUI was requested
	h = (eExecAction != ea_ExportGui) ? CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0) : NULL;
	// Snapshot opened?
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
				LPCWSTR pszName = PointToName(PI.szExeFile);
				ProcInfo pi = {
					PI.th32ProcessID,
					PI.th32ParentProcessID,
					IsConsoleServer(pszName) ? 1 : 0
				};
				lstrcpyn(pi.szExeFile, pszName, countof(pi.szExeFile));
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
			wchar_t szName[64] = L"???";
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
					lstrcpyn(szName, pi.szExeFile, countof(szName));
					// nTestPID is already pi.nPID
					bParentFound = true;
					break;
				}
				i++;
			}

			if (nTestPID == nSrvPID)
				break; // Fin, this is root server
			if (!bParentFound)
				break; // Fin, no more parents
			if (nTestPID == nOurParentPID)
				continue; // Useless, we just inherited ALL those variables from our parent

			// Apply environment
			CESERVER_REQ *pOut = ExecuteHkCmd(nTestPID, pIn, ghConWnd, FALSE, TRUE);

			if (!pOut && !bSilent)
			{
				wchar_t szInfo[200];
				_wsprintf(szInfo, SKIPCOUNT(szInfo)
					WIN3264TEST(L"ConEmuC",L"ConEmuC64")
					L": process %s PID=%u was skipped: noninteractive or lack of ConEmuHk\n",
					szName, nTestPID);
				_wprintf(szInfo);
			}

			ExecuteFreeResult(pOut);
		}
	}

	// We are trying to apply environment to parent tree even if NO server or GUI was found
	if (nSrvPID)
	{
		// Server found? Try to apply environment
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

		// ea_ExportTab, ea_ExportGui, ea_ExportAll -> export to ConEmu window
		ExecuteGuiCmd(ghConWnd, pIn, ghConWnd, TRUE);
	}

	if (!bSilent)
	{
		wchar_t szVars[80];
		_wsprintf(szVars, SKIPCOUNT(szVars) WIN3264TEST(L"ConEmuC",L"ConEmuC64") L": %i %s processed\n", iVarCount, (iVarCount == 1) ? L"variable was" : L"variables were");
		_wprintf(szVars);
	}

	iRc = 0;
wrap:
	// Fin
	if (pszAllVars)
		FreeEnvironmentStringsW((LPWCH)pszAllVars);
	if (pIn)
		ExecuteFreeResult(pIn);

	return iRc;
}

static void PrintTime(LPCWSTR asLabel)
{
	SYSTEMTIME st = {}; GetLocalTime(&st);
	wchar_t szTime[80];
	_wsprintf(szTime, SKIPLEN(countof(szTime)) L"%i:%02i:%02i.%03i{%u} %s",
	           st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, GetCurrentThreadId(), asLabel);
	_wprintf(szTime);
	#if defined(_DEBUG)
	OutputDebugString(szTime);
	#endif
}
static void WINAPI DownloadCallback(const CEDownloadInfo* pInfo)
{
	wchar_t* pszInfo = pInfo->GetFormatted(true);
	PrintTime(pInfo->lParam==(dc_ErrCallback+1) ? L"Error: " : pInfo->lParam==(dc_LogCallback+1) ? L"Info:  " : pInfo->lParam==(dc_ProgressCallback+1) ? L"Progr: " : L"");
	_wprintf(pszInfo ? pszInfo : L"<NULL>\n");
	#if defined(_DEBUG)
	OutputDebugString(pszInfo);
	#endif
	SafeFree(pszInfo);
}

int DoDownload(LPCWSTR asCmdLine)
{
	int iRc = CERR_CARGUMENT;
	DWORD_PTR drc;
	CmdArg szArg;
	wchar_t* pszUrl = NULL;
	size_t iFiles = 0;
	CEDownloadErrorArg args[4];
	wchar_t* pszExpanded = NULL;
	wchar_t szFullPath[MAX_PATH*2];
	DWORD nFullRc;
	wchar_t *pszProxy = NULL, *pszProxyLogin = NULL, *pszProxyPassword = NULL;
	wchar_t *pszLogin = NULL, *pszPassword = NULL;
	wchar_t *pszOTimeout = NULL, *pszTimeout = NULL;
	wchar_t *pszAsync = NULL;

	DownloadCommand(dc_Init, 0, NULL);

	args[0].uintArg = (DWORD_PTR)DownloadCallback; args[0].argType = at_Uint;
	args[1].argType = at_Uint;
	_ASSERTE(dc_ErrCallback==0 && dc_LogCallback==2);
	for (int i = dc_ErrCallback; i <= dc_LogCallback; i++)
	{
		args[1].uintArg = (i+1);
		DownloadCommand((CEDownloadCommand)i, 2, args);
	}

	struct {
		LPCWSTR   pszArgName;
		wchar_t** ppszValue;
	} KnownArgs[] = {
		{L"login", &pszLogin},
		{L"password", &pszPassword},
		{L"proxy", &pszProxy},
		{L"proxylogin", &pszProxyLogin},
		{L"proxypassword", &pszProxyPassword},
		{L"otimeout", &pszOTimeout},
		{L"timeout", &pszTimeout},
		{L"async", &pszAsync},
		{NULL}
	};

	while (NextArg(&asCmdLine, szArg) == 0)
	{
		LPCWSTR psz = szArg;
		if ((psz[0] == L'-') || (psz[0] == L'/'))
		{
			psz++;
			bool bKnown = false;
			for (size_t i = 0; KnownArgs[i].pszArgName; i++)
			{
				if (lstrcmpi(psz, KnownArgs[i].pszArgName) == 0)
				{
					SafeFree(*KnownArgs[i].ppszValue);
					if (NextArg(&asCmdLine, szArg) == 0)
						*KnownArgs[i].ppszValue = szArg.Detach();
					bKnown = true;
					break;
				}
			}
			if (!bKnown)
			{
				_printf("Unknown argument '");
				_wprintf(psz);
				_printf("'\n");
				iRc = CERR_CARGUMENT;
				goto wrap;
			}
			continue;
		}

		SafeFree(pszUrl);
		pszUrl = szArg.Detach();
		if (NextArg(&asCmdLine, szArg) != 0)
		{
			iRc = CERR_CARGUMENT;
			goto wrap;
		}

		// Proxy?
		if (pszProxy || pszProxyLogin)
		{
			args[0].strArg = pszProxy;         args[0].argType = at_Str;
			args[1].strArg = pszProxyLogin;    args[1].argType = at_Str;
			args[2].strArg = pszProxyPassword; args[2].argType = at_Str;
			DownloadCommand(dc_SetProxy, 3, args);
		}
		// Server login
		if (pszLogin)
		{
			args[0].strArg = pszLogin;    args[0].argType = at_Str;
			args[1].strArg = pszPassword; args[1].argType = at_Str;
			DownloadCommand(dc_SetLogin, 2, args);
		}
		// Sync or Ansync mode?
		if (pszAsync)
		{
			args[0].uintArg = *pszAsync && (pszAsync[0] != L'0') && (pszAsync[0] != L'N') && (pszAsync[0] != L'n'); args[0].argType = at_Uint;
			DownloadCommand(dc_SetAsync, 1, args);
		}
		// Timeouts?
		if (pszOTimeout)
		{
			wchar_t* pszEnd;
			args[0].uintArg = 0; args[0].argType = at_Uint;
			args[1].uintArg = wcstol(pszOTimeout, &pszEnd, 10); args[1].argType = at_Uint;
			DownloadCommand(dc_SetTimeout, 2, args);
		}
		if (pszTimeout)
		{
			wchar_t* pszEnd;
			args[0].uintArg = 1; args[0].argType = at_Uint;
			args[1].uintArg = wcstol(pszTimeout, &pszEnd, 10); args[1].argType = at_Uint;
			DownloadCommand(dc_SetTimeout, 2, args);
		}

		args[0].strArg = pszUrl; args[0].argType = at_Str;
		args[1].strArg = szArg;  args[1].argType = at_Str;
		args[2].uintArg = TRUE;  args[2].argType = at_Uint;

		// May be file name was specified relatively or even with env.vars?
		SafeFree(pszExpanded);
		pszExpanded = ExpandEnvStr(szArg);
		nFullRc = GetFullPathName((pszExpanded && *pszExpanded) ? pszExpanded : (LPCWSTR)szArg, countof(szFullPath), szFullPath, NULL);
		if (nFullRc && nFullRc < countof(szFullPath))
			args[1].strArg = szFullPath;

		gbIsDownloading = true;
		drc = DownloadCommand(dc_DownloadFile, 3, args);
		gbIsDownloading = false;
		if (drc == 0)
		{
			iRc = CERR_DOWNLOAD_FAILED;
			_printf("Download failed\n");
			goto wrap;
		}

		iFiles++;
		_printf("File downloaded, size=%u, crc32=x%08X\n", (DWORD)args[0].uintArg, (DWORD)args[1].uintArg, NULL);
	}

	iRc = iFiles ? CERR_DOWNLOAD_SUCCEEDED : CERR_CARGUMENT;
wrap:
	DownloadCommand(dc_Deinit, 0, NULL);
	SafeFree(pszExpanded);
	for (size_t i = 0; KnownArgs[i].pszArgName; i++)
	{
		SafeFree(*KnownArgs[i].ppszValue);
	}
	return iRc;
}

int DoParseArgs(LPCWSTR asCmdLine)
{
	int iShellCount = 0;
	LPWSTR* ppszShl = CommandLineToArgvW(asCmdLine, &iShellCount);

	int i = 0;
	CmdArg szArg;
	_printf("ConEmu `NextArg` splitter\n");
	while (NextArg(&asCmdLine, szArg) == 0)
	{
		_printf("%u: `", ++i);
		_wprintf(szArg);
		_printf("`\n");
	}
	_printf("Total arguments parsed: %u\n", i);

	_printf("Standard shell splitter\n");
	for (int j = 0; j < iShellCount; j++)
	{
		_printf("%u: `", j);
		_wprintf(ppszShl[j]);
		_printf("`\n");
	}
	_printf("Total arguments parsed: %u\n", iShellCount);
	LocalFree(ppszShl);

	return i;
}

struct MacroInstance
{
	HWND  hConEmuWnd;  // Root! window
	DWORD nTabIndex;   // Specially selected tab, 1-based
	DWORD nSplitIndex; // Specially selected split, 1-based
	DWORD nPID;
};

BOOL CALLBACK FindTopGuiOrConsole(HWND hWnd, LPARAM lParam)
{
	MacroInstance* p = (MacroInstance*)lParam;
	wchar_t szClass[MAX_PATH];
	if (GetClassName(hWnd, szClass, countof(szClass)) < 1)
		return TRUE; // continue search

	// If called "-GuiMacro:0" we need to find first GUI window!
	bool bConClass = isConsoleClass(szClass);
	if (!p->nPID && bConClass)
		return TRUE;

	bool bGuiClass = (lstrcmp(szClass, VirtualConsoleClassMain) == 0);
	if (!bGuiClass && !bConClass)
		return TRUE; // continue search

	DWORD nTestPID = 0; GetWindowThreadProcessId(hWnd, &nTestPID);
	if (nTestPID == p->nPID || !p->nPID)
	{
		p->hConEmuWnd = hWnd;
		return FALSE; // Found! stop search
	}

	return TRUE; // continue search
}

void ArgGuiMacro(CmdArg& szArg, MacroInstance& Inst)
{
	wchar_t szLog[200];
	if (gpLogSize) gpLogSize->LogString(szArg);

	// Могли указать PID или HWND требуемого инстанса
	if (szArg[9] == L':' || szArg[9] == L'=')
	{
		wchar_t* pszEnd = NULL;
		wchar_t* pszID = szArg.ms_Arg+10;
		// Loop through GuiMacro options
		while (pszID && *pszID)
		{
			// HWND of main ConEmu (GUI) window
			if ((pszID[0] == L'0' && (pszID[1] == L'x' || pszID[1] == L'X'))
				|| (pszID[0] == L'x' || pszID[0] == L'X'))
			{
				Inst.hConEmuWnd = (HWND)(DWORD_PTR)wcstoul(pszID[0] == L'0' ? (pszID+2) : (pszID+1), &pszEnd, 16);
				pszID = pszEnd;

				if (gpLogSize)
				{
					_wsprintf(szLog, SKIPLEN(countof(szLog)) L"Exact instance requested, HWND=x%08X", (DWORD)(DWORD_PTR)Inst.hConEmuWnd);
					gpLogSize->LogString(szLog);
				}
			}
			else if (isDigit(pszID[0]))
			{
				// Если тут передать "0" - то выполняем в первом попавшемся (по Z-order) окне ConEmu
				// То есть даже если ConEmuC вызван во вкладке, то
				// а) макро может выполниться в другом окне ConEmu (если наше свернуто)
				// б) макро может выполниться в другой вкладке (если наша не активна)
				Inst.nPID = wcstoul(pszID, &pszEnd, 10);
				EnumWindows(FindTopGuiOrConsole, (LPARAM)&Inst);

				if (gpLogSize)
				{
					if (Inst.hConEmuWnd && Inst.nPID)
						_wsprintf(szLog, SKIPLEN(countof(szLog)) L"Exact PID=%u requested, instance found HWND=x%08X", Inst.nPID, (DWORD)(DWORD_PTR)Inst.hConEmuWnd);
					else if (Inst.hConEmuWnd && !Inst.nPID)
						_wsprintf(szLog, SKIPLEN(countof(szLog)) L"First found requested, instance found HWND=x%08X", (DWORD)(DWORD_PTR)Inst.hConEmuWnd);
					else
						_wsprintf(szLog, SKIPLEN(countof(szLog)) L"No instances was found (requested PID=%u) GuiMacro will fails", Inst.nPID);
					gpLogSize->LogString(szLog);
				}

				if (pszID == pszEnd)
					break;
				pszID = pszEnd;
			}
			else if (wcschr(L"SsTt", pszID[0]) && isDigit(pszID[1]))
			{
				switch (pszID[0])
				{
				case L'S': case L's':
					Inst.nSplitIndex = wcstoul(pszID+1, &pszEnd, 10);
					_wsprintf(szLog, SKIPLEN(countof(szLog)) L"Split was requested: %u", Inst.nSplitIndex);
					break;
				case L'T': case L't':
					Inst.nTabIndex = wcstoul(pszID+1, &pszEnd, 10);
					_wsprintf(szLog, SKIPLEN(countof(szLog)) L"Tab was requested: %u", Inst.nTabIndex);
					break;
				}
				gpLogSize->LogString(szLog);
				if (pszID == pszEnd)
					break;
				pszID = pszEnd;
			}
			else if (*pszID == L':')
			{
				pszID++;
			}
			else
			{
				_ASSERTE(FALSE && "Unsupported GuiMacro option");
				if (gpLogSize)
				{
					CEStr strErr(lstrmerge(L"Unsupported GuiMacro option: ", szArg.ms_Arg));
					gpLogSize->LogString(strErr);
				}
				break;
			}
		}

		// This may be VirtualConsoleClassMain or RealConsoleClass...
		if (Inst.hConEmuWnd)
		{
			// Has no effect, if hMacroInstance == RealConsoleClass
			wchar_t szClass[MAX_PATH] = L"";
			if ((GetClassName(Inst.hConEmuWnd, szClass, countof(szClass)) > 0) && (lstrcmp(szClass, VirtualConsoleClassMain) == 0))
			{
				DWORD nGuiPID = 0; GetWindowThreadProcessId(Inst.hConEmuWnd, &nGuiPID);
				AllowSetForegroundWindow(nGuiPID);
			}
			if (gpLogSize)
			{
				gpLogSize->LogString(L"Found instance class name: ", true, NULL, false);
				gpLogSize->LogString(szClass, false, NULL, true);
			}
		}
	}
}

int DoGuiMacro(LPCWSTR asCmdArg, MacroInstance& Inst)
{
	// If neither hMacroInstance nor ghConEmuWnd was set - Macro will fails most likely
	_ASSERTE(Inst.hConEmuWnd!=NULL || ghConEmuWnd!=NULL);

	HWND hCallWnd = Inst.hConEmuWnd ? Inst.hConEmuWnd : ghConWnd;

	// Все что в asCmdArg - выполнить в Gui
	int iRc = CERR_GUIMACRO_FAILED;
	int nLen = lstrlen(asCmdArg);
	//SetEnvironmentVariable(CEGUIMACRORETENVVAR, NULL);
	CESERVER_REQ *pIn = NULL, *pOut = NULL;
	pIn = ExecuteNewCmd(CECMD_GUIMACRO, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_GUIMACRO)+nLen*sizeof(wchar_t));
	pIn->GuiMacro.nTabIndex = Inst.nTabIndex;
	pIn->GuiMacro.nSplitIndex = Inst.nSplitIndex;
	lstrcpyW(pIn->GuiMacro.sMacro, asCmdArg);

	pOut = ExecuteGuiCmd(hCallWnd, pIn, ghConWnd);

	if (pOut)
	{
		if (pOut->GuiMacro.nSucceeded)
		{
			// Только если текущая консоль запущена В ConEmu
			if (ghConEmuWnd)
			{
				// Передать переменную в родительский процесс
				// Сработает только если включены хуки (Inject ConEmuHk)
				// И может игнорироваться некоторыми шеллами (если победить не удастся)
				DoExportEnv(CEGUIMACRORETENVVAR, ea_ExportCon, true/*bSilent*/);
			}

			// Let reuse `-Silent` switch
			if (!gbPrefereSilentMode || IsOutputRedirected())
			{
				// Show macro result in StdOutput
				_wprintf(pOut->GuiMacro.sMacro);

				// PowerShell... it does not insert linefeed
				if (!IsOutputRedirected())
					_wprintf(L"\n");
			}
			iRc = CERR_GUIMACRO_SUCCEEDED; // OK
		}
		ExecuteFreeResult(pOut);
	}
	ExecuteFreeResult(pIn);
	return iRc;
}

HMODULE LoadConEmuHk()
{
	HMODULE hHooks = NULL;
	LPCWSTR pszHooksName = WIN3264TEST(L"ConEmuHk.dll",L"ConEmuHk64.dll");

	if ((hHooks = GetModuleHandle(pszHooksName)) == NULL)
	{
		wchar_t szSelf[MAX_PATH];
		if (GetModuleFileName(ghOurModule, szSelf, countof(szSelf)))
		{
			wchar_t* pszName = (wchar_t*)PointToName(szSelf);
			int nSelfName = lstrlen(pszName);
			_ASSERTE(nSelfName == lstrlen(pszHooksName));
			lstrcpyn(pszName, pszHooksName, nSelfName+1);

			// ConEmuHk.dll / ConEmuHk64.dll
			hHooks = LoadLibrary(szSelf);
		}
	}

	return hHooks;
}

int DoOutput(ConEmuExecAction eExecAction, LPCWSTR asCmdArg)
{
	int iRc = 0;
	wchar_t* pszTemp = NULL;
	wchar_t* pszLine = NULL;
	char*    pszOem = NULL;
	char*    pszFile = NULL;
	LPCWSTR  pszText = NULL;
	DWORD    cchLen = 0, dwWritten = 0;
	HANDLE   hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	bool     bAddNewLine = true;
	bool     bProcessed = true;
	bool     bToBottom = false;
	CmdArg   szArg;
	HANDLE   hFile = NULL;
	DWORD    DefaultCP = 0;

	_ASSERTE(asCmdArg && (*asCmdArg != L' '));
	asCmdArg = SkipNonPrintable(asCmdArg);

	while ((*asCmdArg == L'-' || *asCmdArg == L'/') && (NextArg(&asCmdArg, szArg) == 0))
	{
		wchar_t* psz;

		// Make uniform
		if (szArg.ms_Arg[0] == L'/')
			szArg.ms_Arg[0] = L'-';

		// Do not CRLF after printing
		if (lstrcmpi(szArg, L"-n") == 0)
			bAddNewLine = false;
		// ‘RAW’: Do not process ANSI and specials (^e^[^r^n^t^b...)
		else if (lstrcmpi(szArg, L"-r") == 0)
			bProcessed = false;
		// Scroll to bottom of screen buffer (ConEmu TrueColor buffer compatible)
		else if (lstrcmpi(szArg, L"-b") == 0)
			bToBottom = true;
		// Forced codepage of typed text file
		else if (isDigit(szArg.ms_Arg[1]))
			DefaultCP = wcstoul(szArg, &psz, 10);
	}

	_ASSERTE(asCmdArg && (*asCmdArg != L' '));
	asCmdArg = SkipNonPrintable(asCmdArg);

	if (eExecAction == ea_OutType)
	{
		if (NextArg(&asCmdArg, szArg) == 0)
		{
			DWORD nSize = 0, nErrCode = 0;
			int iRead = ReadTextFile(szArg, (1<<24), pszTemp, cchLen, nErrCode, DefaultCP);
			if (iRead < 0)
			{
				wchar_t szInfo[100];
				_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"\r\nCode=%i, Error=%u\r\n", iRead, nErrCode);
				pszTemp = lstrmerge(L"Reading source file failed!\r\n", szArg, szInfo);
				cchLen = pszTemp ? lstrlen(pszTemp) : 0;
				iRc = 4;
			}
			pszText = pszTemp;
		}
	}
	else if (eExecAction == ea_OutEcho)
	{
		// Get the string
		int nLen = lstrlen(asCmdArg);
		if ((nLen < 1) && !bAddNewLine)
			goto wrap;

		// Cut double-quotes, trailing spaces, process ASCII analogues
		if (nLen > 0)
		{
			int j = nLen-1;
			pszLine = lstrdup(asCmdArg,2/*Add two extra wchar_t*/);
			if (!pszLine)
			{
				iRc = 100; goto wrap;
			}

			while ((j > 0) && (pszLine[j] == L' '))
			{
				pszLine[j--] = 0;
			}

			if (((nLen > 2) && (asCmdArg[0] == L'"'))
				&& ((j > 0) && (pszLine[j] == L'"')))
			{
				pszLine[j] = 0;
				asCmdArg = pszLine + 1;
			}
			else
			{
				asCmdArg = pszLine;
				j++;
				_ASSERTE(j >= 0 && j <= nLen);
			}

			if (bAddNewLine)
			{
				pszLine[j++] = L'\r';
				pszLine[j++] = L'\n';
				pszLine[j] = 0;
				bAddNewLine = false; // Already prepared
			}

			// Process special symbols: ^e^[^r^n^t^b
			if (bProcessed)
			{
				wchar_t* pszDst = wcschr(pszLine, L'^');
				if (pszDst)
				{
					const wchar_t* pszSrc = pszDst;
					const wchar_t* pszEnd = pszLine + j;
					while (pszSrc < pszEnd)
					{
						if (*pszSrc == L'^')
						{
							switch (*(++pszSrc))
							{
							case L'^':
								*pszDst = L'^'; break;
							case L'r': case L'R':
								*pszDst = L'\r'; break;
							case L'n': case L'N':
								*pszDst = L'\n'; break;
							case L't': case L'T':
								*pszDst = L'\t'; break;
							case L'a': case L'A':
								*pszDst = 7; break;
							case L'b': case L'B':
								*pszDst = L'\b'; break;
							case L'e': case L'E': case L'[':
								*pszDst = 27; break;
							default:
								// Unknown ctrl-sequence, bypass
								*(pszDst++) = *(pszSrc++);
								continue;
							}
							pszDst++; pszSrc++;
						}
						else
						{
							*(pszDst++) = *(pszSrc++);
						}
					}
					// Was processed? Zero terminate it.
					*pszDst = 0;
				}
			}
		}

		if (bAddNewLine)
		{
			pszTemp = lstrmerge(asCmdArg, L"\r\n");
			pszText = pszTemp;
		}
		else
		{
			pszText = asCmdArg;
		}
		cchLen = pszText ? lstrlen(pszText) : 0;
	}

	if (bToBottom)
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi = {};
		if (GetConsoleScreenBufferInfo(hOut, &csbi))
		{
			COORD crBottom = {0, csbi.dwSize.Y-1};
			SetConsoleCursorPosition(hOut, crBottom);
			int Y1 = crBottom.Y - (csbi.srWindow.Bottom-csbi.srWindow.Top);
			SMALL_RECT srWindow = {0, Y1, csbi.srWindow.Right-srWindow.Left, crBottom.Y};
			SetConsoleWindowInfo(hOut, TRUE, &srWindow);
		}
	}

	if (!pszText || !cchLen)
	{
		iRc = 1;
		goto wrap;
	}

	if (!IsOutputRedirected())
	{
		BOOL bRc;
		typedef BOOL (WINAPI* WriteProcessed_t)(LPCWSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten);
		WriteProcessed_t WriteProcessed = NULL;
		HMODULE hHooks = NULL;
		if (bProcessed)
		{
			// ConEmuHk.dll / ConEmuHk64.dll
			if ((hHooks = LoadConEmuHk()) != NULL)
			{
				WriteProcessed = (WriteProcessed_t)GetProcAddress(hHooks, "WriteProcessed");
			}
		}

		// If possible - use processed (with ANSI support) output
		if (WriteProcessed)
		{
			bRc = WriteProcessed(pszText, cchLen, &dwWritten);
		}
		else
		{
			bRc = WriteConsoleW(hOut, pszText, cchLen, &dwWritten, 0);
		}

		if (!bRc)
			iRc = 3;
	}
	else
	{
		// Current process output was redirected to file!

		UINT  cp = GetConsoleOutputCP();
		int   nDstLen = WideCharToMultiByte(cp, 0, pszText, cchLen, NULL, 0, NULL, NULL);
		if (nDstLen < 1)
		{
			iRc = 2;
			goto wrap;
		}

		pszOem = (char*)malloc(nDstLen);
		if (pszOem)
		{
			int nWrite = WideCharToMultiByte(cp, 0, pszText, cchLen, pszOem, nDstLen, NULL, NULL);
			if (nWrite > 1)
			{
				WriteFile(hOut, pszOem, nWrite, &dwWritten, 0);
			}
		}
	}

wrap:
	SafeFree(pszLine);
	SafeFree(pszTemp);
	SafeFree(pszOem);
	return iRc;
}

int DoStoreCWD(LPCWSTR asCmdArg)
{
	int iRc = 1;
	CmdArg szDir;

	if ((NextArg(&asCmdArg, szDir) != 0) || szDir.IsEmpty())
	{
		if (GetDirectory(szDir) <= 0)
			goto wrap;
	}

	// Sends CECMD_STORECURDIR into RConServer
	SendCurrentDirectory(ghConWnd, szDir);
	iRc = 0;
wrap:
	return iRc;
}

int DoExecAction(ConEmuExecAction eExecAction, LPCWSTR asCmdArg /* rest of cmdline */, MacroInstance& Inst)
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
			iRc = DoGuiMacro(asCmdArg, Inst);
			break;
		}
	case ea_CheckUnicodeFont:
		{
			iRc = CheckUnicodeFont();
			break;
		}
	case ea_OsVerInfo:
		{
			iRc = OsVerInfo();
			break;
		}
	case ea_ExportCon:
	case ea_ExportTab:
	case ea_ExportGui:
	case ea_ExportAll:
		{
			iRc = DoExportEnv(asCmdArg, eExecAction, gbPrefereSilentMode);
			break;
		}
	case ea_Download:
		{
			iRc = DoDownload(asCmdArg);
			break;
		}
	case ea_ParseArgs:
		{
			iRc = DoParseArgs(asCmdArg);
			break;
		}
	case ea_ErrorLevel:
		{
			wchar_t* pszEnd = NULL;
			iRc = wcstol(asCmdArg, &pszEnd, 10);
			break;
		}
	case ea_OutEcho:
	case ea_OutType:
		{
			iRc = DoOutput(eExecAction, asCmdArg);
			break;
		}
	case ea_StoreCWD:
		{
			iRc = DoStoreCWD(asCmdArg);
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
	SetConEmuWorkEnvVar(ghOurModule);
}

// 1. Заменить подстановки вида: !ConEmuHWND!, !ConEmuDrawHWND!, !ConEmuBackHWND!, !ConEmuWorkDir!
// 2. Развернуть переменные окружения (PowerShell, например, не признает переменные в качестве параметров)
wchar_t* ParseConEmuSubst(LPCWSTR asCmd)
{
	if (!asCmd || !*asCmd)
	{
		LogFunction(L"ParseConEmuSubst - skipped");
		return NULL;
	}

	LogFunction(L"ParseConEmuSubst");

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
			LogFunction(L"Variables must be set already!");
			_ASSERTE(*szDbg && "Variables must be set already!");
			break; // другие не проверять - лишние ассерты
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

	wchar_t* pszExpand = ExpandEnvStr(pszCmdCopy ? pszCmdCopy : asCmd);

	SafeFree(pszCmdCopy);
	return pszExpand;
}

BOOL SetTitle(LPCWSTR lsTitle)
{
	LogFunction(L"SetTitle");

	LPCWSTR pszSetTitle = lsTitle ? lsTitle : L"";

	#ifdef SHOW_SETCONTITLE_MSGBOX
	MessageBox(NULL, pszSetTitle, WIN3264TEST(L"ConEmuCD - set title",L"ConEmuCD64 - set title"), MB_SYSTEMMODAL);
	#endif

	BOOL bRc = SetConsoleTitle(pszSetTitle);

	if (gpLogSize)
	{
		wchar_t* pszLog = lstrmerge(bRc ? L"Done: " : L"Fail: ", pszSetTitle);
		LogFunction(pszLog);
		SafeFree(pszLog);
	}

	return bRc;
}

void UpdateConsoleTitle()
{
	LogFunction(L"UpdateConsoleTitle");

	CmdArg   szTemp;
	wchar_t *pszBuffer = NULL;
	LPCWSTR  pszSetTitle = NULL, pszCopy;
	LPCWSTR  pszReq = gpszForcedTitle ? gpszForcedTitle : gpszRunCmd;

	if (!pszReq || !*pszReq)
	{
		// Не должны сюда попадать - сброс заголовка не допустим
		#ifdef _DEBUG
		if (!(gbAttachMode & am_Modes))
		{
			_ASSERTE(pszReq && *pszReq);
		}
		#endif
		return;
	}

	pszBuffer = ParseConEmuSubst(pszReq);
	if (pszBuffer)
		pszReq = pszBuffer;
	pszCopy = pszReq;

	if (!gpszForcedTitle && (NextArg(&pszCopy, szTemp) == 0))
	{
		wchar_t* pszName = (wchar_t*)PointToName(szTemp.ms_Arg);
		wchar_t* pszExt = (wchar_t*)PointToExt(pszName);
		if (pszExt)
			*pszExt = 0;
		pszSetTitle = pszName;
	}
	else
	{
		pszSetTitle = pszReq;
	}

	// Need to change title? Do it.
	if (pszSetTitle && *pszSetTitle)
	{
		#ifdef _DEBUG
		int nLen = 4096; //GetWindowTextLength(ghConWnd); -- KIS2009 гундит "Посылка оконного сообщения"...
		gpszPrevConTitle = (wchar_t*)calloc(nLen+1,2);
		if (gpszPrevConTitle)
			GetConsoleTitleW(gpszPrevConTitle, nLen+1);
		#endif

		SetTitle(pszSetTitle);
	}

	SafeFree(pszBuffer);
}

void CdToProfileDir()
{
	BOOL bRc = FALSE;
	wchar_t szPath[MAX_PATH] = L"";
	HRESULT hr = SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, szPath);
	if (FAILED(hr))
		GetEnvironmentVariable(L"USERPROFILE", szPath, countof(szPath));
	if (szPath[0])
		bRc = SetCurrentDirectory(szPath);
	// Write action to log file
	if (gpLogSize)
	{
		wchar_t* pszMsg = lstrmerge(bRc ? L"Work dir changed to %USERPROFILE%: " : L"CD failed to %USERPROFILE%: ", szPath);
		LogFunction(pszMsg);
		SafeFree(pszMsg);
	}
}

#ifndef WIN64
void CheckNeedSkipWowChange(LPCWSTR asCmdLine)
{
	LogFunction(L"CheckNeedSkipWowChange");

	// Команды вида: C:\Windows\SysNative\reg.exe Query "HKCU\Software\Far2"|find "Far"
	// Для них нельзя отключать редиректор (wow.Disable()), иначе SysNative будет недоступен
	if (IsWindows64())
	{
		LPCWSTR pszTest = asCmdLine;
		CmdArg szApp;

		if (NextArg(&pszTest, szApp) == 0)
		{
			wchar_t szSysnative[MAX_PATH+32];
			int nLen = GetWindowsDirectory(szSysnative, MAX_PATH);

			if (nLen >= 2 && nLen < MAX_PATH)
			{
				AddEndSlash(szSysnative, countof(szSysnative));
				wcscat_c(szSysnative, L"Sysnative\\");
				nLen = lstrlenW(szSysnative);
				int nAppLen = lstrlenW(szApp);

				if (nAppLen > nLen)
				{
					szApp.ms_Arg[nLen] = 0;

					if (lstrcmpiW(szApp, szSysnative) == 0)
					{
						gbSkipWowChange = TRUE;
					}
				}
			}
		}
	}
}
#endif

// When DefTerm debug console is started for Win32 app
// we need to allocate hidden console, and there is no
// active process, until parent DevEnv successfully starts
// new debugging process session
DWORD WaitForRootConsoleProcess(DWORD nTimeout)
{
	if (pfnGetConsoleProcessList==NULL)
	{
		_ASSERTE(FALSE && "Attach to console app was requested, but required WinXP or higher!");
		return 0;
	}

	_ASSERTE(gbCreatingHiddenConsole);
	_ASSERTE(ghConWnd!=NULL);

	DWORD nFoundPID = 0;
	DWORD nStart = GetTickCount(), nDelta = 0;
	DWORD nProcesses[20] = {}, nProcCount, i;

	PROCESSENTRY32 pi = {};
	GetProcessInfo(gnSelfPID, &pi);

	while (!nFoundPID && (nDelta < nTimeout))
	{
		Sleep(50);
		nProcCount = pfnGetConsoleProcessList(nProcesses, countof(nProcesses));

		for (i = 0; i < nProcCount; i++)
		{
			DWORD nPID = nProcesses[i];
			if (nPID && (nPID != gnSelfPID) && (nPID != pi.th32ParentProcessID))
			{
				nFoundPID = nPID;
				break;
			}
		}

		nDelta = (GetTickCount() - nStart);
	}

	if (!nFoundPID)
	{
		apiShowWindow(ghConWnd, SW_SHOWNORMAL);
		_ASSERTE(FALSE && "Was unable to find starting process");
	}

	return nFoundPID;
}

void ApplyProcessSetEnvCmd()
{
	if (gpSetEnv)
	{
		gpSetEnv->Apply();
	}
}

// Use here 'wchar_t*' because we replace '=' with '\0' here
void ApplyEnvironmentCommands(wchar_t* pszCommand)
{
	UINT nSetCP = 0; // Postponed

	// Do loop
	while (*pszCommand)
	{
		if (lstrcmpi(pszCommand, L"set") == 0)
		{
			// Get variable "Name=Value"
			wchar_t* pszName = pszCommand + lstrlen(pszCommand) + 1;
			// Get next command
			pszCommand = pszName + lstrlen(pszName) + 1;
			// Process it
			wchar_t* pszValue = (wchar_t*)wcschr(pszName, L'=');
			if (pszValue)
				*(pszValue++) = 0;
			wchar_t* pszExpand = ExpandEnvStr(pszValue);
			SetEnvironmentVariable(pszName, pszExpand ? pszExpand : pszValue);
			SafeFree(pszExpand);
		}
		else if (lstrcmpi(pszCommand, L"alias") == 0)
		{
			// Get variable "Name=Value"
			wchar_t* pszName = pszCommand + lstrlen(pszCommand) + 1;
			// Get next command
			pszCommand = pszName + lstrlen(pszName) + 1;
			// Process it
			wchar_t* pszValue = (wchar_t*)wcschr(pszName, L'=');
			if (pszValue)
				*(pszValue++) = 0;
			// NULL will remove alias
			// We set aliases for "cmd.exe" executable, as Far Manager supports too
			AddConsoleAlias(pszName, *pszValue ? pszValue : NULL, L"cmd.exe");
		}
		else if (lstrcmpi(pszCommand, L"chcp") == 0)
		{
			wchar_t* pszCP = pszCommand + lstrlen(pszCommand) + 1;
			pszCommand = pszCP + lstrlen(pszCP) + 1;
			nSetCP = GetCpFromString(pszCP);
		}
		else
		{
			wchar_t* pszUnsupported = pszCommand + lstrlen(pszCommand) + 1;
			_ASSERTE(FALSE && "Command was not implemented yet");
			pszCommand = pszUnsupported + lstrlen(pszUnsupported) + 1;
		}
	}

	// Postponed commands?
	if (nSetCP)
	{
		SetConsoleCpHelper(nSetCP);
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
	CmdArg szArg;
	CmdArg szExeTest;
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
	bool lbNeedCdToProfileDir = false;

	ConEmuStateCheck eStateCheck = ec_None;
	ConEmuExecAction eExecAction = ea_None;
	MacroInstance MacroInst = {}; // Special ConEmu instance for GUIMACRO and other options

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

		// Following code wants '/'style arguments
		// Change '-'style to '/'style
		if (szArg[0] == L'-')
			szArg.SetAt(0, L'/');
		else if (szArg[0] != L'/')
			continue;

		#ifdef _DEBUG
		if (lstrcmpi(szArg, L"/DEBUGTRAP")==0)
		{
			int i, j = 1; j--; i = 1 / j;
		}
		else
		#endif

		// **** Unit tests ****
		if (lstrcmpi(szArg, L"/Args")==0 || lstrcmpi(szArg, L"/ParseArgs")==0)
		{
			eExecAction = ea_ParseArgs;
			break;
		}
		else if (lstrcmpi(szArg, L"/CheckUnicode")==0)
		{
			eExecAction = ea_CheckUnicodeFont;
			break;
		}
		else if (lstrcmpi(szArg, L"/OsVerInfo")==0)
		{
			eExecAction = ea_OsVerInfo;
			break;
		}
		else if (lstrcmpi(szArg, L"/ErrorLevel")==0)
		{
			eExecAction = ea_ErrorLevel;
			break;
		}
		else if (lstrcmpi(szArg, L"/Result")==0)
		{
			gbPrintRetErrLevel = TRUE;
		}
		else if (lstrcmpi(szArg, L"/echo")==0 || lstrcmpi(szArg, L"/e")==0)
		{
			eExecAction = ea_OutEcho;
			break;
		}
		else if (lstrcmpi(szArg, L"/type")==0 || lstrcmpi(szArg, L"/t")==0)
		{
			eExecAction = ea_OutType;
			break;
		}
		// **** Regular use ****
		else if (wcsncmp(szArg, L"/REGCONFONT=", 12)==0)
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
		// /GUIMACRO[:PID|HWND] <Macro string>
		else if (lstrcmpni(szArg, L"/GUIMACRO", 9) == 0)
		{
			// Все что в lsCmdLine - выполнить в Gui
			ArgGuiMacro(szArg, MacroInst);
			eExecAction = ea_GuiMacro;
			break;
		}
		else if (lstrcmpi(szArg, L"/STORECWD") == 0)
		{
			eExecAction = ea_StoreCWD;
			break;
		}
		else if (lstrcmpi(szArg, L"/SILENT")==0)
		{
			gbPrefereSilentMode = true;
		}
		else if (lstrcmpni(szArg, L"/EXPORT", 7)==0)
		{
			//_ASSERTE(FALSE && "Continue to export");
			if (lstrcmpi(szArg, L"/EXPORT=ALL")==0 || lstrcmpi(szArg, L"/EXPORTALL")==0)
				eExecAction = ea_ExportAll;
			else if (lstrcmpi(szArg, L"/EXPORT=CON")==0 || lstrcmpi(szArg, L"/EXPORTCON")==0)
				eExecAction = ea_ExportCon;
			else if (lstrcmpi(szArg, L"/EXPORT=GUI")==0 || lstrcmpi(szArg, L"/EXPORTGUI")==0)
				eExecAction = ea_ExportGui;
			else
				eExecAction = ea_ExportTab;
			break;
		}
		else if (lstrcmpi(szArg, L"/Download")==0)
		{
			eExecAction = ea_Download;
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
		else if (lstrcmpi(szArg, L"/IsAdmin")==0)
		{
			eStateCheck = ec_IsAdmin;
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
		else if (wcscmp(szArg, L"/SKIPHOOKERS")==0)
		{
			gbSkipHookersCheck = true;
		}
		else if (wcscmp(szArg, L"/ADMIN")==0)
		{
			#if defined(SHOW_ATTACH_MSGBOX)
			if (!IsDebuggerPresent())
			{
				wchar_t szTitle[100]; _wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"%s PID=%u /ADMIN", gsModuleName, gnSelfPID);
				const wchar_t* pszCmdLine = GetCommandLineW();
				MessageBox(NULL,pszCmdLine,szTitle,MB_SYSTEMMODAL);
			}
			#endif

			gbAttachMode |= am_Admin;
			gnRunMode = RM_SERVER;
		}
		else if (wcscmp(szArg, L"/ATTACH")==0)
		{
			#if defined(SHOW_ATTACH_MSGBOX)
			if (!IsDebuggerPresent())
			{
				wchar_t szTitle[100]; _wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"%s PID=%u /ATTACH", gsModuleName, gnSelfPID);
				const wchar_t* pszCmdLine = GetCommandLineW();
				MessageBox(NULL,pszCmdLine,szTitle,MB_SYSTEMMODAL);
			}
			#endif

			if (!(gbAttachMode & am_Modes))
				gbAttachMode |= am_Simple;
			gnRunMode = RM_SERVER;
		}
		else if ((lstrcmpi(szArg, L"/AUTOATTACH")==0) || (lstrcmpi(szArg, L"/ATTACHDEFTERM")==0))
		{
			#if defined(SHOW_ATTACH_MSGBOX)
			if (!IsDebuggerPresent())
			{
				wchar_t szTitle[100]; _wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"%s PID=%u %s", gsModuleName, gnSelfPID, szArg.ms_Arg);
				const wchar_t* pszCmdLine = GetCommandLineW();
				MessageBox(NULL,pszCmdLine,szTitle,MB_SYSTEMMODAL);
			}
			#endif

			gnRunMode = RM_SERVER;
			gbAttachMode |= am_Auto;
			gbAlienMode = TRUE;
			gbNoCreateProcess = TRUE;
			if (lstrcmpi(szArg, L"/AUTOATTACH")==0)
				gbAttachMode |= am_Async;
			if (lstrcmpi(szArg, L"/ATTACHDEFTERM")==0)
				gbAttachMode |= am_DefTerm;

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
				wchar_t szTitle[100]; _wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"%s PID=%u /GUIATTACH", gsModuleName, gnSelfPID);
				const wchar_t* pszCmdLine = GetCommandLineW();
				MessageBox(NULL,pszCmdLine,szTitle,MB_SYSTEMMODAL);
			}
			#endif

			if (!(gbAttachMode & am_Modes))
				gbAttachMode |= am_Simple;
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
			pszStart = szArg.ms_Arg+14;
			gpSrv->dwParentFarPID = wcstoul(pszStart, &pszEnd, 10);
		}
		else if (wcscmp(szArg, L"/CREATECON")==0)
		{
			gbCreatingHiddenConsole = TRUE;
			//_ASSERTE(FALSE && "Continue to create con");
		}
		else if (wcscmp(szArg, L"/ROOTEXE")==0)
		{
			if (0 == NextArg(&lsCmdLine, szArg))
				gpszRootExe = lstrmerge(L"\"", szArg, L"\"");
		}
		else if (wcsncmp(szArg, L"/PID=", 5)==0 || wcsncmp(szArg, L"/TRMPID=", 8)==0
			|| wcsncmp(szArg, L"/FARPID=", 8)==0 || wcsncmp(szArg, L"/CONPID=", 8)==0)
		{
			gnRunMode = RM_SERVER;
			gbNoCreateProcess = TRUE; // Процесс УЖЕ запущен
			gbAlienMode = TRUE; // Консоль создана НЕ нами
			wchar_t* pszEnd = NULL, *pszStart;

			if (wcsncmp(szArg, L"/TRMPID=", 8)==0)
			{
				// This is called from *.vshost.exe when "AllocConsole" just created
				gbDefTermCall = TRUE;
				gbDontInjectConEmuHk = TRUE;
				pszStart = szArg.ms_Arg+8;
			}
			else if (wcsncmp(szArg, L"/FARPID=", 8)==0)
			{
				gbAttachFromFar = TRUE;
				gbRootIsCmdExe = FALSE;
				pszStart = szArg.ms_Arg+8;
			}
			else if (wcsncmp(szArg, L"/CONPID=", 8)==0)
			{
				//_ASSERTE(FALSE && "Continue to alternative attach mode");
				gbAlternativeAttach = TRUE;
				gbRootIsCmdExe = FALSE;
				pszStart = szArg.ms_Arg+8;
			}
			else
			{
				pszStart = szArg.ms_Arg+5;
			}

			gpSrv->dwRootProcess = wcstoul(pszStart, &pszEnd, 10);

			if ((gpSrv->dwRootProcess == 0) && gbCreatingHiddenConsole)
			{
				gpSrv->dwRootProcess = WaitForRootConsoleProcess(30000);
			}

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
						LODWORD(hFindConWnd), nFindConPID, piCon.szExeFile, szTitle,
						LODWORD(hSaveCon)
						);

					_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"%s: PID=%u", gsModuleName, GetCurrentProcessId());

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

				// Need to be set, because of new console === new handler
				SetConsoleCtrlHandler((PHANDLER_ROUTINE)HandlerRoutine, true);

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
				_ASSERTE(FALSE && "Attach to GUI was requested, but invalid PID specified");
				return CERR_CARGUMENT;
			}
		}
		else if (wcsncmp(szArg, L"/CINMODE=", 9)==0)
		{
			wchar_t* pszEnd = NULL, *pszStart = szArg.ms_Arg+9;
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
		else if (lstrcmpni(szArg, L"/LOG", 4) == 0) //-V112
		{
			int nLevel = 0;
			if (szArg[4]==L'1') nLevel = 1; else if (szArg[4]>=L'2') nLevel = 2;

			CreateLogSizeFile(nLevel);
		}
		else if (wcsncmp(szArg, L"/GID=", 5)==0)
		{
			gnRunMode = RM_SERVER;
			wchar_t* pszEnd = NULL;
			gnConEmuPID = wcstoul(szArg+5, &pszEnd, 10);

			if (gnConEmuPID == 0)
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
				_ASSERTE(gnConEmuPID == 0);
				gnConEmuPID = 0;
				gpSrv->bRequestNewGuiWnd = TRUE;
			}
			else
			{
				LPCWSTR pszDescr = szArg+7;
				if (pszDescr[0] == L'0' && (pszDescr[1] == L'x' || pszDescr[1] == L'X'))
					pszDescr += 2; // That may be useful for calling from batch files
				gpSrv->hGuiWnd = (HWND)wcstoul(pszDescr, &pszEnd, 16);
				gpSrv->bRequestNewGuiWnd = FALSE;

				if ((gpSrv->hGuiWnd) == NULL || !IsWindow(gpSrv->hGuiWnd))
				{
					_printf("Invalid GUI HWND specified:\n");
					_wprintf(GetCommandLineW());
					_printf("\n");
					_ASSERTE(FALSE && "Invalid window was specified in /GHWND arg");
					return CERR_CARGUMENT;
				}

				DWORD nPID = 0;
				GetWindowThreadProcessId(gpSrv->hGuiWnd, &nPID);
				_ASSERTE(gnConEmuPID == 0 || gnConEmuPID == nPID);
				gnConEmuPID = nPID;
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

				HANDLE hConOut = ghConOut;
				CONSOLE_SCREEN_BUFFER_INFO csbi5 = {};
				GetConsoleScreenBufferInfo(hConOut, &csbi5);

				if (gnDefTextColors || gnDefPopupColors)
				{
					BOOL bPassed = FALSE;

					if (gnDefPopupColors && (gnOsVer >= 0x600))
					{
						MY_CONSOLE_SCREEN_BUFFER_INFOEX csbi = {sizeof(csbi)};
						if (apiGetConsoleScreenBufferInfoEx(hConOut, &csbi))
						{
							// Microsoft bug? When console is started elevated - it does NOT show
							// required attributes, BUT GetConsoleScreenBufferInfoEx returns them.
							if (!(gbAttachMode & am_Admin)
								&& (!gnDefTextColors || (csbi.wAttributes = gnDefTextColors))
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
								//	apiShowWindow(ghConWnd, SW_SHOWMINNOACTIVE);
								//	#ifdef _DEBUG
								//	apiShowWindow(ghConWnd, SW_SHOWNORMAL);
								//	apiShowWindow(ghConWnd, SW_HIDE);
								//	#endif
								//}

								bPassed = apiSetConsoleScreenBufferInfoEx(hConOut, &csbi);

								// Что-то Win7 хулиганит
								if (!gbVisibleOnStartup)
								{
									apiShowWindow(ghConWnd, SW_HIDE);
								}
							}
						}
					}


					if (!bPassed && gnDefTextColors)
					{
						SetConsoleTextAttribute(hConOut, gnDefTextColors);
						RefillConsoleAttributes(csbi5, csbi5.wAttributes, gnDefTextColors);
					}
				}
			}
		}
		else if (lstrcmpni(szArg, L"/DEBUGPID=", 10)==0)
		{
			//gnRunMode = RM_SERVER; -- не будем ставить, RM_UNDEFINED будет признаком того, что просто хотят дебаггер

			gbNoCreateProcess = TRUE;
			gpSrv->DbgInfo.bDebugProcess = TRUE;
			gpSrv->DbgInfo.bDebugProcessTree = FALSE;

			wchar_t* pszEnd = NULL;
			gpSrv->dwRootProcess = wcstoul(szArg+10, &pszEnd, 10);

			if (gpSrv->dwRootProcess == 0)
			{
				_printf("Debug of process was requested, but invalid PID specified:\n");
				_wprintf(GetCommandLineW());
				_printf("\n");
				_ASSERTE(FALSE);
				return CERR_CARGUMENT;
			}

			// "Comma" is a mark that debug/dump was requested for a bunch of processes
			if (pszEnd && (*pszEnd == L','))
			{
				gpSrv->DbgInfo.bDebugMultiProcess = TRUE;
				gpSrv->DbgInfo.pDebugAttachProcesses = new MArray<DWORD>;
				while (pszEnd && (*pszEnd == L',') && *(pszEnd+1))
				{
					DWORD nPID = wcstoul(pszEnd+1, &pszEnd, 10);
					if (nPID != 0)
						gpSrv->DbgInfo.pDebugAttachProcesses->push_back(nPID);
				}
			}
		}
		else if (lstrcmpi(szArg, L"/DEBUGEXE")==0 || lstrcmpi(szArg, L"/DEBUGTREE")==0)
		{
			//gnRunMode = RM_SERVER; -- не будем ставить, RM_UNDEFINED будет признаком того, что просто хотят дебаггер

			_ASSERTE(gpSrv->DbgInfo.bDebugProcess==FALSE);

			gbNoCreateProcess = TRUE;
			gpSrv->DbgInfo.bDebugProcess = TRUE;
			gpSrv->DbgInfo.bDebugProcessTree = (lstrcmpi(szArg, L"/DEBUGTREE")==0);

			wchar_t* pszLine = lstrdup(GetCommandLineW());
			if (!pszLine || !*pszLine)
			{
				_printf("Debug of process was requested, but GetCommandLineW failed\n");
				_ASSERTE(FALSE);
				return CERR_CARGUMENT;
			}

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

			gpSrv->DbgInfo.pszDebuggingCmdLine = pszDebugCmd;

			break;
		}
		else if (lstrcmpi(szArg, L"/DUMP")==0)
		{
			gpSrv->DbgInfo.nDebugDumpProcess = 1;
		}
		else if (lstrcmpi(szArg, L"/MINIDUMP")==0 || lstrcmpi(szArg, L"/MINI")==0)
		{
			gpSrv->DbgInfo.nDebugDumpProcess = 2;
		}
		else if (lstrcmpi(szArg, L"/FULLDUMP")==0 || lstrcmpi(szArg, L"/FULL")==0)
		{
			gpSrv->DbgInfo.nDebugDumpProcess = 3;
		}
		else if (lstrcmpi(szArg, L"/PROFILECD")==0)
		{
			lbNeedCdToProfileDir = true;
		}
		else if (wcscmp(szArg, L"/A")==0 || wcscmp(szArg, L"/a")==0)
		{
			gnCmdUnicodeMode = 1;
		}
		else if (wcscmp(szArg, L"/U")==0 || wcscmp(szArg, L"/u")==0)
		{
			gnCmdUnicodeMode = 2;
		}
		else if (lstrcmpi(szArg, L"/CONFIG")==0)
		{
			if ((iRc = NextArg(&lsCmdLine, szArg)) != 0)
			{
				_ASSERTE(FALSE && "Config name was not specified!");
				_wprintf(L"Config name was not specified!\r\n");
				break;
			}
			// Reuse config if starting "ConEmu.exe" from console server!
			SetEnvironmentVariable(ENV_CONEMU_CONFIG_W, szArg);
		}
		else if (lstrcmpi(szArg, L"/ASYNC") == 0 || lstrcmpi(szArg, L"/FORK") == 0)
		{
			gbAsyncRun = TRUE;
		}
		else if (lstrcmpi(szArg, L"/NOINJECT")==0)
		{
			gbDontInjectConEmuHk = TRUE;
		}
		else if (lstrcmpi(szArg, L"/DOSBOX")==0)
		{
			gbUseDosBox = TRUE;
		}
		// После этих аргументов - идет то, что передается в CreateProcess!
		else if (lstrcmpi(szArg, L"/ROOT")==0)
		{
			#ifdef SHOW_SERVER_STARTED_MSGBOX
			ShowServerStartedMsgBox();
			#endif
			gnRunMode = RM_SERVER; gbNoCreateProcess = FALSE;
			gbAsyncRun = FALSE;
			SetWorkEnvVar();
			break; // lsCmdLine уже указывает на запускаемую программу
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
		else
		{
			_ASSERTE(FALSE && "Unknown switch!");
			_wprintf(L"Unknown switch: ");
			_wprintf(szArg);
			_wprintf(L"\r\n");
		}
	}

	LogFunction(L"ParseCommandLine{in-progress}");

	// Switch "/PROFILECD" used when server to be started under different credentials as GUI.
	// So, we need to do "cd %USERPROFILE%" which is more suitable to user.
	if (lbNeedCdToProfileDir)
	{
		CdToProfileDir();
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
			iFRc = DoExecAction(eExecAction, lsCmdLine, MacroInst);
		}

		// И сразу на выход
		gbInShutdown = TRUE;
		return iFRc;
	}

	if ((gbAttachMode & am_DefTerm) && !gbParmVisibleSize)
	{
		// To avoid "small" and trimmed text after starting console
		_ASSERTE(gcrVisibleSize.X==80 && gcrVisibleSize.Y==25);
		gbParmVisibleSize = TRUE;
	}

	// Параметры из комстроки разобраны. Здесь могут уже быть известны
	// gpSrv->hGuiWnd {/GHWND}, gnConEmuPID {/GPID}, gpSrv->dwGuiAID {/AID}
	// gbAttachMode для ключей {/ADMIN}, {/ATTACH}, {/AUTOATTACH}, {/GUIATTACH}

	// В принципе, gbAttachMode включается и при "/ADMIN", но при запуске из ConEmu такого быть не может,
	// будут установлены и gpSrv->hGuiWnd, и gnConEmuPID

	// Issue 364, например, идет билд в VS, запускается CustomStep, в этот момент автоаттач нафиг не нужен
	// Теоретически, в Студии не должно бы быть запуска ConEmuC.exe, но он может оказаться в "COMSPEC", так что проверим.
	if (gbAttachMode && (gnRunMode == RM_SERVER) && (gnConEmuPID == 0))
	{
		//-- ассерт не нужен вроде
		//_ASSERTE(!gbAlternativeAttach && "Alternative mode must be already processed!");

		BOOL lbIsWindowVisible = FALSE;
		// Добавим проверку на telnet
		if (!ghConWnd
			|| !(lbIsWindowVisible = IsAutoAttachAllowed())
			|| isTerminalMode())
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

		if (!gbAlternativeAttach && !(gbAttachMode & am_DefTerm) && !gpSrv->dwRootProcess)
		{
			// В принципе, сюда мы можем попасть при запуске, например: "ConEmuC.exe /ADMIN /ROOT cmd"
			// Но только не при запуске "из ConEmu" (т.к. будут установлены gpSrv->hGuiWnd, gnConEmuPID)

			// Из батника убрал, покажем инфу тут
			PrintVersion();
			char szAutoRunMsg[128];
			_wsprintfA(szAutoRunMsg, SKIPLEN(countof(szAutoRunMsg)) "Starting attach autorun (NewWnd=%s)\n", gpSrv->bRequestNewGuiWnd ? "YES" : "NO");
			_printf(szAutoRunMsg);
		}
	}

	xf_check();

	// Debugger or minidump requested?
	// Switches ‘/DEBUGPID=PID1[,PID2[...]]’ to debug already running process
	// or ‘/DEBUGEXE <your command line>’ or ‘/DEBUGTREE <your command line>’
	// to start new process and debug it (and its children if ‘/DEBUGTREE’)
	if (gpSrv->DbgInfo.bDebugProcess)
	{
		_ASSERTE(gnRunMode == RM_UNDEFINED);
		// Run debugger thread and wait for its completion
		int iDbgRc = RunDebugger();
		return iDbgRc;
	}

	// Validate Сonsole (find it may be) or ChildGui process we need to attach into ConEmu window
	if ((gnRunMode == RM_SERVER) && (gbNoCreateProcess && gbAttachMode))
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

	xf_check();

	// iRc is result of our ‘NextArg(&lsCmdLine,...)’
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

	// Prepare our environment and GUI window
	if (gnRunMode == RM_SERVER)
	{
		// We need to reserve or start new ConEmu tab/window...

		// Если уже известен HWND ConEmu (root window)
		if (gpSrv->hGuiWnd)
		{
			DWORD nGuiPID = 0; GetWindowThreadProcessId(gpSrv->hGuiWnd, &nGuiPID);
			DWORD nWrongValue = 0;
			SetLastError(0);
			LGSResult lgsRc = ReloadGuiSettings(NULL, &nWrongValue);
			if (lgsRc < lgs_Succeeded)
			{
				wchar_t szLgsError[200], szLGS[80];
				_wsprintf(szLGS, SKIPCOUNT(szLGS) L"LGS=%u, Code=%u, GUI PID=%u, Srv PID=%u", lgsRc, GetLastError(), nGuiPID, GetCurrentProcessId());
				switch (lgsRc)
				{
				case lgs_WrongVersion:
					_wsprintf(szLgsError, SKIPCOUNT(szLgsError) L"Failed to load ConEmu info!\n"
						L"Found ProtocolVer=%u but Required=%u.\n"
						L"%s.\n"
						L"Please update all ConEmu components!",
						nWrongValue, (DWORD)CESERVER_REQ_VER, szLGS);
					break;
				case lgs_WrongSize:
					_wsprintf(szLgsError, SKIPCOUNT(szLgsError) L"Failed to load ConEmu info!\n"
						L"Found MapSize=%u but Required=%u."
						L"%s.\n"
						L"Please update all ConEmu components!",
						nWrongValue, (DWORD)sizeof(ConEmuGuiMapping), szLGS);
					break;
				default:
					_wsprintf(szLgsError, SKIPCOUNT(szLgsError) L"Failed to load ConEmu info!\n"
						L"%s.\n"
						L"Please update all ConEmu components!",
						szLGS);
				}
				// Add log info
				LogFunction(szLGS);
				// Show user message
				wchar_t szTitle[128];
				_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuC[Srv]: PID=%u", GetCurrentProcessId());
				MessageBox(NULL, szLgsError, szTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
				return CERR_WRONG_GUI_VERSION;
			}
		}
	}

	xf_check();

	if (gnRunMode == RM_COMSPEC)
	{
		// Может просили открыть новую консоль?
		if (IsNewConsoleArg(lsCmdLine))
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
					// попытаться найти открытый ConEmu
					hConEmu = FindWindowEx(NULL, NULL, VirtualConsoleClassMain, NULL);
					if (hConEmu)
						gbNonGuiMode = TRUE; // Чтобы не пытаться выполнить SendStopped (ибо некому)
				}

				int iNewConRc = CERR_RUNNEWCONSOLE;

				// Query current environment
				CEnvStrings strs(GetEnvironmentStringsW());

				DWORD nCmdLen = lstrlen(lsCmdLine)+1;
				CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_NEWCMD, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_NEWCMD)+((nCmdLen+strs.mcch_Length)*sizeof(wchar_t)));
				if (pIn)
				{
					pIn->NewCmd.hFromConWnd = ghConWnd;

					// ghConWnd may differs from parent process, but ENV_CONEMUDRAW_VAR_W would be inherited
					wchar_t* pszDcWnd = GetEnvVar(ENV_CONEMUDRAW_VAR_W);
					if (pszDcWnd && (pszDcWnd[0] == L'0') && (pszDcWnd[1] == L'x'))
					{
						wchar_t* pszEnd = NULL;
						pIn->NewCmd.hFromDcWnd.u = wcstoul(pszDcWnd+2, &pszEnd, 16);
					}
					SafeFree(pszDcWnd);

					GetCurrentDirectory(countof(pIn->NewCmd.szCurDir), pIn->NewCmd.szCurDir);
					pIn->NewCmd.SetCommand(lsCmdLine);
					pIn->NewCmd.SetEnvStrings(strs.ms_Strings, strs.mcch_Length);

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

		if (gnRunMode == RM_SERVER)
		{
			LogFunction(L"ProcessSetEnvCmd {set, title, chcp, etc.}");
			// Console may be started as follows:
			// "set PATH=C:\Program Files;%PATH%" & ... & cmd
			// Supported commands:
			//  set abc=val
			//  "set PATH=C:\Program Files;%PATH%"
			//  chcp [utf8|ansi|oem|<cp_no>]
			//  title "Console init title"
			CmdArg lsForcedTitle;
			if (!gpSetEnv)
				gpSetEnv = new CProcessEnvCmd();
			ProcessSetEnvCmd(lsCmdLine, true, &lsForcedTitle, gpSetEnv);
			if (!lsForcedTitle.IsEmpty())
				gpszForcedTitle = lsForcedTitle.Detach();
		}

		gpszCheck4NeedCmd = lsCmdLine; // Для отладки

		gbRunViaCmdExe = IsNeedCmd((gnRunMode == RM_SERVER), lsCmdLine, szExeTest,
			&pszArguments4EnvVar, &lbNeedCutStartEndQuot, &gbRootIsCmdExe, &bAlwaysConfirmExit, &bAutoDisableConfirmExit);

		if (gnConfirmExitParm == 0)
		{
			gbAlwaysConfirmExit = bAlwaysConfirmExit;
			gbAutoDisableConfirmExit = bAutoDisableConfirmExit;
		}
	}


	#ifndef WIN64
	// Команды вида: C:\Windows\SysNative\reg.exe Query "HKCU\Software\Far2"|find "Far"
	// Для них нельзя отключать редиректор (wow.Disable()), иначе SysNative будет недоступен
	CheckNeedSkipWowChange(lsCmdLine);
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
		gszComSpec[0] = 0;

		#ifdef _DEBUG
		// ComSpec больше не переопределяется.
		// Для надежности и проверки уже был вызван RemoveOldComSpecC()
		if (GetEnvironmentVariable(L"ComSpecC", gszComSpec, MAX_PATH))
		{
			_ASSERTE(gszComSpec[0] == 0);
		}
		#endif

		if (!GetEnvironmentVariable(L"ComSpec", gszComSpec, MAX_PATH) || gszComSpec[0] == 0)
			gszComSpec[0] = 0;

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

	size_t nCchLen = nCmdLine+1; // nCmdLine учитывает длину lsCmdLine + gszComSpec + еще чуть-чуть на "/C" и прочее
	gpszRunCmd = (wchar_t*)calloc(nCchLen,sizeof(wchar_t));

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

	// ====
	if (gbRunViaCmdExe)
	{
		CheckUnicodeMode();

		// -- для унификации - окавычиваем всегда
		gpszRunCmd[0] = L'"';
		_wcscpy_c(gpszRunCmd+1, nCchLen-1, gszComSpec);

		if (gnCmdUnicodeMode)
			_wcscat_c(gpszRunCmd, nCchLen, (gnCmdUnicodeMode == 2) ? L" /U" : L" /A");

		_wcscat_c(gpszRunCmd, nCchLen, gpSrv->bK ? L"\" /K " : L"\" /C ");

		// Собственно, командная строка
		_wcscat_c(gpszRunCmd, nCchLen, lsCmdLine);
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
	if (args.pszStartupDir && *args.pszStartupDir)
	{
		SetCurrentDirectory(args.pszStartupDir);
	}
	//
	gbRunInBackgroundTab = (args.BackgroundTab == crb_On);
	if (args.BufHeight == crb_On)
	{
		TODO("gcrBufferSize - и ширину буфера");
		gnBufferHeight = args.nBufHeight;
		gbParmBufSize = TRUE;
	}
	// DosBox?
	if (args.ForceDosBox == crb_On)
	{
		gbUseDosBox = TRUE;
	}
	// Overwrite mode in Prompt?
	if (args.OverwriteMode == crb_On)
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
	if (!gbStopExitWaitForKey)
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
	LogFunction(L"SendStarted");

	WARNING("Подозрение, что слишком много вызовов при старте сервера. Неаккуратно");

	static bool bSent = false;

	if (bSent)
	{
		_ASSERTE(FALSE && "SendStarted repetition");
		return; // отсылать только один раз
	}

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
		ConsoleMap.InitName(CECONMAPNAME, LODWORD(hConWnd)); //-V205
		const CESERVER_CONSOLE_MAPPING_HDR* pConsoleInfo = ConsoleMap.Open();

		if (!pConsoleInfo)
		{
			_ASSERTE(pConsoleInfo && "ConsoleMap was not initialized for AltServer/ComSpec");
		}
		else
		{
			nMainServerPID = pConsoleInfo->nServerPID;
			nAltServerPID = pConsoleInfo->nAltServerPID;
			nGuiPID = pConsoleInfo->nGuiPID;

			if (pConsoleInfo->cbSize >= sizeof(CESERVER_CONSOLE_MAPPING_HDR))
			{
				if (pConsoleInfo->nLogLevel)
					CreateLogSizeFile(pConsoleInfo->nLogLevel, pConsoleInfo);
			}

			//UnmapViewOfFile(pConsoleInfo);
			ConsoleMap.CloseMap();
		}

		if (nMainServerPID == 0)
		{
			gbNonGuiMode = TRUE; // Не посылать ExecuteGuiCmd при выходе. Это не наша консоль
			return; // Режим ComSpec, но сервера нет, соответственно, в GUI ничего посылать не нужно
		}
	}
	else
	{
		nGuiPID = gnConEmuPID;
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
		pIn->StartStop.dwAID = gpSrv->dwGuiAID;
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
		CmdArg lsRoot;

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
			pOut = ExecuteGuiCmd(ghConEmuWnd, pIn, ghConWnd);
		}
		else if (gnRunMode == RM_ALTSERVER)
		{
			// Подготовить хэндл своего процесса для MainServer
			pIn->StartStop.hServerProcessHandle = (u64)(DWORD_PTR)DuplicateProcessHandle(nMainServerPID);

			_ASSERTE(pIn->hdr.nCmd == CECMD_CMDSTARTSTOP);
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
				ghSendStartedThread = apiCreateThread(SendStartedThreadProc, pIn, &gnSendStartedThread, "SendStartedThreadProc");
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
			SetConEmuWindows(pOut->StartStopRet.hWnd, pOut->StartStopRet.hWndDc, pOut->StartStopRet.hWndBack);
			if (gpSrv)
			{
				_ASSERTE(gnConEmuPID == pOut->StartStopRet.dwPID);
				gnConEmuPID = pOut->StartStopRet.dwPID;
				#ifdef _DEBUG
				DWORD dwPID; GetWindowThreadProcessId(ghConEmuWnd, &dwPID);
				_ASSERTE(ghConEmuWnd==NULL || dwPID==gnConEmuPID);
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
	LogFunction(L"SendStopped");

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
			apiTerminateThread(ghSendStartedThread, 100);    // раз корректно не хочет...
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
void CreateLogSizeFile(int nLevel, const CESERVER_CONSOLE_MAPPING_HDR* pConsoleInfo /*= NULL*/)
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

	if (pszName && (pszName > szFile))
	{
		// If we were started from ConEmu, the pConsoleInfo must be defined...
		if (pConsoleInfo && pConsoleInfo->cbSize && pConsoleInfo->sConEmuExe[0])
		{
			lstrcpyn(szDir, pConsoleInfo->sConEmuExe, countof(szDir));
			wchar_t* pszConEmuExe = (wchar_t*)PointToName(szDir);
			if (pszConEmuExe)
			{
				*(pszConEmuExe-1) = 0;
				pszDir = szDir;
			}
		}

		if (!pszDir)
		{
			// if our exe lays in subfolder of ConEmu.exe?
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
				wchar_t szGuiFiles[] = L"\\ConEmu.exe" L"\0" L"\\ConEmu64.exe" L"\0\0";
				if (FilesExists(szDir, szGuiFiles))
				{
					pszDir = szFile; // GUI лежит в той же папке, что и "сервер"
				}
				else
				{
					// На уровень выше?
					*pszDir = 0;
					if (FilesExists(szDir, szGuiFiles))
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
	}

	_wcscpy_c(pszDot, 16, L"-size");
	gpLogSize = new MFileLog(pszName, pszDir, GetCurrentProcessId());

	if (!gpLogSize)
		return;

	dwErr = (DWORD)gpLogSize->CreateLogFile();
	if (dwErr != 0)
	{
		_printf("Create console log file failed! ErrCode=0x%08X\n", dwErr, gpLogSize->GetLogFileName()); //-V576
		_ASSERTE(FALSE && "gpLogSize->CreateLogFile failed");
		SafeDelete(gpLogSize);
		return;
	}

	gpLogSize->LogStartEnv(gpStartEnv);

	LogSize(NULL, 0, "Startup");
}

void LogString(LPCSTR asText)
{
	if (!gpLogSize) return;

	char szInfo[255]; szInfo[0] = 0;
	LPCSTR pszThread = " <unknown thread>";
	DWORD dwId = GetCurrentThreadId();

	if (dwId == gdwMainThreadId)
		pszThread = "MainThread";
	else if (gpSrv->CmdServer.IsPipeThread(dwId))
		pszThread = "ServThread";
	else if (dwId == gpSrv->dwRefreshThread)
		pszThread = "RefrThread";
	//#ifdef USE_WINEVENT_SRV
	//else if (dwId == gpSrv->dwWinEventThread)
	//	pszThread = " WinEventThread";
	//#endif
	else if (gpSrv->InputServer.IsPipeThread(dwId))
		pszThread = "InptThread";
	else if (gpSrv->DataServer.IsPipeThread(dwId))
		pszThread = "DataThread";

	gpLogSize->LogString(asText, true, pszThread);
}

void LogString(LPCWSTR asText)
{
	if (!gpLogSize) return;

	wchar_t szInfo[255]; szInfo[0] = 0;
	LPCWSTR pszThread = L" <unknown thread>";
	DWORD dwId = GetCurrentThreadId();

	if (dwId == gdwMainThreadId)
		pszThread = L"MainThread";
	else if (gpSrv->CmdServer.IsPipeThread(dwId))
		pszThread = L"ServThread";
	else if (dwId == gpSrv->dwRefreshThread)
		pszThread = L"RefrThread";
	else if (gpSrv->InputServer.IsPipeThread(dwId))
		pszThread = L"InptThread";
	else if (gpSrv->DataServer.IsPipeThread(dwId))
		pszThread = L"DataThread";

	gpLogSize->LogString(asText, true, pszThread);
}

void LogSize(const COORD* pcrSize, int newBufferHeight, LPCSTR pszLabel, bool bForceWriteLog)
{
	if (!gpLogSize) return;

	static DWORD nLastWriteTick = 0;
	const DWORD TickDelta = 1000;
	static LONG nSkipped = 0;
	const LONG SkipDelta = 50;
	static CONSOLE_SCREEN_BUFFER_INFO lsbiLast = {};
	bool bWriteLog = false;
	CONSOLE_SCREEN_BUFFER_INFO lsbi = {};

	HANDLE hCon = ghConOut;
	BOOL bHandleOK = (hCon != NULL);
	if (!bHandleOK)
		hCon = GetStdHandle(STD_OUTPUT_HANDLE);

	// В дебажный лог помещаем реальные значения
	BOOL bConsoleOK = GetConsoleScreenBufferInfo(hCon, &lsbi);
	char szInfo[300]; szInfo[0] = 0;
	DWORD nErrCode = GetLastError();

	int fontX = 0, fontY = 0; wchar_t szFontName[LF_FACESIZE] = L""; char szFontInfo[60] = "<NA>";
	if (apiGetConsoleFontSize(hCon, fontY, fontX, szFontName))
	{
		LPCSTR szState = "";
		if (!g_LastSetConsoleFont.cbSize)
		{
			szState = " <?>";
		}
		else if ((g_LastSetConsoleFont.dwFontSize.Y != fontY)
			|| (g_LastSetConsoleFont.dwFontSize.X != fontX))
		{
			// nConFontHeight/nConFontWidth не совпадает с получаемым,
			// нужно организовать спец переменные в WConsole
			_ASSERTE(g_LastSetConsoleFont.dwFontSize.Y==fontY && g_LastSetConsoleFont.dwFontSize.X==fontX);
			szState = " <!>";
		}

		szFontInfo[0] = '`';
		WideCharToMultiByte(CP_UTF8, 0, szFontName, -1, szFontInfo+1, 40, NULL, NULL);
		int iLen = lstrlenA(szFontInfo); // result of WideCharToMultiByte is not suitable (contains trailing zero)
		_wsprintfA(szFontInfo+iLen, SKIPLEN(countof(szFontInfo)-iLen) "` %ix%i%s",
			fontY, fontX, szState);
	}

	#ifdef _DEBUG
	wchar_t szClass[100] = L"";
	GetClassName(ghConWnd, szClass, countof(szClass));
	_ASSERTE(lstrcmp(szClass, L"ConsoleWindowClass")==0);
	#endif

	char szWindowInfo[40] = "<NA>"; RECT rcConsole = {};
	if (GetWindowRect(ghConWnd, &rcConsole))
	{
		_wsprintfA(szWindowInfo, SKIPCOUNT(szWindowInfo) "{(%i,%i) (%ix%i)}", rcConsole.left, rcConsole.top, LOGRECTSIZE(rcConsole));
	}

	if (pcrSize)
	{
		bWriteLog = true;

		_wsprintfA(szInfo, SKIPCOUNT(szInfo) "CurSize={%ix%i} ChangeTo={%ix%ix%i} %s (skipped=%i) {%u:%u:x%X:%u} %s %s",
		           lsbi.dwSize.X, lsbi.dwSize.Y, pcrSize->X, pcrSize->Y, newBufferHeight, (pszLabel ? pszLabel : ""), nSkipped,
		           bConsoleOK, bHandleOK, (DWORD)(DWORD_PTR)hCon, nErrCode,
				   szWindowInfo, szFontInfo);
	}
	else
	{
		if (bForceWriteLog)
			bWriteLog = true;
		// Avoid numerous writing of equal lines to log
		else if ((lsbi.dwSize.X != lsbiLast.dwSize.X) || (lsbi.dwSize.Y != lsbiLast.dwSize.Y))
			bWriteLog = true;
		else if (((GetTickCount() - nLastWriteTick) >= TickDelta) || (nSkipped >= SkipDelta))
			bWriteLog = true;

		_wsprintfA(szInfo, SKIPLEN(countof(szInfo)) "CurSize={%ix%i} %s (skipped=%i) {%u:%u:x%X:%u} %s %s",
		           lsbi.dwSize.X, lsbi.dwSize.Y, (pszLabel ? pszLabel : ""), nSkipped,
		           bConsoleOK, bHandleOK, (DWORD)(DWORD_PTR)hCon, nErrCode,
				   szWindowInfo, szFontInfo);
	}

	lsbiLast = lsbi;

	if (!bWriteLog)
	{
		InterlockedIncrement(&nSkipped);
	}
	else
	{
		nSkipped = 0;
		LogFunction(szInfo);
		nLastWriteTick = GetTickCount();
	}
}

int CLogFunction::m_FnLevel = 0; // Simple, without per-thread devision
CLogFunction::CLogFunction() : mb_Logged(false)
{
}
CLogFunction::CLogFunction(const char* asFnName) : mb_Logged(false)
{
	int nLen = MultiByteToWideChar(CP_ACP, 0, asFnName, -1, NULL, 0);
	wchar_t sBuf[80] = L"";
	wchar_t *pszBuf = NULL;
	if (nLen >= 80)
		pszBuf = (wchar_t*)calloc(nLen+1,sizeof(*pszBuf));
	else
		pszBuf = sBuf;

	MultiByteToWideChar(CP_ACP, 0, asFnName, -1, pszBuf, nLen+1);

	DoLogFunction(pszBuf);

	if (pszBuf != sBuf)
		SafeFree(pszBuf);
}
CLogFunction::CLogFunction(const wchar_t* asFnName) : mb_Logged(false)
{
	DoLogFunction(asFnName);
}
void CLogFunction::DoLogFunction(const wchar_t* asFnName)
{
	if (mb_Logged)
		return;

	LONG lLevel = InterlockedIncrement((LONG*)&m_FnLevel);
	mb_Logged = true;

	if (!gpLogSize) return;

	if (lLevel > 20) lLevel = 20;
	wchar_t cFnInfo[120];
	wchar_t* pc = cFnInfo;
	for (LONG l = 1; l < lLevel; l++)
	{
		*(pc++) = L' '; *(pc++) = L' '; *(pc++) = L' ';
	}
	*pc = 0;

	INT_PTR nPrefix = (pc - cFnInfo);
	INT_PTR nFnLen = lstrlen(asFnName);

	if (nFnLen < ((INT_PTR)countof(cFnInfo) - nPrefix))
	{
		lstrcpyn(pc, asFnName, countof(cFnInfo) - (pc - cFnInfo));
		LogString(cFnInfo);
	}
	else
	{
		wchar_t* pszMrg = lstrmerge(cFnInfo, asFnName);
		LogString(pszMrg);
		SafeFree(pszMrg);
	}
}
CLogFunction::~CLogFunction()
{
	if (!mb_Logged)
		return;
	InterlockedDecrement((LONG*)&m_FnLevel);
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

	bool bForceWriteLog = false;
	char szCmdName[40];
	switch (in.hdr.nCmd)
	{
	case CECMD_SETSIZESYNC:
		lstrcpynA(szCmdName, ":CECMD_SETSIZESYNC", countof(szCmdName)); break;
	case CECMD_SETSIZENOSYNC:
		lstrcpynA(szCmdName, ":CECMD_SETSIZENOSYNC", countof(szCmdName)); break;
	case CECMD_CMDSTARTED:
		lstrcpynA(szCmdName, ":CECMD_CMDSTARTED", countof(szCmdName)); break;
	case CECMD_CMDFINISHED:
		lstrcpynA(szCmdName, ":CECMD_CMDFINISHED", countof(szCmdName)); break;
	case CECMD_UNLOCKSTATION:
		lstrcpynA(szCmdName, ":CECMD_UNLOCKSTATION", countof(szCmdName)); bForceWriteLog = true; break;
	default:
		_ASSERTE(FALSE && "Unnamed command");
		_wsprintfA(szCmdName, SKIPCOUNT(szCmdName) ":UnnamedCmd(%u)", in.hdr.nCmd);
	}

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
		//SHORT  nNewTopVisible = -1;
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

		crNewSize = in.SetSize.size;

		//// rect по идее вообще не нужен, за блокировку при прокрутке отвечает nSendTopLine
		//rNewRect = in.SetSize.rcWindow;

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

		#if 0
		// Блокировка при прокрутке, значение используется только "виртуально" в CorrectVisibleRect
		gpSrv->TopLeft = in.SetSize.TopLeft;
		#endif

		nTick1 = GetTickCount();
		csRead.Unlock();
		WARNING("Если указан dwFarPID - это что-ли два раза подряд выполнится?");
		// rNewRect по идее вообще не нужен, за блокировку при прокрутке отвечает nSendTopLine
		nWasSetSize = SetConsoleSize(nBufferHeight, crNewSize, rNewRect, szCmdName, bForceWriteLog);
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

	DWORD lnNextPacketId = (DWORD)InterlockedIncrement(&gpSrv->nLastPacketID);
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

// Запрос к серверу - "Подцепись в ConEmu GUI".
// Может прийти из
// * GUI (диалог аттача)
// * из Far плагина (меню или команда "Attach to ConEmu")
// * из "ConEmuC /ATTACH"
BOOL cmd_Attach2Gui(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;

	// If command comes from GUI - update GUI HWND
	// Else - gpSrv->hGuiWnd will be set to NULL and suitable HWND will be found in Attach2Gui
	gpSrv->hGuiWnd = FindConEmuByPID(in.hdr.nSrcPID);

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
			(*out)->dwData[0] = LODWORD(hDc); //-V205 // Дескриптор окна
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
		LogString(L"WM_CLOSE posted to console window, termination was requested");
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
			           in.Msg.bPost ? "PostMessage" : "SendMessage", LODWORD(hSendWnd), //-V205
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
			goto wrap;  // не удалось выделить память

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

struct AltServerStartStop
{
	bool AltServerChanged; // = false;
	bool ForceThawAltServer; // = false;
	bool bPrevFound;
	DWORD nAltServerWasStarted; // = 0
	DWORD nAltServerWasStopped; // = 0;
	DWORD nCurAltServerPID; // = gpSrv->dwAltServerPID;
	HANDLE hAltServerWasStarted; // = NULL;
	AltServerInfo info; // = {};
};

void OnAltServerChanged(int nStep, StartStopType nStarted, DWORD nAltServerPID, CESERVER_REQ_STARTSTOP* pStartStop, AltServerStartStop& AS)
{
	if (nStep == 1)
	{
		if (nStarted == sst_AltServerStart)
		{
			// Перевести нить монитора в режим ожидания завершения AltServer, инициализировать gpSrv->dwAltServerPID, gpSrv->hAltServer
			AS.nAltServerWasStarted = nAltServerPID;
			if (pStartStop)
				AS.hAltServerWasStarted = (HANDLE)(DWORD_PTR)pStartStop->hServerProcessHandle;
			AS.AltServerChanged = true;
		}
		else
		{
			AS.bPrevFound = gpSrv->AltServers.Get(nAltServerPID, &AS.info, true/*Remove*/);

			// Сначала проверяем, не текущий ли альт.сервер закрывается
			if (gpSrv->dwAltServerPID && (nAltServerPID == gpSrv->dwAltServerPID))
			{
				// Поскольку текущий сервер завершается - то сразу сбросим PID (его морозить уже не нужно)
				AS.nAltServerWasStopped = nAltServerPID;
				gpSrv->dwAltServerPID = 0;
				// Переключаемся на "старый" (если был)
				if (AS.bPrevFound && AS.info.nPrevPID)
				{
					// _ASSERTE могут приводить к ошибкам блокировки gpSrv->csProc в других потоках. Но ассертов быть не должно )
					_ASSERTE(AS.info.hPrev!=NULL);
					// Перевести нить монитора в обычный режим, закрыть gpSrv->hAltServer
					// Активировать альтернативный сервер (повторно), отпустить его нити чтения
					AS.AltServerChanged = true;
					AS.nAltServerWasStarted = AS.info.nPrevPID;
					AS.hAltServerWasStarted = AS.info.hPrev;
					AS.ForceThawAltServer = true;
				}
				else
				{
					// _ASSERTE могут приводить к ошибкам блокировки gpSrv->csProc в других потоках. Но ассертов быть не должно )
					_ASSERTE(AS.info.hPrev==NULL);
					AS.AltServerChanged = true;
				}
			}
			else
			{
				// _ASSERTE могут приводить к ошибкам блокировки gpSrv->csProc в других потоках. Но ассертов быть не должно )
				_ASSERTE(((nAltServerPID == gpSrv->dwAltServerPID) || !gpSrv->dwAltServerPID || ((nStarted != sst_AltServerStop) && (nAltServerPID != gpSrv->dwAltServerPID) && !AS.bPrevFound)) && "Expected active alt.server!");
			}
		}
	}
	else if (nStep == 2)
	{
		if (AS.AltServerChanged)
		{
			if (AS.nAltServerWasStarted)
			{
				AltServerWasStarted(AS.nAltServerWasStarted, AS.hAltServerWasStarted, AS.ForceThawAltServer);
			}
			else if (AS.nCurAltServerPID && (nAltServerPID == AS.nCurAltServerPID))
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
					if (pStartStop)
						pGuiIn->StartStop = *pStartStop;
					pGuiIn->StartStop.dwPID = AS.nAltServerWasStarted ? AS.nAltServerWasStarted : AS.nAltServerWasStopped;
					pGuiIn->StartStop.hServerProcessHandle = NULL; // для GUI смысла не имеет
					pGuiIn->StartStop.nStarted = AS.nAltServerWasStarted ? sst_AltServerStart : sst_AltServerStop;
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
	}
}

BOOL cmd_FarDetached(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;

	// После детача в фаре команда (например dir) схлопнется, чтобы
	// консоль неожиданно не закрылась...
	gbAutoDisableConfirmExit = FALSE;
	gbAlwaysConfirmExit = TRUE;

	MSectionLock CS; CS.Lock(gpSrv->csProc);
	UINT nPrevCount = gpSrv->nProcessCount;
	_ASSERTE(in.hdr.nSrcPID!=0);
	DWORD nPID = in.hdr.nSrcPID;
	DWORD nPrevAltServerPID = gpSrv->dwAltServerPID;

	BOOL lbChanged = ProcessRemove(in.hdr.nSrcPID, nPrevCount, &CS);

	MSectionLock CsAlt;
	CsAlt.Lock(gpSrv->csAltSrv, TRUE, 1000);

	AltServerStartStop AS = {};
	AS.nCurAltServerPID = gpSrv->dwAltServerPID;

	OnAltServerChanged(1, sst_AltServerStop, nPID, NULL, AS);

	// ***
	if (lbChanged)
		ProcessCountChanged(TRUE, nPrevCount, &CS);
	CS.Unlock();
	// ***

	// После Unlock-а, зовем функцию
	if (AS.AltServerChanged)
	{
		OnAltServerChanged(2, sst_AltServerStop, in.hdr.nSrcPID, NULL, AS);
	}

	// Обновить мэппинг
	UpdateConsoleMapHeader();

	CsAlt.Unlock();

	return lbRc;
}

BOOL cmd_OnActivation(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;

	if (gpSrv)
	{
		LogString(L"CECMD_ONACTIVATION received, state update pending");

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
			// Warning: If refresh thread is in an AltServerStop
			// transaction, ReloadFullConsoleInfo with (TRUE) will deadlock.
			ReloadFullConsoleInfo(FALSE);
			// Force refresh thread to cycle
			SetEvent(gpSrv->hRefreshEvent);
		}
	}

	return lbRc;
}

BOOL cmd_SetWindowPos(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;
	BOOL lbWndRc = FALSE, lbShowRc = FALSE;
	DWORD nErrCode[2] = {};

	if ((in.SetWndPos.hWnd == ghConWnd) && gpLogSize)
		LogSize(NULL, 0, ":SetWindowPos.before");

	lbWndRc = SetWindowPos(in.SetWndPos.hWnd, in.SetWndPos.hWndInsertAfter,
	             in.SetWndPos.X, in.SetWndPos.Y, in.SetWndPos.cx, in.SetWndPos.cy,
	             in.SetWndPos.uFlags);
	nErrCode[0] = lbWndRc ? 0 : GetLastError();

	if ((in.SetWndPos.uFlags & SWP_SHOWWINDOW) && !IsWindowVisible(in.SetWndPos.hWnd))
	{
		lbShowRc = apiShowWindowAsync(in.SetWndPos.hWnd, SW_SHOW);
		nErrCode[1] = lbShowRc ? 0 : GetLastError();
	}

	if ((in.SetWndPos.hWnd == ghConWnd) && gpLogSize)
		LogSize(NULL, 0, ":SetWindowPos.after");

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
	SetConEmuWindows(NULL, NULL, NULL);
	gnConEmuPID = 0;
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

	if (hGuiApp != NULL)
	{
		DisableAutoConfirmExit(FALSE);

		SafeCloseHandle(gpSrv->hRootProcess);
		SafeCloseHandle(gpSrv->hRootThread);

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
	DEBUGSTARTSTOPBOX(szDbg);
#endif

	// _ASSERTE могут приводить к ошибкам блокировки gpSrv->csProc в других потоках. Но ассертов быть не должно )
	_ASSERTE(in.StartStop.dwPID!=0);


	// Переменные, чтобы позвать функцию AltServerWasStarted после отпускания CS.Unlock()
	/*
	bool AltServerChanged = false;
	DWORD nAltServerWasStarted = 0, nAltServerWasStopped = 0;
	DWORD nCurAltServerPID = gpSrv->dwAltServerPID;
	HANDLE hAltServerWasStarted = NULL;
	bool ForceThawAltServer = false;
	AltServerInfo info = {};
	*/
	AltServerStartStop AS = {};
	AS.nCurAltServerPID = gpSrv->dwAltServerPID;


	if (in.StartStop.nStarted == sst_AltServerStart)
	{
		// Перевести нить монитора в режим ожидания завершения AltServer, инициализировать gpSrv->dwAltServerPID, gpSrv->hAltServer
		// _ASSERTE могут приводить к ошибкам блокировки gpSrv->csProc в других потоках. Но ассертов быть не должно )
		_ASSERTE(in.StartStop.hServerProcessHandle!=0);

		OnAltServerChanged(1, sst_AltServerStart, in.StartStop.dwPID, &in.StartStop, AS);

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
			OnAltServerChanged(1, in.StartStop.nStarted, nAltPID, NULL, AS);

			// _ASSERTE могут приводить к ошибкам блокировки gpSrv->csProc в других потоках. Но ассертов быть не должно )
			_ASSERTE(in.StartStop.nStarted==sst_ComspecStop || in.StartStop.nOtherPID==AS.info.nPrevPID);
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
	if (AS.AltServerChanged)
	{
		OnAltServerChanged(2, in.StartStop.nStarted, in.StartStop.dwPID, &in.StartStop, AS);
	}

	// Обновить мэппинг
	UpdateConsoleMapHeader();

	CsAlt.Unlock();

	int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_STARTSTOPRET);
	*out = ExecuteNewCmd(CECMD_CMDSTARTSTOP,nOutSize);

	if (*out != NULL)
	{
		(*out)->StartStopRet.bWasBufferHeight = (gnBufferHeight != 0);
		(*out)->StartStopRet.hWnd = ghConEmuWnd;
		(*out)->StartStopRet.hWndDc = ghConEmuWndDC;
		(*out)->StartStopRet.hWndBack = ghConEmuWndBack;
		(*out)->StartStopRet.dwPID = gnConEmuPID;
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

	gpSrv->nActiveFarPID = in.hdr.nSrcPID;

	// Will update mapping using MainServer if this is Alternative
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

bool TerminateOneProcess(DWORD nPID, DWORD& nErrCode)
{
	bool lbRc = false;
	bool bNeedClose = false;
	HANDLE hProcess = NULL;

	if (gpSrv && gpSrv->pConsole)
	{
		if (nPID == gpSrv->dwRootProcess)
		{
			hProcess = gpSrv->hRootProcess;
			bNeedClose = FALSE;
		}
	}

	if (!hProcess)
	{
		hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, nPID);
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
			lbRc = true;
		}
		else
		{
			nErrCode = GetLastError();
			if (!bNeedClose)
			{
				hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, nPID);
				if (hProcess != NULL)
				{
					bNeedClose = true;
					if (TerminateProcess(hProcess, 100))
						lbRc = true;
				}
			}
		}
	}

	if (hProcess && bNeedClose)
		CloseHandle(hProcess);

	return lbRc;
}

bool TerminateProcessGroup(int nCount, LPDWORD pPID, DWORD& nErrCode)
{
	bool lbRc = false;
	HANDLE hJob, hProcess;

	if ((hJob = CreateJobObject(NULL, NULL)) == NULL)
	{
		// Job failed? Do it one-by-one
		for (int i = nCount-1; i >= 0; i--)
		{
			lbRc = TerminateOneProcess(pPID[i], nErrCode);
		}
	}
	else
	{
		MArray<HANDLE> hh;

		for (int i = 0; i < nCount; i++)
		{
			if (pPID[i] == gpSrv->dwRootProcess)
				continue;

			hProcess = OpenProcess(PROCESS_TERMINATE|PROCESS_SET_QUOTA, FALSE, pPID[i]);
			if (!hProcess) continue;

			if (AssignProcessToJobObject(hJob, hProcess))
			{
				hh.push_back(hProcess);
			}
			else
			{
				// Strange, can't assign to jub? Do it manually
				if (TerminateProcess(hProcess, 100))
					lbRc = true;
				CloseHandle(hProcess);
			}
		}

		if (!hh.empty())
		{
			lbRc = (TerminateJobObject(hJob, 100) != FALSE);

			while (hh.pop_back(hProcess))
			{
				if (!lbRc) // If job failed - last try
					TerminateProcess(hProcess, 100);
				CloseHandle(hProcess);
			}
		}

		CloseHandle(hJob);
	}

	return lbRc;
}

// CECMD_TERMINATEPID
BOOL cmd_TerminatePid(CESERVER_REQ& in, CESERVER_REQ** out)
{
	// Прибить процесс (TerminateProcess)
	size_t cbInSize = in.DataSize();
	if ((cbInSize < 2*sizeof(DWORD)) || (cbInSize < (sizeof(DWORD)*(1+in.dwData[0]))))
		return FALSE;

	BOOL lbRc = FALSE;
	HANDLE hProcess = NULL;
	BOOL bNeedClose = FALSE;
	DWORD nErrCode = 0;
	DWORD nCount = in.dwData[0];
	LPDWORD pPID = in.dwData+1;

	if (nCount == 1)
		lbRc = TerminateOneProcess(pPID[0], nErrCode);
	else
		lbRc = TerminateProcessGroup(nCount, pPID, nErrCode);


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

	return lbRc;
}

// CECMD_AFFNTYPRIORITY
BOOL cmd_AffinityPriority(CESERVER_REQ& in, CESERVER_REQ** out)
{
	size_t cbInSize = in.DataSize();
	if (cbInSize < 2*sizeof(u64))
		return FALSE;

	BOOL lbRc = FALSE;
	HANDLE hProcess = NULL;
	BOOL bNeedClose = FALSE;
	DWORD nErrCode = 0;

	DWORD_PTR nAffinity = (DWORD_PTR)in.qwData[0];
	DWORD nPriority = (DWORD)in.qwData[1];

	MSectionLock CS; CS.Lock(gpSrv->csProc);
	CheckProcessCount(TRUE);
	for (UINT i = 0; i < gpSrv->nProcessCount; i++)
	{
		DWORD nPID = gpSrv->pnProcesses[i];
		if (nPID == gnSelfPID) continue;

		HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, nPID);
		if (hProcess)
		{
			if (SetProcessAffinityMask(hProcess, nAffinity)
					&& SetPriorityClass(hProcess, nPriority))
				lbRc = TRUE;
			else
				nErrCode = GetLastError();

			CloseHandle(hProcess);
		}
		else
		{
			nErrCode = GetLastError();
		}
	}
	CS.Unlock();

	int nOutSize = sizeof(CESERVER_REQ_HDR) + 2*sizeof(DWORD);
	*out = ExecuteNewCmd(CECMD_AFFNTYPRIORITY,nOutSize);

	if (*out != NULL)
	{
		(*out)->dwData[0] = lbRc;
		(*out)->dwData[1] = nErrCode;
	}

	return lbRc;
}

// CECMD_PAUSE
BOOL cmd_Pause(CESERVER_REQ& in, CESERVER_REQ** out)
{
	size_t cbInSize = in.DataSize();
	if ((cbInSize < sizeof(DWORD)) || !gpSrv || !gpSrv->pConsole)
		return FALSE;

	BOOL bOk = FALSE; DWORD_PTR dwRc = 0;
	DWORD nTimeout = 15000;
	CONSOLE_SELECTION_INFO lsel1 = {}, lsel2 = {};
	BOOL bSelInfo1, bSelInfo2;

	bSelInfo1 = GetConsoleSelectionInfo(&lsel1);

	CEPauseCmd cmd = (CEPauseCmd)in.dwData[0], newState;
	if (cmd == CEPause_Switch)
		cmd = (bSelInfo1 && (lsel1.dwFlags & CONSOLE_SELECTION_IN_PROGRESS)) ? CEPause_Off : CEPause_On;

	switch (cmd)
	{
	case CEPause_On:
		SetConEmuFlags(gpSrv->pConsole->info.Flags, CECI_Paused, CECI_Paused);
		bOk = (SendMessageTimeout(ghConWnd, WM_SYSCOMMAND, 65522, 0, SMTO_NORMAL, nTimeout, &dwRc) != 0);
		break;
	case CEPause_Off:
		SetConEmuFlags(gpSrv->pConsole->info.Flags, CECI_Paused, CECI_None);
		bOk = (SendMessageTimeout(ghConWnd, WM_KEYDOWN, VK_ESCAPE, 0, SMTO_NORMAL, nTimeout, &dwRc) != 0);
		break;
	}

	// May be console server will not react immediately?
	DWORD nStartTick = GetTickCount();
	while (true)
	{
		bSelInfo2 = GetConsoleSelectionInfo(&lsel2);
		if (!bSelInfo2 || (lsel2.dwFlags != lsel1.dwFlags))
			break;
		Sleep(10);
		if ((GetTickCount() - nStartTick) >= 500)
			break;
	}
	// Return new state
	newState = (bSelInfo2 && (lsel2.dwFlags & CONSOLE_SELECTION_IN_PROGRESS)) ? CEPause_On : CEPause_Off;
	_ASSERTE(newState == cmd);

	int nOutSize = sizeof(CESERVER_REQ_HDR) + 2*sizeof(DWORD);
	*out = ExecuteNewCmd(CECMD_PAUSE,nOutSize);

	if (*out != NULL)
	{
		(*out)->dwData[0] = newState;
		(*out)->dwData[1] = bOk;
	}

	return TRUE;
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
		_ASSERTEX((in.AttachGuiApp.nPID == gpSrv->dwRootProcess || in.AttachGuiApp.nPID == gpSrv->Portable.nPID) && gpSrv->dwRootProcess && in.AttachGuiApp.nPID);

		wchar_t szInfo[MAX_PATH*2];

		// Вызывается два раза. Первый (при запуске exe) ahGuiWnd==NULL, второй - после фактического создания окна
		if (gbAttachMode || (gpSrv->hRootProcessGui == NULL))
		{
			_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"GUI application (PID=%u) was attached to ConEmu:\n%s\n",
				in.AttachGuiApp.nPID, in.AttachGuiApp.sAppFilePathName);
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

// CECMD_REDRAWHWND
BOOL cmd_RedrawHWND(CESERVER_REQ& in, CESERVER_REQ** out)
{
	size_t cbInSize = in.DataSize();
	if (cbInSize < sizeof(DWORD))
		return FALSE;

	BOOL bRedraw = FALSE;
	HWND hWnd = (HWND)in.dwData[0];

	// We need to invalidate client and non-client areas, following lines does the trick
	RECT rcClient = {};
	if (GetWindowRect(hWnd, &rcClient))
	{
		MapWindowPoints(NULL, hWnd, (LPPOINT)&rcClient, 2);
		bRedraw = RedrawWindow(hWnd, &rcClient, NULL, RDW_ALLCHILDREN|RDW_INVALIDATE|RDW_FRAME);
	}

	int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD);
	*out = ExecuteNewCmd(CECMD_REDRAWHWND,nOutSize);

	if (*out != NULL)
	{
		(*out)->dwData[0] = bRedraw;
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
				// OK, GUI will be informed by RM_SERVER itself
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
				// Обновим только курсор, а то юзер может получить черный экран, вместо ожидаемого текста
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

	size_t cbReplySize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_FULLSCREEN);
	*out = ExecuteNewCmd(CECMD_SETFULLSCREEN, cbReplySize);

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
			else
				gpSrv->pfnWasFullscreenMode = pfnGetConsoleDisplayMode;
		}
		lbRc = TRUE;
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
	*out = ExecuteNewCmd(CECMD_SETCONCOLORS, cbReplySize);

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
							apiShowWindow(ghConWnd, SW_HIDE);
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

		/*
					// If some cells was marked as "RowIds" - we need to change them manually
					SHORT nFromRow = csbi5.dwCursorPosition.Y, nRow = 0;
					CEConsoleMark Mark;
					while ((nFromRow >= 0) && FindConsoleRowId(ghConOut, nFromRow, &nRow, &Mark))
					{
						nFromRow = nRow - 1;
						// Check attributes
					}
		*/

		if (bOk && bCsbi5 && bTextChanged && in.SetConColor.ReFillConsole && csbi5.dwSize.X && csbi5.dwSize.Y)
		{
			// Считать из консоли текущие атрибуты (построчно/поблочно)
			// И там, где они совпадают с OldText - заменить на in.SetConColor.NewTextAttributes
			RefillConsoleAttributes(csbi5, OldText, in.SetConColor.NewTextAttributes);
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
		(*out)->dwData[0] = SetTitle((LPCWSTR)in.wData);
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

				//pSizeIn->SetSize.rcWindow.Left = pSizeIn->SetSize.rcWindow.Top = 0;
				//pSizeIn->SetSize.rcWindow.Right = lsbi.srWindow.Right - lsbi.srWindow.Left;
				//pSizeIn->SetSize.rcWindow.Bottom = lsbi.srWindow.Bottom - lsbi.srWindow.Top;
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

BOOL cmd_UpdConMapHdr(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = TRUE, lbAccepted = FALSE;

	size_t cbInSize = in.DataSize(), cbReqSize = sizeof(CESERVER_CONSOLE_MAPPING_HDR);
	if (cbInSize == cbReqSize)
	{
		if (in.ConInfo.cbSize == sizeof(CESERVER_CONSOLE_MAPPING_HDR)
			&& in.ConInfo.nProtocolVersion == CESERVER_REQ_VER
			&& in.ConInfo.nServerPID == GetCurrentProcessId())
		{
			//SetVisibleSize(in.ConInfo.crLockedVisible.X, in.ConInfo.crLockedVisible.Y); // ??

			if (gpSrv->pConsoleMap)
			{
				FixConsoleMappingHdr(&in.ConInfo);
				gpSrv->pConsoleMap->SetFrom(&in.ConInfo);
			}

			lbAccepted = TRUE;
		}
	}

	size_t cbReplySize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD);
	*out = ExecuteNewCmd(CECMD_UPDCONMAPHDR, cbReplySize);
	if ((*out) != NULL)
	{
		(*out)->dwData[0] = lbAccepted;
	}
	else
	{
		lbRc = FALSE;
	}

	return lbRc;
}

BOOL cmd_SetConScrBuf(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = TRUE;
	DWORD nSafeWait = (DWORD)-1;
	HANDLE hInEvent = NULL, hOutEvent = NULL, hWaitEvent = NULL;
	char szLog[200];

	size_t cbInSize = in.DataSize(), cbReqSize = sizeof(CESERVER_REQ_SETCONSCRBUF);
	if (cbInSize == cbReqSize)
	{
		char* pszLogLbl;
		if (gpLogSize)
		{
			msprintf(szLog, countof(szLog), "CECMD_SETCONSCRBUF x%08X {%i,%i} ",
				(DWORD)(DWORD_PTR)in.SetConScrBuf.hRequestor, (int)in.SetConScrBuf.dwSize.X, (int)in.SetConScrBuf.dwSize.Y);
			pszLogLbl = szLog + lstrlenA(szLog);
		}

		if (in.SetConScrBuf.bLock)
		{
			if (gpLogSize)
			{
				lstrcpynA(pszLogLbl, "Recvd", 10);
				LogString(szLog);
			}
			// Блокируем нить чтения и дождемся пока она перейдет в режим ожидания
			#ifdef _DEBUG
			if (gpSrv->hInWaitForSetConBufThread)
			{
				_ASSERTE(gpSrv->hInWaitForSetConBufThread==NULL);
			}
			#endif
			gpSrv->hInWaitForSetConBufThread = hInEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			gpSrv->hOutWaitForSetConBufThread = hOutEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			gpSrv->hWaitForSetConBufThread = hWaitEvent = (in.SetConScrBuf.hRequestor ? in.SetConScrBuf.hRequestor : INVALID_HANDLE_VALUE);
			nSafeWait = WaitForSingleObject(hInEvent, WAIT_SETCONSCRBUF_MIN_TIMEOUT);
			if (gpLogSize)
			{
				lstrcpynA(pszLogLbl, "Ready", 10);
				LogString(szLog);
			}
		}
		else
		{
			if (gpLogSize)
			{
				lstrcpynA(pszLogLbl, "Finish", 10);
				LogString(szLog);
			}
			hOutEvent = gpSrv->hOutWaitForSetConBufThread;
			// Otherwise - we must be in the call of ANOTHER thread!
			if (hOutEvent == in.SetConScrBuf.hTemp)
			{
				SetEvent(hOutEvent);
			}
			else
			{
				// Timeout may be, or another process we are waiting for already...
				_ASSERTE(hOutEvent == in.SetConScrBuf.hTemp);
			}
		}
	}

	size_t cbReplySize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_SETCONSCRBUF);
	*out = ExecuteNewCmd(CECMD_SETCONSCRBUF, cbReplySize);
	if ((*out) != NULL)
	{
		if (in.SetConScrBuf.bLock)
		{
			(*out)->SetConScrBuf.bLock = nSafeWait;
			(*out)->SetConScrBuf.hTemp = hOutEvent;
		}
	}
	else
	{
		lbRc = FALSE;
	}

	return lbRc;
}

BOOL cmd_PortableStarted(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = TRUE;

	if (in.DataSize() == sizeof(gpSrv->Portable))
	{
		gpSrv->Portable = in.PortableStarted;
		CheckProcessCount(TRUE);
		// For example, CommandPromptPortable.exe starts cmd.exe
		CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_PORTABLESTART, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_PORTABLESTARTED));
		if (pIn)
		{
			pIn->PortableStarted = in.PortableStarted;
			pIn->PortableStarted.hProcess = NULL; // hProcess is valid for our server only
			CESERVER_REQ* pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
			ExecuteFreeResult(pOut);
			ExecuteFreeResult(pIn);
		}
	}
	else
	{
		_ASSERTE(FALSE && "Invalid PortableStarted data");
	}

	size_t cbReplySize = sizeof(CESERVER_REQ_HDR);
	*out = ExecuteNewCmd(CECMD_PORTABLESTART, cbReplySize);
	lbRc = ((*out) != NULL);

	return lbRc;
}

BOOL cmd_CtrlBreakEvent(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = TRUE;
	BOOL lbGenRc = FALSE;

	if (in.DataSize() == (2 * sizeof(DWORD)))
	{
		lbGenRc = GenerateConsoleCtrlEvent(in.dwData[0], in.dwData[1]);
	}
	else
	{
		_ASSERTE(FALSE && "Invalid CtrlBreakEvent data");
	}

	size_t cbReplySize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD);
	*out = ExecuteNewCmd(CECMD_CTRLBREAK, cbReplySize);
	if (*out)
		(*out)->dwData[0] = lbGenRc;
	lbRc = ((*out) != NULL);

	return lbRc;
}

BOOL cmd_SetTopLeft(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = TRUE;
	BOOL lbGenRc = FALSE;

	if (in.DataSize() >= sizeof(in.ReqConInfo.TopLeft))
	{
		gpSrv->TopLeft = in.ReqConInfo.TopLeft;

		// May be we can scroll the console down?
		if (in.ReqConInfo.TopLeft.isLocked())
		{
			// We need real (uncorrected) position
			CONSOLE_SCREEN_BUFFER_INFO csbi = {};
			if (GetConsoleScreenBufferInfo(ghConOut, &csbi))
			{
				bool bChange = false;
				SMALL_RECT srNew = csbi.srWindow;
				int height = srNew.Bottom - srNew.Top + 1;

				// In some cases we can do physical scrolling
				if ((in.ReqConInfo.TopLeft.y >= 0)
					// cursor was visible?
					&& ((csbi.dwCursorPosition.Y >= srNew.Top)
						&& (csbi.dwCursorPosition.Y <= srNew.Bottom))
					// and cursor is remains visible?
					&& ((csbi.dwCursorPosition.Y >= in.ReqConInfo.TopLeft.y)
						&& (csbi.dwCursorPosition.Y < (in.ReqConInfo.TopLeft.y + height)))
					)
				{
					int shiftY = in.ReqConInfo.TopLeft.y - srNew.Top;
					srNew.Top = in.ReqConInfo.TopLeft.y;
					srNew.Bottom += shiftY;
					bChange = true;
				}

				if (bChange)
				{
					SetConsoleWindowInfo(ghConOut, TRUE, &srNew);
				}
			}
		}
	}
	else
	{
		_ASSERTE(FALSE && "Invalid SetTopLeft data");
	}

	size_t cbReplySize = sizeof(CESERVER_REQ_HDR);
	*out = ExecuteNewCmd(CECMD_SETTOPLEFT, cbReplySize);
	lbRc = ((*out) != NULL);

	return lbRc;
}

BOOL cmd_PromptStarted(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = TRUE;
	wchar_t szStarted[MAX_PATH+80];

	if (in.DataSize() >= sizeof(wchar_t))
	{
		int iLen = lstrlen(in.PromptStarted.szExeName);
		if (iLen > MAX_PATH) in.PromptStarted.szExeName[MAX_PATH] = 0;
		_wsprintf(szStarted, SKIPCOUNT(szStarted) L"Prompt (Hook server) was started, PID=%u {%s}", in.hdr.nSrcPID, in.PromptStarted.szExeName);
		LogFunction(szStarted);
	}

	*out = ExecuteNewCmd(CECMD_PROMPTSTARTED, sizeof(CESERVER_REQ_HDR));
	lbRc = ((*out) != NULL);

	return lbRc;
}

BOOL cmd_LockStation(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = TRUE;

	gpSrv->bStationLocked = TRUE;

	LogSize(NULL, 0, ":CECMD_LOCKSTATION");

	*out = ExecuteNewCmd(CECMD_LOCKSTATION, sizeof(CESERVER_REQ_HDR));
	lbRc = ((*out) != NULL);

	return lbRc;
}

BOOL cmd_UnlockStation(CESERVER_REQ& in, CESERVER_REQ** out, bool bProcessed)
{
	BOOL lbRc = TRUE;

	gpSrv->bStationLocked = FALSE;

	LogString("CECMD_UNLOCKSTATION");

	if (!bProcessed)
	{
		lbRc = cmd_SetSizeXXX_CmdStartedFinished(in, out);
	}
	else
	{
		*out = ExecuteNewCmd(CECMD_UNLOCKSTATION, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_RETSIZE));

		if (*out)
		{
			(*out)->SetSizeRet.crMaxSize = MyGetLargestConsoleWindowSize(ghConOut);
			MyGetConsoleScreenBufferInfo(ghConOut, &((*out)->SetSizeRet.SetSizeRet));
			DWORD lnNextPacketId = (DWORD)InterlockedIncrement(&gpSrv->nLastPacketID);
			(*out)->SetSizeRet.nNextPacketId = lnNextPacketId;
		}

		LogSize(NULL, 0, ":UnlockStation");

		lbRc = ((*out) != NULL);
	}

	return lbRc;
}

bool ProcessAltSrvCommand(CESERVER_REQ& in, CESERVER_REQ** out, BOOL& lbRc)
{
	bool lbProcessed = false;
	// Если крутится альтернативный сервер - команду нужно выполнять в нем
	if (gpSrv->dwAltServerPID && (gpSrv->dwAltServerPID != gnSelfPID))
	{
		HANDLE hSave = NULL, hDup = NULL; DWORD nDupError = (DWORD)-1;
		if ((in.hdr.nCmd == CECMD_SETCONSCRBUF) && (in.DataSize() >= sizeof(in.SetConScrBuf)) && in.SetConScrBuf.bLock)
		{
			if (gpSrv->hAltServer)
			{
				hSave = in.SetConScrBuf.hRequestor;
				if (!hSave)
					nDupError = (DWORD)-3;
				else if (!DuplicateHandle(GetCurrentProcess(), hSave, gpSrv->hAltServer, &hDup, 0, FALSE, DUPLICATE_SAME_ACCESS))
					nDupError = GetLastError();
				else
					in.SetConScrBuf.hRequestor = hDup;
			}
			else
			{
				nDupError = (DWORD)-2;
			}
		}

		(*out) = ExecuteSrvCmd(gpSrv->dwAltServerPID, &in, ghConWnd);
		lbProcessed = ((*out) != NULL);
		lbRc = lbProcessed;

		if (hSave)
		{
			in.SetConScrBuf.hRequestor = hSave;
			if (lbProcessed)
				SafeCloseHandle(hSave);
		}
		UNREFERENCED_PARAMETER(nDupError);
	}
	return lbProcessed;
}

BOOL ProcessSrvCommand(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc;
	MCHKHEAP;

	switch (in.hdr.nCmd)
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
		case CECMD_FARDETACHED:
		{
			// После детача в фаре команда (например dir) схлопнется, чтобы
			// консоль неожиданно не закрылась...
			lbRc = cmd_FarDetached(in, out);
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
		case CECMD_AFFNTYPRIORITY:
		{
			lbRc = cmd_AffinityPriority(in, out);
		} break;
		case CECMD_PAUSE:
		{
			lbRc = cmd_Pause(in, out);
		} break;
		case CECMD_ATTACHGUIAPP:
		{
			lbRc = cmd_GuiAppAttached(in, out);
		} break;
		case CECMD_REDRAWHWND:
		{
			lbRc = cmd_RedrawHWND(in, out);
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
		case CECMD_UPDCONMAPHDR:
		{
			lbRc = cmd_UpdConMapHdr(in, out);
		} break;
		case CECMD_SETCONSCRBUF:
		{
			// Если крутится альтернативный сервер - команду нужно выполнять в нем
			if (!ProcessAltSrvCommand(in, out, lbRc))
			{
				lbRc = cmd_SetConScrBuf(in, out);
			}
		} break;
		case CECMD_PORTABLESTART:
		{
			lbRc = cmd_PortableStarted(in, out);
		} break;
		case CECMD_CTRLBREAK:
		{
			lbRc = cmd_CtrlBreakEvent(in, out);
		} break;
		case CECMD_SETTOPLEFT:
		{
			lbRc = cmd_SetTopLeft(in, out);
		} break;
		case CECMD_PROMPTSTARTED:
		{
			lbRc = cmd_PromptStarted(in, out);
		} break;
		case CECMD_LOCKSTATION:
		{
			// Execute command in the alternative server too
			ProcessAltSrvCommand(in, out, lbRc);
			lbRc = cmd_LockStation(in, out);
		} break;
		case CECMD_UNLOCKSTATION:
		{
			// Execute command in the alternative server too
			bool lbProcessed = ProcessAltSrvCommand(in, out, lbRc);
			lbRc = cmd_UnlockStation(in, out, lbProcessed);
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






static void UndoConsoleWindowZoom()
{
	BOOL lbRc = FALSE;

	// Если юзер случайно нажал максимизацию, когда консольное окно видимо - ничего хорошего не будет
	if (IsZoomed(ghConWnd))
	{
		SendMessage(ghConWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
		DWORD dwStartTick = GetTickCount();

		do
		{
			Sleep(20); // подождем чуть, но не больше секунды
		}
		while (IsZoomed(ghConWnd) && (GetTickCount()-dwStartTick)<=1000);

		Sleep(20); // и еще чуть-чуть, чтобы консоль прочухалась
		// Теперь нужно вернуть (вдруг он изменился) размер буфера консоли
		// Если этого не сделать - размер консоли нельзя УМЕНЬШИТЬ
		RECT rcConPos;
		GetWindowRect(ghConWnd, &rcConPos);
		MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, 1, 1, 1);

		TODO("Horizontal scroll");
		if (gnBufferHeight == 0)
		{
			//specified width and height cannot be less than the width and height of the console screen buffer's window
			lbRc = hkFunc.setConsoleScreenBufferSize(ghConOut, gcrVisibleSize);
		}
		else
		{
			// ресайз для BufferHeight
			COORD crHeight = {gcrVisibleSize.X, gnBufferHeight};
			MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, 1, 1, 1);
			lbRc = hkFunc.setConsoleScreenBufferSize(ghConOut, crHeight); // а не crNewSize - там "оконные" размеры
		}

		// И вернуть тот видимый прямоугольник, который был получен в последний раз (успешный раз)
		lbRc = SetConsoleWindowInfo(ghConOut, TRUE, &gpSrv->sbi.srWindow);
	}

	UNREFERENCED_PARAMETER(lbRc);
}

void static CorrectDBCSCursorPosition(HANDLE ahConOut, CONSOLE_SCREEN_BUFFER_INFO& csbi)
{
	if (!gbIsDBCS || csbi.dwSize.X <= 0 || csbi.dwCursorPosition.X <= 0)
		return;

	// Issue 577: Chinese display error on Chinese
	// -- GetConsoleScreenBufferInfo возвращает координаты курсора в DBCS, а нам нужен wchar_t !!!
	DWORD nCP = GetConsoleOutputCP();
	if ((nCP != CP_UTF8) && (nCP != CP_UTF7))
	{
		UINT MaxCharSize = 0;
		if (AreCpInfoLeads(nCP, &MaxCharSize) && (MaxCharSize > 1))
		{
			_ASSERTE(MaxCharSize==2);
			WORD Attrs[200];
			LONG cchMax = countof(Attrs);
			WORD* pAttrs = (csbi.dwCursorPosition.X <= cchMax) ? Attrs : (WORD*)calloc(csbi.dwCursorPosition.X, sizeof(*pAttrs));
			if (pAttrs)
				cchMax = csbi.dwCursorPosition.X;
			else
				pAttrs = Attrs; // memory allocation fail? try part of line?
			COORD crRead = {0, csbi.dwCursorPosition.Y};
			//120830 - DBCS uses 2 cells per hieroglyph
			DWORD nRead = 0;
			if (ReadConsoleOutputAttribute(ahConOut, pAttrs, cchMax, crRead, &nRead) && nRead)
			{
				_ASSERTE(nRead==cchMax);
				int nXShift = 0;
				LPWORD p = pAttrs, pEnd = pAttrs+nRead;
				while (p < pEnd)
				{
					if ((*p) & COMMON_LVB_LEADING_BYTE)
					{
						nXShift++;
						p++;
						_ASSERTE((p < pEnd) && ((*p) & COMMON_LVB_TRAILING_BYTE));
					}
					p++;
				}
				_ASSERTE(nXShift <= csbi.dwCursorPosition.X);
				csbi.dwCursorPosition.X = max(0,(csbi.dwCursorPosition.X - nXShift));
			}
			if (pAttrs && (pAttrs != Attrs))
			{
				free(pAttrs);
			}
		}
	}
}

// Действует аналогично функции WinApi (GetConsoleScreenBufferInfo), но в режиме сервера:
// 1. запрещает (то есть отменяет) максимизацию консольного окна
// 2. корректирует srWindow: сбрасывает горизонтальную прокрутку,
//    а если не обнаружен "буферный режим" - то и вертикальную.
BOOL MyGetConsoleScreenBufferInfo(HANDLE ahConOut, PCONSOLE_SCREEN_BUFFER_INFO apsc)
{
	BOOL lbRc = FALSE, lbSetRc = TRUE;
	DWORD nErrCode = 0;

	// ComSpec окно менять НЕ ДОЛЖЕН!
	if ((gnRunMode == RM_SERVER || gnRunMode == RM_ALTSERVER)
		&& ghConEmuWnd && IsWindow(ghConEmuWnd))
	{
		// Если юзер случайно нажал максимизацию, когда консольное окно видимо - ничего хорошего не будет
		if (IsZoomed(ghConWnd))
		{
			UndoConsoleWindowZoom();
		}
	}

	SMALL_RECT srRealWindow = {};
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	lbRc = GetConsoleScreenBufferInfo(ahConOut, &csbi);
	if (!lbRc)
	{
		nErrCode = GetLastError();
		if (gpLogSize) LogSize(NULL, 0, ":MyGetConsoleScreenBufferInfo");
		_ASSERTE(FALSE && "GetConsoleScreenBufferInfo failed, conhost was destroyed?");
		goto wrap;
	}

	if ((csbi.dwSize.X <= 0)
		|| (csbi.dwSize.Y <= 0)
		|| (csbi.dwSize.Y > LONGOUTPUTHEIGHT_MAX)
		)
	{
		nErrCode = GetLastError();
		if (gpLogSize) LogSize(NULL, 0, ":MyGetConsoleScreenBufferInfo");
		_ASSERTE(FALSE && "GetConsoleScreenBufferInfo failed, conhost was destroyed?");
		goto wrap;
	}

	srRealWindow = csbi.srWindow;

	// Issue 373: wmic (Horizontal buffer)
	{
		TODO("Его надо и из ConEmu обновлять");
		if (csbi.dwSize.X && (gnBufferWidth != csbi.dwSize.X))
			gnBufferWidth = csbi.dwSize.X;
		else if (gnBufferWidth == (csbi.srWindow.Right - csbi.srWindow.Left + 1))
			gnBufferWidth = 0;
	}

	// CONSOLE_FULLSCREEN/*1*/ or CONSOLE_FULLSCREEN_HARDWARE/*2*/
	if (pfnGetConsoleDisplayMode && pfnGetConsoleDisplayMode(&gpSrv->dwDisplayMode))
	{
		// The bug of Windows 10 b9879
		if ((gnOsVer == 0x0604) && (gOSVer.dwBuildNumber == 9879))
			gpSrv->dwDisplayMode = 0;
		if (gpSrv->dwDisplayMode & CONSOLE_FULLSCREEN_HARDWARE)
		{
			// While in hardware fullscreen - srWindow still shows window region
			// as it can be, if returned in GUI mode (I suppose)
			//_ASSERTE(!csbi.srWindow.Left && !csbi.srWindow.Top);
			csbi.dwSize.X = csbi.srWindow.Right+1-csbi.srWindow.Left;
			csbi.dwSize.Y = csbi.srWindow.Bottom+1+csbi.srWindow.Top;
			// Log that change
			if (gpLogSize)
			{
				char szLabel[80];
				_wsprintfA(szLabel, SKIPCOUNT(szLabel) "CONSOLE_FULLSCREEN_HARDWARE{x%08X}", gpSrv->dwDisplayMode);
				LogSize(&csbi.dwSize, 0, szLabel);
			}
		}
	}

	//
	_ASSERTE((csbi.srWindow.Bottom-csbi.srWindow.Top)<200);

	// ComSpec окно менять НЕ ДОЛЖЕН!
	// ??? Приложениям запрещено менять размер видимой области.
	// ??? Размер буфера - могут менять, но не менее чем текущая видимая область
	if ((gnRunMode == RM_SERVER || gnRunMode == RM_ALTSERVER) && gpSrv)
	{
		// Если мы НЕ в ресайзе, проверить максимально допустимый размер консоли
		if (!gpSrv->nRequestChangeSize && (gpSrv->crReqSizeNewSize.X > 0) && (gpSrv->crReqSizeNewSize.Y > 0))
		{
			COORD crMax = MyGetLargestConsoleWindowSize(ghConOut);
			// Это может случиться, если пользователь резко уменьшил разрешение экрана
			// ??? Польза здесь сомнительная
			if (crMax.X > 0 && crMax.X < gpSrv->crReqSizeNewSize.X)
			{
				gpSrv->crReqSizeNewSize.X = crMax.X;
				TODO("Обновить gcrVisibleSize");
			}
			if (crMax.Y > 0 && crMax.Y < gpSrv->crReqSizeNewSize.Y)
			{
				gpSrv->crReqSizeNewSize.Y = crMax.Y;
				TODO("Обновить gcrVisibleSize");
			}
		}
	}

	// Issue 577: Chinese display error on Chinese
	// -- GetConsoleScreenBufferInfo возвращает координаты курсора в DBCS, а нам нужен wchar_t !!!
	if (gbIsDBCS && csbi.dwSize.X && csbi.dwCursorPosition.X)
	{
		CorrectDBCSCursorPosition(ahConOut, csbi);
	}

	// Блокировка видимой области, TopLeft задается из GUI
	if ((gnRunMode == RM_SERVER || gnRunMode == RM_ALTSERVER) && gpSrv)
	{
		// Коррекция srWindow
		if (gpSrv->TopLeft.isLocked())
		{
			SHORT nWidth = (csbi.srWindow.Right - csbi.srWindow.Left + 1);
			SHORT nHeight = (csbi.srWindow.Bottom - csbi.srWindow.Top + 1);

			if (gpSrv->TopLeft.x >= 0)
			{
				csbi.srWindow.Right = min(csbi.dwSize.X, gpSrv->TopLeft.x+nWidth) - 1;
				csbi.srWindow.Left = max(0, csbi.srWindow.Right-nWidth+1);
			}
			if (gpSrv->TopLeft.y >= 0)
			{
				csbi.srWindow.Bottom = min(csbi.dwSize.Y, gpSrv->TopLeft.y+nHeight) - 1;
				csbi.srWindow.Top = max(0, csbi.srWindow.Bottom-nHeight+1);
			}
		}

		// Eсли
		// a) в прошлый раз курсор БЫЛ видим, а сейчас нет, И
		// b) TopLeft не был изменен с тех пор (GUI не прокручивали)
		// Это значит, что в консоли, например, нажали Enter в prompt
		// или приложение продолжает вывод в консоль, в общем,
		// содержимое консоли прокручивается,
		// и то же самое нужно сделать в GUI
		if (gpSrv->pConsole && gpSrv->TopLeft.isLocked())
		{
			// Был видим в той области, которая ушла в GUI
			if (CoordInSmallRect(gpSrv->pConsole->info.sbi.dwCursorPosition, gpSrv->pConsole->info.sbi.srWindow)
				// и видим сейчас в RealConsole
				&& CoordInSmallRect(csbi.dwCursorPosition, srRealWindow)
				// но стал НЕ видим в скорректированном (по TopLeft) прямоугольнике
				&& !CoordInSmallRect(csbi.dwCursorPosition, csbi.srWindow)
				// И TopLeft в GUI не менялся
				&& gpSrv->TopLeft.Equal(gpSrv->pConsole->info.TopLeft))
			{
				// Сбросить блокировку и вернуть реальное положение в консоли
				gpSrv->TopLeft.Reset();
				csbi.srWindow = srRealWindow;
			}
		}

		if (gpSrv->pConsole)
		{
			gpSrv->pConsole->info.TopLeft = gpSrv->TopLeft;
			gpSrv->pConsole->info.srRealWindow = srRealWindow;
		}
	}

wrap:
	if (apsc)
		*apsc = csbi;
	// Возвращаем (возможно) скорректированные данные
	UNREFERENCED_PARAMETER(nErrCode);
	return lbRc;
}


static bool AdaptConsoleFontSize(const COORD& crNewSize)
{
	bool lbRc = true;
	char szLogInfo[128];

	// Minimum console size
	int curSizeY = -1, curSizeX = -1;
	wchar_t sFontName[LF_FACESIZE] = L"";
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
				if (gpLogSize)
				{
					_wsprintfA(szLogInfo, SKIPLEN(countof(szLogInfo)) "Need to reduce minSize. Cur={%i,%i}, Req={%i,%i}", minSizeX, minSizeY, crNewSize.X, crNewSize.Y);
					LogString(szLogInfo);
				}

				apiFixFontSizeForBufferSize(ghConOut, crNewSize, szLogInfo, countof(szLogInfo));
				LogString(szLogInfo);

				apiGetConsoleFontSize(ghConOut, curSizeY, curSizeX, sFontName);
			}
		}
		if (gpLogSize)
		{
			_wsprintfA(szLogInfo, SKIPLEN(countof(szLogInfo)) "Console font size H=%i W=%i N=", curSizeY, curSizeX);
			int nLen = lstrlenA(szLogInfo);
			WideCharToMultiByte(CP_UTF8, 0, sFontName, -1, szLogInfo+nLen, countof(szLogInfo)-nLen, NULL, NULL);
			LogFunction(szLogInfo);
		}
	}
	else
	{
		LogFunction(L"Function GetConsoleFontSize is not available");
	}


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
		if (bCanChangeFontSize)
		{
			BOOL bChangeRc = apiFixFontSizeForBufferSize(ghConOut, crNewSize, szLogInfo, countof(szLogInfo));
			LogString(szLogInfo);

			if (bChangeRc)
			{
				crMax = MyGetLargestConsoleWindowSize(ghConOut);

				if (gpLogSize)
				{
					_wsprintfA(szLogInfo, SKIPLEN(countof(szLogInfo)) "Largest console size is {%i,%i}", crMax.X, crMax.Y);
					LogString(szLogInfo);
				}
			}

			if (!bChangeRc
				|| (crMax.X && crNewSize.X > crMax.X)
				|| (crMax.Y && crNewSize.Y > crMax.Y))
			{
				lbRc = false;
				LogString("Change console size skipped: can't adapt font");
				goto wrap;
			}
		}
		else
		{
			LogString("Change console size skipped: too large");
			lbRc = false;
			goto wrap;
		}
	}

wrap:
	return lbRc;
}

// No buffer (scrolling) in the console
// По идее, это только для приложений которые сами меняют высоту буфера
// для работы в "полноэкранном" режиме, типа Far 1.7x или Far 2+ без ключа /w
static bool ApplyConsoleSizeSimple(const COORD& crNewSize, const CONSOLE_SCREEN_BUFFER_INFO& csbi, DWORD& dwErr, bool bForceWriteLog)
{
	bool lbRc = true;
	dwErr = 0;

	bool lbNeedChange = (csbi.dwSize.X != crNewSize.X) || (csbi.dwSize.Y != crNewSize.Y)
		|| ((csbi.srWindow.Right - csbi.srWindow.Left + 1) != crNewSize.X)
		|| ((csbi.srWindow.Bottom - csbi.srWindow.Top + 1) != crNewSize.Y);

	RECT rcConPos = {};
	GetWindowRect(ghConWnd, &rcConPos);

	SMALL_RECT rNewRect = {};

	#ifdef _DEBUG
	if (!lbNeedChange)
	{
		int nDbg = 0;
	}
	#endif

	if (lbNeedChange)
	{
		// Если этого не сделать - размер консоли нельзя УМЕНЬШИТЬ
		if (crNewSize.X <= (csbi.srWindow.Right-csbi.srWindow.Left) || crNewSize.Y <= (csbi.srWindow.Bottom-csbi.srWindow.Top))
		{
			rNewRect.Left = 0; rNewRect.Top = 0;
			rNewRect.Right = min((crNewSize.X - 1),(csbi.srWindow.Right-csbi.srWindow.Left));
			rNewRect.Bottom = min((crNewSize.Y - 1),(csbi.srWindow.Bottom-csbi.srWindow.Top));

			if (!SetConsoleWindowInfo(ghConOut, TRUE, &rNewRect))
			{
				// Last chance to shrink visible area of the console if ConApi was failed
				MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, 1, 1, 1);
			}
		}

		//specified width and height cannot be less than the width and height of the console screen buffer's window
		if (!hkFunc.setConsoleScreenBufferSize(ghConOut, crNewSize))
		{
			lbRc = false;
			dwErr = GetLastError();
		}

		//TODO: а если правый нижний край вылезет за пределы экрана?
		rNewRect.Left = 0; rNewRect.Top = 0;
		rNewRect.Right = crNewSize.X - 1;
		rNewRect.Bottom = crNewSize.Y - 1;
		if (!SetConsoleWindowInfo(ghConOut, TRUE, &rNewRect))
		{
			// Non-critical error?
			dwErr = GetLastError();
		}
	}

	LogSize(NULL, 0, lbRc ? "ApplyConsoleSizeSimple OK" : "ApplyConsoleSizeSimple FAIL", bForceWriteLog);

	return lbRc;
}

static SHORT FindFirstDirtyLine(SHORT anFrom, SHORT anTo, SHORT anWidth, WORD wDefAttrs)
{
	SHORT iFound = anFrom;
	SHORT iStep = (anTo < anFrom) ? -1 : 1;
	HANDLE hCon = ghConOut;
	BOOL bReadRc;
	CHAR_INFO* pch = (CHAR_INFO*)calloc(anWidth, sizeof(*pch));
	COORD crBufSize = {anWidth, 1}, crNil = {};
	SMALL_RECT rcRead = {0, anFrom, anWidth-1, anFrom};
	BYTE bDefAttr = LOBYTE(wDefAttrs); // Trim to colors only, do not compare extended attributes!

	for (rcRead.Top = anFrom; rcRead.Top != anTo; rcRead.Top += iStep)
	{
		rcRead.Bottom = rcRead.Top;

		InterlockedIncrement(&gnInReadConsoleOutput);
		bReadRc = ReadConsoleOutput(hCon, pch, crBufSize, crNil, &rcRead);
		InterlockedDecrement(&gnInReadConsoleOutput);
		if (!bReadRc)
			break;

		// Is line dirty?
		for (SHORT i = 0; i < anWidth; i++)
		{
			// Non-space char or non-default color/background
			if ((pch[i].Char.UnicodeChar != L' ') || (LOBYTE(pch[i].Attributes) != bDefAttr))
			{
				iFound = rcRead.Top;
				goto wrap;
			}
		}
	}

	iFound = min(anTo, anFrom);
wrap:
	SafeFree(pch);
	return iFound;
}

// По идее, rNewRect должен на входе содержать текущую видимую область
static void EvalVisibleResizeRect(SMALL_RECT& rNewRect,
	SHORT anOldBottom,
	const COORD& crNewSize,
	bool bCursorInScreen, SHORT nCursorAtBottom,
	SHORT nScreenAtBottom,
	const CONSOLE_SCREEN_BUFFER_INFO& csbi)
{
	// Абсолютная (буферная) координата
	const SHORT nMaxX = csbi.dwSize.X-1, nMaxY = csbi.dwSize.Y-1;

	// сначала - не трогая rNewRect.Left, вдруг там горизонтальная прокрутка?
	// anWidth - желаемая ширина видимой области
	rNewRect.Right = rNewRect.Left + crNewSize.X - 1;
	// не может выходить за пределы ширины буфера
	if (rNewRect.Right > nMaxX)
	{
		rNewRect.Left = max(0, csbi.dwSize.X-crNewSize.X);
		rNewRect.Right = min(nMaxX, rNewRect.Left+crNewSize.X-1);
	}

	// Теперь - танцы с вертикалью. Логика такая
	// * Если ДО ресайза все видимые строки были заполнены (кейбар фара внизу экрана) - оставить anOldBottom
	// * Иначе, если курсор был видим
	//   * приоритетно - двигать верхнюю границу видимой области (показывать максимум строк из back-scroll-buffer)
	//   * не допускать, чтобы расстояние между курсором и низом видимой области УМЕНЬШИЛОСЬ до менее чем 2-х строк
	// * Иначе если курсор был НЕ видим
	//   * просто показывать максимум стро из back-scroll-buffer (фиксирую нижнюю границу)

	// BTW, сейчас при ресайзе меняется только ширина csbi.dwSize.X (ну, кроме случаев изменения высоты буфера)

	if ((nScreenAtBottom <= 0) && (nCursorAtBottom <= 0))
	{
		// Все просто, фиксируем нижнюю границу по размеру буфера
		rNewRect.Bottom = csbi.dwSize.Y-1;
		rNewRect.Top = max(0, rNewRect.Bottom-crNewSize.Y+1);
	}
	else
	{
		// Значит консоль еще не дошла до низа
		SHORT nRectHeight = (rNewRect.Bottom - rNewRect.Top + 1);

		if (nCursorAtBottom > 0)
		{
			_ASSERTE(nCursorAtBottom<=3);
			// Оставить строку с курсором "приклеенной" к нижней границе окна (с макс. отступом nCursorAtBottom строк)
			rNewRect.Bottom = min(nMaxY, csbi.dwCursorPosition.Y+nCursorAtBottom-1);
		}
		// Уменьшение видимой области
		else if (crNewSize.Y < nRectHeight)
		{
			if ((nScreenAtBottom > 0) && (nScreenAtBottom <= 3))
			{
				// Оставить nScreenAtBottom строк (включая) между anOldBottom и низом консоли
				rNewRect.Bottom = min(nMaxY, anOldBottom+nScreenAtBottom-1);
			}
			else if (anOldBottom > (rNewRect.Top + crNewSize.Y - 1))
			{
				// Если нижняя граница приблизилась или перекрыла
				// нашу старую строку (которая была anOldBottom)
				rNewRect.Bottom = min(anOldBottom, csbi.dwSize.Y-1);
			}
			else
			{
				// Иначе - не трогать верхнюю границу
				rNewRect.Bottom = min(nMaxY, rNewRect.Top+crNewSize.Y-1);
			}
			//rNewRect.Top = rNewRect.Bottom-crNewSize.Y+1; // на 0 скорректируем в конце
		}
		// Увеличение видимой области
		else if (crNewSize.Y > nRectHeight)
		{
			if (nScreenAtBottom > 0)
			{
				// Оставить nScreenAtBottom строк (включая) между anOldBottom и низом консоли
				rNewRect.Bottom = min(nMaxY, anOldBottom+nScreenAtBottom-1);
			}
			//rNewRect.Top = rNewRect.Bottom-crNewSize.Y+1; // на 0 скорректируем в конце
		}

		// Но курсор не должен уходить за пределы экрана
		if (bCursorInScreen && (csbi.dwCursorPosition.Y < (rNewRect.Bottom-crNewSize.Y+1)))
		{
			rNewRect.Bottom = max(0, csbi.dwCursorPosition.Y+crNewSize.Y-1);
		}

		// And top, will be corrected to (>0) below
		rNewRect.Top = rNewRect.Bottom-crNewSize.Y+1;

		// Проверка на выход за пределы буфера
		if (rNewRect.Bottom > nMaxY)
		{
			rNewRect.Bottom = nMaxY;
			rNewRect.Top = max(0, rNewRect.Bottom-crNewSize.Y+1);
		}
		else if (rNewRect.Top < 0)
		{
			rNewRect.Top = 0;
			rNewRect.Bottom = min(nMaxY, rNewRect.Top+crNewSize.Y-1);
		}
	}

	_ASSERTE((rNewRect.Bottom-rNewRect.Top+1) == crNewSize.Y);
}

// There is the buffer (scrolling) in the console
// Ресайз для BufferHeight
static bool ApplyConsoleSizeBuffer(const USHORT BufferHeight, const COORD& crNewSize, const CONSOLE_SCREEN_BUFFER_INFO& csbi, DWORD& dwErr, bool bForceWriteLog)
{
	bool lbRc = true;
	dwErr = 0;

	RECT rcConPos = {};
	GetWindowRect(ghConWnd, &rcConPos);

	TODO("Horizontal scrolling?");
	COORD crHeight = {crNewSize.X, BufferHeight};
	SMALL_RECT rcTemp = {};

	// По идее (в планах), lbCursorInScreen всегда должен быть true,
	// если только само консольное приложение не выполняет прокрутку.
	// Сам ConEmu должен "крутить" консоль только виртуально, не трогая физический скролл.
	bool lbCursorInScreen = CoordInSmallRect(csbi.dwCursorPosition, csbi.srWindow);
	bool lbScreenAtBottom = (csbi.srWindow.Top > 0) && (csbi.srWindow.Bottom >= (csbi.dwSize.Y - 1));
	bool lbCursorAtBottom = (lbCursorInScreen && (csbi.dwCursorPosition.Y >= (csbi.srWindow.Bottom - 2)));
	SHORT nCursorAtBottom = lbCursorAtBottom ? (csbi.srWindow.Bottom - csbi.dwCursorPosition.Y + 1) : 0;
	SHORT nBottomLine = csbi.srWindow.Bottom;
	SHORT nScreenAtBottom = 0;

	// Прикинуть, где должна будет быть нижняя граница видимой области
	if (!lbScreenAtBottom)
	{
		// Ищем снизу вверх (найти самую нижнюю грязную строку)
		SHORT nTo = lbCursorInScreen ? csbi.dwCursorPosition.Y : csbi.srWindow.Top;
		SHORT nWidth = (csbi.srWindow.Right - csbi.srWindow.Left + 1);
		SHORT nDirtyLine = FindFirstDirtyLine(nBottomLine, nTo, nWidth, csbi.wAttributes);

		// Если удачно
		if (nDirtyLine >= csbi.srWindow.Top && nDirtyLine < csbi.dwSize.Y)
		{
			if (lbCursorInScreen)
			{
				nBottomLine = max(nDirtyLine, min(csbi.dwCursorPosition.Y+1/*-*/,csbi.srWindow.Bottom));
			}
			else
			{
				nBottomLine = nDirtyLine;
			}
		}
		nScreenAtBottom = (csbi.srWindow.Bottom - nBottomLine + 1);

		// Чтобы информации НАД курсором не стало меньше чем пустых строк ПОД курсором
		if (lbCursorInScreen)
		{
			if (nScreenAtBottom <= 4)
			{
				SHORT nAboveLines = (crNewSize.Y - nScreenAtBottom);
				if (nAboveLines <= (nScreenAtBottom + 1))
				{
					nCursorAtBottom = max(1, crNewSize.Y - nScreenAtBottom - 1);
				}
			}
		}
	}

	SMALL_RECT rNewRect = csbi.srWindow;
	EvalVisibleResizeRect(rNewRect, nBottomLine, crNewSize, lbCursorInScreen, nCursorAtBottom, nScreenAtBottom, csbi);

#if 0
	// Подправим будущую видимую область
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
#endif

	// Если этого не сделать - размер консоли нельзя УМЕНЬШИТЬ
	if (crNewSize.X <= (csbi.srWindow.Right-csbi.srWindow.Left)
	        || crNewSize.Y <= (csbi.srWindow.Bottom-csbi.srWindow.Top))
	{
		#if 0
		rcTemp.Left = 0;
		WARNING("А при уменьшении высоты, тащим нижнюю границе окна вверх, Top глючить не будет?");
		rcTemp.Top = max(0,(csbi.srWindow.Bottom-crNewSize.Y+1));
		rcTemp.Right = min((crNewSize.X - 1),(csbi.srWindow.Right-csbi.srWindow.Left));
		rcTemp.Bottom = min((BufferHeight - 1),(rcTemp.Top+crNewSize.Y-1));//(csbi.srWindow.Bottom-csbi.srWindow.Top)); //-V592
		_ASSERTE(((rcTemp.Bottom-rcTemp.Top+1)==crNewSize.Y) && ((rcTemp.Bottom-rcTemp.Top)==(rNewRect.Bottom-rNewRect.Top)));
		#endif

		if (!SetConsoleWindowInfo(ghConOut, TRUE, &rNewRect))
		{
			// Last chance to shrink visible area of the console if ConApi was failed
			MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, 1, 1, 1);
		}
	}

	// crHeight, а не crNewSize - там "оконные" размеры
	if (!hkFunc.setConsoleScreenBufferSize(ghConOut, crHeight))
	{
		lbRc = false;
		dwErr = GetLastError();
	}

	// Особенно в Win10 после "заворота строк",
	// нужно получить новое реальное состояние консоли после изменения буфера
	CONSOLE_SCREEN_BUFFER_INFO csbiNew = {};
	if (GetConsoleScreenBufferInfo(ghConOut, &csbiNew))
	{
		rNewRect = csbiNew.srWindow;
		EvalVisibleResizeRect(rNewRect, nBottomLine, crNewSize, lbCursorAtBottom, nCursorAtBottom, nScreenAtBottom, csbiNew);
	}

	#if 0
	// Последняя коррекция видимой области.
	// Левую граница - всегда 0 (горизонтальную прокрутку пока не поддерживаем)
	// Вертикальное положение - пляшем от rNewRect.Top

	rNewRect.Left = 0;
	rNewRect.Right = crHeight.X-1;

	if (lbScreenAtBottom)
	{
	}
	else if (lbCursorInScreen)
	{
	}
	else
	{
		TODO("Маркеры для блокировки положения в окне после заворота строк в Win10?");
	}

	rNewRect.Bottom = min((crHeight.Y-1), (rNewRect.Top+gcrVisibleSize.Y-1)); //-V592
	#endif

	_ASSERTE((rNewRect.Bottom-rNewRect.Top)<200);

	if (!SetConsoleWindowInfo(ghConOut, TRUE, &rNewRect))
	{
		dwErr = GetLastError();
	}

	LogSize(NULL, 0, lbRc ? "ApplyConsoleSizeBuffer OK" : "ApplyConsoleSizeBuffer FAIL", bForceWriteLog);

	return lbRc;
}


void RefillConsoleAttributes(const CONSOLE_SCREEN_BUFFER_INFO& csbi5, WORD OldText, WORD NewText)
{
	// Считать из консоли текущие атрибуты (построчно/поблочно)
	// И там, где они совпадают с OldText - заменить на in.SetConColor.NewTextAttributes
	DWORD nMaxLines = max(1,min((8000 / csbi5.dwSize.X),csbi5.dwSize.Y));
	WORD* pnAttrs = (WORD*)malloc(nMaxLines*csbi5.dwSize.X*sizeof(*pnAttrs));
	if (!pnAttrs)
	{
		// Memory allocation error
		return;
	}

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
			if ((pnAttrs[i] & 0xFF) == OldText)
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


// BufferHeight  - высота БУФЕРА (0 - без прокрутки)
// crNewSize     - размер ОКНА (ширина окна == ширине буфера)
// rNewRect      - для (BufferHeight!=0) определяет new upper-left and lower-right corners of the window
//	!!! rNewRect по идее вообще не нужен, за блокировку при прокрутке отвечает nSendTopLine
BOOL SetConsoleSize(USHORT BufferHeight, COORD crNewSize, SMALL_RECT rNewRect, LPCSTR asLabel, bool bForceWriteLog)
{
	_ASSERTE(ghConWnd);
	_ASSERTE(BufferHeight==0 || BufferHeight>crNewSize.Y); // Otherwise - it will be NOT a bufferheight...

	if (!ghConWnd) return FALSE;

	if (CheckWasFullScreen())
	{
		LogString("SetConsoleSize was skipped due to CONSOLE_FULLSCREEN_HARDWARE");
		return FALSE;
	}

	DWORD dwCurThId = GetCurrentThreadId();
	DWORD dwWait = 0;
	DWORD dwErr = 0;

	if ((gnRunMode == RM_SERVER) || (gnRunMode == RM_ALTSERVER))
	{
		// Запомним то, что последний раз установил сервер. пригодится
		gpSrv->nReqSizeBufferHeight = BufferHeight;
		gpSrv->crReqSizeNewSize = crNewSize;
		_ASSERTE(gpSrv->crReqSizeNewSize.X!=0);
		WARNING("выпилить gpSrv->rReqSizeNewRect и rNewRect");
		gpSrv->rReqSizeNewRect = rNewRect;
		gpSrv->sReqSizeLabel = asLabel;
		gpSrv->bReqSizeForceLog = bForceWriteLog;

		// Ресайз выполнять только в нити RefreshThread. Поэтому если нить другая - ждем...
		if (gpSrv->dwRefreshThread && dwCurThId != gpSrv->dwRefreshThread)
		{
			ResetEvent(gpSrv->hReqSizeChanged);
			if (InterlockedIncrement(&gpSrv->nRequestChangeSize) <= 0)
			{
				_ASSERTE(FALSE && "gpSrv->nRequestChangeSize has invalid value");
				gpSrv->nRequestChangeSize = 1;
			}
			// Ожидание, пока сработает RefreshThread
			HANDLE hEvents[2] = {ghQuitEvent, gpSrv->hReqSizeChanged};
			DWORD nSizeTimeout = REQSIZE_TIMEOUT;

			#ifdef _DEBUG
			if (IsDebuggerPresent())
				nSizeTimeout = INFINITE;
			#endif

			dwWait = WaitForMultipleObjects(2, hEvents, FALSE, nSizeTimeout);

			// Generally, it must be decremented by RefreshThread...
			if ((dwWait == WAIT_TIMEOUT) && (gpSrv->nRequestChangeSize > 0))
			{
				InterlockedDecrement(&gpSrv->nRequestChangeSize);
			}
			// Checking invalid value...
			if (gpSrv->nRequestChangeSize < 0)
			{
				// Decremented by RefreshThread and CurrentThread? Must not be...
				_ASSERTE(gpSrv->nRequestChangeSize >= 0);
				gpSrv->nRequestChangeSize = 0;
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

	if (gpLogSize) LogSize(&crNewSize, BufferHeight, asLabel);

	_ASSERTE(crNewSize.X>=MIN_CON_WIDTH && crNewSize.Y>=MIN_CON_HEIGHT);

	// Проверка минимального размера
	if (crNewSize.X</*4*/MIN_CON_WIDTH)
		crNewSize.X = /*4*/MIN_CON_WIDTH;

	if (crNewSize.Y</*3*/MIN_CON_HEIGHT)
		crNewSize.Y = /*3*/MIN_CON_HEIGHT;

	CONSOLE_SCREEN_BUFFER_INFO csbi = {};

	// Нам нужно реальное состояние консоли, чтобы не поломать ее вид после ресайза
	if (!GetConsoleScreenBufferInfo(ghConOut, &csbi))
	{
		DWORD nErrCode = GetLastError();
		_ASSERTE(FALSE && "GetConsoleScreenBufferInfo was failed");
		SetLastError(nErrCode ? nErrCode : ERROR_INVALID_HANDLE);
		return FALSE;
	}

	BOOL lbRc = TRUE;

	if (!AdaptConsoleFontSize(crNewSize))
	{
		lbRc = FALSE;
		goto wrap;
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

	// The resize itself
	if (BufferHeight == 0)
	{
		// No buffer in the console
		lbRc = ApplyConsoleSizeSimple(crNewSize, csbi, dwErr, bForceWriteLog);
	}
	else
	{
		// Начался ресайз для BufferHeight
		lbRc = ApplyConsoleSizeBuffer(gnBufferHeight, crNewSize, csbi, dwErr, bForceWriteLog);
	}


wrap:
	gpSrv->bRequestChangeSizeResult = lbRc;

	if ((gnRunMode == RM_SERVER) && gpSrv->hRefreshEvent)
	{
		SetEvent(gpSrv->hRefreshEvent);
	}

	return lbRc;
}

#if defined(__GNUC__)
extern "C"
#endif
BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
	if (gbIsDownloading
		&& ((dwCtrlType == CTRL_CLOSE_EVENT) || (dwCtrlType == CTRL_LOGOFF_EVENT) || (dwCtrlType == CTRL_SHUTDOWN_EVENT)
			|| (dwCtrlType == CTRL_C_EVENT) || (dwCtrlType == CTRL_BREAK_EVENT)))
	{
		DownloadCommand(dc_RequestTerminate, 0, NULL);
	}

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

		// Our debugger is running?
		if (gpSrv->DbgInfo.bDebuggerActive)
		{
			// pfnDebugActiveProcessStop is useless, because
			// 1. pfnDebugSetProcessKillOnExit was called already
			// 2. we can debug more than a one process

			//gpSrv->DbgInfo.bDebuggerActive = FALSE;
		}
		else
		{
			#if 0
			wchar_t szTitle[128]; wsprintf(szTitle, L"ConEmuC, PID=%u", GetCurrentProcessId());
			//MessageBox(NULL, L"CTRL_CLOSE_EVENT in ConEmuC", szTitle, MB_SYSTEMMODAL);
			DWORD nWait = WaitForSingleObject(ghExitQueryEvent, CLOSE_CONSOLE_TIMEOUT);
			if (nWait == WAIT_OBJECT_0)
			{
				#ifdef _DEBUG
				OutputDebugString(L"All console processes was terminated\n");
				#endif
			}
			else
			{
				// Поскольку мы (сервер) сейчас свалимся, то нужно показать
				// консольное окно, раз в нем остались какие-то процессы
				EmergencyShow();
			}
			#endif

			// trick to let ConsoleMain2() finish correctly
			ExitThread(1);
		}
	}
	else if ((dwCtrlType == CTRL_C_EVENT) || (dwCtrlType == CTRL_BREAK_EVENT))
	{
		if (gbTerminateOnCtrlBreak)
		{
			PRINT_COMSPEC(L"Ctrl+Break received, server will be terminated\n", 0);
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
				PRINT_COMSPEC(L"Ctrl+Break received, debugger will be stopped\n", 0);
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

	if (asAddLine)
	{
		_wprintf(asAddLine);
		_printf("\n");
	}
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
	DEBUGTEST(BOOL bWriteRC =)
	WriteFile(hOut, asBuffer, nAllLen, &dwWritten, 0);
	UNREFERENCED_PARAMETER(dwWritten);
}

void print_error(DWORD dwErr/*= 0*/, LPCSTR asFormat/*= NULL*/)
{
	if (!dwErr)
		dwErr = GetLastError();

	wchar_t* lpMsgBuf = NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&lpMsgBuf, 0, NULL);

	_printf(asFormat ? asFormat : "\nErrCode=0x%08X, Description:\n", dwErr);
	_wprintf((!lpMsgBuf || !*lpMsgBuf) ? L"<Unknown error>" : lpMsgBuf);

	if (lpMsgBuf) LocalFree(lpMsgBuf);
	SetLastError(dwErr);
}

bool IsOutputRedirected()
{
	static int isRedirected = 0;
	if (isRedirected)
	{
		return (isRedirected == 2);
	}

	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO sbi = {};
	BOOL bIsConsole = GetConsoleScreenBufferInfo(hOut, &sbi);
	if (bIsConsole)
	{
		isRedirected = 1;
		return false;
	}
	else
	{
		isRedirected = 2;
		return true;
	}
}

void _wprintf(LPCWSTR asBuffer)
{
	if (!asBuffer) return;

	int nAllLen = lstrlenW(asBuffer);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwWritten = 0;

	if (!IsOutputRedirected())
	{
		WriteConsoleW(hOut, asBuffer, nAllLen, &dwWritten, 0);
	}
	else
	{
		UINT  cp = GetConsoleOutputCP();
		int cchMax = WideCharToMultiByte(cp, 0, asBuffer, -1, NULL, 0, NULL, NULL) + 1;
		char* pszOem = (cchMax > 1) ? (char*)malloc(cchMax) : NULL;
		if (pszOem)
		{
			int nWrite = WideCharToMultiByte(cp, 0, asBuffer, -1, pszOem, cchMax, NULL, NULL);
			if (nWrite > 1)
			{
				// Don't write terminating '\0' to redirected output
				WriteFile(hOut, pszOem, nWrite-1, &dwWritten, 0);
			}
			free(pszOem);
		}
	}
}

void DisableAutoConfirmExit(BOOL abFromFarPlugin)
{
	// Консоль могла быть приаттачена к GUI в тот момент, когда сервер ожидает
	// от юзера подтверждение закрытия консоли
	if (!gbInExitWaitForKey)
	{
		_ASSERTE(gnConfirmExitParm==0 || abFromFarPlugin);
		gbAutoDisableConfirmExit = FALSE; gbAlwaysConfirmExit = FALSE;
		// менять nProcessStartTick не нужно. проверка только по флажкам
		//gpSrv->nProcessStartTick = GetTickCount() - 2*CHECK_ROOTSTART_TIMEOUT;
	}
}

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
					_wsprintf(szWide, SKIPLEN(countof(szWide)) L"ConsKeybLayout changed from %s to %s", gpSrv->szKeybLayout, szCurKeybLayout);
					WideCharToMultiByte(CP_ACP,0,szWide,-1,szInfo,128,0,0);
					LogFunction(szInfo);
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
		//WARNING("BUGBUG: 16 цифр не вернет"); -- тут именно 8 цифр. Это LayoutNAME, а не string(HKL)
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

	//WARNING("BUGBUG: 16 цифр не вернет"); -- тут именно 8 цифр. Это LayoutNAME, а не string(HKL)
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
