
/*
Copyright (c) 2011-present Maximus5
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

#include "../common/Common.h"

#include <tchar.h>
#include <shlwapi.h>
#include "../common/shlobj.h"
#include "../common/CmdLine.h"
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

#include "DllOptions.h"
#include "hlpConsole.h"
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

static const DWORD HIDDEN_SCREEN_POSITION = 32767;

//int CShellProc::mn_InShellExecuteEx = 0;
int gnInShellExecuteEx = 0;

bool  CShellProc::mb_StartingNewGuiChildTab = 0;
DWORD CShellProc::mn_LastStartedPID = 0;
PROCESS_INFORMATION CShellProc:: m_WaitDebugVsThread = {};


namespace {
void LogFarExecCommand(
	enum CmdOnCreateType aCmd,
	LPCWSTR asAction, LPCWSTR asFile, LPCWSTR asParam, LPCWSTR asDir,
	DWORD* /*anShellFlags*/, DWORD* /*anCreateFlags*/, DWORD* /*anStartFlags*/,
	DWORD* /*anShowCmd*/, HANDLE* /*lphStdIn*/, HANDLE* /*lphStdOut*/, HANDLE* /*lphStdErr*/)
{
	if (!gfnSrvLogString)
		return;
	wchar_t far_info[120];
	msprintf(far_info, std::size(far_info),
		L", Version=%u.%u.%u%s x%u, LongConsoleOutput=%s",
		gFarMode.farVer.dwVerMajor, gFarMode.farVer.dwVerMinor, gFarMode.farVer.dwBuild,
		gFarMode.farVer.Bis ? L"bis" : L"", gFarMode.farVer.dwBits,
		gFarMode.bLongConsoleOutput ? L"yes" : L"no");
	CEStr log_str(
		L"Far.exe: action=",
		(aCmd == eShellExecute)
		? ((asAction && *asAction) ? asAction : L"<shell>")
		: L"<create>",
		(asFile && *asFile) ? L", file=" : nullptr, asFile,
		(asParam && *asParam) ? L", parm=" : nullptr, asParam,
		(asDir && *asDir) ? L", dir=" : nullptr, asDir,
		far_info);
	gfnSrvLogString(log_str);
}
}

ShellWorkOptions operator|=(const ShellWorkOptions e1, const ShellWorkOptions e2)
{
	return static_cast<ShellWorkOptions>(static_cast<uint32_t>(e1) | static_cast<uint32_t>(e2));
}

ShellWorkOptions operator|(const ShellWorkOptions e1, const ShellWorkOptions e2)
{
	return static_cast<ShellWorkOptions>(static_cast<uint32_t>(e1) | static_cast<uint32_t>(e2));
}

bool operator&(const ShellWorkOptions e1, const ShellWorkOptions e2)
{
	if (e2 == ShellWorkOptions::None)
		return e1 == ShellWorkOptions::None;
	const auto masked = static_cast<uint32_t>(e1) & static_cast<uint32_t>(e2);
	return masked == static_cast<uint32_t>(e2);
}


#define LogExit(frc) LogExitLine(frc, __LINE__)

CShellProc::CShellProc()
{
	mn_CP = AreFileApisANSI() ? CP_ACP : CP_OEMCP;
	mh_PreConEmuWnd = ghConEmuWnd; mh_PreConEmuWndDC = ghConEmuWndDC;

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

	if (mb_InShellExecuteEx)
	{
		if (gnInShellExecuteEx > 0)
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
	IPersistFile* pFile = nullptr;
	IShellLinkW*  pShellLink = nullptr;
	// ReSharper disable once CppJoinDeclarationAndAssignment
	HRESULT hr;
	uint64_t nLnkSize;
	int cchMaxArgSize = 0;
	static bool bCoInitialized = false;

	if (!FileExists(asLnkFile, &nLnkSize))
		goto wrap;

	if (!InitOle32())
		goto wrap;

	hr = CoCreateInstance_f(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, reinterpret_cast<void**>(&pShellLink));
	if (FAILED(hr) && !bCoInitialized)
	{
		bCoInitialized = true;
		hr = CoInitializeEx_f(nullptr, COINIT_MULTITHREADED);
		if (SUCCEEDED(hr))
			hr = CoCreateInstance_f(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, reinterpret_cast<void**>(&pShellLink));
	}
	if (FAILED(hr) || !pShellLink)
		goto wrap;

	hr = pShellLink->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&pFile));
	if (FAILED(hr) || !pFile)
		goto wrap;

	hr = pFile->Load(asLnkFile, STGM_READ);
	if (FAILED(hr))
		goto wrap;

	hr = pShellLink->GetPath(rsExe.GetBuffer(MAX_PATH), MAX_PATH, nullptr, 0);
	if (FAILED(hr) || rsExe.IsEmpty())
		goto wrap;

	hr = pShellLink->GetWorkingDirectory(rsWorkDir.GetBuffer(MAX_PATH + 1), MAX_PATH + 1);
	if (FAILED(hr))
		goto wrap;

	cchMaxArgSize = static_cast<int>(std::min<uint64_t>(nLnkSize, std::numeric_limits<int>::max()));
	hr = pShellLink->GetArguments(rsArgs.GetBuffer(cchMaxArgSize), cchMaxArgSize);
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
		return nullptr;

	if (ghConEmuWnd)
	{
		if (IsWindow(ghConEmuWnd))
		{
			return ghConEmuWnd; // OK, живое
		}

		// Сброс, ищем заново
		ghConEmuWnd = ghConEmuWndDC = nullptr;
		mb_TempConEmuWnd = TRUE;
	}

	HWND h = FindWindowEx(nullptr, nullptr, VirtualConsoleClassMain, nullptr);
	if (h)
	{
		ghConEmuWnd = h;
		HWND hWork = FindWindowEx(h, nullptr, VirtualConsoleClassWork, nullptr);
		_ASSERTEX(hWork!=nullptr && "Workspace must be inside ConEmu"); // код расчитан на это
		ghConEmuWndDC = FindWindowEx(h, nullptr, VirtualConsoleClass, nullptr);
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

void CShellProc::LogExitLine(const int rc, const int line) const
{
	wchar_t szInfo[200];
	msprintf(szInfo, countof(szInfo),
		L"PrepareExecuteParams rc=%i T:%u %u:%u:%u W:%u I:%u,%u,%u D:%u H:%u S:%u,%u line=%i",
		/*rc*/(rc), /*T:*/gbPrepareDefaultTerminal ? 1 : 0,
		(UINT)mn_ImageSubsystem, (UINT)mn_ImageBits, (UINT)mb_isCurrentGuiClient,
		/*W:*/(UINT)workOptions_,
		/*I:*/(UINT)mb_NeedInjects, (UINT)mb_PostInjectWasRequested, (UINT)mb_Opt_DontInject,
		/*D:*/(UINT)mb_DebugWasRequested, /*H:*/(UINT)mb_HiddenConsoleDetachNeed,
		/*S:*/(UINT)mb_Opt_SkipNewConsole, (UINT)mb_Opt_SkipCmdStart,
		line);
	LogShellString(szInfo);
}

wchar_t* CShellProc::str2wcs(const char* psz, UINT anCP)
{
	if (!psz)
		return nullptr;
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
		// ghConWnd may be nullptr (if was started for devenv.exe) or NOT nullptr (after AllocConsole in *.vshost.exe)
		//_ASSERTEX(ghConWnd!=nullptr && "ConWnd was not initialized");

		// Parameters are stored in the registry now
		if (!CDefTermHk::IsDefTermEnabled())
		{
			_ASSERTEX(gpDefTerm != nullptr);
			LogShellString(L"LoadSrvMapping failed: !IsDefTermEnabled()");
			return FALSE;
		}

		if (!CDefTermHk::LoadDefTermSrvMapping(m_SrvMapping))
		{
			LogShellString(L"LoadSrvMapping failed: !LoadDefTermSrvMapping()");
			return FALSE;
		}

		_ASSERTE(m_SrvMapping.ComSpec.ConEmuExeDir[0] && m_SrvMapping.ComSpec.ConEmuBaseDir[0]);

		return TRUE;
	}

	if (!m_SrvMapping.cbSize || (m_SrvMapping.hConEmuWndDc && !IsWindow(m_SrvMapping.hConEmuWndDc)))
	{
		if (!::LoadSrvMapping(ghConWnd, m_SrvMapping))
		{
			LogShellString(L"LoadSrvMapping failed: !::LoadSrvMapping()");
			return FALSE;
		}
		_ASSERTE(m_SrvMapping.ComSpec.ConEmuExeDir[0] && m_SrvMapping.ComSpec.ConEmuBaseDir[0]);
	}

	if (!m_SrvMapping.hConEmuWndDc || !IsWindow(m_SrvMapping.hConEmuWndDc))
	{
		LogShellString(L"LoadSrvMapping failed: !::IsWindow()");
		return FALSE;
	}

	return TRUE;
}

CESERVER_REQ* CShellProc::NewCmdOnCreate(CmdOnCreateType aCmd,
				LPCWSTR asAction, LPCWSTR asFile, LPCWSTR asParam, LPCWSTR asDir,
				DWORD* anShellFlags, DWORD* anCreateFlags, DWORD* anStartFlags, DWORD* anShowCmd,
				const int nImageBits, const int nImageSubsystem,
				HANDLE hStdIn, HANDLE hStdOut, HANDLE hStdErr
				/*wchar_t (&szBaseDir)[MAX_PATH+2], BOOL& bDosBoxAllowed*/)
{
	if (!LoadSrvMapping())
		return nullptr;

	if (m_SrvMapping.nLoggingType != glt_Processes)
		return nullptr;

	CEStr tempCurDir;

	return ExecuteNewCmdOnCreate(&m_SrvMapping, ghConWnd, aCmd,
				asAction, asFile, asParam,
				(asDir && *asDir) ? asDir : GetDirectory(tempCurDir),
				anShellFlags, anCreateFlags, anStartFlags, anShowCmd,
				nImageBits, nImageSubsystem,
				hStdIn, hStdOut, hStdErr);
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

		bHooksTempDisabled = (wcsstr(szVar, ENV_CONEMU_HOOKS_DISABLED) != nullptr);

		bHooksSkipNewConsole = (wcsstr(szVar, ENV_CONEMU_HOOKS_NOARGS) != nullptr)
			|| (m_SrvMapping.cbSize && !(m_SrvMapping.Flags & ConEmu::ConsoleFlags::ProcessNewCon));

		bHooksSkipCmdStart = (wcsstr(szVar, ENV_CONEMU_HOOKS_NOSTART) != nullptr)
			|| (m_SrvMapping.cbSize && !(m_SrvMapping.Flags & ConEmu::ConsoleFlags::ProcessCmdStart));
	}
	else
	{
		// When application is DefTerm host, it must be able to process "-new_console" switches.
		// Especially the "-new_console:z" switch. But the m_SrvMapping is not filled in most cases.
		bHooksSkipNewConsole = !gbPrepareDefaultTerminal
			&& (m_SrvMapping.cbSize && !(m_SrvMapping.Flags & ConEmu::ConsoleFlags::ProcessNewCon));
		// "start /?" is a "cmd.exe" feature mainly. User may want to disable "start" processing
		bHooksSkipCmdStart = (m_SrvMapping.cbSize && !(m_SrvMapping.Flags & ConEmu::ConsoleFlags::ProcessCmdStart));
	}

	mb_Opt_DontInject = bHooksTempDisabled;
	mb_Opt_SkipNewConsole = bHooksSkipNewConsole;
	mb_Opt_SkipCmdStart = bHooksSkipCmdStart;
}

BOOL CShellProc::ChangeExecuteParams(enum CmdOnCreateType aCmd,
				LPCWSTR asFile, LPCWSTR asParam,
				ChangeExecFlags Flags, const RConStartArgs& args,
				DWORD& ImageBits, DWORD& ImageSubsystem,
				LPWSTR* psFile, LPWSTR* psParam)
{
	if (!LoadSrvMapping())
		return FALSE;

	bool lbRc = false;
	wchar_t *szComspec = nullptr;
	wchar_t *pszOurExe = nullptr; // ConEmuC64.exe или ConEmu64.exe (для DefTerm)
	BOOL lbUseDosBox = FALSE;
	CEStr szDosBoxExe, szDosBoxCfg;
	BOOL lbComSpec = FALSE; // TRUE - если %COMSPEC% отбрасывается
	size_t nCchSize = 0;
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

	if (aCmd == eCreateProcess)
	{
		if (asFile && !*asFile)
			asFile = nullptr;

		// Кто-то может додуматься окавычить asFile
		wchar_t* pszFileUnquote = nullptr;
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
				int cchMax = std::max(nLen,(MAX_PATH+1));
				CEStr testBuffer;
				if (testBuffer.GetBuffer(cchMax))
				{
					// Сначала пробуем на полное соответствие
					if (asFile)
					{
						_wcscpyn_c(testBuffer.data(), nLen, (*pszParam == L'"') ? (pszParam+1) : pszParam, nLen); //-V501
						testBuffer.SetAt(nLen - 1, 0);
						// Сравнить asFile с первым аргументом в asParam
						if (lstrcmpi(testBuffer.c_str(), asFile) == 0)
						{
							// exe-шник уже указан в asParam, добавлять дополнительно НЕ нужно
							asFile = nullptr;
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
							CmdArg  szFirst;
							if (!((pszCopy = NextArg(pszCopy, szFirst))))
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
									// -- asFile = nullptr; -- трогать нельзя, только он содержит полный путь!
									asParam = pszCopy;
									pszFileOnly = nullptr;
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
											// -- asFile = nullptr; -- трогать нельзя, только он содержит полный путь!
											asParam = pszCopy;
											pszFileOnly = nullptr;
										}
										else if (wcsrchr(szFirst, L'.'))
										{
											if (lstrcmpi(pszFirstName, szTmpFileOnly) == 0)
											{
												// exe-шник уже указан в asParam, добавлять дополнительно НЕ нужно
												// -- asFile = nullptr; -- трогать нельзя, только он содержит полный путь!
												asParam = pszCopy;
												pszFileOnly = nullptr;
											}
										}
									}
								}
							}
						}
					}
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
						asFile = nullptr;
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
				// asFile уже сброшен в nullptr, если он совпадает с первым аргументом в asParam
				// Возможны два варианта asParam (как минимум)
				// "c:\windows\system32\cmd.exe" /c dir
				// "c:\windows\system32\cmd.exe /c dir"
				// Второй - пока проигнорируем, как маловероятный
				INT_PTR nLen = lstrlen(szComspec) + 1;
				CEStr testBuffer;
				if (testBuffer.GetBuffer(nLen))
				{
					_wcscpyn_c(testBuffer.data(), nLen, (*psz == L'"') ? (psz+1) : psz, nLen); //-V501
					testBuffer.SetAt(nLen - 1, 0);
					// Сравнить первый аргумент в asParam с %COMSPEC%
					const wchar_t* pszCmdLeft = nullptr;
					if (lstrcmpi(testBuffer.c_str(), szComspec) == 0)
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
							_ASSERTE(asFile == nullptr); // уже должен быть nullptr
							asParam = pszCmdLeft+2; // /C или /K добавляется к ConEmuC.exe
							lbNewCmdCheck = TRUE;

							lbComSpec = TRUE;
						}
					}
				}
			}
		}

		if (lbNewCmdCheck)
		{
			NeedCmdOptions opt{};
			//DWORD nFileAttrs = (DWORD)-1;
			ms_ExeTmp.Clear();
			IsNeedCmd(false, SkipNonPrintable(asParam), ms_ExeTmp, &opt);
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
					switch (m_SrvMapping.ComSpec.csBits)  // NOLINT(clang-diagnostic-switch-enum)
					{
					case csb_SameOS:
						ImageBits = IsWindows64() ? 64 : 32;
						break;
					case csb_x32:  // NOLINT(bugprone-branch-clone)
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

	if ((ImageBits != 16) && lbComSpec && asParam && *asParam)
	{
		// Could it be Dos-application starting with "cmd /c ..."?
		if (ImageSubsystem != IMAGE_SUBSYSTEM_DOS_EXECUTABLE)
		{
			LPCWSTR pszCmdLine = asParam;

			ms_ExeTmp.Clear();
			if ((pszCmdLine = NextArg(pszCmdLine, ms_ExeTmp)))
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
							_ASSERTEX(asFile==nullptr);
						}
					}
				}
			}
		}
	}

	if (gbPrepareDefaultTerminal)
	{
		lbUseDosBox = FALSE; // Don't set now
		if (mb_ConsoleMode)
		{
			pszOurExe = lstrmerge(m_SrvMapping.ComSpec.ConEmuBaseDir, L"\\", (ImageBits == 32) ? L"ConEmuC.exe" : L"ConEmuC64.exe"); //-V112
		}
		else
		{
			pszOurExe = lstrdup(m_SrvMapping.sConEmuExe);
		}
	}
	else if ((ImageBits == 32) || (ImageBits == 64)) //-V112
	{
		if (!mb_ConsoleMode)
			SetConsoleMode(true);

		pszOurExe = lstrmerge(m_SrvMapping.ComSpec.ConEmuBaseDir, L"\\", (ImageBits == 32) ? L"ConEmuC.exe" : L"ConEmuC64.exe");
	}
	else if (ImageBits == 16)
	{
		if (!mb_ConsoleMode)
			SetConsoleMode(true);

		if (m_SrvMapping.cbSize && (m_SrvMapping.Flags & ConEmu::ConsoleFlags::DosBox))
		{
			szDosBoxExe.Attach(JoinPath(m_SrvMapping.ComSpec.ConEmuBaseDir, L"\\DosBox\\DosBox.exe"));
			szDosBoxCfg.Attach(JoinPath(m_SrvMapping.ComSpec.ConEmuBaseDir, L"\\DosBox\\DosBox.conf"));

			if (!FileExists(szDosBoxExe) || !FileExists(szDosBoxCfg))
			{
				// DoxBox is not installed
				lbRc = FALSE;
				goto wrap;
			}

			// DosBox.exe - 32-bit process
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
		_ASSERTE(pszOurExe!=nullptr);
		lbRc = FALSE;
		goto wrap;
	}

	// We change application to our executable (GUI or ConEmuC), so don't inject ConEmuHk there
	SetNeedInjects(false);

	nCchSize = (asFile ? wcslen(asFile) : 0) + (asParam ? wcslen(asParam) : 0) + 64;
	if (lbUseDosBox)
	{
		// Escaping of special symbols, dosbox arguments, etc.
		nCchSize += (nCchSize)
			+ wcslen(szDosBoxExe) + wcslen(szDosBoxCfg) + 128
			+ (MAX_PATH * 2/*cd and others*/);
	}

	if (m_SrvMapping.nLogLevel)
	{
		nCchSize += 5; // + " /LOG"
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
		nCchSize += wcslen(pszOurExe) + 1;
		*psFile = nullptr;
	}

	#if 0
	// Если запускается новый GUI как вкладка?
	lbNewGuiConsole = (ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI) || (ghAttachGuiClient != nullptr);
	#endif

	// Starting CONSOLE application from GUI tab? This affect "Default terminal" too.
	lbNewConsoleFromGui = (ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI) && (gbPrepareDefaultTerminal || mb_isCurrentGuiClient);

	#if 0
	if (lbNewGuiConsole)
	{
		// add /ATTACH /GID=%i, etc.
		nCchSize += 128;
	}
	#endif
	if (lbNewConsoleFromGui)
	{
		// add /ATTACH /GID=%i, etc.
		nCchSize += 128;
	}
	if (args.InjectsDisable == crb_On)
	{
		// add " /NOINJECT"
		nCchSize += 12;
	}

	if (gFarMode.cbSize && gFarMode.bFarHookMode)
	{
		// add /PARENTFARPID=%u
		nCchSize += 32;
	}

	if (gbPrepareDefaultTerminal)
	{
		// reserve space for Default terminal arguments (server additional)
		// "runas" (Shell) - start server
		// CreateProcess - start GUI (ConEmu.exe)
		_ASSERTEX(aCmd==eCreateProcess || aCmd==eShellExecute);
		nCchSize += CDefTermHk::GetSrvAddArgs(!mb_ConsoleMode, mb_ForceInjectOriginal, szDefTermArg, szDefTermArg2);
	}

	if (Flags & CEF_NEWCON_PREPEND)
	{
		nCchSize += 16; // + "-new_console " for "start" processing
	}

	// В ShellExecute необходимо "ConEmuC.exe" вернуть в psFile, а для CreatePocess - в psParam
	// /C или /K в обоих случаях нужно пихать в psParam
	_ASSERTE(lbEndQuote == FALSE); // Must not be set yet
	*psParam = static_cast<wchar_t*>(malloc(nCchSize*sizeof(wchar_t)));
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

	if (m_SrvMapping.nLogLevel)
		_wcscat_c((*psParam), nCchSize, L" /LOG");

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

		if (mb_ConsoleMode)
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
			LPWSTR pszParam = nullptr;
			if (!asFile || !*asFile)
			{
				// exe-шника в asFile указано НЕ было, значит он в asParam, нужно его вытащить, и сформировать команду DosBox
				NeedCmdOptions opt{};
				ms_ExeTmp.Clear();
				IsNeedCmd(false, SkipNonPrintable(asParam), ms_ExeTmp, &opt);

				if (ms_ExeTmp[0])
				{
					LPCWSTR pszQuot = SkipNonPrintable(asParam);
					if (opt.needCutStartEndQuot)
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
				CmdArg szTest;
				if ((pszTest = NextArg(pszTest, szTest)))
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
					pszNewPtr = nullptr;
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
		CEStr dbgMsg;
		if (dbgMsg.GetBuffer(cchLen))
		{
			msprintf(dbgMsg.data(), cchLen, L"RunChanged(ParentPID=%u): %s <%s> <%s>",
				GetCurrentProcessId(),
				(aCmd == eShellExecute) ? L"Shell" : (aCmd == eCreateProcess) ? L"Create" : L"???",
				*psFile ? *psFile : L"", *psParam ? *psParam : L"");
			LogShellString(dbgMsg);
		}
	}

	lbRc = TRUE;
wrap:
	if (szComspec)
		free(szComspec);
	if (pszOurExe)
		free(pszOurExe);
	return lbRc;
}

void CShellProc::CheckIsCurrentGuiClient()
{
	// gbAttachGuiClient is TRUE if some application (GUI subsystem) was not created window yet
	// this is, for example, CommandPromptPortable.exe
	// ghAttachGuiClient is HWND of already created GUI child client window
	mb_isCurrentGuiClient = ((gbAttachGuiClient != FALSE) || (ghAttachGuiClient != nullptr));
}

bool CShellProc::IsAnsiConLoader(LPCWSTR asFile, LPCWSTR asParam)
{
	bool bAnsiCon = false;
	LPCWSTR params[] = { asFile, asParam };

	for (const auto* psz : params)
	{
		if (!psz || !*psz)
			continue;

		#ifdef _DEBUG
		bool bAnsiConFound = false;
		LPCWSTR pszDbg = psz;
		ms_ExeTmp.Clear();
		if ((pszDbg = NextArg(pszDbg, ms_ExeTmp)))
		{
			CharUpperBuff(ms_ExeTmp.ms_Val, lstrlen(ms_ExeTmp));
			LPCWSTR names[] = { L"ANSI-LLW", L"ANSICON" };
			for (const auto* name : names)
			{
				pszDbg = wcsstr(ms_ExeTmp, name);
				if (pszDbg && (pszDbg[lstrlen(name)] != L'\\'))
				{
					bAnsiConFound = true;
					break;
				}
			}
		}
		#endif

		ms_ExeTmp.Clear();
		if (!NextArg(psz, ms_ExeTmp))
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
		WriteConsoleW(hStdOut, sErrMsg, nLen, &nLen, nullptr);
		LogShellString(L"ANSICON was blocked");
		return true;
	}

	return false;
}

// In some cases we need to pre-replace command line,
// for example, in cmd prompt: start -new_console:z
bool CShellProc::PrepareNewConsoleInFile(
	const CmdOnCreateType aCmd, LPCWSTR& asFile, LPCWSTR& asParam, CEStr& lsReplaceFile, CEStr& lsReplaceParm, CEStr& exeName)
{
	if (!asFile || !*asFile)
		return false;
	if ((wcsstr(asFile, L"-new_console") == nullptr)
		&& (wcsstr(asFile, L"-cur_console") == nullptr))
		return false;

	bool isNewConsoleArg = false;
	// To be sure, it's our "-new_console" or "-cur_console" switch,
	// but not a something else...
	RConStartArgs lTestArg;
	lTestArg.pszSpecialCmd = lstrdup(asFile);
	if (lTestArg.ProcessNewConArg() > 0)
	{
		// pszSpecialCmd is supposed to be empty now, because asFile can contain only one "token"
		_ASSERTE(lTestArg.pszSpecialCmd == nullptr || *lTestArg.pszSpecialCmd == 0);
		lsReplaceParm = lstrmerge(asFile, (asFile && *asFile && asParam && *asParam) ? L" " : nullptr, asParam);
		_ASSERTE(!lsReplaceParm.IsEmpty());
		if (gbIsCmdProcess
			&& !GetStartingExeName(nullptr, lsReplaceParm, exeName)
			)
		{
			// when just "start" is executed from "cmd.exe" - it starts itself in the new console
			CEStr lsTempCmd;
			if (GetModulePathName(nullptr, lsTempCmd))
			{
				exeName.Set(PointToName(lsTempCmd));
				// It's supposed to be "cmd.exe", so there must not be spaces in file name
				_ASSERTE(!wcschr(exeName, L' '));
				// Insert "cmd.exe " before "-new_console" switches for clearness
				if (aCmd == eCreateProcess)
				{
					lsReplaceParm = lstrmerge(exeName, L" ", lsReplaceParm);
				}
			}
		}
		if (!exeName.IsEmpty())
		{
			lsReplaceFile.Set(exeName.ms_Val);
			asFile = lsReplaceFile.ms_Val;
		}
		isNewConsoleArg = true;
		asParam = lsReplaceParm.ms_Val;
	}

	return isNewConsoleArg;
}

bool CShellProc::CheckForDefaultTerminal(
	const CmdOnCreateType aCmd, LPCWSTR asAction, const DWORD* anShellFlags, const DWORD* anCreateFlags,
	const DWORD* anShowCmd, bool& bIgnoreSuspended, bool& bDebugWasRequested, bool& lbGnuDebugger)
{
	if (!gbPrepareDefaultTerminal)
		return true; // nothing to do

	lbGnuDebugger = IsGDB(ms_ExeTmp); // Allow GDB in Lazarus etc.

	if (IsVsDebugConsoleExe(ms_ExeTmp))
	{
		SetForceInjectOriginal(true);
	}

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
				LogShellString(L"Forcing mb_NeedInjects for cmd.exe started from VisualStudio Code");
				SetNeedInjects(true);
				SetConsoleMode(true);
			}

			// Creating process without console window, not our case
			LogExit(0);
			return false;
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
					SetDebugWasRequested(true);
					LogExit(0);
					return false;
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
			return false;
		}
		// Skip some executions
		if (anShellFlags
			&& ((*anShellFlags) & (SEE_MASK_CLASSNAME|SEE_MASK_CLASSKEY|SEE_MASK_IDLIST|SEE_MASK_INVOKEIDLIST)))
		{
			LogExit(0);
			return false;
		}
	}
	else
	{
		_ASSERTE(FALSE && "Unsupported in Default terminal");
		LogExit(0);
		return false;
	}

	if (anShowCmd && (*anShowCmd == SW_HIDE)
		&& !lbGnuDebugger
		)
	{
		// Creating process with window initially hidden, not our case
		LogExit(0);
		return false;
	}

	// Started from explorer/taskmgr - CREATE_SUSPENDED is set (why?)
	if ((workOptions_ & ShellWorkOptions::WasSuspended) && anCreateFlags)
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
		if (m_SrvMapping.Flags & ConEmu::ConsoleFlags::BlockChildDbg)
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

	return true;
}

void CShellProc::CheckForExeName(const CEStr& exeName, const DWORD* anCreateFlags, bool lbGnuDebugger,
		bool& bDebugWasRequested, bool& lbGuiApp, bool& bVsNetHostRequested)
{
	if (exeName.IsEmpty())
		return;

	const auto nLen = exeName.GetLen();
	// Non empty string which does not end with backslash - may be a file
	const BOOL lbMayBeFile = (nLen > 0) && (exeName[nLen - 1] != L'\\') && (exeName[nLen - 1] != L'/');

	mn_ImageBits = 0;
	mn_ImageSubsystem = IMAGE_SUBSYSTEM_UNKNOWN;

	if (!lbMayBeFile)
	{
		mn_ImageBits = 0;
		mn_ImageSubsystem = IMAGE_SUBSYSTEM_UNKNOWN;
	}
	else if (FindImageSubsystem(exeName, mn_ImageSubsystem, mn_ImageBits))
	{
		// gh-681: NodeJSPortable.exe just runs "Server.cmd"
		if (mn_ImageSubsystem == IMAGE_SUBSYSTEM_BATCH_FILE)
			mn_ImageSubsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
		lbGuiApp = (mn_ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI);
		if (gbPrepareDefaultTerminal)
		{
			if (IsVsNetHostExe(exeName))
			{
				// *.vshost.exe
				bVsNetHostRequested = true;
				// Intended for .Net debugging?
				if (anCreateFlags && ((*anCreateFlags) & CREATE_SUSPENDED))
				{
					bDebugWasRequested = true;
				}
			} // end of check "starting *.vshost.exe" (seems like not used in latest VS & .Net)
			else if (gbIsVSDebug && (mn_ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI))
			{
				// Intended for .Net debugging (without native support)
				if (anCreateFlags && ((*anCreateFlags) & CREATE_SUSPENDED)
					// DEBUG_XXX flags are processed above explicitly
					&& !((*anCreateFlags) & (DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS)))
				{
					bDebugWasRequested = true;
				}
			} // end of check "starting C# console app from msvsmon.exe"
			else if (lbGnuDebugger)
			{
				bDebugWasRequested = true;
				SetPostInjectWasRequested(true);
			} // end of check "starting new gnu debugger"
			else if (CompareProcessNames(gsExeName, exeName))
			{
				// Idle from (Pythonw.exe, gh-457), VisualStudio Code (code.exe), and so on
				//
				// > C:\tools\Python27\pythonw.exe C:\tools\Python27\Lib\idlelib\idle.pyw
				// and in the pytonw window
				// > import subprocess
				// > subprocess.call([r'C:\tools\Python27\python.exe'])
				//
				// -- bVsNetHostRequested = true;
				const CEStr lsMsg(L"Forcing mb_NeedInjects for `", exeName, L"` started from `", gsExeName, L"`");
				LogShellString(lsMsg);
				SetNeedInjects(true);
			} // end of check "<starting exe> == <current exe>"
		}
	}

#ifdef _DEBUG
	const LPCWSTR pszName = PointToName(exeName);
	if (lstrcmpi(pszName, L"ANSI-LLW.exe") == 0)
	{
		_ASSERTEX(FALSE && "Trying to start 'ANSI-LLW.exe'");
	}
	else if (lstrcmpi(pszName, L"ansicon.exe") == 0)
	{
		_ASSERTEX(FALSE && "Trying to start 'ansicon.exe'");
	}
#endif
}

CShellProc::PrepareExecuteResult CShellProc::PrepareExecuteParams(
			enum CmdOnCreateType aCmd,
			LPCWSTR asAction, LPCWSTR asFile, LPCWSTR asParam, LPCWSTR asDir,
			DWORD* anShellFlags, DWORD* anCreateFlags, DWORD* anStartFlags, DWORD* anShowCmd,
			HANDLE* lphStdIn, HANDLE* lphStdOut, HANDLE* lphStdErr,
			LPWSTR* psFile, LPWSTR* psParam, LPWSTR* psStartDir)
{
	// !!! anFlags может быть nullptr;
	// !!! asAction может быть nullptr;
	_ASSERTE(*psFile==nullptr && *psParam==nullptr);
	if (!IsInterceptionEnabled())
	{
		LogShellString(L"PrepareExecuteParams skipped");
		return PrepareExecuteResult::Bypass; // In ConEmu only
	}

	if (!asFile && !asParam)
	{
		LogShellString(L"PrepareExecuteParams skipped (null params)");
		return PrepareExecuteResult::Bypass; // That is most probably asAction="OpenNewWindow"
	}

	// Just in case of unexpected LastError changes
	ScopedObject(CLastErrorGuard);

	// Log with ConEmuCD when Far is working as Alternative Server
	if (gFarMode.cbSize && gFarMode.bFarHookMode)
	{
		LogFarExecCommand(
			aCmd, asAction, asFile, asParam, asDir,
			anShellFlags, anCreateFlags, anStartFlags, anShowCmd,
			lphStdIn, lphStdOut, lphStdErr);
	}

	CEStr szLnkExe, szLnkArg, szLnkDir;
	if (asFile && (aCmd == eShellExecute))
	{
		LPCWSTR pszExt = PointToExt(asFile);
		if (pszExt && (lstrcmpi(pszExt, L".lnk") == 0))
		{
			if (GetLinkProperties(asFile, szLnkExe, szLnkArg, szLnkDir))
			{
				_ASSERTE(asParam == nullptr);
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

	if (IsAnsiConLoader(asFile, asParam))
	{
		return PrepareExecuteResult::Restrict;
	}

	ms_ExeTmp.Clear();

	BOOL bGoChangeParm = FALSE;
	SetConsoleMode(false);
	if ((aCmd == eShellExecute) && (asAction && (lstrcmpi(asAction, L"runas") == 0)))
	{
		// Always run ConEmuC instead of GUI
		SetConsoleMode(true);
	}

	// DefTerm logging
	wchar_t szInfo[200];

	{ // Log PrepareExecuteParams function call -begin
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
		(aCmd == eShellExecute) ? L"PrepareExecuteParams Shell " : L"PrepareExecuteParams Create ",
		szInfo, asFile, asParam);
	LogShellString(lsLog);
	} // Log PrepareExecuteParams function call -end

	// Для логирования - запомним сразу
	HANDLE hIn  = lphStdIn  ? *lphStdIn  : nullptr;
	HANDLE hOut = lphStdOut ? *lphStdOut : nullptr;
	HANDLE hErr = lphStdErr ? *lphStdErr : nullptr;
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
			SetConsoleMode(true);
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
	BOOL bLongConsoleOutput = gFarMode.farVer.dwVerMajor && gFarMode.bFarHookMode && gFarMode.bLongConsoleOutput && !bDetachedOrHidden;

	// Current application is GUI subsystem run in ConEmu tab?
	CheckIsCurrentGuiClient();

	ChangeExecFlags NewConsoleFlags = CEF_DEFAULT;
	bool bForceNewConsole = false, bCurConsoleArg = false;

	// Service object (moved to members: RConStartArgs m_Args)
	_ASSERTEX(m_Args.pszSpecialCmd == nullptr); // Must not be touched yet!

	bool bDebugWasRequested = false;
	bool bVsNetHostRequested = false;
	bool bIgnoreSuspended = false;
	_ASSERTE(mb_DebugWasRequested == FALSE);
	_ASSERTE(mb_PostInjectWasRequested == FALSE);

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
				&& (*lphStdErr == nullptr))
			{
				*lphStdOut = nullptr;
				*anStartFlags &= ~0x400;
			}
		}
	}

	// Проверяем настройку ConEmuGuiMapping.useInjects
	// #HOOKS Check if there is -new_console, if so - allow even if ConEmuUseInjects::DontUse
	if (!LoadSrvMapping() || !(m_SrvMapping.cbSize && ((m_SrvMapping.useInjects == ConEmuUseInjects::Use) || gbPrepareDefaultTerminal)))
	{
		// -- зависимо только от флажка "Use Injects", иначе нельзя управлять добавлением "ConEmuC.exe" из ConEmu --
		// Настройка в ConEmu ConEmuGuiMapping.useInjects, или gFarMode.bFarHookMode. Иначе - сразу выходим
		if (!bLongConsoleOutput)
		{
			LogExit(0);
			return PrepareExecuteResult::Bypass;
		}
	}

	// "ConEmuHooks=OFF" - don't inject the created process
	// "ConEmuHooks=NOARG" - don't process -new_console and -cur_console args
	// "ConEmuHooks=..." - any other - all enabled
	CheckHooksDisabled();

	// We need the get executable name before some other checks
	mn_ImageSubsystem = mn_ImageBits = 0;
	bool bForceCutNewConsole = false;

	// In some cases we need to pre-replace command line,
	// for example, in cmd prompt: start -new_console:z
	CEStr lsReplaceFile, lsReplaceParm;
	ms_ExeTmp.Clear();
	bForceCutNewConsole |= PrepareNewConsoleInFile(aCmd, asFile, asParam, lsReplaceFile, lsReplaceParm, ms_ExeTmp);

	if (ms_ExeTmp.IsEmpty())
	{
		GetStartingExeName(asFile, asParam, ms_ExeTmp);
	}

	bool lbGnuDebugger = false;

	// Some additional checks for "Default terminal" mode
	if (!CheckForDefaultTerminal(
		aCmd, asAction, anShellFlags, anCreateFlags, anShowCmd,
		bIgnoreSuspended, bDebugWasRequested, lbGnuDebugger))
	{
		return PrepareExecuteResult::Bypass;
	}

	// #HOOKS try to process *.lnk files?

	//wchar_t *szTest = (wchar_t*)malloc(MAX_PATH*2*sizeof(wchar_t)); //[MAX_PATH*2]
	//wchar_t *szExe = (wchar_t*)malloc((MAX_PATH+1)*sizeof(wchar_t)); //[MAX_PATH+1];
	//DWORD /*mn_ImageSubsystem = 0, mn_ImageBits = 0,*/ nFileAttrs = (DWORD)-1;
	bool lbGuiApp = false;
	//int nActionLen = (asAction ? lstrlen(asAction) : 0)+1;
	//int nFileLen = (asFile ? lstrlen(asFile) : 0)+1;
	//int nParamLen = (asParam ? lstrlen(asParam) : 0)+1;

	if (!ms_ExeTmp.IsEmpty())
	{
		// check image subsystem, vsnet debugger helpers, etc.
		CheckForExeName(ms_ExeTmp, anCreateFlags, lbGnuDebugger, bDebugWasRequested, lbGuiApp, bVsNetHostRequested);
	}

	BOOL lbChanged = FALSE;
	CESERVER_REQ *pIn = nullptr;
	pIn = NewCmdOnCreate(aCmd,
			asAction, asFile, asParam, asDir,
			anShellFlags, anCreateFlags, anStartFlags, anShowCmd,
			mn_ImageBits, mn_ImageSubsystem,
			hIn, hOut, hErr/*, szBaseDir, mb_DosBoxAllowed*/);
	if (pIn)
	{
		HWND hConWnd = GetRealConsoleWindow();
		CESERVER_REQ *pOut = nullptr;
		pOut = ExecuteGuiCmd(hConWnd, pIn, hConWnd);
		ExecuteFreeResult(pIn); pIn = nullptr;
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
		SetNeedInjects(FALSE);
		goto wrap;
	}

	// logging
	{
		int cchLen = (asFile ? lstrlen(asFile) : 0) + (asParam ? lstrlen(asParam) : 0) + 128;
		CEStr dbgMsg;
		if (dbgMsg.GetBuffer(cchLen))
		{
			msprintf(dbgMsg.data(), cchLen, L"Run(ParentPID=%u): %s <%s> <%s>",
				GetCurrentProcessId(),
				(aCmd == eShellExecute) ? L"Shell" : (aCmd == eCreateProcess) ? L"Create" : L"???",
				asFile ? asFile : L"", asParam ? asParam : L"");
			LogShellString(dbgMsg);
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
				// The "-cur_console" we have to process _here_
				bCurConsoleArg = true;

				if ((m_Args.ForceDosBox == crb_On) && m_SrvMapping.cbSize && (m_SrvMapping.Flags & ConEmu::ConsoleFlags::DosBox))
				{
					mn_ImageSubsystem = IMAGE_SUBSYSTEM_DOS_EXECUTABLE;
					mn_ImageBits = 16;
					bLongConsoleOutput = FALSE;
					lbGuiApp = false;
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
		&& ((anShowCmd == nullptr) || (*anShowCmd != SW_HIDE)))
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
				gnAttachPortableGuiCui = static_cast<GuiCui>(IMAGE_SUBSYSTEM_WINDOWS_CUI);
				SetNeedInjects(TRUE);
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
		&& ((anShowCmd == nullptr)
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
		// MultiArc issue. Don't turn on LongConsoleOutput for "windowless" archive operations
		// gh-1307: In Win10 ShellExecuteEx executes CreateProcess from background thread
		if (!((aCmd == eCreateProcess) && (gnInShellExecuteEx > 0))
			&& (GetCurrentThreadId() != gnHookMainThreadId)
			)
		{
			bLongConsoleOutput = FALSE;
		}
	}

	if (aCmd == eShellExecute)
	{
		WARNING("Уточнить условие для флагов ShellExecute!");
		// !!! anFlags может быть nullptr;
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
					_ASSERTE(anShellFlags!=nullptr);
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
			SetNeedInjects(FALSE);
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
			SetNeedInjects(TRUE);
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
	// Force run through ConEmuC.exe. Only for debugging!
	bool lbAlwaysAddConEmuC = false;
	#define isDebugAddConEmuC lbAlwaysAddConEmuC
	#else
	#define isDebugAddConEmuC false
	#endif

	if ((mn_ImageBits == 0) && (mn_ImageSubsystem == 0) && !isDebugAddConEmuC)
	{
		// This could be the document (ShellExecute), e.g. .doc or .sln file
		goto wrap;
	}

	_ASSERTE(mn_ImageBits!=0);

	if (gbPrepareDefaultTerminal)
	{
		// set up default terminal
		bGoChangeParm = ((m_Args.NoDefaultTerm != crb_On) && (bVsNetHostRequested || mn_ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI || mn_ImageSubsystem == IMAGE_SUBSYSTEM_BATCH_FILE));
		if (bGoChangeParm)
		{
			// when we already have ConEmu inside - no sense to start GUI, just start new console server (tab)
			if (!mb_ConsoleMode && CDefTermHk::IsInsideMode())
			{
				SetConsoleMode(true);
			}
			// if hooks were requested by DefTerm options
			if (gpDefTerm)
			{
				if (gpDefTerm->GetOpt().bNoInjects)
				{
					// injects were disabled in DefTerm options
					SetNeedInjects(false);
					SetForceInjectOriginal(false);
				}
				else
				{
					// allow ConEmuHk to be injected into debugged process
					SetForceInjectOriginal(true);
				}
			}
		}
	}
	else
	{
		for (bool once = true; once; once = false)
		{
			// it's an attach of ChildGui into new ConEmu tab OR new console from GUI
			if (lbGuiApp && (NewConsoleFlags || bForceNewConsole))
			{
				bGoChangeParm = true; break;
			}

			// Native console applications
			if ((mn_ImageBits != 16) && (m_SrvMapping.useInjects == ConEmuUseInjects::Use))
			{
				if (NewConsoleFlags // CEF_NEWCON_SWITCH | CEF_NEWCON_PREPEND
					|| (bCurConsoleArg && (m_Args.LongOutputDisable != crb_On)) // "-cur_console", except "-cur_console:o"
					|| isDebugAddConEmuC // debugging only
					)
				{
					bGoChangeParm = true; break;
				}
				if (bLongConsoleOutput) // Far Manager, support "View console output" from Far Plugin
				{
					const auto& ver = gFarMode.farVer;
					// Certain builds of Far Manager 3.x use ShellExecute
					if ((aCmd == eShellExecute)
						&& (ver.dwVerMajor >= 3)
						&& (anShellFlags && (*anShellFlags & SEE_MASK_NO_CONSOLE)) && (anShowCmd && *anShowCmd))
					{
						bGoChangeParm = true; break;
					}
					// Others Far versions - CreateProcess for console applications
					const bool createDefaultErrorMode = (aCmd == eCreateProcess) && (anCreateFlags && (*anCreateFlags & CREATE_DEFAULT_ERROR_MODE));
					if (createDefaultErrorMode
						// gh-1307: when user runs "script.py" it's executed by Far3 via ShellExecuteEx->CreateProcess(py.exe),
						// we don't know it's a console process at the moment of ShellExecuteEx
						&& ((ver.dwVerMajor <= 2)
							|| ((gnInShellExecuteEx > 0) && (ver.dwVerMajor >= 3))
							// gh-2201: Far 3 build 5709 executor changes
							|| ((gnInShellExecuteEx == 0) && ((ver.dwVerMajor == 3 && ver.dwBuild >= 5709) || (ver.dwVerMajor >= 3)))
							))
					{
						bGoChangeParm = true; break;
					}
				}
			}

			// If this is old DOS application and the DosBox is enabled also insert ConEmuC.exe /DOSBOX
			if ((mn_ImageBits == 16) && (mn_ImageSubsystem == IMAGE_SUBSYSTEM_DOS_EXECUTABLE)
				&& m_SrvMapping.cbSize && (m_SrvMapping.Flags & ConEmu::ConsoleFlags::DosBox))
			{
				bGoChangeParm = true; break;
			}
		}
	}

	if (bGoChangeParm)
	{
		if (gbPrepareDefaultTerminal)
		{
			// normal DefTerm feature
			if (mb_ConsoleMode && anShowCmd)
			{
				*anShowCmd = SW_HIDE;
			}

			if (bDebugWasRequested)
			{
				// We need to post attach ConEmu GUI to started console
				if (bVsNetHostRequested)
					SetPostInjectWasRequested(true);
				else
					SetDebugWasRequested(true);
				// Пока что не будем убирать "мелькание" окошка.
				// На факт "видимости" консольного окна ориентируется ConEmuC
				// при аттаче. Если окошко НЕ видимое - считаем, что оно было
				// запущено процессом для служебных целей, и не трогаем его...
				// -> ConsoleMain.cpp: ParseCommandLine: "if (!ghConWnd || !(lbIsWindowVisible = IsWindowVisible(ghConWnd)) || isTerminalMode())"
				goto wrap;
			}
		}

		_ASSERTE(lbChanged == false);

		lbChanged = ChangeExecuteParams(aCmd, asFile, asParam,
			NewConsoleFlags, m_Args, mn_ImageBits, mn_ImageSubsystem, psFile, psParam);

		if (!lbChanged)
		{
			// Хуки нельзя ставить в 16битные приложение - будет облом, ntvdm.exe игнорировать!
			// И если просили не ставить хуки (-new_console:i) - тоже
			SetNeedInjects((mn_ImageBits != 16) && (m_Args.InjectsDisable != crb_On));
		}
		else
		{
			// Замена на "ConEmuC.exe ...", все "-new_console" / "-cur_console" будут обработаны в нем
			bCurConsoleArg = false;

			HWND hConWnd = GetRealConsoleWindow();

			if (!mb_ConsoleMode)
			{
				// If we start ConEmu.exe (our GUI) application, we need to show it
				if (anShowCmd && *anShowCmd == SW_HIDE)
				{
					*anShowCmd = SW_SHOWNORMAL;
				}
			}
			else if (lbGuiApp && (NewConsoleFlags || bForceNewConsole))
			{
				if (anShowCmd)
				{
					*anShowCmd = SW_HIDE;
				}

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
				CESERVER_REQ *pOut = nullptr;
				pOut = ExecuteGuiCmd(hConWnd, pIn, hConWnd);
				ExecuteFreeResult(pIn); pIn = nullptr;
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
		SetNeedInjects((aCmd == eCreateProcess) && (mn_ImageBits != 16)
			&& (m_Args.InjectsDisable != crb_On) && !gbPrepareDefaultTerminal
			&& !bDontForceInjects);

		// Параметр -cur_console / -new_console нужно вырезать
		if (NewConsoleFlags || bCurConsoleArg)
		{
			_ASSERTEX(m_Args.pszSpecialCmd!=nullptr && "Must be replaced already!");

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
				m_Args.pszStartupDir = nullptr;
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
				m_Args.pszSpecialCmd = nullptr;
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
						//SHORT nNewHeight = std::max((csbi.srWindow.Bottom - csbi.srWindow.Top + 1),(SHORT)m_Args.nBufHeight);
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
	return lbChanged ? PrepareExecuteResult::Modified : PrepareExecuteResult::Bypass;
} // PrepareExecuteParams

bool CShellProc::GetStartingExeName(LPCWSTR asFile, LPCWSTR asParam, CEStr& rsExeTmp)
{
	rsExeTmp.Clear();

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
		IsNeedCmd(false, SkipNonPrintable(asParam), rsExeTmp);
	}

	return (!rsExeTmp.IsEmpty());
}

// returns FALSE if need to block execution
BOOL CShellProc::OnShellExecuteA(LPCSTR* asAction, LPCSTR* asFile, LPCSTR* asParam, LPCSTR* asDir, DWORD* anFlags, DWORD* anShowCmd)
{
	if (!IsInterceptionEnabled())
	{
		LogShellString(L"OnShellExecuteA skipped");
		return TRUE; // don't intercept, pass to kernel
	}

	if (GetCurrentThreadId() == gnHookMainThreadId)
	{
		mb_InShellExecuteEx = TRUE;
		gnInShellExecuteEx ++;
	}

	mpwsz_TempAction = str2wcs(asAction ? *asAction : nullptr, mn_CP);
	mpwsz_TempFile = str2wcs(asFile ? *asFile : nullptr, mn_CP);
	mpwsz_TempParam = str2wcs(asParam ? *asParam : nullptr, mn_CP);
	const CEStr lsDir(str2wcs(asDir ? *asDir : nullptr, mn_CP));

	_ASSERTEX(!mpwsz_TempRetFile && !mpwsz_TempRetParam && !mpwsz_TempRetDir);

	const auto prepareResult = PrepareExecuteParams(eShellExecute,
					mpwsz_TempAction, mpwsz_TempFile, mpwsz_TempParam, lsDir,
					anFlags, nullptr, nullptr, anShowCmd,
					nullptr, nullptr, nullptr, // *StdHandles
					&mpwsz_TempRetFile, &mpwsz_TempRetParam, &mpwsz_TempRetDir);
	if (prepareResult == PrepareExecuteResult::Restrict)
		return false;

	const bool changed = (prepareResult == PrepareExecuteResult::Modified);

	if (mpwsz_TempRetFile && *mpwsz_TempRetFile)
	{
		mpsz_TempRetFile = wcs2str(mpwsz_TempRetFile, mn_CP);
		if (asFile)
			*asFile = mpsz_TempRetFile;
		else if (mpsz_TempRetFile && *mpsz_TempRetFile)
			return false; // something went wrong
	}
	else if (changed)
	{
		if (asFile)
			*asFile = nullptr;
	}

	if (changed || mpwsz_TempRetParam)
	{
		mpsz_TempRetParam = wcs2str(mpwsz_TempRetParam, mn_CP);
		if (asParam)
			*asParam = mpsz_TempRetParam;
		else if (mpsz_TempRetParam && *mpsz_TempRetParam)
			return false; // something went wrong
	}

	if (mpwsz_TempRetDir && asDir)
	{
		mpsz_TempRetDir = wcs2str(mpwsz_TempRetDir, mn_CP);
		*asDir = mpsz_TempRetDir;
	}

	return TRUE;
}

// returns FALSE if need to block execution
BOOL CShellProc::OnShellExecuteW(LPCWSTR* asAction, LPCWSTR* asFile, LPCWSTR* asParam, LPCWSTR* asDir, DWORD* anFlags, DWORD* anShowCmd)
{
	if (!IsInterceptionEnabled())
	{
		LogShellString(L"OnShellExecuteW skipped");
		return TRUE; // don't intercept, pass to kernel
	}

	// We do not hook lpIDList
	if (anFlags && (*anFlags & SEE_MASK_IDLIST))
	{
		LogShellString(L"SEE_MASK_IDLIST skipped");
		return TRUE;
	}

	if (GetCurrentThreadId() == gnHookMainThreadId)
	{
		mb_InShellExecuteEx = TRUE;
		gnInShellExecuteEx ++;
	}

	_ASSERTEX(!mpwsz_TempRetFile && !mpwsz_TempRetParam && !mpwsz_TempRetDir);

	const auto prepareResult = PrepareExecuteParams(eShellExecute,
					asAction ? *asAction : nullptr,
					asFile ? *asFile : nullptr,
					asParam ? *asParam : nullptr,
					asDir ? *asDir : nullptr,
					anFlags, nullptr, nullptr, anShowCmd,
					nullptr, nullptr, nullptr, // *StdHandles
					&mpwsz_TempRetFile, &mpwsz_TempRetParam, &mpwsz_TempRetDir);
	if (prepareResult == PrepareExecuteResult::Restrict)
		return false;

	const bool changed = (prepareResult == PrepareExecuteResult::Modified);

	if (changed || mpwsz_TempRetFile)
	{
		if (asFile)
			*asFile = mpwsz_TempRetFile;
		else if (mpwsz_TempRetFile && *mpwsz_TempRetFile)
			return false; // something went wrong
	}

	if (changed || mpwsz_TempRetParam)
	{
		if (asParam)
			*asParam = mpwsz_TempRetParam;
		else if (mpwsz_TempRetParam && *mpwsz_TempRetParam)
			return false; // something went wrong
	}

	if (mpwsz_TempRetDir && asDir)
	{
		*asDir = mpwsz_TempRetDir;
	}

	return TRUE;
}

BOOL CShellProc::FixShellArgs(DWORD afMask, HWND ahWnd, DWORD* pfMask, HWND* phWnd) const
{
	BOOL lbRc = FALSE;

	// Turn on the flag to avoid the warning. "No Zone Check" ConEmu option works in Far Manager only.
	if (!(afMask & SEE_MASK_NOZONECHECKS) && gFarMode.bFarHookMode && gFarMode.bShellNoZoneCheck)
	{
		if (IsWinXP(1))
		{
			(*pfMask) |= SEE_MASK_NOZONECHECKS;
			lbRc = TRUE;
		}
	}

	// Set correct window to let any GUI dialogs appear in front of ConEmu instead of being invisible
	// and inaccessible under ConEmu window.
	if ((!ahWnd || (ahWnd == ghConWnd)) && ghConEmuWnd)
	{
		*phWnd = ghConEmuWnd;
		lbRc = TRUE;
	}

	return lbRc;
}

bool CShellProc::IsInterceptionEnabled()
{
	if (!ghConEmuWndDC || !IsWindow(ghConEmuWndDC))
	{
		if (CDefTermHk::IsDefTermEnabled())
		{
			// OK, continue to "Default terminal" feature (console applications and batch files only)
		}
		else
		{
			return false;
		}
	}

	return true;
}

// returns FALSE if need to block execution
BOOL CShellProc::OnShellExecuteExA(LPSHELLEXECUTEINFOA* lpExecInfo)
{
	if (!lpExecInfo || !IsInterceptionEnabled())
	{
		LogShellString(L"OnShellExecuteExA skipped");
		return TRUE; // don't intercept, pass to kernel
	}

	mlp_SaveExecInfoA = *lpExecInfo;
	mlp_ExecInfoA = static_cast<LPSHELLEXECUTEINFOA>(malloc((*lpExecInfo)->cbSize));
	if (!mlp_ExecInfoA)
	{
		_ASSERTE(mlp_ExecInfoA!=nullptr);
		return TRUE;
	}
	memmove(mlp_ExecInfoA, (*lpExecInfo), (*lpExecInfo)->cbSize);

	FixShellArgs((*lpExecInfo)->fMask, (*lpExecInfo)->hwnd, &(mlp_ExecInfoA->fMask), &(mlp_ExecInfoA->hwnd));

	const BOOL lbRc = OnShellExecuteA(
		&mlp_ExecInfoA->lpVerb, &mlp_ExecInfoA->lpFile, &mlp_ExecInfoA->lpParameters, &mlp_ExecInfoA->lpDirectory,
		&mlp_ExecInfoA->fMask, reinterpret_cast<DWORD*>(&mlp_ExecInfoA->nShow));

	if (lbRc)
		*lpExecInfo = mlp_ExecInfoA;

	return lbRc;
}

// returns FALSE if need to block execution
BOOL CShellProc::OnShellExecuteExW(LPSHELLEXECUTEINFOW* lpExecInfo)
{
	if (!lpExecInfo || !IsInterceptionEnabled())
	{
		LogShellString(L"OnShellExecuteExW skipped");
		return TRUE; // don't intercept, pass to kernel
	}

	mlp_SaveExecInfoW = *lpExecInfo;
	mlp_ExecInfoW = static_cast<LPSHELLEXECUTEINFOW>(malloc((*lpExecInfo)->cbSize));
	if (!mlp_ExecInfoW)
	{
		_ASSERTE(mlp_ExecInfoW!=nullptr);
		return TRUE;
	}
	memmove(mlp_ExecInfoW, (*lpExecInfo), (*lpExecInfo)->cbSize);

	FixShellArgs((*lpExecInfo)->fMask, (*lpExecInfo)->hwnd, &(mlp_ExecInfoW->fMask), &(mlp_ExecInfoW->hwnd));

	const BOOL lbRc = OnShellExecuteW(
		&mlp_ExecInfoW->lpVerb, &mlp_ExecInfoW->lpFile, &mlp_ExecInfoW->lpParameters, &mlp_ExecInfoW->lpDirectory,
		&mlp_ExecInfoW->fMask, reinterpret_cast<DWORD*>(&mlp_ExecInfoW->nShow));

	if (lbRc)
		*lpExecInfo = mlp_ExecInfoW;

	return lbRc;
}

CShellProc::CreatePrepareData CShellProc::OnCreateProcessPrepare(
	const DWORD* anCreationFlags, const DWORD dwFlags, const WORD wShowWindow, const DWORD dwX, const DWORD dwY)
{
	CreatePrepareData result{};

	// For example, mintty starts root using pipe redirection
	result.consoleNoWindow = (dwFlags & STARTF_USESTDHANDLES)
		// or the caller need to run some process as "detached"
		|| ((*anCreationFlags) & (DETACHED_PROCESS|CREATE_NO_WINDOW));

	// Some "heuristics" - when created process may show its window? (Console or GUI)
	result.showCmd = (dwFlags & STARTF_USESHOWWINDOW)
		? wShowWindow
		: ((gbAttachGuiClient || gbGuiClientAttached) && result.consoleNoWindow) ? 0
		: SW_SHOWNORMAL;

	// Console.exe starts cmd.exe with STARTF_USEPOSITION flag
	if ((dwFlags & STARTF_USEPOSITION) && (dwX == HIDDEN_SCREEN_POSITION) && (dwY == HIDDEN_SCREEN_POSITION))
		result.showCmd = SW_HIDE; // Let's think it is starting hidden

	result.defaultShowCmd = result.showCmd;

	_ASSERTE(!(workOptions_ & ShellWorkOptions::WasSuspended));
	if (anCreationFlags && (((*anCreationFlags) & CREATE_SUSPENDED) == CREATE_SUSPENDED))
		SetWasSuspended();

	return result;
}

bool CShellProc::OnCreateProcessResult(const PrepareExecuteResult prepareResult, const CreatePrepareData& state, DWORD* anCreationFlags, WORD& siShowWindow, DWORD& siFlags)
{
	bool siChanged = false;

	if (mb_NeedInjects)
	{
		// In DefTerm (gbPrepareDefaultTerminal) mb_NeedInjects is allowed too in some cases...
		// code.exe -> "cmd /c start /wait" -> cmd.exe
		(*anCreationFlags) |= CREATE_SUSPENDED;
	}

	if (prepareResult == PrepareExecuteResult::Modified)
	{
		WORD newShowCmd = LOWORD(state.showCmd);

		if (gbPrepareDefaultTerminal)
		{
			_ASSERTE(m_Args.NoDefaultTerm != crb_On);

			if (!mb_PostInjectWasRequested)
			{
				if (!mb_DebugWasRequested && !mb_ConsoleMode)
					newShowCmd = SW_SHOWNORMAL; // ConEmu itself must starts normally
			}
		}
		if (state.defaultShowCmd != newShowCmd)
		{
			siShowWindow = newShowCmd;
			siFlags |= STARTF_USESHOWWINDOW;
			siChanged = true;
		}
	}
	// Avoid flickering of RealConsole while starting debugging with DefTerm feature
	else if (CDefTermHk::IsDefTermEnabled())
	{
		if (!state.consoleNoWindow && state.showCmd && anCreationFlags)
		{
			switch (mn_ImageSubsystem)
			{
			case IMAGE_SUBSYSTEM_WINDOWS_CUI:
				if (((*anCreationFlags) & (DEBUG_ONLY_THIS_PROCESS | DEBUG_PROCESS))
					&& ((*anCreationFlags) & CREATE_NEW_CONSOLE))
				{
					// e.g. debugger start of Win32 app from VisualStudio
					// trick to avoid RealConsole window flickering
					// ReSharper disable once CppLocalVariableMayBeConst
					HWND hConWnd = GetRealConsoleWindow();
					if (hConWnd == nullptr)
					{
						_ASSERTE(gnServerPID == 0);
						// Alloc hidden console and attach it to our VS GUI window
						if (CDefTermHk::AllocHiddenConsole(true))
						{
							_ASSERTE(gnServerPID != 0);
							// We managed to create hidden console window, run application there
							SetHiddenConsoleDetachNeed(true);
							// Do not need to "Show" it
							siShowWindow = SW_HIDE;
							siFlags |= STARTF_USESHOWWINDOW;
							siChanged = true;
							// Reuse existing console
							(*anCreationFlags) &= ~CREATE_NEW_CONSOLE;
						}
					}
				}
				break; // IMAGE_SUBSYSTEM_WINDOWS_CUI

			case IMAGE_SUBSYSTEM_WINDOWS_GUI:
				if (!((*anCreationFlags) & (DEBUG_ONLY_THIS_PROCESS | DEBUG_PROCESS))
					&& !((*anCreationFlags) & CREATE_NEW_CONSOLE))
				{
					// Start of msvsmon.exe?
					const auto* pszExeName = PointToName(ms_ExeTmp);
					if (pszExeName && (lstrcmpi(pszExeName, L"msvsmon.exe") == 0))
					{
						// Would be nice to hook only those instances which are started to work with *local* processes
						// e.g.: msvsmon.exe ... /hostname [::1] /port ... /__pseudoremote
						SetPostInjectWasRequested(true);
						if (!(workOptions_ & ShellWorkOptions::WasSuspended))
							(*anCreationFlags) |= CREATE_SUSPENDED;
					}
				}
				break; // IMAGE_SUBSYSTEM_WINDOWS_GUI
			}
		}
		else if (state.defaultShowCmd != state.showCmd)
		{
			siShowWindow = LOWORD(state.showCmd);
			siFlags |= STARTF_USESHOWWINDOW;
			siChanged = true;
		}
	}

	return siChanged;
}

// returns FALSE if need to block execution
BOOL CShellProc::OnCreateProcessA(LPCSTR* asFile, LPCSTR* asCmdLine, LPCSTR* asDir, DWORD* anCreationFlags, LPSTARTUPINFOA* ppStartupInfo)
{
	if (!ppStartupInfo || !*ppStartupInfo || !IsInterceptionEnabled())
	{
		LogShellString(L"OnCreateProcessA skipped");
		return TRUE; // don't intercept, pass to kernel
	}

	const size_t cbLocalStartupInfoSize = std::max<size_t>(sizeof(STARTUPINFOA), (*ppStartupInfo)->cb);
	m_lpStartupInfoA.reset(static_cast<LPSTARTUPINFOA>(calloc(1, cbLocalStartupInfoSize)));
	if (!m_lpStartupInfoA)
	{
		LogShellString(L"OnCreateProcessW failed");
		return TRUE; // don't intercept, pass to kernel
	}
	auto* lpSi = m_lpStartupInfoA.get();
	if ((*ppStartupInfo)->cb)
		memmove_s(lpSi, cbLocalStartupInfoSize, *ppStartupInfo, (*ppStartupInfo)->cb);
	else
		lpSi->cb = sizeof(*lpSi);

	mpwsz_TempFile = str2wcs(asFile ? *asFile : nullptr, mn_CP);
	mpwsz_TempParam = str2wcs(asCmdLine ? *asCmdLine : nullptr, mn_CP);
	const CEStr lsDir(str2wcs(asDir ? *asDir : nullptr, mn_CP));

	_ASSERTEX(!mpwsz_TempRetFile && !mpwsz_TempRetParam && !mpwsz_TempRetDir);

	// Preprocess flags and options
	auto state = OnCreateProcessPrepare(anCreationFlags, lpSi->dwFlags, lpSi->wShowWindow, lpSi->dwX, lpSi->dwY);

	const auto prepareResult = PrepareExecuteParams(eCreateProcess,
		nullptr, mpwsz_TempFile, mpwsz_TempParam, lsDir,
		nullptr, anCreationFlags, &lpSi->dwFlags, &state.showCmd,
		&lpSi->hStdInput, &lpSi->hStdOutput, &lpSi->hStdError,
		&mpwsz_TempRetFile, &mpwsz_TempRetParam, &mpwsz_TempRetDir);
	if (prepareResult == PrepareExecuteResult::Restrict)
		return false;

	const bool changed = (prepareResult == PrepareExecuteResult::Modified);

	// patch flags and variables based on decision
	if (OnCreateProcessResult(prepareResult, state, anCreationFlags, lpSi->wShowWindow, lpSi->dwFlags))
		*ppStartupInfo = lpSi;

	// Patch modified strings (wide to ansi/oem)

	if (mpwsz_TempRetFile && *mpwsz_TempRetFile)
	{
		mpsz_TempRetFile = wcs2str(mpwsz_TempRetFile, mn_CP);
		if (asFile)
			*asFile = mpsz_TempRetFile;
		else if (mpsz_TempRetFile && *mpsz_TempRetFile)
			return false; // something went wrong
	}
	else if (changed)
	{
		if (asFile)
			*asFile = nullptr;
	}

	if (changed || mpwsz_TempRetParam)
	{
		if (mpwsz_TempRetParam)
		{
			mpsz_TempRetParam = wcs2str(mpwsz_TempRetParam, mn_CP);
			if (asCmdLine)
				*asCmdLine = mpsz_TempRetParam;
			else if (mpsz_TempRetParam && *mpsz_TempRetParam)
				return false; // something went wrong
		}
		else
		{
			if (asCmdLine)
				*asCmdLine = nullptr;
		}
	}

	if (mpwsz_TempRetDir && asDir)
	{
		mpsz_TempRetDir = wcs2str(mpwsz_TempRetDir, mn_CP);
		*asDir = mpsz_TempRetDir;
	}

	return TRUE;
}

// returns FALSE if need to block execution
BOOL CShellProc::OnCreateProcessW(LPCWSTR* asFile, LPCWSTR* asCmdLine, LPCWSTR* asDir, DWORD* anCreationFlags, LPSTARTUPINFOW* ppStartupInfo)
{
	if (!ppStartupInfo || !*ppStartupInfo || !IsInterceptionEnabled())
	{
		LogShellString(L"OnCreateProcessW skipped");
		return TRUE; // don't intercept, pass to kernel
	}

	const size_t cbLocalStartupInfoSize = std::max<size_t>(sizeof(STARTUPINFOW), (*ppStartupInfo)->cb);
	m_lpStartupInfoW.reset(static_cast<LPSTARTUPINFOW>(calloc(1, cbLocalStartupInfoSize)));
	if (!m_lpStartupInfoW)
	{
		LogShellString(L"OnCreateProcessW failed");
		return TRUE; // don't intercept, pass to kernel
	}

	auto* lpSi = m_lpStartupInfoW.get();
	if ((*ppStartupInfo)->cb)
		memmove_s(lpSi, cbLocalStartupInfoSize, *ppStartupInfo, (*ppStartupInfo)->cb);
	else
		lpSi->cb = sizeof(*lpSi); // VS 2019 passes cb==0 while starting console applications (run & debug)

	// Preprocess flags and options
	auto state = OnCreateProcessPrepare(anCreationFlags, lpSi->dwFlags, lpSi->wShowWindow, lpSi->dwX, lpSi->dwY);

	_ASSERTEX(!mpwsz_TempRetFile && !mpwsz_TempRetParam && !mpwsz_TempRetDir);

	// Main logic
	const auto prepareResult = PrepareExecuteParams(eCreateProcess,
					nullptr,
					asFile ? *asFile : nullptr,
					asCmdLine ? *asCmdLine : nullptr,
					asDir ? *asDir : nullptr,
					nullptr, anCreationFlags, &lpSi->dwFlags, &state.showCmd,
					&lpSi->hStdInput, &lpSi->hStdOutput, &lpSi->hStdError,
					&mpwsz_TempRetFile, &mpwsz_TempRetParam, &mpwsz_TempRetDir);
	if (prepareResult == PrepareExecuteResult::Restrict)
		return false;

	const bool changed = (prepareResult == PrepareExecuteResult::Modified);

	// patch flags and variables based on decision
	if (OnCreateProcessResult(prepareResult, state, anCreationFlags, lpSi->wShowWindow, lpSi->dwFlags))
		*ppStartupInfo = lpSi;


	// Patch modified strings (wide to ansi/oem)

	if (changed || mpwsz_TempRetFile)
	{
		if (asFile)
			*asFile = mpwsz_TempRetFile;
		else if (mpwsz_TempRetFile && *mpwsz_TempRetFile)
			return false; // something went wrong
	}

	if (changed || mpwsz_TempRetParam)
	{
		if (asCmdLine)
			*asCmdLine = mpwsz_TempRetParam;
		else if (mpwsz_TempRetParam && *mpwsz_TempRetParam)
			return false; // something went wrong
	}

	if (mpwsz_TempRetDir && asDir)
	{
		*asDir = mpwsz_TempRetDir;
	}

	return TRUE;
}

void CShellProc::OnCreateProcessFinished(BOOL abSucceeded, PROCESS_INFORMATION *lpPI)
{
	// logging
	{
		const size_t cchLen = 255;
		wchar_t szDbgMsg[cchLen];
		if (!abSucceeded)
		{
			msprintf(szDbgMsg, cchLen,
				L"Create(ParentPID=%u): Failed, ErrCode=0x%08X",
				GetCurrentProcessId(), GetLastError());
		}
		else
		{
			msprintf(szDbgMsg, cchLen,
				L"Create(ParentPID=%u): Ok, PID=%u",
				GetCurrentProcessId(), lpPI->dwProcessId);
			if (WaitForSingleObject(lpPI->hProcess, 0) == WAIT_OBJECT_0)
			{
				DWORD dwExitCode = 0;
				GetExitCodeProcess(lpPI->hProcess, &dwExitCode);
				int len = lstrlen(szDbgMsg);
				msprintf(szDbgMsg+len, cchLen-len,
					L", Terminated!!! Code=%u", dwExitCode);
			}
		}
		LogShellString(szDbgMsg);
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
				CESERVER_REQ* pOut = ExecuteGuiCmd(ghConWnd, pIn, nullptr);
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
				_ASSERTE(wcschr(ms_ExeTmp,L'\\')!=nullptr);
				lstrcpyn(pIn->PortableStarted.sAppFilePathName, ms_ExeTmp, countof(pIn->PortableStarted.sAppFilePathName));
				pIn->PortableStarted.nPID = lpPI->dwProcessId;
				HANDLE hServer = OpenProcess(PROCESS_DUP_HANDLE, FALSE, gnServerPID);
				if (hServer)
				{
					HANDLE hDup = nullptr;
					DuplicateHandle(GetCurrentProcess(), lpPI->hProcess, hServer, &hDup, 0, FALSE, DUPLICATE_SAME_ACCESS);
					pIn->PortableStarted.hProcess = hDup;
					CloseHandle(hServer);
				}

				CESERVER_REQ* pOut = ExecuteSrvCmd(gnServerPID, pIn, nullptr);
				ExecuteFreeResult(pOut);
				ExecuteFreeResult(pIn);
			}
		}

		if (m_Args.nPTY)
		{
			CEAnsi::ChangeTermMode(tmc_TerminalType, (m_Args.nPTY & pty_XTerm) ? 1 : 0, lpPI->dwProcessId);
		}

		if (CDefTermHk::IsDefTermEnabled())
		{
			// #DefTerm: add special flag to auto hooked apps (msvsmon, VsConsoleDebugger, etc)
			if (gpDefTerm->IsAppNameMonitored(ms_ExeTmp))
			{
				// #DefTerm: conemuInsidePid should be in m_SrvMapping already
				gpDefTerm->CreateChildMapping(lpPI->dwProcessId, lpPI->hProcess, 0/*conemuInsidePid*/);
			}

			// Starting .Net debugging session from VS or CodeBlocks console app (gdb)
			if (mb_PostInjectWasRequested)
			{
				// This is "*.vshost.exe", it is GUI which can be used for debugging .Net console applications
				// Also in CodeBlocks' gdb debugger
				if ((workOptions_ & ShellWorkOptions::WasSuspended))
				{
					// Supposing Studio will process only one "*.vshost.exe" at a time
					m_WaitDebugVsThread = *lpPI;
				}
				else
				{
					OnResumeDebuggeeThreadCalled(lpPI->hThread, lpPI);
				}
			}
			// Starting debugging session from VS (for example)?
			else if (mb_DebugWasRequested && !mb_HiddenConsoleDetachNeed)
			{
				if (mb_ForceInjectOriginal)
				{
					// If user choose to inject ConEmuHk into debugging process (to have ANSI support, etc.)
					RunInjectHooks(WIN3264TEST(L"DefTerm",L"DefTerm64"), lpPI);
				}

				// We need to start console app directly, but it will be nice
				// to attach it to the existing or new ConEmu window.
				size_t cchMax = MAX_PATH + 80;
				CEStr szSrvArgs, szNewCon;
				cchMax += CDefTermHk::GetSrvAddArgs(false, false, szSrvArgs, szNewCon);
				_ASSERTE(szNewCon.IsEmpty());

				CEStr cmdLine;
				if (cmdLine.GetBuffer(cchMax))
				{
					_ASSERTEX(m_SrvMapping.ComSpec.ConEmuBaseDir[0]!=0);
					msprintf(cmdLine.data(), cchMax, L"\"%s\\%s\" /ATTACH /CONPID=%u%s",
						m_SrvMapping.ComSpec.ConEmuBaseDir, ConEmuC_EXE_3264,
						lpPI->dwProcessId,
						static_cast<LPCWSTR>(szSrvArgs));
					STARTUPINFO si = {};
					si.cb = sizeof(si);
					PROCESS_INFORMATION pi = {};
					bAttachCreated = CreateProcess(nullptr, cmdLine.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, m_SrvMapping.ComSpec.ConEmuBaseDir, &si, &pi);
					if (bAttachCreated)
					{
						SafeCloseHandle(pi.hProcess);
						SafeCloseHandle(pi.hThread);
					}
					else
					{
						const DWORD nErr = GetLastError();
						CEStr dbgMsg;
						if (dbgMsg.GetBuffer(MAX_PATH * 3))
						{
							msprintf(dbgMsg.data(), dbgMsg.GetMaxCount(), L"ConEmuHk: Failed to start attach server, Err=%u! %s", nErr, cmdLine.c_str());
							LogShellString(dbgMsg);
						}
					}
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

void CShellProc::RunInjectHooks(LPCWSTR asFrom, PROCESS_INFORMATION *lpPI) const
{
	wchar_t szDbgMsg[255];
	msprintf(szDbgMsg, countof(szDbgMsg), L"%s: InjectHooks(x%u), ParentPID=%u (%s), ChildPID=%u",
		asFrom, WIN3264TEST(32,64), GetCurrentProcessId(), gsExeName, lpPI->dwProcessId);
	LogShellString(szDbgMsg);

	LPCWSTR pszDllDir = nullptr;
	if (CDefTermHk::IsDefTermEnabled() && gpDefTerm)
		pszDllDir = gpDefTerm->GetOpt().pszConEmuBaseDir;
	else
		pszDllDir = gsConEmuBaseDir;

	const CINJECTHK_EXIT_CODES iHookRc = InjectHooks(*lpPI, mn_ImageBits,
		(m_SrvMapping.cbSize && (m_SrvMapping.nLoggingType == glt_Processes)),
		pszDllDir, ghConWnd);

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
		GuiMessageBox(nullptr, szDbgMsg, szTitle, MB_SYSTEMMODAL);
	}

	// Release process, it called was not set CREATE_SUSPENDED flag
	if (!(workOptions_ & ShellWorkOptions::WasSuspended))
	{
		ResumeThread(lpPI->hThread);
	}
}

void CShellProc::SetWasSuspended()
{
	workOptions_ |= ShellWorkOptions::WasSuspended;
}

void CShellProc::SetWasDebug()
{
	workOptions_ |= ShellWorkOptions::WasDebug;
}

void CShellProc::SetGnuDebugger()
{
	workOptions_ |= ShellWorkOptions::GnuDebugger;
}

void CShellProc::SetVsNetHost()
{
	workOptions_ |= ShellWorkOptions::VsNetHost;
}

void CShellProc::SetVsDebugConsole()
{
	workOptions_ |= ShellWorkOptions::VsDebugConsole;
}

void CShellProc::SetChildGui()
{
	workOptions_ |= ShellWorkOptions::ChildGui;
}

void CShellProc::SetNeedInjects(const bool value)
{
	mb_NeedInjects = value;
}

void CShellProc::SetForceInjectOriginal(const bool value)
{
	mb_ForceInjectOriginal = value;
}

void CShellProc::SetConsoleMode(const bool value)
{
	mb_ConsoleMode = value;
}

void CShellProc::SetDebugWasRequested(const bool value)
{
	mb_DebugWasRequested = value;
}

void CShellProc::SetHiddenConsoleDetachNeed(const bool value)
{
	mb_HiddenConsoleDetachNeed = value;
}

void CShellProc::SetPostInjectWasRequested(const bool value)
{
	mb_PostInjectWasRequested = value;
}

bool CShellProc::OnResumeDebuggeeThreadCalled(HANDLE hThread, PROCESS_INFORMATION* lpPI /*= nullptr*/)
{
	if ((!hThread || (m_WaitDebugVsThread.hThread != hThread)) && !lpPI)
		return false;

	int iHookRc = -1;
	CLastErrorGuard errGuard;
	DWORD nResumeRC = -1;

	DWORD nPID = lpPI ? lpPI->dwProcessId : m_WaitDebugVsThread.dwProcessId;
	HANDLE hProcess = lpPI ? lpPI->hProcess : m_WaitDebugVsThread.hProcess;
	_ASSERTEX(hThread != nullptr);
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
		SuspendThread(hThread);  // -V720

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

void CShellProc::LogShellString(LPCWSTR asMessage) const
{
	CDefTermHk::DefTermLogString(asMessage);

	#ifdef PRINT_SHELL_LOG
	wprintf(L"%s\n", asMessage);
	#endif
}
