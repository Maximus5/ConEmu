
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

#include "Font.h"
#include "FontPtr.h"

bool operator== (const CFontPtr &a, const CFontPtr &b)
{
	return a.Equal(b.CPtr());
}

bool operator!= (const CFontPtr &a, const CFontPtr &b)
{
	return !(a.Equal(b.CPtr()));
}


CFontPtr::CFontPtr(CFont* apRef /*= NULL*/)
	: mp_Ref(NULL)
{
	Attach(apRef);
}

CFontPtr::~CFontPtr()
{
	Release();
}

int CFontPtr::Release()
{
	CFont* p = NULL;
	klSwap(p, mp_Ref);
	int iRef = p ? p->Release() : 0;
	return iRef;
}

bool CFontPtr::Attach(CFont* apRef)
{
	if (apRef != mp_Ref)
	{
		klSwap(mp_Ref, apRef);
		if (mp_Ref)
			mp_Ref->AddRef();
		SafeRelease(apRef);
	}

	return (mp_Ref != NULL);
}

// Dereference
CFont* CFontPtr::operator->() const
{
	_ASSERTE(mp_Ref!=NULL);
	return mp_Ref;
}

// Releases any current VCon and loads specified
CFontPtr& CFontPtr::operator=(CFont* apRef)
{
	Attach(apRef);
	return *this;
}

CFontPtr& CFontPtr::operator=(const CFontPtr& aPtr)
{
	Attach(aPtr.Ptr());
	return *this;
}

// Ptr, No Asserts
CFontPtr::operator CFont*() const
{
	return Ptr();
}
CFont* CFontPtr::Ptr() const
{
	return mp_Ref;
}
const CFont* CFontPtr::CPtr() const
{
	return mp_Ref;
}

// Validation
bool CFontPtr::IsSet() const
{
	return (mp_Ref && mp_Ref->IsSet());
}

bool CFontPtr::Equal(const CFont* pFont) const
{
	if (mp_Ref == pFont)
		return true;
	if (mp_Ref)
		return mp_Ref->Equal(pFont);
	return false;
}
