
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

// Класс предполагает, что первый DWORD в (T*) это размер передаваемого блока в байтах!
template <class T, int MaxCount>
class CPipeServer
{
protected:
	enum PipeState
	{
		CONNECTING_STATE = 0,
		READING_STATE = 1,
		WRITING_STATE = 2,
	};
	struct PipeInst
	{
		// Common
		HANDLE hPipeInst; // Pipe Handle
		HANDLE hEvent; // Overlapped event
		OVERLAPPED oOverlap;
		BOOL fPendingIO;
		PipeState dwState;
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
		T* ptrReply;
		T* AllocReplyBuf(DWORD anSize)
		{
			if (ptrReply && (cbMaxReplySize >= anSize))
				return ptrReply;
			if (ptrReply)
				free(ptrReply);
			ptrReply = (T*)malloc(anSize);
			cbMaxReplySize = anSize;
			return ptrReply;
		};
		// Event descriptors
		HANDLE hTermEvent;
		HANDLE hServerSemaphore;
		// Pointer to object
		CPipeServer *pServer;
		// Server thread
		HANDLE hThread;
		DWORD nThreadId;
	};
	wchar_t ms_PipeName[MAX_PATH];
	PipeInst m_Pipes[MaxCount];
	PipeInst mp_ActivePipe;
	// Callback function and it's LParam
	typedef BOOL (WINAPI* PipeServerCommand_t)(T* pCmd, T** ppReply, DWORD* pcbReplySize, DWORD* pcbMaxReplySize, LPARAM lParam);
	typedef BOOL (WINAPI* PipeServerReady_t)(LPARAM lParam);
	PipeServerCommand_t mfn_PipeServerCommand;
	PipeServerReady_t mfn_PipeServerReady;
	BOOL mb_ReadyCalled;
	LPARAM m_lParam;
protected:
	// Event descriptors
	HANDLE mh_TermEvent;
	HANDLE mh_ServerSemaphore;

	BOOL PipeServerRead(PipeInst* pPipe)
	{
		DWORD cbRead = 0, dwErr = 0, cbWholeSize = 0;
		BOOL fSuccess = FALSE;
		T In, *pIn = NULL;;

		// Send a message to the pipe server and read the response. 
		fSuccess = ReadFile( 
			pPipe->hPipeInst, // pipe handle 
			&In,              // buffer to receive reply
			sizeof(In),       // size of read buffer
			&cbRead,          // bytes read
			NULL);            // not overlapped 

		if ((!fSuccess && ((dwErr = GetLastError()) != ERROR_MORE_DATA)) || (cbRead < sizeof(DWORD)))
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
		//_ASSERTE(in.hdr.cbSize>=sizeof(CESERVER_REQ_HDR) && cbRead>=sizeof(CESERVER_REQ_HDR));
		//if (cbRead < sizeof(CESERVER_REQ_HDR) || /*in.hdr.cbSize < cbRead ||*/ in.hdr.nVersion != CESERVER_REQ_VER) {
		//	//CloseHandle(hPipe);
		//	return;
		//}

		cbWholeSize = *((DWORD*)&In);
		if (cbWholeSize <= cbRead)
		{
			_ASSERTE(cbWholeSize >= cbRead);
			pIn = pPipe->AllocRequestBuf(cbRead);
			memmove(pIn, &In, cbRead);
			pPipe->cbReadSize = cbRead;
		}
		else
		{
			int nAllSize = cbWholeSize;
			pIn = pPipe->AllocRequestBuf(cbWholeSize);
			_ASSERTE(pIn!=NULL);
			memmove(pIn, &In, cbRead);

			LPBYTE ptrData = ((LPBYTE)pIn)+cbRead;
			nAllSize -= cbRead;

			while (nAllSize>0)
			{ 
				//_tprintf(TEXT("%s\n"), chReadBuf);

				// Break if TransactNamedPipe or ReadFile is successful
				if(fSuccess)
					break;

				// Read from the pipe if there is more data in the message.
				fSuccess = ReadFile( 
					pPipe->hPipeInst, // pipe handle 
					ptrData,          // buffer to receive reply 
					nAllSize,         // size of buffer 
					&cbRead,          // number of bytes read 
					NULL);            // not overlapped 

				// Exit if an error other than ERROR_MORE_DATA occurs.
				if( !fSuccess && ((dwErr = GetLastError()) != ERROR_MORE_DATA)) 
					break;
				ptrData += cbRead;
				nAllSize -= cbRead;
			}

			TODO("Может возникнуть ASSERT, если консоль была закрыта в процессе чтения");
			if (nAllSize>0)
			{
				_ASSERTE(nAllSize==0);
				return FALSE; // удалось считать не все данные
			}

			pPipe->cbReadSize = (DWORD)(ptrData - ((LPBYTE)pIn));
		}

		return TRUE;
	}

	// Processing methods
	static DWORD WINAPI PipeServerThread(LPVOID lpvParam)
	{
		PipeInst* pPipe = (PipeInst*)lpvParam;
		BOOL fConnected = FALSE;
		DWORD dwErr = 0;
		HANDLE hWait[2] = {NULL,NULL};
		DWORD dwTID = GetCurrentThreadId();

		// The main loop creates an instance of the named pipe and 
		// then waits for a client to connect to it. When the client 
		// connects, a thread is created to handle communications 
		// with that client, and the loop is repeated. 

		hWait[0] = pPipe->hTermEvent;
		hWait[1] = pPipe->hServerSemaphore;

		// Пока не затребовано завершение сервера
		while (WaitForSingleObject ( pPipe->hTermEvent, 0 ) != WAIT_OBJECT_0)
		{
			while (TRUE)
			{
				_ASSERTE(pPipe->hPipeInst == NULL);

				// !!! Переносить проверку семафора ПОСЛЕ CreateNamedPipe нельзя, т.к. в этом случае
				//     нити дерутся и клиент не может подцепиться к серверу

				// Дождаться разрешения семафора, или завершения сервера
				dwErr = WaitForMultipleObjects ( 2, hWait, FALSE, INFINITE );
				if (dwErr == WAIT_OBJECT_0)
				{
					return 0; // Сервер закрывается
				}
				_ASSERTE(dwErr == (WAIT_OBJECT_0+1));

				pPipe->pServer->mp_ActivePipe = pPipe;

				TODO("gpNullSecurity заменить на gpLocalSecurity");
				_ASSERTE(gpNullSecurity);
				pPipe->hPipeInst = CreateNamedPipeW(
					pPipe->pServer->ms_PipeName, // pipe name 
					PIPE_ACCESS_DUPLEX |         // read/write access 
					FILE_FLAG_OVERLAPPED,        // overlapped mode 
					PIPE_TYPE_MESSAGE |         // message type pipe 
					PIPE_READMODE_MESSAGE |     // message-read mode 
					PIPE_WAIT,                  // blocking mode 
					MaxCount,                   // max. instances  
					PIPEBUFSIZE,                // output buffer size 
					PIPEBUFSIZE,                // input buffer size 
					0,                          // client time-out 
					gpNullSecurity);            // default security attribute 

				if (pPipe->hPipeInst == INVALID_HANDLE_VALUE) 
				{
					dwErr = GetLastError();

					_ASSERTE(pPipe->hPipeInst != INVALID_HANDLE_VALUE);
					//DisplayLastError(L"CreateNamedPipe failed"); 
					pPipe->hPipeInst = NULL;
					// Разрешить этой или другой нити создать серверный пайп
					ReleaseSemaphore(hWait[1], 1, NULL);
					//Sleep(50);
					continue;
				}

				// Чтобы ConEmuC знал, что серверный пайп готов
				if (!pPipe->pServer->mb_ReadyCalled && pPipe->pServer->mfn_PipeServerReady)
				{
					pPipe->pServer->mb_ReadyCalled = TRUE;
					pPipe->pServer->mfn_PipeServerReady(pPipe->pServer->m_lParam);
					//SetEvent(pRCon->mh_GuiAttached);
					//SafeCloseHandle(pRCon->mh_GuiAttached);
				}


				// Wait for the client to connect; if it succeeds, 
				// the function returns a nonzero value. If the function
				// returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 

				fConnected = ConnectNamedPipe(pPipe->hPipeInst, NULL)
					? TRUE : ((dwErr = GetLastError()) == ERROR_PIPE_CONNECTED);

				// сразу разрешить другой нити принять вызов
				ReleaseSemaphore(hWait[1], 1, NULL);

				// Сервер закрывается?
				if (WaitForSingleObject ( hWait[0], 0 ) == WAIT_OBJECT_0)
				{
					TODO("Записать в пайп (DWORD)0");
					//FlushFileBuffers(hPipe); -- это не нужно, мы ничего не возвращали
					//DisconnectNamedPipe(hPipe); 
					SafeCloseHandle(pPipe->hPipeInst);
					return 0;
				}

				if (fConnected)
					break;
				else
					SafeCloseHandle(pPipe->hPipeInst);
			}

			// Чтение данных запроса из пайпа
			if (!pPipe->pServer->PipeServerRead(pPipe))
			{
				// Запись в результат - (DWORD)0
			}
			// Если успешно - запись в пайп результата
			else if (pPipe->pServer->mfn_PipeServerCommand(pPipe->ptrRequest, &pPipe->ptrReply,
					&pPipe->cbReplySize, &pPipe->cbMaxReplySize, pPipe->pServer->m_lParam))
			{
				_ASSERTE(pPipe->cbReplySize == *((DWORD*)pPipe->ptrReply));
				_ASSERTE(pPipe->cbReplySize <= pPipe->cbMaxReplySize);
			}
			else
			{
				// Иначе - запись (DWORD)0
			}

			TODO("Запись в пайп результата");

			FlushFileBuffers(pPipe->hPipeInst);
			//DisconnectNamedPipe(hPipe); 
			SafeCloseHandle(pPipe->hPipeInst);
		
			// Перейти к открытию нового instance пайпа
		}

		return 0; 
	};
public:
	CPipeServer()
	{
	};
	~CPipeServer()
	{
	};
public:
	BOOL StartPipeServer(LPCWSTR asPipeName, PipeServerCommand_t apfnPipeServerCommand, LPARAM alParam)
	{
	};
	void StopPipeServer()
	{
	};
};
