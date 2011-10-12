
/*
Copyright (c) 2009-2011 Maximus5
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

#ifdef _DEBUG
	#define TRAP_ON_MOUSE_0x0
#else
	#undef TRAP_ON_MOUSE_0x0
#endif

// Иначе не опередяется GetConsoleAliases (хотя он должен быть доступен в Win2k)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501

#define DEFINE_HOOK_MACROS

#define SETCONCP_READYTIMEOUT 5000
#define SETCONCP_TIMEOUT 1000

#include <windows.h>
#include <TCHAR.h>
#include <Tlhelp32.h>
#include <shlwapi.h>
#include "..\common\common.hpp"
#include "..\common\ConEmuCheck.h"
#include "SetHook.h"
#include "..\common\execute.h"
#include "ConEmuHooks.h"
#include "RegHooks.h"
#include "ShellProcessor.h"

//110131 попробуем просто добвавить ее в ExcludedModules
//#include <WinInet.h>
//#pragma comment(lib, "wininet.lib")

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
//__declspec( thread )
//static BOOL    gbInShellExecuteEx = FALSE;

//#ifdef USE_INPUT_SEMAPHORE
//HANDLE ghConInSemaphore = NULL;
//#endif

/* ************ Globals for SetHook ************ */
//HWND ghConEmuWndDC = NULL; // Содержит хэндл окна отрисовки. Это ДОЧЕРНЕЕ окно.
extern HWND    ghConWnd;      // RealConsole
extern HWND    ghConEmuWnd;   // Root! ConEmu window
extern HWND    ghConEmuWndDC; // ConEmu DC window
extern RECT    grcConEmuClient; // Для аттача гуевых окон
extern BOOL    gbAttachGuiClient; // Для аттача гуевых окон
BOOL gbGuiClientAttached; // Для аттача гуевых окон (успешно подключились)
HWND ghAttachGuiClient = NULL; // Чтобы ShowWindow перехватить
BOOL gbForceShowGuiClient = FALSE; // --
HMENU ghAttachGuiClientMenu = NULL;
RECT grcAttachGuiClientPos;
#ifdef _DEBUG
HHOOK ghGuiClientRetHook = NULL;
#endif
//HWND ghConsoleHwnd = NULL;
//BOOL gbFARuseASCIIsort = FALSE;
//DWORD gdwServerPID = 0;
//BOOL gbShellNoZoneCheck = FALSE;
/* ************ Globals for SetHook ************ */

/* ************ Globals for Far Hooks ************ */
struct HookModeFar gFarMode = {sizeof(HookModeFar)};

//const wchar_t *kernel32 = L"kernel32.dll";
//const wchar_t *user32   = L"user32.dll";
//const wchar_t *shell32  = L"shell32.dll";
//const wchar_t *advapi32 = L"Advapi32.dll";
//HMODULE ghKernel32 = NULL, ghUser32 = NULL, ghShell32 = NULL, ghAdvapi32 = NULL;

//static TCHAR kernel32[] = _T("kernel32.dll");
//static TCHAR user32[]   = _T("user32.dll");
//static TCHAR shell32[]  = _T("shell32.dll");
//static HMODULE ghKernel32 = NULL, ghUser32 = NULL, ghShell32 = NULL;
//110131 попробуем просто добвавить ее в ExcludedModules
//static TCHAR wininet[]  = _T("wininet.dll");

//extern const wchar_t kernel32[]; // = L"kernel32.dll";
//extern const wchar_t user32[]; //   = L"user32.dll";
//extern const wchar_t shell32[]; //  = L"shell32.dll";
//extern HMODULE ghKernel32, ghUser32, ghShell32;

//static BOOL bHooksWin2k3R2Only = FALSE;
//static HookItem HooksWin2k3R2Only[] = {
//	{OnSetConsoleCP, "SetConsoleCP", kernel32, 0},
//	{OnSetConsoleOutputCP, "SetConsoleOutputCP", kernel32, 0},
//	/* ************************ */
//	{0, 0, 0}
//};

int WINAPI OnCompareStringW(LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2);
//
//static BOOL WINAPI OnHttpSendRequestA(LPVOID hRequest, LPCSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength);
//static BOOL WINAPI OnHttpSendRequestW(LPVOID hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength);

//static HookItem HooksFarOnly[] =
//{
////	{OnlstrcmpiA,      "lstrcmpiA",      kernel32, 0},
//	{(void*)OnCompareStringW, "CompareStringW", kernel32},
//
//	/* ************************ */
//	//110131 попробуем просто добвавить ее в ExcludedModules
//	//{(void*)OnHttpSendRequestA, "HttpSendRequestA", wininet, 0},
//	//{(void*)OnHttpSendRequestW, "HttpSendRequestW", wininet, 0},
//	/* ************************ */
//
//	{0, 0, 0}
//};

// Service functions
//typedef DWORD (WINAPI* GetProcessId_t)(HANDLE Process);
GetProcessId_t gfGetProcessId = NULL;

//BOOL gbShowOnSetForeground = FALSE;



// Forward declarations of the hooks
// Common hooks (except LoadLibrary..., FreeLibrary, GetProcAddress)
BOOL WINAPI OnTrackPopupMenu(HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, CONST RECT * prcRect);
BOOL WINAPI OnTrackPopupMenuEx(HMENU hmenu, UINT fuFlags, int x, int y, HWND hWnd, LPTPMPARAMS lptpm);
BOOL WINAPI OnShellExecuteExA(LPSHELLEXECUTEINFOA lpExecInfo);
BOOL WINAPI OnShellExecuteExW(LPSHELLEXECUTEINFOW lpExecInfo);
HINSTANCE WINAPI OnShellExecuteA(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd);
HINSTANCE WINAPI OnShellExecuteW(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd);
BOOL WINAPI OnFlashWindow(HWND hWnd, BOOL bInvert);
BOOL WINAPI OnFlashWindowEx(PFLASHWINFO pfwi);
BOOL WINAPI OnSetForegroundWindow(HWND hWnd);
int WINAPI OnCompareStringW(LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2);
DWORD WINAPI OnGetConsoleAliasesW(LPWSTR AliasBuffer, DWORD AliasBufferLength, LPWSTR ExeName);
BOOL WINAPI OnGetNumberOfConsoleInputEvents(HANDLE hConsoleInput, LPDWORD lpcNumberOfEvents);
BOOL WINAPI OnPeekConsoleInputA(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
BOOL WINAPI OnPeekConsoleInputW(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
BOOL WINAPI OnReadConsoleInputA(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
BOOL WINAPI OnReadConsoleInputW(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
BOOL WINAPI OnWriteConsoleInputA(HANDLE hConsoleInput, const INPUT_RECORD *lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsWritten);
BOOL WINAPI OnWriteConsoleInputW(HANDLE hConsoleInput, const INPUT_RECORD *lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsWritten);
//BOOL WINAPI OnPeekConsoleInputAx(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
//BOOL WINAPI OnPeekConsoleInputWx(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
//BOOL WINAPI OnReadConsoleInputAx(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
//BOOL WINAPI OnReadConsoleInputWx(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
HANDLE WINAPI OnCreateConsoleScreenBuffer(DWORD dwDesiredAccess, DWORD dwShareMode, const SECURITY_ATTRIBUTES *lpSecurityAttributes, DWORD dwFlags, LPVOID lpScreenBufferData);
BOOL WINAPI OnAllocConsole(void);
BOOL WINAPI OnFreeConsole(void);
HWND WINAPI OnGetConsoleWindow();
BOOL WINAPI OnWriteConsoleOutputA(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
BOOL WINAPI OnWriteConsoleOutputW(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
BOOL WINAPI OnSetConsoleTextAttribute(HANDLE hConsoleOutput, WORD wAttributes);
//BOOL WINAPI OnWriteConsoleOutputAx(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
//BOOL WINAPI OnWriteConsoleOutputWx(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
BOOL WINAPI OnGetWindowRect(HWND hWnd, LPRECT lpRect);
BOOL WINAPI OnScreenToClient(HWND hWnd, LPPOINT lpPoint);
BOOL WINAPI OnCreateProcessA(LPCSTR lpApplicationName,  LPSTR lpCommandLine,  LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory,  LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
BOOL WINAPI OnCreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
#ifdef _DEBUG
HANDLE WINAPI OnCreateNamedPipeW(LPCWSTR lpName, DWORD dwOpenMode, DWORD dwPipeMode, DWORD nMaxInstances,DWORD nOutBufferSize, DWORD nInBufferSize, DWORD nDefaultTimeOut,LPSECURITY_ATTRIBUTES lpSecurityAttributes);
#endif
BOOL WINAPI OnSetConsoleCP(UINT wCodePageID);
BOOL WINAPI OnSetConsoleOutputCP(UINT wCodePageID);
#ifdef _DEBUG
LPVOID WINAPI OnVirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
#endif
BOOL WINAPI OnChooseColorA(LPCHOOSECOLORA lpcc);
BOOL WINAPI OnChooseColorW(LPCHOOSECOLORW lpcc);
//HWND WINAPI OnCreateWindowA(LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
//HWND WINAPI OnCreateWindowW(LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
HWND WINAPI OnCreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
HWND WINAPI OnCreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
BOOL WINAPI OnShowWindow(HWND hWnd, int nCmdShow);
HWND WINAPI OnSetParent(HWND hWndChild, HWND hWndNewParent);
HWND WINAPI OnGetParent(HWND hWnd);
HWND WINAPI OnGetWindow(HWND hWnd, UINT uCmd);
HWND WINAPI OnGetAncestor(HWND hwnd, UINT gaFlags);
BOOL WINAPI OnMoveWindow(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint);
BOOL WINAPI OnSetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);
BOOL WINAPI OnSetWindowPlacement(HWND hWnd, WINDOWPLACEMENT *lpwndpl);
BOOL WINAPI OnPostMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL WINAPI OnPostMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL WINAPI OnSendMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL WINAPI OnSendMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL WINAPI OnSetConsoleKeyShortcuts(BOOL bSet, BYTE bReserveKeys, LPVOID p1, DWORD n1);



bool InitHooksCommon()
{
#ifndef HOOKS_SKIP_COMMON
	// Основные хуки
	HookItem HooksCommon[] =
	{
		/* ***** MOST CALLED ***** */
		#ifndef HOOKS_COMMON_PROCESS_ONLY
		{(void*)OnGetConsoleWindow,     "GetConsoleWindow",     kernel32},
		//{(void*)OnWriteConsoleOutputWx,	"WriteConsoleOutputW",  kernel32},
		//{(void*)OnWriteConsoleOutputAx,	"WriteConsoleOutputA",  kernel32},
		{(void*)OnWriteConsoleOutputW,	"WriteConsoleOutputW",  kernel32},
		{(void*)OnWriteConsoleOutputA,	"WriteConsoleOutputA",  kernel32},
		//{(void*)OnPeekConsoleInputWx,	"PeekConsoleInputW",	kernel32},
		//{(void*)OnPeekConsoleInputAx,	"PeekConsoleInputA",	kernel32},
		//{(void*)OnReadConsoleInputWx,	"ReadConsoleInputW",	kernel32},
		//{(void*)OnReadConsoleInputAx,	"ReadConsoleInputA",	kernel32},
		{(void*)OnPeekConsoleInputW,	"PeekConsoleInputW",	kernel32},
		{(void*)OnPeekConsoleInputA,	"PeekConsoleInputA",	kernel32},
		{(void*)OnReadConsoleInputW,	"ReadConsoleInputW",	kernel32},
		{(void*)OnReadConsoleInputA,	"ReadConsoleInputA",	kernel32},
		{(void*)OnWriteConsoleInputA,	"WriteConsoleInputA",	kernel32},
		{(void*)OnWriteConsoleInputW,	"WriteConsoleInputW",	kernel32},
		{(void*)OnSetConsoleTextAttribute, "SetConsoleTextAttribute", kernel32},
		{(void*)OnSetConsoleKeyShortcuts, "SetConsoleKeyShortcuts", kernel32},
		#endif
		/* ************************ */
		{(void*)OnCreateProcessA,		"CreateProcessA",		kernel32},
		{(void*)OnCreateProcessW,		"CreateProcessW",		kernel32},
		/* ************************ */
		#ifndef HOOKS_COMMON_PROCESS_ONLY
		{(void*)OnGetConsoleAliasesW,	"GetConsoleAliasesW",	kernel32},
		{(void*)OnAllocConsole,			"AllocConsole",			kernel32},
		{(void*)OnFreeConsole,			"FreeConsole",			kernel32},
		{
			(void*)OnGetNumberOfConsoleInputEvents,
			"GetNumberOfConsoleInputEvents",
			kernel32
		},
		{
			(void*)OnCreateConsoleScreenBuffer,
			"CreateConsoleScreenBuffer",
			kernel32
		},
		#endif
		/* ************************ */
		#ifdef _DEBUG
		#ifndef HOOKS_COMMON_PROCESS_ONLY
		{(void*)OnCreateNamedPipeW,		"CreateNamedPipeW",		kernel32},
		#endif
		#endif
		#ifdef _DEBUG
		#ifdef HOOKS_VIRTUAL_ALLOC
		{(void*)OnVirtualAlloc,			"VirtualAlloc",			kernel32},
		#endif
		#endif
		// Microsoft bug?
		// http://code.google.com/p/conemu-maximus5/issues/detail?id=60
		#ifndef HOOKS_COMMON_PROCESS_ONLY
		{(void*)OnSetConsoleCP,			"SetConsoleCP",			kernel32},
		{(void*)OnSetConsoleOutputCP,	"SetConsoleOutputCP",	kernel32},
		#endif
		/* ************************ */
		{(void*)OnShellExecuteExA,		"ShellExecuteExA",		shell32},
		{(void*)OnShellExecuteExW,		"ShellExecuteExW",		shell32},
		{(void*)OnShellExecuteA,		"ShellExecuteA",		shell32},
		{(void*)OnShellExecuteW,		"ShellExecuteW",		shell32},
		/* ************************ */
		{(void*)OnChooseColorA,			"ChooseColorA",			comdlg32},
		{(void*)OnChooseColorW,			"ChooseColorW",			comdlg32},
		/* ************************ */
		{0}
	};
	InitHooks(HooksCommon);
#endif
	return true;
}

bool InitHooksUser32()
{
#ifndef HOOKS_SKIP_COMMON
	// Основные хуки
	HookItem HooksCommon[] =
	{
		/* ************************ */
		#ifndef HOOKS_COMMON_PROCESS_ONLY
		{(void*)OnTrackPopupMenu,		"TrackPopupMenu",		user32},
		{(void*)OnTrackPopupMenuEx,		"TrackPopupMenuEx",		user32},
		{(void*)OnFlashWindow,			"FlashWindow",			user32},
		{(void*)OnFlashWindowEx,		"FlashWindowEx",		user32},
		{(void*)OnSetForegroundWindow,	"SetForegroundWindow",	user32},
		{(void*)OnGetWindowRect,		"GetWindowRect",		user32},
		{(void*)OnScreenToClient,		"ScreenToClient",		user32},
		#endif
		/* ************************ */
		//{(void*)OnCreateWindowA,		"CreateWindowA",		user32}, -- таких экспортов нет
		//{(void*)OnCreateWindowW,		"CreateWindowW",		user32}, -- таких экспортов нет
		{(void*)OnCreateWindowExA,		"CreateWindowExA",		user32},
		{(void*)OnCreateWindowExW,		"CreateWindowExW",		user32},
		{(void*)OnShowWindow,			"ShowWindow",			user32},
		{(void*)OnSetParent,			"SetParent",			user32},
		{(void*)OnGetParent,			"GetParent",			user32},
		{(void*)OnGetWindow,			"GetWindow",			user32},
		{(void*)OnGetAncestor,			"GetAncestor",			user32},
		{(void*)OnMoveWindow,			"MoveWindow",			user32},
		{(void*)OnSetWindowPos,			"SetWindowPos",			user32},
		{(void*)OnSetWindowPlacement,	"SetWindowPlacement",	user32},
		{(void*)OnPostMessageA,			"PostMessageA",			user32},
		{(void*)OnPostMessageW,			"PostMessageW",			user32},
		{(void*)OnSendMessageA,			"SendMessageA",			user32},
		{(void*)OnSendMessageW,			"SendMessageW",			user32},
		/* ************************ */
		{0}
	};
	InitHooks(HooksCommon);
#endif
	return true;
}

bool InitHooksFar()
{
#ifndef HOOKS_SKIP_COMMON
	// Проверить, фар ли это? Если нет - можно не ставить HooksFarOnly
	bool lbIsFar = false;
	wchar_t* pszExe = (wchar_t*)calloc(MAX_PATH+1,sizeof(wchar_t));
	if (pszExe)
	{
		if (GetModuleFileName(NULL, pszExe, MAX_PATH))
		{
			LPCWSTR pszName = PointToName(pszExe);
			if (pszName && lstrcmpi(pszName, L"far.exe") == 0)
				lbIsFar = true;
		}
		free(pszExe);
	}
	
	if (!lbIsFar)
		return true; // не хукать
	
	HookItem HooksFarOnly[] =
	{
	//	{OnlstrcmpiA,      "lstrcmpiA",      kernel32, 0},
		{(void*)OnCompareStringW, "CompareStringW", kernel32},

		/* ************************ */
		//110131 попробуем просто добвавить ее в ExcludedModules
		//{(void*)OnHttpSendRequestA, "HttpSendRequestA", wininet, 0},
		//{(void*)OnHttpSendRequestW, "HttpSendRequestW", wininet, 0},
		/* ************************ */

		{0, 0, 0}
	};
	InitHooks(HooksFarOnly);
#endif
	return true;
}



//static HookItem HooksCommon[] =
//{
//	/* ***** MOST CALLED ***** */
////	{(void*)OnGetConsoleWindow,     "GetConsoleWindow",     kernel32}, -- пока смысла нет. инжекты еще не на старте ставятся
//	//{(void*)OnWriteConsoleOutputWx,	"WriteConsoleOutputW",  kernel32},
//	//{(void*)OnWriteConsoleOutputAx,	"WriteConsoleOutputA",  kernel32},
//	{(void*)OnWriteConsoleOutputW,	"WriteConsoleOutputW",  kernel32},
//	{(void*)OnWriteConsoleOutputA,	"WriteConsoleOutputA",  kernel32},
//	/* ************************ */
//	//{(void*)OnPeekConsoleInputWx,	"PeekConsoleInputW",	kernel32},
//	//{(void*)OnPeekConsoleInputAx,	"PeekConsoleInputA",	kernel32},
//	//{(void*)OnReadConsoleInputWx,	"ReadConsoleInputW",	kernel32},
//	//{(void*)OnReadConsoleInputAx,	"ReadConsoleInputA",	kernel32},
//	{(void*)OnPeekConsoleInputW,	"PeekConsoleInputW",	kernel32},
//	{(void*)OnPeekConsoleInputA,	"PeekConsoleInputA",	kernel32},
//	{(void*)OnReadConsoleInputW,	"ReadConsoleInputW",	kernel32},
//	{(void*)OnReadConsoleInputA,	"ReadConsoleInputA",	kernel32},
//	/* ************************ */
//	{(void*)OnCreateProcessA,		"CreateProcessA",		kernel32},
//	{(void*)OnCreateProcessW,		"CreateProcessW",		kernel32},
//	/* ************************ */
//	{(void*)OnGetConsoleAliasesW,	"GetConsoleAliasesW",	kernel32},
//	{(void*)OnAllocConsole,			"AllocConsole",			kernel32},
//	{(void*)OnFreeConsole,			"FreeConsole",			kernel32},
//	{
//		(void*)OnGetNumberOfConsoleInputEvents,
//		"GetNumberOfConsoleInputEvents",
//		kernel32
//	},
//	{
//		(void*)OnCreateConsoleScreenBuffer,
//		"CreateConsoleScreenBuffer",
//		kernel32
//	},
//#ifdef _DEBUG
//	{(void*)OnCreateNamedPipeW,		"CreateNamedPipeW",		kernel32},
//#endif
//#ifdef _DEBUG
//	{(void*)OnVirtualAlloc,			"VirtualAlloc",			kernel32},
//#endif
//	// Microsoft bug?
//	// http://code.google.com/p/conemu-maximus5/issues/detail?id=60
//	{(void*)OnSetConsoleCP,			"SetConsoleCP",			kernel32},
//	{(void*)OnSetConsoleOutputCP,	"SetConsoleOutputCP",	kernel32},
//	/* ************************ */
//	{(void*)OnTrackPopupMenu,		"TrackPopupMenu",		user32},
//	{(void*)OnTrackPopupMenuEx,		"TrackPopupMenuEx",		user32},
//	{(void*)OnFlashWindow,			"FlashWindow",			user32},
//	{(void*)OnFlashWindowEx,		"FlashWindowEx",		user32},
//	{(void*)OnSetForegroundWindow,	"SetForegroundWindow",	user32},
//	{(void*)OnGetWindowRect,		"GetWindowRect",		user32},
//	{(void*)OnScreenToClient,		"ScreenToClient",		user32},
//	/* ************************ */
//	{(void*)OnShellExecuteExA,		"ShellExecuteExA",		shell32},
//	{(void*)OnShellExecuteExW,		"ShellExecuteExW",		shell32},
//	{(void*)OnShellExecuteA,		"ShellExecuteA",		shell32},
//	{(void*)OnShellExecuteW,		"ShellExecuteW",		shell32},
//	/* ************************ */
//	{0}
//};

void __stdcall SetFarHookMode(struct HookModeFar *apFarMode)
{
	if (!apFarMode)
	{
		gFarMode.bFarHookMode = FALSE;
	}
	else if (apFarMode->cbSize != sizeof(HookModeFar))
	{
		_ASSERTE(apFarMode->cbSize == sizeof(HookModeFar));
		/*
		gFarMode.bFarHookMode = FALSE;
		wchar_t szTitle[64], szText[255];
		msprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuHk, PID=%u", GetCurrentProcessId());
		msprintf(szText, SKIPLEN(countof(szText)) L"SetFarHookMode recieved invalid sizeof = %u\nRequired = %u", apFarMode->cbSize, (DWORD)sizeof(HookModeFar));
		Message BoxW(NULL, szText, szTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
		*/
	}
	else
	{
		memmove(&gFarMode, apFarMode, sizeof(gFarMode));
	}
}

//void InitializeConsoleInputSemaphore()
//{
//#ifdef USE_INPUT_SEMAPHORE
//	if (ghConInSemaphore != NULL)
//	{
//		ReleaseConsoleInputSemaphore();
//	}
//	wchar_t szName[128];
//	HWND hConWnd = GetConEmuHWND(2);
//	if (hConWnd != ghConWnd)
//	{
//		_ASSERTE(ghConWnd == hConWnd);
//	}
//	if (hConWnd != NULL)
//	{
//		msprintf(szName, SKIPLEN(countof(szName)) CEINPUTSEMAPHORE, (DWORD)hConWnd);
//		ghConInSemaphore = CreateSemaphore(LocalSecurity(), 1, 1, szName);
//		_ASSERTE(ghConInSemaphore != NULL);
//	}
//#endif
//}
//
//void ReleaseConsoleInputSemaphore()
//{
//#ifdef USE_INPUT_SEMAPHORE
//	if (ghConInSemaphore != NULL)
//	{
//		CloseHandle(ghConInSemaphore);
//		ghConInSemaphore = NULL;
//	}
//#endif
//}

// Эту функцию нужно позвать из DllMain
BOOL StartupHooks(HMODULE ahOurDll)
{
#ifdef _DEBUG
	// Консольное окно уже должно быть иницализировано в DllMain
	_ASSERTE(ghConWnd != NULL && ghConWnd == GetConsoleWindow());
	wchar_t sClass[128]; GetClassName(ghConWnd, sClass, countof(sClass));
	_ASSERTE(lstrcmp(sClass, L"ConsoleWindowClass") == 0);
#endif

	// -- ghConEmuWnd уже должен быть установлен в DllMain!!!
	//ghConEmuWndDC = GetConEmuHWND(FALSE);
	//ghConEmuWnd = ghConEmuWndDC ? ghConEmuWnd : NULL;
	//gbInShellExecuteEx = FALSE;

	WARNING("Получить из мэппинга gdwServerPID");

	// Зовем LoadLibrary. Kernel-то должен был сразу загрузится (static link) в любой
	// windows приложении, но вот shell32 - не обязательно, а нам нужно хуки проинициализировать
	ghKernel32 = LoadLibrary(kernel32);
	// user32/shell32/advapi32 тянут за собой много других библиотек, НЕ загружаем, если они еще не подлинкованы
	ghUser32 = GetModuleHandle(user32);
	if (ghUser32) ghUser32 = LoadLibrary(user32); // если подлинкован - увеличить счетчик
	ghShell32 = GetModuleHandle(shell32);
	if (ghShell32) ghShell32 = LoadLibrary(shell32); // если подлинкован - увеличить счетчик
	ghAdvapi32 = GetModuleHandle(advapi32);
	if (ghAdvapi32) ghAdvapi32 = LoadLibrary(advapi32); // если подлинкован - увеличить счетчик
	ghComdlg32 = GetModuleHandle(comdlg32);
	if (ghComdlg32) ghComdlg32 = LoadLibrary(comdlg32); // если подлинкован - увеличить счетчик

	if (ghKernel32)
		gfGetProcessId = (GetProcessId_t)GetProcAddress(ghKernel32, "GetProcessId");

	// Общие
	InitHooksCommon();

	// user32
	InitHooksUser32();
	
	// Far only functions
	InitHooksFar();

	// Реестр
	InitHooksReg();
	
	// Теперь можно обработать модули
	return SetAllHooks(ahOurDll, NULL, TRUE);
}


BOOL MyAllowSetForegroundWindow(DWORD dwProcessId)
{
	BOOL lbRc = FALSE;
	typedef BOOL (WINAPI* AllowSetForegroundWindow_t)(DWORD);
	AllowSetForegroundWindow_t AllowSetForegroundWindow_f = (AllowSetForegroundWindow_t)GetProcAddress(ghUser32, "AllowSetForegroundWindow");
	if (AllowSetForegroundWindow_f)
	{
		lbRc = AllowSetForegroundWindow_f(dwProcessId);
	}
	else
	{
		_ASSERTE(AllowSetForegroundWindow_f!=NULL);
	}
	return lbRc;
}

DWORD MyGetWindowThreadProcessId(HWND hWnd, LPDWORD lpdwProcessId)
{
	DWORD lRc = 0;
	typedef BOOL (WINAPI* GetWindowThreadProcessId_t)(HWND,LPDWORD);
	GetWindowThreadProcessId_t GetWindowThreadProcessId_f = (GetWindowThreadProcessId_t)GetProcAddress(ghUser32, "GetWindowThreadProcessId");
	if (GetWindowThreadProcessId_f)
	{
		lRc = GetWindowThreadProcessId_f(hWnd, lpdwProcessId);
	}
	else
	{
		_ASSERTE(GetWindowThreadProcessId_f!=NULL);
	}
	return lRc;
}

BOOL MySetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
	BOOL lbRc = FALSE;
	typedef BOOL (WINAPI* SetWindowPos_t)(HWND,HWND,int,int,int,int,UINT);
	SetWindowPos_t SetWindowPos_f = (SetWindowPos_t)GetProcAddress(ghUser32, "SetWindowPos");
	if (SetWindowPos_f)
	{
		lbRc = SetWindowPos_f(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
	}
	else
	{
		_ASSERTE(SetWindowPos_f!=NULL);
	}
	return lbRc;
}

LONG_PTR MyGetWindowLongPtrW(HWND hWnd, int nIndex)
{
	LONG_PTR lRc = 0;
	typedef LONG_PTR (WINAPI* GetWindowLongPtr_t)(HWND,int);
	GetWindowLongPtr_t GetWindowLongPtr_f = (GetWindowLongPtr_t)GetProcAddress(ghUser32,WIN3264TEST("GetWindowLongW","GetWindowLongPtrW"));
	if (GetWindowLongPtr_f)
	{
		lRc = GetWindowLongPtr_f(hWnd, nIndex);
	}
	else
	{
		_ASSERTE(GetWindowLongPtr_f!=NULL);
	}
	return lRc;
}

LONG_PTR MySetWindowLongPtrW(HWND hWnd, int nIndex, LONG_PTR dwNewLong)
{
	LONG_PTR lRc = 0;
	typedef LONG_PTR (WINAPI* SetWindowLongPtr_t)(HWND,int,LONG_PTR);
	SetWindowLongPtr_t SetWindowLongPtr_f = (SetWindowLongPtr_t)GetProcAddress(ghUser32,WIN3264TEST("SetWindowLongW","SetWindowLongPtrW"));
	if (SetWindowLongPtr_f)
	{
		lRc = SetWindowLongPtr_f(hWnd, nIndex, dwNewLong);
	}
	else
	{
		_ASSERTE(SetWindowLongPtr_f!=NULL);
	}
	return lRc;
}

BOOL MyGetWindowRect(HWND hWnd, LPRECT lpRect)
{
	BOOL bRc = FALSE;
	typedef BOOL (WINAPI* GetWindowRect_t)(HWND,LPRECT);
	GetWindowRect_t GetWindowRect_f = (GetWindowRect_t)GetProcAddress(ghUser32, "GetWindowRect");
	if (GetWindowRect_f)
	{
		bRc = GetWindowRect_f(hWnd, lpRect);
	}
	else
	{
		_ASSERTE(GetWindowRect_f!=NULL);
	}
	return bRc;
}

BOOL MySystemParametersInfoW(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni)
{
	BOOL bRc = FALSE;
	typedef BOOL (WINAPI* SystemParametersInfo_t)(UINT,UINT,PVOID,UINT);
	SystemParametersInfo_t SystemParametersInfo_f = (SystemParametersInfo_t)GetProcAddress(ghUser32, "SystemParametersInfoW");
	if (SystemParametersInfo_f)
	{
		bRc = SystemParametersInfo_f(uiAction, uiParam, pvParam, fWinIni);
	}
	else
	{
		_ASSERTE(SystemParametersInfo_f!=NULL);
	}
	return bRc;
}

BOOL MySetWindowTextW(HWND hWnd, LPCWSTR lpString)
{
	BOOL bRc = FALSE;
	typedef BOOL (WINAPI* SetWindowText_t)(HWND,LPCWSTR);
	SetWindowText_t SetWindowText_f = (SetWindowText_t)GetProcAddress(ghUser32, "SetWindowTextW");
	if (SetWindowText_f)
	{
		bRc = SetWindowText_f(hWnd, lpString);
	}
	else
	{
		_ASSERTE(SetWindowText_f!=NULL);
	}
	return bRc;
}

BOOL MyEndDialog(HWND hDlg, INT_PTR nResult)
{
	BOOL bRc = FALSE;
	typedef BOOL (WINAPI* EndDialog_t)(HWND,INT_PTR);
	EndDialog_t EndDialog_f = (EndDialog_t)GetProcAddress(ghUser32, "EndDialog");
	if (EndDialog_f)
	{
		bRc = EndDialog_f(hDlg, nResult);
	}
	else
	{
		_ASSERTE(EndDialog_f!=NULL);
	}
	return bRc;
}

INT_PTR MyDialogBoxIndirectParamW(HINSTANCE hInstance, LPCDLGTEMPLATEW hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	INT_PTR nRc = 0;
	typedef INT_PTR (WINAPI* DialogBoxIndirectParam_t)(HINSTANCE,LPCDLGTEMPLATE,HWND,DLGPROC,LPARAM);
	DialogBoxIndirectParam_t DialogBoxIndirectParam_f = (DialogBoxIndirectParam_t)GetProcAddress(ghUser32, "DialogBoxIndirectParamW");
	if (DialogBoxIndirectParam_f)
	{
		nRc = DialogBoxIndirectParam_f(hInstance, hDialogTemplate, hWndParent, lpDialogFunc, dwInitParam);
	}
	else
	{
		_ASSERTE(DialogBoxIndirectParam_f!=NULL);
	}
	return nRc;
}

TODO("User32.dll - импорты");
/*
GetClassInfoExA
GetClassInfoExW
GetClassNameW
GetClientRect
GetSystemMenu
GetWindowLongW
GetWindowRect
InsertMenuW
IsWindow
LoadMenuA
LoadMenuW
SendMessageW
SetMenu
*/


//void ShutdownHooks()
//{
//	UnsetAllHooks();
//	
//	//// Завершить работу с реестром
//	//DoneHooksReg();
//
//	// Уменьшение счетчиков загрузок
//	if (ghKernel32)
//	{
//		FreeLibrary(ghKernel32);
//		ghKernel32 = NULL;
//	}
//	if (ghUser32)
//	{
//		FreeLibrary(ghUser32);
//		ghUser32 = NULL;
//	}
//	if (ghShell32)
//	{
//		FreeLibrary(ghShell32);
//		ghShell32 = NULL;
//	}
//	if (ghAdvapi32)
//	{
//		FreeLibrary(ghAdvapi32);
//		ghAdvapi32 = NULL;
//	}
//	if (ghComdlg32)
//	{
//		FreeLibrary(ghComdlg32);
//		ghComdlg32 = NULL;
//	}
//}



// Forward
void GuiSetForeground(HWND hWnd);
void GuiFlashWindow(BOOL bSimple, HWND hWnd, BOOL bInvert, DWORD dwFlags, UINT uCount, DWORD dwTimeout);
//BOOL PrepareExecuteParmsA(enum CmdOnCreateType aCmd, LPCSTR asAction, DWORD anFlags, 
//				LPCSTR asFile, LPCSTR asParam,
//				HANDLE hStdIn, HANDLE hStdOut, HANDLE hStdErr,
//				LPSTR* psFile, LPSTR* psParam, DWORD& nImageSubsystem, DWORD& nImageBits);
//BOOL PrepareExecuteParmsW(enum CmdOnCreateType aCmd, LPCWSTR asAction, DWORD anFlags, 
//				LPCWSTR asFile, LPCWSTR asParam,
//				HANDLE hStdIn, HANDLE hStdOut, HANDLE hStdErr,
//				LPWSTR* psFile, LPWSTR* psParam, DWORD& nImageSubsystem, DWORD& nImageBits);
//wchar_t* str2wcs(const char* psz, UINT anCP);
//wchar_t* wcs2str(const char* psz, UINT anCP);


BOOL WINAPI OnCreateProcessA(LPCSTR lpApplicationName,  LPSTR lpCommandLine,  LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory,  LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
	typedef BOOL (WINAPI* OnCreateProcessA_t)(LPCSTR lpApplicationName,  LPSTR lpCommandLine,  LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory,  LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
	ORIGINALFAST(CreateProcessA);
	BOOL bMainThread = FALSE; // поток не важен
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
	sp->OnCreateProcessA(&lpApplicationName, (LPCSTR*)&lpCommandLine, &dwCreationFlags, lpStartupInfo);
	if ((dwCreationFlags & CREATE_SUSPENDED) == 0)
		OutputDebugString(L"CreateProcessA without CREATE_SUSPENDED Flag!\n");


	lbRc = F(CreateProcessA)(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	if (!lbRc) dwErr = GetLastError();


	// Если lbParamsChanged == TRUE - об инжектах позаботится ConEmuC.exe
	sp->OnCreateProcessFinished(lbRc, lpProcessInformation);
	delete sp;


	if (ph && ph->PostCallBack)
	{
		SETARGS10(&lbRc, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
		ph->PostCallBack(&args);
	}
	
	return lbRc;
}

BOOL WINAPI OnCreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
	typedef BOOL (WINAPI* OnCreateProcessW_t)(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
	ORIGINALFAST(CreateProcessW);
	BOOL bMainThread = FALSE; // поток не важен
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
	sp->OnCreateProcessW(&lpApplicationName, (LPCWSTR*)&lpCommandLine, &dwCreationFlags, lpStartupInfo);
	if ((dwCreationFlags & CREATE_SUSPENDED) == 0)
		OutputDebugString(L"CreateProcessW without CREATE_SUSPENDED Flag!\n");


	lbRc = F(CreateProcessW)(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	if (!lbRc) dwErr = GetLastError();

	// Если lbParamsChanged == TRUE - об инжектах позаботится ConEmuC.exe
	sp->OnCreateProcessFinished(lbRc, lpProcessInformation);
	delete sp;


	if (ph && ph->PostCallBack)
	{
		SETARGS10(&lbRc, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
		ph->PostCallBack(&args);
	}

	return lbRc;
}



BOOL WINAPI OnTrackPopupMenu(HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, CONST RECT * prcRect)
{
	typedef BOOL (WINAPI* OnTrackPopupMenu_t)(HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, CONST RECT * prcRect);
	ORIGINALFASTEX(TrackPopupMenu,NULL);
#ifndef CONEMU_MINIMAL
	WCHAR szMsg[128]; msprintf(szMsg, SKIPLEN(countof(szMsg)) L"TrackPopupMenu(hwnd=0x%08X)\n", (DWORD)hWnd);
	DebugString(szMsg);
#endif

	if (ghConEmuWndDC)
	{
		// Необходимо "поднять" наверх консольное окно, т.к. от него идет меню
		GuiSetForeground(hWnd);
	}

	BOOL lbRc = FALSE;
	if (F(TrackPopupMenu) != NULL)
		lbRc = F(TrackPopupMenu)(hMenu, uFlags, x, y, nReserved, hWnd, prcRect);
	return lbRc;
}


BOOL WINAPI OnTrackPopupMenuEx(HMENU hmenu, UINT fuFlags, int x, int y, HWND hWnd, LPTPMPARAMS lptpm)
{
	typedef BOOL (WINAPI* OnTrackPopupMenuEx_t)(HMENU hmenu, UINT fuFlags, int x, int y, HWND hWnd, LPTPMPARAMS lptpm);
	ORIGINALFASTEX(TrackPopupMenuEx,NULL);
#ifndef CONEMU_MINIMAL
	WCHAR szMsg[128]; msprintf(szMsg, SKIPLEN(countof(szMsg)) L"TrackPopupMenuEx(hwnd=0x%08X)\n", (DWORD)hWnd);
	DebugString(szMsg);
#endif

	if (ghConEmuWndDC)
	{
		// Необходимо "поднять" наверх консольное окно, т.к. от него идет меню
		GuiSetForeground(hWnd);
	}

	BOOL lbRc = FALSE;
	if (F(TrackPopupMenuEx) != NULL)
		lbRc = F(TrackPopupMenuEx)(hmenu, fuFlags, x, y, hWnd, lptpm);
	return lbRc;
}

#ifndef SEE_MASK_NOZONECHECKS
#define SEE_MASK_NOZONECHECKS      0x00800000
#endif


BOOL WINAPI OnShellExecuteExA(LPSHELLEXECUTEINFOA lpExecInfo)
{
	typedef BOOL (WINAPI* OnShellExecuteExA_t)(LPSHELLEXECUTEINFOA lpExecInfo);
	ORIGINALFASTEX(ShellExecuteExA,NULL);
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
	sp->OnShellExecuteExA(&lpExecInfo);

	//// Under ConEmu only!
	//if (ghConEmuWndDC)
	//{
	//	if (!lpNew->hwnd || lpNew->hwnd == GetConsoleWindow())
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
	
	//if (lbRc && gfGetProcessId && lpNew->hProcess)
	//{
	//	dwProcessID = gfGetProcessId(lpNew->hProcess);
	//}

	//#ifdef _DEBUG
	//
	//	if (lpNew->lpParameters)
	//	{
	//		OutputDebugStringW(L"After ShellExecuteEx\n");
	//		OutputDebugStringA(lpNew->lpParameters);
	//		OutputDebugStringW(L"\n");
	//	}
	//
	//	if (lbRc && dwProcessID)
	//	{
	//		wchar_t szDbgMsg[128]; msprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"Process created: %u\n", dwProcessID);
	//		OutputDebugStringW(szDbgMsg);
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
	return lbRc;
}

//BOOL OnShellExecuteExW_SEH(OnShellExecuteExW_t f, LPSHELLEXECUTEINFOW lpExecInfo, BOOL* pbRc)
//{
//	BOOL lbOk = FALSE;
//	SAFETRY
//	{
//		*pbRc = f(lpExecInfo);
//		lbOk = TRUE;
//	} SAFECATCH
//	{
//		lbOk = FALSE;
//	}
//	return lbOk;
//}
BOOL WINAPI OnShellExecuteExW(LPSHELLEXECUTEINFOW lpExecInfo)
{
	typedef BOOL (WINAPI* OnShellExecuteExW_t)(LPSHELLEXECUTEINFOW lpExecInfo);
	ORIGINALFASTEX(ShellExecuteExW,NULL);
	if (!F(ShellExecuteExW))
	{
		SetLastError(ERROR_INVALID_FUNCTION);
		return FALSE;
	}
	#ifdef _DEBUG
	BOOL bMainThread = (GetCurrentThreadId() == gnHookMainThreadId);
	#endif

	CShellProc* sp = new CShellProc();
	sp->OnShellExecuteExW(&lpExecInfo);

	BOOL lbRc = FALSE;

	lbRc = F(ShellExecuteExW)(lpExecInfo);

	sp->OnShellFinished(lbRc, lpExecInfo->hInstApp, lpExecInfo->hProcess);
	delete sp;

	return lbRc;
}


HINSTANCE WINAPI OnShellExecuteA(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd)
{
	typedef HINSTANCE(WINAPI* OnShellExecuteA_t)(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd);
	ORIGINALFASTEX(ShellExecuteA,NULL);
	if (!F(ShellExecuteA))
	{
		SetLastError(ERROR_INVALID_FUNCTION);
		return FALSE;
	}

	if (ghConEmuWndDC)
	{
		if (!hwnd || hwnd == GetConsoleWindow())
			hwnd = ghConEmuWnd;
	}
	
	//gbInShellExecuteEx = TRUE;
	CShellProc* sp = new CShellProc();
	sp->OnShellExecuteA(&lpOperation, &lpFile, &lpParameters, NULL, (DWORD*)&nShowCmd);

	HINSTANCE lhRc;
	lhRc = F(ShellExecuteA)(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
	
	sp->OnShellFinished(((INT_PTR)lhRc > 32), lhRc, NULL); //-V112
	delete sp;

	//gbInShellExecuteEx = FALSE;
	return lhRc;
}

HINSTANCE WINAPI OnShellExecuteW(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd)
{
	typedef HINSTANCE(WINAPI* OnShellExecuteW_t)(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd);
	ORIGINALFASTEX(ShellExecuteW,NULL);
	if (!F(ShellExecuteW))
	{
		SetLastError(ERROR_INVALID_FUNCTION);
		return FALSE;
	}

	if (ghConEmuWndDC)
	{
		if (!hwnd || hwnd == GetConsoleWindow())
			hwnd = ghConEmuWnd;
	}
	
	//gbInShellExecuteEx = TRUE;
	CShellProc* sp = new CShellProc();
	sp->OnShellExecuteW(&lpOperation, &lpFile, &lpParameters, NULL, (DWORD*)&nShowCmd);

	HINSTANCE lhRc;
	lhRc = F(ShellExecuteW)(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
	
	sp->OnShellFinished(((INT_PTR)lhRc > 32), lhRc, NULL); //-V112
	delete sp;

	//gbInShellExecuteEx = FALSE;
	return lhRc;
}


BOOL WINAPI OnFlashWindow(HWND hWnd, BOOL bInvert)
{
	typedef BOOL (WINAPI* OnFlashWindow_t)(HWND hWnd, BOOL bInvert);
	ORIGINALFASTEX(FlashWindow,NULL);

	if (ghConEmuWndDC)
	{
		GuiFlashWindow(TRUE, hWnd, bInvert, 0,0,0);
		return TRUE;
	}

	BOOL lbRc = NULL;
	if (F(FlashWindow) != NULL)
		lbRc = F(FlashWindow)(hWnd, bInvert);
	return lbRc;
}

BOOL WINAPI OnFlashWindowEx(PFLASHWINFO pfwi)
{
	typedef BOOL (WINAPI* OnFlashWindowEx_t)(PFLASHWINFO pfwi);
	ORIGINALFASTEX(FlashWindowEx,NULL);

	if (ghConEmuWndDC)
	{
		GuiFlashWindow(FALSE, pfwi->hwnd, 0, pfwi->dwFlags, pfwi->uCount, pfwi->dwTimeout);
		return TRUE;
	}

	BOOL lbRc = FALSE;
	if (F(FlashWindowEx) != NULL)
		lbRc = F(FlashWindowEx)(pfwi);
	return lbRc;
}


BOOL WINAPI OnSetForegroundWindow(HWND hWnd)
{
	typedef BOOL (WINAPI* OnSetForegroundWindow_t)(HWND hWnd);
	ORIGINALFASTEX(SetForegroundWindow,NULL);

	if (ghConEmuWndDC)
	{
		GuiSetForeground(hWnd);
	}

	BOOL lbRc = FALSE;
	if (F(SetForegroundWindow) != NULL)
	{
		lbRc = F(SetForegroundWindow)(hWnd);

		//if (gbShowOnSetForeground && lbRc)
		//{
		//	if (IsWindow(hWnd) && !IsWindowVisible(hWnd))
		//		ShowWindow(hWnd, SW_SHOW);
		//}
	}
	return lbRc;
}


BOOL WINAPI OnGetWindowRect(HWND hWnd, LPRECT lpRect)
{
	typedef BOOL (WINAPI* OnGetWindowRect_t)(HWND hWnd, LPRECT lpRect);
	ORIGINALFASTEX(GetWindowRect,NULL);
	BOOL lbRc = FALSE;

	if ((hWnd == ghConWnd) && ghConEmuWndDC)
	{
		//EMenu gui mode issues (center in window). "Remove" Far window from mouse cursor.
		hWnd = ghConEmuWndDC;
	}

	if (F(GetWindowRect) != NULL)
		lbRc = F(GetWindowRect)(hWnd, lpRect);
	//if (ghConEmuWndDC && lpRect)
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


BOOL WINAPI OnScreenToClient(HWND hWnd, LPPOINT lpPoint)
{
	typedef BOOL (WINAPI* OnScreenToClient_t)(HWND hWnd, LPPOINT lpPoint);
	ORIGINALFASTEX(ScreenToClient,NULL);
	BOOL lbRc = FALSE;

	if (F(ScreenToClient) != NULL)
		lbRc = F(ScreenToClient)(hWnd, lpPoint);

	if (lbRc && ghConEmuWndDC && lpPoint && (hWnd == ghConWnd))
	{
		//EMenu text mode issues. "Remove" Far window from mouse cursor.
		lpPoint->x = lpPoint->y = -1;
	}

	return lbRc;
}

#ifdef _DEBUG
LRESULT CALLBACK GuiClientRetHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		CWPRETSTRUCT* p = (CWPRETSTRUCT*)lParam;
		CREATESTRUCT* pc = NULL;
		wchar_t szDbg[200]; szDbg[0] = 0;
		wchar_t szClass[127]; szClass[0] = 0;
		switch (p->message)
		{
			case WM_DESTROY:
			{
				GetClassName(p->hwnd, szClass, countof(szClass));
				_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_DESTROY on 0x%08X (%s)\n", (DWORD)p->hwnd, szClass);
				break;
			}
			case WM_CREATE:
			{
				pc = (CREATESTRUCT*)p->lParam;
				if (p->lResult == -1)
				{
					wcscpy_c(szDbg, L"WM_CREATE --FAILED--\n");
					break;
				}
			}
		}

		if (*szDbg)
			OutputDebugString(szDbg);
	}

	return CallNextHookEx(ghGuiClientRetHook, nCode, wParam, lParam);
}
#endif


static bool CheckCanCreateWindow(LPCSTR lpClassNameA, LPCWSTR lpClassNameW, DWORD& dwStyle, DWORD& dwExStyle, HWND& hWndParent, BOOL& bAttachGui, BOOL& bStyleHidden)
{
	bAttachGui = FALSE;
	
	// "!dwStyle" добавил для shell32.dll!CExecuteApplication::_CreateHiddenDDEWindow()
	_ASSERTE(hWndParent==NULL || ghConEmuWnd == NULL || hWndParent!=ghConEmuWnd || !dwStyle);
	
	if (gbAttachGuiClient && ghConEmuWndDC && (GetCurrentThreadId() == gnHookMainThreadId))
	{
		bool lbCanAttach = (dwStyle & WS_OVERLAPPEDWINDOW) == WS_OVERLAPPEDWINDOW;
		if (dwStyle & (WS_POPUP|DS_MODALFRAME|WS_CHILDWINDOW))
			lbCanAttach = false;
		else if (dwExStyle & (WS_EX_TOOLWINDOW|WS_EX_TOPMOST|WS_EX_DLGMODALFRAME|WS_EX_MDICHILD))
			lbCanAttach = false;

		if (lbCanAttach)
		{
			// Родительское окно - ConEmu DC
			hWndParent = ghConEmuWndDC; // Надо ли его ставить сразу, если не включаем WS_CHILD?

			// WS_CHILDWINDOW перед созданием выставлять нельзя, т.к. например у WordPad.exe сносит крышу:
			// все его окна создаются нормально, но он показывает ошибку "Не удалось создать новый документ"
			//// Уберем рамку, меню и заголовок - оставим
			//dwStyle = (dwStyle | WS_CHILDWINDOW|WS_TABSTOP) & ~(WS_THICKFRAME/*|WS_CAPTION|WS_MINIMIZEBOX|WS_MAXIMIZEBOX*/);
			dwStyle &= ~WS_VISIBLE; // А вот видимость - точно сбросим
			////dwExStyle = dwExStyle & ~WS_EX_WINDOWEDGE;

			bAttachGui = TRUE;
			//gbAttachGuiClient = FALSE; // Только одно окно приложения -- сбросим после реального создания окна
			gbGuiClientAttached = TRUE; // Сразу взведем флажок режима

			#ifdef _DEBUG
			if (!ghGuiClientRetHook)
				ghGuiClientRetHook = SetWindowsHookEx(WH_CALLWNDPROCRET, GuiClientRetHook, NULL, GetCurrentThreadId());
			#endif
		}
		return true;
	}

	if (gbGuiClientAttached /*ghAttachGuiClient*/)
	{
		return true; // В GUI приложениях - разрешено все
	}

#ifndef _DEBUG
	return true;
#else
	if (gnHookMainThreadId && gnHookMainThreadId != GetCurrentThreadId())
		return true; // Разрешено, отдается на откуп консольной программе/плагинам
	
	if ((dwStyle & (WS_POPUP|DS_MODALFRAME)) == (WS_POPUP|DS_MODALFRAME))
	{
		// Это скорее всего обычный диалог, разрешим, но пока для отладчика - assert
		_ASSERTE((dwStyle & WS_POPUP) == 0);
		return true;
	}

	if ((lpClassNameA && ((DWORD_PTR)lpClassNameA) <= 0xFFFF)
		|| (lpClassNameW && ((DWORD_PTR)lpClassNameW) <= 0xFFFF))
	{
		// Что-то системное
		return true;
	}
	
	// Окно на любой чих создается. dwStyle == 0x88000000.
	if ((lpClassNameW && lstrcmpW(lpClassNameW, L"CicMarshalWndClass") == 0)
		|| (lpClassNameA && lstrcmpA(lpClassNameA, "CicMarshalWndClass") == 0))
	{
		return true;
	}

	#ifdef _DEBUG
	// В консоли нет обработчика сообщений, поэтому создание окон в главной
	// нити приводит к "зависанию" приложения - например, любые программы,
	// использующие DDE будут виснуть.
	_ASSERTE(dwStyle == 0 && FALSE);
	//SetLastError(ERROR_THREAD_MODE_NOT_BACKGROUND);
	//return false;
	#endif
	
	// Разрешить? По настройке?
	return true;
#endif
}

static void OnGuiWindowAttached(HWND hWindow, HMENU hMenu, LPCSTR asClassA, LPCWSTR asClassW, DWORD anStyle, DWORD anStyleEx, BOOL abStyleHidden)
{
	ghAttachGuiClient = hWindow;
	gbForceShowGuiClient = TRUE;
	gbAttachGuiClient = FALSE; // Только одно окно приложения. Пока?

#if 0
	// Для WS_CHILDWINDOW меню нельзя указать при создании окна
	if (!hMenu && !ghAttachGuiClientMenu && (asClassA || asClassW))
	{
		BOOL lbRcClass;
		WNDCLASSEXA wca = {sizeof(WNDCLASSEXA)};
		WNDCLASSEXW wcw = {sizeof(WNDCLASSEXW)};
		if (asClassA)
		{
			lbRcClass = GetClassInfoExA(GetModuleHandle(NULL), asClassA, &wca);
			if (lbRcClass)
				ghAttachGuiClientMenu = LoadMenuA(wca.hInstance, wca.lpszMenuName);
		}
		else
		{
			lbRcClass = GetClassInfoExW(GetModuleHandle(NULL), asClassW, &wcw);
			if (lbRcClass)
				ghAttachGuiClientMenu = LoadMenuW(wca.hInstance, wcw.lpszMenuName);
		}
		hMenu = ghAttachGuiClientMenu;
	}
	if (hMenu)
	{
		// Для WS_CHILDWINDOW - не работает
		SetMenu(hWindow, hMenu);
		HMENU hSys = GetSystemMenu(hWindow, FALSE);
		TODO("Это в принципе прокатывает, но нужно транслировать WM_SYSCOMMAND -> WM_COMMAND, соответственно, перехватывать WndProc, или хук ставить");
		if (hSys)
		{
			TODO("Хотя, хорошо бы не все в Popup засоывать, а извлечь ChildPopups из hMenu");
			InsertMenu(hSys, 0, MF_BYPOSITION|MF_POPUP, (UINT_PTR)hMenu, L"Window menu");
			InsertMenu(hSys, 1, MF_BYPOSITION|MF_SEPARATOR, NULL, NULL);
		}
	}
#endif

	DWORD nCurStyle = (DWORD)MyGetWindowLongPtrW(hWindow, GWL_STYLE);
	DWORD nCurStyleEx = (DWORD)MyGetWindowLongPtrW(hWindow, GWL_EXSTYLE);

	DWORD nSize = sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_ATTACHGUIAPP);
	CESERVER_REQ *pIn = (CESERVER_REQ*)calloc(nSize,1);
	ExecutePrepareCmd(pIn, CECMD_ATTACHGUIAPP, nSize);

	pIn->AttachGuiApp.bOk = TRUE;
	pIn->AttachGuiApp.nPID = GetCurrentProcessId();
	pIn->AttachGuiApp.hWindow = hWindow;
	pIn->AttachGuiApp.nStyle = nCurStyle; // стили могли измениться после создания окна,
	pIn->AttachGuiApp.nStyleEx = nCurStyleEx; // поэтому получим актуальные
	GetWindowRect(hWindow, &pIn->AttachGuiApp.rcWindow);
	GetModuleFileName(NULL, pIn->AttachGuiApp.sAppFileName, countof(pIn->AttachGuiApp.sAppFileName));

	wchar_t szGuiPipeName[128];
	msprintf(szGuiPipeName, countof(szGuiPipeName), CEGUIPIPENAME, L".", (DWORD)ghConEmuWnd);

	CESERVER_REQ* pOut = ExecuteCmd(szGuiPipeName, pIn, 1000, NULL);

	free(pIn);

	// abStyleHidden == TRUE, если окно при создании указало флаг WS_VISIBLE (т.е. не собиралось звать ShowWindow)

	if (pOut)
	{
		if (pOut->hdr.cbSize > sizeof(CESERVER_REQ_HDR))
		{
			_ASSERTE(pOut->AttachGuiApp.bOk);

			if (!(nCurStyle & WS_CHILDWINDOW))
			{
				nCurStyle = (nCurStyle | WS_CHILDWINDOW|WS_TABSTOP); // & ~(WS_THICKFRAME/*|WS_CAPTION|WS_MINIMIZEBOX|WS_MAXIMIZEBOX*/);
				MySetWindowLongPtrW(hWindow, GWL_STYLE, nCurStyle);
				SetParent(hWindow, ghConEmuWndDC);
			}
			
			RECT rcGui = grcAttachGuiClientPos = pOut->AttachGuiApp.rcWindow;
			if (MySetWindowPos(hWindow, HWND_TOP, rcGui.left,rcGui.top, rcGui.right-rcGui.left, rcGui.bottom-rcGui.top,
				SWP_DRAWFRAME | SWP_FRAMECHANGED | (abStyleHidden ? SWP_SHOWWINDOW : 0)))
			{
				if (abStyleHidden)
					abStyleHidden = FALSE;
			}
		}
		ExecuteFreeResult(pOut);
	}

	if (abStyleHidden)
	{
		ShowWindow(hWindow, SW_SHOW);
	}
}


BOOL WINAPI OnShowWindow(HWND hWnd, int nCmdShow)
{
	typedef BOOL (WINAPI* OnShowWindow_t)(HWND hWnd, int nCmdShow);
	ORIGINALFASTEX(ShowWindow,NULL);
	BOOL lbRc = FALSE, lbGuiAttach = FALSE;
	
	if (ghConEmuWndDC && hWnd == ghConEmuWndDC)
	{
		_ASSERTE(hWnd != ghConEmuWndDC);
		return TRUE; // обманем
	}

	if (gbForceShowGuiClient && (ghAttachGuiClient == hWnd))
	{
		RECT rcGui = grcAttachGuiClientPos;
		MySetWindowPos(hWnd, HWND_TOP, rcGui.left,rcGui.top, rcGui.right-rcGui.left, rcGui.bottom-rcGui.top,
			SWP_FRAMECHANGED);
	
		nCmdShow = SW_SHOWNORMAL;
		gbForceShowGuiClient = FALSE; // Один раз?
		lbGuiAttach = TRUE;
	}

	if (F(ShowWindow))
		lbRc = F(ShowWindow)(hWnd, nCmdShow);

	if (lbGuiAttach)
	{
		//SetFocus(ghConEmuWnd);
		RECT rcGui = grcAttachGuiClientPos;
		//MySetWindowPos(hWnd, HWND_TOP, rcGui.left,rcGui.top, rcGui.right-rcGui.left, rcGui.bottom-rcGui.top,
		//	SWP_FRAMECHANGED|SWP_SHOWWINDOW);
		GetClientRect(hWnd, &rcGui);
		SendMessage(hWnd, WM_SIZE, SIZE_RESTORED, MAKELONG((rcGui.right-rcGui.left),(rcGui.bottom-rcGui.top)));
	}

	return lbRc;
}


HWND WINAPI OnSetParent(HWND hWndChild, HWND hWndNewParent)
{
	typedef HWND (WINAPI* OnSetParent_t)(HWND hWndChild, HWND hWndNewParent);
	ORIGINALFASTEX(SetParent,NULL);
	HWND lhRc = NULL;

	if (ghConEmuWndDC && hWndChild == ghConEmuWndDC)
	{
		_ASSERTE(hWndChild != ghConEmuWndDC);
		return NULL; // обманем
	}
	
	if (hWndNewParent && ghConEmuWndDC)
	{
		_ASSERTE(hWndNewParent!=ghConEmuWnd);
		//hWndNewParent = ghConEmuWndDC;
	}

	if (F(SetParent))
	{
		lhRc = F(SetParent)(hWndChild, hWndNewParent);
	}

	return lhRc;
}


HWND WINAPI OnGetParent(HWND hWnd)
{
	typedef HWND (WINAPI* OnGetParent_t)(HWND hWnd);
	ORIGINALFASTEX(GetParent,NULL);
	HWND lhRc = NULL;

	if (ghConEmuWndDC && hWnd == ghConEmuWndDC)
	{
		hWnd = ghConEmuWnd;
	}

	if (F(GetParent))
	{
		lhRc = F(GetParent)(hWnd);
	}

	return lhRc;
}


HWND WINAPI OnGetWindow(HWND hWnd, UINT uCmd)
{
	typedef HWND (WINAPI* OnGetWindow_t)(HWND hWnd, UINT uCmd);
	ORIGINALFASTEX(GetWindow,NULL);
	HWND lhRc = NULL;

	if (ghConEmuWndDC && (hWnd == ghConEmuWndDC) && (uCmd == GW_OWNER))
	{
		hWnd = ghConEmuWnd;
	}

	if (F(GetWindow))
	{
		lhRc = F(GetWindow)(hWnd, uCmd);
	}

	return lhRc;
}


HWND WINAPI OnGetAncestor(HWND hWnd, UINT gaFlags)
{
	typedef HWND (WINAPI* OnGetAncestor_t)(HWND hWnd, UINT gaFlags);
	ORIGINALFASTEX(GetAncestor,NULL);
	HWND lhRc = NULL;

	if (ghConEmuWndDC && hWnd == ghConEmuWndDC)
	{
		hWnd = ghConEmuWnd;
	}

	if (F(GetAncestor))
	{
		lhRc = F(GetAncestor)(hWnd, gaFlags);
	}

	return lhRc;
}

BOOL WINAPI OnMoveWindow(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint)
{
	typedef BOOL (WINAPI* OnMoveWindow_t)(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint);
	ORIGINALFASTEX(MoveWindow,NULL);
	BOOL lbRc = FALSE;

	if (ghConEmuWndDC && hWnd == ghConEmuWndDC)
	{
		return TRUE; // обманем. приложениям запрещено "двигать" ConEmuDC
	}

	if (F(MoveWindow))
		lbRc = F(MoveWindow)(hWnd, X, Y, nWidth, nHeight, bRepaint);

	return lbRc;
}

BOOL WINAPI OnSetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
	typedef BOOL (WINAPI* OnSetWindowPos_t)(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);
	ORIGINALFASTEX(SetWindowPos,NULL);
	BOOL lbRc = FALSE;

	if (ghConEmuWndDC && hWnd == ghConEmuWndDC)
	{
		return TRUE; // обманем. приложениям запрещено "двигать" ConEmuDC
	}

	if (F(SetWindowPos))
		lbRc = F(SetWindowPos)(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);

	return lbRc;
}

BOOL WINAPI OnSetWindowPlacement(HWND hWnd, WINDOWPLACEMENT *lpwndpl)
{
	typedef BOOL (WINAPI* OnSetWindowPlacement_t)(HWND hWnd, WINDOWPLACEMENT *lpwndpl);
	ORIGINALFASTEX(SetWindowPlacement,NULL);
	BOOL lbRc = FALSE;

	if (ghConEmuWndDC && hWnd == ghConEmuWndDC)
	{
		return TRUE; // обманем. приложениям запрещено "двигать" ConEmuDC
	}

	if (F(SetWindowPlacement))
		lbRc = F(SetWindowPlacement)(hWnd, lpwndpl);

	return lbRc;
}

bool CanSendMessage(HWND& hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT& lRc)
{
	if (ghConEmuWndDC && hWnd == ghConEmuWndDC)
	{
		switch (Msg)
		{
			case WM_SIZE:
			case WM_MOVE:
			case WM_SHOWWINDOW:
				// Эти сообщения - вообще игнорировать
				return false;
			case WM_INPUTLANGCHANGEREQUEST:
			case WM_INPUTLANGCHANGE:
			case WM_QUERYENDSESSION:
			case WM_ENDSESSION:
			case WM_QUIT:
			case WM_DESTROY:
			case WM_CLOSE:
			case WM_ENABLE:
			case WM_SETREDRAW:
				// Эти сообщения - нужно посылать в RealConsole
				hWnd = ghConWnd;
				return true;
			case WM_SETICON:
				TODO("Можно бы передать иконку в ConEmu и показать ее на табе");
				break;
		}
	}
	return true; // разрешено
}

BOOL WINAPI OnPostMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	typedef BOOL (WINAPI* OnPostMessageA_t)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	ORIGINALFASTEX(PostMessageA,NULL);
	LRESULT lRc = 0;

	if (!CanSendMessage(hWnd, Msg, wParam, lParam, lRc))
		return lRc; // Обманем, это сообщение запрещено посылать в ConEmuDC

	if (F(PostMessageA))
		lRc = F(PostMessageA)(hWnd, Msg, wParam, lParam);

	return lRc;
}
BOOL WINAPI OnPostMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	typedef BOOL (WINAPI* OnPostMessageW_t)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	ORIGINALFASTEX(PostMessageW,NULL);
	LRESULT lRc = 0;

	if (!CanSendMessage(hWnd, Msg, wParam, lParam, lRc))
		return lRc; // Обманем, это сообщение запрещено посылать в ConEmuDC

	if (F(PostMessageW))
		lRc = F(PostMessageW)(hWnd, Msg, wParam, lParam);

	return lRc;
}
BOOL WINAPI OnSendMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	typedef BOOL (WINAPI* OnSendMessageA_t)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	ORIGINALFASTEX(SendMessageA,NULL);
	LRESULT lRc = 0;

	if (!CanSendMessage(hWnd, Msg, wParam, lParam, lRc))
		return lRc; // Обманем, это сообщение запрещено посылать в ConEmuDC

	if (F(SendMessageA))
		lRc = F(SendMessageA)(hWnd, Msg, wParam, lParam);

	return lRc;
}
BOOL WINAPI OnSendMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	typedef BOOL (WINAPI* OnSendMessageW_t)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	ORIGINALFASTEX(SendMessageW,NULL);
	LRESULT lRc = 0;

	if (!CanSendMessage(hWnd, Msg, wParam, lParam, lRc))
		return lRc; // Обманем, это сообщение запрещено посылать в ConEmuDC

	if (F(SendMessageW))
		lRc = F(SendMessageW)(hWnd, Msg, wParam, lParam);

	return lRc;
}


HWND WINAPI OnCreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	typedef HWND (WINAPI* OnCreateWindowExA_t)(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
	ORIGINALFASTEX(CreateWindowExA,NULL);
	HWND hWnd = NULL;
	BOOL bAttachGui = FALSE, bStyleHidden = FALSE;
	DWORD lStyle = dwStyle, lStyleEx = dwExStyle;

	if (CheckCanCreateWindow(lpClassName, NULL, dwStyle, dwExStyle, hWndParent, bAttachGui, bStyleHidden) && F(CreateWindowExA) != NULL)
	{
		if (bAttachGui)
		{
			x = grcConEmuClient.left; y = grcConEmuClient.top;
			nWidth = grcConEmuClient.right - grcConEmuClient.left; nHeight = grcConEmuClient.bottom - grcConEmuClient.top;
		}

		hWnd = F(CreateWindowExA)(dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

		if (hWnd && bAttachGui)
		{
			OnGuiWindowAttached(hWnd, hMenu, lpClassName, NULL, lStyle, lStyleEx, bStyleHidden);
		}
	}

	return hWnd;
}


HWND WINAPI OnCreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	typedef HWND (WINAPI* OnCreateWindowExW_t)(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
	ORIGINALFASTEX(CreateWindowExW,NULL);
	HWND hWnd = NULL;
	BOOL bAttachGui = FALSE, bStyleHidden = FALSE;
	DWORD lStyle = dwStyle, lStyleEx = dwExStyle;

	if (CheckCanCreateWindow(NULL, lpClassName, dwStyle, dwExStyle, hWndParent, bAttachGui, bStyleHidden) && F(CreateWindowExW) != NULL)
	{
		if (bAttachGui)
		{
			x = grcConEmuClient.left; y = grcConEmuClient.top;
			nWidth = grcConEmuClient.right - grcConEmuClient.left; nHeight = grcConEmuClient.bottom - grcConEmuClient.top;
		}

		hWnd = F(CreateWindowExW)(dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

		if (hWnd && bAttachGui)
		{
			OnGuiWindowAttached(hWnd, hMenu, NULL, lpClassName, lStyle, lStyleEx, bStyleHidden);
		}
	}

	return hWnd;
}

#ifndef NORM_STOP_ON_NULL
#define NORM_STOP_ON_NULL 0x10000000
#endif

// ANSI хукать смысла нет, т.к. FAR 1.x сравнивает сам
int WINAPI OnCompareStringW(LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2)
{
	typedef int (WINAPI* OnCompareStringW_t)(LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2);
	ORIGINALFAST(CompareStringW);
	int nCmp = -1;

	if (gFarMode.bFarHookMode && gFarMode.bFARuseASCIIsort)
	{
		if (dwCmpFlags == (NORM_IGNORECASE|NORM_STOP_ON_NULL|SORT_STRINGSORT))
		{
			int n = 0;
			WCHAR ch1 = *lpString1++, /*ch10 = 0,*/ ch2 = *lpString2++ /*,ch20 = 0*/;
			int n1 = (cchCount1==cchCount2) ? cchCount1
			         : (cchCount1!=-1 && cchCount2!=-1) ? -1
			         : (cchCount1!=-1) ? cchCount2
			         : (cchCount2!=-1) ? cchCount1 : min(cchCount1,cchCount2);

			while(!n && /*(ch1=*lpString1++) && (ch2=*lpString2++) &&*/ n1--)
			{
				if (ch1==ch2)
				{
					if (!ch1) break;

					// n = 0;
				}
				else if (ch1<0x80 && ch2<0x80)
				{
					if (ch1>=L'A' && ch1<=L'Z') ch1 |= 0x20; //-V112

					if (ch2>=L'A' && ch2<=L'Z') ch2 |= 0x20; //-V112

					n = (ch1==ch2) ? 0 : (ch1<ch2) ? -1 : 1;
				}
				else
				{
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


DWORD WINAPI OnGetConsoleAliasesW(LPWSTR AliasBuffer, DWORD AliasBufferLength, LPWSTR ExeName)
{
	typedef DWORD (WINAPI* OnGetConsoleAliasesW_t)(LPWSTR AliasBuffer, DWORD AliasBufferLength, LPWSTR ExeName);
	ORIGINALFAST(GetConsoleAliasesW);
	DWORD nError = 0;
	DWORD nRc = F(GetConsoleAliasesW)(AliasBuffer,AliasBufferLength,ExeName);

	if (!nRc)
	{
		nError = GetLastError();

		// финт ушами
		if (nError == ERROR_NOT_ENOUGH_MEMORY) // && gdwServerPID)
		{
			DWORD nServerPID = 0;
			HWND hConWnd = GetConsoleWindow();
			_ASSERTE(hConWnd == ghConWnd);
			MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> ConInfo;
			ConInfo.InitName(CECONMAPNAME, (DWORD)hConWnd); //-V205
			CESERVER_CONSOLE_MAPPING_HDR *pInfo = ConInfo.Open();
			if (pInfo 
				&& (pInfo->cbSize >= sizeof(CESERVER_CONSOLE_MAPPING_HDR))
				//&& (pInfo->nProtocolVersion == CESERVER_REQ_VER)
				)
			{
				nServerPID = pInfo->nServerPID;
				ConInfo.CloseMap();
			}

			if (nServerPID)
			{
				CESERVER_REQ_HDR In;
				ExecutePrepareCmd((CESERVER_REQ*)&In, CECMD_GETALIASES,sizeof(CESERVER_REQ_HDR));
				CESERVER_REQ* pOut = ExecuteSrvCmd(nServerPID/*gdwServerPID*/, (CESERVER_REQ*)&In, hConWnd);

				if (pOut)
				{
					size_t nData = min(AliasBufferLength,(pOut->hdr.cbSize-sizeof(pOut->hdr)));

					if (nData)
					{
						memmove(AliasBuffer, pOut->Data, nData);
						nRc = TRUE;
					}

					ExecuteFreeResult(pOut);
				}
			}
		}

		if (!nRc)
			SetLastError(nError); // вернуть, вдруг какая функция его поменяла
	}

	return nRc;
}


// Microsoft bug?
// http://code.google.com/p/conemu-maximus5/issues/detail?id=60
typedef BOOL (WINAPI* OnSetConsoleCP_t)(UINT wCodePageID);
typedef BOOL (WINAPI* OnSetConsoleOutputCP_t)(UINT wCodePageID);
struct SCOCP
{
	UINT  wCodePageID;
	OnSetConsoleCP_t f;
	HANDLE hReady;
	DWORD dwErrCode;
	BOOL  lbRc;
};
DWORD WINAPI SetConsoleCPThread(LPVOID lpParameter)
{
	SCOCP* p = (SCOCP*)lpParameter;
	p->dwErrCode = -1;
	SetEvent(p->hReady);
	p->lbRc = p->f(p->wCodePageID);
	p->dwErrCode = GetLastError();
	return 0;
}
BOOL WINAPI OnSetConsoleCP(UINT wCodePageID)
{
	ORIGINALFAST(SetConsoleCP);
	_ASSERTE(OnSetConsoleCP!=SetConsoleCP);
	BOOL lbRc = FALSE;
	SCOCP sco = {wCodePageID, (OnSetConsoleCP_t)F(SetConsoleCP), CreateEvent(NULL,FALSE,FALSE,NULL)};
	DWORD nTID = 0, nWait = -1, nErr;
	/*
	wchar_t szErrText[255], szTitle[64];
	msprintf(szTitle, SKIPLEN(countof(szTitle)) L"PID=%u, TID=%u", GetCurrentProcessId(), GetCurrentThreadId());
	*/
	#ifdef _DEBUG
	DWORD nPrevCP = GetConsoleCP();
	#endif
	DWORD nCurCP = 0;
	HANDLE hThread = CreateThread(NULL,0,SetConsoleCPThread,&sco,0,&nTID);

	if (!hThread)
	{
		nErr = GetLastError();
		_ASSERTE(hThread!=NULL);
		/*
		msprintf(szErrText, SKIPLEN(countof(szErrText)) L"chcp failed, ErrCode=0x%08X\nConEmuHooks.cpp:OnSetConsoleCP", nErr);
		Message BoxW(NULL, szErrText, szTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
		lbRc = FALSE;
		*/
	}
	else
	{
		HANDLE hEvents[2] = {hThread, sco.hReady};
		nWait = WaitForMultipleObjects(2, hEvents, FALSE, SETCONCP_READYTIMEOUT);

		if (nWait != WAIT_OBJECT_0)
			nWait = WaitForSingleObject(hThread, SETCONCP_TIMEOUT);

		if (nWait == WAIT_OBJECT_0)
		{
			lbRc = sco.lbRc;
		}
		else
		{
			//BUGBUG: На некоторых системых (Win2k3, WinXP) SetConsoleCP (и иже с ними) просто зависают
			TerminateThread(hThread,100);
			nCurCP = GetConsoleCP();
			if (nCurCP == wCodePageID)
			{
				lbRc = TRUE; // таки получилось
			}
			else
			{
				_ASSERTE(nCurCP == wCodePageID);
				/*
				msprintf(szErrText, SKIPLEN(countof(szErrText))
				          L"chcp hung detected\n"
				          L"ConEmuHooks.cpp:OnSetConsoleCP\n"
				          L"ReqCP=%u, PrevCP=%u, CurCP=%u\n"
				          //L"\nPress <OK> to stop waiting"
				          ,wCodePageID, nPrevCP, nCurCP);
				Message BoxW(NULL, szErrText, szTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
				*/
			}

			//nWait = WaitForSingleObject(hThread, 0);
			//if (nWait == WAIT_TIMEOUT)
			//	TerminateThread(hThread,100);
			//if (GetConsoleCP() == wCodePageID)
			//	lbRc = TRUE;
		}

		CloseHandle(hThread);
	}

	if (sco.hReady)
		CloseHandle(sco.hReady);

	return lbRc;
}
BOOL WINAPI OnSetConsoleOutputCP(UINT wCodePageID)
{
	ORIGINALFAST(SetConsoleOutputCP);
	_ASSERTE(OnSetConsoleOutputCP!=SetConsoleOutputCP);
	BOOL lbRc = FALSE;
	SCOCP sco = {wCodePageID, (OnSetConsoleCP_t)F(SetConsoleOutputCP), CreateEvent(NULL,FALSE,FALSE,NULL)};
	DWORD nTID = 0, nWait = -1, nErr;
	/*
	wchar_t szErrText[255], szTitle[64];
	msprintf(szTitle, SKIPLEN(countof(szTitle)) L"PID=%u, TID=%u", GetCurrentProcessId(), GetCurrentThreadId());
	*/
	#ifdef _DEBUG
	DWORD nPrevCP = GetConsoleOutputCP();
	#endif
	DWORD nCurCP = 0;
	HANDLE hThread = CreateThread(NULL,0,SetConsoleCPThread,&sco,0,&nTID);

	if (!hThread)
	{
		nErr = GetLastError();
		_ASSERTE(hThread!=NULL);
		/*
		msprintf(szErrText, SKIPLEN(countof(szErrText)) L"chcp(out) failed, ErrCode=0x%08X\nConEmuHooks.cpp:OnSetConsoleOutputCP", nErr);
		Message BoxW(NULL, szErrText, szTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
		lbRc = FALSE;
		*/
	}
	else
	{
		HANDLE hEvents[2] = {hThread, sco.hReady};
		nWait = WaitForMultipleObjects(2, hEvents, FALSE, SETCONCP_READYTIMEOUT);

		if (nWait != WAIT_OBJECT_0)
			nWait = WaitForSingleObject(hThread, SETCONCP_TIMEOUT);

		if (nWait == WAIT_OBJECT_0)
		{
			lbRc = sco.lbRc;
		}
		else
		{
			//BUGBUG: На некоторых системых (Win2k3, WinXP) SetConsoleCP (и иже с ними) просто зависают
			TerminateThread(hThread,100);
			nCurCP = GetConsoleOutputCP();
			if (nCurCP == wCodePageID)
			{
				lbRc = TRUE; // таки получилось
			}
			else
			{
				_ASSERTE(nCurCP == wCodePageID);
				/*
				msprintf(szErrText, SKIPLEN(countof(szErrText))
				          L"chcp(out) hung detected\n"
				          L"ConEmuHooks.cpp:OnSetConsoleOutputCP\n"
				          L"ReqOutCP=%u, PrevOutCP=%u, CurOutCP=%u\n"
				          //L"\nPress <OK> to stop waiting"
				          ,wCodePageID, nPrevCP, nCurCP);
				Message BoxW(NULL, szErrText, szTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
				*/
			}
			//nWait = WaitForSingleObject(hThread, 0);
			//if (nWait == WAIT_TIMEOUT)
			//	TerminateThread(hThread,100);
			//if (GetConsoleOutputCP() == wCodePageID)
			//	lbRc = TRUE;
		}

		CloseHandle(hThread);
	}

	if (sco.hReady)
		CloseHandle(sco.hReady);

	return lbRc;
}


BOOL WINAPI OnGetNumberOfConsoleInputEvents(HANDLE hConsoleInput, LPDWORD lpcNumberOfEvents)
{
	typedef BOOL (WINAPI* OnGetNumberOfConsoleInputEvents_t)(HANDLE hConsoleInput, LPDWORD lpcNumberOfEvents);
	ORIGINAL(GetNumberOfConsoleInputEvents);
	BOOL lbRc = FALSE;

	//if (gpFarInfo && bMainThread)
	//	TouchReadPeekConsoleInputs();

	if (ph && ph->PreCallBack)
	{
		SETARGS2(&lbRc,hConsoleInput,lpcNumberOfEvents);

		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	lbRc = F(GetNumberOfConsoleInputEvents)(hConsoleInput, lpcNumberOfEvents);

	if (ph && ph->PostCallBack)
	{
		SETARGS2(&lbRc,hConsoleInput,lpcNumberOfEvents);
		ph->PostCallBack(&args);
	}

	return lbRc;
}

//typedef BOOL (WINAPI* OnPeekConsoleInputAx_t)(HANDLE,PINPUT_RECORD,DWORD,LPDWORD);
//BOOL WINAPI OnPeekConsoleInputAx(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
//{
//	ORIGINALx(PeekConsoleInputA);
//	//if (gpFarInfo && bMainThread)
//	//	TouchReadPeekConsoleInputs(1);
//	BOOL lbRc = FALSE;
//
//	if (ph && ph->PreCallBack)
//	{
//		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);
//
//		// Если функция возвращает FALSE - реальное чтение не будет вызвано
//		if (!ph->PreCallBack(&args))
//			return lbRc;
//	}
//
//	#ifdef USE_INPUT_SEMAPHORE
//	DWORD nSemaphore = ghConInSemaphore ? WaitForSingleObject(ghConInSemaphore, INSEMTIMEOUT_READ) : 1;
//	_ASSERTE(nSemaphore<=1);
//	#endif
//
//	lbRc = Fx(PeekConsoleInputA)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);
//
//	#ifdef USE_INPUT_SEMAPHORE
//	if ((nSemaphore == WAIT_OBJECT_0) && ghConInSemaphore) ReleaseSemaphore(ghConInSemaphore, 1, NULL);
//	#endif
//
//	if (ph && ph->PostCallBack)
//	{
//		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);
//		ph->PostCallBack(&args);
//	}
//
//	return lbRc;
//}
//
//typedef BOOL (WINAPI* OnPeekConsoleInputWx_t)(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
//BOOL WINAPI OnPeekConsoleInputWx(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
//{
//	ORIGINALx(PeekConsoleInputW);
//	//if (gpFarInfo && bMainThread)
//	//	TouchReadPeekConsoleInputs(1);
//	BOOL lbRc = FALSE;
//
//	if (ph && ph->PreCallBack)
//	{
//		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);
//
//		// Если функция возвращает FALSE - реальное чтение не будет вызвано
//		if (!ph->PreCallBack(&args))
//			return lbRc;
//	}
//
//	#ifdef USE_INPUT_SEMAPHORE
//	DWORD nSemaphore = ghConInSemaphore ? WaitForSingleObject(ghConInSemaphore, INSEMTIMEOUT_READ) : 1;
//	_ASSERTE(nSemaphore<=1);
//	#endif
//
//	lbRc = Fx(PeekConsoleInputW)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);
//
//	#ifdef USE_INPUT_SEMAPHORE
//	if ((nSemaphore == WAIT_OBJECT_0) && ghConInSemaphore) ReleaseSemaphore(ghConInSemaphore, 1, NULL);
//	#endif
//
//	if (ph && ph->PostCallBack)
//	{
//		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);
//		ph->PostCallBack(&args);
//	}
//
//	return lbRc;
//}
//
//typedef BOOL (WINAPI* OnReadConsoleInputAx_t)(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
//BOOL WINAPI OnReadConsoleInputAx(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
//{
//	ORIGINALx(ReadConsoleInputA);
//	//if (gpFarInfo && bMainThread)
//	//	TouchReadPeekConsoleInputs(0);
//	BOOL lbRc = FALSE;
//
//	if (ph && ph->PreCallBack)
//	{
//		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);
//
//		// Если функция возвращает FALSE - реальное чтение не будет вызвано
//		if (!ph->PreCallBack(&args))
//			return lbRc;
//	}
//
//	#ifdef USE_INPUT_SEMAPHORE
//	DWORD nSemaphore = ghConInSemaphore ? WaitForSingleObject(ghConInSemaphore, INSEMTIMEOUT_READ) : 1;
//	_ASSERTE(nSemaphore<=1);
//	#endif
//
//	lbRc = Fx(ReadConsoleInputA)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);
//
//	#ifdef USE_INPUT_SEMAPHORE
//	if ((nSemaphore == WAIT_OBJECT_0) && ghConInSemaphore) ReleaseSemaphore(ghConInSemaphore, 1, NULL);
//	#endif
//
//	if (ph && ph->PostCallBack)
//	{
//		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);
//		ph->PostCallBack(&args);
//	}
//
//	return lbRc;
//}
//
//typedef BOOL (WINAPI* OnReadConsoleInputWx_t)(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
//BOOL WINAPI OnReadConsoleInputWx(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
//{
//	ORIGINALx(ReadConsoleInputW);
//	//if (gpFarInfo && bMainThread)
//	//	TouchReadPeekConsoleInputs(0);
//	BOOL lbRc = FALSE;
//
//	if (ph && ph->PreCallBack)
//	{
//		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);
//
//		// Если функция возвращает FALSE - реальное чтение не будет вызвано
//		if (!ph->PreCallBack(&args))
//			return lbRc;
//	}
//
//	#ifdef USE_INPUT_SEMAPHORE
//	DWORD nSemaphore = ghConInSemaphore ? WaitForSingleObject(ghConInSemaphore, INSEMTIMEOUT_READ) : 1;
//	_ASSERTE(nSemaphore<=1);
//	#endif
//
//	lbRc = Fx(ReadConsoleInputW)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);
//
//	#ifdef USE_INPUT_SEMAPHORE
//	if ((nSemaphore == WAIT_OBJECT_0) && ghConInSemaphore) ReleaseSemaphore(ghConInSemaphore, 1, NULL);
//	#endif
//
//	if (ph && ph->PostCallBack)
//	{
//		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);
//		ph->PostCallBack(&args);
//	}
//
//	return lbRc;
//}

// Для нотификации вкладки Debug в ConEmu
void OnPeekReadConsoleInput(char acPeekRead/*'P'/'R'*/, char acUnicode/*'A'/'W'*/, HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nRead)
{
#ifdef TRAP_ON_MOUSE_0x0
	if (lpBuffer && nRead)
	{
		for (UINT i = 0; i < nRead; i++)
		{
			if (lpBuffer[i].EventType == MOUSE_EVENT)
			{
				if (lpBuffer[i].Event.MouseEvent.dwMousePosition.X == 0 && lpBuffer[i].Event.MouseEvent.dwMousePosition.Y == 0)
				{
					_ASSERTE(!(lpBuffer[i].Event.MouseEvent.dwMousePosition.X == 0 && lpBuffer[i].Event.MouseEvent.dwMousePosition.Y == 0));
				}
				//if (lpBuffer[i].Event.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED && lpBuffer[i].Event.MouseEvent.dwMousePosition.X != 5)
				//{
				//	_ASSERTE(!(lpBuffer[i].Event.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED && lpBuffer[i].Event.MouseEvent.dwMousePosition.X != 5));
				//}
			}
		}
	}
#endif

	if (!gFarMode.bFarHookMode || !gFarMode.bMonitorConsoleInput || !nRead || !lpBuffer)
		return;
		
	//// Пока - только Read. Peek игнорируем
	//if (acPeekRead != 'R')
	//	return;
	
	CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_PEEKREADINFO, sizeof(CESERVER_REQ_HDR) //-V119
		+sizeof(CESERVER_REQ_PEEKREADINFO)+(nRead-1)*sizeof(INPUT_RECORD));
	if (pIn)
	{
		pIn->PeekReadInfo.nCount = (WORD)nRead;
		pIn->PeekReadInfo.cPeekRead = acPeekRead;
		pIn->PeekReadInfo.cUnicode = acUnicode;
		pIn->PeekReadInfo.h = hConsoleInput;
		pIn->PeekReadInfo.nTID = GetCurrentThreadId();
		pIn->PeekReadInfo.nPID = GetCurrentProcessId();
		pIn->PeekReadInfo.bMainThread = (pIn->PeekReadInfo.nTID == gnHookMainThreadId);
		memmove(pIn->PeekReadInfo.Buffer, lpBuffer, nRead*sizeof(INPUT_RECORD));
	
		CESERVER_REQ* pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
		if (pOut) ExecuteFreeResult(pOut);
		ExecuteFreeResult(pIn);
	}
}

#ifdef _DEBUG
void PreWriteConsoleInput(BOOL abUnicode, const INPUT_RECORD *lpBuffer, DWORD nLength)
{
#ifdef TRAP_ON_MOUSE_0x0
	if (lpBuffer && nLength)
	{
		for (UINT i = 0; i < nLength; i++)
		{
			if (lpBuffer[i].EventType == MOUSE_EVENT)
			{
				if (lpBuffer[i].Event.MouseEvent.dwMousePosition.X == 0 && lpBuffer[i].Event.MouseEvent.dwMousePosition.Y == 0)
				{
					_ASSERTE(!(lpBuffer[i].Event.MouseEvent.dwMousePosition.X == 0 && lpBuffer[i].Event.MouseEvent.dwMousePosition.Y == 0));
				}
				//if (lpBuffer[i].Event.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED && lpBuffer[i].Event.MouseEvent.dwMousePosition.X != 5)
				//{
				//	_ASSERTE(!(lpBuffer[i].Event.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED && lpBuffer[i].Event.MouseEvent.dwMousePosition.X != 5));
				//}
			}
		}
	}
#endif
}
#endif


BOOL WINAPI OnPeekConsoleInputA(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
	typedef BOOL (WINAPI* OnPeekConsoleInputA_t)(HANDLE,PINPUT_RECORD,DWORD,LPDWORD);
	ORIGINAL(PeekConsoleInputA);
	//if (gpFarInfo && bMainThread)
	//	TouchReadPeekConsoleInputs(1);
	BOOL lbRc = FALSE;

	if (ph && ph->PreCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);

		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	//#ifdef USE_INPUT_SEMAPHORE
	//DWORD nSemaphore = ghConInSemaphore ? WaitForSingleObject(ghConInSemaphore, INSEMTIMEOUT_READ) : 1;
	//_ASSERTE(nSemaphore<=1);
	//#endif

	lbRc = F(PeekConsoleInputA)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);

	//#ifdef USE_INPUT_SEMAPHORE
	//if ((nSemaphore == WAIT_OBJECT_0) && ghConInSemaphore) ReleaseSemaphore(ghConInSemaphore, 1, NULL);
	//#endif
	
	if (ph && ph->PostCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);
		ph->PostCallBack(&args);
	}
	
	if (lbRc && lpNumberOfEventsRead && *lpNumberOfEventsRead && lpBuffer)
		OnPeekReadConsoleInput('P', 'A', hConsoleInput, lpBuffer, *lpNumberOfEventsRead);

	return lbRc;
}


BOOL WINAPI OnPeekConsoleInputW(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
	typedef BOOL (WINAPI* OnPeekConsoleInputW_t)(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
	ORIGINAL(PeekConsoleInputW);
	//if (gpFarInfo && bMainThread)
	//	TouchReadPeekConsoleInputs(1);
	BOOL lbRc = FALSE;

	if (ph && ph->PreCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);

		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	//#ifdef USE_INPUT_SEMAPHORE
	//DWORD nSemaphore = ghConInSemaphore ? WaitForSingleObject(ghConInSemaphore, INSEMTIMEOUT_READ) : 1;
	//_ASSERTE(nSemaphore<=1);
	//#endif

	lbRc = F(PeekConsoleInputW)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);

	//#ifdef USE_INPUT_SEMAPHORE
	//if ((nSemaphore == WAIT_OBJECT_0) && ghConInSemaphore) ReleaseSemaphore(ghConInSemaphore, 1, NULL);
	//#endif

	if (ph && ph->PostCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);
		ph->PostCallBack(&args);
	}

	if (lbRc && lpNumberOfEventsRead && *lpNumberOfEventsRead && lpBuffer)
		OnPeekReadConsoleInput('P', 'W', hConsoleInput, lpBuffer, *lpNumberOfEventsRead);
//#ifdef _DEBUG
//	wchar_t szDbg[128];
//	if (lbRc && lpNumberOfEventsRead && *lpNumberOfEventsRead && lpBuffer && lpBuffer->EventType == MOUSE_EVENT)
//		wsprintfW(szDbg, L"ConEmuHk.OnPeekConsoleInputW(x%04X,x%04X) %u\n",
//			lpBuffer->Event.MouseEvent.dwButtonState, lpBuffer->Event.MouseEvent.dwControlKeyState, *lpNumberOfEventsRead);
//	else
//		lstrcpyW(szDbg, L"ConEmuHk.OnPeekConsoleInputW(Non mouse event)\n");
//	OutputDebugStringW(szDbg);
//#endif

	return lbRc;
}


BOOL WINAPI OnReadConsoleInputA(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
	typedef BOOL (WINAPI* OnReadConsoleInputA_t)(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
	ORIGINAL(ReadConsoleInputA);
	//if (gpFarInfo && bMainThread)
	//	TouchReadPeekConsoleInputs(0);
	BOOL lbRc = FALSE;

	if (ph && ph->PreCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);

		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	//#ifdef USE_INPUT_SEMAPHORE
	//DWORD nSemaphore = ghConInSemaphore ? WaitForSingleObject(ghConInSemaphore, INSEMTIMEOUT_READ) : 1;
	//_ASSERTE(nSemaphore<=1);
	//#endif

	lbRc = F(ReadConsoleInputA)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);

	//#ifdef USE_INPUT_SEMAPHORE
	//if ((nSemaphore == WAIT_OBJECT_0) && ghConInSemaphore) ReleaseSemaphore(ghConInSemaphore, 1, NULL);
	//#endif

	if (ph && ph->PostCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);
		ph->PostCallBack(&args);
	}

	if (lbRc && lpNumberOfEventsRead && *lpNumberOfEventsRead && lpBuffer)
		OnPeekReadConsoleInput('R', 'A', hConsoleInput, lpBuffer, *lpNumberOfEventsRead);

	return lbRc;
}


BOOL WINAPI OnReadConsoleInputW(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
	typedef BOOL (WINAPI* OnReadConsoleInputW_t)(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
	ORIGINAL(ReadConsoleInputW);
	//if (gpFarInfo && bMainThread)
	//	TouchReadPeekConsoleInputs(0);
	BOOL lbRc = FALSE;

	if (ph && ph->PreCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);

		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	//#ifdef USE_INPUT_SEMAPHORE
	//DWORD nSemaphore = ghConInSemaphore ? WaitForSingleObject(ghConInSemaphore, INSEMTIMEOUT_READ) : 1;
	//_ASSERTE(nSemaphore<=1);
	//#endif

	lbRc = F(ReadConsoleInputW)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);

	//#ifdef USE_INPUT_SEMAPHORE
	//if ((nSemaphore == WAIT_OBJECT_0) && ghConInSemaphore) ReleaseSemaphore(ghConInSemaphore, 1, NULL);
	//#endif

	if (ph && ph->PostCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);
		ph->PostCallBack(&args);
	}

	if (lbRc && lpNumberOfEventsRead && *lpNumberOfEventsRead && lpBuffer)
		OnPeekReadConsoleInput('R', 'W', hConsoleInput, lpBuffer, *lpNumberOfEventsRead);

	return lbRc;
}


BOOL WINAPI OnWriteConsoleInputA(HANDLE hConsoleInput, const INPUT_RECORD *lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsWritten)
{
	typedef BOOL (WINAPI* OnWriteConsoleInputA_t)(HANDLE hConsoleInput, const INPUT_RECORD *lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsWritten);
	ORIGINAL(WriteConsoleInputA);
	BOOL lbRc = FALSE;

	#ifdef _DEBUG
	PreWriteConsoleInput(FALSE, lpBuffer, nLength);
	#endif

	if (ph && ph->PreCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsWritten);

		// Если функция возвращает FALSE - реальная запись не будет вызвана
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	lbRc = F(WriteConsoleInputA)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsWritten);

	if (ph && ph->PostCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsWritten);
		ph->PostCallBack(&args);
	}

	return lbRc;
}


BOOL WINAPI OnWriteConsoleInputW(HANDLE hConsoleInput, const INPUT_RECORD *lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsWritten)
{
	typedef BOOL (WINAPI* OnWriteConsoleInputW_t)(HANDLE hConsoleInput, const INPUT_RECORD *lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsWritten);
	ORIGINAL(WriteConsoleInputW);
	BOOL lbRc = FALSE;

	#ifdef _DEBUG
	PreWriteConsoleInput(FALSE, lpBuffer, nLength);
	#endif

	if (ph && ph->PreCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsWritten);

		// Если функция возвращает FALSE - реальная запись не будет вызвана
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	lbRc = F(WriteConsoleInputW)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsWritten);

	if (ph && ph->PostCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsWritten);
		ph->PostCallBack(&args);
	}

	return lbRc;
}


HANDLE WINAPI OnCreateConsoleScreenBuffer(DWORD dwDesiredAccess, DWORD dwShareMode, const SECURITY_ATTRIBUTES *lpSecurityAttributes, DWORD dwFlags, LPVOID lpScreenBufferData)
{
	typedef HANDLE(WINAPI* OnCreateConsoleScreenBuffer_t)(DWORD dwDesiredAccess, DWORD dwShareMode, const SECURITY_ATTRIBUTES *lpSecurityAttributes, DWORD dwFlags, LPVOID lpScreenBufferData);
	ORIGINALFAST(CreateConsoleScreenBuffer);

	if ((dwShareMode & (FILE_SHARE_READ|FILE_SHARE_WRITE)) != (FILE_SHARE_READ|FILE_SHARE_WRITE))
		dwShareMode |= (FILE_SHARE_READ|FILE_SHARE_WRITE);

	HANDLE h;
	h = F(CreateConsoleScreenBuffer)(dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwFlags, lpScreenBufferData);
	return h;
}


BOOL WINAPI OnAllocConsole(void)
{
	typedef BOOL (WINAPI* OnAllocConsole_t)(void);
	ORIGINALFAST(AllocConsole);
	BOOL bMainThread = (GetCurrentThreadId() == gnHookMainThreadId);
	BOOL lbRc = FALSE;

	if (ph && ph->PreCallBack)
	{
		SETARGS(&lbRc);

		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	lbRc = F(AllocConsole)();

	//InitializeConsoleInputSemaphore();

	if (ph && ph->PostCallBack)
	{
		SETARGS(&lbRc);
		ph->PostCallBack(&args);
	}

	return lbRc;
}


BOOL WINAPI OnFreeConsole(void)
{
	typedef BOOL (WINAPI* OnFreeConsole_t)(void);
	ORIGINALFAST(FreeConsole);
	BOOL bMainThread = (GetCurrentThreadId() == gnHookMainThreadId);
	BOOL lbRc = FALSE;

	if (ph && ph->PreCallBack)
	{
		SETARGS(&lbRc);

		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	//ReleaseConsoleInputSemaphore();

	lbRc = F(FreeConsole)();

	if (ph && ph->PostCallBack)
	{
		SETARGS(&lbRc);
		ph->PostCallBack(&args);
	}

	return lbRc;
}


HWND WINAPI OnGetConsoleWindow(void)
{
	typedef HWND (WINAPI* OnGetConsoleWindow_t)(void);
	ORIGINALFAST(GetConsoleWindow);

	if (ghConEmuWndDC && IsWindow(ghConEmuWndDC) /*ghConsoleHwnd*/)
	{
		//return ghConsoleHwnd;
		return ghConEmuWndDC;
	}

	HWND h;
	h = F(GetConsoleWindow)();
	return h;
}


BOOL WINAPI OnWriteConsoleOutputA(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion)
{
	typedef BOOL (WINAPI* OnWriteConsoleOutputA_t)(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
	ORIGINAL(WriteConsoleOutputA);
	BOOL lbRc = FALSE;

	if (ph && ph->PreCallBack)
	{
		SETARGS5(&lbRc, hConsoleOutput, lpBuffer, &dwBufferSize, &dwBufferCoord, lpWriteRegion);
		ph->PreCallBack(&args);
	}

	lbRc = F(WriteConsoleOutputA)(hConsoleOutput, lpBuffer, dwBufferSize, dwBufferCoord, lpWriteRegion);

	if (ph && ph->PostCallBack)
	{
		SETARGS5(&lbRc, hConsoleOutput, lpBuffer, &dwBufferSize, &dwBufferCoord, lpWriteRegion);
		ph->PostCallBack(&args);
	}

	return lbRc;
}


BOOL WINAPI OnWriteConsoleOutputW(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion)
{
	typedef BOOL (WINAPI* OnWriteConsoleOutputW_t)(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
	ORIGINAL(WriteConsoleOutputW);
	BOOL lbRc = FALSE;

	if (ph && ph->PreCallBack)
	{
		SETARGS5(&lbRc, hConsoleOutput, lpBuffer, &dwBufferSize, &dwBufferCoord, lpWriteRegion);
		ph->PreCallBack(&args);
	}

	lbRc = F(WriteConsoleOutputW)(hConsoleOutput, lpBuffer, dwBufferSize, dwBufferCoord, lpWriteRegion);

	if (ph && ph->PostCallBack)
	{
		SETARGS5(&lbRc, hConsoleOutput, lpBuffer, &dwBufferSize, &dwBufferCoord, lpWriteRegion);
		ph->PostCallBack(&args);
	}

	return lbRc;
}


// Sets the attributes of characters written to the console screen buffer by
// the WriteFile or WriteConsole function, or echoed by the ReadFile or ReadConsole function.
// This function affects text written after the function call.
BOOL WINAPI OnSetConsoleTextAttribute(HANDLE hConsoleOutput, WORD wAttributes)
{
	typedef BOOL (WINAPI* OnSetConsoleTextAttribute_t)(HANDLE hConsoleOutput, WORD wAttributes);
	ORIGINALFAST(SetConsoleTextAttribute);

	BOOL lbRc = FALSE;

	if (ph && ph->PreCallBack)
	{
		BOOL bMainThread = (GetCurrentThreadId() == gnHookMainThreadId);
		SETARGS2(&lbRc, hConsoleOutput, wAttributes);
		ph->PreCallBack(&args);
	}
	
	#ifdef _DEBUG
	if (wAttributes != 7)
	{
		// Что-то в некоторых случаях сбивается цвет вывода для printf
		_ASSERTE("SetConsoleTextAttribute" && (wAttributes==0x07));
	}
	#endif

	lbRc = F(SetConsoleTextAttribute)(hConsoleOutput, wAttributes);

	if (ph && ph->PostCallBack)
	{
		BOOL bMainThread = (GetCurrentThreadId() == gnHookMainThreadId);
		SETARGS2(&lbRc, hConsoleOutput, wAttributes);
		ph->PostCallBack(&args);
	}

	return lbRc;
}

typedef BOOL (WINAPI* SimpleApiFunction_t)(void*);
struct SimpleApiFunctionArg
{
	enum SimpleApiFunctionType
	{
		sft_ChooseColorA = 1,
		sft_ChooseColorW = 2,
	} funcType;
	SimpleApiFunction_t funcPtr;
	void  *pArg;
	BOOL   bResult;
	DWORD  nLastError;
};

INT_PTR CALLBACK SimpleApiDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SimpleApiFunctionArg* P = NULL;

	//typedef LONG_PTR (WINAPI* SetWindowLongPtr_t)(HWND,int,LONG_PTR);
	//SetWindowLongPtr_t SetWindowLongPtr_f = (SetWindowLongPtr_t)GetProcAddress(ghUser32,WIN3264TEST("SetWindowLongW","SetWindowLongPtrW"));
	
	if (uMsg == WM_INITDIALOG)
	{
		P = (SimpleApiFunctionArg*)lParam;
		MySetWindowLongPtrW(hwndDlg, GWLP_USERDATA, lParam);
		
		//typedef BOOL (WINAPI* GetWindowRect_t)(HWND,LPRECT);
		//GetWindowRect_t GetWindowRect_f = (GetWindowRect_t)GetProcAddress(ghUser32, "GetWindowRect");
		//typedef BOOL (WINAPI* SystemParametersInfo_t)(UINT,UINT,PVOID,UINT);
		//SystemParametersInfo_t SystemParametersInfo_f = (SystemParametersInfo_t)GetProcAddress(ghUser32, "SystemParametersInfoW");


		int x = 0, y = 0;
		RECT rcDC = {};
		if (!MyGetWindowRect(ghConEmuWndDC, &rcDC))
			MySystemParametersInfoW(SPI_GETWORKAREA, 0, &rcDC, 0);
		if (rcDC.right > rcDC.left && rcDC.bottom > rcDC.top)
		{
			x = max(rcDC.left, ((rcDC.right+rcDC.left-300)>>1));
			y = max(rcDC.top, ((rcDC.bottom+rcDC.top-200)>>1));
		}
		
		//typedef BOOL (WINAPI* SetWindowText_t)(HWND,LPCWSTR);
		//SetWindowText_t SetWindowText_f = (SetWindowText_t)GetProcAddress(ghUser32, "SetWindowTextW");

		
		switch (P->funcType)
		{
			case SimpleApiFunctionArg::sft_ChooseColorA:
				((LPCHOOSECOLORA)P->pArg)->hwndOwner = hwndDlg;
				MySetWindowTextW(hwndDlg, L"ConEmu ColorPicker");
				break;
			case SimpleApiFunctionArg::sft_ChooseColorW:
				((LPCHOOSECOLORW)P->pArg)->hwndOwner = hwndDlg;
				MySetWindowTextW(hwndDlg, L"ConEmu ColorPicker");
				break;
		}
		
		MySetWindowPos(hwndDlg, HWND_TOP, x, y, 0, 0, SWP_SHOWWINDOW);
		
		P->bResult = P->funcPtr(P->pArg);
		P->nLastError = GetLastError();
		
		//typedef BOOL (WINAPI* EndDialog_t)(HWND,INT_PTR);
		//EndDialog_t EndDialog_f = (EndDialog_t)GetProcAddress(ghUser32, "EndDialog");
		MyEndDialog(hwndDlg, 1);
		
		return FALSE;
	}

	switch (uMsg)
	{
	case WM_GETMINMAXINFO:
		{
			MINMAXINFO* p = (MINMAXINFO*)lParam;
			p->ptMinTrackSize.x = p->ptMinTrackSize.y = 0;
			MySetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, 0);
		}
		return TRUE;
	}
	
	return FALSE;
}

BOOL MyChooseColor(SimpleApiFunction_t funcPtr, void* lpcc, BOOL abUnicode)
{
	BOOL lbRc = FALSE;

	SimpleApiFunctionArg Arg =
	{
		abUnicode?SimpleApiFunctionArg::sft_ChooseColorW:SimpleApiFunctionArg::sft_ChooseColorA,
		funcPtr, lpcc
	};

	DLGTEMPLATE* pTempl = (DLGTEMPLATE*)GlobalAlloc(GPTR, sizeof(DLGTEMPLATE)+3*sizeof(WORD)); //-V119
	if (!pTempl)
	{
		_ASSERTE(pTempl!=NULL);
		Arg.bResult = Arg.funcPtr(Arg.pArg);
		Arg.nLastError = GetLastError();
		return Arg.bResult;
	}
	pTempl->style = WS_POPUP|/*DS_MODALFRAME|DS_CENTER|*/DS_SETFOREGROUND;
	
	
	//typedef INT_PTR (WINAPI* DialogBoxIndirectParam_t)(HINSTANCE,LPCDLGTEMPLATE,HWND,DLGPROC,LPARAM);
	//DialogBoxIndirectParam_t DialogBoxIndirectParam_f = (DialogBoxIndirectParam_t)GetProcAddress(ghUser32, "DialogBoxIndirectParamW");
	
	//if (DialogBoxIndirectParam_f)
	//{

	INT_PTR iRc = MyDialogBoxIndirectParamW(ghHookOurModule, pTempl, NULL, SimpleApiDialogProc, (LPARAM)&Arg);
	if (iRc == 1)
	{
		lbRc = Arg.bResult;
		SetLastError(Arg.nLastError);
	}

	//}
	
	return lbRc;
}

BOOL WINAPI OnChooseColorA(LPCHOOSECOLORA lpcc)
{
	ORIGINALFASTEX(ChooseColorA,ChooseColorA_f);
	BOOL lbRc;
	if (lpcc->hwndOwner == NULL || lpcc->hwndOwner == GetConsoleWindow())
		lbRc = MyChooseColor((SimpleApiFunction_t)F(ChooseColorA), lpcc, FALSE);
	else
		lbRc = F(ChooseColorA)(lpcc);
	return lbRc;
}

BOOL WINAPI OnChooseColorW(LPCHOOSECOLORW lpcc)
{
	ORIGINALFASTEX(ChooseColorW,ChooseColorW_f);
	BOOL lbRc;
 	if (lpcc->hwndOwner == NULL || lpcc->hwndOwner == GetConsoleWindow())
		lbRc = MyChooseColor((SimpleApiFunction_t)F(ChooseColorW), lpcc, TRUE);
	else
		lbRc = F(ChooseColorW)(lpcc);
	return lbRc;
}

// Undocumented function
BOOL WINAPI OnSetConsoleKeyShortcuts(BOOL bSet, BYTE bReserveKeys, LPVOID p1, DWORD n1)
{
	typedef BOOL (WINAPI* OnSetConsoleKeyShortcuts_t)(BOOL,BYTE,LPVOID,DWORD);
	ORIGINALFASTEX(SetConsoleKeyShortcuts,NULL);
	BOOL lbRc = FALSE;

	if (F(SetConsoleKeyShortcuts))
		lbRc = F(SetConsoleKeyShortcuts)(bSet, bReserveKeys, p1, n1);

	if (ghConEmuWnd)
	{
		DWORD nLastErr = GetLastError();
		DWORD nSize = sizeof(CESERVER_REQ_HDR)+sizeof(BYTE)*2;
		CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_KEYSHORTCUTS, nSize);
		if (pIn)
		{
			pIn->Data[0] = bSet;
			pIn->Data[1] = bReserveKeys;

			wchar_t szGuiPipeName[128];
			msprintf(szGuiPipeName, countof(szGuiPipeName), CEGUIPIPENAME, L".", (DWORD)ghConWnd);

			CESERVER_REQ* pOut = ExecuteCmd(szGuiPipeName, pIn, 1000, NULL);

			if (pOut)
				ExecuteFreeResult(pOut);
			ExecuteFreeResult(pIn);
		}
		SetLastError(nLastErr);
	}

	return lbRc;
}

//typedef BOOL (WINAPI* OnWriteConsoleOutputAx_t)(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
//BOOL WINAPI OnWriteConsoleOutputAx(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion)
//{
//	ORIGINALx(WriteConsoleOutputA);
//	BOOL lbRc = FALSE;
//
//	if (ph && ph->PreCallBack)
//	{
//		SETARGS5(&lbRc, hConsoleOutput, lpBuffer, &dwBufferSize, &dwBufferCoord, lpWriteRegion);
//		ph->PreCallBack(&args);
//	}
//
//	lbRc = Fx(WriteConsoleOutputA)(hConsoleOutput, lpBuffer, dwBufferSize, dwBufferCoord, lpWriteRegion);
//
//	if (ph && ph->PostCallBack)
//	{
//		SETARGS5(&lbRc, hConsoleOutput, lpBuffer, &dwBufferSize, &dwBufferCoord, lpWriteRegion);
//		ph->PostCallBack(&args);
//	}
//
//	return lbRc;
//}
//
//typedef BOOL (WINAPI* OnWriteConsoleOutputWx_t)(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
//BOOL WINAPI OnWriteConsoleOutputWx(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion)
//{
//	ORIGINALx(WriteConsoleOutputW);
//	BOOL lbRc = FALSE;
//
//	if (ph && ph->PreCallBack)
//	{
//		SETARGS5(&lbRc, hConsoleOutput, lpBuffer, &dwBufferSize, &dwBufferCoord, lpWriteRegion);
//		ph->PreCallBack(&args);
//	}
//
//	lbRc = Fx(WriteConsoleOutputW)(hConsoleOutput, lpBuffer, dwBufferSize, dwBufferCoord, lpWriteRegion);
//
//	if (ph && ph->PostCallBack)
//	{
//		SETARGS5(&lbRc, hConsoleOutput, lpBuffer, &dwBufferSize, &dwBufferCoord, lpWriteRegion);
//		ph->PostCallBack(&args);
//	}
//
//	return lbRc;
//}

#ifdef _DEBUG
HANDLE WINAPI OnCreateNamedPipeW(LPCWSTR lpName, DWORD dwOpenMode, DWORD dwPipeMode, DWORD nMaxInstances,DWORD nOutBufferSize, DWORD nInBufferSize, DWORD nDefaultTimeOut,LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
	typedef HANDLE(WINAPI* OnCreateNamedPipeW_t)(LPCWSTR lpName, DWORD dwOpenMode, DWORD dwPipeMode, DWORD nMaxInstances,DWORD nOutBufferSize, DWORD nInBufferSize, DWORD nDefaultTimeOut,LPSECURITY_ATTRIBUTES lpSecurityAttributes);
	ORIGINALFAST(CreateNamedPipeW);
#ifdef _DEBUG

	if (!lpName || !*lpName)
	{
		_ASSERTE(lpName && *lpName);
	}
	else
	{
		OutputDebugStringW(L"CreateNamedPipeW("); OutputDebugStringW(lpName); OutputDebugStringW(L")\n");
	}

#endif
	HANDLE hPipe = F(CreateNamedPipeW)(lpName, dwOpenMode, dwPipeMode, nMaxInstances, nOutBufferSize, nInBufferSize, nDefaultTimeOut, lpSecurityAttributes);
	return hPipe;
}
#endif

#ifdef _DEBUG
LPVOID WINAPI OnVirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect)
{
	typedef HANDLE(WINAPI* OnVirtualAlloc_t)(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
	ORIGINALFAST(VirtualAlloc);
	LPVOID lpResult = F(VirtualAlloc)(lpAddress, dwSize, flAllocationType, flProtect);
	if (lpResult == NULL)
	{
		DWORD nErr = GetLastError();
		_ASSERTE(lpResult != NULL);
		/*
		wchar_t szText[MAX_PATH*2], szTitle[64];
		msprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuHk, PID=%u, TID=%u", GetCurrentProcessId(), GetCurrentThreadId());
		msprintf(szText, SKIPLEN(countof(szText)) L"VirtualAlloc failed (0x%08X..0x%08X)\nErrorCode=0x%08X\n\nWarning! This will be an error in Release!\n\n",
			(DWORD)lpAddress, (DWORD)((LPBYTE)lpAddress+dwSize));
		GetModuleFileName(NULL, szText+lstrlen(szText), MAX_PATH);
		Message BoxW(NULL, szText, szTitle, MB_SYSTEMMODAL|MB_OK|MB_ICONSTOP);
		*/
		SetLastError(nErr);
		lpResult = F(VirtualAlloc)(NULL, dwSize, flAllocationType, flProtect);
	}
	return lpResult;
}
#endif

//110131 попробуем просто добвавить ее в ExcludedModules
//// WinInet.dll
//typedef BOOL (WINAPI* OnHttpSendRequestA_t)(LPVOID hRequest, LPCSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength);
//typedef BOOL (WINAPI* OnHttpSendRequestW_t)(LPVOID hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength);
//// смысла нет - __try не помогает
////BOOL OnHttpSendRequestA_SEH(OnHttpSendRequestA_t f, LPVOID hRequest, LPCSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength, BOOL* pbRc)
////{
////	BOOL lbOk = FALSE;
////	SAFETRY {
////		*pbRc = f(hRequest, lpszHeaders, dwHeadersLength, lpOptional, dwOptionalLength);
////		lbOk = TRUE;
////	} SAFECATCH {
////		lbOk = FALSE;
////	}
////	return lbOk;
////}
////BOOL OnHttpSendRequestW_SEH(OnHttpSendRequestW_t f, LPVOID hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength, BOOL* pbRc)
////{
////	BOOL lbOk = FALSE;
////	SAFETRY {
////		*pbRc = f(hRequest, lpszHeaders, dwHeadersLength, lpOptional, dwOptionalLength);
////		lbOk = TRUE;
////	} SAFECATCH {
////		lbOk = FALSE;
////	}
////	return lbOk;
////}
//BOOL WINAPI OnHttpSendRequestA(LPVOID hRequest, LPCSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength)
//{
//	//MessageBoxW(NULL, L"HttpSendRequestA (1)", L"ConEmu plugin", MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_ICONEXCLAMATION);
//	ORIGINALFAST(HttpSendRequestA);
//
//	BOOL lbRc;
//
//	gbHooksTemporaryDisabled = TRUE;
//	//MessageBoxW(NULL, L"HttpSendRequestA (2)", L"ConEmu plugin", MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_ICONEXCLAMATION);
//	lbRc = F(HttpSendRequestA)(hRequest, lpszHeaders, dwHeadersLength, lpOptional, dwOptionalLength);
//	//if (!OnHttpSendRequestA_SEH(F(HttpSendRequestA), hRequest, lpszHeaders, dwHeadersLength, lpOptional, dwOptionalLength)) {
//	//	MessageBoxW(NULL, L"Exception in HttpSendRequestA", L"ConEmu plugin", MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_ICONSTOP);
//	//}
//	gbHooksTemporaryDisabled = FALSE;
//	//MessageBoxW(NULL, L"HttpSendRequestA (3)", L"ConEmu plugin", MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_ICONEXCLAMATION);
//
//	return lbRc;
//}
//BOOL WINAPI OnHttpSendRequestW(LPVOID hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength)
//{
//	//MessageBoxW(NULL, L"HttpSendRequestW (1)", L"ConEmu plugin", MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_ICONEXCLAMATION);
//	ORIGINALFAST(HttpSendRequestW);
//
//	BOOL lbRc;
//
//	gbHooksTemporaryDisabled = TRUE;
//	//MessageBoxW(NULL, L"HttpSendRequestW (2)", L"ConEmu plugin", MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_ICONEXCLAMATION);
//	lbRc = F(HttpSendRequestW)(hRequest, lpszHeaders, dwHeadersLength, lpOptional, dwOptionalLength);
//	//if (!OnHttpSendRequestW_SEH(F(HttpSendRequestW), hRequest, lpszHeaders, dwHeadersLength, lpOptional, dwOptionalLength, &lbRc)) {
//	//	MessageBoxW(NULL, L"Exception in HttpSendRequestW", L"ConEmu plugin", MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_ICONSTOP);
//	//}
//	gbHooksTemporaryDisabled = FALSE;
//	//MessageBoxW(NULL, L"HttpSendRequestW (3)", L"ConEmu plugin", MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_ICONEXCLAMATION);
//
//	return lbRc;
//}

void GuiSetForeground(HWND hWnd)
{
	if (ghConEmuWndDC)
	{
		CESERVER_REQ *pIn = (CESERVER_REQ*)malloc(sizeof(*pIn)), *pOut;
		if (pIn)
		{
			ExecutePrepareCmd(pIn, CECMD_SETFOREGROUND, sizeof(CESERVER_REQ_HDR)+sizeof(u64)); //-V119
			pIn->qwData[0] = (u64)hWnd;
			HWND hConWnd = GetConsoleWindow();
			pOut = ExecuteGuiCmd(hConWnd, pIn, hConWnd);

			if (pOut) ExecuteFreeResult(pOut);
			free(pIn);
		}
	}
}

void GuiFlashWindow(BOOL bSimple, HWND hWnd, BOOL bInvert, DWORD dwFlags, UINT uCount, DWORD dwTimeout)
{
	if (ghConEmuWndDC)
	{
		CESERVER_REQ *pIn = (CESERVER_REQ*)malloc(sizeof(*pIn)), *pOut;
		if (pIn)
		{
			ExecutePrepareCmd(pIn, CECMD_FLASHWINDOW, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_FLASHWINFO)); //-V119
			pIn->Flash.bSimple = bSimple;
			pIn->Flash.hWnd = hWnd;
			pIn->Flash.bInvert = bInvert;
			pIn->Flash.dwFlags = dwFlags;
			pIn->Flash.uCount = uCount;
			pIn->Flash.dwTimeout = dwTimeout;
			HWND hConWnd = GetConsoleWindow();
			pOut = ExecuteGuiCmd(hConWnd, pIn, hConWnd);

			if (pOut) ExecuteFreeResult(pOut);
			free(pIn);
		}
	}
}

#ifdef _DEBUG
// IsConsoleHandle
bool IsHandleConsole(HANDLE handle, bool output = true)
{
	// MSDN рекомендует пользовать GetConsoleMode (оно и Input и Output умеет)

	// Консоль?
	if (((DWORD)handle & 0x10000003) != 3)
		return false;

	// Проверка типа консольного буфера (In/Out)
	DWORD num;

	if (!output)
		if (GetNumberOfConsoleInputEvents(handle, &num))
			return true;
		else
			return false;
	else if (GetNumberOfConsoleInputEvents(handle, &num))
		return false;
	else
		return true;
}
#endif

