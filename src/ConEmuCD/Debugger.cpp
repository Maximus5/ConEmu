
/*
Copyright (c) 2013 Maximus5
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
#include <ShlObj.h>
#include "../ConEmu/version.h"
#include "../common/MMap.h"
#include "../common/TokenHelper.h"
#include "ConsoleHelp.h"
#include "UnicodeTest.h"

#ifdef __GNUC__
	#include "../common/DbgHlpGcc.h"
#else
	#include <Dbghelp.h>
#endif

#ifndef _T
	#define __T(x) L ## x
	#define _T(x) __T(x)
#endif


DWORD WINAPI DebugThread(LPVOID lpvParam);
void WriteMiniDump(DWORD dwProcessId, DWORD dwThreadId, EXCEPTION_RECORD *pExceptionRecord, LPCSTR asConfirmText = NULL, BOOL bTreeBreak = FALSE);
void GenerateTreeDebugBreak(DWORD nExcludePID);

#define CE_TREE_TEMPLATE WIN3264TEST(L"ConEmuC",L"ConEmuC64") L": DebuggerPID=%u RootPID=%u Count=%i"


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
	_wsprintf(szTitle, SKIPLEN(countof(szTitle)) CE_TREE_TEMPLATE,
		GetCurrentProcessId(), gpSrv->dwRootProcess, gpSrv->DbgInfo.nProcessCount);
	SetTitle(szTitle);
}


int RunDebugger()
{
	if (!gpSrv->DbgInfo.pDebugTreeProcesses)
	{
		gpSrv->DbgInfo.pDebugTreeProcesses = (MMap<DWORD,CEDebugProcessInfo>*)calloc(1,sizeof(*gpSrv->DbgInfo.pDebugTreeProcesses));
		gpSrv->DbgInfo.pDebugTreeProcesses->Init(1024);
	}

	UpdateDebuggerTitle();

	// Если это новая консоль - увеличить ее размер, для удобства
	if (IsWindowVisible(ghConWnd))
	{
		HANDLE hCon = ghConOut;
		CONSOLE_SCREEN_BUFFER_INFO csbi = {};
		GetConsoleScreenBufferInfo(hCon, &csbi);
		if (csbi.dwSize.X < 260)
		{
			COORD crNewSize = {260, 9999};
			SetConsoleScreenBufferSize(ghConOut, crNewSize);
		}
		//if ((csbi.srWindow.Right - csbi.srWindow.Left + 1) < 120)
		//{
		//	COORD crMax = GetLargestConsoleWindowSize(hCon);
		//	if ((crMax.X - 10) > (csbi.srWindow.Right - csbi.srWindow.Left + 1))
		//	{
		//		COORD crSize = {((int)((crMax.X - 15)/10))*10, min(crMax.Y, (csbi.srWindow.Bottom - csbi.srWindow.Top + 1))};
		//		SMALL_RECT srWnd = {0, csbi.srWindow.Top, crSize.X - 1, csbi.srWindow.Bottom};
		//		MONITORINFO mi = {sizeof(mi)};
		//		GetMonitorInfo(MonitorFromWindow(ghConWnd, MONITOR_DEFAULTTONEAREST), &mi);
		//		RECT rcWnd = {}; GetWindowRect(ghConWnd, &rcWnd);
		//		SetWindowPos(ghConWnd, NULL, min(rcWnd.left,(mi.rcWork.left+50)), rcWnd.top, 0,0, SWP_NOSIZE|SWP_NOZORDER);
		//		SetConsoleSize(9999, crSize, srWnd, "StartDebugger");
		//	}
		//}
	}

	// Вывести в консоль информацию о версии.
	PrintVersion();
	#ifdef SHOW_DEBUG_STARTED_MSGBOX
	wchar_t szInfo[128];
	StringCchPrintf(szInfo, countof(szInfo), L"Attaching debugger...\nConEmuC PID = %u\nDebug PID = %u",
	                GetCurrentProcessId(), gpSrv->dwRootProcess);
	MessageBox(GetConEmuHWND(2), szInfo, L"ConEmuC.Debugger", 0);
	#endif

	//if (!DebugActiveProcess(gpSrv->dwRootProcess))
	//{
	//	DWORD dwErr = GetLastError();
	//	_printf("Can't start debugger! ErrCode=0x%08X\n", dwErr);
	//	return CERR_CANTSTARTDEBUGGER;
	//}
	//// Дополнительная инициализация, чтобы закрытие дебагера (наш процесс) не привело
	//// к закрытию "отлаживаемой" программы
	//pfnDebugActiveProcessStop = (FDebugActiveProcessStop)GetProcAddress(GetModuleHandle(L"kernel32.dll"),"DebugActiveProcessStop");
	//pfnDebugSetProcessKillOnExit = (FDebugSetProcessKillOnExit)GetProcAddress(GetModuleHandle(L"kernel32.dll"),"DebugSetProcessKillOnExit");
	//if (pfnDebugSetProcessKillOnExit)
	//	pfnDebugSetProcessKillOnExit(FALSE/*KillOnExit*/);
	//gpSrv->DbgInfo.bDebuggerActive = TRUE;
	//PrintDebugInfo();

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

	_ASSERTE(((gpSrv->hRootProcess!=NULL) || (gpSrv->DbgInfo.pszDebuggingCmdLine!=NULL)) && "Process handle must be opened");

	gpSrv->DbgInfo.hDebugReady = CreateEvent(NULL, FALSE, FALSE, NULL);
	// Перенес обработку отладочных событий в отдельную нить, чтобы случайно не заблокироваться с главной
	gpSrv->DbgInfo.hDebugThread = CreateThread(NULL, 0, DebugThread, NULL, 0, &gpSrv->DbgInfo.dwDebugThreadId);
	HANDLE hEvents[2] = {gpSrv->DbgInfo.hDebugReady, gpSrv->DbgInfo.hDebugThread};
	DWORD nReady = WaitForMultipleObjects(countof(hEvents), hEvents, FALSE, INFINITE);

	if (nReady != WAIT_OBJECT_0)
	{
		DWORD nExit = 0;
		GetExitCodeThread(gpSrv->DbgInfo.hDebugThread, &nExit);
		return nExit;
	}

	gpszRunCmd = (wchar_t*)calloc(1,2);

	if (!gpszRunCmd)
	{
		_printf("Can't allocate 1 wchar!\n");
		return CERR_NOTENOUGHMEM1;
	}

	gpszRunCmd[0] = 0;
	gpSrv->DbgInfo.bDebuggerActive = TRUE;
	return 0;
}

HANDLE GetProcessHandleForDebug(DWORD nPID, LPDWORD pnErrCode = NULL)
{
	_ASSERTE(gpSrv->DbgInfo.bDebuggerActive);

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
			// PROCESS_ALL_ACCESS (defined in new SDK) may fails on WinXP!
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
				_ASSERTE(pi.nPID == nPID); // уже должен быть
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
	//DWORD nExternalExitCode = -1;
	wchar_t szInfo[1024];

	if (gpSrv->DbgInfo.pszDebuggingCmdLine != NULL)
	{
		STARTUPINFO si = {sizeof(si)};
		PROCESS_INFORMATION pi = {};

		if (gpSrv->DbgInfo.bDebugProcessTree)
		{
			SetEnvironmentVariable(ENV_CONEMU_BLOCKCHILDDEBUGGERS, ENV_CONEMU_BLOCKCHILDDEBUGGERS_YES);
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
			lstrcpyn(szInfo+lstrlen(szInfo), gpSrv->DbgInfo.pszDebuggingCmdLine, 400);
			wcscat_c(szInfo, L"\n");
			_wprintf(szInfo);
			return CERR_CANTSTARTDEBUGGER;
		}

		gpSrv->hRootProcess = pi.hProcess;
		gpSrv->hRootThread = pi.hThread;
		gpSrv->dwRootProcess = pi.dwProcessId;
		gpSrv->dwRootThread = pi.dwThreadId;
		gpSrv->dwRootStartTime = GetTickCount();
	}


	/* ************************* */
	int iDbgIdx = 0, iAttachedCount = 0;
	while (true)
	{
		HANDLE hDbgProcess = NULL;
		DWORD  nDbgProcessID = 0;

		bool bFirstPID = ((iDbgIdx++) == 0);
		if (bFirstPID)
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
				if (gpSrv->DbgInfo.pszDebuggingCmdLine != NULL)
				{
					_printf("Bitness of ConEmuC and debugging program does not match\n");
					if (bFirstPID)
						return CERR_CANTSTARTDEBUGGER;
					else
						continue;
				}

				wchar_t szExe[MAX_PATH+16];
				wchar_t szCmdLine[MAX_PATH*2];
				if (GetModuleFileName(NULL, szExe, countof(szExe)-16))
				{
					wchar_t* pszName = (wchar_t*)PointToName(szExe);
					_wcscpy_c(pszName, 16, (nBits == 32) ? L"ConEmuC.exe" : L"ConEmuC64.exe");
					_wsprintf(szCmdLine, SKIPLEN(countof(szCmdLine))
						L"\"%s\" /DEBUGPID=%u %s", szExe, nDbgProcessID,
						(gpSrv->DbgInfo.nDebugDumpProcess == 1) ? L"/DUMP" :
						(gpSrv->DbgInfo.nDebugDumpProcess == 2) ? L"/MINIDUMP" :
						(gpSrv->DbgInfo.nDebugDumpProcess == 3) ? L"/FULLDUMP" : L"");

					STARTUPINFO si = {sizeof(si)};
					PROCESS_INFORMATION pi = {};
					if (CreateProcess(NULL, szCmdLine, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
					{
						// Ждать НЕ будем, сразу на выход

						//HANDLE hEvents[2] = {pi.hProcess, ghExitQueryEvent};
						//nWait = WaitForMultipleObjects(countof(hEvents), hEvents, FALSE, INFINITE);
						//if (nWait == WAIT_OBJECT_0)
						//{
						//	//GetExitCodeProcess(pi.hProcess, &nExternalExitCode);
						//	nExternalExitCode = 0;
						//}

						//CloseHandle(pi.hProcess);
						//CloseHandle(pi.hThread);

						//if (nExternalExitCode == 0)
						//{
						//	goto done;
						//}

						// Может там еще процессы в списке на дамп?
						continue;
					}
					else
					{
						DWORD dwErr = GetLastError();
						_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"Can't start external debugger '%s'. ErrCode=0x%08X\n",
							szCmdLine, dwErr);
						_wprintf(szInfo);
						if (bFirstPID)
							return CERR_CANTSTARTDEBUGGER;
						else
							continue;
					}
				}


				wchar_t szProc[64]; szProc[0] = 0;
				PROCESSENTRY32 pi = {sizeof(pi)};
				if (GetProcessInfo(nDbgProcessID, &pi))
					_wcscpyn_c(szProc, countof(szProc), pi.szExeFile, countof(szProc));

				_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"Bits are incompatible. Can't debug '%s' PID=%i\n",
					szProc[0] ? szProc : L"not found", nDbgProcessID);
				_wprintf(szInfo);
				if (bFirstPID)
					return CERR_CANTSTARTDEBUGGER;
				else
					continue;
			}
		}

		if (gpSrv->DbgInfo.pszDebuggingCmdLine == NULL)
		{
			if (!DebugActiveProcess(nDbgProcessID))
			{
				DWORD dwErr = GetLastError();

				wchar_t szProc[64]; szProc[0] = 0;
				PROCESSENTRY32 pi = {sizeof(pi)};
				if (GetProcessInfo(nDbgProcessID, &pi))
					_wcscpyn_c(szProc, countof(szProc), pi.szExeFile, countof(szProc));

				_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"Can't attach debugger to '%s' PID=%i. ErrCode=0x%08X\n",
					szProc[0] ? szProc : L"not found", nDbgProcessID, dwErr);
				_wprintf(szInfo);
				return CERR_CANTSTARTDEBUGGER;
			}
		}

		iAttachedCount++;
	}

	if (iAttachedCount == 0)
	{
		return CERR_CANTSTARTDEBUGGER;
	}
	/* **************** */

	// Дополнительная инициализация, чтобы закрытие дебагера (наш процесс) не привело
	// к закрытию "отлаживаемой" программы
	pfnDebugActiveProcessStop = (FDebugActiveProcessStop)GetProcAddress(GetModuleHandle(L"kernel32.dll"),"DebugActiveProcessStop");
	pfnDebugSetProcessKillOnExit = (FDebugSetProcessKillOnExit)GetProcAddress(GetModuleHandle(L"kernel32.dll"),"DebugSetProcessKillOnExit");

	if (pfnDebugSetProcessKillOnExit)
		pfnDebugSetProcessKillOnExit(FALSE/*KillOnExit*/);

	gpSrv->DbgInfo.bDebuggerActive = TRUE;
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

bool GetSaveDumpName(DWORD dwProcessId, bool bFull, wchar_t* dmpfile, DWORD cchMaxDmpFile)
{
	bool bRc = false;

	HMODULE hCOMDLG32 = NULL;
	typedef BOOL (WINAPI* GetSaveFileName_t)(LPOPENFILENAMEW lpofn);
	GetSaveFileName_t _GetSaveFileName = NULL;

	if (!gpSrv->DbgInfo.bDebugProcessTree)
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

	if (gpSrv->DbgInfo.bDebugProcessTree || !_GetSaveFileName)
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
			wchar_t szMinor[8] = L""; lstrcpyn(szMinor, _T(MVV_4a), countof(szMinor));
			_wsprintf(dmpfile+nLen, SKIPLEN(cchMaxDmpFile-nLen) L"\\Trap-%02u%02u%02u%s-%u.%s",
				MVV_1, MVV_2, MVV_3,szMinor, dwProcessId,
				bFull ? L"dmp" : L"mdmp");

			bRc = true;
		}
	}

	return bRc;
}

// gpSrv->dwRootProcess
void WriteMiniDump(DWORD dwProcessId, DWORD dwThreadId, EXCEPTION_RECORD *pExceptionRecord, LPCSTR asConfirmText /*= NULL*/, BOOL bTreeBreak /*= FALSE*/)
{
	MINIDUMP_TYPE dumpType = MiniDumpNormal;
	
	char szTitleA[64];
	_wsprintfA(szTitleA, SKIPLEN(countof(szTitleA)) "ConEmuC Debugging PID=%u, Debugger PID=%u", dwProcessId, GetCurrentProcessId());
	wchar_t szTitle[64];
	_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuC Debugging PID=%u, Debugger PID=%u", dwProcessId, GetCurrentProcessId());

	int nBtn = 0;
	
	if (gpSrv->DbgInfo.nDebugDumpProcess == 2 || gpSrv->DbgInfo.nDebugDumpProcess == 3)
		nBtn = (gpSrv->DbgInfo.nDebugDumpProcess == 2) ? IDYES : IDNO;
	else
		nBtn = MessageBoxA(NULL, asConfirmText ? asConfirmText : "Create minidump (<No> - fulldump)?", szTitleA, MB_YESNOCANCEL|MB_SYSTEMMODAL);

	switch (nBtn)
	{
	case IDYES:
		break;
	case IDNO:
		dumpType = MiniDumpWithFullMemory;
		break;
	default:
		return;
	}

	// Т.к. в режиме "ProcessTree" мы пишем пачку дампов - спрашивать тип дампа будем один раз.
	if (gpSrv->DbgInfo.bDebugProcessTree && (gpSrv->DbgInfo.nDebugDumpProcess <= 1))
	{
		gpSrv->DbgInfo.nDebugDumpProcess = (nBtn == IDNO) ? 3 : 2;
	}

	if (bTreeBreak)
	{
		GenerateTreeDebugBreak(dwProcessId);
	}

	bool bDumpSucceeded = false;
	HANDLE hDmpFile = NULL;
	//HMODULE hDbghelp = NULL;
	wchar_t szErrInfo[MAX_PATH*2];

	wchar_t dmpfile[MAX_PATH]; dmpfile[0] = 0;

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
				_printf("\nMiniDumpWriteDump succeeded\n");
				bDumpSucceeded = true;
			}

			break;
		}

	} // end while (GetSaveDumpName(dwProcessId, (dumpType == MiniDumpWithFullMemory), dmpfile, countof(dmpfile)))

	if (hDmpFile != INVALID_HANDLE_VALUE && hDmpFile != NULL)
	{
		CloseHandle(hDmpFile);
	}

	//if (hDbghelp)
	//{
	//	FreeLibrary(hDbghelp);
	//}

	//if (hCOMDLG32)
	//{
	//	FreeLibrary(hCOMDLG32);
	//}

	// В Win2k еще не было функции "отцепиться от процесса"
	if (bDumpSucceeded && gpSrv->DbgInfo.nDebugDumpProcess && !gpSrv->DbgInfo.bDebugProcessTree && (gnOsVer >= 0x0501))
	{
		// Дело сделали, закрываемся
		SetTerminateEvent(ste_WriteMiniDump);

		//if (pfnGetConsoleProcessList)
		//{
		//	DWORD nCurCount = 0;
		//	DWORD nConsolePids[128] = {};
		//	nCurCount = pfnGetConsoleProcessList(nConsolePids, countof(nConsolePids));

		//	// Но только если в консоли кроме нас никого нет
		//	if (nCurCount == 0)
		//	{
		//		PostMessage(ghConWnd, WM_CLOSE, 0, 0);
		//	}
		//	else
		//	{
		//		SetTerminateEvent();
		//	}
		//}
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
						_printf("ConEmuC: Press Ctrl+Break to create minidump of debugging process\n");
					}
					else
					{
						// Сразу сделать дамп и выйти
						HandlerRoutine(CTRL_BREAK_EVENT);
					}
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
			}
			case EXCEPTION_DEBUG_EVENT:
				//1 Reports an exception debugging event. The value of u.Exception specifies an EXCEPTION_DEBUG_INFO structure.
			{
				lbNonContinuable = (evt.u.Exception.ExceptionRecord.ExceptionFlags&EXCEPTION_NONCONTINUABLE)==EXCEPTION_NONCONTINUABLE;

				//static bool bAttachEventRecieved = false;
				//if (!bAttachEventRecieved)
				//{
				//	bAttachEventRecieved = true;
				//	StringCchPrintfA(szDbgText, countof(szDbgText),"{%i.%i} Debugger attached successfully. (0x%08X address 0x%08X flags 0x%08X%s)\n",
				//		evt.dwProcessId,evt.dwThreadId,
				//		evt.u.Exception.ExceptionRecord.ExceptionCode,
				//		evt.u.Exception.ExceptionRecord.ExceptionAddress,
				//		evt.u.Exception.ExceptionRecord.ExceptionFlags,
				//		(evt.u.Exception.ExceptionRecord.ExceptionFlags&EXCEPTION_NONCONTINUABLE) ? "(EXCEPTION_NONCONTINUABLE)" : "");
				//}
				//else
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
					}
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

				if (gpSrv->DbgInfo.bDebugProcessTree
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
						}
						else
						{
							bDumpOnBreakPoint = TRUE;
						}
					}
				}

				if (gpSrv->DbgInfo.bDebuggerRequestDump
					|| (!lbNonContinuable && !gpSrv->DbgInfo.bDebugProcessTree
						&& (evt.u.Exception.ExceptionRecord.ExceptionCode != EXCEPTION_BREAKPOINT))
					|| (gpSrv->DbgInfo.bDebugProcessTree
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
						if (gpSrv->DbgInfo.nDebugDumpProcess)
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

					//GenerateTreeDebugBreak

					WriteMiniDump(evt.dwProcessId, evt.dwThreadId, &evt.u.Exception.ExceptionRecord, szConfirm, bGenerateTreeBreak);

					if (gpSrv->DbgInfo.bDebugProcessTree && (evt.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT))
					{
						if (gpSrv->DbgInfo.nWaitTreeBreaks > 0)
							gpSrv->DbgInfo.nWaitTreeBreaks--;
					}
				}

				if (!lbNonContinuable /*|| (evt.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT)*/)
				{
					dwContinueStatus = DBG_CONTINUE;
				}
			}
			break;
			case OUTPUT_DEBUG_STRING_EVENT:
				//8 Reports an output-debugging-string debugging event. The value of u.DebugString specifies an OUTPUT_DEBUG_STRING_INFO structure.
			{
				wszDbgText[0] = 0;

				if (evt.u.DebugString.nDebugStringLength >= 1024) evt.u.DebugString.nDebugStringLength = 1023;

				DWORD_PTR nRead = 0;

				HANDLE hProcess = GetProcessHandleForDebug(evt.dwProcessId);

				if (evt.u.DebugString.fUnicode)
				{
					if (!ReadProcessMemory(hProcess, evt.u.DebugString.lpDebugStringData, wszDbgText, 2*evt.u.DebugString.nDebugStringLength, &nRead))
					{
						wcscpy_c(wszDbgText, L"???");
					}
					else
					{
						wszDbgText[min(1023,nRead+1)] = 0;
					}

					static int nPrefixLen = lstrlen(CONEMU_CONHOST_CREATED_MSG);
					if (memcmp(wszDbgText, CONEMU_CONHOST_CREATED_MSG, nPrefixLen*sizeof(wszDbgText[0])) == 0)
					{
						LPWSTR pszEnd = NULL;
						DWORD nConHostPID = wcstoul(wszDbgText+nPrefixLen, &pszEnd, 10);
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
						wcscpy_c(wszDbgText, L"???");
					}
					else
					{
						szDbgText[min(1023,nRead+1)] = 0;
						// CP_ACP differs from CP_OEMCP, thats why we need some overhead...
						MultiByteToWideChar(CP_ACP, 0, szDbgText, -1, wszDbgText, 1024);
					}
				}

				WideCharToMultiByte(CP_OEMCP, 0, wszDbgText, -1, szDbgText, 1024, 0, 0);

				#ifdef CRTPRINTF
				{
					_printf("{PID=%i.TID=%i} ", evt.dwProcessId,evt.dwThreadId, wszDbgText);
				}
				#else
				{
					_printf("{PID=%i.TID=%i} %s", evt.dwProcessId,evt.dwThreadId, szDbgText);
					int nLen = lstrlenA(szDbgText);

					if (nLen > 0 && szDbgText[nLen-1] != '\n')
						_printf("\n");
				}
				#endif
				
				dwContinueStatus = DBG_CONTINUE;
			}
			break;
		}

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
		_printf("ConEmuC: Sending DebugBreak event to process\n");
		gpSrv->DbgInfo.bDebuggerRequestDump = TRUE;
		DWORD dwErr = 0;
		_ASSERTE(gpSrv->hRootProcess!=NULL);
		if (!DebugBreakProcess_f(gpSrv->hRootProcess))
		{
			dwErr = GetLastError();
			//_ASSERTE(FALSE && dwErr==0);
			_printf("ConEmuC: Sending DebugBreak event failed, Code=x%X, WriteMiniDump on the fly\n", dwErr);
			gpSrv->DbgInfo.bDebuggerRequestDump = FALSE;
			WriteMiniDump(gpSrv->dwRootProcess, gpSrv->dwRootThread, NULL);
		}
	}
	else
	{
		_printf("ConEmuC: DebugBreakProcess not found in kernel32.dll\n");
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
		_printf("ConEmuC: Sending DebugBreak event to processes:");

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
		_printf("ConEmuC: DebugBreakProcess not found in kernel32.dll\n");
	}
}
