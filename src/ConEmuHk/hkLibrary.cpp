
/*
Copyright (c) 2015 Maximus5
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

#ifdef _DEBUG
	#define DebugString(x) //OutputDebugString(x)
#else
	#define DebugString(x) //OutputDebugString(x)
#endif

#include "../common/common.hpp"

#include "hkLibrary.h"
#include "hlpProcess.h"
#include "MainThread.h"

/* **************** */

struct UNICODE_STRING
{
  USHORT Length;
  USHORT MaximumLength;
  PWSTR  Buffer;
};
enum LDR_DLL_NOTIFICATION_REASON
{
	LDR_DLL_NOTIFICATION_REASON_LOADED = 1,
	LDR_DLL_NOTIFICATION_REASON_UNLOADED = 2,
};
struct LDR_DLL_LOADED_NOTIFICATION_DATA
{
	ULONG Flags;                    //Reserved.
	const UNICODE_STRING* FullDllName;   //The full path name of the DLL module.
	const UNICODE_STRING* BaseDllName;   //The base file name of the DLL module.
	PVOID DllBase;                  //A pointer to the base address for the DLL in memory.
	ULONG SizeOfImage;              //The size of the DLL image, in bytes.
};
struct LDR_DLL_UNLOADED_NOTIFICATION_DATA
{
	ULONG Flags;                    //Reserved.
	const UNICODE_STRING* FullDllName;   //The full path name of the DLL module.
	const UNICODE_STRING* BaseDllName;   //The base file name of the DLL module.
	PVOID DllBase;                  //A pointer to the base address for the DLL in memory.
	ULONG SizeOfImage;              //The size of the DLL image, in bytes.
};
union LDR_DLL_NOTIFICATION_DATA
{
	LDR_DLL_LOADED_NOTIFICATION_DATA Loaded;
	LDR_DLL_UNLOADED_NOTIFICATION_DATA Unloaded;
};
typedef VOID (CALLBACK* PLDR_DLL_NOTIFICATION_FUNCTION)(ULONG NotificationReason, const LDR_DLL_NOTIFICATION_DATA* NotificationData, PVOID Context);
VOID CALLBACK LdrDllNotification(ULONG NotificationReason, const LDR_DLL_NOTIFICATION_DATA* NotificationData, PVOID Context);
typedef NTSTATUS (NTAPI* LdrRegisterDllNotification_t)(ULONG Flags, PLDR_DLL_NOTIFICATION_FUNCTION NotificationFunction, PVOID Context, PVOID *Cookie);
typedef NTSTATUS (NTAPI* LdrUnregisterDllNotification_t)(PVOID Cookie);
static LdrRegisterDllNotification_t LdrRegisterDllNotification = NULL;
static LdrUnregisterDllNotification_t LdrUnregisterDllNotification = NULL;
static PVOID gpLdrDllNotificationCookie = NULL;
static NTSTATUS gnLdrDllNotificationState = (NTSTATUS)-1;
bool gbLdrDllNotificationUsed = false;


bool CheckLdrNotificationAvailable()
{
	static bool bLdrWasChecked = false;
	if (bLdrWasChecked)
		return gbLdrDllNotificationUsed;

	#ifndef _WIN32_WINNT_WIN8
	#define _WIN32_WINNT_WIN8 0x602
	#endif
	_ASSERTE(_WIN32_WINNT_WIN8==0x602);
	OSVERSIONINFOEXW osvi = {sizeof(osvi), HIBYTE(_WIN32_WINNT_WIN8), LOBYTE(_WIN32_WINNT_WIN8)};
	DWORDLONG const dwlConditionMask = VerSetConditionMask(VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_MINORVERSION, VER_GREATER_EQUAL);
	BOOL isAllowed = VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask);

	// LdrDllNotification работает так как нам надо начиная с Windows 8
	// В предыдущих версиях Windows нотификатор вызывается из LdrpFindOrMapDll
	// ДО того, как были обработаны импорты функцией LdrpProcessStaticImports (а точнее LdrpSnapThunk)

	if (isAllowed)
	{
		HMODULE hNtDll = GetModuleHandle(L"ntdll.dll");
		if (hNtDll)
		{
			LdrRegisterDllNotification = (LdrRegisterDllNotification_t)GetProcAddress(hNtDll, "LdrRegisterDllNotification");
			LdrUnregisterDllNotification = (LdrUnregisterDllNotification_t)GetProcAddress(hNtDll, "LdrUnregisterDllNotification");

			if (LdrRegisterDllNotification && LdrUnregisterDllNotification)
			{
				gnLdrDllNotificationState = LdrRegisterDllNotification(0, LdrDllNotification, NULL, &gpLdrDllNotificationCookie);
				gbLdrDllNotificationUsed = (gnLdrDllNotificationState == 0/*STATUS_SUCCESS*/);
			}
		}
	}

	bLdrWasChecked = true;

	return gbLdrDllNotificationUsed;
}

void UnregisterLdrNotification()
{
	if (gbLdrDllNotificationUsed)
	{
		_ASSERTEX(LdrUnregisterDllNotification!=NULL);
		LdrUnregisterDllNotification(gpLdrDllNotificationCookie);
	}
}


VOID CALLBACK LdrDllNotification(ULONG NotificationReason, const LDR_DLL_NOTIFICATION_DATA* NotificationData, PVOID Context)
{
	DWORD   dwSaveErrCode = GetLastError();
	wchar_t szModule[MAX_PATH*2] = L"";
	HMODULE hModule;

	const UNICODE_STRING* FullDllName;   //The full path name of the DLL module.
	const UNICODE_STRING* BaseDllName;   //The base file name of the DLL module.

	switch (NotificationReason)
	{
	case LDR_DLL_NOTIFICATION_REASON_LOADED:
		FullDllName = NotificationData->Loaded.FullDllName;
		BaseDllName = NotificationData->Loaded.BaseDllName;
		hModule = (HMODULE)NotificationData->Loaded.DllBase;
		break;
	case LDR_DLL_NOTIFICATION_REASON_UNLOADED:
		FullDllName = NotificationData->Unloaded.FullDllName;
		BaseDllName = NotificationData->Unloaded.BaseDllName;
		hModule = (HMODULE)NotificationData->Unloaded.DllBase;
		break;
	default:
		return;
	}

	if (FullDllName && FullDllName->Buffer)
		memmove(szModule, FullDllName->Buffer, min(sizeof(szModule)-2,FullDllName->Length));
	else if (BaseDllName && BaseDllName->Buffer)
		memmove(szModule, BaseDllName->Buffer, min(sizeof(szModule)-2,BaseDllName->Length));

	#ifdef _DEBUG
	wchar_t szDbgInfo[MAX_PATH*3];
	_wsprintf(szDbgInfo, SKIPLEN(countof(szDbgInfo)) L"ConEmuHk: Ldr(%s) " WIN3264TEST(L"0x%08X",L"0x%08X%08X") L" '%s'\n",
		(NotificationReason==LDR_DLL_NOTIFICATION_REASON_LOADED) ? L"Loaded" : L"Unload",
		WIN3264WSPRINT(hModule),
		szModule);
	DebugString(szDbgInfo);
	#endif

	switch (NotificationReason)
	{
	case LDR_DLL_NOTIFICATION_REASON_LOADED:
		if (PrepareNewModule(hModule, NULL, szModule, TRUE, TRUE))
		{
			HookItem* ph = NULL;
			GetOriginalAddress((LPVOID)OnLoadLibraryW, HOOK_FN_ID(LoadLibraryW), NULL, &ph, true);
			if (ph && ph->PostCallBack)
			{
				SETARGS1(&hModule,szModule);
				ph->PostCallBack(&args);
			}
		}
		break;
	case LDR_DLL_NOTIFICATION_REASON_UNLOADED:
		UnprepareModule(hModule, szModule, 0);
		UnprepareModule(hModule, szModule, 2);
		break;
	}

	SetLastError(dwSaveErrCode);
}

/* **************** */

#ifdef _DEBUG
void OnLoadLibraryLog(LPCSTR lpLibraryA, LPCWSTR lpLibraryW)
{
	#if 0
	if ((lpLibraryA && strncmp(lpLibraryA, "advapi32", 8)==0)
		|| (lpLibraryW && wcsncmp(lpLibraryW, L"advapi32", 8)==0))
	{
		extern HANDLE ghDebugSshLibs, ghDebugSshLibsRc;
		if (ghDebugSshLibs)
		{
			SetEvent(ghDebugSshLibs);
			WaitForSingleObject(ghDebugSshLibsRc, 1000);
		}
	}
	#endif
}
#else
#define OnLoadLibraryLog(lpLibraryA,lpLibraryW)
#endif

/* ************** */
HMODULE WINAPI OnLoadLibraryAWork(FARPROC lpfn, HookItem *ph, const char* lpFileName)
{
	typedef HMODULE(WINAPI* OnLoadLibraryA_t)(const char* lpFileName);
	OnLoadLibraryLog(lpFileName,NULL);
	HMODULE module = ((OnLoadLibraryA_t)lpfn)(lpFileName);
	DWORD dwLoadErrCode = GetLastError();

	// Issue 1079: Almost hangs with PHP
	if (lstrcmpiA(lpFileName, "kernel32.dll") == 0)
		return module;

	if (PrepareNewModule(module, lpFileName, NULL))
	{
		if (ph && ph->PostCallBack)
		{
			SETARGS1(&module,lpFileName);
			ph->PostCallBack(&args);
		}
	}

	SetLastError(dwLoadErrCode);
	return module;
}

HMODULE WINAPI OnLoadLibraryA(const char* lpFileName)
{
	//typedef HMODULE(WINAPI* OnLoadLibraryA_t)(const char* lpFileName);
	ORIGINAL(LoadLibraryA);
	return OnLoadLibraryAWork((FARPROC)F(LoadLibraryA), ph, lpFileName);
}

/* ************** */
HMODULE WINAPI OnLoadLibraryWWork(FARPROC lpfn, HookItem *ph, const wchar_t* lpFileName)
{
	typedef HMODULE(WINAPI* OnLoadLibraryW_t)(const wchar_t* lpFileName);
	HMODULE module = NULL;

	OnLoadLibraryLog(NULL,lpFileName);

	// Спрятать ExtendedConsole.dll с глаз долой, в сервисную папку "ConEmu"
	if (lpFileName 
		&& ((lstrcmpiW(lpFileName, L"ExtendedConsole.dll") == 0)
			|| lstrcmpiW(lpFileName, L"ExtendedConsole64.dll") == 0))
	{
		CESERVER_CONSOLE_MAPPING_HDR *Info = (CESERVER_CONSOLE_MAPPING_HDR*)calloc(1,sizeof(*Info));
		if (Info && ::LoadSrvMapping(ghConWnd, *Info))
		{
			size_t cchMax = countof(Info->ComSpec.ConEmuBaseDir)+64;
			wchar_t* pszFullPath = (wchar_t*)calloc(cchMax,sizeof(*pszFullPath));
			if (pszFullPath)
			{
				_wcscpy_c(pszFullPath, cchMax, Info->ComSpec.ConEmuBaseDir);
				_wcscat_c(pszFullPath, cchMax, WIN3264TEST(L"\\ExtendedConsole.dll",L"\\ExtendedConsole64.dll"));

				module = ((OnLoadLibraryW_t)lpfn)(pszFullPath);

				SafeFree(pszFullPath);
			}
		}
		SafeFree(Info);
	}

	if (!module)
		module = ((OnLoadLibraryW_t)lpfn)(lpFileName);
	DWORD dwLoadErrCode = GetLastError();

	if (gbLdrDllNotificationUsed)
		return module;

	// Issue 1079: Almost hangs with PHP
	if (lstrcmpi(lpFileName, L"kernel32.dll") == 0)
		return module;

	if (PrepareNewModule(module, NULL, lpFileName))
	{
		if (ph && ph->PostCallBack)
		{
			SETARGS1(&module,lpFileName);
			ph->PostCallBack(&args);
		}
	}

	SetLastError(dwLoadErrCode);
	return module;
}

HMODULE WINAPI OnLoadLibraryW(const wchar_t* lpFileName)
{
	//typedef HMODULE(WINAPI* OnLoadLibraryW_t)(const wchar_t* lpFileName);
	ORIGINAL(LoadLibraryW);
	return OnLoadLibraryWWork((FARPROC)F(LoadLibraryW), ph, lpFileName);
}

/* ************** */
HMODULE WINAPI OnLoadLibraryExAWork(FARPROC lpfn, HookItem *ph, const char* lpFileName, HANDLE hFile, DWORD dwFlags)
{
	typedef HMODULE(WINAPI* OnLoadLibraryExA_t)(const char* lpFileName, HANDLE hFile, DWORD dwFlags);
	OnLoadLibraryLog(lpFileName,NULL);
	HMODULE module = ((OnLoadLibraryExA_t)lpfn)(lpFileName, hFile, dwFlags);
	DWORD dwLoadErrCode = GetLastError();

	if (PrepareNewModule(module, lpFileName, NULL))
	{
		if (ph && ph->PostCallBack)
		{
			SETARGS3(&module,lpFileName,hFile,dwFlags);
			ph->PostCallBack(&args);
		}
	}

	SetLastError(dwLoadErrCode);
	return module;
}

HMODULE WINAPI OnLoadLibraryExA(const char* lpFileName, HANDLE hFile, DWORD dwFlags)
{
	//typedef HMODULE(WINAPI* OnLoadLibraryExA_t)(const char* lpFileName, HANDLE hFile, DWORD dwFlags);
	ORIGINAL(LoadLibraryExA);
	return OnLoadLibraryExAWork((FARPROC)F(LoadLibraryExA), ph, lpFileName, hFile, dwFlags);
}

/* ************** */
HMODULE WINAPI OnLoadLibraryExWWork(FARPROC lpfn, HookItem *ph, const wchar_t* lpFileName, HANDLE hFile, DWORD dwFlags)
{
	typedef HMODULE(WINAPI* OnLoadLibraryExW_t)(const wchar_t* lpFileName, HANDLE hFile, DWORD dwFlags);
	OnLoadLibraryLog(NULL,lpFileName);
	HMODULE module = ((OnLoadLibraryExW_t)lpfn)(lpFileName, hFile, dwFlags);
	DWORD dwLoadErrCode = GetLastError();

	if (PrepareNewModule(module, NULL, lpFileName))
	{
		if (ph && ph->PostCallBack)
		{
			SETARGS3(&module,lpFileName,hFile,dwFlags);
			ph->PostCallBack(&args);
		}
	}

	SetLastError(dwLoadErrCode);
	return module;
}

HMODULE WINAPI OnLoadLibraryExW(const wchar_t* lpFileName, HANDLE hFile, DWORD dwFlags)
{
	//typedef HMODULE(WINAPI* OnLoadLibraryExW_t)(const wchar_t* lpFileName, HANDLE hFile, DWORD dwFlags);
	ORIGINAL(LoadLibraryExW);
	return OnLoadLibraryExWWork((FARPROC)F(LoadLibraryExW), ph, lpFileName, hFile, dwFlags);
}

BOOL WINAPI OnFreeLibraryWork(FARPROC lpfn, HookItem *ph, HMODULE hModule)
{
	typedef BOOL (WINAPI* OnFreeLibrary_t)(HMODULE hModule);
	BOOL lbRc = FALSE;
	BOOL lbResource = LDR_IS_RESOURCE(hModule);
	// lbResource получается TRUE например при вызовах из version.dll

	UnprepareModule(hModule, NULL, 0);

#ifdef _DEBUG
	BOOL lbModulePre = IsModuleValid(hModule); // GetModuleFileName(hModule, szModule, countof(szModule));
#endif

	// Section locking is inadmissible. One FreeLibrary may cause another FreeLibrary in _different_ thread.
	lbRc = ((OnFreeLibrary_t)lpfn)(hModule);
	DWORD dwFreeErrCode = GetLastError();

	// Далее только если !LDR_IS_RESOURCE
	if (lbRc && !lbResource)
		UnprepareModule(hModule, NULL, 1);

	SetLastError(dwFreeErrCode);
	return lbRc;
}

BOOL WINAPI OnFreeLibrary(HMODULE hModule)
{
	//typedef BOOL (WINAPI* OnFreeLibrary_t)(HMODULE hModule);
	ORIGINALFAST(FreeLibrary);
	return OnFreeLibraryWork((FARPROC)F(FreeLibrary), ph, hModule);
}
