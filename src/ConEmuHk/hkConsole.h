
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

#if __GNUC__
extern "C" {
#ifndef GetConsoleAliases
	DWORD WINAPI GetConsoleAliasesA(LPSTR AliasBuffer, DWORD AliasBufferLength, LPSTR ExeName);
	DWORD WINAPI GetConsoleAliasesW(LPWSTR AliasBuffer, DWORD AliasBufferLength, LPWSTR ExeName);
#define GetConsoleAliases  GetConsoleAliasesW
#endif
}
#endif

/* *************************** */

// SetConsoleTitleA
typedef BOOL (WINAPI* OnSetConsoleTitleA_t)(LPCSTR lpConsoleTitle);
BOOL WINAPI OnSetConsoleTitleA(LPCSTR lpConsoleTitle);

// SetConsoleTitleW
typedef BOOL (WINAPI* OnSetConsoleTitleW_t)(LPCWSTR lpConsoleTitle);
BOOL WINAPI OnSetConsoleTitleW(LPCWSTR lpConsoleTitle);

// GetConsoleAliasesW
typedef DWORD (WINAPI* OnGetConsoleAliasesW_t)(LPWSTR AliasBuffer, DWORD AliasBufferLength, LPWSTR ExeName);
DWORD WINAPI OnGetConsoleAliasesW(LPWSTR AliasBuffer, DWORD AliasBufferLength, LPWSTR ExeName);

// SetConsoleCP
typedef BOOL (WINAPI* OnSetConsoleCP_t)(UINT wCodePageID);
BOOL WINAPI OnSetConsoleCP(UINT wCodePageID);

// SetConsoleOutputCP
typedef BOOL (WINAPI* OnSetConsoleOutputCP_t)(UINT wCodePageID);
BOOL WINAPI OnSetConsoleOutputCP(UINT wCodePageID);

// AllocConsole
typedef BOOL (WINAPI* OnAllocConsole_t)(void);
BOOL WINAPI OnAllocConsole(void);

// FreeConsole
typedef BOOL (WINAPI* OnFreeConsole_t)(void);
BOOL WINAPI OnFreeConsole(void);

// GetConsoleWindow
typedef HWND (WINAPI* OnGetConsoleWindow_t)(void);
HWND WINAPI OnGetConsoleWindow(void);

// SetConsoleKeyShortcuts
typedef BOOL (WINAPI* OnSetConsoleKeyShortcuts_t)(BOOL,BYTE,LPVOID,DWORD);
BOOL WINAPI OnSetConsoleKeyShortcuts(BOOL bSet, BYTE bReserveKeys, LPVOID p1, DWORD n1);

// GetCurrentConsoleFont
typedef BOOL (WINAPI* OnGetCurrentConsoleFont_t)(HANDLE hConsoleOutput, BOOL bMaximumWindow, PCONSOLE_FONT_INFO lpConsoleCurrentFont);
BOOL WINAPI OnGetCurrentConsoleFont(HANDLE hConsoleOutput, BOOL bMaximumWindow, PCONSOLE_FONT_INFO lpConsoleCurrentFont);

// GetConsoleFontSize
typedef COORD (WINAPI* OnGetConsoleFontSize_t)(HANDLE hConsoleOutput, DWORD nFont);
COORD WINAPI OnGetConsoleFontSize(HANDLE hConsoleOutput, DWORD nFont);

// CreateConsoleScreenBuffer
typedef HANDLE(WINAPI* OnCreateConsoleScreenBuffer_t)(DWORD dwDesiredAccess, DWORD dwShareMode, const SECURITY_ATTRIBUTES *lpSecurityAttributes, DWORD dwFlags, LPVOID lpScreenBufferData);
HANDLE WINAPI OnCreateConsoleScreenBuffer(DWORD dwDesiredAccess, DWORD dwShareMode, const SECURITY_ATTRIBUTES *lpSecurityAttributes, DWORD dwFlags, LPVOID lpScreenBufferData);

// SetConsoleActiveScreenBuffer
typedef BOOL (WINAPI* OnSetConsoleActiveScreenBuffer_t)(HANDLE hConsoleOutput);
BOOL WINAPI OnSetConsoleActiveScreenBuffer(HANDLE hConsoleOutput);

// SetConsoleWindowInfo
typedef BOOL (WINAPI* OnSetConsoleWindowInfo_t)(HANDLE hConsoleOutput, BOOL bAbsolute, const SMALL_RECT *lpConsoleWindow);
BOOL WINAPI OnSetConsoleWindowInfo(HANDLE hConsoleOutput, BOOL bAbsolute, const SMALL_RECT *lpConsoleWindow);

// SetConsoleScreenBufferSize
typedef BOOL (WINAPI* OnSetConsoleScreenBufferSize_t)(HANDLE hConsoleOutput, COORD dwSize);
BOOL WINAPI OnSetConsoleScreenBufferSize(HANDLE hConsoleOutput, COORD dwSize);

// SetCurrentConsoleFontEx
typedef BOOL (WINAPI* OnSetCurrentConsoleFontEx_t)(HANDLE hConsoleOutput, BOOL bMaximumWindow, MY_CONSOLE_FONT_INFOEX* lpConsoleCurrentFontEx);
BOOL WINAPI OnSetCurrentConsoleFontEx(HANDLE hConsoleOutput, BOOL bMaximumWindow, MY_CONSOLE_FONT_INFOEX* lpConsoleCurrentFontEx);

// SetConsoleScreenBufferInfoEx
typedef BOOL (WINAPI* OnSetConsoleScreenBufferInfoEx_t)(HANDLE hConsoleOutput, MY_CONSOLE_SCREEN_BUFFER_INFOEX* lpConsoleScreenBufferInfoEx);
BOOL WINAPI OnSetConsoleScreenBufferInfoEx(HANDLE hConsoleOutput, MY_CONSOLE_SCREEN_BUFFER_INFOEX* lpConsoleScreenBufferInfoEx);

// GetLargestConsoleWindowSize
typedef COORD (WINAPI* OnGetLargestConsoleWindowSize_t)(HANDLE hConsoleOutput);
COORD WINAPI OnGetLargestConsoleWindowSize(HANDLE hConsoleOutput);

// SetConsoleCursorPosition
typedef BOOL (WINAPI* OnSetConsoleCursorPosition_t)(HANDLE,COORD);
BOOL WINAPI OnSetConsoleCursorPosition(HANDLE hConsoleOutput, COORD dwCursorPosition);

// SetConsoleCursorInfo
typedef BOOL (WINAPI* OnSetConsoleCursorInfo_t)(HANDLE,const CONSOLE_CURSOR_INFO *);
BOOL WINAPI OnSetConsoleCursorInfo(HANDLE hConsoleOutput, const CONSOLE_CURSOR_INFO *lpConsoleCursorInfo);

