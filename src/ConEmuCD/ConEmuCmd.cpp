
/*
Copyright (c) 2009-present Maximus5
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


#include "ConsoleMain.h"
#include "ConEmuCmd.h"
#include "ConEmuSrv.h"
#include "ConsoleState.h"
#include "ExitCodes.h"
#include "../common/EnvVar.h"
#include "../common/WFiles.h"


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


WorkerComspec::~WorkerComspec()  // NOLINT(modernize-use-equals-default)
{
	_ASSERTE(gpState->runMode_ == RunMode::Comspec || gpState->runMode_ == RunMode::Undefined);
}

WorkerComspec::WorkerComspec()  // NOLINT(modernize-use-equals-default)
	: WorkerBase()
{
	_ASSERTE(gpState->runMode_ == RunMode::Comspec || gpState->runMode_ == RunMode::Undefined);
}

int WorkerComspec::Init()
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
	gbRootWasFoundInCon = 2; // не добавлять к "Press Enter to close console" - "or wait"
	gbComspecInitCalled = TRUE; // Нельзя вызывать ComspecDone, если не было вызова ComspecInit
	// в режиме ComSpec - запрещено!
	gpState->alwaysConfirmExit_ = false;
	gpState->autoDisableConfirmExit_ = false;
#ifdef _DEBUG
	xf_validate();
	xf_dump_chk();
#endif

	// sbi is used to restore properties
	const auto lbSbiRc = GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &this->consoleInfo.sbi);
	if (!lbSbiRc)
	{
		if (!((this->consoleInfo.dwSbiRc = GetLastError())))
			this->consoleInfo.dwSbiRc = -1;
	}
	else
	{
		this->consoleInfo.dwSbiRc = 0;
	}
	// Process was started with redirection?
	_ASSERTE(lbSbiRc || (this->consoleInfo.dwSbiRc == ERROR_INVALID_HANDLE));

	wchar_t szComSpec[MAX_PATH+1];
	const wchar_t* pszComSpecName = nullptr;

	WARNING("TCC/ComSpec");
	if (GetEnvironmentVariable(L"ComSpec", szComSpec, MAX_PATH) && szComSpec[0] != 0)
	{
		pszComSpecName = const_cast<wchar_t*>(PointToName(szComSpec));
		if (IsConsoleServer(pszComSpecName))
			pszComSpecName = nullptr;
	}
	if (!pszComSpecName || !*pszComSpecName)
	{
		WARNING("TCC/ComSpec");
		pszComSpecName = L"cmd.exe";
	}

	lstrcpyn(szComSpecName, pszComSpecName, countof(szComSpecName));

	if (pszComSpecName)
	{
		wchar_t szSelf[MAX_PATH+1];

		if (GetModuleFileName(nullptr, szSelf, MAX_PATH))
		{
			lstrcpyn(szSelfName, PointToName(szSelf), countof(szSelfName));

			if (!GetAliases(szSelfName, &pszPreAliases, &nPreAliasSize))
			{
				if (pszPreAliases)
				{
					_wprintf(pszPreAliases);
					free(pszPreAliases);
					pszPreAliases = nullptr;
				}
			}
		}
	}

	SendStarted();
	//ConOutCloseHandle()
	return 0;
}


void WorkerComspec::Done(const int exitCode, const bool reportShutdown)
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
	if (szComSpecName[0] && szSelfName[0])
	{
		// Скопировать алиасы из cmd.exe в conemuc.exe
		wchar_t *pszPostAliases = nullptr;
		DWORD nPostAliasSize;
		bool lbChanged = (pszPreAliases == nullptr);

		if (!GetAliases(szComSpecName, &pszPostAliases, &nPostAliasSize))
		{
			if (pszPostAliases)
				_wprintf(pszPostAliases);
		}
		else
		{
			if (!lbChanged)
			{
				lbChanged = (nPreAliasSize!=nPostAliasSize);
			}

			if (!lbChanged && nPreAliasSize && pszPreAliases && pszPostAliases)
			{
				lbChanged = memcmp(pszPreAliases,pszPostAliases,nPreAliasSize)!=0;
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

					AddConsoleAlias(pszNewName, pszNewTarget, szSelfName);
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

	if (!gbNonGuiMode && (gpWorker->ParentFarPid() != 0))
	{
		CONSOLE_SCREEN_BUFFER_INFO l_csbi = {{0}};
		lbRc2 = GetConsoleScreenBufferInfo(hOut2 = GetStdHandle(STD_OUTPUT_HANDLE), &l_csbi);

		CESERVER_REQ *pOut = SendStopped(&l_csbi);

		if (pOut)
		{
			if (!pOut->StartStopRet.bWasBufferHeight)
			{
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
				wchar_t szTitle[128]; swprintf_c(szTitle, L"ConEmuC (PID=%i)", GetCurrentProcessId());
				MessageBox(nullptr, L"BufferHeight was not turned OFF", szTitle, MB_SETFOREGROUND|MB_SYSTEMMODAL);
			}
			#endif

			if (lbRc1 && lbRc2 && sbi2.dwSize.Y == sbi1.dwSize.Y)
			{
				// If ConEmu GUI fails to return buffer height, it could be bad for Far Manager
				// Far does not change buffer height and could feel dizzy on thousands of rows
				int nNeedHeight = this->consoleInfo.sbi.dwSize.Y;

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

					if (gpLogSize) LogSize(nullptr, 0, ":ComspecDone.RetSize.after");
				}
			}
		}
	}

	if (pszPreAliases)
	{
		free(pszPreAliases);
		pszPreAliases = nullptr;
	}

	// Final steps
	WorkerBase::Done(exitCode, reportShutdown);
}

int WorkerComspec::ProcessNewConsoleArg(LPCWSTR asCmdLine)
{
	HWND hConWnd = gpState->realConWnd_, hConEmu = gpState->conemuWnd_;
	if (!hConWnd)
	{
		// This may be ConEmuC started from WSL or connector
		CEStr guiPid(GetEnvVar(ENV_CONEMUPID_VAR_W));
		CEStr srvPid(GetEnvVar(ENV_CONEMUSERVERPID_VAR_W));
		if (guiPid && srvPid)
		{
			DWORD GuiPID = wcstoul(guiPid, NULL, 10);
			DWORD SrvPID = wcstoul(srvPid, NULL, 10);
			ConEmuGuiMapping GuiMapping = { sizeof(GuiMapping) };
			if (GuiPID && LoadGuiMapping(GuiPID, GuiMapping))
			{
				for (size_t i = 0; i < countof(GuiMapping.Consoles); ++i)
				{
					if (GuiMapping.Consoles[i].ServerPID == SrvPID)
					{
						hConWnd = GuiMapping.Consoles[i].Console;
						hConEmu = GuiMapping.hGuiWnd;
						break;
					}
				}
			}
		}
	}

	if (hConWnd)
	{
		xf_check();
		// тогда обрабатываем
		bNewConsole = true;

		// По идее, должен запускаться в табе ConEmu (в существующей консоли), но если нет
		if (!hConEmu || !IsWindow(hConEmu))
		{
			// попытаться найти открытый ConEmu
			hConEmu = FindWindowEx(NULL, NULL, VirtualConsoleClassMain, NULL);
			if (hConEmu)
				gbNonGuiMode = TRUE; // Чтобы не пытаться выполнить SendStopped (ибо некому)
		}

		int iNewConRc = CERR_RUNNEWCONSOLE;

		// Query current environment
		CEnvStrings strs(GetEnvironmentStringsW());

		DWORD nCmdLen = lstrlen(asCmdLine) + 1;
		CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_NEWCMD, sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_NEWCMD) + ((nCmdLen + strs.mcch_Length) * sizeof(wchar_t)));
		if (pIn)
		{
			pIn->NewCmd.hFromConWnd = hConWnd;

			// hConWnd may differ from parent process, but ENV_CONEMUDRAW_VAR_W would be inherited
			wchar_t* pszDcWnd = GetEnvVar(ENV_CONEMUDRAW_VAR_W);
			if (pszDcWnd && (pszDcWnd[0] == L'0') && (pszDcWnd[1] == L'x'))
			{
				wchar_t* pszEnd = NULL;
				pIn->NewCmd.hFromDcWnd.u = wcstoul(pszDcWnd + 2, &pszEnd, 16);
			}
			SafeFree(pszDcWnd);

			GetCurrentDirectory(countof(pIn->NewCmd.szCurDir), pIn->NewCmd.szCurDir);
			pIn->NewCmd.SetCommand(asCmdLine);
			pIn->NewCmd.SetEnvStrings(strs.ms_Strings, static_cast<DWORD>(strs.mcch_Length));

			CESERVER_REQ* pOut = ExecuteGuiCmd(hConEmu, pIn, hConWnd);
			if (pOut)
			{
				if (pOut->hdr.cbSize <= sizeof(pOut->hdr) || pOut->Data[0] == FALSE)
				{
					iNewConRc = CERR_RUNNEWCONSOLEFAILED;
				}
				ExecuteFreeResult(pOut);
			}
			else
			{
				_ASSERTE(pOut != NULL);
				iNewConRc = CERR_RUNNEWCONSOLEFAILED;
			}
			ExecuteFreeResult(pIn);
		}
		else
		{
			iNewConRc = CERR_NOTENOUGHMEM1;
		}

		DisableAutoConfirmExit();
		return iNewConRc;
	}

	// Executed outside of ConEmu, impossible to bypass command to new console
	_ASSERTE(hConWnd != NULL);
	return 0; // try to continue as usual
}

bool WorkerComspec::IsCmdK() const
{
	return bK;
}

void WorkerComspec::SetCmdK(bool useCmdK)
{
	bK = useCmdK;
}


bool WorkerComspec::GetAliases(wchar_t* asExeName, wchar_t** rsAliases, LPDWORD rnAliasesSize) const
{
	bool lbRc = false;
	// ReSharper disable once CppJoinDeclarationAndAssignment
	DWORD nAliasRC, nAliasErr;
	DWORD nAliasAErr = 0, nSizeA = 0;
	_ASSERTE(asExeName && rsAliases && rnAliasesSize);
	_ASSERTE(*rsAliases == nullptr);
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

				swprintf_c(*rsAliases, 127/*#SECURELEN*/, L"\nConEmuC: GetConsoleAliases failed, ErrCode=0x%08X(0x%08X), AliasesLength=%i(%i)\n\n", nAliasErr, nAliasAErr, *rnAliasesSize, nSizeA);
			}
		}
	}

	return lbRc;
}
