
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

//#ifdef _DEBUG
//#define USE_LOCK_SECTION
//#endif

#define SHOWDEBUGSTR

#define HIDE_USE_EXCEPTION_INFO
#include <windows.h>
#include "defines.h"
#include "MAssert.h"
#include "MEvent.h"

#define DEBUGSTREVT(s) DEBUGSTR(s)

MEvent::MEvent()
{
	ms_EventName[0] = 0;
	mh_Event = NULL;
	mn_LastError = 0;
	mb_NameIsNull = false;
	#if defined(_DEBUG)
	mb_DebugNotify = false;
	#endif
}

MEvent::~MEvent()
{
	if (mh_Event)
		Close();
}

void MEvent::Close()
{
	#if defined(_DEBUG)
	if (mb_DebugNotify && mh_Event)
		OnDebugNotify(evn_Close);
	#endif

	mn_LastError = 0;
	ms_EventName[0] = 0;
	mb_NameIsNull = false;

	if (mh_Event)
	{
		::CloseHandle(mh_Event);
		mh_Event = NULL;
	}
}

void MEvent::InitName(const wchar_t *aszTemplate, DWORD Parm1)
{
	if (mh_Event)
		Close();

	mb_NameIsNull = (aszTemplate == NULL);
	if (!mb_NameIsNull)
		_wsprintf(ms_EventName, SKIPLEN(countof(ms_EventName)) aszTemplate, Parm1);
	mn_LastError = 0;
}

HANDLE MEvent::Create(LPSECURITY_ATTRIBUTES pSec, bool bManualReset, bool bInitialState, LPCWSTR lpName)
{
	if (mh_Event)  // Если уже открыто - сразу вернуть!
		return mh_Event;

	if (lpName)
	{
		_ASSERTE(_tcslen(lpName) < countof(ms_EventName));
		mb_NameIsNull = false;
		lstrcpyn(ms_EventName, lpName, countof(ms_EventName));
	}
	else
	{
		mb_NameIsNull = true;
		ms_EventName[0] = 0;
	}

	return Open(true, pSec, bManualReset, bInitialState);
}

HANDLE MEvent::Open(bool bCreate /*= false*/, LPSECURITY_ATTRIBUTES pSec /*= NULL*/, bool bManualReset /*= false*/, bool bInitialState /*= false*/)
{
	if (mh_Event)  // Если уже открыто - сразу вернуть!
		return mh_Event;

	if ((ms_EventName[0] == 0) && !mb_NameIsNull)
	{
		_ASSERTE(ms_EventName[0]!=0);
		mn_LastError = ERROR_BAD_ARGUMENTS;
		return NULL;
	}

	if (mb_NameIsNull && !bCreate)
	{
		_ASSERTE(bCreate);
		mn_LastError = ERROR_BAD_ARGUMENTS;
		return NULL;
	}

	mn_LastError = 0;

	if (bCreate)
	{
		mh_Event = ::CreateEvent(pSec, bManualReset, FALSE, mb_NameIsNull ? NULL : ms_EventName);
		// Ensure that Event has proper state
		if (mh_Event)
		{
			if (bInitialState)
				::SetEvent(mh_Event);
			else
				::ResetEvent(mh_Event);
		}
	}
	else
	{
		mh_Event = ::OpenEvent(EVENT_MODIFY_STATE|SYNCHRONIZE, FALSE, ms_EventName);
	}

	if (!mh_Event)
		mn_LastError = ::GetLastError();

	#if defined(_DEBUG)
	if (mb_DebugNotify)
		OnDebugNotify(bCreate ? evn_Create : evn_Open);
	#endif

	return mh_Event;
}

bool MEvent::Set()
{
	if (!mh_Event)
	{
		_ASSERTE(mh_Event != NULL);
		mn_LastError = ERROR_BAD_ARGUMENTS;
		return false;
	}

	#if defined(_DEBUG)
	if (mb_DebugNotify)
		OnDebugNotify(evn_Set);
	#endif

	BOOL b = ::SetEvent(mh_Event);

	#if defined(_DEBUG)
	mn_LastError = ::GetLastError();
	#endif

	return (b != FALSE);
}

bool MEvent::Reset()
{
	if (!mh_Event)
	{
		_ASSERTE(mh_Event != NULL);
		mn_LastError = ERROR_BAD_ARGUMENTS;
		return false;
	}

	#if defined(_DEBUG)
	if (mb_DebugNotify)
		OnDebugNotify(evn_Reset);
	#endif

	BOOL b = ::ResetEvent(mh_Event);

	#if defined(_DEBUG)
	mn_LastError = ::GetLastError();
	#endif

	return (b != FALSE);
}

DWORD MEvent::Wait(DWORD dwMilliseconds, BOOL abAutoOpen/*=TRUE*/)
{
	if (!mh_Event && abAutoOpen)
		Open();

	if (!mh_Event)
		return WAIT_ABANDONED;

	DWORD dwWait = ::WaitForSingleObject(mh_Event, dwMilliseconds);

	#if defined(_DEBUG)
	mn_LastError = ::GetLastError();
	#endif

	return dwWait;
}

HANDLE MEvent::GetHandle()
{
	return mh_Event;
}

LPCWSTR MEvent::GetName()
{
	return mb_NameIsNull ? NULL : ms_EventName;
}

#if defined(_DEBUG)
void MEvent::OnDebugNotify(MEventNotification Action)
{
	wchar_t szInfo[MAX_PATH];
	wcscpy_c(szInfo, L"MEvent: ");
	switch (Action)
	{
	case evn_Create:
		wcscat_c(szInfo, L"Create"); break;
	case evn_Open:
		wcscat_c(szInfo, L"Open"); break;
	case evn_Set:
		wcscat_c(szInfo, L"Set"); break;
	case evn_Reset:
		wcscat_c(szInfo, L"Reset"); break;
	case evn_Close:
		wcscat_c(szInfo, L"Close"); break;
	default:
		wcscat_c(szInfo, L"???");
	}
	wcscat_c(szInfo, L": ");
	if (mb_NameIsNull)
		_wsprintf(szInfo + _tcslen(szInfo), SKIPLEN(countof(szInfo) - _tcslen(szInfo)) L"Handle=0x%p", mh_Event);
	else
		wcscat_c(szInfo, ms_EventName);
	DEBUGSTREVT(szInfo);
	szInfo[0] = 0;
}
#endif
