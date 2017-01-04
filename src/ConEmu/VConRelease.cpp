
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

#define HIDE_USE_EXCEPTION_INFO
#include "Header.h"
#include "ConEmu.h"
#include "VConGroup.h"
#include "VConRelease.h"
#include "VirtualConsole.h"

//#define REF_FINALIZE 0x7FFFFFFF

CVConRelease::CVConRelease(CVirtualConsole* pOwner)
{
	mp_VCon = pOwner;
}

void CVConRelease::FinalRelease()
{
	if (!isMainThread() && ghWnd)
	{
		CVirtualConsole* pVCon = mp_VCon;
		gpConEmu->DeleteVConMainThread(pVCon);
		return;
	}
	DeleteFromMainThread();
};

void CVConRelease::DeleteFromMainThread()
{
	CVirtualConsole* pVCon = mp_VCon;
	delete pVCon;
}

//CVConRelease::CVConRelease()
//{
//	mn_RefCount = 1;
//
//	#ifdef _DEBUG
//	CVirtualConsole* pVCon = (CVirtualConsole*)this;
//	_ASSERTE((void*)pVCon == (void*)this);
//	#endif
//}
//
//CVConRelease::~CVConRelease()
//{
//	_ASSERTE(mn_RefCount==REF_FINALIZE);
//}
//
//void CVConRelease::AddRef()
//{
//	if (!this)
//	{
//		_ASSERTE(this!=NULL);
//		return;
//	}
//
//	InterlockedIncrement(&mn_RefCount);
//}
//
//int CVConRelease::Release()
//{
//	if (!this)
//		return 0;
//
//	InterlockedDecrement(&mn_RefCount);
//
//	_ASSERTE(mn_RefCount>=0);
//	if (mn_RefCount <= 0)
//	{
//		mn_RefCount = REF_FINALIZE; // принудительно, чтобы не было повторных срабатываний delete при вызове деструкторов
//		CVirtualConsole* pVCon = (CVirtualConsole*)this;
//		delete pVCon;
//		return 0;
//	}
//
//	return mn_RefCount;
//}


CVConGuard::CVConGuard()
	: CRefGuard<CVirtualConsole>()
	, mi_Valid(0)
{
}

CVConGuard::CVConGuard(CVirtualConsole* apRef)
	: CRefGuard<CVirtualConsole>()
	, mi_Valid(0)
{
	Attach(apRef);
}

bool CVConGuard::Attach(CVirtualConsole* apRef)
{
	if (!CVConGroup::setRef(mp_Ref, apRef))
	{
		_ASSERTE(mp_Ref == NULL);
	}

	mi_Valid = mp_Ref ? CVConGroup::isValid(mp_Ref) ? 1 : -1 : 0;
	Assert(mi_Valid >= 0);

	return (mp_Ref != NULL);
}

CVConGuard::~CVConGuard()
{
	// inherited virtual destructor
}
