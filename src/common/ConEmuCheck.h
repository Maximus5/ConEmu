
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


#pragma once

#include "common.hpp"

#define VirtualConsoleClass L"VirtualConsoleClass"
#define VirtualConsoleClassMain L"VirtualConsoleClass"
#define VirtualConsoleClassApp L"VirtualConsoleClassApp"
#define VirtualConsoleClassBack L"VirtualConsoleClassBack"
#define VirtualConsoleClassScroll L"VirtualConsoleClassScroll"

#ifdef _DEBUG
	#define EXECUTE_CMD_WARN_TIMEOUT 500
	#define EXECUTE_CMD_TIMEOUT_SRV_ABSENT 500
#endif

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


// Returns HWND of Gui console DC window (abRoot==FALSE),
//              or Gui Main window (abRoot==TRUE)
HWND GetConEmuHWND(BOOL abRoot);

// hConEmuWnd - HWND с отрисовкой!
void SetConEmuEnvVar(HWND hConEmuWnd);

//LPCWSTR CreatePipeName(wchar_t (&szGuiPipeName)[128], LPCWSTR asFormat, DWORD anValue);
int GuiMessageBox(HWND hConEmuWndRoot, LPCWSTR asText, LPCWSTR asTitle, int anBtns);

HANDLE ExecuteOpenPipe(const wchar_t* szPipeName, wchar_t (&szErr)[MAX_PATH*2], const wchar_t* szModule);
CESERVER_REQ* ExecuteNewCmd(DWORD nCmd, DWORD nSize);
void ExecutePrepareCmd(CESERVER_REQ* pIn, DWORD nCmd, DWORD cbSize);
void ExecutePrepareCmd(CESERVER_REQ_HDR* pHdr, DWORD nCmd, DWORD cbSize);
CESERVER_REQ* ExecuteGuiCmd(HWND hConWnd, const CESERVER_REQ* pIn, HWND hOwner);
CESERVER_REQ* ExecuteSrvCmd(DWORD dwSrvPID, const CESERVER_REQ* pIn, HWND hOwner);
CESERVER_REQ* ExecuteCmd(const wchar_t* szGuiPipeName, const CESERVER_REQ* pIn, DWORD nWaitPipe, HWND hOwner);
void ExecuteFreeResult(CESERVER_REQ* pOut);

BOOL LoadSrvMapping(HWND hConWnd, CESERVER_CONSOLE_MAPPING_HDR& SrvMapping);
#ifndef CONEMU_MINIMAL
BOOL LoadGuiMapping(DWORD nConEmuPID, ConEmuGuiMapping& GuiMapping);
BOOL LoadGuiMapping(HWND hConEmuWnd, ConEmuGuiMapping& GuiMapping);
#endif
CESERVER_REQ* ExecuteNewCmdOnCreate(enum CmdOnCreateType aCmd,
				LPCWSTR asAction, LPCWSTR asFile, LPCWSTR asParam,
				DWORD* anShellFlags, DWORD* anCreateFlags, DWORD* anStartFlags, DWORD* anShowCmd,
				int mn_ImageBits, int mn_ImageSubsystem,
				HANDLE hStdIn, HANDLE hStdOut, HANDLE hStdErr);

BOOL FindConEmuBaseDir(wchar_t (&rsConEmuBaseDir)[MAX_PATH+1], wchar_t (&rsConEmuExe)[MAX_PATH+1]);

HWND myGetConsoleWindow();

extern SECURITY_ATTRIBUTES* gpLocalSecurity;
