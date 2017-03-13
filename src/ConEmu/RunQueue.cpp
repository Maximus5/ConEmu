
/*
Copyright (c) 2013-2017 Maximus5
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
#include "../common/MSectionSimple.h"
#include "../common/WThreads.h"
#include "ConEmu.h"
#include "RunQueue.h"
#include "VConGroup.h"
#include "VirtualConsole.h"

#define RUNQUEUE_TIMER_DELAY 100
#define RUNQUEUE_WAIT_TERMINATION 1000

CRunQueue::CRunQueue()
{
	mn_LastExecutionTick = 0;
	mb_InExecution = false;
	mpcs_QueueLock = new MSectionSimple(true);

	mn_ThreadId = 0;
	mb_Terminate = false;
	mh_AdvanceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	// Don't start Queue processor now, wait until MainWindow is created
	mh_Thread = NULL; mn_ThreadId = 0;
}

CRunQueue::~CRunQueue()
{
	if (mh_Thread)
	{
		Terminate();
		SafeCloseHandle(mh_Thread);
	}

	SafeCloseHandle(mh_AdvanceEvent);
	SafeDelete(mpcs_QueueLock);
}

void CRunQueue::StartQueue()
{
	_ASSERTE(gpConEmu->mn_StartupFinished >= CConEmuMain::ss_CreateQueueReady);

	if (mh_Thread)
	{
		_ASSERTE(mh_Thread == NULL);
		return;
	}

	mh_Thread = apiCreateThread(RunQueueThreadHelper, this, &mn_ThreadId, "RunQueueThreadHelper");
	_ASSERTE(mh_Thread != NULL);

	// Don't wait, start first RCon immediately
	AdvanceQueue();
}

void CRunQueue::Terminate()
{
	if (this && mh_Thread)
	{
		mb_Terminate = true;
		SetEvent(mh_AdvanceEvent);

		if (GetCurrentThreadId() == mn_ThreadId)
			return; // Don't block

		DWORD nWait = WaitForSingleObject(mh_Thread, RUNQUEUE_WAIT_TERMINATION);

		#ifdef _DEBUG
		if (nWait == WAIT_TIMEOUT)
		{
			_ASSERTE(FALSE && "Terminating RunQueue(mh_Thread) thread");
			if (IsDebuggerPresent())
			{
				nWait = WaitForSingleObject(mh_Thread, 0);
			}
		}
		#endif

		if (nWait == WAIT_TIMEOUT)
		{
			apiTerminateThread(mh_Thread, 100);
		}
	}
}

void CRunQueue::RequestRConStartup(CRealConsole* pRCon)
{
	bool bFound = false;
	CVirtualConsole* pVCon = pRCon->VCon();

	// Должен вызываться в главной нити, чтобы соблюсти порядок создания при запуске группы
	_ASSERTE(isMainThread() == true);

	MSectionLockSimple cs;
	cs.Lock(mpcs_QueueLock);

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

	cs.Unlock();

	// Trigger our thread
	AdvanceQueue();
}

bool CRunQueue::isRunnerThread()
{
	DWORD dwTID = GetCurrentThreadId();
	return dwTID == mn_ThreadId;
}

DWORD CRunQueue::RunQueueThreadHelper(LPVOID lpParameter)
{
	CRunQueue* p = (CRunQueue*)lpParameter;
	DWORD nRc = p ? p->RunQueueThread() : 999;
	_ASSERTE(nRc == 0);
	return nRc;
}

DWORD CRunQueue::RunQueueThread()
{
	DWORD nWait;
	bool  bPending = false;

	while (!mb_Terminate)
	{
		nWait = WaitForSingleObject(mh_AdvanceEvent, RUNQUEUE_TIMER_DELAY);

		gpConEmu->OnTimer_Background();

		if (nWait == WAIT_TIMEOUT)
		{
			if (CVConGroup::InCreateGroup())
				continue;

			// To avoid "initialization hangs" we enforce bPending
			// even if mh_AdvanceEvent was not set (unexpected behavior?)
			bPending = !m_RunQueue.empty();

			#ifdef _DEBUG
			if (bPending)
			{
				DWORD nWait2 = WaitForSingleObject(mh_AdvanceEvent, 0);
				_ASSERTE((nWait2 == WAIT_OBJECT_0) && "AdvanceQueue() was not called?");
			}
			#endif
		}
		else if (nWait == WAIT_OBJECT_0)
		{
			bPending = !m_RunQueue.empty();
		}

		if (bPending)
		{
			ProcessRunQueue();
		}
	}

	return 0;
}

void CRunQueue::ProcessRunQueue()
{
	#ifdef _DEBUG
	// We run in self thread
	if (mb_InExecution)
	{
		_ASSERTE(!mb_InExecution);
	}
	#endif

	// Block adding new requests from other threads
	MSectionLockSimple cs;
	cs.Lock(mpcs_QueueLock);
	RunQueueItem item = {};
	while (m_RunQueue.pop_back(item))
	{
		//item = m_RunQueue[0];
		//m_RunQueue.erase(0);

		m_WorkStack.push_back(item);
	}
	cs.Unlock();

	DWORD nCurDelay, nWaitExtra;
	bool bOpt;

	// And process stack
	while (!mb_Terminate && m_WorkStack.pop_back(item))
	{
		if (!gpConEmu->isValid(item.pVCon))
			continue;

		CVConGuard VCon(item.pVCon);

		if (!VCon.VCon())
			continue;

		// Avoid too fast process creation?
		if (mn_LastExecutionTick)
		{
			nCurDelay = (GetTickCount() - mn_LastExecutionTick);
			if (nCurDelay < gpSet->nStartCreateDelay)
			{
				nWaitExtra = (gpSet->nStartCreateDelay - nCurDelay);
				Sleep(nWaitExtra);
			}
		}

		mb_InExecution = true;
		bOpt = gpConEmu->ExecuteProcessPrepare();

		VCon->RCon()->OnStartProcessAllowed();

		gpConEmu->ExecuteProcessFinished(bOpt);
		mb_InExecution = false;

		// Remember last execution moment
		mn_LastExecutionTick = GetTickCount();
	}
}

void CRunQueue::AdvanceQueue()
{
	// If Startup was not completed yet - just do nothing
	if (gpConEmu->mn_StartupFinished < CConEmuMain::ss_CreateQueueReady)
		return;

	// Nothing to do?
	if (m_RunQueue.empty())
		return;

	// Execution will start after group (task or -cmdlist) creates all RCon's
	// AdvanceQueue will be called from CVConGroup::OnCreateGroupEnd()
	if (CVConGroup::InCreateGroup())
		return;

	// Trigger our thread
	if (mh_Thread)
	{
		SetEvent(mh_AdvanceEvent);
	}
	else
	{
		_ASSERTE(mh_Thread);
		// In case that thread was not started?
		ProcessRunQueue();
	}
}
