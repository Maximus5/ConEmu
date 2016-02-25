
/*
Copyright (c) 2013-2015 Maximus5
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

#include <windows.h>
#include "Common.h"
#include "ConEmuCheck.h"

#ifdef _DEBUG
	#define TRANSACT_WARN_TIMEOUT 500
#endif

#define TRANSACT_MAX_TIMEOUT 30000

template <class T_IN, class T_OUT>
class MPipe
{
	protected:
		WCHAR ms_PipeName[MAX_PATH], ms_Module[32], ms_Error[MAX_PATH*2];
		HANDLE mh_Pipe, mh_Heap, mh_TermEvent;
		BOOL mb_Overlapped;
		OVERLAPPED m_Ovl;
		typedef BOOL (WINAPI* CancelIo_t)(HANDLE hFile);
		typedef BOOL (WINAPI* CancelIoEx_t)(HANDLE hFile, LPOVERLAPPED lpOverlapped);
		CancelIo_t _CancelIo;
		CancelIoEx_t _CancelIoEx;
		T_IN m_In; // для справки...
		T_OUT* mp_Out; DWORD mn_OutSize, mn_MaxOutSize;
		T_OUT m_Tmp;
		DWORD mn_ErrCode;
		LONG  mn_OpenCount, mn_CloseCount, mn_FailCount;
		void SaveErrorCode(DWORD nCode)
		{
			mn_ErrCode = nCode;
		};
		T_OUT* AllocateBuffer(DWORD nAllSize, T_OUT* pPart, DWORD cbPart)
		{
			if (!mp_Out || (int)mn_MaxOutSize < nAllSize)
			{
				T_OUT* ptrNew = (T_OUT*)HeapAlloc(mh_Heap, 0, nAllSize); //-V106

				if (!ptrNew)
				{
					_ASSERTE(ptrNew!=NULL);
					msprintf(ms_Error, countof(ms_Error), L"%s: Can't allocate %u bytes!", ms_Module, nAllSize);
					return FALSE;
				}

				if (pPart && cbPart)
				{
					memmove(ptrNew, pPart, cbPart); //-V106
				}

				if (mp_Out) HeapFree(mh_Heap, 0, mp_Out);

				mn_MaxOutSize = nAllSize;
				mp_Out = ptrNew;
			}
			return mp_Out;
		}
	public:
		MPipe()
		{
			ms_PipeName[0] = ms_Module[0] = ms_Error[0] = 0;
			mh_Pipe = NULL;
			memset(&m_In, 0, sizeof(m_In));
			mp_Out = NULL;
			mn_OutSize = mn_MaxOutSize = 0;
			memset(&m_Tmp, 0, sizeof(m_Tmp));
			mn_ErrCode = 0;
			mn_OpenCount = mn_CloseCount = mn_FailCount = 0;
			mh_Heap = GetProcessHeap();
			mh_TermEvent = NULL;
			mb_Overlapped = FALSE;
			memset(&m_Ovl, 0, sizeof(m_Ovl));
			HMODULE hKernel = GetModuleHandle(L"kernel32.dll");
			_CancelIo = hKernel ? (CancelIo_t)GetProcAddress(hKernel,"CancelIo") : NULL;
			_CancelIoEx = hKernel ? (CancelIoEx_t)GetProcAddress(hKernel,"CancelIoEx") : NULL;
			_ASSERTE(mh_Heap!=NULL);
		};
		void SetTimeout(DWORD anTimeout)
		{
			//TODO: Если anTimeout!=-1 - создавать нить и выполнять команду в ней. Ожидать нить не более и прибить ее, если пришел Timeout
		};
		void SetTermEvent(HANDLE hTermEvent)
		{
			mh_TermEvent = hTermEvent;
		};
		virtual ~MPipe()
		{
			ReleaseBuffer();
			_ASSERTE(mn_CloseCount==mn_OpenCount);
		};
		void ReleaseBuffer()
		{
			if (mp_Out && mp_Out != &m_Tmp)
			{
				if (mh_Heap)
					HeapFree(mh_Heap, 0, mp_Out);
				else
					_ASSERTE(mh_Heap!=NULL);
			}
			mp_Out = NULL;
		};
		void Close()
		{
			if (mh_Pipe && mh_Pipe != INVALID_HANDLE_VALUE)
			{
				InterlockedIncrement(&mn_CloseCount);
				if (_CancelIoEx)
					_CancelIoEx(mh_Pipe, NULL);
				else if (_CancelIo)
					_CancelIo(mh_Pipe);
				CloseHandle(mh_Pipe);
			}
			if (m_Ovl.hEvent)
			{
				SafeCloseHandle(m_Ovl.hEvent);
				m_Ovl.hEvent = NULL;
			}

			mh_Pipe = NULL;
		};
		void InitName(const wchar_t* asModule, const wchar_t *aszTemplate, const wchar_t *Parm1, DWORD Parm2)
		{
			msprintf(ms_PipeName, countof(ms_PipeName), aszTemplate, Parm1, Parm2);
			lstrcpynW(ms_Module, asModule, countof(ms_Module)); //-V303

			if (mh_Pipe)
				Close();
		};
		BOOL Open()
		{
			if (mh_Pipe && mh_Pipe != INVALID_HANDLE_VALUE)
				return TRUE;

			mb_Overlapped = (mh_TermEvent != NULL);
			if (mb_Overlapped && (m_Ovl.hEvent == NULL))
			{
				m_Ovl.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			}

			mh_Pipe = ExecuteOpenPipe(ms_PipeName, ms_Error, ms_Module, 0, 0, mb_Overlapped, mh_TermEvent);

			if (mh_Pipe == INVALID_HANDLE_VALUE)
			{
				SaveErrorCode(GetLastError());
				InterlockedIncrement(&mn_FailCount);
				_ASSERTE(mh_Pipe != INVALID_HANDLE_VALUE);
				mh_Pipe = NULL;
			}

			InterlockedIncrement(&mn_OpenCount);
			return (mh_Pipe!=NULL);
		};
		BOOL Transact(const T_IN* apIn, DWORD anInSize, const T_OUT** rpOut)
		{
			if (!apIn || !rpOut)
			{
				_ASSERTE(apIn && rpOut);
				return FALSE;
			}

			ms_Error[0] = 0;
			*rpOut = &m_Tmp;

			DEBUGTEST(DWORD nOpenTick = GetTickCount());

			if (!Open())  // Пайп может быть уже открыт, функция это учитывает
				return FALSE;

			_ASSERTE(sizeof(m_In) >= sizeof(CESERVER_REQ_HDR));
			_ASSERTE(sizeof(T_OUT) >= sizeof(CESERVER_REQ_HDR));
			BOOL fSuccess = FALSE;
			DWORD cbRead, dwErr, cbReadBuf, nOverlappedWait;
			// Для справки, информация о последнем запросе
			cbReadBuf = min(anInSize, sizeof(m_In)); //-V105 //-V103
			memmove(&m_In, apIn, cbReadBuf); //-V106
			// Send a message to the pipe server and read the response.
			cbRead = 0; cbReadBuf = sizeof(m_Tmp);
			T_OUT* ptrOut = &m_Tmp;

			if (mp_Out && (mn_MaxOutSize > cbReadBuf))
			{
				ptrOut = mp_Out;
				cbReadBuf = mn_MaxOutSize;
				*rpOut = mp_Out;
			}

			DEBUGTEST(DWORD nStartTick = GetTickCount());

			//SetLastError(0);

			WARNING("BLOKIROVKA! Inogda zavisaet pri zakrytii konsoli");
			fSuccess = TransactNamedPipe(mh_Pipe, (LPVOID)apIn, anInSize, ptrOut, cbReadBuf, &cbRead, mb_Overlapped ? &m_Ovl : NULL);
			dwErr = fSuccess ? 0 : GetLastError();
			SaveErrorCode(dwErr);

			DEBUGTEST(DWORD nStartTick2 = GetTickCount());
			DEBUGTEST(DWORD nTickOvl = 0);

			// MSDN: If the operation cannot be completed immediately,
			// TransactNamedPipe returns FALSE and GetLastError returns ERROR_IO_PENDING.
			if (mb_Overlapped && !fSuccess && dwErr == ERROR_IO_PENDING)
			{
				if (mh_TermEvent)
				{
					HANDLE hEvents[2] = {m_Ovl.hEvent, mh_TermEvent};
					nOverlappedWait = WaitForMultipleObjects(2, hEvents, FALSE, TRANSACT_MAX_TIMEOUT);
				}
				else
				{
					nOverlappedWait = WaitForSingleObject(m_Ovl.hEvent, TRANSACT_MAX_TIMEOUT);
				}

				if ((nOverlappedWait == WAIT_TIMEOUT) || (nOverlappedWait == (WAIT_OBJECT_0+1)))
				{
					// Ошибка ожидания или закрытие приложения
					InterlockedIncrement(&mn_FailCount);
					Close();
					return FALSE;
				}

				fSuccess = GetOverlappedResult(mh_Pipe, &m_Ovl, &cbRead, FALSE);
				dwErr = GetLastError();
				SaveErrorCode(dwErr);

				DEBUGTEST(nTickOvl = GetTickCount());
			}

			#ifdef _DEBUG
			DWORD nEndTick = GetTickCount();
			DWORD nDelta = nEndTick - nStartTick;
			int nTermWait = mh_TermEvent ? WaitForSingleObject(mh_TermEvent, 0) : -1;
			if (nDelta >= TRANSACT_WARN_TIMEOUT && !IsDebuggerPresent())
			{
				//_ASSERTE(nDelta <= TRANSACT_WARN_TIMEOUT);
			}
			#endif

			if (!fSuccess && dwErr == ERROR_BROKEN_PIPE)
			{
				// Сервер не вернул данных, но обработал команду
				InterlockedIncrement(&mn_FailCount);
				Close(); // Раз пайп закрыт - прибиваем хэндл
				return TRUE;
			}

			if (!fSuccess && (dwErr != ERROR_MORE_DATA))
			{
				DEBUGSTR(L" - FAILED!\n");
				DWORD nCmd = 0;

				if (anInSize >= sizeof(CESERVER_REQ_HDR))
					nCmd = ((CESERVER_REQ_HDR*)apIn)->nCmd;

				msprintf(ms_Error, countof(ms_Error), L"%s: TransactNamedPipe failed, Cmd=%i, ErrCode=%u!", ms_Module, nCmd, dwErr);
				InterlockedIncrement(&mn_FailCount);
				Close(); // Поскольку произошла неизвестная ошибка - пайп лучше закрыть (чтобы потом переоткрыть)
				return FALSE;
			}

			// Пошли проверки заголовка
			if (cbRead < sizeof(CESERVER_REQ_HDR))
			{
				_ASSERTE(cbRead >= sizeof(CESERVER_REQ_HDR));
				msprintf(ms_Error, countof(ms_Error),
				          L"%s: Only %i bytes received, required %i bytes at least!",
				          ms_Module, cbRead, (DWORD)sizeof(CESERVER_REQ_HDR));
				return FALSE;
			}

			if (((CESERVER_REQ_HDR*)apIn)->nCmd != ((CESERVER_REQ_HDR*)&m_In)->nCmd)
			{
				_ASSERTE(((CESERVER_REQ_HDR*)apIn)->nCmd == ((CESERVER_REQ_HDR*)&m_In)->nCmd);
				msprintf(ms_Error, countof(ms_Error),
				          L"%s: Invalid CmdID=%i received, required CmdID=%i!",
				          ms_Module, ((CESERVER_REQ_HDR*)apIn)->nCmd, ((CESERVER_REQ_HDR*)&m_In)->nCmd);
				return FALSE;
			}

			if (((CESERVER_REQ_HDR*)ptrOut)->nVersion != CESERVER_REQ_VER)
			{
				_ASSERTE(((CESERVER_REQ_HDR*)ptrOut)->nVersion == CESERVER_REQ_VER);
				msprintf(ms_Error, countof(ms_Error),
				          L"%s: Old version packet received (%i), required (%i)!",
				          ms_Module, ((CESERVER_REQ_HDR*)ptrOut)->nCmd, CESERVER_REQ_VER);
				return FALSE;
			}

			if (((CESERVER_REQ_HDR*)ptrOut)->cbSize < cbRead)
			{
				_ASSERTE(((CESERVER_REQ_HDR*)ptrOut)->cbSize >= cbRead);
				msprintf(ms_Error, countof(ms_Error),
				          L"%s: Invalid packet size (%i), must be greater or equal to %i!",
				          ms_Module, ((CESERVER_REQ_HDR*)ptrOut)->cbSize, cbRead);
				return FALSE;
			}

			#ifdef _DEBUG
			DWORD nMoreDataTick[6] = {GetTickCount()};
			#endif

			if (dwErr == ERROR_MORE_DATA)
			{
				int nAllSize = ((CESERVER_REQ_HDR*)ptrOut)->cbSize;

				if (!mp_Out || (int)mn_MaxOutSize < nAllSize)
				{
					if (!AllocateBuffer(nAllSize, ptrOut, cbRead))
					{
						return FALSE;
					}
				}
				else if (ptrOut == &m_Tmp)
				{
					memmove(mp_Out, &m_Tmp, cbRead); //-V106
				}

				*rpOut = mp_Out;
				LPBYTE ptrData = ((LPBYTE)mp_Out)+cbRead; //-V104
				nAllSize -= cbRead;

				while (nAllSize>0)
				{
					DEBUGTEST(nMoreDataTick[1] = GetTickCount());
					// Read from the pipe if there is more data in the message.
					//WARNING: Если в буфере пайпа данных меньше чем nAllSize - повиснем!
					fSuccess = ReadFile(mh_Pipe, ptrData, nAllSize, &cbRead, mb_Overlapped ? &m_Ovl : NULL);
					dwErr = fSuccess ? 0 : GetLastError();
					SaveErrorCode(dwErr);

					DEBUGTEST(nMoreDataTick[2] = GetTickCount());

					// MSDN: ReadFile may return before the read operation is complete.
					// In this scenario, ReadFile returns FALSE and the GetLastError
					// function returns ERROR_IO_PENDING.
					if (mb_Overlapped && !fSuccess && dwErr == ERROR_IO_PENDING)
					{
						if (mh_TermEvent)
						{
							HANDLE hEvents[2] = {m_Ovl.hEvent, mh_TermEvent};
							nOverlappedWait = WaitForMultipleObjects(2, hEvents, FALSE, TRANSACT_MAX_TIMEOUT);
						}
						else
						{
							nOverlappedWait = WaitForSingleObject(m_Ovl.hEvent, TRANSACT_MAX_TIMEOUT);
						}

						DEBUGTEST(nMoreDataTick[3] = GetTickCount());

						if ((nOverlappedWait == WAIT_TIMEOUT) || (nOverlappedWait == (WAIT_OBJECT_0+1)))
						{
							// Ошибка ожидания или закрытие приложения
							InterlockedIncrement(&mn_FailCount);
							Close();
							return FALSE;
						}

						fSuccess = GetOverlappedResult(mh_Pipe, &m_Ovl, &cbRead, FALSE);
						dwErr = GetLastError();
						SaveErrorCode(dwErr);
					}


					// Exit if an error other than ERROR_MORE_DATA occurs.
					if (!fSuccess && (dwErr != ERROR_MORE_DATA))
						break;

					ptrData += cbRead; //-V102
					nAllSize -= cbRead;
				}

				TODO("Mozhet vozniknutj ASSERT, esli konsolj byla zakryta v processe chtenija");

				if (nAllSize > 0)
				{
					_ASSERTE(nAllSize==0);
					msprintf(ms_Error, countof(ms_Error), L"%s: Can't read %u bytes!", ms_Module, nAllSize);
					return FALSE;
				}

				if (dwErr == ERROR_MORE_DATA)
				{
					_ASSERTE(dwErr != ERROR_MORE_DATA);
					//	BYTE cbTemp[512];
					//	while (1) {
					//		fSuccess = ReadFile( mh_Pipe, cbTemp, 512, &cbRead, NULL);
					//		dwErr = GetLastError();
					//		SaveErrorCode(dwErr);
					//		// Exit if an error other than ERROR_MORE_DATA occurs.
					//		if ( !fSuccess && (dwErr != ERROR_MORE_DATA))
					//			break;
					//	}
				}

				DEBUGTEST(nMoreDataTick[4] = GetTickCount());

				// Надо ли это?
				fSuccess = FlushFileBuffers(mh_Pipe);

				DEBUGTEST(nMoreDataTick[5] = GetTickCount());
			}

			return TRUE;
		};
		// Informational
		LPCWSTR GetErrorText()
		{
			return ms_Error;
		};
		DWORD GetErrorCode()
		{
			return mn_ErrCode;
		};
};
