
/*
Copyright (c) 2009-present Maximus5
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

#include "defines.h"
#include "CmdLine.h"
#include "EnvVar.h"
#include "MHandle.h"
#include "MModule.h"
#include "MStrDup.h"
#include "WObjects.h"

#include "MWow64Disable.h"

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

bool FileExists(const wchar_t* asFilePath, uint64_t* pnSize /*= nullptr*/)
{
	#if CE_UNIT_TEST==1
	{
		bool result = false;
		extern bool FileExistsMock(const wchar_t* asFilePath, uint64_t * pnSize, bool& result);
		if (FileExistsMock(asFilePath, pnSize, result))
		{
			return result;
		}
	}
	#endif

	if (pnSize)
		*pnSize = 0;
	if (!asFilePath || !*asFilePath)
		return false;

	_ASSERTE(wcschr(asFilePath, L'\t')==nullptr);


	MHandle hFile;
	MWow64Disable wow;
	if (!hFile.SetHandle(CreateFileW(asFilePath, GENERIC_READ, FILE_SHARE_READ,
		nullptr, OPEN_EXISTING, 0, nullptr), CloseHandle))
	{
		const DWORD nErrCode = GetLastError();
		#ifndef _WIN64
		// e.g. checking "C:\Windows\System32\wsl.exe" from 32-bit ConEmuC
		if (nErrCode == ERROR_FILE_NOT_FOUND)
		{
			wow.Disable();
			
			hFile.SetHandle(CreateFileW(asFilePath, GENERIC_READ, FILE_SHARE_READ,
				nullptr, OPEN_EXISTING, 0, nullptr), CloseHandle);
		}
		#endif
		std::ignore = nErrCode;
	}
	if (hFile.HasHandle())
	{
		bool lbFileFound = false;
		BY_HANDLE_FILE_INFORMATION fi{};
		if (GetFileInformationByHandle(hFile, &fi) && !(fi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			if (pnSize)
				*pnSize = (static_cast<uint64_t>(fi.nFileSizeHigh) << 32) | static_cast<uint64_t>(fi.nFileSizeLow);
			lbFileFound = true;
		}
		return lbFileFound;
	}
	wow.Restore();


	bool lbFound = false;
	// #TODO May we safely remove FindFirstFileW check and rely on CreateFileW only?
	WIN32_FIND_DATAW fnd{};
	const MHandle hFind(FindFirstFileW(asFilePath, &fnd), FindClose);

	if (hFind.HasHandle())
	{
		do
		{
			if ((fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				lbFound = true;

				if (pnSize)
					*pnSize = (static_cast<uint64_t>(fnd.nFileSizeHigh) << 32) | static_cast<uint64_t>(fnd.nFileSizeLow);

				_ASSERTE(FALSE && "CreateFile should be succeesful above");

				break;
			}
		} while (FindNextFileW(hFind, &fnd));
	}

	return lbFound;
}

int apiSearchPath(LPCWSTR lpPath, LPCWSTR lpFileName, LPCWSTR lpExtension, CEStr& rsPath)
{
	#if CE_UNIT_TEST==1
	{
		int result = 0;
		extern bool SearchPathMock(LPCWSTR path, LPCWSTR fileName, LPCWSTR extension, CEStr& resultPath, int& rc);
		if (SearchPathMock(lpPath, lpFileName, lpExtension, rsPath, result))
		{
			return result;
		}
	}
	#endif

	if (!lpFileName || !*lpFileName)
	{
		_ASSERTE(lpFileName != nullptr);
		return 0;
	}
	
	bool bFound = false;
	wchar_t *pszFilePart = nullptr, *pszBuffer = nullptr;
	wchar_t szFind[MAX_PATH+1];

	const DWORD nLen = SearchPath(lpPath, lpFileName, lpExtension, countof(szFind), szFind, &pszFilePart);
	if (nLen)
	{
		if (nLen < countof(szFind))
		{
			rsPath.Set(szFind);
			bFound = true;
		}
		else
		{
			// Too long path, allocate more space
			pszBuffer = static_cast<wchar_t*>(malloc((static_cast<size_t>(nLen) + 1) * sizeof(*pszBuffer)));
			if (pszBuffer)
			{
				const DWORD nLen2 = SearchPath(lpPath, lpFileName, lpExtension, nLen + 1, pszBuffer, &pszFilePart);
				if (nLen2 && (nLen2 <= nLen))
				{
					bFound = true;
					rsPath.Attach(std::move(pszBuffer));  // NOLINT(performance-move-const-arg)
				}
				else
				{
					free(pszBuffer);
				}
			}
		}
	}

	int result = 0;
	if (bFound)
	{
		const auto len = rsPath.GetLen();
		// no sense in path strings greater than supported by OS
		_ASSERTE(len >= 0 && len <= MAX_WIDE_PATH_LENGTH);
		result = static_cast<int>(len);
	}
	return result;
}

int apiGetFullPathName(LPCWSTR lpFileName, CEStr& rsPath)
{
	if (!lpFileName || !*lpFileName)
	{
		_ASSERTE(lpFileName != nullptr);
		return 0;
	}

	int iFoundLen = 0;
	wchar_t* pszFilePart = nullptr;
	// ReSharper disable once CppInitializedValueIsAlwaysRewritten
	wchar_t* pszBuffer = nullptr;
	wchar_t szFind[MAX_PATH+1] = L"";

	DWORD nLen = GetFullPathName(lpFileName, countof(szFind), szFind, &pszFilePart);
	if (nLen)
	{
		if (nLen < countof(szFind))
		{
			rsPath.Set(szFind);
			iFoundLen = static_cast<int>(rsPath.GetLen());
		}
		else
		{
			// Too long path, allocate more space
			pszBuffer = static_cast<wchar_t*>(malloc((++nLen)*sizeof(*pszBuffer)));
			if (pszBuffer)
			{
				const DWORD nLen2 = GetFullPathName(lpFileName, nLen, pszBuffer, &pszFilePart);
				if (nLen2 && (nLen2 < nLen))
				{
					rsPath.Set(pszBuffer);
					iFoundLen = static_cast<int>(rsPath.GetLen());
				}
				free(pszBuffer);
			}
		}
	}

	return (iFoundLen > 0) ? iFoundLen : 0;
}

bool FileSearchInDir(LPCWSTR asFilePath, CEStr& rsFound)
{
	// Possibilities
	// a) asFilePath does not contain path, only: "far"
	// b) asFilePath contains path, but without extension: "C:\\Program Files\\Far\\Far"

	if (!asFilePath || !*asFilePath)
	{
		_ASSERTE(asFilePath != nullptr);
		return false;
	}

	LPCWSTR pszSearchFile = asFilePath;
	const auto* pszSlash = wcsrchr(asFilePath, L'\\');
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
		const auto* pszFwd = wcschr(asFilePath, L'/');
		if (pszFwd != nullptr)
		{
			return false;
		}
	}

	CEStr searchPathBuff;
	if (pszSlash)
	{
		searchPathBuff.Set(asFilePath);
		if (searchPathBuff)
		{
			pszSearchFile = pszSlash + 1;
			searchPathBuff.SetAt(pszSearchFile - asFilePath, L'\0');
		}
	}

	// ReSharper disable once CppLocalVariableMayBeConst
	LPCWSTR pszExt = PointToExt(asFilePath);

	// Try to find the executable file in %PATH%
	const auto foundLen = apiSearchPath(searchPathBuff, pszSearchFile, pszExt ? nullptr : L".exe", rsFound);
	return (foundLen > 0);
}

SearchAppPaths_t gfnSearchAppPaths = nullptr;

bool FileExistsSearch(LPCWSTR asFilePath, CEStr& rsFound, bool abSetPath/*= true*/)
{
	if (!asFilePath || !*asFilePath)
	{
		_ASSERTEX(asFilePath != nullptr);
		return false;
	}

	if (FileExists(asFilePath))
	{
		rsFound.Set(asFilePath);
		return true;
	}

	// Expand %EnvVars%
	if (wcschr(asFilePath, L'%'))
	{
		CEStr szExpand(ExpandEnvStr(asFilePath));
		if (!szExpand.IsEmpty() && FileExists(szExpand.c_str()))
		{
			// asFilePath will be invalid after .Set
			rsFound = std::move(szExpand);
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

	#if defined(_DEBUG)
	CEStr lsExecutable;
	GetModulePathName(nullptr, lsExecutable);
	_ASSERTE(gfnSearchAppPaths!=nullptr || (!IsConsoleServer(lsExecutable) && !IsConEmuGui(lsExecutable)));
	#endif

	// В ConEmuHk этот блок не активен, потому что может быть "только" перехват CreateProcess,
	// а о его параметрах должно заботиться вызывающее (текущее) приложение
	if (gfnSearchAppPaths && !wcschr(asFilePath, L'\\'))
	{
		// Если в asFilePath НЕ указан путь - искать приложение в реестре,
		// и там могут быть указаны доп. параметры (пока только добавка в %PATH%)
		if (gfnSearchAppPaths(asFilePath, rsFound, abSetPath, nullptr))
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
		        || (wcschr(fnd.cFileName, L' ') != nullptr))
		{
			if (lstrlen(fnd.cAlternateFileName) >= cchShortNameMax) //-V303
				return false;
			_wcscpy_c(rsShortName, cchShortNameMax, fnd.cAlternateFileName); //-V106
			return TRUE;
		}
	}
	else if (wcschr(fnd.cFileName, L' ') != nullptr)
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
		return nullptr;

	int nSrcLen = lstrlenW(asLong); //-V303
	wchar_t* pszLong = lstrdup(asLong);

	int nMaxLen = nSrcLen + MAX_PATH; // "короткое" имя может более MAX_PATH
	wchar_t* pszShort = (wchar_t*)calloc(nMaxLen, sizeof(wchar_t)); //-V106

	wchar_t* pszResult = nullptr;
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

		if (wcscmp(pszSrc, L"\\..") == 0)
		{
			int nTrim = -1;
			for (int i = nCurLen-2; i > 0; --i)
			{
				if (pszShort[i] == L'\\')
				{
					nTrim = i;
					break;
				}
			}
			if (nTrim > 0)
			{
				nCurLen = nTrim+1;
				pszShort[nCurLen] = 0;
				szName[0] = 0;
			}
			else
			{
				wcscpy_s(szName, MAX_PATH+1, L"..");
			}
		}
		else if (!GetShortFileName(pszLong, MAX_PATH+1, szName, abFavorLength))
		{
			goto wrap;
		}

		nLen = lstrlenW(szName);
		if (nLen > 0)
		{
			if ((nLen + nCurLen) >= nMaxLen)
				goto wrap;

			_wcscpyn_c(pszShort+nCurLen, nMaxLen-nCurLen, szName, nLen);
			nCurLen += nLen;
		}

		if (pszSlash)
		{
			*pszSlash = L'\\';
			if (nLen > 0)
				pszShort[nCurLen++] = L'\\'; // память выделяется calloc!
		}
	}

	nLen = lstrlenW(pszShort);

	if ((nLen > 0) && (pszShort[nLen-1] == L'\\'))
		pszShort[--nLen] = 0;

	if (nLen <= MAX_PATH)
	{
		pszResult = pszShort;
		pszShort = nullptr;
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

DWORD GetModulePathName(HMODULE hModule, CEStr& lsPathName)
{
	DWORD cchMax = MAX_PATH;
	wchar_t* pszData = lsPathName.GetBuffer(cchMax);
	if (!pszData)
		return 0; // Memory allocation error

	DWORD nRc = GetModuleFileName(hModule, pszData, cchMax);
	if (!nRc)
		return 0;

	const DWORD errCode = GetLastError();

	// Not enough space?
	if (nRc == cchMax || errCode == ERROR_INSUFFICIENT_BUFFER)
	{
		cchMax = MAX_WIDE_PATH_LENGTH;
		pszData = lsPathName.GetBuffer(cchMax);
		if (!pszData)
			return 0; // Memory allocation error
		nRc = GetModuleFileName(hModule, pszData, cchMax);
		if (!nRc || (nRc == cchMax))
			return 0;
	}

	return nRc;
}

#ifndef __GNUC__
extern "C" IMAGE_DOS_HEADER __ImageBase;
#endif

DWORD GetCurrentModulePathName(CEStr& lsPathName)
{
	auto* module =
#ifndef __GNUC__
		reinterpret_cast<HMODULE>(&__ImageBase)
#else
		static_cast<HMODULE>(nullptr)
#endif
		;
	return GetModulePathName(module, lsPathName);
}

//wchar_t* GetShortFileNameEx(LPCWSTR asLong, BOOL abFavorLength=FALSE)
//{
//	TODO("хорошо бы и сетевые диски обрабатывать");
//
//	if (!asLong) return nullptr;
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
//	return nullptr;
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
			HKEY hk = nullptr;
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
		RegKeyExistArg Keys[] = {{HKEY_CURRENT_USER, L"Software\\Wine"}, {HKEY_LOCAL_MACHINE, L"Software\\Wine"}, {nullptr}};
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
			{nullptr}};
		ibIsWinPE = CheckRegKeyExist(Keys) ? 1 : -1;
	}
	// В общем случае, на флажок ориентироваться нельзя. Это для информации.
	return (ibIsWinPE == 1);
}

bool GetOsVersionInformational(OSVERSIONINFO* pOsVer)
{
	#pragma warning(disable: 4996)
	// #WINVER Support Rtl function like _VerifyVersionInfo does
	// ReSharper disable once CppDeprecatedEntity
	const BOOL result = GetVersionExW(pOsVer);  // NOLINT(clang-diagnostic-deprecated-declarations)
	#pragma warning(default: 4996)
	return (result != FALSE);
}

OSVERSIONINFOEXW MakeOsVersionEx(const DWORD dwMajorVersion, const DWORD dwMinorVersion)
{
	OSVERSIONINFOEXW osVersion{};
	osVersion.dwOSVersionInfoSize = sizeof(osVersion);
	osVersion.dwMajorVersion = dwMajorVersion;
	osVersion.dwMinorVersion = dwMinorVersion;
	return osVersion;
}

bool _VerifyVersionInfo(LPOSVERSIONINFOEXW lpVersionInformation, DWORD dwTypeMask, DWORDLONG dwlConditionMask)
{
	bool rc;

	// If our dll was loaded into some executable without proper `supportedOS`
	// in its manifest, the VerifyVersionInfoW fails to check *real* things
	static BOOL (WINAPI *rtlVerifyVersionInfo)(LPOSVERSIONINFOEXW, DWORD, DWORDLONG) = nullptr;
	static int hasRtl = 0;
	if (hasRtl == 0)
	{
		MModule ntdll(GetModuleHandle(L"ntdll.dll"));
		hasRtl = ntdll.GetProcAddress("RtlVerifyVersionInfo", rtlVerifyVersionInfo) ? 1 : -1;
	}

	if (rtlVerifyVersionInfo)
		rc = (0/*STATUS_SUCCESS*/ == rtlVerifyVersionInfo(lpVersionInformation, dwTypeMask, dwlConditionMask));
	else
		rc = !!VerifyVersionInfoW(lpVersionInformation, dwTypeMask, dwlConditionMask);
	return rc;
}

bool IsWinVerOrHigher(const WORD osVerNum)
{
	auto osVersion = MakeOsVersionEx(HIBYTE(osVerNum), LOBYTE(osVerNum));
	DWORDLONG const dwlConditionMask = VerSetConditionMask(VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_MINORVERSION, VER_GREATER_EQUAL);
	const BOOL ibIsWinOrHigher = _VerifyVersionInfo(&osVersion, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask);
	return (ibIsWinOrHigher != FALSE);
}

bool IsWinVerEqual(const WORD osVerNum)
{
	auto osVersion = MakeOsVersionEx(HIBYTE(osVerNum), LOBYTE(osVerNum));
	DWORDLONG const dwlConditionMask = VerSetConditionMask(VerSetConditionMask(0, VER_MAJORVERSION, VER_EQUAL), VER_MINORVERSION, VER_EQUAL);
	const BOOL ibIsWinOrHigher = _VerifyVersionInfo(&osVersion, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask);
	return (ibIsWinOrHigher != FALSE);
}

// Only Windows 2000
bool IsWin2kEql()
{
	static int ibIsWin2K = 0;
	if (!ibIsWin2K)
	{
		ibIsWin2K = IsWinVerEqual(_WIN32_WINNT_WIN2K) ? 1 : -1;;
	}
	return (ibIsWin2K == 1);
}

// Only 5.x family (Win2k, WinXP, Win 2003 server)
bool IsWin5family()
{
	static int ibIsWin5fam = 0;
	if (!ibIsWin5fam)
	{
		// Don't use IsWinVerEqual here - we need to compare only major version!
		auto osVersion = MakeOsVersionEx(HIBYTE(_WIN32_WINNT_WIN2K), LOBYTE(_WIN32_WINNT_WIN2K));
		DWORDLONG const dwlConditionMask = VerSetConditionMask(0, VER_MAJORVERSION, VER_EQUAL);
		ibIsWin5fam = _VerifyVersionInfo(&osVersion, VER_MAJORVERSION, dwlConditionMask) ? 1 : -1;
	}
	return (ibIsWin5fam == 1);
}

// Windows XP or higher
bool IsWinXP()
{
	static int ibIsWinXp = 0;
	if (!ibIsWinXp)
	{
		auto osVersion = MakeOsVersionEx(HIBYTE(_WIN32_WINNT_WINXP), LOBYTE(_WIN32_WINNT_WINXP));
		DWORDLONG const dwlConditionMask = VerSetConditionMask(VerSetConditionMask(0,
			VER_MAJORVERSION, VER_GREATER_EQUAL),
			VER_MINORVERSION, VER_GREATER_EQUAL);
		ibIsWinXp = _VerifyVersionInfo(&osVersion, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask) ? 1 : -1;;
	}
	return (ibIsWinXp == 1);
}

// WinXP SP1 or higher
bool IsWinXP(const WORD servicePack)
{
	static int ibIsWinXpSp1 = 0;
	if (!ibIsWinXpSp1)
	{
		auto osVersion = MakeOsVersionEx(HIBYTE(_WIN32_WINNT_WINXP), LOBYTE(_WIN32_WINNT_WINXP));
		osVersion.wServicePackMajor = servicePack;
		DWORDLONG const dwlConditionMask = VerSetConditionMask(VerSetConditionMask(VerSetConditionMask(0,
			VER_MAJORVERSION, VER_GREATER_EQUAL),
			VER_MINORVERSION, VER_GREATER_EQUAL),
			VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
		ibIsWinXpSp1 = _VerifyVersionInfo(&osVersion, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR, dwlConditionMask) ? 1 : -1;;
	}
	return (ibIsWinXpSp1 == 1);
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

// Windows 7 exactly
bool IsWin7Eql()
{
	static int ibIsWin7Eql = 0;
	if (!ibIsWin7Eql)
	{
		_ASSERTE(_WIN32_WINNT_WIN7 == 0x601);
		ibIsWin7Eql = IsWinVerEqual(_WIN32_WINNT_WIN7) ? 1 : -1;
	}
	return (ibIsWin7Eql == 1);
}

// Windows 8 and higher
bool IsWin8()
{
	static int ibIsWin8 = 0;
	if (!ibIsWin8)
	{
		#ifndef _WIN32_WINNT_WIN8
		#define _WIN32_WINNT_WIN8 0x602
		#endif
		_ASSERTE(_WIN32_WINNT_WIN8 == 0x602);
		ibIsWin8 = IsWinVerOrHigher(_WIN32_WINNT_WIN8) ? 1 : -1;
	}
	return (ibIsWin8 == 1);
}

// Windows 8.1 and higher
// ReSharper disable once CppInconsistentNaming
bool IsWin8_1()
{
	static int ibIsWin8_1 = 0;
	if (!ibIsWin8_1)
	{
		#ifndef _WIN32_WINNT_WIN8_1
		#define _WIN32_WINNT_WIN8_1 0x603
		#endif
		_ASSERTE(_WIN32_WINNT_WIN8_1 == 0x603);
		ibIsWin8_1 = IsWinVerOrHigher(_WIN32_WINNT_WIN8_1) ? 1 : -1;
	}
	return (ibIsWin8_1 == 1);
}

// Windows 10 and higher
bool IsWin10()
{
	static int ibIsWin10 = 0;
	if (!ibIsWin10)
	{
		// First insider builds of Win10 were numbered as 6.4
		// Now it's a 10.0, but we still compare with ‘old’ number
		#ifndef _WIN32_WINNT_WIN10_PREVIEW
		#define _WIN32_WINNT_WIN10_PREVIEW 0x604
		#endif
		_ASSERTE(_WIN32_WINNT_WIN10_PREVIEW == 0x604);
		ibIsWin10 = IsWinVerOrHigher(_WIN32_WINNT_WIN10_PREVIEW) ? 1 : -1;

		//BUGBUG: It return FALSE on Win10 when our dll is loaded into some app (Far.exe) without new manifest/OSGUID

		if ((ibIsWin10 == -1) && IsWin8_1())
		{
			HMODULE hKernel32 = GetModuleHandle(L"kernel32.dll");
			if (hKernel32)
			{
				FARPROC pfnCheck = hKernel32 ? (FARPROC)GetProcAddress(hKernel32, "FreeMemoryJobObject") : nullptr;
				// Seems like it's a Win10
				if (pfnCheck)
				{
					ibIsWin10 = 1;
				}
				/*
				VS_FIXEDFILEINFO OsVer = {};
				wchar_t szKernelPath[MAX_PATH] = L"";
				if (!GetModuleFileName(hKernel32, szKernelPath, countof(szKernelPath)))
					wcscpy_c(szKernelPath, L"kernel32.dll");
				// Is returns 6.3 too!
				if (LoadModuleVersion(szKernelPath, OsVer))
				{
					if (HIWORD(OsVer.dwFileVersionMS) >= 10)
					{
						ibIsWin10 = 1;
					}
				}
				*/
			}
		}
	}
	return (ibIsWin10 == 1);
}

// Returns true for CJK versions of Windows (Chinese, Japanese, etc.)
// Their consoles behave in different way than European versions unfortunately
bool IsWinDBCS()
{
	static int isDBCS = 0;
	if (!isDBCS)
	{
		// GetSystemMetrics may hangs if called during normal process termination
		isDBCS = (GetSystemMetrics(SM_DBCSENABLED) != 0) ? 1 : -1;
	}
	return (isDBCS == 1);
}

// Проверить битность OS
bool IsWindows64()
{
	static bool is64bitOs = false, isOsChecked = false;
	if (isOsChecked)
		return is64bitOs;

#ifdef WIN64
	is64bitOs = true;
#else
	// Проверяем, где мы запущены
	bool isWow64process = false;
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
	if (IsWin6())
		return false; // Vista or higher - not available
	else
		return true;
}

/// Substitute in the pszFormat macros '%1' ... '%9' with pszValues[0] .. pszValues[8]
wchar_t* ExpandMacroValues(LPCWSTR pszFormat, LPCWSTR* pszValues, size_t nValCount)
{
	wchar_t* pszCommand = nullptr;
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
			LPCWSTR pszMacro = nullptr;

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
					pszMacro = nullptr;
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

LPCWSTR GetComspecFromEnvVar(wchar_t* pszComspec, DWORD cchMax, ComSpecBits Bits/* = csb_SameOS*/)
{
	if (!pszComspec || (cchMax < MAX_PATH))
	{
		_ASSERTE(pszComspec && (cchMax >= MAX_PATH));
		return nullptr;
	}

	*pszComspec = 0;
	const auto bWin64 = IsWindows64();

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
		DWORD n = SearchPathW(nullptr, L"cmd.exe", nullptr, cchMax, pszComspec, &psFilePart);
		if (!n || (n >= cchMax))
			_wcscpy_c(pszComspec, cchMax, L"cmd.exe");
	}

	return pszComspec;
}

// Найти "ComSpec" и вернуть lstrdup на него
wchar_t* GetComspec(const ConEmuComspec* pOpt)
{
	wchar_t* pszComSpec = nullptr;

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
		_ASSERTE(pOpt && L"pOpt should be passed");
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

	// Some internal variables are allowed to be exported
	if ((lstrcmpi(szName, ENV_CONEMU_SLEEP_INDICATE_W) == 0)
		|| (lstrcmpi(szName, CEGUIMACRORETENVVAR) == 0))
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
			SetEnvironmentVariableW(pszName, *pszVal ? pszVal : nullptr);
		}
		asEnvNameVal = pszNext;
	}
}

bool CoordInSmallRect(const COORD& cr, const SMALL_RECT& rc)
{
	return (cr.X >= rc.Left && cr.X <= rc.Right && cr.Y >= rc.Top && cr.Y <= rc.Bottom);
}

UINT GetCpFromString(LPCWSTR asString, LPCWSTR* ppszEnd /*= nullptr*/)
{
	UINT nCP = 0; wchar_t* pszEnd = nullptr;

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
		{nullptr}
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
