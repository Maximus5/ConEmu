
/*
Copyright (c) 2009-present Maximus5
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

#include "defines.h"
#include "MAssert.h"
#include "MSetter.h"

MSetter::MSetter(std::atomic_int& st)
{
	DEBUGTEST(ZeroStruct(DataPtr));
	type = st_aint;
	mp_aint = &st;
	mp_aint->fetch_add(1);
}

MSetter::MSetter(LONG* st)
{
	DEBUGTEST(ZeroStruct(DataPtr));
	type = st_LONG;
	mp_longVal = st;
	if (mp_longVal)
	{
		InterlockedIncrement(mp_longVal);
	}
}
MSetter::MSetter(bool* st)
{
	DEBUGTEST(ZeroStruct(DataPtr));
	type = st_bool;
	mp_boolVal = st;
	if (mp_boolVal)
	{
		mb_OldBoolValue = *mp_boolVal;
		*mp_boolVal = true;
	}
}
MSetter::MSetter(DWORD* st, DWORD setValue)
{
	DEBUGTEST(ZeroStruct(DataPtr));
	type = st_DWORD;
	mpdw_DwordVal = st;
	if (mpdw_DwordVal)
	{
		mdw_OldDwordValue = *mpdw_DwordVal;
		*mpdw_DwordVal = setValue;
	}
}
MSetter::~MSetter()
{
	Unlock();
}
void MSetter::Unlock()
{
	switch (type)
	{
	case st_aint:
		if (mp_aint)
			mp_aint->fetch_sub(1);
		mp_aint = nullptr;
		break;
	case st_LONG:
		if (mp_longVal)
			InterlockedDecrement(mp_longVal);
		mp_longVal = nullptr;
		break;
	case st_bool:
		if (mp_boolVal)
			*mp_boolVal = mb_OldBoolValue;
		mp_boolVal = nullptr;
		break;
	case st_DWORD:
		if (mpdw_DwordVal)
			*mpdw_DwordVal = mdw_OldDwordValue;
		mpdw_DwordVal = nullptr;
		break;
	}
}
