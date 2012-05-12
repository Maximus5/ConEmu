
/*
Copyright (c) 2012 Maximus5
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

#define DEFINE_HOOK_MACROS

#include <windows.h>
#include <WinError.h>
#include <WinNT.h>
#include <TCHAR.h>
#include "../common/common.hpp"
#include "../common/ConEmuCheck.h"
#include "SetHook.h"
#include "ConEmuHooks.h"
#include "UserImp.h"
#include "../common/ConsoleAnnotation.h"

#undef isPressed
#define isPressed(inp) ((user->getKeyState(inp) & 0x8000) == 0x8000)

#ifdef _DEBUG
#define DebugString(x) OutputDebugString(x)
#else
#define DebugString(x) //OutputDebugString(x)
#endif

extern HMODULE ghOurModule; // Хэндл нашей dll'ки (здесь хуки не ставятся)
extern DWORD   gnHookMainThreadId;
extern BOOL    gbHooksTemporaryDisabled;

/* ************ Globals for SetHook ************ */
//HWND ghConEmuWndDC = NULL; // Содержит хэндл окна отрисовки. Это ДОЧЕРНЕЕ окно.
extern HWND    ghConWnd;      // RealConsole
extern HWND    ghConEmuWnd;   // Root! ConEmu window
extern HWND    ghConEmuWndDC; // ConEmu DC window
extern DWORD   gnGuiPID;
extern GetConsoleWindow_T gfGetRealConsoleWindow;
extern HWND WINAPI GetRealConsoleWindow(); // Entry.cpp
extern HANDLE ghCurrentOutBuffer;
extern HANDLE ghStdOutHandle;
/* ************ Globals for SetHook ************ */

HANDLE ghLastAnsiCapable = NULL;
HANDLE ghLastAnsiNotCapable = NULL;

BOOL WINAPI OnWriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
BOOL WINAPI OnWriteConsoleA(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved);
BOOL WINAPI OnWriteConsoleW(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved);

bool IsAnsiCapable(HANDLE hFile)
{
	if (!hFile)
		return false;

	TODO("Проверять наличие ConEmu?");

	if (hFile == ghLastAnsiCapable)
		return true;
	else if (hFile == ghLastAnsiNotCapable)
		return false;

	DWORD Mode = 0;
	BOOL  Processed = FALSE;

	// Оптимизация.
	WARNING("Проверять ENABLE_PROCESSED_OUTPUT будем?");
	if (GetConsoleMode(hFile, &Mode))
	{
		Processed = (Mode & ENABLE_PROCESSED_OUTPUT);
		ghLastAnsiCapable = hFile;
		return true;
	}

	ghLastAnsiNotCapable = hFile;
	return false;
}

BOOL WINAPI OnWriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
	typedef BOOL (WINAPI* OnWriteFile_t)(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
	ORIGINALFAST(WriteFile);
	BOOL bMainThread = FALSE; // поток не важен
	BOOL lbRc = FALSE;

	if (lpBuffer && nNumberOfBytesToWrite && IsAnsiCapable(hFile))
		lbRc = OnWriteConsoleA(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, NULL);
	else
		lbRc = F(WriteFile)(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);

	return lbRc;
}

BOOL WINAPI OnWriteConsoleA(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved)
{
	typedef BOOL (WINAPI* OnWriteConsoleA_t)(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved);
	ORIGINALFAST(WriteConsoleA);
	BOOL bMainThread = FALSE; // поток не важен
	BOOL lbRc = FALSE;

	if (lpBuffer && nNumberOfCharsToWrite && IsAnsiCapable(hConsoleOutput))
	{
		wchar_t* buf = NULL;
		DWORD len, cp;
		
		cp = GetConsoleOutputCP();
		len = MultiByteToWideChar(cp, 0, (LPCSTR)lpBuffer, nNumberOfCharsToWrite, 0, 0);
		buf = (wchar_t*)malloc((len+1)*sizeof(wchar_t));
		if (buf == NULL)
		{
			if (lpNumberOfCharsWritten != NULL)
				*lpNumberOfCharsWritten = 0;
		}
		else
		{
			buf[len] = 0;
			DWORD newLen = MultiByteToWideChar(cp, 0, (LPCSTR)lpBuffer, nNumberOfCharsToWrite, buf, len);
			_ASSERTE(newLen==len);

			lbRc = OnWriteConsoleW(hConsoleOutput, buf, len, lpNumberOfCharsWritten, NULL);

			free( buf );
		}
	}
	else
	{
		lbRc = F(WriteConsoleA)(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, lpReserved);
	}

	return lbRc;
}

BOOL WINAPI OnWriteConsoleW(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved)
{
	typedef BOOL (WINAPI* OnWriteConsoleW_t)(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved);
	ORIGINALFAST(WriteConsoleW);
	BOOL bMainThread = FALSE; // поток не важен
	BOOL lbRc = FALSE;

	#ifdef _DEBUG
	const wchar_t* pch = (const wchar_t*)lpBuffer;
	#endif

	lbRc = F(WriteConsoleW)(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, lpReserved);

	return lbRc;
}
