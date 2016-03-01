
/*
Copyright (c) 2009-2016 Maximus5
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

#define AssertCantActivate(x) //MBoxAssert(x)

#define SHOWDEBUGSTR
//#define ALLOWUSEFARSYNCHRO

#include "Header.h"
#include <Tlhelp32.h>
#include "../common/ConEmuCheck.h"
#include "../common/RgnDetect.h"
#include "../common/Execute.h"
#include "../common/PipeServer.h"
#include "../common/WUser.h"
#include "RealServer.h"
#include "RealConsole.h"
#include "RealBuffer.h"
#include "VirtualConsole.h"
#include "TabBar.h"
#include "ConEmu.h"
#include "ConEmuApp.h"
#include "VConChild.h"
#include "VConGroup.h"
#include "ConEmuPipe.h"
#include "Macro.h"
#include "OptionsClass.h"

#define DEBUGSTRCMD(s) //DEBUGSTR(s)
#define DEBUGSTRLANG(s) //DEBUGSTR(s)// ; Sleep(2000)


CRealServer::CRealServer()
{
	mp_RCon = NULL;
	mh_GuiAttached = NULL;
	mp_RConServer = NULL;
	mb_RealForcedTermination = false;
}

CRealServer::~CRealServer()
{
	Stop(true);
}

void CRealServer::Init(CRealConsole* apRCon)
{
	mp_RCon = apRCon;
	mp_RConServer = (PipeServer<CESERVER_REQ>*)calloc(3,sizeof(*mp_RConServer));
	mp_RConServer->SetMaxCount(3);
	//mh_ServerSemaphore = NULL;
	//memset(mh_RConServerThreads, 0, sizeof(mh_RConServerThreads));
	//mh_ActiveRConServerThread = NULL;
	//memset(mn_RConServerThreadsId, 0, sizeof(mn_RConServerThreadsId));
}

bool CRealServer::Start()
{
	DWORD nConWndID = (DWORD)(((DWORD_PTR)mp_RCon->hConWnd) & 0xFFFFFFFF);
	_wsprintf(mp_RCon->ms_VConServer_Pipe, SKIPLEN(countof(mp_RCon->ms_VConServer_Pipe)) CEGUIPIPENAME, L".", nConWndID);

	if (!mh_GuiAttached)
	{
		wchar_t szEvent[64];

		_wsprintf(szEvent, SKIPLEN(countof(szEvent)) CEGUIRCONSTARTED, nConWndID);
		//// Скорее всего событие в сервере еще не создано
		//mh_GuiAttached = OpenEvent(EVENT_MODIFY_STATE, FALSE, mp_RCon->ms_VConServer_Pipe);
		//// Вроде, когда используется run as administrator - event открыть не получается?
		//if (!mh_GuiAttached) {
		mh_GuiAttached = CreateEvent(gpLocalSecurity, TRUE, FALSE, szEvent);
		_ASSERTE(mh_GuiAttached!=NULL);
		//}
	}

	mp_RConServer->SetOverlapped(true);
	mp_RConServer->SetLoopCommands(false);
	mp_RConServer->SetDummyAnswerSize(sizeof(CESERVER_REQ_HDR));

	// ConEmuC ожидает готовый пайп после возврата из CECMD_SRVSTARTSTOP
	if (!mp_RConServer->StartPipeServer(false, mp_RCon->ms_VConServer_Pipe, (LPARAM)this, LocalSecurity(),
			ServerCommand, ServerCommandFree, NULL, NULL, ServerThreadReady))
	{
		MBoxAssert("mp_RConServer->StartPipeServer"==0);
		return false;
	}

	//if (!mh_ServerSemaphore)
	//	mh_ServerSemaphore = CreateSemaphore(NULL, 1, 1, NULL);
	//for (int i=0; i<MAX_SERVER_THREADS; i++)
	//{
	//	if (mh_RConServerThreads[i])
	//		continue;
	//	mn_RConServerThreadsId[i] = 0;
	//	mh_RConServerThreads[i] = apiCreateThread(NULL, 0, RConServerThread, (LPVOID)this, 0, &mn_RConServerThreadsId[i]);
	//	_ASSERTE(mh_RConServerThreads[i]!=NULL);
	//}

	// чтобы ConEmuC знал, что мы готовы
	//			if (mh_GuiAttached) {
	//				SetEvent(mh_GuiAttached);
	//			Sleep(10);
	//			SafeCloseHandle(mh_GuiAttached);
	//		}
	return true;
}

void CRealServer::Stop(bool abDeinitialize/*=false*/)
{
	//SafeCloseHandle(mh_ServerSemaphore);

	if (mp_RConServer)
	{
		mp_RConServer->StopPipeServer(false, mb_RealForcedTermination);

		if (abDeinitialize)
		{
			SafeFree(mp_RConServer);
		}
	}

	SafeCloseHandle(mh_GuiAttached);

	ShutdownGuiStep(L"Real server stopped");
}

CESERVER_REQ* CRealServer::cmdStartStop(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
{
	CESERVER_REQ* pOut = ExecuteNewCmd(pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_STARTSTOPRET));

	//
	DWORD nStarted = pIn->StartStop.nStarted;
	DWORD nParentFarPid = pIn->StartStop.nParentFarPID;
	HWND  hWnd     = (HWND)pIn->StartStop.hWnd;

#ifdef _DEBUG
	wchar_t szDbg[128];

	switch (nStarted)
	{
		case sst_ServerStart:
		case sst_ServerStop:
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"GUI received CECMD_CMDSTARTSTOP(%s,%i,PID=%u,From=%u)\n",
				(nStarted==sst_ServerStart)?L"ServerStart":L"ServerStop", pIn->hdr.nCreateTick, pIn->StartStop.dwPID, pIn->hdr.nSrcPID);
			break;
		case sst_AltServerStart:
		case sst_AltServerStop:
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"GUI received CECMD_CMDSTARTSTOP(%s,%i,PID=%u,From=%u)\n",
				(nStarted==sst_AltServerStart)?L"AltServerStart":L"AltServerStop", pIn->hdr.nCreateTick, pIn->StartStop.dwPID, pIn->hdr.nSrcPID);
			break;
		case sst_ComspecStart:
		case sst_ComspecStop:
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"GUI received CECMD_CMDSTARTSTOP(%s,%i,PID=%u,From=%u)\n",
				(nStarted==sst_ComspecStart)?L"ComspecStart":L"ComspecStop", pIn->hdr.nCreateTick, pIn->StartStop.dwPID, pIn->hdr.nSrcPID);
			break;
		case sst_AppStart:
		case sst_AppStop:
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"GUI received CECMD_CMDSTARTSTOP(%s,%i,PID=%u,From=%u)\n",
				(nStarted==sst_AppStart)?L"AppStart":L"AppStop", pIn->hdr.nCreateTick, pIn->StartStop.dwPID, pIn->hdr.nSrcPID);
			break;
		case sst_App16Start:
		case sst_App16Stop:
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"GUI received CECMD_CMDSTARTSTOP(%s,%i,PID=%u,From=%u)\n",
				(nStarted==sst_App16Start)?L"App16Start":L"App16Stop", pIn->hdr.nCreateTick, pIn->StartStop.dwPID, pIn->hdr.nSrcPID);
			break;
		default:
			_ASSERTE(nStarted==sst_ServerStart && "Unknown start code");
	}

	DEBUGSTRCMD(szDbg);
#endif

	_ASSERTE(pIn->StartStop.dwPID!=0);
	DWORD nPID     = pIn->StartStop.dwPID;
	DWORD nSubSystem = pIn->StartStop.nSubSystem;
	BOOL bRunViaCmdExe = pIn->StartStop.bRootIsCmdExe;
	BOOL bUserIsAdmin = pIn->StartStop.bUserIsAdmin;
	BOOL lbWasBuffer = pIn->StartStop.bWasBufferHeight;
	HANDLE hServerProcessHandle = (HANDLE)(DWORD_PTR)pIn->StartStop.hServerProcessHandle;
	//DWORD nInputTID = pIn->StartStop.dwInputTID;
	_ASSERTE(sizeof(CESERVER_REQ_STARTSTOPRET) <= sizeof(CESERVER_REQ_STARTSTOP));
	//pIn->hdr.cbSize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_STARTSTOPRET);

	// Если процесс отваливается (кроме корневого сервера) - добавить его в mp_RCon->m_TerminatedPIDs
	TODO("Проверить, нужно ли добавлять отваливающийся sst_App16Stop? По идее, все равно после него должен sst_ComspecStop прийти?");
	if (nStarted == sst_ComspecStop || nStarted == sst_AppStop /*|| nStarted == sst_App16Stop*/)
	{
		bool lbPushed = false;
		// Может он уже добавлен в mp_RCon->m_TerminatedPIDs (хотя не должен по идее)
		for (size_t i = 0; i < countof(mp_RCon->m_TerminatedPIDs); i++)
		{
			if (mp_RCon->m_TerminatedPIDs[i] == nPID)
			{
				lbPushed = true;
				break;
			}
		}
		// Найти первую пустую и добавить
		for (UINT k = 0; !lbPushed && k <= 1; k++)
		{
			UINT iStart = !k ? mp_RCon->mn_TerminatedIdx : 0;
			UINT iEnd = !k ? countof(mp_RCon->m_TerminatedPIDs) : min(mp_RCon->mn_TerminatedIdx,countof(mp_RCon->m_TerminatedPIDs));
			// циклический буфер
			for (UINT i = iStart; i < iEnd; i++)
			{
				if (!mp_RCon->m_TerminatedPIDs[i])
				{
					mp_RCon->m_TerminatedPIDs[i] = nPID;
					mp_RCon->mn_TerminatedIdx = ((i + 1) < countof(mp_RCon->m_TerminatedPIDs)) ? (i + 1) : 0;
					lbPushed = true;
					break;
				}
			}
		}
	}


	if ((nStarted == sst_AltServerStart) || (nStarted == sst_AltServerStop))
	{
		// Перейти в режим AltServer, переоткрыть m_GetDataPipe
		// -- команда старта альп.сервера должна приходить из главного сервера
		_ASSERTE(pIn->StartStop.dwPID == nPID && nPID != pIn->hdr.nSrcPID && pIn->hdr.nSrcPID == mp_RCon->mn_MainSrv_PID);

		// При закрытия альт.сервера может также (сразу) закрываться и главный сервер
		// в этом случае, переоткрывать пайпы смысла не имеет!
		if ((nStarted == sst_AltServerStop)
			&& pIn->StartStop.bMainServerClosing)
		{
			if (pIn->hdr.nSrcPID == mp_RCon->mn_MainSrv_PID) // должно приходить из главного сервера
			{
				_ASSERTE(FALSE && "CECMD_SRVSTARTSTOP is used instead");
				mp_RCon->OnServerClosing(mp_RCon->mn_MainSrv_PID, NULL);
			}
			else
			{
				_ASSERTE(pIn->hdr.nSrcPID == mp_RCon->mn_MainSrv_PID && "Must arrive from main server");
			}
		}
		else
		{
			_ASSERTE((nStarted == sst_AltServerStop) || !pIn->StartStop.bMainServerClosing);
		}

		// Если процесс запущен под другим логином - передать хэндл (hServerProcessHandle) не получится
		mp_RCon->InitAltServer((nStarted == sst_AltServerStart) ? nPID : 0);

		// В принципе, альт. сервер уже все знает, но вернем...
		pOut->StartStopRet.hWnd = ghWnd;
		pOut->StartStopRet.hWndDc = mp_RCon->mp_VCon->GetView();
		pOut->StartStopRet.hWndBack = mp_RCon->mp_VCon->GetBack();
		pOut->StartStopRet.dwPID = GetCurrentProcessId();
		if (lbWasBuffer != mp_RCon->isBufferHeight())
		{
			mp_RCon->mp_RBuf->BuferModeChangeLock();
			mp_RCon->mp_RBuf->SetBufferHeightMode(lbWasBuffer, TRUE); // Сразу меняем, иначе команда неправильно сформируется
			//mp_RCon->mp_RBuf->SetConsoleSize(mp_RCon->mp_RBuf->GetTextWidth()/*con.m_sbi.dwSize.X*/, mp_RCon->mp_RBuf->TextHeight(), pOut->StartStopRet.nBufferHeight, CECMD_CMDSTARTED);
			mp_RCon->mp_RBuf->BuferModeChangeUnlock();
		}
		pOut->StartStopRet.bWasBufferHeight = mp_RCon->isBufferHeight();
		pOut->StartStopRet.nBufferHeight = pOut->StartStopRet.bWasBufferHeight ? pIn->StartStop.sbi.dwSize.Y : 0;
		pOut->StartStopRet.nWidth = mp_RCon->mp_RBuf->GetBufferWidth()/*con.m_sbi.dwSize.X*/;
		pOut->StartStopRet.nHeight = mp_RCon->mp_RBuf->TextHeight()/*con.m_sbi.dwSize.Y*/;

	}
	else if (nStarted == sst_ServerStart || nStarted == sst_ComspecStart)
	{
		if (nStarted == sst_ServerStart)
		{
			wchar_t szInfo[80];
			_wsprintf(szInfo, SKIPCOUNT(szInfo) L"Console server started PID=%u...", nPID);
			mp_RCon->SetConStatus(szInfo, CRealConsole::cso_ResetOnConsoleReady|CRealConsole::cso_Critical);

			// Активным должен быть реальный буфер
			_ASSERTE(mp_RCon->mp_ABuf==mp_RCon->mp_RBuf);
			mp_RCon->mp_RBuf->InitMaxSize(pIn->StartStop.crMaxSize);
		}

		// Сразу заполним результат
		pOut->StartStopRet.bWasBufferHeight = mp_RCon->isBufferHeight(); // чтобы comspec знал, что буфер нужно будет отключить
		//DWORD nParentPID = 0;
		//if (nStarted == 2)
		//{
		//	ConProcess* pPrc = NULL;
		//	int i, nProcCount = GetProcesses(&pPrc);
		//	if (pPrc != NULL)
		//	{
		//		for (i = 0; i < nProcCount; i++) {
		//			if (pPrc[i].ProcessID == nPID) {
		//				nParentPID = pPrc[i].ParentPID; break;
		//			}
		//		}
		//		if (nParentPID == 0) {
		//			_ASSERTE(nParentPID != 0);
		//		} else {
		//			BOOL lbFar = FALSE;
		//			for (i = 0; i < nProcCount; i++) {
		//				if (pPrc[i].ProcessID == nParentPID) {
		//					lbFar = pPrc[i].IsFar; break;
		//				}
		//			}
		//			if (!lbFar) {
		//				_ASSERTE(lbFar);
		//				nParentPID = 0;
		//			}
		//		}
		//		free(pPrc);
		//	}
		//}
		//pOut->StartStopRet.bWasBufferHeight = FALSE;// (nStarted == 2) && (nParentPID == 0); // comspec должен уведомить о завершении
		pOut->StartStopRet.hWnd = ghWnd;
		pOut->StartStopRet.hWndDc = mp_RCon->mp_VCon->GetView();
		pOut->StartStopRet.hWndBack = mp_RCon->mp_VCon->GetBack();
		pOut->StartStopRet.dwPID = GetCurrentProcessId();
		if (nStarted == sst_ServerStart)
		{
			_ASSERTE(mp_RCon->mn_MainSrv_PID == pIn->hdr.nSrcPID);
			pOut->StartStopRet.dwMainSrvPID = mp_RCon->mn_MainSrv_PID;
			pOut->StartStopRet.dwAltSrvPID = mp_RCon->mn_AltSrv_PID;
		}
		else
		{
			_ASSERTE(nStarted == sst_ComspecStart);
			//pOut->StartStopRet.dwSrvPID = mp_RCon->GetServerPID();
			pOut->StartStopRet.dwMainSrvPID = mp_RCon->mn_MainSrv_PID;
			pOut->StartStopRet.dwAltSrvPID = mp_RCon->mn_AltSrv_PID;
		}
		pOut->StartStopRet.bNeedLangChange = FALSE;

		if (nStarted == sst_ServerStart)
		{
			//_ASSERTE(nInputTID);
			//_ASSERTE(mn_ConEmuC_Input_TID==0 || mn_ConEmuC_Input_TID==nInputTID);
			//mn_ConEmuC_Input_TID = nInputTID;
			//
			if ((mp_RCon->m_Args.RunAsAdministrator != crb_On) && bUserIsAdmin)
				mp_RCon->m_Args.RunAsAdministrator = crb_On;

			if (mp_RCon->mn_InRecreate>=1)
				mp_RCon->mn_InRecreate = 0; // корневой процесс успешно пересоздался

			// Если один Layout на все консоли
			if ((gpSet->isMonitorConsoleLang & 2) == 2)
			{
				// Команду - низя, нити еще не функционируют
				//	SwitchKeyboardLayout(INPUTLANGCHANGE_SYSCHARSET,gpConEmu->GetActiveKeyboardLayout());
				pOut->StartStopRet.bNeedLangChange = TRUE;
				TODO("Проверить на x64, не будет ли проблем с 0xFFFFFFFFFFFFFFFFFFFFF");
				pOut->StartStopRet.NewConsoleLang = gpConEmu->GetActiveKeyboardLayout();
			}

			// Теперь мы гарантированно знаем дескриптор окна консоли
			mp_RCon->SetHwnd(hWnd, TRUE);
			// Открыть мэппинги, выставить KeyboardLayout, и т.п.
			mp_RCon->OnServerStarted(pIn->StartStop.dwPID, hServerProcessHandle, pIn->StartStop.dwKeybLayout);
		}

		AllowSetForegroundWindow(nPID);

		COORD cr16bit = mp_RCon->mp_RBuf->GetDefaultNtvdmHeight();

		/*
		#define IMAGE_SUBSYSTEM_DOS_EXECUTABLE  255
		#define IMAGE_SUBSYSTEM_BATCH_FILE  254
		*/

		// ComSpec started
		if (nStarted == sst_ComspecStart)
		{
			// Устанавливается в TRUE если будет запущено 16битное приложение
			if (nSubSystem == IMAGE_SUBSYSTEM_DOS_EXECUTABLE/*255*/)
			{
				DEBUGSTRCMD(L"16 bit application STARTED, aquired from CECMD_CMDSTARTSTOP\n");

				//if (!(mp_RCon->mn_ProgramStatus & CES_NTVDM))
				//	mp_RCon->mn_ProgramStatus |= CES_NTVDM; -- в mp_RCon->OnDosAppStartStop

				mp_RCon->mn_Comspec4Ntvdm = nPID;
				mp_RCon->OnDosAppStartStop(sst_App16Start, nPID);
				//mp_RCon->mb_IgnoreCmdStop = TRUE; -- уже, в mp_RCon->OnDosAppStartStop

				mp_RCon->mp_RBuf->SetConsoleSize(cr16bit.X, cr16bit.Y, 0, CECMD_CMDSTARTED);
				pOut->StartStopRet.nBufferHeight = 0;
				pOut->StartStopRet.nWidth = cr16bit.X;
				pOut->StartStopRet.nHeight = cr16bit.Y;
			}
			else
			{
				bool bDontTouchBuffer = false;
				int iDefaultBufferHeight = gpSet->DefaultBufferHeight;
				BOOL bAllowBufferHeight = (gpSet->AutoBufferHeight || mp_RCon->isBufferHeight()) && (nParentFarPid != 0);

				if (pIn->StartStop.sbi.dwSize.Y == 0)
				{
					// Issue 1763: Assertion while starting something with redirection
					bDontTouchBuffer = true;
				}
				else if (pIn->StartStop.bForceBufferHeight)
				{
					bAllowBufferHeight = (pIn->StartStop.nForceBufferHeight != 0);
				}
				else
				{
					int nNewWidth = 0, nNewHeight = 0; DWORD nScroll = 0;
					if (mp_RCon->mp_RBuf->GetConWindowSize(pIn->StartStop.sbi, &nNewWidth, &nNewHeight, &nScroll))
					{
						if (nScroll & rbs_Vert)
						{
							bAllowBufferHeight = TRUE;
							iDefaultBufferHeight = pIn->StartStop.sbi.dwSize.Y;
						}
					}
				}

				// но пока его нужно сбросить
				mp_RCon->mb_IgnoreCmdStop = FALSE;
				// Если пользователь указал требуемое к строке запуска приложения
				if (pIn->StartStop.bForceBufferHeight)
				{
					pOut->StartStopRet.nBufferHeight = pIn->StartStop.nForceBufferHeight;
				}
				else
				{
					// в ComSpec передаем именно то, что сейчас в gpSet
					pOut->StartStopRet.nBufferHeight = bAllowBufferHeight ? iDefaultBufferHeight : 0;
				}
				// 111125 было "con.m_sbi.dwSize.X" и "con.m_sbi.dwSize.X"
				pOut->StartStopRet.nWidth = mp_RCon->mp_RBuf->GetBufferWidth()/*con.m_sbi.dwSize.X*/;
				pOut->StartStopRet.nHeight = mp_RCon->mp_RBuf->GetBufferHeight()/*con.m_sbi.dwSize.Y*/;

				bool bNewBuffer = (pOut->StartStopRet.nBufferHeight != 0);
				bool bOldBuffer = (mp_RCon->isBufferHeight() != FALSE);
				if (!bDontTouchBuffer && (bNewBuffer != bOldBuffer))
				{
					WARNING("Тут наверное нужно бы заблокировать прием команды смена размера из сервера ConEmuC");
					//con.m_sbi.dwSize.Y = gpSet->DefaultBufferHeight; -- не будем менять сразу, а то SetConsoleSize просто skip
					mp_RCon->mp_RBuf->BuferModeChangeLock();
					mp_RCon->mp_RBuf->SetBufferHeightMode((pOut->StartStopRet.nBufferHeight != 0), TRUE); // Сразу меняем, иначе команда неправильно сформируется
					mp_RCon->mp_RBuf->SetConsoleSize(mp_RCon->mp_RBuf->GetTextWidth()/*con.m_sbi.dwSize.X*/, mp_RCon->mp_RBuf->TextHeight(), pOut->StartStopRet.nBufferHeight, CECMD_CMDSTARTED);
					WARNING("Переделать! Размер нужно просто вернуть в сервер, а он сам разберется");
					mp_RCon->mp_RBuf->BuferModeChangeUnlock();
				}
			}

			// Раз стартован ComSpec (cmd.exe/ConEmuC.exe/...)
			mp_RCon->SetFarPID(0);
		}
		else if (nStarted == sst_ServerStart)
		{
			BOOL b = mp_RCon->mp_RBuf->BuferModeChangeLock();

			// Server
			if (nSubSystem == IMAGE_SUBSYSTEM_DOS_EXECUTABLE/*255*/)
			{
				pOut->StartStopRet.nBufferHeight = 0;
				pOut->StartStopRet.nWidth = cr16bit.X;
				pOut->StartStopRet.nHeight = cr16bit.Y;
			}
			else
			{
				pOut->StartStopRet.nWidth = mp_RCon->mp_RBuf->GetBufferWidth()/*con.m_sbi.dwSize.X*/;

				//0x101 - запуск отладчика
				if (nSubSystem != 0x100   // 0x100 - Аттач из фар-плагина
				        && (mp_RCon->mp_RBuf->isScroll()
				            || (mp_RCon->mn_DefaultBufferHeight && bRunViaCmdExe)))
				{
					// Смысл ассерта в том, что консоль запускаемая ИЗ ConEmu должна стартовать
					// с корректным размером (заранее заданные через параметры для ConEmuC)
					// А вот если идет аттач внешних консолей - то размер будет отличаться (и это нормально)
					_ASSERTE(mp_RCon->mb_WasStartDetached || mp_RCon->mn_DefaultBufferHeight == mp_RCon->mp_RBuf->GetBufferHeight()/*con.m_sbi.dwSize.Y*/ || mp_RCon->mp_RBuf->GetBufferHeight()/*con.m_sbi.dwSize.Y*/ == mp_RCon->TextHeight());

					pOut->StartStopRet.nBufferHeight = max(mp_RCon->mp_RBuf->GetBufferHeight()/*con.m_sbi.dwSize.Y*/,mp_RCon->mn_DefaultBufferHeight);
					_ASSERTE(mp_RCon->mp_RBuf->TextHeight()/*con.nTextHeight*/ >= 1);
					pOut->StartStopRet.nHeight = mp_RCon->mp_RBuf->TextHeight()/*con.nTextHeight*/;
					//111126 - убрал. выше буфер блокируется
					//con.m_sbi.dwSize.Y = pOut->StartStopRet.nBufferHeight; // Сразу обновить, иначе буфер может сброситься самопроизвольно
				}
				else
				{
					_ASSERTE(!mp_RCon->mp_RBuf->isScroll());
					pOut->StartStopRet.nBufferHeight = 0;
					pOut->StartStopRet.nHeight = mp_RCon->mp_RBuf->TextHeight()/*con.m_sbi.dwSize.Y*/;
				}

			}

			mp_RCon->mp_RBuf->SetBufferHeightMode((pOut->StartStopRet.nBufferHeight != 0), TRUE);
			mp_RCon->mp_RBuf->SetChange2Size(pOut->StartStopRet.nWidth, pOut->StartStopRet.nHeight);

			if (b) mp_RCon->mp_RBuf->BuferModeChangeUnlock();
		}

		// 23.06.2009 Maks - уберем пока. Должно работать в ApplyConsoleInfo
		//Process Add(nPID);

	} // (nStarted == sst_ServerStart || nStarted == sst_ComspecStart)
	else if (nStarted == sst_ServerStop || nStarted == sst_ComspecStop)
	{
		// ServerStop вроде не приходит - посылается CECMD_SRVSTARTSTOP в ConEmuWnd
		// Может быть для AltServer???
		_ASSERTE(nStarted != sst_ServerStop);

		// 23.06.2009 Maks - уберем пока. Должно работать в ApplyConsoleInfo
		//Process Delete(nPID);

		// ComSpec stopped
		if ((nStarted == sst_ComspecStop) && (nParentFarPid == 0))
		{
			// из cmd.exe была запущена команда с "-new_console"
			// Сервер она не интересует
			_ASSERTE(!mp_RCon->isNtvdm() && "Need resize console after NTVDM?");
		}
		else if (nStarted == sst_ComspecStop)
		{
			BOOL lbNeedResizeWnd = FALSE;
			BOOL lbNeedResizeGui = FALSE;
			// {mp_RCon->TextWidth(),mp_RCon->TextHeight()} использовать нельзя,
			// т.к. при если из фара выполняется "cmd -new_console:s" то при завершении
			// RM_COMSPEC выполняется "возврат" размера буфера и это обламывает синхронизацию
			// размера под измененную конфигурацию сплитов...
			RECT rcCon = gpConEmu->CalcRect(CER_CONSOLE_CUR, mp_RCon->mp_VCon);
			//COORD crNewSize = {mp_RCon->TextWidth(),mp_RCon->TextHeight()};
			COORD crNewSize = {(SHORT)rcCon.right, (SHORT)rcCon.bottom};
			int nNewWidth=0, nNewHeight=0;

			if ((mp_RCon->mn_ProgramStatus & CES_NTVDM) == 0
			        && (gpConEmu->GetWindowMode() == wmNormal))
			{
				pOut->StartStopRet.bWasBufferHeight = FALSE;

				// В некоторых случаях (comspec без консоли?) GetConsoleScreenBufferInfo обламывается
				if (pOut->StartStop.sbi.dwSize.X && pOut->StartStop.sbi.dwSize.Y)
				{
					DWORD nScroll = 0;

					// Интересуют реальные размеры консоли, определенные при текущему SBI
					// 111125 - bBufferHeight заменен на nScroll (который учитывает и наличие горизонтальной прокрутки)
					if (mp_RCon->mp_RBuf->GetConWindowSize(pOut->StartStop.sbi, &nNewWidth, &nNewHeight, &nScroll))
					{
						lbNeedResizeGui = (crNewSize.X != nNewWidth || crNewSize.Y != nNewHeight);

						WARNING("ConResize: Сомнительно, что тут нужно ресайзить GUI");
						if (nScroll || crNewSize.X != nNewWidth || crNewSize.Y != nNewHeight)
						{
							// Это к вопросу о том, может ли консольное приложение менять размер окна,
							// или это может делать только юзер, меняя размер окна ConEmu
							// Буфер менять можно (он и не проверяется), а вот размер видимой области...
							// Хотя, может быть, например, команда "mode con lines=25 cols=80"
							_ASSERTE(crNewSize.X == nNewWidth && crNewSize.Y == nNewHeight);

							//CVConGroup::SyncWindowToConsole(); - его использовать нельзя. во первых это не главная нить, во вторых - размер pVCon может быть еще не изменен
							lbNeedResizeWnd = TRUE;

							crNewSize.X = nNewWidth;
							crNewSize.Y = nNewHeight;

							//pOut->StartStopRet.bWasBufferHeight = TRUE; -- 111124 всегда ставилось pOut->StartStopRet.bWasBufferHeight=TRUE;
							_ASSERTE(nScroll!=0);
							pOut->StartStopRet.bWasBufferHeight = (nScroll!=0);
						}
					}
				}
			}

			if (mp_RCon->mb_IgnoreCmdStop || (mp_RCon->mn_ProgramStatus & CES_NTVDM) == CES_NTVDM)
			{
				// Ветка активируется только в WinXP и выше
				// Было запущено 16битное приложение, сейчас запомненный размер консоли скорее всего 80x25
				// что не соответствует желаемому размеру при выходе из 16бит. Консоль нужно подресайзить
				// поз размер окна. Это сделает OnWinEvent.
				//SetBufferHeightMode(FALSE, TRUE);
				//WARNING("Если не вызвать CECMD_CMDFINISHED не включатся WinEvents");
				mp_RCon->mb_IgnoreCmdStop = FALSE; // наверное сразу сбросим, две подряд прийти не могут
				DEBUGSTRCMD(L"16 bit application TERMINATED (aquired from CECMD_CMDFINISHED)\n");

				//mp_RCon->mn_ProgramStatus &= ~CES_NTVDM; -- сбросим после синхронизации размера консоли, чтобы не слетел
				if (lbWasBuffer)
				{
					mp_RCon->mp_RBuf->SetBufferHeightMode(TRUE, TRUE); // Сразу выключаем, иначе команда неправильно сформируется
				}

				mp_RCon->SyncConsole2Window(TRUE); // После выхода из 16bit режима хорошо бы отресайзить консоль по размеру GUI

				if (mp_RCon->mn_Comspec4Ntvdm)
				{
					#ifdef _DEBUG
					if (mp_RCon->mn_Comspec4Ntvdm != nPID)
					{
						_ASSERTE(mp_RCon->mn_Comspec4Ntvdm == nPID);
					}
					#endif
					if ((nSubSystem == IMAGE_SUBSYSTEM_DOS_EXECUTABLE) || (mp_RCon->mn_Comspec4Ntvdm == nPID))
						mp_RCon->mn_Comspec4Ntvdm = 0;
				}

				// mp_RCon->mn_ProgramStatus &= ~CES_NTVDM;
				if ((nSubSystem == IMAGE_SUBSYSTEM_DOS_EXECUTABLE)
					|| ((mp_RCon->mn_ProgramStatus & CES_NTVDM) == CES_NTVDM))
				{
					_ASSERTE(nSubSystem == IMAGE_SUBSYSTEM_DOS_EXECUTABLE)
					mp_RCon->OnDosAppStartStop(sst_App16Stop, nPID);
				}

				lbNeedResizeWnd = FALSE;
				crNewSize.X = mp_RCon->TextWidth();
				crNewSize.Y = mp_RCon->TextHeight();
			} //else {

			// Восстановить размер через серверный ConEmuC
			mp_RCon->mp_RBuf->BuferModeChangeLock();

			//111126 - убрал, ниже зовется SetConsoleSize
			//con.m_sbi.dwSize.Y = crNewSize.Y;

			if (!lbWasBuffer)
			{
				mp_RCon->mp_RBuf->SetBufferHeightMode(FALSE, TRUE); // Сразу выключаем, иначе команда неправильно сформируется
			}

			#ifdef _DEBUG
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"Returns normal window size begin at %i\n", GetTickCount());
			DEBUGSTRCMD(szDbg);
			#endif

			// Обязательно. Иначе сервер не узнает, что команда завершена
			mp_RCon->mp_RBuf->SetConsoleSize(crNewSize.X, crNewSize.Y, 0, CECMD_CMDFINISHED);

			#ifdef _DEBUG
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"Finished returns normal window size begin at %i\n", GetTickCount());
			DEBUGSTRCMD(szDbg);
			#endif

			#ifdef _DEBUG
			#ifdef WIN64
			//				WARNING("Есть подозрение, что после этого на Win7 x64 приходит старый пакет с буферной высотой и возникает уже некорректная синхронизация размера!");
			#endif
			#endif
			// может nChange2TextWidth, nChange2TextHeight нужно использовать?

			if (lbNeedResizeGui)
			{
				_ASSERTE(FALSE && "GUI must not follow console sizes");
				#if 0
				RECT rcCon = MakeRect(nNewWidth, nNewHeight);
				RECT rcNew = gpConEmu->CalcRect(CER_MAIN, rcCon, CER_CONSOLE);
				RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);

				if (gpSet->gpConEmu->opt.DesktopMode)
				{
					MapWindowPoints(NULL, gpConEmu->mh_ShellWindow, (LPPOINT)&rcWnd, 2);
				}

				MOVEWINDOW(ghWnd, rcWnd.left, rcWnd.top, rcNew.right, rcNew.bottom, 1);
				#endif
			}

			mp_RCon->mp_RBuf->BuferModeChangeUnlock();
			//}

			UNREFERENCED_PARAMETER(lbNeedResizeWnd);
		}
		else
		{
			// сюда мы попадать не должны!
			_ASSERTE(FALSE);
		}
	} // (nStarted == sst_ServerStop || nStarted == sst_ComspecStop)
	else if (nStarted == sst_App16Start || nStarted == sst_App16Stop)
	{
		mp_RCon->OnDosAppStartStop((enum StartStopType)nStarted, nPID);
	}
	else if (nStarted == sst_AppStop && pIn->StartStop.nSubSystem == IMAGE_SUBSYSTEM_WINDOWS_GUI)
	{
		_ASSERTE(mp_RCon->GuiWnd()!=NULL);
		mp_RCon->setGuiWnd(NULL);
	}

	// Готовим результат к отправке

	//pOut = ExecuteNewCmd(pIn->hdr.nCmd, pIn->hdr.cbSize);
	//if (pIn->hdr.cbSize > sizeof(CESERVER_REQ_HDR))
	//	memmove(pOut->Data, pIn->Data, pIn->hdr.cbSize - (int)sizeof(CESERVER_REQ_HDR));

	return pOut;
}

//CESERVER_REQ* CRealServer::cmdGetGuiHwnd(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
//{
//	CESERVER_REQ* pOut = NULL;
//
//	DEBUGSTRCMD(L"GUI recieved CECMD_GETGUIHWND\n");
//	pOut = ExecuteNewCmd(pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR) + 2*sizeof(DWORD));
//	pOut->dwData[0] = (DWORD)ghWnd; //-V205
//	pOut->dwData[1] = (DWORD)mp_VCon->GetView(); //-V205
//	return pOut;
//}

CESERVER_REQ* CRealServer::cmdTabsChanged(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
{
	CESERVER_REQ* pOut = NULL;

	DEBUGSTRCMD(L"GUI recieved CECMD_TABSCHANGED\n");

	BOOL fSuccess = FALSE;
	//DWORD cbWritten = 0;

	if (nDataSize == 0)
	{
		// ФАР закрывается
		if (pIn->hdr.nSrcPID == mp_RCon->mn_FarPID)
		{
			mp_RCon->mn_ProgramStatus &= ~CES_FARACTIVE;

			for (UINT i = 0; i < mp_RCon->mn_FarPlugPIDsCount; i++)  // сбросить ИД списка плагинов
			{
				if (mp_RCon->m_FarPlugPIDs[i] == mp_RCon->mn_FarPID)
					mp_RCon->m_FarPlugPIDs[i] = 0;
			}

			mp_RCon->SetFarPID(0);
			mp_RCon->SetFarPluginPID(0);
			mp_RCon->CloseFarMapData();

			if (mp_RCon->isActive(false)) gpConEmu->UpdateProcessDisplay(FALSE);  // обновить PID в окне настройки
		}

		mp_RCon->tabs.m_Tabs.MarkTabsInvalid(CTabStack::MatchNonPanel, pIn->hdr.nSrcPID);
		mp_RCon->SetTabs(NULL, 1, 0);
		gpConEmu->mp_TabBar->PrintRecentStack();
	}
	else
	{
		_ASSERTE(nDataSize>=4); //-V112
		_ASSERTE(((pIn->Tabs.nTabCount-1)*sizeof(ConEmuTab))==(nDataSize-sizeof(CESERVER_REQ_CONEMUTAB)));
		BOOL lbCanUpdate = TRUE;

		// Если выполняется макрос - изменение размеров окна FAR при авто-табах нежелательно
		if (pIn->Tabs.bMacroActive)
		{
			if (gpSet->isTabs == 2)
			{
				lbCanUpdate = FALSE;
				// Нужно вернуть в плагин информацию, что нужно отложенное обновление
				CESERVER_REQ *pRet = ExecuteNewCmd(CECMD_TABSCHANGED, sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_CONEMUTAB_RET));

				if (pRet)
				{
					pRet->TabsRet.bNeedPostTabSend = TRUE;
					// Отправляем (сразу, чтобы клиент не ждал, пока ГУЙ закончит свои процессы)
					fSuccess = mp_RConServer->DelayedWrite(pInst, pRet, pRet->hdr.cbSize);
					//fSuccess = WriteFile(
					//               hPipe,        // handle to pipe
					//               pRet,         // buffer to write from
					//               pRet->hdr.cbSize,  // number of bytes to write
					//               &cbWritten,   // number of bytes written
					//               NULL);        // not overlapped I/O
					ExecuteFreeResult(pRet);

					// Чтобы в конце метода не дергаться
					pOut = (CESERVER_REQ*)INVALID_HANDLE_VALUE;
				}
			}
		}

		if (lbCanUpdate)
		{
			TODO("DoubleView: все видимые");
			//gpConEmu->ActiveCon()->Invalidate();
			CVConGroup::InvalidateAll();
			mp_RCon->SetTabs(pIn->Tabs.tabs, pIn->Tabs.nTabCount, pIn->hdr.nSrcPID);
			gpConEmu->mp_TabBar->SetRedraw(TRUE);
			//gpConEmu->ActiveCon()->Redraw();
			CVConGroup::Redraw();
		}
	}

	// Если еще не ответили плагину
	if (pOut == NULL)
		pOut = ExecuteNewCmd(pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR));

	return pOut;
}

CESERVER_REQ* CRealServer::cmdGetOutputFile(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
{
	CESERVER_REQ* pOut = NULL;

	DEBUGSTRCMD(L"GUI recieved CECMD_GETOUTPUTFILE\n");
	_ASSERTE(nDataSize>=4); //-V112
	BOOL lbUnicode = pIn->OutputFile.bUnicode;
	pOut = ExecuteNewCmd(pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_OUTPUTFILE));
	pOut->OutputFile.bUnicode = lbUnicode;
	pOut->OutputFile.szFilePathName[0] = 0; // Сформирует mp_RCon->PrepareOutputFile

	if (!mp_RCon->PrepareOutputFile(lbUnicode, pOut->OutputFile.szFilePathName))
	{
		pOut->OutputFile.szFilePathName[0] = 0;
	}

	return pOut;
}

CESERVER_REQ* CRealServer::cmdGuiMacro(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
{
	CESERVER_REQ* pOut = NULL;

	DEBUGSTRCMD(L"GUI recieved CECMD_GUIMACRO\n");
	DWORD nFarPluginPID = mp_RCon->GetFarPID(true);
	LPWSTR pszResult = ConEmuMacro::ExecuteMacro(pIn->GuiMacro.sMacro, mp_RCon, (nFarPluginPID==pIn->hdr.nSrcPID), &pIn->GuiMacro);

	int nLen = pszResult ? _tcslen(pszResult) : 0;
	pOut = ExecuteNewCmd(pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_GUIMACRO)+nLen*sizeof(wchar_t));

	if (pszResult)
	{
		lstrcpy(pOut->GuiMacro.sMacro, pszResult);
		pOut->GuiMacro.nSucceeded = 1;
		free(pszResult);
	}
	else
	{
		pOut->GuiMacro.sMacro[0] = 0;
		pOut->GuiMacro.nSucceeded = 0;
	}

	return pOut;
}

CESERVER_REQ* CRealServer::cmdLangChange(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
{
	CESERVER_REQ* pOut = NULL;

	DEBUGSTRLANG(L"GUI recieved CECMD_LANGCHANGE\n");
	_ASSERTE(nDataSize>=4); //-V112
	// LayoutName: "00000409", "00010409", ...
	// А HKL от него отличается, так что передаем DWORD
	// HKL в x64 выглядит как: "0x0000000000020409", "0xFFFFFFFFF0010409"
	DWORD dwName = pIn->dwData[0];

	mp_RCon->OnConsoleKeyboardLayout(dwName);

	pOut = ExecuteNewCmd(pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR));

	//#ifdef _DEBUG
	////Sleep(2000);
	//WCHAR szMsg[255];
	//// --> Видимо именно это не "нравится" руслату. Нужно переправить Post'ом в основную нить
	//HKL hkl = GetKeyboardLayout(0);
	//swprintf_c(szMsg, L"ConEmu: GetKeyboardLayout(0) on CECMD_LANGCHANGE after GetKeyboardLayout(0) = 0x%08I64X\n",
	//	(unsigned __int64)(DWORD_PTR)hkl);
	//DEBUGSTRLANG(szMsg);
	////Sleep(2000);
	//#endif
	//
	//wchar_t szName[10]; swprintf_c(szName, L"%08X", dwName);
	//DWORD_PTR dwNewKeybLayout = (DWORD_PTR)LoadKeyboardLayout(szName, 0);
	//
	//#ifdef _DEBUG
	//DEBUGSTRLANG(L"ConEmu: Calling GetKeyboardLayout(0)\n");
	////Sleep(2000);
	//hkl = GetKeyboardLayout(0);
	//swprintf_c(szMsg, L"ConEmu: GetKeyboardLayout(0) after LoadKeyboardLayout = 0x%08I64X\n",
	//	(unsigned __int64)(DWORD_PTR)hkl);
	//DEBUGSTRLANG(szMsg);
	////Sleep(2000);
	//#endif
	//
	//if ((gpSet->isMonitorConsoleLang & 1) == 1) {
	//    if (con.dwKeybLayout != dwNewKeybLayout) {
	//        con.dwKeybLayout = dwNewKeybLayout;
	//		if (mp_RCon->isActive(false)) {
	//            gpConEmu->SwitchKeyboardLayout(dwNewKeybLayout);
	//
	//			#ifdef _DEBUG
	//			hkl = GetKeyboardLayout(0);
	//			swprintf_c(szMsg, L"ConEmu: GetKeyboardLayout(0) after SwitchKeyboardLayout = 0x%08I64X\n",
	//				(unsigned __int64)(DWORD_PTR)hkl);
	//			DEBUGSTRLANG(szMsg);
	//			//Sleep(2000);
	//			#endif
	//		}
	//    }
	//}
	return pOut;
}

CESERVER_REQ* CRealServer::cmdTabsCmd(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
{
	CESERVER_REQ* pOut = NULL;

	// 0: спрятать/показать табы, 1: перейти на следующую, 2: перейти на предыдущую, 3: commit switch
	DEBUGSTRCMD(L"GUI recieved CECMD_TABSCMD\n");
	_ASSERTE(nDataSize>=1);
	DWORD nTabCmd = pIn->Data[0];
	gpConEmu->TabCommand((ConEmuTabCommand)nTabCmd);
	pOut = ExecuteNewCmd(pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR));

	return pOut;
}

CESERVER_REQ* CRealServer::cmdResources(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
{
	CESERVER_REQ* pOut = NULL;

	DEBUGSTRCMD(L"GUI recieved CECMD_RESOURCES\n");
	_ASSERTE(nDataSize>=6);
	//mb_PluginDetected = TRUE; // Запомним, что в фаре есть плагин (хотя фар может быть закрыт)
	DWORD nPID = pIn->dwData[0]; // Запомним, что в фаре есть плагин
	mp_RCon->mb_SkipFarPidChange = TRUE;
	// Запомнить этот PID в списке фаров
	bool bAlreadyExist = false;
	int j = -1;

	for (UINT i = 0; i < mp_RCon->mn_FarPlugPIDsCount; i++)
	{
		if (mp_RCon->m_FarPlugPIDs[i] == nPID)
		{
			bAlreadyExist = true; break;
		}
		else if (mp_RCon->m_FarPlugPIDs[i] == 0)
		{
			j = i;
		}
	}

	if (!bAlreadyExist)
	{
		// Если с списке фаров этого PIDа еще нет - по возможности запомнить
		if ((j == -1) && (mp_RCon->mn_FarPlugPIDsCount < countof(mp_RCon->m_FarPlugPIDs)))
			j = mp_RCon->mn_FarPlugPIDsCount++;

		if (j >= 0)
			mp_RCon->m_FarPlugPIDs[j] = nPID;
	}

	// Запомним, что в фаре есть плагин
	mp_RCon->SetFarPluginPID(nPID);
	mp_RCon->OpenFarMapData(); // переоткроет мэппинг с информацией о фаре
	// Разрешить мониторинг PID фара в MonitorThread (оно будет переоткрывать mp_RCon->OpenFarMapData)
	mp_RCon->mb_SkipFarPidChange = FALSE;

	if (mp_RCon->isActive(false)) gpConEmu->UpdateProcessDisplay(FALSE);  // обновить PID в окне настройки

	//mn_Far_PluginInputThreadId      = pIn->dwData[1];
	//CheckColorMapping(mp_RCon->mn_FarPID_PluginDetected);
	// 23.06.2009 Maks - уберем пока. Должно работать в ApplyConsoleInfo
	//Process Add(mp_RCon->mn_FarPID_PluginDetected); // На всякий случай, вдруг он еще не в нашем списке?
	wchar_t* pszRes = (wchar_t*)(&(pIn->dwData[1])), *pszNext;

	if (*pszRes)
	{
		//EnableComSpec(mp_RCon->mn_FarPID_PluginDetected, TRUE);
		//mp_RCon->UpdateFarSettings(mp_RCon->mn_FarPID_PluginDetected);
		wchar_t* pszItems[] = {mp_RCon->ms_EditorRus,mp_RCon->ms_ViewerRus,mp_RCon->ms_TempPanelRus/*,ms_NameTitle*/};
		wchar_t* pszEnd = (wchar_t*)(pIn->Data + nDataSize);
		_ASSERTE(countof(mp_RCon->ms_EditorRus)>=32 && countof(mp_RCon->ms_ViewerRus)>=32 && countof(mp_RCon->ms_TempPanelRus));

		for (UINT i = 0; (i < countof(pszItems)) && (pszRes < pszEnd) && *pszRes; i++)
		{
			size_t nLen = _tcslen(pszRes);
			pszNext = pszRes + nLen + 1;

			if (nLen>=30)
				pszRes[30] = 0;

			lstrcpy(pszItems[i], pszRes);

			if (pszItems[i] != mp_RCon->ms_TempPanelRus)
				lstrcat(pszItems[i], L" ");

			pszRes = pszNext;
		}

		//pszNext = pszRes + _tcslen(pszRes)+1;
		//if (_tcslen(pszRes)>=30) pszRes[30] = 0;
		//lstrcpy(ms_EditorRus, pszRes); lstrcat(ms_EditorRus, L" ");
		//pszRes = pszNext;
		//if (*pszRes) {
		//    pszNext = pszRes + _tcslen(pszRes)+1;
		//    if (_tcslen(pszRes)>=30) pszRes[30] = 0;
		//    lstrcpy(ms_ViewerRus, pszRes); lstrcat(ms_ViewerRus, L" ");
		//    pszRes = pszNext;
		//    if (*pszRes) {
		//        pszNext = pszRes + _tcslen(pszRes)+1;
		//        if (_tcslen(pszRes)>=31) pszRes[31] = 0;
		//        lstrcpy(ms_TempPanelRus, pszRes);
		//        pszRes = pszNext;
		//    }
		//}
	}

	size_t cchRet = sizeof(CESERVER_REQ_HDR) + sizeof(FAR_REQ_FARSETCHANGED);
	pOut = ExecuteNewCmd(pIn->hdr.nCmd, cchRet);

	mp_RCon->UpdateFarSettings(mp_RCon->mn_FarPID_PluginDetected, &pOut->FarSetChanged);

	return pOut;
}

CESERVER_REQ* CRealServer::cmdSetForeground(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
{
	CESERVER_REQ* pOut = NULL;

	DEBUGSTRCMD(L"GUI recieved CECMD_SETFOREGROUNDWND\n");
	AllowSetForegroundWindow(pIn->hdr.nSrcPID);

	HWND hWnd = (HWND)pIn->qwData[0];
	DWORD nWndPID = 0; GetWindowThreadProcessId(hWnd, &nWndPID);
	if (nWndPID == GetCurrentProcessId())
	{
		// Если это один из hWndDC - поднимаем главное окно
		if (hWnd != ghWnd && GetParent(hWnd) == ghWnd)
			hWnd = ghWnd;
	}

	BOOL lbRc = apiSetForegroundWindow(hWnd);
	pOut = ExecuteNewCmd(pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR)+sizeof(DWORD));
	if (pOut)
		pOut->dwData[0] = lbRc;

	return pOut;
}

CESERVER_REQ* CRealServer::cmdFlashWindow(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
{
	CESERVER_REQ* pOut = NULL;

	DEBUGSTRCMD(L"GUI recieved CECMD_FLASHWINDOW\n");

	gpConEmu->DoFlashWindow(&pIn->Flash, false);

	pOut = ExecuteNewCmd(pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR));

	return pOut;
}

CESERVER_REQ* CRealServer::cmdRegPanelView(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
{
	CESERVER_REQ* pOut = NULL;

	DEBUGSTRCMD(L"GUI recieved CECMD_REGPANELVIEW\n");
	pOut = ExecuteNewCmd(pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR)+sizeof(pIn->PVI));
	pOut->PVI = pIn->PVI;

	if (pOut->PVI.cbSize != sizeof(pOut->PVI))
	{
		pOut->PVI.cbSize = 0; // ошибка версии?
	}
	else if (!mp_RCon->mp_VCon->RegisterPanelView(&(pOut->PVI)))
	{
		pOut->PVI.cbSize = 0; // ошибка
	}

	return pOut;
}

CESERVER_REQ* CRealServer::cmdSetBackground(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
{
	CESERVER_REQ* pOut = NULL;

	DEBUGSTRCMD(L"GUI recieved CECMD_SETBACKGROUND\n");
	pOut = ExecuteNewCmd(pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_SETBACKGROUNDRET));
	// Set background Image
	UINT nCalcSize = pIn->hdr.cbSize - sizeof(pIn->hdr);

	if (nCalcSize < sizeof(CESERVER_REQ_SETBACKGROUND))
	{
		_ASSERTE(nCalcSize >= sizeof(CESERVER_REQ_SETBACKGROUND));
		pOut->BackgroundRet.nResult = esbr_InvalidArg;
	}
	else
	{
		UINT nCalcBmSize = nCalcSize - (((LPBYTE)&pIn->Background.bmp) - ((LPBYTE)&pIn->Background));

		if (pIn->Background.bEnabled && nCalcSize < nCalcBmSize)
		{
			_ASSERTE(nCalcSize >= nCalcBmSize);
			pOut->BackgroundRet.nResult = esbr_InvalidArg;
		}
		else
		{
			pOut->BackgroundRet.nResult = mp_RCon->mp_VCon->SetBackgroundImageData(&pIn->Background);
		}
	}

	return pOut;
}

CESERVER_REQ* CRealServer::cmdActivateCon(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
{
	CESERVER_REQ* pOut = NULL;

	DEBUGSTRCMD(L"GUI recieved CECMD_ACTIVATECON\n");
	pOut = ExecuteNewCmd(pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_ACTIVATECONSOLE));
	// Activate current console
	_ASSERTE(mp_RCon->hConWnd == (HWND)pIn->ActivateCon.hConWnd);

	if (gpConEmu->Activate(mp_RCon->mp_VCon))
		pOut->ActivateCon.hConWnd = mp_RCon->hConWnd;
	else
		pOut->ActivateCon.hConWnd = NULL;

	return pOut;
}

CESERVER_REQ* CRealServer::cmdOnCreateProc(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
{
	CESERVER_REQ* pOut = NULL;

	DEBUGSTRCMD(L"GUI recieved CECMD_ONCREATEPROC\n");
	pOut = ExecuteNewCmd(pIn->hdr.nCmd,
		sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_ONCREATEPROCESSRET)
		/*+MAX_PATH*6*/);

	BOOL lbDos = (pIn->OnCreateProc.nImageBits == 16)
		&& (pIn->OnCreateProc.nImageSubsystem == IMAGE_SUBSYSTEM_DOS_EXECUTABLE);

	if (gpSetCls->GetPage(gpSetCls->thi_Debug))
	{
		DebugLogShellActivity *shl = (DebugLogShellActivity*)calloc(sizeof(DebugLogShellActivity),1);
		shl->nParentPID = pIn->hdr.nSrcPID;
		shl->nParentBits = pIn->OnCreateProc.nSourceBits;
		wcscpy_c(shl->szFunction, pIn->OnCreateProc.sFunction);
		shl->pszAction = lstrdup(pIn->OnCreateProc.wsValue);
		shl->pszFile   = lstrdup(pIn->OnCreateProc.wsValue+pIn->OnCreateProc.nActionLen);
		shl->pszParam  = lstrdup(pIn->OnCreateProc.wsValue+pIn->OnCreateProc.nActionLen+pIn->OnCreateProc.nFileLen);
		shl->bDos = lbDos;
		shl->nImageBits = pIn->OnCreateProc.nImageBits;
		shl->nImageSubsystem = pIn->OnCreateProc.nImageSubsystem;
		shl->nShellFlags = pIn->OnCreateProc.nShellFlags;
		shl->nCreateFlags = pIn->OnCreateProc.nCreateFlags;
		shl->nStartFlags = pIn->OnCreateProc.nStartFlags;
		shl->nShowCmd = pIn->OnCreateProc.nShowCmd;
		shl->hStdIn = (DWORD)pIn->OnCreateProc.hStdIn;
		shl->hStdOut = (DWORD)pIn->OnCreateProc.hStdOut;
		shl->hStdErr = (DWORD)pIn->OnCreateProc.hStdErr;

		PostMessage(gpSetCls->GetPage(gpSetCls->thi_Debug), DBGMSG_LOG_ID, DBGMSG_LOG_SHELL_MAGIC, (LPARAM)shl);
	}

	if (pIn->OnCreateProc.nImageBits > 0)
	{
		TODO("!!! DosBox allowed?");
		_ASSERTE(lbDos==FALSE || gpConEmu->mb_DosBoxExists); //WARNING("Зачем (lbDos && FALSE)?");

		if (gpSet->AutoBufferHeight // LongConsoleOutput
			|| (lbDos && FALSE)) // DosBox!!!
		{
			pOut->OnCreateProcRet.bContinue = TRUE;
			//pOut->OnCreateProcRet.bUnicode = TRUE;
			pOut->OnCreateProcRet.bForceBufferHeight = gpSet->AutoBufferHeight;

			TODO("!!! DosBox allowed?");
			pOut->OnCreateProcRet.bAllowDosbox = FALSE;
			//pOut->OnCreateProcRet.nFileLen = pIn->OnCreateProc.nFileLen;
			//pOut->OnCreateProcRet.nBaseLen = _tcslen(gpConEmu->ms_ConEmuBaseDir)+2; // +слеш+\0

			////_wcscpy_c(pOut->OnCreateProcRet.wsValue, MAX_PATH+1, pszFile);
			//_wcscpy_c(pOut->OnCreateProcRet.wsValue, MAX_PATH+1, gpConEmu->ms_ConEmuBaseDir);
			//_wcscat_c(pOut->OnCreateProcRet.wsValue, MAX_PATH+1, L"\\");
		}
	}

	return pOut;
}

//CESERVER_REQ* CRealServer::cmdNewConsole(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
//{
//	CESERVER_REQ* pOut = NULL;
//
//	DEBUGSTRCMD(L"GUI recieved CECMD_NEWCONSOLE\n");
//	pOut = ExecuteNewCmd(pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR)+sizeof(wchar_t));
//	pOut->wData[0] = 0;
//
//	return pOut;
//}

CESERVER_REQ* CRealServer::cmdOnPeekReadInput(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
{
	CESERVER_REQ* pOut = NULL;

	DEBUGSTRCMD(L"GUI recieved CECMD_PEEKREADINFO\n");

	if (gpSetCls->GetPage(gpSetCls->thi_Debug) && gpSetCls->m_ActivityLoggingType == glt_Input)
	{
		if (nDataSize >= sizeof(CESERVER_REQ_PEEKREADINFO))
		{
			CESERVER_REQ_PEEKREADINFO* pCopy = (CESERVER_REQ_PEEKREADINFO*)malloc(nDataSize);
			if (pCopy)
			{
				memmove(pCopy, &pIn->PeekReadInfo, nDataSize);
				PostMessage(gpSetCls->GetPage(gpSetCls->thi_Debug), DBGMSG_LOG_ID, DBGMSG_LOG_INPUT_MAGIC, (LPARAM)pCopy);
			}
		}
	}

	pOut = ExecuteNewCmd(pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR));
	return pOut;
}

CESERVER_REQ* CRealServer::cmdOnSetConsoleKeyShortcuts(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
{
	CESERVER_REQ* pOut = NULL;

	DEBUGSTRCMD(L"GUI recieved CECMD_KEYSHORTCUTS\n");

	mp_RCon->m_ConsoleKeyShortcuts = pIn->Data[0] ? pIn->Data[1] : 0;
	gpConEmu->UpdateWinHookSettings();

	pOut = ExecuteNewCmd(pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR));
	return pOut;
}

CESERVER_REQ* CRealServer::cmdLockDc(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
{
	CESERVER_REQ* pOut = NULL;

	DEBUGSTRCMD(L"GUI recieved CECMD_LOCKDC\n");

	_ASSERTE(pIn->LockDc.hDcWnd == mp_RCon->mp_VCon->GetView());

	mp_RCon->mp_VCon->LockDcRect(pIn->LockDc.bLock, &pIn->LockDc.Rect);

	pOut = ExecuteNewCmd(pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR));
	return pOut;
}

CESERVER_REQ* CRealServer::cmdGetAllTabs(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
{
	CESERVER_REQ* pOut = NULL;

	DEBUGSTRCMD(L"GUI recieved CECMD_GETALLTABS\n");

	CESERVER_REQ_GETALLTABS::TabInfo* pTabs = NULL;
	size_t cchCount = CConEmuCtrl::GetOpenedTabs(pTabs);

	if (cchCount && pTabs)
	{
		size_t RetSize = sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_GETALLTABS)+((cchCount-1)*sizeof(CESERVER_REQ_GETALLTABS::TabInfo));
		pOut = ExecuteNewCmd(pIn->hdr.nCmd, RetSize);
		if (pOut)
		{
			pOut->GetAllTabs.Count = cchCount;
			memmove(pOut->GetAllTabs.Tabs, pTabs, cchCount*sizeof(*pTabs));
		}
	}
	else
	{
		pOut = ExecuteNewCmd(pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_GETALLTABS));
	}
	SafeFree(pTabs);

	return pOut;
}

CESERVER_REQ* CRealServer::cmdGetAllPanels(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
{
	CESERVER_REQ* pOut = NULL;

	DEBUGSTRCMD(L"GUI recieved CECMD_GETALLPANELS\n");

	wchar_t* pszDirs = NULL;
	int iCount = 0, iCurrent = 0;
	size_t cchSize = CConEmuCtrl::GetOpenedPanels(pszDirs, iCount, iCurrent);

	if (cchSize && pszDirs)
	{
		size_t RetSize = sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_GETALLPANELS)+(cchSize*sizeof(*pszDirs));
		pOut = ExecuteNewCmd(pIn->hdr.nCmd, RetSize);
		if (pOut)
		{
			pOut->Panels.iCount = iCount;
			pOut->Panels.iCurrent = iCurrent;
			memmove(pOut->Panels.szDirs, pszDirs, cchSize*sizeof(*pszDirs));
		}
	}
	else
	{
		pOut = ExecuteNewCmd(pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_GETALLPANELS));
	}

	SafeFree(pszDirs);

	return pOut;
}

CESERVER_REQ* CRealServer::cmdActivateTab(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
{
	CESERVER_REQ* pOut = NULL;

	DEBUGSTRCMD(L"GUI recieved CECMD_ACTIVATETAB\n");

	BOOL lbTabOk = FALSE;
	if (nDataSize >= 2*sizeof(DWORD))
	{
		CVConGuard VCon;
		if (CVConGroup::GetVCon(pIn->dwData[0], &VCon) && VCon->RCon())
		{
			// Latest Far3 has broken tab-indexes/far-windows behavior
			CTab tab(__FILE__,__LINE__);
			if (VCon->RCon()->GetTab(pIn->dwData[1], tab))
				lbTabOk = VCon->RCon()->ActivateFarWindow(tab->Info.nFarWindowID);
		}
		gpConEmu->ConActivate(pIn->dwData[0]);
	}

	pOut = ExecuteNewCmd(pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR)+sizeof(DWORD));
	if (pOut)
		pOut->dwData[0] = lbTabOk;
	return pOut;
}

CESERVER_REQ* CRealServer::cmdRenameTab(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
{
	CESERVER_REQ* pOut = NULL;

	DEBUGSTRCMD(L"GUI recieved CECMD_SETTABTITLE\n");

	LPCWSTR pszNewText = NULL;
	if (nDataSize >= 2*sizeof(wchar_t))
		pszNewText = (wchar_t*)pIn->wData;

	mp_RCon->RenameTab(pszNewText);

	pOut = ExecuteNewCmd(pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR)+sizeof(DWORD));
	if (pOut)
		pOut->dwData[0] = TRUE;
	return pOut;
}

CESERVER_REQ* CRealServer::cmdSetProgress(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
{
	CESERVER_REQ* pOut = NULL;

	DEBUGSTRCMD(L"GUI recieved CECMD_SETPROGRESS\n");

	bool lbOk = false;
	if (nDataSize >= 2*sizeof(pIn->wData[0]))
	{
		LPCWSTR pszName = (nDataSize >= 4*sizeof(pIn->wData[0])) ? (LPCWSTR)(pIn->wData+2) : NULL;

		mp_RCon->SetProgress(pIn->wData[0], pIn->wData[1], pszName);
	}

	pOut = ExecuteNewCmd(pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR)+sizeof(DWORD));
	if (pOut)
		pOut->dwData[0] = lbOk;
	return pOut;
}

//CESERVER_REQ* CRealServer::cmdAssert(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
//{
//	CESERVER_REQ* pOut = NULL;
//
//	DEBUGSTRCMD(L"GUI recieved CECMD_ASSERT\n");
//	pOut = ExecuteNewCmd(pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR)+sizeof(wchar_t));
//	pOut->wData[0] = MessageBox(NULL, pIn->AssertInfo.szDebugInfo, pIn->AssertInfo.szTitle, MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_RETRYCANCEL);
//
//	return pOut;
//}

CESERVER_REQ* CRealServer::cmdExportEnvVarAll(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
{
	DWORD nCmd = pIn->hdr.nCmd;
	DEBUGSTRCMD((nCmd==CECMD_EXPORTVARSALL) ? L"GUI recieved CECMD_EXPORTVARSALL\n" : L"GUI recieved CECMD_EXPORTVARS\n");

	// В свой процесс тоже засосать переменные, чтобы для новых табов применялись
	LPCWSTR pszSrc = (LPCWSTR)pIn->wData;
	ApplyExportEnvVar(pszSrc);

	// Применить переменные во всех открытых табах (кроме mp_RCon)
	if (nCmd == CECMD_EXPORTVARSALL)
	{
		CVConGroup::ExportEnvVarAll(pIn, mp_RCon);
	}

	// pIn->hdr.nCmd перебивается на CECMD_EXPORTVARS, поэтому возвращаем сохраненный ID
	CESERVER_REQ* pOut = ExecuteNewCmd(nCmd, sizeof(CESERVER_REQ_HDR)+sizeof(DWORD));
	if (pOut)
		pOut->dwData[0] = TRUE;
	return pOut;
}

CESERVER_REQ* CRealServer::cmdStartXTerm(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
{
	BOOL  bProcessed = TRUE;
	DWORD nCmd = pIn->hdr.nCmd;
	DEBUGSTRCMD(L"GUI recieved CECMD_STARTXTERM\n");

	switch (pIn->dwData[0])
	{
	case tmc_Keyboard:
		_ASSERTE(pIn->dwData[1] == te_win32 || pIn->dwData[1] == te_xterm);
		mp_RCon->StartStopXTerm(pIn->hdr.nSrcPID, (pIn->dwData[1] != te_win32));
		break;
	case tmc_BracketedPaste:
		mp_RCon->StartStopBracketedPaste(pIn->hdr.nSrcPID, (pIn->dwData[1] != 0));
		break;
	case tmc_AppCursorKeys:
		mp_RCon->StartStopAppCursorKeys(pIn->hdr.nSrcPID, (pIn->dwData[1] != 0));
		break;
	default:
		bProcessed = FALSE;
	}

	CESERVER_REQ* pOut = ExecuteNewCmd(nCmd, sizeof(CESERVER_REQ_HDR)+sizeof(DWORD));
	if (pOut)
		pOut->dwData[0] = bProcessed;
	return pOut;
}

CESERVER_REQ* CRealServer::cmdPortableStarted(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
{
	DWORD nCmd = pIn->hdr.nCmd;
	DEBUGSTRCMD(L"GUI recieved CECMD_PORTABLESTART\n");

	// В свой процесс тоже засосать переменные, чтобы для новых табов применялись
	mp_RCon->PortableStarted(&pIn->PortableStarted);

	CESERVER_REQ* pOut = ExecuteNewCmd(nCmd, sizeof(CESERVER_REQ_HDR));
	return pOut;
}

CESERVER_REQ* CRealServer::cmdQueryPalette(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
{
	COLORREF* pcrColors = mp_RCon->VCon()->GetColors(false);
	COLORREF* pcrFadeColors = mp_RCon->VCon()->GetColors(true);
	CESERVER_REQ* pOut = NULL;
	if (pcrColors)
	{
		if ((pOut = ExecuteNewCmd(pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_PALETTE))) != NULL)
		{
			_ASSERTE(sizeof(pOut->Palette.crPalette) == sizeof(COLORREF)*16);
			memmove(pOut->Palette.crPalette, pcrColors, sizeof(pOut->Palette.crPalette));

			_ASSERTE(sizeof(pOut->Palette.crFadePalette) == sizeof(COLORREF)*16);
			memmove(pOut->Palette.crFadePalette, pcrFadeColors, sizeof(pOut->Palette.crFadePalette));
		}
	}
	return pOut;
}

CESERVER_REQ* CRealServer::cmdGetTaskCmd(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize)
{
	CEStr lsData;
	const CommandTasks* pTask = (pIn->DataSize() > sizeof(pIn->GetTask)) ? gpSet->CmdTaskGetByName(pIn->GetTask.data) : NULL;
	if (pTask)
	{
		LPCWSTR pszTemp = pTask->pszCommands;
		if (0 == NextLine(&pszTemp, lsData))
		{
			RConStartArgs args;
			LPCWSTR pszRaw = gpConEmu->ParseScriptLineOptions(lsData.ms_Val, NULL, NULL);
			if (pszRaw)
			{
				args.pszSpecialCmd = lstrdup(pszRaw);
				// Parse all -new_console's
				args.ProcessNewConArg();
				// Prohbit external requests for credentials
				args.CleanSecure();
				// Directory?
				if (!args.pszStartupDir && pTask->pszGuiArgs)
					pTask->ParseGuiArgs(&args);
				// Prepare for execution
				lsData = args.CreateCommandLine(false);
			}
		}
	}

	int nLen = lsData.GetLen();

	CESERVER_REQ* pOut = ExecuteNewCmd(pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_TASK) + nLen*sizeof(wchar_t));
	if (!pOut)
		return NULL;

	if (nLen > 0)
	{
		pOut->GetTask.nIdx = TRUE;
		lstrcpy(pOut->GetTask.data, lsData.ms_Val);
	}
	else
	{
		pOut->GetTask.nIdx = FALSE;
		pOut->GetTask.data[0] = 0;
	}

	return pOut;
}

// Эта функция пайп не закрывает!
//void CRealServer::ServerThreadCommand(HANDLE hPipe)
BOOL CRealServer::ServerCommand(LPVOID pInst, CESERVER_REQ* pIn, CESERVER_REQ* &ppReply, DWORD &pcbReplySize, DWORD &pcbMaxReplySize, LPARAM lParam)
{
	BOOL lbRc = FALSE;
	CRealServer* pRSrv = (CRealServer*)lParam;
	CRealConsole* pRCon = (CRealConsole*)pRSrv->mp_RCon;

	ExecuteFreeResult(ppReply);
	CESERVER_REQ *pOut = NULL;

	if (pIn->hdr.nVersion != CESERVER_REQ_VER)
	{
		gpConEmu->ReportOldCmdVersion(pIn->hdr.nCmd, pIn->hdr.nVersion, pIn->hdr.nSrcPID==pRCon->GetServerPID() ? 1 : 0,
			pIn->hdr.nSrcPID, pIn->hdr.hModule, pIn->hdr.nBits);
		return FALSE;
	}

	if (pIn->hdr.cbSize < sizeof(CESERVER_REQ_HDR))
	{
		_ASSERTE(pIn->hdr.cbSize>=sizeof(CESERVER_REQ_HDR));
		//CloseHandle(hPipe);
		return FALSE;
	}

	if (pIn->hdr.bAsync)
		pRSrv->mp_RConServer->BreakConnection(pInst);

	DWORD dwTimeStart = timeGetTime();

	int nDataSize = pIn->DataSize();

	// Все данные из пайпа получены, обрабатываем команду и возвращаем (если нужно) результат

	switch (pIn->hdr.nCmd)
	{
	case CECMD_CMDSTARTSTOP:
		pOut = pRSrv->cmdStartStop(pInst, pIn, nDataSize);
		break;
	//else if (pIn->hdr.nCmd == CECMD_GETGUIHWND)
	//	pOut = pRSrv->cmdGetGuiHwnd(pInst, pIn, nDataSize);
	case CECMD_TABSCHANGED:
		pOut = pRSrv->cmdTabsChanged(pInst, pIn, nDataSize);
		break;
	case CECMD_GETOUTPUTFILE:
		pOut = pRSrv->cmdGetOutputFile(pInst, pIn, nDataSize);
		break;
	case CECMD_GUIMACRO:
		pOut = pRSrv->cmdGuiMacro(pInst, pIn, nDataSize);
		break;
	case CECMD_LANGCHANGE:
		pOut = pRSrv->cmdLangChange(pInst, pIn, nDataSize);
		break;
	case CECMD_TABSCMD:
		pOut = pRSrv->cmdTabsCmd(pInst, pIn, nDataSize);
		break;
	case CECMD_RESOURCES:
		pOut = pRSrv->cmdResources(pInst, pIn, nDataSize);
		break;
	case CECMD_SETFOREGROUND:
		pOut = pRSrv->cmdSetForeground(pInst, pIn, nDataSize);
		break;
	case CECMD_FLASHWINDOW:
		pOut = pRSrv->cmdFlashWindow(pInst, pIn, nDataSize);
		break;
	case CECMD_REGPANELVIEW:
		pOut = pRSrv->cmdRegPanelView(pInst, pIn, nDataSize);
		break;
	case CECMD_SETBACKGROUND:
		pOut = pRSrv->cmdSetBackground(pInst, pIn, nDataSize);
		break;
	case CECMD_ACTIVATECON:
		pOut = pRSrv->cmdActivateCon(pInst, pIn, nDataSize);
		break;
	case CECMD_ONCREATEPROC:
		pOut = pRSrv->cmdOnCreateProc(pInst, pIn, nDataSize);
		break;
	//else if (pIn->hdr.nCmd == CECMD_NEWCONSOLE)
	//	pOut = pRSrv->cmdNewConsole(pInst, pIn, nDataSize);
	case CECMD_PEEKREADINFO:
		pOut = pRSrv->cmdOnPeekReadInput(pInst, pIn, nDataSize);
		break;
	case CECMD_KEYSHORTCUTS:
		pOut = pRSrv->cmdOnSetConsoleKeyShortcuts(pInst, pIn, nDataSize);
		break;
	case CECMD_ALIVE:
		pOut = ExecuteNewCmd(CECMD_ALIVE, sizeof(CESERVER_REQ_HDR));
		break;
	//else if (pIn->hdr.nCmd == CECMD_ASSERT)
	case CECMD_LOCKDC:
		pOut = pRSrv->cmdLockDc(pInst, pIn, nDataSize);
		break;
	case CECMD_GETALLTABS:
		pOut = pRSrv->cmdGetAllTabs(pInst, pIn, nDataSize);
		break;
	case CECMD_GETALLPANELS:
		pOut = pRSrv->cmdGetAllPanels(pInst, pIn, nDataSize);
		break;
	case CECMD_ACTIVATETAB:
		pOut = pRSrv->cmdActivateTab(pInst, pIn, nDataSize);
		break;
	case CECMD_SETTABTITLE:
		pOut = pRSrv->cmdRenameTab(pInst, pIn, nDataSize);
		break;
	case CECMD_SETPROGRESS:
		pOut = pRSrv->cmdSetProgress(pInst, pIn, nDataSize);
		break;
	case CECMD_EXPORTVARS:
	case CECMD_EXPORTVARSALL:
		pOut = pRSrv->cmdExportEnvVarAll(pInst, pIn, nDataSize);
		break;
	case CECMD_STARTXTERM:
		pOut = pRSrv->cmdStartXTerm(pInst, pIn, nDataSize);
		break;
	case CECMD_PORTABLESTART:
		pOut = pRSrv->cmdPortableStarted(pInst, pIn, nDataSize);
		break;
	case CECMD_STORECURDIR:
		pRSrv->mp_RCon->StoreCurWorkDir(&pIn->CurDir);
		pOut = (CESERVER_REQ*)INVALID_HANDLE_VALUE;
		break;
	case CECMD_QUERYPALETTE:
		pOut = pRSrv->cmdQueryPalette(pInst, pIn, nDataSize);
		break;
	case CECMD_GETTASKCMD:
		pOut = pRSrv->cmdGetTaskCmd(pInst, pIn, nDataSize);
		break;
	case CECMD_GETROOTINFO:
		_ASSERTE(!pIn->RootInfo.bRunning && pIn->RootInfo.nPID);
		pRSrv->mp_RCon->UpdateRootInfo(pIn->RootInfo);
		pOut = ExecuteNewCmd(CECMD_GETROOTINFO, sizeof(CESERVER_REQ_HDR));
		break;
	//else if (pIn->hdr.nCmd == CECMD_ASSERT)
	//	pOut = cmdAssert(pInst, pIn, nDataSize);
	default:
		// Неизвестная команда
		_ASSERTE(FALSE && "Unsupported command pIn->hdr.nCmd");

		// Хотя бы "пустую" команду в ответ кинуть, а то ошибка (Pipe was closed) у клиента возникает
		// 0 - чтобы assert-ами ловить необработанные команды
		pOut = ExecuteNewCmd(0/*pIn->hdr.nCmd*/, sizeof(CESERVER_REQ_HDR));
	}



	if (pOut != (CESERVER_REQ*)INVALID_HANDLE_VALUE)
	{
		if (pOut == NULL)
		{
			// Для четкости, методы должны сами возвращать реальный результат
			_ASSERTE(pOut!=NULL);
			// Хотя бы "пустую" команду в ответ кинуть, а то ошибка (Pipe was closed) у клиента возникает
			pOut = ExecuteNewCmd(pIn->hdr.nCmd, sizeof(CESERVER_REQ_HDR));
		}

		DWORD dwDur = timeGetTime() - dwTimeStart;
		gpSetCls->debugLogCommand(pIn, TRUE, dwTimeStart, dwDur, pRCon->ms_VConServer_Pipe, pOut);

		ppReply = pOut;
		if (pOut)
		{
			pcbReplySize = pOut->hdr.cbSize;
			lbRc = TRUE;
		}

		//// Всегда чего-нибудь ответить в пайп, а то ошибка (Pipe was closed) у клиента возникает
		//fSuccess = WriteFile(hPipe, pOut, pOut->hdr.cbSize, &cbWritten, NULL);
		//ExecuteFreeResult(pOut);
	}
	else
	{
		DWORD dwDur = timeGetTime() - dwTimeStart;
		gpSetCls->debugLogCommand(pIn, TRUE, dwTimeStart, dwDur, pRCon->ms_VConServer_Pipe, NULL/*pOut*/);

		// Delayed write
		_ASSERTE(ppReply==NULL);
		ppReply = NULL;
		lbRc = TRUE;
	}


	//// Освободить память
	//if (pIn && (LPVOID)pIn != (LPVOID)&in)
	//{
	//	free(pIn); pIn = NULL;
	//}

	MCHKHEAP;
	//CloseHandle(hPipe);
	return lbRc;
}

BOOL CRealServer::ServerThreadReady(LPVOID pInst, LPARAM lParam)
{
	CRealServer* pRSrv = (CRealServer*)lParam;

	// Чтобы ConEmuC знал, что серверный пайп готов
	if (pRSrv && pRSrv->mh_GuiAttached)
	{
		SetEvent(pRSrv->mh_GuiAttached);
		SafeCloseHandle(pRSrv->mh_GuiAttached);
	}
	else
	{
		_ASSERTE(pRSrv && pRSrv->mh_GuiAttached);
	}

	return TRUE;
}

void CRealServer::ServerCommandFree(CESERVER_REQ* pReply, LPARAM lParam)
{
	ExecuteFreeResult(pReply);
}
