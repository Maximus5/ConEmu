
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
#include "../common/WConsole.h"
#include "SetHook.h"

/* *************************** */

extern BOOL gbWasSucceededInRead;

/* *************************** */

void OnReadConsoleStart(bool bUnicode, HANDLE hConsoleInput, LPVOID lpBuffer, DWORD nNumberOfCharsToRead, LPDWORD lpNumberOfCharsRead, MY_CONSOLE_READCONSOLE_CONTROL* pInputControl);
void OnReadConsoleEnd(BOOL bSucceeded, bool bUnicode, HANDLE hConsoleInput, LPVOID lpBuffer, DWORD nNumberOfCharsToRead, LPDWORD lpNumberOfCharsRead, MY_CONSOLE_READCONSOLE_CONTROL* pInputControl);

/* *************************** */

HOOK_PROTOTYPE(FlushConsoleInputBuffer,BOOL,WINAPI,(HANDLE hConsoleInput));
HOOK_PROTOTYPE(GetNumberOfConsoleInputEvents,BOOL,WINAPI,(HANDLE hConsoleInput, LPDWORD lpcNumberOfEvents));
HOOK_PROTOTYPE(PeekConsoleInputA,BOOL,WINAPI,(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead));
HOOK_PROTOTYPE(PeekConsoleInputW,BOOL,WINAPI,(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead));
HOOK_PROTOTYPE(ReadConsoleA,BOOL,WINAPI,(HANDLE hConsoleInput, LPVOID lpBuffer, DWORD nNumberOfCharsToRead, LPDWORD lpNumberOfCharsRead, LPVOID pInputControl));
HOOK_PROTOTYPE(ReadConsoleInputA,BOOL,WINAPI,(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead));
HOOK_PROTOTYPE(ReadConsoleInputW,BOOL,WINAPI,(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead));
HOOK_PROTOTYPE(ReadConsoleW,BOOL,WINAPI,(HANDLE hConsoleInput, LPVOID lpBuffer, DWORD nNumberOfCharsToRead, LPDWORD lpNumberOfCharsRead, MY_CONSOLE_READCONSOLE_CONTROL* pInputControl));
HOOK_PROTOTYPE(WriteConsoleInputA,BOOL,WINAPI,(HANDLE hConsoleInput, const INPUT_RECORD *lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsWritten));
HOOK_PROTOTYPE(WriteConsoleInputW,BOOL,WINAPI,(HANDLE hConsoleInput, const INPUT_RECORD *lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsWritten));
