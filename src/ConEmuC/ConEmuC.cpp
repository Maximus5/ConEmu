#include <Windows.h>
#include <stdio.h>
#include <Shlwapi.h>
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

wchar_t szPipename[MAX_PATH], szInputname[MAX_PATH];
HANDLE  hConIn = NULL;

int main()
{
	wchar_t sComSpec[MAX_PATH];
	wchar_t* psFilePart;
	wchar_t* psCmdLine = GetCommandLineW();
	size_t nCmdLine = lstrlenW(psCmdLine);
	wchar_t* psNewCmd = NULL;
    DWORD dwThreadId, dwPID;
    HANDLE hThread; 
	BOOL bViaCmdExe = TRUE;

#ifdef _DEBUG
	//if (!IsDebuggerPresent()) MessageBox(0,L"Loaded",L"ComEmuC",0);
#endif
	
	if (!psCmdLine)
	{
		printf ("GetCommandLineW failed!\n" );
		return 100;
	}
	if ((psCmdLine = wcsstr(psCmdLine, L"/C")) == NULL)
	{
		printf ("/C argument not found!\n" );
		return 100;
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
			return 101;
		}
		
		nCmdLine += lstrlenW(sComSpec)+10;
	}
	
	psNewCmd = new wchar_t[nCmdLine];
	if (!psNewCmd)
	{
		printf ("Can't allocate %i wchars!\n", nCmdLine);
		return 102;
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
		DWORD dwErr = GetLastError();
		printf("CreateThread failed, ErrCode=0x%08X\n", dwErr); 
		delete psNewCmd; psNewCmd = NULL;
		return 104;
	}

	
	
	PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
	STARTUPINFOW si; memset(&si, 0, sizeof(si));
	
	si.cb = sizeof(si);
	
	BOOL lbRc = CreateProcessW(NULL, psNewCmd, NULL,NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
	DWORD dwErr = GetLastError();
	delete psNewCmd; psNewCmd = NULL;
	if (!lbRc)
	{
		printf ("Can't create process, ErrCode=0x%08X!\n", dwErr);
		return 103;
	}

	// Ожидаем завершения процесса
	WaitForSingleObject(pi.hProcess, INFINITE);

	//TODO: Вообще-то это нужно делать более корректно, и завершаться только когда процессов в консоли более не останется

	// Закрываем дескрипторы и выходим
	CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
	TerminateThread ( hThread, 100 ); //TODO: Сделать корректное завершение
	CloseHandle(hThread);
	//
	if (hConIn) {
		CloseHandle(hConIn); hConIn = NULL;
	}
    return 0;
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
	      DWORD dwErr = GetLastError();
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
	        DWORD dwErr = GetLastError();
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
   DWORD dwThreadId;
   HANDLE hPipe; 
   
 
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
	      DWORD dwErr = GetLastError();
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

BOOL GetAnswerToRequest(CESERVER_REQ& in, CESERVER_REQ** out)
{
	BOOL lbRc = FALSE;
	HANDLE hConOut = NULL;

	switch (in.nCmd) {
		case CECMD_GETFULLINFO:
		{
			DWORD dwAllSize = 0;
			
			hConOut = CreateFile(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
			            0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

			HWND hWnd = GetConsoleWindow();
			dwAllSize += sizeof(hWnd);
			
			DWORD dwSelRc = 0; CONSOLE_SELECTION_INFO sel = {0}; // GetConsoleSelectionInfo
			if (!GetConsoleSelectionInfo(&sel)) { dwSelRc = GetLastError(); if (!dwSelRc) dwSelRc = -1; }
			dwAllSize += sizeof(dwSelRc)+sizeof(sel);
			
			DWORD dwCiRc = 0; CONSOLE_CURSOR_INFO ci = {0}; // GetConsoleCursorInfo
			if (!GetConsoleCursorInfo(hConOut, &ci)) { dwCiRc = GetLastError(); if (!dwCiRc) dwCiRc = -1; }
			dwAllSize += sizeof(dwCiRc)+sizeof(ci);
			
			DWORD dwConsoleCP = GetConsoleCP();
			DWORD dwConsoleOutputCP = GetConsoleOutputCP();
			DWORD dwConsoleMode = GetConsoleMode(hConIn);
			dwAllSize += 3*sizeof(DWORD);
			
			DWORD dwSbiRc = 0; CONSOLE_SCREEN_BUFFER_INFO sbi = {{0,0}}; // GetConsoleScreenBufferInfo
			if (!GetConsoleScreenBufferInfo(hConOut, &sbi)) { dwSbiRc = GetLastError(); if (!dwSbiRc) dwSbiRc = -1; }
			dwAllSize += sizeof(dwSbiRc)+sizeof(sbi);
			
			wchar_t* psChars = NULL;
			WORD* pnAttrs = NULL;
			DWORD dwOutputRc = 0;
			dwAllSize += sizeof(DWORD);
			
			SHORT TextWidth=0, TextHeight=0;
			DWORD TextLen=0;
			COORD coord;
			
			if (dwSbiRc == 0) {
				//TODO: а точно по srWindow ширину нужно смотреть?
				TextWidth = max(csbi.dwSize.X, (csbi.srWindow.Right - csbi.srWindow.Left + 1));
				TextHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
				TextLen = TextWidth * TextHeight;
				//TODO: может все-таки из {0,csbi.srWindow.Top} начинать нужно?
				coord.X = csbi.srWindow.Left; coord.Y = csbi.srWindow.Top;
				
				psChars = (wchar_t*)calloc(TextWidth*TextHeight,sizeof(wchar_t));
				pnAttrs = (WORD*)calloc(TextWidth*TextHeight,sizeof(WORD));
				if (psChars && pnAttrs) {
					dwAllSize += TextWidth*TextHeight*4;
					
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
				} else {
					dwOutputRc = -1;
				}
			}
			
		} break;
		case CECMD_SETSIZE:
		{
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
