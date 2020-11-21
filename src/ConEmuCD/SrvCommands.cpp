
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

#undef VALIDATE_AND_DELAY_ON_TERMINATE
#define SHOWDEBUGSTR
#define DEBUGSTRCMD(x) DEBUGSTR(x)
#define DEBUGSTARTSTOPBOX(x) //MessageBox(NULL, x, WIN3264TEST(L"ConEmuC",L"ConEmuC64"), MB_ICONINFORMATION|MB_SYSTEMMODAL)
#define DEBUGSTRFIN(x) DEBUGSTR(x)
#define DEBUGSTRCP(x) DEBUGSTR(x)
#define DEBUGSTRSIZE(x) DEBUGSTR(x)

//#define SHOW_INJECT_MSGBOX
//#define SHOW_FAR_PLUGIN_LOADED_MSGBOX

#include "ConsoleMain.h"
#include "ConEmuSrv.h"
#include "../common/ConsoleMixAttr.h"
#include "../common/ConsoleRead.h"
#include "../common/EmergencyShow.h"
#include "../common/MArray.h"
#include "../common/MMap.h"
#include "../common/MSectionSimple.h"
#include "../common/RConStartArgs.h"
#include "../common/StartupEnvEx.h"
#include "../common/WConsole.h"
#include "../common/WUser.h"
#include "../ConEmu/version.h"
#include "Actions.h"
#include "ConAnsi.h"
#include "ConProcess.h"
#include "ConsoleArgs.h"
#include "ConsoleState.h"
#include "RunMode.h"
#include "../common/WErrGuard.h"


/* ********************************** */

// Far process console resizes only after any mouse event
// (Probably, to avoid artifacts/problems in "far /w" mode?)
void ForceFarResize()
{
	CONSOLE_SCREEN_BUFFER_INFO sc = {};
	INPUT_RECORD r = { MOUSE_EVENT };
	r.Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
	if (GetConsoleScreenBufferInfo(ghConOut, &sc))
	{
		r.Event.MouseEvent.dwMousePosition.X = sc.srWindow.Right - 1;
		r.Event.MouseEvent.dwMousePosition.Y = sc.srWindow.Bottom - 1;
	}

	wchar_t szLog[120];
	swprintf_c(szLog, L"ForceFarResize: Size={%i,%i} Buf={%i,%i} Mouse={%i,%i}",
		sc.srWindow.Right-sc.srWindow.Left+1, sc.srWindow.Bottom-sc.srWindow.Top+1,
		sc.dwSize.X, sc.dwSize.Y,
		r.Event.MouseEvent.dwMousePosition.X, r.Event.MouseEvent.dwMousePosition.Y);
	LogString(szLog);

	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	DWORD cbWritten = 0;
	WriteConsoleInput(hIn, &r, 1, &cbWritten);
}

bool TerminateOneProcess(DWORD nPID, DWORD& nErrCode)
{
	bool lbRc = false;
	bool bNeedClose = false;
	HANDLE hProcess = NULL;

	if (gpSrv && gpSrv->pConsole)
	{
		if (nPID == gpWorker->RootProcessId())
		{
			hProcess = gpWorker->RootProcessHandle();
			bNeedClose = FALSE;
		}
	}

	if (!hProcess)
	{
		hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, nPID);
		if (hProcess != NULL)
		{
			bNeedClose = TRUE;
		}
		else
		{
			nErrCode = GetLastError();
		}
	}

	if (hProcess != NULL)
	{
		if (TerminateProcess(hProcess, 100))
		{
			lbRc = true;
		}
		else
		{
			nErrCode = GetLastError();
			if (!bNeedClose)
			{
				hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, nPID);
				if (hProcess != NULL)
				{
					bNeedClose = true;
					if (TerminateProcess(hProcess, 100))
						lbRc = true;
				}
			}
		}
	}

	if (hProcess && bNeedClose)
		CloseHandle(hProcess);

	return lbRc;
}

bool TerminateProcessGroup(int nCount, LPDWORD pPID, DWORD& nErrCode)
{
	bool lbRc = false;
	HANDLE hJob, hProcess;

	if ((hJob = CreateJobObject(NULL, NULL)) == NULL)
	{
		// Job failed? Do it one-by-one
		for (int i = nCount-1; i >= 0; i--)
		{
			lbRc = TerminateOneProcess(pPID[i], nErrCode);
		}
	}
	else
	{
		MArray<HANDLE> hh;

		for (int i = 0; i < nCount; i++)
		{
			if (pPID[i] == gpWorker->RootProcessId())
				continue;

			hProcess = OpenProcess(PROCESS_TERMINATE|PROCESS_SET_QUOTA, FALSE, pPID[i]);
			if (!hProcess) continue;

			if (AssignProcessToJobObject(hJob, hProcess))
			{
				hh.push_back(hProcess);
			}
			else
			{
				// Strange, can't assign to jub? Do it manually
				if (TerminateProcess(hProcess, 100))
					lbRc = true;
				CloseHandle(hProcess);
			}
		}

		if (!hh.empty())
		{
			lbRc = (TerminateJobObject(hJob, 100) != FALSE);

			while (hh.pop_back(hProcess))
			{
				if (!lbRc) // If job failed - last try
					TerminateProcess(hProcess, 100);
				CloseHandle(hProcess);
			}
		}

		CloseHandle(hJob);
	}

	return lbRc;
}




/* ********************************** */

BOOL cmd_SetSizeXXX_CmdStartedFinished(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;

	bool bForceWriteLog = false;
	char szCmdName[40];
	switch (in.hdr.nCmd)
	{
	case CECMD_SETSIZESYNC:
		DEBUGSTRSIZE(L"CECMD_SETSIZESYNC");
		lstrcpynA(szCmdName, ":CECMD_SETSIZESYNC", countof(szCmdName)); break;
	case CECMD_SETSIZENOSYNC:
		DEBUGSTRSIZE(L"CECMD_SETSIZENOSYNC");
		lstrcpynA(szCmdName, ":CECMD_SETSIZENOSYNC", countof(szCmdName)); break;
	case CECMD_CMDSTARTED:
		DEBUGSTRSIZE(L"CECMD_CMDSTARTED");
		lstrcpynA(szCmdName, ":CECMD_CMDSTARTED", countof(szCmdName)); break;
	case CECMD_CMDFINISHED:
		DEBUGSTRSIZE(L"CECMD_CMDFINISHED");
		lstrcpynA(szCmdName, ":CECMD_CMDFINISHED", countof(szCmdName)); break;
	case CECMD_UNLOCKSTATION:
		DEBUGSTRSIZE(L"CECMD_UNLOCKSTATION");
		lstrcpynA(szCmdName, ":CECMD_UNLOCKSTATION", countof(szCmdName)); bForceWriteLog = true; break;
	default:
		_ASSERTE(FALSE && "Unnamed command");
		sprintf_c(szCmdName, ":UnnamedCmd(%u)", in.hdr.nCmd);
	}

	MCHKHEAP;
	int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_RETSIZE);
	*out = ExecuteNewCmd(0,nOutSize);

	if (*out == NULL) return FALSE;

	MCHKHEAP;

	// Need to block all requests to output buffer in other threads
	MSectionLockSimple csRead = gpWorker->LockConsoleReaders(LOCK_READOUTPUT_TIMEOUT);

	DWORD nTick1 = 0, nTick2 = 0, nTick3 = 0, nTick4 = 0, nTick5 = 0;
	DWORD nWasSetSize = -1;

	if (in.hdr.cbSize >= (sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_SETSIZE)))
	{
		USHORT nBufferHeight = 0;
		COORD  crNewSize = {0,0};
		SMALL_RECT rNewRect = {0};
		//SHORT  nNewTopVisible = -1;
		//memmove(&nBufferHeight, in.Data, sizeof(USHORT));
		nBufferHeight = in.SetSize.nBufferHeight;

		if (nBufferHeight == (USHORT)-1)
		{
			// Для 'far /w' нужно оставить высоту буфера!
			if ((in.SetSize.size.Y < gpWorker->GetSbi().dwSize.Y)
				&& gpWorker->GetSbi().dwSize.Y >= SRectHeight(gpWorker->GetSbi().srWindow))
			{
				nBufferHeight = gpWorker->GetSbi().dwSize.Y;
			}
			else
			{
				nBufferHeight = 0;
			}
		}

		crNewSize = in.SetSize.size;

		//// rect по идее вообще не нужен, за блокировку при прокрутке отвечает nSendTopLine
		//rNewRect = in.SetSize.rcWindow;

		MCHKHEAP;
		(*out)->hdr.nCmd = in.hdr.nCmd;
		// Все остальные поля заголовка уже заполнены в ExecuteNewCmd

		//#ifdef _DEBUG
		if (in.hdr.nCmd == CECMD_CMDFINISHED)
		{
			PRINT_COMSPEC(L"CECMD_CMDFINISHED, Set height to: %i\n", crNewSize.Y);
			DEBUGSTRSIZE(L"\n!!! CECMD_CMDFINISHED !!!\n\n");
			// Вернуть нотификатор
			TODO("Смена режима рефреша консоли")
			//if (gpSrv->dwWinEventThread != 0)
			//	PostThreadMessage(gpSrv->dwWinEventThread, gpSrv->nMsgHookEnableDisable, TRUE, 0);
		}
		else if (in.hdr.nCmd == CECMD_CMDSTARTED)
		{
			PRINT_COMSPEC(L"CECMD_CMDSTARTED, Set height to: %i\n", nBufferHeight);
			DEBUGSTRSIZE(L"\n!!! CECMD_CMDSTARTED !!!\n\n");
			// Отключить нотификатор
			TODO("Смена режима рефреша консоли")
			//if (gpSrv->dwWinEventThread != 0)
			//	PostThreadMessage(gpSrv->dwWinEventThread, gpSrv->nMsgHookEnableDisable, FALSE, 0);
		}

		//#endif

		if (in.hdr.nCmd == CECMD_CMDFINISHED)
		{
			// Сохранить данные ВСЕЙ консоли
			PRINT_COMSPEC(L"Storing long output\n", 0);
			WorkerServer::Instance().CmdOutputStore();
			PRINT_COMSPEC(L"Storing long output (done)\n", 0);
		}

		MCHKHEAP;

		if (in.hdr.nCmd == CECMD_SETSIZESYNC)
		{
			ResetEvent(gpSrv->hAllowInputEvent);
			gpSrv->bInSyncResize = TRUE;
		}

		nTick1 = GetTickCount();
		csRead.Unlock();
		WARNING("Если указан dwFarPID - это что-ли два раза подряд выполнится?");
		// rNewRect по идее вообще не нужен, за блокировку при прокрутке отвечает nSendTopLine
		nWasSetSize = gpWorker->SetConsoleSize(nBufferHeight, crNewSize, rNewRect, szCmdName, bForceWriteLog);
		WARNING("!! Не может ли возникнуть конфликт с фаровским фиксом для убирания полос прокрутки?");
		nTick2 = GetTickCount();

		// вернуть блокировку
		csRead = gpWorker->LockConsoleReaders(LOCK_READOUTPUT_TIMEOUT);

		if (in.hdr.nCmd == CECMD_SETSIZENOSYNC)
		{
			if (WorkerServer::Instance().GetLastActiveFarPid())
			{
				ForceFarResize();
			}
		} else
		if (in.hdr.nCmd == CECMD_SETSIZESYNC)
		{
			CESERVER_REQ *pPlgIn = NULL, *pPlgOut = NULL;

			//TODO("Пока закомментарим, чтобы GUI реагировало побыстрее");
			if (in.SetSize.dwFarPID /*&& !nBufferHeight*/)
			{
				// Во избежание каких-то накладок FAR (по крайней мере с /w)
				// стал ресайзить панели только после дерганья мышкой над консолью

				ForceFarResize();

				csRead.Unlock();

				// Команду можно выполнить через плагин FARа
				wchar_t szPipeName[128];
				swprintf_c(szPipeName, CEPLUGINPIPENAME, L".", in.SetSize.dwFarPID);
				//DWORD nHILO = ((DWORD)crNewSize.X) | (((DWORD)(WORD)crNewSize.Y) << 16);
				//pPlgIn = ExecuteNewCmd(CMD_SETSIZE, sizeof(CESERVER_REQ_HDR)+sizeof(nHILO));
				pPlgIn = ExecuteNewCmd(CMD_REDRAWFAR, sizeof(CESERVER_REQ_HDR));
				//pPlgIn->dwData[0] = nHILO;
				pPlgOut = ExecuteCmd(szPipeName, pPlgIn, 500, gState.realConWnd_);

				if (pPlgOut) ExecuteFreeResult(pPlgOut);
			}

			SetEvent(gpSrv->hAllowInputEvent);
			gpSrv->bInSyncResize = FALSE;

			// ReloadFullConsoleInfo - передает управление в Refresh thread,
			// поэтому блокировку нужно предварительно снять
			csRead.Unlock();

			// Передернуть RefreshThread - перечитать консоль
			WorkerServer::Instance().ReloadFullConsoleInfo(FALSE); // вызовет Refresh в Refresh thread

			// вернуть блокировку
			csRead = gpWorker->LockConsoleReaders(LOCK_READOUTPUT_TIMEOUT);
		}

		MCHKHEAP;

		if (in.hdr.nCmd == CECMD_CMDSTARTED)
		{
			// Восстановить текст скрытой (прокрученной вверх) части консоли
			// #LongConsoleOutput This does not work
			WorkerServer::Instance().CmdOutputRestore(false);
		}
	}

	MCHKHEAP;

	nTick3 = GetTickCount();

	// already blocked
	//csRead.Lock(&gpSrv->csReadConsoleInfo, LOCK_READOUTPUT_TIMEOUT);

	// Prepare result
	(*out)->SetSizeRet.crMaxSize = MyGetLargestConsoleWindowSize(ghConOut);

	nTick4 = GetTickCount();

	PCONSOLE_SCREEN_BUFFER_INFO psc = &((*out)->SetSizeRet.SetSizeRet);
	MyGetConsoleScreenBufferInfo(ghConOut, psc);

	nTick5 = GetTickCount();

	DWORD lnNextPacketId = (DWORD)InterlockedIncrement(&gpSrv->nLastPacketID);
	(*out)->SetSizeRet.nNextPacketId = lnNextPacketId;

	//gpSrv->bForceFullSend = TRUE;
	SetEvent(gpSrv->hRefreshEvent);
	MCHKHEAP;
	lbRc = TRUE;

	UNREFERENCED_PARAMETER(nTick1);
	UNREFERENCED_PARAMETER(nTick2);
	UNREFERENCED_PARAMETER(nTick3);
	UNREFERENCED_PARAMETER(nTick4);
	UNREFERENCED_PARAMETER(nTick5);
	UNREFERENCED_PARAMETER(nWasSetSize);
	return lbRc;
}

#if 0
BOOL cmd_GetOutput(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;
	MSectionLock CS; CS.Lock(gpcsStoredOutput, FALSE);

	if (gpStoredOutput)
	{
		_ASSERTE(sizeof(CESERVER_CONSAVE_HDR) > sizeof(gpStoredOutput->hdr.hdr));
		DWORD nSize = sizeof(CESERVER_CONSAVE_HDR)
		              + std::min((int)gpStoredOutput->hdr.cbMaxOneBufferSize,
		                    (gpStoredOutput->hdr.sbi.dwSize.X*gpStoredOutput->hdr.sbi.dwSize.Y*2));
		*out = ExecuteNewCmd(CECMD_GETOUTPUT, nSize);
		if (*out)
		{
			memmove((*out)->Data, ((LPBYTE)gpStoredOutput) + sizeof(gpStoredOutput->hdr.hdr), nSize - sizeof(gpStoredOutput->hdr.hdr));
			lbRc = TRUE;
		}
	}
	else
	{
		*out = ExecuteNewCmd(CECMD_GETOUTPUT, sizeof(CESERVER_REQ_HDR));
		lbRc = (*out != NULL);
	}

	return lbRc;
}
#endif

// Запрос к серверу - "Подцепись в ConEmu GUI".
// Может прийти из
// * GUI (диалог аттача)
// * из Far плагина (меню или команда "Attach to ConEmu")
// * из "ConEmuC /ATTACH"
BOOL cmd_Attach2Gui(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;

	// If command comes from GUI - update GUI HWND
	// Else - gState.hGuiWnd will be set to NULL and suitable HWND will be found in Attach2Gui
	gState.hGuiWnd = FindConEmuByPID(in.hdr.nSrcPID);

	// Может придти из Attach2Gui() плагина
	HWND hDc = WorkerServer::Instance().Attach2Gui(ATTACH2GUI_TIMEOUT);

	if (hDc != NULL)
	{
		int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD);
		*out = ExecuteNewCmd(CECMD_ATTACH2GUI,nOutSize);

		if (*out != NULL)
		{
			// Чтобы не отображалась "Press any key to close console"
			gState.DisableAutoConfirmExit();
			//
			(*out)->dwData[0] = LODWORD(hDc); //-V205 // Дескриптор окна
			lbRc = TRUE;

			gState.bWasReattached_ = TRUE;
			if (gpSrv->hRefreshEvent)
			{
				SetEvent(gpSrv->hRefreshEvent);
			}
			else
			{
				_ASSERTE(gpSrv->hRefreshEvent!=NULL);
			}
		}
	}

	return lbRc;
}

BOOL cmd_FarLoaded(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;

	#ifdef SHOW_FAR_PLUGIN_LOADED_MSGBOX
	wchar_t szDbg[512], szExe[MAX_PATH], szTitle[512];
	GetModuleFileName(NULL, szExe, countof(szExe));
	swprintf_c(szTitle, L"cmd_FarLoaded: %s", PointToName(szExe));
	swprintf_c(szDbg,
		L"cmd_FarLoaded was received\nServerPID=%u, name=%s\nFarPID=%u\nRootPID=%u\nDisablingConfirm=%s",
		GetCurrentProcessId(), PointToName(szExe), in.hdr.nSrcPID, gpWorker->RootProcessId(),
		((gState.autoDisableConfirmExit_ || (gpConsoleArgs->confirmExitParm_ == 1)) && gpWorker->RootProcessId() == in.dwData[0]) ? L"YES" :
		gState.alwaysConfirmExit_ ? L"AlreadyOFF" : L"NO");
	MessageBox(NULL, szDbg, szTitle, MB_SYSTEMMODAL);
	#endif

	// If far.exe was started and "ConEmu plugin" was successfully loaded
	// we can disable "Press Enter or Esc to exit" confirmation as Far "works properly"
	if (gpWorker->RootProcessId() == in.dwData[0])
	{
		gState.DisableAutoConfirmExit(true);
	}

	const DWORD nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD);
	*out = ExecuteNewCmd(CECMD_FARLOADED, nOutSize);
	if (*out != nullptr)
	{
		(*out)->dwData[0] = GetCurrentProcessId();
		lbRc = TRUE;
	}

	return lbRc;
}

BOOL cmd_PostConMsg(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;

	HWND hSendWnd = (HWND)in.Msg.hWnd;

	if ((in.Msg.nMsg == WM_CLOSE) && (hSendWnd == gState.realConWnd_))
	{
		// Чтобы при закрытии не _мелькало_ "Press Enter to Close console"
		gbInShutdown = TRUE;
		LogString(L"WM_CLOSE posted to console window, termination was requested");
	}

	// Info & Log
	if (in.Msg.nMsg == WM_INPUTLANGCHANGE || in.Msg.nMsg == WM_INPUTLANGCHANGEREQUEST)
	{
		WPARAM wParam = (WPARAM)in.Msg.wParam;
		LPARAM lParam = (LPARAM)in.Msg.lParam;
		unsigned __int64 l = lParam;

		#ifdef _DEBUG
		wchar_t szDbg[255];
		swprintf_c(szDbg, L"ConEmuC: %s(0x%08X, %s, CP:%i, HKL:0x%08I64X)\n",
		          in.Msg.bPost ? L"PostMessage" : L"SendMessage", LODWORD(hSendWnd),
		          (in.Msg.nMsg == WM_INPUTLANGCHANGE) ? L"WM_INPUTLANGCHANGE" :
		          (in.Msg.nMsg == WM_INPUTLANGCHANGEREQUEST) ? L"WM_INPUTLANGCHANGEREQUEST" :
		          (in.Msg.nMsg == WM_SHOWWINDOW) ? L"WM_SHOWWINDOW" :
		          L"<Other message>",
		          LODWORD(wParam), l);
		DEBUGLOGLANG(szDbg);
		#endif

		if (gpLogSize)
		{
			char szInfo[255];
			sprintf_c(szInfo, "ConEmuC: %s(0x%08X, %s, CP:%i, HKL:0x%08I64X)",
			           in.Msg.bPost ? "PostMessage" : "SendMessage", LODWORD(hSendWnd), //-V205
			           (in.Msg.nMsg == WM_INPUTLANGCHANGE) ? "WM_INPUTLANGCHANGE" :
			           (in.Msg.nMsg == WM_INPUTLANGCHANGEREQUEST) ? "WM_INPUTLANGCHANGEREQUEST" :
			           "<Other message>",
			           (DWORD)wParam, l);
			LogString(szInfo);
		}
	}

	if (in.Msg.nMsg == WM_SHOWWINDOW)
	{
		DWORD lRc = 0;

		if (in.Msg.bPost)
			lRc = apiShowWindowAsync(hSendWnd, (int)(in.Msg.wParam & 0xFFFF));
		else
			lRc = apiShowWindow(hSendWnd, (int)(in.Msg.wParam & 0xFFFF));

		// Возвращаем результат
		DWORD dwErr = GetLastError();
		int nOutSize = sizeof(CESERVER_REQ_HDR) + 2*sizeof(DWORD);
		*out = ExecuteNewCmd(CECMD_POSTCONMSG,nOutSize);

		if (*out != NULL)
		{
			(*out)->dwData[0] = lRc;
			(*out)->dwData[1] = dwErr;
			lbRc = TRUE;
		}
	}
	else if (in.Msg.nMsg == WM_SIZE)
	{
		//
	}
	else if (in.Msg.bPost)
	{
		PostMessage(hSendWnd, in.Msg.nMsg, (WPARAM)in.Msg.wParam, (LPARAM)in.Msg.lParam);
	}
	else
	{
		LRESULT lRc = SendMessage(hSendWnd, in.Msg.nMsg, (WPARAM)in.Msg.wParam, (LPARAM)in.Msg.lParam);
		// Возвращаем результат
		int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(uint64_t);
		*out = ExecuteNewCmd(CECMD_POSTCONMSG,nOutSize);

		if (*out != NULL)
		{
			(*out)->qwData[0] = lRc;
			lbRc = TRUE;
		}
	}

	return lbRc;
}

BOOL cmd_SaveAliases(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;

	//wchar_t* pszAliases; DWORD nAliasesSize;
	// Запомнить алиасы
	DWORD nNewSize = in.hdr.cbSize - sizeof(in.hdr);

	if (nNewSize > gpSrv->nAliasesSize)
	{
		MCHKHEAP;
		wchar_t* pszNew = (wchar_t*)calloc(nNewSize, 1);

		if (!pszNew)
			goto wrap;  // не удалось выделить память

		MCHKHEAP;

		if (gpSrv->pszAliases) free(gpSrv->pszAliases);

		MCHKHEAP;
		gpSrv->pszAliases = pszNew;
		gpSrv->nAliasesSize = nNewSize;
	}

	if (nNewSize > 0 && gpSrv->pszAliases)
	{
		MCHKHEAP;
		memmove(gpSrv->pszAliases, in.Data, nNewSize);
		MCHKHEAP;
	}

wrap:
	return lbRc;
}

BOOL cmd_GetAliases(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;

	// Возвращаем запомненные алиасы
	int nOutSize = sizeof(CESERVER_REQ_HDR) + gpSrv->nAliasesSize;
	*out = ExecuteNewCmd(CECMD_GETALIASES,nOutSize);

	if (*out != NULL)
	{
		if (gpSrv->pszAliases && gpSrv->nAliasesSize)
		{
			memmove((*out)->Data, gpSrv->pszAliases, gpSrv->nAliasesSize);
		}
		lbRc = TRUE;
	}

	return lbRc;
}

BOOL cmd_FarDetached(CESERVER_REQ& in, CESERVER_REQ** out)
{
	const BOOL lbRc = FALSE;

	WorkerServer::Instance().OnFarDetached(in.hdr.nSrcPID);

	return lbRc;
}

BOOL cmd_OnActivation(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;

	if (gpSrv)
	{
		LogString(L"CECMD_ONACTIVATION received, state update pending");

		// Принудить RefreshThread перечитать статус активности консоли
		gpSrv->nLastConsoleActiveTick = 0;

		auto& server = WorkerServer::Instance();

		if (server.GetAltServerPid() && (server.GetAltServerPid() != gnSelfPID))
		{
			CESERVER_REQ* pAltIn = ExecuteNewCmd(CECMD_ONACTIVATION, sizeof(CESERVER_REQ_HDR));
			if (pAltIn)
			{
				CESERVER_REQ* pAltOut = ExecuteSrvCmd(server.GetAltServerPid(), pAltIn, gState.realConWnd_);
				ExecuteFreeResult(pAltIn);
				ExecuteFreeResult(pAltOut);
			}
		}
		else
		{
			// Warning: If refresh thread is in an AltServerStop
			// transaction, ReloadFullConsoleInfo with (TRUE) will deadlock.
			WorkerServer::Instance().ReloadFullConsoleInfo(FALSE);
			// Force refresh thread to cycle
			SetEvent(gpSrv->hRefreshEvent);
		}
	}

	return lbRc;
}

BOOL cmd_SetWindowPos(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;
	BOOL lbWndRc = FALSE, lbShowRc = FALSE;
	DWORD nErrCode[2] = {};

	if ((in.SetWndPos.hWnd == gState.realConWnd_) && gpLogSize)
		LogSize(NULL, 0, ":SetWindowPos.before");

	if (!(in.SetWndPos.uFlags & (SWP_NOMOVE|SWP_NOSIZE)))
		SendMessage(in.SetWndPos.hWnd, WM_ENTERSIZEMOVE, 0, 0);

	lbWndRc = SetWindowPos(in.SetWndPos.hWnd, in.SetWndPos.hWndInsertAfter,
	             in.SetWndPos.X, in.SetWndPos.Y, in.SetWndPos.cx, in.SetWndPos.cy,
	             in.SetWndPos.uFlags);
	nErrCode[0] = lbWndRc ? 0 : GetLastError();

	if (!(in.SetWndPos.uFlags & (SWP_NOMOVE | SWP_NOSIZE)))
		SendMessage(in.SetWndPos.hWnd, WM_EXITSIZEMOVE, 0, 0);

	if ((in.SetWndPos.uFlags & SWP_SHOWWINDOW) && !IsWindowVisible(in.SetWndPos.hWnd))
	{
		lbShowRc = apiShowWindowAsync(in.SetWndPos.hWnd, SW_SHOW);
		nErrCode[1] = lbShowRc ? 0 : GetLastError();
	}

	if ((in.SetWndPos.hWnd == gState.realConWnd_) && gpLogSize)
		LogSize(NULL, 0, ":SetWindowPos.after");

	// Результат
	int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD)*4;
	*out = ExecuteNewCmd(CECMD_SETWINDOWPOS,nOutSize);

	if (*out != NULL)
	{
		(*out)->dwData[0] = lbWndRc;
		(*out)->dwData[1] = nErrCode[0];
		(*out)->dwData[2] = lbShowRc;
		(*out)->dwData[3] = nErrCode[1];

		lbRc = TRUE;
	}

	return lbRc;
}

BOOL cmd_SetFocus(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE, lbForeRc = FALSE;
	HWND hFocusRc = NULL;

	if (in.setFocus.bSetForeground)
		lbForeRc = SetForegroundWindow(in.setFocus.hWindow);
	else
		hFocusRc = SetFocus(in.setFocus.hWindow);

	return lbRc;
}

BOOL cmd_SetParent(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE; //, lbForeRc = FALSE;

	HWND h = SetParent(in.setParent.hWnd, in.setParent.hParent);

	int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_SETPARENT);
	*out = ExecuteNewCmd(CECMD_SETPARENT,nOutSize);

	if (*out != NULL)
	{
		(*out)->setParent.hWnd.u = GetLastError();
		(*out)->setParent.hParent = h;

		lbRc = TRUE;
	}

	return lbRc;
}

BOOL cmd_SetWindowRgn(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;

	WorkerServer::Instance().MySetWindowRgn(&in.SetWndRgn);

	return lbRc;
}

BOOL cmd_DetachCon(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;

	//if (gOSVer.dwMajorVersion >= 6)
	//{
	//	if ((gcrVisibleSize.X <= 0) && (gcrVisibleSize.Y <= 0))
	//	{
	//		_ASSERTE((gcrVisibleSize.X > 0) && (gcrVisibleSize.Y > 0));
	//	}
	//	else
	//	{
	//		int curSizeY, curSizeX;
	//		wchar_t sFontName[LF_FACESIZE];
	//		HANDLE hOutput = (HANDLE)ghConOut;

	//		if (apiGetConsoleFontSize(hOutput, curSizeY, curSizeX, sFontName) && curSizeY && curSizeX)
	//		{
	//			COORD crLargest = MyGetLargestConsoleWindowSize(hOutput);
	//			HMONITOR hMon = MonitorFromWindow(gState.realConWnd, MONITOR_DEFAULTTOPRIMARY);
	//			MONITORINFO mi = {sizeof(mi)};
	//			int nMaxX = 0, nMaxY = 0;
	//			if (GetMonitorInfo(hMon, &mi))
	//			{
	//				nMaxX = mi.rcWork.right - mi.rcWork.left - 2*GetSystemMetrics(SM_CXSIZEFRAME) - GetSystemMetrics(SM_CYCAPTION);
	//				nMaxY = mi.rcWork.bottom - mi.rcWork.top - 2*GetSystemMetrics(SM_CYSIZEFRAME);
	//			}
	//
	//			if ((nMaxX > 0) && (nMaxY > 0))
	//			{
	//				int nFontX = nMaxX  / gcrVisibleSize.X;
	//				int nFontY = nMaxY / gcrVisibleSize.Y;
	//				// Too large height?
	//				if (nFontY > 28)
	//				{
	//					nFontX = 28 * nFontX / nFontY;
	//					nFontY = 28;
	//				}

	//				// Evaluate default width for the font
	//				int nEvalX = EvaluateDefaultFontWidth(nFontY, sFontName);
	//				if (nEvalX > 0)
	//					nFontX = nEvalX;

	//				// Look in the registry?
	//				HKEY hk;
	//				DWORD nRegSize = 0, nLen;
	//				if (!RegOpenKeyEx(HKEY_CURRENT_USER, L"Console", 0, KEY_READ, &hk))
	//				{
	//					if (RegQueryValueEx(hk, L"FontSize", NULL, NULL, (LPBYTE)&nRegSize, &(nLen=sizeof(nRegSize))) || nLen!=sizeof(nRegSize))
	//						nRegSize = 0;
	//					RegCloseKey(hk);
	//				}
	//				if (!nRegSize && !RegOpenKeyEx(HKEY_CURRENT_USER, L"Console\\%SystemRoot%_system32_cmd.exe", 0, KEY_READ, &hk))
	//				{
	//					if (RegQueryValueEx(hk, L"FontSize", NULL, NULL, (LPBYTE)&nRegSize, &(nLen=sizeof(nRegSize))) || nLen!=sizeof(nRegSize))
	//						nRegSize = 0;
	//					RegCloseKey(hk);
	//				}
	//				if ((HIWORD(nRegSize) > curSizeY) && (HIWORD(nRegSize) < nFontY)
	//					&& (LOWORD(nRegSize) > curSizeX) && (LOWORD(nRegSize) < nFontX))
	//				{
	//					nFontY = HIWORD(nRegSize);
	//					nFontX = LOWORD(nRegSize);
	//				}

	//				if ((nFontX > curSizeX) || (nFontY > curSizeY))
	//				{
	//					apiSetConsoleFontSize(hOutput, nFontY, nFontX, sFontName);
	//				}
	//			}
	//		}
	//	}
	//}

	gState.bWasDetached_ = TRUE;
	#ifdef _DEBUG
	g_IgnoreSetLargeFont = true;
	#endif
	gState.conemuWnd_ = NULL;
	WorkerServer::Instance().SetConEmuWindows(nullptr, nullptr, nullptr);
	gState.conemuPid_ = 0;
	WorkerServer::Instance().UpdateConsoleMapHeader(L"CECMD_DETACHCON");

	HWND hGuiApp = NULL;
	if (in.DataSize() >= sizeof(DWORD))
	{
		hGuiApp = (HWND)(DWORD_PTR)in.dwData[0];
		if (hGuiApp && !IsWindow(hGuiApp))
			hGuiApp = NULL;
	}

	if (in.DataSize() >= 2*sizeof(DWORD) && in.dwData[1])
	{
		if (hGuiApp)
			PostMessage(hGuiApp, WM_CLOSE, 0, 0);
		PostMessage(gState.realConWnd_, WM_CLOSE, 0, 0);
	}
	else
	{
		// Без мелькания консольного окошка почему-то пока не получается
		// Наверх выносится ConEmu вместо "отцепленного" GUI приложения
		EmergencyShow(gState.realConWnd_, (int)in.dwData[2], (int)in.dwData[3]);
	}

	if (hGuiApp != NULL)
	{
		gState.DisableAutoConfirmExit(FALSE);

		gpWorker->CloseRootProcessHandles();

		//apiShowWindow(gState.realConWnd, SW_SHOWMINIMIZED);
		apiSetForegroundWindow(hGuiApp);

		SetTerminateEvent(ste_CmdDetachCon);
	}

	gState.attachMode_ = am_None;

	int nOutSize = sizeof(CESERVER_REQ_HDR);
	*out = ExecuteNewCmd(CECMD_DETACHCON,nOutSize);
	lbRc = (*out != NULL);

	return lbRc;
}

BOOL cmd_CmdStartStop(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;

	MSectionLock CS; CS.Lock(gpWorker->Processes().csProc);

	const UINT nPrevCount = gpWorker->Processes().nProcessCount;
	BOOL lbChanged = FALSE;

	auto& server = WorkerServer::Instance();

	_ASSERTE(in.StartStop.dwPID!=0);
	const DWORD nPID = in.StartStop.dwPID;
	const DWORD nPrevAltServerPID = server.GetAltServerPid();

	if (!gpWorker->Processes().nProcessStartTick && (gpWorker->RootProcessId() == in.StartStop.dwPID))
	{
		gpWorker->Processes().nProcessStartTick = GetTickCount();
	}

	MSectionLock CsAlt;
	if (((in.StartStop.nStarted == sst_AltServerStop) || (in.StartStop.nStarted == sst_AppStop))
		&& in.StartStop.dwPID && (in.StartStop.dwPID == server.GetAltServerPid()))
	{
		// _ASSERTE могут приводить к ошибкам блокировки gpWorker->Processes().csProc в других потоках. Но ассертов быть не должно )
		_ASSERTE(GetCurrentThreadId() != server.GetRefreshThreadId());
		CsAlt.Lock(gpSrv->csAltSrv, TRUE, 1000);
	}

	wchar_t szDbg[128];

	switch (in.StartStop.nStarted)
	{
		case sst_ServerStart:
			swprintf_c(szDbg, L"SRV received CECMD_CMDSTARTSTOP(ServerStart,%i,PID=%u)", in.hdr.nCreateTick, in.StartStop.dwPID);
			break;
		case sst_AltServerStart:
			swprintf_c(szDbg, L"SRV received CECMD_CMDSTARTSTOP(AltServerStart,%i,PID=%u)", in.hdr.nCreateTick, in.StartStop.dwPID);
			break;
		case sst_ServerStop:
			swprintf_c(szDbg, L"SRV received CECMD_CMDSTARTSTOP(ServerStop,%i,PID=%u)", in.hdr.nCreateTick, in.StartStop.dwPID);
			break;
		case sst_AltServerStop:
			swprintf_c(szDbg, L"SRV received CECMD_CMDSTARTSTOP(AltServerStop,%i,PID=%u)", in.hdr.nCreateTick, in.StartStop.dwPID);
			break;
		case sst_ComspecStart:
			swprintf_c(szDbg, L"SRV received CECMD_CMDSTARTSTOP(ComspecStart,%i,PID=%u)", in.hdr.nCreateTick, in.StartStop.dwPID);
			break;
		case sst_ComspecStop:
			swprintf_c(szDbg, L"SRV received CECMD_CMDSTARTSTOP(ComspecStop,%i,PID=%u)", in.hdr.nCreateTick, in.StartStop.dwPID);
			break;
		case sst_AppStart:
			swprintf_c(szDbg, L"SRV received CECMD_CMDSTARTSTOP(AppStart,%i,PID=%u)", in.hdr.nCreateTick, in.StartStop.dwPID);
			break;
		case sst_AppStop:
			swprintf_c(szDbg, L"SRV received CECMD_CMDSTARTSTOP(AppStop,%i,PID=%u)", in.hdr.nCreateTick, in.StartStop.dwPID);
			break;
		case sst_App16Start:
			swprintf_c(szDbg, L"SRV received CECMD_CMDSTARTSTOP(App16Start,%i,PID=%u)", in.hdr.nCreateTick, in.StartStop.dwPID);
			break;
		case sst_App16Stop:
			swprintf_c(szDbg, L"SRV received CECMD_CMDSTARTSTOP(App16Stop,%i,PID=%u)", in.hdr.nCreateTick, in.StartStop.dwPID);
			break;
		default:
			// _ASSERTE могут приводить к ошибкам блокировки gpWorker->Processes().csProc в других потоках. Но ассертов быть не должно )
			_ASSERTE(in.StartStop.nStarted==sst_ServerStart && "Unknown StartStop code!");
	}

	if (gpLogSize) { LogString(szDbg); } else { DEBUGSTRCMD(szDbg); }
	DEBUGSTARTSTOPBOX(szDbg);

	// _ASSERTE могут приводить к ошибкам блокировки gpWorker->Processes().csProc в других потоках. Но ассертов быть не должно )
	_ASSERTE(in.StartStop.dwPID!=0);


	// Переменные, чтобы позвать функцию AltServerWasStarted после отпускания CS.Unlock()
	/*
	bool AltServerChanged = false;
	DWORD nAltServerWasStarted = 0, nAltServerWasStopped = 0;
	DWORD nCurAltServerPID = server.GetAltServerPid();
	HANDLE hAltServerWasStarted = NULL;
	bool ForceThawAltServer = false;
	AltServerInfo info = {};
	*/
	AltServerStartStop AS = {};
	AS.nCurAltServerPID = server.GetAltServerPid();


	if (in.StartStop.nStarted == sst_AltServerStart)
	{
		// Перевести нить монитора в режим ожидания завершения AltServer, инициализировать server.GetAltServerPid(), gpSrv->hAltServer
		// _ASSERTE могут приводить к ошибкам блокировки gpWorker->Processes().csProc в других потоках. Но ассертов быть не должно )
		_ASSERTE(in.StartStop.hServerProcessHandle!=0);

		server.OnAltServerChanged(1, sst_AltServerStart, in.StartStop.dwPID, &in.StartStop, AS);

	}
	else if ((in.StartStop.nStarted == sst_ComspecStart)
			|| (in.StartStop.nStarted == sst_AppStart))
	{
		// Добавить процесс в список
		// _ASSERTE могут приводить к ошибкам блокировки gpWorker->Processes().csProc в других потоках. Но ассертов быть не должно )
		_ASSERTE(gpWorker->Processes().pnProcesses[0] == gnSelfPID);
		lbChanged = gpWorker->Processes().ProcessAdd(nPID, CS);
	}
	else if ((in.StartStop.nStarted == sst_AltServerStop)
			|| (in.StartStop.nStarted == sst_ComspecStop)
			|| (in.StartStop.nStarted == sst_AppStop))
	{
		// Issue 623: Не удалять из списка корневой процесс _сразу_, пусть он "отвалится" обычным образом
		if (nPID != gpWorker->RootProcessId())
		{
			// Удалить процесс из списка
			// _ASSERTE могут приводить к ошибкам блокировки gpWorker->Processes().csProc в других потоках. Но ассертов быть не должно )
			_ASSERTE(gpWorker->Processes().pnProcesses[0] == gnSelfPID);
			lbChanged = gpWorker->Processes().ProcessRemove(nPID, nPrevCount, CS);
		}
		else
		{
			if (in.StartStop.bWasSucceededInRead)
			{
				// В консоли был успешный вызов ReadConsole/ReadConsoleInput.
				// Отключить "Press enter to close console".
				if (gState.autoDisableConfirmExit_ && (gpConsoleArgs->confirmExitParm_ == 0))
				{
					gState.DisableAutoConfirmExit(false);
				}
			}
		}

		DWORD nAltPID = (in.StartStop.nStarted == sst_ComspecStop) ? in.StartStop.nOtherPID : nPID;

		// Если это закрылся AltServer
		if (nAltPID)
		{
			server.OnAltServerChanged(1, in.StartStop.nStarted, nAltPID, NULL, AS);

			// _ASSERTE могут приводить к ошибкам блокировки gpWorker->Processes().csProc в других потоках. Но ассертов быть не должно )
			_ASSERTE(in.StartStop.nStarted==sst_ComspecStop || in.StartStop.nOtherPID==AS.info.nPrevPID);
		}
	}
	else
	{
		// _ASSERTE могут приводить к ошибкам блокировки gpWorker->Processes().csProc в других потоках. Но ассертов быть не должно )
		_ASSERTE(in.StartStop.nStarted==sst_AppStart || in.StartStop.nStarted==sst_AppStop || in.StartStop.nStarted==sst_ComspecStart || in.StartStop.nStarted==sst_ComspecStop);
	}

	// ***
	if (lbChanged)
		gpWorker->Processes().ProcessCountChanged(TRUE, nPrevCount, CS);
	CS.Unlock();
	// ***

	// После Unlock-а, зовем функцию
	if (AS.AltServerChanged)
	{
		server.OnAltServerChanged(2, in.StartStop.nStarted, in.StartStop.dwPID, &in.StartStop, AS);
	}

	// Обновить мэппинг
	WorkerServer::Instance().UpdateConsoleMapHeader(L"CECMD_CMDSTARTSTOP");

	CsAlt.Unlock();

	int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_STARTSTOPRET);
	*out = ExecuteNewCmd(CECMD_CMDSTARTSTOP,nOutSize);

	if (*out != NULL)
	{
		(*out)->StartStopRet.bWasBufferHeight = (gnBufferHeight != 0);
		(*out)->StartStopRet.hWnd = gState.conemuWnd_;
		(*out)->StartStopRet.hWndDc = gState.conemuWndDC_;
		(*out)->StartStopRet.hWndBack = gState.conemuWndBack_;
		(*out)->StartStopRet.dwPID = gState.conemuPid_;
		(*out)->StartStopRet.nBufferHeight = gnBufferHeight;
		(*out)->StartStopRet.nWidth = gpWorker->GetSbi().dwSize.X;
		(*out)->StartStopRet.nHeight = (gpWorker->GetSbi().srWindow.Bottom - gpWorker->GetSbi().srWindow.Top + 1);
		_ASSERTE(gState.runMode_==RunMode::Server);
		(*out)->StartStopRet.dwMainSrvPID = GetCurrentProcessId();
		(*out)->StartStopRet.dwAltSrvPID = server.GetAltServerPid();
		if (in.StartStop.nStarted == sst_AltServerStart)
		{
			(*out)->StartStopRet.dwPrevAltServerPID = nPrevAltServerPID;
		}

		lbRc = TRUE;
	}

	return lbRc;
}

BOOL cmd_SetFarPID(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;

	// Will also update mapping using MainServer if this is Alternative
	WorkerServer::Instance().SetFarPid(in.hdr.nSrcPID);

	return lbRc;
}

BOOL cmd_GuiChanged(CESERVER_REQ& in, CESERVER_REQ** out)
{
	// посылается в сервер, чтобы он обновил у себя ConEmuGuiMapping->CESERVER_CONSOLE_MAPPING_HDR
	BOOL lbRc = FALSE;

	if (gpSrv && gpSrv->pConsole)
	{
		WorkerServer::Instance().ReloadGuiSettings(&in.GuiInfo);
	}

	return lbRc;
}

// CECMD_TERMINATEPID
BOOL cmd_TerminatePid(CESERVER_REQ& in, CESERVER_REQ** out)
{
	// Прибить процесс (TerminateProcess)
	size_t cbInSize = in.DataSize();
	if ((cbInSize < 2*sizeof(DWORD)) || (cbInSize < (sizeof(DWORD)*(1+in.dwData[0]))))
		return FALSE;

	BOOL lbRc = FALSE;
	DWORD nErrCode = 0;
	const DWORD nCount = in.dwData[0];
	// ReSharper disable once CppLocalVariableMayBeConst
	LPDWORD pPid = reinterpret_cast<LPDWORD>(&(in.dwData[1]));

	if (nCount == 1)
		lbRc = TerminateOneProcess(pPid[0], nErrCode);
	else
		lbRc = TerminateProcessGroup(nCount, pPid, nErrCode);


	if (lbRc)
	{
		// To avoid "Press enter to close console" when process is killed
		gState.DisableAutoConfirmExit();
	}

	int nOutSize = sizeof(CESERVER_REQ_HDR) + 2*sizeof(DWORD);
	*out = ExecuteNewCmd(CECMD_TERMINATEPID,nOutSize);

	if (*out != NULL)
	{
		(*out)->dwData[0] = lbRc;
		(*out)->dwData[1] = nErrCode;
	}

	return lbRc;
}

// CECMD_AFFNTYPRIORITY
BOOL cmd_AffinityPriority(CESERVER_REQ& in, CESERVER_REQ** out)
{
	size_t cbInSize = in.DataSize();
	if (cbInSize < 2*sizeof(uint64_t))
		return FALSE;

	BOOL lbRc = FALSE;
	HANDLE hProcess = NULL;
	DWORD nErrCode = 0;

	DWORD_PTR nAffinity = (DWORD_PTR)in.qwData[0];
	DWORD nPriority = (DWORD)in.qwData[1];

	MSectionLock CS; CS.Lock(gpWorker->Processes().csProc);
	gpWorker->Processes().CheckProcessCount(TRUE);
	for (UINT i = 0; i < gpWorker->Processes().nProcessCount; i++)
	{
		DWORD nPID = gpWorker->Processes().pnProcesses[i];
		if (nPID == gnSelfPID) continue;

		HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, nPID);
		if (hProcess)
		{
			if (SetProcessAffinityMask(hProcess, nAffinity)
					&& SetPriorityClass(hProcess, nPriority))
				lbRc = TRUE;
			else
				nErrCode = GetLastError();

			CloseHandle(hProcess);
		}
		else
		{
			nErrCode = GetLastError();
		}
	}
	CS.Unlock();

	int nOutSize = sizeof(CESERVER_REQ_HDR) + 2*sizeof(DWORD);
	*out = ExecuteNewCmd(CECMD_AFFNTYPRIORITY,nOutSize);

	if (*out != NULL)
	{
		(*out)->dwData[0] = lbRc;
		(*out)->dwData[1] = nErrCode;
	}

	return lbRc;
}

// CECMD_PAUSE
BOOL cmd_Pause(CESERVER_REQ& in, CESERVER_REQ** out)
{
	size_t cbInSize = in.DataSize();
	if ((cbInSize < sizeof(DWORD)) || !gpSrv || !gpSrv->pConsole)
		return FALSE;

	BOOL bOk = FALSE;
	CONSOLE_SELECTION_INFO lsel1 = {}, lsel2 = {};
	BOOL bSelInfo1, bSelInfo2;

	bSelInfo1 = apiGetConsoleSelectionInfo(&lsel1);

	CEPauseCmd cmd = (CEPauseCmd)in.dwData[0], newState;
	if (cmd == CEPause_Switch)
		cmd = (bSelInfo1 && (lsel1.dwFlags & CONSOLE_SELECTION_IN_PROGRESS)) ? CEPause_Off : CEPause_On;

	switch (cmd)
	{
	case CEPause_On:
		SetConEmuFlags(gpSrv->pConsole->ConState.Flags, CECI_Paused, CECI_Paused);
		bOk = apiPauseConsoleOutput(gState.realConWnd_, true);
		break;
	case CEPause_Off:
		SetConEmuFlags(gpSrv->pConsole->ConState.Flags, CECI_Paused, CECI_None);
		bOk = apiPauseConsoleOutput(gState.realConWnd_, false);
		break;
	}

	// May be console server will not react immediately?
	DWORD nStartTick = GetTickCount();
	while (true)
	{
		bSelInfo2 = apiGetConsoleSelectionInfo(&lsel2);
		if (!bSelInfo2 || (lsel2.dwFlags != lsel1.dwFlags))
			break;
		Sleep(10);
		if ((GetTickCount() - nStartTick) >= 500)
			break;
	}
	// Return new state
	newState = (bSelInfo2 && (lsel2.dwFlags & CONSOLE_SELECTION_IN_PROGRESS)) ? CEPause_On : CEPause_Off;
	_ASSERTE(newState == cmd);

	int nOutSize = sizeof(CESERVER_REQ_HDR) + 2*sizeof(DWORD);
	*out = ExecuteNewCmd(CECMD_PAUSE,nOutSize);

	if (*out != NULL)
	{
		(*out)->dwData[0] = newState;
		(*out)->dwData[1] = bOk;
	}

	return TRUE;
}

// CECMD_FINDNEXTROWID
BOOL cmd_FindNextRowId(CESERVER_REQ& in, CESERVER_REQ** out)
{
	// IN:  dwData[0] - from row, dwData[1] - search upward (TRUE/FALSE)
	// OUT: dwData[0] - found row or DWORD(-1), dwData[1] - rowid
	size_t cbInSize = in.DataSize();
	if ((cbInSize < sizeof(DWORD)*2) || !gpSrv || !gpSrv->pConsole)
		return FALSE;

	SHORT newRow = -1, newRowId = -1, findRow = -1;
	CEConsoleMark rowMark, rowMark2;
	SHORT fromRow = LOSHORT(in.dwData[0]);
	bool  bUpWard = (in.dwData[1] != FALSE);

	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	GetConsoleScreenBufferInfo(ghConOut, &csbi);

	// Skip our first marked line
	if (ReadConsoleRowId(ghConOut, fromRow, &rowMark2))
	{
		if (bUpWard && (fromRow > 0))
		{
			fromRow -= 1;
			if (IsConsoleLineEmpty(ghConOut, fromRow, csbi.dwSize.X))
				fromRow -= 1;
		}
		else if (!bUpWard && ((fromRow + 1) < csbi.dwSize.Y))
		{
			fromRow += 2;
		}
	}

	if (!FindConsoleRowId(ghConOut, fromRow, bUpWard, &findRow, &rowMark))
	{
		newRow = bUpWard ? 0 : (csbi.dwSize.Y - 1);
	}
	else
	{
		newRow = findRow;
		newRowId = rowMark.RowId;
		// We mark two rows to ensure that tab-completion routine would not break our row mark
		if (bUpWard)
		{
			if (newRow > 0)
			{
				if (ReadConsoleRowId(ghConOut, newRow - 1, &rowMark2))
				{
					// Line above the prompt may be empty, no sense to scroll to it
					if (!IsConsoleLineEmpty(ghConOut, newRow - 1, csbi.dwSize.X))
					{
						newRow--;
						newRowId = rowMark2.RowId;
					}
				}
			}
		}
		else
		{
			if ((newRow + 1) < csbi.dwSize.Y)
			{
				if (IsConsoleLineEmpty(ghConOut, newRow, csbi.dwSize.X))
				{
					// We need the row below, but its rowid may be damaged (doesn't matter, just show it)
					newRow++;
					if (ReadConsoleRowId(ghConOut, newRow - 1, &rowMark2))
						newRowId = rowMark2.RowId;
				}
			}
		}
	}

	int nOutSize = sizeof(CESERVER_REQ_HDR) + 2*sizeof(DWORD);
	*out = ExecuteNewCmd(CECMD_FINDNEXTROWID,nOutSize);

	if (*out != NULL)
	{
		(*out)->dwData[0] = newRow;
		(*out)->dwData[1] = newRowId;
	}

	return TRUE;
}

BOOL cmd_GuiAppAttached(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;

	if (!gpSrv)
	{
		_ASSERTE(gpSrv!=NULL);
	}
	else
	{
		_ASSERTEX((in.AttachGuiApp.nPID == gpWorker->RootProcessId() || in.AttachGuiApp.nPID == gpSrv->Portable.nPID) && gpWorker->RootProcessId() && in.AttachGuiApp.nPID);

		wchar_t szInfo[MAX_PATH*2];

		// Вызывается два раза. Первый (при запуске exe) ahGuiWnd==NULL, второй - после фактического создания окна
		if (gState.attachMode_ || (gpWorker->RootProcessGui() == NULL))
		{
			swprintf_c(szInfo, L"GUI application (PID=%u) was attached to ConEmu:\n%s\n",
				in.AttachGuiApp.nPID, in.AttachGuiApp.sAppFilePathName);
			_wprintf(szInfo);
		}

		if (in.AttachGuiApp.hAppWindow && (gState.attachMode_ || (gpWorker->RootProcessGui() != in.AttachGuiApp.hAppWindow)))
		{
			wchar_t szTitle[MAX_PATH] = {};
			GetWindowText(in.AttachGuiApp.hAppWindow, szTitle, countof(szTitle));
			wchar_t szClass[MAX_PATH] = {};
			GetClassName(in.AttachGuiApp.hAppWindow, szClass, countof(szClass));
			swprintf_c(szInfo,
				L"\nWindow (x%08X,Style=x%08X,Ex=x%X,Flags=x%X) was attached to ConEmu:\nTitle: \"%s\"\nClass: \"%s\"\n",
				(DWORD)in.AttachGuiApp.hAppWindow, in.AttachGuiApp.Styles.nStyle, in.AttachGuiApp.Styles.nStyleEx, in.AttachGuiApp.nFlags, szTitle, szClass);
			_wprintf(szInfo);
		}

		if (in.AttachGuiApp.hAppWindow == NULL)
		{
			gpWorker->SetRootProcessGui(GUI_NOT_CREATED_YET);
		}
		else
		{
			gpWorker->SetRootProcessGui(in.AttachGuiApp.hAppWindow);
		}
		// Смысла в подтверждении нет - GUI приложение в консоль ничего не выводит
		gState.alwaysConfirmExit_ = FALSE;
		gpWorker->Processes().CheckProcessCount(TRUE);
		lbRc = TRUE;
	}

	int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD);
	*out = ExecuteNewCmd(CECMD_ATTACHGUIAPP,nOutSize);

	if (*out != NULL)
	{
		(*out)->dwData[0] = lbRc;
	}

	return TRUE;
}

// CECMD_REDRAWHWND
BOOL cmd_RedrawHWND(CESERVER_REQ& in, CESERVER_REQ** out)
{
	size_t cbInSize = in.DataSize();
	if (cbInSize < sizeof(DWORD))
		return FALSE;

	BOOL bRedraw = FALSE;
	HWND hWnd = (HWND)(DWORD_PTR)in.dwData[0];

	// We need to invalidate client and non-client areas, following lines does the trick
	RECT rcClient = {};
	if (GetWindowRect(hWnd, &rcClient))
	{
		MapWindowPoints(NULL, hWnd, (LPPOINT)&rcClient, 2);
		bRedraw = RedrawWindow(hWnd, &rcClient, NULL, RDW_ALLCHILDREN|RDW_INVALIDATE|RDW_FRAME);
	}

	int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD);
	*out = ExecuteNewCmd(CECMD_REDRAWHWND,nOutSize);

	if (*out != NULL)
	{
		(*out)->dwData[0] = bRedraw;
	}

	return TRUE;
}

#ifdef USE_COMMIT_EVENT
BOOL cmd_RegExtConsole(CESERVER_REQ& in, CESERVER_REQ** out)
{
	PRAGMA_ERROR("CECMD_REGEXTCONSOLE is deprecated!");

	BOOL lbRc = FALSE;

	if (!gpSrv)
	{
		_ASSERTE(gpSrv!=NULL);
	}
	else
	{
		if (in.RegExtCon.hCommitEvent != NULL)
		{
			if (gpSrv->hExtConsoleCommit && gpSrv->hExtConsoleCommit != (HANDLE)in.RegExtCon.hCommitEvent)
				CloseHandle(gpSrv->hExtConsoleCommit);
			gpSrv->hExtConsoleCommit = in.RegExtCon.hCommitEvent;
			gpSrv->nExtConsolePID = in.hdr.nSrcPID;
		}
		else
		{
			if (gpSrv->hExtConsoleCommit == (HANDLE)in.RegExtCon.hCommitEvent)
			{
				CloseHandle(gpSrv->hExtConsoleCommit);
				gpSrv->hExtConsoleCommit = NULL;
				gpSrv->nExtConsolePID = 0;
			}
		}

		lbRc = TRUE;
	}

	int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD);
	*out = ExecuteNewCmd(CECMD_REGEXTCONSOLE, nOutSize);

	if (*out != NULL)
	{
		(*out)->dwData[0] = lbRc;
	}

	return TRUE;
}
#endif

BOOL cmd_FreezeAltServer(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;
	DWORD nPrevAltServer = 0;

	if (!gpSrv)
	{
		_ASSERTE(gpSrv!=NULL);
	}
	else if (in.DataSize() < (2 * sizeof(in.dwData[0])))
	{
		_ASSERTE(FALSE && "DataSize should be 2*DWORD");
	}
	else
	{
		// dwData[0]=1-Freeze, 0-Thaw; dwData[1]=New Alt server PID
		wchar_t szLog[80] = L"";

		auto& server = WorkerServer::Instance();

		if (gpLogSize)
		{
			if (in.dwData[0] == 1)
				swprintf_c(szLog, L"AltServer: freeze requested, new AltServer=%u", in.dwData[1]);  // NOLINT(clang-diagnostic-array-bounds)
			else
				swprintf_c(szLog, L"AltServer: thaw requested, prev AltServer=%u", server.GetPrevAltServerPid());
			LogString(szLog);
		}

		if (in.dwData[0] == 1)
		{
			server.SetPrevAltServerPid(in.dwData[1]);  // NOLINT(clang-diagnostic-array-bounds)

			WorkerServer::Instance().FreezeRefreshThread();
		}
		else
		{
			nPrevAltServer = server.GetPrevAltServerPid();
			server.SetPrevAltServerPid(0);

			WorkerServer::Instance().ThawRefreshThread();

			if (gState.runMode_ == RunMode::AltServer)
			{
				// OK, GUI will be informed by RunMode::RM_SERVER itself
			}
			else
			{
				swprintf_c(szLog, L"AltServer: Wrong gpStatus->runMode_=%u", gState.runMode_);
				LogString(szLog);
				_ASSERTE(gState.runMode_ == RunMode::AltServer);
			}
		}

		lbRc = TRUE;
	}

	const size_t nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD) * 2;
	*out = ExecuteNewCmd(CECMD_FREEZEALTSRV, nOutSize);

	if (*out != nullptr)
	{
		(*out)->dwData[0] = lbRc;
		(*out)->dwData[1] = nPrevAltServer; // Informational  // NOLINT(clang-diagnostic-array-bounds)
	}

	return TRUE;
}

namespace {
// hOutput - our console output handle
// max_height - 0 for unlimited, >0 for detected dynamic height
BOOL LoadFullConsoleData(HANDLE hOutput, WORD max_height, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;

	CONSOLE_SCREEN_BUFFER_INFO lsbi = {{0,0}};
	// !!! Нас интересует реальное положение дел в консоли,
	//     а не скорректированное функцией MyGetConsoleScreenBufferInfo
	if (!GetConsoleScreenBufferInfo(hOutput, &lsbi))
	{
		return FALSE; // Не смогли получить информацию о консоли...
	}
	// Support dynamic height - do not load all 32K lines
	if (max_height && (static_cast<WORD>(lsbi.dwSize.Y) > max_height))
	{
		lsbi.dwSize.Y = max_height;
	}

	CESERVER_CONSAVE_MAP* pData = NULL;
	size_t cbReplySize = sizeof(CESERVER_CONSAVE_MAP) + (lsbi.dwSize.X * lsbi.dwSize.Y * sizeof(pData->Data[0]));
	*out = ExecuteNewCmd(CECMD_CONSOLEFULL, cbReplySize);

	if ((*out) != NULL)
	{
		pData = (CESERVER_CONSAVE_MAP*)*out;
		COORD BufSize = {lsbi.dwSize.X, lsbi.dwSize.Y};
		SMALL_RECT ReadRect = {0, 0, lsbi.dwSize.X-1, lsbi.dwSize.Y-1};

		// #PTY Use proper server implementation
		lbRc = WorkerServer::Instance().MyReadConsoleOutput(hOutput, pData->Data, BufSize, ReadRect);

		if (lbRc)
		{
			pData->Succeeded = lbRc;
			pData->MaxCellCount = lsbi.dwSize.X * lsbi.dwSize.Y;
			static DWORD nLastFullIndex;
			pData->CurrentIndex = ++nLastFullIndex;

			// Еще раз считать информацию по консоли (курсор положение и прочее...)
			// За время чтения данных - они могли прокрутиться вверх
			CONSOLE_SCREEN_BUFFER_INFO lsbi2 = {{0,0}};
			if (GetConsoleScreenBufferInfo(hOutput, &lsbi2))
			{
				// Обновим только курсор, а то юзер может получить черный экран, вместо ожидаемого текста
				// Если во время "dir c:\ /s" запросить AltConsole - получаем черный экран.
				// Смысл в том, что пока читали строки НИЖЕ курсора - экран уже убежал.
				lsbi.dwCursorPosition = lsbi2.dwCursorPosition;
			}

			pData->info = lsbi;
		}
	}

	return lbRc;
}

BOOL LoadConsoleMapData(CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;
	CONSOLE_SCREEN_BUFFER_INFO lsbi = {{0,0}};
	CESERVER_CONSAVE_MAPHDR* pMapHdr = NULL;
	CESERVER_CONSAVE_MAP* pMapData = NULL;
	if (!WorkerServer::Instance().CmdOutputOpenMap(lsbi, pMapHdr, pMapData))
		return FALSE;
	// #AltBuffer try to detect max used row
	const size_t max_cells = std::min<DWORD>(lsbi.dwSize.X * lsbi.dwSize.Y, pMapData->MaxCellCount);
	const size_t max_cells_cb = (max_cells * sizeof(pMapData->Data[0]));
	const size_t cbReplySize = sizeof(CESERVER_CONSAVE_MAP) + max_cells_cb;
	*out = ExecuteNewCmd(CECMD_CONSOLEFULL, cbReplySize);

	if ((*out) != NULL)
	{
		auto pData = (CESERVER_CONSAVE_MAP*)*out;
		pData->info = lsbi;
		pData->Succeeded = lbRc;
		pData->MaxCellCount = lsbi.dwSize.X * lsbi.dwSize.Y;
		pData->CurrentIndex = pMapData->CurrentIndex;
		memmove_s(pData->Data, max_cells_cb, pMapData->Data, max_cells_cb);
		lbRc = TRUE;
	}
	return lbRc;
}

BOOL LoadFullConsoleDataReal(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;

	// В Win7 закрытие дескриптора в ДРУГОМ процессе - закрывает консольный буфер ПОЛНОСТЬЮ!!!
	// В итоге, буфер вывода telnet'а схлопывается!
	if (WorkerServer::Instance().IsReopenHandleAllowed())
	{
		WorkerServer::Instance().ConOutCloseHandle();
	}

	return LoadFullConsoleData(ghConOut, (in.DataSize() >= sizeof(DWORD)) ? LOWORD(in.dwData[0]) : 0, out);
}
}  // namespace

BOOL cmd_LoadFullConsoleData(CESERVER_REQ& in, CESERVER_REQ** out)
{
	// Need to block all requests to output buffer in other threads
	MSectionLockSimple csRead = gpWorker->LockConsoleReaders(LOCK_READOUTPUT_TIMEOUT);

	BOOL alt_screen = (in.DataSize() >= 2 * sizeof(DWORD)) ? in.dwData[1] : FALSE;
	if (!alt_screen)
		return LoadFullConsoleDataReal(in, out);
	else if (gPrimaryBuffer.HasHandle())
		return LoadFullConsoleData(gPrimaryBuffer, gnPrimaryBufferLastRow, out);
	else
		return LoadConsoleMapData(out);
}

BOOL cmd_SetFullScreen(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;
	//ghConOut

	const size_t cbReplySize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_FULLSCREEN);
	*out = ExecuteNewCmd(CECMD_SETFULLSCREEN, cbReplySize);

	// Need to block all requests to output buffer in other threads
	MSectionLockSimple csRead = gpWorker->LockConsoleReaders(LOCK_READOUTPUT_TIMEOUT);

	if ((*out) != nullptr)
	{
		CLastErrorGuard lastErrorGuard;
		if (!gpWorker->EnterHwFullScreen(&(*out)->FullScreenRet.crNewSize))
		{
			(*out)->FullScreenRet.bSucceeded = false;
			(*out)->FullScreenRet.nErrCode = GetLastError();
		}
		else
		{
			(*out)->FullScreenRet.bSucceeded = true;
		}
		lbRc = TRUE;
	}
	else
	{
		lbRc = FALSE;
	}

	return lbRc;
}

// CECMD_SETCONCOLORS
BOOL cmd_SetConColors(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;
	BOOL bOk = FALSE;
	//ghConOut

	LogString(L"CECMD_SETCONCOLORS: Received");

	// Need to block all requests to output buffer in other threads
	MSectionLockSimple csRead = gpWorker->LockConsoleReaders(LOCK_READOUTPUT_TIMEOUT);

	size_t cbReplySize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_SETCONSOLORS);
	*out = ExecuteNewCmd(CECMD_SETCONCOLORS, cbReplySize);

	if ((*out) != NULL)
	{
		BOOL bTextChanged = FALSE, bPopupChanged = FALSE, bNeedRepaint = TRUE;
		WORD OldText = 0x07, OldPopup = 0x3E;

		CONSOLE_SCREEN_BUFFER_INFO csbi5 = {};
		BOOL bCsbi5 = GetConsoleScreenBufferInfo(ghConOut, &csbi5);

		if (bCsbi5)
		{
			(*out)->SetConColor.ChangeTextAttr = TRUE;
			(*out)->SetConColor.NewTextAttributes = csbi5.wAttributes;
		}

		if (IsWin6())
		{
			LogString(L"CECMD_SETCONCOLORS: acquiring CONSOLE_SCREEN_BUFFER_INFOEX");
			MY_CONSOLE_SCREEN_BUFFER_INFOEX csbi6 = {sizeof(csbi6)};
			if (apiGetConsoleScreenBufferInfoEx(ghConOut, &csbi6))
			{
				OldText = csbi6.wAttributes;
				OldPopup = csbi6.wPopupAttributes;
				bTextChanged = (csbi6.wAttributes != in.SetConColor.NewTextAttributes);
				bPopupChanged = in.SetConColor.ChangePopupAttr && (csbi6.wPopupAttributes != in.SetConColor.NewPopupAttributes);

				(*out)->SetConColor.ChangePopupAttr = TRUE;
				(*out)->SetConColor.NewPopupAttributes = OldPopup;

				if (!bTextChanged && !bPopupChanged)
				{
					bOk = TRUE;
				}
				else
				{
					csbi6.wAttributes = in.SetConColor.NewTextAttributes;
					csbi6.wPopupAttributes = in.SetConColor.NewPopupAttributes;
					if (bPopupChanged)
					{
						BOOL bIsVisible = IsWindowVisible(gState.realConWnd_);
						LogString(L"CECMD_SETCONCOLORS: applying CONSOLE_SCREEN_BUFFER_INFOEX");
						bOk = apiSetConsoleScreenBufferInfoEx(ghConOut, &csbi6);
						bNeedRepaint = FALSE;
						if (!bIsVisible && IsWindowVisible(gState.realConWnd_))
						{
							LogString(L"CECMD_SETCONCOLORS: RealConsole was shown unexpectedly");
							apiShowWindow(gState.realConWnd_, SW_HIDE);
						}
						if (bOk)
						{
							(*out)->SetConColor.NewTextAttributes = csbi6.wAttributes;
							(*out)->SetConColor.NewPopupAttributes = csbi6.wPopupAttributes;
						}
					}
					else
					{
						LogString(L"CECMD_SETCONCOLORS: applying ConsoleTextAttributes");
						bOk = SetConsoleTextAttribute(ghConOut, in.SetConColor.NewTextAttributes);
						if (bOk)
						{
							(*out)->SetConColor.NewTextAttributes = csbi6.wAttributes;
						}
					}
				}
			}
		}
		else
		{
			if (bCsbi5)
			{
				OldText = csbi5.wAttributes;
				bTextChanged = (csbi5.wAttributes != in.SetConColor.NewTextAttributes);

				if (!bTextChanged)
				{
					bOk = TRUE;
				}
				else
				{
					LogString(L"CECMD_SETCONCOLORS: applying ConsoleTextAttributes");
					bOk = SetConsoleTextAttribute(ghConOut, in.SetConColor.NewTextAttributes);
					if (bOk)
					{
						(*out)->SetConColor.NewTextAttributes = in.SetConColor.NewTextAttributes;
					}
				}
			}
		}

		/*
					// If some cells was marked as "RowIds" - we need to change them manually
					SHORT nFromRow = csbi5.dwCursorPosition.Y, nRow = 0;
					CEConsoleMark Mark;
					while ((nFromRow >= 0) && FindConsoleRowId(ghConOut, nFromRow, &nRow, &Mark))
					{
						nFromRow = nRow - 1;
						// Check attributes
					}
		*/

		if (bOk && bCsbi5 && bTextChanged && in.SetConColor.ReFillConsole && csbi5.dwSize.X && csbi5.dwSize.Y)
		{
			// Считать из консоли текущие атрибуты (построчно/поблочно)
			// И там, где они совпадают с OldText - заменить на in.SetConColor.NewTextAttributes
			// #Refill Pass current DynamicHeight into
			gpWorker->RefillConsoleAttributes(csbi5, OldText, in.SetConColor.NewTextAttributes);
		}

		(*out)->SetConColor.ReFillConsole = bOk;
	}
	else
	{
		lbRc = FALSE;
	}

	LogString(bOk ? L"CECMD_SETCONCOLORS: Succeeded" : L"CECMD_SETCONCOLORS: Failed");

	return lbRc;
}

BOOL cmd_SetConTitle(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;

	size_t cbReplySize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD);
	*out = ExecuteNewCmd(CECMD_SETCONTITLE, cbReplySize);

	if ((*out) != NULL)
	{
		(*out)->dwData[0] = SetTitle((LPCWSTR)in.wData);
	}
	else
	{
		lbRc = FALSE;
	}

	return lbRc;
}

BOOL cmd_AltBuffer(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = TRUE;
	CONSOLE_SCREEN_BUFFER_INFO lsbi = {};

	// Need to block all requests to output buffer in other threads
	MSectionLockSimple csRead = gpWorker->LockConsoleReaders(LOCK_READOUTPUT_TIMEOUT);
	if (!csRead.IsLocked())
	{
		LogString("cmd_AltBuffer: csReadConsoleInfo.Lock failed");
		lbRc = FALSE;
	}
	// !!! Нас интересует реальное положение дел в консоли,
	//     а не скорректированное функцией MyGetConsoleScreenBufferInfo
	else if (!GetConsoleScreenBufferInfo(ghConOut, &lsbi))
	{
		LogString("cmd_AltBuffer: GetConsoleScreenBufferInfo failed");
		lbRc = FALSE;
	}
	else
	{
		if (gpLogSize)
		{
			char szInfo[100];
			sprintf_c(szInfo, "cmd_AltBuffer: (%u) %s%s%s%s",
				in.AltBuf.BufferHeight,
				!in.AltBuf.AbFlags ? "AbFlags==0" :
				(in.AltBuf.AbFlags & abf_SaveContents) ? " abf_SaveContents" : "",
				(in.AltBuf.AbFlags & abf_RestoreContents) ? " abf_RestoreContents" : "",
				(in.AltBuf.AbFlags & abf_BufferOn) ? " abf_BufferOn" : "",
				(in.AltBuf.AbFlags & abf_BufferOff) ? " abf_BufferOff" : "");
			LogString(szInfo);
		}

		//do resize of the active screeen buffer (as usual)
		//cmd_SetSizeXXX_CmdStartedFinished(CESERVER_REQ& in, CESERVER_REQ** out)
		//CECMD_SETSIZESYNC
		auto do_resize_buffer = [&in, &lsbi, &csRead]()
		{
			bool lbRc = true;
			CESERVER_REQ* pSizeOut = NULL;
			CESERVER_REQ* pSizeIn = ExecuteNewCmd(CECMD_SETSIZESYNC, sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_SETSIZE));
			if (!pSizeIn)
			{
				lbRc = false;
			}
			else
			{
				pSizeIn->hdr = in.hdr; // Как-бы фиктивно пришло из приложения
				pSizeIn->hdr.nCmd = CECMD_SETSIZESYNC;
				// we have to restore proper size, reverted two lines above
				pSizeIn->hdr.cbSize = (sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_SETSIZE));

				//pSizeIn->SetSize.rcWindow.Left = pSizeIn->SetSize.rcWindow.Top = 0;
				//pSizeIn->SetSize.rcWindow.Right = lsbi.srWindow.Right - lsbi.srWindow.Left;
				//pSizeIn->SetSize.rcWindow.Bottom = lsbi.srWindow.Bottom - lsbi.srWindow.Top;
				pSizeIn->SetSize.size.X = lsbi.srWindow.Right - lsbi.srWindow.Left + 1;
				pSizeIn->SetSize.size.Y = lsbi.srWindow.Bottom - lsbi.srWindow.Top + 1;


				if (in.AltBuf.AbFlags & abf_BufferOff)
				{
					pSizeIn->SetSize.nBufferHeight = 0;
				}
				else
				{
					TODO("BufferWidth");
					pSizeIn->SetSize.nBufferHeight = (in.AltBuf.BufferHeight > 0) ? in.AltBuf.BufferHeight : 1000;
				}

				csRead.Unlock();

				if (!cmd_SetSizeXXX_CmdStartedFinished(*pSizeIn, &pSizeOut))
				{
					lbRc = false;
				}
				else
				{
					gpSrv->bAltBufferEnabled = ((in.AltBuf.AbFlags & (abf_BufferOff|abf_SaveContents)) == (abf_BufferOff|abf_SaveContents));
					WARNING("Caller must clear active screen contents");
				}

				ExecuteFreeResult(pSizeIn);
				ExecuteFreeResult(pSizeOut);
			}
			return lbRc;
		};

		// In Windows 7 we have to use legacy mode
		if (!WorkerServer::Instance().IsReopenHandleAllowed()
			|| !(in.AltBuf.AbFlags & abf_Connector))
		{
			if (in.AltBuf.AbFlags & abf_SaveContents)
				WorkerServer::Instance().CmdOutputStore();
			if (in.AltBuf.AbFlags & (abf_BufferOn|abf_BufferOff))
				lbRc = do_resize_buffer();
			if (in.AltBuf.AbFlags & abf_RestoreContents)
				WorkerServer::Instance().CmdOutputRestore(true/*Simple*/);
		}
		// Switch console buffer handles
		else if (in.AltBuf.AbFlags & (abf_SaveContents|abf_RestoreContents))
		{
			static bool alt_buffer_set = false;
			_ASSERTE(!!(in.AltBuf.AbFlags & abf_BufferOff) == !!(in.AltBuf.AbFlags & abf_SaveContents));
			_ASSERTE(!!(in.AltBuf.AbFlags & abf_BufferOn) == !!(in.AltBuf.AbFlags & abf_RestoreContents));
			_ASSERTE(!!(in.AltBuf.AbFlags & abf_BufferOff) != !!(in.AltBuf.AbFlags & abf_BufferOn));

			if ((in.AltBuf.AbFlags & abf_BufferOff) && !alt_buffer_set)
			{
				SECURITY_ATTRIBUTES sec = {sizeof(sec), nullptr, TRUE};
				if (gPrimaryBuffer.SetHandle(::GetStdHandle(STD_OUTPUT_HANDLE), MConHandle::StdMode::Output)
					&& gAltBuffer.SetHandle(::CreateConsoleScreenBuffer(
						GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, &sec, CONSOLE_TEXTMODE_BUFFER, nullptr)
						, MConHandle::StdMode::None)
					&& SetConsoleActiveScreenBuffer(gAltBuffer)
					)
				{
					ghConOut.SetHandlePtr(gAltBuffer);
					if ((lbRc = do_resize_buffer()))
					{
						gnPrimaryBufferLastRow = in.AltBuf.BufferHeight;
						alt_buffer_set = true;
					}
					else
					{
						SetConsoleActiveScreenBuffer(gPrimaryBuffer);
						ghConOut.SetHandlePtr(nullptr);
						gPrimaryBuffer.Close();
						gAltBuffer.Close();
					}
				}
				else
				{
					gPrimaryBuffer.Close();
					gAltBuffer.Close();
				}
			}
			else if ((in.AltBuf.AbFlags & abf_BufferOn) && alt_buffer_set)
			{
				SetConsoleActiveScreenBuffer(gPrimaryBuffer);
				ghConOut.SetHandlePtr(nullptr);
				gPrimaryBuffer.Close();
				gAltBuffer.Close();
				alt_buffer_set = false;
				gnPrimaryBufferLastRow = 0;
				lbRc = do_resize_buffer();
			}
		}
		// Only buffer height change was requested
		else if (in.AltBuf.AbFlags & (abf_BufferOn|abf_BufferOff))
		{
			lbRc = do_resize_buffer();
		}
	}

	size_t cbReplySize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_ALTBUFFER);
	*out = ExecuteNewCmd(CECMD_ALTBUFFER, cbReplySize);
	if ((*out) != NULL)
	{
		(*out)->AltBuf.AbFlags = in.AltBuf.AbFlags;
		TODO("BufferHeight");
		(*out)->AltBuf.BufferHeight = lsbi.dwSize.Y;
	}
	else
	{
		lbRc = FALSE;
	}

	return lbRc;
}

BOOL cmd_AltBufferState(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = TRUE;

	size_t cbReplySize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD);
	*out = ExecuteNewCmd(CECMD_ALTBUFFERSTATE, cbReplySize);
	if ((*out) != NULL)
	{
		(*out)->dwData[0] = gpSrv->bAltBufferEnabled;
	}
	else
	{
		lbRc = FALSE;
	}

	return lbRc;
}

BOOL cmd_ApplyExportVars(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = TRUE;

	ApplyExportEnvVar((LPCWSTR)in.wData);

	size_t cbReplySize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD);
	*out = ExecuteNewCmd(CECMD_EXPORTVARS, cbReplySize);
	if ((*out) != NULL)
	{
		(*out)->dwData[0] = TRUE;
	}
	else
	{
		lbRc = FALSE;
	}

	return lbRc;
}

BOOL cmd_UpdConMapHdr(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = TRUE, lbAccepted = FALSE;

	size_t cbInSize = in.DataSize(), cbReqSize = sizeof(CESERVER_CONSOLE_MAPPING_HDR);
	if (cbInSize == cbReqSize)
	{
		if (in.ConInfo.cbSize == sizeof(CESERVER_CONSOLE_MAPPING_HDR)
			&& in.ConInfo.nProtocolVersion == CESERVER_REQ_VER
			&& in.ConInfo.nServerPID == GetCurrentProcessId())
		{
			//SetVisibleSize(in.ConInfo.crLockedVisible.X, in.ConInfo.crLockedVisible.Y); // ??

			if (gpSrv->pConsoleMap)
			{
				WorkerServer::Instance().FixConsoleMappingHdr(&in.ConInfo);
				gpSrv->pConsoleMap->SetFrom(&in.ConInfo);
			}

			lbAccepted = TRUE;
		}
	}

	size_t cbReplySize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD);
	*out = ExecuteNewCmd(CECMD_UPDCONMAPHDR, cbReplySize);
	if ((*out) != NULL)
	{
		(*out)->dwData[0] = lbAccepted;
	}
	else
	{
		lbRc = FALSE;
	}

	return lbRc;
}

BOOL cmd_SetConScrBuf(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = TRUE;
	DWORD nSafeWait = (DWORD)-1;
	HANDLE hInEvent = NULL, hOutEvent = NULL, hWaitEvent = NULL;
	char szLog[200];

	size_t cbInSize = in.DataSize(), cbReqSize = sizeof(CESERVER_REQ_SETCONSCRBUF);
	if (cbInSize == cbReqSize)
	{
		char* pszLogLbl;
		if (gpLogSize)
		{
			msprintf(szLog, countof(szLog), "CECMD_SETCONSCRBUF x%08X {%i,%i} ",
				(DWORD)(DWORD_PTR)in.SetConScrBuf.hRequestor, (int)in.SetConScrBuf.dwSize.X, (int)in.SetConScrBuf.dwSize.Y);
			pszLogLbl = szLog + lstrlenA(szLog);
		}

		if (in.SetConScrBuf.bLock)
		{
			if (gpLogSize)
			{
				lstrcpynA(pszLogLbl, "Recvd", 10);
				LogString(szLog);
			}
			// Блокируем нить чтения и дождемся пока она перейдет в режим ожидания
			#ifdef _DEBUG
			if (gpSrv->hInWaitForSetConBufThread)
			{
				_ASSERTE(gpSrv->hInWaitForSetConBufThread==NULL);
			}
			#endif
			gpSrv->hInWaitForSetConBufThread = hInEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			gpSrv->hOutWaitForSetConBufThread = hOutEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			gpSrv->hWaitForSetConBufThread = hWaitEvent = (in.SetConScrBuf.hRequestor ? in.SetConScrBuf.hRequestor : INVALID_HANDLE_VALUE);
			nSafeWait = WaitForSingleObject(hInEvent, WAIT_SETCONSCRBUF_MIN_TIMEOUT);
			if (gpLogSize)
			{
				lstrcpynA(pszLogLbl, "Ready", 10);
				LogString(szLog);
			}
		}
		else
		{
			if (gpLogSize)
			{
				lstrcpynA(pszLogLbl, "Finish", 10);
				LogString(szLog);
			}
			hOutEvent = gpSrv->hOutWaitForSetConBufThread;
			// Otherwise - we must be in the call of ANOTHER thread!
			if (hOutEvent == in.SetConScrBuf.hTemp)
			{
				SetEvent(hOutEvent);
			}
			else
			{
				// Timeout may be, or another process we are waiting for already...
				_ASSERTE(hOutEvent == in.SetConScrBuf.hTemp);
			}
		}
	}

	size_t cbReplySize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_SETCONSCRBUF);
	*out = ExecuteNewCmd(CECMD_SETCONSCRBUF, cbReplySize);
	if ((*out) != NULL)
	{
		if (in.SetConScrBuf.bLock)
		{
			(*out)->SetConScrBuf.bLock = nSafeWait;
			(*out)->SetConScrBuf.hTemp = hOutEvent;
		}
	}
	else
	{
		lbRc = FALSE;
	}

	return lbRc;
}

BOOL cmd_PortableStarted(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = TRUE;

	if (in.DataSize() == sizeof(gpSrv->Portable))
	{
		gpSrv->Portable = in.PortableStarted;
		gpWorker->Processes().CheckProcessCount(TRUE);
		// For example, CommandPromptPortable.exe starts cmd.exe
		CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_PORTABLESTART, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_PORTABLESTARTED));
		if (pIn)
		{
			pIn->PortableStarted = in.PortableStarted;
			pIn->PortableStarted.hProcess = NULL; // hProcess is valid for our server only
			CESERVER_REQ* pOut = ExecuteGuiCmd(gState.realConWnd_, pIn, gState.realConWnd_);
			ExecuteFreeResult(pOut);
			ExecuteFreeResult(pIn);
		}
	}
	else
	{
		_ASSERTE(FALSE && "Invalid PortableStarted data");
	}

	size_t cbReplySize = sizeof(CESERVER_REQ_HDR);
	*out = ExecuteNewCmd(CECMD_PORTABLESTART, cbReplySize);
	lbRc = ((*out) != NULL);

	return lbRc;
}

BOOL cmd_CtrlBreakEvent(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = TRUE;
	BOOL lbGenRc = FALSE;
	wchar_t szLogData[80];

	if (in.DataSize() == (2 * sizeof(DWORD)))
	{
		lbGenRc = GenerateConsoleCtrlEvent(in.dwData[0], in.dwData[1]);
		msprintf(szLogData, countof(szLogData), L"GenerateConsoleCtrlEvent(%u,%u)=%u", in.dwData[0], in.dwData[1], lbGenRc);
	}
	else
	{
		msprintf(szLogData, countof(szLogData), L"Invalid CtrlBreakEvent data size=%u", in.DataSize());
		_ASSERTE(FALSE && "Invalid CtrlBreakEvent data size");
	}

	if (gpLogSize) gpLogSize->LogString(szLogData);

	size_t cbReplySize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD);
	*out = ExecuteNewCmd(CECMD_CTRLBREAK, cbReplySize);
	if (*out)
		(*out)->dwData[0] = lbGenRc;
	lbRc = ((*out) != NULL);

	return lbRc;
}

BOOL cmd_SetTopLeft(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = TRUE;
	BOOL lbGenRc = FALSE;

	if (in.DataSize() >= sizeof(in.ReqConInfo))
	{
		gpSrv->TopLeft = in.ReqConInfo.TopLeft;

		// May be we can scroll the console down?
		if (!in.ReqConInfo.VirtualOnly && in.ReqConInfo.TopLeft.isLocked())
		{
			// We need real (uncorrected) position
			CONSOLE_SCREEN_BUFFER_INFO csbi = {};
			if (GetConsoleScreenBufferInfo(ghConOut, &csbi))
			{
				bool bChange = false;
				SMALL_RECT srNew = csbi.srWindow;
				const int height = srNew.Bottom - srNew.Top + 1;

				// In some cases we can do physical scrolling
				if ((in.ReqConInfo.TopLeft.y >= 0)
					// cursor was visible?
					&& ((csbi.dwCursorPosition.Y >= srNew.Top)
						&& (csbi.dwCursorPosition.Y <= srNew.Bottom))
					// and cursor is remains visible?
					&& ((csbi.dwCursorPosition.Y >= in.ReqConInfo.TopLeft.y)
						&& (csbi.dwCursorPosition.Y < (in.ReqConInfo.TopLeft.y + height)))
					)
				{
					const int shiftY = in.ReqConInfo.TopLeft.y - srNew.Top;
					srNew.Top = in.ReqConInfo.TopLeft.y;
					srNew.Bottom += shiftY;
					bChange = true;
				}

				if (bChange)
				{
					SetConsoleWindowInfo(ghConOut, TRUE, &srNew);
				}
			}
		}
	}
	else
	{
		_ASSERTE(FALSE && "Invalid SetTopLeft data");
	}

	const size_t cbReplySize = sizeof(CESERVER_REQ_HDR);
	*out = ExecuteNewCmd(CECMD_SETTOPLEFT, cbReplySize);
	lbRc = ((*out) != nullptr);

	return lbRc;
}

BOOL cmd_PromptStarted(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = TRUE;
	wchar_t szStarted[MAX_PATH+80];

	if (in.DataSize() >= sizeof(wchar_t))
	{
		int iLen = lstrlen(in.PromptStarted.szExeName);
		if (iLen > MAX_PATH) in.PromptStarted.szExeName[MAX_PATH] = 0;
		swprintf_c(szStarted, L"Prompt (Hook server) was started, PID=%u {%s}", in.hdr.nSrcPID, in.PromptStarted.szExeName);
		LogFunction(szStarted);
	}

	*out = ExecuteNewCmd(CECMD_PROMPTSTARTED, sizeof(CESERVER_REQ_HDR));
	lbRc = ((*out) != NULL);

	return lbRc;
}

BOOL cmd_LockStation(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = TRUE;

	gState.bStationLocked_ = TRUE;

	LogSize(NULL, 0, ":CECMD_LOCKSTATION");

	*out = ExecuteNewCmd(CECMD_LOCKSTATION, sizeof(CESERVER_REQ_HDR));
	lbRc = ((*out) != NULL);

	return lbRc;
}

BOOL cmd_UnlockStation(CESERVER_REQ& in, CESERVER_REQ** out, bool bProcessed)
{
	BOOL lbRc = TRUE;

	gState.bStationLocked_ = FALSE;

	LogString("CECMD_UNLOCKSTATION");

	if (!bProcessed)
	{
		lbRc = cmd_SetSizeXXX_CmdStartedFinished(in, out);
	}
	else
	{
		*out = ExecuteNewCmd(CECMD_UNLOCKSTATION, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_RETSIZE));

		if (*out)
		{
			(*out)->SetSizeRet.crMaxSize = MyGetLargestConsoleWindowSize(ghConOut);
			MyGetConsoleScreenBufferInfo(ghConOut, &((*out)->SetSizeRet.SetSizeRet));
			DWORD lnNextPacketId = (DWORD)InterlockedIncrement(&gpSrv->nLastPacketID);
			(*out)->SetSizeRet.nNextPacketId = lnNextPacketId;
		}

		LogSize(NULL, 0, ":UnlockStation");

		lbRc = ((*out) != NULL);
	}

	return lbRc;
}

BOOL cmd_GetRootInfo(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = TRUE;

	LogString("CECMD_GETROOTINFO");

	*out = ExecuteNewCmd(CECMD_GETROOTINFO, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_ROOT_INFO));

	if (*out)
	{
		gpWorker->Processes().GetRootInfo(*out);
	}

	lbRc = ((*out) != NULL);

	return lbRc;
}

BOOL cmd_WriteText(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = TRUE;

	LogString("CECMD_WRITETEXT");

	*out = ExecuteNewCmd(CECMD_WRITETEXT, sizeof(CESERVER_REQ_HDR)+sizeof(DWORD));

	if (*out)
	{
		DWORD nWritten = 0;

		DWORD nBytes = in.DataSize();
		_ASSERTE(nBytes && !(nBytes % sizeof(wchar_t)));
		LPCWSTR pszText = (LPCWSTR)in.wData;
		int nLen = nBytes>>1;

		// Z-Terminated string is expected here, but we check
		if (nLen > 0)
		{
			if (pszText[nLen-1] == 0)
				nLen--;
			else
				_ASSERTE(pszText[nLen-1] == 0);
		}

		if (nLen > 0)
		{
			WriteOutput(pszText, nLen, &nWritten, true/*bProcessed*/, false/*bAsciiPrint*/, false/*bStreamBy1*/, false/*bToBottom*/);
		}

		(*out)->dwData[0] = nWritten;
	}

	lbRc = ((*out) != NULL);

	return lbRc;
}


/* ********************************** */

/// Helper routine to delegate CMD execution to active AltServer
bool ProcessAltSrvCommand(CESERVER_REQ& in, CESERVER_REQ** out, BOOL& lbRc)
{
	bool lbProcessed = false;
	auto& server = WorkerServer::Instance();
	// Если крутится альтернативный сервер - команду нужно выполнять в нем
	if (server.GetAltServerPid() && (server.GetAltServerPid() != gnSelfPID))
	{
		HANDLE hSave = nullptr, hDup = nullptr; DWORD nDupError = static_cast<DWORD>(-1);
		if ((in.hdr.nCmd == CECMD_SETCONSCRBUF) && (in.DataSize() >= sizeof(in.SetConScrBuf)) && in.SetConScrBuf.bLock)
		{
			hSave = in.SetConScrBuf.hRequestor;
			nDupError = server.DuplicateHandleForAltServer(in.SetConScrBuf.hRequestor, hDup);
			if (nDupError == 0)
				in.SetConScrBuf.hRequestor = hDup;
		}

		(*out) = ExecuteSrvCmd(server.GetAltServerPid(), &in, gState.realConWnd_);
		lbProcessed = ((*out) != nullptr);
		lbRc = lbProcessed;

		if (hSave)
		{
			in.SetConScrBuf.hRequestor = hSave;
			if (lbProcessed)
				SafeCloseHandle(hSave);
		}
		std::ignore = nDupError;
	}
	return lbProcessed;
}


BOOL cmd_StartXTerm(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = TRUE;

	LogString("CECMD_STARTXTERM");

	// We must add the PID immediately, otherwise the flag
	// may be reseted unexpectedly due to "process termination"
	if (in.DataSize() >= 3*sizeof(DWORD))
	{
		gpWorker->Processes().StartStopXTermMode((TermModeCommand)in.dwData[0], in.dwData[1], in.dwData[2]);

		if (in.DataSize() >= 4*sizeof(DWORD))
		{
			gpSrv->pConsole->hdr.stdConBlockingPID = in.dwData[3];
			WorkerServer::Instance().UpdateConsoleMapHeader(L"CECMD_STARTXTERM");
		}
	}

	// Inform the GUI
	CESERVER_REQ* pGuiOut = ExecuteGuiCmd(gState.realConWnd_, &in, gState.realConWnd_);
	ExecuteFreeResult(pGuiOut);

	*out = ExecuteNewCmd(CECMD_STARTXTERM, sizeof(CESERVER_REQ_HDR));
	lbRc = ((*out) != NULL);
	return lbRc;
}

BOOL cmd_StartPtySrv(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = TRUE;

	LogString("CECMD_STARTPTYSRV");

	if (in.DataSize() >= sizeof(CESERVER_REQ::Data[0]) && in.Data[0] == TRUE)
	{
		auto handles = SrvAnsi::Object()->GetClientHandles(in.hdr.nSrcPID);
		if ((*out = ExecuteNewCmd(CECMD_STARTPTYSRV, sizeof(CESERVER_REQ_HDR) + 2*sizeof(CESERVER_REQ::qwData[0]))))
		{
			(*out)->qwData[0] = uint64_t(handles.first);
			(*out)->qwData[1] = uint64_t(handles.second);
		}
	}
	else
	{
		*out = ExecuteNewCmd(CECMD_STARTPTYSRV, sizeof(CESERVER_REQ_HDR));
	}

	lbRc = ((*out) != NULL);
	return lbRc;
}


/// Main routine to process CECMD
BOOL ProcessSrvCommand(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc;
	MCHKHEAP;

	switch (in.hdr.nCmd)
	{
		case CECMD_SETSIZESYNC:
		case CECMD_SETSIZENOSYNC:
		case CECMD_CMDSTARTED:
		case CECMD_CMDFINISHED:
		{
			lbRc = cmd_SetSizeXXX_CmdStartedFinished(in, out);
		} break;
		case CECMD_STARTXTERM:
		{
			lbRc = cmd_StartXTerm(in, out);
		} break;
		case CECMD_ATTACH2GUI:
		{
			lbRc = cmd_Attach2Gui(in, out);
		}  break;
		case CECMD_FARLOADED:
		{
			lbRc = cmd_FarLoaded(in, out);
		} break;
		case CECMD_POSTCONMSG:
		{
			lbRc = cmd_PostConMsg(in, out);
		} break;
		case CECMD_SAVEALIASES:
		{
			lbRc = cmd_SaveAliases(in, out);
		} break;
		case CECMD_GETALIASES:
		{
			lbRc = cmd_GetAliases(in, out);
		} break;
		case CECMD_FARDETACHED:
		{
			// После детача в фаре команда (например dir) схлопнется, чтобы
			// консоль неожиданно не закрылась...
			lbRc = cmd_FarDetached(in, out);
		} break;
		case CECMD_ONACTIVATION:
		{
			lbRc = cmd_OnActivation(in, out);
		} break;
		case CECMD_SETWINDOWPOS:
		{
			//SetWindowPos(in.SetWndPos.hWnd, in.SetWndPos.hWndInsertAfter,
			//             in.SetWndPos.X, in.SetWndPos.Y, in.SetWndPos.cx, in.SetWndPos.cy,
			//             in.SetWndPos.uFlags);
			lbRc = cmd_SetWindowPos(in, out);
		} break;
		case CECMD_SETFOCUS:
		{
			lbRc = cmd_SetFocus(in, out);
		} break;
		case CECMD_SETPARENT:
		{
			lbRc = cmd_SetParent(in, out);
		} break;
		case CECMD_SETWINDOWRGN:
		{
			//MySetWindowRgn(&in.SetWndRgn);
			lbRc = cmd_SetWindowRgn(in, out);
		} break;
		case CECMD_DETACHCON:
		{
			lbRc = cmd_DetachCon(in, out);
		} break;
		case CECMD_CMDSTARTSTOP:
		{
			lbRc = cmd_CmdStartStop(in, out);
		} break;
		case CECMD_SETFARPID:
		{
			lbRc = cmd_SetFarPID(in, out);
		} break;
		case CECMD_GUICHANGED:
		{
			lbRc = cmd_GuiChanged(in, out);
		} break;
		case CECMD_TERMINATEPID:
		{
			lbRc = cmd_TerminatePid(in, out);
		} break;
		case CECMD_AFFNTYPRIORITY:
		{
			lbRc = cmd_AffinityPriority(in, out);
		} break;
		case CECMD_PAUSE:
		{
			lbRc = cmd_Pause(in, out);
		} break;
		case CECMD_ATTACHGUIAPP:
		{
			lbRc = cmd_GuiAppAttached(in, out);
		} break;
		case CECMD_REDRAWHWND:
		{
			lbRc = cmd_RedrawHWND(in, out);
		} break;
		case CECMD_ALIVE:
		{
			*out = ExecuteNewCmd(CECMD_ALIVE, sizeof(CESERVER_REQ_HDR));
			lbRc = TRUE;
		} break;
		#ifdef USE_COMMIT_EVENT
		case CECMD_REGEXTCONSOLE:
		{
			lbRc = cmd_RegExtConsole(in, out);
		} break;
		#endif
		case CECMD_FREEZEALTSRV:
		{
			lbRc = cmd_FreezeAltServer(in, out);
		} break;
		case CECMD_CONSOLEFULL:
		{
			lbRc = cmd_LoadFullConsoleData(in, out);
		} break;
		case CECMD_SETFULLSCREEN:
		{
			lbRc = cmd_SetFullScreen(in, out);
		} break;
		case CECMD_SETCONCOLORS:
		{
			lbRc = cmd_SetConColors(in, out);
		} break;
		case CECMD_SETCONTITLE:
		{
			lbRc = cmd_SetConTitle(in, out);
		} break;
		case CECMD_ALTBUFFER:
		{
			// Если крутится альтернативный сервер - команду нужно выполнять в нем
			if (!ProcessAltSrvCommand(in, out, lbRc))
			{
				lbRc = cmd_AltBuffer(in, out);
			}
		} break;
		case CECMD_ALTBUFFERSTATE:
		{
			// Если крутится альтернативный сервер - команду нужно выполнять в нем
			if (!ProcessAltSrvCommand(in, out, lbRc))
			{
				lbRc = cmd_AltBufferState(in, out);
			}
		} break;
		case CECMD_EXPORTVARS:
		{
			lbRc = cmd_ApplyExportVars(in, out);
		} break;
		case CECMD_UPDCONMAPHDR:
		{
			lbRc = cmd_UpdConMapHdr(in, out);
		} break;
		case CECMD_SETCONSCRBUF:
		{
			// Если крутится альтернативный сервер - команду нужно выполнять в нем
			if (!ProcessAltSrvCommand(in, out, lbRc))
			{
				lbRc = cmd_SetConScrBuf(in, out);
			}
		} break;
		case CECMD_PORTABLESTART:
		{
			lbRc = cmd_PortableStarted(in, out);
		} break;
		case CECMD_CTRLBREAK:
		{
			lbRc = cmd_CtrlBreakEvent(in, out);
		} break;
		case CECMD_SETTOPLEFT:
		{
			lbRc = cmd_SetTopLeft(in, out);
		} break;
		case CECMD_PROMPTSTARTED:
		{
			lbRc = cmd_PromptStarted(in, out);
		} break;
		case CECMD_LOCKSTATION:
		{
			// Execute command in the alternative server too
			ProcessAltSrvCommand(in, out, lbRc);
			lbRc = cmd_LockStation(in, out);
		} break;
		case CECMD_UNLOCKSTATION:
		{
			// Execute command in the alternative server too
			bool lbProcessed = ProcessAltSrvCommand(in, out, lbRc);
			lbRc = cmd_UnlockStation(in, out, lbProcessed);
		} break;
		case CECMD_GETROOTINFO:
		{
			lbRc = cmd_GetRootInfo(in, out);
		} break;
		case CECMD_WRITETEXT:
		{
			lbRc = cmd_WriteText(in, out);
		} break;
		case CECMD_FINDNEXTROWID:
		{
			lbRc = cmd_FindNextRowId(in, out);
		} break;
		case CECMD_STARTPTYSRV:
		{
			lbRc = cmd_StartPtySrv(in, out);
		} break;
		default:
		{
			// Отлов необработанных
			_ASSERTE(FALSE && "Command was not processed");
			lbRc = FALSE;
		}
	}

	if (gbInRecreateRoot) gbInRecreateRoot = FALSE;

	return lbRc;
}
