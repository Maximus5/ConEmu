
/*
Copyright (c) 2015-present Maximus5
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
	#define DebugStringConSize(x) //OutputDebugString(x)
	#define DefTermMsg(s) //MessageBox(NULL, s, L"ConEmuHk", MB_SYSTEMMODAL)
	#define BsDelWordMsg(s) //MessageBox(NULL, s, L"OnPromptBsDeleteWord called", MB_SYSTEMMODAL);
#else
	#define DebugString(x) //OutputDebugString(x)
	#define DebugStringConSize(x) //OutputDebugString(x)
	#define DefTermMsg(s) //MessageBox(NULL, s, L"ConEmuHk", MB_SYSTEMMODAL)
	#define BsDelWordMsg(s) //MessageBox(NULL, s, L"OnPromptBsDeleteWord called", MB_SYSTEMMODAL);
#endif

#include "../common/Common.h"
#include "../common/ConsoleRead.h"
#include "../common/MRect.h"
#include "../common/UnicodeChars.h"
#include "../common/WConsole.h"

#include "Ansi.h"
#include "hlpConsole.h"
#include "hlpProcess.h"

/* **************** */

BOOL MyGetConsoleFontSize(COORD& crFontSize)
{
	BOOL lbRc = FALSE;

	if (ghConEmuWnd)
	{
		_ASSERTE(gnGuiPID!=0);
		ConEmuGuiMapping* inf = (ConEmuGuiMapping*)malloc(sizeof(ConEmuGuiMapping));
		if (inf)
		{
			if (LoadGuiMapping(gnGuiPID, *inf))
			{
				crFontSize.Y = (SHORT)inf->MainFont.nFontHeight;
				crFontSize.X = (SHORT)inf->MainFont.nFontCellWidth;
				lbRc = TRUE;
			}
			free(inf);
		}

		_ASSERTEX(lbRc && (crFontSize.X > 3) && (crFontSize.Y > 4));
	}

	return lbRc;
}

bool IsConsoleActive()
{
	bool bActive = true;

	if (ghConEmuWnd)
	{
		_ASSERTE(gnGuiPID!=0);
		ConEmuGuiMapping* inf = (ConEmuGuiMapping*)malloc(sizeof(ConEmuGuiMapping));
		if (inf)
		{
			if (LoadGuiMapping(gnGuiPID, *inf))
			{
				for (size_t i = 0; i < countof(inf->Consoles); ++i)
				{
					if ((HWND)inf->Consoles[i].Console == ghConWnd)
					{
						if (!(inf->Consoles[i].Flags & ccf_Active))
							bActive = false;
						break;
					}
				}
			}
			free(inf);
		}
	}

	return bActive;
}

BOOL IsVisibleRectLocked(COORD& crLocked)
{
	CESERVER_CONSOLE_MAPPING_HDR SrvMap;
	if (LoadSrvMapping(ghConWnd, SrvMap))
	{
		if (SrvMap.bLockVisibleArea && (SrvMap.crLockedVisible.X > 0) && (SrvMap.crLockedVisible.Y > 0))
		{
			crLocked = SrvMap.crLockedVisible;
			return TRUE;
		}
	}
	return FALSE;
}


// Due to Microsoft bug we need to lock Server reading thread to avoid crash of kernel
// http://conemu.github.io/en/MicrosoftBugs.html#Exception_in_ReadConsoleOutput
void LockServerReadingThread(bool bLock, COORD dwSize, CESERVER_REQ*& pIn, CESERVER_REQ*& pOut)
{
	DWORD nServerPID = gnServerPID;
	if (!nServerPID)
		return;

	DWORD nErrSave = GetLastError();

	if (bLock)
	{
		HANDLE hOurThreadHandle = NULL;

		// We need to give our thread handle (to server process) to avoid
		// locking of server reading thread (in case of our thread fails)
		DWORD dwErr = -1;
		HANDLE hServer = OpenProcess(PROCESS_DUP_HANDLE, FALSE, nServerPID);
		if (!hServer)
		{
			dwErr = GetLastError();
			_ASSERTEX(hServer!=NULL && "Open server handle fails, Can't dup handle!");
		}
		else
		{
			if (!DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), hServer, &hOurThreadHandle,
					SYNCHRONIZE|THREAD_QUERY_INFORMATION, FALSE, 0))
			{
				dwErr = GetLastError();
				_ASSERTEX(hServer!=NULL && "DuplicateHandle fails, Can't dup handle!");
				hOurThreadHandle = NULL;
			}
			CloseHandle(hServer);
		}

		pIn = ExecuteNewCmd(CECMD_SETCONSCRBUF, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_SETCONSCRBUF));
		if (pIn)
		{
			pIn->SetConScrBuf.bLock = TRUE;
			pIn->SetConScrBuf.dwSize = dwSize; // Informational
			pIn->SetConScrBuf.hRequestor = hOurThreadHandle;
		}
	}
	else
	{
		if (pIn)
		{
			pIn->SetConScrBuf.bLock = FALSE;
		}
	}

	if (pIn)
	{
		ExecuteFreeResult(pOut);
		pOut = ExecuteSrvCmd(nServerPID, pIn, ghConWnd);
		if (pOut && pOut->DataSize() >= sizeof(CESERVER_REQ_SETCONSCRBUF))
		{
			pIn->SetConScrBuf.hTemp = pOut->SetConScrBuf.hTemp;
		}
	}

	if (!bLock)
	{
		ExecuteFreeResult(pIn);
		ExecuteFreeResult(pOut);
	}

	// Transparently to calling function...
	SetLastError(nErrSave);
}

BOOL GetConsoleScreenBufferInfoCached(HANDLE hConsoleOutput, PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo, BOOL bForced /*= FALSE*/)
{
	BOOL lbRc = FALSE;

	static DWORD s_LastCheckTick = 0;
	static CONSOLE_SCREEN_BUFFER_INFO s_csbi = {};
	static HANDLE s_hConOut = NULL;
	static DWORD s_last_attr = -1;
	//DWORD nTickDelta = 0;
	//const DWORD TickDeltaMax = 250;

	if (hConsoleOutput == NULL)
	{
		// Сброс
		s_hConOut = NULL;
		GetConsoleModeCached(NULL, NULL);
		return FALSE;
	}

	if (!lpConsoleScreenBufferInfo)
	{
		_ASSERTEX(lpConsoleScreenBufferInfo!=NULL);
		return FALSE;
	}

#if 0
	if (s_hConOut && (s_hConOut == hConsoleOutput))
	{
		nTickDelta = GetTickCount() - s_LastCheckTick;
		if (nTickDelta <= TickDeltaMax)
		{
			if (bForced)
			{
				#ifdef _DEBUG
				lbRc = FALSE;
				#endif
			}
			else
			{
				*lpConsoleScreenBufferInfo = s_csbi;
				lbRc = TRUE;
			}
		}
	}
#endif

	if (!lbRc)
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi = {};
		lbRc = GetConsoleScreenBufferInfo(hConsoleOutput, &csbi);
		DEBUGTEST(DWORD nBufErr = GetLastError());
		*lpConsoleScreenBufferInfo = csbi;
		if (lbRc)
		{
			if (s_last_attr != csbi.wAttributes)
			{
				s_last_attr = csbi.wAttributes;
				if (CONFORECOLOR(csbi.wAttributes) == CONBACKCOLOR(csbi.wAttributes))
				{
					CEAnsi::WriteAnsiLogFormat("ConsoleTextAttribute >> 0x%02X", csbi.wAttributes);
				}
			}

			s_csbi = csbi;
			s_LastCheckTick = GetTickCount();
			s_hConOut = hConsoleOutput;
			UpdateAppMapRows(std::max(csbi.dwCursorPosition.Y, csbi.srWindow.Bottom), false);
		}
	}

	return lbRc;
}

BOOL GetConsoleModeCached(HANDLE hConsoleHandle, LPDWORD lpMode, BOOL bForced /*= FALSE*/)
{
	BOOL lbRc = FALSE;

	static DWORD s_LastCheckTick = 0;
	static DWORD s_dwMode = 0;
	static HANDLE s_hConHandle = NULL;
	DWORD nTickDelta = 0;
	const DWORD TickDeltaMax = 250;

	if (hConsoleHandle == NULL)
	{
		// Сброс
		s_hConHandle = NULL;
		return FALSE;
	}

	if (!lpMode)
	{
		_ASSERTEX(lpMode!=NULL);
		return FALSE;
	}

	if (s_hConHandle && (s_hConHandle == hConsoleHandle))
	{
		nTickDelta = GetTickCount() - s_LastCheckTick;
		if (nTickDelta <= TickDeltaMax)
		{
			if (bForced)
			{
				#ifdef _DEBUG
				lbRc = FALSE;
				#endif
			}
			else
			{
				*lpMode = s_dwMode;
				lbRc = TRUE;
			}
		}
	}

	if (!lbRc)
	{
		DWORD dwMode = 0;
		lbRc = GetConsoleMode(hConsoleHandle, &dwMode);
		*lpMode = dwMode;
		if (lbRc)
		{
			s_dwMode = dwMode;
			s_LastCheckTick = GetTickCount();
			s_hConHandle = hConsoleHandle;
		}
	}

	return lbRc;
}


AttachConsole_t GetAttachConsoleProc()
{
	static HMODULE hKernel = NULL;
	static AttachConsole_t _AttachConsole = NULL;
	if (!hKernel)
	{
		hKernel = GetModuleHandle(L"kernel32.dll");
		if (hKernel)
		{
			_AttachConsole = (AttachConsole_t)GetProcAddress(hKernel, "AttachConsole");
		}
	}
	return _AttachConsole;
}

bool AttachServerConsole()
{
	bool lbAttachRc = false;
	DWORD nErrCode;
	HWND hCurCon = GetRealConsoleWindow();
	if (hCurCon == NULL && gnServerPID != 0)
	{
		// функция есть только в WinXP и выше
		AttachConsole_t _AttachConsole = GetAttachConsoleProc();
		if (_AttachConsole)
		{
			lbAttachRc = (_AttachConsole(gnServerPID) != 0);
			if (!lbAttachRc)
			{
				nErrCode = GetLastError();
				_ASSERTE(nErrCode==0 && lbAttachRc);
			}
		}
	}
	return lbAttachRc;
}

// Drop the progress flag, because the prompt may appear
void CheckPowershellProgressPresence()
{
	if (!gbPowerShellMonitorProgress || (gnPowerShellProgressValue == -1))
		return;

	// При возврате в Prompt - сброс прогресса
	gnPowerShellProgressValue = -1;
	GuiSetProgress(0,0);
}

// PowerShell AI для определения прогресса в консоли
void CheckPowerShellProgress(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion)
{
	_ASSERTE(dwBufferSize.Y >= 5);
	int nProgress = -1;
	WORD nNeedAttr = lpBuffer->Attributes;

	for (SHORT Y = dwBufferSize.Y - 2; Y > 0; Y--)
	{
		const CHAR_INFO* pLine = lpBuffer + dwBufferSize.X * Y;

		// 120720 - PS игнорирует PopupColors в консоли. Вывод прогресса всегда идет 0x3E
		if (nNeedAttr/*gnConsolePopupColors*/ != 0)
		{
			if ((pLine[4].Attributes != nNeedAttr/*gnConsolePopupColors*/)
				|| (pLine[dwBufferSize.X - 7].Attributes != nNeedAttr/*gnConsolePopupColors*/))
				break; // не оно
		}

		if ((pLine[4].Char.UnicodeChar == L'[') && (pLine[dwBufferSize.X - 7].Char.UnicodeChar == L']'))
		{
			// Считаем проценты
			SHORT nLen = dwBufferSize.X - 7 - 5;
			if (nLen > 0)
			{
				nProgress = 100;
				for (SHORT X = 5; X < (dwBufferSize.X - 8); X++)
				{
					if (pLine[X].Char.UnicodeChar == L' ')
					{
						nProgress = (X - 5) * 100 / nLen;
						break;
					}
				}
			}
			break;
		}
	}

	if (nProgress != gnPowerShellProgressValue)
	{
		gnPowerShellProgressValue = nProgress;
		GuiSetProgress((nProgress != -1) ? 1 : 0, nProgress);
	}
}

void FixHwnd4ConText(HWND& hWnd)
{
	if (ghConEmuWndDC && (hWnd == ghConEmuWndDC || hWnd == ghConEmuWnd))
	{
		hWnd = ghConWnd;
	}
}
