
/*
Copyright (c) 2012-2017 Maximus5
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

#define CE_MAX_INPUT_QUEUE_BUFFER 255

struct InQueue
{
	HANDLE hInputEvent; // <<-- gpSrv->hInputEvent. Выставляется в InputThread, флаг появления новых событий в очереди
	int nInputQueue, nMaxInputQueue;
	INPUT_RECORD* pInputQueue;
	INPUT_RECORD* pInputQueueEnd;
	INPUT_RECORD* pInputQueueRead;
	INPUT_RECORD* pInputQueueWrite;

	// Informational!!!
	LONG nUsedLen;

	BOOL Initialize(int anMaxInputQueue, HANDLE ahInputEvent/*by value*/);
	void Release();

	BOOL WriteInputQueue(const INPUT_RECORD *pr, BOOL bSetEvent = TRUE, DWORD nLength = 1);
	BOOL IsInputQueueEmpty();
	BOOL ReadInputQueue(INPUT_RECORD *prs, DWORD *pCount, BOOL bNoRemove = FALSE);
	BOOL GetNumberOfBufferEvents();
};
