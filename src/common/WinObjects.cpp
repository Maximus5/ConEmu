
/*
Copyright (c) 2009-2012 Maximus5
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
#include "../ConEmu/version.h"
#include <TlHelp32.h>
#ifndef CONEMU_MINIMAL
#include <shlobj.h>
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

BOOL FileCompare(LPCWSTR asFilePath1, LPCWSTR asFilePath2)
{
	BOOL bMatch = FALSE;
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
BOOL FileExists(LPCWSTR asFilePath, DWORD* pnSize /*= NULL*/)
{
	if (!asFilePath || !*asFilePath)
		return FALSE;

	WIN32_FIND_DATAW fnd = {0};
	HANDLE hFind = FindFirstFile(asFilePath, &fnd);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		BOOL lbFileFound = FALSE;

		// FindFirstFile может обломаться из-за симлинков
		if (GetLastError() == ERROR_ACCESS_DENIED)
		{
			hFind = CreateFile(asFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

			if (hFind != NULL)
			{
				BY_HANDLE_FILE_INFORMATION fi = {0};

				if (GetFileInformationByHandle(hFind, &fi) && !(fi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					lbFileFound = TRUE;

					if (pnSize)
						*pnSize = fi.nFileSizeHigh ? 0xFFFFFFFF : fi.nFileSizeLow; //-V112
				}
			}

			CloseHandle(hFind);
		}

		return lbFileFound;
	}

	BOOL lbFound = FALSE;

	do
	{
		if ((fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			lbFound = TRUE;

			if (pnSize)
				*pnSize = fnd.nFileSizeHigh ? 0xFFFFFFFF : fnd.nFileSizeLow; //-V112

			break;
		}
	}
	while (FindNextFile(hFind, &fnd));

	FindClose(hFind);
	return lbFound;
}

BOOL DirectoryExists(LPCWSTR asPath)
{
	if (!asPath || !*asPath)
		return FALSE;

	WIN32_FIND_DATAW fnd = {0};
	HANDLE hFind = FindFirstFile(asPath, &fnd);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	BOOL lbFound = FALSE;

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
	return lbFound;
}

BOOL MyCreateDirectory(wchar_t* asPath)
{
	if (!asPath || !*asPath)
		return FALSE;

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
			return TRUE;
		}
	}
	else
	{
		if (DirectoryExists(asPath))
			return TRUE;
	}

	if (CreateDirectory(asPath, NULL))
	{
		bOk = TRUE;
	}
	else
	{
		dwErr = GetLastError();
		if (pszSlash && (dwErr !=  ERROR_ACCESS_DENIED))
			bOk = MyCreateDirectory(asPath);
	}

	if (pszSlash)
	{
		*pszSlash = L'\\';
	}

	return bOk;
}

// Первичная проверка, может ли asFilePath быть путем
BOOL IsFilePath(LPCWSTR asFilePath)
{
	// Если в пути встречаются недопустимые символы
	if (wcschr(asFilePath, L'"') ||
	        wcschr(asFilePath, L'>') ||
	        wcschr(asFilePath, L'<') ||
	        wcschr(asFilePath, L'|')
	  )
		return FALSE;

	// Пропуск UNC "\\?\"
	if (asFilePath[0] == L'\\' && asFilePath[1] == L'\\' && asFilePath[2] == L'?' && asFilePath[3] == L'\\')
		asFilePath += 4; //-V112

	// Если asFilePath содержит два (и более) ":\"
	LPCWSTR pszColon = wcschr(asFilePath, L':');

	if (pszColon)
	{
		// Если есть ":", то это должен быть путь вида "X:\xxx", т.е. ":" - второй символ
		if (pszColon != (asFilePath+1))
			return FALSE;

		if (wcschr(pszColon+1, L':'))
			return FALSE;
	}

	// May be file path
	return TRUE;
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
BOOL IsUserAdmin()
{
	OSVERSIONINFO osv = {sizeof(OSVERSIONINFO)};
	GetVersionEx(&osv);

	// Проверять нужно только для висты, чтобы на XP лишний "Щит" не отображался
	if (osv.dwMajorVersion < 6)
		return FALSE;

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

	return(b);
}


#include <Sddl.h> // ConvertSidToStringSid
// *ppszSID - must be LocalFree'd
BOOL GetLogonSID (HANDLE hToken, wchar_t **ppszSID)
{
	BOOL bSuccess = FALSE;
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

	bSuccess = TRUE;

Cleanup: 

	// Free the buffer for the token groups.
	if ((ptu != NULL) && (ptu != &user))
		free(ptu);
	if (bFreeToken && hToken)
		CloseHandle(hToken);
	
	return bSuccess;
}
#endif


bool IsWine()
{
#ifdef _DEBUG
//	return true;
#endif
	bool bIsWine = false;
	HKEY hk1 = NULL, hk2 = NULL;
	if (!RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Wine", 0, KEY_READ, &hk1)
		&& !RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Wine", 0, KEY_READ, &hk2))
		bIsWine = true; // В общем случае, на флажок ориентироваться нельзя. Это для информации.
	if (hk1) RegCloseKey(hk1);
	if (hk2) RegCloseKey(hk2);
	return bIsWine;
}

bool IsDbcs()
{
	bool bIsDBCS = (GetSystemMetrics(SM_DBCSENABLED) != 0);
	return bIsDBCS;
}

typedef BOOL (WINAPI* IsWow64Process_t)(HANDLE hProcess, PBOOL Wow64Process);

// Проверить битность OS
BOOL IsWindows64()
{
	static BOOL is64bitOs = FALSE, isOsChecked = FALSE;
	if (isOsChecked)
		return is64bitOs;

	BOOL isWow64process = FALSE;
#ifdef WIN64
	is64bitOs = TRUE; isWow64process = FALSE;
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
				isWow64process = TRUE;
			}
		}
	}

	is64bitOs = isWow64process;
#endif

	isOsChecked = TRUE;
	return is64bitOs;
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

bool IsFarExe(LPCWSTR asModuleName)
{
	if (asModuleName && *asModuleName)
	{
		LPCWSTR pszName = PointToName(asModuleName);
		if (lstrcmpi(pszName, L"far.exe") == 0 || lstrcmpi(pszName, L"far") == 0
			|| lstrcmpi(pszName, L"far64.exe") == 0 || lstrcmpi(pszName, L"far64") == 0)
		{
			return true;
		}
	}
	return false;
}

// Проверить, валиден ли модуль?
bool IsModuleValid(HMODULE module)
{
	if ((module == NULL) || (module == INVALID_HANDLE_VALUE))
		return false;
	if (LDR_IS_RESOURCE(module))
		return false;

#ifdef USE_SEH
	bool lbValid = true;
	IMAGE_DOS_HEADER dos;
	IMAGE_NT_HEADERS nt;

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

	return lbValid;
#else
	if (IsBadReadPtr((void*)module, sizeof(IMAGE_DOS_HEADER)))
		return false;

	if (((IMAGE_DOS_HEADER*)module)->e_magic != IMAGE_DOS_SIGNATURE /*'ZM'*/)
		return false;

	IMAGE_NT_HEADERS* nt_header = (IMAGE_NT_HEADERS*)((char*)module + ((IMAGE_DOS_HEADER*)module)->e_lfanew);
	if (IsBadReadPtr(nt_header, sizeof(IMAGE_NT_HEADERS)))
		return false;

	if (nt_header->Signature != 0x004550)
		return false;

	return true;
#endif
}

BOOL CheckCallbackPtr(HMODULE hModule, size_t ProcCount, FARPROC* CallBack, BOOL abCheckModuleInfo, BOOL abAllowNTDLL)
{
	if ((hModule == NULL) || (hModule == INVALID_HANDLE_VALUE) || LDR_IS_RESOURCE(hModule))
	{
		_ASSERTE((hModule != NULL) && (hModule != INVALID_HANDLE_VALUE) && !LDR_IS_RESOURCE(hModule));
		return FALSE;
	}
	if (!CallBack || !ProcCount)
	{
		_ASSERTE(CallBack && ProcCount);
		return FALSE;
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
		if (!IsModuleValid(hModule))
		{
			_ASSERTE("!IsModuleValid(hModule)" && 0);
			return FALSE;
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
			return FALSE;
		}

		if ((((DWORD_PTR)(CallBack[i])) < nModulePtr)
			&& (!nModulePtr2 || (((DWORD_PTR)(CallBack[i])) < nModulePtr2)))
		{
			_ASSERTE(((DWORD_PTR)(CallBack[i])) >= nModulePtr);
			return FALSE;
		}

		if ((((DWORD_PTR)(CallBack[i])) > (nModuleSize + nModulePtr))
			&& (!nModulePtr2 || (((DWORD_PTR)(CallBack[i])) > (nModuleSize2 + nModulePtr2))))
		{
			_ASSERTE(((DWORD_PTR)(CallBack[i])) <= (nModuleSize + nModulePtr));
			return FALSE;
		}
	}

	return TRUE;
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
const wchar_t* Unquote(wchar_t* asParm)
{
	if (!asParm)
		return NULL;
	if (*asParm != L'"')
		return asParm;
	wchar_t* pszEndQ = wcsrchr(asParm, L'"');
	if (!pszEndQ || (pszEndQ == asParm))
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

	DWORD cchMax = MAX_PATH*2;
	wchar_t* pszExpand = (wchar_t*)malloc(cchMax*sizeof(*pszExpand));
	if (pszExpand)
	{
		pszExpand[0] = 0;

		DWORD nExp = ExpandEnvironmentStrings(pszCommand, pszExpand, cchMax);
		if (nExp && (nExp < cchMax) && *pszExpand)
			return pszExpand;

		SafeFree(pszExpand);
	}
	return NULL;
}



#ifndef __GNUC__
#pragma warning( push )
#pragma warning(disable : 6400)
#endif
BOOL IsExecutable(LPCWSTR aszFilePathName, wchar_t** rsExpandedVars /*= NULL*/)
{
#ifndef __GNUC__
#pragma warning( push )
#pragma warning(disable : 6400)
#endif

	wchar_t* pszExpand = NULL;

	for (int i = 0; i <= 1; i++)
	{
		LPCWSTR pwszDot = PointToExt(aszFilePathName);

		if (pwszDot)  // Если указан .exe или .com файл
		{
			if (lstrcmpiW(pwszDot, L".exe")==0 || lstrcmpiW(pwszDot, L".com")==0)
			{
				if (FileExists(aszFilePathName))
					return TRUE;
			}
		}

		if (!i && wcschr(aszFilePathName, L'%'))
		{
			pszExpand = ExpandEnvStr(aszFilePathName);
			if (!pszExpand)
				break;
			aszFilePathName = pszExpand;
		}
	}

	if (rsExpandedVars)
	{
		*rsExpandedVars = pszExpand; pszExpand = NULL;
	}
	else
	{
		SafeFree(pszExpand);
	}

	return FALSE;
}
#ifndef __GNUC__
#pragma warning( pop )
#endif

// Команды, которые не нужно пытаться развернуть в exe?
// кроме того, если команда содержит "?" или "*" - тоже не пытаться.
const wchar_t* gsInternalCommands = L"ACTIVATE\0ALIAS\0ASSOC\0ATTRIB\0BEEP\0BREAK\0CALL\0CDD\0CHCP\0COLOR\0COPY\0DATE\0DEFAULT\0DEL\0DELAY\0DESCRIBE\0DETACH\0DIR\0DIRHISTORY\0DIRS\0DRAWBOX\0DRAWHLINE\0DRAWVLINE\0ECHO\0ECHOERR\0ECHOS\0ECHOSERR\0ENDLOCAL\0ERASE\0ERRORLEVEL\0ESET\0EXCEPT\0EXIST\0EXIT\0FFIND\0FOR\0FREE\0FTYPE\0GLOBAL\0GOTO\0HELP\0HISTORY\0IF\0IFF\0INKEY\0INPUT\0KEYBD\0KEYS\0LABEL\0LIST\0LOG\0MD\0MEMORY\0MKDIR\0MOVE\0MSGBOX\0NOT\0ON\0OPTION\0PATH\0PAUSE\0POPD\0PROMPT\0PUSHD\0RD\0REBOOT\0REN\0RENAME\0RMDIR\0SCREEN\0SCRPUT\0SELECT\0SET\0SETDOS\0SETLOCAL\0SHIFT\0SHRALIAS\0START\0TEE\0TIME\0TIMER\0TITLE\0TOUCH\0TREE\0TRUENAME\0TYPE\0UNALIAS\0UNSET\0VER\0VERIFY\0VOL\0VSCRPUT\0WINDOW\0Y\0\0";

BOOL IsNeedCmd(LPCWSTR asCmdLine, LPCWSTR* rsArguments, BOOL *rbNeedCutStartEndQuot,
			   wchar_t (&szExe)[MAX_PATH+1],
			   BOOL& rbRootIsCmdExe, BOOL& rbAlwaysConfirmExit, BOOL& rbAutoDisableConfirmExit)
{
	_ASSERTE(asCmdLine && *asCmdLine);
	rbRootIsCmdExe = TRUE;

	#ifdef _DEBUG
	// Это минимальные проверки, собственно к коду - не относятся
	wchar_t szDbgFirst[MAX_PATH+1];
	bool bIsBatch = false;
	{
		LPCWSTR psz = asCmdLine;
		NextArg(&psz, szDbgFirst);
		psz = PointToExt(szDbgFirst);
		if (lstrcmpi(psz, L".cmd")==0 || lstrcmpi(psz, L".bat")==0)
			bIsBatch = true;
	}
	#endif

	memset(szExe, 0, sizeof(szExe));

	if (!asCmdLine || *asCmdLine == 0)
	{
		_ASSERTE(asCmdLine && *asCmdLine);
		return TRUE;
	}
		
	//110202 перенес вниз, т.к. это уже может быть cmd.exe, и тогда у него сносит крышу
	//// Если есть одна из команд перенаправления, или слияния - нужен CMD.EXE
	//if (wcschr(asCmdLine, L'&') ||
	//        wcschr(asCmdLine, L'>') ||
	//        wcschr(asCmdLine, L'<') ||
	//        wcschr(asCmdLine, L'|') ||
	//        wcschr(asCmdLine, L'^') // или экранирования
	//  )
	//{
	//	return TRUE;
	//}

	//wchar_t szArg[MAX_PATH+10] = {0};
	int iRc = 0;
	BOOL lbFirstWasGot = FALSE;
	LPCWSTR pwszCopy = asCmdLine;
	// cmd /c ""c:\program files\arc\7z.exe" -?"   // да еще и внутри могут быть двойными...
	// cmd /c "dir c:\"
	int nLastChar = lstrlenW(pwszCopy) - 1;

	if (pwszCopy[0] == L'"' && pwszCopy[nLastChar] == L'"')
	{
		if (pwszCopy[1] == L'"' && pwszCopy[2])
		{
			pwszCopy ++; // Отбросить первую кавычку в командах типа: ""c:\program files\arc\7z.exe" -?"

			if (rbNeedCutStartEndQuot) *rbNeedCutStartEndQuot = TRUE;
		}
		else
			// глючила на ""F:\VCProject\FarPlugin\#FAR180\far.exe  -new_console""
			//if (wcschr(pwszCopy+1, L'"') == (pwszCopy+nLastChar)) {
			//	LPCWSTR pwszTemp = pwszCopy;
			//	// Получим первую команду (исполняемый файл?)
			//	if ((iRc = NextArg(&pwszTemp, szArg)) != 0) {
			//		//Parsing command line failed
			//		return TRUE;
			//	}
			//	pwszCopy ++; // Отбросить первую кавычку в командах типа: "c:\arc\7z.exe -?"
			//	lbFirstWasGot = TRUE;
			//	if (rbNeedCutStartEndQuot) *rbNeedCutStartEndQuot = TRUE;
			//} else
		{
			// отбросить первую кавычку в: "C:\GCC\msys\bin\make.EXE -f "makefile" COMMON="../../../plugins/common""
			LPCWSTR pwszTemp = pwszCopy + 1;

			// Получим первую команду (исполняемый файл?)
			if ((iRc = NextArg(&pwszTemp, szExe)) != 0)
			{
				//Parsing command line failed
				#ifdef WARN_NEED_CMD
				_ASSERTE(FALSE);
				#endif
				return TRUE;
			}
			
			if (lstrcmpiW(szExe, L"start") == 0)
			{
				// Команду start обрабатывает только процессор
				#ifdef WARN_NEED_CMD
				_ASSERTE(FALSE);
				#endif
				return TRUE;
			}

			LPCWSTR pwszQ = pwszCopy + 1 + lstrlen(szExe);
			wchar_t* pszExpand = NULL;

			if (*pwszQ != L'"' && IsExecutable(szExe, &pszExpand))
			{
				pwszCopy ++; // отбрасываем
				lbFirstWasGot = TRUE;

				if (pszExpand)
				{
					lstrcpyn(szExe, pszExpand, countof(szExe));
					SafeFree(pszExpand);
					if (rsArguments)
						*rsArguments = pwszQ;
				}

				if (rbNeedCutStartEndQuot) *rbNeedCutStartEndQuot = TRUE;
			}
		}
	}

	// Получим первую команду (исполняемый файл?)
	if (!lbFirstWasGot)
	{
		szExe[0] = 0;
		// 17.10.2010 - поддержка переданного исполняемого файла без параметров, но с пробелами в пути
		LPCWSTR pchEnd = pwszCopy + lstrlenW(pwszCopy);

		while (pchEnd > pwszCopy && *(pchEnd-1) == L' ') pchEnd--;

		if ((pchEnd - pwszCopy) < MAX_PATH)
		{
			memcpy(szExe, pwszCopy, (pchEnd - pwszCopy)*sizeof(wchar_t));
			szExe[(pchEnd - pwszCopy)] = 0;

			if (!FileExists(szExe))
			{
				bool bFound = false;
				// Переменные окружения
				if (wcschr(szExe, L'%'))
				{
					wchar_t* pszExpand = ExpandEnvStr(szExe);
					if (pszExpand)
						lstrcpyn(szExe, pszExpand, countof(szExe));
					SafeFree(pszExpand);

					if (FileExists(szExe))
					{
						if (rsArguments)
							*rsArguments = pchEnd;
						bFound = true;
					}
				}

				if (!bFound)
					szExe[0] = 0;
			}
		}

		if (szExe[0] == 0)
		{
			if ((iRc = NextArg(&pwszCopy, szExe)) != 0)
			{
				//Parsing command line failed
				#ifdef WARN_NEED_CMD
				_ASSERTE(FALSE);
				#endif
				return TRUE;
			}

			if (wcschr(szExe, L'%') && !FileExists(szExe))
			{
				DEBUGTEST(bool bFound = false);

				// Переменные окружения
				wchar_t* pszExpand = ExpandEnvStr(szExe);
				if (pszExpand && FileExists(pszExpand))
				{
					lstrcpyn(szExe, pszExpand, countof(szExe));
					SafeFree(pszExpand);

					if (rsArguments)
						*rsArguments = pwszCopy;

					DEBUGTEST(bFound = true);
				}
			}
		}
	}
	
	if (!*szExe)
	{
		_ASSERTE(szExe[0] != 0);
	}
	else
	{
		// "Левые" символы в имени файла
		if (wcspbrk(szExe, L"?*<>|"))
		{
			rbRootIsCmdExe = TRUE; // запуск через "процессор"
			return TRUE; // добавить "cmd.exe"
		}
		
		// если "путь" не указан
		if (wcschr(szExe, L'\\') == NULL) 
		{
			bool bHasExt = (wcschr(szExe, L'.') != NULL);
			// Проверим, может это команда процессора (типа "DIR")?
			if (!bHasExt)
			{
				bool bIsCommand = false;
				wchar_t* pszUppr = lstrdup(szExe);
				if (pszUppr)
				{
					// избежать линковки на user32.dll
					//CharUpperBuff(pszUppr, lstrlen(pszUppr));
					for (wchar_t* p = pszUppr; *p; p++)
					{
						if (*p >= L'a' && *p <= 'z')
							*p -= 0x20;
					}
					
					const wchar_t* pszFind = gsInternalCommands;
					while (*pszFind)
					{
						if (lstrcmp(pszUppr, pszFind) == 0)
						{
							bIsCommand = true;
							break;
						}
						pszFind += lstrlen(pszFind)+1;
					}
					free(pszUppr);
				}
				if (bIsCommand)
				{
					#ifdef WARN_NEED_CMD
					_ASSERTE(FALSE);
					#endif
					rbRootIsCmdExe = TRUE; // запуск через "процессор"
					return TRUE; // добавить "cmd.exe"
				}
			}
			
			// Пробуем найти "по путям" соответствующий exe-шник.
			DWORD nCchMax = countof(szExe); // выделить память, длинее чем szExe вернуть не сможем
			wchar_t* pszSearch = (wchar_t*)malloc(nCchMax*sizeof(wchar_t));
			if (pszSearch)
			{
				#ifndef CONEMU_MINIMAL
				MWow64Disable wow; wow.Disable(); // Отключить редиректор!
				#endif
				wchar_t *pszName = NULL;
				DWORD nRc = SearchPath(NULL, szExe, bHasExt ? NULL : L".exe", nCchMax, pszSearch, &pszName);
				if (nRc && (nRc < nCchMax))
				{
					// Нашли, возвращаем что нашли
					wcscpy_c(szExe, pszSearch);
				}
				free(pszSearch);
			}
		} // end: if (wcschr(szExe, L'\\') == NULL) 
	}

	// Если szExe не содержит путь к файлу - запускаем через cmd
	// "start "" C:\Utils\Files\Hiew32\hiew32.exe C:\00\Far.exe"
	if (!IsFilePath(szExe))
	{
		#ifdef WARN_NEED_CMD
		_ASSERTE(FALSE);
		#endif
		rbRootIsCmdExe = TRUE; // запуск через "процессор"
		return TRUE; // добавить "cmd.exe"
	}

	//pwszCopy = wcsrchr(szArg, L'\\'); if (!pwszCopy) pwszCopy = szArg; else pwszCopy ++;
	pwszCopy = PointToName(szExe);
	//2009-08-27
	wchar_t *pwszEndSpace = szExe + lstrlenW(szExe) - 1;

	while((*pwszEndSpace == L' ') && (pwszEndSpace > szExe))
		*(pwszEndSpace--) = 0;

#ifndef __GNUC__
#pragma warning( push )
#pragma warning(disable : 6400)
#endif

	if (lstrcmpiW(pwszCopy, L"cmd")==0 || lstrcmpiW(pwszCopy, L"cmd.exe")==0
		|| lstrcmpiW(pwszCopy, L"tcc")==0 || lstrcmpiW(pwszCopy, L"tcc.exe")==0)
	{
		rbRootIsCmdExe = TRUE; // уже должен быть выставлен, но проверим
		rbAlwaysConfirmExit = TRUE; rbAutoDisableConfirmExit = FALSE;
		_ASSERTE(!bIsBatch);
		return FALSE; // уже указан командный процессор, cmd.exe в начало добавлять не нужно
	}


	// Если есть одна из команд перенаправления, или слияния - нужен CMD.EXE
	if (wcschr(asCmdLine, L'&') ||
		wcschr(asCmdLine, L'>') ||
		wcschr(asCmdLine, L'<') ||
		wcschr(asCmdLine, L'|') ||
		wcschr(asCmdLine, L'^') // или экранирования
		)
	{
		#ifdef WARN_NEED_CMD
		_ASSERTE(FALSE);
		#endif
		return TRUE;
	}


	//if (lstrcmpiW(pwszCopy, L"far")==0 || lstrcmpiW(pwszCopy, L"far.exe")==0)
	if (IsFarExe(pwszCopy))
	{
		bool bFound = (wcschr(pwszCopy, L'.') != NULL);
		// Если указали при запуске просто "far" - это может быть батник, расположенный в %PATH%
		if (!bFound)
		{
			wchar_t szSearch[MAX_PATH+1], *pszPart = NULL;
			DWORD n = SearchPath(NULL, pwszCopy, L".exe", countof(szSearch), szSearch, &pszPart);
			if (n && (n < countof(szSearch)))
			{
				if (lstrcmpi(PointToExt(pszPart), L".exe") == 0)
					bFound = true;
			}
		}

		if (bFound)
		{
			rbAutoDisableConfirmExit = TRUE;
			rbRootIsCmdExe = FALSE; // FAR!
			_ASSERTE(!bIsBatch);
			return FALSE; // уже указан исполняемый файл, cmd.exe в начало добавлять не нужно
		}
	}

	if (IsExecutable(szExe))
	{
		rbRootIsCmdExe = FALSE; // Для других программ - буфер не включаем
		_ASSERTE(!bIsBatch);
		return FALSE; // Запускается конкретная консольная программа. cmd.exe не требуется
	}

	//Можно еще Доделать поиски с: SearchPath, GetFullPathName, добавив расширения .exe & .com
	//хотя фар сам формирует полные пути к командам, так что можно не заморачиваться
	#ifdef WARN_NEED_CMD
	_ASSERTE(FALSE);
	#endif
	rbRootIsCmdExe = TRUE;
#ifndef __GNUC__
#pragma warning( pop )
#endif
	return TRUE;
}

const wchar_t* SkipNonPrintable(const wchar_t* asParams)
{
	if (!asParams)
		return NULL;
	const wchar_t* psz = asParams;
	while (*psz == L' ' || *psz == L'\t' || *psz == L'\r' || *psz == L'\n') psz++;
	return psz;
}

// One trailing (or middle) asterisk allowed
bool CompareFileMask(const wchar_t* asFileName, const wchar_t* asMask)
{
	if (!asFileName || !*asFileName || !asMask || !*asMask)
		return false;
	// Any file?
	if (*asMask == L'*' && *(asMask+1) == 0)
		return true;

	int iCmp = -1;
	
	wchar_t sz1[MAX_PATH+1], sz2[MAX_PATH+1];
	lstrcpyn(sz1, asFileName, countof(sz1));
	size_t nLen1 = lstrlen(sz1);
	CharUpperBuffW(sz1, (DWORD)nLen1);
	lstrcpyn(sz2, asMask, countof(sz2));
	size_t nLen2 = lstrlen(sz2);
	CharUpperBuffW(sz2, (DWORD)nLen2);

	wchar_t* pszAst = wcschr(sz2, L'*');
	if (!pszAst)
	{
		iCmp = lstrcmp(sz1, sz2);
	}
	else
	{
		*pszAst = 0;
		size_t nLen = pszAst - sz2;
		size_t nRight = lstrlen(pszAst+1);
		if (wcsncmp(sz1, sz2, nLen) == 0)
		{
			if (!nRight)
				iCmp = 0;
			else if (nLen1 >= (nRight + nLen))
				iCmp = lstrcmp(sz1+nLen1-nRight, pszAst+1);
		}
	}

	return (iCmp == 0);
}


//// Вернуть путь к папке, содержащей ConEmuC.exe
//BOOL FindConEmuBaseDir(wchar_t (&rsConEmuBaseDir)[MAX_PATH+1], wchar_t (&rsConEmuExe)[MAX_PATH+1])
//{
//	// Сначала пробуем Mapping консоли (вдруг есть?)
//	{
//		MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> ConMap;
//		ConMap.InitName(CECONMAPNAME, (DWORD)GetConEmuHWND(2));
//		CESERVER_CONSOLE_MAPPING_HDR* p = ConMap.Open();
//		if (p && p->sConEmuBaseDir[0])
//		{
//			// Успешно
//			wcscpy_c(rsConEmuBaseDir, p->sConEmuBaseDir);
//			wcscpy_c(rsConEmuExe, p->sConEmuExe);
//			return TRUE;
//		}
//	}
//
//	// Теперь - пробуем найти существующее окно ConEmu
//	HWND hConEmu = FindWindow(Virtual_Console_ClassMain, NULL);
//	DWORD dwGuiPID = 0;
//	if (hConEmu)
//	{
//		if (GetWindowThreadProcessId(hConEmu, &dwGuiPID) && dwGuiPID)
//		{
//			MFileMapping<ConEmuGuiMapping> GuiMap;
//			GuiMap.InitName(CEGUIINFOMAPNAME, dwGuiPID);
//			ConEmuGuiMapping* p = GuiMap.Open();
//			if (p && p->sConEmuBaseDir[0])
//			{
//				wcscpy_c(rsConEmuBaseDir, p->sConEmuBaseDir);
//				wcscpy_c(rsConEmuExe, p->sConEmuExe);
//				return TRUE;
//			}
//		}
//	}
//
//	
//	wchar_t szExePath[MAX_PATH+1];
//	HKEY hkRoot[] = {NULL,HKEY_CURRENT_USER,HKEY_LOCAL_MACHINE,HKEY_LOCAL_MACHINE};
//	DWORD samDesired = KEY_QUERY_VALUE;
//	DWORD RedirectionFlag = 0;
//	BOOL isWin64 = FALSE;
//	#ifdef _WIN64
//	isWin64 = TRUE;
//	RedirectionFlag = KEY_WOW64_32KEY;
//	#else
//	isWin64 = IsWindows64();
//	RedirectionFlag = isWin64 ? KEY_WOW64_64KEY : 0;
//	#endif
//	for (size_t i = 0; i < countof(hkRoot); i++)
//	{
//		szExePath[0] = 0;
//
//		if (i == 0)
//		{
//			// Запущенного ConEmu.exe нет, можно поискать в каталоге текущего приложения
//
//			if (!GetModuleFileName(NULL, szExePath, countof(szExePath)-20))
//				continue;
//			wchar_t* pszName = wcsrchr(szExePath, L'\\');
//			if (!pszName)
//				continue;
//			*(pszName+1) = 0;
//		}
//		else
//		{
//			// Остался последний шанс - если ConEmu установлен через MSI, то путь указан в реестре
//			// [HKEY_LOCAL_MACHINE\SOFTWARE\ConEmu]
//			// "InstallDir"="C:\\Utils\\Far180\\"
//
//			if (i == (countof(hkRoot)-1))
//			{
//				if (RedirectionFlag)
//					samDesired |= RedirectionFlag;
//				else
//					break;
//			}
//
//			HKEY hKey;
//			if (RegOpenKeyEx(hkRoot[i], L"Software\\ConEmu", 0, samDesired, &hKey) != ERROR_SUCCESS)
//				continue;
//			memset(szExePath, 0, countof(szExePath));
//			DWORD nType = 0, nSize = sizeof(szExePath)-20*sizeof(wchar_t);
//			int RegResult = RegQueryValueEx(hKey, L"", NULL, &nType, (LPBYTE)szExePath, &nSize);
//			RegCloseKey(hKey);
//			if (RegResult != ERROR_SUCCESS)
//				continue;
//		}
//
//		if (szExePath[0])
//		{
//			// Хоть и задано в реестре - файлов может не быть. Проверяем
//			if (szExePath[lstrlen(szExePath)-1] != L'\\')
//				wcscat_c(szExePath, L"\\");
//			wcscpy_c(rsConEmuExe, szExePath);
//			BOOL lbExeFound = FALSE;
//			wchar_t* pszName = rsConEmuExe+lstrlen(rsConEmuExe);
//			LPCWSTR szGuiExe[2] = {L"ConEmu64.exe", L"ConEmu.exe"};
//			for (int i = 0; !lbExeFound && (i < countof(szGuiExe)); i++)
//			{
//				if (!i && !isWin64) continue;
//				wcscpy_add(pszName, rsConEmuExe, szGuiExe[i]);
//				lbExeFound = FileExists(rsConEmuExe);
//			}
//
//			// Если GUI-exe найден - ищем "base"
//			if (lbExeFound)
//			{
//				wchar_t* pszName = szExePath+lstrlen(szExePath);
//				LPCWSTR szSrvExe[4] = {L"ConEmuC64.exe", L"ConEmu\\ConEmuC64.exe", L"ConEmuC.exe", L"ConEmu\\ConEmuC.exe"};
//				for (int i = 0; (i < countof(szSrvExe)); i++)
//				{
//					if ((i <=1) && !isWin64) continue;
//					wcscpy_add(pszName, szExePath, szSrvExe[i]);
//					if (FileExists(szExePath))
//					{
//						pszName = wcsrchr(szExePath, L'\\');
//						if (pszName)
//						{
//							*pszName = 0; // БЕЗ слеша на конце!
//							wcscpy_c(rsConEmuBaseDir, szExePath);
//							return TRUE;
//						}
//					}
//				}
//			}
//		}
//	}
//
//	// Не удалось
//	return FALSE;
//}

//// Undocumented console message
//#define WM_SETCONSOLEINFO			(WM_USER+201)
//
//#pragma pack(push, 1)
//
////
////	Structure to send console via WM_SETCONSOLEINFO
////
//typedef struct _CONSOLE_INFO
//{
//	ULONG		Length;
//	COORD		ScreenBufferSize;
//	COORD		WindowSize;
//	ULONG		WindowPosX;
//	ULONG		WindowPosY;
//
//	COORD		FontSize;
//	ULONG		FontFamily;
//	ULONG		FontWeight;
//	WCHAR		FaceName[32];
//
//	ULONG		CursorSize;
//	ULONG		FullScreen;
//	ULONG		QuickEdit;
//	ULONG		AutoPosition;
//	ULONG		InsertMode;
//
//	USHORT		ScreenColors;
//	USHORT		PopupColors;
//	ULONG		HistoryNoDup;
//	ULONG		HistoryBufferSize;
//	ULONG		NumberOfHistoryBuffers;
//
//	COLORREF	ColorTable[16];
//
//	ULONG		CodePage;
//	HWND		Hwnd;
//
//	WCHAR		ConsoleTitle[0x100];
//
//} CONSOLE_INFO;
//
//CONSOLE_INFO* gpConsoleInfoStr = NULL;
//HANDLE ghConsoleSection = NULL;
//const UINT gnConsoleSectionSize = sizeof(CONSOLE_INFO)+1024;
//
//#pragma pack(pop)
//
//
//
//
//
//
//
//
//
//
//BOOL GetShortFileName(LPCWSTR asFullPath, wchar_t* rsShortName/*name only, MAX_PATH required*/)
//{
//	WIN32_FIND_DATAW fnd; memset(&fnd, 0, sizeof(fnd));
//	HANDLE hFind = FindFirstFile(asFullPath, &fnd);
//	if (hFind == INVALID_HANDLE_VALUE)
//		return FALSE;
//	FindClose(hFind);
//	if (fnd.cAlternateFileName[0]) {
//		if (lstrlenW(fnd.cAlternateFileName) < lstrlenW(fnd.cFileName)) {
//			lstrcpyW(rsShortName, fnd.cAlternateFileName);
//			return TRUE;
//		}
//	}
//
//	return FALSE;
//}
//
//wchar_t* GetShortFileNameEx(LPCWSTR asLong)
//{
//	TODO("хорошо бы и сетевые диски обрабатывать");
//	if (!asLong) return NULL;
//
//	wchar_t* pszShort = /*lstrdup(asLong);*/(wchar_t*)malloc((lstrlenW(asLong)+1)*2);
//	lstrcpyW(pszShort, asLong);
//	wchar_t* pszCur = wcschr(pszShort+3, L'\\');
//	wchar_t* pszSlash;
//	wchar_t  szName[MAX_PATH+1];
//	size_t nLen = 0;
//
//	while (pszCur) {
//		*pszCur = 0;
//		{
//			if (GetShortFileName(pszShort, szName)) {
//				if ((pszSlash = wcsrchr(pszShort, L'\\'))==0)
//					goto wrap;
//				lstrcpyW(pszSlash+1, szName);
//				pszSlash += 1+lstrlenW(szName);
//				lstrcpyW(pszSlash+1, pszCur+1);
//				pszCur = pszSlash;
//			}
//		}
//		*pszCur = L'\\';
//		pszCur = wcschr(pszCur+1, L'\\');
//	}
//	nLen = lstrlenW(pszShort);
//	if (nLen>0 && pszShort[nLen-1]==L'\\')
//		pszShort[--nLen] = 0;
//	if (nLen <= MAX_PATH)
//		return pszShort;
//
//wrap:
//	free(pszShort);
//	return NULL;
//}
//
//int NextArg(const wchar_t** asCmdLine, wchar_t* rsArg/*[MAX_PATH+1]*/, const wchar_t** rsArgStart/*=NULL*/)
//{
//    LPCWSTR psCmdLine = *asCmdLine, pch = NULL;
//    wchar_t ch = *psCmdLine;
//    size_t nArgLen = 0;
//
//    while (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n') ch = *(++psCmdLine);
//    if (ch == 0) return CERR_CMDLINEEMPTY;
//
//    // аргумент начинается с "
//    if (ch == L'"') {
//        psCmdLine++;
//        pch = wcschr(psCmdLine, L'"');
//        if (!pch) return CERR_CMDLINE;
//        while (pch[1] == L'"') {
//            pch += 2;
//            pch = wcschr(pch, L'"');
//            if (!pch) return CERR_CMDLINE;
//        }
//        // Теперь в pch ссылка на последнюю "
//    } else {
//        // До конца строки или до первого пробела
//        //pch = wcschr(psCmdLine, L' ');
//        // 09.06.2009 Maks - обломался на: cmd /c" echo Y "
//        pch = psCmdLine;
//        while (*pch && *pch!=L' ' && *pch!=L'"') pch++;
//        //if (!pch) pch = psCmdLine + lstrlenW(psCmdLine); // до конца строки
//    }
//
//    nArgLen = pch - psCmdLine;
//    if (nArgLen > MAX_PATH) return CERR_CMDLINE;
//
//    // Вернуть аргумент
//    memcpy(rsArg, psCmdLine, nArgLen*sizeof(wchar_t));
//    if (rsArgStart) *rsArgStart = psCmdLine;
//    rsArg[nArgLen] = 0;
//
//    psCmdLine = pch;
//
//    // Finalize
//    ch = *psCmdLine; // может указывать на закрывающую кавычку
//    if (ch == L'"') ch = *(++psCmdLine);
//    while (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n') ch = *(++psCmdLine);
//    *asCmdLine = psCmdLine;
//
//    return 0;
//}
//
//
//BOOL PackInputRecord(const INPUT_RECORD* piRec, MSG* pMsg)
//{
//	_ASSERTE(pMsg!=NULL && piRec!=NULL);
//	memset(pMsg, 0, sizeof(MSG));
//
//    UINT nMsg = 0; WPARAM wParam = 0; LPARAM lParam = 0;
//    if (piRec->EventType == KEY_EVENT) {
//    	nMsg = piRec->Event.KeyEvent.bKeyDown ? WM_KEYDOWN : WM_KEYUP;
//
//		lParam |= (WORD)piRec->Event.KeyEvent.uChar.UnicodeChar;
//		lParam |= ((BYTE)piRec->Event.KeyEvent.wVirtualKeyCode) << 16;
//		lParam |= ((BYTE)piRec->Event.KeyEvent.wVirtualScanCode) << 24;
//
//        wParam |= (WORD)piRec->Event.KeyEvent.dwControlKeyState;
//        wParam |= ((DWORD)piRec->Event.KeyEvent.wRepeatCount & 0xFF) << 16;
//
//    } else if (piRec->EventType == MOUSE_EVENT) {
//		switch (piRec->Event.MouseEvent.dwEventFlags) {
//			case MOUSE_MOVED:
//				nMsg = MOUSE_EVENT_MOVE;
//				break;
//			case 0:
//				nMsg = MOUSE_EVENT_CLICK;
//				break;
//			case DOUBLE_CLICK:
//				nMsg = MOUSE_EVENT_DBLCLICK;
//				break;
//			case MOUSE_WHEELED:
//				nMsg = MOUSE_EVENT_WHEELED;
//				break;
//			case /*MOUSE_HWHEELED*/ 0x0008:
//				nMsg = MOUSE_EVENT_HWHEELED;
//				break;
//			default:
//				_ASSERT(FALSE);
//		}
//
//    	lParam = ((WORD)piRec->Event.MouseEvent.dwMousePosition.X)
//    	       | (((DWORD)(WORD)piRec->Event.MouseEvent.dwMousePosition.Y) << 16);
//
//		// max 0x0010/*FROM_LEFT_4ND_BUTTON_PRESSED*/
//		wParam |= ((DWORD)piRec->Event.MouseEvent.dwButtonState) & 0xFF;
//
//		// max - ENHANCED_KEY == 0x0100
//		wParam |= (((DWORD)piRec->Event.MouseEvent.dwControlKeyState) & 0xFFFF) << 8;
//
//		if (nMsg == MOUSE_EVENT_WHEELED || nMsg == MOUSE_EVENT_HWHEELED) {
//    		// HIWORD() - short (direction[1/-1])*count*120
//    		short nWheel = (short)((((DWORD)piRec->Event.MouseEvent.dwButtonState) & 0xFFFF0000) >> 16);
//    		char  nCount = nWheel / 120;
//    		wParam |= ((DWORD)(BYTE)nCount) << 24;
//		}
//
//
//    } else if (piRec->EventType == FOCUS_EVENT) {
//    	nMsg = piRec->Event.FocusEvent.bSetFocus ? WM_SETFOCUS : WM_KILLFOCUS;
//
//    } else {
//    	_ASSERT(FALSE);
//    	return FALSE;
//    }
//    _ASSERTE(nMsg!=0);
//
//
//    pMsg->message = nMsg;
//    pMsg->wParam = wParam;
//    pMsg->lParam = lParam;
//
//    return TRUE;
//}
//
//BOOL UnpackInputRecord(const MSG* piMsg, INPUT_RECORD* pRec)
//{
//	_ASSERTE(piMsg!=NULL && pRec!=NULL);
//	memset(pRec, 0, sizeof(INPUT_RECORD));
//
//	if (piMsg->message == 0)
//		return FALSE;
//
//	if (piMsg->message == WM_KEYDOWN || piMsg->message == WM_KEYUP) {
//		pRec->EventType = KEY_EVENT;
//
//		// lParam
//        pRec->Event.KeyEvent.bKeyDown = (piMsg->message == WM_KEYDOWN);
//        pRec->Event.KeyEvent.uChar.UnicodeChar = (WCHAR)(piMsg->lParam & 0xFFFF);
//        pRec->Event.KeyEvent.wVirtualKeyCode   = (((DWORD)piMsg->lParam) & 0xFF0000) >> 16;
//        pRec->Event.KeyEvent.wVirtualScanCode  = (((DWORD)piMsg->lParam) & 0xFF000000) >> 24;
//
//        // wParam. Пока что тут может быть max(ENHANCED_KEY==0x0100)
//        pRec->Event.KeyEvent.dwControlKeyState = ((DWORD)piMsg->wParam & 0xFFFF);
//
//        pRec->Event.KeyEvent.wRepeatCount = ((DWORD)piMsg->wParam & 0xFF0000) >> 16;
//
//	} else if (piMsg->message >= MOUSE_EVENT_FIRST && piMsg->message <= MOUSE_EVENT_LAST) {
//		pRec->EventType = MOUSE_EVENT;
//
//		switch (piMsg->message) {
//			case MOUSE_EVENT_MOVE:
//				pRec->Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
//				break;
//			case MOUSE_EVENT_CLICK:
//				pRec->Event.MouseEvent.dwEventFlags = 0;
//				break;
//			case MOUSE_EVENT_DBLCLICK:
//				pRec->Event.MouseEvent.dwEventFlags = DOUBLE_CLICK;
//				break;
//			case MOUSE_EVENT_WHEELED:
//				pRec->Event.MouseEvent.dwEventFlags = MOUSE_WHEELED;
//				break;
//			case MOUSE_EVENT_HWHEELED:
//				pRec->Event.MouseEvent.dwEventFlags = /*MOUSE_HWHEELED*/ 0x0008;
//				break;
//		}
//
//		pRec->Event.MouseEvent.dwMousePosition.X = LOWORD(piMsg->lParam);
//		pRec->Event.MouseEvent.dwMousePosition.Y = HIWORD(piMsg->lParam);
//
//		// max 0x0010/*FROM_LEFT_4ND_BUTTON_PRESSED*/
//		pRec->Event.MouseEvent.dwButtonState = ((DWORD)piMsg->wParam) & 0xFF;
//
//		// max - ENHANCED_KEY == 0x0100
//		pRec->Event.MouseEvent.dwControlKeyState = (((DWORD)piMsg->wParam) & 0xFFFF00) >> 8;
//
//		if (piMsg->message == MOUSE_EVENT_WHEELED || piMsg->message == MOUSE_EVENT_HWHEELED) {
//    		// HIWORD() - short (direction[1/-1])*count*120
//    		signed char nDir = (signed char)((((DWORD)piMsg->wParam) & 0xFF000000) >> 24);
//    		WORD wDir = nDir*120;
//    		pRec->Event.MouseEvent.dwButtonState |= wDir << 16;
//		}
//
//	} else if (piMsg->message == WM_SETFOCUS || piMsg->message == WM_KILLFOCUS) {
//        pRec->EventType = FOCUS_EVENT;
//
//        pRec->Event.FocusEvent.bSetFocus = (piMsg->message == WM_SETFOCUS);
//
//	} else {
//		return FALSE;
//	}
//
//	return TRUE;
//}
//
//class MNullDesc {
//protected:
//	PSECURITY_DESCRIPTOR mp_NullDesc;
//	SECURITY_ATTRIBUTES  m_NullSecurity;
//public:
//	DWORD mn_LastError;
//public:
//	MNullDesc() {
//		memset(&m_NullSecurity, 0, sizeof(m_NullSecurity));
//		mp_NullDesc = NULL;
//		mn_LastError = 0;
//	};
//	~MNullDesc() {
//		memset(&m_NullSecurity, 0, sizeof(m_NullSecurity));
//		LocalFree(mp_NullDesc); mp_NullDesc = NULL;
//	};
//public:
//	SECURITY_ATTRIBUTES* NullSecurity() {
//		mn_LastError = 0;
//
//		if (mp_NullDesc) {
//			_ASSERTE(m_NullSecurity.lpSecurityDescriptor==mp_NullDesc);
//			return (&m_NullSecurity);
//		}
//		mp_NullDesc = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR,
//		      SECURITY_DESCRIPTOR_MIN_LENGTH);
//
//		if (mp_NullDesc == NULL) {
//			mn_LastError = GetLastError();
//			return NULL;
//		}
//
//		if (!InitializeSecurityDescriptor(mp_NullDesc, SECURITY_DESCRIPTOR_REVISION)) {
//			mn_LastError = GetLastError();
//			LocalFree(mp_NullDesc); mp_NullDesc = NULL;
//			return NULL;
//		}
//
//		// Add a null DACL to the security descriptor.
//		if (!SetSecurityDescriptorDacl(mp_NullDesc, TRUE, (PACL) NULL, FALSE)) {
//			mn_LastError = GetLastError();
//			LocalFree(mp_NullDesc); mp_NullDesc = NULL;
//			return NULL;
//		}
//
//		m_NullSecurity.nLength = sizeof(m_NullSecurity);
//		m_NullSecurity.lpSecurityDescriptor = mp_NullDesc;
//		m_NullSecurity.bInheritHandle = TRUE;
//
//		return (&m_NullSecurity);
//	};
//};
//MNullDesc *gNullDesc = NULL;
//
//SECURITY_ATTRIBUTES* NullSecurity()
//{
//	if (!gNullDesc) gNullDesc = new MNullDesc();
//	return gNullDesc->NullSecurity();
//}
//
//void CommonShutdown()
//{
//	if (gNullDesc) {
//		delete gNullDesc;
//		gNullDesc = NULL;
//	}
//
//	// Clean memory
//	if (ghConsoleSection) {
//		CloseHandle(ghConsoleSection);
//		ghConsoleSection = NULL;
//	}
//	if (gpConsoleInfoStr) {
//		free(gpConsoleInfoStr);
//		gpConsoleInfoStr = NULL;
//	}
//}
//
//BOOL IsUserAdmin()
//{
//	OSVERSIONINFO osv = {sizeof(OSVERSIONINFO)};
//	GetVersionEx(&osv);
//	// Проверять нужно только для висты, чтобы на XP лишний "Щит" не отображался
//	if (osv.dwMajorVersion < 6)
//		return FALSE;
//
//	BOOL b;
//	SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};
//	PSID AdministratorsGroup;
//
//	b = AllocateAndInitializeSid(
//		&NtAuthority,
//		2,
//		SECURITY_BUILTIN_DOMAIN_RID,
//		DOMAIN_ALIAS_RID_ADMINS,
//		0, 0, 0, 0, 0, 0,
//		&AdministratorsGroup);
//	if (b)
//	{
//		if (!CheckTokenMembership(NULL, AdministratorsGroup, &b))
//		{
//			b = FALSE;
//		}
//		FreeSid(AdministratorsGroup);
//	}
//	return(b);
//}
//


#ifndef CONEMU_MINIMAL
WARNING("После перехода на альтернативный сервер - должен работать строго в 'стандартном' режиме (mn_StdMode=STD_OUTPUT_HANDLE/STD_INPUT_HANLDE)");
MConHandle::MConHandle(LPCWSTR asName)
{
	mn_StdMode = 0;
	mb_OpenFailed = FALSE; mn_LastError = 0;
	mh_Handle = INVALID_HANDLE_VALUE;
	mpp_OutBuffer = NULL;
	lstrcpynW(ms_Name, asName, 9);
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
		return *mpp_OutBuffer;
	}

	if (mh_Handle == INVALID_HANDLE_VALUE)
	{
		if (mn_StdMode)
		{
			mh_Handle = GetStdHandle(mn_StdMode);
			return mh_Handle;
		}

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
		}
	}

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
			mh_Handle = INVALID_HANDLE_VALUE;
		}
		else
		{
			HANDLE h = mh_Handle;
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



#ifndef CONEMU_MINIMAL
MFileLog::MFileLog(LPCWSTR asName, LPCWSTR asDir /*= NULL*/, DWORD anPID /*= 0*/)
{
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
}
void MFileLog::CloseLogFile()
{
	SafeCloseHandle(mh_LogFile);

	SafeFree(ms_FilePathName);
	SafeFree(ms_FileName);
}
// Returns 0 if succeeded, otherwise - GetLastError() code
HRESULT MFileLog::CreateLogFile(LPCWSTR asName /*= NULL*/, DWORD anPID /*= 0*/)
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

	wchar_t szVer[2] = {MVV_4a[0],0}, szConEmu[64];
	_wsprintf(szConEmu, SKIPLEN(countof(szConEmu)) L"ConEmu %u%02u%02u%s[%s] log", MVV_1,MVV_2,MVV_3,szVer,WIN3264TEST(L"32",L"64"));

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

	size_t cchTextLen = asText ? lstrlen(asText) : 0;
	size_t cchThrdLen = asThreadName ? lstrlen(asThreadName) : 0;
	size_t cchMax = (cchTextLen + cchThrdLen)*3 + 32;
	char szBuffer[1024];
	char* pszBuffer;
	if (cchMax < countof(szBuffer))
	{
		pszBuffer = szBuffer;
	}
	else
	{
		pszBuffer = (char*)malloc(cchMax);
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
void MFileLog::LogStartEnv(CEStartupEnv* apStartEnv)
{
	if (!apStartEnv || (apStartEnv->cbSize < sizeof(*apStartEnv)))
	{
		LogString(L"LogStartEnv failed, invalid apStartEnv");
		return;
	}

	// Пишем инфу
	wchar_t szSI[MAX_PATH*4], szDesktop[128], szTitle[128];
	lstrcpyn(szDesktop, apStartEnv->si.lpDesktop ? apStartEnv->si.lpDesktop : L"", countof(szDesktop));
	lstrcpyn(szTitle, apStartEnv->si.lpTitle ? apStartEnv->si.lpTitle : L"", countof(szTitle));

	BOOL bWin64 = IsWindows64();

	OSVERSIONINFOEXW osv = {sizeof(osv)};
	GetVersionEx((OSVERSIONINFOW*)&osv);

	LPCWSTR pszReactOS = osv.szCSDVersion + lstrlen(osv.szCSDVersion) + 1;
	if (!*pszReactOS)
		pszReactOS++;

	//wchar_t cVer = MVV_4a[0];
	_wsprintf(szSI, SKIPLEN(countof(szSI)) L"Startup info\r\n"
		L"\tOsVer: %u.%u.%u.x%u, Product: %u, SP: %u.%u, Suite: 0x%X, SM_SERVERR2: %u\r\n"
		L"\tCSDVersion: %s, ReactOS: %u (%s), Rsrv: %u\r\n"
		L"\tDBCS: %u, WINE: %u, ACP: %u, OEMCP: %u\r\n"
		L"\tDesktop: %s\r\n\tTitle: %s\r\n\tSize: {%u,%u},{%u,%u}\r\n"
		L"\tFlags: 0x%08X, ShowWindow: %u\r\n\tHandles: 0x%08X, 0x%08X, 0x%08X"
		,
		osv.dwMajorVersion, osv.dwMinorVersion, osv.dwBuildNumber, bWin64 ? 64 : 32,
		osv.wProductType, osv.wServicePackMajor, osv.wServicePackMinor, osv.wSuiteMask, GetSystemMetrics(89/*SM_SERVERR2*/),
		osv.szCSDVersion, apStartEnv->bIsReactOS, pszReactOS, osv.wReserved,
		apStartEnv->bIsDbcs, apStartEnv->bIsWine,
		apStartEnv->nAnsiCP, apStartEnv->nOEMCP,
		szDesktop, szTitle,
		apStartEnv->si.dwX, apStartEnv->si.dwY, apStartEnv->si.dwXSize, apStartEnv->si.dwYSize,
		apStartEnv->si.dwFlags, (DWORD)apStartEnv->si.wShowWindow,
		(DWORD)apStartEnv->si.hStdInput, (DWORD)apStartEnv->si.hStdOutput, (DWORD)apStartEnv->si.hStdError
		);
	LogString(szSI, true);
	
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




#ifndef CONEMU_MINIMAL
static BOOL CALLBACK MyEnumMonitors(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	LPRECT lprc = (LPRECT)dwData;

	if (lprcMonitor->left < lprc->left)
		lprc->left = lprcMonitor->left;

	if (lprcMonitor->top < lprc->top)
		lprc->top = lprcMonitor->top;

	if (lprcMonitor->right > lprc->right)
		lprc->right = lprcMonitor->right;

	if (lprcMonitor->bottom > lprc->bottom)
		lprc->bottom = lprcMonitor->bottom;

	return TRUE;
}
#endif



// Undocumented console message
#define WM_SETCONSOLEINFO			(WM_USER+201)

//#pragma pack(push, 1)
#include <pshpack1.h>

//
//	Structure to send console via WM_SETCONSOLEINFO
//
typedef struct _CONSOLE_INFO
{
	ULONG		Length;
	COORD		ScreenBufferSize;
	COORD		WindowSize;
	ULONG		WindowPosX;
	ULONG		WindowPosY;

	COORD		FontSize;
	ULONG		FontFamily;
	ULONG		FontWeight;
	WCHAR		FaceName[32];

	ULONG		CursorSize;
	ULONG		FullScreen;
	ULONG		QuickEdit;
	ULONG		AutoPosition;
	ULONG		InsertMode;

	USHORT		ScreenColors;
	USHORT		PopupColors;
	ULONG		HistoryNoDup;
	ULONG		HistoryBufferSize;
	ULONG		NumberOfHistoryBuffers;

	COLORREF	ColorTable[16];

	ULONG		CodePage;
	HWND		Hwnd;

	WCHAR		ConsoleTitle[0x100];

} CONSOLE_INFO;

#ifndef CONEMU_MINIMAL
CONSOLE_INFO* gpConsoleInfoStr = NULL;
HANDLE ghConsoleSection = NULL;
const UINT gnConsoleSectionSize = sizeof(CONSOLE_INFO)+1024;
#endif

//#pragma pack(pop)
#include <poppack.h>

#ifndef CONEMU_MINIMAL
void WINAPI ShutdownConsole()
{
	if (ghConsoleSection)
	{
		CloseHandle(ghConsoleSection);
		ghConsoleSection = NULL;
	}

	if (gpConsoleInfoStr)
	{
		LocalFree(gpConsoleInfoStr);
		gpConsoleInfoStr = NULL;
	}
}
#endif


//
//	SETCONSOLEINFO.C
//
//	Undocumented method to set console attributes at runtime
//
//	For Vista use the newly documented SetConsoleScreenBufferEx API
//
//	www.catch22.net
//

#ifndef CONEMU_MINIMAL
//
//	Wrapper around WM_SETCONSOLEINFO. We need to create the
//  necessary section (file-mapping) object in the context of the
//  process which owns the console, before posting the message
//
BOOL SetConsoleInfo(HWND hwndConsole, CONSOLE_INFO *pci)
{
	DWORD   dwConsoleOwnerPid, dwCurProcId;
	PVOID   ptrView = 0;
	DWORD   dwLastError=0;
	WCHAR   ErrText[255];
	//
	//	Retrieve the process which "owns" the console
	//
	dwCurProcId = GetCurrentProcessId();
	
	DEBUGTEST(DWORD dwConsoleThreadId =)
	GetWindowThreadProcessId(hwndConsole, &dwConsoleOwnerPid);

	// We'll fail, if console was created by other process
	if (dwConsoleOwnerPid != dwCurProcId)
	{
		#ifdef _DEBUG
		// Wine related
		PROCESSENTRY32W pi = {};
		GetProcessInfo(dwConsoleOwnerPid, &pi);
		if (lstrcmpi(pi.szExeFile, L"wineconsole.exe")!=0)
		{
			wchar_t szDbgMsg[512], szTitle[128];
			szDbgMsg[0] = 0;
			GetModuleFileName(NULL, szDbgMsg, countof(szDbgMsg));
			msprintf(szTitle, countof(szTitle), L"%s: PID=%u", PointToName(szDbgMsg), GetCurrentProcessId());
			msprintf(szDbgMsg, countof(szDbgMsg), L"GetWindowThreadProcessId()\nPID=%u, TID=%u, %s\n%s",
				dwConsoleOwnerPid, dwConsoleThreadId,
				pi.szExeFile, szTitle);
			MessageBox(NULL, szDbgMsg, szTitle, MB_SYSTEMMODAL);
		}
		//_ASSERTE(dwConsoleOwnerPid == dwCurProcId);
		#endif

		return FALSE;
	}

	//
	// Create a SECTION object backed by page-file, then map a view of
	// this section into the owner process so we can write the contents
	// of the CONSOLE_INFO buffer into it
	//
	if (!ghConsoleSection)
	{
		ghConsoleSection = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, gnConsoleSectionSize, 0);

		if (!ghConsoleSection)
		{
			dwLastError = GetLastError();
			_wsprintf(ErrText, SKIPLEN(countof(ErrText)) L"Can't CreateFileMapping(ghConsoleSection). ErrCode=%i", dwLastError);
			MessageBox(NULL, ErrText, L"ConEmu", MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
			return FALSE;
		}

		_ASSERTE(OnShutdownConsole==NULL || OnShutdownConsole==ShutdownConsole);
		OnShutdownConsole = ShutdownConsole;
	}

	//
	//	Copy our console structure into the section-object
	//
	ptrView = MapViewOfFile(ghConsoleSection, FILE_MAP_WRITE|FILE_MAP_READ, 0, 0, gnConsoleSectionSize);

	if (!ptrView)
	{
		dwLastError = GetLastError();
		_wsprintf(ErrText, SKIPLEN(countof(ErrText)) L"Can't MapViewOfFile. ErrCode=%i", dwLastError);
		MessageBox(NULL, ErrText, L"ConEmu", MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
	}
	else
	{
		_ASSERTE(pci->Length==sizeof(CONSOLE_INFO));
		//2010-09-19 что-то на XP стало окошко мелькать.
		// при отсылке WM_SETCONSOLEINFO консоль отображается :(
		BOOL lbWasVisible = IsWindowVisible(hwndConsole);
		RECT rcOldPos = {0}, rcAllMonRect = {0};

		if (!lbWasVisible)
		{
			GetWindowRect(hwndConsole, &rcOldPos);
			// В много-мониторных конфигурациях координаты на некоторых могут быть отрицательными!
			EnumDisplayMonitors(NULL, NULL, MyEnumMonitors, (LPARAM)&rcAllMonRect);
			pci->AutoPosition = FALSE;
			pci->WindowPosX = rcAllMonRect.left - 1280;
			pci->WindowPosY = rcAllMonRect.top - 1024;
		}

		memcpy(ptrView, pci, pci->Length); //-V106
		UnmapViewOfFile(ptrView);

		//  Send console window the "update" message
		DEBUGTEST(LRESULT dwConInfoRc =)
		SendMessage(hwndConsole, WM_SETCONSOLEINFO, (WPARAM)ghConsoleSection, 0);

		DEBUGTEST(DWORD dwConInfoErr = GetLastError());

		if (!lbWasVisible && IsWindowVisible(hwndConsole))
		{
			//DEBUGTEST(Sleep(10));

			ShowWindow(hwndConsole, SW_HIDE);
			//SetWindowPos(hwndConsole, NULL, rcOldPos.left, rcOldPos.top, 0,0, SWP_NOSIZE|SWP_NOZORDER);
			// -- чтобы на некоторых системах не возникала проблема с позиционированием -> {0,0}
			// Issue 274: Окно реальной консоли позиционируется в неудобном месте
			SetWindowPos(hwndConsole, NULL, 0, 0, 0,0, SWP_NOSIZE|SWP_NOZORDER);
		}
	}

	return TRUE;
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

void ChangeScreenBufferSize(CONSOLE_SCREEN_BUFFER_INFO& sbi, SHORT VisibleX, SHORT VisibleY, SHORT BufferX, SHORT BufferY)
{
	_ASSERTE(BufferX>=VisibleX && VisibleX && BufferY>=VisibleY && VisibleY);
	sbi.dwSize.X = BufferX;
	sbi.dwSize.Y = BufferY;
	sbi.srWindow.Right = min(BufferX,(sbi.srWindow.Left+VisibleX))-1;
	sbi.srWindow.Left = max(0,(sbi.srWindow.Right+1-VisibleX));
	sbi.srWindow.Bottom = min(BufferY,(sbi.srWindow.Top+VisibleY))-1;
	sbi.srWindow.Top = max(0,(sbi.srWindow.Bottom+1-VisibleY));
}

// sbi - откуда брать данные
// nCurWidth, nCurHeight - текущие (запомненные) высота и ширина рабочей области
// nCurScroll - текущие (запомненные) флаги
// pnNewWidth, pnNewHeight - результат
// pnScroll - флаги из RealBufferScroll
BOOL GetConWindowSize(const CONSOLE_SCREEN_BUFFER_INFO& sbi, int nCurWidth, int nCurHeight, DWORD nCurScroll, int* pnNewWidth, int* pnNewHeight, DWORD* pnScroll)
{
	DWORD nScroll = rbs_None; // enum RealBufferScroll
	int nNewWidth = 0, nNewHeight = 0;
	
	// Функция возвращает размер ОКНА (видимой области), то есть буфер может быть больше
	
	if (sbi.dwSize.X == nCurWidth)
	{
		nNewWidth = sbi.dwSize.X;
	}
	else
	{
		if (((nCurScroll & rbs_Horz) && (sbi.dwSize.X > nCurWidth))
			|| (sbi.dwSize.X > EvalBufferTurnOnSize(nCurWidth)))
		{
			nNewWidth = sbi.srWindow.Right - sbi.srWindow.Left + 1;
			_ASSERTE(nNewWidth <= sbi.dwSize.X);
		}
		else
		{
			nNewWidth = sbi.dwSize.X;
		}
	}
	// Флаги
	if (/*(sbi.dwSize.X > sbi.dwMaximumWindowSize.X) ||*/ (nNewWidth < sbi.dwSize.X))
	{
		// для проверки условий
		//_ASSERTE((sbi.dwSize.X > sbi.dwMaximumWindowSize.X) && (nNewWidth < sbi.dwSize.X));
		nScroll |= rbs_Horz;
	}


	if (sbi.dwSize.Y == nCurHeight)
	{
		nNewHeight = sbi.dwSize.Y;
	}
	else
	{
		if (((nCurScroll & rbs_Vert) && (sbi.dwSize.Y > nCurHeight))
			|| (sbi.dwSize.Y > EvalBufferTurnOnSize(nCurHeight)))
		{
			nNewHeight = sbi.srWindow.Bottom - sbi.srWindow.Top + 1;
		}
		else
		{
			nNewHeight = sbi.dwSize.Y;
		}
	}
	// Флаги
	if (/*(sbi.dwSize.Y > sbi.dwMaximumWindowSize.Y) ||*/ (nNewHeight < sbi.dwSize.Y))
	{
		// для проверки условий
		//_ASSERTE((sbi.dwSize.Y >= sbi.dwMaximumWindowSize.Y) && (nNewHeight < sbi.dwSize.Y));
		nScroll |= rbs_Vert;
	}

	// Validation
	if ((nNewWidth <= 0) || (nNewHeight <= 0))
	{
		_ASSERTE(nNewWidth>0 && nNewHeight>0);
		return FALSE;
	}
	
	// Result
	if (pnNewWidth)
		*pnNewWidth = nNewWidth;
	if (pnNewHeight)
		*pnNewHeight = nNewHeight;
	if (pnScroll)
		*pnScroll = nScroll;
	
	return TRUE;
	
	//BOOL lbBufferHeight = this->isScroll();

	//// Проверка режимов прокрутки
	//if (!lbBufferHeight)
	//{
	//	if (sbi.dwSize.Y > sbi.dwMaximumWindowSize.Y)
	//	{
	//		lbBufferHeight = TRUE; // однозначное включение прокрутки
	//	}
	//}

	//if (lbBufferHeight)
	//{
	//	if (sbi.srWindow.Top == 0
	//	        && sbi.dwSize.Y == (sbi.srWindow.Bottom + 1)
	//	  )
	//	{
	//		lbBufferHeight = FALSE; // однозначное вЫключение прокрутки
	//	}
	//}

	//// Теперь собственно размеры
	//if (!lbBufferHeight)
	//{
	//	nNewHeight =  sbi.dwSize.Y;
	//}
	//else
	//{
	//	// Это может прийти во время смены размера
	//	if ((sbi.srWindow.Bottom - sbi.srWindow.Top + 1) < MIN_CON_HEIGHT)
	//		nNewHeight = con.nTextHeight;
	//	else
	//		nNewHeight = sbi.srWindow.Bottom - sbi.srWindow.Top + 1;
	//}

	//WARNING("Здесь нужно выполнить коррекцию, если nNewHeight велико - включить режим BufferHeight");

	//if (pbBufferHeight)
	//	*pbBufferHeight = lbBufferHeight;

	//_ASSERTE(nNewWidth>=MIN_CON_WIDTH && nNewHeight>=MIN_CON_HEIGHT);

	//if (!nNewWidth || !nNewHeight)
	//{
	//	Assert(nNewWidth && nNewHeight);
	//	return FALSE;
	//}

	////if (nNewWidth < sbi.dwSize.X)
	////    nNewWidth = sbi.dwSize.X;
	//return TRUE;
}
#endif

#ifndef CONEMU_MINIMAL
//
//	Fill the CONSOLE_INFO structure with information
//  about the current console window
//
void GetConsoleSizeInfo(CONSOLE_INFO *pci)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	HANDLE hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
	BOOL lbRc = GetConsoleScreenBufferInfo(hConsoleOut, &csbi);

	if (lbRc)
	{
		pci->ScreenBufferSize = csbi.dwSize;
		pci->WindowSize.X	  = csbi.srWindow.Right - csbi.srWindow.Left + 1;
		pci->WindowSize.Y	  = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
		// Было... а это координаты окна (хотя включается флажок "Autoposition"
		//pci->WindowPosX	      = csbi.srWindow.Left;
		//pci->WindowPosY		  = csbi.srWindow.Top;
	}
	else
	{
		_ASSERTE(lbRc);
		pci->ScreenBufferSize.X = pci->WindowSize.X = 80;
		pci->ScreenBufferSize.Y = pci->WindowSize.Y = 25;
	}

	// Поскольку включен флажок "AutoPosition" - то это игнорируется
	pci->WindowPosX = pci->WindowPosY = 0;
	/*
	RECT rcWnd = {0}; GetWindowRect(Get ConsoleWindow(), &rcWnd);
	pci->WindowPosX	      = rcWnd.left;
	pci->WindowPosY		  = rcWnd.top;
	*/
}
#endif


#if defined(__GNUC__)
#define __in
#define __out
#undef ENABLE_AUTO_POSITION
#endif

//VISTA support:
#ifndef ENABLE_AUTO_POSITION
typedef struct _CONSOLE_FONT_INFOEX
{
	ULONG cbSize;
	DWORD nFont;
	COORD dwFontSize;
	UINT FontFamily;
	UINT FontWeight;
	WCHAR FaceName[LF_FACESIZE];
} CONSOLE_FONT_INFOEX, *PCONSOLE_FONT_INFOEX;
#endif


// Vista+ only
BOOL apiGetConsoleScreenBufferInfoEx(HANDLE hConsoleOutput, MY_CONSOLE_SCREEN_BUFFER_INFOEX* lpConsoleScreenBufferInfoEx)
{
	typedef BOOL (WINAPI* GetConsoleScreenBufferInfoEx_t)(HANDLE hConsoleOutput, MY_CONSOLE_SCREEN_BUFFER_INFOEX* lpConsoleScreenBufferInfoEx);
	static GetConsoleScreenBufferInfoEx_t GetConsoleScreenBufferInfoEx_f = NULL;
	static bool bFuncChecked = false;

	if (!bFuncChecked)
	{
		HMODULE hKernel32 = GetModuleHandle(L"kernel32.dll");
		GetConsoleScreenBufferInfoEx_f = (GetConsoleScreenBufferInfoEx_t)GetProcAddress(hKernel32, "GetConsoleScreenBufferInfoEx");
		bFuncChecked = true;
	}

	BOOL lbRc = FALSE;

	if (GetConsoleScreenBufferInfoEx_f)
	{
		lbRc = GetConsoleScreenBufferInfoEx_f(hConsoleOutput, lpConsoleScreenBufferInfoEx);
	}

	return lbRc;
}

// Vista+ only
// Функция глюкавая. По крайней мере в Win7.
// 1. После ее вызова слетает видимая область в окне консоли
// 2. После ее вызова окно консоли безусловно показыватся
// 1 - поправлено здесь, 2 - озаботиться должен вызывающий
BOOL apiSetConsoleScreenBufferInfoEx(HANDLE hConsoleOutput, MY_CONSOLE_SCREEN_BUFFER_INFOEX* lpConsoleScreenBufferInfoEx)
{
	typedef BOOL (WINAPI* SetConsoleScreenBufferInfoEx_t)(HANDLE hConsoleOutput, MY_CONSOLE_SCREEN_BUFFER_INFOEX* lpConsoleScreenBufferInfoEx);
	static SetConsoleScreenBufferInfoEx_t SetConsoleScreenBufferInfoEx_f = NULL;
	static bool bFuncChecked = false;

	if (!bFuncChecked)
	{
		HMODULE hKernel32 = GetModuleHandle(L"kernel32.dll");
		SetConsoleScreenBufferInfoEx_f = (SetConsoleScreenBufferInfoEx_t)GetProcAddress(hKernel32, "SetConsoleScreenBufferInfoEx");
		bFuncChecked = true;
	}

	BOOL lbRc = FALSE;
	DEBUGTEST(BOOL lbWnd = FALSE);

	if (SetConsoleScreenBufferInfoEx_f)
	{
		lbRc = SetConsoleScreenBufferInfoEx_f(hConsoleOutput, lpConsoleScreenBufferInfoEx);

		// Win7 x64 - глюк. после вызова этой функции идет срыв размеров видимой области.
		DEBUGTEST(lbWnd =)
		SetConsoleWindowInfo(hConsoleOutput, TRUE, &lpConsoleScreenBufferInfoEx->srWindow);
	}

	return lbRc;
}

// Vista+ only
BOOL apiGetConsoleFontSize(HANDLE hOutput, int &SizeY, int &SizeX, wchar_t (&rsFontName)[LF_FACESIZE])
{
	HMODULE hKernel = GetModuleHandle(L"kernel32.dll");

	if (!hKernel)
	{
		_ASSERTE(hKernel!=NULL);
		return FALSE;
	}

	BOOL lbRc = FALSE;

	typedef BOOL (WINAPI *PGetCurrentConsoleFontEx)(HANDLE /*hConsoleOutput*/, BOOL /*bMaximumWindow*/, PCONSOLE_FONT_INFOEX /*lpConsoleCurrentFontEx*/);
	PGetCurrentConsoleFontEx GetCurrentConsoleFontEx = (PGetCurrentConsoleFontEx)
	        GetProcAddress(hKernel, "GetCurrentConsoleFontEx");

	if (GetCurrentConsoleFontEx)  // We have Vista
	{
		CONSOLE_FONT_INFOEX cfi = {sizeof(cfi)};
		lbRc = GetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
		if (lbRc)
		{
			SizeX = cfi.dwFontSize.X;
			SizeY = cfi.dwFontSize.Y;
			lstrcpynW(rsFontName, cfi.FaceName, countof(rsFontName));
		}
	}

	return lbRc;
}

// Vista+ only
BOOL apiSetConsoleFontSize(HANDLE hOutput, int inSizeY, int inSizeX, const wchar_t *asFontName)
{
	HMODULE hKernel = GetModuleHandle(L"kernel32.dll");

	if (!hKernel)
	{
		_ASSERTE(hKernel!=NULL);
		return FALSE;
	}

	BOOL lbRc = FALSE;

	typedef BOOL (WINAPI *PSetCurrentConsoleFontEx)(HANDLE /*hConsoleOutput*/, BOOL /*bMaximumWindow*/, PCONSOLE_FONT_INFOEX /*lpConsoleCurrentFontEx*/);
	PSetCurrentConsoleFontEx SetCurrentConsoleFontEx = (PSetCurrentConsoleFontEx)
	        GetProcAddress(hKernel, "SetCurrentConsoleFontEx");

	if (SetCurrentConsoleFontEx)  // We have Vista
	{
		CONSOLE_FONT_INFOEX cfi = {sizeof(cfi)};
		//GetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
		cfi.dwFontSize.X = inSizeX;
		cfi.dwFontSize.Y = inSizeY;
		lstrcpynW(cfi.FaceName, asFontName ? asFontName : L"Lucida Console", countof(cfi.FaceName));
		lbRc = SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
	}

	return lbRc;
}

// Vista+ only
BOOL apiFixFontSizeForBufferSize(HANDLE hOutput, COORD dwSize)
{
	BOOL lbRetry = FALSE;
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};

	if (dwSize.Y > 0 && dwSize.X > 0
		&& GetConsoleScreenBufferInfo(hOutput, &csbi))
	{
		COORD crLargest = MyGetLargestConsoleWindowSize(hOutput);
		int nMaxX = GetSystemMetrics(SM_CXFULLSCREEN);
		int nMaxY = GetSystemMetrics(SM_CYFULLSCREEN);

		#ifdef _DEBUG
		// Для отладки, смотрим какой размер получится по crLargest
		int nDbgFontX = crLargest.X ? (nMaxX / crLargest.X) : -1;
		int nDbgFontY = crLargest.Y ? (nMaxY / crLargest.Y) : -1;
		#endif
		
		int curSizeY, curSizeX, newSizeY, newSizeX, calcSizeY, calcSizeX;
		wchar_t sFontName[LF_FACESIZE];
		if (apiGetConsoleFontSize(hOutput, curSizeY, curSizeX, sFontName) && curSizeY && curSizeX)
		{
			// Увеличение
			if (crLargest.X && crLargest.Y && ((dwSize.X > crLargest.X) || (dwSize.Y > crLargest.Y)))
			{
				// Теперь прикинуть, какой размер шрифта нам нужен
				newSizeY = max(1,(nMaxY / (dwSize.Y+1)));
				newSizeX = max(1,(nMaxX / (dwSize.X+1)));
				if ((newSizeY < curSizeY) || (newSizeX < curSizeX))
				{
					calcSizeX = newSizeY * curSizeX / curSizeY;
					calcSizeY = newSizeX * curSizeY / curSizeX;
					if (calcSizeY < curSizeY)
						calcSizeX = min(calcSizeX,(calcSizeY * curSizeX / curSizeY));
					if (calcSizeX < curSizeX)
						calcSizeY = min(calcSizeY,(calcSizeX * curSizeY / curSizeX));

					newSizeY = max(1,min(calcSizeY,curSizeY));
					newSizeX = max(1,min(calcSizeX,curSizeX));
					lbRetry = TRUE;
				}
			}
			// Уменьшение
			else if ((dwSize.X <= (csbi.srWindow.Right - csbi.srWindow.Left))
				|| (dwSize.Y <= (csbi.srWindow.Bottom - csbi.srWindow.Top)))
			{
				int nMinY = GetSystemMetrics(SM_CYMIN) - GetSystemMetrics(SM_CYSIZEFRAME) - GetSystemMetrics(SM_CYCAPTION);
				int nMinX = GetSystemMetrics(SM_CXMIN) - 2*GetSystemMetrics(SM_CXSIZEFRAME);
				if ((nMinX > 0) && (nMinY > 0))
				{
					// Теперь прикинуть, какой размер шрифта нам нужен
					newSizeY = (nMinY / (dwSize.Y)) + 1;
					newSizeX = (nMinX / (dwSize.X)) + 1;
					if ((newSizeY > curSizeY) || (newSizeX > curSizeX))
					{
						calcSizeX = newSizeY * curSizeX / curSizeY;
						calcSizeY = newSizeX * curSizeY / curSizeX;
						if (calcSizeY > curSizeY)
							calcSizeX = max(calcSizeX,(calcSizeY * curSizeX / curSizeY));
						if (calcSizeX > curSizeX)
							calcSizeY = max(calcSizeY,(calcSizeX * curSizeY / curSizeX));

						newSizeY = max(calcSizeY,curSizeY);
						newSizeX = max(calcSizeX,curSizeX);
						lbRetry = TRUE;
					}
				}
			}

			if (lbRetry)
			{
				lbRetry = apiSetConsoleFontSize(hOutput, newSizeY, newSizeX, sFontName);
			}
		}
	}

	return lbRetry;
}



#ifndef CONEMU_MINIMAL
void SetConsoleFontSizeTo(HWND inConWnd, int inSizeY, int inSizeX, const wchar_t *asFontName, WORD anTextColors /*= 0*/, WORD anPopupColors /*= 0*/)
{
	OSVERSIONINFO osv = {sizeof(osv)};
	GetVersionEx(&osv);

	if (osv.dwMajorVersion >= 6)
	{
		// We have Vista
		apiSetConsoleFontSize(GetStdHandle(STD_OUTPUT_HANDLE), inSizeY, inSizeX, asFontName);
	}
	else 
	{
		// We have older NT (Win2k, WinXP)
		// So... using undocumented hack

		const COLORREF DefaultColors[16] =
		{
			0x00000000, 0x00800000, 0x00008000, 0x00808000,
			0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0,
			0x00808080,	0x00ff0000, 0x0000ff00, 0x00ffff00,
			0x000000ff, 0x00ff00ff,	0x0000ffff, 0x00ffffff
		};

		if (!gpConsoleInfoStr)
		{
			gpConsoleInfoStr = (CONSOLE_INFO*)LocalAlloc(LPTR, sizeof(CONSOLE_INFO));

			if (!gpConsoleInfoStr)
			{
				_ASSERTE(gpConsoleInfoStr!=NULL);
				return; // memory allocation failed
			}

			gpConsoleInfoStr->Length = sizeof(CONSOLE_INFO);
		}

		if (!anTextColors)
		{
			CONSOLE_SCREEN_BUFFER_INFO csbi = {};
			GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
			anTextColors = csbi.wAttributes ? csbi.wAttributes : MAKEWORD(0x7, 0x0);
		}
		if (!anPopupColors)
		{
			anPopupColors = MAKEWORD(0x5, 0xf);
		}

		int i;
		// get current size/position settings rather than using defaults..
		GetConsoleSizeInfo(gpConsoleInfoStr);
		// set these to zero to keep current settings
		gpConsoleInfoStr->FontSize.X				= inSizeX;
		gpConsoleInfoStr->FontSize.Y				= inSizeY;
		gpConsoleInfoStr->FontFamily				= 0;//0x30;//FF_MODERN|FIXED_PITCH;//0x30;
		gpConsoleInfoStr->FontWeight				= 0;//0x400;
		lstrcpynW(gpConsoleInfoStr->FaceName, asFontName ? asFontName : L"Lucida Console", countof(gpConsoleInfoStr->FaceName)); //-V303
		gpConsoleInfoStr->CursorSize				= 25;
		gpConsoleInfoStr->FullScreen				= FALSE;
		gpConsoleInfoStr->QuickEdit					= FALSE;
		//gpConsoleInfoStr->AutoPosition			= 0x10000;
		gpConsoleInfoStr->AutoPosition				= FALSE;
		RECT rcCon; GetWindowRect(inConWnd, &rcCon);
		gpConsoleInfoStr->WindowPosX = rcCon.left;
		gpConsoleInfoStr->WindowPosY = rcCon.top;
		gpConsoleInfoStr->InsertMode				= TRUE;
		gpConsoleInfoStr->ScreenColors				= anTextColors; //MAKEWORD(0x7, 0x0);
		gpConsoleInfoStr->PopupColors				= anPopupColors; //MAKEWORD(0x5, 0xf);
		gpConsoleInfoStr->HistoryNoDup				= FALSE;
		gpConsoleInfoStr->HistoryBufferSize			= 50;
		gpConsoleInfoStr->NumberOfHistoryBuffers	= 32; //-V112

		// Issue 700: Default history buffers count too small.
		HKEY hkConsole = NULL;
		LONG lRegRc;
		if (0 == (lRegRc = RegCreateKeyEx(HKEY_CURRENT_USER, L"Console\\ConEmu", 0, NULL, 0, KEY_READ, NULL, &hkConsole, NULL)))
		{
			DWORD nSize = sizeof(DWORD), nValue, nType;
			struct {
				LPCWSTR pszName;
				DWORD nMin, nMax;
				ULONG *pnVal;
			} BufferValues[] = {
				{L"HistoryBufferSize", 16, 999, &gpConsoleInfoStr->HistoryBufferSize},
				{L"NumberOfHistoryBuffers", 16, 999, &gpConsoleInfoStr->NumberOfHistoryBuffers}
			};
			for (size_t i = 0; i < countof(BufferValues); ++i)
			{
				lRegRc = RegQueryValueEx(hkConsole, BufferValues[i].pszName, NULL, &nType, (LPBYTE)&nValue, &nSize);
				if ((lRegRc == 0) && (nType == REG_DWORD) && (nSize == sizeof(DWORD)))
				{
					if (nValue < BufferValues[i].nMin)
						nValue = BufferValues[i].nMin;
					else if (nValue > BufferValues[i].nMax)
						nValue = BufferValues[i].nMax;

					if (nValue != *BufferValues[i].pnVal)
						*BufferValues[i].pnVal = nValue;
				}
			}
		}

		// color table
		for(i = 0; i < 16; i++)
			gpConsoleInfoStr->ColorTable[i] = DefaultColors[i];

		gpConsoleInfoStr->CodePage					= GetConsoleOutputCP();//0;//0x352;
		gpConsoleInfoStr->Hwnd						= inConWnd;
		gpConsoleInfoStr->ConsoleTitle[0] = 0;

		// Send data to console window
		SetConsoleInfo(inConWnd, gpConsoleInfoStr);
	}
}
#endif
/*
-- пробовал в Win7 это не работает
void SetConsoleBufferSize(HWND inConWnd, int anWidth, int anHeight, int anBufferHeight)
{
	if (!gpConsoleInfoStr) {
		_ASSERTE(gpConsoleInfoStr!=NULL);
		return; // memory allocation failed
	}

	TODO("Заполнить и другие текущие значения!");
	gpConsoleInfoStr->CodePage					= GetConsoleOutputCP();//0;//0x352;

	// Теперь собственно, что хотим поменять
	gpConsoleInfoStr->ScreenBufferSize.X = gpConsoleInfoStr->WindowSize.X = anWidth;
	gpConsoleInfoStr->WindowSize.Y = anHeight;
	gpConsoleInfoStr->ScreenBufferSize.Y = anBufferHeight;

	SetConsoleInfo(inConWnd, gpConsoleInfoStr);
}
*/

#ifndef CONEMU_MINIMAL
// Evaluate default width for the font
int EvaluateDefaultFontWidth(int inSizeY, const wchar_t *asFontName)
{
	if (inSizeY <= 0)
		return 0;

	int nDefaultX = inSizeY * 10 / 16; // rough
	LOGFONT lf = {inSizeY};
	lstrcpyn(lf.lfFaceName, asFontName ? asFontName : L"Lucida Console", countof(lf.lfFaceName));
	HFONT hFont = CreateFontIndirect(&lf);
	if (hFont)
	{
		HDC hDC = CreateCompatibleDC(NULL);
		if (hDC)
		{
			HFONT hOldF = (HFONT)SelectObject(hDC, hFont);
			TEXTMETRIC tm = {};
			BOOL lbTM = GetTextMetrics(hDC, &tm);
			if (lbTM && (tm.tmAveCharWidth > 0))
			{
				nDefaultX = tm.tmAveCharWidth;
			}
			SelectObject(hDC, hOldF);
			DeleteDC(hDC);
		}
		DeleteObject(hFont);
	}

	return nDefaultX;
}
#endif


#ifndef CONEMU_MINIMAL
void SetUserFriendlyFont(HWND hConWnd)
{
	OSVERSIONINFO OSVer = {sizeof(OSVer)};
	GetVersionEx(&OSVer);

	// Соответствующие функции появились только в API Vista
	// Win2k & WinXP - доступны только хаки, что не подходит
	if (OSVer.dwMajorVersion >= 6)
	{
		HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		COORD crVisibleSize = {};
		CONSOLE_SCREEN_BUFFER_INFO csbi = {};
		if (GetConsoleScreenBufferInfo(hOutput, &csbi))
		{
			crVisibleSize.X = csbi.srWindow.Right - csbi.srWindow.Left + 1;
			crVisibleSize.Y = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
		}

		if ((crVisibleSize.X <= 0) && (crVisibleSize.Y <= 0))
		{
			_ASSERTE((crVisibleSize.X > 0) && (crVisibleSize.Y > 0));
		}
		else
		{
			int curSizeY, curSizeX;
			wchar_t sFontName[LF_FACESIZE];

			if (apiGetConsoleFontSize(hOutput, curSizeY, curSizeX, sFontName) && curSizeY && curSizeX)
			{
				DEBUGTEST(COORD crLargest = MyGetLargestConsoleWindowSize(hOutput));

				HMONITOR hMon = MonitorFromWindow(hConWnd, MONITOR_DEFAULTTOPRIMARY);
				MONITORINFO mi = {sizeof(mi)};
				int nMaxX = 0, nMaxY = 0;
				if (GetMonitorInfo(hMon, &mi))
				{
					nMaxX = mi.rcWork.right - mi.rcWork.left - 2*GetSystemMetrics(SM_CXSIZEFRAME) - GetSystemMetrics(SM_CYCAPTION);
					nMaxY = mi.rcWork.bottom - mi.rcWork.top - 2*GetSystemMetrics(SM_CYSIZEFRAME);
				}
				
				if ((nMaxX > 0) && (nMaxY > 0))
				{
					int nFontX = nMaxX  / crVisibleSize.X;
					int nFontY = nMaxY / crVisibleSize.Y;
					// Too large height?
					if (nFontY > 28)
					{
						nFontX = 28 * nFontX / nFontY;
						nFontY = 28;
					}

					// Evaluate default width for the font
					int nEvalX = EvaluateDefaultFontWidth(nFontY, sFontName);
					if (nEvalX > 0)
					{
						if ((nEvalX > nFontX) && (nFontX > 0))
							nFontY = nFontX * nFontY / nEvalX;
						else
							nFontX = nEvalX;
					}

					// Look in the registry?
					HKEY hk;
					DWORD nRegSize = 0, nLen;
					if (!RegOpenKeyEx(HKEY_CURRENT_USER, L"Console", 0, KEY_READ, &hk))
					{
						if (RegQueryValueEx(hk, L"FontSize", NULL, NULL, (LPBYTE)&nRegSize, &(nLen=sizeof(nRegSize))) || nLen!=sizeof(nRegSize))
							nRegSize = 0;
						RegCloseKey(hk);
					}
					if (!nRegSize && !RegOpenKeyEx(HKEY_CURRENT_USER, L"Console\\%SystemRoot%_system32_cmd.exe", 0, KEY_READ, &hk))
					{
						if (RegQueryValueEx(hk, L"FontSize", NULL, NULL, (LPBYTE)&nRegSize, &(nLen=sizeof(nRegSize))) || nLen!=sizeof(nRegSize))
							nRegSize = 0;
						RegCloseKey(hk);
					}
					if ((HIWORD(nRegSize) > curSizeY) && (HIWORD(nRegSize) < nFontY)
						&& (LOWORD(nRegSize) > curSizeX) && (LOWORD(nRegSize) < nFontX))
					{
						nFontY = HIWORD(nRegSize);
						nFontX = LOWORD(nRegSize);
					}

					if ((nFontX > curSizeX) || (nFontY > curSizeY))
					{
						apiSetConsoleFontSize(hOutput, nFontY, nFontX, sFontName);
					}
				}
			}
		}
	}
}

void CorrectConsolePos(HWND hConWnd)
{
	RECT rcNew = {};
	if (GetWindowRect(hConWnd, &rcNew))
	{
		HMONITOR hMon = MonitorFromWindow(hConWnd, MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO mi = {sizeof(mi)};
		//int nMaxX = 0, nMaxY = 0;
		if (GetMonitorInfo(hMon, &mi))
		{
			int newW = (rcNew.right-rcNew.left), newH = (rcNew.bottom-rcNew.top);
			int newX = rcNew.left, newY = rcNew.top;

			if (newX < mi.rcWork.left)
				newX = mi.rcWork.left;
			else if (rcNew.right > mi.rcWork.right)
				newX = max(mi.rcWork.left,(mi.rcWork.right-newW));

			if (newY < mi.rcWork.top)
				newY = mi.rcWork.top;
			else if (rcNew.bottom > mi.rcWork.bottom)
				newY = max(mi.rcWork.top,(mi.rcWork.bottom-newH));

			if ((newX != rcNew.left) || (newY != rcNew.top))
				SetWindowPos(hConWnd, HWND_TOP, newX, newY,0,0, SWP_NOSIZE);
		}
	}
}

void EmergencyShow(HWND hConWnd)
{
	if (!hConWnd)
	{
		hConWnd = GetConsoleWindow();
		if (IsWindowVisible(hConWnd))
			return; // уже, делать ничего не будем
	}

	if (hConWnd)
	{
		SetUserFriendlyFont(hConWnd);

		CorrectConsolePos(hConWnd);

		if (!IsWindowVisible(hConWnd))
		{
			SetWindowPos(hConWnd, HWND_NOTOPMOST, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE);
			//SetWindowPos(hConWnd, HWND_TOP, 50,50,0,0, SWP_NOSIZE);
			apiShowWindowAsync(hConWnd, SW_SHOWNORMAL);
		}
		else
		{
			// Снять TOPMOST
			SetWindowPos(hConWnd, HWND_NOTOPMOST, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE);
		}

		if (!IsWindowEnabled(hConWnd))
			EnableWindow(hConWnd, true);
	}
}

// используется в GUI при загрузке настроек
void FindComspec(ConEmuComspec* pOpt)
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
		DWORD nExpand = ExpandEnvironmentStrings(L"%ConEmuBaseDir%\tcc.exe", szPath, countof(szPath));
		if (nExpand && (nExpand < countof(szPath)))
		{
			if (FileExists(szPath))
			{
				wcscpy_c(pOpt->Comspec32, szPath);
				wcscpy_c(pOpt->Comspec64, szPath);
			}
		}

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
							memset(szPath, 0, sizeof(szPath)); DWORD nSize = (countof(szPath)-1)*sizeof(szPath[0]);
							if (!RegQueryValueExW(hk2, NULL, NULL, NULL, (LPBYTE)szPath, &nSize) && *szPath)
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

void UpdateComspec(ConEmuComspec* pOpt)
{
	if (!pOpt)
	{
		_ASSERTE(pOpt!=NULL);
		return;
	}

	if (pOpt->isUpdateEnv)
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

	if (pOpt->isAddConEmu2Path)
	{
		if (pOpt->ConEmuBaseDir[0] == 0)
		{
			_ASSERTE(pOpt->ConEmuBaseDir[0] != 0);
		}
		else
		{
			DWORD cchMax = 32767;
			wchar_t* pszCur = (wchar_t*)malloc(cchMax*sizeof(*pszCur));
			wchar_t* pszUpr = (wchar_t*)malloc(cchMax*sizeof(*pszCur));
			wchar_t* pszDirUpr = (wchar_t*)malloc(MAX_PATH*sizeof(*pszCur));

			if (pszCur && pszUpr && pszDirUpr)
			{
				DWORD n = GetEnvironmentVariable(L"PATH", pszCur, cchMax);
				if (n < cchMax)
				{
					if (!n)
						*pszCur = 0;
					lstrcpyn(pszUpr, pszCur, cchMax);
					if (n)
						CharUpperBuff(pszUpr, n);
					lstrcpyn(pszDirUpr, pOpt->ConEmuBaseDir, MAX_PATH);
					CharUpperBuff(pszDirUpr, lstrlen(pszDirUpr));
					int nDirLen = lstrlen(pszDirUpr);
					LPCWSTR pszFind = wcsstr(pszUpr, pszDirUpr);
					if (!(pszFind && (pszFind[nDirLen] == L';' || pszFind[nDirLen] == 0))
						&& ((n + nDirLen + 1) < cchMax))
					{
						lstrcpy(pszUpr, pOpt->ConEmuBaseDir);
						lstrcat(pszUpr, L";");
						lstrcat(pszUpr, pszCur);
						SetEnvironmentVariable(L"PATH", pszUpr);
					}
				}
			}

			SafeFree(pszCur);
			SafeFree(pszUpr);
			SafeFree(pszDirUpr);
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
		_ASSERTE(pOpt && "pOpt должен быть передан, по идее");
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

