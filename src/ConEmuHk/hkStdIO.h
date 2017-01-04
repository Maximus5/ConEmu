
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

extern HANDLE ghLastConInHandle;
extern HANDLE ghLastNotConInHandle;

extern HANDLE ghSkipSetThreadContextForThread; // Injects.cpp

/* *************************** */

HOOK_PROTOTYPE(CloseHandle,BOOL,WINAPI,(HANDLE hObject));
HOOK_PROTOTYPE(CreateFileA,HANDLE,WINAPI,(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile));
HOOK_PROTOTYPE(CreateFileW,HANDLE,WINAPI,(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile));
HOOK_PROTOTYPE(MapViewOfFile,LPVOID,WINAPI,(HANDLE hFileMappingObject, DWORD dwDesiredAccess, DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow, SIZE_T dwNumberOfBytesToMap));
HOOK_PROTOTYPE(OpenFileMappingW,HANDLE,WINAPI,(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName));
HOOK_PROTOTYPE(ReadFile,BOOL,WINAPI,(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped));
HOOK_PROTOTYPE(SetStdHandle,BOOL,WINAPI,(DWORD nStdHandle, HANDLE hHandle));
HOOK_PROTOTYPE(UnmapViewOfFile,BOOL,WINAPI,(LPCVOID lpBaseAddress));
HOOK_PROTOTYPE(WriteFile,BOOL,WINAPI,(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped));

#ifdef _DEBUG
HOOK_PROTOTYPE(CreateNamedPipeW,HANDLE,WINAPI,(LPCWSTR lpName, DWORD dwOpenMode, DWORD dwPipeMode, DWORD nMaxInstances,DWORD nOutBufferSize, DWORD nInBufferSize, DWORD nDefaultTimeOut,LPSECURITY_ATTRIBUTES lpSecurityAttributes));
#endif
