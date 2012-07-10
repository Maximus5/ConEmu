
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

#define HIDE_USE_EXCEPTION_INFO
#include "Header.h"
#include <Wininet.h>
#include <shlobj.h>
#include "Update.h"
#include "UpdateSet.h"
#include "Options.h"
#include "ConEmu.h"
#include "TrayIcon.h"
#include "version.h"

CConEmuUpdate* gpUpd = NULL;

#define UPDATETHREADTIMEOUT 2500

static bool CalcCRC(const BYTE *pData, size_t cchSize, DWORD& crc)
{
	if (!pData)
	{
		_ASSERTE(pData==NULL || cchSize==0)
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

	HttpOpenRequestW_t _HttpOpenRequestW;
	HttpQueryInfoW_t _HttpQueryInfoW;
	HttpSendRequestW_t _HttpSendRequestW;
	InternetCloseHandle_t _InternetCloseHandle;
	InternetConnectW_t _InternetConnectW;
	InternetOpenW_t _InternetOpenW;
	InternetReadFile_t _InternetReadFile;
	InternetSetOptionW_t _InternetSetOptionW;
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
	};
	~CWinInet()
	{
		if (_hWinInet)
			FreeLibrary(_hWinInet);
	};
	bool Init(CConEmuUpdate* pUpd)
	{
		if (_hWinInet)
			return true;

		wchar_t name[MAX_PATH] = L"iWInen.tldl";
		char func[64];

		for (wchar_t* p = name; *p && *(p+1); p+=2) { char c = p[0]; p[0] = p[1]; p[1] = c; }
		_hWinInet = LoadLibrary(name);
		if (!_hWinInet)
		{
			pUpd->ReportError(L"LoadLibrary(%s) failed, code=%u", name, GetLastError());
			return false;
		}

		#define LoadFunc(s,n) \
			lstrcpyA(func,n); \
			for (char* p = func; *p && *(p+1); p+=2) { char c = p[0]; p[0] = p[1]; p[1] = c; } \
			_##s = (s##_t)GetProcAddress(_hWinInet, func); \
			if (_##s == NULL) \
			{ \
				MultiByteToWideChar(CP_ACP, 0, func, -1, name, countof(name)); \
				pUpd->ReportError(L"GetProcAddress(%s) failed, code=%u", name, GetLastError()); \
				FreeLibrary(_hWinInet); \
				_hWinInet = NULL; \
				return false; \
			}

		//const char* Htt = "Htt";
		////const char* Interne = "qqq"; // "Interne";
		//char Interne[128], FuncEnd[128];
		//Interne[0] = 'I'; Interne[2] = 't'; Interne[4] = 'r'; Interne[6] = 'e';
		//Interne[1] = 'n'; Interne[3] = 'e'; Interne[5] = 'n'; Interne[7] = 0;

		LoadFunc(HttpOpenRequestW,"tHptpOneeRuqseWt");
		LoadFunc(HttpQueryInfoW,"tHptuQreIyfnWo");
		LoadFunc(HttpSendRequestW,"tHpteSdneRuqseWt");
		LoadFunc(InternetCloseHandle,"nIetnrtelCsoHenalde");
		LoadFunc(InternetConnectW,"nIetnrteoCnnceWt");
		LoadFunc(InternetSetOptionW,"nIetnrteeSOttpoiWn");
		LoadFunc(InternetOpenW,"nIetnrtepOneW");
		LoadFunc(InternetReadFile,"nIetnrteeRdaiFel");

		return true;
	}
};


CConEmuUpdate::CConEmuUpdate()
{
	mb_InCheckProcedure = FALSE;
	mn_CheckThreadId = 0;
	mh_CheckThread = NULL;
	//mh_StopThread = NULL;
	mb_RequestTerminate = FALSE;
	mp_Set = NULL;
	ms_LastErrorInfo = NULL;
	mb_InShowLastError = false;
	mp_LastErrorSC = new MSection;
	mh_Internet = mh_Connect = NULL;
	mn_InternetContentLen = mn_InternetContentReady = 0;
	mpsz_DeleteIniFile = mpsz_DeletePackageFile = mpsz_DeleteBatchFile = NULL;
	mpsz_PendingPackageFile = mpsz_PendingBatchFile = NULL;
	wi = NULL;
	m_UpdateStep = us_NotStarted;
	ms_NewVersion[0] = ms_CurVersion[0] = ms_SkipVersion[0] = 0;

	lstrcpyn(ms_DefaultTitle, gpConEmu->GetDefaultTitle(), countof(ms_DefaultTitle));
}

CConEmuUpdate::~CConEmuUpdate()
{
	if (mh_CheckThread)
	{
		DWORD nWait;
		if ((nWait = WaitForSingleObject(mh_CheckThread, 0)) == WAIT_TIMEOUT)
		{
			//_ASSERTE(mh_StopThread!=NULL);
			//SetEvent(mh_StopThread);
			mb_RequestTerminate = TRUE;
			nWait = WaitForSingleObject(mh_CheckThread, UPDATETHREADTIMEOUT);
		}
		
		if (nWait != WAIT_OBJECT_0)
		{
			TerminateThread(mh_CheckThread, 100);
		}
		
		CloseHandle(mh_CheckThread);
		mh_CheckThread = NULL;
	}
	
	//if (mh_StopThread)
	//{
	//	CloseHandle(mh_StopThread);
	//	mh_StopThread = NULL;
	//}

	DeleteBadTempFiles();

	if (wi)
	{
		delete wi;
		wi = NULL;
	}
	
	SafeFree(ms_LastErrorInfo);

	if (mp_LastErrorSC)
	{
		delete mp_LastErrorSC;
		mp_LastErrorSC = NULL;
	}

	if (m_UpdateStep == us_ExitAndUpdate && mpsz_PendingBatchFile)
	{
		WaitAllInstances();

		wchar_t *pszCmd = lstrdup(L"cmd.exe"); // Мало ли что в ComSpec пользователь засунул...
		size_t cchParmMax = lstrlen(mpsz_PendingBatchFile)+16;
		wchar_t *pszParm = (wchar_t*)calloc(cchParmMax,sizeof(*pszParm));
		// Обязательно двойное окавычивание. cmd.exe отбрасывает кавычки,
		// и при наличии разделителей (пробелы, скобки,...) получаем проблемы
		_wsprintf(pszParm, SKIPLEN(cchParmMax) L"/c \"\"%s\"\"", mpsz_PendingBatchFile);
		// ghWnd уже закрыт
		INT_PTR nShellRc = (INT_PTR)ShellExecute(NULL, bNeedRunElevation ? L"runas" : L"open", pszCmd, pszParm, NULL, SW_SHOWMINIMIZED);
		if (nShellRc <= 32)
		{
			wchar_t szErrInfo[MAX_PATH*4];
			_wsprintf(szErrInfo, SKIPLEN(countof(szErrInfo))
				L"Failed to start update batch\n%s\nError code=%i", mpsz_PendingBatchFile, (int)nShellRc);
			MessageBoxW(NULL, szErrInfo, L"ConEmu", MB_ICONSTOP|MB_SYSTEMMODAL);

			DeleteFile(mpsz_PendingBatchFile);
			if (!(mp_Set && mp_Set->isUpdateLeavePackages))
				DeleteFile(mpsz_PendingPackageFile);
		}
		SafeFree(pszCmd);
		SafeFree(pszParm);
	}
	SafeFree(mpsz_PendingBatchFile);
	SafeFree(mpsz_PendingPackageFile);

	if (mp_Set)
	{
		delete mp_Set;
		mp_Set = NULL;
	}
}

void CConEmuUpdate::StartCheckProcedure(BOOL abShowMessages)
{
	DWORD nWait = WAIT_OBJECT_0;
	
	if (InUpdate() != us_NotStarted)
	{
		// Already in update procedure
		if (m_UpdateStep == us_ExitAndUpdate)
		{
			// Повторно?
			gpConEmu->RequestExitUpdate();
		}
		else if (abShowMessages)
		{
			MBoxA(L"Checking for updates already started");
		}
		return;
	}

	gpSet->UpdSet.dwLastUpdateCheck = GetTickCount();

	// Сразу проверим, как нужно будет запускаться
	bNeedRunElevation = NeedRunElevation();
	
	mb_RequestTerminate = FALSE;
	//if (!mh_StopThread)
	//	mh_StopThread = CreateEvent(NULL, TRUE/*manual*/, FALSE, NULL);
	//ResetEvent(mh_StopThread);
	
	// Запомнить текущие параметры обновления
	if (!mp_Set)
		mp_Set = new ConEmuUpdateSettings;
	mp_Set->LoadFrom(&gpSet->UpdSet);
	
	mb_ManualCallMode = abShowMessages;
	{
		MSectionLock SC; SC.Lock(mp_LastErrorSC, TRUE);
		SafeFree(ms_LastErrorInfo);
	}

	wchar_t szReason[128];
	if (!mp_Set->UpdatesAllowed(szReason))
	{
		wchar_t szErrMsg[255]; wcscpy_c(szErrMsg, L"Updates are not enabled in ConEmu settings\n");
		wcscat_c(szErrMsg, szReason);
		DisplayLastError(szErrMsg, -1);
		return;
	}

	mb_InCheckProcedure = TRUE;	
	mh_CheckThread = CreateThread(NULL, 0, CheckThreadProc, this, 0, &mn_CheckThreadId);
	if (!mh_CheckThread)
	{
		mb_InCheckProcedure = FALSE;
		DisplayLastError(L"CreateThread(CheckThreadProc) failed");
		return;
	}
	// OK
}

void CConEmuUpdate::StopChecking()
{
	if (MessageBox(NULL, L"Are you sure, stop updates checking?", ms_DefaultTitle, MB_SYSTEMMODAL|MB_ICONQUESTION|MB_YESNO) != IDYES)
		return;

	mb_RequestTerminate = TRUE;

	if (mh_CheckThread)
	{
		DWORD nWait;
		if ((nWait = WaitForSingleObject(mh_CheckThread, 0)) == WAIT_TIMEOUT)
		{
			//_ASSERTE(mh_StopThread!=NULL);
			//SetEvent(mh_StopThread);
			mb_RequestTerminate = TRUE;
			nWait = WaitForSingleObject(mh_CheckThread, UPDATETHREADTIMEOUT);
		}

		if (nWait != WAIT_OBJECT_0)
		{
			TerminateThread(mh_CheckThread, 100);
		}
	}

	DeleteBadTempFiles();
	m_UpdateStep = us_NotStarted;

	if (mpsz_PendingBatchFile)
	{
		if (*mpsz_PendingBatchFile)
			DeleteFile(mpsz_PendingBatchFile);
		SafeFree(mpsz_PendingBatchFile);
	}

	if (mpsz_PendingPackageFile)
	{
		if (*mpsz_PendingPackageFile && !(mp_Set && mp_Set->isUpdateLeavePackages))
			DeleteFile(mpsz_PendingPackageFile);
		SafeFree(mpsz_PendingPackageFile);
	}
}

void CConEmuUpdate::ShowLastError()
{
	if (ms_LastErrorInfo && *ms_LastErrorInfo)
	{
		mb_InShowLastError = true;

		MSectionLock SC; SC.Lock(mp_LastErrorSC, TRUE);
		wchar_t* pszError = ms_LastErrorInfo;
		ms_LastErrorInfo = NULL;
		SC.Unlock();

		MBoxA(pszError);
		SafeFree(pszError);

		mb_InShowLastError = false;
	}
}

bool CConEmuUpdate::ShowConfirmation()
{
	bool lbConfirm = false;

	gpConEmu->UpdateProgress();

	if (ms_LastErrorInfo && *ms_LastErrorInfo)
	{
		mb_InShowLastError = true;

		MSectionLock SC; SC.Lock(mp_LastErrorSC, TRUE);
		wchar_t* pszConfirm = ms_LastErrorInfo;
		ms_LastErrorInfo = NULL;
		SC.Unlock();

		DontEnable de;
		int iBtn = MessageBox(NULL, pszConfirm, ms_DefaultTitle, MB_ICONQUESTION|MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_YESNO);

		SafeFree(pszConfirm);

		mb_InShowLastError = false;

		lbConfirm = (iBtn == IDYES);
	}
	else
	{
		MBoxAssert(ms_LastErrorInfo && *ms_LastErrorInfo);
	}

	if (!lbConfirm)
	{
		mb_RequestTerminate = true;
		gpConEmu->UpdateProgress();
	}

	return lbConfirm;
}

// Worker thread
DWORD CConEmuUpdate::CheckThreadProc(LPVOID lpParameter)
{
	DWORD nRc = 0;
	CConEmuUpdate *pUpdate = (CConEmuUpdate*)lpParameter;
	_ASSERTE(pUpdate!=NULL && pUpdate->mn_CheckThreadId==GetCurrentThreadId());
	
	SAFETRY
	{
		nRc = pUpdate->CheckProcInt();
	}
	SAFECATCH
	{
		pUpdate->ReportError(L"Exception in CheckProcInt", 0);
	}
	
	pUpdate->mb_InCheckProcedure = FALSE;
	gpConEmu->UpdateProgress();
	return nRc;
}

DWORD CConEmuUpdate::CheckProcInt()
{
	BOOL lbDownloadRc = FALSE, lbExecuteRc = FALSE;
	LPCWSTR pszUpdateVerLocationSet = mp_Set->UpdateVerLocation();
	wchar_t *pszUpdateVerLocation = NULL, *pszLocalPackage = NULL, *pszBatchFile = NULL;
	bool bTempUpdateVerLocation = false;
	wchar_t szSection[64], szItem[64];
	wchar_t szSourceFull[1024];
	wchar_t *pszSource, *pszEnd, *pszFileName;
	DWORD nSrcLen, nSrcCRC, nLocalCRC = 0;
	bool lbSourceLocal;
	INT_PTR nShellRc = 0;

#ifdef _DEBUG
	// Чтобы успел сервер проинититься и не ругался под отладчиком...
	if (!mb_ManualCallMode)
		Sleep(2500);
#endif

	_ASSERTE(m_UpdateStep==us_NotStarted);
	m_UpdateStep = us_Check;

	DeleteBadTempFiles();

	//120315 - OK, положим в архив и 64битный гуй
	//#ifdef _WIN64
	//if (mp_Set->UpdateDownloadSetup() == 2)
	//{
	//	if (mb_ManualCallMode)
	//	{
	//		ReportError(L"64bit versions of ConEmu may be updated with ConEmuSetup.exe only!", 0);
	//	}
	//	goto wrap;
	//}
	//#endif

	if (!wi)
	{
		wi = new CWinInet;
		if (!wi)
			ReportError(L"new CWinInet failed", 0);
	}
	if (!wi || !wi->Init(this))
	{
		goto wrap;
	}
	
	_wsprintf(ms_CurVersion, SKIPLEN(countof(ms_CurVersion)) L"%02u%02u%02u%s", (MVV_1%100),MVV_2,MVV_3,_T(MVV_4a));
	
	// Загрузить информацию о файлах обновления
	if (IsLocalFile(pszUpdateVerLocationSet))
	{
		pszUpdateVerLocation = (wchar_t*)pszUpdateVerLocationSet;
	}
	else
	{
		HANDLE hInfo = NULL;
		BOOL bInfoRc;
		DWORD crc;
		
		pszUpdateVerLocation = CreateTempFile(mp_Set->szUpdateDownloadPath/*L"%TEMP%"*/, L"ConEmuVersion.ini", hInfo);
		if (!pszUpdateVerLocation)
			goto wrap;
		bTempUpdateVerLocation = true;
		
		bInfoRc = DownloadFile(pszUpdateVerLocationSet, pszUpdateVerLocation, hInfo, crc);
		CloseHandle(hInfo);
		if (!bInfoRc)
		{
			if (!mb_ManualCallMode)
			{
				DeleteFile(pszUpdateVerLocation);
				SafeFree(pszUpdateVerLocation);
			}
			goto wrap;
		}
	}
	
	// Проверить версии
	_wcscpy_c(szSection, countof(szSection), (mp_Set->isUpdateUseBuilds==1) ? L"ConEmu_Stable" : L"ConEmu_Devel");
	_wcscpy_c(szItem, countof(szItem), (mp_Set->UpdateDownloadSetup()==1) ? L"location_exe" : L"location_arc");
	
	if (!GetPrivateProfileString(szSection, L"version", ms_CurVersion, ms_NewVersion, countof(ms_NewVersion), pszUpdateVerLocation))
	{
		ReportError(L"ProfileString(%s,version) failed, code=%u", szSection, GetLastError());
		goto wrap;
	}
	
	if (!GetPrivateProfileString(szSection, szItem, L"", szSourceFull, countof(szSourceFull), pszUpdateVerLocation) || !*szSourceFull)
	{
		ReportError(L"Version not available (%s,%s)", szSection, szItem, 0);
		goto wrap;
	}
	
	if ((lstrcmpi(ms_NewVersion, ms_CurVersion) <= 0)
		// Если пользователь отказался от обновления в этом сеансе - не предлагать ту же версию при ежечасных проверках
		|| (!mb_ManualCallMode && (lstrcmp(ms_NewVersion, ms_SkipVersion) == 0)))
	{
		// Новых версий нет
		if (mb_ManualCallMode)
		{
			ReportError(L"You are using latest available %s version (%s)", (mp_Set->isUpdateUseBuilds==1) ? L"stable" : L"developer", ms_CurVersion, 0);
		}

		if (bTempUpdateVerLocation && pszUpdateVerLocation && *pszUpdateVerLocation)
		{
			DeleteFile(pszUpdateVerLocation);
			SafeFree(pszUpdateVerLocation);
		}
		goto wrap;
	}

	pszSource = szSourceFull;
	nSrcLen = wcstoul(pszSource, &pszEnd, 10);
	if (!nSrcLen || !pszEnd || *pszEnd != L',' || *(pszEnd+1) != L'x')
	{
		ReportError(L"Invalid format in version description (size)\n%s", szSourceFull, 0);
		goto wrap;
	}
	pszSource = pszEnd+2;
	nSrcCRC = wcstoul(pszSource, &pszEnd, 16);
	if (!nSrcCRC || !pszEnd || *pszEnd != L',')
	{
		ReportError(L"Invalid format in version description (CRC32)\n%s", szSourceFull, 0);
		goto wrap;
	}
	pszSource = pszEnd+1;
	lbSourceLocal = IsLocalFile(pszSource);
	
	if (mb_RequestTerminate)
		goto wrap;

	if (!QueryConfirmation(us_ConfirmDownload, pszSource))
	{
		// Если пользователь отказался от обновления в этом сеансе - не предлагать ту же версию при ежечасных проверках
		wcscpy_c(ms_SkipVersion, ms_NewVersion);
		goto wrap;
	}

	mn_InternetContentReady = mn_InternetContentLen = 0;
	m_UpdateStep = us_Downloading;
	gpConEmu->UpdateProgress();

	pszFileName = wcsrchr(pszSource, lbSourceLocal ? L'\\' : L'/');
	if (!pszFileName)
	{
		ReportError(L"Invalid source url\n%s", szSourceFull, 0);
		goto wrap;
	}
	else
	{
		// Загрузить пакет обновления
		pszFileName++; // пропустить слеш
		
		HANDLE hTarget = NULL;
		
		pszLocalPackage = CreateTempFile(mp_Set->szUpdateDownloadPath, pszFileName, hTarget);
		if (!pszLocalPackage)
			goto wrap;
		
		lbDownloadRc = DownloadFile(pszSource, pszLocalPackage, hTarget, nLocalCRC, TRUE);
		if (lbDownloadRc)
		{
			wchar_t szInfo[2048];
			LARGE_INTEGER liSize = {};
			if (!GetFileSizeEx(hTarget, &liSize) || liSize.HighPart || liSize.LowPart != nSrcLen)
			{
				_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"%s\nRequired size=%u, local size=%u", pszSource, nSrcLen, liSize.LowPart);
				ReportError(L"Downloaded file does not match\n%s\n%s", pszLocalPackage, szInfo, 0);
				lbDownloadRc = FALSE;
			}
			else if (nLocalCRC != nSrcCRC)
			{
				_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"Required CRC32=x%08X, local CRC32=x%08X", nSrcCRC, nLocalCRC);
				ReportError(L"Invalid local file\n%s\n%s", pszLocalPackage, szInfo, 0);
				lbDownloadRc = FALSE;
			}
		}
		CloseHandle(hTarget);
		if (!lbDownloadRc)
			goto wrap;
	}
	
	if (wi)
	{
		delete wi;
		wi = NULL;
	}

	if (mb_RequestTerminate)
		goto wrap;

	pszBatchFile = CreateBatchFile(pszLocalPackage);
	if (!pszBatchFile)
		goto wrap;
	
	/*
	nShellRc = (INT_PTR)ShellExecute(ghWnd, bNeedRunElevation ? L"runas" : L"open", pszBatchFile, NULL, NULL, SW_SHOWMINIMIZED);
	if (nShellRc <= 32)
	{
		ReportError(L"Failed to start update batch\n%s\nError code=%i", pszBatchFile, (int)nShellRc);
		goto wrap;
	}
	*/
	if (!QueryConfirmation(us_ConfirmUpdate))
	{
		// Если пользователь отказался от обновления в этом сеансе - не предлагать ту же версию при ежечасных проверках
		wcscpy_c(ms_SkipVersion, ms_NewVersion);
		goto wrap;
	}
	mpsz_PendingPackageFile = pszLocalPackage;
	pszLocalPackage = NULL;
	mpsz_PendingBatchFile = pszBatchFile;
	pszBatchFile = NULL;
	m_UpdateStep = us_ExitAndUpdate;
	gpConEmu->RequestExitUpdate();
	lbExecuteRc = TRUE;

wrap:
	_ASSERTE(mpsz_DeleteIniFile==NULL);
	mpsz_DeleteIniFile = NULL;
	if (bTempUpdateVerLocation && pszUpdateVerLocation)
	{
		if (*pszUpdateVerLocation)
			mpsz_DeleteIniFile = pszUpdateVerLocation;
			//DeleteFile(pszUpdateVerLocation);
		else
			SafeFree(pszUpdateVerLocation);
	}

	_ASSERTE(mpsz_DeletePackageFile==NULL);
	mpsz_DeletePackageFile = NULL;
	if (pszLocalPackage)
	{
		if (*pszLocalPackage && (!lbDownloadRc || (!lbExecuteRc && !mp_Set->isUpdateLeavePackages)))
			mpsz_DeletePackageFile = pszLocalPackage;
			//DeleteFile(pszLocalPackage);
		else
			SafeFree(pszLocalPackage);
	}

	_ASSERTE(mpsz_DeleteBatchFile==NULL);
	mpsz_DeleteBatchFile = NULL;
	if (pszBatchFile)
	{
		if (*pszBatchFile && !lbExecuteRc)
			mpsz_DeleteBatchFile = pszBatchFile;
			//DeleteFile(pszBatchFile);
		else
			SafeFree(pszBatchFile);
	}

	if (!lbExecuteRc)
		m_UpdateStep = us_NotStarted;

	if (wi)
	{
		delete wi;
		wi = NULL;
	}
	
	mb_InCheckProcedure = FALSE;
	return 0;
}

wchar_t* CConEmuUpdate::CreateBatchFile(LPCWSTR asPackage)
{
	BOOL lbRc = FALSE;
	HANDLE hBatch = NULL;
	wchar_t* pszBatch;
	wchar_t* pszCommand = NULL;
	BOOL lbWrite;
	DWORD nLen, nWritten;
	char szOem[4096];
	LPCWSTR pszFormat = NULL;
	size_t cchCmdMax = 0;

	wchar_t szPID[16]; _wsprintf(szPID, SKIPLEN(countof(szPID)) L"%u", GetCurrentProcessId());
	wchar_t szCPU[4]; wcscpy_c(szCPU, WIN3264TEST(L"x86",L"x64"));
	WARNING("Битность установщика? Если ставим в ProgramFiles64 на Win64");
	
	pszBatch = CreateTempFile(mp_Set->szUpdateDownloadPath, L"ConEmuUpdate.cmd", hBatch);
	if (!pszBatch)
		goto wrap;
	
	#define WRITE_BATCH_A(s) \
		nLen = lstrlenA(s); \
		lbWrite = WriteFile(hBatch, s, nLen, &nWritten, NULL); \
		if (!lbWrite || (nLen != nWritten)) { ReportError(L"WriteBatch failed, code=%u", GetLastError()); goto wrap; }
	
	#define WRITE_BATCH_W(s) \
		nLen = WideCharToMultiByte(CP_OEMCP, 0, s, -1, szOem, countof(szOem), NULL, NULL); \
		if (!nLen) { ReportError(L"WideCharToMultiByte failed, len=%i", lstrlen(s)); goto wrap; } \
		WRITE_BATCH_A(szOem);
	
	WRITE_BATCH_A("@echo off\r\n");

	WRITE_BATCH_A("cd /d \"");
	WRITE_BATCH_W(gpConEmu->ms_ConEmuExeDir);
	WRITE_BATCH_A("\\\"\r\necho Current folder\r\ncd\r\necho .\r\n\r\necho Starting update...\r\n");
	
	// Формат.
	pszFormat = (mp_Set->UpdateDownloadSetup()==1) ? mp_Set->UpdateExeCmdLine() : mp_Set->UpdateArcCmdLine();
	
	// Замена %1 и т.п.
	for (int s = 0; s < 2; s++)
	{
		// На первом шаге - считаем требуемый размер под pszCommand, на втором - формируем команду
		if (s)
		{
			if (!cchCmdMax)
			{
				ReportError(L"Invalid %s update command (%s)", (mp_Set->UpdateDownloadSetup()==1) ? L"exe" : L"arc", pszFormat, 0);
				goto wrap;
			}
			pszCommand = (wchar_t*)malloc((cchCmdMax+1)*sizeof(wchar_t));
		}
		wchar_t* pDst = pszCommand;
		LPCWSTR pszMacro;
		for (LPCWSTR pSrc = pszFormat; *pSrc; pSrc++)
		{
			switch (*pSrc)
			{
			case L'%':
				pSrc++;
				switch (*pSrc)
				{
				case L'%':
					pszMacro = L"%";
					break;
				// "%1"-archive or setup file, "%2"-ConEmu.exe folder, "%3"-x86/x64, "%4"-ConEmu PID
				case L'1':
					pszMacro = asPackage;
					break;
				case L'2':
					pszMacro = gpConEmu->ms_ConEmuExeDir;
					break;
				case L'3':
					pszMacro = szCPU;
					break;
				case L'4':
					pszMacro = szPID;
					break;
				default:
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
					size_t cchLen = _tcslen(pszMacro);
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
				break;
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
	
	// Выполнить команду обновления
	WRITE_BATCH_A("echo ");
	WRITE_BATCH_W(pszCommand);
	WRITE_BATCH_A("\r\ncall ");
	WRITE_BATCH_W(pszCommand);
	WRITE_BATCH_A("\r\nif errorlevel 1 goto err\r\n");

	// Если юзер просил что-то выполнить после распаковки установки	
	if (mp_Set->szUpdatePostUpdateCmd && *mp_Set->szUpdatePostUpdateCmd)
	{
		WRITE_BATCH_A("\r\n");
		WRITE_BATCH_W(mp_Set->szUpdatePostUpdateCmd);
		WRITE_BATCH_A("\r\necho Starting ConEmu...\r\nstart \"ConEmu\" \"");
		WRITE_BATCH_W(gpConEmu->ms_ConEmuExe);
		WRITE_BATCH_A("\" ");
		if (gpConEmu->mpsz_ConEmuArgs)
		{
			WRITE_BATCH_W(gpConEmu->mpsz_ConEmuArgs);
		}
		WRITE_BATCH_A("\r\n");
	}
	
	// Сообщение об ошибке?
	WRITE_BATCH_A("\r\ngoto fin\r\n:err\r\n");
	WRITE_BATCH_A((mp_Set->UpdateDownloadSetup()==1) ? "echo \7Installation failed\7" : "echo \7Extraction failed\7\r\n");
	WRITE_BATCH_A("\r\npause\r\n:fin\r\n");

	// Грохнуть пакет обновления
	if (!mp_Set->isUpdateLeavePackages)
	{
		WRITE_BATCH_A("del \"");
		WRITE_BATCH_W(asPackage);
		WRITE_BATCH_A("\"\r\n");
	}

	// Грохнуть сам батч
	WRITE_BATCH_A("del \"%~0\"\r\n");

	//// Для отладки
	//WRITE_BATCH_A("\r\npause\r\n");
	
	// Succeeded
	lbRc = TRUE;
wrap:
	SafeFree(pszCommand);

	if (!lbRc)
	{
		SafeFree(pszBatch);
	}

	if (hBatch && hBatch != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hBatch);
	}
	return pszBatch;
}

CConEmuUpdate::UpdateStep CConEmuUpdate::InUpdate()
{
	DWORD nWait = WAIT_OBJECT_0;
	
	if (mh_CheckThread)
		nWait = WaitForSingleObject(mh_CheckThread, 0);

	switch (m_UpdateStep)
	{
	case us_Check:
	case us_ConfirmDownload:
	case us_ConfirmUpdate:
		if (nWait == WAIT_OBJECT_0)
			m_UpdateStep = us_NotStarted;
		break;
	case us_ExitAndUpdate:
		// Тут у нас нить уже имеет право завершиться
		break;
	default:
		;
	}
	
	return m_UpdateStep;
}

// В asDir могут быть переменные окружения.
wchar_t* CConEmuUpdate::CreateTempFile(LPCWSTR asDir, LPCWSTR asFileNameTempl, HANDLE& hFile)
{
	wchar_t szFile[MAX_PATH*2+2];
	wchar_t szName[128];
	
	if (!asDir || !*asDir)
		asDir = L"%TEMP%\\ConEmu";
	if (!asFileNameTempl || !*asFileNameTempl)
		asFileNameTempl = L"ConEmu.tmp";
	
	if (wcschr(asDir, L'%'))
	{
		DWORD nExp = ExpandEnvironmentStrings(asDir, szFile, MAX_PATH);
		if (!nExp || (nExp >= MAX_PATH))
		{
			ReportError(L"CreateTempFile.ExpandEnvironmentStrings(%s) failed, code=%u", asDir, GetLastError());
			return NULL;
		}
	}
	else
	{
		lstrcpyn(szFile, asDir, MAX_PATH);
	}

	// Checking %TEMP% for valid path
	LPCWSTR pszColon1, pszColon2;
	if ((pszColon1 = wcschr(szFile, L':')) != NULL)
	{
		if ((pszColon2 = wcschr(pszColon1+1, L':')) != NULL)
		{
			ReportError(L"Invalid download path (%%TEMP%% variable?)\n%s", szFile, 0);
			return NULL;
		}
	}
	
	int nLen = lstrlen(szFile);
	if (nLen <= 0)
	{
		ReportError(L"CreateTempFile.asDir(%s) failed, path is null", asDir, 0);
		return NULL;
	}
	if (szFile[nLen-1] != L'\\')
	{
		szFile[nLen++] = L'\\'; szFile[nLen] = 0;
	}
	wchar_t* pszFilePart = szFile + nLen;

	wchar_t* pszDirectory = lstrdup(szFile);
	
	
	LPCWSTR pszName = PointToName(asFileNameTempl);
	_ASSERTE(pszName == asFileNameTempl);
	if (!pszName || !*pszName || (*pszName == L'.'))
	{
		_ASSERTE(pszName && *pszName && (*pszName != L'.'));
		pszName = L"ConEmu";
	}
	
	LPCWSTR pszExt = PointToExt(pszName);
	if (pszExt == NULL)
	{
		_ASSERTE(pszExt != NULL);
		pszExt = L".tmp";
	}
	
	lstrcpyn(szName, pszName, countof(szName)-16);
	wchar_t* psz = wcsrchr(szName, L'.');
	if (psz)
		*psz = 0;
	
	wchar_t* pszResult = NULL;
	DWORD dwErr = 0;

	for (UINT i = 0; i < 9999; i++)
	{
		_wcscpy_c(pszFilePart, MAX_PATH, szName);
		if (i)
			_wsprintf(pszFilePart+_tcslen(pszFilePart), SKIPLEN(16) L"(%u)", i);
		_wcscat_c(pszFilePart, MAX_PATH, pszExt);
		
		hFile = CreateFile(szFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_TEMPORARY, NULL);
		//ERROR_PATH_NOT_FOUND?
		if (!hFile || (hFile == INVALID_HANDLE_VALUE))
		{
			dwErr = GetLastError();
			// на первом обломе - попытаться создать директорию, может ее просто нет?
			if ((dwErr == ERROR_PATH_NOT_FOUND) && (i == 0))
			{
				if (!MyCreateDirectory(pszDirectory))
				{
					ReportError(L"CreateTempFile.asDir(%s) failed", asDir, 0);
					goto wrap;
				}
			}
		}

		if (hFile && hFile != INVALID_HANDLE_VALUE)
		{
			psz = lstrdup(szFile);
			if (!psz)
			{
				CloseHandle(hFile); hFile = NULL;
				ReportError(L"Can't allocate memory (%i bytes)", lstrlen(szFile));
			}
			pszResult = psz;
			goto wrap;
		}
	}
	
	ReportError(L"Can't create temp file(%s), code=%u", szFile, dwErr);
	hFile = NULL;
wrap:
	SafeFree(pszDirectory);
	return pszResult;
}

bool CConEmuUpdate::IsLocalFile(LPCWSTR& asPathOrUrl)
{
	LPWSTR psz = (LPWSTR)asPathOrUrl;
	bool lbLocal = IsLocalFile(psz);
	asPathOrUrl = psz;
	return lbLocal;
}

bool CConEmuUpdate::IsLocalFile(LPWSTR& asPathOrUrl)
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

BOOL CConEmuUpdate::DownloadFile(LPCWSTR asSource, LPCWSTR asTarget, HANDLE hDstFile, DWORD& crc, BOOL abPackage /*= FALSE*/)
{
	BOOL lbRc = FALSE, lbRead = FALSE, lbWrite = FALSE;
	HANDLE hSrcFile = NULL;
	DWORD nRead;
	bool lbNeedTargetClose = false;
	bool lbInet = !IsLocalFile(asSource);
	bool lbTargetLocal = IsLocalFile(asTarget);
	_ASSERTE(lbTargetLocal);
	DWORD cchDataMax = 64*1024;
	BYTE* ptrData = (BYTE*)malloc(cchDataMax);
	//DWORD nTotalSize = 0;
	BOOL lbFirstThunk = TRUE;

	mn_InternetContentReady = 0;
	mn_InternetContentLen = 0;
	
	crc = 0xFFFFFFFF;
	
	if (!asSource || !*asSource || !asTarget || !*asTarget)
	{
		ReportError(L"DownloadFile. Invalid arguments", 0);
		goto wrap;
	}
	
	if (!ptrData)
	{
		ReportError(L"Failed to allocate memory (%u bytes)", cchDataMax);
		goto wrap;
	}

	if (hDstFile == NULL || hDstFile == INVALID_HANDLE_VALUE)
	{
		hDstFile = CreateFile(asTarget, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (!hDstFile || hDstFile == INVALID_HANDLE_VALUE)
		{
			ReportError(L"Failed to create target file(%s), code=%u", asTarget, GetLastError());
			goto wrap;
		}
		lbNeedTargetClose = true;
	}
	
	if (lbInet)
	{
		DWORD ProxyType = INTERNET_OPEN_TYPE_DIRECT;
		LPCWSTR ProxyName = NULL;
		if (mp_Set->isUpdateUseProxy)
		{
			if (mp_Set->szUpdateProxy && *mp_Set->szUpdateProxy)
			{
				ProxyType = INTERNET_OPEN_TYPE_PROXY;
				ProxyName = mp_Set->szUpdateProxy;
			}
			else
			{
				ProxyType = INTERNET_OPEN_TYPE_PRECONFIG;
			}
		}

		wchar_t szServer[MAX_PATH], szSrvPath[MAX_PATH*2];
		if (wmemcmp(asSource, L"http://", 7) != 0)
		{
			ReportError(L"Only http addresses are supported!\n%s", asSource, 0);
			goto wrap;
		}
		LPCWSTR pszSlash = wcschr(asSource+7, L'/');
		if (!pszSlash || (pszSlash == (asSource+7)) || ((pszSlash - (asSource+7)) >= countof(szServer)))
		{
			ReportError(L"Invalid server specified!\n%s", asSource, 0);
			goto wrap;
		}
		lstrcpyn(szServer, asSource+7, (pszSlash - (asSource+6)));
		if (!*(pszSlash+1))
		{
			ReportError(L"Invalid server path specified!\n%s", asSource, 0);
			goto wrap;
		}
		lstrcpyn(szSrvPath, pszSlash, countof(szSrvPath));

		if (mb_RequestTerminate)
			goto wrap;

		// Открыть WinInet, если этого еще не сделали
		if (mh_Internet == NULL)
		{
			mh_Internet = wi->_InternetOpenW(TEXT("Mozilla/5.0 (compatible; ConEmu Update)"), ProxyType, ProxyName, NULL, 0);
			if (!mh_Internet)
			{
				DWORD dwErr = GetLastError();
				ReportError(L"Network initialization failed, code=%u", dwErr);
				goto wrap;
			}
		}

		if (mb_RequestTerminate)
			goto wrap;

		if (mh_Connect != NULL)
		{
			wi->_InternetCloseHandle(mh_Connect);
			mh_Connect = NULL;
		}
		
		//TODO после включения ноута вылезла ошибка ERROR_INTERNET_NAME_NOT_RESOLVED==12007

		if (mh_Connect == NULL)
		{
			mh_Connect = wi->_InternetConnectW(mh_Internet, szServer, INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 1);
			if (!mh_Connect)
			{
				DWORD dwErr = GetLastError();
				if (mb_ManualCallMode || abPackage)
					ReportError(L"Connection failed, code=%u", dwErr);
				goto wrap;
			}

			if (ProxyName)
			{
				if (mp_Set->szUpdateProxyUser && *mp_Set->szUpdateProxyUser)
				{
					if (!wi->_InternetSetOptionW(mh_Connect, INTERNET_OPTION_PROXY_USERNAME, (LPVOID)mp_Set->szUpdateProxyUser, lstrlen(mp_Set->szUpdateProxyUser)))
					{
						ReportError(L"ProxyUserName failed, code=%u", GetLastError());
						goto wrap;
					}
				}
				if (mp_Set->szUpdateProxyPassword && *mp_Set->szUpdateProxyPassword)
				{
					if (!wi->_InternetSetOptionW(mh_Connect, INTERNET_OPTION_PROXY_PASSWORD, (LPVOID)mp_Set->szUpdateProxyPassword, lstrlen(mp_Set->szUpdateProxyPassword)))
					{
						ReportError(L"ProxyPassword failed, code=%u", GetLastError());
						goto wrap;
					}
				}
			}

			HTTP_VERSION_INFO httpver={1,1};
			if (!wi->_InternetSetOptionW(mh_Connect, INTERNET_OPTION_HTTP_VERSION, &httpver, sizeof(httpver)))
			{
				ReportError(L"HttpVersion failed, code=%u", GetLastError());
				goto wrap;
			}

			TODO("INTERNET_OPTION_RECEIVE_TIMEOUT - Sets or retrieves an unsigned long integer value that contains the time-out value, in milliseconds");

		} // if (mh_Connect == NULL)

		if (mb_RequestTerminate)
			goto wrap;

		// Отправить запрос на файл
		hSrcFile = wi->_HttpOpenRequestW(mh_Connect, L"GET", szSrvPath, L"HTTP/1.1", NULL, 0,
			INTERNET_FLAG_KEEP_CONNECTION|INTERNET_FLAG_NO_CACHE_WRITE|INTERNET_FLAG_PRAGMA_NOCACHE|INTERNET_FLAG_RELOAD, 1);
		if (!hSrcFile)
		{
			DWORD dwErr = GetLastError();
			if (mb_ManualCallMode || abPackage)
			{
				// In offline mode, HttpSendRequest returns ERROR_FILE_NOT_FOUND if the resource is not found in the Internet cache.
				ReportError(
					(dwErr == 2)
					? L"HttpOpenRequest failed\nURL=%s\ncode=%u, Internet is offline?"
					: L"HttpOpenRequest failed\nURL=%s\ncode=%u"
					, asSource, dwErr);
			}
			goto wrap;
		}

		if (!wi->_HttpSendRequestW(hSrcFile,NULL,0,NULL,0))
		{
			DWORD dwErr = GetLastError();
			if (mb_ManualCallMode || abPackage)
				ReportError(L"HttpSendRequest failed\nURL=%s\ncode=%u", asSource, dwErr);
			goto wrap;
		}
		else
		{
			mn_InternetContentLen = 0;
			DWORD sz = sizeof(mn_InternetContentLen);
			DWORD dwIndex = 0;
			if (!wi->_HttpQueryInfoW(hSrcFile, HTTP_QUERY_CONTENT_LENGTH|HTTP_QUERY_FLAG_NUMBER, &mn_InternetContentLen, &sz, &dwIndex))
			{
				DWORD dwErr = GetLastError();
				// были ошибки: ERROR_HTTP_HEADER_NOT_FOUND
				if (mb_ManualCallMode || abPackage)
					ReportError(L"QueryContentLen failed\nURL=%s\ncode=%u", asSource, dwErr);
				goto wrap;
			}
		}
	}
	else
	{
		hSrcFile = CreateFile(asSource, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (!hSrcFile || hSrcFile == INVALID_HANDLE_VALUE)
		{
			ReportError(L"Failed to open source file(%s), code=%u", asSource, GetLastError());
			goto wrap;
		}
		LARGE_INTEGER liSize;
		if (GetFileSizeEx(hSrcFile, &liSize))
			mn_InternetContentLen = liSize.LowPart;
	}
	
	while (TRUE)
	{
		if (mb_RequestTerminate)
			goto wrap;

		lbRead = ReadSource(asSource, lbInet, hSrcFile, ptrData, cchDataMax, &nRead);
		if (!lbRead)
			goto wrap;
			
		if (!nRead)
		{
			if (!mn_InternetContentReady)
			{
				ReportError(L"Invalid source file (%s), file is empty", asSource, 0);
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
				if (mb_ManualCallMode)
				{
					ReportError(L"Remote file not found\nURL: %s\nLocal: %s", asSource, asTarget, 0);
				}
				goto wrap;
			}
		}

		mn_InternetContentReady += nRead;
		gpConEmu->UpdateProgress();
	}
	
	// Succeeded
	crc ^= 0xFFFFFFFF;
	lbRc = TRUE;
wrap:
	if (hSrcFile && hSrcFile != INVALID_HANDLE_VALUE)
	{
		if (lbInet)
		{
			wi->_InternetCloseHandle(hSrcFile);
		}
		else
		{
			CloseHandle(hSrcFile);
		}
	}

	if (mh_Connect)
	{
		wi->_InternetCloseHandle(mh_Connect);
		mh_Connect = NULL;
	}

	if (mh_Internet)
	{
		wi->_InternetCloseHandle(mh_Internet);
		mh_Internet = NULL;
	}

	if (lbNeedTargetClose && hDstFile && hDstFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hDstFile);
	}

	SafeFree(ptrData);
	return lbRc;
}

BOOL CConEmuUpdate::ReadSource(LPCWSTR asSource, BOOL bInet, HANDLE hSource, BYTE* pData, DWORD cbData, DWORD* pcbRead)
{
	BOOL lbRc = FALSE;
	
	if (bInet)
	{
		lbRc = wi->_InternetReadFile(hSource, pData, cbData, pcbRead);
		if (!lbRc)
			ReportError(L"DownloadFile(%s) failed, code=%u", asSource, GetLastError());
	}
	else
	{
		lbRc = ReadFile(hSource, pData, cbData, pcbRead, NULL);
		if (!lbRc)
			ReportError(L"ReadFile(%s) failed, code=%u", asSource, GetLastError());
	}
	
	return lbRc;
}

BOOL CConEmuUpdate::WriteTarget(LPCWSTR asTarget, HANDLE hTarget, const BYTE* pData, DWORD cbData)
{
	BOOL lbRc;
	DWORD nWritten;
	
	lbRc = WriteFile(hTarget, pData, cbData, &nWritten, NULL);
	
	if (lbRc && (nWritten != cbData))
	{
		lbRc = FALSE;
		ReportError(L"WriteFile(%s) failed, no data, code=%u", asTarget, GetLastError());
	}
	else if (!lbRc)
	{
		ReportError(L"WriteFile(%s) failed, code=%u", asTarget, GetLastError());
	}
	
	return lbRc;
}

void CConEmuUpdate::ReportErrorInt(wchar_t* asErrorInfo)
{
	if (mb_InShowLastError)
		return; // Две ошибки сразу не показываем, а то зациклимся

	MSectionLock SC; SC.Lock(mp_LastErrorSC, TRUE);
	SafeFree(ms_LastErrorInfo);
	ms_LastErrorInfo = asErrorInfo;
	SC.Unlock();

	gpConEmu->ReportUpdateError();
}

void CConEmuUpdate::ReportError(LPCWSTR asFormat, DWORD nErrCode)
{
	if (!asFormat)
		return;
	
	size_t cchMax = _tcslen(asFormat) + 32;
	wchar_t* pszErrInfo = (wchar_t*)malloc(cchMax*sizeof(wchar_t));
	if (pszErrInfo)
	{
		_wsprintf(pszErrInfo, SKIPLEN(cchMax) asFormat, nErrCode);
		ReportErrorInt(pszErrInfo);
	}
}

void CConEmuUpdate::ReportError(LPCWSTR asFormat, LPCWSTR asArg, DWORD nErrCode)
{
	if (!asFormat || !asArg)
		return;
	
	size_t cchMax = _tcslen(asFormat) + _tcslen(asArg) + 32;
	wchar_t* pszErrInfo = (wchar_t*)malloc(cchMax*sizeof(wchar_t));
	if (pszErrInfo)
	{
		_wsprintf(pszErrInfo, SKIPLEN(cchMax) asFormat, asArg, nErrCode);
		ReportErrorInt(pszErrInfo);	
	}
}

void CConEmuUpdate::ReportError(LPCWSTR asFormat, LPCWSTR asArg1, LPCWSTR asArg2, DWORD nErrCode)
{
	if (!asFormat || !asArg1 || !asArg2)
		return;
	
	size_t cchMax = _tcslen(asFormat) + _tcslen(asArg1) + _tcslen(asArg2) + 32;
	wchar_t* pszErrInfo = (wchar_t*)malloc(cchMax*sizeof(wchar_t));
	if (pszErrInfo)
	{
		_wsprintf(pszErrInfo, SKIPLEN(cchMax) asFormat, asArg1, asArg2, nErrCode);
		ReportErrorInt(pszErrInfo);
	}
}

bool CConEmuUpdate::NeedRunElevation()
{
	//TODO: В каких случаях нужен "runas"
	//TODO: Vista+: (если сейчас НЕ "Admin") && (установка в %ProgramFiles%)
	//TODO: WinXP-: (установка в %ProgramFiles%) && (нет доступа в %ProgramFiles%)

	DWORD dwErr = 0;
	wchar_t szTestFile[MAX_PATH+20];
	wcscpy_c(szTestFile, gpConEmu->ms_ConEmuExeDir);
	wcscat_c(szTestFile, L"\\");
	
	if (gOSVer.dwMajorVersion >= 6)
	{
		if (IsUserAdmin())
			return false; // Уже под админом (Vista+)
		// куда мы установлены? Если НЕ в %ProgramFiles%, то для распаковки совсем не нужно Elevation требовать
		int nFolderIdl[] = {
			CSIDL_PROGRAM_FILES,
			CSIDL_PROGRAM_FILES_COMMON,
			#ifdef _WIN64
			CSIDL_PROGRAM_FILESX86,
			CSIDL_PROGRAM_FILES_COMMONX86,
			#endif
		};
		size_t nLen;
		wchar_t szSystem[MAX_PATH+2];
		for (size_t i = 0; i < countof(nFolderIdl); i++)
		{
			for (size_t j = 0; j <= 1; j++)
			{
				if ((S_OK == SHGetFolderPath(NULL, nFolderIdl[i], NULL, j ? SHGFP_TYPE_DEFAULT : SHGFP_TYPE_CURRENT, szSystem))
					&& (nLen = _tcslen(szSystem)))
				{
					if (szSystem[nLen-1] != L'\\')
					{
						szSystem[nLen++] = L'\\'; szSystem[nLen] = 0;
					}
					if (lstrcmpi(szTestFile, szSystem) == 0)
						return true; // Установлены в ProgramFiles
				}
			}
		}
		// Скорее всего не надо
		return false;
	}

	// XP и ниже
	wcscat_c(szTestFile, L"ConEmuUpdate.flag");
	HANDLE hFile = CreateFile(szTestFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		dwErr = GetLastError();
		return true;
	}
	CloseHandle(hFile);
	DeleteFile(szTestFile);

	// RunAs не нужен
	return false;
}

void CConEmuUpdate::DeleteBadTempFiles()
{
	if (mpsz_DeleteIniFile)
	{
		DeleteFile(mpsz_DeleteIniFile);
		SafeFree(mpsz_DeleteIniFile);
	}
	if (mpsz_DeletePackageFile)
	{
		DeleteFile(mpsz_DeletePackageFile);
		SafeFree(mpsz_DeletePackageFile);
	}
	if (mpsz_DeleteBatchFile)
	{
		DeleteFile(mpsz_DeleteBatchFile);
		SafeFree(mpsz_DeleteBatchFile);
	}
}

bool CConEmuUpdate::QueryConfirmation(CConEmuUpdate::UpdateStep step, LPCWSTR asParm)
{
	if (mb_RequestTerminate)
	{
		return false;
	}

	bool lbRc = false;
	wchar_t* pszMsg = NULL;
	size_t cchMax;

	switch (step)
	{
	case us_ConfirmDownload:
		{
			cchMax = _tcslen(asParm)+255;
			pszMsg = (wchar_t*)malloc(cchMax*sizeof(*pszMsg));

			if (mb_ManualCallMode == 2)
			{
				lbRc = true;
			}
			else if (mp_Set->isUpdateConfirmDownload || mb_ManualCallMode)
			{
				wchar_t* pszDup = lstrdup(asParm);
				wchar_t* pszFile = pszDup ? wcsrchr(pszDup, L'/') : NULL;
				if (pszFile)
				{
					pszFile[1] = 0;
					pszFile = (wchar_t*)(asParm + (pszFile - pszDup + 1));
					asParm = pszDup;
				}

				_wsprintf(pszMsg, SKIPLEN(cchMax) L"New %s version available: %s\n\n%s\n%s\n\nDownload?",
					(mp_Set->isUpdateUseBuilds==1) ? L"stable" : L"developer",
					ms_NewVersion, asParm ? asParm : L"", pszFile ? pszFile : L"");
				SafeFree(pszDup);

				m_UpdateStep = step;
				lbRc = QueryConfirmationInt(pszMsg);
			}
			else
			{
				_wsprintf(pszMsg, SKIPLEN(cchMax) L"New %s version available: %s\nClick here to download",
					(mp_Set->isUpdateUseBuilds==1) ? L"stable" : L"developer",
					ms_NewVersion);
				Icon.ShowTrayIcon(pszMsg, tsa_Source_Updater);

				lbRc = false;
			}
		}
		break;
	case us_ConfirmUpdate:
		cchMax = 512;
		pszMsg = (wchar_t*)malloc(cchMax*sizeof(*pszMsg));
		_wsprintf(pszMsg, SKIPLEN(cchMax)
			L"Do you want to close ConEmu and\n"
			L"update to new %s version %s?",
			(mp_Set->isUpdateUseBuilds==1) ? L"stable" : L"developer", ms_NewVersion);
		m_UpdateStep = step;
		lbRc = QueryConfirmationInt(pszMsg);
		break;
	default:
		_ASSERTE(step==us_ConfirmDownload);
		lbRc = false;
	}

	return lbRc;
}

bool CConEmuUpdate::QueryConfirmationInt(LPCWSTR asConfirmInfo)
{
	if (mb_InShowLastError)
		return false; // Если отображается ошибк - не звать

	MSectionLock SC; SC.Lock(mp_LastErrorSC, TRUE);
	SafeFree(ms_LastErrorInfo);
	ms_LastErrorInfo = lstrdup(asConfirmInfo);
	SC.Unlock();

	bool bRc = gpConEmu->ReportUpdateConfirmation();
	return bRc;
}

short CConEmuUpdate::GetUpdateProgress()
{
	if (mb_RequestTerminate)
		return -1;

	switch (InUpdate())
	{
	case us_NotStarted:
		return -1;
	case us_Check:
		return mb_ManualCallMode ? 1 : -1;
	case us_ConfirmDownload:
		return 10;
	case us_Downloading:
		if (mn_InternetContentLen > 0)
		{
			int nValue = (mn_InternetContentReady * 88 / mn_InternetContentLen);
			if (nValue < 0)
				nValue = 0;
			else if (nValue > 88)
				nValue = 88;
			return nValue+10;
		}
		return 10;
	case us_ConfirmUpdate:
		return 98;
	case us_ExitAndUpdate:
		return 99;
	}

	return -1;
}

void CConEmuUpdate::WaitAllInstances()
{
	while (true)
	{
		bool bStillExists = false;
		wchar_t szMessage[255];
		wcscpy_c(szMessage, L"Please, close all ConEmu instances before continue");

		HWND hFind = FindWindowEx(NULL, NULL, VirtualConsoleClassMain, NULL);
		if (hFind)
		{
			bStillExists = true;
			DWORD nPID; GetWindowThreadProcessId(hFind, &nPID);
			_wsprintf(szMessage+_tcslen(szMessage), SKIPLEN(64) L"\nConEmu still running, PID=%u", nPID);
		}

		if (!bStillExists)
		{
			TODO("Можно бы проехаться по всем модулям запущенных процессов на предмет блокировки файлов в папках ConEmu");
		}

		// Если никого запущенного не нашли - выходим из цикла
		if (!bStillExists)
			return;

		// Ругнуться
		int nBtn = MessageBox(NULL, szMessage, ms_DefaultTitle, MB_ICONEXCLAMATION|MB_OKCANCEL);
		if (nBtn == IDCANCEL)
			return; // "Cancel" - means stop checking
	}
}
