
/*
Copyright (c) 2009-2017 Maximus5
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


#include "ConEmuC.h"
#include "../common/WFiles.h"

bool GetAliases(wchar_t* asExeName, wchar_t** rsAliases, LPDWORD rnAliasesSize);

#ifndef _WIN32_WINNT
PRAGMA_ERROR("_WIN32_WINNT not defined");
#elif (_WIN32_WINNT<0x0501)
// Хотя ConsoleAliases доступны уже в Win2k, но объявлены почему-то только для WinXP
PRAGMA_ERROR("_WIN32_WINNT is less then 5.01");
#endif

#if __GNUC__
extern "C" {
#ifndef AddConsoleAlias
	BOOL WINAPI AddConsoleAliasA(LPSTR Source, LPSTR Target, LPSTR ExeName);
	BOOL WINAPI AddConsoleAliasW(LPWSTR Source, LPWSTR Target, LPWSTR ExeName);
#define AddConsoleAlias  AddConsoleAliasW
#endif

#ifndef GetConsoleAlias
	DWORD WINAPI GetConsoleAliasA(LPSTR Source, LPSTR TargetBuffer, DWORD TargetBufferLength, LPSTR ExeName);
	DWORD WINAPI GetConsoleAliasW(LPWSTR Source, LPWSTR TargetBuffer, DWORD TargetBufferLength, LPWSTR ExeName);
#define GetConsoleAlias  GetConsoleAliasW
#endif

#ifndef GetConsoleAliasesLength
	DWORD WINAPI GetConsoleAliasesLengthA(LPSTR ExeName);
	DWORD WINAPI GetConsoleAliasesLengthW(LPWSTR ExeName);
#define GetConsoleAliasesLength  GetConsoleAliasesLengthW
#endif

#ifndef GetConsoleAliases
	DWORD WINAPI GetConsoleAliasesA(LPSTR AliasBuffer, DWORD AliasBufferLength, LPSTR ExeName);
	DWORD WINAPI GetConsoleAliasesW(LPWSTR AliasBuffer, DWORD AliasBufferLength, LPWSTR ExeName);
#define GetConsoleAliases  GetConsoleAliasesW
#endif
}
#endif

#ifndef AddConsoleAlias
PRAGMA_ERROR("AddConsoleAlias was not defined");
#endif


int ComspecInit()
{
	TODO("Определить код родительского процесса, и если это FAR - запомнить его (для подключения к пайпу плагина)");
	TODO("Размер получить из GUI, если оно есть, иначе - по умолчанию");
	TODO("GUI может скорректировать размер с учетом полосы прокрутки");
	WARNING("CreateFile(CONOUT$) по идее возвращает текущий ScreenBuffer. Можно его самим возвращать в ComspecDone");

	// Правда нужно проверить, что там происходит с ghConOut.Close(),...
	// Размер должен менять сам GUI, через серверный ConEmuC!
#ifdef SHOW_STARTED_MSGBOX
	MessageBox(GetConEmuHWND(2), L"ConEmuC (comspec mode) is about to START", L"ConEmuC.ComSpec", 0);
#endif
	//int nNewBufferHeight = 0;
	//COORD crNewSize = {0,0};
	//SMALL_RECT rNewWindow = gpSrv->sbi.srWindow;
	BOOL lbSbiRc = FALSE;
	gbRootWasFoundInCon = 2; // не добавлять к "Press Enter to close console" - "or wait"
	gbComspecInitCalled = TRUE; // Нельзя вызывать ComspecDone, если не было вызова ComspecInit
	// в режиме ComSpec - запрещено!
	gbAlwaysConfirmExit = FALSE; gbAutoDisableConfirmExit = FALSE;
#ifdef _DEBUG
	xf_validate();
	xf_dump_chk();
#endif
	// Это наверное и не нужно, просто для информации...
	lbSbiRc = GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &gpSrv->sbi);
	#ifdef _DEBUG
	DWORD nErrCode = lbSbiRc ? 0 : GetLastError();
	// Процесс запущен с редиректом вывода?
	_ASSERTE(lbSbiRc || (nErrCode == ERROR_INVALID_HANDLE));
	#endif

#if 0
	// 111211 - "-new_console" теперь передается в GUI и исполняется в нем
	// Сюда мы попадаем если был ключик -new_console
	// А этом случае нужно завершить ЭТОТ экземпляр и запустить в ConEmu новую вкладку
	if (gpSrv->bNewConsole)
	{
#ifdef _DEBUG
		xf_validate();
		xf_dump_chk();
#endif
		PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
		STARTUPINFOW si; memset(&si, 0, sizeof(si)); si.cb = sizeof(si);
		si.dwFlags = STARTF_USESHOWWINDOW|STARTF_USECOUNTCHARS;
		si.dwXCountChars = gpSrv->sbi.dwSize.X;
		si.dwYCountChars = gpSrv->sbi.dwSize.Y;
		si.wShowWindow = SW_HIDE;
		PRINT_COMSPEC(L"Creating new console for:\n%s\n", gpszRunCmd);
#ifdef _DEBUG
		xf_validate();
		xf_dump_chk();
#endif
		// CREATE_NEW_PROCESS_GROUP - низя, перестает работать Ctrl-C
		// Запускается новый сервер (новая консоль), сюда хуки ставить не надо.
		BOOL lbRc = createProcess(TRUE, NULL, gpszRunCmd, NULL,NULL, TRUE,
		                           NORMAL_PRIORITY_CLASS|CREATE_NEW_CONSOLE,
		                           NULL, NULL, &si, &pi);
		DWORD dwErr = GetLastError();

		if (!lbRc)
		{
			PrintExecuteError(gpszRunCmd, dwErr);
			return CERR_CREATEPROCESS;
		}

#ifdef _DEBUG
		xf_validate();
		xf_dump_chk();
#endif
		//delete psNewCmd; psNewCmd = NULL;
		AllowSetForegroundWindow(pi.dwProcessId);
		PRINT_COMSPEC(L"New console created. PID=%i. Exiting...\n", pi.dwProcessId);
		SafeCloseHandle(pi.hProcess); SafeCloseHandle(pi.hThread);
		DisableAutoConfirmExit();
		//gpSrv->nProcessStartTick = GetTickCount() - 2*CHECK_ROOTSTART_TIMEOUT; // менять nProcessStartTick не нужно. проверка только по флажкам
#ifdef _DEBUG
		xf_validate();
		xf_dump_chk();
#endif
		return CERR_RUNNEWCONSOLE;
	}
#endif

	wchar_t szComSpec[MAX_PATH+1];
	const wchar_t* pszComSpecName = NULL;

	WARNING("TCC/ComSpec");
	if (GetEnvironmentVariable(L"ComSpec", szComSpec, MAX_PATH) && szComSpec[0] != 0)
	{
		pszComSpecName = (wchar_t*)PointToName(szComSpec);
		if (IsConsoleServer(pszComSpecName))
			pszComSpecName = NULL;
	}
	if (!pszComSpecName || !*pszComSpecName)
	{
		WARNING("TCC/ComSpec");
		pszComSpecName = L"cmd.exe";
	}

	lstrcpyn(gpSrv->szComSpecName, pszComSpecName, countof(gpSrv->szComSpecName));

	if (pszComSpecName)
	{
		wchar_t szSelf[MAX_PATH+1];

		if (GetModuleFileName(NULL, szSelf, MAX_PATH))
		{
			lstrcpyn(gpSrv->szSelfName, (wchar_t*)PointToName(szSelf), countof(gpSrv->szSelfName));

			if (!GetAliases(gpSrv->szSelfName, &gpSrv->pszPreAliases, &gpSrv->nPreAliasSize))
			{
				if (gpSrv->pszPreAliases)
				{
					_wprintf(gpSrv->pszPreAliases);
					free(gpSrv->pszPreAliases);
					gpSrv->pszPreAliases = NULL;
				}
			}
		}
	}

	SendStarted();
	//ConOutCloseHandle()
	return 0;
}


void ComspecDone(int aiRc)
{
#ifdef _DEBUG
	xf_dump_chk();
	xf_validate(NULL);
#endif
	//WARNING("Послать в GUI CONEMUCMDSTOPPED");
	LogSize(NULL, 0, "ComspecDone");

	// Это необходимо делать, т.к. при смене буфера (SetConsoleActiveScreenBuffer) приложением,
	// дескриптор нужно закрыть, иначе conhost может не вернуть предыдущий буфер
	//ConOutCloseHandle()

	// Поддержка алиасов
	if (gpSrv->szComSpecName[0] && gpSrv->szSelfName[0])
	{
		// Скопировать алиасы из cmd.exe в conemuc.exe
		wchar_t *pszPostAliases = NULL;
		DWORD nPostAliasSize;
		BOOL lbChanged = (gpSrv->pszPreAliases == NULL);

		if (!GetAliases(gpSrv->szComSpecName, &pszPostAliases, &nPostAliasSize))
		{
			if (pszPostAliases)
				_wprintf(pszPostAliases);
		}
		else
		{
			if (!lbChanged)
			{
				lbChanged = (gpSrv->nPreAliasSize!=nPostAliasSize);
			}

			if (!lbChanged && gpSrv->nPreAliasSize && gpSrv->pszPreAliases && pszPostAliases)
			{
				lbChanged = memcmp(gpSrv->pszPreAliases,pszPostAliases,gpSrv->nPreAliasSize)!=0;
			}

			if (lbChanged)
			{
				xf_dump_chk();

				if (gnMainServerPID)
				{
					MCHKHEAP;
					CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_SAVEALIASES,sizeof(CESERVER_REQ_HDR)+nPostAliasSize);

					if (pIn)
					{
						MCHKHEAP;
						memmove(pIn->Data, pszPostAliases, nPostAliasSize);
						MCHKHEAP;
						CESERVER_REQ* pOut = ExecuteSrvCmd(gnMainServerPID, pIn, GetConEmuHWND(2), FALSE, 0, TRUE);
						MCHKHEAP;

						if (pOut) ExecuteFreeResult(pOut);

						ExecuteFreeResult(pIn);
						MCHKHEAP;
					}
				}

				xf_dump_chk();
				wchar_t *pszNewName = pszPostAliases, *pszNewTarget, *pszNewLine;

				while (pszNewName && *pszNewName)
				{
					pszNewLine = pszNewName + lstrlen(pszNewName);
					pszNewTarget = wcschr(pszNewName, L'=');

					if (pszNewTarget)
					{
						*(pszNewTarget++) = 0;
						if (*pszNewTarget == 0)
							pszNewTarget = NULL;
					}

					AddConsoleAlias(pszNewName, pszNewTarget, gpSrv->szSelfName);
					pszNewName = pszNewLine+1;
				}

				xf_dump_chk();
			}
		}

		if (pszPostAliases)
		{
			free(pszPostAliases); pszPostAliases = NULL;
		}
	}

	xf_dump_chk();
	//TODO("Уведомить плагин через пайп (если родитель - FAR) что процесс завершен. Плагин должен считать и запомнить содержимое консоли и только потом вернуть управление в ConEmuC!");
	DWORD dwErr1 = 0; //, dwErr2 = 0;
	HANDLE hOut1 = NULL, hOut2 = NULL;
	BOOL lbRc1 = FALSE, lbRc2 = FALSE;
	CONSOLE_SCREEN_BUFFER_INFO sbi1 = {{0,0}}, sbi2 = {{0,0}};

	#ifdef _DEBUG
	HWND hWndCon = GetConEmuHWND(2);
	#endif

	// Тут нужна реальная, а не скорректированная информация!
	if (!gbNonGuiMode)
	{
		// Если GUI не сможет через сервер вернуть высоту буфера - это нужно сделать нам!
		lbRc1 = GetConsoleScreenBufferInfo(hOut1 = GetStdHandle(STD_OUTPUT_HANDLE), &sbi1);

		if (!lbRc1)
			dwErr1 = GetLastError();

		xf_dump_chk();
	}

	//PRAGMA_ERROR("Размер должен возвращать сам GUI, через серверный ConEmuC!");
	#ifdef SHOW_STARTED_MSGBOX
	MessageBox(GetConEmuHWND(2), L"ConEmuC (comspec mode) is about to TERMINATE", L"ConEmuC.ComSpec", 0);
	#endif

	#ifdef _DEBUG
	xf_dump_chk();
	xf_validate(NULL);
	#endif

	if (!gbNonGuiMode && (gpSrv->dwParentFarPID != 0))
	{
		//// Вернуть размер буфера (высота И ширина)
		//if (gpSrv->sbi.dwSize.X && gpSrv->sbi.dwSize.Y) {
		//	SMALL_RECT rc = {0};
		//	SetConsoleSize(0, gpSrv->sbi.dwSize, rc, "ComspecDone");
		//}
		//ConOutCloseHandle()
		CONSOLE_SCREEN_BUFFER_INFO l_csbi = {{0}};
		lbRc2 = GetConsoleScreenBufferInfo(hOut2 = GetStdHandle(STD_OUTPUT_HANDLE), &l_csbi);

		CESERVER_REQ *pOut = SendStopped(&l_csbi);

		if (pOut)
		{
			if (!pOut->StartStopRet.bWasBufferHeight)
			{
				//gpSrv->sbi.dwSize = pIn->StartStop.sbi.dwSize;
				lbRc1 = FALSE; // Консольное приложение самостоятельно сбросило буферный режим. Не дергаться...
			}
			else
			{
				lbRc1 = TRUE;
			}

			ExecuteFreeResult(pOut); pOut = NULL;
		}

		if (!gbWasBufferHeight)
		{
			lbRc2 = GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &sbi2);

			#ifdef _DEBUG
			if (sbi2.dwSize.Y > 200)
			{
				wchar_t szTitle[128]; _wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuC (PID=%i)", GetCurrentProcessId());
				MessageBox(NULL, L"BufferHeight was not turned OFF", szTitle, MB_SETFOREGROUND|MB_SYSTEMMODAL);
			}
			#endif

			if (lbRc1 && lbRc2 && sbi2.dwSize.Y == sbi1.dwSize.Y)
			{
				// GUI не смог вернуть высоту буфера...
				// Это плохо, т.к. фар высоту буфера не меняет и будет сильно глючить на N сотнях строк...
				int nNeedHeight = gpSrv->sbi.dwSize.Y;

				if (nNeedHeight < 10)
				{
					nNeedHeight = (sbi2.srWindow.Bottom-sbi2.srWindow.Top+1);
				}

				if (sbi2.dwSize.Y != nNeedHeight)
				{
					_ASSERTE(sbi2.dwSize.Y == nNeedHeight);
					PRINT_COMSPEC(L"Error: BufferHeight was not changed from %i\n", sbi2.dwSize.Y);
					SMALL_RECT rc = {0};
					sbi2.dwSize.Y = nNeedHeight;

					if (gpLogSize) LogSize(&sbi2.dwSize, 0, ":ComspecDone.RetSize.before");

					SetConsoleSize(0, sbi2.dwSize, rc, "ComspecDone.Force");

					if (gpLogSize) LogSize(NULL, 0, ":ComspecDone.RetSize.after");
				}
			}
		}
	}

	if (gpSrv->pszPreAliases) { free(gpSrv->pszPreAliases); gpSrv->pszPreAliases = NULL; }

	//SafeCloseHandle(ghCtrlCEvent);
	//SafeCloseHandle(ghCtrlBreakEvent);
}

bool GetAliases(wchar_t* asExeName, wchar_t** rsAliases, LPDWORD rnAliasesSize)
{
	bool lbRc = false;
	DWORD nAliasRC, nAliasErr, nAliasAErr = 0, nSizeA = 0;
	_ASSERTE(asExeName && rsAliases && rnAliasesSize);
	_ASSERTE(*rsAliases == NULL);
	*rnAliasesSize = GetConsoleAliasesLength(asExeName);

	if (*rnAliasesSize == 0)
	{
		lbRc = true;
	}
	else
	{
		*rsAliases = (wchar_t*)calloc(*rnAliasesSize+2,1);
		nAliasRC = GetConsoleAliases(*rsAliases,*rnAliasesSize,asExeName);

		if (nAliasRC)
		{
			lbRc = true;
		}
		else
		{
			nAliasErr = GetLastError();

			if (nAliasErr == ERROR_NOT_ENOUGH_MEMORY)
			{
				// Попробовать ANSI функции
				UINT nCP = CP_OEMCP;
				char szExeName[MAX_PATH+1];
				char *pszAliases = NULL;
				WideCharToMultiByte(nCP,0,asExeName,-1,szExeName,MAX_PATH+1,0,0);
				nSizeA = GetConsoleAliasesLengthA(szExeName);

				if (nSizeA)
				{
					pszAliases = (char*)calloc(nSizeA+1,1);
					nAliasRC = GetConsoleAliasesA(pszAliases,nSizeA,szExeName);

					if (nAliasRC)
					{
						lbRc = true;
						MultiByteToWideChar(nCP,0,pszAliases,nSizeA,*rsAliases,((*rnAliasesSize)/2)+1);
					}
					else
					{
						nAliasAErr = GetLastError();
					}

					free(pszAliases);
				}
			}

			if (!nAliasRC)
			{
				if ((*rnAliasesSize) < 255) { free(*rsAliases); *rsAliases = (wchar_t*)calloc(128,2); }

				_wsprintf(*rsAliases, SKIPLEN(127) L"\nConEmuC: GetConsoleAliases failed, ErrCode=0x%08X(0x%08X), AliasesLength=%i(%i)\n\n", nAliasErr, nAliasAErr, *rnAliasesSize, nSizeA);
			}
		}
	}

	return lbRc;
}
