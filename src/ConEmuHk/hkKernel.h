
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

/* *************************** */

#ifdef _DEBUG
// VirtualAlloc
typedef LPVOID(WINAPI* OnVirtualAlloc_t)(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
LPVOID WINAPI OnVirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
#endif

#ifdef _DEBUG
// VirtualProtect
typedef BOOL(WINAPI* OnVirtualProtect_t)(LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect);
BOOL WINAPI OnVirtualProtect(LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect);
#endif

#ifdef _DEBUG
// SetUnhandledExceptionFilter
typedef LPTOP_LEVEL_EXCEPTION_FILTER(WINAPI* OnSetUnhandledExceptionFilter_t)(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter);
LPTOP_LEVEL_EXCEPTION_FILTER WINAPI OnSetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter);
#endif

// GetSystemTime
typedef void (WINAPI* OnGetSystemTime_t)(LPSYSTEMTIME);
void WINAPI OnGetSystemTime(LPSYSTEMTIME lpSystemTime);

// GetLocalTime
typedef void (WINAPI* OnGetLocalTime_t)(LPSYSTEMTIME);
void WINAPI OnGetLocalTime(LPSYSTEMTIME lpSystemTime);

// GetSystemTimeAsFileTime
typedef void (WINAPI* OnGetSystemTimeAsFileTime_t)(LPFILETIME);
void WINAPI OnGetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime);

// GetLastError
typedef DWORD (WINAPI* OnGetLastError_t)();
DWORD WINAPI OnGetLastError();

// SetLastError
typedef DWORD (WINAPI* OnSetLastError_t)(DWORD dwErrCode);
VOID WINAPI OnSetLastError(DWORD dwErrCode);

