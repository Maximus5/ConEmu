
/*
Copyright (c) 2015-2016 Maximus5
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

HOOK_PROTOTYPE(GetConsoleMode,BOOL,WINAPI,(HANDLE hConsoleHandle,LPDWORD lpMode));
HOOK_PROTOTYPE(ScrollConsoleScreenBufferA,BOOL,WINAPI,(HANDLE hConsoleOutput, const SMALL_RECT *lpScrollRectangle, const SMALL_RECT *lpClipRectangle, COORD dwDestinationOrigin, const CHAR_INFO *lpFill));
HOOK_PROTOTYPE(ScrollConsoleScreenBufferW,BOOL,WINAPI,(HANDLE hConsoleOutput, const SMALL_RECT *lpScrollRectangle, const SMALL_RECT *lpClipRectangle, COORD dwDestinationOrigin, const CHAR_INFO *lpFill));
HOOK_PROTOTYPE(SetConsoleMode,BOOL,WINAPI,(HANDLE hConsoleHandle, DWORD dwMode));
HOOK_PROTOTYPE(SetConsoleTextAttribute,BOOL,WINAPI,(HANDLE hConsoleOutput, WORD wAttributes));
HOOK_PROTOTYPE(WriteConsoleA,BOOL,WINAPI,(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved));
HOOK_PROTOTYPE(WriteConsoleOutputA,BOOL,WINAPI,(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion));
HOOK_PROTOTYPE(WriteConsoleOutputCharacterA,BOOL,WINAPI,(HANDLE hConsoleOutput, LPCSTR lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten));
HOOK_PROTOTYPE(WriteConsoleOutputCharacterW,BOOL,WINAPI,(HANDLE hConsoleOutput, LPCWSTR lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten));
HOOK_PROTOTYPE(WriteConsoleOutputW,BOOL,WINAPI,(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion));
HOOK_PROTOTYPE(WriteConsoleW,BOOL,WINAPI,(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved));
