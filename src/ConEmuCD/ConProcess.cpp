
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

#define SHOWDEBUGSTR
#define DEBUGSTRPROC(x) DEBUGSTR(x)

#include "ConEmuC.h"
#include "ConProcess.h"
#include "../common/MSection.h"
#include "../common/ProcessData.h"

BOOL   gbUseDosBox = FALSE;
HANDLE ghDosBoxProcess = NULL;
DWORD  gnDosBoxPID = 0;

void ProcessCountChanged(BOOL abChanged, UINT anPrevCount, MSectionLock *pCS)
{
	int nExitPlaceAdd = 2; // 2,3,4,5,6,7,8,9 +(nExitPlaceStep)
	bool bPrevCount2 = (anPrevCount>1);

	wchar_t szCountInfo[256];
	if (abChanged && RELEASEDEBUGTEST((gpLogSize!=NULL),true))
	{
		DWORD nPID, nLastPID = 0, nFoundPID = 0;
		_wsprintf(szCountInfo, SKIPCOUNT(szCountInfo) L"Process list was changed: %u -> %i", anPrevCount, gpSrv ? gpSrv->nProcessCount : -1);

		wcscat_c(szCountInfo, L"\r\n                        Processes:");
		INT_PTR iLen = lstrlen(szCountInfo);
		wchar_t *psz = (szCountInfo + iLen), *pszEnd = szCountInfo + (countof(szCountInfo) - iLen - 64);
		for (size_t i = 0; (i < gpSrv->nProcessCount) && (psz < pszEnd); i++)
		{
			nPID = gpSrv->pnProcesses[i];
			if (!nPID) continue;
			_wsprintf(psz, SKIPLEN(12) L" %u", nPID);
			psz += lstrlen(psz);
			nLastPID = nPID;
			if (nPID == gpSrv->nLastFoundPID)
				nFoundPID = nPID;
		}

		if (nFoundPID || nLastPID)
		{
			nPID = nFoundPID ? nFoundPID : nLastPID;
			msprintf(psz, (psz - szCountInfo), L"\r\n                        ActivePID: %u, Name: ", nPID);
			psz += lstrlen(psz);
			CProcessData procName;
			procName.GetProcessName(nPID, psz, (psz - szCountInfo), NULL, 0, NULL);
		}

		LogString(szCountInfo);
	}


	// Use section, if was not locked before
	MSectionLock CS;
	if (!pCS)
		CS.Lock(gpSrv->csProc);

#ifdef USE_COMMIT_EVENT
	// Если кто-то регистрировался как "ExtendedConsole.dll"
	// то проверить, а не свалился ли процесс его вызвавший
	if (gpSrv && gpSrv->nExtConsolePID)
	{
		DWORD nExtPID = gpSrv->nExtConsolePID;
		bool bExist = false;
		for (UINT i = 0; i < gpSrv->nProcessCount; i++)
		{
			if (gpSrv->pnProcesses[i] == nExtPID)
			{
				bExist = true; break;
			}
		}
		if (!bExist)
		{
			if (gpSrv->hExtConsoleCommit)
			{
				CloseHandle(gpSrv->hExtConsoleCommit);
				gpSrv->hExtConsoleCommit = NULL;
			}
			gpSrv->nExtConsolePID = 0;
		}
	}
#endif

#ifndef WIN64
	// Найти "ntvdm.exe"
	if (abChanged && !gpSrv->nNtvdmPID && !IsWindows64())
	{
		//BOOL lbFarExists = FALSE, lbTelnetExist = FALSE;

		if (gpSrv->nProcessCount > 1)
		{
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
								//if (IsFarExe(prc.szExeFile))
								//{
								//	lbFarExists = TRUE;
								//	//if (gpSrv->nProcessCount <= 2) // нужно проверить и ntvdm
								//	//	break; // возможно, в консоли еще есть и telnet?
								//}

								//#ifndef WIN64
								//else
								if (!gpSrv->nNtvdmPID && lstrcmpiW(prc.szExeFile, L"ntvdm.exe")==0)
								{
									gpSrv->nNtvdmPID = prc.th32ProcessID;
									break;
								}
								//#endif

								//// 23.04.2010 Maks - telnet нужно определять, т.к. у него проблемы с Ins и курсором
								//else if (lstrcmpiW(prc.szExeFile, L"telnet.exe")==0)
								//{
								//	lbTelnetExist = TRUE;
								//}
							}
						}

						//if (lbFarExists && lbTelnetExist
						//	#ifndef WIN64
						//        && gpSrv->nNtvdmPID
						//	#endif
						//    )
						//{
						//	break; // чтобы условие выхода внятнее было
						//}
					}
					while (Process32Next(hSnap, &prc));
				}

				CloseHandle(hSnap);
			}
		}

		//gpSrv->bTelnetActive = lbTelnetExist;
	}
#endif

	gpSrv->dwProcessLastCheckTick = GetTickCount();

	// Если корневой процесс проработал достаточно (10 сек), значит он живой и gbAlwaysConfirmExit можно сбросить
	// Если gbAutoDisableConfirmExit==FALSE - сброс подтверждение закрытия консоли не выполняется
	if (gbAlwaysConfirmExit  // если уже не сброшен
	        && gbAutoDisableConfirmExit // требуется ли вообще такая проверка (10 сек)
	        && anPrevCount > 1 // если в консоли был зафиксирован запущенный процесс
	        && gpSrv->hRootProcess) // и корневой процесс был вообще запущен
	{
		if ((gpSrv->dwProcessLastCheckTick - gpSrv->nProcessStartTick) > CHECK_ROOTOK_TIMEOUT)
		{
			_ASSERTE(gnConfirmExitParm==0);
			// эта проверка выполняется один раз
			gbAutoDisableConfirmExit = FALSE;
			// 10 сек. прошло, теперь необходимо проверить, а жив ли процесс?
			DWORD dwProcWait = WaitForSingleObject(gpSrv->hRootProcess, 0);

			if (dwProcWait == WAIT_OBJECT_0)
			{
				// Корневой процесс завершен, возможно, была какая-то проблема?
				gbRootAliveLess10sec = TRUE; // корневой процесс проработал менее 10 сек
			}
			else
			{
				// Корневой процесс все еще работает, считаем что все ок и подтверждения закрытия консоли не потребуется
				DisableAutoConfirmExit();
				//gpSrv->nProcessStartTick = GetTickCount() - 2*CHECK_ROOTSTART_TIMEOUT; // менять nProcessStartTick не нужно. проверка только по флажкам
			}
		}
	}

	if (gbRootWasFoundInCon == 0 && gpSrv->nProcessCount > 1 && gpSrv->hRootProcess && gpSrv->dwRootProcess)
	{
		if (WaitForSingleObject(gpSrv->hRootProcess, 0) == WAIT_OBJECT_0)
		{
			gbRootWasFoundInCon = 2; // в консоль процесс не попал, и уже завершился
		}
		else
		{
			for (UINT n = 0; n < gpSrv->nProcessCount; n++)
			{
				if (gpSrv->dwRootProcess == gpSrv->pnProcesses[n])
				{
					// Процесс попал в консоль
					gbRootWasFoundInCon = 1; break;
				}
			}
		}
	}

	// Только для x86. На x64 ntvdm.exe не бывает.
	#ifndef WIN64
	WARNING("gpSrv->bNtvdmActive нигде не устанавливается");
	if (gpSrv->nProcessCount == 2 && !gpSrv->bNtvdmActive && gpSrv->nNtvdmPID)
	{
		// Возможно было запущено 16битное приложение, а ntvdm.exe не выгружается при его закрытии
		// gnSelfPID не обязательно будет в gpSrv->pnProcesses[0]
		if ((gpSrv->pnProcesses[0] == gnSelfPID && gpSrv->pnProcesses[1] == gpSrv->nNtvdmPID)
		        || (gpSrv->pnProcesses[1] == gnSelfPID && gpSrv->pnProcesses[0] == gpSrv->nNtvdmPID))
		{
			// Послать в нашу консоль команду закрытия
			PostMessage(ghConWnd, WM_CLOSE, 0, 0);
		}
	}
	#endif

	WARNING("Если в консоли ДО этого были процессы - все условия вида 'gpSrv->nProcessCount == 1' обломаются");

	bool bForcedTo2 = false;
	DWORD nWaitDbg1 = -1, nWaitDbg2 = -1;
	// Похоже "пример" не соответствует условию, оставлю пока, для истории
	// -- Пример - запускаемся из фара. Количество процессов ИЗНАЧАЛЬНО - 5
	// -- cmd вываливается сразу (path not found)
	// -- количество процессов ОСТАЕТСЯ 5 и ни одно из ниже условий не проходит
	if (anPrevCount == 1 && gpSrv->nProcessCount == 1
		&& gpSrv->nProcessStartTick && gpSrv->dwProcessLastCheckTick
		&& ((gpSrv->dwProcessLastCheckTick - gpSrv->nProcessStartTick) > CHECK_ROOTSTART_TIMEOUT)
		&& (nWaitDbg1 = WaitForSingleObject(ghExitQueryEvent,0)) == WAIT_TIMEOUT
		// выходить можно только если корневой процесс завершился
		&& gpSrv->hRootProcess && ((nWaitDbg2 = WaitForSingleObject(gpSrv->hRootProcess,0)) != WAIT_TIMEOUT))
	{
		anPrevCount = 2; // чтобы сработало следующее условие
		bForcedTo2 = true;
		//2010-03-06 - установка флажка должна быть при старте сервера
		//if (!gbAlwaysConfirmExit) gbAlwaysConfirmExit = TRUE; // чтобы консоль не схлопнулась
	}

	if (anPrevCount > 1 && gpSrv->nProcessCount == 1)
	{
		if (gpSrv->pnProcesses[0] != gnSelfPID)
		{
			_ASSERTE(gpSrv->pnProcesses[0] == gnSelfPID);
		}
		else
		{
			#ifdef DEBUG_ISSUE_623
			_ASSERTE(FALSE && "Calling SetTerminateEvent");
			#endif

			// !!! Во время сильной загрузки процессора периодически
			// !!! случается, что ConEmu отваливается быстрее, чем в
			// !!! консоли появится фар. Обратить внимание на nPrevProcessedDbg[]
			// !!! в вызывающей функции. Откуда там появилось 2 процесса,
			// !!! и какого фига теперь стал только 1?
			_ASSERTE(gbTerminateOnCtrlBreak==FALSE);
			// !!! ****


			if (pCS)
				pCS->Unlock();
			else
				CS.Unlock();

			//2010-03-06 это не нужно, проверки делаются по другому
			//if (!gbAlwaysConfirmExit && (gpSrv->dwProcessLastCheckTick - gpSrv->nProcessStartTick) <= CHECK_ROOTSTART_TIMEOUT) {
			//	gbAlwaysConfirmExit = TRUE; // чтобы консоль не схлопнулась
			//}
			if (gbAlwaysConfirmExit && (gpSrv->dwProcessLastCheckTick - gpSrv->nProcessStartTick) <= CHECK_ROOTSTART_TIMEOUT)
				gbRootAliveLess10sec = TRUE; // корневой процесс проработал менее 10 сек

			if (bForcedTo2)
				nExitPlaceAdd = bPrevCount2 ? 3 : 4;
			else if (bPrevCount2)
				nExitPlaceAdd = 5;

			if (!nExitQueryPlace) nExitQueryPlace = nExitPlaceAdd/*2,3,4,5,6,7,8,9*/+(nExitPlaceStep);

			ShutdownSrvStep(L"All processes are terminated, SetEvent(ghExitQueryEvent)");
			SetTerminateEvent(ste_ProcessCountChanged);
		}
	}
	else if (abChanged && gpSrv->pConsole)
	{
		gpSrv->pConsole->bDataChanged = TRUE;
		SetEvent(gpSrv->hRefreshEvent);
	}

	UNREFERENCED_PARAMETER(nWaitDbg1); UNREFERENCED_PARAMETER(nWaitDbg2); UNREFERENCED_PARAMETER(bForcedTo2);
}

BOOL ProcessAdd(DWORD nPID, MSectionLock *pCS /*= NULL*/)
{
	MSectionLock CS;
	if ((pCS == NULL) && (gpSrv->csProc != NULL))
	{
		pCS = &CS;
		CS.Lock(gpSrv->csProc);
	}

	UINT nPrevCount = gpSrv->nProcessCount;
	BOOL lbChanged = FALSE;
	_ASSERTE(nPID!=0);

	// Добавить процесс в список
	_ASSERTE(gpSrv->pnProcesses[0] == gnSelfPID);
	BOOL lbFound = FALSE;
	for (DWORD n = 0; n < nPrevCount; n++)
	{
		if (gpSrv->pnProcesses[n] == nPID)
		{
			lbFound = TRUE;
			break;
		}
	}
	if (!lbFound)
	{
		if (nPrevCount < gpSrv->nMaxProcesses)
		{
			pCS->RelockExclusive(200);
			gpSrv->pnProcesses[gpSrv->nProcessCount++] = nPID;
			gpSrv->nLastFoundPID = nPID;
			lbChanged = TRUE;
		}
		else
		{
			_ASSERTE(nPrevCount < gpSrv->nMaxProcesses);
		}
	}

	return lbChanged;
}

BOOL ProcessRemove(DWORD nPID, UINT nPrevCount, MSectionLock *pCS /*= NULL*/)
{
	BOOL lbChanged = FALSE;

	MSectionLock CS;
	if ((pCS == NULL) && (gpSrv->csProc != NULL))
	{
		pCS = &CS;
		CS.Lock(gpSrv->csProc);
	}

	// Удалить процесс из списка
	_ASSERTE(gpSrv->pnProcesses[0] == gnSelfPID);
	DWORD nChange = 0;
	for (DWORD n = 0; n < nPrevCount; n++)
	{
		if (gpSrv->pnProcesses[n] == nPID)
		{
			pCS->RelockExclusive(200);
			lbChanged = TRUE;
			if (gpSrv->nLastFoundPID == nPID)
				gpSrv->nLastFoundPID = 0;
			gpSrv->nProcessCount--;
			continue;
		}
		if (lbChanged)
		{
			gpSrv->pnProcesses[nChange] = gpSrv->pnProcesses[n];
		}
		nChange++;
	}

	return lbChanged;
}

#ifdef _DEBUG
void DumpProcInfo(LPCWSTR sLabel, DWORD nCount, DWORD* pPID)
{
#ifdef WINE_PRINT_PROC_INFO
	DWORD nErr = GetLastError();
	wchar_t szDbgMsg[255];
	msprintf(szDbgMsg, countof(szDbgMsg), L"%s: Err=%u, Count=%u:", sLabel ? sLabel : L"", nErr, nCount);
	nCount = min(nCount,10);
	for (DWORD i = 0; pPID && (i < nCount); i++)
	{
		int nLen = lstrlen(szDbgMsg);
		msprintf(szDbgMsg+nLen, countof(szDbgMsg)-nLen, L" %u", pPID[i]);
	}
	wcscat_c(szDbgMsg, L"\r\n");
	_wprintf(szDbgMsg);
#endif
}
#define DUMP_PROC_INFO(s,n,p) //DumpProcInfo(s,n,p)
#else
#define DUMP_PROC_INFO(s,n,p)
#endif

BOOL CheckProcessCount(BOOL abForce/*=FALSE*/)
{
	//static DWORD dwLastCheckTick = GetTickCount();
	UINT nPrevCount = gpSrv->nProcessCount;
#ifdef _DEBUG
	DWORD nCurProcessesDbg[128] = {}; // для отладки, получение текущего состояния консоли
	DWORD nPrevProcessedDbg[128] = {}; // для отладки, запомнить предыдущее состояние консоли
	if (gpSrv->pnProcesses && gpSrv->nProcessCount)
		memmove(nPrevProcessedDbg, gpSrv->pnProcesses, min(countof(nPrevProcessedDbg),gpSrv->nProcessCount)*sizeof(*gpSrv->pnProcesses));
#endif

	if (gpSrv->nProcessCount <= 0)
	{
		abForce = TRUE;
	}

	if (!abForce)
	{
		DWORD dwCurTick = GetTickCount();

		if ((dwCurTick - gpSrv->dwProcessLastCheckTick) < (DWORD)CHECK_PROCESSES_TIMEOUT)
			return FALSE;
	}

	BOOL lbChanged = FALSE;
	MSectionLock CS; CS.Lock(gpSrv->csProc);

	if (gpSrv->nProcessCount == 0)
	{
		gpSrv->pnProcesses[0] = gnSelfPID;
		gpSrv->nProcessCount = 1;
	}

	if (gpSrv->DbgInfo.bDebuggerActive)
	{
		//if (gpSrv->hRootProcess) {
		//	if (WaitForSingleObject(gpSrv->hRootProcess, 0) == WAIT_OBJECT_0) {
		//		gpSrv->nProcessCount = 1;
		//		return TRUE;
		//	}
		//}
		//gpSrv->pnProcesses[1] = gpSrv->dwRootProcess;
		//gpSrv->nProcessCount = 2;
		return FALSE;
	}

	#ifdef _DEBUG
	bool bDumpProcInfo = gbIsWine;
	#endif

	bool bProcFound = false;
	bool bConsoleOnly = (gpSrv->hRootProcessGui == NULL);
	bool bMayBeConsolePaf = gpSrv->Portable.nPID && (gpSrv->Portable.nSubsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI);

	if (pfnGetConsoleProcessList && (bConsoleOnly || bMayBeConsolePaf))
	{
		WARNING("Переделать, как-то слишком сложно получается");
		DWORD nCurCount;
		nCurCount = pfnGetConsoleProcessList(gpSrv->pnProcessesGet, gpSrv->nMaxProcesses);

		#ifdef _DEBUG
		SetLastError(0);
		int nCurCountDbg = pfnGetConsoleProcessList(nCurProcessesDbg, countof(nCurProcessesDbg));
		DUMP_PROC_INFO(L"WinXP mode", nCurCountDbg, nCurProcessesDbg);
		#endif

		lbChanged = (gpSrv->nProcessCount != nCurCount);

		if (nCurCount && (gpSrv->nLastFoundPID != gpSrv->pnProcessesGet[0]))
			gpSrv->nLastFoundPID = gpSrv->pnProcessesGet[0];
		else if (!nCurCount)
			gpSrv->nLastFoundPID = 0;

		bProcFound = bConsoleOnly && (nCurCount > 0);

		if (nCurCount == 0)
		{
			_ASSERTE(gbTerminateOnCtrlBreak==FALSE);

			// Похоже что сюда мы попадаем также при ошибке 0xC0000142 запуска root-process
			#ifdef _DEBUG
			_ASSERTE(FALSE && "(nCurCount==0)?");
			// Some diagnostics for debugger
			DWORD nErr;
			HWND hConWnd = GetConsoleWindow(); nErr = GetLastError();
			HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE); nErr = GetLastError();
			CONSOLE_SCREEN_BUFFER_INFO sbi = {}; BOOL bSbi = GetConsoleScreenBufferInfo(hOut, &sbi); nErr = GetLastError();
			DWORD nWait = WaitForSingleObject(gpSrv->hRootProcess, 0);
			GetExitCodeProcess(gpSrv->hRootProcess, &nErr);
			#endif

			// Это значит в Win7 свалился conhost.exe
			//if ((gnOsVer >= 0x601) && !gbIsWine)
			{
				#ifdef _DEBUG
				DWORD dwErr = GetLastError();
				#endif
				gpSrv->nProcessCount = 1;
				SetEvent(ghQuitEvent);

				if (!nExitQueryPlace) nExitQueryPlace = 1+(nExitPlaceStep);

				SetTerminateEvent(ste_CheckProcessCount);
				return TRUE;
			}
		}
		else
		{
			if (nCurCount > gpSrv->nMaxProcesses)
			{
				DWORD nSize = nCurCount + 100;
				DWORD* pnPID = (DWORD*)calloc(nSize, sizeof(DWORD));

				if (pnPID)
				{
					CS.RelockExclusive(200);
					nCurCount = pfnGetConsoleProcessList(pnPID, nSize);

					if (nCurCount > 0 && nCurCount <= nSize)
					{
						free(gpSrv->pnProcessesGet);
						gpSrv->pnProcessesGet = pnPID; pnPID = NULL;
						free(gpSrv->pnProcesses);
						gpSrv->pnProcesses = (DWORD*)calloc(nSize, sizeof(DWORD));
						_ASSERTE(nExitQueryPlace == 0 || nCurCount == 1);
						gpSrv->nProcessCount = nCurCount;
						gpSrv->nMaxProcesses = nSize;
					}

					if (pnPID)
						free(pnPID);
				}
			}

			// PID-ы процессов в консоли могут оказаться перемешаны. Нас же интересует gnSelfPID на первом месте
			gpSrv->pnProcesses[0] = gnSelfPID;
			DWORD nCorrect = 1, nMax = gpSrv->nMaxProcesses, nDosBox = 0;
			if (gbUseDosBox)
			{
				if (ghDosBoxProcess && WaitForSingleObject(ghDosBoxProcess, 0) == WAIT_TIMEOUT)
				{
					gpSrv->pnProcesses[nCorrect++] = gnDosBoxPID;
					nDosBox = 1;
				}
				else if (ghDosBoxProcess)
				{
					ghDosBoxProcess = NULL;
				}
			}
			for (DWORD n = 0; n < nCurCount && nCorrect < nMax; n++)
			{
				if (gpSrv->pnProcessesGet[n] != gnSelfPID)
				{
					if (gpSrv->pnProcesses[nCorrect] != gpSrv->pnProcessesGet[n])
					{
						gpSrv->pnProcesses[nCorrect] = gpSrv->pnProcessesGet[n];
						lbChanged = TRUE;
					}
					nCorrect++;
				}
			}
			nCurCount += nDosBox;


			if (nCurCount < gpSrv->nMaxProcesses)
			{
				// Сбросить в 0 ячейки со старыми процессами
				_ASSERTE(gpSrv->nProcessCount < gpSrv->nMaxProcesses);

				if (nCurCount < gpSrv->nProcessCount)
				{
					UINT nSize = sizeof(DWORD)*(gpSrv->nProcessCount - nCurCount);
					memset(gpSrv->pnProcesses + nCurCount, 0, nSize);
				}

				_ASSERTE(nCurCount>0);
				_ASSERTE(nExitQueryPlace == 0 || nCurCount == 1);
				gpSrv->nProcessCount = nCurCount;
			}

			UINT nSize = sizeof(DWORD)*min(gpSrv->nMaxProcesses,START_MAX_PROCESSES);
			#ifdef _DEBUG
			_ASSERTE(!IsBadWritePtr(gpSrv->pnProcessesCopy,nSize));
			_ASSERTE(!IsBadWritePtr(gpSrv->pnProcesses,nSize));
			#endif

			if (!lbChanged)
			{
				lbChanged = memcmp(gpSrv->pnProcessesCopy, gpSrv->pnProcesses, nSize) != 0;
				MCHKHEAP;
			}

			if (lbChanged)
			{
				memmove(gpSrv->pnProcessesCopy, gpSrv->pnProcesses, nSize);

				MCHKHEAP;
			}
		}

		if (!bProcFound && bMayBeConsolePaf && (nCurCount > 0) && gpSrv->pnProcesses)
		{
			for (DWORD i = 0; i < nCurCount; i++)
			{
				if (gpSrv->pnProcesses[i] == gpSrv->Portable.nPID)
				{
					// В консоли обнаружен процесс запущенный из ChildGui (e.g. CommandPromptPortable.exe -> cmd.exe)
                    bProcFound = true;
				}
			}
		}
	}

	// Wine related: GetConsoleProcessList may fails
	// or ChildGui related: GUI app started in tab and (optionally) it starts another app (console or gui)
	if (!bProcFound)
	{
		_ASSERTE(gpSrv->pnProcesses[0] == gnSelfPID);
		gpSrv->pnProcesses[0] = gnSelfPID;

		HANDLE hRootProcess = gpSrv->hRootProcess;
		DWORD  nRootPID = gpSrv->dwRootProcess;

		if (hRootProcess || gpSrv->Portable.hProcess)
		{
			DWORD nRootWait = hRootProcess ? WaitForSingleObject(hRootProcess, 0) : WAIT_OBJECT_0;
			// PortableApps?
			if (nRootWait == WAIT_OBJECT_0)
			{
				if (gpSrv->Portable.hProcess)
				{
					nRootWait = WaitForSingleObject(gpSrv->Portable.hProcess, 0);
					if (nRootWait == WAIT_TIMEOUT)
						nRootPID = gpSrv->Portable.nPID;
				}
			}
			// Ok, Check it
			if (nRootWait == WAIT_OBJECT_0)
			{
				gpSrv->pnProcesses[1] = 0;
				lbChanged = gpSrv->nProcessCount != 1;
				gpSrv->nProcessCount = 1;
			}
			else
			{
				gpSrv->pnProcesses[1] = nRootPID;
				lbChanged = gpSrv->nProcessCount != 2;
				_ASSERTE(nExitQueryPlace == 0);
				gpSrv->nProcessCount = 2;
			}
		}
		else if (gpSrv->hRootProcessGui)
		{
			if (!IsWindow(gpSrv->hRootProcessGui))
			{
				// Process handle must be opened!
				_ASSERTE(gpSrv->hRootProcess != NULL);
				// Fin
				gpSrv->pnProcesses[1] = 0;
				lbChanged = gpSrv->nProcessCount != 1;
				gpSrv->nProcessCount = 1;
			}
		}

		DUMP_PROC_INFO(L"Win2k mode", gpSrv->nProcessCount, gpSrv->pnProcesses);
	}

	gpSrv->dwProcessLastCheckTick = GetTickCount();

	ProcessCountChanged(lbChanged, nPrevCount, &CS);


	return lbChanged;
}

bool GetRootInfo(CESERVER_REQ* pReq)
{
	if (!pReq)
		return false;
	if (!gpSrv || !gpSrv->dwRootProcess || !gpSrv->hRootProcess)
		return false;

	DWORD nWait = WaitForSingleObject(gpSrv->hRootProcess, 0);
	pReq->RootInfo.bRunning = (nWait == WAIT_TIMEOUT);
	pReq->RootInfo.nPID = gpSrv->dwRootProcess;
	pReq->RootInfo.nExitCode = (nWait == WAIT_TIMEOUT) ? STILL_ACTIVE : 999;
	GetExitCodeProcess(gpSrv->hRootProcess, &pReq->RootInfo.nExitCode);
	if (nWait == WAIT_TIMEOUT)
	{
		pReq->RootInfo.nUpTime = (GetTickCount() - gpSrv->dwRootStartTime);
	}
	else
	{
		FILETIME ftCreation = {}, ftExit = {}, ftKernel = {}, ftUser = {};
		GetProcessTimes(gpSrv->hRootProcess, &ftCreation, &ftExit, &ftKernel, &ftUser);
		__int64 upTime = ((*(u64*)&ftExit) - (*(u64*)&ftCreation)) / 10000LL;
		pReq->RootInfo.nUpTime = (LODWORD(upTime) == upTime) ? LODWORD(upTime) : (DWORD)-1;
	}

	return true;
}
