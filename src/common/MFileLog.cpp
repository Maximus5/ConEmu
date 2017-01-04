
/*
Copyright (c) 2009-2017 Maximus5
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

//#define USE_FORCE_FLASH_LOG
#undef USE_FORCE_FLASH_LOG

#define HIDE_USE_EXCEPTION_INFO
#include <windows.h>
#include "defines.h"
#include "MAssert.h"
#include "MFileLog.h"
#include "MSectionSimple.h"
#include "MStrDup.h"
#include "WObjects.h"
#include "../ConEmu/version.h"
#pragma warning(disable: 4091)
#include <shlobj.h>

#if !defined(CONEMU_MINIMAL)
#include "StartupEnvEx.h"
#endif

#ifdef _DEBUG
#define DebugString(x) //OutputDebugString(x)
#define DebugStringA(x) //OutputDebugStringA(x)
#define DEBUGSTRLOG(x) OutputDebugStringA(x)
#else
#define DebugString(x) //OutputDebugString(x)
#define DebugStringA(x) //OutputDebugStringA(x)
#define DEBUGSTRLOG(x)
#endif


MFileLog::MFileLog(LPCWSTR asName, LPCWSTR asDir /*= NULL*/, DWORD anPID /*= 0*/)
{
	mp_Acquired = NULL;
	mpcs_Lock = new MSectionSimple(true);
	mh_LogFile = NULL;
	ms_FilePathName = NULL;
	ms_DefPath = (asDir && *asDir) ? lstrdup(asDir) : NULL;
	ZeroStruct(mst_LastWrite);
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
}

MFileLog::~MFileLog()
{
	if (mp_Acquired)
	{
		_ASSERTE(mp_Acquired==NULL && "Must be already released!");
		mp_Acquired->pLog = NULL;
		mp_Acquired->hLogFile = NULL;
		mp_Acquired->lock.Unlock();
		mp_Acquired = NULL;
	}
	CloseLogFile();
	SafeFree(ms_DefPath);
	SafeDelete(mpcs_Lock);
}

bool MFileLog::IsLogOpened()
{
	return (mh_LogFile && (mh_LogFile != INVALID_HANDLE_VALUE));
}

void MFileLog::CloseLogFile()
{
	if (IsLogOpened())
		LogString(L"Closing log file");

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
	wchar_t szConEmu[128];
	GetLocalTime(&mst_LastWrite);
	_wsprintf(szConEmu, SKIPLEN(countof(szConEmu))
		L"%i-%02i-%02i %i:%02i:%02i.%03i ConEmu %u%02u%02u%s%s[%s%s] log%s",
		mst_LastWrite.wYear, mst_LastWrite.wMonth, mst_LastWrite.wDay,
		mst_LastWrite.wHour, mst_LastWrite.wMinute, mst_LastWrite.wSecond, mst_LastWrite.wMilliseconds,
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
			if (!mh_LogFile || (mh_LogFile == INVALID_HANDLE_VALUE))
			{
				dwErr = GetLastError();
				_ASSERTEX(FALSE && "Access denied?"); // ERROR_ACCESS_DENIED/*5*/?
				// Clean variable to try Desktop folder
				mh_LogFile = NULL;
				SafeFree(ms_FilePathName);
				UNREFERENCED_PARAMETER(dwErr);
			}
		}

		// If folder was not specified or is not writable
		if (mh_LogFile == NULL)
		{
			wchar_t szDesktop[MAX_PATH+1] = L"";

			typedef HRESULT (WINAPI* SHGetFolderPathW_t)(HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, LPWSTR pszPath);
			SHGetFolderPathW_t _SHGetFolderPath;
			HRESULT hFolderRc = E_NOTIMPL;

			// To avoid static link in ConEmuHk
			#if !defined(CONEMU_MINIMAL)
				_SHGetFolderPath = SHGetFolderPathW;
			#else
				HMODULE hShell32 = LoadLibrary(L"Shell32.dll");
				_SHGetFolderPath = hShell32 ? (SHGetFolderPathW_t)GetProcAddress(hShell32, "SHGetFolderPathW") : NULL;
				_ASSERTEX(_SHGetFolderPath!=NULL);
			#endif

			if (_SHGetFolderPath)
			{
				hFolderRc = _SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY|CSIDL_FLAG_CREATE, NULL, 0/*SHGFP_TYPE_CURRENT*/, szDesktop);
			}

			#if defined(CONEMU_MINIMAL)
			if (hShell32)
			{
				FreeLibrary(hShell32);
			}
			#endif

			// Check the result
			if (hFolderRc == S_OK)
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
				// Access denied? On desktop? Really?
				if (!mh_LogFile || (mh_LogFile == INVALID_HANDLE_VALUE))
				{
					dwErr = GetLastError();
					_ASSERTEX(FALSE && "Can't create file on desktop");
					mh_LogFile = NULL;
				}
			}
		}
	}
	else
	{
		if (!ms_FilePathName || !*ms_FilePathName)
			return -1;

		// Reopen same file after temporary close?
		// Use append mode, don't clear existing contents.
		mh_LogFile = CreateFileW(ms_FilePathName, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (!mh_LogFile || (mh_LogFile == INVALID_HANDLE_VALUE))
		{
			dwErr = GetLastError();
			_ASSERTEX(FALSE && "Can't open file for writing");
			mh_LogFile = NULL;
		}
	}

	if (!mh_LogFile || (mh_LogFile == INVALID_HANDLE_VALUE))
	{
		mh_LogFile = NULL;
		return (dwErr ? dwErr : -1);
	}

	LogString(szConEmu, false);

	return 0; // OK
}

LPCWSTR MFileLog::GetLogFileName()
{
	if (!this)
		return L"<NULL>";

	return (ms_FilePathName ? ms_FilePathName : L"<NullFileName>");
}

bool MFileLog::AcquireHandle(LPCWSTR asText, MFileLogHandle& Acquired)
{
	LogString((asText && *asText) ? asText : L"Acquiring LogFile handle", true, NULL, false);
	if (!IsLogOpened())
	{
		LogString(" - FAILED (log was not opened)");
		return false;
	}
	if (InterlockedCompareExchangePointer((PVOID*)&mp_Acquired, &Acquired, NULL))
	{
		LogString(" - FAILED (already acquired)");
		return false;
	}
	if (!mp_Acquired->lock.Lock(mpcs_Lock, 500))
	{
		InterlockedExchangePointer((PVOID*)&mp_Acquired, NULL);
		LogString(" - FAILED (can't lock)");
		return false;
	}
	mp_Acquired->pLog = this;
	mp_Acquired->hLogFile = mh_LogFile;
	return true;
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
	size_t cchMax = (cchTextLen + cchThrdLen)*3 + 80;
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

	size_t cchCur = 0;

	if (abWriteTime)
	{
		SYSTEMTIME st; GetLocalTime(&st);
		char szTime[32];
		if (memcmp(&st, &mst_LastWrite, 4*sizeof(st.wYear)) == 0)
			_wsprintfA(szTime, SKIPLEN(countof(szTime)) "%i:%02i:%02i.%03i ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
		else
			_wsprintfA(szTime, SKIPLEN(countof(szTime)) "%i-%02i-%02i %i:%02i:%02i.%03i ", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
		mst_LastWrite = st;
		INT_PTR dwLen = lstrlenA(szTime);
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
		MSectionLockSimple lock; lock.Lock(mpcs_Lock, 500);
		DWORD dwLen = (DWORD)cchCur;
		WriteFile(mh_LogFile, pszBuffer, dwLen, &dwLen, 0);
		#if defined(USE_FORCE_FLASH_LOG)
		FlushFileBuffers(mh_LogFile);
		#endif
	}

	#ifdef _DEBUG
	DEBUGSTRLOG(pszBuffer);
	#endif

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

#if !defined(CONEMU_MINIMAL)
void MFileLog::LogStartEnvInt(LPCWSTR asText, LPARAM lParam, bool bFirst, bool bNewLine)
{
	MFileLog* p = (MFileLog*)lParam;
	p->LogString(asText, bFirst, NULL, bNewLine);
}

void MFileLog::LogStartEnv(CEStartupEnv* apStartEnv)
{
	LoadStartupEnvEx::ToString(apStartEnv, LogStartEnvInt, (LPARAM)this);
	LogString(L"MFileLog::LogStartEnv finished", true);
}
#endif


MFileLogHandle::MFileLogHandle()
	: pLog(NULL)
	, hLogFile(NULL)
{
}

MFileLogHandle::~MFileLogHandle()
{
	if (pLog)
	{
		InterlockedExchangePointer((PVOID*)&pLog->mp_Acquired, NULL);
		pLog = NULL;
		hLogFile = NULL;
	}
	lock.Unlock();
}

MFileLogHandle::operator HANDLE() const
{
	return hLogFile;
}
