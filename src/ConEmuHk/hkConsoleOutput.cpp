
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

#define HIDE_USE_EXCEPTION_INFO

#ifdef _DEBUG
	#define DebugString(x) //OutputDebugString(x)
#else
	#define DebugString(x) //OutputDebugString(x)
#endif

#include "../common/common.hpp"
#include "../common/MConHandle.h"

#include "Ansi.h"
#include "ExtConsole.h"
#include "GuiAttach.h"
#include "hkConsoleOutput.h"
#include "MainThread.h"

/* **************** */

extern void CheckPowerShellProgress(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);

/* **************** */

// Spare?
BOOL WINAPI OnGetConsoleMode(HANDLE hConsoleHandle,LPDWORD lpMode)
{
	//typedef BOOL (WINAPI* OnGetConsoleMode_t)(HANDLE,LPDWORD);
	ORIGINAL_KRNL(GetConsoleMode);
	BOOL b;

	b = F(GetConsoleMode)(hConsoleHandle,lpMode);

	return b;
}


BOOL WINAPI OnSetConsoleMode(HANDLE hConsoleHandle, DWORD dwMode)
{
	//typedef BOOL (WINAPI* OnSetConsoleMode_t)(HANDLE hConsoleHandle, DWORD dwMode);
	ORIGINAL_KRNL(SetConsoleMode);
	BOOL lbRc = FALSE;

	#if 0
	if (!(dwMode & ENABLE_PROCESSED_OUTPUT))
	{
		_ASSERTEX((dwMode & ENABLE_PROCESSED_OUTPUT)==ENABLE_PROCESSED_OUTPUT);
	}
	#endif

	if (gbIsVimProcess)
	{
		if ((dwMode & (ENABLE_WRAP_AT_EOL_OUTPUT|ENABLE_PROCESSED_OUTPUT)) != (ENABLE_WRAP_AT_EOL_OUTPUT|ENABLE_PROCESSED_OUTPUT))
		{
			if (CEAnsi::IsOutputHandle(hConsoleHandle))
			{
				dwMode |= ENABLE_WRAP_AT_EOL_OUTPUT|ENABLE_PROCESSED_OUTPUT;
			}
			else
			{
				dwMode |= ENABLE_WINDOW_INPUT;
			}
		}
	}

	lbRc = F(SetConsoleMode)(hConsoleHandle, dwMode);

	return lbRc;
}


// Sets the attributes of characters written to the console screen buffer by
// the WriteFile or WriteConsole function, or echoed by the ReadFile or ReadConsole function.
// This function affects text written after the function call.
BOOL WINAPI OnSetConsoleTextAttribute(HANDLE hConsoleOutput, WORD wAttributes)
{
	//typedef BOOL (WINAPI* OnSetConsoleTextAttribute_t)(HANDLE hConsoleOutput, WORD wAttributes);
	ORIGINAL_KRNL(SetConsoleTextAttribute);

	BOOL lbRc = FALSE;

	if (ph && ph->PreCallBack)
	{
		SETARGS2(&lbRc, hConsoleOutput, wAttributes);
		ph->PreCallBack(&args);
	}

	#ifdef _DEBUG
	// We do not care if visible ChildGui is doing smth in their console
	// Process here only our native console windows
	if ((ghAttachGuiClient == NULL) && !gbAttachGuiClient && (wAttributes != 7))
	{
		wchar_t szDbgInfo[128];
		msprintf(szDbgInfo, countof(szDbgInfo), L"PID=%u, SetConsoleTextAttribute=0x%02X(%u)\n", GetCurrentProcessId(), (int)wAttributes, (int)wAttributes);
		DebugString(szDbgInfo);
	}
	#endif

	lbRc = F(SetConsoleTextAttribute)(hConsoleOutput, wAttributes);

	if (ph && ph->PostCallBack)
	{
		SETARGS2(&lbRc, hConsoleOutput, wAttributes);
		ph->PostCallBack(&args);
	}

	return lbRc;
}


BOOL WINAPI OnWriteConsoleOutputA(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion)
{
	//typedef BOOL (WINAPI* OnWriteConsoleOutputA_t)(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
	ORIGINAL_KRNL(WriteConsoleOutputA);
	BOOL lbRc = FALSE;

	#ifdef _DEBUG
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwMode = 0;
	GetConsoleMode(hOut, &dwMode);
	GetConsoleMode(hConsoleOutput, &dwMode);
	#endif

	if (ph && ph->PreCallBack)
	{
		SETARGS5(&lbRc, hConsoleOutput, lpBuffer, &dwBufferSize, &dwBufferCoord, lpWriteRegion);
		ph->PreCallBack(&args);
	}

	lbRc = F(WriteConsoleOutputA)(hConsoleOutput, lpBuffer, dwBufferSize, dwBufferCoord, lpWriteRegion);

	if (ph && ph->PostCallBack)
	{
		SETARGS5(&lbRc, hConsoleOutput, lpBuffer, &dwBufferSize, &dwBufferCoord, lpWriteRegion);
		ph->PostCallBack(&args);
	}

	return lbRc;
}


BOOL WINAPI OnWriteConsoleOutputW(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion)
{
	//typedef BOOL (WINAPI* OnWriteConsoleOutputW_t)(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
	ORIGINAL_KRNL(WriteConsoleOutputW);
	BOOL lbRc = FALSE;

	if (ph && ph->PreCallBack)
	{
		SETARGS5(&lbRc, hConsoleOutput, lpBuffer, &dwBufferSize, &dwBufferCoord, lpWriteRegion);
		ph->PreCallBack(&args);
	}

	// PowerShell AI для определения прогресса в консоли
	if (gbPowerShellMonitorProgress)
	{
		// Первичные проверки "прогресс ли это"
		if ((dwBufferSize.Y >= 5) && !dwBufferCoord.X && !dwBufferCoord.Y
			&& lpWriteRegion && !lpWriteRegion->Left && (lpWriteRegion->Right == (dwBufferSize.X - 1))
			&& lpBuffer && (lpBuffer->Char.UnicodeChar == L' '))
		{
			#ifdef _DEBUG
			MY_CONSOLE_SCREEN_BUFFER_INFOEX csbi6 = {sizeof(csbi6)};
			apiGetConsoleScreenBufferInfoEx(hConsoleOutput, &csbi6);
			#endif
			// 120720 - PS игнорирует PopupColors в консоли. Вывод прогресса всегда идет 0x3E
			//&& (!gnConsolePopupColors || (lpBuffer->Attributes == gnConsolePopupColors)))
			if (lpBuffer->Attributes == 0x3E)
			{
				CheckPowerShellProgress(hConsoleOutput, lpBuffer, dwBufferSize, dwBufferCoord, lpWriteRegion);
			}
		}
	}

	lbRc = F(WriteConsoleOutputW)(hConsoleOutput, lpBuffer, dwBufferSize, dwBufferCoord, lpWriteRegion);

	if (ph && ph->PostCallBack)
	{
		SETARGS5(&lbRc, hConsoleOutput, lpBuffer, &dwBufferSize, &dwBufferCoord, lpWriteRegion);
		ph->PostCallBack(&args);
	}

	return lbRc;
}


BOOL WINAPI OnWriteConsoleA(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved)
{
	//typedef BOOL (WINAPI* OnWriteConsoleA_t)(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved);
	ORIGINAL_KRNL(WriteConsoleA);
	BOOL lbRc = FALSE;
	wchar_t* buf = NULL;
	DWORD len, cp;
	static bool badUnicode = false;
	DEBUGTEST(bool curBadUnicode = badUnicode);
	DEBUGTEST(wchar_t szTmp[2] = L"");

	FIRST_ANSI_CALL((const BYTE*)lpBuffer, nNumberOfCharsToWrite);

	if (badUnicode)
	{
		badUnicode = false;
		#ifdef _DEBUG
		if (lpBuffer && nNumberOfCharsToWrite)
		{
			szTmp[0] = ((char*)lpBuffer)[0];
			CEAnsi::DumpEscape(szTmp, 1, 2);
		}
		#endif
		goto badchar;
	}

	if (lpBuffer && nNumberOfCharsToWrite && hConsoleOutput && CEAnsi::IsAnsiCapable(hConsoleOutput))
	{
		cp = gCpConv.nDefaultCP ? gCpConv.nDefaultCP : GetConsoleOutputCP();

		DWORD nLastErr = 0;
		DWORD nFlags = 0; //MB_ERR_INVALID_CHARS;

		if (! ((cp == 42) || (cp == 65000) || (cp >= 57002 && cp <= 57011) || (cp >= 50220 && cp <= 50229)) )
		{
			nFlags = MB_ERR_INVALID_CHARS;
			nLastErr = GetLastError();
		}

		len = MultiByteToWideChar(cp, nFlags, (LPCSTR)lpBuffer, nNumberOfCharsToWrite, 0, 0);

		if (nFlags /*gpStartEnv->bIsDbcs*/ && (GetLastError() == ERROR_NO_UNICODE_TRANSLATION))
		{
			badUnicode = true;
			#ifdef _DEBUG
			szTmp[0] = ((char*)lpBuffer)[0];
			CEAnsi::DumpEscape(szTmp, 1, 2);
			#endif
			SetLastError(nLastErr);
			goto badchar;
		}

		buf = (wchar_t*)malloc((len+1)*sizeof(wchar_t));
		if (buf == NULL)
		{
			if (lpNumberOfCharsWritten != NULL)
				*lpNumberOfCharsWritten = 0;
		}
		else
		{
			DWORD newLen = MultiByteToWideChar(cp, nFlags, (LPCSTR)lpBuffer, nNumberOfCharsToWrite, buf, len);
			_ASSERTEX(newLen==len);
			buf[newLen] = 0; // ASCII-Z, хотя, если функцию WriteConsoleW зовет приложение - "\0" может и не быть...

			HANDLE hWrite;
			MConHandle hConOut(L"CONOUT$");
			if (hConsoleOutput == CEAnsi::ghLastConOut)
				hWrite = hConOut;
			else
				hWrite = hConsoleOutput;

			DWORD nWideWritten = 0;
			lbRc = CEAnsi::OurWriteConsoleW(hWrite, buf, len, &nWideWritten, NULL);

			// Issue 1291:	Python fails to print string sequence with ASCII character followed by Chinese character.
			if (lpNumberOfCharsWritten)
			{
				*lpNumberOfCharsWritten = (nWideWritten == len) ? nNumberOfCharsToWrite : nWideWritten;
			}
		}
		goto fin;
	}

badchar:
	// По идее, сюда попадать не должны. Ошибка в параметрах?
	_ASSERTEX((lpBuffer && nNumberOfCharsToWrite && hConsoleOutput && CEAnsi::IsAnsiCapable(hConsoleOutput)) || (curBadUnicode||badUnicode));
	lbRc = F(WriteConsoleA)(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, lpReserved);

fin:
	SafeFree(buf);
	return lbRc;
}


BOOL WINAPI OnWriteConsoleW(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved)
{
	return CEAnsi::OurWriteConsoleW(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, lpReserved, false);
}


TODO("По хорошему, после WriteConsoleOutputAttributes тоже нужно делать efof_ResetExt");
// Но пока можно это проигнорировать, большинство (?) программ, использует ее в связке
// WriteConsoleOutputAttributes/WriteConsoleOutputCharacter


BOOL WINAPI OnWriteConsoleOutputCharacterA(HANDLE hConsoleOutput, LPCSTR lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten)
{
	//typedef BOOL (WINAPI* OnWriteConsoleOutputCharacterA_t)(HANDLE hConsoleOutput, LPCSTR lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten);
	ORIGINAL_KRNL(WriteConsoleOutputCharacterA);

	FIRST_ANSI_CALL((const BYTE*)lpCharacter, nLength);

	ExtFillOutputParm fll = {sizeof(fll),
		efof_Attribute|(CEAnsi::gDisplayParm.WasSet ? efof_Current : efof_ResetExt),
		hConsoleOutput, {}, 0, dwWriteCoord, nLength};
	ExtFillOutput(&fll);

	BOOL lbRc = F(WriteConsoleOutputCharacterA)(hConsoleOutput, lpCharacter, nLength, dwWriteCoord, lpNumberOfCharsWritten);

	return lbRc;
}


BOOL WINAPI OnWriteConsoleOutputCharacterW(HANDLE hConsoleOutput, LPCWSTR lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten)
{
	//typedef BOOL (WINAPI* OnWriteConsoleOutputCharacterW_t)(HANDLE hConsoleOutput, LPCWSTR lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten);
	ORIGINAL_KRNL(WriteConsoleOutputCharacterW);

	FIRST_ANSI_CALL((const BYTE*)lpCharacter, nLength);

	ExtFillOutputParm fll = {sizeof(fll),
		efof_Attribute|(CEAnsi::gDisplayParm.WasSet ? efof_Current : efof_ResetExt),
		hConsoleOutput, {}, 0, dwWriteCoord, nLength};
	ExtFillOutput(&fll);

	BOOL lbRc = F(WriteConsoleOutputCharacterW)(hConsoleOutput, lpCharacter, nLength, dwWriteCoord, lpNumberOfCharsWritten);

	return lbRc;
}


BOOL WINAPI OnScrollConsoleScreenBufferA(HANDLE hConsoleOutput, const SMALL_RECT *lpScrollRectangle, const SMALL_RECT *lpClipRectangle, COORD dwDestinationOrigin, const CHAR_INFO *lpFill)
{
	//typedef BOOL (WINAPI* OnScrollConsoleScreenBufferA_t)(HANDLE hConsoleOutput, const SMALL_RECT *lpScrollRectangle, const SMALL_RECT *lpClipRectangle, COORD dwDestinationOrigin, const CHAR_INFO *lpFill);
	ORIGINAL_KRNL(ScrollConsoleScreenBufferA);
	BOOL lbRc = FALSE;

	if (CEAnsi::IsOutputHandle(hConsoleOutput))
	{
		WARNING("Проверка аргументов! Скролл может быть частичным");
		ExtScrollScreenParm scrl = {sizeof(scrl), essf_ExtOnly, hConsoleOutput, dwDestinationOrigin.Y - lpScrollRectangle->Top};
		ExtScrollScreen(&scrl);
	}

	lbRc = F(ScrollConsoleScreenBufferA)(hConsoleOutput, lpScrollRectangle, lpClipRectangle, dwDestinationOrigin, lpFill);

	return lbRc;
}


BOOL WINAPI OnScrollConsoleScreenBufferW(HANDLE hConsoleOutput, const SMALL_RECT *lpScrollRectangle, const SMALL_RECT *lpClipRectangle, COORD dwDestinationOrigin, const CHAR_INFO *lpFill)
{
	typedef BOOL (WINAPI* OnScrollConsoleScreenBufferW_t)(HANDLE hConsoleOutput, const SMALL_RECT *lpScrollRectangle, const SMALL_RECT *lpClipRectangle, COORD dwDestinationOrigin, const CHAR_INFO *lpFill);
	ORIGINAL_KRNL(ScrollConsoleScreenBufferW);
	BOOL lbRc = FALSE;

	if (CEAnsi::IsOutputHandle(hConsoleOutput))
	{
		WARNING("Проверка аргументов! Скролл может быть частичным");
		ExtScrollScreenParm scrl = {sizeof(scrl), essf_ExtOnly, hConsoleOutput, dwDestinationOrigin.Y - lpScrollRectangle->Top};
		ExtScrollScreen(&scrl);
	}

	//Warning: This function called from "cmd.exe /c cls" whith arguments:
	//lpScrollRectangle - full scroll buffer
	//lpClipRectangle - NULL
	//dwDestinationOrigin = {0, -9999}

	lbRc = F(ScrollConsoleScreenBufferW)(hConsoleOutput, lpScrollRectangle, lpClipRectangle, dwDestinationOrigin, lpFill);

	return lbRc;
}
