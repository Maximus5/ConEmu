
/*
Copyright (c) 2009-2012 Maximus5
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
#include "Header.h"
#include "ConEmu.h"
#include "VConRelease.h"
#include "VirtualConsole.h"

#define REF_FINALIZE 0x7FFFFFFF

CVConRelease::CVConRelease()
{
	mn_RefCount = 1;
	
	#ifdef _DEBUG
	CVirtualConsole* pVCon = (CVirtualConsole*)this;
	_ASSERTE((void*)pVCon == (void*)this);
	#endif
}

CVConRelease::~CVConRelease()
{
	_ASSERTE(mn_RefCount==REF_FINALIZE);
}

void CVConRelease::AddRef()
{
	if (!this)
	{
		_ASSERTE(this!=NULL);
		return;
	}
	
	InterlockedIncrement(&mn_RefCount);
}

int CVConRelease::Release()
{
	if (!this)
		return 0;

	InterlockedDecrement(&mn_RefCount);
	
	_ASSERTE(mn_RefCount>=0);
	if (mn_RefCount <= 0)
	{
		mn_RefCount = REF_FINALIZE; // принудительно, чтобы не было повторных срабатываний delete при вызове деструкторов
		CVirtualConsole* pVCon = (CVirtualConsole*)this;
		delete pVCon;
		return 0;
	}

	return mn_RefCount;
}


CVConGuard::CVConGuard()
{
	mp_Ref = NULL;
}

CVConGuard::CVConGuard(CVirtualConsole* apRef)
{
	mp_Ref = NULL;

	Attach(apRef);
}

void CVConGuard::Attach(CVirtualConsole* apRef)
{
	if (mp_Ref == apRef)
	{
		//_ASSERTE((apRef == NULL || mp_Ref != apRef) && "Already assigned?");
		return;
	}

	CVirtualConsole *pOldRef = mp_Ref;

	mp_Ref = apRef;

	if (pOldRef != mp_Ref)
	{
		if (mp_Ref)
		{
			mp_Ref->AddRef();
		}

		if (pOldRef)
		{
			pOldRef->Release();
		}
	}
}

void CVConGuard::Release()
{
	if (mp_Ref)
	{
		mp_Ref->Release();
		mp_Ref = NULL;
	}
}

CVConGuard::~CVConGuard()
{
	Release();
}

// Dereference
CVirtualConsole* CVConGuard::operator->() const
{
	_ASSERTE(mp_Ref!=NULL);
	return mp_Ref;
}

// Releases any current VCon and loads specified
CVConGuard& CVConGuard::operator=(CVirtualConsole* apRef)
{
	Attach(apRef);

	return *this;
}

CVirtualConsole* CVConGuard::VCon()
{
	return mp_Ref;
}
