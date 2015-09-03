
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
#include "hkEnvironment.h"
#include "hkFarExe.h"
#include "hkGDI.h"
#include "hkKernel.h"
#include "hkLibrary.h"
#include "hkMessages.h"
#include "hkProcess.h"
#include "hkStdIO.h"
#include "hkWindow.h"

#include "hlpProcess.h"


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


#undef isPressed
#define isPressed(inp) ((GetKeyState(inp) & 0x8000) == 0x8000)



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

// Service functions
//typedef DWORD (WINAPI* GetProcessId_t)(HANDLE Process);
GetProcessId_t gfGetProcessId = NULL;


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



bool InitHooksLibrary()
{
	#ifdef _DEBUG
	extern HookItem *gpHooks;
	extern size_t gnHookedFuncs;
	// Must be placed on top
	_ASSERTE(gpHooks && gnHookedFuncs==0 && gpHooks[0].NewAddress==NULL);
	#endif

	HLOG1("InitHooksLibrary",0);
	bool lbRc = false;

	HookItem HooksLib1[] =
	{
		/* ************************ */
		{(void*)OnLoadLibraryA,		"LoadLibraryA",			kernel32},
		{(void*)OnLoadLibraryExA,	"LoadLibraryExA",		kernel32},
		{(void*)OnLoadLibraryExW,	"LoadLibraryExW",		kernel32},
		{(void*)OnFreeLibrary,		"FreeLibrary",			kernel32},
		/* ************************ */
		{0}
	};

	HookItem HooksLib2[] =
	{
		/* ************************ */
		{(void*)OnLoadLibraryW,		"LoadLibraryW",			kernel32},
		/* ************************ */
		{0}
	};


	// No need to hook these functions in Vista+
	if (!gbLdrDllNotificationUsed)
	{
		if (InitHooks(HooksLib1) < 0)
			goto wrap;
	}

	// With only exception of LoadLibraryW - it handles "ExtendedConsole.dll" loading in Far 64
	if (gbIsFarProcess || !gbLdrDllNotificationUsed)
	{
		if (InitHooks(HooksLib2) < 0)
			goto wrap;
	}

	lbRc = true;
wrap:
	HLOGEND1();

	return lbRc;
}


bool InitHooksKernel()
{
	HLOG1("InitHooksKernel",0);
	bool lbRc = false;

	HookItem HooksKernel[] =
	{
		/* ************************ */
		{(void*)OnOpenFileMappingW,			"OpenFileMappingW",		kernel32},
		{(void*)OnMapViewOfFile,			"MapViewOfFile",		kernel32},
		{(void*)OnUnmapViewOfFile,			"UnmapViewOfFile",		kernel32},
		/* ************************ */
		{(void*)OnSetEnvironmentVariableA,	"SetEnvironmentVariableA",	kernel32},
		{(void*)OnSetEnvironmentVariableW,	"SetEnvironmentVariableW",	kernel32},
		{(void*)OnGetEnvironmentVariableA,	"GetEnvironmentVariableA",	kernel32},
		{(void*)OnGetEnvironmentVariableW,	"GetEnvironmentVariableW",	kernel32},
		#if 0
		{(void*)OnGetEnvironmentStringsA,	"GetEnvironmentStringsA",	kernel32},
		#endif
		{(void*)OnGetEnvironmentStringsW,	"GetEnvironmentStringsW",	kernel32},
		/* ************************ */
		{(void*)OnGetSystemTime,			"GetSystemTime",			kernel32},
		{(void*)OnGetLocalTime,				"GetLocalTime",				kernel32},
		{(void*)OnGetSystemTimeAsFileTime,	"GetSystemTimeAsFileTime",	kernel32},
		/* ************************ */
		{0}
	};

	if (InitHooks(HooksKernel) < 0)
		goto wrap;

	#if 0
	// Проверка, как реагирует на дубли
	HooksCommon[1].NewAddress = NULL;
	_ASSERTEX(FALSE && "Testing");
	InitHooks(HooksCommon);
	#endif

	lbRc = true;
wrap:
	HLOGEND1();

	return lbRc;
}

bool InitHooksDebugging()
{
	HLOG1("InitHooksDebugging",0);
	bool lbRc = false;

	#ifdef _DEBUG
	HookItem HooksDbg[] =
	{
		/* ************************ */

		{(void*)OnCreateNamedPipeW,			"CreateNamedPipeW",		kernel32},

		{(void*)OnVirtualAlloc,				"VirtualAlloc",			kernel32},
		{(void*)OnVirtualProtect,			"VirtualProtect",		kernel32},
		{(void*)OnSetUnhandledExceptionFilter,
											"SetUnhandledExceptionFilter",
																kernel32},

		/* ************************ */

		#ifdef HOOK_ERROR_PROC
		{(void*)OnGetLastError,			"GetLastError",			kernel32},
		{(void*)OnSetLastError,			"SetLastError",			kernel32}, // eSetLastError
		#endif

		/* ************************ */
		{0}
	};

	if (InitHooks(HooksDbg) < 0)
		goto wrap;
	#endif // _DEBUG

	lbRc = true;
wrap:
	HLOGEND1();

	return lbRc;
}

// Console, ANSI, Read/Write, etc.
bool InitHooksConsole()
{
	HLOG1("InitHooksConsole",0);
	bool lbRc = false;

	HookItem HooksConsole[] =
	{
		/* ************************ */
		{(void*)OnCreateFileW,			"CreateFileW",  		kernel32},
		{(void*)OnWriteFile,			"WriteFile",  			kernel32},
		{(void*)OnReadFile,				"ReadFile",				kernel32},
		{(void*)OnCloseHandle,			"CloseHandle",			kernel32},
		/* ************************ */
		{(void*)OnGetConsoleWindow,     "GetConsoleWindow",     kernel32},
		{(void*)OnGetConsoleMode,		"GetConsoleMode",		kernel32},
		{(void*)OnSetConsoleMode,		"SetConsoleMode",  		kernel32},
		{(void*)OnSetConsoleTitleA,		"SetConsoleTitleA",		kernel32},
		{(void*)OnSetConsoleTitleW,		"SetConsoleTitleW",		kernel32},
		{(void*)OnGetConsoleAliasesW,	"GetConsoleAliasesW",	kernel32},
		{(void*)OnAllocConsole,			"AllocConsole",			kernel32},
		{(void*)OnFreeConsole,			"FreeConsole",			kernel32},
		{(void*)OnSetConsoleKeyShortcuts, "SetConsoleKeyShortcuts", kernel32},
		/* ************************ */
		{(void*)OnSetConsoleTextAttribute, "SetConsoleTextAttribute", kernel32},
		{(void*)OnWriteConsoleOutputW,	"WriteConsoleOutputW",  kernel32},
		{(void*)OnWriteConsoleOutputA,	"WriteConsoleOutputA",  kernel32},
		{(void*)OnReadConsoleW,			"ReadConsoleW",			kernel32},
		{(void*)OnReadConsoleA,			"ReadConsoleA",			kernel32},
		{(void*)OnPeekConsoleInputW,	"PeekConsoleInputW",	kernel32},
		{(void*)OnPeekConsoleInputA,	"PeekConsoleInputA",	kernel32},
		{(void*)OnReadConsoleInputW,	"ReadConsoleInputW",	kernel32},
		{(void*)OnReadConsoleInputA,	"ReadConsoleInputA",	kernel32},
		{(void*)OnWriteConsoleInputA,	"WriteConsoleInputA",	kernel32},
		{(void*)OnWriteConsoleInputW,	"WriteConsoleInputW",	kernel32},
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
		/* Others console functions */
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
		/* ************************ */
		{(void*)OnGetCurrentConsoleFont, "GetCurrentConsoleFont",		kernel32},
		{(void*)OnGetConsoleFontSize,    "GetConsoleFontSize",			kernel32},
		/* ************************ */

		// https://conemu.github.io/en/MicrosoftBugs.html#chcp_hung
		{(void*)OnSetConsoleCP,			"SetConsoleCP",			kernel32},
		{(void*)OnSetConsoleOutputCP,	"SetConsoleOutputCP",	kernel32},
		/* ************************ */
		{0}
	};

	if (InitHooks(HooksConsole) < 0)
		goto wrap;

	lbRc = true;
wrap:
	HLOGEND1();

	return lbRc;
}

bool InitHooksExecutor()
{
	HLOG1("InitHooksExecutor",0);
	bool lbRc = false;

	HookItem HooksCommon[] =
	{
		/* ************************ */
		{(void*)OnExitProcess,			"ExitProcess",			kernel32},
		{(void*)OnTerminateProcess,		"TerminateProcess",		kernel32},
		/* ************************ */
		{(void*)OnCreateThread,			"CreateThread",			kernel32},
		{(void*)OnSetThreadContext,		"SetThreadContext",		kernel32},
		{(void*)OnTerminateThread,		"TerminateThread",		kernel32},
		/* ************************ */
		{(void*)OnCreateProcessA,		"CreateProcessA",		kernel32},
		{(void*)OnCreateProcessW,		"CreateProcessW",		kernel32},
		/* ************************ */
		{(void*)OnSetCurrentDirectoryA, "SetCurrentDirectoryA", kernel32},
		{(void*)OnSetCurrentDirectoryW, "SetCurrentDirectoryW", kernel32},
		/* ************************ */
		{(void*)OnShellExecuteExA,		"ShellExecuteExA",		shell32},
		{(void*)OnShellExecuteExW,		"ShellExecuteExW",		shell32},
		{(void*)OnShellExecuteA,		"ShellExecuteA",		shell32},
		{(void*)OnShellExecuteW,		"ShellExecuteW",		shell32},
		/* ************************ */
		// OnWinExec/WinExec is used in DefTerm only
		/* ************************ */
		{0}
	};

	if (InitHooks(HooksCommon) < 0)
		goto wrap;

	lbRc = true;
wrap:
	HLOGEND1();

	return lbRc;
}

// user32 & gdi32
bool InitHooksUser32()
{
	HLOG1("InitHooksUser32",0);
	bool lbRc = false;

	HookItem HooksUserGdi[] =
	{
		/* ************************ */
		{(void*)OnTrackPopupMenu,		"TrackPopupMenu",		user32},
		{(void*)OnTrackPopupMenuEx,		"TrackPopupMenuEx",		user32},
		{(void*)OnFlashWindow,			"FlashWindow",			user32},
		{(void*)OnFlashWindowEx,		"FlashWindowEx",		user32},
		{(void*)OnSetForegroundWindow,	"SetForegroundWindow",	user32},
		{(void*)OnGetForegroundWindow,	"GetForegroundWindow",	user32},
		{(void*)OnGetWindowRect,		"GetWindowRect",		user32},
		{(void*)OnScreenToClient,		"ScreenToClient",		user32},
		/* ************************ */
		//{(void*)OnCreateWindowA,		"CreateWindowA",		user32}, -- there is not such export
		//{(void*)OnCreateWindowW,		"CreateWindowW",		user32}, -- there is not such export
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
		{(void*)OnDialogBoxIndirectParamAorW,
										"DialogBoxIndirectParamAorW",
																user32},
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

	if (InitHooks(HooksUserGdi) < 0)
		goto wrap;

	lbRc = true;
wrap:
	HLOGEND1();

	return lbRc;
}

bool InitHooksFarExe()
{
	HLOG1("InitHooksFarExe",0);
	bool lbRc = false;

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

	HookItem HooksFarOnly[] =
	{
		/* ************************ */
		{(void*)OnCompareStringW, "CompareStringW", kernel32},
		/* ************************ */
		{0}
	};

	if (lbIsFar)
	{
		if (InitHooks(HooksFarOnly) < 0)
			goto wrap;
	}

	lbRc = true;
wrap:
	HLOGEND1();

	return lbRc;
}

bool InitHooksCmdExe()
{
	if (!gbIsCmdProcess)
		return true;

	HLOG1("InitHooksCmdExe",0);
	bool lbRc = false;

	HookItem HooksCmdOnly[] =
	{
		// Vista and below: AdvApi32.dll
		// **NB** In WinXP this module is not linked statically
		{(void*)OnRegQueryValueExW, "RegQueryValueExW", IsWin7() ? kernel32 : advapi32},
		{0, 0, 0}
	};

	if (InitHooks(HooksCmdOnly) < 0)
		goto wrap;

	lbRc = true;
wrap:
	HLOGEND1();

	return lbRc;
}




/* ************************ */
/* Default Terminal support */
/* ************************ */
bool InitHooksDefTerm()
{
	HLOG1("InitHooksDefTerm",0);
	bool lbRc = false;

	// These functions are required for seizing console and behave as Default Terminal

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

	// Required for hooking in OS
	// bool check
	if (!InitHooksLibrary())
		goto wrap;

	// Start our functions
	if (InitHooks(HooksCommon) < 0)
		goto wrap;

	// Windows 7. There is new undocumented function "ShellExecCmdLine" used by Explorer
	if (IsWin7())
	{
		if (InitHooks(HooksCmdLine) < 0)
			goto wrap;
	}

	// "*.vshost.exe" uses special helper
	if (gbIsNetVsHost)
	{
		if (InitHooks(HooksVshost) < 0)
			goto wrap;
	}

	// Required in VisualStudio and CodeBlocks (gdb) debuggers
	// Don't restrict to them, other Dev Envs may behave in similar way
	{
		if (InitHooks(HooksDevStudio) < 0)
			goto wrap;
	}

	lbRc = true;
wrap:
	HLOGEND1();

	return lbRc;
}



/* ******************* */
/* Console or ChildGui */
/* ******************* */

bool InitHooksCommon()
{
	if (!InitHooksLibrary())
		return false;

	if (!InitHooksKernel())
		return false;

	if (!InitHooksExecutor())
		return false;

	if (!InitHooksDebugging())
		return false;

	if (!InitHooksConsole())
		return false;

	if (!InitHooksUser32())
		return false;

	if (!InitHooksFarExe())
		return false;

	if (!InitHooksCmdExe())
		return false;

	return true;
}
