
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

#define HIDE_USE_EXCEPTION_INFO

#include "../common/defines.h"

#include "AsyncCmdQueue.h"

#include "DllOptions.h"
#include "../common/WThreads.h"
#include "../common/ConEmuCheck.h"

AsyncCmdQueue* gpAsyncCmdQueue = nullptr;

static const DWORD ASYNC_WAIT_TERMINATE = 500;

AsyncCmdQueue::AsyncCmdQueue()
{
	event_.Create(nullptr, true, false, nullptr);
	threadHandle_.SetHandle(apiCreateThread(&AsyncCmdQueue::Thread, this, &threadId_, "HkAsyncCmdQueue"), CloseHandle);
}

AsyncCmdQueue::~AsyncCmdQueue()
{
	Stop();

	if (threadHandle_)
	{
		const auto waitResult = WaitForSingleObject(threadHandle_, ASYNC_WAIT_TERMINATE);
		if (waitResult != WAIT_OBJECT_0)
		{
			apiTerminateThread(threadHandle_, 100);
		}
	}
}

void AsyncCmdQueue::Initialize()
{
	if (!gpAsyncCmdQueue)
	{
		gpAsyncCmdQueue = new AsyncCmdQueue;
	}
}

void AsyncCmdQueue::Terminate()
{
	if (gpAsyncCmdQueue)
	{
		gpAsyncCmdQueue->Stop();
		AsyncCmdQueue* ptr = nullptr;
		std::swap(ptr, gpAsyncCmdQueue);
		delete ptr;
	}
}

void AsyncCmdQueue::AsyncExecute(CESERVER_REQ*&& request)
{
	if (stopped_.load())
	{
		ExecuteFreeResult(request);
		return;
	}
	if (!threadHandle_)
	{
		Exec(request);
		return;
	}
	MSectionLockSimple guard; guard.Lock(&lock_);
	data_.push_back(CallData{ request });
	event_.Set();
}

void AsyncCmdQueue::Stop()
{
	stopped_.store(true);
	event_.Set();
}

void AsyncCmdQueue::AsyncActivateConsole()
{
	const auto* hActiveCon = reinterpret_cast<HWND>(GetWindowLongPtr(ghConEmuWnd, GWLP_USERDATA));
	if (hActiveCon == ghConWnd)
	{
		return;
	}

	CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_ACTIVATECON, sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_ACTIVATECONSOLE));
	if (pIn)
	{
		pIn->ActivateCon.hConWnd = ghConWnd;
		AsyncExecute(std::move(pIn));
	}
}

DWORD AsyncCmdQueue::Thread(LPVOID lpThreadParameter)
{
	auto* obj = static_cast<AsyncCmdQueue*>(lpThreadParameter);
	if (obj)
	{
		obj->Run();
	}
	return 0;
}

void AsyncCmdQueue::Run()
{
	while (!stopped_.load())
	{
		const auto wasSet = event_.Wait(100);
		if (wasSet != WAIT_OBJECT_0)
			continue;

		CallData request{};
		{
			MSectionLockSimple guard; guard.Lock(&lock_);
			if (data_.empty())
				continue;
			request = data_.front();
			data_.pop_front();
		}

		if (request.request_)
		{
			Exec(request.request_);
		}
	}
}

void AsyncCmdQueue::Exec(CESERVER_REQ*& request)
{
	request->hdr.nSrcThreadId = GetCurrentThreadId();
	CESERVER_REQ* pOut = ExecuteGuiCmd(ghConWnd, request, ghConWnd, true);
	ExecuteFreeResult(request);
	ExecuteFreeResult(pOut);
}
