
/*
Copyright (c) 2013-2017 Maximus5
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
#define DEBUGSTRCMD(x) DEBUGSTR(x)
#define DEBUGSTRFIN(x) DEBUGSTR(x)
#define DEBUGSTRCP(x) DEBUGSTR(x)

#include "ConEmuC.h"
#pragma warning(disable: 4091)
#include <ShlObj.h>
#pragma warning(default: 4091)
#include "../ConEmu/version.h"
#include "../common/CEStr.h"
#include "../common/MMap.h"
#include "../common/MProcess.h"
#include "../common/MProcessBits.h"
#include "../common/MStrDup.h"
#include "../common/TokenHelper.h"
#include "../common/WFiles.h"
#include "../common/WThreads.h"
#include "ConsoleHelp.h"
#include "UnicodeTest.h"

#if !defined(__GNUC__) || defined(__MINGW32__)
	#pragma warning(disable: 4091)
	#include <dbghelp.h>
	#pragma warning(default: 4091)
#else
	#include "../common/DbgHlpGcc.h"
#endif

#ifndef _T
	#define __T(x) L ## x
	#define _T(x) __T(x)
#endif


DWORD WINAPI DebugThread(LPVOID lpvParam);
void WriteMiniDump(DWORD dwProcessId, DWORD dwThreadId, EXCEPTION_RECORD *pExceptionRecord, LPCSTR asConfirmText = NULL, BOOL bTreeBreak = FALSE);
void GenerateTreeDebugBreak(DWORD nExcludePID);

#define CE_CONEMUC_NAME_W WIN3264TEST(L"ConEmuC[32]",L"ConEmuC[64]")
#define CE_CONEMUC_NAME_A WIN3264TEST("ConEmuC[32]","ConEmuC[64]")
#define CE_TREE_TEMPLATE CE_CONEMUC_NAME_W L": DebuggerPID=%u RootPID=%u Count=%i"


void PrintDebugInfo()
{
	_printf("Debugger successfully attached to PID=%u\n", gpSrv->dwRootProcess);
	TODO("Вывести информацию о загруженных модулях, потоках, и стеке потоков");
}

void UpdateDebuggerTitle()
{
	if (!gpSrv->DbgInfo.bDebugProcessTree)
		return;

	wchar_t szTitle[100];
	LPCWSTR pszTemplate = CE_TREE_TEMPLATE;
	if (GetConsoleTitle(szTitle, sizeof(szTitle)))
	{
		// Заголовок уже установлен, возможно из другого процесса,
		// при одновременном дампе/отладке нескольких процессов разных битностей
		if ((wcsncmp(szTitle, pszTemplate, 8) == 0) && wcsstr(szTitle, L"DebuggerPID"))
			return;
	}

	_wsprintf(szTitle, SKIPLEN(countof(szTitle)) pszTemplate,
		GetCurrentProcessId(), gpSrv->dwRootProcess, gpSrv->DbgInfo.nProcessCount);
	SetTitle(szTitle);
}

int ConfirmDumpType(DWORD dwProcessId, LPCSTR asConfirmText /*= NULL*/)
{
	if (gpSrv->DbgInfo.bAutoDump)
		return 2; // Automatic MINI-dumps

	if (gpSrv->DbgInfo.nDebugDumpProcess == 2 || gpSrv->DbgInfo.nDebugDumpProcess == 3)
		return gpSrv->DbgInfo.nDebugDumpProcess;

	// ANSI is used because of asConfirmText created as ANSI
	char szTitleA[64];
	_wsprintfA(szTitleA, SKIPLEN(countof(szTitleA)) CE_CONEMUC_NAME_A " Debugging PID=%u, Debugger PID=%u", dwProcessId, GetCurrentProcessId());

	int nBtn = MessageBoxA(NULL, asConfirmText ? asConfirmText : "Create minidump (<No> - fulldump)?", szTitleA, MB_YESNOCANCEL|MB_SYSTEMMODAL);

	switch (nBtn)
	{
	case IDYES:
		return 2; // mini dump
	case IDNO:
		return 3; // full dump
	default:
		return 0;
	}
}

int RunDebugger()
{
	if (!gpSrv->DbgInfo.pDebugTreeProcesses)
	{
		gpSrv->DbgInfo.pDebugTreeProcesses = (MMap<DWORD,CEDebugProcessInfo>*)calloc(1,sizeof(*gpSrv->DbgInfo.pDebugTreeProcesses));
		gpSrv->DbgInfo.pDebugTreeProcesses->Init(1024);
	}

	UpdateDebuggerTitle();

	// Increase console buffer size
	{
		HANDLE hCon = ghConOut;
		CONSOLE_SCREEN_BUFFER_INFO csbi = {};
		GetConsoleScreenBufferInfo(hCon, &csbi);
		if (IsWindowVisible(ghConWnd) && (csbi.dwSize.X < 260))
		{
			// Enlarge both width and height
			COORD crNewSize = {260, 32000};
			SetConsoleScreenBufferSize(ghConOut, crNewSize);
		}
		else if (csbi.dwSize.Y < 1000)
		{
			// ConEmu do not support horizontal scrolling yet
			COORD crNewSize = {csbi.dwSize.X, 32000};
			SetConsoleScreenBufferSize(ghConOut, crNewSize);
		}
	}

	// Вывести в консоль информацию о версии.
	PrintVersion();
	#ifdef SHOW_DEBUG_STARTED_MSGBOX
	wchar_t szInfo[128];
	StringCchPrintf(szInfo, countof(szInfo), L"Attaching debugger...\n" CE_CONEMUC_NAME_W " PID = %u\nDebug PID = %u",
	                GetCurrentProcessId(), gpSrv->dwRootProcess);
	MessageBox(GetConEmuHWND(2), szInfo, CE_CONEMUC_NAME_W L".Debugger", 0);
	#endif

	if (gpSrv->DbgInfo.pszDebuggingCmdLine == NULL)
	{
		int iAttachRc = AttachRootProcessHandle();
		if (iAttachRc != 0)
			return iAttachRc;
	}
	else
	{
		_ASSERTE(!gpSrv->DbgInfo.bDebuggerActive);
	}

	gpszRunCmd = (wchar_t*)calloc(1,sizeof(*gpszRunCmd));
	if (!gpszRunCmd)
	{
		_printf("Can't allocate 1 wchar!\n");
		return CERR_NOTENOUGHMEM1;
	}

	_ASSERTE(((gpSrv->hRootProcess!=NULL) || (gpSrv->DbgInfo.pszDebuggingCmdLine!=NULL)) && "Process handle must be opened");

	// Если просили сделать дамп нескольких процессов - нужно сразу уточнить его тип
	if (gpSrv->DbgInfo.bDebugMultiProcess && (gpSrv->DbgInfo.nDebugDumpProcess == 1))
	{
		// 2 - minidump, 3 - fulldump
		int nConfirmDumpType = ConfirmDumpType(gpSrv->dwRootProcess, NULL);
		if (nConfirmDumpType < 2)
		{
			// Отмена
			return CERR_CANTSTARTDEBUGGER;
		}
		gpSrv->DbgInfo.nDebugDumpProcess = nConfirmDumpType;
	}

	// gpSrv->DbgInfo.bDebuggerActive must be set in DebugThread

	if (gpSrv->DbgInfo.bAutoDump)
		gpSrv->DbgInfo.bDebuggerRequestDump = TRUE;

	// Run DebugThread
	gpSrv->DbgInfo.hDebugReady = CreateEvent(NULL, FALSE, FALSE, NULL);

	// All debugger events are processed in special thread
	gpSrv->DbgInfo.hDebugThread = apiCreateThread(DebugThread, NULL, &gpSrv->DbgInfo.dwDebugThreadId, "DebugThread");
	HANDLE hEvents[2] = {gpSrv->DbgInfo.hDebugReady, gpSrv->DbgInfo.hDebugThread};

	// First we must wait for debugger thread initialization finish
	DWORD nReady = WaitForMultipleObjects(countof(hEvents), hEvents, FALSE, INFINITE);
	if (nReady != WAIT_OBJECT_0)
	{
		DWORD nExit = 0;
		GetExitCodeThread(gpSrv->DbgInfo.hDebugThread, &nExit);
		return nExit;
	}

	// gpSrv->DbgInfo.bDebuggerActive was set in DebugThread

	// And wait for debugger thread completion
	_ASSERTE(gnRunMode == RM_UNDEFINED);
	DWORD nDebugThread; // = WaitForSingleObject(gpSrv->DbgInfo.hDebugThread, INFINITE);
	DWORD nDbgTimeout = min(max(25, gpSrv->DbgInfo.nAutoInterval), 100);

	LONG nLastCounter = gpSrv->DbgInfo.nDumpsCounter - 1; // First dump create always
	DWORD nLastTick = GetTickCount();
	while ((nDebugThread = WaitForSingleObject(gpSrv->DbgInfo.hDebugThread, nDbgTimeout)) == WAIT_TIMEOUT)
	{
		TODO("Add console input reader");

		DWORD nDelta = GetTickCount() - nLastTick;
		if (gpSrv->DbgInfo.bAutoDump)
		{
			if ((nDelta + 10) >= gpSrv->DbgInfo.nAutoInterval)
			{
				// Don't request next dump until previous was finished
				if (nLastCounter != gpSrv->DbgInfo.nDumpsCounter)
				{
					nLastCounter = gpSrv->DbgInfo.nDumpsCounter;
					// Request mini-dump
					GenerateMiniDumpFromCtrlBreak();
				}
				// Next step
				nLastTick = GetTickCount();
			}
		}
	}
	_ASSERTE(nDebugThread == WAIT_OBJECT_0); UNREFERENCED_PARAMETER(nDebugThread);

	gbInShutdown = TRUE;
	return 0;
}

HANDLE GetProcessHandleForDebug(DWORD nPID, LPDWORD pnErrCode = NULL)
{
	_ASSERTE(gpSrv->DbgInfo.bDebugProcess);

	HANDLE hProcess = NULL;
	DWORD  nErrCode = 0;

	if ((nPID == gpSrv->dwRootProcess) && gpSrv->hRootProcess)
	{
		hProcess = gpSrv->hRootProcess;
	}
	else
	{
		CEDebugProcessInfo pi = {};

		if (gpSrv->DbgInfo.pDebugTreeProcesses->Get(nPID, &pi) && pi.hProcess)
		{
			// Уже
			hProcess = pi.hProcess;
		}
		else
		{
			// Нужно открыть дескриптор процесса, это запущенный дочерний второго уровня

			DWORD dwFlags = PROCESS_QUERY_INFORMATION|SYNCHRONIZE;

			if (gpSrv->DbgInfo.bDebuggerActive || gpSrv->DbgInfo.bDebugProcessTree)
				dwFlags |= PROCESS_VM_READ;

			CAdjustProcessToken token;
			token.Enable(1, SE_DEBUG_NAME);

			// Сначала - пробуем "все права"
			// PROCESS_ALL_ACCESS (defined in new SDK) may fail on WinXP!
			hProcess = OpenProcess(MY_PROCESS_ALL_ACCESS, FALSE, nPID);
			if (!hProcess)
			{
				DEBUGTEST(nErrCode = GetLastError());

				hProcess = OpenProcess(dwFlags, FALSE, nPID);

				if (!hProcess)
				{
					nErrCode = GetLastError();
				}
			}

			token.Release();

			if (nPID != gpSrv->dwRootProcess)
			{
				// Запомнить дескриптор
				_ASSERTE(pi.nPID == nPID || pi.nPID == 0); // уже должен быть
				pi.nPID = nPID;
				pi.hProcess = hProcess;
				gpSrv->DbgInfo.pDebugTreeProcesses->Set(nPID, pi);
			}
		}
	}

	if (pnErrCode)
		*pnErrCode = nErrCode;

	return hProcess;
}

// Используется и в обычном, и в "отладочном" режиме
int AttachRootProcessHandle()
{
	if (gpSrv->DbgInfo.pszDebuggingCmdLine != NULL)
	{
		_ASSERTE(gpSrv->DbgInfo.bDebuggerActive);
		return 0; // Started from DebuggingThread
	}

	DWORD dwErr = 0;
	// Нужно открыть HANDLE корневого процесса
	_ASSERTE(gpSrv->hRootProcess==NULL || gpSrv->hRootProcess==GetCurrentProcess());
	if (gpSrv->dwRootProcess == GetCurrentProcessId())
	{
		if (gpSrv->hRootProcess == NULL)
		{
			gpSrv->hRootProcess = GetCurrentProcess();
		}
	}
	else if (gpSrv->DbgInfo.bDebuggerActive)
	{
		if (gpSrv->hRootProcess == NULL)
		{
			gpSrv->hRootProcess = GetProcessHandleForDebug(gpSrv->dwRootProcess);
		}
	}
	else if (gpSrv->hRootProcess == NULL)
	{
		gpSrv->hRootProcess = OpenProcess(MY_PROCESS_ALL_ACCESS, FALSE, gpSrv->dwRootProcess);
		if (gpSrv->hRootProcess == NULL)
		{
			gpSrv->hRootProcess = OpenProcess(SYNCHRONIZE|PROCESS_QUERY_INFORMATION, FALSE, gpSrv->dwRootProcess);
		}
	}

	if (!gpSrv->hRootProcess)
	{
		dwErr = GetLastError();
		wchar_t* lpMsgBuf = NULL;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&lpMsgBuf, 0, NULL);
		_printf("\nCan't open process (%i) handle, ErrCode=0x%08X, Description:\n", //-V576
		        gpSrv->dwRootProcess, dwErr, (lpMsgBuf == NULL) ? L"<Unknown error>" : lpMsgBuf);

		if (lpMsgBuf) LocalFree(lpMsgBuf);
		SetLastError(dwErr);

		return CERR_CREATEPROCESS;
	}

	if (gpSrv->DbgInfo.bDebuggerActive)
	{
		wchar_t szTitle[64];
		_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"Debugging PID=%u, Debugger PID=%u", gpSrv->dwRootProcess, GetCurrentProcessId());
		SetTitle(szTitle);

		UpdateDebuggerTitle();
	}

	return 0;
}

void AttachConHost(DWORD nConHostPID)
{
	DWORD  nErrCode = 0;
	HANDLE hConHost = GetProcessHandleForDebug(nConHostPID, &nErrCode);

	if (!hConHost)
	{
		_printf("Opening ConHost process handle failed, Code=%u\n", nErrCode);
	}
	else if (!DebugActiveProcess(nConHostPID))
	{
		nErrCode = GetLastError();
		_printf("Attaching to ConHost process handle failed, Code=%u\n", nErrCode);
	}

	if (!nErrCode)
	{
		//TODO: Запустить ConEmuC (требуемой битности) под админом
	}
}

DWORD WINAPI DebugThread(LPVOID lpvParam)
{
	DWORD nWait = WAIT_TIMEOUT;
	wchar_t szInfo[1024];
	wchar_t szPID[20];
	int iAttachedCount = 0;
	CEStr szOtherBitPids, szOtherDebugCmd;

	// Дополнительная инициализация, чтобы закрытие дебагера (наш процесс) не привело
	// к закрытию "отлаживаемой" программы
	pfnDebugActiveProcessStop = (FDebugActiveProcessStop)GetProcAddress(GetModuleHandle(L"kernel32.dll"),"DebugActiveProcessStop");
	pfnDebugSetProcessKillOnExit = (FDebugSetProcessKillOnExit)GetProcAddress(GetModuleHandle(L"kernel32.dll"),"DebugSetProcessKillOnExit");

	// Affect GetProcessHandleForDebug
	gpSrv->DbgInfo.bDebuggerActive = TRUE;

	// If dump was requested
	if (gpSrv->DbgInfo.nDebugDumpProcess)
	{
		gpSrv->DbgInfo.bUserRequestDump = TRUE;
		gpSrv->DbgInfo.bDebuggerRequestDump = TRUE;
	}

	// "/DEBUGEXE" or "/DEBUGTREE"
	if (gpSrv->DbgInfo.pszDebuggingCmdLine != NULL)
	{
		STARTUPINFO si = {sizeof(si)};
		PROCESS_INFORMATION pi = {};

		if (gpSrv->DbgInfo.bDebugProcessTree)
		{
			SetEnvironmentVariable(ENV_CONEMU_BLOCKCHILDDEBUGGERS_W, ENV_CONEMU_BLOCKCHILDDEBUGGERS_YES);
		}

		if (!CreateProcess(NULL, gpSrv->DbgInfo.pszDebuggingCmdLine, NULL, NULL, FALSE,
			NORMAL_PRIORITY_CLASS|CREATE_NEW_CONSOLE|
			DEBUG_PROCESS | (gpSrv->DbgInfo.bDebugProcessTree ? 0 : DEBUG_ONLY_THIS_PROCESS),
			NULL, NULL, &si, &pi))
		{
			DWORD dwErr = GetLastError();

			wchar_t szProc[64]; szProc[0] = 0;
			PROCESSENTRY32 pi = {sizeof(pi)};
			if (GetProcessInfo(gpSrv->dwRootProcess, &pi))
				_wcscpyn_c(szProc, countof(szProc), pi.szExeFile, countof(szProc));

			_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"Can't start debugging process. ErrCode=0x%08X\n", dwErr);
			CEStr lsInfo(lstrmerge(szInfo, gpSrv->DbgInfo.pszDebuggingCmdLine, L"\n"));
			_wprintf(lsInfo);
			return CERR_CANTSTARTDEBUGGER;
		}

		gpSrv->hRootProcess = pi.hProcess;
		gpSrv->hRootThread = pi.hThread;
		gpSrv->dwRootProcess = pi.dwProcessId;
		gpSrv->dwRootThread = pi.dwThreadId;
		gpSrv->dwRootStartTime = GetTickCount();

		// Let's know that at least one process is debugging
		iAttachedCount++;
	}


	/* ************************* */
	int iDbgIdx = 0;
	bool bSetKillOnExit = true;

	while (true)
	{
		HANDLE hDbgProcess = NULL;
		DWORD  nDbgProcessID = 0;

		if ((iDbgIdx++) == 0)
		{
			hDbgProcess = gpSrv->hRootProcess;
			nDbgProcessID = gpSrv->dwRootProcess;
		}
		else
		{
			// Взять из pDebugAttachProcesses
			if (!gpSrv->DbgInfo.pDebugAttachProcesses)
				break;
			if (!gpSrv->DbgInfo.pDebugAttachProcesses->pop_back(nDbgProcessID))
				break;
			hDbgProcess = GetProcessHandleForDebug(nDbgProcessID);
			if (!hDbgProcess)
			{
				_ASSERTE(hDbgProcess!=NULL && "Can't open debugging process handle");
				continue;
			}
		}


		_ASSERTE(hDbgProcess!=NULL && "Process handle must be opened");


		// Битность отладчика должна соответствовать битности приложения!
		if (IsWindows64())
		{
			int nBits = GetProcessBits(nDbgProcessID, hDbgProcess);
			if ((nBits == 32 || nBits == 64) && (nBits != WIN3264TEST(32,64)))
			{
				// If /DEBUGEXE or /DEBUGTREE was used
				if (gpSrv->DbgInfo.pszDebuggingCmdLine != NULL)
				{
					_printf("Bitness of ConEmuC and debugging program does not match\n");
					continue;
				}

				// Добавить процесс в список для запуска альтернативного дебаггера соотвествующей битности
				// Force trailing "," even if only one PID specified ( --> bDebugMultiProcess = TRUE)
				lstrmerge(&szOtherBitPids.ms_Val, _itow(nDbgProcessID, szPID, 10), L",");

				// Может там еще процессы в списке на дамп?
				continue;
			}
		}

		if (gpSrv->DbgInfo.pszDebuggingCmdLine == NULL)
		{
			if (DebugActiveProcess(nDbgProcessID))
			{
				iAttachedCount++;
			}
			else
			{
				DWORD dwErr = GetLastError();

				wchar_t szProc[64]; szProc[0] = 0;
				PROCESSENTRY32 pi = {sizeof(pi)};
				if (GetProcessInfo(nDbgProcessID, &pi))
					_wcscpyn_c(szProc, countof(szProc), pi.szExeFile, countof(szProc));

				_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"Can't attach debugger to '%s' PID=%i. ErrCode=0x%08X\n",
					szProc[0] ? szProc : L"not found", nDbgProcessID, dwErr);
				_wprintf(szInfo);

				// Может другие подцепить получится?
				continue;
			}
		}

		// To avoid debugged processes killing
		if (bSetKillOnExit && pfnDebugSetProcessKillOnExit)
		{
			// affects all current and future debuggees connected to the calling thread
			if (pfnDebugSetProcessKillOnExit(FALSE/*KillOnExit*/))
			{
				bSetKillOnExit = false;
			}
		}
	}

	// Different bitness, need to start appropriate debugger
	if (szOtherBitPids.ms_Val && *szOtherBitPids.ms_Val)
	{
		wchar_t szExe[MAX_PATH+5], *pszName;
		if (!GetModuleFileName(NULL, szExe, MAX_PATH))
		{
			_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"GetModuleFileName(NULL) failed. ErrCode=0x%08X\n", GetLastError());
			_wprintf(szInfo);
		}
		else if (!(pszName = (wchar_t*)PointToName(szExe)))
		{
			_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"GetModuleFileName(NULL) returns invalid path\n%s\n", szExe);
			_wprintf(szInfo);
		}
		else
		{
			*pszName = 0;
			// Reverted to current bitness
			wcscat_c(szExe, WIN3264TEST(L"ConEmuC64.exe", L"ConEmuC.exe"));

			szOtherDebugCmd.Attach(lstrmerge(L"\"", szExe, L"\" "
				L"/DEBUGPID=", szOtherBitPids.ms_Val,
				(gpSrv->DbgInfo.nDebugDumpProcess == 1) ? L" /DUMP" :
				(gpSrv->DbgInfo.nDebugDumpProcess == 2) ? L" /MINIDUMP" :
				(gpSrv->DbgInfo.nDebugDumpProcess == 3) ? L" /FULLDUMP" : L""));

			STARTUPINFO si = {sizeof(si)};
			PROCESS_INFORMATION pi = {};
			if (CreateProcess(NULL, szOtherDebugCmd.ms_Val, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
			{
				// Ждать не будем
			}
			else
			{
				DWORD dwErr = GetLastError();
				_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"Can't start external debugger, ErrCode=0x%08X\n", dwErr);
				CEStr lsInfo(lstrmerge(szInfo, szOtherDebugCmd, L"\n"));
				_wprintf(lsInfo);
			}
		}
	}

	//_ASSERTE(FALSE && "Continue to dump");

	// If neither /DEBUG[EXE|TREE] nor /DEBUGPID was not succeeded
	if (iAttachedCount == 0)
	{
		gpSrv->DbgInfo.bDebuggerActive = FALSE;
		return CERR_CANTSTARTDEBUGGER;
	}
	else if (iAttachedCount > 1)
	{
		_ASSERTE(gpSrv->DbgInfo.bDebugMultiProcess && "Already must be set from arguments parser");
		gpSrv->DbgInfo.bDebugMultiProcess = TRUE;
	}

	if (gpSrv->DbgInfo.bUserRequestDump)
	{
		gpSrv->DbgInfo.nWaitTreeBreaks = iAttachedCount;
	}

	/* **************** */

	// To avoid debugged processes killing (JIC, must be called already)
	if (bSetKillOnExit && pfnDebugSetProcessKillOnExit)
	{
		// affects all current and future debuggees connected to the calling thread
		if (pfnDebugSetProcessKillOnExit(FALSE/*KillOnExit*/))
		{
			bSetKillOnExit = false;
		}
	}

	PrintDebugInfo();
	SetEvent(gpSrv->DbgInfo.hDebugReady);


	while (nWait == WAIT_TIMEOUT)
	{
		ProcessDebugEvent();

		if (ghExitQueryEvent)
			nWait = WaitForSingleObject(ghExitQueryEvent, 0);
	}

//done:
	gbRootAliveLess10sec = FALSE;
	gbInShutdown = TRUE;
	gbAlwaysConfirmExit = FALSE;

	_ASSERTE(gbTerminateOnCtrlBreak==FALSE);

	if (!nExitQueryPlace) nExitQueryPlace = 12+(nExitPlaceStep);

	SetTerminateEvent(ste_DebugThread);
	return 0;
}

bool IsDumpMulti()
{
	if (gpSrv->DbgInfo.bDebugProcessTree || gpSrv->DbgInfo.bDebugMultiProcess)
		return true;
	return false;
}

wchar_t* FormatDumpName(wchar_t* DmpFile, size_t cchDmpMax, DWORD dwProcessId, bool bTrap, bool bFull)
{
	//TODO: Добавить в DmpFile имя без пути? <exename>-<ver>-<pid>-<yymmddhhmmss>.[m]dmp
	wchar_t szMinor[8] = L""; lstrcpyn(szMinor, _T(MVV_4a), countof(szMinor));
	SYSTEMTIME lt = {}; GetLocalTime(&lt);
	_wsprintf(DmpFile, SKIPLEN(cchDmpMax) L"%s-%02u%02u%02u%s-%u-%02u%02u%02u%02u%02u%02u%03u.%s",
		bTrap ? L"Trap" : L"CEDump",
		MVV_1, MVV_2, MVV_3, szMinor, dwProcessId,
		lt.wYear%100, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute, lt.wSecond, lt.wMilliseconds,
		bFull ? L"dmp" : L"mdmp");
	return DmpFile;
}

bool GetSaveDumpName(DWORD dwProcessId, bool bFull, wchar_t* dmpfile, DWORD cchMaxDmpFile)
{
	bool bRc = false;

	HMODULE hCOMDLG32 = NULL;
	typedef BOOL (WINAPI* GetSaveFileName_t)(LPOPENFILENAMEW lpofn);
	GetSaveFileName_t _GetSaveFileName = NULL;
	bool bDumpMulti = IsDumpMulti() || gpSrv->DbgInfo.bAutoDump;

	if (!bDumpMulti)
	{
		if (!hCOMDLG32)
			hCOMDLG32 = LoadLibraryW(L"COMDLG32.dll");
		if (hCOMDLG32 && !_GetSaveFileName)
			_GetSaveFileName = (GetSaveFileName_t)GetProcAddress(hCOMDLG32, "GetSaveFileNameW");

		if (_GetSaveFileName)
		{
			OPENFILENAMEW ofn; memset(&ofn,0,sizeof(ofn));
			ofn.lStructSize=sizeof(ofn);
			ofn.hwndOwner = NULL;
			ofn.lpstrFilter = L"Debug dumps (*.mdmp)\0*.mdmp;*.dmp\0Debug dumps (*.dmp)\0*.dmp;*.mdmp\0\0";
			ofn.nFilterIndex = bFull ? 2 : 1;
			ofn.lpstrFile = dmpfile;
			ofn.nMaxFile = cchMaxDmpFile;
			ofn.lpstrTitle = bFull ? L"Save debug full-dump" : L"Save debug mini-dump";
			ofn.lpstrDefExt = bFull ? L"dmp" : L"mdmp";
			ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
			            | OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;

			if (_GetSaveFileName(&ofn))
			{
				bRc = true;
			}
		}

		if (hCOMDLG32)
		{
			FreeLibrary(hCOMDLG32);
		}
	}

	if (bDumpMulti || !_GetSaveFileName)
	{
		HRESULT dwErr = SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0/*SHGFP_TYPE_CURRENT*/, dmpfile);
		if (FAILED(dwErr))
		{
			memset(dmpfile, 0, cchMaxDmpFile*sizeof(*dmpfile));
			if (GetTempPath(cchMaxDmpFile-32, dmpfile) && *dmpfile)
				dwErr = S_OK;
		}

		if (FAILED(dwErr))
		{
			_printf("\nGetSaveDumpName called, get desktop folder failed, code=%u\n", (DWORD)dwErr);
		}
		else
		{
			if (*dmpfile && dmpfile[lstrlen(dmpfile)-1] != L'\\')
				_wcscat_c(dmpfile, cchMaxDmpFile, L"\\");

			_wcscat_c(dmpfile, cchMaxDmpFile, L"ConEmuTrap");
			CreateDirectory(dmpfile, NULL);

			INT_PTR nLen = lstrlen(dmpfile);
			dmpfile[nLen++] = L'\\'; dmpfile[nLen] = 0;
			FormatDumpName(dmpfile+nLen, cchMaxDmpFile-nLen, dwProcessId, (gpSrv->DbgInfo.bDebuggerRequestDump!=FALSE), bFull);

			bRc = true;
		}
	}

	return bRc;
}

// gpSrv->dwRootProcess
void WriteMiniDump(DWORD dwProcessId, DWORD dwThreadId, EXCEPTION_RECORD *pExceptionRecord, LPCSTR asConfirmText /*= NULL*/, BOOL bTreeBreak /*= FALSE*/)
{
	// 2 - minidump, 3 - fulldump
	int nConfirmDumpType = ConfirmDumpType(dwProcessId, asConfirmText);
	if (nConfirmDumpType < 2)
	{
		// Отмена
		return;
	}

	MINIDUMP_TYPE dumpType = (nConfirmDumpType == 2) ? MiniDumpNormal : MiniDumpWithFullMemory;

	// Т.к. в режиме "ProcessTree" мы пишем пачку дампов - спрашивать тип дампа будем один раз.
	if (IsDumpMulti() // several processes were attached
		&& (gpSrv->DbgInfo.nDebugDumpProcess <= 1)) // 2 - minidump, 3 - fulldump
	{
		gpSrv->DbgInfo.nDebugDumpProcess = nConfirmDumpType;
	}

	if (bTreeBreak)
	{
		GenerateTreeDebugBreak(dwProcessId);
	}

	bool bDumpSucceeded = false;
	HANDLE hDmpFile = NULL;
	//HMODULE hDbghelp = NULL;
	wchar_t szErrInfo[MAX_PATH*2];

	wchar_t szTitle[64];
	_wsprintf(szTitle, SKIPLEN(countof(szTitle)) CE_CONEMUC_NAME_W L" Debugging PID=%u, Debugger PID=%u", dwProcessId, GetCurrentProcessId());

	wchar_t dmpfile[MAX_PATH] = L""; dmpfile[0] = 0;
	FormatDumpName(dmpfile, countof(dmpfile), dwProcessId, false, (dumpType == MiniDumpWithFullMemory));

	typedef BOOL (WINAPI* MiniDumpWriteDump_t)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType,
	        PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	        PMINIDUMP_CALLBACK_INFORMATION CallbackParam);
	MiniDumpWriteDump_t MiniDumpWriteDump_f = NULL;

	while (GetSaveDumpName(dwProcessId, (dumpType == MiniDumpWithFullMemory), dmpfile, countof(dmpfile)))
	{
		if (hDmpFile != INVALID_HANDLE_VALUE && hDmpFile != NULL)
		{
			CloseHandle(hDmpFile); hDmpFile = NULL;
		}

		hDmpFile = CreateFileW(dmpfile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_WRITE_THROUGH, NULL);

		if (hDmpFile == INVALID_HANDLE_VALUE)
		{
			DWORD nErr = GetLastError();
			_wsprintf(szErrInfo, SKIPLEN(countof(szErrInfo)) L"Can't create debug dump file\n%s\nErrCode=0x%08X\n\nChoose another name?", dmpfile, nErr);

			if (MessageBoxW(NULL, szErrInfo, szTitle, MB_YESNO|MB_SYSTEMMODAL|MB_ICONSTOP)!=IDYES)
				break;

			continue; // еще раз выбрать
		}

		if (!gpSrv->DbgInfo.hDbghelp)
		{
			gpSrv->DbgInfo.hDbghelp = LoadLibraryW(L"Dbghelp.dll");

			if (gpSrv->DbgInfo.hDbghelp == NULL)
			{
				DWORD nErr = GetLastError();
				_wsprintf(szErrInfo, SKIPLEN(countof(szErrInfo)) L"Can't load debug library 'Dbghelp.dll'\nErrCode=0x%08X\n\nTry again?", nErr);

				if (MessageBoxW(NULL, szErrInfo, szTitle, MB_YESNO|MB_SYSTEMMODAL|MB_ICONSTOP)!=IDYES)
					break;

				continue; // еще раз выбрать
			}
		}

		if (gpSrv->DbgInfo.MiniDumpWriteDump_f)
		{
			MiniDumpWriteDump_f = (MiniDumpWriteDump_t)gpSrv->DbgInfo.MiniDumpWriteDump_f;
		}
		else if (!MiniDumpWriteDump_f)
		{
			MiniDumpWriteDump_f = (MiniDumpWriteDump_t)GetProcAddress(gpSrv->DbgInfo.hDbghelp, "MiniDumpWriteDump");

			if (!MiniDumpWriteDump_f)
			{
				DWORD nErr = GetLastError();
				_wsprintf(szErrInfo, SKIPLEN(countof(szErrInfo)) L"Can't locate 'MiniDumpWriteDump' in library 'Dbghelp.dll', ErrCode=%u", nErr);
				MessageBoxW(NULL, szErrInfo, szTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
				break;
			}

			gpSrv->DbgInfo.MiniDumpWriteDump_f = (FARPROC)MiniDumpWriteDump_f;
		}

		if (MiniDumpWriteDump_f)
		{
			MINIDUMP_EXCEPTION_INFORMATION mei = {dwThreadId};
			EXCEPTION_POINTERS ep = {pExceptionRecord};
			ep.ContextRecord = NULL; // Непонятно, откуда его можно взять
			mei.ExceptionPointers = &ep;
			mei.ClientPointers = FALSE;
			PMINIDUMP_EXCEPTION_INFORMATION pmei = NULL; // пока
			_printf("Creating minidump: ");
			_wprintf(dmpfile);
			_printf("...");

			HANDLE hProcess = GetProcessHandleForDebug(dwProcessId);

			BOOL lbDumpRc = MiniDumpWriteDump_f(
			                    hProcess, dwProcessId,
			                    hDmpFile,
			                    dumpType,
			                    pmei,
			                    NULL, NULL);

			if (!lbDumpRc)
			{
				DWORD nErr = GetLastError();
				_wsprintf(szErrInfo, SKIPLEN(countof(szErrInfo)) L"MiniDumpWriteDump failed.\nErrorCode=0x%08X", nErr);
				_printf("\nFailed, ErrorCode=0x%08X\n", nErr);
				MessageBoxW(NULL, szErrInfo, szTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
			}
			else
			{
				int iLeft = (gpSrv->DbgInfo.nWaitTreeBreaks > 0) ? (gpSrv->DbgInfo.nWaitTreeBreaks - 1) : 0;
				_wsprintf(szErrInfo, SKIPLEN(countof(szErrInfo)) L"\nMiniDumpWriteDump succeeded, %i left\n", iLeft);
				bDumpSucceeded = true;
			}

			break;
		}

	} // end while (GetSaveDumpName(dwProcessId, (dumpType == MiniDumpWithFullMemory), dmpfile, countof(dmpfile)))

	if (hDmpFile != INVALID_HANDLE_VALUE && hDmpFile != NULL)
	{
		CloseHandle(hDmpFile);
	}


	// В Win2k еще не было функции "отцепиться от процесса"
	if ((gnOsVer >= 0x0501)
		&& bDumpSucceeded && gpSrv->DbgInfo.bUserRequestDump
		// И все дампы были созданы
		&& (gpSrv->DbgInfo.nWaitTreeBreaks <= 1)
		// И это не ключи /DEBUGEXE или /DEBUGTREE
		&& !gpSrv->DbgInfo.pszDebuggingCmdLine
		)
	{
		// По завершении создания дампов - выйти
		SetTerminateEvent(ste_WriteMiniDump);
	}
}

void ProcessDebugEvent()
{
	static wchar_t wszDbgText[1024];
	static char szDbgText[1024];
	BOOL lbNonContinuable = FALSE;
	DEBUG_EVENT evt = {0};
	BOOL lbEvent = WaitForDebugEvent(&evt,10);
	#ifdef _DEBUG
	DWORD dwErr = GetLastError();
	#endif
	static bool bFirstExitThreadEvent = false; // Чтобы вывести на экран подсказку по возможностям "дебаггера"
	//HMODULE hCOMDLG32 = NULL;
	//typedef BOOL (WINAPI* GetSaveFileName_t)(LPOPENFILENAMEW lpofn);
	//GetSaveFileName_t _GetSaveFileName = NULL;
	DWORD dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;


	if (lbEvent)
	{
		lbNonContinuable = FALSE;

		switch (evt.dwDebugEventCode)
		{
			case CREATE_PROCESS_DEBUG_EVENT:
			case CREATE_THREAD_DEBUG_EVENT:
			case EXIT_PROCESS_DEBUG_EVENT:
			case EXIT_THREAD_DEBUG_EVENT:
			case RIP_EVENT:
			{
				LPCSTR pszName = "Unknown";

				switch (evt.dwDebugEventCode)
				{
					case CREATE_PROCESS_DEBUG_EVENT: pszName = "CREATE_PROCESS_DEBUG_EVENT"; break;
					case CREATE_THREAD_DEBUG_EVENT: pszName = "CREATE_THREAD_DEBUG_EVENT"; break;
					case EXIT_PROCESS_DEBUG_EVENT: pszName = "EXIT_PROCESS_DEBUG_EVENT"; break;
					case EXIT_THREAD_DEBUG_EVENT: pszName = "EXIT_THREAD_DEBUG_EVENT"; break;
					case RIP_EVENT: pszName = "RIP_EVENT"; break;
				}

				_wsprintfA(szDbgText, SKIPLEN(countof(szDbgText)) "{%i.%i} %s\n", evt.dwProcessId,evt.dwThreadId, pszName);
				_printf(szDbgText);
				if (!bFirstExitThreadEvent && evt.dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT)
				{
					bFirstExitThreadEvent = true;
					if (gpSrv->DbgInfo.nDebugDumpProcess == 0)
					{
						_printf(CE_CONEMUC_NAME_A ": Press Ctrl+Break to create minidump of debugging process\n");
					}
					//else // -- wrong way, HandlerRoutine will/may fail on first attach
					//{
					//	// Сразу сделать дамп и выйти
					//	HandlerRoutine(CTRL_BREAK_EVENT);
					//}
				}

				if (evt.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
				{
					gpSrv->DbgInfo.nProcessCount++;

					_ASSERTE(gpSrv->DbgInfo.pDebugTreeProcesses!=NULL);
					CEDebugProcessInfo pi = {evt.dwProcessId};
					gpSrv->DbgInfo.pDebugTreeProcesses->Set(evt.dwProcessId, pi);

					UpdateDebuggerTitle();
				}
				else if (evt.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
				{
					CEDebugProcessInfo pi = {};
					if (gpSrv->DbgInfo.pDebugTreeProcesses
						&& gpSrv->DbgInfo.pDebugTreeProcesses->Get(evt.dwProcessId, &pi, true)
						&& pi.hProcess)
					{
						CloseHandle(pi.hProcess);
					}

					if (gpSrv->DbgInfo.nProcessCount > 0)
						gpSrv->DbgInfo.nProcessCount--;

					UpdateDebuggerTitle();
				}

				break;
			}

			case LOAD_DLL_DEBUG_EVENT:
			case UNLOAD_DLL_DEBUG_EVENT:
			{
				LPCSTR pszName = "Unknown";
				char szBase[32] = {};
				char szFile[MAX_PATH+128] = {};

				struct MY_FILE_NAME_INFO
				{
					DWORD FileNameLength;
					WCHAR FileName[1];
				};
				typedef BOOL (WINAPI* GetFileInformationByHandleEx_t)(HANDLE hFile, int FileInformationClass, LPVOID lpFileInformation, DWORD dwBufferSize);
				static GetFileInformationByHandleEx_t _GetFileInformationByHandleEx = NULL;

				switch (evt.dwDebugEventCode)
				{
					case LOAD_DLL_DEBUG_EVENT:
						//6 Reports a load-dynamic-link-library (DLL) debugging event. The value of u.LoadDll specifies a LOAD_DLL_DEBUG_INFO structure.
						pszName = "LOAD_DLL_DEBUG_EVENT";

						if (evt.u.LoadDll.hFile)
						{
							if (gnOsVer >= 0x0600)
							{
								if (!_GetFileInformationByHandleEx)
									_GetFileInformationByHandleEx = (GetFileInformationByHandleEx_t)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "GetFileInformationByHandleEx");

								if (_GetFileInformationByHandleEx)
								{
									DWORD nSize = sizeof(MY_FILE_NAME_INFO)+MAX_PATH*sizeof(wchar_t);
									MY_FILE_NAME_INFO* pfi = (MY_FILE_NAME_INFO*)calloc(nSize+2,1);
									if (pfi)
									{
										pfi->FileNameLength = MAX_PATH;
										if (_GetFileInformationByHandleEx(evt.u.LoadDll.hFile, 2/*FileNameInfo*/, pfi, nSize)
											&& pfi->FileName[0])
										{
											wchar_t szFullPath[MAX_PATH+1] = {}, *pszFile;
											DWORD n = GetFullPathName(pfi->FileName, countof(szFullPath), szFullPath, &pszFile);
											if (!n || (n >= countof(szFullPath)))
											{
												lstrcpyn(szFullPath, pfi->FileName, countof(szFullPath));
												pszFile = (wchar_t*)PointToName(pfi->FileName);
											}
											else if (!pszFile)
											{
												pszFile = (wchar_t*)PointToName(szFullPath);
											}
											lstrcpyA(szFile, ", ");
											WideCharToMultiByte(CP_OEMCP, 0, pszFile, -1, szFile+lstrlenA(szFile), 80, 0,0);
											lstrcatA(szFile, "\n\t");
											WideCharToMultiByte(CP_OEMCP, 0, szFullPath, -1, szFile+lstrlenA(szFile), MAX_PATH, 0,0);
										}
										free(pfi);
									}
								}
							}
							CloseHandle(evt.u.LoadDll.hFile);
						}
						_wsprintfA(szBase, SKIPLEN(countof(szBase))
						           " at " WIN3264TEST("0x%08X","0x%08X%08X"),
						           WIN3264WSPRINT((DWORD_PTR)evt.u.LoadDll.lpBaseOfDll));

						break;
					case UNLOAD_DLL_DEBUG_EVENT:
						//7 Reports an unload-DLL debugging event. The value of u.UnloadDll specifies an UNLOAD_DLL_DEBUG_INFO structure.
						pszName = "UNLOAD_DLL_DEBUG_EVENT";
						_wsprintfA(szBase, SKIPLEN(countof(szBase))
						           " at " WIN3264TEST("0x%08X","0x%08X%08X"),
						           WIN3264WSPRINT((DWORD_PTR)evt.u.UnloadDll.lpBaseOfDll));
						break;
				}

				_wsprintfA(szDbgText, SKIPLEN(countof(szDbgText)) "{%i.%i} %s%s%s\n", evt.dwProcessId,evt.dwThreadId, pszName, szBase, szFile);
				_printf(szDbgText);
				break;
			} // LOAD_DLL_DEBUG_EVENT, UNLOAD_DLL_DEBUG_EVENT

			case EXCEPTION_DEBUG_EVENT:
				//1 Reports an exception debugging event. The value of u.Exception specifies an EXCEPTION_DEBUG_INFO structure.
			{
				lbNonContinuable = (evt.u.Exception.ExceptionRecord.ExceptionFlags&EXCEPTION_NONCONTINUABLE)==EXCEPTION_NONCONTINUABLE;

				switch (evt.u.Exception.ExceptionRecord.ExceptionCode)
				{
					case EXCEPTION_ACCESS_VIOLATION: // The thread tried to read from or write to a virtual address for which it does not have the appropriate access.
					{
						if (evt.u.Exception.ExceptionRecord.NumberParameters>=2)
						{
							_wsprintfA(szDbgText, SKIPLEN(countof(szDbgText))
							           "{%i.%i} EXCEPTION_ACCESS_VIOLATION at " WIN3264TEST("0x%08X","0x%08X%08X") " flags 0x%08X%s %s of " WIN3264TEST("0x%08X","0x%08X%08X") " FC=%u\n", evt.dwProcessId,evt.dwThreadId,
							           WIN3264WSPRINT((DWORD_PTR)evt.u.Exception.ExceptionRecord.ExceptionAddress),
							           evt.u.Exception.ExceptionRecord.ExceptionFlags,
							           ((evt.u.Exception.ExceptionRecord.ExceptionFlags&EXCEPTION_NONCONTINUABLE) ? "(EXCEPTION_NONCONTINUABLE)" : ""),
							           ((evt.u.Exception.ExceptionRecord.ExceptionInformation[0]==0) ? "Read" :
							            (evt.u.Exception.ExceptionRecord.ExceptionInformation[0]==1) ? "Write" :
							            (evt.u.Exception.ExceptionRecord.ExceptionInformation[0]==8) ? "DEP" : "???"),
							           WIN3264WSPRINT(evt.u.Exception.ExceptionRecord.ExceptionInformation[1]),
							           evt.u.Exception.dwFirstChance
							          );
						}
						else
						{
							_wsprintfA(szDbgText, SKIPLEN(countof(szDbgText))
							           "{%i.%i} EXCEPTION_ACCESS_VIOLATION at " WIN3264TEST("0x%08X","0x%08X%08X") " flags 0x%08X%s FC=%u\n", evt.dwProcessId,evt.dwThreadId,
							           WIN3264WSPRINT((DWORD_PTR)evt.u.Exception.ExceptionRecord.ExceptionAddress),
							           evt.u.Exception.ExceptionRecord.ExceptionFlags,
							           (evt.u.Exception.ExceptionRecord.ExceptionFlags&EXCEPTION_NONCONTINUABLE) ? "(EXCEPTION_NONCONTINUABLE)" : "",
							           evt.u.Exception.dwFirstChance);
						}

						_printf(szDbgText);
					} // EXCEPTION_ACCESS_VIOLATION
					break;
					case MS_VC_THREADNAME_EXCEPTION:
					{
						_wsprintfA(szDbgText, SKIPLEN(countof(szDbgText))
							"{%i.%i} %s at " WIN3264TEST("0x%08X", "0x%08X%08X") " flags 0x%08X%s FC=%u\n",
							evt.dwProcessId, evt.dwThreadId,
							"MS_VC_THREADNAME_EXCEPTION",
							WIN3264WSPRINT((DWORD_PTR)evt.u.Exception.ExceptionRecord.ExceptionAddress),
							evt.u.Exception.ExceptionRecord.ExceptionFlags,
							(evt.u.Exception.ExceptionRecord.ExceptionFlags&EXCEPTION_NONCONTINUABLE)
							? "(EXCEPTION_NONCONTINUABLE)" : "",
							evt.u.Exception.dwFirstChance);
						_printf(szDbgText);
					} // MS_VC_THREADNAME_EXCEPTION
					break;
					default:
					{
						char szName[32]; LPCSTR pszName; pszName = szName;
						#define EXCASE(s) case s: pszName = #s; break

						switch(evt.u.Exception.ExceptionRecord.ExceptionCode)
						{
								EXCASE(EXCEPTION_ARRAY_BOUNDS_EXCEEDED); // The thread tried to access an array element that is out of bounds and the underlying hardware supports bounds checking.
								EXCASE(EXCEPTION_BREAKPOINT); // A breakpoint was encountered.
								EXCASE(EXCEPTION_DATATYPE_MISALIGNMENT); // The thread tried to read or write data that is misaligned on hardware that does not provide alignment. For example, 16-bit values must be aligned on 2-byte boundaries; 32-bit values on 4-byte boundaries, and so on.
								EXCASE(EXCEPTION_FLT_DENORMAL_OPERAND); // One of the operands in a floating-point operation is denormal. A denormal value is one that is too small to represent as a standard floating-point value.
								EXCASE(EXCEPTION_FLT_DIVIDE_BY_ZERO); // The thread tried to divide a floating-point value by a floating-point divisor of zero.
								EXCASE(EXCEPTION_FLT_INEXACT_RESULT); // The result of a floating-point operation cannot be represented exactly as a decimal fraction.
								EXCASE(EXCEPTION_FLT_INVALID_OPERATION); // This exception represents any floating-point exception not included in this list.
								EXCASE(EXCEPTION_FLT_OVERFLOW); // The exponent of a floating-point operation is greater than the magnitude allowed by the corresponding type.
								EXCASE(EXCEPTION_FLT_STACK_CHECK); // The stack overflowed or underflowed as the result of a floating-point operation.
								EXCASE(EXCEPTION_FLT_UNDERFLOW); // The exponent of a floating-point operation is less than the magnitude allowed by the corresponding type.
								EXCASE(EXCEPTION_ILLEGAL_INSTRUCTION); // The thread tried to execute an invalid instruction.
								EXCASE(EXCEPTION_IN_PAGE_ERROR); // The thread tried to access a page that was not present, and the system was unable to load the page. For example, this exception might occur if a network connection is lost while running a program over the network.
								EXCASE(EXCEPTION_INT_DIVIDE_BY_ZERO); // The thread tried to divide an integer value by an integer divisor of zero.
								EXCASE(EXCEPTION_INT_OVERFLOW); // The result of an integer operation caused a carry out of the most significant bit of the result.
								EXCASE(EXCEPTION_INVALID_DISPOSITION); // An exception handler returned an invalid disposition to the exception dispatcher. Programmers using a high-level language such as C should never encounter this exception.
								EXCASE(EXCEPTION_NONCONTINUABLE_EXCEPTION); // The thread tried to continue execution after a noncontinuable exception occurred.
								EXCASE(EXCEPTION_PRIV_INSTRUCTION); // The thread tried to execute an instruction whose operation is not allowed in the current machine mode.
								EXCASE(EXCEPTION_SINGLE_STEP); // A trace trap or other single-instruction mechanism signaled that one instruction has been executed.
								EXCASE(EXCEPTION_STACK_OVERFLOW); // The thread used up its stack.
							default:
								_wsprintfA(szName, SKIPLEN(countof(szName))
									        "Exception 0x%08X", evt.u.Exception.ExceptionRecord.ExceptionCode);
						}

						_wsprintfA(szDbgText, SKIPLEN(countof(szDbgText))
							        "{%i.%i} %s at " WIN3264TEST("0x%08X","0x%08X%08X") " flags 0x%08X%s FC=%u\n",
							        evt.dwProcessId,evt.dwThreadId,
							        pszName,
							        WIN3264WSPRINT((DWORD_PTR)evt.u.Exception.ExceptionRecord.ExceptionAddress),
							        evt.u.Exception.ExceptionRecord.ExceptionFlags,
							        (evt.u.Exception.ExceptionRecord.ExceptionFlags&EXCEPTION_NONCONTINUABLE)
							        ? "(EXCEPTION_NONCONTINUABLE)" : "",
							        evt.u.Exception.dwFirstChance);
						_printf(szDbgText);
					}
				}

				BOOL bDumpOnBreakPoint = gpSrv->DbgInfo.bDebuggerRequestDump;

				// Если просили цеплять к отладчику создаваемые дочерние процессы
				// или сделать дампы пачки процессов сразу
				if (IsDumpMulti()
					&& (!lbNonContinuable && (evt.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT)))
				{
					// Когда отладчик цепляется к процессу в первый раз - возникает EXCEPTION_BREAKPOINT
					CEDebugProcessInfo pi = {};
					if (gpSrv->DbgInfo.pDebugTreeProcesses
						&& gpSrv->DbgInfo.pDebugTreeProcesses->Get(evt.dwProcessId, &pi))
					{
						if (!pi.bWasBreak)
						{
							pi.bWasBreak = TRUE;
							gpSrv->DbgInfo.pDebugTreeProcesses->Set(evt.dwProcessId, pi);
							if (gpSrv->DbgInfo.bUserRequestDump)
								bDumpOnBreakPoint = TRUE;
						}
						else
						{
							bDumpOnBreakPoint = TRUE;
						}
					}
				}

				if (gpSrv->DbgInfo.bDebuggerRequestDump
					|| (!lbNonContinuable && !gpSrv->DbgInfo.bDebugProcessTree
						&& (evt.u.Exception.ExceptionRecord.ExceptionCode != MS_VC_THREADNAME_EXCEPTION)
						&& (evt.u.Exception.ExceptionRecord.ExceptionCode != EXCEPTION_BREAKPOINT))
					|| (IsDumpMulti()
						&& ((evt.u.Exception.ExceptionRecord.ExceptionCode>=0xC0000000)
							|| (bDumpOnBreakPoint && (evt.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT))))
					)
				{
					BOOL bGenerateTreeBreak = gpSrv->DbgInfo.bDebugProcessTree
						&& (gpSrv->DbgInfo.bDebuggerRequestDump || lbNonContinuable);

					if (gpSrv->DbgInfo.bDebugProcessTree
						&& !bGenerateTreeBreak
						&& (evt.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT))
					{
						if (gpSrv->DbgInfo.nWaitTreeBreaks == 0)
						{
							bGenerateTreeBreak = TRUE;
							gpSrv->DbgInfo.nWaitTreeBreaks++;
						}
					}

					gpSrv->DbgInfo.bDebuggerRequestDump = FALSE; // один раз

					char szConfirm[2048];

					if (evt.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
					{
						if (gpSrv->DbgInfo.nDebugDumpProcess || gpSrv->DbgInfo.bAutoDump)
							szConfirm[0] = 0;
						else
							lstrcpynA(szConfirm, szDbgText, countof(szConfirm));
					}
					else
					{
						_wsprintfA(szConfirm, SKIPLEN(countof(szConfirm)) "%s exception (FC=%u)\n",
							lbNonContinuable ? "Non continuable" : "Continuable",
							evt.u.Exception.dwFirstChance);
						StringCchCatA(szConfirm, countof(szConfirm), szDbgText);
					}
					StringCchCatA(szConfirm, countof(szConfirm), "\nCreate minidump (<No> - fulldump)?");

					//bGenerateTreeBreak -> GenerateTreeDebugBreak(evt.dwProcessId)

					WriteMiniDump(evt.dwProcessId, evt.dwThreadId, &evt.u.Exception.ExceptionRecord, szConfirm, bGenerateTreeBreak);

					if (IsDumpMulti() && (evt.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT))
					{
						if (gpSrv->DbgInfo.nWaitTreeBreaks > 0)
							gpSrv->DbgInfo.nWaitTreeBreaks--;
					}

					gpSrv->DbgInfo.nDumpsCounter++;
				}

				if (!lbNonContinuable /*|| (evt.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT)*/)
				{
					dwContinueStatus = DBG_CONTINUE;
				}

				break;
			} // EXCEPTION_DEBUG_EVENT

			case OUTPUT_DEBUG_STRING_EVENT:
				//8 Reports an output-debugging-string debugging event. The value of u.DebugString specifies an OUTPUT_DEBUG_STRING_INFO structure.
			{
				wszDbgText[0] = 0;
				msprintf(wszDbgText, countof(wszDbgText), L"{PID=%i.TID=%i} ", evt.dwProcessId, evt.dwThreadId);
				int iPidTidLen = lstrlen(wszDbgText);
				DWORD_PTR iReadMax = countof(wszDbgText) - iPidTidLen - 2;
				wchar_t* pwszDbg = wszDbgText + iPidTidLen;

				if (evt.u.DebugString.nDebugStringLength > iReadMax)
					evt.u.DebugString.nDebugStringLength = LOWORD(iReadMax);

				DWORD_PTR nRead = 0;

				HANDLE hProcess = GetProcessHandleForDebug(evt.dwProcessId);

				if (evt.u.DebugString.fUnicode)
				{
					if (!ReadProcessMemory(hProcess, evt.u.DebugString.lpDebugStringData, pwszDbg, 2*evt.u.DebugString.nDebugStringLength, &nRead))
					{
						_wcscpy_c(pwszDbg, iReadMax, L"???");
					}
					else
					{
						pwszDbg[min(iReadMax,nRead+1)] = 0;
					}

					static int nPrefixLen = lstrlen(CONEMU_CONHOST_CREATED_MSG);
					if (memcmp(pwszDbg, CONEMU_CONHOST_CREATED_MSG, nPrefixLen*sizeof(pwszDbg[0])) == 0)
					{
						LPWSTR pszEnd = NULL;
						DWORD nConHostPID = wcstoul(pwszDbg+nPrefixLen, &pszEnd, 10);
						if (nConHostPID && !gpSrv->DbgInfo.pDebugTreeProcesses->Get(nConHostPID, NULL))
						{
							AttachConHost(nConHostPID);
						}
					}
				}
				else
				{
					if (!ReadProcessMemory(hProcess, evt.u.DebugString.lpDebugStringData, szDbgText, evt.u.DebugString.nDebugStringLength, &nRead))
					{
						_wcscpy_c(pwszDbg, iReadMax, L"???");
					}
					else
					{
						szDbgText[min(iReadMax,nRead+1)] = 0;
						if ((memcmp(szDbgText, "\xEF\xBB\xBF", 3) != 0)
							|| !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, szDbgText+3, -1, pwszDbg, iReadMax+1))
						MultiByteToWideChar(CP_ACP, 0, szDbgText, -1, pwszDbg, iReadMax+1);
					}
				}

				int iWrite = lstrlen(pwszDbg);
				if ((iWrite > 0) && (pwszDbg[iWrite-1] != L'\n'))
				{
					pwszDbg[iWrite - 1] = L'\n'; pwszDbg[iWrite] = 0;
				}

				_wprintf(wszDbgText);

				dwContinueStatus = DBG_CONTINUE;

				break;
			} // OUTPUT_DEBUG_STRING_EVENT
		} //switch (evt.dwDebugEventCode)

		// Продолжить отлаживаемый процесс
		ContinueDebugEvent(evt.dwProcessId, evt.dwThreadId, dwContinueStatus);
	}

	//if (hCOMDLG32)
	//	FreeLibrary(hCOMDLG32);
}

void GenerateMiniDumpFromCtrlBreak()
{
	HMODULE hKernel = GetModuleHandle(L"kernel32.dll");
	typedef BOOL (WINAPI* DebugBreakProcess_t)(HANDLE Process);
	DebugBreakProcess_t DebugBreakProcess_f = (DebugBreakProcess_t)(hKernel ? GetProcAddress(hKernel, "DebugBreakProcess") : NULL);
	if (DebugBreakProcess_f)
	{
		_printf(CE_CONEMUC_NAME_A ": Sending DebugBreak event to process\n");
		gpSrv->DbgInfo.bDebuggerRequestDump = TRUE;
		DWORD dwErr = 0;
		_ASSERTE(gpSrv->hRootProcess!=NULL);
		if (!DebugBreakProcess_f(gpSrv->hRootProcess))
		{
			dwErr = GetLastError();
			//_ASSERTE(FALSE && dwErr==0);
			_printf(CE_CONEMUC_NAME_A ": Sending DebugBreak event failed, Code=x%X, WriteMiniDump on the fly\n", dwErr);
			gpSrv->DbgInfo.bDebuggerRequestDump = FALSE;
			WriteMiniDump(gpSrv->dwRootProcess, gpSrv->dwRootThread, NULL);
		}
	}
	else
	{
		_printf(CE_CONEMUC_NAME_A ": DebugBreakProcess not found in kernel32.dll\n");
	}
}

void GenerateTreeDebugBreak(DWORD nExcludePID)
{
	_ASSERTE(gpSrv->DbgInfo.bDebugProcessTree);

	DWORD dwErr = 0;
	HMODULE hKernel = GetModuleHandle(L"kernel32.dll");
	typedef BOOL (WINAPI* DebugBreakProcess_t)(HANDLE Process);
	DebugBreakProcess_t DebugBreakProcess_f = (DebugBreakProcess_t)(hKernel ? GetProcAddress(hKernel, "DebugBreakProcess") : NULL);
	if (DebugBreakProcess_f)
	{
		_printf(CE_CONEMUC_NAME_A ": Sending DebugBreak event to processes:");

		DWORD nPID = 0; HANDLE hProcess = NULL;

		MMap<DWORD,CEDebugProcessInfo>* pDebugTreeProcesses = gpSrv->DbgInfo.pDebugTreeProcesses;
		CEDebugProcessInfo pi = {};

		if (pDebugTreeProcesses->GetNext(NULL, &nPID, &pi))
		{
			while (nPID)
			{
				if (nPID != nExcludePID)
				{
					_printf(" %u", nPID);

					if (!pi.hProcess)
					{
						pi.hProcess = GetProcessHandleForDebug(nPID);
					}

					if (DebugBreakProcess_f(pi.hProcess))
					{
						gpSrv->DbgInfo.nWaitTreeBreaks++;
					}
					else
					{
						dwErr = GetLastError();
						_printf("\nConEmuC: Sending DebugBreak event failed, Code=x%X\n", dwErr);
					}
				}

				if (!pDebugTreeProcesses->GetNext(&nPID, &nPID, &pi))
					break;
			}
			_printf("\n");
		}
	}
	else
	{
		_printf(CE_CONEMUC_NAME_A ": DebugBreakProcess not found in kernel32.dll\n");
	}
}
