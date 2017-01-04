
/*
Copyright (c) 2009-2017 Maximus5
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

// Declare function ID variables
#define DECLARE_CONEMU_HOOK_FUNCTION_ID

#include <windows.h>
#include <WinError.h>
#include <WinNT.h>
#include <TCHAR.h>
#include <Tlhelp32.h>
#include <shlwapi.h>
#include "../common/Common.h"
#include "../common/ConEmuCheck.h"
#include "SetHook.h"
#include "../common/execute.h"
#include "DefTermHk.h"
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


/* ************ Globals for SetHook ************ */
extern HWND    ghConWnd;      // RealConsole
extern HWND    ghConEmuWnd;   // Root! ConEmu window
extern HWND    ghConEmuWndDC; // ConEmu DC window
extern DWORD   gnGuiPID;
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
bool     gbClinkInjectRequested = false;
bool     gbAllowUncPaths = false;
bool     gbCurDirChanged = false;
/* ************ Globals for cmd.exe/clink ************ */

/* ************ Globals for powershell ************ */
bool gbIsPowerShellProcess = false;
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

/* ************ Globals for Plink ************ */
bool gbIsPlinkProcess = false;
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
		HOOK_ITEM_BY_NAME(LoadLibraryA,			kernel32),
		HOOK_ITEM_BY_NAME(LoadLibraryExA,		kernel32),
		HOOK_ITEM_BY_NAME(LoadLibraryExW,		kernel32),
		HOOK_ITEM_BY_NAME(FreeLibrary,			kernel32),
		/* ************************ */
		{0}
	};

	HookItem HooksLib2[] =
	{
		/* ************************ */
		HOOK_ITEM_BY_NAME(LoadLibraryW,			kernel32),
		/* ************************ */
		{0}
	};


	// Need to hook All LoadLibrary### if the work isn't done by LdrRegisterDllNotification
	if (gnLdrDllNotificationUsed != ldr_FullSupport)
	{
		if (InitHooks(HooksLib1) < 0)
			goto wrap;
	}

	// Also LoadLibraryW, it handles "ExtendedConsole.dll" loading in Far 32/64
	if (gbIsFarProcess || (gnLdrDllNotificationUsed != ldr_FullSupport))
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
		HOOK_ITEM_BY_NAME(OpenFileMappingW,		kernel32),
		HOOK_ITEM_BY_NAME(MapViewOfFile,		kernel32),
		HOOK_ITEM_BY_NAME(UnmapViewOfFile,		kernel32),
		/* ************************ */
		HOOK_ITEM_BY_NAME(SetEnvironmentVariableA,	kernel32),
		HOOK_ITEM_BY_NAME(SetEnvironmentVariableW,	kernel32),
		HOOK_ITEM_BY_NAME(GetEnvironmentVariableA,	kernel32),
		HOOK_ITEM_BY_NAME(GetEnvironmentVariableW,	kernel32),
		#if 0
		HOOK_ITEM_BY_NAME(GetEnvironmentStringsA,	kernel32),
		#endif
		HOOK_ITEM_BY_NAME(GetEnvironmentStringsW,	kernel32),
		/* ************************ */
		HOOK_ITEM_BY_NAME(GetSystemTime,			kernel32),
		HOOK_ITEM_BY_NAME(GetLocalTime,				kernel32),
		HOOK_ITEM_BY_NAME(GetSystemTimeAsFileTime,	kernel32),
		/* ************************ */
		HOOK_ITEM_BY_NAME(Beep,						kernel32),
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
#if !defined(_DEBUG)

	return true;

#else

	HLOG1("InitHooksDebugging",0);
	bool lbRc = false;

	HookItem HooksDbg[] =
	{
		/* ************************ */

		HOOK_ITEM_BY_NAME(CreateNamedPipeW,		kernel32),

		HOOK_ITEM_BY_NAME(VirtualAlloc,			kernel32),
		#if 0
		HOOK_ITEM_BY_NAME(VirtualProtect,		kernel32),
		#endif
		HOOK_ITEM_BY_NAME(SetUnhandledExceptionFilter, kernel32),

		/* ************************ */

		#ifdef HOOK_ERROR_PROC
		HOOK_ITEM_BY_NAME(GetLastError,			kernel32),
		HOOK_ITEM_BY_NAME(SetLastError,			kernel32), // eSetLastError
		#endif

		/* ************************ */
		{0}
	};

	if (InitHooks(HooksDbg) < 0)
		goto wrap;

	lbRc = true;
wrap:
	HLOGEND1();

	return lbRc;
#endif // _DEBUG
}

// Console, ANSI, Read/Write, etc.
bool InitHooksConsole()
{
	HLOG1("InitHooksConsole",0);
	bool lbRc = false;

	HookItem HooksConsole[] =
	{
		/* ************************ */
		HOOK_ITEM_BY_NAME(CreateFileW,  		kernel32),
		HOOK_ITEM_BY_NAME(CreateFileA,  		kernel32),
		HOOK_ITEM_BY_NAME(WriteFile,  			kernel32),
		HOOK_ITEM_BY_NAME(ReadFile,				kernel32),
		HOOK_ITEM_BY_NAME(CloseHandle,			kernel32),
		/* ************************ */
		HOOK_ITEM_BY_NAME(SetStdHandle,			kernel32),
		/* ************************ */
		HOOK_ITEM_BY_NAME(GetConsoleWindow,     kernel32),
		HOOK_ITEM_BY_NAME(GetConsoleMode,		kernel32),
		HOOK_ITEM_BY_NAME(SetConsoleMode,  		kernel32),
		HOOK_ITEM_BY_NAME(SetConsoleTitleA,		kernel32),
		HOOK_ITEM_BY_NAME(SetConsoleTitleW,		kernel32),
		HOOK_ITEM_BY_NAME(GetConsoleAliasesW,	kernel32),
		HOOK_ITEM_BY_NAME(AllocConsole,			kernel32),
		HOOK_ITEM_BY_NAME(FreeConsole,			kernel32),
		HOOK_ITEM_BY_NAME(SetConsoleKeyShortcuts, kernel32),
		/* ************************ */
		HOOK_ITEM_BY_NAME(SetConsoleTextAttribute, kernel32),
		HOOK_ITEM_BY_NAME(WriteConsoleOutputW,  kernel32),
		HOOK_ITEM_BY_NAME(WriteConsoleOutputA,  kernel32),
		HOOK_ITEM_BY_NAME(ReadConsoleW,			kernel32),
		HOOK_ITEM_BY_NAME(ReadConsoleA,			kernel32),
		HOOK_ITEM_BY_NAME(PeekConsoleInputW,	kernel32),
		HOOK_ITEM_BY_NAME(PeekConsoleInputA,	kernel32),
		HOOK_ITEM_BY_NAME(ReadConsoleInputW,	kernel32),
		HOOK_ITEM_BY_NAME(ReadConsoleInputA,	kernel32),
		HOOK_ITEM_BY_NAME(WriteConsoleInputA,	kernel32),
		HOOK_ITEM_BY_NAME(WriteConsoleInputW,	kernel32),
		HOOK_ITEM_BY_NAME(WriteConsoleA,  		kernel32),
		HOOK_ITEM_BY_NAME(WriteConsoleW,  		kernel32),
		HOOK_ITEM_BY_NAME(ScrollConsoleScreenBufferA, kernel32),
		HOOK_ITEM_BY_NAME(ScrollConsoleScreenBufferW, kernel32),
		HOOK_ITEM_BY_NAME(WriteConsoleOutputCharacterA, kernel32),
		HOOK_ITEM_BY_NAME(WriteConsoleOutputCharacterW, kernel32),
		/* Others console functions */
		HOOK_ITEM_BY_NAME(GetNumberOfConsoleInputEvents, kernel32),
		HOOK_ITEM_BY_NAME(FlushConsoleInputBuffer, kernel32),
		HOOK_ITEM_BY_NAME(CreateConsoleScreenBuffer, kernel32),
		HOOK_ITEM_BY_NAME(SetConsoleActiveScreenBuffer, kernel32),
		HOOK_ITEM_BY_NAME(SetConsoleWindowInfo, kernel32),
		HOOK_ITEM_BY_NAME(SetConsoleScreenBufferSize, kernel32),
		HOOK_ITEM_BY_NAME(SetCurrentConsoleFontEx, kernel32),
		HOOK_ITEM_BY_NAME(SetConsoleScreenBufferInfoEx, kernel32),
		HOOK_ITEM_BY_NAME(GetLargestConsoleWindowSize, kernel32),
		HOOK_ITEM_BY_NAME(SetConsoleCursorPosition, kernel32),
		HOOK_ITEM_BY_NAME(SetConsoleCursorInfo, kernel32),
		/* ************************ */
		HOOK_ITEM_BY_NAME(GetCurrentConsoleFont,		kernel32),
		HOOK_ITEM_BY_NAME(GetConsoleFontSize,			kernel32),
		/* ************************ */

		// https://conemu.github.io/en/MicrosoftBugs.html#chcp_hung
		HOOK_ITEM_BY_NAME(SetConsoleCP,			kernel32),
		HOOK_ITEM_BY_NAME(SetConsoleOutputCP,	kernel32),
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
		HOOK_ITEM_BY_LIBR(ExitProcess,			kernel32, 0, ghKernel32), // Hook the kernel32.dll, not kernelbase.dll
		HOOK_ITEM_BY_NAME(TerminateProcess,		kernel32),
		/* ************************ */
		HOOK_ITEM_BY_NAME(CreateThread,			kernel32),
		HOOK_ITEM_BY_NAME(SetThreadContext,		kernel32),
		HOOK_ITEM_BY_NAME(TerminateThread,		kernel32),
		/* ************************ */
		HOOK_ITEM_BY_NAME(CreateProcessA,		kernel32),
		HOOK_ITEM_BY_NAME(CreateProcessW,		kernel32),
		/* ************************ */
		HOOK_ITEM_BY_NAME(SetCurrentDirectoryA, kernel32),
		HOOK_ITEM_BY_NAME(SetCurrentDirectoryW, kernel32),
		/* ************************ */
		HOOK_ITEM_BY_NAME(ShellExecuteExA,		shell32),
		HOOK_ITEM_BY_NAME(ShellExecuteExW,		shell32),
		HOOK_ITEM_BY_NAME(ShellExecuteA,		shell32),
		HOOK_ITEM_BY_NAME(ShellExecuteW,		shell32),
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
		HOOK_ITEM_BY_NAME(TrackPopupMenu,		user32),
		HOOK_ITEM_BY_NAME(TrackPopupMenuEx,		user32),
		HOOK_ITEM_BY_NAME(FlashWindow,			user32),
		HOOK_ITEM_BY_NAME(FlashWindowEx,		user32),
		HOOK_ITEM_BY_NAME(SetForegroundWindow,	user32),
		HOOK_ITEM_BY_NAME(GetForegroundWindow,	user32),
		HOOK_ITEM_BY_NAME(GetWindowRect,		user32),
		HOOK_ITEM_BY_NAME(ScreenToClient,		user32),
		/* ************************ */
		//HOOK_ITEM_BY_NAME(CreateWindowA,		user32), -- there is not such export
		//HOOK_ITEM_BY_NAME(CreateWindowW,		user32), -- there is not such export
		HOOK_ITEM_BY_NAME(CreateWindowExA,		user32),
		HOOK_ITEM_BY_NAME(CreateWindowExW,		user32),
		HOOK_ITEM_BY_NAME(ShowCursor,			user32),
		HOOK_ITEM_BY_NAME(ShowWindow,			user32),
		HOOK_ITEM_BY_NAME(SetFocus,				user32),
		HOOK_ITEM_BY_NAME(SetParent,			user32),
		HOOK_ITEM_BY_NAME(GetParent,			user32),
		HOOK_ITEM_BY_NAME(GetWindow,			user32),
		HOOK_ITEM_BY_NAME(GetAncestor,			user32),
		HOOK_ITEM_BY_NAME(GetClassNameA,		user32),
		HOOK_ITEM_BY_NAME(GetClassNameW,		user32),
		HOOK_ITEM_BY_NAME(GetActiveWindow,		user32),
		HOOK_ITEM_BY_NAME(MoveWindow,			user32),
		HOOK_ITEM_BY_NAME(SetWindowPos,			user32),
		HOOK_ITEM_BY_NAME(SetWindowLongA,		user32),
		HOOK_ITEM_BY_NAME(SetWindowLongW,		user32),
		#ifdef WIN64
		HOOK_ITEM_BY_NAME(SetWindowLongPtrA,	user32),
		HOOK_ITEM_BY_NAME(SetWindowLongPtrW,	user32),
		#endif
		HOOK_ITEM_BY_NAME(GetWindowLongA,		user32),
		HOOK_ITEM_BY_NAME(GetWindowLongW,		user32),
		#ifdef WIN64
		HOOK_ITEM_BY_NAME(GetWindowLongPtrA,	user32),
		HOOK_ITEM_BY_NAME(GetWindowLongPtrW,	user32),
		#endif
		HOOK_ITEM_BY_NAME(GetWindowTextLengthA,	user32),
		HOOK_ITEM_BY_NAME(GetWindowTextLengthW,	user32),
		HOOK_ITEM_BY_NAME(GetWindowTextA,		user32),
		HOOK_ITEM_BY_NAME(GetWindowTextW,		user32),
		//
		HOOK_ITEM_BY_NAME(GetWindowPlacement,	user32),
		HOOK_ITEM_BY_NAME(SetWindowPlacement,	user32),
		HOOK_ITEM_BY_NAME(mouse_event,			user32),
		HOOK_ITEM_BY_NAME(SendInput,			user32),
		HOOK_ITEM_BY_NAME(PostMessageA,			user32),
		HOOK_ITEM_BY_NAME(PostMessageW,			user32),
		HOOK_ITEM_BY_NAME(SendMessageA,			user32),
		HOOK_ITEM_BY_NAME(SendMessageW,			user32),
		HOOK_ITEM_BY_NAME(GetMessageA,			user32),
		HOOK_ITEM_BY_NAME(GetMessageW,			user32),
		HOOK_ITEM_BY_NAME(PeekMessageA,			user32),
		HOOK_ITEM_BY_NAME(PeekMessageW,			user32),
		HOOK_ITEM_BY_NAME(MessageBeep,			user32),
		HOOK_ITEM_BY_NAME(CreateDialogParamA,	user32),
		HOOK_ITEM_BY_NAME(CreateDialogParamW,	user32),
		HOOK_ITEM_BY_NAME(CreateDialogIndirectParamA, user32),
		HOOK_ITEM_BY_NAME(CreateDialogIndirectParamW, user32),
		HOOK_ITEM_BY_NAME(DialogBoxIndirectParamAorW, user32),
		HOOK_ITEM_BY_NAME(SetMenu,				user32),
		HOOK_ITEM_BY_NAME(GetDC,				user32),
		HOOK_ITEM_BY_NAME(GetDCEx,				user32),
		HOOK_ITEM_BY_NAME(ReleaseDC,			user32),
		/* ************************ */

		/* ************************ */
		HOOK_ITEM_BY_NAME(StretchDIBits,		gdi32),
		HOOK_ITEM_BY_NAME(BitBlt,				gdi32),
		HOOK_ITEM_BY_NAME(StretchBlt,			gdi32),
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
		HOOK_ITEM_BY_NAME(CompareStringW, kernel32),
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
		HOOK_ITEM_BY_NAME(RegQueryValueExW, IsWin7() ? kernel32 : advapi32),
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
		HOOK_ITEM_BY_NAME(CreateProcessW,		kernel32),
		// Need to "hook" OnCreateProcessA because it is used in "OnWinExec"
		HOOK_ITEM_BY_NAME(CreateProcessA,		kernel32),
		// Used in some programs, Issue 853
		HOOK_ITEM_BY_NAME(WinExec,				kernel32),
		// Need for hook "Run as administrator"
		HOOK_ITEM_BY_NAME(ShellExecuteExW,		shell32),
		{0}
	};
	HookItem HooksAllocConsole[] =
	{
		// gh-888, gh-55: Allow to use ConEmu as default console in third-party applications
		HOOK_ITEM_BY_NAME(AllocConsole,			kernel32), // Only for "*.vshost.exe"?
		{0}
	};
	HookItem HooksCmdLine[] =
	{
		// Issue 1125: "Run as administrator" too. Must be last export
		HOOK_ITEM_BY_ORDN(ShellExecCmdLine,		shell32,   265),
		{0}
	};
	HookItem HooksVshost[] =
	{
		// Issue 1312: .Net applications runs in "*.vshost.exe" helper GUI application when debugging
		// AllocConsole moved to HooksCommon
		HOOK_ITEM_BY_NAME(ShowWindow,			user32),
		/* ************************ */
		{0}
	};
	// Required in VisualStudio and CodeBlocks (gdb) debuggers
	// Don't restrict to them, other Dev Envs may behave in similar way
	HookItem HooksDevStudio[] =
	{
		HOOK_ITEM_BY_NAME(ResumeThread,			kernel32),
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

	if (gbIsNetVsHost || gbPrepareDefaultTerminal)
	{
		if (InitHooks(HooksAllocConsole) < 0)
			goto wrap;
	}

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

	gnDllState |= ds_HooksDefTerm;

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
	if (!(gnDllState & ds_HooksDefTerm))
	{
		if (!InitHooksLibrary())
			return false;
	}

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

	gnDllState |= ds_HooksCommon;

	return true;
}
