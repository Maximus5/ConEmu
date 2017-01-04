
/*
Copyright (c) 2013-2017 Maximus5
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
#include <Windows.h>
#include "../common/defines.h"
#include "../common/MAssert.h"
#include "../common/MSectionSimple.h"
#include "../common/WFiles.h"
#include "../common/WThreads.h"
#include "crc32.h"
#include "ConEmuC.h"
#include "ExitCodes.h"
#include "DownloaderCall.h"

#undef _DOWNLOADER_ASSERT
#define _DOWNLOADER_ASSERT(x) //_ASSERTE(x)

struct LineBuffer
{
	char* ptrData;
	size_t cchMax, cchUsed;

	void AddBlock(const char* ptrRead, size_t cbSize)
	{
		// Enough storage?
		if (!ptrData || ((cchMax - cchUsed) <= cbSize))
		{
			size_t cchNew = cchMax + max(8192,cbSize+1);
			char* ptrNew = (char*)realloc(ptrData, cchNew);
			if (!ptrNew)
				return; // memory allocation failed
			ptrData = ptrNew;
			cchMax = cchNew;
		}

		// Append read data
		size_t cchLen = strlen(ptrRead);
		if (cchLen == cbSize)
		{
			memmove(ptrData+cchUsed, ptrRead, cbSize);
		}
		else
		{
			for (size_t i = cchUsed, j = 0; j < cbSize; i++, j++)
			{
				if ((ptrData[i] = ptrRead[i]) == 0)
				{
					ptrData[i] = ' ';
				}
			}
		}
		cchUsed += cbSize;
		_ASSERTE(cchUsed < cchMax);
		ptrData[cchUsed] = 0;
	};

	bool GetLine(CEStr& szLine)
	{
		if (!ptrData || !cchUsed)
			return false;
		const char* ptrLineEnd = strpbrk(ptrData, "\r\n");
		if (!ptrLineEnd)
			return false; // There is no one completed line yet

		// Multi-line block?
		if ((ptrLineEnd[0] == L'\n') && (ptrLineEnd[1] == L' ' || ptrLineEnd[1] == L'\t'))
		{
			const char* ptrTestEnd = strpbrk(ptrLineEnd+1, "\r\n");
			if (ptrTestEnd)
				ptrLineEnd = ptrTestEnd;
		}

		// \r\n is not expected, but if it happens...
		const char* ptrNewLine = (ptrLineEnd[0] == '\r' && ptrLineEnd[1] == '\n') ? (ptrLineEnd+2) : (ptrLineEnd+1);
		// Alloc the buffer
		size_t cchLen = (ptrLineEnd - ptrData);
		wchar_t* ptrDst = szLine.GetBuffer(cchLen);
		if (!ptrDst)
		{
			return false; // memory allocation failed
		}

		// Return the string
		MultiByteToWideChar(CP_OEMCP, 0, ptrData, cchLen, ptrDst, cchLen);
		ptrDst[cchLen] = 0;
		// Shift the tail
		_ASSERTE((INT_PTR)cchUsed >= (ptrNewLine - ptrData));
		cchUsed -= (ptrNewLine - ptrData);
		_ASSERTE(cchUsed < cchMax);
		memmove(ptrData, ptrNewLine, cchUsed);
		ptrData[cchUsed] = 0;
		// Succeeded
		return true;
	};
};

class CDownloader
{
protected:
	bool mb_AsyncMode;
	
	struct {
		wchar_t* szProxy;
		wchar_t* szProxyUser;
		wchar_t* szProxyPassword;
	} m_Proxy;

	wchar_t* szCmdStringFormat; // "curl -L %1 -o %2", "wget %1 -O %2"

	FDownloadCallback mfn_Callback[dc_LogCallback+1];
	LPARAM m_CallbackLParam[dc_LogCallback+1];

	bool  mb_RequestTerminate;

	// ConEmuC.exe
	STARTUPINFO m_SI;
	PROCESS_INFORMATION m_PI;

public:
	// asProxy = "" - autoconfigure
	// asProxy = "server:port"
	void SetProxy(LPCWSTR asProxy, LPCWSTR asProxyUser, LPCWSTR asProxyPassword)
	{
		SafeFree(m_Proxy.szProxy);
		SafeFree(m_Proxy.szProxyUser);
		if (m_Proxy.szProxyPassword)
			SecureZeroMemory(m_Proxy.szProxyPassword, lstrlen(m_Proxy.szProxyPassword)*sizeof(*m_Proxy.szProxyPassword));
		SafeFree(m_Proxy.szProxyPassword);

		// Empty string - for ‘autoconfig’
		m_Proxy.szProxy = lstrdup(asProxy ? asProxy : L"");

		if (asProxyUser)
			m_Proxy.szProxyUser = lstrdup(asProxyUser);
		if (asProxyPassword)
			m_Proxy.szProxyPassword = lstrdup(asProxyPassword);
	};

	// Logging, errors, download progress
	void SetCallback(CEDownloadCommand cb, FDownloadCallback afnErrCallback, LPARAM lParam)
	{
		if (cb > dc_LogCallback)
		{
			_ASSERTE(cb <= dc_LogCallback && cb >= dc_ErrCallback);
			return;
		}
		mfn_Callback[cb] = afnErrCallback;
		m_CallbackLParam[cb] = lParam;
	};

	void SetCmdStringFormat(LPCWSTR asFormat)
	{
		SafeFree(szCmdStringFormat);
		if (asFormat && *asFormat)
		{
			szCmdStringFormat = lstrdup(asFormat);
		}
	}

	void CloseInternet(bool bFull)
	{
		DWORD nWait = WAIT_OBJECT_0;
		if (m_PI.hProcess)
		{
			nWait = WaitForSingleObject(m_PI.hProcess, 0);
			if (nWait != WAIT_OBJECT_0)
			{
				TerminateProcess(m_PI.hProcess, CERR_DOWNLOAD_FAILED);
			}
		}

		CloseHandles();
	};

	void RequestTerminate()
	{
		mb_RequestTerminate = true;
		CloseInternet(true);
	};

protected:
	void CloseHandles()
	{
		SafeCloseHandle(m_PI.hProcess);
		SafeCloseHandle(m_PI.hThread);
	};

protected:
	bool CalcFileHash(LPCWSTR asFile, DWORD& size, DWORD& crc)
	{
		bool bRc = false;
		HANDLE hFile;
		LARGE_INTEGER liSize = {};
		const DWORD nBufSize = 0x10000;
		BYTE* buf = new BYTE[nBufSize];
		DWORD nReadLeft, nRead, nToRead;

		crc = 0xFFFFFFFF;

		hFile = CreateFile(asFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (!hFile || hFile == INVALID_HANDLE_VALUE)
		{
			Sleep(250);
			hFile = CreateFile(asFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			if (!hFile || hFile == INVALID_HANDLE_VALUE)
				goto wrap;
		}

		if (!GetFileSizeEx(hFile, &liSize) || liSize.HighPart)
			goto wrap;

		size = liSize.LowPart;
		nReadLeft = liSize.LowPart;

		while (nReadLeft)
		{
			nToRead = min(nBufSize, nReadLeft);
			if (!ReadFile(hFile, buf, nToRead, &nRead, NULL) || (nToRead != nRead))
				goto wrap;
			if (!CalcCRC(buf, nRead, crc))
				goto wrap;
			_ASSERTE(nReadLeft >= nRead);
			nReadLeft -= nRead;
		}

		// Succeeded
		crc ^= 0xFFFFFFFF;
		bRc = true;
	wrap:
		delete[] buf;
		SafeCloseHandle(hFile);
		return bRc;
	};

protected:
	/* *** The part related to stdin/out redirection *** */

	struct PipeThreadParm
	{
		CDownloader* pObj;
	};

	bool mb_Terminating;
	HANDLE mh_PipeErrRead, mh_PipeErrWrite, mh_PipeErrThread;
	DWORD mn_PipeErrThreadId;

	static DWORD WINAPI StdErrReaderThread(LPVOID lpParameter)
	{
		CDownloader* pObj = ((PipeThreadParm*)lpParameter)->pObj;
		char Line[4096+1]; // When output is redirected, RAW ASCII is used
		const DWORD dwToRead = countof(Line)-1;
		DWORD dwRead, nValue, nErrCode;
		BOOL bSuccess;

		CEStr szLine;
		const wchar_t *ptr;
		const wchar_t sProgressMark[]    = L" " CEDLOG_MARK_PROGR;
		const wchar_t sInformationMark[] = L" " CEDLOG_MARK_INFO;
		const wchar_t sErrorMark[]       = L" " CEDLOG_MARK_ERROR;

		LineBuffer buffer = {};
		bool bExit = false;

		while (!bExit)
		{
			bSuccess = ReadFile(pObj->mh_PipeErrRead, Line, dwToRead, &dwRead, NULL);
			if (!bSuccess || dwRead == 0)
			{
				nErrCode = GetLastError();
				break;
			}
			_ASSERTE(dwRead < countof(Line));
			Line[dwRead] = 0; // Ensure it is ASCIIZ
			// Append to the line-buffer
			buffer.AddBlock(Line, dwRead);

			// Parse read line
			while (buffer.GetLine(szLine))
			{
				bool bProgress = false;
				if ((ptr = wcsstr(szLine, sProgressMark)) != NULL)
				{
					bProgress = true;
					if (pObj->mfn_Callback[dc_ProgressCallback])
					{
						// 09:01:20.811{1234} Progr: Bytes downloaded 1656
						wchar_t* ptrEnd = NULL;
						LPCWSTR pszFrom = wcspbrk(ptr+wcslen(sProgressMark), L"0123456789");
						nValue = pszFrom ? wcstoul(pszFrom, &ptrEnd, 10) : 0;

						if (nValue)
						{
							CEDownloadInfo progrInfo = {
								sizeof(progrInfo),
								pObj->m_CallbackLParam[dc_ProgressCallback]
							};
							progrInfo.argCount = 1;
							progrInfo.Args[0].argType = at_Uint;
							progrInfo.Args[0].uintArg = nValue;

							pObj->mfn_Callback[dc_ProgressCallback](&progrInfo);
						}
					}
				}

				// For logging purposes
				if (!bProgress && ((ptr = wcsstr(szLine, sErrorMark)) != NULL))
				{
					if (pObj->mfn_Callback[dc_ErrCallback])
					{
						CEDownloadInfo Error = {
							sizeof(Error),
							pObj->m_CallbackLParam[dc_ErrCallback],
							szLine.ms_Val
						};
						pObj->mfn_Callback[dc_ErrCallback](&Error);
					}
				}
				else //if (bProgress || ((ptr = wcsstr(szLine, sInformationMark)) != NULL))
				{
					if (pObj->mfn_Callback[dc_LogCallback])
					{
						CEDownloadInfo Info = {
							sizeof(Info),
							pObj->m_CallbackLParam[dc_LogCallback],
							szLine.ms_Val
						};
						pObj->mfn_Callback[dc_LogCallback](&Info);
					}
					// Exit?
					ptr = wcsstr(szLine, sInformationMark);
					if (ptr)
					{
						ptr += wcslen(sInformationMark);
						if (wcsncmp(ptr, L"Exit", 4) == 0)
						{
							bExit = true;
							break;
						}
					}
				}
			}
		}

		return 0;
	};

	UINT ExecuteDownloader(LPWSTR pszCommand, LPWSTR szCmdDirectory)
	{
		UINT iRc;
		DWORD nWait;
		DWORD nThreadWait = WAIT_TIMEOUT;
		PipeThreadParm threadParm = {this};

		ZeroStruct(m_SI); m_SI.cb = sizeof(m_SI);
		ZeroStruct(m_PI);

		mb_Terminating = false;

		DWORD nCreateFlags = 0
			//| CREATE_NO_WINDOW
			| NORMAL_PRIORITY_CLASS;

		m_SI.dwFlags |= STARTF_USESHOWWINDOW;

		m_SI.wShowWindow = RELEASEDEBUGTEST(SW_HIDE,SW_SHOWNA);

		wchar_t szConTitle[] = L"ConEmu: Downloading from network";
		m_SI.lpTitle = szConTitle;

		if (!szCmdStringFormat || !*szCmdStringFormat)
		{
			// We need to redirect only StdError output

			SECURITY_ATTRIBUTES saAttr = {sizeof(saAttr), NULL, TRUE};
			if (!CreatePipe(&mh_PipeErrRead, &mh_PipeErrWrite, &saAttr, 0))
			{
				iRc = GetLastError();
				_ASSERTE(FALSE && "CreatePipe was failed");
				if (!iRc)
					iRc = E_UNEXPECTED;
				goto wrap;
			}
			// Ensure the read handle to the pipe for STDOUT is not inherited.
			SetHandleInformation(mh_PipeErrRead, HANDLE_FLAG_INHERIT, 0);

			mh_PipeErrThread = apiCreateThread(StdErrReaderThread, (LPVOID)&threadParm, &mn_PipeErrThreadId, "Downloader::ReaderThread");
			if (mh_PipeErrThread != NULL)
			{
				m_SI.dwFlags |= STARTF_USESTDHANDLES;
				// Let's try to change only Error pipe?
				m_SI.hStdError = mh_PipeErrWrite;
			}
		}

		// Now we can run the downloader
		if (!CreateProcess(NULL, pszCommand, NULL, NULL, TRUE/*!Inherit!*/, nCreateFlags, NULL, szCmdDirectory, &m_SI, &m_PI))
		{
			iRc = GetLastError();
			_ASSERTE(FALSE && "Create downloader process was failed");
			if (!iRc)
				iRc = E_UNEXPECTED;
			goto wrap;
		}

		nWait = WaitForSingleObject(m_PI.hProcess, INFINITE);

		if (GetExitCodeProcess(m_PI.hProcess, &nWait)
			&& (nWait == 0)) // CERR_DOWNLOAD_SUCCEEDED is not returned for compatibility purposes
		{
			iRc = 0; // OK
		}
		else
		{
			_DOWNLOADER_ASSERT(nWait == 0 && "Downloader has returned an error");
			iRc = nWait;
		}

	wrap:
		// Finalize reading routine
		mb_Terminating = true;
		if (mh_PipeErrThread)
		{
			nThreadWait = WaitForSingleObject(mh_PipeErrThread, 2500);
			if (nThreadWait == WAIT_TIMEOUT)
			{
				apiCancelSynchronousIo(mh_PipeErrThread);
			}
		}
		SafeCloseHandle(mh_PipeErrRead);
		SafeCloseHandle(mh_PipeErrWrite);
		if (mh_PipeErrThread)
		{
			if (nThreadWait == WAIT_TIMEOUT)
			{
				nThreadWait = WaitForSingleObject(mh_PipeErrThread, 2500);
			}
			if (nThreadWait == WAIT_TIMEOUT)
			{
				_ASSERTE(FALSE && "StdErr reading thread hangs, terminating");
				apiTerminateThread(mh_PipeErrThread, 999);
			}
			SafeCloseHandle(mh_PipeErrThread);
		}
		// Exit
		return iRc;
	};

	wchar_t* CreateCommand(LPCWSTR asSource, LPCWSTR asTarget, UINT& iRc)
	{
		wchar_t* pszCommand = NULL;

		if (!szCmdStringFormat || !*szCmdStringFormat)
		{
			wchar_t szConEmuC[MAX_PATH] = L"", *psz;

			struct _Switches {
				LPCWSTR pszName, pszValue;
			} Switches[] = {
				{L"-proxy", m_Proxy.szProxy},
				{L"-proxylogin", m_Proxy.szProxyUser},
				{L"-proxypassword", m_Proxy.szProxyPassword},
				{L"-async", mb_AsyncMode ? L"Y" : L"N"},
			};

			if (!asSource || !*asSource || !asTarget || !*asTarget)
			{
				iRc = E_INVALIDARG;
				goto wrap;
			}

			if (!GetModuleFileName(ghOurModule, szConEmuC, countof(szConEmuC)) || !(psz = wcsrchr(szConEmuC, L'\\')))
			{
				//ReportMessage(dc_LogCallback, L"GetModuleFileName(ghOurModule) failed, code=%u", at_Uint, GetLastError(), at_None);
				iRc = E_HANDLE;
				goto wrap;
			}
			psz[1] = 0;
			wcscat_c(szConEmuC, WIN3264TEST(L"ConEmuC.exe",L"ConEmuC64.exe"));

			/*
			ConEmuC -download [-debug]
			        [-proxy <address:port> [-proxylogin <name> -proxypassword <pwd>]]
			        "full_url_to_file" "local_path_name"
			*/

			if (!(pszCommand = lstrmerge(L"\"", szConEmuC, L"\" -download ")))
			{
				iRc = E_OUTOFMEMORY;
				goto wrap;
			}

			#if defined(_DEBUG)
			lstrmerge(&pszCommand, L"-debug ");
			#endif

			for (INT_PTR i = 0; i < countof(Switches); i++)
			{
				LPCWSTR pszValue = Switches[i].pszValue;
				if (pszValue
					&& ((*pszValue)
						|| (lstrcmp(Switches[i].pszName, L"-proxy") == 0) // Pass empty string for proxy autoconfig
						)
					&& !lstrmerge(&pszCommand, Switches[i].pszName, L" \"", pszValue, L"\" "))
				{
					iRc = E_OUTOFMEMORY;
					goto wrap;
				}
			}

			if (!lstrmerge(&pszCommand, asSource, L" \"", asTarget, L"\""))
			{
				iRc = E_OUTOFMEMORY;
				goto wrap;
			}
		}
		else
		{
			// Environment string? Expand it
			wchar_t* pszExp = ExpandEnvStr(szCmdStringFormat);

			// "curl -L %1 -o %2"
			// "wget %1 -O %2"
			LPCWSTR Values[] = {asSource, asTarget};
			pszCommand = ExpandMacroValues((pszExp && *pszExp) ? pszExp : szCmdStringFormat, Values, countof(Values));
			SafeFree(pszExp);

			if (!pszCommand)
			{
				iRc = E_OUTOFMEMORY;
				goto wrap;
			}
		}

	wrap:
		return pszCommand;
	}


public:
	UINT DownloadFile(LPCWSTR asSource, LPCWSTR asTarget, DWORD& crc, DWORD& size, BOOL abShowAllErrors = FALSE)
	{
		UINT iRc = E_UNEXPECTED;
		UINT nWait;
		wchar_t* pszCommand = NULL;
		wchar_t* szCmdDirectory = NULL; // Destination directory for file creation

		MCHKHEAP;

		// Split target into directory and file-name
		LPCWSTR pszName = PointToName(asTarget);
		if (pszName > asTarget)
		{
			szCmdDirectory = lstrdup(asTarget);
			if (!szCmdDirectory)
			{
				iRc = E_OUTOFMEMORY;
				goto wrap;
			}
			szCmdDirectory[pszName-asTarget] = 0;
		}

		// Prepare command line for downloader tool
		pszCommand = CreateCommand(asSource, pszName, iRc);
		if (!pszCommand)
		{
			_ASSERTE(iRc!=0);
			goto wrap;
		}

		_ASSERTE(m_PI.hProcess==NULL);
		MCHKHEAP;

		nWait = ExecuteDownloader(pszCommand, szCmdDirectory);

		// Now check the result of downloader proc
		if (nWait != 0)
		{
			iRc = nWait;
			goto wrap;
		}

		if (!CalcFileHash(asTarget, size, crc))
		{
			iRc = GetLastError();
			if (!iRc)
				iRc = E_UNEXPECTED;
			goto wrap;
		}

		iRc = 0; // OK
	wrap:
		MCHKHEAP;
		SafeFree(pszCommand);
		SafeFree(szCmdDirectory);
		CloseHandles();
		return iRc;
	};

public:
	CDownloader()
	{
		DEBUGTEST(gbAllowChkHeap = true);
		mb_AsyncMode = true;
		ZeroStruct(m_Proxy);
		szCmdStringFormat = NULL;
		ZeroStruct(mfn_Callback);
		ZeroStruct(m_CallbackLParam);
		mb_RequestTerminate = false;
		ZeroStruct(m_SI); m_SI.cb = sizeof(m_SI);
		ZeroStruct(m_PI);
		mb_Terminating = false;
		mh_PipeErrRead = mh_PipeErrWrite = mh_PipeErrThread = NULL;
		mn_PipeErrThreadId = 0;
	};

	~CDownloader()
	{
		CloseInternet(true);
		SetProxy(NULL, NULL, NULL);
		SafeFree(szCmdStringFormat);
	};
};

static CDownloader* gpInet = NULL;

#if defined(__GNUC__)
extern "C"
#endif
DWORD_PTR WINAPI DownloadCommand(CEDownloadCommand cmd, int argc, CEDownloadErrorArg* argv)
{
	DWORD_PTR nResult = 0;

	if (!argv) argc = 0;

	switch (cmd)
	{
	case dc_Init:
		if (!gpInet)
			gpInet = new CDownloader;
		nResult = (gpInet != NULL);
		break;
	case dc_Reset:
		if (gpInet)
			gpInet->CloseInternet(false);
		nResult = TRUE;
		break;
	case dc_Deinit:
		SafeDelete(gpInet);
		nResult = TRUE;
		break;
	case dc_SetProxy: // [0]="Server:Port", [1]="User", [2]="Password"
		if (gpInet)
		{
			gpInet->SetProxy(
				(argc > 0 && argv[0].argType == at_Str) ? argv[0].strArg : NULL,
				(argc > 1 && argv[1].argType == at_Str) ? argv[1].strArg : NULL,
				(argc > 2 && argv[2].argType == at_Str) ? argv[2].strArg : NULL);
			nResult = TRUE;
		}
		break;
	case dc_ErrCallback: // [0]=FDownloadCallback, [1]=lParam
	case dc_ProgressCallback: // [0]=FDownloadCallback, [1]=lParam
	case dc_LogCallback: // [0]=FDownloadCallback, [1]=lParam
		if (gpInet)
		{
			gpInet->SetCallback(
				cmd,
				(argc > 0) ? (FDownloadCallback)argv[0].uintArg : 0,
				(argc > 1) ? argv[1].uintArg : 0);
			nResult = TRUE;
		}
		break;
	case dc_DownloadFile: // [0]="http", [1]="DestLocalFilePath", [2]=abShowErrors
		if (gpInet && (argc >= 2))
		{
			DWORD crc = 0, size = 0;
			UINT iDlRc = gpInet->DownloadFile(
				(argc > 0 && argv[0].argType == at_Str) ? argv[0].strArg : NULL,
				(argc > 1 && argv[1].argType == at_Str) ? argv[1].strArg : NULL,
				crc, size,
				(argc > 2) ? argv[2].uintArg : TRUE);
			// Succeeded?
			if (iDlRc == 0)
			{
				argv[0].uintArg = size;
				argv[1].uintArg = crc;
				nResult = TRUE;
			}
			else
			{
				argv[0].uintArg = iDlRc;
				nResult = FALSE;
			}
		}
		break;
	case dc_RequestTerminate:
		if (gpInet)
		{
			gpInet->RequestTerminate();
			nResult = TRUE;
		}
		break;
	case dc_SetCmdString:
		if (gpInet && (argc > 0) && (argv[0].argType == at_Str))
		{
			gpInet->SetCmdStringFormat(argv[0].strArg);
			nResult = TRUE;
		}
		break;

	#ifdef _DEBUG
	default:
		_ASSERTE(FALSE && "Unsupported command!");
	#endif
	}

	return nResult;
}
