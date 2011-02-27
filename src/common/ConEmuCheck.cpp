
/*
Copyright (c) 2009-2010 Maximus5
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

SECURITY_ATTRIBUTES* gpLocalSecurity = NULL;

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
	_ASSERTE(gpLocalSecurity!=NULL);

	// Try to open a named pipe; wait for it, if necessary.
	while(1)
	{
		hPipe = CreateFile(
		            szPipeName,     // pipe name
		            GENERIC_READ|GENERIC_WRITE,
		            0,              // no sharing
		            gpLocalSecurity, // default security attributes
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
				_wsprintf(szErr, SKIPLEN(countof(szErr)) L"%s.%u: CreateFile(%s) failed, code=0x%08X, Timeout",
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
				_wsprintf(szErr, SKIPLEN(countof(szErr)) L"%s.%u: CreateFile(%s) failed, code=0x%08X",
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
			_wsprintf(szErr, SKIPLEN(countof(szErr)) L"%s.%u: SetNamedPipeHandleState(%s) failed, code=0x%08X",
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

// Forward
CESERVER_REQ* ExecuteCmd(const wchar_t* szGuiPipeName, const CESERVER_REQ* pIn, DWORD nWaitPipe, HWND hOwner);

// Выполнить в GUI (в CRealConsole)
CESERVER_REQ* ExecuteGuiCmd(HWND hConWnd, const CESERVER_REQ* pIn, HWND hOwner)
{
	wchar_t szGuiPipeName[MAX_PATH];

	if (!hConWnd)
		return NULL;

	DWORD nLastErr = GetLastError();
	_wsprintf(szGuiPipeName, SKIPLEN(countof(szGuiPipeName)) CEGUIPIPENAME, L".", (DWORD)hConWnd);
#ifdef _DEBUG
	DWORD nStartTick = GetTickCount();
#endif
	CESERVER_REQ* lpRet = ExecuteCmd(szGuiPipeName, pIn, 1000, hOwner);
#ifdef _DEBUG
	DWORD nEndTick = GetTickCount();
	DWORD nDelta = nEndTick - nStartTick;
	if (nDelta >= EXECUTE_CMD_WARN_TIMEOUT && !IsDebuggerPresent())
	{
		_ASSERTE(nDelta <= EXECUTE_CMD_WARN_TIMEOUT);
	}
#endif
	SetLastError(nLastErr); // Чтобы не мешать процессу своими возможными ошибками общения с пайпом
	return lpRet;
}

// Выполнить в ConEmuC
CESERVER_REQ* ExecuteSrvCmd(DWORD dwSrvPID, const CESERVER_REQ* pIn, HWND hOwner)
{
	wchar_t szGuiPipeName[MAX_PATH];

	if (!dwSrvPID)
		return NULL;

	DWORD nLastErr = GetLastError();
	_wsprintf(szGuiPipeName, SKIPLEN(countof(szGuiPipeName)) CESERVERPIPENAME, L".", (DWORD)dwSrvPID);
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
		if (hOwner)
		{
			if (hOwner == GetConsoleWindow())
				SetConsoleTitle(szErr);
			else
				SetWindowText(hOwner, szErr);
		}

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
	HWND FarHwnd = NULL, ConEmuHwnd = NULL, ConEmuRoot = NULL;
	FarHwnd = myGetConsoleWindow();
	if (!FarHwnd)
		return NULL;

	// Сначала пробуем Mapping консоли (вдруг есть?)
	if (!ConEmuRoot)
	{
		MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> ConMap;
		ConMap.InitName(CECONMAPNAME, (DWORD)FarHwnd);
		CESERVER_CONSOLE_MAPPING_HDR* p = ConMap.Open();
		if (p && p->hConEmuRoot && IsWindow(p->hConEmuRoot))
		{
			// Успешно
			ConEmuRoot = p->hConEmuRoot;
			ConEmuHwnd = p->hConEmuWnd;
		}
	}

	if (!ConEmuRoot)
	{
		//BOOL lbRc = FALSE;
		CESERVER_REQ in = {{0}}, *pOut = NULL;

		ExecutePrepareCmd(&in, CECMD_GETGUIHWND, sizeof(CESERVER_REQ_HDR));
		pOut = ExecuteGuiCmd(FarHwnd, &in, NULL);

		if (!pOut)
			return NULL;

		if (pOut->hdr.cbSize != (sizeof(CESERVER_REQ_HDR)+2*sizeof(DWORD)) || pOut->hdr.nCmd != in.hdr.nCmd)
		{
			ExecuteFreeResult(pOut);
			return NULL;
		}

		ConEmuRoot = (HWND)pOut->dwData[0];
		ConEmuHwnd = (HWND)pOut->dwData[1];
		ExecuteFreeResult(pOut);
	}

	return (abRoot) ? ConEmuRoot : ConEmuHwnd;
}


// hConEmuWnd - HWND с отрисовкой!
void SetConEmuEnvVar(HWND hConEmuWnd)
{
	if (hConEmuWnd)
	{
		// Установить переменную среды с дескриптором окна
		WCHAR szVar[32];
		_wsprintf(szVar, SKIPLEN(countof(szVar)) L"0x%08X", hConEmuWnd);
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
			for (int i = 0; !lbExeFound && (i < countof(szGuiExe)); i++)
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
				for (int i = 0; (i < countof(szSrvExe)); i++)
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
