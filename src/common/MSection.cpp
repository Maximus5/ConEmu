
/*
Copyright (c) 2009-2017 Maximus5
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

//#ifdef _DEBUG
//#define USE_LOCK_SECTION
//#endif

#define HIDE_USE_EXCEPTION_INFO
#include <windows.h>
#include "defines.h"
#include "MAssert.h"
#include "MSection.h"

#ifdef _DEBUG
#define DebugString(x) //OutputDebugString(x)
#define DebugStringA(x) //OutputDebugStringA(x)
#define DEBUGSTRLOG(x) //OutputDebugStringA(x)
#else
#define DebugString(x) //OutputDebugString(x)
#define DebugStringA(x) //OutputDebugStringA(x)
#define DEBUGSTRLOG(x)
#endif

MSection::MSection()
{
	mn_TID = 0; mn_Locked = 0; mb_Exclusive = FALSE;
#ifdef _DEBUG
	mn_UnlockedExclusiveTID = 0;
#endif
	ZeroStruct(mn_LockedTID); ZeroStruct(mn_LockedCount);
	m_cs.Init();
	m_lock_cs.Init();
	mh_ReleaseEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	_ASSERTEX(mh_ReleaseEvent!=NULL);

	if (mh_ReleaseEvent) ResetEvent(mh_ReleaseEvent);
	mh_ExclusiveThread = NULL;
}

MSection::~MSection()
{
	m_cs.Close();
	m_lock_cs.Close();

	SafeCloseHandle(mh_ReleaseEvent);
	SafeCloseHandle(mh_ExclusiveThread);
}

void MSection::Process_Lock()
{
	//#ifdef USE_LOCK_SECTION
	m_lock_cs.Enter();
	//#endif
}

void MSection::Process_Unlock()
{
	//#ifdef USE_LOCK_SECTION
	m_lock_cs.Leave();
	//#endif
}

void MSection::ThreadTerminated(DWORD dwTID)
{
	for (size_t i = 1; i < countof(mn_LockedTID); i++)
	{
		if (mn_LockedTID[i] == dwTID)
		{
			mn_LockedTID[i] = 0;

			if (mn_LockedCount[i] != 0)
			{
				_ASSERTEX(mn_LockedCount[i] == 0);
			}

			break;
		}
	}
}

void MSection::AddRef(DWORD dwTID)
{
	Process_Lock();

	#ifdef _DEBUG
	int dbgCurLockCount = 0;
	if (mn_Locked == 0)
	{
		for (int d=1; d<countof(mn_LockedCount); d++)
		{
			_ASSERTE(mn_LockedCount[d]>=0);
			dbgCurLockCount += mn_LockedCount[d];
		}
		if (dbgCurLockCount != 0)
		{
			_ASSERTEX(dbgCurLockCount==0 || mn_Locked);
		}
	}
	#endif

	mn_Locked ++; // увеличиваем счетчик nonexclusive locks
	_ASSERTEX(mn_Locked>0);
	ResetEvent(mh_ReleaseEvent); // На всякий случай сбросим Event
	INT_PTR j = -1; // будет -2, если ++ на существующий, иначе - +1 на пустой

	for (size_t i = 1; i < countof(mn_LockedTID); i++)
	{
		if (mn_LockedTID[i] == dwTID)
		{
			_ASSERTE(mn_LockedCount[i]>=0);
			mn_LockedCount[i] ++;
			j = -2;
			break;
		}
	}

	if (j == -1)
	{
		for (size_t i = 1; i < countof(mn_LockedTID); i++)
		{
			if (mn_LockedTID[i] == 0)
			{
				mn_LockedTID[i] = dwTID;
				_ASSERTE(mn_LockedCount[i]>=0);
				mn_LockedCount[i] ++;
				j = i;
				break;
			}
		}
	}

	if (j == -1)  // Этого быть не должно
	{
		_ASSERTEX(j != -1);
	}

	Process_Unlock();
}

int MSection::ReleaseRef(DWORD dwTID)
{
	Process_Lock();

	_ASSERTEX(mn_Locked>0);
	int nInThreadLeft = 0;

	if (mn_Locked > 0)
		mn_Locked --;

	if (mn_Locked == 0)
	{
		SetEvent(mh_ReleaseEvent); // Больше nonexclusive locks не осталось
	}

	for (size_t i = 1; i < countof(mn_LockedTID); i++)
	{
		if (mn_LockedTID[i] == dwTID)
		{
			if (mn_LockedCount[i] > 0)
			{
				mn_LockedCount[i] --;
			}
			else
			{
				_ASSERTEX(mn_LockedCount[i] > 0);
			}

			if ((nInThreadLeft = mn_LockedCount[i]) == 0)
				mn_LockedTID[i] = 0; // Иначе при динамически создаваемых нитях - 10 будут в момент использованы

			break;
		}
	}

	if (mn_Locked == 0)
	{
		#ifdef _DEBUG
		int dbgCurLockCount = 0;
		for (int d=1; d<countof(mn_LockedCount); d++)
		{
			_ASSERTEX(mn_LockedCount[d]>=0);
			dbgCurLockCount += mn_LockedCount[d];
		}
		if (dbgCurLockCount != 0)
		{
			_ASSERTEX(dbgCurLockCount==0);
		}
		#endif
	}

	Process_Unlock();

	return nInThreadLeft;
}

void MSection::WaitUnlocked(DWORD dwTID, DWORD anTimeout)
{
	DWORD dwStartTick = GetTickCount();
	int nSelfCount = 0;

	for (size_t i = 1; i < countof(mn_LockedTID); i++)
	{
		if (mn_LockedTID[i] == dwTID)
		{
			_ASSERTEX(mn_LockedCount[i]>=0);
			nSelfCount = mn_LockedCount[i];
			break;
		}
	}

	while(mn_Locked > nSelfCount)
	{
#ifdef _DEBUG
		DEBUGSTR(L"!!! Waiting for exclusive access\n");
		DWORD nWait =
#endif
		    WaitForSingleObject(mh_ReleaseEvent, 10);
		DWORD dwCurTick = GetTickCount();
		DWORD dwDelta = dwCurTick - dwStartTick;

		if (anTimeout != (DWORD)-1)
		{
			if (dwDelta > anTimeout)
			{
				#ifndef CSECTION_NON_RAISE
				_ASSERTEX(dwDelta<=anTimeout);
				#endif
				break;
			}
		}

		#ifdef _DEBUG
		else if (dwDelta > 3000)
		{
			#ifndef CSECTION_NON_RAISE
			_ASSERTEX(dwDelta <= 3000);
			#endif
			break;
		}
		#endif
	}
}

bool MSection::MyEnterCriticalSection(DWORD anTimeout)
{
	DWORD nCurTID = GetCurrentThreadId(), nExclWait = -1;
	//EnterCriticalSection(&m_cs);
	// дождаться пока секцию отпустят
	// НАДА. Т.к. может быть задан nTimeout (для DC)
	DWORD dwTryLockSectionStart = GetTickCount(), dwCurrentTick;

	if (!m_cs.TryEnter())
	{
		Sleep(10);

		while (!m_cs.TryEnter())
		{
			if ((mn_TID != nCurTID) && mh_ExclusiveThread)
			{
				BOOL lbLocked = FALSE;

				if (mh_ExclusiveThread)
				{
					HANDLE h = mh_ExclusiveThread;
					nExclWait = WaitForSingleObject(h, 0);
					if (nExclWait != WAIT_TIMEOUT)
					{
						// Все, m_cs протух. Его нужно пересоздать

						Process_Lock();

						// Первым выполнить Process_Lock мог другой поток.
						// Нужно проверить хэндл на соответствие, если он другой
						// то на этом шаге уже не дергаться
						if (h == mh_ExclusiveThread)
						{
							_ASSERTEX(FALSE && "Exclusively locked thread was abnormally terminated?");

							SafeCloseHandle(mh_ExclusiveThread);

							lbLocked = m_cs.RecreateAndLock();

							_ASSERTEX(mn_LockedTID[0] = mn_TID);
							mn_LockedTID[0] = 0;
							if (mn_LockedCount[0] > 0)
							{
								mn_LockedCount[0] --; // на [0] mn_Locked не распространяется
							}

							mn_TID = nCurTID;
						}

						Process_Unlock();
					}
				}

				if (lbLocked)
				{
					break;
				}
			}

			Sleep(10);
			DEBUGSTR(L"TryEnterCriticalSection failed!!!\n");
			dwCurrentTick = GetTickCount();

			if (anTimeout != (DWORD)-1)
			{
				if (((dwCurrentTick - dwTryLockSectionStart) > anTimeout))
				{
					#ifndef CSECTION_NON_RAISE
					_ASSERTEX((dwCurrentTick - dwTryLockSectionStart) <= anTimeout);
					#endif
					return false;
				}
			}

			#ifdef _DEBUG
			else if ((dwCurrentTick - dwTryLockSectionStart) > 3000)
			{
				#ifndef CSECTION_NON_RAISE
				_ASSERTEX((dwCurrentTick - dwTryLockSectionStart) <= 3000);
				#endif
				dwTryLockSectionStart = GetTickCount();
			}
			#endif
		}
	}

	return true;
}

BOOL MSection::Lock(BOOL abExclusive, DWORD anTimeout/*=-1*/)
{
	DWORD dwTID = GetCurrentThreadId();

	// Может эта нить уже полностью заблокирована?
	if (mb_Exclusive && dwTID == mn_TID)
	{
		//111126 возвращался FALSE
		_ASSERTEX(!mb_Exclusive || dwTID != mn_TID);
		return TRUE; // Уже, но Unlock делать не нужно!
	}

	if (!abExclusive)
	{
		if (!mb_Exclusive)
		{
			// Быстрая блокировка, не запрещающая чтение другим нитям.
			// Запрещено только изменение (пересоздание буфера например)
			AddRef(dwTID);
		}
		// Если другая нить уже захватила exclusive
		else //if (mb_Exclusive)
		{
			_ASSERTEX(mb_Exclusive);
			//int nLeft = ReleaseRef(dwTID); // Иначе можем попасть на взаимную блокировку
			//if (nLeft > 0)
			//{
			//	// Нужно избегать этого. Значит выше по стеку в этой нити
			//	// более одного раза был выполнен non exclusive lock
			//	_ASSERTEX(nLeft == 0);
			//}
			#ifdef _DEBUG
			int nInThreadLeft = 0;
			for (int i=1; i<countof(mn_LockedTID); i++)
			{
				if (mn_LockedTID[i] == dwTID)
				{
					_ASSERTEX(mn_LockedCount[i]>=0);
					nInThreadLeft = mn_LockedCount[i];
					break;
				}
			}
			if (nInThreadLeft > 0)
			{
				// Нужно избегать этого. Значит выше по стеку в этой нити
				// более одного раза был выполнен non exclusive lock
				_ASSERTEX(nInThreadLeft == 0);
			}
			#endif


			DEBUGSTR(L"!!! Failed non exclusive lock, trying to use CriticalSection\n");
			bool lbEntered = MyEnterCriticalSection(anTimeout); // дождаться пока секцию отпустят
			// mb_Exclusive может быть выставлен, если сейчас другая нить пытается выполнить exclusive lock
			_ASSERTEX(!mb_Exclusive); // После LeaveCriticalSection mb_Exclusive УЖЕ должен быть сброшен
			AddRef(dwTID); // накрутить счетчик

			// Но поскольку нам нужен только nonexclusive lock
			if (lbEntered)
				m_cs.Leave();
		}
	}
	else // abExclusive
	{
		// Требуется Exclusive Lock
		#ifdef _DEBUG
		if (mb_Exclusive)
		{
			// Этого надо стараться избегать
			DEBUGSTR(L"!!! Exclusive lock found in other thread\n");
		}
        #endif

		// Если есть ExclusiveLock (в другой нити) - дождется сама EnterCriticalSection
		#ifdef _DEBUG
		BOOL lbPrev = mb_Exclusive;
		DWORD nPrevTID = mn_TID;
		#endif

		// Сразу установим mb_Exclusive, чтобы в других нитях случайно не прошел nonexclusive lock
		// иначе может получиться, что nonexclusive lock мешает выполнить exclusive lock (ждут друг друга)
		mb_Exclusive = TRUE;
		TODO("Need to check, if MyEnterCriticalSection failed on timeout!");

		if (!MyEnterCriticalSection(anTimeout))
		{
			// Пока поставил _ASSERTE, чтобы посмотреть, возникают ли Timeout-ы при блокировке
			_ASSERTEX(FALSE);

			if (mn_TID == 0)  // поскольку заблокировать не удалось - сбросим флажок
				mb_Exclusive = FALSE;

			return FALSE;
		}

		// 120710 - добавил "|| (mn_TID==dwTID)". Это в том случае, если предыдущая ExclusiveThread была прибита.
		_ASSERTEX(!(lbPrev && mb_Exclusive) || (mn_TID==dwTID)); // После LeaveCriticalSection mb_Exclusive УЖЕ должен быть сброшен
		mn_TID = dwTID; // И запомним, в какой нити это произошло

		HANDLE h = mh_ExclusiveThread;
		mh_ExclusiveThread = OpenThread(SYNCHRONIZE, FALSE, dwTID);
		SafeCloseHandle(h);

		mb_Exclusive = TRUE; // Флаг могла сбросить другая нить, выполнившая Leave
		_ASSERTEX(mn_LockedTID[0] == 0 && mn_LockedCount[0] == 0);
		mn_LockedTID[0] = dwTID;
		mn_LockedCount[0] ++; // на [0] mn_Locked не распространяется

		/*if (abRelockExclusive) {
			ReleaseRef(dwTID); // Если до этого был nonexclusive lock
		}*/

		// B если есть nonexclusive locks - дождаться их завершения
		if (mn_Locked)
		{
			//WARNING: Тут есть шанс наколоться, если сначала был NonExclusive, а потом в этой же нити - Exclusive
			// В таких случаях нужно вызывать с параметром abRelockExclusive
			WaitUnlocked(dwTID, anTimeout);
		}
	}

	return TRUE;
}

void MSection::Unlock(BOOL abExclusive)
{
	DWORD dwTID = GetCurrentThreadId();

	if (abExclusive)
	{
		//Process_Lock();

		_ASSERTEX(mn_TID == dwTID && mb_Exclusive);
		_ASSERTEX(mn_LockedTID[0] == dwTID);
		#ifdef _DEBUG
		mn_UnlockedExclusiveTID = dwTID;
		#endif
		mb_Exclusive = FALSE; mn_TID = 0;
		mn_LockedTID[0] = 0;
		if (mn_LockedCount[0] > 0)
		{
			mn_LockedCount[0] --; // на [0] mn_Locked не распространяется
		}
		else
		{
			_ASSERTEX(mn_LockedCount[0]>0);
		}
		m_cs.Leave();

		//Process_Unlock();
	}
	else
	{
		ReleaseRef(dwTID);
	}
}

bool MSection::isLockedExclusive()
{
	if (!this || !mb_Exclusive)
		return false;
	DWORD nTID = GetCurrentThreadId();
	if (mn_LockedTID[0] != nTID)
		return false;
	_ASSERTEX(mn_LockedCount[0] > 0);
	return (mn_LockedCount[0] > 0);
}

MSectionThread::MSectionThread(MSection* apS)
{
	mp_S = apS; mn_TID = GetCurrentThreadId();
}

MSectionThread::~MSectionThread()
{
	if (mp_S && mn_TID)
		mp_S->ThreadTerminated(mn_TID);
}



MSectionLock::MSectionLock()
{
	mp_S = NULL; mb_Locked = FALSE; mb_Exclusive = FALSE;
}

MSectionLock::~MSectionLock()
{
	_ASSERTEX((mb_Locked==FALSE || mb_Locked==TRUE) && (mb_Exclusive==FALSE || mb_Exclusive==TRUE));
	if (mb_Locked)
	{
		DWORD nCurTID = GetCurrentThreadId();

		Unlock();

		#ifdef _DEBUG
		wchar_t szDbg[80];
		msprintf(szDbg, countof(szDbg), L"::~MSectionLock, TID=%u\n", nCurTID);
		DebugString(szDbg);
		#endif
		UNREFERENCED_PARAMETER(nCurTID);
	}
}

BOOL MSectionLock::Lock(MSection* apS, BOOL abExclusive/*=FALSE*/, DWORD anTimeout/*=-1*/)
{
	if (!apS)
		return FALSE;
	if (mb_Locked && apS == mp_S && (abExclusive == mb_Exclusive || mb_Exclusive))
		return TRUE; // уже заблокирован //111126 - возвращался FALSE

	_ASSERTEX(!mb_Locked);
	mb_Exclusive = (abExclusive!=FALSE);
	mp_S = apS;
	mb_Locked = mp_S->Lock(mb_Exclusive, anTimeout);
	return mb_Locked;
}

BOOL MSectionLock::RelockExclusive(DWORD anTimeout/*=-1*/)
{
	if (!mp_S)
	{
		_ASSERTEX(mp_S!=NULL);
		return FALSE;
	}
	if (mb_Locked && mb_Exclusive)
		return TRUE;  // уже

	// Чистый ReLock делать нельзя. Виснут другие нити, которые тоже запросили ReLock
	Unlock();
	mb_Exclusive = TRUE;
	mb_Locked = mp_S->Lock(mb_Exclusive, anTimeout);
	return mb_Locked;
}

void MSectionLock::Unlock()
{
	if (mp_S && mb_Locked)
	{
		#ifdef _DEBUG
		DWORD nCurTID = GetCurrentThreadId();
		wchar_t szDbg[80];
		if (mb_Exclusive)
			msprintf(szDbg, countof(szDbg), L"::Unlock(), TID=%u\n", nCurTID);
		else
			szDbg[0] = 0;
		#endif

		_ASSERTEX(mb_Exclusive || mp_S->mn_Locked>0);
		mp_S->Unlock(mb_Exclusive);
		mb_Locked = FALSE;

		#ifdef _DEBUG
		if (*szDbg)
		{
			DebugString(szDbg);
		}
		#endif
	}
}

BOOL MSectionLock::isLocked(BOOL abExclusiveOnly/*=FALSE*/)
{
	return (mp_S!=NULL) && mb_Locked && (!abExclusiveOnly || mb_Exclusive);
};
