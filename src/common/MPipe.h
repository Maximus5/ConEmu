
/*
Copyright (c) 2013-present Maximus5
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

#include "Common.h"
#include "ConEmuCheck.h"
#include "MModule.h"
#include "WUser.h"
#include <limits>
#include <stdexcept>
#include <iterator>

#ifndef NOMINMAX
#error NOMINMAX was not defined
#endif
#ifdef max
#error max macro was defined
#endif

#ifdef _DEBUG
	#define TRANSACT_WARN_TIMEOUT 500
#endif

#define TRANSACT_MAX_TIMEOUT 30000

class MPipeBase
{
	protected:
		friend class MPipeDual;

		wchar_t ms_Module[32] = L"ConEmu";
		wchar_t ms_Error[MAX_PATH*2] = L"";

		HANDLE mh_Pipe = nullptr, mh_Heap = nullptr, mh_TermEvent = nullptr;
		bool mb_Overlapped = false;
		OVERLAPPED m_Ovl = {};

		using CancelIo_t = BOOL (WINAPI*)(HANDLE hFile);
		using CancelIoEx_t = BOOL (WINAPI*)(HANDLE hFile, LPOVERLAPPED lpOverlapped);
		CancelIo_t cancelIo_ = nullptr;
		CancelIoEx_t cancelIoEx_ = nullptr;

		void* mp_Out = nullptr;
		size_t mn_OutSize = 0, mn_MaxOutSize = 0;
		DWORD mn_ErrCode = 0;

		std::atomic_int mn_OpenCount{0}, mn_CloseCount{0}, mn_FailCount{0};

		void SetErrorCode(DWORD nCode)
		{
			mn_ErrCode = nCode;
		}

		/// Allocate buffer *mp_Out* and store (optional) *pPart* read before
		/// @param nAllSize size in bytes
		/// @param pPart Partially retrieved head of the message
		/// @param cbPart Size of pPart in bytes
		void* AllocateBuffer(size_t nAllSize, void* pPart = nullptr, const size_t cbPart = 0)
		{
			if (!mp_Out || mn_MaxOutSize < nAllSize)
			{
				void* ptrNew = HeapAlloc(mh_Heap, 0, nAllSize);

				if (!ptrNew)
				{
					_ASSERTE(ptrNew!=nullptr);  // -V547
					msprintf(ms_Error, countof(ms_Error), L"%s: Can't allocate %u bytes!", ms_Module, nAllSize);
					return nullptr;
				}

				if (pPart && cbPart)
				{
					memmove(ptrNew, pPart, cbPart); //-V106
				}

				ReleaseBuffer();

				mn_MaxOutSize = nAllSize;
				mp_Out = ptrNew;
			}
			return mp_Out;
		}

		void SetHandle(HANDLE hPipe)
		{
			_ASSERTE(mh_Pipe == nullptr);
			Close();
			mh_Pipe = hPipe;
		}

	public:
		MPipeBase()
			: mh_Heap(GetProcessHeap())
		{
			_ASSERTE(mh_Heap!=nullptr);
			const MModule kernel(L"kernel32.dll");
			kernel.GetProcAddress("CancelIo", cancelIo_);
			kernel.GetProcAddress("CancelIoEx", cancelIoEx_);
		}

		MPipeBase(const MPipeBase&) = delete;
		MPipeBase(MPipeBase&&) = delete;
		MPipeBase& operator=(const MPipeBase&) = delete;
		MPipeBase& operator=(MPipeBase&&) = delete;

		virtual ~MPipeBase()
		{
			Close();
			MPipeBase::ReleaseBuffer();
			_ASSERTE(mn_CloseCount == mn_OpenCount);
		}

		void SetTermEvent(HANDLE hTermEvent)
		{
			mh_TermEvent = hTermEvent;
		}

		virtual void ReleaseBuffer()
		{
			if (mp_Out)
			{
				HeapFree(mh_Heap, 0, mp_Out);
				mp_Out = nullptr;
			}
		}

		void Close()
		{
			if (mh_Pipe && mh_Pipe != INVALID_HANDLE_VALUE)
			{
				++mn_CloseCount;
				if (cancelIoEx_)
					cancelIoEx_(mh_Pipe, nullptr);
				else if (cancelIo_)
					cancelIo_(mh_Pipe);
				CloseHandle(mh_Pipe);
			}
			if (m_Ovl.hEvent)
			{
				SafeCloseHandle(m_Ovl.hEvent);
				m_Ovl.hEvent = nullptr;
			}

			mh_Pipe = nullptr;
		}

		// Informational
		LPCWSTR GetErrorText() const
		{
			return ms_Error;
		}

		DWORD GetErrorCode() const
		{
			return mn_ErrCode;
		}
};

template <class T_IN, class T_OUT>
class MPipe final : public MPipeBase
{
	protected:
		WCHAR ms_PipeName[MAX_PATH] = L"";
		T_IN m_In = {}; // informational
		T_OUT m_Tmp = {};

	public:
		MPipe()
		{
		};

		void ReleaseBuffer() override
		{
			if (mp_Out && mp_Out != &m_Tmp)
			{
				if (mh_Heap)
					HeapFree(mh_Heap, 0, mp_Out);
				else
					_ASSERTE(mh_Heap != nullptr);  // -V547
			}
			mp_Out = nullptr;
		}

		void InitName(const wchar_t* asModule, const wchar_t *aszTemplate, const wchar_t *Parm1, DWORD Parm2)
		{
			msprintf(ms_PipeName, countof(ms_PipeName), aszTemplate, Parm1, Parm2);
			lstrcpynW(ms_Module, asModule, countof(ms_Module)); //-V303

			if (mh_Pipe)
				Close();
		}

		bool Open()
		{
			if (mh_Pipe && mh_Pipe != INVALID_HANDLE_VALUE)
				return true;

			mb_Overlapped = (mh_TermEvent != nullptr);
			if (mb_Overlapped && (m_Ovl.hEvent == nullptr))
			{
				m_Ovl.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
			}

			mh_Pipe = ExecuteOpenPipe(ms_PipeName, ms_Error, ms_Module, 0, 0, mb_Overlapped, mh_TermEvent);

			if (mh_Pipe == INVALID_HANDLE_VALUE)
			{
				SetErrorCode(GetLastError());
				++mn_FailCount;
				_ASSERTE(mh_Pipe != INVALID_HANDLE_VALUE);
				mh_Pipe = nullptr;
			}

			++mn_OpenCount;
			return (mh_Pipe!=nullptr);
		}

		bool Transact(const T_IN* apIn, DWORD anInSize, const T_OUT** rpOut)
		{
			if (!apIn || !rpOut)
			{
				_ASSERTE(apIn && rpOut);
				return false;
			}

			ms_Error[0] = 0;
			*rpOut = &m_Tmp;

			// ReSharper disable once CppDeclaratorNeverUsed
			DEBUGTEST(DWORD nOpenTick = GetTickCount());

			// If pipe is opened already, function just returns true
			if (!Open())
				return false;

			_ASSERTE(sizeof(m_In) >= sizeof(CESERVER_REQ_HDR));
			_ASSERTE(sizeof(T_OUT) >= sizeof(CESERVER_REQ_HDR));

			// Informational, last transaction
			memmove_s(&m_In, sizeof(m_In), apIn, std::min<size_t>(anInSize, sizeof(m_In)));

			bool fSuccess = false;
			DWORD cbRead, cbReadBuf, nOverlappedWait;

			// Send a message to the pipe server and read the response.
			cbRead = 0;
			T_OUT* ptrOut;

			if (mp_Out && (mn_MaxOutSize > sizeof(m_Tmp)))
			{
				ptrOut = static_cast<T_OUT*>(mp_Out);
				cbReadBuf = static_cast<DWORD>(std::min<size_t>(mn_MaxOutSize, std::numeric_limits<DWORD>::max()));
				*rpOut = static_cast<T_OUT*>(mp_Out);
			}
			else
			{
				ptrOut = &m_Tmp;
				cbReadBuf = static_cast<DWORD>(sizeof(m_Tmp));
			}

			// ReSharper disable once CppLocalVariableMayBeConst
			DEBUGTEST(DWORD nStartTick = GetTickCount());

			//SetLastError(0);

			// #PIPE Зависала при закрытии консоли?
			fSuccess = TransactNamedPipe(
				mh_Pipe, const_cast<void*>(static_cast<const void*>(apIn)), anInSize,
				ptrOut, cbReadBuf, &cbRead, mb_Overlapped ? &m_Ovl : nullptr);
			DWORD dwErr = fSuccess ? 0 : GetLastError();
			SetErrorCode(dwErr);

			// ReSharper disable once CppDeclaratorNeverUsed
			DEBUGTEST(DWORD nStartTick2 = GetTickCount());
			// ReSharper disable once CppTooWideScope
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
					++mn_FailCount;
					Close();
					return false;
				}

				fSuccess = GetOverlappedResult(mh_Pipe, &m_Ovl, &cbRead, FALSE);
				dwErr = GetLastError();
				SetErrorCode(dwErr);

				DEBUGTEST(nTickOvl = GetTickCount());
			}

			#ifdef _DEBUG
			const DWORD nEndTick = GetTickCount();
			const DWORD nDelta = nEndTick - nStartTick;
			// ReSharper disable once CppDeclaratorNeverUsed
			int nTermWait = mh_TermEvent ? WaitForSingleObject(mh_TermEvent, 0) : -1;
			if (nDelta >= TRANSACT_WARN_TIMEOUT && !IsDebuggerPresent())
			{
				//_ASSERTE(nDelta <= TRANSACT_WARN_TIMEOUT);
			}
			#endif

			if (!fSuccess && dwErr == ERROR_BROKEN_PIPE)
			{
				// Сервер не вернул данных, но обработал команду
				++mn_FailCount;
				Close(); // Раз пайп закрыт - прибиваем хэндл
				return true;
			}

			if (!fSuccess && (dwErr != ERROR_MORE_DATA))
			{
				DEBUGSTR(L" - FAILED!\n");
				DWORD nCmd = 0;

				if (anInSize >= sizeof(CESERVER_REQ_HDR))
					nCmd = static_cast<const CESERVER_REQ_HDR*>(apIn)->nCmd;

				msprintf(ms_Error, countof(ms_Error), L"%s: TransactNamedPipe failed, Cmd=%i, ErrCode=%u!", ms_Module, nCmd, dwErr);
				++mn_FailCount;
				Close(); // Поскольку произошла неизвестная ошибка - пайп лучше закрыть (чтобы потом переоткрыть)
				return false;
			}

			// Пошли проверки заголовка
			if (cbRead < sizeof(CESERVER_REQ_HDR))
			{
				_ASSERTE(cbRead >= sizeof(CESERVER_REQ_HDR));  // -V547
				msprintf(ms_Error, countof(ms_Error),
				          L"%s: Only %i bytes received, required %i bytes at least!",
				          ms_Module, cbRead, static_cast<DWORD>(sizeof(CESERVER_REQ_HDR)));
				return false;
			}

			if (static_cast<const CESERVER_REQ_HDR*>(apIn)->nCmd != static_cast<const CESERVER_REQ_HDR*>(&m_In)->nCmd)
			{
				_ASSERTE((static_cast<const CESERVER_REQ_HDR*>(apIn))->nCmd == (static_cast<const CESERVER_REQ_HDR*>(&m_In))->nCmd);
				msprintf(ms_Error, countof(ms_Error),
				          L"%s: Invalid CmdID=%i received, required CmdID=%i!",
				          ms_Module, static_cast<const CESERVER_REQ_HDR*>(apIn)->nCmd, static_cast<const CESERVER_REQ_HDR*>(&m_In)->nCmd);
				return false;
			}

			if (static_cast<CESERVER_REQ_HDR*>(ptrOut)->nVersion != CESERVER_REQ_VER)
			{
				_ASSERTE((static_cast<CESERVER_REQ_HDR*>(ptrOut))->nVersion == CESERVER_REQ_VER);
				msprintf(ms_Error, countof(ms_Error),
				          L"%s: Old version packet received (%i), required (%i)!",
				          ms_Module, static_cast<CESERVER_REQ_HDR*>(ptrOut)->nCmd, CESERVER_REQ_VER);
				return false;
			}

			if (static_cast<CESERVER_REQ_HDR*>(ptrOut)->cbSize < cbRead)
			{
				_ASSERTE((static_cast<CESERVER_REQ_HDR*>(ptrOut))->cbSize >= cbRead);
				msprintf(ms_Error, countof(ms_Error),
				          L"%s: Invalid packet size (%i), must be greater or equal to %i!",
				          ms_Module, static_cast<CESERVER_REQ_HDR*>(ptrOut)->cbSize, cbRead);
				return false;
			}

			#ifdef _DEBUG
			DWORD nMoreDataTick[6] = {GetTickCount()};  // -V1009
			#endif

			if (dwErr == ERROR_MORE_DATA)
			{
				int nAllSize = static_cast<CESERVER_REQ_HDR*>(ptrOut)->cbSize;

				if (!mp_Out || static_cast<int>(mn_MaxOutSize) < nAllSize)
				{
					if (!AllocateBuffer(nAllSize, ptrOut, cbRead))
					{
						return false;
					}
				}
				else if (ptrOut == &m_Tmp)
				{
					memmove(mp_Out, &m_Tmp, cbRead); //-V106
				}

				*rpOut = static_cast<T_OUT*>(mp_Out);
				LPBYTE ptrData = static_cast<LPBYTE>(mp_Out) + cbRead; //-V104
				nAllSize -= cbRead;

				while (nAllSize>0)
				{
					DEBUGTEST(nMoreDataTick[1] = GetTickCount());
					// Read from the pipe if there is more data in the message.
					//WARNING: Если в буфере пайпа данных меньше чем nAllSize - повиснем!
					fSuccess = ReadFile(mh_Pipe, ptrData, nAllSize, &cbRead, mb_Overlapped ? &m_Ovl : nullptr);
					dwErr = fSuccess ? 0 : GetLastError();
					SetErrorCode(dwErr);

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
							++mn_FailCount;
							Close();
							return false;
						}

						fSuccess = GetOverlappedResult(mh_Pipe, &m_Ovl, &cbRead, FALSE);
						dwErr = GetLastError();
						SetErrorCode(dwErr);
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
					_ASSERTE(nAllSize==0);  // -V547
					msprintf(ms_Error, countof(ms_Error), L"%s: Can't read %u bytes!", ms_Module, nAllSize);
					return false;
				}

				if (dwErr == ERROR_MORE_DATA)
				{
					_ASSERTE(dwErr != ERROR_MORE_DATA);  // -V547
					//	BYTE cbTemp[512];
					//	while (1) {
					//		fSuccess = ReadFile( mh_Pipe, cbTemp, 512, &cbRead, nullptr);
					//		dwErr = GetLastError();
					//		SaveErrorCode(dwErr);
					//		// Exit if an error other than ERROR_MORE_DATA occurs.
					//		if ( !fSuccess && (dwErr != ERROR_MORE_DATA))
					//			break;
					//	}
				}

				DEBUGTEST(nMoreDataTick[4] = GetTickCount());

				// Is that required?
				// ReSharper disable once CppAssignedValueIsNeverUsed
				fSuccess = FlushFileBuffers(mh_Pipe);

				DEBUGTEST(nMoreDataTick[5] = GetTickCount());
			}

			return true;
		};
};

class MPipeDual final
{
protected:
	MPipeBase read_pipe, write_pipe;
	HANDLE mh_client_read = nullptr, mh_client_write = nullptr;

public:
	/// ctor for server side
	MPipeDual()
	{
		HANDLE h_read[2] = {}, h_write[2] = {};
		BOOL created[2] = {};
		for (size_t i = 0; i < 2; ++i)
		{
			created[i] = CreatePipe(h_read+i, h_write+i, LocalSecurity(), 0);
		}
		if (!created[0] || !created[1])
		{
			_ASSERTE(created[0] && created[1]);
			for (size_t i = 0; i < 2; ++i)
			{
				if (!created[i]) continue;
				CloseHandle(h_read[i]);
				CloseHandle(h_write[i]);
			}
			throw std::invalid_argument("::CreatePipe failed");
		}

		read_pipe.SetHandle(h_read[0]);
		mh_client_write = h_write[0];
		write_pipe.SetHandle(h_write[1]);
		mh_client_read = h_read[1];
	}

	/// ctor for client side
	MPipeDual(HANDLE hRead, HANDLE hWrite)
	{
		read_pipe.SetHandle(hRead);
		write_pipe.SetHandle(hWrite);
	}

	MPipeDual(const MPipeDual&) = delete;
	MPipeDual(MPipeDual&&) = delete;
	MPipeDual& operator=(const MPipeDual&) = delete;
	MPipeDual& operator=(MPipeDual&&) = delete;

	/// dtor, close handles
	~MPipeDual()
	{
		Close();
	}

	void Close()
	{
		if (mh_client_read)
			CloseHandle(mh_client_read);
		if (mh_client_write)
			CloseHandle(mh_client_write);

		read_pipe.Close();
		write_pipe.Close();
	}

	/// Prepare handles for target *clientPID* process
	/// @returns pair of {read,write} handles for target process
	std::pair<HANDLE,HANDLE> GetClientHandles(DWORD clientPID)
	{
		if (!mh_client_read || !mh_client_write)
		{
			_ASSERTE(FALSE && "GetClientHandles already called");
			throw std::invalid_argument("GetClientHandles already called");
		}

		std::pair<HANDLE,HANDLE> rc = {};
		HANDLE dst[2] = {}, src[2] = {mh_client_read, mh_client_write};
		if (DuplicateHandleForPID(clientPID, std::size(src), src, dst))
		{
			rc.first = dst[0];
			rc.second = dst[1];
			CloseHandle(mh_client_read);
			mh_client_read = nullptr;
			CloseHandle(mh_client_write);
			mh_client_write = nullptr;
		}
		return rc;
	}

	/// Write operation may be blocking
	/// @param data - pointer to the data to write
	/// @param size - explicitly 32-bit because the code is used in both bitnesses
	bool Write(const void* data, uint32_t size)
	{
		if (!write_pipe.mh_Pipe)
		{
			// The error have to be already processed
			_ASSERTE(FALSE && "pipe was closed");
			return false;
		}

		DWORD written = 0;

		if (!WriteFile(write_pipe.mh_Pipe, &size, sizeof(size), &written, nullptr) || written != sizeof(size))
		{
			write_pipe.SetErrorCode(::GetLastError());
			msprintf(write_pipe.ms_Error, countof(write_pipe.ms_Error),
				L"%s: Write::WriteFile failed, ErrCode=%u!", write_pipe.ms_Module, write_pipe.mn_ErrCode);
			write_pipe.Close();
			return false;
		}

		if (!WriteFile(write_pipe.mh_Pipe, data, size, &written, nullptr) || written != size)
		{
			write_pipe.SetErrorCode(::GetLastError());
			msprintf(write_pipe.ms_Error, countof(write_pipe.ms_Error),
				L"%s: Write::WriteFile failed, ErrCode=%u!", write_pipe.ms_Module, write_pipe.mn_ErrCode);
			write_pipe.Close();
			return false;
		}

		return true;
	}

	/// Read operation returns {nullptr} immediately if there is no data (packet) pending
	/// @result pair of {data,data_size}, data is valid until next Read call
	std::pair<void*,uint32_t> Read(bool blocking)
	{
		if (!write_pipe.mh_Pipe)
		{
			// The error have to be already processed
			_ASSERTE(FALSE && "pipe was closed");
			return std::pair<void*,uint32_t>{};
		}

		std::pair<void*,uint32_t> rc = {};
		DWORD avail = 0, read = 0; uint32_t data_size = 0;

		if (blocking || (PeekNamedPipe(read_pipe.mh_Pipe, nullptr, 0, nullptr, &avail, nullptr) && avail >= sizeof(data_size)))
		{
			if (!ReadFile(read_pipe.mh_Pipe, &data_size, sizeof(data_size), &read, nullptr) || read != sizeof(data_size))
			{
				read_pipe.SetErrorCode(::GetLastError());
				msprintf(read_pipe.ms_Error, countof(read_pipe.ms_Error),
					L"%s: Read::ReadFile failed, ErrCode=%u!", read_pipe.ms_Module, read_pipe.mn_ErrCode);
				read_pipe.Close();
				return std::pair<void*,uint32_t>{};
			}

			rc.first = read_pipe.AllocateBuffer(std::max<uint32_t>(data_size, 64));
			if (!ReadFile(read_pipe.mh_Pipe, rc.first, data_size, &read, nullptr) || read != data_size)
			{
				read_pipe.SetErrorCode(::GetLastError());
				msprintf(read_pipe.ms_Error, countof(read_pipe.ms_Error),
					L"%s: Read::ReadFile failed, ErrCode=%u!", read_pipe.ms_Module, read_pipe.mn_ErrCode);
				read_pipe.Close();
				return std::pair<void*,uint32_t>{};
			}
			rc.second = read;
		}

		return rc;
	}
};
