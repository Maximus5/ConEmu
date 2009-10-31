
#include <windows.h>
#define _T(s) s
#include "ConEmuCheck.h"

SECURITY_ATTRIBUTES* gpNullSecurity = NULL;

#ifdef _DEBUG
	#include <crtdbg.h>
#else
	#ifndef _ASSERT
	#define _ASSERT()
	#endif
	
	#ifndef _ASSERTE
	#define _ASSERTE(f)
	#endif
#endif

extern LPVOID _calloc(size_t,size_t);
extern LPVOID _malloc(size_t);
extern void   _free(LPVOID);

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

HANDLE ExecuteOpenPipe(const wchar_t* szPipeName, wchar_t* pszErr/*[MAX_PATH*2]*/, const wchar_t* szModule)
{
    HANDLE hPipe = NULL;
    DWORD dwErr = 0, dwMode = 0;
    BOOL fSuccess = FALSE;
    DWORD dwStartTick = GetTickCount();
    int nTries = 2;

	_ASSERTE(gpNullSecurity!=NULL);

    // Try to open a named pipe; wait for it, if necessary. 
    while (1)

    { 
        hPipe = CreateFile( 
            szPipeName,     // pipe name 
            GENERIC_READ|GENERIC_WRITE, 
            0,              // no sharing 
            gpNullSecurity, // default security attributes
            OPEN_EXISTING,  // opens existing pipe 
            0,              // default attributes 
            NULL);          // no template file 

        // Break if the pipe handle is valid. 
        if (hPipe != INVALID_HANDLE_VALUE) 
            break; // OK, открыли
        dwErr = GetLastError();

        // Сделаем так, чтобы хотя бы пару раз он попробовал повторить
        if ((nTries <= 0) && (GetTickCount() - dwStartTick) > 100) {
			if (pszErr) {
				wsprintf(pszErr, L"%s: CreateFile(%s) failed, code=0x%08X, Timeout", 
					szModule ? szModule : L"Unknown", szPipeName, dwErr);
			}
            return NULL;
        } else {
        	nTries--;
        }
        
        // Может быть пайп еще не создан (в процессе срабатывания семафора)
        if (dwErr == ERROR_FILE_NOT_FOUND) {
        	Sleep(10);
        	continue;
        }

        // Exit if an error other than ERROR_PIPE_BUSY occurs. 
		if (dwErr != ERROR_PIPE_BUSY) {
			if (pszErr) {
				wsprintf(pszErr, L"%s: CreateFile(%s) failed, code=0x%08X", 
					szModule ? szModule : L"Unknown", szPipeName, dwErr);
			}
            return NULL;
		}

        // All pipe instances are busy, so wait for 1000 ms.
		if (!WaitNamedPipe(szPipeName, 1000) ) {
			dwErr = GetLastError();
			if (pszErr) {
				wsprintf(pszErr, L"%s: WaitNamedPipe(%s) failed, code=0x%08X, WaitNamedPipe", 
					szModule ? szModule : L"Unknown", szPipeName, dwErr);
			}
            return NULL;
		}
    }

    // The pipe connected; change to message-read mode. 
    dwMode = PIPE_READMODE_MESSAGE; 
    fSuccess = SetNamedPipeHandleState( 
        hPipe,    // pipe handle 
        &dwMode,  // new pipe mode 
        NULL,     // don't set maximum bytes 
        NULL);    // don't set maximum time 
    if (!fSuccess) {
    	dwErr = GetLastError();
    	_ASSERT(fSuccess);
		if (pszErr) {
			wsprintf(pszErr, L"%s: SetNamedPipeHandleState(%s) failed, code=0x%08X", 
				szModule ? szModule : L"Unknown", szPipeName, dwErr);
		}
        CloseHandle(hPipe);
        return NULL;
    }
    
    return hPipe;
}


CESERVER_REQ* ExecuteNewCmd(DWORD nCmd, DWORD nSize)
{
    CESERVER_REQ* pIn = NULL;
    if (nSize) {
		// Обязательно с обнулением выделяемой памяти
        pIn = (CESERVER_REQ*)_calloc(nSize, 1);
        if (pIn) {
	        pIn->hdr.nCmd = nCmd;
	        pIn->hdr.nSrcThreadId = GetCurrentThreadId();
	        pIn->hdr.nSize = nSize;
	        pIn->hdr.nVersion = CESERVER_REQ_VER;
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

	wsprintfW(szGuiPipeName, CEGUIPIPENAME, L".", (DWORD)hConWnd);
	
	return ExecuteCmd(szGuiPipeName, pIn, 1000, hOwner);
}

// Выполнить в ConEmuC
CESERVER_REQ* ExecuteSrvCmd(DWORD dwSrvPID, const CESERVER_REQ* pIn, HWND hOwner)
{
	wchar_t szGuiPipeName[MAX_PATH];
	
	if (!dwSrvPID)
		return NULL;

	wsprintfW(szGuiPipeName, CESERVERPIPENAME, L".", (DWORD)dwSrvPID);
	
	return ExecuteCmd(szGuiPipeName, pIn, 1000, hOwner);
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

	if (!pIn || !szGuiPipeName) {
		_ASSERTE(pIn && szGuiPipeName);
		return NULL;
	}
		
	hPipe = ExecuteOpenPipe(szGuiPipeName, szErr, NULL/*Сюда хорошо бы имя модуля подкрутить*/);
	if (hPipe == NULL || hPipe == INVALID_HANDLE_VALUE) {
		#ifdef _DEBUG
		if (hOwner) {
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
		pIn->hdr.nSize,             // message length 
		cbReadBuf,              // buffer to receive reply
		sizeof(cbReadBuf),      // size of read buffer
		&cbRead,                // bytes read
		NULL);                  // not overlapped 

	dwErr = GetLastError();
	CloseHandle(hPipe);

	if (!fSuccess /*&& (dwErr != ERROR_MORE_DATA)*/)
	{
		return NULL;
	}

	if (cbRead < sizeof(CESERVER_REQ_HDR))
		return NULL;

	pOut = (CESERVER_REQ*)cbReadBuf; // temporary
	
	if (pOut->hdr.nSize != cbRead) {
		OutputDebugString(L"!!! Wrong nSize received from GUI server !!!\n");
		return NULL;
	}

	if (pOut->hdr.nVersion != CESERVER_REQ_VER) {
		OutputDebugString(L"!!! Wrong nVersion received from GUI server !!!\n");
		return NULL;
	}
	
	pOut = (CESERVER_REQ*)_malloc(cbRead);
	_ASSERTE(pOut);
	if (!pOut) return NULL;
	memmove(pOut, cbReadBuf, cbRead);

	return pOut;
}

void ExecuteFreeResult(CESERVER_REQ* pOut)
{
	if (!pOut) return;
	_free(pOut);
}

HWND myGetConsoleWindow()
{
	HWND hConWnd = NULL;
	static FGetConsoleWindow fGetConsoleWindow = NULL;
	if (!fGetConsoleWindow) {
		HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
		if (hKernel32) {
			fGetConsoleWindow = (FGetConsoleWindow)GetProcAddress( hKernel32, "GetConsoleWindow" );
		}
	}
	if (fGetConsoleWindow) 
		hConWnd = fGetConsoleWindow();
	return hConWnd;
}

// Returns HWND of Gui console DC window
HWND GetConEmuHWND(BOOL abRoot)
{
	//BOOL lbRc = FALSE;
	HWND FarHwnd=NULL, ConEmuHwnd=NULL, ConEmuRoot=NULL;
	CESERVER_REQ in = {{0}}, *pOut = NULL;

	FarHwnd = myGetConsoleWindow();
	if ( !FarHwnd )
		return NULL;

	in.hdr.nSize = sizeof(CESERVER_REQ_HDR);
	in.hdr.nVersion = CESERVER_REQ_VER;
	in.hdr.nCmd  = CECMD_GETGUIHWND;
	in.hdr.nSrcThreadId = GetCurrentThreadId();

	pOut = ExecuteGuiCmd(FarHwnd, &in, NULL);
	if (!pOut)
		return NULL;
	
	if (pOut->hdr.nSize != (sizeof(CESERVER_REQ_HDR)+2*sizeof(DWORD)) || pOut->hdr.nCmd != in.hdr.nCmd) {
		ExecuteFreeResult(pOut);
		return NULL;
	}

	ConEmuRoot = (HWND)pOut->dwData[0];
	ConEmuHwnd = (HWND)pOut->dwData[1];

	ExecuteFreeResult(pOut);
	
	return (abRoot) ? ConEmuRoot : ConEmuHwnd;
}


// hConEmuWnd - HWND с отрисовкой!
void SetConEmuEnvVar(HWND hConEmuWnd)
{
	if (hConEmuWnd) {
        // Установить переменную среды с дескриптором окна
        WCHAR szVar[32];
        wsprintf(szVar, L"0x%08X", hConEmuWnd);
        SetEnvironmentVariable(L"ConEmuHWND", szVar);
    } else {
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
    
    if (ConEmuWnd == NULL) {
	    return 1; // NO ConEmu (simple console mode)
    } else {
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
