
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

//#define USE_FORCE_FLASH_LOG
#undef USE_FORCE_FLASH_LOG

#define HIDE_USE_EXCEPTION_INFO

#include "defines.h"
#include "MAssert.h"
#include "MModule.h"
#include "MFileLog.h"

#include "CmdLine.h"
#include "MSectionSimple.h"
#include "../ConEmu/version.h"
#include "../common/shlobj.h"

#ifdef _DEBUG
#define DebugString(x) //OutputDebugString(x)
#define DebugStringA(x) //OutputDebugStringA(x)
#define DEBUGSTRLOG(x) OutputDebugStringA(x)
#else
#define DebugString(x) //OutputDebugString(x)
#define DebugStringA(x) //OutputDebugStringA(x)
#define DEBUGSTRLOG(x)
#endif


MFileLog::MFileLog(LPCWSTR asName, LPCWSTR asDir /*= nullptr*/, DWORD anPID /*= 0*/)
{
	defaultPath_.Set(asDir);
	InitFileName(asName, anPID);
}

HRESULT MFileLog::InitFileName(LPCWSTR asName /*= nullptr*/, DWORD anPID /*= 0*/)
{
	if (!anPID) anPID = GetCurrentProcessId();

	if (!asName || !*asName) asName = L"LogFile";

	wchar_t index[20] = L"";
	swprintf_c(index, countof(index), L"-%u", anPID);
	fileName_ = CEStr(asName, index, L".log");
	if (fileName_.IsEmpty())
	{
		_ASSERTEX(!fileName_.IsEmpty());
		return E_UNEXPECTED;
	}

	return S_OK;
}

MFileLog::~MFileLog()
{
	CloseLogFile();
}

bool MFileLog::IsLogOpened() const
{
	return (logHandle_ && (logHandle_ != INVALID_HANDLE_VALUE));
}

void MFileLog::CloseLogFile()
{
	if (IsLogOpened())
		LogString(L"Closing log file");

	SafeCloseHandle(logHandle_);

	filePathName_.Release();
	fileName_.Release();
}

// Returns 0 if succeeded, otherwise - GetLastError() code
HRESULT MFileLog::CreateLogFile(LPCWSTR asName /*= nullptr*/, DWORD anPID /*= 0*/, DWORD anLevel /*= 0*/)
{
	if (!this)
		return -1;

	// fileName_ мог быть проинициализирован в конструкторе, поэтому CloseLogFile не зовем
	if (logHandle_ && logHandle_ != INVALID_HANDLE_VALUE)
	{
		SafeCloseHandle(logHandle_);
	}

	if (asName)
	{
		// А вот если указали новое имя - нужно все передернуть
		CloseLogFile();

		const auto hr = InitFileName(asName, anPID);
		if (FAILED(hr))
			return -1;
	}

	if (!fileName_ || !*fileName_)
	{
		return -1;
	}

	DWORD dwErr = static_cast<DWORD>(-1);

	wchar_t szLevel[16] = L"";
	if (anLevel > 0)
		swprintf_c(szLevel, L"[%u]", anLevel);
	wchar_t szVer4[8] = L"";
	lstrcpyn(szVer4, WSTRING(MVV_4a), countof(szVer4));
	wchar_t szConEmu[128];
	GetLocalTime(&lastWrite_);
	swprintf_c(szConEmu,
		L"%i-%02i-%02i %i:%02i:%02i.%03i ConEmu %u%02u%02u%s%s[%s%s] log%s",
		lastWrite_.wYear, lastWrite_.wMonth, lastWrite_.wDay,
		lastWrite_.wHour, lastWrite_.wMinute, lastWrite_.wSecond, lastWrite_.wMilliseconds,
		MVV_1, MVV_2, MVV_3, szVer4[0]&&szVer4[1]?L"-":L"", szVer4,
		WIN3264TEST(L"32",L"64"), RELEASEDEBUGTEST(L"",L"D"), szLevel);

	if (!filePathName_)
	{
		// Первое открытие. нужно сформировать путь к лог-файлу
		logHandle_ = nullptr;

		if (defaultPath_ && *defaultPath_)
		{
			filePathName_ = JoinPath(defaultPath_, fileName_);
			if (!filePathName_)
				return -1;

			logHandle_ = CreateFileW(filePathName_, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			// Нет прав на запись в текущую папку?
			if (!logHandle_ || (logHandle_ == INVALID_HANDLE_VALUE))
			{
				dwErr = GetLastError();
				_ASSERTEX(FALSE && "Access denied?"); // ERROR_ACCESS_DENIED/*5*/?
				// Clean variable to try Desktop folder
				logHandle_ = nullptr;
				filePathName_.Release();
				UNREFERENCED_PARAMETER(dwErr);
			}
		}

		// If folder was not specified or is not writable
		if (logHandle_ == nullptr)
		{
			wchar_t szDesktop[MAX_PATH+1] = L"";

			typedef HRESULT (WINAPI* SHGetFolderPathW_t)(HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, LPWSTR pszPath);
			SHGetFolderPathW_t shGetFolderPathW{ nullptr };
			HRESULT hFolderRc = E_NOTIMPL;

			// To avoid static link in ConEmuHk
			const MModule hShell32(L"Shell32.dll");
			hShell32.GetProcAddress("SHGetFolderPathW", shGetFolderPathW);
			_ASSERTEX(shGetFolderPathW!=nullptr);

			if (shGetFolderPathW)
			{
				hFolderRc = shGetFolderPathW(nullptr, CSIDL_DESKTOPDIRECTORY|CSIDL_FLAG_CREATE, nullptr, 0/*SHGFP_TYPE_CURRENT*/, szDesktop);
			}

			// Check the result
			if (hFolderRc == S_OK)
			{
				const CEStr desktopLogs = JoinPath(szDesktop, L"ConEmuLogs");
				if (!desktopLogs)
					return -1;
				CreateDirectory(desktopLogs, nullptr);
				filePathName_ = JoinPath(desktopLogs, fileName_);
				if (!filePathName_)
					return -1;

				logHandle_ = CreateFileW(filePathName_, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
				// Access denied? On desktop? Really?
				if (!logHandle_ || (logHandle_ == INVALID_HANDLE_VALUE))
				{
					dwErr = GetLastError();
					_ASSERTEX(FALSE && "Can't create file on desktop");
					logHandle_ = nullptr;
				}
			}
		}
	}
	else
	{
		if (!filePathName_ || !*filePathName_)
			return -1;

		// Reopen same file after temporary close?
		// Use append mode, don't clear existing contents.
		logHandle_ = CreateFileW(filePathName_, GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (!logHandle_ || (logHandle_ == INVALID_HANDLE_VALUE))
		{
			dwErr = GetLastError();
			_ASSERTEX(FALSE && "Can't open file for writing");
			logHandle_ = nullptr;
		}
	}

	if (!logHandle_ || (logHandle_ == INVALID_HANDLE_VALUE))
	{
		logHandle_ = nullptr;
		return (dwErr ? dwErr : -1);
	}

	LogString(szConEmu, false);

	return 0; // OK
}

LPCWSTR MFileLog::GetLogFileName()
{
	if (!this)
		return L"<nullptr>";

	return (filePathName_ ? filePathName_ : L"<NullFileName>");
}

void MFileLog::LogString(LPCSTR asText, bool abWriteTime /*= true*/, LPCSTR asThreadName /*= nullptr*/, bool abNewLine /*= true*/, UINT anCP /*= CP_ACP*/)
{
	if (!this)
		return;

	if (logHandle_ == INVALID_HANDLE_VALUE || logHandle_ == nullptr)
		return;

	wchar_t szInfo[460]; szInfo[0] = 0;
	wchar_t* pszBuf = szInfo;
	wchar_t szThread[32]; szThread[0] = 0;

	if (asText)
	{
		const UINT nLen = lstrlenA(asText);
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

void MFileLog::LogString(LPCWSTR asText, bool abWriteTime /*= true*/, LPCWSTR asThreadName /*= nullptr*/, bool abNewLine /*= true*/)
{
	if (!this) return;

	if (!asText) return;

	//DEBUGTEST(abWriteTime = false);

	size_t cchTextLen = asText ? lstrlen(asText) : 0;
	size_t cchThrdLen = asThreadName ? lstrlen(asThreadName) : 0;
	size_t cchMax = (cchTextLen + cchThrdLen)*3 + 80;
	char szBuffer[1024];
	char *pszBuffer, *pszTemp = nullptr;
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
		if (memcmp(&st, &lastWrite_, 4*sizeof(st.wYear)) == 0)
			sprintf_c(szTime, "%i:%02i:%02i.%03i ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
		else
			sprintf_c(szTime, "%i-%02i-%02i %i:%02i:%02i.%03i ", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
		lastWrite_ = st;
		INT_PTR dwLen = lstrlenA(szTime);
		memmove(pszBuffer+cchCur, szTime, dwLen);
		cchCur += dwLen;
	}

	if (asThreadName && *asThreadName)
	{
		int nLen = WideCharToMultiByte(CP_UTF8, 0, asThreadName, -1, pszBuffer+cchCur, (int)cchThrdLen*3+1, nullptr, nullptr);
		if (nLen > 1) // including terminating null char
			cchCur += (nLen-1);
		pszBuffer[cchCur++] = L' ';
	}

	if (asText && *asText)
	{
		int nLen = WideCharToMultiByte(CP_UTF8, 0, asText, -1, pszBuffer+cchCur, (int)cchTextLen*3+1, nullptr, nullptr);
		if (nLen > 1) // including terminating null char
			cchCur += (nLen-1);
	}

	if (abNewLine)
	{
		memmove(pszBuffer+cchCur, "\r\n", 2);
		cchCur += 2;
	}

	pszBuffer[cchCur] = 0;

	if (logHandle_)
	{
		MSectionLockSimple lock; lock.Lock(&lock_, 500);
		DWORD dwLen = (DWORD)cchCur;
		WriteFile(logHandle_, pszBuffer, dwLen, &dwLen, 0);
		#if defined(USE_FORCE_FLASH_LOG)
		FlushFileBuffers(logHandle_);
		#endif
	}

	#ifdef _DEBUG
	DEBUGSTRLOG(pszBuffer);
	#endif

	SafeFree(pszTemp);

#if 0
	if (!this)
		return;

	if (logHandle_ == INVALID_HANDLE_VALUE || logHandle_ == nullptr)
		return;

	wchar_t szInfo[512]; szInfo[0] = 0;
	SYSTEMTIME st; GetLocalTime(&st);
	swprintf_c(szInfo, L"%i:%02i:%02i.%03i ",
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
	WriteFile(logHandle_, szInfo, nCur*2, &dwLen, 0);
	FlushFileBuffers(logHandle_);
#endif
}
