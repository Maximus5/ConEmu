
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

#undef TEST_REFRESH_DELAYED

#include "ConEmuC.h"
#include "../common/ConsoleAnnotation.h"
#include "../common/ConsoleRead.h"
#include "../common/Execute.h"
#include "../common/MStrSafe.h"
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
//int CreateColorerHeader();


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
		if (gpLogSize) LogSize(NULL, ":SetConsoleFontSizeTo.before");

		SetConsoleFontSizeTo(ghConWnd, gpSrv->nConFontHeight, gpSrv->nConFontWidth, gpSrv->szConsoleFont, gnDefTextColors, gnDefPopupColors);

		if (gpLogSize) LogSize(NULL, ":SetConsoleFontSizeTo.after");
	}
}

BOOL LoadGuiSettings(ConEmuGuiMapping& GuiMapping)
{
	BOOL lbRc = FALSE;
	HWND hGuiWnd = ghConEmuWnd ? ghConEmuWnd : gpSrv->hGuiWnd;

	if (hGuiWnd && IsWindow(hGuiWnd))
	{
		DWORD dwGuiThreadId, dwGuiProcessId;
		MFileMapping<ConEmuGuiMapping> GuiInfoMapping;
		dwGuiThreadId = GetWindowThreadProcessId(hGuiWnd, &dwGuiProcessId);

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

		UpdateComspec(&gpSrv->guiSettings.ComSpec); // isAddConEmu2Path, ...

		SetEnvironmentVariableW(ENV_CONEMUDIR_VAR_W, gpSrv->guiSettings.sConEmuDir);
		SetEnvironmentVariableW(ENV_CONEMUBASEDIR_VAR_W, gpSrv->guiSettings.sConEmuBaseDir);

		// Не будем ставить сами, эту переменную заполняет Gui при своем запуске
		// соответственно, переменная наследуется серверами
		//SetEnvironmentVariableW(L"ConEmuArgs", pInfo->sConEmuArgs);

		wchar_t szHWND[16]; _wsprintf(szHWND, SKIPLEN(countof(szHWND)) L"0x%08X", gpSrv->guiSettings.hGuiWnd.u);
		SetEnvironmentVariableW(ENV_CONEMUHWND_VAR_W, szHWND);

		if (gpSrv->pConsole)
		{
			// !!! Warning !!! Изменил здесь, поменяй и CreateMapHeader() !!!
			
			gpSrv->pConsole->hdr.nLoggingType = gpSrv->guiSettings.nLoggingType;
			gpSrv->pConsole->hdr.bDosBox = gpSrv->guiSettings.bDosBox;
			gpSrv->pConsole->hdr.bUseInjects = gpSrv->guiSettings.bUseInjects;
			gpSrv->pConsole->hdr.bUseTrueColor = gpSrv->guiSettings.bUseTrueColor;
			gpSrv->pConsole->hdr.bProcessAnsi = gpSrv->guiSettings.bProcessAnsi;
			gpSrv->pConsole->hdr.bUseClink = gpSrv->guiSettings.bUseClink;
			
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

	_ASSERTE((gpSrv->hRootProcess == NULL || gpSrv->hRootProcess == GetCurrentProcess()) && "Must not be opened yet");

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

								if (!dwFarPID && IsFarExe(prc.szExeFile))
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
		wchar_t szSelf[MAX_PATH+1];
		wchar_t szCommand[MAX_PATH+100];

		if (!GetModuleFileName(NULL, szSelf, countof(szSelf)))
		{
			dwErr = GetLastError();
			_printf("GetModuleFileName failed, ErrCode=0x%08X\n", dwErr);
			SetLastError(dwErr);
			return CERR_CREATEPROCESS;
		}

		//if (wcschr(pszSelf, L' '))
		//{
		//	*(--pszSelf) = L'"';
		//	lstrcatW(pszSelf, L"\"");
		//}

		_wsprintf(szCommand, SKIPLEN(countof(szCommand)) L"\"%s\" /ATTACH %s/PID=%u", szSelf, gpSrv->bRequestNewGuiWnd ? L"/GHWND=NEW " : L"", dwParentPID);
		PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
		STARTUPINFOW si; memset(&si, 0, sizeof(si)); si.cb = sizeof(si);
		PRINT_COMSPEC(L"Starting modeless:\n%s\n", pszSelf);
		// CREATE_NEW_PROCESS_GROUP - низя, перестает работать Ctrl-C
		// Это запуск нового сервера в этой консоли. В сервер хуки ставить не нужно
		BOOL lbRc = createProcess(TRUE, NULL, szCommand, NULL,NULL, TRUE,
		                           NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
		dwErr = GetLastError();

		if (!lbRc)
		{
			PrintExecuteError(szCommand, dwErr);
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
		int iAttachRc = AttachDebuggingProcess();
		if (iAttachRc != 0)
			return iAttachRc;
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

	while (true)
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

		DWORD nInitTick = GetTickCount();

		while (true)
		{
			#ifdef _DEBUG
			DWORD nStartTick = GetTickCount();
			#endif

			lbCallRc = CallNamedPipe(szServerPipe, &In, In.hdr.cbSize, &Out, sizeof(Out), &dwRead, EXECUTE_CONNECT_GUI_CALL_TIMEOUT);

			DWORD dwErr = GetLastError(), nEndTick = GetTickCount();
			DWORD nConnectDelta = nEndTick - nInitTick;

			#ifdef _DEBUG
			DWORD nDelta = nEndTick - nStartTick;
			if (lbCallRc && (nDelta >= EXECUTE_CMD_WARN_TIMEOUT))
			{
				if (!IsDebuggerPresent())
				{
					//_ASSERTE(nDelta <= EXECUTE_CMD_WARN_TIMEOUT || (pIn->hdr.nCmd == CECMD_CMDSTARTSTOP && nDelta <= EXECUTE_CMD_WARN_TIMEOUT2));
					_ASSERTEX(nDelta <= EXECUTE_CMD_WARN_TIMEOUT);
				}
			}
			#endif

			if (lbCallRc || (nConnectDelta > EXECUTE_CONNECT_GUI_TIMEOUT))
				break;

			if (!lbCallRc)
			{
				_ASSERTE(lbCallRc && (dwErr==ERROR_FILE_NOT_FOUND) && "GUI was not initialized yet?");
				Sleep(250);
			}

			UNREFERENCED_PARAMETER(dwErr);
		}


		if (!lbCallRc || !Out.StartStopRet.hWndDc || !Out.StartStopRet.hWndBack)
		{
			dwErr = GetLastError();
			#ifdef _DEBUG
			if (gbIsWine)
			{
				wchar_t szDbgMsg[512], szTitle[128];
				GetModuleFileName(NULL, szDbgMsg, countof(szDbgMsg));
				msprintf(szTitle, countof(szTitle), L"%s: PID=%u", PointToName(szDbgMsg), gnSelfPID);
				msprintf(szDbgMsg, countof(szDbgMsg), L"CallNamedPipe(%s)\nFailed, code=%u, RC=%u, hWndDC=x%08X, hWndBack=x%08X", szServerPipe, dwErr, lbCallRc, (DWORD)Out.StartStopRet.hWndDc, (DWORD)Out.StartStopRet.hWndBack);
				MessageBox(NULL, szDbgMsg, szTitle, MB_SYSTEMMODAL);
			}
			else
			{
				_ASSERTEX(lbCallRc && Out.StartStopRet.hWndDc && Out.StartStopRet.hWndBack);
			}
			SetLastError(dwErr);
			#endif
		}
		else
		{
			ghConEmuWnd = Out.StartStop.hWnd;
			SetConEmuWindows(Out.StartStopRet.hWndDc, Out.StartStopRet.hWndBack);
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
		DEBUGTEST(DWORD t1 = GetTickCount());

		nWaitRc = WaitForSingleObject(gpSrv->hConEmuGuiAttached, 500);

		#ifdef _DEBUG
		DWORD t2 = GetTickCount(), tDur = t2-t1;
		if (tDur > GUIATTACHEVENT_TIMEOUT)
		{
			_ASSERTE((tDur <= GUIATTACHEVENT_TIMEOUT) && "GUI tab creation take more than 250ms");
		}
		#endif
	}

	CheckConEmuHwnd();

	return iRc;
}

bool AltServerWasStarted(DWORD nPID, HANDLE hAltServer, bool ForceThaw)
{
	_ASSERTE(nPID!=0);

	if (hAltServer == NULL)
	{
		hAltServer = OpenProcess(MY_PROCESS_ALL_ACCESS, FALSE, nPID);
		if (hAltServer == NULL)
		{
			hAltServer = OpenProcess(SYNCHRONIZE|PROCESS_QUERY_INFORMATION, FALSE, nPID);
			if (hAltServer == NULL)
			{
				return false;
			}
		}
	}

	if (gpSrv->dwAltServerPID && (gpSrv->dwAltServerPID != nPID))
	{
		// Остановить старый (текущий) сервер
		CESERVER_REQ* pFreezeIn = ExecuteNewCmd(CECMD_FREEZEALTSRV, sizeof(CESERVER_REQ_HDR)+2*sizeof(DWORD));
		if (pFreezeIn)
		{
			pFreezeIn->dwData[0] = 1;
			pFreezeIn->dwData[1] = nPID;
			CESERVER_REQ* pFreezeOut = ExecuteSrvCmd(gpSrv->dwAltServerPID, pFreezeIn, ghConWnd);
			ExecuteFreeResult(pFreezeIn);
			ExecuteFreeResult(pFreezeOut);
		}

		// Если для nPID не было назначно "предыдущего" альт.сервера
		if (!gpSrv->AltServers.Get(nPID, NULL))
		{
			// нужно сохранить параметры этого предыдущего (пусть даже и пустые)
			AltServerInfo info = {nPID};
			info.hPrev = gpSrv->hAltServer; // may be NULL
			info.nPrevPID = gpSrv->dwAltServerPID; // may be 0
			gpSrv->AltServers.Set(nPID, info);
		}
	}


	// Перевести нить монитора в режим ожидания завершения AltServer, инициализировать gpSrv->dwAltServerPID, gpSrv->hAltServer

	//if (gpSrv->hAltServer && (gpSrv->hAltServer != hAltServer))
	//{
	//	gpSrv->dwAltServerPID = 0;
	//	SafeCloseHandle(gpSrv->hAltServer);
	//}

	gpSrv->hAltServer = hAltServer;
	gpSrv->dwAltServerPID = nPID;

	if (gpSrv->hAltServerChanged && (GetCurrentThreadId() != gpSrv->dwRefreshThread))
	{
		// В RefreshThread ожидание хоть и небольшое (100мс), но лучше передернуть
		SetEvent(gpSrv->hAltServerChanged);
	}


	if (ForceThaw)
	{
		// Отпустить новый сервер (который раньше замораживался)
		CESERVER_REQ* pFreezeIn = ExecuteNewCmd(CECMD_FREEZEALTSRV, sizeof(CESERVER_REQ_HDR)+2*sizeof(DWORD));
		if (pFreezeIn)
		{
			pFreezeIn->dwData[0] = 0;
			pFreezeIn->dwData[1] = 0;
			CESERVER_REQ* pFreezeOut = ExecuteSrvCmd(gpSrv->dwAltServerPID, pFreezeIn, ghConWnd);
			ExecuteFreeResult(pFreezeIn);
			ExecuteFreeResult(pFreezeOut);
		}
	}

	return (hAltServer != NULL);
}

DWORD WINAPI SetOemCpProc(LPVOID lpParameter)
{
	UINT nCP = (UINT)(DWORD_PTR)lpParameter;
	SetConsoleCP(nCP);
	SetConsoleOutputCP(nCP);
	return 0;
}


// Создать необходимые события и нити
int ServerInit(int anWorkMode/*0-Server,1-AltServer,2-Reserved*/)
{
	int iRc = 0;
	DWORD dwErr = 0;
	wchar_t szName[64];

	if (anWorkMode == 0)
	{
		gpSrv->dwMainServerPID = GetCurrentProcessId();
		gpSrv->hMainServer = GetCurrentProcess();

		_ASSERTE(gnRunMode==RM_SERVER);

		if (gbIsDBCS)
		{
			UINT nOemCP = GetOEMCP();
			UINT nConCP = GetConsoleOutputCP();
			if (nConCP != nOemCP)
			{
				DWORD nTID;
				HANDLE h = CreateThread(NULL, 0, SetOemCpProc, (LPVOID)nOemCP, 0, &nTID);
				if (h && (h != INVALID_HANDLE_VALUE))
				{
					DWORD nWait = WaitForSingleObject(h, 5000);
					if (nWait != WAIT_OBJECT_0)
					{
						_ASSERTE(nWait==WAIT_OBJECT_0 && "SetOemCpProc HUNGS!!!");
						TerminateThread(h, 100);
					}
					CloseHandle(h);
				}
			}
		}
	}


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

	// Шрифт в консоли нужно менять в самом начале, иначе могут быть проблемы с установкой размера консоли
	if ((anWorkMode == 0) && !gpSrv->bDebuggerActive && !gbNoCreateProcess)
		//&& (!gbNoCreateProcess || (gbAttachMode && gbNoCreateProcess && gpSrv->dwRootProcess))
		//)
	{
		ServerInitFont();

		//bool bMovedBottom = false;

		// Minimized окошко нужно развернуть!
		// Не помню уже зачем, возможно, что-то с мышкой связано...
		if (IsIconic(ghConWnd))
		{
			//WINDOWPLACEMENT wplGui = {sizeof(wplGui)};
			//// По идее, HWND гуя нам уже должен быть известен (передан аргументом)
			//if (gpSrv->hGuiWnd)
			//	GetWindowPlacement(gpSrv->hGuiWnd, &wplGui);
			//SendMessage(ghConWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
			WINDOWPLACEMENT wplCon = {sizeof(wplCon)};
			GetWindowPlacement(ghConWnd, &wplCon);
			//wplCon.showCmd = SW_SHOWNA;
			////RECT rc = {wplGui.rcNormalPosition.left+3,wplGui.rcNormalPosition.top+3,wplCon.rcNormalPosition.right-wplCon.rcNormalPosition.left,wplCon.rcNormalPosition.bottom-wplCon.rcNormalPosition.top};
			//// т.к. ниже все равно делается "SetWindowPos(ghConWnd, NULL, 0, 0, ..." - можем задвинуть подальше
			//RECT rc = {-30000,-30000,-30000+wplCon.rcNormalPosition.right-wplCon.rcNormalPosition.left,-30000+wplCon.rcNormalPosition.bottom-wplCon.rcNormalPosition.top};
			//wplCon.rcNormalPosition = rc;
			////SetWindowPos(ghConWnd, HWND_BOTTOM, 0, 0, 0,0, SWP_NOSIZE|SWP_NOMOVE);
			//SetWindowPlacement(ghConWnd, &wplCon);
			wplCon.showCmd = SW_RESTORE;
			SetWindowPlacement(ghConWnd, &wplCon);
			//bMovedBottom = true;
		}

		if (!gbVisibleOnStartup && IsWindowVisible(ghConWnd))
		{
			ShowWindow(ghConWnd, SW_HIDE);
			//if (bMovedBottom)
			//{
			//	SetWindowPos(ghConWnd, HWND_TOP, 0, 0, 0,0, SWP_NOSIZE|SWP_NOMOVE);
			//}
		}

		// -- чтобы на некоторых системах не возникала проблема с позиционированием -> {0,0}
		// Issue 274: Окно реальной консоли позиционируется в неудобном месте
		SetWindowPos(ghConWnd, NULL, 0, 0, 0,0, SWP_NOSIZE|SWP_NOZORDER);
	}

	// Не будем, наверное. OnTop попытается поставить сервер,
	// при показе консоли через Ctrl+Win+Alt+Space
	// Но вот если консоль уже видима, и это "/root", тогда
	// попытаемся поставить окну консоли флаг "OnTop"
	if (!gbNoCreateProcess && !gbDebugProcess && !gbIsWine /*&& !gnDefPopupColors*/)
	{
		//if (!gbVisibleOnStartup)
		//	ShowWindow(ghConWnd, SW_HIDE);
		SetWindowPos(ghConWnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
	}
	//if (!gbVisibleOnStartup && IsWindowVisible(ghConWnd))
	//{
	//	ShowWindow(ghConWnd, SW_HIDE);
	//}

	// Подготовить буфер для длинного вывода
	// RM_SERVER - создать и считать текущее содержимое консоли
	// RM_ALTSERVER - только создать (по факту - выполняется открытие созданного в RM_SERVER)
	_ASSERTE(gnRunMode==RM_SERVER || gnRunMode==RM_ALTSERVER);
	CmdOutputStore(true/*abCreateOnly*/);
	#if 0
	_ASSERTE(gpcsStoredOutput==NULL && gpStoredOutput==NULL);
	if (!gpcsStoredOutput)
	{
		gpcsStoredOutput = new MSection;
	}
	#endif


	// Включить по умолчанию выделение мышью
	if ((anWorkMode == 0) && !gbNoCreateProcess && gbConsoleModeFlags /*&& !(gbParmBufferSize && gnBufferHeight == 0)*/)
	{
		ServerInitConsoleMode();
	}

	//2009-08-27 Перенес снизу
	if (!gpSrv->hConEmuGuiAttached && (!gbDebugProcess || gpSrv->dwGuiPID || gpSrv->hGuiWnd))
	{
		wchar_t szTempName[MAX_PATH];
		_wsprintf(szTempName, SKIPLEN(countof(szTempName)) CEGUIRCONSTARTED, (DWORD)ghConWnd); //-V205

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

	gpSrv->csAltSrv = new MSection();
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
	
	gpSrv->InputQueue.Initialize(CE_MAX_INPUT_QUEUE_BUFFER, gpSrv->hInputEvent);
	//gpSrv->nMaxInputQueue = CE_MAX_INPUT_QUEUE_BUFFER;
	//gpSrv->pInputQueue = (INPUT_RECORD*)calloc(gpSrv->nMaxInputQueue, sizeof(INPUT_RECORD));
	//gpSrv->pInputQueueEnd = gpSrv->pInputQueue+gpSrv->nMaxInputQueue;
	//gpSrv->pInputQueueWrite = gpSrv->pInputQueue;
	//gpSrv->pInputQueueRead = gpSrv->pInputQueueEnd;

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
	if (gbNoCreateProcess && (gbAttachMode || (gpSrv->bDebuggerActive && (gpSrv->hRootProcess == NULL))))
	{
		iRc = AttachRootProcess();
		if (iRc != 0)
			goto wrap;
	}


	gpSrv->hAllowInputEvent = CreateEvent(NULL, TRUE, TRUE, NULL);

	if (!gpSrv->hAllowInputEvent) SetEvent(gpSrv->hAllowInputEvent);

	

	_ASSERTE(gpSrv->pConsole!=NULL);
	//gpSrv->pConsole->hdr.bConsoleActive = TRUE;
	//gpSrv->pConsole->hdr.bThawRefreshThread = TRUE;

	// Если указаны параметры (gbParmBufferSize && gcrVisibleSize.X && gcrVisibleSize.Y) - установить размер
	// Иначе - получить текущие размеры из консольного окна
	ServerInitConsoleSize();

	//// Minimized окошко нужно развернуть!
	//if (IsIconic(ghConWnd))
	//{
	//	WINDOWPLACEMENT wplCon = {sizeof(wplCon)};
	//	GetWindowPlacement(ghConWnd, &wplCon);
	//	wplCon.showCmd = SW_RESTORE;
	//	SetWindowPlacement(ghConWnd, &wplCon);
	//}

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

	_ASSERTE(gnSelfPID == GetCurrentProcessId());
	_wsprintf(szName, SKIPLEN(countof(szName)) CEFARWRITECMTEVENT, gnSelfPID);
	gpSrv->hFarCommitEvent = CreateEvent(NULL,FALSE,FALSE,szName);
	if (!gpSrv->hFarCommitEvent)
	{
		dwErr = GetLastError();
		_ASSERTE(gpSrv->hFarCommitEvent!=NULL);
		_printf("CreateEvent(hFarCommitEvent) failed, ErrCode=0x%08X\n", dwErr);
		iRc = CERR_REFRESHEVENT; goto wrap;
	}

	_wsprintf(szName, SKIPLEN(countof(szName)) CECURSORCHANGEEVENT, gnSelfPID);
	gpSrv->hCursorChangeEvent = CreateEvent(NULL,FALSE,FALSE,szName);
	if (!gpSrv->hCursorChangeEvent)
	{
		dwErr = GetLastError();
		_ASSERTE(gpSrv->hCursorChangeEvent!=NULL);
		_printf("CreateEvent(hCursorChangeEvent) failed, ErrCode=0x%08X\n", dwErr);
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
	gpSrv->pReqSizeSection = new MSection();

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
		_ASSERTE(IsWindow(ghConEmuWndBack));
		_ASSERTE(IsWindow(gpSrv->hRootProcessGui));
		pIn->AttachGuiApp.hConEmuWnd = ghConEmuWnd;
		pIn->AttachGuiApp.hConEmuDc = ghConEmuWndDC;
		pIn->AttachGuiApp.hConEmuBack = ghConEmuWndBack;
		pIn->AttachGuiApp.hAppWindow = gpSrv->hRootProcessGui;
		pIn->AttachGuiApp.hSrvConWnd = ghConWnd;
		wchar_t szPipe[MAX_PATH];
		_ASSERTE(gpSrv->dwRootProcess!=0);
		_wsprintf(szPipe, SKIPLEN(countof(szPipe)) CEHOOKSPIPENAME, L".", gpSrv->dwRootProcess);
		CESERVER_REQ* pOut = ExecuteCmd(szPipe, pIn, GUIATTACH_TIMEOUT, ghConWnd);
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

	_ASSERTE(gnSelfPID == GetCurrentProcessId());
	_wsprintf(szName, SKIPLEN(countof(szName)) CESRVSTARTEDEVENT, gnSelfPID);
	// Event мог быть создан и ранее (в Far-плагине, например)
	gpSrv->hServerStartedEvent = CreateEvent(LocalSecurity(), TRUE, FALSE, szName);
	if (!gpSrv->hServerStartedEvent)
	{
		_ASSERTE(gpSrv->hServerStartedEvent!=NULL);
	}
	else
	{
		SetEvent(gpSrv->hServerStartedEvent);
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
		if (!nExitQueryPlace) nExitQueryPlace = 10+(nExitPlaceStep);

		SetTerminateEvent(ste_ServerDone);
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



	if (gpSrv->hInputThread)
	{
		// Подождем немножко, пока нить сама завершится
		WARNING("Не завершается?");

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
		gpSrv->InputServer.StopPipeServer(false);

	//SafeCloseHandle(gpSrv->hInputPipe);
	SafeCloseHandle(gpSrv->hInputEvent);

	if (gpSrv)
		gpSrv->DataServer.StopPipeServer(false);

	if (gpSrv)
		gpSrv->CmdServer.StopPipeServer(false);

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

	SafeCloseHandle(gpSrv->hAltServerChanged);

	SafeCloseHandle(gpSrv->hRefreshEvent);

	SafeCloseHandle(gpSrv->hFarCommitEvent);

	SafeCloseHandle(gpSrv->hCursorChangeEvent);

	SafeCloseHandle(gpSrv->hRefreshDoneEvent);

	SafeCloseHandle(gpSrv->hDataReadyEvent);

	SafeCloseHandle(gpSrv->hServerStartedEvent);

	//if (gpSrv->hChangingSize) {
	//    SafeCloseHandle(gpSrv->hChangingSize);
	//}
	// Отключить все хуки
	//gpSrv->bWinHookAllow = FALSE; gpSrv->nWinHookMode = 0;
	//HookWinEvents ( -1 );

	SafeDelete(gpSrv->pStoredOutputItem);
	SafeDelete(gpSrv->pStoredOutputHdr);
	#if 0
	{
		MSectionLock CS; CS.Lock(gpcsStoredOutput, TRUE);
		SafeFree(gpStoredOutput);
		CS.Unlock();
		SafeDelete(gpcsStoredOutput);
	}
	#endif

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

	SafeDelete(gpSrv->csAltSrv);
	SafeDelete(gpSrv->csProc);

	SafeFree(gpSrv->pnProcesses);

	SafeFree(gpSrv->pnProcessesGet);

	SafeFree(gpSrv->pnProcessesCopy);

	//SafeFree(gpSrv->pInputQueue);
	gpSrv->InputQueue.Release();

	CloseMapHeader();

	//SafeCloseHandle(gpSrv->hColorerMapping);
	SafeDelete(gpSrv->pColorerMapping);

	SafeCloseHandle(gpSrv->hReqSizeChanged);
	SafeDelete(gpSrv->pReqSizeSection);
}

// Консоль любит глючить, при попытках запроса более определенного количества ячеек.
// MAX_CONREAD_SIZE подобрано экспериментально
BOOL MyReadConsoleOutput(HANDLE hOut, CHAR_INFO *pData, COORD& bufSize, SMALL_RECT& rgn)
{
	//BOOL lbRc = FALSE;
	//static bool bDBCS = false, bDBCS_Checked = false;
	//if (!bDBCS_Checked)
	//{
	//	bDBCS = (GetSystemMetrics(SM_DBCSENABLED) != 0);
	//	bDBCS_Checked = true;
	//}

	//bool   bDBCS_CP = bDBCS;
	//LPCSTR szLeads = NULL;
	//UINT   MaxCharSize = 0;
	//DWORD  nCP, nCP1, nMode;

	//if (bDBCS)
	//{
	//	nCP = GetConsoleOutputCP();
	//	nCP1 = GetConsoleCP();
	//	GetConsoleMode(hOut, &nMode);

	//	szLeads = GetCpInfoLeads(nCP, &MaxCharSize);
	//	if (!szLeads || !*szLeads || MaxCharSize < 2)
	//	{
	//		bDBCS_CP = false;
	//	}
	//}

	//size_t nBufWidth = bufSize.X;
	//int nWidth = (rgn.Right - rgn.Left + 1);
	//int nHeight = (rgn.Bottom - rgn.Top + 1);
	//int nCurSize = nWidth * nHeight;

	//_ASSERTE(bufSize.X >= nWidth);
	//_ASSERTE(bufSize.Y >= nHeight);
	//_ASSERTE(rgn.Top>=0 && rgn.Bottom>=rgn.Top);
	//_ASSERTE(rgn.Left>=0 && rgn.Right>=rgn.Left);

	MSectionLock RCS;
	if (gpSrv->pReqSizeSection && !RCS.Lock(gpSrv->pReqSizeSection, TRUE, 30000))
	{
		_ASSERTE(FALSE);
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;

	}

	return ReadConsoleOutputEx(hOut, pData, bufSize, rgn);

	//COORD bufCoord = {0,0};
	//DWORD dwErrCode = 0;

	//if (!bDBCS_CP && (nCurSize <= MAX_CONREAD_SIZE))
	//{
	//	if (ReadConsoleOutputW(hOut, pData, bufSize, bufCoord, &rgn))
	//		lbRc = TRUE;
	//}

	//if (!lbRc)
	//{
	//	// Придется читать построчно
	//	
	//	// Теоретически - можно и блоками, но оверхед очень маленький, а велик
	//	// шанс обломаться, если консоль "глючит". Поэтому построчно...

	//	//bufSize.X = TextWidth;
	//	bufSize.Y = 1;
	//	bufCoord.X = 0; bufCoord.Y = 0;
	//	//rgn = gpSrv->sbi.srWindow;

	//	int Y1 = rgn.Top;
	//	int Y2 = rgn.Bottom;

	//	CHAR_INFO* pLine = pData;
	//	if (!bDBCS_CP)
	//	{
	//		for (int y = Y1; y <= Y2; y++, rgn.Top++, pLine+=nBufWidth)
	//		{
	//			rgn.Bottom = rgn.Top;
	//			lbRc = ReadConsoleOutputW(hOut, pLine, bufSize, bufCoord, &rgn);
	//			if (!lbRc)
	//			{
	//				dwErrCode = GetLastError();
	//				_ASSERTE(FALSE && "ReadConsoleOutputW failed in MyReadConsoleOutput");
	//				break;
	//			}
	//		}
	//	}
	//	else
	//	{
	//		DWORD nAttrsMax = bufSize.X;
	//		DWORD nCharsMax = nAttrsMax/* *4 */; // -- максимум там вроде на некоторых CP - 4 байта
	//		wchar_t* pszChars = (wchar_t*)malloc(nCharsMax*sizeof(*pszChars));
	//		char* pszCharsA = (char*)malloc(nCharsMax*sizeof(*pszCharsA));
	//		WORD* pnAttrs = (WORD*)malloc(bufSize.X*sizeof(*pnAttrs));
	//		if (pszChars && pszCharsA && pnAttrs)
	//		{
	//			COORD crRead = {rgn.Left,Y1};
	//			DWORD nChars, nAttrs, nCharsA;
	//			CHAR_INFO* pLine = pData;
	//			for (; crRead.Y <= Y2; crRead.Y++, pLine+=nBufWidth)
	//			{
	//				rgn.Bottom = rgn.Top;

	//				nChars = nCharsA = nAttrs = 0;
	//				BOOL lbRcTxt = ReadConsoleOutputCharacterA(hOut, pszCharsA, nCharsMax, crRead, &nCharsA);
	//				dwErrCode = GetLastError();
	//				if (!lbRcTxt || !nCharsA)
	//				{
	//					nCharsA = 0;
	//					lbRcTxt = ReadConsoleOutputCharacterW(hOut, pszChars, nCharsMax, crRead, &nChars);
	//					dwErrCode = GetLastError();
	//				}
	//				BOOL lbRcAtr = ReadConsoleOutputAttribute(hOut, pnAttrs, bufSize.X, crRead, &nAttrs);
	//				dwErrCode = GetLastError();
	//				
	//				lbRc = lbRcTxt && lbRcAtr;

	//				if (!lbRc)
	//				{
	//					dwErrCode = GetLastError();
	//					_ASSERTE(FALSE && "ReadConsoleOutputAttribute failed in MyReadConsoleOutput");

	//					CHAR_INFO* p = pLine;
	//					for (size_t i = 0; i < nAttrsMax; ++i, ++p)
	//					{
	//						p->Attributes = 4; // red on black
	//						p->Char.UnicodeChar = 0xFFFE; // not a character
	//					}

	//					break;
	//				}
	//				else
	//				{
	//					if (nCharsA)
	//					{
	//						nChars = MultiByteToWideChar(nCP, 0, pszCharsA, nCharsA, pszChars, nCharsMax);
	//					}
	//					CHAR_INFO* p = pLine;
	//					wchar_t* psz = pszChars;
	//					WORD* pn = pnAttrs;
	//					//int i = nAttrsMax;
	//					//while ((i--) > 0)
	//					//{
	//					//	p->Attributes = *(pn++);
	//					//	p->Char.UnicodeChar = *(psz++);
	//					//	p++;
	//					//}
	//					size_t x1 = min(nChars,nAttrsMax);
	//					size_t x2 = nAttrsMax;
	//					for (size_t i = 0; i < x1; ++i, ++p)
	//					{
	//						p->Attributes = *pn;
	//						p->Char.UnicodeChar = *psz;

	//						WARNING("Некорректно! pn может указывать на начало блока DBCS/QBCS");
	//						pn++; // += MaxCharSize;
	//						psz++;
	//					}
	//					WORD nLastAttr = pnAttrs[max(0,(int)nAttrs-1)];
	//					for (size_t i = x1; i < x2; ++i, ++p)
	//					{
	//						p->Attributes = nLastAttr;
	//						p->Char.UnicodeChar = L' ';
	//					}
	//				}
	//			}
	//		}
	//		SafeFree(pszChars);
	//		SafeFree(pszCharsA);
	//		SafeFree(pnAttrs);
	//	}
	//}

	//return lbRc;
}

// Консоль любит глючить, при попытках запроса более определенного количества ячеек.
// MAX_CONREAD_SIZE подобрано экспериментально
BOOL MyWriteConsoleOutput(HANDLE hOut, CHAR_INFO *pData, COORD& bufSize, COORD& crBufPos, SMALL_RECT& rgn)
{
	BOOL lbRc = FALSE;

	size_t nBufWidth = bufSize.X;
	int nWidth = (rgn.Right - rgn.Left + 1);
	int nHeight = (rgn.Bottom - rgn.Top + 1);
	int nCurSize = nWidth * nHeight;

	_ASSERTE(bufSize.X >= nWidth);
	_ASSERTE(bufSize.Y >= nHeight);
	_ASSERTE(rgn.Top>=0 && rgn.Bottom>=rgn.Top);
	_ASSERTE(rgn.Left>=0 && rgn.Right>=rgn.Left);

	COORD bufCoord = crBufPos;

	if (nCurSize <= MAX_CONREAD_SIZE)
	{
		if (WriteConsoleOutputW(hOut, pData, bufSize, bufCoord, &rgn))
			lbRc = TRUE;
	}

	if (!lbRc)
	{
		// Придется читать построчно
		
		// Теоретически - можно и блоками, но оверхед очень маленький, а велик
		// шанс обломаться, если консоль "глючит". Поэтому построчно...

		//bufSize.X = TextWidth;
		bufSize.Y = 1;
		bufCoord.X = 0; bufCoord.Y = 0;
		//rgn = gpSrv->sbi.srWindow;

		int Y1 = rgn.Top;
		int Y2 = rgn.Bottom;

		CHAR_INFO* pLine = pData;
		for (int y = Y1; y <= Y2; y++, rgn.Top++, pLine+=nBufWidth)
		{
			rgn.Bottom = rgn.Top;
			lbRc = WriteConsoleOutputW(hOut, pLine, bufSize, bufCoord, &rgn);
		}
	}

	return lbRc;
}


bool CmdOutputOpenMap(CONSOLE_SCREEN_BUFFER_INFO& lsbi, CESERVER_CONSAVE_MAPHDR*& pHdr, CESERVER_CONSAVE_MAP*& pData)
{
	pHdr = NULL;
	pData = NULL;

	// В Win7 закрытие дескриптора в ДРУГОМ процессе - закрывает консольный буфер ПОЛНОСТЬЮ!!!
	// В итоге, буфер вывода telnet'а схлопывается!
	if (gpSrv->bReopenHandleAllowed)
	{
		ghConOut.Close();
	}


	// !!! Нас интересует реальное положение дел в консоли,
	//     а не скорректированное функцией MyGetConsoleScreenBufferInfo
	if (!GetConsoleScreenBufferInfo(ghConOut, &lsbi))
	{
		//CS.RelockExclusive();
		//SafeFree(gpStoredOutput);
		return false; // Не смогли получить информацию о консоли...
	}


	if (!gpSrv->pStoredOutputHdr)
	{
		gpSrv->pStoredOutputHdr = new MFileMapping<CESERVER_CONSAVE_MAPHDR>;
		gpSrv->pStoredOutputHdr->InitName(CECONOUTPUTNAME, (DWORD)ghConWnd); //-V205
		if (!(pHdr = gpSrv->pStoredOutputHdr->Create()))
		{
			_ASSERTE(FALSE && "Failed to create mapping: CESERVER_CONSAVE_MAPHDR");
			gpSrv->pStoredOutputHdr->CloseMap();
			return false;
		}

		ExecutePrepareCmd(&pHdr->hdr, 0, sizeof(*pHdr));
	}
	else
	{
		if (!(pHdr = gpSrv->pStoredOutputHdr->Ptr()))
		{
			_ASSERTE(FALSE && "Failed to get mapping Ptr: CESERVER_CONSAVE_MAPHDR");
			gpSrv->pStoredOutputHdr->CloseMap();
			return false;
		}
	}

	WARNING("А вот это нужно бы делать в RefreshThread!!!");
	DEBUGSTR(L"--- CmdOutputStore begin\n");

	//MSectionLock CS; CS.Lock(gpcsStoredOutput, FALSE);



	COORD crMaxSize = MyGetLargestConsoleWindowSize(ghConOut);
	DWORD cchOneBufferSize = lsbi.dwSize.X * lsbi.dwSize.Y; // Читаем всю консоль целиком!
	DWORD cchMaxBufferSize = max(pHdr->MaxCellCount,(DWORD)(lsbi.dwSize.Y * lsbi.dwSize.X));


	bool lbNeedRecreate = false; // требуется новый или больший, или сменился индекс (создан в другом сервере)
	bool lbNeedReopen = (gpSrv->pStoredOutputItem == NULL);
	// Warning! Мэппинг уже мог быть создан другим сервером.
	if (!pHdr->CurrentIndex || (pHdr->MaxCellCount < cchOneBufferSize))
	{
		pHdr->CurrentIndex++;
		lbNeedRecreate = true;
	}
	int nNewIndex = pHdr->CurrentIndex;

	// Проверить, если мэппинг уже открывался ранее, может его нужно переоткрыть - сменился индекс (создан в другом сервере)
	if (!lbNeedRecreate && gpSrv->pStoredOutputItem)
	{
		if (!gpSrv->pStoredOutputItem->IsValid()
			|| (nNewIndex != gpSrv->pStoredOutputItem->Ptr()->CurrentIndex))
		{
			lbNeedReopen = lbNeedRecreate = true;
		}
	}

	if (lbNeedRecreate
		|| (!gpSrv->pStoredOutputItem)
		|| (pHdr->MaxCellCount < cchOneBufferSize))
	{
		if (!gpSrv->pStoredOutputItem)
		{
			gpSrv->pStoredOutputItem = new MFileMapping<CESERVER_CONSAVE_MAP>;
			_ASSERTE(lbNeedReopen);
			lbNeedReopen = true;
		}

		if (!lbNeedRecreate && pHdr->MaxCellCount)
		{
			_ASSERTE(pHdr->MaxCellCount >= cchOneBufferSize);
			cchMaxBufferSize = pHdr->MaxCellCount;
		}

		if (lbNeedReopen || lbNeedRecreate || !gpSrv->pStoredOutputItem->IsValid())
		{
			LPCWSTR pszName = gpSrv->pStoredOutputItem->InitName(CECONOUTPUTITEMNAME, (DWORD)ghConWnd, nNewIndex); //-V205
			DWORD nMaxSize = sizeof(*pData) + cchMaxBufferSize * sizeof(pData->Data[0]);

			if (!(pData = gpSrv->pStoredOutputItem->Create(nMaxSize)))
			{
				_ASSERTE(FALSE && "Failed to create item mapping: CESERVER_CONSAVE_MAP");
				gpSrv->pStoredOutputItem->CloseMap();
				pHdr->sCurrentMap[0] = 0; // сброс, если был ранее назначен
				return false;
			}

			ExecutePrepareCmd(&pData->hdr, 0, nMaxSize);
			// Save current mapping
			pData->CurrentIndex = nNewIndex;
			pData->MaxCellCount = cchMaxBufferSize;
			pHdr->MaxCellCount = cchMaxBufferSize;
			wcscpy_c(pHdr->sCurrentMap, pszName);

			goto wrap;
		}
	}

	if (!(pData = gpSrv->pStoredOutputItem->Ptr()))
	{
		_ASSERTE(FALSE && "Failed to get item mapping Ptr: CESERVER_CONSAVE_MAP");
		gpSrv->pStoredOutputItem->CloseMap();
		return false;
	}

wrap:
	if (!pData || (pData->hdr.nVersion != CESERVER_REQ_VER) || (pData->hdr.cbSize <= sizeof(CESERVER_CONSAVE_MAP)))
	{
		_ASSERTE(pData && (pData->hdr.nVersion == CESERVER_REQ_VER) && (pData->hdr.cbSize > sizeof(CESERVER_CONSAVE_MAP)));
		gpSrv->pStoredOutputItem->CloseMap();
		return false;
	}

	return (pData != NULL);
}

// Сохранить данные ВСЕЙ консоли в gpStoredOutput
void CmdOutputStore(bool abCreateOnly /*= false*/)
{
	CONSOLE_SCREEN_BUFFER_INFO lsbi = {{0,0}};
	CESERVER_CONSAVE_MAPHDR* pHdr = NULL;
	CESERVER_CONSAVE_MAP* pData = NULL;

	if (!CmdOutputOpenMap(lsbi, pHdr, pData))
		return;

	if (!pHdr || !pData)
	{
		_ASSERTE(pHdr && pData);
		return;
	}

	if (!pHdr->info.dwSize.Y || !abCreateOnly)
		pHdr->info = lsbi;
	if (!pData->info.dwSize.Y || !abCreateOnly)
		pData->info = lsbi;

	if (abCreateOnly)
		return;

	//// Если требуется увеличение размера выделенной памяти
	//if (gpStoredOutput)
	//{
	//	if (gpStoredOutput->hdr.cbMaxOneBufferSize < (DWORD)nOneBufferSize)
	//	{
	//		CS.RelockExclusive();
	//		SafeFree(gpStoredOutput);
	//	}
	//}

	//if (gpStoredOutput == NULL)
	//{
	//	CS.RelockExclusive();
	//	// Выделяем память: заголовок + буфер текста (на атрибуты забьем)
	//	gpStoredOutput = (CESERVER_CONSAVE*)calloc(sizeof(CESERVER_CONSAVE_HDR)+nOneBufferSize,1);
	//	_ASSERTE(gpStoredOutput!=NULL);

	//	if (gpStoredOutput == NULL)
	//		return; // Не смогли выделить память

	//	gpStoredOutput->hdr.cbMaxOneBufferSize = nOneBufferSize;
	//}

	//// Запомнить sbi
	////memmove(&gpStoredOutput->hdr.sbi, &lsbi, sizeof(lsbi));
	//gpStoredOutput->hdr.sbi = lsbi;

	// Теперь читаем данные
	COORD BufSize = {lsbi.dwSize.X, lsbi.dwSize.Y};
	SMALL_RECT ReadRect = {0, 0, lsbi.dwSize.X-1, lsbi.dwSize.Y-1};

	// Запомнить/обновить sbi
	pData->info = lsbi;

	pData->Succeeded = MyReadConsoleOutput(ghConOut, pData->Data, BufSize, ReadRect);

#if 0
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

	if ((nReadLen > MAX_CONREAD_SIZE)
		!ReadConsoleOutput(ghConOut, pData->Data, BufSize, coord, &nbActuallyRead)
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
#endif

	DEBUGSTR(L"--- CmdOutputStore end\n");
}

void CmdOutputRestore()
{
	WARNING("Переделать/доделать CmdOutputRestore");
#if 0
	CONSOLE_SCREEN_BUFFER_INFO lsbi = {{0,0}};
	CESERVER_CONSAVE_MAPHDR* pHdr = NULL;
	CESERVER_CONSAVE_MAP* pData = NULL;
	if (!CmdOutputOpenMap(lsbi, pHdr, pData))
		return;

	if (lsbi.srWindow.Top > 0)
	{
		_ASSERTE(lsbi.srWindow.Top == 0 && "Upper left corner of window expected");
		return;
	}

	if (lsbi.dwSize.Y <= (lsbi.srWindow.Bottom - lsbi.srWindow.Top + 1))
	{
		// There is no scroller in window
		// Nothing to do
		return;
	}

	CHAR_INFO chrFill = {};
	chrFill.Attributes = lsbi.wAttributes;
	chrFill.Char.UnicodeChar = L' ';

	SMALL_RECT rcTop = {0,0, lsbi.dwSize.X-1, lsbi.srWindow.Bottom};
	COORD crMoveTo = {0, lsbi.dwSize.Y - 1 - lsbi.srWindow.Bottom};
	if (!ScrollConsoleScreenBuffer(ghConOut, &rcTop, NULL, crMoveTo, &chrFill))
	{
		return;
	}

	SMALL_RECT rcBottom = {0, crMoveTo.Y, lsbi.srWindow.Right, lsbi.dwSize.Y-1};
	SetConsoleWindowInfo(ghConOut, TRUE, &rcBottom);

	COORD crNewPos = {lsbi.dwCursorPosition.X, lsbi.dwCursorPosition.Y + crMoveTo.Y};
	SetConsoleCursorPosition(ghConOut, crNewPos);

#if 0
	MSectionLock CS; CS.Lock(gpcsStoredOutput, TRUE);
	if (gpStoredOutput)
	{
		
		// Учесть, что ширина консоли могла измениться со времени выполнения предыдущей команды.
		// Сейчас у нас в верхней части консоли может оставаться кусочек предыдущего вывода (восстановил FAR).
		// 1) Этот кусочек нужно считать
		// 2) Скопировать в нижнюю часть консоли (до которой докрутилась предыдущая команда)
		// 3) прокрутить консоль до предыдущей команды (куда мы только что скопировали данные сверху)
		// 4) восстановить оставшуюся часть консоли. Учесть, что фар может
		//    выполнять некоторые команды сам и курсор вообще-то мог несколько уехать...
	}
#endif

	// Восстановить текст скрытой (прокрученной вверх) части консоли
	// Учесть, что ширина консоли могла измениться со времени выполнения предыдущей команды.
	// Сейчас у нас в верхней части консоли может оставаться кусочек предыдущего вывода (восстановил FAR).
	// 1) Этот кусочек нужно считать
	// 2) Скопировать в нижнюю часть консоли (до которой докрутилась предыдущая команда)
	// 3) прокрутить консоль до предыдущей команды (куда мы только что скопировали данные сверху)
	// 4) восстановить оставшуюся часть консоли. Учесть, что фар может
	//    выполнять некоторые команды сам и курсор вообще-то мог несколько уехать...


	WARNING("Попытаться подобрать те строки, которые НЕ нужно выводить в консоль");
	// из-за прокрутки консоли самим фаром, некоторые строки могли уехать вверх.


	CONSOLE_SCREEN_BUFFER_INFO storedSbi = pData->info;

	SHORT nStoredHeight = min(storedSbi.srWindow.Top,rcBottom.Top);
	if (nStoredHeight < 1)
	{
		// Nothing to restore?
		return;
	}

	COORD crOldBufSize = pData->info.dwSize; // Может быть шире или уже чем текущая консоль!
	SMALL_RECT rcWrite = {0,rcBottom.Top-nStoredHeight,min(crOldBufSize.X,lsbi.dwSize.X)-1,rcBottom.Top-1};
	COORD crBufPos = {0, storedSbi.srWindow.Top-nStoredHeight};

	MyWriteConsoleOutput(ghConOut, pData->Data, crOldBufSize, crBufPos, rcWrite);
#endif
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

void SetConEmuWindows(HWND hDcWnd, HWND hBackWnd)
{
	_ASSERTE(ghConEmuWnd!=NULL || (hDcWnd==NULL && hBackWnd==NULL));
	ghConEmuWndDC = hDcWnd;
	ghConEmuWndBack = hBackWnd;

	wchar_t szHWND[16];

	if (ghConEmuWndDC)
	{
		_wsprintf(szHWND, SKIPLEN(countof(szHWND)) L"0x%08X", (DWORD)(DWORD_PTR)ghConEmuWndDC);
		SetEnvironmentVariableW(ENV_CONEMUDRAW_VAR_W, szHWND);
	}
	else
	{
		SetEnvironmentVariableW(ENV_CONEMUDRAW_VAR_W, NULL);
	}

	if (ghConEmuWndBack)
	{
		_wsprintf(szHWND, SKIPLEN(countof(szHWND)) L"0x%08X", (DWORD)(DWORD_PTR)ghConEmuWndBack);
		SetEnvironmentVariableW(ENV_CONEMUBACK_VAR_W, szHWND);
	}
	else
	{
		SetEnvironmentVariableW(ENV_CONEMUBACK_VAR_W, NULL);
	}
}

void CheckConEmuHwnd()
{
	WARNING("Подозрение, что слишком много вызовов при старте сервера");

	//HWND hWndFore = GetForegroundWindow();
	//HWND hWndFocus = GetFocus();
	DWORD dwGuiThreadId = 0;

	if (gpSrv->bDebuggerActive)
	{
		HWND  hDcWnd = NULL;
		ghConEmuWnd = FindConEmuByPID();

		if (ghConEmuWnd)
		{
			GetWindowThreadProcessId(ghConEmuWnd, &gpSrv->dwGuiPID);
			// Просто для информации
			hDcWnd = FindWindowEx(ghConEmuWnd, NULL, VirtualConsoleClass, NULL);
		}
		else
		{
			hDcWnd = NULL;
		}

		UNREFERENCED_PARAMETER(hDcWnd);

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
			HWND hBack = NULL, hDc = NULL;
			wchar_t szClass[128];
			while (!ghConEmuWndDC)
			{
				hBack = FindWindowEx(ghConEmuWnd, hBack, VirtualConsoleClassBack, NULL);
				if (!hBack)
					break;
				if (GetWindowLong(hBack, 0) == (LONG)(DWORD)ghConWnd)
				{
					hDc = (HWND)(DWORD)GetWindowLong(hBack, 4);
					if (IsWindow(hDc) && GetClassName(hDc, szClass, countof(szClass) && !lstrcmp(szClass, VirtualConsoleClass)))
					{
						SetConEmuWindows(hDc, hBack);
						break;
					}
				}
			}
			_ASSERTE(ghConEmuWndDC!=NULL);
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

bool TryConnect2Gui(HWND hGui, HWND& hDcWnd, CESERVER_REQ* pIn)
{
	bool bConnected = false;
	DWORD nDupErrCode = 0;

	_ASSERTE(pIn && pIn->hdr.nCmd==CECMD_ATTACH2GUI);

	//if (lbNeedSetFont) {
	//	lbNeedSetFont = FALSE;
	//
	//    if (gpLogSize) LogSize(NULL, ":SetConsoleFontSizeTo.before");
	//    SetConsoleFontSizeTo(ghConWnd, gpSrv->nConFontHeight, gpSrv->nConFontWidth, gpSrv->szConsoleFont);
	//    if (gpLogSize) LogSize(NULL, ":SetConsoleFontSizeTo.after");
	//}
	// Если GUI запущен не от имени админа - то он обломается при попытке
	// открыть дескриптор процесса сервера. Нужно будет ему помочь.
	pIn->StartStop.hServerProcessHandle = NULL;

	if (pIn->StartStop.bUserIsAdmin || gbAttachMode)
	{
		DWORD  nGuiPid = 0;

		if (GetWindowThreadProcessId(hGui, &nGuiPid) && nGuiPid)
		{
			// Issue 791: Fails, when GUI started under different credentials (login) as server
			HANDLE hGuiHandle = OpenProcess(PROCESS_DUP_HANDLE, FALSE, nGuiPid);

			if (!hGuiHandle)
			{
				nDupErrCode = GetLastError();
				_ASSERTE((hGuiHandle!=NULL) && "Failed to transfer server process handle to GUI");
			}
			else
			{
				HANDLE hDupHandle = NULL;

				if (DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(),
				                   hGuiHandle, &hDupHandle, MY_PROCESS_ALL_ACCESS/*PROCESS_QUERY_INFORMATION|SYNCHRONIZE*/,
				                   FALSE, 0)
				        && hDupHandle)
				{
					pIn->StartStop.hServerProcessHandle = (u64)hDupHandle;
				}
				else
				{
					nDupErrCode = GetLastError();
					_ASSERTE((hGuiHandle!=NULL) && "Failed to transfer server process handle to GUI");
				}

				CloseHandle(hGuiHandle);
			}
		}
	}

	UNREFERENCED_PARAMETER(nDupErrCode);

	// Execute CECMD_ATTACH2GUI
	wchar_t szPipe[64]; _wsprintf(szPipe, SKIPLEN(countof(szPipe)) CEGUIPIPENAME, L".", (DWORD)hGui); //-V205
	CESERVER_REQ *pOut = ExecuteCmd(szPipe, pIn, GUIATTACH_TIMEOUT, ghConWnd);

	if (!pOut)
	{
		_ASSERTE(pOut!=NULL);
	}
	else
	{
		//ghConEmuWnd = hGui;
		ghConEmuWnd = pOut->StartStopRet.hWnd;
		hDcWnd = pOut->StartStopRet.hWndDc;
		SetConEmuWindows(pOut->StartStopRet.hWndDc, pOut->StartStopRet.hWndBack);
		gpSrv->dwGuiPID = pOut->StartStopRet.dwPID;
		_ASSERTE(gpSrv->pConsoleMap != NULL); // мэппинг уже должен быть создан,
		_ASSERTE(gpSrv->pConsole != NULL); // и локальная копия тоже
		//gpSrv->pConsole->info.nGuiPID = pOut->StartStopRet.dwPID;
		CESERVER_CONSOLE_MAPPING_HDR *pMap = gpSrv->pConsoleMap->Ptr();
		if (pMap)
		{
			pMap->nGuiPID = pOut->StartStopRet.dwPID;
			pMap->hConEmuRoot = ghConEmuWnd;
			pMap->hConEmuWndDc = ghConEmuWndDC;
			pMap->hConEmuWndBack = ghConEmuWndBack;
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
		
		bConnected = true;
	}

	return bConnected;
}

HWND Attach2Gui(DWORD nTimeout)
{
	if (isTerminalMode())
	{
		_ASSERTE(FALSE && "Attach is not allowed in telnet");
		return NULL;
	}

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

	if (gpSrv->bRequestNewGuiWnd && !gpSrv->dwGuiPID && !gpSrv->hGuiWnd)
	{
		bNeedStartGui = TRUE;
		hGui = (HWND)-1;
	}
	else if (gpSrv->dwGuiPID && gpSrv->hGuiWnd)
	{
		wchar_t szClass[128];
		GetClassName(gpSrv->hGuiWnd, szClass, countof(szClass));
		if (lstrcmp(szClass, VirtualConsoleClassMain) == 0)
			hGui = gpSrv->hGuiWnd;
		else
			gpSrv->hGuiWnd = NULL;
	}
	else if (gpSrv->hGuiWnd)
	{
		_ASSERTE(gpSrv->hGuiWnd==NULL);
		gpSrv->hGuiWnd = NULL;
	}

	if (!hGui)
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
						if (lstrcmpiW(prc.szExeFile, L"conemu.exe")==0
							|| lstrcmpiW(prc.szExeFile, L"conemu64.exe")==0)
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
		wchar_t szSelf[MAX_PATH+128];
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

		bool bExeFound = false;

		for (int s = 0; s <= 1; s++)
		{
			if (s)
			{
				// Он может быть на уровень выше
				*pszSlash = 0;
				pszSlash = wcsrchr(pszSelf, L'\\');
				if (!pszSlash)
					break;
			}

			if (IsWindows64())
			{
				lstrcpyW(pszSlash+1, L"ConEmu64.exe");
				if (FileExists(pszSelf))
				{
					bExeFound = true;
					break;
				}
			}

			lstrcpyW(pszSlash+1, L"ConEmu.exe");
			if (FileExists(pszSelf))
			{
				bExeFound = true;
				break;
			}
		}

		if (!bExeFound)
		{
			_printf("ConEmu.exe not found!\n");
			return NULL;
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
		#ifdef _DEBUG
		lstrcatW(pszSelf, L" /nokeyhooks");
		#endif

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
	pIn->StartStop.dwAID = gpSrv->dwGuiAID;
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
		IsNeedCmd(gpszRunCmd, NULL, &lbNeedCutQuot, pIn->StartStop.sModuleName, lbRootIsCmd, lbConfirmExit, lbAutoDisable);
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

	pIn->StartStop.crMaxSize = MyGetLargestConsoleWindowSize(hOut);

//LoopFind:
	// В обычном "серверном" режиме шрифт уже установлен, а при аттаче
	// другого процесса - шрифт все-равно поменять не получится
	//BOOL lbNeedSetFont = TRUE;

	// Если с первого раза не получится (GUI мог еще не загрузиться) пробуем еще
	while (!hDcWnd && dwDelta <= nTimeout)
	{
		if (gpSrv->hGuiWnd)
		{
			if (TryConnect2Gui(gpSrv->hGuiWnd, hDcWnd, pIn) && hDcWnd)
				break; // OK
		}
		else
		{
			HWND hFindGui = NULL;

			while ((hFindGui = FindWindowEx(NULL, hFindGui, VirtualConsoleClassMain, NULL)) != NULL)
			{
				if (TryConnect2Gui(hFindGui, hDcWnd, pIn))
					break; // OK
			}
		}

		if (hDcWnd)
			break;

		dwCur = GetTickCount(); dwDelta = dwCur - dwStart;

		if (dwDelta > nTimeout)
			break;

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
	COORD crMax = MyGetLargestConsoleWindowSize(h);

	if (crMax.X < 80 || crMax.Y < 25)
	{
#ifdef _DEBUG
		DWORD dwErr = GetLastError();
		if (gbIsWine)
		{
			wchar_t szDbgMsg[512], szTitle[128];
			szDbgMsg[0] = 0;
			GetModuleFileName(NULL, szDbgMsg, countof(szDbgMsg));
			msprintf(szTitle, countof(szTitle), L"%s: PID=%u", PointToName(szDbgMsg), GetCurrentProcessId());
			msprintf(szDbgMsg, countof(szDbgMsg), L"GetLargestConsoleWindowSize failed -> {%ix%i}, Code=%u", crMax.X, crMax.Y, dwErr);
			MessageBox(NULL, szDbgMsg, szTitle, MB_SYSTEMMODAL);
		}
		else
		{
			_ASSERTE(crMax.X >= 80 && crMax.Y >= 25);
		}
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
	gpSrv->pConsole->hdr.nLogLevel = (gpLogSize!=NULL) ? 1 : 0;
	gpSrv->pConsole->hdr.crMaxConSize = crMax;
	gpSrv->pConsole->hdr.bDataReady = FALSE;
	gpSrv->pConsole->hdr.hConWnd = ghConWnd; _ASSERTE(ghConWnd!=NULL);
	_ASSERTE(gpSrv->dwMainServerPID!=0);
	gpSrv->pConsole->hdr.nServerPID = gpSrv->dwMainServerPID;
	gpSrv->pConsole->hdr.nAltServerPID = (gnRunMode==RM_ALTSERVER) ? GetCurrentProcessId() : gpSrv->dwAltServerPID;
	gpSrv->pConsole->hdr.nGuiPID = gpSrv->dwGuiPID;
	gpSrv->pConsole->hdr.hConEmuRoot = ghConEmuWnd;
	gpSrv->pConsole->hdr.hConEmuWndDc = ghConEmuWndDC;
	gpSrv->pConsole->hdr.hConEmuWndBack = ghConEmuWndBack;
	_ASSERTE(gpSrv->pConsole->hdr.hConEmuRoot==NULL || gpSrv->pConsole->hdr.nGuiPID!=0);
	//gpSrv->pConsole->hdr.bConsoleActive = TRUE; // пока - TRUE (это на старте сервера)
	gpSrv->pConsole->hdr.nServerInShutdown = 0;
	//gpSrv->pConsole->hdr.bThawRefreshThread = TRUE; // пока - TRUE (это на старте сервера)
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

	BOOL lbCreated;
	if (gnRunMode == RM_SERVER)
		lbCreated = (gpSrv->pConsoleMap->Create() != NULL);
	else
		lbCreated = (gpSrv->pConsoleMap->Open() != NULL);

	if (!lbCreated)
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
	WARNING("***ALT*** не нужно обновлять мэппинг одновременно и в сервере и в альт.сервере");

	if (gpSrv && gpSrv->pConsole)
	{
		if (gnRunMode == RM_SERVER) // !!! RM_ALTSERVER - ниже !!!
		{
			if (ghConEmuWndDC && (!gpSrv->pColorerMapping || (gpSrv->pConsole->hdr.hConEmuWndDc != ghConEmuWndDC)))
			{
				bool bRecreate = (gpSrv->pColorerMapping && (gpSrv->pConsole->hdr.hConEmuWndDc != ghConEmuWndDC));
				CreateColorerHeader(bRecreate);
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
		gpSrv->pConsole->hdr.hConEmuWndDc = ghConEmuWndDC;
		gpSrv->pConsole->hdr.hConEmuWndBack = ghConEmuWndBack;
		_ASSERTE(gpSrv->pConsole->hdr.hConEmuRoot==NULL || gpSrv->pConsole->hdr.nGuiPID!=0);
		gpSrv->pConsole->hdr.nActiveFarPID = gpSrv->nActiveFarPID;

		// Нельзя альт.серверу мэппинг менять - подерутся
		if (gnRunMode != RM_SERVER /*&& gnRunMode != RM_ALTSERVER*/)
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

int CreateColorerHeader(bool bForceRecreate /*= false*/)
{
	if (!gpSrv)
	{
		_ASSERTE(gpSrv!=NULL);
		return -1;
	}

	int iRc = -1;
	DWORD dwErr = 0;
	HWND lhConWnd = NULL;
	const AnnotationHeader* pHdr = NULL;

	EnterCriticalSection(&gpSrv->csColorerMappingCreate);

	_ASSERTE(gpSrv->pColorerMapping == NULL);

	if (bForceRecreate)
	{
		if (gpSrv->pColorerMapping)
		{
			// По идее, не должно быть пересоздания TrueColor мэппинга
			_ASSERTE(FALSE && "Recreating pColorerMapping?");
			delete gpSrv->pColorerMapping;
			gpSrv->pColorerMapping = NULL;
		}
		else
		{
			// Если уж был запрос на пересоздание - должно быть уже создано
			_ASSERTE(gpSrv->pColorerMapping!=NULL);
		}
	}
	else if (gpSrv->pColorerMapping != NULL)
	{
		_ASSERTE(FALSE && "pColorerMapping was already created");
		iRc = 0;
		goto wrap;
	}

	// 111101 - было "GetConEmuHWND(2)", но GetConsoleWindow теперь перехватывается.
	lhConWnd = ghConEmuWndDC; // GetConEmuHWND(2);

	if (!lhConWnd)
	{
		_ASSERTE(lhConWnd != NULL);
		dwErr = GetLastError();
		_printf("Can't create console data file mapping. ConEmu DC window is NULL.\n");
		//iRc = CERR_COLORERMAPPINGERR; -- ошибка не критическая и не обрабатывается
		iRc = 0;
		goto wrap;
	}

	//COORD crMaxSize = MyGetLargestConsoleWindowSize(GetStdHandle(STD_OUTPUT_HANDLE));
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
	pHdr = gpSrv->pColorerMapping->Open();

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

wrap:
	LeaveCriticalSection(&gpSrv->csColorerMappingCreate);
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






static int ReadConsoleInfo()
{
	//int liRc = 1;
	BOOL lbChanged = gpSrv->pConsole->bDataChanged; // Если что-то еще не отослали - сразу TRUE
	//CONSOLE_SELECTION_INFO lsel = {0}; // GetConsoleSelectionInfo
	CONSOLE_CURSOR_INFO lci = {0}; // GetConsoleCursorInfo
	DWORD ldwConsoleCP=0, ldwConsoleOutputCP=0, ldwConsoleMode=0;
	CONSOLE_SCREEN_BUFFER_INFO lsbi = {{0,0}}; // MyGetConsoleScreenBufferInfo
	HANDLE hOut = (HANDLE)ghConOut;
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	//HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	//DWORD nConInMode = 0;

	if (hOut == INVALID_HANDLE_VALUE)
		hOut = hStdOut;

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
		DWORD dwErr = GetLastError();
		_ASSERTE(FALSE && "ReadConsole::MyGetConsoleScreenBufferInfo failed");

		gpSrv->dwSbiRc = dwErr; if (!gpSrv->dwSbiRc) gpSrv->dwSbiRc = -1;

		//liRc = -1;
		return -1;
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

			if (gpLogSize) LogSize(NULL, ":ReadConsoleInfo");

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
	return lbChanged ? 1 : 0;
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
	COORD bufSize; //, bufCoord;
	SMALL_RECT rgn;
	DWORD nCurSize, nHdrSize;
	// -- начинаем потихоньку горизонтальную прокрутку
	_ASSERTE(gpSrv->sbi.srWindow.Left == 0); // этот пока оставим
	//_ASSERTE(gpSrv->sbi.srWindow.Right == (gpSrv->sbi.dwSize.X - 1));
	DWORD nCurScroll = (gnBufferHeight ? rbs_Vert : 0) | (gnBufferWidth ? rbs_Horz : 0);
	DWORD nNewScroll = 0;
	int TextWidth = 0, TextHeight = 0;
	BOOL bSuccess = ::GetConWindowSize(gpSrv->sbi, gcrVisibleSize.X, gcrVisibleSize.Y, nCurScroll, &TextWidth, &TextHeight, &nNewScroll);
	UNREFERENCED_PARAMETER(bSuccess);
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

	//if (nCurSize <= MAX_CONREAD_SIZE)
	{
		bufSize.X = TextWidth; bufSize.Y = TextHeight;
		//bufCoord.X = 0; bufCoord.Y = 0;
		//rgn = gpSrv->sbi.srWindow;

		//if (ReadConsoleOutput(hOut, gpSrv->pConsoleDataCopy, bufSize, bufCoord, &rgn))
		if (MyReadConsoleOutput(hOut, gpSrv->pConsoleDataCopy, bufSize, rgn))
			lbRc = TRUE;
	}

	//if (!lbRc)
	//{
	//	// Придется читать построчно
	//	bufSize.X = TextWidth; bufSize.Y = 1;
	//	bufCoord.X = 0; bufCoord.Y = 0;
	//	//rgn = gpSrv->sbi.srWindow;
	//	CHAR_INFO* pLine = gpSrv->pConsoleDataCopy;

	//	for(int y = 0; y < (int)TextHeight; y++, rgn.Top++, pLine+=TextWidth)
	//	{
	//		rgn.Bottom = rgn.Top;
	//		ReadConsoleOutput(hOut, pLine, bufSize, bufCoord, &rgn);
	//	}
	//}

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

#ifdef USE_COMMIT_EVENT
	if (gpSrv->hExtConsoleCommit)
	{
		WaitForSingleObject(gpSrv->hExtConsoleCommit, EXTCONCOMMIT_TIMEOUT);
	}
#endif

	if (abForceSend)
		gpSrv->pConsole->bDataChanged = TRUE;

	int iInfoRc = ReadConsoleInfo();
	if (iInfoRc == -1)
	{
		lbChanged = FALSE;
	}
	else
	{
		if (iInfoRc == 1)
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
	}

	return lbChanged;
}






DWORD WINAPI RefreshThread(LPVOID lpvParam)
{
	DWORD nWait = 0, nAltWait = 0, nFreezeWait = 0;
	
	HANDLE hEvents[4] = {ghQuitEvent, gpSrv->hRefreshEvent};
	DWORD  nEventsBaseCount = 2;
	DWORD  nRefreshEventId = (WAIT_OBJECT_0+1);

	DWORD nDelta = 0;
	DWORD nLastReadTick = 0; //GetTickCount();
	DWORD nLastConHandleTick = GetTickCount();
	BOOL  /*lbEventualChange = FALSE,*/ /*lbForceSend = FALSE,*/ lbChanged = FALSE; //, lbProcessChanged = FALSE;
	DWORD dwTimeout = 10; // периодичность чтения информации об окне (размеров, курсора,...)
	DWORD dwAltTimeout = 100;
	//BOOL  bForceRefreshSetSize = FALSE; // После изменения размера нужно сразу перечитать консоль без задержек
	BOOL lbWasSizeChange = FALSE;
	//BOOL bThaw = TRUE; // Если FALSE - снизить нагрузку на conhost
	BOOL bFellInSleep = FALSE; // Если TRUE - снизить нагрузку на conhost
	BOOL bConsoleActive = TRUE;
	BOOL bConsoleVisible = TRUE;
	DWORD nLastConsoleActiveTick = 0;
	BOOL bOnlyCursorChanged;
	BOOL bSetRefreshDoneEvent;
	DWORD nWaitCursor = 99;
	DWORD nWaitCommit = 99;

	while (TRUE)
	{
		bOnlyCursorChanged = FALSE;
		bSetRefreshDoneEvent = FALSE;
		nWaitCursor = 99;
		nWaitCommit = 99;

		nWait = WAIT_TIMEOUT;
		//lbForceSend = FALSE;
		MCHKHEAP;

		if (gpSrv->hFreezeRefreshThread)
		{
			HANDLE hFreeze[2] = {gpSrv->hFreezeRefreshThread, ghQuitEvent};
			nFreezeWait = WaitForMultipleObjects(countof(hFreeze), hFreeze, FALSE, INFINITE);
			if (nFreezeWait == (WAIT_OBJECT_0+1))
				break; // затребовано заверешение потока
		}

		// проверка альтернативного сервера
		if (gpSrv->hAltServer)
		{
			if (!gpSrv->hAltServerChanged)
				gpSrv->hAltServerChanged = CreateEvent(NULL, FALSE, FALSE, NULL);

			HANDLE hAltWait[3] = {gpSrv->hAltServer, gpSrv->hAltServerChanged, ghQuitEvent};
			nAltWait = WaitForMultipleObjects(countof(hAltWait), hAltWait, FALSE, dwAltTimeout);

			if ((nAltWait == (WAIT_OBJECT_0+0)) || (nAltWait == (WAIT_OBJECT_0+1)))
			{
				// Если это закрылся AltServer
				if ((nAltWait == (WAIT_OBJECT_0+0)))
				{
					MSectionLock CsAlt; CsAlt.Lock(gpSrv->csAltSrv, TRUE, 10000);

					if (gpSrv->hAltServer)
					{
						HANDLE h = gpSrv->hAltServer;
						gpSrv->hAltServer = NULL;
						gpSrv->hCloseAltServer = h;

						DWORD nAltServerWasStarted = 0;
						DWORD nAltServerWasStopped = gpSrv->dwAltServerPID;
						AltServerInfo info = {};
						if (gpSrv->dwAltServerPID)
						{
							// Поскольку текущий сервер завершается - то сразу сбросим PID (его морозить уже не нужно)
							gpSrv->dwAltServerPID = 0;
							// Был "предыдущий" альт.сервер?
							if (gpSrv->AltServers.Get(nAltServerWasStopped, &info, true/*Remove*/))
							{
								// Переключаемся на "старый" (если был)
								if (info.nPrevPID)
								{
									_ASSERTE(info.hPrev!=NULL);
									// Перевести нить монитора в обычный режим, закрыть gpSrv->hAltServer
									// Активировать альтернативный сервер (повторно), отпустить его нити чтения
									nAltServerWasStarted = info.nPrevPID;
									AltServerWasStarted(info.nPrevPID, info.hPrev, true);
								}
							}
						}

						CsAlt.Unlock();

						// Уведомить ГУЙ
						CESERVER_REQ *pGuiIn = NULL, *pGuiOut = NULL;
						int nSize = sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_STARTSTOP);
						pGuiIn = ExecuteNewCmd(CECMD_CMDSTARTSTOP, nSize);

						if (!pGuiIn)
						{
							_ASSERTE(pGuiIn!=NULL && "Memory allocation failed");
						}
						else
						{
							pGuiIn->StartStop.dwPID = nAltServerWasStarted ? nAltServerWasStarted : nAltServerWasStopped;
							pGuiIn->StartStop.hServerProcessHandle = 0; // для GUI смысла не имеет
							pGuiIn->StartStop.nStarted = nAltServerWasStarted ? sst_AltServerStart : sst_AltServerStop;

							pGuiOut = ExecuteGuiCmd(ghConWnd, pGuiIn, ghConWnd);

							_ASSERTE(pGuiOut!=NULL && "Can not switch GUI to alt server?"); // успешное выполнение?
							ExecuteFreeResult(pGuiIn);
							ExecuteFreeResult(pGuiOut);
						}
					}
				}

				// Смена сервера
				nAltWait = WAIT_OBJECT_0;
			}
			else if (nAltWait == (WAIT_OBJECT_0+2))
			{
				// затребовано заверешение потока
				break;
			}
			#ifdef _DEBUG
			else
			{
            	// Неожиданный результат WaitForMultipleObjects
				_ASSERTE(nAltWait==WAIT_OBJECT_0 || nAltWait==WAIT_TIMEOUT);
			}
			#endif
		}
		else
		{
			nAltWait = WAIT_OBJECT_0;
		}

		if (gpSrv->hCloseAltServer)
		{
			// Чтобы не подраться между потоками - закрывать хэндл только здесь
			if (gpSrv->hCloseAltServer != gpSrv->hAltServer)
			{
				SafeCloseHandle(gpSrv->hCloseAltServer);
			}
			else
			{
				gpSrv->hCloseAltServer = NULL;
			}
		}

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
		{
			nWait = nRefreshEventId; // требуется перечитать консоль после изменения размера!
			bSetRefreshDoneEvent = TRUE;
		}
		else
		{
			DWORD nEvtCount = nEventsBaseCount;
			DWORD nWaitTimeout = dwTimeout;
			DWORD nFarCommit = 99;
			DWORD nCursorChanged = 99;

			if (gpSrv->bFarCommitRegistered)
			{
				nFarCommit = nEvtCount;
				hEvents[nEvtCount++] = gpSrv->hFarCommitEvent;
				nWaitTimeout = 2500; // No need to force console scanning, Far & ExtendedConsole.dll takes care
			}
			if (gpSrv->bCursorChangeRegistered)
			{
				nCursorChanged = nEvtCount;
				hEvents[nEvtCount++] = gpSrv->hCursorChangeEvent;
			}

			nWait = WaitForMultipleObjects(nEvtCount, hEvents, FALSE, nWaitTimeout);
			
			if (nWait == nFarCommit || nWait == nCursorChanged)
			{
				if (nWait == nFarCommit)
				{
					// После Commit вызванного из Far может быть изменена позиция курсора. Подождем чуть-чуть.
					if (gpSrv->bCursorChangeRegistered)
					{
						nWaitCursor = WaitForSingleObject(gpSrv->hCursorChangeEvent, 10);
					}
				}
				else if (gpSrv->bFarCommitRegistered)
				{
					nWaitCommit = WaitForSingleObject(gpSrv->hFarCommitEvent, 0);
				}

				if (gpSrv->bFarCommitRegistered && (nWait == nCursorChanged))
				{
					TODO("Можно выполнить облегченное чтение консоли");
					bOnlyCursorChanged = TRUE;
				}

				nWait = nRefreshEventId;
			}
			else
			{
				bSetRefreshDoneEvent = (nWait == nRefreshEventId);

				// Для информации и для гарантированного сброса событий
				
				if (gpSrv->bCursorChangeRegistered)
				{
					nWaitCursor = WaitForSingleObject(gpSrv->hCursorChangeEvent, 0);
				}

				if (gpSrv->bFarCommitRegistered)
				{
					nWaitCommit = WaitForSingleObject(gpSrv->hFarCommitEvent, 0);
				}
			}
			UNREFERENCED_PARAMETER(nWaitCursor);
		}

		if (nWait == WAIT_OBJECT_0)
		{
			break; // затребовано завершение нити
		}// else if (nWait == WAIT_TIMEOUT && dwConWait == WAIT_OBJECT_0) {

		//	nWait = (WAIT_OBJECT_0+1);
		//}

		//lbEventualChange = (nWait == (WAIT_OBJECT_0+1))/* || lbProcessChanged*/;
		//lbForceSend = (nWait == (WAIT_OBJECT_0+1));

		//WARNING("gpSrv->pConsole->hdr.bConsoleActive и gpSrv->pConsole->hdr.bThawRefreshThread могут быть неактуальными!");
		//if (gpSrv->pConsole->hdr.bConsoleActive && gpSrv->pConsoleMap)
		//{
		//BOOL bNewThaw = TRUE;
		BOOL bNewActive = TRUE;
		BOOL bNewFellInSleep = FALSE;

		if (!nLastConsoleActiveTick || ((GetTickCount() - nLastConsoleActiveTick) >= REFRESH_FELL_SLEEP_TIMEOUT))
		{
			ReloadGuiSettings(NULL);
			bConsoleVisible = IsWindowVisible(ghConEmuWndDC);
		}

		if ((ghConWnd == gpSrv->guiSettings.hActiveCon) || (gpSrv->guiSettings.hActiveCon == NULL) || bConsoleVisible)
			bNewActive = gpSrv->guiSettings.bGuiActive || !gpSrv->guiSettings.bSleepInBackg;
		else
			bNewActive = FALSE;

		bNewFellInSleep = gpSrv->guiSettings.bSleepInBackg && !bNewActive;

		//if (gpSrv->pConsoleMap->IsValid())
		//{
		//	CESERVER_CONSOLE_MAPPING_HDR* p = gpSrv->pConsoleMap->Ptr();
		//	bNewThaw = p->bThawRefreshThread;
		//	bConsoleActive = p->bConsoleActive;
		//}
		//else
		//{
		//	bNewThaw = bConsoleActive = TRUE;
		//}
		////}

		//if (bNewThaw != bThaw)
		if ((bNewActive != bConsoleActive) || (bNewFellInSleep != bFellInSleep))
		{
			//bThaw = bNewThaw;
			bConsoleActive = bNewActive;
			bFellInSleep = bNewFellInSleep;

			if (gpLogSize)
			{
				char szInfo[128];
				_wsprintfA(szInfo, SKIPLEN(countof(szInfo)) "ConEmuC: RefreshThread: Sleep changed, speed(%s)", bNewFellInSleep ? "high" : "low");
				LogString(szInfo);
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
		            || bFellInSleep)
		        // и не дернули событие gpSrv->hRefreshEvent
		        && (nWait != nRefreshEventId)
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
		if (nWait == nRefreshEventId)
		{
			DEBUGSTR(L"*** hRefreshEvent was set, checking console...\n");
		}
		#endif

		if (ghConEmuWnd && !IsWindow(ghConEmuWnd))
		{
			gpSrv->bWasDetached = TRUE;
			ghConEmuWnd = NULL;
			SetConEmuWindows(NULL, NULL);
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
			bool lbReloadNow = true;
			#if defined(TEST_REFRESH_DELAYED)
			static DWORD nDbgTick = 0;
			const DWORD nMax = 1000;
			HANDLE hOut = (HANDLE)ghConOut;
			DWORD nWaitOut = WaitForSingleObject(hOut, 0);
			DWORD nCurTick = GetTickCount();
			if ((nWaitOut == WAIT_OBJECT_0) || (nCurTick - nDbgTick) >= nMax)
			{
				nDbgTick = nCurTick;
			}
			else
			{
				lbReloadNow = false;
				lbChanged = FALSE;
			}
			//ShutdownSrvStep(L"ReloadFullConsoleInfo begin");
			#endif
			
			if (lbReloadNow)
			{
				lbChanged = ReloadFullConsoleInfo(gpSrv->bWasReattached/*lbForceSend*/);
			}

			#if defined(TEST_REFRESH_DELAYED)
			//ShutdownSrvStep(L"ReloadFullConsoleInfo end (%u,%u)", (int)lbReloadNow, (int)lbChanged);
			#endif

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

		if (bSetRefreshDoneEvent)
		{
			SetEvent(gpSrv->hRefreshDoneEvent);
		}

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
