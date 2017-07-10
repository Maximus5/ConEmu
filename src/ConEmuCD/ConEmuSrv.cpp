
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

#define SHOWDEBUGSTR

#undef TEST_REFRESH_DELAYED

#include "ConEmuC.h"
#include "../common/CmdLine.h"
#include "../common/ConsoleAnnotation.h"
#include "../common/ConsoleRead.h"
#include "../common/EmergencyShow.h"
#include "../common/execute.h"
#include "../common/MProcess.h"
#include "../common/MProcessBits.h"
#include "../common/MRect.h"
#include "../common/MSectionSimple.h"
#include "../common/MStrDup.h"
#include "../common/MStrSafe.h"
#include "../common/ProcessSetEnv.h"
#include "../common/SetEnvVar.h"
#include "../common/StartupEnvDef.h"
#include "../common/WConsole.h"
#include "../common/WConsoleInfo.h"
#include "../common/WFiles.h"
#include "../common/WThreads.h"
#include "../common/WUser.h"
//#include "TokenHelper.h"
#include "ConProcess.h"
#include "SrvPipes.h"
#include "Queue.h"

//#define DEBUGSTRINPUTEVENT(s) //DEBUGSTR(s) // SetEvent(gpSrv->hInputEvent)
//#define DEBUGLOGINPUT(s) DEBUGSTR(s) // ConEmuC.MouseEvent(X=
//#define DEBUGSTRINPUTWRITE(s) DEBUGSTR(s) // *** ConEmuC.MouseEvent(X=
//#define DEBUGSTRINPUTWRITEALL(s) //DEBUGSTR(s) // *** WriteConsoleInput(Write=
//#define DEBUGSTRINPUTWRITEFAIL(s) DEBUGSTR(s) // ### WriteConsoleInput(Write=
//#define DEBUGSTRCHANGES(s) DEBUGSTR(s)

#ifdef _DEBUG
	//#define DEBUG_SLEEP_NUMLCK
	#undef DEBUG_SLEEP_NUMLCK
#else
	#undef DEBUG_SLEEP_NUMLCK
#endif

#define MAX_EVENTS_PACK 20

#ifdef _DEBUG
//#define ASSERT_UNWANTED_SIZE
#else
#undef ASSERT_UNWANTED_SIZE
#endif

extern BOOL gbTerminateOnExit; // для отладчика
extern BOOL gbTerminateOnCtrlBreak;
extern OSVERSIONINFO gOSVer;

// Some forward definitions
bool TryConnect2Gui(HWND hGui, DWORD anGuiPID, CESERVER_REQ* pIn);


// Установить мелкий шрифт, иначе может быть невозможно увеличение размера GUI окна
void ServerInitFont()
{
	LogFunction(L"ServerInitFont");

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
	else if ((!gbAlienMode || gOSVer.dwMajorVersion >= 6) && !gpStartEnv->bIsReactOS)
	{
		if (gpLogSize) LogSize(NULL, 0, ":SetConsoleFontSizeTo.before");

		#ifdef _DEBUG
		if (gpSrv->nConFontHeight >= 10)
			g_IgnoreSetLargeFont = true;
		#endif

		SetConsoleFontSizeTo(ghConWnd, gpSrv->nConFontHeight, gpSrv->nConFontWidth, gpSrv->szConsoleFont, gnDefTextColors, gnDefPopupColors);

		if (gpLogSize)
		{
			int curSizeY = -1, curSizeX = -1;
			wchar_t sFontName[LF_FACESIZE] = L"";
			if (apiGetConsoleFontSize(ghConOut, curSizeY, curSizeX, sFontName) && curSizeY && curSizeX)
			{
				char szLogInfo[128];
				_wsprintfA(szLogInfo, SKIPLEN(countof(szLogInfo)) "Console font size H=%i W=%i N=", curSizeY, curSizeX);
				int nLen = lstrlenA(szLogInfo);
				WideCharToMultiByte(CP_UTF8, 0, sFontName, -1, szLogInfo+nLen, countof(szLogInfo)-nLen, NULL, NULL);
				LogFunction(szLogInfo);
			}
		}

		if (gpLogSize) LogSize(NULL, 0, ":SetConsoleFontSizeTo.after");
	}
}

LGSResult LoadGuiSettingsPtr(ConEmuGuiMapping& GuiMapping, const ConEmuGuiMapping* pInfo, bool abNeedReload, bool abForceCopy, DWORD& rnWrongValue)
{
	LGSResult liRc = lgs_Failed;
	DWORD cbSize = 0;
	bool lbNeedCopy = false;
	bool lbCopied = false;
	wchar_t szLog[80];

	if (!pInfo)
	{
		liRc = lgs_MapPtr;
		wcscpy_c(szLog, L"LoadGuiSettings(Failed, MapPtr is null)");
		LogFunction(szLog);
		goto wrap;
	}

	if (abForceCopy)
	{
		cbSize = min(sizeof(GuiMapping), pInfo->cbSize);
		memmove(&GuiMapping, pInfo, cbSize);
		gpSrv->guiSettings.cbSize = cbSize;
		lbCopied = true;
	}

	if (pInfo->cbSize >= (size_t)(sizeof(pInfo->nProtocolVersion) + ((LPBYTE)&pInfo->nProtocolVersion) - (LPBYTE)pInfo))
	{
		if (pInfo->nProtocolVersion != CESERVER_REQ_VER)
		{
			liRc = lgs_WrongVersion;
			rnWrongValue = pInfo->nProtocolVersion;
			wcscpy_c(szLog, L"LoadGuiSettings(Failed, MapPtr is null)");
			_wsprintf(szLog, SKIPCOUNT(szLog) L"LoadGuiSettings(Failed, Version=%u, Required=%u)", rnWrongValue, (DWORD)CESERVER_REQ_VER);
			LogFunction(szLog);
			goto wrap;
		}
	}

	if (pInfo->cbSize != sizeof(ConEmuGuiMapping))
	{
		liRc = lgs_WrongSize;
		rnWrongValue = pInfo->cbSize;
		_wsprintf(szLog, SKIPCOUNT(szLog) L"LoadGuiSettings(Failed, cbSize=%u, Required=%u)", pInfo->cbSize, (DWORD)sizeof(ConEmuGuiMapping));
		LogFunction(szLog);
		goto wrap;
	}

	lbNeedCopy = abNeedReload
		|| (gpSrv->guiSettingsChangeNum != pInfo->nChangeNum)
		|| (GuiMapping.bGuiActive != pInfo->bGuiActive)
		;

	if (lbNeedCopy)
	{
		wcscpy_c(szLog, L"LoadGuiSettings(Changed)");
		LogFunction(szLog);
		if (!lbCopied)
			memmove(&GuiMapping, pInfo, pInfo->cbSize);
		_ASSERTE(GuiMapping.ComSpec.ConEmuExeDir[0]!=0 && GuiMapping.ComSpec.ConEmuBaseDir[0]!=0);
		liRc = lgs_Updated;
	}
	else if (GuiMapping.dwActiveTick != pInfo->dwActiveTick)
	{
		// But active consoles list may be changed
		if (!lbCopied)
			memmove(GuiMapping.Consoles, pInfo->Consoles, sizeof(GuiMapping.Consoles));
		liRc = lgs_ActiveChanged;
	}
	else
	{
		liRc = lgs_Succeeded;
	}

wrap:
	return liRc;
}

LGSResult LoadGuiSettings(ConEmuGuiMapping& GuiMapping, DWORD& rnWrongValue)
{
	LGSResult liRc = lgs_Failed;
	bool lbNeedReload = false;
	DWORD dwGuiThreadId, dwGuiProcessId;
	HWND hGuiWnd = ghConEmuWnd ? ghConEmuWnd : gpSrv->hGuiWnd;
	const ConEmuGuiMapping* pInfo = NULL;

	if (!hGuiWnd || !IsWindow(hGuiWnd))
	{
		LogFunction(L"LoadGuiSettings(Invalid window)");
		goto wrap;
	}

	if (!gpSrv->pGuiInfoMap || (gpSrv->hGuiInfoMapWnd != hGuiWnd))
	{
		lbNeedReload = true;
	}

	if (lbNeedReload)
	{
		LogFunction(L"LoadGuiSettings(Opening)");

		dwGuiThreadId = GetWindowThreadProcessId(hGuiWnd, &dwGuiProcessId);
		if (!dwGuiThreadId)
		{
			_ASSERTE(dwGuiProcessId);
			LogFunction(L"LoadGuiSettings(Failed, dwGuiThreadId==0)");
			goto wrap;
		}

		if (!gpSrv->pGuiInfoMap)
			gpSrv->pGuiInfoMap = new MFileMapping<ConEmuGuiMapping>;
		else
			gpSrv->pGuiInfoMap->CloseMap();

		gpSrv->pGuiInfoMap->InitName(CEGUIINFOMAPNAME, dwGuiProcessId);
		pInfo = gpSrv->pGuiInfoMap->Open();

		if (pInfo)
		{
			gpSrv->hGuiInfoMapWnd = hGuiWnd;
		}
	}
	else
	{
		pInfo = gpSrv->pGuiInfoMap->Ptr();
	}

	liRc = LoadGuiSettingsPtr(GuiMapping, pInfo, lbNeedReload, false, rnWrongValue);
wrap:
	return liRc;
}

LGSResult ReloadGuiSettings(ConEmuGuiMapping* apFromCmd, LPDWORD pnWrongValue /*= NULL*/)
{
	bool lbChanged = false;
	LGSResult lgsResult = lgs_Failed;
	DWORD nWrongValue = 0;

	if (apFromCmd)
	{
		LogFunction(L"ReloadGuiSettings(apFromCmd)");
		lgsResult = LoadGuiSettingsPtr(gpSrv->guiSettings, apFromCmd, false, true, nWrongValue);
		lbChanged = (lgsResult >= lgs_Succeeded);
	}
	else
	{
		gpSrv->guiSettings.cbSize = sizeof(ConEmuGuiMapping);
		lgsResult = LoadGuiSettings(gpSrv->guiSettings, nWrongValue);
		lbChanged = (lgsResult >= lgs_Succeeded)
			&& ((gpSrv->guiSettingsChangeNum != gpSrv->guiSettings.nChangeNum)
				|| (gpSrv->pConsole && gpSrv->pConsole->hdr.ComSpec.ConEmuExeDir[0] == 0));
	}

	if (pnWrongValue)
		*pnWrongValue = nWrongValue;

	if (lbChanged)
	{
		LogFunction(L"ReloadGuiSettings(Apply)");

		gpSrv->guiSettingsChangeNum = gpSrv->guiSettings.nChangeNum;

		gbLogProcess = (gpSrv->guiSettings.nLoggingType == glt_Processes);

		UpdateComspec(&gpSrv->guiSettings.ComSpec); // isAddConEmu2Path, ...

		SetConEmuFolders(gpSrv->guiSettings.ComSpec.ConEmuExeDir, gpSrv->guiSettings.ComSpec.ConEmuBaseDir);

		// Не будем ставить сами, эту переменную заполняет Gui при своем запуске
		// соответственно, переменная наследуется серверами
		//SetEnvironmentVariableW(L"ConEmuArgs", pInfo->sConEmuArgs);

		//wchar_t szHWND[16]; _wsprintf(szHWND, SKIPLEN(countof(szHWND)) L"0x%08X", gpSrv->guiSettings.hGuiWnd.u);
		//SetEnvironmentVariable(ENV_CONEMUHWND_VAR_W, szHWND);
		SetConEmuWindows(gpSrv->guiSettings.hGuiWnd, ghConEmuWndDC, ghConEmuWndBack);

		if (gpSrv->pConsole)
		{
			CopySrvMapFromGuiMap();

			UpdateConsoleMapHeader(L"guiSettings were changed");
		}
	}

	return lgsResult;
}

// AutoAttach делать нельзя, когда ConEmu запускает процесс обновления
bool IsAutoAttachAllowed()
{
	if (!ghConWnd)
		return false;

	if (gbAttachMode & am_Admin)
		return true;

	if (!IsWindowVisible(ghConWnd))
		return (gbDefTermCall || gbAttachFromFar);

	if (IsIconic(ghConWnd))
		return false;

	return true;
}

void WaitForServerActivated(DWORD anServerPID, HANDLE ahServer, DWORD nTimeout = 30000)
{
	if (!gpSrv || !gpSrv->pConsoleMap || !gpSrv->pConsoleMap->IsValid())
	{
		_ASSERTE(FALSE && "ConsoleMap was not initialized!");
		Sleep(nTimeout);
		return;
	}

	HWND hDcWnd = NULL;
	DWORD nStartTick = GetTickCount(), nDelta = 0, nWait = STILL_ACTIVE, nSrvPID = 0, nExitCode = STILL_ACTIVE;
	while (nDelta <= nTimeout)
	{
		nWait = WaitForSingleObject(ahServer, 100);
		if (nWait == WAIT_OBJECT_0)
		{
			// Server was terminated unexpectedly?
			if (!GetExitCodeProcess(ahServer, &nExitCode))
				nExitCode = E_UNEXPECTED;
			break;
		}

		nSrvPID = gpSrv->pConsoleMap->Ptr()->nServerPID;
		_ASSERTE(((nSrvPID == 0) || (nSrvPID == anServerPID)) && (nSrvPID != GetCurrentProcessId()));

		hDcWnd = gpSrv->pConsoleMap->Ptr()->hConEmuWndDc;

		// Well, server was started and attached to ConEmu
		if (nSrvPID && hDcWnd)
		{
			break;
		}

		nDelta = (GetTickCount() - nStartTick);
	}

	// Wait a little more, to be sure Server loads process tree
	Sleep(1000);
	UNREFERENCED_PARAMETER(nExitCode);
}

// Вызывается при запуске сервера: (gbNoCreateProcess && (gbAttachMode || gpSrv->DbgInfo.bDebuggerActive))
int AttachRootProcess()
{
	LogFunction(L"AttachRootProcess");

	DWORD dwErr = 0;

	_ASSERTE((gpSrv->hRootProcess == NULL || gpSrv->hRootProcess == GetCurrentProcess()) && "Must not be opened yet");

	if (!gpSrv->DbgInfo.bDebuggerActive && !IsAutoAttachAllowed() && !(gnConEmuPID || gbAttachFromFar))
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

	// "/AUTOATTACH" must be asynchronous
	if ((gbAttachMode & am_Async) || (gpSrv->dwRootProcess == 0 && !gpSrv->DbgInfo.bDebuggerActive))
	{
		// Нужно попытаться определить PID корневого процесса.
		// Родительским может быть cmd (comspec, запущенный из FAR)
		DWORD dwParentPID = 0, dwFarPID = 0;
		DWORD dwServerPID = 0; // Вдруг в этой консоли уже есть сервер?
		_ASSERTE(!gpSrv->DbgInfo.bDebuggerActive);

		if (gpSrv->nProcessCount >= 2 && !gpSrv->DbgInfo.bDebuggerActive)
		{
			//TODO: Reuse MToolHelp.h
			HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);

			if (hSnap != INVALID_HANDLE_VALUE)
			{
				PROCESSENTRY32 prc = {sizeof(PROCESSENTRY32)};

				if (Process32First(hSnap, &prc))
				{
					do
					{
						for (UINT i = 0; i < gpSrv->nProcessCount; i++)
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
					while (Process32Next(hSnap, &prc));
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

		int nParentBitness = GetProcessBits(gpSrv->dwRootProcess, gpSrv->hRootProcess);

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

		if (nParentBitness && (nParentBitness != WIN3264TEST(32,64)))
		{
			wchar_t* pszName = (wchar_t*)PointToName(szSelf);
			*pszName = 0;
			wcscat_c(szSelf, (nParentBitness==32) ? L"ConEmuC.exe" : L"ConEmuC64.exe");
		}

		//if (wcschr(pszSelf, L' '))
		//{
		//	*(--pszSelf) = L'"';
		//	lstrcatW(pszSelf, L"\"");
		//}

		wchar_t szGuiWnd[32];
		if (gpSrv->bRequestNewGuiWnd)
			wcscpy_c(szGuiWnd, L"/GHWND=NEW");
		else if (gpSrv->hGuiWnd)
			_wsprintf(szGuiWnd, SKIPLEN(countof(szGuiWnd)) L"/GHWND=%08X", (DWORD)(DWORD_PTR)gpSrv->hGuiWnd);
		else
			szGuiWnd[0] = 0;

		_wsprintf(szCommand, SKIPLEN(countof(szCommand)) L"\"%s\" %s /ATTACH /PID=%u", szSelf, szGuiWnd, dwParentPID);

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

		WaitForServerActivated(pi.dwProcessId, pi.hProcess, 30000);

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
		int iAttachRc = AttachRootProcessHandle();
		if (iAttachRc != 0)
			return iAttachRc;
	}

	return 0; // OK
}

BOOL ServerInitConsoleMode()
{
	LogFunction(L"ServerInitConsoleMode");

	BOOL bConRc = FALSE;

	HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
	DWORD dwFlags = 0;
	bConRc = GetConsoleMode(h, &dwFlags);
	LogModeChange(L"[GetConInMode]", 0, dwFlags);

	// This can be passed with "/CINMODE=..." or "-cur_console:w" switches
	if (!gnConsoleModeFlags)
	{
		// Умолчание (параметр /CINMODE= не указан)
		// DON'T turn on ENABLE_QUICK_EDIT_MODE by default, let console applications "use" mouse
		dwFlags |= (ENABLE_EXTENDED_FLAGS|ENABLE_INSERT_MODE);
	}
	else
	{
		DWORD nMask = (gnConsoleModeFlags & 0xFFFF0000) >> 16;
		DWORD nOr   = (gnConsoleModeFlags & 0xFFFF);
		dwFlags &= ~nMask;
		dwFlags |= (nOr | ENABLE_EXTENDED_FLAGS);
	}

	bConRc = SetConsoleMode(h, dwFlags); //-V519
	LogModeChange(L"[SetConInMode]", 0, dwFlags);

	return bConRc;
}

int ServerInitCheckExisting(bool abAlternative)
{
	LogFunction(L"ServerInitCheckExisting");

	int iRc = 0;
	CESERVER_CONSOLE_MAPPING_HDR test = {};

	BOOL lbExist = LoadSrvMapping(ghConWnd, test);
	_ASSERTE(!lbExist || (test.ComSpec.ConEmuExeDir[0] && test.ComSpec.ConEmuBaseDir[0]));

	if (!abAlternative)
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
	LogFunction(L"ServerInitConsoleSize");

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

			gcrVisibleSize.X = lsbi.srWindow.Right - lsbi.srWindow.Left + 1;
			gcrVisibleSize.Y = lsbi.srWindow.Bottom - lsbi.srWindow.Top + 1;
			gnBufferHeight = (lsbi.dwSize.Y == gcrVisibleSize.Y) ? 0 : lsbi.dwSize.Y;
			gnBufferWidth = (lsbi.dwSize.X == gcrVisibleSize.X) ? 0 : lsbi.dwSize.X;
		}
	}

	return 0;
}

int ServerInitAttach2Gui()
{
	LogFunction(L"ServerInitAttach2Gui");

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

		wchar_t szTitle[128];
		_wsprintf(szTitle, SKIPCOUNT(szTitle) WIN3264TEST(L"ConEmuC",L"ConEmuC64") L" PID=%u", GetCurrentProcessId());
		if (MessageBox(NULL, L"Available ConEmu GUI window not found!", szTitle,
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
		gbInShutdown = TRUE;
		DisableAutoConfirmExit();
		iRc = CERR_ATTACHFAILED; goto wrap;
	}

wrap:
	return iRc;
}

// Дернуть ConEmu, чтобы он отдал HWND окна отрисовки
// (!gbAttachMode && !gpSrv->DbgInfo.bDebuggerActive)
int ServerInitGuiTab()
{
	LogFunction(L"ServerInitGuiTab");

	int iRc = CERR_ATTACH_NO_GUIWND;
	DWORD nWaitRc = 99;
	HWND hGuiWnd = FindConEmuByPID();

	if (hGuiWnd == NULL)
	{
		if (gnRunMode == RM_SERVER || gnRunMode == RM_ALTSERVER)
		{
			// Если запускается сервер - то он должен смочь найти окно ConEmu в которое его просят
			_ASSERTEX((hGuiWnd!=NULL));
			_ASSERTE(iRc == CERR_ATTACH_NO_GUIWND);
			goto wrap;
		}
		else
		{
			_ASSERTEX(gnRunMode == RM_SERVER || gnRunMode == RM_ALTSERVER);
		}
	}
	else
	{
		_ASSERTE(ghConWnd!=NULL);

		CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_SRVSTARTSTOP, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_SRVSTARTSTOP));
		if (!pIn)
		{
			_ASSERTEX(pIn);
			iRc = CERR_NOTENOUGHMEM1;
			goto wrap;
		}
		pIn->SrvStartStop.Started = srv_Started; // сервер запущен
		pIn->SrvStartStop.hConWnd = ghConWnd;
		// Сразу передать текущий KeyboardLayout
		IsKeyboardLayoutChanged(&pIn->SrvStartStop.dwKeybLayout);

		if (TryConnect2Gui(hGuiWnd, 0, pIn))
		{
			iRc = 0;
		}
		else
		{
			ASSERTE(FALSE && "TryConnect2Gui failed");
		}

		ExecuteFreeResult(pIn);
	}

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
wrap:
	return iRc;
}

bool AltServerWasStarted(DWORD nPID, HANDLE hAltServer, bool ForceThaw)
{
	wchar_t szFnArg[200];
	_wsprintf(szFnArg, SKIPCOUNT(szFnArg) L"AltServerWasStarted PID=%u H=x%p ForceThaw=%s ",
		nPID, hAltServer, ForceThaw ? L"true" : L"false");
	if (gpLogSize)
	{
		PROCESSENTRY32 AltSrv;
		if (GetProcessInfo(nPID, &AltSrv))
		{
			int iLen = lstrlen(szFnArg);
			lstrcpyn(szFnArg+iLen, PointToName(AltSrv.szExeFile), countof(szFnArg)-iLen);
		}
	}
	LogFunction(szFnArg);

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

		// Если для nPID не было назначено "предыдущего" альт.сервера
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


void ServerInitEnvVars()
{
	LogFunction(L"ServerInitEnvVars");

	wchar_t szValue[32];
	//DWORD nRc;
	//nRc = GetEnvironmentVariable(ENV_CONEMU_HOOKS, szValue, countof(szValue));
	//if ((nRc == 0) && (GetLastError() == ERROR_ENVVAR_NOT_FOUND)) ...

	SetEnvironmentVariable(ENV_CONEMU_HOOKS_W, ENV_CONEMU_HOOKS_ENABLED);

	if (gnRunMode == RM_SERVER)
	{
		_wsprintf(szValue, SKIPLEN(countof(szValue)) L"%u", GetCurrentProcessId());
		SetEnvironmentVariable(ENV_CONEMUSERVERPID_VAR_W, szValue);
	}

	if (gpSrv && (gpSrv->guiSettings.cbSize == sizeof(gpSrv->guiSettings)))
	{
		SetConEmuFolders(gpSrv->guiSettings.ComSpec.ConEmuExeDir, gpSrv->guiSettings.ComSpec.ConEmuBaseDir);

		// Не будем ставить сами, эту переменную заполняет Gui при своем запуске
		// соответственно, переменная наследуется серверами
		//SetEnvironmentVariableW(L"ConEmuArgs", pInfo->sConEmuArgs);

		//wchar_t szHWND[16]; _wsprintf(szHWND, SKIPLEN(countof(szHWND)) L"0x%08X", gpSrv->guiSettings.hGuiWnd.u);
		//SetEnvironmentVariable(ENV_CONEMUHWND_VAR_W, szHWND);
		SetConEmuWindows(gpSrv->guiSettings.hGuiWnd, ghConEmuWndDC, ghConEmuWndBack);

		#ifdef _DEBUG
		bool bNewConArg = ((gpSrv->guiSettings.Flags & CECF_ProcessNewCon) != 0);
		//SetEnvironmentVariable(ENV_CONEMU_HOOKS, bNewConArg ? ENV_CONEMU_HOOKS_ENABLED : ENV_CONEMU_HOOKS_NOARGS);
		#endif

		bool bAnsi = ((gpSrv->guiSettings.Flags & CECF_ProcessAnsi) != 0);
		SetEnvironmentVariable(ENV_CONEMUANSI_VAR_W, bAnsi ? L"ON" : L"OFF");

		if (bAnsi)
		{
			wchar_t szInfo[40];
			HANDLE hOut = (HANDLE)ghConOut;
			CONSOLE_SCREEN_BUFFER_INFO lsbi = {{0,0}}; // интересует реальное положение дел
			GetConsoleScreenBufferInfo(hOut, &lsbi);

			msprintf(szInfo, countof(szInfo), L"%ux%u (%ux%u)", lsbi.dwSize.X, lsbi.dwSize.Y, lsbi.srWindow.Right-lsbi.srWindow.Left+1, lsbi.srWindow.Bottom-lsbi.srWindow.Top+1);
			SetEnvironmentVariable(ENV_ANSICON_VAR_W, szInfo);

			//static SHORT Con2Ansi[16] = {0,4,2,6,1,5,3,7,8|0,8|4,8|2,8|6,8|1,8|5,8|3,8|7};
			//DWORD clrDefault = Con2Ansi[CONFORECOLOR(lsbi.wAttributes)]
			//	| (Con2Ansi[CONBACKCOLOR(lsbi.wAttributes)] << 4);
			msprintf(szInfo, countof(szInfo), L"%X", LOBYTE(lsbi.wAttributes));
			SetEnvironmentVariable(ENV_ANSICON_DEF_VAR_W, szInfo);
		}
		else
		{
			SetEnvironmentVariable(ENV_ANSICON_VAR_W, NULL);
			SetEnvironmentVariable(ENV_ANSICON_DEF_VAR_W, NULL);
		}
		SetEnvironmentVariable(ENV_ANSICON_VER_VAR_W, NULL);
	}
	else
	{
		//_ASSERTE(gpSrv && (gpSrv->guiSettings.cbSize == sizeof(gpSrv->guiSettings)));
	}
}


// Создать необходимые события и нити
int ServerInit()
{
	LogFunction(L"ServerInit");

	int iRc = 0;
	DWORD dwErr = 0;
	wchar_t szName[64];
	DWORD nTick = GetTickCount();

	if (gbDumpServerInitStatus) { _printf("ServerInit: started"); }
	#define DumpInitStatus(fmt) if (gbDumpServerInitStatus) { DWORD nCurTick = GetTickCount(); _printf(" - %ums" fmt, (nCurTick-nTick)); nTick = nCurTick; }

	if (gnRunMode == RM_SERVER)
	{
		_ASSERTE(!(gbAttachMode & am_Async));

		gpSrv->dwMainServerPID = GetCurrentProcessId();
		gpSrv->hMainServer = GetCurrentProcess();

		_ASSERTE(gnRunMode==RM_SERVER);

		if (gbIsDBCS)
		{
			UINT nOemCP = GetOEMCP();
			UINT nConCP = GetConsoleOutputCP();
			if (nConCP != nOemCP)
			{
				DumpInitStatus("\nServerInit: CreateThread(SetOemCpProc)");
				DWORD nTID;
				HANDLE h = apiCreateThread(SetOemCpProc, (LPVOID)nOemCP, &nTID, "SetOemCpProc");
				if (h && (h != INVALID_HANDLE_VALUE))
				{
					DWORD nWait = WaitForSingleObject(h, 5000);
					if (nWait != WAIT_OBJECT_0)
					{
						_ASSERTE(nWait==WAIT_OBJECT_0 && "SetOemCpProc HUNGS!!!");
						apiTerminateThread(h, 100);
					}
					CloseHandle(h);
				}
			}
		}

		//// Set up some environment variables
		//DumpInitStatus("\nServerInit: ServerInitEnvVars");
		//ServerInitEnvVars();
	}


	gpSrv->osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetOsVersionInformational(&gpSrv->osv);

	// Смысла вроде не имеет, без ожидания "очистки" очереди винда "проглатывает мышиные события
	// Межпроцессный семафор не помогает, оставил пока только в качестве заглушки
	//InitializeConsoleInputSemaphore();

	if (gpSrv->osv.dwMajorVersion == 6 && gpSrv->osv.dwMinorVersion == 1)
		gpSrv->bReopenHandleAllowed = FALSE;
	else
		gpSrv->bReopenHandleAllowed = TRUE;

	if (gnRunMode == RM_SERVER)
	{
		if (!gnConfirmExitParm)
		{
			gbAlwaysConfirmExit = TRUE; gbAutoDisableConfirmExit = TRUE;
		}
	}
	else
	{
		_ASSERTE(gnRunMode==RM_ALTSERVER || gnRunMode==RM_AUTOATTACH);
		// По идее, включены быть не должны, но убедимся
		_ASSERTE(!gbAlwaysConfirmExit);
		gbAutoDisableConfirmExit = FALSE; gbAlwaysConfirmExit = FALSE;
	}

	// Remember RealConsole font at the startup moment
	if (gnRunMode == RM_ALTSERVER)
	{
		apiInitConsoleFontSize(ghConOut);
	}

	// Шрифт в консоли нужно менять в самом начале, иначе могут быть проблемы с установкой размера консоли
	if ((gnRunMode == RM_SERVER) && !gpSrv->DbgInfo.bDebuggerActive && !gbNoCreateProcess)
		//&& (!gbNoCreateProcess || (gbAttachMode && gbNoCreateProcess && gpSrv->dwRootProcess))
		//)
	{
		//DumpInitStatus("\nServerInit: ServerInitFont");
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
			//DumpInitStatus("\nServerInit: Hiding console");
			apiShowWindow(ghConWnd, SW_HIDE);
			//if (bMovedBottom)
			//{
			//	SetWindowPos(ghConWnd, HWND_TOP, 0, 0, 0,0, SWP_NOSIZE|SWP_NOMOVE);
			//}
		}

		//DumpInitStatus("\nServerInit: Set console window pos {0,0}");
		// -- чтобы на некоторых системах не возникала проблема с позиционированием -> {0,0}
		// Issue 274: Окно реальной консоли позиционируется в неудобном месте
		SetWindowPos(ghConWnd, NULL, 0, 0, 0,0, SWP_NOSIZE|SWP_NOZORDER);
	}

	// Не будем, наверное. OnTop попытается поставить сервер,
	// при показе консоли через Ctrl+Win+Alt+Space
	// Но вот если консоль уже видима, и это "/root", тогда
	// попытаемся поставить окну консоли флаг "OnTop"
	if (!gbNoCreateProcess && !gbIsWine)
	{
		//if (!gbVisibleOnStartup)
		//	apiShowWindow(ghConWnd, SW_HIDE);
		//DumpInitStatus("\nServerInit: Set console window TOP_MOST");
		SetWindowPos(ghConWnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
	}
	//if (!gbVisibleOnStartup && IsWindowVisible(ghConWnd))
	//{
	//	apiShowWindow(ghConWnd, SW_HIDE);
	//}

	// Подготовить буфер для длинного вывода
	// RM_SERVER - создать и считать текущее содержимое консоли
	// RM_ALTSERVER - только создать (по факту - выполняется открытие созданного в RM_SERVER)
	if (gnRunMode==RM_SERVER || gnRunMode==RM_ALTSERVER)
	{
		DumpInitStatus("\nServerInit: CmdOutputStore");
		CmdOutputStore(true/*abCreateOnly*/);
	}
	else
	{
		_ASSERTE(gnRunMode==RM_AUTOATTACH);
	}
	#if 0
	_ASSERTE(gpcsStoredOutput==NULL && gpStoredOutput==NULL);
	if (!gpcsStoredOutput)
	{
		gpcsStoredOutput = new MSection;
	}
	#endif


	// Включить по умолчанию выделение мышью
	if ((gnRunMode == RM_SERVER) && !gbNoCreateProcess && gbConsoleModeFlags /*&& !(gbParmBufferSize && gnBufferHeight == 0)*/)
	{
		//DumpInitStatus("\nServerInit: ServerInitConsoleMode");
		ServerInitConsoleMode();
	}

	//2009-08-27 Перенес снизу
	if (!gpSrv->hConEmuGuiAttached && (!gpSrv->DbgInfo.bDebugProcess || gnConEmuPID || gpSrv->hGuiWnd))
	{
		wchar_t szTempName[MAX_PATH];
		_wsprintf(szTempName, SKIPLEN(countof(szTempName)) CEGUIRCONSTARTED, LODWORD(ghConWnd)); //-V205

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

	if (ghConEmuWndDC == NULL)
	{
		// в AltServer режиме HWND уже должен быть известен
		_ASSERTE((gnRunMode == RM_SERVER) || (gnRunMode == RM_AUTOATTACH) || (ghConEmuWndDC != NULL));
	}
	else
	{
		DumpInitStatus("\nServerInit: ServerInitCheckExisting");
		iRc = ServerInitCheckExisting((gnRunMode != RM_SERVER));
		if (iRc != 0)
			goto wrap;
	}

	// Создать MapFile для заголовка (СРАЗУ!!!) и буфера для чтения и сравнения
	//DumpInitStatus("\nServerInit: CreateMapHeader");
	iRc = CreateMapHeader();

	if (iRc != 0)
		goto wrap;

	_ASSERTE((ghConEmuWndDC==NULL) || (gpSrv->pColorerMapping!=NULL));

	if ((gnRunMode == RM_ALTSERVER) && gpSrv->pConsole && (gpSrv->pConsole->hdr.Flags & CECF_ConExcHandler))
	{
		SetupCreateDumpOnException();
	}

	gpSrv->csAltSrv = new MSection();
	gpSrv->csProc = new MSection();
	gpSrv->nMainTimerElapse = 10;
	gpSrv->TopLeft.Reset(); // блокировка прокрутки не включена
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

	//DumpInitStatus("\nServerInit: CheckProcessCount");
	CheckProcessCount(TRUE); // Сначала добавит себя

	// в принципе, серверный режим может быть вызван из фара, чтобы подцепиться к GUI.
	// больше двух процессов в консоли вполне может быть, например, еще не отвалился
	// предыдущий conemuc.exe, из которого этот запущен немодально.
	_ASSERTE(gpSrv->DbgInfo.bDebuggerActive || (gpSrv->nProcessCount<=2) || ((gpSrv->nProcessCount>2) && gbAttachMode && gpSrv->dwRootProcess));

	// Запустить нить обработки событий (клавиатура, мышь, и пр.)
	if (gnRunMode == RM_SERVER)
	{
		gpSrv->hInputEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
		gpSrv->hInputWasRead = CreateEvent(NULL,FALSE,FALSE,NULL);

		if (gpSrv->hInputEvent && gpSrv->hInputWasRead)
		{
			DumpInitStatus("\nServerInit: CreateThread(InputThread)");
			gpSrv->hInputThread = apiCreateThread(InputThread, NULL, &gpSrv->dwInputThread, "InputThread");

		}

		if (gpSrv->hInputEvent == NULL || gpSrv->hInputWasRead == NULL || gpSrv->hInputThread == NULL)
		{
			dwErr = GetLastError();
			_printf("CreateThread(InputThread) failed, ErrCode=0x%08X\n", dwErr);
			iRc = CERR_CREATEINPUTTHREAD; goto wrap;
		}

		SetThreadPriority(gpSrv->hInputThread, THREAD_PRIORITY_ABOVE_NORMAL);

		DumpInitStatus("\nServerInit: InputQueue.Initialize");
		gpSrv->InputQueue.Initialize(CE_MAX_INPUT_QUEUE_BUFFER, gpSrv->hInputEvent);

		// Запустить пайп обработки событий (клавиатура, мышь, и пр.)
		DumpInitStatus("\nServerInit: InputServerStart");
		if (!InputServerStart())
		{
			dwErr = GetLastError();
			_printf("CreateThread(InputServerStart) failed, ErrCode=0x%08X\n", dwErr);
			iRc = CERR_CREATEINPUTTHREAD; goto wrap;
		}
	}

	// Пайп возврата содержимого консоли
	if ((gnRunMode == RM_SERVER) || (gnRunMode == RM_ALTSERVER))
	{
		DumpInitStatus("\nServerInit: DataServerStart");
		if (!DataServerStart())
		{
			dwErr = GetLastError();
			_printf("CreateThread(DataServerStart) failed, ErrCode=0x%08X\n", dwErr);
			iRc = CERR_CREATEINPUTTHREAD; goto wrap;
		}
	}


	// Проверка. Для дебаггера должен быть RM_UNDEFINED!
	// И он не должен быть "сервером" - работает как обычное приложение!
	_ASSERTE(!(gpSrv->DbgInfo.bDebuggerActive || gpSrv->DbgInfo.bDebugProcess || gpSrv->DbgInfo.bDebugProcessTree));

	if (!gbAttachMode && !gpSrv->DbgInfo.bDebuggerActive)
	{
		DumpInitStatus("\nServerInit: ServerInitGuiTab");
		iRc = ServerInitGuiTab();
		if (iRc != 0)
			goto wrap;
	}

	if ((gnRunMode == RM_SERVER) && (gbAttachMode & ~am_Async) && !(gbAttachMode & am_Async))
	{
		DumpInitStatus("\nServerInit: ServerInitAttach2Gui");
		iRc = ServerInitAttach2Gui();
		if (iRc != 0)
			goto wrap;
	}

	// Ensure the console has proper size before further steps (echo for example)
	ServerInitConsoleSize();

	// Ensure that "set" commands in the command line will override ConEmu's default environment (settings page)
	// This function also process all other "configuration" and "output" commands like 'echo', 'type', 'chcp' etc.
	ApplyProcessSetEnvCmd();

	// Если "корневой" процесс консоли запущен не нами (аттач или дебаг)
	// то нужно к нему "подцепиться" (открыть HANDLE процесса)
	if (gbNoCreateProcess && (gbAttachMode || (gpSrv->DbgInfo.bDebuggerActive && (gpSrv->hRootProcess == NULL))))
	{
		DumpInitStatus("\nServerInit: AttachRootProcess");
		iRc = AttachRootProcess();
		if (iRc != 0)
			goto wrap;

		if (gbAttachMode & am_Async)
		{
			_ASSERTE(FALSE && "Not expected to be here!");
			iRc = CERR_CARGUMENT;
			goto wrap;
		}
	}


	gpSrv->hAllowInputEvent = CreateEvent(NULL, TRUE, TRUE, NULL);

	if (!gpSrv->hAllowInputEvent) SetEvent(gpSrv->hAllowInputEvent);



	_ASSERTE(gpSrv->pConsole!=NULL);
	//gpSrv->pConsole->hdr.bConsoleActive = TRUE;
	//gpSrv->pConsole->hdr.bThawRefreshThread = TRUE;

	//// Minimized окошко нужно развернуть!
	//if (IsIconic(ghConWnd))
	//{
	//	WINDOWPLACEMENT wplCon = {sizeof(wplCon)};
	//	GetWindowPlacement(ghConWnd, &wplCon);
	//	wplCon.showCmd = SW_RESTORE;
	//	SetWindowPlacement(ghConWnd, &wplCon);
	//}

	// Сразу получить текущее состояние консоли
	DumpInitStatus("\nServerInit: ReloadFullConsoleInfo");
	ReloadFullConsoleInfo(TRUE);

	//DumpInitStatus("\nServerInit: Creating events");

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
		// External attach to running process, required ConEmuHk is not loaded yet
		if (!gbAlternativeAttach && gbNoCreateProcess && gbAlienMode && !gbDontInjectConEmuHk)
		{
			if (gpSrv->dwRootProcess)
			{
				DumpInitStatus("\nServerInit: InjectRemote (gbAlienMode)");
				CINFILTRATE_EXIT_CODES iRemote = InjectRemote(gpSrv->dwRootProcess);
				if (iRemote != CIR_OK/*0*/ && iRemote != CIR_AlreadyInjected/*1*/)
				{
					_printf("ServerInit warning: InjectRemote PID=%u failed, Code=%i\n", gpSrv->dwRootProcess, iRemote);
				}
			}
			else
			{
				_printf("ServerInit warning: gpSrv->dwRootProcess==0\n", 0);
			}
		}
	}

	// Запустить нить наблюдения за консолью
	DumpInitStatus("\nServerInit: CreateThread(RefreshThread)");
	gpSrv->hRefreshThread = apiCreateThread(RefreshThread, NULL, &gpSrv->dwRefreshThread, "RefreshThread");
	if (gpSrv->hRefreshThread == NULL)
	{
		dwErr = GetLastError();
		_printf("CreateThread(RefreshThread) failed, ErrCode=0x%08X\n", dwErr);
		iRc = CERR_CREATEREFRESHTHREAD; goto wrap;
	}

	//#ifdef USE_WINEVENT_SRV
	////gpSrv->nMsgHookEnableDisable = RegisterWindowMessage(L"ConEmuC::HookEnableDisable");
	//// The client thread that calls SetWinEventHook must have a message loop in order to receive events.");
	//gpSrv->hWinEventThread = apiCreateThread(NULL, 0, WinEventThread, NULL, 0, &gpSrv->dwWinEventThread);
	//if (gpSrv->hWinEventThread == NULL)
	//{
	//	dwErr = GetLastError();
	//	_printf("CreateThread(WinEventThread) failed, ErrCode=0x%08X\n", dwErr);
	//	iRc = CERR_WINEVENTTHREAD; goto wrap;
	//}
	//#endif

	// Запустить пайп обработки команд
	DumpInitStatus("\nServerInit: CmdServerStart");
	if (!CmdServerStart())
	{
		dwErr = GetLastError();
		_printf("CreateThread(CmdServerStart) failed, ErrCode=0x%08X\n", dwErr);
		iRc = CERR_CREATESERVERTHREAD; goto wrap;
	}

	// Set up some environment variables
	DumpInitStatus("\nServerInit: ServerInitEnvVars");
	ServerInitEnvVars();

	// Пометить мэппинг, как готовый к отдаче данных
	gpSrv->pConsole->hdr.bDataReady = TRUE;

	UpdateConsoleMapHeader(L"ServerInit");

	// Set console title in server mode
	if (gnRunMode == RM_SERVER)
	{
		UpdateConsoleTitle();
	}

	DumpInitStatus("\nServerInit: SendStarted");
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
		_ASSERTE(gpSrv->dwMainServerPID && (gpSrv->dwMainServerPID==GetCurrentProcessId()));
		pIn->AttachGuiApp.nServerPID = gpSrv->dwMainServerPID;
		pIn->AttachGuiApp.hConEmuWnd = ghConEmuWnd;
		pIn->AttachGuiApp.hConEmuDc = ghConEmuWndDC;
		pIn->AttachGuiApp.hConEmuBack = ghConEmuWndBack;
		pIn->AttachGuiApp.hAppWindow = gpSrv->hRootProcessGui;
		pIn->AttachGuiApp.hSrvConWnd = ghConWnd;
		wchar_t szPipe[MAX_PATH];
		_ASSERTE(gpSrv->dwRootProcess!=0);
		_wsprintf(szPipe, SKIPLEN(countof(szPipe)) CEHOOKSPIPENAME, L".", gpSrv->dwRootProcess);
		DumpInitStatus("\nServerInit: CECMD_ATTACHGUIAPP");
		CESERVER_REQ* pOut = ExecuteCmd(szPipe, pIn, GUIATTACH_TIMEOUT, ghConWnd);
		if (!pOut
			|| (pOut->hdr.cbSize < (sizeof(CESERVER_REQ_HDR)+sizeof(DWORD)))
			|| (pOut->dwData[0] != LODWORD(gpSrv->hRootProcessGui)))
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
	DumpInitStatus("\nServerInit: finished\n");
	#undef DumpInitStatus
	return iRc;
}

// Завершить все нити и закрыть дескрипторы
void ServerDone(int aiRc, bool abReportShutdown /*= false*/)
{
	LogFunction(L"ServerDone");

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

			UpdateConsoleMapHeader(L"ServerDone");
		}

		#ifdef _DEBUG
		int nCurProcCount = gpSrv->nProcessCount;
		DWORD nCurProcs[20];
		memmove(nCurProcs, gpSrv->pnProcesses, min(nCurProcCount,20)*sizeof(DWORD));
		_ASSERTE(nCurProcCount <= 1);
		#endif

		wchar_t szServerPipe[MAX_PATH];
		_wsprintf(szServerPipe, SKIPLEN(countof(szServerPipe)) CEGUIPIPENAME, L".", LODWORD(ghConEmuWnd)); //-V205

		CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_SRVSTARTSTOP, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_SRVSTARTSTOP));
		if (pIn)
		{
			pIn->SrvStartStop.Started = srv_Stopped/*101*/;
			pIn->SrvStartStop.hConWnd = ghConWnd;
			pIn->SrvStartStop.nShellExitCode = gnExitCode;
			// Здесь dwKeybLayout уже не интересен

			// Послать в GUI уведомление, что сервер закрывается
			/*CallNamedPipe(szServerPipe, &In, In.hdr.cbSize, &Out, sizeof(Out), &dwRead, 1000);*/
			// 131017 При закрытии не успевает отработать. Серверу нужно дождаться ответа как обычно
			CESERVER_REQ* pOut = ExecuteCmd(szServerPipe, pIn, 1000, ghConWnd, FALSE/*bAsyncNoResult*/);

			ExecuteFreeResult(pIn);
			ExecuteFreeResult(pOut);
		}
	}

	// Our debugger is running?
	if (gpSrv->DbgInfo.bDebuggerActive)
	{
		// pfnDebugActiveProcessStop is useless, because
		// 1. pfnDebugSetProcessKillOnExit was called already
		// 2. we can debug more than a one process

		//gpSrv->DbgInfo.bDebuggerActive = FALSE;
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
			apiTerminateThread(gpSrv->hInputThread, 100);    // раз корректно не хочет...
			#ifndef __GNUC__
			#pragma warning( pop )
			#endif
		}

		SafeCloseHandle(gpSrv->hInputThread);
		//gpSrv->dwInputThread = 0; -- не будем чистить ИД, Для истории
	}

	// Пайп консольного ввода
	if (gpSrv)
		gpSrv->InputServer.StopPipeServer(false, gpSrv->bServerForcedTermination);

	//SafeCloseHandle(gpSrv->hInputPipe);
	SafeCloseHandle(gpSrv->hInputEvent);
	SafeCloseHandle(gpSrv->hInputWasRead);

	if (gpSrv)
		gpSrv->DataServer.StopPipeServer(false, gpSrv->bServerForcedTermination);

	if (gpSrv)
		gpSrv->CmdServer.StopPipeServer(false, gpSrv->bServerForcedTermination);

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
			apiTerminateThread(gpSrv->hRefreshThread, 100);
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
	//Delete Critical Section(&gpSrv->csConBuf);
	//Delete Critical Section(&gpSrv->csChar);
	//Delete Critical Section(&gpSrv->csChangeSize);
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
	MSectionLock RCS;
	if (gpSrv->pReqSizeSection && !RCS.Lock(gpSrv->pReqSizeSection, TRUE, 30000))
	{
		_ASSERTE(FALSE);
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;

	}

	return ReadConsoleOutputEx(hOut, pData, bufSize, rgn);
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

void ConOutCloseHandle()
{
	if (gpSrv->bReopenHandleAllowed)
	{
		// Need to block all requests to output buffer in other threads
		MSectionLockSimple csRead;
		if (csRead.Lock(&gpSrv->csReadConsoleInfo, LOCK_REOPENCONOUT_TIMEOUT))
		{
			ghConOut.Close();
		}
	}
}

bool CmdOutputOpenMap(CONSOLE_SCREEN_BUFFER_INFO& lsbi, CESERVER_CONSAVE_MAPHDR*& pHdr, CESERVER_CONSAVE_MAP*& pData)
{
	LogFunction(L"CmdOutputOpenMap");

	pHdr = NULL;
	pData = NULL;

	// В Win7 закрытие дескриптора в ДРУГОМ процессе - закрывает консольный буфер ПОЛНОСТЬЮ!!!
	// В итоге, буфер вывода telnet'а схлопывается!
	if (gpSrv->bReopenHandleAllowed)
	{
		ConOutCloseHandle();
	}

	// Need to block all requests to output buffer in other threads
	MSectionLockSimple csRead; csRead.Lock(&gpSrv->csReadConsoleInfo, LOCK_READOUTPUT_TIMEOUT);

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
		gpSrv->pStoredOutputHdr->InitName(CECONOUTPUTNAME, LODWORD(ghConWnd)); //-V205
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
			LPCWSTR pszName = gpSrv->pStoredOutputItem->InitName(CECONOUTPUTITEMNAME, LODWORD(ghConWnd), nNewIndex); //-V205
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
	LogFunction(L"CmdOutputStore");

	CONSOLE_SCREEN_BUFFER_INFO lsbi = {{0,0}};
	CESERVER_CONSAVE_MAPHDR* pHdr = NULL;
	CESERVER_CONSAVE_MAP* pData = NULL;

	// Need to block all requests to output buffer in other threads
	MSectionLockSimple csRead; csRead.Lock(&gpSrv->csReadConsoleInfo, LOCK_READOUTPUT_TIMEOUT);

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

	csRead.Unlock();

	LogString("CmdOutputStore finished");
	DEBUGSTR(L"--- CmdOutputStore end\n");
}

// abSimpleMode==true  - просто восстановить экран на момент вызова CmdOutputStore
//             ==false - пытаться подгонять строки вывода под текущее состояние
//                       задел на будущее для выполнения команд из Far (без /w), mc, или еще кого.
void CmdOutputRestore(bool abSimpleMode)
{
	LogFunction(L"CmdOutputRestore");

	if (!abSimpleMode)
	{
		//_ASSERTE(FALSE && "Non Simple mode is not supported!");
		WARNING("Переделать/доделать CmdOutputRestore для Far");
		LogString("--- Skipped, only simple mode supported yet");
		return;
	}


	// Need to block all requests to output buffer in other threads
	MSectionLockSimple csRead; csRead.Lock(&gpSrv->csReadConsoleInfo, LOCK_READOUTPUT_TIMEOUT);

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

#if 0
	// Event if there were no backscroll - we may restore saved content!
	if (lsbi.dwSize.Y <= (lsbi.srWindow.Bottom - lsbi.srWindow.Top + 1))
	{
		// There is no scroller in window
		// Nothing to do
		return;
	}
#endif

	CHAR_INFO chrFill = {};
	chrFill.Attributes = lsbi.wAttributes;
	chrFill.Char.UnicodeChar = L' ';

	SMALL_RECT rcTop = {0,0, lsbi.dwSize.X-1, lsbi.srWindow.Bottom};
	COORD crMoveTo = {0, lsbi.dwSize.Y - 1 - lsbi.srWindow.Bottom};
	if (!ScrollConsoleScreenBuffer(ghConOut, &rcTop, NULL, crMoveTo, &chrFill))
	{
		return;
	}

	short h = lsbi.srWindow.Bottom - lsbi.srWindow.Top + 1;
	short w = lsbi.srWindow.Right - lsbi.srWindow.Left + 1;

	if (abSimpleMode)
	{

		crMoveTo.Y = min(pData->info.srWindow.Top,max(0,lsbi.dwSize.Y-h));
	}

	SMALL_RECT rcBottom = {0, crMoveTo.Y, w - 1, crMoveTo.Y + h - 1};
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
	COORD crOldBufSize = pData->info.dwSize; // Может быть шире или уже чем текущая консоль!
	SMALL_RECT rcWrite = {0,0,min(crOldBufSize.X,lsbi.dwSize.X)-1,min(crOldBufSize.Y,lsbi.dwSize.Y)-1};
	COORD crBufPos = {0,0};

	if (!abSimpleMode)
	{
		// Что восстанавливать - при выполнении команд из фара - нужно
		// восстановить только область выше первой видимой строки,
		// т.к. видимую область фар восстанавливает сам
		SHORT nStoredHeight = min(storedSbi.srWindow.Top,rcBottom.Top);
		if (nStoredHeight < 1)
		{
			// Nothing to restore?
			return;
		}

		rcWrite.Top = rcBottom.Top-nStoredHeight;
		rcWrite.Right = min(crOldBufSize.X,lsbi.dwSize.X)-1;
		rcWrite.Bottom = rcBottom.Top-1;

		crBufPos.Y = storedSbi.srWindow.Top-nStoredHeight;
	}
	else
	{
		// А в "простом" режиме - тупо восстановить консоль как она была на момент сохранения!
	}

	MyWriteConsoleOutput(ghConOut, pData->Data, crOldBufSize, crBufPos, rcWrite);

	if (abSimpleMode)
	{
		SetConsoleTextAttribute(ghConOut, pData->info.wAttributes);
	}

	LogString("CmdOutputRestore finished");
}

static BOOL CALLBACK FindConEmuByPidProc(HWND hwnd, LPARAM lParam)
{
	DWORD dwPID;
	GetWindowThreadProcessId(hwnd, &dwPID);
	if (dwPID == gnConEmuPID)
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

HWND FindConEmuByPID(DWORD anSuggestedGuiPID /*= 0*/)
{
	LogFunction(L"FindConEmuByPID");

	HWND hConEmuWnd = NULL;
	DWORD nConEmuPID = anSuggestedGuiPID ? anSuggestedGuiPID : gnConEmuPID;
	DWORD dwGuiThreadId = 0, dwGuiProcessId = 0;

	// В большинстве случаев PID GUI передан через параметры
	if (nConEmuPID == 0)
	{
		// GUI может еще "висеть" в ожидании или в отладчике, так что пробуем и через Snapshot
		//TODO: Reuse MToolHelp.h
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
						nConEmuPID = prc.th32ParentProcessID;
						break;
					}
				}
				while (Process32Next(hSnap, &prc));
			}

			CloseHandle(hSnap);
		}
	}

	if (nConEmuPID)
	{
		HWND hGui = NULL;

		while ((hGui = FindWindowEx(NULL, hGui, VirtualConsoleClassMain, NULL)) != NULL)
		{
			dwGuiThreadId = GetWindowThreadProcessId(hGui, &dwGuiProcessId);

			if (dwGuiProcessId == nConEmuPID)
			{
				hConEmuWnd = hGui;
				break;
			}
		}

		// Если "в лоб" по имени класса ничего не нашли - смотрим
		// среди всех дочерних для текущего десктопа
		if ((hConEmuWnd == NULL) && !anSuggestedGuiPID)
		{
			HWND hDesktop = GetDesktopWindow();
			EnumChildWindows(hDesktop, FindConEmuByPidProc, (LPARAM)&hConEmuWnd);
		}
	}

	// Ensure that returned hConEmuWnd match gnConEmuPID
	if (!anSuggestedGuiPID && hConEmuWnd)
	{
		GetWindowThreadProcessId(hConEmuWnd, &gnConEmuPID);
	}

	return hConEmuWnd;
}

void SetConEmuFolders(LPCWSTR asExeDir, LPCWSTR asBaseDir)
{
	_ASSERTE(asExeDir && *asExeDir!=0 && asBaseDir && *asBaseDir);
	SetEnvironmentVariable(ENV_CONEMUDIR_VAR_W, asExeDir);
	SetEnvironmentVariable(ENV_CONEMUBASEDIR_VAR_W, asBaseDir);
	CEStr BaseShort(GetShortFileNameEx(asBaseDir, false));
	SetEnvironmentVariable(ENV_CONEMUBASEDIRSHORT_VAR_W, BaseShort.IsEmpty() ? asBaseDir : BaseShort.ms_Val);
}

void SetConEmuWindows(HWND hRootWnd, HWND hDcWnd, HWND hBackWnd)
{
	LogFunction(L"SetConEmuWindows");

	char szInfo[100] = "";

	// Main ConEmu window
	if (hRootWnd && !IsWindow(hRootWnd))
	{
		_ASSERTE(FALSE && "hRootWnd is invalid");
		hRootWnd = NULL;
	}
	// Changed?
	if (ghConEmuWnd != hRootWnd)
	{
		_wsprintfA(szInfo+lstrlenA(szInfo), SKIPLEN(30) "ConEmuWnd=x%08X ", (DWORD)(DWORD_PTR)hRootWnd);
		ghConEmuWnd = hRootWnd;

		// Than check GuiPID
		DWORD nGuiPid = 0;
		if (GetWindowThreadProcessId(hRootWnd, &nGuiPid) && nGuiPid)
		{
			gnConEmuPID = nGuiPid;
		}
	}
	// Do AllowSetForegroundWindow
	if (hRootWnd)
	{
		DWORD dwGuiThreadId = GetWindowThreadProcessId(hRootWnd, &gnConEmuPID);
		AllowSetForegroundWindow(gnConEmuPID);
	}
	// "ConEmuHWND"="0x%08X", "ConEmuPID"="%u"
	SetConEmuEnvVar(ghConEmuWnd);

	// Drawing canvas & Background windows
	_ASSERTE(ghConEmuWnd!=NULL || (hDcWnd==NULL && hBackWnd==NULL));
	if (hDcWnd && !IsWindow(hDcWnd))
	{
		_ASSERTE(FALSE && "hDcWnd is invalid");
		hDcWnd = NULL;
	}
	if (hBackWnd && !IsWindow(hBackWnd))
	{
		_ASSERTE(FALSE && "hBackWnd is invalid");
		hBackWnd = NULL;
	}
	// Set new descriptors
	if ((ghConEmuWndDC != hDcWnd) || (ghConEmuWndBack != hBackWnd))
	{
		_wsprintfA(szInfo+lstrlenA(szInfo), SKIPLEN(60) "WndDC=x%08X WndBack=x%08X", (DWORD)(DWORD_PTR)hDcWnd, (DWORD)(DWORD_PTR)hBackWnd);
		ghConEmuWndDC = hDcWnd;
		ghConEmuWndBack = hBackWnd;
		// "ConEmuDrawHWND"="0x%08X", "ConEmuBackHWND"="0x%08X"
		SetConEmuEnvVarChild(hDcWnd, hBackWnd);
	}

	if (gpLogSize && *szInfo)
	{
		LogFunction(szInfo);
	}
}

void CheckConEmuHwnd()
{
	LogFunction(L"CheckConEmuHwnd");

	WARNING("Подозрение, что слишком много вызовов при старте сервера");

	//HWND hWndFore = GetForegroundWindow();
	//HWND hWndFocus = GetFocus();
	DWORD dwGuiThreadId = 0;

	if (gpSrv->DbgInfo.bDebuggerActive)
	{
		HWND  hDcWnd = NULL;
		ghConEmuWnd = FindConEmuByPID();

		if (ghConEmuWnd)
		{
			GetWindowThreadProcessId(ghConEmuWnd, &gnConEmuPID);
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

	// GUI может еще "висеть" в ожидании или в отладчике, так что пробуем и через Snapshot
	if (ghConEmuWnd == NULL)
	{
		ghConEmuWnd = FindConEmuByPID();
	}

	if (ghConEmuWnd == NULL)
	{
		// Если уж ничего не помогло...
		LogFunction(L"GetConEmuHWND");
		ghConEmuWnd = GetConEmuHWND(1/*Gui Main window*/);
	}

	if (ghConEmuWnd)
	{
		if (!ghConEmuWndDC)
		{
			// ghConEmuWndDC по идее уже должен быть получен из GUI через пайпы
			LogFunction(L"Warning, ghConEmuWndDC still not initialized");
			_ASSERTE(ghConEmuWndDC!=NULL);
			HWND hBack = NULL, hDc = NULL;
			wchar_t szClass[128];
			while (!ghConEmuWndDC)
			{
				hBack = FindWindowEx(ghConEmuWnd, hBack, VirtualConsoleClassBack, NULL);
				if (!hBack)
					break;
				if (GetWindowLong(hBack, 0) == LOLONG(ghConWnd))
				{
					hDc = (HWND)(DWORD)GetWindowLong(hBack, 4);
					if (IsWindow(hDc) && GetClassName(hDc, szClass, countof(szClass) && !lstrcmp(szClass, VirtualConsoleClass)))
					{
						SetConEmuWindows(ghConEmuWnd, hDc, hBack);
						break;
					}
				}
			}
			_ASSERTE(ghConEmuWndDC!=NULL);
		}

		// Установить переменную среды с дескриптором окна
		SetConEmuWindows(ghConEmuWnd, ghConEmuWndDC, ghConEmuWndBack);

		//if (hWndFore == ghConWnd || hWndFocus == ghConWnd)
		//if (hWndFore != ghConEmuWnd)

		if (GetForegroundWindow() == ghConWnd)
			apiSetForegroundWindow(ghConEmuWnd); // 2009-09-14 почему-то было было ghConWnd ?
	}
	else
	{
		// да и фиг с ним. нас могли просто так, без gui запустить
		//_ASSERTE(ghConEmuWnd!=NULL);
	}
}

void FixConsoleMappingHdr(CESERVER_CONSOLE_MAPPING_HDR *pMap)
{
	pMap->nGuiPID = gnConEmuPID;
	pMap->hConEmuRoot = ghConEmuWnd;
	pMap->hConEmuWndDc = ghConEmuWndDC;
	pMap->hConEmuWndBack = ghConEmuWndBack;
}

bool TryConnect2Gui(HWND hGui, DWORD anGuiPID, CESERVER_REQ* pIn)
{
	LogFunction(L"TryConnect2Gui");

	bool bConnected = false;
	CESERVER_REQ *pOut = NULL;
	CESERVER_REQ_SRVSTARTSTOPRET* pStartStopRet = NULL;

	_ASSERTE(pIn && ((pIn->hdr.nCmd==CECMD_ATTACH2GUI && gbAttachMode) || (pIn->hdr.nCmd==CECMD_SRVSTARTSTOP && !gbAttachMode)));

	ZeroStruct(gpSrv->ConnectInfo);
	gpSrv->ConnectInfo.hGuiWnd = hGui;

	//if (lbNeedSetFont) {
	//	lbNeedSetFont = FALSE;
	//
	//    if (gpLogSize) LogSize(NULL, ":SetConsoleFontSizeTo.before");
	//    SetConsoleFontSizeTo(ghConWnd, gpSrv->nConFontHeight, gpSrv->nConFontWidth, gpSrv->szConsoleFont);
	//    if (gpLogSize) LogSize(NULL, ":SetConsoleFontSizeTo.after");
	//}

	// Если GUI запущен не от имени админа - то он обломается при попытке
	// открыть дескриптор процесса сервера. Нужно будет ему помочь.
	if (pIn->hdr.nCmd == CECMD_ATTACH2GUI)
	{
		// CESERVER_REQ_STARTSTOP::sCmdLine[1] is variable length!
		_ASSERTE(pIn->DataSize() >= sizeof(pIn->StartStop));
		pIn->StartStop.hServerProcessHandle = NULL;

		if (pIn->StartStop.bUserIsAdmin || gbAttachMode)
		{
			DWORD  nGuiPid = 0;

			if ((hGui && GetWindowThreadProcessId(hGui, &nGuiPid) && nGuiPid)
				|| ((nGuiPid = anGuiPID) != 0))
			{
				// Issue 791: Fails, when GUI started under different credentials (login) as server
				HANDLE hGuiHandle = OpenProcess(PROCESS_DUP_HANDLE, FALSE, nGuiPid);

				if (!hGuiHandle)
				{
					gpSrv->ConnectInfo.nDupErrCode = GetLastError();
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
						gpSrv->ConnectInfo.nDupErrCode = GetLastError();
						_ASSERTE((hGuiHandle!=NULL) && "Failed to transfer server process handle to GUI");
					}

					CloseHandle(hGuiHandle);
				}
			}
		}

		// Palette may revive after detach from ConEmu
		MyLoadConsolePalette((HANDLE)ghConOut, pIn->StartStop.Palette);

	}
	else
	{
		_ASSERTE(pIn->hdr.nCmd == CECMD_SRVSTARTSTOP);
		_ASSERTE(pIn->DataSize() == sizeof(pIn->SrvStartStop));
	}

	// Execute CECMD_ATTACH2GUI
	wchar_t szServerPipe[64];
	if (hGui)
	{
		_wsprintf(szServerPipe, SKIPLEN(countof(szServerPipe)) CEGUIPIPENAME, L".", LODWORD(hGui)); //-V205
	}
	else if (anGuiPID)
	{
		_wsprintf(szServerPipe, SKIPLEN(countof(szServerPipe)) CESERVERPIPENAME, L".", anGuiPID);
	}
	else
	{
		_ASSERTEX((hGui!=NULL) || (anGuiPID!=0));
		goto wrap;
	}

	gpSrv->ConnectInfo.nInitTick = GetTickCount();

	while (true)
	{
		gpSrv->ConnectInfo.nStartTick = GetTickCount();

		ExecuteFreeResult(pOut);
		pOut = ExecuteCmd(szServerPipe, pIn, EXECUTE_CONNECT_GUI_CALL_TIMEOUT, ghConWnd);
		gpSrv->ConnectInfo.bCallRc = (pOut->DataSize() >= sizeof(CESERVER_REQ_STARTSTOPRET));

		gpSrv->ConnectInfo.nErr = GetLastError();
		gpSrv->ConnectInfo.nEndTick = GetTickCount();
		gpSrv->ConnectInfo.nConnectDelta = gpSrv->ConnectInfo.nEndTick - gpSrv->ConnectInfo.nInitTick;
		gpSrv->ConnectInfo.nDelta = gpSrv->ConnectInfo.nEndTick - gpSrv->ConnectInfo.nStartTick;

		#ifdef _DEBUG
		if (gpSrv->ConnectInfo.bCallRc && (gpSrv->ConnectInfo.nDelta >= EXECUTE_CMD_WARN_TIMEOUT))
		{
			if (!IsDebuggerPresent())
			{
				//_ASSERTE(gpSrv->ConnectInfo.nDelta <= EXECUTE_CMD_WARN_TIMEOUT || (pIn->hdr.nCmd == CECMD_CMDSTARTSTOP && nDelta <= EXECUTE_CMD_WARN_TIMEOUT2));
				_ASSERTEX(gpSrv->ConnectInfo.nDelta <= EXECUTE_CMD_WARN_TIMEOUT);
			}
		}
		#endif

		if (gpSrv->ConnectInfo.bCallRc || (gpSrv->ConnectInfo.nConnectDelta > EXECUTE_CONNECT_GUI_TIMEOUT))
			break;

		if (!gpSrv->ConnectInfo.bCallRc)
		{
			_ASSERTE(gpSrv->ConnectInfo.bCallRc && (gpSrv->ConnectInfo.nErr==ERROR_FILE_NOT_FOUND) && "GUI was not initialized yet?");
			Sleep(250);
		}
	}

	// Этот блок if-else нужно вынести в отдельную функцию инициализации сервера (для аттача и обычный)
	pStartStopRet = (pOut->DataSize() >= sizeof(CESERVER_REQ_SRVSTARTSTOPRET)) ? &pOut->SrvStartStopRet : NULL;

	if (!pStartStopRet || !pStartStopRet->Info.hWnd || !pStartStopRet->Info.hWndDc || !pStartStopRet->Info.hWndBack)
	{
		gpSrv->ConnectInfo.nErr = GetLastError();

		#ifdef _DEBUG
		wchar_t szDbgMsg[512], szTitle[128];
		GetModuleFileName(NULL, szDbgMsg, countof(szDbgMsg));
		msprintf(szTitle, countof(szTitle), L"%s: PID=%u", PointToName(szDbgMsg), gnSelfPID);
		msprintf(szDbgMsg, countof(szDbgMsg),
			L"ExecuteCmd('%s',%u)\nFailed, code=%u, pOut=%s, Size=%u, "
			L"hWnd=x%08X, hWndDC=x%08X, hWndBack=x%08X",
			szServerPipe, (pOut ? pOut->hdr.nCmd : 0),
			gpSrv->ConnectInfo.nErr, (pOut ? L"OK" : L"NULL"), pOut->DataSize(),
			pStartStopRet ? (DWORD)pStartStopRet->Info.hWnd : 0,
			pStartStopRet ? (DWORD)pStartStopRet->Info.hWndDc : 0,
			pStartStopRet ? (DWORD)pStartStopRet->Info.hWndBack : 0);
		MessageBox(NULL, szDbgMsg, szTitle, MB_SYSTEMMODAL);
		SetLastError(gpSrv->ConnectInfo.nErr);
		#endif

		goto wrap;
	}

	_ASSERTE(pStartStopRet->GuiMapping.cbSize == sizeof(pStartStopRet->GuiMapping));

	// Environment initialization
	if (pStartStopRet->EnvCommands.cchCount)
	{
		ApplyEnvironmentCommands(pStartStopRet->EnvCommands.Demangle());
	}

	SetEnvironmentVariable(ENV_CONEMU_TASKNAME_W, pStartStopRet->TaskName.Demangle());
	SetEnvironmentVariable(ENV_CONEMU_PALETTENAME_W, pStartStopRet->PaletteName.Demangle());

	if (pStartStopRet->Palette.bPalletteLoaded)
	{
		gpSrv->ConsolePalette = pStartStopRet->Palette;
	}

	// Also calls SetConEmuEnvVar
	SetConEmuWindows(pStartStopRet->Info.hWnd, pStartStopRet->Info.hWndDc, pStartStopRet->Info.hWndBack);
	_ASSERTE(gnConEmuPID == pStartStopRet->Info.dwPID);
	gnConEmuPID = pStartStopRet->Info.dwPID;
	_ASSERTE(ghConEmuWnd!=NULL && "Must be set!");

	// Refresh settings
	ReloadGuiSettings(&pStartStopRet->GuiMapping);

	// Limited logging of console contents (same output as processed by CECF_ProcessAnsi)
	InitAnsiLog(pStartStopRet->AnsiLog);

	if (!gbAttachMode) // Часть с "обычным" запуском сервера
	{
		#ifdef _DEBUG
		DWORD nGuiPID; GetWindowThreadProcessId(ghConEmuWnd, &nGuiPID);
		_ASSERTEX(pOut->hdr.nSrcPID==nGuiPID);
		#endif

		gpSrv->bWasDetached = FALSE;
		UpdateConsoleMapHeader(L"TryConnect2Gui, !gbAttachMode");
	}
	else // Запуск сервера "с аттачем" (это может быть RunAsAdmin и т.п.)
	{
		_ASSERTE(pOut->hdr.nCmd==CECMD_ATTACH2GUI);
		_ASSERTE(gpSrv->pConsoleMap != NULL); // мэппинг уже должен быть создан,
		_ASSERTE(gpSrv->pConsole != NULL); // и локальная копия тоже

		//gpSrv->pConsole->info.nGuiPID = pStartStopRet->dwPID;
		CESERVER_CONSOLE_MAPPING_HDR *pMap = gpSrv->pConsoleMap->Ptr();
		if (pMap)
		{
			_ASSERTE(gnConEmuPID == pStartStopRet->Info.dwPID);
			FixConsoleMappingHdr(pMap);
			_ASSERTE(pMap->hConEmuRoot==NULL || pMap->nGuiPID!=0);
		}

		// Только если подцепились успешно
		if (ghConEmuWnd)
		{
			//DisableAutoConfirmExit();

			// В принципе, консоль может действительно запуститься видимой. В этом случае ее скрывать не нужно
			// Но скорее всего, консоль запущенная под Админом в Win7 будет отображена ошибочно
			// 110807 - Если gbAttachMode, тоже консоль нужно спрятать
			if (gbForceHideConWnd || (gbAttachMode && (gbAttachMode != am_Admin)))
			{
				if (!(gpSrv->guiSettings.Flags & CECF_RealConVisible))
					apiShowWindow(ghConWnd, SW_HIDE);
			}

			// Установить шрифт в консоли
			if (pStartStopRet->Font.cbSize == sizeof(CESERVER_REQ_SETFONT))
			{
				lstrcpy(gpSrv->szConsoleFont, pStartStopRet->Font.sFontName);
				gpSrv->nConFontHeight = pStartStopRet->Font.inSizeY;
				gpSrv->nConFontWidth = pStartStopRet->Font.inSizeX;
				ServerInitFont();
			}

			COORD crNewSize = {(SHORT)pStartStopRet->Info.nWidth, (SHORT)pStartStopRet->Info.nHeight};
			//SMALL_RECT rcWnd = {0,pIn->StartStop.sbi.srWindow.Top};
			SMALL_RECT rcWnd = {0};

			CESERVER_REQ *pSizeIn = NULL, *pSizeOut = NULL;
			if (gpSrv->dwAltServerPID && ((pSizeIn = ExecuteNewCmd(CECMD_SETSIZESYNC, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_SETSIZE))) != NULL))
			{
				// conhost uses only SHORT, SetSize.nBufferHeight is defines as USHORT
				_ASSERTE(!HIWORD(pStartStopRet->Info.nBufferHeight));
				pSizeIn->SetSize.nBufferHeight = LOWORD(pStartStopRet->Info.nBufferHeight);
				pSizeIn->SetSize.size = crNewSize;
				//pSizeIn->SetSize.nSendTopLine = -1;
				//pSizeIn->SetSize.rcWindow = rcWnd;

				pSizeOut = ExecuteSrvCmd(gpSrv->dwAltServerPID, pSizeIn, ghConWnd);
			}
			else
			{
				SetConsoleSize((USHORT)pStartStopRet->Info.nBufferHeight, crNewSize, rcWnd, "Attach2Gui:Ret");
			}
			ExecuteFreeResult(pSizeIn);
			ExecuteFreeResult(pSizeOut);
		}
	}

	// Только если подцепились успешно
	if (ghConEmuWnd)
	{
		CheckConEmuHwnd();
		gpSrv->ConnectInfo.bConnected = TRUE;
		bConnected = true;
	}
	else
	{
		//-- не надо, это сделает вызывающая функция
		//SetTerminateEvent(ste_Attach2GuiFailed);
		//DisableAutoConfirmExit();
		_ASSERTE(bConnected == false);
	}

wrap:
	ExecuteFreeResult(pOut);
	return bConnected;
}

HWND Attach2Gui(DWORD nTimeout)
{
	LogFunction(L"Attach2Gui");

	if (isTerminalMode())
	{
		_ASSERTE(FALSE && "Attach is not allowed in telnet");
		return NULL;
	}

	// Нить Refresh НЕ должна быть запущена, иначе в мэппинг могут попасть данные из консоли
	// ДО того, как отработает ресайз (тот размер, который указал установить GUI при аттаче)
	_ASSERTE(gpSrv->dwRefreshThread==0 || gpSrv->bWasDetached);
	HWND hGui = NULL;
	DWORD nToolhelpFoundGuiPID = 0;
	//UINT nMsg = RegisterWindowMessage(CONEMUMSG_ATTACH);
	BOOL bNeedStartGui = FALSE;
	DWORD nStartedGuiPID = 0;
	DWORD dwErr = 0;
	DWORD dwStartWaitIdleResult = -1;
	// Будем подцепляться заново
	if (gpSrv->bWasDetached)
	{
		gpSrv->bWasDetached = FALSE;
		_ASSERTE(gbAttachMode==am_None);
		if (!(gbAttachMode & am_Modes))
			gbAttachMode |= am_Simple;
		if (gpSrv->pConsole)
			gpSrv->pConsole->bDataChanged = TRUE;
	}

	if (!gpSrv->pConsoleMap)
	{
		_ASSERTE(gpSrv->pConsoleMap!=NULL);
	}
	else
	{
		// Чтобы GUI не пытался считать информацию из консоли до завершения аттача (до изменения размеров)
		gpSrv->pConsoleMap->Ptr()->bDataReady = FALSE;
	}

	if (gpSrv->bRequestNewGuiWnd && !gnConEmuPID && !gpSrv->hGuiWnd)
	{
		bNeedStartGui = TRUE;
		hGui = (HWND)-1;
	}
	else if (gpSrv->hGuiWnd)
	{
		// Only HWND may be (was) specified, especially when running from batches
		if (!gnConEmuPID)
			GetWindowThreadProcessId(gpSrv->hGuiWnd, &gnConEmuPID);

		wchar_t szClass[128] = L"";
		GetClassName(gpSrv->hGuiWnd, szClass, countof(szClass));
		if (gnConEmuPID && lstrcmp(szClass, VirtualConsoleClassMain) == 0)
			hGui = gpSrv->hGuiWnd;
		else
			gpSrv->hGuiWnd = NULL;
	}

	// That may fail if processes are running under different credentials or permissions
	if (!hGui)
		hGui = FindWindowEx(NULL, hGui, VirtualConsoleClassMain, NULL);

	if (!hGui)
	{
		//TODO: Reuse MToolHelp.h
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);

		if (hSnap != INVALID_HANDLE_VALUE)
		{
			PROCESSENTRY32 prc = {sizeof(PROCESSENTRY32)};

			if (Process32First(hSnap, &prc))
			{
				do
				{
					for (UINT i = 0; i < gpSrv->nProcessCount; i++)
					{
						if (lstrcmpiW(prc.szExeFile, L"conemu.exe")==0
							|| lstrcmpiW(prc.szExeFile, L"conemu64.exe")==0)
						{
							nToolhelpFoundGuiPID = prc.th32ProcessID;
							break;
						}
					}

					if (nToolhelpFoundGuiPID) break;
				}
				while (Process32Next(hSnap, &prc));
			}

			CloseHandle(hSnap);

			if (!nToolhelpFoundGuiPID) bNeedStartGui = TRUE;
		}
	}

	if (bNeedStartGui)
	{
		wchar_t szGuiExe[MAX_PATH];
		wchar_t *pszSlash = NULL;

		if (!GetModuleFileName(NULL, szGuiExe, MAX_PATH))
		{
			dwErr = GetLastError();
			_printf("GetModuleFileName failed, ErrCode=0x%08X\n", dwErr);
			return NULL;
		}

		pszSlash = wcsrchr(szGuiExe, L'\\');

		if (!pszSlash)
		{
			_printf("Invalid GetModuleFileName, backslash not found!\n", 0, szGuiExe); //-V576
			return NULL;
		}

		bool bExeFound = false;

		for (int s = 0; s <= 1; s++)
		{
			if (s)
			{
				// Он может быть на уровень выше
				*pszSlash = 0;
				pszSlash = wcsrchr(szGuiExe, L'\\');
				if (!pszSlash)
					break;
			}

			if (IsWindows64())
			{
				lstrcpyW(pszSlash+1, L"ConEmu64.exe");
				if (FileExists(szGuiExe))
				{
					bExeFound = true;
					break;
				}
			}

			lstrcpyW(pszSlash+1, L"ConEmu.exe");
			if (FileExists(szGuiExe))
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

		lstrcpyn(gpSrv->guiSettings.sConEmuExe, szGuiExe, countof(gpSrv->guiSettings.sConEmuExe));
		lstrcpyn(gpSrv->guiSettings.ComSpec.ConEmuExeDir, szGuiExe, countof(gpSrv->guiSettings.ComSpec.ConEmuExeDir));
		wchar_t* pszCut = wcsrchr(gpSrv->guiSettings.ComSpec.ConEmuExeDir, L'\\');
		if (pszCut)
			*pszCut = 0;
		if (gpSrv->pConsole)
		{
			lstrcpyn(gpSrv->pConsole->hdr.sConEmuExe, szGuiExe, countof(gpSrv->pConsole->hdr.sConEmuExe));
			lstrcpyn(gpSrv->pConsole->hdr.ComSpec.ConEmuExeDir, gpSrv->guiSettings.ComSpec.ConEmuExeDir, countof(gpSrv->pConsole->hdr.ComSpec.ConEmuExeDir));
		}

		bool bNeedQuot = IsQuotationNeeded(szGuiExe);
		CEStr lsGuiCmd(bNeedQuot ? L"\"" : NULL, szGuiExe, bNeedQuot ? L"\"" : NULL);

		// "/config" and others!
		CEStr cfgSwitches(GetEnvVar(L"ConEmuArgs"));
		if (!cfgSwitches.IsEmpty())
		{
			// `-cmd`, `-cmdlist`, `-run` or `-runlist` must be in the "ConEmuArgs2" only!
			#ifdef _DEBUG
			CEStr lsFirst;
			_ASSERTE(QueryNextArg(cfgSwitches,lsFirst) && !lsFirst.OneOfSwitches(L"-cmd",L"-cmdlist",L"-run",L"-runlist"));
			#endif

			lstrmerge(&lsGuiCmd.ms_Val, L" ", cfgSwitches);
			lstrcpyn(gpSrv->guiSettings.sConEmuArgs, cfgSwitches, countof(gpSrv->guiSettings.sConEmuArgs));
		}

		// The server called from am_Async (RM_AUTOATTACH) mode
		lstrmerge(&lsGuiCmd.ms_Val, L" -Detached");
		#ifdef _DEBUG
		lstrmerge(&lsGuiCmd.ms_Val, L" -NoKeyHooks");
		#endif

		PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
		STARTUPINFOW si; memset(&si, 0, sizeof(si)); si.cb = sizeof(si);
		PRINT_COMSPEC(L"Starting GUI:\n%s\n", pszSelf);
		// CREATE_NEW_PROCESS_GROUP - низя, перестает работать Ctrl-C
		// Запуск GUI (conemu.exe), хуки ест-но не нужны
		BOOL lbRc = createProcess(TRUE, NULL, lsGuiCmd.ms_Val, NULL,NULL, TRUE,
		                           NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
		dwErr = GetLastError();

		if (!lbRc)
		{
			PrintExecuteError(lsGuiCmd, dwErr);
			return NULL;
		}

		//delete psNewCmd; psNewCmd = NULL;
		nStartedGuiPID = pi.dwProcessId;
		AllowSetForegroundWindow(pi.dwProcessId);
		PRINT_COMSPEC(L"Detached GUI was started. PID=%i, Attaching...\n", pi.dwProcessId);
		dwStartWaitIdleResult = WaitForInputIdle(pi.hProcess, INFINITE); // был nTimeout, видимо часто обламывался
		SafeCloseHandle(pi.hProcess); SafeCloseHandle(pi.hThread);
		//if (nTimeout > 1000) nTimeout = 1000;
	}

	DWORD dwStart = GetTickCount(), dwDelta = 0, dwCur = 0;
	CESERVER_REQ *pIn = NULL;
	_ASSERTE(sizeof(CESERVER_REQ_STARTSTOP) >= sizeof(CESERVER_REQ_STARTSTOPRET));
	DWORD cchCmdMax = max((gpszRunCmd ? lstrlen(gpszRunCmd) : 0),(MAX_PATH + 2)) + 1;
	DWORD nInSize =
		sizeof(CESERVER_REQ_HDR)
		+sizeof(CESERVER_REQ_STARTSTOP)
		+cchCmdMax*sizeof(wchar_t);
	pIn = ExecuteNewCmd(CECMD_ATTACH2GUI, nInSize);
	pIn->StartStop.nStarted = sst_ServerStart;
	pIn->StartStop.hWnd = ghConWnd;
	pIn->StartStop.dwPID = gnSelfPID;
	pIn->StartStop.dwAID = gpSrv->dwGuiAID;
	//pIn->StartStop.dwInputTID = gpSrv->dwInputPipeThreadId;
	pIn->StartStop.nSubSystem = gnImageSubsystem;
	// Сразу передать текущий KeyboardLayout
	IsKeyboardLayoutChanged(&pIn->StartStop.dwKeybLayout);
	// После детача/аттача
	DWORD nAltWait;
	if (gpSrv->dwAltServerPID && gpSrv->hAltServer
		&& (WAIT_OBJECT_0 != (nAltWait = WaitForSingleObject(gpSrv->hAltServer, 0))))
	{
		pIn->StartStop.nAltServerPID = gpSrv->dwAltServerPID;
	}

	if (gbAttachFromFar)
		pIn->StartStop.bRootIsCmdExe = FALSE;
	else
		pIn->StartStop.bRootIsCmdExe = gbRootIsCmdExe || (gbAttachMode && !gbNoCreateProcess);

	pIn->StartStop.bRunInBackgroundTab = gbRunInBackgroundTab;

	bool bCmdSet = false;

	if (!bCmdSet && (gpszRunCmd && *gpszRunCmd))
	{
		_wcscpy_c(pIn->StartStop.sCmdLine, cchCmdMax, gpszRunCmd);
		bCmdSet = true;
	}

	if (!bCmdSet && (gpszRootExe && *gpszRootExe))
	{
		_wcscpy_c(pIn->StartStop.sCmdLine, cchCmdMax, gpszRootExe);
		bCmdSet = true;
	}

	//TODO: In the (gbAttachMode & am_Async) mode dwRootProcess is expected to be determined already
	_ASSERTE(bCmdSet || ((gbAttachMode & (am_Async|am_Simple)) && gpSrv->dwRootProcess));
	if (!bCmdSet && gpSrv->dwRootProcess)
	{
		PROCESSENTRY32 pi;
		if (GetProcessInfo(gpSrv->dwRootProcess, &pi))
		{
			msprintf(pIn->StartStop.sCmdLine, cchCmdMax, L"\"%s\"", pi.szExeFile);
		}
	}

	if (pIn->StartStop.sCmdLine[0])
	{
		CEStr lsExe;
		IsNeedCmd(true, pIn->StartStop.sCmdLine, lsExe);
		lstrcpyn(pIn->StartStop.sModuleName, lsExe, countof(pIn->StartStop.sModuleName));
		_ASSERTE(pIn->StartStop.sModuleName[0]!=0);
	}
	else
	{
		// Must be set, otherwise ConEmu will not be able to determine proper AddDistinct options
		_ASSERTE(pIn->StartStop.sCmdLine[0]!=0);
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

	_ASSERTE(ghConEmuWndDC==NULL);
	ghConEmuWndDC = NULL;

	// Если с первого раза не получится (GUI мог еще не загрузиться) пробуем еще
	_ASSERTE(dwDelta < nTimeout); // Must runs at least once!
	while (dwDelta <= nTimeout)
	{
		if (gpSrv->hGuiWnd)
		{
			// On success, it will set ghConEmuWndDC and others...
			TryConnect2Gui(gpSrv->hGuiWnd, 0, pIn);
		}
		else
		{
			HWND hFindGui = FindWindowEx(NULL, NULL, VirtualConsoleClassMain, NULL);
			DWORD nFindPID;

			if ((hFindGui == NULL) && nToolhelpFoundGuiPID)
			{
				TryConnect2Gui(NULL, nToolhelpFoundGuiPID, pIn);
			}
			else do
			{
				// Если ConEmu.exe мы запустили сами
				if (nStartedGuiPID)
				{
					// то цепляемся ТОЛЬКО к этому PID!
					GetWindowThreadProcessId(hFindGui, &nFindPID);
					if (nFindPID != nStartedGuiPID)
						continue;
				}

				// On success, it will set ghConEmuWndDC and others...
				if (TryConnect2Gui(hFindGui, 0, pIn))
					break; // OK
			} while ((hFindGui = FindWindowEx(NULL, hFindGui, VirtualConsoleClassMain, NULL)) != NULL);
		}

		if (ghConEmuWndDC)
			break;

		dwCur = GetTickCount(); dwDelta = dwCur - dwStart;

		if (dwDelta > nTimeout)
			break;

		Sleep(500);
		dwCur = GetTickCount(); dwDelta = dwCur - dwStart;
	}

	return ghConEmuWndDC;
}



void CopySrvMapFromGuiMap()
{
	LogFunction(L"CopySrvMapFromGuiMap");

	if (!gpSrv || !gpSrv->pConsole)
	{
		// Должно быть уже создано!
		_ASSERTE(gpSrv && gpSrv->pConsole);
		return;
	}

	if (!gpSrv->guiSettings.cbSize)
	{
		_ASSERTE(gpSrv->guiSettings.cbSize==sizeof(ConEmuGuiMapping));
		return;
	}

	// настройки командного процессора
	_ASSERTE(gpSrv->guiSettings.ComSpec.ConEmuExeDir[0]!=0 && gpSrv->guiSettings.ComSpec.ConEmuBaseDir[0]!=0);
	gpSrv->pConsole->hdr.ComSpec = gpSrv->guiSettings.ComSpec;

	// Путь к GUI
	if (gpSrv->guiSettings.sConEmuExe[0] != 0)
	{
		wcscpy_c(gpSrv->pConsole->hdr.sConEmuExe, gpSrv->guiSettings.sConEmuExe);
	}
	else
	{
		_ASSERTE(gpSrv->guiSettings.sConEmuExe[0]!=0);
	}

	gpSrv->pConsole->hdr.nLoggingType = gpSrv->guiSettings.nLoggingType;
	gpSrv->pConsole->hdr.bUseInjects = gpSrv->guiSettings.bUseInjects;
	//gpSrv->pConsole->hdr.bDosBox = gpSrv->guiSettings.bDosBox;
	//gpSrv->pConsole->hdr.bUseTrueColor = gpSrv->guiSettings.bUseTrueColor;
	//gpSrv->pConsole->hdr.bProcessAnsi = gpSrv->guiSettings.bProcessAnsi;
	//gpSrv->pConsole->hdr.bUseClink = gpSrv->guiSettings.bUseClink;
	gpSrv->pConsole->hdr.Flags = gpSrv->guiSettings.Flags;

	// Обновить пути к ConEmu
	_ASSERTE(gpSrv->guiSettings.sConEmuExe[0]!=0);
	wcscpy_c(gpSrv->pConsole->hdr.sConEmuExe, gpSrv->guiSettings.sConEmuExe);
	//wcscpy_c(gpSrv->pConsole->hdr.sConEmuBaseDir, gpSrv->guiSettings.sConEmuBaseDir);
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
	LogFunction(L"CreateMapHeader");

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
	bool lbCreated, lbUseExisting = false;

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

	if (!gpSrv->pGuiInfoMap)
		gpSrv->pGuiInfoMap = new MFileMapping<ConEmuGuiMapping>;

	if (!gpSrv->pConsoleMap)
		gpSrv->pConsoleMap = new MFileMapping<CESERVER_CONSOLE_MAPPING_HDR>;
	if (!gpSrv->pConsoleMap)
	{
		_printf("ConEmuC: calloc(MFileMapping<CESERVER_CONSOLE_MAPPING_HDR>) failed, pConsoleMap is null", 0); //-V576
		goto wrap;
	}

	if (!gpSrv->pAppMap)
		gpSrv->pAppMap = new MFileMapping<CESERVER_CONSOLE_APP_MAPPING>;
	if (!gpSrv->pAppMap)
	{
		_printf("ConEmuC: calloc(MFileMapping<CESERVER_CONSOLE_APP_MAPPING>) failed, pAppMap is null", 0); //-V576
		goto wrap;
	}

	gpSrv->pConsoleMap->InitName(CECONMAPNAME, LODWORD(ghConWnd)); //-V205
	gpSrv->pAppMap->InitName(CECONAPPMAPNAME, LODWORD(ghConWnd)); //-V205

	if (gnRunMode == RM_SERVER || gnRunMode == RM_AUTOATTACH)
	{
		lbCreated = (gpSrv->pConsoleMap->Create() != NULL)
			&& (gpSrv->pAppMap->Create() != NULL);
	}
	else
	{
		lbCreated = (gpSrv->pConsoleMap->Open() != NULL)
			&& (gpSrv->pAppMap->Open(TRUE) != NULL);
	}

	if (!lbCreated)
	{
		_ASSERTE(FALSE && "Failed to create/open mapping!");
		_wprintf(gpSrv->pConsoleMap->GetErrorText());
		SafeDelete(gpSrv->pConsoleMap);
		iRc = CERR_CREATEMAPPINGERR; goto wrap;
	}
	else if (gnRunMode == RM_ALTSERVER)
	{
		// На всякий случай, перекинем параметры
		if (gpSrv->pConsoleMap->GetTo(&gpSrv->pConsole->hdr))
		{
			lbUseExisting = true;

			if (gpSrv->pConsole->hdr.ComSpec.ConEmuExeDir[0] && gpSrv->pConsole->hdr.ComSpec.ConEmuBaseDir[0])
			{
				gpSrv->guiSettings.ComSpec = gpSrv->pConsole->hdr.ComSpec;
			}
		}
	}
	else if (gnRunMode == RM_SERVER || gnRunMode == RM_AUTOATTACH)
	{
		CESERVER_CONSOLE_APP_MAPPING init = {sizeof(CESERVER_CONSOLE_APP_MAPPING), CESERVER_REQ_VER};
		init.HookedPids.Init();
		gpSrv->pAppMap->SetFrom(&init);
	}

	// !!! Warning !!! Изменил здесь, поменяй и ReloadGuiSettings/CopySrvMapFromGuiMap !!!
	gpSrv->pConsole->cbMaxSize = nTotalSize;
	gpSrv->pConsole->hdr.cbSize = sizeof(gpSrv->pConsole->hdr);
	if (!lbUseExisting)
		gpSrv->pConsole->hdr.nLogLevel = (gpLogSize!=NULL) ? 1 : 0;
	gpSrv->pConsole->hdr.crMaxConSize = crMax;
	gpSrv->pConsole->hdr.bDataReady = FALSE;
	gpSrv->pConsole->hdr.hConWnd = ghConWnd; _ASSERTE(ghConWnd!=NULL);
	_ASSERTE((gpSrv->dwMainServerPID!=0) || (gbAttachMode & am_Async));
	if (gbAttachMode & am_Async)
	{
		_ASSERTE(gpSrv->dwMainServerPID == 0);
		gpSrv->pConsole->hdr.nServerPID = 0;
	}
	else
	{
		gpSrv->pConsole->hdr.nServerPID = gpSrv->dwMainServerPID;
	}
	gpSrv->pConsole->hdr.nAltServerPID = (gnRunMode==RM_ALTSERVER) ? GetCurrentProcessId() : gpSrv->dwAltServerPID;
	gpSrv->pConsole->hdr.nGuiPID = gnConEmuPID;
	gpSrv->pConsole->hdr.hConEmuRoot = ghConEmuWnd;
	gpSrv->pConsole->hdr.hConEmuWndDc = ghConEmuWndDC;
	gpSrv->pConsole->hdr.hConEmuWndBack = ghConEmuWndBack;
	_ASSERTE(gpSrv->pConsole->hdr.hConEmuRoot==NULL || gpSrv->pConsole->hdr.nGuiPID!=0);
	gpSrv->pConsole->hdr.nServerInShutdown = 0;
	gpSrv->pConsole->hdr.nProtocolVersion = CESERVER_REQ_VER;
	gpSrv->pConsole->hdr.nActiveFarPID = gpSrv->nActiveFarPID; // PID последнего активного фара

	// Обновить переменные окружения (через ConEmuGuiMapping)
	if (ghConEmuWnd) // если уже известен - тогда можно
		ReloadGuiSettings(NULL);

	// По идее, уже должно быть настроено
	if (gpSrv->guiSettings.cbSize == sizeof(ConEmuGuiMapping))
	{
		CopySrvMapFromGuiMap();
	}
	else
	{
		_ASSERTE(gpSrv->guiSettings.cbSize==sizeof(ConEmuGuiMapping) || (gbAttachMode && !ghConEmuWnd));
	}


	gpSrv->pConsole->ConState.hConWnd = ghConWnd; _ASSERTE(ghConWnd!=NULL);
	gpSrv->pConsole->ConState.crMaxSize = crMax;

	// Проверять, нужно ли реестр хукать, будем в конце ServerInit

	//WARNING! Сразу ставим флаг измененности чтобы данные сразу пошли в GUI
	gpSrv->pConsole->bDataChanged = TRUE;


	UpdateConsoleMapHeader(L"CreateMapHeader");
wrap:
	return iRc;
}

CESERVER_CONSOLE_APP_MAPPING* GetAppMapPtr()
{
	if (!gpSrv || !gpSrv->pAppMap)
		return NULL;
	return gpSrv->pAppMap->Ptr();
}

int Compare(const CESERVER_CONSOLE_MAPPING_HDR* p1, const CESERVER_CONSOLE_MAPPING_HDR* p2)
{
	if (!p1 || !p2)
	{
		_ASSERTE(FALSE && "Invalid arguments");
		return 0;
	}
	#ifdef _DEBUG
	_ASSERTE(p1->cbSize==sizeof(CESERVER_CONSOLE_MAPPING_HDR) && (p1->cbSize==p2->cbSize || p2->cbSize==0));
	if (p1->cbSize != p2->cbSize)
		return 1;
	if (p1->nAltServerPID != p2->nAltServerPID)
		return 2;
	if (p1->nActiveFarPID != p2->nActiveFarPID)
		return 3;
	if (memcmp(&p1->crLockedVisible, &p2->crLockedVisible, sizeof(p1->crLockedVisible))!=0)
		return 4;
	#endif
	int nCmp = memcmp(p1, p2, p1->cbSize);
	return nCmp;
};

void UpdateConsoleMapHeader(LPCWSTR asReason /*= NULL*/)
{
	CEStr lsLog(L"UpdateConsoleMapHeader{", asReason, L"}");
	LogFunction(lsLog);

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
			gpSrv->pConsole->hdr.nAltServerPID = gpSrv->dwAltServerPID;
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
		gpSrv->pConsole->hdr.nGuiPID = gnConEmuPID;
		gpSrv->pConsole->hdr.hConEmuRoot = ghConEmuWnd;
		gpSrv->pConsole->hdr.hConEmuWndDc = ghConEmuWndDC;
		gpSrv->pConsole->hdr.hConEmuWndBack = ghConEmuWndBack;
		_ASSERTE(gpSrv->pConsole->hdr.hConEmuRoot==NULL || gpSrv->pConsole->hdr.nGuiPID!=0);
		gpSrv->pConsole->hdr.nActiveFarPID = gpSrv->nActiveFarPID;

		if (gnRunMode == RM_SERVER)
		{
			// Limited logging of console contents (same output as processed by CECF_ProcessAnsi)
			gpSrv->pConsole->hdr.AnsiLog = gpSrv->AnsiLog;
		}

		#ifdef _DEBUG
		int nMapCmp = -100;
		if (gpSrv->pConsoleMap)
			nMapCmp = Compare(&gpSrv->pConsole->hdr, gpSrv->pConsoleMap->Ptr());
		#endif

		// Нельзя альт.серверу мэппинг менять - подерутся
		if ((gnRunMode != RM_SERVER) && (gnRunMode != RM_AUTOATTACH))
		{
			_ASSERTE(gnRunMode == RM_SERVER || gnRunMode == RM_ALTSERVER || gnRunMode == RM_AUTOATTACH);
			// Могли измениться: gcrVisibleSize, nActiveFarPID
			if (gpSrv->dwMainServerPID && gpSrv->dwMainServerPID != GetCurrentProcessId())
			{
				size_t nReqSize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_CONSOLE_MAPPING_HDR);
				CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_UPDCONMAPHDR, nReqSize);
				if (pIn)
				{
					pIn->ConInfo = gpSrv->pConsole->hdr;
					CESERVER_REQ* pOut = ExecuteSrvCmd(gpSrv->dwMainServerPID, pIn, ghConWnd);
					ExecuteFreeResult(pIn);
					ExecuteFreeResult(pOut);
				}
			}
			return;
		}

		if (gpSrv->pConsoleMap)
		{
			if (gpSrv->pConsole->hdr.ComSpec.ConEmuExeDir[0]==0 || gpSrv->pConsole->hdr.ComSpec.ConEmuBaseDir[0]==0)
			{
				_ASSERTE((gpSrv->pConsole->hdr.ComSpec.ConEmuExeDir[0]!=0 && gpSrv->pConsole->hdr.ComSpec.ConEmuBaseDir[0]!=0) || (gbAttachMode && !ghConEmuWnd));
				wchar_t szSelfPath[MAX_PATH+1];
				if (GetModuleFileName(NULL, szSelfPath, countof(szSelfPath)))
				{
					wchar_t* pszSlash = wcsrchr(szSelfPath, L'\\');
					if (pszSlash)
					{
						*pszSlash = 0;
						lstrcpy(gpSrv->pConsole->hdr.ComSpec.ConEmuBaseDir, szSelfPath);
					}
				}
			}

			if (gpSrv->pConsole->hdr.sConEmuExe[0] == 0)
			{
				_ASSERTE((gpSrv->pConsole->hdr.sConEmuExe[0]!=0) || (gbAttachMode && !ghConEmuWnd));
			}

			gpSrv->pConsoleMap->SetFrom(&(gpSrv->pConsole->hdr));
		}
	}
}

int CreateColorerHeader(bool bForceRecreate /*= false*/)
{
	LogFunction(L"CreateColorerHeader");

	if (!gpSrv)
	{
		_ASSERTE(gpSrv!=NULL);
		return -1;
	}

	int iRc = -1;
	DWORD dwErr = 0;
	HWND lhConWnd = NULL;
	const AnnotationHeader* pHdr = NULL;
	int nHdrSize;

	MSectionLockSimple CS;
	CS.Lock(&gpSrv->csColorerMappingCreate);

	// По идее, не должно быть пересоздания TrueColor мэппинга, разве что при Detach/Attach
	_ASSERTE((gpSrv->pColorerMapping == NULL) || (gbAttachMode == am_Simple));

	if (bForceRecreate)
	{
		if (gpSrv->pColorerMapping)
		{
			SafeDelete(gpSrv->pColorerMapping);
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
	_ASSERTE(sizeof(AnnotationInfo) == 8*sizeof(int)/*sizeof(AnnotationInfo::raw)*/);

	if (gpSrv->pColorerMapping == NULL)
	{
		gpSrv->pColorerMapping = new MFileMapping<const AnnotationHeader>;
	}
	// Задать имя для mapping, если надо - сам сделает CloseMap();
	gpSrv->pColorerMapping->InitName(AnnotationShareName, (DWORD)sizeof(AnnotationInfo), LODWORD(lhConWnd)); //-V205

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

	// AnnotationShareName is CREATED in ConEmu.exe
	// May be it would be better, to avoid hooking and cycling (minhook),
	// call CreateFileMapping instead of OpenFileMapping...
	pHdr = gpSrv->pColorerMapping->Open();

	if (!pHdr)
	{
		dwErr = GetLastError();
		// The TrueColor may be disabled in ConEmu settings, don't warn user about it
		SafeDelete(gpSrv->pColorerMapping);
		goto wrap;
	}
	else if ((nHdrSize = pHdr->struct_size) != sizeof(AnnotationHeader))
	{
		Sleep(500);
		int nDbgSize = pHdr->struct_size;
		_ASSERTE(nHdrSize == sizeof(AnnotationHeader));
		UNREFERENCED_PARAMETER(nDbgSize);

		if (pHdr->struct_size != sizeof(AnnotationHeader))
		{
			SafeDelete(gpSrv->pColorerMapping);
			goto wrap;
		}
	}

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

	//}

wrap:
	CS.Unlock();
	return iRc;
}

void CloseMapHeader()
{
	LogFunction(L"CloseMapHeader");

	if (gpSrv->pConsoleMap)
	{
		//gpSrv->pConsoleMap->CloseMap(); -- не требуется, сделает деструктор
		SafeDelete(gpSrv->pConsoleMap);
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

	if (gpSrv->pConsoleMap)
	{
		free(gpSrv->pConsoleMap);
		gpSrv->pConsoleMap = NULL;
	}
}


// Limited logging of console contents (same output as processed by CECF_ProcessAnsi)
void InitAnsiLog(const ConEmuAnsiLog& AnsiLog)
{
	LogFunction(L"InitAnsiLog");
	// Reset first
	SetEnvironmentVariable(ENV_CONEMUANSILOG_VAR_W, L"");
	gpSrv->AnsiLog.Enabled = FALSE;
	gpSrv->AnsiLog.Path[0] = 0;
	// Enabled?
	if (!AnsiLog.Enabled || !*AnsiLog.Path)
	{
		_ASSERTE(!AnsiLog.Enabled || AnsiLog.Path[0]!=0);
		return;
	}
	// May contains variables
	wchar_t* pszExp = ExpandEnvStr(AnsiLog.Path);
	// Max path = (MAX_PATH - "ConEmu-yyyy-mm-dd-p12345.log")
	wchar_t szPath[MAX_PATH] = L"", szName[40] = L"";
	SYSTEMTIME st = {}; GetLocalTime(&st);
	msprintf(szName, countof(szName), CEANSILOGNAMEFMT, st.wYear, st.wMonth, st.wDay, GetCurrentProcessId());
	int nNameLen = lstrlen(szName);
	lstrcpyn(szPath, pszExp ? pszExp : AnsiLog.Path, countof(szPath)-nNameLen);
	int nLen = lstrlen(szPath);
	if ((nLen >= 1) && (szPath[nLen-1] != L'\\'))
		szPath[nLen++] = L'\\';
	if (!DirectoryExists(szPath))
	{
		if (!MyCreateDirectory(szPath))
		{
			DWORD dwErr = GetLastError();
			_printf("Failed to create AnsiLog-files directory:\n");
			_wprintf(szPath);
			print_error(dwErr);
			return;
		}
	}
	// Prepare path
	wcscat_c(szPath, szName);
	// Try to create
	HANDLE hLog = CreateFile(szPath, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hLog == INVALID_HANDLE_VALUE)
	{
		DWORD dwErr = GetLastError();
		_printf("Failed to create new AnsiLog-file:\n");
		_wprintf(szPath);
		print_error(dwErr);
		return;
	}
	CloseHandle(hLog);
	// OK!
	gpSrv->AnsiLog.Enabled = TRUE;
	wcscpy_c(gpSrv->AnsiLog.Path, szPath);
	SetEnvironmentVariable(ENV_CONEMUANSILOG_VAR_W, szPath);
}

#if 0
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
	else if (gpSrv->nTopVisibleLine != -1)
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
#endif


bool CheckWasFullScreen()
{
	bool bFullScreenHW = false;

	if (gpSrv->pfnWasFullscreenMode)
	{
		DWORD nModeFlags = 0; gpSrv->pfnWasFullscreenMode(&nModeFlags);
		if (nModeFlags & CONSOLE_FULLSCREEN_HARDWARE)
		{
			bFullScreenHW = true;
		}
		else
		{
			gpSrv->pfnWasFullscreenMode = NULL;
		}
	}

	return bFullScreenHW;
}

static int ReadConsoleInfo()
{
	// Need to block all requests to output buffer in other threads
	MSectionLockSimple csRead; csRead.Lock(&gpSrv->csReadConsoleInfo, LOCK_READOUTPUT_TIMEOUT);

	if (CheckWasFullScreen())
	{
		LogString("!!! ReadConsoleInfo was skipped due to CONSOLE_FULLSCREEN_HARDWARE !!!");
		return -1;
	}

	//int liRc = 1;
	BOOL lbChanged = gpSrv->pConsole->bDataChanged; // Если что-то еще не отослали - сразу TRUE
	CONSOLE_SELECTION_INFO lsel = {0}; // apiGetConsoleSelectionInfo
	CONSOLE_CURSOR_INFO lci = {0}; // GetConsoleCursorInfo
	DWORD ldwConsoleCP=0, ldwConsoleOutputCP=0, ldwConsoleMode;
	CONSOLE_SCREEN_BUFFER_INFO lsbi = {{0,0}}; // MyGetConsoleScreenBufferInfo
	HANDLE hOut = (HANDLE)ghConOut;
	HANDLE hStdOut = NULL;
	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	//DWORD nConInMode = 0;

	if (hOut == INVALID_HANDLE_VALUE)
		hOut = hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	// Могут возникать проблемы при закрытии ComSpec и уменьшении высоты буфера
	MCHKHEAP;

	if (!apiGetConsoleSelectionInfo(&lsel))
	{
		SetConEmuFlags(gpSrv->pConsole->ConState.Flags,CECI_Paused,(CECI_None));
	}
	else
	{
		SetConEmuFlags(gpSrv->pConsole->ConState.Flags,CECI_Paused,((lsel.dwFlags & CONSOLE_SELECTION_IN_PROGRESS) ? CECI_Paused : CECI_None));
	}

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
		LogModeChange(L"ConCP", gpSrv->dwConsoleCP, ldwConsoleCP);
		gpSrv->dwConsoleCP = ldwConsoleCP; lbChanged = TRUE;
	}

	ldwConsoleOutputCP = GetConsoleOutputCP();

	if (gpSrv->dwConsoleOutputCP!=ldwConsoleOutputCP)
	{
		LogModeChange(L"ConOutCP", gpSrv->dwConsoleOutputCP, ldwConsoleOutputCP);
		gpSrv->dwConsoleOutputCP = ldwConsoleOutputCP; lbChanged = TRUE;
	}

	// ConsoleInMode
	ldwConsoleMode = 0;
	DEBUGTEST(BOOL lbConModRc =)
	GetConsoleMode(hStdIn, &ldwConsoleMode);
	if (gpSrv->dwConsoleInMode != LOWORD(ldwConsoleMode))
	{
		_ASSERTE(LOWORD(ldwConsoleMode) == ldwConsoleMode);
		LogModeChange(L"ConInMode", gpSrv->dwConsoleInMode, ldwConsoleMode);
		gpSrv->dwConsoleInMode = LOWORD(ldwConsoleMode); lbChanged = TRUE;
	}

	// ConsoleOutMode
	ldwConsoleMode = 0;
	DEBUGTEST(lbConModRc =)
	GetConsoleMode(hOut, &ldwConsoleMode);
	if (gpSrv->dwConsoleOutMode != LOWORD(ldwConsoleMode))
	{
		_ASSERTE(LOWORD(ldwConsoleMode) == ldwConsoleMode);
		LogModeChange(L"ConOutMode", gpSrv->dwConsoleOutMode, ldwConsoleMode);
		gpSrv->dwConsoleOutMode = LOWORD(ldwConsoleMode); lbChanged = TRUE;
	}

	MCHKHEAP;

	if (!MyGetConsoleScreenBufferInfo(hOut, &lsbi))
	{
		DWORD dwErr = GetLastError();
		_ASSERTE(FALSE && "!!! ReadConsole::MyGetConsoleScreenBufferInfo failed !!!");

		gpSrv->dwSbiRc = dwErr; if (!gpSrv->dwSbiRc) gpSrv->dwSbiRc = -1;

		//liRc = -1;
		return -1;
	}
	else
	{
		DWORD nCurScroll = (gnBufferHeight ? rbs_Vert : 0) | (gnBufferWidth ? rbs_Horz : 0);
		DWORD nNewScroll = 0;
		int TextWidth = 0, TextHeight = 0;
		short nMaxWidth = -1, nMaxHeight = -1;
		BOOL bSuccess = ::GetConWindowSize(lsbi, gcrVisibleSize.X, gcrVisibleSize.Y, nCurScroll, &TextWidth, &TextHeight, &nNewScroll);

		// Скорректировать "видимый" буфер. Видимым считаем то, что показывается в ConEmu
		if (bSuccess)
		{
			//rgn = gpSrv->sbi.srWindow;
			if (!(nNewScroll & rbs_Horz))
			{
				lsbi.srWindow.Left = 0;
				nMaxWidth = max(gpSrv->pConsole->hdr.crMaxConSize.X,(lsbi.srWindow.Right-lsbi.srWindow.Left+1));
				lsbi.srWindow.Right = min(nMaxWidth,lsbi.dwSize.X-1);
			}

			if (!(nNewScroll & rbs_Vert))
			{
				lsbi.srWindow.Top = 0;
				nMaxHeight = max(gpSrv->pConsole->hdr.crMaxConSize.Y,(lsbi.srWindow.Bottom-lsbi.srWindow.Top+1));
				lsbi.srWindow.Bottom = min(nMaxHeight,lsbi.dwSize.Y-1);
			}
		}

		if (memcmp(&gpSrv->sbi, &lsbi, sizeof(gpSrv->sbi)))
		{
			InputLogger::Log(InputLogger::Event::evt_ConSbiChanged);

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
				// ширина
				if ((lsbi.srWindow.Left == 0  // или окно соответствует полному буферу
				        && lsbi.dwSize.X == (lsbi.srWindow.Right - lsbi.srWindow.Left + 1)))
				{
					// Это значит, что прокрутки нет, и консольное приложение изменило размер буфера
					gnBufferWidth = 0;
					gcrVisibleSize.X = lsbi.dwSize.X;
				}
				// высота
				if ((lsbi.srWindow.Top == 0  // или окно соответствует полному буферу
				        && lsbi.dwSize.Y == (lsbi.srWindow.Bottom - lsbi.srWindow.Top + 1)))
				{
					// Это значит, что прокрутки нет, и консольное приложение изменило размер буфера
					gnBufferHeight = 0;
					gcrVisibleSize.Y = lsbi.dwSize.Y;
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

			if (gpLogSize) LogSize(NULL, 0, ":ReadConsoleInfo");

			gpSrv->sbi = lsbi;
			lbChanged = TRUE;
		}
	}

	if (!gnBufferHeight)
	{
		int nWndHeight = (gpSrv->sbi.srWindow.Bottom - gpSrv->sbi.srWindow.Top + 1);

		if (gpSrv->sbi.dwSize.Y > (max(gcrVisibleSize.Y,nWndHeight)+200)
		        || ((gpSrv->nRequestChangeSize > 0) && gpSrv->nReqSizeBufferHeight))
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
	gpSrv->pConsole->hdr.hConWnd = gpSrv->pConsole->ConState.hConWnd = ghConWnd;
	_ASSERTE(gpSrv->dwMainServerPID!=0);
	gpSrv->pConsole->hdr.nServerPID = gpSrv->dwMainServerPID;
	gpSrv->pConsole->hdr.nAltServerPID = (gnRunMode==RM_ALTSERVER) ? GetCurrentProcessId() : gpSrv->dwAltServerPID;
	//gpSrv->pConsole->info.nInputTID = gpSrv->dwInputThreadId;
	gpSrv->pConsole->ConState.nReserved0 = 0;
	gpSrv->pConsole->ConState.dwCiSize = sizeof(gpSrv->ci);
	gpSrv->pConsole->ConState.ci = gpSrv->ci;
	gpSrv->pConsole->ConState.dwConsoleCP = gpSrv->dwConsoleCP;
	gpSrv->pConsole->ConState.dwConsoleOutputCP = gpSrv->dwConsoleOutputCP;
	gpSrv->pConsole->ConState.dwConsoleInMode = gpSrv->dwConsoleInMode;
	gpSrv->pConsole->ConState.dwConsoleOutMode = gpSrv->dwConsoleOutMode;
	gpSrv->pConsole->ConState.dwSbiSize = sizeof(gpSrv->sbi);
	gpSrv->pConsole->ConState.sbi = gpSrv->sbi;
	gpSrv->pConsole->ConState.ConsolePalette = gpSrv->ConsolePalette;


	// Если есть возможность (WinXP+) - получим реальный список процессов из консоли
	//CheckProcessCount(); -- уже должно быть вызвано !!!
	//2010-05-26 Изменения в списке процессов не приходили в GUI до любого чиха в консоль.
	#ifdef _DEBUG
	_ASSERTE(gpSrv->pnProcesses!=NULL);
	if (!gpSrv->nProcessCount)
	{
		_ASSERTE(gpSrv->nProcessCount); //CheckProcessCount(); -- must be already initialized !!!
	}
	#endif

	DWORD nCurProcCount = GetProcessCount(gpSrv->pConsole->ConState.nProcesses, countof(gpSrv->pConsole->ConState.nProcesses));
	_ASSERTE(nCurProcCount && gpSrv->pConsole->ConState.nProcesses[0]);
	if (nCurProcCount
		&& memcmp(gpSrv->nLastRetProcesses, gpSrv->pConsole->ConState.nProcesses, sizeof(gpSrv->nLastRetProcesses)))
	{
		// Process list was changed
		memmove(gpSrv->nLastRetProcesses, gpSrv->pConsole->ConState.nProcesses, sizeof(gpSrv->nLastRetProcesses));
		lbChanged = TRUE;
	}

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
	// Need to block all requests to output buffer in other threads
	MSectionLockSimple csRead; csRead.Lock(&gpSrv->csReadConsoleInfo, LOCK_READOUTPUT_TIMEOUT);

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
	short nMaxWidth = -1, nMaxHeight = -1;
	char sFailedInfo[128];

	// sbi считывается в ReadConsoleInfo
	BOOL bSuccess = ::GetConWindowSize(gpSrv->sbi, gcrVisibleSize.X, gcrVisibleSize.Y, nCurScroll, &TextWidth, &TextHeight, &nNewScroll);

	UNREFERENCED_PARAMETER(bSuccess);
	//TextWidth  = gpSrv->sbi.dwSize.X;
	//TextHeight = (gpSrv->sbi.srWindow.Bottom - gpSrv->sbi.srWindow.Top + 1);
	TextLen = TextWidth * TextHeight;

	if (!gpSrv->pConsole)
	{
		_ASSERTE(gpSrv->pConsole!=NULL);
		LogString("gpSrv->pConsole == NULL");
		goto wrap;
	}

	//rgn = gpSrv->sbi.srWindow;
	if (nNewScroll & rbs_Horz)
	{
		rgn.Left = gpSrv->sbi.srWindow.Left;
		rgn.Right = min(gpSrv->sbi.srWindow.Left+TextWidth,gpSrv->sbi.dwSize.X)-1;
	}
	else
	{
		rgn.Left = 0;
		nMaxWidth = max(gpSrv->pConsole->hdr.crMaxConSize.X,(gpSrv->sbi.srWindow.Right-gpSrv->sbi.srWindow.Left+1));
		rgn.Right = min(nMaxWidth,(gpSrv->sbi.dwSize.X-1));
	}

	if (nNewScroll & rbs_Vert)
	{
		rgn.Top = gpSrv->sbi.srWindow.Top;
		rgn.Bottom = min(gpSrv->sbi.srWindow.Top+TextHeight,gpSrv->sbi.dwSize.Y)-1;
	}
	else
	{
		rgn.Top = 0;
		nMaxHeight = max(gpSrv->pConsole->hdr.crMaxConSize.Y,(gpSrv->sbi.srWindow.Bottom-gpSrv->sbi.srWindow.Top+1));
		rgn.Bottom = min(nMaxHeight,(gpSrv->sbi.dwSize.Y-1));
	}


	if (!TextWidth || !TextHeight)
	{
		_ASSERTE(TextWidth && TextHeight);
		goto wrap;
	}

	nCurSize = TextLen * sizeof(CHAR_INFO);
	nHdrSize = sizeof(CESERVER_REQ_CONINFO_FULL)-sizeof(CHAR_INFO);

	if (gpSrv->pConsole->cbMaxSize < (nCurSize+nHdrSize))
	{
		_ASSERTE(gpSrv->pConsole && gpSrv->pConsole->cbMaxSize >= (nCurSize+nHdrSize));

		_wsprintfA(sFailedInfo, SKIPLEN(countof(sFailedInfo)) "ReadConsoleData FAIL: MaxSize(%u) < CurSize(%u), TextSize(%ux%u)", gpSrv->pConsole->cbMaxSize, (nCurSize+nHdrSize), TextWidth, TextHeight);
		LogString(sFailedInfo);

		TextHeight = (gpSrv->pConsole->ConState.crMaxSize.X * gpSrv->pConsole->ConState.crMaxSize.Y - (TextWidth-1)) / TextWidth;

		if (TextHeight <= 0)
		{
			_ASSERTE(TextHeight > 0);
			goto wrap;
		}

		rgn.Bottom = min(rgn.Bottom,(rgn.Top+TextHeight-1));
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

	gpSrv->pConsole->ConState.crWindow = MakeCoord(TextWidth, TextHeight);

	gpSrv->pConsole->ConState.sbi.srWindow = rgn;

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
		InputLogger::Log(InputLogger::Event::evt_ConDataChanged);
		memmove(gpSrv->pConsole->data, gpSrv->pConsoleDataCopy, nCurSize);
		gpSrv->pConsole->bDataChanged = TRUE; // TRUE уже может быть с прошлого раза, не сбрасывать в FALSE
		lbChanged = TRUE;
		LogString("ReadConsoleData: content was changed");
	}


	if (!lbChanged && gpSrv->pColorerMapping)
	{
		AnnotationHeader ahdr;
		if (gpSrv->pColorerMapping->GetTo(&ahdr, sizeof(ahdr)))
		{
			if (gpSrv->ColorerHdr.flushCounter != ahdr.flushCounter && !ahdr.locked)
			{
				gpSrv->ColorerHdr = ahdr;
				gpSrv->pConsole->bDataChanged = TRUE; // TRUE уже может быть с прошлого раза, не сбрасывать в FALSE
				lbChanged = TRUE;
			}
		}
	}


	// низя - он уже установлен в максимальное значение
	//gpSrv->pConsoleData->crBufSize = gpSrv->pConsoleDataCopy->crBufSize;
wrap:
	//if (lbChanged)
	//	gpSrv->pConsole->bDataChanged = TRUE;
	UNREFERENCED_PARAMETER(nMaxWidth); UNREFERENCED_PARAMETER(nMaxHeight);
	return lbChanged;
}




// abForceSend выставляется в TRUE, чтобы гарантированно
// передернуть GUI по таймауту (не реже 1 сек).
BOOL ReloadFullConsoleInfo(BOOL abForceSend)
{
	if (CheckWasFullScreen())
	{
		LogString("ReloadFullConsoleInfo was skipped due to CONSOLE_FULLSCREEN_HARDWARE");
		return FALSE;
	}

	BOOL lbChanged = abForceSend;
	BOOL lbDataChanged = abForceSend;
	DWORD dwCurThId = GetCurrentThreadId();

	// Должен вызываться ТОЛЬКО в нити (RefreshThread)
	// Иначе возможны блокировки
	if (abForceSend && gpSrv->dwRefreshThread && dwCurThId != gpSrv->dwRefreshThread)
	{
		//ResetEvent(gpSrv->hDataReadyEvent);
		gpSrv->bForceConsoleRead = TRUE;

		ResetEvent(gpSrv->hRefreshDoneEvent);
		SetEvent(gpSrv->hRefreshEvent);
		// Ожидание, пока сработает RefreshThread
		HANDLE hEvents[2] = {ghQuitEvent, gpSrv->hRefreshDoneEvent};
		DWORD nWait = WaitForMultipleObjects(2, hEvents, FALSE, RELOAD_INFO_TIMEOUT);
		lbChanged = (nWait == (WAIT_OBJECT_0+1));

		gpSrv->bForceConsoleRead = FALSE;

		return lbChanged;
	}

#ifdef _DEBUG
	DWORD nPacketID = gpSrv->pConsole->ConState.nPacketId;
#endif

#ifdef USE_COMMIT_EVENT
	if (gpSrv->hExtConsoleCommit)
	{
		WaitForSingleObject(gpSrv->hExtConsoleCommit, EXTCONCOMMIT_TIMEOUT);
	}
#endif

	if (abForceSend)
		gpSrv->pConsole->bDataChanged = TRUE;

	DWORD nTick1 = GetTickCount(), nTick2 = 0, nTick3 = 0, nTick4 = 0, nTick5 = 0;

	// Need to block all requests to output buffer in other threads
	MSectionLockSimple csRead; csRead.Lock(&gpSrv->csReadConsoleInfo, LOCK_READOUTPUT_TIMEOUT);

	nTick2 = GetTickCount();

	// Read sizes flags and other information
	int iInfoRc = ReadConsoleInfo();

	nTick3 = GetTickCount();

	if (iInfoRc == -1)
	{
		lbChanged = FALSE;
	}
	else
	{
		if (iInfoRc == 1)
			lbChanged = TRUE;

		// Read chars and attributes for visible (or locked) area
		if (ReadConsoleData())
			lbChanged = lbDataChanged = TRUE;

		nTick4 = GetTickCount();

		if (lbChanged && !gpSrv->pConsole->hdr.bDataReady)
		{
			gpSrv->pConsole->hdr.bDataReady = TRUE;
		}

		//if (memcmp(&(gpSrv->pConsole->hdr), gpSrv->pConsoleMap->Ptr(), gpSrv->pConsole->hdr.cbSize))
		int iMapCmp = Compare(&gpSrv->pConsole->hdr, gpSrv->pConsoleMap->Ptr());
		if (iMapCmp)
		{
			lbChanged = TRUE;

			UpdateConsoleMapHeader(L"ReloadFullConsoleInfo");
		}

		if (lbChanged)
		{
			// Накрутить счетчик и Tick
			//gpSrv->pConsole->bChanged = TRUE;
			//if (lbDataChanged)
			gpSrv->pConsole->ConState.nPacketId++;
			gpSrv->pConsole->ConState.nSrvUpdateTick = GetTickCount();

			if (gpSrv->hDataReadyEvent)
				SetEvent(gpSrv->hDataReadyEvent);

			//if (nPacketID == gpSrv->pConsole->info.nPacketId) {
			//	gpSrv->pConsole->info.nPacketId++;
			//	TODO("Можно заменить на multimedia tick");
			//	gpSrv->pConsole->info.nSrvUpdateTick = GetTickCount();
			//	//			gpSrv->nFarInfoLastIdx = gpSrv->pConsole->info.nFarInfoIdx;
			//}
		}

		nTick5 = GetTickCount();
	}

	csRead.Unlock();

	UNREFERENCED_PARAMETER(nTick1);
	UNREFERENCED_PARAMETER(nTick2);
	UNREFERENCED_PARAMETER(nTick3);
	UNREFERENCED_PARAMETER(nTick4);
	UNREFERENCED_PARAMETER(nTick5);
	return lbChanged;
}


enum SleepIndicatorType
{
	sit_None = 0,
	sit_Num,
	sit_Title,
};

SleepIndicatorType CheckIndicateSleepNum()
{
	static SleepIndicatorType bCheckIndicateSleepNum = sit_None;
	static DWORD nLastCheckTick = 0;

	if (!nLastCheckTick || ((GetTickCount() - nLastCheckTick) >= 3000))
	{
		wchar_t szVal[32] = L"";
		DWORD nLen = GetEnvironmentVariable(ENV_CONEMU_SLEEP_INDICATE_W, szVal, countof(szVal)-1);
		if (nLen && (nLen < countof(szVal)))
		{
			if (lstrcmpni(szVal, ENV_CONEMU_SLEEP_INDICATE_NUM, lstrlen(ENV_CONEMU_SLEEP_INDICATE_NUM)) == 0)
				bCheckIndicateSleepNum = sit_Num;
			else if (lstrcmpni(szVal, ENV_CONEMU_SLEEP_INDICATE_TTL, lstrlen(ENV_CONEMU_SLEEP_INDICATE_TTL)) == 0)
				bCheckIndicateSleepNum = sit_Title;
			else
				bCheckIndicateSleepNum = sit_None;
		}
		else
		{
			bCheckIndicateSleepNum = sit_None; // Надо, ибо может быть обратный сброс переменной
		}
		nLastCheckTick = GetTickCount();
	}

	return bCheckIndicateSleepNum;
}

void ShowSleepIndicator(SleepIndicatorType SleepType, bool bSleeping)
{
	switch (SleepType)
	{
	case sit_Num:
		{
			// Num: Sleeping - OFF, Active - ON
			bool bNum = (GetKeyState(VK_NUMLOCK) & 1) == 1;
			if (bNum == bSleeping)
			{
				keybd_event(VK_NUMLOCK, 0, 0, 0);
				keybd_event(VK_NUMLOCK, 0, KEYEVENTF_KEYUP, 0);
			}
		} break;
	case sit_Title:
		{
			const wchar_t szSleepPrefix[] = L"[Sleep] ";
			const int nPrefixLen = lstrlen(szSleepPrefix);
			static wchar_t szTitle[2000];
			DWORD nLen = GetConsoleTitle(szTitle+nPrefixLen, countof(szTitle)-nPrefixLen);
			bool bOld = (wcsstr(szTitle+nPrefixLen, szSleepPrefix) != NULL);
			if (bOld && !bSleeping)
			{
				wchar_t* psz;
				while ((psz = wcsstr(szTitle+nPrefixLen, szSleepPrefix)) != NULL)
				{
					wmemmove(psz, psz+nPrefixLen, wcslen(psz+nPrefixLen)+1);
				}
				SetConsoleTitle(szTitle+nPrefixLen);
			}
			else if (!bOld && bSleeping)
			{
				wmemmove(szTitle, szSleepPrefix, nPrefixLen);
				SetConsoleTitle(szTitle);
			}
		} break;
	}
}


bool FreezeRefreshThread()
{
	MSectionLockSimple csControl;
	csControl.Lock(&gpSrv->csRefreshControl, LOCK_REFRESH_CONTROL_TIMEOUT);
	if (!gpSrv->hFreezeRefreshThread)
		gpSrv->hFreezeRefreshThread = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (GetCurrentThreadId() == gpSrv->dwRefreshThread)
	{
		_ASSERTE(GetCurrentThreadId() != gpSrv->dwRefreshThread);
		return false;
	}
	
	gpSrv->nRefreshFreezeRequests++;
	ResetEvent(gpSrv->hFreezeRefreshThread);

	csControl.Unlock();

	// wait while refresh thread becomes freezed
	DWORD nStartTick = GetTickCount(), nCurTick = 0; //TODO: replace with service class
	DWORD nWait = (DWORD)-1;
	HANDLE hWait[] = {gpSrv->hRefreshThread, ghQuitEvent};
	while (gpSrv->hRefreshThread && (gpSrv->nRefreshIsFreezed <= 0))
	{
		nWait = WaitForMultipleObjects(countof(hWait), hWait, FALSE, 100);
		if ((nWait == WAIT_OBJECT_0) || (nWait == (WAIT_OBJECT_0+1)))
			break;
		if (((nCurTick = GetTickCount()) - nStartTick) > LOCK_REFRESH_CONTROL_TIMEOUT)
			break;
	}

	return (gpSrv->nRefreshIsFreezed > 0) || (gpSrv->hRefreshThread == NULL);
}

bool ThawRefreshThread()
{
	MSectionLockSimple csControl;
	csControl.Lock(&gpSrv->csRefreshControl, LOCK_REFRESH_CONTROL_TIMEOUT);

	if (gpSrv->nRefreshFreezeRequests > 0)
	{
		gpSrv->nRefreshFreezeRequests--;
	}
	else
	{
		_ASSERTE(FALSE && "Unbalanced FreezeRefreshThread/ThawRefreshThread calls");
	}

	// decrease counter, if == 0 thaw the thread
	if (gpSrv->hFreezeRefreshThread && (gpSrv->nRefreshFreezeRequests == 0))
		SetEvent(gpSrv->hFreezeRefreshThread);

	return (gpSrv->nRefreshFreezeRequests == 0);
}


DWORD WINAPI RefreshThread(LPVOID lpvParam)
{
	DWORD nWait = 0, nAltWait = 0, nFreezeWait = 0, nThreadWait = 0;

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
	BOOL bConsoleActive = (BOOL)-1;
	BOOL bDCWndVisible = (BOOL)-1;
	BOOL bNewActive = (BOOL)-1, bNewFellInSleep = FALSE;
	BOOL ActiveSleepInBg = (gpSrv->guiSettings.Flags & CECF_SleepInBackg);
	BOOL RetardNAPanes = (gpSrv->guiSettings.Flags & CECF_RetardNAPanes);
	BOOL bOurConActive = (BOOL)-1, bOneConActive = (BOOL)-1;
	bool bLowSpeed = false;
	BOOL bOnlyCursorChanged;
	BOOL bSetRefreshDoneEvent;
	DWORD nWaitCursor = 99;
	DWORD nWaitCommit = 99;
	SleepIndicatorType SleepType = sit_None;
	DWORD nLastConsoleActiveTick = 0;
	DWORD nLastConsoleActiveDelta = 0;

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
			InterlockedIncrement(&gpSrv->nRefreshIsFreezed);
			nFreezeWait = WaitForMultipleObjects(countof(hFreeze), hFreeze, FALSE, INFINITE);
			InterlockedDecrement(&gpSrv->nRefreshIsFreezed);
			if (nFreezeWait == (WAIT_OBJECT_0+1))
				break; // затребовано завершение потока
		}


		if (gpSrv->hWaitForSetConBufThread)
		{
			HANDLE hInEvent = gpSrv->hInWaitForSetConBufThread;
			HANDLE hOutEvent = gpSrv->hOutWaitForSetConBufThread;
			HANDLE hWaitEvent = gpSrv->hWaitForSetConBufThread;
			// Tell server thread, that it is safe to return control to application
			if (hInEvent)
				SetEvent(hInEvent);
			// What we are waiting for...
			HANDLE hThreadWait[] = {ghQuitEvent, hOutEvent, hWaitEvent};
			// To avoid infinite blocking - limit waiting time?
			if (hWaitEvent != INVALID_HANDLE_VALUE)
				nThreadWait = WaitForMultipleObjects(countof(hThreadWait), hThreadWait, FALSE, WAIT_SETCONSCRBUF_MAX_TIMEOUT);
			else
				nThreadWait = WaitForMultipleObjects(countof(hThreadWait)-1, hThreadWait, FALSE, WAIT_SETCONSCRBUF_MIN_TIMEOUT);
			// Done, close handles, they are no longer needed
			if (hInEvent == gpSrv->hInWaitForSetConBufThread) gpSrv->hInWaitForSetConBufThread = NULL;
			if (hOutEvent == gpSrv->hOutWaitForSetConBufThread) gpSrv->hOutWaitForSetConBufThread = NULL;
			if (hWaitEvent == gpSrv->hWaitForSetConBufThread) gpSrv->hWaitForSetConBufThread = NULL;
			SafeCloseHandle(hWaitEvent);
			SafeCloseHandle(hInEvent);
			SafeCloseHandle(hOutEvent);
			UNREFERENCED_PARAMETER(nThreadWait);
			if (nThreadWait == WAIT_OBJECT_0)
				break; // затребовано завершение потока
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
				if (nAltWait == (WAIT_OBJECT_0+0))
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
							// Обновить мэппинг
							wchar_t szLog[80]; _wsprintf(szLog, SKIPCOUNT(szLog) L"RefreshThread, new AltServer=%u", gpSrv->dwAltServerPID);
							UpdateConsoleMapHeader(szLog);
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
				// затребовано завершение потока
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
			// Need to block all requests to output buffer in other threads
			ConOutCloseHandle();
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
		if (!nAltWait && (gpSrv->nRequestChangeSize > 0))
		{
			if (gpSrv->bStationLocked)
			{
				LogString("!!! Change size request received while station is LOCKED !!!");
				_ASSERTE(!gpSrv->bStationLocked);
			}

			InterlockedDecrement(&gpSrv->nRequestChangeSize);
			// AVP гундит... да вроде и не нужно
			//DWORD dwSusp = 0, dwSuspErr = 0;
			//if (gpSrv->hRootThread) {
			//	WARNING("A 64-bit application can suspend a WOW64 thread using the Wow64SuspendThread function");
			//	// The handle must have the THREAD_SUSPEND_RESUME access right
			//	dwSusp = SuspendThread(gpSrv->hRootThread);
			//	if (dwSusp == (DWORD)-1) dwSuspErr = GetLastError();
			//}
			SetConsoleSize(gpSrv->nReqSizeBufferHeight, gpSrv->crReqSizeNewSize, gpSrv->rReqSizeNewRect, gpSrv->sReqSizeLabel, gpSrv->bReqSizeForceLog);
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

		// While station is locked - no sense to scan console contents
		if (gpSrv->bStationLocked)
		{
			nWait = WaitForSingleObject(ghQuitEvent, 50);
			if (nWait == WAIT_OBJECT_0)
			{
				break; // Server stop was requested
			}
			// Skip until station will be unlocked
			continue;
		}

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
				if (nEvtCount < countof(hEvents))
				{
					nFarCommit = nEvtCount;
					hEvents[nEvtCount++] = gpSrv->hFarCommitEvent;
					nWaitTimeout = 2500; // No need to force console scanning, Far & ExtendedConsole.dll takes care
				}
				else
				{
					_ASSERTE(nEvtCount < countof(hEvents));
				}
			}
			if (gpSrv->bCursorChangeRegistered)
			{
				if (nEvtCount < countof(hEvents))
				{
					nCursorChanged = nEvtCount;
					hEvents[nEvtCount++] = gpSrv->hCursorChangeEvent;
				}
				else
				{
					_ASSERTE(nEvtCount < countof(hEvents));
				}
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
		}

		nLastConsoleActiveTick = gpSrv->nLastConsoleActiveTick;
		nLastConsoleActiveDelta = GetTickCount() - nLastConsoleActiveTick;
		if (!nLastConsoleActiveTick || (nLastConsoleActiveDelta >= REFRESH_FELL_SLEEP_TIMEOUT))
		{
			ReloadGuiSettings(NULL);
			BOOL lbDCWndVisible = bDCWndVisible;
			bDCWndVisible = (IsWindowVisible(ghConEmuWndDC) != FALSE);
			gpSrv->nLastConsoleActiveTick = GetTickCount();

			if (gpLogSize && (bDCWndVisible != lbDCWndVisible))
			{
				LogString(bDCWndVisible ? L"bDCWndVisible changed to true" : L"bDCWndVisible changed to false");
			}
		}

		BOOL lbOurConActive = FALSE, lbOneConActive = FALSE;
		for (size_t i = 0; i < countof(gpSrv->guiSettings.Consoles); i++)
		{
			ConEmuConsoleInfo& ci = gpSrv->guiSettings.Consoles[i];
			if ((ci.Flags & ccf_Active) && (HWND)ci.Console)
			{
				lbOneConActive = TRUE;
				if (ghConWnd == (HWND)ci.Console)
				{
					lbOurConActive = TRUE;
					break;
				}
			}
		}
		if (gpLogSize && (bOurConActive != lbOurConActive))
			LogString(lbOurConActive ? L"bOurConActive changed to true" : L"bOurConActive changed to false");
		bOurConActive = lbOurConActive;
		if (gpLogSize && (bOneConActive != lbOneConActive))
			LogString(lbOneConActive ? L"bOneConActive changed to true" : L"bOneConActive changed to false");
		bOneConActive = lbOneConActive;

		ActiveSleepInBg = (gpSrv->guiSettings.Flags & CECF_SleepInBackg);
		RetardNAPanes = (gpSrv->guiSettings.Flags & CECF_RetardNAPanes);

		BOOL lbNewActive;
		if (bOurConActive || (bDCWndVisible && !RetardNAPanes))
		{
			// Mismatch may appears during console closing
			//if (gpLogSize && gbInShutdown && (bDCWndVisible != bOurConActive))
			//	LogString(L"bDCWndVisible and bOurConActive mismatch");
			lbNewActive = gpSrv->guiSettings.bGuiActive || !ActiveSleepInBg;
		}
		else
		{
			lbNewActive = FALSE;
		}
		if (gpLogSize && (bNewActive != lbNewActive))
			LogString(lbNewActive ? L"bNewActive changed to true" : L"bNewActive changed to false");
		bNewActive = lbNewActive;

		BOOL lbNewFellInSleep = ActiveSleepInBg && !bNewActive;
		if (gpLogSize && (bNewFellInSleep != lbNewFellInSleep))
			LogString(lbNewFellInSleep ? L"bNewFellInSleep changed to true" : L"bNewFellInSleep changed to false");
		bNewFellInSleep = lbNewFellInSleep;

		if ((bNewActive != bConsoleActive) || (bNewFellInSleep != bFellInSleep))
		{
			bConsoleActive = bNewActive;
			bFellInSleep = bNewFellInSleep;

			bLowSpeed = (!bNewActive || bNewFellInSleep);
			InputLogger::Log(bLowSpeed ? InputLogger::Event::evt_SpeedLow : InputLogger::Event::evt_SpeedHigh);

			if (gpLogSize)
			{
				char szInfo[128];
				_wsprintfA(szInfo, SKIPLEN(countof(szInfo)) "ConEmuC: RefreshThread: Sleep changed, speed(%s)",
					bLowSpeed ? "low" : "high");
				LogString(szInfo);
			}
		}


		// Обновляется по таймауту
		SleepType = CheckIndicateSleepNum();


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

			if (SleepType)
			{
				// Выключить индикатор (low speed)
				ShowSleepIndicator(SleepType, true);
			}

			// #define MAX_FORCEREFRESH_INTERVAL 500
			if (nDelta <= MAX_FORCEREFRESH_INTERVAL)
			{
				// Чтобы не грузить процессор
				continue;
			}
		}
		else if (SleepType)
		{
			ShowSleepIndicator(SleepType, false);
		}


		#ifdef _DEBUG
		if (nWait == nRefreshEventId)
		{
			DEBUGSTR(L"*** hRefreshEvent was set, checking console...\n");
		}
		#endif

		// GUI was crashed or was detached?
		if (ghConEmuWndDC && isConEmuTerminated())
		{
			gpSrv->bWasDetached = TRUE;
			SetConEmuWindows(NULL, NULL, NULL);
			_ASSERTE(!ghConEmuWnd && !ghConEmuWndDC && !ghConEmuWndBack);
			gnConEmuPID = 0;
			UpdateConsoleMapHeader(L"RefreshThread: GUI was crashed or was detached?");
			EmergencyShow(ghConWnd);
		}

		// Reattach?
		if (!ghConEmuWndDC && gpSrv->bWasDetached && (gnRunMode == RM_ALTSERVER))
		{
			CESERVER_CONSOLE_MAPPING_HDR* pMap = gpSrv->pConsoleMap->Ptr();
			if (pMap && pMap->hConEmuWndDc && IsWindow(pMap->hConEmuWndDc))
			{
				// Reset GUI HWND's
				_ASSERTE(!gnConEmuPID);
				SetConEmuWindows(pMap->hConEmuRoot, pMap->hConEmuWndDc, pMap->hConEmuWndBack);
				_ASSERTE(gnConEmuPID && ghConEmuWnd && ghConEmuWndDC && ghConEmuWndBack);

				// To be sure GUI will be updated with full info
				gpSrv->pConsole->bDataChanged = TRUE;
			}
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
		if (!nAltWait && !gpSrv->DbgInfo.bDebuggerActive)
		{
			if (pfnGetConsoleKeyboardLayoutName)
				CheckKeyboardLayout();
		}

		/* ****************** */
		/* Перечитать консоль */
		/* ****************** */
		// 120507 - Если крутится альт.сервер - то игнорировать
		if (!nAltWait && !gpSrv->DbgInfo.bDebuggerActive)
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

	for (int i = 1; i < pRgn->nRectCount; i++)
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
