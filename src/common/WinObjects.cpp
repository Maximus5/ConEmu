
/*
Copyright (c) 2009-2014 Maximus5
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
#include "MAssert.h"
#include "WinObjects.h"
#include "CmdLine.h"
#include "../ConEmu/version.h"
#include <TlHelp32.h>

#ifdef _DEBUG
	//#define WARN_NEED_CMD
	#undef WARN_NEED_CMD
	//#define SHOW_COMSPEC_CHANGE
	#undef SHOW_COMSPEC_CHANGE
#else
	#undef WARN_NEED_CMD
	#undef SHOW_COMSPEC_CHANGE
#endif

#ifndef TTS_BALLOON
#define TTS_BALLOON             0x40
#define TTF_TRACK               0x0020
#define TTF_ABSOLUTE            0x0080
#define TTM_SETMAXTIPWIDTH      (WM_USER + 24)
#define TTM_TRACKPOSITION       (WM_USER + 18)  // lParam = dwPos
#define TTM_TRACKACTIVATE       (WM_USER + 17)  // wParam = TRUE/FALSE start end  lparam = LPTOOLINFO
#endif


#ifdef _DEBUG
#define DebugString(x) //OutputDebugString(x)
#define DebugStringA(x) //OutputDebugStringA(x)
#define DEBUGSTRLOG(x) //OutputDebugStringA(x)
#else
#define DebugString(x) //OutputDebugString(x)
#define DebugStringA(x) //OutputDebugStringA(x)
#define DEBUGSTRLOG(x)
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
		msprintf(rsInfo, countof(rsInfo), L"0x%08X: Invalid window handle", (DWORD)ahWnd);
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

		msprintf(rsInfo, countof(rsInfo), L"0x%08X: %s - '%s'%s", (DWORD)ahWnd, szClass, szTitle, szProc);
	}

	if (pnPID)
		*pnPID = nPID;
}

#ifndef CONEMU_MINIMAL
BOOL apiSetForegroundWindow(HWND ahWnd)
{
#ifdef _DEBUG
	wchar_t szLastFore[1024]; getWindowInfo(GetForegroundWindow(), szLastFore);
	wchar_t szWnd[1024]; getWindowInfo(ahWnd, szWnd);
#endif
	BOOL lbRc = ::SetForegroundWindow(ahWnd);
	return lbRc;
}
#endif

#ifndef CONEMU_MINIMAL
// If the window was previously visible, the return value is nonzero.
// If the window was previously hidden, the return value is zero.
BOOL apiShowWindow(HWND ahWnd, int anCmdShow)
{
#ifdef _DEBUG
	wchar_t szLastFore[1024]; getWindowInfo(GetForegroundWindow(), szLastFore);
	wchar_t szWnd[1024]; getWindowInfo(ahWnd, szWnd);
#endif
	BOOL lbRc = ::ShowWindow(ahWnd, anCmdShow);
	return lbRc;
}
#endif

#ifndef CONEMU_MINIMAL
BOOL apiShowWindowAsync(HWND ahWnd, int anCmdShow)
{
#ifdef _DEBUG
	wchar_t szLastFore[1024]; getWindowInfo(GetForegroundWindow(), szLastFore);
	wchar_t szWnd[1024]; getWindowInfo(ahWnd, szWnd);
#endif
	BOOL lbRc = ::ShowWindowAsync(ahWnd, anCmdShow);
	return lbRc;
}
#endif


bool FileCompare(LPCWSTR asFilePath1, LPCWSTR asFilePath2)
{
	bool bMatch = false;
	HANDLE hFile1, hFile2 = NULL;
	LPBYTE pBuf1 = NULL, pBuf2 = NULL;
	LARGE_INTEGER lSize1 = {}, lSize2 = {};
	DWORD nRead1 = 0, nRead2 = 0;
	BOOL bRead1, bRead2;

	hFile1 = CreateFile(asFilePath1, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (!hFile1 || hFile1 == INVALID_HANDLE_VALUE)
		goto wrap;
	hFile2 = CreateFile(asFilePath2, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (!hFile2 || hFile2 == INVALID_HANDLE_VALUE)
		goto wrap;

	if (!GetFileSizeEx(hFile1, &lSize1) || !lSize1.QuadPart || lSize1.HighPart)
		goto wrap;
	if (!GetFileSizeEx(hFile2, &lSize2) || !lSize2.QuadPart || lSize2.HighPart)
		goto wrap;
	if (lSize1.QuadPart != lSize2.QuadPart)
		goto wrap;

	// Need binary comparision
	pBuf1 = (LPBYTE)malloc(lSize1.LowPart);
	pBuf2 = (LPBYTE)malloc(lSize2.LowPart);
	if (!pBuf1 || !pBuf2)
		goto wrap;

	bRead1 = ReadFile(hFile1, pBuf1, lSize1.LowPart, &nRead1, NULL);
	bRead2 = ReadFile(hFile2, pBuf2, lSize2.LowPart, &nRead2, NULL);

	if (!bRead1 || !bRead2 || nRead1 != lSize1.LowPart || nRead2 != nRead1)
		goto wrap;

	// Comparision
	bMatch = (memcmp(pBuf1, pBuf2, lSize1.LowPart) == 0);
wrap:
	if (hFile1 && hFile1 != INVALID_HANDLE_VALUE)
		CloseHandle(hFile1);
	if (hFile2 && hFile2 != INVALID_HANDLE_VALUE)
		CloseHandle(hFile2);
	SafeFree(pBuf1);
	SafeFree(pBuf2);
	return bMatch;
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

// asDirectory - folder, where to check files (doesn't have to have trailing "\")
// asFileList  - "\0" separated, "\0\0" teminated list of file names (may have subfolders)
// but if (anListCount != -1) - only first anListCount files in asFileList will be checked
bool FilesExists(LPCWSTR asDirectory, LPCWSTR asFileList, bool abAll /*= false*/, int anListCount /*= -1*/)
{
	if (!asDirectory || !*asDirectory)
		return false;
	if (!asFileList || !*asFileList)
		return false;

	bool bFound = false;
	wchar_t* pszBuf = NULL;
	LPCWSTR pszCur = asFileList;
	int nDirLen = lstrlen(asDirectory);

	while (*pszCur)
	{
		int nNameLen = lstrlen(pszCur);
		if (asDirectory[nDirLen-1] != L'\\' && *pszCur != L'\\')
			pszBuf = lstrmerge(asDirectory, L"\\", pszCur);
		else
			pszBuf = lstrmerge(asDirectory, pszCur);

		bool bFile = FileExists(pszBuf);
		SafeFree(pszBuf);

		if (bFile)
		{
			bFound = true;
			if (!abAll)
				break;
		}
		else if (abAll)
		{
			bFound = false;
			break;
		}

		if (anListCount == -1)
		{
			pszCur += nNameLen+1;
		}
		else
		{
			anListCount--;
			if (anListCount < 1)
				break;
		}
	}

	return bFound;
}

bool DirectoryExists(LPCWSTR asPath)
{
	if (!asPath || !*asPath)
		return false;

	bool lbFound = false;

	HANDLE hFind = CreateFile(asPath, 0, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (hFind && hFind != INVALID_HANDLE_VALUE)
	{
		BY_HANDLE_FILE_INFORMATION fi = {0};

		if (GetFileInformationByHandle(hFind, &fi) && (fi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			lbFound = true;
		}
		CloseHandle(hFind);

		return lbFound;
	}

	// Try to use FindFirstFile?
	WIN32_FIND_DATAW fnd = {0};
	hFind = FindFirstFile(asPath, &fnd);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	do
	{
		if ((fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
		{
			if (lstrcmp(fnd.cFileName, L".") && lstrcmp(fnd.cFileName, L".."))
			{
				lbFound = TRUE;
				break;
			}
		}
	}
	while (FindNextFile(hFind, &fnd));

	FindClose(hFind);
	return (lbFound != FALSE);
}

bool MyCreateDirectory(wchar_t* asPath)
{
	if (!asPath || !*asPath)
		return false;

	BOOL bOk = FALSE;
	DWORD dwErr = 0;
	wchar_t* psz1, *pszSlash;
	psz1 = wcschr(asPath, L'\\');
	pszSlash = wcsrchr(asPath, L'\\');
	if (pszSlash && (pszSlash == psz1))
		pszSlash = NULL;

	if (pszSlash)
	{
		_ASSERTE(*pszSlash == L'\\');
		*pszSlash = 0;

		if ((pszSlash[1] == 0) && DirectoryExists(asPath))
		{
			*pszSlash = L'\\';
			return true;
		}
	}
	else
	{
		if (DirectoryExists(asPath))
			return true;
	}

	if (CreateDirectory(asPath, NULL))
	{
		bOk = TRUE;
	}
	else
	{
		dwErr = GetLastError();
		if (pszSlash && (dwErr !=  ERROR_ACCESS_DENIED))
		{
			bOk = MyCreateDirectory(asPath);
		}
		else if (dwErr == ERROR_ALREADY_EXISTS)
		{
			_ASSERTE(FALSE && "Must not get here");
		}
	}

	if (pszSlash)
	{
		*pszSlash = L'\\';
	}

	return (bOk != FALSE);
}

// Первичная проверка, может ли asFilePath быть путем
bool IsFilePath(LPCWSTR asFilePath, bool abFullRequired /*= false*/)
{
	if (!asFilePath || !*asFilePath)
		return false;

	// Если в пути встречаются недопустимые символы
	if (wcschr(asFilePath, L'"') ||
	        wcschr(asFilePath, L'>') ||
	        wcschr(asFilePath, L'<') ||
	        wcschr(asFilePath, L'|')
			// '/' не проверяем для совместимости с cygwin?
	  )
		return false;

	// Пропуск UNC "\\?\"
	if (asFilePath[0] == L'\\' && asFilePath[1] == L'\\' && asFilePath[2] == L'?' && asFilePath[3] == L'\\')
		asFilePath += 4; //-V112

	// Если asFilePath содержит два (и более) ":\"
	LPCWSTR pszColon = wcschr(asFilePath, L':');

	if (pszColon)
	{
		// Если есть ":", то это должен быть путь вида "X:\xxx", т.е. ":" - второй символ
		if (pszColon != (asFilePath+1))
			return false;

		if (!isDriveLetter(asFilePath[0]))
			return false;

		if (wcschr(pszColon+1, L':'))
			return false;
	}

	if (abFullRequired)
	{
		if ((asFilePath[0] == L'\\' && asFilePath[1] == L'\\' && asFilePath[2] && wcschr(asFilePath+3,L'\\'))
				|| (isDriveLetter(asFilePath[0]) && asFilePath[1] == L':' && asFilePath[2]))
			return true;
		return false;
	}

	// May be file path
	return true;
}

bool IsPathNeedQuot(LPCWSTR asPath)
{
	if (wcspbrk(asPath, L"<>()&|^\""))
		return true;
	return false;
}

BOOL GetShortFileName(LPCWSTR asFullPath, int cchShortNameMax, wchar_t* rsShortName/*[MAX_PATH+1]-name only*/, BOOL abFavorLength=FALSE)
{
	WARNING("FindFirstFile использовать нельзя из-за симлинков");
	WIN32_FIND_DATAW fnd; memset(&fnd, 0, sizeof(fnd));
	HANDLE hFind = FindFirstFile(asFullPath, &fnd);

	if (hFind == INVALID_HANDLE_VALUE)
		return FALSE;

	FindClose(hFind);

	if (fnd.cAlternateFileName[0])
	{
		if ((abFavorLength && (lstrlenW(fnd.cAlternateFileName) < lstrlenW(fnd.cFileName))) //-V303
		        || (wcschr(fnd.cFileName, L' ') != NULL))
		{
			if (lstrlen(fnd.cAlternateFileName) >= cchShortNameMax) //-V303
				return FALSE;
			_wcscpy_c(rsShortName, cchShortNameMax, fnd.cAlternateFileName); //-V106
			return TRUE;
		}
	}
	else if (wcschr(fnd.cFileName, L' ') != NULL)
	{
		return FALSE;
	}

	if (lstrlen(fnd.cFileName) >= cchShortNameMax) //-V303
		return FALSE;
	_wcscpy_c(rsShortName, cchShortNameMax, fnd.cFileName); //-V106
	return TRUE;
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

#ifndef CONEMU_MINIMAL
bool IsUserAdmin()
{
	// No need to show any "Shield" on XP or 2k
	_ASSERTE(_WIN32_WINNT_VISTA==0x600);
	OSVERSIONINFOEXW osvi = {sizeof(osvi), HIBYTE(_WIN32_WINNT_VISTA), LOBYTE(_WIN32_WINNT_VISTA)};
	DWORDLONG const dwlConditionMask = VerSetConditionMask(VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_MINORVERSION, VER_GREATER_EQUAL);
	if (!VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask))
		return false;

	BOOL b;
	SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};
	PSID AdministratorsGroup;
	b = AllocateAndInitializeSid(
	        &NtAuthority,
	        2,
	        SECURITY_BUILTIN_DOMAIN_RID,
	        DOMAIN_ALIAS_RID_ADMINS,
	        0, 0, 0, 0, 0, 0,
	        &AdministratorsGroup);

	if (b)
	{
		if (!CheckTokenMembership(NULL, AdministratorsGroup, &b))
		{
			b = FALSE;
		}

		FreeSid(AdministratorsGroup);
	}

	return (b != FALSE);
}


#include <Sddl.h> // ConvertSidToStringSid
// *ppszSID - must be LocalFree'd
bool GetLogonSID (HANDLE hToken, wchar_t **ppszSID)
{
	bool bSuccess = false;
	//DWORD dwIndex;
	DWORD dwLength = 0;
	TOKEN_USER user;
	PTOKEN_USER ptu = &user;
	BOOL bFreeToken = FALSE;

	// Verify the parameter passed in is not NULL.
	if (NULL == ppszSID)
		goto Cleanup;
	*ppszSID = NULL;

	if (!hToken)
		bFreeToken = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);

	// Get required buffer size and allocate the TOKEN_GROUPS buffer.
	if (!GetTokenInformation(
		hToken,         // handle to the access token
		TokenUser,      // get information about the token's user account
		(LPVOID) ptu,   // pointer to TOKEN_USER buffer
		0,              // size of buffer
		&dwLength       // receives required buffer size
	))
	{
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			goto Cleanup;

		ptu = (PTOKEN_USER)calloc(dwLength,1);

		if (ptu == NULL)
			goto Cleanup;
	}

	// Get the token group information from the access token.

	if (!GetTokenInformation(
		hToken,         // handle to the access token
		TokenUser,      // get information about the token's user account
		(LPVOID) ptu,   // pointer to TOKEN_USER buffer
		dwLength,       // size of buffer
		&dwLength       // receives required buffer size
	))
	{
		goto Cleanup;
	}

	if (!ConvertSidToStringSid(ptu->User.Sid, ppszSID) || (*ppszSID == NULL))
		goto Cleanup;

	bSuccess = true;

Cleanup:

	// Free the buffer for the token groups.
	if ((ptu != NULL) && (ptu != &user))
		free(ptu);
	if (bFreeToken && hToken)
		CloseHandle(hToken);

	return bSuccess;
}
#endif

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
// Used: MyGetLargestConsoleWindowSize, OnGetLargestConsoleWindowSize
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

bool IsWin10()
{
	static int ibIsWin10 = 0;
	if (!ibIsWin10)
	{
		#define _WIN32_WINNT_WIN10 0x604
		OSVERSIONINFOEXW osvi = {sizeof(osvi), HIBYTE(_WIN32_WINNT_WIN10), LOBYTE(_WIN32_WINNT_WIN10)};
		DWORDLONG const dwlConditionMask = VerSetConditionMask(VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_MINORVERSION, VER_GREATER_EQUAL);
		ibIsWin10 = VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask) ? 1 : -1;
	}
	return (ibIsWin10 == 1);
}

bool IsDbcs()
{
	bool bIsDBCS = (GetSystemMetrics(SM_DBCSENABLED) != 0);
	return bIsDBCS;
}

typedef BOOL (WINAPI* IsWow64Process_t)(HANDLE hProcess, PBOOL Wow64Process);

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
		HANDLE h = hProcess ? hProcess : hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, nPID);

		BOOL bWow64 = FALSE;
		if (IsWow64Process_f(h, &bWow64) && !bWow64)
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
			// Snapshoot больше не нужен
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
		lpTest = (LPBYTE)VirtualAlloc((LPVOID)module, cbCommitSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
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

#ifndef CONEMU_MINIMAL
void RemoveOldComSpecC()
{
	wchar_t szComSpec[MAX_PATH], szComSpecC[MAX_PATH], szRealComSpec[MAX_PATH];
	//110202 - comspec более не переопределяется, поэтому вернем "cmd",
	// если был переопреден и унаследован от старой версии conemu
	if (GetEnvironmentVariable(L"ComSpecC", szComSpecC, countof(szComSpecC)) && szComSpecC[0] != 0)
	{
		szRealComSpec[0] = 0;

		if (!GetEnvironmentVariable(L"ComSpec", szComSpec, countof(szComSpec)))
			szComSpec[0] = 0;

		#ifndef __GNUC__
		#pragma warning( push )
		#pragma warning(disable : 6400)
		#endif

		LPCWSTR pwszName = PointToName(szComSpec);

		if (lstrcmpiW(pwszName, L"ConEmuC.exe")==0 || lstrcmpiW(pwszName, L"ConEmuC64.exe")==0)
		{
			pwszName = PointToName(szComSpecC);
			if (lstrcmpiW(pwszName, L"ConEmuC.exe")!=0 && lstrcmpiW(pwszName, L"ConEmuC64.exe")!=0)
			{
				wcscpy_c(szRealComSpec, szComSpecC);
			}
		}
		#ifndef __GNUC__
		#pragma warning( pop )
		#endif

		if (szRealComSpec[0] == 0)
		{
			//\system32\cmd.exe
			GetComspecFromEnvVar(szRealComSpec, countof(szRealComSpec));
		}

		SetEnvironmentVariable(L"ComSpec", szRealComSpec);
		SetEnvironmentVariable(L"ComSpecC", NULL);
	}
}
#endif

const wchar_t* PointToName(const wchar_t* asFileOrPath)
{
	if (!asFileOrPath)
	{
		_ASSERTE(asFileOrPath!=NULL);
		return NULL;
	}

	// Utilize both type of slashes
	const wchar_t* pszBSlash = wcsrchr(asFileOrPath, L'\\');
	const wchar_t* pszFSlash = wcsrchr(pszBSlash ? pszBSlash : asFileOrPath, L'/');

	const wchar_t* pszFile = pszFSlash ? pszFSlash : pszBSlash;
	if (!pszFile) pszFile = asFileOrPath; else pszFile++;

	return pszFile;
}

const char* PointToName(const char* asFileOrPath)
{
	if (!asFileOrPath)
	{
		_ASSERTE(asFileOrPath!=NULL);
		return NULL;
	}

	// Utilize both type of slashes
	const char* pszBSlash = strrchr(asFileOrPath, '\\');
	const char* pszFSlash = strrchr(pszBSlash ? pszBSlash : asFileOrPath, '/');

	const char* pszSlash = pszFSlash ? pszFSlash : pszBSlash;;

	if (pszSlash)
		return pszSlash+1;

	return asFileOrPath;
}

// Возвращает ".ext" или NULL в случае ошибки
const wchar_t* PointToExt(const wchar_t* asFullPath)
{
	const wchar_t* pszName = PointToName(asFullPath);
	if (!pszName)
		return NULL; // _ASSERTE уже был
	const wchar_t* pszExt = wcsrchr(pszName, L'.');
	return pszExt;
}

// !!! Меняет asParm !!!
// Cut leading and trailing quotas
const wchar_t* Unquote(wchar_t* asParm, bool abFirstQuote /*= false*/)
{
	if (!asParm)
		return NULL;
	if (*asParm != L'"')
		return asParm;
	wchar_t* pszEndQ = abFirstQuote ? wcschr(asParm+1, L'"') : wcsrchr(asParm+1, L'"');
	if (!pszEndQ)
	{
		*asParm = 0;
		return asParm;
	}
	*pszEndQ = 0;
	return (asParm+1);
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

// GetFullPathName & ExpandEnvStr
wchar_t* GetFullPathNameEx(LPCWSTR asPath)
{
	wchar_t* pszResult = NULL;
	wchar_t* pszTemp = NULL;

	if (wcschr(asPath, L'%'))
	{
		if ((pszTemp = ExpandEnvStr(asPath)) != NULL)
		{
			asPath = pszTemp;
		}
	}

	DWORD cchMax = MAX_PATH+1;
	pszResult = (wchar_t*)calloc(cchMax,sizeof(*pszResult));
	if (pszResult)
	{
		wchar_t* pszFilePart;
		DWORD nLen = GetFullPathName(asPath, cchMax, pszResult, &pszFilePart);
		if (!nLen  || (nLen >= cchMax))
		{
			_ASSERTEX(FALSE && "GetFullPathName failed");
			SafeFree(pszResult);
		}
	}

	if (!pszResult)
	{
		_ASSERTEX(pszResult!=NULL);
		pszResult = lstrdup(asPath);
	}

	SafeFree(pszTemp);
	return pszResult;
}

wchar_t* JoinPath(LPCWSTR asPath, LPCWSTR asPart1, LPCWSTR asPart2 /*= NULL*/)
{
	//TODO: Добавить слеши если их нет на гранях
	//TODO: удалить лишние, если они указаны в обеих частях
	return lstrmerge(asPath, asPart1, asPart2);
}







COORD MyGetLargestConsoleWindowSize(HANDLE hConsoleOutput)
{
	// В Wine не работает
	COORD crMax = GetLargestConsoleWindowSize(hConsoleOutput);
	DWORD dwErr = (crMax.X && crMax.Y) ? 0 : GetLastError();
	UNREFERENCED_PARAMETER(dwErr);
	// Wine BUG
	//if (!crMax.X || !crMax.Y)
	if ((crMax.X == 80 && crMax.Y == 24) && IsWine())
	{
		crMax.X = 255;
		crMax.Y = 255;
	}
	else if (IsWin10())
	{
		// Windows 10 Preview has a new bug in GetLargestConsoleWindowSize
		crMax.X = max(crMax.X,555);
		crMax.Y = max(crMax.Y,555);
	}
	return crMax;
}

#ifndef CONEMU_MINIMAL
HANDLE DuplicateProcessHandle(DWORD anTargetPID)
{
	HANDLE hDup = NULL;
	if (anTargetPID == 0)
	{
		_ASSERTEX(anTargetPID != 0);
	}
	else
	{
		HANDLE hTarget = OpenProcess(PROCESS_DUP_HANDLE, FALSE, anTargetPID);
		if (hTarget)
		{
			DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), hTarget, &hDup, MY_PROCESS_ALL_ACCESS, FALSE, 0);

			CloseHandle(hTarget);
		}
	}
	return hDup;
}

#endif


#ifndef CONEMU_MINIMAL
// используется в GUI при загрузке настроек
void FindComspec(ConEmuComspec* pOpt, bool bCmdAlso /*= true*/)
{
	if (!pOpt)
		return;

	pOpt->Comspec32[0] = 0;
	pOpt->Comspec64[0] = 0;

	// Ищем tcc.exe
	if (pOpt->csType == cst_AutoTccCmd)
	{
		HKEY hk;
		BOOL bWin64 = IsWindows64();
		wchar_t szPath[MAX_PATH+1];

		// Если tcc.exe положили в папку с ConEmuC.exe берем его?
		DWORD nExpand = ExpandEnvironmentStrings(L"%ConEmuBaseDir%\\tcc.exe", szPath, countof(szPath));
		if (nExpand && (nExpand < countof(szPath)))
		{
			if (FileExists(szPath))
			{
				wcscpy_c(pOpt->Comspec32, szPath);
				wcscpy_c(pOpt->Comspec64, szPath);
			}
		}

		// On this step - check "Take Command"!
		if (!*pOpt->Comspec32 || !*pOpt->Comspec64)
		{
			// [HKEY_LOCAL_MACHINE\SOFTWARE\JP Software\Take Command 13.0]
			// @="\"C:\\Program Files\\JPSoft\\TCMD13\\tcmd.exe\""
			for (int b = 0; b <= 1; b++)
			{
				// b==0 - 32bit, b==1 - 64bit
				if (b && !bWin64)
					continue;
				bool bFound = false;
				DWORD nOpt = (b == 0) ? (bWin64 ? KEY_WOW64_32KEY : 0) : (bWin64 ? KEY_WOW64_64KEY : 0);
				if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\JP Software", 0, KEY_READ|nOpt, &hk))
				{
					wchar_t szName[MAX_PATH+1]; DWORD nLen;
					for (DWORD k = 0; !bFound && !RegEnumKeyEx(hk, k, szName, &(nLen = countof(szName)-1), 0,0,0,0); k++)
					{
						HKEY hk2;
						if (!RegOpenKeyEx(hk, szName, 0, KEY_READ|nOpt, &hk2))
						{
							// Just in case, check "Path" too
							LPCWSTR rsNames[] = {NULL, L"Path"};

							for (size_t n = 0; n < countof(rsNames); n++)
							{
								ZeroStruct(szPath); DWORD nSize = (countof(szPath)-1)*sizeof(szPath[0]);
								if (!RegQueryValueExW(hk2, rsNames[n], NULL, NULL, (LPBYTE)szPath, &nSize) && *szPath)
								{
									wchar_t* psz, *pszEnd;
									psz = (wchar_t*)Unquote(szPath, true);
									pszEnd = wcsrchr(psz, L'\\');
									if (!pszEnd || lstrcmpi(pszEnd, L"\\tcmd.exe") || !FileExists(psz))
										continue;
									lstrcpyn(pszEnd+1, L"tcc.exe", 8);
									if (FileExists(psz))
									{
										bFound = true;
										if (b == 0)
                                			wcscpy_c(pOpt->Comspec32, psz);
                            			else
                                			wcscpy_c(pOpt->Comspec64, psz);
									}
								}
							} // for (size_t n = 0; n < countof(rsNames); n++)
							RegCloseKey(hk2);
						}
					} //  for, подключи
					RegCloseKey(hk);
				} // L"SOFTWARE\\JP Software"
			} // for (int b = 0; b <= 1; b++)

			// Если установлен TCMD - предпочтительно использовать именно его, независимо от битности
			if (*pOpt->Comspec32 && !*pOpt->Comspec64)
				wcscpy_c(pOpt->Comspec64, pOpt->Comspec32);
			else if (*pOpt->Comspec64 && !*pOpt->Comspec32)
				wcscpy_c(pOpt->Comspec32, pOpt->Comspec64);
		}

		// If "Take Command" not installed - try "TCC/LE"
		if (!*pOpt->Comspec32 || !*pOpt->Comspec64)
		{
			// [HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{16A21882-4138-4ADA-A390-F62DC27E4504}]
			// "DisplayVersion"="13.04.60"
			// "Publisher"="JP Software"
			// "DisplayName"="Take Command 13.0"
			// или
			// "DisplayName"="TCC/LE 13.0"
			// и наконец
			// "InstallLocation"="C:\\Program Files\\JPSoft\\TCMD13\\"
			for (int b = 0; b <= 1; b++)
			{
				// b==0 - 32bit, b==1 - 64bit
				if (b && !bWin64)
					continue;
				if (((b == 0) ? *pOpt->Comspec32 : *pOpt->Comspec64))
					continue; // этот уже нашелся в TCMD

				bool bFound = false;
				DWORD nOpt = (b == 0) ? (bWin64 ? KEY_WOW64_32KEY : 0) : (bWin64 ? KEY_WOW64_64KEY : 0);
				if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", 0, KEY_READ|nOpt, &hk))
				{
					wchar_t szName[MAX_PATH+1]; DWORD nLen;
					for (DWORD n = 0; !bFound && !RegEnumKeyEx(hk, n, szName, &(nLen = countof(szName)-1), 0,0,0,0); n++)
					{
						if (*szName != L'{')
							continue;
						HKEY hk2;
						if (!RegOpenKeyEx(hk, szName, 0, KEY_READ|nOpt, &hk2))
						{
							ZeroStruct(szPath); DWORD nSize = (countof(szPath) - 1)*sizeof(szPath[0]);
							if (!RegQueryValueExW(hk2, L"Publisher", NULL, NULL, (LPBYTE)szPath, &nSize)
								&& !lstrcmpi(szPath, L"JP Software"))
							{
								nSize = (countof(szPath)-12)*sizeof(szPath[0]);
								if (!RegQueryValueExW(hk2, L"InstallLocation", NULL, NULL, (LPBYTE)szPath, &nSize)
									&& *szPath)
								{
									wchar_t* psz, *pszEnd;
									if (szPath[0] == L'"')
									{
										psz = szPath + 1;
										pszEnd = wcschr(psz, L'"');
										if (pszEnd)
											*pszEnd = 0;
									}
									else
									{
										psz = szPath;
									}
									if (*psz)
									{
										pszEnd = psz+lstrlen(psz);
										if (*(pszEnd-1) != L'\\')
											*(pszEnd++) = L'\\';
										lstrcpyn(pszEnd, L"tcc.exe", 8);
										if (FileExists(psz))
										{
											bFound = true;
											if (b == 0)
		                                		wcscpy_c(pOpt->Comspec32, psz);
											else
		                                		wcscpy_c(pOpt->Comspec64, psz);
										}
									}
								}
							}
							RegCloseKey(hk2);
						}
					} // for, подключи
					RegCloseKey(hk);
				} // L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
			} // for (int b = 0; b <= 1; b++)
		}

		// Попытаться "в лоб" из "Program Files"
		if (!*pOpt->Comspec32 && !*pOpt->Comspec64)
		{

			const wchar_t* pszTcmd  = L"C:\\Program Files\\JPSoft\\TCMD13\\tcc.exe";
			const wchar_t* pszTccLe = L"C:\\Program Files\\JPSoft\\TCCLE13\\tcc.exe";
			if (FileExists(pszTcmd))
				wcscpy_c(pOpt->Comspec32, pszTcmd);
			else if (FileExists(pszTccLe))
				wcscpy_c(pOpt->Comspec32, pszTccLe);
		}

		if (*pOpt->Comspec32 && !*pOpt->Comspec64)
			wcscpy_c(pOpt->Comspec64, pOpt->Comspec32);
		else if (*pOpt->Comspec64 && !*pOpt->Comspec32)
			wcscpy_c(pOpt->Comspec32, pOpt->Comspec64);
	} // if (pOpt->csType == cst_AutoTccCmd)

	// С поиском tcc закончили. Теперь, если pOpt->Comspec32/pOpt->Comspec64 остались не заполнены
	// нужно сначала попытаться обработать переменную окружения ComSpec, а потом - просто "cmd.exe"
	if (!*pOpt->Comspec32)
		GetComspecFromEnvVar(pOpt->Comspec32, countof(pOpt->Comspec32), csb_x32);
	if (!*pOpt->Comspec64)
		GetComspecFromEnvVar(pOpt->Comspec64, countof(pOpt->Comspec64), csb_x64);
}
#endif

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

#ifndef CONEMU_MINIMAL
void UpdateComspec(ConEmuComspec* pOpt, bool DontModifyPath /*= false*/)
{
	if (!pOpt)
	{
		_ASSERTE(pOpt!=NULL);
		return;
	}

	if (pOpt->isUpdateEnv && (pOpt->csType != cst_EnvVar))
	{
		//if (pOpt->csType == cst_AutoTccCmd) -- always, if isUpdateEnv
		{
			LPCWSTR pszNew = NULL;
			switch (pOpt->csBits)
			{
			case csb_SameOS:
				pszNew = IsWindows64() ? pOpt->Comspec64 : pOpt->Comspec32;
				break;
			case csb_SameApp:
				pszNew = WIN3264TEST(pOpt->Comspec32,pOpt->Comspec64);
				break;
			case csb_x32:
				pszNew = pOpt->Comspec32;
				break;
			default:
				_ASSERTE(pOpt->csBits==csb_SameOS || pOpt->csBits==csb_SameApp || pOpt->csBits==csb_x32);
				pszNew = NULL;
			}
			if (pszNew && *pszNew)
			{
				#ifdef SHOW_COMSPEC_CHANGE
				wchar_t szCurrent[MAX_PATH]; GetEnvironmentVariable(L"ComSpec", szCurrent, countof(szCurrent));
				if (lstrcmpi(szCurrent, pszNew))
				{
					wchar_t szMsg[MAX_PATH*4], szProc[MAX_PATH] = {}, szPid[MAX_PATH];
					GetModuleFileName(NULL, szProc, countof(szProc));
					_wsprintf(szPid, SKIPLEN(countof(szPid))
						L"PID=%u, '%s'", GetCurrentProcessId(), PointToName(szProc));
					_wsprintf(szMsg, SKIPLEN(countof(szMsg))
						L"Changing %%ComSpec%% in %s\nCur=%s\nNew=%s",
						szPid , szCurrent, pszNew);
					MessageBox(NULL, szMsg, szPid, MB_SYSTEMMODAL);
				}
				#endif

				_ASSERTE(wcschr(pszNew, L'%')==NULL);
				SetEnvVarExpanded(L"ComSpec", pszNew);
			}
		}
	}

	if (pOpt->AddConEmu2Path && !DontModifyPath)
	{
		if ((pOpt->ConEmuBaseDir[0] == 0) || (pOpt->ConEmuExeDir[0] == 0))
		{
			_ASSERTE(pOpt->ConEmuBaseDir[0] != 0);
			_ASSERTE(pOpt->ConEmuExeDir[0] != 0);
		}
		else
		{
			wchar_t* pszCur = GetEnvVar(L"PATH");

			if (!pszCur)
				pszCur = lstrdup(L"");

			DWORD n = lstrlen(pszCur);
			wchar_t* pszUpr = lstrdup(pszCur);
			wchar_t* pszDirUpr = (wchar_t*)malloc(MAX_PATH*sizeof(*pszCur));

			MCHKHEAP;

			if (!pszUpr || !pszDirUpr)
			{
				_ASSERTE(pszUpr && pszDirUpr);
			}
			else
			{
				bool bChanged = false;
				wchar_t* pszAdd = NULL;

				CharUpperBuff(pszUpr, n);

				for (int i = 0; i <= 1; i++)
				{
					switch (i)
					{
					case 0:
						if (!(pOpt->AddConEmu2Path & CEAP_AddConEmuExeDir))
							continue;
						pszAdd = pOpt->ConEmuExeDir;
						break;
					case 1:
						if (!(pOpt->AddConEmu2Path & CEAP_AddConEmuBaseDir))
							continue;
						if (lstrcmp(pOpt->ConEmuExeDir, pOpt->ConEmuBaseDir) == 0)
							continue; // второй раз ту же директорию не добавляем
						pszAdd = pOpt->ConEmuBaseDir;
						break;
					}

					int nDirLen = lstrlen(pszAdd);
					lstrcpyn(pszDirUpr, pszAdd, MAX_PATH);
					CharUpperBuff(pszDirUpr, nDirLen);

					MCHKHEAP;

					// Need to find exact match!
					bool bFound = false;

					LPCWSTR pszFind = wcsstr(pszUpr, pszDirUpr);
					while (pszFind)
					{
						if (pszFind[nDirLen] == L';' || pszFind[nDirLen] == 0)
						{
							// OK, found
							bFound = true;
							break;
						}
						// Next try (may be partial match of subdirs...)
						pszFind = wcsstr(pszFind+nDirLen, pszDirUpr);
					}

					if (!bFound)
					{
						wchar_t* pszNew = lstrmerge(pszAdd, L";", pszCur);
						if (!pszNew)
						{
							_ASSERTE(pszNew && "Failed to reallocate PATH variable");
							break;
						}
						MCHKHEAP;
						SafeFree(pszCur);
						pszCur = pszNew;
						bChanged = true; // Set flag, check next dir
					}
				}

				MCHKHEAP;

				if (bChanged)
				{
					SetEnvironmentVariable(L"PATH", pszCur);
				}
			}

			MCHKHEAP;

			SafeFree(pszUpr);
			SafeFree(pszDirUpr);

			MCHKHEAP;
			SafeFree(pszCur);
		}
	}
}

void SetEnvVarExpanded(LPCWSTR asName, LPCWSTR asValue)
{
	if (!asName || !*asName || !asValue || !*asValue)
	{
		_ASSERTE(asName && *asName && asValue && *asValue);
		return;
	}

	DWORD cchMax = MAX_PATH;
	wchar_t *pszTemp = (wchar_t*)malloc(cchMax*sizeof(*pszTemp));
	if (wcschr(asValue, L'%'))
	{
		DWORD n = ExpandEnvironmentStrings(asValue, pszTemp, cchMax);
		if (n > cchMax)
		{
			cchMax = n+1;
			free(pszTemp);
			pszTemp = (wchar_t*)malloc(cchMax*sizeof(*pszTemp));
			n = ExpandEnvironmentStrings(asValue, pszTemp, cchMax);
		}
		if (n && n <= cchMax)
			asValue = pszTemp;
	}

	SetEnvironmentVariable(asName, asValue);

	SafeFree(pszTemp);
}
#endif // #ifndef CONEMU_MINIMAL

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
			SetEnvironmentVariableW(pszName, pszVal);
		}
		asEnvNameVal = pszNext;
	}
}


#ifndef CONEMU_MINIMAL
int ReadTextFile(LPCWSTR asPath, DWORD cchMax, wchar_t*& rsBuffer, DWORD& rnChars, DWORD& rnErrCode, DWORD DefaultCP /*= 0*/)
{
	HANDLE hFile = CreateFile(asPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

	if (!hFile || hFile == INVALID_HANDLE_VALUE)
	{
		rnErrCode = GetLastError();
		return -1;
	}

	DWORD nSize = GetFileSize(hFile, NULL);

	if (!nSize || nSize >= cchMax)
	{
		rnErrCode = GetLastError();
		CloseHandle(hFile);
		return -2;
	}

	char* pszDataA = (char*)malloc(nSize+5);
	if (!pszDataA)
	{
		_ASSERTE(pszDataA);
		CloseHandle(hFile);
		return -3;
	}
	pszDataA[nSize] = 0; pszDataA[nSize+1] = 0; pszDataA[nSize+2] = 0; pszDataA[nSize+3] = 0; pszDataA[nSize+4] = 0;

	DWORD nRead = 0;
	BOOL  bRead = ReadFile(hFile, pszDataA, nSize, &nRead, 0);
	rnErrCode = GetLastError();
	CloseHandle(hFile);

	if (!bRead || (nRead != nSize))
	{
		free(pszDataA);
		return -4;
	}

	// Опредлить код.страницу файла
	if ((DefaultCP == CP_UTF8) || (pszDataA[0] == '\xEF' && pszDataA[1] == '\xBB' && pszDataA[2] == '\xBF'))
	{
		bool BOM = (pszDataA[0] == '\xEF' && pszDataA[1] == '\xBB' && pszDataA[2] == '\xBF');
		LPCSTR pszNoBom = pszDataA + (BOM ? 3 : 0);
		DWORD  nLenNoBom = nSize - (BOM ? 3 : 0);
		// UTF-8 BOM
		rsBuffer = (wchar_t*)calloc(nSize+2,2);
		if (!rsBuffer)
		{
			_ASSERTE(rsBuffer);
			free(pszDataA);
			return -5;
		}
		int nLen = MultiByteToWideChar(CP_UTF8, 0, pszNoBom, nLenNoBom, rsBuffer, nSize);
		if ((nLen < 0) || (nLen > (int)nSize))
		{
			SafeFree(pszDataA);
			SafeFree(rsBuffer);
			return -6;
		}
		rnChars = nLen;
	}
	else if ((DefaultCP == 1200) || (pszDataA[0] == '\xFF' && pszDataA[1] == '\xFE'))
	{
		bool BOM = (pszDataA[0] == '\xFF' && pszDataA[1] == '\xFE');
		if (!BOM)
		{
			rsBuffer = (wchar_t*)pszDataA;
			pszDataA = NULL;
			rnChars = (nSize >> 1);
		}
		else
		{
			LPCSTR pszNoBom = pszDataA + 2;
			DWORD  nLenNoBom = nSize - 2;
			// CP-1200 BOM
			rsBuffer = lstrdup((wchar_t*)pszNoBom);
			if (!rsBuffer)
			{
				_ASSERTE(rsBuffer);
				SafeFree(pszDataA);
				return -7;
			}
			rnChars = (nLenNoBom >> 1);
		}
	}
	else
	{
		// Plain ANSI
		rsBuffer = (wchar_t*)calloc(nSize+2,2);
		_ASSERTE(rsBuffer);
		int nLen = MultiByteToWideChar(DefaultCP ? DefaultCP : CP_ACP, 0, pszDataA, nSize, rsBuffer, nSize+1);
		if ((nLen < 0) || (nLen > (int)nSize))
		{
			SafeFree(pszDataA);
			SafeFree(rsBuffer);
			return -8;
		}
		rnChars = nLen;
	}

	SafeFree(pszDataA);
	return 0;
}

// Returns negative numbers on errors
int WriteTextFile(LPCWSTR asPath, const wchar_t* asBuffer, int anSrcLen/* = -1*/, DWORD OutCP /*= CP_UTF8*/, bool WriteBOM /*= true*/, LPDWORD rnErrCode /*= NULL*/)
{
	int iRc = 0;
	int iWriteLen = 0;
	DWORD nWritten = 0;
	LPCVOID ptrBuf = NULL;
	char* pszMultibyte = NULL;
	HANDLE hFile = NULL;

	if (OutCP == 1200)
	{
		ptrBuf = asBuffer;
		iWriteLen = (anSrcLen >= 0) ? (anSrcLen * sizeof(*asBuffer)) : (lstrlen(asBuffer) * sizeof(*asBuffer));
	}
	else //if (DefaultCP == CP_UTF8)
	{
		int iMBLen = WideCharToMultiByte(OutCP, 0, asBuffer, anSrcLen, NULL, 0, NULL, NULL);
		if (iMBLen < 0)
		{
			iRc = -2;
			goto wrap;
		}

		pszMultibyte = (char*)malloc(iMBLen);
		if (!pszMultibyte)
		{
			iRc = -3;
			goto wrap;
		}

		iWriteLen = WideCharToMultiByte(OutCP, 0, asBuffer, anSrcLen, pszMultibyte, iMBLen, NULL, NULL);
	}

	if (iWriteLen < 0)
	{
		iRc = -2;
		goto wrap;
	}

	hFile = CreateFile(asPath, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!hFile || (hFile == INVALID_HANDLE_VALUE))
	{
		iRc = -1;
		goto wrap;
	}

	if (WriteBOM)
	{
		BYTE UTF8BOM[] = {'\xEF','\xBB','\xBF'};
		BYTE CP1200BOM[] = {'\xFF','\xFE'};
		BOOL bWrite = TRUE; DWORD nBomSize = 0;

		switch (OutCP)
		{
		case CP_UTF8:
			iRc = (WriteFile(hFile, UTF8BOM, 3, &nBomSize, NULL) && (nBomSize == 3)) ? nBomSize : -4;
			break;
		case 1200:
			iRc = (WriteFile(hFile, CP1200BOM, 2, &nBomSize, NULL) && (nBomSize == 2)) ? nBomSize : -4;
			break;
		}

		if (iRc < 0)
			goto wrap;
	}

	if (!WriteFile(hFile, ptrBuf, (DWORD)iWriteLen, &nWritten, NULL) || ((DWORD)iWriteLen != nWritten))
	{
		iRc = -5;
		goto wrap;
	}

	_ASSERTE(iRc >= 0);
	iRc += nWritten;
wrap:
	if (rnErrCode)
		*rnErrCode = GetLastError();
	if (hFile && (hFile != INVALID_HANDLE_VALUE))
		CloseHandle(hFile);
	SafeFree(pszMultibyte);
	return iRc;
}
#endif
