#include <Windows.h>
#include <stdio.h>
#include <Shlwapi.h>
#include <conio.h>
#include <vector>
#include "..\common\common.hpp"

WARNING("Наверное все-же стоит производить периодические чтения содержимого консоли, а не только по событию");

#if defined(__GNUC__)
	//#include "assert.h"
	#define _ASSERTE(x)
#else
	#include <crtdbg.h>
#endif

#ifndef EVENT_CONSOLE_CARET
#define EVENT_CONSOLE_CARET             0x4001
#define EVENT_CONSOLE_UPDATE_REGION     0x4002
#define EVENT_CONSOLE_UPDATE_SIMPLE     0x4003
#define EVENT_CONSOLE_UPDATE_SCROLL     0x4004
#define EVENT_CONSOLE_LAYOUT            0x4005
#define EVENT_CONSOLE_START_APPLICATION 0x4006
#define EVENT_CONSOLE_END_APPLICATION   0x4007
#endif


DWORD WINAPI InstanceThread(LPVOID);
DWORD WINAPI ServerThread(LPVOID lpvParam);
DWORD WINAPI InputThread(LPVOID lpvParam);
BOOL GetAnswerToRequest(CESERVER_REQ& in, CESERVER_REQ** out); 
DWORD WINAPI WinEventThread(LPVOID lpvParam);
void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
void CheckCursorPos();
void SendConsoleChanges(CESERVER_REQ* pOut);
CESERVER_REQ* CreateConsoleInfo(CESERVER_CHAR* pCharOnly, BOOL bCharAttrBuff);
void ReloadConsoleInfo();


DWORD dwSelfPID = 0;
wchar_t szPipename[MAX_PATH], szInputname[MAX_PATH], szGuiPipeName[MAX_PATH];
HANDLE  hConIn = NULL, hConOut = NULL;
HWINEVENTHOOK hWinHook = NULL;
HWND hConWnd = NULL;
//HANDLE hGlblUpdateEvt = NULL;
HANDLE hExitEvent = NULL;
BOOL bAlwaysConfirmExit = FALSE;
BOOL bContentsChanged = TRUE; // Первое чтение параметров должно быть ПОЛНЫМ
CRITICAL_SECTION csProc;
CRITICAL_SECTION csConBuf;
wchar_t* psChars = NULL;
WORD* pnAttrs = NULL;
DWORD nBufCharCount = NULL;
DWORD dwSelRc = 0; CONSOLE_SELECTION_INFO sel = {0}; // GetConsoleSelectionInfo
DWORD dwCiRc = 0; CONSOLE_CURSOR_INFO ci = {0}; // GetConsoleCursorInfo
DWORD dwConsoleCP=0, dwConsoleOutputCP=0, dwConsoleMode=0;
DWORD dwSbiRc = 0; CONSOLE_SCREEN_BUFFER_INFO sbi = {{0,0}}; // GetConsoleScreenBufferInfo


// Список процессов нам нужен, чтобы определить, когда консоль уже не нужна.
// Например, запустили FAR, он запустил Update, FAR перезапущен...
std::vector<DWORD> nProcesses;

//#define CES_NTVDM 0x10 -- common.hpp
DWORD dwActiveFlags = 0;

#define CERR_GETCOMMANDLINE 100
#define CERR_CARGUMENT 101
#define CERR_CMDEXENOTFOUND 102
#define CERR_NOTENOUGHMEM1 103
#define CERR_CREATESERVERTHREAD 104
#define CERR_CREATEPROCESS 105
#define CERR_WINEVENTTHREAD 106
#define CERR_CONINFAILED 107
#define CERR_GETCONSOLEWINDOW 108
#define CERR_EXITEVENT 109
#define CERR_GLOBALUPDATE 110
#define CERR_WINHOOKNOTCREATED 111
#define CERR_CREATEINPUTTHREAD 112
#define CERR_CONOUTFAILED 113
#define CERR_PROCESSTIMEOUT 114


int main()
{
	TODO("printf ошибок бессмысленен, т.к. ConEmu их все-равно не увидит...");
	TODO("можно при ошибках показать консоль, предварительно поставив 80x25 и установив крупный шрифт");

	int iRc = 100;
	wchar_t sComSpec[MAX_PATH];
	wchar_t* psFilePart;
	wchar_t* psCmdLine = GetCommandLineW();
	size_t nCmdLine = lstrlenW(psCmdLine);
	wchar_t* psNewCmd = NULL;
    DWORD dwThreadId = 0, dwWinEventThread = 0, dwThreadInputId = 0;
    HANDLE hThread = NULL, hInputThread, hWinEventThread = NULL, hWait[2]={NULL,NULL};
	BOOL bViaCmdExe = TRUE;
	PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
	STARTUPINFOW si; memset(&si, 0, sizeof(si)); si.cb = sizeof(si);
	DWORD dwErr = 0, nWait = 0;
	BOOL lbRc = FALSE;
	szGuiPipeName[0] = 0;

	dwSelfPID = GetCurrentProcessId();
	wsprintfW(szPipename, CESERVERPIPENAME, L".", dwSelfPID);
	wsprintfW(szInputname, CESERVERINPUTNAME, L".", dwSelfPID);
	
	InitializeCriticalSection(&csConBuf);
	InitializeCriticalSection(&csProc);
	hConWnd = GetConsoleWindow();
	_ASSERTE(hConWnd!=NULL);
	if (!hConWnd) {
		dwErr = GetLastError();
		printf("hConWnd==NULL, ErrCode=0x%08X\n", dwErr); 
		iRc=CERR_GETCONSOLEWINDOW; goto wrap;
	}
	hExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL); ResetEvent(hExitEvent);
	if (!hExitEvent) {
		dwErr = GetLastError();
		printf("CreateEvent() failed, ErrCode=0x%08X\n", dwErr); 
		iRc=CERR_EXITEVENT; goto wrap;
	}
	//wsprintfW(sName, CE_NEEDUPDATE, dwSelfPID);
	//hGlblUpdateEvt = CreateEvent(NULL, FALSE, FALSE, sName);
	//_ASSERTE(hGlblUpdateEvt!=NULL);
	//if (!hGlblUpdateEvt) {
	//	dwErr = GetLastError();
	//	printf("CreateEvent(ConEmuNeedUpdate%u) failed, ErrCode=0x%08X\n", (DWORD)hConWnd, dwErr); 
	//	iRc=CERR_GLOBALUPDATE; goto wrap;
	//}

#ifdef _DEBUG
	if (!IsDebuggerPresent()) MessageBox(0,L"Loaded",L"ComEmuC",0);
#endif
	
	if (!psCmdLine)
	{
		printf ("GetCommandLineW failed!\n" );
		iRc=CERR_GETCOMMANDLINE; goto wrap;
	}
	if ((psCmdLine = wcsstr(psCmdLine, L"/C")) == NULL)
	{
		printf ("/C argument not found!\n" );
		iRc=CERR_CARGUMENT; goto wrap;
	}
	if (wcsncmp(psCmdLine, L"/CMD", 4)==0) {
		bViaCmdExe = FALSE; psCmdLine += 4;
	} else {
		psCmdLine += 2; // нас интересует все что ПОСЛЕ /C
	}
	while (*psCmdLine == L' ') psCmdLine++;
	if (!bViaCmdExe) {
		if (lstrcmpiW(psCmdLine, L"cmd")==0 || lstrcmpiW(psCmdLine, L"cmd ")==0 ||
			lstrcmpiW(psCmdLine, L"cmd.exe")==0 || lstrcmpiW(psCmdLine, L"cmd.exe")==0) {
			bViaCmdExe = FALSE;
		}
	}

	if (!bViaCmdExe) {
		nCmdLine += 10;
	} else {
		//TODO: Хорошо бы попробовать через переменную окружения "старый" процессор
		//TODO: находить. Например, перед заменой можно сохранять ComSpec в ComSpecC
		if (!SearchPathW(NULL, L"cmd.exe", NULL, MAX_PATH, sComSpec, &psFilePart))
		{
			printf ("Can't find cmd.exe!\n");
			iRc=CERR_CMDEXENOTFOUND; goto wrap;
		}
		
		nCmdLine += lstrlenW(sComSpec)+10;
	}
	
	psNewCmd = new wchar_t[nCmdLine];
	if (!psNewCmd)
	{
		printf ("Can't allocate %i wchars!\n", nCmdLine);
		iRc=CERR_NOTENOUGHMEM1; goto wrap;
	}
	
	if (!bViaCmdExe)
	{
		lstrcpyW( psNewCmd, psCmdLine );
	} else {
		psNewCmd[0] = L'"';
		lstrcpyW( psNewCmd+1, sComSpec );
		lstrcatW( psNewCmd, L"\" /C " );
		lstrcatW( psNewCmd, psCmdLine );
	}

	

	hConIn  = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
	            0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hConIn == INVALID_HANDLE_VALUE) {
		dwErr = GetLastError();
		printf("CreateFile(CONIN$) failed, ErrCode=0x%08X\n", dwErr); 
		iRc=CERR_CONINFAILED; goto wrap;
	}
	hConOut = CreateFile(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
	            0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hConOut == INVALID_HANDLE_VALUE) {
		dwErr = GetLastError();
		printf("CreateFile(CONOUT$) failed, ErrCode=0x%08X\n", dwErr); 
		iRc=CERR_CONOUTFAILED; goto wrap;
	}
	// Сразу получить текущее состояние консоли
	ReloadConsoleInfo();

	TODO("Нити обработки команд и SetWinEventHook нужно выполнять только в корневом ConEmuC, а не в ComSpec");

	#ifdef _DEBUG
		//if (!IsDebuggerPresent()) MessageBox(0,L"Debug",L"ConEmuC",0);
	#endif
	// The client thread that calls SetWinEventHook must have a message loop in order to receive events.");
	hWait[0] = CreateEvent(NULL,FALSE,FALSE,NULL);
	_ASSERTE(hWait[0]!=NULL);
	hWinEventThread = CreateThread( 
		NULL,              // no security attribute 
		0,                 // default stack size 
		WinEventThread,      // thread proc
		hWait[0],              // thread parameter 
		0,                 // not suspended 
		&dwWinEventThread);      // returns thread ID 
	if (hWinEventThread == NULL) 
	{
		dwErr = GetLastError();
		printf("CreateThread(WinEventThread) failed, ErrCode=0x%08X\n", dwErr); 
		CloseHandle(hWait[0]); hWait[0]=NULL; hWait[1]=NULL;
		iRc=CERR_WINEVENTTHREAD; goto wrap;
	}
	hWait[1] = hWinEventThread;
	dwErr = WaitForMultipleObjects(2, hWait, FALSE, 10000);
	CloseHandle(hWait[0]); hWait[0]=NULL; hWait[1]=NULL;
	if (!hWinHook) {
		_ASSERT(dwErr == WAIT_TIMEOUT);
		if (dwErr == WAIT_TIMEOUT) { // по идее этого быть не должно
			TerminateThread(hWinEventThread,100);
			CloseHandle(hWinEventThread); hWinEventThread = NULL;
		}
		// Ошибка на экран уже выведена, нить уже завершена, закрыть дескриптор
		CloseHandle(hWinEventThread); hWinEventThread = NULL;
		iRc=CERR_WINHOOKNOTCREATED; goto wrap;
	}


	// Запустить нить обработки команд	
	hThread = CreateThread( 
		NULL,              // no security attribute 
		0,                 // default stack size 
		ServerThread,      // thread proc
		NULL,              // thread parameter 
		0,                 // not suspended 
		&dwThreadId);      // returns thread ID 

	if (hThread == NULL) 
	{
		dwErr = GetLastError();
		printf("CreateThread(ServerThread) failed, ErrCode=0x%08X\n", dwErr); 
		iRc=CERR_CREATESERVERTHREAD; goto wrap;
	}

	// Запустить нить обработки событий (клавиатура, мышь, и пр.)
	hInputThread = CreateThread( 
		NULL,              // no security attribute 
		0,                 // default stack size 
		InputThread,      // thread proc
		NULL,              // thread parameter 
		0,                 // not suspended 
		&dwThreadInputId);      // returns thread ID 

	if (hInputThread == NULL) 
	{
		dwErr = GetLastError();
		printf("CreateThread(InputThread) failed, ErrCode=0x%08X\n", dwErr); 
		iRc=CERR_CREATEINPUTTHREAD; goto wrap;
	}

	
	
	
	
	lbRc = CreateProcessW(NULL, psNewCmd, NULL,NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
	dwErr = GetLastError();
	if (!lbRc)
	{
		printf ("Can't create process, ErrCode=0x%08X! Command to be executed:\n%s", dwErr, psNewCmd);
		iRc=CERR_CREATEPROCESS; goto wrap;
	}
	delete psNewCmd; psNewCmd = NULL;

	// Ждем, пока в консоли не останется процессов (кроме нашего)
	TODO("Проверить, может ли так получиться, что CreateProcess прошел, а к консоли он не прицепился? Может, если процесс GUI");
	nWait = WaitForSingleObject(hExitEvent, 6*1000); //Запуск процесса наверное может задержать антивирус
	if (nWait != WAIT_OBJECT_0) { // Если таймаут
		EnterCriticalSection(&csProc);
		iRc = nProcesses.size();
		LeaveCriticalSection(&csProc);
		// И процессов в консоли все еще нет
		if (iRc == 0) {
			printf ("Process was not attached to console. Is it GUI?\nCommand to be executed:\n%s", psNewCmd);
			iRc=CERR_PROCESSTIMEOUT; goto wrap;
		}

		// По крайней мере один процесс в консоли запустился. Ждем пока в консоли не останется никого кроме нас
		WaitForSingleObject(hExitEvent, INFINITE);
	}

	iRc = 0;
wrap:
	// Закрываем дескрипторы и выходим
	if (dwWinEventThread && hWinEventThread) {
		PostThreadMessage(dwWinEventThread, WM_QUIT, 0, 0);
		// Подождем немножко, пока нить сама завершится
		if (WaitForSingleObject(hWinEventThread, 500) != WAIT_OBJECT_0) {
			TerminateThread ( hWinEventThread, 100 ); // раз корректно не хочет...
		}
		CloseHandle(hWinEventThread); hWinEventThread = NULL;
	}
	if (hInputThread) {
		TerminateThread ( hInputThread, 100 ); TODO("Сделать корректное завершение");
		CloseHandle(hInputThread); hInputThread = NULL;
	}
	if (pi.hProcess) CloseHandle(pi.hProcess); 
	if (pi.hThread) CloseHandle(pi.hThread);
	if (hThread) {
		TerminateThread ( hThread, 100 ); TODO("Сделать корректное завершение");
		CloseHandle(hThread); hThread = NULL;
	}
	//
	if (hConIn && hConIn!=INVALID_HANDLE_VALUE) {
		CloseHandle(hConIn); hConIn = NULL;
	}
	if (hConOut && hConOut!=INVALID_HANDLE_VALUE) {
		CloseHandle(hConOut); hConIn = NULL;
	}
    if (hWinHook) {
        UnhookWinEvent(hWinHook); hWinHook = NULL;
    }
	if (psNewCmd) { delete psNewCmd; psNewCmd = NULL; }
	if (psChars) { free(psChars); psChars = NULL; }
	if (pnAttrs) { free(pnAttrs); pnAttrs = NULL; }
	DeleteCriticalSection(&csConBuf);
	DeleteCriticalSection(&csProc);

	if (iRc!=0 || bAlwaysConfirmExit) {
		printf("Press any key to close console");
		_getch();
		printf("\n");
	}
    return iRc;
}




DWORD WINAPI ServerThread(LPVOID lpvParam) 
{ 
   BOOL fConnected = FALSE;
   DWORD dwThreadId = 0, dwErr = 0;
   HANDLE hPipe = NULL, hThread = NULL;
   
 
// The main loop creates an instance of the named pipe and 
// then waits for a client to connect to it. When the client 
// connects, a thread is created to handle communications 
// with that client, and the loop is repeated. 
 
   for (;;) 
   { 
      hPipe = CreateNamedPipe( 
          szPipename,               // pipe name 
          PIPE_ACCESS_DUPLEX,       // read/write access 
          PIPE_TYPE_MESSAGE |       // message type pipe 
          PIPE_READMODE_MESSAGE |   // message-read mode 
          PIPE_WAIT,                // blocking mode 
          PIPE_UNLIMITED_INSTANCES, // max. instances  
          PIPEBUFSIZE,              // output buffer size 
          PIPEBUFSIZE,              // input buffer size 
          0,                        // client time-out 
          NULL);                    // default security attribute 

      _ASSERTE(hPipe != INVALID_HANDLE_VALUE);
      
      if (hPipe == INVALID_HANDLE_VALUE) 
      {
	      dwErr = GetLastError();
          printf("CreatePipe failed, ErrCode=0x%08X\n"); 
          Sleep(50);
          //return 99;
          continue;
      }
 
      // Wait for the client to connect; if it succeeds, 
      // the function returns a nonzero value. If the function
      // returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 
 
      fConnected = ConnectNamedPipe(hPipe, NULL) ? 
         TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 
 
      if (fConnected) 
      { 
      // Create a thread for this client. 
         hThread = CreateThread( 
            NULL,              // no security attribute 
            0,                 // default stack size 
            InstanceThread,    // thread proc
            (LPVOID) hPipe,    // thread parameter 
            0,                 // not suspended 
            &dwThreadId);      // returns thread ID 

         if (hThread == NULL) 
         {
	        dwErr = GetLastError();
            printf("CreateThread failed, ErrCode=0x%08X\n"); 
            Sleep(50);
            //return 0;
            continue;
         }
         else CloseHandle(hThread); 
       } 
      else 
        // The client could not connect, so close the pipe. 
         CloseHandle(hPipe); 
   } 
   return 1; 
} 

DWORD WINAPI InputThread(LPVOID lpvParam) 
{ 
   BOOL fConnected, fSuccess; 
   //DWORD dwThreadId;
   HANDLE hPipe = NULL; 
   DWORD dwErr = 0;
   
 
// The main loop creates an instance of the named pipe and 
// then waits for a client to connect to it. When the client 
// connects, a thread is created to handle communications 
// with that client, and the loop is repeated. 
 
   for (;;) 
   { 
      hPipe = CreateNamedPipe( 
          szInputname,              // pipe name 
          PIPE_ACCESS_INBOUND,      // goes from client to server only
          PIPE_TYPE_MESSAGE |       // message type pipe 
          PIPE_READMODE_MESSAGE |   // message-read mode 
          PIPE_WAIT,                // blocking mode 
          PIPE_UNLIMITED_INSTANCES, // max. instances  
          sizeof(INPUT_RECORD),     // output buffer size 
          sizeof(INPUT_RECORD),     // input buffer size 
          0,                        // client time-out
          NULL);                    // default security attribute 

      _ASSERTE(hPipe != INVALID_HANDLE_VALUE);
      
      if (hPipe == INVALID_HANDLE_VALUE) 
      {
	      dwErr = GetLastError();
          printf("CreatePipe failed, ErrCode=0x%08X\n"); 
          Sleep(50);
          //return 99;
          continue;
      }
 
      // Wait for the client to connect; if it succeeds, 
      // the function returns a nonzero value. If the function
      // returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 
 
      fConnected = ConnectNamedPipe(hPipe, NULL) ? 
         TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 
 
      if (fConnected) 
      { 
	      //TODO:
	      DWORD cbBytesRead, cbWritten;
	      INPUT_RECORD iRec; memset(&iRec,0,sizeof(iRec));
	      while (fSuccess = ReadFile( 
	         hPipe,        // handle to pipe 
	         &iRec,        // buffer to receive data 
	         sizeof(iRec), // size of buffer 
	         &cbBytesRead, // number of bytes read 
	         NULL))        // not overlapped I/O 
	      {
		      // предусмотреть возможность завершения нити
		      if (iRec.EventType == 0xFFFF) {
			      CloseHandle(hPipe);
			      break;
		      }
		      if (iRec.EventType) {
		      
			      fSuccess = WriteConsoleInput(hConIn, &iRec, 1, &cbWritten);
			      _ASSERTE(fSuccess && cbWritten==1);
		      }
		      // next
		      memset(&iRec,0,sizeof(iRec));
	      }
      } 
      else 
        // The client could not connect, so close the pipe. 
         CloseHandle(hPipe);
   } 
   return 1; 
} 
 
DWORD WINAPI InstanceThread(LPVOID lpvParam) 
{ 
   CESERVER_REQ in={0}, *pOut=NULL;
   DWORD cbBytesRead, cbWritten; 
   BOOL fSuccess; 
   HANDLE hPipe; 
 
// The thread's parameter is a handle to a pipe instance. 
 
   hPipe = (HANDLE) lpvParam; 
 
   while (1) 
   { 
   // Read client requests from the pipe. 
      memset(&in, 0, sizeof(in));
      fSuccess = ReadFile( 
         hPipe,        // handle to pipe 
         &in,          // buffer to receive data 
         sizeof(in),   // size of buffer 
         &cbBytesRead, // number of bytes read 
         NULL);        // not overlapped I/O 

      if (!fSuccess || cbBytesRead < 8 || in.nSize < 8)
         break;
         
      if (!GetAnswerToRequest(in, &pOut) || pOut==NULL)
	     break;
 
   // Write the reply to the pipe. 
      fSuccess = WriteFile( 
         hPipe,        // handle to pipe 
         pOut,         // buffer to write from 
         pOut->nSize,  // number of bytes to write 
         &cbWritten,   // number of bytes written 
         NULL);        // not overlapped I/O 

      // освободить память
      free(pOut);
         
      if (!fSuccess || pOut->nSize != cbWritten) break; 
  } 
 
// Flush the pipe to allow the client to read the pipe's contents 
// before disconnecting. Then disconnect the pipe, and close the 
// handle to this pipe instance. 
 
   FlushFileBuffers(hPipe); 
   DisconnectNamedPipe(hPipe); 
   CloseHandle(hPipe); 

   return 1;
}

// На результат видимо придется не обращать внимания - не блокировать же выполнение
// сообщениями об ошибках... Возможно, что информацию об ошибке стоит записать
// в сам текстовый буфер.
DWORD ReadConsoleData(CONSOLE_SCREEN_BUFFER_INFO &sbi) // sbi инициализируется вызывающей функцией
{
	BOOL lbRc = TRUE;
	DWORD cbDataSize = 0; // Size in bytes of ONE buffer
	bContentsChanged = FALSE;
	EnterCriticalSection(&csConBuf);

	SHORT TextWidth=0, TextHeight=0;
	DWORD TextLen=0;
	COORD coord;
	
	//TODO: а точно по srWindow ширину нужно смотреть?
	TextWidth = max(sbi.dwSize.X, (sbi.srWindow.Right - sbi.srWindow.Left + 1));
	TextHeight = sbi.srWindow.Bottom - sbi.srWindow.Top + 1;
	TextLen = TextWidth * TextHeight;
	if (TextLen > nBufCharCount) {
		free(psChars);
		psChars = (wchar_t*)calloc(TextLen,sizeof(wchar_t));
		_ASSERTE(psChars!=NULL);
		free(pnAttrs);
		pnAttrs = (WORD*)calloc(TextLen,sizeof(WORD));
		_ASSERTE(pnAttrs!=NULL);
		if (psChars && pnAttrs) {
			nBufCharCount = TextLen;
		} else {
			nBufCharCount = 0; // ошибка выделения памяти
			lbRc = FALSE;
		}
	}
	//TODO: может все-таки из {0,sbi.srWindow.Top} начинать нужно?
	coord.X = sbi.srWindow.Left; coord.Y = sbi.srWindow.Top;

	//TODO: перечитывать содержимое ТОЛЬКО по bContentsChanged
	// ПЕРЕД чтением - сбросить bContentsChanged в FALSE
	// буфер сделать глобальным, 
	// его использование - закрыть через CriticalSection
	
	if (psChars && pnAttrs) {
		//dwAllSize += TextWidth*TextHeight*4;
		
	    // Get attributes (first) and text (second)
	    // [Roman Kuzmin]
	    // In FAR Manager source code this is mentioned as "fucked method". Yes, it is.
	    // Functions ReadConsoleOutput* fail if requested data size exceeds their buffer;
	    // MSDN says 64K is max but it does not say how much actually we can request now.
	    // Experiments show that this limit is floating and it can be much less than 64K.
	    // The solution below is not optimal when a user sets small font and large window,
	    // but it is safe and practically optimal, because most of users set larger fonts
	    // for large window and ReadConsoleOutput works OK. More optimal solution for all
	    // cases is not that difficult to develop but it will be increased complexity and
	    // overhead often for nothing, not sure that we really should use it.
	    DWORD nbActuallyRead;
	    if (!ReadConsoleOutputAttribute(hConOut, pnAttrs, TextLen, coord, &nbActuallyRead) ||
	        !ReadConsoleOutputCharacter(hConOut, psChars, TextLen, coord, &nbActuallyRead))
	    {
		    OutputDebugString(L"!!! Read from console Line-By-Line\n");
		    
	        WORD* ConAttrNow = pnAttrs;
	        wchar_t* ConCharNow = psChars;
	        for(int y = 0; y < (int)TextHeight; ++y)
	        {
	            ReadConsoleOutputAttribute(hConOut, ConAttrNow, TextWidth, coord, &nbActuallyRead);
	            ReadConsoleOutputCharacter(hConOut, ConCharNow, TextWidth, coord, &nbActuallyRead);
	            ConAttrNow += TextWidth;
	            ConCharNow += TextWidth;
	            ++coord.Y;
	        }
	    }

		cbDataSize = TextLen * 2; // Size in bytes of ONE buffer

	} else {
		lbRc = FALSE;
	}

	LeaveCriticalSection(&csConBuf);
	return cbDataSize;
}

BOOL GetAnswerToRequest(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;
	BOOL bContentsChanged = FALSE;

	switch (in.nCmd) {
		case CECMD_GETSHORTINFO:
		case CECMD_GETFULLINFO:
		{
			if (szGuiPipeName[0] == 0) { // Серверный пайп в CVirtualConsole уже должен быть запущен
				wsprintf(szGuiPipeName, CEGUIPIPENAME, L".", dwSelfPID);
			}
			bContentsChanged = (in.nCmd == CECMD_GETFULLINFO);

			_ASSERT(hConOut && hConOut!=INVALID_HANDLE_VALUE);
			if (hConOut==NULL || hConOut==INVALID_HANDLE_VALUE)
				return FALSE;

			*out = CreateConsoleInfo(NULL, bContentsChanged);

			lbRc = TRUE;
		} break;
		case CECMD_SETSIZE:
		{
			//TODO:
		} break;
		case CECMD_WRITEINPUT:
		{
		} break;
	}
	
	return lbRc;
}


DWORD WINAPI WinEventThread(LPVOID lpvParam)
{
	DWORD dwErr = 0;
	HANDLE hStartedEvent = (HANDLE)lpvParam;
	
	// "Ловим" все консольные события
    hWinHook = SetWinEventHook(EVENT_CONSOLE_CARET,EVENT_CONSOLE_END_APPLICATION,
        NULL, WinEventProc, 0,0, WINEVENT_OUTOFCONTEXT /*| WINEVENT_SKIPOWNPROCESS ?*/);
	dwErr = GetLastError();
	if (!hWinHook) {
		dwErr = GetLastError();
		printf("SetWinEventHook failed, ErrCode=0x%08X\n", dwErr); 
		SetEvent(hStartedEvent);
		return 100;
	}
	SetEvent(hStartedEvent); hStartedEvent = NULL; // здесь он более не требуется

    MSG lpMsg;
    while (GetMessage(&lpMsg, NULL, 0, 0))
    {
		TranslateMessage(&lpMsg);
		DispatchMessage(&lpMsg);
    }
    
    // Закрыть хук
    if (hWinHook) {
        UnhookWinEvent(hWinHook); hWinHook = NULL;
    }
    
	return 0;
}

//Minimum supported client Windows 2000 Professional 
//Minimum supported server Windows 2000 Server 
void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
	if (hwnd != hConWnd) {
		_ASSERTE(hwnd); // по идее, тут должен быть хэндл консольного окна, проверим
		return;
	}

	BOOL bNeedConAttrBuf = FALSE;
	CESERVER_CHAR ch = {{0,0}};

    switch(event)
    {
    case EVENT_CONSOLE_START_APPLICATION:
		//A new console process has started. 
		//The idObject parameter contains the process identifier of the newly created process. 
		//If the application is a 16-bit application, the idChild parameter is CONSOLE_APPLICATION_16BIT and idObject is the process identifier of the NTVDM session associated with the console.

		#ifdef _DEBUG
		WCHAR szDbg[128]; wsprintfW(szDbg, L"EVENT_CONSOLE_START_APPLICATION(PID=%i%s)\n", idObject, (idChild == CONSOLE_APPLICATION_16BIT) ? L" 16bit" : L"");
		OutputDebugString(szDbg);
		#endif

		if (idObject != dwSelfPID) {
			EnterCriticalSection(&csProc);
			nProcesses.push_back(idObject);
			LeaveCriticalSection(&csProc);

			if (idChild == CONSOLE_APPLICATION_16BIT) {
				DWORD ntvdmPID = idObject;
				dwActiveFlags |= CES_NTVDM;
				SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
			}
		}
        return; // обновление экрана не требуется

    case EVENT_CONSOLE_END_APPLICATION:
		//A console process has exited. 
		//The idObject parameter contains the process identifier of the terminated process.

		#ifdef _DEBUG
		wsprintfW(szDbg, L"EVENT_CONSOLE_END_APPLICATION(PID=%i%s)\n", idObject, (idChild == CONSOLE_APPLICATION_16BIT) ? L" 16bit" : L"");
		OutputDebugString(szDbg);
		#endif

		if (idObject != dwSelfPID) {
			std::vector<DWORD>::iterator iter;
			EnterCriticalSection(&csProc);
			for (iter=nProcesses.begin(); iter!=nProcesses.end(); iter++) {
				if (idObject == *iter) {
					nProcesses.erase(iter);
					if (idChild == CONSOLE_APPLICATION_16BIT) {
						DWORD ntvdmPID = idObject;
						dwActiveFlags &= ~CES_NTVDM;
						//TODO: возможно стоит прибить процесс NTVDM?
						SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
					}
					// Процессов в консоли не осталось?
					if (nProcesses.size() == 0) {
						LeaveCriticalSection(&csProc);
						SetEvent(hExitEvent);
						return;
					}
					break;
				}
			}
			LeaveCriticalSection(&csProc);
		}
        return; // обновление экрана не требуется

	case EVENT_CONSOLE_UPDATE_REGION: // 0x4002 
		{
		//More than one character has changed.
		//The idObject parameter is a COORD structure that specifies the start of the changed region. 
		//The idChild parameter is a COORD structure that specifies the end of the changed region.
			#ifdef _DEBUG
			COORD crStart, crEnd; memmove(&crStart, &idObject, sizeof(idObject)); memmove(&crEnd, &idChild, sizeof(idChild));
			WCHAR szDbg[128]; wsprintfW(szDbg, L"EVENT_CONSOLE_UPDATE_REGION({%i, %i} - {%i, %i})\n", crStart.X,crStart.Y, crEnd.X,crEnd.Y);
			OutputDebugString(szDbg);
			#endif
			bNeedConAttrBuf = TRUE;
		} break;

	case EVENT_CONSOLE_UPDATE_SCROLL: //0x4004
		{
		//The console has scrolled.
		//The idObject parameter is the horizontal distance the console has scrolled. 
		//The idChild parameter is the vertical distance the console has scrolled.
			#ifdef _DEBUG
			WCHAR szDbg[128]; wsprintfW(szDbg, L"EVENT_CONSOLE_UPDATE_SCROLL(X=%i, Y=%i)\n", idObject, idChild);
			OutputDebugString(szDbg);
			#endif
			bNeedConAttrBuf = TRUE;
		} break;

	case EVENT_CONSOLE_UPDATE_SIMPLE: //0x4003
		{
		//A single character has changed.
		//The idObject parameter is a COORD structure that specifies the character that has changed.
		//Warning! В писании от  микрософта тут ошибка!
		//The idChild parameter specifies the character in the low word and the character attributes in the high word.
			memmove(&ch.crWhere, &idObject, sizeof(idObject));
			ch.ch = (WCHAR)LOWORD(idChild); ch.wA = HIWORD(idChild);
			#ifdef _DEBUG
			WCHAR szDbg[128]; wsprintfW(szDbg, L"EVENT_CONSOLE_UPDATE_SIMPLE({%i, %i} '%c'(\\x%04X) A=%i)\n", ch.crWhere.X,ch.crWhere.Y, ch.ch, ch.ch, ch.wA);
			OutputDebugString(szDbg);
			#endif
		} break;

	case EVENT_CONSOLE_CARET: //0x4001
		{
		//Warning! WinXPSP3. Это событие проходит ТОЛЬКО если консоль в фокусе. 
		//         А с ConEmu она НИКОГДА не в фокусе, так что курсор не обновляется.
		//The console caret has moved.
		//The idObject parameter is one or more of the following values:
		//		CONSOLE_CARET_SELECTION or CONSOLE_CARET_VISIBLE.
		//The idChild parameter is a COORD structure that specifies the cursor's current position.
			#ifdef _DEBUG
			COORD crWhere; memmove(&crWhere, &idChild, sizeof(idChild));
			WCHAR szDbg[128]; wsprintfW(szDbg, L"EVENT_CONSOLE_CARET({%i, %i} Sel=%c, Vis=%c\n", crWhere.X,crWhere.Y, 
				((idObject & CONSOLE_CARET_SELECTION)==CONSOLE_CARET_SELECTION) ? L'Y' : L'N',
				((idObject & CONSOLE_CARET_VISIBLE)==CONSOLE_CARET_VISIBLE) ? L'Y' : L'N');
			OutputDebugString(szDbg);
			#endif
		} break;

	case EVENT_CONSOLE_LAYOUT: //0x4005
		{
		//The console layout has changed.
			#ifdef _DEBUG
			OutputDebugString(L"EVENT_CONSOLE_LAYOUT\n");
			#endif
			bNeedConAttrBuf = TRUE;
		} break;
    }

	// Перечитать размер, положение курсора, и пр.
	ReloadConsoleInfo();

	// Дальнейшее имеет смысл только если CVirtualConsole уже запустил серверный пайп
	if (szGuiPipeName[0] == 0)
		return;

	CESERVER_REQ* pOut = 
		CreateConsoleInfo (
			(event == EVENT_CONSOLE_UPDATE_SIMPLE) ? &ch : NULL,
			bNeedConAttrBuf
		);
	_ASSERTE(pOut!=NULL);
	if (!pOut)
		return;

	//Warning! WinXPSP3. EVENT_CONSOLE_CARET проходит ТОЛЬКО если консоль в фокусе. 
	//         А с ConEmu она НИКОГДА не в фокусе, так что курсор не обновляется.
	// Т.е. изменилось содержимое консоли - уведомить GUI часть
	//if (event != EVENT_CONSOLE_CARET) { // Если изменилось только положение курсора - перечитывать содержимое не нужно
	//	bContentsChanged = TRUE;
	//}
	//SetEvent(hGlblUpdateEvt);

	//WARNING("Все изменения пересылать в GUI через CEGUIPIPENAME");
	//WARNING("Этот пайп обрабатывается в каждом CVirtualConsole по отдельности");

	SendConsoleChanges ( pOut );

	free ( pOut );
}

void SendConsoleChanges(CESERVER_REQ* pOut)
{
	//szGuiPipeName инициализируется только после первого RetrieveConsoleInfo в GUI. Это значит, что там запущена серверная нить
	if (szGuiPipeName[0] == 0)
		return;

	HANDLE hPipe = NULL;
	DWORD dwErr = 0, dwMode = 0;
	BOOL fSuccess = FALSE;

	// Try to open a named pipe; wait for it, if necessary. 
	while (1) 
	{ 
		hPipe = CreateFile( 
			szGuiPipeName,  // pipe name 
			GENERIC_WRITE, 
			0,              // no sharing 
			NULL,           // default security attributes
			OPEN_EXISTING,  // opens existing pipe 
			0,              // default attributes 
			NULL);          // no template file 

		// Break if the pipe handle is valid. 
		if (hPipe != INVALID_HANDLE_VALUE) 
			break; // OK, открыли

		// Exit if an error other than ERROR_PIPE_BUSY occurs. 
		dwErr = GetLastError();
		if (dwErr != ERROR_PIPE_BUSY) 
			return;

		// All pipe instances are busy, so wait for 100 ms.
		if (!WaitNamedPipe(szGuiPipeName, 100) ) 
			return;
	} 

	// The pipe connected; change to message-read mode. 
	dwMode = PIPE_READMODE_MESSAGE; 
	fSuccess = SetNamedPipeHandleState( 
		hPipe,    // pipe handle 
		&dwMode,  // new pipe mode 
		NULL,     // don't set maximum bytes 
		NULL);    // don't set maximum time 
	_ASSERT(fSuccess);
	if (!fSuccess)
		return;


	// Собственно запись в пайп
	DWORD dwWritten = 0;
	fSuccess = WriteFile ( hPipe, pOut, pOut->nSize, &dwWritten, NULL);
	_ASSERTE(fSuccess && dwWritten == pOut->nSize);
}

CESERVER_REQ* CreateConsoleInfo(CESERVER_CHAR* pCharOnly, BOOL bCharAttrBuff)
{
	CESERVER_REQ* pOut = NULL;
	DWORD dwAllSize = sizeof(CESERVER_REQ), nSize;

	// 1
	HWND hWnd = NULL;
	dwAllSize += (nSize=sizeof(hWnd)); _ASSERTE(nSize==4);
	// 2
	// во время чтения содержимого консоли может увеличиться количество процессов
	// поэтому РЕАЛЬНОЕ количество - выставим после чтения и CriticalSection(csProc);
	//EnterCriticalSection(&csProc);
	//DWORD dwProcCount = nProcesses.size()+20;
	//LeaveCriticalSection(&csProc);
	//dwAllSize += sizeof(DWORD)*(dwProcCount+1); // PID процессов + их количество
	dwAllSize += (nSize=sizeof(DWORD)); // список процессов формируется в GUI, так что пока - просто 0 (Reserved)
	// 3
	//DWORD dwSelRc = 0; CONSOLE_SELECTION_INFO sel = {0}; // GetConsoleSelectionInfo
	dwAllSize += sizeof(dwSelRc)+((dwSelRc==0) ? (nSize=sizeof(sel)) : 0);
	// 4
	//DWORD dwCiRc = 0; CONSOLE_CURSOR_INFO ci = {0}; // GetConsoleCursorInfo
	dwAllSize += sizeof(dwCiRc)+((dwCiRc==0) ? (nSize=sizeof(ci)) : 0);
	// 5, 6, 7
	//DWORD dwConsoleCP=0, dwConsoleOutputCP=0, dwConsoleMode=0;
	dwAllSize += 3*sizeof(DWORD);
	// 8
	//DWORD dwSbiRc = 0; CONSOLE_SCREEN_BUFFER_INFO sbi = {{0,0}}; // GetConsoleScreenBufferInfo
	//if (!GetConsoleScreenBufferInfo(hConOut, &sbi)) { dwSbiRc = GetLastError(); if (!dwSbiRc) dwSbiRc = -1; }
	dwAllSize += sizeof(dwSbiRc)+((dwSbiRc==0) ? (nSize=sizeof(sbi)) : 0);
	// 9
	dwAllSize += sizeof(DWORD) + (pCharOnly ? (nSize=sizeof(CESERVER_CHAR)) : 0);
	// 10
	DWORD OneBufferSize = 0;
	dwAllSize += sizeof(DWORD);
	if (bCharAttrBuff) {
		OneBufferSize = ReadConsoleData(sbi); // returns size in bytes of ONE buffer
		if (OneBufferSize) {
			dwAllSize += OneBufferSize * 2;
		}
	}

	// Выделение буфера
	pOut = (CESERVER_REQ*)calloc(dwAllSize+sizeof(CESERVER_REQ), 1); // размер считали в байтах
	_ASSERT(pOut!=NULL);
	if (pOut == NULL) {
		return FALSE;
	}

	// инициализация
	pOut->nSize = dwAllSize;
	pOut->nCmd = bCharAttrBuff ? CECMD_GETFULLINFO : CECMD_GETSHORTINFO;
	pOut->nVersion = CESERVER_REQ_VER;

	// поехали
	LPBYTE lpCur = (LPBYTE)(pOut->Data);

	// 1
	hWnd = GetConsoleWindow();
	_ASSERTE(hWnd == hConWnd);
	*((DWORD*)lpCur) = (DWORD)hWnd;
	lpCur += sizeof(hWnd);

	// 2
	// во время чтения содержимого консоли может увеличиться количество процессов
	// поэтому РЕАЛЬНОЕ количество - выставим после чтения и CriticalSection(csProc);
	*((DWORD*)lpCur) = 0; lpCur += sizeof(DWORD);
	//EnterCriticalSection(&csProc);
	//DWORD dwTestCount = nProcesses.size();
	//_ASSERTE(dwTestCount<=dwProcCount);
	//if (dwTestCount < dwProcCount) dwProcCount = dwTestCount;
	//*((DWORD*)lpCur) = dwProcCount; lpCur += sizeof(DWORD);
	//for (DWORD n=0; n<dwProcCount; n++) {
	//	*((DWORD*)lpCur) = nProcesses[n];
	//	lpCur += sizeof(DWORD);
	//}
	//LeaveCriticalSection(&csProc);

	// 3
	nSize=sizeof(sel); *((DWORD*)lpCur) = (dwSelRc == 0) ? nSize : 0; lpCur += sizeof(DWORD);
	if (dwSelRc == 0) {
		memmove(lpCur, &sel, nSize); lpCur += nSize;
	}

	// 4
	nSize=sizeof(ci); *((DWORD*)lpCur) = (dwCiRc == 0) ? nSize : 0; lpCur += sizeof(DWORD);
	if (dwCiRc == 0) {
		memmove(lpCur, &ci, nSize); lpCur += nSize;
	}

	// 5
	*((DWORD*)lpCur) = dwConsoleCP; lpCur += sizeof(DWORD);
	// 6
	*((DWORD*)lpCur) = dwConsoleOutputCP; lpCur += sizeof(DWORD);
	// 7
	*((DWORD*)lpCur) = dwConsoleMode; lpCur += sizeof(DWORD);

	// 8
	//if (!GetConsoleScreenBufferInfo(hConOut, &sbi)) { dwSbiRc = GetLastError(); if (!dwSbiRc) dwSbiRc = -1; }
	nSize=sizeof(sbi); *((DWORD*)lpCur) = (dwSbiRc == 0) ? nSize : 0; lpCur += sizeof(DWORD);
	if (dwSbiRc == 0) {
		memmove(lpCur, &sbi, nSize); lpCur += nSize;
	}

	// 9
	*((DWORD*)lpCur) = pCharOnly ? sizeof(CESERVER_CHAR) : 0; lpCur += sizeof(DWORD);
	if (pCharOnly) {
		memmove(lpCur, pCharOnly, sizeof(CESERVER_CHAR)); lpCur += (nSize=sizeof(CESERVER_CHAR));
	}

	// 10 - здесь будет 0, если текст в консоли не менялся
	*((DWORD*)lpCur) = OneBufferSize; lpCur += sizeof(DWORD);
	if (OneBufferSize && OneBufferSize!=-1) {
		memmove(lpCur, psChars, OneBufferSize); lpCur += OneBufferSize;
		memmove(lpCur, pnAttrs, OneBufferSize); lpCur += OneBufferSize;
	}

	return pOut;
}

void ReloadConsoleInfo()
{
	if (!GetConsoleSelectionInfo(&sel)) { dwSelRc = GetLastError(); if (!dwSelRc) dwSelRc = -1; } else dwSelRc = 0;

	if (!GetConsoleCursorInfo(hConOut, &ci)) { dwCiRc = GetLastError(); if (!dwCiRc) dwCiRc = -1; } else dwCiRc = 0;

	dwConsoleCP = GetConsoleCP();
	dwConsoleOutputCP = GetConsoleOutputCP();
	dwConsoleMode=0; GetConsoleMode(hConIn, &dwConsoleMode);

	if (!GetConsoleScreenBufferInfo(hConOut, &sbi)) { dwSbiRc = GetLastError(); if (!dwSbiRc) dwSbiRc = -1; } else dwSbiRc = 0;
}
