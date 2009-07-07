
#include <windows.h>
#define _T(s) s
#include "ConEmuCheck.h"

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

CESERVER_REQ* ExecuteNewCmd(DWORD nCmd, DWORD nSize)
{
    CESERVER_REQ* pIn = NULL;
    if (nSize) {
        pIn = (CESERVER_REQ*)calloc(nSize, 1);
        if (pIn) {
	        pIn->hdr.nCmd = nCmd;
	        pIn->hdr.nSrcThreadId = GetCurrentThreadId();
	        pIn->hdr.nSize = nSize;
	        pIn->hdr.nVersion = CESERVER_REQ_VER;
        }
    }
    return pIn;
}

//Arguments:
//   hConWnd - Хэндл КОНСОЛЬНОГО окна (по нему формируется имя пайпа для GUI)
//   pIn     - выполняемая команда
//Returns:
//   CESERVER_REQ. Его необходимо освободить через free(...);
//WARNING!!!
//   Эта процедура не может получить с сервера более 512 байт данных!
CESERVER_REQ* ExecuteGuiCmd(HWND hConWnd, const CESERVER_REQ* pIn)
{
	CESERVER_REQ* pOut = NULL;
	wchar_t szGuiPipeName[MAX_PATH];
	HANDLE hPipe = NULL; 
	BYTE cbReadBuf[600]; // чтобы CESERVER_REQ_OUTPUTFILE поместился
	BOOL fSuccess = FALSE;
	DWORD cbRead = 0, dwMode = 0, dwErr = 0;

	if (!hConWnd)
		return NULL;

	wsprintfW(szGuiPipeName, CEGUIPIPENAME, L".", (DWORD)hConWnd);
	//

	// Try to open a named pipe; wait for it, if necessary. 
	while (1) 
	{ 
		hPipe = CreateFile( 
			szGuiPipeName,  // pipe name 
			GENERIC_READ |  // read and write access 
			GENERIC_WRITE, 
			0,              // no sharing 
			NULL,           // default security attributes
			OPEN_EXISTING,  // opens existing pipe 
			0,              // default attributes 
			NULL);          // no template file 

		// Break if the pipe handle is valid. 
		if (hPipe != INVALID_HANDLE_VALUE) 
			break; 

		// Exit if an error other than ERROR_PIPE_BUSY occurs. 
		dwErr = GetLastError();
		if (dwErr != ERROR_PIPE_BUSY) 
		{
			return NULL;
		}

		// All pipe instances are busy, so wait for 1 second.
		if (!WaitNamedPipe(szGuiPipeName, 1000) ) 
		{
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
	if (!fSuccess) 
	{
		CloseHandle(hPipe);
		return NULL;
	}

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
	
	pOut = (CESERVER_REQ*)malloc(cbRead);
	_ASSERTE(pOut);
	if (!pOut) return NULL;
	memmove(pOut, cbReadBuf, cbRead);

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

	pOut = ExecuteGuiCmd(FarHwnd, &in);
	if (!pOut)
		return NULL;
	
	if (pOut->hdr.nSize != (sizeof(CESERVER_REQ_HDR)+2*sizeof(DWORD)) || pOut->hdr.nCmd != in.hdr.nCmd) {
		free(pOut);
		return NULL;
	}

	ConEmuRoot = (HWND)pOut->dwData[0];
	ConEmuHwnd = (HWND)pOut->dwData[1];

	free(pOut);
	
	return (abRoot) ? ConEmuRoot : ConEmuHwnd;
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


//// Returns HWND of Gui console DC window
////     pnConsoleIsChild [out]
////        -1 -- Non ConEmu mode
////         0 -- console is standalone window
////         1 -- console is child of ConEmu DC window
////         2 -- console is child of Main ConEmu window (why?)
////         3 -- same as 2, but ConEmu DC window - absent (old conemu version?)
////         4 -- same as 0, but ConEmu DC window - absent (old conemu version?)
//HWND CheckConEmuChild(HWND ConEmuHwnd, int* pnConsoleIsChild/*=NULL*/)
//{
//	char className[100];
//	HWND hParent=NULL;
//	
//	// начальный сброс
//	if (pnConsoleIsChild) *pnConsoleIsChild = -1;
//	if (!ConEmuHwnd) return NULL;
//
//	
//	GetClassNameA(ConEmuHwnd, className, 100);
//	if (lstrcmpiA(className, VirtualConsoleClass) != 0 &&
//		lstrcmpiA(className, VirtualConsoleClassMain) != 0)
//	{   // Родитель консоли - НЕ КонЭму
//		ConEmuHwnd = NULL; // будем определять его дальше
//	} else {
//		//bWasSetParent = TRUE;
//		if (pnConsoleIsChild) *pnConsoleIsChild = 2; // init
//		// Проверяем родителя родителя консоли
//		hParent = GetAncestor(ConEmuHwnd, GA_PARENT);
//		if (hParent) {
//		    // Должен быть таким
//			GetClassNameA(hParent, className, 100);
//			if (lstrcmpiA(className, VirtualConsoleClassMain) == 0) {
//				if (pnConsoleIsChild) *pnConsoleIsChild = 1; // console is child of ConEmu DC window
//			} else {
//				hParent = NULL; // непорядок, имя класса неправильное, значит консоль в основном окне ConEmu
//			}
//		}
//		if (!hParent) {
//			// 2 -- console is child of Main ConEmu window (why?)
//			if (pnConsoleIsChild) *pnConsoleIsChild = 2;
//			hParent = ConEmuHwnd;
//			ConEmuHwnd = FindWindowExA(hParent, NULL, VirtualConsoleClass, NULL);
//			if (ConEmuHwnd==NULL) {
//			    // same as 2, but ConEmu DC windows - absent (old conemu version?)
//				if (pnConsoleIsChild) *pnConsoleIsChild = 3;
//				ConEmuHwnd = hParent;
//			}
//		}
//	}
//	
//	return ConEmuHwnd;
//}
//
//// Returns HWND of Gui console DC window
////     pnConsoleIsChild [out]
////        -1 -- Non ConEmu mode
////         0 -- console is standalone window
////         1 -- console is child of ConEmu DC window
////         2 -- console is child of Main ConEmu window (why?)
////         3 -- same as 2, but ConEmu DC window - absent (old conemu version?)
////         4 -- same as 0, but ConEmu DC window - absent (old conemu version?)
//HWND GetConEmuHWND(BOOL abRoot, int* pnConsoleIsChild/*=NULL*/)
//{
//	HWND FarHwnd=NULL, ConEmuHwnd=NULL, ConEmuRoot=NULL;
//	FGetConsoleWindow fGetConsoleWindow = NULL;
//	int nChk = -1;
//	
//	fGetConsoleWindow = (FGetConsoleWindow)GetProcAddress( GetModuleHandleA("kernel32.dll"), "GetConsoleWindow" );
//
//	if (fGetConsoleWindow)
//		FarHwnd = fGetConsoleWindow();
//	// начальный сброс
//	if (pnConsoleIsChild) *pnConsoleIsChild = nChk;
//
//	// Если удалось получить хэндл окна консоли - проверяем его родителя
//	if (FarHwnd)
//		ConEmuHwnd = GetAncestor(FarHwnd, GA_PARENT);
//	if (ConEmuHwnd == (HWND)0x10014)
//		ConEmuHwnd = NULL; else
//	if (ConEmuHwnd != NULL)
//	{
//		// Теоретически, если был сделан SetParent - то в дочернее окно с отрисовкой, но фиг его знает. проверим
//		ConEmuHwnd = CheckConEmuChild(ConEmuHwnd, &nChk);
//	}
//
//
//#ifndef CONMANSERVER
//	if (!ConEmuHwnd) {
//		char className[100];
//		if (GetEnvironmentVariableA("ConEmuHWND", className, 99)) {
//			if (className[0]==L'0' && className[1]==L'x') {
//				// Получаем хэндл окна ConEmu из переменной среды
//				ConEmuHwnd = AtoH(className+2, 8);
//				
//				if (ConEmuHwnd) {
//					
//					GetClassNameA(ConEmuHwnd, className, 100);
//					if (lstrcmpiA(className, VirtualConsoleClass) != 0)
//					{
//						ConEmuHwnd = NULL;
//					} else {
//						// Нужно проверить, что консоль принадлежит процессу ConEmu
//						DWORD dwEmuPID = 0;
//						GetWindowThreadProcessId(ConEmuHwnd, &dwEmuPID);
//						if (dwEmuPID == 0) {
//							// Ошибка, не удалось получить код процесса ConEmu, оставим полученный хэндл
//						} else {
//							DWORD *ProcList = (DWORD*)calloc(1000,sizeof(DWORD));
//							DWORD dwCount = 0;
//							FGetConsoleProcessList fGetConsoleProcessList = 
//								(FGetConsoleProcessList)GetProcAddress(
//									GetModuleHandleA("kernel32.dll"), "GetConsoleProcessList");
//	                        if (!fGetConsoleProcessList)
//		                        dwCount = 1001;
//		                    else
//                                dwCount = fGetConsoleProcessList(ProcList,1000);
//                                
//							if(dwCount>1000) {
//								// Ошибка, в консоли слишком много процессов, оставим полученный хэндл
//							} else {
//								DWORD dw=0;
//								for (dw=0; dw<dwCount; dw++) {
//									if (dwEmuPID == ProcList[dw]) {
//										dwEmuPID = 0; break;
//									}
//								}
//								if (dwEmuPID)
//									ConEmuHwnd = NULL; // Консоль не принадлежит ConEmu
//							}
//							free(ProcList);
//						}
//					}
//				}
//				// Проверить "правильность" хэндла
//				ConEmuHwnd = CheckConEmuChild(ConEmuHwnd, &nChk);
//				// Если мы дошли сюда, значит консоль не дочернее окно, и возвращать нужно 4
//				if (nChk==3) nChk=4;
//				else if (nChk==1 || nChk==2) nChk=0;
//			}
//		}
//	}
//#endif
//
//	if (!ConEmuHwnd && FarHwnd) {
//		while((ConEmuHwnd = FindWindowExA(NULL, ConEmuHwnd, VirtualConsoleClassMain, NULL))!=NULL)
//		{
//			HWND hCheck = (HWND)GetWindowLongPtr(ConEmuHwnd, GWLP_USERDATA);
//			if (hCheck == FarHwnd) {
//				// Нашли нужный ConEmu
//				// Проверить "правильность" хэндла
//				ConEmuHwnd = CheckConEmuChild(ConEmuHwnd, &nChk);
//				// Если мы дошли сюда, значит консоль не дочернее окно, и возвращать нужно 4
//				if (nChk==3) nChk=4;
//				else if (nChk==1 || nChk==2) nChk=0;
//				break;
//			}
//		}
//		
//	}
//
//    if (abRoot && ConEmuHwnd && (nChk<3))
//	    ConEmuRoot = GetAncestor(ConEmuHwnd, GA_PARENT);
//	
//	if (pnConsoleIsChild) *pnConsoleIsChild = nChk;
//	
//	if (abRoot)
//		return ConEmuRoot;
//	return ConEmuHwnd;
//}

HWND AtoH(char *Str, int Len)
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
}
