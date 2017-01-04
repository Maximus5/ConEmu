
/*
Copyright (c) 2015-2017 Maximus5
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

#include <windows.h>
#include "SetHook.h"

/* *************************** */

#ifndef SEE_MASK_NOZONECHECKS
#define SEE_MASK_NOZONECHECKS      0x00800000
#endif

/* *************************** */

HRESULT OurShellExecCmdLine(HWND hwnd, LPCWSTR pwszCommand, LPCWSTR pwszStartDir, bool bRunAsAdmin, bool bForce);

/* *************************** */

HOOK_PROTOTYPE(CreateProcessA,BOOL,WINAPI,(LPCSTR lpApplicationName,  LPSTR lpCommandLine,  LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory,  LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation));
HOOK_PROTOTYPE(CreateProcessW,BOOL,WINAPI,(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation));
HOOK_PROTOTYPE(CreateThread,HANDLE,WINAPI,(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId));
HOOK_PROTOTYPE(ExitProcess,VOID,WINAPI,(UINT uExitCode));
HOOK_PROTOTYPE(ResumeThread,DWORD,WINAPI,(HANDLE hThread));
HOOK_PROTOTYPE(SetCurrentDirectoryA,BOOL,WINAPI,(LPCSTR lpPathName));
HOOK_PROTOTYPE(SetCurrentDirectoryW,BOOL,WINAPI,(LPCWSTR lpPathName));
HOOK_PROTOTYPE(SetThreadContext,BOOL,WINAPI,(HANDLE hThread, CONST CONTEXT *lpContext));
HOOK_PROTOTYPE(ShellExecCmdLine,HRESULT,WINAPI,(HWND hwnd, LPCWSTR pwszCommand, LPCWSTR pwszStartDir, int nShow, LPVOID pUnused, DWORD dwSeclFlags));
HOOK_PROTOTYPE(ShellExecuteA,HINSTANCE,WINAPI,(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd));
HOOK_PROTOTYPE(ShellExecuteExA,BOOL,WINAPI,(LPSHELLEXECUTEINFOA lpExecInfo));
HOOK_PROTOTYPE(ShellExecuteExW,BOOL,WINAPI,(LPSHELLEXECUTEINFOW lpExecInfo));
HOOK_PROTOTYPE(ShellExecuteW,HINSTANCE,WINAPI,(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd));
HOOK_PROTOTYPE(TerminateProcess,BOOL,WINAPI,(HANDLE hProcess, UINT uExitCode));
HOOK_PROTOTYPE(TerminateThread,BOOL,WINAPI,(HANDLE hThread, DWORD dwExitCode));
HOOK_PROTOTYPE(WinExec,UINT,WINAPI,(LPCSTR lpCmdLine, UINT uCmdShow)); // Kernel32.dll
