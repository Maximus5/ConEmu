
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
#include "../common/WCodePage.h"
#include "../common/WConsole.h"
#include "../common/WFiles.h"
#include "../common/WThreads.h"
#include "../common/WUser.h"
#include "../ConEmu/version.h"
#include "../ConEmuHk/Injects.h"
#include "Actions.h"
#include "ConProcess.h"
#include "ConsoleHelp.h"
#include "GuiMacro.h"
#include "Debugger.h"
#include "StartEnv.h"
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
wchar_t gsVersion[20] = L"";
wchar_t gsSelfExe[MAX_PATH] = L"";  // Full path+exe to our executable
wchar_t gsSelfPath[MAX_PATH] = L""; // Directory of our executable
BOOL    gbTerminateOnExit = FALSE;
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
RConStartArgs::CloseConfirm gnConfirmExitParm = RConStartArgs::eConfDefault; // | eConfAlways | eConfNever | eConfEmpty | eConfHalt
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
wchar_t* gpszTaskCmd = NULL;
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

void InitVersion()
{
	wchar_t szMinor[8] = L""; lstrcpyn(szMinor, _T(MVV_4a), countof(szMinor));
	//msprintf не умеет "%02u"
	_wsprintf(gsVersion, SKIPLEN(countof(gsVersion)) L"%02u%02u%02u%s", MVV_1, MVV_2, MVV_3, szMinor);
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

			hkFunc.Init(WIN3264TEST(L"ConEmuCD.dll",L"ConEmuCD64.dll"), ghOurModule);

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

			InitVersion();

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
	if (gnRunMode == RM_GUIMACRO)
	{
		// Must not be called in GuiMacro mode!
		_ASSERTE(gnRunMode!=RM_GUIMACRO);
		return;
	}

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
void ShowServerStartedMsgBox(LPCWSTR asCmdLine)
{
	wchar_t szTitle[100]; _wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuC [Server] started (PID=%i)", gnSelfPID);
	const wchar_t* pszCmdLine = asCmdLine;
	MessageBox(NULL,pszCmdLine,szTitle,0);
}
#endif


void LoadExePath()
{
	// Already loaded?
	if (gsSelfExe[0] && gsSelfPath[0])
		return;

	DWORD nSelfLen = GetModuleFileNameW(NULL, gsSelfExe, countof(gsSelfExe));
	if (!nSelfLen || (nSelfLen >= countof(gsSelfExe)))
	{
		_ASSERTE(FALSE && "GetModuleFileNameW(NULL) failed");
		gsSelfExe[0] = 0;
	}
	else
	{
		lstrcpyn(gsSelfPath, gsSelfExe, countof(gsSelfPath));
		wchar_t* pszSlash = (wchar_t*)PointToName(gsSelfPath);
		if (pszSlash && (pszSlash > gsSelfPath))
			*pszSlash = 0;
		else
			gsSelfPath[0] = 0;
	}
}

void UnlockCurrentDirectory()
{
	if ((gnRunMode == RM_SERVER) && gsSelfPath[0])
	{
		SetCurrentDirectory(gsSelfPath);
	}
}


bool CheckAndWarnHookers()
{
	if (gbSkipHookersCheck)
	{
		if (gpLogSize) gpLogSize->LogString(L"CheckAndWarnHookers skipped due to /OMITHOOKSWARN switch");
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
int __stdcall ConsoleMain3(int anWorkMode/*0-Server&ComSpec,1-AltServer,2-Reserved,3-GuiMacro*/, LPCWSTR asCmdLine)
{
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


	switch (anWorkMode)
	{
	case 0:
		gnRunMode = RM_SERVER; break;
	case 1:
		gnRunMode = RM_ALTSERVER; break;
	case 3:
		gnRunMode = RM_GUIMACRO; break;
	default:
		gnRunMode = RM_UNDEFINED;
	}

	// Check linker fails!
	if (ghOurModule == NULL)
	{
		wchar_t szTitle[128]; _wsprintf(szTitle, SKIPLEN(countof(szTitle)) WIN3264TEST(L"ConEmuCD",L"ConEmuCD64") L", PID=%u", GetCurrentProcessId());
		MessageBox(NULL, L"ConsoleMain2: ghOurModule is NULL\nDllMain was not executed", szTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
		return CERR_DLLMAIN_SKIPPED;
	}

	if (!gpSrv && (gnRunMode != RM_GUIMACRO))
		gpSrv = (SrvInfo*)calloc(sizeof(SrvInfo),1);
	if (gpSrv)
	{
		gpSrv->InitFields();

		if (ghConEmuWnd)
		{
			GetWindowThreadProcessId(ghConEmuWnd, &gnConEmuPID);
		}
	}

#if defined(SHOW_STARTED_PRINT)
	BOOL lbDbgWrite; DWORD nDbgWrite; HANDLE hDbg; char szDbgString[255], szHandles[128];
	wchar_t szTitle[255];
	_wsprintf(szTitle, SKIPCOUNT(szTitle) L"ConEmuCD[%u]: PID=%u", WIN3264TEST(32,64), GetCurrentProcessId());
	MessageBox(0, asCmdLine, szTitle, MB_SYSTEMMODAL);
	hDbg = CreateFile(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
	                  0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	_wsprintfA(szHandles, SKIPCOUNT(szHandles) "STD_OUTPUT_HANDLE(0x%08X) STD_ERROR_HANDLE(0x%08X) CONOUT$(0x%08X)",
	        (DWORD)GetStdHandle(STD_OUTPUT_HANDLE), (DWORD)GetStdHandle(STD_ERROR_HANDLE), (DWORD)hDbg);
	printf("ConEmuC: Printf: %s\n", szHandles);
	_wsprintfA(szDbgString, SKIPCOUNT(szDbgString) "ConEmuC: STD_OUTPUT_HANDLE: %s\n", szHandles);
	lbDbgWrite = WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), szDbgString, lstrlenA(szDbgString), &nDbgWrite, NULL);
	_wsprintfA(szDbgString, SKIPCOUNT(szDbgString) "ConEmuC: STD_ERROR_HANDLE:  %s\n", szHandles);
	lbDbgWrite = WriteFile(GetStdHandle(STD_ERROR_HANDLE), szDbgString, lstrlenA(szDbgString), &nDbgWrite, NULL);
	_wsprintfA(szDbgString, SKIPCOUNT(szDbgString) "ConEmuC: CONOUT$: %s", szHandles);
	lbDbgWrite = WriteFile(hDbg, szDbgString, lstrlenA(szDbgString), &nDbgWrite, NULL);
	CloseHandle(hDbg);
	//_wsprintfA(szDbgString, SKIPCOUNT(szDbgString) "ConEmuC: PID=%u", GetCurrentProcessId());
	//MessageBoxA(0, "Press Ok to continue", szDbgString, MB_SYSTEMMODAL);
#elif defined(SHOW_STARTED_PRINT_LITE)
	{
	wchar_t szPath[MAX_PATH]; GetModuleFileNameW(NULL, szPath, countof(szPath));
	wchar_t szDbgMsg[MAX_PATH*2];
	_wsprintf(szDbgMsg, SKIPCOUNT(szDbgMsg) L"%s started, PID=%u\n", PointToName(szPath), GetCurrentProcessId());
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
		wchar_t szTitle[100]; _wsprintf(szTitle, SKIPCOUNT(szTitle) L"ConEmuCD[%u]: PID=%u", WIN3264TEST(32,64) GetCurrentProcessId());
		MessageBox(NULL, asCmdLine, szTitle, MB_SYSTEMMODAL);
	}
	#endif


	#ifdef SHOW_STARTED_ASSERT
	if (!IsDebuggerPresent())
	{
		_ASSERT(FALSE);
	}
	#endif


	LoadExePath();


	PRINT_COMSPEC(L"ConEmuC started: %s\n", asCmdLine);
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

	// Event is used in almost all modes, even in "debugger"
	if (!ghExitQueryEvent
		&& (gnRunMode != RM_GUIMACRO)
		)
	{
		ghExitQueryEvent = CreateEvent(NULL, TRUE/*используется в нескольких нитях, manual*/, FALSE, NULL);
	}

	LPCWSTR pszFullCmdLine = asCmdLine;
	wchar_t szDebugCmdLine[MAX_PATH];
	lstrcpyn(szDebugCmdLine, pszFullCmdLine ? pszFullCmdLine : L"", countof(szDebugCmdLine));

	if (gnRunMode == RM_GUIMACRO)
	{
		// Separate parser function
		iRc = GuiMacroCommandLine(pszFullCmdLine);
		_ASSERTE(iRc == CERR_GUIMACRO_SUCCEEDED || iRc == CERR_GUIMACRO_FAILED);
		goto wrap;
	}
	else if (anWorkMode)
	{
		// Alternative mode
		_ASSERTE(anWorkMode==1); // может еще и 2 появится - для StandAloneGui
		_ASSERTE(gnRunMode == RM_UNDEFINED || gnRunMode == RM_ALTSERVER);
		gnRunMode = RM_ALTSERVER;
		_ASSERTE(!gbCreateDumpOnExceptionInstalled);
		_ASSERTE(gbAttachMode==am_None);
		if (!(gbAttachMode & am_Modes))
			gbAttachMode |= am_Simple;
		gnConfirmExitParm = RConStartArgs::eConfNever;
		gbAlwaysConfirmExit = FALSE; gbAutoDisableConfirmExit = FALSE;
		gbNoCreateProcess = TRUE;
		gbAlienMode = TRUE;
		gpSrv->dwRootProcess = GetCurrentProcessId();
		gpSrv->hRootProcess = GetCurrentProcess();
		//gnConEmuPID = ...;
		gpszRunCmd = (wchar_t*)calloc(1,2);
		CreateColorerHeader();
	}
	else if ((iRc = ParseCommandLine(pszFullCmdLine)) != 0)
	{
		wchar_t szLog[80];
		_wsprintf(szLog, SKIPCOUNT(szLog) L"ParseCommandLine returns %i, exiting", iRc);
		LogFunction(szLog);
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
			_wprintf(asCmdLine);
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
		ShowServerStartedMsgBox(asCmdLine);
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

	if (!ghQuitEvent)
		ghQuitEvent = CreateEvent(NULL, TRUE/*used in several threads, manual!*/, FALSE, NULL);

	if (!ghQuitEvent)
	{
		dwErr = GetLastError();
		_printf("CreateEvent() failed, ErrCode=0x%08X\n", dwErr);
		iRc = CERR_EXITEVENT; goto wrap;
	}

	ResetEvent(ghQuitEvent);
	xf_check();


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

	/* ******************************** */
	/* ****** "Server-mode" init ****** */
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
	/* ****** Start child process ****** */
	/* ********************************* */
#ifdef SHOW_STARTED_PRINT
	_wsprintfA(szDbgString, SKIPLEN(szDbgString) "ConEmuC: PID=%u", GetCurrentProcessId());
	MessageBoxA(0, "Press Ok to continue", szDbgString, MB_SYSTEMMODAL);
#endif

	// Don't use CREATE_NEW_PROCESS_GROUP, or Ctrl-C stops working

	// We need to set `0` before CreateProcess, otherwise we may get timeout,
	// due to misc antivirus software, BEFORE CreateProcess finishes
	if (!(gbAttachMode & am_Modes))
		gpSrv->nProcessStartTick = 0;

	if (gbNoCreateProcess)
	{
		// Process already started, just attach RealConsole to ConEmu (VirtualConsole)
		lbRc = TRUE;
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
				CEStr szExe;
				if (NextArg(&psz, szExe, &pszStart) == 0)
				{
					MWow64Disable wow;
					if (!gbSkipWowChange) wow.Disable();

					DWORD RunImageSubsystem = 0, RunImageBits = 0, RunFileAttrs = 0;
					bool bSubSystem = GetImageSubsystem(szExe, RunImageSubsystem, RunImageBits, RunFileAttrs);

					if (!bSubSystem)
					{
						// szExe may be simple "notepad", we must seek for executable...
						CEStr szFound;
						// We are interesting only on ".exe" files,
						// supposing that other executable extensions can't be GUI applications
						if (apiSearchPath(NULL, szExe, L".exe", szFound))
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
		#endif
		CEStr szExeName;
		{
			LPCWSTR pszStart = gpszRunCmd;
			if (NextArg(&pszStart, szExeName) == 0)
			{
				#ifdef _DEBUG
				if (FileExists(szExeName))
				{
					pszRunCmpApp = szExeName;
					pszRunCmpApp = NULL;
				}
				#endif
			}
		}

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
			if (gsSelfExe[0] != 0)
			{
				wchar_t szSelf[MAX_PATH*2];
				wcscpy_c(szSelf, gsSelfExe);

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

			if (gbDontInjectConEmuHk
				|| (!szExeName.IsEmpty() && IsConsoleServer(szExeName)))
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
			CEStr szExec;

			if (NextArg(&pszCmd, szExec) == 0)
			{
				SHELLEXECUTEINFO sei = {sizeof(SHELLEXECUTEINFO)};
				sei.hwnd = ghConEmuWnd;
				sei.fMask = SEE_MASK_NO_CONSOLE; //SEE_MASK_NOCLOSEPROCESS; -- смысла ждать завершения нет - процесс запускается в новой консоли
				wcscpy_c(szVerb, L"open"); sei.lpVerb = szVerb;
				sei.lpFile = szExec.ms_Val;
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

	/* *************************** */
	/* *** Waiting for startup *** */
	/* *************************** */

	// Don't "lock" startup folder
	UnlockCurrentDirectory();

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


	// JIC, if there was "goto"
	UnlockCurrentDirectory();


	if (gnRunMode == RM_ALTSERVER)
	{
		// Alternative server, we can't wait for "self" termination
		iRc = 0;
		goto AltServerDone;
	} // (gnRunMode == RM_ALTSERVER)
	else if (gnRunMode == RM_SERVER)
	{
		nExitPlaceStep = EPS_WAITING4PROCESS/*550*/;
		// There is at least one process in .onsole. Wait until there would be nobody except us.
		nWait = WAIT_TIMEOUT; nWaitExitEvent = -2;

		_ASSERTE(!gpSrv->DbgInfo.bDebuggerActive);

		#ifdef _DEBUG
		while (nWait == WAIT_TIMEOUT)
		{
			nWait = nWaitExitEvent = WaitForSingleObject(ghExitQueryEvent, 100);
			// Not actual? Что-то при загрузке компа иногда все-таки не дожидается, когда процесс в консоли появится
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


		// Root ExitCode
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

		// Close all handles now
		if (pi.hProcess)
			SafeCloseHandle(pi.hProcess);

		if (pi.hThread)
			SafeCloseHandle(pi.hThread);

		#ifdef _DEBUG
		xf_validate(NULL);
		#endif
	} // (gnRunMode == RM_COMSPEC)

	/* *********************** */
	/* *** Finalizing work *** */
	/* *********************** */
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

		// Post information to GUI
		if (gnMainServerPID)
		{
			CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_GETROOTINFO, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_ROOT_INFO));
			if (pIn && GetRootInfo(pIn))
			{
				CESERVER_REQ *pSrvOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd, TRUE/*async*/);
				ExecuteFreeResult(pSrvOut);
			}
			ExecuteFreeResult(pIn);
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
		UnlockCurrentDirectory();

		//#ifdef _DEBUG
		//if (!gbInShutdown)
		//	MessageBox(0, L"ExitWaitForKey", L"ConEmuC", MB_SYSTEMMODAL);
		//#endif

		BOOL lbProcessesLeft = FALSE, lbDontShowConsole = FALSE;
		BOOL lbLineFeedAfter = TRUE;
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
		else if ((gnConfirmExitParm == RConStartArgs::eConfEmpty)
				|| (gnConfirmExitParm == RConStartArgs::eConfHalt))
		{
			lbLineFeedAfter = FALSE; // Don't print anything to console
		}
		else
		{
			if (gbRootWasFoundInCon == 1)
			{
				// If root process has been working less than CHECK_ROOTOK_TIMEOUT
				if (gbRootAliveLess10sec && (gnConfirmExitParm != RConStartArgs::eConfAlways))
				{
					static wchar_t szMsg[255];
					if (gnExitCode)
					{
						PrintExecuteError(gpszRunCmd, 0, L"\n");
					}
					wchar_t szExitCode[40];
					if (gnExitCode > 255)
						_wsprintf(szExitCode, SKIPLEN(countof(szExitCode)) L"x%08X(%u)", gnExitCode, gnExitCode);
					else
						_wsprintf(szExitCode, SKIPLEN(countof(szExitCode)) L"%u", gnExitCode);
					_wsprintf(szMsg, SKIPLEN(countof(szMsg))
						L"\n\nConEmuC: Root process was alive less than 10 sec, ExitCode=%s.\nPress Enter or Esc to close console...",
						szExitCode);
					pszMsg = szMsg;
				}
				else
				{
					pszMsg = L"\n\nPress Enter or Esc to close console...";
				}
			}
		}

		if (!pszMsg
			&& (gnConfirmExitParm != RConStartArgs::eConfEmpty)
			&& (gnConfirmExitParm != RConStartArgs::eConfHalt))
		{
			// Let's show anything (default message)
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

		DWORD keys = (gnConfirmExitParm == RConStartArgs::eConfHalt) ? 0
			: (VK_RETURN|(VK_ESCAPE<<8));
		ExitWaitForKey(keys, pszMsg, lbLineFeedAfter, lbDontShowConsole);

		UNREFERENCED_PARAMETER(nProcCount);
		UNREFERENCED_PARAMETER(nProcesses[0]);

		// During the wait, new process may be started in our console
		{
			int nCount = gpSrv->nProcessCount;

			if (nCount > 1 || gpSrv->DbgInfo.bDebuggerActive)
			{
				// OK, new root found, wait for it
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
	SafeFree(gpszTaskCmd);
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

#if defined(__GNUC__)
extern "C"
#endif
int __stdcall ConsoleMain2(int anWorkMode/*0-Server&ComSpec,1-AltServer,2-Reserved*/)
{
	return ConsoleMain3(anWorkMode, GetCommandLineW());
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
			LogString(szFailMsg);
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
				LogString(szFailMsg);
				liArgsFailed = 2;
				// will return CERR_CARGUMENT
			}
		}
	}
	else if (pfnGetConsoleProcessList==NULL)
	{
		wcscpy_c(szFailMsg, L"Attach to console app was requested, but required WinXP or higher!");
		LogString(szFailMsg);
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
			LogString(szFailMsg);
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
				LogString(szFailMsg);
				liArgsFailed = 5;
				//will return CERR_CARGUMENT
			}
			else if ((gpSrv->dwRootProcess == 0) && (nProcCount > 2))
			{
				_wsprintf(szFailMsg, SKIPLEN(countof(szFailMsg)) L"Attach to GUI was requested, but\n" L"there is more than 2 console processes: %s\n", szProc);
				LogString(szFailMsg);
				liArgsFailed = 6;
				//will return CERR_CARGUMENT
			}
		}
	}

	if (liArgsFailed)
	{
		DWORD nSelfPID = GetCurrentProcessId();
		PROCESSENTRY32 self = {sizeof(self)}, parent = {sizeof(parent)};
		// Not optimal, needs refactoring
		if (GetProcessInfo(nSelfPID, &self))
			GetProcessInfo(self.th32ParentProcessID, &parent);

		LPCWSTR pszCmdLine = GetCommandLineW(); if (!pszCmdLine) pszCmdLine = L"";

		wchar_t szTitle[MAX_PATH*2];
		_wsprintf(szTitle, SKIPLEN(countof(szTitle))
			L"ConEmuC %s [%u], PID=%u, Code=%i" L"\r\n"
			L"ParentPID=%u: %s" L"\r\n"
			L"  ", // szFailMsg follows this
			gsVersion, WIN3264TEST(32,64), nSelfPID, liArgsFailed,
			self.th32ParentProcessID, parent.szExeFile[0] ? parent.szExeFile : L"<terminated>");

		CEStr lsMsg = lstrmerge(szTitle, szFailMsg, L"\r\nCommand line:\r\n  ", pszCmdLine);

		// Avoid automatic termination of ExitWaitForKey
		gbInShutdown = FALSE;

		// Force light-red on black for error message
		MSetConTextAttr setAttr(ghConOut, 12);

		const DWORD nAttachErrorTimeoutMessage = 15*1000; // 15 sec
		ExitWaitForKey(VK_RETURN|(VK_ESCAPE<<8), lsMsg, true, true, nAttachErrorTimeoutMessage);

		LogString(L"CheckAttachProcess: CERR_CARGUMENT after ExitWaitForKey");

		gbInShutdown = TRUE;
		return CERR_CARGUMENT;
	}

	return 0; // OK
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

	CEStr   szTemp;
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
		wchar_t* pszName = (wchar_t*)PointToName(szTemp.ms_Val);
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
		CEStr szApp;

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
					szApp.ms_Val[nLen] = 0;

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
		CStartEnv setEnv;
		gpSetEnv->Apply(&setEnv);
	}
}

// Lines come from Settings/Environment page
void ApplyEnvironmentCommands(LPCWSTR pszCommands)
{
	if (!pszCommands || !*pszCommands)
	{
		_ASSERTE(pszCommands && *pszCommands);
		return;
	}

	UINT nSetCP = 0; // Postponed

	if (!gpSetEnv)
		gpSetEnv = new CProcessEnvCmd();

	// These must be applied before commands from CommandLine
	gpSetEnv->AddLines(pszCommands, true);
}

// Allow smth like: ConEmuC -c {Far} /e text.txt
wchar_t* ExpandTaskCmd(LPCWSTR asCmdLine)
{
	if (!ghConWnd)
	{
		_ASSERTE(ghConWnd);
		return NULL;
	}
	if (!asCmdLine || (asCmdLine[0] != TaskBracketLeft))
	{
		_ASSERTE(asCmdLine && (asCmdLine[0] == TaskBracketLeft));
		return NULL;
	}
	LPCWSTR pszNameEnd = wcschr(asCmdLine, TaskBracketRight);
	if (!pszNameEnd)
		return NULL;
	pszNameEnd++;

	size_t cchCount = (pszNameEnd - asCmdLine);
	DWORD cbSize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_TASK) + cchCount*sizeof(asCmdLine[0]);
	CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_GETTASKCMD, cbSize);
	if (!pIn)
		return NULL;
	wmemmove(pIn->GetTask.data, asCmdLine, cchCount);
	_ASSERTE(pIn->GetTask.data[cchCount] == 0);

	wchar_t* pszResult = NULL;
	CESERVER_REQ* pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
	if (pOut && (pOut->DataSize() > sizeof(pOut->GetTask)) && pOut->GetTask.data[0])
	{
		LPCWSTR pszTail = SkipNonPrintable(pszNameEnd);
		pszResult = lstrmerge(pOut->GetTask.data, (pszTail && *pszTail) ? L" " : NULL, pszTail);
	}
	ExecuteFreeResult(pIn);
	ExecuteFreeResult(pOut);

	return pszResult;
}

// Parse ConEmuC command line switches
int ParseCommandLine(LPCWSTR asCmdLine)
{
	int iRc = 0;
	CEStr szArg;
	CEStr szExeTest;
	LPCWSTR pszArgStarts = NULL;
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
		else if (lstrcmpi(szArg, L"/TestUnicode")==0)
		{
			eExecAction = ea_TestUnicodeCvt;
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
		else if (lstrcmpi(szArg, L"/STRUCT") == 0)
		{
			eExecAction = ea_DumpStruct;
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
		else if ((wcscmp(szArg, L"/CONFIRM")==0)
			|| (wcscmp(szArg, L"/CONFHALT")==0)
			|| (wcscmp(szArg, L"/ECONFIRM")==0))
		{
			gnConfirmExitParm = (wcscmp(szArg, L"/CONFIRM")==0) ? RConStartArgs::eConfAlways
				: (wcscmp(szArg, L"/CONFHALT")==0) ? RConStartArgs::eConfHalt
				: RConStartArgs::eConfEmpty;
			gbAlwaysConfirmExit = TRUE; gbAutoDisableConfirmExit = FALSE;
		}
		else if (wcscmp(szArg, L"/NOCONFIRM")==0)
		{
			gnConfirmExitParm = RConStartArgs::eConfNever;
			gbAlwaysConfirmExit = FALSE; gbAutoDisableConfirmExit = FALSE;
		}
		else if (wcscmp(szArg, L"/OMITHOOKSWARN")==0)
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
				wchar_t szTitle[100]; _wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"%s PID=%u %s", gsModuleName, gnSelfPID, szArg.ms_Val);
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
			pszStart = szArg.ms_Val+14;
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
				pszStart = szArg.ms_Val+8;
			}
			else if (wcsncmp(szArg, L"/FARPID=", 8)==0)
			{
				gbAttachFromFar = TRUE;
				gbRootIsCmdExe = FALSE;
				pszStart = szArg.ms_Val+8;
			}
			else if (wcsncmp(szArg, L"/CONPID=", 8)==0)
			{
				//_ASSERTE(FALSE && "Continue to alternative attach mode");
				gbAlternativeAttach = TRUE;
				gbRootIsCmdExe = FALSE;
				pszStart = szArg.ms_Val+8;
			}
			else
			{
				pszStart = szArg.ms_Val+5;
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
					LogString(L"CERR_CARGUMENT: (gbAlternativeAttach && gpSrv->dwRootProcess)");
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
				LogString("CERR_CARGUMENT: Attach to GUI was requested, but invalid PID specified");
				_printf("Attach to GUI was requested, but invalid PID specified:\n");
				_wprintf(GetCommandLineW());
				_printf("\n");
				_ASSERTE(FALSE && "Attach to GUI was requested, but invalid PID specified");
				return CERR_CARGUMENT;
			}
		}
		else if (wcsncmp(szArg, L"/CINMODE=", 9)==0)
		{
			wchar_t* pszEnd = NULL, *pszStart = szArg.ms_Val+9;
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
				LogString(L"CERR_CARGUMENT: Invalid GUI PID specified");
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
				wchar_t szLog[120];
				LPCWSTR pszDescr = szArg+7;
				if (pszDescr[0] == L'0' && (pszDescr[1] == L'x' || pszDescr[1] == L'X'))
					pszDescr += 2; // That may be useful for calling from batch files
				gpSrv->hGuiWnd = (HWND)wcstoul(pszDescr, &pszEnd, 16);
				gpSrv->bRequestNewGuiWnd = FALSE;

				BOOL isWnd = gpSrv->hGuiWnd ? IsWindow(gpSrv->hGuiWnd) : FALSE;
				DWORD nErr = gpSrv->hGuiWnd ? GetLastError() : 0;

				_wsprintf(szLog, SKIPCOUNT(szLog) L"GUI HWND=0x%08X, %s, ErrCode=%u", LODWORD(gpSrv->hGuiWnd), isWnd ? L"Valid" : L"Invalid", nErr);
				LogString(szLog);

				if (!isWnd)
				{
					LogString(L"CERR_CARGUMENT: Invalid GUI HWND was specified in /GHWND arg");
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
				LogString(L"CERR_CARGUMENT: Debug of process was requested, but invalid PID specified");
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
				LogString(L"CERR_CARGUMENT: Debug of process was requested, but GetCommandLineW failed");
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
				LogString(L"CERR_CARGUMENT: Debug of process was requested, but command was not found");
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
		else if (lstrcmpi(szArg, L"/AUTOMINI")==0)
		{
			//_ASSERTE(FALSE && "Continue to /AUTOMINI");
			gpSrv->DbgInfo.nDebugDumpProcess = 0;
			gpSrv->DbgInfo.bAutoDump = TRUE;
			gpSrv->DbgInfo.nAutoInterval = 1000;
			if (lsCmdLine && *lsCmdLine && isDigit(lsCmdLine[0])
				&& (NextArg(&lsCmdLine, szArg, &pszArgStarts) == 0))
			{
				wchar_t* pszEnd;
				DWORD nVal = wcstol(szArg, &pszEnd, 10);
				if (nVal)
				{
					if (pszEnd && *pszEnd)
					{
						if (lstrcmpni(pszEnd, L"ms", 2) == 0)
						{
							// Already milliseconds
							pszEnd += 2;
						}
						else if (lstrcmpni(pszEnd, L"s", 1) == 0)
						{
							nVal *= 60; // seconds
							pszEnd++;
						}
						else if (lstrcmpni(pszEnd, L"m", 2) == 0)
						{
							nVal *= 60*60; // minutes
							pszEnd++;
						}
					}
					gpSrv->DbgInfo.nAutoInterval = nVal;
				}
			}
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
			ShowServerStartedMsgBox(asCmdLine);
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

			if (lsCmdLine && (lsCmdLine[0] == TaskBracketLeft) && wcschr(lsCmdLine, TaskBracketRight))
			{
				// Allow smth like: ConEmuC -c {Far} /e text.txt
				gpszTaskCmd = ExpandTaskCmd(lsCmdLine);
				if (gpszTaskCmd && *gpszTaskCmd)
					lsCmdLine = gpszTaskCmd;
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
			if (gpLogSize)
			{
				if (!ghConWnd) { LogFunction(L"!ghConWnd"); }
				else if (!lbIsWindowVisible) { LogFunction(L"!IsAutoAttachAllowed"); }
				else { LogFunction(L"isTerminalMode"); }
			}
			// Но это может быть все-таки наше окошко. Как проверить...
			// Найдем первый параметр
			LPCWSTR pszSlash = lsCmdLine ? wcschr(lsCmdLine, L'/') : NULL;
			if (pszSlash)
			{
				LogFunction(pszSlash);
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
		LogString(L"CERR_CARGUMENT: Parsing command line failed (/C argument not found)");
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
			if (!gpSetEnv)
				gpSetEnv = new CProcessEnvCmd();
			CStartEnvTitle setTitleVar(&gpszForcedTitle);
			ProcessSetEnvCmd(lsCmdLine, gpSetEnv, &setTitleVar);
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

		if (!GetEnvironmentVariable(L"ComSpec", gszComSpec, MAX_PATH) || gszComSpec[0] == 0)
			gszComSpec[0] = 0;

		// ComSpec/ComSpecC не определен, используем cmd.exe
		if (gszComSpec[0] == 0)
		{
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

int ExitWaitForKey(DWORD vkKeys, LPCWSTR asConfirm, BOOL abNewLine, BOOL abDontShowConsole, DWORD anMaxTimeout /*= 0*/)
{
	gbInExitWaitForKey = TRUE;

	int nKeyPressed = -1;

	//-- Don't exit on ANY key if -new_console:c1
	//if (!vkKeys) vkKeys = VK_ESCAPE;

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

				if (ghConEmuWndDC && !isConEmuTerminated())
				{
					// ConEmu will deal the situation?
					// EmergencyShow annoys user if parent window was killed (InsideMode)
					lbGuiAlive = TRUE;
				}
				else if (ghConEmuWnd && IsWindow(ghConEmuWnd))
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
	DWORD nPreFlush = 0, nPostFlush = 0;
	#ifdef _DEBUG
	DWORD nPreQuit = 0;
	#endif

	GetNumberOfConsoleInputEvents(hIn, &nPreFlush);

	FlushConsoleInputBuffer(hIn);

	PRINT_COMSPEC(L"Finalizing. gbInShutdown=%i\n", gbInShutdown);
	GetNumberOfConsoleInputEvents(hIn, &nPostFlush);

	if (gbInShutdown)
		goto wrap; // Event закрытия мог припоздниться

	SetConsoleMode(hOut, ENABLE_PROCESSED_OUTPUT|ENABLE_WRAP_AT_EOL_OUTPUT);

	//
	if (asConfirm && *asConfirm)
	{
		_wprintf(asConfirm);
	}

	DWORD nStartTick = GetTickCount(), nDelta = 0;

	while (TRUE)
	{
		if (gbStopExitWaitForKey)
		{
			// Был вызван HandlerRoutine(CLOSE)
			break;
		}

		// Allow exit by timeout
		if (anMaxTimeout)
		{
			nDelta = GetTickCount() - nStartTick;
			if (nDelta >= anMaxTimeout)
			{
				break;
			}
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
			#ifdef _DEBUG
			GetNumberOfConsoleInputEvents(hIn, &nPreQuit);
			#endif

			// Avoid ConIn overflow, even if (vkKeys == 0)
			if (ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &r, 1, &dwCount)
				&& dwCount
				&& vkKeys)
			{
				bool lbMatch = false;

				if (r.EventType == KEY_EVENT && r.Event.KeyEvent.bKeyDown)
				{
					for (DWORD vk = vkKeys; !lbMatch && LOBYTE(vk); vk=vk>>8)
					{
						lbMatch = (r.Event.KeyEvent.wVirtualKeyCode == LOBYTE(vk));
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


bool isConEmuTerminated()
{
	// HWND of our VirtualConsole
	if (!ghConEmuWndDC)
	{
		_ASSERTE(FALSE && "ghConEmuWndDC is expected to be NOT NULL");
		return false;
	}

	if (::IsWindow(ghConEmuWndDC))
	{
		// ConEmu is alive
		return false;
	}

	//TODO: It would be better to check process presence via connected Pipe
	//TODO: Same as in ConEmu, it must check server presence via pipe
	// For now, don't annoy user with RealConsole if all processes were finished
	if (gbInExitWaitForKey // We are waiting for Enter or Esc
		&& (gpSrv->nProcessCount <= 1) // No active processes are found in console (except our SrvPID)
		)
	{
		// Let RealConsole remain invisible, ConEmu will deal the situation
		return false;
	}

	// ConEmu was killed?
	return true;
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
		CEStr lsRoot;

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
	if (!gpLogSize)
	{
		#ifdef _DEBUG
		if (asText && *asText)
		{
			wchar_t* pszWide = lstrdupW(asText);
			if (pszWide)
			{
				DEBUGSTR(pszWide);
				free(pszWide);
			}
		}
		#endif
		return;
	}

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
	if (!gpLogSize)
	{
		DEBUGSTR(asText);
		return;
	}

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

		_wsprintfA(szInfo, SKIPCOUNT(szInfo) "CurSize={%i,%i,%i} ChangeTo={%i,%i,%i} %s (skipped=%i) {%u:%u:x%X:%u} %s %s",
		           lsbi.dwSize.X, lsbi.srWindow.Bottom-lsbi.srWindow.Top+1, lsbi.dwSize.Y, pcrSize->X, pcrSize->Y, newBufferHeight, (pszLabel ? pszLabel : ""), nSkipped,
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

		_wsprintfA(szInfo, SKIPLEN(countof(szInfo)) "CurSize={%i,%i,%i} %s (skipped=%i) {%u:%u:x%X:%u} %s %s",
		           lsbi.dwSize.X, lsbi.srWindow.Bottom-lsbi.srWindow.Top+1, lsbi.dwSize.Y, (pszLabel ? pszLabel : ""), nSkipped,
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
			lbRc = SetConsoleScreenBufferSize(ghConOut, gcrVisibleSize);
		}
		else
		{
			// ресайз для BufferHeight
			COORD crHeight = {gcrVisibleSize.X, gnBufferHeight};
			MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, 1, 1, 1);
			lbRc = SetConsoleScreenBufferSize(ghConOut, crHeight); // а не crNewSize - там "оконные" размеры
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


WARNING("Use MyGetConsoleScreenBufferInfo instead of GetConsoleScreenBufferInfo");

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
		if (!SetConsoleScreenBufferSize(ghConOut, crNewSize))
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
	if (!SetConsoleScreenBufferSize(ghConOut, crHeight))
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
	// Log it first
	wchar_t szLog[80], szType[20];
	switch (dwCtrlType)
	{
	case CTRL_C_EVENT:
		wcscpy_c(szType, L"CTRL_C_EVENT"); break;
	case CTRL_BREAK_EVENT:
		wcscpy_c(szType, L"CTRL_BREAK_EVENT"); break;
	case CTRL_CLOSE_EVENT:
		wcscpy_c(szType, L"CTRL_CLOSE_EVENT"); break;
	case CTRL_LOGOFF_EVENT:
		wcscpy_c(szType, L"CTRL_LOGOFF_EVENT"); break;
	case CTRL_SHUTDOWN_EVENT:
		wcscpy_c(szType, L"CTRL_SHUTDOWN_EVENT"); break;
	default:
		msprintf(szType, countof(szType), L"ID=%u", dwCtrlType);
	}
	wcscpy_c(szLog, L"  ---  ConsoleCtrlHandler triggered: ");
	wcscat_c(szLog, szType);
	LogString(szLog);

	// Continue processing

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
