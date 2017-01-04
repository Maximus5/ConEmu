
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


#pragma once

#include "Common.h"

#define EXECUTE_CONNECT_GUI_CALL_TIMEOUT 1000
#define EXECUTE_CONNECT_GUI_TIMEOUT (EXECUTE_CONNECT_GUI_CALL_TIMEOUT*10)

#ifdef _DEBUG
	#define EXECUTE_CMD_WARN_TIMEOUT 500
	#define EXECUTE_CMD_WARN_TIMEOUT2 10000
	#define EXECUTE_CMD_TIMEOUT_SRV_ABSENT 500
#endif

#define EXECUTE_CMD_OPENPIPE_TIMEOUT 30000
#define EXECUTE_CMD_CALLPIPE_TIMEOUT 1000

enum CmdOnCreateType
{
	eShellExecute = 1,
	eCreateProcess,
	eInjectingHooks,
	eHooksLoaded,
	eParmsChanged,
	eLoadLibrary,
	eFreeLibrary,
	eSrvLoaded,
};

// Service function
//HWND AtoH(char *Str, int Len);


// 0 -- All OK (ConEmu found, Version OK)
// 1 -- NO ConEmu (simple console mode)
int ConEmuCheck(HWND* ahConEmuWnd);


// Returns HWND of ...
//  aiType==0: Gui console DC window
//        ==1: Gui Main window
//        ==2: Console window
//        ==3: Back window
HWND GetConEmuHWND(int aiType);

// RealConsole window
HWND myGetConsoleWindow();

// Don't use gfGetRealConsoleWindow indirectly,
// Use myGetConsoleWindow instead!
typedef HWND(WINAPI* GetConsoleWindow_T)();
//extern GetConsoleWindow_T gfGetRealConsoleWindow;

bool isConsoleClass(LPCWSTR asClass);
bool isConsoleWindow(HWND hWnd);

//LPCWSTR CreatePipeName(wchar_t (&szGuiPipeName)[128], LPCWSTR asFormat, DWORD anValue);
int GuiMessageBox(HWND hConEmuWndRoot, LPCWSTR asText, LPCWSTR asTitle, int anBtns);

HANDLE ExecuteOpenPipe(const wchar_t* szPipeName, wchar_t (&szErr)[MAX_PATH*2], const wchar_t* szModule, DWORD nServerPID = 0, DWORD nTimeout = 0, BOOL Overlapped = FALSE, HANDLE hStop = NULL, BOOL bIgnoreAbsence = FALSE);
CESERVER_REQ* ExecuteNewCmd(DWORD nCmd, size_t nSize);
bool ExecuteNewCmd(CESERVER_REQ* &ppCmd, DWORD &pcbCurMaxSize, DWORD nCmd, size_t nSize);
void ExecutePrepareCmd(CESERVER_REQ* pIn, DWORD nCmd, size_t cbSize);
void ExecutePrepareCmd(CESERVER_REQ_HDR* pHdr, DWORD nCmd, size_t cbSize);
CESERVER_REQ* ExecuteGuiCmd(HWND hConWnd, CESERVER_REQ* pIn, HWND hOwner, BOOL bAsyncNoResult = FALSE);
CESERVER_REQ* ExecuteSrvCmd(DWORD dwSrvPID, CESERVER_REQ* pIn, HWND hOwner, BOOL bAsyncNoResult = FALSE, DWORD nTimeout = 0, BOOL bIgnoreAbsence = FALSE);
CESERVER_REQ* ExecuteHkCmd(DWORD dwHkPID, CESERVER_REQ* pIn, HWND hOwner, BOOL bAsyncNoResult = FALSE, BOOL bIgnoreAbsence = FALSE);
CESERVER_REQ* ExecuteCmd(const wchar_t* szGuiPipeName, CESERVER_REQ* pIn, DWORD nWaitPipe, HWND hOwner, BOOL bAsyncNoResult = FALSE, DWORD nServerPID = 0, BOOL bIgnoreAbsence = FALSE);
void ExecuteFreeResult(CESERVER_REQ* &pOut);

bool AllocateSendCurrentDirectory(CESERVER_REQ* &ppCmd, DWORD &pcbCurMaxSize, LPCWSTR asDirectory, LPCWSTR asPassiveDirectory = NULL);
void SendCurrentDirectory(HWND hConWnd, LPCWSTR asDirectory, LPCWSTR asPassiveDirectory = NULL);

BOOL LoadSrvMapping(HWND hConWnd, CESERVER_CONSOLE_MAPPING_HDR& SrvMapping);
BOOL LoadGuiMapping(DWORD nConEmuPID, ConEmuGuiMapping& GuiMapping);
CESERVER_REQ* ExecuteNewCmdOnCreate(CESERVER_CONSOLE_MAPPING_HDR* pSrvMap, HWND hConWnd, enum CmdOnCreateType aCmd,
				LPCWSTR asAction, LPCWSTR asFile, LPCWSTR asParam, LPCWSTR asDir,
				DWORD* anShellFlags, DWORD* anCreateFlags, DWORD* anStartFlags, DWORD* anShowCmd,
				int mn_ImageBits, int mn_ImageSubsystem,
				HANDLE hStdIn, HANDLE hStdOut, HANDLE hStdErr);

extern SECURITY_ATTRIBUTES* gpLocalSecurity;
extern u64 ghWorkingModule;

#ifdef _DEBUG
extern bool gbPipeDebugBoxes;
#endif
