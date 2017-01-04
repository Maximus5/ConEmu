
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

#define HIDE_USE_EXCEPTION_INFO
#include "defines.h"
#include "MGlobal.h"

/* GlobalAlloc/GlobalLock/GlobalUnlock/GlobalFree encapsulation */

MGlobal::MGlobal()
	: mh_Global(NULL)
	, mp_Locked(NULL)
{
}

MGlobal::MGlobal(UINT nFlags, size_t cbSize)
	: mh_Global(NULL)
	, mp_Locked(NULL)
{
	Alloc(nFlags, cbSize);
}

MGlobal::~MGlobal()
{
	Free();
}

HGLOBAL MGlobal::Alloc(UINT nFlags, size_t cbSize)
{
	Free(); // JIC

	InterlockedExchangePointer((PVOID*)&mh_Global, GlobalAlloc(nFlags, cbSize));
	return mh_Global;
}

void MGlobal::Free()
{
	if (!mh_Global)
		return;

	Unlock();

	HGLOBAL p = (HGLOBAL)InterlockedExchangePointer((PVOID*)&mh_Global, NULL);
	if (p)
		GlobalFree(p);
}

LPVOID MGlobal::Lock()
{
	if (!mh_Global)
		return NULL;
	if (mp_Locked)
		return mp_Locked;

	InterlockedExchangePointer((PVOID*)&mp_Locked, GlobalLock(mh_Global));
	return mp_Locked;
}

void MGlobal::Unlock()
{
	if (!mp_Locked)
		return;

	LPVOID p = InterlockedExchangePointer((PVOID*)&mp_Locked, NULL);
	if (p)
		GlobalUnlock(p);
}

MGlobal::operator HGLOBAL()
{
	Unlock();
	return mh_Global;
}

HGLOBAL MGlobal::Detach()
{
	Unlock();
	HGLOBAL p = (HGLOBAL)InterlockedExchangePointer((PVOID*)&mh_Global, NULL);
	return p;
}
