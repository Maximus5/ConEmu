
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

#ifndef CONEMU_MINIMAL
#include <shlobj.h>
#include "Monitors.h"
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



void getWindowInfo(HWND ahWnd, wchar_t (&rsInfo)[1024])
{
//#ifdef CONEMU_MINIMAL
//	rsInfo[0] = 0;
//#else
	if (!ahWnd)
	{
		wcscpy_c(rsInfo, L"<NULL>");
	}
	else if (!isWindow(ahWnd))
	{
		msprintf(rsInfo, countof(rsInfo), L"0x%08X: Invalid window handle", (DWORD)ahWnd);
	}
	else
	{
		wchar_t szClass[256], szTitle[512];
		
		// Избежать статической линковки для user32.dll
		typedef int (WINAPI* GetClassName_t)(HWND hWnd,LPWSTR lpClassName,int nMaxCount);
		static GetClassName_t GetClassName_f = NULL;
		typedef int (WINAPI* GetWindowText_t)(HWND hWnd, LPWSTR lpString, int nMaxCount);
		static GetWindowText_t GetWindowText_f = NULL;
		
		if (!GetClassName_f)
		{
			HMODULE hUser32 = GetModuleHandle(L"user32.dll");
			if (!hUser32)
			{
				// Скорее всего, user32 уже должен быть загружен, но если вдруг - сильно
				// плохо, если LoadLibrary будет вызываться из DllMain
				_ASSERTE(hUser32!=NULL);
			}
			else
			{
				GetClassName_f = (GetClassName_t)GetProcAddress(hUser32,"GetClassNameW");
				GetWindowText_f = (GetWindowText_t)GetProcAddress(hUser32,"GetWindowTextW");
			}
		}
		
		if (!GetClassName_f || !GetClassName_f(ahWnd, szClass, 256)) wcscpy_c(szClass, L"<GetClassName failed>");
		if (!GetWindowText_f || !GetWindowText_f(ahWnd, szTitle, 512)) szTitle[0] = 0;

		msprintf(rsInfo, countof(rsInfo), L"0x%08X: %s - '%s'", (DWORD)ahWnd, szClass, szTitle);
	}
//#endif
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


IsWindow_t Is_Window = NULL;
BOOL isWindow(HWND hWnd)
{
	if (!hWnd)
		return FALSE;
	if (!Is_Window)
	{
		HMODULE hUser = GetModuleHandle(L"User32.dll");
		if (hUser)
			Is_Window = (IsWindow_t)GetProcAddress(hUser, "IsWindow");
	}
	if (Is_Window)
		return Is_Window(hWnd);
	return TRUE;
}

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

bool FileExistsSearch(wchar_t* rsFilePath, size_t cchPathMax)
{
	if (!rsFilePath || !*rsFilePath)
	{
		_ASSERTEX(rsFilePath && *rsFilePath);
		return false;
	}

	if (FileExists(rsFilePath))
	{
		return true;
	}

	// Переменные окружения
	if (wcschr(rsFilePath, L'%'))
	{
		bool bFound = false;
		wchar_t* pszExpand = ExpandEnvStr(rsFilePath);
		if (pszExpand && FileExists(pszExpand))
		{
			_ASSERTEX(lstrlen(pszExpand) < (INT_PTR)cchPathMax);
			lstrcpyn(rsFilePath, pszExpand, (int)cchPathMax);
			bFound = true;
		}
		SafeFree(pszExpand);

		if (bFound)
		{
			return true;
		}
	}

	// Search "Path"
	LPCWSTR pszSearchFile = rsFilePath;
	LPCWSTR pszSlash = wcsrchr(rsFilePath, L'\\');
	if (pszSlash && ((pszSlash - rsFilePath) >= MAX_PATH))
		return FALSE; // No need to continue, this is invalid path to executable

	wchar_t* pszSearchPath = NULL;
	if (pszSlash)
	{
		if ((pszSearchPath = lstrdup(rsFilePath)) != NULL)
		{
			pszSearchFile = pszSlash + 1;
			pszSearchPath[pszSearchFile - rsFilePath] = 0;
		}
	}

	// попытаемся найти
	wchar_t *pszFilePart;
	wchar_t szFind[MAX_PATH+1];
	LPCWSTR pszExt = PointToExt(rsFilePath);

	DWORD nLen = SearchPath(pszSearchPath, pszSearchFile, pszExt ? NULL : L".exe", countof(szFind), szFind, &pszFilePart);

	SafeFree(pszSearchPath);

	if (nLen && (nLen < countof(szFind)))
	{
		_ASSERTEX(lstrlen(szFind) < (INT_PTR)cchPathMax);
		lstrcpyn(rsFilePath, szFind, (int)cchPathMax);
		return true;
	}

	return false;
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
bool IsFilePath(LPCWSTR asFilePath)
{
	// Если в пути встречаются недопустимые символы
	if (wcschr(asFilePath, L'"') ||
	        wcschr(asFilePath, L'>') ||
	        wcschr(asFilePath, L'<') ||
	        wcschr(asFilePath, L'|')
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

		if (wcschr(pszColon+1, L':'))
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
		//// Только если это (случайно) не conemuc.exe
		//wchar_t* pwszCopy = (wchar_t*)PointToName(szComSpec); //wcsrchr(szComSpec, L'\\');
		////if (!pwszCopy) pwszCopy = szComSpec;

		//if (lstrcmpiW(pwszCopy, L"ConEmuC")==0 || lstrcmpiW(pwszCopy, L"ConEmuC.exe")==0
		//        /*|| lstrcmpiW(pwszCopy, L"ConEmuC64")==0 || lstrcmpiW(pwszCopy, L"ConEmuC64.exe")==0*/)
		//	szComSpec[0] = 0;
		//#pragma warning( pop )

		//if (szComSpec[0])
		//{
		//}

}
#endif

const wchar_t* PointToName(const wchar_t* asFileOrPath)
{
	if (!asFileOrPath)
	{
		_ASSERTE(asFileOrPath!=NULL);
		return NULL;
	}
	const wchar_t* pszFile = wcsrchr(asFileOrPath, L'\\');
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

	const char* pszSlash = strrchr(asFileOrPath, '\\');

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








#ifndef CONEMU_MINIMAL
WARNING("После перехода на альтернативный сервер - должен работать строго в 'стандартном' режиме (mn_StdMode=STD_OUTPUT_HANDLE/STD_INPUT_HANLDE)");
MConHandle::MConHandle(LPCWSTR asName)
{
	mn_StdMode = 0;
	mb_OpenFailed = FALSE; mn_LastError = 0;
	mh_Handle = INVALID_HANDLE_VALUE;
	mpp_OutBuffer = NULL;
	lstrcpynW(ms_Name, asName, 9);
	m_logidx = -1;
	/*
	FAR2 последний
	Conemu последний

	без плагов каких либо

	пропиши одну ассоциацию

	[HKEY_CURRENT_USER\Software\Far2\Associations\Type0]
	"Mask"="*.ini"
	"Description"=""
	"Execute"="@"
	"AltExec"=""
	"View"=""
	"AltView"=""
	"Edit"=""
	"AltEdit"=""
	"State"=dword:0000003f

	ФАР валится по двойному щелчку на INI файле. По Enter - не валится.


	1:31:11.647 Mouse: {10x15} Btns:{L} KeyState: 0x00000000 |DOUBLE_CLICK
	CECMD_CMDSTARTED(Cols=80, Rows=25, Buf=1000, Top=-1)
	SetConsoleSizeSrv.Not waiting for ApplyFinished
	SyncWindowToConsole(Cols=80, Rows=22)
	Current size: Cols=80, Buf=1000
	CECMD_CMDFINISHED(Cols=80, Rows=22, Buf=0, Top=-1)
	SetConsoleSizeSrv.Not waiting for ApplyFinished
	1:31:11.878 Mouse: {10x15} Btns:{} KeyState: 0x00000000
	Current size: Cols=80, Rows=22
	1:31:11.906 Mouse: {10x15} Btns:{} KeyState: 0x00000000 |MOUSE_MOVED
	1:31:11.949 Mouse: {10x15} Btns:{L} KeyState: 0x00000000 |DOUBLE_CLICK
	CECMD_CMDSTARTED(Cols=80, Rows=22, Buf=1000, Top=-1)
	SetConsoleSizeSrv.Not waiting for ApplyFinished
	Current size: Cols=80, Buf=1000
	CECMD_CMDFINISHED(Cols=80, Rows=22, Buf=0, Top=-1)
	SetConsoleSizeSrv.Not waiting for ApplyFinished
	1:31:12.163 Mouse: {10x15} Btns:{} KeyState: 0x00000000
	1:31:12.196 Mouse: {10x15} Btns:{} KeyState: 0x00000000 |MOUSE_MOVED
	Current size: Cols=80, Rows=22
	1:31:12.532 Mouse: {11x15} Btns:{} KeyState: 0x00000000 |MOUSE_MOVED
	1:31:12.545 Mouse: {13x15} Btns:{} KeyState: 0x00000000 |MOUSE_MOVED
	1:31:12.573 Mouse: {14x15} Btns:{} KeyState: 0x00000000 |MOUSE_MOVED
	1:31:12.686 Mouse: {15x15} Btns:{} KeyState: 0x00000000 |MOUSE_MOVED
	1:31:12.779 Mouse: {16x15} Btns:{} KeyState: 0x00000000 |MOUSE_MOVED
	1:31:12.859 Mouse: {16x15} Btns:{L} KeyState: 0x00000000
	1:31:12.876 Mouse: {16x15} Btns:{L} KeyState: 0x00000000 |MOUSE_MOVED
	1:31:12.944 Mouse: {16x15} Btns:{} KeyState: 0x00000000
	1:31:12.956 Mouse: {16x15} Btns:{} KeyState: 0x00000000 |MOUSE_MOVED
	1:31:13.010 Mouse: {16x15} Btns:{L} KeyState: 0x00000000 |DOUBLE_CLICK
	CECMD_CMDSTARTED(Cols=80, Rows=22, Buf=1000, Top=-1)
	SetConsoleSizeSrv.Not waiting for ApplyFinished
	SyncWindowToConsole(Cols=80, Rows=19)
	Current size: Cols=80, Buf=1000
	CECMD_CMDFINISHED(Cols=80, Rows=19, Buf=0, Top=-1)
	1:31:13.150 Mouse: {16x15} Btns:{} KeyState: 0x00000000 |MOUSE_MOVED
	1:31:13.175 Mouse: {16x15} Btns:{L} KeyState: 0x00000000
	1:31:13.206 Mouse: {16x15} Btns:{L} KeyState: 0x00000000 |MOUSE_MOVED
	SetConsoleSizeSrv.Not waiting for ApplyFinished
	1:31:13.240 Mouse: {16x15} Btns:{} KeyState: 0x00000000
	Current size: Cols=80, Rows=191:31:13.317 Mouse: {16x15} Btns:{} KeyState: 0x00000000 |MOUSE_MOVED
	1:31:13.357 Mouse: {16x15} Btns:{L} KeyState: 0x00000000 |DOUBLE_CLICK


	1:31:11 CurSize={80x25} ChangeTo={80x25} RefreshThread :CECMD_SETSIZESYNC
	1:31:11 CurSize={80x1000} ChangeTo={80x25} RefreshThread :CECMD_SETSIZESYNC
	1:31:11 CurSize={80x25} ChangeTo={80x25} RefreshThread :CECMD_SETSIZESYNC
	1:31:11 CurSize={80x1000} ChangeTo={80x22} RefreshThread :CECMD_SETSIZESYNC
	1:31:12 CurSize={80x22} ChangeTo={80x22} RefreshThread :CECMD_SETSIZESYNC
	1:31:12 CurSize={80x1000} ChangeTo={80x22} RefreshThread :CECMD_SETSIZESYNC
	1:31:13 CurSize={80x22} ChangeTo={80x22} RefreshThread :CECMD_SETSIZESYNC
	1:31:13 CurSize={80x1000} ChangeTo={80x19} RefreshThread :CECMD_SETSIZESYNC
	1:31:15 CurSize={80x19} ChangeTo={80x19} RefreshThread :CECMD_SETSIZESYNC
	1:31:15 CurSize={80x1000} ChangeTo={80x19} RefreshThread :CECMD_SETSIZESYNC
	1:31:16 CurSize={80x19} ChangeTo={80x19} RefreshThread :CECMD_SETSIZESYNC
	1:31:16 CurSize={80x1000} ChangeTo={80x19} RefreshThread :CECMD_SETSIZESYNC
	1:31:16 CurSize={80x19} ChangeTo={80x19} RefreshThread :CECMD_SETSIZESYNC
	1:31:16 CurSize={80x19} ChangeTo={80x19} RefreshThread :CECMD_SETSIZESYNC
	1:31:16 CurSize={80x1000} ChangeTo={80x16} RefreshThread :CECMD_SETSIZESYNC
	1:31:25 CurSize={80x16} ChangeTo={80x16} RefreshThread :CECMD_SETSIZESYNC
	1:31:25 CurSize={80x1000} ChangeTo={80x16} RefreshThread :CECMD_SETSIZESYNC




	*/
	//OSVERSIONINFO osv = {sizeof(OSVERSIONINFO)}; GetVersionEx(&osv);
	//if (osv.dwMajorVersion == 6 && osv.dwMinorVersion == 1) {
	//	if (!lstrcmpW(ms_Name, L"CONOUT$"))
	//		mn_StdMode = STD_OUTPUT_HANDLE;
	//	else if (!lstrcmpW(ms_Name, L"CONIN$"))
	//		mn_StdMode = STD_INPUT_HANDLE;
	//}
};

#ifndef __GNUC__
#include <intrin.h>
#else
#define _InterlockedIncrement InterlockedIncrement
#endif

void MConHandle::LogHandle(UINT evt, HANDLE h)
{
	LONG i = (_InterlockedIncrement(&m_logidx) & (HANDLE_BUFFER_SIZE - 1));
	m_log[i].evt = (Event::EventType)evt;
	DEBUGTEST(m_log[i].time = GetTickCount());
	m_log[i].TID = GetCurrentThreadId();
	m_log[i].h = h;
}

MConHandle::~MConHandle()
{
	Close();
};

void MConHandle::SetBufferPtr(HANDLE* ppOutBuffer)
{
	mpp_OutBuffer = ppOutBuffer;
}

MConHandle::operator const HANDLE()
{
	if (mpp_OutBuffer && *mpp_OutBuffer && (*mpp_OutBuffer != INVALID_HANDLE_VALUE))
	{
		LogHandle(Event::e_GetHandlePtr, *mpp_OutBuffer);
		return *mpp_OutBuffer;
	}

	if (mh_Handle == INVALID_HANDLE_VALUE)
	{
		if (mn_StdMode)
		{
			mh_Handle = GetStdHandle(mn_StdMode);
			LogHandle(Event::e_CreateHandleStd, mh_Handle);
		}
		else
		{
			// Чтобы случайно не открыть хэндл несколько раз в разных потоках
			MSectionLock CS; CS.Lock(&mcs_Handle, TRUE);

			// Во время ожидания хэндл мог быт открыт в другом потоке
			if (mh_Handle == INVALID_HANDLE_VALUE)
			{
				mh_Handle = CreateFileW(ms_Name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
										0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

				if (mh_Handle != INVALID_HANDLE_VALUE)
				{
					mb_OpenFailed = FALSE;
				}
				else
				{
					mn_LastError = GetLastError();

					if (!mb_OpenFailed)
					{
						mb_OpenFailed = TRUE; // чтобы ошибка вываливалась только один раз!
						char szErrMsg[512], szNameA[10], szSelfFull[MAX_PATH];
						const char *pszSelf;
						char *pszDot;

						if (!GetModuleFileNameA(0,szSelfFull,MAX_PATH))
						{
							pszSelf = "???";
						}
						else
						{
							pszSelf = strrchr(szSelfFull, '\\');
							if (pszSelf) pszSelf++; else pszSelf = szSelfFull;

							pszDot = strrchr((char*)pszSelf, '.');

							if (pszDot) *pszDot = 0;
						}

						WideCharToMultiByte(CP_OEMCP, 0, ms_Name, -1, szNameA, sizeof(szNameA), 0,0);
						_wsprintfA(szErrMsg, SKIPLEN(countof(szErrMsg)) "%s: CreateFile(%s) failed, ErrCode=0x%08X\n", pszSelf, szNameA, mn_LastError);
						HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);

						if (h && h!=INVALID_HANDLE_VALUE)
						{
							DWORD dwWritten = 0;
							WriteFile(h, szErrMsg, lstrlenA(szErrMsg), &dwWritten, 0);
						}
					}
				}

				LogHandle(Event::e_CreateHandle, mh_Handle);
			}
		}
	}

	LogHandle(Event::e_GetHandle, mh_Handle);
	return mh_Handle;
};

void MConHandle::Close()
{
	// Если установлен указатель на хэндл буфера - закрывать не будем
	if (mpp_OutBuffer && *mpp_OutBuffer && (*mpp_OutBuffer != INVALID_HANDLE_VALUE))
	{
		return;
	}

	if (mh_Handle != INVALID_HANDLE_VALUE)
	{
		if (mn_StdMode)
		{
			LogHandle(Event::e_CloseHandleStd, mh_Handle);
			mh_Handle = INVALID_HANDLE_VALUE;
		}
		else
		{
			HANDLE h = mh_Handle;
			LogHandle(Event::e_CloseHandleStd, h);
			mh_Handle = INVALID_HANDLE_VALUE;
			mb_OpenFailed = FALSE;
			CloseHandle(h);
		}
	}
};
#endif



#ifndef CONEMU_MINIMAL
MEvent::MEvent()
{
	ms_EventName[0] = 0;
	mh_Event = NULL;
	mn_LastError = 0;
}

MEvent::~MEvent()
{
	if (mh_Event)
		Close();
}

void MEvent::Close()
{
	mn_LastError = 0;
	ms_EventName[0] = 0;

	if (mh_Event)
	{
		CloseHandle(mh_Event);
		mh_Event = NULL;
	}
}

void MEvent::InitName(const wchar_t *aszTemplate, DWORD Parm1)
{
	if (mh_Event)
		Close();

	_wsprintf(ms_EventName, SKIPLEN(countof(ms_EventName)) aszTemplate, Parm1);
	mn_LastError = 0;
}

HANDLE MEvent::Open()
{
	if (mh_Event)  // Если уже открыто - сразу вернуть!
		return mh_Event;

	if (ms_EventName[0] == 0)
	{
		_ASSERTE(ms_EventName[0]!=0);
		return NULL;
	}

	mn_LastError = 0;
	mh_Event = OpenEvent(EVENT_MODIFY_STATE|SYNCHRONIZE, FALSE, ms_EventName);

	if (!mh_Event)
		mn_LastError = GetLastError();

	return mh_Event;
}

DWORD MEvent::Wait(DWORD dwMilliseconds, BOOL abAutoOpen/*=TRUE*/)
{
	if (!mh_Event && abAutoOpen)
		Open();

	if (!mh_Event)
		return WAIT_ABANDONED;

	DWORD dwWait = WaitForSingleObject(mh_Event, dwMilliseconds);
#ifdef _DEBUG
	mn_LastError = GetLastError();
#endif
	return dwWait;
}

HANDLE MEvent::GetHandle()
{
	return mh_Event;
}
#endif















#ifndef CONEMU_MINIMAL
MSetter::MSetter(BOOL* st)
{
	DEBUGTEST(ZeroStruct(DataPtr));
	type = st_BOOL;
	mp_BOOLVal = st;

	if (mp_BOOLVal) *mp_BOOLVal = TRUE;
}
MSetter::MSetter(bool* st)
{
	DEBUGTEST(ZeroStruct(DataPtr));
	type = st_bool;
	mp_boolVal = st;

	if (mp_boolVal) *mp_boolVal = true;
}
MSetter::MSetter(DWORD* st, DWORD setValue)
{
	DEBUGTEST(ZeroStruct(DataPtr));
	type = st_DWORD; mdw_OldDwordValue = *st; *st = setValue;
	mdw_DwordVal = st;
}
MSetter::~MSetter()
{
	Unlock();
}
void MSetter::Unlock()
{
	if (type==st_BOOL)
	{
		if (mp_BOOLVal) *mp_BOOLVal = FALSE;
	}
	else if (type==st_bool)
	{
		if (mp_boolVal) *mp_boolVal = false;
	}
	else if (type==st_DWORD)
	{
		if (mdw_DwordVal) *mdw_DwordVal = mdw_OldDwordValue;
	}
}
#endif



MSection::MSection()
{
	mn_TID = 0; mn_Locked = 0; mb_Exclusive = FALSE;
#ifdef _DEBUG
	mn_UnlockedExclusiveTID = 0;
#endif
	ZeroStruct(mn_LockedTID); ZeroStruct(mn_LockedCount);
	InitializeCriticalSection(&m_cs);
	InitializeCriticalSection(&m_lock_cs);
	mh_ReleaseEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	_ASSERTEX(mh_ReleaseEvent!=NULL);

	if (mh_ReleaseEvent) ResetEvent(mh_ReleaseEvent);
	mh_ExclusiveThread = NULL;
};
MSection::~MSection()
{
	DeleteCriticalSection(&m_cs);
	DeleteCriticalSection(&m_lock_cs);

	SafeCloseHandle(mh_ReleaseEvent);
	SafeCloseHandle(mh_ExclusiveThread);
};
void MSection::Process_Lock()
{
	//#ifdef USE_LOCK_SECTION
	EnterCriticalSection(&m_lock_cs);
	//#endif
}
void MSection::Process_Unlock()
{
	//#ifdef USE_LOCK_SECTION
	LeaveCriticalSection(&m_lock_cs);
	//#endif
}
void MSection::ThreadTerminated(DWORD dwTID)
{
	for (size_t i = 1; i < countof(mn_LockedTID); i++)
	{
		if (mn_LockedTID[i] == dwTID)
		{
			mn_LockedTID[i] = 0;

			if (mn_LockedCount[i] != 0)
			{
				_ASSERTEX(mn_LockedCount[i] == 0);
			}

			break;
		}
	}
};
void MSection::AddRef(DWORD dwTID)
{
	Process_Lock();

	#ifdef _DEBUG
	int dbgCurLockCount = 0;
	if (mn_Locked == 0)
	{
		for (int d=1; d<countof(mn_LockedCount); d++)
		{
			_ASSERTE(mn_LockedCount[d]>=0);
			dbgCurLockCount += mn_LockedCount[d];
		}
		if (dbgCurLockCount != 0)
		{
			_ASSERTEX(dbgCurLockCount==0 || mn_Locked);
		}
	}
	#endif

	mn_Locked ++; // увеличиваем счетчик nonexclusive locks
	_ASSERTEX(mn_Locked>0);
	ResetEvent(mh_ReleaseEvent); // На всякий случай сбросим Event
	INT_PTR j = -1; // будет -2, если ++ на существующий, иначе - +1 на пустой

	for (size_t i = 1; i < countof(mn_LockedTID); i++)
	{
		if (mn_LockedTID[i] == dwTID)
		{
			_ASSERTE(mn_LockedCount[i]>=0);
			mn_LockedCount[i] ++;
			j = -2;
			break;
		}
	}

	if (j == -1)
	{
		for (size_t i = 1; i < countof(mn_LockedTID); i++)
		{
			if (mn_LockedTID[i] == 0)
			{
				mn_LockedTID[i] = dwTID;
				_ASSERTE(mn_LockedCount[i]>=0);
				mn_LockedCount[i] ++;
				j = i;
				break;
			}
		}
	}

	if (j == -1)  // Этого быть не должно
	{
		_ASSERTEX(j != -1);
	}

	Process_Unlock();
};
int MSection::ReleaseRef(DWORD dwTID)
{
	Process_Lock();

	_ASSERTEX(mn_Locked>0);
	int nInThreadLeft = 0;

	if (mn_Locked > 0)
		mn_Locked --;

	if (mn_Locked == 0)
	{
		SetEvent(mh_ReleaseEvent); // Больше nonexclusive locks не осталось
	}

	for (size_t i = 1; i < countof(mn_LockedTID); i++)
	{
		if (mn_LockedTID[i] == dwTID)
		{
			if (mn_LockedCount[i] > 0)
			{
				mn_LockedCount[i] --;
			}
			else
			{
				_ASSERTEX(mn_LockedCount[i] > 0);
			}

			if ((nInThreadLeft = mn_LockedCount[i]) == 0)
				mn_LockedTID[i] = 0; // Иначе при динамически создаваемых нитях - 10 будут в момент использованы

			break;
		}
	}

	if (mn_Locked == 0)
	{
		#ifdef _DEBUG
		int dbgCurLockCount = 0;
		for (int d=1; d<countof(mn_LockedCount); d++)
		{
			_ASSERTEX(mn_LockedCount[d]>=0);
			dbgCurLockCount += mn_LockedCount[d];
		}
		if (dbgCurLockCount != 0)
		{
			_ASSERTEX(dbgCurLockCount==0);
		}
		#endif
	}

	Process_Unlock();

	return nInThreadLeft;
};
void MSection::WaitUnlocked(DWORD dwTID, DWORD anTimeout)
{
	DWORD dwStartTick = GetTickCount();
	int nSelfCount = 0;

	for (size_t i = 1; i < countof(mn_LockedTID); i++)
	{
		if (mn_LockedTID[i] == dwTID)
		{
			_ASSERTEX(mn_LockedCount[i]>=0);
			nSelfCount = mn_LockedCount[i];
			break;
		}
	}

	while(mn_Locked > nSelfCount)
	{
#ifdef _DEBUG
		DEBUGSTR(L"!!! Waiting for exclusive access\n");
		DWORD nWait =
#endif
		    WaitForSingleObject(mh_ReleaseEvent, 10);
		DWORD dwCurTick = GetTickCount();
		DWORD dwDelta = dwCurTick - dwStartTick;

		if (anTimeout != (DWORD)-1)
		{
			if (dwDelta > anTimeout)
			{
				#ifndef CSECTION_NON_RAISE
				_ASSERTEX(dwDelta<=anTimeout);
				#endif
				break;
			}
		}

		#ifdef _DEBUG
		else if (dwDelta > 3000)
		{
			#ifndef CSECTION_NON_RAISE
			_ASSERTEX(dwDelta <= 3000);
			#endif
			break;
		}
		#endif
	}
};
bool MSection::MyEnterCriticalSection(DWORD anTimeout)
{
	DWORD nCurTID = GetCurrentThreadId(), nExclWait = -1;
	//EnterCriticalSection(&m_cs);
	// дождаться пока секцию отпустят
	// НАДА. Т.к. может быть задан nTimeout (для DC)
	DWORD dwTryLockSectionStart = GetTickCount(), dwCurrentTick;

	if (!TryEnterCriticalSection(&m_cs))
	{
		Sleep(10);

		while (!TryEnterCriticalSection(&m_cs))
		{
			if ((mn_TID != nCurTID) && mh_ExclusiveThread)
			{
				BOOL lbLocked = FALSE;

				if (mh_ExclusiveThread)
				{
					HANDLE h = mh_ExclusiveThread;
					nExclWait = WaitForSingleObject(h, 0);
					if (nExclWait != WAIT_TIMEOUT)
					{
						// Все, m_cs протух. Его нужно пересоздать

						Process_Lock();

						// Первым выполнить Process_Lock мог другой поток.
						// Нужно проверить хэндл на соответствие, если он другой
						// то на этом шаге уже не дергаться
						if (h == mh_ExclusiveThread)
						{
							_ASSERTEX(FALSE && "Exclusively locked thread was abnormally terminated?");

							SafeCloseHandle(mh_ExclusiveThread);
							CRITICAL_SECTION csNew, csOld;
							InitializeCriticalSection(&csNew);
							csOld = m_cs;
							DeleteCriticalSection(&csOld);
							lbLocked = TryEnterCriticalSection(&csNew);
							m_cs = csNew;

							_ASSERTEX(mn_LockedTID[0] = mn_TID);
							mn_LockedTID[0] = 0;
							if (mn_LockedCount[0] > 0)
							{
								mn_LockedCount[0] --; // на [0] mn_Locked не распространяется
							}

							mn_TID = nCurTID;
						}

						Process_Unlock();
					}
				}

				if (lbLocked)
				{
					break;
				}
			}

			Sleep(10);
			DEBUGSTR(L"TryEnterCriticalSection failed!!!\n");
			dwCurrentTick = GetTickCount();

			if (anTimeout != (DWORD)-1)
			{
				if (((dwCurrentTick - dwTryLockSectionStart) > anTimeout))
				{
					#ifndef CSECTION_NON_RAISE
					_ASSERTEX((dwCurrentTick - dwTryLockSectionStart) <= anTimeout);
					#endif
					return false;
				}
			}

			#ifdef _DEBUG
			else if ((dwCurrentTick - dwTryLockSectionStart) > 3000)
			{
				#ifndef CSECTION_NON_RAISE
				_ASSERTEX((dwCurrentTick - dwTryLockSectionStart) <= 3000);
				#endif
				dwTryLockSectionStart = GetTickCount();
			}
			#endif
		}
	}

	return true;
}
BOOL MSection::Lock(BOOL abExclusive, DWORD anTimeout/*=-1*/)
{
	DWORD dwTID = GetCurrentThreadId();

	// Может эта нить уже полностью заблокирована?
	if (mb_Exclusive && dwTID == mn_TID)
	{
		//111126 возвращался FALSE
		_ASSERTEX(!mb_Exclusive || dwTID != mn_TID);
		return TRUE; // Уже, но Unlock делать не нужно!
	}

	if (!abExclusive)
	{
		if (!mb_Exclusive)
		{
			// Быстрая блокировка, не запрещающая чтение другим нитям.
			// Запрещено только изменение (пересоздание буфера например)
			AddRef(dwTID);
		}
		// Если другая нить уже захватила exclusive
		else //if (mb_Exclusive)
		{
			_ASSERTEX(mb_Exclusive);
			//int nLeft = ReleaseRef(dwTID); // Иначе можем попасть на взаимную блокировку
			//if (nLeft > 0)
			//{
			//	// Нужно избегать этого. Значит выше по стеку в этой нити
			//	// более одного раза был выполнен non exclusive lock
			//	_ASSERTEX(nLeft == 0);
			//}
			#ifdef _DEBUG
			int nInThreadLeft = 0;
			for (int i=1; i<countof(mn_LockedTID); i++)
			{
				if (mn_LockedTID[i] == dwTID)
				{
					_ASSERTEX(mn_LockedCount[i]>=0);
					nInThreadLeft = mn_LockedCount[i];
					break;
				}
			}
			if (nInThreadLeft > 0)
			{
				// Нужно избегать этого. Значит выше по стеку в этой нити
				// более одного раза был выполнен non exclusive lock
				_ASSERTEX(nInThreadLeft == 0);
			}
			#endif


			DEBUGSTR(L"!!! Failed non exclusive lock, trying to use CriticalSection\n");
			bool lbEntered = MyEnterCriticalSection(anTimeout); // дождаться пока секцию отпустят
			// mb_Exclusive может быть выставлен, если сейчас другая нить пытается выполнить exclusive lock
			_ASSERTEX(!mb_Exclusive); // После LeaveCriticalSection mb_Exclusive УЖЕ должен быть сброшен
			AddRef(dwTID); // накрутить счетчик

			// Но поскольку нам нужен только nonexclusive lock
			if (lbEntered)
				LeaveCriticalSection(&m_cs);
		}
	}
	else // abExclusive
	{
		// Требуется Exclusive Lock
		#ifdef _DEBUG
		if (mb_Exclusive)
		{
			// Этого надо стараться избегать
			DEBUGSTR(L"!!! Exclusive lock found in other thread\n");
		}
        #endif

		// Если есть ExclusiveLock (в другой нити) - дождется сама EnterCriticalSection
		#ifdef _DEBUG
		BOOL lbPrev = mb_Exclusive;
		DWORD nPrevTID = mn_TID;
		#endif

		// Сразу установим mb_Exclusive, чтобы в других нитях случайно не прошел nonexclusive lock
		// иначе может получиться, что nonexclusive lock мешает выполнить exclusive lock (ждут друг друга)
		mb_Exclusive = TRUE;
		TODO("Need to check, if MyEnterCriticalSection failed on timeout!\n");

		if (!MyEnterCriticalSection(anTimeout))
		{
			// Пока поставил _ASSERTE, чтобы посмотреть, возникают ли Timeout-ы при блокировке
			_ASSERTEX(FALSE);

			if (mn_TID == 0)  // поскольку заблокировать не удалось - сбросим флажок
				mb_Exclusive = FALSE;

			return FALSE;
		}

		// 120710 - добавил "|| (mn_TID==dwTID)". Это в том случае, если предыдущая ExclusiveThread была прибита.
		_ASSERTEX(!(lbPrev && mb_Exclusive) || (mn_TID==dwTID)); // После LeaveCriticalSection mb_Exclusive УЖЕ должен быть сброшен
		mn_TID = dwTID; // И запомним, в какой нити это произошло

		HANDLE h = mh_ExclusiveThread;
		mh_ExclusiveThread = OpenThread(SYNCHRONIZE, FALSE, dwTID);
		SafeCloseHandle(h);

		mb_Exclusive = TRUE; // Флаг могла сбросить другая нить, выполнившая Leave
		_ASSERTEX(mn_LockedTID[0] == 0 && mn_LockedCount[0] == 0);
		mn_LockedTID[0] = dwTID;
		mn_LockedCount[0] ++; // на [0] mn_Locked не распространяется

		/*if (abRelockExclusive) {
			ReleaseRef(dwTID); // Если до этого был nonexclusive lock
		}*/

		// B если есть nonexclusive locks - дождаться их завершения
		if (mn_Locked)
		{
			//WARNING: Тут есть шанс наколоться, если сначала был NonExclusive, а потом в этой же нити - Exclusive
			// В таких случаях нужно вызывать с параметром abRelockExclusive
			WaitUnlocked(dwTID, anTimeout);
		}
	}

	return TRUE;
};
void MSection::Unlock(BOOL abExclusive)
{
	DWORD dwTID = GetCurrentThreadId();

	if (abExclusive)
	{
		//Process_Lock();

		_ASSERTEX(mn_TID == dwTID && mb_Exclusive);
		_ASSERTEX(mn_LockedTID[0] == dwTID);
		#ifdef _DEBUG
		mn_UnlockedExclusiveTID = dwTID;
		#endif
		mb_Exclusive = FALSE; mn_TID = 0;
		mn_LockedTID[0] = 0;
		if (mn_LockedCount[0] > 0)
		{
			mn_LockedCount[0] --; // на [0] mn_Locked не распространяется
		}
		else
		{
			_ASSERTEX(mn_LockedCount[0]>0);
		}
		LeaveCriticalSection(&m_cs);

		//Process_Unlock();
	}
	else
	{
		ReleaseRef(dwTID);
	}
};
bool MSection::isLockedExclusive()
{
	if (!this || !mb_Exclusive)
		return false;
	DWORD nTID = GetCurrentThreadId();
	if (mn_LockedTID[0] != nTID)
		return false;
	_ASSERTEX(mn_LockedCount[0] > 0);
	return (mn_LockedCount[0] > 0);
};


#ifndef CONEMU_MINIMAL
MSectionThread::MSectionThread(MSection* apS)
{
	mp_S = apS; mn_TID = GetCurrentThreadId();
};
MSectionThread::~MSectionThread()
{
	if (mp_S && mn_TID)
		mp_S->ThreadTerminated(mn_TID);
};
#endif



MSectionLock::MSectionLock()
{
	mp_S = NULL; mb_Locked = FALSE; mb_Exclusive = FALSE;
};
MSectionLock::~MSectionLock()
{
	_ASSERTEX((mb_Locked==FALSE || mb_Locked==TRUE) && (mb_Exclusive==FALSE || mb_Exclusive==TRUE));
	if (mb_Locked)
	{
		DWORD nCurTID = GetCurrentThreadId();

		Unlock();
		
		#ifdef _DEBUG
		wchar_t szDbg[80];		
		msprintf(szDbg, countof(szDbg), L"::~MSectionLock, TID=%u\n", nCurTID);
		DebugString(szDbg);
		#endif
		UNREFERENCED_PARAMETER(nCurTID);
	}
};
BOOL MSectionLock::Lock(MSection* apS, BOOL abExclusive/*=FALSE*/, DWORD anTimeout/*=-1*/)
{
	if (!apS)
		return FALSE;
	if (mb_Locked && apS == mp_S && (abExclusive == mb_Exclusive || mb_Exclusive))
		return TRUE; // уже заблокирован //111126 - возвращался FALSE

	_ASSERTEX(!mb_Locked);
	mb_Exclusive = (abExclusive!=FALSE);
	mp_S = apS;
	mb_Locked = mp_S->Lock(mb_Exclusive, anTimeout);
	return mb_Locked;
};
BOOL MSectionLock::RelockExclusive(DWORD anTimeout/*=-1*/)
{
	if (!mp_S)
	{
		_ASSERTEX(mp_S!=NULL);
		return FALSE;
	}
	if (mb_Locked && mb_Exclusive)
		return TRUE;  // уже

	// Чистый ReLock делать нельзя. Виснут другие нити, которые тоже запросили ReLock
	Unlock();
	mb_Exclusive = TRUE;
	mb_Locked = mp_S->Lock(mb_Exclusive, anTimeout);
	return mb_Locked;
};
void MSectionLock::Unlock()
{
	if (mp_S && mb_Locked)
	{
		#ifdef _DEBUG
		DWORD nCurTID = GetCurrentThreadId();
		wchar_t szDbg[80];
		if (mb_Exclusive)
			msprintf(szDbg, countof(szDbg), L"::Unlock(), TID=%u\n", nCurTID);
		else
			szDbg[0] = 0;
		#endif

		_ASSERTEX(mb_Exclusive || mp_S->mn_Locked>0);
		mp_S->Unlock(mb_Exclusive);
		mb_Locked = FALSE;

		#ifdef _DEBUG
		if (*szDbg)
		{
			DebugString(szDbg);
		}
		#endif
	}
};
BOOL MSectionLock::isLocked(BOOL abExclusiveOnly/*=FALSE*/)
{
	return (mp_S!=NULL) && mb_Locked && (!abExclusiveOnly || mb_Exclusive);
};



MSectionLockSimple::MSectionLockSimple()
{
	mp_S = NULL;
	mb_Locked = false;
	#ifdef _DEBUG
	mn_LockTID = mn_LockTick = 0;
	#endif
}

MSectionLockSimple::~MSectionLockSimple()
{
	if (mb_Locked)
	{
		Unlock();
	}
}

BOOL MSectionLockSimple::Lock(CRITICAL_SECTION* apS, DWORD anTimeout/*=-1*/)
{
	if (mb_Locked && (mp_S != apS))
		Unlock();

	mp_S = apS;

	if (!mp_S)
	{
		_ASSERTEX(apS);
		return FALSE;
	}

	_ASSERTEX(!mb_Locked);
	
	bool bLocked = false;
	DWORD nStartTick = GetTickCount();
	DWORD nDelta;
#if 0
	EnterCriticalSection(apS);

	bLocked = mb_Locked = true;

	nDelta = GetTickCount() - nStartTick;
	if (nDelta >= anTimeout)
	{
		_ASSERTEX(FALSE && "Failed to lock CriticalSection, timeout");
	}
#else
	while (true)
	{
		if (TryEnterCriticalSection(apS))
		{
			bLocked = mb_Locked = true;
			#ifdef _DEBUG
			mn_LockTID = GetCurrentThreadId();
			mn_LockTick = GetTickCount();
			#endif
			break;
		}

		if (anTimeout != (DWORD)-1)
		{
			nDelta = GetTickCount() - nStartTick;
			if (nDelta >= anTimeout)
			{
				_ASSERTEX(FALSE && "Failed to lock CriticalSection, timeout");
				break;
			}
		}

		Sleep(1);
	}
#endif

	return bLocked;
}

void MSectionLockSimple::Unlock()
{
	if (mb_Locked)
	{
		if (mp_S)
			LeaveCriticalSection(mp_S);
		mb_Locked = false;
	}
}

BOOL MSectionLockSimple::isLocked()
{
	return mb_Locked;
}



#ifndef CONEMU_MINIMAL
MFileLog::MFileLog(LPCWSTR asName, LPCWSTR asDir /*= NULL*/, DWORD anPID /*= 0*/)
{
	InitializeCriticalSection(&mcs_Lock);
	mh_LogFile = NULL;
	ms_FilePathName = NULL;
	ms_DefPath = (asDir && *asDir) ? lstrdup(asDir) : NULL;
	InitFileName(asName, anPID);
}

HRESULT MFileLog::InitFileName(LPCWSTR asName /*= NULL*/, DWORD anPID /*= 0*/)
{
	if (!anPID) anPID = GetCurrentProcessId();

	if (!asName || !*asName) asName = L"LogFile";

	size_t cchMax = lstrlen(asName)+16;
	ms_FileName = (wchar_t*)malloc(cchMax*sizeof(*ms_FileName));
	if (!ms_FileName)
	{
		_ASSERTEX(ms_FileName);
		return E_UNEXPECTED;
	}

	_wsprintf(ms_FileName, SKIPLEN(cchMax) L"%s-%u.log", asName, anPID);

	return S_OK;

	//wchar_t szTemp[MAX_PATH]; szTemp[0] = 0;
	//GetTempPath(MAX_PATH-16, szTemp);

	//if (!asDir || !*asDir)
	//{
	//	wcscat_c(szTemp, L"ConEmuLog");
	//	CreateDirectoryW(szTemp, NULL);
	//	wcscat_c(szTemp, L"\\");
	//	asDir = szTemp;
	//}

	//int nDirLen = lstrlenW(asDir);
	//wchar_t szFile[MAX_PATH*2];
	//_wsprintf(szFile, SKIPLEN(countof(szFile)) L"%s-%u.log", asName ? asName : L"LogFile", anPID);
	//int nFileLen = lstrlenW(szFile);
	//int nCchMax = nDirLen+nFileLen+3;
	//ms_FilePathName = (wchar_t*)calloc(nCchMax,2);
	//_wcscpy_c(ms_FilePathName, nCchMax, asDir);

	//if (nDirLen > 0 && ms_FilePathName[nDirLen-1] != L'\\')
	//	_wcscat_c(ms_FilePathName, nCchMax, L"\\");

	//_wcscat_c(ms_FilePathName, nCchMax, szFile);
}
MFileLog::~MFileLog()
{
	CloseLogFile();
	SafeFree(ms_DefPath);
	DeleteCriticalSection(&mcs_Lock);
}
void MFileLog::CloseLogFile()
{
	SafeCloseHandle(mh_LogFile);

	SafeFree(ms_FilePathName);
	SafeFree(ms_FileName);
}
// Returns 0 if succeeded, otherwise - GetLastError() code
HRESULT MFileLog::CreateLogFile(LPCWSTR asName /*= NULL*/, DWORD anPID /*= 0*/, DWORD anLevel /*= 0*/)
{
	if (!this)
		return -1;

	// ms_FileName мог быть проинициализирован в конструкторе, поэтому CloseLogFile не зовем
	if (mh_LogFile && mh_LogFile != INVALID_HANDLE_VALUE)
	{
		SafeCloseHandle(mh_LogFile);
	}

	if (asName)
	{
		// А вот если указали новое имя - нужно все передернуть
		CloseLogFile();

		HRESULT hr = InitFileName(asName, anPID);
		if (FAILED(hr))
			return -1;
	}

	if (!ms_FileName || !*ms_FileName)
	{
		return -1;
	}

	DWORD dwErr = (DWORD)-1;

	wchar_t szLevel[16] = L"";
	if (anLevel > 0)
		_wsprintf(szLevel, SKIPLEN(countof(szLevel)) L"[%u]", anLevel);
	wchar_t szVer4[8] = L"";
	lstrcpyn(szVer4, WSTRING(MVV_4a), countof(szVer4));
	wchar_t szConEmu[64];
	_wsprintf(szConEmu, SKIPLEN(countof(szConEmu)) L"ConEmu %u%02u%02u%s%s[%s%s] log%s",
		MVV_1, MVV_2, MVV_3, szVer4[0]&&szVer4[1]?L"-":L"", szVer4,
		WIN3264TEST(L"32",L"64"), RELEASEDEBUGTEST(L"",L"D"), szLevel);

	if (!ms_FilePathName)
	{
		// Первое открытие. нужно сформировать путь к лог-файлу
		mh_LogFile = NULL;

		size_t cchMax, cchNamLen = lstrlen(ms_FileName);

		if (ms_DefPath && *ms_DefPath)
		{
			size_t cchDirLen = lstrlen(ms_DefPath);
            cchMax = cchDirLen + cchNamLen + 3;

            ms_FilePathName = (wchar_t*)calloc(cchMax,sizeof(*ms_FilePathName));
            if (!ms_FilePathName)
            	return -1;

        	_wcscpy_c(ms_FilePathName, cchMax, ms_DefPath);
        	if (ms_DefPath[cchMax-1] != L'\\')
        		_wcscat_c(ms_FilePathName, cchMax, L"\\");
    		_wcscat_c(ms_FilePathName, cchMax, ms_FileName);


    		mh_LogFile = CreateFileW(ms_FilePathName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    		// Нет прав на запись в текущую папку?
			if (mh_LogFile == INVALID_HANDLE_VALUE)
			{
				dwErr = GetLastError();
				if (dwErr == ERROR_ACCESS_DENIED/*5*/)
					mh_LogFile = NULL;
				SafeFree(ms_FilePathName);
			}
		}

		if (mh_LogFile == NULL)
		{
			wchar_t szDesktop[MAX_PATH+1] = L"";
			if (S_OK == SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY|CSIDL_FLAG_CREATE, NULL, 0/*SHGFP_TYPE_CURRENT*/, szDesktop))
			{
				size_t cchDirLen = lstrlen(szDesktop);
	            cchMax = cchDirLen + cchNamLen + 32;

	            ms_FilePathName = (wchar_t*)calloc(cchMax,sizeof(*ms_FilePathName));
	            if (!ms_FilePathName)
	            	return -1;
				
	        	_wcscpy_c(ms_FilePathName, cchMax, szDesktop);
	        	_wcscat_c(ms_FilePathName, cchMax, (szDesktop[cchDirLen-1] != L'\\') ? L"\\ConEmuLogs" : L"ConEmuLogs");
				CreateDirectory(ms_FilePathName, NULL);
				_wcscat_c(ms_FilePathName, cchMax, L"\\");
	    		_wcscat_c(ms_FilePathName, cchMax, ms_FileName);

	    		mh_LogFile = CreateFileW(ms_FilePathName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	    		// Нет прав на запись в текущую папку?
				if (mh_LogFile == INVALID_HANDLE_VALUE)
				{
					dwErr = GetLastError();
					mh_LogFile = NULL;
				}
			}
		}
	}
	else
	{
		if (!ms_FilePathName || !*ms_FilePathName)
			return -1;

		mh_LogFile = CreateFileW(ms_FilePathName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (mh_LogFile == INVALID_HANDLE_VALUE)
		{
			mh_LogFile = NULL;
			dwErr = GetLastError();
		}
	}

	if (!mh_LogFile || (mh_LogFile == INVALID_HANDLE_VALUE))
	{
		mh_LogFile = NULL;
		return (dwErr ? dwErr : -1);
	}

	LogString(szConEmu, true);

	return 0; // OK
}
LPCWSTR MFileLog::GetLogFileName()
{
	if (!this)
		return L"<NULL>";

	return (ms_FilePathName ? ms_FilePathName : L"<NullFileName>");
}
void MFileLog::LogString(LPCSTR asText, bool abWriteTime /*= true*/, LPCSTR asThreadName /*= NULL*/, bool abNewLine /*= true*/, UINT anCP /*= CP_ACP*/)
{
	if (!this)
		return;

	if (mh_LogFile == INVALID_HANDLE_VALUE || mh_LogFile == NULL)
		return;

	wchar_t szInfo[460]; szInfo[0] = 0;
	wchar_t* pszBuf = szInfo;
	wchar_t szThread[32]; szThread[0] = 0;

	if (asText)
	{
		UINT nLen = lstrlenA(asText);
		if (nLen < countof(szInfo))
		{
			pszBuf = szInfo;
		}
		else
		{
			//Too large, need more memory
			pszBuf = (wchar_t*)malloc((nLen+1)*sizeof(*pszBuf));
		}

		if (pszBuf)
		{
			MultiByteToWideChar(anCP, 0, asText, -1, pszBuf, nLen);
			pszBuf[nLen] = 0;
		}
	}

	if (asThreadName)
	{
		MultiByteToWideChar(anCP, 0, asThreadName, -1, szThread, countof(szThread));
		szThread[countof(szThread)-1] = 0;
	}

	LogString(pszBuf, abWriteTime, szThread, abNewLine);

	if (pszBuf && (pszBuf != szInfo))
	{
		SafeFree(pszBuf);
	}
}
void MFileLog::LogString(LPCWSTR asText, bool abWriteTime /*= true*/, LPCWSTR asThreadName /*= NULL*/, bool abNewLine /*= true*/)
{
	if (!this) return;

	if (!asText) return;

	//DEBUGTEST(abWriteTime = false);

	size_t cchTextLen = asText ? lstrlen(asText) : 0;
	size_t cchThrdLen = asThreadName ? lstrlen(asThreadName) : 0;
	size_t cchMax = (cchTextLen + cchThrdLen)*3 + 32;
	char szBuffer[1024];
	char *pszBuffer, *pszTemp = NULL;
	if (cchMax < countof(szBuffer))
	{
		pszBuffer = szBuffer;
	}
	else
	{
		pszTemp = pszBuffer = (char*)malloc(cchMax);
	}
	if (!pszBuffer)
		return;

	size_t cchCur = 0, dwLen;

	if (abWriteTime)
	{
		SYSTEMTIME st; GetLocalTime(&st);
		char szTime[32];
		_wsprintfA(szTime, SKIPLEN(countof(szTime)) "%i:%02i:%02i.%03i ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
		dwLen = lstrlenA(szTime);
		memmove(pszBuffer+cchCur, szTime, dwLen);
		cchCur += dwLen;
	}

	if (asThreadName && *asThreadName)
	{
		int nLen = WideCharToMultiByte(CP_UTF8, 0, asThreadName, -1, pszBuffer+cchCur, (int)cchThrdLen*3+1, NULL, NULL);
		if (nLen > 1) // including terminating null char
			cchCur += (nLen-1);
		pszBuffer[cchCur++] = L' ';
	}

	if (asText && *asText)
	{
		int nLen = WideCharToMultiByte(CP_UTF8, 0, asText, -1, pszBuffer+cchCur, (int)cchTextLen*3+1, NULL, NULL);
		if (nLen > 1) // including terminating null char
			cchCur += (nLen-1);
	}

	if (abNewLine)
	{
		memmove(pszBuffer+cchCur, "\r\n", 2);
		cchCur += 2;
	}

	pszBuffer[cchCur] = 0;

	if (mh_LogFile)
	{
		MSectionLockSimple lock; lock.Lock(&mcs_Lock);
		DWORD dwLen = (DWORD)cchCur;
		WriteFile(mh_LogFile, pszBuffer, dwLen, &dwLen, 0);
		FlushFileBuffers(mh_LogFile);
	}
	else
	{
		#ifdef _DEBUG
		DEBUGSTRLOG(asText);
		#endif
	}

	SafeFree(pszTemp);

#if 0
	if (!this)
		return;

	if (mh_LogFile == INVALID_HANDLE_VALUE || mh_LogFile == NULL)
		return;

	wchar_t szInfo[512]; szInfo[0] = 0;
	SYSTEMTIME st; GetLocalTime(&st);
	_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"%i:%02i:%02i.%03i ",
	          st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	int nCur = lstrlenW(szInfo);

	if (asThreadName && *asThreadName)
	{
		lstrcpynW(szInfo+nCur, asThreadName, 32); //-V112
		nCur += lstrlenW(szInfo+nCur);
	}

	if (asText && *asText)
	{
		lstrcpynW(szInfo+nCur, asText, 508-nCur);
		nCur += lstrlenW(szInfo+nCur);
	}

	wcscpy_add(nCur, szInfo, L"\r\n");
	nCur += 2;
	DWORD dwLen = 0;
	WriteFile(mh_LogFile, szInfo, nCur*2, &dwLen, 0);
	FlushFileBuffers(mh_LogFile);
#endif
}
BOOL MFileLog::MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	MFileLog* p = (MFileLog*)dwData;
	wchar_t szInfo[200];

	MONITORINFO mi = {sizeof(mi)};
	GetMonitorInfo(hMonitor, &mi);

	_wsprintf(szInfo, SKIPLEN(countof(szInfo))
		L"  %08X: {%i,%i}-{%i,%i} (%ix%i), Working: {%i,%i}-{%i,%i} (%ix%i)%s",
		(DWORD)(DWORD_PTR)hMonitor,
		lprcMonitor->left, lprcMonitor->top, lprcMonitor->right, lprcMonitor->bottom, lprcMonitor->right-lprcMonitor->left, lprcMonitor->bottom-lprcMonitor->top,
		mi.rcWork.left, mi.rcWork.top, mi.rcWork.right, mi.rcWork.bottom, mi.rcWork.right-mi.rcWork.left, mi.rcWork.bottom-mi.rcWork.top,
		(mi.dwFlags & MONITORINFOF_PRIMARY) ? L" <<== Primary" : L"");

	p->LogString(szInfo, false, NULL, true);

	return TRUE;
}
void MFileLog::LogStartEnv(CEStartupEnv* apStartEnv)
{
	if (!apStartEnv || (apStartEnv->cbSize < sizeof(*apStartEnv)))
	{
		LogString(L"LogStartEnv failed, invalid apStartEnv");
		return;
	}

	// Пишем инфу
	wchar_t szSI[MAX_PATH*4], szDesktop[128] = L"", szTitle[128] = L"";
	lstrcpyn(szDesktop, apStartEnv->si.lpDesktop ? apStartEnv->si.lpDesktop : L"", countof(szDesktop));
	lstrcpyn(szTitle, apStartEnv->si.lpTitle ? apStartEnv->si.lpTitle : L"", countof(szTitle));

	BOOL bWin64 = IsWindows64();

	#pragma warning(disable: 4996)
	OSVERSIONINFOEXW osv = {sizeof(osv)};
	GetVersionEx((OSVERSIONINFOW*)&osv);
	#pragma warning(default: 4996)

	LPCWSTR pszReactOS = osv.szCSDVersion + lstrlen(osv.szCSDVersion) + 1;
	if (!*pszReactOS)
		pszReactOS++;

	HWND hConWnd = GetConsoleWindow();

	_wsprintf(szSI, SKIPLEN(countof(szSI)) L"Startup info\r\n"
		L"\tOsVer: %u.%u.%u.x%u, Product: %u, SP: %u.%u, Suite: 0x%X, SM_SERVERR2: %u\r\n"
		L"\tCSDVersion: %s, ReactOS: %u (%s), Rsrv: %u\r\n"
		L"\tDBCS: %u, WINE: %u, PE: %u, ACP: %u, OEMCP: %u\r\n"
		L"\tDesktop: %s\r\n\tTitle: %s\r\n\tSize: {%u,%u},{%u,%u}\r\n"
		L"\tFlags: 0x%08X, ShowWindow: %u, ConHWnd: 0x%08X\r\n"
		L"\tHandles: 0x%08X, 0x%08X, 0x%08X"
		,
		osv.dwMajorVersion, osv.dwMinorVersion, osv.dwBuildNumber, bWin64 ? 64 : 32,
		osv.wProductType, osv.wServicePackMajor, osv.wServicePackMinor, osv.wSuiteMask, GetSystemMetrics(89/*SM_SERVERR2*/),
		osv.szCSDVersion, apStartEnv->bIsReactOS, pszReactOS, osv.wReserved,
		apStartEnv->bIsDbcs, apStartEnv->bIsWine, apStartEnv->bIsWinPE,
		apStartEnv->nAnsiCP, apStartEnv->nOEMCP,
		szDesktop, szTitle,
		apStartEnv->si.dwX, apStartEnv->si.dwY, apStartEnv->si.dwXSize, apStartEnv->si.dwYSize,
		apStartEnv->si.dwFlags, (DWORD)apStartEnv->si.wShowWindow, (DWORD)hConWnd,
		(DWORD)apStartEnv->si.hStdInput, (DWORD)apStartEnv->si.hStdOutput, (DWORD)apStartEnv->si.hStdError
		);
	LogString(szSI, true);

	if (hConWnd)
	{
		szSI[0] = 0;
		HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");
		typedef BOOL (__stdcall *FGetConsoleKeyboardLayoutName)(wchar_t*);
		FGetConsoleKeyboardLayoutName pfnGetConsoleKeyboardLayoutName = hKernel ? (FGetConsoleKeyboardLayoutName)GetProcAddress(hKernel, "GetConsoleKeyboardLayoutNameW") : NULL;
		if (pfnGetConsoleKeyboardLayoutName)
		{
			ZeroStruct(szTitle);
			if (pfnGetConsoleKeyboardLayoutName(szTitle))
				_wsprintf(szSI, SKIPLEN(countof(szSI)) L"Active console layout name: '%s'", szTitle);
		}
		if (!*szSI)
			_wsprintf(szSI, SKIPLEN(countof(szSI)) L"Active console layout: Unknown, code=%u", GetLastError());
		LogString(szSI, false);
	}

	// Текущий HKL (он может отличаться от GetConsoleKeyboardLayoutNameW
	HKL hkl[32] = {NULL};
	hkl[0] = GetKeyboardLayout(0);
	_wsprintf(szSI, SKIPLEN(countof(szSI)) L"Active HKL: " WIN3264TEST(L"0x%08X",L"0x%08X%08X"), WIN3264WSPRINT((DWORD_PTR)hkl[0]));
	LogString(szSI, false);
	// Установленные в системе HKL
	UINT nHkl = GetKeyboardLayoutList(countof(hkl), hkl);
	if (!nHkl || (nHkl > countof(hkl)))
	{
		_wsprintf(szSI, SKIPLEN(countof(szSI)) L"GetKeyboardLayoutList failed, code=%u", GetLastError());
		LogString(szSI, false);
	}
	else
	{
		wcscpy_c(szSI, L"GetKeyboardLayoutList:");
		size_t iLen = lstrlen(szSI);
		_ASSERTE((iLen + 1 + nHkl*17)<countof(szSI));

		for (UINT i = 0; i < nHkl; i++)
		{
			_wsprintf(szSI+iLen, SKIPLEN(18) WIN3264TEST(L" 0x%08X",L" 0x%08X%08X"), WIN3264WSPRINT((DWORD_PTR)hkl[i]));
			iLen += lstrlen(szSI+iLen);
		}
		LogString(szSI, false);
	}
	
	LogString("CmdLine: ", false, NULL, false);
	LogString(apStartEnv->pszCmdLine ? apStartEnv->pszCmdLine : L"<NULL>", false, NULL, true);
	LogString("ExecMod: ", false, NULL, false);
	LogString(apStartEnv->pszExecMod ? apStartEnv->pszExecMod : L"<NULL>", false, NULL, true);
	LogString("WorkDir: ", false, NULL, false);
	LogString(apStartEnv->pszWorkDir ? apStartEnv->pszWorkDir : L"<NULL>", false, NULL, true);
	LogString("PathEnv: ", false, NULL, false);
	LogString(apStartEnv->pszPathEnv ? apStartEnv->pszPathEnv : L"<NULL>", false, NULL, true);
	LogString("ConFont: ", false, NULL, false);
	LogString(apStartEnv->pszRegConFonts ? apStartEnv->pszRegConFonts : L"<NULL>", false, NULL, true);

	// szSI уже не используется, можно

	HWND hFore = GetForegroundWindow();
	RECT rcFore = {0}; if (hFore) GetWindowRect(hFore, &rcFore);
	if (hFore) GetClassName(hFore, szDesktop, countof(szDesktop)-1); else szDesktop[0] = 0;
	if (hFore) GetWindowText(hFore, szTitle, countof(szTitle)-1); else szTitle[0] = 0;
	_wsprintf(szSI, SKIPLEN(countof(szSI)) L"Foreground: x%08X {%i,%i}-{%i,%i} '%s' - %s", (DWORD)(DWORD_PTR)hFore, rcFore.left, rcFore.top, rcFore.right, rcFore.bottom, szDesktop, szTitle);
	LogString(szSI, false, NULL, true);

	POINT ptCur = {0}; GetCursorPos(&ptCur);
	_wsprintf(szSI, SKIPLEN(countof(szSI)) L"Cursor: {%i,%i}", ptCur.x, ptCur.y);
	LogString(szSI, false, NULL, true);

	HDC hdcScreen = GetDC(NULL);
	int nBits = GetDeviceCaps(hdcScreen,BITSPIXEL);
	int nPlanes = GetDeviceCaps(hdcScreen,PLANES);
	int nAlignment = GetDeviceCaps(hdcScreen,BLTALIGNMENT);
	int nVRefr = GetDeviceCaps(hdcScreen,VREFRESH);
	int nShadeCaps = GetDeviceCaps(hdcScreen,SHADEBLENDCAPS);
	int nDevCaps = GetDeviceCaps(hdcScreen,RASTERCAPS);
	_wsprintf(szSI, SKIPLEN(countof(szSI))
		L"Display: bpp=%i, planes=%i, align=%i, vrefr=%i, shade=x%08X, rast=x%08X",
		nBits, nPlanes, nAlignment, nVRefr, nShadeCaps, nDevCaps);
	ReleaseDC(NULL, hdcScreen);
	LogString(szSI, false, NULL, true);

	LogString("Monitors:", false, NULL, true);
	EnumDisplayMonitors(NULL, NULL, MFileLog::MonitorEnumProc, (LPARAM)this);

	LogString("Modules:", false, NULL, true);
	HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
	MODULEENTRY32W mi = {sizeof(mi)};
	if (h && (h != INVALID_HANDLE_VALUE))
	{
		if (Module32First(h, &mi))
		{
			do
			{
				DWORD_PTR ptrStart = (DWORD_PTR)mi.modBaseAddr;
				DWORD_PTR ptrEnd = (DWORD_PTR)mi.modBaseAddr + (DWORD_PTR)(mi.modBaseSize ? (mi.modBaseSize-1) : 0);
				_wsprintf(szSI, SKIPLEN(countof(szSI))
					L"  " WIN3264TEST(L"%08X-%08X",L"%08X%08X-%08X%08X") L" %8X %s",
					WIN3264WSPRINT(ptrStart), WIN3264WSPRINT(ptrEnd), mi.modBaseSize, mi.szExePath);
				LogString(szSI, false, NULL, true);
			} while (Module32Next(h, &mi));
		}
		CloseHandle(h);
	}
}
#endif


#ifndef CONEMU_MINIMAL
/* *********************************** */
CToolTip::CToolTip()
{
	mh_Tip = mh_Ball = NULL;
	mpsz_LastTip = NULL;
	mn_LastTipCchMax = 0;
	memset(&mti_Tip,  0, sizeof(mti_Tip));
	memset(&mti_Ball, 0, sizeof(mti_Ball));
	mb_LastTipBalloon = -1;
}
CToolTip::~CToolTip()
{
	if (mh_Tip)
	{
		if (IsWindow(mh_Tip))
			DestroyWindow(mh_Tip);
		mh_Tip = NULL;
	}
	if (mh_Ball)
	{
		if (IsWindow(mh_Ball))
			DestroyWindow(mh_Ball);
		mh_Ball = NULL;
	}
	if (mpsz_LastTip)
	{
		free(mpsz_LastTip);
		mpsz_LastTip = NULL;
	}
}
void CToolTip::ShowTip(HWND ahOwner, HWND ahControl, LPCWSTR asText, BOOL abBalloon, POINT pt, HINSTANCE hInstance)
{
	HideTip();

	if (!asText || !*asText)
		return;

	
	int nTipLen = lstrlen(asText);
	if (!mpsz_LastTip || (nTipLen >= mn_LastTipCchMax))
	{
		if (mpsz_LastTip)
			free(mpsz_LastTip);
		mn_LastTipCchMax = nTipLen + 1;
		mpsz_LastTip = (wchar_t*)malloc(mn_LastTipCchMax*sizeof(wchar_t));
	}
	_wcscpy_c(mpsz_LastTip, mn_LastTipCchMax, asText);
	
	TOOLINFO *pti = abBalloon ? (&mti_Ball) : (&mti_Tip);
	
	if (abBalloon)
	{
		if (!mh_Ball || !IsWindow(mh_Ball))
		{
			mh_Ball = CreateWindowEx ( WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
			                           WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON | TTS_NOPREFIX,
			                           CW_USEDEFAULT, CW_USEDEFAULT,
			                           CW_USEDEFAULT, CW_USEDEFAULT,
			                           ahOwner, NULL,
			                           hInstance/*g_hInstance*/, NULL);
			SetWindowPos(mh_Ball, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
			// Set up tool information.
			// In this case, the "tool" is the entire parent window.
			pti->cbSize = 44; // был sizeof(TOOLINFO);
			pti->uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
			pti->hwnd = ahControl;
			pti->hinst = hInstance;
			static wchar_t szAsterisk[] = L"*"; // eliminate GCC warning
			pti->lpszText = szAsterisk;
			pti->uId = (UINT_PTR)ahControl;
			GetClientRect(ahControl, &(pti->rect));
			// Associate the ToolTip with the tool window.
			SendMessage(mh_Ball, TTM_ADDTOOL, 0, (LPARAM)pti);
			// Allow multiline
			SendMessage(mh_Ball, TTM_SETMAXTIPWIDTH, 0, (LPARAM)300);
		}
	}
	else
	{
		if (!mh_Tip || !IsWindow(mh_Tip))
		{
			mh_Tip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
			                         WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON | TTS_NOPREFIX,
			                         CW_USEDEFAULT, CW_USEDEFAULT,
			                         CW_USEDEFAULT, CW_USEDEFAULT,
			                         ahOwner, NULL,
			                         hInstance, NULL);
			SetWindowPos(mh_Tip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
			SendMessage(mh_Tip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 30000);
		}
	}
	
	mb_LastTipBalloon = abBalloon;
	
	HWND hTip = abBalloon ? (mh_Ball) : (mh_Tip);
	if (!hTip)
	{
		_ASSERTE(hTip != NULL);
		return;
	}
	
	pti->lpszText = mpsz_LastTip;

	SendMessage(hTip, TTM_UPDATETIPTEXT, 0, (LPARAM)pti);
	//RECT rcControl; GetWindowRect(GetDlgItem(hDlg, nCtrlID), &rcControl);
	//int ptx = rcControl.right - 10;
	//int pty = (rcControl.top + rcControl.bottom) / 2;
	SendMessage(hTip, TTM_TRACKPOSITION, 0, MAKELONG(pt.x,pt.y));
	SendMessage(hTip, TTM_TRACKACTIVATE, TRUE, (LPARAM)pti);
	
	//SetTimer(hDlg, FAILED_FONT_TIMERID, nTimeout/*FAILED_FONT_TIMEOUT*/, 0);
}
void CToolTip::HideTip()
{
	HWND hTip = (mb_LastTipBalloon == 0) ? mh_Tip : mh_Ball;
	TOOLINFO *pti = (mb_LastTipBalloon == 0) ? (&mti_Tip) : (&mti_Ball);
	
	if (hTip)
		SendMessage(hTip, TTM_TRACKACTIVATE, FALSE, (LPARAM)pti);
}
#endif




#ifndef CONEMU_MINIMAL
// Обертка для таймера
CTimer::CTimer()
{
	mh_Wnd = NULL;
	mn_TimerId = mn_Elapse = 0;
	mb_Started = false;
}

CTimer::~CTimer()
{
	Stop();
}

bool CTimer::IsStarted()
{
	return mb_Started;
}

void CTimer::Start(UINT anElapse /*= 0*/)
{
	if (!mh_Wnd || mb_Started)
		return;

	mb_Started = true;
	INT_PTR iRc = SetTimer(mh_Wnd, mn_TimerId, anElapse ? anElapse : mn_Elapse, NULL);
	UNREFERENCED_PARAMETER(iRc);
	#ifdef _DEBUG
	DWORD nErr = GetLastError();
	_ASSERTE(iRc!=0);
	#endif
}

void CTimer::Stop()
{
	if (mb_Started)
	{
		mb_Started = false;
		KillTimer(mh_Wnd, mn_TimerId);
	}
}

void CTimer::Restart(UINT anElapse /*= 0*/)
{
	if (mb_Started)
		Stop();

	Start(anElapse);
}

void CTimer::Init(HWND ahWnd, UINT_PTR anTimerID, UINT anElapse)
{
	Stop();
	mh_Wnd = ahWnd; mn_TimerId = anTimerID; mn_Elapse = anElapse;
}
#endif



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
					for (DWORD n = 0; !bFound && !RegEnumKeyEx(hk, n, szName, &(nLen = countof(szName)-1), 0,0,0,0); n++)
					{
						HKEY hk2;
						if (!RegOpenKeyEx(hk, szName, 0, KEY_READ|nOpt, &hk2))
						{
							// Just in case, check "Path" too
							LPCWSTR rsNames[] = {NULL, L"Path"};

							for (size_t n = 0; n < countof(rsNames); n++)
							{
								memset(szPath, 0, sizeof(szPath)); DWORD nSize = (countof(szPath)-1)*sizeof(szPath[0]);
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
							wchar_t szPath[MAX_PATH+1] = {}; DWORD nSize = (countof(szPath)-1)*sizeof(szPath[0]);
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

wchar_t* GetEnvVar(LPCWSTR VarName, DWORD cchDefaultMax /*= 2000*/)
{
	if (!VarName || !*VarName)
	{
		return NULL;
	}

	_ASSERTE(cchDefaultMax >= MAX_PATH);

	DWORD cchMax = cchDefaultMax, nRc, nErr;
	wchar_t* pszVal = (wchar_t*)malloc(cchMax*sizeof(*pszVal));
	if (!pszVal)
	{
		_ASSERTE((pszVal!=NULL) && "GetEnvVar memory allocation failed");
		return NULL;
	}

	nRc = GetEnvironmentVariable(VarName, pszVal, cchMax);
	if (nRc == 0)
	{
		// Weird. This may be empty variable or not existing variable
		nErr = GetLastError();
		if (nErr == ERROR_ENVVAR_NOT_FOUND)
			SafeFree(pszVal);
		return pszVal;
	}

	// If buffer is not large enough to hold the data, the return value is the buffer size,
	// in characters, required to hold the string and its terminating null character.
	if (nRc >= cchMax)
	{
		cchMax = nRc+1;
		pszVal = (wchar_t*)realloc(pszVal, cchMax*sizeof(*pszVal));
		if (!pszVal)
		{
			_ASSERTE((pszVal!=NULL) && "GetEnvVar memory reallocation failed");
			return NULL;
		}

		nRc = GetEnvironmentVariable(VarName, pszVal, cchMax);
		if ((nRc == 0) || (nRc >= cchMax))
		{
			_ASSERTE(nRc > 0 && nRc < cchMax);
			SafeFree(pszVal);
		}
	}

	return pszVal;
}

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
	if (lstrcmpi(szName, ENV_CONEMU_SLEEP_INDICATE) == 0)
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
