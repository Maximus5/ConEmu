
/*
Copyright (c) 2015 Maximus5
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

/* *************************** */

#ifndef SEE_MASK_NOZONECHECKS
#define SEE_MASK_NOZONECHECKS      0x00800000
#endif

/* *************************** */

// ExitProcess
typedef BOOL (WINAPI* OnExitProcess_t)(UINT uExitCode);
VOID WINAPI OnExitProcess(UINT uExitCode);

// TerminateProcess
typedef BOOL (WINAPI* OnTerminateProcess_t)(HANDLE hProcess, UINT uExitCode);
BOOL WINAPI OnTerminateProcess(HANDLE hProcess, UINT uExitCode);

// TerminateThread
typedef BOOL (WINAPI* OnTerminateThread_t)(HANDLE hThread, UINT dwExitCode);
BOOL WINAPI OnTerminateThread(HANDLE hThread, DWORD dwExitCode);

// CreateProcessA
typedef BOOL (WINAPI* OnCreateProcessA_t)(LPCSTR lpApplicationName,  LPSTR lpCommandLine,  LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory,  LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
BOOL WINAPI OnCreateProcessA(LPCSTR lpApplicationName,  LPSTR lpCommandLine,  LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory,  LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);

// CreateProcessW
typedef BOOL (WINAPI* OnCreateProcessW_t)(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
BOOL WINAPI OnCreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);

// WinExec
typedef BOOL (WINAPI* OnWinExec_t)(LPCSTR lpCmdLine, UINT uCmdShow);
UINT WINAPI OnWinExec(LPCSTR lpCmdLine, UINT uCmdShow);

// SetCurrentDirectoryA
typedef BOOL (WINAPI* OnSetCurrentDirectoryA_t)(LPCSTR lpPathName);
BOOL WINAPI OnSetCurrentDirectoryA(LPCSTR lpPathName);

// SetCurrentDirectoryW
typedef BOOL (WINAPI* OnSetCurrentDirectoryW_t)(LPCWSTR lpPathName);
BOOL WINAPI OnSetCurrentDirectoryW(LPCWSTR lpPathName);

// SetThreadContext
typedef BOOL (WINAPI* OnSetThreadContext_t)(HANDLE hThread, CONST CONTEXT *lpContext);
BOOL WINAPI OnSetThreadContext(HANDLE hThread, CONST CONTEXT *lpContext);

// ShellExecuteExA
typedef BOOL (WINAPI* OnShellExecuteExA_t)(LPSHELLEXECUTEINFOA lpExecInfo);
BOOL WINAPI OnShellExecuteExA(LPSHELLEXECUTEINFOA lpExecInfo);

// ShellExecuteExW
typedef BOOL (WINAPI* OnShellExecuteExW_t)(LPSHELLEXECUTEINFOW lpExecInfo);
BOOL WINAPI OnShellExecuteExW(LPSHELLEXECUTEINFOW lpExecInfo);

// ShellExecCmdLine
typedef HRESULT (WINAPI* OnShellExecCmdLine_t)(HWND hwnd, LPCWSTR pwszCommand, LPCWSTR pwszStartDir, int nShow, LPVOID pUnused, DWORD dwSeclFlags);
HRESULT WINAPI OnShellExecCmdLine(HWND hwnd, LPCWSTR pwszCommand, LPCWSTR pwszStartDir, int nShow, LPVOID pUnused, DWORD dwSeclFlags);

// ShellExecuteA
typedef HINSTANCE(WINAPI* OnShellExecuteA_t)(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd);
HINSTANCE WINAPI OnShellExecuteA(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd);

// ShellExecuteW
typedef HINSTANCE(WINAPI* OnShellExecuteW_t)(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd);
HINSTANCE WINAPI OnShellExecuteW(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd);

// CreateThread
typedef HANDLE(WINAPI* OnCreateThread_t)(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);
HANDLE WINAPI OnCreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);

// ResumeThread
typedef DWORD (WINAPI* OnResumeThread_t)(HANDLE);
DWORD WINAPI OnResumeThread(HANDLE hThread);

