
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
#include "..\common\common.hpp"
#include "..\common\ConEmuCheck.h"
#include "..\common\SetHook.h"
#include "..\common\execute.h"
#include "ConEmuC.h"
#include "ShellProcessor.h"

#ifndef SEE_MASK_NOZONECHECKS
#define SEE_MASK_NOZONECHECKS 0x800000
#endif
#ifndef SEE_MASK_NOASYNC
#define SEE_MASK_NOASYNC 0x00000100
#endif


int CShellProc::mn_InShellExecuteEx = 0;

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
	mb_DosBoxAllowed = FALSE;
}

CShellProc::~CShellProc()
{
	if (mb_InShellExecuteEx && mn_InShellExecuteEx)
	{
		mn_InShellExecuteEx--;
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

CESERVER_REQ* CShellProc::NewCmdOnCreate(enum CmdOnCreateType aCmd,
				LPCWSTR asAction, LPCWSTR asFile, LPCWSTR asParam,
				DWORD* anShellFlags, DWORD* anCreateFlags, DWORD* anStartFlags, DWORD* anShowCmd,
				int mn_ImageBits, int mn_ImageSubsystem,
				HANDLE hStdIn, HANDLE hStdOut, HANDLE hStdErr,
				wchar_t (&szBaseDir)[MAX_PATH+2], BOOL& bDosBoxAllowed)
{
	szBaseDir[0] = 0;

	// Проверим, а надо ли?
	DWORD dwGuiProcessId = 0;
	if (!ghConEmuWnd || !GetWindowThreadProcessId(ghConEmuWnd, &dwGuiProcessId))
		return NULL;

	MFileMapping<ConEmuGuiMapping> GuiInfoMapping;
	GuiInfoMapping.InitName(CEGUIINFOMAPNAME, dwGuiProcessId);
	const ConEmuGuiMapping* pInfo = GuiInfoMapping.Open();
	if (!pInfo)
		return NULL;
	else if (pInfo->nProtocolVersion != CESERVER_REQ_VER)
		return NULL;
	else
	{
		bDosBoxAllowed = pInfo->bDosBox;
		wcscpy_c(szBaseDir, pInfo->sConEmuBaseDir);
		wcscat_c(szBaseDir, L"\\");
		if (pInfo->nLoggingType != glt_Processes)
			return NULL;
	}
	GuiInfoMapping.CloseMap();

	
	CESERVER_REQ *pIn = NULL;
	
	int nActionLen = (asAction ? lstrlen(asAction) : 0)+1;
	int nFileLen = (asFile ? lstrlen(asFile) : 0)+1;
	int nParamLen = (asParam ? lstrlen(asParam) : 0)+1;
	
	pIn = ExecuteNewCmd(CECMD_ONCREATEPROC, sizeof(CESERVER_REQ_HDR)
		+sizeof(CESERVER_REQ_ONCREATEPROCESS)+(nActionLen+nFileLen+nParamLen)*sizeof(wchar_t));
	
#ifdef _WIN64
	pIn->OnCreateProc.nSourceBits = 64;
#else
	pIn->OnCreateProc.nSourceBits = 32;
#endif
	//pIn->OnCreateProc.bUnicode = TRUE;
	pIn->OnCreateProc.nImageSubsystem = mn_ImageSubsystem;
	pIn->OnCreateProc.nImageBits = mn_ImageBits;
	pIn->OnCreateProc.hStdIn = (unsigned __int64)hStdIn;
	pIn->OnCreateProc.hStdOut = (unsigned __int64)hStdOut;
	pIn->OnCreateProc.hStdErr = (unsigned __int64)hStdErr;
	
	if (aCmd == eShellExecute)
		wcscpy_c(pIn->OnCreateProc.sFunction, L"Shell");
	else if (aCmd == eCreateProcess)
		wcscpy_c(pIn->OnCreateProc.sFunction, L"Create");
	else if (aCmd == eInjectingHooks)
		wcscpy_c(pIn->OnCreateProc.sFunction, L"Hooks");
	else if (aCmd == eHooksLoaded)
		wcscpy_c(pIn->OnCreateProc.sFunction, L"Loaded");
	else if (aCmd == eParmsChanged)
		wcscpy_c(pIn->OnCreateProc.sFunction, L"Changed");
	else
		wcscpy_c(pIn->OnCreateProc.sFunction, L"Unknown");
	
	pIn->OnCreateProc.nShellFlags = anShellFlags ? *anShellFlags : 0;
	pIn->OnCreateProc.nCreateFlags = anCreateFlags ? *anCreateFlags : 0;
	pIn->OnCreateProc.nStartFlags = anStartFlags ? *anStartFlags : 0;
	pIn->OnCreateProc.nShowCmd = anShowCmd ? *anShowCmd : 0;
	pIn->OnCreateProc.nActionLen = nActionLen;
	pIn->OnCreateProc.nFileLen = nFileLen;
	pIn->OnCreateProc.nParamLen = nParamLen;
	
	wchar_t* psz = pIn->OnCreateProc.wsValue;
	if (nActionLen > 1)
		_wcscpy_c(psz, nActionLen, asAction);
	psz += nActionLen;
	if (nFileLen > 1)
		_wcscpy_c(psz, nFileLen, asFile);
	psz += nFileLen;
	if (nParamLen > 1)
		_wcscpy_c(psz, nParamLen, asParam);
	psz += nParamLen;
	
	return pIn;
}

BOOL CShellProc::ChangeExecuteParms(enum CmdOnCreateType aCmd,
				LPCWSTR asFile, LPCWSTR asParam, LPCWSTR asBaseDir,
				LPCWSTR asExeFile, DWORD& ImageBits, DWORD& ImageSubsystem,
				LPWSTR* psFile, LPWSTR* psParam)
{
	wchar_t szConEmuC[MAX_PATH+16]; // ConEmuC64.exe
	wcscpy_c(szConEmuC, asBaseDir);
	
	_ASSERTE(aCmd==eShellExecute || aCmd==eCreateProcess);

	BOOL lbComSpecK = FALSE; // TRUE - если нужно запустить /K, а не /C

	if (aCmd == eCreateProcess)
	{
		// Для простоты - сразу откинем asFile если он совпадает с первым аргументом в asParam
		if (asFile && !*asFile) asFile = NULL;
		if (asFile && *asFile && asParam && *asParam)
		{
			LPCWSTR pszParam = SkipNonPrintable(asParam);
			while (*pszParam == L'"') pszParam++;
			int nLen = lstrlen(asFile)+1;
			wchar_t* pszTest = (wchar_t*)malloc(nLen*sizeof(wchar_t));
			_ASSERTE(pszTest);
			if (pszTest)
			{
				_wcscpyn_c(pszTest, nLen, pszParam, nLen);
				pszTest[nLen-1] = 0;
				// Сравнить asFile с первым аргументом в asParam
				if (lstrcmpi(pszTest, asFile) == 0)
				{
					// exe-шник уже указан в asParam, добавлять дополнительно НЕ нужно
					asFile = NULL;
				}
				free(pszTest);
			}
		}
	}

	wchar_t szComspec[MAX_PATH+20]; szComspec[0] = 0;
	if (GetEnvironmentVariable(L"ComSpec", szComspec, countof(szComspec)))
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
	BOOL lbComSpec = FALSE; // TRUE - если %COMSPEC% отбрасывается
	if (asParam)
	{
		const wchar_t* psz = SkipNonPrintable(asParam);
		// Если запускают cmd.exe без параметров - не отбрасывать его!
		if (szComspec[0] && *psz)
		{
			// asFile может быть и для ShellExecute и для CreateProcess, проверим в одном месте
			if (asFile && (lstrcmpi(szComspec, asFile) == 0))
			{
				if (psz[0] == L'/' && wcschr(L"CcKk", psz[1]))
				{
					// не добавлять в измененную команду asFile (это отбрасываемый cmd.exe)
					lbComSpecK = (psz[1] == L'K' || psz[1] == L'k');
					asFile = NULL;
					asParam = psz+2; // /C или /K добавляется к ConEmuC.exe
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
				int nLen = lstrlen(szComspec)+1;
				wchar_t* pszTest = (wchar_t*)malloc(nLen*sizeof(wchar_t));
				_ASSERTE(pszTest);
				if (pszTest)
				{
					_wcscpyn_c(pszTest, nLen, (*psz == L'"') ? (psz+1) : psz, nLen);
					pszTest[nLen-1] = 0;
					// Сравнить первый аргумент в asParam с %COMSPEC%
					const wchar_t* pszCmdLeft = NULL;
					if (lstrcmpi(pszTest, szComspec) == 0)
					{
						if (*psz == L'"')
						{
							WARNING("!!! Проверить ветку !!!");
							if (psz[nLen+1] == L'"')
							{
								pszCmdLeft = psz+nLen+2;
							}
						}
						else
						{
							pszCmdLeft = psz+nLen-1;
						}
						// Теперь нужно проверить, что там в хвосте команды (если команды нет - оставить cmd.exe)
						pszCmdLeft = SkipNonPrintable(pszCmdLeft);
						if (pszCmdLeft[0] == L'/' && wcschr(L"CcKk", pszCmdLeft[1]))
						{
							// не добавлять в измененную команду asFile (это отбрасываемый cmd.exe)
							lbComSpecK = (psz[1] == L'K' || psz[1] == L'k');
							_ASSERTE(asFile == NULL); // уже должен быть NULL
							asParam = pszCmdLeft+2; // /C или /K добавляется к ConEmuC.exe
							lbComSpec = TRUE;
						}
					}
					free(pszTest);
				}
			}
		}
	}

	wchar_t szNewExe[MAX_PATH+1];
	const wchar_t* pszExeExt = NULL;
	// Проверить, может запускается батник?
	if (/*lbComSpec &&*/ !asFile && asParam)
	{
		const wchar_t* pszTemp = asParam;
		if (0 == NextArg(&pszTemp, szNewExe))
		{
			pszExeExt = PointToExt(szNewExe);
		}
	}
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
	



	if (ImageBits == 32)
	{
		wcscat_c(szConEmuC, L"ConEmuC.exe");
	}
	else if (ImageBits == 64)
	{
		wcscat_c(szConEmuC, L"ConEmuC64.exe");
	}
	else if (ImageBits == 16)
	{
		if (mb_DosBoxAllowed)
		{
			wcscat_c(szConEmuC, L"ConEmuC.exe");
		}
		else
		{
			_ASSERTE(ImageBits != 16);
			return FALSE;
		}
	}
	else
	{
		_ASSERTE(ImageBits==16||ImageBits==32||ImageBits==64);
		wcscat_c(szConEmuC, L"ConEmuC.exe");
	}
	
	int nCchSize = (asFile ? lstrlen(asFile) : 0) + (asParam ? lstrlen(asParam) : 0) + 64;
	if (aCmd == eShellExecute)
	{
		*psFile = lstrdup(szConEmuC);
	}
	else
	{
		nCchSize += lstrlen(szConEmuC);
		*psFile = NULL;
	}
	
	*psParam = (wchar_t*)malloc(nCchSize*sizeof(wchar_t));
	(*psParam)[0] = 0;
	// В ShellExecute необходимо "ConEmuC.exe" вернуть в psFile, а для CreatePocess - в psParam
	// /C или /K в обоих случаях нужно пихать в psParam
	BOOL lbDoubleQuote = FALSE;
	if (aCmd == eShellExecute)
	{
		if (asFile && *asFile)
		{
			lbDoubleQuote = TRUE;
			if (ImageBits == 16)
				_wcscat_c((*psParam), nCchSize, L" /DOSBOX");
			_wcscat_c((*psParam), nCchSize, lbComSpecK ? L" /K \"\"" : L" /C \"\"");
			_wcscat_c((*psParam), nCchSize, asFile ? asFile : L"");
			_wcscat_c((*psParam), nCchSize, L"\"");
		}
		else
		{
			if (ImageBits == 16)
				_wcscat_c((*psParam), nCchSize, L" /DOSBOX");
			_wcscat_c((*psParam), nCchSize, lbComSpecK ? L" /K " : L" /C ");
		}
	}
	else
	{
		(*psParam)[0] = L'"';
		_wcscpy_c((*psParam)+1, nCchSize-1, szConEmuC);
		_wcscat_c((*psParam), nCchSize, L"\"");
		if (ImageBits == 16)
			_wcscat_c((*psParam), nCchSize, L" /DOSBOX");
		_wcscat_c((*psParam), nCchSize, lbComSpecK ? L" /K " : L" /C ");
		// Это CreateProcess. Исполняемый файл может быть уже указан в asParam
		//BOOL lbNeedExe = (asFile && *asFile);
		// уже отсечено выше
		//if (lbNeedExe && asParam && *asParam)
		//{
		//	LPCWSTR pszParam = asParam;
		//	while (*pszParam == L'"') pszParam++;
		//	int nLen = lstrlen(asExeFile);
		//	wchar_t szTest[MAX_PATH*2];
		//	_ASSERTE(nLen <= (MAX_PATH+10)); // размер из IsNeedCmd.
		//	_wcscpyn_c(szTest, countof(szTest), pszParam, nLen+1);
		//	// Сравнить asFile с первым аргументом в asParam
		//	if (lstrcmpiW(szTest, asExeFile) == 0)
		//	{
		//		// exe-шник уже указан в asParam, добавлять дополнительно НЕ нужно
		//		lbNeedExe = FALSE;
		//	}
		//}
		//if (lbNeedExe)
		if (asFile && *asFile)
		{
			_wcscat_c((*psParam), nCchSize, L"\"");
			_ASSERTE(asFile!=NULL);
			_wcscat_c((*psParam), nCchSize, asFile);
			_wcscat_c((*psParam), nCchSize, L"\"");
		}
	}

	if (asParam && *asParam)
	{
		_wcscat_c((*psParam), nCchSize, L" ");
		_wcscat_c((*psParam), nCchSize, asParam);
		if ((aCmd == eShellExecute) && lbDoubleQuote)
		_wcscat_c((*psParam), nCchSize, L"\"");
	}
	else if ((aCmd == eShellExecute) && lbDoubleQuote)
		_wcscat_c((*psParam), nCchSize, L" \"");

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
		
	// Issue 351: После перехода исполнятеля фара на ShellExecuteEx почему-то сюда стал приходить
	//            левый хэндл (hStdOutput = 0x00010001)
	//            и недокументированный флаг 0x400 в lpStartupInfo->dwFlags
	if ((aCmd == eCreateProcess) && (mn_InShellExecuteEx > 0)
		&& lphStdOut && lphStdErr && anStartFlags && (*anStartFlags) == 0x401)
	{
		OSVERSIONINFO osv = {sizeof(OSVERSIONINFO)};

		if (GetVersionEx(&osv))
		{
			if (osv.dwMajorVersion == 5 && (osv.dwMinorVersion == 1/*WinXP*/ || osv.dwMinorVersion == 2/*Win2k3*/))
			{
				if ((*lphStdOut == (HANDLE)0x00010001) && (*lphStdErr == NULL))
				{
					*lphStdOut = NULL;
					*anStartFlags &= ~0x400;
				}
			}
		}
	}
		
		
	wchar_t szTest[MAX_PATH*2], szExe[MAX_PATH+1];
	DWORD /*mn_ImageSubsystem = 0, mn_ImageBits = 0,*/ nFileAttrs = (DWORD)-1;
	bool lbGuiApp = false;
	//int nActionLen = (asAction ? lstrlen(asAction) : 0)+1;
	//int nFileLen = (asFile ? lstrlen(asFile) : 0)+1;
	//int nParamLen = (asParam ? lstrlen(asParam) : 0)+1;
	BOOL lbNeedCutStartEndQuot = FALSE;

	mn_ImageSubsystem = mn_ImageBits = 0;
	
	if (/*(aCmd == eShellExecute) &&*/ (asFile && *asFile))
	{
		wcscpy_c(szExe, asFile ? asFile : L"");
	}
	else
	{
		IsNeedCmd(asParam, &lbNeedCutStartEndQuot, szExe);
	}
	
	if (szExe[0])
	{
		wchar_t *pszNamePart = NULL;
		int nLen = lstrlen(szExe);
		// Длина больше 0 и не заканчивается слешом
		BOOL lbMayBeFile = (nLen > 0) && (szExe[nLen-1] != L'\\') && (szExe[nLen-1] != L'/');
		const wchar_t* pszExt = PointToExt(szExe);

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
		else if (FindImageSubsystem(szExe, mn_ImageSubsystem, mn_ImageBits, nFileAttrs))
		{
			lbGuiApp = (mn_ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI);
			lbSubsystemOk = TRUE;
		}
		//else if (GetFullPathName(szExe, countof(szTest), szTest, &pszNamePart)
		//	&& GetImageSubsystem(szTest, mn_ImageSubsystem, mn_ImageBits, nFileAttrs))
		//{
		//	lbGuiApp = (mn_ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI);
		//	lbSubsystemOk = TRUE;
		//}
		//else if (SearchPath(NULL, szExe, NULL, countof(szTest), szTest, &pszNamePart)
		//	&& GetImageSubsystem(szTest, mn_ImageSubsystem, mn_ImageBits, nFileAttrs))
		//{
		//	lbGuiApp = (mn_ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI);
		//	lbSubsystemOk = TRUE;
		//}
		else
		{
			szTest[0] = 0;
		}
		

		if (!lbSubsystemOk)
		{
			mn_ImageBits = 0;
			mn_ImageSubsystem = IMAGE_SUBSYSTEM_UNKNOWN;

			if ((nFileAttrs != (DWORD)-1) && !(nFileAttrs & FILE_ATTRIBUTE_DIRECTORY))
			{
				LPCWSTR pszExt = wcsrchr(szTest, L'.');
				if (pszExt)
				{
					if ((lstrcmpiW(pszExt, L".cmd") == 0) || (lstrcmpiW(pszExt, L".bat") == 0))
					{
						#ifdef _WIN64
						mn_ImageBits = 64;
						#else
						mn_ImageBits = IsWindows64() ? 64 : 32;
						#endif
						mn_ImageSubsystem = IMAGE_SUBSYSTEM_BATCH_FILE;
					}
				}
			}
		}
	}
	
	BOOL lbChanged = FALSE;
	mb_NeedInjects = FALSE;
	wchar_t szBaseDir[MAX_PATH+2]; szBaseDir[0] = 0;
	CESERVER_REQ *pIn = NULL;
	pIn = NewCmdOnCreate(aCmd, 
			asAction, asFile, asParam, 
			anShellFlags, anCreateFlags, anStartFlags, anShowCmd, 
			mn_ImageBits, mn_ImageSubsystem, 
			hIn, hOut, hErr, szBaseDir, mb_DosBoxAllowed);
	if (pIn)
	{
		HWND hConWnd = GetConsoleWindow();
		CESERVER_REQ *pOut = NULL;
		pOut = ExecuteGuiCmd(hConWnd, pIn, hConWnd);
		ExecuteFreeResult(pIn); pIn = NULL;
		if (!pOut)
			return FALSE;
		if (!pOut->OnCreateProcRet.bContinue)
			goto wrap;
		ExecuteFreeResult(pOut);
	}

	//wchar_t* pszExecFile = (wchar_t*)pOut->OnCreateProcRet.wsValue;
	//wchar_t* pszBaseDir = (wchar_t*)(pOut->OnCreateProcRet.wsValue); // + pOut->OnCreateProcRet.nFileLen);
	
	
	if (aCmd == eShellExecute)
	{
		WARNING("Уточнить условие для флагов ShellExecute!");
		// !!! anFlags может быть NULL;
		DWORD nFlags = anShellFlags ? *anShellFlags : 0;
		if ((nFlags & (SEE_MASK_FLAG_NO_UI|SEE_MASK_NOASYNC|SEE_MASK_NOCLOSEPROCESS|SEE_MASK_NO_CONSOLE))
	        != (SEE_MASK_FLAG_NO_UI|SEE_MASK_NOASYNC|SEE_MASK_NOCLOSEPROCESS|SEE_MASK_NO_CONSOLE))
			goto wrap; // пока так - это фар выполняет консольную команду
		if (asAction && (lstrcmpiW(asAction, L"open") != 0))
			goto wrap; // runas, print, и прочая нас не интересует
	}
	else
	{
		// Посмотреть, какие еще условия нужно отсеять для CreateProcess?
		DWORD nFlags = anCreateFlags ? *anCreateFlags : 0;
		if ((nFlags & (CREATE_NO_WINDOW|DETACHED_PROCESS)) != 0)
			goto wrap; // запускается по тихому (без консольного окна), пропускаем

		// Это УЖЕ может быть ConEmuC.exe
		const wchar_t* pszExeName = PointToName(szExe);
		if (pszExeName && (!lstrcmpi(pszExeName, L"ConEmuC.exe") || !lstrcmpi(pszExeName, L"ConEmuC64.exe")))
		{
			mb_NeedInjects = FALSE;
			goto wrap;
		}
	}
	
	//bool lbGuiApp = false;
	//DWORD ImageSubsystem = 0, ImageBits = 0;

	//if (!pszExecFile || !*pszExecFile)
	if (szExe[0] == 0)
	{
		_ASSERTE(szExe[0] != 0);
		goto wrap; // ошибка?
	}
	//if (GetImageSubsystem(pszExecFile,ImageSubsystem,ImageBits))
	//lbGuiApp = (ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI);

	if (lbGuiApp)
		goto wrap; // гуй - не перехватывать

	// Подставлять ConEmuC.exe нужно только для того, чтобы в фаре 
	// включить длинный буфер и запомнить длинный результат в консоли
	// 110223 - заменять нужно и ShellExecute в любом случае, т.к. не факт,
	//          что этот ShellExecute вызовет CreateProcess из kernel32.
	//          В Win7 это может быть вызов других системных модулей (App-.....dll)
	if (gFarMode.bFarHookMode || (aCmd == eShellExecute))
	{
		lbChanged = ChangeExecuteParms(aCmd, asFile, asParam, szBaseDir, 
						szExe, mn_ImageBits, mn_ImageSubsystem, psFile, psParam);
		if (!lbChanged)
		{
			mb_NeedInjects = TRUE;
		}
		else
		{
			CESERVER_REQ *pIn = NULL;
			pIn = NewCmdOnCreate(eParmsChanged, 
					asAction, *psFile, *psParam, 
					anShellFlags, anCreateFlags, anStartFlags, anShowCmd, 
					mn_ImageBits, mn_ImageSubsystem, 
					hIn, hOut, hErr, szBaseDir, mb_DosBoxAllowed);
			if (pIn)
			{
				HWND hConWnd = GetConsoleWindow();
				CESERVER_REQ *pOut = NULL;
				pOut = ExecuteGuiCmd(hConWnd, pIn, hConWnd);
				ExecuteFreeResult(pIn); pIn = NULL;
				if (pOut)
					ExecuteFreeResult(pOut);
			}
		}
	}
	else 
	if ((mn_ImageBits == 16 ) && (mn_ImageSubsystem == IMAGE_SUBSYSTEM_DOS_EXECUTABLE))
	{
		TODO("DosBox?");
		mb_NeedInjects = FALSE; // пока?
	}
	else
	{
		//lbChanged = ChangeExecuteParms(aCmd, asFile, asParam, pszBaseDir, 
		//				szExe, mn_ImageBits, mn_ImageSubsystem, psFile, psParam);
		mb_NeedInjects = TRUE;
	}

wrap:
	return lbChanged;
}

BOOL CShellProc::OnShellExecuteA(LPCSTR* asAction, LPCSTR* asFile, LPCSTR* asParam, DWORD* anFlags, DWORD* anShowCmd)
{
	if (!ghConEmuWndDC || !IsWindow(ghConEmuWndDC))
		return FALSE; // Перехватывать только под ConEmu
		
	mb_InShellExecuteEx = TRUE;
	mn_InShellExecuteEx ++;

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
	if (!ghConEmuWndDC || !IsWindow(ghConEmuWndDC))
		return FALSE; // Перехватывать только под ConEmu
	
	mb_InShellExecuteEx = TRUE;
	mn_InShellExecuteEx ++;

	BOOL lbRc = PrepareExecuteParms(eShellExecute,
					*asAction, *asFile, *asParam,
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
BOOL CShellProc::OnShellExecuteExA(LPSHELLEXECUTEINFOA* lpExecInfo)
{
	if (!ghConEmuWndDC || !IsWindow(ghConEmuWndDC) || !lpExecInfo)
		return FALSE; // Перехватывать только под ConEmu

	mlp_SaveExecInfoA = *lpExecInfo;
	mlp_ExecInfoA = (LPSHELLEXECUTEINFOA)malloc((*lpExecInfo)->cbSize);
	if (!mlp_ExecInfoA)
	{
		_ASSERTE(mlp_ExecInfoA!=NULL);
		return FALSE;
	}
	memmove(mlp_ExecInfoA, (*lpExecInfo), (*lpExecInfo)->cbSize);
	
	if (gFarMode.bFarHookMode && gFarMode.bShellNoZoneCheck)
	{
		OSVERSIONINFOEX osv = {sizeof(OSVERSIONINFOEX)};
		if (GetVersionEx((LPOSVERSIONINFO)&osv))
		{
			if ((osv.dwMajorVersion >= 6)
				|| (osv.dwMajorVersion == 5 
					&& (osv.dwMinorVersion > 1 
						|| (osv.dwMinorVersion == 1 && osv.wServicePackMajor >= 1))))
			{
				mlp_ExecInfoA->fMask |= SEE_MASK_NOZONECHECKS;
			}
		}
	}

	BOOL lbRc = OnShellExecuteA(&mlp_ExecInfoA->lpVerb, &mlp_ExecInfoA->lpFile, &mlp_ExecInfoA->lpParameters, &mlp_ExecInfoA->fMask, (DWORD*)&mlp_ExecInfoA->nShow);
	if (lbRc)
		*lpExecInfo = mlp_ExecInfoA;
	return lbRc;
}
BOOL CShellProc::OnShellExecuteExW(LPSHELLEXECUTEINFOW* lpExecInfo)
{
	if (!ghConEmuWndDC || !IsWindow(ghConEmuWndDC) || !lpExecInfo)
		return FALSE; // Перехватывать только под ConEmu

	mlp_SaveExecInfoW = *lpExecInfo;
	mlp_ExecInfoW = (LPSHELLEXECUTEINFOW)malloc((*lpExecInfo)->cbSize);
	if (!mlp_ExecInfoW)
	{
		_ASSERTE(mlp_ExecInfoW!=NULL);
		return FALSE;
	}
	memmove(mlp_ExecInfoW, (*lpExecInfo), (*lpExecInfo)->cbSize);
	
	if (gFarMode.bFarHookMode && gFarMode.bShellNoZoneCheck)
		mlp_ExecInfoW->fMask |= SEE_MASK_NOZONECHECKS;

	BOOL lbRc = OnShellExecuteW(&mlp_ExecInfoW->lpVerb, &mlp_ExecInfoW->lpFile, &mlp_ExecInfoW->lpParameters, &mlp_ExecInfoW->fMask, (DWORD*)&mlp_ExecInfoW->nShow);
	if (lbRc)
		*lpExecInfo = mlp_ExecInfoW;
	return lbRc;
}
BOOL CShellProc::OnCreateProcessA(LPCSTR* asFile, LPCSTR* asCmdLine, DWORD* anCreationFlags, LPSTARTUPINFOA lpSI)
{
	if (!ghConEmuWndDC || !IsWindow(ghConEmuWndDC))
		return FALSE; // Перехватывать только под ConEmu

	mpwsz_TempFile = str2wcs(asFile ? *asFile : NULL, mn_CP);
	mpwsz_TempParam = str2wcs(asCmdLine ? *asCmdLine : NULL, mn_CP);
	DWORD nShowCmd = lpSI->wShowWindow;
	mb_WasSuspended = ((*anCreationFlags) & CREATE_SUSPENDED) == CREATE_SUSPENDED;

	BOOL lbRc = PrepareExecuteParms(eCreateProcess,
					NULL, mpwsz_TempFile, mpwsz_TempParam,
					NULL, anCreationFlags, &lpSI->dwFlags, &nShowCmd,
					&lpSI->hStdInput, &lpSI->hStdOutput, &lpSI->hStdError,
					&mpwsz_TempRetFile, &mpwsz_TempRetParam);
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
		if (mb_NeedInjects)
			(*anCreationFlags) |= CREATE_SUSPENDED;
	}
	
	return lbRc;
}
BOOL CShellProc::OnCreateProcessW(LPCWSTR* asFile, LPCWSTR* asCmdLine, DWORD* anCreationFlags, LPSTARTUPINFOW lpSI)
{
	if (!ghConEmuWndDC || !IsWindow(ghConEmuWndDC))
		return FALSE; // Перехватывать только под ConEmu
		
	DWORD nShowCmd = lpSI->wShowWindow;
	mb_WasSuspended = ((*anCreationFlags) & CREATE_SUSPENDED) == CREATE_SUSPENDED;

	BOOL lbRc = PrepareExecuteParms(eCreateProcess,
					NULL, *asFile, *asCmdLine,
					NULL, anCreationFlags, &lpSI->dwFlags, &nShowCmd,
					&lpSI->hStdInput, &lpSI->hStdOutput, &lpSI->hStdError,
					&mpwsz_TempRetFile, &mpwsz_TempRetParam);
	if (lbRc)
	{
		if (lpSI->wShowWindow != nShowCmd)
			lpSI->wShowWindow = (WORD)nShowCmd;
		*asFile = mpwsz_TempRetFile;
		*asCmdLine = mpwsz_TempRetParam;
		if (mb_NeedInjects)
			(*anCreationFlags) |= CREATE_SUSPENDED;
	}
	
	return lbRc;
}

void CShellProc::OnCreateProcessFinished(BOOL abSucceeded, PROCESS_INFORMATION *lpPI)
{
	if (abSucceeded)
	{
		if (mb_NeedInjects)
		{
			int iHookRc = InjectHooks(*lpPI, FALSE);

			if (iHookRc != 0)
			{
				DWORD nErrCode = GetLastError();
				_ASSERTE(iHookRc == 0);
				wchar_t szDbgMsg[255], szTitle[128];
				_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuC, PID=%u", GetCurrentProcessId());
				_wsprintf(szDbgMsg, SKIPLEN(countof(szDbgMsg)) L"ConEmuC.W, PID=%u\nInjecting hooks into PID=%u\nFAILED, code=%i:0x%08X",
					GetCurrentProcessId(), lpPI->dwProcessId, iHookRc, nErrCode);
				MessageBoxW(NULL, szDbgMsg, szTitle, MB_SYSTEMMODAL);
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
		if (mn_InShellExecuteEx > 0)
			mn_InShellExecuteEx--;
		mb_InShellExecuteEx = FALSE;
	}
}
