
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

#define HIDE_USE_EXCEPTION_INFO

#ifdef _DEBUG
	#define DebugString(x) //OutputDebugString(x)
#else
	#define DebugString(x) //OutputDebugString(x)
#endif

#ifdef _DEBUG
	//#define TRAP_ON_MOUSE_0x0
	#undef TRAP_ON_MOUSE_0x0
#else
	#undef TRAP_ON_MOUSE_0x0
#endif


#include "../common/Common.h"
#include "../common/CmdLine.h"
#include "../common/MRect.h"

#include "Ansi.h"
#include "hkConsoleInput.h"
#include "MainThread.h"
#include "hlpConsole.h"

/* **************** */

BOOL gbWasSucceededInRead = FALSE;

/* **************** */

static bool USE_INTERNAL_QUEUE = true;

static CESERVER_CONSOLE_APP_MAPPING* gpReadConAppMap = NULL;
static DWORD gnReadConAppPID = 0;

#ifdef _DEBUG
// для отладки ecompl
static INPUT_RECORD gir_Written[16] = {};
static INPUT_RECORD gir_Real[16] = {};
static INPUT_RECORD gir_Virtual[16] = {};
#endif

/* **************** */

extern bool gbCurDirChanged;
extern struct HookModeFar gFarMode;
extern void CheckPowershellProgressPresence();

/* **************** */

// Helper function
void PreReadConsoleInput(HANDLE hConIn, DWORD nFlags/*enum CEReadConsoleInputFlags*/, CESERVER_CONSOLE_APP_MAPPING** ppAppMap = NULL)
{
	#if defined(_DEBUG) && defined(PRE_PEEK_CONSOLE_INPUT)
	INPUT_RECORD ir = {}; DWORD nRead = 0, nBuffer = 0;
	BOOL bNumGot = GetNumberOfConsoleInputEvents(hConIn, &nBuffer);
	BOOL bConInPeek = nBuffer ? PeekConsoleInputW(hConIn, &ir, 1, &nRead) : FALSE;
	#endif

	if (gbPowerShellMonitorProgress)
	{
		CheckPowershellProgressPresence();
	}

	if (gbCurDirChanged)
	{
		gbCurDirChanged = false;

		if (ghConEmuWndDC)
		{
			if (gFarMode.cbSize
				&& gFarMode.OnCurDirChanged
				&& !IsBadCodePtr((FARPROC)gFarMode.OnCurDirChanged))
			{
				gFarMode.OnCurDirChanged();
			}
			else
			{
				CEStr szDir;
				if (GetDirectory(szDir))
				{
					// Sends CECMD_STORECURDIR into RConServer
					SendCurrentDirectory(ghConWnd, szDir);
				}
			}
		}
	}

	if (!(nFlags & rcif_Peek))
	{
		// On the one hand - there is a problem with unexpected Enter/Space keypress
		// github#19: After executing php.exe from command prompt (it runs by Enter KeyDown)
		//            the app gets in its input queue unexpected Enter KeyUp
		// On the other hand - application must be able to understand if the key was released
		// Powershell's 'get-help Get-ChildItem -full | out-host -paging' or Issue 1927 (jilrun.exe)
		CESERVER_CONSOLE_APP_MAPPING* pAppMap = UpdateAppMapFlags(nFlags);
		if (pAppMap && ppAppMap) *ppAppMap = pAppMap;
	}
}

// Helper function
void PostReadConsoleInput(HANDLE hConIn, DWORD nFlags/*enum CEReadConsoleInputFlags*/, CESERVER_CONSOLE_APP_MAPPING* pAppMap = NULL)
{
	if ((nFlags & rcif_LLInput) && !(nFlags & rcif_Peek))
	{
		if (pAppMap)
		{
			InterlockedCompareZero(&pAppMap->nReadConsoleInputPID, GetCurrentProcessId());
		}
	}
}

// Helper function: support Tab-completion in cmd.exe and others
COORD FixupReadStartCursorPos(DWORD nNumberOfCharsToRead, CONSOLE_SCREEN_BUFFER_INFO& csbi, const MY_CONSOLE_READCONSOLE_CONTROL* pInputControl)
{
	COORD crStartCursorPos = csbi.dwCursorPosition;

	if (pInputControl && (pInputControl->nLength >= sizeof(*pInputControl)))
	{
		if (pInputControl->nInitialChars < nNumberOfCharsToRead)
		{
			// Part of input is already "printed" to the buffer, take it into account
			int nNewX = (int)crStartCursorPos.X - pInputControl->nInitialChars;
			int nNewY = (int)crStartCursorPos.Y;
			if (nNewX < 0)
			{
				int nRows = klMin(nNewY, (int)((-nNewX) / csbi.dwSize.X));
				if ((nRows < nNewY) && ((nNewX + (nRows * csbi.dwSize.X)) < 0))
					nRows++;
				_ASSERTE(((nNewY - nRows) >= 0) && ((nNewX + (nRows * csbi.dwSize.X)) >= 0));
				nNewX += (nRows * csbi.dwSize.X);
				nNewY -= nRows;
			}
			if (nNewX >= 0 && nNewY >= 0)
			{
				crStartCursorPos = MakeCoord(nNewX, nNewY);
				csbi.dwCursorPosition = crStartCursorPos;
			}
			else
			{
				_ASSERTE(nNewX >= 0 && nNewY >= 0);
			}
		}
	}

	return crStartCursorPos;
}

// Helper function
void OnReadConsoleStart(bool bUnicode, HANDLE hConsoleInput, LPVOID lpBuffer, DWORD nNumberOfCharsToRead, LPDWORD lpNumberOfCharsRead, MY_CONSOLE_READCONSOLE_CONTROL* pInputControl)
{
	if (gReadConsoleInfo.InReadConsoleTID)
		gReadConsoleInfo.LastReadConsoleTID = gReadConsoleInfo.InReadConsoleTID;

#ifdef _DEBUG
	wchar_t szCurDir[MAX_PATH+1];
	GetCurrentDirectory(countof(szCurDir), szCurDir);
#endif

	// To minimize startup duration and possible problems
	// hook server will start on first 'user interaction'
	CheckHookServer();

	bool bCatch = false;
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	HANDLE hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD nConIn = 0, nConOut = 0;
	if (GetConsoleScreenBufferInfo(hConOut, &csbi))
	{
		if (GetConsoleMode(hConsoleInput, &nConIn) && GetConsoleMode(hConOut, &nConOut))
		{
			if ((nConIn & ENABLE_ECHO_INPUT) && (nConIn & ENABLE_LINE_INPUT))
			{
				bCatch = true;
				gReadConsoleInfo.InReadConsoleTID = GetCurrentThreadId();
				gReadConsoleInfo.bIsUnicode = bUnicode;
				gReadConsoleInfo.hConsoleInput = hConsoleInput;
				// Support Tab-completion in cmd.exe and others
				gReadConsoleInfo.crStartCursorPos = FixupReadStartCursorPos(nNumberOfCharsToRead, csbi, pInputControl);
				gReadConsoleInfo.nConInMode = nConIn;
				gReadConsoleInfo.nConOutMode = nConOut;

				CEAnsi::OnReadConsoleBefore(hConOut, csbi);
			}
		}
	}

	if (!bCatch)
	{
		gReadConsoleInfo.InReadConsoleTID = 0;
	}

	PreReadConsoleInput(hConsoleInput, (bUnicode ? rcif_Unicode : rcif_Ansi));
}

// Helper function
void OnReadConsoleEnd(BOOL bSucceeded, bool bUnicode, HANDLE hConsoleInput, LPVOID lpBuffer, DWORD nNumberOfCharsToRead, LPDWORD lpNumberOfCharsRead, MY_CONSOLE_READCONSOLE_CONTROL* pInputControl)
{
	if (bSucceeded && !gbWasSucceededInRead && lpNumberOfCharsRead && *lpNumberOfCharsRead)
		gbWasSucceededInRead = TRUE;

	if (gReadConsoleInfo.InReadConsoleTID)
	{
		gReadConsoleInfo.LastReadConsoleTID = gReadConsoleInfo.InReadConsoleTID;
		gReadConsoleInfo.InReadConsoleTID = 0;

		TODO("Отослать в ConEmu считанную строку?");
	}

	bool bNoLineFeed = true;
	if (bSucceeded)
	{
		// Empty line was "readed" (Ctrl+C)?
		// Or 'Tab'-eneded when Tab was pressed (for completion)?
		if (lpNumberOfCharsRead && lpBuffer)
		{
			if (!*lpNumberOfCharsRead)
				bNoLineFeed = true; // empty line
			else if (bUnicode && (((wchar_t*)lpBuffer)[*lpNumberOfCharsRead] == L'\t'))
				bNoLineFeed = true; // completion was requested
			else if (!bUnicode && (((char*)lpBuffer)[*lpNumberOfCharsRead] == '\t'))
				bNoLineFeed = true; // completion was requested

			// ANSI logging
			if (bUnicode)
				CEAnsi::WriteAnsiLogW((wchar_t*)lpBuffer, *lpNumberOfCharsRead);
			else
				CEAnsi::WriteAnsiLogA((char*)lpBuffer, *lpNumberOfCharsRead);
		}
	}

	CEAnsi::OnReadConsoleAfter(true, bNoLineFeed);

	// Сброс кешированных значений
	GetConsoleScreenBufferInfoCached(NULL, NULL);

	PostReadConsoleInput(hConsoleInput, (bUnicode ? rcif_Unicode : rcif_Ansi));
}

// Helper function: Notification ‘Debug’ setting page in ConEmu
void OnPeekReadConsoleInput(char acPeekRead/*'P'/'R'*/, char acUnicode/*'A'/'W'*/, HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nRead)
{
#ifdef TRAP_ON_MOUSE_0x0
	if (lpBuffer && nRead)
	{
		for (UINT i = 0; i < nRead; i++)
		{
			if (lpBuffer[i].EventType == MOUSE_EVENT)
			{
				if (lpBuffer[i].Event.MouseEvent.dwMousePosition.X == 0 && lpBuffer[i].Event.MouseEvent.dwMousePosition.Y == 0)
				{
					_ASSERTE(!(lpBuffer[i].Event.MouseEvent.dwMousePosition.X == 0 && lpBuffer[i].Event.MouseEvent.dwMousePosition.Y == 0));
				}
				//if (lpBuffer[i].Event.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED && lpBuffer[i].Event.MouseEvent.dwMousePosition.X != 5)
				//{
				//	_ASSERTE(!(lpBuffer[i].Event.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED && lpBuffer[i].Event.MouseEvent.dwMousePosition.X != 5));
				//}
			}
		}
	}
#endif

	DWORD nCurrentTID = GetCurrentThreadId();

	gReadConsoleInfo.LastReadConsoleInputTID = nCurrentTID;
	gReadConsoleInfo.hConsoleInput2 = hConsoleInput;

	if (nRead)
	{
		if (!gbWasSucceededInRead)
			gbWasSucceededInRead = TRUE;

		// Сброс кешированных значений
		GetConsoleScreenBufferInfoCached(NULL, NULL);
	}

	if (!gFarMode.bFarHookMode || !gFarMode.bMonitorConsoleInput || !nRead || !lpBuffer)
		return;

	//// Пока - только Read. Peek игнорируем
	//if (acPeekRead != 'R')
	//	return;

	CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_PEEKREADINFO, sizeof(CESERVER_REQ_HDR) //-V119
		+sizeof(CESERVER_REQ_PEEKREADINFO)+(nRead-1)*sizeof(INPUT_RECORD));
	if (pIn)
	{
		pIn->PeekReadInfo.nCount = (WORD)nRead;
		pIn->PeekReadInfo.cPeekRead = acPeekRead;
		pIn->PeekReadInfo.cUnicode = acUnicode;
		pIn->PeekReadInfo.h = hConsoleInput;
		pIn->PeekReadInfo.nTID = nCurrentTID;
		pIn->PeekReadInfo.nPID = GetCurrentProcessId();
		pIn->PeekReadInfo.bMainThread = (pIn->PeekReadInfo.nTID == gnHookMainThreadId);
		memmove(pIn->PeekReadInfo.Buffer, lpBuffer, nRead*sizeof(INPUT_RECORD));

		CESERVER_REQ* pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
		if (pOut) ExecuteFreeResult(pOut);
		ExecuteFreeResult(pIn);
	}


}

#ifdef _DEBUG
// Helper function
void PreWriteConsoleInput(BOOL abUnicode, const INPUT_RECORD *lpBuffer, DWORD nLength)
{
#ifdef TRAP_ON_MOUSE_0x0
	if (lpBuffer && nLength)
	{
		for (UINT i = 0; i < nLength; i++)
		{
			if (lpBuffer[i].EventType == MOUSE_EVENT)
			{
				if (lpBuffer[i].Event.MouseEvent.dwMousePosition.X == 0 && lpBuffer[i].Event.MouseEvent.dwMousePosition.Y == 0)
				{
					_ASSERTE(!(lpBuffer[i].Event.MouseEvent.dwMousePosition.X == 0 && lpBuffer[i].Event.MouseEvent.dwMousePosition.Y == 0));
				}
				//if (lpBuffer[i].Event.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED && lpBuffer[i].Event.MouseEvent.dwMousePosition.X != 5)
				//{
				//	_ASSERTE(!(lpBuffer[i].Event.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED && lpBuffer[i].Event.MouseEvent.dwMousePosition.X != 5));
				//}
			}
		}
	}
#endif
}
#endif

bool isProcessCtrlZ()
{
	CESERVER_CONSOLE_MAPPING_HDR* pMap = GetConMap();
	if (!pMap)
		return false;
	if (!pMap->cbSize || !(pMap->Flags & CECF_ProcessCtrlZ))
		return false;
	return true;
}


/* **************** */

BOOL WINAPI OnReadConsoleA(HANDLE hConsoleInput, LPVOID lpBuffer, DWORD nNumberOfCharsToRead, LPDWORD lpNumberOfCharsRead, LPVOID pInputControl)
{
	// typedef BOOL (WINAPI* OnReadConsoleA_t)(HANDLE hConsoleInput, LPVOID lpBuffer, DWORD nNumberOfCharsToRead, LPDWORD lpNumberOfCharsRead, LPVOID pInputControl);
	SUPPRESSORIGINALSHOWCALL;
	ORIGINAL_KRNL(ReadConsoleA);
	BOOL lbRc = FALSE;

	_ASSERTE(pInputControl==NULL); // pInputControl is expected to be NULL in ANSI version
	OnReadConsoleStart(false, hConsoleInput, lpBuffer, nNumberOfCharsToRead, lpNumberOfCharsRead, (MY_CONSOLE_READCONSOLE_CONTROL*)pInputControl);

	lbRc = F(ReadConsoleA)(hConsoleInput, lpBuffer, nNumberOfCharsToRead, lpNumberOfCharsRead, pInputControl);
	DWORD nErr = GetLastError();

	// gh#465: For consistence with OnReadConsoleW
	if (lbRc && lpBuffer && lpNumberOfCharsRead && *lpNumberOfCharsRead
		// pInputControl has no effect on ANSI version
		&& (*lpNumberOfCharsRead <= 3))
	{
		// To avoid checking of the mapping, we check result first
		const char* pchTest = (const char*)lpBuffer;
		if ((pchTest[0] == 26 /* Ctrl-Z / ^Z / (char)('Z' - '@') */)
			&& (((*lpNumberOfCharsRead == 3) && (pchTest[1] == '\r') && (pchTest[2] == '\n')) // Expected variant
				|| ((*lpNumberOfCharsRead == 2) && (pchTest[1] == '\n')) // Possible variant?
				))
		{
			if (isProcessCtrlZ())
				*lpNumberOfCharsRead = 0;
		}
	}

	OnReadConsoleEnd(lbRc, false, hConsoleInput, lpBuffer, nNumberOfCharsToRead, lpNumberOfCharsRead, (MY_CONSOLE_READCONSOLE_CONTROL*)pInputControl);
	SetLastError(nErr);

	return lbRc;
}


BOOL WINAPI OnReadConsoleW(HANDLE hConsoleInput, LPVOID lpBuffer, DWORD nNumberOfCharsToRead, LPDWORD lpNumberOfCharsRead, MY_CONSOLE_READCONSOLE_CONTROL* pInputControl)
{
	//typedef BOOL (WINAPI* OnReadConsoleW_t)(HANDLE hConsoleInput, LPVOID lpBuffer, DWORD nNumberOfCharsToRead, LPDWORD lpNumberOfCharsRead, MY_CONSOLE_READCONSOLE_CONTROL* pInputControl);
	SUPPRESSORIGINALSHOWCALL;
	ORIGINAL_KRNL(ReadConsoleW);
	BOOL lbRc = FALSE;
	DWORD nErr = GetLastError();
	DWORD nStartTick = GetTickCount(), nEndTick = 0;

	OnReadConsoleStart(true, hConsoleInput, lpBuffer, nNumberOfCharsToRead, lpNumberOfCharsRead, pInputControl);

	lbRc = F(ReadConsoleW)(hConsoleInput, lpBuffer, nNumberOfCharsToRead, lpNumberOfCharsRead, pInputControl);
	nErr = GetLastError();
	// Debug purposes
	nEndTick = GetTickCount();

	// gh#465: Terminate Go input with Ctrl-Z
	if (lbRc && lpBuffer && lpNumberOfCharsRead && *lpNumberOfCharsRead
		&& (!pInputControl) // Probably, if application pass this structure it really knows what it's doing
		&& (*lpNumberOfCharsRead <= 3))
	{
		// To avoid checking of the mapping, we check result first
		const wchar_t* pchTest = (const wchar_t*)lpBuffer;
		if ((pchTest[0] == 26 /* Ctrl-Z / ^Z / (char)('Z' - '@') */)
			&& (((*lpNumberOfCharsRead == 3) && (pchTest[1] == '\r') && (pchTest[2] == '\n')) // Expected variant
				|| ((*lpNumberOfCharsRead == 2) && (pchTest[1] == '\n')) // Possible variant?
				))
		{
			if (isProcessCtrlZ())
				*lpNumberOfCharsRead = 0;
		}
	}

	OnReadConsoleEnd(lbRc, true, hConsoleInput, lpBuffer, nNumberOfCharsToRead, lpNumberOfCharsRead, pInputControl);

	// cd \\server\share\dir\...
	if (gbAllowUncPaths && lpBuffer && lpNumberOfCharsRead && *lpNumberOfCharsRead
		&& (nNumberOfCharsToRead >= *lpNumberOfCharsRead) && (nNumberOfCharsToRead > 6))
	{
		wchar_t* pszCmd = (wchar_t*)lpBuffer;
		// "cd " ?
		if ((pszCmd[0]==L'c' || pszCmd[0]==L'C') && (pszCmd[1]==L'd' || pszCmd[1]==L'D') && pszCmd[2] == L' ')
		{
			pszCmd[*lpNumberOfCharsRead] = 0;
			wchar_t* pszPath = (wchar_t*)SkipNonPrintable(pszCmd+3);
			// Don't worry about local paths, check only network
			if (pszPath[0] == L'\\' && pszPath[1] == L'\\')
			{
				wchar_t* pszEnd;
				if (*pszPath == L'"')
					pszEnd = wcschr(++pszPath, L'"');
				else
					pszEnd = wcspbrk(pszPath, L"\r\n\t&| ");
				if (!pszEnd)
					pszEnd = pszPath + lstrlen(pszPath);
				if ((pszEnd - pszPath) < MAX_PATH)
				{
					wchar_t ch = *pszEnd; *pszEnd = 0;
					BOOL bSet = SetCurrentDirectory(pszPath);
					if (ch) *pszEnd = ch;
					if (bSet)
					{
						if (*pszEnd == L'"') pszEnd++;
						pszEnd = (wchar_t*)SkipNonPrintable(pszEnd);
						if (*pszEnd && wcschr(L"&|", *pszEnd))
						{
							while (*pszEnd && wcschr(L"&|", *pszEnd)) pszEnd++;
							pszEnd = (wchar_t*)SkipNonPrintable(pszEnd);
						}
						// Return anything...
						if (*pszEnd)
						{
							int nLeft = lstrlen(pszEnd);
							memmove(lpBuffer, pszEnd, (nLeft+1)*sizeof(*pszEnd));
						}
						else
						{
							lstrcpyn((wchar_t*)lpBuffer, L"cd\r\n", nNumberOfCharsToRead);

						}
						*lpNumberOfCharsRead = lstrlen((wchar_t*)lpBuffer);
					}
				}
			}
		}
	}

	SetLastError(nErr);

	UNREFERENCED_PARAMETER(nStartTick);
	UNREFERENCED_PARAMETER(nEndTick);
	return lbRc;
}


BOOL WINAPI OnGetNumberOfConsoleInputEvents(HANDLE hConsoleInput, LPDWORD lpcNumberOfEvents)
{
	//typedef BOOL (WINAPI* OnGetNumberOfConsoleInputEvents_t)(HANDLE hConsoleInput, LPDWORD lpcNumberOfEvents);
	SUPPRESSORIGINALSHOWCALL;
	ORIGINAL_KRNL(GetNumberOfConsoleInputEvents);
	BOOL lbRc = FALSE;

	//if (gpFarInfo && bMainThread)
	//	TouchReadPeekConsoleInputs();

	if (ph && ph->PreCallBack)
	{
		SETARGS2(&lbRc,hConsoleInput,lpcNumberOfEvents);

		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	lbRc = F(GetNumberOfConsoleInputEvents)(hConsoleInput, lpcNumberOfEvents);

	// Hiew fails pasting from clipboard on high speed
	if (gbIsHiewProcess && lpcNumberOfEvents && (*lpcNumberOfEvents > 1))
	{
		*lpcNumberOfEvents = 1;
	}

	if (ph && ph->PostCallBack)
	{
		SETARGS2(&lbRc,hConsoleInput,lpcNumberOfEvents);
		ph->PostCallBack(&args);
	}

	return lbRc;
}


BOOL WINAPI OnFlushConsoleInputBuffer(HANDLE hConsoleInput)
{
	//typedef BOOL (WINAPI* OnFlushConsoleInputBuffer_t)(HANDLE hConsoleInput);
	SUPPRESSORIGINALSHOWCALL;
	ORIGINAL_KRNL(FlushConsoleInputBuffer);
	BOOL lbRc = FALSE;

	//_ASSERTEX(FALSE && "calling FlushConsoleInputBuffer");
	DebugString(L"### calling FlushConsoleInputBuffer\n");

	lbRc = F(FlushConsoleInputBuffer)(hConsoleInput);

	return lbRc;
}


BOOL WINAPI OnPeekConsoleInputA(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
	//typedef BOOL (WINAPI* OnPeekConsoleInputA_t)(HANDLE,PINPUT_RECORD,DWORD,LPDWORD);
	SUPPRESSORIGINALSHOWCALL;
	ORIGINAL_KRNL(PeekConsoleInputA);
	//if (gpFarInfo && bMainThread)
	//	TouchReadPeekConsoleInputs(1);
	BOOL lbRc = FALSE;

	if (ph && ph->PreCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);

		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	PreReadConsoleInput(hConsoleInput, rcif_Ansi|rcif_Peek|rcif_LLInput);

	//#ifdef USE_INPUT_SEMAPHORE
	//DWORD nSemaphore = ghConInSemaphore ? WaitForSingleObject(ghConInSemaphore, INSEMTIMEOUT_READ) : 1;
	//_ASSERTE(nSemaphore<=1);
	//#endif

#if 0
	DWORD nMode = 0; GetConsoleMode(hConsoleInput, &nMode);
#endif

	lbRc = F(PeekConsoleInputA)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);

	PostReadConsoleInput(hConsoleInput, rcif_Ansi|rcif_Peek|rcif_LLInput);

	//#ifdef USE_INPUT_SEMAPHORE
	//if ((nSemaphore == WAIT_OBJECT_0) && ghConInSemaphore) ReleaseSemaphore(ghConInSemaphore, 1, NULL);
	//#endif

	if (ph && ph->PostCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);
		ph->PostCallBack(&args);
	}

	if (lbRc && lpNumberOfEventsRead && *lpNumberOfEventsRead && lpBuffer)
		OnPeekReadConsoleInput('P', 'A', hConsoleInput, lpBuffer, *lpNumberOfEventsRead);

	return lbRc;
}


BOOL WINAPI OnPeekConsoleInputW(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
	//typedef BOOL (WINAPI* OnPeekConsoleInputW_t)(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
	SUPPRESSORIGINALSHOWCALL;
	ORIGINAL_KRNL(PeekConsoleInputW);
	//if (gpFarInfo && bMainThread)
	//	TouchReadPeekConsoleInputs(1);
	BOOL lbRc = FALSE;

	if (ph && ph->PreCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);

		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	PreReadConsoleInput(hConsoleInput, rcif_Unicode|rcif_Peek|rcif_LLInput);

	//#ifdef USE_INPUT_SEMAPHORE
	//DWORD nSemaphore = ghConInSemaphore ? WaitForSingleObject(ghConInSemaphore, INSEMTIMEOUT_READ) : 1;
	//_ASSERTE(nSemaphore<=1);
	//#endif

	if (gFarMode.bFarHookMode && USE_INTERNAL_QUEUE) // ecompl speed-up
	{
		#ifdef _DEBUG
		DWORD nDbgReadReal = countof(gir_Real), nDbgReadVirtual = countof(gir_Virtual);
		BOOL bReadReal = F(PeekConsoleInputW)(hConsoleInput, gir_Real, nDbgReadReal, &nDbgReadReal);
		BOOL bReadVirt = gInQueue.ReadInputQueue(gir_Virtual, &nDbgReadVirtual, TRUE);
		#endif

		if ((!lbRc || !(lpNumberOfEventsRead && *lpNumberOfEventsRead)) && !gInQueue.IsInputQueueEmpty())
		{
			DWORD n = nLength;
			lbRc = gInQueue.ReadInputQueue(lpBuffer, &n, TRUE);
			if (lpNumberOfEventsRead)
				*lpNumberOfEventsRead = lbRc ? n : 0;
		}
		else
		{
			lbRc = F(PeekConsoleInputW)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);
		}
	}
	else
	{
		lbRc = F(PeekConsoleInputW)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);
	}

	PostReadConsoleInput(hConsoleInput, rcif_Unicode|rcif_Peek|rcif_LLInput);

	//#ifdef USE_INPUT_SEMAPHORE
	//if ((nSemaphore == WAIT_OBJECT_0) && ghConInSemaphore) ReleaseSemaphore(ghConInSemaphore, 1, NULL);
	//#endif

	if (ph && ph->PostCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);
		ph->PostCallBack(&args);
	}

	if (lbRc && lpNumberOfEventsRead && *lpNumberOfEventsRead && lpBuffer)
	{
		OnPeekReadConsoleInput('P', 'W', hConsoleInput, lpBuffer, *lpNumberOfEventsRead);
	}

	//#ifdef _DEBUG
	//	wchar_t szDbg[128];
	//	if (lbRc && lpNumberOfEventsRead && *lpNumberOfEventsRead && lpBuffer && lpBuffer->EventType == MOUSE_EVENT)
	//		msprintf(szDbg, L"ConEmuHk.OnPeekConsoleInputW(x%04X,x%04X) %u\n",
	//			lpBuffer->Event.MouseEvent.dwButtonState, lpBuffer->Event.MouseEvent.dwControlKeyState, *lpNumberOfEventsRead);
	//	else
	//		lstrcpyW(szDbg, L"ConEmuHk.OnPeekConsoleInputW(Non mouse event)\n");
	//	DebugStringW(szDbg);
	//#endif

	return lbRc;
}


BOOL WINAPI OnReadConsoleInputA(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
	//typedef BOOL (WINAPI* OnReadConsoleInputA_t)(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
	SUPPRESSORIGINALSHOWCALL;
	ORIGINAL_KRNL(ReadConsoleInputA);
	//if (gpFarInfo && bMainThread)
	//	TouchReadPeekConsoleInputs(0);
	BOOL lbRc = FALSE;

	#if defined(_DEBUG)
	#if 1
	UINT nCp = GetConsoleCP();
	UINT nOutCp = GetConsoleOutputCP();
	UINT nOemCp = GetOEMCP();
	UINT nAnsiCp = GetACP();
	#endif
	#endif

	// To minimize startup duration and possible problems
	// hook server will start on first 'user interaction'
	CheckHookServer();

	if (ph && ph->PreCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);

		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	CESERVER_CONSOLE_APP_MAPPING* pAppMap = NULL;
	PreReadConsoleInput(hConsoleInput, rcif_Ansi|rcif_LLInput, &pAppMap);

	//#ifdef USE_INPUT_SEMAPHORE
	//DWORD nSemaphore = ghConInSemaphore ? WaitForSingleObject(ghConInSemaphore, INSEMTIMEOUT_READ) : 1;
	//_ASSERTE(nSemaphore<=1);
	//#endif

	lbRc = F(ReadConsoleInputA)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);

	// cygwin/msys shells prompt? seems like they use [W] version now, but JIC
	if (lbRc && CEAnsi::ghAnsiLogFile
		&& (nLength == 1) && (*lpNumberOfEventsRead == 1) && (lpBuffer->EventType == KEY_EVENT)
		&& lpBuffer->Event.KeyEvent.bKeyDown && (lpBuffer->Event.KeyEvent.wVirtualKeyCode == VK_RETURN)
		)
	{
		CEAnsi::AnsiLogEnterPressed();
	}

	PostReadConsoleInput(hConsoleInput, rcif_Ansi|rcif_LLInput, pAppMap);

	//#ifdef USE_INPUT_SEMAPHORE
	//if ((nSemaphore == WAIT_OBJECT_0) && ghConInSemaphore) ReleaseSemaphore(ghConInSemaphore, 1, NULL);
	//#endif

	if (ph && ph->PostCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);
		ph->PostCallBack(&args);
	}

	if (lbRc && lpNumberOfEventsRead && *lpNumberOfEventsRead && lpBuffer)
	{
		OnPeekReadConsoleInput('R', 'A', hConsoleInput, lpBuffer, *lpNumberOfEventsRead);
	}

	return lbRc;
}


BOOL WINAPI OnReadConsoleInputW(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
	//typedef BOOL (WINAPI* OnReadConsoleInputW_t)(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead);
	SUPPRESSORIGINALSHOWCALL;
	ORIGINAL_KRNL(ReadConsoleInputW);
	//if (gpFarInfo && bMainThread)
	//	TouchReadPeekConsoleInputs(0);
	BOOL lbRc = FALSE;

	// To minimize startup duration and possible problems
	// hook server will start on first 'user interaction'
	CheckHookServer();

	if (ph && ph->PreCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);

		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	CESERVER_CONSOLE_APP_MAPPING* pAppMap = NULL;
	PreReadConsoleInput(hConsoleInput, rcif_Unicode|rcif_LLInput, &pAppMap);

	//#ifdef USE_INPUT_SEMAPHORE
	//DWORD nSemaphore = ghConInSemaphore ? WaitForSingleObject(ghConInSemaphore, INSEMTIMEOUT_READ) : 1;
	//_ASSERTE(nSemaphore<=1);
	//#endif

	#if 0
	// get-help Get-ChildItem -full | out-host -paging
	HANDLE hInTest;
	HANDLE hTestHandle = NULL;
	DWORD nInMode, nArgMode;
	BOOL bInTest = FALSE, bArgTest = FALSE;
	if (gbPowerShellMonitorProgress)
	{
		hInTest = GetStdHandle(STD_INPUT_HANDLE);
		#ifdef _DEBUG
		bInTest = GetConsoleMode(hInTest, &nInMode);
		bArgTest = GetConsoleMode(hConsoleInput, &nArgMode);
		hTestHandle = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (hTestHandle != INVALID_HANDLE_VALUE)
			CloseHandle(hTestHandle);
		#endif
	}
	#endif

	if (gFarMode.bFarHookMode && USE_INTERNAL_QUEUE) // ecompl speed-up
	{
		#ifdef _DEBUG
		DWORD nDbgReadReal = countof(gir_Real), nDbgReadVirtual = countof(gir_Virtual);
		BOOL bReadReal = PeekConsoleInputW(hConsoleInput, gir_Real, nDbgReadReal, &nDbgReadReal);
		BOOL bReadVirt = gInQueue.ReadInputQueue(gir_Virtual, &nDbgReadVirtual, TRUE);
		#endif

		if ((!lbRc || !(lpNumberOfEventsRead && *lpNumberOfEventsRead)) && !gInQueue.IsInputQueueEmpty())
		{
			DWORD n = nLength;
			lbRc = gInQueue.ReadInputQueue(lpBuffer, &n, FALSE);
			if (lpNumberOfEventsRead)
				*lpNumberOfEventsRead = lbRc ? n : 0;
		}
		else
		{
			lbRc = F(ReadConsoleInputW)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);
		}
	}
	else
	{
		lbRc = F(ReadConsoleInputW)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);
	}

	// cygwin/msys shells prompt
	if (lbRc && CEAnsi::ghAnsiLogFile
		&& (nLength == 1) && (*lpNumberOfEventsRead == 1) && (lpBuffer->EventType == KEY_EVENT)
		&& lpBuffer->Event.KeyEvent.bKeyDown && (lpBuffer->Event.KeyEvent.wVirtualKeyCode == VK_RETURN)
		)
	{
		CEAnsi::AnsiLogEnterPressed();
	}

	PostReadConsoleInput(hConsoleInput, rcif_Unicode|rcif_LLInput, pAppMap);

	//#ifdef USE_INPUT_SEMAPHORE
	//if ((nSemaphore == WAIT_OBJECT_0) && ghConInSemaphore) ReleaseSemaphore(ghConInSemaphore, 1, NULL);
	//#endif

	if (ph && ph->PostCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);
		ph->PostCallBack(&args);
	}

	if (lbRc && lpNumberOfEventsRead && *lpNumberOfEventsRead && lpBuffer)
	{
		OnPeekReadConsoleInput('R', 'W', hConsoleInput, lpBuffer, *lpNumberOfEventsRead);
	}

	return lbRc;
}


BOOL WINAPI OnWriteConsoleInputA(HANDLE hConsoleInput, const INPUT_RECORD *lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsWritten)
{
	//typedef BOOL (WINAPI* OnWriteConsoleInputA_t)(HANDLE hConsoleInput, const INPUT_RECORD *lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsWritten);
	ORIGINAL_KRNL(WriteConsoleInputA);
	BOOL lbRc = FALSE;

	#ifdef _DEBUG
	PreWriteConsoleInput(FALSE, lpBuffer, nLength);
	#endif

	if (ph && ph->PreCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsWritten);

		// Если функция возвращает FALSE - реальная запись не будет вызвана
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	lbRc = F(WriteConsoleInputA)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsWritten);

	if (ph && ph->PostCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsWritten);
		ph->PostCallBack(&args);
	}

	return lbRc;
}


BOOL WINAPI OnWriteConsoleInputW(HANDLE hConsoleInput, const INPUT_RECORD *lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsWritten)
{
	//typedef BOOL (WINAPI* OnWriteConsoleInputW_t)(HANDLE hConsoleInput, const INPUT_RECORD *lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsWritten);
	ORIGINAL_KRNL(WriteConsoleInputW);
	BOOL lbRc = FALSE;

	#ifdef _DEBUG
	PreWriteConsoleInput(FALSE, lpBuffer, nLength);
	#endif

	if (ph && ph->PreCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsWritten);

		// Если функция возвращает FALSE - реальная запись не будет вызвана
		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	// ecompl speed-up
	if (gFarMode.bFarHookMode && USE_INTERNAL_QUEUE
		&& nLength && lpBuffer && lpBuffer->EventType == KEY_EVENT && lpBuffer->Event.KeyEvent.uChar.UnicodeChar)
	{
		#ifdef _DEBUG
		memset(gir_Written, 0, sizeof(gir_Written));
		memmove(gir_Written, lpBuffer, min(countof(gir_Written),nLength)*sizeof(*lpBuffer));
		#endif
		lbRc = gInQueue.WriteInputQueue(lpBuffer, FALSE, nLength);
	}
	else
	{
		lbRc = F(WriteConsoleInputW)(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsWritten);
	}

	if (ph && ph->PostCallBack)
	{
		SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsWritten);
		ph->PostCallBack(&args);
	}

	return lbRc;
}
