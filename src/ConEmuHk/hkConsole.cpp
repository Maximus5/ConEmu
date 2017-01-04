
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
	#define DebugStringConSize(x) //OutputDebugString(x)
	#define DefTermMsg(s) //MessageBox(NULL, s, L"ConEmuHk", MB_SYSTEMMODAL)
#else
	#define DebugString(x) //OutputDebugString(x)
	#define DebugStringConSize(x) //OutputDebugString(x)
	#define DefTermMsg(s) //MessageBox(NULL, s, L"ConEmuHk", MB_SYSTEMMODAL)
#endif

#include "../common/Common.h"
#include "../common/HandleKeeper.h"
#include "../common/MModule.h"
#include "../common/WConsole.h"
#include "../common/WErrGuard.h"

#include "Ansi.h"
#include "DefTermHk.h"
#include "GuiAttach.h"
#include "hkConsole.h"
#include "MainThread.h"

#define SETCONCP_READYTIMEOUT 5000
#define SETCONCP_TIMEOUT 1000

/* **************** */

HANDLE ghStdOutHandle = NULL;
HANDLE ghCurrentOutBuffer = NULL;

/* **************** */

extern HANDLE ghConsoleCursorChanged;
extern void LockServerReadingThread(bool bLock, COORD dwSize, CESERVER_REQ*& pIn, CESERVER_REQ*& pOut);
extern BOOL IsVisibleRectLocked(COORD& crLocked);
extern BOOL MyGetConsoleFontSize(COORD& crFontSize);

/* **************** */

BOOL WINAPI OnSetConsoleTitleA(LPCSTR lpConsoleTitle)
{
	//typedef BOOL (WINAPI* OnSetConsoleTitleA_t)(LPCSTR lpConsoleTitle);
	ORIGINAL_KRNL(SetConsoleTitleA);
	BOOL bRc = FALSE;
	if (F(SetConsoleTitleA))
		bRc = F(SetConsoleTitleA)(lpConsoleTitle);
	return bRc;
}


BOOL WINAPI OnSetConsoleTitleW(LPCWSTR lpConsoleTitle)
{
	//typedef BOOL (WINAPI* OnSetConsoleTitleW_t)(LPCWSTR lpConsoleTitle);
	ORIGINAL_KRNL(SetConsoleTitleW);

	#ifdef DEBUG_CON_TITLE
	if (!gpLastSetConTitle)
		gpLastSetConTitle = new CEStr(lstrdup(lpConsoleTitle));
	else
		gpLastSetConTitle->Set(lpConsoleTitle);
	CEStr lsDbg(lstrmerge(L"SetConsoleTitleW('", lpConsoleTitle, L"')\n"));
	OutputDebugString(lsDbg);
	#endif

	BOOL bRc = FALSE;
	if (F(SetConsoleTitleW))
		bRc = F(SetConsoleTitleW)(lpConsoleTitle);
	return bRc;
}


DWORD WINAPI OnGetConsoleAliasesW(LPWSTR AliasBuffer, DWORD AliasBufferLength, LPWSTR ExeName)
{
	//typedef DWORD (WINAPI* OnGetConsoleAliasesW_t)(LPWSTR AliasBuffer, DWORD AliasBufferLength, LPWSTR ExeName);
	ORIGINAL_KRNL(GetConsoleAliasesW);
	DWORD nError = 0;
	DWORD nRc = F(GetConsoleAliasesW)(AliasBuffer,AliasBufferLength,ExeName);

	if (!nRc)
	{
		nError = GetLastError();

		// финт ушами
		if (nError == ERROR_NOT_ENOUGH_MEMORY) // && gdwServerPID)
		{
			DWORD nServerPID = gnServerPID;
			HWND hConWnd = GetRealConsoleWindow();
			_ASSERTE(hConWnd == ghConWnd);

			//MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> ConInfo;
			//ConInfo.InitName(CECONMAPNAME, (DWORD)hConWnd); //-V205
			//CESERVER_CONSOLE_MAPPING_HDR *pInfo = ConInfo.Open();
			//if (pInfo
			//	&& (pInfo->cbSize >= sizeof(CESERVER_CONSOLE_MAPPING_HDR))
			//	//&& (pInfo->nProtocolVersion == CESERVER_REQ_VER)
			//	)
			//{
			//	nServerPID = pInfo->nServerPID;
			//	ConInfo.CloseMap();
			//}

			if (nServerPID)
			{
				CESERVER_REQ_HDR In;
				ExecutePrepareCmd(&In, CECMD_GETALIASES, sizeof(CESERVER_REQ_HDR));
				CESERVER_REQ* pOut = ExecuteSrvCmd(nServerPID/*gdwServerPID*/, (CESERVER_REQ*)&In, hConWnd);

				if (pOut)
				{
					size_t nData = min(AliasBufferLength,(pOut->hdr.cbSize-sizeof(pOut->hdr)));

					if (nData)
					{
						memmove(AliasBuffer, pOut->Data, nData);
						nRc = TRUE;
					}

					ExecuteFreeResult(pOut);
				}
			}
		}

		if (!nRc)
			SetLastError(nError); // вернуть, вдруг какая функция его поменяла
	}

	return nRc;
}


// https://conemu.github.io/en/MicrosoftBugs.html#chcp_hung
// https://github.com/Maximus5/conemu-old-issues/issues/60

struct SCOCP
{
	UINT  wCodePageID;
	OnSetConsoleCP_t f;
	HANDLE hReady;
	DWORD dwErrCode;
	BOOL  lbRc;
};

DWORD WINAPI SetConsoleCPThread(LPVOID lpParameter)
{
	SCOCP* p = (SCOCP*)lpParameter;
	p->dwErrCode = -1;
	SetEvent(p->hReady);
	p->lbRc = p->f(p->wCodePageID);
	p->dwErrCode = GetLastError();
	return 0;
}

BOOL WINAPI OnSetConsoleCP(UINT wCodePageID)
{
	//typedef BOOL (WINAPI* OnSetConsoleCP_t)(UINT wCodePageID);
	ORIGINAL_KRNL(SetConsoleCP);
	_ASSERTE(OnSetConsoleCP!=SetConsoleCP);
	BOOL lbRc = FALSE;
	SCOCP sco = {wCodePageID, (OnSetConsoleCP_t)F(SetConsoleCP), CreateEvent(NULL,FALSE,FALSE,NULL)};
	DWORD nTID = 0, nWait = -1, nErr;
	/*
	wchar_t szErrText[255], szTitle[64];
	msprintf(szTitle, SKIPLEN(countof(szTitle)) L"PID=%u, TID=%u", GetCurrentProcessId(), GetCurrentThreadId());
	*/
	#ifdef _DEBUG
	DWORD nPrevCP = GetConsoleCP();
	#endif
	DWORD nCurCP = 0;
	HANDLE hThread = apiCreateThread(SetConsoleCPThread, &sco, &nTID, "OnSetConsoleCP(%u)", wCodePageID);

	if (!hThread)
	{
		nErr = GetLastError();
		_ASSERTE(hThread!=NULL);
		/*
		msprintf(szErrText, SKIPLEN(countof(szErrText)) L"chcp failed, ErrCode=0x%08X\nConEmuHooks.cpp:OnSetConsoleCP", nErr);
		Message BoxW(NULL, szErrText, szTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
		lbRc = FALSE;
		*/
	}
	else
	{
		HANDLE hEvents[2] = {hThread, sco.hReady};
		nWait = WaitForMultipleObjects(2, hEvents, FALSE, SETCONCP_READYTIMEOUT);

		if (nWait != WAIT_OBJECT_0)
			nWait = WaitForSingleObject(hThread, SETCONCP_TIMEOUT);

		if (nWait == WAIT_OBJECT_0)
		{
			lbRc = sco.lbRc;
		}
		else
		{
			//BUGBUG: На некоторых системах (Win2k3, WinXP) SetConsoleCP (и иже с ними) просто зависают
			apiTerminateThread(hThread,100);
			nCurCP = GetConsoleCP();
			if (nCurCP == wCodePageID)
			{
				lbRc = TRUE; // таки получилось
			}
			else
			{
				_ASSERTE(nCurCP == wCodePageID);
				/*
				msprintf(szErrText, SKIPLEN(countof(szErrText))
				          L"chcp hung detected\n"
				          L"ConEmuHooks.cpp:OnSetConsoleCP\n"
				          L"ReqCP=%u, PrevCP=%u, CurCP=%u\n"
				          //L"\nPress <OK> to stop waiting"
				          ,wCodePageID, nPrevCP, nCurCP);
				Message BoxW(NULL, szErrText, szTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
				*/
			}

			//nWait = WaitForSingleObject(hThread, 0);
			//if (nWait == WAIT_TIMEOUT)
			//	apiTerminateThread(hThread,100);
			//if (GetConsoleCP() == wCodePageID)
			//	lbRc = TRUE;
		}

		CloseHandle(hThread);
	}

	if (sco.hReady)
		CloseHandle(sco.hReady);

	return lbRc;
}

BOOL WINAPI OnSetConsoleOutputCP(UINT wCodePageID)
{
	//typedef BOOL (WINAPI* OnSetConsoleOutputCP_t)(UINT wCodePageID);
	ORIGINAL_KRNL(SetConsoleOutputCP);
	_ASSERTE(OnSetConsoleOutputCP!=SetConsoleOutputCP);
	BOOL lbRc = FALSE;
	SCOCP sco = {wCodePageID, (OnSetConsoleCP_t)F(SetConsoleOutputCP), CreateEvent(NULL,FALSE,FALSE,NULL)};
	DWORD nTID = 0, nWait = -1, nErr;
	/*
	wchar_t szErrText[255], szTitle[64];
	msprintf(szTitle, SKIPLEN(countof(szTitle)) L"PID=%u, TID=%u", GetCurrentProcessId(), GetCurrentThreadId());
	*/
	#ifdef _DEBUG
	DWORD nPrevCP = GetConsoleOutputCP();
	#endif
	DWORD nCurCP = 0;
	HANDLE hThread = apiCreateThread(SetConsoleCPThread, &sco, &nTID, "OnSetConsoleOutputCP(%u)", wCodePageID);

	if (!hThread)
	{
		nErr = GetLastError();
		_ASSERTE(hThread!=NULL);
		/*
		msprintf(szErrText, SKIPLEN(countof(szErrText)) L"chcp(out) failed, ErrCode=0x%08X\nConEmuHooks.cpp:OnSetConsoleOutputCP", nErr);
		Message BoxW(NULL, szErrText, szTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
		lbRc = FALSE;
		*/
	}
	else
	{
		HANDLE hEvents[2] = {hThread, sco.hReady};
		nWait = WaitForMultipleObjects(2, hEvents, FALSE, SETCONCP_READYTIMEOUT);

		if (nWait != WAIT_OBJECT_0)
			nWait = WaitForSingleObject(hThread, SETCONCP_TIMEOUT);

		if (nWait == WAIT_OBJECT_0)
		{
			lbRc = sco.lbRc;
		}
		else
		{
			//BUGBUG: На некоторых системах (Win2k3, WinXP) SetConsoleCP (и иже с ними) просто зависают
			apiTerminateThread(hThread,100);
			nCurCP = GetConsoleOutputCP();
			if (nCurCP == wCodePageID)
			{
				lbRc = TRUE; // таки получилось
			}
			else
			{
				_ASSERTE(nCurCP == wCodePageID);
				/*
				msprintf(szErrText, SKIPLEN(countof(szErrText))
				          L"chcp(out) hung detected\n"
				          L"ConEmuHooks.cpp:OnSetConsoleOutputCP\n"
				          L"ReqOutCP=%u, PrevOutCP=%u, CurOutCP=%u\n"
				          //L"\nPress <OK> to stop waiting"
				          ,wCodePageID, nPrevCP, nCurCP);
				Message BoxW(NULL, szErrText, szTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
				*/
			}
			//nWait = WaitForSingleObject(hThread, 0);
			//if (nWait == WAIT_TIMEOUT)
			//	apiTerminateThread(hThread,100);
			//if (GetConsoleOutputCP() == wCodePageID)
			//	lbRc = TRUE;
		}

		CloseHandle(hThread);
	}

	if (sco.hReady)
		CloseHandle(sco.hReady);

	return lbRc;
}


BOOL WINAPI OnAllocConsole(void)
{
	//typedef BOOL (WINAPI* OnAllocConsole_t)(void);
	ORIGINAL_KRNL(AllocConsole);
	BOOL lbRc = FALSE, lbAllocated = FALSE;
	COORD crLocked;
	HMODULE hKernel = NULL;
	DWORD nErrCode = 0;
	BOOL lbAttachRc = FALSE;
	HWND hOldConWnd = GetRealConsoleWindow();

	if (ph && ph->PreCallBack)
	{
		SETARGS(&lbRc);

		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	if (gbPrepareDefaultTerminal && gbIsNetVsHost)
	{
		if (!ghConWnd)
			gnVsHostStartConsole = 2;
		else
			gnVsHostStartConsole = 0;
	}

	// Попытаться создать консольное окно "по тихому"
	if (gpDefTerm && !hOldConWnd && !gnServerPID)
	{
		HWND hCreatedCon = gpDefTerm->AllocHiddenConsole(false);
		if (hCreatedCon)
		{
			hOldConWnd = hCreatedCon;
			lbAllocated = TRUE;
		}
	}

	// GUI приложение во вкладке. Если окна консоли еще нет - попробовать прицепиться
	// к родительской консоли (консоли серверного процесса)
	if ((gbAttachGuiClient || ghAttachGuiClient) && !gbPrepareDefaultTerminal)
	{
		if (AttachServerConsole())
		{
			hOldConWnd = GetRealConsoleWindow();
			lbAllocated = TRUE; // Консоль уже есть, ничего не надо
		}
	}

	DefTermMsg(L"AllocConsole calling");

	if (!lbAllocated && F(AllocConsole))
	{
		lbRc = F(AllocConsole)();

		if (lbRc && !gbPrepareDefaultTerminal && IsVisibleRectLocked(crLocked))
		{
			// Размер _видимой_ области. Консольным приложениям запрещено менять его "изнутри".
			// Размер может менять только пользователь ресайзом окна ConEmu
			HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
			CONSOLE_SCREEN_BUFFER_INFO csbi = {};
			if (GetConsoleScreenBufferInfo(hStdOut, &csbi))
			{
				//specified width and height cannot be less than the width and height of the console screen buffer's window
				SMALL_RECT rNewRect = {0, 0, crLocked.X-1, crLocked.Y-1};
				OnSetConsoleWindowInfo(hStdOut, TRUE, &rNewRect);
				#ifdef _DEBUG
				COORD crNewSize = {crLocked.X, max(crLocked.Y, csbi.dwSize.Y)};
				#endif
				SetConsoleScreenBufferSize(hStdOut, crLocked);
			}
		}

		if (lbRc)
		{
			int (WINAPI* fnRequestLocalServer)(/*[IN/OUT]*/RequestLocalServerParm* Parm);
			MModule server(WIN3264TEST(L"ConEmuCD.dll",L"ConEmuCD64.dll"));
			if (server.GetProcAddress("PrivateEntry",fnRequestLocalServer))
			{
				RequestLocalServerParm args = {sizeof(args)};
				args.Flags = slsf_OnAllocConsole;
				fnRequestLocalServer(&args);
			}
		}
	}

	//InitializeConsoleInputSemaphore();

	if (ph && ph->PostCallBack)
	{
		SETARGS(&lbRc);
		ph->PostCallBack(&args);
	}

	HWND hNewConWnd = GetRealConsoleWindow();

	#ifdef _DEBUG
	//_ASSERTEX(lbRc && ghConWnd);
	wchar_t szAlloc[500], szFile[MAX_PATH];
	GetModuleFileName(NULL, szFile, countof(szFile));
	msprintf(szAlloc, countof(szAlloc), L"OnAllocConsole\nOld=x%08X, New=x%08X, ghConWnd=x%08X\ngbPrepareDefaultTerminal=%i, gbIsNetVsHost=%i\n%s",
		LODWORD(hOldConWnd), LODWORD(hNewConWnd), LODWORD(ghConWnd), gbPrepareDefaultTerminal, gbIsNetVsHost, szFile);
	// VisualStudio host file calls AllocConsole TWICE(!)
	// Second call is totally spare (console already created)
	//MessageBox(NULL, szAlloc, L"OnAllocConsole called", MB_SYSTEMMODAL);
	#endif

	if (hNewConWnd)
	{
		if (gpDefTerm)
		{
			// This would start ConEmuC (console server) attached to our process
			DefTermMsg(L"Calling gpDefTerm->OnAllocConsoleFinished");
			// This would call OnConWndChanged too
			gpDefTerm->OnAllocConsoleFinished(hNewConWnd);
		}
		else
		{
			DefTermMsg(L"Console was already allocated");
			// Refresh ghConWnd, mapping, ServerPID, etc.
			OnConWndChanged(hNewConWnd);
		}
	}
	else
	{
		DefTermMsg(L"Something was wrong");
	}


	// On success - reset LastError (our functions may interfere)
	if (lbRc)
		SetLastError(0);

	TODO("Можно бы по настройке установить параметры. Кодовую страницу, например");

	return lbRc;
}


BOOL WINAPI OnFreeConsole(void)
{
	//typedef BOOL (WINAPI* OnFreeConsole_t)(void);
	ORIGINAL_KRNL(FreeConsole);
	BOOL lbRc = FALSE;

	if (ph && ph->PreCallBack)
	{
		SETARGS(&lbRc);

		if (!ph->PreCallBack(&args))
			return lbRc;
	}

	//ReleaseConsoleInputSemaphore();

	if (ghConWnd)
	{
		CLastErrorGuard guard;
		int (WINAPI* fnRequestLocalServer)(/*[IN/OUT]*/RequestLocalServerParm* Parm);
		MModule server(GetModuleHandle(WIN3264TEST(L"ConEmuCD.dll",L"ConEmuCD64.dll")));
		if (server.GetProcAddress("PrivateEntry",fnRequestLocalServer))
		{
			RequestLocalServerParm args = {sizeof(args)};
			args.Flags = slsf_OnFreeConsole;
			fnRequestLocalServer(&args);
		}
	}

	lbRc = F(FreeConsole)();

	if (ph && ph->PostCallBack)
	{
		SETARGS(&lbRc);
		ph->PostCallBack(&args);
	}

	return lbRc;
}


HWND WINAPI OnGetConsoleWindow(void)
{
	//typedef HWND (WINAPI* OnGetConsoleWindow_t)(void);
	ORIGINAL_KRNL(GetConsoleWindow);

	_ASSERTE(F(GetConsoleWindow) != GetRealConsoleWindow);
	// && F(GetConsoleWindow) != GetConsoleWindow - for minhook generation

	if (ghConEmuWndDC && IsWindow(ghConEmuWndDC) /*ghConsoleHwnd*/)
	{
		if (ghAttachGuiClient)
		{
			// В GUI режиме (notepad, putty) отдавать реальный результат GetRealConsoleWindow()
			// в этом режиме не нужно отдавать ни ghConEmuWndDC, ни серверную консоль
			HWND hReal = GetRealConsoleWindow();
			return hReal;
		}
		else
		{
			//return ghConsoleHwnd;
			return ghConEmuWndDC;
		}
	}

	HWND h;
	h = F(GetConsoleWindow)();
	return h;
}


// Undocumented function
BOOL WINAPI OnSetConsoleKeyShortcuts(BOOL bSet, BYTE bReserveKeys, LPVOID p1, DWORD n1)
{
	//typedef BOOL (WINAPI* OnSetConsoleKeyShortcuts_t)(BOOL,BYTE,LPVOID,DWORD);
	ORIGINAL_KRNL_EX(SetConsoleKeyShortcuts);
	BOOL lbRc = FALSE;

	if (F(SetConsoleKeyShortcuts))
		lbRc = F(SetConsoleKeyShortcuts)(bSet, bReserveKeys, p1, n1);

	if (ghConEmuWnd && IsWindow(ghConEmuWnd))
	{
		DWORD nLastErr = GetLastError();
		DWORD nSize = sizeof(CESERVER_REQ_HDR)+sizeof(BYTE)*2;
		CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_KEYSHORTCUTS, nSize);
		if (pIn)
		{
			pIn->Data[0] = bSet;
			pIn->Data[1] = bReserveKeys;

			wchar_t szGuiPipeName[128];
			msprintf(szGuiPipeName, countof(szGuiPipeName), CEGUIPIPENAME, L".", LODWORD(ghConWnd));

			CESERVER_REQ* pOut = ExecuteCmd(szGuiPipeName, pIn, 1000, NULL);

			if (pOut)
				ExecuteFreeResult(pOut);
			ExecuteFreeResult(pIn);
		}
		SetLastError(nLastErr);
	}

	return lbRc;
}


BOOL WINAPI OnGetCurrentConsoleFont(HANDLE hConsoleOutput, BOOL bMaximumWindow, PCONSOLE_FONT_INFO lpConsoleCurrentFont)
{
	//typedef BOOL (WINAPI* OnGetCurrentConsoleFont_t)(HANDLE hConsoleOutput, BOOL bMaximumWindow, PCONSOLE_FONT_INFO lpConsoleCurrentFont);
	ORIGINAL_KRNL(GetCurrentConsoleFont);
	BOOL lbRc = FALSE;
	COORD crSize = {};

	if (lpConsoleCurrentFont && MyGetConsoleFontSize(crSize))
	{
		lpConsoleCurrentFont->dwFontSize = crSize;
		lpConsoleCurrentFont->nFont = 1;
		lbRc = TRUE;
	}
	else if (F(GetCurrentConsoleFont))
		lbRc = F(GetCurrentConsoleFont)(hConsoleOutput, bMaximumWindow, lpConsoleCurrentFont);

	return lbRc;
}


COORD WINAPI OnGetConsoleFontSize(HANDLE hConsoleOutput, DWORD nFont)
{
	//typedef COORD (WINAPI* OnGetConsoleFontSize_t)(HANDLE hConsoleOutput, DWORD nFont);
	ORIGINAL_KRNL(GetConsoleFontSize);
	COORD cr = {};

	if (!MyGetConsoleFontSize(cr))
	{
		if (F(GetConsoleFontSize))
			cr = F(GetConsoleFontSize)(hConsoleOutput, nFont);
	}

	return cr;
}


HANDLE WINAPI OnCreateConsoleScreenBuffer(DWORD dwDesiredAccess, DWORD dwShareMode, const SECURITY_ATTRIBUTES *lpSecurityAttributes, DWORD dwFlags, LPVOID lpScreenBufferData)
{
	//typedef HANDLE(WINAPI* OnCreateConsoleScreenBuffer_t)(DWORD dwDesiredAccess, DWORD dwShareMode, const SECURITY_ATTRIBUTES *lpSecurityAttributes, DWORD dwFlags, LPVOID lpScreenBufferData);
	ORIGINAL_KRNL(CreateConsoleScreenBuffer);

	#ifdef SHOWCREATEBUFFERINFO
	wchar_t szDebugInfo[255];
	msprintf(szDebugInfo, countof(szDebugInfo), L"CreateConsoleScreenBuffer(0x%X,0x%X,0x%X,0x%X,0x%X)",
		dwDesiredAccess, dwShareMode, (DWORD)(DWORD_PTR)lpSecurityAttributes, dwFlags, (DWORD)(DWORD_PTR)lpScreenBufferData);

	#endif

	if ((dwShareMode & (FILE_SHARE_READ|FILE_SHARE_WRITE)) != (FILE_SHARE_READ|FILE_SHARE_WRITE))
		dwShareMode |= (FILE_SHARE_READ|FILE_SHARE_WRITE);

	if ((dwDesiredAccess & (GENERIC_READ|GENERIC_WRITE)) != (GENERIC_READ|GENERIC_WRITE))
		dwDesiredAccess |= (GENERIC_READ|GENERIC_WRITE);

	if (!ghStdOutHandle)
		ghStdOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);

	HANDLE h = INVALID_HANDLE_VALUE;
	if (F(CreateConsoleScreenBuffer))
		h = F(CreateConsoleScreenBuffer)(dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwFlags, lpScreenBufferData);

	if (h && (h != INVALID_HANDLE_VALUE))
		HandleKeeper::AllocHandleInfo(h, hs_CreateConsoleScreenBuffer);

#ifdef SHOWCREATEBUFFERINFO
	msprintf(szDebugInfo+lstrlen(szDebugInfo), 32, L"=0x%X", (DWORD)(DWORD_PTR)h);
	GuiMessageBox(ghConEmuWnd, szDebugInfo, L"ConEmuHk", MB_SETFOREGROUND|MB_SYSTEMMODAL);
#endif

	return h;
}


BOOL WINAPI OnSetConsoleActiveScreenBuffer(HANDLE hConsoleOutput)
{
	//typedef BOOL (WINAPI* OnSetConsoleActiveScreenBuffer_t)(HANDLE hConsoleOutput);
	ORIGINAL_KRNL(SetConsoleActiveScreenBuffer);

	if (!ghStdOutHandle)
		ghStdOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);

	#ifdef _DEBUG
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hIn  = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);
	#endif

	BOOL lbRc = FALSE;
	if (F(SetConsoleActiveScreenBuffer))
		lbRc = F(SetConsoleActiveScreenBuffer)(hConsoleOutput);

	if (lbRc && (ghCurrentOutBuffer || (hConsoleOutput != ghStdOutHandle)))
	{
		#ifdef SHOWCREATEBUFFERINFO
		CONSOLE_SCREEN_BUFFER_INFO lsbi = {};
		BOOL lbTest = GetConsoleScreenBufferInfo(hConsoleOutput, &lsbi);
		DWORD nErrCode = GetLastError();
		_ASSERTE(lbTest && lsbi.dwSize.Y && "GetConsoleScreenBufferInfo(hConsoleOutput) failed");
		#endif

		ghCurrentOutBuffer = hConsoleOutput;
		RequestLocalServerParm Parm = {(DWORD)sizeof(Parm), slsf_SetOutHandle, &ghCurrentOutBuffer};
		RequestLocalServer(&Parm);
	}

	return lbRc;
}


BOOL WINAPI OnSetConsoleWindowInfo(HANDLE hConsoleOutput, BOOL bAbsolute, const SMALL_RECT *lpConsoleWindow)
{
	//typedef BOOL (WINAPI* OnSetConsoleWindowInfo_t)(HANDLE hConsoleOutput, BOOL bAbsolute, const SMALL_RECT *lpConsoleWindow);
	ORIGINAL_KRNL(SetConsoleWindowInfo);
	BOOL lbRc = FALSE;
	SMALL_RECT tmp;
	COORD crLocked;

	#ifdef _DEBUG
	CONSOLE_SCREEN_BUFFER_INFO sbi = {};
	BOOL lbSbi = GetConsoleScreenBufferInfo(hConsoleOutput, &sbi);
	UNREFERENCED_PARAMETER(lbSbi);

	wchar_t szDbgSize[512];
	msprintf(szDbgSize, countof(szDbgSize), L"SetConsoleWindowInfo(%08X, %s, {%ix%i}-{%ix%i}), Current={%ix%i}, Wnd={%ix%i}-{%ix%i}\n",
		LODWORD(hConsoleOutput), bAbsolute ? L"ABS" : L"REL",
		lpConsoleWindow->Left, lpConsoleWindow->Top, lpConsoleWindow->Right, lpConsoleWindow->Bottom,
		sbi.dwSize.X, sbi.dwSize.Y,
		sbi.srWindow.Left, sbi.srWindow.Top, sbi.srWindow.Right, sbi.srWindow.Bottom);
	DebugStringConSize(szDbgSize);
	#endif

	BOOL lbLocked = IsVisibleRectLocked(crLocked);

	if (lpConsoleWindow && lbLocked)
	{
		tmp = *lpConsoleWindow;
		if (((tmp.Right - tmp.Left + 1) != crLocked.X) || ((tmp.Bottom - tmp.Top + 1) != crLocked.Y))
		{

			// Размер _видимой_ области. Консольным приложениям запрещено менять его "изнутри".
			// Размер может менять только пользователь ресайзом окна ConEmu
			if ((tmp.Right - tmp.Left + 1) != crLocked.X)
			{
				if (!bAbsolute)
				{
					WARNING("Need to be corrected!");
					tmp.Left = tmp.Right = 0;
				}
				else
				{
					tmp.Right = tmp.Left + crLocked.X - 1;
				}
			}

			if ((tmp.Bottom - tmp.Top + 1) != crLocked.Y)
			{
				if (!bAbsolute)
				{
					WARNING("Need to be corrected!");
					if (tmp.Top != tmp.Bottom)
					{
						tmp.Top = tmp.Bottom = 0;
					}
				}
				else
				{
					tmp.Bottom = tmp.Top + crLocked.Y - 1;
				}
			}

			lpConsoleWindow = &tmp;

			#ifdef _DEBUG
			msprintf(szDbgSize, countof(szDbgSize), L"---> IsVisibleRectLocked, lpConsoleWindow was patched {%ix%i}-{%ix%i}\n",
				tmp.Left, tmp.Top, tmp.Right, tmp.Bottom);
			DebugStringConSize(szDbgSize);
			#endif
		}
	}

	if (F(SetConsoleWindowInfo))
	{
		lbRc = F(SetConsoleWindowInfo)(hConsoleOutput, bAbsolute, lpConsoleWindow);
	}

	return lbRc;
}


BOOL WINAPI OnSetConsoleScreenBufferSize(HANDLE hConsoleOutput, COORD dwSize)
{
	//typedef BOOL (WINAPI* OnSetConsoleScreenBufferSize_t)(HANDLE hConsoleOutput, COORD dwSize);
	ORIGINAL_KRNL(SetConsoleScreenBufferSize);
	BOOL lbRc = FALSE, lbRetry = FALSE;
	COORD crLocked;
	DWORD dwErr = -1;

	CONSOLE_SCREEN_BUFFER_INFO sbi = {};
	BOOL lbSbi = GetConsoleScreenBufferInfo(hConsoleOutput, &sbi);
	UNREFERENCED_PARAMETER(lbSbi);

	#ifdef _DEBUG
	wchar_t szDbgSize[512];
	msprintf(szDbgSize, countof(szDbgSize), L"SetConsoleScreenBufferSize(%08X, {%ix%i}), Current={%ix%i}, Wnd={%ix%i}\n",
		LODWORD(hConsoleOutput), dwSize.X, dwSize.Y, sbi.dwSize.X, sbi.dwSize.Y,
		sbi.srWindow.Right-sbi.srWindow.Left+1, sbi.srWindow.Bottom-sbi.srWindow.Top+1);
	DebugStringConSize(szDbgSize);
	#endif

	BOOL lbLocked = IsVisibleRectLocked(crLocked);

	if (lbLocked && ((crLocked.X > dwSize.X) || (crLocked.Y > dwSize.Y)))
	{
		// Размер _видимой_ области. Консольным приложениям запрещено менять его "изнутри".
		// Размер может менять только пользователь ресайзом окна ConEmu
		if (crLocked.X > dwSize.X)
			dwSize.X = crLocked.X;
		if (crLocked.Y > dwSize.Y)
			dwSize.Y = crLocked.Y;

		#ifdef _DEBUG
		msprintf(szDbgSize, countof(szDbgSize), L"---> IsVisibleRectLocked, dwSize was patched {%ix%i}\n",
			dwSize.X, dwSize.Y);
		DebugStringConSize(szDbgSize);
		#endif
	}

	// Do not do useless calls
	if ((dwSize.X == sbi.dwSize.X) && (dwSize.Y == sbi.dwSize.Y))
	{
		lbRc = TRUE;
		goto wrap;
	}

	if (F(SetConsoleScreenBufferSize))
	{
		CESERVER_REQ *pIn = NULL, *pOut = NULL;
		LockServerReadingThread(true, dwSize, pIn, pOut);

		lbRc = F(SetConsoleScreenBufferSize)(hConsoleOutput, dwSize);
		dwErr = GetLastError();

		// The specified dimensions also cannot be less than the minimum size allowed
		// by the system. This minimum depends on the current font size for the console
		// (selected by the user) and the SM_CXMIN and SM_CYMIN values returned by the
		// GetSystemMetrics function.
		if (!lbRc && (dwErr == ERROR_INVALID_PARAMETER))
		{
			// Попытаться увеличить/уменьшить шрифт в консоли
			lbRetry = apiFixFontSizeForBufferSize(hConsoleOutput, dwSize);

			if (lbRetry)
			{
				lbRc = F(SetConsoleScreenBufferSize)(hConsoleOutput, dwSize);
				if (lbRc)
					dwErr = 0;
			}

			// Иногда при закрытии Far возникает ERROR_INVALID_PARAMETER
			// при этом, szDbgSize:
			// SetConsoleScreenBufferSize(0000000C, {80x1000}), Current={94x33}, Wnd={94x33}
			// т.е. коррекция не выполнялась
			_ASSERTE(lbRc && (dwErr != ERROR_INVALID_PARAMETER));
			if (!lbRc)
				SetLastError(dwErr); // вернуть "ошибку"
		}

		LockServerReadingThread(false, dwSize, pIn, pOut);
	}

wrap:
	return lbRc;
}


// WARNING!!! This function exist in Vista and higher OS only!!!
// Issue 1410
BOOL WINAPI OnSetCurrentConsoleFontEx(HANDLE hConsoleOutput, BOOL bMaximumWindow, MY_CONSOLE_FONT_INFOEX* lpConsoleCurrentFontEx)
{
	//typedef BOOL (WINAPI* OnSetCurrentConsoleFontEx_t)(HANDLE hConsoleOutput, BOOL bMaximumWindow, MY_CONSOLE_FONT_INFOEX* lpConsoleCurrentFontEx);
	ORIGINAL_KRNL_EX(SetCurrentConsoleFontEx);
	BOOL lbRc = FALSE;

	if (ghConEmuWndDC)
	{
		DebugString(L"Application tries to change console font! Prohibited!");
		//_ASSERTEX(FALSE && "Application tries to change console font! Prohibited!");
		//SetLastError(ERROR_INVALID_HANDLE);
		//Damn powershell close itself if received an error on this call
		lbRc = TRUE; // Cheating
		goto wrap;
	}

	if (F(SetCurrentConsoleFontEx))
		lbRc = F(SetCurrentConsoleFontEx)(hConsoleOutput, bMaximumWindow, lpConsoleCurrentFontEx);

wrap:
	return lbRc;
}


// WARNING!!! This function exist in Vista and higher OS only!!!
BOOL WINAPI OnSetConsoleScreenBufferInfoEx(HANDLE hConsoleOutput, MY_CONSOLE_SCREEN_BUFFER_INFOEX* lpConsoleScreenBufferInfoEx)
{
	//typedef BOOL (WINAPI* OnSetConsoleScreenBufferInfoEx_t)(HANDLE hConsoleOutput, MY_CONSOLE_SCREEN_BUFFER_INFOEX* lpConsoleScreenBufferInfoEx);
	ORIGINAL_KRNL_EX(SetConsoleScreenBufferInfoEx);
	BOOL lbRc = FALSE;

	COORD crLocked;
	DWORD dwErr = -1;

	CONSOLE_SCREEN_BUFFER_INFO sbi = {};
	BOOL lbSbi = GetConsoleScreenBufferInfo(hConsoleOutput, &sbi);
	UNREFERENCED_PARAMETER(lbSbi);

	#ifdef _DEBUG
	wchar_t szDbgSize[512];
	if (lpConsoleScreenBufferInfoEx)
	{
		msprintf(szDbgSize, countof(szDbgSize), L"SetConsoleScreenBufferInfoEx(%08X, {%ix%i}), Current={%ix%i}, Wnd={%ix%i}\n",
			LODWORD(hConsoleOutput), lpConsoleScreenBufferInfoEx->dwSize.X, lpConsoleScreenBufferInfoEx->dwSize.Y, sbi.dwSize.X, sbi.dwSize.Y,
			sbi.srWindow.Right-sbi.srWindow.Left+1, sbi.srWindow.Bottom-sbi.srWindow.Top+1);
	}
	else
	{
		lstrcpyn(szDbgSize, L"SetConsoleScreenBufferInfoEx(%08X, NULL)\n", LODWORD(hConsoleOutput));
	}
	DebugStringConSize(szDbgSize);
	#endif

	BOOL lbLocked = IsVisibleRectLocked(crLocked);

	if (lbLocked && lpConsoleScreenBufferInfoEx
		&& ((crLocked.X > lpConsoleScreenBufferInfoEx->dwSize.X) || (crLocked.Y > lpConsoleScreenBufferInfoEx->dwSize.Y)))
	{
		// Размер _видимой_ области. Консольным приложениям запрещено менять его "изнутри".
		// Размер может менять только пользователь ресайзом окна ConEmu
		if (crLocked.X > lpConsoleScreenBufferInfoEx->dwSize.X)
			lpConsoleScreenBufferInfoEx->dwSize.X = crLocked.X;
		if (crLocked.Y > lpConsoleScreenBufferInfoEx->dwSize.Y)
			lpConsoleScreenBufferInfoEx->dwSize.Y = crLocked.Y;

		#ifdef _DEBUG
		msprintf(szDbgSize, countof(szDbgSize), L"---> IsVisibleRectLocked, dwSize was patched {%ix%i}\n",
			lpConsoleScreenBufferInfoEx->dwSize.X, lpConsoleScreenBufferInfoEx->dwSize.Y);
		DebugStringConSize(szDbgSize);
		#endif
	}

	if (F(SetConsoleScreenBufferInfoEx))
	{
		CESERVER_REQ *pIn = NULL, *pOut = NULL;
		if (lpConsoleScreenBufferInfoEx)
			LockServerReadingThread(true, lpConsoleScreenBufferInfoEx->dwSize, pIn, pOut);

		lbRc = F(SetConsoleScreenBufferInfoEx)(hConsoleOutput, lpConsoleScreenBufferInfoEx);
		dwErr = GetLastError();

		if (lpConsoleScreenBufferInfoEx)
			LockServerReadingThread(false, lpConsoleScreenBufferInfoEx->dwSize, pIn, pOut);
	}

	UNREFERENCED_PARAMETER(dwErr);
	return lbRc;
}


COORD WINAPI OnGetLargestConsoleWindowSize(HANDLE hConsoleOutput)
{
	//typedef COORD (WINAPI* OnGetLargestConsoleWindowSize_t)(HANDLE hConsoleOutput);
	ORIGINAL_KRNL(GetLargestConsoleWindowSize);
	COORD cr = {80,25}, crLocked = {0,0};

	if (ghConEmuWndDC && IsVisibleRectLocked(crLocked))
	{
		cr = crLocked;
	}
	else
	{
		if (F(GetLargestConsoleWindowSize))
		{
			cr = F(GetLargestConsoleWindowSize)(hConsoleOutput);
		}

		// Wine BUG
		//if (!cr.X || !cr.Y)
		if ((cr.X == 80 && cr.Y == 24) && IsWine())
		{
			cr.X = 255;
			cr.Y = 255;
		}
	}

	return cr;
}


BOOL WINAPI OnSetConsoleCursorPosition(HANDLE hConsoleOutput, COORD dwCursorPosition)
{
	//typedef BOOL (WINAPI* OnSetConsoleCursorPosition_t)(HANDLE,COORD);
	ORIGINAL_KRNL(SetConsoleCursorPosition);

	BOOL lbRc;

	if (gbIsVimAnsi)
	{
		#ifdef DUMP_VIM_SETCURSORPOS
		wchar_t szDbg[80]; msprintf(szDbg, countof(szDbg), L"ViM trying to set cursor pos: {%i,%i}\n", (UINT)dwCursorPosition.X, (UINT)dwCursorPosition.Y);
		OutputDebugString(szDbg);
		#endif
		lbRc = FALSE;
	}
	else
	{
		lbRc = F(SetConsoleCursorPosition)(hConsoleOutput, dwCursorPosition);
	}

	if (lbRc && CEAnsi::ghAnsiLogFile
		&& !CEAnsi::gbAnsiWasNewLine
		&& (dwCursorPosition.X == 0))
	{
		CEAnsi::gbAnsiLogNewLine = true;
	}

	if (ghConsoleCursorChanged)
		SetEvent(ghConsoleCursorChanged);

	return lbRc;
}


BOOL WINAPI OnSetConsoleCursorInfo(HANDLE hConsoleOutput, const CONSOLE_CURSOR_INFO *lpConsoleCursorInfo)
{
	//typedef BOOL (WINAPI* OnSetConsoleCursorInfo_t)(HANDLE,const CONSOLE_CURSOR_INFO *);
	ORIGINAL_KRNL(SetConsoleCursorInfo);

	BOOL lbRc = F(SetConsoleCursorInfo)(hConsoleOutput, lpConsoleCursorInfo);

	if (ghConsoleCursorChanged)
		SetEvent(ghConsoleCursorChanged);

	return lbRc;
}
