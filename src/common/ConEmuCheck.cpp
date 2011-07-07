
/*
Copyright (c) 2009-2011 Maximus5
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


#include <windows.h>
#include "MAssert.h"
#include "common.hpp"
#include "Memory.h"
#include "ConEmuCheck.h"
#include "WinObjects.h"

// !!! Использовать только через функцию: LocalSecurity() !!!
SECURITY_ATTRIBUTES* gpLocalSecurity = NULL;
u64 ghWorkingModule = 0;

//#ifdef _DEBUG
//	#include <crtdbg.h>
//#else
//	#ifndef _ASSERT
//	#define _ASSERT()
//	#endif
//
//	#ifndef _ASSERTE
//	#define _ASSERTE(f)
//	#endif
//#endif

//extern LPVOID _calloc(size_t,size_t);
//extern LPVOID _malloc(size_t);
//extern void   _free(LPVOID);

typedef HWND (APIENTRY *FGetConsoleWindow)();
//typedef DWORD (APIENTRY *FGetConsoleProcessList)(LPDWORD,DWORD);

//WARNING("Для 'Простых' запросов можно использовать 'CallNamedPipe', Это если нужно например получить хэндлы окон");


//#if defined(__GNUC__)
//extern "C" {
//#endif
//	WINBASEAPI DWORD GetEnvironmentVariableW(LPCWSTR lpName,LPWSTR lpBuffer,DWORD nSize);
//#if defined(__GNUC__)
//};
//#endif

//// Это для CONEMU_MINIMAL, чтобы wsprintf не был нужен
//LPCWSTR CreatePipeName(wchar_t (&szGuiPipeName)[128], LPCWSTR asFormat, DWORD anValue)
//{
//	return msprintf(szGuiPipeName, countof(szGuiPipeName), asFormat, L".", anValue);
//	//szGuiPipeName[0] = 0;
//
//	//const wchar_t* pszSrc = asFormat;
//	//wchar_t* pszDst = szGuiPipeName;
//
//	//while (*pszSrc)
//	//{
//	//	if (*pszSrc == L'%')
//	//	{
//	//		pszSrc++;
//	//		switch (*pszSrc)
//	//		{
//	//		case L's':
//	//			pszSrc++;
//	//			*(pszDst++) = L'.';
//	//			continue;
//	//		case L'u':
//	//			{
//	//				pszSrc++;
//	//				wchar_t szValueDec[16] = {};
//	//				DWORD nValue = anValue, i = 0;
//	//				wchar_t* pszValue = szValueDec;
//	//				while (nValue)
//	//				{
//	//					WORD n = (WORD)(nValue % 10);
//	//					*(pszValue++) = (wchar_t)(L'0' + n);
//	//					nValue = (nValue - n) / 10;
//	//				}
//	//				if (pszValue == szValueDec)
//	//					*(pszValue++) = L'0';
//	//				// Теперь перекинуть в szGuiPipeName
//	//				while (pszValue > szValueDec)
//	//				{
//	//					*(pszDst++) = *(pszValue--);
//	//				}
//	//				continue;
//	//			}
//	//		case L'0':
//	//			if (pszSrc[1] == L'8' && pszSrc[2] == L'X')
//	//			{
//	//				pszSrc += 3;
//	//				wchar_t szValueHex[16] = L"00000000";
//	//				DWORD nValue = anValue, i = 0;
//	//				wchar_t* pszValue = szValueHex;
//	//				while (nValue)
//	//				{
//	//					WORD n = (WORD)(nValue & 0xF);
//	//					if (n <= 9)
//	//						*(pszValue++) = (wchar_t)(L'0' + n);
//	//					else
//	//						*(pszValue++) = (wchar_t)(L'A' + n - 10);
//	//					nValue = nValue >> 4;
//	//				}
//	//				// Теперь перекинуть в szGuiPipeName
//	//				for (pszValue = (szValueHex+7); pszValue >= szValueHex; pszValue--)
//	//				{
//	//					*(pszDst++) = *pszValue;
//	//				}
//	//				continue;
//	//			}
//	//		default:
//	//			_ASSERTE(*pszSrc == L'u' || *pszSrc == L's');
//	//		}
//	//	}
//	//	else
//	//	{
//	//		*(pszDst++) = *(pszSrc++);
//	//	}
//	//}
//	//*pszDst = 0;
//
//	//_ASSERTE(lstrlen(szGuiPipeName) < countof(szGuiPipeName));
//
//	//return szGuiPipeName;
//}

LPCWSTR ModuleName(LPCWSTR asDefault)
{
	if (asDefault && *asDefault)
		return asDefault;

	static wchar_t szFile[32];

	if (szFile[0])
		return szFile;

	wchar_t szPath[MAX_PATH*2];

	if (GetModuleFileNameW(NULL, szPath, countof(szPath)))
	{
		wchar_t *pszSlash = wcsrchr(szPath, L'\\');

		if (pszSlash)
			pszSlash++;
		else
			pszSlash = szPath;

		lstrcpynW(szFile, pszSlash, 32);
	}

	if (szFile[0] == 0)
	{
		wcscpy_c(szFile, L"Unknown");
	}

	return szFile;
}

HANDLE ExecuteOpenPipe(const wchar_t* szPipeName, wchar_t (&szErr)[MAX_PATH*2], const wchar_t* szModule)
{
	HANDLE hPipe = NULL;
	DWORD dwErr = 0, dwMode = 0;
	BOOL fSuccess = FALSE;
	DWORD dwStartTick = GetTickCount();
	int nTries = 2;
	_ASSERTE(LocalSecurity()!=NULL);

	// Try to open a named pipe; wait for it, if necessary.
	while(1)
	{
		hPipe = CreateFile(
		            szPipeName,     // pipe name
		            GENERIC_READ|GENERIC_WRITE,
		            0,              // no sharing
		            LocalSecurity(), // default security attributes
		            OPEN_EXISTING,  // opens existing pipe
		            0,              // default attributes
		            NULL);          // no template file

		// Break if the pipe handle is valid.
		if (hPipe != INVALID_HANDLE_VALUE)
			break; // OK, открыли

		dwErr = GetLastError();

		// Сделаем так, чтобы хотя бы пару раз он попробовал повторить
		if ((nTries <= 0) && (GetTickCount() - dwStartTick) > 1000)
		{
			//if (pszErr)
			{
				msprintf(szErr, countof(szErr), L"%s.%u: CreateFile(%s) failed, code=0x%08X, Timeout",
				          ModuleName(szModule), GetCurrentProcessId(), szPipeName, dwErr);
			}
			return NULL;
		}
		else
		{
			nTries--;
		}

		// Может быть пайп еще не создан (в процессе срабатывания семафора)
		if (dwErr == ERROR_FILE_NOT_FOUND)
		{
			Sleep(10);
			continue;
		}

		// Exit if an error other than ERROR_PIPE_BUSY occurs.
		if (dwErr != ERROR_PIPE_BUSY)
		{
			//if (pszErr)
			{
				msprintf(szErr, countof(szErr), L"%s.%u: CreateFile(%s) failed, code=0x%08X",
				          ModuleName(szModule), GetCurrentProcessId(), szPipeName, dwErr);
			}
			return NULL;
		}

		// All pipe instances are busy, so wait for 500 ms.
		WaitNamedPipe(szPipeName, 500);
		//if (!WaitNamedPipe(szPipeName, 1000) )
		//{
		//	dwErr = GetLastError();
		//	if (pszErr)
		//	{
		//		StringCchPrintf(pszErr, countof(pszErr), L"%s: WaitNamedPipe(%s) failed, code=0x%08X, WaitNamedPipe",
		//			szModule ? szModule : L"Unknown", szPipeName, dwErr);
		//		// Видимо это возникает в момент запуска (обычно для ShiftEnter - новая консоль)
		//		// не сразу срабатывает GUI и RCon еще не создал Pipe для HWND консоли
		//		_ASSERTE(dwErr == 0);
		//	}
		//    return NULL;
		//}
	}

	// The pipe connected; change to message-read mode.
	dwMode = PIPE_READMODE_MESSAGE;
	fSuccess = SetNamedPipeHandleState(
	               hPipe,    // pipe handle
	               &dwMode,  // new pipe mode
	               NULL,     // don't set maximum bytes
	               NULL);    // don't set maximum time

	if (!fSuccess)
	{
		dwErr = GetLastError();
		_ASSERT(fSuccess);
		//if (pszErr)
		{
			msprintf(szErr, countof(szErr), L"%s.%u: SetNamedPipeHandleState(%s) failed, code=0x%08X",
			          ModuleName(szModule), GetCurrentProcessId(), szPipeName, dwErr);
		}
		CloseHandle(hPipe);
		return NULL;
	}

	return hPipe;
}


void ExecutePrepareCmd(CESERVER_REQ* pIn, DWORD nCmd, DWORD cbSize)
{
	if (!pIn)
		return;

	ExecutePrepareCmd(&(pIn->hdr), nCmd, cbSize);
}

void ExecutePrepareCmd(CESERVER_REQ_HDR* pHdr, DWORD nCmd, DWORD cbSize)
{
	if (!pHdr)
		return;

	pHdr->nCmd = nCmd;
	pHdr->nSrcThreadId = GetCurrentThreadId();
	pHdr->nSrcPID = GetCurrentProcessId();
	pHdr->cbSize = cbSize;
	pHdr->nVersion = CESERVER_REQ_VER;
	pHdr->nCreateTick = GetTickCount();
	_ASSERTE(ghWorkingModule!=0);
	pHdr->hModule = ghWorkingModule;
#ifdef _WIN64
	pHdr->nBits = 64;
#else
	pHdr->nBits = 32;
#endif
	pHdr->nReserved = 0;
}

CESERVER_REQ* ExecuteNewCmd(DWORD nCmd, DWORD nSize)
{
	CESERVER_REQ* pIn = NULL;

	if (nSize)
	{
		// Обязательно с обнулением выделяемой памяти
		pIn = (CESERVER_REQ*)calloc(nSize, 1);

		if (pIn)
		{
			ExecutePrepareCmd(pIn, nCmd, nSize);
		}
	}

	return pIn;
}

BOOL LoadSrvMapping(HWND hConWnd, CESERVER_CONSOLE_MAPPING_HDR& SrvMapping)
{
	if (!hConWnd)
		return FALSE;

	MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> SrvInfoMapping;
	SrvInfoMapping.InitName(CECONMAPNAME, (DWORD)hConWnd);
	const CESERVER_CONSOLE_MAPPING_HDR* pInfo = SrvInfoMapping.Open();
	if (!pInfo)
		return FALSE;
	else if (pInfo->nProtocolVersion != CESERVER_REQ_VER)
		return FALSE;
	else
	{
		memmove(&SrvMapping, pInfo, min(pInfo->cbSize, sizeof(SrvMapping)));
		/*bDosBoxAllowed = pInfo->bDosBox;
		wcscpy_c(szBaseDir, pInfo->sConEmuBaseDir);
		wcscat_c(szBaseDir, L"\\");
		if (pInfo->nLoggingType != glt_Processes)
			return NULL;*/
	}
	SrvInfoMapping.CloseMap();

	return (SrvMapping.cbSize != 0);
}

#ifndef CONEMU_MINIMAL
BOOL LoadGuiMapping(HWND hConEmuWnd, ConEmuGuiMapping& GuiMapping)
{
	//if (m_GuiMapping.cbSize)
	//	return TRUE;

	// Проверим, а можно ли?
	DWORD dwGuiProcessId = 0;
	if (!hConEmuWnd || !GetWindowThreadProcessId(hConEmuWnd, &dwGuiProcessId))
		return FALSE;

	return LoadGuiMapping(dwGuiProcessId, GuiMapping);
}

BOOL LoadGuiMapping(DWORD nConEmuPID, ConEmuGuiMapping& GuiMapping)
{
	if (!nConEmuPID)
		return FALSE;

	MFileMapping<ConEmuGuiMapping> GuiInfoMapping;
	GuiInfoMapping.InitName(CEGUIINFOMAPNAME, nConEmuPID);
	const ConEmuGuiMapping* pInfo = GuiInfoMapping.Open();
	if (!pInfo)
		return FALSE;
	else if (pInfo->nProtocolVersion != CESERVER_REQ_VER)
		return FALSE;
	else
	{
		memmove(&GuiMapping, pInfo, min(pInfo->cbSize, sizeof(GuiMapping)));
		/*bDosBoxAllowed = pInfo->bDosBox;
		wcscpy_c(szBaseDir, pInfo->sConEmuBaseDir);
		wcscat_c(szBaseDir, L"\\");
		if (pInfo->nLoggingType != glt_Processes)
			return NULL;*/
	}
	GuiInfoMapping.CloseMap();

	return (GuiMapping.cbSize != 0);
}
#endif

CESERVER_REQ* ExecuteNewCmdOnCreate(enum CmdOnCreateType aCmd,
				LPCWSTR asAction, LPCWSTR asFile, LPCWSTR asParam,
				DWORD* anShellFlags, DWORD* anCreateFlags, DWORD* anStartFlags, DWORD* anShowCmd,
				int mn_ImageBits, int mn_ImageSubsystem,
				HANDLE hStdIn, HANDLE hStdOut, HANDLE hStdErr)
{
	//szBaseDir[0] = 0;

	//// Проверим, а надо ли?
	//MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> ConMap;
	//ConMap.InitName(CECONMAPNAME, (DWORD)FarHwnd);
	//CESERVER_CONSOLE_MAPPING_HDR* p = ConMap.Open();
	//if (p && p->hConEmuRoot && isWindow(p->hConEmuRoot))
	//{

	////	bDosBoxAllowed = pInfo->bDosBox;
	////	wcscpy_c(szBaseDir, pInfo->sConEmuBaseDir);
	////	wcscat_c(szBaseDir, L"\\");
	//if (p->nLoggingType != glt_Processes)
	//	return NULL;

	//DWORD dwGuiProcessId = 0;
	//if (!ghConEmuWnd || !GetWindowThreadProcessId(ghConEmuWnd, &dwGuiProcessId))
	//	return NULL;

	//MFileMapping<ConEmuGuiMapping> GuiInfoMapping;
	//GuiInfoMapping.InitName(CEGUIINFOMAPNAME, dwGuiProcessId);
	//const ConEmuGuiMapping* pInfo = GuiInfoMapping.Open();
	//if (!pInfo)
	//	return NULL;
	//else if (pInfo->nProtocolVersion != CESERVER_REQ_VER)
	//	return NULL;
	//else
	//{
	//	bDosBoxAllowed = pInfo->bDosBox;
	//	wcscpy_c(szBaseDir, pInfo->sConEmuBaseDir);
	//	wcscat_c(szBaseDir, L"\\");
	//	if (pInfo->nLoggingType != glt_Processes)
	//		return NULL;
	//}
	//GuiInfoMapping.CloseMap();

	
	CESERVER_REQ *pIn = NULL;
	
	int nActionLen = (asAction ? lstrlen(asAction) : 0)+1;
	int nFileLen = (asFile ? lstrlen(asFile) : 0)+1;
	int nParamLen = (asParam ? lstrlen(asParam) : 0)+1;
	
	pIn = ExecuteNewCmd(CECMD_ONCREATEPROC, sizeof(CESERVER_REQ_HDR)
		+sizeof(CESERVER_REQ_ONCREATEPROCESS)+(nActionLen+nFileLen+nParamLen)*sizeof(wchar_t));
	
#ifdef _WIN64
	pIn->OnCreateProc.nSourceBits = 64;
#else
	pIn->OnCreateProc.nSourceBits = 32;
#endif
	//pIn->OnCreateProc.bUnicode = TRUE;
	pIn->OnCreateProc.nImageSubsystem = mn_ImageSubsystem;
	pIn->OnCreateProc.nImageBits = mn_ImageBits;
	pIn->OnCreateProc.hStdIn = (unsigned __int64)hStdIn;
	pIn->OnCreateProc.hStdOut = (unsigned __int64)hStdOut;
	pIn->OnCreateProc.hStdErr = (unsigned __int64)hStdErr;
	
	if (aCmd == eShellExecute)
		wcscpy_c(pIn->OnCreateProc.sFunction, L"Shell");
	else if (aCmd == eCreateProcess)
		wcscpy_c(pIn->OnCreateProc.sFunction, L"Create");
	else if (aCmd == eInjectingHooks)
		wcscpy_c(pIn->OnCreateProc.sFunction, L"Inject");
	else if (aCmd == eHooksLoaded)
		wcscpy_c(pIn->OnCreateProc.sFunction, L"HkLoad");
	else if (aCmd == eSrvLoaded)
		wcscpy_c(pIn->OnCreateProc.sFunction, L"SrLoad");
	else if (aCmd == eParmsChanged)
		wcscpy_c(pIn->OnCreateProc.sFunction, L"Changed");
	else if (aCmd == eLoadLibrary)
		wcscpy_c(pIn->OnCreateProc.sFunction, L"LdLib");
	else if (aCmd == eFreeLibrary)
		wcscpy_c(pIn->OnCreateProc.sFunction, L"FrLib");
	else
		wcscpy_c(pIn->OnCreateProc.sFunction, L"Unknown");
	
	pIn->OnCreateProc.nShellFlags = anShellFlags ? *anShellFlags : 0;
	pIn->OnCreateProc.nCreateFlags = anCreateFlags ? *anCreateFlags : 0;
	pIn->OnCreateProc.nStartFlags = anStartFlags ? *anStartFlags : 0;
	pIn->OnCreateProc.nShowCmd = anShowCmd ? *anShowCmd : 0;
	pIn->OnCreateProc.nActionLen = nActionLen;
	pIn->OnCreateProc.nFileLen = nFileLen;
	pIn->OnCreateProc.nParamLen = nParamLen;
	
	wchar_t* psz = pIn->OnCreateProc.wsValue;
	if (nActionLen > 1)
		_wcscpy_c(psz, nActionLen, asAction);
	psz += nActionLen;
	if (nFileLen > 1)
		_wcscpy_c(psz, nFileLen, asFile);
	psz += nFileLen;
	if (nParamLen > 1)
		_wcscpy_c(psz, nParamLen, asParam);
	psz += nParamLen;
	
	return pIn;
}

// Forward
CESERVER_REQ* ExecuteCmd(const wchar_t* szGuiPipeName, const CESERVER_REQ* pIn, DWORD nWaitPipe, HWND hOwner);

// Выполнить в GUI (в CRealConsole)
CESERVER_REQ* ExecuteGuiCmd(HWND hConWnd, const CESERVER_REQ* pIn, HWND hOwner)
{
	wchar_t szGuiPipeName[128];

	if (!hConWnd)
		return NULL;

	DWORD nLastErr = GetLastError();
	//_wsprintf(szGuiPipeName, SKIPLEN(countof(szGuiPipeName)) CEGUIPIPENAME, L".", (DWORD)hConWnd);
	msprintf(szGuiPipeName, countof(szGuiPipeName), CEGUIPIPENAME, L".", (DWORD)hConWnd);
#ifdef _DEBUG
	DWORD nStartTick = GetTickCount();
#endif
	CESERVER_REQ* lpRet = ExecuteCmd(szGuiPipeName, pIn, 1000, hOwner);
#ifdef _DEBUG
	DWORD nEndTick = GetTickCount();
	DWORD nDelta = nEndTick - nStartTick;
	if (nDelta >= EXECUTE_CMD_WARN_TIMEOUT && !IsDebuggerPresent())
	{
		if (lpRet)
		{
			_ASSERTE(nDelta <= EXECUTE_CMD_WARN_TIMEOUT || (pIn->hdr.nCmd == CECMD_CMDSTARTSTOP && nDelta <= EXECUTE_CMD_WARN_TIMEOUT2));
		}
		else
		{
			_ASSERTE(nDelta <= EXECUTE_CMD_TIMEOUT_SRV_ABSENT);
		}
	}
#endif
	SetLastError(nLastErr); // Чтобы не мешать процессу своими возможными ошибками общения с пайпом
	return lpRet;
}

// Выполнить в ConEmuC
CESERVER_REQ* ExecuteSrvCmd(DWORD dwSrvPID, const CESERVER_REQ* pIn, HWND hOwner)
{
	wchar_t szGuiPipeName[128];

	if (!dwSrvPID)
		return NULL;

	DWORD nLastErr = GetLastError();
	//_wsprintf(szGuiPipeName, SKIPLEN(countof(szGuiPipeName)) CESERVERPIPENAME, L".", (DWORD)dwSrvPID);
	msprintf(szGuiPipeName, countof(szGuiPipeName), CESERVERPIPENAME, L".", (DWORD)dwSrvPID);
	CESERVER_REQ* lpRet = ExecuteCmd(szGuiPipeName, pIn, 1000, hOwner);
	SetLastError(nLastErr); // Чтобы не мешать процессу своими возможными ошибками общения с пайпом
	return lpRet;
}

//Arguments:
//   hConWnd - Хэндл КОНСОЛЬНОГО окна (по нему формируется имя пайпа для GUI)
//   pIn     - выполняемая команда
//Returns:
//   CESERVER_REQ. Его необходимо освободить через free(...);
//WARNING!!!
//   Эта процедура не может получить с сервера более 600 байт данных!
// В заголовке hOwner в дебаге может быть отображена ошибка
CESERVER_REQ* ExecuteCmd(const wchar_t* szGuiPipeName, const CESERVER_REQ* pIn, DWORD nWaitPipe, HWND hOwner)
{
	CESERVER_REQ* pOut = NULL;
	HANDLE hPipe = NULL;
	BYTE cbReadBuf[600]; // чтобы CESERVER_REQ_OUTPUTFILE поместился
	wchar_t szErr[MAX_PATH*2];
	BOOL fSuccess = FALSE;
	DWORD cbRead = 0, /*dwMode = 0,*/ dwErr = 0;

	if (!pIn || !szGuiPipeName)
	{
		_ASSERTE(pIn && szGuiPipeName);
		return NULL;
	}

	_ASSERTE(pIn->hdr.nSrcPID && pIn->hdr.nSrcThreadId);
	_ASSERTE(pIn->hdr.cbSize >= sizeof(pIn->hdr));
	hPipe = ExecuteOpenPipe(szGuiPipeName, szErr, NULL/*Сюда хорошо бы имя модуля подкрутить*/);

	if (hPipe == NULL || hPipe == INVALID_HANDLE_VALUE)
	{
#ifdef _DEBUG

		//_ASSERTE(hPipe != NULL && hPipe != INVALID_HANDLE_VALUE);
#ifdef CONEMU_MINIMAL
		SetConsoleTitle(szErr);
#else
		if (hOwner)
		{
			if (hOwner == GetConsoleWindow())
				SetConsoleTitle(szErr);
			else
				SetWindowText(hOwner, szErr);
		}
#endif

#endif
		return NULL;
	}

	//// Try to open a named pipe; wait for it, if necessary.
	//while (1)
	//{
	//	hPipe = CreateFile(
	//		szGuiPipeName,  // pipe name
	//		GENERIC_READ |  // read and write access
	//		GENERIC_WRITE,
	//		0,              // no sharing
	//		NULL,           // default security attributes
	//		OPEN_EXISTING,  // opens existing pipe
	//		0,              // default attributes
	//		NULL);          // no template file
	//
	//	// Break if the pipe handle is valid.
	//	if (hPipe != INVALID_HANDLE_VALUE)
	//		break;
	//
	//	// Exit if an error other than ERROR_PIPE_BUSY occurs.
	//	dwErr = GetLastError();
	//	if (dwErr != ERROR_PIPE_BUSY)
	//	{
	//		return NULL;
	//	}
	//
	//	// All pipe instances are busy, so wait for 1 second.
	//	if (!WaitNamedPipe(szGuiPipeName, nWaitPipe) )
	//	{
	//		return NULL;
	//	}
	//}
	//
	//// The pipe connected; change to message-read mode.
	//dwMode = PIPE_READMODE_MESSAGE;
	//fSuccess = SetNamedPipeHandleState(
	//	hPipe,    // pipe handle
	//	&dwMode,  // new pipe mode
	//	NULL,     // don't set maximum bytes
	//	NULL);    // don't set maximum time
	//if (!fSuccess)
	//{
	//	CloseHandle(hPipe);
	//	return NULL;
	//}
	_ASSERTE(pIn->hdr.nSrcThreadId==GetCurrentThreadId());
	// Send a message to the pipe server and read the response.
	fSuccess = TransactNamedPipe(
	               hPipe,                  // pipe handle
	               (LPVOID)pIn,            // message to server
	               pIn->hdr.cbSize,         // message length
	               cbReadBuf,              // buffer to receive reply
	               sizeof(cbReadBuf),      // size of read buffer
	               &cbRead,                // bytes read
	               NULL);                  // not overlapped
	dwErr = GetLastError();
	//CloseHandle(hPipe);

	if (!fSuccess && (dwErr != ERROR_MORE_DATA))
	{
		//_ASSERTE(fSuccess || (dwErr == ERROR_MORE_DATA));
		CloseHandle(hPipe);
		return NULL;
	}

	if (cbRead < sizeof(CESERVER_REQ_HDR))
	{
		CloseHandle(hPipe);
		return NULL;
	}

	pOut = (CESERVER_REQ*)cbReadBuf; // temporary

	if (pOut->hdr.cbSize < cbRead)
	{
		CloseHandle(hPipe);
		OutputDebugString(L"!!! Wrong nSize received from GUI server !!!\n");
		return NULL;
	}

	if (pOut->hdr.nVersion != CESERVER_REQ_VER)
	{
		CloseHandle(hPipe);
		OutputDebugString(L"!!! Wrong nVersion received from GUI server !!!\n");
		return NULL;
	}

	int nAllSize = pOut->hdr.cbSize;
	pOut = (CESERVER_REQ*)malloc(nAllSize);
	_ASSERTE(pOut);

	if (!pOut)
	{
		CloseHandle(hPipe);
		return NULL;
	}

	memmove(pOut, cbReadBuf, cbRead);
	LPBYTE ptrData = ((LPBYTE)pOut)+cbRead;
	nAllSize -= cbRead;

	while(nAllSize>0)
	{
		// Break if TransactNamedPipe or ReadFile is successful
		if (fSuccess)
			break;

		// Read from the pipe if there is more data in the message.
		fSuccess = ReadFile(
		               hPipe,      // pipe handle
		               ptrData,    // buffer to receive reply
		               nAllSize,   // size of buffer
		               &cbRead,    // number of bytes read
		               NULL);      // not overlapped

		// Exit if an error other than ERROR_MORE_DATA occurs.
		if (!fSuccess && (GetLastError() != ERROR_MORE_DATA))
			break;

		ptrData += cbRead;
		nAllSize -= cbRead;
	}

	CloseHandle(hPipe);
	if (pOut && (pOut->hdr.nCmd != pIn->hdr.nCmd))
	{
		_ASSERTE(pOut->hdr.nCmd == pIn->hdr.nCmd);
		if (pOut->hdr.nCmd == 0)
		{
			ExecuteFreeResult(pOut);
			pOut = NULL;
		}
	}
	return pOut;
}

void ExecuteFreeResult(CESERVER_REQ* pOut)
{
	if (!pOut) return;

	free(pOut);
}

HWND myGetConsoleWindow()
{
	HWND hConWnd = NULL;
	static FGetConsoleWindow fGetConsoleWindow = NULL;

	if (!fGetConsoleWindow)
	{
		HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");

		if (hKernel32)
		{
			fGetConsoleWindow = (FGetConsoleWindow)GetProcAddress(hKernel32, "GetConsoleWindow");
		}
	}

	if (fGetConsoleWindow)
		hConWnd = fGetConsoleWindow();

	return hConWnd;
}

// Returns HWND of Gui console DC window
HWND GetConEmuHWND(BOOL abRoot)
{
	DWORD nLastErr = GetLastError();
	HWND FarHwnd = NULL, ConEmuHwnd = NULL, ConEmuRoot = NULL;
	FarHwnd = myGetConsoleWindow();
	if (!FarHwnd)
	{
		goto wrap;
		//SetLastError(nLastErr);
		//return NULL;
	}

	// Сначала пробуем Mapping консоли (вдруг есть?)
	if (!ConEmuRoot)
	{
		MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> ConMap;
		ConMap.InitName(CECONMAPNAME, (DWORD)FarHwnd);
		CESERVER_CONSOLE_MAPPING_HDR* p = ConMap.Open();
		if (p && p->hConEmuRoot && isWindow(p->hConEmuRoot))
		{
			// Успешно
			ConEmuRoot = p->hConEmuRoot;
			ConEmuHwnd = p->hConEmuWnd;
		}
	}

#ifdef _DEBUG
	// Сервер не мог подцепиться БЕЗ создания мэппинга, поэтому CECMD_GETGUIHWND можно не делать
	if (!ConEmuRoot)
	{
		//BOOL lbRc = FALSE;
		CESERVER_REQ in = {{0}}, *pOut = NULL;
		wchar_t szGuiPipeName[128];

		ExecutePrepareCmd(&in, CECMD_GETGUIHWND, sizeof(CESERVER_REQ_HDR));
		//_wsprintf(szGuiPipeName, SKIPLEN(countof(szGuiPipeName)) CEGUIPIPENAME, L".", (DWORD)FarHwnd);
		msprintf(szGuiPipeName, countof(szGuiPipeName), CEGUIPIPENAME, L".", (DWORD)FarHwnd);
		// Таймаут уменьшим, т.к. на результат не надеемся
		pOut = ExecuteCmd(szGuiPipeName, &in, 250, FarHwnd);

		if (!pOut)
			goto wrap;

		if (pOut->hdr.cbSize != (sizeof(CESERVER_REQ_HDR)+2*sizeof(DWORD)) || pOut->hdr.nCmd != in.hdr.nCmd)
		{
			ExecuteFreeResult(pOut);
			goto wrap;
		}

		ConEmuRoot = (HWND)pOut->dwData[0];
		ConEmuHwnd = (HWND)pOut->dwData[1];
		// Сервер не мог подцепиться БЕЗ создания мэппинга, поэтому CECMD_GETGUIHWND не должен был пройти успешно
		_ASSERTE(ConEmuRoot == NULL);
		ExecuteFreeResult(pOut);
	}
#endif

wrap:
	SetLastError(nLastErr);
	return (abRoot) ? ConEmuRoot : ConEmuHwnd;
}


// hConEmuWnd - HWND с отрисовкой!
void SetConEmuEnvVar(HWND hConEmuWnd)
{
	if (hConEmuWnd)
	{
		// Установить переменную среды с дескриптором окна
		wchar_t szVar[16];
		msprintf(szVar, countof(szVar), L"0x%08X", (DWORD)hConEmuWnd);
		SetEnvironmentVariable(L"ConEmuHWND", szVar);
	}
	else
	{
		SetEnvironmentVariable(L"ConEmuHWND", NULL);
	}
}


// 0 -- All OK (ConEmu found, Version OK)
// 1 -- NO ConEmu (simple console mode)
// (obsolete) 2 -- ConEmu found, but old version
int ConEmuCheck(HWND* ahConEmuWnd)
{
	//int nChk = -1;
	HWND ConEmuWnd = NULL;
	ConEmuWnd = GetConEmuHWND(FALSE/*abRoot*/  /*, &nChk*/);

	// Если хотели узнать хэндл - возвращаем его
	if (ahConEmuWnd) *ahConEmuWnd = ConEmuWnd;

	if (ConEmuWnd == NULL)
	{
		return 1; // NO ConEmu (simple console mode)
	}
	else
	{
		//if (nChk>=3)
		//    return 2; // ConEmu found, but old version
		return 0;
	}
}

/*HWND AtoH(char *Str, int Len)
{
  DWORD Ret=0;
  for (; Len && *Str; Len--, Str++)
  {
    if (*Str>='0' && *Str<='9')
      Ret = (Ret<<4)+(*Str-'0');
    else if (*Str>='a' && *Str<='f')
      Ret = (Ret<<4)+(*Str-'a'+10);
    else if (*Str>='A' && *Str<='F')
      Ret = (Ret<<4)+(*Str-'A'+10);
  }
  return (HWND)Ret;
}*/

// Вернуть путь к папке, содержащей ConEmuC.exe
#ifndef CONEMU_MINIMAL
BOOL FindConEmuBaseDir(wchar_t (&rsConEmuBaseDir)[MAX_PATH+1], wchar_t (&rsConEmuExe)[MAX_PATH+1])
{
	// Сначала пробуем Mapping консоли (вдруг есть?)
	{
		MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> ConMap;
		ConMap.InitName(CECONMAPNAME, (DWORD)myGetConsoleWindow());
		CESERVER_CONSOLE_MAPPING_HDR* p = ConMap.Open();
		if (p && p->sConEmuBaseDir[0])
		{
			// Успешно
			wcscpy_c(rsConEmuBaseDir, p->sConEmuBaseDir);
			wcscpy_c(rsConEmuExe, p->sConEmuExe);
			return TRUE;
		}
	}

	// Теперь - пробуем найти существующее окно ConEmu
	HWND hConEmu = FindWindow(VirtualConsoleClassMain, NULL);
	DWORD dwGuiPID = 0;
	if (hConEmu)
	{
		if (GetWindowThreadProcessId(hConEmu, &dwGuiPID) && dwGuiPID)
		{
			MFileMapping<ConEmuGuiMapping> GuiMap;
			GuiMap.InitName(CEGUIINFOMAPNAME, dwGuiPID);
			ConEmuGuiMapping* p = GuiMap.Open();
			if (p && p->sConEmuBaseDir[0])
			{
				wcscpy_c(rsConEmuBaseDir, p->sConEmuBaseDir);
				wcscpy_c(rsConEmuExe, p->sConEmuExe);
				return TRUE;
			}
		}
	}


	wchar_t szExePath[MAX_PATH+1];
	HKEY hkRoot[] = {NULL,HKEY_CURRENT_USER,HKEY_LOCAL_MACHINE,HKEY_LOCAL_MACHINE};
	DWORD samDesired = KEY_QUERY_VALUE;
	DWORD RedirectionFlag = 0;
	BOOL isWin64 = FALSE;
#ifdef _WIN64
	isWin64 = TRUE;
	RedirectionFlag = KEY_WOW64_32KEY;
#else
	isWin64 = IsWindows64();
	RedirectionFlag = isWin64 ? KEY_WOW64_64KEY : 0;
#endif
	for (size_t i = 0; i < countof(hkRoot); i++)
	{
		szExePath[0] = 0;

		if (i == 0)
		{
			// Запущенного ConEmu.exe нет, можно поискать в каталоге текущего приложения

			if (!GetModuleFileName(NULL, szExePath, countof(szExePath)-20))
				continue;
			wchar_t* pszName = wcsrchr(szExePath, L'\\');
			if (!pszName)
				continue;
			*(pszName+1) = 0;
		}
		else
		{
			// Остался последний шанс - если ConEmu установлен через MSI, то путь указан в реестре
			// [HKEY_LOCAL_MACHINE\SOFTWARE\ConEmu]
			// "InstallDir"="C:\\Utils\\Far180\\"

			if (i == (countof(hkRoot)-1))
			{
				if (RedirectionFlag)
					samDesired |= RedirectionFlag;
				else
					break;
			}

			HKEY hKey;
			if (RegOpenKeyEx(hkRoot[i], L"Software\\ConEmu", 0, samDesired, &hKey) != ERROR_SUCCESS)
				continue;
			memset(szExePath, 0, countof(szExePath));
			DWORD nType = 0, nSize = sizeof(szExePath)-20*sizeof(wchar_t);
			int RegResult = RegQueryValueEx(hKey, L"", NULL, &nType, (LPBYTE)szExePath, &nSize);
			RegCloseKey(hKey);
			if (RegResult != ERROR_SUCCESS)
				continue;
		}

		if (szExePath[0])
		{
			// Хоть и задано в реестре - файлов может не быть. Проверяем
			if (szExePath[lstrlen(szExePath)-1] != L'\\')
				wcscat_c(szExePath, L"\\");
			wcscpy_c(rsConEmuExe, szExePath);
			BOOL lbExeFound = FALSE;
			wchar_t* pszName = rsConEmuExe+lstrlen(rsConEmuExe);
			LPCWSTR szGuiExe[2] = {L"ConEmu64.exe", L"ConEmu.exe"};
			for (UINT i = 0; !lbExeFound && (i < countof(szGuiExe)); i++)
			{
				if (!i && !isWin64) continue;
				wcscpy_add(pszName, rsConEmuExe, szGuiExe[i]);
				lbExeFound = FileExists(rsConEmuExe);
			}

			// Если GUI-exe найден - ищем "base"
			if (lbExeFound)
			{
				wchar_t* pszName = szExePath+lstrlen(szExePath);
				LPCWSTR szSrvExe[4] = {L"ConEmuC64.exe", L"ConEmu\\ConEmuC64.exe", L"ConEmuC.exe", L"ConEmu\\ConEmuC.exe"};
				for (UINT i = 0; (i < countof(szSrvExe)); i++)
				{
					if ((i <=1) && !isWin64) continue;
					wcscpy_add(pszName, szExePath, szSrvExe[i]);
					if (FileExists(szExePath))
					{
						pszName = wcsrchr(szExePath, L'\\');
						if (pszName)
						{
							*pszName = 0; // БЕЗ слеша на конце!
							wcscpy_c(rsConEmuBaseDir, szExePath);
							return TRUE;
						}
					}
				}
			}
		}
	}

	// Не удалось
	return FALSE;
}
#endif

int GuiMessageBox(HWND hConEmuWndRoot, LPCWSTR asText, LPCWSTR asTitle, int anBtns)
{
	int nResult = 0;
	
	if (hConEmuWndRoot)
	{
		HWND hConWnd = GetConsoleWindow();
		CESERVER_REQ In;
		ExecutePrepareCmd(&In, CECMD_ASSERT, sizeof(CESERVER_REQ_HDR)+sizeof(MyAssertInfo));
		In.AssertInfo.nBtns = anBtns;
		_wcscpyn_c(In.AssertInfo.szTitle, countof(In.AssertInfo.szTitle), asTitle, countof(In.AssertInfo.szTitle));
		_wcscpyn_c(In.AssertInfo.szDebugInfo, countof(In.AssertInfo.szDebugInfo), asText, countof(In.AssertInfo.szDebugInfo));

		wchar_t szGuiPipeName[128];
		msprintf(szGuiPipeName, countof(szGuiPipeName), CEGUIPIPENAME, L".", (DWORD)hConEmuWndRoot);

		CESERVER_REQ* pOut = ExecuteCmd(szGuiPipeName, &In, 1000, hConWnd);

		if (pOut)
		{
			if (pOut->hdr.cbSize > sizeof(CESERVER_REQ_HDR))
			{
				nResult = pOut->dwData[0];
			}
			ExecuteFreeResult(pOut);
		}
	}

	return nResult;
}