
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

#define HIDE_USE_EXCEPTION_INFO
#include <windows.h>
#include "defines.h"
#include "MAssert.h"
#include "MTimer.h"

// Обертка для таймера
CTimer::CTimer()
{
	mh_Wnd = NULL;
	mn_TimerId = mn_Elapse = 0;
	mb_Started = false;
}

CTimer::~CTimer()
{
	Stop();
}

bool CTimer::IsStarted()
{
	return mb_Started;
}

void CTimer::Start(UINT anElapse /*= 0*/)
{
	if (!mh_Wnd || mb_Started)
		return;

	mb_Started = true;
	INT_PTR iRc = SetTimer(mh_Wnd, mn_TimerId, anElapse ? anElapse : mn_Elapse, NULL);
	UNREFERENCED_PARAMETER(iRc);
	#ifdef _DEBUG
	DWORD nErr = GetLastError();
	_ASSERTE(iRc!=0);
	#endif
}

void CTimer::Stop()
{
	if (mb_Started)
	{
		mb_Started = false;
		KillTimer(mh_Wnd, mn_TimerId);
	}
}

void CTimer::Restart(UINT anElapse /*= 0*/)
{
	if (mb_Started)
		Stop();

	Start(anElapse);
}

void CTimer::Init(HWND ahWnd, UINT_PTR anTimerID, UINT anElapse)
{
	Stop();
	mh_Wnd = ahWnd; mn_TimerId = anTimerID; mn_Elapse = anElapse;
}
