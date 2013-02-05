
/*
Copyright (c) 2013 Maximus5
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

#include "header.h"

#include "../common/MArray.h"
#include "ConEmu.h"
#include "RunQueue.h"
#include "VConGroup.h"
#include "VirtualConsole.h"

#define RUNQUEUE_TIMER_DELAY 100
#define RUNQUEUE_DELAY_LIMIT (((RUNQUEUE_TIMER_DELAY) * 3) >> 2)


CRunQueue::CRunQueue()
{
	mn_LastExecutionTick = 0;
	mb_PostRequested = false;
	mb_InExecution = false;
	InitializeCriticalSection(&mcs_QueueLock);
}

CRunQueue::~CRunQueue()
{
	DeleteCriticalSection(&mcs_QueueLock);
}

void CRunQueue::RequestRConStartup(CRealConsole* pRCon)
{
	bool bFound = false;
	CVirtualConsole* pVCon = pRCon->VCon();

	// Должен вызываться в главной нити, чтобы соблюсти порядок создания при запуске группы
	_ASSERTE(gpConEmu->isMainThread() == true);

	EnterCriticalSection(&mcs_QueueLock);

	// May be exist already in queue?
	for (INT_PTR i = m_RunQueue.size(); (--i) >= 0;)
	{
		if (m_RunQueue[i].pVCon == pVCon)
		{
			bFound = true;
			break;
		}
	}

	// push
	if (!bFound)
	{
		RunQueueItem item = {pVCon};
		m_RunQueue.push_back(item);
	}

	// Call "in section" to avoid "KillTimer" from main thread
	AdvanceQueue();

	LeaveCriticalSection(&mcs_QueueLock);
}

void CRunQueue::ProcessRunQueue(bool bFromPostMessage)
{
	// Previous execution was not finished yet?
	if (mb_InExecution)
	{
		if (!m_RunQueue.empty())
		{
			// Ensure timer is ON
			gpConEmu->SetRunQueueTimer(true, RUNQUEUE_TIMER_DELAY);
		}

		// Skip this timer
		return;
	}

	if (bFromPostMessage)
	{
		mb_PostRequested = false;
	}
	else if (RUNQUEUE_DELAY_LIMIT > 0)
	{
		// This is from WM_TIMER, check delay (if not first call)
		if (mn_LastExecutionTick != 0)
		{
			DWORD nCurDelay = (GetTickCount() - mn_LastExecutionTick);

			if (nCurDelay < RUNQUEUE_DELAY_LIMIT)
			{
				// Timeout was not passed till last execution

				if (!m_RunQueue.empty())
				{
					// Ensure timer is ON
					gpConEmu->SetRunQueueTimer(true, RUNQUEUE_TIMER_DELAY);
				}

				return;
			}
		}
	}

	_ASSERTE(gpConEmu->isMainThread() == true);

	if (CVConGroup::InCreateGroup())
	{
		if (!m_RunQueue.empty())
		{
			// Ensure timer is ON
			gpConEmu->SetRunQueueTimer(true, RUNQUEUE_TIMER_DELAY);
		}

		return;
	}

	CVConGuard VCon;

	EnterCriticalSection(&mcs_QueueLock);
	// Nothing to run?
	if (m_RunQueue.empty())
	{
		gpConEmu->SetRunQueueTimer(false, 0);
	}
	else
	{
		while (!m_RunQueue.empty())
		{
			RunQueueItem item = m_RunQueue[0];
			m_RunQueue.erase(0);

			if (gpConEmu->isValid(item.pVCon))
			{
				VCon = item.pVCon;
				break;
			}
		}
	}
	LeaveCriticalSection(&mcs_QueueLock);


	// Queue is empty now?
	if (VCon.VCon())
	{
		mb_InExecution = true;
		bool bOpt = gpConEmu->ExecuteProcessPrepare();

		VCon->RCon()->OnStartProcessAllowed();

		gpConEmu->ExecuteProcessFinished(bOpt);
		mb_InExecution = false;

		// Remember last execution moment
		mn_LastExecutionTick = GetTickCount();
	}
	else
	{
		#ifdef _DEBUG
		if (!m_RunQueue.empty())
		{
			_ASSERTE(m_RunQueue.empty());
		}
		#endif
	}

	
	// Trigger next RCon execution
	if (!m_RunQueue.empty())
	{
		AdvanceQueue();
	}
}

void CRunQueue::AdvanceQueue()
{
	if (m_RunQueue.empty())
	{
		return;
	}

	DWORD nCurDelay = (GetTickCount() - mn_LastExecutionTick);

	if ((mn_LastExecutionTick == 0) // this is first attempt to start console, run it NOW
		|| (nCurDelay >= RUNQUEUE_DELAY_LIMIT)) // Timeout passed till last execution
	{
		// Just trigger execution in main thread
		if (!mb_PostRequested)
		{
			mb_PostRequested = true;
			PostMessage(ghWnd, gpConEmu->mn_MsgRequestRunProcess, 0, 0);
		}
	}

	// Ensure timer is ON
	gpConEmu->SetRunQueueTimer(true, RUNQUEUE_TIMER_DELAY);
}
