
/*
Copyright (c) 2012-2017 Maximus5
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

#include "../common/Common.h"
#include "../common/MFileLog.h"
#include "../common/MStrDup.h"
#include "../common/MToolHelp.h"
#include "hlpConsole.h"
#include "hlpProcess.h"
#include "DefTermHk.h"
#include "SetHook.h"
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
bool gbIsVsCode = false;
int  gnVsHostStartConsole = 0;
bool gbIsGdbHost = false;
//ConEmuGuiMapping* gpDefaultTermParm = NULL; -- // полный путь к ConEmu.exe (GUI), "/config", параметры для "confirm" и "no-injects"

CDefTermHk* gpDefTerm = NULL;

// helper
bool isDefTermEnabled()
{
	if (!gbPrepareDefaultTerminal || !gpDefTerm)
		return false;
	bool bDontCheckName = gbIsNetVsHost
		// Especially for Code, which starts detached "cmd /c start /wait"
		|| ((ghConWnd == NULL) && (gnImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI))
		;
	if (!gpDefTerm->isDefaultTerminalAllowed(bDontCheckName))
		return false;
	return true;
}

#if defined(__GNUC__)
extern "C"
#endif
void WINAPI RequestDefTermShutdown()
{
	if (!gbPrepareDefaultTerminal)
	{
		_ASSERTEX(FALSE && "Expected in DefTerm only!");
		return;
	}
	gnDllState |= ds_OnDefTermShutdown;
	DoDllStop(true, ds_OnDefTermShutdown);
}

bool InitDefTerm()
{
	bool lbRc = true;

	wchar_t szInfo[MAX_PATH*2];
	msprintf(szInfo, countof(szInfo), L"!!! TH32CS_SNAPMODULE, TID=%u, InitDefaultTerm\n", GetCurrentThreadId());
	DebugStr(szInfo); // Don't call DefTermLogString here - gpDefTerm was not initialized yet

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
					msprintf(szInfo, countof(szInfo),
						L"Prevous ConEmuHk module found at address " WIN3264TEST(L"0x%08X",L"0x%X%08X") L": %s",
						WIN3264WSPRINT(mi.hModule), mi.szExePath);
					DefTermLogString(szInfo);

					hPrevHooks = mi.hModule;
					break; // Prev (old version) instance found!
				}
			} while (Module32Next(hSnap, &mi));
		}

		CloseHandle(hSnap);
	}

	if (hPrevHooks)
	{
		DefTermLogString(L"Trying to unload previous ConEmuHk module");

		// To avoid possible problems with CloseHandle in "unhooking" state
		gpDefTerm->mh_InitDefTermContinueFrom = OpenThread(SYNCHRONIZE, FALSE, GetCurrentThreadId());

		// gh#322: We must not call MH_Uninitialize because it deinitializes HEAP
		//	but when we are in DefTerm mode, this OPCODE addresses are used ATM...
		HANDLE hThread = apiCreateThread(CDefTermHk::InitDefTermContinue, (LPVOID)hPrevHooks, &gpDefTerm->mn_InitDefTermContinueTID, "InitDefTermContinue");
		if (hThread)
		{
			CloseHandle(hThread);
		}
		else
		{
			lbRc = false;
		}
	}
	else
	{
		CDefTermHk::InitDefTermContinue(NULL);
	}

	return lbRc;
}

DWORD CDefTermHk::InitDefTermContinue(LPVOID ahPrevHooks)
{
	HMODULE hPrevHooks = (HMODULE)ahPrevHooks;
	DWORD nFromWait = 1;

	// Old library was found, unload it before continue
	if (hPrevHooks)
	{
		DefTermLogString(L"Trying to unload previous ConEmuHk module (thread)");

		// To avoid possible problems with CloseHandle in "unhooking" state
		nFromWait = WaitForSingleObject(gpDefTerm->mh_InitDefTermContinueFrom, 30000);
		if (!gpDefTerm)
		{
			DefTermLogString(L"Unloading failed, gpDefTerm destroyed");
			return 1;
		}
		_ASSERTEX(nFromWait == WAIT_OBJECT_0);
		// Wait a little more
		Sleep(100);
		SafeCloseHandle(gpDefTerm->mh_InitDefTermContinueFrom);

		typedef void (WINAPI* RequestDefTermShutdown_t)();
		RequestDefTermShutdown_t fnShutdown = (RequestDefTermShutdown_t)GetProcAddress(hPrevHooks, "RequestDefTermShutdown");
		if (fnShutdown)
		{
			fnShutdown();
		}

		SetLastError(0);
		BOOL bFree = FreeLibrary(hPrevHooks);
		DWORD nFreeRc = GetLastError();
		if (!bFree)
		{
			gpDefTerm->DisplayLastError(L"Unloading failed", nFreeRc);
			return 2;
		}
		else
		{
			DefTermLogString(L"Unloading succeeded");
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
						DefTermLogString(L"Child VsNetHost found, hooking");
						gpDefTerm->StartDefTermHooker(pe.th32ProcessID);
						break;
					}
				}
			} while (Process32Next(hSnap, &pe));
			CloseHandle(hSnap);
		}
	}
	else
	{
		MToolHelpProcess findForks;
		MArray<PROCESSENTRY32> Forks;
		TODO("OPTIMIZE: Try to find and process children from all levels, BUT ONLY from ROOT process?");
		if (findForks.FindForks(Forks))
		{
			PROCESSENTRY32 p;
			while (Forks.pop_back(p))
			{
				DefTermLogString(L"Forked process found, hooking");
				gpDefTerm->StartDefTermHooker(p.th32ProcessID);
			}
		}
	}

	DefTermLogString(L"InitDefaultTerm finished, calling StartDefTerm");

	gpDefTerm->StartDefTerm();

	// Continue to hooks
	DllStart_Continue();

	return 0;
}

void DefTermLogString(LPCSTR asMessage, LPCWSTR asLabel /*= NULL*/)
{
	if (!gpDefTerm || !asMessage || !*asMessage)
		return;
	INT_PTR iLen = lstrlenA(asMessage);
	CEStr lsMsg;
	MultiByteToWideChar(CP_ACP, 0, asMessage, iLen, lsMsg.GetBuffer(iLen), iLen);
	DefTermLogString(lsMsg.ms_Val, asLabel);
}

void DefTermLogString(LPCWSTR asMessage, LPCWSTR asLabel /*= NULL*/)
{
	if (!asMessage || !*asMessage)
	{
		return;
	}

	// To ensure that we may force debug output for troubleshooting
	// even from non-def-term-ed applications
	if (!gpDefTerm)
	{
		DebugStr(asMessage);
		if (asMessage[lstrlen(asMessage) - 1] != L'\n')
		{
			DebugStr(L"\n");
		}
		return;
	}

	LPCWSTR pszReady = asMessage;
	CEStr lsBuf;
	if (asLabel && *asLabel)
	{
		lsBuf = lstrmerge(asLabel, asMessage);
		if (lsBuf.ms_Val)
			pszReady = lsBuf.ms_Val;
	}

	gpDefTerm->LogHookingStatus(GetCurrentProcessId(), pszReady);
}


/* ************ Globals for "Default terminal ************ */


CDefTermHk::CDefTermHk()
	: CDefTermBase(false)
{
	mh_StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	wchar_t szSelfName[MAX_PATH+1] = L"";
	GetModuleFileName(NULL, szSelfName, countof(szSelfName));
	lstrcpyn(ms_ExeName, PointToName(szSelfName), countof(ms_ExeName));

	mp_FileLog = NULL;

	mn_LastCheck = 0;
	ReloadSettings();

	mn_InitDefTermContinueTID = 0;
	mh_InitDefTermContinueFrom = NULL;
}

CDefTermHk::~CDefTermHk()
{
	SafeDelete(mp_FileLog);
	StopHookers();
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
	mb_Termination = true;

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

	if (m_Opt.bDebugLog)
	{
		LogInit();
	}
	else
	{
		SafeDelete(mp_FileLog);
	}
}

void CDefTermHk::LogInit()
{
	if (mp_FileLog && mp_FileLog->IsLogOpened())
		return;

	if (!mp_FileLog)
	{
		mp_FileLog = new MFileLog(ms_ExeName);
		if (!mp_FileLog)
			return;
	}

	HRESULT hr = mp_FileLog->CreateLogFile();
	if (hr != 0)
	{
		SafeDelete(mp_FileLog);
		return;
	}
	mp_FileLog->LogString(GetCommandLineW());
}

void CDefTermHk::LogHookingStatus(DWORD nForePID, LPCWSTR sMessage)
{
	if (!mp_FileLog || !mp_FileLog->IsLogOpened())
	{
		if (sMessage && *sMessage)
		{
			DebugStr(sMessage);
			if (sMessage[lstrlen(sMessage)-1] != L'\n')
			{
				DebugStr(L"\n");
			}
		}
		return;
	}
	wchar_t szPID[16];
	CEStr lsLog(L"DefTerm[", _ultow(nForePID, szPID, 10), L"]: ", sMessage);
	mp_FileLog->LogString(lsLog);
}

CDefTermBase* CDefTermHk::GetInterface()
{
	return this;
}

int CDefTermHk::DisplayLastError(LPCWSTR asLabel, DWORD dwError/*=0*/, DWORD dwMsgFlags/*=0*/, LPCWSTR asTitle/*=NULL*/, HWND hParent/*=NULL*/)
{
	if (dwError)
	{
		wchar_t szInfo[64];
		msprintf(szInfo, countof(szInfo), L", ErrCode=x%X(%i)", dwError, (int)dwError);
		CEStr lsMsg = lstrmerge(asLabel, szInfo);
		LogHookingStatus(GetCurrentProcessId(), lsMsg);
	}
	else
	{
		LogHookingStatus(GetCurrentProcessId(), asLabel);
	}
	return 0;
}

void CDefTermHk::ShowTrayIconError(LPCWSTR asErrText)
{
	LogHookingStatus(GetCurrentProcessId(), asErrText);
	DefTermMsg(asErrText);
}

HWND CDefTermHk::AllocHiddenConsole(bool bTempForVS)
{
	// Function AttachConsole exists in WinXP and above, need dynamic link
	AttachConsole_t _AttachConsole = GetAttachConsoleProc();
	if (!_AttachConsole)
	{
		LogHookingStatus(GetCurrentProcessId(), L"Can't create hidden console, function does not exist");
		return NULL;
	}

	LogHookingStatus(GetCurrentProcessId(), L"AllocHiddenConsole");

	ReloadSettings();
	_ASSERTEX(isDefTermEnabled() && (gbIsNetVsHost || bTempForVS || gbPrepareDefaultTerminal));

	if (!isDefTermEnabled())
	{
		// Disabled in settings or registry
		LogHookingStatus(GetCurrentProcessId(), L"Application skipped by settings");
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
		LogHookingStatus(GetCurrentProcessId(), L"Applicatio skipped by settings");
		return 0;
	}

	bool bAttachCreated = false;

	CEStr szAddArgs, szAddArgs2;
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

		LogHookingStatus(GetCurrentProcessId(), pszCmdLine);

		if (CreateProcess(NULL, pszCmdLine, NULL, NULL, FALSE, nCreateFlags, NULL, pOpt->pszConEmuBaseDir, &si, &pi))
		{
			LogHookingStatus(GetCurrentProcessId(), L"Console server was successfully created");
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
				LogHookingStatus(GetCurrentProcessId(), pszErrMsg);
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

void CDefTermHk::OnAllocConsoleFinished(HWND hNewConWnd)
{
	if (!gbPrepareDefaultTerminal || !this)
	{
		_ASSERTEX((gbPrepareDefaultTerminal && gpDefTerm) && "Must be called in DefTerm mode only");
		return;
	}

	if (!hNewConWnd || !IsWindow(hNewConWnd))
	{
		LogHookingStatus(GetCurrentProcessId(), L"OnAllocConsoleFinished: hNewConWnd must be initialized already!");
		_ASSERTEX(FALSE && "hNewConWnd must be initialized already!");
		return;
	}

	ReloadSettings();

	if (!isDefTermEnabled())
	{
		_ASSERTEX(isDefTermEnabled());
		// Disabled in settings or registry
		LogHookingStatus(GetCurrentProcessId(), L"OnAllocConsoleFinished: !isDefTermEnabled()");
		return;
	}

	// VsConsoleHost has some specifics (debugging managed apps)
	if (gbIsNetVsHost)
	{
		// По идее, после AllocConsole окно RealConsole всегда видимо
		BOOL bConWasVisible = IsWindowVisible(hNewConWnd);
		_ASSERTEX(bConWasVisible);
		// Чтобы минимизировать "мелькания" - сразу спрячем его
		ShowWindow(hNewConWnd, SW_HIDE);
		LogHookingStatus(GetCurrentProcessId(), L"Console window was hidden");

		if (!StartConsoleServer(gnSelfPID, false, NULL))
		{
			if (bConWasVisible)
				ShowWindow(hNewConWnd, SW_SHOW);
			LogHookingStatus(GetCurrentProcessId(), L"Starting attach server failed?");
		}
	}

	// If required - hook full set of console functions
	if (!(gnDllState & ds_HooksCommon))
	{
		// Add to list all **absent** functions
		InitHooksCommon();
		// Set hooks on all functions weren't hooked before
		SetAllHooks();
	}

	// Refresh ghConWnd, mapping, ServerPID, etc.
	OnConWndChanged(hNewConWnd);
}

size_t CDefTermHk::GetSrvAddArgs(bool bGuiArgs, CEStr& rsArgs, CEStr& rsNewCon)
{
	rsArgs.Empty();
	rsNewCon.Empty();

	if (!this)
		return 0;

	size_t cchMax = 64
		+ ((m_Opt.pszCfgFile && *m_Opt.pszCfgFile) ? (20 + lstrlen(m_Opt.pszCfgFile)) : 0)
		+ ((m_Opt.pszConfigName && *m_Opt.pszConfigName) ? (12 + lstrlen(m_Opt.pszConfigName)) : 0)
		;
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

	// That switches must be processed in server too!
	if (m_Opt.pszCfgFile && *m_Opt.pszCfgFile)
	{
		_wcscat_c(psz, cchMax, L" /LoadCfgFile \"");
		_wcscat_c(psz, cchMax, m_Opt.pszCfgFile);
		_wcscat_c(psz, cchMax, L"\"");
	}
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
