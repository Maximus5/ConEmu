
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

#define SETCONCP_READYTIMEOUT 5000
#define SETCONCP_TIMEOUT 1000

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
#include "../common/UnicodeChars.h"
#include "../common/WConsole.h"
#include "../common/WThreads.h"


bool USE_INTERNAL_QUEUE = true;

#ifdef _DEBUG
// для отладки ecompl
INPUT_RECORD gir_Written[16] = {};
INPUT_RECORD gir_Real[16] = {};
INPUT_RECORD gir_Virtual[16] = {};
#endif

#ifdef _DEBUG
// Only for input_bug search purposes in Debug builds
const LONG gn_LogReadCharsMax = 4096; // must be power of 2
wchar_t gs_LogReadChars[gn_LogReadCharsMax*2+1] = L""; // "+1" для ASCIIZ
LONG gn_LogReadChars = -1;
#endif

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

//110131 попробуем просто добвавить ее в ExcludedModules
//#include <WinInet.h>
//#pragma comment(lib, "wininet.lib")


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
#define DebugStringConSize(x) //OutputDebugString(x)
#else
#define DebugString(x) //OutputDebugString(x)
#define DebugStringConSize(x)
#endif


//#ifdef _DEBUG
//	#include <crtdbg.h>
//#else
//	#ifndef _ASSERTE
//		#define _ASSERTE(x)
//	#endif
//#endif

extern HMODULE ghOurModule; // Хэндл нашей dll'ки (здесь хуки не ставятся)
extern BOOL    gbHooksTemporaryDisabled;
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
extern BOOL    gbWasSucceededInRead;
HDC ghTempHDC = NULL;
GetConsoleWindow_T gfGetRealConsoleWindow = NULL;
//extern HWND WINAPI GetRealConsoleWindow(); // Entry.cpp
extern HANDLE ghCurrentOutBuffer;
HANDLE ghStdOutHandle = NULL;
HANDLE ghLastConInHandle = NULL, ghLastNotConInHandle = NULL;
/* ************ Globals for SetHook ************ */


/* ************ Globals for Far Hooks ************ */
HookModeFar gFarMode = {sizeof(HookModeFar)};
bool    gbIsFarProcess = false;
InQueue gInQueue = {};
HANDLE  ghConsoleCursorChanged = NULL;
/* ************ Globals for Far Hooks ************ */

enum CEReadConsoleInputFlags
{
	rcif_Ansi      = 1,
	rcif_Unicode   = 2,
	rcif_Peek      = 4,
	rcif_LLInput   = 8, // [Read|Peek]ConsoleInput[A|W]
};
void PreReadConsoleInput(HANDLE hConIn, DWORD nFlags, CESERVER_CONSOLE_APP_MAPPING** ppAppMap = NULL);
void PostReadConsoleInput(HANDLE hConIn, DWORD nFlags, CESERVER_CONSOLE_APP_MAPPING* ppAppMap = NULL);

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
wchar_t* gszClinkCmdLine = NULL;
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


//
//static BOOL WINAPI OnHttpSendRequestA(LPVOID hRequest, LPCSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength);
//static BOOL WINAPI OnHttpSendRequestW(LPVOID hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength);

//static HookItem HooksFarOnly[] =
//{
////	{OnlstrcmpiA,      "lstrcmpiA",      kernel32, 0},
//	{(void*)OnCompareStringW, "CompareStringW", kernel32},
//
//	/* ************************ */
//	//110131 попробуем просто добавить ее в ExcludedModules
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
HRESULT WINAPI OnShellExecCmdLine(HWND hwnd, LPCWSTR pwszCommand, LPCWSTR pwszStartDir, int nShow, LPVOID pUnused, DWORD dwSeclFlags);
HINSTANCE WINAPI OnShellExecuteA(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd);
HINSTANCE WINAPI OnShellExecuteW(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd);
BOOL WINAPI OnFlashWindow(HWND hWnd, BOOL bInvert);
BOOL WINAPI OnFlashWindowEx(PFLASHWINFO pfwi);
BOOL WINAPI OnSetForegroundWindow(HWND hWnd);
HWND WINAPI OnGetForegroundWindow();
int WINAPI OnCompareStringW(LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2);
DWORD WINAPI OnGetConsoleAliasesW(LPWSTR AliasBuffer, DWORD AliasBufferLength, LPWSTR ExeName);
BOOL WINAPI OnReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
BOOL WINAPI OnReadConsoleA(HANDLE hConsoleInput, LPVOID lpBuffer, DWORD nNumberOfCharsToRead, LPDWORD lpNumberOfCharsRead, LPVOID pInputControl);
BOOL WINAPI OnReadConsoleW(HANDLE hConsoleInput, LPVOID lpBuffer, DWORD nNumberOfCharsToRead, LPDWORD lpNumberOfCharsRead, MY_CONSOLE_READCONSOLE_CONTROL* pInputControl);
BOOL WINAPI OnGetNumberOfConsoleInputEvents(HANDLE hConsoleInput, LPDWORD lpcNumberOfEvents);
BOOL WINAPI OnFlushConsoleInputBuffer(HANDLE hConsoleInput);
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
BOOL WINAPI OnAllocConsole(void);
BOOL WINAPI OnFreeConsole(void);
HWND WINAPI OnGetConsoleWindow();
BOOL WINAPI OnGetConsoleMode(HANDLE hConsoleHandle,LPDWORD lpMode);
BOOL WINAPI OnWriteConsoleOutputA(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
BOOL WINAPI OnWriteConsoleOutputW(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
BOOL WINAPI OnSetConsoleTextAttribute(HANDLE hConsoleOutput, WORD wAttributes);
//BOOL WINAPI OnWriteConsoleOutputAx(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
//BOOL WINAPI OnWriteConsoleOutputWx(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
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







extern HANDLE ghSkipSetThreadContextForThread;
BOOL WINAPI OnSetThreadContext(HANDLE hThread, CONST CONTEXT *lpContext);

HANDLE WINAPI OnOpenFileMappingW(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName);
LPVOID WINAPI OnMapViewOfFile(HANDLE hFileMappingObject, DWORD dwDesiredAccess, DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow, SIZE_T dwNumberOfBytesToMap);
BOOL WINAPI OnUnmapViewOfFile(LPCVOID lpBaseAddress);
BOOL WINAPI OnCloseHandle(HANDLE hObject);
BOOL WINAPI OnGetModuleHandleExW(DWORD dwFlags, LPCWSTR lpModuleName, HMODULE *phModule);

#ifdef _DEBUG
HANDLE WINAPI OnCreateNamedPipeW(LPCWSTR lpName, DWORD dwOpenMode, DWORD dwPipeMode, DWORD nMaxInstances,DWORD nOutBufferSize, DWORD nInBufferSize, DWORD nDefaultTimeOut,LPSECURITY_ATTRIBUTES lpSecurityAttributes);
#endif
BOOL WINAPI OnSetConsoleCP(UINT wCodePageID);
BOOL WINAPI OnSetConsoleOutputCP(UINT wCodePageID);
#ifdef _DEBUG
LPVOID WINAPI OnVirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
BOOL WINAPI OnVirtualProtect(LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect);
LPTOP_LEVEL_EXCEPTION_FILTER WINAPI OnSetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter);
#endif
HANDLE WINAPI OnCreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);
BOOL WINAPI OnChooseColorA(LPCHOOSECOLORA lpcc);
BOOL WINAPI OnChooseColorW(LPCHOOSECOLORW lpcc);
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

//#ifdef HOOK_ANSI_SEQUENCES
//BOOL WINAPI OnWriteConsoleA(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved);
//BOOL WINAPI OnWriteConsoleW(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved);
//BOOL WINAPI OnWriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
//BOOL WINAPI OnScrollConsoleScreenBufferA(HANDLE hConsoleOutput, const SMALL_RECT *lpScrollRectangle, const SMALL_RECT *lpClipRectangle, COORD dwDestinationOrigin, const CHAR_INFO *lpFill);
//BOOL WINAPI OnScrollConsoleScreenBufferW(HANDLE hConsoleOutput, const SMALL_RECT *lpScrollRectangle, const SMALL_RECT *lpClipRectangle, COORD dwDestinationOrigin, const CHAR_INFO *lpFill);
//BOOL WINAPI OnWriteConsoleOutputCharacterA(HANDLE hConsoleOutput, LPCSTR lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten);
//BOOL WINAPI OnWriteConsoleOutputCharacterW(HANDLE hConsoleOutput, LPCWSTR lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten);
//#ifdef _DEBUG
//BOOL WINAPI OnSetConsoleMode(HANDLE hConsoleHandle, DWORD dwMode);
//#endif
//#endif
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
		// Issue 1899: Support GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS flag because of CreateFileW
		{(void*)OnGetModuleHandleExW,	"GetModuleHandleExW",	kernel32},
		/* ANSI Escape Sequences SUPPORT */
		//#ifdef HOOK_ANSI_SEQUENCES
		{(void*)CEAnsi::OnCreateFileW,	"CreateFileW",  		kernel32},
		{(void*)CEAnsi::OnWriteFile,	"WriteFile",  			kernel32},
		{(void*)CEAnsi::OnWriteConsoleA,"WriteConsoleA",  		kernel32},
		{(void*)CEAnsi::OnWriteConsoleW,"WriteConsoleW",  		kernel32},
		{(void*)CEAnsi::OnScrollConsoleScreenBufferA,
										"ScrollConsoleScreenBufferA",
																kernel32},
		{(void*)CEAnsi::OnScrollConsoleScreenBufferW,
										"ScrollConsoleScreenBufferW",
																kernel32},
		{(void*)CEAnsi::OnWriteConsoleOutputCharacterA,
										"WriteConsoleOutputCharacterA",
																kernel32},
		{(void*)CEAnsi::OnWriteConsoleOutputCharacterW,
										"WriteConsoleOutputCharacterW",
																kernel32},
		{(void*)CEAnsi::OnSetConsoleMode,
										"SetConsoleMode",  		kernel32},
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

bool InitHooksDefaultTrm()
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

	if (gbIsVStudio)
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
		return true; // не хукать

	HookItem HooksFarOnly[] =
	{
	//	{OnlstrcmpiA,      "lstrcmpiA",      kernel32, 0},
		{(void*)OnCompareStringW, "CompareStringW", kernel32},

		/* ************************ */
		//110131 попробуем просто добавить ее в ExcludedModules
		//{(void*)OnHttpSendRequestA, "HttpSendRequestA", wininet, 0},
		//{(void*)OnHttpSendRequestW, "HttpSendRequestW", wininet, 0},
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
BOOL StartupHooks(HMODULE ahOurDll)
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
		InitHooksDefaultTrm();
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
	bool lbRc = SetAllHooks(ahOurDll, NULL, TRUE);
	HLOGEND1();

	print_timings(L"SetAllHooks - done");

	//HLOGEND(); // StartupHooks - done

	return lbRc;
}




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

// May be called from "C" programs
VOID WINAPI OnExitProcess(UINT uExitCode)
{
	typedef BOOL (WINAPI* OnExitProcess_t)(UINT uExitCode);
	ORIGINALFAST(ExitProcess);

	#if 0
	if (gbIsLessProcess)
	{
		_ASSERTE(FALSE && "Continue to ExitProcess");
	}
	#endif

	// And terminate our threads
	DoDllStop(false, true);

	// Issue 1865: Due to possible dead locks in LdrpAcquireLoaderLock() call TerminateProcess
	if (gbHookServerForcedTermination)
	{
		TerminateProcess(GetCurrentProcess(), uExitCode);
		return; // Assume not to get here
	}

	F(ExitProcess)(uExitCode);
}

// For example, mintty is terminated ‘abnormally’. It calls TerminateProcess instead of ExitProcess.
BOOL WINAPI OnTerminateProcess(HANDLE hProcess, UINT uExitCode)
{
	typedef BOOL (WINAPI* OnTerminateProcess_t)(HANDLE hProcess, UINT uExitCode);
	ORIGINALFAST(TerminateProcess);
	BOOL lbRc;

	if (hProcess == GetCurrentProcess())
	{
		// We need not to unset hooks (due to process will be force-killed below)
		// And terminate our threads
		DoDllStop(false, true);
	}

	lbRc = F(TerminateProcess)(hProcess, uExitCode);

	return lbRc;
}

BOOL WINAPI OnTerminateThread(HANDLE hThread, DWORD dwExitCode)
{
	typedef BOOL (WINAPI* OnTerminateThread_t)(HANDLE hThread, UINT dwExitCode);
	ORIGINALFAST(TerminateThread);
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
		DoDllStop(false);
	}

	lbRc = F(TerminateThread)(hThread, dwExitCode);

	return lbRc;
}


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
	typedef BOOL (WINAPI* OnCreateProcessW_t)(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
	ORIGINALFAST(CreateProcessW);
	BOOL bMainThread = FALSE; // поток не важен
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
	wchar_t szTimingMsg[512]; HANDLE hTimingHandle = GetStdHandle(STD_OUTPUT_HANDLE);
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
	ORIGINALFAST(CreateProcessA);
	BOOL bMainThread = FALSE; // поток не важен
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

HANDLE WINAPI OnOpenFileMappingW(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName)
{
	typedef HANDLE (WINAPI* OnOpenFileMappingW_t)(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName);
	ORIGINALFAST(OpenFileMappingW);
	//BOOL bMainThread = FALSE; // поток не важен
	HANDLE hRc = FALSE;

	if (ghConEmuWndDC && lpName && *lpName)
	{
		/**
		* Share name to search for
		* Two replacements:
		*   %d sizeof(AnnotationInfo) - compatibility/versioning field.
		*   %d console window handle
		*/
		wchar_t szTrueColorMap[64];
		// #define  L"Console_annotationInfo_%x_%x"
		msprintf(szTrueColorMap, countof(szTrueColorMap), AnnotationShareName, (DWORD)sizeof(AnnotationInfo), ghConEmuWndDC);
		// При попытке открыть мэппинг для TrueColor - перейти в режим локального сервера
		if (lstrcmpi(lpName, szTrueColorMap) == 0)
		{
			RequestLocalServerParm Parm = {(DWORD)sizeof(Parm), slsf_RequestTrueColor|slsf_GetCursorEvent};
			if (RequestLocalServer(&Parm) == 0)
			{
				if (Parm.pAnnotation)
				{
					gpAnnotationHeader = Parm.pAnnotation;
					hRc = (HANDLE)Parm.pAnnotation;
					goto wrap;
				}
				else
				{
					WARNING("Перенести обработку AnnotationShareName в хуки");
				}

				if (ghConsoleCursorChanged && (ghConsoleCursorChanged != Parm.hCursorChangeEvent))
					CloseHandle(ghConsoleCursorChanged);
				ghConsoleCursorChanged = Parm.hCursorChangeEvent;
			}
		}
	}

	hRc = F(OpenFileMappingW)(dwDesiredAccess, bInheritHandle, lpName);

wrap:
	return hRc;
}

LPVOID WINAPI OnMapViewOfFile(HANDLE hFileMappingObject, DWORD dwDesiredAccess, DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow, SIZE_T dwNumberOfBytesToMap)
{
	typedef LPVOID (WINAPI* OnMapViewOfFile_t)(HANDLE hFileMappingObject, DWORD dwDesiredAccess, DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow, SIZE_T dwNumberOfBytesToMap);
	ORIGINALFAST(MapViewOfFile);
	//BOOL bMainThread = FALSE; // поток не важен
	LPVOID ptr = NULL;

	if (gpAnnotationHeader && (hFileMappingObject == (HANDLE)gpAnnotationHeader))
	{
		_ASSERTE(!dwFileOffsetHigh && !dwFileOffsetLow && !dwNumberOfBytesToMap);
		ptr = gpAnnotationHeader;
	}
	else
	{
		ptr = F(MapViewOfFile)(hFileMappingObject, dwDesiredAccess, dwFileOffsetHigh, dwFileOffsetLow, dwNumberOfBytesToMap);
	}

	return ptr;
}

BOOL WINAPI OnUnmapViewOfFile(LPCVOID lpBaseAddress)
{
	typedef BOOL (WINAPI* OnUnmapViewOfFile_t)(LPCVOID lpBaseAddress);
	ORIGINALFAST(UnmapViewOfFile);
	//BOOL bMainThread = FALSE; // поток не важен
    BOOL lbRc = FALSE;

	if (gpAnnotationHeader && (lpBaseAddress == gpAnnotationHeader))
	{
		lbRc = TRUE;
	}
	else
	{
    	lbRc = F(UnmapViewOfFile)(lpBaseAddress);
	}

    return lbRc;
}

BOOL WINAPI OnCloseHandle(HANDLE hObject)
{
	typedef BOOL (WINAPI* OnCloseHandle_t)(HANDLE hObject);
	ORIGINALFAST(CloseHandle);
	//BOOL bMainThread = FALSE; // поток не важен
	BOOL lbRc = FALSE;

	LPHANDLE hh[] = {
		&CEAnsi::ghLastAnsiCapable,
		&CEAnsi::ghLastAnsiNotCapable,
		&CEAnsi::ghLastConOut,
		&ghLastConInHandle,
		&ghLastNotConInHandle,
		NULL
	};

	if (hObject)
	{
		for (INT_PTR i = 0; hh[i]; i++)
		{
			if (hh[i] && (*(hh[i]) == hObject))
			{
				*(hh[i]) = NULL;
			}
		}
	}

	if (gpAnnotationHeader && (hObject == (HANDLE)gpAnnotationHeader))
	{
		lbRc = TRUE;
	}
	else
	{
		lbRc = F(CloseHandle)(hObject);
	}

	if (ghSkipSetThreadContextForThread == hObject)
		ghSkipSetThreadContextForThread = NULL;

	return lbRc;
}

BOOL WINAPI OnGetModuleHandleExW(DWORD dwFlags, LPCWSTR lpModuleName, HMODULE *phModule)
{
	typedef BOOL (WINAPI* OnGetModuleHandleExW_t)(DWORD dwFlags, LPCWSTR lpModuleName, HMODULE *phModule);
	ORIGINALFAST(GetModuleHandleExW);
	BOOL lbRc = FALSE;
	LPCWSTR lpModuleName2 = lpModuleName;

	// Issue 1899: Java uses following code
	//		GetModuleHandleExW((GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT),
	//			(LPCWSTR)&CreateFileW, &h)
	// which caused NULL result because CreateFileW was hooked
	if ((dwFlags & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS) != 0)
	{
		HookItem* ph = NULL;
		void* ptrOldProc = GetOriginalAddress((FARPROC)lpModuleName, NULL, FALSE, &ph);
		if (ptrOldProc)
		{
			lpModuleName2 = (LPCWSTR)ptrOldProc;
		}
	}

	lbRc = F(GetModuleHandleExW)(dwFlags, lpModuleName2, phModule);
	return lbRc;
}

BOOL WINAPI OnSetCurrentDirectoryA(LPCSTR lpPathName)
{
	typedef BOOL (WINAPI* OnSetCurrentDirectoryA_t)(LPCSTR lpPathName);
	ORIGINALFAST(SetCurrentDirectoryA);
	BOOL bMainThread = FALSE; // поток не важен
	BOOL lbRc = FALSE;

	lbRc = F(SetCurrentDirectoryA)(lpPathName);

	gbCurDirChanged |= (lbRc != FALSE);

	return lbRc;
}

BOOL WINAPI OnSetCurrentDirectoryW(LPCWSTR lpPathName)
{
	typedef BOOL (WINAPI* OnSetCurrentDirectoryW_t)(LPCWSTR lpPathName);
	ORIGINALFAST(SetCurrentDirectoryW);
	BOOL bMainThread = FALSE; // поток не важен
	BOOL lbRc = FALSE;

	lbRc = F(SetCurrentDirectoryW)(lpPathName);

	gbCurDirChanged |= (lbRc != FALSE);

	return lbRc;
}

BOOL WINAPI OnSetThreadContext(HANDLE hThread, CONST CONTEXT *lpContext)
{
	typedef BOOL (WINAPI* OnSetThreadContext_t)(HANDLE hThread, CONST CONTEXT *lpContext);
	ORIGINALFAST(SetThreadContext);
	//BOOL bMainThread = FALSE; // поток не важен
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

BOOL WINAPI OnTrackPopupMenu(HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, CONST RECT * prcRect)
{
	typedef BOOL (WINAPI* OnTrackPopupMenu_t)(HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, CONST RECT * prcRect);
	ORIGINALFASTEX(TrackPopupMenu,NULL);
#ifndef CONEMU_MINIMAL
	WCHAR szMsg[128]; msprintf(szMsg, SKIPLEN(countof(szMsg)) L"TrackPopupMenu(hwnd=0x%08X)\n", (DWORD)hWnd);
	DebugString(szMsg);
#endif

	// Seems like that is required(?) for Far's EMenu only
	// and can harm other applications: gh#112
	if (ghConEmuWndDC && (gFarMode.cbSize == sizeof(gFarMode)) && gFarMode.bFarHookMode)
	{
		// We have to ensure that hWnd is on top (has focus) because menu expects that
		GuiSetForeground(hWnd);

		// Far Manager related (EMenu especially)
		if (gFarMode.cbSize == sizeof(gFarMode) && gFarMode.bPopupMenuPos)
		{
			gFarMode.bPopupMenuPos = FALSE; // one time
			POINT pt; GetCursorPos(&pt);
			x = pt.x; y = pt.y;
		}
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

		if (gFarMode.cbSize == sizeof(gFarMode) && gFarMode.bPopupMenuPos)
		{
			gFarMode.bPopupMenuPos = FALSE; // однократно
			POINT pt; GetCursorPos(&pt);
			x = pt.x; y = pt.y;
		}
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
	if (!sp || !sp->OnShellExecuteExA(&lpExecInfo))
	{
		delete sp;
		SetLastError(ERROR_FILE_NOT_FOUND);
		return FALSE;
	}

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

HRESULT OurShellExecCmdLine(HWND hwnd, LPCWSTR pwszCommand, LPCWSTR pwszStartDir, bool bRunAsAdmin, bool bForce)
{
	HRESULT hr = E_UNEXPECTED;
	BOOL bShell = FALSE;

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
			goto wrap;
		}
		if (nCheckSybsystem1 != IMAGE_SUBSYSTEM_WINDOWS_CUI)
		{
			hr = (HRESULT)-1;
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

// Issue 1125: Hook undocumented function used for running commands typed in Windows 7 Win-menu search string.
HRESULT WINAPI OnShellExecCmdLine(HWND hwnd, LPCWSTR pwszCommand, LPCWSTR pwszStartDir, int nShow, LPVOID pUnused, DWORD dwSeclFlags)
{
	typedef BOOL (WINAPI* OnShellExecCmdLine_t)(HWND hwnd, LPCWSTR pwszCommand, LPCWSTR pwszStartDir, int nShow, LPVOID pUnused, DWORD dwSeclFlags);
	ORIGINALFASTEX(ShellExecCmdLine,NULL);
	HRESULT hr = S_OK;

	// This is used from "Run" dialog too. We need to process command internally, because
	// otherwise Win can pass CREATE_SUSPENDED into CreateProcessW, so console will flickers.
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

	_ASSERTRESULT(lbRc);
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


BOOL WINAPI OnShowCursor(BOOL bShow)
{
	typedef BOOL (WINAPI* OnShowCursor_t)(BOOL bShow);
	ORIGINALFASTEX(ShowCursor,NULL);
	BOOL bRc = FALSE;

	if (gbIsMinTtyProcess)
	{
		if (!bShow)
		{
			// Issue 888: ConEmu-Mintty: Mouse Cursor is hidden after application switch
			// _ASSERTEX(bShow!=FALSE);
			bShow = TRUE; // Disallow cursor hiding
		}
	}

	if (F(ShowCursor))
	{
		bRc = F(ShowCursor)(bShow);
	}

	return bRc;
}

HWND WINAPI OnSetFocus(HWND hWnd)
{
	typedef HWND (WINAPI* OnSetFocus_t)(HWND hWnd);
	ORIGINALFASTEX(SetFocus,NULL);
	HWND hRet = NULL;
	DWORD nPID = 0, nTID = 0;

	if (ghAttachGuiClient && ((nTID = GetWindowThreadProcessId(hWnd, &nPID)) != 0))
	{
		ghAttachGuiFocused = (nPID == GetCurrentProcessId()) ? hWnd : NULL;
	}

	if (F(SetFocus))
		hRet = F(SetFocus)(hWnd);

	return hRet;
}

BOOL WINAPI OnShowWindow(HWND hWnd, int nCmdShow)
{
	typedef BOOL (WINAPI* OnShowWindow_t)(HWND hWnd, int nCmdShow);
	ORIGINALFASTEX(ShowWindow,NULL);
	BOOL lbRc = FALSE, lbGuiAttach = FALSE, lbInactiveTab = FALSE;
	static bool bShowWndCalled = false;

	if (gbPrepareDefaultTerminal && ghConWnd && (hWnd == ghConWnd) && nCmdShow && (gnVsHostStartConsole > 0))
	{
		DefTermMsg(L"ShowWindow(hConWnd) calling");
		nCmdShow = SW_HIDE;
		gnVsHostStartConsole--;
	}

	if (ghConEmuWndDC && (hWnd == ghConEmuWndDC || hWnd == ghConWnd))
	{
		#ifdef _DEBUG
		if (hWnd == ghConEmuWndDC)
		{
			static bool bShowWarned = false;
			if (!bShowWarned) { bShowWarned = true; _ASSERTE(hWnd != ghConEmuWndDC && L"OnShowWindow(ghConEmuWndDC)"); }
		}
		else
		{
			static bool bShowWarned = false;
			if (!bShowWarned) { bShowWarned = true; _ASSERTE(hWnd != ghConEmuWndDC && L"OnShowWindow(ghConWnd)"); }
		}
		#endif
		return TRUE; // обманем
	}

	if ((ghAttachGuiClient == hWnd) || (!ghAttachGuiClient && gbAttachGuiClient))
		OnShowGuiClientWindow(hWnd, nCmdShow, lbGuiAttach, lbInactiveTab);

	if (F(ShowWindow))
	{
		lbRc = F(ShowWindow)(hWnd, nCmdShow);
		// Первый вызов может быть обломным, из-за того, что корневой процесс
		// запускается с wShowCmd=SW_HIDE (чтобы не мелькал)
		if (!bShowWndCalled)
		{
			bShowWndCalled = true;
			if (!lbRc && nCmdShow && !IsWindowVisible(hWnd))
			{
				F(ShowWindow)(hWnd, nCmdShow);
			}
		}

		// Если вкладка НЕ активная - то вернуть фокус в ConEmu
		if (lbGuiAttach && lbInactiveTab && nCmdShow && ghConEmuWnd)
		{
			SetForegroundWindow(ghConEmuWnd);
		}
	}
	DWORD dwErr = GetLastError();

	if (lbGuiAttach)
		OnPostShowGuiClientWindow(hWnd, nCmdShow);

	SetLastError(dwErr);
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

	//if (ghConEmuWndDC && hWnd == ghConEmuWndDC)
	//{
	//	hWnd = ghConEmuWnd;
	//}
	if (ghConEmuWndDC)
	{
		if (ghAttachGuiClient)
		{
			if (hWnd == ghAttachGuiClient || hWnd == ghConEmuWndDC)
			{
				// Обмануть GUI-клиента, пусть он думает, что он "сверху"
				hWnd = ghConEmuWnd;
			}
		}
		else if (hWnd == ghConEmuWndDC)
		{
			hWnd = ghConEmuWnd;
		}
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

	if (ghConEmuWndDC)
	{
		if (ghAttachGuiClient)
		{
			if ((hWnd == ghAttachGuiClient || hWnd == ghConEmuWndDC) && (uCmd == GW_OWNER))
			{
				hWnd = ghConEmuWnd;
			}
		}
		else if ((hWnd == ghConEmuWndDC) && (uCmd == GW_OWNER))
		{
			hWnd = ghConEmuWnd;
		}
	}

	if (F(GetWindow))
	{
		lhRc = F(GetWindow)(hWnd, uCmd);

		if (ghAttachGuiClient && (uCmd == GW_OWNER) && (lhRc == ghConEmuWndDC))
		{
			_ASSERTE(lhRc != ghConEmuWndDC);
			lhRc = ghAttachGuiClient;
		}
	}

	_ASSERTRESULT(lhRc!=NULL);
	return lhRc;
}


HWND WINAPI OnGetAncestor(HWND hWnd, UINT gaFlags)
{
	typedef HWND (WINAPI* OnGetAncestor_t)(HWND hWnd, UINT gaFlags);
	ORIGINALFASTEX(GetAncestor,NULL);
	HWND lhRc = NULL;

	#ifdef LOG_GETANCESTOR
	if (ghAttachGuiClient)
	{
		wchar_t szInfo[1024];
		getWindowInfo(hWnd, szInfo);
		lstrcat(szInfo, L"\n");
		DebugString(szInfo);
	}
	#endif

	//if (ghConEmuWndDC && hWnd == ghConEmuWndDC)
	//{
	//	hWnd = ghConEmuWnd;
	//}
	if (ghConEmuWndDC)
	{
		#ifdef _DEBUG
		if ((GetKeyState(VK_CAPITAL) & 1))
		{
			int nDbg = 0;
		}
		#endif

		if (ghAttachGuiClient)
		{
			// Обмануть GUI-клиента, пусть он думает, что он "сверху"
			if (hWnd == ghAttachGuiClient || hWnd == ghConEmuWndDC)
			{
				hWnd = ghConEmuWnd;
			}
			#if 0
			else
			{
				wchar_t szName[255];
				GetClassName(hWnd, szName, countof(szName));
				if (wcsncmp(szName, L"WindowsForms", 12) == 0)
				{
					GetWindowText(hWnd, szName, countof(szName));
					if (wcsncmp(szName, L"toolStrip", 8) == 0 || wcsncmp(szName, L"menuStrip", 8) == 0)
					{
						hWnd = ghConEmuWndDC;
					}
				}
			}
			#endif
		}
		else if (hWnd == ghConEmuWndDC)
		{
			hWnd = ghConEmuWnd;
		}
	}

	if (F(GetAncestor))
	{
		lhRc = F(GetAncestor)(hWnd, gaFlags);

		if (ghAttachGuiClient && (gaFlags == GA_ROOTOWNER || gaFlags == GA_ROOT) && lhRc == ghConEmuWnd)
		{
			lhRc = ghAttachGuiClient;
		}
	}

	_ASSERTRESULT(lhRc);
	return lhRc;
}

//Issue 469: Some programs requires "ConsoleWindowClass" for GetConsoleWindow
int WINAPI OnGetClassNameA(HWND hWnd, LPSTR lpClassName, int nMaxCount)
{
	typedef int (WINAPI *OnGetClassNameA_t)(HWND hWnd, LPSTR lpClassName, int nMaxCount);
	ORIGINALFASTEX(GetClassNameA,NULL);
	int iRc = 0;
	if (ghConEmuWndDC && hWnd == ghConEmuWndDC && lpClassName)
	{
		//lstrcpynA(lpClassName, RealConsoleClass, nMaxCount);
		WideCharToMultiByte(CP_ACP, 0, RealConsoleClass, -1, lpClassName, nMaxCount, 0,0);
		iRc = lstrlenA(lpClassName);
	}
	else if (F(GetClassNameA))
		iRc = F(GetClassNameA)(hWnd, lpClassName, nMaxCount);
	return iRc;
}
int WINAPI OnGetClassNameW(HWND hWnd, LPWSTR lpClassName, int nMaxCount)
{
	typedef int (WINAPI *OnGetClassNameW_t)(HWND hWnd, LPWSTR lpClassName, int nMaxCount);
	ORIGINALFASTEX(GetClassNameW,NULL);
	int iRc = 0;
	if (ghConEmuWndDC && hWnd == ghConEmuWndDC && lpClassName)
	{
		lstrcpynW(lpClassName, RealConsoleClass, nMaxCount);
		iRc = lstrlenW(lpClassName);
	}
	else if (F(GetClassNameW))
		iRc = F(GetClassNameW)(hWnd, lpClassName, nMaxCount);
	return iRc;
}

HWND WINAPI OnGetActiveWindow()
{
	typedef HWND (WINAPI* OnGetActiveWindow_t)();
	ORIGINALFASTEX(GetActiveWindow,NULL);
	HWND hWnd = NULL;

	if (F(GetActiveWindow))
		hWnd = F(GetActiveWindow)();

	#if 1
	if (ghAttachGuiClient)
	{
		if (hWnd == ghConEmuWnd || hWnd == ghConEmuWndDC)
			hWnd = ghAttachGuiClient;
		else if (hWnd == NULL)
			hWnd = ghAttachGuiClient;
	}
	#endif

	return hWnd;
}

HWND WINAPI OnGetForegroundWindow()
{
	typedef HWND (WINAPI* OnGetForegroundWindow_t)();
	ORIGINALFASTEX(GetForegroundWindow,NULL);

	HWND hFore = NULL;
	if (F(GetForegroundWindow))
		hFore = F(GetForegroundWindow)();

	if (ghConEmuWndDC)
	{
		if (ghAttachGuiClient && ((hFore == ghConEmuWnd) || (hFore == ghConEmuWndDC)))
		{
			// Обмануть GUI-клиента, пусть он думает, что он "сверху"
			hFore = ghAttachGuiClient;
		}
		else if (hFore == ghConEmuWnd)
		{
			hFore = ghConEmuWndDC;
		}
	}

	return hFore;
}

BOOL WINAPI OnSetForegroundWindow(HWND hWnd)
{
	typedef BOOL (WINAPI* OnSetForegroundWindow_t)(HWND hWnd);
	ORIGINALFASTEX(SetForegroundWindow,NULL);

	BOOL lbRc = FALSE;

	if (ghConEmuWndDC)
	{
		if (hWnd == ghConEmuWndDC)
			hWnd = ghConEmuWnd;
		lbRc = GuiSetForeground(hWnd);
	}

	// ConEmu наверное уже все сделал, но на всякий случай, дернем и здесь
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

BOOL WINAPI OnSetMenu(HWND hWnd, HMENU hMenu)
{
	typedef BOOL (WINAPI* OnSetMenu_t)(HWND hWnd, HMENU hMenu);
	ORIGINALFASTEX(SetMenu,NULL);

	BOOL lbRc = FALSE;

	if (hMenu && ghAttachGuiClient && hWnd == ghAttachGuiClient)
	{
		if ((gnAttachGuiClientFlags & (agaf_WS_CHILD|agaf_NoMenu|agaf_DotNet)) == (agaf_WS_CHILD|agaf_NoMenu))
		{
			gnAttachGuiClientFlags &= ~(agaf_WS_CHILD|agaf_NoMenu);
			DWORD_PTR dwStyle = GetWindowLongPtr(ghAttachGuiClient, GWL_STYLE);
			DWORD_PTR dwNewStyle = (dwStyle & ~WS_CHILD) | (gnAttachGuiClientStyle & WS_POPUP);
			if (dwStyle != dwNewStyle)
			{
				SetWindowLongPtr(ghAttachGuiClient, GWL_STYLE, dwNewStyle);
			}
		}
	}

	if (F(SetMenu) != NULL)
	{
		lbRc = F(SetMenu)(hWnd, hMenu);
	}

	return lbRc;
}

BOOL WINAPI OnMoveWindow(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint)
{
	typedef BOOL (WINAPI* OnMoveWindow_t)(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint);
	ORIGINALFASTEX(MoveWindow,NULL);
	BOOL lbRc = FALSE;

	if (ghConEmuWndDC && (hWnd == ghConEmuWndDC || hWnd == ghConEmuWnd))
		return TRUE; // обманем. приложениям запрещено "двигать" ConEmuDC

	if (ghConEmuWndDC && ghAttachGuiClient && hWnd == ghAttachGuiClient)
	{
		// GUI приложениями запрещено самостоятельно двигаться внутри ConEmu
		OnSetGuiClientWindowPos(hWnd, NULL, X, Y, nWidth, nHeight, 0);
	}

	if (F(MoveWindow))
		lbRc = F(MoveWindow)(hWnd, X, Y, nWidth, nHeight, bRepaint);

	return lbRc;
}

LONG WINAPI OnSetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong)
{
	typedef LONG (WINAPI* OnSetWindowLongA_t)(HWND hWnd, int nIndex, LONG dwNewLong);
	ORIGINALFASTEX(SetWindowLongA,NULL);
	LONG lRc = 0;

	if (ghConEmuWndDC && (hWnd == ghConEmuWndDC || hWnd == ghConEmuWnd))
	{
		_ASSERTRESULT(FALSE);
		SetLastError(ERROR_INVALID_HANDLE);
		lRc = 0; // обманем. приложениям запрещено менять ConEmuDC
	}
	else if (F(SetWindowLongA))
	{
		lRc = F(SetWindowLongA)(hWnd, nIndex, dwNewLong);
	}

	return lRc;
}
LONG WINAPI OnSetWindowLongW(HWND hWnd, int nIndex, LONG dwNewLong)
{
	typedef LONG (WINAPI* OnSetWindowLongW_t)(HWND hWnd, int nIndex, LONG dwNewLong);
	ORIGINALFASTEX(SetWindowLongW,NULL);
	LONG lRc = 0;

	if (ghConEmuWndDC && (hWnd == ghConEmuWndDC || hWnd == ghConEmuWnd))
	{
		_ASSERTRESULT(FALSE);
		SetLastError(ERROR_INVALID_HANDLE);
		lRc = 0; // обманем. приложениям запрещено менять ConEmuDC
	}
	else if (F(SetWindowLongW))
	{
		lRc = F(SetWindowLongW)(hWnd, nIndex, dwNewLong);
	}

	return lRc;
}
#ifdef WIN64
LONG_PTR WINAPI OnSetWindowLongPtrA(HWND hWnd, int nIndex, LONG_PTR dwNewLong)
{
	typedef LONG_PTR (WINAPI* OnSetWindowLongPtrA_t)(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
	ORIGINALFASTEX(SetWindowLongPtrA,NULL);
	LONG_PTR lRc = 0;

	if (ghConEmuWndDC && (hWnd == ghConEmuWndDC || hWnd == ghConEmuWnd))
	{
		_ASSERTRESULT(FALSE);
		SetLastError(ERROR_INVALID_HANDLE);
		lRc = 0; // обманем. приложениям запрещено менять ConEmuDC
	}
	else if (F(SetWindowLongPtrA))
	{
		lRc = F(SetWindowLongPtrA)(hWnd, nIndex, dwNewLong);
	}

	return lRc;
}
LONG_PTR WINAPI OnSetWindowLongPtrW(HWND hWnd, int nIndex, LONG_PTR dwNewLong)
{
	typedef LONG_PTR (WINAPI* OnSetWindowLongPtrW_t)(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
	ORIGINALFASTEX(SetWindowLongPtrW,NULL);
	LONG_PTR lRc = 0;

	if (ghConEmuWndDC && (hWnd == ghConEmuWndDC || hWnd == ghConEmuWnd))
	{
		_ASSERTRESULT(FALSE);
		SetLastError(ERROR_INVALID_HANDLE);
		lRc = 0; // обманем. приложениям запрещено менять ConEmuDC
	}
	else if (F(SetWindowLongPtrW))
	{
		lRc = F(SetWindowLongPtrW)(hWnd, nIndex, dwNewLong);
	}

	return lRc;
}
#endif

void FixHwnd4ConText(HWND& hWnd)
{
	if (ghConEmuWndDC && (hWnd == ghConEmuWndDC || hWnd == ghConEmuWnd))
		hWnd = ghConWnd;
}

int WINAPI OnGetWindowTextLengthA(HWND hWnd)
{
	typedef int (WINAPI* OnGetWindowTextLengthA_t)(HWND hWnd);
	ORIGINALFASTEX(GetWindowTextLengthA,NULL);
	int iRc = 0;

	FixHwnd4ConText(hWnd);

	if (F(GetWindowTextLengthA))
		iRc = F(GetWindowTextLengthA)(hWnd);

	return iRc;
}
int WINAPI OnGetWindowTextLengthW(HWND hWnd)
{
	typedef int (WINAPI* OnGetWindowTextLengthW_t)(HWND hWnd);
	ORIGINALFASTEX(GetWindowTextLengthW,NULL);
	int iRc = 0;

	FixHwnd4ConText(hWnd);

	if (F(GetWindowTextLengthW))
		iRc = F(GetWindowTextLengthW)(hWnd);

	return iRc;
}
int WINAPI OnGetWindowTextA(HWND hWnd, LPSTR lpString, int nMaxCount)
{
	typedef int (WINAPI* OnGetWindowTextA_t)(HWND hWnd, LPSTR lpString, int nMaxCount);
	ORIGINALFASTEX(GetWindowTextA,NULL);
	int iRc = 0;

	FixHwnd4ConText(hWnd);

	if (F(GetWindowTextA))
		iRc = F(GetWindowTextA)(hWnd, lpString, nMaxCount);

	return iRc;
}
int WINAPI OnGetWindowTextW(HWND hWnd, LPWSTR lpString, int nMaxCount)
{
	typedef int (WINAPI* OnGetWindowTextW_t)(HWND hWnd, LPWSTR lpString, int nMaxCount);
	ORIGINALFASTEX(GetWindowTextW,NULL);
	int iRc = 0;

	FixHwnd4ConText(hWnd);

	if (F(GetWindowTextW))
		iRc = F(GetWindowTextW)(hWnd, lpString, nMaxCount);

	#ifdef DEBUG_CON_TITLE
	wchar_t szPrefix[32]; _wsprintf(szPrefix, SKIPCOUNT(szPrefix) L"GetWindowTextW(x%08X)='", (DWORD)(DWORD_PTR)hWnd);
	CEStr lsDbg(lstrmerge(szPrefix, lpString, L"'\n"));
	OutputDebugString(lsDbg);
	if (gFarMode.cbSize && lpString && gpLastSetConTitle && gpLastSetConTitle->ms_Arg)
	{
		int iCmp = lstrcmp(gpLastSetConTitle->ms_Arg, lpString);
		_ASSERTE((iCmp == 0) && "Console window title was changed outside or was not applied yet");
	}
	#endif

	return iRc;
}
// Не перехватываются пока, для информации
BOOL WINAPI OnSetConsoleTitleA(LPCSTR lpConsoleTitle)
{
	typedef BOOL (WINAPI* OnSetConsoleTitleA_t)(LPCSTR lpConsoleTitle);
	ORIGINALFASTEX(SetConsoleTitleA,NULL);
	BOOL bRc = FALSE;
	if (F(SetConsoleTitleA))
		bRc = F(SetConsoleTitleA)(lpConsoleTitle);
	return bRc;
}
BOOL WINAPI OnSetConsoleTitleW(LPCWSTR lpConsoleTitle)
{
	typedef BOOL (WINAPI* OnSetConsoleTitleW_t)(LPCWSTR lpConsoleTitle);
	ORIGINALFASTEX(SetConsoleTitleW,NULL);

	#ifdef DEBUG_CON_TITLE
	if (!gpLastSetConTitle)
		gpLastSetConTitle = new CEStr(lstrdup(lpConsoleTitle));
	else
		gpLastSetConTitle->Set(lpConsoleTitle);
	CEStr lsDbg(lstrmerge(L"SetConsoleTitleW('", lpConsoleTitle, L"')\n"));
	OutputDebugString(lsDbg);
	#endif

	BOOL bRc = FALSE;
	if (F(SetConsoleTitleW))
		bRc = F(SetConsoleTitleW)(lpConsoleTitle);
	return bRc;
}


BOOL WINAPI OnSetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
	typedef BOOL (WINAPI* OnSetWindowPos_t)(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);
	ORIGINALFASTEX(SetWindowPos,NULL);
	BOOL lbRc = FALSE;

	if (ghConEmuWndDC && (hWnd == ghConEmuWndDC || hWnd == ghConEmuWnd))
	{
		_ASSERTRESULT(FALSE);
		return TRUE; // обманем. приложениям запрещено "двигать" ConEmuDC
	}

	if (ghConEmuWndDC && ghAttachGuiClient && hWnd == ghAttachGuiClient)
	{
		// GUI приложениями запрещено самостоятельно двигаться внутри ConEmu
		OnSetGuiClientWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
	}

	if (F(SetWindowPos))
		lbRc = F(SetWindowPos)(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);

	_ASSERTRESULT(lbRc);
	return lbRc;
}

BOOL WINAPI OnGetWindowPlacement(HWND hWnd, WINDOWPLACEMENT *lpwndpl)
{
	typedef BOOL (WINAPI* OnGetWindowPlacement_t)(HWND hWnd, WINDOWPLACEMENT *lpwndpl);
	ORIGINALFASTEX(GetWindowPlacement,NULL);
	BOOL lbRc = FALSE;

	if (F(GetWindowPlacement))
		lbRc = F(GetWindowPlacement)(hWnd, lpwndpl);

	if (lbRc && ghConEmuWndDC && !gbGuiClientExternMode && ghAttachGuiClient
		&& hWnd == ghAttachGuiClient)
	{
		MapWindowPoints(ghConEmuWndDC, NULL, (LPPOINT)&lpwndpl->rcNormalPosition, 2);
	}

	return lbRc;
}

BOOL WINAPI OnSetWindowPlacement(HWND hWnd, WINDOWPLACEMENT *lpwndpl)
{
	typedef BOOL (WINAPI* OnSetWindowPlacement_t)(HWND hWnd, WINDOWPLACEMENT *lpwndpl);
	ORIGINALFASTEX(SetWindowPlacement,NULL);
	BOOL lbRc = FALSE;
	WINDOWPLACEMENT wpl = {sizeof(wpl)};

	if (ghConEmuWndDC && (hWnd == ghConEmuWndDC || hWnd == ghConEmuWnd))
		return TRUE; // обманем. приложениям запрещено "двигать" ConEmuDC

	if (lpwndpl && ghConEmuWndDC && ghAttachGuiClient && !gbGuiClientExternMode && hWnd == ghAttachGuiClient)
	{
		// GUI приложениями запрещено самостоятельно двигаться внутри ConEmu
		int X, Y, cx, cy;
		if (OnSetGuiClientWindowPos(hWnd, NULL, X, Y, cx, cy, 0))
		{
			wpl.showCmd = SW_RESTORE;
			wpl.rcNormalPosition.left = X;
			wpl.rcNormalPosition.top = Y;
			wpl.rcNormalPosition.right = X + cx;
			wpl.rcNormalPosition.bottom = Y + cy;
			wpl.ptMinPosition = lpwndpl->ptMinPosition;
			wpl.ptMaxPosition = lpwndpl->ptMaxPosition;
			lpwndpl = &wpl;
		}
	}

	if (F(SetWindowPlacement))
		lbRc = F(SetWindowPlacement)(hWnd, lpwndpl);

	return lbRc;
}

VOID WINAPI Onmouse_event(DWORD dwFlags, DWORD dx, DWORD dy, DWORD dwData, ULONG_PTR dwExtraInfo)
{
	typedef VOID (WINAPI* Onmouse_event_t)(DWORD dwFlags, DWORD dx, DWORD dy, DWORD dwData, ULONG_PTR dwExtraInfo);
	ORIGINALFASTEX(mouse_event,NULL);

	F(mouse_event)(dwFlags, dx, dy, dwData, dwExtraInfo);
}

UINT WINAPI OnSendInput(UINT nInputs, LPINPUT pInputs, int cbSize)
{
	typedef UINT (WINAPI* OnSendInput_t)(UINT nInputs, LPINPUT pInputs, int cbSize);
	ORIGINALFASTEX(SendInput,NULL);
	UINT nRc;

	nRc = F(SendInput)(nInputs, pInputs, cbSize);

	return nRc;
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

//void PatchGuiMessage(LPMSG lpMsg)
//{
//	if (!ghAttachGuiClient)
//		return;
//
//	PatchGuiMessage(lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);
//}

BOOL WINAPI OnPostMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	typedef BOOL (WINAPI* OnPostMessageA_t)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	ORIGINALFASTEX(PostMessageA,NULL);
	BOOL lRc = 0;
	LRESULT ll = 0;

	if (!CanSendMessage(hWnd, Msg, wParam, lParam, ll))
		return TRUE; // Обманем, это сообщение запрещено посылать в ConEmuDC

	if (ghAttachGuiClient)
		PatchGuiMessage(false, hWnd, Msg, wParam, lParam);

	if (F(PostMessageA))
		lRc = F(PostMessageA)(hWnd, Msg, wParam, lParam);

	return lRc;
}
BOOL WINAPI OnPostMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	typedef BOOL (WINAPI* OnPostMessageW_t)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	ORIGINALFASTEX(PostMessageW,NULL);
	BOOL lRc = 0;
	LRESULT ll = 0;

	if (!CanSendMessage(hWnd, Msg, wParam, lParam, ll))
		return TRUE; // Обманем, это сообщение запрещено посылать в ConEmuDC

	if (ghAttachGuiClient)
		PatchGuiMessage(false, hWnd, Msg, wParam, lParam);

	if (F(PostMessageW))
		lRc = F(PostMessageW)(hWnd, Msg, wParam, lParam);

	return lRc;
}
LRESULT WINAPI OnSendMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	typedef LRESULT (WINAPI* OnSendMessageA_t)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	ORIGINALFASTEX(SendMessageA,NULL);
	LRESULT lRc = 0;

	if (!CanSendMessage(hWnd, Msg, wParam, lParam, lRc))
		return lRc; // Обманем, это сообщение запрещено посылать в ConEmuDC

	if (ghAttachGuiClient)
		PatchGuiMessage(false, hWnd, Msg, wParam, lParam);

	if (F(SendMessageA))
		lRc = F(SendMessageA)(hWnd, Msg, wParam, lParam);

	return lRc;
}
LRESULT WINAPI OnSendMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	typedef LRESULT (WINAPI* OnSendMessageW_t)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	ORIGINALFASTEX(SendMessageW,NULL);
	LRESULT lRc = 0;

	if (!CanSendMessage(hWnd, Msg, wParam, lParam, lRc))
		return lRc; // Обманем, это сообщение запрещено посылать в ConEmuDC

	if (ghAttachGuiClient)
		PatchGuiMessage(false, hWnd, Msg, wParam, lParam);

	if (F(SendMessageW))
		lRc = F(SendMessageW)(hWnd, Msg, wParam, lParam);

	return lRc;
}

BOOL WINAPI OnGetMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
{
	typedef BOOL (WINAPI* OnGetMessageA_t)(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
	ORIGINALFASTEX(GetMessageA,NULL);
	BOOL lRc = 0;

	if (F(GetMessageA))
		lRc = F(GetMessageA)(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);

	if (lRc && ghAttachGuiClient)
		PatchGuiMessage(true, lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);

	return lRc;
}
BOOL WINAPI OnGetMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
{
	typedef BOOL (WINAPI* OnGetMessageW_t)(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
	ORIGINALFASTEX(GetMessageW,NULL);
	BOOL lRc = 0;

	if (F(GetMessageW))
		lRc = F(GetMessageW)(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);

	if (lRc && ghAttachGuiClient)
		PatchGuiMessage(true, lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);

	_ASSERTRESULT(TRUE);
	return lRc;
}
BOOL WINAPI OnPeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
{
	typedef BOOL (WINAPI* OnPeekMessageA_t)(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
	ORIGINALFASTEX(PeekMessageA,NULL);
	BOOL lRc = 0;

	if (F(PeekMessageA))
		lRc = F(PeekMessageA)(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);

	if (lRc && ghAttachGuiClient)
		PatchGuiMessage(true, lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);

	return lRc;
}
BOOL WINAPI OnPeekMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
{
	typedef BOOL (WINAPI* OnPeekMessageW_t)(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
	ORIGINALFASTEX(PeekMessageW,NULL);
	BOOL lRc = 0;

	if (F(PeekMessageW))
		lRc = F(PeekMessageW)(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);

	if (lRc && ghAttachGuiClient)
		PatchGuiMessage(true, lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);

	return lRc;
}

HWND WINAPI OnCreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	typedef HWND (WINAPI* OnCreateWindowExA_t)(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
	ORIGINALFASTEX(CreateWindowExA,NULL);
	HWND hWnd = NULL;
	BOOL bAttachGui = FALSE, bStyleHidden = FALSE;
	DWORD lStyle = dwStyle, lStyleEx = dwExStyle;

	if (CheckCanCreateWindow(lpClassName, NULL, dwStyle, dwExStyle, hWndParent, bAttachGui, bStyleHidden)
		&& F(CreateWindowExA) != NULL)
	{
		if (bAttachGui)
		{
			if ((grcConEmuClient.right > grcConEmuClient.left) && (grcConEmuClient.bottom > grcConEmuClient.top))
			{
				x = grcConEmuClient.left; y = grcConEmuClient.top;
				nWidth = grcConEmuClient.right - grcConEmuClient.left; nHeight = grcConEmuClient.bottom - grcConEmuClient.top;
			}
			else
			{
				_ASSERTEX((grcConEmuClient.right > grcConEmuClient.left) && (grcConEmuClient.bottom > grcConEmuClient.top));
			}
		}

		hWnd = F(CreateWindowExA)(dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
		DWORD dwErr = GetLastError();

		if (hWnd && bAttachGui)
		{
			OnGuiWindowAttached(hWnd, hMenu, lpClassName, NULL, lStyle, lStyleEx, bStyleHidden);

			SetLastError(dwErr);
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

	if (CheckCanCreateWindow(NULL, lpClassName, dwStyle, dwExStyle, hWndParent, bAttachGui, bStyleHidden)
		&& F(CreateWindowExW) != NULL)
	{
		if (bAttachGui)
		{
			x = grcConEmuClient.left; y = grcConEmuClient.top;
			nWidth = grcConEmuClient.right - grcConEmuClient.left; nHeight = grcConEmuClient.bottom - grcConEmuClient.top;
		}

		hWnd = F(CreateWindowExW)(dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
		DWORD dwErr = GetLastError();

		if (hWnd && bAttachGui)
		{
			OnGuiWindowAttached(hWnd, hMenu, NULL, lpClassName, lStyle, lStyleEx, bStyleHidden);

			SetLastError(dwErr);
		}
	}

	_ASSERTRESULT(hWnd!=NULL);
	return hWnd;
}

HWND WINAPI OnCreateDialogIndirectParamA(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lParamInit)
{
	typedef HWND (WINAPI* OnCreateDialogIndirectParamA_t)(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lParamInit);
	ORIGINALFASTEX(CreateDialogIndirectParamA,NULL);
	HWND hWnd = NULL;
	BOOL bAttachGui = (hWndParent == NULL), bStyleHidden = FALSE;
	// Со стилями - полная фигня сюда попадает
	DWORD lStyle = lpTemplate ? lpTemplate->style : 0;
	DWORD lStyleEx = lpTemplate ? lpTemplate->dwExtendedStyle : 0;

	if (/*CheckCanCreateWindow((LPCSTR)32770, NULL, lStyle, lStyleEx, hWndParent, bAttachGui, bStyleHidden)
		&&*/ F(CreateDialogIndirectParamA) != NULL)
	{
		//if (bAttachGui)
		//{
		//	x = grcConEmuClient.left; y = grcConEmuClient.top;
		//	nWidth = grcConEmuClient.right - grcConEmuClient.left; nHeight = grcConEmuClient.bottom - grcConEmuClient.top;
		//}

		hWnd = F(CreateDialogIndirectParamA)(hInstance, lpTemplate, hWndParent, lpDialogFunc, lParamInit);
		DWORD dwErr = GetLastError();

		if (hWnd && bAttachGui)
		{
			lStyle = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
			lStyleEx = (DWORD)GetWindowLongPtr(hWnd, GWL_EXSTYLE);
			if (CheckCanCreateWindow((LPCSTR)32770, NULL, lStyle, lStyleEx, hWndParent, bAttachGui, bStyleHidden) && bAttachGui)
			{
				OnGuiWindowAttached(hWnd, NULL, (LPCSTR)32770, NULL, lStyle, lStyleEx, bStyleHidden);
			}

			SetLastError(dwErr);
		}
	}

	return hWnd;
}

HWND WINAPI OnCreateDialogIndirectParamW(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lParamInit)
{
	typedef HWND (WINAPI* OnCreateDialogIndirectParamW_t)(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lParamInit);
	ORIGINALFASTEX(CreateDialogIndirectParamW,NULL);
	HWND hWnd = NULL;
	BOOL bAttachGui = (hWndParent == NULL), bStyleHidden = FALSE;
	// Со стилями - полная фигня сюда попадает
	DWORD lStyle = lpTemplate ? lpTemplate->style : 0;
	DWORD lStyleEx = lpTemplate ? lpTemplate->dwExtendedStyle : 0;

	if (/*CheckCanCreateWindow(NULL, (LPCWSTR)32770, lStyle, lStyleEx, hWndParent, bAttachGui, bStyleHidden)
		&&*/ F(CreateDialogIndirectParamW) != NULL)
	{
		//if (bAttachGui)
		//{
		//	x = grcConEmuClient.left; y = grcConEmuClient.top;
		//	nWidth = grcConEmuClient.right - grcConEmuClient.left; nHeight = grcConEmuClient.bottom - grcConEmuClient.top;
		//}

		hWnd = F(CreateDialogIndirectParamW)(hInstance, lpTemplate, hWndParent, lpDialogFunc, lParamInit);
		DWORD dwErr = GetLastError();

		if (hWnd && bAttachGui)
		{
			lStyle = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
			lStyleEx = (DWORD)GetWindowLongPtr(hWnd, GWL_EXSTYLE);
			if (CheckCanCreateWindow((LPCSTR)32770, NULL, lStyle, lStyleEx, hWndParent, bAttachGui, bStyleHidden) && bAttachGui)
			{
				OnGuiWindowAttached(hWnd, NULL, NULL, (LPCWSTR)32770, lStyle, lStyleEx, bStyleHidden);
			}

			SetLastError(dwErr);
		}
	}

	return hWnd;
}

HWND WINAPI OnCreateDialogParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	typedef HWND (WINAPI* OnCreateDialogParamA_t)(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
	ORIGINALFASTEX(CreateDialogParamA,NULL);
	HWND hWnd = NULL;
	BOOL bAttachGui = FALSE, bStyleHidden = FALSE;
	LPCDLGTEMPLATE lpTemplate = NULL;
	DWORD lStyle = 0; //lpTemplate ? lpTemplate->style : 0;
	DWORD lStyleEx = 0; //lpTemplate ? lpTemplate->dwExtendedStyle : 0;

	// Загрузить ресурс диалога, и глянуть его параметры lStyle/lStyleEx
	HRSRC hDlgSrc = FindResourceA(hInstance, lpTemplateName, /*RT_DIALOG*/MAKEINTRESOURCEA(5));
	if (hDlgSrc)
	{
		HGLOBAL hDlgHnd = LoadResource(hInstance, hDlgSrc);
		if (hDlgHnd)
		{
			lpTemplate = (LPCDLGTEMPLATE)LockResource(hDlgHnd);
			if (lpTemplate)
			{
				lStyle = lpTemplate ? lpTemplate->style : 0;
				lStyleEx = lpTemplate ? lpTemplate->dwExtendedStyle : 0;
			}
		}
	}

	if ((!lpTemplate || CheckCanCreateWindow((LPSTR)32770, NULL, lStyle, lStyleEx, hWndParent, bAttachGui, bStyleHidden))
		&& F(CreateDialogParamA) != NULL)
	{
		//if (bAttachGui)
		//{
		//	x = grcConEmuClient.left; y = grcConEmuClient.top;
		//	nWidth = grcConEmuClient.right - grcConEmuClient.left; nHeight = grcConEmuClient.bottom - grcConEmuClient.top;
		//}

		hWnd = F(CreateDialogParamA)(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
		DWORD dwErr = GetLastError();

		if (hWnd && bAttachGui)
		{
			OnGuiWindowAttached(hWnd, NULL, (LPCSTR)32770, NULL, lStyle, lStyleEx, bStyleHidden);

			SetLastError(dwErr);
		}
	}

	return hWnd;
}

HWND WINAPI OnCreateDialogParamW(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	typedef HWND (WINAPI* OnCreateDialogParamW_t)(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
	ORIGINALFASTEX(CreateDialogParamW,NULL);
	HWND hWnd = NULL;
	BOOL bAttachGui = FALSE, bStyleHidden = FALSE;
	LPCDLGTEMPLATE lpTemplate = NULL;
	DWORD lStyle = 0; //lpTemplate ? lpTemplate->style : 0;
	DWORD lStyleEx = 0; //lpTemplate ? lpTemplate->dwExtendedStyle : 0;

	// Загрузить ресурс диалога, и глянуть его параметры lStyle/lStyleEx
	HRSRC hDlgSrc = FindResourceW(hInstance, lpTemplateName, RT_DIALOG);
	if (hDlgSrc)
	{
		HGLOBAL hDlgHnd = LoadResource(hInstance, hDlgSrc);
		if (hDlgHnd)
		{
			lpTemplate = (LPCDLGTEMPLATE)LockResource(hDlgHnd);
			if (lpTemplate)
			{
				lStyle = lpTemplate ? lpTemplate->style : 0;
				lStyleEx = lpTemplate ? lpTemplate->dwExtendedStyle : 0;
			}
		}
	}

	if ((!lpTemplate || CheckCanCreateWindow(NULL, (LPWSTR)32770, lStyle, lStyleEx, hWndParent, bAttachGui, bStyleHidden))
		&& F(CreateDialogParamW) != NULL)
	{
		//if (bAttachGui)
		//{
		//	x = grcConEmuClient.left; y = grcConEmuClient.top;
		//	nWidth = grcConEmuClient.right - grcConEmuClient.left; nHeight = grcConEmuClient.bottom - grcConEmuClient.top;
		//}

		hWnd = F(CreateDialogParamW)(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
		DWORD dwErr = GetLastError();

		if (hWnd && bAttachGui)
		{
			OnGuiWindowAttached(hWnd, NULL, NULL, (LPCWSTR)32770, lStyle, lStyleEx, bStyleHidden);

			SetLastError(dwErr);
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
			DWORD nServerPID = gnServerPID;
			HWND hConWnd = GetConsoleWindow();
			_ASSERTE(hConWnd == ghConWnd);

			//MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> ConInfo;
			//ConInfo.InitName(CECONMAPNAME, (DWORD)hConWnd); //-V205
			//CESERVER_CONSOLE_MAPPING_HDR *pInfo = ConInfo.Open();
			//if (pInfo
			//	&& (pInfo->cbSize >= sizeof(CESERVER_CONSOLE_MAPPING_HDR))
			//	//&& (pInfo->nProtocolVersion == CESERVER_REQ_VER)
			//	)
			//{
			//	nServerPID = pInfo->nServerPID;
			//	ConInfo.CloseMap();
			//}

			if (nServerPID)
			{
				CESERVER_REQ_HDR In;
				ExecutePrepareCmd(&In, CECMD_GETALIASES, sizeof(CESERVER_REQ_HDR));
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
	HANDLE hThread = apiCreateThread(SetConsoleCPThread, &sco, &nTID, "OnSetConsoleCP(%u)", wCodePageID);

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
			apiTerminateThread(hThread,100);
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
			//	apiTerminateThread(hThread,100);
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
	HANDLE hThread = apiCreateThread(SetConsoleCPThread, &sco, &nTID, "OnSetConsoleOutputCP(%u)", wCodePageID);

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
			apiTerminateThread(hThread,100);
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
			//	apiTerminateThread(hThread,100);
			//if (GetConsoleOutputCP() == wCodePageID)
			//	lbRc = TRUE;
		}

		CloseHandle(hThread);
	}

	if (sco.hReady)
		CloseHandle(sco.hReady);

	return lbRc;
}


void OnReadConsoleStart(bool bUnicode, HANDLE hConsoleInput, LPVOID lpBuffer, DWORD nNumberOfCharsToRead, LPDWORD lpNumberOfCharsRead, LPVOID pInputControl)
{
	if (gReadConsoleInfo.InReadConsoleTID)
		gReadConsoleInfo.LastReadConsoleTID = gReadConsoleInfo.InReadConsoleTID;

#ifdef _DEBUG
	wchar_t szCurDir[MAX_PATH+1];
	GetCurrentDirectory(countof(szCurDir), szCurDir);
#endif

	// To minimize startup duration and possible problems
	// hook server will start on first 'user interaction'
	CheckHookServer();

	bool bCatch = false;
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	HANDLE hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD nConIn = 0, nConOut = 0;
	if (GetConsoleScreenBufferInfo(hConOut, &csbi))
	{
		if (GetConsoleMode(hConsoleInput, &nConIn) && GetConsoleMode(hConOut, &nConOut))
		{
			if ((nConIn & ENABLE_ECHO_INPUT) && (nConIn & ENABLE_LINE_INPUT))
			{
				bCatch = true;
				gReadConsoleInfo.InReadConsoleTID = GetCurrentThreadId();
				gReadConsoleInfo.bIsUnicode = bUnicode;
				gReadConsoleInfo.hConsoleInput = hConsoleInput;
				gReadConsoleInfo.crStartCursorPos = csbi.dwCursorPosition;
				gReadConsoleInfo.nConInMode = nConIn;
				gReadConsoleInfo.nConOutMode = nConOut;

				CEAnsi::OnReadConsoleBefore(hConOut, csbi);
			}
		}
	}

	if (!bCatch)
	{
		gReadConsoleInfo.InReadConsoleTID = 0;
	}

	PreReadConsoleInput(hConsoleInput, (bUnicode ? rcif_Unicode : rcif_Ansi));
}

void OnReadConsoleEnd(BOOL bSucceeded, bool bUnicode, HANDLE hConsoleInput, LPVOID lpBuffer, DWORD nNumberOfCharsToRead, LPDWORD lpNumberOfCharsRead, LPVOID pInputControl)
{
	if (bSucceeded && !gbWasSucceededInRead && lpNumberOfCharsRead && *lpNumberOfCharsRead)
		gbWasSucceededInRead = TRUE;

	if (gReadConsoleInfo.InReadConsoleTID)
	{
		gReadConsoleInfo.LastReadConsoleTID = gReadConsoleInfo.InReadConsoleTID;
		gReadConsoleInfo.InReadConsoleTID = 0;

		TODO("Отослать в ConEmu считанную строку?");
	}

	bool bNoLineFeed = true;
	if (bSucceeded)
	{
		// Empty line was "readed" (Ctrl+C)?
		// Or 'Tab'-eneded when Tab was pressed (for completion)?
		if (lpNumberOfCharsRead && lpBuffer)
		{
			if (!*lpNumberOfCharsRead)
				bNoLineFeed = true; // empty line
			else if (bUnicode && (((wchar_t*)lpBuffer)[*lpNumberOfCharsRead] == L'\t'))
				bNoLineFeed = true; // completion was requested
			else if (!bUnicode && (((char*)lpBuffer)[*lpNumberOfCharsRead] == '\t'))
				bNoLineFeed = true; // completion was requested
		}
	}

	CEAnsi::OnReadConsoleAfter(true, bNoLineFeed);

	// Сброс кешированных значений
	GetConsoleScreenBufferInfoCached(NULL, NULL);

	PostReadConsoleInput(hConsoleInput, (bUnicode ? rcif_Unicode : rcif_Ansi));
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

BOOL WINAPI OnReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
	typedef BOOL (WINAPI* OnReadFile_t)(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
	SUPPRESSORIGINALSHOWCALL;
	ORIGINAL(ReadFile);
	BOOL lbRc = FALSE;

	bool bConIn = false;

	DWORD nPrevErr = GetLastError();

	if (hFile == ghLastConInHandle)
	{
		bConIn = true;
	}
	else if (hFile != ghLastNotConInHandle)
	{
		DWORD nMode = 0;
		BOOL lbConRc = GetConsoleMode(hFile, &nMode);
		if (lbConRc
			&& ((nMode & (ENABLE_LINE_INPUT & ENABLE_ECHO_INPUT)) == (ENABLE_LINE_INPUT & ENABLE_ECHO_INPUT)))
		{
			bConIn = true;
			ghLastConInHandle = hFile;
		}
		else
		{
			ghLastNotConInHandle = hFile;
		}
	}

	if (bConIn)
		OnReadConsoleStart(false, hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, NULL);

	SetLastError(nPrevErr);
	lbRc = F(ReadFile)(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
	DWORD nErr = GetLastError();

	if (bConIn)
	{
		OnReadConsoleEnd(lbRc, false, hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, NULL);

		#ifdef _DEBUG
		// Only for input_bug search purposes in Debug builds
		const char* pszChr = (const char*)lpBuffer;
		int cchRead = *lpNumberOfBytesRead;
		wchar_t* pszDbgCurChars = NULL;
		while ((cchRead--) > 0)
		{
			LONG idx = (InterlockedIncrement(&gn_LogReadChars) & (gn_LogReadCharsMax-1))*2;
			if (!pszDbgCurChars) pszDbgCurChars = gs_LogReadChars+idx;
			gs_LogReadChars[idx++] = L'|';
			gs_LogReadChars[idx++] = *pszChr ? *pszChr : L'.';
			gs_LogReadChars[idx] = 0;
			pszChr++;
		}
		#endif

		SetLastError(nErr);
	}

	UNREFERENCED_PARAMETER(bMainThread);

	return lbRc;
}

BOOL WINAPI OnReadConsoleA(HANDLE hConsoleInput, LPVOID lpBuffer, DWORD nNumberOfCharsToRead, LPDWORD lpNumberOfCharsRead, LPVOID pInputControl)
{
	typedef BOOL (WINAPI* OnReadConsoleA_t)(HANDLE hConsoleInput, LPVOID lpBuffer, DWORD nNumberOfCharsToRead, LPDWORD lpNumberOfCharsRead, LPVOID pInputControl);
	SUPPRESSORIGINALSHOWCALL;
	ORIGINAL(ReadConsoleA);
	BOOL lbRc = FALSE;

	OnReadConsoleStart(false, hConsoleInput, lpBuffer, nNumberOfCharsToRead, lpNumberOfCharsRead, pInputControl);

	lbRc = F(ReadConsoleA)(hConsoleInput, lpBuffer, nNumberOfCharsToRead, lpNumberOfCharsRead, pInputControl);
	DWORD nErr = GetLastError();

	OnReadConsoleEnd(lbRc, false, hConsoleInput, lpBuffer, nNumberOfCharsToRead, lpNumberOfCharsRead, pInputControl);
	SetLastError(nErr);

	UNREFERENCED_PARAMETER(bMainThread);

	return lbRc;
}

bool InitializeClink()
{
	if (gnCmdInitialized)
		return true;
	gnCmdInitialized = 1; // Single

	//if (!gnAllowClinkUsage)
	//	return false;

	CESERVER_CONSOLE_MAPPING_HDR* pConMap = GetConMap();
	if (!pConMap /*|| !(pConMap->Flags & CECF_UseClink_Any)*/)
	{
		//gnAllowClinkUsage = 0;
		gnCmdInitialized = -1;
		return false;
	}

	// Запомнить режим
	//gnAllowClinkUsage =
	//	(pConMap->Flags & CECF_UseClink_2) ? 2 :
	//	(pConMap->Flags & CECF_UseClink_1) ? 1 :
	//	CECF_Empty;
	gbAllowClinkUsage = ((pConMap->Flags & CECF_UseClink_Any) != 0);
	gbAllowUncPaths = (pConMap->ComSpec.isAllowUncPaths != FALSE);

	if (gbAllowClinkUsage)
	{
		wchar_t szClinkBat[MAX_PATH+32];
		wcscpy_c(szClinkBat, pConMap->ComSpec.ConEmuBaseDir);
		wcscat_c(szClinkBat, L"\\clink\\clink.bat");
		if (!FileExists(szClinkBat))
		{
			gbAllowClinkUsage = false;
		}
		else
		{
			int iLen = lstrlen(szClinkBat) + 16;
			gszClinkCmdLine = (wchar_t*)malloc(iLen*sizeof(*gszClinkCmdLine));
			if (gszClinkCmdLine)
			{
				*gszClinkCmdLine = L'"';
				_wcscpy_c(gszClinkCmdLine+1, iLen-1, szClinkBat);
				_wcscat_c(gszClinkCmdLine, iLen, L"\" inject");
			}
		}
	}

	return true;

	//BOOL bRunRc = FALSE;
	//DWORD nErrCode = 0;

	//if (gnAllowClinkUsage == 2)
	//{
	//	// New style. TODO
	//	wchar_t szClinkDir[MAX_PATH+32], szClinkArgs[MAX_PATH+64];

	//	wcscpy_c(szClinkDir, pConMap->ComSpec.ConEmuBaseDir);
	//	wcscat_c(szClinkDir, L"\\clink");

	//	wcscpy_c(szClinkArgs, L"\"");
	//	wcscat_c(szClinkArgs, szClinkDir);
	//	wcscat_c(szClinkArgs, WIN3264TEST(L"\\clink_x86.exe",L"\\clink_x64.exe"));
	//	wcscat_c(szClinkArgs, L"\" inject");

	//	STARTUPINFO si = {sizeof(si)};
	//	PROCESS_INFORMATION pi = {};
	//	bRunRc = CreateProcess(NULL, szClinkArgs, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, szClinkDir, &si, &pi);
	//
	//	if (bRunRc)
	//	{
	//		WaitForSingleObject(pi.hProcess, INFINITE);
	//		CloseHandle(pi.hProcess);
	//		CloseHandle(pi.hThread);
	//	}
	//	else
	//	{
	//		nErrCode = GetLastError();
	//		_ASSERTEX(FALSE && "Clink loader failed");
	//		UNREFERENCED_PARAMETER(nErrCode);
	//		UNREFERENCED_PARAMETER(bRunRc);
	//	}
	//}
	//else if (gnAllowClinkUsage == 1)
	//{
	//	if (!ghClinkDll)
	//	{
	//		wchar_t szClinkModule[MAX_PATH+30];
	//		_wsprintf(szClinkModule, SKIPLEN(countof(szClinkModule)) L"%s\\clink\\%s",
	//			pConMap->ComSpec.ConEmuBaseDir, WIN3264TEST(L"clink_dll_x86.dll",L"clink_dll_x64.dll"));
	//
	//		ghClinkDll = LoadLibrary(szClinkModule);
	//		if (!ghClinkDll)
	//			return false;
	//	}

	//	if (!gpfnClinkReadLine)
	//	{
	//		gpfnClinkReadLine = (call_readline_t)GetProcAddress(ghClinkDll, "call_readline");
	//		_ASSERTEX(gpfnClinkReadLine!=NULL);
	//	}
	//}

	//return (gpfnClinkReadLine != NULL);
}

static bool IsInteractive()
{
	const wchar_t* const cmdLine = ::GetCommandLineW();
	if (!cmdLine)
	{
		return true;	// can't know - assume it is
	}

	const wchar_t* pos = cmdLine;
	while ((pos = wcschr(pos, L'/')) != NULL)
	{
		switch (pos[1])
		{
		case L'K': case L'k':
			return true;	// /k - execute and remain working
		case L'C': case L'c':
			return false;	// /c - execute and exit
		}
		++pos;
	}

	return true;
}

// cmd.exe only!
LONG WINAPI OnRegQueryValueExW(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
	typedef LONG (WINAPI* OnRegQueryValueExW_t)(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
	ORIGINALFASTEX(RegQueryValueExW,NULL);
	//BOOL bMainThread = TRUE; // Does not care
	LONG lRc = -1;

	if (gbIsCmdProcess && hKey && lpValueName)
	{
		if (InitializeClink())
		{
			if (lstrcmpi(lpValueName, L"DisableUNCCheck") == 0)
			{
				if (lpData)
				{
					if (lpcbData && *lpcbData >= sizeof(DWORD))
						*((LPDWORD)lpData) = gbAllowUncPaths;
					else
						*lpData = gbAllowUncPaths;
				}
				if (lpType)
					*lpType = REG_DWORD;
				if (lpcbData)
					*lpcbData = sizeof(DWORD);
				lRc = 0;
				goto wrap;
			}
			if (gbAllowClinkUsage && gszClinkCmdLine && (lstrcmpi(lpValueName, L"AutoRun") == 0)
				&& IsInteractive())
			{
				// Is already loaded?
				HMODULE hClink = GetModuleHandle(WIN3264TEST(L"clink_dll_x86.dll",L"clink_dll_x64.dll"));
				if (hClink == NULL)
				{
					// May be it is set up itself?
					typedef LONG (WINAPI* RegOpenKeyEx_t)(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
					typedef LONG (WINAPI* RegCloseKey_t)(HKEY hKey);
					HMODULE hAdvApi = LoadLibrary(L"AdvApi32.dll");
					if (hAdvApi)
					{
						RegOpenKeyEx_t _RegOpenKeyEx = (RegOpenKeyEx_t)GetProcAddress(hAdvApi, "RegOpenKeyExW");
						RegCloseKey_t  _RegCloseKey  = (RegCloseKey_t)GetProcAddress(hAdvApi, "RegCloseKey");
						if (_RegOpenKeyEx && _RegCloseKey)
						{
							const DWORD cchMax = 0x3FF0;
							const DWORD cbMax = cchMax*2;
							wchar_t* pszCmd = (wchar_t*)malloc(cbMax);
							if (pszCmd)
							{
								DWORD cbSize;
								bool bClinkInstalled = false;
								for (int i = 0; i <= 1 && !bClinkInstalled; i++)
								{
									HKEY hk;
									if (_RegOpenKeyEx(i?HKEY_LOCAL_MACHINE:HKEY_CURRENT_USER, L"Software\\Microsoft\\Command Processor", 0, KEY_READ, &hk))
										continue;
									if (!F(RegQueryValueExW)(hk, lpValueName, NULL, NULL, (LPBYTE)pszCmd, &(cbSize = cbMax))
										&& (cbSize+2) < cbMax)
									{
										cbSize /= 2; pszCmd[cbSize] = 0;
										CharLowerBuffW(pszCmd, cbSize);
										if (wcsstr(pszCmd, L"\\clink.bat"))
											bClinkInstalled = true;
									}
									_RegCloseKey(hk);
								}
								// Not installed via "Autorun"
								if (!bClinkInstalled)
								{
									int iLen = lstrlen(gszClinkCmdLine);
									_wcscpy_c(pszCmd, cchMax, gszClinkCmdLine);
									_wcscpy_c(pszCmd+iLen, cchMax-iLen, L" & "); // conveyer next command indifferent to %errorlevel%

									cbSize = cbMax - (iLen + 3)*sizeof(*pszCmd);
									if (F(RegQueryValueExW)(hKey, lpValueName, NULL, NULL, (LPBYTE)(pszCmd + iLen + 3), &cbSize)
										|| (pszCmd[iLen+3] == 0))
									{
										pszCmd[iLen] = 0; // There is no self value in registry
									}
									cbSize = (lstrlen(pszCmd)+1)*sizeof(*pszCmd);

									// Return
									lRc = 0;
									if (lpData && lpcbData)
									{
										if (*lpcbData < cbSize)
											lRc = ERROR_MORE_DATA;
										else
											_wcscpy_c((wchar_t*)lpData, (*lpcbData)/2, pszCmd);
									}
									if (lpcbData)
										*lpcbData = cbSize;
									free(pszCmd);
									FreeLibrary(hAdvApi);
									goto wrap;
								}
								free(pszCmd);
							}
						}
						FreeLibrary(hAdvApi);
					}
				}
			}
		}
	}

	if (F(RegQueryValueExW))
		lRc = F(RegQueryValueExW)(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);

wrap:
	return lRc;
}


BOOL WINAPI OnReadConsoleW(HANDLE hConsoleInput, LPVOID lpBuffer, DWORD nNumberOfCharsToRead, LPDWORD lpNumberOfCharsRead, MY_CONSOLE_READCONSOLE_CONTROL* pInputControl)
{
	typedef BOOL (WINAPI* OnReadConsoleW_t)(HANDLE hConsoleInput, LPVOID lpBuffer, DWORD nNumberOfCharsToRead, LPDWORD lpNumberOfCharsRead, MY_CONSOLE_READCONSOLE_CONTROL* pInputControl);
	SUPPRESSORIGINALSHOWCALL;
	ORIGINAL(ReadConsoleW);
	BOOL lbRc = FALSE;
	DWORD nErr = GetLastError();
	DWORD nStartTick = GetTickCount(), nEndTick = 0;

	OnReadConsoleStart(true, hConsoleInput, lpBuffer, nNumberOfCharsToRead, lpNumberOfCharsRead, pInputControl);

	lbRc = F(ReadConsoleW)(hConsoleInput, lpBuffer, nNumberOfCharsToRead, lpNumberOfCharsRead, pInputControl);
	nErr = GetLastError();
	// Debug purposes
	nEndTick = GetTickCount();

	OnReadConsoleEnd(lbRc, true, hConsoleInput, lpBuffer, nNumberOfCharsToRead, lpNumberOfCharsRead, pInputControl);

	// cd \\server\share\dir\...
	if (gbAllowUncPaths && lpBuffer && lpNumberOfCharsRead && *lpNumberOfCharsRead
		&& (nNumberOfCharsToRead >= *lpNumberOfCharsRead) && (nNumberOfCharsToRead > 6))
	{
		wchar_t* pszCmd = (wchar_t*)lpBuffer;
		// "cd " ?
		if ((pszCmd[0]==L'c' || pszCmd[0]==L'C') && (pszCmd[1]==L'd' || pszCmd[1]==L'D') && pszCmd[2] == L' ')
		{
			pszCmd[*lpNumberOfCharsRead] = 0;
			wchar_t* pszPath = (wchar_t*)SkipNonPrintable(pszCmd+3);
			// Don't worry about local paths, check only network
			if (pszPath[0] == L'\\' && pszPath[1] == L'\\')
			{
				wchar_t* pszEnd;
				if (*pszPath == L'"')
					pszEnd = wcschr(++pszPath, L'"');
				else
					pszEnd = wcspbrk(pszPath, L"\r\n\t&| ");
				if (!pszEnd)
					pszEnd = pszPath + lstrlen(pszPath);
				if ((pszEnd - pszPath) < MAX_PATH)
				{
					wchar_t ch = *pszEnd; *pszEnd = 0;
					BOOL bSet = SetCurrentDirectory(pszPath);
					if (ch) *pszEnd = ch;
					if (bSet)
					{
						if (*pszEnd == L'"') pszEnd++;
						pszEnd = (wchar_t*)SkipNonPrintable(pszEnd);
						if (*pszEnd && wcschr(L"&|", *pszEnd))
						{
							while (*pszEnd && wcschr(L"&|", *pszEnd)) pszEnd++;
							pszEnd = (wchar_t*)SkipNonPrintable(pszEnd);
						}
						// Return anything...
						if (*pszEnd)
						{
							int nLeft = lstrlen(pszEnd);
							memmove(lpBuffer, pszEnd, (nLeft+1)*sizeof(*pszEnd));
						}
						else
						{
							lstrcpyn((wchar_t*)lpBuffer, L"cd\r\n", nNumberOfCharsToRead);

						}
						*lpNumberOfCharsRead = lstrlen((wchar_t*)lpBuffer);
					}
				}
			}
		}
	}

	SetLastError(nErr);

	UNREFERENCED_PARAMETER(nStartTick);
	UNREFERENCED_PARAMETER(nEndTick);
	return lbRc;
}


BOOL WINAPI OnGetNumberOfConsoleInputEvents(HANDLE hConsoleInput, LPDWORD lpcNumberOfEvents)
{
	typedef BOOL (WINAPI* OnGetNumberOfConsoleInputEvents_t)(HANDLE hConsoleInput, LPDWORD lpcNumberOfEvents);
	SUPPRESSORIGINALSHOWCALL;
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

	// Hiew fails pasting from clipboard on high speed
	if (gbIsHiewProcess && lpcNumberOfEvents && (*lpcNumberOfEvents > 1))
	{
		*lpcNumberOfEvents = 1;
	}

	if (ph && ph->PostCallBack)
	{
		SETARGS2(&lbRc,hConsoleInput,lpcNumberOfEvents);
		ph->PostCallBack(&args);
	}

	return lbRc;
}

BOOL WINAPI OnFlushConsoleInputBuffer(HANDLE hConsoleInput)
{
	typedef BOOL (WINAPI* OnFlushConsoleInputBuffer_t)(HANDLE hConsoleInput);
	SUPPRESSORIGINALSHOWCALL;
	ORIGINALFAST(FlushConsoleInputBuffer);
	BOOL lbRc = FALSE;

	//_ASSERTEX(FALSE && "calling FlushConsoleInputBuffer");
	DebugString(L"### calling FlushConsoleInputBuffer\n");

	lbRc = F(FlushConsoleInputBuffer)(hConsoleInput);

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

	DWORD nCurrentTID = GetCurrentThreadId();

	gReadConsoleInfo.LastReadConsoleInputTID = nCurrentTID;
	gReadConsoleInfo.hConsoleInput2 = hConsoleInput;

	if (nRead)
	{
		if (!gbWasSucceededInRead)
			gbWasSucceededInRead = TRUE;

		// Сброс кешированных значений
		GetConsoleScreenBufferInfoCached(NULL, NULL);
	}

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
		pIn->PeekReadInfo.nTID = nCurrentTID;
		pIn->PeekReadInfo.nPID = GetCurrentProcessId();
		pIn->PeekReadInfo.bMainThread = (pIn->PeekReadInfo.nTID == gnHookMainThreadId);
		memmove(pIn->PeekReadInfo.Buffer, lpBuffer, nRead*sizeof(INPUT_RECORD));

		CESERVER_REQ* pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
		if (pOut) ExecuteFreeResult(pOut);
		ExecuteFreeResult(pIn);
	}


}

static CESERVER_CONSOLE_APP_MAPPING* gpReadConAppMap = NULL;
static DWORD gnReadConAppPID = 0;

void PreReadConsoleInput(HANDLE hConIn, DWORD nFlags/*enum CEReadConsoleInputFlags*/, CESERVER_CONSOLE_APP_MAPPING** ppAppMap /*= NULL*/)
{
	#if defined(_DEBUG) && defined(PRE_PEEK_CONSOLE_INPUT)
	INPUT_RECORD ir = {}; DWORD nRead = 0, nBuffer = 0;
	BOOL bNumGot = GetNumberOfConsoleInputEvents(hConIn, &nBuffer);
	BOOL bConInPeek = nBuffer ? PeekConsoleInputW(hConIn, &ir, 1, &nRead) : FALSE;
	#endif

	if (gbPowerShellMonitorProgress)
	{
		CheckPowershellProgressPresence();
	}

	if (gbCurDirChanged)
	{
		gbCurDirChanged = false;

		if (ghConEmuWndDC)
		{
			if (gFarMode.cbSize
				&& gFarMode.OnCurDirChanged
				&& !IsBadCodePtr((FARPROC)gFarMode.OnCurDirChanged))
			{
				gFarMode.OnCurDirChanged();
			}
			else
			{
				CmdArg szDir;
				if (GetDirectory(szDir) > 0)
				{
					// Sends CECMD_STORECURDIR into RConServer
					SendCurrentDirectory(ghConWnd, szDir);
				}
			}
		}
	}

	if ((nFlags & rcif_LLInput) && !(nFlags & rcif_Peek))
	{
		// On the one hand - there is a problem with unexpected Enter/Space keypress
		// github#19: After executing php.exe from command prompt (it runs by Enter KeyDown)
		//            the app gets in its input queue unexpected Enter KeyUp
		// On the other hand - application must be able to understand if the key was released
		// Powershell's 'get-help Get-ChildItem -full | out-host -paging' or Issue 1927 (jilrun.exe)
		CESERVER_CONSOLE_APP_MAPPING* pAppMap = gpAppMap ? gpAppMap->Ptr() : NULL;
		if (pAppMap)
		{
			DWORD nSelfPID = GetCurrentProcessId();
			pAppMap->nReadConsoleInputPID = nSelfPID;
			pAppMap->nLastReadConsoleInputPID = nSelfPID;
			if (ppAppMap) *ppAppMap = pAppMap;
		}
	}
}

void PostReadConsoleInput(HANDLE hConIn, DWORD nFlags/*enum CEReadConsoleInputFlags*/, CESERVER_CONSOLE_APP_MAPPING* pAppMap /*= NULL*/)
{
	if ((nFlags & rcif_LLInput) && !(nFlags & rcif_Peek))
	{
		if (pAppMap && (pAppMap->nReadConsoleInputPID == GetCurrentProcessId()))
		{
			pAppMap->nReadConsoleInputPID = 0;
		}
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
	SUPPRESSORIGINALSHOWCALL;
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

	PreReadConsoleInput(hConsoleInput, rcif_Ansi|rcif_Peek|rcif_LLInput);

	//#ifdef USE_INPUT_SEMAPHORE
	//DWORD nSemaphore = ghConInSemaphore ? WaitForSingleObject(ghConInSemaphore, INSEMTIMEOUT_READ) : 1;
	//_ASSERTE(nSemaphore<=1);
	//#endif

#if 0
	DWORD nMode = 0; GetConsoleMode(hConsoleInput, &nMode);
#endif

	lbRc = F(PeekConsoleInputA)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);

	PostReadConsoleInput(hConsoleInput, rcif_Ansi|rcif_Peek|rcif_LLInput);

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
	SUPPRESSORIGINALSHOWCALL;
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

	PreReadConsoleInput(hConsoleInput, rcif_Unicode|rcif_Peek|rcif_LLInput);

	//#ifdef USE_INPUT_SEMAPHORE
	//DWORD nSemaphore = ghConInSemaphore ? WaitForSingleObject(ghConInSemaphore, INSEMTIMEOUT_READ) : 1;
	//_ASSERTE(nSemaphore<=1);
	//#endif

	if (gFarMode.bFarHookMode && USE_INTERNAL_QUEUE) // ecompl speed-up
	{
		#ifdef _DEBUG
		DWORD nDbgReadReal = countof(gir_Real), nDbgReadVirtual = countof(gir_Virtual);
		BOOL bReadReal = F(PeekConsoleInputW)(hConsoleInput, gir_Real, nDbgReadReal, &nDbgReadReal);
		BOOL bReadVirt = gInQueue.ReadInputQueue(gir_Virtual, &nDbgReadVirtual, TRUE);
		#endif

		if ((!lbRc || !(lpNumberOfEventsRead && *lpNumberOfEventsRead)) && !gInQueue.IsInputQueueEmpty())
		{
			DWORD n = nLength;
			lbRc = gInQueue.ReadInputQueue(lpBuffer, &n, TRUE);
			if (lpNumberOfEventsRead)
				*lpNumberOfEventsRead = lbRc ? n : 0;
		}
		else
		{
			lbRc = F(PeekConsoleInputW)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);
		}
	}
	else
	{
		lbRc = F(PeekConsoleInputW)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);
	}

	PostReadConsoleInput(hConsoleInput, rcif_Unicode|rcif_Peek|rcif_LLInput);

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
//		msprintf(szDbg, L"ConEmuHk.OnPeekConsoleInputW(x%04X,x%04X) %u\n",
//			lpBuffer->Event.MouseEvent.dwButtonState, lpBuffer->Event.MouseEvent.dwControlKeyState, *lpNumberOfEventsRead);
//	else
//		lstrcpyW(szDbg, L"ConEmuHk.OnPeekConsoleInputW(Non mouse event)\n");
//	DebugStringW(szDbg);
//#endif

	return lbRc;
}


BOOL WINAPI OnReadConsoleInputA(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
	typedef BOOL (WINAPI* OnReadConsoleInputA_t)(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
	SUPPRESSORIGINALSHOWCALL;
	ORIGINAL(ReadConsoleInputA);
	//if (gpFarInfo && bMainThread)
	//	TouchReadPeekConsoleInputs(0);
	BOOL lbRc = FALSE;

	#if defined(_DEBUG)
	#if 1
	UINT nCp = GetConsoleCP();
	UINT nOutCp = GetConsoleOutputCP();
	UINT nOemCp = GetOEMCP();
	UINT nAnsiCp = GetACP();
	#endif
	#endif

	// To minimize startup duration and possible problems
	// hook server will start on first 'user interaction'
	CheckHookServer();

	if (ph && ph->PreCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);

		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	CESERVER_CONSOLE_APP_MAPPING* pAppMap = NULL;
	PreReadConsoleInput(hConsoleInput, rcif_Ansi|rcif_LLInput, &pAppMap);

	//#ifdef USE_INPUT_SEMAPHORE
	//DWORD nSemaphore = ghConInSemaphore ? WaitForSingleObject(ghConInSemaphore, INSEMTIMEOUT_READ) : 1;
	//_ASSERTE(nSemaphore<=1);
	//#endif

	lbRc = F(ReadConsoleInputA)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);

	PostReadConsoleInput(hConsoleInput, rcif_Ansi|rcif_LLInput, pAppMap);

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
	SUPPRESSORIGINALSHOWCALL;
	ORIGINAL(ReadConsoleInputW);
	//if (gpFarInfo && bMainThread)
	//	TouchReadPeekConsoleInputs(0);
	BOOL lbRc = FALSE;

	// To minimize startup duration and possible problems
	// hook server will start on first 'user interaction'
	CheckHookServer();

	if (ph && ph->PreCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);

		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	CESERVER_CONSOLE_APP_MAPPING* pAppMap = NULL;
	PreReadConsoleInput(hConsoleInput, rcif_Unicode|rcif_LLInput, &pAppMap);

	//#ifdef USE_INPUT_SEMAPHORE
	//DWORD nSemaphore = ghConInSemaphore ? WaitForSingleObject(ghConInSemaphore, INSEMTIMEOUT_READ) : 1;
	//_ASSERTE(nSemaphore<=1);
	//#endif

	#if 0
	// get-help Get-ChildItem -full | out-host -paging
	HANDLE hInTest;
	HANDLE hTestHandle = NULL;
	DWORD nInMode, nArgMode;
	BOOL bInTest = FALSE, bArgTest = FALSE;
	if (gbPowerShellMonitorProgress)
	{
		hInTest = GetStdHandle(STD_INPUT_HANDLE);
		#ifdef _DEBUG
		bInTest = GetConsoleMode(hInTest, &nInMode);
		bArgTest = GetConsoleMode(hConsoleInput, &nArgMode);
		hTestHandle = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (hTestHandle != INVALID_HANDLE_VALUE)
			CloseHandle(hTestHandle);
		#endif
	}
	#endif

	if (gFarMode.bFarHookMode && USE_INTERNAL_QUEUE) // ecompl speed-up
	{
		#ifdef _DEBUG
		DWORD nDbgReadReal = countof(gir_Real), nDbgReadVirtual = countof(gir_Virtual);
		BOOL bReadReal = PeekConsoleInputW(hConsoleInput, gir_Real, nDbgReadReal, &nDbgReadReal);
		BOOL bReadVirt = gInQueue.ReadInputQueue(gir_Virtual, &nDbgReadVirtual, TRUE);
		#endif

		if ((!lbRc || !(lpNumberOfEventsRead && *lpNumberOfEventsRead)) && !gInQueue.IsInputQueueEmpty())
		{
			DWORD n = nLength;
			lbRc = gInQueue.ReadInputQueue(lpBuffer, &n, FALSE);
			if (lpNumberOfEventsRead)
				*lpNumberOfEventsRead = lbRc ? n : 0;
		}
		else
		{
			lbRc = F(ReadConsoleInputW)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);
		}
	}
	else
	{
		lbRc = F(ReadConsoleInputW)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);
	}

	PostReadConsoleInput(hConsoleInput, rcif_Unicode|rcif_LLInput, pAppMap);

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

	// ecompl speed-up
	if (gFarMode.bFarHookMode && USE_INTERNAL_QUEUE
		&& nLength && lpBuffer && lpBuffer->EventType == KEY_EVENT && lpBuffer->Event.KeyEvent.uChar.UnicodeChar)
	{
		#ifdef _DEBUG
		memset(gir_Written, 0, sizeof(gir_Written));
		memmove(gir_Written, lpBuffer, min(countof(gir_Written),nLength)*sizeof(*lpBuffer));
		#endif
		lbRc = gInQueue.WriteInputQueue(lpBuffer, FALSE, nLength);
	}
	else
	lbRc = F(WriteConsoleInputW)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsWritten);

	if (ph && ph->PostCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsWritten);
		ph->PostCallBack(&args);
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

// Used in VisualStudio only, required for DefTerm support while debugging
DWORD WINAPI OnResumeThread(HANDLE hThread)
{
	typedef DWORD (WINAPI* OnResumeThread_t)(HANDLE);
	ORIGINALFAST(ResumeThread);

	CShellProc::OnResumeDebugeeThreadCalled(hThread);

	DWORD nRc = F(ResumeThread)(hThread);

	return nRc;
}

BOOL WINAPI OnAllocConsole(void)
{
	typedef BOOL (WINAPI* OnAllocConsole_t)(void);
	ORIGINALFAST(AllocConsole);
	BOOL bMainThread = (GetCurrentThreadId() == gnHookMainThreadId);
	BOOL lbRc = FALSE, lbAllocated = FALSE;
	COORD crLocked;
	HMODULE hKernel = NULL;
	DWORD nErrCode = 0;
	BOOL lbAttachRc = FALSE;
	HWND hOldConWnd = GetRealConsoleWindow();

	if (ph && ph->PreCallBack)
	{
		SETARGS(&lbRc);

		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	if (gbPrepareDefaultTerminal && gbIsNetVsHost)
	{
		if (!ghConWnd)
			gnVsHostStartConsole = 2;
		else
			gnVsHostStartConsole = 0;
	}

	// Попытаться создать консольное окно "по тихому"
	if (gpDefTerm && !hOldConWnd && !gnServerPID)
	{
		HWND hCreatedCon = gpDefTerm->AllocHiddenConsole(false);
		if (hCreatedCon)
		{
			hOldConWnd = hCreatedCon;
			lbAllocated = TRUE;
		}
	}

	// GUI приложение во вкладке. Если окна консоли еще нет - попробовать прицепиться
	// к родительской консоли (консоли серверного процесса)
	if ((gbAttachGuiClient || ghAttachGuiClient) && !gbPrepareDefaultTerminal)
	{
		if (AttachServerConsole())
		{
			hOldConWnd = GetRealConsoleWindow();
			lbAllocated = TRUE; // Консоль уже есть, ничего не надо
		}
	}

	DefTermMsg(L"AllocConsole calling");

	if (!lbAllocated && F(AllocConsole))
	{
		lbRc = F(AllocConsole)();

		if (lbRc && !gbPrepareDefaultTerminal && IsVisibleRectLocked(crLocked))
		{
			// Размер _видимой_ области. Консольным приложениям запрещено менять его "изнутри".
			// Размер может менять только пользователь ресайзом окна ConEmu
			HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
			CONSOLE_SCREEN_BUFFER_INFO csbi = {};
			if (GetConsoleScreenBufferInfo(hStdOut, &csbi))
			{
				//specified width and height cannot be less than the width and height of the console screen buffer's window
				SMALL_RECT rNewRect = {0, 0, crLocked.X-1, crLocked.Y-1};
				OnSetConsoleWindowInfo(hStdOut, TRUE, &rNewRect);
				#ifdef _DEBUG
				COORD crNewSize = {crLocked.X, max(crLocked.Y, csbi.dwSize.Y)};
				#endif
				SetConsoleScreenBufferSize(hStdOut, crLocked);
			}
		}
	}

	//InitializeConsoleInputSemaphore();

	if (ph && ph->PostCallBack)
	{
		SETARGS(&lbRc);
		ph->PostCallBack(&args);
	}

	HWND hNewConWnd = GetRealConsoleWindow();

	// Обновить ghConWnd и мэппинг
	OnConWndChanged(hNewConWnd);

	#ifdef _DEBUG
	//_ASSERTEX(lbRc && ghConWnd);
	wchar_t szAlloc[500], szFile[MAX_PATH];
	GetModuleFileName(NULL, szFile, countof(szFile));
	msprintf(szAlloc, countof(szAlloc), L"OnAllocConsole\nOld=x%08X, New=x%08X, ghConWnd=x%08X\ngbPrepareDefaultTerminal=%i, gbIsNetVsHost=%i\n%s",
		(DWORD)hOldConWnd, (DWORD)hNewConWnd, (DWORD)ghConWnd, gbPrepareDefaultTerminal, gbIsNetVsHost, szFile);
	// VisualStudio host file calls AllocConsole TWICE(!)
	// Second call is totally spare (console already created)
	//MessageBox(NULL, szAlloc, L"OnAllocConsole called", MB_SYSTEMMODAL);
	#endif

	if (hNewConWnd && (hNewConWnd != hOldConWnd) && gpDefTerm && gbIsNetVsHost)
	{
		DefTermMsg(L"Calling gpDefTerm->OnAllocConsoleFinished");
		gpDefTerm->OnAllocConsoleFinished();
		SetLastError(0);
	}
	else if (hNewConWnd)
	{
		DefTermMsg(L"Console was already allocated");
	}
	else
	{
		DefTermMsg(L"Something was wrong");
	}

	TODO("Можно бы по настройке установить параметры. Кодовую страницу, например");

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

	_ASSERTE(F(GetConsoleWindow) != GetRealConsoleWindow);

	if (ghConEmuWndDC && IsWindow(ghConEmuWndDC) /*ghConsoleHwnd*/)
	{
		if (ghAttachGuiClient)
		{
			// В GUI режиме (notepad, putty) отдавать реальный результат GetConsoleWindow()
			// в этом режиме не нужно отдавать ни ghConEmuWndDC, ни серверную консоль
			HWND hReal = GetRealConsoleWindow();
			return hReal;
		}
		else
		{
			//return ghConsoleHwnd;
			return ghConEmuWndDC;
		}
	}

	HWND h;
	h = F(GetConsoleWindow)();
	return h;
}

BOOL WINAPI OnGetConsoleMode(HANDLE hConsoleHandle,LPDWORD lpMode)
{
	typedef BOOL (WINAPI* OnGetConsoleMode_t)(HANDLE,LPDWORD);
	ORIGINALFAST(GetConsoleMode);
	BOOL b;

	#if 0
	if (gbIsBashProcess)
	{
		if (lpMode)
			*lpMode = 0;
		b = FALSE;
	}
	else
	#endif
	{
		b = F(GetConsoleMode)(hConsoleHandle,lpMode);
	}

	return b;
}


BOOL WINAPI OnWriteConsoleOutputA(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion)
{
	typedef BOOL (WINAPI* OnWriteConsoleOutputA_t)(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
	ORIGINAL(WriteConsoleOutputA);
	BOOL lbRc = FALSE;

	#ifdef _DEBUG
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwMode = 0;
	GetConsoleMode(hOut, &dwMode);
	GetConsoleMode(hConsoleOutput, &dwMode);
	#endif

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

	// PowerShell AI для определения прогресса в консоли
	if (gbPowerShellMonitorProgress)
	{
		// Первичные проверки "прогресс ли это"
		if ((dwBufferSize.Y >= 5) && !dwBufferCoord.X && !dwBufferCoord.Y
			&& lpWriteRegion && !lpWriteRegion->Left && (lpWriteRegion->Right == (dwBufferSize.X - 1))
			&& lpBuffer && (lpBuffer->Char.UnicodeChar == L' '))
		{
			#ifdef _DEBUG
			MY_CONSOLE_SCREEN_BUFFER_INFOEX csbi6 = {sizeof(csbi6)};
			apiGetConsoleScreenBufferInfoEx(hConsoleOutput, &csbi6);
			#endif
			// 120720 - PS игнорирует PopupColors в консоли. Вывод прогресса всегда идет 0x3E
			//&& (!gnConsolePopupColors || (lpBuffer->Attributes == gnConsolePopupColors)))
			if (lpBuffer->Attributes == 0x3E)
			{
				CheckPowerShellProgress(hConsoleOutput, lpBuffer, dwBufferSize, dwBufferCoord, lpWriteRegion);
			}
		}
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
	// Если во вкладку ConEmu приаттачено GUI приложение - нам пофигу
	// что оно делает со своей консолью
	if ((ghAttachGuiClient == NULL) && !gbAttachGuiClient && (wAttributes != 7))
	{
		//// Что-то в некоторых случаях сбивается цвет вывода для printf
		//_ASSERTE("SetConsoleTextAttribute" && (wAttributes==0x07));
		wchar_t szDbgInfo[128];
		msprintf(szDbgInfo, countof(szDbgInfo), L"PID=%u, SetConsoleTextAttribute=0x%02X(%u)\n", GetCurrentProcessId(), (int)wAttributes, (int)wAttributes);
		DebugString(szDbgInfo);
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

	if (uMsg == WM_INITDIALOG)
	{
		P = (SimpleApiFunctionArg*)lParam;
		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);

		int x = 0, y = 0;
		RECT rcDC = {};
		if (!GetWindowRect(ghConEmuWndDC, &rcDC))
			SystemParametersInfoW(SPI_GETWORKAREA, 0, &rcDC, 0);
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
				SetWindowTextW(hwndDlg, L"ConEmu ColorPicker");
				break;
			case SimpleApiFunctionArg::sft_ChooseColorW:
				((LPCHOOSECOLORW)P->pArg)->hwndOwner = hwndDlg;
				SetWindowTextW(hwndDlg, L"ConEmu ColorPicker");
				break;
		}

		SetWindowPos(hwndDlg, HWND_TOP, x, y, 0, 0, SWP_SHOWWINDOW);

		P->bResult = P->funcPtr(P->pArg);
		P->nLastError = GetLastError();

		//typedef BOOL (WINAPI* EndDialog_t)(HWND,INT_PTR);
		//EndDialog_t EndDialog_f = (EndDialog_t)GetProcAddress(ghUser32, "EndDialog");
		EndDialog(hwndDlg, 1);

		return FALSE;
	}

	switch (uMsg)
	{
	case WM_GETMINMAXINFO:
		{
			MINMAXINFO* p = (MINMAXINFO*)lParam;
			p->ptMinTrackSize.x = p->ptMinTrackSize.y = 0;
			SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, 0);
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

	INT_PTR iRc = DialogBoxIndirectParam(ghOurModule, pTempl, NULL, SimpleApiDialogProc, (LPARAM)&Arg);
	if (iRc == 1)
	{
		lbRc = Arg.bResult;
		SetLastError(Arg.nLastError);
	}

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

	if (ghConEmuWnd && IsWindow(ghConEmuWnd))
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
		int nLen = lstrlen(lpName)+64;
		wchar_t* psz = (wchar_t*)malloc(nLen*sizeof(*psz));
		if (psz)
		{
			msprintf(psz, nLen, L"CreateNamedPipeW(%s)\n", lpName);
			DebugString(psz);
			free(psz);
		}
	}

#endif
	HANDLE hPipe = F(CreateNamedPipeW)(lpName, dwOpenMode, dwPipeMode, nMaxInstances, nOutBufferSize, nInBufferSize, nDefaultTimeOut, lpSecurityAttributes);
	return hPipe;
}
#endif


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
	typedef HANDLE(WINAPI* OnVirtualAlloc_t)(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
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

// ssh (msysgit) crash issue. Need to know if thread was started by application but not remotely.
HANDLE WINAPI OnCreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId)
{
	typedef HANDLE(WINAPI* OnCreateThread_t)(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);
	ORIGINALFAST(CreateThread);
	DWORD nTemp = 0;
	LPDWORD pThreadID = lpThreadId ? lpThreadId : &nTemp;

	HANDLE hThread = F(CreateThread)(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, pThreadID);

	if (hThread)
		gStartedThreads.Set(*pThreadID,true);

	return hThread;
}

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

static BOOL MyGetConsoleFontSize(COORD& crFontSize)
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

BOOL WINAPI OnGetCurrentConsoleFont(HANDLE hConsoleOutput, BOOL bMaximumWindow, PCONSOLE_FONT_INFO lpConsoleCurrentFont)
{
	typedef BOOL (WINAPI* OnGetCurrentConsoleFont_t)(HANDLE hConsoleOutput, BOOL bMaximumWindow, PCONSOLE_FONT_INFO lpConsoleCurrentFont);
	ORIGINALFASTEX(GetCurrentConsoleFont,NULL);
	BOOL lbRc = FALSE;
	COORD crSize = {};

	if (lpConsoleCurrentFont && MyGetConsoleFontSize(crSize))
	{
		lpConsoleCurrentFont->dwFontSize = crSize;
		lpConsoleCurrentFont->nFont = 1;
		lbRc = TRUE;
	}
	else if (F(GetCurrentConsoleFont))
		lbRc = F(GetCurrentConsoleFont)(hConsoleOutput, bMaximumWindow, lpConsoleCurrentFont);

	return lbRc;
}

COORD WINAPI OnGetConsoleFontSize(HANDLE hConsoleOutput, DWORD nFont)
{
	typedef COORD (WINAPI* OnGetConsoleFontSize_t)(HANDLE hConsoleOutput, DWORD nFont);
	ORIGINALFASTEX(GetConsoleFontSize,NULL);
	COORD cr = {};

	if (!MyGetConsoleFontSize(cr))
	{
		if (F(GetConsoleFontSize))
			cr = F(GetConsoleFontSize)(hConsoleOutput, nFont);
	}

	return cr;
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

HANDLE WINAPI OnCreateConsoleScreenBuffer(DWORD dwDesiredAccess, DWORD dwShareMode, const SECURITY_ATTRIBUTES *lpSecurityAttributes, DWORD dwFlags, LPVOID lpScreenBufferData)
{
	typedef HANDLE(WINAPI* OnCreateConsoleScreenBuffer_t)(DWORD dwDesiredAccess, DWORD dwShareMode, const SECURITY_ATTRIBUTES *lpSecurityAttributes, DWORD dwFlags, LPVOID lpScreenBufferData);
	ORIGINALFAST(CreateConsoleScreenBuffer);

	#ifdef SHOWCREATEBUFFERINFO
	wchar_t szDebugInfo[255];
	msprintf(szDebugInfo, countof(szDebugInfo), L"CreateConsoleScreenBuffer(0x%X,0x%X,0x%X,0x%X,0x%X)",
		dwDesiredAccess, dwShareMode, (DWORD)(DWORD_PTR)lpSecurityAttributes, dwFlags, (DWORD)(DWORD_PTR)lpScreenBufferData);

	#endif

	if ((dwShareMode & (FILE_SHARE_READ|FILE_SHARE_WRITE)) != (FILE_SHARE_READ|FILE_SHARE_WRITE))
		dwShareMode |= (FILE_SHARE_READ|FILE_SHARE_WRITE);

	if ((dwDesiredAccess & (GENERIC_READ|GENERIC_WRITE)) != (GENERIC_READ|GENERIC_WRITE))
		dwDesiredAccess |= (GENERIC_READ|GENERIC_WRITE);

	if (!ghStdOutHandle)
		ghStdOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);

	HANDLE h = INVALID_HANDLE_VALUE;
	if (F(CreateConsoleScreenBuffer))
		h = F(CreateConsoleScreenBuffer)(dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwFlags, lpScreenBufferData);

#ifdef SHOWCREATEBUFFERINFO
	msprintf(szDebugInfo+lstrlen(szDebugInfo), 32, L"=0x%X", (DWORD)(DWORD_PTR)h);
	GuiMessageBox(ghConEmuWnd, szDebugInfo, L"ConEmuHk", MB_SETFOREGROUND|MB_SYSTEMMODAL);
#endif

	return h;
}

BOOL WINAPI OnSetConsoleActiveScreenBuffer(HANDLE hConsoleOutput)
{
	typedef BOOL (WINAPI* OnSetConsoleActiveScreenBuffer_t)(HANDLE hConsoleOutput);
	ORIGINALFAST(SetConsoleActiveScreenBuffer);

	if (!ghStdOutHandle)
		ghStdOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);

	#ifdef _DEBUG
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hIn  = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);
	#endif

	BOOL lbRc = FALSE;
	if (F(SetConsoleActiveScreenBuffer))
		lbRc = F(SetConsoleActiveScreenBuffer)(hConsoleOutput);

	if (lbRc && (ghCurrentOutBuffer || (hConsoleOutput != ghStdOutHandle)))
	{
		#ifdef SHOWCREATEBUFFERINFO
		CONSOLE_SCREEN_BUFFER_INFO lsbi = {};
		BOOL lbTest = GetConsoleScreenBufferInfo(hConsoleOutput, &lsbi);
		DWORD nErrCode = GetLastError();
		_ASSERTE(lbTest && lsbi.dwSize.Y && "GetConsoleScreenBufferInfo(hConsoleOutput) failed");
		#endif

		ghCurrentOutBuffer = hConsoleOutput;
		RequestLocalServerParm Parm = {(DWORD)sizeof(Parm), slsf_SetOutHandle, &ghCurrentOutBuffer};
		RequestLocalServer(&Parm);
	}

	return lbRc;
}

BOOL WINAPI OnSetConsoleWindowInfo(HANDLE hConsoleOutput, BOOL bAbsolute, const SMALL_RECT *lpConsoleWindow)
{
	typedef BOOL (WINAPI* OnSetConsoleWindowInfo_t)(HANDLE hConsoleOutput, BOOL bAbsolute, const SMALL_RECT *lpConsoleWindow);
	ORIGINALFAST(SetConsoleWindowInfo);
	BOOL lbRc = FALSE;
	SMALL_RECT tmp;
	COORD crLocked;

	#ifdef _DEBUG
	CONSOLE_SCREEN_BUFFER_INFO sbi = {};
	BOOL lbSbi = GetConsoleScreenBufferInfo(hConsoleOutput, &sbi);
	UNREFERENCED_PARAMETER(lbSbi);

	wchar_t szDbgSize[512];
	msprintf(szDbgSize, countof(szDbgSize), L"SetConsoleWindowInfo(%08X, %s, {%ix%i}-{%ix%i}), Current={%ix%i}, Wnd={%ix%i}-{%ix%i}\n",
		(DWORD)hConsoleOutput, bAbsolute ? L"ABS" : L"REL",
		lpConsoleWindow->Left, lpConsoleWindow->Top, lpConsoleWindow->Right, lpConsoleWindow->Bottom,
		sbi.dwSize.X, sbi.dwSize.Y,
		sbi.srWindow.Left, sbi.srWindow.Top, sbi.srWindow.Right, sbi.srWindow.Bottom);
	DebugStringConSize(szDbgSize);
	#endif

	BOOL lbLocked = IsVisibleRectLocked(crLocked);

	if (lpConsoleWindow && lbLocked)
	{
		tmp = *lpConsoleWindow;
		if (((tmp.Right - tmp.Left + 1) != crLocked.X) || ((tmp.Bottom - tmp.Top + 1) != crLocked.Y))
		{

			// Размер _видимой_ области. Консольным приложениям запрещено менять его "изнутри".
			// Размер может менять только пользователь ресайзом окна ConEmu
			if ((tmp.Right - tmp.Left + 1) != crLocked.X)
			{
				if (!bAbsolute)
				{
					WARNING("Need to be corrected!");
					tmp.Left = tmp.Right = 0;
				}
				else
				{
					tmp.Right = tmp.Left + crLocked.X - 1;
				}
			}

			if ((tmp.Bottom - tmp.Top + 1) != crLocked.Y)
			{
				if (!bAbsolute)
				{
					WARNING("Need to be corrected!");
					if (tmp.Top != tmp.Bottom)
					{
						tmp.Top = tmp.Bottom = 0;
					}
				}
				else
				{
					tmp.Bottom = tmp.Top + crLocked.Y - 1;
				}
			}

			lpConsoleWindow = &tmp;

			#ifdef _DEBUG
			msprintf(szDbgSize, countof(szDbgSize), L"---> IsVisibleRectLocked, lpConsoleWindow was patched {%ix%i}-{%ix%i}\n",
				tmp.Left, tmp.Top, tmp.Right, tmp.Bottom);
			DebugStringConSize(szDbgSize);
			#endif
		}
	}

	if (F(SetConsoleWindowInfo))
	{
		lbRc = F(SetConsoleWindowInfo)(hConsoleOutput, bAbsolute, lpConsoleWindow);
	}

	return lbRc;
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

BOOL WINAPI OnSetConsoleScreenBufferSize(HANDLE hConsoleOutput, COORD dwSize)
{
	typedef BOOL (WINAPI* OnSetConsoleScreenBufferSize_t)(HANDLE hConsoleOutput, COORD dwSize);
	ORIGINALFAST(SetConsoleScreenBufferSize);
	BOOL lbRc = FALSE, lbRetry = FALSE;
	COORD crLocked;
	DWORD dwErr = -1;

	CONSOLE_SCREEN_BUFFER_INFO sbi = {};
	BOOL lbSbi = GetConsoleScreenBufferInfo(hConsoleOutput, &sbi);
	UNREFERENCED_PARAMETER(lbSbi);

	#ifdef _DEBUG
	wchar_t szDbgSize[512];
	msprintf(szDbgSize, countof(szDbgSize), L"SetConsoleScreenBufferSize(%08X, {%ix%i}), Current={%ix%i}, Wnd={%ix%i}\n",
		(DWORD)hConsoleOutput, dwSize.X, dwSize.Y, sbi.dwSize.X, sbi.dwSize.Y,
		sbi.srWindow.Right-sbi.srWindow.Left+1, sbi.srWindow.Bottom-sbi.srWindow.Top+1);
	DebugStringConSize(szDbgSize);
	#endif

	BOOL lbLocked = IsVisibleRectLocked(crLocked);

	if (lbLocked && ((crLocked.X > dwSize.X) || (crLocked.Y > dwSize.Y)))
	{
		// Размер _видимой_ области. Консольным приложениям запрещено менять его "изнутри".
		// Размер может менять только пользователь ресайзом окна ConEmu
		if (crLocked.X > dwSize.X)
			dwSize.X = crLocked.X;
		if (crLocked.Y > dwSize.Y)
			dwSize.Y = crLocked.Y;

		#ifdef _DEBUG
		msprintf(szDbgSize, countof(szDbgSize), L"---> IsVisibleRectLocked, dwSize was patched {%ix%i}\n",
			dwSize.X, dwSize.Y);
		DebugStringConSize(szDbgSize);
		#endif
	}

	// Do not do useless calls
	if ((dwSize.X == sbi.dwSize.X) && (dwSize.Y == sbi.dwSize.Y))
	{
		lbRc = TRUE;
		goto wrap;
	}

	if (F(SetConsoleScreenBufferSize))
	{
		CESERVER_REQ *pIn = NULL, *pOut = NULL;
		LockServerReadingThread(true, dwSize, pIn, pOut);

		lbRc = F(SetConsoleScreenBufferSize)(hConsoleOutput, dwSize);
		dwErr = GetLastError();

		// The specified dimensions also cannot be less than the minimum size allowed
		// by the system. This minimum depends on the current font size for the console
		// (selected by the user) and the SM_CXMIN and SM_CYMIN values returned by the
		// GetSystemMetrics function.
		if (!lbRc && (dwErr == ERROR_INVALID_PARAMETER))
		{
			// Попытаться увеличить/уменьшить шрифт в консоли
			lbRetry = apiFixFontSizeForBufferSize(hConsoleOutput, dwSize);

			if (lbRetry)
			{
				lbRc = F(SetConsoleScreenBufferSize)(hConsoleOutput, dwSize);
				if (lbRc)
					dwErr = 0;
			}

			// Иногда при закрытии Far возникает ERROR_INVALID_PARAMETER
			// при этом, szDbgSize:
			// SetConsoleScreenBufferSize(0000000C, {80x1000}), Current={94x33}, Wnd={94x33}
			// т.е. коррекция не выполнялась
			_ASSERTE(lbRc && (dwErr != ERROR_INVALID_PARAMETER));
			if (!lbRc)
				SetLastError(dwErr); // вернуть "ошибку"
		}

		LockServerReadingThread(false, dwSize, pIn, pOut);
	}

wrap:
	return lbRc;
}

// WARNING!!! This function exist in Vista and higher OS only!!!
// Issue 1410
BOOL WINAPI OnSetCurrentConsoleFontEx(HANDLE hConsoleOutput, BOOL bMaximumWindow, MY_CONSOLE_FONT_INFOEX* lpConsoleCurrentFontEx)
{
	typedef BOOL (WINAPI* OnSetCurrentConsoleFontEx_t)(HANDLE hConsoleOutput, BOOL bMaximumWindow, MY_CONSOLE_FONT_INFOEX* lpConsoleCurrentFontEx);
	ORIGINALFASTEX(SetCurrentConsoleFontEx,NULL);
	BOOL lbRc = FALSE;

	if (ghConEmuWndDC)
	{
		DebugString(L"Application tries to change console font! Prohibited!");
		//_ASSERTEX(FALSE && "Application tries to change console font! Prohibited!");
		//SetLastError(ERROR_INVALID_HANDLE);
		//Damn powershell close itself if received an error on this call
		lbRc = TRUE; // Cheating
		goto wrap;
	}

	if (F(SetCurrentConsoleFontEx))
		lbRc = F(SetCurrentConsoleFontEx)(hConsoleOutput, bMaximumWindow, lpConsoleCurrentFontEx);

wrap:
	return lbRc;
}

// WARNING!!! This function exist in Vista and higher OS only!!!
BOOL WINAPI OnSetConsoleScreenBufferInfoEx(HANDLE hConsoleOutput, MY_CONSOLE_SCREEN_BUFFER_INFOEX* lpConsoleScreenBufferInfoEx)
{
	typedef BOOL (WINAPI* OnSetConsoleScreenBufferInfoEx_t)(HANDLE hConsoleOutput, MY_CONSOLE_SCREEN_BUFFER_INFOEX* lpConsoleScreenBufferInfoEx);
	ORIGINALFASTEX(SetConsoleScreenBufferInfoEx,NULL);
	BOOL lbRc = FALSE;

	COORD crLocked;
	DWORD dwErr = -1;

	CONSOLE_SCREEN_BUFFER_INFO sbi = {};
	BOOL lbSbi = GetConsoleScreenBufferInfo(hConsoleOutput, &sbi);
	UNREFERENCED_PARAMETER(lbSbi);

	#ifdef _DEBUG
	wchar_t szDbgSize[512];
	if (lpConsoleScreenBufferInfoEx)
	{
		msprintf(szDbgSize, countof(szDbgSize), L"SetConsoleScreenBufferInfoEx(%08X, {%ix%i}), Current={%ix%i}, Wnd={%ix%i}\n",
			(DWORD)hConsoleOutput, lpConsoleScreenBufferInfoEx->dwSize.X, lpConsoleScreenBufferInfoEx->dwSize.Y, sbi.dwSize.X, sbi.dwSize.Y,
			sbi.srWindow.Right-sbi.srWindow.Left+1, sbi.srWindow.Bottom-sbi.srWindow.Top+1);
	}
	else
	{
		lstrcpyn(szDbgSize, L"SetConsoleScreenBufferInfoEx(%08X, NULL)\n", (DWORD)hConsoleOutput);
	}
	DebugStringConSize(szDbgSize);
	#endif

	BOOL lbLocked = IsVisibleRectLocked(crLocked);

	if (lbLocked && lpConsoleScreenBufferInfoEx
		&& ((crLocked.X > lpConsoleScreenBufferInfoEx->dwSize.X) || (crLocked.Y > lpConsoleScreenBufferInfoEx->dwSize.Y)))
	{
		// Размер _видимой_ области. Консольным приложениям запрещено менять его "изнутри".
		// Размер может менять только пользователь ресайзом окна ConEmu
		if (crLocked.X > lpConsoleScreenBufferInfoEx->dwSize.X)
			lpConsoleScreenBufferInfoEx->dwSize.X = crLocked.X;
		if (crLocked.Y > lpConsoleScreenBufferInfoEx->dwSize.Y)
			lpConsoleScreenBufferInfoEx->dwSize.Y = crLocked.Y;

		#ifdef _DEBUG
		msprintf(szDbgSize, countof(szDbgSize), L"---> IsVisibleRectLocked, dwSize was patched {%ix%i}\n",
			lpConsoleScreenBufferInfoEx->dwSize.X, lpConsoleScreenBufferInfoEx->dwSize.Y);
		DebugStringConSize(szDbgSize);
		#endif
	}

	if (F(SetConsoleScreenBufferInfoEx))
	{
		CESERVER_REQ *pIn = NULL, *pOut = NULL;
		if (lpConsoleScreenBufferInfoEx)
			LockServerReadingThread(true, lpConsoleScreenBufferInfoEx->dwSize, pIn, pOut);

		lbRc = F(SetConsoleScreenBufferInfoEx)(hConsoleOutput, lpConsoleScreenBufferInfoEx);
		dwErr = GetLastError();

		if (lpConsoleScreenBufferInfoEx)
			LockServerReadingThread(false, lpConsoleScreenBufferInfoEx->dwSize, pIn, pOut);
	}

	UNREFERENCED_PARAMETER(dwErr);
	return lbRc;
}

COORD WINAPI OnGetLargestConsoleWindowSize(HANDLE hConsoleOutput)
{
	typedef COORD (WINAPI* OnGetLargestConsoleWindowSize_t)(HANDLE hConsoleOutput);
	ORIGINALFAST(GetLargestConsoleWindowSize);
	COORD cr = {80,25}, crLocked = {0,0};

	if (ghConEmuWndDC && IsVisibleRectLocked(crLocked))
	{
		cr = crLocked;
	}
	else
	{
		if (F(GetLargestConsoleWindowSize))
		{
			cr = F(GetLargestConsoleWindowSize)(hConsoleOutput);
		}

		// Wine BUG
		//if (!cr.X || !cr.Y)
		if ((cr.X == 80 && cr.Y == 24) && IsWine())
		{
			cr.X = 255;
			cr.Y = 255;
		}
	}

	return cr;
}

BOOL WINAPI OnSetConsoleCursorPosition(HANDLE hConsoleOutput, COORD dwCursorPosition)
{
	typedef BOOL (WINAPI* OnSetConsoleCursorPosition_t)(HANDLE,COORD);
	ORIGINALFAST(SetConsoleCursorPosition);

	BOOL lbRc;

	if (gbIsVimAnsi)
	{
		#ifdef DUMP_VIM_SETCURSORPOS
		wchar_t szDbg[80]; msprintf(szDbg, countof(szDbg), L"ViM trying to set cursor pos: {%i,%i}\n", (UINT)dwCursorPosition.X, (UINT)dwCursorPosition.Y);
		OutputDebugString(szDbg);
		#endif
		lbRc = FALSE;
	}
	else
	{
		lbRc = F(SetConsoleCursorPosition)(hConsoleOutput, dwCursorPosition);
	}

	if (ghConsoleCursorChanged)
		SetEvent(ghConsoleCursorChanged);

	return lbRc;
}

BOOL WINAPI OnSetConsoleCursorInfo(HANDLE hConsoleOutput, const CONSOLE_CURSOR_INFO *lpConsoleCursorInfo)
{
	typedef BOOL (WINAPI* OnSetConsoleCursorInfo_t)(HANDLE,const CONSOLE_CURSOR_INFO *);
	ORIGINALFAST(SetConsoleCursorInfo);

	BOOL lbRc = F(SetConsoleCursorInfo)(hConsoleOutput, lpConsoleCursorInfo);

	if (ghConsoleCursorChanged)
		SetEvent(ghConsoleCursorChanged);

	return lbRc;
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

// Нужна для "поднятия" консольного окна при вызове Shell операций
INT_PTR WINAPI OnDialogBoxParamW(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	typedef INT_PTR (WINAPI* OnDialogBoxParamW_t)(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
	ORIGINALFASTEX(DialogBoxParamW,NULL);
	INT_PTR iRc = 0;

	if (ghConEmuWndDC)
	{
		// Необходимо "поднять" наверх консольное окно, иначе Shell-овский диалог окажется ПОД ConEmu
		GuiSetForeground(hWndParent ? hWndParent : ghConWnd);
		// bugreport from Andrey Budko: conemu + emenu/{Run Sandboxed} замораживает фар
		PatchDialogParentWnd(hWndParent);
	}

	if (F(DialogBoxParamW))
		iRc = F(DialogBoxParamW)(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);

	return iRc;
}

HDC WINAPI OnGetDC(HWND hWnd)
{
	typedef HDC (WINAPI* OnGetDC_t)(HWND hWnd);
	ORIGINALFASTEX(GetDC,NULL);
	HDC hDC = NULL;

	if (F(GetDC))
		hDC = F(GetDC)(hWnd);

	if (hDC && ghConEmuWndDC && hWnd == ghConEmuWndDC)
		ghTempHDC = hDC;

	return hDC;
}

HDC WINAPI OnGetDCEx(HWND hWnd, HRGN hrgnClip, DWORD flags)
{
	typedef HDC (WINAPI* OnGetDCEx_t)(HWND hWnd, HRGN hrgnClip, DWORD flags);
	ORIGINALFASTEX(GetDCEx,NULL);
	HDC hDC = NULL;

	if (F(GetDCEx))
		hDC = F(GetDCEx)(hWnd, hrgnClip, flags);

	if (hDC && ghConEmuWndDC && hWnd == ghConEmuWndDC)
		ghTempHDC = hDC;

	return hDC;
}

int WINAPI OnReleaseDC(HWND hWnd, HDC hDC)
{
	typedef int (WINAPI* OnReleaseDC_t)(HWND hWnd, HDC hDC);
	ORIGINALFASTEX(ReleaseDC,NULL);
	int iRc = 0;

	if (F(ReleaseDC))
		iRc = F(ReleaseDC)(hWnd, hDC);

	if (ghTempHDC == hDC)
		ghTempHDC = NULL;

	return iRc;
}

int WINAPI OnStretchDIBits(HDC hdc, int XDest, int YDest, int nDestWidth, int nDestHeight, int XSrc, int YSrc, int nSrcWidth, int nSrcHeight, const VOID *lpBits, const BITMAPINFO *lpBitsInfo, UINT iUsage, DWORD dwRop)
{
	typedef int (WINAPI* OnStretchDIBits_t)(HDC hdc, int XDest, int YDest, int nDestWidth, int nDestHeight, int XSrc, int YSrc, int nSrcWidth, int nSrcHeight, const VOID *lpBits, const BITMAPINFO *lpBitsInfo, UINT iUsage, DWORD dwRop);
	ORIGINALFASTEX(StretchDIBits,NULL);
	int iRc = 0;

	if (F(StretchDIBits))
		iRc = F(StretchDIBits)(hdc, XDest, YDest, nDestWidth, nDestHeight, XSrc, YSrc, nSrcWidth, nSrcHeight, lpBits, lpBitsInfo, iUsage, dwRop);

	// Если рисуют _прямо_ на канвасе ConEmu
	if (iRc != (int)GDI_ERROR && hdc && hdc == ghTempHDC)
	{
		// Уведомить GUI, что у него прямо на канвасе кто-то что-то нарисовал :)
		CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_LOCKDC, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_LOCKDC));
		if (pIn)
		{
			pIn->LockDc.hDcWnd = ghConEmuWndDC; // На всякий случай
			pIn->LockDc.bLock = TRUE;
			pIn->LockDc.Rect.left = XDest;
			pIn->LockDc.Rect.top = YDest;
			pIn->LockDc.Rect.right = XDest+nDestWidth-1;
			pIn->LockDc.Rect.bottom = YDest+nDestHeight-1;

			CESERVER_REQ* pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);

			if (pOut)
				ExecuteFreeResult(pOut);
			ExecuteFreeResult(pIn);
		}
	}

	return iRc;
}

BOOL WINAPI OnBitBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, DWORD dwRop)
{
	typedef int (WINAPI* OnBitBlt_t)(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, DWORD dwRop);
	ORIGINALFASTEX(BitBlt,NULL);
	BOOL bRc = FALSE;

	if (F(BitBlt))
		bRc = F(BitBlt)(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, dwRop);

	// Если рисуют _прямо_ на канвасе ConEmu
	if (bRc && hdcDest && hdcDest == ghTempHDC)
	{
		// Уведомить GUI, что у него прямо на канвасе кто-то что-то нарисовал :)
		CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_LOCKDC, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_LOCKDC));
		if (pIn)
		{
			pIn->LockDc.hDcWnd = ghConEmuWndDC; // На всякий случай
			pIn->LockDc.bLock = TRUE;
			pIn->LockDc.Rect.left = nXDest;
			pIn->LockDc.Rect.top = nYDest;
			pIn->LockDc.Rect.right = nXDest+nWidth-1;
			pIn->LockDc.Rect.bottom = nYDest+nHeight-1;

			CESERVER_REQ* pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);

			if (pOut)
				ExecuteFreeResult(pOut);
			ExecuteFreeResult(pIn);
		}
	}

	return bRc;
}

// Поддержка батчей из GdipDrawImageRectRectI
static RECT StretchBltBatch = {};

BOOL WINAPI OnStretchBlt(HDC hdcDest, int nXOriginDest, int nYOriginDest, int nWidthDest, int nHeightDest, HDC hdcSrc, int nXOriginSrc, int nYOriginSrc, int nWidthSrc, int nHeightSrc, DWORD dwRop)
{
	typedef int (WINAPI* OnStretchBlt_t)(HDC hdcDest, int nXOriginDest, int nYOriginDest, int nWidthDest, int nHeightDest, HDC hdcSrc, int nXOriginSrc, int nYOriginSrc, int nWidthSrc, int nHeightSrc, DWORD dwRop);
	ORIGINALFASTEX(StretchBlt,NULL);
	BOOL bRc = FALSE;

	//#ifdef _DEBUG
	//HWND h = WindowFromDC(hdcDest);
	//#endif

	// Если рисуют _прямо_ на канвасе ConEmu
	if (/*bRc &&*/ hdcDest && hdcDest == ghTempHDC)
	{
		if (
			(!StretchBltBatch.bottom && !StretchBltBatch.top)
			|| (nYOriginDest <= StretchBltBatch.top)
			|| (nXOriginDest != StretchBltBatch.left)
			|| (StretchBltBatch.right != (nXOriginDest+nWidthDest-1))
			|| (StretchBltBatch.bottom != (nYOriginDest-1))
			)
		{
			// Сброс батча
			StretchBltBatch.left = nXOriginDest;
			StretchBltBatch.top = nYOriginDest;
			StretchBltBatch.right = nXOriginDest+nWidthDest-1;
			StretchBltBatch.bottom = nYOriginDest+nHeightDest-1;
		}
		else
		{
			StretchBltBatch.bottom = nYOriginDest+nHeightDest-1;
		}

		// Уведомить GUI, что у него прямо на канвасе кто-то что-то нарисовал :)
		CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_LOCKDC, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_LOCKDC));
		if (pIn)
		{
			pIn->LockDc.hDcWnd = ghConEmuWndDC; // На всякий случай
			pIn->LockDc.bLock = TRUE;
			pIn->LockDc.Rect = StretchBltBatch;

			CESERVER_REQ* pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);

			if (pOut)
				ExecuteFreeResult(pOut);
			ExecuteFreeResult(pIn);
		}
	}

	if (F(StretchBlt))
		bRc = F(StretchBlt)(hdcDest, nXOriginDest, nYOriginDest, nWidthDest, nHeightDest, hdcSrc, nXOriginSrc, nYOriginSrc, nWidthSrc, nHeightSrc, dwRop);

	return bRc;
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
