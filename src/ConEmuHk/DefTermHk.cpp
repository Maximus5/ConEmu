
/*
Copyright (c) 2012-2014 Maximus5
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

#include "../common/common.hpp"
#include "ConEmuHooks.h"
#include "DefTermHk.h"
#include "../ConEmu/version.h"

#define FOREGROUND_CHECK_DELAY  1000
#define STOP_HOOKERS_TIMEOUT    30000
#define SERIALIZE_CHECK_TIMEOUT 1000

#ifdef _DEBUG
	// Do not call OutputDebugString more that required, even in _DEBUG
	#define DebugStr(s) //OutputDebugString(s)
#else
	#define DebugStr(s)
#endif

#ifdef _DEBUG
	#define DefTermMsg(s) //MessageBox(NULL, s, L"ConEmuHk", MB_SYSTEMMODAL)
#else
	#define DefTermMsg(s) //MessageBox(NULL, s, L"ConEmuHk", MB_SYSTEMMODAL)
#endif

extern HMODULE ghOurModule; // Хэндл нашей dll'ки (здесь хуки не ставятся)

/* ************ Globals for "Default terminal ************ */
bool gbPrepareDefaultTerminal = false;
bool gbIsNetVsHost = false;
bool gbIsVStudio = false;
int  gnVsHostStartConsole = 0;
bool gbIsGdbHost = false;
//ConEmuGuiMapping* gpDefaultTermParm = NULL; -- // полный путь к ConEmu.exe (GUI), "/config", параметры для "confirm" и "no-injects"

CDefTermHk* gpDefTerm = NULL;

// helper
bool isDefTermEnabled()
{
	if (!gbPrepareDefaultTerminal || !gpDefTerm)
		return false;
	if (!gpDefTerm->isDefaultTerminalAllowed(gbIsNetVsHost))
		return false;
	return true;
}

bool InitDefTerm()
{
	bool lbRc = true;

	#ifdef _DEBUG
	wchar_t szInfo[100];
	msprintf(szInfo, countof(szInfo), L"!!! TH32CS_SNAPMODULE, TID=%u, InitDefaultTerm\n", GetCurrentThreadId());
	DebugStr(szInfo);
	#endif

	_ASSERTEX(gpDefTerm==NULL);
	gpDefTerm = new CDefTermHk();
	if (!gpDefTerm)
	{
		_ASSERTEX(gpDefTerm!=NULL);
		return false;
	}

	//_ASSERTE(FALSE && "InitDefaultTerm");

	// При обновлении ConEmu может обновиться и ConEmuHk.dll
	// Но в процессы с "DefTerm" грузится копия dll-ки, поэтому
	// после обновления в уже хукнутый процесс загружается
	// вторая "ConEmuHk.YYMMDD.dll", а старую при этом нужно
	// выгрузить. Этим и займемся.
	HMODULE hPrevHooks = NULL;
	_ASSERTEX(gnSelfPID!=0 && ghOurModule!=NULL);
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, gnSelfPID);
	if (hSnap != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 mi = {sizeof(mi)};
		//wchar_t szOurName[MAX_PATH] = L"";
		//GetModuleFileName(ghOurModule, szOurName, MAX_PATH);
		wchar_t szMinor[8] = L""; lstrcpyn(szMinor, WSTRING(MVV_4a), countof(szMinor));
		wchar_t szAddName[40];
		msprintf(szAddName, countof(szAddName),
			CEDEFTERMDLLFORMAT /*L"ConEmuHk%s.%02u%02u%02u%s.dll"*/,
			WIN3264TEST(L"",L"64"), MVV_1, MVV_2, MVV_3, szMinor);
		//LPCWSTR pszOurName = PointToName(szOurName);
		wchar_t* pszDot = wcschr(szAddName, L'.');
		wchar_t szCheckName[MAX_PATH+1];

		if (pszDot && Module32First(hSnap, &mi))
		{
			pszDot[1] = 0; // Need to check only name, without version number
			int nCurLen = lstrlen(szAddName);
			do {
				if (mi.hModule == ghOurModule)
					continue;
				lstrcpyn(szCheckName, PointToName(mi.szExePath), nCurLen+1);
				if (lstrcmpi(szCheckName, szAddName) == 0)
				{
					hPrevHooks = mi.hModule;
					break; // Prev (old version) instance found!
				}
			} while (Module32Next(hSnap, &mi));
		}

		CloseHandle(hSnap);
	}

	// Old library was found, unload it before continue
	if (hPrevHooks)
	{
		if (!FreeLibrary(hPrevHooks))
		{
			lbRc = false;
		}
	}

	// For Visual Studio check all spawned processes (children of gnSelfPID), find *.vshost.exe
	if (gbIsVStudio)
	{
		//_ASSERTEX(FALSE && "Continue to find existing *.vshost.exe");
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnap != INVALID_HANDLE_VALUE)
		{
			PROCESSENTRY32 pe = {sizeof(pe)};
			if (Process32First(hSnap, &pe)) do
			{
				if (pe.th32ParentProcessID == gnSelfPID)
				{
					if (IsVsNetHostExe(pe.szExeFile)) // *.vshost.exe
					{
						// Found! Hook it!
						gpDefTerm->StartDefTermHooker(pe.th32ProcessID);
						break;
					}
				}
			} while (Process32Next(hSnap, &pe));
			CloseHandle(hSnap);
		}
	}

	#ifdef _DEBUG
	DebugStr(L"InitDefaultTerm finished\n");
	#endif

	gpDefTerm->StartDefTerm();

	return lbRc;
}

/* ************ Globals for "Default terminal ************ */


CDefTermHk::CDefTermHk()
	: CDefTermBase(false)
{
	mh_StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	wchar_t szSelfName[MAX_PATH+1] = L"";
	GetModuleFileName(NULL, szSelfName, countof(szSelfName));
	lstrcpyn(ms_ExeName, PointToName(szSelfName), countof(ms_ExeName));

	mn_LastCheck = 0;
	ReloadSettings();
}

CDefTermHk::~CDefTermHk()
{
}

void CDefTermHk::StartDefTerm()
{
	Initialize(false, false, false);
}

void CDefTermHk::StopHookers()
{
	if (!gbPrepareDefaultTerminal)
		return;
	gbPrepareDefaultTerminal = false;

	SetEvent(mh_StopEvent);

	WaitForSingleObject(mh_PostThread, STOP_HOOKERS_TIMEOUT);

	CDefTermBase::StopHookers();
}

bool CDefTermHk::isDefaultTerminalAllowed(bool bDontCheckName /*= false*/)
{
	_ASSERTEX((gbPrepareDefaultTerminal==false) || ((gpDefTerm!=NULL) && (this==gpDefTerm)));

	if (!gbPrepareDefaultTerminal || !this)
	{
		_ASSERTEX(gbPrepareDefaultTerminal && this);
		return false;
	}

	DWORD nDelta = (GetTickCount() - mn_LastCheck);
	if (nDelta > SERIALIZE_CHECK_TIMEOUT)
	{
		ReloadSettings();
	}

	// Разрешен ли вообще?
	if (!m_Opt.bUseDefaultTerminal)
		return false;
	// Известны пути?
	if (!m_Opt.pszConEmuExe || !*m_Opt.pszConEmuExe || !m_Opt.pszConEmuBaseDir || !*m_Opt.pszConEmuBaseDir)
		return false;
	// Проверить НАШ exe-шник на наличие в m_Opt.pszzHookedApps
	if (!bDontCheckName && !IsAppMonitored(ms_ExeName))
		return false;
	// Да
	return true;
}

void CDefTermHk::PostCreateThreadFinished()
{
	// Запустить цикл проверки, необходимый для Agressive mode
	DWORD dwWait = WAIT_TIMEOUT;
	DWORD nForePID = 0;
	HWND  hFore = NULL;
	while ((dwWait = WaitForSingleObject(mh_StopEvent, FOREGROUND_CHECK_DELAY)) == WAIT_TIMEOUT)
	{
		// If non-aggressive - don't do anything here...
		if (!isDefaultTerminalAllowed(true) || !m_Opt.bAgressive)
			continue;

		// Aggressive mode
		hFore = GetForegroundWindow();
		if (!hFore)
			continue;
		if (!GetWindowThreadProcessId(hFore, &nForePID))
			continue;
		CheckForeground(hFore, nForePID, false);
	}
}

void CDefTermHk::ReloadSettings()
{
	MSectionLockSimple CS;
	CS.Lock(mcs);

	m_Opt.Serialize();
	mn_LastCheck = GetTickCount();
}

int CDefTermHk::DisplayLastError(LPCWSTR asLabel, DWORD dwError/*=0*/, DWORD dwMsgFlags/*=0*/, LPCWSTR asTitle/*=NULL*/, HWND hParent/*=NULL*/)
{
	DefTermMsg(asLabel);
	return 0;
}

void CDefTermHk::ShowTrayIconError(LPCWSTR asErrText)
{
	DefTermMsg(asErrText);
}

HWND CDefTermHk::AllocHiddenConsole(bool bTempForVS)
{
	// функция AttachConsole есть только в WinXP и выше
	AttachConsole_t _AttachConsole = GetAttachConsoleProc();
	if (!_AttachConsole)
		return NULL;

	DefTermMsg(L"AllocHiddenConsole");

	ReloadSettings();
	_ASSERTEX(isDefTermEnabled() && (gbIsNetVsHost || bTempForVS));

	if (!isDefTermEnabled())
	{
		// Disabled in settings or registry
		return NULL;
	}

	HANDLE hSrvProcess = NULL;
	DWORD nAttachPID = bTempForVS ? 0 : gnSelfPID;
	DWORD nSrvPID = StartConsoleServer(nAttachPID, true, &hSrvProcess);
	if (!nSrvPID)
	{
		// Failed to start process?
		return NULL;
	}
	_ASSERTEX(hSrvProcess!=NULL);

	HWND hCreatedCon = NULL;

	// Do while server process is alive
	DWORD nStart = GetTickCount(), nMaxDelta = 30000, nDelta = 0;
	DWORD nWait = WaitForSingleObject(hSrvProcess, 0);
	while (nWait != WAIT_OBJECT_0)
	{
		if (_AttachConsole(nSrvPID))
		{
			hCreatedCon = GetRealConsoleWindow();
			if (hCreatedCon)
				break;
		}

		nWait = WaitForSingleObject(hSrvProcess, 150);

		nDelta = (GetTickCount() - nStart);
		if (nDelta > nMaxDelta)
			break;
	}

	return hCreatedCon;
}

DWORD CDefTermHk::StartConsoleServer(DWORD nAttachPID, bool bNewConWnd, PHANDLE phSrvProcess)
{
	// Options must be loaded already
	const CEDefTermOpt* pOpt = GetOpt();
	if (!pOpt || !isDefTermEnabled())
	{
		return 0;
	}

	bool bAttachCreated = false;

	CmdArg szAddArgs, szAddArgs2;
	size_t cchAddArgLen;
	size_t cchMax;
	wchar_t* pszCmdLine;
	DWORD nErr = 0;
	STARTUPINFO si = {sizeof(si)};
	PROCESS_INFORMATION pi = {};

	cchAddArgLen = GetSrvAddArgs(false, szAddArgs, szAddArgs2);

	wchar_t szExeName[MAX_PATH] = L"";
	if (GetModuleFileName(NULL, szExeName, countof(szExeName)) && szExeName[0])
		cchAddArgLen += 20 + lstrlen(szExeName);

	cchMax = MAX_PATH+100+cchAddArgLen;
	pszCmdLine = (wchar_t*)malloc(cchMax*sizeof(*pszCmdLine));
	if (pszCmdLine)
	{
		_ASSERTE(nAttachPID || bNewConWnd);

		msprintf(pszCmdLine, cchMax, L"\"%s\\%s\" /ATTACH %s/TRMPID=%u",
			pOpt->pszConEmuBaseDir,
			WIN3264TEST(L"ConEmuC.exe",L"ConEmuC64.exe"),
			bNewConWnd ? L"/CREATECON " : L"",
			nAttachPID);

		if (szExeName[0])
		{
			_wcscat_c(pszCmdLine, cchMax, L" /ROOTEXE \"");
			_wcscat_c(pszCmdLine, cchMax, szExeName);
			_wcscat_c(pszCmdLine, cchMax, L"\"");
		}

		if (!szAddArgs.IsEmpty())
			_wcscat_c(pszCmdLine, cchMax, szAddArgs);

		_ASSERTEX(szAddArgs2.IsEmpty()); // No "-new_console" switches are expected here!

		DWORD nCreateFlags = NORMAL_PRIORITY_CLASS | (bNewConWnd ? CREATE_NEW_CONSOLE : 0);
		if (bNewConWnd)
		{
			si.wShowWindow = SW_HIDE;
			si.dwFlags = STARTF_USESHOWWINDOW;
		}

		if (CreateProcess(NULL, pszCmdLine, NULL, NULL, FALSE, nCreateFlags, NULL, pOpt->pszConEmuBaseDir, &si, &pi))
		{
			bAttachCreated = true;
			if (phSrvProcess)
				*phSrvProcess = pi.hProcess;
			else
				CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			SetServerPID(pi.dwProcessId);
		}
		else
		{
			nErr = GetLastError();
			wchar_t* pszErrMsg = (wchar_t*)malloc(MAX_PATH*3*sizeof(*pszErrMsg));
			if (pszErrMsg)
			{
				msprintf(pszErrMsg, MAX_PATH*3, L"ConEmuHk: Failed to start attach server, Err=%u! %s\n", nErr, pszCmdLine);
				wchar_t szTitle[64];
				msprintf(szTitle, countof(szTitle), WIN3264TEST(L"ConEmuHk",L"ConEmuHk64") L", PID=%u", GetCurrentProcessId());
				//OutputDebugString(pszErrMsg);
				MessageBox(NULL, pszErrMsg, szTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
				free(pszErrMsg);
			}
			// Failed to start, pi.dwProcessId must be 0
			_ASSERTEX(pi.dwProcessId==0);
			pi.dwProcessId = 0;
		}
		free(pszCmdLine);
	}

	return pi.dwProcessId;
}

void CDefTermHk::OnAllocConsoleFinished()
{
	if (!gbPrepareDefaultTerminal || !this)
	{
		_ASSERTEX((gbPrepareDefaultTerminal && gpDefTerm) && "Must be called in DefTerm mode only");
		return;
	}

	if (!ghConWnd || !IsWindow(ghConWnd))
	{
		_ASSERTEX(FALSE && "ghConWnd must be initialized already!");
		return;
	}

	ReloadSettings();
	_ASSERTEX(isDefTermEnabled() && gbIsNetVsHost);

	if (!isDefTermEnabled())
	{
		// Disabled in settings or registry
		return;
	}

	// По идее, после AllocConsole окно RealConsole всегда видимо
	BOOL bConWasVisible = IsWindowVisible(ghConWnd);
	_ASSERTEX(bConWasVisible);
	// Чтобы минимизировать "мелькания" - сразу спрячем его
	ShowWindow(ghConWnd, SW_HIDE);
	DefTermMsg(L"Console window hided");

	if (!StartConsoleServer(gnSelfPID, false, NULL))
	{
		if (bConWasVisible)
			ShowWindow(ghConWnd, SW_SHOW);
		DefTermMsg(L"Starting attach server failed?");
	}
}

size_t CDefTermHk::GetSrvAddArgs(bool bGuiArgs, CmdArg& rsArgs, CmdArg& rsNewCon)
{
	rsArgs.Empty();
	rsNewCon.Empty();

	if (!this)
		return 0;

	size_t cchMax = 64 + ((m_Opt.pszConfigName && *m_Opt.pszConfigName) ? lstrlen(m_Opt.pszConfigName) : 0);
	wchar_t* psz = rsArgs.GetBuffer(cchMax);
	size_t cchNew = 32; // "-new_console:ni"
	wchar_t* pszNew = rsNewCon.GetBuffer(cchNew);

	if (!psz || !pszNew)
		return 0;

	*psz = 0;
	*pszNew = 0;

	wchar_t szNewConSw[10] = L"";

	// Do not inject ConEmuHk in the target process?
	if (m_Opt.bNoInjects && !bGuiArgs)
		_wcscat_c(psz, cchMax, L" /NOINJECT");
	else if (m_Opt.bNoInjects)
		wcscat_c(szNewConSw, L"i");

	// New or existing window we shall use?
	if (m_Opt.bNewWindow && !bGuiArgs)
		_wcscat_c(psz, cchMax, L" /GHWND=NEW");
	else if (m_Opt.bNewWindow)
		_wcscat_c(psz, cchMax, L" /NOSINGLE");
	else if (bGuiArgs)
		_wcscat_c(psz, cchMax, L" /REUSE");

	// Confirmations
	if (m_Opt.nDefaultTerminalConfirmClose == 1)
	{
		if (!bGuiArgs)
			_wcscat_c(psz, cchMax, L" /CONFIRM");
		else
			wcscat_c(szNewConSw, L"c");
	}
	else if (m_Opt.nDefaultTerminalConfirmClose == 2)
	{
		if (!bGuiArgs)
			_wcscat_c(psz, cchMax, L" /NOCONFIRM");
		else
			wcscat_c(szNewConSw, L"n");
	}

	// That switch must be processed in server too!
	if (m_Opt.pszConfigName && *m_Opt.pszConfigName)
	{
		_wcscat_c(psz, cchMax, L" /CONFIG \"");
		_wcscat_c(psz, cchMax, m_Opt.pszConfigName);
		_wcscat_c(psz, cchMax, L"\"");
	}

	if (*szNewConSw)
	{
		_wcscpy_c(pszNew, cchNew, L"-new_console:");
		_wcscat_c(pszNew, cchNew, szNewConSw);
		_wcscat_c(pszNew, cchNew, L" ");
	}

	size_t cchLen = wcslen(psz) + wcslen(pszNew);
	return cchLen;
}
