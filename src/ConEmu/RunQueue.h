
/*
Copyright (c) 2013-2014 Maximus5
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

class CRealConsole;
class CVirtualConsole;

class CRunQueue
{
public:
	CRunQueue();
	~CRunQueue();

public:
	void RequestRConStartup(CRealConsole* pRCon);

public:
	void ProcessRunQueue(bool bFromPostMessage);

private:
	struct RunQueueItem
	{
		CVirtualConsole* pVCon; // NOT CVConGuard. We need only PTR for further checking...
	};
	MArray<RunQueueItem> m_RunQueue;
	// We need simple lock here, without overhead
	CRITICAL_SECTION mcs_QueueLock;
	// Is "Post" was requested?
	bool mb_PostRequested;
	// Advancing
	void AdvanceQueue();
	// Execution now?
	bool mb_InExecution;
	// as is
	DWORD mn_LastExecutionTick;
};
