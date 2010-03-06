
/*
Copyright (c) 2009-2010 Maximus5
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
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

bool GetAliases(wchar_t* asExeName, wchar_t** rsAliases, LPDWORD rnAliasesSize);

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



int ComspecInit()
{
	TODO("Определить код родительского процесса, и если это FAR - запомнить его (для подключения к пайпу плагина)");
	TODO("Размер получить из GUI, если оно есть, иначе - по умолчанию");
	TODO("GUI может скорректировать размер с учетом полосы прокрутки");
	
	WARNING("CreateFile(CONOUT$) по идее возвращает текущий ScreenBuffer. Можно его самим возвращать в ComspecDone");
	// Правда нужно проверить, что там происходит с ghConOut.Close(),... 

	// Размер должен менять сам GUI, через серверный ConEmuC!
	#ifdef SHOW_STARTED_MSGBOX
	MessageBox(GetConsoleWindow(), L"ConEmuC (comspec mode) is about to START", L"ConEmuC.ComSpec", 0);
	#endif


	//int nNewBufferHeight = 0;
	//COORD crNewSize = {0,0};
	//SMALL_RECT rNewWindow = cmd.sbi.srWindow;
	BOOL lbSbiRc = FALSE;
	
	gbRootWasFoundInCon = 2; // не добавлять к "Press Enter to close console" - "or wait"

	// в режиме ComSpec - запрещено!
	gbAlwaysConfirmExit = FALSE; gbAutoDisableConfirmExit = FALSE;

	// Это наверное и не нужно, просто для информации...
	lbSbiRc = MyGetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cmd.sbi);
	
	
	// Сюда мы попадаем если был ключик -new_console
	// А этом случае нужно завершить ЭТОТ экземпляр и запустить в ConEmu новую вкладку
	if (cmd.bNewConsole) {
		PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
		STARTUPINFOW si; memset(&si, 0, sizeof(si)); si.cb = sizeof(si);
		si.dwFlags = STARTF_USESHOWWINDOW|STARTF_USECOUNTCHARS;
		si.dwXCountChars = cmd.sbi.dwSize.X;
		si.dwYCountChars = cmd.sbi.dwSize.Y;
		si.wShowWindow = SW_HIDE;
		
		PRINT_COMSPEC(L"Creating new console for:\n%s\n", gpszRunCmd);
	
		// CREATE_NEW_PROCESS_GROUP - низя, перестает работать Ctrl-C
		BOOL lbRc = CreateProcessW(NULL, gpszRunCmd, NULL,NULL, TRUE, 
				NORMAL_PRIORITY_CLASS|CREATE_NEW_CONSOLE, 
				NULL, NULL, &si, &pi);
		DWORD dwErr = GetLastError();
		if (!lbRc)
		{
			_printf ("Can't create process, ErrCode=0x%08X! Command to be executed:\n", dwErr, gpszRunCmd);
			return CERR_CREATEPROCESS;
		}
		//delete psNewCmd; psNewCmd = NULL;
		AllowSetForegroundWindow(pi.dwProcessId);
		PRINT_COMSPEC(L"New console created. PID=%i. Exiting...\n", pi.dwProcessId);
		SafeCloseHandle(pi.hProcess); SafeCloseHandle(pi.hThread);
		DisableAutoConfirmExit();
		//srv.nProcessStartTick = GetTickCount() - 2*CHECK_ROOTSTART_TIMEOUT; // менять nProcessStartTick не нужно. проверка только по флажкам
		return CERR_RUNNEWCONSOLE;
	}
	
	
	// Если определена ComSpecC - значит ConEmuC переопределил стандартный ComSpec
	// Вернем его
	wchar_t szComSpec[MAX_PATH+1], *pszComSpecName;
	if (GetEnvironmentVariable(L"ComSpecC", szComSpec, MAX_PATH) && szComSpec[0] != 0)
	{
		// Только если это (случайно) не conemuc.exe
		wchar_t* pwszCopy = (wchar_t*)PointToName(szComSpec); //wcsrchr(szComSpec, L'\\'); 
		//if (!pwszCopy) pwszCopy = szComSpec;
		#pragma warning( push )
		#pragma warning(disable : 6400)
		if (lstrcmpiW(pwszCopy, L"ConEmuC")==0 || lstrcmpiW(pwszCopy, L"ConEmuC.exe")==0)
			szComSpec[0] = 0;
		#pragma warning( pop )
		if (szComSpec[0]) {
			SetEnvironmentVariable(L"ComSpec", szComSpec);
			SetEnvironmentVariable(L"ComSpecC", NULL);
		}
		pszComSpecName = (wchar_t*)PointToName(szComSpec);
	} else {
		pszComSpecName = L"cmd.exe";
	}
	lstrcpyn(cmd.szComSpecName, pszComSpecName, countof(cmd.szComSpecName));


	if (pszComSpecName) {
		wchar_t szSelf[MAX_PATH+1];
		if (GetModuleFileName(NULL, szSelf, MAX_PATH)) {
			lstrcpyn(cmd.szSelfName, (wchar_t*)PointToName(szSelf), countof(cmd.szSelfName));

			if (!GetAliases(cmd.szSelfName, &cmd.pszPreAliases, &cmd.nPreAliasSize)) {
				if (cmd.pszPreAliases) {
					wprintf(cmd.pszPreAliases);
					free(cmd.pszPreAliases);
					cmd.pszPreAliases = NULL;
				}
			}
		}
	}
	
	SendStarted();
	
	//ghConOut.Close();
	
	return 0;
}


void ComspecDone(int aiRc)
{
	//WARNING("Послать в GUI CONEMUCMDSTOPPED");
	LogSize(NULL, "ComspecDone");

	// Это необходимо делать, т.к. при смене буфера (SetConsoleActiveScreenBuffer) приложением,
	// дескриптор нужно закрыть, иначе conhost может не вернуть предыдущий буфер
	//ghConOut.Close();

	// Поддержка алиасов
	if (cmd.szComSpecName[0] && cmd.szSelfName[0]) {
		// Скопировать алиасы из cmd.exe в conemuc.exe
		wchar_t *pszPostAliases = NULL;
		DWORD nPostAliasSize;
		BOOL lbChanged = (cmd.pszPreAliases == NULL);

		if (!GetAliases(cmd.szComSpecName, &pszPostAliases, &nPostAliasSize)) {
			if (pszPostAliases) wprintf(pszPostAliases);
		} else {
			if (!lbChanged) {
				lbChanged = (cmd.nPreAliasSize!=nPostAliasSize);
			}
			if (!lbChanged && cmd.nPreAliasSize && cmd.pszPreAliases && pszPostAliases) {
				lbChanged = memcmp(cmd.pszPreAliases,pszPostAliases,cmd.nPreAliasSize)!=0;
			}
			if (lbChanged) {
				if (cmd.dwSrvPID) {
					MCHKHEAP;
					CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_SAVEALIASES,sizeof(CESERVER_REQ_HDR)+nPostAliasSize);
					if (pIn) {
						MCHKHEAP;
						memmove(pIn->Data, pszPostAliases, nPostAliasSize);
						MCHKHEAP;
						CESERVER_REQ* pOut = ExecuteSrvCmd(cmd.dwSrvPID, pIn, GetConsoleWindow());
						MCHKHEAP;
						if (pOut) ExecuteFreeResult(pOut);
						ExecuteFreeResult(pIn);
						MCHKHEAP;
					}
				}

				wchar_t *pszNewName = pszPostAliases, *pszNewTarget, *pszNewLine;
				while (pszNewName && *pszNewName) {
					pszNewLine = pszNewName + lstrlen(pszNewName);
					pszNewTarget = wcschr(pszNewName, L'=');
					if (pszNewTarget) {
						*pszNewTarget = 0;
						pszNewTarget++;
					}
					if (*pszNewTarget == 0) pszNewTarget = NULL;
					AddConsoleAlias(pszNewName, pszNewTarget, cmd.szSelfName);
					pszNewName = pszNewLine+1;
				}
			}
		}
		if (pszPostAliases) { free(pszPostAliases); pszPostAliases = NULL; }
	}
	

	//TODO("Уведомить плагин через пайп (если родитель - FAR) что процесс завершен. Плагин должен считать и запомнить содержимое консоли и только потом вернуть управление в ConEmuC!");

	DWORD dwErr1 = 0, dwErr2 = 0;
	HANDLE hOut1 = NULL, hOut2 = NULL;
	BOOL lbRc1 = FALSE, lbRc2 = FALSE;
	CONSOLE_SCREEN_BUFFER_INFO sbi1 = {{0,0}}, sbi2 = {{0,0}};
	#ifdef _DEBUG
	HWND hWndCon = GetConsoleWindow();
	#endif
	// Тут нужна реальная, а не скорректированная информация!
	if (!cmd.bNonGuiMode) {
		// Если GUI не сможет через сервер вернуть высоту буфера - это нужно сделать нам!
		lbRc1 = GetConsoleScreenBufferInfo(hOut1 = GetStdHandle(STD_OUTPUT_HANDLE), &sbi1);
		if (!lbRc1)
			dwErr1 = GetLastError();
	}


	//PRAGMA_ERROR("Размер должен возвращать сам GUI, через серверный ConEmuC!");
	#ifdef SHOW_STARTED_MSGBOX
	MessageBox(GetConsoleWindow(), L"ConEmuC (comspec mode) is about to TERMINATE", L"ConEmuC.ComSpec", 0);
	#endif
	
	if (!cmd.bNonGuiMode)
	{
		//// Вернуть размер буфера (высота И ширина)
		//if (cmd.sbi.dwSize.X && cmd.sbi.dwSize.Y) {
		//	SMALL_RECT rc = {0};
		//	SetConsoleSize(0, cmd.sbi.dwSize, rc, "ComspecDone");
		//}
		
		//ghConOut.Close();

		CESERVER_REQ *pIn = NULL, *pOut = NULL;
		int nSize = sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_STARTSTOP);
		pIn = ExecuteNewCmd(CECMD_CMDSTARTSTOP,nSize);
		if (pIn) {
			pIn->StartStop.nStarted = 3; // Cmd режим завершен
			pIn->StartStop.hWnd = ghConWnd;
			pIn->StartStop.dwPID = gnSelfPID;
			pIn->StartStop.nSubSystem = gnImageSubsystem;
			// НЕ MyGet..., а то можем заблокироваться...
			// ghConOut может быть NULL, если ошибка произошла во время разбора аргументов
			lbRc2 = GetConsoleScreenBufferInfo(hOut2 = GetStdHandle(STD_OUTPUT_HANDLE), &pIn->StartStop.sbi);
			if (!lbRc2)
				dwErr2 = GetLastError();

			PRINT_COMSPEC(L"Finalizing comspec mode (ExecuteGuiCmd started)\n",0);
			pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
			PRINT_COMSPEC(L"Finalizing comspec mode (ExecuteGuiCmd finished)\n",0);
			if (pOut) {
				if (!pOut->StartStopRet.bWasBufferHeight) {
					//cmd.sbi.dwSize = pIn->StartStop.sbi.dwSize;
					lbRc1 = FALSE; // Консольное приложение самостоятельно сбросило буферный режим. Не дергаться...
				} else {
					lbRc1 = TRUE;
				}
				ExecuteFreeResult(pOut); pOut = NULL;
			}
			ExecuteFreeResult(pIn); pIn = NULL; // не освобождалось
		}

		lbRc2 = GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &sbi2);
		#ifdef _DEBUG
		if (sbi2.dwSize.Y > 200) {
			wchar_t szTitle[128]; wsprintfW(szTitle, L"ConEmuC (PID=%i)", GetCurrentProcessId());
			MessageBox(NULL, L"BufferHeight was not turned OFF", szTitle, MB_SETFOREGROUND|MB_SYSTEMMODAL);
		}
		#endif
		if (lbRc1 && lbRc2 && sbi2.dwSize.Y == sbi1.dwSize.Y) {
			// GUI не смог вернуть высоту буфера... 
			// Это плохо, т.к. фар высоту буфера не меняет и будет сильно глючить на N сотнях строк...
			int nNeedHeight = cmd.sbi.dwSize.Y;
			if (nNeedHeight < 10) {
				nNeedHeight = (sbi2.srWindow.Bottom-sbi2.srWindow.Top+1);
			}
			if (sbi2.dwSize.Y != nNeedHeight) {
				PRINT_COMSPEC(L"Error: BufferHeight was not changed from %i\n", sbi2.dwSize.Y);
				SMALL_RECT rc = {0};
				sbi2.dwSize.Y = nNeedHeight;
				if (ghLogSize) LogSize(&sbi2.dwSize, ":ComspecDone.RetSize.before");
				SetConsoleSize(0, sbi2.dwSize, rc, "ComspecDone.Force");
				if (ghLogSize) LogSize(NULL, ":ComspecDone.RetSize.after");
			}
		}
	}

	if (cmd.pszPreAliases) { free(cmd.pszPreAliases); cmd.pszPreAliases = NULL; }
	
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
	if (*rnAliasesSize == 0) {
		lbRc = true;
	} else {
		*rsAliases = (wchar_t*)calloc(*rnAliasesSize+2,1);
		nAliasRC = GetConsoleAliases(*rsAliases,*rnAliasesSize,asExeName);
		if (nAliasRC) {
			lbRc = true;
		} else {
			nAliasErr = GetLastError();
			if (nAliasErr == ERROR_NOT_ENOUGH_MEMORY) {
				// Попробовать ANSI функции
				UINT nCP = CP_OEMCP;
				char szExeName[MAX_PATH+1];
				char *pszAliases = NULL;
				WideCharToMultiByte(nCP,0,asExeName,-1,szExeName,MAX_PATH+1,0,0);
				nSizeA = GetConsoleAliasesLengthA(szExeName);
				if (nSizeA) {
					pszAliases = (char*)calloc(nSizeA+1,1);
					nAliasRC = GetConsoleAliasesA(pszAliases,nSizeA,szExeName);
					if (nAliasRC) {
						lbRc = true;
						MultiByteToWideChar(nCP,0,pszAliases,nSizeA,*rsAliases,((*rnAliasesSize)/2)+1);
					} else {
						nAliasAErr = GetLastError();
					}
					free(pszAliases);
				}
			}
			if (!nAliasRC) {
				if ((*rnAliasesSize) < 255) { free(*rsAliases); *rsAliases = (wchar_t*)calloc(128,2); }
				wsprintf(*rsAliases, L"\nConEmuC: GetConsoleAliases failed, ErrCode=0x%08X(0x%08X), AliasesLength=%i(%i)\n\n", nAliasErr, nAliasAErr, *rnAliasesSize, nSizeA);
			}
		}
	}
	return lbRc;
}
