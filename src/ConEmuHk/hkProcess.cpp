
/*
Copyright (c) 2015-2017 Maximus5
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
	#define DebugString(x) //OutputDebugString(x)
#else
	#define DebugString(x) //OutputDebugString(x)
#endif

#include "../common/Common.h"
#include "../common/execute.h"
#include "../common/MMap.h"
#include "../common/MStrDup.h"

#include "Ansi.h"
#include "DefTermHk.h"
#include "hkProcess.h"
#include "MainThread.h"
#include "ShellProcessor.h"

/* **************** */

extern bool gbCurDirChanged;
extern HANDLE ghSkipSetThreadContextForThread; // Injects.cpp

/* **************** */

// Called from OnShellExecCmdLine
HRESULT OurShellExecCmdLine(HWND hwnd, LPCWSTR pwszCommand, LPCWSTR pwszStartDir, bool bRunAsAdmin, bool bForce)
{
	HRESULT hr = E_UNEXPECTED;
	BOOL bShell = FALSE;

	CEStr lsLog = lstrmerge(L"OnShellExecCmdLine", bRunAsAdmin ? L"(RunAs): " : L": ", pwszCommand);
	DefTermLogString(lsLog);

	// Bad thing, ShellExecuteEx needs File&Parm, but we get both in pwszCommand
	CEStr szExe;
	LPCWSTR pszFile = pwszCommand;
	LPCWSTR pszParm = pwszCommand;
	if (NextArg(&pszParm, szExe) == 0)
	{
		pszFile = szExe; pszParm = SkipNonPrintable(pszParm);
	}
	else
	{
		// Failed
		pszFile = pwszCommand; pszParm = NULL;
	}

	if (!bForce)
	{
		DWORD nCheckSybsystem1 = 0, nCheckBits1 = 0;
		if (!FindImageSubsystem(pszFile, nCheckSybsystem1, nCheckBits1))
		{
			hr = (HRESULT)-1;
			DefTermLogString(L"OnShellExecCmdLine: FindImageSubsystem failed");
			goto wrap;
		}
		if (nCheckSybsystem1 != IMAGE_SUBSYSTEM_WINDOWS_CUI)
		{
			hr = (HRESULT)-1;
			DefTermLogString(L"OnShellExecCmdLine: !=IMAGE_SUBSYSTEM_WINDOWS_CUI");
			goto wrap;
		}
	}

	// "Run as admin" was requested?
	if (bRunAsAdmin)
	{
		SHELLEXECUTEINFO sei = {sizeof(sei), 0, hwnd, L"runas", pszFile, pszParm, pwszStartDir, SW_SHOWNORMAL};
		bShell = OnShellExecuteExW(&sei);
	}
	else
	{
		wchar_t* pwCommand = lstrdup(pwszCommand);
		DWORD nCreateFlags = CREATE_NEW_CONSOLE|CREATE_UNICODE_ENVIRONMENT|CREATE_DEFAULT_ERROR_MODE;
		STARTUPINFO si = {sizeof(si)};
		PROCESS_INFORMATION pi = {};
		bShell = OnCreateProcessW(NULL, pwCommand, NULL, NULL, FALSE, nCreateFlags, NULL, pwszStartDir, &si, &pi);
		if (bShell)
		{
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
	}

	hr = bShell ? S_OK : HRESULT_FROM_WIN32(GetLastError());
wrap:
	return hr;
}

/* **************** */

// May be called from "C" programs
VOID WINAPI OnExitProcess(UINT uExitCode)
{
	//typedef BOOL (WINAPI* OnExitProcess_t)(UINT uExitCode);
	ORIGINAL_KRNL(ExitProcess);

	#if 0
	if (gbIsLessProcess)
	{
		_ASSERTE(FALSE && "Continue to ExitProcess");
	}
	#endif

	gnDllState |= ds_OnExitProcess;

	#ifdef PRINT_ON_EXITPROCESS_CALLS
	wchar_t szInfo[80]; _wsprintf(szInfo, SKIPCOUNT(szInfo) L"\n\x1B[1;31;40m::ExitProcess(%u) called\x1B[m\n", uExitCode);
	WriteProcessed2(szInfo, lstrlen(szInfo), NULL, wps_Error);
	#endif

	// And terminate our threads
	DoDllStop(false, ds_OnExitProcess);

	bool bUseForceTerminate;

	// Issue 1865: Due to possible dead locks in LdrpAcquireLoaderLock() call TerminateProcess
	bUseForceTerminate = gbHookServerForcedTermination;

	#ifdef USE_GH_272_WORKAROUND
	// gh#272: For unknown yet reason existance of nvd3d9wrap.dll (or nvd3d9wrapx.dll on 64-bit)
	//		caused stack overflow with following calls
	//
	//		nvd3d9wrap!GetNVDisplayW+0x174f
	//		nvd3d9wrap!GetNVDisplayW+0x174f
	//		user32!_UserClientDllInitialize+0x2ca
	//		ntdll!LdrpCallInitRoutine+0x14
	//		ntdll!LdrShutdownProcess+0x1aa
	//		ntdll!RtlExitUserProcess+0x74
	//		kernel32!ExitProcessStub+0x12
	//		CallExit!main+0x47
	if (!bUseForceTerminate && GetModuleHandle(WIN3264TEST(L"nvd3d9wrap.dll",L"nvd3d9wrapx.dll")))
	{
		bUseForceTerminate = true;
	}
	#endif // USE_GH_272_WORKAROUND

	#ifdef FORCE_GH_272_WORKAROUND
	bUseForceTerminate = true;
	#endif

	if (bUseForceTerminate)
	{
		//CancelSynchronousIo(GetCurrentThread()); // -- VISTA

		//HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		//FlushFileBuffers(hOut);
		//CloseHandle(hOut);

		ORIGINAL_KRNL(TerminateProcess);
		F(TerminateProcess)(GetCurrentProcess(), uExitCode);
		return; // Assume not to get here
	}

	F(ExitProcess)(uExitCode);
}


// For example, mintty is terminated ‘abnormally’. It calls TerminateProcess instead of ExitProcess.
BOOL WINAPI OnTerminateProcess(HANDLE hProcess, UINT uExitCode)
{
	//typedef BOOL (WINAPI* OnTerminateProcess_t)(HANDLE hProcess, UINT uExitCode);
	ORIGINAL_KRNL(TerminateProcess);
	BOOL lbRc;

	if (hProcess == GetCurrentProcess())
	{
		#ifdef PRINT_ON_EXITPROCESS_CALLS
		wchar_t szInfo[80]; _wsprintf(szInfo, SKIPCOUNT(szInfo) L"\n\x1B[1;31;40m::TerminateProcess(%u) called\x1B[m\n", uExitCode);
		WriteProcessed2(szInfo, lstrlen(szInfo), NULL, wps_Error);
		#endif

		gnDllState |= ds_OnTerminateProcess;
		// We don't need to do proper/full deinitialization,
		// because the process is to be terminated abnormally
		DoDllStop(false, ds_OnTerminateProcess);

		lbRc = F(TerminateProcess)(hProcess, uExitCode);
	}
	else
	{
		lbRc = F(TerminateProcess)(hProcess, uExitCode);
	}

	return lbRc;
}


BOOL WINAPI OnTerminateThread(HANDLE hThread, DWORD dwExitCode)
{
	//typedef BOOL (WINAPI* OnTerminateThread_t)(HANDLE hThread, UINT dwExitCode);
	ORIGINAL_KRNL(TerminateThread);
	BOOL lbRc;

	#if 0
	if (gbIsLessProcess)
	{
		_ASSERTE(FALSE && "Continue to terminate thread");
	}
	#endif

	if (hThread == GetCurrentThread())
	{
		// And terminate our service threads
		gnDllState |= ds_OnTerminateThread;

		// Main thread is abnormally terminating?
		if (GetCurrentThreadId() == gnHookMainThreadId)
		{
			DoDllStop(false, ds_OnTerminateThread);
		}
	}

	lbRc = F(TerminateThread)(hThread, dwExitCode);

	return lbRc;
}


BOOL WINAPI OnCreateProcessA(LPCSTR lpApplicationName,  LPSTR lpCommandLine,  LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory,  LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
	//typedef BOOL (WINAPI* OnCreateProcessA_t)(LPCSTR lpApplicationName,  LPSTR lpCommandLine,  LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory,  LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
	ORIGINAL_KRNL(CreateProcessA);
	BOOL lbRc = FALSE;
	DWORD dwErr = 0;


	if (ph && ph->PreCallBack)
	{
		SETARGS10(&lbRc, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);

		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	CShellProc* sp = new CShellProc();
	if (!sp || !sp->OnCreateProcessA(&lpApplicationName, (LPCSTR*)&lpCommandLine, &lpCurrentDirectory, &dwCreationFlags, lpStartupInfo))
	{
		delete sp;
		SetLastError(ERROR_FILE_NOT_FOUND);
		return FALSE;
	}
	if ((dwCreationFlags & CREATE_SUSPENDED) == 0)
	{
		DebugString(L"CreateProcessA without CREATE_SUSPENDED Flag!\n");
	}

	lbRc = F(CreateProcessA)(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	dwErr = GetLastError();


	// Если lbParamsChanged == TRUE - об инжектах позаботится ConEmuC.exe
	sp->OnCreateProcessFinished(lbRc, lpProcessInformation);
	delete sp;


	if (ph && ph->PostCallBack)
	{
		SETARGS10(&lbRc, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
		ph->PostCallBack(&args);
	}

	SetLastError(dwErr);
	return lbRc;
}


BOOL WINAPI OnCreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
	//typedef BOOL (WINAPI* OnCreateProcessW_t)(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
	ORIGINAL_KRNL(CreateProcessW);
	BOOL lbRc = FALSE;
	DWORD dwErr = 0;
	DWORD ldwCreationFlags = dwCreationFlags;

	if (ph && ph->PreCallBack)
	{
		SETARGS10(&lbRc, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, ldwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);

		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	CShellProc* sp = new CShellProc();
	if (!sp || !sp->OnCreateProcessW(&lpApplicationName, (LPCWSTR*)&lpCommandLine, &lpCurrentDirectory, &ldwCreationFlags, lpStartupInfo))
	{
		delete sp;
		SetLastError(ERROR_FILE_NOT_FOUND);
		return FALSE;
	}

	if ((ldwCreationFlags & CREATE_SUSPENDED) == 0)
	{
		DebugString(L"CreateProcessW without CREATE_SUSPENDED Flag!\n");
	}

	#ifdef _DEBUG
	SetLastError(0);
	#endif

	#ifdef SHOWCREATEPROCESSTICK
	prepare_timings;
	force_print_timings(L"CreateProcessW");
	#endif

	#if 0
	// This is disabled for now. Command will create new visible console window,
	// but excpected behavior will be "reuse" of existing console window
	if (!sp->GetArgs()->bNewConsole && sp->GetArgs()->pszUserName)
	{
		LPCWSTR pszName = sp->GetArgs()->pszUserName;
		LPCWSTR pszDomain = sp->GetArgs()->pszDomain;
		LPCWSTR pszPassword = sp->GetArgs()->szUserPassword;
		STARTUPINFOW si = {sizeof(si)};
		PROCESS_INFORMATION pi = {};
		DWORD dwOurFlags = (ldwCreationFlags & ~EXTENDED_STARTUPINFO_PRESENT);
		lbRc = CreateProcessWithLogonW(pszName, pszDomain, (pszPassword && *pszPassword) ? pszPassword : NULL, LOGON_WITH_PROFILE,
			lpApplicationName, lpCommandLine, dwOurFlags, lpEnvironment, lpCurrentDirectory,
			&si, &pi);
		if (lbRc)
			*lpProcessInformation = pi;
	}
	else
	#endif
	{
		lbRc = F(CreateProcessW)(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, ldwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	}
	dwErr = GetLastError();

	#ifdef SHOWCREATEPROCESSTICK
	force_print_timings(L"CreateProcessW - done");
	#endif

	// Если lbParamsChanged == TRUE - об инжектах позаботится ConEmuC.exe
	sp->OnCreateProcessFinished(lbRc, lpProcessInformation);
	delete sp;

	#ifdef SHOWCREATEPROCESSTICK
	force_print_timings(L"OnCreateProcessFinished - done");
	#endif


	if (ph && ph->PostCallBack)
	{
		SETARGS10(&lbRc, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, ldwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
		ph->PostCallBack(&args);
	}

	SetLastError(dwErr);
	return lbRc;
}


UINT WINAPI OnWinExec(LPCSTR lpCmdLine, UINT uCmdShow)
{
	//typedef BOOL (WINAPI* OnWinExec_t)(LPCSTR lpCmdLine, UINT uCmdShow);

	if (!lpCmdLine || !*lpCmdLine)
	{
		_ASSERTEX(lpCmdLine && *lpCmdLine);
		return 0;
	}

	STARTUPINFOA si = {sizeof(si)};
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = uCmdShow;
	DWORD nCreateFlags = NORMAL_PRIORITY_CLASS;
	PROCESS_INFORMATION pi = {};

	int nLen = lstrlenA(lpCmdLine);
	LPSTR pszCmd = (char*)malloc(nLen+1);
	if (!pszCmd)
	{
		return 0; // out of memory
	}
	lstrcpynA(pszCmd, lpCmdLine, nLen+1);

	UINT nRc = 0;
	BOOL bRc = OnCreateProcessA(NULL, pszCmd, NULL, NULL, FALSE, nCreateFlags, NULL, NULL, &si, &pi);

	free(pszCmd);

	if (bRc)
	{
		// If the function succeeds, the return value is greater than 31.
		nRc = 32;
	}
	else
	{
		nRc = GetLastError();
		if (nRc != ERROR_BAD_FORMAT && nRc != ERROR_FILE_NOT_FOUND && nRc != ERROR_PATH_NOT_FOUND)
			nRc = 0;
	}

	return nRc;

#if 0
	typedef BOOL (WINAPI* OnWinExec_t)(LPCSTR lpCmdLine, UINT uCmdShow);
	ORIGINAL_KRNL(CreateProcessA);
	BOOL lbRc = FALSE;
	DWORD dwErr = 0;

	CShellProc* sp = new CShellProc();
	if (!sp || !sp->OnCreateProcessA(NULL, &lpCmdLine, NULL, &nCreateFlags, sa))
	{
		delete sp;
		SetLastError(ERROR_FILE_NOT_FOUND);
		return ERROR_FILE_NOT_FOUND;
	}

	if ((nCreateFlags & CREATE_SUSPENDED) == 0)
	{
		DebugString(L"CreateProcessA without CREATE_SUSPENDED Flag!\n");
	}

	lbRc = F(CreateProcessA)(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	dwErr = GetLastError();


	// Если lbParamsChanged == TRUE - об инжектах позаботится ConEmuC.exe
	sp->OnCreateProcessFinished(lbRc, lpProcessInformation);
	delete sp;


	if (ph && ph->PostCallBack)
	{
		SETARGS10(&lbRc, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
		ph->PostCallBack(&args);
	}

	SetLastError(dwErr);
	return lbRc;
#endif
}


BOOL WINAPI OnSetCurrentDirectoryA(LPCSTR lpPathName)
{
	//typedef BOOL (WINAPI* OnSetCurrentDirectoryA_t)(LPCSTR lpPathName);
	ORIGINAL_KRNL(SetCurrentDirectoryA);
	BOOL lbRc = FALSE;

	lbRc = F(SetCurrentDirectoryA)(lpPathName);

	gbCurDirChanged |= (lbRc != FALSE);

	return lbRc;
}


BOOL WINAPI OnSetCurrentDirectoryW(LPCWSTR lpPathName)
{
	//typedef BOOL (WINAPI* OnSetCurrentDirectoryW_t)(LPCWSTR lpPathName);
	ORIGINAL_KRNL(SetCurrentDirectoryW);
	BOOL lbRc = FALSE;

	lbRc = F(SetCurrentDirectoryW)(lpPathName);

	gbCurDirChanged |= (lbRc != FALSE);

	return lbRc;
}


BOOL WINAPI OnSetThreadContext(HANDLE hThread, CONST CONTEXT *lpContext)
{
	//typedef BOOL (WINAPI* OnSetThreadContext_t)(HANDLE hThread, CONST CONTEXT *lpContext);
	ORIGINAL_KRNL(SetThreadContext);
	BOOL lbRc = FALSE;

	if (ghSkipSetThreadContextForThread && (hThread == ghSkipSetThreadContextForThread))
	{
		lbRc = FALSE;
		SetLastError(ERROR_INVALID_HANDLE);
	}
	else
	{
		lbRc = F(SetThreadContext)(hThread, lpContext);
	}

	return lbRc;
}


BOOL WINAPI OnShellExecuteExA(LPSHELLEXECUTEINFOA lpExecInfo)
{
	//typedef BOOL (WINAPI* OnShellExecuteExA_t)(LPSHELLEXECUTEINFOA lpExecInfo);
	ORIGINAL_EX(ShellExecuteExA);
	if (!F(ShellExecuteExA))
	{
		SetLastError(ERROR_INVALID_FUNCTION);
		return FALSE;
	}

	//LPSHELLEXECUTEINFOA lpNew = NULL;
	//DWORD dwProcessID = 0;
	//wchar_t* pszTempParm = NULL;
	//wchar_t szComSpec[MAX_PATH+1], szConEmuC[MAX_PATH+1]; szComSpec[0] = szConEmuC[0] = 0;
	//LPSTR pszTempApp = NULL, pszTempArg = NULL;
	//lpNew = (LPSHELLEXECUTEINFOA)malloc(lpExecInfo->cbSize);
	//memmove(lpNew, lpExecInfo, lpExecInfo->cbSize);

	CShellProc* sp = new CShellProc();
	if (!sp || !sp->OnShellExecuteExA(&lpExecInfo))
	{
		delete sp;
		SetLastError(ERROR_FILE_NOT_FOUND);
		return FALSE;
	}

	//// Under ConEmu only!
	//if (ghConEmuWndDC)
	//{
	//	if (!lpNew->hwnd || lpNew->hwnd == GetRealConsoleWindow())
	//		lpNew->hwnd = ghConEmuWnd;

	//	HANDLE hDummy = NULL;
	//	DWORD nImageSubsystem = 0, nImageBits = 0;
	//	if (PrepareExecuteParmsA(eShellExecute, lpNew->lpVerb, lpNew->fMask,
	//			lpNew->lpFile, lpNew->lpParameters,
	//			hDummy, hDummy, hDummy,
	//			&pszTempApp, &pszTempArg, nImageSubsystem, nImageBits))
	//	{
	//		// Меняем
	//		lpNew->lpFile = pszTempApp;
	//		lpNew->lpParameters = pszTempArg;
	//	}
	//}

	BOOL lbRc;

	//if (gFarMode.bFarHookMode && gFarMode.bShellNoZoneCheck)
	//	lpExecInfo->fMask |= SEE_MASK_NOZONECHECKS;

	//gbInShellExecuteEx = TRUE;

	lbRc = F(ShellExecuteExA)(lpExecInfo);
	DWORD dwErr = GetLastError();

	//if (lbRc && gfGetProcessId && lpNew->hProcess)
	//{
	//	dwProcessID = gfGetProcessId(lpNew->hProcess);
	//}

	//#ifdef _DEBUG
	//
	//	if (lpNew->lpParameters)
	//	{
	//		DebugStringW(L"After ShellExecuteEx\n");
	//		DebugStringA(lpNew->lpParameters);
	//		DebugStringW(L"\n");
	//	}
	//
	//	if (lbRc && dwProcessID)
	//	{
	//		wchar_t szDbgMsg[128]; msprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"Process created: %u\n", dwProcessID);
	//		DebugStringW(szDbgMsg);
	//	}
	//
	//#endif
	//lpExecInfo->hProcess = lpNew->hProcess;
	//lpExecInfo->hInstApp = lpNew->hInstApp;
	sp->OnShellFinished(lbRc, lpExecInfo->hInstApp, lpExecInfo->hProcess);
	delete sp;

	//if (pszTempParm)
	//	free(pszTempParm);
	//if (pszTempApp)
	//	free(pszTempApp);
	//if (pszTempArg)
	//	free(pszTempArg);

	//free(lpNew);
	//gbInShellExecuteEx = FALSE;
	SetLastError(dwErr);
	return lbRc;
}


BOOL WINAPI OnShellExecuteExW(LPSHELLEXECUTEINFOW lpExecInfo)
{
	//typedef BOOL (WINAPI* OnShellExecuteExW_t)(LPSHELLEXECUTEINFOW lpExecInfo);
	ORIGINAL_EX(ShellExecuteExW);
	if (!F(ShellExecuteExW))
	{
		SetLastError(ERROR_INVALID_FUNCTION);
		return FALSE;
	}

	CShellProc* sp = new CShellProc();
	if (!sp || !sp->OnShellExecuteExW(&lpExecInfo))
	{
		delete sp;
		SetLastError(ERROR_FILE_NOT_FOUND);
		return FALSE;
	}

	BOOL lbRc = FALSE;

	lbRc = F(ShellExecuteExW)(lpExecInfo);
	DWORD dwErr = GetLastError();

	sp->OnShellFinished(lbRc, lpExecInfo->hInstApp, lpExecInfo->hProcess);
	delete sp;

	SetLastError(dwErr);
	return lbRc;
}


// Issue 1125: Hook undocumented function used for running commands typed in Windows 7 Win-menu search string.
HRESULT WINAPI OnShellExecCmdLine(HWND hwnd, LPCWSTR pwszCommand, LPCWSTR pwszStartDir, int nShow, LPVOID pUnused, DWORD dwSeclFlags)
{
	//typedef HRESULT (WINAPI* OnShellExecCmdLine_t)(HWND hwnd, LPCWSTR pwszCommand, LPCWSTR pwszStartDir, int nShow, LPVOID pUnused, DWORD dwSeclFlags);
	ORIGINAL_EX(ShellExecCmdLine);
	HRESULT hr = S_OK;

	// This is used from "Run" dialog too. We need to process command internally, because
	// otherwise Win can pass CREATE_SUSPENDED into CreateProcessW, so console will flicker.
	// From Win7 start menu: "cmd" and Ctrl+Shift+Enter - dwSeclFlags==0x79
	if (nShow && pwszCommand && pwszStartDir)
	{
		if (!IsBadStringPtrW(pwszCommand, MAX_PATH) && !IsBadStringPtrW(pwszStartDir, MAX_PATH))
		{
			hr = OurShellExecCmdLine(hwnd, pwszCommand, pwszStartDir, (dwSeclFlags & 0x40)==0x40, false);
			if (hr == (HRESULT)-1)
				goto ApiCall;
			else
				goto wrap;
		}
	}

ApiCall:
	if (!F(ShellExecCmdLine))
	{
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
	}
	else
	{
		hr = F(ShellExecCmdLine)(hwnd, pwszCommand, pwszStartDir, nShow, pUnused, dwSeclFlags);
	}

wrap:
	return hr;
}


HINSTANCE WINAPI OnShellExecuteA(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd)
{
	//typedef HINSTANCE(WINAPI* OnShellExecuteA_t)(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd);
	ORIGINAL_EX(ShellExecuteA);
	if (!F(ShellExecuteA))
	{
		SetLastError(ERROR_INVALID_FUNCTION);
		return FALSE;
	}

	if (ghConEmuWndDC)
	{
		if (!hwnd || hwnd == GetRealConsoleWindow())
			hwnd = ghConEmuWnd;
	}

	//gbInShellExecuteEx = TRUE;
	CShellProc* sp = new CShellProc();
	if (!sp || !sp->OnShellExecuteA(&lpOperation, &lpFile, &lpParameters, &lpDirectory, NULL, (DWORD*)&nShowCmd))
	{
		delete sp;
		SetLastError(ERROR_FILE_NOT_FOUND);
		return (HINSTANCE)ERROR_FILE_NOT_FOUND;
	}

	HINSTANCE lhRc;
	lhRc = F(ShellExecuteA)(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
	DWORD dwErr = GetLastError();

	sp->OnShellFinished(((INT_PTR)lhRc > 32), lhRc, NULL); //-V112
	delete sp;

	//gbInShellExecuteEx = FALSE;
	SetLastError(dwErr);
	return lhRc;
}


HINSTANCE WINAPI OnShellExecuteW(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd)
{
	//typedef HINSTANCE(WINAPI* OnShellExecuteW_t)(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd);
	ORIGINAL_EX(ShellExecuteW);
	if (!F(ShellExecuteW))
	{
		SetLastError(ERROR_INVALID_FUNCTION);
		return FALSE;
	}

	if (ghConEmuWndDC)
	{
		if (!hwnd || hwnd == GetRealConsoleWindow())
			hwnd = ghConEmuWnd;
	}

	//gbInShellExecuteEx = TRUE;
	CShellProc* sp = new CShellProc();
	if (!sp || !sp->OnShellExecuteW(&lpOperation, &lpFile, &lpParameters, &lpDirectory, NULL, (DWORD*)&nShowCmd))
	{
		delete sp;
		SetLastError(ERROR_FILE_NOT_FOUND);
		return (HINSTANCE)ERROR_FILE_NOT_FOUND;
	}

	HINSTANCE lhRc;
	lhRc = F(ShellExecuteW)(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
	DWORD dwErr = GetLastError();

	sp->OnShellFinished(((INT_PTR)lhRc > 32), lhRc, NULL); //-V112
	delete sp;

	//gbInShellExecuteEx = FALSE;
	SetLastError(dwErr);
	return lhRc;
}


// ssh (msysgit) crash issue. Need to know if thread was started by application but not remotely.
HANDLE WINAPI OnCreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId)
{
	//typedef HANDLE(WINAPI* OnCreateThread_t)(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);
	ORIGINAL_KRNL(CreateThread);
	DWORD nTemp = 0;
	LPDWORD pThreadID = lpThreadId ? lpThreadId : &nTemp;

	HANDLE hThread = F(CreateThread)(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, pThreadID);

	if (hThread)
	{
		// TRUE shows that thread was started by CreateThread functions
		gStartedThreads.Set(*pThreadID, TRUE);
	}

	return hThread;
}


// Required in VisualStudio and CodeBlocks (gdb) for DefTerm support while debugging
DWORD WINAPI OnResumeThread(HANDLE hThread)
{
	//typedef DWORD (WINAPI* OnResumeThread_t)(HANDLE);
	ORIGINAL_KRNL(ResumeThread);

	CShellProc::OnResumeDebugeeThreadCalled(hThread);

	DWORD nRc = F(ResumeThread)(hThread);

	return nRc;
}
