
/*
Copyright (c) 2009-2012 Maximus5
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

#define SHOWDEBUGSTR

#include "ConEmuC.h"
#include "../common/ConsoleAnnotation.h"
#include "../common/Execute.h"
#include "TokenHelper.h"
#include "SrvPipes.h"
#include "Queue.h"

//#define DEBUGSTRINPUTEVENT(s) //DEBUGSTR(s) // SetEvent(gpSrv->hInputEvent)
//#define DEBUGLOGINPUT(s) DEBUGSTR(s) // ConEmuC.MouseEvent(X=
//#define DEBUGSTRINPUTWRITE(s) DEBUGSTR(s) // *** ConEmuC.MouseEvent(X=
//#define DEBUGSTRINPUTWRITEALL(s) //DEBUGSTR(s) // *** WriteConsoleInput(Write=
//#define DEBUGSTRINPUTWRITEFAIL(s) DEBUGSTR(s) // ### WriteConsoleInput(Write=
//#define DEBUGSTRCHANGES(s) DEBUGSTR(s)

#define MAX_EVENTS_PACK 20

#ifdef _DEBUG
//#define ASSERT_UNWANTED_SIZE
#else
#undef ASSERT_UNWANTED_SIZE
#endif

extern BOOL gbTerminateOnExit; // для отладчика
extern BOOL gbTerminateOnCtrlBreak;
extern OSVERSIONINFO gOSVer;

//BOOL ProcessInputMessage(MSG64 &msg, INPUT_RECORD &r);
//BOOL SendConsoleEvent(INPUT_RECORD* pr, UINT nCount);
//BOOL ReadInputQueue(INPUT_RECORD *prs, DWORD *pCount);
//BOOL WriteInputQueue(const INPUT_RECORD *pr);
//BOOL IsInputQueueEmpty();
//BOOL WaitConsoleReady(BOOL abReqEmpty); // Дождаться, пока консольный буфер готов принять события ввода. Возвращает FALSE, если сервер закрывается!
//DWORD WINAPI InputThread(LPVOID lpvParam);
int CreateColorerHeader();


// Установить мелкий шрифт, иначе может быть невозможно увеличение размера GUI окна
void ServerInitFont()
{
	// Размер шрифта и Lucida. Обязательно для серверного режима.
	if (gpSrv->szConsoleFont[0])
	{
		// Требуется проверить наличие такого шрифта!
		LOGFONT fnt = {0};
		lstrcpynW(fnt.lfFaceName, gpSrv->szConsoleFont, LF_FACESIZE);
		gpSrv->szConsoleFont[0] = 0; // сразу сбросим. Если шрифт есть - имя будет скопировано в FontEnumProc
		HDC hdc = GetDC(NULL);
		EnumFontFamiliesEx(hdc, &fnt, (FONTENUMPROCW) FontEnumProc, (LPARAM)&fnt, 0);
		DeleteDC(hdc);
	}

	if (gpSrv->szConsoleFont[0] == 0)
	{
		lstrcpyW(gpSrv->szConsoleFont, L"Lucida Console");
		gpSrv->nConFontWidth = 3; gpSrv->nConFontHeight = 5;
	}

	if (gpSrv->nConFontHeight<5) gpSrv->nConFontHeight = 5;

	if (gpSrv->nConFontWidth==0 && gpSrv->nConFontHeight==0)
	{
		gpSrv->nConFontWidth = 3; gpSrv->nConFontHeight = 5;
	}
	else if (gpSrv->nConFontWidth==0)
	{
		gpSrv->nConFontWidth = gpSrv->nConFontHeight * 2 / 3;
	}
	else if (gpSrv->nConFontHeight==0)
	{
		gpSrv->nConFontHeight = gpSrv->nConFontWidth * 3 / 2;
	}

	if (gpSrv->nConFontHeight<5 || gpSrv->nConFontWidth <3)
	{
		gpSrv->nConFontWidth = 3; gpSrv->nConFontHeight = 5;
	}

	if (gbAttachMode && gbNoCreateProcess && gpSrv->dwRootProcess && gbAttachFromFar)
	{
		// Скорее всего это аттач из Far плагина. Попробуем установить шрифт в консоли через плагин.
		wchar_t szPipeName[128];
		_wsprintf(szPipeName, SKIPLEN(countof(szPipeName)) CEPLUGINPIPENAME, L".", gpSrv->dwRootProcess);
		CESERVER_REQ In;
		ExecutePrepareCmd(&In, CMD_SET_CON_FONT, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_SETFONT));
		In.Font.cbSize = sizeof(In.Font);
		In.Font.inSizeX = gpSrv->nConFontWidth;
		In.Font.inSizeY = gpSrv->nConFontHeight;
		lstrcpy(In.Font.sFontName, gpSrv->szConsoleFont);
		CESERVER_REQ *pPlgOut = ExecuteCmd(szPipeName, &In, 500, ghConWnd);

		if (pPlgOut) ExecuteFreeResult(pPlgOut);
	}
	else if (!gbAlienMode || gOSVer.dwMajorVersion >= 6)
	{
		if (ghLogSize) LogSize(NULL, ":SetConsoleFontSizeTo.before");

		SetConsoleFontSizeTo(ghConWnd, gpSrv->nConFontHeight, gpSrv->nConFontWidth, gpSrv->szConsoleFont);

		if (ghLogSize) LogSize(NULL, ":SetConsoleFontSizeTo.after");
	}
}

BOOL LoadGuiSettings(ConEmuGuiMapping& GuiMapping)
{
	BOOL lbRc = FALSE;
	if (ghConEmuWnd && IsWindow(ghConEmuWnd))
	{
		DWORD dwGuiThreadId, dwGuiProcessId;
		MFileMapping<ConEmuGuiMapping> GuiInfoMapping;
		dwGuiThreadId = GetWindowThreadProcessId(ghConEmuWnd, &dwGuiProcessId);

		if (!dwGuiThreadId)
		{
			_ASSERTE(dwGuiProcessId);
		}
		else
		{
			GuiInfoMapping.InitName(CEGUIINFOMAPNAME, dwGuiProcessId);
			const ConEmuGuiMapping* pInfo = GuiInfoMapping.Open();

			if (pInfo
				&& pInfo->cbSize == sizeof(ConEmuGuiMapping)
				&& pInfo->nProtocolVersion == CESERVER_REQ_VER)
			{
				memmove(&GuiMapping, pInfo, pInfo->cbSize);
				lbRc = TRUE;
			}
		}
	}
	return lbRc;
}

BOOL ReloadGuiSettings(ConEmuGuiMapping* apFromCmd)
{
	BOOL lbRc = FALSE;
	if (apFromCmd)
	{
		DWORD cbSize = min(sizeof(gpSrv->guiSettings), apFromCmd->cbSize);
		memmove(&gpSrv->guiSettings, apFromCmd, cbSize);
		_ASSERTE(cbSize == apFromCmd->cbSize);
		gpSrv->guiSettings.cbSize = cbSize;
		lbRc = TRUE;
	}
	else
	{
		gpSrv->guiSettings.cbSize = sizeof(ConEmuGuiMapping);
		lbRc = LoadGuiSettings(gpSrv->guiSettings);
	}

	if (lbRc)
	{
		gbLogProcess = (gpSrv->guiSettings.nLoggingType == glt_Processes);

		UpdateComspec(&gpSrv->guiSettings.ComSpec);

		SetEnvironmentVariableW(L"ConEmuDir", gpSrv->guiSettings.sConEmuDir);
		SetEnvironmentVariableW(L"ConEmuBaseDir", gpSrv->guiSettings.sConEmuBaseDir);

		// Не будем ставить сами, эту переменную заполняет Gui при своем запуске
		// соответственно, переменная наследуется серверами
		//SetEnvironmentVariableW(L"ConEmuArgs", pInfo->sConEmuArgs);

		wchar_t szHWND[16]; _wsprintf(szHWND, SKIPLEN(countof(szHWND)) L"0x%08X", gpSrv->guiSettings.hGuiWnd.u);
		SetEnvironmentVariableW(L"ConEmuHWND", szHWND);

		if (gpSrv->pConsole)
		{
			// !!! Warning !!! Изменил здесь, поменяй и CreateMapHeader() !!!
			
			gpSrv->pConsole->hdr.nLoggingType = gpSrv->guiSettings.nLoggingType;
			gpSrv->pConsole->hdr.bDosBox = gpSrv->guiSettings.bDosBox;
			gpSrv->pConsole->hdr.bUseInjects = gpSrv->guiSettings.bUseInjects;
			gpSrv->pConsole->hdr.bUseTrueColor = gpSrv->guiSettings.bUseTrueColor;

			// Обновить пути к ConEmu
			wcscpy_c(gpSrv->pConsole->hdr.sConEmuExe, gpSrv->guiSettings.sConEmuExe);
			wcscpy_c(gpSrv->pConsole->hdr.sConEmuBaseDir, gpSrv->guiSettings.sConEmuBaseDir);

			// И настройки командного процессора
			gpSrv->pConsole->hdr.ComSpec = gpSrv->guiSettings.ComSpec;

			// Проверить, нужно ли реестр хукать
			gpSrv->pConsole->hdr.isHookRegistry = gpSrv->guiSettings.isHookRegistry;
			wcscpy_c(gpSrv->pConsole->hdr.sHiveFileName, gpSrv->guiSettings.sHiveFileName);
			gpSrv->pConsole->hdr.hMountRoot = gpSrv->guiSettings.hMountRoot;
			wcscpy_c(gpSrv->pConsole->hdr.sMountKey, gpSrv->guiSettings.sMountKey);

			// !!! Warning !!! Изменил здесь, поменяй и CreateMapHeader() !!!
			
			UpdateConsoleMapHeader();
		}
		else
		{
			lbRc = FALSE;
		}
	}
	return lbRc;
}

// Вызывается при запуске сервера: (gbNoCreateProcess && (gbAttachMode || gpSrv->bDebuggerActive))
int AttachRootProcess()
{
	DWORD dwErr = 0;

	if (!gpSrv->bDebuggerActive && !IsWindowVisible(ghConWnd) && !(gpSrv->dwGuiPID || gbAttachFromFar))
	{
		PRINT_COMSPEC(L"Console windows is not visible. Attach is unavailable. Exiting...\n", 0);
		DisableAutoConfirmExit();
		//gpSrv->nProcessStartTick = GetTickCount() - 2*CHECK_ROOTSTART_TIMEOUT; // менять nProcessStartTick не нужно. проверка только по флажкам
		#ifdef _DEBUG
		xf_validate();
		xf_dump_chk();
		#endif
		return CERR_RUNNEWCONSOLE;
	}

	if (gpSrv->dwRootProcess == 0 && !gpSrv->bDebuggerActive)
	{
		// Нужно попытаться определить PID корневого процесса.
		// Родительским может быть cmd (comspec, запущенный из FAR)
		DWORD dwParentPID = 0, dwFarPID = 0;
		DWORD dwServerPID = 0; // Вдруг в этой консоли уже есть сервер?
		_ASSERTE(!gpSrv->bDebuggerActive);

		if (gpSrv->nProcessCount >= 2 && !gpSrv->bDebuggerActive)
		{
			HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);

			if (hSnap != INVALID_HANDLE_VALUE)
			{
				PROCESSENTRY32 prc = {sizeof(PROCESSENTRY32)};

				if (Process32First(hSnap, &prc))
				{
					do
					{
						for(UINT i = 0; i < gpSrv->nProcessCount; i++)
						{
							if (prc.th32ProcessID != gnSelfPID
							        && prc.th32ProcessID == gpSrv->pnProcesses[i])
							{
								if (lstrcmpiW(prc.szExeFile, L"conemuc.exe")==0
								        /*|| lstrcmpiW(prc.szExeFile, L"conemuc64.exe")==0*/)
								{
									CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_ATTACH2GUI, 0);
									CESERVER_REQ* pOut = ExecuteSrvCmd(prc.th32ProcessID, pIn, ghConWnd);

									if (pOut) dwServerPID = prc.th32ProcessID;

									ExecuteFreeResult(pIn); ExecuteFreeResult(pOut);

									// Если команда успешно выполнена - выходим
									if (dwServerPID)
										break;
								}

								if (!dwFarPID && lstrcmpiW(prc.szExeFile, L"far.exe")==0)
								{
									dwFarPID = prc.th32ProcessID;
								}

								if (!dwParentPID)
									dwParentPID = prc.th32ProcessID;
							}
						}

						// Если уже выполнили команду в сервере - выходим, перебор больше не нужен
						if (dwServerPID)
							break;
					}
					while(Process32Next(hSnap, &prc));
				}

				CloseHandle(hSnap);

				if (dwFarPID) dwParentPID = dwFarPID;
			}
		}

		if (dwServerPID)
		{
			AllowSetForegroundWindow(dwServerPID);
			PRINT_COMSPEC(L"Server was already started. PID=%i. Exiting...\n", dwServerPID);
			DisableAutoConfirmExit(); // сервер уже есть?
			// менять nProcessStartTick не нужно. проверка только по флажкам
			//gpSrv->nProcessStartTick = GetTickCount() - 2*CHECK_ROOTSTART_TIMEOUT;
			#ifdef _DEBUG
			xf_validate();
			xf_dump_chk();
			#endif
			return CERR_RUNNEWCONSOLE;
		}

		if (!dwParentPID)
		{
			_printf("Attach to GUI was requested, but there is no console processes:\n", 0, GetCommandLineW()); //-V576
			_ASSERTE(FALSE);
			return CERR_CARGUMENT;
		}

		// Нужно открыть HANDLE корневого процесса
		gpSrv->hRootProcess = OpenProcess(PROCESS_QUERY_INFORMATION|SYNCHRONIZE, FALSE, dwParentPID);

		if (!gpSrv->hRootProcess)
		{
			dwErr = GetLastError();
			wchar_t* lpMsgBuf = NULL;
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&lpMsgBuf, 0, NULL);
			_printf("Can't open process (%i) handle, ErrCode=0x%08X, Description:\n", //-V576
			        dwParentPID, dwErr, (lpMsgBuf == NULL) ? L"<Unknown error>" : lpMsgBuf);

			if (lpMsgBuf) LocalFree(lpMsgBuf);
			SetLastError(dwErr);

			return CERR_CREATEPROCESS;
		}

		gpSrv->dwRootProcess = dwParentPID;
		// Запустить вторую копию ConEmuC НЕМОДАЛЬНО!
		wchar_t szSelf[MAX_PATH+100];
		wchar_t* pszSelf = szSelf+1;

		if (!GetModuleFileName(NULL, pszSelf, MAX_PATH))
		{
			dwErr = GetLastError();
			_printf("GetModuleFileName failed, ErrCode=0x%08X\n", dwErr);
			SetLastError(dwErr);
			return CERR_CREATEPROCESS;
		}

		if (wcschr(pszSelf, L' '))
		{
			*(--pszSelf) = L'"';
			lstrcatW(pszSelf, L"\"");
		}

		wsprintf(pszSelf+lstrlen(pszSelf), L" /ATTACH /PID=%i", dwParentPID);
		PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
		STARTUPINFOW si; memset(&si, 0, sizeof(si)); si.cb = sizeof(si);
		PRINT_COMSPEC(L"Starting modeless:\n%s\n", pszSelf);
		// CREATE_NEW_PROCESS_GROUP - низя, перестает работать Ctrl-C
		// Это запуск нового сервера в этой консоли. В сервер хуки ставить не нужно
		BOOL lbRc = createProcess(TRUE, NULL, pszSelf, NULL,NULL, TRUE,
		                           NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
		dwErr = GetLastError();

		if (!lbRc)
		{
			PrintExecuteError(pszSelf, dwErr);
			SetLastError(dwErr);
			return CERR_CREATEPROCESS;
		}

		//delete psNewCmd; psNewCmd = NULL;
		AllowSetForegroundWindow(pi.dwProcessId);
		PRINT_COMSPEC(L"Modeless server was started. PID=%i. Exiting...\n", pi.dwProcessId);
		SafeCloseHandle(pi.hProcess); SafeCloseHandle(pi.hThread);
		DisableAutoConfirmExit(); // сервер запущен другим процессом, чтобы не блокировать bat файлы
		// менять nProcessStartTick не нужно. проверка только по флажкам
		//gpSrv->nProcessStartTick = GetTickCount() - 2*CHECK_ROOTSTART_TIMEOUT;
		#ifdef _DEBUG
		xf_validate();
		xf_dump_chk();
		#endif
		return CERR_RUNNEWCONSOLE;
	}
	else
	{
		// Нужно открыть HANDLE корневого процесса
		DWORD dwFlags = PROCESS_QUERY_INFORMATION|SYNCHRONIZE;

		if (gpSrv->bDebuggerActive)
			dwFlags |= PROCESS_VM_READ;

		CAdjustProcessToken token;
		token.Enable(1, SE_DEBUG_NAME);

		// PROCESS_ALL_ACCESS may fails on WinXP!
		gpSrv->hRootProcess = OpenProcess((STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|0xFFF), FALSE, gpSrv->dwRootProcess);
		if (!gpSrv->hRootProcess)
			gpSrv->hRootProcess = OpenProcess(dwFlags, FALSE, gpSrv->dwRootProcess);

		token.Release();

		if (!gpSrv->hRootProcess)
		{
			dwErr = GetLastError();
			wchar_t* lpMsgBuf = NULL;
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&lpMsgBuf, 0, NULL);
			_printf("Can't open process (%i) handle, ErrCode=0x%08X, Description:\n", //-V576
			        gpSrv->dwRootProcess, dwErr, (lpMsgBuf == NULL) ? L"<Unknown error>" : lpMsgBuf);

			if (lpMsgBuf) LocalFree(lpMsgBuf);
			SetLastError(dwErr);

			return CERR_CREATEPROCESS;
		}

		if (gpSrv->bDebuggerActive)
		{
			wchar_t szTitle[64];
			_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"Debugging PID=%u, Debugger PID=%u", gpSrv->dwRootProcess, GetCurrentProcessId());
			SetConsoleTitleW(szTitle);
		}
	}

	return 0; // OK
}

BOOL ServerInitConsoleMode()
{
	BOOL bConRc = FALSE;

	HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
	DWORD dwFlags = 0;
	bConRc = GetConsoleMode(h, &dwFlags);

	if (!gnConsoleModeFlags)
	{
		// Умолчание (параметр /CINMODE= не указан)
		dwFlags |= (ENABLE_QUICK_EDIT_MODE|ENABLE_EXTENDED_FLAGS|ENABLE_INSERT_MODE);
	}
	else
	{
		DWORD nMask = (gnConsoleModeFlags & 0xFFFF0000) >> 16;
		DWORD nOr   = (gnConsoleModeFlags & 0xFFFF);
		dwFlags &= ~nMask;
		dwFlags |= (nOr | ENABLE_EXTENDED_FLAGS);
	}

	bConRc = SetConsoleMode(h, dwFlags); //-V519

	return bConRc;
}

int ServerInitCheckExisting(bool abAlternative)
{
	int iRc = 0;
	CESERVER_CONSOLE_MAPPING_HDR test = {};

	BOOL lbExist = LoadSrvMapping(ghConWnd, test);
	if (abAlternative == FALSE)
	{
		_ASSERTE(gnRunMode==RM_SERVER);
		// Основной сервер! Мэппинг консоли по идее создан еще быть не должен!
		// Это должно быть ошибка - попытка запуска второго сервера в той же консоли!
		if (lbExist)
		{
			CESERVER_REQ_HDR In; ExecutePrepareCmd(&In, CECMD_ALIVE, sizeof(CESERVER_REQ_HDR));
			CESERVER_REQ* pOut = ExecuteSrvCmd(test.nServerPID, (CESERVER_REQ*)&In, NULL);
			if (pOut)
			{
				_ASSERTE(test.nServerPID == 0);
				ExecuteFreeResult(pOut);
				wchar_t szErr[127];
				msprintf(szErr, countof(szErr), L"\nServer (PID=%u) already exist in console! Current PID=%u\n", test.nServerPID, GetCurrentProcessId());
				_wprintf(szErr);
				iRc = CERR_SERVER_ALREADY_EXISTS;
				goto wrap;
			}

			// Старый сервер умер, запустился новый? нужна какая-то дополнительная инициализация?
			_ASSERTE(test.nServerPID == 0 && "Server already exists");
		}
	}
	else
	{
		_ASSERTE(gnRunMode==RM_ALTSERVER);
		// По идее, в консоли должен быть _живой_ сервер.
		_ASSERTE(lbExist && test.nServerPID != 0);
		if (test.nServerPID == 0)
		{
			iRc = CERR_MAINSRV_NOT_FOUND;
			goto wrap;
		}
		else
		{
			gpSrv->dwMainServerPID = test.nServerPID;
			gpSrv->hMainServer = OpenProcess(SYNCHRONIZE|PROCESS_QUERY_INFORMATION, FALSE, test.nServerPID);
		}
	}

wrap:
	return iRc;
}

int ServerInitConsoleSize()
{
	if ((gbParmVisibleSize || gbParmBufSize) && gcrVisibleSize.X && gcrVisibleSize.Y)
	{
		SMALL_RECT rc = {0};
		SetConsoleSize(gnBufferHeight, gcrVisibleSize, rc, ":ServerInit.SetFromArg"); // может обломаться? если шрифт еще большой
	}
	else
	{
		HANDLE hOut = (HANDLE)ghConOut;
		CONSOLE_SCREEN_BUFFER_INFO lsbi = {{0,0}}; // интересует реальное положение дел

		if (GetConsoleScreenBufferInfo(hOut, &lsbi))
		{
			gpSrv->crReqSizeNewSize = lsbi.dwSize;
			_ASSERTE(gpSrv->crReqSizeNewSize.X!=0);
			gcrVisibleSize.X = lsbi.dwSize.X;

			if (lsbi.dwSize.Y > lsbi.dwMaximumWindowSize.Y)
			{
				// Буферный режим
				gcrVisibleSize.Y = (lsbi.srWindow.Bottom - lsbi.srWindow.Top + 1);
				gnBufferHeight = lsbi.dwSize.Y;
			}
			else
			{
				// Режим без прокрутки!
				gcrVisibleSize.Y = lsbi.dwSize.Y;
				gnBufferHeight = 0;
			}
		}
	}

	return 0;
}

int ServerInitAttach2Gui()
{
	int iRc = 0;

	// Нить Refresh НЕ должна быть запущена, иначе в мэппинг могут попасть данные из консоли
	// ДО того, как отработает ресайз (тот размер, который указал установить GUI при аттаче)
	_ASSERTE(gpSrv->dwRefreshThread==0);
	HWND hDcWnd = NULL;

	while(true)
	{
		hDcWnd = Attach2Gui(ATTACH2GUI_TIMEOUT);

		if (hDcWnd)
			break; // OK

		if (MessageBox(NULL, L"Available ConEmu GUI window not found!", L"ConEmu",
		              MB_RETRYCANCEL|MB_SYSTEMMODAL|MB_ICONQUESTION) != IDRETRY)
			break; // Отказ
	}

	// 090719 попробуем в сервере это делать всегда. Нужно передать в GUI - TID нити ввода
	//// Если это НЕ новая консоль (-new_console) и не /ATTACH уже существующей консоли
	//if (!gbNoCreateProcess)
	//	SendStarted();

	if (!hDcWnd)
	{
		//_printf("Available ConEmu GUI window not found!\n"); -- не будем гадить в консоль
		gbAlwaysConfirmExit = TRUE; gbInShutdown = TRUE;
		iRc = CERR_ATTACHFAILED; goto wrap;
	}

wrap:
	return iRc;
}

// Дернуть ConEmu, чтобы он отдал HWND окна отрисовки
// (!gbAttachMode && !gpSrv->bDebuggerActive)
int ServerInitGuiTab()
{
	int iRc = 0;
	DWORD dwRead = 0, dwErr = 0; BOOL lbCallRc = FALSE;
	HWND hConEmuWnd = FindConEmuByPID();

	if (hConEmuWnd == NULL)
	{
		if (gnRunMode == RM_SERVER || gnRunMode == RM_ALTSERVER)
		{
			// Если запускается сервер - то он должен смочь найти окно ConEmu в которое его просят
			_ASSERTEX((hConEmuWnd!=NULL));
			return CERR_ATTACH_NO_GUIWND;
		}
		else
		{
			_ASSERTEX(gnRunMode == RM_SERVER || gnRunMode == RM_ALTSERVER);
		}
	}
	else
	{
		//UINT nMsgSrvStarted = RegisterWindowMessage(CONEMUMSG_SRVSTARTED);
		//DWORD_PTR nRc = 0;
		//SendMessageTimeout(hConEmuWnd, nMsgSrvStarted, (WPARAM)ghConWnd, gnSelfPID,
		//	SMTO_BLOCK, 500, &nRc);
		_ASSERTE(ghConWnd!=NULL);
		wchar_t szServerPipe[MAX_PATH];
		_wsprintf(szServerPipe, SKIPLEN(countof(szServerPipe)) CEGUIPIPENAME, L".", (DWORD)hConEmuWnd); //-V205
		CESERVER_REQ In, Out;
		ExecutePrepareCmd(&In, CECMD_SRVSTARTSTOP, sizeof(CESERVER_REQ_HDR)+sizeof(DWORD)*2);
		In.dwData[0] = 1; // запущен сервер
		In.dwData[1] = (DWORD)ghConWnd; //-V205

		#ifdef _DEBUG
		DWORD nStartTick = timeGetTime();
		#endif

		lbCallRc = CallNamedPipe(szServerPipe, &In, In.hdr.cbSize, &Out, sizeof(Out), &dwRead, 1000);

		#ifdef _DEBUG
		DWORD dwErr = GetLastError(), nEndTick = timeGetTime(), nDelta = nEndTick - nStartTick;
		if (!lbCallRc || nDelta >= EXECUTE_CMD_WARN_TIMEOUT)
		{
			if (!IsDebuggerPresent())
			{
				//_ASSERTE(nDelta <= EXECUTE_CMD_WARN_TIMEOUT || (pIn->hdr.nCmd == CECMD_CMDSTARTSTOP && nDelta <= EXECUTE_CMD_WARN_TIMEOUT2));
				_ASSERTEX(nDelta <= EXECUTE_CMD_WARN_TIMEOUT);
			}
		}
		#endif


		if (!lbCallRc || !Out.StartStopRet.hWndDC)
		{
			dwErr = GetLastError();
			#ifdef _DEBUG
			_ASSERTEX(lbCallRc && Out.StartStopRet.hWndDC);
			SetLastError(dwErr);
			#endif
		}
		else
		{
			ghConEmuWnd = Out.StartStop.hWnd;
			ghConEmuWndDC = Out.StartStopRet.hWndDC;
			gpSrv->dwGuiPID = Out.StartStopRet.dwPID;
			#ifdef _DEBUG
			DWORD nGuiPID; GetWindowThreadProcessId(ghConEmuWnd, &nGuiPID);
			_ASSERTEX(Out.hdr.nSrcPID==nGuiPID);
			#endif
			gpSrv->bWasDetached = FALSE;
			UpdateConsoleMapHeader();
		}
	}

	DWORD nWaitRc = 99;
	if (gpSrv->hConEmuGuiAttached)
	{
		DEBUGTEST(DWORD t1 = timeGetTime());

		nWaitRc = WaitForSingleObject(gpSrv->hConEmuGuiAttached, 500);

		#ifdef _DEBUG
		DWORD t2 = timeGetTime(), tDur = t2-t1;
		if (tDur > GUIATTACHEVENT_TIMEOUT)
		{
			_ASSERTE(tDur <= GUIATTACHEVENT_TIMEOUT);
		}
		#endif
	}

	CheckConEmuHwnd();

	return iRc;
}

bool AltServerWasStarted(DWORD nPID, HANDLE hAltServer)
{
	// Перевести нить монитора в режим ожидания завершения AltServer, инициализировать gpSrv->dwAltServerPID, gpSrv->hAltServer
	if (gpSrv->hAltServer && (gpSrv->hAltServer != hAltServer))
	{
		gpSrv->dwAltServerPID = 0;
		SafeCloseHandle(gpSrv->hAltServer);
	}

	if (hAltServer == NULL)
	{
		hAltServer = OpenProcess(MY_PROCESS_ALL_ACCESS, FALSE, nPID);
		if (hAltServer == NULL)
			hAltServer = OpenProcess(SYNCHRONIZE|PROCESS_QUERY_INFORMATION, FALSE, nPID);
	}

	gpSrv->hAltServer = hAltServer;
	gpSrv->dwAltServerPID = nPID;

	return (hAltServer != NULL);
}

// Создать необходимые события и нити
int ServerInit(int anWorkMode/*0-Server,1-AltServer,2-Reserved*/)
{
	int iRc = 0;
	DWORD dwErr = 0;

	if (anWorkMode == 0)
	{
		gpSrv->dwMainServerPID = GetCurrentProcessId();
		gpSrv->hMainServer = GetCurrentProcess();
	}

	// Сразу попытаемся поставить окну консоли флаг "OnTop"
	SetWindowPos(ghConWnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
	gpSrv->osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&gpSrv->osv);

	// Смысла вроде не имеет, без ожидания "очистки" очереди винда "проглатывает мышиные события
	// Межпроцессный семафор не помогает, оставил пока только в качестве заглушки
	//InitializeConsoleInputSemaphore();

	if (gpSrv->osv.dwMajorVersion == 6 && gpSrv->osv.dwMinorVersion == 1)
		gpSrv->bReopenHandleAllowed = FALSE;
	else
		gpSrv->bReopenHandleAllowed = TRUE;

	if (anWorkMode == 0)
	{
		if (!gnConfirmExitParm)
		{
			gbAlwaysConfirmExit = TRUE; gbAutoDisableConfirmExit = TRUE;
		}
	}
	else
	{
		_ASSERTE(anWorkMode==1 && gnRunMode==RM_ALTSERVER);
		// По идее, включены быть не должны, но убедимся
		_ASSERTE(!gbAlwaysConfirmExit);
		gbAutoDisableConfirmExit = FALSE; gbAlwaysConfirmExit = FALSE;
	}

	_ASSERTE(gpcsStoredOutput==NULL && gpStoredOutput==NULL);
	if (!gpcsStoredOutput)
	{
		gpcsStoredOutput = new MSection;
	}

	// Шрифт в консоли нужно менять в самом начале, иначе могут быть проблемы с установкой размера консоли
	if ((anWorkMode == 0) && !gpSrv->bDebuggerActive && !gbNoCreateProcess)
		//&& (!gbNoCreateProcess || (gbAttachMode && gbNoCreateProcess && gpSrv->dwRootProcess))
		//)
	{
		ServerInitFont();
		// -- чтобы на некоторых системах не возникала проблема с позиционированием -> {0,0}
		// Issue 274: Окно реальной консоли позиционируется в неудобном месте
		SetWindowPos(ghConWnd, NULL, 0, 0, 0,0, SWP_NOSIZE|SWP_NOZORDER);
	}

	// Включить по умолчанию выделение мышью
	if ((anWorkMode == 0) && !gbNoCreateProcess && gbConsoleModeFlags /*&& !(gbParmBufferSize && gnBufferHeight == 0)*/)
	{
		ServerInitConsoleMode();
	}

	//2009-08-27 Перенес снизу
	if (!gpSrv->hConEmuGuiAttached)
	{
		wchar_t szTempName[MAX_PATH];
		_wsprintf(szTempName, SKIPLEN(countof(szTempName)) CEGUIRCONSTARTED, (DWORD)ghConWnd); //-V205
		//gpSrv->hConEmuGuiAttached = OpenEvent(EVENT_ALL_ACCESS, FALSE, szTempName);
		//if (gpSrv->hConEmuGuiAttached == NULL)
		gpSrv->hConEmuGuiAttached = CreateEvent(gpLocalSecurity, TRUE, FALSE, szTempName);
		_ASSERTE(gpSrv->hConEmuGuiAttached!=NULL);
		//if (gpSrv->hConEmuGuiAttached) ResetEvent(gpSrv->hConEmuGuiAttached); -- низя. может уже быть создано/установлено в GUI
	}

	// Было 10, чтобы не перенапрягать консоль при ее быстром обновлении ("dir /s" и т.п.)
	gpSrv->nMaxFPS = 100;

	#ifdef _DEBUG
	if (ghFarInExecuteEvent)
		SetEvent(ghFarInExecuteEvent);
	#endif

	_ASSERTE(anWorkMode!=2); // Для StandAloneGui - проверить

	if (ghConEmuWndDC == NULL)
	{
		// в AltServer режиме HWND уже должен быть известен
		_ASSERTE((anWorkMode==0) || ghConEmuWndDC!=NULL);
	}
	else
	{
		iRc = ServerInitCheckExisting((anWorkMode==1));
		if (iRc != 0)
			goto wrap;
	}

	// Создать MapFile для заголовка (СРАЗУ!!!) и буфера для чтения и сравнения
	iRc = CreateMapHeader();

	if (iRc != 0)
		goto wrap;

	_ASSERTE((ghConEmuWndDC==NULL) || (gpSrv->pColorerMapping!=NULL));

	gpSrv->csProc = new MSection();
	gpSrv->nMainTimerElapse = 10;
	gpSrv->nTopVisibleLine = -1; // блокировка прокрутки не включена
	// Инициализация имен пайпов
	_wsprintf(gpSrv->szPipename, SKIPLEN(countof(gpSrv->szPipename)) CESERVERPIPENAME, L".", gnSelfPID);
	_wsprintf(gpSrv->szInputname, SKIPLEN(countof(gpSrv->szInputname)) CESERVERINPUTNAME, L".", gnSelfPID);
	_wsprintf(gpSrv->szQueryname, SKIPLEN(countof(gpSrv->szQueryname)) CESERVERQUERYNAME, L".", gnSelfPID);
	_wsprintf(gpSrv->szGetDataPipe, SKIPLEN(countof(gpSrv->szGetDataPipe)) CESERVERREADNAME, L".", gnSelfPID);
	_wsprintf(gpSrv->szDataReadyEvent, SKIPLEN(countof(gpSrv->szDataReadyEvent)) CEDATAREADYEVENT, gnSelfPID);
	gpSrv->nMaxProcesses = START_MAX_PROCESSES; gpSrv->nProcessCount = 0;
	gpSrv->pnProcesses = (DWORD*)calloc(START_MAX_PROCESSES, sizeof(DWORD));
	gpSrv->pnProcessesGet = (DWORD*)calloc(START_MAX_PROCESSES, sizeof(DWORD));
	gpSrv->pnProcessesCopy = (DWORD*)calloc(START_MAX_PROCESSES, sizeof(DWORD));
	MCHKHEAP;

	if (gpSrv->pnProcesses == NULL || gpSrv->pnProcessesGet == NULL || gpSrv->pnProcessesCopy == NULL)
	{
		_printf("Can't allocate %i DWORDS!\n", gpSrv->nMaxProcesses);
		iRc = CERR_NOTENOUGHMEM1; goto wrap;
	}

	CheckProcessCount(TRUE); // Сначала добавит себя

	// в принципе, серверный режим может быть вызван из фара, чтобы подцепиться к GUI.
	// больше двух процессов в консоли вполне может быть, например, еще не отвалился
	// предыдущий conemuc.exe, из которого этот запущен немодально.
	_ASSERTE(gpSrv->bDebuggerActive || (gpSrv->nProcessCount<=2) || ((gpSrv->nProcessCount>2) && gbAttachMode && gpSrv->dwRootProcess));
	// Запустить нить обработки событий (клавиатура, мышь, и пр.)
	gpSrv->hInputEvent = CreateEvent(NULL,FALSE,FALSE,NULL);

	if (gpSrv->hInputEvent) gpSrv->hInputThread = CreateThread(
		        NULL,              // no security attribute
		        0,                 // default stack size
		        InputThread,       // thread proc
		        NULL,              // thread parameter
		        0,                 // not suspended
		        &gpSrv->dwInputThread);      // returns thread ID

	if (gpSrv->hInputEvent == NULL || gpSrv->hInputThread == NULL)
	{
		dwErr = GetLastError();
		_printf("CreateThread(InputThread) failed, ErrCode=0x%08X\n", dwErr);
		iRc = CERR_CREATEINPUTTHREAD; goto wrap;
	}

	SetThreadPriority(gpSrv->hInputThread, THREAD_PRIORITY_ABOVE_NORMAL);
	gpSrv->nMaxInputQueue = 255;
	gpSrv->pInputQueue = (INPUT_RECORD*)calloc(gpSrv->nMaxInputQueue, sizeof(INPUT_RECORD));
	gpSrv->pInputQueueEnd = gpSrv->pInputQueue+gpSrv->nMaxInputQueue;
	gpSrv->pInputQueueWrite = gpSrv->pInputQueue;
	gpSrv->pInputQueueRead = gpSrv->pInputQueueEnd;
	// Запустить пайп обработки событий (клавиатура, мышь, и пр.)
	if (!InputServerStart())
	{
		dwErr = GetLastError();
		_printf("CreateThread(InputServerStart) failed, ErrCode=0x%08X\n", dwErr);
		iRc = CERR_CREATEINPUTTHREAD; goto wrap;
	}

	// Пайп возврата содержимого консоли
	if (!DataServerStart())
	{
		dwErr = GetLastError();
		_printf("CreateThread(DataServerStart) failed, ErrCode=0x%08X\n", dwErr);
		iRc = CERR_CREATEINPUTTHREAD; goto wrap;
	}


	if (!gbAttachMode && !gpSrv->bDebuggerActive)
	{
		iRc = ServerInitGuiTab();
		if (iRc != 0)
			goto wrap;
	}

	// Если "корневой" процесс консоли запущен не нами (аттач или дебаг)
	// то нужно к нему "подцепиться" (открыть HANDLE процесса)
	if (gbNoCreateProcess && (gbAttachMode || gpSrv->bDebuggerActive))
	{
		iRc = AttachRootProcess();
		if (iRc != 0)
			goto wrap;
	}


	gpSrv->hAllowInputEvent = CreateEvent(NULL, TRUE, TRUE, NULL);

	if (!gpSrv->hAllowInputEvent) SetEvent(gpSrv->hAllowInputEvent);

	

	_ASSERTE(gpSrv->pConsole!=NULL);
	gpSrv->pConsole->hdr.bConsoleActive = TRUE;
	gpSrv->pConsole->hdr.bThawRefreshThread = TRUE;

	// Если указаны параметры (gbParmBufferSize && gcrVisibleSize.X && gcrVisibleSize.Y) - установить размер
	// Иначе - получить текущие размеры из консольного окна
	ServerInitConsoleSize();

	// Minimized окошко нужно развернуть!
	if (IsIconic(ghConWnd))
	{
		WINDOWPLACEMENT wplCon = {sizeof(wplCon)};
		GetWindowPlacement(ghConWnd, &wplCon);
		wplCon.showCmd = SW_RESTORE;
		SetWindowPlacement(ghConWnd, &wplCon);
	}

	// Сразу получить текущее состояние консоли
	ReloadFullConsoleInfo(TRUE);
	
	//
	gpSrv->hRefreshEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	if (!gpSrv->hRefreshEvent)
	{
		dwErr = GetLastError();
		_printf("CreateEvent(hRefreshEvent) failed, ErrCode=0x%08X\n", dwErr);
		iRc = CERR_REFRESHEVENT; goto wrap;
	}

	gpSrv->hRefreshDoneEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	if (!gpSrv->hRefreshDoneEvent)
	{
		dwErr = GetLastError();
		_printf("CreateEvent(hRefreshDoneEvent) failed, ErrCode=0x%08X\n", dwErr);
		iRc = CERR_REFRESHEVENT; goto wrap;
	}

	gpSrv->hDataReadyEvent = CreateEvent(gpLocalSecurity,FALSE,FALSE,gpSrv->szDataReadyEvent);
	if (!gpSrv->hDataReadyEvent)
	{
		dwErr = GetLastError();
		_printf("CreateEvent(hDataReadyEvent) failed, ErrCode=0x%08X\n", dwErr);
		iRc = CERR_REFRESHEVENT; goto wrap;
	}

	// !! Event может ожидаться в нескольких нитях !!
	gpSrv->hReqSizeChanged = CreateEvent(NULL,TRUE,FALSE,NULL);
	if (!gpSrv->hReqSizeChanged)
	{
		dwErr = GetLastError();
		_printf("CreateEvent(hReqSizeChanged) failed, ErrCode=0x%08X\n", dwErr);
		iRc = CERR_REFRESHEVENT; goto wrap;
	}

	if ((gnRunMode == RM_SERVER) && gbAttachMode)
	{
		iRc = ServerInitAttach2Gui();
		if (iRc != 0)
			goto wrap;
	}

	// Запустить нить наблюдения за консолью
	gpSrv->hRefreshThread = CreateThread(NULL, 0, RefreshThread, NULL, 0, &gpSrv->dwRefreshThread);
	if (gpSrv->hRefreshThread == NULL)
	{
		dwErr = GetLastError();
		_printf("CreateThread(RefreshThread) failed, ErrCode=0x%08X\n", dwErr);
		iRc = CERR_CREATEREFRESHTHREAD; goto wrap;
	}

	//#ifdef USE_WINEVENT_SRV
	////gpSrv->nMsgHookEnableDisable = RegisterWindowMessage(L"ConEmuC::HookEnableDisable");
	//// The client thread that calls SetWinEventHook must have a message loop in order to receive events.");
	//gpSrv->hWinEventThread = CreateThread(NULL, 0, WinEventThread, NULL, 0, &gpSrv->dwWinEventThread);
	//if (gpSrv->hWinEventThread == NULL)
	//{
	//	dwErr = GetLastError();
	//	_printf("CreateThread(WinEventThread) failed, ErrCode=0x%08X\n", dwErr);
	//	iRc = CERR_WINEVENTTHREAD; goto wrap;
	//}
	//#endif

	// Запустить пайп обработки команд
	if (!CmdServerStart())
	{
		dwErr = GetLastError();
		_printf("CreateThread(CmdServerStart) failed, ErrCode=0x%08X\n", dwErr);
		iRc = CERR_CREATESERVERTHREAD; goto wrap;
	}


	// Пометить мэппинг, как готовый к отдаче данных
	gpSrv->pConsole->hdr.bDataReady = TRUE;
	//gpSrv->pConsoleMap->SetFrom(&(gpSrv->pConsole->hdr));
	UpdateConsoleMapHeader();

	if (!gpSrv->bDebuggerActive)
		SendStarted();

	CheckConEmuHwnd();
	
	// Обновить переменные окружения и мэппинг консоли (по ConEmuGuiMapping)
	// т.к. в момент CreateMapHeader ghConEmu еще был неизвестен
	ReloadGuiSettings(NULL);

	// Если мы аттачим существующее GUI окошко
	if (gbNoCreateProcess && gbAttachMode && gpSrv->hRootProcessGui)
	{
		// Его нужно дернуть, чтобы инициализировать цикл аттача во вкладку ConEmu
		CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_ATTACHGUIAPP, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_ATTACHGUIAPP));
		_ASSERTE(((DWORD)gpSrv->hRootProcessGui)!=0xCCCCCCCC);
		_ASSERTE(IsWindow(ghConEmuWnd));
		_ASSERTE(IsWindow(ghConEmuWndDC));
		_ASSERTE(IsWindow(gpSrv->hRootProcessGui));
		pIn->AttachGuiApp.hConEmuWnd = ghConEmuWnd;
		pIn->AttachGuiApp.hConEmuWndDC = ghConEmuWndDC;
		pIn->AttachGuiApp.hAppWindow = gpSrv->hRootProcessGui;
		pIn->AttachGuiApp.hSrvConWnd = ghConWnd;
		wchar_t szPipe[MAX_PATH];
		_ASSERTE(gpSrv->dwRootProcess!=0);
		_wsprintf(szPipe, SKIPLEN(countof(szPipe)) CEHOOKSPIPENAME, L".", gpSrv->dwRootProcess);
		CESERVER_REQ* pOut = ExecuteCmd(szPipe, pIn, 500, ghConWnd);
		if (!pOut 
			|| (pOut->hdr.cbSize < (sizeof(CESERVER_REQ_HDR)+sizeof(DWORD)))
			|| (pOut->dwData[0] != (DWORD)gpSrv->hRootProcessGui))
		{
			iRc = CERR_ATTACH_NO_GUIWND;
		}
		ExecuteFreeResult(pOut);
		ExecuteFreeResult(pIn);
		if (iRc != 0)
			goto wrap;
	}

wrap:
	return iRc;
}

// Завершить все нити и закрыть дескрипторы
void ServerDone(int aiRc, bool abReportShutdown /*= false*/)
{
	gbQuit = true;
	gbTerminateOnExit = FALSE;

#ifdef _DEBUG
	// Проверить, не вылезло ли Assert-ов в других потоках
	MyAssertShutdown();
#endif

	// На всякий случай - выставим событие
	if (ghExitQueryEvent)
	{
		_ASSERTE(gbTerminateOnCtrlBreak==FALSE);
		if (!nExitQueryPlace) nExitQueryPlace = 10+(nExitPlaceStep+nExitPlaceThread);

		SetEvent(ghExitQueryEvent);
	}

	if (ghQuitEvent) SetEvent(ghQuitEvent);

	if (ghConEmuWnd && IsWindow(ghConEmuWnd))
	{
		if (gpSrv->pConsole && gpSrv->pConsoleMap)
		{
			gpSrv->pConsole->hdr.nServerInShutdown = GetTickCount();
			//gpSrv->pConsoleMap->SetFrom(&(gpSrv->pConsole->hdr));
			UpdateConsoleMapHeader();
		}

#ifdef _DEBUG
		int nCurProcCount = gpSrv->nProcessCount;
		DWORD nCurProcs[20];
		memmove(nCurProcs, gpSrv->pnProcesses, min(nCurProcCount,20)*sizeof(DWORD));
		_ASSERTE(nCurProcCount <= 1);
#endif
		wchar_t szServerPipe[MAX_PATH];
		_wsprintf(szServerPipe, SKIPLEN(countof(szServerPipe)) CEGUIPIPENAME, L".", (DWORD)ghConEmuWnd); //-V205
		CESERVER_REQ In, Out; DWORD dwRead = 0;
		ExecutePrepareCmd(&In, CECMD_SRVSTARTSTOP, sizeof(CESERVER_REQ_HDR)+sizeof(DWORD)*2);
		In.dwData[0] = 101;
		In.dwData[1] = (DWORD)ghConWnd; //-V205
		// Послать в GUI уведомление, что сервер закрывается
		CallNamedPipe(szServerPipe, &In, In.hdr.cbSize, &Out, sizeof(Out), &dwRead, 1000);
	}

	// Остановить отладчик, иначе отлаживаемый процесс тоже схлопнется
	if (gpSrv->bDebuggerActive)
	{
		if (pfnDebugActiveProcessStop) pfnDebugActiveProcessStop(gpSrv->dwRootProcess);

		gpSrv->bDebuggerActive = FALSE;
	}

	//// Пошлем события сразу во все нити, а потом будем ждать
	//#ifdef USE_WINEVENT_SRV
	//if (gpSrv->dwWinEventThread && gpSrv->hWinEventThread)
	//	PostThreadMessage(gpSrv->dwWinEventThread, WM_QUIT, 0, 0);
	//#endif

	//if (gpSrv->dwInputThreadId && gpSrv->hInputThread)
	//	PostThreadMessage(gpSrv->dwInputThreadId, WM_QUIT, 0, 0);
	// Передернуть пайп серверной нити
	//HANDLE hPipe = CreateFile(gpSrv->szPipename,GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);

	//if (hPipe == INVALID_HANDLE_VALUE)
	//{
	//	DEBUGSTR(L"All pipe instances closed?\n");
	//}

	//// Передернуть нить ввода
	//if (gpSrv->hInputPipe && gpSrv->hInputPipe != INVALID_HANDLE_VALUE)
	//{
	//	//DWORD dwSize = 0;
	//	//BOOL lbRc = WriteFile(gpSrv->hInputPipe, &dwSize, sizeof(dwSize), &dwSize, NULL);
	//	/*BOOL lbRc = DisconnectNamedPipe(gpSrv->hInputPipe);
	//	CloseHandle(gpSrv->hInputPipe);
	//	gpSrv->hInputPipe = NULL;*/
	//	TODO("Нифига не работает, похоже нужно на Overlapped переходить");
	//}

	//HANDLE hInputPipe = CreateFile(gpSrv->szInputname,GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
	//if (hInputPipe == INVALID_HANDLE_VALUE) {
	//	DEBUGSTR(L"Input pipe was not created?\n");
	//} else {
	//	MSG msg = {NULL}; msg.message = 0xFFFF; DWORD dwOut = 0;
	//	WriteFile(hInputPipe, &msg, sizeof(msg), &dwOut, 0);
	//}

	//#ifdef USE_WINEVENT_SRV
	//// Закрываем дескрипторы и выходим
	//if (/*gpSrv->dwWinEventThread &&*/ gpSrv->hWinEventThread)
	//{
	//	// Подождем немножко, пока нить сама завершится
	//	if (WaitForSingleObject(gpSrv->hWinEventThread, 500) != WAIT_OBJECT_0)
	//	{
	//		gbTerminateOnExit = gpSrv->bWinEventTermination = TRUE;
	//		#ifdef _DEBUG
	//			// Проверить, не вылезло ли Assert-ов в других потоках
	//			MyAssertShutdown();
	//		#endif

	//		#ifndef __GNUC__
	//		#pragma warning( push )
	//		#pragma warning( disable : 6258 )
	//		#endif
	//		TerminateThread(gpSrv->hWinEventThread, 100);    // раз корректно не хочет...
	//		#ifndef __GNUC__
	//		#pragma warning( pop )
	//		#endif
	//	}

	//	SafeCloseHandle(gpSrv->hWinEventThread);
	//	//gpSrv->dwWinEventThread = 0; -- не будем чистить ИД, Для истории
	//}
	//#endif

	if (gpSrv->hInputThread)
	{
		// Подождем немножко, пока нить сама завершится
		WARNING("Не завершается");

		if (WaitForSingleObject(gpSrv->hInputThread, 500) != WAIT_OBJECT_0)
		{
			gbTerminateOnExit = gpSrv->bInputTermination = TRUE;
			#ifdef _DEBUG
				// Проверить, не вылезло ли Assert-ов в других потоках
				MyAssertShutdown();
			#endif

			#ifndef __GNUC__
			#pragma warning( push )
			#pragma warning( disable : 6258 )
			#endif
			TerminateThread(gpSrv->hInputThread, 100);    // раз корректно не хочет...
			#ifndef __GNUC__
			#pragma warning( pop )
			#endif
		}

		SafeCloseHandle(gpSrv->hInputThread);
		//gpSrv->dwInputThread = 0; -- не будем чистить ИД, Для истории
	}

	// Пайп консольного ввода
	if (gpSrv)
		gpSrv->InputServer.StopPipeServer();
	//if (gpSrv->hInputPipeThread)
	//{
	//	// Подождем немножко, пока нить сама завершится
	//	if (WaitForSingleObject(gpSrv->hInputPipeThread, 50) != WAIT_OBJECT_0)
	//	{
	//		gbTerminateOnExit = gpSrv->bInputPipeTermination = TRUE;
	//		#ifdef _DEBUG
	//			// Проверить, не вылезло ли Assert-ов в других потоках
	//			MyAssertShutdown();
	//		#endif
	//#ifndef __GNUC__
	//#pragma warning( push )
	//#pragma warning( disable : 6258 )
	//#endif
	//		TerminateThread(gpSrv->hInputPipeThread, 100);    // раз корректно не хочет...
	//#ifndef __GNUC__
	//#pragma warning( pop )
	//#endif
	//	}
	//
	//	SafeCloseHandle(gpSrv->hInputPipeThread);
	//	//gpSrv->dwInputPipeThreadId = 0; -- не будем чистить ИД, Для истории
	//}

	//SafeCloseHandle(gpSrv->hInputPipe);
	SafeCloseHandle(gpSrv->hInputEvent);

	if (gpSrv)
		gpSrv->DataServer.StopPipeServer();
	//if (gpSrv->hGetDataPipeThread)
	//{
	//	// Подождем немножко, пока нить сама завершится
	//	TODO("Сама нить не завершится. Нужно либо делать overlapped, либо что-то пихать в нить");
	//	//if (WaitForSingleObject(gpSrv->hGetDataPipeThread, 50) != WAIT_OBJECT_0)
	//	{
	//		gbTerminateOnExit = gpSrv->bGetDataPipeTermination = TRUE;
	//		#ifdef _DEBUG
	//			// Проверить, не вылезло ли Assert-ов в других потоках
	//			MyAssertShutdown();
	//		#endif
	//#ifndef __GNUC__
	//#pragma warning( push )
	//#pragma warning( disable : 6258 )
	//#endif
	//		TerminateThread(gpSrv->hGetDataPipeThread, 100);    // раз корректно не хочет...
	//#ifndef __GNUC__
	//#pragma warning( pop )
	//#endif
	//	}
	//	SafeCloseHandle(gpSrv->hGetDataPipeThread);
	//	gpSrv->dwGetDataPipeThreadId = 0;
	//}

	if (gpSrv)
		gpSrv->CmdServer.StopPipeServer();
	//if (gpSrv->hServerThread)
	//{
	//	// Подождем немножко, пока нить сама завершится
	//	if (WaitForSingleObject(gpSrv->hServerThread, 500) != WAIT_OBJECT_0)
	//	{
	//		gbTerminateOnExit = gpSrv->bServerTermination = TRUE;
	//		#ifdef _DEBUG
	//			// Проверить, не вылезло ли Assert-ов в других потоках
	//			MyAssertShutdown();
	//		#endif
	//#ifndef __GNUC__
	//#pragma warning( push )
	//#pragma warning( disable : 6258 )
	//#endif
	//		TerminateThread(gpSrv->hServerThread, 100);    // раз корректно не хочет...
	//#ifndef __GNUC__
	//#pragma warning( pop )
	//#endif
	//	}
	//	SafeCloseHandle(gpSrv->hServerThread);
	//}

	//SafeCloseHandle(hPipe);

	if (gpSrv->hRefreshThread)
	{
		if (WaitForSingleObject(gpSrv->hRefreshThread, 250)!=WAIT_OBJECT_0)
		{
			_ASSERT(FALSE);
			gbTerminateOnExit = gpSrv->bRefreshTermination = TRUE;
			#ifdef _DEBUG
				// Проверить, не вылезло ли Assert-ов в других потоках
				MyAssertShutdown();
			#endif

			#ifndef __GNUC__
			#pragma warning( push )
			#pragma warning( disable : 6258 )
			#endif
			TerminateThread(gpSrv->hRefreshThread, 100);
			#ifndef __GNUC__
			#pragma warning( pop )
			#endif
		}

		SafeCloseHandle(gpSrv->hRefreshThread);
	}

	if (gpSrv->hRefreshEvent)
	{
		SafeCloseHandle(gpSrv->hRefreshEvent);
	}

	if (gpSrv->hRefreshDoneEvent)
	{
		SafeCloseHandle(gpSrv->hRefreshDoneEvent);
	}

	if (gpSrv->hDataReadyEvent)
	{
		SafeCloseHandle(gpSrv->hDataReadyEvent);
	}

	//if (gpSrv->hChangingSize) {
	//    SafeCloseHandle(gpSrv->hChangingSize);
	//}
	// Отключить все хуки
	//gpSrv->bWinHookAllow = FALSE; gpSrv->nWinHookMode = 0;
	//HookWinEvents ( -1 );

	{
		MSectionLock CS; CS.Lock(gpcsStoredOutput, TRUE);
		SafeFree(gpStoredOutput);
		CS.Unlock();
		SafeDelete(gpcsStoredOutput);
	}

	SafeFree(gpSrv->pszAliases);

	//if (gpSrv->psChars) { free(gpSrv->psChars); gpSrv->psChars = NULL; }
	//if (gpSrv->pnAttrs) { free(gpSrv->pnAttrs); gpSrv->pnAttrs = NULL; }
	//if (gpSrv->ptrLineCmp) { free(gpSrv->ptrLineCmp); gpSrv->ptrLineCmp = NULL; }
	//DeleteCriticalSection(&gpSrv->csConBuf);
	//DeleteCriticalSection(&gpSrv->csChar);
	//DeleteCriticalSection(&gpSrv->csChangeSize);
	SafeCloseHandle(gpSrv->hAllowInputEvent);
	SafeCloseHandle(gpSrv->hRootProcess);
	SafeCloseHandle(gpSrv->hRootThread);

	if (gpSrv->csProc)
	{
		//WARNING("БЛОКИРОВКА! Иногда зависает при закрытии консоли");
		//Перекрыл new & delete, посмотрим
		delete gpSrv->csProc;
		gpSrv->csProc = NULL;
	}

	if (gpSrv->pnProcesses)
	{
		free(gpSrv->pnProcesses); gpSrv->pnProcesses = NULL;
	}

	if (gpSrv->pnProcessesGet)
	{
		free(gpSrv->pnProcessesGet); gpSrv->pnProcessesGet = NULL;
	}

	if (gpSrv->pnProcessesCopy)
	{
		free(gpSrv->pnProcessesCopy); gpSrv->pnProcessesCopy = NULL;
	}

	if (gpSrv->pInputQueue)
	{
		free(gpSrv->pInputQueue); gpSrv->pInputQueue = NULL;
	}

	CloseMapHeader();

	//SafeCloseHandle(gpSrv->hColorerMapping);
	if (gpSrv->pColorerMapping)
	{
		delete gpSrv->pColorerMapping;
		gpSrv->pColorerMapping = NULL;
	}
}



// Сохранить данные ВСЕЙ консоли в gpStoredOutput
void CmdOutputStore()
{
	// В Win7 закрытие дескриптора в ДРУГОМ процессе - закрывает консольный буфер ПОЛНОСТЬЮ!!!
	// В итоге, буфер вывода telnet'а схлопывается!
	if (gpSrv->bReopenHandleAllowed)
	{
		ghConOut.Close();
	}

	WARNING("А вот это нужно бы делать в RefreshThread!!!");
	DEBUGSTR(L"--- CmdOutputStore begin\n");
	CONSOLE_SCREEN_BUFFER_INFO lsbi = {{0,0}};

	MSectionLock CS; CS.Lock(gpcsStoredOutput, FALSE);

	// !!! Нас интересует реальное положение дел в консоли,
	//     а не скорректированное функцией MyGetConsoleScreenBufferInfo
	if (!GetConsoleScreenBufferInfo(ghConOut, &lsbi))
	{
		CS.RelockExclusive();
		SafeFree(gpStoredOutput);

		return; // Не смогли получить информацию о консоли...
	}

	int nOneBufferSize = lsbi.dwSize.X * lsbi.dwSize.Y * 2; // Читаем всю консоль целиком!

	// Если требуется увеличение размера выделенной памяти
	if (gpStoredOutput)
	{
		if (gpStoredOutput->hdr.cbMaxOneBufferSize < (DWORD)nOneBufferSize)
		{
			CS.RelockExclusive();
			SafeFree(gpStoredOutput);
		}
	}

	if (gpStoredOutput == NULL)
	{
		CS.RelockExclusive();
		// Выделяем память: заголовок + буфер текста (на атрибуты забьем)
		gpStoredOutput = (CESERVER_CONSAVE*)calloc(sizeof(CESERVER_CONSAVE_HDR)+nOneBufferSize,1);
		_ASSERTE(gpStoredOutput!=NULL);

		if (gpStoredOutput == NULL)
			return; // Не смогли выделить память

		gpStoredOutput->hdr.cbMaxOneBufferSize = nOneBufferSize;
	}

	// Запомнить sbi
	//memmove(&gpStoredOutput->hdr.sbi, &lsbi, sizeof(lsbi));
	gpStoredOutput->hdr.sbi = lsbi;
	// Теперь читаем данные
	COORD coord = {0,0};
	DWORD nbActuallyRead = 0;
	DWORD nReadLen = lsbi.dwSize.X * lsbi.dwSize.Y;

	// [Roman Kuzmin]
	// In FAR Manager source code this is mentioned as "fucked method". Yes, it is.
	// Functions ReadConsoleOutput* fail if requested data size exceeds their buffer;
	// MSDN says 64K is max but it does not say how much actually we can request now.
	// Experiments show that this limit is floating and it can be much less than 64K.
	// The solution below is not optimal when a user sets small font and large window,
	// but it is safe and practically optimal, because most of users set larger fonts
	// for large window and ReadConsoleOutput works OK. More optimal solution for all
	// cases is not that difficult to develop but it will be increased complexity and
	// overhead often for nothing, not sure that we really should use it.

	if (!ReadConsoleOutputCharacter(ghConOut, gpStoredOutput->Data, nReadLen, coord, &nbActuallyRead)
	        || (nbActuallyRead != nReadLen))
	{
		DEBUGSTR(L"--- Full block read failed: read line by line\n");
		wchar_t* ConCharNow = gpStoredOutput->Data;
		nReadLen = lsbi.dwSize.X;

		for(int y = 0; y < (int)lsbi.dwSize.Y; y++, coord.Y++)
		{
			ReadConsoleOutputCharacter(ghConOut, ConCharNow, nReadLen, coord, &nbActuallyRead);
			ConCharNow += lsbi.dwSize.X;
		}
	}

	DEBUGSTR(L"--- CmdOutputStore end\n");
}

void CmdOutputRestore()
{
	MSectionLock CS; CS.Lock(gpcsStoredOutput, TRUE);
	if (gpStoredOutput)
	{
		TODO("Восстановить текст скрытой (прокрученной вверх) части консоли");
		// Учесть, что ширина консоли могла измениться со времени выполнения предыдущей команды.
		// Сейчас у нас в верхней части консоли может оставаться кусочек предыдущего вывода (восстановил FAR).
		// 1) Этот кусочек нужно считать
		// 2) Скопировать в нижнюю часть консоли (до которой докрутилась предыдущая команда)
		// 3) прокрутить консоль до предыдущей команды (куда мы только что скопировали данные сверху)
		// 4) восстановить оставшуюся часть консоли. Учесть, что фар может
		//    выполнять некоторые команды сам и курсор вообще-то мог несколько уехать...
	}
}

static BOOL CALLBACK FindConEmuByPidProc(HWND hwnd, LPARAM lParam)
{
	DWORD dwPID;
	GetWindowThreadProcessId(hwnd, &dwPID);
	if (dwPID == gpSrv->dwGuiPID)
	{
		wchar_t szClass[128];
		if (GetClassName(hwnd, szClass, countof(szClass)))
		{
			if (lstrcmp(szClass, VirtualConsoleClassMain) == 0)
			{
				*(HWND*)lParam = hwnd;
				return FALSE;
			}
		}
	}
	return TRUE;
}

HWND FindConEmuByPID()
{
	HWND hConEmuWnd = NULL;
	DWORD dwGuiThreadId = 0, dwGuiProcessId = 0;

	// В большинстве случаев PID GUI передан через параметры
	if (gpSrv->dwGuiPID == 0)
	{
		// GUI может еще "висеть" в ожидании или в отладчике, так что пробуем и через Snapshoot
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);

		if (hSnap != INVALID_HANDLE_VALUE)
		{
			PROCESSENTRY32 prc = {sizeof(PROCESSENTRY32)};

			if (Process32First(hSnap, &prc))
			{
				do
				{
					if (prc.th32ProcessID == gnSelfPID)
					{
						gpSrv->dwGuiPID = prc.th32ParentProcessID;
						break;
					}
				}
				while(Process32Next(hSnap, &prc));
			}

			CloseHandle(hSnap);
		}
	}

	if (gpSrv->dwGuiPID)
	{
		HWND hGui = NULL;

		while ((hGui = FindWindowEx(NULL, hGui, VirtualConsoleClassMain, NULL)) != NULL)
		{
			dwGuiThreadId = GetWindowThreadProcessId(hGui, &dwGuiProcessId);

			if (dwGuiProcessId == gpSrv->dwGuiPID)
			{
				hConEmuWnd = hGui;
				break;
			}
		}

		// Если "в лоб" по имени класса ничего не нашли - смотрим
		// среди всех дочерних для текущего десктопа
		if (hConEmuWnd == NULL)
		{
			HWND hDesktop = GetDesktopWindow();
			EnumChildWindows(hDesktop, FindConEmuByPidProc, (LPARAM)&hConEmuWnd);
		}
	}

	return hConEmuWnd;
}

void CheckConEmuHwnd()
{
	WARNING("Подозрение, что слишком много вызовов при старте сервера");

	//HWND hWndFore = GetForegroundWindow();
	//HWND hWndFocus = GetFocus();
	DWORD dwGuiThreadId = 0;

	if (gpSrv->bDebuggerActive)
	{
		ghConEmuWnd = FindConEmuByPID();

		if (ghConEmuWnd)
		{
			GetWindowThreadProcessId(ghConEmuWnd, &gpSrv->dwGuiPID);
			// Просто для информации
			ghConEmuWndDC = FindWindowEx(ghConEmuWnd, NULL, VirtualConsoleClassMain, NULL);
		}
		else
		{
			ghConEmuWndDC = NULL;
		}

		return;
	}

	if (ghConEmuWnd == NULL)
	{
		SendStarted(); // Он и окно проверит, и параметры перешлет и размер консоли скорректирует
	}

	// GUI может еще "висеть" в ожидании или в отладчике, так что пробуем и через Snapshoot
	if (ghConEmuWnd == NULL)
	{
		ghConEmuWnd = FindConEmuByPID();
	}

	if (ghConEmuWnd == NULL)
	{
		// Если уж ничего не помогло...
		ghConEmuWnd = GetConEmuHWND(TRUE/*abRoot*/);
	}

	if (ghConEmuWnd)
	{
		if (!ghConEmuWndDC)
		{
			// ghConEmuWndDC по идее уже должен быть получен из GUI через пайпы
			_ASSERTE(ghConEmuWndDC!=NULL);
			ghConEmuWndDC = FindWindowEx(ghConEmuWnd, NULL, VirtualConsoleClassMain, NULL);
		}

		// Установить переменную среды с дескриптором окна
		SetConEmuEnvVar(ghConEmuWnd);
		dwGuiThreadId = GetWindowThreadProcessId(ghConEmuWnd, &gpSrv->dwGuiPID);
		AllowSetForegroundWindow(gpSrv->dwGuiPID);

		//if (hWndFore == ghConWnd || hWndFocus == ghConWnd)
		//if (hWndFore != ghConEmuWnd)

		if (GetForegroundWindow() == ghConWnd)
			apiSetForegroundWindow(ghConEmuWnd); // 2009-09-14 почему-то было было ghConWnd ?
	}
	else
	{
		// да и фиг сним. нас могли просто так, без gui запустить
		//_ASSERTE(ghConEmuWnd!=NULL);
	}
}

HWND Attach2Gui(DWORD nTimeout)
{
	// Нить Refresh НЕ должна быть запущена, иначе в мэппинг могут попасть данные из консоли
	// ДО того, как отработает ресайз (тот размер, который указал установить GUI при аттаче)
	_ASSERTE(gpSrv->dwRefreshThread==0 || gpSrv->bWasDetached);
	HWND hGui = NULL, hDcWnd = NULL;
	//UINT nMsg = RegisterWindowMessage(CONEMUMSG_ATTACH);
	BOOL bNeedStartGui = FALSE;
	DWORD dwErr = 0;
	DWORD dwStartWaitIdleResult = -1;
	// Будем подцепляться заново
	gpSrv->bWasDetached = FALSE;

	if (!gpSrv->pConsoleMap)
	{
		_ASSERTE(gpSrv->pConsoleMap!=NULL);
	}
	else
	{
		// Чтобы GUI не пытался считать информацию из консоли до завершения аттача (до изменения размеров)
		gpSrv->pConsoleMap->Ptr()->bDataReady = FALSE;
	}

	hGui = FindWindowEx(NULL, hGui, VirtualConsoleClassMain, NULL);

	if (!hGui)
	{
		DWORD dwGuiPID = 0;
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);

		if (hSnap != INVALID_HANDLE_VALUE)
		{
			PROCESSENTRY32 prc = {sizeof(PROCESSENTRY32)};

			if (Process32First(hSnap, &prc))
			{
				do
				{
					for(UINT i = 0; i < gpSrv->nProcessCount; i++)
					{
						if (lstrcmpiW(prc.szExeFile, L"conemu.exe")==0)
						{
							dwGuiPID = prc.th32ProcessID;
							break;
						}
					}

					if (dwGuiPID) break;
				}
				while(Process32Next(hSnap, &prc));
			}

			CloseHandle(hSnap);

			if (!dwGuiPID) bNeedStartGui = TRUE;
		}
	}

	if (bNeedStartGui)
	{
		wchar_t szSelf[MAX_PATH+100];
		wchar_t* pszSelf = szSelf+1, *pszSlash = NULL;

		if (!GetModuleFileName(NULL, pszSelf, MAX_PATH))
		{
			dwErr = GetLastError();
			_printf("GetModuleFileName failed, ErrCode=0x%08X\n", dwErr);
			return NULL;
		}

		pszSlash = wcsrchr(pszSelf, L'\\');

		if (!pszSlash)
		{
			_printf("Invalid GetModuleFileName, backslash not found!\n", 0, pszSelf); //-V576
			return NULL;
		}

		lstrcpyW(pszSlash+1, L"ConEmu.exe");

		if (!FileExists(pszSelf))
		{
			// Он может быть на уровень выше
			*pszSlash = 0;
			pszSlash = wcsrchr(pszSelf, L'\\');
			lstrcpyW(pszSlash+1, L"ConEmu.exe");

			if (!FileExists(pszSelf))
			{
				_printf("ConEmu.exe not found!\n");
				return NULL;
			}
		}

		if (wcschr(pszSelf, L' '))
		{
			*(--pszSelf) = L'"';
			//lstrcpyW(pszSlash, L"ConEmu.exe\"");
			lstrcatW(pszSlash, L"\"");
		}

		//else
		//{
		//	lstrcpyW(pszSlash, L"ConEmu.exe");
		//}
		lstrcatW(pszSelf, L" /detached");
		PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
		STARTUPINFOW si; memset(&si, 0, sizeof(si)); si.cb = sizeof(si);
		PRINT_COMSPEC(L"Starting GUI:\n%s\n", pszSelf);
		// CREATE_NEW_PROCESS_GROUP - низя, перестает работать Ctrl-C
		// Запуск GUI (conemu.exe), хуки ест-но не нужны
		BOOL lbRc = createProcess(TRUE, NULL, pszSelf, NULL,NULL, TRUE,
		                           NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
		dwErr = GetLastError();

		if (!lbRc)
		{
			PrintExecuteError(pszSelf, dwErr);
			return NULL;
		}

		//delete psNewCmd; psNewCmd = NULL;
		AllowSetForegroundWindow(pi.dwProcessId);
		PRINT_COMSPEC(L"Detached GUI was started. PID=%i, Attaching...\n", pi.dwProcessId);
		dwStartWaitIdleResult = WaitForInputIdle(pi.hProcess, INFINITE); // был nTimeout, видимо часто обламывался
		SafeCloseHandle(pi.hProcess); SafeCloseHandle(pi.hThread);
		//if (nTimeout > 1000) nTimeout = 1000;
	}

	DWORD dwStart = GetTickCount(), dwDelta = 0, dwCur = 0;
	CESERVER_REQ *pIn = NULL;
	_ASSERTE(sizeof(CESERVER_REQ_STARTSTOP) >= sizeof(CESERVER_REQ_STARTSTOPRET));
	DWORD nInSize = sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_STARTSTOP)+(gpszRunCmd ? lstrlen(gpszRunCmd) : 0)*sizeof(wchar_t);
	pIn = ExecuteNewCmd(CECMD_ATTACH2GUI, nInSize);
	pIn->StartStop.nStarted = sst_ServerStart;
	pIn->StartStop.hWnd = ghConWnd;
	pIn->StartStop.dwPID = gnSelfPID;
	//pIn->StartStop.dwInputTID = gpSrv->dwInputPipeThreadId;
	pIn->StartStop.nSubSystem = gnImageSubsystem;

	if (gbAttachFromFar)
		pIn->StartStop.bRootIsCmdExe = FALSE;
	else
		pIn->StartStop.bRootIsCmdExe = gbRootIsCmdExe || (gbAttachMode && !gbNoCreateProcess);
	
	pIn->StartStop.bRunInBackgroundTab = gbRunInBackgroundTab;

	if (gpszRunCmd && *gpszRunCmd)
	{
		BOOL lbNeedCutQuot = FALSE, lbRootIsCmd = FALSE, lbConfirmExit = FALSE, lbAutoDisable = FALSE;
		IsNeedCmd(gpszRunCmd, &lbNeedCutQuot, pIn->StartStop.sModuleName, lbRootIsCmd, lbConfirmExit, lbAutoDisable);
		lstrcpy(pIn->StartStop.sCmdLine, gpszRunCmd);
	}

	// Если GUI запущен не от имени админа - то он обломается при попытке
	// открыть дескриптор процесса сервера. Нужно будет ему помочь.
	pIn->StartStop.bUserIsAdmin = IsUserAdmin();
	HANDLE hOut = (HANDLE)ghConOut;

	if (!GetConsoleScreenBufferInfo(hOut, &pIn->StartStop.sbi))
	{
		_ASSERTE(FALSE);
	}
	else
	{
		gpSrv->crReqSizeNewSize = pIn->StartStop.sbi.dwSize;
		_ASSERTE(gpSrv->crReqSizeNewSize.X!=0);
	}

	pIn->StartStop.crMaxSize = GetLargestConsoleWindowSize(hOut);

//LoopFind:
	// В обычном "серверном" режиме шрифт уже установлен, а при аттаче
	// другого процесса - шрифт все-равно поменять не получится
	//BOOL lbNeedSetFont = TRUE;
	// Нужно сбросить. Могли уже искать...
	hGui = NULL;

	// Если с первого раза не получится (GUI мог еще не загрузиться) пробуем еще
	while(!hDcWnd && dwDelta <= nTimeout)
	{
		while((hGui = FindWindowEx(NULL, hGui, VirtualConsoleClassMain, NULL)) != NULL)
		{
			//if (lbNeedSetFont) {
			//	lbNeedSetFont = FALSE;
			//
			//    if (ghLogSize) LogSize(NULL, ":SetConsoleFontSizeTo.before");
			//    SetConsoleFontSizeTo(ghConWnd, gpSrv->nConFontHeight, gpSrv->nConFontWidth, gpSrv->szConsoleFont);
			//    if (ghLogSize) LogSize(NULL, ":SetConsoleFontSizeTo.after");
			//}
			// Если GUI запущен не от имени админа - то он обломается при попытке
			// открыть дескриптор процесса сервера. Нужно будет ему помочь.
			pIn->StartStop.hServerProcessHandle = NULL;

			if (pIn->StartStop.bUserIsAdmin)
			{
				DWORD  nGuiPid = 0;

				if (GetWindowThreadProcessId(hGui, &nGuiPid) && nGuiPid)
				{
					HANDLE hGuiHandle = OpenProcess(PROCESS_DUP_HANDLE, FALSE, nGuiPid);

					if (hGuiHandle)
					{
						HANDLE hDupHandle = NULL;

						if (DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(),
						                   hGuiHandle, &hDupHandle, PROCESS_QUERY_INFORMATION|SYNCHRONIZE,
						                   FALSE, 0)
						        && hDupHandle)
						{
							pIn->StartStop.hServerProcessHandle = (u64)hDupHandle;
						}

						CloseHandle(hGuiHandle);
					}
				}
			}

			wchar_t szPipe[64];
			_wsprintf(szPipe, SKIPLEN(countof(szPipe)) CEGUIPIPENAME, L".", (DWORD)hGui); //-V205
			CESERVER_REQ *pOut = ExecuteCmd(szPipe, pIn, GUIATTACH_TIMEOUT, ghConWnd);

			if (!pOut)
			{
				_ASSERTE(pOut!=NULL);
			}
			else
			{
				//ghConEmuWnd = hGui;
				ghConEmuWnd = pOut->StartStopRet.hWnd;
				ghConEmuWndDC = hDcWnd = pOut->StartStopRet.hWndDC;
				gpSrv->dwGuiPID = pOut->StartStopRet.dwPID;
				_ASSERTE(gpSrv->pConsoleMap != NULL); // мэппинг уже должен быть создан,
				_ASSERTE(gpSrv->pConsole != NULL); // и локальная копия тоже
				//gpSrv->pConsole->info.nGuiPID = pOut->StartStopRet.dwPID;
				CESERVER_CONSOLE_MAPPING_HDR *pMap = gpSrv->pConsoleMap->Ptr();
				if (pMap)
				{
					pMap->nGuiPID = pOut->StartStopRet.dwPID;
					pMap->hConEmuRoot = ghConEmuWnd;
					pMap->hConEmuWnd = ghConEmuWndDC;
					_ASSERTE(pMap->hConEmuRoot==NULL || pMap->nGuiPID!=0);
				}

				//DisableAutoConfirmExit();

				// В принципе, консоль может действительно запуститься видимой. В этом случае ее скрывать не нужно
				// Но скорее всего, консоль запущенная под Админом в Win7 будет отображена ошибочно
				// 110807 - Если gbAttachMode, тоже консоль нужно спрятать
				if (gbForceHideConWnd || gbAttachMode)
					apiShowWindow(ghConWnd, SW_HIDE);

				// Установить шрифт в консоли
				if (pOut->StartStopRet.Font.cbSize == sizeof(CESERVER_REQ_SETFONT))
				{
					lstrcpy(gpSrv->szConsoleFont, pOut->StartStopRet.Font.sFontName);
					gpSrv->nConFontHeight = pOut->StartStopRet.Font.inSizeY;
					gpSrv->nConFontWidth = pOut->StartStopRet.Font.inSizeX;
					ServerInitFont();
				}

				COORD crNewSize = {(SHORT)pOut->StartStopRet.nWidth, (SHORT)pOut->StartStopRet.nHeight};
				//SMALL_RECT rcWnd = {0,pIn->StartStop.sbi.srWindow.Top};
				SMALL_RECT rcWnd = {0};
				SetConsoleSize((USHORT)pOut->StartStopRet.nBufferHeight, crNewSize, rcWnd, "Attach2Gui:Ret");
				// Установить переменную среды с дескриптором окна
				SetConEmuEnvVar(ghConEmuWnd);
				CheckConEmuHwnd();
				ExecuteFreeResult(pOut);
				break;
			}
		}

		if (hDcWnd) break;

		dwCur = GetTickCount(); dwDelta = dwCur - dwStart;

		if (dwDelta > nTimeout) break;

		Sleep(500);
		dwCur = GetTickCount(); dwDelta = dwCur - dwStart;
	}

	return hDcWnd;
}




/*
!! Во избежание растранжиривания памяти фиксированно создавать MAP только для шапки (Info), сами же данные
   загружать в дополнительный MAP, "ConEmuFileMapping.%08X.N", где N == nCurDataMapIdx
   Собственно это для того, чтобы при увеличении размера консоли можно было безболезненно увеличить объем данных
   и легко переоткрыть память во всех модулях

!! Для начала, в плагине после реальной записи в консоль просто дергать событие. Т.к. фар и сервер крутятся
   под одним аккаунтом - проблем с открытием события быть не должно.

1. ReadConsoleData должна сразу прикидывать, Размер данных больше 30K? Если больше - то и не пытаться читать блоком.
2. ReadConsoleInfo & ReadConsoleData должны возвращать TRUE при наличии изменений (последняя должна кэшировать последнее чтение для сравнения)
3. ReloadFullConsoleInfo пытаться звать ReadConsoleInfo & ReadConsoleData только ОДИН раз. Счетчик крутить только при наличии изменений. Но можно обновить Tick

4. В RefreshThread ждать события hRefresh 10мс (строго) или hExit.
-- Если есть запрос на изменение размера -
   ставить в TRUE локальную переменную bChangingSize
   менять размер и только после этого
-- звать ReloadFullConsoleInfo
-- После нее, если bChangingSize - установить Event hSizeChanged.
5. Убить все критические секции. Они похоже уже не нужны, т.к. все чтение консоли будет в RefreshThread.
6. Команда смена размера НЕ должна сама менять размер, а передавать запрос в RefreshThread и ждать события hSizeChanged
*/


int CreateMapHeader()
{
	int iRc = 0;
	//wchar_t szMapName[64];
	//int nConInfoSize = sizeof(CESERVER_CONSOLE_MAPPING_HDR);
	_ASSERTE(gpSrv->pConsole == NULL);
	_ASSERTE(gpSrv->pConsoleMap == NULL);
	_ASSERTE(gpSrv->pConsoleDataCopy == NULL);
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD crMax = GetLargestConsoleWindowSize(h);

	if (crMax.X < 80 || crMax.Y < 25)
	{
#ifdef _DEBUG
		DWORD dwErr = GetLastError();
		_ASSERTE(crMax.X >= 80 && crMax.Y >= 25);
#endif

		if (crMax.X < 80) crMax.X = 80;

		if (crMax.Y < 25) crMax.Y = 25;
	}

	// Размер шрифта может быть еще не уменьшен? Прикинем размер по максимуму?
	HMONITOR hMon = MonitorFromWindow(ghConWnd, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO mi = {sizeof(MONITORINFO)};

	if (GetMonitorInfo(hMon, &mi))
	{
		int x = (mi.rcWork.right - mi.rcWork.left) / 3;
		int y = (mi.rcWork.bottom - mi.rcWork.top) / 5;

		if (crMax.X < x || crMax.Y < y)
		{
			//_ASSERTE((crMax.X + 16) >= x && (crMax.Y + 32) >= y);
			if (crMax.X < x)
				crMax.X = x;

			if (crMax.Y < y)
				crMax.Y = y;
		}
	}

	//TODO("Добавить к nConDataSize размер необходимый для хранения crMax ячеек");
	int nTotalSize = 0;
	DWORD nMaxCells = (crMax.X * crMax.Y);
	//DWORD nHdrSize = ((LPBYTE)gpSrv->pConsoleDataCopy->Buf) - ((LPBYTE)gpSrv->pConsoleDataCopy);
	//_ASSERTE(sizeof(CESERVER_REQ_CONINFO_DATA) == (sizeof(COORD)+sizeof(CHAR_INFO)));
	int nMaxDataSize = nMaxCells * sizeof(CHAR_INFO); // + nHdrSize;
	gpSrv->pConsoleDataCopy = (CHAR_INFO*)calloc(nMaxDataSize,1);

	if (!gpSrv->pConsoleDataCopy)
	{
		_printf("ConEmuC: calloc(%i) failed, pConsoleDataCopy is null", nMaxDataSize);
		goto wrap;
	}

	//gpSrv->pConsoleDataCopy->crMaxSize = crMax;
	nTotalSize = sizeof(CESERVER_REQ_CONINFO_FULL) + (nMaxCells * sizeof(CHAR_INFO));
	gpSrv->pConsole = (CESERVER_REQ_CONINFO_FULL*)calloc(nTotalSize,1);

	if (!gpSrv->pConsole)
	{
		_printf("ConEmuC: calloc(%i) failed, pConsole is null", nTotalSize);
		goto wrap;
	}

	// !!! Warning !!! Изменил здесь, поменяй и ReloadGuiSettings() !!!
	gpSrv->pConsole->cbMaxSize = nTotalSize;
	//gpSrv->pConsole->cbActiveSize = ((LPBYTE)&(gpSrv->pConsole->data)) - ((LPBYTE)gpSrv->pConsole);
	//gpSrv->pConsole->bChanged = TRUE; // Initially == changed
	gpSrv->pConsole->hdr.cbSize = sizeof(gpSrv->pConsole->hdr);
	gpSrv->pConsole->hdr.nLogLevel = (ghLogSize!=NULL) ? 1 : 0;
	gpSrv->pConsole->hdr.crMaxConSize = crMax;
	gpSrv->pConsole->hdr.bDataReady = FALSE;
	gpSrv->pConsole->hdr.hConWnd = ghConWnd; _ASSERTE(ghConWnd!=NULL);
	_ASSERTE(gpSrv->dwMainServerPID!=0);
	gpSrv->pConsole->hdr.nServerPID = gpSrv->dwMainServerPID;
	gpSrv->pConsole->hdr.nAltServerPID = (gnRunMode==RM_ALTSERVER) ? GetCurrentProcessId() : gpSrv->dwAltServerPID;
	gpSrv->pConsole->hdr.nGuiPID = gpSrv->dwGuiPID;
	gpSrv->pConsole->hdr.hConEmuRoot = ghConEmuWnd;
	gpSrv->pConsole->hdr.hConEmuWnd = ghConEmuWndDC;
	_ASSERTE(gpSrv->pConsole->hdr.hConEmuRoot==NULL || gpSrv->pConsole->hdr.nGuiPID!=0);
	gpSrv->pConsole->hdr.bConsoleActive = TRUE; // пока - TRUE (это на старте сервера)
	gpSrv->pConsole->hdr.nServerInShutdown = 0;
	gpSrv->pConsole->hdr.bThawRefreshThread = TRUE; // пока - TRUE (это на старте сервера)
	gpSrv->pConsole->hdr.nProtocolVersion = CESERVER_REQ_VER;
	gpSrv->pConsole->hdr.nActiveFarPID = gpSrv->nActiveFarPID; // PID последнего активного фара

	// Обновить переменные окружения (через ConEmuGuiMapping)
	if (ghConEmuWnd) // если уже известен - тогда можно
		ReloadGuiSettings(NULL);

	//WARNING! В начале структуры info идет CESERVER_REQ_HDR для унификации общения через пайпы
	gpSrv->pConsole->info.cmd.cbSize = sizeof(gpSrv->pConsole->info); // Пока тут - только размер заголовка
	gpSrv->pConsole->info.hConWnd = ghConWnd; _ASSERTE(ghConWnd!=NULL);
	gpSrv->pConsole->info.crMaxSize = crMax;
	
	// Проверять, нужно ли реестр хукать, будем в конце ServerInit
	
	//WARNING! Сразу ставим флаг измененности чтобы данные сразу пошли в GUI
	gpSrv->pConsole->bDataChanged = TRUE;
	
	gpSrv->pConsoleMap = new MFileMapping<CESERVER_CONSOLE_MAPPING_HDR>;

	if (!gpSrv->pConsoleMap)
	{
		_printf("ConEmuC: calloc(MFileMapping<CESERVER_CONSOLE_MAPPING_HDR>) failed, pConsoleMap is null", 0); //-V576
		goto wrap;
	}

	gpSrv->pConsoleMap->InitName(CECONMAPNAME, (DWORD)ghConWnd); //-V205

	if (!gpSrv->pConsoleMap->Create())
	{
		_wprintf(gpSrv->pConsoleMap->GetErrorText());
		delete gpSrv->pConsoleMap; gpSrv->pConsoleMap = NULL;
		iRc = CERR_CREATEMAPPINGERR; goto wrap;
	}

	//gpSrv->pConsoleMap->SetFrom(&(gpSrv->pConsole->hdr));
	UpdateConsoleMapHeader();
wrap:
	return iRc;
}

void UpdateConsoleMapHeader()
{
	if (gpSrv && gpSrv->pConsole)
	{
		if (gnRunMode == RM_SERVER) // !!! RM_ALTSERVER - ниже !!!
		{
			if (ghConEmuWndDC && (!gpSrv->pColorerMapping || (gpSrv->pConsole->hdr.hConEmuWnd != ghConEmuWndDC)))
			{
				if (gpSrv->pColorerMapping && (gpSrv->pConsole->hdr.hConEmuWnd != ghConEmuWndDC))
				{
					// По идее, не должно быть пересоздания TrueColor мэппинга
					_ASSERTE(gpSrv->pColorerMapping);
					delete gpSrv->pColorerMapping;
					gpSrv->pColorerMapping = NULL;
				}
				CreateColorerHeader();
			}
			gpSrv->pConsole->hdr.nServerPID = GetCurrentProcessId();
		}
		else if (gnRunMode == RM_ALTSERVER)
		{
			DWORD nCurServerInMap = 0;
			if (gpSrv->pConsoleMap && gpSrv->pConsoleMap->IsValid())
				nCurServerInMap = gpSrv->pConsoleMap->Ptr()->nServerPID;

			_ASSERTE(gpSrv->pConsole->hdr.nServerPID && (gpSrv->pConsole->hdr.nServerPID == gpSrv->dwMainServerPID));
			if (nCurServerInMap && (nCurServerInMap != gpSrv->dwMainServerPID))
			{
				if (IsMainServerPID(nCurServerInMap))
				{
					// Странно, основной сервер сменился?
					_ASSERTE((nCurServerInMap == gpSrv->dwMainServerPID) && "Main server was changed?");
					CloseHandle(gpSrv->hMainServer);
					gpSrv->dwMainServerPID = nCurServerInMap;
					gpSrv->hMainServer = OpenProcess(SYNCHRONIZE|PROCESS_QUERY_INFORMATION, FALSE, nCurServerInMap);
				}
			}

			gpSrv->pConsole->hdr.nServerPID = gpSrv->dwMainServerPID;
			gpSrv->pConsole->hdr.nAltServerPID = GetCurrentProcessId();
		}

		if (gnRunMode == RM_SERVER || gnRunMode == RM_ALTSERVER)
		{
			// Размер _видимой_ области. Консольным приложениям запрещено менять его "изнутри".
			// Размер может менять только пользователь ресайзом окна ConEmu
			_ASSERTE(gcrVisibleSize.X>0 && gcrVisibleSize.X<=400 && gcrVisibleSize.Y>0 && gcrVisibleSize.Y<=300);
			gpSrv->pConsole->hdr.bLockVisibleArea = TRUE;
			gpSrv->pConsole->hdr.crLockedVisible = gcrVisibleSize;
			// Какая прокрутка допустима. Пока - любая.
			gpSrv->pConsole->hdr.rbsAllowed = rbs_Any;
		}
		gpSrv->pConsole->hdr.nGuiPID = gpSrv->dwGuiPID;
		gpSrv->pConsole->hdr.hConEmuRoot = ghConEmuWnd;
		gpSrv->pConsole->hdr.hConEmuWnd = ghConEmuWndDC;
		_ASSERTE(gpSrv->pConsole->hdr.hConEmuRoot==NULL || gpSrv->pConsole->hdr.nGuiPID!=0);
		gpSrv->pConsole->hdr.nActiveFarPID = gpSrv->nActiveFarPID;

		if (gnRunMode != RM_SERVER && gnRunMode != RM_ALTSERVER)
		{
			_ASSERTE(gnRunMode == RM_SERVER || gnRunMode == RM_ALTSERVER);
			return;
		}

		if (gpSrv->pConsoleMap)
		{
			gpSrv->pConsoleMap->SetFrom(&(gpSrv->pConsole->hdr));
		}
	}
}

int CreateColorerHeader()
{
	int iRc = -1;
	//wchar_t szMapName[64];
	DWORD dwErr = 0;
	//int nConInfoSize = sizeof(CESERVER_CONSOLE_MAPPING_HDR);
	//int nMapCells = 0;
	//DWORD nMapSize = 0;
	HWND lhConWnd = NULL;
	_ASSERTE(gpSrv->pColorerMapping == NULL);
	// 111101 - было "GetConEmuHWND(2)", но GetConsoleWindow теперь перехватывается.
	lhConWnd = ghConEmuWndDC; // GetConEmuHWND(2);

	if (!lhConWnd)
	{
		_ASSERTE(lhConWnd != NULL);
		dwErr = GetLastError();
		_printf("Can't create console data file mapping. ConEmu DC window is NULL.\n");
		//iRc = CERR_COLORERMAPPINGERR; -- ошибка не критическая и не обрабатывается
		return 0;
	}

	//COORD crMaxSize = GetLargestConsoleWindowSize(GetStdHandle(STD_OUTPUT_HANDLE));
	//nMapCells = max(crMaxSize.X,200) * max(crMaxSize.Y,200) * 2;
	//nMapSize = nMapCells * sizeof(AnnotationInfo) + sizeof(AnnotationHeader);

	if (gpSrv->pColorerMapping == NULL)
	{
		gpSrv->pColorerMapping = new MFileMapping<const AnnotationHeader>;
	}
	// Задать имя для mapping, если надо - сам сделает CloseMap();
	gpSrv->pColorerMapping->InitName(AnnotationShareName, (DWORD)sizeof(AnnotationInfo), (DWORD)lhConWnd); //-V205

	//_wsprintf(szMapName, SKIPLEN(countof(szMapName)) AnnotationShareName, sizeof(AnnotationInfo), (DWORD)lhConWnd);
	//gpSrv->hColorerMapping = CreateFileMapping(INVALID_HANDLE_VALUE,
	//                                        gpLocalSecurity, PAGE_READWRITE, 0, nMapSize, szMapName);

	//if (!gpSrv->hColorerMapping)
	//{
	//	dwErr = GetLastError();
	//	_printf("Can't create colorer data mapping. ErrCode=0x%08X\n", dwErr, szMapName);
	//	iRc = CERR_COLORERMAPPINGERR;
	//}
	//else
	//{
	// Заголовок мэппинга содержит информацию о размере, нужно заполнить!
	//AnnotationHeader* pHdr = (AnnotationHeader*)MapViewOfFile(gpSrv->hColorerMapping, FILE_MAP_ALL_ACCESS,0,0,0);
	// 111101 - было "Create(nMapSize);"
	const AnnotationHeader* pHdr = gpSrv->pColorerMapping->Open();

	if (!pHdr)
	{
		dwErr = GetLastError();
		// 111101 теперь не ошибка - мэппинг может быть отключен в ConEmu
		//_printf("Can't map colorer data mapping. ErrCode=0x%08X\n", dwErr/*, szMapName*/);
		//_wprintf(gpSrv->pColorerMapping->GetErrorText());
		//_printf("\n");
		//iRc = CERR_COLORERMAPPINGERR;
		//CloseHandle(gpSrv->hColorerMapping); gpSrv->hColorerMapping = NULL;
		delete gpSrv->pColorerMapping;
		gpSrv->pColorerMapping = NULL;
	}
	else if (pHdr->struct_size != sizeof(AnnotationHeader))
	{
		_ASSERTE(pHdr->struct_size == sizeof(AnnotationHeader));
		delete gpSrv->pColorerMapping;
		gpSrv->pColorerMapping = NULL;
	}
	else
	{
		//pHdr->struct_size = sizeof(AnnotationHeader);
		//pHdr->bufferSize = nMapCells;
		_ASSERTE((gnRunMode == RM_ALTSERVER) || (pHdr->locked == 0 && pHdr->flushCounter == 0));
		//pHdr->locked = 0;
		//pHdr->flushCounter = 0;
		gpSrv->ColorerHdr = *pHdr;
		//// В сервере - данные не нужны
		////UnmapViewOfFile(pHdr);
		//gpSrv->pColorerMapping->ClosePtr();

		// OK
		iRc = 0;
	}

	//}

	return iRc;
}

void CloseMapHeader()
{
	if (gpSrv->pConsoleMap)
	{
		//gpSrv->pConsoleMap->CloseMap(); -- не требуется, сделает деструктор
		delete gpSrv->pConsoleMap;
		gpSrv->pConsoleMap = NULL;
	}

	if (gpSrv->pConsole)
	{
		free(gpSrv->pConsole);
		gpSrv->pConsole = NULL;
	}

	if (gpSrv->pConsoleDataCopy)
	{
		free(gpSrv->pConsoleDataCopy);
		gpSrv->pConsoleDataCopy = NULL;
	}
}


// Возвращает TRUE - если меняет РАЗМЕР видимой области (что нужно применить в консоль)
BOOL CorrectVisibleRect(CONSOLE_SCREEN_BUFFER_INFO* pSbi)
{
	BOOL lbChanged = FALSE;
	_ASSERTE(gcrVisibleSize.Y<200); // высота видимой области
	// Игнорируем горизонтальный скроллинг
	SHORT nLeft = 0;
	SHORT nRight = pSbi->dwSize.X - 1;
	SHORT nTop = pSbi->srWindow.Top;
	SHORT nBottom = pSbi->srWindow.Bottom;

	if (gnBufferHeight == 0)
	{
		// Сервер мог еще не успеть среагировать на изменение режима BufferHeight
		if (pSbi->dwMaximumWindowSize.Y < pSbi->dwSize.Y)
		{
			// Это однозначно буферный режим, т.к. высота буфера больше максимально допустимого размера окна
			// Вполне нормальная ситуация. Запуская VBinDiff который ставит свой буфер,
			// соответственно сам убирая прокрутку, а при выходе возвращая ее...
			//_ASSERTE(pSbi->dwMaximumWindowSize.Y >= pSbi->dwSize.Y);
			gnBufferHeight = pSbi->dwSize.Y;
		}
	}

	// Игнорируем вертикальный скроллинг для обычного режима
	if (gnBufferHeight == 0)
	{
		nTop = 0;
		nBottom = pSbi->dwSize.Y - 1;
	}
	else if (gpSrv->nTopVisibleLine!=-1)
	{
		// А для 'буферного' режима позиция может быть заблокирована
		nTop = gpSrv->nTopVisibleLine;
		nBottom = min((pSbi->dwSize.Y-1), (gpSrv->nTopVisibleLine+gcrVisibleSize.Y-1)); //-V592
	}
	else
	{
		// Просто корректируем нижнюю строку по отображаемому в GUI региону
		// хорошо бы эту коррекцию сделать так, чтобы курсор был видим
		if (pSbi->dwCursorPosition.Y == pSbi->srWindow.Bottom)
		{
			// Если курсор находится в нижней видимой строке (теоретически, это может быть единственная видимая строка)
			nTop = pSbi->dwCursorPosition.Y - gcrVisibleSize.Y + 1; // раздвигаем область вверх от курсора
		}
		else
		{
			// Иначе - раздвигаем вверх (или вниз) минимально, чтобы курсор стал видим
			if ((pSbi->dwCursorPosition.Y < pSbi->srWindow.Top) || (pSbi->dwCursorPosition.Y > pSbi->srWindow.Bottom))
			{
				nTop = pSbi->dwCursorPosition.Y - gcrVisibleSize.Y + 1;
			}
		}

		// Страховка от выхода за пределы
		if (nTop<0) nTop = 0;

		// Корректируем нижнюю границу по верхней + желаемой высоте видимой области
		nBottom = (nTop + gcrVisibleSize.Y - 1);

		// Если же расчетный низ вылезает за пределы буфера (хотя не должен бы?)
		if (nBottom >= pSbi->dwSize.Y)
		{
			// корректируем низ
			nBottom = pSbi->dwSize.Y - 1;
			// и верх по желаемому размеру
			nTop = max(0, (nBottom - gcrVisibleSize.Y + 1));
		}
	}

#ifdef _DEBUG

	if ((pSbi->srWindow.Bottom - pSbi->srWindow.Top)>pSbi->dwMaximumWindowSize.Y)
	{
		_ASSERTE((pSbi->srWindow.Bottom - pSbi->srWindow.Top)<pSbi->dwMaximumWindowSize.Y);
	}

#endif

	if (nLeft != pSbi->srWindow.Left
	        || nRight != pSbi->srWindow.Right
	        || nTop != pSbi->srWindow.Top
	        || nBottom != pSbi->srWindow.Bottom)
		lbChanged = TRUE;

	return lbChanged;
}






static BOOL ReadConsoleInfo()
{
	BOOL lbRc = TRUE;
	BOOL lbChanged = gpSrv->pConsole->bDataChanged; // Если что-то еще не отослали - сразу TRUE
	//CONSOLE_SELECTION_INFO lsel = {0}; // GetConsoleSelectionInfo
	CONSOLE_CURSOR_INFO lci = {0}; // GetConsoleCursorInfo
	DWORD ldwConsoleCP=0, ldwConsoleOutputCP=0, ldwConsoleMode=0;
	CONSOLE_SCREEN_BUFFER_INFO lsbi = {{0,0}}; // MyGetConsoleScreenBufferInfo
	HANDLE hOut = (HANDLE)ghConOut;
	//HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	//DWORD nConInMode = 0;

	if (hOut == INVALID_HANDLE_VALUE)
		hOut = GetStdHandle(STD_OUTPUT_HANDLE);

	// Могут возникать проблемы при закрытии ComSpec и уменьшении высоты буфера
	MCHKHEAP;

	if (!GetConsoleCursorInfo(hOut, &lci))
	{
		gpSrv->dwCiRc = GetLastError(); if (!gpSrv->dwCiRc) gpSrv->dwCiRc = -1;
	}
	else
	{
		TODO("Нифига не реагирует на режим вставки в cmd.exe, видимо, GetConsoleMode можно получить только в cmd.exe");
		//if (gpSrv->bTelnetActive) lci.dwSize = 15;  // telnet "глючит" при нажатии Ins - меняет курсор даже когда нажат Ctrl например
		//GetConsoleMode(hIn, &nConInMode);
		//GetConsoleMode(hOut, &nConInMode);
		//if (GetConsoleMode(hIn, &nConInMode) && !(nConInMode & ENABLE_INSERT_MODE) && (lci.dwSize < 50))
		//	lci.dwSize = 50;

		gpSrv->dwCiRc = 0;

		if (memcmp(&gpSrv->ci, &lci, sizeof(gpSrv->ci)))
		{
			gpSrv->ci = lci;
			lbChanged = TRUE;
		}
	}

	ldwConsoleCP = GetConsoleCP();

	if (gpSrv->dwConsoleCP!=ldwConsoleCP)
	{
		gpSrv->dwConsoleCP = ldwConsoleCP; lbChanged = TRUE;
	}

	ldwConsoleOutputCP = GetConsoleOutputCP();

	if (gpSrv->dwConsoleOutputCP!=ldwConsoleOutputCP)
	{
		gpSrv->dwConsoleOutputCP = ldwConsoleOutputCP; lbChanged = TRUE;
	}

	ldwConsoleMode = 0;
#ifdef _DEBUG
	BOOL lbConModRc =
#endif
	    GetConsoleMode(/*ghConIn*/GetStdHandle(STD_INPUT_HANDLE), &ldwConsoleMode);

	if (gpSrv->dwConsoleMode!=ldwConsoleMode)
	{
		gpSrv->dwConsoleMode = ldwConsoleMode; lbChanged = TRUE;
	}

	MCHKHEAP;

	if (!MyGetConsoleScreenBufferInfo(hOut, &lsbi))
	{

		gpSrv->dwSbiRc = GetLastError(); if (!gpSrv->dwSbiRc) gpSrv->dwSbiRc = -1;

		lbRc = FALSE;
	}
	else
	{
		DWORD nCurScroll = (gnBufferHeight ? rbs_Vert : 0) | (gnBufferWidth ? rbs_Horz : 0);
		DWORD nNewScroll = 0;
		int TextWidth = 0, TextHeight = 0;
		BOOL bSuccess = ::GetConWindowSize(lsbi, gcrVisibleSize.X, gcrVisibleSize.Y, nCurScroll, &TextWidth, &TextHeight, &nNewScroll);

		// Скорректировать "видимый" буфер. Видимым считаем то, что показывается в ConEmu
		if (bSuccess)
		{
			//rgn = gpSrv->sbi.srWindow;
			if (!(nNewScroll & rbs_Horz))
			{
				lsbi.srWindow.Left = 0;
				lsbi.srWindow.Right = lsbi.dwSize.X-1;
			}

			if (!(nNewScroll & rbs_Vert))
			{
				lsbi.srWindow.Top = 0;
				lsbi.srWindow.Bottom = lsbi.dwSize.Y-1;
			}
		}

		if (memcmp(&gpSrv->sbi, &lsbi, sizeof(gpSrv->sbi)))
		{
			_ASSERTE(lsbi.srWindow.Left == 0);
			/*
			//Issue 373: при запуске wmic устанавливается ШИРИНА буфера в 1500 символов
			//           пока ConEmu не поддерживает горизонтальную прокрутку - игнорим,
			//           будем показывать только видимую область окна
			if (lsbi.srWindow.Right != (lsbi.dwSize.X - 1))
			{
				//_ASSERTE(lsbi.srWindow.Right == (lsbi.dwSize.X - 1)); -- ругаться пока не будем
				lsbi.dwSize.X = (lsbi.srWindow.Right - lsbi.srWindow.Left + 1);
			}
			*/

			// Консольное приложение могло изменить размер буфера
			if (!NTVDMACTIVE)  // НЕ при запущенном 16битном приложении - там мы все жестко фиксируем, иначе съезжает размер при закрытии 16бит
			{
				if ((lsbi.srWindow.Top == 0  // или окно соответсвует полному буферу
				        && lsbi.dwSize.Y == (lsbi.srWindow.Bottom - lsbi.srWindow.Top + 1)))
				{
					// Это значит, что прокрутки нет, и консольное приложение изменило размер буфера
					gnBufferHeight = 0; gcrVisibleSize = lsbi.dwSize;
				}

				if (lsbi.dwSize.X != gpSrv->sbi.dwSize.X
				        || (lsbi.srWindow.Bottom - lsbi.srWindow.Top) != (gpSrv->sbi.srWindow.Bottom - gpSrv->sbi.srWindow.Top))
				{
					// При изменении размера видимой области - обязательно передернуть данные
					gpSrv->pConsole->bDataChanged = TRUE;
				}
			}

#ifdef ASSERT_UNWANTED_SIZE
			COORD crReq = gpSrv->crReqSizeNewSize;
			COORD crSize = lsbi.dwSize;

			if (crReq.X != crSize.X && !gpSrv->dwDisplayMode && !IsZoomed(ghConWnd))
			{
				// Только если не было запрошено изменение размера консоли!
				if (!gpSrv->nRequestChangeSize)
				{
					LogSize(NULL, ":ReadConsoleInfo(AssertWidth)");
					wchar_t /*szTitle[64],*/ szInfo[128];
					//_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuC, PID=%i", GetCurrentProcessId());
					_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"Size req by server: {%ix%i},  Current size: {%ix%i}",
					          crReq.X, crReq.Y, crSize.X, crSize.Y);
					//MessageBox(NULL, szInfo, szTitle, MB_OK|MB_SETFOREGROUND|MB_SYSTEMMODAL);
					MY_ASSERT_EXPR(FALSE, szInfo, false);
				}
			}

#endif

			if (ghLogSize) LogSize(NULL, ":ReadConsoleInfo");

			gpSrv->sbi = lsbi;
			lbChanged = TRUE;
		}
	}

	if (!gnBufferHeight)
	{
		int nWndHeight = (gpSrv->sbi.srWindow.Bottom - gpSrv->sbi.srWindow.Top + 1);

		if (gpSrv->sbi.dwSize.Y > (max(gcrVisibleSize.Y,nWndHeight)+200)
		        || (gpSrv->nRequestChangeSize && gpSrv->nReqSizeBufferHeight))
		{
			// Приложение изменило размер буфера!

			if (!gpSrv->nReqSizeBufferHeight)
			{
				//#ifdef _DEBUG
				//EmergencyShow(ghConWnd);
				//#endif
				WARNING("###: Приложение изменило вертикальный размер буфера");
				if (gpSrv->sbi.dwSize.Y > 200)
				{
					//_ASSERTE(gpSrv->sbi.dwSize.Y <= 200);
					DEBUGLOGSIZE(L"!!! gpSrv->sbi.dwSize.Y > 200 !!! in ConEmuC.ReloadConsoleInfo\n");
				}
				gpSrv->nReqSizeBufferHeight = gpSrv->sbi.dwSize.Y;
			}

			gnBufferHeight = gpSrv->nReqSizeBufferHeight;
		}

		//	Sleep(10);
		//} else {
		//	break; // OK
	}

	// Лучше всегда делать, чтобы данные были гарантированно актуальные
	gpSrv->pConsole->hdr.hConWnd = gpSrv->pConsole->info.hConWnd = ghConWnd;
	_ASSERTE(gpSrv->dwMainServerPID!=0);
	gpSrv->pConsole->hdr.nServerPID = gpSrv->dwMainServerPID;
	gpSrv->pConsole->hdr.nAltServerPID = (gnRunMode==RM_ALTSERVER) ? GetCurrentProcessId() : gpSrv->dwAltServerPID;
	//gpSrv->pConsole->info.nInputTID = gpSrv->dwInputThreadId;
	gpSrv->pConsole->info.nReserved0 = 0;
	gpSrv->pConsole->info.dwCiSize = sizeof(gpSrv->ci);
	gpSrv->pConsole->info.ci = gpSrv->ci;
	gpSrv->pConsole->info.dwConsoleCP = gpSrv->dwConsoleCP;
	gpSrv->pConsole->info.dwConsoleOutputCP = gpSrv->dwConsoleOutputCP;
	gpSrv->pConsole->info.dwConsoleMode = gpSrv->dwConsoleMode;
	gpSrv->pConsole->info.dwSbiSize = sizeof(gpSrv->sbi);
	gpSrv->pConsole->info.sbi = gpSrv->sbi;
	// Если есть возможность (WinXP+) - получим реальный список процессов из консоли
	//CheckProcessCount(); -- уже должно быть вызвано !!!
	//2010-05-26 Изменения в списке процессов не приходили в GUI до любого чиха в консоль.
	_ASSERTE(gpSrv->pnProcesses!=NULL);

	if (!gpSrv->nProcessCount /*&& gpSrv->pConsole->info.nProcesses[0]*/)
	{
		_ASSERTE(gpSrv->nProcessCount); //CheckProcessCount(); -- уже должно быть вызвано !!!
		lbChanged = TRUE;
	}
	else if (memcmp(gpSrv->pnProcesses, gpSrv->pConsole->info.nProcesses,
	               sizeof(DWORD)*min(gpSrv->nProcessCount,countof(gpSrv->pConsole->info.nProcesses))))
	{
		// Список процессов изменился!
		lbChanged = TRUE;
	}

	GetProcessCount(gpSrv->pConsole->info.nProcesses, countof(gpSrv->pConsole->info.nProcesses));
	_ASSERTE(gpSrv->pConsole->info.nProcesses[0]);
	//if (memcmp(&(gpSrv->pConsole->hdr), gpSrv->pConsoleMap->Ptr(), gpSrv->pConsole->hdr.cbSize))
	//	gpSrv->pConsoleMap->SetFrom(&(gpSrv->pConsole->hdr));
	//if (lbChanged) {
	//	gpSrv->pConsoleMap->SetFrom(&(gpSrv->pConsole->hdr));
	//	//lbChanged = TRUE;
	//}
	//if (lbChanged) {
	//	//gpSrv->pConsole->bChanged = TRUE;
	//}
	return lbChanged;
}

// !! test test !!

// !!! Засечка времени чтения данных консоли показала, что само чтение занимает мизерное время
// !!! Повтор 1000 раз чтения буфера размером 140x76 занимает 100мс.
// !!! Чтение 1000 раз по строке (140x1) занимает 30мс.
// !!! Резюме. Во избежание усложнения кода и глюков отрисовки читаем всегда все полностью.
// !!! А выигрыш за счет частичного чтения - незначителен и создает риск некорректного чтения.


static BOOL ReadConsoleData()
{
	BOOL lbRc = FALSE, lbChanged = FALSE;
#ifdef _DEBUG
	CONSOLE_SCREEN_BUFFER_INFO dbgSbi = gpSrv->sbi;
#endif
	HANDLE hOut = NULL;
	//USHORT TextWidth=0, TextHeight=0;
	DWORD TextLen=0;
	COORD bufSize, bufCoord;
	SMALL_RECT rgn;
	DWORD nCurSize, nHdrSize;
	// -- начинаем потихоньку горизонтальную прокрутку
	_ASSERTE(gpSrv->sbi.srWindow.Left == 0); // этот пока оставим
	//_ASSERTE(gpSrv->sbi.srWindow.Right == (gpSrv->sbi.dwSize.X - 1));
	DWORD nCurScroll = (gnBufferHeight ? rbs_Vert : 0) | (gnBufferWidth ? rbs_Horz : 0);
	DWORD nNewScroll = 0;
	int TextWidth = 0, TextHeight = 0;
	BOOL bSuccess = ::GetConWindowSize(gpSrv->sbi, gcrVisibleSize.X, gcrVisibleSize.Y, nCurScroll, &TextWidth, &TextHeight, &nNewScroll);
	//TextWidth  = gpSrv->sbi.dwSize.X;
	//TextHeight = (gpSrv->sbi.srWindow.Bottom - gpSrv->sbi.srWindow.Top + 1);
	TextLen = TextWidth * TextHeight;

	//rgn = gpSrv->sbi.srWindow;
	if (nNewScroll & rbs_Horz)
	{
		rgn.Left = gpSrv->sbi.srWindow.Left;
		rgn.Right = min(gpSrv->sbi.srWindow.Left+TextWidth,gpSrv->sbi.dwSize.X)-1;
	}
	else
	{
		rgn.Left = 0;
		rgn.Right = gpSrv->sbi.dwSize.X-1;
	}

	if (nNewScroll & rbs_Vert)
	{
		rgn.Top = gpSrv->sbi.srWindow.Top;
		rgn.Bottom = min(gpSrv->sbi.srWindow.Top+TextHeight,gpSrv->sbi.dwSize.Y)-1;
	}
	else
	{
		rgn.Top = 0;
		rgn.Bottom = gpSrv->sbi.dwSize.Y-1;
	}
	

	if (!TextWidth || !TextHeight)
	{
		_ASSERTE(TextWidth && TextHeight);
		goto wrap;
	}

	nCurSize = TextLen * sizeof(CHAR_INFO);
	nHdrSize = sizeof(CESERVER_REQ_CONINFO_FULL)-sizeof(CHAR_INFO);

	if (!gpSrv->pConsole || gpSrv->pConsole->cbMaxSize < (nCurSize+nHdrSize))
	{
		_ASSERTE(gpSrv->pConsole && gpSrv->pConsole->cbMaxSize >= (nCurSize+nHdrSize));
		TextHeight = (gpSrv->pConsole->info.crMaxSize.X * gpSrv->pConsole->info.crMaxSize.Y) / TextWidth;

		if (!TextHeight)
		{
			_ASSERTE(TextHeight);
			goto wrap;
		}

		TextLen = TextWidth * TextHeight;
		nCurSize = TextLen * sizeof(CHAR_INFO);
		// Если MapFile еще не создавался, или был увеличен размер консоли
		//if (!RecreateMapData())
		//{
		//	// Раз не удалось пересоздать MapFile - то и дергаться не нужно...
		//	goto wrap;
		//}
		//_ASSERTE(gpSrv->nConsoleDataSize >= (nCurSize+nHdrSize));
	}

	gpSrv->pConsole->info.crWindow.X = TextWidth; gpSrv->pConsole->info.crWindow.Y = TextHeight;
	hOut = (HANDLE)ghConOut;

	if (hOut == INVALID_HANDLE_VALUE)
		hOut = GetStdHandle(STD_OUTPUT_HANDLE);

	lbRc = FALSE;

	if (nCurSize <= MAX_CONREAD_SIZE)
	{
		bufSize.X = TextWidth; bufSize.Y = TextHeight;
		bufCoord.X = 0; bufCoord.Y = 0;
		//rgn = gpSrv->sbi.srWindow;

		if (ReadConsoleOutput(hOut, gpSrv->pConsoleDataCopy, bufSize, bufCoord, &rgn))
			lbRc = TRUE;
	}

	if (!lbRc)
	{
		// Придется читать построчно
		bufSize.X = TextWidth; bufSize.Y = 1;
		bufCoord.X = 0; bufCoord.Y = 0;
		//rgn = gpSrv->sbi.srWindow;
		CHAR_INFO* pLine = gpSrv->pConsoleDataCopy;

		for(int y = 0; y < (int)TextHeight; y++, rgn.Top++, pLine+=TextWidth)
		{
			rgn.Bottom = rgn.Top;
			ReadConsoleOutput(hOut, pLine, bufSize, bufCoord, &rgn);
		}
	}

	// низя - он уже установлен в максимальное значение
	//gpSrv->pConsoleDataCopy->crBufSize.X = TextWidth;
	//gpSrv->pConsoleDataCopy->crBufSize.Y = TextHeight;

	if (memcmp(gpSrv->pConsole->data, gpSrv->pConsoleDataCopy, nCurSize))
	{
		memmove(gpSrv->pConsole->data, gpSrv->pConsoleDataCopy, nCurSize);
		gpSrv->pConsole->bDataChanged = TRUE; // TRUE уже может быть с прошлого раза, не сбросывать в FALSE
		lbChanged = TRUE;
	}


	if (!lbChanged && gpSrv->pColorerMapping)
	{
		AnnotationHeader ahdr;
		if (gpSrv->pColorerMapping->GetTo(&ahdr, sizeof(ahdr)))
		{
			if (gpSrv->ColorerHdr.flushCounter != ahdr.flushCounter && !ahdr.locked)
			{
				gpSrv->ColorerHdr = ahdr;
				gpSrv->pConsole->bDataChanged = TRUE; // TRUE уже может быть с прошлого раза, не сбросывать в FALSE
				lbChanged = TRUE;
			}
		}
	}


	// низя - он уже установлен в максимальное значение
	//gpSrv->pConsoleData->crBufSize = gpSrv->pConsoleDataCopy->crBufSize;
wrap:
	//if (lbChanged)
	//	gpSrv->pConsole->bDataChanged = TRUE;
	return lbChanged;
}




// abForceSend выставляется в TRUE, чтобы гарантированно
// передернуть GUI по таймауту (не реже 1 сек).
BOOL ReloadFullConsoleInfo(BOOL abForceSend)
{
	BOOL lbChanged = abForceSend;
	BOOL lbDataChanged = abForceSend;
	DWORD dwCurThId = GetCurrentThreadId();

	// Должен вызываться ТОЛЬКО в нити (RefreshThread)
	// Иначе возможны блокировки
	if (gpSrv->dwRefreshThread && dwCurThId != gpSrv->dwRefreshThread)
	{
		//ResetEvent(gpSrv->hDataReadyEvent);
		if (abForceSend)
			gpSrv->bForceConsoleRead = TRUE;

		ResetEvent(gpSrv->hRefreshDoneEvent);
		SetEvent(gpSrv->hRefreshEvent);
		// Ожидание, пока сработает RefreshThread
		HANDLE hEvents[2] = {ghQuitEvent, gpSrv->hRefreshDoneEvent};
		DWORD nWait = WaitForMultipleObjects(2, hEvents, FALSE, RELOAD_INFO_TIMEOUT);
		lbChanged = (nWait == (WAIT_OBJECT_0+1));

		if (abForceSend)
			gpSrv->bForceConsoleRead = FALSE;

		return lbChanged;
	}

#ifdef _DEBUG
	DWORD nPacketID = gpSrv->pConsole->info.nPacketId;
#endif

	if (gpSrv->hExtConsoleCommit)
	{
		WaitForSingleObject(gpSrv->hExtConsoleCommit, EXTCONCOMMIT_TIMEOUT);
	}

	if (abForceSend)
		gpSrv->pConsole->bDataChanged = TRUE;

	if (ReadConsoleInfo())
		lbChanged = TRUE;

	if (ReadConsoleData())
		lbChanged = lbDataChanged = TRUE;

	if (lbChanged && !gpSrv->pConsole->hdr.bDataReady)
	{
		gpSrv->pConsole->hdr.bDataReady = TRUE;
	}

	if (memcmp(&(gpSrv->pConsole->hdr), gpSrv->pConsoleMap->Ptr(), gpSrv->pConsole->hdr.cbSize))
	{
		lbChanged = TRUE;
		//gpSrv->pConsoleMap->SetFrom(&(gpSrv->pConsole->hdr));
		UpdateConsoleMapHeader();
	}

	if (lbChanged)
	{
		// Накрутить счетчик и Tick
		//gpSrv->pConsole->bChanged = TRUE;
		//if (lbDataChanged)
		gpSrv->pConsole->info.nPacketId++;
		gpSrv->pConsole->info.nSrvUpdateTick = GetTickCount();

		if (gpSrv->hDataReadyEvent)
			SetEvent(gpSrv->hDataReadyEvent);

		//if (nPacketID == gpSrv->pConsole->info.nPacketId) {
		//	gpSrv->pConsole->info.nPacketId++;
		//	TODO("Можно заменить на multimedia tick");
		//	gpSrv->pConsole->info.nSrvUpdateTick = GetTickCount();
		//	//			gpSrv->nFarInfoLastIdx = gpSrv->pConsole->info.nFarInfoIdx;
		//}
	}

	return lbChanged;
}






DWORD WINAPI RefreshThread(LPVOID lpvParam)
{
	DWORD nWait = 0, nAltWait = 0;
	HANDLE hEvents[2] = {ghQuitEvent, gpSrv->hRefreshEvent};
	DWORD nDelta = 0;
	DWORD nLastReadTick = 0; //GetTickCount();
	DWORD nLastConHandleTick = GetTickCount();
	BOOL  /*lbEventualChange = FALSE,*/ /*lbForceSend = FALSE,*/ lbChanged = FALSE; //, lbProcessChanged = FALSE;
	DWORD dwTimeout = 10; // периодичность чтения информации об окне (размеров, курсора,...)
	DWORD dwAltTimeout = 100;
	//BOOL  bForceRefreshSetSize = FALSE; // После изменения размера нужно сразу перечитать консоль без задержек
	BOOL lbWasSizeChange = FALSE;

	while (TRUE)
	{
		nWait = WAIT_TIMEOUT;
		//lbForceSend = FALSE;
		MCHKHEAP;

		nAltWait = gpSrv->hAltServer ? WaitForSingleObject(gpSrv->hAltServer, dwAltTimeout) : WAIT_OBJECT_0;

		// Always update con handle, мягкий вариант
		// !!! В Win7 закрытие дескриптора в ДРУГОМ процессе - закрывает консольный буфер ПОЛНОСТЬЮ. В итоге, буфер вывода telnet'а схлопывается! !!!
		// 120507 - Если крутится альт.сервер - то игнорировать
		if (gpSrv->bReopenHandleAllowed
			&& !nAltWait
			&& ((GetTickCount() - nLastConHandleTick) > UPDATECONHANDLE_TIMEOUT))
		{
			ghConOut.Close();
			nLastConHandleTick = GetTickCount();
		}

		//// Попытка поправить CECMD_SETCONSOLECP
		//if (gpSrv->hLockRefreshBegin)
		//{
		//	// Если создано событие блокировки обновления -
		//	// нужно дождаться, пока оно (hLockRefreshBegin) будет выставлено
		//	SetEvent(gpSrv->hLockRefreshReady);
		//
		//	while(gpSrv->hLockRefreshBegin
		//	        && WaitForSingleObject(gpSrv->hLockRefreshBegin, 10) == WAIT_TIMEOUT)
		//		SetEvent(gpSrv->hLockRefreshReady);
		//}

		#ifdef _DEBUG
		if (nAltWait == WAIT_TIMEOUT)
		{
			// Если крутится альт.сервер - то запрос на изменение размера сюда приходить НЕ ДОЛЖЕН
			_ASSERTE(!gpSrv->nRequestChangeSize);
		}
		#endif

		// Из другой нити поступил запрос на изменение размера консоли
		// 120507 - Если крутится альт.сервер - то игнорировать
		if (!nAltWait && gpSrv->nRequestChangeSize)
		{
			// AVP гундит... да вроде и не нужно
			//DWORD dwSusp = 0, dwSuspErr = 0;
			//if (gpSrv->hRootThread) {
			//	WARNING("A 64-bit application can suspend a WOW64 thread using the Wow64SuspendThread function");
			//	// The handle must have the THREAD_SUSPEND_RESUME access right
			//	dwSusp = SuspendThread(gpSrv->hRootThread);
			//	if (dwSusp == (DWORD)-1) dwSuspErr = GetLastError();
			//}
			SetConsoleSize(gpSrv->nReqSizeBufferHeight, gpSrv->crReqSizeNewSize, gpSrv->rReqSizeNewRect, gpSrv->sReqSizeLabel);
			//if (gpSrv->hRootThread) {
			//	ResumeThread(gpSrv->hRootThread);
			//}
			// Событие выставим ПОСЛЕ окончания перечитывания консоли
			lbWasSizeChange = TRUE;
			//SetEvent(gpSrv->hReqSizeChanged);
		}

		// Проверить количество процессов в консоли.
		// Функция выставит ghExitQueryEvent, если все процессы завершились.
		//lbProcessChanged = CheckProcessCount();
		// Функция срабатывает только через интервал CHECK_PROCESSES_TIMEOUT (внутри защита от частых вызовов)
		// #define CHECK_PROCESSES_TIMEOUT 500
		CheckProcessCount();

		// Подождать немножко
		if (gpSrv->nMaxFPS>0)
		{
			dwTimeout = 1000 / gpSrv->nMaxFPS;

			// Было 50, чтобы не перенапрягать консоль при ее быстром обновлении ("dir /s" и т.п.)
			if (dwTimeout < 10) dwTimeout = 10;
		}
		else
		{
			dwTimeout = 100;
		}

		// !!! Здесь таймаут должен быть минимальным, ну разве что консоль неактивна
		//		HANDLE hOut = (HANDLE)ghConOut;
		//		if (hOut == INVALID_HANDLE_VALUE) hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		//		TODO("Проверить, а собственно реагирует на изменения в консоли?");
		//		-- на изменения (запись) в консоли не реагирует
		//		dwConWait = WaitForSingleObject ( hOut, dwTimeout );
		//#ifdef _DEBUG
		//		if (dwConWait == WAIT_OBJECT_0) {
		//			DEBUGSTRCHANGES(L"STD_OUTPUT_HANDLE was set\n");
		//		}
		//#endif
		//
		if (lbWasSizeChange)
			nWait = (WAIT_OBJECT_0+1); // требуется перечитать консоль после изменения размера!
		else
			nWait = WaitForMultipleObjects(2, hEvents, FALSE, dwTimeout/*dwTimeout*/);

		if (nWait == WAIT_OBJECT_0)
		{
			break; // затребовано завершение нити
		}// else if (nWait == WAIT_TIMEOUT && dwConWait == WAIT_OBJECT_0) {

		//	nWait = (WAIT_OBJECT_0+1);
		//}

		//lbEventualChange = (nWait == (WAIT_OBJECT_0+1))/* || lbProcessChanged*/;
		//lbForceSend = (nWait == (WAIT_OBJECT_0+1));

		BOOL bThaw = TRUE; // Если FALSE - снизить нагрузку на conhost
		BOOL bConsoleActive = TRUE;
		WARNING("gpSrv->pConsole->hdr.bConsoleActive и gpSrv->pConsole->hdr.bThawRefreshThread могут быть неактуальными!");
		//if (gpSrv->pConsole->hdr.bConsoleActive && gpSrv->pConsoleMap)
		{
			if (gpSrv->pConsoleMap->IsValid())
			{
				CESERVER_CONSOLE_MAPPING_HDR* p = gpSrv->pConsoleMap->Ptr();
				bThaw = p->bThawRefreshThread;
				bConsoleActive = p->bConsoleActive;
			}
		}

		// Чтобы не грузить процессор неактивными консолями спим, если
		// только что не было затребовано изменение размера консоли
		if (!lbWasSizeChange
		        // не требуется принудительное перечитывание
		        && !gpSrv->bForceConsoleRead
		        // Консоль не активна
		        && (!bConsoleActive
		            // или активна, но сам ConEmu GUI не в фокусе
		            || (bConsoleActive && !bThaw))
		        // и не дернули событие gpSrv->hRefreshEvent
		        && (nWait != (WAIT_OBJECT_0+1))
				&& !gpSrv->bWasReattached)
		{
			DWORD nCurTick = GetTickCount();
			nDelta = nCurTick - nLastReadTick;

			// #define MAX_FORCEREFRESH_INTERVAL 500
			if (nDelta <= MAX_FORCEREFRESH_INTERVAL)
			{
				// Чтобы не грузить процессор
				continue;
			}
		}

#ifdef _DEBUG

		if (nWait == (WAIT_OBJECT_0+1))
		{
			DEBUGSTR(L"*** hRefreshEvent was set, checking console...\n");
		}

#endif

		if (ghConEmuWnd && !IsWindow(ghConEmuWnd))
		{
			gpSrv->bWasDetached = TRUE;
			ghConEmuWnd = NULL;
			ghConEmuWndDC = NULL;
			gpSrv->dwGuiPID = 0;
			UpdateConsoleMapHeader();
			EmergencyShow(ghConWnd);
		}

		// 17.12.2009 Maks - попробую убрать
		//if (ghConEmuWnd && GetForegroundWindow() == ghConWnd) {
		//	if (lbFirstForeground || !IsWindowVisible(ghConWnd)) {
		//		DEBUGSTR(L"...apiSetForegroundWindow(ghConEmuWnd);\n");
		//		apiSetForegroundWindow(ghConEmuWnd);
		//		lbFirstForeground = FALSE;
		//	}
		//}

		// Если можем - проверим текущую раскладку в консоли
		// 120507 - Если крутится альт.сервер - то игнорировать
		if (!nAltWait && !gpSrv->bDebuggerActive)
		{
			if (pfnGetConsoleKeyboardLayoutName)
				CheckKeyboardLayout();
		}

		/* ****************** */
		/* Перечитать консоль */
		/* ****************** */
		// 120507 - Если крутится альт.сервер - то игнорировать
		if (!nAltWait && !gpSrv->bDebuggerActive)
		{
			lbChanged = ReloadFullConsoleInfo(gpSrv->bWasReattached/*lbForceSend*/);
			// При этом должно передернуться gpSrv->hDataReadyEvent
			if (gpSrv->bWasReattached)
			{
				_ASSERTE(lbChanged);
				_ASSERTE(gpSrv->pConsole && gpSrv->pConsole->bDataChanged);
				gpSrv->bWasReattached = FALSE;
			}
		}
		else
		{
			lbChanged = FALSE;
		}

		// Событие выставим ПОСЛЕ окончания перечитывания консоли
		if (lbWasSizeChange)
		{
			SetEvent(gpSrv->hReqSizeChanged);
			lbWasSizeChange = FALSE;
		}

		if (nWait == (WAIT_OBJECT_0+1))
			SetEvent(gpSrv->hRefreshDoneEvent);

		// запомнить последний tick
		//if (lbChanged)
		nLastReadTick = GetTickCount();
		MCHKHEAP
	}

	return 0;
}



int MySetWindowRgn(CESERVER_REQ_SETWINDOWRGN* pRgn)
{
	if (!pRgn)
	{
		_ASSERTE(pRgn!=NULL);
		return 0; // Invalid argument!
	}

	if (pRgn->nRectCount == 0)
	{
		return SetWindowRgn((HWND)pRgn->hWnd, NULL, pRgn->bRedraw);
	}
	else if (pRgn->nRectCount == -1)
	{
		apiShowWindow((HWND)pRgn->hWnd, SW_HIDE);
		return SetWindowRgn((HWND)pRgn->hWnd, NULL, FALSE);
	}

	// Нужно считать...
	HRGN hRgn = NULL, hSubRgn = NULL, hCombine = NULL;
	BOOL lbPanelVisible = TRUE;
	hRgn = CreateRectRgn(pRgn->rcRects->left, pRgn->rcRects->top, pRgn->rcRects->right, pRgn->rcRects->bottom);

	for(int i = 1; i < pRgn->nRectCount; i++)
	{
		RECT rcTest;

		// IntersectRect не работает, если низ совпадает?
		_ASSERTE(pRgn->rcRects->bottom != (pRgn->rcRects+i)->bottom);
		if (!IntersectRect(&rcTest, pRgn->rcRects, pRgn->rcRects+i))
			continue;

		// Все остальные прямоугольники вычитать из hRgn
		hSubRgn = CreateRectRgn(rcTest.left, rcTest.top, rcTest.right, rcTest.bottom);

		if (!hCombine)
			hCombine = CreateRectRgn(0,0,1,1);

		int nCRC = CombineRgn(hCombine, hRgn, hSubRgn, RGN_DIFF);

		if (nCRC)
		{
			// Замена переменных
			HRGN hTmp = hRgn; hRgn = hCombine; hCombine = hTmp;
			// А этот больше не нужен
			DeleteObject(hSubRgn); hSubRgn = NULL;
		}

		// Проверка, может регион уже пуст?
		if (nCRC == NULLREGION)
		{
			lbPanelVisible = FALSE; break;
		}
	}

	int iRc = 0;
	SetWindowRgn((HWND)pRgn->hWnd, hRgn, TRUE); hRgn = NULL;

	// чистка
	if (hCombine) { DeleteObject(hCombine); hCombine = NULL; }

	return iRc;
}
