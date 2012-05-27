
/*
Copyright (c) 2011 Maximus5
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

#pragma once

#ifndef PIPEBUFSIZE
#define PIPEBUFSIZE 4096
#endif

#undef _GetTime
#if !defined(_DEBUG)
	#define _GetTime GetTickCount
	//#pragma message("--PipeServer.h in Release mode")
#elif defined(__GNUC__)
	#define _GetTime GetTickCount
	//#pragma message("--PipeServer.h in __GNUC__ mode")
#else
	#define _GetTime timeGetTime
	#pragma comment(lib, "winmm.lib")
	//#pragma message("--PipeServer.h in _DEBUG mode")
#endif

// struct - для того, чтобы выделять память простым calloc. "new" не прокатывает, т.к. Crt может быть не инициализирован (ConEmuHk).

// Класс предполагает, что первый DWORD в (T*) это размер передаваемого блока в байтах!
template <class T>
struct PipeServer
{
	protected:
		enum PipeState
		{
			STARTING_STATE = 0,
			STARTED_STATE,
			CREATEPIPE_STATE,
			PIPECREATED_STATE,
			CONNECTING_STATE,
			WAITCLIENT_STATE,
			READING_STATE,
			WRITING_STATE,
			DISCONNECTING_STATE,
			DISCONNECTED_STATE,
			CLOSING_STATE,
			CLOSED_STATE,
			TERMINATED_STATE,
		};
		struct PipeInst
		{
			// Common
			HANDLE hPipeInst; // Pipe Handle

			DWORD  dwConnErr;
			DWORD  dwConnWait;

			BOOL   bDelayedWrite;
			BOOL   fWriteSuccess;
			DWORD  dwWriteErr;

			DWORD  nTerminateDelay, nTerminateDelay1, nTerminateDelay2, nTerminateDelay3;

			BOOL   bBreakConnection;
			
			HANDLE hEvent; // Overlapped event
			OVERLAPPED oOverlap;
			BOOL fPendingIO;
			PipeState dwState;
			
			wchar_t sErrorMsg[128];

			#ifdef _DEBUG
			int nCreateCount, nConnectCount, nCallCount, nAnswCount;
			SYSTEMTIME stLastCall, stLastEndCall;
			#endif
			
			// Request (from client)
			DWORD cbReadSize, cbMaxReadSize;
			T* ptrRequest;
			T* AllocRequestBuf(DWORD anSize)
			{
				if (ptrRequest && (cbMaxReadSize >= anSize))
					return ptrRequest;

				if (ptrRequest)
					free(ptrRequest);

				ptrRequest = (T*)malloc(anSize);
				cbMaxReadSize = anSize;
				return ptrRequest;
			};
			// Answer (to client)
			DWORD cbReplySize, cbMaxReplySize;
			T* ptrReply; // !!! Память выделяется в mfn_PipeServerCommand
			//T* AllocReplyBuf(DWORD anSize)
			//{
			//	if (ptrReply && (cbMaxReplySize >= anSize))
			//		return ptrReply;
			//	if (ptrReply)
			//		free(ptrReply);
			//	ptrReply = (T*)malloc(anSize);
			//	cbMaxReplySize = anSize;
			//	return ptrReply;
			//};
			//// Event descriptors
			//HANDLE hTermEvent;
			//HANDLE hServerSemaphore;

			// Pointer to object
			PipeServer *pServer;

			// Server thread.
			HANDLE hThread;
			DWORD nThreadId;
			HANDLE hThreadEnd;
		};
	protected:
		bool mb_Initialized;
		bool mb_Terminate;
		bool mb_Overlapped;
		bool mb_LoopCommands;
		bool mb_InputOnly;
		int  mn_Priority;
		wchar_t ms_PipeName[MAX_PATH];
		int mn_MaxCount;
		int mn_PipesToCreate;
		int mn_DummyAnswerSize;
		PipeInst *m_Pipes/*[MaxCount]*/;
		PipeInst *mp_ActivePipe;
		
		LPSECURITY_ATTRIBUTES mlp_Sec;
		
		// *** Callback function and it's LParam ***
		
		// Сервером получена команда. Выполнить, вернуть результат.
		// Для облегчения жизни - сервер кеширует данные, калбэк может использовать ту же память (*pcbMaxReplySize)
		typedef BOOL (WINAPI* PipeServerCommand_t)(LPVOID pInst, T* pCmd, T* &ppReply, DWORD &pcbReplySize, DWORD &pcbMaxReplySize, LPARAM lParam);
		PipeServerCommand_t mfn_PipeServerCommand;
		
		// Вызывается после подключения клиента к серверному пайпу, но перед PipeServerRead и PipeServerCommand_t
		// (которых может быть много на одном подключении)
		typedef BOOL (WINAPI* PipeServerConnected_t)(LPVOID pInst, LPARAM lParam);
		PipeServerConnected_t mfn_PipeServerConnected;
		// Уведомление, что клиент отвалился (или его отавалили). Вызывается только если был mfn_PipeServerConnected
		PipeServerConnected_t mfn_PipeServerDisconnected;
		
		// Вызывается после того, как создан Pipe Instance (CreateNamedPipeW)
		typedef BOOL (WINAPI* PipeServerReady_t)(LPVOID pInst, LPARAM lParam);
		PipeServerReady_t mfn_PipeServerReady;
		
		// освободить память, отведенную под результат
		typedef void (WINAPI* PipeServerFree_t)(T* pReply, LPARAM lParam);
		PipeServerFree_t mfn_PipeServerFree;
		
		// Params
		BOOL mb_ReadyCalled;
		LPARAM m_lParam; // указан при StartPipeServer, передается в калбэки
	protected:
		// Event descriptors
		HANDLE mh_TermEvent;
		//BOOL   mb_Terminate;
		HANDLE mh_ServerSemaphore;

		void DumpError(PipeInst *pPipe, LPCWSTR asFormat, BOOL abShow = TRUE)
		{
			wchar_t sTitle[128], sText[MAX_PATH*2];
			DWORD dwErr = GetLastError();
			msprintf(pPipe->sErrorMsg, countof(pPipe->sErrorMsg), asFormat/*i.e. L"ConnectNamedPipe failed with 0x%08X."*/, dwErr);

			if (abShow)
			{
				msprintf(sText, countof(sText), L"%s\n%s", pPipe->sErrorMsg, ms_PipeName);
				msprintf(sTitle, countof(sTitle), L"PipeServerError, PID=%u, TID=%u",
				                GetCurrentProcessId(), GetCurrentThreadId());
				GuiMessageBox(NULL, sText, sTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
			}
			else
			{
				msprintf(sText, countof(sText), L"PipeServerError, PID=%u, TID=%u\n  %s\n  %s\n",
				                GetCurrentProcessId(), GetCurrentThreadId(),
				                pPipe->sErrorMsg, ms_PipeName);
				OutputDebugStringW(sText);
			}
		}

		int WaitOverlapped(PipeInst* pPipe, DWORD* pcbDataSize)
		{
			BOOL fSuccess = FALSE;
			// Wait for overlapped connection
			HANDLE hConnWait[2] = {mh_TermEvent, pPipe->hEvent};
			DWORD nWait = (DWORD)-1, dwErr = (DWORD)-1, dwErrT = (DWORD)-1;
			DWORD nTimeout = RELEASEDEBUGTEST(1000,30000);
			while (!mb_Terminate)
			{
				nWait = WaitForMultipleObjects(2, hConnWait, FALSE, nTimeout);
				if (nWait == WAIT_OBJECT_0)
				{
					_ASSERTEX(mb_Terminate==true);
					DWORD t1 = _GetTime();
					TODO("Подозрение на длительную обработку");
					pPipe->dwState = DISCONNECTING_STATE;
					DisconnectNamedPipe(pPipe->hPipeInst);
					pPipe->dwState = DISCONNECTED_STATE;
					DWORD t2 = _GetTime();
					pPipe->nTerminateDelay1 = (t2 - t1);
					pPipe->nTerminateDelay += pPipe->nTerminateDelay1;
					return 0; // Завершение сервера
				}
				else if (nWait == (WAIT_OBJECT_0+1))
				{
					fSuccess = GetOverlappedResult(pPipe->hPipeInst, &pPipe->oOverlap, pcbDataSize, FALSE);
					dwErr = GetLastError();

					if (fSuccess || dwErr == ERROR_MORE_DATA)
						return 1; // OK, клиент подключился

					#ifdef _DEBUG
					_ASSERTEX(dwErr==ERROR_BROKEN_PIPE); // Канал был закрыт?
					SetLastError(dwErr);
					#endif
					return 2; // Требуется переподключение
				}
				else if (nWait == WAIT_TIMEOUT)
				{
					fSuccess = GetOverlappedResult(pPipe->hPipeInst, &pPipe->oOverlap, pcbDataSize, FALSE);
					dwErrT = GetLastError();

					if (fSuccess || dwErrT == ERROR_MORE_DATA)
						return 1; // OK, клиент подключился

					if (dwErrT == ERROR_IO_INCOMPLETE)
						continue;

					_ASSERTEX(dwErrT == ERROR_IO_INCOMPLETE);
					DumpError(pPipe, L"PipeServerThread:GetOverlappedResult failed with 0x%08X", FALSE);
				}
				else
				{
					_ASSERTEX(nWait == 0 || nWait == 1);
					DumpError(pPipe, L"PipeServerThread:WaitForMultipleObjects failed with 0x%08X", FALSE);
				}

				// Force client disconnect (error on server side)
				pPipe->dwState = DISCONNECTING_STATE;
				DisconnectNamedPipe(pPipe->hPipeInst);
				pPipe->dwState = DISCONNECTED_STATE;
				// Wait a little
				if (!mb_Terminate)
					Sleep(10);
				// Next
				pPipe->dwState = CLOSING_STATE;
				CloseHandle(pPipe->hPipeInst);
				pPipe->hPipeInst = NULL;
				pPipe->dwState = CLOSED_STATE;
				return 2; // ошибка, пробуем еще раз
			}

			return 2; // Terminate!
		}

		BOOL PipeServerRead(PipeInst* pPipe)
		{
			if (mb_Overlapped)
				ResetEvent(pPipe->hEvent);
			
			int nOverRc;
			DWORD cbRead = 0, dwErr = 0, cbWholeSize = 0;
			BOOL fSuccess = FALSE;
			DWORD In[32];
			T *pIn = NULL;
			SetLastError(0);
			// Send a message to the pipe server and read the response.
			fSuccess = ReadFile(
			               pPipe->hPipeInst, // pipe handle
			               In,               // buffer to receive reply
			               sizeof(In),       // size of read buffer
			               &cbRead,          // bytes read
			               mb_Overlapped ? &pPipe->oOverlap : NULL);
			dwErr = GetLastError();

			if (mb_Overlapped)
			{
				if (!fSuccess || !cbRead)
				{
					nOverRc = WaitOverlapped(pPipe, &cbRead);
					if (nOverRc == 1)
					{
						dwErr = GetLastError();
						fSuccess = TRUE;
					}
					else if (nOverRc == 2)
					{
						BreakConnection(pPipe);
						return FALSE;
					}
					else
					{
						_ASSERTEX(mb_Terminate);
						return FALSE; // terminate
					}
				}
			}
			
			if ((!fSuccess && (dwErr != ERROR_MORE_DATA)) || (cbRead < sizeof(DWORD)) || mb_Terminate)
			{
				// Сервер закрывается?
				//DEBUGSTRPROC(L"!!! ReadFile(pipe) failed - console in close?\n");
				return FALSE;
			}

			//if (in.hdr.nVersion != CESERVER_REQ_VER)
			//{
			//	gConEmu.ShowOldCmdVersion(in.hdr.nCmd, in.hdr.nVersion, in.hdr.nSrcPID==GetServerPID() ? 1 : 0);
			//	return FALSE;
			//}
			//_ASSERTEX(in.hdr.cbSize>=sizeof(CESERVER_REQ_HDR) && cbRead>=sizeof(CESERVER_REQ_HDR));
			//if (cbRead < sizeof(CESERVER_REQ_HDR) || /*in.hdr.cbSize < cbRead ||*/ in.hdr.nVersion != CESERVER_REQ_VER) {
			//	//CloseHandle(hPipe);
			//	return;
			//}
			
			cbWholeSize = In[0];

			if (cbWholeSize <= cbRead)
			{
				_ASSERTEX(cbWholeSize >= cbRead);
				pIn = pPipe->AllocRequestBuf(cbRead);
				if (!pIn)
				{
					_ASSERTEX(pIn!=NULL);
					return FALSE;
				}
				memmove(pIn, In, cbRead);
				pPipe->cbReadSize = cbRead;
			}
			else
			{
				int nAllSize = cbWholeSize;
				pIn = pPipe->AllocRequestBuf(cbWholeSize);
				if (!pIn)
				{
					_ASSERTEX(pIn!=NULL);
					return FALSE;
				}
				memmove(pIn, In, cbRead);
				LPBYTE ptrData = ((LPBYTE)pIn)+cbRead;
				nAllSize -= cbRead;

				while (nAllSize>0)
				{
					if (mb_Overlapped)
						ResetEvent(pPipe->hEvent);

					SetLastError(0);
					// Read from the pipe if there is more data in the message.
					fSuccess = ReadFile(
					               pPipe->hPipeInst, // pipe handle
					               ptrData,          // buffer to receive reply
					               nAllSize,         // size of buffer
					               &cbRead,          // number of bytes read
					               mb_Overlapped ? &pPipe->oOverlap : NULL);
					dwErr = GetLastError();

					if (mb_Overlapped)
					{
						if (!fSuccess || !cbRead)
						{
							nOverRc = WaitOverlapped(pPipe, &cbRead);
							if (nOverRc == 1)
							{
								dwErr = GetLastError();
								fSuccess = TRUE;
							}
							else
							{
								_ASSERTEX(mb_Terminate);
								return FALSE; // terminate
							}
						}
					}
					
					// Exit if an error other than ERROR_MORE_DATA occurs.
					if (!fSuccess && (dwErr != ERROR_MORE_DATA))
						break;

					ptrData += cbRead;
					nAllSize -= cbRead;
				}

				TODO("Mozhet vozniknutj ASSERT, esli konsolj byla zakryta v processe chtenija");

				if (nAllSize>0)
				{
					_ASSERTEX(nAllSize==0);
					return FALSE; // удалось считать не все данные
				}

				pPipe->cbReadSize = (DWORD)(ptrData - ((LPBYTE)pIn));
			}

			return TRUE;
		}
		
		int PipeServerWrite(PipeInst* pPipe, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, BOOL bDelayed=FALSE)
		{
			if (pPipe->bBreakConnection)
				return 0;

			BOOL fWriteSuccess = FALSE, fSuccess;
			DWORD dwErr = 0;

			if (mb_Overlapped)
				ResetEvent(pPipe->hEvent);

			pPipe->bDelayedWrite = bDelayed;
			
			if (!bDelayed || lpBuffer)
			{
				pPipe->fWriteSuccess = fWriteSuccess = WriteFile(pPipe->hPipeInst, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, mb_Overlapped ? &pPipe->oOverlap : NULL);

				// The write operation is still pending?
				pPipe->dwWriteErr = dwErr = GetLastError();
			}
			else if (bDelayed)
			{
				fWriteSuccess = pPipe->fWriteSuccess;
				dwErr = pPipe->dwWriteErr;
			}
			

			if (mb_Overlapped)
			{
				if (!fWriteSuccess && (dwErr == ERROR_IO_PENDING))
				{
					if (bDelayed)
						return 1;

					int nOverRc = WaitOverlapped(pPipe, lpNumberOfBytesWritten);
					if (nOverRc == 0)
						return 0;
					else if (nOverRc == 1)
						return 1;
					else
						return 2;
				}
				else if (fWriteSuccess)
				{
					fSuccess = GetOverlappedResult(pPipe->hPipeInst, &pPipe->oOverlap, lpNumberOfBytesWritten, FALSE);
					_ASSERTEX(fSuccess);
					return fSuccess ? 1 : 2;
				}
				else
				{
					return 2;
				}
			}
			else
			{
				return mb_Terminate ? 0 : fWriteSuccess ? 1 : 2;
			}
		}

		bool WaitForClient(PipeInst* pPipe, BOOL& fConnected, BOOL& bNeedReply)
		{
			bool   lbRc = false;
			DWORD  cbOver = 0;
			HANDLE hWait[2] = {mh_TermEvent,mh_ServerSemaphore};

			// Цикл ожидания подключения
			while (!mb_Terminate)
			{
				// !!! Переносить проверку семафора ПОСЛЕ CreateNamedPipe нельзя, т.к. в этом случае
				//     нити дерутся и клиент не может подцепиться к серверу
				// Дождаться разрешения семафора, или завершения сервера
				pPipe->dwConnWait = WaitForMultipleObjects(2, hWait, FALSE, INFINITE);

				if (pPipe->dwConnWait == WAIT_OBJECT_0)
				{
					_ASSERTEX(mb_Terminate==true);
					break; // Сервер закрывается
				}

				if (mb_Overlapped)
				{
					ResetEvent(pPipe->hEvent);
				}

				_ASSERTEX(pPipe->dwConnWait == (WAIT_OBJECT_0+1));
				mp_ActivePipe = pPipe;

				// Пайп уже может быть создан
				if ((pPipe->hPipeInst == NULL) || (pPipe->hPipeInst == INVALID_HANDLE_VALUE))
				{
					// На WinXP наблюдался странный глючок при MaxCount==1, возможно из-за того, что не вызывался DisconnectNamedPipe
					int InstanceCount = RELEASEDEBUGTEST(PIPE_UNLIMITED_INSTANCES,mn_MaxCount);
					DWORD nOutSize = PIPEBUFSIZE;
					DWORD nInSize = PIPEBUFSIZE;

					DWORD dwOpenMode = 
						(mb_InputOnly ? PIPE_ACCESS_INBOUND : PIPE_ACCESS_DUPLEX) |
						(mb_Overlapped ? FILE_FLAG_OVERLAPPED : 0); // overlapped mode?

					DWORD dwPipeMode =
						PIPE_TYPE_MESSAGE |         // message type pipe
						PIPE_READMODE_MESSAGE |     // message-read mode
						PIPE_WAIT;                  // Blocking mode is enabled.

					_ASSERTEX(mlp_Sec!=NULL);
					pPipe->dwState = CREATEPIPE_STATE;
					pPipe->hPipeInst = CreateNamedPipeW(ms_PipeName, dwOpenMode, dwPipeMode, InstanceCount, nOutSize, nInSize, 0, mlp_Sec);
					pPipe->dwState = PIPECREATED_STATE;

					// Сервер закрывается?
					if (mb_Terminate)
					{
						if (mb_Overlapped)
							ReleaseSemaphore(hWait[1], 1, NULL);
						break;
					}

					if ((pPipe->hPipeInst == INVALID_HANDLE_VALUE) || (pPipe->hPipeInst == NULL))
					{
						pPipe->dwConnErr = GetLastError();

						_ASSERTEX(pPipe->hPipeInst != INVALID_HANDLE_VALUE);
						DumpError(pPipe, L"PipeServerThread:ConnectNamedPipe failed with 0x%08X", FALSE);

						//DisplayLastError(L"CreateNamedPipe failed");
						pPipe->hPipeInst = NULL;
						// Wait a little
						Sleep(10);
						// Разрешить этой или другой нити создать серверный пайп
						ReleaseSemaphore(hWait[1], 1, NULL);
						// Try again
						continue;
					}

					DEBUGTEST(pPipe->nCreateCount++);
				}

				// Чтобы ConEmuC знал, что серверный пайп готов
				if (!mb_ReadyCalled && mfn_PipeServerReady)
				{
					mb_ReadyCalled = TRUE;
					mfn_PipeServerReady(pPipe, m_lParam);
					//SetEvent(pRCon->mh_GuiAttached);
					//SafeCloseHandle(pRCon->mh_GuiAttached);
				}

				_ASSERTEX(pPipe->hPipeInst && (pPipe->hPipeInst!=INVALID_HANDLE_VALUE));
				// Wait for the client to connect; if it succeeds,
				// the function returns a nonzero value. If the function
				// returns zero, GetLastError returns ERROR_PIPE_CONNECTED.
				SetLastError(0);
				pPipe->dwState = CONNECTING_STATE;
				fConnected = ConnectNamedPipe(pPipe->hPipeInst, mb_Overlapped ? &pPipe->oOverlap : NULL);
				pPipe->dwConnErr = GetLastError();
				//? TRUE : ((dwErr = GetLastError()) == ERROR_PIPE_CONNECTED);
				// сразу разрешить другой нити принять вызов
				if (!mb_Overlapped)
					ReleaseSemaphore(hWait[1], 1, NULL);

				bool lbFail = false;
				if (mb_Overlapped)
				{
					// Overlapped ConnectNamedPipe should return zero.
					if (fConnected)
					{
						DumpError(pPipe, L"PipeServerThread:ConnectNamedPipe failed with 0x%08X", FALSE);
						Sleep(100);
						ReleaseSemaphore(hWait[1], 1, NULL);
						continue;
					}
					bNeedReply = (pPipe->dwConnErr == ERROR_PIPE_CONNECTED);
				}
				else
				{
					if (!fConnected && pPipe->dwConnErr == ERROR_PIPE_CONNECTED)
						fConnected = TRUE;
					bNeedReply = fConnected;
				}

				// Сервер закрывается?
				if (mb_Terminate)
				{
					if (mb_Overlapped)
						ReleaseSemaphore(hWait[1], 1, NULL);
					break;
				}

				// Обработка подключения
				if (mb_Overlapped)
				{
					// Wait for overlapped connection
					_ASSERTEX(pPipe->hPipeInst && (pPipe->hPipeInst!=INVALID_HANDLE_VALUE));
					int nOverRc = (pPipe->dwConnErr == ERROR_PIPE_CONNECTED) ? 1 : WaitOverlapped(pPipe, &cbOver);
					
					DWORD t1 = _GetTime();
					ReleaseSemaphore(hWait[1], 1, NULL);
					DWORD t2 = _GetTime();

					if (nOverRc == 0)
					{
						_ASSERTEX(mb_Terminate==true);
						pPipe->nTerminateDelay2 = (t2 - t1);
						pPipe->nTerminateDelay += pPipe->nTerminateDelay2;
						break; // Завершение сервера
					}
					else if (nOverRc == 1)
					{
						// OK, клиент подключился
						lbRc = true;
						bNeedReply = TRUE;
						break;
					}
					else
					{
						// Проверить, какие коды ошибок? Может быть нужно CloseHandle дернуть?
						_ASSERTEX(nOverRc==1);
						continue; // ошибка, пробуем еще раз
					}
				} // end - mb_Overlapped
				else
				{
					if (fConnected)
					{
						// OK, Выйти из цикла ожидания подключения, перейти к чтению
						lbRc = true;
						break;
					}
					else
					{
						pPipe->dwState = DISCONNECTING_STATE;
						DisconnectNamedPipe(pPipe->hPipeInst);
						pPipe->dwState = DISCONNECTED_STATE;
						// В каких случаях это может возникнуть? Нужно ли закрывать хэндл, или можно переподключиться?
						_ASSERTEX(fConnected!=FALSE);
						pPipe->dwState = CLOSING_STATE;
						CloseHandle(pPipe->hPipeInst);
						pPipe->hPipeInst = NULL;
						pPipe->dwState = CLOSED_STATE;
					}
				} // end - !mb_Overlapped
			} // Цикл ожидания подключения while (!mb_Terminate)

			return lbRc;
		}
		
		BOOL WriteDummyAnswer(PipeInst* pPipe)
		{
			if (pPipe->bBreakConnection)
				return FALSE;

			BOOL fWriteSuccess = FALSE;
			DWORD cbSize = mn_DummyAnswerSize, cbWritten;
			_ASSERTEX(cbSize >= sizeof(DWORD) && cbSize <= sizeof(T));
			BYTE *ptrNil = (BYTE*)calloc(cbSize, 1);
			if (ptrNil)
			{
				fWriteSuccess = PipeServerWrite(pPipe, ptrNil, cbSize, &cbWritten);
				free(ptrNil);
			}
			return fWriteSuccess;
		}

		// Processing methods
		DWORD PipeServerThread(PipeInst* pPipe)
		{
			_ASSERTEX(this && pPipe && pPipe->pServer==this);
			BOOL fConnected = FALSE;
			BOOL fNewCheckStart = (mn_PipesToCreate > 0);
			DWORD dwErr = 0;
			HANDLE hWait[2] = {NULL,NULL}; // без Overlapped - используется только при ожидании свободного сервера
			DWORD dwTID = GetCurrentThreadId();
			// The main loop creates an instance of the named pipe and
			// then waits for a client to connect to it. When the client
			// connects, a thread is created to handle communications
			// with that client, and the loop is repeated.
			hWait[0] = mh_TermEvent;
			hWait[1] = mh_ServerSemaphore;

			// debuging purposes
			pPipe->dwState = STARTING_STATE;
			pPipe->nTerminateDelay = pPipe->nTerminateDelay1 = pPipe->nTerminateDelay2 = pPipe->nTerminateDelay3 = 0;
			
			_ASSERTEX(!mb_Overlapped || pPipe->hEvent != NULL);
			_ASSERTEX(pPipe->hPipeInst == NULL);

			// Пока не затребовано завершение сервера
			while (!mb_Terminate)
			{
				int   fWriteSuccess = 0;
				BOOL  bNeedReply = FALSE;
				DWORD cbWritten = 0;

				// Цикл ожидания подключения
				if (!WaitForClient(pPipe, fConnected, bNeedReply))
				{
					_ASSERTEX(mb_Terminate==true);
				}
				else
				{
					DEBUGTEST(pPipe->nConnectCount++);
				}
				
				// Сервер закрывается?
				if (mb_Terminate)
				{
					DWORD t1 = _GetTime();
					// Всегда чего-нибудь ответить в пайп, а то ошибка (Pipe was closed) у клиента возникает
					if (pPipe->hPipeInst)
					{
						if (bNeedReply)
						{
							fWriteSuccess = WriteDummyAnswer(pPipe);
							
							// Function does not return until the client process has read all the data.
							//FlushFileBuffers(pPipe->hPipeInst);
						}
						//DisconnectNamedPipe(hPipe);
						pPipe->dwState = CLOSING_STATE;
						CloseHandle(pPipe->hPipeInst);
						pPipe->hPipeInst = NULL;
						pPipe->dwState = CLOSED_STATE;
					}
					DWORD t2 = _GetTime();
					pPipe->nTerminateDelay3 = (t2 - t1);
					pPipe->nTerminateDelay += pPipe->nTerminateDelay3;
					return 0;
				}
				
				// Если еще не все пайпы открыты/потоки запущены
				if (fNewCheckStart && (mn_PipesToCreate > 0))
				{
					mn_PipesToCreate--;
					fNewCheckStart = FALSE;
					for (int j = 0; j < mn_MaxCount; j++)
					{
						if (m_Pipes[j].hThread == NULL)
						{
							// Стартуем новый instance
							bool bStartNewInstance = StartPipeInstance(&(m_Pipes[j]));
							_ASSERTEX(bStartNewInstance==true);
							break;
						}
					}
				}
				
				BOOL lbFirstRead = TRUE;
				BOOL lbAllowClient = TRUE;
				
				if (mfn_PipeServerConnected)
				{
					if (!mfn_PipeServerConnected(pPipe, m_lParam))
					{
						lbAllowClient = FALSE; // запрет подключения от этого клиента
					}
				}
				

				if (lbAllowClient)
				{
					// *** Основной рабочий цикл ***
					// Чтение данных запроса из пайпа и запись результата
					while (PipeServerRead(pPipe))
					{
						#ifdef _DEBUG
						pPipe->nCallCount++;
						GetLocalTime(&pPipe->stLastCall);
						#endif

						if (lbFirstRead)
						{
							lbFirstRead = FALSE;
						}
						
						pPipe->bBreakConnection = false;

						if (mfn_PipeServerCommand(pPipe, pPipe->ptrRequest, /*OUT&*/pPipe->ptrReply,
									/*OUT&*/pPipe->cbReplySize, /*OUT&*/pPipe->cbMaxReplySize, m_lParam))
						{
							#ifdef _DEBUG
							{
								CESERVER_REQ_HDR* ph = (CESERVER_REQ_HDR*)pPipe->ptrRequest;
								if ((ph->cbSize >= sizeof(CESERVER_REQ_HDR))
									&& (ph->nVersion == CESERVER_REQ_VER)
									&& (ph->bAsync))
								{
									_ASSERTEX(pPipe->bBreakConnection && "Break connection on anysc!");
								}
							}
							#endif
							// Если данные вернули (могли сами в пайп ответить)
							if (!mb_InputOnly && pPipe->ptrReply && pPipe->cbReplySize)
							{
								// Если успешно - запись в пайп результата
								_ASSERTEX(pPipe->cbReplySize>=(DWORD)mn_DummyAnswerSize);
								fWriteSuccess = PipeServerWrite(pPipe, pPipe->ptrReply, pPipe->cbReplySize, &cbWritten);
							}
						}
						else if (!mb_InputOnly)
						{
							// Всегда чего-нибудь ответить в пайп, а то ошибка (Pipe was closed) у клиента возникает
							fWriteSuccess = WriteDummyAnswer(pPipe);
						}

						#ifdef _DEBUG
						if (fWriteSuccess)
							pPipe->nAnswCount++;
						GetLocalTime(&pPipe->stLastEndCall);
						#endif

						if (pPipe->bBreakConnection)
							break; // Переоткрыть пайп (ошибка протокола)
					
						if (mb_Terminate || !mb_LoopCommands)
							break;

					} // while (PipeServerRead(pPipe))
				}
				
				if (!pPipe->bBreakConnection)
				{
					// Fin
					if (lbFirstRead)
					{
						// Всегда чего-нибудь ответить в пайп, а то ошибка (Pipe was closed) у клиента возникает
						fWriteSuccess = WriteDummyAnswer(pPipe);
					}
					
					if (!mb_Terminate)
					{
						// Function does not return until the client process has read all the data.
						FlushFileBuffers(pPipe->hPipeInst);
					}
				}
				
				if (mfn_PipeServerDisconnected)
					mfn_PipeServerDisconnected(pPipe, m_lParam);
				
				if (!mb_Terminate)
				{
					// FlushFileBuffers уже вызван, т.е. клиент данные не потеряет
					// А вот вызов CloseHandle перед DisconnectNamedPipe может привести к ошибке 231
					// (All pipe instances are busy) при следующей попытке CreateNamedPipe
					pPipe->dwState = DISCONNECTING_STATE;
					DisconnectNamedPipe(pPipe->hPipeInst);
					pPipe->dwState = DISCONNECTED_STATE;
				}

				TODO("Vozmogno, v kakih-to sluchajah nujno zakryt' pipe handle");

				// Перейти к открытию нового instance пайпа

			} // Пока не затребовано завершение сервера while (!mb_Terminate)

			if (pPipe->hPipeInst && (pPipe->hPipeInst != INVALID_HANDLE_VALUE))
			{
				pPipe->dwState = CLOSING_STATE;
				CloseHandle(pPipe->hPipeInst);
				pPipe->hPipeInst = NULL;
				pPipe->dwState = CLOSED_STATE;
			}

			return 0;
		};
		bool StartPipeInstance(PipeInst* pPipe)
		{
			if (mb_Overlapped)
			{
				if (pPipe->hEvent == NULL)
				{
					pPipe->hEvent = CreateEvent(
						NULL,    // default security attribute
						TRUE,    // manual-reset(!) event
						TRUE,    // initial state = signaled
						NULL);   // unnamed event object

					if (pPipe->hEvent == NULL)
					{
						DumpError(pPipe, L"StartPipeInstance:CreateEvent failed with 0x%08X");
						return false; //-(200+j);
					}
				}

				pPipe->oOverlap.hEvent = pPipe->hEvent;
			}

			_ASSERTEX(pPipe->hThread == NULL);
			_ASSERTEX(pPipe->hThreadEnd == NULL);

			pPipe->pServer = this;
			pPipe->nThreadId = 0;
			pPipe->hThreadEnd = CreateEvent(NULL, TRUE, FALSE, NULL);
			pPipe->hThread = CreateThread(NULL, 0, _PipeServerThread, pPipe, 0, &pPipe->nThreadId);
			if (pPipe->hThread == NULL)
			{
				//_ASSERTEX(m_Pipes[i].hThread!=NULL);
				DumpError(pPipe, L"StartPipeInstance:CreateThread failed, code=0x%08X");
				return false; // Не удалось создать серверные потоки
			}
			if (mn_Priority)
				::SetThreadPriority(pPipe->hThread, mn_Priority);

			return true;
		};
		static DWORD WINAPI _PipeServerThread(LPVOID lpvParam)
		{
			DWORD nResult = 101;
			PipeInst* pPipe = (PipeInst*)lpvParam;
			_ASSERTEX(pPipe!=NULL && pPipe->pServer!=NULL);

			pPipe->dwState = STARTED_STATE;

			if (!pPipe->pServer->mb_Terminate)
			{
				// Working method
				nResult = pPipe->pServer->PipeServerThread(pPipe);
			}

			pPipe->dwState = TERMINATED_STATE;
			
			// Force thread termination cause of unknown attached dll's.
			if (pPipe->pServer->mb_Terminate)
			{
				TerminateThread(GetCurrentThread(), 100);
			}
			return nResult;
		};
	//public:
	//	CPipeServer()
	//	{
	//		mb_Initialized = FALSE;
	//		ms_PipeName[0] = 0;
	//		memset(m_Pipes, 0, sizeof(m_Pipes));
	//		mp_ActivePipe = NULL;
	//		mfn_PipeServerCommand = NULL;
	//		mfn_PipeServerReady = NULL;
	//		mb_ReadyCalled = FALSE;
	//		m_lParam = 0;
	//		mh_TermEvent = NULL;
	//		mb_Terminate = FALSE;
	//		mh_ServerSemaphore = NULL;
	//	};
	//	~CPipeServer()
	//	{
	//		StopPipeServer();
	//	};
	public:
		void SetPriority(int nPriority)
		{
			// Звать ПЕРЕД StartPipeServer
			_ASSERTEX(mb_Initialized==FALSE);
			mn_Priority = nPriority;
		};

		void SetInputOnly(bool bInputOnly)
		{
			// Звать ПЕРЕД StartPipeServer
			_ASSERTEX(mb_Initialized==FALSE);
			mb_InputOnly = bInputOnly;
		};

		void SetOverlapped(bool bOverlapped)
		{
			// Звать ПЕРЕД StartPipeServer
			_ASSERTEX(mb_Initialized==FALSE);
			mb_Overlapped = bOverlapped;
		};

		void SetLoopCommands(bool bLoopCommands)
		{
			// Звать ПЕРЕД StartPipeServer
			_ASSERTEX(mb_Initialized==FALSE);
			mb_LoopCommands = bLoopCommands;
		};

		void SetDummyAnswerSize(int anDummyAnswerSize)
		{
			// Звать ПЕРЕД StartPipeServer
			mn_DummyAnswerSize = anDummyAnswerSize;
		};

		void SetMaxCount(int nMaxCount)
		{
			// Звать ПЕРЕД StartPipeServer
			if (mb_Initialized || m_Pipes || mn_MaxCount > 0)
			{
				_ASSERTEX(mb_Initialized==FALSE && !m_Pipes && mn_MaxCount==0);
				return;
			}
			mn_MaxCount = nMaxCount;
		};
		
		bool StartPipeServer(
			    LPCWSTR asPipeName,
			    LPARAM alParam,
			    LPSECURITY_ATTRIBUTES alpSec /*= LocalSecurity()*/, // MUST! be alive for life cicle
			    PipeServerCommand_t apfnPipeServerCommand,
			    PipeServerFree_t apfnPipeServerFree = NULL,
			    PipeServerConnected_t apfnPipeServerConnected = NULL,
			    PipeServerConnected_t apfnPipeServerDisconnected = NULL,
			    PipeServerReady_t apfnPipeServerReady = NULL
		    )
		{
			if (mb_Initialized)
			{
				_ASSERTEX(mb_Initialized==FALSE);
				return false;
			}
			
			mb_Initialized = false;
			//mb_Overlapped = abOverlapped;
			//mb_LoopCommands = abLoopCommands;
			
			if (mn_MaxCount < 1)
				mn_MaxCount = 1;
			if (mn_DummyAnswerSize <= 0 || mn_DummyAnswerSize > sizeof(T))
				mn_DummyAnswerSize = sizeof(T);
				
			m_Pipes = (PipeInst*)calloc(mn_MaxCount,sizeof(*m_Pipes));
			if (m_Pipes == NULL)
			{
				_ASSERTEX(m_Pipes!=NULL);
				return false;
			}
			//memset(m_Pipes, 0, sizeof(m_Pipes));

			mp_ActivePipe = NULL;

			mb_ReadyCalled = FALSE;
			mh_TermEvent = CreateEvent(NULL,TRUE/*MANUAL - используется в нескольких нитях!*/,FALSE,NULL); ResetEvent(mh_TermEvent);
			mb_Terminate = false;
			mh_ServerSemaphore = CreateSemaphore(NULL, 1, 1, NULL);
			
			wcscpy_c(ms_PipeName, asPipeName);
			m_lParam = alParam;
			mlp_Sec = alpSec;
			
			mfn_PipeServerCommand = apfnPipeServerCommand;
			mfn_PipeServerReady = apfnPipeServerReady;
			mfn_PipeServerConnected = apfnPipeServerConnected;
			mfn_PipeServerDisconnected = apfnPipeServerDisconnected;
			mfn_PipeServerFree = apfnPipeServerFree;
			_ASSERTEX(mb_ReadyCalled==FALSE);
			
			// Необходимо указать как минимум эту функцию
			if (!apfnPipeServerCommand)
			{
				//_ASSERTEX(apfnPipeServerCommand!=NULL);
				DumpError(&(m_Pipes[0]), L"StartPipeServer:apfnPipeServerCommand is NULL");
				return false;
			}

			if (!StartPipeInstance(&(m_Pipes[0])))
			{
				// DumpError уже позвали
				return false;
			}
			mn_PipesToCreate = (mn_MaxCount > 1) ? (mn_MaxCount - 1) : 0;

			mb_Initialized = TRUE;
			
			//#if 0
			//// Overlapped, доделать
			//// Созаем Instances
			//int iRc = InitPipes();
			//return (iRc == 0);
			//#endif
			
			return true;
		};

		void StopPipeServer()
		{
			if (!mb_Initialized)
				return;

			DWORD nWait;
			
			mb_Terminate = true;
			if (mh_TermEvent)
				SetEvent(mh_TermEvent);

			for (int i = 0; i < mn_MaxCount; i++)
			{
				if (m_Pipes[i].hThread)
				{
					DWORD nTimeout = 250;
					#ifdef _DEBUG
						nTimeout = 5000;
					#endif

					HANDLE hEvt[2] = {m_Pipes[i].hThread, m_Pipes[i].hThreadEnd};
					nWait = (m_Pipes[i].dwState == STARTING_STATE) ? WAIT_FAILED : WaitForMultipleObjects(countof(hEvt), hEvt, FALSE, nTimeout);

					if (nWait != WAIT_OBJECT_0)
					{
						#ifdef _DEBUG
							if (m_Pipes[i].dwState != STARTING_STATE)
							{
								SuspendThread(m_Pipes[i].hThread);
								_ASSERTEX(nWait == WAIT_OBJECT_0 && ("Ожидание завершения пайпа"));
								ResumeThread(m_Pipes[i].hThread);
							}
						#endif

						TerminateThread(m_Pipes[i].hThread, 100);
					}

					CloseHandle(m_Pipes[i].hThread);
					m_Pipes[i].hThread = NULL;
				}
				if (m_Pipes[i].hThreadEnd)
				{
					CloseHandle(m_Pipes[i].hThreadEnd);
					m_Pipes[i].hThreadEnd = NULL;
				}

				if (m_Pipes[i].hPipeInst && (m_Pipes[i].hPipeInst != INVALID_HANDLE_VALUE))
				{
					m_Pipes[i].dwState = DISCONNECTING_STATE;
					DisconnectNamedPipe(m_Pipes[i].hPipeInst);
					m_Pipes[i].dwState = DISCONNECTED_STATE;

					m_Pipes[i].dwState = CLOSING_STATE;
					CloseHandle(m_Pipes[i].hPipeInst);
					m_Pipes[i].dwState = CLOSED_STATE;
				}
				m_Pipes[i].hPipeInst = NULL;
				
				if (m_Pipes[i].hEvent)
				{
					CloseHandle(m_Pipes[i].hEvent);
					m_Pipes[i].hEvent = NULL;
				}
				
				if (m_Pipes[i].ptrRequest)
				{
					free(m_Pipes[i].ptrRequest);
					m_Pipes[i].ptrRequest = NULL;
				}
				
				if (m_Pipes[i].ptrReply && mfn_PipeServerFree)
				{
					mfn_PipeServerFree(m_Pipes[i].ptrReply, m_lParam);
					m_Pipes[i].ptrReply = NULL;
				}
			}
			
			if (mh_ServerSemaphore)
			{
				CloseHandle(mh_ServerSemaphore);
				mh_ServerSemaphore = NULL;
			}
			if (mh_TermEvent)
			{
				CloseHandle(mh_TermEvent);
				mh_TermEvent = NULL;
			}

			free(m_Pipes);
			m_Pipes = NULL;

			mb_Initialized = FALSE;
		};

		bool DelayedWrite(LPVOID pInstance, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite)
		{
			PipeInst* pPipe = (PipeInst*)pInstance;
			DWORD dwWritten = 0;

			_ASSERTEX((int)nNumberOfBytesToWrite>=mn_DummyAnswerSize);
			int iRc = PipeServerWrite(pPipe, lpBuffer, nNumberOfBytesToWrite, &dwWritten, TRUE);

			return (iRc == 1);
		};

		void BreakConnection(LPVOID pInstance)
		{
			PipeInst* pPipe = (PipeInst*)pInstance;
			pPipe->bBreakConnection = true;
		};

		HANDLE GetPipeHandle(LPVOID pInstance)
		{
			return pInstance ? ((PipeInst*)pInstance)->hPipeInst : NULL;
		};

		bool IsPipeThread(DWORD TID)
		{
			if (!mb_Initialized)
				return false;

			for (int i = 0; i < mn_MaxCount; i++)
			{
				if (m_Pipes[i].hThread && m_Pipes[i].nThreadId == TID)
					return true;
			}

			return false;
		};
};
