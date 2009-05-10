#include <Windows.h>
#include <stdio.h>
#include <Shlwapi.h>
#include <conio.h>
#include <vector>
#include "..\common\common.hpp"

#if defined(__GNUC__)
	//#include "assert.h"
	#define _ASSERTE(x)
#else
	#include <crtdbg.h>
#endif

#define BUFSIZE 4096
 
DWORD WINAPI InstanceThread(LPVOID);
DWORD WINAPI ServerThread(LPVOID lpvParam);
DWORD WINAPI InputThread(LPVOID lpvParam);
BOOL GetAnswerToRequest(CESERVER_REQ& in, CESERVER_REQ** out); 
void WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);

wchar_t szPipename[MAX_PATH], szInputname[MAX_PATH];
HANDLE  hConIn = NULL;
HWINEVENTHOOK hWinHook = NULL;
HWND hConWnd = NULL;
HANDLE hGlblUpdateEvt = NULL;
HANDLE hExitEvent = NULL;
BOOL bAlwaysConfirmExit = FALSE;
BOOL bContentsChanged = TRUE; // Первое чтение параметров должно быть ПОЛНЫМ
CRITICAL_SECTION csConBuf, csProc;
wchar_t* psChars = NULL;
WORD* pnAttrs = NULL;
DWORD nBufCharCount = NULL;

std::vector<DWORD> nProcesses;

//#define CES_NTVDM 0x10 -- common.hpp
DWORD dwActiveFlags = 0;

int main()
{
	int iRc = 100;
	wchar_t sComSpec[MAX_PATH], sName[64];
	wchar_t* psFilePart;
	wchar_t* psCmdLine = GetCommandLineW();
	size_t nCmdLine = lstrlenW(psCmdLine);
	wchar_t* psNewCmd = NULL;
    DWORD dwThreadId, dwPID;
    HANDLE hThread; 
	BOOL bViaCmdExe = TRUE;
	PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
	STARTUPINFOW si; memset(&si, 0, sizeof(si)); si.cb = sizeof(si);
	DWORD dwErr = 0;

	InitializeCriticalSection(&csConBuf);
	InitializeCriticalSection(&csProc);
	hConWnd = GetConsoleWindow();
	if (!hConWnd) {
		dwErr = GetLastError();
		printf("hConWnd==NULL, ErrCode=0x%08X\n", dwErr); 
		iRc=108; goto wrap;
	}
	hExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL); ResetEvent(hExitEvent);
	if (!hExitEvent) {
		dwErr = GetLastError();
		printf("CreateEvent() failed, ErrCode=0x%08X\n", dwErr); 
		iRc=109; goto wrap;
	}
	wsprintfW(sName, CE_NEEDUPDATE, (DWORD)hConWnd);
	hGlblUpdateEvt = CreateEvent(NULL, FALSE, FALSE, sName);
	if (!hGlblUpdateEvt) {
		dwErr = GetLastError();
		printf("CreateEvent(ConEmuNeedUpdate%u) failed, ErrCode=0x%08X\n", (DWORD)hConWnd, dwErr); 
		iRc=110; goto wrap;
	}

#ifdef _DEBUG
	//if (!IsDebuggerPresent()) MessageBox(0,L"Loaded",L"ComEmuC",0);
#endif
	
	if (!psCmdLine)
	{
		printf ("GetCommandLineW failed!\n" );
		iRc=100; goto wrap;
	}
	if ((psCmdLine = wcsstr(psCmdLine, L"/C")) == NULL)
	{
		printf ("/C argument not found!\n" );
		iRc=101; goto wrap;
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
			iRc=102; goto wrap;
		}
		
		nCmdLine += lstrlenW(sComSpec)+10;
	}
	
	psNewCmd = new wchar_t[nCmdLine];
	if (!psNewCmd)
	{
		printf ("Can't allocate %i wchars!\n", nCmdLine);
		iRc=103; goto wrap;
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

	
	dwPID = GetCurrentProcessId();
	wsprintfW(szPipename, CESERVERPIPENAME, L".", dwPID);
	wsprintfW(szInputname, CESERVERINPUTNAME, L".", dwPID);

	hConIn  = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
	            0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hConIn == INVALID_HANDLE_VALUE) {
		dwErr = GetLastError();
		printf("CreateFile(CONIN$) failed, ErrCode=0x%08X\n", dwErr); 
		iRc=107; goto wrap;
	}


    hWinHook = SetWinEventHook(EVENT_CONSOLE_CARET,EVENT_CONSOLE_END_APPLICATION,
        NULL, WinEventProc, 0,0, WINEVENT_OUTOFCONTEXT);
	if (!hWinHook) {
		dwErr = GetLastError();
		printf("SetWinEventHook failed, ErrCode=0x%08X\n", dwErr); 
		iRc=106; goto wrap;
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
		iRc=104; goto wrap;
	}

	
	
	
	
	
	BOOL lbRc = CreateProcessW(NULL, psNewCmd, NULL,NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
	dwErr = GetLastError();
	if (!lbRc)
	{
		printf ("Can't create process, ErrCode=0x%08X! Command to be executed:\n%s", dwErr, psNewCmd);
		iRc=105; goto wrap;
	}
	delete psNewCmd; psNewCmd = NULL;

	// Ожидаем завершения процесса
	WaitForSingleObject(pi.hProcess, INFINITE);

	//TODO: Вообще-то это нужно делать более корректно, и завершаться только когда процессов в консоли более не останется

	iRc = 0;
wrap:
	// Закрываем дескрипторы и выходим
	if (pi.hProcess) CloseHandle(pi.hProcess); 
	if (pi.hThread) CloseHandle(pi.hThread);
	if (hThread) {
		TerminateThread ( hThread, 100 ); //TODO: Сделать корректное завершение
		CloseHandle(hThread);
	}
	//
	if (hConIn) {
		CloseHandle(hConIn); hConIn = NULL;
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
   BOOL fConnected; 
   DWORD dwThreadId;
   HANDLE hPipe, hThread; 
   
 
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
          BUFSIZE,                  // output buffer size 
          BUFSIZE,                  // input buffer size 
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
DWORD ReadConsoleData(HANDLE hConOut, CONSOLE_SCREEN_BUFFER_INFO &sbi) // sbi инициализируется вызывающей функцией
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
	HANDLE hConOut = NULL;

	switch (in.nCmd) {
		case CECMD_GETFULLINFO:
		{
			hConOut = CreateFile(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
			            0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
			_ASSERT(hConOut!=INVALID_HANDLE_VALUE);
			if (hConOut==INVALID_HANDLE_VALUE)
				return FALSE;

			DWORD dwAllSize = sizeof(CESERVER_REQ);

			// 1
			HWND hWnd = NULL;
			dwAllSize += sizeof(hWnd);
			// 2
			// во время чтения содержимого консоли может увеличиться количество процессов
			// поэтому РЕАЛЬНОЕ количество - выставим после чтения и CriticalSection(csProc);
			EnterCriticalSection(&csProc);
			DWORD dwProcCount = nProcesses.size()+20;
			LeaveCriticalSection(&csProc);
			dwAllSize += sizeof(DWORD)*(dwProcCount+1); // PID процессов + их количество
			// 3
			DWORD dwSelRc = 0; CONSOLE_SELECTION_INFO sel = {0}; // GetConsoleSelectionInfo
			dwAllSize += sizeof(dwSelRc)+sizeof(sel);
			// 4
			DWORD dwCiRc = 0; CONSOLE_CURSOR_INFO ci = {0}; // GetConsoleCursorInfo
			dwAllSize += sizeof(dwCiRc)+sizeof(ci);
			// 5, 6, 7
			DWORD dwConsoleCP=0, dwConsoleOutputCP=0, dwConsoleMode=0;
			dwAllSize += 3*sizeof(DWORD);
			// 8
			DWORD dwSbiRc = 0; CONSOLE_SCREEN_BUFFER_INFO sbi = {{0,0}}; // GetConsoleScreenBufferInfo
			dwAllSize += sizeof(dwSbiRc)+sizeof(sbi);
			// 9
			DWORD OneBufferSize = 0;
			dwAllSize += sizeof(DWORD);
			if (bContentsChanged) {
				bContentsChanged = FALSE;
				OneBufferSize = ReadConsoleData(hConOut, sbi); // returns size in bytes of ONE buffer
				if (OneBufferSize) {
					dwAllSize += OneBufferSize * 2;
				} else {
					OneBufferSize = -1; // ошибка
				}
			}

			// Выделение буфера
			*out = (CESERVER_REQ*)calloc(dwAllSize+sizeof(CESERVER_REQ), 1); // размер считали в байтах
			_ASSERT((*out)!=NULL);
			if ((*out) == NULL) {
				CloseHandle(hConOut); hConOut = NULL;
				return FALSE;
			}

			// инициализация
			(*out)->nSize = dwAllSize;
			(*out)->nCmd = in.nCmd;
			(*out)->nVersion = CESERVER_REQ_VER;

			// поехали
			LPBYTE lpCur = (LPBYTE)(*out)->Data;

			// 1
			hWnd = GetConsoleWindow();
			_ASSERTE(hWnd == hConWnd);
			*((DWORD*)lpCur) = (DWORD)hWnd;
			lpCur += sizeof(hWnd);

			// 2
			// во время чтения содержимого консоли может увеличиться количество процессов
			// поэтому РЕАЛЬНОЕ количество - выставим после чтения и CriticalSection(csProc);
			EnterCriticalSection(&csProc);
			DWORD dwTestCount = nProcesses.size();
			_ASSERTE(dwTestCount<=dwProcCount);
			if (dwTestCount < dwProcCount) dwProcCount = dwTestCount;
			*((DWORD*)lpCur) = dwProcCount; lpCur += sizeof(DWORD);
			for (DWORD n=0; n<dwProcCount; n++) {
				*((DWORD*)lpCur) = nProcesses[n];
				lpCur += sizeof(DWORD);
			}
			LeaveCriticalSection(&csProc);

			// 3
			if (!GetConsoleSelectionInfo(&sel)) { dwSelRc = GetLastError(); if (!dwSelRc) dwSelRc = -1; }
			*((DWORD*)lpCur) = dwSelRc; lpCur += sizeof(DWORD);
			memmove(lpCur, &sel, sizeof(sel)); lpCur += sizeof(sel);

			// 4
			if (!GetConsoleCursorInfo(hConOut, &ci)) { dwCiRc = GetLastError(); if (!dwCiRc) dwCiRc = -1; }
			*((DWORD*)lpCur) = dwCiRc; lpCur += sizeof(DWORD);
			memmove(lpCur, &ci, sizeof(ci)); lpCur += sizeof(ci);

			// 5
			dwConsoleCP = GetConsoleCP();
			*((DWORD*)lpCur) = dwConsoleCP; lpCur += sizeof(DWORD);
			// 6
			dwConsoleOutputCP = GetConsoleOutputCP();
			*((DWORD*)lpCur) = dwConsoleOutputCP; lpCur += sizeof(DWORD);
			// 7
			DWORD dwConsoleMode=0; GetConsoleMode(hConIn, &dwConsoleMode);
			*((DWORD*)lpCur) = dwConsoleMode; lpCur += sizeof(DWORD);

			// 8
			if (!GetConsoleScreenBufferInfo(hConOut, &sbi)) { dwSbiRc = GetLastError(); if (!dwSbiRc) dwSbiRc = -1; }
			*((DWORD*)lpCur) = dwSbiRc; lpCur += sizeof(DWORD);
			memmove(lpCur, &sbi, sizeof(sbi));

			// 9 - здесь будет 0, если текст в консоли не менялся
			*((DWORD*)lpCur) = OneBufferSize; lpCur += sizeof(DWORD);
			if (OneBufferSize && OneBufferSize!=-1) {
				memmove(lpCur, psChars, OneBufferSize); lpCur += OneBufferSize;
				memmove(lpCur, pnAttrs, OneBufferSize); lpCur += OneBufferSize;
			}

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
	
	if (hConOut) {
		CloseHandle(hConOut); hConOut = NULL;
	}
	
	return lbRc;
}

//Minimum supported client Windows 2000 Professional 
//Minimum supported server Windows 2000 Server 
void WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
	if (hwnd != hConWnd) {
		_ASSERTE(hwnd); // по идее, тут должен быть хэндл консольного окна, проверим
		return;
	}

    switch(event)
    {
    case EVENT_CONSOLE_START_APPLICATION:
		//A new console process has started. 
		//The idObject parameter contains the process identifier of the newly created process. 
		//If the application is a 16-bit application, the idChild parameter is CONSOLE_APPLICATION_16BIT and idObject is the process identifier of the NTVDM session associated with the console.

		EnterCriticalSection(&csProc);
		nProcesses.push_back(idObject);
		LeaveCriticalSection(&csProc);

        if (idChild == CONSOLE_APPLICATION_16BIT) {
            DWORD ntvdmPID = idObject;
			dwActiveFlags |= CES_NTVDM;
			SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
        }
        return; // обновление экрана не требуется
    case EVENT_CONSOLE_END_APPLICATION:
		//A console process has exited. 
		//The idObject parameter contains the process identifier of the terminated process.

		{
			std::vector<DWORD>::iterator iter;
			EnterCriticalSection(&csProc);
			for (iter=nProcesses.begin(); iter!=nProcesses.end(); iter++) {
				if (idObject == *iter) {
					nProcesses.erase(iter);
					if (idChild == CONSOLE_APPLICATION_16BIT)
						dwActiveFlags &= ~CES_NTVDM;
					DWORD ntvdmPID = idObject;
					//TODO: возможно стоит прибить процесс NTVDM?
					SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
					// Процессов в консоли не осталось?
					SetEvent(hExitEvent);
					break;
				}
			}
			LeaveCriticalSection(&csProc);
		}
        return; // обновление экрана не требуется
    }
	/*
	EVENT_CONSOLE_UPDATE_REGION 
0x4002 More than one character has changed. The idObject parameter is a COORD structure that specifies the start of the changed region. The idChild parameter is a COORD structure that specifies the end of the changed region.
	EVENT_CONSOLE_UPDATE_SCROLL 
0x4004 The console has scrolled. The idObject parameter is the horizontal distance the console has scrolled. The idChild parameter is the vertical distance the console has scrolled.
	EVENT_CONSOLE_UPDATE_SIMPLE 
0x4003 A single character has changed. The idObject parameter is a COORD structure that specifies the character that has changed. The idChild parameter specifies the character in the high word and the character attributes in the low word.
	EVENT_CONSOLE_CARET 
0x4001 The console caret has moved. The idObject parameter is one or more of the following values: CONSOLE_CARET_SELECTION or CONSOLE_CARET_VISIBLE.
The idChild parameter is a COORD structure that specifies the cursor's current position.
	EVENT_CONSOLE_LAYOUT 
0x4005 The console layout has changed.
	*/
	// Т.е. изменилось содержимое консоли - уведомить GUI часть
	if (event != EVENT_CONSOLE_CARET) { // Если изменилось только положение курсора - перечитывать содержимое не нужно
		bContentsChanged = TRUE;
	}
	SetEvent(hGlblUpdateEvt);
}
