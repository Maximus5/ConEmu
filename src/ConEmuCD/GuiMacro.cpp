
/*
Copyright (c) 2016-2017 Maximus5
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
#define SHOWDEBUGSTR

#include "../common/Common.h"
#include "../common/CmdLine.h"
#include "../common/ConEmuCheck.h"
#include "../common/CEStr.h"
#include "../common/MStrDup.h"
#include "../common/ProcessData.h"

#include "Actions.h"
#include "ConEmuC.h"
#include "GuiMacro.h"

bool    gbPreferSilentMode = false;
bool    gbMacroExportResult = false;


BOOL CALLBACK FindTopGuiOrConsole(HWND hWnd, LPARAM lParam)
{
	MacroInstance* p = (MacroInstance*)lParam;
	wchar_t szClass[MAX_PATH];
	if (GetClassName(hWnd, szClass, countof(szClass)) < 1)
		return TRUE; // continue search

	// If called "-GuiMacro:0" we need to find first GUI window!
	bool bConClass = isConsoleClass(szClass);
	if (!p->nPID && bConClass)
		return TRUE;

	bool bGuiClass = (lstrcmp(szClass, VirtualConsoleClassMain) == 0);
	if (!bGuiClass && !bConClass)
		return TRUE; // continue search

	DWORD nTestPID = 0; GetWindowThreadProcessId(hWnd, &nTestPID);
	if (nTestPID == p->nPID || !p->nPID)
	{
		p->hConEmuWnd = hWnd;
		return FALSE; // Found! stop search
	}

	return TRUE; // continue search
}

int GuiMacroCommandLine(LPCWSTR asCmdLine)
{
	int iRc = CERR_CMDLINEEMPTY;
	CEStr szArg;
	LPCWSTR pszArgStarts = NULL;
	LPCWSTR lsCmdLine = asCmdLine;
	MacroInstance MacroInst = {}; // Special ConEmu instance for GUIMACRO and other options

	while ((iRc = NextArg(&lsCmdLine, szArg, &pszArgStarts)) == 0)
	{
		// Following code wants '/'style arguments
		// Change '-'style to '/'style
		if (szArg[0] == L'-')
			szArg.SetAt(0, L'/');
		else if (szArg[0] != L'/')
			continue;

		// -GUIMACRO[:PID|HWND] <Macro string>
		if (lstrcmpni(szArg, L"/GUIMACRO", 9) == 0)
		{
			ArgGuiMacro(szArg, MacroInst);

			// Return result via EnvVar only
			gbPreferSilentMode = true;

			_ASSERTE(gnRunMode == RM_GUIMACRO);
			iRc = DoGuiMacro(lsCmdLine, MacroInst, gmf_SetEnvVar);

			// We've done
			gbInShutdown = TRUE;
			break;
		}
	}

	return iRc;
}

HWND GetConEmuExeHWND(DWORD nConEmuPID)
{
	HWND hConEmu = NULL;

	// EnumThreadWindows will not find child window,
	// so we use map (but it may be not created yet)

	MFileMapping<ConEmuGuiMapping> guiMap;
	guiMap.InitName(CEGUIINFOMAPNAME, nConEmuPID);
	const ConEmuGuiMapping* pMap = guiMap.Open(FALSE);
	if (pMap)
	{
		if ((pMap->cbSize >= (5*sizeof(DWORD)))
			&& (pMap->nGuiPID == nConEmuPID)
			&& IsWindow(pMap->hGuiWnd))
		{
			hConEmu = pMap->hGuiWnd;
		}
	}

	return hConEmu;
}

void ArgGuiMacro(CEStr& szArg, MacroInstance& Inst)
{
	wchar_t szLog[200];
	if (gpLogSize) gpLogSize->LogString(szArg);

	ZeroStruct(Inst);

	// Caller may specify PID/HWND of desired ConEmu instance,
	// or some other switches, like Tab or Split index
	_ASSERTE(!szArg.IsEmpty() && (lstrcmpni(szArg.ms_Val, L"/GuiMacro", 9) == 0));
	if (szArg[9] == L':' || szArg[9] == L'=')
	{
		wchar_t* pszEnd = NULL;
		wchar_t* pszID = szArg.ms_Val+10;
		// Loop through GuiMacro options
		while (pszID && *pszID)
		{
			// HWND of main ConEmu (GUI) window
			if ((pszID[0] == L'0' && (pszID[1] == L'x' || pszID[1] == L'X'))
				|| (pszID[0] == L'x' || pszID[0] == L'X'))
			{
				Inst.hConEmuWnd = (HWND)(DWORD_PTR)wcstoul(pszID[0] == L'0' ? (pszID+2) : (pszID+1), &pszEnd, 16);
				pszID = pszEnd;

				if (gpLogSize)
				{
					_wsprintf(szLog, SKIPLEN(countof(szLog)) L"Exact instance requested, HWND=x%08X", (DWORD)(DWORD_PTR)Inst.hConEmuWnd);
					gpLogSize->LogString(szLog);
				}
			}
			else if (isDigit(pszID[0]))
			{
				// Если тут передать "0" - то выполняем в первом попавшемся (по Z-order) окне ConEmu
				// То есть даже если ConEmuC вызван во вкладке, то
				// а) макро может выполниться в другом окне ConEmu (если наше свернуто)
				// б) макро может выполниться в другой вкладке (если наша не активна)
				Inst.nPID = wcstoul(pszID, &pszEnd, 10);

				wchar_t szExe[MAX_PATH] = L"";
				{
					CProcessData proc;
					if (!proc.GetProcessName(Inst.nPID, szExe, countof(szExe), NULL, 0, NULL))
						szExe[0] = 0;
				}

				// Using mapping information may be preferable due to speed
				// and more, ConEmu may be started as Child window
				if (!(Inst.hConEmuWnd = GetConEmuExeHWND(Inst.nPID))
					&& IsConEmuGui(szExe))
				{
					// If ConEmu.exe has just been started, mapping may be not ready yet
					HANDLE hWaitProcess = OpenProcess(SYNCHRONIZE|PROCESS_QUERY_INFORMATION, FALSE, Inst.nPID);
					if (!hWaitProcess && IsWin6())
						hWaitProcess = OpenProcess(SYNCHRONIZE|0x1000/*PROCESS_QUERY_LIMITED_INFORMATION*/, FALSE, Inst.nPID);
					DWORD nStartTick = GetTickCount();
					DWORD nWait, nDelay = hWaitProcess ? 60000 : 10000;
					// Until succeeded?
					do
					{
						// Wait a little while ConEmu is initializing
						if (hWaitProcess)
						{
							nWait = WaitForSingleObject(hWaitProcess, 100);
							if (nWait == WAIT_OBJECT_0)
								break; // ConEmu.exe was terminated
						}
						else
						{
							Sleep(100);
						}
						// Recheck
						if ((Inst.hConEmuWnd = GetConEmuExeHWND(Inst.nPID)) != NULL)
							break; // Found
					} while ((GetTickCount() - nStartTick) <= nDelay);
					// Stop checking
					SafeCloseHandle(hWaitProcess);
				}

				// If mapping with GUI PID does not exist - try to enum all Top level windows
				// We are searching for RealConsole or ConEmu window
				if (!Inst.hConEmuWnd)
				{
					EnumWindows(FindTopGuiOrConsole, (LPARAM)&Inst);
				}

				if (gpLogSize)
				{
					if (Inst.hConEmuWnd && Inst.nPID)
						_wsprintf(szLog, SKIPLEN(countof(szLog)) L"Exact PID=%u requested, instance found HWND=x%08X", Inst.nPID, (DWORD)(DWORD_PTR)Inst.hConEmuWnd);
					else if (Inst.hConEmuWnd && !Inst.nPID)
						_wsprintf(szLog, SKIPLEN(countof(szLog)) L"First found requested, instance found HWND=x%08X", (DWORD)(DWORD_PTR)Inst.hConEmuWnd);
					else
						_wsprintf(szLog, SKIPLEN(countof(szLog)) L"No instances was found (requested PID=%u) GuiMacro will fail", Inst.nPID);
					gpLogSize->LogString(szLog);
				}

				if (pszID == pszEnd)
					break;
				pszID = pszEnd;
			}
			else if (wcschr(L"SsTt", pszID[0]) && isDigit(pszID[1]))
			{
				switch (pszID[0])
				{
				case L'S': case L's':
					Inst.nSplitIndex = wcstoul(pszID+1, &pszEnd, 10);
					_wsprintf(szLog, SKIPLEN(countof(szLog)) L"Split was requested: %u", Inst.nSplitIndex);
					break;
				case L'T': case L't':
					Inst.nTabIndex = wcstoul(pszID+1, &pszEnd, 10);
					_wsprintf(szLog, SKIPLEN(countof(szLog)) L"Tab was requested: %u", Inst.nTabIndex);
					break;
				}
				gpLogSize->LogString(szLog);
				if (pszID == pszEnd)
					break;
				pszID = pszEnd;
			}
			else if (*pszID == L':')
			{
				pszID++;
			}
			else
			{
				_ASSERTE(FALSE && "Unsupported GuiMacro option");
				if (gpLogSize)
				{
					CEStr strErr(lstrmerge(L"Unsupported GuiMacro option: ", szArg.ms_Val));
					gpLogSize->LogString(strErr);
				}
				break;
			}
		}

		// This may be VirtualConsoleClassMain or RealConsoleClass...
		if (Inst.hConEmuWnd)
		{
			// Has no effect, if hMacroInstance == RealConsoleClass
			wchar_t szClass[MAX_PATH] = L"";
			if ((GetClassName(Inst.hConEmuWnd, szClass, countof(szClass)) > 0) && (lstrcmp(szClass, VirtualConsoleClassMain) == 0))
			{
				DWORD nGuiPID = 0; GetWindowThreadProcessId(Inst.hConEmuWnd, &nGuiPID);
				AllowSetForegroundWindow(nGuiPID);
			}
			if (gpLogSize)
			{
				gpLogSize->LogString(L"Found instance class name: ", true, NULL, false);
				gpLogSize->LogString(szClass, false, NULL, true);
			}
		}
	}
}

int DoGuiMacro(LPCWSTR asCmdArg, MacroInstance& Inst, GuiMacroFlags Flags, BSTR* bsResult /*= NULL*/)
{
	// If neither hMacroInstance nor ghConEmuWnd was set - Macro will fail most likely
	_ASSERTE(Inst.hConEmuWnd!=NULL || ghConEmuWnd!=NULL);

	wchar_t szErrInst[80] = L"FAILED:Specified ConEmu instance is not found";
	wchar_t szErrExec[80] = L"FAILED:Unknown GuiMacro execution error";

	// Don't allow to execute on wrong instance
	if ((Inst.nPID && !Inst.hConEmuWnd)
		|| (Inst.hConEmuWnd && !IsWindow(Inst.hConEmuWnd))
		)
	{
		if (bsResult)
		{
			*bsResult = ::SysAllocString(szErrInst);
		}

		bool bRedirect = false;
		bool bPrintError = (Flags & gmf_PrintResult) && ((bRedirect = IsOutputRedirected()) || !gbPreferSilentMode);
		if (bPrintError)
		{
			if (bRedirect) wcscat_c(szErrInst, L"\n"); // PowerShell... it does not insert linefeed
			_wprintf(szErrInst);
		}

		return CERR_GUIMACRO_FAILED;
	}

	HWND hCallWnd = Inst.hConEmuWnd ? Inst.hConEmuWnd : ghConWnd;

	// Все что в asCmdArg - выполнить в Gui
	int iRc = CERR_GUIMACRO_FAILED;
	int nLen = lstrlen(asCmdArg);
	//SetEnvironmentVariable(CEGUIMACRORETENVVAR, NULL);
	CESERVER_REQ *pIn = NULL, *pOut = NULL;
	pIn = ExecuteNewCmd(CECMD_GUIMACRO, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_GUIMACRO)+nLen*sizeof(wchar_t));
	pIn->GuiMacro.nTabIndex = Inst.nTabIndex;
	pIn->GuiMacro.nSplitIndex = Inst.nSplitIndex;
	lstrcpyW(pIn->GuiMacro.sMacro, asCmdArg);

	LogString(L"DoGuiMacro: executing CECMD_GUIMACRO");

	pOut = ExecuteGuiCmd(hCallWnd, pIn, ghConWnd);

	LogString(L"DoGuiMacro: CECMD_GUIMACRO finished");

	if (pOut)
	{
		if (pOut->GuiMacro.nSucceeded)
		{
			LPCWSTR pszResult = (pOut->DataSize() >= sizeof(pOut->GuiMacro)) ? pOut->GuiMacro.sMacro : L"";

			iRc = CERR_GUIMACRO_SUCCEEDED; // OK

			if (bsResult)
			{
				*bsResult = ::SysAllocString(pszResult);
			}

			if (Flags & (gmf_SetEnvVar|gmf_ExportEnvVar))
			{
				if (gpLogSize)
				{
					CEStr lsLog(L"DoGuiMacro: set ", CEGUIMACRORETENVVAR, L"=", pszResult);
					LogString(lsLog);
				}
				SetEnvironmentVariable(CEGUIMACRORETENVVAR, pszResult);
			}

			// If current RealConsole was already started in ConEmu
			if (Flags & gmf_ExportEnvVar)
			{
				_ASSERTE((Flags & gmf_SetEnvVar));
				// Transfer EnvVar to parent console processes
				// This would work only if ‘Inject ConEmuHk’ is enabled
				// However, it's ignored by some shells
				LogString(L"DoGuiMacro: exporting result");
				DoExportEnv(CEGUIMACRORETENVVAR, ea_ExportCon, true/*bSilent*/);
				LogString(L"DoGuiMacro: DoExportEnv finished");
			}

			// Let reuse `-Silent` switch
			if ((Flags & gmf_PrintResult)
				&& (!gbPreferSilentMode || IsOutputRedirected()))
			{
				LogString(L"DoGuiMacro: printing result");

				// Show macro result in StdOutput
				_wprintf(pszResult);

				// PowerShell... it does not insert linefeed
				if (!IsOutputRedirected())
					_wprintf(L"\n");
			}
		}

		ExecuteFreeResult(pOut);
	}
	ExecuteFreeResult(pIn);

	if ((iRc != CERR_GUIMACRO_SUCCEEDED) && bsResult)
	{
		*bsResult = ::SysAllocString(szErrExec);
	}

	return iRc;
}

int __stdcall GuiMacro(LPCWSTR asInstance, LPCWSTR asMacro, BSTR* bsResult /*= NULL*/)
{
	LogString(L"GuiMacro function called");

	MacroInstance Inst = {};

	if (asInstance && *asInstance)
	{
		_ASSERTE((lstrcmpni(asInstance, L"/GuiMacro", 9) != 0) && (lstrcmpni(asInstance, L"-GuiMacro", 9) != 0));

		CEStr lsArg = lstrmerge(L"/GuiMacro:", asInstance);
		ArgGuiMacro(lsArg, Inst);
	}

	GuiMacroFlags Flags = gmf_None;
	if (bsResult == NULL)
		Flags = gmf_SetEnvVar;
	else if (bsResult == (BSTR*)-1)
		bsResult = NULL;

	int iRc = DoGuiMacro(asMacro, Inst, Flags, bsResult);
	return iRc;
}
