
/*
Copyright (c) 2009-present Maximus5
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

#include "../common/defines.h"
#include "../common/InQueue.h"
#include "ConEmuHooks.h"
#include "DllOptions.h"

HMODULE ghOurModule = nullptr;

wchar_t gsConEmuBaseDir[MAX_PATH + 1] = L"";

// Executable name
wchar_t gsExeName[80] = L"";
// cygwin/msys/clink and so on...
CEActiveAppFlags gnExeFlags = caf_Standard;

CEStartupEnv* gpStartEnv = nullptr;
bool    gbConEmuCProcess = false;
bool    gbConEmuConnector = false;
DWORD   gnSelfPID = 0;
BOOL    gbSelfIsRootConsoleProcess = FALSE;
BOOL    gbForceStartPipeServer = FALSE;
DWORD   gnServerPID = 0;
DWORD   gnPrevAltServerPID = 0;
DWORD   gnGuiPID = 0;
HWND    ghConWnd = nullptr; // Console window
HWND    ghConEmuWnd = nullptr; // Root! window
HWND    ghConEmuWndDC = nullptr; // ConEmu DC window
HWND    ghConEmuWndBack = nullptr; // ConEmu Back window - holder for GUI client
BOOL    gbWasBufferHeight = FALSE;
BOOL    gbNonGuiMode = FALSE;
DWORD   gnImageSubsystem = 0;
DWORD   gnImageBits = WIN3264TEST(32,64); //-V112
wchar_t gsInitConTitle[512] = {};
BOOL    gbTrueColorServerRequested = FALSE;


/* ************ Globals for Far Hooks ************ */
HookModeFar gFarMode = {sizeof(HookModeFar), 0, 0, 0, 0, 0, 0, {}, nullptr};
bool    gbIsFarProcess = false;
InQueue gInQueue = {};
HANDLE  ghConsoleCursorChanged = nullptr;
SrvLogString_t gfnSrvLogString = nullptr;
/* ************ Globals for Far Hooks ************ */

/* ************ Globals for cmd.exe/clink ************ */
bool     gbIsCmdProcess = false;
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
/* ************ Globals for powershell ************ */

/* ************ Globals for Node.JS ************ */
bool gbIsNodeJsProcess = false;
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


/* ************ Globals for "Default terminal ************ */
bool gbPrepareDefaultTerminal = false;
bool gbIsNetVsHost = false;
bool gbIsVStudio = false;
bool gbIsVSDebug = false; // msvsmon.exe
bool gbIsVsCode = false;
int  gnVsHostStartConsole = 0;
bool gbIsGdbHost = false;
CDefTermHk* gpDefTerm = nullptr;
/* ************ Globals for "Default terminal ************ */



// ReSharper disable twice CppParameterMayBeConst
void SetConEmuHkWindows(HWND hDcWnd, HWND hBackWnd)
{
	ghConEmuWndDC = hDcWnd;
	ghConEmuWndBack = hBackWnd;
}

void SetServerPID(const DWORD anMainSrvPid)
{
	gnServerPID = anMainSrvPid;
}
