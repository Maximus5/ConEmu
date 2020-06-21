
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

#include "../common/defines.h"

#include <intrin.h>

// Message Logger
// Originally from http://preshing.com/20120522/lightweight-in-memory-logging
namespace InputLogger
{
	static const int BUFFER_INFO_SIZE = RELEASEDEBUGTEST(0x1000,0x1000);   // Must be a power of 2
	struct Event {
		DWORD time;
		enum Source {
			evt_Empty,
			evt_ReadInputQueue,
			evt_SetEvent,
			evt_ResetEvent,
			evt_SendStart,
			evt_SendEnd,
			evt_ProcessInputMessage,
			evt_WriteInputQueue1,
			evt_WaitInputReady,
			evt_WriteInputQueue2,
			evt_InputQueueFlush,
			evt_Overflow,
			evt_SpeedHigh,
			evt_SpeedLow,
			evt_WaitConSize,
			evt_WaitConEmpty,
			evt_WriteConInput,
			evt_ConSbiChanged,
			evt_ConDataChanged,
		} what;
		LONG val;
		INPUT_RECORD ir;
	};
	extern Event g_evt[BUFFER_INFO_SIZE];
	extern LONG g_evtidx;
	extern LONG g_overflow;

	inline void Log(Event::Source what, LONG val = 0)
	{
		// Get next message index
		// Wrap to buffer size
		LONG i = (_InterlockedIncrement(&g_evtidx) & (BUFFER_INFO_SIZE - 1));
		// Write a message at this index
		g_evt[i].what = what;
		g_evt[i].time = GetTickCount();
		g_evt[i].val = val;
		g_evt[i].ir.EventType = 0;
	}

	inline void Log(Event::Source what, const INPUT_RECORD& ir, LONG val = 0)
	{
		// Get next message index
		// Wrap to buffer size
		LONG i = (_InterlockedIncrement(&g_evtidx) & (BUFFER_INFO_SIZE - 1));
		// Write a message at this index
		g_evt[i].what = what;
		g_evt[i].time = GetTickCount();
		// Fill info
		g_evt[i].val = val;
		if (ir.EventType == KEY_EVENT)
		{
			ZeroStruct(g_evt[i].ir);
			g_evt[i].ir.EventType = ir.EventType;
			g_evt[i].ir.Event.KeyEvent.bKeyDown = ir.Event.KeyEvent.bKeyDown;
			g_evt[i].ir.Event.KeyEvent.wRepeatCount = ir.Event.KeyEvent.wRepeatCount;
			g_evt[i].ir.Event.KeyEvent.dwControlKeyState = ir.Event.KeyEvent.dwControlKeyState;
		}
		else
		{
			g_evt[i].ir = ir;
		}
	}
}
