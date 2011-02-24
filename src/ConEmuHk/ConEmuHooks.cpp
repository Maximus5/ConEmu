
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

#define SETCONCP_READYTIMEOUT 5000
#define SETCONCP_TIMEOUT 1000

#include <windows.h>
#include <TCHAR.h>
#include <Tlhelp32.h>
#include <shlwapi.h>
#include "..\common\common.hpp"
#include "..\common\ConEmuCheck.h"
#include "..\common\SetHook.h"
#include "..\common\execute.h"
#include "ConEmuC.h"
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

#ifdef USE_INPUT_SEMAPHORE
HANDLE ghConInSemaphore = NULL;
#endif

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


static TCHAR kernel32[] = _T("kernel32.dll");
static TCHAR user32[]   = _T("user32.dll");
static TCHAR shell32[]  = _T("shell32.dll");
static HMODULE ghKernel32 = NULL, ghUser32 = NULL, ghShell32 = NULL;
//110131 попробуем просто добвавить ее в ExcludedModules
//static TCHAR wininet[]  = _T("wininet.dll");

//static BOOL bHooksWin2k3R2Only = FALSE;
//static HookItem HooksWin2k3R2Only[] = {
//	{OnSetConsoleCP, "SetConsoleCP", kernel32, 0},
//	{OnSetConsoleOutputCP, "SetConsoleOutputCP", kernel32, 0},
//	/* ************************ */
//	{0, 0, 0}
//};

static int WINAPI OnCompareStringW(LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2);
//
//static BOOL WINAPI OnHttpSendRequestA(LPVOID hRequest, LPCSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength);
//static BOOL WINAPI OnHttpSendRequestW(LPVOID hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength);

static HookItem HooksFarOnly[] =
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

// Service functions
//typedef DWORD (WINAPI* GetProcessId_t)(HANDLE Process);
GetProcessId_t gfGetProcessId = NULL;



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
static int WINAPI OnCompareStringW(LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2);
static DWORD WINAPI OnGetConsoleAliasesW(LPWSTR AliasBuffer, DWORD AliasBufferLength, LPWSTR ExeName);
static BOOL WINAPI OnGetNumberOfConsoleInputEvents(HANDLE hConsoleInput, LPDWORD lpcNumberOfEvents);
static BOOL WINAPI OnPeekConsoleInputA(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
static BOOL WINAPI OnPeekConsoleInputW(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
static BOOL WINAPI OnReadConsoleInputA(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
static BOOL WINAPI OnReadConsoleInputW(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
//static BOOL WINAPI OnPeekConsoleInputAx(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
//static BOOL WINAPI OnPeekConsoleInputWx(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
//static BOOL WINAPI OnReadConsoleInputAx(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
//static BOOL WINAPI OnReadConsoleInputWx(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
static HANDLE WINAPI OnCreateConsoleScreenBuffer(DWORD dwDesiredAccess, DWORD dwShareMode, const SECURITY_ATTRIBUTES *lpSecurityAttributes, DWORD dwFlags, LPVOID lpScreenBufferData);
static BOOL WINAPI OnAllocConsole(void);
static BOOL WINAPI OnFreeConsole(void);
//static HWND WINAPI OnGetConsoleWindow(void); // в фаре дофига и больше вызовов этой функции
static BOOL WINAPI OnWriteConsoleOutputA(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
static BOOL WINAPI OnWriteConsoleOutputW(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
//static BOOL WINAPI OnWriteConsoleOutputAx(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
//static BOOL WINAPI OnWriteConsoleOutputWx(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
static BOOL WINAPI OnGetWindowRect(HWND hWnd, LPRECT lpRect);
static BOOL WINAPI OnScreenToClient(HWND hWnd, LPPOINT lpPoint);
static BOOL WINAPI OnCreateProcessA(LPCSTR lpApplicationName,  LPSTR lpCommandLine,  LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory,  LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
static BOOL WINAPI OnCreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
#ifdef _DEBUG
static HANDLE WINAPI OnCreateNamedPipeW(LPCWSTR lpName, DWORD dwOpenMode, DWORD dwPipeMode, DWORD nMaxInstances,DWORD nOutBufferSize, DWORD nInBufferSize, DWORD nDefaultTimeOut,LPSECURITY_ATTRIBUTES lpSecurityAttributes);
#endif
static BOOL WINAPI OnSetConsoleCP(UINT wCodePageID);
static BOOL WINAPI OnSetConsoleOutputCP(UINT wCodePageID);



static HookItem HooksCommon[] =
{
	/* ***** MOST CALLED ***** */
//	{(void*)OnGetConsoleWindow,     "GetConsoleWindow",     kernel32}, -- пока смысла нет. инжекты еще не на старте ставятся
	//{(void*)OnWriteConsoleOutputWx,	"WriteConsoleOutputW",  kernel32},
	//{(void*)OnWriteConsoleOutputAx,	"WriteConsoleOutputA",  kernel32},
	{(void*)OnWriteConsoleOutputW,	"WriteConsoleOutputW",  kernel32},
	{(void*)OnWriteConsoleOutputA,	"WriteConsoleOutputA",  kernel32},
	/* ************************ */
	//{(void*)OnPeekConsoleInputWx,	"PeekConsoleInputW",	kernel32},
	//{(void*)OnPeekConsoleInputAx,	"PeekConsoleInputA",	kernel32},
	//{(void*)OnReadConsoleInputWx,	"ReadConsoleInputW",	kernel32},
	//{(void*)OnReadConsoleInputAx,	"ReadConsoleInputA",	kernel32},
	{(void*)OnPeekConsoleInputW,	"PeekConsoleInputW",	kernel32},
	{(void*)OnPeekConsoleInputA,	"PeekConsoleInputA",	kernel32},
	{(void*)OnReadConsoleInputW,	"ReadConsoleInputW",	kernel32},
	{(void*)OnReadConsoleInputA,	"ReadConsoleInputA",	kernel32},
	/* ************************ */
	{(void*)OnCreateProcessA,		"CreateProcessA",		kernel32},
	{(void*)OnCreateProcessW,		"CreateProcessW",		kernel32},
	/* ************************ */
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
#ifdef _DEBUG
	{(void*)OnCreateNamedPipeW,		"CreateNamedPipeW",		kernel32},
#endif
	// Microsoft bug?
	// http://code.google.com/p/conemu-maximus5/issues/detail?id=60
	{OnSetConsoleCP,				"SetConsoleCP",			kernel32},
	{OnSetConsoleOutputCP,			"SetConsoleOutputCP",	kernel32},
	/* ************************ */
	{(void*)OnTrackPopupMenu,		"TrackPopupMenu",		user32},
	{(void*)OnTrackPopupMenuEx,		"TrackPopupMenuEx",		user32},
	{(void*)OnFlashWindow,			"FlashWindow",			user32},
	{(void*)OnFlashWindowEx,		"FlashWindowEx",		user32},
	{(void*)OnSetForegroundWindow,	"SetForegroundWindow",	user32},
	{(void*)OnGetWindowRect,		"GetWindowRect",		user32},
	{(void*)OnScreenToClient,		"ScreenToClient",		user32},
	/* ************************ */
	{(void*)OnShellExecuteExA,		"ShellExecuteExA",		shell32},
	{(void*)OnShellExecuteExW,		"ShellExecuteExW",		shell32},
	{(void*)OnShellExecuteA,		"ShellExecuteA",		shell32},
	{(void*)OnShellExecuteW,		"ShellExecuteW",		shell32},
	/* ************************ */
	{0}
};

void __stdcall SetFarHookMode(struct HookModeFar *apFarMode)
{
	if (!apFarMode)
	{
		gFarMode.bFarHookMode = FALSE;
	}
	else if (apFarMode->cbSize != sizeof(HookModeFar))
	{
		gFarMode.bFarHookMode = FALSE;
		wchar_t szTitle[64], szText[255];
		_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuHk, PID=%u", GetCurrentProcessId());
		_wsprintf(szText, SKIPLEN(countof(szText)) L"SetFarHookMode recieved invalid sizeof = %u\nRequired = %u", apFarMode->cbSize, (DWORD)sizeof(HookModeFar));
		MessageBoxW(NULL, szText, szTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
	}
	else
	{
		memmove(&gFarMode, apFarMode, sizeof(gFarMode));
	}
}

void InitializeConsoleInputSemaphore()
{
#ifdef USE_INPUT_SEMAPHORE
	if (ghConInSemaphore != NULL)
	{
		ReleaseConsoleInputSemaphore();
	}
	wchar_t szName[128];
	HWND hConWnd = GetConsoleWindow();
	if (hConWnd != ghConWnd)
	{
		_ASSERTE(ghConWnd == hConWnd);
	}
	if (hConWnd != NULL)
	{
		_wsprintf(szName, SKIPLEN(countof(szName)) CEINPUTSEMAPHORE, (DWORD)hConWnd);
		ghConInSemaphore = CreateSemaphore(LocalSecurity(), 1, 1, szName);
		_ASSERTE(ghConInSemaphore != NULL);
	}
#endif
}

void ReleaseConsoleInputSemaphore()
{
#ifdef USE_INPUT_SEMAPHORE
	if (ghConInSemaphore != NULL)
	{
		CloseHandle(ghConInSemaphore);
		ghConInSemaphore = NULL;
	}
#endif
}

// Эту функцию нужно позвать из DllMain
BOOL StartupHooks(HMODULE ahOurDll)
{
	ghConWnd = GetConsoleWindow();
	ghConEmuWndDC = GetConEmuHWND(FALSE);
	ghConEmuWnd = ghConEmuWndDC ? GetParent(ghConEmuWndDC) : NULL;
	//gbInShellExecuteEx = FALSE;

	WARNING("Получить из мэппинга gdwServerPID");

	// Зовем LoadLibrary. Kernel-то должен был сразу загрузится (static link) в любой
	// windows приложении, но вот shell32 - не обязательно, а нам нужно хуки проинициализировать
	ghKernel32 = LoadLibrary(kernel32);
	ghUser32 = LoadLibrary(user32);
	ghShell32 = LoadLibrary(shell32);

	if (ghKernel32)
		gfGetProcessId = (GetProcessId_t)GetProcAddress(ghKernel32, "GetProcessId");

	// Основные хуки
	InitHooks(HooksCommon);
	TODO("Проверить, фар ли это? Если нет - можно не ставить HooksFarOnly");
	InitHooks(HooksFarOnly);
	// Реестр
	TODO("Hook registry");
	return SetAllHooks(ahOurDll, NULL, TRUE);
}


void ShutdownHooks()
{
	UnsetAllHooks();

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
}



// Forward
static void GuiSetForeground(HWND hWnd);
static void GuiFlashWindow(BOOL bSimple, HWND hWnd, BOOL bInvert, DWORD dwFlags, UINT uCount, DWORD dwTimeout);
//static BOOL PrepareExecuteParmsA(enum CmdOnCreateType aCmd, LPCSTR asAction, DWORD anFlags, 
//				LPCSTR asFile, LPCSTR asParam,
//				HANDLE hStdIn, HANDLE hStdOut, HANDLE hStdErr,
//				LPSTR* psFile, LPSTR* psParam, DWORD& nImageSubsystem, DWORD& nImageBits);
//static BOOL PrepareExecuteParmsW(enum CmdOnCreateType aCmd, LPCWSTR asAction, DWORD anFlags, 
//				LPCWSTR asFile, LPCWSTR asParam,
//				HANDLE hStdIn, HANDLE hStdOut, HANDLE hStdErr,
//				LPWSTR* psFile, LPWSTR* psParam, DWORD& nImageSubsystem, DWORD& nImageBits);
//static wchar_t* str2wcs(const char* psz, UINT anCP);
//static wchar_t* wcs2str(const char* psz, UINT anCP);

typedef BOOL (WINAPI* OnCreateProcessA_t)(LPCSTR lpApplicationName,  LPSTR lpCommandLine,  LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory,  LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
static BOOL WINAPI OnCreateProcessA(LPCSTR lpApplicationName,  LPSTR lpCommandLine,  LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory,  LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
	ORIGINALFAST(CreateProcessA);
	BOOL bMainThread = FALSE; // поток не важен
	BOOL lbRc = FALSE;
	//LPSTR pszTempApp = NULL, pszTempCmd = NULL;
	//BOOL lbSuspended = (dwCreationFlags & CREATE_SUSPENDED) == CREATE_SUSPENDED;
	DWORD dwErr = 0;
	

	if (ph && ph->PreCallBack)
	{
		SETARGS10(&lbRc, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);

		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	CShellProc sp;
	sp.OnCreateProcessA(&lpApplicationName, (LPCSTR*)&lpCommandLine, &dwCreationFlags, lpStartupInfo);

	//// Issue 351: После перехода исполнятеля фара на ShellExecuteEx почему-то сюда стал приходить
	////            левый хэндл (hStdOutput = 0x00010001)
	////            и недокументированный флаг 0x400 в lpStartupInfo->dwFlags
	//if (gbInShellExecuteEx && lpStartupInfo->dwFlags == 0x401)
	//{
	//	OSVERSIONINFO osv = {sizeof(OSVERSIONINFO)};

	//	if (GetVersionEx(&osv) && osv.dwMajorVersion == 5 && osv.dwMinorVersion == 1)
	//	{
	//		if (lpStartupInfo->hStdOutput == (HANDLE)0x00010001 && !lpStartupInfo->hStdError)
	//		{
	//			lpStartupInfo->hStdOutput = NULL;
	//			lpStartupInfo->dwFlags &= ~0x400;
	//		}
	//	}
	//}

	//// Under ConEmu only!
	//// Тупо PrepareExecuteParmsX нельзя, его уже мог обработать OnShellExecuteXXX, поэтому !gbInShellExecuteEx
	//BOOL lbParamsChanged = FALSE;
	//DWORD nImageSubsystem = 0, nImageBits = 0, nFileAttrs = (DWORD)-1;
	//if (ghConEmuWndDC)
	//{
	//	if (!gbInShellExecuteEx)
	//	{
	//		if (PrepareExecuteParmsA(eCreateProcess, "", dwCreationFlags, lpApplicationName, lpCommandLine,
	//				lpStartupInfo->hStdInput, lpStartupInfo->hStdOutput, lpStartupInfo->hStdError,
	//				&pszTempApp, &pszTempCmd, nImageSubsystem, nImageBits))
	//		{
	//			lpApplicationName = pszTempApp;
	//			lpCommandLine = pszTempCmd;
	//			lbParamsChanged = TRUE;
	//		}
	//		lbNeedInjects = (nImageSubsystem != IMAGE_SUBSYSTEM_WINDOWS_GUI) && !lbParamsChanged;
	//	}
	//	else
	//	{
	//		CESERVER_REQ* pIn = NewCmdOnCreateA(eCreateProcess, "", 0, lpApplicationName, lpCommandLine, 
	//				0, 0, lpStartupInfo->hStdInput, lpStartupInfo->hStdOutput, lpStartupInfo->hStdError);
	//		if (pIn)
	//		{
	//			HWND hConWnd = GetConsoleWindow();
	//			CESERVER_REQ* pOut = ExecuteGuiCmd(hConWnd, pIn, hConWnd);
	//			ExecuteFreeResult(pIn);
	//			if (pOut) ExecuteFreeResult(pOut);
	//		}
	//	
	//		wchar_t szExe[MAX_PATH+1]; szExe[0] = 0;
	//		UINT nCP = AreFileApisANSI() ? CP_ACP : CP_OEMCP;
	//		if (lpApplicationName)
	//		{
	//			//_strcpyn_c(szExe, countof(szExe), lpApplicationName, countof(szExe));
	//			if (MultiByteToWideChar(nCP, 0, lpApplicationName, -1, szExe, countof(szExe)) <= 0)
	//				szExe[0] = 0;
	//			else
	//				szExe[MAX_PATH] = 0;
	//		}
	//		else
	//		{
	//			//LPCSTR pszTemp = lpCommandLine;
	//			//if (NextArg(&pszTemp, szExe) != 0)
	//			//	szExe[0] = 0;

	//			LPWSTR pwszCmdLine = str2wcs(lpCommandLine, nCP);
	//			BOOL lbNeedCutStartEndQuot = FALSE;
	//			IsNeedCmd(pwszCmdLine, &lbNeedCutStartEndQuot, szExe);
	//			free(pwszCmdLine);
	//		}
	//		if (szExe[0])
	//			GetImageSubsystem(szExe, nImageSubsystem, nImageBits, nFileAttrs);
	//		lbNeedInjects = (nImageSubsystem != IMAGE_SUBSYSTEM_WINDOWS_GUI);

	//		LPCWSTR pszName = PointToName(szExe);
	//		if (pszName && lbNeedInjects)
	//		{
	//			if (lstrcmpi(pszName, L"ConEmuC.exe") == 0 || lstrcmpi(pszName, L"ConEmuC64.exe") == 0)
	//				lbParamsChanged = TRUE;
	//		}
	//	}

	//	if (!lbSuspended && !lbParamsChanged && lbNeedInjects)
	//		dwCreationFlags |= CREATE_SUSPENDED;
	//}

	lbRc = F(CreateProcessA)(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	if (!lbRc) dwErr = GetLastError();

	// Если lbParamsChanged == TRUE - об инжектах позаботится ConEmuC.exe
	sp.OnCreateProcessFinished(lbRc, lpProcessInformation);

	//if (lbRc && ghConEmuWndDC && !lbParamsChanged && lbNeedInjects)
	//{
	//	int iHookRc = InjectHooks(*lpProcessInformation, FALSE);

	//	if (iHookRc != 0)
	//	{
	//		DWORD nErrCode = GetLastError();
	//		_ASSERTE(iHookRc == 0);
	//		wchar_t szDbgMsg[255], szTitle[128];
	//		_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuC, PID=%u", GetCurrentProcessId());
	//		_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"ConEmuC.W, PID=%u\nInjecting hooks into PID=%u\nFAILED, code=%i:0x%08X", GetCurrentProcessId(), lpProcessInformation->dwProcessId, iHookRc, nErrCode);
	//		MessageBoxW(NULL, szDbgMsg, szTitle, MB_SYSTEMMODAL);
	//	}

	//	// Отпустить процесс
	//	if (!lbSuspended)
	//		ResumeThread(lpProcessInformation->hThread);
	//}

	if (ph && ph->PostCallBack)
	{
		SETARGS10(&lbRc, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
		ph->PostCallBack(&args);
	}
	
	//if (pszTempApp)
	//	free(pszTempApp);
	//if (pszTempCmd)
	//	free(pszTempCmd);

	return lbRc;
}
typedef BOOL (WINAPI* OnCreateProcessW_t)(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
static BOOL WINAPI OnCreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
	ORIGINALFAST(CreateProcessW);
	BOOL bMainThread = FALSE; // поток не важен
	BOOL lbRc = FALSE;
	//LPWSTR pszTempApp = NULL, pszTempCmd = NULL;
	//BOOL lbSuspended = (dwCreationFlags & CREATE_SUSPENDED) == CREATE_SUSPENDED;
	//BOOL lbNeedInjects = TRUE;
	DWORD dwErr = 0;

	if (ph && ph->PreCallBack)
	{
		SETARGS10(&lbRc, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);

		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	CShellProc sp;
	sp.OnCreateProcessW(&lpApplicationName, (LPCWSTR*)&lpCommandLine, &dwCreationFlags, lpStartupInfo);

	//// Issue 351: После перехода исполнятеля фара на ShellExecuteEx почему-то сюда стал приходить
	////            левый хэндл (hStdOutput = 0x00010001)
	////            и недокументированный флаг 0x400 в lpStartupInfo->dwFlags
	//if (gbInShellExecuteEx && lpStartupInfo->dwFlags == 0x401)
	//{
	//	OSVERSIONINFO osv = {sizeof(OSVERSIONINFO)};

	//	if (GetVersionEx(&osv))
	//	{
	//		if (osv.dwMajorVersion == 5 && (osv.dwMinorVersion == 1/*WinXP*/ || osv.dwMinorVersion == 2/*Win2k3*/))
	//		{
	//			if (lpStartupInfo->hStdOutput == (HANDLE)0x00010001 && !lpStartupInfo->hStdError)
	//			{
	//				lpStartupInfo->hStdOutput = NULL;
	//				lpStartupInfo->dwFlags &= ~0x400;
	//			}
	//		}
	//	}
	//}

	//// Under ConEmu only!
	//// Тупо PrepareExecuteParmsX нельзя, его уже мог обработать OnShellExecuteXXX, поэтому !gbInShellExecuteEx
	//BOOL lbParamsChanged = FALSE;
	//DWORD nImageSubsystem = 0, nImageBits = 0, nFileAttrs = (DWORD)-1;
	//if (ghConEmuWndDC)
	//{
	//	if (!gbInShellExecuteEx)
	//	{
	//		if (PrepareExecuteParmsW(eCreateProcess, L"", dwCreationFlags, lpApplicationName, lpCommandLine, 
	//				lpStartupInfo->hStdInput, lpStartupInfo->hStdOutput, lpStartupInfo->hStdError,
	//				&pszTempApp, &pszTempCmd, nImageSubsystem, nImageBits))
	//		{
	//			lpApplicationName = pszTempApp;
	//			lpCommandLine = pszTempCmd;
	//			lbParamsChanged = TRUE;
	//		}
	//		lbNeedInjects = (nImageSubsystem != IMAGE_SUBSYSTEM_WINDOWS_GUI) && !lbParamsChanged;
	//	}
	//	else
	//	{
	//		wchar_t szBaseDir[MAX_PATH+2];
	//		CESERVER_REQ* pIn = NewCmdOnCreateW(eCreateProcess, L"", 0, lpApplicationName, lpCommandLine, 
	//				0, 0, lpStartupInfo->hStdInput, lpStartupInfo->hStdOutput, lpStartupInfo->hStdError, szBaseDir);
	//		if (pIn)
	//		{
	//			HWND hConWnd = GetConsoleWindow();
	//			CESERVER_REQ* pOut = ExecuteGuiCmd(hConWnd, pIn, hConWnd);
	//			ExecuteFreeResult(pIn);
	//			if (pOut) ExecuteFreeResult(pOut);
	//		}
	//	
	//		wchar_t szExe[MAX_PATH+1];
	//		if (lpApplicationName)
	//		{
	//			_wcscpyn_c(szExe, countof(szExe), lpApplicationName, countof(szExe));
	//		}
	//		else
	//		{
	//			//LPCWSTR pszTemp = lpCommandLine;
	//			//if (NextArg(&pszTemp, szExe) != 0)
	//			szExe[0] = 0;
	//			BOOL lbNeedCutStartEndQuot = FALSE;
	//			IsNeedCmd(lpCommandLine, &lbNeedCutStartEndQuot, szExe);
	//		}
	//		if (szExe[0])
	//			GetImageSubsystem(szExe, nImageSubsystem, nImageBits, nFileAttrs);
	//		lbNeedInjects = (nImageSubsystem != IMAGE_SUBSYSTEM_WINDOWS_GUI);

	//		LPCWSTR pszName = PointToName(szExe);
	//		if (pszName && lbNeedInjects)
	//		{
	//			if (lstrcmpiW(pszName, L"ConEmuC.exe") == 0 || lstrcmpiW(pszName, L"ConEmuC64.exe") == 0)
	//				lbParamsChanged = TRUE;
	//		}
	//	}

	//	if (!lbSuspended && !lbParamsChanged && lbNeedInjects)
	//		dwCreationFlags |= CREATE_SUSPENDED;
	//}

	lbRc = F(CreateProcessW)(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	if (!lbRc) dwErr = GetLastError();

	// Если lbParamsChanged == TRUE - об инжектах позаботится ConEmuC.exe
	sp.OnCreateProcessFinished(lbRc, lpProcessInformation);

	//if (lbRc && ghConEmuWndDC && !lbParamsChanged && lbNeedInjects)
	//{
	//	int iHookRc = InjectHooks(*lpProcessInformation, FALSE);

	//	if (iHookRc != 0)
	//	{
	//		DWORD nErrCode = GetLastError();
	//		_ASSERTE(iHookRc == 0);
	//		wchar_t szDbgMsg[255], szTitle[128];
	//		_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuC, PID=%u", GetCurrentProcessId());
	//		_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"ConEmuC.W, PID=%u\nInjecting hooks into PID=%u\nFAILED, code=%i:0x%08X", GetCurrentProcessId(), lpProcessInformation->dwProcessId, iHookRc, nErrCode);
	//		MessageBoxW(NULL, szDbgMsg, szTitle, MB_SYSTEMMODAL);
	//	}

	//	// Отпустить процесс
	//	if (!lbSuspended)
	//		ResumeThread(lpProcessInformation->hThread);
	//}

	if (ph && ph->PostCallBack)
	{
		SETARGS10(&lbRc, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
		ph->PostCallBack(&args);
	}
	
	//if (pszTempApp)
	//	free(pszTempApp);
	//if (pszTempCmd)
	//	free(pszTempCmd);

	return lbRc;
}


typedef BOOL (WINAPI* OnTrackPopupMenu_t)(HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, CONST RECT * prcRect);
static BOOL WINAPI OnTrackPopupMenu(HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, CONST RECT * prcRect)
{
	ORIGINALFAST(TrackPopupMenu);
#ifdef _DEBUG
	WCHAR szMsg[128]; _wsprintf(szMsg, SKIPLEN(countof(szMsg)) L"TrackPopupMenu(hwnd=0x%08X)\n", (DWORD)hWnd);
	DebugString(szMsg);
#endif

	if (ghConEmuWndDC)
	{
		// Необходимо "поднять" наверх консольное окно, т.к. от него идет меню
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
	WCHAR szMsg[128]; _wsprintf(szMsg, SKIPLEN(countof(szMsg)) L"TrackPopupMenuEx(hwnd=0x%08X)\n", (DWORD)hWnd);
	DebugString(szMsg);
#endif

	if (ghConEmuWndDC)
	{
		// Необходимо "поднять" наверх консольное окно, т.к. от него идет меню
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

	//LPSHELLEXECUTEINFOA lpNew = NULL;
	//DWORD dwProcessID = 0;
	//wchar_t* pszTempParm = NULL;
	//wchar_t szComSpec[MAX_PATH+1], szConEmuC[MAX_PATH+1]; szComSpec[0] = szConEmuC[0] = 0;
	//LPSTR pszTempApp = NULL, pszTempArg = NULL;
	//lpNew = (LPSHELLEXECUTEINFOA)malloc(lpExecInfo->cbSize);
	//memmove(lpNew, lpExecInfo, lpExecInfo->cbSize);
	
	CShellProc sp;
	sp.OnShellExecuteExA(&lpExecInfo);

	//// Under ConEmu only!
	//if (ghConEmuWndDC)
	//{
	//	if (!lpNew->hwnd || lpNew->hwnd == GetConsoleWindow())
	//		lpNew->hwnd = GetParent(ghConEmuWndDC);

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
	//		wchar_t szDbgMsg[128]; _wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"Process created: %u\n", dwProcessID);
	//		OutputDebugStringW(szDbgMsg);
	//	}
	//
	//#endif
	//lpExecInfo->hProcess = lpNew->hProcess;
	//lpExecInfo->hInstApp = lpNew->hInstApp;
	sp.OnShellFinished(lbRc, lpExecInfo->hInstApp, lpExecInfo->hProcess);

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
static BOOL OnShellExecuteExW_SEH(OnShellExecuteExW_t f, LPSHELLEXECUTEINFOW lpExecInfo, BOOL* pbRc)
{
	BOOL lbOk = FALSE;
	SAFETRY
	{
		*pbRc = f(lpExecInfo);
		lbOk = TRUE;
	} SAFECATCH
	{
		lbOk = FALSE;
	}
	return lbOk;
}
static BOOL WINAPI OnShellExecuteExW(LPSHELLEXECUTEINFOW lpExecInfo)
{
	ORIGINAL(ShellExecuteExW);
	//LPSHELLEXECUTEINFOW lpNew = NULL;
	//DWORD dwProcessID = 0;
	//wchar_t* pszTempParm = NULL;
	//wchar_t szComSpec[MAX_PATH+1], szConEmuC[MAX_PATH+1]; szComSpec[0] = szConEmuC[0] = 0;
	//LPWSTR pszTempApp = NULL, pszTempArg = NULL;
	//lpNew = (LPSHELLEXECUTEINFOW)malloc(lpExecInfo->cbSize);
	//memmove(lpNew, lpExecInfo, lpExecInfo->cbSize);

	//// Under ConEmu only!
	//if (ghConEmuWndDC)
	//{
	//	if (!lpNew->hwnd || lpNew->hwnd == GetConsoleWindow())
	//		lpNew->hwnd = GetParent(ghConEmuWndDC);

	//	HANDLE hDummy = NULL;
	//	DWORD nImageSubsystem = 0, nImageBits = 0;
	//	if (PrepareExecuteParmsW(eShellExecute, lpNew->lpVerb, lpNew->fMask, 
	//			lpNew->lpFile, lpNew->lpParameters,
	//			hDummy, hDummy, hDummy,
	//			&pszTempApp, &pszTempArg, nImageSubsystem, nImageBits))
	//	{
	//		// Меняем
	//		lpNew->lpFile = pszTempApp;
	//		lpNew->lpParameters = pszTempArg;
	//	}
	//}

	CShellProc sp;
	sp.OnShellExecuteExW(&lpExecInfo);

	BOOL lbRc;

	//if (gFarMode.bFarHookMode && gFarMode.bShellNoZoneCheck)
	//	lpNew->fMask |= SEE_MASK_NOZONECHECKS;

	//// Issue 351: После перехода исполнятеля фара на ShellExecuteEx почему-то сюда стал приходить левый хэндл
	//gbInShellExecuteEx = TRUE;

	//BUGBUG: FAR периодически валится на этой функции
	//должно быть: lpNew->cbSize==0x03C; lpNew->fMask==0x00800540;
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

	//if (lbRc && gfGetProcessId && lpNew->hProcess)
	//{
	//	dwProcessID = gfGetProcessId(lpNew->hProcess);
	//}

	//#ifdef _DEBUG
	//
	//	if (lpNew->lpParameters)
	//	{
	//		OutputDebugStringW(L"After ShellExecuteEx\n");
	//		OutputDebugStringW(lpNew->lpParameters);
	//		OutputDebugStringW(L"\n");
	//	}
	//
	//	if (lbRc && dwProcessID)
	//	{
	//		wchar_t szDbgMsg[128]; _wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"Process created: %u\n", dwProcessID);
	//		OutputDebugStringW(szDbgMsg);
	//	}
	//
	//#endif
	//lpExecInfo->hProcess = lpNew->hProcess;
	//lpExecInfo->hInstApp = lpNew->hInstApp;

	sp.OnShellFinished(lbRc, lpExecInfo->hInstApp, lpExecInfo->hProcess);


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

typedef HINSTANCE(WINAPI* OnShellExecuteA_t)(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd);
static HINSTANCE WINAPI OnShellExecuteA(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd)
{
	ORIGINALFAST(ShellExecuteA);

	if (ghConEmuWndDC)
	{
		if (!hwnd || hwnd == GetConsoleWindow())
			hwnd = GetParent(ghConEmuWndDC);
	}
	
	//gbInShellExecuteEx = TRUE;
	CShellProc sp;
	sp.OnShellExecuteA(&lpOperation, &lpFile, &lpParameters, NULL, (DWORD*)&nShowCmd);

	HINSTANCE lhRc;
	lhRc = F(ShellExecuteA)(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
	
	//gbInShellExecuteEx = FALSE;
	return lhRc;
}
typedef HINSTANCE(WINAPI* OnShellExecuteW_t)(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd);
static HINSTANCE WINAPI OnShellExecuteW(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd)
{
	ORIGINALFAST(ShellExecuteW);

	if (ghConEmuWndDC)
	{
		if (!hwnd || hwnd == GetConsoleWindow())
			hwnd = GetParent(ghConEmuWndDC);
	}
	
	//gbInShellExecuteEx = TRUE;
	CShellProc sp;
	sp.OnShellExecuteW(&lpOperation, &lpFile, &lpParameters, NULL, (DWORD*)&nShowCmd);

	HINSTANCE lhRc;
	lhRc = F(ShellExecuteW)(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
	
	//gbInShellExecuteEx = FALSE;
	return lhRc;
}

typedef BOOL (WINAPI* OnFlashWindow_t)(HWND hWnd, BOOL bInvert);
static BOOL WINAPI OnFlashWindow(HWND hWnd, BOOL bInvert)
{
	ORIGINALFAST(FlashWindow);

	if (ghConEmuWndDC)
	{
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

	if (ghConEmuWndDC)
	{
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

	if (ghConEmuWndDC)
	{
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

	if ((hWnd == ghConWnd) && ghConEmuWndDC)
	{
		//EMenu gui mode issues (center in window). "Remove" Far window from mouse cursor.
		hWnd = ghConEmuWndDC;
	}

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
static BOOL WINAPI OnScreenToClient(HWND hWnd, LPPOINT lpPoint)
{
	ORIGINALFAST(ScreenToClient);
	BOOL lbRc;
	lbRc = F(ScreenToClient)(hWnd, lpPoint);

	if (ghConEmuWndDC && lpPoint && (hWnd == ghConWnd))
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
static int WINAPI OnCompareStringW(LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2)
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
static DWORD WINAPI OnGetConsoleAliasesW(LPWSTR AliasBuffer, DWORD AliasBufferLength, LPWSTR ExeName)
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
static DWORD WINAPI SetConsoleCPThread(LPVOID lpParameter)
{
	SCOCP* p = (SCOCP*)lpParameter;
	p->dwErrCode = -1;
	SetEvent(p->hReady);
	p->lbRc = p->f(p->wCodePageID);
	p->dwErrCode = GetLastError();
	return 0;
}
static BOOL WINAPI OnSetConsoleCP(UINT wCodePageID)
{
	ORIGINALFAST(SetConsoleCP);
	_ASSERTE(OnSetConsoleCP!=SetConsoleCP);
	BOOL lbRc = FALSE;
	SCOCP sco = {wCodePageID, (OnSetConsoleCP_t)F(SetConsoleCP), CreateEvent(NULL,FALSE,FALSE,NULL)};
	DWORD nTID = 0, nWait = -1, nErr;
	wchar_t szErrText[255], szTitle[64];
	_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"PID=%u, TID=%u", GetCurrentProcessId(), GetCurrentThreadId());
	DWORD nPrevCP = GetConsoleCP(), nCurCP = 0;
	HANDLE hThread = CreateThread(NULL,0,SetConsoleCPThread,&sco,0,&nTID);

	if (!hThread)
	{
		nErr = GetLastError();
		_wsprintf(szErrText, SKIPLEN(countof(szErrText)) L"chcp failed, ErrCode=0x%08X\nConEmuHooks.cpp:OnSetConsoleCP", nErr);
		MessageBoxW(NULL, szErrText, szTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
		lbRc = FALSE;
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
				_wsprintf(szErrText, SKIPLEN(countof(szErrText))
				          L"chcp hung detected\n"
				          L"ConEmuHooks.cpp:OnSetConsoleCP\n"
				          L"ReqCP=%u, PrevCP=%u, CurCP=%u\n"
				          //L"\nPress <OK> to stop waiting"
				          ,wCodePageID, nPrevCP, nCurCP);
				MessageBoxW(NULL, szErrText, szTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
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
static BOOL WINAPI OnSetConsoleOutputCP(UINT wCodePageID)
{
	ORIGINALFAST(SetConsoleOutputCP);
	_ASSERTE(OnSetConsoleOutputCP!=SetConsoleOutputCP);
	BOOL lbRc = FALSE;
	SCOCP sco = {wCodePageID, (OnSetConsoleCP_t)F(SetConsoleOutputCP), CreateEvent(NULL,FALSE,FALSE,NULL)};
	DWORD nTID = 0, nWait = -1, nErr;
	wchar_t szErrText[255], szTitle[64];
	_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"PID=%u, TID=%u", GetCurrentProcessId(), GetCurrentThreadId());
	DWORD nPrevCP = GetConsoleOutputCP(), nCurCP = 0;
	HANDLE hThread = CreateThread(NULL,0,SetConsoleCPThread,&sco,0,&nTID);

	if (!hThread)
	{
		nErr = GetLastError();
		_wsprintf(szErrText, SKIPLEN(countof(szErrText)) L"chcp(out) failed, ErrCode=0x%08X\nConEmuHooks.cpp:OnSetConsoleOutputCP", nErr);
		MessageBoxW(NULL, szErrText, szTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
		lbRc = FALSE;
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
				_wsprintf(szErrText, SKIPLEN(countof(szErrText))
				          L"chcp(out) hung detected\n"
				          L"ConEmuHooks.cpp:OnSetConsoleOutputCP\n"
				          L"ReqOutCP=%u, PrevOutCP=%u, CurOutCP=%u\n"
				          //L"\nPress <OK> to stop waiting"
				          ,wCodePageID, nPrevCP, nCurCP);
				MessageBoxW(NULL, szErrText, szTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
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
static BOOL WINAPI OnGetNumberOfConsoleInputEvents(HANDLE hConsoleInput, LPDWORD lpcNumberOfEvents)
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
//static BOOL WINAPI OnPeekConsoleInputAx(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
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
//static BOOL WINAPI OnPeekConsoleInputWx(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
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
//static BOOL WINAPI OnReadConsoleInputAx(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
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
//static BOOL WINAPI OnReadConsoleInputWx(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
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

typedef BOOL (WINAPI* OnPeekConsoleInputA_t)(HANDLE,PINPUT_RECORD,DWORD,LPDWORD);
static BOOL WINAPI OnPeekConsoleInputA(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
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

	#ifdef USE_INPUT_SEMAPHORE
	DWORD nSemaphore = ghConInSemaphore ? WaitForSingleObject(ghConInSemaphore, INSEMTIMEOUT_READ) : 1;
	_ASSERTE(nSemaphore<=1);
	#endif

	lbRc = F(PeekConsoleInputA)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);

	#ifdef USE_INPUT_SEMAPHORE
	if ((nSemaphore == WAIT_OBJECT_0) && ghConInSemaphore) ReleaseSemaphore(ghConInSemaphore, 1, NULL);
	#endif

	if (ph && ph->PostCallBack)
	{
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

	if (ph && ph->PreCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);

		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	#ifdef USE_INPUT_SEMAPHORE
	DWORD nSemaphore = ghConInSemaphore ? WaitForSingleObject(ghConInSemaphore, INSEMTIMEOUT_READ) : 1;
	_ASSERTE(nSemaphore<=1);
	#endif

	lbRc = F(PeekConsoleInputW)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);

	#ifdef USE_INPUT_SEMAPHORE
	if ((nSemaphore == WAIT_OBJECT_0) && ghConInSemaphore) ReleaseSemaphore(ghConInSemaphore, 1, NULL);
	#endif

	if (ph && ph->PostCallBack)
	{
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

	if (ph && ph->PreCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);

		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	#ifdef USE_INPUT_SEMAPHORE
	DWORD nSemaphore = ghConInSemaphore ? WaitForSingleObject(ghConInSemaphore, INSEMTIMEOUT_READ) : 1;
	_ASSERTE(nSemaphore<=1);
	#endif

	lbRc = F(ReadConsoleInputA)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);

	#ifdef USE_INPUT_SEMAPHORE
	if ((nSemaphore == WAIT_OBJECT_0) && ghConInSemaphore) ReleaseSemaphore(ghConInSemaphore, 1, NULL);
	#endif

	if (ph && ph->PostCallBack)
	{
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

	if (ph && ph->PreCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);

		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	#ifdef USE_INPUT_SEMAPHORE
	DWORD nSemaphore = ghConInSemaphore ? WaitForSingleObject(ghConInSemaphore, INSEMTIMEOUT_READ) : 1;
	_ASSERTE(nSemaphore<=1);
	#endif

	lbRc = F(ReadConsoleInputW)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);

	#ifdef USE_INPUT_SEMAPHORE
	if ((nSemaphore == WAIT_OBJECT_0) && ghConInSemaphore) ReleaseSemaphore(ghConInSemaphore, 1, NULL);
	#endif

	if (ph && ph->PostCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);
		ph->PostCallBack(&args);
	}

	return lbRc;
}

typedef HANDLE(WINAPI* OnCreateConsoleScreenBuffer_t)(DWORD dwDesiredAccess, DWORD dwShareMode, const SECURITY_ATTRIBUTES *lpSecurityAttributes, DWORD dwFlags, LPVOID lpScreenBufferData);
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

	if (ph && ph->PreCallBack)
	{
		SETARGS(&lbRc);

		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	lbRc = F(AllocConsole)();

	InitializeConsoleInputSemaphore();

	if (ph && ph->PostCallBack)
	{
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

	if (ph && ph->PreCallBack)
	{
		SETARGS(&lbRc);

		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	ReleaseConsoleInputSemaphore();

	lbRc = F(FreeConsole)();

	if (ph && ph->PostCallBack)
	{
		SETARGS(&lbRc);
		ph->PostCallBack(&args);
	}

	return lbRc;
}

//typedef HWND (WINAPI* OnGetConsoleWindow_t)(void);
//static HWND WINAPI OnGetConsoleWindow(void)
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
static BOOL WINAPI OnWriteConsoleOutputA(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion)
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
static BOOL WINAPI OnWriteConsoleOutputW(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion)
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

//typedef BOOL (WINAPI* OnWriteConsoleOutputAx_t)(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
//static BOOL WINAPI OnWriteConsoleOutputAx(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion)
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
//static BOOL WINAPI OnWriteConsoleOutputWx(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion)
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
static HANDLE WINAPI OnCreateNamedPipeW(LPCWSTR lpName, DWORD dwOpenMode, DWORD dwPipeMode, DWORD nMaxInstances,DWORD nOutBufferSize, DWORD nInBufferSize, DWORD nDefaultTimeOut,LPSECURITY_ATTRIBUTES lpSecurityAttributes)
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


//110131 попробуем просто добвавить ее в ExcludedModules
//// WinInet.dll
//typedef BOOL (WINAPI* OnHttpSendRequestA_t)(LPVOID hRequest, LPCSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength);
//typedef BOOL (WINAPI* OnHttpSendRequestW_t)(LPVOID hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength);
//// смысла нет - __try не помогает
////static BOOL OnHttpSendRequestA_SEH(OnHttpSendRequestA_t f, LPVOID hRequest, LPCSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength, BOOL* pbRc)
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
////static BOOL OnHttpSendRequestW_SEH(OnHttpSendRequestW_t f, LPVOID hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength, BOOL* pbRc)
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
//static BOOL WINAPI OnHttpSendRequestA(LPVOID hRequest, LPCSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength)
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
//static BOOL WINAPI OnHttpSendRequestW(LPVOID hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength)
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

static void GuiSetForeground(HWND hWnd)
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

static void GuiFlashWindow(BOOL bSimple, HWND hWnd, BOOL bInvert, DWORD dwFlags, UINT uCount, DWORD dwTimeout)
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

static bool IsHandleConsole(HANDLE handle, bool output = true)
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


//static BOOL SrvSetConsoleCP(BOOL bSetOutputCP, DWORD nCP)
//{
//	_ASSERTE(ghConEmuWndDC);
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


//static BOOL ChangeExecuteParms(enum CmdOnCreateType aCmd,
//				LPCWSTR asFile, LPCWSTR asParam, LPCWSTR asBaseDir,
//				LPCWSTR asExeFile, int ImageBits, int ImageSubsystem,
//				LPWSTR* psFile, LPWSTR* psParam)
//{
//	wchar_t szConEmuC[MAX_PATH+16]; // ConEmuC64.exe
//	wcscpy_c(szConEmuC, asBaseDir);
//	
//	if (ImageBits == 32)
//	{
//		wcscat_c(szConEmuC, L"ConEmuC.exe");
//	}
//	else if (ImageBits == 64)
//	{
//		wcscat_c(szConEmuC, L"ConEmuC64.exe");
//	}
//	else if (ImageBits == 16)
//	{
//		_ASSERTE(ImageBits != 16);
//		return FALSE;
//	}
//	else
//	{
//		_ASSERTE(ImageBits==16||ImageBits==32||ImageBits==64);
//		wcscat_c(szConEmuC, L"ConEmuC.exe");
//	}
//	
//	int nCchSize = (asFile ? lstrlenW(asFile) : 0) + (asParam ? lstrlenW(asParam) : 0) + 32;
//	if (aCmd == eShellExecute)
//	{
//		*psFile = lstrdup(szConEmuC);
//	}
//	else
//	{
//		nCchSize += lstrlenW(szConEmuC);
//		*psFile = NULL;
//	}
//	
//	*psParam = (wchar_t*)malloc(nCchSize*sizeof(wchar_t));
//	(*psParam)[0] = 0;
//	if (aCmd == eShellExecute)
//	{
//		_wcscat_c((*psParam), nCchSize, L" /C \"");
//		_wcscat_c((*psParam), nCchSize, asFile ? asFile : L"");
//		_wcscat_c((*psParam), nCchSize, L"\"");
//	}
//	else
//	{
//		(*psParam)[0] = L'"';
//		_wcscpy_c((*psParam)+1, nCchSize-1, szConEmuC);
//		_wcscat_c((*psParam), nCchSize, L"\" /C ");
//		// Это CreateProcess. Исполняемый файл может быть уже указан в asParam
//		BOOL lbNeedExe = TRUE;
//		if (asParam && *asParam)
//		{
//			LPCWSTR pszParam = asParam;
//			while (*pszParam == L'"')
//				pszParam++;
//			int nLen = lstrlenW(asExeFile);
//			wchar_t szTest[MAX_PATH*2];
//			_ASSERTE(nLen <= (MAX_PATH+10)); // размер из IsNeedCmd.
//			_wcscpyn_c(szTest, countof(szTest), asExeFile, nLen+1);
//			if (lstrcmpiW(szTest, asExeFile) == 0)
//				lbNeedExe = FALSE;
//		}
//		if (lbNeedExe)
//		{
//			_wcscat_c((*psParam), nCchSize, L"\"");
//			_ASSERTE(asFile!=NULL);
//			_wcscat_c((*psParam), nCchSize, asFile ? asFile : L"");
//			_wcscat_c((*psParam), nCchSize, L"\"");
//		}
//	}
//
//	if (asParam && *asParam)
//	{
//		_wcscat_c((*psParam), nCchSize, L" ");
//		_wcscat_c((*psParam), nCchSize, asParam);
//	}
//
//	return TRUE;
//}

//static wchar_t* str2wcs(const char* psz, UINT anCP)
//{
//	if (!psz)
//		return NULL;
//	int nLen = lstrlenA(psz);
//	wchar_t* pwsz = (wchar_t*)calloc((nLen+1),sizeof(wchar_t));
//	if (nLen > 0)
//	{
//		MultiByteToWideChar(anCP, 0, psz, nLen+1, pwsz, nLen+1);
//	}
//	else
//	{
//		pwsz[0] = 0;
//	}
//	return pwsz;
//}
//static char* wcs2str(const wchar_t* pwsz, UINT anCP)
//{
//	int nLen = lstrlenW(pwsz);
//	char* psz = (char*)calloc((nLen+1),sizeof(char));
//	if (nLen > 0)
//	{
//		WideCharToMultiByte(anCP, 0, pwsz, nLen+1, psz, nLen+1, 0, 0);
//	}
//	else
//	{
//		psz[0] = 0;
//	}
//	return psz;
//}

//static BOOL PrepareExecuteParmsA(
//				enum CmdOnCreateType aCmd, LPCSTR asAction, DWORD anFlags, 
//				LPCSTR asFile, LPCSTR asParam,
//				HANDLE hStdIn, HANDLE hStdOut, HANDLE hStdErr,
//				LPSTR* psFile, LPSTR* psParam, DWORD& nImageSubsystem, DWORD& nImageBits)
//{
//	_ASSERTE(*psFile==NULL && *psParam==NULL);
//	if (!ghConEmuWndDC)
//		return FALSE; // Перехватывать только под ConEmu
//
//	UINT nCP = AreFileApisANSI() ? CP_ACP : CP_OEMCP;
//	LPWSTR pwszAction = str2wcs(asAction, nCP);
//	LPWSTR pwszFile = str2wcs(asFile, nCP);
//	LPWSTR pwszParam = str2wcs(asParam, nCP);
//	LPWSTR pwszRetFile = NULL;
//	LPWSTR pwszRetParam = NULL;
//
//	BOOL lbRc = PrepareExecuteParmsW(aCmd, pwszAction, anFlags, pwszFile, pwszParam, 
//			hStdIn, hStdOut, hStdErr, &pwszRetFile, &pwszRetParam, nImageSubsystem, nImageBits);
//
//	if (lbRc)
//	{
//		if (pwszRetFile && *pwszRetFile)
//			*psFile = wcs2str(pwszRetFile, nCP);
//		else
//			*psFile = NULL;
//		*psParam = wcs2str(pwszRetParam, nCP);
//	}
//
//	if (pwszAction) free(pwszAction);
//	if (pwszFile) free(pwszFile);
//	if (pwszParam) free(pwszParam);
//	if (pwszRetFile) free(pwszRetFile);
//	if (pwszRetParam) free(pwszRetParam);
//
//	return lbRc;
//}

//BOOL IsExecutable(LPCWSTR aszFilePathName);
//BOOL IsNeedCmd(LPCWSTR asCmdLine, BOOL *rbNeedCutStartEndQuot, wchar_t (&szExe)[MAX_PATH+10]);

//CESERVER_REQ* NewCmdOnCreateA(
//				enum CmdOnCreateType aCmd, LPCSTR asAction, DWORD anFlags, 
//				LPCSTR asFile, LPCSTR asParam, int nImageBits, int nImageSubsystem,
//				HANDLE hStdIn, HANDLE hStdOut, HANDLE hStdErr)
//{
//	UINT nCP = AreFileApisANSI() ? CP_ACP : CP_OEMCP;
//	LPWSTR pwszAction = str2wcs(asAction, nCP);
//	LPWSTR pwszFile = str2wcs(asFile, nCP);
//	LPWSTR pwszParam = str2wcs(asParam, nCP);
//
//	wchar_t szBaseDir[MAX_PATH+2];
//	CESERVER_REQ* pIn = NewCmdOnCreateW(aCmd, pwszAction, anFlags, pwszFile, pwszParam, 
//			nImageBits, nImageSubsystem, hStdIn, hStdOut, hStdErr, szBaseDir);
//
//	if (pwszAction) free(pwszAction);
//	if (pwszFile) free(pwszFile);
//	if (pwszParam) free(pwszParam);
//	
//	return pIn;
//}

//CESERVER_REQ* NewCmdOnCreateW(
//				enum CmdOnCreateType aCmd, LPCWSTR asAction, DWORD anFlags, 
//				LPCWSTR asFile, LPCWSTR asParam, int nImageBits, int nImageSubsystem,
//				HANDLE hStdIn, HANDLE hStdOut, HANDLE hStdErr,
//				wchar_t (&szBaseDir)[MAX_PATH+2])
//{
//	szBaseDir[0] = 0;
//
//	// Проверим, а надо ли?
//	DWORD dwGuiProcessId = 0;
//	if (!ghConEmuWnd || !GetWindowThreadProcessId(ghConEmuWnd, &dwGuiProcessId))
//		return NULL;
//	MFileMapping<ConEmuGuiMapping> GuiInfoMapping;
//	GuiInfoMapping.InitName(CEGUIINFOMAPNAME, dwGuiProcessId);
//	const ConEmuGuiMapping* pInfo = GuiInfoMapping.Open();
//	if (!pInfo)
//		return NULL;
//	else if (pInfo->nProtocolVersion != CESERVER_REQ_VER)
//		return NULL;
//	else
//	{
//		wcscpy_c(szBaseDir, pInfo->sConEmuBaseDir);
//		wcscat_c(szBaseDir, L"\\");
//		if (pInfo->nLoggingType != glt_Processes)
//			return NULL;
//	}
//	GuiInfoMapping.CloseMap();
//
//	
//	CESERVER_REQ *pIn = NULL;
//	
//	int nActionLen = (asAction ? lstrlenW(asAction) : 0)+1;
//	int nFileLen = (asFile ? lstrlenW(asFile) : 0)+1;
//	int nParamLen = (asParam ? lstrlenW(asParam) : 0)+1;
//	
//	pIn = ExecuteNewCmd(CECMD_ONCREATEPROC, sizeof(CESERVER_REQ_HDR)
//		+sizeof(CESERVER_REQ_ONCREATEPROCESS)+(nActionLen+nFileLen+nParamLen)*sizeof(wchar_t));
//	
//	//pIn->OnCreateProc.bUnicode = TRUE;
//	pIn->OnCreateProc.nImageSubsystem = nImageSubsystem;
//	pIn->OnCreateProc.nImageBits = nImageBits;
//	pIn->OnCreateProc.hStdIn = (unsigned __int64)hStdIn;
//	pIn->OnCreateProc.hStdOut = (unsigned __int64)hStdOut;
//	pIn->OnCreateProc.hStdErr = (unsigned __int64)hStdErr;
//	
//	if (aCmd == eShellExecute)
//		wcscpy_c(pIn->OnCreateProc.sFunction, L"Shell");
//	else if (aCmd == eCreateProcess)
//		wcscpy_c(pIn->OnCreateProc.sFunction, L"Create");
//	else if (aCmd == eInjectingHooks)
//		wcscpy_c(pIn->OnCreateProc.sFunction, L"Hooks");
//	else if (aCmd == eHooksLoaded)
//		wcscpy_c(pIn->OnCreateProc.sFunction, L"Loaded");
//	else
//		wcscpy_c(pIn->OnCreateProc.sFunction, L"Unknown");
//	
//	pIn->OnCreateProc.nFlags = anFlags;
//	pIn->OnCreateProc.nActionLen = nActionLen;
//	pIn->OnCreateProc.nFileLen = nFileLen;
//	pIn->OnCreateProc.nParamLen = nParamLen;
//	
//	wchar_t* psz = pIn->OnCreateProc.wsValue;
//	if (nActionLen > 1)
//		_wcscpy_c(psz, nActionLen, asAction);
//	psz += nActionLen;
//	if (nFileLen > 1)
//		_wcscpy_c(psz, nFileLen, asFile);
//	psz += nFileLen;
//	if (nParamLen > 1)
//		_wcscpy_c(psz, nParamLen, asParam);
//	psz += nParamLen;
//	
//	return pIn;
//}

//static BOOL PrepareExecuteParmsW(
//				enum CmdOnCreateType aCmd, LPCWSTR asAction, DWORD anFlags, 
//				LPCWSTR asFile, LPCWSTR asParam,
//				HANDLE hStdIn, HANDLE hStdOut, HANDLE hStdErr,
//				LPWSTR* psFile, LPWSTR* psParam, DWORD& nImageSubsystem, DWORD& nImageBits)
//{
//	_ASSERTE(*psFile==NULL && *psParam==NULL);
//	if (!ghConEmuWndDC)
//		return FALSE; // Перехватывать только под ConEmu
//	
//	wchar_t szTest[MAX_PATH*2], szExe[MAX_PATH+1];
//	DWORD /*nImageSubsystem = 0, nImageBits = 0,*/ nFileAttrs = (DWORD)-1;
//	bool lbGuiApp = false;
//	//int nActionLen = (asAction ? lstrlenW(asAction) : 0)+1;
//	//int nFileLen = (asFile ? lstrlenW(asFile) : 0)+1;
//	//int nParamLen = (asParam ? lstrlenW(asParam) : 0)+1;
//	BOOL lbNeedCutStartEndQuot = FALSE;
//
//	nImageSubsystem = nImageBits = 0;
//	
//	if ((aCmd == eShellExecute) || (asFile && *asFile))
//	{
//		wcscpy_c(szExe, asFile ? asFile : L"");
//	}
//	else
//	{
//		IsNeedCmd(asParam, &lbNeedCutStartEndQuot, szExe);
//	}
//	
//	if (szExe[0])
//	{
//		wchar_t *pszNamePart = NULL;
//		int nLen = lstrlen(szExe);
//		BOOL lbMayBeFile = (nLen > 0) && (szExe[nLen-1] != L'\\') && (szExe[nLen-1] != L'/');
//
//		BOOL lbSubsystemOk = FALSE;
//		nImageBits = 0;
//		nImageSubsystem = IMAGE_SUBSYSTEM_UNKNOWN;
//
//		if (!lbMayBeFile)
//		{
//			nImageBits = 0;
//			nImageSubsystem = IMAGE_SUBSYSTEM_UNKNOWN;
//		}
//		else if (GetFullPathName(szExe, countof(szTest), szTest, &pszNamePart)
//			&& GetImageSubsystem(szTest, nImageSubsystem, nImageBits, nFileAttrs))
//		{
//			lbGuiApp = (nImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI);
//			lbSubsystemOk = TRUE;
//		}
//		else if (SearchPath(NULL, szExe, NULL, countof(szTest), szTest, &pszNamePart)
//			&& GetImageSubsystem(szTest, nImageSubsystem, nImageBits, nFileAttrs))
//		{
//			lbGuiApp = (nImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI);
//			lbSubsystemOk = TRUE;
//		}
//		else
//		{
//			szTest[0] = 0;
//		}
//		
//
//		if (!lbSubsystemOk)
//		{
//			nImageBits = 0;
//			nImageSubsystem = IMAGE_SUBSYSTEM_UNKNOWN;
//
//			if ((nFileAttrs != (DWORD)-1) && !(nFileAttrs & FILE_ATTRIBUTE_DIRECTORY))
//			{
//				LPCWSTR pszExt = wcsrchr(szTest, L'.');
//				if (pszExt)
//				{
//					if ((lstrcmpiW(pszExt, L".cmd") == 0) || (lstrcmpiW(pszExt, L".bat") == 0))
//					{
//						#ifdef _WIN64
//						nImageBits = 64;
//						#else
//						nImageBits = 32;
//						#endif
//						nImageSubsystem = IMAGE_SUBSYSTEM_BATCH_FILE;
//					}
//				}
//			}
//		}
//	}
//	
//	
//	BOOL lbChanged = FALSE;
//	wchar_t szBaseDir[MAX_PATH+2]; szBaseDir[0] = 0;
//	CESERVER_REQ *pIn = NULL;
//	pIn = NewCmdOnCreateW(aCmd, asAction, anFlags, asFile, asParam, nImageBits, nImageSubsystem, hStdIn, hStdOut, hStdErr, szBaseDir);
//	if (pIn)
//	{
//		HWND hConWnd = GetConsoleWindow();
//		CESERVER_REQ *pOut = NULL;
//		pOut = ExecuteGuiCmd(hConWnd, pIn, hConWnd);
//		ExecuteFreeResult(pIn); pIn = NULL;
//		if (!pOut)
//			return FALSE;
//		if (!pOut->OnCreateProcRet.bContinue)
//			goto wrap;
//		ExecuteFreeResult(pOut);
//	}
//
//	//wchar_t* pszExecFile = (wchar_t*)pOut->OnCreateProcRet.wsValue;
//	//wchar_t* pszBaseDir = (wchar_t*)(pOut->OnCreateProcRet.wsValue); // + pOut->OnCreateProcRet.nFileLen);
//	
//	
//	if (aCmd == eShellExecute)
//	{
//		WARNING("Уточнить условие для флагов ShellExecute!");
//		if ((anFlags & (SEE_MASK_FLAG_NO_UI|SEE_MASK_NOASYNC|SEE_MASK_NOCLOSEPROCESS|SEE_MASK_NO_CONSOLE))
//	        != (SEE_MASK_FLAG_NO_UI|SEE_MASK_NOASYNC|SEE_MASK_NOCLOSEPROCESS|SEE_MASK_NO_CONSOLE))
//			goto wrap; // пока так - это фар выполняет консольную команду
//		if (asAction && (lstrcmpiW(asAction, L"open") != 0))
//			goto wrap; // runas, print, и прочая нас не интересует
//	}
//	else
//	{
//		// Посмотреть, какие еще условия нужно отсеять для CreateProcess?
//		if ((anFlags & (CREATE_NO_WINDOW|DETACHED_PROCESS)) != 0)
//			goto wrap; // запускается по тихому (без консольного окна), пропускаем
//	}
//	
//	//bool lbGuiApp = false;
//	//DWORD ImageSubsystem = 0, ImageBits = 0;
//
//	//if (!pszExecFile || !*pszExecFile)
//	if (szExe[0] == 0)
//	{
//		_ASSERTE(szExe[0] != 0);
//		goto wrap; // ошибка?
//	}
//	//if (GetImageSubsystem(pszExecFile,ImageSubsystem,ImageBits))
//	//lbGuiApp = (ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI);
//
//	if (lbGuiApp)
//		goto wrap; // гуй - не перехватывать
//
//	// Подставлять ConEmuC.exe нужно только для того, чтобы в фаре 
//	// включить длинный буфер и запомнить длинный результат в консоли
//	if (gFarMode.bFarHookMode)
//	{
//		lbChanged = ChangeExecuteParms(aCmd, asFile, asParam, szBaseDir, 
//						szExe, nImageBits, nImageSubsystem, psFile, psParam);
//	}
//	else 
//	if ((nImageBits == 16 ) && (nImageSubsystem == IMAGE_SUBSYSTEM_DOS_EXECUTABLE))
//	{
//		TODO("DosBox?");
//	}
//	else
//	{
//		//lbChanged = ChangeExecuteParms(aCmd, asFile, asParam, pszBaseDir, 
//		//				szExe, nImageBits, nImageSubsystem, psFile, psParam);
//		lbChanged = FALSE;
//	}
//
//wrap:
//	return lbChanged;
//}
