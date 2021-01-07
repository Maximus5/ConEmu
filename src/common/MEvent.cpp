
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

#define SHOWDEBUGSTR

#define HIDE_USE_EXCEPTION_INFO

#include "defines.h"
#include "MAssert.h"
#include "MEvent.h"

#define DEBUGSTREVT(s) DEBUGSTR(s)

MEvent::MEvent()
{
}

MEvent::~MEvent()
{
	if (handle_)
		Close();
}

void MEvent::Close()
{
	#if defined(_DEBUG)
	if (debugNotify_ && handle_)
		OnDebugNotify(MEventNotification::Close);
	#endif

	lastError_ = 0;
	eventName_[0] = 0;
	nameIsNull_ = false;

	if (handle_)
	{
		::CloseHandle(handle_);
		handle_ = nullptr;
	}
}

void MEvent::InitName(const wchar_t *aszTemplate, const DWORD parm1)
{
	if (handle_)
		Close();

	nameIsNull_ = (aszTemplate == nullptr);
	if (!nameIsNull_)
		swprintf_c(eventName_, aszTemplate, parm1);
	lastError_ = 0;
}

HANDLE MEvent::Create(SECURITY_ATTRIBUTES* pSec, const bool bManualReset, const bool bInitialState, const wchar_t* lpName)
{
	if (handle_)  // if was already opened - return the value!
		return handle_;

	if (lpName)
	{
		_ASSERTE(_tcslen(lpName) < countof(eventName_));
		nameIsNull_ = false;
		lstrcpyn(eventName_, lpName, countof(eventName_));
	}
	else
	{
		nameIsNull_ = true;
		eventName_[0] = 0;
	}

	return Open(true, pSec, bManualReset, bInitialState);
}

HANDLE MEvent::Open(const bool bCreate /*= false*/, SECURITY_ATTRIBUTES* pSec /*= nullptr*/, const bool bManualReset /*= false*/, const bool bInitialState /*= false*/)
{
	if (handle_)  // if was already opened - return the value!
		return handle_;

	if ((eventName_[0] == 0) && !nameIsNull_)
	{
		_ASSERTE(eventName_[0]!=0);
		lastError_ = ERROR_BAD_ARGUMENTS;
		return nullptr;
	}

	if (nameIsNull_ && !bCreate)
	{
		_ASSERTE(bCreate);
		lastError_ = ERROR_BAD_ARGUMENTS;
		return nullptr;
	}

	lastError_ = 0;

	if (bCreate)
	{
		handle_ = ::CreateEvent(pSec, bManualReset, FALSE, nameIsNull_ ? nullptr : eventName_);
		// Ensure that Event has proper state
		if (handle_)
		{
			if (bInitialState)
				::SetEvent(handle_);
			else
				::ResetEvent(handle_);
		}
	}
	else
	{
		handle_ = ::OpenEvent(EVENT_MODIFY_STATE|SYNCHRONIZE, FALSE, eventName_);
	}

	if (!handle_)
		lastError_ = ::GetLastError();

	#if defined(_DEBUG)
	if (debugNotify_)
		OnDebugNotify(bCreate ? MEventNotification::Create : MEventNotification::Open);
	#endif

	return handle_;
}

bool MEvent::Set()
{
	if (!handle_)
	{
		_ASSERTE(handle_ != nullptr);
		lastError_ = ERROR_BAD_ARGUMENTS;
		return false;
	}

	#if defined(_DEBUG)
	if (debugNotify_)
		OnDebugNotify(MEventNotification::Set);
	#endif

	const BOOL b = ::SetEvent(handle_);

	#if defined(_DEBUG)
	lastError_ = ::GetLastError();
	#endif

	return (b != FALSE);
}

bool MEvent::Reset()
{
	if (!handle_)
	{
		_ASSERTE(handle_ != nullptr);
		lastError_ = ERROR_BAD_ARGUMENTS;
		return false;
	}

	#if defined(_DEBUG)
	if (debugNotify_)
		OnDebugNotify(MEventNotification::Reset);
	#endif

	const BOOL b = ::ResetEvent(handle_);

	#if defined(_DEBUG)
	lastError_ = ::GetLastError();
	#endif

	return (b != FALSE);
}

DWORD MEvent::Wait(const DWORD dwMilliseconds, const bool abAutoOpen/*=TRUE*/)
{
	if (!handle_ && abAutoOpen)
		Open();

	if (!handle_)
		return WAIT_ABANDONED;

	const DWORD dwWait = ::WaitForSingleObject(handle_, dwMilliseconds);

	#if defined(_DEBUG)
	lastError_ = ::GetLastError();
	#endif

	return dwWait;
}

HANDLE MEvent::GetHandle() const
{
	return handle_;
}

LPCWSTR MEvent::GetName() const
{
	return nameIsNull_ ? nullptr : eventName_;
}

#if defined(_DEBUG)
void MEvent::OnDebugNotify(const MEventNotification action) const
{
	wchar_t szInfo[MAX_PATH] = L"";
	wcscpy_c(szInfo, L"MEvent: ");
	switch (action)
	{
	case MEventNotification::Create:
		wcscat_c(szInfo, L"Create"); break;
	case MEventNotification::Open:
		wcscat_c(szInfo, L"Open"); break;
	case MEventNotification::Set:
		wcscat_c(szInfo, L"Set"); break;
	case MEventNotification::Reset:
		wcscat_c(szInfo, L"Reset"); break;
	case MEventNotification::Close:
		wcscat_c(szInfo, L"Close"); break;
	default:
		wcscat_c(szInfo, L"???");
	}
	wcscat_c(szInfo, L": ");
	if (nameIsNull_)
		swprintf_c(szInfo + _tcslen(szInfo), countof(szInfo) - _tcslen(szInfo)/*#SECURELEN*/, L"Handle=0x%p", handle_);
	else
		wcscat_c(szInfo, eventName_);
	DEBUGSTREVT(szInfo);
	szInfo[0] = 0;
}
#endif
