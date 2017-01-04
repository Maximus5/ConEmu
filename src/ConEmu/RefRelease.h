
/*
Copyright (c) 2013-2017 Maximus5
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

#ifdef _DEBUG
#include "../common/MMap.h"
#include "../common/MSectionSimple.h"
#endif

class CRefRelease
{
private:
	LONG volatile mn_RefCount;
	static const LONG REF_FINALIZE = 0x7FFFFFFF;
private:
	#ifdef _DEBUG
	static MSectionSimple mcs_Locks;
	MMap<DWORD,LONG> m_Locks;
	#endif

protected:
	virtual void FinalRelease() = 0;

public:
	CRefRelease()
	{
		mn_RefCount = 1;
		#ifdef _DEBUG
		if (!mcs_Locks.IsInitialized())
			mcs_Locks.Init();
		m_Locks.Init(32,true);
		m_Locks.Set(GetCurrentThreadId(), 1);
		#endif
	};

	void AddRef()
	{
		if (!this)
		{
			_ASSERTE(this!=NULL);
			return;
		}

		_ASSERTE(mn_RefCount != REF_FINALIZE);
		InterlockedIncrement(&mn_RefCount);

		#ifdef _DEBUG
		DWORD nTID = GetCurrentThreadId(); LONG nLocks = 0;
		MSectionLockSimple CS; CS.Lock(&mcs_Locks);
		if (!m_Locks.Get(nTID, &nLocks))
			nLocks = 1;
		else
			nLocks++;
		m_Locks.Set(nTID, nLocks);
		CS.Unlock();
		#endif
	};

	int Release()
	{
		if (!this)
			return 0;

		if (mn_RefCount == REF_FINALIZE)
		{
			_ASSERTE(FALSE && "Must not be destroyed yet?");
			return 0;
		}
		InterlockedDecrement(&mn_RefCount);

		#ifdef _DEBUG
		DWORD nTID = GetCurrentThreadId(); LONG nLocks = 0;
		MSectionLockSimple CS; CS.Lock(&mcs_Locks);
		if (m_Locks.Get(nTID, &nLocks))
		{
			nLocks--;
			if (nLocks > 0)
				m_Locks.Set(nTID, nLocks);
			else
				m_Locks.Del(nTID);
		}
		CS.Unlock();
		#endif

		_ASSERTE(mn_RefCount>=0);
		if (mn_RefCount <= 0)
		{
			mn_RefCount = REF_FINALIZE; // принудительно, чтобы не было повторных срабатываний delete при вызове деструкторов
			FinalRelease();
			//CVirtualConsole* pVCon = (CVirtualConsole*)this;
			//delete pVCon;
			return 0;
		}

		return mn_RefCount;
	};

#ifdef _DEBUG
	int RefCount()
	{
		return mn_RefCount;
	}
#endif
protected:
	virtual ~CRefRelease()
	{
		_ASSERTE(mn_RefCount==REF_FINALIZE);
		#ifdef _DEBUG
		m_Locks.Release();
		#endif
	};
};

template <class T>
class CRefGuard
{
protected:
	T *mp_Ref;
	DWORD mn_Tick;

protected:
	#if defined(HAS_CPP11)
	// Use strict `Attach` instead of `=` to avoid
	// unexpected differences in compiler implementations
	CRefGuard<T>& operator=(CRefGuard<T>&& guard)
	{
		this->Attach(guard.Ptr());
		return this;
	};
	#endif

public:
	CRefGuard()
	{
		mp_Ref = NULL;
		mn_Tick = GetTickCount();
	};

	CRefGuard(T* apRef)
	{
		mp_Ref = NULL;
		mn_Tick = GetTickCount();

		Attach(apRef);
	};

	virtual ~CRefGuard()
	{
		Release();
	};

	virtual void Release()
	{
		if (mp_Ref)
		{
			mp_Ref->Release();
			mp_Ref = NULL;
		}
	};

	virtual bool Attach(T* apRef)
	{
		if (mp_Ref != apRef)
		{
			if (apRef)
				apRef->AddRef();

			klSwap(mp_Ref, apRef);

			if (apRef)
				apRef->Release();
		}

		return (mp_Ref != NULL);
	};


public:
	// Dereference
	T* operator->() const
	{
		_ASSERTE(mp_Ref!=NULL);
		return mp_Ref;
	};

	// Releases any current VCon and loads specified
	CRefGuard& operator=(T* apRef)
	{
		Attach(apRef);
		return *this;
	};

	// Ptr, No Asserts
	T* Ptr()
	{
		return mp_Ref;
	};
};
