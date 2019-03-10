
/*
Copyright (c) 2016-present Maximus5
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

#include "defines.h"
#include "MArray.h"

struct PerfCounter
{
	// ID must be filled before calling `Start`
	UINT ID;
	// For internal use
	BOOL Initialized;
	// Caller must not change this value
	LARGE_INTEGER Start;
};

struct PerfCounterStats
{
	// number of times Start was called
	ULONG Counter;
	// 0..100% of all counters time
	ULONG Percentage;
	// human-readable duration in ms
	ULONG MilliSeconds;
	// duration from QueryPerformanceCounter
	LARGE_INTEGER Duration;
};

class MPerfCounter
{
protected:
	// QueryPerformanceFrequency()
	LARGE_INTEGER m_Frequency = {};
	// Total captured time for all counters
	LARGE_INTEGER m_Total = {};

	// Used for arg-less Start() and Stop()
	PerfCounter self_counter_ = {};

protected:
	// Data storage
	struct PerfCounterData
	{
		BOOL Initialized;
		LONG Count;
		LARGE_INTEGER Start;
		LARGE_INTEGER Stop;
		LARGE_INTEGER Duration;
	};

	// The object support counting several counters
	MArray<PerfCounterData> m_Counters;

public:
	MPerfCounter(const UINT Counters = 1);
	~MPerfCounter();

	// ID = 0..(Counters-1)
	void Start(PerfCounter& counter);
	void Stop(PerfCounter& counter);

	// ID = 0
	void Start();
	ULONG Stop();

	// returns count of calls
	ULONG GetCounter(const UINT ID, ULONG* pPercentage, ULONG* pMilliSeconds, LARGE_INTEGER* pDuration) const;
	// return full statistics about ID
	PerfCounterStats GetStats(const UINT ID) const;
};
