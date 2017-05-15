
/*
Copyright (c) 2016-2017 Maximus5
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

#define HIDE_USE_EXCEPTION_INFO
#include <windows.h>
#include <intrin.h>
#include "defines.h"
#include "MPerfCounter.h"

MPerfCounter::MPerfCounter(UINT Counters /*= 1*/)
{
	if (!QueryPerformanceFrequency(&m_Frequency) || (m_Frequency.QuadPart < 0))
		m_Frequency.QuadPart = 0;
	m_Total.QuadPart = 0;
	m_Counters.alloc((INT_PTR)Counters);
	PerfCounterData dummy = {};
	for (UINT i = 0; i < Counters; i++)
	{
		m_Counters.push_back(dummy);
	}
}

MPerfCounter::~MPerfCounter()
{
}

void MPerfCounter::Start(PerfCounter& counter)
{
	if (counter.ID >= (UINT)m_Counters.size())
	{
		return;
	}
	PerfCounterData* p = &(m_Counters[counter.ID]);

	_ASSERTE(!counter.Initialized);

	if ((counter.Initialized = QueryPerformanceCounter(&counter.Start)))
	{
		p->Initialized = TRUE;
		p->Start = counter.Start;
	}
}

#if defined(_WIN64)
#define iAdd64(Addend,Value) InterlockedAdd64(Addend, Value)
#else
// Reads and writes to 64-bit values are not guaranteed to be atomic on 32-bit Windows. 
inline __int64 iAdd64(__int64 volatile * Addend, __int64 Value)
{
	*Addend += Value;
	return *Addend;
};
#endif

void MPerfCounter::Stop(PerfCounter& counter)
{
	if (counter.ID >= (UINT)m_Counters.size())
	{
		_ASSERTE(counter.ID < (UINT)m_Counters.size());
		return;
	}
	if (!counter.Initialized)
	{
		_ASSERTE(counter.Initialized);
		return;
	}
	PerfCounterData* p = &(m_Counters[counter.ID]);

	LARGE_INTEGER tick;
	if (QueryPerformanceCounter(&tick))
	{
		LONGLONG duration = tick.QuadPart - counter.Start.QuadPart;
		p->Stop.QuadPart = tick.QuadPart;
		InterlockedIncrement(&p->Count);
		iAdd64(&p->Duration.QuadPart, duration);
		iAdd64(&m_Total.QuadPart, duration);
	}
	counter.Initialized = FALSE;
}

ULONG MPerfCounter::GetCounter(UINT ID, ULONG* pPercentage, ULONG* pMilliSeconds, LARGE_INTEGER* pDuration)
{
	ULONG nCount = 0;
	LONGLONG duration = 0;
	if (ID < (UINT)m_Counters.size())
	{
		PerfCounterData* p = &(m_Counters[ID]);
		duration = p->Duration.QuadPart;
		nCount = (ULONG)p->Count;
	}
	_ASSERTE(m_Frequency.QuadPart > 0);

	if (pPercentage)
	{
		if (m_Total.QuadPart > 0)
			*pPercentage = (ULONG)(duration * 100LL / m_Total.QuadPart); // Evaluate as LONGLONG and only the result cast to ULONG
		else
			*pPercentage = 0;
	}

	if (pMilliSeconds)
	{
		if (m_Frequency.QuadPart > 0)
			*pMilliSeconds = (ULONG)(duration * 1000LL / m_Frequency.QuadPart); // Evaluate as LONGLONG and only the result cast to ULONG
		else
			*pMilliSeconds = 0;
	}

	if (pDuration)
	{
		pDuration->QuadPart = duration;
	}

	return nCount;
}
