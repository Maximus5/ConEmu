
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
#include "MSectionSimple.h"

MSectionSimple::MSectionSimple(bool bInit /*= false*/)
{
	mb_Initialized = false;
	mn_LastError = 0;

	if (bInit)
		Init();
	else
		ZeroStruct(m_S);
}

MSectionSimple::~MSectionSimple()
{
	Close();
}

bool MSectionSimple::IsInitialized()
{
	return (this && mb_Initialized);
}

void MSectionSimple::Init()
{
	if (!mb_Initialized)
	{
		InitializeCriticalSection(&m_S);
		mb_Initialized = true;
	}
}

void MSectionSimple::Close()
{
	if (mb_Initialized)
	{
		DeleteCriticalSection(&m_S);
		mb_Initialized = false;
	}
}

void MSectionSimple::Enter()
{
	_ASSERTEX(mb_Initialized);
	EnterCriticalSection(&m_S);
}

void MSectionSimple::Leave()
{
	_ASSERTEX(mb_Initialized);
	LeaveCriticalSection(&m_S);
}

bool MSectionSimple::TryEnter()
{
	_ASSERTEX(mb_Initialized);
	BOOL brc = TryEnterCriticalSection(&m_S);
	if (!brc)
	{
		mn_LastError = GetLastError();
		return false;
	}
	return true;
}

bool MSectionSimple::RecreateAndLock()
{
	BOOL lbLocked;
	CRITICAL_SECTION csNew;
	InitializeCriticalSection(&csNew);

	CRITICAL_SECTION csOld = m_S;
	DeleteCriticalSection(&csOld);

	lbLocked = TryEnterCriticalSection(&csNew);
	m_S = csNew;

	return (lbLocked != FALSE);
}


MSectionLockSimple::MSectionLockSimple()
{
	mp_S = NULL;
	mb_Locked = false;
	#ifdef _DEBUG
	mn_LockTID = mn_LockTick = 0;
	#endif
}

MSectionLockSimple::~MSectionLockSimple()
{
	if (mb_Locked)
	{
		Unlock();
	}
}

BOOL MSectionLockSimple::Lock(MSectionSimple* apS, DWORD anTimeout/*=-1*/)
{
	if (mb_Locked && (mp_S != apS))
		Unlock();

	mp_S = apS;

	if (!mp_S)
	{
		_ASSERTEX(apS);
		return FALSE;
	}

	#ifdef _DEBUG
	if (mb_Locked)
	{
		_ASSERTEX(!mb_Locked);
	}
	_ASSERTEX(apS->IsInitialized());
	#endif

	bool bLocked = false;
	DWORD nStartTick = GetTickCount();
	DWORD nDelta = -1;
#if 0
	EnterCriticalS	ection(&apS->m_S);

	bLocked = mb_Locked = true;

	nDelta = GetTickCount() - nStartTick;
	if (nDelta >= anTimeout)
	{
		_ASSERTEX(FALSE && "Failed to lock CriticalSection, timeout");
	}
#else
	while (true)
	{
		if (apS->TryEnter())
		{
			bLocked = mb_Locked = true;
			#ifdef _DEBUG
			mn_LockTID = GetCurrentThreadId();
			mn_LockTick = GetTickCount();
			#endif
			break;
		}

		nDelta = GetTickCount() - nStartTick;

		if (anTimeout != (DWORD)-1)
		{
			if (nDelta >= anTimeout)
			{
				_ASSERTEX(IsDebuggerPresent() && "Failed to lock CriticalSection, timeout");
				break;
			}
		}

		Sleep(1);
	}
#endif

	return bLocked;
}

void MSectionLockSimple::Unlock()
{
	if (mb_Locked)
	{
		if (mp_S)
			mp_S->Leave();
		mb_Locked = false;
	}
}

BOOL MSectionLockSimple::isLocked()
{
	return mb_Locked;
}
