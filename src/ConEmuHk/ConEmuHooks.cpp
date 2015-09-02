
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


#define HOOKS_VIRTUAL_ALLOC
#ifdef _DEBUG
	#define REPORT_VIRTUAL_ALLOC
#else
	#undef REPORT_VIRTUAL_ALLOC
#endif

#ifdef _DEBUG
	#define DefTermMsg(s) //MessageBox(NULL, s, L"ConEmuHk", MB_SYSTEMMODAL)
#else
	#define DefTermMsg(s) //MessageBox(NULL, s, L"ConEmuHk", MB_SYSTEMMODAL)
#endif

#ifdef _DEBUG
	#define BsDelWordMsg(s) //MessageBox(NULL, s, L"OnPromptBsDeleteWord called", MB_SYSTEMMODAL);
#else
	#define BsDelWordMsg(s) //MessageBox(NULL, s, L"OnPromptBsDeleteWord called", MB_SYSTEMMODAL);
#endif

#ifdef _DEBUG
	//#define PRE_PEEK_CONSOLE_INPUT
	#undef PRE_PEEK_CONSOLE_INPUT
#else
	#undef PRE_PEEK_CONSOLE_INPUT
#endif

#define DROP_SETCP_ON_WIN2K3R2
//#define SHOWDEBUGSTR -- специально отключено, CONEMU_MINIMAL, OutputDebugString могут нарушать работу процессов
//#define SKIPHOOKLOG

#undef SHOWCREATEPROCESSTICK
#undef SHOWCREATEBUFFERINFO
#ifdef _DEBUG
	#define SHOWCREATEPROCESSTICK
//	#define SHOWCREATEBUFFERINFO
	#define DUMP_VIM_SETCURSORPOS
#endif

#ifdef _DEBUG
	//#define TRAP_ON_MOUSE_0x0
	#undef TRAP_ON_MOUSE_0x0
	#undef LOG_GETANCESTOR
#else
	#undef TRAP_ON_MOUSE_0x0
	#undef LOG_GETANCESTOR
#endif

// Иначе не определяется GetConsoleAliases (хотя он должен быть доступен в Win2k)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501

#define DEFINE_HOOK_MACROS

#include <windows.h>
#include <WinError.h>
#include <WinNT.h>
#include <TCHAR.h>
#include <Tlhelp32.h>
#include <shlwapi.h>
#include "../common/common.hpp"
#include "../common/ConEmuCheck.h"
#include "SetHook.h"
#include "../common/execute.h"
#include "ConEmuHooks.h"
#include "DefTermHk.h"
#include "RegHooks.h"
#include "ShellProcessor.h"
#include "GuiAttach.h"
#include "Ansi.h"
#include "MainThread.h"
#include "../common/CmdLine.h"
#include "../common/ConsoleAnnotation.h"
#include "../common/ConsoleRead.h"
#include "../common/HkFunc.h"
#include "../common/UnicodeChars.h"
#include "../common/WConsole.h"
#include "../common/WThreads.h"

#include "hkCmdExe.h"
#include "hkConsole.h"
#include "hkConsoleInput.h"
#include "hkConsoleOutput.h"
#include "hkDialog.h"
#include "hkGDI.h"
#include "hkProcess.h"
#include "hkStdIO.h"
#include "hkWindow.h"


#ifdef _DEBUG
	//#define DEBUG_CON_TITLE
	#undef DEBUG_CON_TITLE
	#ifdef DEBUG_CON_TITLE
	CEStr* gpLastSetConTitle = NULL;
	#endif
#else
	#undef DEBUG_CON_TITLE
#endif


/* Forward declarations */
BOOL IsVisibleRectLocked(COORD& crLocked);
void PatchDialogParentWnd(HWND& hWndParent);


#undef isPressed
#define isPressed(inp) ((GetKeyState(inp) & 0x8000) == 0x8000)


#ifdef _DEBUG
extern bool gbShowExeMsgBox;
extern DWORD gnLastShowExeTick;
#define force_print_timings(s) { \
	DWORD w, nCurTick = GetTickCount(); \
	msprintf(szTimingMsg, countof(szTimingMsg), L">>> PID=%u >>> %u >>> %s\n", GetCurrentProcessId(), (nCurTick - gnLastShowExeTick), s); \
	/*OnWriteConsoleW(hTimingHandle, szTimingMsg, lstrlen(szTimingMsg), &w, NULL);*/ UNREFERENCED_PARAMETER(w); \
	gnLastShowExeTick = nCurTick; \
	}
#define print_timings(s) if (gbShowExeMsgBox) { force_print_timings(s); }
#else
#define print_timings(s)
#endif


#ifdef _DEBUG
#define DebugString(x) //OutputDebugString(x)
#else
#define DebugString(x) //OutputDebugString(x)
#endif


//#ifdef _DEBUG
//	#include <crtdbg.h>
//#else
//	#ifndef _ASSERTE
//		#define _ASSERTE(x)
//	#endif
//#endif

extern HMODULE ghOurModule; // Хэндл нашей dll'ки (здесь хуки не ставятся)
extern MMap<DWORD,BOOL> gStartedThreads;
//__declspec( thread )
//static BOOL    gbInShellExecuteEx = FALSE;

//#ifdef USE_INPUT_SEMAPHORE
//HANDLE ghConInSemaphore = NULL;
//#endif

/* ************ Globals for SetHook ************ */
extern HWND    ghConWnd;      // RealConsole
extern HWND    ghConEmuWnd;   // Root! ConEmu window
extern HWND    ghConEmuWndDC; // ConEmu DC window
extern DWORD   gnGuiPID;
extern HANDLE ghCurrentOutBuffer;
/* ************ Globals for SetHook ************ */


/* ************ Globals for Far Hooks ************ */
HookModeFar gFarMode = {sizeof(HookModeFar)};
bool    gbIsFarProcess = false;
InQueue gInQueue = {};
HANDLE  ghConsoleCursorChanged = NULL;
/* ************ Globals for Far Hooks ************ */

/* ************ Globals for cmd.exe/clink ************ */
bool     gbIsCmdProcess = false;
static bool IsInteractive();
//size_t   gcchLastWriteConsoleMax = 0;
//wchar_t *gpszLastWriteConsole = NULL;
//HMODULE  ghClinkDll = NULL;
//call_readline_t gpfnClinkReadLine = NULL;
int      gnCmdInitialized = 0; // 0 - Not already, 1 - OK, -1 - Fail
bool     gbAllowClinkUsage = false;
bool     gbAllowUncPaths = false;
bool     gbCurDirChanged = false;
/* ************ Globals for cmd.exe/clink ************ */

/* ************ Globals for powershell ************ */
bool gbPowerShellMonitorProgress = false;
WORD gnConsolePopupColors = 0x003E;
int  gnPowerShellProgressValue = -1;
void CheckPowershellProgressPresence();
void CheckPowerShellProgress(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
/* ************ Globals for powershell ************ */

/* ************ Globals for Node.JS ************ */
bool     gbIsNodeJSProcess = false;
/* ************ Globals for Node.JS ************ */

/* ************ Globals for cygwin/msys ************ */
bool gbIsBashProcess = false;
bool gbIsSshProcess = false;
bool gbIsLessProcess = false;
/* ************ Globals for cygwin/msys ************ */

/* ************ Globals for ViM ************ */
bool gbIsVimProcess = false;
bool gbIsVimAnsi = false;
/* ************ Globals for ViM ************ */

/* ************ Globals for MinTTY ************ */
bool gbIsMinTtyProcess;
/* ************ Globals for ViM ************ */

/* ************ Globals for HIEW32.EXE ************ */
bool gbIsHiewProcess = false;
/* ************ Globals for HIEW32.EXE ************ */

/* ************ Globals for DosBox.EXE ************ */
bool gbDosBoxProcess = false;
/* ************ Globals for DosBox.EXE ************ */

/* ************ Don't show VirtualAlloc errors ************ */
bool gbSkipVirtualAllocErr = false;
/* ************ Don't show VirtualAlloc errors ************ */

/* ************ Hooking time functions ************ */
DWORD gnTimeEnvVarLastCheck = 0;
wchar_t gszTimeEnvVarSave[32] = L"";
/* ************ Hooking time functions ************ */


struct ReadConsoleInfo gReadConsoleInfo = {};

int WINAPI OnCompareStringW(LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2);

// Только для cmd.exe (clink)
LONG WINAPI OnRegQueryValueExW(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);


// Service functions
//typedef DWORD (WINAPI* GetProcessId_t)(HANDLE Process);
GetProcessId_t gfGetProcessId = NULL;


// Forward declarations of the hooks
// Common hooks (except LoadLibrary..., FreeLibrary, GetProcAddress)
BOOL WINAPI OnTrackPopupMenu(HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, CONST RECT * prcRect);
BOOL WINAPI OnTrackPopupMenuEx(HMENU hmenu, UINT fuFlags, int x, int y, HWND hWnd, LPTPMPARAMS lptpm);
BOOL WINAPI OnShellExecuteExA(LPSHELLEXECUTEINFOA lpExecInfo);
BOOL WINAPI OnShellExecuteExW(LPSHELLEXECUTEINFOW lpExecInfo);
HRESULT WINAPI OnShellExecCmdLine(HWND hwnd, LPCWSTR pwszCommand, LPCWSTR pwszStartDir, int nShow, LPVOID pUnused, DWORD dwSeclFlags);
HINSTANCE WINAPI OnShellExecuteA(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd);
HINSTANCE WINAPI OnShellExecuteW(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd);
BOOL WINAPI OnFlashWindow(HWND hWnd, BOOL bInvert);
BOOL WINAPI OnFlashWindowEx(PFLASHWINFO pfwi);
BOOL WINAPI OnSetForegroundWindow(HWND hWnd);
HWND WINAPI OnGetForegroundWindow();
int WINAPI OnCompareStringW(LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2);
DWORD WINAPI OnGetConsoleAliasesW(LPWSTR AliasBuffer, DWORD AliasBufferLength, LPWSTR ExeName);
BOOL WINAPI OnAllocConsole(void);
BOOL WINAPI OnFreeConsole(void);
HWND WINAPI OnGetConsoleWindow();
BOOL WINAPI OnGetConsoleMode(HANDLE hConsoleHandle,LPDWORD lpMode);
BOOL WINAPI OnGetWindowRect(HWND hWnd, LPRECT lpRect);
BOOL WINAPI OnScreenToClient(HWND hWnd, LPPOINT lpPoint);
BOOL WINAPI OnCreateProcessA(LPCSTR lpApplicationName,  LPSTR lpCommandLine,  LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory,  LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
BOOL WINAPI OnCreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
UINT WINAPI OnWinExec(LPCSTR lpCmdLine, UINT uCmdShow);
BOOL WINAPI OnSetCurrentDirectoryA(LPCSTR lpPathName);
BOOL WINAPI OnSetCurrentDirectoryW(LPCWSTR lpPathName);
VOID WINAPI OnExitProcess(UINT uExitCode);
BOOL WINAPI OnTerminateProcess(HANDLE hProcess, UINT uExitCode);
BOOL WINAPI OnTerminateThread(HANDLE hThread, DWORD dwExitCode);
DWORD WINAPI OnResumeThread(HANDLE hThread);







extern HANDLE ghSkipSetThreadContextForThread; // Injects.cpp
BOOL WINAPI OnSetThreadContext(HANDLE hThread, CONST CONTEXT *lpContext);

BOOL WINAPI OnSetConsoleCP(UINT wCodePageID);
BOOL WINAPI OnSetConsoleOutputCP(UINT wCodePageID);
#ifdef _DEBUG
LPVOID WINAPI OnVirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
BOOL WINAPI OnVirtualProtect(LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect);
LPTOP_LEVEL_EXCEPTION_FILTER WINAPI OnSetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter);
#endif
HANDLE WINAPI OnCreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);
//HWND WINAPI OnCreateWindowA(LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
//HWND WINAPI OnCreateWindowW(LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
HWND WINAPI OnCreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
HWND WINAPI OnCreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
HWND WINAPI OnSetFocus(HWND hWnd);
BOOL WINAPI OnShowWindow(HWND hWnd, int nCmdShow);
BOOL WINAPI OnShowCursor(BOOL bShow);
HWND WINAPI OnSetParent(HWND hWndChild, HWND hWndNewParent);
HWND WINAPI OnGetParent(HWND hWnd);
HWND WINAPI OnGetWindow(HWND hWnd, UINT uCmd);
HWND WINAPI OnGetAncestor(HWND hwnd, UINT gaFlags);
int WINAPI OnGetClassNameA(HWND hWnd, LPSTR lpClassName, int nMaxCount);
int WINAPI OnGetClassNameW(HWND hWnd, LPWSTR lpClassName, int nMaxCount);
BOOL WINAPI OnMoveWindow(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint);
BOOL WINAPI OnSetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);
LONG WINAPI OnSetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong);
LONG WINAPI OnSetWindowLongW(HWND hWnd, int nIndex, LONG dwNewLong);
#ifdef WIN64
LONG_PTR WINAPI OnSetWindowLongPtrA(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
LONG_PTR WINAPI OnSetWindowLongPtrW(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
#endif
int WINAPI OnGetWindowTextLengthA(HWND hWnd);
int WINAPI OnGetWindowTextLengthW(HWND hWnd);
int WINAPI OnGetWindowTextA(HWND hWnd, LPSTR lpString, int nMaxCount);
int WINAPI OnGetWindowTextW(HWND hWnd, LPWSTR lpString, int nMaxCount);
BOOL WINAPI OnSetConsoleTitleA(LPCSTR lpConsoleTitle);
BOOL WINAPI OnSetConsoleTitleW(LPCWSTR lpConsoleTitle);
BOOL WINAPI OnGetWindowPlacement(HWND hWnd, WINDOWPLACEMENT *lpwndpl);
BOOL WINAPI OnSetWindowPlacement(HWND hWnd, WINDOWPLACEMENT *lpwndpl);
VOID WINAPI Onmouse_event(DWORD dwFlags, DWORD dx, DWORD dy, DWORD dwData, ULONG_PTR dwExtraInfo);
UINT WINAPI OnSendInput(UINT nInputs, LPINPUT pInputs, int cbSize);
BOOL WINAPI OnPostMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL WINAPI OnPostMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI OnSendMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI OnSendMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL WINAPI OnGetMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
BOOL WINAPI OnGetMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
BOOL WINAPI OnPeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
BOOL WINAPI OnPeekMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
BOOL WINAPI OnSetConsoleKeyShortcuts(BOOL bSet, BYTE bReserveKeys, LPVOID p1, DWORD n1);
HWND WINAPI OnCreateDialogIndirectParamA(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lParamInit);
HWND WINAPI OnCreateDialogIndirectParamW(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lParamInit);
HWND WINAPI OnCreateDialogParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
HWND WINAPI OnCreateDialogParamW(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
BOOL WINAPI OnGetCurrentConsoleFont(HANDLE hConsoleOutput, BOOL bMaximumWindow, PCONSOLE_FONT_INFO lpConsoleCurrentFont);
COORD WINAPI OnGetConsoleFontSize(HANDLE hConsoleOutput, DWORD nFont);
HWND WINAPI OnGetActiveWindow();
BOOL WINAPI OnSetMenu(HWND hWnd, HMENU hMenu);
//BOOL WINAPI OnSetConsoleMode(HANDLE hConsoleHandle, DWORD dwMode);
HANDLE WINAPI OnCreateConsoleScreenBuffer(DWORD dwDesiredAccess, DWORD dwShareMode, const SECURITY_ATTRIBUTES *lpSecurityAttributes, DWORD dwFlags, LPVOID lpScreenBufferData);
BOOL WINAPI OnSetConsoleActiveScreenBuffer(HANDLE hConsoleOutput);
BOOL WINAPI OnSetConsoleWindowInfo(HANDLE hConsoleOutput, BOOL bAbsolute, const SMALL_RECT *lpConsoleWindow);
BOOL WINAPI OnSetConsoleScreenBufferSize(HANDLE hConsoleOutput, COORD dwSize);
// WARNING!!! This function exist in Vista and higher OS only!!!
BOOL WINAPI OnSetCurrentConsoleFontEx(HANDLE hConsoleOutput, BOOL bMaximumWindow, MY_CONSOLE_FONT_INFOEX* lpConsoleCurrentFontEx);
// WARNING!!! This function exist in Vista and higher OS only!!!
BOOL WINAPI OnSetConsoleScreenBufferInfoEx(HANDLE hConsoleOutput, MY_CONSOLE_SCREEN_BUFFER_INFOEX* lpConsoleScreenBufferInfoEx);
COORD WINAPI OnGetLargestConsoleWindowSize(HANDLE hConsoleOutput);
BOOL WINAPI OnSetConsoleCursorPosition(HANDLE hConsoleOutput, COORD dwCursorPosition);
BOOL WINAPI OnSetConsoleCursorInfo(HANDLE hConsoleOutput, const CONSOLE_CURSOR_INFO *lpConsoleCursorInfo);
INT_PTR WINAPI OnDialogBoxParamW(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
HDC WINAPI OnGetDC(HWND hWnd); // user32
HDC WINAPI OnGetDCEx(HWND hWnd, HRGN hrgnClip, DWORD flags); // user32
int WINAPI OnReleaseDC(HWND hWnd, HDC hDC); //user32
int WINAPI OnStretchDIBits(HDC hdc, int XDest, int YDest, int nDestWidth, int nDestHeight, int XSrc, int YSrc, int nSrcWidth, int nSrcHeight, const VOID *lpBits, const BITMAPINFO *lpBitsInfo, UINT iUsage, DWORD dwRop); //gdi32
BOOL WINAPI OnBitBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, DWORD dwRop);
BOOL WINAPI OnStretchBlt(HDC hdcDest, int nXOriginDest, int nYOriginDest, int nWidthDest, int nHeightDest, HDC hdcSrc, int nXOriginSrc, int nYOriginSrc, int nWidthSrc, int nHeightSrc, DWORD dwRop);

BOOL WINAPI OnSetEnvironmentVariableA(LPCSTR lpName, LPCSTR lpValue);
BOOL WINAPI OnSetEnvironmentVariableW(LPCWSTR lpName, LPCWSTR lpValue);
DWORD WINAPI OnGetEnvironmentVariableA(LPCSTR lpName, LPSTR lpBuffer, DWORD nSize);
DWORD WINAPI OnGetEnvironmentVariableW(LPCWSTR lpName, LPWSTR lpBuffer, DWORD nSize);
#if 0
LPCH WINAPI OnGetEnvironmentStringsA();
#endif
LPWCH WINAPI OnGetEnvironmentStringsW();

void WINAPI OnGetSystemTime(LPSYSTEMTIME lpSystemTime);
void WINAPI OnGetLocalTime(LPSYSTEMTIME lpSystemTime);
void WINAPI OnGetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime);



bool InitHooksCommon()
{
#ifndef HOOKS_SKIP_COMMON
	// Основные хуки
	HookItem HooksCommon[] =
	{
		/* ***** MOST CALLED ***** */
		#ifndef HOOKS_COMMON_PROCESS_ONLY
		{(void*)OnGetConsoleWindow,     "GetConsoleWindow",     kernel32},
		{(void*)OnGetConsoleMode,		"GetConsoleMode",		kernel32},
		//{(void*)OnWriteConsoleOutputWx,	"WriteConsoleOutputW",  kernel32},
		//{(void*)OnWriteConsoleOutputAx,	"WriteConsoleOutputA",  kernel32},
		{(void*)OnWriteConsoleOutputW,	"WriteConsoleOutputW",  kernel32},
		{(void*)OnWriteConsoleOutputA,	"WriteConsoleOutputA",  kernel32},
		//{(void*)OnPeekConsoleInputWx,	"PeekConsoleInputW",	kernel32},
		//{(void*)OnPeekConsoleInputAx,	"PeekConsoleInputA",	kernel32},
		//{(void*)OnReadConsoleInputWx,	"ReadConsoleInputW",	kernel32},
		//{(void*)OnReadConsoleInputAx,	"ReadConsoleInputA",	kernel32},
		{(void*)OnReadConsoleW,			"ReadConsoleW",			kernel32},
		{(void*)OnReadConsoleA,			"ReadConsoleA",			kernel32},
		{(void*)OnReadFile,				"ReadFile",				kernel32},
		{(void*)OnPeekConsoleInputW,	"PeekConsoleInputW",	kernel32},
		{(void*)OnPeekConsoleInputA,	"PeekConsoleInputA",	kernel32},
		{(void*)OnReadConsoleInputW,	"ReadConsoleInputW",	kernel32},
		{(void*)OnReadConsoleInputA,	"ReadConsoleInputA",	kernel32},
		{(void*)OnWriteConsoleInputA,	"WriteConsoleInputA",	kernel32},
		{(void*)OnWriteConsoleInputW,	"WriteConsoleInputW",	kernel32},
		/* ANSI Escape Sequences SUPPORT */
		//#ifdef HOOK_ANSI_SEQUENCES
		{(void*)OnCreateFileW,			"CreateFileW",  		kernel32},
		{(void*)OnWriteFile,			"WriteFile",  			kernel32},
		{(void*)OnWriteConsoleA,		"WriteConsoleA",  		kernel32},
		{(void*)OnWriteConsoleW,		"WriteConsoleW",  		kernel32},
		{(void*)OnScrollConsoleScreenBufferA,
										"ScrollConsoleScreenBufferA",
																kernel32},
		{(void*)OnScrollConsoleScreenBufferW,
										"ScrollConsoleScreenBufferW",
																kernel32},
		{(void*)OnWriteConsoleOutputCharacterA,
										"WriteConsoleOutputCharacterA",
																kernel32},
		{(void*)OnWriteConsoleOutputCharacterW,
										"WriteConsoleOutputCharacterW",
																kernel32},
		{(void*)OnSetConsoleMode,		"SetConsoleMode",  		kernel32},
		{(void*)OnSetConsoleTitleA,		"SetConsoleTitleA",		kernel32},
		{(void*)OnSetConsoleTitleW,		"SetConsoleTitleW",		kernel32},
		//#endif
		/* Others console functions */
		{(void*)OnSetConsoleTextAttribute, "SetConsoleTextAttribute", kernel32},
		{(void*)OnSetConsoleKeyShortcuts, "SetConsoleKeyShortcuts", kernel32},
		#endif
		/* ************************ */
		{(void*)OnExitProcess,			"ExitProcess",			kernel32},
		{(void*)OnTerminateProcess,		"TerminateProcess",		kernel32},
		{(void*)OnTerminateThread,		"TerminateThread",		kernel32},
		/* ************************ */
		{(void*)OnCreateProcessA,		"CreateProcessA",		kernel32},
		{(void*)OnCreateProcessW,		"CreateProcessW",		kernel32},
		{(void*)OnOpenFileMappingW,		"OpenFileMappingW",		kernel32},
		{(void*)OnMapViewOfFile,		"MapViewOfFile",		kernel32},
		{(void*)OnUnmapViewOfFile,		"UnmapViewOfFile",		kernel32},
		{(void*)OnCloseHandle,			"CloseHandle",			kernel32},
		{(void*)OnSetThreadContext,		"SetThreadContext",		kernel32},
		{(void*)OnSetCurrentDirectoryA, "SetCurrentDirectoryA", kernel32},
		{(void*)OnSetCurrentDirectoryW, "SetCurrentDirectoryW", kernel32},
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
			(void*)OnFlushConsoleInputBuffer,
			"FlushConsoleInputBuffer",
			kernel32
		},
		{
			(void*)OnCreateConsoleScreenBuffer,
			"CreateConsoleScreenBuffer",
			kernel32
		},
		{
			(void*)OnSetConsoleActiveScreenBuffer,
			"SetConsoleActiveScreenBuffer",
			kernel32
		},
		{
			(void*)OnSetConsoleWindowInfo,
			"SetConsoleWindowInfo",
			kernel32
		},
		{
			(void*)OnSetConsoleScreenBufferSize,
			"SetConsoleScreenBufferSize",
			kernel32
		},
		{
			(void*)OnSetCurrentConsoleFontEx,
			"SetCurrentConsoleFontEx",
			kernel32
		},
		{
			(void*)OnSetConsoleScreenBufferInfoEx,
			"SetConsoleScreenBufferInfoEx",
			kernel32
		},
		{
			(void*)OnGetLargestConsoleWindowSize,
			"GetLargestConsoleWindowSize",
			kernel32
		},
		{
			(void*)OnSetConsoleCursorPosition,
			"SetConsoleCursorPosition",
			kernel32
		},
		{
			(void*)OnSetConsoleCursorInfo,
			"SetConsoleCursorInfo",
			kernel32
		},
		#endif
		{(void*)OnSetEnvironmentVariableA,	"SetEnvironmentVariableA",	kernel32},
		{(void*)OnSetEnvironmentVariableW,	"SetEnvironmentVariableW",	kernel32},
		{(void*)OnGetEnvironmentVariableA,	"GetEnvironmentVariableA",	kernel32},
		{(void*)OnGetEnvironmentVariableW,	"GetEnvironmentVariableW",	kernel32},
		#if 0
		{(void*)OnGetEnvironmentStringsA,  "GetEnvironmentStringsA",	kernel32},
		#endif
		{(void*)OnGetEnvironmentStringsW,	"GetEnvironmentStringsW",	kernel32},
		/* ************************ */
		{(void*)OnGetSystemTime,			"GetSystemTime",			kernel32},
		{(void*)OnGetLocalTime,				"GetLocalTime",				kernel32},
		{(void*)OnGetSystemTimeAsFileTime,	"GetSystemTimeAsFileTime",	kernel32},
		/* ************************ */
		{(void*)OnGetCurrentConsoleFont, "GetCurrentConsoleFont",		kernel32},
		{(void*)OnGetConsoleFontSize,    "GetConsoleFontSize",			kernel32},
		/* ************************ */

		#ifdef _DEBUG
		#ifndef HOOKS_COMMON_PROCESS_ONLY
		{(void*)OnCreateNamedPipeW,		"CreateNamedPipeW",		kernel32},
		#endif
		#endif

		#ifdef _DEBUG
		//#ifdef HOOKS_VIRTUAL_ALLOC
		{(void*)OnVirtualAlloc,			"VirtualAlloc",			kernel32},
		{(void*)OnVirtualProtect,		"VirtualProtect",		kernel32},
		{(void*)OnSetUnhandledExceptionFilter,
										"SetUnhandledExceptionFilter",
																kernel32},
		//#endif
		#endif
		{(void*)OnCreateThread,			"CreateThread",			kernel32},

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

	#if 0
	// Проверка, как реагирует на дубли
	HooksCommon[1].NewAddress = NULL;
	_ASSERTEX(FALSE && "Testing");
	InitHooks(HooksCommon);
	#endif

#endif
	return true;
}

bool InitHooksDefTerm()
{
	// Хуки требующиеся для установки ConEmu как терминала по умолчанию
	HookItem HooksCommon[] =
	{
		{(void*)OnCreateProcessW,		"CreateProcessW",		kernel32},
		// Need to "hook" OnCreateProcessA because it is used in "OnWinExec"
		{(void*)OnCreateProcessA,		"CreateProcessA",		kernel32},
		// Used in some programs, Issue 853
		{(void*)OnWinExec,				"WinExec",				kernel32},
		// Need for hook "Run as administrator"
		{(void*)OnShellExecuteExW,		"ShellExecuteExW",		shell32},
		{0}
	};
	HookItem HooksCmdLine[] =
	{
		// Issue 1125: "Run as administrator" too. Must be last export
		{(void*)OnShellExecCmdLine,		"ShellExecCmdLine",		shell32,   265},
		{0}
	};
	HookItem HooksVshost[] =
	{
		// Issue 1312: .Net applications runs in "*.vshost.exe" helper GUI apllication when debugging
		{(void*)OnAllocConsole,			"AllocConsole",			kernel32}, // Only for "*.vshost.exe"?
		{(void*)OnShowWindow,			"ShowWindow",			user32},
		/* ************************ */
		{0}
	};
	// Required in VisualStudio and CodeBlocks (gdb) debuggers
	// Don't restrict to them, other Dev Envs may behave in similar way
	HookItem HooksDevStudio[] =
	{
		{(void*)OnResumeThread,			"ResumeThread",			kernel32},
		/* ************************ */
		{0}
	};

	InitHooks(HooksCommon);

	// Windows 7. There is new undocumented function "ShellExecCmdLine" used by Explorer
	_ASSERTE(_WIN32_WINNT_WIN7==0x601);
	OSVERSIONINFOEXW osvi = {sizeof(osvi), HIBYTE(_WIN32_WINNT_WIN7), LOBYTE(_WIN32_WINNT_WIN7)};
	DWORDLONG const dwlConditionMask = VerSetConditionMask(VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_MINORVERSION, VER_GREATER_EQUAL);
	BOOL isWindows7 = VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask);
	if (isWindows7)
	{
		InitHooks(HooksCmdLine);
	}

	// "*.vshost.exe" uses special helper
	if (gbIsNetVsHost)
	{
		InitHooks(HooksVshost);
	}

	// Required in VisualStudio and CodeBlocks (gdb) debuggers
	// Don't restrict to them, other Dev Envs may behave in similar way
	{
		InitHooks(HooksDevStudio);
	}

	return true;
}

// user32 & gdi32
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
		{(void*)OnGetForegroundWindow,	"GetForegroundWindow",	user32},
		{(void*)OnGetWindowRect,		"GetWindowRect",		user32},
		{(void*)OnScreenToClient,		"ScreenToClient",		user32},
		#endif
		/* ************************ */
		//{(void*)OnCreateWindowA,		"CreateWindowA",		user32}, -- таких экспортов нет
		//{(void*)OnCreateWindowW,		"CreateWindowW",		user32}, -- таких экспортов нет
		{(void*)OnCreateWindowExA,		"CreateWindowExA",		user32},
		{(void*)OnCreateWindowExW,		"CreateWindowExW",		user32},
		{(void*)OnShowCursor,			"ShowCursor",			user32},
		{(void*)OnShowWindow,			"ShowWindow",			user32},
		{(void*)OnSetFocus,				"SetFocus",				user32},
		{(void*)OnSetParent,			"SetParent",			user32},
		{(void*)OnGetParent,			"GetParent",			user32},
		{(void*)OnGetWindow,			"GetWindow",			user32},
		{(void*)OnGetAncestor,			"GetAncestor",			user32},
		{(void*)OnGetClassNameA,		"GetClassNameA",		user32},
		{(void*)OnGetClassNameW,		"GetClassNameW",		user32},
		{(void*)OnGetActiveWindow,		"GetActiveWindow",		user32},
		{(void*)OnMoveWindow,			"MoveWindow",			user32},
		{(void*)OnSetWindowPos,			"SetWindowPos",			user32},
		{(void*)OnSetWindowLongA,		"SetWindowLongA",		user32},
		{(void*)OnSetWindowLongW,		"SetWindowLongW",		user32},
		#ifdef WIN64
		{(void*)OnSetWindowLongPtrA,	"SetWindowLongPtrA",	user32},
		{(void*)OnSetWindowLongPtrW,	"SetWindowLongPtrW",	user32},
		#endif
		{(void*)OnGetWindowTextLengthA,	"GetWindowTextLengthA",	user32},
		{(void*)OnGetWindowTextLengthW,	"GetWindowTextLengthW",	user32},
		{(void*)OnGetWindowTextA,		"GetWindowTextA",		user32},
		{(void*)OnGetWindowTextW,		"GetWindowTextW",		user32},
		//
		{(void*)OnGetWindowPlacement,	"GetWindowPlacement",	user32},
		{(void*)OnSetWindowPlacement,	"SetWindowPlacement",	user32},
		{(void*)Onmouse_event,			"mouse_event",			user32},
		{(void*)OnSendInput,			"SendInput",			user32},
		{(void*)OnPostMessageA,			"PostMessageA",			user32},
		{(void*)OnPostMessageW,			"PostMessageW",			user32},
		{(void*)OnSendMessageA,			"SendMessageA",			user32},
		{(void*)OnSendMessageW,			"SendMessageW",			user32},
		{(void*)OnGetMessageA,			"GetMessageA",			user32},
		{(void*)OnGetMessageW,			"GetMessageW",			user32},
		{(void*)OnPeekMessageA,			"PeekMessageA",			user32},
		{(void*)OnPeekMessageW,			"PeekMessageW",			user32},
		{(void*)OnCreateDialogParamA,	"CreateDialogParamA",	user32},
		{(void*)OnCreateDialogParamW,	"CreateDialogParamW",	user32},
		{(void*)OnCreateDialogIndirectParamA, "CreateDialogIndirectParamA", user32},
		{(void*)OnCreateDialogIndirectParamW, "CreateDialogIndirectParamW", user32},
		{(void*)OnDialogBoxParamW,		"DialogBoxParamW",		user32},
		{(void*)OnSetMenu,				"SetMenu",				user32},
		{(void*)OnGetDC,				"GetDC",				user32},
		{(void*)OnGetDCEx,				"GetDCEx",				user32},
		{(void*)OnReleaseDC,			"ReleaseDC",			user32},
		/* ************************ */

		/* ************************ */
		{(void*)OnStretchDIBits,		"StretchDIBits",		gdi32},
		{(void*)OnBitBlt,				"BitBlt",				gdi32},
		{(void*)OnStretchBlt,			"StretchBlt",			gdi32},
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
			if (IsFarExe(pszExe))
				lbIsFar = true;
		}
		free(pszExe);
	}

	if (!lbIsFar)
		return true; // Don't hook

	HookItem HooksFarOnly[] =
	{
		{(void*)OnCompareStringW, "CompareStringW", kernel32},
		/* ************************ */
		{0, 0, 0}
	};
	InitHooks(HooksFarOnly);
#endif
	return true;
}

bool InitHooksClink()
{
	//if (!gnAllowClinkUsage)
	if (!gbIsCmdProcess)
		return true;

	HookItem HooksCmdOnly[] =
	{
		{(void*)OnRegQueryValueExW, "RegQueryValueExW", kernel32},
		{0, 0, 0}
	};

	// Для Vista и ниже - AdvApi32.dll
	// Причем, в WinXP этот модуль не прилинкован статически
	_ASSERTE(_WIN32_WINNT_VISTA==0x600);
	OSVERSIONINFOEXW osvi = {sizeof(osvi), HIBYTE(_WIN32_WINNT_VISTA), LOBYTE(_WIN32_WINNT_VISTA)};
	DWORDLONG const dwlConditionMask = VerSetConditionMask(VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_MINORVERSION, VER_GREATER_EQUAL);
	if (!VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask))
	{
		HooksCmdOnly[0].DllName = advapi32;
	}

	InitHooks(HooksCmdOnly);

	return true;
}


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
		// При запуске Ctrl+Shift+W - фар как-то криво инитится... Ctrl+O не работает
		#ifdef _DEBUG
		char szInfo[100]; msprintf(szInfo, countof(szInfo), "SetFarHookMode. FarHookMode=%u, LongConsoleOutput=%u\n", apFarMode->bFarHookMode, apFarMode->bLongConsoleOutput);
		OutputDebugStringA(szInfo);
		#endif
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
BOOL StartupHooks()
{
	//HLOG0("StartupHooks",0);
#ifdef _DEBUG
	// Консольное окно уже должно быть инициализировано в DllMain
	_ASSERTE(gbAttachGuiClient || gbDosBoxProcess || gbPrepareDefaultTerminal || (ghConWnd != NULL && ghConWnd == GetRealConsoleWindow()));
	wchar_t sClass[128];
	if (ghConWnd)
	{
		GetClassName(ghConWnd, sClass, countof(sClass));
		_ASSERTE(isConsoleClass(sClass));
	}

	wchar_t szTimingMsg[512]; HANDLE hTimingHandle = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

	// -- ghConEmuWnd уже должен быть установлен в DllMain!!!
	//gbInShellExecuteEx = FALSE;

	WARNING("Получить из мэппинга gdwServerPID");

	//TODO: Change GetModuleHandle to GetModuleHandleEx? Does it exist in Win2k?

	// Зовем LoadLibrary. Kernel-то должен был сразу загрузиться (static link) в любой
	// windows приложении, но вот shell32 - не обязательно, а нам нужно хуки проинициализировать
	ghKernel32 = LoadLibrary(kernel32);
	// user32/shell32/advapi32 тянут за собой много других библиотек, НЕ загружаем, если они еще не подлинкованы
	if (!ghUser32)
	{
		ghUser32 = GetModuleHandle(user32);
		if (ghUser32) ghUser32 = LoadLibrary(user32); // если подлинкован - увеличить счетчик
	}
	ghShell32 = GetModuleHandle(shell32);
	if (ghShell32) ghShell32 = LoadLibrary(shell32); // если подлинкован - увеличить счетчик
	ghAdvapi32 = GetModuleHandle(advapi32);
	if (ghAdvapi32) ghAdvapi32 = LoadLibrary(advapi32); // если подлинкован - увеличить счетчик
	ghComdlg32 = GetModuleHandle(comdlg32);
	if (ghComdlg32) ghComdlg32 = LoadLibrary(comdlg32); // если подлинкован - увеличить счетчик

	if (ghKernel32)
		gfGetProcessId = (GetProcessId_t)GetProcAddress(ghKernel32, "GetProcessId");

	if (gbPrepareDefaultTerminal)
	{
		HLOG1("StartupHooks.InitHooks",0);
		InitHooksDefTerm();
		HLOGEND1();
	}
	else
	{
		// Общие
		HLOG1("StartupHooks.InitHooks",0);
		InitHooksCommon();
		HLOGEND1();

		// user32 & gdi32
		HLOG1_("StartupHooks.InitHooks",1);
		InitHooksUser32();
		HLOGEND1();

		// Far only functions
		HLOG1_("StartupHooks.InitHooks",2);
		InitHooksFar();
		HLOGEND1();

		// Cmd.exe only functions
		//if (gnAllowClinkUsage)
		if (gbIsCmdProcess)
		{
			HLOG1_("StartupHooks.InitHooks",3);
			InitHooksClink();
			HLOGEND1();
		}

		// Реестр
		HLOG1_("StartupHooks.InitHooks",4);
		InitHooksReg();
		HLOGEND1();
	}

#if 0
	HLOG1_("InitHooksSort",0);
	InitHooksSort();
	HLOGEND1();
#endif

	print_timings(L"SetAllHooks");

	// Теперь можно обработать модули
	HLOG1("SetAllHooks",0);
	bool lbRc = SetAllHooks();
	HLOGEND1();

	extern FARPROC CallWriteConsoleW;
	CallWriteConsoleW = (FARPROC)GetOriginalAddress((LPVOID)OnWriteConsoleW, NULL);

	extern GetConsoleWindow_T gfGetRealConsoleWindow; // from ConEmuCheck.cpp
	gfGetRealConsoleWindow = (GetConsoleWindow_T)GetOriginalAddress((LPVOID)OnGetConsoleWindow, NULL);

	print_timings(L"SetAllHooks - done");

	//HLOGEND(); // StartupHooks - done

	return lbRc;
}





// Forward
BOOL GuiSetForeground(HWND hWnd);
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






// Called from OnShellExecCmdLine
HRESULT OurShellExecCmdLine(HWND hwnd, LPCWSTR pwszCommand, LPCWSTR pwszStartDir, bool bRunAsAdmin, bool bForce)
{
	HRESULT hr = E_UNEXPECTED;
	BOOL bShell = FALSE;

	CEStr lsLog = lstrmerge(L"OnShellExecCmdLine", bRunAsAdmin ? L"(RunAs): " : L": ", pwszCommand);
	DefTermLogString(lsLog);

	// Bad thing, ShellExecuteEx needs File&Parm, but we get both in pwszCommand
	CmdArg szExe;
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




void FixHwnd4ConText(HWND& hWnd)
{
	if (ghConEmuWndDC && (hWnd == ghConEmuWndDC || hWnd == ghConEmuWnd))
		hWnd = ghConWnd;
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
			case WM_SYSCOMMAND:
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
			case WM_GETTEXT:
			case WM_GETTEXTLENGTH:
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

void PatchGuiMessage(bool bReceived, HWND& hWnd, UINT& Msg, WPARAM& wParam, LPARAM& lParam)
{
	if (!ghAttachGuiClient)
		return;

	switch (Msg)
	{
	case WM_MOUSEACTIVATE:
		if (wParam == (WPARAM)ghConEmuWnd)
		{
			wParam = (WPARAM)ghAttachGuiClient;
		}
		break;

	case WM_LBUTTONDOWN:
		if (bReceived && ghConEmuWnd && ghConWnd)
		{
			HWND hActiveCon = (HWND)GetWindowLongPtr(ghConEmuWnd, GWLP_USERDATA);
			if (hActiveCon != ghConWnd)
			{
				CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_ACTIVATECON, sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_ACTIVATECONSOLE));
				if (pIn)
				{
					pIn->ActivateCon.hConWnd = ghConWnd;
					CESERVER_REQ* pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
					ExecuteFreeResult(pIn);
					ExecuteFreeResult(pOut);
				}
			}
			#if 0
			// Если окно ChildGui активируется кликом - то ConEmu (holder)
			// может остаться "под" предыдущим окном бывшим в фокусе...
			// Победить пока не получается.
			DWORD nTID = 0, nPID = 0;
			HWND hFore = GetForegroundWindow();
			if (hFore && ((nTID = GetWindowThreadProcessId(hFore, &nPID)) != 0))
			{
				if ((nPID != GetCurrentProcessId()) && (nPID != gnGuiPID))
				{
					SetForegroundWindow(ghConEmuWnd);
				}
			}
			#endif
		}
		break;

	#ifdef _DEBUG
	case WM_DESTROY:
		if (hWnd == ghAttachGuiClient)
		{
			int iDbg = -1;
		}
		break;
	#endif
	}
}





#ifndef NORM_STOP_ON_NULL
#define NORM_STOP_ON_NULL 0x10000000
#endif

// ANSI хукать смысла нет, т.к. FAR 1.x сравнивает сам
int WINAPI OnCompareStringW(LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2)
{
	typedef int (WINAPI* OnCompareStringW_t)(LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2);
	SUPPRESSORIGINALSHOWCALL;
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





bool IsPromptActionAllowed(bool bForce, bool bBashMargin, HANDLE* phConIn)
{
	if (!gReadConsoleInfo.InReadConsoleTID && !gReadConsoleInfo.LastReadConsoleInputTID)
		return false;

	DWORD nConInMode = 0;

	HANDLE hConIn = gReadConsoleInfo.InReadConsoleTID ? gReadConsoleInfo.hConsoleInput : gReadConsoleInfo.hConsoleInput2;
	if (!hConIn)
		return false;

	if (!gReadConsoleInfo.InReadConsoleTID && gReadConsoleInfo.LastReadConsoleInputTID)
	{
		// Проверить, может программа мышь сама обрабатывает?
		if (!GetConsoleMode(hConIn, &nConInMode))
			return false;
		if (!bForce && ((nConInMode & ENABLE_MOUSE_INPUT) == ENABLE_MOUSE_INPUT))
			return false; // Разрешить обрабатывать самой программе
	}

	if (phConIn)
		*phConIn = hConIn;

	return true;
}

BOOL OnPromptBsDeleteWord(bool bForce, bool bBashMargin)
{
	HANDLE hConIn = NULL;
	if (!IsPromptActionAllowed(bForce, bBashMargin, &hConIn))
	{
		BsDelWordMsg(L"Skipped due to !IsPromptActionAllowed!");
		return FALSE;
	}

	int iBSCount = 0;
	BOOL lbWrite = FALSE;
	DWORD dwLastError = 0;

	HANDLE hConOut = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	if (GetConsoleScreenBufferInfo(hConOut, &csbi) && csbi.dwSize.X && csbi.dwSize.Y)
	{
		if (csbi.dwCursorPosition.X == 0)
		{
			iBSCount = 1;
		}
		else
		{
			bool bDBCS = false;
			DWORD nRead = 0;
			BOOL bReadOk = FALSE;

			// Если в консоли выбрана DBCS кодировка - там все не просто
			DWORD nCP = GetConsoleOutputCP();
			if (nCP && nCP != CP_UTF7 && nCP != CP_UTF8 && nCP != 1200 && nCP != 1201)
			{
				CPINFO cp = {};
				if (GetCPInfo(nCP, &cp) && (cp.MaxCharSize > 1))
				{
					bDBCS = true;
				}
			}

			#ifdef _DEBUG
			wchar_t szDbg[120];
			_wsprintf(szDbg, SKIPCOUNT(szDbg) L"CP=%u bDBCS=%u IsDbcs=%u X=%i", nCP, bDBCS, IsDbcs(), csbi.dwCursorPosition.X);
			BsDelWordMsg(szDbg);
			#endif

			int xPos = csbi.dwCursorPosition.X;
			COORD cr = {0, csbi.dwCursorPosition.Y};
			if ((xPos == 0) && (csbi.dwCursorPosition.Y > 0))
			{
				cr.Y--;
				xPos = csbi.dwSize.X;
			}
			COORD cursorFix = {xPos, cr.Y};

			wchar_t* pwszLine = (wchar_t*)malloc((csbi.dwSize.X+1)*sizeof(*pwszLine));


			if (pwszLine)
			{
				pwszLine[csbi.dwSize.X] = 0;

				// Считать строку
				if (bDBCS)
				{
					CHAR_INFO *pData = (CHAR_INFO*)calloc(csbi.dwSize.X, sizeof(CHAR_INFO));
					COORD bufSize = {csbi.dwSize.X, 1};
					SMALL_RECT rgn = {0, cr.Y, csbi.dwSize.X-1, cr.Y};

					bReadOk = ReadConsoleOutputEx(hConOut, pData, bufSize, rgn, &cursorFix);
					dwLastError = GetLastError();
					_ASSERTE(bReadOk);

					if (bReadOk)
					{
						for (int i = 0; i < csbi.dwSize.X; i++)
							pwszLine[i] = pData[i].Char.UnicodeChar;
						nRead = csbi.dwSize.X;
						xPos = cursorFix.X;
					}

					SafeFree(pData);
				}
				else
				{
					bReadOk = ReadConsoleOutputCharacterW(hConOut, pwszLine, csbi.dwSize.X, cr, &nRead);
					if (bReadOk && !nRead)
						bReadOk = FALSE;
				}

				if (bReadOk)
				{
					// Count chars
					{
						if ((int)nRead >= xPos)
						{
							int i = xPos - 1;
							_ASSERTEX(i >= 0);

							iBSCount = 0;

							// Only RIGHT brackets here to be sure that `(x86)` will be deleted including left bracket
							wchar_t cBreaks[] = L"\x20\xA0>])}$.,/\\\"";

							// Delete all `spaces` first
							while ((i >= 0) && ((pwszLine[i] == ucSpace) || (pwszLine[i] == ucNoBreakSpace)))
								iBSCount++, i--;
							_ASSERTE(cBreaks[0]==ucSpace && cBreaks[1]==ucNoBreakSpace);
							// delimiters
							while ((i >= 0) && wcschr(cBreaks+2, pwszLine[i]))
								iBSCount++, i--;
							// and all `NON-spaces`
							while ((i >= 0) && !wcschr(cBreaks, pwszLine[i]))
								iBSCount++, i--;
						}
					}
				}
			}

			// Done, string was processed
			SafeFree(pwszLine);
		}
	}
	else
	{
		BsDelWordMsg(L"GetConsoleScreenBufferInfo failed");
	}

	if (iBSCount > 0)
	{
		INPUT_RECORD* pr = (INPUT_RECORD*)calloc((size_t)iBSCount,sizeof(*pr));
		if (pr != NULL)
		{
			WORD vk = VK_BACK;
			HKL hkl = GetKeyboardLayout(gReadConsoleInfo.InReadConsoleTID ? gReadConsoleInfo.InReadConsoleTID : gReadConsoleInfo.LastReadConsoleInputTID);
			WORD sc = MapVirtualKeyEx(vk, 0/*MAPVK_VK_TO_VSC*/, hkl);
			if (!sc)
			{
				_ASSERTEX(sc!=NULL && "Can't determine SC?");
				sc = 0x0E;
			}

			for (int i = 0; i < iBSCount; i++)
			{
				pr[i].EventType = KEY_EVENT;
				pr[i].Event.KeyEvent.bKeyDown = TRUE;
				pr[i].Event.KeyEvent.wRepeatCount = 1;
				pr[i].Event.KeyEvent.wVirtualKeyCode = vk;
				pr[i].Event.KeyEvent.wVirtualScanCode = sc;
				pr[i].Event.KeyEvent.uChar.UnicodeChar = vk; // BS
				pr[i].Event.KeyEvent.dwControlKeyState = 0;
			}

			DWORD nWritten = 0;

			while (iBSCount > 0)
			{
				lbWrite = WriteConsoleInputW(hConIn, pr, min(iBSCount,256), &nWritten);
				if (!lbWrite || !nWritten)
					break;
				iBSCount -= nWritten;
			}

			free(pr);
		}
	}
	else
	{
		BsDelWordMsg(L"Nothing to delete");
	}

	UNREFERENCED_PARAMETER(dwLastError);
	return FALSE;
}

// bBashMargin - sh.exe has pad in one space cell on right edge of window
BOOL OnReadConsoleClick(SHORT xPos, SHORT yPos, bool bForce, bool bBashMargin)
{
	TODO("Тут бы нужно еще учитывать, что консоль могла прокрутиться вверх на несколько строк, если был ENABLE_WRAP_AT_EOL_OUTPUT");
	TODO("Еще интересно, что будет, если координата начала вдруг окажется за пределами буфера (типа сузили окно, и курсор уехал)");

	HANDLE hConIn = NULL;
	if (!IsPromptActionAllowed(bForce, bBashMargin, &hConIn))
		return FALSE;

	BOOL lbRc = FALSE, lbWrite = FALSE;
    int nChars = 0;
    DWORD nWritten = 0, dwLastError = 0;

	HANDLE hConOut = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	if (GetConsoleScreenBufferInfo(hConOut, &csbi) && csbi.dwSize.X && csbi.dwSize.Y)
	{
		bool bHomeEnd = false;
		lbRc = TRUE;

		nChars = (csbi.dwSize.X * (yPos - csbi.dwCursorPosition.Y))
			+ (xPos - csbi.dwCursorPosition.X);

		if (nChars != 0)
		{
			char* pszLine = (char*)malloc(csbi.dwSize.X+1);
			wchar_t* pwszLine = (wchar_t*)malloc((csbi.dwSize.X+1)*sizeof(*pwszLine));

			if (pszLine && pwszLine)
			{
				int nChecked = 0;
				int iCount;
				DWORD nRead;
				COORD cr;
				SHORT nPrevSpaces = 0, nPrevChars = 0;
				SHORT nWhole = 0, nPrint = 0;
				bool bDBCS = false;
				// Если в консоли выбрана DBCS кодировка - там все не просто
				DWORD nCP = GetConsoleOutputCP();
				if (nCP && nCP != CP_UTF7 && nCP != CP_UTF8 && nCP != 1200 && nCP != 1201)
				{
					CPINFO cp = {};
					if (GetCPInfo(nCP, &cp) && (cp.MaxCharSize > 1))
					{
						bDBCS = true;
					}
				}

				TODO("DBCS!!! Must to convert cursor pos ('DBCS') to char pos!");
				// Ok, теперь нужно проверить, не был ли клик сделан "за пределами строки ввода"

				SHORT y = csbi.dwCursorPosition.Y;
				while (true)
				{
					cr.Y = y;
					if (nChars > 0)
					{
						cr.X = (y == csbi.dwCursorPosition.Y) ? csbi.dwCursorPosition.X : 0;
						iCount = (y == yPos) ? (xPos - cr.X) : (csbi.dwSize.X - cr.X);
						if (iCount < 0)
							break;
					}
					else
					{
						cr.X = 0;
						iCount = ((y == csbi.dwCursorPosition.Y) ? csbi.dwCursorPosition.X : csbi.dwSize.X)
							- ((y == yPos) ? xPos : 0);
						if (iCount < 0)
							break;
					}

					// Считать строку
					if (bDBCS)
					{
						// На DBCS кодировках "ReadConsoleOutputCharacterW" фигню возвращает
						BOOL bReadOk = ReadConsoleOutputCharacterA(hConOut, pszLine, iCount, cr, &nRead);
						dwLastError = GetLastError();

						if (!bReadOk || !nRead)
						{
							// Однако и ReadConsoleOutputCharacterA может глючить, пробуем "W"
							bReadOk = ReadConsoleOutputCharacterW(hConOut, pwszLine, iCount, cr, &nRead);
							dwLastError = GetLastError();

							if (!bReadOk || !nRead)
								break;

							bDBCS = false; // Thread string as simple Unicode.
						}
						else
						{
							nRead = MultiByteToWideChar(nCP, 0, pszLine, nRead, pwszLine, csbi.dwSize.X);
						}

						// Check chars count
						if (((int)nRead) <= 0)
							break;
					}
					else
					{
						if (!ReadConsoleOutputCharacterW(hConOut, pwszLine, iCount, cr, &nRead) || !nRead)
							break;
					}

					if (nRead > (DWORD)csbi.dwSize.X)
					{
						_ASSERTEX(nRead <= (DWORD)csbi.dwSize.X);
						break;
					}
					pwszLine[nRead] = 0;

					nWhole = nPrint = (SHORT)nRead;
					// Сначала посмотреть сколько в конце строки пробелов
					while ((nPrint > 0) && (pwszLine[nPrint-1] == L' '))
					{
						nPrint--;
					}

					// В каком направлении идем
					if (nChars > 0) // Вниз
					{
						// Если знаков (не пробелов) больше 0 - учитываем и концевые пробелы предыдущей строки
						if (nPrint > 0)
						{
							nChecked += nPrevSpaces + nPrint;
						}
						else
						{
							// Если на предыдущей строке значащих символов не было - завершаем
							if (nPrevChars <= 0)
								break;
						}
					}
					else // Вверх
					{
						if (nPrint <= 0)
							break; // На первой же пустой строке мы останавливаемся
						nChecked += nWhole;
					}
					nPrevChars = nPrint;
					nPrevSpaces = nWhole - nPrint;
					_ASSERTEX(nPrevSpaces>=0);


					// Цикл + условие
					if (nChars > 0)
					{
						if ((++y) > yPos)
							break;
					}
					else
					{
						if ((--y) < yPos)
							break;
					}
				}
				SafeFree(pszLine);
				SafeFree(pwszLine);

				// Changed?
				nChars = (nChars > 0) ? nChecked : -nChecked;
				//nChars = (csbi.dwSize.X * (yPos - csbi.dwCursorPosition.Y))
				//	+ (xPos - csbi.dwCursorPosition.X);
			}
		}

		if (nChars != 0)
		{
			int nCount = bHomeEnd ? 1 : (nChars < 0) ? (-nChars) : nChars;
			if (!bHomeEnd && (nCount > (csbi.dwSize.X * (csbi.srWindow.Bottom - csbi.srWindow.Top))))
			{
				bHomeEnd = true;
				nCount = 1;
			}

			INPUT_RECORD* pr = (INPUT_RECORD*)calloc((size_t)nCount,sizeof(*pr));
			if (pr != NULL)
			{
				WORD vk = bHomeEnd ? ((nChars < 0) ? VK_HOME : VK_END) :
					((nChars < 0) ? VK_LEFT : VK_RIGHT);
				HKL hkl = GetKeyboardLayout(gReadConsoleInfo.InReadConsoleTID ? gReadConsoleInfo.InReadConsoleTID : gReadConsoleInfo.LastReadConsoleInputTID);
				WORD sc = MapVirtualKeyEx(vk, 0/*MAPVK_VK_TO_VSC*/, hkl);
				if (!sc)
				{
					_ASSERTEX(sc!=NULL && "Can't determine SC?");
					sc = (vk == VK_LEFT)  ? 0x4B :
						 (vk == VK_RIGHT) ? 0x4D :
						 (vk == VK_HOME)  ? 0x47 :
						 (vk == VK_RIGHT) ? 0x4F : 0;
				}

				for (int i = 0; i < nCount; i++)
				{
					pr[i].EventType = KEY_EVENT;
					pr[i].Event.KeyEvent.bKeyDown = TRUE;
					pr[i].Event.KeyEvent.wRepeatCount = 1;
					pr[i].Event.KeyEvent.wVirtualKeyCode = vk;
					pr[i].Event.KeyEvent.wVirtualScanCode = sc;
					pr[i].Event.KeyEvent.dwControlKeyState = ENHANCED_KEY;
				}

				while (nCount > 0)
				{
					lbWrite = WriteConsoleInputW(hConIn, pr, min(nCount,256), &nWritten);
					if (!lbWrite || !nWritten)
						break;
					nCount -= nWritten;
				}

				free(pr);
			}
		}
	}

	UNREFERENCED_PARAMETER(dwLastError);
	return lbRc;
}

BOOL OnExecutePromptCmd(LPCWSTR asCmd)
{
	if (!gReadConsoleInfo.InReadConsoleTID && !gReadConsoleInfo.LastReadConsoleInputTID)
		return FALSE;

	HANDLE hConIn = gReadConsoleInfo.InReadConsoleTID ? gReadConsoleInfo.hConsoleInput : gReadConsoleInfo.hConsoleInput2;
	if (!hConIn)
		return FALSE;

	BOOL lbRc = FALSE;
	INPUT_RECORD r[256];
	INPUT_RECORD* pr = r;
	INPUT_RECORD* prEnd = r + countof(r);
	LPCWSTR pch = asCmd;
	DWORD nWrite, nWritten;
	BOOL lbWrite;

	while (*pch)
	{
		// Если (\r\n)|(\n) - слать \r
		if ((*pch == L'\r') || (*pch == L'\n'))
		{
			TranslateKeyPress(0, 0, L'\r', -1, pr, pr+1);

			if (*pch == L'\r' && *(pch+1) == L'\n')
				pch += 2;
			else
				pch ++;
		}
		else
		{
			TranslateKeyPress(0, 0, *pch, -1, pr, pr+1);
			pch ++;
		}

		pr += 2;
		if (pr >= prEnd)
		{
			_ASSERTE(pr == prEnd);

			if (pr && (pr > r))
			{
				nWrite = (DWORD)(pr - r);
				lbWrite = WriteConsoleInputW(hConIn, r, nWrite, &nWritten);
				if (!lbWrite)
				{
					pr = NULL;
					lbRc = FALSE;
					break;
				}
				if (*pch) // Чтобы не было переполнения буфера
					Sleep(10);
				lbRc = TRUE;
			}

			pr = r;
		}
	}

	if (pr && (pr > r))
	{
		nWrite = (DWORD)(pr - r);
		lbRc = WriteConsoleInputW(hConIn, r, nWrite, &nWritten);
	}

	return lbRc;
}











AttachConsole_t GetAttachConsoleProc()
{
	static HMODULE hKernel = GetModuleHandle(L"kernel32.dll");
	static AttachConsole_t _AttachConsole = hKernel ? (AttachConsole_t)GetProcAddress(hKernel, "AttachConsole") : NULL;
	return _AttachConsole;
}

bool AttachServerConsole()
{
	bool lbAttachRc = false;
	DWORD nErrCode;
	HWND hCurCon = GetRealConsoleWindow();
	if (hCurCon == NULL && gnServerPID != 0)
	{
		// функция есть только в WinXP и выше
		AttachConsole_t _AttachConsole = GetAttachConsoleProc();
		if (_AttachConsole)
		{
			lbAttachRc = (_AttachConsole(gnServerPID) != 0);
			if (!lbAttachRc)
			{
				nErrCode = GetLastError();
				_ASSERTE(nErrCode==0 && lbAttachRc);
			}
		}
	}
	return lbAttachRc;
}





// Drop the progress flag, because the prompt may appear
void CheckPowershellProgressPresence()
{
	if (!gbPowerShellMonitorProgress || (gnPowerShellProgressValue == -1))
		return;

	// При возврате в Prompt - сброс прогресса
	gnPowerShellProgressValue = -1;
	GuiSetProgress(0,0);
}

// PowerShell AI для определения прогресса в консоли
void CheckPowerShellProgress(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion)
{
	_ASSERTE(dwBufferSize.Y >= 5);
	int nProgress = -1;
	WORD nNeedAttr = lpBuffer->Attributes;

	for (SHORT Y = dwBufferSize.Y - 2; Y > 0; Y--)
	{
		const CHAR_INFO* pLine = lpBuffer + dwBufferSize.X * Y;

		// 120720 - PS игнорирует PopupColors в консоли. Вывод прогресса всегда идет 0x3E
		if (nNeedAttr/*gnConsolePopupColors*/ != 0)
		{
			if ((pLine[4].Attributes != nNeedAttr/*gnConsolePopupColors*/)
				|| (pLine[dwBufferSize.X - 7].Attributes != nNeedAttr/*gnConsolePopupColors*/))
				break; // не оно
		}

		if ((pLine[4].Char.UnicodeChar == L'[') && (pLine[dwBufferSize.X - 7].Char.UnicodeChar == L']'))
		{
			// Считаем проценты
			SHORT nLen = dwBufferSize.X - 7 - 5;
			if (nLen > 0)
			{
				nProgress = 100;
				for (SHORT X = 5; X < (dwBufferSize.X - 8); X++)
				{
					if (pLine[X].Char.UnicodeChar == L' ')
					{
						nProgress = (X - 5) * 100 / nLen;
						break;
					}
				}
			}
			break;
		}
	}

	if (nProgress != gnPowerShellProgressValue)
	{
		gnPowerShellProgressValue = nProgress;
		GuiSetProgress((nProgress != -1) ? 1 : 0, nProgress);
	}
}






#ifdef _DEBUG
bool FindModuleByAddress(const BYTE* lpAddress, LPWSTR pszModule, int cchMax)
{
	bool bFound = false;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
	if (hSnap != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 mi = {sizeof(mi)};
		if (Module32First(hSnap, &mi))
		{
			do {
				if ((lpAddress >= mi.modBaseAddr) && (lpAddress < (mi.modBaseAddr + mi.modBaseSize)))
				{
					bFound = true;
					if (pszModule)
						lstrcpyn(pszModule, mi.szExePath, cchMax);
					break;
				}
			} while (Module32Next(hSnap, &mi));
		}
		CloseHandle(hSnap);
	}

	if (!bFound && pszModule)
		*pszModule = 0;
	return bFound;
}
#endif

#ifdef _DEBUG
LPVOID WINAPI OnVirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect)
{
	typedef LPVOID(WINAPI* OnVirtualAlloc_t)(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
	ORIGINALFAST(VirtualAlloc);

	LPVOID lpResult = F(VirtualAlloc)(lpAddress, dwSize, flAllocationType, flProtect);
	DWORD dwErr = GetLastError();

	wchar_t szText[MAX_PATH*4];
	if (lpResult == NULL)
	{
		wchar_t szTitle[64];
		msprintf(szTitle, countof(szTitle), L"ConEmuHk, PID=%u, TID=%u", GetCurrentProcessId(), GetCurrentThreadId());

		MEMORY_BASIC_INFORMATION mbi = {};
		SIZE_T mbiSize = VirtualQuery(lpAddress, &mbi, sizeof(mbi));
		wchar_t szOwnedModule[MAX_PATH] = L"";
		FindModuleByAddress((const BYTE*)lpAddress, szOwnedModule, countof(szOwnedModule));

		msprintf(szText, countof(szText)-MAX_PATH,
			L"VirtualAlloc " WIN3264TEST(L"%u",L"x%X%08X") L" bytes failed (0x%08X..0x%08X)\n"
			L"ErrorCode=%u %s\n\n"
			L"Allocated information (" WIN3264TEST(L"x%08X",L"x%X%08X") L".." WIN3264TEST(L"x%08X",L"x%X%08X") L")\n"
			L"Size " WIN3264TEST(L"%u",L"x%X%08X") L" bytes State x%X Type x%X Protect x%X\n"
			L"Module: %s\n\n"
			L"Warning! This may cause an errors in Release!\n"
			L"Press <OK> to VirtualAlloc at the default address\n\n",
			WIN3264WSPRINT(dwSize), (DWORD)lpAddress, (DWORD)((LPBYTE)lpAddress+dwSize),
			dwErr, (dwErr == 487) ? L"(ERROR_INVALID_ADDRESS)" : L"",
			WIN3264WSPRINT(mbi.BaseAddress), WIN3264WSPRINT(((LPBYTE)mbi.BaseAddress+mbi.RegionSize)),
			WIN3264WSPRINT(mbi.RegionSize), mbi.State, mbi.Type, mbi.Protect,
			szOwnedModule[0] ? szOwnedModule : L"<Unknown>",
			0);

		GetModuleFileName(NULL, szText+lstrlen(szText), MAX_PATH);

		DebugString(szText);

	#if defined(REPORT_VIRTUAL_ALLOC)
		// clink use bunch of VirtualAlloc to try to find suitable memory location
		// Some processes raises that error too often (in debug)
		bool bReport = (!gbIsCmdProcess || (dwSize != 0x1000)) && !gbSkipVirtualAllocErr && !gbIsNodeJSProcess;
		if (bReport)
		{
			// Do not report for .Net application
			static int iNetChecked = 0;
			if (!iNetChecked)
			{
				HMODULE hClr = GetModuleHandle(L"mscoree.dll");
				iNetChecked = hClr ? 2 : 1;
			}
			bReport = (iNetChecked == 1);
		}
		int iBtn = bReport ? GuiMessageBox(NULL, szText, szTitle, MB_SYSTEMMODAL|MB_OKCANCEL|MB_ICONSTOP) : IDCANCEL;
		if (iBtn == IDOK)
		{
			lpResult = F(VirtualAlloc)(NULL, dwSize, flAllocationType, flProtect);
			dwErr = GetLastError();
		}
	#endif
	}
	msprintf(szText, countof(szText),
		L"VirtualProtect(" WIN3264TEST(L"0x%08X",L"0x%08X%08X") L"," WIN3264TEST(L"0x%08X",L"0x%08X%08X") L",%u,%u)=" WIN3264TEST(L"0x%08X",L"0x%08X%08X") L"\n",
		WIN3264WSPRINT(lpAddress), WIN3264WSPRINT(dwSize), flAllocationType, flProtect, WIN3264WSPRINT(lpResult));
	DebugString(szText);

	SetLastError(dwErr);
	return lpResult;
}
BOOL WINAPI OnVirtualProtect(LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect)
{
	typedef BOOL(WINAPI* OnVirtualProtect_t)(LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect);
	ORIGINALFAST(VirtualProtect);
	BOOL bResult = FALSE;

	if (F(VirtualProtect))
		bResult = F(VirtualProtect)(lpAddress, dwSize, flNewProtect, lpflOldProtect);

#ifdef _DEBUG
	DWORD dwErr = GetLastError();
	wchar_t szDbgInfo[100];
	msprintf(szDbgInfo, countof(szDbgInfo),
		L"VirtualProtect(" WIN3264TEST(L"0x%08X",L"0x%08X%08X") L"," WIN3264TEST(L"0x%08X",L"0x%08X%08X") L",%u,%u)=%u, code=%u\n",
		WIN3264WSPRINT(lpAddress), WIN3264WSPRINT(dwSize), flNewProtect, lpflOldProtect ? *lpflOldProtect : 0, bResult, dwErr);
	DebugString(szDbgInfo);
	SetLastError(dwErr);
#endif

	return bResult;
}
LPTOP_LEVEL_EXCEPTION_FILTER WINAPI OnSetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter)
{
	typedef LPTOP_LEVEL_EXCEPTION_FILTER(WINAPI* OnSetUnhandledExceptionFilter_t)(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter);
	ORIGINALFAST(SetUnhandledExceptionFilter);

	LPTOP_LEVEL_EXCEPTION_FILTER lpRc = F(SetUnhandledExceptionFilter)(lpTopLevelExceptionFilter);

#ifdef _DEBUG
	DWORD dwErr = GetLastError();
	wchar_t szDbgInfo[100];
	msprintf(szDbgInfo, countof(szDbgInfo),
		L"SetUnhandledExceptionFilter(" WIN3264TEST(L"0x%08X",L"0x%08X%08X") L")=" WIN3264TEST(L"0x%08X",L"0x%08X%08X") L", code=%u\n",
		WIN3264WSPRINT(lpTopLevelExceptionFilter), WIN3264WSPRINT(lpRc), dwErr);
	DebugString(szDbgInfo);
	SetLastError(dwErr);
#endif

	return lpRc;
}
#endif


BOOL GuiSetForeground(HWND hWnd)
{
	BOOL lbRc = FALSE;

	if (ghConEmuWndDC)
	{
		CESERVER_REQ *pIn = (CESERVER_REQ*)malloc(sizeof(*pIn)), *pOut;
		if (pIn)
		{
			ExecutePrepareCmd(pIn, CECMD_SETFOREGROUND, sizeof(CESERVER_REQ_HDR)+sizeof(u64)); //-V119

			DWORD nConEmuPID = ASFW_ANY;
			GetWindowThreadProcessId(ghConEmuWndDC, &nConEmuPID);
			AllowSetForegroundWindow(nConEmuPID);

			pIn->qwData[0] = (u64)hWnd;
			HWND hConWnd = GetRealConsoleWindow();
			pOut = ExecuteGuiCmd(hConWnd, pIn, hConWnd);

			if (pOut)
			{
				if (pOut->hdr.cbSize == (sizeof(CESERVER_REQ_HDR)+sizeof(DWORD)))
					lbRc = pOut->dwData[0];
				ExecuteFreeResult(pOut);
			}
			free(pIn);
		}
	}

	return lbRc;
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
			HWND hConWnd = GetRealConsoleWindow();
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

BOOL MyGetConsoleFontSize(COORD& crFontSize)
{
	BOOL lbRc = FALSE;

	if (ghConEmuWnd)
	{
		_ASSERTE(gnGuiPID!=0);
		ConEmuGuiMapping* inf = (ConEmuGuiMapping*)malloc(sizeof(ConEmuGuiMapping));
		if (inf)
		{
			if (LoadGuiMapping(gnGuiPID, *inf))
			{
				crFontSize.Y = (SHORT)inf->MainFont.nFontHeight;
				crFontSize.X = (SHORT)inf->MainFont.nFontCellWidth;
				lbRc = TRUE;
			}
			free(inf);
		}

		_ASSERTEX(lbRc && (crFontSize.X > 3) && (crFontSize.Y > 4));
	}

	return lbRc;
}


void CheckVariables()
{
	// Пока что он проверяет и меняет только ENV_CONEMUANSI_VAR_W ("ConEmuANSI")
	GetConMap(FALSE);
}

void CheckAnsiConVar(LPCWSTR asName)
{
	bool bAnsi = false;
	CEAnsi::GetFeatures(&bAnsi, NULL);

	if (bAnsi)
	{
		HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO lsbi = {{0,0}};
		if (GetConsoleScreenBufferInfo(hOut, &lsbi))
		{
			wchar_t szInfo[40];
			msprintf(szInfo, countof(szInfo), L"%ux%u (%ux%u)", lsbi.dwSize.X, lsbi.dwSize.Y, lsbi.srWindow.Right-lsbi.srWindow.Left+1, lsbi.srWindow.Bottom-lsbi.srWindow.Top+1);
			SetEnvironmentVariable(ENV_ANSICON_VAR_W, szInfo);

			static WORD clrDefault = 0xFFFF;
			if ((clrDefault == 0xFFFF) && LOBYTE(lsbi.wAttributes))
				clrDefault = LOBYTE(lsbi.wAttributes);
			msprintf(szInfo, countof(szInfo), L"%X", clrDefault);
			SetEnvironmentVariable(ENV_ANSICON_DEF_VAR_W, szInfo);
		}
	}
	else
	{
		SetEnvironmentVariable(ENV_ANSICON_VAR_W, NULL);
		SetEnvironmentVariable(ENV_ANSICON_DEF_VAR_W, NULL);
	}
}

BOOL WINAPI OnSetEnvironmentVariableA(LPCSTR lpName, LPCSTR lpValue)
{
	typedef BOOL (WINAPI* OnSetEnvironmentVariableA_t)(LPCSTR lpName, LPCSTR lpValue);
	ORIGINALFAST(SetEnvironmentVariableA);

	if (lpName && *lpName)
	{
		if (lstrcmpiA(lpName, ENV_CONEMUFAKEDT_VAR_A) == 0)
		{
			MultiByteToWideChar(CP_OEMCP, 0, lpValue ? lpValue : "", -1, gszTimeEnvVarSave, countof(gszTimeEnvVarSave)-1);
		}
	}

	BOOL b = F(SetEnvironmentVariableA)(lpName, lpValue);

	return b;
}

BOOL WINAPI OnSetEnvironmentVariableW(LPCWSTR lpName, LPCWSTR lpValue)
{
	typedef BOOL (WINAPI* OnSetEnvironmentVariableW_t)(LPCWSTR lpName, LPCWSTR lpValue);
	ORIGINALFAST(SetEnvironmentVariableW);

	if (lpName && *lpName)
	{
		if (lstrcmpi(lpName, ENV_CONEMUFAKEDT_VAR_W) == 0)
		{
			lstrcpyn(gszTimeEnvVarSave, lpValue ? lpValue : L"", countof(gszTimeEnvVarSave));
		}
	}

	BOOL b = F(SetEnvironmentVariableW)(lpName, lpValue);

	return b;
}

DWORD WINAPI OnGetEnvironmentVariableA(LPCSTR lpName, LPSTR lpBuffer, DWORD nSize)
{
	typedef DWORD (WINAPI* OnGetEnvironmentVariableA_t)(LPCSTR lpName, LPSTR lpBuffer, DWORD nSize);
	ORIGINALFAST(GetEnvironmentVariableA);
	//BOOL bMainThread = FALSE; // поток не важен
	DWORD lRc = 0;

	if (lpName)
	{
		if ((lstrcmpiA(lpName, ENV_CONEMUANSI_VAR_A) == 0)
			|| (lstrcmpiA(lpName, ENV_CONEMUHWND_VAR_A) == 0)
			|| (lstrcmpiA(lpName, ENV_CONEMUDIR_VAR_A) == 0)
			|| (lstrcmpiA(lpName, ENV_CONEMUBASEDIR_VAR_A) == 0)
			)
		{
			CheckVariables();
		}
		else if (lstrcmpiA(lpName, ENV_CONEMUANSI_VAR_A) == 0)
		{
			CheckAnsiConVar(ENV_CONEMUANSI_VAR_W);
		}
		else if (lstrcmpiA(lpName, ENV_ANSICON_DEF_VAR_A) == 0)
		{
			CheckAnsiConVar(ENV_ANSICON_DEF_VAR_W);
		}
		else if (lstrcmpiA(lpName, ENV_ANSICON_VER_VAR_A) == 0)
		{
			if (lpBuffer && ((INT_PTR)nSize > lstrlenA(ENV_ANSICON_VER_VALUE)))
			{
				lstrcpynA(lpBuffer, ENV_ANSICON_VER_VALUE, nSize);
				lRc = lstrlenA(ENV_ANSICON_VER_VALUE);
			}
			goto wrap;
		}
	}

	lRc = F(GetEnvironmentVariableA)(lpName, lpBuffer, nSize);
wrap:
	return lRc;
}

DWORD WINAPI OnGetEnvironmentVariableW(LPCWSTR lpName, LPWSTR lpBuffer, DWORD nSize)
{
	typedef DWORD (WINAPI* OnGetEnvironmentVariableW_t)(LPCWSTR lpName, LPWSTR lpBuffer, DWORD nSize);
	ORIGINALFAST(GetEnvironmentVariableW);
	//BOOL bMainThread = FALSE; // поток не важен
	DWORD lRc = 0;

	if (lpName)
	{
		if ((lstrcmpiW(lpName, ENV_CONEMUANSI_VAR_W) == 0)
			|| (lstrcmpiW(lpName, ENV_CONEMUHWND_VAR_W) == 0)
			|| (lstrcmpiW(lpName, ENV_CONEMUDIR_VAR_W) == 0)
			|| (lstrcmpiW(lpName, ENV_CONEMUBASEDIR_VAR_W) == 0)
			)
		{
			CheckVariables();
		}
		else if ((lstrcmpiW(lpName, ENV_CONEMUANSI_VAR_W) == 0)
				|| (lstrcmpiW(lpName, ENV_ANSICON_DEF_VAR_W) == 0))
		{
			CheckAnsiConVar(lpName);
		}
		else if (lstrcmpiW(lpName, ENV_ANSICON_VER_VAR_W) == 0)
		{
			if (lpBuffer && ((INT_PTR)nSize > lstrlenA(ENV_ANSICON_VER_VALUE)))
			{
				lstrcpynW(lpBuffer, _CRT_WIDE(ENV_ANSICON_VER_VALUE), nSize);
				lRc = lstrlenA(ENV_ANSICON_VER_VALUE);
			}
			goto wrap;
		}
	}

	lRc = F(GetEnvironmentVariableW)(lpName, lpBuffer, nSize);
wrap:
	return lRc;
}

#if 0
LPCH WINAPI OnGetEnvironmentStringsA()
{
	typedef LPCH (WINAPI* OnGetEnvironmentStringsA_t)();
	ORIGINALFAST(GetEnvironmentStringsA);
	BOOL bMainThread = FALSE; // поток не важен

	CheckVariables();

	LPCH lpRc = F(GetEnvironmentStringsA)();
	return lpRc;
}
#endif

LPWCH WINAPI OnGetEnvironmentStringsW()
{
	typedef LPWCH (WINAPI* OnGetEnvironmentStringsW_t)();
	ORIGINALFAST(GetEnvironmentStringsW);
	//BOOL bMainThread = FALSE; // поток не важен

	CheckVariables();

	LPWCH lpRc = F(GetEnvironmentStringsW)();
	return lpRc;
}


// Helper function
bool LocalToSystemTime(LPFILETIME lpLocal, LPSYSTEMTIME lpSystem)
{
	false;
	FILETIME ftSystem;
	bool bOk = LocalFileTimeToFileTime(lpLocal, &ftSystem)
		&& FileTimeToSystemTime(&ftSystem, lpSystem);
	return bOk;
}

bool GetTime(bool bSystem, LPSYSTEMTIME lpSystemTime)
{
	bool bHacked = false;
	wchar_t szEnvVar[32] = L""; // 2013-01-01T15:16:17.95

	DWORD nCurTick = GetTickCount();
	DWORD nCheckDelta = nCurTick - gnTimeEnvVarLastCheck;
	const DWORD nCheckDeltaMax = 1000;
	if (!gnTimeEnvVarLastCheck || (nCheckDelta >= nCheckDeltaMax))
	{
		gnTimeEnvVarLastCheck = nCurTick;
		GetEnvironmentVariable(ENV_CONEMUFAKEDT_VAR_W, szEnvVar, countof(szEnvVar));
		lstrcpyn(gszTimeEnvVarSave, szEnvVar, countof(gszTimeEnvVarSave));
	}
	else if (*gszTimeEnvVarSave)
	{
		lstrcpyn(szEnvVar, gszTimeEnvVarSave, countof(szEnvVar));
	}
	else
	{
		goto wrap;
	}

	if (*szEnvVar)
	{
		SYSTEMTIME st = {0}; FILETIME ft; wchar_t* p = szEnvVar;

		if (!(st.wYear = LOWORD(wcstol(p, &p, 10))) || !p || (*p != L'-' && *p != L'.'))
			goto wrap;
		if (!(st.wMonth = LOWORD(wcstol(p+1, &p, 10))) || !p || (*p != L'-' && *p != L'.'))
			goto wrap;
		if (!(st.wDay = LOWORD(wcstol(p+1, &p, 10))) || !p || (*p != L'T' && *p != L' ' && *p != 0))
			goto wrap;
		// Possible format 'dd.mm.yyyy'? This is returned by "cmd /k echo %DATE%"
		if (st.wDay >= 1900 && st.wYear <= 31)
		{
			WORD w = st.wDay; st.wDay = st.wYear; st.wYear = w;
		}

		// Time. Optional?
		if (!p || !*p)
		{
			SYSTEMTIME lt; GetLocalTime(&lt);
			st.wHour = lt.wHour;
			st.wMinute = lt.wMinute;
			st.wSecond = lt.wSecond;
			st.wMilliseconds = lt.wMilliseconds;
		}
		else
		{
			if (((st.wHour = LOWORD(wcstol(p+1, &p, 10)))>=24) || !p || (*p != L':' && *p != L'.'))
				goto wrap;
			if (((st.wMinute = LOWORD(wcstol(p+1, &p, 10)))>=60))
				goto wrap;

			// Seconds and MS are optional
			if ((p && (*p == L':' || *p == L'.')) && ((st.wSecond = LOWORD(wcstol(p+1, &p, 10))) >= 60))
				goto wrap;
			// cmd`s %TIME% shows Milliseconds as two digits
			if ((p && (*p == L':' || *p == L'.')) && ((st.wMilliseconds = (10*LOWORD(wcstol(p+1, &p, 10)))) >= 1000))
				goto wrap;
		}

		// Check it
		if (!SystemTimeToFileTime(&st, &ft))
			goto wrap;
		if (bSystem)
		{
			if (!LocalToSystemTime(&ft, lpSystemTime))
				goto wrap;
		}
		else
		{
			*lpSystemTime = st;
		}
		bHacked = true;
	}
wrap:
	return bHacked;
}

// TODO: GetSystemTimeAsFileTime??
void WINAPI OnGetSystemTime(LPSYSTEMTIME lpSystemTime)
{
	typedef void (WINAPI* OnGetSystemTime_t)(LPSYSTEMTIME);
	ORIGINALFAST(GetSystemTime);

	if (!GetTime(true, lpSystemTime))
		F(GetSystemTime)(lpSystemTime);
}

void WINAPI OnGetLocalTime(LPSYSTEMTIME lpSystemTime)
{
	typedef void (WINAPI* OnGetLocalTime_t)(LPSYSTEMTIME);
	ORIGINALFAST(GetLocalTime);

	if (!GetTime(false, lpSystemTime))
		F(GetLocalTime)(lpSystemTime);
}

void WINAPI OnGetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime)
{
	typedef void (WINAPI* OnGetSystemTimeAsFileTime_t)(LPFILETIME);
	ORIGINALFAST(GetSystemTimeAsFileTime);

	SYSTEMTIME st;
	if (GetTime(true, &st))
	{
		SystemTimeToFileTime(&st, lpSystemTimeAsFileTime);
	}
	else
	{
		F(GetSystemTimeAsFileTime)(lpSystemTimeAsFileTime);
	}
}


BOOL IsVisibleRectLocked(COORD& crLocked)
{
	CESERVER_CONSOLE_MAPPING_HDR SrvMap;
	if (LoadSrvMapping(ghConWnd, SrvMap))
	{
		if (SrvMap.bLockVisibleArea && (SrvMap.crLockedVisible.X > 0) && (SrvMap.crLockedVisible.Y > 0))
		{
			crLocked = SrvMap.crLockedVisible;
			return TRUE;
		}
	}
	return FALSE;
}


// Due to Microsoft bug we need to lock Server reading thread to avoid crash of kernel
// http://conemu.github.io/en/MicrosoftBugs.html#Exception_in_ReadConsoleOutput
void LockServerReadingThread(bool bLock, COORD dwSize, CESERVER_REQ*& pIn, CESERVER_REQ*& pOut)
{
	DWORD nServerPID = gnServerPID;
	if (!nServerPID)
		return;

	DWORD nErrSave = GetLastError();

	if (bLock)
	{
		HANDLE hOurThreadHandle = NULL;

		// We need to give our thread handle (to server process) to avoid
		// locking of server reading thread (in case of our thread fails)
		DWORD dwErr = -1;
		HANDLE hServer = OpenProcess(PROCESS_DUP_HANDLE, FALSE, nServerPID);
		if (!hServer)
		{
			dwErr = GetLastError();
			_ASSERTEX(hServer!=NULL && "Open server handle fails, Can't dup handle!");
		}
		else
		{
			if (!DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), hServer, &hOurThreadHandle,
					SYNCHRONIZE|THREAD_QUERY_INFORMATION, FALSE, 0))
			{
				dwErr = GetLastError();
				_ASSERTEX(hServer!=NULL && "DuplicateHandle fails, Can't dup handle!");
				hOurThreadHandle = NULL;
			}
			CloseHandle(hServer);
		}

		pIn = ExecuteNewCmd(CECMD_SETCONSCRBUF, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_SETCONSCRBUF));
		if (pIn)
		{
			pIn->SetConScrBuf.bLock = TRUE;
			pIn->SetConScrBuf.dwSize = dwSize; // Informational
			pIn->SetConScrBuf.hRequestor = hOurThreadHandle;
		}
	}
	else
	{
		if (pIn)
		{
			pIn->SetConScrBuf.bLock = FALSE;
		}
	}

	if (pIn)
	{
		ExecuteFreeResult(pOut);
		pOut = ExecuteSrvCmd(nServerPID, pIn, ghConWnd);
		if (pOut && pOut->DataSize() >= sizeof(CESERVER_REQ_SETCONSCRBUF))
		{
			pIn->SetConScrBuf.hTemp = pOut->SetConScrBuf.hTemp;
		}
	}

	if (!bLock)
	{
		ExecuteFreeResult(pIn);
		ExecuteFreeResult(pOut);
	}

	// Transparently to calling function...
	SetLastError(nErrSave);
}

void PatchDialogParentWnd(HWND& hWndParent)
{
	WARNING("Проверить все перехваты диалогов (A/W). По идее, надо менять hWndParent, а то диалоги прячутся");
	// Re: conemu + emenu/{Run Sandboxed} замораживает фар

	if (ghConEmuWndDC)
	{
		if (!hWndParent || !IsWindowVisible(hWndParent))
			hWndParent = ghConEmuWnd;
	}
}


BOOL GetConsoleScreenBufferInfoCached(HANDLE hConsoleOutput, PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo, BOOL bForced /*= FALSE*/)
{
	BOOL lbRc = FALSE;

	static DWORD s_LastCheckTick = 0;
	static CONSOLE_SCREEN_BUFFER_INFO s_csbi = {};
	static HANDLE s_hConOut = NULL;
	//DWORD nTickDelta = 0;
	//const DWORD TickDeltaMax = 250;

	if (hConsoleOutput == NULL)
	{
		// Сброс
		s_hConOut = NULL;
		GetConsoleModeCached(NULL, NULL);
		return FALSE;
	}

	if (!lpConsoleScreenBufferInfo)
	{
		_ASSERTEX(lpConsoleScreenBufferInfo!=NULL);
		return FALSE;
	}

#if 0
	if (s_hConOut && (s_hConOut == hConsoleOutput))
	{
		nTickDelta = GetTickCount() - s_LastCheckTick;
		if (nTickDelta <= TickDeltaMax)
		{
			if (bForced)
			{
				#ifdef _DEBUG
				lbRc = FALSE;
				#endif
			}
			else
			{
				*lpConsoleScreenBufferInfo = s_csbi;
				lbRc = TRUE;
			}
		}
	}
#endif

	if (!lbRc)
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi = {};
		lbRc = GetConsoleScreenBufferInfo(hConsoleOutput, &csbi);
		*lpConsoleScreenBufferInfo = csbi;
		if (lbRc)
		{
			s_csbi = csbi;
			s_LastCheckTick = GetTickCount();
			s_hConOut = hConsoleOutput;
		}
	}

	return lbRc;
}

BOOL GetConsoleModeCached(HANDLE hConsoleHandle, LPDWORD lpMode, BOOL bForced /*= FALSE*/)
{
	BOOL lbRc = FALSE;

	static DWORD s_LastCheckTick = 0;
	static DWORD s_dwMode = 0;
	static HANDLE s_hConHandle = NULL;
	DWORD nTickDelta = 0;
	const DWORD TickDeltaMax = 250;

	if (hConsoleHandle == NULL)
	{
		// Сброс
		s_hConHandle = NULL;
		return FALSE;
	}

	if (!lpMode)
	{
		_ASSERTEX(lpMode!=NULL);
		return FALSE;
	}

	if (s_hConHandle && (s_hConHandle == hConsoleHandle))
	{
		nTickDelta = GetTickCount() - s_LastCheckTick;
		if (nTickDelta <= TickDeltaMax)
		{
			if (bForced)
			{
				#ifdef _DEBUG
				lbRc = FALSE;
				#endif
			}
			else
			{
				*lpMode = s_dwMode;
				lbRc = TRUE;
			}
		}
	}

	if (!lbRc)
	{
		DWORD dwMode = 0;
		lbRc = GetConsoleMode(hConsoleHandle, &dwMode);
		*lpMode = dwMode;
		if (lbRc)
		{
			s_dwMode = dwMode;
			s_LastCheckTick = GetTickCount();
			s_hConHandle = hConsoleHandle;
		}
	}

	return lbRc;
}
