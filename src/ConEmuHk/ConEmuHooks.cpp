
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
extern HWND    ghConEmuWnd; // Root! window
extern HWND    ghConEmuWndDC; // ConEmu DC window
//HWND ghConsoleHwnd = NULL;
extern HWND    ghConWnd;
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
//BOOL WINAPI OnPeekConsoleInputAx(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
//BOOL WINAPI OnPeekConsoleInputWx(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
//BOOL WINAPI OnReadConsoleInputAx(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
//BOOL WINAPI OnReadConsoleInputWx(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
HANDLE WINAPI OnCreateConsoleScreenBuffer(DWORD dwDesiredAccess, DWORD dwShareMode, const SECURITY_ATTRIBUTES *lpSecurityAttributes, DWORD dwFlags, LPVOID lpScreenBufferData);
BOOL WINAPI OnAllocConsole(void);
BOOL WINAPI OnFreeConsole(void);
//HWND WINAPI OnGetConsoleWindow(void); // в фаре дофига и больше вызовов этой функции
BOOL WINAPI OnWriteConsoleOutputA(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
BOOL WINAPI OnWriteConsoleOutputW(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
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

bool InitHooksCommon()
{
#ifndef HOOKS_SKIP_COMMON
	// Основные хуки
	HookItem HooksCommon[] =
	{
		/* ***** MOST CALLED ***** */
		#ifndef HOOKS_COMMON_PROCESS_ONLY
		//{(void*)OnGetConsoleWindow,     "GetConsoleWindow",     kernel32}, -- пока смысла нет. инжекты еще не на старте ставятся
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
//	HWND hConWnd = GetConsoleWindow();
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
	ghConWnd = GetConsoleWindow();
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

	// Far only functions
	InitHooksFar();

	// Реестр
	InitHooksReg();
	
	// Теперь можно обработать модули
	return SetAllHooks(ahOurDll, NULL, TRUE);
}


void ShutdownHooks()
{
	UnsetAllHooks();
	
	// Завершить работу с реестром
	DoneHooksReg();

	// Уменьшение счетчиков загрузок
	if (ghKernel32)
	{
		FreeLibrary(ghKernel32);
		ghKernel32 = NULL;
	}
	if (ghUser32)
	{
		FreeLibrary(ghUser32);
		ghUser32 = NULL;
	}
	if (ghShell32)
	{
		FreeLibrary(ghShell32);
		ghShell32 = NULL;
	}
	if (ghAdvapi32)
	{
		FreeLibrary(ghAdvapi32);
		ghAdvapi32 = NULL;
	}
	if (ghComdlg32)
	{
		FreeLibrary(ghComdlg32);
		ghComdlg32 = NULL;
	}
}



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

typedef BOOL (WINAPI* OnCreateProcessA_t)(LPCSTR lpApplicationName,  LPSTR lpCommandLine,  LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory,  LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
BOOL WINAPI OnCreateProcessA(LPCSTR lpApplicationName,  LPSTR lpCommandLine,  LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory,  LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
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
typedef BOOL (WINAPI* OnCreateProcessW_t)(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
BOOL WINAPI OnCreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
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


typedef BOOL (WINAPI* OnTrackPopupMenu_t)(HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, CONST RECT * prcRect);
BOOL WINAPI OnTrackPopupMenu(HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, CONST RECT * prcRect)
{
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

typedef BOOL (WINAPI* OnTrackPopupMenuEx_t)(HMENU hmenu, UINT fuFlags, int x, int y, HWND hWnd, LPTPMPARAMS lptpm);
BOOL WINAPI OnTrackPopupMenuEx(HMENU hmenu, UINT fuFlags, int x, int y, HWND hWnd, LPTPMPARAMS lptpm)
{
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

typedef BOOL (WINAPI* OnShellExecuteExA_t)(LPSHELLEXECUTEINFOA lpExecInfo);
BOOL WINAPI OnShellExecuteExA(LPSHELLEXECUTEINFOA lpExecInfo)
{
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
typedef BOOL (WINAPI* OnShellExecuteExW_t)(LPSHELLEXECUTEINFOW lpExecInfo);
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

typedef HINSTANCE(WINAPI* OnShellExecuteA_t)(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd);
HINSTANCE WINAPI OnShellExecuteA(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd)
{
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
	
	sp->OnShellFinished(((INT_PTR)lhRc > 32), lhRc, NULL);
	delete sp;

	//gbInShellExecuteEx = FALSE;
	return lhRc;
}
typedef HINSTANCE(WINAPI* OnShellExecuteW_t)(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd);
HINSTANCE WINAPI OnShellExecuteW(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd)
{
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
	
	sp->OnShellFinished(((INT_PTR)lhRc > 32), lhRc, NULL);
	delete sp;

	//gbInShellExecuteEx = FALSE;
	return lhRc;
}

typedef BOOL (WINAPI* OnFlashWindow_t)(HWND hWnd, BOOL bInvert);
BOOL WINAPI OnFlashWindow(HWND hWnd, BOOL bInvert)
{
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
typedef BOOL (WINAPI* OnFlashWindowEx_t)(PFLASHWINFO pfwi);
BOOL WINAPI OnFlashWindowEx(PFLASHWINFO pfwi)
{
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

typedef BOOL (WINAPI* OnSetForegroundWindow_t)(HWND hWnd);
BOOL WINAPI OnSetForegroundWindow(HWND hWnd)
{
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

typedef BOOL (WINAPI* OnGetWindowRect_t)(HWND hWnd, LPRECT lpRect);
BOOL WINAPI OnGetWindowRect(HWND hWnd, LPRECT lpRect)
{
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

typedef BOOL (WINAPI* OnScreenToClient_t)(HWND hWnd, LPPOINT lpPoint);
BOOL WINAPI OnScreenToClient(HWND hWnd, LPPOINT lpPoint)
{
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

#ifndef NORM_STOP_ON_NULL
#define NORM_STOP_ON_NULL 0x10000000
#endif

// ANSI хукать смысла нет, т.к. FAR 1.x сравнивает сам
typedef int (WINAPI* OnCompareStringW_t)(LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2);
int WINAPI OnCompareStringW(LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2)
{
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
					if (ch1>=L'A' && ch1<=L'Z') ch1 |= 0x20;

					if (ch2>=L'A' && ch2<=L'Z') ch2 |= 0x20;

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

typedef DWORD (WINAPI* OnGetConsoleAliasesW_t)(LPWSTR AliasBuffer, DWORD AliasBufferLength, LPWSTR ExeName);
DWORD WINAPI OnGetConsoleAliasesW(LPWSTR AliasBuffer, DWORD AliasBufferLength, LPWSTR ExeName)
{
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
			ConInfo.InitName(CECONMAPNAME, (DWORD)hConWnd);
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
					DWORD nData = min(AliasBufferLength,(pOut->hdr.cbSize-sizeof(pOut->hdr)));

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

typedef BOOL (WINAPI* OnGetNumberOfConsoleInputEvents_t)(HANDLE hConsoleInput, LPDWORD lpcNumberOfEvents);
BOOL WINAPI OnGetNumberOfConsoleInputEvents(HANDLE hConsoleInput, LPDWORD lpcNumberOfEvents)
{
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
	if (!gFarMode.bFarHookMode || !gFarMode.bMonitorConsoleInput || !nRead || !lpBuffer)
		return;
		
	//// Пока - только Read. Peek игнорируем
	//if (acPeekRead != 'R')
	//	return;
	
	CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_PEEKREADINFO, sizeof(CESERVER_REQ_HDR)
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

typedef BOOL (WINAPI* OnPeekConsoleInputA_t)(HANDLE,PINPUT_RECORD,DWORD,LPDWORD);
BOOL WINAPI OnPeekConsoleInputA(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
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

typedef BOOL (WINAPI* OnPeekConsoleInputW_t)(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
BOOL WINAPI OnPeekConsoleInputW(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
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

typedef BOOL (WINAPI* OnReadConsoleInputA_t)(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
BOOL WINAPI OnReadConsoleInputA(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
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

typedef BOOL (WINAPI* OnReadConsoleInputW_t)(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
BOOL WINAPI OnReadConsoleInputW(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
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

typedef HANDLE(WINAPI* OnCreateConsoleScreenBuffer_t)(DWORD dwDesiredAccess, DWORD dwShareMode, const SECURITY_ATTRIBUTES *lpSecurityAttributes, DWORD dwFlags, LPVOID lpScreenBufferData);
HANDLE WINAPI OnCreateConsoleScreenBuffer(DWORD dwDesiredAccess, DWORD dwShareMode, const SECURITY_ATTRIBUTES *lpSecurityAttributes, DWORD dwFlags, LPVOID lpScreenBufferData)
{
	ORIGINALFAST(CreateConsoleScreenBuffer);

	if ((dwShareMode & (FILE_SHARE_READ|FILE_SHARE_WRITE)) != (FILE_SHARE_READ|FILE_SHARE_WRITE))
		dwShareMode |= (FILE_SHARE_READ|FILE_SHARE_WRITE);

	HANDLE h;
	h = F(CreateConsoleScreenBuffer)(dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwFlags, lpScreenBufferData);
	return h;
}

typedef BOOL (WINAPI* OnAllocConsole_t)(void);
BOOL WINAPI OnAllocConsole(void)
{
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

typedef BOOL (WINAPI* OnFreeConsole_t)(void);
BOOL WINAPI OnFreeConsole(void)
{
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

//typedef HWND (WINAPI* OnGetConsoleWindow_t)(void);
//HWND WINAPI OnGetConsoleWindow(void)
//{
//	ORIGINALFAST(GetConsoleWindow);
//
//	if (ghConEmuWndDC && ghConsoleHwnd)
//	{
//		return ghConsoleHwnd;
//	}
//
//	HWND h;
//	h = F(GetConsoleWindow)();
//	return h;
//}

typedef BOOL (WINAPI* OnWriteConsoleOutputA_t)(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
BOOL WINAPI OnWriteConsoleOutputA(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion)
{
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

typedef BOOL (WINAPI* OnWriteConsoleOutputW_t)(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
BOOL WINAPI OnWriteConsoleOutputW(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion)
{
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

	typedef LONG_PTR (WINAPI* SetWindowLongPtr_t)(HWND,int,LONG_PTR);
	SetWindowLongPtr_t SetWindowLongPtr_f = (SetWindowLongPtr_t)GetProcAddress(ghUser32,
		#ifdef _WIN64
		"SetWindowLongPtrW"
		#else
		"SetWindowLongW"
		#endif
		);
	
	if (uMsg == WM_INITDIALOG)
	{
		P = (SimpleApiFunctionArg*)lParam;
		SetWindowLongPtr_f(hwndDlg, GWLP_USERDATA, lParam);
		
		typedef BOOL (WINAPI* GetWindowRect_t)(HWND,LPRECT);
		GetWindowRect_t GetWindowRect_f = (GetWindowRect_t)GetProcAddress(ghUser32, "GetWindowRect");
		typedef BOOL (WINAPI* SystemParametersInfo_t)(UINT,UINT,PVOID,UINT);
		SystemParametersInfo_t SystemParametersInfo_f = (SystemParametersInfo_t)GetProcAddress(ghUser32, "SystemParametersInfoW");


		int x = 0, y = 0;
		RECT rcDC = {};
		if (!GetWindowRect_f(ghConEmuWndDC, &rcDC))
			SystemParametersInfo_f(SPI_GETWORKAREA, 0, &rcDC, 0);
		if (rcDC.right > rcDC.left && rcDC.bottom > rcDC.top)
		{
			x = max(rcDC.left, ((rcDC.right+rcDC.left-300)>>1));
			y = max(rcDC.top, ((rcDC.bottom+rcDC.top-200)>>1));
		}
		
		typedef BOOL (WINAPI* SetWindowText_t)(HWND,LPCWSTR);
		SetWindowText_t SetWindowText_f = (SetWindowText_t)GetProcAddress(ghUser32, "SetWindowTextW");

		
		switch (P->funcType)
		{
			case SimpleApiFunctionArg::sft_ChooseColorA:
				((LPCHOOSECOLORA)P->pArg)->hwndOwner = hwndDlg;
				SetWindowText_f(hwndDlg, L"ConEmu ColorPicker");
				break;
			case SimpleApiFunctionArg::sft_ChooseColorW:
				((LPCHOOSECOLORW)P->pArg)->hwndOwner = hwndDlg;
				SetWindowText_f(hwndDlg, L"ConEmu ColorPicker");
				break;
		}
		
		typedef BOOL (WINAPI* SetWindowPos_t)(HWND,HWND,int,int,int,int,UINT);
		SetWindowPos_t SetWindowPos_f = (SetWindowPos_t)GetProcAddress(ghUser32, "SetWindowPos");
		
		SetWindowPos_f(hwndDlg, HWND_TOP, x, y, 0, 0, SWP_SHOWWINDOW);
		
		P->bResult = P->funcPtr(P->pArg);
		P->nLastError = GetLastError();
		
		typedef BOOL (WINAPI* EndDialog_t)(HWND,INT_PTR);
		EndDialog_t EndDialog_f = (EndDialog_t)GetProcAddress(ghUser32, "EndDialog");
		EndDialog_f(hwndDlg, 1);
		
		return FALSE;
	}

	switch (uMsg)
	{
	case WM_GETMINMAXINFO:
		{
			MINMAXINFO* p = (MINMAXINFO*)lParam;
			p->ptMinTrackSize.x = p->ptMinTrackSize.y = 0;
			SetWindowLongPtr_f(hwndDlg, DWLP_MSGRESULT, 0);
		}
		return TRUE;
	}
	
	return FALSE;
}

BOOL MyChooseColor(SimpleApiFunction_t funcPtr, void* lpcc, BOOL abUnicode)
{
	BOOL lbRc = FALSE;
	
	DLGTEMPLATE* pTempl = (DLGTEMPLATE*)GlobalAlloc(GPTR, sizeof(DLGTEMPLATE)+3*sizeof(WORD));
	pTempl->style = WS_POPUP|/*DS_MODALFRAME|DS_CENTER|*/DS_SETFOREGROUND;
	
	SimpleApiFunctionArg Arg =
	{
		abUnicode?SimpleApiFunctionArg::sft_ChooseColorW:SimpleApiFunctionArg::sft_ChooseColorA,
		funcPtr, lpcc
	};
	
	typedef INT_PTR (WINAPI* DialogBoxIndirectParam_t)(HINSTANCE,LPCDLGTEMPLATE,HWND,DLGPROC,LPARAM);
	DialogBoxIndirectParam_t DialogBoxIndirectParam_f = (DialogBoxIndirectParam_t)GetProcAddress(ghUser32, "DialogBoxIndirectParamW");
	
	if (DialogBoxIndirectParam_f)
	{
		INT_PTR iRc = DialogBoxIndirectParam_f(ghHookOurModule, pTempl, NULL, SimpleApiDialogProc, (LPARAM)&Arg);
		if (iRc == 1)
		{
			lbRc = Arg.bResult;
			SetLastError(Arg.nLastError);
		}
	}
	
	return lbRc;
}

BOOL WINAPI OnChooseColorA(LPCHOOSECOLORA lpcc)
{
	ORIGINALFASTEX(ChooseColorA,ChooseColorA_f);
	BOOL lbRc = MyChooseColor((SimpleApiFunction_t)F(ChooseColorA), lpcc, FALSE);
	return lbRc;
}

BOOL WINAPI OnChooseColorW(LPCHOOSECOLORW lpcc)
{
	ORIGINALFASTEX(ChooseColorW,ChooseColorW_f);
	BOOL lbRc = MyChooseColor((SimpleApiFunction_t)F(ChooseColorW), lpcc, TRUE);
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
typedef HANDLE(WINAPI* OnCreateNamedPipeW_t)(LPCWSTR lpName, DWORD dwOpenMode, DWORD dwPipeMode, DWORD nMaxInstances,DWORD nOutBufferSize, DWORD nInBufferSize, DWORD nDefaultTimeOut,LPSECURITY_ATTRIBUTES lpSecurityAttributes);
HANDLE WINAPI OnCreateNamedPipeW(LPCWSTR lpName, DWORD dwOpenMode, DWORD dwPipeMode, DWORD nMaxInstances,DWORD nOutBufferSize, DWORD nInBufferSize, DWORD nDefaultTimeOut,LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
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
typedef HANDLE(WINAPI* OnVirtualAlloc_t)(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
LPVOID WINAPI OnVirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect)
{
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
		CESERVER_REQ In, *pOut;
		ExecutePrepareCmd(&In, CECMD_SETFOREGROUND, sizeof(CESERVER_REQ_HDR)+sizeof(u64));
		In.qwData[0] = (u64)hWnd;
		HWND hConWnd = GetConsoleWindow();
		pOut = ExecuteGuiCmd(hConWnd, &In, hConWnd);

		if (pOut) ExecuteFreeResult(pOut);
	}
}

void GuiFlashWindow(BOOL bSimple, HWND hWnd, BOOL bInvert, DWORD dwFlags, UINT uCount, DWORD dwTimeout)
{
	if (ghConEmuWndDC)
	{
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

#ifdef _DEBUG
bool IsHandleConsole(HANDLE handle, bool output = true)
{
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

