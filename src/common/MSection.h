
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


#pragma once

#include <windows.h>
#include "MSectionSimple.h"

class MSectionLock;

class MSection
{
	protected:
		MSectionSimple m_cs;
		MSectionSimple m_lock_cs;
		DWORD mn_TID; // устанавливается только после EnterCriticalSection
		HANDLE mh_ExclusiveThread;
#ifdef _DEBUG
		DWORD mn_UnlockedExclusiveTID;
#endif
		int mn_Locked;
		BOOL mb_Exclusive;
		HANDLE mh_ReleaseEvent;
		friend class MSectionLock;
		DWORD mn_LockedTID[10];
		int   mn_LockedCount[10];
		void Process_Lock();
		void Process_Unlock();
	public:
		MSection();
		~MSection();
	public:
		void ThreadTerminated(DWORD dwTID);
		bool isLockedExclusive();
	protected:
		void AddRef(DWORD dwTID);
		int ReleaseRef(DWORD dwTID);
		void WaitUnlocked(DWORD dwTID, DWORD anTimeout);
		bool MyEnterCriticalSection(DWORD anTimeout);
		BOOL Lock(BOOL abExclusive, DWORD anTimeout=-1/*, BOOL abRelockExclusive=FALSE*/);
		void Unlock(BOOL abExclusive);
};

class MSectionThread
{
	protected:
		MSection* mp_S;
		DWORD mn_TID;
	public:
		MSectionThread(MSection* apS);
		~MSectionThread();
};

class MSectionLock
{
	protected:
		MSection* mp_S;
		BOOL mb_Locked, mb_Exclusive;
	public:
		BOOL Lock(MSection* apS, BOOL abExclusive=FALSE, DWORD anTimeout=-1);
		BOOL RelockExclusive(DWORD anTimeout=-1);
		void Unlock();
		BOOL isLocked(BOOL abExclusiveOnly=FALSE);
	public:
		MSectionLock();
		~MSectionLock();
};
