
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


#pragma once

#include <windows.h>

#if defined(_DEBUG)
enum MEventNotification
{
	evn_Create,
	evn_Open,
	evn_Set,
	evn_Reset,
	evn_Close,
};
#endif

class MEvent
{
protected:
	WCHAR   ms_EventName[MAX_PATH];
	HANDLE  mh_Event;
	DWORD   mn_LastError;
	bool    mb_NameIsNull;
	#if defined(_DEBUG)
	bool    mb_DebugNotify;
	void    OnDebugNotify(MEventNotification Action);
	#endif
public:
	MEvent();
	~MEvent();
public:
	void    InitName(const wchar_t *aszTemplate, DWORD Parm1=0);
public:
	void    Close();
	HANDLE  Create(LPSECURITY_ATTRIBUTES pSec, bool bManualReset, bool bInitialState, LPCWSTR lpName);
	HANDLE  Open(bool bCreate = false, LPSECURITY_ATTRIBUTES pSec = NULL, bool bManualReset = false, bool bInitialState = false);
	bool    Set();
	bool    Reset();
	DWORD   Wait(DWORD dwMilliseconds, BOOL abAutoOpen=TRUE);
	HANDLE  GetHandle();
	LPCWSTR GetName();
};
