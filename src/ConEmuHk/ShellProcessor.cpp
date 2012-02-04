
/*
Copyright (c) 2011 Maximus5
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

#include <windows.h>
#include <TCHAR.h>
#include <Tlhelp32.h>
#include <shlwapi.h>
#include "../common/common.hpp"
#include "../common/ConEmuCheck.h"
#include "SetHook.h"
#include "../common/execute.h"
#include "ConEmuHooks.h"
#include "ShellProcessor.h"
#include "Injects.h"
#include "GuiAttach.h"

#ifndef SEE_MASK_NOZONECHECKS
#define SEE_MASK_NOZONECHECKS 0x800000
#endif
#ifndef SEE_MASK_NOASYNC
#define SEE_MASK_NOASYNC 0x00000100
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

CShellProc::CShellProc()
{
	mn_CP = AreFileApisANSI() ? CP_ACP : CP_OEMCP;
	mpwsz_TempAction = mpwsz_TempFile = mpwsz_TempParam = NULL;
	mpsz_TempRetFile = mpsz_TempRetParam = NULL;
	mpwsz_TempRetFile = mpwsz_TempRetParam = NULL;
	mlp_ExecInfoA = NULL; mlp_ExecInfoW = NULL;
	mlp_SaveExecInfoA = NULL; mlp_SaveExecInfoW = NULL;
	mb_WasSuspended = FALSE;
	mb_NeedInjects = FALSE;
	// int CShellProc::mn_InShellExecuteEx = 0; <-- static
	mb_InShellExecuteEx = FALSE;
	//mb_DosBoxAllowed = FALSE;
	m_SrvMapping.cbSize = 0;
}

CShellProc::~CShellProc()
{
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
	if (mpwsz_TempRetFile)
		free(mpwsz_TempRetFile);
	if (mpwsz_TempRetParam)
		free(mpwsz_TempRetParam);
	// structures
	if (mlp_ExecInfoA)
		free(mlp_ExecInfoA);
	if (mlp_ExecInfoW)
		free(mlp_ExecInfoW);
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

BOOL CShellProc::LoadGuiMapping()
{
	if (!m_SrvMapping.cbSize || (m_SrvMapping.hConEmuWnd && !IsWindow(m_SrvMapping.hConEmuWnd)))
	{
		if (!::LoadSrvMapping(ghConWnd, m_SrvMapping))
			return FALSE;
	}

	if (!m_SrvMapping.hConEmuWnd || !IsWindow(m_SrvMapping.hConEmuWnd))
		return FALSE;

	return TRUE;

	//// Проверим, а можно ли?
	//DWORD dwGuiProcessId = 0;
	//if (!ghConEmuWnd || !GetWindowThreadProcessId(ghConEmuWnd, &dwGuiProcessId))
	//	return FALSE;

	//MFileMapping<ConEmuGuiMapping> GuiInfoMapping;
	//GuiInfoMapping.InitName(CEGUIINFOMAPNAME, dwGuiProcessId);
	//const ConEmuGuiMapping* pInfo = GuiInfoMapping.Open();
	//if (!pInfo)
	//	return FALSE;
	//else if (pInfo->nProtocolVersion != CESERVER_REQ_VER)
	//	return FALSE;
	//else
	//{
	//	memmove(&m_GuiMapping, pInfo, min(pInfo->cbSize, sizeof(m_GuiMapping)));
	//	/*bDosBoxAllowed = pInfo->bDosBox;
	//	wcscpy_c(szBaseDir, pInfo->sConEmuBaseDir);
	//	wcscat_c(szBaseDir, L"\\");
	//	if (pInfo->nLoggingType != glt_Processes)
	//		return NULL;*/
	//}
	//GuiInfoMapping.CloseMap();

	//return (m_GuiMapping.cbSize != 0);
}

CESERVER_REQ* CShellProc::NewCmdOnCreate(enum CmdOnCreateType aCmd,
				LPCWSTR asAction, LPCWSTR asFile, LPCWSTR asParam,
				DWORD* anShellFlags, DWORD* anCreateFlags, DWORD* anStartFlags, DWORD* anShowCmd,
				int mn_ImageBits, int mn_ImageSubsystem,
				HANDLE hStdIn, HANDLE hStdOut, HANDLE hStdErr
				/*wchar_t (&szBaseDir)[MAX_PATH+2], BOOL& bDosBoxAllowed*/)
{
	//szBaseDir[0] = 0;

	// Проверим, а надо ли?
	if (!LoadGuiMapping())
		return NULL;

	//	bDosBoxAllowed = pInfo->bDosBox;
	//	wcscpy_c(szBaseDir, pInfo->sConEmuBaseDir);
	//	wcscat_c(szBaseDir, L"\\");
	if (m_SrvMapping.nLoggingType != glt_Processes)
		return NULL;

	return ExecuteNewCmdOnCreate(aCmd,
				asAction, asFile, asParam,
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

BOOL CShellProc::ChangeExecuteParms(enum CmdOnCreateType aCmd, BOOL abNewConsole,
				LPCWSTR asFile, LPCWSTR asParam, /*LPCWSTR asBaseDir,*/
				LPCWSTR asExeFile, DWORD& ImageBits, DWORD& ImageSubsystem,
				LPWSTR* psFile, LPWSTR* psParam)
{
	if (!LoadGuiMapping())
		return FALSE;

	BOOL lbRc = FALSE;
	size_t cchComspec = MAX_PATH+20;
	wchar_t *szComspec = NULL;
	size_t cchConEmuC = MAX_PATH+16;
	wchar_t *szConEmuC = NULL; // ConEmuC64.exe
	BOOL lbUseDosBox = FALSE;
	size_t cchDosBoxExe = MAX_PATH+16, cchDosBoxCfg = MAX_PATH+16;
	wchar_t *szDosBoxExe = NULL, *szDosBoxCfg = NULL;
	BOOL lbComSpec = FALSE; // TRUE - если %COMSPEC% отбрасывается
	int nCchSize = 0;
	BOOL lbEndQuote = FALSE;
	bool lbNewGuiConsole = false, lbNewConsoleFromGui = false;
	BOOL lbComSpecK = FALSE; // TRUE - если нужно запустить /K, а не /C

	szConEmuC = (wchar_t*)malloc(cchConEmuC*sizeof(*szConEmuC)); // ConEmuC64.exe
	if (!szConEmuC)
	{
		_ASSERTE(szConEmuC!=NULL);
		goto wrap;
	}
	_wcscpy_c(szConEmuC, cchConEmuC, m_SrvMapping.sConEmuBaseDir);
	_wcscat_c(szConEmuC, cchConEmuC, L"\\");
	
	_ASSERTE(aCmd==eShellExecute || aCmd==eCreateProcess);


	if (aCmd == eCreateProcess)
	{
		// Для простоты - сразу откинем asFile если он совпадает с первым аргументом в asParam
		if (asFile && !*asFile) asFile = NULL;
		if (asFile && *asFile && asParam && *asParam)
		{
			LPCWSTR pszParam = SkipNonPrintable(asParam);
			if (pszParam && ((*pszParam != L'"') || (pszParam[0] == L'"' && pszParam[1] != L'"')))
			{
				BOOL lbSkipEndQuote = FALSE;
				if (*pszParam == L'"')
				{
					pszParam++;
					lbSkipEndQuote = TRUE;
				}
				int nLen = lstrlen(asFile)+1;
				int cchMax = max(nLen,(MAX_PATH+1));
				wchar_t* pszTest = (wchar_t*)malloc(cchMax*sizeof(wchar_t));
				_ASSERTE(pszTest);
				if (pszTest)
				{
					// Сначала пробуем на полное соответствие
					if (asFile)
					{
						_wcscpyn_c(pszTest, nLen, pszParam, nLen); //-V501
						pszTest[nLen-1] = 0;
						// Сравнить asFile с первым аргументом в asParam
						if (lstrcmpi(pszTest, asFile) == 0)
						{
							// exe-шник уже указан в asParam, добавлять дополнительно НЕ нужно
							asFile = NULL;
						}
					}
					// Если не сошлось - в asParam может быть только имя запускаемого файла
					// например CreateProcess(L"C:\\GCC\\mingw32-make.exe", L"mingw32-make.exe makefile",...)
					// или      CreateProcess(L"C:\\GCC\\mingw32-make.exe", L"mingw32-make makefile",...)
					if (asFile)
					{
						LPCWSTR pszFileOnly = PointToName(asFile);
						if (pszFileOnly && (pszFileOnly != asFile))
						{
							// Пробуем с откидыванием пути
							nLen = lstrlen(pszFileOnly)+1;
							_wcscpyn_c(pszTest, nLen, pszParam, nLen); //-V501
							pszTest[nLen-1] = 0;
							// Сравнить asFile с первым аргументом в asParam
							if (lstrcmpi(pszTest, pszFileOnly) == 0)
							{
								// exe-шник уже указан в asParam, добавлять дополнительно НЕ нужно
								// -- asFile = NULL; -- трогать нельзя, только он содержит полный путь!
								asParam = pszParam+nLen-(lbSkipEndQuote?0:1);
								pszFileOnly = NULL;
							}
						}
						if (pszFileOnly)
						{
							// а теперь, с откидыванием расширения
							wchar_t szTmpFileOnly[MAX_PATH+1]; szTmpFileOnly[0] = 0;
							_wcscpyn_c(szTmpFileOnly, countof(szTmpFileOnly), pszFileOnly, countof(szTmpFileOnly)); //-V501
							wchar_t* pszExt = wcsrchr(szTmpFileOnly, L'.');
							if (pszExt)
							{
								*pszExt = 0;
								nLen = lstrlen(szTmpFileOnly)+1;
								_wcscpyn_c(pszTest, nLen, pszParam, nLen); //-V501
								pszTest[nLen-1] = 0;
								// Сравнить asFile с первым аргументом в asParam
								if (lstrcmpi(pszTest, szTmpFileOnly) == 0)
								{
									// exe-шник уже указан в asParam, добавлять дополнительно НЕ нужно
									// -- asFile = NULL; -- трогать нельзя, только он содержит полный путь!
									asParam = pszParam+nLen-(lbSkipEndQuote?0:1);
									pszFileOnly =  NULL;
								}
							}
						}
					}

					free(pszTest);
				}
			}
		}
	}

	szComspec = (wchar_t*)calloc(cchComspec,sizeof(*szComspec));
	if (!szComspec)
	{
		_ASSERTE(szComspec);
		goto wrap;
	}
	if (GetEnvironmentVariable(L"ComSpec", szComspec, (DWORD)cchComspec))
	{
		// Не должен быть (даже случайно) ConEmuC.exe
		const wchar_t* pszName = PointToName(szComspec);
		if (!pszName || !lstrcmpi(pszName, L"ConEmuC.exe") || !lstrcmpi(pszName, L"ConEmuC64.exe"))
			szComspec[0] = 0;
	}
	// Если не удалось определить через переменную окружения - пробуем обычный "cmd.exe" из System32
	if (szComspec[0] == 0)
	{
		int n = GetWindowsDirectory(szComspec, MAX_PATH);
		if (n > 0 && n < MAX_PATH)
		{
			// Добавить \System32\cmd.exe
			wcscat_c(szComspec, (szComspec[n-1] == L'\\') ? L"System32\\cmd.exe" : L"\\System32\\cmd.exe");
		}
	}

	// Если удалось определить "ComSpec"
	if (asParam)
	{
		BOOL lbNewCmdCheck = NULL;
		const wchar_t* psz = SkipNonPrintable(asParam);
		// Если запускают cmd.exe без параметров - не отбрасывать его!
		if (psz && *psz && szComspec[0])
		{
			// asFile может быть и для ShellExecute и для CreateProcess, проверим в одном месте
			if (asFile && (lstrcmpi(szComspec, asFile) == 0))
			{
				if (psz[0] == L'/' && wcschr(L"CcKk", psz[1]))
				{
					if (abNewConsole)
					{
						// 111211 - "-new_console" передается в GUI
						_ASSERTEX(psz[1] == L'C' || psz[1] == L'c');
						lbComSpecK = FALSE;
					}
					else
					{
						// не добавлять в измененную команду asFile (это отбрасываемый cmd.exe)
						lbComSpecK = (psz[1] == L'K' || psz[1] == L'k');
						asFile = NULL;
						asParam = SkipNonPrintable(psz+2); // /C или /K добавляется к ConEmuC.exe
						lbNewCmdCheck = TRUE;
					}

					//BOOL lbRootIsCmdExe = FALSE, lbAlwaysConfirmExit = FALSE, lbAutoDisableConfirmExit = FALSE;
					//BOOL lbNeedCutStartEndQuot = FALSE;
					//DWORD nFileAttrs = (DWORD)-1;
					//ms_ExeTmp[0] = 0;
					//IsNeedCmd(asParam, &lbNeedCutStartEndQuot, ms_ExeTmp, lbRootIsCmdExe, lbAlwaysConfirmExit, lbAutoDisableConfirmExit);
					//// это может быть команда ком.процессора!
					//// поэтому, наверное, искать и проверять битность будем только для
					//// файлов с указанным расширением.
					//// cmd.exe /c echo -> НЕ искать
					//// cmd.exe /c echo.exe -> можно и поискать
					//if (ms_ExeTmp[0] && (wcschr(ms_ExeTmp, L'.') || wcschr(ms_ExeTmp, L'\\')))
					//{
					//	DWORD nCheckSybsystem = 0, nCheckBits = 0;
					//	if (FindImageSubsystem(ms_ExeTmp, nCheckSybsystem, nCheckBits, nFileAttrs))
					//	{
					//		ImageSubsystem = nCheckSybsystem;
					//		ImageBits = nCheckBits;
					//	}
					//}

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
			DWORD nFileAttrs = (DWORD)-1;
			ms_ExeTmp[0] = 0;
			IsNeedCmd(asParam, &lbNeedCutStartEndQuot, ms_ExeTmp, lbRootIsCmdExe, lbAlwaysConfirmExit, lbAutoDisableConfirmExit);
			// это может быть команда ком.процессора!
			// поэтому, наверное, искать и проверять битность будем только для
			// файлов с указанным расширением.
			// cmd.exe /c echo -> НЕ искать
			// cmd.exe /c echo.exe -> можно и поискать
			if (ms_ExeTmp[0] && (wcschr(ms_ExeTmp, L'.') || wcschr(ms_ExeTmp, L'\\')))
			{
				DWORD nCheckSybsystem = 0, nCheckBits = 0;
				if (FindImageSubsystem(ms_ExeTmp, nCheckSybsystem, nCheckBits, nFileAttrs))
				{
					ImageSubsystem = nCheckSybsystem;
					ImageBits = nCheckBits;
				}
			}
		}
	}

	//wchar_t szNewExe[MAX_PATH+1];
	//const wchar_t* pszExeExt = NULL;
	//// Проверить, может запускается батник?
	//if (/*lbComSpec &&*/ !asFile && asParam)
	//{
	//	const wchar_t* pszTemp = asParam;
	//	if (0 == NextArg(&pszTemp, szNewExe))
	//	{
	//		pszExeExt = PointToExt(szNewExe);
	//	}
	//}
	// Пока не будем ничего менять, пусть работает "как в фаре"
	//if (pszExeExt && (!lstrcmpi(pszExeExt, L".cmd") || !lstrcmpi(pszExeExt, L".bat")))
	//{
	//	ImageSubsystem = IMAGE_SUBSYSTEM_BATCH_FILE;
	//	// Будем выполнять "батники" в нативной битности
	//	#ifdef _WIN64
	//	ImageBits = 64;
	//	#else
	//	ImageBits = 32;
	//	#endif
	//}
	
//#ifdef _DEBUG
//	// Если запускается батник - попробовать "нативную" битность?
//	if (ImageSubsystem == IMAGE_SUBSYSTEM_BATCH_FILE)
//	{
//		#ifdef _WIN64
//		ImageBits = 64;
//		#else
//		ImageBits = 32;
//		#endif
//	}
//#endif



	lbUseDosBox = FALSE;
	szDosBoxExe = (wchar_t*)calloc(cchDosBoxExe, sizeof(*szDosBoxExe));
	szDosBoxCfg = (wchar_t*)calloc(cchDosBoxCfg, sizeof(*szDosBoxCfg));
	if (!szDosBoxExe || !szDosBoxCfg)
	{
		_ASSERTE(szDosBoxExe && szDosBoxCfg);
		goto wrap;
	}

	if (ImageBits == 32) //-V112
	{
		wcscat_c(szConEmuC, L"ConEmuC.exe");
	}
	else if (ImageBits == 64)
	{
		wcscat_c(szConEmuC, L"ConEmuC64.exe");
	}
	else if (ImageBits == 16)
	{
		if (m_SrvMapping.cbSize && m_SrvMapping.bDosBox)
		{
			wcscpy_c(szDosBoxExe, m_SrvMapping.sConEmuBaseDir);
			wcscat_c(szDosBoxExe, L"\\DosBox\\DosBox.exe");
			wcscpy_c(szDosBoxCfg, m_SrvMapping.sConEmuBaseDir);
			wcscat_c(szDosBoxCfg, L"\\DosBox\\DosBox.conf");

			if (!FileExists(szDosBoxExe) || !FileExists(szDosBoxCfg))
			{
				// DoxBox не установлен!
				lbRc = FALSE;
				//return FALSE;
				goto wrap;
			}

			wcscat_c(szConEmuC, L"ConEmuC.exe");
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
				wcscat_c(szConEmuC, L"ConEmuC.exe");
			}
		}
	}
	else
	{
		// Если не смогли определить что это и как запускается - лучше не трогать
		_ASSERTE(ImageBits==16||ImageBits==32||ImageBits==64);
		//wcscat_c(szConEmuC, L"ConEmuC.exe");
		lbRc = FALSE;
		//return FALSE;
		goto wrap;
	}
	
	nCchSize = (asFile ? lstrlen(asFile) : 0) + (asParam ? lstrlen(asParam) : 0) + 64;
	if (lbUseDosBox)
	{
		// Может быть нужно экранирование кавычек или еще чего, зарезервируем буфер
		// ну и сами параметры для DosBox
		nCchSize = (nCchSize*2) + lstrlen(szDosBoxExe) + lstrlen(szDosBoxCfg) + 128 + MAX_PATH*2/*на cd и прочую фигню*/;
	}

	if (!FileExists(szConEmuC))
	{
		wchar_t szInfo[MAX_PATH+128], szTitle[64];
		wcscpy_c(szInfo, L"ConEmu executable not found!\n");
		wcscat_c(szInfo, szConEmuC);
		msprintf(szTitle, countof(szTitle), L"ConEmuHk, PID=%i", GetCurrentProcessId());
		GuiMessageBox(ghConEmuWnd, szInfo, szTitle, MB_ICONSTOP);
	}

	if (aCmd == eShellExecute)
	{
		*psFile = lstrdup(szConEmuC);
	}
	else
	{
		nCchSize += lstrlen(szConEmuC)+1;
		*psFile = NULL;
	}

	// Если запускается новый GUI как вкладка, или консольное приложения из GUI как вкладки
	lbNewGuiConsole = (ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI) || (ghAttachGuiClient != NULL);
	lbNewConsoleFromGui = (ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI) && (ghAttachGuiClient != NULL);

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
	
	// В ShellExecute необходимо "ConEmuC.exe" вернуть в psFile, а для CreatePocess - в psParam
	// /C или /K в обоих случаях нужно пихать в psParam
	lbEndQuote = FALSE;
	*psParam = (wchar_t*)malloc(nCchSize*sizeof(wchar_t));
	(*psParam)[0] = 0;
	if (aCmd == eCreateProcess)
	{
		(*psParam)[0] = L'"';
		_wcscpy_c((*psParam)+1, nCchSize-1, szConEmuC);
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
	}

	if (lbUseDosBox)
		_wcscat_c((*psParam), nCchSize, L" /DOSBOX");

	// 111211 - "-new_console" передается в GUI
	#if 0
	//if (ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI)
	if (lbNewGuiConsole)
	{
		// Нужно еще добавить /ATTACH /GID=%i,  и т.п.
		int nCurLen = lstrlen(*psParam);
		msprintf((*psParam) + nCurLen, nCchSize - nCurLen, L" /ATTACH /GID=%u /ROOT ", m_SrvMapping.nGuiPID);
		TODO("Наверное, хорошо бы обработать /K|/C? Если консольное запускается из GUI");
	}
	else
	#endif
	if (lbNewConsoleFromGui)
	{
		// Нужно еще добавить /ATTACH /GID=%i,  и т.п.
		int nCurLen = lstrlen(*psParam);
		msprintf((*psParam) + nCurLen, nCchSize - nCurLen, L" /ATTACH /GID=%u /ROOT ", m_SrvMapping.nGuiPID);
		TODO("Наверное, хорошо бы обработать /K|/C? Если консольное запускается из GUI");
	}
	else
	{
		_wcscat_c((*psParam), nCchSize, lbComSpecK ? L" /K " : L" /C ");
	}
	if (asParam && *asParam == L' ')
		asParam++;

	WARNING("###: Перенести обработку параметров DosBox в ConEmuC!");
	if (lbUseDosBox)
	{
		lbEndQuote = TRUE;
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
				ms_ExeTmp[0] = 0;
				IsNeedCmd(asParam, &lbNeedCutStartEndQuot, ms_ExeTmp, lbRootIsCmdExe, lbAlwaysConfirmExit, lbAutoDisableConfirmExit);

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
		_wcscat_c((*psParam), nCchSize, L" \"");


#ifdef _DEBUG
	{
		int cchLen = (*psFile ? lstrlen(*psFile) : 0) + (*psParam ? lstrlen(*psParam) : 0) + 128;
		wchar_t* pszDbgMsg = (wchar_t*)calloc(cchLen, sizeof(wchar_t));
		if (pszDbgMsg)
		{
			msprintf(pszDbgMsg, cchLen, L"RunChanged(ParentPID=%u): %s <%s> <%s>\n",
				GetCurrentProcessId(),
				(aCmd == eShellExecute) ? L"Shell" : (aCmd == eCreateProcess) ? L"Create" : L"???",
				*psFile ? *psFile : L"", *psParam ? *psParam : L"");
			OutputDebugString(pszDbgMsg);
			free(pszDbgMsg);
		}
	}
#endif
		
	//if (aCmd == eShellExecute)
	//{
	//	if (asFile && *asFile)
	//	{
	//		lbEndQuote = TRUE;
	//		if (lbUseDosBox)
	//			_wcscat_c((*psParam), nCchSize, L" /DOSBOX");
	//		_wcscat_c((*psParam), nCchSize, lbComSpecK ? L" /K " : L" /C ");
	//		_wcscat_c((*psParam), nCchSize, L"\"\"");
	//		_wcscat_c((*psParam), nCchSize, asFile ? asFile : L"");
	//		_wcscat_c((*psParam), nCchSize, L"\"");
	//	}
	//	else
	//	{
	//		if (lbUseDosBox)
	//			_wcscat_c((*psParam), nCchSize, L" /DOSBOX");
	//		_wcscat_c((*psParam), nCchSize, lbComSpecK ? L" /K " : L" /C ");
	//	}
	//}
	//else
	//{
	//	//(*psParam)[0] = L'"';
	//	//_wcscpy_c((*psParam)+1, nCchSize-1, szConEmuC);
	//	//_wcscat_c((*psParam), nCchSize, L"\"");

	//	if (lbUseDosBox)
	//		_wcscat_c((*psParam), nCchSize, L" /DOSBOX");
	//	_wcscat_c((*psParam), nCchSize, lbComSpecK ? L" /K " : L" /C ");
	//	// Это CreateProcess. Исполняемый файл может быть уже указан в asParam
	//	if (asFile && *asFile)
	//	{
	//		_wcscat_c((*psParam), nCchSize, L"\"");
	//		_ASSERTE(asFile!=NULL);
	//		_wcscat_c((*psParam), nCchSize, asFile);
	//		_wcscat_c((*psParam), nCchSize, L"\"");
	//	}
	//}
	//if (asParam && *asParam)
	//{
	//	_wcscat_c((*psParam), nCchSize, L" ");
	//	_wcscat_c((*psParam), nCchSize, asParam);
	//}
	//if (lbEndQuote)
	//	_wcscat_c((*psParam), nCchSize, L" \"");

	lbRc = TRUE;
wrap:
	if (szComspec)
		free(szComspec);
	if (szConEmuC)
		free(szConEmuC);
	if (szDosBoxExe)
		free(szDosBoxExe);
	if (szDosBoxCfg)
		free(szDosBoxCfg);
	return TRUE;
}

BOOL CShellProc::PrepareExecuteParms(
			enum CmdOnCreateType aCmd,
			LPCWSTR asAction, LPCWSTR asFile, LPCWSTR asParam,
			DWORD* anShellFlags, DWORD* anCreateFlags, DWORD* anStartFlags, DWORD* anShowCmd,
			HANDLE* lphStdIn, HANDLE* lphStdOut, HANDLE* lphStdErr,
			LPWSTR* psFile, LPWSTR* psParam)
{
	// !!! anFlags может быть NULL;
	// !!! asAction может быть NULL;
	_ASSERTE(*psFile==NULL && *psParam==NULL);
	if (!ghConEmuWndDC)
		return FALSE; // Перехватывать только под ConEmu

	// Для логирования - запомним сразу
	HANDLE hIn  = lphStdIn  ? *lphStdIn  : NULL;
	HANDLE hOut = lphStdOut ? *lphStdOut : NULL;
	HANDLE hErr = lphStdErr ? *lphStdErr : NULL;
	
	bool bNewConsoleArg = false, bForceNewConsole = false;
		
	// Issue 351: После перехода исполнятеля фара на ShellExecuteEx почему-то сюда стал приходить
	//            левый хэндл (hStdOutput = 0x00010001), иногда получается 0x00060265
	//            и недокументированный флаг 0x400 в lpStartupInfo->dwFlags
	if ((aCmd == eCreateProcess) && (gnInShellExecuteEx > 0)
		&& lphStdOut && lphStdErr && anStartFlags && (*anStartFlags) == 0x401)
	{
		OSVERSIONINFO osv = {sizeof(OSVERSIONINFO)};

		if (GetVersionEx(&osv))
		{
			if (osv.dwMajorVersion == 5 && (osv.dwMinorVersion == 1/*WinXP*/ || osv.dwMinorVersion == 2/*Win2k3*/))
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
	}
	
	// Проверяем настройку ConEmuGuiMapping.bUseInjects
	if (!LoadGuiMapping() || !(m_SrvMapping.bUseInjects & 1))
	{
		// Настройка в ConEmu ConEmuGuiMapping.bUseInjects, или gFarMode.bFarHookMode. Иначе - сразу выходим
		if (!gFarMode.bFarHookMode /*&& !ghAttachGuiClient*/)
		{
			return FALSE;
		}
	}

		
	//wchar_t *szTest = (wchar_t*)malloc(MAX_PATH*2*sizeof(wchar_t)); //[MAX_PATH*2]
	//wchar_t *szExe = (wchar_t*)malloc((MAX_PATH+1)*sizeof(wchar_t)); //[MAX_PATH+1];
	DWORD /*mn_ImageSubsystem = 0, mn_ImageBits = 0,*/ nFileAttrs = (DWORD)-1;
	bool lbGuiApp = false;
	//int nActionLen = (asAction ? lstrlen(asAction) : 0)+1;
	//int nFileLen = (asFile ? lstrlen(asFile) : 0)+1;
	//int nParamLen = (asParam ? lstrlen(asParam) : 0)+1;
	BOOL lbNeedCutStartEndQuot = FALSE;

	mn_ImageSubsystem = mn_ImageBits = 0;
	
	if (/*(aCmd == eShellExecute) &&*/ asFile && *asFile)
	{
		if (*asFile == L'"')
		{
			LPCWSTR pszEnd = wcschr(asFile+1, L'"');
			if (pszEnd)
			{
				size_t cchLen = (pszEnd - asFile) - 1;
				_wcscpyn_c(ms_ExeTmp, countof(ms_ExeTmp), asFile+1, cchLen);
			}
			else
			{
				_wcscpyn_c(ms_ExeTmp, countof(ms_ExeTmp), asFile+1, countof(ms_ExeTmp));
			}
		}
		else
		{
			_wcscpyn_c(ms_ExeTmp, countof(ms_ExeTmp), asFile, countof(ms_ExeTmp));
		}
	}
	else
	{
		BOOL lbRootIsCmdExe = FALSE, lbAlwaysConfirmExit = FALSE, lbAutoDisableConfirmExit = FALSE;
		IsNeedCmd(asParam, &lbNeedCutStartEndQuot, ms_ExeTmp, lbRootIsCmdExe, lbAlwaysConfirmExit, lbAutoDisableConfirmExit);
	}
	
	if (ms_ExeTmp[0])
	{
		//wchar_t *pszNamePart = NULL;
		int nLen = lstrlen(ms_ExeTmp);
		// Длина больше 0 и не заканчивается слешом
		BOOL lbMayBeFile = (nLen > 0) && (ms_ExeTmp[nLen-1] != L'\\') && (ms_ExeTmp[nLen-1] != L'/');
		//const wchar_t* pszExt = PointToExt(ms_ExeTmp);

		BOOL lbSubsystemOk = FALSE;
		mn_ImageBits = 0;
		mn_ImageSubsystem = IMAGE_SUBSYSTEM_UNKNOWN;

		if (!lbMayBeFile)
		{
			mn_ImageBits = 0;
			mn_ImageSubsystem = IMAGE_SUBSYSTEM_UNKNOWN;
		}
		//else if (pszExt && (!lstrcmpi(pszExt, L".cmd") || !lstrcmpi(pszExt, L".bat")))
		//{
		//	lbGuiApp = FALSE;
		//	mn_ImageSubsystem = IMAGE_SUBSYSTEM_BATCH_FILE;
		//	mn_ImageBits = IsWindows64() ? 64 : 32;
		//	lbSubsystemOk = TRUE;
		//}
		else if (FindImageSubsystem(ms_ExeTmp, mn_ImageSubsystem, mn_ImageBits, nFileAttrs))
		{
			lbGuiApp = (mn_ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI);
			lbSubsystemOk = TRUE;
		}
		//else if (GetFullPathName(ms_ExeTmp, countof(szTest), szTest, &pszNamePart)
		//	&& GetImageSubsystem(szTest, mn_ImageSubsystem, mn_ImageBits, nFileAttrs))
		//{
		//	lbGuiApp = (mn_ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI);
		//	lbSubsystemOk = TRUE;
		//}
		//else if (SearchPath(NULL, ms_ExeTmp, NULL, countof(szTest), szTest, &pszNamePart)
		//	&& GetImageSubsystem(szTest, mn_ImageSubsystem, mn_ImageBits, nFileAttrs))
		//{
		//	lbGuiApp = (mn_ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI);
		//	lbSubsystemOk = TRUE;
		//}
		//else
		//{
		//	szTest[0] = 0;
		//}
		

		//if (!lbSubsystemOk)
		//{
		//	mn_ImageBits = 0;
		//	mn_ImageSubsystem = IMAGE_SUBSYSTEM_UNKNOWN;

		//	if ((nFileAttrs != (DWORD)-1) && !(nFileAttrs & FILE_ATTRIBUTE_DIRECTORY))
		//	{
		//		LPCWSTR pszExt = wcsrchr(szTest, L'.');
		//		if (pszExt)
		//		{
		//			if ((lstrcmpiW(pszExt, L".cmd") == 0) || (lstrcmpiW(pszExt, L".bat") == 0))
		//			{
		//				#ifdef _WIN64
		//				mn_ImageBits = 64;
		//				#else
		//				mn_ImageBits = IsWindows64() ? 64 : 32;
		//				#endif
		//				mn_ImageSubsystem = IMAGE_SUBSYSTEM_BATCH_FILE;
		//			}
		//		}
		//	}
		//}
	}
	
	BOOL lbChanged = FALSE;
	mb_NeedInjects = FALSE;
	//wchar_t szBaseDir[MAX_PATH+2]; szBaseDir[0] = 0;
	CESERVER_REQ *pIn = NULL;
	pIn = NewCmdOnCreate(aCmd, 
			asAction, asFile, asParam, 
			anShellFlags, anCreateFlags, anStartFlags, anShowCmd, 
			mn_ImageBits, mn_ImageSubsystem, 
			hIn, hOut, hErr/*, szBaseDir, mb_DosBoxAllowed*/);
	if (pIn)
	{
		HWND hConWnd = GetConsoleWindow();
		CESERVER_REQ *pOut = NULL;
		pOut = ExecuteGuiCmd(hConWnd, pIn, hConWnd);
		ExecuteFreeResult(pIn); pIn = NULL;
		if (!pOut)
			goto wrap;
		if (!pOut->OnCreateProcRet.bContinue)
			goto wrap;
		ExecuteFreeResult(pOut);
	}

#ifdef _DEBUG
	{
		int cchLen = (asFile ? lstrlen(asFile) : 0) + (asParam ? lstrlen(asParam) : 0) + 128;
		wchar_t* pszDbgMsg = (wchar_t*)calloc(cchLen, sizeof(wchar_t));
		if (pszDbgMsg)
		{
			msprintf(pszDbgMsg, cchLen, L"Run(ParentPID=%u): %s <%s> <%s>\n",
				GetCurrentProcessId(),
				(aCmd == eShellExecute) ? L"Shell" : (aCmd == eCreateProcess) ? L"Create" : L"???",
				asFile ? asFile : L"", asParam ? asParam : L"");
			OutputDebugString(pszDbgMsg);
			free(pszDbgMsg);
		}
	}
#endif

	//wchar_t* pszExecFile = (wchar_t*)pOut->OnCreateProcRet.wsValue;
	//wchar_t* pszBaseDir = (wchar_t*)(pOut->OnCreateProcRet.wsValue); // + pOut->OnCreateProcRet.nFileLen);
	
	if (asParam)
	{
		const wchar_t* sNewConsole = L"-new_console";
		int nNewConsoleLen = lstrlen(sNewConsole);
		const wchar_t* pszFind = wcsstr(asParam, sNewConsole);
		// 111211 - после "-new_console:" теперь допускаются аргументы
		if (pszFind && ((pszFind == asParam) || (*(pszFind-1) == L' ') || (*(pszFind-1) == L'"'))
			&& ((pszFind[nNewConsoleLen] == 0) || (pszFind[nNewConsoleLen] == L' ') || (pszFind[nNewConsoleLen] == L':') || (pszFind[nNewConsoleLen] == L'"')))
		{
			bNewConsoleArg = true;
		}
	}
	// Если GUI приложение работает во вкладке ConEmu - запускать консольные приложение в новой вкладке ConEmu
	if (!bNewConsoleArg 
		&& ghAttachGuiClient && (mn_ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI)
		&& ((anShowCmd == NULL) || (*anShowCmd != SW_HIDE)))
	{
		WARNING("Не забыть, что цеплять во вкладку нужно только если консоль запускается ВИДИМОЙ");
		bForceNewConsole = true;
	}
	if (ghAttachGuiClient && (bNewConsoleArg || bForceNewConsole) && !lbGuiApp)
	{
		lbGuiApp = true;
	}
	
	if (aCmd == eShellExecute)
	{
		WARNING("Уточнить условие для флагов ShellExecute!");
		// !!! anFlags может быть NULL;
		DWORD nFlagsMask = (SEE_MASK_FLAG_NO_UI|SEE_MASK_NOASYNC|SEE_MASK_NOCLOSEPROCESS|SEE_MASK_NO_CONSOLE);
		DWORD nFlags = (anShellFlags ? *anShellFlags : 0) & nFlagsMask;
		// Если bNewConsoleArg - то однозначно стартовать в новой вкладке ConEmu (GUI теперь тоже цеплять умеем)
		if (bNewConsoleArg || bForceNewConsole)
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
		else if (nFlags != nFlagsMask)
		{
			goto wrap; // пока так - это фар выполняет консольную команду
		}
		if (asAction && (lstrcmpiW(asAction, L"open") != 0))
		{
			goto wrap; // runas, print, и прочая нас не интересует
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

	if (lbGuiApp && !(bNewConsoleArg || bForceNewConsole))
		goto wrap; // гуй - не перехватывать (если только не указан "-new_console")

	// Подставлять ConEmuC.exe нужно только для того, чтобы 
	//	1. в фаре включить длинный буфер и запомнить длинный результат в консоли (ну и ConsoleAlias обработать)
	//	2. при вызовах ShellExecute/ShellExecuteEx, т.к. не факт,
	//     что этот ShellExecute вызовет CreateProcess из kernel32 (который перехвачен).
	//     В Win7 это может быть вызов других системных модулей (App-.....dll)

	#ifdef _DEBUG
	// Для принудительной вставки ConEmuC.exe - поставить true. Только для отладки!
	bool lbAlwaysAddConEmuC = false;
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
	if ((gFarMode.bFarHookMode)
		|| (lbGuiApp && (bNewConsoleArg || bForceNewConsole)) // хотят GUI прицепить к новуй вкладке в ConEmu, или новую консоль из GUI
		// eCreateProcess перехватывать не нужно (сами сделаем InjectHooks после CreateProcess)
		|| ((mn_ImageBits != 16) && (m_SrvMapping.bUseInjects & 1) 
			&& ((aCmd == eShellExecute) 
				#ifdef _DEBUG
				|| lbAlwaysAddConEmuC
				#endif
				))
		// если это Дос-приложение - то если включен DosBox, вставляем ConEmuC.exe /DOSBOX
		|| ((mn_ImageBits == 16) && (mn_ImageSubsystem == IMAGE_SUBSYSTEM_DOS_EXECUTABLE)
		    && m_SrvMapping.cbSize && m_SrvMapping.bDosBox))
	{
		lbChanged = ChangeExecuteParms(aCmd, bNewConsoleArg, asFile, asParam, /*szBaseDir, */
						ms_ExeTmp, mn_ImageBits, mn_ImageSubsystem, psFile, psParam);
		if (!lbChanged)
		{
			// Хуки нельзя ставить в 16битные приложение - будет облом, ntvdm.exe игнорировать!
			mb_NeedInjects = (mn_ImageBits != 16);
		}
		else
		{
			HWND hConWnd = GetConsoleWindow();

			if (lbGuiApp && (bNewConsoleArg || bForceNewConsole))
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
					asAction, *psFile, *psParam, 
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
		mb_NeedInjects = (aCmd == eCreateProcess) && (mn_ImageBits != 16);
	}

wrap:
	return lbChanged;
}

BOOL CShellProc::OnShellExecuteA(LPCSTR* asAction, LPCSTR* asFile, LPCSTR* asParam, DWORD* anFlags, DWORD* anShowCmd)
{
	if (!ghConEmuWndDC || !isWindow(ghConEmuWndDC))
		return FALSE; // Перехватывать только под ConEmu
		
	mb_InShellExecuteEx = TRUE;
	gnInShellExecuteEx ++;

	mpwsz_TempAction = str2wcs(asAction ? *asAction : NULL, mn_CP);
	mpwsz_TempFile = str2wcs(asFile ? *asFile : NULL, mn_CP);
	mpwsz_TempParam = str2wcs(asParam ? *asParam : NULL, mn_CP);

	BOOL lbRc = PrepareExecuteParms(eShellExecute,
					mpwsz_TempAction, mpwsz_TempFile, mpwsz_TempParam,
					anFlags, NULL, NULL, anShowCmd,
					NULL, NULL, NULL, // *StdHandles
					&mpwsz_TempRetFile, &mpwsz_TempRetParam);
	if (lbRc)
	{
		if (mpwsz_TempRetFile && *mpwsz_TempRetFile)
		{
			mpsz_TempRetFile = wcs2str(mpwsz_TempRetFile, mn_CP);
			*asFile = mpsz_TempRetFile;
		}
		else
		{
			*asFile = NULL;
		}
		mpsz_TempRetParam = wcs2str(mpwsz_TempRetParam, mn_CP);
		*asParam = mpsz_TempRetParam;
	}

	return lbRc;
}
BOOL CShellProc::OnShellExecuteW(LPCWSTR* asAction, LPCWSTR* asFile, LPCWSTR* asParam, DWORD* anFlags, DWORD* anShowCmd)
{
	if (!ghConEmuWndDC || !isWindow(ghConEmuWndDC))
		return FALSE; // Перехватывать только под ConEmu
	
	mb_InShellExecuteEx = TRUE;
	gnInShellExecuteEx ++;

	BOOL lbRc = PrepareExecuteParms(eShellExecute,
					asAction ? *asAction : NULL,
					asFile ? *asFile : NULL,
					asParam ? *asParam : NULL,
					anFlags, NULL, NULL, anShowCmd,
					NULL, NULL, NULL, // *StdHandles
					&mpwsz_TempRetFile, &mpwsz_TempRetParam);
	if (lbRc)
	{
		*asFile = mpwsz_TempRetFile;
		*asParam = mpwsz_TempRetParam;
	}

	return lbRc;
}
BOOL CShellProc::FixShellArgs(DWORD afMask, HWND ahWnd, DWORD* pfMask, HWND* phWnd)
{
	BOOL lbRc = FALSE;

	// Включить флажок, чтобы Shell не задавал глупого вопроса "Хотите ли вы запустить этот файл"...
	if (!(afMask & SEE_MASK_NOZONECHECKS) && gFarMode.bFarHookMode && gFarMode.bShellNoZoneCheck)
	{
		OSVERSIONINFOEX osv = {sizeof(OSVERSIONINFOEX)};
		if (GetVersionEx((LPOSVERSIONINFO)&osv))
		{
			if ((osv.dwMajorVersion >= 6)
				|| (osv.dwMajorVersion == 5 
				&& (osv.dwMinorVersion > 1 
				|| (osv.dwMinorVersion == 1 && osv.wServicePackMajor >= 1))))
			{
				(*pfMask) |= SEE_MASK_NOZONECHECKS;
				lbRc = TRUE;
			}
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
BOOL CShellProc::OnShellExecuteExA(LPSHELLEXECUTEINFOA* lpExecInfo)
{
	if (!ghConEmuWndDC || !isWindow(ghConEmuWndDC) || !lpExecInfo)
		return FALSE; // Перехватывать только под ConEmu

	mlp_SaveExecInfoA = *lpExecInfo;
	mlp_ExecInfoA = (LPSHELLEXECUTEINFOA)malloc((*lpExecInfo)->cbSize);
	if (!mlp_ExecInfoA)
	{
		_ASSERTE(mlp_ExecInfoA!=NULL);
		return FALSE;
	}
	memmove(mlp_ExecInfoA, (*lpExecInfo), (*lpExecInfo)->cbSize);
	
	BOOL lbRc = FALSE;

	if (FixShellArgs((*lpExecInfo)->fMask, (*lpExecInfo)->hwnd, &(mlp_ExecInfoA->fMask), &(mlp_ExecInfoA->hwnd)))
		lbRc = TRUE;

	if (OnShellExecuteA(&mlp_ExecInfoA->lpVerb, &mlp_ExecInfoA->lpFile, &mlp_ExecInfoA->lpParameters, &mlp_ExecInfoA->fMask, (DWORD*)&mlp_ExecInfoA->nShow))
		lbRc = TRUE;

	if (lbRc)
		*lpExecInfo = mlp_ExecInfoA;
	return lbRc;
}
BOOL CShellProc::OnShellExecuteExW(LPSHELLEXECUTEINFOW* lpExecInfo)
{
	if (!ghConEmuWndDC || !isWindow(ghConEmuWndDC) || !lpExecInfo)
		return FALSE; // Перехватывать только под ConEmu

	mlp_SaveExecInfoW = *lpExecInfo;
	mlp_ExecInfoW = (LPSHELLEXECUTEINFOW)malloc((*lpExecInfo)->cbSize);
	if (!mlp_ExecInfoW)
	{
		_ASSERTE(mlp_ExecInfoW!=NULL);
		return FALSE;
	}
	memmove(mlp_ExecInfoW, (*lpExecInfo), (*lpExecInfo)->cbSize);
	
	BOOL lbRc = FALSE;

	if (FixShellArgs((*lpExecInfo)->fMask, (*lpExecInfo)->hwnd, &(mlp_ExecInfoW->fMask), &(mlp_ExecInfoW->hwnd)))
		lbRc = TRUE;

	if (OnShellExecuteW(&mlp_ExecInfoW->lpVerb, &mlp_ExecInfoW->lpFile, &mlp_ExecInfoW->lpParameters, &mlp_ExecInfoW->fMask, (DWORD*)&mlp_ExecInfoW->nShow))
		lbRc = TRUE;

	if (lbRc)
		*lpExecInfo = mlp_ExecInfoW;
	return lbRc;
}
void CShellProc::OnCreateProcessA(LPCSTR* asFile, LPCSTR* asCmdLine, DWORD* anCreationFlags, LPSTARTUPINFOA lpSI)
{
	if (!ghConEmuWndDC || !isWindow(ghConEmuWndDC))
		return; // Перехватывать только под ConEmu

	mpwsz_TempFile = str2wcs(asFile ? *asFile : NULL, mn_CP);
	mpwsz_TempParam = str2wcs(asCmdLine ? *asCmdLine : NULL, mn_CP);
	DWORD nShowCmd = lpSI->wShowWindow;
	mb_WasSuspended = ((*anCreationFlags) & CREATE_SUSPENDED) == CREATE_SUSPENDED;

	BOOL lbRc = PrepareExecuteParms(eCreateProcess,
					NULL, mpwsz_TempFile, mpwsz_TempParam,
					NULL, anCreationFlags, &lpSI->dwFlags, &nShowCmd,
					&lpSI->hStdInput, &lpSI->hStdOutput, &lpSI->hStdError,
					&mpwsz_TempRetFile, &mpwsz_TempRetParam);
	// возвращает TRUE только если были изменены СТРОКИ,
	// а если выставлен mb_NeedInjects - строго включить _Suspended
	if (mb_NeedInjects)
		(*anCreationFlags) |= CREATE_SUSPENDED;
	if (lbRc)
	{
		if (lpSI->wShowWindow != nShowCmd)
			lpSI->wShowWindow = (WORD)nShowCmd;
		if (mpwsz_TempRetFile)
		{
			mpsz_TempRetFile = wcs2str(mpwsz_TempRetFile, mn_CP);
			*asFile = mpsz_TempRetFile;
		}
		else
		{
			*asFile = NULL;
		}
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
}
void CShellProc::OnCreateProcessW(LPCWSTR* asFile, LPCWSTR* asCmdLine, DWORD* anCreationFlags, LPSTARTUPINFOW lpSI)
{
	if (!ghConEmuWndDC || !isWindow(ghConEmuWndDC))
		return; // Перехватывать только под ConEmu
		
	DWORD nShowCmd = (lpSI->dwFlags & STARTF_USESHOWWINDOW) ? lpSI->wShowWindow : SW_SHOWNORMAL;
	mb_WasSuspended = ((*anCreationFlags) & CREATE_SUSPENDED) == CREATE_SUSPENDED;

	BOOL lbRc = PrepareExecuteParms(eCreateProcess,
					NULL,
					asFile ? *asFile : NULL,
					asCmdLine ? *asCmdLine : NULL,
					NULL, anCreationFlags, &lpSI->dwFlags, &nShowCmd,
					&lpSI->hStdInput, &lpSI->hStdOutput, &lpSI->hStdError,
					&mpwsz_TempRetFile, &mpwsz_TempRetParam);
	// возвращает TRUE только если были изменены СТРОКИ,
	// а если выставлен mb_NeedInjects - строго включить _Suspended
	if (mb_NeedInjects)
		(*anCreationFlags) |= CREATE_SUSPENDED;
	if (lbRc)
	{
		if (lpSI->wShowWindow != nShowCmd)
		{
			lpSI->wShowWindow = (WORD)nShowCmd;
			if (!(lpSI->dwFlags & STARTF_USESHOWWINDOW))
				lpSI->dwFlags |= STARTF_USESHOWWINDOW;
		}
		*asFile = mpwsz_TempRetFile;
		*asCmdLine = mpwsz_TempRetParam;
	}
}

void CShellProc::OnCreateProcessFinished(BOOL abSucceeded, PROCESS_INFORMATION *lpPI)
{
#ifdef _DEBUG
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
			OutputDebugString(pszDbgMsg);
			free(pszDbgMsg);
		}
	}
#endif

	if (abSucceeded)
	{
		if (mb_NeedInjects)
		{
			wchar_t szDbgMsg[255];
			#ifdef _DEBUG
			msprintf(szDbgMsg, countof(szDbgMsg), L"InjectHooks(x%u), ParentPID=%u, ChildPID=%u\n",
					#ifdef _WIN64
						64
					#else
						86
					#endif
						, GetCurrentProcessId(), lpPI->dwProcessId);
			OutputDebugString(szDbgMsg);
			#endif

			int iHookRc = InjectHooks(*lpPI, FALSE, (m_SrvMapping.cbSize && (m_SrvMapping.nLoggingType == glt_Processes)));

			if (iHookRc != 0)
			{
				DWORD nErrCode = GetLastError();
				// Хуки не получится установить для некоторых системных процессов типа ntvdm.exe,
				// но при запуске dos приложений мы сюда дойти не должны
				_ASSERTE(iHookRc == 0);
				wchar_t szTitle[128];
				msprintf(szTitle, countof(szTitle), L"ConEmuC, PID=%u", GetCurrentProcessId());
				msprintf(szDbgMsg, countof(szDbgMsg), L"ConEmuC.W, PID=%u\nInjecting hooks into PID=%u\nFAILED, code=%i:0x%08X",
					GetCurrentProcessId(), lpPI->dwProcessId, iHookRc, nErrCode);
				GuiMessageBox(NULL, szDbgMsg, szTitle, MB_SYSTEMMODAL);
			}

			// Отпустить процесс
			if (!mb_WasSuspended)
				ResumeThread(lpPI->hThread);
		}
	}
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
