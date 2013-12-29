
/*
Copyright (c) 2013 Maximus5
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
#endif


static bool CalcCRC(const BYTE *pData, size_t cchSize, DWORD& crc)
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
	HANDLE mh_Internet, mh_Connect, mh_SrcFile; // Used
	INTERNET_STATUS_CALLBACK mp_SetCallbackRc;

	DWORD mn_InternetContentLen, mn_InternetContentReady; // Used
	void CloseInternet(bool bFull);

	bool InitInterface();
	
	struct {
		wchar_t* szUpdateProxy;
		wchar_t* szUpdateProxyUser;
		wchar_t* szUpdateProxyPassword;
	} m_Proxy;
	
	DWORD mn_Timeout, mn_ConnTimeout, mn_DataTimeout, mn_FileTimeout;
	
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

public:
	CDownloader();
	virtual ~CDownloader();
	
	void SetProxy(LPCWSTR asProxy, LPCWSTR asProxyUser, LPCWSTR asProxyPassword);
	void SetCallback(CEDownloadCommand cb, FDownloadCallback afnErrCallback, LPARAM lParam);

	BOOL DownloadFile(LPCWSTR asSource, LPCWSTR asTarget, HANDLE hDstFile, DWORD& crc, DWORD& size, BOOL abShowAllErrors = FALSE);

protected:
	BOOL mb_RequestTerminate; // Used
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
protected:
	HMODULE _hWinInet;
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
		char func[64];

		for (wchar_t* p = name; *p && *(p+1); p+=2) { wchar_t c = p[0]; p[0] = p[1]; p[1] = c; }
		_hWinInet = LoadLibrary(name);
		if (!_hWinInet)
		{
			pUpd->ReportMessage(dc_ErrCallback,
				L"LoadLibrary(%s) failed, code=%u", at_Str, name, at_Uint, GetLastError(), at_None);
			return false;
		}

		#define LoadFunc(s,n) \
			lstrcpyA(func,n); \
			for (char* p = func; *p && *(p+1); p+=2) { char c = p[0]; p[0] = p[1]; p[1] = c; } \
			_##s = (s##_t)GetProcAddress(_hWinInet, func); \
			if (_##s == NULL) \
			{ \
				MultiByteToWideChar(CP_ACP, 0, func, -1, name, countof(name)); \
				pUpd->ReportMessage(dc_ErrCallback, \
					L"GetProcAddress(%s) failed, code=%u", at_Str, name, at_Uint, GetLastError(), at_None); \
				FreeLibrary(_hWinInet); \
				_hWinInet = NULL; \
				return false; \
			}

		//const char* Htt = "Htt";
		////const char* Interne = "qqq"; // "Interne";
		//char Interne[128], FuncEnd[128];
		//Interne[0] = 'I'; Interne[2] = 't'; Interne[4] = 'r'; Interne[6] = 'e';
		//Interne[1] = 'n'; Interne[3] = 'e'; Interne[5] = 'n'; Interne[7] = 0;

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

	mn_Timeout = DOWNLOADTIMEOUT;
	mn_ConnTimeout = mn_FileTimeout = 0;
	//mh_StopThread = NULL;
	mb_RequestTerminate = FALSE;
	mb_InetMode = false;
	mh_Internet = mh_Connect = mh_SrcFile = NULL;
	mp_SetCallbackRc = NULL;
	mn_InternetContentLen = mn_InternetContentReady = 0;
	wi = NULL;
}

CDownloader::~CDownloader()
{
	CloseInternet(true);
	SetProxy(NULL, NULL, NULL);
}

// asProxy = "" - autoconfigure
// asProxy = "server:port"
void CDownloader::SetProxy(LPCWSTR asProxy, LPCWSTR asProxyUser, LPCWSTR asProxyPassword)
{
	SafeFree(m_Proxy.szUpdateProxy);
	SafeFree(m_Proxy.szUpdateProxyUser);
	if (m_Proxy.szUpdateProxyPassword)
		SecureZeroMemory(m_Proxy.szUpdateProxyPassword, lstrlen(m_Proxy.szUpdateProxyPassword)*sizeof(*m_Proxy.szUpdateProxyPassword));
	SafeFree(m_Proxy.szUpdateProxyPassword);

	if (asProxy)
		m_Proxy.szUpdateProxy = lstrdup(asProxy);
	if (asProxyUser)
		m_Proxy.szUpdateProxyUser = lstrdup(asProxyUser);
	if (asProxyPassword)
		m_Proxy.szUpdateProxyPassword = lstrdup(asProxyPassword);
}

bool CDownloader::SetProxyForHandle(HANDLE hInternet)
{
	bool bOk = false;

	if (m_Proxy.szUpdateProxyUser && *m_Proxy.szUpdateProxyUser)
	{
		if (!wi->_InternetSetOptionW(hInternet, INTERNET_OPTION_PROXY_USERNAME, (LPVOID)m_Proxy.szUpdateProxyUser, lstrlen(m_Proxy.szUpdateProxyUser)))
		{
			ReportMessage(dc_ErrCallback,
				L"ProxyUserName failed, code=%u", at_Uint, GetLastError(), at_None);
			goto wrap;
		}
	}
	if (m_Proxy.szUpdateProxyPassword && *m_Proxy.szUpdateProxyPassword)
	{
		if (!wi->_InternetSetOptionW(hInternet, INTERNET_OPTION_PROXY_PASSWORD, (LPVOID)m_Proxy.szUpdateProxyPassword, lstrlen(m_Proxy.szUpdateProxyPassword)))
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
		}
	}
	if (!wi || !wi->Init(this))
	{
		ReportMessage(dc_ErrCallback,
			L"CWinInet initialization failed", at_None);
		goto wrap;
	}
	bRc = true;
wrap:
	return bRc;
}

VOID CDownloader::InetCallback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength)
{
	return;
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
	//DWORD nTotalSize = 0;
	BOOL lbFirstThunk = TRUE;
	DWORD nConnTimeoutSet, nDataTimeoutSet, cbSize;
	DWORD ProxyType = INTERNET_OPEN_TYPE_DIRECT;
	LPCWSTR ProxyName = NULL;
	wchar_t szServer[MAX_PATH];
	wchar_t* pszSrvPath = NULL;
	wchar_t *pszColon;
	INTERNET_PORT nServerPort = INTERNET_DEFAULT_HTTP_PORT;
	bool bSecureHTTPS = false;
	HTTP_VERSION_INFO httpver = {1,1};
	wchar_t szHttpVer[32]; _wsprintf(szHttpVer, SKIPLEN(countof(szHttpVer)) L"HTTP/%u.%u", httpver.dwMajorVersion, httpver.dwMinorVersion);
	wchar_t szAgent[] = L"Mozilla/5.0 (compatible; ConEmu Update)";
	LPCWSTR szAcceptTypes[] = {L"*/*", NULL};
	LPCWSTR pszReferrer = NULL;
	DWORD nFlags;

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

	mn_InternetContentReady = 0;
	mn_InternetContentLen = 0;
	
	crc = 0xFFFFFFFF;
	
	if (mb_InetMode)
	{
		LPCWSTR pszSource;
		if (memcmp(asSource, L"http://", 7*sizeof(*asSource)) == 0)
		{
			bSecureHTTPS = false;
			pszSource = asSource + 7;
		}
		if (memcmp(asSource, L"https://", 8*sizeof(*asSource)) == 0)
		{
			bSecureHTTPS = true;
			pszSource = asSource + 8;
			nServerPort = INTERNET_DEFAULT_HTTPS_PORT;
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
		if (m_Proxy.szUpdateProxy)
		{
			if (m_Proxy.szUpdateProxy && *m_Proxy.szUpdateProxy)
			{
				ProxyType = INTERNET_OPEN_TYPE_PROXY;
				ProxyName = m_Proxy.szUpdateProxy;
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
			nFlags = 0; //INTERNET_FLAG_ASYNC;
			mh_Internet = wi->_InternetOpenW(szAgent, ProxyType, ProxyName, NULL, nFlags);
			if (!mh_Internet)
			{
				DWORD dwErr = GetLastError();
				ReportMessage(dc_ErrCallback,
					L"Network initialization failed, code=%u", at_Uint, dwErr, at_None);
				goto wrap;
			}

			#if 0
			mp_SetCallbackRc = wi->_InternetSetStatusCallbackW(mh_Internet, InetCallback);
			_ASSERTE(mp_SetCallbackRc != INTERNET_INVALID_STATUS_CALLBACK);
			#endif

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
		if (!mn_ConnTimeout)
		{
			cbSize = sizeof(mn_ConnTimeout);
			if (!wi->_InternetQueryOptionW(mh_Internet, INTERNET_OPTION_RECEIVE_TIMEOUT, &mn_ConnTimeout, &cbSize))
			{
				ReportMessage(dc_LogCallback, L"Warning: Query internet receive timeout failed, code=%u", at_Uint, GetLastError(), at_None);
				mn_ConnTimeout = 0;
			}
			else
			{
				ReportMessage(dc_LogCallback, L"Current internet receive timeout: %u ms", at_Uint, mn_ConnTimeout, at_None);
			}
		}
		nConnTimeoutSet = max(mn_ConnTimeout,mn_Timeout);
		ReportMessage(dc_LogCallback, L"Set internet receive timeout: %u ms", at_Uint, nConnTimeoutSet, at_None);
		if (!wi->_InternetSetOptionW(mh_Internet, INTERNET_OPTION_RECEIVE_TIMEOUT, &nConnTimeoutSet, sizeof(nConnTimeoutSet)))
		{
			ReportMessage(dc_ErrCallback,
				L"INTERNET_OPTION_RECEIVE_TIMEOUT(mh_Internet,%u) failed, code=%u",
				at_Uint, nConnTimeoutSet, at_Uint, GetLastError(), at_None);
			goto wrap;
		}
		if (!mn_DataTimeout)
		{
			cbSize = sizeof(mn_ConnTimeout);
			if (!wi->_InternetQueryOptionW(mh_Internet, INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, &mn_DataTimeout, &cbSize))
			{
				ReportMessage(dc_LogCallback, L"Warning: Query internet data receive timeout failed, code=%u", at_Uint, GetLastError(), at_None);
				mn_ConnTimeout = 0;
			}
			else
			{
				ReportMessage(dc_LogCallback, L"Current internet data receive timeout: %u ms", at_Uint, mn_DataTimeout, at_None);
			}
		}
		nDataTimeoutSet = max(mn_DataTimeout,mn_Timeout);
		ReportMessage(dc_LogCallback, L"Set internet receive timeout: %u ms", at_Uint, nConnTimeoutSet, at_None);
		if (!wi->_InternetSetOptionW(mh_Internet, INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, &nDataTimeoutSet, sizeof(nDataTimeoutSet)))
		{
			ReportMessage(dc_ErrCallback,
				L"INTERNET_OPTION_DATA_RECEIVE_TIMEOUT(mh_Internet,%u) failed, code=%u",
				at_Uint, nDataTimeoutSet, at_Uint, GetLastError(), at_None);
			goto wrap;
		}


		// 
		_ASSERTE(mh_Connect == NULL);
		
		//TODO после включения ноута вылезла ошибка ERROR_INTERNET_NAME_NOT_RESOLVED==12007

		// Server:Port
		if ((pszColon = wcsrchr(szServer, L':')) != NULL)
		{
			*pszColon = 0;
			nServerPort = (INTERNET_PORT)LOWORD(wcstoul(pszColon+1, &pszColon, 10));
			if (!nServerPort)
				nServerPort = INTERNET_DEFAULT_HTTP_PORT;
		}
		nFlags = 0; // No special flags
		mh_Connect = wi->_InternetConnectW(mh_Internet, szServer, nServerPort, NULL, NULL, INTERNET_SERVICE_HTTP, nFlags, (DWORD_PTR)this);
		if (!mh_Connect)
		{
			DWORD dwErr = GetLastError();
			if (abShowAllErrors)
			{
				ReportMessage(dc_ErrCallback,
					L"Connection failed, code=%u", at_Uint, dwErr, at_None);
			}
			goto wrap;
		}

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

		//INTERNET_OPTION_RECEIVE_TIMEOUT - Sets or retrieves an unsigned long integer value that contains the time-out value, in milliseconds");
		//nConnTimeout, nConnTimeoutSet, nFileTimeout, nFileTimeoutSet
		//if (!mn_ConnTimeout)
		//{
		//	cbSize = sizeof(mn_ConnTimeout);
		//	if (!wi->_InternetQueryOptionW(mh_Connect, INTERNET_OPTION_RECEIVE_TIMEOUT, &mn_ConnTimeout, &cbSize))
		//		mn_ConnTimeout = 0;
		//}
		//nConnTimeoutSet = max(mn_ConnTimeout,mn_Timeout);
		//if (!wi->_InternetSetOptionW(mh_Connect, INTERNET_OPTION_RECEIVE_TIMEOUT, &nConnTimeoutSet, sizeof(nConnTimeoutSet)))
		//{
		//	wchar_t szErr[128]; DWORD nErr = GetLastError();
		//	_wsprintf(szErr, SKIPLEN(countof(szErr)) L"INTERNET_OPTION_RECEIVE_TIMEOUT(mh_Connect,%u) failed, code=%u", nConnTimeoutSet, nErr);
		//	ReportError(szErr, nErr);
		//	goto wrap;
		//}


		if (mb_RequestTerminate)
			goto wrap;

		_ASSERTE(mh_SrcFile==NULL);
		// Send request for the file
		nFlags = INTERNET_FLAG_KEEP_CONNECTION
			|(bSecureHTTPS?(INTERNET_FLAG_SECURE|INTERNET_FLAG_IGNORE_CERT_CN_INVALID|INTERNET_FLAG_IGNORE_CERT_DATE_INVALID):0)
			|INTERNET_FLAG_HYPERLINK
			//|INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP
			|INTERNET_FLAG_NO_CACHE_WRITE
			|INTERNET_FLAG_PRAGMA_NOCACHE
			|INTERNET_FLAG_RELOAD;
		ReportMessage(dc_LogCallback, L"Opening request with flags x%08X", at_Uint, nFlags, at_None);
		mh_SrcFile = wi->_HttpOpenRequestW(mh_Connect, L"GET", pszSrvPath, szHttpVer, pszReferrer,
			szAcceptTypes, nFlags, (DWORD_PTR)this);
		if (!mh_SrcFile || (mh_SrcFile == INVALID_HANDLE_VALUE))
		{
			DWORD dwErr = GetLastError();
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

		ReportMessage(dc_LogCallback, L"Sending request");
		if (!wi->_HttpSendRequestW(mh_SrcFile,NULL,0,NULL,0))
		{
			DWORD dwErr = GetLastError();
			if (abShowAllErrors)
			{
				ReportMessage(dc_ErrCallback,
					L"HttpSendRequest failed\nURL=%s\ncode=%u", at_Str, asSource, at_Uint, dwErr, at_None);
			}
			goto wrap;
		}
		else
		{
			mn_InternetContentLen = 0;
			DWORD sz = sizeof(mn_InternetContentLen);
			DWORD dwIndex = 0;
			nFlags = HTTP_QUERY_CONTENT_LENGTH|HTTP_QUERY_FLAG_NUMBER;
			ReportMessage(dc_LogCallback, L"Quering file info with flags x%08X", at_Uint, nFlags, at_None);
			if (!wi->_HttpQueryInfoW(mh_SrcFile, nFlags, &mn_InternetContentLen, &sz, &dwIndex))
			{
				DWORD dwErr = GetLastError();
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
		}


		////Because of some antivirus programs tail of file may arrives with long delay...
		////INTERNET_OPTION_RECEIVE_TIMEOUT - Sets or retrieves an unsigned long integer value that contains the time-out value, in milliseconds");
		////nConnTimeout, nConnTimeoutSet, nFileTimeout, nFileTimeoutSet
		//if (!mn_FileTimeout)
		//{
		//	cbSize = sizeof(mn_FileTimeout);
		//	if (!wi->_InternetQueryOptionW(mh_SrcFile, INTERNET_OPTION_RECEIVE_TIMEOUT, &mn_FileTimeout, &cbSize))
		//		mn_FileTimeout = 0;
		//}
		//nFileTimeoutSet = max(mn_FileTimeout,mn_Timeout);
		//if (!wi->_InternetSetOptionW(hSrcFile, INTERNET_OPTION_RECEIVE_TIMEOUT, &nFileTimeoutSet, sizeof(nFileTimeoutSet)))
		//{
		//	wchar_t szErr[128]; DWORD nErr = GetLastError();
		//	_wsprintf(szErr, SKIPLEN(countof(szErr)) L"INTERNET_OPTION_RECEIVE_TIMEOUT(hSrcFile,%u) failed, code=%u", nFileTimeoutSet, nErr);
		//	ReportError(szErr, nErr);
		//	goto wrap;
		//}

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
	
	while (TRUE)
	{
		if (mb_RequestTerminate)
			goto wrap;

		lbRead = ReadSource(asSource, mb_InetMode, mh_SrcFile, ptrData, cchDataMax, &nRead);
		if (!lbRead)
			goto wrap;
			
		if (!nRead)
		{
			if (!mn_InternetContentReady)
			{
				ReportMessage(dc_ErrCallback,
					L"Invalid source file (%s), file is empty", at_Str, asSource, at_None);
				goto wrap;
			}
			break;
		}
		
		CalcCRC(ptrData, nRead, crc);
		
		lbWrite = WriteTarget(asTarget, hDstFile, ptrData, nRead);
		if (!lbWrite)
			goto wrap;
		
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

		mn_InternetContentReady += nRead;

		UpdateProgress();
	}
	
	// Succeeded
	crc ^= 0xFFFFFFFF;
	lbRc = TRUE;
wrap:
	size = mn_InternetContentReady;

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
	DWORD nCloseErr = 0;

	if (wi)
	{
		if (mh_SrcFile && (mh_SrcFile != INVALID_HANDLE_VALUE))
		{
			ReportMessage(dc_LogCallback, L"Close source handle");
			bClose = wi->_InternetCloseHandle(mh_SrcFile);
			if (!bClose)
				nCloseErr = GetLastError();
		}

		if (mh_Connect)
		{
			ReportMessage(dc_LogCallback, L"Close connect handle");
			bClose = wi->_InternetCloseHandle(mh_Connect);
			if (!bClose)
				nCloseErr = GetLastError();
		}

		if (bFull && mh_Internet)
		{
			ReportMessage(dc_LogCallback, L"Close internet handle");
			bClose = wi->_InternetCloseHandle(mh_Internet);
			if (!bClose)
				nCloseErr = GetLastError();
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

	UNREFERENCED_PARAMETER(nCloseErr);
}


BOOL CDownloader::ReadSource(LPCWSTR asSource, BOOL bInet, HANDLE hSource, BYTE* pData, DWORD cbData, DWORD* pcbRead)
{
	BOOL lbRc = FALSE;
	
	if (bInet)
	{
		ReportMessage(dc_LogCallback, L"Reading source");
		lbRc = wi->_InternetReadFile(hSource, pData, cbData, pcbRead);
		if (!lbRc)
			ReportMessage(dc_ErrCallback,
				L"DownloadFile(%s) failed, code=%u", at_Str, asSource, at_Uint, GetLastError(), at_None);
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
DWORD_PTR DownloadCommand(CEDownloadCommand cmd, int argc, CEDownloadErrorArg* argv)
{
	DWORD_PTR nResult = 0;

	if (!argv) argc = 0;

	switch (cmd)
	{
	case dc_Init:
		if (!gpInet)
			gpInet = new CDownloader;
		break;
	case dc_Reset:
		break;
	case dc_Deinit:
		SafeDelete(gpInet);
		break;
	case dc_SetProxy: // [0]="Server:Port", [1]="User", [2]="Password"
		if (gpInet)
		{
			gpInet->SetProxy(
				(argc > 0 && argv[0].argType == at_Str) ? argv[0].strArg : NULL,
				(argc > 1 && argv[1].argType == at_Str) ? argv[1].strArg : NULL,
				(argc > 2 && argv[2].argType == at_Str) ? argv[2].strArg : NULL);
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
	}

	return nResult;
}
