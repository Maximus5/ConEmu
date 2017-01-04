
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

#define HIDE_USE_EXCEPTION_INFO

#include <Windows.h>
#include <WinCon.h>
#ifdef _DEBUG
#include <stdio.h>
#endif
#include "../common/Common.h"
#include "../common/ConEmuCheck.h"
#include "../common/WObjects.h"
#include "InQueue.h"

#define DEBUGSTRINPUTEVENT(s) //DEBUGSTR(s) // SetEvent(this->hInputEvent)


BOOL InQueue::Initialize(int MaxInputQueue, HANDLE ahInputEvent/*by value*/)
{
	this->nUsedLen = 0;
	this->nMaxInputQueue = MaxInputQueue;
	this->hInputEvent = ahInputEvent;
	this->pInputQueue = (INPUT_RECORD*)calloc(this->nMaxInputQueue, sizeof(INPUT_RECORD));
	if (!this->pInputQueue)
		return FALSE;
	this->pInputQueueEnd = this->pInputQueue+this->nMaxInputQueue;
	this->pInputQueueWrite = this->pInputQueue;
	this->pInputQueueRead = this->pInputQueueEnd;
	return TRUE;
}

void InQueue::Release()
{
	SafeFree(this->pInputQueue);
}

// this->nMaxInputQueue = CE_MAX_INPUT_QUEUE_BUFFER;
BOOL InQueue::WriteInputQueue(const INPUT_RECORD *pr, BOOL bSetEvent /*= TRUE*/, DWORD nLength /*= 1*/)
{
	// Передернуть буфер (записать в консоль то, что накопилось)
	if (pr == NULL)
	{
		if (bSetEvent)
		{
			DEBUGSTRINPUTEVENT(L"SetEvent(this->hInputEvent)\n");
			if (this->hInputEvent)
				SetEvent(this->hInputEvent);
		}
		return TRUE;
	}

	const INPUT_RECORD *pSrc = pr;

	while (nLength--)
	{
		INPUT_RECORD* pNext = this->pInputQueueWrite;

		// Проверяем, есть ли свободное место в буфере
		if (this->pInputQueueRead != this->pInputQueueEnd)
		{
			if (this->pInputQueueRead < this->pInputQueueEnd
				&& ((this->pInputQueueWrite+1) == this->pInputQueueRead))
			{
				return FALSE;
			}
		}

		// OK
		*pNext = *(pSrc++);
		this->pInputQueueWrite++;
		InterlockedIncrement(&nUsedLen);

		if (this->pInputQueueWrite >= this->pInputQueueEnd)
			this->pInputQueueWrite = this->pInputQueue;

		// Могут писать "пачку", тогда подождать ее окончания
		if (bSetEvent && !nLength)
		{
			DEBUGSTRINPUTEVENT(L"SetEvent(this->hInputEvent)\n");
			if (this->hInputEvent)
				SetEvent(this->hInputEvent);
		}

		// Подвинуть указатель чтения, если до этого буфер был пуст
		if (this->pInputQueueRead == this->pInputQueueEnd)
			this->pInputQueueRead = pNext;
	}

	return TRUE;
}

BOOL InQueue::IsInputQueueEmpty()
{
	if (this->pInputQueueRead != this->pInputQueueEnd
		&& this->pInputQueueRead != this->pInputQueueWrite)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL InQueue::ReadInputQueue(INPUT_RECORD *prs, DWORD *pCount, BOOL bNoRemove /*= FALSE*/)
{
	DWORD nCount = 0;

	if (!IsInputQueueEmpty())
	{
		DWORD n = *pCount;
		INPUT_RECORD *pSrc = this->pInputQueueRead;
		INPUT_RECORD *pEnd = (this->pInputQueueRead < this->pInputQueueWrite) ? this->pInputQueueWrite : this->pInputQueueEnd;
		INPUT_RECORD *pDst = prs;

		while (n && pSrc < pEnd)
		{
			*pDst = *pSrc; nCount++; pSrc++;
			InterlockedDecrement(&nUsedLen);
			//// Для приведения поведения к стандартному RealConsole&Far
			//if (pDst->EventType == KEY_EVENT
			//	// Для нажатия НЕ символьных клавиш
			//	&& pDst->Event.KeyEvent.bKeyDown && pDst->Event.KeyEvent.uChar.UnicodeChar < 32
			//	&& pSrc < (pEnd = (this->pInputQueueRead < this->pInputQueueWrite) ? this->pInputQueueWrite : this->pInputQueueEnd)) // и пока в буфере еще что-то есть
			//{
			//	while (pSrc < (pEnd = (this->pInputQueueRead < this->pInputQueueWrite) ? this->pInputQueueWrite : this->pInputQueueEnd)
			//		&& pSrc->EventType == KEY_EVENT
			//		&& pSrc->Event.KeyEvent.bKeyDown
			//		&& pSrc->Event.KeyEvent.wVirtualKeyCode == pDst->Event.KeyEvent.wVirtualKeyCode
			//		&& pSrc->Event.KeyEvent.wVirtualScanCode == pDst->Event.KeyEvent.wVirtualScanCode
			//		&& pSrc->Event.KeyEvent.uChar.UnicodeChar == pDst->Event.KeyEvent.uChar.UnicodeChar
			//		&& pSrc->Event.KeyEvent.dwControlKeyState == pDst->Event.KeyEvent.dwControlKeyState)
			//	{
			//		pDst->Event.KeyEvent.wRepeatCount++; pSrc++;
			//	}
			//}
			n--; pDst++;
		}

		if (pSrc == this->pInputQueueEnd)
			pSrc = this->pInputQueue;

		TODO("Доделать чтение начала буфера, если считали его конец");
		//
		if (!bNoRemove)
		{
			this->pInputQueueRead = pSrc;
		}
	}

	*pCount = nCount;
	return (nCount>0);
}

BOOL InQueue::GetNumberOfBufferEvents()
{
	DWORD nCount = 0;

	if (!IsInputQueueEmpty())
	{
		INPUT_RECORD *pSrc = this->pInputQueueRead;
		INPUT_RECORD *pEnd = (this->pInputQueueRead < this->pInputQueueWrite) ? this->pInputQueueWrite : this->pInputQueueEnd;

		while (pSrc < pEnd)
		{
			nCount++; pSrc++;
		}

		if (pSrc == this->pInputQueueEnd)
		{
			pSrc = this->pInputQueue;
			pEnd = (this->pInputQueueRead < this->pInputQueueWrite) ? this->pInputQueueWrite : this->pInputQueueEnd;

			while(pSrc < pEnd)
			{
				nCount++; pSrc++;
			}
		}
	}

	return nCount;
}
