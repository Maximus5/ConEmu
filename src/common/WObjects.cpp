
/*
Copyright (c) 2009-2015 Maximus5
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

//#ifdef _DEBUG
//#define USE_LOCK_SECTION
//#endif

#define HIDE_USE_EXCEPTION_INFO
#include <windows.h>
#include <TlHelp32.h>
#include "CmdLine.h"
#include "HkFunc.h"
#include "WObjects.h"

#if !defined(CONEMU_MINIMAL)
#include "WFiles.h"
#endif

#ifdef _DEBUG
	//#define WARN_NEED_CMD
	#undef WARN_NEED_CMD
	//#define SHOW_COMSPEC_CHANGE
	#undef SHOW_COMSPEC_CHANGE
#else
	#undef WARN_NEED_CMD
	#undef SHOW_COMSPEC_CHANGE
#endif


void getWindowInfo(HWND ahWnd, wchar_t (&rsInfo)[1024], bool bProcessName /*= false*/, LPDWORD pnPID /*= NULL*/)
{
	DWORD nPID = 0;

	if (!ahWnd)
	{
		wcscpy_c(rsInfo, L"<NULL>");
	}
	else if (!IsWindow(ahWnd))
	{
		msprintf(rsInfo, countof(rsInfo), L"0x%08X: Invalid window handle", LODWORD(ahWnd));
	}
	else
	{
		wchar_t szClass[256], szTitle[512];
		wchar_t szProc[120] = L"";

		if (!GetClassName(ahWnd, szClass, 256)) wcscpy_c(szClass, L"<GetClassName failed>");
		if (!GetWindowText(ahWnd, szTitle, 512)) szTitle[0] = 0;

		if (bProcessName || pnPID)
		{
			if (GetWindowThreadProcessId(ahWnd, &nPID))
			{
				PROCESSENTRY32 pi = {};
				if (bProcessName && GetProcessInfo(nPID, &pi))
				{
					pi.szExeFile[100] = 0;
					msprintf(szProc, countof(szProc), L" - %s [%u]", pi.szExeFile, nPID);
				}
			}
		}

		msprintf(rsInfo, countof(rsInfo), L"0x%08X: %s - '%s'%s", LODWORD(ahWnd), szClass, szTitle, szProc);
	}

	if (pnPID)
		*pnPID = nPID;
}

// pnSize заполняется только в том случае, если файл найден
bool FileExists(LPCWSTR asFilePath, DWORD* pnSize /*= NULL*/)
{
	if (!asFilePath || !*asFilePath)
		return false;

	_ASSERTE(wcschr(asFilePath, L'\t')==NULL);

	WIN32_FIND_DATAW fnd = {0};
	HANDLE hFind = FindFirstFile(asFilePath, &fnd);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		bool lbFileFound = false;

		// FindFirstFile может обломаться из-за симлинков
		DWORD nErrCode = GetLastError();
		if (nErrCode == ERROR_ACCESS_DENIED)
		{
			hFind = CreateFile(asFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

			if (hFind && hFind != INVALID_HANDLE_VALUE)
			{
				BY_HANDLE_FILE_INFORMATION fi = {0};

				if (GetFileInformationByHandle(hFind, &fi) && !(fi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					lbFileFound = true;

					if (pnSize)
						*pnSize = fi.nFileSizeHigh ? 0xFFFFFFFF : fi.nFileSizeLow; //-V112
				}
			}

			CloseHandle(hFind);
		}

		return lbFileFound;
	}

	bool lbFound = false;

	do
	{
		if ((fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			lbFound = true;

			if (pnSize)
				*pnSize = fnd.nFileSizeHigh ? 0xFFFFFFFF : fnd.nFileSizeLow; //-V112

			break;
		}
	}
	while (FindNextFile(hFind, &fnd));

	FindClose(hFind);
	return lbFound;
}

bool FileSearchInDir(LPCWSTR asFilePath, CmdArg& rsFound)
{
	// Possibilities
	// a) asFilePath does not contain path, only: "far"
	// b) asFilePath contains path, but without extension: "C:\\Program Files\\Far\\Far"

	LPCWSTR pszSearchFile = asFilePath;
	LPCWSTR pszSlash = wcsrchr(asFilePath, L'\\');
	if (pszSlash)
	{
		if ((pszSlash - asFilePath) >= MAX_PATH)
		{
			// No need to continue, this is invalid path to executable
			return false;
		}

		// C:\Far\Far.exe /w /pC:\Far\Plugins\ConEmu;C:\Far\Plugins.My
		if (!IsFilePath(asFilePath, false))
		{
			return false;
		}

		// Do not allow '/' here
		LPCWSTR pszFwd = wcschr(asFilePath, L'/');
		if (pszFwd != NULL)
		{
			return false;
		}
	}

	wchar_t* pszSearchPath = NULL;
	if (pszSlash)
	{
		if ((pszSearchPath = lstrdup(asFilePath)) != NULL)
		{
			pszSearchFile = pszSlash + 1;
			pszSearchPath[pszSearchFile - asFilePath] = 0;
		}
	}

	// Попытаемся найти "по путям" (%PATH%)
	wchar_t *pszFilePart;
	wchar_t szFind[MAX_PATH+1];
	LPCWSTR pszExt = PointToExt(asFilePath);

	DWORD nLen = SearchPath(pszSearchPath, pszSearchFile, pszExt ? NULL : L".exe", countof(szFind), szFind, &pszFilePart);

	SafeFree(pszSearchPath);

	if (nLen && (nLen < countof(szFind)) && FileExists(szFind))
	{
		// asFilePath will be invalid after .Set
		rsFound.Set(szFind);
		return true;
	}

	return false;
}

SearchAppPaths_t gfnSearchAppPaths = NULL;

bool FileExistsSearch(LPCWSTR asFilePath, CmdArg& rsFound, bool abSetPath/*= true*/, bool abRegSearch /*= true*/)
{
	if (!asFilePath || !*asFilePath)
	{
		_ASSERTEX(asFilePath && *asFilePath);
		return false;
	}

	if (FileExists(asFilePath))
	{
		return true;
	}

	// Развернуть переменные окружения
	if (wcschr(asFilePath, L'%'))
	{
		bool bFound = false;
		wchar_t* pszExpand = ExpandEnvStr(asFilePath);
		if (pszExpand && FileExists(pszExpand))
		{
			// asFilePath will be invalid after .Set
			rsFound.Set(pszExpand);
			bFound = true;
		}
		SafeFree(pszExpand);

		if (bFound)
		{
			return true;
		}
	}

	// Search "Path"
	if (FileSearchInDir(asFilePath, rsFound))
	{
		return true;
	}

	// Только если приложение не нашли "по путям" - пытаемся определить его расположение через [App Paths]
	// В противном случае, в частности, может быть запущен "far" не из папки с ConEmu, а зарегистрированный
	// в реестре, если на машине их несколько установлено.

	#if !defined(CONEMU_MINIMAL)
	_ASSERTE(gfnSearchAppPaths!=NULL);
	#endif

	// В ConEmuHk этот блок не активен, потому что может быть "только" перехват CreateProcess,
	// а о его параметрах должно заботиться вызывающее (текущее) приложение
	if (abRegSearch && gfnSearchAppPaths && !wcschr(asFilePath, L'\\'))
	{
		// Если в asFilePath НЕ указан путь - искать приложение в реестре,
		// и там могут быть указаны доп. параметры (пока только добавка в %PATH%)
		if (gfnSearchAppPaths(asFilePath, rsFound, abSetPath, NULL))
		{
			// Нашли по реестру, возможно обновили %PATH%
			return true;
		}
	}

	return false;
}

bool GetShortFileName(LPCWSTR asFullPath, int cchShortNameMax, wchar_t* rsShortName/*[MAX_PATH+1]-name only*/, BOOL abFavorLength/*=FALSE*/)
{
	WARNING("FindFirstFile использовать нельзя из-за симлинков");
	WIN32_FIND_DATAW fnd; memset(&fnd, 0, sizeof(fnd));
	HANDLE hFind = FindFirstFile(asFullPath, &fnd);

	if (hFind == INVALID_HANDLE_VALUE)
		return false;

	FindClose(hFind);

	if (fnd.cAlternateFileName[0])
	{
		if ((abFavorLength && (lstrlenW(fnd.cAlternateFileName) < lstrlenW(fnd.cFileName))) //-V303
		        || (wcschr(fnd.cFileName, L' ') != NULL))
		{
			if (lstrlen(fnd.cAlternateFileName) >= cchShortNameMax) //-V303
				return false;
			_wcscpy_c(rsShortName, cchShortNameMax, fnd.cAlternateFileName); //-V106
			return TRUE;
		}
	}
	else if (wcschr(fnd.cFileName, L' ') != NULL)
	{
		return false;
	}

	if (lstrlen(fnd.cFileName) >= cchShortNameMax) //-V303
		return false;

	_wcscpy_c(rsShortName, cchShortNameMax, fnd.cFileName); //-V106

	return true;
}

wchar_t* GetShortFileNameEx(LPCWSTR asLong, BOOL abFavorLength/*=TRUE*/)
{
	if (!asLong)
		return NULL;

	int nSrcLen = lstrlenW(asLong); //-V303
	wchar_t* pszLong = lstrdup(asLong);

	int nMaxLen = nSrcLen + MAX_PATH; // "короткое" имя может более MAX_PATH
	wchar_t* pszShort = (wchar_t*)calloc(nMaxLen, sizeof(wchar_t)); //-V106

	wchar_t* pszResult = NULL;
	wchar_t* pszSrc = pszLong;
	//wchar_t* pszDst = pszShort;
	wchar_t* pszSlash;
	wchar_t* szName = (wchar_t*)malloc((MAX_PATH+1)*sizeof(wchar_t));
	bool     lbNetwork = false;
	int      nLen, nCurLen = 0;

	if (!pszLong || !pszShort || !szName)
		goto wrap;

	// Если путь сетевой (или UNC?) пропустить префиксы/серверы
	if (pszSrc[0] == L'\\' && pszSrc[1] == '\\')
	{
		// пропуск первых двух слешей
		pszSrc += 2;
		// формат "диска" не поддерживаем \\.\Drive\...
		if (pszSrc[0] == L'.' && pszSrc[1] == L'\\')
			goto wrap;
		// UNC
		if (pszSrc[0] == L'?' && pszSrc[1] == L'\\')
		{
			pszSrc += 2;
			if (pszSrc[0] == L'U' && pszSrc[1] == L'N' && pszSrc[2] == L'C' && pszSrc[3] == L'\\')
			{
				// UNC\Server\share\...
				pszSrc += 4; //-V112
				lbNetwork = true;
			}
			// иначе - ожидается диск
		}
		// Network (\\Server\\Share\...)
		else
		{
			lbNetwork = true;
		}
	}

	if (pszSrc[0] == 0)
		goto wrap;

	if (lbNetwork)
	{
		pszSlash = wcschr(pszSrc, L'\\');
		if (!pszSlash)
			goto wrap;
		pszSlash = wcschr(pszSlash+1, L'\\');
		if (!pszSlash)
			goto wrap;
		pszShort[0] = L'\\'; pszShort[1] = L'\\'; pszShort[2] = 0;
		_wcscatn_c(pszShort, nMaxLen, pszSrc, (pszSlash-pszSrc+1)); // память выделяется calloc! //-V303 //-V104
	}
	else
	{
		// <Drive>:\path...
		if (pszSrc[1] != L':')
			goto wrap;
		if (pszSrc[2] != L'\\' && pszSrc[2] != 0)
			goto wrap;
		pszSlash = pszSrc + 2;
		_wcscatn_c(pszShort, nMaxLen, pszSrc, (pszSlash-pszSrc+1)); // память выделяется calloc!
	}

	nCurLen = lstrlenW(pszShort);

	while (pszSlash && (*pszSlash == L'\\'))
	{
		pszSrc = pszSlash;
		pszSlash = wcschr(pszSrc+1, L'\\');
		if (pszSlash)
			*pszSlash = 0;

		if (!GetShortFileName(pszLong, MAX_PATH+1, szName, abFavorLength))
			goto wrap;
		nLen = lstrlenW(szName);
		if ((nLen + nCurLen) >= nMaxLen)
			goto wrap;
		//pszShort[nCurLen++] = L'\\'; pszShort[nCurLen] = 0;
		_wcscpyn_c(pszShort+nCurLen, nMaxLen-nCurLen, szName, nLen);
		nCurLen += nLen;

		if (pszSlash)
		{
			*pszSlash = L'\\';
			pszShort[nCurLen++] = L'\\'; // память выделяется calloc!
		}
	}

	nLen = lstrlenW(pszShort);

	if ((nLen > 0) && (pszShort[nLen-1] == L'\\'))
		pszShort[--nLen] = 0;

	if (nLen <= MAX_PATH)
	{
		pszResult = pszShort;
		pszShort = NULL;
		goto wrap;
	}

wrap:
	if (szName)
		free(szName);
	if (pszShort)
		free(pszShort);
	if (pszLong)
		free(pszLong);
	return pszResult;
}

//wchar_t* GetShortFileNameEx(LPCWSTR asLong, BOOL abFavorLength=FALSE)
//{
//	TODO("хорошо бы и сетевые диски обрабатывать");
//
//	if (!asLong) return NULL;
//
//	int nMaxLen = lstrlenW(asLong) + MAX_PATH; // "короткое" имя может оказаться длиннее "длинного"
//	wchar_t* pszShort = /*lstrdup(asLong);*/(wchar_t*)malloc(nMaxLen*2);
//	_wcscpy_c(pszShort, nMaxLen, asLong);
//	wchar_t* pszCur = wcschr(pszShort+3, L'\\');
//	wchar_t* pszSlash;
//	wchar_t  szName[MAX_PATH+1];
//	size_t nLen = 0;
//
//	while(pszCur)
//	{
//		*pszCur = 0;
//		{
//			if (GetShortFileName(pszShort, szName, abFavorLength))
//			{
//				if ((pszSlash = wcsrchr(pszShort, L'\\'))==0)
//					goto wrap;
//
//				_wcscpy_c(pszSlash+1, nMaxLen-(pszSlash-pszShort+1), szName);
//				pszSlash += 1+lstrlenW(szName);
//				_wcscpy_c(pszSlash+1, nMaxLen-(pszSlash-pszShort+1), pszCur+1);
//				pszCur = pszSlash;
//			}
//		}
//		*pszCur = L'\\';
//		pszCur = wcschr(pszCur+1, L'\\');
//	}
//
//	nLen = lstrlenW(pszShort);
//
//	if (nLen>0 && pszShort[nLen-1]==L'\\')
//		pszShort[--nLen] = 0;
//
//	if (nLen <= MAX_PATH)
//		return pszShort;
//
//wrap:
//	free(pszShort);
//	return NULL;
//}

// Used in IsWine and IsWinPE
// The function CheckRegKeyExist is using dynamic linking!
struct RegKeyExistArg
{
	HKEY hkParent;
	LPCWSTR pszKey;
};
static bool CheckRegKeyExist(RegKeyExistArg* pKeys)
{
	bool bFound = false;

	typedef LONG (WINAPI* RegOpenKeyExW_t)(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
	typedef LONG (WINAPI* RegCloseKey_t)(HKEY hKey);

	HMODULE hAdvApi = LoadLibrary(L"AdvApi32.dll");
	if (hAdvApi)
	{
		RegOpenKeyExW_t _RegOpenKeyExW = (RegOpenKeyExW_t)GetProcAddress(hAdvApi, "RegOpenKeyExW");
		RegCloseKey_t _RegCloseKey = (RegCloseKey_t)GetProcAddress(hAdvApi, "RegCloseKey");

		while (pKeys->hkParent)
		{
			HKEY hk = NULL;
			LONG lRc = _RegOpenKeyExW(pKeys->hkParent, pKeys->pszKey, 0, KEY_READ, &hk);
			if (hk)
				_RegCloseKey(hk);
			if (lRc == 0)
			{
				bFound = true;
				break;
			}
			pKeys++;
		}

        FreeLibrary(hAdvApi);
	}

	return bFound;
}

// Function is used for patching some bugs of Wine emulator
// Used in: MyGetLargestConsoleWindowSize, OnGetLargestConsoleWindowSize
bool IsWine()
{
#ifdef _DEBUG
//	return true;
#endif
	static int ibIsWine = 0;
	if (!ibIsWine)
	{
		RegKeyExistArg Keys[] = {{HKEY_CURRENT_USER, L"Software\\Wine"}, {HKEY_LOCAL_MACHINE, L"Software\\Wine"}, {NULL}};
		ibIsWine = CheckRegKeyExist(Keys) ? 1 : -1;
	}
	// В общем случае, на флажок ориентироваться нельзя. Это для информации.
	return (ibIsWine == 1);
}

bool IsWinPE()
{
	static int ibIsWinPE = 0;
	if (!ibIsWinPE)
	{
		RegKeyExistArg Keys[] = {
			{HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\MiniNT"},
			{HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\PE Builder"},
			{NULL}};
		ibIsWinPE = CheckRegKeyExist(Keys) ? 1 : -1;
	}
	// В общем случае, на флажок ориентироваться нельзя. Это для информации.
	return (ibIsWinPE == 1);
}

bool GetOsVersionInformational(OSVERSIONINFO* pOsVer)
{
	#pragma warning(disable: 4996)
	BOOL result = GetVersionEx(pOsVer);
	#pragma warning(default: 4996)
	return (result != FALSE);
}

bool IsWinVerOrHigher(WORD OsVer)
{
	OSVERSIONINFOEXW osvi = {sizeof(osvi), HIBYTE(OsVer), LOBYTE(OsVer)};
	DWORDLONG const dwlConditionMask = VerSetConditionMask(VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_MINORVERSION, VER_GREATER_EQUAL);
	BOOL ibIsWinOrHigher = VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask);
	return (ibIsWinOrHigher != FALSE);
}

// Only Windows 2000
bool IsWin2kEql()
{
	static int ibIsWin2K = 0;
	if (!ibIsWin2K)
	{
		OSVERSIONINFOEXW osvi = {sizeof(osvi), HIBYTE(_WIN32_WINNT_WIN2K), LOBYTE(_WIN32_WINNT_WIN2K)};
		DWORDLONG const dwlConditionMask = VerSetConditionMask(VerSetConditionMask(0,
			VER_MAJORVERSION, VER_EQUAL),
			VER_MINORVERSION, VER_EQUAL);
		ibIsWin2K = VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR, dwlConditionMask) ? 1 : -1;;
	}
	return (ibIsWin2K == 1);
}

// Only 5.x family (Win2k, WinXP, Win 2003 server)
bool IsWin5family()
{
	static int ibIsWin5fam = 0;
	if (!ibIsWin5fam)
	{
		OSVERSIONINFOEXW osvi = {sizeof(osvi), 5, 0};
		DWORDLONG const dwlConditionMask = VerSetConditionMask(0, VER_MAJORVERSION, VER_EQUAL);
		ibIsWin5fam = VerifyVersionInfoW(&osvi, VER_MAJORVERSION, dwlConditionMask) ? 1 : -1;
	}
	return (ibIsWin5fam == 1);
}

// WinXP SP1 or higher
bool IsWinXPSP1()
{
	static int ibIsWinXPSP1 = 0;
	if (!ibIsWinXPSP1)
	{
		OSVERSIONINFOEXW osvi = {sizeof(osvi), HIBYTE(_WIN32_WINNT_WINXP), LOBYTE(_WIN32_WINNT_WINXP)};
		osvi.wServicePackMajor = 1;
		DWORDLONG const dwlConditionMask = VerSetConditionMask(VerSetConditionMask(VerSetConditionMask(0,
			VER_MAJORVERSION, VER_GREATER_EQUAL),
			VER_MINORVERSION, VER_GREATER_EQUAL),
			VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
		ibIsWinXPSP1 = VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR, dwlConditionMask) ? 1 : -1;;
	}
	return (ibIsWinXPSP1 == 1);
}

// Vista and higher
bool IsWin6()
{
	static int ibIsWin6 = 0;
	if (!ibIsWin6)
	{
		_ASSERTE(_WIN32_WINNT_WIN6 == 0x600);
		ibIsWin6 = IsWinVerOrHigher(_WIN32_WINNT_WIN6) ? 1 : -1;
	}
	return (ibIsWin6 == 1);
}

// Windows 7 and higher
bool IsWin7()
{
	static int ibIsWin7 = 0;
	if (!ibIsWin7)
	{
		_ASSERTE(_WIN32_WINNT_WIN7 == 0x601);
		ibIsWin7 = IsWinVerOrHigher(_WIN32_WINNT_WIN7) ? 1 : -1;
	}
	return (ibIsWin7 == 1);
}

// Windows 10 and higher
bool IsWin10()
{
	static int ibIsWin10 = 0;
	if (!ibIsWin10)
	{
		#define _WIN32_WINNT_WIN10 0x604
		ibIsWin10 = IsWinVerOrHigher(_WIN32_WINNT_WIN10) ? 1 : -1;
	}
	return (ibIsWin10 == 1);
}

bool IsDbcs()
{
	bool bIsDBCS = (GetSystemMetrics(SM_DBCSENABLED) != 0);
	return bIsDBCS;
}

// Проверить битность OS
bool IsWindows64()
{
	static bool is64bitOs = false, isOsChecked = false;
	if (isOsChecked)
		return is64bitOs;

	bool isWow64process = false;

#ifdef WIN64
	is64bitOs = true; isWow64process = true;
#else
	// Проверяем, где мы запущены
	isWow64process = FALSE;
	HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");

	if (hKernel)
	{
		typedef BOOL (WINAPI* IsWow64Process_t)(HANDLE hProcess, PBOOL Wow64Process);
		IsWow64Process_t IsWow64Process_f = (IsWow64Process_t)GetProcAddress(hKernel, "IsWow64Process");

		if (IsWow64Process_f)
		{
			BOOL bWow64 = FALSE;

			if (IsWow64Process_f(GetCurrentProcess(), &bWow64) && bWow64)
			{
				isWow64process = true;
			}
		}
	}

	is64bitOs = isWow64process;
#endif

	isOsChecked = true;
	return is64bitOs;
}

bool IsHwFullScreenAvailable()
{
	if (IsWindows64())
		return false;

	// HW FullScreen was available in Win2k & WinXP (32bit)
	_ASSERTE(_WIN32_WINNT_VISTA==0x600);
	OSVERSIONINFOEXW osvi = {sizeof(osvi), HIBYTE(_WIN32_WINNT_VISTA), LOBYTE(_WIN32_WINNT_VISTA)};
	DWORDLONG const dwlConditionMask = VerSetConditionMask(VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_MINORVERSION, VER_GREATER_EQUAL);
	if (VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask))
		return false; // Vista or higher - not available
	else
		return true;
}

bool IsVsNetHostExe(LPCWSTR asFilePatName)
{
	bool bVsNetHostRequested = false;
	int iNameLen = asFilePatName ? lstrlen(asFilePatName) : 0;
	LPCWSTR pszVsHostSuffix = L".vshost.exe";
	int iVsHostSuffix = lstrlen(pszVsHostSuffix);
	if ((iNameLen >= iVsHostSuffix) && (lstrcmpi(asFilePatName+iNameLen-iVsHostSuffix, pszVsHostSuffix) == 0))
	{
		bVsNetHostRequested = true;
	}
	return bVsNetHostRequested;
}

bool IsGDB(LPCWSTR asFilePatName)
{
	// "gdb.exe"
	LPCWSTR pszName = PointToName(asFilePatName);
	bool bIsGdb = pszName ? (lstrcmpi(pszName, L"gdb.exe") == 0) : false;
	return bIsGdb;
}

// Check running process bits - 32/64
int GetProcessBits(DWORD nPID, HANDLE hProcess /*= NULL*/)
{
	if (!IsWindows64())
		return 32;

	int ImageBits = WIN3264TEST(32,64); //-V112

	typedef BOOL (WINAPI* IsWow64Process_t)(HANDLE, PBOOL);
	static IsWow64Process_t IsWow64Process_f = NULL;

	if (!IsWow64Process_f)
	{
		HMODULE hKernel = GetModuleHandle(L"kernel32.dll");
		if (hKernel)
		{
			IsWow64Process_f = (IsWow64Process_t)GetProcAddress(hKernel, "IsWow64Process");
		}
	}

	if (IsWow64Process_f)
	{
		BOOL bWow64 = FALSE;
		HANDLE h = hProcess ? hProcess : hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, nPID);

		if (h == NULL)
		{
			// If it is blocked due to access rights - try to find alternative ways (by path or PERF COUNTER)
			ImageBits = 0;
		}
		else if (IsWow64Process_f(h, &bWow64) && !bWow64)
		{
			ImageBits = 64;
		}
		else
		{
			ImageBits = 32;
		}

		if (h && (h != hProcess))
			CloseHandle(h);
	}

	return ImageBits;
}

bool GetProcessInfo(DWORD nPID, PROCESSENTRY32W* Info)
{
	bool bFound = false;
	if (Info)
		memset(Info, 0, sizeof(*Info));
	HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (h && (h != INVALID_HANDLE_VALUE))
	{
		PROCESSENTRY32 PI = {sizeof(PI)};
		if (Process32First(h, &PI))
		{
			do {
				if (PI.th32ProcessID == nPID)
				{
					if (Info)
						*Info = PI;
					bFound = true;
					break;
				}
			} while (Process32Next(h, &PI));
		}

		CloseHandle(h);
	}
	return bFound;
}

bool isTerminalMode()
{
	static bool TerminalMode = false, TerminalChecked = false;

	if (!TerminalChecked)
	{
		// -- переменная "TERM" может быть задана пользователем
		// -- для каких-то специальных целей, полагаться на нее нельзя
		//TCHAR szVarValue[64];
		//szVarValue[0] = 0;
		//if (GetEnvironmentVariable(_T("TERM"), szVarValue, 63) && szVarValue[0])
		//{
		//	TerminalMode = true;
		//}
		//TerminalChecked = true;

		PROCESSENTRY32  P = {sizeof(PROCESSENTRY32)};
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnap == INVALID_HANDLE_VALUE)
		{
			// будем считать, что не в telnet :)
		}
		else if (Process32First(hSnap, &P))
		{
			int nProcCount = 0, nProcMax = 1024;
			PROCESSENTRY32 *pProcesses = (PROCESSENTRY32*)calloc(nProcMax, sizeof(PROCESSENTRY32));
			DWORD nCurPID = GetCurrentProcessId();
			DWORD nParentPID = nCurPID;
			// Сначала загрузить список всех процессов, чтобы потом по нему выйти не корневой
			do
			{
				if (nProcCount == nProcMax)
				{
					nProcMax += 1024;
					PROCESSENTRY32 *p = (PROCESSENTRY32*)calloc(nProcMax, sizeof(PROCESSENTRY32));
					memmove(pProcesses, p, nProcCount*sizeof(PROCESSENTRY32));
					free(pProcesses);
					pProcesses = p;
				}

				pProcesses[nProcCount] = P;
				if (P.th32ProcessID == nParentPID)
				{
					if (P.th32ProcessID != nCurPID)
					{
						if (!lstrcmpi(P.szExeFile, L"tlntsess.exe") || !lstrcmpi(P.szExeFile, L"tlntsvr.exe"))
						{
							TerminalMode = TerminalChecked = true;
							break;
						}
					}
					nParentPID = P.th32ParentProcessID;
				}
				nProcCount++;
			} while (Process32Next(hSnap, &P));
			// Snapshot больше не нужен
			CloseHandle(hSnap);

			int nSteps = 128; // защита от зацикливания
			while (!TerminalMode && (--nSteps) > 0)
			{
				for (int i = 0; i < nProcCount; i++)
				{
					if (pProcesses[i].th32ProcessID == nParentPID)
					{
						if (P.th32ProcessID != nCurPID)
						{
							if (!lstrcmpi(pProcesses[i].szExeFile, L"tlntsess.exe") || !lstrcmpi(pProcesses[i].szExeFile, L"tlntsvr.exe"))
							{
								TerminalMode = TerminalChecked = true;
								break;
							}
						}
						nParentPID = pProcesses[i].th32ParentProcessID;
						break;
					}
				}
			}

			free(pProcesses);
		}
	}

	// В повторых проверках смысла нет
	TerminalChecked = true;
	return TerminalMode;
}

// Проверить, валиден ли модуль?
bool IsModuleValid(HMODULE module, BOOL abTestVirtual /*= TRUE*/)
{
	if ((module == NULL) || (module == INVALID_HANDLE_VALUE))
		return false;
	if (LDR_IS_RESOURCE(module))
		return false;

	bool lbValid = true;
	#ifdef USE_SEH
	IMAGE_DOS_HEADER dos;
	IMAGE_NT_HEADERS nt;
	#endif

	static bool bSysInfoRetrieved = false;
	static SYSTEM_INFO si = {};
	if (!bSysInfoRetrieved)
	{
		GetSystemInfo(&si);
		bSysInfoRetrieved = true;
	}

	LPBYTE lpTest;
	SIZE_T cbCommitSize = max(max(4096,sizeof(IMAGE_DOS_HEADER)),si.dwPageSize);

	// If module is hooked by ConEmuHk, we get excess debug "assertion" from ConEmu.dll (Far plugin)
	if (abTestVirtual)
	{
		// Issue 881
		lpTest = (LPBYTE)hkFunc.virtualAlloc((LPVOID)module, cbCommitSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
		if (lpTest)
		{
			// If we can lock mem region with (IMAGE_DOS_HEADER) of checking module
			if ((lpTest <= (LPBYTE)module) && ((lpTest + cbCommitSize) >= (((LPBYTE)module) + sizeof(IMAGE_DOS_HEADER))))
			{
				// That means, it was unloaded
				lbValid = false;
			}
			VirtualFree(lpTest, 0, MEM_RELEASE);

			if (!lbValid)
				goto wrap;
		}
		else
		{
			// Memory is used (supposing by module)
		}
	}

#ifdef USE_SEH
	SAFETRY
	{
		memmove(&dos, (void*)module, sizeof(dos));
		if (dos.e_magic != IMAGE_DOS_SIGNATURE /*'ZM'*/)
		{
			lbValid = false;
		}
		else
		{
			memmove(&nt, (IMAGE_NT_HEADERS*)((char*)module + ((IMAGE_DOS_HEADER*)module)->e_lfanew), sizeof(nt));
			if (nt.Signature != 0x004550)
				lbValid = false;
		}
	}
	SAFECATCH
	{
		lbValid = false;
	}

#else
	if (IsBadReadPtr((void*)module, sizeof(IMAGE_DOS_HEADER)))
	{
		lbValid = false;
	}
	else if (((IMAGE_DOS_HEADER*)module)->e_magic != IMAGE_DOS_SIGNATURE /*'ZM'*/)
	{
		lbValid = false;
	}
	else
	{
		IMAGE_NT_HEADERS* nt_header = (IMAGE_NT_HEADERS*)((char*)module + ((IMAGE_DOS_HEADER*)module)->e_lfanew);
		if (IsBadReadPtr(nt_header, sizeof(IMAGE_NT_HEADERS)))
		{
			lbValid = false;
		}
		else if (nt_header->Signature != 0x004550)
		{
			return false;
		}
	}
#endif

wrap:
	return lbValid;
}

bool CheckCallbackPtr(HMODULE hModule, size_t ProcCount, FARPROC* CallBack, BOOL abCheckModuleInfo, BOOL abAllowNTDLL, BOOL abTestVirtual /*= TRUE*/)
{
	if ((hModule == NULL) || (hModule == INVALID_HANDLE_VALUE) || LDR_IS_RESOURCE(hModule))
	{
		_ASSERTE((hModule != NULL) && (hModule != INVALID_HANDLE_VALUE) && !LDR_IS_RESOURCE(hModule));
		return false;
	}
	if (!CallBack || !ProcCount)
	{
		_ASSERTE(CallBack && ProcCount);
		return false;
	}

	DWORD_PTR nModulePtr = (DWORD_PTR)hModule;
	DWORD_PTR nModuleSize = (4<<20);
	//BOOL lbModuleInformation = FALSE;

	DWORD_PTR nModulePtr2 = 0;
	DWORD_PTR nModuleSize2 = 0;
	if (abAllowNTDLL)
	{
		nModulePtr2 = (DWORD_PTR)GetModuleHandle(L"ntdll.dll");
		nModuleSize2 = (4<<20);
	}

	// Если разрешили - попробовать определить размер модуля, чтобы CallBack не выпал из его тела
	if (abCheckModuleInfo)
	{
		if (!IsModuleValid(hModule, abTestVirtual))
		{
			_ASSERTE("!IsModuleValid(hModule)" && 0);
			return false;
		}

		IMAGE_NT_HEADERS* nt_header = (IMAGE_NT_HEADERS*)((char*)hModule + ((IMAGE_DOS_HEADER*)hModule)->e_lfanew);

		// Получить размер модуля из OptionalHeader
		nModuleSize = nt_header->OptionalHeader.SizeOfImage;

		if (nModulePtr2)
		{
			nt_header = (IMAGE_NT_HEADERS*)((char*)nModulePtr2 + ((IMAGE_DOS_HEADER*)nModulePtr2)->e_lfanew);
			nModuleSize2 = nt_header->OptionalHeader.SizeOfImage;
		}
	}

	for (size_t i = 0; i < ProcCount; i++)
	{
		if (!(CallBack[i]))
		{
			_ASSERTE((CallBack[i])!=NULL);
			return false;
		}

		if ((((DWORD_PTR)(CallBack[i])) < nModulePtr)
			&& (!nModulePtr2 || (((DWORD_PTR)(CallBack[i])) < nModulePtr2)))
		{
			_ASSERTE(((DWORD_PTR)(CallBack[i])) >= nModulePtr);
			return false;
		}

		if ((((DWORD_PTR)(CallBack[i])) > (nModuleSize + nModulePtr))
			&& (!nModulePtr2 || (((DWORD_PTR)(CallBack[i])) > (nModuleSize2 + nModulePtr2))))
		{
			_ASSERTE(((DWORD_PTR)(CallBack[i])) <= (nModuleSize + nModulePtr));
			return false;
		}
	}

	return true;
}

wchar_t* ExpandMacroValues(LPCWSTR pszFormat, LPCWSTR* pszValues, size_t nValCount)
{
	wchar_t* pszCommand = NULL;
	size_t cchCmdMax = 0;

	// Замена %1 и т.п.
	for (int s = 0; s < 2; s++)
	{
		// На первом шаге - считаем требуемый размер под pszCommand, на втором - формируем команду
		if (s)
		{
			if (!cchCmdMax)
			{
				//ReportError(L"Invalid %s update command (%s)", (mp_Set->UpdateDownloadSetup()==1) ? L"exe" : L"arc", pszFormat, 0);
				goto wrap;
			}
			pszCommand = (wchar_t*)malloc((cchCmdMax+1)*sizeof(wchar_t));
		}

		wchar_t* pDst = pszCommand;

		for (LPCWSTR pSrc = pszFormat; *pSrc; pSrc++)
		{
			LPCWSTR pszMacro = NULL;

			switch (*pSrc)
			{
			case L'%':
				pSrc++;

				if (*pSrc == L'%')
				{
					pszMacro = L"%";
				}
				else if ((*pSrc >= L'1') && (*pSrc <= L'9'))
				{
					size_t n = (size_t)(int)(*pSrc - L'1');
					if (nValCount > n)
					{
						pszMacro = pszValues[n];
					}
				}
				else
				{
					// Недопустимый управляющий символ, это может быть переменная окружения
					pszMacro = NULL;
					pSrc--;
					if (s)
						*(pDst++) = L'%';
					else
						cchCmdMax++;
				}

				if (pszMacro)
				{
					size_t cchLen = lstrlenW(pszMacro);
					if (s)
					{
						_wcscpy_c(pDst, cchLen+1, pszMacro);
						pDst += cchLen;
					}
					else
					{
						cchCmdMax += cchLen;
					}
				}
				break; // end of '%'

			default:
				if (s)
					*(pDst++) = *pSrc;
				else
					cchCmdMax++;
			}
		}
		if (s)
			*pDst = 0;
	}

wrap:
	return pszCommand;
}

wchar_t* ExpandEnvStr(LPCWSTR pszCommand)
{
	if (!pszCommand || !*pszCommand)
		return NULL;

	DWORD cchMax = ExpandEnvironmentStrings(pszCommand, NULL, 0);
	if (!cchMax)
		return lstrdup(pszCommand);

	wchar_t* pszExpand = (wchar_t*)malloc((cchMax+2)*sizeof(*pszExpand));
	if (pszExpand)
	{
		pszExpand[0] = 0;
		pszExpand[cchMax] = 0xFFFF;
		pszExpand[cchMax+1] = 0xFFFF;

		DWORD nExp = ExpandEnvironmentStrings(pszCommand, pszExpand, cchMax);
		if (nExp && (nExp <= cchMax) && *pszExpand)
			return pszExpand;

		SafeFree(pszExpand);
	}
	return NULL;
}

wchar_t* GetEnvVar(LPCWSTR VarName)
{
	if (!VarName || !*VarName)
	{
		return NULL;
	}

	DWORD cchMax, nRc, nErr;

	nRc = GetEnvironmentVariable(VarName, NULL, 0);
	if (nRc == 0)
	{
		// Weird. This may be empty variable or not existing variable
		nErr = GetLastError();
		if (nErr == ERROR_ENVVAR_NOT_FOUND)
			return NULL;
		return (wchar_t*)calloc(3,sizeof(wchar_t));
	}

	cchMax = nRc+2;
	wchar_t* pszVal = (wchar_t*)calloc(cchMax,sizeof(*pszVal));
	if (!pszVal)
	{
		_ASSERTE((pszVal!=NULL) && "GetEnvVar memory allocation failed");
		return NULL;
	}

	nRc = GetEnvironmentVariable(VarName, pszVal, cchMax);
	if ((nRc == 0) || (nRc >= cchMax))
	{
		_ASSERTE(nRc > 0 && nRc < cchMax);
		SafeFree(pszVal);
	}

	return pszVal;
}

LPCWSTR GetComspecFromEnvVar(wchar_t* pszComspec, DWORD cchMax, ComSpecBits Bits/* = csb_SameOS*/)
{
	if (!pszComspec || (cchMax < MAX_PATH))
	{
		_ASSERTE(pszComspec && (cchMax >= MAX_PATH));
		return NULL;
	}

	*pszComspec = 0;
	BOOL bWin64 = IsWindows64();

	if (!((Bits == csb_x32) || (Bits == csb_x64)))
	{
		if (GetEnvironmentVariable(L"ComSpec", pszComspec, cchMax))
		{
			// Не должен быть (даже случайно) ConEmuC.exe
			const wchar_t* pszName = PointToName(pszComspec);
			if (!pszName || !lstrcmpi(pszName, L"ConEmuC.exe") || !lstrcmpi(pszName, L"ConEmuC64.exe")
				|| !FileExists(pszComspec)) // ну и существовать должен
			{
				pszComspec[0] = 0;
			}
		}
	}

	// Если не удалось определить через переменную окружения - пробуем обычный "cmd.exe" из System32
	if (pszComspec[0] == 0)
	{
		int n = GetWindowsDirectory(pszComspec, cchMax - 20);
		if (n > 0 && (((DWORD)n) < (cchMax - 20)))
		{
			// Добавить \System32\cmd.exe

			// Warning! 'c:\Windows\SysNative\cmd.exe' не прокатит, т.к. доступен
			// только для 32битных приложений. А нам нужно в общем виде.
			// Если из 32битного нужно запустить 64битный cmd.exe - нужно выключать редиректор.

			if (!bWin64 || (Bits != csb_x32))
			{
				_wcscat_c(pszComspec, cchMax, (pszComspec[n-1] == L'\\') ? L"System32\\cmd.exe" : L"\\System32\\cmd.exe");
			}
			else
			{
				_wcscat_c(pszComspec, cchMax, (pszComspec[n-1] == L'\\') ? L"SysWOW64\\cmd.exe" : L"\\SysWOW64\\cmd.exe");
			}
		}
	}

	if (pszComspec[0] && !FileExists(pszComspec))
	{
		_ASSERTE("Comspec not found! File not exists!");
		pszComspec[0] = 0;
	}

	// Last chance
	if (pszComspec[0] == 0)
	{
		_ASSERTE(pszComspec[0] != 0); // Уже должен был быть определен
		//lstrcpyn(pszComspec, L"cmd.exe", cchMax);
		wchar_t *psFilePart;
		DWORD n = SearchPathW(NULL, L"cmd.exe", NULL, cchMax, pszComspec, &psFilePart);
		if (!n || (n >= cchMax))
			_wcscpy_c(pszComspec, cchMax, L"cmd.exe");
	}

	return pszComspec;
}

// Найти "ComSpec" и вернуть lstrdup на него
wchar_t* GetComspec(const ConEmuComspec* pOpt)
{
	wchar_t* pszComSpec = NULL;

	if (pOpt)
	{
		if ((pOpt->csType == cst_Explicit) && *pOpt->ComspecExplicit)
		{
			pszComSpec = lstrdup(pOpt->ComspecExplicit);
		}

		if (!pszComSpec && (pOpt->csBits == csb_SameOS))
		{
			BOOL bWin64 = IsWindows64();
			if (bWin64 ? *pOpt->Comspec64 : *pOpt->Comspec32)
				pszComSpec = lstrdup(bWin64 ? pOpt->Comspec64 : pOpt->Comspec32);
		}

		if (!pszComSpec && (pOpt->csBits == csb_SameApp))
		{
			BOOL bWin64 = WIN3264TEST(FALSE,TRUE);
			if (bWin64 ? *pOpt->Comspec64 : *pOpt->Comspec32)
				pszComSpec = lstrdup(bWin64 ? pOpt->Comspec64 : pOpt->Comspec32);
		}

		if (!pszComSpec)
		{
			BOOL bWin64 = (pOpt->csBits != csb_x32);
			if (bWin64 ? *pOpt->Comspec64 : *pOpt->Comspec32)
				pszComSpec = lstrdup(bWin64 ? pOpt->Comspec64 : pOpt->Comspec32);
		}
	}
	else
	{
		_ASSERTE(pOpt && L"pOpt должен быть передан, по идее");
	}

	if (!pszComSpec)
	{
		wchar_t szComSpec[MAX_PATH];
		pszComSpec = lstrdup(GetComspecFromEnvVar(szComSpec, countof(szComSpec)));
	}

	// Уже должно быть хоть что-то
	_ASSERTE(pszComSpec && *pszComSpec);
	return pszComSpec;
}


bool IsExportEnvVarAllowed(LPCWSTR szName)
{
	if (!szName || !*szName)
		return false;

	// Хотя некоторые внутренние переменные менять можно
	if (lstrcmpi(szName, ENV_CONEMU_SLEEP_INDICATE_W) == 0)
		return true;

	// Но большинство внутренних переменных ConEmu запрещено менять (экспортировать)
	wchar_t szTemp[8];
	lstrcpyn(szTemp, szName, 7); szTemp[7] = 0;
	if (lstrcmpi(szTemp, L"ConEmu") == 0)
		return false;

	return true;
}

void ApplyExportEnvVar(LPCWSTR asEnvNameVal)
{
	if (!asEnvNameVal)
		return;

	while (*asEnvNameVal)
	{
		LPCWSTR pszName = asEnvNameVal;
		LPCWSTR pszVal = pszName + lstrlen(pszName) + 1;
		LPCWSTR pszNext = pszVal + lstrlen(pszVal) + 1;
		// Skip ConEmu's internals!
		if (IsExportEnvVarAllowed(pszName))
		{
			// Doubt that someone really needs to export empty var instead of its deletion
			SetEnvironmentVariableW(pszName, *pszVal ? pszVal : NULL);
		}
		asEnvNameVal = pszNext;
	}
}

bool CoordInSmallRect(const COORD& cr, const SMALL_RECT& rc)
{
	return (cr.X >= rc.Left && cr.X <= rc.Right && cr.Y >= rc.Top && cr.Y <= rc.Bottom);
}

UINT GetCpFromString(LPCWSTR asString, LPCWSTR* ppszEnd /*= NULL*/)
{
	UINT nCP = 0; wchar_t* pszEnd = NULL;

	struct KnownCpList {
		LPCWSTR pszName;
		DWORD_PTR nCP;
	} CP[] = {
		// longer - first
		{L"utf-8", CP_UTF8},
		{L"utf8", CP_UTF8},
		{L"ansicp", CP_ACP},
		{L"ansi", CP_ACP},
		{L"acp", CP_ACP},
		{L"oemcp", CP_OEMCP},
		{L"oem", CP_OEMCP},
		{NULL}
	};

	if (!asString)
		goto wrap;

	for (KnownCpList* p = CP; p->pszName; p++)
	{
		int iLen = lstrlen(p->pszName);
		if (lstrcmpni(asString, p->pszName, iLen) == 0)
		{
			// После имени могут быть разделители (знаки пунктуации)
			if (asString[iLen] == 0 || (asString[iLen] && wcschr(L",.:; \t\r\n", asString[iLen])))
			{
				nCP = LODWORD(p->nCP);
				pszEnd = (wchar_t*)asString+iLen;
				// CP_ACP is 0, so jump to wrap instead of break
				goto wrap;
			}
		}
	}

	nCP = wcstoul(asString, &pszEnd, 10);

wrap:
	if (ppszEnd)
		*ppszEnd = pszEnd;

	if (nCP <= 0xFFFF)
		return nCP;
	return 0;
}
