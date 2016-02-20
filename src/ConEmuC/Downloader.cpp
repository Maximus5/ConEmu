
/*
Copyright (c) 2013-2016 Maximus5
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
#include <Wininet.h>
#include "../common/Common.h"
#include "../common/CmdLine.h"
#include "../common/MSectionSimple.h"
#include "../common/MStrDup.h"
#include "../common/WObjects.h"
#include "../ConEmuCD/crc32.h"
#include "../ConEmuCD/ExitCodes.h"
#include "Downloader.h"

LONG gnIsDownloading = 0;

static bool gbVerbose = false;
static bool gbVerboseInitialized = false;

#ifdef _DEBUG
//#define SLOW_CONNECTION_SIMULATE
#endif

#undef WAIT_FOR_DEBUGGER_MSG

#if defined(__CYGWIN__)
typedef unsigned short u_short;
#define INTERNET_OPTION_HTTP_VERSION 59
typedef struct sockaddr {
    u_short sa_family;              /* address family */
    char    sa_data[14];            /* up to 14 bytes of direct address */
} SOCKADDR, *PSOCKADDR;

#define INTERNET_STATUS_COOKIE_SENT             320
#define INTERNET_STATUS_COOKIE_RECEIVED         321
#define INTERNET_STATUS_COOKIE_HISTORY          327
#define INTERNET_STATUS_INTERMEDIATE_RESPONSE   120
#define INTERNET_STATUS_DETECTING_PROXY         80
#define INTERNET_STATUS_STATE_CHANGE            200
#define INTERNET_STATUS_P3P_HEADER              325
#endif


class CWinInet;

class CDownloader
{
protected:
	CWinInet* wi; // Used
	friend class CWinInet;
	bool mb_InetMode; // Used
	bool mb_AsyncMode;
	bool mb_FtpMode;
	HANDLE mh_Internet, mh_Connect, mh_SrcFile; // Used
	INTERNET_STATUS_CALLBACK mp_SetCallbackRc;
	MSectionSimple mcs_Handle;

	DWORD mn_InternetContentLen, mn_InternetContentReady; // Used

	bool InitInterface();

	struct {
		wchar_t* szProxy;
		wchar_t* szProxyUser;
		wchar_t* szProxyPassword;
	} m_Proxy;

	struct {
		wchar_t* szUser;
		wchar_t* szPassword;
	} m_Server;

	wchar_t* msz_AgentName;

	DWORD mn_Timeout;     // DOWNLOADTIMEOUT by default
	DWORD mn_RecvTimeout; // INTERNET_OPTION_RECEIVE_TIMEOUT
	DWORD mn_DataTimeout; // INTERNET_OPTION_DATA_RECEIVE_TIMEOUT
	bool  SetupTimeouts();

	bool IsLocalFile(LPWSTR& asPathOrUrl);
	bool IsLocalFile(LPCWSTR& asPathOrUrl);

	BOOL ReadSource(LPCWSTR asSource, BOOL bInet, HANDLE hSource, BYTE* pData, DWORD cbData, DWORD* pcbRead);
	BOOL WriteTarget(LPCWSTR asTarget, HANDLE hTarget, const BYTE* pData, DWORD cbData);

	bool SetProxyForHandle(HANDLE hInternet);

	FDownloadCallback mfn_Callback[dc_LogCallback+1];
	LPARAM m_CallbackLParam[dc_LogCallback+1];

	void UpdateProgress();

	void ReportMessage(CEDownloadCommand rm, LPCWSTR asFormat, CEDownloadArgType nextArgType = at_None, ...);

	static VOID CALLBACK InetCallback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength);
	#if 0
	bool WaitAsyncResult();
	#endif

	HANDLE mh_CloseEvent;
	LONG   mn_CloseRef;
	bool   InetCloseHandle(HINTERNET& h, bool bForceSync = false);

	HANDLE mh_ReadyEvent;
	LONG   mn_ReadyRef;
	INTERNET_ASYNC_RESULT m_Result;
	bool ExecRequest(BOOL bResult, DWORD& nErrCode, MSectionLockSimple& CS);
	HINTERNET ExecRequest(HINTERNET hResult, DWORD& nErrCode, MSectionLockSimple& CS);

public:
	CDownloader();
	virtual ~CDownloader();

	void SetProxy(LPCWSTR asProxy, LPCWSTR asProxyUser, LPCWSTR asProxyPassword);
	void SetLogin(LPCWSTR asUser, LPCWSTR asPassword);
	void SetCallback(CEDownloadCommand cb, FDownloadCallback afnErrCallback, LPARAM lParam);
	void SetAsync(bool bAsync);
	void SetTimeout(UINT nWhat, DWORD nTimeout);
	void SetAgent(LPCWSTR aszAgentName);

	BOOL DownloadFile(LPCWSTR asSource, LPCWSTR asTarget, DWORD& crc, DWORD& size, BOOL abShowAllErrors = FALSE);

	void CloseInternet(bool bFull);

	void RequestTerminate();

protected:
	bool mb_RequestTerminate; // Used
};


// Избежать статической линковки к WinInet.dll
class CWinInet
{
public:
	typedef HINTERNET (WINAPI* HttpOpenRequestW_t)(HINTERNET hConnect, LPCWSTR lpszVerb, LPCWSTR lpszObjectName, LPCWSTR lpszVersion, LPCWSTR lpszReferrer, LPCWSTR * lplpszAcceptTypes, DWORD dwFlags, DWORD_PTR dwContext);
	typedef BOOL (WINAPI* HttpQueryInfoW_t)(HINTERNET hRequest, DWORD dwInfoLevel, LPVOID lpBuffer, LPDWORD lpdwBufferLength, LPDWORD lpdwIndex);
	typedef BOOL (WINAPI* HttpSendRequestW_t)(HINTERNET hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength);
	typedef BOOL (WINAPI* InternetCloseHandle_t)(HINTERNET hInternet);
	typedef HINTERNET (WINAPI* InternetConnectW_t)(HINTERNET hInternet, LPCWSTR lpszServerName, INTERNET_PORT nServerPort, LPCWSTR lpszUserName, LPCWSTR lpszPassword, DWORD dwService, DWORD dwFlags, DWORD_PTR dwContext);
	typedef HINTERNET (WINAPI* InternetOpenW_t)(LPCWSTR lpszAgent, DWORD dwAccessType, LPCWSTR lpszProxy, LPCWSTR lpszProxyBypass, DWORD dwFlags);
	typedef BOOL (WINAPI* InternetReadFile_t)(HINTERNET hFile, LPVOID lpBuffer, DWORD dwNumberOfBytesToRead, LPDWORD lpdwNumberOfBytesRead);
	typedef BOOL (WINAPI* InternetSetOptionW_t)(HINTERNET hInternet, DWORD dwOption, LPVOID lpBuffer, DWORD dwBufferLength);
	typedef BOOL (WINAPI* InternetQueryOptionW_t)(HINTERNET hInternet, DWORD dwOption, LPVOID lpBuffer, LPDWORD lpdwBufferLength);
	typedef INTERNET_STATUS_CALLBACK (WINAPI* InternetSetStatusCallbackW_t)(HINTERNET hInternet, INTERNET_STATUS_CALLBACK lpfnInternetCallback);
	typedef BOOL (WINAPI* FtpSetCurrentDirectoryW_t)(HINTERNET hConnect, LPCWSTR lpszDirectory);
	typedef HINTERNET (WINAPI* FtpOpenFileW_t)(HINTERNET hConnect, LPCWSTR lpszFileName, DWORD dwAccess, DWORD dwFlags, DWORD_PTR dwContext);
	//typedef BOOL (WINAPI* DeleteUrlCacheEntryW_t)(LPCWSTR lpszUrlName);



	HttpOpenRequestW_t _HttpOpenRequestW;
	HttpQueryInfoW_t _HttpQueryInfoW;
	HttpSendRequestW_t _HttpSendRequestW;
	InternetCloseHandle_t _InternetCloseHandle;
	InternetConnectW_t _InternetConnectW;
	InternetOpenW_t _InternetOpenW;
	InternetReadFile_t _InternetReadFile;
	InternetSetOptionW_t _InternetSetOptionW;
	InternetQueryOptionW_t _InternetQueryOptionW;
	InternetSetStatusCallbackW_t _InternetSetStatusCallbackW;
	FtpSetCurrentDirectoryW_t _FtpSetCurrentDirectoryW;
	FtpOpenFileW_t _FtpOpenFileW;
	//DeleteUrlCacheEntryW_t _DeleteUrlCacheEntryW;
protected:
	HMODULE _hWinInet;
	bool LoadFuncInt(CDownloader* pUpd, FARPROC* pfn, LPCSTR n)
	{
		char func[64];
		lstrcpyA(func,n);
		for (char* p = func; *p && *(p+1); p+=2) { char c = p[0]; p[0] = p[1]; p[1] = c; }
		*pfn = GetProcAddress(_hWinInet, func);
		if (*pfn == NULL)
		{
			wchar_t name[64];
			MultiByteToWideChar(CP_ACP, 0, func, -1, name, countof(name));
			pUpd->ReportMessage(dc_ErrCallback,
				L"GetProcAddress(%s) failed, code=%u", at_Str, name, at_Uint, GetLastError(), at_None);
			FreeLibrary(_hWinInet);
			_hWinInet = NULL;
			return false;
		}
		return true;
	}
public:
	CWinInet()
	{
		_hWinInet = NULL;
		_HttpOpenRequestW = NULL;
		_HttpQueryInfoW = NULL;
		_HttpSendRequestW = NULL;
		_InternetCloseHandle = NULL;
		_InternetConnectW = NULL;
		_InternetOpenW = NULL;
		_InternetReadFile = NULL;
		_InternetSetOptionW = NULL;
        _InternetQueryOptionW = NULL;
		_InternetSetStatusCallbackW = NULL;
		_FtpSetCurrentDirectoryW = NULL;
		_FtpOpenFileW = NULL;
		//_DeleteUrlCacheEntryW = NULL;
	};
	~CWinInet()
	{
		if (_hWinInet)
			FreeLibrary(_hWinInet);
	};
	bool Init(CDownloader* pUpd)
	{
		if (_hWinInet)
			return true;

		wchar_t name[MAX_PATH] = L"iWInen.tldl";

		for (wchar_t* p = name; *p && *(p+1); p+=2) { wchar_t c = p[0]; p[0] = p[1]; p[1] = c; }
		_hWinInet = LoadLibrary(name);
		if (!_hWinInet)
		{
			pUpd->ReportMessage(dc_ErrCallback,
				L"LoadLibrary(%s) failed, code=%u", at_Str, name, at_Uint, GetLastError(), at_None);
			return false;
		}

		#define LoadFunc(s,n) \
			if (!LoadFuncInt(pUpd, (FARPROC*)&_##s, n)) \
				return false;

		LoadFunc(HttpOpenRequestW,     "tHptpOneeRuqseWt");
		LoadFunc(HttpQueryInfoW,       "tHptuQreIyfnWo");
		LoadFunc(HttpSendRequestW,     "tHpteSdneRuqseWt");
		LoadFunc(InternetCloseHandle,  "nIetnrtelCsoHenalde");
		LoadFunc(InternetConnectW,     "nIetnrteoCnnceWt");
		LoadFunc(InternetSetOptionW,   "nIetnrteeSOttpoiWn");
		LoadFunc(InternetQueryOptionW, "nIetnrteuQreOytpoiWn");
		LoadFunc(InternetOpenW,        "nIetnrtepOneW");
		LoadFunc(InternetReadFile,     "nIetnrteeRdaiFel");
		LoadFunc(InternetSetStatusCallbackW, "nIetnrteeSStatutCslablcaWk");
		LoadFunc(FtpSetCurrentDirectoryW, "tFSpteuCrrneDtriceotyrW");
		LoadFunc(FtpOpenFileW,         "tFOpepFnliWe");
		//LoadFunc(DeleteUrlCacheEntryW, "eDeletrUClcaehnErtWy");

		return true;
	}
};



CDownloader::CDownloader()
{
	mfn_Callback[dc_ErrCallback] = NULL;
	m_CallbackLParam[dc_ErrCallback] = 0;

	memset(mfn_Callback, 0, sizeof(mfn_Callback));
	memset(m_CallbackLParam, 0, sizeof(m_CallbackLParam));

	ZeroStruct(m_Proxy);
	ZeroStruct(m_Server);
	msz_AgentName = NULL;

	mb_AsyncMode = true;
	mb_FtpMode = false;
	mn_Timeout = DOWNLOADTIMEOUT;
	mn_RecvTimeout = mn_DataTimeout = 0; // 0 --> use max as mn_Timeout
	//mh_StopThread = NULL;
	mb_RequestTerminate = false;
	mb_InetMode = false;
	mh_Internet = mh_Connect = mh_SrcFile = NULL;
	mp_SetCallbackRc = NULL;
	mn_InternetContentLen = mn_InternetContentReady = 0;
	wi = NULL;

	mh_CloseEvent = NULL;
	mn_CloseRef = 0;
	mh_ReadyEvent = NULL;
	mn_ReadyRef = 0;
	ZeroStruct(m_Result);

	mcs_Handle.Init();
}

CDownloader::~CDownloader()
{
	CloseInternet(true);
	SetProxy(NULL, NULL, NULL);
	SetLogin(NULL, NULL);
	SafeFree(msz_AgentName);
	SafeCloseHandle(mh_CloseEvent);
	SafeCloseHandle(mh_ReadyEvent);
	mcs_Handle.Close();
}

// asProxy = "" - autoconfigure
// asProxy = "server:port"
void CDownloader::SetProxy(LPCWSTR asProxy, LPCWSTR asProxyUser, LPCWSTR asProxyPassword)
{
	SafeFree(m_Proxy.szProxy);
	SafeFree(m_Proxy.szProxyUser);
	if (m_Proxy.szProxyPassword)
		SecureZeroMemory(m_Proxy.szProxyPassword, lstrlen(m_Proxy.szProxyPassword)*sizeof(*m_Proxy.szProxyPassword));
	SafeFree(m_Proxy.szProxyPassword);

	if (asProxy)
		m_Proxy.szProxy = lstrdup(asProxy);
	if (asProxyUser)
		m_Proxy.szProxyUser = lstrdup(asProxyUser);
	if (asProxyPassword)
		m_Proxy.szProxyPassword = lstrdup(asProxyPassword);
}

void CDownloader::SetLogin(LPCWSTR asUser, LPCWSTR asPassword)
{
	SafeFree(m_Server.szUser);
	if (m_Server.szPassword)
		SecureZeroMemory(m_Server.szPassword, lstrlen(m_Server.szPassword)*sizeof(*m_Server.szPassword));
	SafeFree(m_Server.szPassword);

	if (asUser)
		m_Server.szUser = lstrdup(asUser);
	if (asPassword)
		m_Server.szPassword = lstrdup(asPassword);
}

bool CDownloader::SetProxyForHandle(HANDLE hInternet)
{
	bool bOk = false;

	WARNING("Check proxy!");

	if (m_Proxy.szProxyUser && *m_Proxy.szProxyUser)
	{
		if (!wi->_InternetSetOptionW(hInternet, INTERNET_OPTION_PROXY_USERNAME, (LPVOID)m_Proxy.szProxyUser, lstrlen(m_Proxy.szProxyUser)))
		{
			ReportMessage(dc_ErrCallback,
				L"ProxyUserName failed, code=%u", at_Uint, GetLastError(), at_None);
			goto wrap;
		}
	}
	if (m_Proxy.szProxyPassword && *m_Proxy.szProxyPassword)
	{
		if (!wi->_InternetSetOptionW(hInternet, INTERNET_OPTION_PROXY_PASSWORD, (LPVOID)m_Proxy.szProxyPassword, lstrlen(m_Proxy.szProxyPassword)))
		{
			ReportMessage(dc_ErrCallback,
				L"ProxyPassword failed, code=%u", at_Uint, GetLastError(), at_None);
			goto wrap;
		}
	}

	bOk = true;
wrap:
	return bOk;
}




// This checks if file is located on local drive
// (has "file://" prefix, or "\\server\share\..." or "X:\path\...")
// and set asPathOrUrl back to local path (if prefix was specified)
bool CDownloader::IsLocalFile(LPCWSTR& asPathOrUrl)
{
	LPWSTR psz = (LPWSTR)asPathOrUrl;
	bool lbLocal = IsLocalFile(psz);
	asPathOrUrl = psz;
	return lbLocal;
}

// This checks if file is located on local drive
// (has "file://" prefix, or "\\server\share\..." or "X:\path\...")
// and set asPathOrUrl back to local path (if prefix was specified)
// Function DOES NOT modify the contents of buffer pointed by asPathOrUrl!
bool CDownloader::IsLocalFile(LPWSTR& asPathOrUrl)
{
	if (!asPathOrUrl || !*asPathOrUrl)
	{
		_ASSERTE(asPathOrUrl && *asPathOrUrl);
		return true;
	}

	if (asPathOrUrl[0] == L'-' && asPathOrUrl[1] == 0)
		return true; // Write to StdOut

	if (asPathOrUrl[0] == L'\\' && asPathOrUrl[1] == L'\\')
		return true; // network or UNC
	if (asPathOrUrl[1] == L':')
		return true; // Local drive

	wchar_t szPrefix[8]; // "file:"
	lstrcpyn(szPrefix, asPathOrUrl, countof(szPrefix));
	if (lstrcmpi(szPrefix, L"file://") == 0)
	{
		asPathOrUrl += 7;
		return true; // "file:" protocol
	}

	return false;
}

bool CDownloader::InitInterface()
{
	bool bRc = false;

	if (!wi)
	{
		ReportMessage(dc_LogCallback, L"InitInterface()");
		wi = new CWinInet;
		if (!wi)
		{
			ReportMessage(dc_ErrCallback,
				L"new CWinInet failed (memory allocation failed)", at_None);
			goto wrap;
		}
	}

	if (!wi || !wi->Init(this))
	{
		ReportMessage(dc_ErrCallback,
			L"CWinInet initialization failed, code=%u", at_Uint, GetLastError(), at_None);
		goto wrap;
	}

	if (mb_AsyncMode)
	{
		HANDLE* phEvents[] = {&mh_CloseEvent, &mh_ReadyEvent};
		for (size_t i = 0; i < countof(phEvents); i++)
		{
			if (!*(phEvents[i]))
			{
				*(phEvents[i]) = CreateEvent(NULL, FALSE, FALSE, NULL);
				if (!*(phEvents[i]))
				{
					ReportMessage(dc_ErrCallback,
						L"Create %s event failed, code=%u", at_Str, i?L"ready":L"close", at_Uint, GetLastError(), at_None);
					goto wrap;
				}
			}
		}
	}

	bRc = true;
wrap:
	if (!bRc)
	{
		SafeDelete(wi);
	}
	return bRc;
}

#if 0
bool CDownloader::WaitAsyncResult()
{
	if (!mb_AsyncMode)
		return true;
	//TODO!!! Для отладки, пока просто смотрим, какие функции вызывают какие калбэки
	ReportMessage(dc_LogCallback, L"... waiting 5 sec");
	Sleep(5000);
	return true;
}
#endif

bool CDownloader::InetCloseHandle(HINTERNET& h, bool bForceSync /*= false*/)
{
	if (!h || h == INVALID_HANDLE_VALUE)
		return false;

	DWORD nErrCode = 0, nWaitResult;
	BOOL bClose;

	ReportMessage(dc_LogCallback, L"Close handle x%08X", at_Uint, (DWORD_PTR)h, at_None);
	// Debugging and checking purposes
	DEBUGTEST(LONG lCur =)
	InterlockedIncrement(&mn_CloseRef);
	_ASSERTE(lCur == 1);
	ResetEvent(mh_CloseEvent);

	MSectionLockSimple CS;
	CS.Lock(&mcs_Handle);

	SetLastError(0);
	bClose = wi->_InternetCloseHandle(h);
	nErrCode = GetLastError();
	if (!bClose)
	{
		ReportMessage(dc_LogCallback, L"Close handle x%08X failed, code=%u", at_Uint, (DWORD_PTR)h, at_Uint, nErrCode, at_None);
	}
	if (!bForceSync && mb_AsyncMode
		&& (nErrCode != ERROR_INVALID_HANDLE) // Handles mh_SrcFile and mh_Connect fails in WinXP (bClose==true, nErrCode==ERROR_INVALID_HANDLE)
		)
	{
		nWaitResult = WaitForSingleObject(mh_CloseEvent, DOWNLOADCLOSEHANDLETIMEOUT);
		_ASSERTE(nWaitResult == WAIT_OBJECT_0 && "Handle must be closed properly");
		ReportMessage(dc_LogCallback, L"Async close handle x%08X wait result=%u", at_Uint, (DWORD_PTR)h, at_Uint, nWaitResult, at_None);
	}

	CS.Unlock();

	// Done
	h = NULL;
	InterlockedDecrement(&mn_CloseRef);
	return (bClose != FALSE);
}

bool CDownloader::ExecRequest(BOOL bResult, DWORD& nErrCode, MSectionLockSimple& CS)
{
	DWORD nWaitResult;
	nErrCode = bResult ? 0 : GetLastError();

	CS.Unlock();

	if (mb_AsyncMode && (nErrCode == ERROR_IO_PENDING))
	{
		nWaitResult = WaitForSingleObject(mh_ReadyEvent, DOWNLOADOPERATIONTIMEOUT);
		ReportMessage(dc_LogCallback, L"Async operation (BOOL) wait=%u", at_Uint, nWaitResult, at_None);
		if (nWaitResult == WAIT_OBJECT_0)
		{
			bResult = (m_Result.dwResult != 0);
			nErrCode = m_Result.dwError;
		}
	}

	return (bResult != FALSE);
}

HINTERNET CDownloader::ExecRequest(HINTERNET hResult, DWORD& nErrCode, MSectionLockSimple& CS)
{
	DWORD nWaitResult;
	nErrCode = hResult ? 0 : GetLastError();

	CS.Unlock();

	if (mb_AsyncMode && (nErrCode == ERROR_IO_PENDING))
	{
		nWaitResult = WaitForSingleObject(mh_ReadyEvent, DOWNLOADOPERATIONTIMEOUT);
		ReportMessage(dc_LogCallback, L"Async operation (HANDLE) wait=%u", at_Uint, nWaitResult, at_None);
		if (nWaitResult == WAIT_OBJECT_0)
		{
			hResult = (HINTERNET)m_Result.dwResult;
			nErrCode = m_Result.dwError;
		}
	}

	return hResult;
}

VOID CDownloader::InetCallback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength)
{
    InternetCookieHistory cookieHistory;
    CDownloader* pObj = (CDownloader*)dwContext;
	wchar_t sFormat[200];

    UNREFERENCED_PARAMETER(dwStatusInformationLength);

	wcscpy_c(sFormat, L"InetCallback for handle x%08X: ");
	#define LogCallback(msg,arg) \
		wcscat_c(sFormat, msg);  \
		pObj->ReportMessage(dc_LogCallback, sFormat, at_Uint, (DWORD_PTR)hInternet, at_Uint, arg, at_None);

    switch (dwInternetStatus)
    {
        case INTERNET_STATUS_COOKIE_SENT:
			LogCallback(L"Cookie found and will be sent with request", 0);
            break;

        case INTERNET_STATUS_COOKIE_RECEIVED:
            LogCallback(L"Cookie Received", 0);
            break;

        case INTERNET_STATUS_COOKIE_HISTORY:
			wcscat_c(sFormat, L"Cookie History");

            _ASSERTE(lpvStatusInformation);
            _ASSERTE(dwStatusInformationLength == sizeof(InternetCookieHistory));

            cookieHistory = *((InternetCookieHistory*)lpvStatusInformation);

            if (cookieHistory.fAccepted)
            {
                wcscat_c(sFormat, L": Cookie Accepted");
            }
            if (cookieHistory.fLeashed)
            {
                wcscat_c(sFormat, L": Cookie Leashed");
            }
            if (cookieHistory.fDowngraded)
            {
                wcscat_c(sFormat, L": Cookie Downgraded");
            }
            if (cookieHistory.fRejected)
            {
                wcscat_c(sFormat, L": Cookie Rejected");
            }

			LogCallback(L"", 0);

            break;

        case INTERNET_STATUS_CLOSING_CONNECTION:
            LogCallback(L"Closing Connection", 0);
            break;

        case INTERNET_STATUS_CONNECTING_TO_SERVER:
        case INTERNET_STATUS_CONNECTED_TO_SERVER:
			_ASSERTE(lpvStatusInformation);
			wcscat_c(sFormat, dwInternetStatus==INTERNET_STATUS_CONNECTING_TO_SERVER ? L"Connecting" : L"Connected");
			if (dwStatusInformationLength >= (sizeof(u_short)+8))
			{
				wchar_t sAddr[32] = L"";
				MultiByteToWideChar(CP_ACP, 0,
					((SOCKADDR*)lpvStatusInformation)->sa_data, min(dwStatusInformationLength-sizeof(u_short),countof(sAddr)-1),
					sAddr, countof(sAddr)-1);
				wcscat_c(sFormat, L" to Server, family=%u, data=%s");
				pObj->ReportMessage(dc_LogCallback, sFormat,
					at_Uint, (DWORD_PTR)hInternet,
					at_Uint, lpvStatusInformation ? ((SOCKADDR*)lpvStatusInformation)->sa_family : 0,
					at_Str, sAddr, at_None);
			}
			else
			{
				_ASSERTE(dwStatusInformationLength >= (sizeof(u_short)+8));
				wcscat_c(sFormat, L" to Server, datasize=%u");
				pObj->ReportMessage(dc_LogCallback, sFormat, at_Uint, (DWORD_PTR)hInternet, at_Uint, dwStatusInformationLength, at_None);
			}
            break;

        case INTERNET_STATUS_CONNECTION_CLOSED:
            LogCallback(L"Connection Closed", 0);
            break;

        case INTERNET_STATUS_HANDLE_CLOSING:
            LogCallback(L"Handle Closing x%08x", LODWORD(*((HINTERNET*)lpvStatusInformation)));
			SetEvent(pObj->mh_CloseEvent);
            break;

        case INTERNET_STATUS_HANDLE_CREATED:
            _ASSERTE(lpvStatusInformation);
            LogCallback(L"Handle x%08X created",
                    ((LPINTERNET_ASYNC_RESULT)lpvStatusInformation)->dwResult);

            break;

        case INTERNET_STATUS_INTERMEDIATE_RESPONSE:
            LogCallback(L"Intermediate response", 0);
            break;

        case INTERNET_STATUS_RECEIVING_RESPONSE:
            LogCallback(L"Receiving Response", 0);
            break;

        case INTERNET_STATUS_RESPONSE_RECEIVED:
            _ASSERTE(lpvStatusInformation);
            _ASSERTE(dwStatusInformationLength == sizeof(DWORD));

            LogCallback(L"Response Received (%u bytes)", *((LPDWORD)lpvStatusInformation));

            break;

        case INTERNET_STATUS_REDIRECT:
			wcscat_c(sFormat, L"Redirect to '%s'");
			pObj->ReportMessage(dc_LogCallback, sFormat, at_Uint, (DWORD_PTR)hInternet, at_Str, lpvStatusInformation?(LPCWSTR)lpvStatusInformation:L"", at_None);
            break;

        case INTERNET_STATUS_REQUEST_COMPLETE:
			wcscat_c(sFormat, L"Request complete, Result=x%08X, Value=%u");
			_ASSERTE(lpvStatusInformation);
			pObj->ReportMessage(dc_LogCallback, sFormat, at_Uint, (DWORD_PTR)hInternet,
				at_Uint, ((INTERNET_ASYNC_RESULT*)lpvStatusInformation)->dwResult,
				at_Uint, ((INTERNET_ASYNC_RESULT*)lpvStatusInformation)->dwError, at_None);

            pObj->m_Result = *(INTERNET_ASYNC_RESULT*)lpvStatusInformation;
			SetEvent(pObj->mh_ReadyEvent);

            break;

        case INTERNET_STATUS_REQUEST_SENT:
            ASSERTE(lpvStatusInformation);
            ASSERTE(dwStatusInformationLength == sizeof(DWORD));

            LogCallback(L"Request sent (%u bytes)", *((LPDWORD)lpvStatusInformation));
            break;

        case INTERNET_STATUS_DETECTING_PROXY:
            LogCallback(L"Detecting Proxy", 0);
            break;

        case INTERNET_STATUS_RESOLVING_NAME:
            LogCallback(L"Resolving Name", 0);
            break;

        case INTERNET_STATUS_NAME_RESOLVED:
            LogCallback(L"Name Resolved", 0);
            break;

        case INTERNET_STATUS_SENDING_REQUEST:
            LogCallback(L"Sending request", 0);
            break;

        case INTERNET_STATUS_STATE_CHANGE:
            LogCallback(L"State Change", 0);
            break;

        case INTERNET_STATUS_P3P_HEADER:
            LogCallback(L"Received P3P header", 0);
            break;

        default:
            LogCallback(L"Unknown callback status (%u)", dwInternetStatus);
            break;
    }

    return;
}

bool CDownloader::SetupTimeouts()
{
	bool bRc = false;
	DWORD cbSize, nTimeoutSet;
	DEBUGTEST(DWORD nTestTimeout);
	struct {
		LPDWORD pDefault;
		DWORD   dwOption;
		LPCWSTR pszName;
	} TimeOuts[] = {
		{&mn_RecvTimeout, INTERNET_OPTION_RECEIVE_TIMEOUT, L"receive"},
		{&mn_DataTimeout, INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, L"data receive"},
	};

	for (size_t i = 0; i < countof(TimeOuts); i++)
	{
		if (!*TimeOuts[i].pDefault)
		{
			cbSize = sizeof(*TimeOuts[i].pDefault);
			if (!wi->_InternetQueryOptionW(mh_Internet, TimeOuts[i].dwOption,
					TimeOuts[i].pDefault, &cbSize))
			{
				ReportMessage(dc_LogCallback, L"Warning: Query internet %s timeout failed, code=%u",
					at_Str, TimeOuts[i].pszName, at_Uint, GetLastError(), at_None);
				*TimeOuts[i].pDefault = 0;
			}
			else
			{
				ReportMessage(dc_LogCallback, L"Current internet %s timeout: %u ms",
					at_Str, TimeOuts[i].pszName, at_Uint, *TimeOuts[i].pDefault, at_None);
			}
		}

		nTimeoutSet = max(*TimeOuts[i].pDefault,mn_Timeout);
		ReportMessage(dc_LogCallback, L"Set internet %s timeout: %u ms",
			at_Str, TimeOuts[i].pszName, at_Uint, nTimeoutSet, at_None);
		if (!wi->_InternetSetOptionW(mh_Internet, TimeOuts[i].dwOption, &nTimeoutSet, sizeof(nTimeoutSet)))
		{
			ReportMessage(dc_ErrCallback,
				L"Set %s timeout(mh_Internet,%u) failed, code=%u",
				at_Str, TimeOuts[i].pszName, at_Uint, nTimeoutSet, at_Uint, GetLastError(), at_None);
			goto wrap;
		}

		#ifdef _DEBUG
		cbSize = sizeof(nTestTimeout); nTestTimeout = 0;
		wi->_InternetQueryOptionW(mh_Internet, TimeOuts[i].dwOption,
				&nTestTimeout, &cbSize);
		_ASSERTE(nTestTimeout == nTimeoutSet);
		#endif
	}

	bRc = true;
wrap:
	return bRc;
}

BOOL CDownloader::DownloadFile(LPCWSTR asSource, LPCWSTR asTarget, DWORD& crc, DWORD& size, BOOL abShowAllErrors /*= FALSE*/)
{
	BOOL lbRc = FALSE, lbRead = FALSE, lbWrite = FALSE;
	DWORD nRead;
	bool lbNeedTargetClose = false;
	HANDLE hDstFile = NULL;
	bool lbStdOutWrite = false;
	mb_InetMode = !IsLocalFile(asSource);
	bool lbTargetLocal = IsLocalFile(asTarget);
	_ASSERTE(lbTargetLocal);
	UNREFERENCED_PARAMETER(lbTargetLocal);
	DWORD cchDataMax = 64*1024;
	BYTE* ptrData = (BYTE*)malloc(cchDataMax);
	#if 0
	BOOL lbFirstThunk = TRUE;
	#endif
	DWORD ProxyType = INTERNET_OPEN_TYPE_DIRECT;
	LPCWSTR ProxyName = NULL;
	wchar_t szServer[MAX_PATH];
	wchar_t* pszSrvPath = NULL;
	wchar_t *pszColon;
	INTERNET_PORT nServerPort = INTERNET_DEFAULT_HTTP_PORT;
	bool bSecureHTTPS = false;
	bool bFtp = false;
	HTTP_VERSION_INFO httpver = {1,1};
	wchar_t szHttpVer[32]; _wsprintf(szHttpVer, SKIPLEN(countof(szHttpVer)) L"HTTP/%u.%u", httpver.dwMajorVersion, httpver.dwMinorVersion);
	const wchar_t szConEmuAgent[] =
		//L"Mozilla/5.0 (compatible; ConEmu Update)" // This was the cause of not working download redirects
		L"ConEmu Update" // so we use that to enable redirects
		;
	LPCWSTR pszAgent = msz_AgentName ? msz_AgentName : szConEmuAgent;
	LPCWSTR szAcceptTypes[] = {L"*/*", NULL};
	LPCWSTR* ppszAcceptTypes = szAcceptTypes;
	LPCWSTR pszReferrer = NULL;
	DWORD nFlags, nService;
	BOOL bFRc;
	DWORD dwErr;
	MSectionLockSimple CS;

	if (!wi)
	{
		if (!InitInterface())
		{
			goto wrap;
		}
	}
	else
	{
		CloseInternet(false);
	}

	ReportMessage(dc_LogCallback, L"File download requested '%s'", at_Str, asSource, at_None);

	_ASSERTE(mn_CloseRef==0);
	ResetEvent(mh_CloseEvent);
	ResetEvent(mh_ReadyEvent);

	mn_InternetContentReady = 0;
	mn_InternetContentLen = 0;
	mb_FtpMode = false;

	crc = 0xFFFFFFFF;

	if (mb_InetMode)
	{
		LPCWSTR pszSource;
		if (memcmp(asSource, L"http://", 7*sizeof(*asSource)) == 0)
		{
			bSecureHTTPS = false;
			pszSource = asSource + 7;
		}
		else if (memcmp(asSource, L"https://", 8*sizeof(*asSource)) == 0)
		{
			bSecureHTTPS = true;
			pszSource = asSource + 8;
			nServerPort = INTERNET_DEFAULT_HTTPS_PORT;
		}
		else if (memcmp(asSource, L"ftp://", 6*sizeof(*asSource)) == 0)
		{
			bFtp = true;
			pszSource = asSource + 6;
			nServerPort = INTERNET_DEFAULT_FTP_PORT;
		}
		else
		{
			_ASSERTE(FALSE && "Only http addresses are supported now!");
			ReportMessage(dc_ErrCallback,
				L"Only http addresses are supported!\n\t%s", at_Str, asSource, at_None);
			goto wrap;
		}
		LPCWSTR pszSlash = wcschr(pszSource, L'/');
		if (!pszSlash || (pszSlash == pszSource) || ((pszSlash - pszSource) >= (INT_PTR)countof(szServer)))
		{
			ReportMessage(dc_ErrCallback,
				L"Invalid server (domain) specified!\n\t%s", at_Str, asSource, at_None);
			goto wrap;
		}
		if (!*(pszSlash+1))
		{
			ReportMessage(dc_ErrCallback,
				L"Invalid server path specified!\n%s", at_Str, asSource, at_None);
			goto wrap;
		}
		lstrcpyn(szServer, pszSource, (pszSlash - pszSource + 1));
		pszSrvPath = lstrdup(pszSlash);
	}

	if (!asSource || !*asSource || !asTarget || !*asTarget)
	{
		ReportMessage(dc_ErrCallback,
			L"DownloadFile. Invalid arguments", at_None);
		goto wrap;
	}

	if (!ptrData)
	{
		ReportMessage(dc_ErrCallback,
			L"Failed to allocate memory (%u bytes)", at_Uint, cchDataMax, at_None);
		goto wrap;
	}

	// Acquire the destination handle
	{
		lbStdOutWrite = (lstrcmp(asTarget, L"-") == 0);

		if (lbStdOutWrite)
			hDstFile = GetStdHandle(STD_OUTPUT_HANDLE);
		else
			hDstFile = CreateFile(asTarget, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);

		if (!hDstFile || hDstFile == INVALID_HANDLE_VALUE)
		{
			ReportMessage(dc_ErrCallback,
				L"Failed to create target file(%s), code=%u", at_Str, asTarget, at_Uint, GetLastError(), at_None);
			goto wrap;
		}

		lbNeedTargetClose = !lbStdOutWrite;
	}

	if (mb_InetMode)
	{
		if (m_Proxy.szProxy)
		{
			if (m_Proxy.szProxy && *m_Proxy.szProxy)
			{
				ProxyType = INTERNET_OPEN_TYPE_PROXY;
				ProxyName = m_Proxy.szProxy;
			}
			else
			{
				ProxyType = INTERNET_OPEN_TYPE_PRECONFIG;
			}
		}

		if (mb_RequestTerminate)
			goto wrap;

		// Открыть WinInet
		if (mh_Internet == NULL)
		{
			ReportMessage(dc_LogCallback, L"Open internet with agent name '%s'", at_Str, pszAgent, at_None);
			nFlags = (mb_AsyncMode ? INTERNET_FLAG_ASYNC : 0);
			mh_Internet = wi->_InternetOpenW(pszAgent, ProxyType, ProxyName, NULL, nFlags);
			if (!mh_Internet)
			{
				dwErr = GetLastError();
				ReportMessage(dc_ErrCallback,
					L"Network initialization failed, code=%u", at_Uint, dwErr, at_None);
				goto wrap;
			}
			ReportMessage(dc_LogCallback, L"Internet opened x%08X", at_Uint, (DWORD_PTR)mh_Internet, at_None);

			if (mb_AsyncMode)
			{
				mp_SetCallbackRc = wi->_InternetSetStatusCallbackW(mh_Internet, InetCallback);
				if (mp_SetCallbackRc == INTERNET_INVALID_STATUS_CALLBACK)
				{
					ReportMessage(dc_ErrCallback, L"Failed to set internet status callback, code=%u", at_Uint, GetLastError(), at_None);
					_ASSERTE(mp_SetCallbackRc != INTERNET_INVALID_STATUS_CALLBACK);
					mb_AsyncMode = false;
				}
			}

			if (mb_RequestTerminate)
				goto wrap;

			// Proxy User/Password
			if (ProxyName)
			{
				// Похоже, что установка логина/пароля для mh_Internet смысла не имеет
				if (!SetProxyForHandle(mh_Internet))
				{
					goto wrap;
				}
			}

			// Protocol version
			ReportMessage(dc_LogCallback, L"Set protocol version (%u.%u)", at_Uint, httpver.dwMajorVersion, at_Uint, httpver.dwMinorVersion, at_None);
			if (!wi->_InternetSetOptionW(mh_Internet, INTERNET_OPTION_HTTP_VERSION, &httpver, sizeof(httpver)))
			{
				ReportMessage(dc_ErrCallback,
					L"HttpVersion failed, code=%u", at_Uint, GetLastError(), at_None);
				goto wrap;
			}

			// Force Online
			ReportMessage(dc_LogCallback, L"Set IGNORE_OFFLINE option", at_None);
			nFlags = TRUE; _ASSERTE(sizeof(nFlags)==4);
			if (!wi->_InternetSetOptionW(mh_Internet, INTERNET_OPTION_IGNORE_OFFLINE, &nFlags, sizeof(nFlags)))
			{
				ReportMessage(dc_ErrCallback,
					L"Set IGNORE_OFFLINE option, code=%u", at_Uint, GetLastError(), at_None);
				//goto wrap;
			}
		}

		// Timeout
		if (!SetupTimeouts())
		{
			goto wrap;
		}


		//
		_ASSERTE(mh_Connect == NULL);

		// Try to force reload
		//if (wi->_DeleteUrlCacheEntryW)
		//	wi->_DeleteUrlCacheEntryW(asSource);

		//TODO после включения ноута вылезла ошибка ERROR_INTERNET_NAME_NOT_RESOLVED==12007

		// Server:Port
		if ((pszColon = wcsrchr(szServer, L':')) != NULL)
		{
			*pszColon = 0;
			INTERNET_PORT nExplicit = (INTERNET_PORT)LOWORD(wcstoul(pszColon+1, &pszColon, 10));
			if (nExplicit)
				nServerPort = nExplicit;
			_ASSERTE(nServerPort!=NULL);
		}
		nFlags = 0; // No special flags
		nService = bFtp?INTERNET_SERVICE_FTP:INTERNET_SERVICE_HTTP;
		ReportMessage(dc_LogCallback, L"Connecting to server %s:%u (%u)", at_Str, szServer, at_Uint, nServerPort, at_Uint, nService, at_None);
		CS.Lock(&mcs_Handle); // Leaved in ExecRequest
		mh_Connect = ExecRequest(wi->_InternetConnectW(mh_Internet, szServer, nServerPort, m_Server.szUser, m_Server.szPassword, nService, nFlags, (DWORD_PTR)this), dwErr, CS);
		if (!mh_Connect)
		{
			if (abShowAllErrors)
			{
				ReportMessage(dc_ErrCallback,
					(dwErr == 12015) ? L"Authorization failed, code=%u" : L"Connection failed, code=%u",
					at_Uint, dwErr, at_None);
			}
			goto wrap;
		}
		ReportMessage(dc_LogCallback, L"Connect opened x%08X", at_Uint, (DWORD_PTR)mh_Connect, at_None);
		//WaitAsyncResult();

		if (ProxyName)
		{
			// Похоже, что установка логина/пароля для mh_Internet смысла не имеет
			// Поэтому повторяем здесь для хэндла mh_Connect
			if (!SetProxyForHandle(mh_Connect))
			{
				goto wrap;
			}
		}

		// Повторим для mh_Connect, на всякий случай
		if (!wi->_InternetSetOptionW(mh_Connect, INTERNET_OPTION_HTTP_VERSION, &httpver, sizeof(httpver)))
		{
			ReportMessage(dc_ErrCallback,
				L"HttpVersion failed, code=%u", at_Uint, GetLastError(), at_None);
			goto wrap;
		}


		if (mb_RequestTerminate)
			goto wrap;

		_ASSERTE(mh_SrcFile==NULL);
		// Send request for the file
		if (bFtp)
		{
			mb_FtpMode = true;
			wchar_t* pszSlash = wcsrchr(pszSrvPath, L'/');
			// Break path to dir+file
			if (pszSlash == pszSrvPath)
				pszSlash = NULL; // Root
			else if (pszSlash)
				*pszSlash = 0; // It is our memory buffer, we can do anything with it
			// Set ftp directory
			LPCWSTR pszSetDir = pszSlash ? pszSrvPath : L"/";
			LPCWSTR pszFile = pszSlash ? (pszSlash+1) : (*pszSrvPath == L'/') ? (pszSrvPath+1) : pszSrvPath;

			CS.Lock(&mcs_Handle); // Leaved in ExecRequest
			if (!ExecRequest(wi->_FtpSetCurrentDirectoryW(mh_Connect, pszSetDir), dwErr, CS))
			{
				ReportMessage(dc_ErrCallback, L"Ftp set directory failed '%s', code=%u", at_Str, pszSetDir, at_Uint, GetLastError(), at_None);
				if (pszSlash) *pszSlash = L'/'; // return it back
				goto wrap;
			}
			//WaitAsyncResult();

			if (pszSlash) *pszSlash = L'/'; // return it back
			nFlags = FTP_TRANSFER_TYPE_BINARY;

			CS.Lock(&mcs_Handle); // Leaved in ExecRequest
			mh_SrcFile = ExecRequest(wi->_FtpOpenFileW(mh_Connect, pszFile, GENERIC_READ, nFlags, (DWORD_PTR)this), dwErr, CS);
			ReportMessage(dc_LogCallback, L"Ftp file opened x%08X", at_Uint, (DWORD_PTR)mh_SrcFile, at_None);
			//WaitAsyncResult();

			if (pszSlash) *pszSlash = L'/'; // return it back
			if (!mh_SrcFile || (mh_SrcFile == INVALID_HANDLE_VALUE))
			{
				dwErr = GetLastError();
				if (abShowAllErrors)
				{
					// In offline mode, FtpOpenFile returns ERROR_FILE_NOT_FOUND if the resource is not found in the Internet cache.
					ReportMessage(dc_ErrCallback,
						(dwErr == 2)
						? L"Ftp open file failed\nURL=%s\ncode=%u, Internet is offline?"
						: L"Ftp open file failed\nURL=%s\ncode=%u"
						, at_Str, asSource, at_Uint, dwErr, at_None);
				}
				goto wrap;
			}
			// Set length to "Unknown" (simple)
			mn_InternetContentLen = 0;
		}
		else
		{
			nFlags = 0 //INTERNET_FLAG_KEEP_CONNECTION
				|(bSecureHTTPS?(INTERNET_FLAG_SECURE|INTERNET_FLAG_IGNORE_CERT_CN_INVALID|INTERNET_FLAG_IGNORE_CERT_DATE_INVALID):0)
				//|INTERNET_FLAG_HYPERLINK
				//|INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP
				|INTERNET_FLAG_DONT_CACHE
				|INTERNET_FLAG_NO_CACHE_WRITE
				|INTERNET_FLAG_PRAGMA_NOCACHE
				//|INTERNET_FLAG_PRAGMA_NOCACHE
				|INTERNET_FLAG_RELOAD;
			ReportMessage(dc_LogCallback, L"Opening request with flags x%08X", at_Uint, nFlags, at_None);
			mh_SrcFile = wi->_HttpOpenRequestW(mh_Connect, L"GET", pszSrvPath, szHttpVer, pszReferrer,
				ppszAcceptTypes, nFlags, (DWORD_PTR)this);

			if (!mh_SrcFile || (mh_SrcFile == INVALID_HANDLE_VALUE))
			{
				dwErr = GetLastError();
				if (abShowAllErrors)
				{
					// In offline mode, HttpSendRequest returns ERROR_FILE_NOT_FOUND if the resource is not found in the Internet cache.
					ReportMessage(dc_ErrCallback,
						(dwErr == 2)
						? L"HttpOpenRequest failed\nURL=%s\ncode=%u, Internet is offline?"
						: L"HttpOpenRequest failed\nURL=%s\ncode=%u"
						, at_Str, asSource, at_Uint, dwErr, at_None);
				}
				goto wrap;
			}
			ReportMessage(dc_LogCallback, L"Http file opened x%08X", at_Uint, (DWORD_PTR)mh_SrcFile, at_None);
			//WaitAsyncResult();

			ReportMessage(dc_LogCallback, L"Sending request");
			ResetEvent(mh_ReadyEvent);
			CS.Lock(&mcs_Handle); // Leaved in ExecRequest
			bFRc = ExecRequest(wi->_HttpSendRequestW(mh_SrcFile,NULL,0,NULL,0), dwErr, CS);

			if (!bFRc)
			{
				dwErr = GetLastError();
				if (abShowAllErrors)
				{
					ReportMessage(dc_ErrCallback,
						L"HttpSendRequest failed, code=%u\n\tURL=%s", at_Uint, dwErr, at_Str, asSource, at_None);
				}
				goto wrap;
			}
			else
			{
				//WaitAsyncResult();

				mn_InternetContentLen = 0;
				DWORD sz = sizeof(mn_InternetContentLen);
				DWORD dwIndex = 0;
				nFlags = HTTP_QUERY_CONTENT_LENGTH|HTTP_QUERY_FLAG_NUMBER;
				ReportMessage(dc_LogCallback, L"Quering file info with flags x%08X", at_Uint, nFlags, at_None);
				CS.Lock(&mcs_Handle); // Leaved in ExecRequest
				if (!ExecRequest(wi->_HttpQueryInfoW(mh_SrcFile, nFlags, &mn_InternetContentLen, &sz, &dwIndex), dwErr, CS))
				{
					mn_InternetContentLen = 0;
					UNREFERENCED_PARAMETER(dwErr);
					//DWORD dwErr = GetLastError();
					//// были ошибки: ERROR_HTTP_HEADER_NOT_FOUND
					//if (abShowAllErrors)
					//	ReportError(L"QueryContentLen failed\nURL=%s\ncode=%u", asSource, dwErr);
					//goto wrap;
					ReportMessage(dc_LogCallback, L"Warning: Quering file info failed, code=%u", at_Uint, dwErr, at_None);
				}
				else
				{
					ReportMessage(dc_LogCallback, L"File length retrieved: %u bytes", at_Uint, mn_InternetContentLen, at_None);
				}

				//WaitAsyncResult();
			}
		}
	}
	else
	{
		ReportMessage(dc_LogCallback, L"Opening source from file system");
		mh_SrcFile = CreateFile(asSource, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (!mh_SrcFile || (mh_SrcFile == INVALID_HANDLE_VALUE))
		{
			ReportMessage(dc_ErrCallback,
				L"Failed to open source file(%s), code=%u", at_Str, asSource, at_Uint, GetLastError(), at_None);
			goto wrap;
		}
		LARGE_INTEGER liSize;
		if (GetFileSizeEx(mh_SrcFile, &liSize))
		{
			mn_InternetContentLen = liSize.LowPart;
			ReportMessage(dc_LogCallback, L"File length: %u bytes", at_Uint, mn_InternetContentLen, at_None);
		}
	}

	//WaitAsyncResult();

	while (TRUE)
	{
		if (mb_RequestTerminate)
			goto wrap;

		lbRead = ReadSource(asSource, mb_InetMode, mh_SrcFile, ptrData, cchDataMax, &nRead);
		if (!lbRead)
			goto wrap;

		//WaitAsyncResult();

		if (!nRead)
		{
			if (!mn_InternetContentReady)
			{
				ReportMessage(dc_ErrCallback,
					L"Invalid source url '%s', file is empty or not found", at_Str, asSource, at_None);
				goto wrap;
			}
			break;
		}

		CalcCRC(ptrData, nRead, crc);

		lbWrite = WriteTarget(asTarget, hDstFile, ptrData, nRead);
		if (!lbWrite)
			goto wrap;

		#if defined(_DEBUG) && defined(SLOW_CONNECTION_SIMULATE)
		Sleep(1000);
		#endif

		#if 0
		if (lbFirstThunk)
		{
			lbFirstThunk = FALSE;
			LPCSTR psz = (LPCSTR)ptrData;
			while (*psz == L' ' || *psz == L'\r' || *psz == L'\n' || *psz == L'\t')
				psz++;
			// Определить ошибку 404?
			if (*psz == L'<')
			{
				if (abShowAllErrors)
				{
					ReportMessage(dc_ErrCallback,
						L"Remote file not found\nURL: %s\nLocal: %s", at_Str, asSource, at_Str, asTarget, at_None);
				}
				goto wrap;
			}
		}
		#endif

		mn_InternetContentReady += nRead;

		UpdateProgress();
	}

	// Succeeded
	crc ^= 0xFFFFFFFF;
	lbRc = TRUE;
wrap:
	size = mn_InternetContentReady;

	if (lbRc)
		ReportMessage(dc_LogCallback, L"Download finished, %u bytes retrieved", at_Uint, size, at_None);
	else
		ReportMessage(dc_LogCallback, L"Download failed");

	if (mb_InetMode)
	{
		CloseInternet(lbRc == FALSE);
	}
	else
	{
		if (mh_SrcFile && (mh_SrcFile != INVALID_HANDLE_VALUE))
			CloseHandle(mh_SrcFile);
		mh_SrcFile = NULL;
	}

	// Close only real-file handle, skip obtained from GetStdHandle()
	if (lbNeedTargetClose && hDstFile && hDstFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hDstFile);
	}

	SafeFree(ptrData);
	SafeFree(pszSrvPath);
	return lbRc;
}

void CDownloader::CloseInternet(bool bFull)
{
	BOOL bClose = FALSE;

	if (wi)
	{
		if (mh_SrcFile)
		{
			bClose = InetCloseHandle(mh_SrcFile);
		}

		if (mh_Connect)
		{
			bClose = InetCloseHandle(mh_Connect);
		}

		if (bFull && mh_Internet)
		{
			bClose = InetCloseHandle(mh_Internet, true);
		}
	}

	if (bFull)
		mp_SetCallbackRc = NULL;

	mh_SrcFile = NULL;
	mh_Connect = NULL;

	if (bFull)
	{
		mh_Internet = NULL;
	}

	UNREFERENCED_PARAMETER(bClose);
}

void CDownloader::RequestTerminate()
{
	ReportMessage(dc_LogCallback, L"!!! Download termination was requested !!!");
	mb_RequestTerminate = true;
	CloseInternet(false);
}

void CDownloader::SetAsync(bool bAsync)
{
	ReportMessage(dc_LogCallback, L"Change mode to %s was requested", at_Str, bAsync?L"Async":L"Sync", at_None);
	mb_AsyncMode = bAsync;
}

void CDownloader::SetTimeout(UINT nWhat, DWORD nTimeout)
{
	LPCWSTR pszName = L"unknown";
	switch (nWhat)
	{
	case 0:
		// Default is DOWNLOADTIMEOUT (30000 ms)
		pszName = L"operation";
		mn_Timeout = nTimeout;
		break;
	case 1:
		pszName = L"receive";
		mn_RecvTimeout = nTimeout; // INTERNET_OPTION_RECEIVE_TIMEOUT
	    break;
	case 2:
		pszName = L"data receive";
		mn_DataTimeout = nTimeout; // INTERNET_OPTION_DATA_RECEIVE_TIMEOUT
		break;
	}
	ReportMessage(dc_LogCallback, L"Set %s timeout to %u was requested", at_Str, pszName, at_Uint, nTimeout, at_None);
}

void CDownloader::SetAgent(LPCWSTR aszAgentName)
{
	SafeFree(msz_AgentName);
	msz_AgentName = (aszAgentName && *aszAgentName) ? lstrdup(aszAgentName) : NULL;
}

BOOL CDownloader::ReadSource(LPCWSTR asSource, BOOL bInet, HANDLE hSource, BYTE* pData, DWORD cbData, DWORD* pcbRead)
{
	BOOL lbRc = FALSE;
	DWORD dwErr = 0;

	if (bInet)
	{
		ReportMessage(dc_LogCallback, L"Reading source");
		*pcbRead = 0;
		MSectionLockSimple CS;
		CS.Lock(&mcs_Handle); // Leaved in ExecRequest
		lbRc = ExecRequest(wi->_InternetReadFile(hSource, pData, cbData, pcbRead), dwErr, CS);

		if (mb_AsyncMode && lbRc && !*pcbRead && dwErr)
		{
			_ASSERTE(mb_FtpMode && dwErr <= cbData);
			if (dwErr <= cbData)
				*pcbRead = dwErr;
		}

		if (!lbRc)
			ReportMessage(dc_ErrCallback,
				L"DownloadFile(%s) failed, code=%u", at_Str, asSource, at_Uint, dwErr, at_None);
		else if (*pcbRead)
			ReportMessage(dc_LogCallback, L"Retrieved %u bytes in block", at_Uint, *pcbRead, at_None);
		else
			ReportMessage(dc_LogCallback, L"No more data? code=%u", at_Uint, dwErr, at_None);
	}
	else
	{
		ReportMessage(dc_LogCallback, L"Reading file");
		lbRc = ReadFile(hSource, pData, cbData, pcbRead, NULL);
		if (!lbRc)
			ReportMessage(dc_ErrCallback,
				L"ReadFile(%s) failed, code=%u", at_Str, asSource, at_Uint, GetLastError(), at_None);
		else
			ReportMessage(dc_LogCallback, L"Read %u bytes", at_Uint, *pcbRead, at_None);
	}

	return lbRc;
}

BOOL CDownloader::WriteTarget(LPCWSTR asTarget, HANDLE hTarget, const BYTE* pData, DWORD cbData)
{
	BOOL lbRc;
	DWORD nWritten;

	ReportMessage(dc_LogCallback, L"Writing target file %u bytes", at_Uint, cbData, at_None);
	lbRc = WriteFile(hTarget, pData, cbData, &nWritten, NULL);

	if (lbRc && (nWritten != cbData))
	{
		lbRc = FALSE;
		ReportMessage(dc_ErrCallback,
			L"WriteFile(%s) failed, no data, code=%u", at_Str, asTarget, at_Uint, GetLastError(), at_None);
	}
	else if (!lbRc)
	{
		ReportMessage(dc_ErrCallback,
			L"WriteFile(%s) failed, code=%u", at_Str, asTarget, at_Uint, GetLastError(), at_None);
	}

	return lbRc;
}

// Logging, errors, download progress
void CDownloader::SetCallback(CEDownloadCommand cb, FDownloadCallback afnErrCallback, LPARAM lParam)
{
	if (cb > dc_LogCallback)
	{
		_ASSERTE(cb <= dc_LogCallback && cb >= dc_ErrCallback);
		return;
	}
	mfn_Callback[cb] = afnErrCallback;
	m_CallbackLParam[cb] = lParam;
}

void CDownloader::ReportMessage(CEDownloadCommand rm, LPCWSTR asFormat, CEDownloadArgType nextArgType /*= at_None*/, ...)
{
	_ASSERTE(asFormat && *asFormat && asFormat[lstrlen(asFormat)-1]!=L'\n');
	if (!mfn_Callback[rm])
		return;

	CEDownloadArgType argType = nextArgType;

	va_list argptr;
	va_start(argptr, nextArgType);

	CEDownloadInfo args = {sizeof(args), m_CallbackLParam[rm], asFormat, 1};

	size_t i = 0;
	while (argType != at_None)
	{
		if (argType == at_Uint)
			args.Args[i].uintArg = va_arg( argptr, DWORD_PTR );
		else if (argType == at_Str)
			args.Args[i].strArg = va_arg( argptr, wchar_t* );
		else
		{
			_ASSERTE(argType==at_Uint || argType==at_Str);
			break;
		}

		args.Args[i++].argType = argType;
		if (i >= countof(args.Args))
			break;

		argType = (CEDownloadArgType)va_arg( argptr, int );
	}

	va_end(argptr);

	args.argCount = i;

	mfn_Callback[rm](&args);
}

void CDownloader::UpdateProgress()
{
	ReportMessage(dc_ProgressCallback, L"Bytes downloaded %u", at_Uint, mn_InternetContentReady, at_None);
}

static CDownloader* gpInet = NULL;

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
	case dc_SetLogin: // [0]="User", [1]="Password"
		if (gpInet)
		{
			gpInet->SetLogin(
				(argc > 0 && argv[0].argType == at_Str) ? argv[0].strArg : NULL,
				(argc > 1 && argv[1].argType == at_Str) ? argv[1].strArg : NULL);
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
	case dc_DownloadFile: // [0]="http", [1]="DestLocalFilePath", [2]=abShowAllErrors
		if (gpInet && (argc >= 2))
		{
			DWORD crc = 0, size = 0;
			nResult = gpInet->DownloadFile(
				(argc > 0 && argv[0].argType == at_Str) ? argv[0].strArg : NULL,
				(argc > 1 && argv[1].argType == at_Str) ? argv[1].strArg : NULL,
				crc, size,
				(argc > 2) ? argv[2].uintArg : TRUE);
			// Succeeded?
			if (nResult)
			{
				argv[0].uintArg = size;
				argv[1].uintArg = crc;
			}
		}
		break;
	case dc_DownloadData: // [0]="http" -- not implemented yet
		_ASSERTE(FALSE && "dc_DownloadData not implemented yet");
		break;
	case dc_RequestTerminate:
		if (gpInet)
		{
			gpInet->RequestTerminate();
			nResult = TRUE;
		}
		break;
	case dc_SetAsync:
		if (gpInet && (argc > 0) && (argv[0].argType == at_Uint))
		{
			gpInet->SetAsync(argv[0].uintArg != 0);
			nResult = TRUE;
		}
		break;
	case dc_SetTimeout:
		if (gpInet && (argc > 1) && (argv[0].argType == at_Uint) && (argv[1].argType == at_Uint))
		{
			gpInet->SetTimeout(argv[0].uintArg, argv[1].uintArg);
			nResult = TRUE;
		}
		break;
	case dc_SetAgent:
		if (gpInet && (argc > 0) && (argv[0].argType == at_Str))
		{
			gpInet->SetAgent(argv[0].strArg);
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

bool PrintToConsole(HANDLE hCon, LPCWSTR asData, int anLen)
{
	bool bSuccess = false;
	bool bNeedClose = false;
	DWORD nWritten;

	if (!hCon)
	{
		hCon = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
			0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		bNeedClose = (hCon && (hCon != INVALID_HANDLE_VALUE));
	}

	if (WriteConsoleW(hCon, asData, anLen, &nWritten, NULL))
	{
		bSuccess = true;
	}

	if (bNeedClose)
	{
		CloseHandle(hCon);
	}

	return bSuccess;
}

static void PrintDownloadLog(LPCWSTR pszLabel, LPCWSTR pszInfo)
{
	SYSTEMTIME st = {}; GetLocalTime(&st);
	wchar_t szTime[80];
	_wsprintf(szTime, SKIPLEN(countof(szTime)) L"%i:%02i:%02i.%03i{%u} ",
	           st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, GetCurrentThreadId());

	CEStr lsAll(lstrmerge(szTime, pszLabel, (pszInfo && *pszInfo) ? pszInfo : L"<NULL>\n"));

	DWORD nWritten;
	int iLen = lstrlen(lsAll.ms_Val);
	if (iLen <= 0)
		return;

	#if defined(_DEBUG)
	OutputDebugString(lsAll.ms_Val);
	#endif

	// Log downloader events to StdError
	// We may be asked to write downloaded contents to StdOut

	static HANDLE hStdErr = NULL;
	static bool bRedirected = false;
	if (!hStdErr)
	{
		hStdErr = GetStdHandle(STD_ERROR_HANDLE);
		if (!hStdErr)
		{
			hStdErr = INVALID_HANDLE_VALUE;
		}
		else
		{
			CONSOLE_SCREEN_BUFFER_INFO sbi = {};
			BOOL bIsConsole = GetConsoleScreenBufferInfo(hStdErr, &sbi);
			if (!bIsConsole)
			{
				bRedirected = true;
			}
		}
	}
	else if (hStdErr == INVALID_HANDLE_VALUE)
	{
		if (gbVerbose)
		{
			PrintToConsole(NULL, lsAll.ms_Val, iLen);
		}
		return;
	}

	if (!bRedirected)
	{
		PrintToConsole(hStdErr, lsAll.ms_Val, iLen);
	}
	else
	{
		if (gbVerbose)
		{
			PrintToConsole(NULL, lsAll.ms_Val, iLen);
		}

		DWORD cp = GetConsoleCP();
		DWORD nMax = WideCharToMultiByte(cp, 0, lsAll.ms_Val, iLen, NULL, 0, NULL, NULL);

		char szOem[200], *pszOem = NULL;

		if (nMax >= countof(szOem))
		{
			pszOem = (char*)malloc(nMax+1);
		}
		else
		{
			pszOem = szOem;
		}

		if (pszOem)
		{
			pszOem[nMax] = 0; // just for debugging purposes, not requried

			if (WideCharToMultiByte(cp, 0, lsAll.ms_Val, iLen, pszOem, nMax+1, NULL, NULL) > 0)
			{
				WriteFile(hStdErr, pszOem, nMax, &nWritten, NULL);
			}

			if (pszOem != szOem)
				free(pszOem);
		}
	}
}

static void WINAPI DownloadCallback(const CEDownloadInfo* pInfo)
{
	LPCWSTR pszLabel = pInfo->lParam==(dc_ErrCallback+1) ? CEDLOG_MARK_ERROR
		: pInfo->lParam==(dc_ProgressCallback+1) ? CEDLOG_MARK_PROGR
		: pInfo->lParam==(dc_LogCallback+1) ? CEDLOG_MARK_INFO
		: L"";
	CEStr szInfo(pInfo->GetFormatted(true));
	PrintDownloadLog(pszLabel, szInfo);
}

FDownloadCallback gpfn_DownloadCallback = DownloadCallback;

void InitVerbose()
{
	if (!gbVerboseInitialized)
	{
		#if defined(WAIT_FOR_DEBUGGER_MSG)
		MessageBox(NULL, L"Waiting for debugger", L"ConEmu downloader", MB_SYSTEMMODAL);
		#endif

		gbVerboseInitialized = true;
		HWND hConWnd = GetConsoleWindow();
		if (hConWnd)
		{
			// If STARTUPINFO has SW_HIDE flag - first ShowWindow may be ignored
			for (int i = 0; i <= 1; i++)
			{
				if (!::IsWindowVisible(hConWnd))
					::ShowWindowAsync(hConWnd, SW_SHOWNORMAL);
			}
		}
	}
}

static void DownloadLog(CEDownloadCommand logLevel, LPCWSTR asMessage)
{
	CEDownloadInfo Info = {sizeof(Info), logLevel+1, asMessage};

	if (gbVerbose)
	{
		if (!gbVerboseInitialized)
		{
			InitVerbose();
		}

		if (gpfn_DownloadCallback != DownloadCallback)
		{
			DownloadCallback(&Info);
		}
	}

	if (gpfn_DownloadCallback)
	{
		gpfn_DownloadCallback(&Info);
	}
}

int DoDownload(LPCWSTR asCmdLine)
{
	int iRc = CERR_CARGUMENT;
	DWORD_PTR drc;
	CEStr szArg;
	wchar_t* pszUrl = NULL;
	size_t iFiles = 0;
	CEDownloadErrorArg args[4];
	wchar_t* pszExpanded = NULL;
	wchar_t szFullPath[MAX_PATH*2];
	wchar_t szResult[80];
	DWORD nFullRc;
	wchar_t *pszProxy = NULL, *pszProxyLogin = NULL, *pszProxyPassword = NULL;
	wchar_t *pszLogin = NULL, *pszPassword = NULL;
	wchar_t *pszTimeout = NULL, *pszTimeout1 = NULL, *pszTimeout2 = NULL;
	wchar_t *pszAsync = NULL;
	wchar_t *pszAgent = NULL;

	gbVerbose = false;
	gbVerboseInitialized = false;

	DownloadCommand(dc_Init, 0, NULL);

	args[0].uintArg = (DWORD_PTR)gpfn_DownloadCallback; args[0].argType = at_Uint;
	args[1].argType = at_Uint;
	_ASSERTE(dc_ErrCallback==0 && dc_LogCallback==2);
	for (int i = dc_ErrCallback; i <= dc_LogCallback; i++)
	{
		args[1].uintArg = (i+1);
		DownloadCommand((CEDownloadCommand)i, 2, args);
	}

	struct {
		LPCWSTR   pszArgName;
		wchar_t** ppszValue;
		bool*     pbValue;
	} KnownArgs[] = {
		{L"login", &pszLogin},
		{L"password", &pszPassword},
		{L"proxy", &pszProxy},
		{L"proxylogin", &pszProxyLogin},
		{L"proxypassword", &pszProxyPassword},
		// -timeout <ms> - default min timeout for all operations (30000 ms)
		{L"timeout", &pszTimeout},
		// -timeout1 <ms> - INTERNET_OPTION_RECEIVE_TIMEOUT
		{L"timeout1", &pszTimeout1},
		// -timeout2 <ms> - INTERNET_OPTION_DATA_RECEIVE_TIMEOUT
		{L"timeout2", &pszTimeout2},
		// -async Y|N - change internal mode, how ConEmuC interact with WinInet
		{L"async", &pszAsync},
		// -agent "AgentName" - to change from default agent name "ConEmu Update"
		{L"agent", &pszAgent},
		// -debug - may be used to show console and print progress even if output is redirected
		{L"debug", NULL, &gbVerbose}, {L"verbose", NULL, &gbVerbose},
		{NULL}
	};

	while (NextArg(&asCmdLine, szArg) == 0)
	{
		LPCWSTR psz = szArg;
		if ((psz[0] == L'-') || (psz[0] == L'/'))
		{
			psz++;
			bool bKnown = false;
			for (size_t i = 0; KnownArgs[i].pszArgName; i++)
			{
				if (lstrcmpi(psz, KnownArgs[i].pszArgName) == 0)
				{
					bKnown = true;
					if (!KnownArgs[i].ppszValue)
					{
						if (KnownArgs[i].pbValue)
							*KnownArgs[i].pbValue = true;
						continue;
					}
					SafeFree(*KnownArgs[i].ppszValue);
					if (NextArg(&asCmdLine, szArg) == 0)
						*KnownArgs[i].ppszValue = szArg.Detach();
					break;
				}
			}
			if (!bKnown)
			{
				CEStr lsInfo(lstrmerge(L"Unknown argument '", psz, L"'"));
				DownloadLog(dc_ErrCallback, lsInfo);
				iRc = CERR_CARGUMENT;
				goto wrap;
			}
			continue;
		}

		if (gbVerbose)
		{
			InitVerbose();
		}

		SafeFree(pszUrl);
		pszUrl = szArg.Detach();
		if (NextArg(&asCmdLine, szArg) != 0)
		{
			// If NOT redirected to file already
			HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
			CONSOLE_SCREEN_BUFFER_INFO sbi = {};
			BOOL bIsConsole = GetConsoleScreenBufferInfo(hStdOut, &sbi);

			if (bIsConsole)
			{
				// If user omit file name - try to get it from pszUrl
				LPCWSTR pszFS = wcsrchr(pszUrl, L'/');
				LPCWSTR pszBS = wcsrchr(pszUrl, L'\\');
				LPCWSTR pszSlash = (pszFS && pszBS) ? ((pszBS > pszFS) ? pszBS : pszFS) : pszFS ? pszFS : pszBS;

				if (pszSlash)
				{
					pszSlash++;
					pszFS = wcschr(pszSlash, L'?'); // some add args after file name (mirrors etc.)
					if (!pszFS || (pszFS > pszSlash))
						szArg.Set(pszSlash, pszFS ? (pszFS - pszSlash) : -1);
					// Можно было бы еще позаменять недопустимые символы на '_' но пока обойдемся
				}
			}

			if (szArg.IsEmpty())
			{
				szArg.Set(L"-");
			}
		}

		// Proxy?
		if (pszProxy || pszProxyLogin)
		{
			args[0].strArg = pszProxy;         args[0].argType = at_Str;
			args[1].strArg = pszProxyLogin;    args[1].argType = at_Str;
			args[2].strArg = pszProxyPassword; args[2].argType = at_Str;
			DownloadCommand(dc_SetProxy, 3, args);
		}

		// Server login
		if (pszLogin)
		{
			args[0].strArg = pszLogin;    args[0].argType = at_Str;
			args[1].strArg = pszPassword; args[1].argType = at_Str;
			DownloadCommand(dc_SetLogin, 2, args);
		}

		// Sync or Ansync mode?
		if (pszAsync)
		{
			args[0].uintArg = *pszAsync && (pszAsync[0] != L'0') && (pszAsync[0] != L'N') && (pszAsync[0] != L'n'); args[0].argType = at_Uint;
			DownloadCommand(dc_SetAsync, 1, args);
		}

		// Timeouts: all, INTERNET_OPTION_RECEIVE_TIMEOUT, INTERNET_OPTION_DATA_RECEIVE_TIMEOUT
		for (UINT i = 0; i <= 2; i++)
		{
			LPCWSTR pszValue = NULL; wchar_t* pszEnd;
			switch (i)
			{
			case 0: pszValue = pszTimeout; break;
			case 1: pszValue = pszTimeout1; break;
			case 2: pszValue = pszTimeout2; break;
			}
			if (!pszValue || !*pszValue)
				continue;
			args[0].uintArg = i; args[0].argType = at_Uint;
			args[1].uintArg = wcstol(pszValue, &pszEnd, 10); args[1].argType = at_Uint;
			DownloadCommand(dc_SetTimeout, 2, args);
		}

		// Default is "ConEmu Update", user may override it
		if (pszAgent)
		{
			args[0].strArg = pszAgent; args[0].argType = at_Str;
			DownloadCommand(dc_SetAgent, 1, args);
		}

		// Done, now set URL and target file
		args[0].strArg = pszUrl; args[0].argType = at_Str;
		args[1].strArg = szArg;  args[1].argType = at_Str;
		args[2].uintArg = TRUE;  args[2].argType = at_Uint;

		// May be file name was specified relatively or even with env.vars?
		SafeFree(pszExpanded);
		if (lstrcmp(szArg, L"-") != 0)
		{
			pszExpanded = ExpandEnvStr(szArg);
			nFullRc = GetFullPathName((pszExpanded && *pszExpanded) ? pszExpanded : (LPCWSTR)szArg, countof(szFullPath), szFullPath, NULL);
			if (nFullRc && nFullRc < countof(szFullPath))
				args[1].strArg = szFullPath;
		}

		InterlockedIncrement(&gnIsDownloading);
		drc = DownloadCommand(dc_DownloadFile, 3, args);
		InterlockedDecrement(&gnIsDownloading);

		if (drc == 0)
		{
			iRc = CERR_DOWNLOAD_FAILED;
			DownloadLog(dc_ErrCallback, L"Download failed");
			goto wrap;
		}
		else
		{
			wchar_t szInfo[100];
			iFiles++;
			_wsprintf(szInfo, SKIPCOUNT(szInfo) L"File #%u downloaded, size=%u, crc32=x%08X", (DWORD)iFiles, (DWORD)args[0].uintArg, (DWORD)args[1].uintArg);
			DownloadLog(dc_LogCallback, szInfo);
		}
	}

	iRc = iFiles ? CERR_DOWNLOAD_SUCCEEDED : CERR_CARGUMENT;
wrap:
	// Log exit code
	_wsprintf(szResult, SKIPCOUNT(szResult)
		L"Exit with code %s (%i)",
		(iRc==CERR_DOWNLOAD_SUCCEEDED) ? L"CERR_DOWNLOAD_SUCCEEDED" :
		(iRc==CERR_DOWNLOAD_SUCCEEDED) ? L"CERR_DOWNLOAD_FAILED" :
		(iRc==CERR_CARGUMENT) ? L"CERR_CARGUMENT" :
		L"OtherExitCode",
		iRc);
	DownloadLog(dc_LogCallback, szResult);
	// Finalize internet service
	DownloadCommand(dc_Deinit, 0, NULL);
	SafeFree(pszExpanded);
	for (size_t i = 0; KnownArgs[i].pszArgName; i++)
	{
		if (KnownArgs[i].ppszValue)
		{
			SafeFree(*KnownArgs[i].ppszValue);
		}
	}
	return iRc;
}
