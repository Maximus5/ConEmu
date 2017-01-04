
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
#define SHOWDEBUGSTR

#include "Header.h"
#include "LngData.h"
#include "MenuIds.h"
#include "resource.h"

#include <stdlib.h>

struct LngPredefined
{
	UINT_PTR id;
	LPCWSTR  str;

	static int compare(const void* p1, const void* p2)
	{
		int iDiff = (int)((INT_PTR)((LngPredefined*)p1)->id - (INT_PTR)((LngPredefined*)p2)->id);
		return iDiff;
	};
};

// LngPredefined gsDataHints
#include "LngDataHints.h"
// LngPredefined gsDataRsrcs
#include "LngDataRsrcs.h"


// sort gsDataHints (except last item) to further binary search for id
void CLngPredefined::Initialize()
{
	qsort(gsDataHints, countof(gsDataHints)-1, sizeof(gsDataHints[0]), LngPredefined::compare);
	qsort(gsDataRsrcs, countof(gsDataRsrcs)-1, sizeof(gsDataRsrcs[0]), LngPredefined::compare);
}

// Search in gsDataHints for id
LPCWSTR CLngPredefined::getHint(UINT id)
{
	LngPredefined key = {id};

	// Last item of gsDataHints must be empty
	_ASSERTE(gsDataHints[0].id && !gsDataHints[countof(gsDataHints)-1].id);

	LngPredefined* p = (LngPredefined*)bsearch(&key, gsDataHints, countof(gsDataHints)-1, sizeof(gsDataHints[0]), LngPredefined::compare);

	if (p)
		return p->str;

	return NULL;
}

// Search in gsDataRsrcs for id
LPCWSTR CLngPredefined::getRsrc(UINT id)
{
	LngPredefined key = {id};

	// Last item of gsDataRsrcs must be empty
	_ASSERTE(gsDataRsrcs[0].id && !gsDataRsrcs[countof(gsDataRsrcs)-1].id);

	LngPredefined* p = (LngPredefined*)bsearch(&key, gsDataRsrcs, countof(gsDataRsrcs)-1, sizeof(gsDataRsrcs[0]), LngPredefined::compare);

	if (p)
		return p->str;

	return NULL;
}
