
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
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
#include <wchar.h>
#include "common.hpp"


// WinAPI wrappers
BOOL apiSetForegroundWindow(HWND ahWnd);
BOOL apiShowWindow(HWND ahWnd, int anCmdShow);
BOOL apiShowWindowAsync(HWND ahWnd, int anCmdShow);
#ifdef _DEBUG
void getWindowInfo(HWND ahWnd, wchar_t* rsInfo/*[1024]*/);
#endif

// Some WinAPI related functions
wchar_t* GetShortFileNameEx(LPCWSTR asLong);
BOOL IsUserAdmin();


//------------------------------------------------------------------------
///| Section |////////////////////////////////////////////////////////////
//------------------------------------------------------------------------
class MSectionLock;

class MSection
{
protected:
	CRITICAL_SECTION m_cs;
	DWORD mn_TID; // устанавливается только после EnterCriticalSection
	int mn_Locked;
	BOOL mb_Exclusive;
	HANDLE mh_ReleaseEvent;
	friend class MSectionLock;
	DWORD mn_LockedTID[10];
	int   mn_LockedCount[10];
public:
	MSection() {
		mn_TID = 0; mn_Locked = 0; mb_Exclusive = FALSE;
		ZeroStruct(mn_LockedTID); ZeroStruct(mn_LockedCount);
		InitializeCriticalSection(&m_cs);
		mh_ReleaseEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		_ASSERTE(mh_ReleaseEvent!=NULL);
		if (mh_ReleaseEvent) ResetEvent(mh_ReleaseEvent);
	};
	~MSection() {
		DeleteCriticalSection(&m_cs);
		if (mh_ReleaseEvent) {
			CloseHandle(mh_ReleaseEvent); mh_ReleaseEvent = NULL;
		}
	};
public:
	void ThreadTerminated(DWORD dwTID) {
		for (int i=1; i<10; i++) {
			if (mn_LockedTID[i] == dwTID) {
				mn_LockedTID[i] = 0;
				if (mn_LockedCount[i] != 0) {
					_ASSERTE(mn_LockedCount[i] == 0);
				}
				break;
			}
		}
	};
protected:
	void AddRef(DWORD dwTID) {
		mn_Locked ++; // увеличиваем счетчик nonexclusive locks
		_ASSERTE(mn_Locked>0);
		ResetEvent(mh_ReleaseEvent); // На всякий случай сбросим Event
		int j = -1; // будет -2, если ++ на существующий, иначе - +1 на пустой
		for (int i=1; i<10; i++) {
			if (mn_LockedTID[i] == dwTID) {
				mn_LockedCount[i] ++; 
				j = -2; 
				break;
			} else if (j == -1 && mn_LockedTID[i] == 0) {
				mn_LockedTID[i] = dwTID;
				mn_LockedCount[i] ++; 
				j = i;
				break;
			}
		}
		if (j == -1) { // Этого быть не должно
			_ASSERTE(j != -1);
		}
	};
	int ReleaseRef(DWORD dwTID) {
		_ASSERTE(mn_Locked>0);
		int nInThreadLeft = 0;
		if (mn_Locked > 0) mn_Locked --;
		if (mn_Locked == 0)
			SetEvent(mh_ReleaseEvent); // Больше nonexclusive locks не осталось
		for (int i=1; i<10; i++) {
			if (mn_LockedTID[i] == dwTID) {
				mn_LockedCount[i] --; 
				if ((nInThreadLeft = mn_LockedCount[i]) == 0)
					mn_LockedTID[i] = 0; // Иначе при динамически создаваемых нитях - 10 будут в момент использованы
				break;
			}
		}
		return nInThreadLeft;
	};
	void WaitUnlocked(DWORD dwTID, DWORD anTimeout) {
		DWORD dwStartTick = GetTickCount();
		int nSelfCount = 0;
		for (int i=1; i<10; i++) {
			if (mn_LockedTID[i] == dwTID) {
				nSelfCount = mn_LockedCount[i];
				break;
			}
		}
		while (mn_Locked > nSelfCount) {
			#ifdef _DEBUG
			DEBUGSTR(L"!!! Waiting for exclusive access\n");

			DWORD nWait = 
			#endif

			WaitForSingleObject(mh_ReleaseEvent, 10);

			DWORD dwCurTick = GetTickCount();
			DWORD dwDelta = dwCurTick - dwStartTick;

			if (anTimeout != (DWORD)-1) {
				if (dwDelta > anTimeout) {
					#ifndef CSECTION_NON_RAISE
					_ASSERTE(dwDelta<=anTimeout);
					#endif
					break;
				}
			}
			#ifdef _DEBUG
			else if (dwDelta > 3000) {
				#ifndef CSECTION_NON_RAISE
				_ASSERTE(dwDelta <= 3000);
				#endif
				break;
			}
			#endif
		}
	};
	bool MyEnterCriticalSection(DWORD anTimeout) {
		//EnterCriticalSection(&m_cs); 
		// дождаться пока секцию отпустят

		// НАДА. Т.к. может быть задан nTimeout (для DC)
		DWORD dwTryLockSectionStart = GetTickCount(), dwCurrentTick;

		if (!TryEnterCriticalSection(&m_cs)) {
			Sleep(10);
			while (!TryEnterCriticalSection(&m_cs)) {
				Sleep(10);
				DEBUGSTR(L"TryEnterCriticalSection failed!!!\n");

				dwCurrentTick = GetTickCount();
				if (anTimeout != (DWORD)-1) {
					if (((dwCurrentTick - dwTryLockSectionStart) > anTimeout)) {
						#ifndef CSECTION_NON_RAISE
						_ASSERTE((dwCurrentTick - dwTryLockSectionStart) <= anTimeout);
						#endif
						return false;
					}
				}
				#ifdef _DEBUG
				else if ((dwCurrentTick - dwTryLockSectionStart) > 3000) {
					#ifndef CSECTION_NON_RAISE
					_ASSERTE((dwCurrentTick - dwTryLockSectionStart) <= 3000);
					#endif
					dwTryLockSectionStart = GetTickCount();
				}
				#endif
			}
		}

		return true;
	}
	BOOL Lock(BOOL abExclusive, DWORD anTimeout=-1/*, BOOL abRelockExclusive=FALSE*/) {
		DWORD dwTID = GetCurrentThreadId();
		
		// Может эта нить уже полностью заблокирована?
		if (mb_Exclusive && dwTID == mn_TID) {
			return FALSE; // Уже, но Unlock делать не нужно!
		}
		
		if (!abExclusive) {
			// Быстрая блокировка, не запрещающая чтение другим нитям.
			// Запрещено только изменение (пересоздание буфера например)
			AddRef(dwTID);
			
			// Если другая нить уже захватила exclusive
			if (mb_Exclusive) {
				int nLeft = ReleaseRef(dwTID); // Иначе можем попасть на взаимную блокировку
				if (nLeft > 0) {
					_ASSERTE(nLeft == 0);
				}

				DEBUGSTR(L"!!! Failed non exclusive lock, trying to use CriticalSection\n");
				bool lbEntered = MyEnterCriticalSection(anTimeout); // дождаться пока секцию отпустят
				_ASSERTE(!mb_Exclusive); // После LeaveCriticalSection mb_Exclusive УЖЕ должен быть сброшен

				AddRef(dwTID); // Возвращаем блокировку

				// Но поскольку нам нужен только nonexclusive lock
				if (lbEntered)
					LeaveCriticalSection(&m_cs);
			}
		} else {
			// Требуется Exclusive Lock

			#ifdef _DEBUG
			if (mb_Exclusive) {
				// Этого надо стараться избегать
				DEBUGSTR(L"!!! Exclusive lock found in other thread\n");
			}
			#endif
			
			// Если есть ExclusiveLock (в другой нити) - дождется сама EnterCriticalSection
			#ifdef _DEBUG
			BOOL lbPrev = mb_Exclusive;
			DWORD nPrevTID = mn_TID;
			#endif
			mb_Exclusive = TRUE; // Сразу, чтобы в nonexclusive не нарваться
			TODO("Need to check, if MyEnterCriticalSection failed on timeout!\n");
			MyEnterCriticalSection(anTimeout);
			_ASSERTE(!(lbPrev && mb_Exclusive)); // После LeaveCriticalSection mb_Exclusive УЖЕ должен быть сброшен
			mb_Exclusive = TRUE; // Флаг могла сбросить другая нить, выполнившая Leave
			mn_TID = dwTID; // И запомним, в какой нити это произошло
			_ASSERTE(mn_LockedTID[0] == 0 && mn_LockedCount[0] == 0);
			mn_LockedTID[0] = dwTID;
			mn_LockedCount[0] ++;

			/*if (abRelockExclusive) {
				ReleaseRef(dwTID); // Если до этого был nonexclusive lock
			}*/

			// B если есть nonexclusive locks - дождаться их завершения
			if (mn_Locked) {
				//WARNING: Тут есть шанс наколоться, если сначала был NonExclusive, а потом в этой же нити - Exclusive
				// В таких случаях нужно вызывать с параметром abRelockExclusive
				WaitUnlocked(dwTID, anTimeout);
			}
		}
		
		return TRUE;
	};
	void Unlock(BOOL abExclusive) {
		DWORD dwTID = GetCurrentThreadId();
		if (abExclusive) {
			_ASSERTE(dwTID == dwTID && mb_Exclusive);
			_ASSERTE(mn_LockedTID[0] == dwTID);
			mb_Exclusive = FALSE; mn_TID = 0;
			mn_LockedTID[0] = 0; mn_LockedCount[0] --;
			LeaveCriticalSection(&m_cs);
		} else {
			ReleaseRef(dwTID);
		}
	};
};

class MSectionThread
{
protected:
	MSection* mp_S;
	DWORD mn_TID;
public:
	MSectionThread(MSection* apS) {
		mp_S = apS; mn_TID = GetCurrentThreadId();
	};
	~MSectionThread() {
		if (mp_S && mn_TID)
			mp_S->ThreadTerminated(mn_TID);
	};
};

class MSectionLock
{
protected:
	MSection* mp_S;
	BOOL mb_Locked, mb_Exclusive;
public:
	BOOL Lock(MSection* apS, BOOL abExclusive=FALSE, DWORD anTimeout=-1) {
		if (mb_Locked && apS == mp_S && (abExclusive == mb_Exclusive || mb_Exclusive))
			return FALSE; // уже заблокирован
		_ASSERTE(!mb_Locked);
		mb_Exclusive = abExclusive;
		mp_S = apS;
		mb_Locked = mp_S->Lock(mb_Exclusive, anTimeout);
		return mb_Locked;
	};
	BOOL RelockExclusive(DWORD anTimeout=-1) {
		if (mb_Locked && mb_Exclusive) return FALSE; // уже
		// Чистый ReLock делать нельзя. Виснут другие нити, которые тоже запросили ReLock
		Unlock();
		mb_Exclusive = TRUE;
		mb_Locked = mp_S->Lock(mb_Exclusive, anTimeout);
		return mb_Locked;
	};
	void Unlock() {
		if (mp_S && mb_Locked) {
			mp_S->Unlock(mb_Exclusive);
			mb_Locked = FALSE;
		}
	};
	BOOL isLocked() {
		return (mp_S!=NULL) && mb_Locked;
	};
public:
	MSectionLock() {
		mp_S = NULL; mb_Locked = FALSE; mb_Exclusive = FALSE;
	};
	~MSectionLock() {
		if (mb_Locked) Unlock();
	};
};


/* Console Handles */
class MConHandle {
private:
	wchar_t   ms_Name[10];
	HANDLE    mh_Handle;
	MSection  mcs_Handle;
	BOOL      mb_OpenFailed;
	DWORD     mn_LastError;
	DWORD     mn_StdMode;

public:
	operator const HANDLE();

public:
	void Close();

public:
	MConHandle(LPCWSTR asName);
	~MConHandle();
};


template <class T>
class MFileMapping
{
protected:
	HANDLE mh_Mapping;
	BOOL mb_WriteAllowed;
	int mn_Size;
	T* mp_Data; //WARNING!!! Доступ может быть только на чтение!
	wchar_t ms_MapName[MAX_PATH];
	DWORD mn_LastError;
	wchar_t ms_Error[MAX_PATH*2];
public:
	T* Ptr() {
		return mp_Data;
	};
	operator T*() {
		return mp_Data;
	};
	bool IsValid() {
		return (mp_Data!=NULL);
	};
	LPCWSTR GetErrorText() {
		return ms_Error;
	};
	bool SetFrom(const T* pSrc, int nSize=-1) {
		if (!IsValid() || !nSize) return false;
		if (nSize<0) nSize = sizeof(T);
		bool lbChanged = (memcmp(mp_Data, pSrc, nSize)!=0);
		memmove(mp_Data, pSrc, nSize);
		return lbChanged;
	}
	bool GetTo(T* pDst, int nSize=-1) {
		if (!IsValid() || !nSize) return false;
		if (nSize<0) nSize = sizeof(T);
		//потому, что T может быть описан как const - (void*)
		memmove((void*)pDst, mp_Data, nSize);
		return true;
	}
public:
	void InitName(const wchar_t *aszTemplate,DWORD Parm1=0,DWORD Parm2=0) {
		if (mh_Mapping) CloseMap();
		wsprintfW(ms_MapName, aszTemplate, Parm1, Parm2);
	};
	void ClosePtr() {
		if (mp_Data) {
			UnmapViewOfFile(mp_Data);
			mp_Data = NULL;
		}
	};
	void CloseMap() {
		if (mp_Data) ClosePtr();
		if (mh_Mapping) {
			CloseHandle(mh_Mapping);
			mh_Mapping = NULL;
		}
		mh_Mapping = NULL; mb_WriteAllowed = FALSE; mp_Data = NULL; 
		mn_Size = -1; mn_LastError = 0;
	};
protected:
	// mn_Size и nSize используется фактически только при CreateFileMapping или операциях копирования
	T* InternalOpenCreate(BOOL abCreate,BOOL abReadWrite,int nSize) {
		if (mh_Mapping) CloseMap();
		mn_LastError = 0; ms_Error[0] = 0;
		_ASSERTE(mh_Mapping==NULL && mp_Data==NULL);
		_ASSERTE(nSize==-1 || nSize>=sizeof(T));

		if (ms_MapName[0] == 0) {
			_ASSERTE(ms_MapName[0]!=0);
			lstrcpyW (ms_Error, L"Internal error. Mapping file name was not specified.");
			return NULL;
		} else {
			mn_Size = (nSize<=0) ? sizeof(T) : nSize;
			mb_WriteAllowed = abCreate || abReadWrite;
			if (abCreate) {
				mh_Mapping = CreateFileMapping(INVALID_HANDLE_VALUE, 
					NullSecurity(), PAGE_READWRITE, 0, mn_Size, ms_MapName);
			} else {
				mh_Mapping = OpenFileMapping(FILE_MAP_READ, FALSE, ms_MapName);
			}
			if (!mh_Mapping) {
				mn_LastError = GetLastError();
				wsprintfW (ms_Error, L"Can't %s console data file mapping. ErrCode=0x%08X\n%s", 
						abCreate ? L"create" : L"open", mn_LastError, ms_MapName);
			} else {
				DWORD nFlags = mb_WriteAllowed ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ;
				mp_Data = (T*)MapViewOfFile(mh_Mapping, nFlags,0,0,0);
				if (!mp_Data) {
					mn_LastError = GetLastError();
					wsprintfW (ms_Error, L"Can't map console info (%s). ErrCode=0x%08X\n%s", 
							mb_WriteAllowed ? L"ReadWrite" : L"Read" ,mn_LastError, ms_MapName);
				}
			}
		}
		return mp_Data;
	};
public:
	T* Create(int nSize=-1) {
		_ASSERTE(nSize==-1 || nSize>=sizeof(T));
		return InternalOpenCreate(TRUE/*abCreate*/,TRUE/*abReadWrite*/,nSize);
	};
	T* Open(BOOL abReadWrite=FALSE/*FALSE - только Read*/,int nSize=-1) {
		_ASSERTE(nSize==-1 || nSize>=sizeof(T));
		return InternalOpenCreate(FALSE/*abCreate*/,abReadWrite,nSize);
	};
public:
	MFileMapping() {
		mh_Mapping = NULL; mb_WriteAllowed = FALSE; mp_Data = NULL; 
		mn_Size = -1; ms_MapName[0] = ms_Error[0] = 0; mn_LastError = 0;
	};
	~MFileMapping() {
		if (mh_Mapping) CloseMap();
	};
};

class MEvent
{
protected:
	WCHAR  ms_EventName[MAX_PATH];
	HANDLE mh_Event;
	DWORD  mn_LastError;
public:
	MEvent();
	~MEvent();
	void InitName(const wchar_t *aszTemplate, DWORD Parm1=0);
public:
	void   Close();
	HANDLE Open();
	DWORD  Wait(DWORD dwMilliseconds, BOOL abAutoOpen=TRUE);
	HANDLE GetHandle();
};

template <class T_IN, class T_OUT>
class MPipe
{
protected:
	WCHAR ms_PipeName[MAX_PATH], ms_Module[32], ms_Error[MAX_PATH*2];
	HANDLE mh_Pipe, mh_Heap;
	T_IN m_In; // для справки...
	T_OUT* mp_Out; DWORD mn_OutSize, mn_MaxOutSize;
	T_OUT m_Tmp;
	//DWORD mdw_Timeout;
public:
	MPipe() {
		ms_PipeName[0] = ms_Module[0] = 0;
		mh_Pipe = NULL; memset(&m_In, 0, sizeof(m_In));
		mp_Out = NULL; mn_OutSize = mn_MaxOutSize = 0;
		mh_Heap = GetProcessHeap();
		_ASSERTE(mh_Heap!=NULL);
	};
	void SetTimeout(DWORD anTimeout) {
		//TODO: Если anTimeout!=-1 - создавать нить и выполнять команду в ней. Ожидать нить не более и прибить ее, если пришел Timeout
	}
	~MPipe() {
		if (mp_Out && mp_Out != &m_Tmp) {
			_ASSERTE(mh_Heap!=NULL);
			HeapFree(mh_Heap, 0, mp_Out);
			mp_Out = NULL;
		}
	};
	void Close() {
		if (mh_Pipe && mh_Pipe != INVALID_HANDLE_VALUE)
			CloseHandle(mh_Pipe);
		mh_Pipe = NULL;
	};
	void InitName(const wchar_t* asModule, const wchar_t *aszTemplate, const wchar_t *Parm1, DWORD Parm2) {
		//va_list args;
		//va_start( args, aszTemplate );
		//vswprintf_s(ms_PipeName, countof(ms_PipeName), aszTemplate, args);
		wsprintfW(ms_PipeName, aszTemplate, Parm1, Parm2);
		lstrcpynW(ms_Module, asModule, countof(ms_Module));
		if (mh_Pipe)
			Close();
	};
	BOOL Open() {
		if (mh_Pipe && mh_Pipe != INVALID_HANDLE_VALUE)
			return TRUE;
		mh_Pipe = ExecuteOpenPipe(ms_PipeName, ms_Error, ms_Module);
		if (mh_Pipe == INVALID_HANDLE_VALUE) {
			_ASSERTE(mh_Pipe != INVALID_HANDLE_VALUE);
			mh_Pipe = NULL;
		}
		return (mh_Pipe!=NULL);
	};
	BOOL Transact(const T_IN* apIn, DWORD anInSize, const T_OUT** rpOut, DWORD* rnOutSize) {
		ms_Error[0] = 0;
		_ASSERTE(apIn && rnOutSize);

		*rnOutSize = 0;
		*rpOut = &m_Tmp;

		if (!Open()) // Пайп может быть уже открыт, функция это учитывает
			return FALSE;

		_ASSERTE(sizeof(m_In) >= sizeof(CESERVER_REQ_HDR));
		_ASSERTE(sizeof(T_OUT) >= sizeof(CESERVER_REQ_HDR));

		BOOL fSuccess = FALSE;
		DWORD cbRead, dwErr, cbReadBuf;

		// Для справки, информация о последнем запросе
		cbReadBuf = min(anInSize, sizeof(m_In));
		memmove(&m_In, apIn, cbReadBuf);

		// Send a message to the pipe server and read the response. 
		cbRead = 0; cbReadBuf = sizeof(m_Tmp);
		T_OUT* ptrOut = &m_Tmp;
		if (mp_Out && (mn_MaxOutSize > cbReadBuf)) {
			ptrOut = mp_Out;
			cbReadBuf = mn_MaxOutSize;
			*rpOut = mp_Out;
		}
		fSuccess = TransactNamedPipe( mh_Pipe, (LPVOID)apIn, anInSize, ptrOut, cbReadBuf, &cbRead, NULL);
		dwErr = fSuccess ? 0 : GetLastError();

		if (!fSuccess && dwErr == ERROR_BROKEN_PIPE) {
			// Сервер не вернул данных, но обработал команду
			Close(); // Раз пайп закрыт - прибиваем хэндл
			return TRUE;
		}
		
		if (!fSuccess && (dwErr != ERROR_MORE_DATA)) 
		{
			DEBUGSTR(L" - FAILED!\n");
			DWORD nCmd = 0;
			if (anInSize >= sizeof(CESERVER_REQ_HDR))
				nCmd = ((CESERVER_REQ_HDR*)apIn)->nCmd;
			wsprintfW(ms_Error, _T("%s: TransactNamedPipe failed, Cmd=%i, ErrCode = 0x%08X!"), ms_Module, nCmd, dwErr);
			Close(); // Поскольку произошла неизвестная ошибка - пайп лучше закрыть (чтобы потом переоткрыть)
			return FALSE;
		}

		// Пошли проверки заголовка
		if (cbRead < sizeof(CESERVER_REQ_HDR))
		{
			_ASSERTE(cbRead >= sizeof(CESERVER_REQ_HDR));
			wsprintfW(ms_Error,
				_T("%s: Only %i bytes recieved, required %i bytes at least!"), 
				ms_Module, cbRead, (DWORD)sizeof(CESERVER_REQ_HDR));
			return FALSE;
		}
		if (((CESERVER_REQ_HDR*)apIn)->nCmd != ((CESERVER_REQ_HDR*)&m_In)->nCmd) {
			_ASSERTE(((CESERVER_REQ_HDR*)apIn)->nCmd == ((CESERVER_REQ_HDR*)&m_In)->nCmd);
			wsprintfW(ms_Error,
				_T("%s: Invalid CmdID=%i recieved, required CmdID=%i!"), 
				ms_Module, ((CESERVER_REQ_HDR*)apIn)->nCmd, ((CESERVER_REQ_HDR*)&m_In)->nCmd);
			return FALSE;
		}
		if (((CESERVER_REQ_HDR*)ptrOut)->nVersion != CESERVER_REQ_VER)
		{
			_ASSERTE(((CESERVER_REQ_HDR*)ptrOut)->nVersion == CESERVER_REQ_VER);
			wsprintfW(ms_Error,
				_T("%s: Old version packet recieved (%i), required (%i)!"), 
				ms_Module, ((CESERVER_REQ_HDR*)ptrOut)->nCmd, CESERVER_REQ_VER);
			return FALSE;
		}
		if (((CESERVER_REQ_HDR*)ptrOut)->cbSize < cbRead)
		{
			_ASSERTE(((CESERVER_REQ_HDR*)ptrOut)->cbSize >= cbRead);
			wsprintfW(ms_Error,
				_T("%s: Invalid packet size (%i), must be greater or equal to %i!"), 
				ms_Module, ((CESERVER_REQ_HDR*)ptrOut)->cbSize, cbRead);
			return FALSE;
		}

		if (dwErr == ERROR_MORE_DATA)
		{
			int nAllSize = ((CESERVER_REQ_HDR*)ptrOut)->cbSize;

			if (!mp_Out || (int)mn_MaxOutSize < nAllSize) {
				T_OUT* ptrNew = (T_OUT*)HeapAlloc(mh_Heap, 0, nAllSize);
				if (!ptrNew) {
					_ASSERTE(ptrNew!=NULL);
					wsprintfW(ms_Error, _T("%s: Can't allocate %u bytes!"), ms_Module, nAllSize);
					return FALSE;
				}
				memmove(ptrNew, ptrOut, cbRead);

				if (mp_Out) HeapFree(mh_Heap, 0, mp_Out);
				mn_MaxOutSize = nAllSize;
				mp_Out = ptrNew;

			} else if (ptrOut == &m_Tmp) {
				memmove(mp_Out, &m_Tmp, cbRead);
			}
			*rpOut = mp_Out;

			LPBYTE ptrData = ((LPBYTE)mp_Out)+cbRead;

			nAllSize -= cbRead;

			while(nAllSize>0)
			{ 
				// Read from the pipe if there is more data in the message.
				//WARNING: Если в буфере пайпа данных меньше чем nAllSize - повиснем!
				fSuccess = ReadFile( mh_Pipe, ptrData, nAllSize, &cbRead, NULL);
				dwErr = fSuccess ? 0 : GetLastError();
				// Exit if an error other than ERROR_MORE_DATA occurs.
				if( !fSuccess && (dwErr != ERROR_MORE_DATA)) 
					break;
				ptrData += cbRead;
				nAllSize -= cbRead;
			}

			TODO("Может возникнуть ASSERT, если консоль была закрыта в процессе чтения");
			if (nAllSize > 0)
			{
				_ASSERTE(nAllSize==0);
				wsprintfW(ms_Error, _T("%s: Can't read %u bytes!"), ms_Module, nAllSize);
				return FALSE;
			}

			if (dwErr == ERROR_MORE_DATA)
			{
				_ASSERTE(dwErr != ERROR_MORE_DATA);
				//	BYTE cbTemp[512];
				//	while (1) {
				//		fSuccess = ReadFile( mh_Pipe, cbTemp, 512, &cbRead, NULL);
				//		dwErr = GetLastError();
				//		// Exit if an error other than ERROR_MORE_DATA occurs.
				//		if( !fSuccess && (dwErr != ERROR_MORE_DATA)) 
				//			break;
				//	}
			}

			// Надо ли это?
			fSuccess = FlushFileBuffers(mh_Pipe);
		}

		return TRUE;
	};
	LPCWSTR GetErrorText() {
		return ms_Error;
	};
};


class MSetter {
protected:
	enum tag_MSETTERTYPE {
		st_BOOL,
		st_DWORD,
	} type;

	union {
		struct {
			// st_BOOL
			BOOL *mp_BoolVal;
		};
		struct {
			// st_DWORD
			DWORD *mdw_DwordVal; DWORD mdw_OldDwordValue, mdw_NewDwordValue;
		};
	};
public:
	MSetter(BOOL* st);
	MSetter(DWORD* st, DWORD setValue);
	~MSetter();

	void Unlock();
};
