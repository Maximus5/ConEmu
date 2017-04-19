
/*
Copyright (c) 2011-2017 Maximus5
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

#ifdef _DEBUG
//	#define PRINT_SHELL_LOG
	#undef PRINT_SHELL_LOG
#else
	#undef PRINT_SHELL_LOG
#endif

#include <windows.h>
#include <TCHAR.h>
#include <Tlhelp32.h>
#include <shlwapi.h>
#pragma warning(disable: 4091)
#include <Shlobj.h>
#pragma warning(default: 4091)
#include "../common/CmdLine.h"
#include "../common/Common.h"
#include "../common/ConEmuCheck.h"
#include "../common/execute.h"
#include "../common/HandleKeeper.h"
#include "../common/MStrDup.h"
#include "../common/WErrGuard.h"
#include "../common/WObjects.h"
#if defined(__GNUC__) && !defined(__MINGW32__)
#include "../ConEmu/ShObjIdl_Part.h"
#endif
#include "../ConEmuCD/ExitCodes.h"
#include "Ansi.h"
#include "DefTermHk.h"
#include "GuiAttach.h"
#include "Injects.h"
#include "SetHook.h"
#include "ShellProcessor.h"
#include "MainThread.h"

#ifndef SEE_MASK_NOZONECHECKS
#define SEE_MASK_NOZONECHECKS 0x800000
#endif
#ifndef SEE_MASK_NOASYNC
#define SEE_MASK_NOASYNC 0x00000100
#endif

#ifdef PRINT_SHELL_LOG
#include <stdio.h>
#endif


#ifdef _DEBUG
#ifndef CONEMU_MINIMAL
void TestShellProcessor()
{
	for (int i = 0; i < 10; i++)
	{
		MCHKHEAP;
		CShellProc* sp = new CShellProc;
		LPCWSTR pszFile = NULL, pszParam = NULL;
		DWORD nCreateFlags = 0, nShowCmd = 0;
		SHELLEXECUTEINFOW sei = {sizeof(SHELLEXECUTEINFOW)};
		STARTUPINFOW si = {sizeof(STARTUPINFOW)};
		switch (i)
		{
		case 0:
			pszFile = L"C:\\GCC\\mingw\\bin\\mingw32-make.exe";
			pszParam = L"mingw32-make \"1.cpp\" ";
			sp->OnCreateProcessW(&pszFile, &pszParam, &nCreateFlags, &si);
			break;
		case 1:
			pszFile = L"C:\\GCC\\mingw\\bin\\mingw32-make.exe";
			pszParam = L"\"mingw32-make.exe\" \"1.cpp\" ";
			sp->OnCreateProcessW(&pszFile, &pszParam, &nCreateFlags, &si);
			break;
		case 2:
			pszFile = L"C:\\GCC\\mingw\\bin\\mingw32-make.exe";
			pszParam = L"\"C:\\GCC\\mingw\\bin\\mingw32-make.exe\" \"1.cpp\" ";
			sp->OnCreateProcessW(&pszFile, &pszParam, &nCreateFlags, &si);
			break;
		case 3:
			pszFile = L"F:\\VCProject\\FarPlugin\\ConEmu\\Bugs\\DOS\\Prince\\PRINCE.EXE";
			pszParam = L"prince megahit";
			sp->OnCreateProcessW(&pszFile, &pszParam, &nCreateFlags, &si);
			break;
		case 4:
			pszFile = NULL;
			pszParam = L" \"F:\\VCProject\\FarPlugin\\ConEmu\\Bugs\\DOS\\Prince\\PRINCE.EXE\"";
			sp->OnCreateProcessW(&pszFile, &pszParam, &nCreateFlags, &si);
			break;
		case 5:
			pszFile = L"C:\\GCC\\mingw\\bin\\mingw32-make.exe";
			pszParam = L" \"1.cpp\" ";
			sp->OnShellExecuteW(NULL, &pszFile, &pszParam, &nCreateFlags, &nShowCmd);
			break;
		default:
			break;
		}
		MCHKHEAP;
		delete sp;
	}
}
#endif
#endif


//int CShellProc::mn_InShellExecuteEx = 0;
int gnInShellExecuteEx = 0;

extern struct HookModeFar gFarMode;

bool  CShellProc::mb_StartingNewGuiChildTab = 0;
DWORD CShellProc::mn_LastStartedPID = 0;
PROCESS_INFORMATION CShellProc:: m_WaitDebugVsThread = {};

CShellProc::CShellProc()
{
	mn_CP = AreFileApisANSI() ? CP_ACP : CP_OEMCP;
	mpwsz_TempAction = mpwsz_TempFile = mpwsz_TempParam = NULL;
	mpsz_TempRetFile = mpsz_TempRetParam = mpsz_TempRetDir = NULL;
	mpwsz_TempRetFile = mpwsz_TempRetParam = mpwsz_TempRetDir = NULL;
	mlp_ExecInfoA = NULL; mlp_ExecInfoW = NULL;
	mlp_SaveExecInfoA = NULL; mlp_SaveExecInfoW = NULL;
	mb_WasSuspended = FALSE;
	mb_NeedInjects = FALSE;
	mb_Opt_DontInject = false;
	mb_Opt_SkipNewConsole = false;
	mb_Opt_SkipCmdStart = false;
	mb_DebugWasRequested = FALSE;
	mb_HiddenConsoleDetachNeed = FALSE;
	mb_PostInjectWasRequested = FALSE;
	// int CShellProc::mn_InShellExecuteEx = 0; <-- static
	mb_InShellExecuteEx = FALSE;
	//mb_DosBoxAllowed = FALSE;
	m_SrvMapping.cbSize = 0;

	mb_TempConEmuWnd = FALSE;
	mh_PreConEmuWnd = ghConEmuWnd; mh_PreConEmuWndDC = ghConEmuWndDC;

	hOle32 = NULL;

	// Current application is GUI subsystem run in ConEmu tab?
	CheckIsCurrentGuiClient();

	if (gbPrepareDefaultTerminal)
	{
		FindCheckConEmuWindow();
	}
}

CShellProc::~CShellProc()
{
	if (mb_TempConEmuWnd)
	{
		ghConEmuWnd = mh_PreConEmuWnd; ghConEmuWndDC = mh_PreConEmuWndDC;
	}

	if (mb_InShellExecuteEx && gnInShellExecuteEx)
	{
		gnInShellExecuteEx--;
		mb_InShellExecuteEx = FALSE;
	}

	if (mpwsz_TempAction)
		free(mpwsz_TempAction);
	if (mpwsz_TempFile)
		free(mpwsz_TempFile);
	if (mpwsz_TempParam)
		free(mpwsz_TempParam);
	// results
	if (mpsz_TempRetFile)
		free(mpsz_TempRetFile);
	if (mpsz_TempRetParam)
		free(mpsz_TempRetParam);
	if (mpsz_TempRetDir)
		free(mpsz_TempRetDir);
	if (mpwsz_TempRetFile)
		free(mpwsz_TempRetFile);
	if (mpwsz_TempRetParam)
		free(mpwsz_TempRetParam);
	if (mpwsz_TempRetDir)
		free(mpwsz_TempRetDir);
	// structures
	if (mlp_ExecInfoA)
		free(mlp_ExecInfoA);
	if (mlp_ExecInfoW)
		free(mlp_ExecInfoW);

	if (hOle32)
	{
		FreeLibrary(hOle32);
	}
}

bool CShellProc::InitOle32()
{
	if (hOle32)
		return true;

	hOle32 = LoadLibrary(L"Ole32.dll");
	if (!hOle32)
		return false;

	CoInitializeEx_f = (CoInitializeEx_t)GetProcAddress(hOle32, "CoInitializeEx");
	CoCreateInstance_f = (CoCreateInstance_t)GetProcAddress(hOle32, "CoCreateInstance");
	if (!CoInitializeEx_f || !CoCreateInstance_f)
	{
		_ASSERTEX(CoInitializeEx_f && CoCreateInstance_f);
		FreeLibrary(hOle32);
		return false;
	}

	return true;
}

bool CShellProc::GetLinkProperties(LPCWSTR asLnkFile, CEStr& rsExe, CEStr& rsArgs, CEStr& rsWorkDir)
{
	bool bRc = false;
	IPersistFile* pFile = NULL;
	IShellLinkW*  pShellLink = NULL;
	HRESULT hr;
	DWORD nLnkSize;
	static bool bCoInitialized = false;

	if (!FileExists(asLnkFile, &nLnkSize))
		goto wrap;

	if (!InitOle32())
		goto wrap;

	hr = CoCreateInstance_f(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&pShellLink);
	if (FAILED(hr) && !bCoInitialized)
	{
		bCoInitialized = true;
		hr = CoInitializeEx_f(NULL, COINIT_MULTITHREADED);
		if (SUCCEEDED(hr))
			hr = CoCreateInstance_f(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&pShellLink);
	}
	if (FAILED(hr) || !pShellLink)
		goto wrap;

	hr = pShellLink->QueryInterface(IID_IPersistFile, (void**)&pFile);
	if (FAILED(hr) || !pFile)
		goto wrap;

	hr = pFile->Load(asLnkFile, STGM_READ);
	if (FAILED(hr))
		goto wrap;

	hr = pShellLink->GetPath(rsExe.GetBuffer(MAX_PATH), MAX_PATH, NULL, 0);
	if (FAILED(hr) || rsExe.IsEmpty())
		goto wrap;

	hr = pShellLink->GetWorkingDirectory(rsWorkDir.GetBuffer(MAX_PATH+1), MAX_PATH+1);
	if (FAILED(hr))
		goto wrap;

	hr = pShellLink->GetArguments(rsArgs.GetBuffer(nLnkSize), nLnkSize);
	if (FAILED(hr))
		goto wrap;

	bRc = true;
wrap:
	if (pFile)
		pFile->Release();
	if (pShellLink)
		pShellLink->Release();
	return bRc;
}

HWND CShellProc::FindCheckConEmuWindow()
{
	if (!gbPrepareDefaultTerminal)
		return NULL;

	if (ghConEmuWnd)
	{
		if (IsWindow(ghConEmuWnd))
		{
			return ghConEmuWnd; // OK, живое
		}

		// Сброс, ищем заново
		ghConEmuWnd = ghConEmuWndDC = NULL;
		mb_TempConEmuWnd = TRUE;
	}

	HWND h = FindWindowEx(NULL, NULL, VirtualConsoleClassMain, NULL);
	if (h)
	{
		ghConEmuWnd = h;
		HWND hWork = FindWindowEx(h, NULL, VirtualConsoleClassWork, NULL);
		_ASSERTEX(hWork!=NULL && "Workspace must be inside ConEmu"); // код расчитан на это
		ghConEmuWndDC = FindWindowEx(h, NULL, VirtualConsoleClass, NULL);
		if (!ghConEmuWndDC)
		{
			// This may be, if ConEmu was started in "Detached" more,
			// or is not closing on last tab close.
			ghConEmuWndDC = h;
		}
		mb_TempConEmuWnd = TRUE;
	}
	return h;
}

wchar_t* CShellProc::str2wcs(const char* psz, UINT anCP)
{
	if (!psz)
		return NULL;
	int nLen = lstrlenA(psz);
	wchar_t* pwsz = (wchar_t*)calloc((nLen+1),sizeof(wchar_t));
	if (nLen > 0)
	{
		MultiByteToWideChar(anCP, 0, psz, nLen+1, pwsz, nLen+1);
	}
	else
	{
		pwsz[0] = 0;
	}
	return pwsz;
}
char* CShellProc::wcs2str(const wchar_t* pwsz, UINT anCP)
{
	int nLen = lstrlen(pwsz);
	char* psz = (char*)calloc((nLen+1),sizeof(char));
	if (nLen > 0)
	{
		WideCharToMultiByte(anCP, 0, pwsz, nLen+1, psz, nLen+1, 0, 0);
	}
	else
	{
		psz[0] = 0;
	}
	return psz;
}

BOOL CShellProc::GetLogLibraries()
{
	if (m_SrvMapping.cbSize)
		return (m_SrvMapping.nLoggingType == glt_Processes);
	return FALSE;
}

const RConStartArgs* CShellProc::GetArgs()
{
	return &m_Args;
}

BOOL CShellProc::LoadSrvMapping(BOOL bLightCheck /*= FALSE*/)
{
	if (!this)
	{
		_ASSERTEX(FALSE && "Object was not created!");
		return FALSE;
	}

	if (gbPrepareDefaultTerminal)
	{
		// ghConWnd may be NULL (if was started for devenv.exe) or NOT NULL (after AllocConsole in *.vshost.exe)
		//_ASSERTEX(ghConWnd!=NULL && "ConWnd was not initialized");

		if (!gpDefTerm)
		{
			_ASSERTEX(gpDefTerm!=NULL);
			return FALSE;
		}

		// Parameters are stored in the registry now
		if (!isDefTermEnabled())
		{
			return FALSE;
		}

		// May be not required at all?
		#if 0
		if (FindCheckConEmuWindow())
		{
			DWORD nGuiPID;
			if (!GetWindowThreadProcessId(ghConEmuWnd, &nGuiPID) || !nGuiPID)
			{
				_ASSERTEX(FALSE && "LoadGuiMapping failed, getWindowThreadProcessId failed");
				return FALSE;
			}

			if (!::LoadGuiMapping(nGuiPID, m_GuiMapping))
			{
				_ASSERTEX(FALSE && "LoadGuiMapping failed");
				return FALSE;
			}

			*gpDefaultTermParm = m_GuiMapping;

			// Checking loaded settings
			if (!isDefTermEnabled())
			{
				return FALSE; // disabled now
			}
		}
		#endif

		const CEDefTermOpt* pOpt = gpDefTerm->GetOpt();
		_ASSERTE(pOpt!=NULL); // Can't be null because it returns the pointer to member variable

		ZeroStruct(m_SrvMapping);
		m_SrvMapping.cbSize = sizeof(m_SrvMapping);

		m_SrvMapping.nProtocolVersion = CESERVER_REQ_VER;
		m_SrvMapping.nGuiPID = 0; //nGuiPID;
		m_SrvMapping.hConEmuRoot = NULL; //ghConEmuWnd;
		m_SrvMapping.hConEmuWndDc = NULL;
		m_SrvMapping.hConEmuWndBack = NULL;
		m_SrvMapping.Flags = pOpt->nConsoleFlags;
		m_SrvMapping.bUseInjects = pOpt->bNoInjects ? 0 : 1;
		// Пути
		lstrcpy(m_SrvMapping.sConEmuExe, pOpt->pszConEmuExe ? pOpt->pszConEmuExe : L"");
		lstrcpy(m_SrvMapping.ComSpec.ConEmuBaseDir, pOpt->pszConEmuBaseDir ? pOpt->pszConEmuBaseDir : L"");
		lstrcpy(m_SrvMapping.ComSpec.ConEmuExeDir, pOpt->pszConEmuExe ? pOpt->pszConEmuExe : L"");
		wchar_t* pszSlash = wcsrchr(m_SrvMapping.ComSpec.ConEmuExeDir, L'\\');
		if (pszSlash) *pszSlash = 0;

		_ASSERTE(m_SrvMapping.ComSpec.ConEmuExeDir[0] && m_SrvMapping.ComSpec.ConEmuBaseDir[0]);

		return TRUE;
	}

	if (!m_SrvMapping.cbSize || (m_SrvMapping.hConEmuWndDc && !IsWindow(m_SrvMapping.hConEmuWndDc)))
	{
		if (!::LoadSrvMapping(ghConWnd, m_SrvMapping))
			return FALSE;
		_ASSERTE(m_SrvMapping.ComSpec.ConEmuExeDir[0] && m_SrvMapping.ComSpec.ConEmuBaseDir[0]);
	}

	if (!m_SrvMapping.hConEmuWndDc || !IsWindow(m_SrvMapping.hConEmuWndDc))
		return FALSE;

	return TRUE;
}

CESERVER_REQ* CShellProc::NewCmdOnCreate(enum CmdOnCreateType aCmd,
				LPCWSTR asAction, LPCWSTR asFile, LPCWSTR asParam, LPCWSTR asDir,
				DWORD* anShellFlags, DWORD* anCreateFlags, DWORD* anStartFlags, DWORD* anShowCmd,
				int mn_ImageBits, int mn_ImageSubsystem,
				HANDLE hStdIn, HANDLE hStdOut, HANDLE hStdErr
				/*wchar_t (&szBaseDir)[MAX_PATH+2], BOOL& bDosBoxAllowed*/)
{
	//szBaseDir[0] = 0;

	// Проверим, а надо ли?
	if (!LoadSrvMapping())
		return NULL;

	//	bDosBoxAllowed = pInfo->bDosBox;
	//	wcscpy_c(szBaseDir, pInfo->sConEmuBaseDir);
	//	wcscat_c(szBaseDir, L"\\");
	if (m_SrvMapping.nLoggingType != glt_Processes)
		return NULL;

	CEStr lsDir;

	return ExecuteNewCmdOnCreate(&m_SrvMapping, ghConWnd, aCmd,
				asAction, asFile, asParam,
				(asDir && *asDir) ? asDir : GetDirectory(lsDir),
				anShellFlags, anCreateFlags, anStartFlags, anShowCmd,
				mn_ImageBits, mn_ImageSubsystem,
				hStdIn, hStdOut, hStdErr);

	////DWORD dwGuiProcessId = 0;
	////if (!ghConEmuWnd || !GetWindowThreadProcessId(ghConEmuWnd, &dwGuiProcessId))
	////	return NULL;
	//
	////MFileMapping<ConEmuGuiMapping> GuiInfoMapping;
	////GuiInfoMapping.InitName(CEGUIINFOMAPNAME, dwGuiProcessId);
	////const ConEmuGuiMapping* pInfo = GuiInfoMapping.Open();
	////if (!pInfo)
	////	return NULL;
	////else if (pInfo->nProtocolVersion != CESERVER_REQ_VER)
	////	return NULL;
	////else
	////{
	////	bDosBoxAllowed = pInfo->bDosBox;
	////	wcscpy_c(szBaseDir, pInfo->sConEmuBaseDir);
	////	wcscat_c(szBaseDir, L"\\");
	////	if (pInfo->nLoggingType != glt_Processes)
	////		return NULL;
	////}
	////GuiInfoMapping.CloseMap();
	//
	//
	//CESERVER_REQ *pIn = NULL;
	//
	//int nActionLen = (asAction ? lstrlen(asAction) : 0)+1;
	//int nFileLen = (asFile ? lstrlen(asFile) : 0)+1;
	//int nParamLen = (asParam ? lstrlen(asParam) : 0)+1;
	//
	//pIn = ExecuteNewCmd(CECMD_ONCREATEPROC, sizeof(CESERVER_REQ_HDR)
	//	+sizeof(CESERVER_REQ_ONCREATEPROCESS)+(nActionLen+nFileLen+nParamLen)*sizeof(wchar_t));
	//
	//#ifdef _WIN64
	//pIn->OnCreateProc.nSourceBits = 64;
	//#else
	//pIn->OnCreateProc.nSourceBits = 32;
	//#endif
	////pIn->OnCreateProc.bUnicode = TRUE;
	//pIn->OnCreateProc.nImageSubsystem = mn_ImageSubsystem;
	//pIn->OnCreateProc.nImageBits = mn_ImageBits;
	//pIn->OnCreateProc.hStdIn = (unsigned __int64)hStdIn;
	//pIn->OnCreateProc.hStdOut = (unsigned __int64)hStdOut;
	//pIn->OnCreateProc.hStdErr = (unsigned __int64)hStdErr;
	//
	//if (aCmd == eShellExecute)
	//	wcscpy_c(pIn->OnCreateProc.sFunction, L"Shell");
	//else if (aCmd == eCreateProcess)
	//	wcscpy_c(pIn->OnCreateProc.sFunction, L"Create");
	//else if (aCmd == eInjectingHooks)
	//	wcscpy_c(pIn->OnCreateProc.sFunction, L"Hooks");
	//else if (aCmd == eHooksLoaded)
	//	wcscpy_c(pIn->OnCreateProc.sFunction, L"Loaded");
	//else if (aCmd == eParmsChanged)
	//	wcscpy_c(pIn->OnCreateProc.sFunction, L"Changed");
	//else if (aCmd == eLoadLibrary)
	//	wcscpy_c(pIn->OnCreateProc.sFunction, L"LdLib");
	//else if (aCmd == eFreeLibrary)
	//	wcscpy_c(pIn->OnCreateProc.sFunction, L"FrLib");
	//else
	//	wcscpy_c(pIn->OnCreateProc.sFunction, L"Unknown");
	//
	//pIn->OnCreateProc.nShellFlags = anShellFlags ? *anShellFlags : 0;
	//pIn->OnCreateProc.nCreateFlags = anCreateFlags ? *anCreateFlags : 0;
	//pIn->OnCreateProc.nStartFlags = anStartFlags ? *anStartFlags : 0;
	//pIn->OnCreateProc.nShowCmd = anShowCmd ? *anShowCmd : 0;
	//pIn->OnCreateProc.nActionLen = nActionLen;
	//pIn->OnCreateProc.nFileLen = nFileLen;
	//pIn->OnCreateProc.nParamLen = nParamLen;
	//
	//wchar_t* psz = pIn->OnCreateProc.wsValue;
	//if (nActionLen > 1)
	//	_wcscpy_c(psz, nActionLen, asAction);
	//psz += nActionLen;
	//if (nFileLen > 1)
	//	_wcscpy_c(psz, nFileLen, asFile);
	//psz += nFileLen;
	//if (nParamLen > 1)
	//	_wcscpy_c(psz, nParamLen, asParam);
	//psz += nParamLen;
	//
	//return pIn;
}

void CShellProc::CheckHooksDisabled()
{
	bool bHooksTempDisabled = false;
	bool bHooksSkipNewConsole = false;
	bool bHooksSkipCmdStart = false;

	wchar_t szVar[32] = L"";
	if (GetEnvironmentVariable(ENV_CONEMU_HOOKS_W, szVar, countof(szVar)))
	{
		CharUpperBuff(szVar, lstrlen(szVar));

		bHooksTempDisabled = (wcsstr(szVar, ENV_CONEMU_HOOKS_DISABLED) != NULL);

		bHooksSkipNewConsole = (wcsstr(szVar, ENV_CONEMU_HOOKS_NOARGS) != NULL)
			|| (m_SrvMapping.cbSize && !(m_SrvMapping.Flags & CECF_ProcessNewCon));

		bHooksSkipCmdStart = (wcsstr(szVar, ENV_CONEMU_HOOKS_NOSTART) != NULL)
			|| (m_SrvMapping.cbSize && !(m_SrvMapping.Flags & CECF_ProcessCmdStart));
	}
	else
	{
		// When application is DefTerm host, it must be able to process "-new_console" switches.
		// Especially the "-new_console:z" switch. But the m_SrvMapping is not filled in most cases.
		bHooksSkipNewConsole = !gbPrepareDefaultTerminal
			&& (m_SrvMapping.cbSize && !(m_SrvMapping.Flags & CECF_ProcessNewCon));
		// "start /?" is a "cmd.exe" feature mainly. User may want to disable "start" processing
		bHooksSkipCmdStart = (m_SrvMapping.cbSize && !(m_SrvMapping.Flags & CECF_ProcessCmdStart));
	}

	mb_Opt_DontInject = bHooksTempDisabled;
	mb_Opt_SkipNewConsole = bHooksSkipNewConsole;
	mb_Opt_SkipCmdStart = bHooksSkipCmdStart;
}

BOOL CShellProc::ChangeExecuteParms(enum CmdOnCreateType aCmd, bool bConsoleMode,
				LPCWSTR asFile, LPCWSTR asParam, LPCWSTR asExeFile,
				ChangeExecFlags Flags, const RConStartArgs& args,
				DWORD& ImageBits, DWORD& ImageSubsystem,
				LPWSTR* psFile, LPWSTR* psParam)
{
	if (!LoadSrvMapping())
		return FALSE;

	BOOL lbRc = FALSE;
	wchar_t *szComspec = NULL;
	wchar_t *pszOurExe = NULL; // ConEmuC64.exe или ConEmu64.exe (для DefTerm)
	BOOL lbUseDosBox = FALSE;
	size_t cchDosBoxExe = MAX_PATH+16, cchDosBoxCfg = MAX_PATH+16;
	wchar_t *szDosBoxExe = NULL, *szDosBoxCfg = NULL;
	BOOL lbComSpec = FALSE; // TRUE - если %COMSPEC% отбрасывается
	int nCchSize = 0;
	BOOL lbEndQuote = FALSE, lbCheckEndQuote = FALSE;
	#if 0
	bool lbNewGuiConsole = false;
	#endif
	bool lbNewConsoleFromGui = false;
	BOOL lbComSpecK = FALSE; // TRUE - если нужно запустить /K, а не /C
	CEStr szDefTermArg, szDefTermArg2;

	_ASSERTEX(m_SrvMapping.sConEmuExe[0]!=0 && m_SrvMapping.ComSpec.ConEmuBaseDir[0]!=0);
	if (gbPrepareDefaultTerminal)
	{
		_ASSERTEX(ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI || ImageSubsystem == IMAGE_SUBSYSTEM_BATCH_FILE);
	}
	_ASSERTE(aCmd==eShellExecute || aCmd==eCreateProcess);

	LPCWSTR pszConEmuBaseDir = m_SrvMapping.ComSpec.ConEmuBaseDir;

	if (aCmd == eCreateProcess)
	{
		if (asFile && !*asFile)
			asFile = NULL;

		// Кто-то может додуматься окавычить asFile
		wchar_t* pszFileUnquote = NULL;
		if (asFile && (*asFile == L'"'))
		{
			pszFileUnquote = lstrdup(asFile+1);
			asFile = pszFileUnquote;
			pszFileUnquote = wcschr(pszFileUnquote, L'"');
			if (pszFileUnquote)
				*pszFileUnquote = 0;
		}

		// Для простоты - сразу откинем asFile если он совпадает с первым аргументом в asParam
		if (asFile && *asFile && asParam && *asParam)
		{
			LPCWSTR pszParam = SkipNonPrintable(asParam);
			if (pszParam && ((*pszParam != L'"') || (pszParam[0] == L'"' && pszParam[1] != L'"')))
			{
				//BOOL lbSkipEndQuote = FALSE;
				//if (*pszParam == L'"')
				//{
				//	pszParam++;
				//	lbSkipEndQuote = TRUE;
				//}
				int nLen = lstrlen(asFile)+1;
				int cchMax = max(nLen,(MAX_PATH+1));
				wchar_t* pszTest = (wchar_t*)malloc(cchMax*sizeof(wchar_t));
				_ASSERTE(pszTest);
				if (pszTest)
				{
					// Сначала пробуем на полное соответствие
					if (asFile)
					{
						_wcscpyn_c(pszTest, nLen, (*pszParam == L'"') ? (pszParam+1) : pszParam, nLen); //-V501
						pszTest[nLen-1] = 0;
						// Сравнить asFile с первым аргументом в asParam
						if (lstrcmpi(pszTest, asFile) == 0)
						{
							// exe-шник уже указан в asParam, добавлять дополнительно НЕ нужно
							asFile = NULL;
						}
					}
					// Если не сошлось - в asParam может быть только имя или часть пути запускаемого файла
					// например CreateProcess(L"C:\\GCC\\mingw32-make.exe", L"mingw32-make.exe makefile",...)
					// или      CreateProcess(L"C:\\GCC\\mingw32-make.exe", L"mingw32-make makefile",...)
					// или      CreateProcess(L"C:\\GCC\\mingw32-make.exe", L"\\GCC\\mingw32-make.exe makefile",...)
					// Эту часть нужно выкинуть из asParam
					if (asFile)
					{
						LPCWSTR pszFileOnly = PointToName(asFile);

						if (pszFileOnly)
						{
							LPCWSTR pszCopy = pszParam;
							CEStr  szFirst;
							if (NextArg(&pszCopy, szFirst) != 0)
							{
								_ASSERTE(FALSE && "NextArg failed?");
							}
							else
							{
								LPCWSTR pszFirstName = PointToName(szFirst);
								// Сравнить asFile с первым аргументом в asParam
								if (lstrcmpi(pszFirstName, pszFileOnly) == 0)
								{
									// exe-шник уже указан в asParam, добавлять дополнительно НЕ нужно
									// -- asFile = NULL; -- трогать нельзя, только он содержит полный путь!
									asParam = pszCopy;
									pszFileOnly = NULL;
								}
								else
								{
									// Пробуем asFile без расширения
									wchar_t szTmpFileOnly[MAX_PATH+1]; szTmpFileOnly[0] = 0;
									_wcscpyn_c(szTmpFileOnly, countof(szTmpFileOnly), pszFileOnly, countof(szTmpFileOnly)); //-V501
									wchar_t* pszExt = wcsrchr(szTmpFileOnly, L'.');
									if (pszExt)
									{
										*pszExt = 0;
										if (lstrcmpi(pszFirstName, szTmpFileOnly) == 0)
										{
											// exe-шник уже указан в asParam, добавлять дополнительно НЕ нужно
											// -- asFile = NULL; -- трогать нельзя, только он содержит полный путь!
											asParam = pszCopy;
											pszFileOnly = NULL;
										}
										else if (wcsrchr(szFirst, L'.'))
										{
											if (lstrcmpi(pszFirstName, szTmpFileOnly) == 0)
											{
												// exe-шник уже указан в asParam, добавлять дополнительно НЕ нужно
												// -- asFile = NULL; -- трогать нельзя, только он содержит полный путь!
												asParam = pszCopy;
												pszFileOnly = NULL;
											}
										}
									}
								}
							}
						}
					}

					free(pszTest);
				}
			}
		}

		SafeFree(pszFileUnquote);
	}

	//szComspec = (wchar_t*)calloc(cchComspec,sizeof(*szComspec));
	szComspec = GetComspec(&m_SrvMapping.ComSpec);
	if (!szComspec || !*szComspec)
	{
		_ASSERTE(szComspec && *szComspec);
		goto wrap;
	}

	// Если удалось определить "ComSpec"
	// Не менять ком.строку, если запускается "Default Terminal"
	if (!gbPrepareDefaultTerminal && asParam)
	{
		BOOL lbNewCmdCheck = FALSE;
		const wchar_t* psz = SkipNonPrintable(asParam);

		// Если запускают cmd.exe без параметров - не отбрасывать его!
		if (psz && *psz && szComspec[0])
		{
			// asFile может быть и для ShellExecute и для CreateProcess, проверим в одном месте
			if (asFile && (lstrcmpi(szComspec, asFile) == 0))
			{
				if (psz[0] == L'/' && wcschr(L"CcKk", psz[1]))
				{
					if (Flags & CEF_NEWCON_SWITCH)
					{
						// 111211 - "-new_console" передается в GUI
						lbComSpecK = FALSE;
					}
					else
					{
						// не добавлять в измененную команду asFile (это отбрасываемый cmd.exe)
						lbComSpecK = (psz[1] == L'K' || psz[1] == L'k');
						asFile = NULL;
						asParam = SkipNonPrintable(psz+2); // /C или /K добавляется к ConEmuC.exe
						lbNewCmdCheck = FALSE;

						ms_ExeTmp.Set(szComspec);
						DWORD nCheckSybsystem1 = 0, nCheckBits1 = 0;
						if (FindImageSubsystem(ms_ExeTmp, nCheckSybsystem1, nCheckBits1))
						{
							ImageSubsystem = nCheckSybsystem1;
							ImageBits = nCheckBits1;

						}
						else
						{
							_ASSERTEX(FALSE && "Can't determine ComSpec subsystem");
						}
					}

					lbComSpec = TRUE;
				}
			}
			else if ((aCmd == eCreateProcess) && !asFile)
			{
				// Теперь обработка для CreateProcess.
				// asFile уже сброшен в NULL, если он совпадает с первым аргументом в asParam
				// Возможны два варианта asParam (как минимум)
				// "c:\windows\system32\cmd.exe" /c dir
				// "c:\windows\system32\cmd.exe /c dir"
				// Второй - пока проигнорируем, как маловероятный
				INT_PTR nLen = lstrlen(szComspec)+1;
				wchar_t* pszTest = (wchar_t*)malloc(nLen*sizeof(wchar_t));
				_ASSERTE(pszTest);
				if (pszTest)
				{
					_wcscpyn_c(pszTest, nLen, (*psz == L'"') ? (psz+1) : psz, nLen); //-V501
					pszTest[nLen-1] = 0;
					// Сравнить первый аргумент в asParam с %COMSPEC%
					const wchar_t* pszCmdLeft = NULL;
					if (lstrcmpi(pszTest, szComspec) == 0)
					{
						if (*psz == L'"')
						{
							if (psz[nLen] == L'"')
							{
								pszCmdLeft = psz+nLen+1;
							}
						}
						else
						{
							pszCmdLeft = psz+nLen-1;
						}

						// Теперь нужно проверить, что там в хвосте команды (если команды нет - оставить cmd.exe)
						pszCmdLeft = SkipNonPrintable(pszCmdLeft);
						if (pszCmdLeft && pszCmdLeft[0] == L'/' && wcschr(L"CcKk", pszCmdLeft[1]))
						{
							// не добавлять в измененную команду asFile (это отбрасываемый cmd.exe)
							lbComSpecK = (psz[1] == L'K' || psz[1] == L'k');
							_ASSERTE(asFile == NULL); // уже должен быть NULL
							asParam = pszCmdLeft+2; // /C или /K добавляется к ConEmuC.exe
							lbNewCmdCheck = TRUE;

							lbComSpec = TRUE;
						}
					}
					free(pszTest);
				}
			}
		}

		if (lbNewCmdCheck)
		{
			BOOL lbRootIsCmdExe = FALSE, lbAlwaysConfirmExit = FALSE, lbAutoDisableConfirmExit = FALSE;
			BOOL lbNeedCutStartEndQuot = FALSE;
			//DWORD nFileAttrs = (DWORD)-1;
			ms_ExeTmp.Empty();
			IsNeedCmd(false, SkipNonPrintable(asParam), ms_ExeTmp, NULL, &lbNeedCutStartEndQuot, &lbRootIsCmdExe, &lbAlwaysConfirmExit, &lbAutoDisableConfirmExit);
			// это может быть команда ком.процессора!
			// поэтому, наверное, искать и проверять битность будем только для
			// файлов с указанным расширением.
			// cmd.exe /c echo -> НЕ искать
			// cmd.exe /c echo.exe -> можно и поискать
			if (ms_ExeTmp[0] && (wcschr(ms_ExeTmp, L'.') || wcschr(ms_ExeTmp, L'\\')))
			{
				bool bSkip = false;
				LPCWSTR pszExeName = PointToName(ms_ExeTmp);
				// По хорошему, нужно с полным путем проверять,
				// но если кто-то положил гуевый cmd.exe - ССЗБ
				if (lstrcmpi(pszExeName, L"cmd.exe") == 0)
				{
					ImageSubsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
					switch (m_SrvMapping.ComSpec.csBits)
					{
					case csb_SameOS:
						ImageBits = IsWindows64() ? 64 : 32;
						break;
					case csb_x32:
						ImageBits = 32;
						break;
					default:
					//case csb_SameApp:
						ImageBits = WIN3264TEST(32,64);
						break;
					}
					bSkip = true;
				}

				if (!bSkip)
				{
					DWORD nCheckSybsystem = 0, nCheckBits = 0;
					if (FindImageSubsystem(ms_ExeTmp, nCheckSybsystem, nCheckBits))
					{
						ImageSubsystem = nCheckSybsystem;
						ImageBits = nCheckBits;
					}
				}
			}
		}
	}



	lbUseDosBox = FALSE;
	szDosBoxExe = (wchar_t*)calloc(cchDosBoxExe, sizeof(*szDosBoxExe));
	szDosBoxCfg = (wchar_t*)calloc(cchDosBoxCfg, sizeof(*szDosBoxCfg));
	if (!szDosBoxExe || !szDosBoxCfg)
	{
		_ASSERTE(szDosBoxExe && szDosBoxCfg);
		goto wrap;
	}

	if ((ImageBits != 16) && lbComSpec && asParam && *asParam)
	{
		int nLen = lstrlen(asParam);

		// Может это запускается Dos-приложение через "cmd /c ..."?
		if (ImageSubsystem != IMAGE_SUBSYSTEM_DOS_EXECUTABLE)
		{
			LPCWSTR pszCmdLine = asParam;

			ms_ExeTmp.Empty();
			if (NextArg(&pszCmdLine, ms_ExeTmp) == 0)
			{
				LPCWSTR pszExt = PointToExt(ms_ExeTmp);
				if (pszExt && (lstrcmpi(pszExt, L".exe") == 0 || lstrcmpi(pszExt, L".com") == 0))
				{
					DWORD nCheckSybsystem = 0, nCheckBits = 0;
					if (FindImageSubsystem(ms_ExeTmp, nCheckSybsystem, nCheckBits))
					{
						if (nCheckSybsystem == IMAGE_SUBSYSTEM_DOS_EXECUTABLE && nCheckBits == 16)
						{
							ImageSubsystem = nCheckSybsystem;
							ImageBits = nCheckBits;
							_ASSERTEX(asFile==NULL);
						}
					}
				}
			}
		}
	}

	if (gbPrepareDefaultTerminal)
	{
		lbUseDosBox = FALSE; // Don't set now
		if (bConsoleMode)
		{
			// Тут предполагается запуск как "runas", запускаемся через "ConEmuC.exe"
			pszOurExe = lstrmerge(m_SrvMapping.ComSpec.ConEmuBaseDir, L"\\", (ImageBits == 32) ? L"ConEmuC.exe" : L"ConEmuC64.exe"); //-V112
		}
		else
		{
			pszOurExe = lstrdup(m_SrvMapping.sConEmuExe);
		}
	}
	else if ((ImageBits == 32) || (ImageBits == 64)) //-V112
	{
		pszOurExe = lstrmerge(m_SrvMapping.ComSpec.ConEmuBaseDir, L"\\", (ImageBits == 32) ? L"ConEmuC.exe" : L"ConEmuC64.exe");
	}
	else if (ImageBits == 16)
	{
		if (m_SrvMapping.cbSize && (m_SrvMapping.Flags & CECF_DosBox))
		{
			wcscpy_c(szDosBoxExe, m_SrvMapping.ComSpec.ConEmuBaseDir);
			wcscat_c(szDosBoxExe, L"\\DosBox\\DosBox.exe");
			wcscpy_c(szDosBoxCfg, m_SrvMapping.ComSpec.ConEmuBaseDir);
			wcscat_c(szDosBoxCfg, L"\\DosBox\\DosBox.conf");

			if (!FileExists(szDosBoxExe) || !FileExists(szDosBoxCfg))
			{
				// DoxBox не установлен!
				lbRc = FALSE;
				//return FALSE;
				goto wrap;
			}

			// DosBox.exe - 32-битный
			pszOurExe = lstrmerge(m_SrvMapping.ComSpec.ConEmuBaseDir, L"\\", L"ConEmuC.exe");
			lbUseDosBox = TRUE;
		}
		else
		{
			// в любом разе нужно запускать через ConEmuC.exe, чтобы GUI мог точно знать, когда 16бит приложение завершится
			// Но вот в 64битных OS - ntvdm отсутствует, так что (если DosBox-а нет), дергаться не нужно
			if (IsWindows64())
			{
				lbRc = FALSE;
				//return FALSE;
				goto wrap;
			}
			else
			{
				pszOurExe = lstrmerge(m_SrvMapping.ComSpec.ConEmuBaseDir, L"\\", L"ConEmuC.exe");
			}
		}
	}
	else
	{
		// Если не смогли определить что это и как запускается - лучше не трогать
		_ASSERTE(ImageBits==16||ImageBits==32||ImageBits==64);
		lbRc = FALSE;
		goto wrap;
	}

	if (!pszOurExe)
	{
		_ASSERTE(pszOurExe!=NULL);
		lbRc = FALSE;
		goto wrap;
	}

	nCchSize = (asFile ? lstrlen(asFile) : 0) + (asParam ? lstrlen(asParam) : 0) + 64;
	if (lbUseDosBox)
	{
		// Может быть нужно экранирование кавычек или еще чего, зарезервируем буфер
		// ну и сами параметры для DosBox
		nCchSize = (nCchSize*2) + lstrlen(szDosBoxExe) + lstrlen(szDosBoxCfg) + 128 + MAX_PATH*2/*на cd и прочую фигню*/;
	}

	if (!FileExists(pszOurExe))
	{
		wchar_t szInfo[MAX_PATH+128], szTitle[64];
		wcscpy_c(szInfo, L"ConEmu executable not found!\n");
		wcscat_c(szInfo, pszOurExe);
		msprintf(szTitle, countof(szTitle), L"ConEmuHk, PID=%i", GetCurrentProcessId());
		GuiMessageBox(ghConEmuWnd, szInfo, szTitle, MB_ICONSTOP);
	}

	if (aCmd == eShellExecute)
	{
		*psFile = lstrdup(pszOurExe);
	}
	else
	{
		nCchSize += lstrlen(pszOurExe)+1;
		*psFile = NULL;
	}

	#if 0
	// Если запускается новый GUI как вкладка?
	lbNewGuiConsole = (ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI) || (ghAttachGuiClient != NULL);
	#endif

	// Starting CONSOLE application from GUI tab? This affect "Default terminal" too.
	lbNewConsoleFromGui = (ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI) && (gbPrepareDefaultTerminal || mb_isCurrentGuiClient);

	#if 0
	if (lbNewGuiConsole)
	{
		// Нужно еще добавить /ATTACH /GID=%i,  и т.п.
		nCchSize += 128;
	}
	#endif
	if (lbNewConsoleFromGui)
	{
		// Нужно еще добавить /ATTACH /GID=%i,  и т.п.
		nCchSize += 128;
	}
	if (args.InjectsDisable == crb_On)
	{
		// добавить " /NOINJECT"
		nCchSize += 12;
	}

	if (gFarMode.cbSize && gFarMode.bFarHookMode)
	{
		// Добавить /PARENTFARPID=%u
		nCchSize += 32;
	}

	if (gbPrepareDefaultTerminal)
	{
		// reserve space for Default terminal arguments (server additional)
		// "runas" (Shell) - запуск сервера
		// CreateProcess - запуск через GUI (ConEmu.exe)
		_ASSERTEX(aCmd==eCreateProcess || aCmd==eShellExecute);
		nCchSize += gpDefTerm->GetSrvAddArgs(!bConsoleMode, szDefTermArg, szDefTermArg2);
	}

	if (Flags & CEF_NEWCON_PREPEND)
	{
		nCchSize += 16; // + "-new_console " for "start" processing
	}

	// В ShellExecute необходимо "ConEmuC.exe" вернуть в psFile, а для CreatePocess - в psParam
	// /C или /K в обоих случаях нужно пихать в psParam
	_ASSERTE(lbEndQuote == FALSE); // Must not be set yet
	*psParam = (wchar_t*)malloc(nCchSize*sizeof(wchar_t));
	(*psParam)[0] = 0;
	if (aCmd == eCreateProcess)
	{
		(*psParam)[0] = L'"';
		_wcscpy_c((*psParam)+1, nCchSize-1, pszOurExe);
		_wcscat_c((*psParam), nCchSize, L"\"");
	}


	if (aCmd == eShellExecute)
	{
		// C:\Windows\system32\cmd.exe /C ""F:\Batches\!!Save&SetNewCFG.cmd" "
		lbEndQuote = (asFile && *asFile); // иначе некоторые имена обрабатываются некорректно
	}
	else if (aCmd == eCreateProcess)
	{
		// as_Param: "C:\test.cmd" "c:\my documents\test.txt"
		// Если это не окавычить - cmd.exe отрежет первую и последнюю, и обломается
		lbEndQuote = (asFile && *asFile == L'"') || (!asFile && asParam && *asParam == L'"');
		// But don't put excess quotas
		lbCheckEndQuote = TRUE;
	}

	if (lbUseDosBox)
		_wcscat_c((*psParam), nCchSize, L" /DOSBOX");

	if (gFarMode.cbSize && gFarMode.bFarHookMode)
	{
		_ASSERTEX(!gbPrepareDefaultTerminal);
		// Добавить /PARENTFAR=%u
		wchar_t szParentFar[64];
		msprintf(szParentFar, countof(szParentFar), L" /PARENTFARPID=%u", GetCurrentProcessId());
		_wcscat_c((*psParam), nCchSize, szParentFar);
	}

	// Don't add when gbPrepareDefaultTerminal - we are calling "ConEmu.exe", not "ConEmuC.exe"
	if ((args.InjectsDisable == crb_On) && !gbPrepareDefaultTerminal)
	{
		// добавить " /NOINJECT"
		_wcscat_c((*psParam), nCchSize, L" /NOINJECT");
	}

	if (gbPrepareDefaultTerminal)
	{
		if (!szDefTermArg.IsEmpty())
		{
			_wcscat_c((*psParam), nCchSize, szDefTermArg);
		}

		if (bConsoleMode)
		{
			// Здесь предполагается "runas", поэтому запускается сервер (ConEmuC.exe)
			_wcscat_c((*psParam), nCchSize, L" /ATTACHDEFTERM /ROOT ");
		}
		else
		{
			// Starting GUI
			_wcscat_c((*psParam), nCchSize, L" -run ");
		}

		if (!szDefTermArg2.IsEmpty())
		{
			_wcscat_c((*psParam), nCchSize, szDefTermArg2);
		}
	}
	// 111211 - "-new_console" передается в GUI
	else if (lbNewConsoleFromGui)
	{
		// Нужно еще добавить /ATTACH /GID=%i,  и т.п.
		int nCurLen = lstrlen(*psParam);
		msprintf((*psParam) + nCurLen, nCchSize - nCurLen, L" /ATTACH /GID=%u /GHWND=%08X /ROOT ",
			m_SrvMapping.nGuiPID, (DWORD)m_SrvMapping.hConEmuRoot);
		TODO("Наверное, хорошо бы обработать /K|/C? Если консольное запускается из GUI");
	}
	else
	{
		_wcscat_c((*psParam), nCchSize, lbComSpecK ? L" /K " : L" /C ");
		// If was used "start" from cmd prompt or batch
		if (Flags & CEF_NEWCON_PREPEND)
			_wcscat_c((*psParam), nCchSize, L"-new_console ");
	}
	if (asParam && *asParam == L' ')
		asParam++;

	WARNING("###: Перенести обработку параметров DosBox в ConEmuC!");
	if (lbUseDosBox)
	{
		_ASSERTEX(!gbPrepareDefaultTerminal);
		lbEndQuote = TRUE; lbCheckEndQuote = FALSE;
		_wcscat_c((*psParam), nCchSize, L"\"\"");
		_wcscat_c((*psParam), nCchSize, szDosBoxExe);
		_wcscat_c((*psParam), nCchSize, L"\" -noconsole ");
		_wcscat_c((*psParam), nCchSize, L" -conf \"");
		_wcscat_c((*psParam), nCchSize, szDosBoxCfg);
		_wcscat_c((*psParam), nCchSize, L"\" ");
		_wcscat_c((*psParam), nCchSize, L" -c \"");
		//_wcscat_c((*psParam), nCchSize, L" \"");
		// исполняемый файл (если есть, может быть только в asParam)
		if (asFile && *asFile)
		{
			LPCWSTR pszRunFile = asFile;
			wchar_t* pszShort = GetShortFileNameEx(asFile);
			if (pszShort)
				pszRunFile = pszShort;
			LPCWSTR pszSlash = wcsrchr(pszRunFile, L'\\');
			if (pszSlash)
			{
				if (pszRunFile[1] == L':')
				{
					_wcscatn_c((*psParam), nCchSize, pszRunFile, 2);
					_wcscat_c((*psParam), nCchSize, L"\" -c \"");
				}
				_wcscat_c((*psParam), nCchSize, L"cd ");
				_wcscatn_c((*psParam), nCchSize, pszRunFile, (pszSlash-pszRunFile));
				_wcscat_c((*psParam), nCchSize, L"\" -c \"");
			}
			_wcscat_c((*psParam), nCchSize, pszRunFile);

			if (pszShort)
				free(pszShort);

			if (asParam && *asParam)
				_wcscat_c((*psParam), nCchSize, L" ");
		}
		// параметры, кавычки нужно экранировать!
		if (asParam && *asParam)
		{
			LPWSTR pszParam = NULL;
			if (!asFile || !*asFile)
			{
				// exe-шника в asFile указано НЕ было, значит он в asParam, нужно его вытащить, и сформировать команду DosBox
				BOOL lbRootIsCmdExe = FALSE, lbAlwaysConfirmExit = FALSE, lbAutoDisableConfirmExit = FALSE;
				BOOL lbNeedCutStartEndQuot = FALSE;
				ms_ExeTmp.Empty();
				IsNeedCmd(false, SkipNonPrintable(asParam), ms_ExeTmp, NULL, &lbNeedCutStartEndQuot, &lbRootIsCmdExe, &lbAlwaysConfirmExit, &lbAutoDisableConfirmExit);

				if (ms_ExeTmp[0])
				{
					LPCWSTR pszQuot = SkipNonPrintable(asParam);
					if (lbNeedCutStartEndQuot)
					{
						while (*pszQuot == L'"') pszQuot++;
						pszQuot += lstrlen(ms_ExeTmp);
						if (*pszQuot == L'"') pszQuot++;
					}
					else
					{
						pszQuot += lstrlen(ms_ExeTmp);
					}
					if (pszQuot && *pszQuot)
					{
						pszParam = lstrdup(pszQuot);
						INT_PTR nLen = lstrlen(pszParam);
						if (pszParam[nLen-1] == L'"')
							pszParam[nLen-1] = 0;
					}
					asParam = pszParam;

					LPCWSTR pszRunFile = ms_ExeTmp;
					wchar_t* pszShort = GetShortFileNameEx(ms_ExeTmp);
					if (pszShort)
						pszRunFile = pszShort;
					LPCWSTR pszSlash = wcsrchr(pszRunFile, L'\\');
					if (pszSlash)
					{
						if (pszRunFile[1] == L':')
						{
							_wcscatn_c((*psParam), nCchSize, pszRunFile, 2);
							_wcscat_c((*psParam), nCchSize, L"\" -c \"");
						}
						_wcscat_c((*psParam), nCchSize, L"cd ");
						_wcscatn_c((*psParam), nCchSize, pszRunFile, (pszSlash-pszRunFile));
						_wcscat_c((*psParam), nCchSize, L"\" -c \"");
					}
					_wcscat_c((*psParam), nCchSize, pszRunFile);

					if (pszShort)
						free(pszShort);

					if (asParam && *asParam)
						_wcscat_c((*psParam), nCchSize, L" ");
				}
			}

			wchar_t* pszDst = (*psParam)+lstrlen((*psParam));
			const wchar_t* pszSrc = SkipNonPrintable(asParam);
			while (pszSrc && *pszSrc)
			{
				if (*pszSrc == L'"')
					*(pszDst++) = L'\\';
				*(pszDst++) = *(pszSrc++);
			}
			*pszDst = 0;

			if (pszParam)
				free(pszParam);
		}

		_wcscat_c((*psParam), nCchSize, L"\" -c \"exit\" ");

		//_wcscat_c((*psParam), nCchSize, L" -noconsole ");
		//_wcscat_c((*psParam), nCchSize, L" -conf \"");
		//_wcscat_c((*psParam), nCchSize, szDosBoxCfg);
		//_wcscat_c((*psParam), nCchSize, L"\" ");

	}
	else //NOT lbUseDosBox
	{
		if (lbEndQuote && lbCheckEndQuote)
		{
			// May be double quotation will be excess?
			if (!(asFile && *asFile) && asParam && *asParam)
			{
				LPCWSTR pszTest = asParam;
				CEStr szTest;
				if (NextArg(&pszTest, szTest) == 0)
				{
					pszTest = SkipNonPrintable(pszTest);
					// Now - checks
					if (!pszTest || !*pszTest)
					{
						lbEndQuote = FALSE;
					}
				}
			}
		}

		if (lbEndQuote)
			_wcscat_c((*psParam), nCchSize, L"\"");

		if (asFile && *asFile)
		{
			if (*asFile != L'"')
				_wcscat_c((*psParam), nCchSize, L"\"");
			_wcscat_c((*psParam), nCchSize, asFile);
			if (*asFile != L'"')
				_wcscat_c((*psParam), nCchSize, L"\"");

			if (asParam && *asParam)
				_wcscat_c((*psParam), nCchSize, L" ");
		}

		if (asParam && *asParam)
		{
			// 111211 - "-new_console" передается в GUI
			#if 0
			const wchar_t* sNewConsole = L"-new_console";
			int nNewConsoleLen = lstrlen(sNewConsole);
			const wchar_t* pszNewPtr = wcsstr(asParam, sNewConsole);
			if (pszNewPtr)
			{
				if (!(((pszNewPtr == asParam) || (*(pszNewPtr-1) == L' ')) && ((pszNewPtr[nNewConsoleLen] == 0) || (pszNewPtr[nNewConsoleLen] == L' '))))
					pszNewPtr = NULL;
			}

			if ((lbNewGuiConsole) && pszNewPtr)
			{
				// Откусить "-new_console" из аргументов
				int nCurLen = lstrlen((*psParam));
				wchar_t* pszDst = (*psParam)+nCurLen;
				INT_PTR nCchLeft = nCchSize - nCurLen;
				if (pszNewPtr > asParam)
				{
					_wcscpyn_c(pszDst, nCchLeft, asParam, (pszNewPtr-asParam)+1);
					pszDst += pszNewPtr-asParam;
					*pszDst = 0;
					nCchLeft -= pszNewPtr-asParam;
				}
				_wcscpy_c(pszDst, nCchLeft, pszNewPtr+lstrlen(sNewConsole));
			}
			else
			#endif
			{
				_wcscat_c((*psParam), nCchSize, asParam);
			}
		}
	}

	if (lbEndQuote)
	{
		// [conemu_ml:254] TCC fails while executing: ""F:\program files\take command\tcc.exe" /C "alias where" "
		//--_wcscat_c((*psParam), nCchSize, L" \"");
		_wcscat_c((*psParam), nCchSize, L"\"");
	}

	// logging
	{
		int cchLen = (*psFile ? lstrlen(*psFile) : 0) + (*psParam ? lstrlen(*psParam) : 0) + 128;
		wchar_t* pszDbgMsg = (wchar_t*)calloc(cchLen, sizeof(wchar_t));
		if (pszDbgMsg)
		{
			msprintf(pszDbgMsg, cchLen, L"RunChanged(ParentPID=%u): %s <%s> <%s>\n",
				GetCurrentProcessId(),
				(aCmd == eShellExecute) ? L"Shell" : (aCmd == eCreateProcess) ? L"Create" : L"???",
				*psFile ? *psFile : L"", *psParam ? *psParam : L"");
			LogShellString(pszDbgMsg);
			free(pszDbgMsg);
		}
	}

	lbRc = TRUE;
wrap:
	if (szComspec)
		free(szComspec);
	if (pszOurExe)
		free(pszOurExe);
	if (szDosBoxExe)
		free(szDosBoxExe);
	if (szDosBoxCfg)
		free(szDosBoxCfg);
	return TRUE;
}

void CShellProc::CheckIsCurrentGuiClient()
{
	// gbAttachGuiClient is TRUE if some application (GUI subsystem) was not created window yet
	// this is, for example, CommandPromptPortable.exe
	// ghAttachGuiClient is HWND of already created GUI child client window
	mb_isCurrentGuiClient = ((gbAttachGuiClient != FALSE) || (ghAttachGuiClient != NULL));
}

// -1: if need to block execution
//  0: continue
//  1: continue, changes was made
int CShellProc::PrepareExecuteParms(
			enum CmdOnCreateType aCmd,
			LPCWSTR asAction, LPCWSTR asFile, LPCWSTR asParam, LPCWSTR asDir,
			DWORD* anShellFlags, DWORD* anCreateFlags, DWORD* anStartFlags, DWORD* anShowCmd,
			HANDLE* lphStdIn, HANDLE* lphStdOut, HANDLE* lphStdErr,
			LPWSTR* psFile, LPWSTR* psParam, LPWSTR* psStartDir)
{
	// !!! anFlags может быть NULL;
	// !!! asAction может быть NULL;
	_ASSERTE(*psFile==NULL && *psParam==NULL);
	if (!ghConEmuWndDC && !isDefTermEnabled())
		return 0; // Перехватывать только под ConEmu

	if (!asFile && !asParam)
		return 0; // That is most probably asAction="OpenNewWindow"

	// Just in case of unexpected LastError changes
	ScopedObject(CLastErrorGuard);

	CEStr szLnkExe, szLnkArg, szLnkDir;
	if (asFile && (aCmd == eShellExecute))
	{
		LPCWSTR pszExt = PointToExt(asFile);
		if (pszExt && (lstrcmpi(pszExt, L".lnk") == 0))
		{
			if (GetLinkProperties(asFile, szLnkExe, szLnkArg, szLnkDir))
			{
				_ASSERTE(asParam == NULL);
				asFile = szLnkExe.ms_Val;
				asParam = szLnkArg.ms_Val;
			}
		}
	}

	// Write Far prompt into ANSI Log File
	if (gFarMode.cbSize && CEAnsi::ghAnsiLogFile)
	{
		if ((aCmd == eShellExecute)
			&& (!lphStdOut && !lphStdErr)
			&& (anShellFlags && ((*anShellFlags) & SEE_MASK_NO_CONSOLE))
			)
		{
			CEAnsi::WriteAnsiLogFarPrompt();
		}
	}
	// cygwin/ansi prompt?
	if (CEAnsi::gnEnterPressed > 0)
	{
		CEAnsi::WriteAnsiLogW(L"\n", 1);
	}


	bool bAnsiCon = false;
	for (int i = 0; (i <= 1); i++)
	{
		LPCWSTR psz = (i == 0) ? asFile : asParam;
		if (!psz || !*psz)
			continue;

		#ifdef _DEBUG
		bool bAnsiConFound = false;
		LPCWSTR pszDbg = psz;
		ms_ExeTmp.Empty();
		if (NextArg(&pszDbg, ms_ExeTmp) == 0)
		{
			CharUpperBuff(ms_ExeTmp.ms_Val, lstrlen(ms_ExeTmp));
			if ((pszDbg = wcsstr(ms_ExeTmp, L"ANSI-LLW")) && (pszDbg[lstrlen(L"ANSI-LLW")] != L'\\'))
				bAnsiConFound = true;
			else if ((pszDbg = wcsstr(ms_ExeTmp, L"ANSICON")) && (pszDbg[lstrlen(L"ANSICON")] != L'\\'))
				bAnsiConFound = true;
		}
		#endif

		ms_ExeTmp.Empty();
		if (NextArg(&psz, ms_ExeTmp) != 0)
		{
			// AnsiCon exists in command line?
			_ASSERTEX(bAnsiConFound==false);
			continue;
		}

		CharUpperBuff(ms_ExeTmp.ms_Val, lstrlen(ms_ExeTmp));
		psz = PointToName(ms_ExeTmp);
		if ((lstrcmp(psz, L"ANSI-LLW.EXE") == 0) || (lstrcmp(psz, L"ANSI-LLW") == 0)
			|| (lstrcmp(psz, L"ANSICON.EXE") == 0) || (lstrcmp(psz, L"ANSICON") == 0))
		{
			bAnsiCon = true;
			break;
		}

		_ASSERTEX(bAnsiConFound==false);
	}

	if (bAnsiCon)
	{
		//_ASSERTEX(FALSE && "AnsiCon execution will be blocked");
		HANDLE hStdOut = GetStdHandle(STD_ERROR_HANDLE);
		LPCWSTR sErrMsg = L"\nConEmu blocks ANSICON injection\n";
		CONSOLE_SCREEN_BUFFER_INFO csbi = {sizeof(csbi)};
		if (GetConsoleScreenBufferInfo(hStdOut, &csbi) && (csbi.dwCursorPosition.X == 0))
		{
			sErrMsg++;
		}
		DWORD nLen = lstrlen(sErrMsg);
		WriteConsoleW(hStdOut, sErrMsg, nLen, &nLen, NULL);
		LogShellString(L"ANSICON was blocked");
		return -1;
	}
	ms_ExeTmp.Empty();


	BOOL bGoChangeParm = FALSE;
	bool bConsoleMode = false;
	if ((aCmd == eShellExecute) && (asAction && (lstrcmpi(asAction, L"runas") == 0)))
	{
		// Always run ConEmuC instead of GUI
		bConsoleMode = true;
	}

	// DefTerm logging
	wchar_t szInfo[140];
	#define LogExit(frc) \
		msprintf(szInfo, countof(szInfo), \
			L"PrepareExecuteParms rc=%i %u:%u:%u W:%u I:%u,%u,%u D:%u H:%u S:%u,%u line=%i", \
			(frc), \
			(UINT)mn_ImageSubsystem, (UINT)mn_ImageBits, (UINT)mb_isCurrentGuiClient, \
			(UINT)mb_WasSuspended, (UINT)mb_NeedInjects, (UINT)mb_PostInjectWasRequested, \
			(UINT)mb_Opt_DontInject, (UINT)mb_DebugWasRequested, (UINT)mb_HiddenConsoleDetachNeed, \
			(UINT)mb_Opt_SkipNewConsole, (UINT)mb_Opt_SkipCmdStart, \
			__LINE__); \
		LogShellString(szInfo);

	{ // Log PrepareExecuteParms function call -begin
	lstrcpyn(szInfo, asAction ? asAction : L"-exec-", countof(szInfo));
	struct ParmsStr { LPDWORD pVal; LPCWSTR pName; }
	Parms[] = {
		{anShellFlags, L"ShellFlags"},
		{anCreateFlags, L"CreateFlags"},
		{anStartFlags, L"StartFlags"},
		{anShowCmd, L"ShowCmd"}
	};
	for (INT_PTR i = 0; i < countof(Parms); i++)
	{
		if (!Parms[i].pVal)
			continue;
		int iLen = lstrlen(szInfo);
		msprintf(szInfo+iLen, countof(szInfo)-iLen, L" %s:x%X", Parms[i].pName, *Parms[i].pVal);
	}
	CEStr lsLog = lstrmerge(
		(aCmd == eShellExecute) ? L"PrepareExecuteParms Shell " : L"PrepareExecuteParms Create ",
		szInfo, asFile, asParam);
	LogShellString(lsLog);
	} // Log PrepareExecuteParms function call -end

	// Для логирования - запомним сразу
	HANDLE hIn  = lphStdIn  ? *lphStdIn  : NULL;
	HANDLE hOut = lphStdOut ? *lphStdOut : NULL;
	HANDLE hErr = lphStdErr ? *lphStdErr : NULL;
	// В некоторых случаях - LongConsoleOutput бессмысленен
	// ShellExecute(SW_HIDE) или CreateProcess(CREATE_NEW_CONSOLE|CREATE_NO_WINDOW|DETACHED_PROCESS,SW_HIDE)
	bool bDetachedOrHidden = false;
	bool bDontForceInjects = false;
	if (aCmd == eShellExecute)
	{
		if (!anShellFlags && anShowCmd && *anShowCmd == 0)
			bDontForceInjects = bDetachedOrHidden = true;
	}
	else if (aCmd == eCreateProcess)
	{
		// If Std console handles were defined - force run console server instead of GUI
		if ((anStartFlags && ((*anStartFlags) & STARTF_USESTDHANDLES))
			&& (hIn || hOut || hErr))
		{
			bConsoleMode = true;
		}

		// Creating hidden
		if (anShowCmd && *anShowCmd == 0)
		{
			// Historical (create process detached from parent console)
			if (anCreateFlags && (*anCreateFlags & (CREATE_NEW_CONSOLE|CREATE_NO_WINDOW|DETACHED_PROCESS)))
				bDontForceInjects = bDetachedOrHidden = true;
			// Detect creating "root" from mintty-like applications
			else if ((gbAttachGuiClient || gbGuiClientAttached) && anCreateFlags && (*anCreateFlags & (CREATE_BREAKAWAY_FROM_JOB)))
				bDontForceInjects = bDetachedOrHidden = true;
		}
		// Started with redirected output? For example, from Far cmd line:
		// edit:<git log
		if (!bDetachedOrHidden && (gnInShellExecuteEx <= 0) && (lphStdOut || lphStdErr))
		{
			if (lphStdOut && *lphStdOut)
			{
				if (!HandleKeeper::IsOutputHandle(*lphStdOut))
					bDetachedOrHidden = true;
			}
			else if (lphStdErr && *lphStdErr)
			{
				if (!HandleKeeper::IsOutputHandle(*lphStdErr))
					bDetachedOrHidden = true;
			}
			#if 0
			else
			{
				// Issue 1763: Multiarc очень странно делает вызовы архиваторов
				// в первый раз указывая хэндлы, а во второй - меняя STD_OUTPUT_HANDLE?
				HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
				//if (!HandleKeeper::IsOutputHandle(hStdOut))
				//	bDetachedOrHidden = true;
			}
			#endif
		}
	}

	// Used to automatically increase the height of the console (backscroll) buffer when starting smth from Far Manager prompt,
	// save output to our server internal buffer, and revert the height to original size.
	// Was useful until ‘Far -w’ appeared.
	BOOL bLongConsoleOutput = gFarMode.FarVer.dwVerMajor && gFarMode.bFarHookMode && gFarMode.bLongConsoleOutput && !bDetachedOrHidden;

	// Current application is GUI subsystem run in ConEmu tab?
	CheckIsCurrentGuiClient();

	ChangeExecFlags NewConsoleFlags = CEF_DEFAULT;
	bool bForceNewConsole = false, bCurConsoleArg = false;

	// Service object (moved to members: RConStartArgs m_Args)
	_ASSERTEX(m_Args.pszSpecialCmd == NULL); // Must not be touched yet!

	bool bDebugWasRequested = false;
	bool bVsNetHostRequested = false;
	bool bIgnoreSuspended = false;
	mb_DebugWasRequested = FALSE;
	mb_PostInjectWasRequested = FALSE;

	// Issue 351: После перехода исполнятеля фара на ShellExecuteEx почему-то сюда стал приходить
	//            левый хэндл (hStdOutput = 0x00010001), иногда получается 0x00060265
	//            и недокументированный флаг 0x400 в lpStartupInfo->dwFlags
	if ((aCmd == eCreateProcess) && (gnInShellExecuteEx > 0)
		&& lphStdOut && lphStdErr && anStartFlags && (*anStartFlags) == 0x401)
	{
		// Win2k, WinXP, Win2k3
		if (IsWin5family())
		{
			if (//(*lphStdOut == (HANDLE)0x00010001)
				(((DWORD_PTR)*lphStdOut) >= 0x00010000)
				&& (*lphStdErr == NULL))
			{
				*lphStdOut = NULL;
				*anStartFlags &= ~0x400;
			}
		}
	}

	// Проверяем настройку ConEmuGuiMapping.bUseInjects
	if (!LoadSrvMapping() || !(m_SrvMapping.cbSize && ((m_SrvMapping.bUseInjects & 1) || gbPrepareDefaultTerminal)))
	{
		// -- зависимо только от флажка "Use Injects", иначе нельзя управлять добавлением "ConEmuC.exe" из ConEmu --
		// Настройка в ConEmu ConEmuGuiMapping.bUseInjects, или gFarMode.bFarHookMode. Иначе - сразу выходим
		if (!bLongConsoleOutput)
		{
			LogExit(0);
			return 0;
		}
	}

	// "ConEmuHooks=OFF" - don't inject the created process
	// "ConEmuHooks=NOARG" - don't process -new_console and -cur_console args
	// "ConEmuHooks=..." - any other - all enabled
	CheckHooksDisabled();

	// We need the get executable name before some other checks
	mn_ImageSubsystem = mn_ImageBits = 0;
	// In some cases we need to pre-replace command line,
	// for example, in cmd prompt: start -new_console:z
	ms_ExeTmp.Empty();
	bool bForceCutNewConsole = false;
	CEStr lsReplaceFile, lsReplaceParm;
	if (asFile && *asFile && (wcsstr(asFile, L"-new_console") || wcsstr(asFile, L"-cur_console")))
	{
		// To be sure, it's our "-new_console" or "-cur_console" switch,
		// but not a something else...
		RConStartArgs lTestArg;
		lTestArg.pszSpecialCmd = lstrdup(asFile);
		if (lTestArg.ProcessNewConArg() > 0)
		{
			// pszSpecialCmd is supposed to be empty now, because asFile can contain only one "token"
			_ASSERTE(lTestArg.pszSpecialCmd == NULL || *lTestArg.pszSpecialCmd == 0);
			lsReplaceParm = lstrmerge(asFile, (asFile && *asFile && asParam && *asParam) ? L" " : NULL, asParam);
			_ASSERTE(!lsReplaceParm.IsEmpty());
			if (gbIsCmdProcess
				&& !GetStartingExeName(NULL, lsReplaceParm, ms_ExeTmp)
				)
			{
				// when just "start" is executed from "cmd.exe" - it starts itself in the new console
				CEStr lsTempCmd;
				if (GetModulePathName(NULL, lsTempCmd))
				{
					ms_ExeTmp.Set(PointToName(lsTempCmd));
					// It's supposed to be "cmd.exe", so there must not be spaces in file name
					_ASSERTE(!wcschr(ms_ExeTmp, L' '));
					// Insert "cmd.exe " before "-new_console" switches for clearness
					if (aCmd == eCreateProcess)
					{
						lsReplaceParm = lstrmerge(ms_ExeTmp, L" ", lsReplaceParm);
					}
				}
			}
			if (!ms_ExeTmp.IsEmpty())
			{
				lsReplaceFile.Set(ms_ExeTmp.ms_Val);
				asFile = lsReplaceFile.ms_Val;
			}
			bForceCutNewConsole = true;
			asParam = lsReplaceParm.ms_Val;
		}
	}
	if (ms_ExeTmp.IsEmpty())
		GetStartingExeName(asFile, asParam, ms_ExeTmp);

	bool lbGnuDebugger = false;

	// Some additional checks for "Default terminal" mode
	if (gbPrepareDefaultTerminal)
	{
		lbGnuDebugger = IsGDB(ms_ExeTmp); // Allow GDB in Lazarus etc.

		if (aCmd == eCreateProcess)
		{
			if (anCreateFlags && ((*anCreateFlags) & (CREATE_NO_WINDOW|DETACHED_PROCESS))
				&& !lbGnuDebugger
				)
			{
				if (gbIsVsCode
					&& (lstrcmpi(PointToName(ms_ExeTmp), L"cmd.exe") == 0))
				{
					// :: Code.exe >> "cmd /c start /wait" >> "cmd.exe"
					// :: have to hook both cmd to get second in ConEmu tab
					// :: first cmd is started with CREATE_NO_WINDOW flag!
					LogShellString(L"Forcing mb_NeedInjects for cmd.exe started from VisuaStudio Code");
					mb_NeedInjects = true;
					bConsoleMode = true;
				}

				// Creating process without console window, not our case
				LogExit(0);
				return 0;
			}
			// GDB console debugging
			if (gbIsGdbHost
				&& (anCreateFlags && ((*anCreateFlags) & (DEBUG_ONLY_THIS_PROCESS|DEBUG_PROCESS)))
				)
			{
				if (FindImageSubsystem(ms_ExeTmp, mn_ImageSubsystem, mn_ImageBits))
				{
					if (mn_ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI)
					{
						mb_DebugWasRequested = true;
						LogExit(0);
						return 0;
					}
				}
			}
		}
		else if (aCmd == eShellExecute)
		{
			// We need to hook only "Run as administrator" action, and only console applications (Subsystem will be checked below)
			if (!asAction || (lstrcmpi(asAction, L"runas") != 0))
			{
				// This may be, for example, starting of "Taskmgr.exe" from "explorer.exe"
				LogExit(0);
				return 0;
			}
			// Skip some executions
			if (anShellFlags
				&& ((*anShellFlags) & (SEE_MASK_CLASSNAME|SEE_MASK_CLASSKEY|SEE_MASK_IDLIST|SEE_MASK_INVOKEIDLIST)))
			{
				LogExit(0);
				return 0;
			}
		}
		else
		{
			_ASSERTE(FALSE && "Unsupported in Default terminal");
			LogExit(0);
			return 0;
		}

		if (anShowCmd && (*anShowCmd == SW_HIDE)
			&& !lbGnuDebugger
			)
		{
			// Creating process with window initially hidden, not our case
			LogExit(0);
			return 0;
		}

		// Started from explorer/taskmgr - CREATE_SUSPENDED is set (why?)
		if (mb_WasSuspended && anCreateFlags)
		{
			_ASSERTE(anCreateFlags && ((*anCreateFlags) & CREATE_SUSPENDED));
			_ASSERTE(aCmd == eCreateProcess);
			_ASSERTE(gbPrepareDefaultTerminal);
			// Actually, this branch can be activated from any GUI application
			// For example, Issue 1516: Dopus, notepad, etc.
			if (!((*anCreateFlags) & (DEBUG_PROCESS|DEBUG_ONLY_THIS_PROCESS)))
			{
				// Well, there is a chance, that CREATE_SUSPENDED was intended for
				// setting hooks from current process with SetThreadContext, but
				// on the other hand, we get here if only user himself activates
				// "Default terminal" feature for this process. So, I believe,
				// this decision is almost safe.
				bIgnoreSuspended = true;
			}
		}

		// Issue 1312: .Net applications runs as "CREATE_SUSPENDED" when debugging in VS
		//    Also: How to Disable the Hosting Process
		//    http://msdn.microsoft.com/en-us/library/ms185330.aspx
		if (anCreateFlags && ((*anCreateFlags) & (DEBUG_PROCESS|DEBUG_ONLY_THIS_PROCESS|(bIgnoreSuspended?0:CREATE_SUSPENDED))))
		{
			#if 0
			// Для поиска трапов в дереве запускаемых процессов
			if (m_SrvMapping.Flags & CECF_BlockChildDbg)
			{
				(*anCreateFlags) &= ~(DEBUG_PROCESS|DEBUG_ONLY_THIS_PROCESS);
			}
			else
			#endif
			{
				bDebugWasRequested = true;
			}
			// Пока продолжим, нам нужно определить, а консольное ли это приложение?
		}
	}

	// Может быть указан *.lnk а не физический файл...
	if (aCmd == eShellExecute)
	{
#if 1
		bool bDbg = 1;
#else
		// Считаем, что один файл (*.exe, *.cmd, ...) или ярлык (*.lnk)
		// это одна запускаемая консоль в ConEmu.
		CEStr szPart[MAX_PATH+1]
		wchar_t szExe[MAX_PATH+1], szArguments[32768], szDir[MAX_PATH+1];
		HRESULT hr = S_OK;
		IShellLinkW* pShellLink = NULL;
		IPersistFile* pFile = NULL;
		if (StrStrI(asSource, L".lnk"))
		{
			hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&pShellLink);
			if (FAILED(hr) || !pShellLink)
			{
				DisplayLastError(L"Can't create IID_IShellLinkW", (DWORD)hr);
				LogExit(0);
				return 0;
			}
			hr = pShellLink->QueryInterface(IID_IPersistFile, (void**)&pFile);
			if (FAILED(hr) || !pFile)
			{
				DisplayLastError(L"Can't create IID_IPersistFile", (DWORD)hr);
				pShellLink->Release();
				LogExit(0);
				return 0;
			}
		}

		// Поехали
		LPWSTR pszConsoles[MAX_CONSOLE_COUNT] = {};
		size_t cchLen, cchAllLen = 0, iCount = 0;
		while ((iCount < MAX_CONSOLE_COUNT) && (0 == NextArg(&asSource, szPart)))
		{
			if (lstrcmpi(PointToExt(szPart), L".lnk") == 0)
			{
				// Ярлык
				hr = pFile->Load(szPart, STGM_READ);
				if (SUCCEEDED(hr))
				{
					hr = pShellLink->GetPath(szExe, countof(szExe), NULL, 0);
					if (SUCCEEDED(hr) && *szExe)
					{
						hr = pShellLink->GetArguments(szArguments, countof(szArguments));
						if (FAILED(hr))
							szArguments[0] = 0;
						hr = pShellLink->GetWorkingDirectory(szDir, countof(szDir));
						if (FAILED(hr))
							szDir[0] = 0;

						cchLen = _tcslen(szExe)+3
							+ _tcslen(szArguments)+1
							+ (*szDir ? (_tcslen(szDir)+32) : 0); // + "-new_console:d<Dir>
						pszConsoles[iCount] = (wchar_t*)malloc(cchLen*sizeof(wchar_t));
						_wsprintf(pszConsoles[iCount], SKIPLEN(cchLen) L"\"%s\"%s%s",
							Unquote(szExe), *szArguments ? L" " : L"", szArguments);
						if (*szDir)
						{
							_wcscat_c(pszConsoles[iCount], cchLen, L" \"-new_console:d");
							_wcscat_c(pszConsoles[iCount], cchLen, Unquote(szDir));
							_wcscat_c(pszConsoles[iCount], cchLen, L"\"");
						}
						iCount++;

						cchAllLen += cchLen+3;
					}
				}
			}
			else
			{
				cchLen = _tcslen(szPart) + 3;
				pszConsoles[iCount] = (wchar_t*)malloc(cchLen*sizeof(wchar_t));
				_wsprintf(pszConsoles[iCount], SKIPLEN(cchLen) L"\"%s\"", szPart);
				iCount++;

				cchAllLen += cchLen+3;
			}
		}

		if (pShellLink)
			pShellLink->Release();
		if (pFile)
			pFile->Release();
#endif
	}

	//wchar_t *szTest = (wchar_t*)malloc(MAX_PATH*2*sizeof(wchar_t)); //[MAX_PATH*2]
	//wchar_t *szExe = (wchar_t*)malloc((MAX_PATH+1)*sizeof(wchar_t)); //[MAX_PATH+1];
	//DWORD /*mn_ImageSubsystem = 0, mn_ImageBits = 0,*/ nFileAttrs = (DWORD)-1;
	bool lbGuiApp = false;
	//int nActionLen = (asAction ? lstrlen(asAction) : 0)+1;
	//int nFileLen = (asFile ? lstrlen(asFile) : 0)+1;
	//int nParamLen = (asParam ? lstrlen(asParam) : 0)+1;

	if (ms_ExeTmp[0])
	{
		int nLen = lstrlen(ms_ExeTmp);
		// Длина больше 0 и не заканчивается слешом
		BOOL lbMayBeFile = (nLen > 0) && (ms_ExeTmp[nLen-1] != L'\\') && (ms_ExeTmp[nLen-1] != L'/');

		BOOL lbSubsystemOk = FALSE;
		mn_ImageBits = 0;
		mn_ImageSubsystem = IMAGE_SUBSYSTEM_UNKNOWN;

		if (!lbMayBeFile)
		{
			mn_ImageBits = 0;
			mn_ImageSubsystem = IMAGE_SUBSYSTEM_UNKNOWN;
		}
		else if (FindImageSubsystem(ms_ExeTmp, mn_ImageSubsystem, mn_ImageBits))
		{
			// gh-681: NodeJSPortable.exe just runs "Server.cmd"
			if (mn_ImageSubsystem == IMAGE_SUBSYSTEM_BATCH_FILE)
				mn_ImageSubsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
			lbGuiApp = (mn_ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI);
			lbSubsystemOk = TRUE;
			if (gbPrepareDefaultTerminal)
			{
				if (IsVsNetHostExe(ms_ExeTmp))
				{
					// *.vshost.exe
					bVsNetHostRequested = true;
					// Intended for .Net debugging?
					if (anCreateFlags && ((*anCreateFlags) & CREATE_SUSPENDED))
					{
						bDebugWasRequested = true;
					}
				}
				else if (lbGnuDebugger)
				{
					bDebugWasRequested = true;
					mb_PostInjectWasRequested = true;
				}
				else if (lstrcmpi(gsExeName, ms_ExeTmp))
				{
					// Idle from (Pythonw.exe, gh-457), VisualStudio Code (code.exe), and so on
					// -- bVsNetHostRequested = true;
					CEStr lsMsg(L"Forcing mb_NeedInjects for `", gsExeName, L"` started from `", gsExeName, L"`");
					LogShellString(lsMsg);
					mb_NeedInjects = true;
				}
			}
		}


		LPCWSTR pszName = PointToName(ms_ExeTmp);

		if (lstrcmpi(pszName, L"ANSI-LLW.exe") == 0)
		{
			_ASSERTEX(FALSE && "Trying to start 'ANSI-LLW.exe'");
		}
		else if (lstrcmpi(pszName, L"ansicon.exe") == 0)
		{
			_ASSERTEX(FALSE && "Trying to start 'ansicon.exe'");
		}
	}

	BOOL lbChanged = FALSE;
	mb_NeedInjects = FALSE;
	//wchar_t szBaseDir[MAX_PATH+2]; szBaseDir[0] = 0;
	CESERVER_REQ *pIn = NULL;
	pIn = NewCmdOnCreate(aCmd,
			asAction, asFile, asParam, asDir,
			anShellFlags, anCreateFlags, anStartFlags, anShowCmd,
			mn_ImageBits, mn_ImageSubsystem,
			hIn, hOut, hErr/*, szBaseDir, mb_DosBoxAllowed*/);
	if (pIn)
	{
		HWND hConWnd = GetRealConsoleWindow();
		CESERVER_REQ *pOut = NULL;
		pOut = ExecuteGuiCmd(hConWnd, pIn, hConWnd);
		ExecuteFreeResult(pIn); pIn = NULL;
		if (!pOut)
			goto wrap;
		if (!pOut->OnCreateProcRet.bContinue)
			goto wrap;
		ExecuteFreeResult(pOut);
	}

	// When user set env var "ConEmuHooks=OFF" - don't set hooks
	if (mb_Opt_DontInject
		// Using minhook technique "ssh-agent" is able to fork itself without problem,
		// so unconditional unhooking of "ssh-agent.exe" was removed.
		)
	{
		mb_NeedInjects = FALSE;
		goto wrap;
	}

	// logging
	{
		int cchLen = (asFile ? lstrlen(asFile) : 0) + (asParam ? lstrlen(asParam) : 0) + 128;
		wchar_t* pszDbgMsg = (wchar_t*)calloc(cchLen, sizeof(wchar_t));
		if (pszDbgMsg)
		{
			msprintf(pszDbgMsg, cchLen, L"Run(ParentPID=%u): %s <%s> <%s>\n",
				GetCurrentProcessId(),
				(aCmd == eShellExecute) ? L"Shell" : (aCmd == eCreateProcess) ? L"Create" : L"???",
				asFile ? asFile : L"", asParam ? asParam : L"");
			LogShellString(pszDbgMsg);
			free(pszDbgMsg);
		}
	}

	//wchar_t* pszExecFile = (wchar_t*)pOut->OnCreateProcRet.wsValue;
	//wchar_t* pszBaseDir = (wchar_t*)(pOut->OnCreateProcRet.wsValue); // + pOut->OnCreateProcRet.nFileLen);

	if (asParam && *asParam && !mb_Opt_SkipNewConsole)
	{
		m_Args.pszSpecialCmd = lstrdup(asParam);
		if (m_Args.ProcessNewConArg() > 0)
		{
			if (m_Args.NoDefaultTerm == crb_On)
			{
				bForceCutNewConsole = true;
				goto wrap;
			}
			else if (m_Args.NewConsole == crb_On)
			{
				NewConsoleFlags |= CEF_NEWCON_SWITCH;
			}
			else
			{
				// А вот "-cur_console" нужно обрабатывать _здесь_
				bCurConsoleArg = true;

				if ((m_Args.ForceDosBox == crb_On) && m_SrvMapping.cbSize && (m_SrvMapping.Flags & CECF_DosBox))
				{
					mn_ImageSubsystem = IMAGE_SUBSYSTEM_DOS_EXECUTABLE;
					mn_ImageBits = 16;
					bLongConsoleOutput = FALSE;
					lbGuiApp = FALSE;
				}

				if (m_Args.LongOutputDisable == crb_On)
				{
					bLongConsoleOutput = FALSE;
				}
			}
		}
	}
	// Если GUI приложение работает во вкладке ConEmu - запускать консольные приложение в новой вкладке ConEmu
	// Use mb_isCurrentGuiClient instead of ghAttachGuiClient, because of 'CommandPromptPortable.exe' for example
	if (!(NewConsoleFlags & CEF_NEWCON_SWITCH)
		&& mb_isCurrentGuiClient && (mn_ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI)
		&& ((anShowCmd == NULL) || (*anShowCmd != SW_HIDE)))
	{
		// 1. Цеплять во вкладку нужно только если консоль запускается ВИДИМОЙ
		// 2. Если запускается, например, CommandPromptPortable.exe (GUI) то подцепить запускаемый CUI в уже существующую вкладку!
		// 3. Putty with plink as proxy, plink must not get into ConEmu tab. Putty sets only CREATE_NO_WINDOW, DETACHED_PROCESS JIC.
		if (gbAttachGuiClient && !ghAttachGuiClient
			&& (anCreateFlags && !((*anCreateFlags) & (CREATE_NO_WINDOW|DETACHED_PROCESS)))
			&& (aCmd == eCreateProcess))
		{
			if (AttachServerConsole())
			{
				_ASSERTE(gnAttachPortableGuiCui==0);
				gnAttachPortableGuiCui = (GuiCui)IMAGE_SUBSYSTEM_WINDOWS_CUI;
				mb_NeedInjects = TRUE;
				bForceNewConsole = false;
				if (anCreateFlags)
				{
					_ASSERTE(((*anCreateFlags) & (CREATE_NO_WINDOW|DETACHED_PROCESS)) == 0);
					*anCreateFlags &= ~CREATE_NEW_CONSOLE;
				}
			}
		}
		else
		{
			bForceNewConsole = true;
		}
	}
	if (mb_isCurrentGuiClient && ((NewConsoleFlags & CEF_NEWCON_SWITCH) || bForceNewConsole) && !lbGuiApp)
	{
		lbGuiApp = true;
	}
	// Used with "start" for example, but ignore "start /min ..."
	if ((aCmd == eCreateProcess)
		&& !mb_Opt_SkipCmdStart // Issue 1822
		&& (anCreateFlags && (*anCreateFlags & (CREATE_NEW_CONSOLE)))
		&& !(NewConsoleFlags & CEF_NEWCON_SWITCH) && !bForceNewConsole
		&& (mn_ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI)
		&& ((anShowCmd == NULL)
			|| ((*anShowCmd != SW_HIDE)
				&& (*anShowCmd != SW_SHOWMINNOACTIVE) && (*anShowCmd != SW_SHOWMINIMIZED) && (*anShowCmd != SW_MINIMIZE))
			)
		)
	{
		*anShowCmd = SW_HIDE;
		NewConsoleFlags |= CEF_NEWCON_PREPEND;
		m_Args.NewConsole = crb_On;
	}

	if (bLongConsoleOutput)
	{
		// MultiArc issue. При поиске нефиг включать длинный буфер. Как отсечь?
		// Пока по запуску не из главного потока.
		if (GetCurrentThreadId() != gnHookMainThreadId)
			bLongConsoleOutput = FALSE;
	}

	if (aCmd == eShellExecute)
	{
		WARNING("Уточнить условие для флагов ShellExecute!");
		// !!! anFlags может быть NULL;
		DWORD nFlagsMask = (SEE_MASK_FLAG_NO_UI|SEE_MASK_NOASYNC|SEE_MASK_NOCLOSEPROCESS|SEE_MASK_NO_CONSOLE);
		DWORD nFlags = (anShellFlags ? *anShellFlags : 0) & nFlagsMask;
		if (gbPrepareDefaultTerminal)
		{
			if (!asAction || (lstrcmpiW(asAction, L"runas") != 0))
			{
				goto wrap; // хватаем только runas
			}
		}
		else
		{
			// Если bNewConsoleArg - то однозначно стартовать в новой вкладке ConEmu (GUI теперь тоже цеплять умеем)
			if (NewConsoleFlags || bForceNewConsole)
			{
				if (anShellFlags)
				{
					// 111211 - "-new_console" выполняется в GUI
					// Будет переопределение на ConEmuC, и его нужно запустить в ЭТОЙ консоли
					//--WARNING("Хотя, наверное нужно не так, чтобы он не гадил в консоль, фар ведь ждать не будет, он думает что запустил ГУЙ");
					//--*anShellFlags = (SEE_MASK_FLAG_NO_UI|SEE_MASK_NOASYNC|SEE_MASK_NOCLOSEPROCESS);
					if (anShowCmd && !(*anShellFlags & SEE_MASK_NO_CONSOLE))
					{
						*anShowCmd = SW_HIDE;
					}
				}
				else
				{
					_ASSERTE(anShellFlags!=NULL);
				}
			}
			else if ((nFlags != nFlagsMask) && !bLongConsoleOutput)
			{
				goto wrap; // пока так - это фар выполняет консольную команду
			}
			if (asAction && (lstrcmpiW(asAction, L"open") != 0))
			{
				goto wrap; // runas, print, и прочая нас не интересует
			}
		}
	}
	else
	{
		// Посмотреть, какие еще условия нужно отсеять для CreateProcess?
		DWORD nFlags = anCreateFlags ? *anCreateFlags : 0;
		if ((nFlags & (CREATE_NO_WINDOW|DETACHED_PROCESS)) != 0)
			goto wrap; // запускается по тихому (без консольного окна), пропускаем

		// Это УЖЕ может быть ConEmuC.exe
		const wchar_t* pszExeName = PointToName(ms_ExeTmp);
		if (pszExeName && (!lstrcmpi(pszExeName, L"ConEmuC.exe") || !lstrcmpi(pszExeName, L"ConEmuC64.exe")))
		{
			mb_NeedInjects = FALSE;
			goto wrap;
		}
	}

	//bool lbGuiApp = false;
	//DWORD ImageSubsystem = 0, ImageBits = 0;

	//if (!pszExecFile || !*pszExecFile)
	if (ms_ExeTmp[0] == 0)
	{
		_ASSERTE(ms_ExeTmp[0] != 0);
		goto wrap; // ошибка?
	}
	//if (GetImageSubsystem(pszExecFile,ImageSubsystem,ImageBits))
	//lbGuiApp = (ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI);

	if (lbGuiApp && !(NewConsoleFlags || bForceNewConsole || bVsNetHostRequested))
	{
		if (gbAttachGuiClient && !ghAttachGuiClient && (mn_ImageBits != 16) && (m_Args.InjectsDisable != crb_On))
		{
			_ASSERTE(gnAttachPortableGuiCui==0);
			gnAttachPortableGuiCui = (GuiCui)IMAGE_SUBSYSTEM_WINDOWS_GUI;
			mb_NeedInjects = TRUE;
		}
		goto wrap; // гуй - не перехватывать (если только не указан "-new_console")
	}

	// Подставлять ConEmuC.exe нужно только для того, чтобы
	//	1. в фаре включить длинный буфер и запомнить длинный результат в консоли (ну и ConsoleAlias обработать)
	//	2. при вызовах ShellExecute/ShellExecuteEx, т.к. не факт,
	//     что этот ShellExecute вызовет CreateProcess из kernel32 (который перехвачен).
	//     В Win7 это может быть вызов других системных модулей (App-.....dll)

	#ifdef _DEBUG
	// Для принудительной вставки ConEmuC.exe - поставить true. Только для отладки!
	bool lbAlwaysAddConEmuC; lbAlwaysAddConEmuC = false;
	#endif

	if ((mn_ImageBits == 0) && (mn_ImageSubsystem == 0)
		#ifdef _DEBUG
		&& !lbAlwaysAddConEmuC
		#endif
		)
	{
		// Это может быть запускаемый документ, например .doc, или .sln файл
		goto wrap;
	}

	_ASSERTE(mn_ImageBits!=0);

	// Если это Фар - однозначно вставляем ConEmuC.exe
	// -- bFarHookMode заменен на bLongConsoleOutput --
	if (gbPrepareDefaultTerminal)
	{
		// set up default terminal
		bGoChangeParm = ((m_Args.NoDefaultTerm != crb_On) && (bVsNetHostRequested || mn_ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI || mn_ImageSubsystem == IMAGE_SUBSYSTEM_BATCH_FILE));
	}
	else
	{
		// хотят GUI прицепить к новой вкладке в ConEmu, или новую консоль из GUI
		if (lbGuiApp && (NewConsoleFlags || bForceNewConsole))
			bGoChangeParm = true;
		// eCreateProcess перехватывать не нужно (сами сделаем InjectHooks после CreateProcess)
		else if ((mn_ImageBits != 16) && (m_SrvMapping.bUseInjects & 1)
				&& (NewConsoleFlags // CEF_NEWCON_SWITCH | CEF_NEWCON_PREPEND
					|| (bLongConsoleOutput &&
						(((gFarMode.FarVer.dwVerMajor >= 3) && (aCmd == eShellExecute) && (anShellFlags && (*anShellFlags & SEE_MASK_NO_CONSOLE)) && (anShowCmd && *anShowCmd))
						|| ((gFarMode.FarVer.dwVerMajor <= 2) && (aCmd == eCreateProcess) && (anCreateFlags && (*anCreateFlags & CREATE_DEFAULT_ERROR_MODE))))
						)
					|| (bCurConsoleArg && (m_Args.LongOutputDisable != crb_On))
					#ifdef _DEBUG
					|| lbAlwaysAddConEmuC
					#endif
					))
			bGoChangeParm = true;
		// если это Дос-приложение - то если включен DosBox, вставляем ConEmuC.exe /DOSBOX
		else if ((mn_ImageBits == 16) && (mn_ImageSubsystem == IMAGE_SUBSYSTEM_DOS_EXECUTABLE)
				&& m_SrvMapping.cbSize && (m_SrvMapping.Flags & CECF_DosBox))
			bGoChangeParm = true;
	}

	if (bGoChangeParm)
	{
		if (bDebugWasRequested && gbPrepareDefaultTerminal)
		{
			mb_NeedInjects = FALSE;
			// We need to post attach ConEmu GUI to started console
			if (bVsNetHostRequested)
				mb_PostInjectWasRequested = TRUE;
			else
				mb_DebugWasRequested = TRUE;
			// Пока что не будем убирать "мелькание" окошка.
			// На факт "видимости" консольного окна ориентируется ConEmuC
			// при аттаче. Если окошко НЕ видимое - считаем, что оно было
			// запущено процессом для служебных целей, и не трогаем его...
			// -> ConsoleMain.cpp: ParseCommandLine: "if (!ghConWnd || !(lbIsWindowVisible = IsWindowVisible(ghConWnd)) || isTerminalMode())"
			#if 0
			// Remove flickering?
			if (anShowCmd)
			{
				*anShowCmd = SW_HIDE;
				lbChanged = TRUE;
			}
			else
			#endif
			{
				lbChanged = FALSE;
			}
			goto wrap;
		}

		lbChanged = ChangeExecuteParms(aCmd, bConsoleMode, asFile, asParam, ms_ExeTmp,
			NewConsoleFlags, m_Args, mn_ImageBits, mn_ImageSubsystem, psFile, psParam);

		if (!lbChanged)
		{
			// Хуки нельзя ставить в 16битные приложение - будет облом, ntvdm.exe игнорировать!
			// И если просили не ставить хуки (-new_console:i) - тоже
			mb_NeedInjects = (mn_ImageBits != 16) && (m_Args.InjectsDisable != crb_On);
		}
		else
		{
			// Замена на "ConEmuC.exe ...", все "-new_console" / "-cur_console" будут обработаны в нем
			bCurConsoleArg = false;

			HWND hConWnd = GetRealConsoleWindow();

			if (lbGuiApp && (NewConsoleFlags || bForceNewConsole))
			{
				if (anShowCmd)
					*anShowCmd = SW_HIDE;

				if (anShellFlags && hConWnd)
				{
					*anShellFlags |= SEE_MASK_NO_CONSOLE;
				}

				#if 0
				// нужно запускаться ВНЕ текущей консоли!
				if (aCmd == eCreateProcess)
				{
					if (anCreateFlags)
					{
						*anCreateFlags |= CREATE_NEW_CONSOLE;
						*anCreateFlags &= ~(DETACHED_PROCESS|CREATE_NO_WINDOW);
					}
				}
				else if (aCmd == eShellExecute)
				{
					if (anShellFlags)
						*anShellFlags |= SEE_MASK_NO_CONSOLE;
				}
				#endif
			}
			pIn = NewCmdOnCreate(eParmsChanged,
					asAction, *psFile, *psParam, asDir,
					anShellFlags, anCreateFlags, anStartFlags, anShowCmd,
					mn_ImageBits, mn_ImageSubsystem,
					hIn, hOut, hErr/*, szBaseDir, mb_DosBoxAllowed*/);
			if (pIn)
			{
				CESERVER_REQ *pOut = NULL;
				pOut = ExecuteGuiCmd(hConWnd, pIn, hConWnd);
				ExecuteFreeResult(pIn); pIn = NULL;
				if (pOut)
					ExecuteFreeResult(pOut);
			}
		}
	}
	else
	{
		//lbChanged = ChangeExecuteParms(aCmd, asFile, asParam, pszBaseDir,
		//				ms_ExeTmp, mn_ImageBits, mn_ImageSubsystem, psFile, psParam);
		// Хуки нельзя ставить в 16битные приложение - будет облом, ntvdm.exe игнорировать!
		// И если просили не ставить хуки (-new_console:i) - тоже
		mb_NeedInjects = (aCmd == eCreateProcess) && (mn_ImageBits != 16)
			&& (m_Args.InjectsDisable != crb_On) && !gbPrepareDefaultTerminal
			&& !bDontForceInjects;

		// Параметр -cur_console / -new_console нужно вырезать
		if (NewConsoleFlags || bCurConsoleArg)
		{
			_ASSERTEX(m_Args.pszSpecialCmd!=NULL && "Must be replaced already!");

			// явно выставим в TRUE, т.к. это мог быть -new_console
			bCurConsoleArg = TRUE;
		}
	}

wrap:
	if ((bCurConsoleArg && (aCmd != eShellExecute))
		|| (bForceCutNewConsole && !lbChanged) // For example, -new_console:z (don't start new console by `start` in ConEmu)
		)
	{
		if (lbChanged)
		{
			_ASSERTE(lbChanged==FALSE && "bCurConsoleArg must be reset?");
		}
		else
		{
			// Указание рабочей папки
			if (m_Args.pszStartupDir)
			{
				*psStartDir = m_Args.pszStartupDir;
				m_Args.pszStartupDir = NULL;
			}

			if (bForceCutNewConsole && lsReplaceFile)
			{
				*psFile = lsReplaceFile.Detach();
			}

			if (bForceCutNewConsole && lsReplaceParm && (aCmd == eShellExecute))
			{
				*psParam = lsReplaceParm.Detach();
			}
			else
			{
				// Parameters substitution (cut -cur_console, -new_console)
				*psParam = m_Args.pszSpecialCmd;
				m_Args.pszSpecialCmd = NULL;
			}

			// Высота буфера!
			if ((m_Args.BufHeight == crb_On) && gnServerPID)
			{
				//CESERVER_REQ *pIn = ;
				//ExecutePrepareCmd(&In, CECMD_SETSIZESYNC, sizeof(CESERVER_REQ_HDR));
				//CESERVER_REQ* pOut = ExecuteSrvCmd(nServerPID/*gdwServerPID*/, (CESERVER_REQ*)&In, hConWnd);
				HANDLE hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
				CONSOLE_SCREEN_BUFFER_INFO csbi = {};
				if (GetConsoleScreenBufferInfo(hConOut, &csbi))
				{
					bool bNeedChange = false;
					BOOL bBufChanged = FALSE;
					if (m_Args.nBufHeight)
					{
						WARNING("Хорошо бы на команду сервера это перевести");
						//SHORT nNewHeight = max((csbi.srWindow.Bottom - csbi.srWindow.Top + 1),(SHORT)m_Args.nBufHeight);
						//if (nNewHeight != csbi.dwSize.Y)
						//{
						//	csbi.dwSize.Y = nNewHeight;
						//	bNeedChange = true;
						//}
					}
					else if (csbi.dwSize.Y > (csbi.srWindow.Bottom - csbi.srWindow.Top + 1))
					{
						bNeedChange = true;
						csbi.dwSize.Y = (csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
					}

					if (bNeedChange)
					{
						bBufChanged = SetConsoleScreenBufferSize(hConOut, csbi.dwSize);
					}
					UNREFERENCED_PARAMETER(bBufChanged);
				}
			}
		}
	}

	if (lbChanged && (psStartDir && !*psStartDir) && !szLnkDir.IsEmpty())
	{
		*psStartDir = szLnkDir.Detach();
	}

	if (m_Args.ForceHooksServer == crb_On)
	{
		CheckHookServer();
	}

	LogExit(lbChanged ? 1 : 0);
	return lbChanged ? 1 : 0;
} // PrepareExecuteParms

bool CShellProc::GetStartingExeName(LPCWSTR asFile, LPCWSTR asParam, CEStr& rsExeTmp)
{
	rsExeTmp.Empty();

	if (asFile && *asFile)
	{
		if (*asFile == L'"')
		{
			LPCWSTR pszEnd = wcschr(asFile+1, L'"');
			if (pszEnd)
			{
				size_t cchLen = (pszEnd - asFile) - 1;
				rsExeTmp.Set(asFile+1, cchLen);
			}
			else
			{
				rsExeTmp.Set(asFile+1);
			}
		}
		else
		{
			rsExeTmp.Set(asFile);
		}
	}
	else if (asParam)
	{
		// Strip "-new_console" and "-cur_console" (they may prepend executable)
		RConStartArgs lTempArgs;
		if (wcsstr(asParam, L"-new_console") || wcsstr(asParam, L"-cur_console"))
		{
			lTempArgs.pszSpecialCmd = lstrdup(SkipNonPrintable(asParam));
			if (lTempArgs.ProcessNewConArg())
			{
				asParam = SkipNonPrintable(lTempArgs.pszSpecialCmd);
				if (!asParam || !*asParam)
				{
					return false;
				}
			}
		}

		// If path to executable contains specials (spaces, etc.) it may be quoted, or not...
		// So, we can't just call NextArg, logic is more complicated.
		IsNeedCmd(false, SkipNonPrintable(asParam), rsExeTmp, NULL, NULL, NULL, NULL, NULL);
	}

	return (!rsExeTmp.IsEmpty());
}

// returns FALSE if need to block execution
BOOL CShellProc::OnShellExecuteA(LPCSTR* asAction, LPCSTR* asFile, LPCSTR* asParam, LPCSTR* asDir, DWORD* anFlags, DWORD* anShowCmd)
{
	if ((!ghConEmuWndDC || !IsWindow(ghConEmuWndDC)) && !isDefTermEnabled())
	{
		LogShellString(L"OnShellExecuteA skipped");
		return TRUE; // Перехватывать только под ConEmu
	}

	mb_InShellExecuteEx = TRUE;
	gnInShellExecuteEx ++;

	mpwsz_TempAction = str2wcs(asAction ? *asAction : NULL, mn_CP);
	mpwsz_TempFile = str2wcs(asFile ? *asFile : NULL, mn_CP);
	mpwsz_TempParam = str2wcs(asParam ? *asParam : NULL, mn_CP);
	CEStr lsDir(str2wcs(asDir ? *asDir : NULL, mn_CP));

	_ASSERTEX(!mpwsz_TempRetFile && !mpwsz_TempRetParam && !mpwsz_TempRetDir);

	int liRc = PrepareExecuteParms(eShellExecute,
					mpwsz_TempAction, mpwsz_TempFile, mpwsz_TempParam, lsDir,
					anFlags, NULL, NULL, anShowCmd,
					NULL, NULL, NULL, // *StdHandles
					&mpwsz_TempRetFile, &mpwsz_TempRetParam, &mpwsz_TempRetDir);
	if (liRc == -1)
		return FALSE; // Запретить выполнение файла

	BOOL lbRc = (liRc != 0);

	if (mpwsz_TempRetFile && *mpwsz_TempRetFile)
	{
		mpsz_TempRetFile = wcs2str(mpwsz_TempRetFile, mn_CP);
		*asFile = mpsz_TempRetFile;
	}
	else if (lbRc)
	{
		*asFile = NULL;
	}

	if (lbRc || mpwsz_TempRetParam)
	{
		mpsz_TempRetParam = wcs2str(mpwsz_TempRetParam, mn_CP);
		*asParam = mpsz_TempRetParam;
	}

	if (mpwsz_TempRetDir)
	{
		mpsz_TempRetDir = wcs2str(mpwsz_TempRetDir, mn_CP);
		*asDir = mpsz_TempRetDir;
	}

	return TRUE;
}

// returns FALSE if need to block execution
BOOL CShellProc::OnShellExecuteW(LPCWSTR* asAction, LPCWSTR* asFile, LPCWSTR* asParam, LPCWSTR* asDir, DWORD* anFlags, DWORD* anShowCmd)
{
	if ((!ghConEmuWndDC || !IsWindow(ghConEmuWndDC)) && !isDefTermEnabled())
	{
		LogShellString(L"OnShellExecuteW skipped");
		return TRUE; // Перехватывать только под ConEmu
	}

	// We do not hook lpIDList
	if (anFlags && (*anFlags & SEE_MASK_IDLIST))
	{
		LogShellString(L"SEE_MASK_IDLIST skipped");
		return TRUE;
	}

	mb_InShellExecuteEx = TRUE;
	gnInShellExecuteEx ++;

	_ASSERTEX(!mpwsz_TempRetFile && !mpwsz_TempRetParam && !mpwsz_TempRetDir);

	int liRc = PrepareExecuteParms(eShellExecute,
					asAction ? *asAction : NULL,
					asFile ? *asFile : NULL,
					asParam ? *asParam : NULL,
					asDir ? *asDir : NULL,
					anFlags, NULL, NULL, anShowCmd,
					NULL, NULL, NULL, // *StdHandles
					&mpwsz_TempRetFile, &mpwsz_TempRetParam, &mpwsz_TempRetDir);
	if (liRc == -1)
		return FALSE; // Запретить выполнение файла

	BOOL lbRc = (liRc != 0);

	if (lbRc || mpwsz_TempRetFile)
	{
		*asFile = mpwsz_TempRetFile;
	}

	if (lbRc || mpwsz_TempRetParam)
	{
		*asParam = mpwsz_TempRetParam;
	}

	if (mpwsz_TempRetDir)
	{
		*asDir = mpwsz_TempRetDir;
	}

	return TRUE;
}

BOOL CShellProc::FixShellArgs(DWORD afMask, HWND ahWnd, DWORD* pfMask, HWND* phWnd)
{
	BOOL lbRc = FALSE;

	// Включить флажок, чтобы Shell не задавал глупого вопроса "Хотите ли вы запустить этот файл"...
	if (!(afMask & SEE_MASK_NOZONECHECKS) && gFarMode.bFarHookMode && gFarMode.bShellNoZoneCheck)
	{
		if (IsWinXPSP1())
		{
			(*pfMask) |= SEE_MASK_NOZONECHECKS;
			lbRc = TRUE;
		}
	}

	// Чтобы запросы UAC или еще какие GUI диалоги всплывали там где надо, а не ПОД ConEmu
	if ((!ahWnd || (ahWnd == ghConWnd)) && ghConEmuWnd)
	{
		*phWnd = ghConEmuWnd;
		lbRc = TRUE;
	}

	return lbRc;
}

// returns FALSE if need to block execution
BOOL CShellProc::OnShellExecuteExA(LPSHELLEXECUTEINFOA* lpExecInfo)
{
	if (!lpExecInfo)
		return TRUE;

	if ((!ghConEmuWndDC || !IsWindow(ghConEmuWndDC)) && !isDefTermEnabled())
	{
		LogShellString(L"OnShellExecuteExA skipped");
		return TRUE; // Перехватывать только под ConEmu
	}

	mlp_SaveExecInfoA = *lpExecInfo;
	mlp_ExecInfoA = (LPSHELLEXECUTEINFOA)malloc((*lpExecInfo)->cbSize);
	if (!mlp_ExecInfoA)
	{
		_ASSERTE(mlp_ExecInfoA!=NULL);
		return TRUE;
	}
	memmove(mlp_ExecInfoA, (*lpExecInfo), (*lpExecInfo)->cbSize);

	FixShellArgs((*lpExecInfo)->fMask, (*lpExecInfo)->hwnd, &(mlp_ExecInfoA->fMask), &(mlp_ExecInfoA->hwnd));

	BOOL lbRc = OnShellExecuteA(&mlp_ExecInfoA->lpVerb, &mlp_ExecInfoA->lpFile, &mlp_ExecInfoA->lpParameters, &mlp_ExecInfoA->lpDirectory, &mlp_ExecInfoA->fMask, (DWORD*)&mlp_ExecInfoA->nShow);

	if (lbRc)
		*lpExecInfo = mlp_ExecInfoA;

	return lbRc;
}

// returns FALSE if need to block execution
BOOL CShellProc::OnShellExecuteExW(LPSHELLEXECUTEINFOW* lpExecInfo)
{
	if (!lpExecInfo)
		return TRUE;

	if ((!ghConEmuWndDC || !IsWindow(ghConEmuWndDC)) && !isDefTermEnabled())
	{
		LogShellString(L"OnShellExecuteExW skipped");
		return TRUE; // Перехватывать только под ConEmu или в DefTerm
	}

	mlp_SaveExecInfoW = *lpExecInfo;
	mlp_ExecInfoW = (LPSHELLEXECUTEINFOW)malloc((*lpExecInfo)->cbSize);
	if (!mlp_ExecInfoW)
	{
		_ASSERTE(mlp_ExecInfoW!=NULL);
		return TRUE;
	}
	memmove(mlp_ExecInfoW, (*lpExecInfo), (*lpExecInfo)->cbSize);

	FixShellArgs((*lpExecInfo)->fMask, (*lpExecInfo)->hwnd, &(mlp_ExecInfoW->fMask), &(mlp_ExecInfoW->hwnd));

	BOOL lbRc = OnShellExecuteW(&mlp_ExecInfoW->lpVerb, &mlp_ExecInfoW->lpFile, &mlp_ExecInfoW->lpParameters, &mlp_ExecInfoW->lpDirectory, &mlp_ExecInfoW->fMask, (DWORD*)&mlp_ExecInfoW->nShow);

	if (lbRc)
		*lpExecInfo = mlp_ExecInfoW;

	return lbRc;
}

// returns FALSE if need to block execution
BOOL CShellProc::OnCreateProcessA(LPCSTR* asFile, LPCSTR* asCmdLine, LPCSTR* asDir, DWORD* anCreationFlags, LPSTARTUPINFOA lpSI)
{
	if (!ghConEmuWndDC || !IsWindow(ghConEmuWndDC))
	{
		LogShellString(L"OnCreateProcessA skipped");
		return TRUE; // Перехватывать только под ConEmu
	}

	mpwsz_TempFile = str2wcs(asFile ? *asFile : NULL, mn_CP);
	mpwsz_TempParam = str2wcs(asCmdLine ? *asCmdLine : NULL, mn_CP);
	DWORD nShowCmd = lpSI->wShowWindow;
	mb_WasSuspended = ((*anCreationFlags) & CREATE_SUSPENDED) == CREATE_SUSPENDED;
	CEStr lsDir(str2wcs(asDir ? *asDir : NULL, mn_CP));

	_ASSERTEX(!mpwsz_TempRetFile && !mpwsz_TempRetParam && !mpwsz_TempRetDir);

	int liRc = PrepareExecuteParms(eCreateProcess,
					NULL, mpwsz_TempFile, mpwsz_TempParam, lsDir,
					NULL, anCreationFlags, &lpSI->dwFlags, &nShowCmd,
					&lpSI->hStdInput, &lpSI->hStdOutput, &lpSI->hStdError,
					&mpwsz_TempRetFile, &mpwsz_TempRetParam, &mpwsz_TempRetDir);
	if (liRc == -1)
		return FALSE; // Запретить выполнение файла

	BOOL lbRc = (liRc != 0);

	// возвращает TRUE только если были изменены СТРОКИ,
	// а если выставлен mb_NeedInjects - строго включить _Suspended
	if (mb_NeedInjects)
		(*anCreationFlags) |= CREATE_SUSPENDED;

	if (lbRc && (lpSI->wShowWindow != nShowCmd))
		lpSI->wShowWindow = (WORD)nShowCmd;

	if (mpwsz_TempRetFile && *mpwsz_TempRetFile)
	{
		mpsz_TempRetFile = wcs2str(mpwsz_TempRetFile, mn_CP);
		*asFile = mpsz_TempRetFile;
	}
	else if (lbRc)
	{
		*asFile = NULL;
	}

	if (lbRc || mpwsz_TempRetParam)
	{
		if (mpwsz_TempRetParam)
		{
			mpsz_TempRetParam = wcs2str(mpwsz_TempRetParam, mn_CP);
			*asCmdLine = mpsz_TempRetParam;
		}
		else
		{
			*asCmdLine = NULL;
		}
	}
	if (mpwsz_TempRetDir)
	{
		mpsz_TempRetDir = wcs2str(mpwsz_TempRetDir, mn_CP);
		*asDir = mpsz_TempRetDir;
	}

	return TRUE;
}

// returns FALSE if need to block execution
BOOL CShellProc::OnCreateProcessW(LPCWSTR* asFile, LPCWSTR* asCmdLine, LPCWSTR* asDir, DWORD* anCreationFlags, LPSTARTUPINFOW lpSI)
{
	if (!ghConEmuWndDC || !IsWindow(ghConEmuWndDC))
	{
		if (isDefTermEnabled())
		{
			// OK, continue to "Default terminal" feature (console applications and batch files only)
		}
		else
		{
			LogShellString(L"OnCreateProcessW skipped");
			return TRUE; // Перехватывать только под ConEmu
		}
	}

	// For example, mintty starts root using pipe redirection
	bool bConsoleNoWindow = (lpSI->dwFlags & STARTF_USESTDHANDLES)
		// or the caller need to run some process as "detached"
		|| ((*anCreationFlags) & (DETACHED_PROCESS|CREATE_NO_WINDOW));

	// Some "heuristics" - when created process may show its window? (Console or GUI)
	DWORD nShowCmd = (lpSI->dwFlags & STARTF_USESHOWWINDOW)
		? lpSI->wShowWindow
		: ((gbAttachGuiClient || gbGuiClientAttached) && bConsoleNoWindow) ? 0
		: SW_SHOWNORMAL;

	// Console.exe starts cmd.exe with STARTF_USEPOSITION flag
	if ((lpSI->dwFlags & STARTF_USEPOSITION) && (lpSI->dwX == 32767) && (lpSI->dwY == 32767))
		nShowCmd = SW_HIDE; // Lets thing it is stating hidden

	mb_WasSuspended = ((*anCreationFlags) & CREATE_SUSPENDED) == CREATE_SUSPENDED;

	_ASSERTEX(!mpwsz_TempRetFile && !mpwsz_TempRetParam && !mpwsz_TempRetDir);

	int liRc = PrepareExecuteParms(eCreateProcess,
					NULL,
					asFile ? *asFile : NULL,
					asCmdLine ? *asCmdLine : NULL,
					asDir ? *asDir : NULL,
					NULL, anCreationFlags, &lpSI->dwFlags, &nShowCmd,
					&lpSI->hStdInput, &lpSI->hStdOutput, &lpSI->hStdError,
					&mpwsz_TempRetFile, &mpwsz_TempRetParam, &mpwsz_TempRetDir);
	if (liRc == -1)
		return FALSE; // Запретить выполнение файла

	BOOL lbRc = (liRc != 0);

	// возвращает TRUE только если были изменены СТРОКИ,
	// а если выставлен mb_NeedInjects - строго включить _Suspended
	if (mb_NeedInjects)
	{
		// In DefTerm (gbPrepareDefaultTerminal) mb_NeedInjects is allowed too in some cases...
		// code.exe -> "cmd /c start /wait" -> cmd.exe
		(*anCreationFlags) |= CREATE_SUSPENDED;
	}

	if (lbRc)
	{
		if (gbPrepareDefaultTerminal)
		{
			_ASSERTE(m_Args.NoDefaultTerm != crb_On);

			if (mb_PostInjectWasRequested)
			{
				lbRc = FALSE; // Stop other changes?
			}
			else
			{
				if (mb_DebugWasRequested)
				{
					lpSI->wShowWindow = LOWORD(nShowCmd); // this is SW_HIDE, disable flickering
					lbRc = FALSE; // Stop other changes?
				}
				else
				{
					lpSI->wShowWindow = SW_SHOWNORMAL; // ConEmu itself must starts normally?
				}
				lpSI->dwFlags |= STARTF_USESHOWWINDOW;
			}
		}
		else if (lpSI->wShowWindow != nShowCmd)
		{
			lpSI->wShowWindow = (WORD)nShowCmd;
			if (!(lpSI->dwFlags & STARTF_USESHOWWINDOW))
				lpSI->dwFlags |= STARTF_USESHOWWINDOW;
		}

		if (lbRc)
		{
			*asFile = mpwsz_TempRetFile;
		}
	}
	// Avoid flickering of RealConsole while starting debugging with DefTerm feature
	else if (isDefTermEnabled() && !bConsoleNoWindow && nShowCmd && anCreationFlags && lpSI)
	{
		switch (mn_ImageSubsystem)
		{
		case IMAGE_SUBSYSTEM_WINDOWS_CUI:
			if (((*anCreationFlags) & (DEBUG_ONLY_THIS_PROCESS|DEBUG_PROCESS))
				&& ((*anCreationFlags) & CREATE_NEW_CONSOLE))
			{
				// Запуск отладчика Win32 приложения из VisualStudio (например)
				// Финт ушами для избежания мелькания RealConsole window
				HWND hConWnd = GetRealConsoleWindow();
				if (hConWnd == NULL)
				{
					_ASSERTE(gnServerPID==0);
					// Alloc hidden console and attach it to our VS GUI window
					if (gpDefTerm->AllocHiddenConsole(true))
					{
						_ASSERTE(gnServerPID!=0);
						// Удалось создать скрытое консольное окно, приложение можно запустить в нем
						mb_HiddenConsoleDetachNeed = TRUE;
						// Do not need to "Show" it
						lpSI->wShowWindow = SW_HIDE;
						lpSI->dwFlags |= STARTF_USESHOWWINDOW;
						// Reuse existing console
						(*anCreationFlags) &= ~CREATE_NEW_CONSOLE;
					}
				}
			}
			break; // IMAGE_SUBSYSTEM_WINDOWS_CUI

		case IMAGE_SUBSYSTEM_WINDOWS_GUI:
			if (!((*anCreationFlags) & (DEBUG_ONLY_THIS_PROCESS|DEBUG_PROCESS))
				&& !((*anCreationFlags) & CREATE_NEW_CONSOLE))
			{
				// Запуск msvsmon.exe?
				LPCWSTR pszExeName = PointToName(ms_ExeTmp);
				if (pszExeName && (lstrcmpi(pszExeName, L"msvsmon.exe") == 0))
				{
					// Теоретически, хорошо бы хукать только те отладчики, которые запускаются
					// для работы с локальными процессами: msvsmon.exe ... /hostname [::1] /port ... /__pseudoremote
					mb_PostInjectWasRequested = TRUE;
					if (!mb_WasSuspended)
						(*anCreationFlags) |= CREATE_SUSPENDED;
				}
			}
			break; // IMAGE_SUBSYSTEM_WINDOWS_GUI
		}
	}

	if (lbRc || mpwsz_TempRetFile)
	{
		*asFile = mpwsz_TempRetFile;
	}

	if (lbRc || mpwsz_TempRetParam)
	{
		*asCmdLine = mpwsz_TempRetParam;
	}

	if (mpwsz_TempRetDir)
	{
		*asDir = mpwsz_TempRetDir;
	}

	return TRUE;
}

void CShellProc::OnCreateProcessFinished(BOOL abSucceeded, PROCESS_INFORMATION *lpPI)
{
	// logging
	{
		int cchLen = 255;
		wchar_t* pszDbgMsg = (wchar_t*)calloc(cchLen, sizeof(wchar_t));
		if (pszDbgMsg)
		{
			if (!abSucceeded)
			{
				msprintf(pszDbgMsg, cchLen, L"Create(ParentPID=%u): Failed, ErrCode=0x%08X",
					GetCurrentProcessId(), GetLastError());
			}
			else
			{
				msprintf(pszDbgMsg, cchLen, L"Create(ParentPID=%u): Ok, PID=%u",
					GetCurrentProcessId(), lpPI->dwProcessId);
				if (WaitForSingleObject(lpPI->hProcess, 0) == WAIT_OBJECT_0)
				{
					DWORD dwExitCode = 0;
					GetExitCodeProcess(lpPI->hProcess, &dwExitCode);
					msprintf(pszDbgMsg+lstrlen(pszDbgMsg), cchLen-lstrlen(pszDbgMsg),
						L", Terminated!!! Code=%u", dwExitCode);
				}
			}
			_wcscat_c(pszDbgMsg, cchLen, L"\n");
			LogShellString(pszDbgMsg);
			free(pszDbgMsg);
		}
	}

	BOOL bAttachCreated = FALSE;

	if (abSucceeded)
	{
		CShellProc::mn_LastStartedPID = lpPI->dwProcessId;

		// ssh-agent.exe is intended to do self-fork to detach from current console
		if (lpPI->dwProcessId && !ms_ExeTmp.IsEmpty() && IsSshAgentHelper(ms_ExeTmp))
		{
			CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_SSHAGENTSTART, sizeof(CESERVER_REQ_HDR)+sizeof(DWORD));
			if (pIn)
			{
				pIn->dwData[0] = lpPI->dwProcessId;
				CESERVER_REQ* pOut = ExecuteGuiCmd(ghConWnd, pIn, NULL);
				ExecuteFreeResult(pOut);
				ExecuteFreeResult(pIn);
			}
		}

		if (gnAttachPortableGuiCui && gnServerPID)
		{
			CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_PORTABLESTART, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_PORTABLESTARTED));
			if (pIn)
			{
				pIn->PortableStarted.nSubsystem = mn_ImageSubsystem;
				pIn->PortableStarted.nImageBits = mn_ImageBits;
				_ASSERTE(wcschr(ms_ExeTmp,L'\\')!=NULL);
				lstrcpyn(pIn->PortableStarted.sAppFilePathName, ms_ExeTmp, countof(pIn->PortableStarted.sAppFilePathName));
				pIn->PortableStarted.nPID = lpPI->dwProcessId;
				HANDLE hServer = OpenProcess(PROCESS_DUP_HANDLE, FALSE, gnServerPID);
				if (hServer)
				{
					HANDLE hDup = NULL;
					DuplicateHandle(GetCurrentProcess(), lpPI->hProcess, hServer, &hDup, 0, FALSE, DUPLICATE_SAME_ACCESS);
					pIn->PortableStarted.hProcess = hDup;
					CloseHandle(hServer);
				}

				CESERVER_REQ* pOut = ExecuteSrvCmd(gnServerPID, pIn, NULL);
				ExecuteFreeResult(pOut);
				ExecuteFreeResult(pIn);
			}
		}

		if (m_Args.nPTY)
		{
			CEAnsi::ChangeTermMode(tmc_Keyboard, (m_Args.nPTY == 1), lpPI->dwProcessId);
		}

		if (isDefTermEnabled())
		{
			// Starting .Net debugging session from VS or CodeBlocks console app (gdb)
			if (mb_PostInjectWasRequested)
			{
				// This is "*.vshost.exe", it is GUI which can be used for debugging .Net console applications
				// Also in CodeBlocks' gdb debugger
				if (mb_WasSuspended)
				{
					// Supposing Studio will process only one "*.vshost.exe" at a time
					m_WaitDebugVsThread = *lpPI;
				}
				else
				{
					OnResumeDebugeeThreadCalled(lpPI->hThread, lpPI);
				}
			}
			// Starting debugging session from VS (for example)?
			else if (mb_DebugWasRequested && !mb_HiddenConsoleDetachNeed)
			{
				// We need to start console app directly, but it will be nice
				// to attach it to the existing or new ConEmu window.
				size_t cchMax = MAX_PATH+80;
				CEStr szSrvArgs, szNewCon;
				cchMax += gpDefTerm->GetSrvAddArgs(false, szSrvArgs, szNewCon);
				_ASSERTE(szNewCon.IsEmpty());

				wchar_t* pszCmdLine = (wchar_t*)malloc(cchMax*sizeof(*pszCmdLine));
				if (pszCmdLine)
				{
					_ASSERTEX(m_SrvMapping.ComSpec.ConEmuBaseDir[0]!=0);
					msprintf(pszCmdLine, cchMax, L"\"%s\\%s\" /ATTACH /CONPID=%u%s",
						m_SrvMapping.ComSpec.ConEmuBaseDir, WIN3264TEST(L"ConEmuC.exe",L"ConEmuC64.exe"),
						lpPI->dwProcessId,
						(LPCWSTR)szSrvArgs);
					STARTUPINFO si = {sizeof(si)};
					PROCESS_INFORMATION pi = {};
					bAttachCreated = CreateProcess(NULL, pszCmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, m_SrvMapping.ComSpec.ConEmuBaseDir, &si, &pi);
					if (bAttachCreated)
					{
						CloseHandle(pi.hProcess);
						CloseHandle(pi.hThread);
					}
					else
					{
						DWORD nErr = GetLastError();
						wchar_t* pszDbg = (wchar_t*)malloc(MAX_PATH*3*sizeof(*pszDbg));
						if (pszDbg)
						{
							msprintf(pszDbg, MAX_PATH*3, L"ConEmuHk: Failed to start attach server, Err=%u! %s\n", nErr, pszCmdLine);
							LogShellString(pszDbg);
							free(pszDbg);
						}
					}
					free(pszCmdLine);
				}
			}
			else if (mb_NeedInjects)
			{
				// code.exe -> "cmd /c start /wait" -> cmd.exe
				RunInjectHooks(WIN3264TEST(L"DefTerm",L"DefTerm64"), lpPI);
			}
		}
		else if (mb_NeedInjects)
		{
			RunInjectHooks(WIN3264TEST(L"ConEmuHk",L"ConEmuHk64"), lpPI);
		}
	}

	if (mb_HiddenConsoleDetachNeed)
	{
		FreeConsole();
		SetServerPID(0);
	}
}

void CShellProc::RunInjectHooks(LPCWSTR asFrom, PROCESS_INFORMATION *lpPI)
{
	wchar_t szDbgMsg[255];
	msprintf(szDbgMsg, countof(szDbgMsg), L"%s: InjectHooks(x%u), ParentPID=%u (%s), ChildPID=%u\n",
		asFrom, WIN3264TEST(32,64), GetCurrentProcessId(), gsExeName, lpPI->dwProcessId);
	LogShellString(szDbgMsg);

	LPCWSTR pszDllDir = NULL;
	if (isDefTermEnabled() && gpDefTerm)
		pszDllDir = gpDefTerm->GetOpt()->pszConEmuBaseDir;

	CINJECTHK_EXIT_CODES iHookRc = InjectHooks(*lpPI,
		(m_SrvMapping.cbSize && (m_SrvMapping.nLoggingType == glt_Processes)),
		pszDllDir);

	if (iHookRc != CIH_OK/*0*/)
	{
		DWORD nErrCode = GetLastError();
		// Хуки не получится установить для некоторых системных процессов типа ntvdm.exe,
		// но при запуске dos приложений мы сюда дойти не должны
		_ASSERTE(iHookRc == 0);
		wchar_t szTitle[128];
		msprintf(szTitle, countof(szTitle), L"%s, PID=%u", asFrom, GetCurrentProcessId());
		msprintf(szDbgMsg, countof(szDbgMsg), L"%s: PID=%u\nInjecting hooks into PID=%u\nFAILED, code=%i:0x%08X",
			gsExeName, GetCurrentProcessId(), lpPI->dwProcessId, iHookRc, nErrCode);
		GuiMessageBox(NULL, szDbgMsg, szTitle, MB_SYSTEMMODAL);
	}

	// Release process, it called was not set CREATE_SUSPENDED flag
	if (!mb_WasSuspended)
	{
		ResumeThread(lpPI->hThread);
	}
}

bool CShellProc::OnResumeDebugeeThreadCalled(HANDLE hThread, PROCESS_INFORMATION* lpPI /*= NULL*/)
{
	if ((!hThread || (m_WaitDebugVsThread.hThread != hThread)) && !lpPI)
		return false;

	int iHookRc = -1;
	DWORD nPreError = GetLastError();
	DWORD nResumeRC = -1;

	DWORD nPID = lpPI ? lpPI->dwProcessId : m_WaitDebugVsThread.dwProcessId;
	HANDLE hProcess = lpPI ? lpPI->hProcess : m_WaitDebugVsThread.hProcess;
	_ASSERTEX(hThread != NULL);
	ZeroStruct(m_WaitDebugVsThread);

	bool bNotInitialized = true;
	DWORD nErrCode;
	DWORD nStartTick = GetTickCount(), nCurTick, nDelta = 0, nDeltaMax = 5000;

	// DefTermHooker needs to know about process modules
	// But it is impossible if process was not initialized yet
	while (bNotInitialized
		&& (WaitForSingleObject(hProcess, 0) == WAIT_TIMEOUT))
	{
		nResumeRC = ResumeThread(hThread);
		Sleep(50);
		SuspendThread(hThread);

		// We need to ensure that process has been 'started'
		// If not - CreateToolhelp32Snapshot will return ERROR_PARTIAL_COPY
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, nPID);
		if (hSnap && (hSnap != INVALID_HANDLE_VALUE))
		{
			CloseHandle(hSnap);
			bNotInitialized = false;
		}
		else
		{
			nErrCode = GetLastError();
			bNotInitialized = (nErrCode == ERROR_PARTIAL_COPY);
		}

		// Wait for 5 seconds max
		nDelta = (nCurTick = GetTickCount()) - nStartTick;
		if (nDelta > nDeltaMax)
			break;
	}


	// *****************************************************
	// if process was not terminated yet, call DefTermHooker
	// *****************************************************
	DWORD nProcessWait = WaitForSingleObject(hProcess, 0);
	if (nProcessWait == WAIT_TIMEOUT)
	{
		iHookRc = gpDefTerm->StartDefTermHooker(nPID, hProcess);
	}


	// Resume thread if it was done by us
	if (lpPI)
		ResumeThread(hThread);

	// Restore ErrCode
	SetLastError(nPreError);

	return (iHookRc == 0);
}

void CShellProc::OnShellFinished(BOOL abSucceeded, HINSTANCE ahInstApp, HANDLE ahProcess)
{
	DWORD dwProcessID = 0;
	if (abSucceeded && gfGetProcessId && ahProcess)
	{
		dwProcessID = gfGetProcessId(ahProcess);
	}

	// InjectHooks & ResumeThread тут делать не нужно, просто вернуть параметры, если было переопределение
	if (mlp_SaveExecInfoW)
	{
		mlp_SaveExecInfoW->hInstApp = ahInstApp;
		mlp_SaveExecInfoW->hProcess = ahProcess;
	}
	else if (mlp_SaveExecInfoA)
	{
		mlp_SaveExecInfoA->hInstApp = ahInstApp;
		mlp_SaveExecInfoA->hProcess = ahProcess;
	}

	if (mb_InShellExecuteEx)
	{
		if (gnInShellExecuteEx > 0)
			gnInShellExecuteEx--;
		mb_InShellExecuteEx = FALSE;
	}
}

void CShellProc::LogShellString(LPCWSTR asMessage)
{
	DefTermLogString(asMessage);

	#ifdef PRINT_SHELL_LOG
	wprintf(L"%s\n", asMessage);
	#endif
}
