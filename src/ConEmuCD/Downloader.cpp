
/*
Copyright (c) 2013-2014 Maximus5
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
#include "../common/defines.h"
#include "../common/MAssert.h"
#include "Downloader.h"

#ifdef __GNUC__
typedef struct {
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
} HTTP_VERSION_INFO, * LPHTTP_VERSION_INFO;
#define INTERNET_OPTION_HTTP_VERSION 59
#define SecureZeroMemory(p,s) memset(p,0,s)
struct InternetCookieHistory
{
    BOOL    fAccepted;
    BOOL    fLeashed;
    BOOL    fDowngraded;
    BOOL    fRejected;
};
#define INTERNET_STATUS_COOKIE_SENT             320
#define INTERNET_STATUS_COOKIE_RECEIVED         321
#define INTERNET_STATUS_COOKIE_HISTORY          327
#define INTERNET_STATUS_INTERMEDIATE_RESPONSE   120
#define INTERNET_STATUS_DETECTING_PROXY         80
#define INTERNET_STATUS_STATE_CHANGE            200
#define INTERNET_STATUS_P3P_HEADER              325
#endif


#if defined(__GNUC__)
extern "C"
#endif
bool WINAPI CalcCRC(const BYTE *pData, size_t cchSize, DWORD& crc)
{
	if (!pData)
	{
		_ASSERTE(pData==NULL || cchSize==0);
		return false;
	}

	static DWORD CRCtable[] =
	{
		0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 
		0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 
		0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2, 
		0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7, 
		0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 
		0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 
		0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C, 
		0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59, 
		0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 
		0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 
		0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106, 
		0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433, 
		0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 
		0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 
		0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950, 
		0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65, 
		0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 
		0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 
		0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA, 
		0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F, 
		0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 
		0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 
		0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84, 
		0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1, 
		0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 
		0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 
		0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E, 
		0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B, 
		0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 
		0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 
		0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28, 
		0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D, 
		0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 
		0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 
		0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242, 
		0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777, 
		0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 
		0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 
		0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC, 
		0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9, 
		0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 
		0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 
		0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
	};
	
	//crc = 0xFFFFFFFF;

	for (const BYTE* p = pData; cchSize; cchSize--)
	{
		crc = ( crc >> 8 ) ^ CRCtable[(unsigned char) ((unsigned char) crc ^ *p++ )];
	}

	//crc ^= 0xFFFFFFFF;

	return true;
}


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
	CRITICAL_SECTION mcs_Handle;

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
	
	DWORD mn_Timeout;     // DOWNLOADTIMEOUT by default
	DWORD mn_ConnTimeout; // INTERNET_OPTION_RECEIVE_TIMEOUT
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
	bool ExecRequest(BOOL bResult, DWORD& nErrCode);
	HINTERNET ExecRequest(HINTERNET hResult, DWORD& nErrCode);

public:
	CDownloader();
	virtual ~CDownloader();
	
	void SetProxy(LPCWSTR asProxy, LPCWSTR asProxyUser, LPCWSTR asProxyPassword);
	void SetLogin(LPCWSTR asUser, LPCWSTR asPassword);
	void SetCallback(CEDownloadCommand cb, FDownloadCallback afnErrCallback, LPARAM lParam);
	void SetAsync(bool bAsync);
	void SetTimeout(bool bOperation, DWORD nTimeout);

	BOOL DownloadFile(LPCWSTR asSource, LPCWSTR asTarget, HANDLE hDstFile, DWORD& crc, DWORD& size, BOOL abShowAllErrors = FALSE);

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

	mb_AsyncMode = true;
	mn_Timeout = DOWNLOADTIMEOUT;
	mn_ConnTimeout = mn_DataTimeout = 0;
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

	InitializeCriticalSection(&mcs_Handle);
}

CDownloader::~CDownloader()
{
	CloseInternet(true);
	SetProxy(NULL, NULL, NULL);
	SetLogin(NULL, NULL);
	SafeCloseHandle(mh_CloseEvent);
	SafeCloseHandle(mh_ReadyEvent);
	DeleteCriticalSection(&mcs_Handle);
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
	LONG lCur = InterlockedIncrement(&mn_CloseRef);
	_ASSERTE(lCur == 1);
	ResetEvent(mh_CloseEvent);

	EnterCriticalSection(&mcs_Handle);

	bClose = wi->_InternetCloseHandle(h);
	nErrCode = GetLastError();
	if (!bClose)
	{
		ReportMessage(dc_LogCallback, L"Close handle x%08X failed, code=%u", at_Uint, (DWORD_PTR)h, at_Uint, nErrCode, at_None);
	}
	if (!bForceSync && mb_AsyncMode)
	{
		nWaitResult = WaitForSingleObject(mh_CloseEvent, DOWNLOADCLOSEHANDLETIMEOUT);
		ReportMessage(dc_LogCallback, L"Async close handle x%08X wait result=%u", at_Uint, (DWORD_PTR)h, at_Uint, nWaitResult, at_None);
	}

	LeaveCriticalSection(&mcs_Handle);

	// Done
	h = NULL;
	InterlockedDecrement(&mn_CloseRef);
	return (bClose != FALSE);
}

bool CDownloader::ExecRequest(BOOL bResult, DWORD& nErrCode)
{
	DWORD nWaitResult;
	nErrCode = bResult ? 0 : GetLastError();

	LeaveCriticalSection(&mcs_Handle);

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

HINTERNET CDownloader::ExecRequest(HINTERNET hResult, DWORD& nErrCode)
{
	DWORD nWaitResult;
	nErrCode = hResult ? 0 : GetLastError();

	LeaveCriticalSection(&mcs_Handle);

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
            LogCallback(L"Handle Closing x%08x", (DWORD)*((HINTERNET*)lpvStatusInformation));
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
		{&mn_ConnTimeout, INTERNET_OPTION_RECEIVE_TIMEOUT, L"receive"},
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

BOOL CDownloader::DownloadFile(LPCWSTR asSource, LPCWSTR asTarget, HANDLE hDstFile, DWORD& crc, DWORD& size, BOOL abShowAllErrors /*= FALSE*/)
{
	BOOL lbRc = FALSE, lbRead = FALSE, lbWrite = FALSE;
	DWORD nRead;
	bool lbNeedTargetClose = false;
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
	wchar_t szAgent[] =
		//L"Mozilla/5.0 (compatible; ConEmu Update)" // This was the cause of not working download redirects
		L"ConEmu Update" // so we use that to enable redirects
		;
	LPCWSTR szAcceptTypes[] = {L"*/*", NULL};
	LPCWSTR* ppszAcceptTypes = szAcceptTypes;
	LPCWSTR pszReferrer = NULL;
	DWORD nFlags, nService;
	BOOL bFRc;
	DWORD dwErr;

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

	if (hDstFile == NULL || hDstFile == INVALID_HANDLE_VALUE)
	{
		// Strange, in Update.cpp file was "created" with OPEN_EXISTING flag
		hDstFile = CreateFile(asTarget, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
		if (!hDstFile || hDstFile == INVALID_HANDLE_VALUE)
		{
			ReportMessage(dc_ErrCallback,
				L"Failed to create target file(%s), code=%u", at_Str, asTarget, at_Uint, GetLastError(), at_None);
			goto wrap;
		}
		lbNeedTargetClose = true;
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
			ReportMessage(dc_LogCallback, L"Open internet");
			nFlags = (mb_AsyncMode ? INTERNET_FLAG_ASYNC : 0);
			mh_Internet = wi->_InternetOpenW(szAgent, ProxyType, ProxyName, NULL, nFlags);
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
		}

		// Timeout
		if (!SetupTimeouts())
		{
			goto wrap;
		}


		// 
		_ASSERTE(mh_Connect == NULL);
		
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
		EnterCriticalSection(&mcs_Handle); // Leaved in ExecRequest
		mh_Connect = ExecRequest(wi->_InternetConnectW(mh_Internet, szServer, nServerPort, m_Server.szUser, m_Server.szPassword, nService, nFlags, (DWORD_PTR)this), dwErr);
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

			EnterCriticalSection(&mcs_Handle); // Leaved in ExecRequest
			if (!ExecRequest(wi->_FtpSetCurrentDirectoryW(mh_Connect, pszSetDir), dwErr))
			{
				ReportMessage(dc_ErrCallback, L"Ftp set directory failed '%s', code=%u", at_Str, pszSetDir, at_Uint, GetLastError(), at_None);
				if (pszSlash) *pszSlash = L'/'; // return it back
				goto wrap;
			}
			//WaitAsyncResult();

			if (pszSlash) *pszSlash = L'/'; // return it back
			nFlags = FTP_TRANSFER_TYPE_BINARY;

			EnterCriticalSection(&mcs_Handle); // Leaved in ExecRequest
			mh_SrcFile = ExecRequest(wi->_FtpOpenFileW(mh_Connect, pszFile, GENERIC_READ, nFlags, (DWORD_PTR)this), dwErr);
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
				|INTERNET_FLAG_NO_CACHE_WRITE
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
			EnterCriticalSection(&mcs_Handle); // Leaved in ExecRequest
			bFRc = ExecRequest(wi->_HttpSendRequestW(mh_SrcFile,NULL,0,NULL,0), dwErr);

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
				EnterCriticalSection(&mcs_Handle); // Leaved in ExecRequest
				if (!ExecRequest(wi->_HttpQueryInfoW(mh_SrcFile, nFlags, &mn_InternetContentLen, &sz, &dwIndex), dwErr))
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

void CDownloader::SetTimeout(bool bOperation, DWORD nTimeout)
{
	ReportMessage(dc_LogCallback, L"Set %s timeout to %u was requested", at_Str, bOperation?L"operation":L"receive", at_Uint, nTimeout, at_None);
	if (bOperation)
	{
		mn_Timeout = nTimeout;
	}
	else
	{
		mn_ConnTimeout = mn_DataTimeout = nTimeout;
	}
}

BOOL CDownloader::ReadSource(LPCWSTR asSource, BOOL bInet, HANDLE hSource, BYTE* pData, DWORD cbData, DWORD* pcbRead)
{
	BOOL lbRc = FALSE;
	DWORD dwErr = 0;
	
	if (bInet)
	{
		ReportMessage(dc_LogCallback, L"Reading source");
		*pcbRead = 0;
		EnterCriticalSection(&mcs_Handle); // Leaved in ExecRequest
		lbRc = ExecRequest(wi->_InternetReadFile(hSource, pData, cbData, pcbRead), dwErr);

		if (mb_AsyncMode && lbRc && !*pcbRead && dwErr)
		{
			_ASSERTE(mb_FtpMode && dwErr <= cbData);
			if (dwErr <= cbData)
				*pcbRead = dwErr;
		}

		if (!lbRc)
			ReportMessage(dc_ErrCallback,
				L"DownloadFile(%s) failed, code=%u", at_Str, asSource, at_Uint, dwErr, at_None);
		else
			ReportMessage(dc_LogCallback, L"Retrieved %u bytes", at_Uint, *pcbRead, at_None);
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

		argType = va_arg( argptr, CEDownloadArgType );
	}

	args.argCount = i;

	mfn_Callback[rm](&args);
}

void CDownloader::UpdateProgress()
{
	ReportMessage(dc_ProgressCallback, L"Bytes downloaded %u", at_Uint, mn_InternetContentReady, at_None);
}

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
	case dc_DownloadFile: // [0]="http", [1]="DestLocalFilePath"
		if (gpInet && (argc >= 2))
		{
			DWORD crc = 0, size = 0;
			nResult = gpInet->DownloadFile(
				(argc > 0 && argv[0].argType == at_Str) ? argv[0].strArg : NULL,
				(argc > 1 && argv[1].argType == at_Str) ? argv[1].strArg : NULL,
				(argc > 2) ? (HANDLE)argv[2].uintArg : 0,
				crc, size,
				(argc > 3) ? argv[3].uintArg : TRUE);
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
			gpInet->SetTimeout(argv[0].uintArg == 0, argv[1].uintArg);
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
