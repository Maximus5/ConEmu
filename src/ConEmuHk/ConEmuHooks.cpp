
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


#define DROP_SETCP_ON_WIN2K3R2

// Иначе не опередяется GetConsoleAliases (хотя он должен быть доступен в Win2k)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501

#define DEFINE_HOOK_MACROS

#include <windows.h>
#include <TCHAR.h>
#include <Tlhelp32.h>
#include <shlwapi.h>
#include "..\common\common.hpp"
#include "..\common\ConEmuCheck.h"
#include "..\common\SetHook.h"

#include <WinInet.h>
#pragma comment(lib, "wininet.lib")

#define DebugString(x) // OutputDebugString(x)

//#ifdef _DEBUG
//	#include <crtdbg.h>
//#else
//	#ifndef _ASSERTE
//		#define _ASSERTE(x)
//	#endif
//#endif

extern HMODULE ghHookOurModule; // Хэндл нашей dll'ки (здесь хуки не ставятся)
extern DWORD   gnHookMainThreadId;
extern BOOL    gbHooksTemporaryDisabled;

/* ************ Globals for SetHook ************ */
HWND ghConEmuDC = NULL; // Содержит хэндл окна отрисовки. Это ДОЧЕРНЕЕ окно.
HWND ghConsoleHwnd = NULL;
BOOL gbFARuseASCIIsort = FALSE;
DWORD gdwServerPID = 0;
BOOL gbShellNoZoneCheck = FALSE;
/* ************ Globals for SetHook ************ */


static TCHAR kernel32[] = _T("kernel32.dll");
static TCHAR user32[]   = _T("user32.dll");
static TCHAR shell32[]  = _T("shell32.dll");
static TCHAR wininet[]  = _T("wininet.dll");

//static BOOL bHooksWin2k3R2Only = FALSE;
//static HookItem HooksWin2k3R2Only[] = {
//	{OnSetConsoleCP, "SetConsoleCP", kernel32, 0},
//	{OnSetConsoleOutputCP, "SetConsoleOutputCP", kernel32, 0},
//	/* ************************ */
//	{0, 0, 0}
//};

extern int WINAPI OnCompareStringW(LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2);
//
extern BOOL WINAPI OnHttpSendRequestA(LPVOID hRequest, LPCSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength);
extern BOOL WINAPI OnHttpSendRequestW(LPVOID hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength);

static HookItem HooksFarOnly[] = {
//	{OnlstrcmpiA,      "lstrcmpiA",      kernel32, 0},
	{(void*)OnCompareStringW, "CompareStringW", kernel32, 0},
	/* ************************ */
	{(void*)OnHttpSendRequestA, "HttpSendRequestA", wininet, 0},
	{(void*)OnHttpSendRequestW, "HttpSendRequestW", wininet, 0},
	/* ************************ */
	{0, 0, 0}
};


// Forward declarations of the hooks
// Common hooks (except LoadLibrary..., FreeLibrary, GetProcAddress)
static BOOL WINAPI OnTrackPopupMenu(HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, CONST RECT * prcRect);
static BOOL WINAPI OnTrackPopupMenuEx(HMENU hmenu, UINT fuFlags, int x, int y, HWND hWnd, LPTPMPARAMS lptpm);
static BOOL WINAPI OnShellExecuteExA(LPSHELLEXECUTEINFOA lpExecInfo);
static BOOL WINAPI OnShellExecuteExW(LPSHELLEXECUTEINFOW lpExecInfo);
static HINSTANCE WINAPI OnShellExecuteA(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd);
static HINSTANCE WINAPI OnShellExecuteW(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd);
static BOOL WINAPI OnFlashWindow(HWND hWnd, BOOL bInvert);
static BOOL WINAPI OnFlashWindowEx(PFLASHWINFO pfwi);
static BOOL WINAPI OnSetForegroundWindow(HWND hWnd);
int WINAPI OnCompareStringW(LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2);
static DWORD WINAPI OnGetConsoleAliasesW(LPWSTR AliasBuffer, DWORD AliasBufferLength, LPWSTR ExeName);
static BOOL WINAPI OnGetNumberOfConsoleInputEvents(HANDLE hConsoleInput, LPDWORD lpcNumberOfEvents);
static BOOL WINAPI OnPeekConsoleInputA(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
static BOOL WINAPI OnPeekConsoleInputW(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
static BOOL WINAPI OnReadConsoleInputA(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
static BOOL WINAPI OnReadConsoleInputW(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
static HANDLE WINAPI OnCreateConsoleScreenBuffer(DWORD dwDesiredAccess, DWORD dwShareMode, const SECURITY_ATTRIBUTES *lpSecurityAttributes, DWORD dwFlags, LPVOID lpScreenBufferData);
static BOOL WINAPI OnAllocConsole(void);
static BOOL WINAPI OnFreeConsole(void);
static HWND WINAPI OnGetConsoleWindow(void); // в фаре дофига и больше вызовов этой функции
static BOOL WINAPI OnWriteConsoleOutputA(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
static BOOL WINAPI OnWriteConsoleOutputW(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
static BOOL WINAPI OnGetWindowRect(HWND hWnd, LPRECT lpRect);
static BOOL WINAPI OnScreenToClient(HWND hWnd, LPPOINT lpPoint);
static BOOL WINAPI OnCreateProcessA(LPCSTR lpApplicationName,  LPSTR lpCommandLine,  LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory,  LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
static BOOL WINAPI OnCreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);


static HookItem HooksCommon[] = {
	/* ***** MOST CALLED ***** */
//	{(void*)OnGetConsoleWindow,     "GetConsoleWindow",     kernel32}, -- пока смысла нет. инжекты еще не на старте ставятся
	{(void*)OnWriteConsoleOutputW,  "WriteConsoleOutputW",  kernel32},
	{(void*)OnWriteConsoleOutputA,  "WriteConsoleOutputA",  kernel32},
	/* ************************ */
	{(void*)OnCreateProcessA,		"CreateProcessA",		kernel32},
	{(void*)OnCreateProcessW,		"CreateProcessW",		kernel32},
	/* ************************ */
	{(void*)OnPeekConsoleInputW,	"PeekConsoleInputW",	kernel32},
	{(void*)OnPeekConsoleInputA,	"PeekConsoleInputA",	kernel32},
	{(void*)OnReadConsoleInputW,	"ReadConsoleInputW",	kernel32},
	{(void*)OnReadConsoleInputA,	"ReadConsoleInputA",	kernel32},
	{(void*)OnGetConsoleAliasesW,	"GetConsoleAliasesW",	kernel32},
	{(void*)OnAllocConsole,			"AllocConsole",			kernel32},
	{(void*)OnFreeConsole,			"FreeConsole",			kernel32},
	{(void*)OnGetNumberOfConsoleInputEvents,
									"GetNumberOfConsoleInputEvents",
															kernel32},
	{(void*)OnCreateConsoleScreenBuffer,
									"CreateConsoleScreenBuffer",
															kernel32},
	/* ************************ */
	{(void*)OnTrackPopupMenu,		"TrackPopupMenu",		user32},
	{(void*)OnTrackPopupMenuEx,		"TrackPopupMenuEx",		user32},
	{(void*)OnFlashWindow,			"FlashWindow",			user32},
	{(void*)OnFlashWindowEx,		"FlashWindowEx",			user32},
	{(void*)OnSetForegroundWindow,	"SetForegroundWindow",	user32},
	{(void*)OnGetWindowRect,		"GetWindowRect",			user32},
	{(void*)OnScreenToClient,		"ScreenToClient",		user32},
	/* ************************ */
	{(void*)OnShellExecuteExA,		"ShellExecuteExA",		shell32},
	{(void*)OnShellExecuteExW,		"ShellExecuteExW",		shell32},
	{(void*)OnShellExecuteA,		"ShellExecuteA",			shell32},
	{(void*)OnShellExecuteW,		"ShellExecuteW",			shell32},
	/* ************************ */
	{0}
};



// Эту функцию нужно позвать из DllMain
BOOL StartupHooks(HMODULE ahOurDll)
{
	// Основные хуки
	InitHooks( HooksCommon );
	// Проверить, фар ли это?
	InitHooks( HooksFarOnly );
	// Реестр
	TODO("Hook registry");
	
	return SetAllHooks(ahOurDll);
}


void ShutdownHooks()
{
	UnsetAllHooks();
}



// Forward
static void GuiSetForeground(HWND hWnd);
static void GuiFlashWindow(BOOL bSimple, HWND hWnd, BOOL bInvert, DWORD dwFlags, UINT uCount, DWORD dwTimeout);


typedef BOOL (WINAPI* OnCreateProcessA_t)(LPCSTR lpApplicationName,  LPSTR lpCommandLine,  LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory,  LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
static BOOL WINAPI OnCreateProcessA(LPCSTR lpApplicationName,  LPSTR lpCommandLine,  LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory,  LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
	ORIGINALFAST(CreateProcessA);

	BOOL lbRc;
	
	lbRc = F(CreateProcessA)(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	
	return lbRc;
}
typedef BOOL (WINAPI* OnCreateProcessW_t)(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
static BOOL WINAPI OnCreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
	ORIGINALFAST(CreateProcessW);
	BOOL bMainThread = FALSE; // поток не важен

	BOOL lbRc = FALSE;

	if (ph && ph->PreCallBack) {
		SETARGS10(&lbRc, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	lbRc = F(CreateProcessW)(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	
    if (ph && ph->PostCallBack) {
    	SETARGS10(&lbRc, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
    	ph->PostCallBack(&args);
    }

	return lbRc;
}


typedef BOOL (WINAPI* OnTrackPopupMenu_t)(HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, CONST RECT * prcRect);
static BOOL WINAPI OnTrackPopupMenu(HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, CONST RECT * prcRect)
{
	ORIGINALFAST(TrackPopupMenu);
	
	#ifdef _DEBUG
		WCHAR szMsg[128]; wsprintf(szMsg, L"TrackPopupMenu(hwnd=0x%08X)\n", (DWORD)hWnd);
		DebugString(szMsg);
	#endif
	
	if (ghConEmuDC)
	{
		GuiSetForeground(hWnd);
	}
	
	BOOL lbRc;
	
	lbRc = F(TrackPopupMenu)(hMenu, uFlags, x, y, nReserved, hWnd, prcRect);
	
	return lbRc;
}

typedef BOOL (WINAPI* OnTrackPopupMenuEx_t)(HMENU hmenu, UINT fuFlags, int x, int y, HWND hWnd, LPTPMPARAMS lptpm);
static BOOL WINAPI OnTrackPopupMenuEx(HMENU hmenu, UINT fuFlags, int x, int y, HWND hWnd, LPTPMPARAMS lptpm)
{
	ORIGINALFAST(TrackPopupMenuEx);
	
	#ifdef _DEBUG
		WCHAR szMsg[128]; wsprintf(szMsg, L"TrackPopupMenuEx(hwnd=0x%08X)\n", (DWORD)hWnd);
		DebugString(szMsg);
	#endif
	
	if (ghConEmuDC) {
		GuiSetForeground(hWnd);
	}
	
	BOOL lbRc;
	
	lbRc = F(TrackPopupMenuEx)(hmenu, fuFlags, x, y, hWnd, lptpm);
	
	return lbRc;
}

#ifndef SEE_MASK_NOZONECHECKS
#define SEE_MASK_NOZONECHECKS      0x00800000
#endif

typedef BOOL (WINAPI* OnShellExecuteExA_t)(LPSHELLEXECUTEINFOA lpExecInfo);
static BOOL WINAPI OnShellExecuteExA(LPSHELLEXECUTEINFOA lpExecInfo)
{
	ORIGINALFAST(ShellExecuteExA);

	if (ghConEmuDC) {
		if (!lpExecInfo->hwnd || lpExecInfo->hwnd == GetConsoleWindow())
			lpExecInfo->hwnd = GetParent(ghConEmuDC);
	}
	
	BOOL lbRc;
	
	if (gbShellNoZoneCheck)
		lpExecInfo->fMask |= SEE_MASK_NOZONECHECKS;

	lbRc = F(ShellExecuteExA)(lpExecInfo);
	
	return lbRc;
}
typedef BOOL (WINAPI* OnShellExecuteExW_t)(LPSHELLEXECUTEINFOW lpExecInfo);
static BOOL OnShellExecuteExW_SEH(OnShellExecuteExW_t f, LPSHELLEXECUTEINFOW lpExecInfo, BOOL* pbRc)
{
	BOOL lbOk = FALSE;
	SAFETRY {
		*pbRc = f(lpExecInfo);
		lbOk = TRUE;
	} SAFECATCH {
		lbOk = FALSE;
	}
	return lbOk;
}
static BOOL WINAPI OnShellExecuteExW(LPSHELLEXECUTEINFOW lpExecInfo)
{
	ORIGINAL(ShellExecuteExW);

	if (ghConEmuDC) {
		if (!lpExecInfo->hwnd || lpExecInfo->hwnd == GetConsoleWindow())
			lpExecInfo->hwnd = GetParent(ghConEmuDC);
	}

	BOOL lbRc;

	if (gbShellNoZoneCheck)
		lpExecInfo->fMask |= SEE_MASK_NOZONECHECKS;

	//BUGBUG: FAR периодически валится на этой функции
	//должно быть: lpExecInfo->cbSize==0x03C; lpExecInfo->fMask==0x00800540;
	if (ph && ph->ExceptCallBack)
	{
		if (!OnShellExecuteExW_SEH(F(ShellExecuteExW), lpExecInfo, &lbRc))
		{
			SETARGS1(&lbRc,lpExecInfo);
			ph->ExceptCallBack(&args);
		}
	}
	else
	{
		lbRc = F(ShellExecuteExW)(lpExecInfo);
	}

	return lbRc;
}

typedef HINSTANCE (WINAPI* OnShellExecuteA_t)(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd);
static HINSTANCE WINAPI OnShellExecuteA(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd)
{
	ORIGINALFAST(ShellExecuteA);
	
	if (ghConEmuDC) {
		if (!hwnd || hwnd == GetConsoleWindow())
			hwnd = GetParent(ghConEmuDC);
	}
	
	HINSTANCE lhRc;
	
	lhRc = F(ShellExecuteA)(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
	
	return lhRc;
}
typedef HINSTANCE (WINAPI* OnShellExecuteW_t)(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd);
static HINSTANCE WINAPI OnShellExecuteW(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd)
{
	ORIGINALFAST(ShellExecuteW);
	
	if (ghConEmuDC) {
		if (!hwnd || hwnd == GetConsoleWindow())
			hwnd = GetParent(ghConEmuDC);
	}
	
	HINSTANCE lhRc;
	
	lhRc = F(ShellExecuteW)(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
	
	return lhRc;
}

typedef BOOL (WINAPI* OnFlashWindow_t)(HWND hWnd, BOOL bInvert);
static BOOL WINAPI OnFlashWindow(HWND hWnd, BOOL bInvert)
{
	ORIGINALFAST(FlashWindow);

	if (ghConEmuDC) {
		GuiFlashWindow(TRUE, hWnd, bInvert, 0,0,0);
		return TRUE;
	}
	
	BOOL lbRc;
	
	lbRc = F(FlashWindow)(hWnd, bInvert);
	
	return lbRc;
}
typedef BOOL (WINAPI* OnFlashWindowEx_t)(PFLASHWINFO pfwi);
static BOOL WINAPI OnFlashWindowEx(PFLASHWINFO pfwi)
{
	ORIGINALFAST(FlashWindowEx);

	if (ghConEmuDC) {
		GuiFlashWindow(FALSE, pfwi->hwnd, 0, pfwi->dwFlags, pfwi->uCount, pfwi->dwTimeout);
		return TRUE;
	}
	
	BOOL lbRc;
	
	lbRc = F(FlashWindowEx)(pfwi);
	
	return lbRc;
}

typedef BOOL (WINAPI* OnSetForegroundWindow_t)(HWND hWnd);
static BOOL WINAPI OnSetForegroundWindow(HWND hWnd)
{
	ORIGINALFAST(SetForegroundWindow);
	
	if (ghConEmuDC) {
		GuiSetForeground(hWnd);
	}
	
	BOOL lbRc;
	
	lbRc = F(SetForegroundWindow)(hWnd);
	
	return lbRc;
}

typedef BOOL (WINAPI* OnGetWindowRect_t)(HWND hWnd, LPRECT lpRect);
static BOOL WINAPI OnGetWindowRect(HWND hWnd, LPRECT lpRect)
{
	ORIGINALFAST(GetWindowRect);

	BOOL lbRc;

	if (hWnd == ghConsoleHwnd && ghConEmuDC)
	{
		//EMenu gui mode issues (center in window). "Remove" Far window from mouse cursor.
		hWnd = ghConEmuDC;
	}

	lbRc = F(GetWindowRect)(hWnd, lpRect);

	//if (ghConEmuDC && lpRect)
	//{
	//	//EMenu text mode issues. "Remove" Far window from mouse cursor.
	//	POINT ptCur = {0};
	//	GetCursorPos(&ptCur);
	//	lpRect->left += ptCur.x;
	//	lpRect->right += ptCur.x;
	//	lpRect->top += ptCur.y;
	//	lpRect->bottom += ptCur.y;
	//}

	return lbRc;
}

typedef BOOL (WINAPI* OnScreenToClient_t)(HWND hWnd, LPPOINT lpPoint);
static BOOL WINAPI OnScreenToClient(HWND hWnd, LPPOINT lpPoint)
{
	ORIGINALFAST(ScreenToClient);

	BOOL lbRc;

	lbRc = F(ScreenToClient)(hWnd, lpPoint);

	if (ghConEmuDC && lpPoint && hWnd == ghConsoleHwnd)
	{
		//EMenu text mode issues. "Remove" Far window from mouse cursor.
		lpPoint->x = lpPoint->y = -1;
	}

	return lbRc;
}

#ifndef NORM_STOP_ON_NULL
#define NORM_STOP_ON_NULL 0x10000000
#endif

// ANSI хукать смысла нет, т.к. FAR 1.x сравнивает сам
typedef int (WINAPI* OnCompareStringW_t)(LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2);
int WINAPI OnCompareStringW(LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2)
{
	ORIGINALFAST(CompareStringW);
	int nCmp = -1;
	
	if (gbFARuseASCIIsort)
	{
		if (dwCmpFlags == (NORM_IGNORECASE|NORM_STOP_ON_NULL|SORT_STRINGSORT)) {
			int n = 0;
			WCHAR ch1 = *lpString1++, /*ch10 = 0,*/ ch2 = *lpString2++ /*,ch20 = 0*/;
			int n1 = (cchCount1==cchCount2) ? cchCount1
				: (cchCount1!=-1 && cchCount2!=-1) ? -1 
					: (cchCount1!=-1) ? cchCount2 
						: (cchCount2!=-1) ? cchCount1 : min(cchCount1,cchCount2);
			while (!n && /*(ch1=*lpString1++) && (ch2=*lpString2++) &&*/ n1--)
			{
				if (ch1==ch2) {
					if (!ch1) break;
					// n = 0;
				} else if (ch1<0x80 && ch2<0x80) {
					if (ch1>=L'A' && ch1<=L'Z') ch1 |= 0x20;
					if (ch2>=L'A' && ch2<=L'Z') ch2 |= 0x20;
					n = (ch1==ch2) ? 0 : (ch1<ch2) ? -1 : 1;
				} else {
					n = CompareStringW(Locale, dwCmpFlags, &ch1, 1, &ch2, 1)-2;
				}
				if (!ch1 || !ch2 || !n1) break;
				ch1 = *lpString1++;
				ch2 = *lpString2++;
			}

			nCmp = (n<0) ? 1 : (n>0) ? 3 : 2;
		}
	}
	
	if (nCmp == -1)
		nCmp = F(CompareStringW)(Locale, dwCmpFlags, lpString1, cchCount1, lpString2, cchCount2);

	return nCmp;
}

typedef DWORD (WINAPI* OnGetConsoleAliasesW_t)(LPWSTR AliasBuffer, DWORD AliasBufferLength, LPWSTR ExeName);
static DWORD WINAPI OnGetConsoleAliasesW(LPWSTR AliasBuffer, DWORD AliasBufferLength, LPWSTR ExeName)
{
	ORIGINALFAST(GetConsoleAliasesW);
	DWORD nError = 0;
	DWORD nRc = F(GetConsoleAliasesW)(AliasBuffer,AliasBufferLength,ExeName);
	if (!nRc) {
		nError = GetLastError();
		// финт ушами
		if (nError == ERROR_NOT_ENOUGH_MEMORY && gdwServerPID) {
			CESERVER_REQ_HDR In;
			ExecutePrepareCmd((CESERVER_REQ*)&In, CECMD_GETALIASES,sizeof(CESERVER_REQ_HDR));
			CESERVER_REQ* pOut = ExecuteSrvCmd(gdwServerPID, (CESERVER_REQ*)&In, GetConsoleWindow());
			if (pOut) {
				DWORD nData = min(AliasBufferLength,(pOut->hdr.cbSize-sizeof(pOut->hdr)));
				if (nData) {
					memmove(AliasBuffer, pOut->Data, nData);
					nRc = TRUE;
				}
				ExecuteFreeResult(pOut);
			}
			if (!nRc)
				SetLastError(nError); // вернуть, вдруг какая функция его поменяла
		}
	}
	return nRc;
}

//static BOOL WINAPI OnSetConsoleCP(UINT wCodePageID)
//{
//	_ASSERTE(OnSetConsoleCP!=SetConsoleCP);
//	TODO("Виснет в 2k3R2 при 'chcp 866 <enter> chcp 20866 <enter>");
//	BOOL lbRc = FALSE;
//	if (gdwServerPID) {
//		lbRc = SrvSetConsoleCP(FALSE/*bSetOutputCP*/, wCodePageID);
//	} else {
//		lbRc = SetConsoleCP(wCodePageID);
//	}
//	return lbRc;
//}
//
//static BOOL WINAPI OnSetConsoleOutputCP(UINT wCodePageID)
//{
//	_ASSERTE(OnSetConsoleOutputCP!=SetConsoleOutputCP);
//	BOOL lbRc = FALSE;
//	if (gdwServerPID) {
//		lbRc = SrvSetConsoleCP(TRUE/*bSetOutputCP*/, wCodePageID);
//	} else {
//		lbRc = SetConsoleOutputCP(wCodePageID);
//	}
//	return lbRc;
//}

typedef BOOL (WINAPI* OnGetNumberOfConsoleInputEvents_t)(HANDLE hConsoleInput, LPDWORD lpcNumberOfEvents);
static BOOL WINAPI OnGetNumberOfConsoleInputEvents(HANDLE hConsoleInput, LPDWORD lpcNumberOfEvents)
{
	ORIGINAL(GetNumberOfConsoleInputEvents);
	BOOL lbRc = FALSE;

	//if (gpFarInfo && bMainThread)	
	//	TouchReadPeekConsoleInputs();
		
	if (ph && ph->PreCallBack) {
		SETARGS2(&lbRc,hConsoleInput,lpcNumberOfEvents);
		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!ph->PreCallBack(&args))
			return lbRc;
	}
	
	lbRc = F(GetNumberOfConsoleInputEvents)(hConsoleInput, lpcNumberOfEvents);
	
	if (ph && ph->PostCallBack) {
		SETARGS2(&lbRc,hConsoleInput,lpcNumberOfEvents);
		ph->PostCallBack(&args);
	}
	
	return lbRc;
}

typedef BOOL (WINAPI* OnPeekConsoleInputA_t)(HANDLE,PINPUT_RECORD,DWORD,LPDWORD);
static BOOL WINAPI OnPeekConsoleInputA(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
	ORIGINAL(PeekConsoleInputA);
	
	//if (gpFarInfo && bMainThread)
	//	TouchReadPeekConsoleInputs(1);
		
	BOOL lbRc = FALSE;

	if (ph && ph->PreCallBack) {
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);
		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	lbRc = F(PeekConsoleInputA)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);
	
	if (ph && ph->PostCallBack) {
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);
		ph->PostCallBack(&args);
	}
	
	return lbRc;
}

typedef BOOL (WINAPI* OnPeekConsoleInputW_t)(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
static BOOL WINAPI OnPeekConsoleInputW(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
	ORIGINAL(PeekConsoleInputW);
	
	//if (gpFarInfo && bMainThread)
	//	TouchReadPeekConsoleInputs(1);
		
	BOOL lbRc = FALSE;

	if (ph && ph->PreCallBack) {
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);
		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	lbRc = F(PeekConsoleInputW)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);

	if (ph && ph->PostCallBack) {
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);
		ph->PostCallBack(&args);
	}

	return lbRc;
}

typedef BOOL (WINAPI* OnReadConsoleInputA_t)(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
static BOOL WINAPI OnReadConsoleInputA(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
	ORIGINAL(ReadConsoleInputA);
	
	//if (gpFarInfo && bMainThread)
	//	TouchReadPeekConsoleInputs(0);
		
	BOOL lbRc = FALSE;

	if (ph && ph->PreCallBack) {
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);
		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	lbRc = F(ReadConsoleInputA)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);

	if (ph && ph->PostCallBack) {
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);
		ph->PostCallBack(&args);
	}

	return lbRc;
}

typedef BOOL (WINAPI* OnReadConsoleInputW_t)(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
static BOOL WINAPI OnReadConsoleInputW(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
	ORIGINAL(ReadConsoleInputW);
	
	//if (gpFarInfo && bMainThread)
	//	TouchReadPeekConsoleInputs(0);

	BOOL lbRc = FALSE;

	if (ph && ph->PreCallBack) {
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);
		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	lbRc = F(ReadConsoleInputW)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);

	if (ph && ph->PostCallBack) {
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);
		ph->PostCallBack(&args);
	}

	return lbRc;
}

typedef HANDLE (WINAPI* OnCreateConsoleScreenBuffer_t)(DWORD dwDesiredAccess, DWORD dwShareMode, const SECURITY_ATTRIBUTES *lpSecurityAttributes, DWORD dwFlags, LPVOID lpScreenBufferData);
static HANDLE WINAPI OnCreateConsoleScreenBuffer(DWORD dwDesiredAccess, DWORD dwShareMode, const SECURITY_ATTRIBUTES *lpSecurityAttributes, DWORD dwFlags, LPVOID lpScreenBufferData)
{
	ORIGINALFAST(CreateConsoleScreenBuffer);
	
	if ((dwShareMode & (FILE_SHARE_READ|FILE_SHARE_WRITE)) != (FILE_SHARE_READ|FILE_SHARE_WRITE))
		dwShareMode |= (FILE_SHARE_READ|FILE_SHARE_WRITE);

	HANDLE h;
	h = F(CreateConsoleScreenBuffer)(dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwFlags, lpScreenBufferData);
	return h;
}

typedef BOOL (WINAPI* OnAllocConsole_t)(void);
static BOOL WINAPI OnAllocConsole(void)
{
	ORIGINALFAST(AllocConsole);
	BOOL bMainThread = (GetCurrentThreadId() == gnHookMainThreadId);

	BOOL lbRc = FALSE;

	if (ph && ph->PreCallBack) {
		SETARGS(&lbRc);
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	lbRc = F(AllocConsole)();

	if (ph && ph->PostCallBack) {
		SETARGS(&lbRc);
		ph->PostCallBack(&args);
	}

	return lbRc;
}

typedef BOOL (WINAPI* OnFreeConsole_t)(void);
static BOOL WINAPI OnFreeConsole(void)
{
	ORIGINALFAST(FreeConsole);
	BOOL bMainThread = (GetCurrentThreadId() == gnHookMainThreadId);

	BOOL lbRc = FALSE;

	if (ph && ph->PreCallBack) {
		SETARGS(&lbRc);
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	lbRc = F(FreeConsole)();

	if (ph && ph->PostCallBack) {
		SETARGS(&lbRc);
		ph->PostCallBack(&args);
	}

	return lbRc;
}

typedef HWND (WINAPI* OnGetConsoleWindow_t)(void);
static HWND WINAPI OnGetConsoleWindow(void)
{
	ORIGINALFAST(GetConsoleWindow);

	if (ghConEmuDC && ghConsoleHwnd) {
		return ghConsoleHwnd;
	}

	HWND h;

	h = F(GetConsoleWindow)();

	return h;
}

typedef BOOL (WINAPI* OnWriteConsoleOutputA_t)(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
static BOOL WINAPI OnWriteConsoleOutputA(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion)
{
	ORIGINAL(WriteConsoleOutputA);

	BOOL lbRc = FALSE;

	if (ph && ph->PreCallBack) {
		SETARGS5(&lbRc, hConsoleOutput, lpBuffer, &dwBufferSize, &dwBufferCoord, lpWriteRegion);
		ph->PreCallBack(&args);
	}

	lbRc = F(WriteConsoleOutputA)(hConsoleOutput, lpBuffer, dwBufferSize, dwBufferCoord, lpWriteRegion);

	if (ph && ph->PostCallBack) {
		SETARGS5(&lbRc, hConsoleOutput, lpBuffer, &dwBufferSize, &dwBufferCoord, lpWriteRegion);
		ph->PostCallBack(&args);
	}

	return lbRc;
}

typedef BOOL (WINAPI* OnWriteConsoleOutputW_t)(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
static BOOL WINAPI OnWriteConsoleOutputW(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion)
{
	ORIGINAL(WriteConsoleOutputW);

	BOOL lbRc = FALSE;

	if (ph && ph->PreCallBack) {
		SETARGS5(&lbRc, hConsoleOutput, lpBuffer, &dwBufferSize, &dwBufferCoord, lpWriteRegion);
		ph->PreCallBack(&args);
	}

	lbRc = F(WriteConsoleOutputW)(hConsoleOutput, lpBuffer, dwBufferSize, dwBufferCoord, lpWriteRegion);

	if (ph && ph->PostCallBack) {
		SETARGS5(&lbRc, hConsoleOutput, lpBuffer, &dwBufferSize, &dwBufferCoord, lpWriteRegion);
		ph->PostCallBack(&args);
	}

	return lbRc;
}



// WinInet.dll
typedef BOOL (WINAPI* OnHttpSendRequestA_t)(LPVOID hRequest, LPCSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength);
typedef BOOL (WINAPI* OnHttpSendRequestW_t)(LPVOID hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength);
// смысла нет - __try не помогает
//static BOOL OnHttpSendRequestA_SEH(OnHttpSendRequestA_t f, LPVOID hRequest, LPCSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength, BOOL* pbRc)
//{
//	BOOL lbOk = FALSE;
//	SAFETRY {
//		*pbRc = f(hRequest, lpszHeaders, dwHeadersLength, lpOptional, dwOptionalLength);
//		lbOk = TRUE;
//	} SAFECATCH {
//		lbOk = FALSE;
//	}
//	return lbOk;
//}
//static BOOL OnHttpSendRequestW_SEH(OnHttpSendRequestW_t f, LPVOID hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength, BOOL* pbRc)
//{
//	BOOL lbOk = FALSE;
//	SAFETRY {
//		*pbRc = f(hRequest, lpszHeaders, dwHeadersLength, lpOptional, dwOptionalLength);
//		lbOk = TRUE;
//	} SAFECATCH {
//		lbOk = FALSE;
//	}
//	return lbOk;
//}
BOOL WINAPI OnHttpSendRequestA(LPVOID hRequest, LPCSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength)
{
	//MessageBoxW(NULL, L"HttpSendRequestA (1)", L"ConEmu plugin", MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_ICONEXCLAMATION);
	ORIGINALFAST(HttpSendRequestA);
	
	BOOL lbRc;
	
	gbHooksTemporaryDisabled = TRUE;
	//MessageBoxW(NULL, L"HttpSendRequestA (2)", L"ConEmu plugin", MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_ICONEXCLAMATION);
	lbRc = F(HttpSendRequestA)(hRequest, lpszHeaders, dwHeadersLength, lpOptional, dwOptionalLength);
	//if (!OnHttpSendRequestA_SEH(F(HttpSendRequestA), hRequest, lpszHeaders, dwHeadersLength, lpOptional, dwOptionalLength)) {
	//	MessageBoxW(NULL, L"Exception in HttpSendRequestA", L"ConEmu plugin", MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_ICONSTOP);
	//}
	gbHooksTemporaryDisabled = FALSE;
	//MessageBoxW(NULL, L"HttpSendRequestA (3)", L"ConEmu plugin", MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_ICONEXCLAMATION);
	
	return lbRc;
}
BOOL WINAPI OnHttpSendRequestW(LPVOID hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength)
{
	//MessageBoxW(NULL, L"HttpSendRequestW (1)", L"ConEmu plugin", MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_ICONEXCLAMATION);
	ORIGINALFAST(HttpSendRequestW);
	
	BOOL lbRc;
	
	gbHooksTemporaryDisabled = TRUE;
	//MessageBoxW(NULL, L"HttpSendRequestW (2)", L"ConEmu plugin", MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_ICONEXCLAMATION);
	lbRc = F(HttpSendRequestW)(hRequest, lpszHeaders, dwHeadersLength, lpOptional, dwOptionalLength);
	//if (!OnHttpSendRequestW_SEH(F(HttpSendRequestW), hRequest, lpszHeaders, dwHeadersLength, lpOptional, dwOptionalLength, &lbRc)) {
	//	MessageBoxW(NULL, L"Exception in HttpSendRequestW", L"ConEmu plugin", MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_ICONSTOP);
	//}
	gbHooksTemporaryDisabled = FALSE;
	//MessageBoxW(NULL, L"HttpSendRequestW (3)", L"ConEmu plugin", MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_ICONEXCLAMATION);
	
	return lbRc;
}

static void GuiSetForeground(HWND hWnd)
{
	if (ghConEmuDC) {
		CESERVER_REQ In, *pOut;
		ExecutePrepareCmd(&In, CECMD_SETFOREGROUND, sizeof(CESERVER_REQ_HDR)+sizeof(u64));
		In.qwData[0] = (u64)hWnd;
		HWND hConWnd = GetConsoleWindow();
		pOut = ExecuteGuiCmd(hConWnd, &In, hConWnd);
		if (pOut) ExecuteFreeResult(pOut);
	}
}

static void GuiFlashWindow(BOOL bSimple, HWND hWnd, BOOL bInvert, DWORD dwFlags, UINT uCount, DWORD dwTimeout)
{
	if (ghConEmuDC) {
		CESERVER_REQ In, *pOut;
		ExecutePrepareCmd(&In, CECMD_FLASHWINDOW, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_FLASHWINFO));
		In.Flash.bSimple = bSimple;
		In.Flash.hWnd = hWnd;
		In.Flash.bInvert = bInvert;
		In.Flash.dwFlags = dwFlags;
		In.Flash.uCount = uCount;
		In.Flash.dwTimeout = dwTimeout;
		HWND hConWnd = GetConsoleWindow();
		pOut = ExecuteGuiCmd(hConWnd, &In, hConWnd);
		if (pOut) ExecuteFreeResult(pOut);
	}
}

static bool IsHandleConsole( HANDLE handle, bool output = true )
{
	// Консоль?
    if( ( (DWORD)handle & 0x10000003) != 3 )
        return false;

    // Проверка типа консольного буфера (In/Out)
    DWORD num;
    if( !output )
        if( GetNumberOfConsoleInputEvents( handle, &num ) )
            return true;
        else
            return false;
    else
        if( GetNumberOfConsoleInputEvents( handle, &num ) )
            return false;
        else
            return true;
}


//static BOOL SrvSetConsoleCP(BOOL bSetOutputCP, DWORD nCP)
//{
//	_ASSERTE(ghConEmuDC);
//
//	BOOL lbRc = FALSE;
//
//	if (gdwServerPID) {
//		// Проверить живость процесса
//		HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, gdwServerPID);
//		if (hProcess) {
//			if (WaitForSingleObject(hProcess,0) == WAIT_OBJECT_0)
//				gdwServerPID = 0; // Процесс сервера завершился
//			CloseHandle(hProcess);
//		} else {
//			gdwServerPID = 0;
//		}
//	}
//
//	if (gdwServerPID) {
//#ifndef DROP_SETCP_ON_WIN2K3R2
//		CESERVER_REQ In, *pOut;
//		ExecutePrepareCmd(&In, CECMD_SETCONSOLECP, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_SETCONCP));
//		In.SetConCP.bSetOutputCP = bSetOutputCP;
//		In.SetConCP.nCP = nCP;
//		HWND hConWnd = GetConsoleWindow();
//		pOut = ExecuteSrvCmd(gdwServerPID, &In, hConWnd);
//		if (pOut) {
//			if (pOut->hdr.nSize >= In.hdr.nSize) {
//				lbRc = pOut->SetConCP.bSetOutputCP;
//				if (!lbRc)
//					SetLastError(pOut->SetConCP.nCP);
//			}
//			ExecuteFreeResult(pOut);
//		}
//#else
//		lbRc = TRUE;
//#endif
//	} else {
//		if (bSetOutputCP)
//			lbRc = SetConsoleOutputCP(nCP);
//		else
//			lbRc = SetConsoleCP(nCP);
//	}
//
//	return lbRc;
//}
