
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


#pragma once

#include "defines.h"

#if defined(_DEBUG)
enum class MEventNotification
{
	Create,
	Open,
	Set,
	Reset,
	Close,
};
#endif

class MEvent final
{
protected:
	WCHAR   eventName_[MAX_PATH] = L"";
	HANDLE  handle_ = nullptr;
	DWORD   lastError_ = 0;
	bool    nameIsNull_ = false;
	#if defined(_DEBUG)
	bool    debugNotify_ = false;
	void    OnDebugNotify(MEventNotification action) const;
	#endif
public:
	MEvent();
	~MEvent();
	MEvent(const MEvent&) = delete;
	MEvent& operator=(const MEvent&) = delete;
	MEvent(MEvent&&) = default;
	MEvent& operator=(MEvent&&) = default;
public:
	void    InitName(const wchar_t* aszTemplate, DWORD parm1 = 0);
public:
	void    Close();
	HANDLE  Create(SECURITY_ATTRIBUTES* pSec, bool bManualReset, bool bInitialState, const wchar_t* lpName);
	HANDLE  Open(bool bCreate = false, SECURITY_ATTRIBUTES* pSec = nullptr, bool bManualReset = false, bool bInitialState = false);
	bool    Set();
	bool    Reset();
	DWORD   Wait(DWORD dwMilliseconds, bool abAutoOpen = true);
	HANDLE  GetHandle() const;
	LPCWSTR GetName() const;
};
