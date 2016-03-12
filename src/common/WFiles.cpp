
/*
Copyright (c) 2014-2016 Maximus5
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
#include "Common.h"
#include "MStrDup.h"
#include "WFiles.h"
#include "WObjects.h"

// Returns true, if application was found in registry:
// [HKCU|HKLM]\Software\Microsoft\Windows\CurrentVersion\App Paths
// Also, function may change local process %PATH% variable
bool SearchAppPaths(LPCWSTR asFilePath, CEStr& rsFound, bool abSetPath, CEStr* rpsPathRestore /*= NULL*/)
{
	#if defined(CONEMU_MINIMAL)
	PRAGMA_ERROR("Must not be included in ConEmuHk");
	#endif

	if (rpsPathRestore)
		rpsPathRestore->Empty();

	if (!asFilePath || !*asFilePath)
		return false;

	LPCWSTR pszSearchFile = PointToName(asFilePath);
	LPCWSTR pszExt = PointToExt(pszSearchFile);

	// Lets try find it in "App Paths"
	// "HKCU\Software\Microsoft\Windows\CurrentVersion\App Paths\"
	// "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\"
	LPCWSTR pszRoot = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\";
	HKEY hk; LONG lRc;
	CEStr lsName(pszRoot, pszSearchFile, pszExt ? NULL : L".exe");
	// Seems like 32-bit and 64-bit registry branches are the same now, but just in case - will check both
	DWORD nWOW[2] = {WIN3264TEST(KEY_WOW64_32KEY,KEY_WOW64_64KEY), WIN3264TEST(KEY_WOW64_64KEY,KEY_WOW64_32KEY)};
	for (int i = 0; i < 3; i++)
	{
		bool bFound = false;
		DWORD nFlags = ((i && IsWindows64()) ? nWOW[i-1] : 0);
		if ((i == 2) && !nFlags)
			break; // This is 32-bit OS
		lRc = RegOpenKeyEx(i ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, lsName, 0, KEY_READ|nFlags, &hk);
		if (lRc != 0) continue;
		wchar_t szVal[MAX_PATH+1] = L"";
		DWORD nType, nSize = sizeof(szVal)-sizeof(szVal[0]);
		lRc = RegQueryValueEx(hk, NULL, NULL, &nType, (LPBYTE)szVal, &nSize);
		if (lRc == 0)
		{
			wchar_t *pszCheck = NULL;
			if (nType == REG_SZ)
			{
				pszCheck = szVal;
			}
			else if (nType == REG_EXPAND_SZ)
			{
				pszCheck = ExpandEnvStr(szVal);
			}
			// May be quoted
			if (pszCheck)
			{
				LPCWSTR pszPath = Unquote(pszCheck, true);
				if (FileExists(pszPath))
				{
					// asFilePath will be invalid after .Set
					rsFound.Set(pszPath);
					bFound = true;

					if (pszCheck != szVal)
						free(pszCheck);

					// The program may require additional "%PATH%". So, if allowed...
					if (abSetPath)
					{
						nSize = 0;
						lRc = RegQueryValueEx(hk, L"PATH", NULL, &nType, NULL, &nSize);
						if (lRc == 0 && nSize)
						{
							wchar_t* pszCurPath = GetEnvVar(L"PATH");
							wchar_t* pszAddPath = (wchar_t*)calloc(nSize+4,1);
							wchar_t* pszNewPath = NULL;
							if (pszAddPath)
							{
								lRc = RegQueryValueEx(hk, L"PATH", NULL, &nType, (LPBYTE)pszAddPath, &nSize);
								if (lRc == 0 && *pszAddPath)
								{
									// Если в "%PATH%" этого нет (в начале) - принудительно добавить
									int iCurLen = pszCurPath ? lstrlen(pszCurPath) : 0;
									int iAddLen = lstrlen(pszAddPath);
									bool bNeedAdd = true;
									if ((iCurLen >= iAddLen) && (pszCurPath[iAddLen] == L';' || pszCurPath[iAddLen] == 0))
									{
										wchar_t ch = pszCurPath[iAddLen]; pszCurPath[iAddLen] = 0;
										if (lstrcmpi(pszCurPath, pszAddPath) == 0)
											bNeedAdd = false;
										pszCurPath[iAddLen] = ch;
									}
									// Если пути еще нет
									if (bNeedAdd)
									{
										if (rpsPathRestore)
										{
											rpsPathRestore->SavePathVar(pszCurPath);
										}
										pszNewPath = lstrmerge(pszAddPath, L";", pszCurPath);
										if (pszNewPath)
										{
											SetEnvironmentVariable(L"PATH", pszNewPath);
										}
										else
										{
											_ASSERTE(pszNewPath!=NULL && "Allocation failed?");
										}
									}
								}
							}
							SafeFree(pszAddPath);
							SafeFree(pszCurPath);
							SafeFree(pszNewPath);
						}
					}
				}
			}
		}
		RegCloseKey(hk);

		if (bFound)
			return true;
	}

	return false;
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

	CEStr lsFull;
	if (!apiGetFullPathName(asPath, lsFull))
	{
		_ASSERTEX(FALSE && "GetFullPathName failed");
	}
	else
	{
		pszResult = lsFull.Detach();
	}


	if (!pszResult)
	{
		_ASSERTEX(pszResult!=NULL);
		pszResult = lstrdup(asPath);
	}

	SafeFree(pszTemp);
	return pszResult;
}

bool FindFileName(LPCWSTR asPath, CEStr& rsName)
{
	LPCWSTR pszName = wcsrchr(asPath, L'\\');
	if (!pszName || pszName[1] == 0)
	{
		rsName.Empty();
		return false;
	}
	else
	{
		pszName++;
	}

	rsName.Set(pszName);

	// "." or ".."
	if (IsDotsName(pszName))
	{
		return true;
	}

	bool lbFileFound = false;
	WIN32_FIND_DATAW fnd = {0};
	DWORD nErrCode;
	HANDLE hFind = FindFirstFile(asPath, &fnd);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		// FindFirstFile может обломаться из-за симлинков
		nErrCode = GetLastError();
		if (nErrCode == ERROR_ACCESS_DENIED)
		{
			hFind = CreateFile(asPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			if (hFind == INVALID_HANDLE_VALUE)
				hFind = CreateFile(asPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
			lbFileFound = (hFind && hFind != INVALID_HANDLE_VALUE);
			SafeCloseHandle(hFind);
		}
	}
	else
	{
		// that may be folder or file
		rsName.Set(fnd.cFileName);
		lbFileFound = true;
		FindClose(hFind);
	}

	return lbFileFound;
}

bool MakePathProperCase(CEStr& rsPath)
{
	if (rsPath.IsEmpty())
		return false;

	bool bFound = false;
	// Must be full path here with back-slashes
	_ASSERTE(wcschr(rsPath,L'/')==NULL && wcschr(rsPath,L'\\')!=NULL);

	// Let loop through folders
	wchar_t* psz = rsPath.ms_Val;
	if (psz[0] == L'\\' && psz[1] == L'\\')
	{
		// Network or UNC
		psz += 2;
		if ((psz[0] == L'?' || psz[0] == L'.') && psz[1] == L'\\')
		{
			psz += 2;
			if (psz[0] == L'U' && psz[1] == L'N' && psz[2] == L'C' && psz[3] == L'\\')
				psz += 4;
		}
		if (!(isDriveLetter(psz[0]) && psz[1] == L':'))
		{
			// "Server\Share\" is expected here, just skip them
		}
	}

	if (isDriveLetter(psz[0]) && psz[1] == L':')
	{
		// Uppercase the drive letter
		CharUpperBuff(psz, 1);
		psz += 2;
		if (psz[0] == L'\\')
		{
			psz++; // Skip first slash
		}
		else
		{
			_ASSERTE(psz[0] == L'\\');
		}
	}

	CEStr lsName;
	int iName, iFound;

	while (psz && *psz)
	{
		while (*psz == L'\\') psz++; // Hmm? Double slashes?
		if (!*psz)
			break;

		wchar_t* pszSlash = wcschr(psz, L'\\');
		if (pszSlash)
			*pszSlash = 0;

		bFound = FindFileName(rsPath.ms_Val, lsName);
		if (bFound)
		{
			iName = lstrlen(psz);
			iFound = lstrlen(lsName);
			// If length matches but case differs (do not touch short names like as "PROGRA~1")
			if ((iName == iFound) && lstrcmp(psz, lsName))
			{
				memmove(psz, lsName.ms_Val, sizeof(*psz)*iName);
			}
		}

		if (pszSlash)
			*pszSlash = L'\\';

		if (!pszSlash || !bFound)
			break;
		psz = pszSlash+1;
	}

	return bFound;
}

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

	// If caller requested RAW data
	if (DefaultCP == (DWORD)-1)
	{
		rsBuffer = (wchar_t*)pszDataA;
		rnChars = nRead;
		return 0;
	}

	// Detect codepage
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
		char UTF8BOM[] = {'\xEF','\xBB','\xBF'};
		char CP1200BOM[] = {'\xFF','\xFE'};
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

bool IsDotsName(LPCWSTR asName)
{
	return (asName && (asName[0] == L'.' && (asName[1] == 0 || (asName[1] == L'.' && asName[2] == 0))));
}

// Useful for searching sources from compilation error log
bool FileExistSubDir(LPCWSTR asDirectory, LPCWSTR asFile, int iDepth, CEStr& rsFound)
{
	if (FilesExists(asDirectory, asFile, true, 1))
	{
		rsFound.Attach(JoinPath(asDirectory, asFile));
		return true;
	}
	else if (iDepth <= 0)
	{
		return false;
	}

	CEStr lsFind(JoinPath(asDirectory, L"*"));

	WIN32_FIND_DATAW fnd = {0};
	HANDLE hFind = FindFirstFile(lsFind, &fnd);
	if (!hFind || (hFind == INVALID_HANDLE_VALUE))
	{
		return false;
	}

	bool lbFound = false;

	do
	{
		if ((fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
		{
			// Each find returns "." and ".." items
			// We do not need them
			if (!IsDotsName(fnd.cFileName))
			{
				lsFind.Attach(JoinPath(asDirectory, fnd.cFileName));
				int iChildDepth = (iDepth-1);
				if (lstrcmpi(fnd.cFileName, L".git") == 0 || lstrcmpi(fnd.cFileName, L".svn") == 0)
					iChildDepth = 0; // do not scan children of ".git", ".svn"
				// Recursion
				lbFound = FileExistSubDir(lsFind, asFile,  iChildDepth, rsFound);
				if (lbFound)
					break;
			}
		}
	}
	while (FindNextFile(hFind, &fnd));

	FindClose(hFind);

	return lbFound;
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
			// Each find returns "." and ".." items
			// We do not need them
			if (!IsDotsName(fnd.cFileName))
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

// MyCreateDirectory creates full chain of absent folders
//
// asPath must eigher have trailing slash:
//   "C:\User\Documents\FolderName\"
// or even be a full path to file:
//   "C:\User\Documents\FolderName\file.txt"
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

#if 0
// This cancels ASYNCHRONOUS calls
int apiCancelIoEx(HANDLE hFile, LPOVERLAPPED lpOverlapped)
{
	int iRc = -1;
	BOOL bRc = FALSE;

	typedef BOOL (WINAPI* CancelIoEx_t)(HANDLE hFile, LPOVERLAPPED lpOverlapped);
	static CancelIoEx_t fnCancelIoEx = NULL;
	static bool fnChecked = false;

	if (!fnChecked)
	{
		HMODULE hKernel = GetModuleHandle(L"Kernel32.dll");
		fnCancelIoEx = hKernel ? (CancelIoEx_t)GetProcAddress(hKernel, "CancelIoEx") : NULL;
		fnChecked = false;
	}

	if (fnCancelIoEx)
	{
		bRc = fnCancelIoEx(hFile, lpOverlapped);
		iRc = bRc ? 1 : 0;
	}

	return iRc;
}
#endif

// This cancels SYNCHRONOUS calls
int apiCancelSynchronousIo(HANDLE hThread)
{
	int iRc = -1;
	BOOL bRc = FALSE;

	typedef BOOL (WINAPI* CancelSynchronousIo_t)(HANDLE hThread);
	static CancelSynchronousIo_t fnCancelSynchronousIo = NULL;
	static bool fnChecked = false;

	if (!fnChecked)
	{
		HMODULE hKernel = GetModuleHandle(L"Kernel32.dll");
		fnCancelSynchronousIo = hKernel ? (CancelSynchronousIo_t)GetProcAddress(hKernel, "CancelSynchronousIo") : NULL;
		fnChecked = false;
	}

	if (fnCancelSynchronousIo)
	{
		bRc = fnCancelSynchronousIo(hThread);
		iRc = bRc ? 1 : 0;
	}

	return iRc;
}

// URLZONE_LOCAL_MACHINE = 0
// URLZONE_INTRANET      = 1
// URLZONE_TRUSTED       = 2
// URLZONE_INTERNET      = 3
// URLZONE_UNTRUSTED     = 4
bool HasZoneIdentifier(LPCWSTR asFile, int& nZoneID)
{
	if (!asFile || !*asFile)
	{
		_ASSERTE(asFile && *asFile);
		return false;
	}

	bool bHasZone = false;
	CEStr lsZoneStream(asFile, L":Zone.Identifier");

	#ifdef _DEBUG
	char szData[128] = ""; DWORD nRead = 0; BOOL bRead;
	#endif
	HANDLE hStream = CreateFile(lsZoneStream, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hStream && (hStream != INVALID_HANDLE_VALUE))
	{
		#ifdef _DEBUG
		bRead = ReadFile(hStream, szData, sizeof(szData), &nRead, NULL);
		#endif
		CloseHandle(hStream);

		nZoneID = (int)GetPrivateProfileInt(L"ZoneTransfer", L"ZoneId", 3, lsZoneStream);
		bHasZone = true;
	}

	return bHasZone;
}

bool DropZoneIdentifier(LPCWSTR asFile, DWORD& nErrCode)
{
	CEStr lsZoneStream(asFile, L":Zone.Identifier");

	// First, try to delete "Zone.Identifier" stream
	if (DeleteFile(lsZoneStream))
		return true;
	nErrCode = GetLastError();

	// If failed - try to change it
	if (WritePrivateProfileString(L"ZoneTransfer", L"ZoneId", 0, lsZoneStream))
		return true;
	nErrCode = GetLastError();

	// Access denied?
	return false;
}
