
/*
Copyright (c) 2015-2017 Maximus5
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

// !!! First DWORD/LONG of VAL_T **must** be a hash of value !!!
// !!! VAL_C must be power of 2 !!!
template<typename VAL_T, LONG VAL_C>
struct MCircular
{
	// Max elements count [Informational]
	LONG  nCount;
	// InterlockedIncrement'ed index
	LONG  nIndex;
	// Zero values are not accepted
	VAL_T Values[VAL_C];
	// Helpers
	void Init()
	{
		_ASSERTE(sizeof(VAL_T) >= sizeof(LONG));
		if (!nCount)
			nCount = VAL_C;
	};
	void AddValue(const VAL_T& val)
	{
		// Init nCount [Informational]
		Init();
		// Get next circular index
		LONG lIdx = (InterlockedIncrement(&nIndex) - 1) & (VAL_C - 1);
		// ‘Old’ values are expected to be overwritten
		Values[lIdx] = val;
	};
	bool HasValue(const VAL_T& val)
	{
		LONG hash = *((LONG*)&val);
		for (LONG i = 0; i < VAL_C; i++)
		{
			if (*(LONG*)(Values + i) == hash)
				return true;
		}
		return false;
	};
	void DelValue(const VAL_T& val)
	{
		LONG hash = *((LONG*)&val);
		for (LONG i = 0; i < VAL_C; i++)
		{
			if (InterlockedCompareExchange((LONG*)(Values + i), 0, hash) == hash)
				break;
		}
	};
};
