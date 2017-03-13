
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

#define SHOWDEBUGSTR

#include "ConEmuC.h"
#include "Queue.h"

#define DEBUGSTRINPUTPIPE(s) //DEBUGSTR(s) // ConEmuC: Received key... / ConEmuC: Received input
#define DEBUGSTRINPUTEVENT(s) //DEBUGSTR(s) // SetEvent(gpSrv->hInputEvent)
#define DEBUGLOGINPUT(s) //DEBUGSTR(s) // ConEmuC.MouseEvent(X=
#define DEBUGSTRINPUTWRITE(s) //DEBUGSTR(s) // *** ConEmuC.MouseEvent(X=
#define DEBUGSTRINPUTWRITEALL(s) //DEBUGSTR(s) // *** WriteConsoleInput(Write=
#define DEBUGSTRINPUTWRITEFAIL(s) //DEBUGSTR(s) // ### WriteConsoleInput(Write=

#ifdef _DEBUG
// Only for input_bug search purposes in Debug builds
const LONG gn_LogWrittenCharsMax = 4096; // must be power of 2
wchar_t gs_LogWrittenChars[gn_LogWrittenCharsMax*2+1] = L""; // "+1" для ASCIIZ
LONG gn_LogWrittenChars = -1;
#endif

BOOL ProcessInputMessage(MSG64::MsgStr &msg, INPUT_RECORD &r)
{
	memset(&r, 0, sizeof(r));
	BOOL lbOk = FALSE;

	if (!UnpackInputRecord(&msg, &r))
	{
		_ASSERT(FALSE);
	}
	else
	{
		TODO("Сделать обработку пачки сообщений, вдруг они накопились в очереди?");
		//#ifdef _DEBUG
		//if (r.EventType == KEY_EVENT && (r.Event.KeyEvent.wVirtualKeyCode == 'C' || r.Event.KeyEvent.wVirtualKeyCode == VK_CANCEL))
		//{
		//	DEBUGSTR(L"  ---  CtrlC/CtrlBreak recieved\n");
		//}
		//#endif
		bool lbProcessEvent = false;
		bool lbIngoreKey = false;

		if (r.EventType == KEY_EVENT && r.Event.KeyEvent.bKeyDown &&
		        (r.Event.KeyEvent.wVirtualKeyCode == 'C' || r.Event.KeyEvent.wVirtualKeyCode == VK_CANCEL)
		        && (      // Удерживается ТОЛЬКО Ctrl
		            (r.Event.KeyEvent.dwControlKeyState & CTRL_MODIFIERS) &&
		            ((r.Event.KeyEvent.dwControlKeyState & ALL_MODIFIERS)
		             == (r.Event.KeyEvent.dwControlKeyState & CTRL_MODIFIERS))
		        )
		  )
		{
			wchar_t szLog[100];
			lbProcessEvent = true;
			LogString(L"  ---  CtrlC/CtrlBreak recieved");
			DWORD dwMode = 0;
			GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &dwMode);

			// CTRL+C (and Ctrl+Break?) is processed by the system and is not placed in the input buffer
			if ((dwMode & ENABLE_PROCESSED_INPUT) == ENABLE_PROCESSED_INPUT)
				lbIngoreKey = lbProcessEvent = true;
			else
				lbProcessEvent = false;

			if (RELEASEDEBUGTEST((gpLogSize!=NULL),true))
			{
			bool bAlt = isPressed(VK_MENU), bShift = isPressed(VK_SHIFT), bCtrl = isPressed(VK_CONTROL);
			if (bAlt || bShift || !bCtrl)
			{
				msprintf(szLog, countof(szLog), L"  ---  CtrlC/CtrlBreak may fail because of bad Alt/Shift/Ctrl state (%u,%u,%u)!", (UINT)bAlt, (UINT)bShift, (UINT)bCtrl);
				LogString(szLog);
			}
			if (!lbProcessEvent)
			{
				LogString(L"  ---  CtrlC/CtrlBreak may fail because of disabled ENABLE_PROCESSED_INPUT!");
			}
			}

			if (lbProcessEvent)
			{
				// Issue 590: GenerateConsoleCtrlEvent does not break ReadConsole[A|W] function!
				SetLastError(0);
				LRESULT lSendRc =
				SendMessage(ghConWnd, WM_KEYDOWN, r.Event.KeyEvent.wVirtualKeyCode, 0);
				DWORD nErrCode = GetLastError();
				msprintf(szLog, countof(szLog), L"  ---  CtrlC/CtrlBreak sent (%u,%u)", LODWORD(lSendRc), nErrCode);
				LogString(szLog);
			}

			if (lbIngoreKey)
				return FALSE;

			// In the real console, when CtrlBreak is received, input buffer is cleared.
			// Otherwise, in Far Manager for example, it's impossible to stop some operation,
			// it will try to peek old data and CtrlBreak may be left unread
			if (r.Event.KeyEvent.wVirtualKeyCode == VK_CANCEL)
			{
				LogString(L"  ---  VK_CANCEL received, flushing, sending...");
				FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
				SendConsoleEvent(&r, 1);
				return FALSE;
			}
		}

		#ifdef _DEBUG
		if (r.EventType == KEY_EVENT && r.Event.KeyEvent.bKeyDown &&
		        r.Event.KeyEvent.wVirtualKeyCode == VK_F11)
		{
			LogString(L"  ---  F11 recieved\n");
		}
		#endif

		#ifdef _DEBUG
		if (r.EventType == MOUSE_EVENT)
		{
			static DWORD nLastEventTick = 0;

			if (nLastEventTick && (GetTickCount() - nLastEventTick) > 2000)
			{
				OutputDebugString(L".\n");
			}

			wchar_t szDbg[60];
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"    ConEmuC.MouseEvent(X=%i,Y=%i,Btns=0x%04x,Moved=%i)\n", r.Event.MouseEvent.dwMousePosition.X, r.Event.MouseEvent.dwMousePosition.Y, r.Event.MouseEvent.dwButtonState, (r.Event.MouseEvent.dwEventFlags & MOUSE_MOVED));
			DEBUGLOGINPUT(szDbg);
			nLastEventTick = GetTickCount();
		}
		#endif

		// Запомнить, когда была последняя активность пользователя
		if (r.EventType == KEY_EVENT
		        || (r.EventType == MOUSE_EVENT
		            && (r.Event.MouseEvent.dwButtonState || r.Event.MouseEvent.dwEventFlags
		                || r.Event.MouseEvent.dwEventFlags == DOUBLE_CLICK)))
		{
			gpSrv->dwLastUserTick = GetTickCount();
		}

		lbOk = TRUE;
		//SendConsoleEvent(&r, 1);
	}

	return lbOk;
}

//// gpSrv->hInputThread && gpSrv->dwInputThreadId
//DWORD WINAPI InputThread(LPVOID lpvParam)
//{
//	MSG msg;
//	//while (GetMessage(&msg,0,0,0))
//	INPUT_RECORD rr[MAX_EVENTS_PACK];
//
//	while (TRUE) {
//		if (!PeekMessage(&msg,0,0,0,PM_REMOVE)) {
//			Sleep(10);
//			continue;
//		}
//		if (msg.message == WM_QUIT)
//			break;
//
//		if (ghQuitEvent) {
//			if (WaitForSingleObject(ghQuitEvent, 0) == WAIT_OBJECT_0)
//				break;
//		}
//		if (msg.message == WM_NULL) {
//			_ASSERTE(msg.message != WM_NULL);
//			continue;
//		}
//
//		//if (msg.message == INPUT_THREAD_ALIVE_MSG) {
//		//	//pRCon->mn_FlushOut = msg.wParam;
//		//	TODO("INPUT_THREAD_ALIVE_MSG");
//		//	continue;
//		//
//		//} else {
//
//		// обработка пачки сообщений, вдруг они накопились в очереди?
//		UINT nCount = 0;
//		while (nCount < MAX_EVENTS_PACK)
//		{
//			if (msg.message == WM_NULL) {
//				_ASSERTE(msg.message != WM_NULL);
//			} else {
//				if (ProcessInputMessage(msg, rr[nCount]))
//					nCount++;
//			}
//			if (!PeekMessage(&msg,0,0,0,PM_REMOVE))
//				break;
//			if (msg.message == WM_QUIT)
//				break;
//		}
//		if (nCount && msg.message != WM_QUIT) {
//			SendConsoleEvent(rr, nCount);
//		}
//		//}
//	}
//
//	return 0;
//}


//// gpSrv->nMaxInputQueue = CE_MAX_INPUT_QUEUE_BUFFER;
//BOOL WriteInputQueue(const INPUT_RECORD *pr, BOOL bSetEvent /*= TRUE*/)
//{
//	// Передернуть буфер (записать в консоль то, что накопилось)
//	if (pr == NULL)
//	{
//		if (bSetEvent)
//		{
//			DEBUGSTRINPUTEVENT(L"SetEvent(gpSrv->hInputEvent)\n");
//			SetEvent(gpSrv->hInputEvent);
//		}
//		return TRUE;
//	}
//
//
//	INPUT_RECORD* pNext = gpSrv->pInputQueueWrite;
//
//	// Проверяем, есть ли свободное место в буфере
//	if (gpSrv->pInputQueueRead != gpSrv->pInputQueueEnd)
//	{
//		if (gpSrv->pInputQueueRead < gpSrv->pInputQueueEnd
//			&& ((gpSrv->pInputQueueWrite+1) == gpSrv->pInputQueueRead))
//		{
//			return FALSE;
//		}
//	}
//
//	// OK
//	*pNext = *pr;
//	gpSrv->pInputQueueWrite++;
//
//	if (gpSrv->pInputQueueWrite >= gpSrv->pInputQueueEnd)
//		gpSrv->pInputQueueWrite = gpSrv->pInputQueue;
//
//	// Могут писать "пачку", тогда подождать ее окончания
//	if (bSetEvent)
//	{
//		DEBUGSTRINPUTEVENT(L"SetEvent(gpSrv->hInputEvent)\n");
//		SetEvent(gpSrv->hInputEvent);
//	}
//
//	// Подвинуть указатель чтения, если до этого буфер был пуст
//	if (gpSrv->pInputQueueRead == gpSrv->pInputQueueEnd)
//		gpSrv->pInputQueueRead = pNext;
//
//	return TRUE;
//}
//
//BOOL IsInputQueueEmpty()
//{
//	if (gpSrv->pInputQueueRead != gpSrv->pInputQueueEnd
//		&& gpSrv->pInputQueueRead != gpSrv->pInputQueueWrite)
//		return FALSE;
//
//	return TRUE;
//}
//
//BOOL ReadInputQueue(INPUT_RECORD *prs, DWORD *pCount)
//{
//	DWORD nCount = 0;
//
//	if (!IsInputQueueEmpty())
//	{
//		DWORD n = *pCount;
//		INPUT_RECORD *pSrc = gpSrv->pInputQueueRead;
//		INPUT_RECORD *pEnd = (gpSrv->pInputQueueRead < gpSrv->pInputQueueWrite) ? gpSrv->pInputQueueWrite : gpSrv->pInputQueueEnd;
//		INPUT_RECORD *pDst = prs;
//
//		while(n && pSrc < pEnd)
//		{
//			*pDst = *pSrc; nCount++; pSrc++;
//			//// Для приведения поведения к стандартному RealConsole&Far
//			//if (pDst->EventType == KEY_EVENT
//			//	// Для нажатия НЕ символьных клавиш
//			//	&& pDst->Event.KeyEvent.bKeyDown && pDst->Event.KeyEvent.uChar.UnicodeChar < 32
//			//	&& pSrc < (pEnd = (gpSrv->pInputQueueRead < gpSrv->pInputQueueWrite) ? gpSrv->pInputQueueWrite : gpSrv->pInputQueueEnd)) // и пока в буфере еще что-то есть
//			//{
//			//	while (pSrc < (pEnd = (gpSrv->pInputQueueRead < gpSrv->pInputQueueWrite) ? gpSrv->pInputQueueWrite : gpSrv->pInputQueueEnd)
//			//		&& pSrc->EventType == KEY_EVENT
//			//		&& pSrc->Event.KeyEvent.bKeyDown
//			//		&& pSrc->Event.KeyEvent.wVirtualKeyCode == pDst->Event.KeyEvent.wVirtualKeyCode
//			//		&& pSrc->Event.KeyEvent.wVirtualScanCode == pDst->Event.KeyEvent.wVirtualScanCode
//			//		&& pSrc->Event.KeyEvent.uChar.UnicodeChar == pDst->Event.KeyEvent.uChar.UnicodeChar
//			//		&& pSrc->Event.KeyEvent.dwControlKeyState == pDst->Event.KeyEvent.dwControlKeyState)
//			//	{
//			//		pDst->Event.KeyEvent.wRepeatCount++; pSrc++;
//			//	}
//			//}
//			n--; pDst++;
//		}
//
//		if (pSrc == gpSrv->pInputQueueEnd)
//			pSrc = gpSrv->pInputQueue;
//
//		TODO("Доделать чтение начала буфера, если считали его конец");
//		//
//		gpSrv->pInputQueueRead = pSrc;
//	}
//
//	*pCount = nCount;
//	return (nCount>0);
//}
//
//#ifdef _DEBUG
//BOOL GetNumberOfBufferEvents()
//{
//	DWORD nCount = 0;
//
//	if (!IsInputQueueEmpty())
//	{
//		INPUT_RECORD *pSrc = gpSrv->pInputQueueRead;
//		INPUT_RECORD *pEnd = (gpSrv->pInputQueueRead < gpSrv->pInputQueueWrite) ? gpSrv->pInputQueueWrite : gpSrv->pInputQueueEnd;
//
//		while(pSrc < pEnd)
//		{
//			nCount++; pSrc++;
//		}
//
//		if (pSrc == gpSrv->pInputQueueEnd)
//		{
//			pSrc = gpSrv->pInputQueue;
//			pEnd = (gpSrv->pInputQueueRead < gpSrv->pInputQueueWrite) ? gpSrv->pInputQueueWrite : gpSrv->pInputQueueEnd;
//
//			while(pSrc < pEnd)
//			{
//				nCount++; pSrc++;
//			}
//		}
//	}
//
//	return nCount;
//}
//#endif

// Дождаться, пока консольный буфер готов принять события ввода
// Возвращает FALSE, если сервер закрывается!
BOOL WaitConsoleReady(BOOL abReqEmpty)
{
	// Если сейчас идет ресайз - нежелательно помещение в буфер событий
	if (gpSrv->bInSyncResize)
	{
		InputLogger::Log(InputLogger::Event::evt_WaitConSize);
		WaitForSingleObject(gpSrv->hAllowInputEvent, MAX_SYNCSETSIZE_WAIT);
	}

	// если убить ожидание очистки очереди - перестает действовать 'Right selection fix'!

	DWORD nQuitWait = WaitForSingleObject(ghQuitEvent, 0);

	if (nQuitWait == WAIT_OBJECT_0)
		return FALSE;

	if (abReqEmpty)
	{
		InputLogger::Log(InputLogger::Event::evt_WaitConEmpty);

		//#ifdef USE_INPUT_SEMAPHORE
		//// Нет смысла использовать вместе с семафором ввода
		//_ASSERTE(FALSE);
		//#endif

		DWORD nCurInputCount = 0; //, cbWritten = 0;
		//INPUT_RECORD irDummy[2] = {{0},{0}};
		HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE); // тут был ghConIn

		// 27.06.2009 Maks - If input queue is not empty - wait for a while, to avoid conflicts with FAR reading queue
		// 19.02.2010 Maks - замена на GetNumberOfConsoleInputEvents
		//if (PeekConsoleInput(hIn, irDummy, 1, &(nCurInputCount = 0)) && nCurInputCount > 0) {
		if (GetNumberOfConsoleInputEvents(hIn, &(nCurInputCount = 0)) && nCurInputCount > 0)
		{
			DWORD dwStartTick = GetTickCount(), dwDelta, dwTick;

			do
			{
				//Sleep(5);
				nQuitWait = WaitForSingleObject(ghQuitEvent, 5);

				if (nQuitWait == WAIT_OBJECT_0)
					return FALSE;

				//if (!PeekConsoleInput(hIn, irDummy, 1, &(nCurInputCount = 0)))
				if (!GetNumberOfConsoleInputEvents(hIn, &(nCurInputCount = 0)))
					nCurInputCount = 0;

				dwTick = GetTickCount(); dwDelta = dwTick - dwStartTick;
			}
			while ((nCurInputCount > 0) && (dwDelta < MAX_INPUT_QUEUE_EMPTY_WAIT));
		}

		if (WaitForSingleObject(ghQuitEvent, 0) == WAIT_OBJECT_0)
			return FALSE;
	}

	//return (nCurInputCount == 0);
	return TRUE; // Если готов - всегда TRUE
}

BOOL SendConsoleEvent(INPUT_RECORD* pr, UINT nCount)
{
	if (!nCount || !pr)
	{
		_ASSERTE(nCount>0 && pr!=NULL);
		return FALSE;
	}

	BOOL fSuccess = FALSE;
	//// Если сейчас идет ресайз - нежелательно помещение в буфер событий
	//if (gpSrv->bInSyncResize)
	//	WaitForSingleObject(gpSrv->hAllowInputEvent, MAX_SYNCSETSIZE_WAIT);
	//DWORD nCurInputCount = 0, cbWritten = 0;
	//INPUT_RECORD irDummy[2] = {{0},{0}};
	//HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE); // тут был ghConIn
	// 02.04.2010 Maks - перенесено в WaitConsoleReady
	//// 27.06.2009 Maks - If input queue is not empty - wait for a while, to avoid conflicts with FAR reading queue
	//// 19.02.2010 Maks - замена на GetNumberOfConsoleInputEvents
	////if (PeekConsoleInput(hIn, irDummy, 1, &(nCurInputCount = 0)) && nCurInputCount > 0) {
	//if (GetNumberOfConsoleInputEvents(hIn, &(nCurInputCount = 0)) && nCurInputCount > 0) {
	//	DWORD dwStartTick = GetTickCount();
	//	WARNING("Do NOT wait, but place event in Cyclic queue");
	//	do {
	//		Sleep(5);
	//		//if (!PeekConsoleInput(hIn, irDummy, 1, &(nCurInputCount = 0)))
	//		if (!GetNumberOfConsoleInputEvents(hIn, &(nCurInputCount = 0)))
	//			nCurInputCount = 0;
	//	} while ((nCurInputCount > 0) && ((GetTickCount() - dwStartTick) < MAX_INPUT_QUEUE_EMPTY_WAIT));
	//}
	INPUT_RECORD* prNew = NULL;
	int nAllCount = 0;
	BOOL lbReqEmpty = FALSE;

	for (UINT n = 0; n < nCount; n++)
	{
		if (pr[n].EventType != KEY_EVENT)
		{
			nAllCount++;
			if (!lbReqEmpty && (pr[n].EventType == MOUSE_EVENT))
			{
				// По всей видимости дурит консоль Windows.
				// Если в буфере сейчас еще есть мышиные события, то запись
				// в буфер возвращает ОК, но считывающее приложение получает 0-событий.
				// В итоге, получаем пропуск некоторых событий, что очень неприятно 
				// при выделении кучи файлов правой кнопкой мыши (проводкой с зажатой кнопкой)
				if (pr[n].Event.MouseEvent.dwButtonState /*== RIGHTMOST_BUTTON_PRESSED*/)
					lbReqEmpty = TRUE;
			}
		}
		else
		{
			WORD vk = pr[n].Event.KeyEvent.wVirtualKeyCode;
			if (((vk == VK_RETURN) || (vk == VK_SPACE)) && gpSrv->pAppMap)
			{
				// github#19: don't post Enter/Space KeyUp events to the console input buffer
				// Also, type in PS prompt
				// get-help Get-ChildItem -full | out-host -paging
				// And press "Enter". You'll get one waste reaction for "Enter" KeyUp
				static DWORD nLastPID = 0;
				static WORD nLastVK = 0;
				bool bBypass = false;
				if (pr[n].Event.KeyEvent.bKeyDown)
				{
					// The application may use Read only for getting the event when it is ready
					// So the application is not waiting for and simple nReadConsoleInputPID
					// will be zero most of time...
					nLastPID = gpSrv->pAppMap->Ptr()->nLastReadInputPID;
					nLastVK = nLastPID ? vk : 0;
					bBypass = true;
				}
				else
				{
					if ((nLastPID != gpSrv->pAppMap->Ptr()->nLastReadInputPID)
						|| (nLastVK != vk))
					{
						// Skip this event
						pr[n].EventType = 0;
						nLastPID = 0; nLastVK = 0;
						continue;
					}
					else
					{
						bBypass = true;
					}
				}
				// Else - allow Enter/Space KeyUp
				UNREFERENCED_PARAMETER(bBypass);
			}

			if (!pr[n].Event.KeyEvent.wRepeatCount)
			{
				_ASSERTE(pr[n].Event.KeyEvent.wRepeatCount!=0);
				pr[n].Event.KeyEvent.wRepeatCount = 1;
			}

			nAllCount += pr[n].Event.KeyEvent.wRepeatCount;
		}
	}

	if (nAllCount > (int)nCount)
	{
		prNew = (INPUT_RECORD*)malloc(sizeof(INPUT_RECORD)*nAllCount);

		if (prNew)
		{
			INPUT_RECORD* ppr = prNew;
			INPUT_RECORD* pprMod = NULL;

			for (UINT n = 0; n < nCount; n++)
			{
				if (!pr[n].EventType)
					continue;

				*(ppr++) = pr[n];

				if (pr[n].EventType == KEY_EVENT)
				{
					UINT nCurCount = pr[n].Event.KeyEvent.wRepeatCount;

					if (nCurCount > 1)
					{
						pprMod = (ppr-1);
						pprMod->Event.KeyEvent.wRepeatCount = 1;

						for (UINT i = 1; i < nCurCount; i++)
						{
							*(ppr++) = *pprMod;
						}
					}
				}
				else if (!lbReqEmpty && (pr[n].EventType == MOUSE_EVENT))
				{
					// По всей видимости дурит консоль Windows.
					// Если в буфере сейчас еще есть мышиные события, то запись
					// в буфер возвращает ОК, но считывающее приложение получает 0-событий.
					// В итоге, получаем пропуск некоторых событий, что очень неприятно 
					// при выделении кучи файлов правой кнопкой мыши (проводкой с зажатой кнопкой)
					if (pr[n].Event.MouseEvent.dwButtonState == RIGHTMOST_BUTTON_PRESSED)
						lbReqEmpty = TRUE;
				}
			}

			pr = prNew;
			_ASSERTE(nAllCount == (ppr-prNew));
			nCount = (UINT)(ppr-prNew);
		}
	}

	// Если не готов - все равно запишем
	DEBUGTEST(BOOL bConReady = )
	WaitConsoleReady(lbReqEmpty);


	DWORD cbWritten = 0;

#ifdef _DEBUG
	wchar_t* pszDbgCurChars = NULL;
	wchar_t szDbg[255];
	for (UINT i = 0; i < nCount; i++)
	{
		switch (pr[i].EventType)
		{
		case MOUSE_EVENT:
			_wsprintf(szDbg, SKIPLEN(countof(szDbg))
				L"*** ConEmuC.MouseEvent(X=%i,Y=%i,Btns=0x%04x,Moved=%i)\n",
				pr[i].Event.MouseEvent.dwMousePosition.X, pr[i].Event.MouseEvent.dwMousePosition.Y, pr[i].Event.MouseEvent.dwButtonState, (pr[i].Event.MouseEvent.dwEventFlags & MOUSE_MOVED));
			DEBUGSTRINPUTWRITE(szDbg);
			
			#ifdef _DEBUG
			{
				static int LastMsButton;
				if ((LastMsButton & 1) && (pr[i].Event.MouseEvent.dwButtonState == 0))
				{
					// LButton was Down, now - Up
					LastMsButton = pr[i].Event.MouseEvent.dwButtonState;
				}
				else if (!LastMsButton && (pr[i].Event.MouseEvent.dwButtonState & 1))
				{
					// LButton was Up, now - Down
					LastMsButton = pr[i].Event.MouseEvent.dwButtonState;
				}
				else
				{ //-V523
					LastMsButton = pr[i].Event.MouseEvent.dwButtonState;
				}
			}
			#endif
			break;
		case KEY_EVENT:
			{
				wchar_t szCh[3] = {pr[i].Event.KeyEvent.uChar.UnicodeChar};
				switch (szCh[0])
				{
				case 8:  szCh[0] = L'\\'; szCh[1] = L'b'; break;
				case 9:  szCh[0] = L'\\'; szCh[1] = L't'; break;
				case 10: szCh[0] = L'\\'; szCh[1] = L'r'; break;
				case 13: szCh[0] = L'\\'; szCh[1] = L'n'; break;
				case 27: szCh[0] = L'\\'; szCh[1] = L'e'; break;
				}
				_wsprintf(szDbg, SKIPLEN(countof(szDbg))
					L"*** ConEmuC.KeybdEvent(%s, VK=%u, CH=%s)\n",
					pr[i].Event.KeyEvent.bKeyDown ? L"Dn" : L"Up", pr[i].Event.KeyEvent.wVirtualKeyCode, szCh);
				DEBUGSTRINPUTWRITE(szDbg);
			}
			break;
		}

		// Only for input_bug search purposes in Debug builds
		LONG idx = (InterlockedIncrement(&gn_LogWrittenChars) & (gn_LogWrittenCharsMax-1))*2;
		if (!pszDbgCurChars) pszDbgCurChars = gs_LogWrittenChars+idx;
		if (pr[i].EventType == KEY_EVENT)
		{
			gs_LogWrittenChars[idx++] = pr[i].Event.KeyEvent.bKeyDown ? L'\\' : L'/';
			gs_LogWrittenChars[idx] = pr[i].Event.KeyEvent.uChar.UnicodeChar ? pr[i].Event.KeyEvent.uChar.UnicodeChar : L'.';
		}
		else
		{
			gs_LogWrittenChars[idx++] = L'=';
			switch (pr[i].EventType)
			{
			case MOUSE_EVENT:
				gs_LogWrittenChars[idx] = L'm'; break;
			case WINDOW_BUFFER_SIZE_EVENT:
				gs_LogWrittenChars[idx] = L'w'; break;
			case MENU_EVENT:
				gs_LogWrittenChars[idx] = L'e'; break;
			case FOCUS_EVENT:
				gs_LogWrittenChars[idx] = L'f'; break;
			default:
				gs_LogWrittenChars[idx] = L'x'; break;
			}
		}
		gs_LogWrittenChars[++idx] = 0;
	}
	int nDbgSendLen = pszDbgCurChars ? lstrlen(pszDbgCurChars) : -1;
	SetLastError(0);
#endif

	InputLogger::Log(InputLogger::Event::evt_WriteConInput, nCount);

	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE); // тут был ghConIn
	// Strange VIM reaction on xterm-keypresses
	if ((nCount > 2) && (nCount <= 32) && (pr->EventType == KEY_EVENT) && (pr->Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE))
	{
		DWORD nWritten = 0; cbWritten = 0;
		for (UINT n = 0; n < nCount; n++)
		{
			if ((n + 1) == nCount)
			{
				DEBUGTEST(bConReady = ) WaitConsoleReady(TRUE);
			}
			fSuccess = WriteConsoleInput(hIn, pr+n, 1, &nWritten);
			if (fSuccess) cbWritten += nWritten;
		}
	}
	else
	{
		fSuccess = WriteConsoleInput(hIn, pr, nCount, &cbWritten);
	}

	// Error ERROR_INVALID_HANDLE may occur when ConEmu was Attached to some external console with redirected input.

#ifdef _DEBUG
	DWORD dwErr = GetLastError();
	if (!fSuccess || (nCount != cbWritten))
	{
		_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"### WriteConsoleInput(Write=%i, Written=%i, Left=%i, Err=x%X)\n", nCount, cbWritten, gpSrv->InputQueue.GetNumberOfBufferEvents(), dwErr);
		DEBUGSTRINPUTWRITEFAIL(szDbg);
	}
	else
	{
		_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"*** WriteConsoleInput(Write=%i, Written=%i, Left=%i)\n", nCount, cbWritten, gpSrv->InputQueue.GetNumberOfBufferEvents());
		DEBUGSTRINPUTWRITEALL(szDbg);
	}
	// Some Ctrl+<key> are eaten by console input buffer. ConsoleMode==0xA7 (cmd.exe)
	bool bEaten = fSuccess && nCount==1 && !cbWritten && pr[0].EventType==KEY_EVENT
		&& pr[0].Event.KeyEvent.bKeyDown
		&& ((pr[0].Event.KeyEvent.dwControlKeyState & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED|LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED|SHIFT_PRESSED))
			== (pr[0].Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)));
	_ASSERTE((fSuccess && (cbWritten==nCount || bEaten)) || (!fSuccess && dwErr==ERROR_INVALID_HANDLE && gbAttachMode));
#endif

	if (prNew) free(prNew);

	return fSuccess;
}

DWORD WINAPI InputThread(LPVOID lpvParam)
{
	HANDLE hEvents[2] = {ghQuitEvent, gpSrv->hInputEvent};
	DWORD dwWait = 0;
	INPUT_RECORD ir[100];

	while ((dwWait = WaitForMultipleObjects(2, hEvents, FALSE, INPUT_QUEUE_TIMEOUT)) != WAIT_OBJECT_0)
	{
		if (gpSrv->InputQueue.IsInputQueueEmpty())
			continue;

		// -- перенесено в SendConsoleEvent
		//// Если не готов - все равно запишем
		//if (!WaitConsoleReady())
		//	break;

		// Читаем и пишем
		DWORD nInputCount = sizeof(ir)/sizeof(ir[0]);

		//#ifdef USE_INPUT_SEMAPHORE
		//DWORD nSemaphore = ghConInSemaphore ? WaitForSingleObject(ghConInSemaphore, INSEMTIMEOUT_WRITE) : 1;
		//_ASSERTE(ghConInSemaphore && (nSemaphore == WAIT_OBJECT_0));
		//#endif

		InputLogger::Log(InputLogger::Event::evt_ReadInputQueue);

		if (gpSrv->InputQueue.ReadInputQueue(ir, &nInputCount))
		{
			_ASSERTE(nInputCount>0);

			// Выставить флаг, что прошло очередное чтение
			InputLogger::Log(InputLogger::Event::evt_SetEvent, nInputCount);
			SetEvent(gpSrv->hInputWasRead);

			#ifdef _DEBUG
			for (DWORD j = 0; j < nInputCount; j++)
			{
				if (ir[j].EventType == KEY_EVENT
					&& (ir[j].Event.KeyEvent.wVirtualKeyCode == 'C' || ir[j].Event.KeyEvent.wVirtualKeyCode == VK_CANCEL)
					&& (      // Удерживается ТОЛЬКО Ctrl
					(ir[j].Event.KeyEvent.dwControlKeyState & CTRL_MODIFIERS) &&
					((ir[j].Event.KeyEvent.dwControlKeyState & ALL_MODIFIERS)
					== (ir[j].Event.KeyEvent.dwControlKeyState & CTRL_MODIFIERS)))
					)
				{
					DEBUGSTR(L"  ---  CtrlC/CtrlBreak recieved\n");
				}
			}
			#endif

			
			// Write

			InputLogger::Log(InputLogger::Event::evt_SendStart, nInputCount);
			
			//DEBUGSTRINPUTPIPE(L"SendConsoleEvent\n");
			SendConsoleEvent(ir, nInputCount);

			InputLogger::Log(InputLogger::Event::evt_SendEnd, nInputCount);
		}

		//#ifdef USE_INPUT_SEMAPHORE
		//if ((nSemaphore == WAIT_OBJECT_0) && ghConInSemaphore) ReleaseSemaphore(ghConInSemaphore, 1, NULL);
		//#endif

		// Если во время записи в консоль в буфере еще что-то появилось - передернем
		if (!gpSrv->InputQueue.IsInputQueueEmpty())
			SetEvent(gpSrv->hInputEvent);
	}

	return 1;
}

//DWORD WINAPI InputPipeThread(LPVOID lpvParam)
//{
//	BOOL fConnected, fSuccess;
//	//DWORD nCurInputCount = 0;
//	//DWORD gpSrv->dwServerThreadId;
//	//HANDLE hPipe = NULL;
//	DWORD dwErr = 0;
//
//	// The main loop creates an instance of the named pipe and
//	// then waits for a client to connect to it. When the client
//	// connects, a thread is created to handle communications
//	// with that client, and the loop is repeated.
//
//	while(!gbQuit)
//	{
//		MCHKHEAP;
//		gpSrv->hInputPipe = Create NamedPipe(
//			gpSrv->szInputname,       // pipe name
//			PIPE_ACCESS_INBOUND,      // goes from client to server only
//			CE_PIPE_TYPE |            // message type pipe
//			CE_PIPE_READMODE |        // message-read mode
//			PIPE_WAIT,                // blocking mode
//			PIPE_UNLIMITED_INSTANCES, // max. instances
//			PIPEBUFSIZE,              // output buffer size
//			PIPEBUFSIZE,              // input buffer size
//			0,                        // client time-out
//			gpLocalSecurity);         // default security attribute
//
//		if (gpSrv->hInputPipe == INVALID_HANDLE_VALUE)
//		{
//			dwErr = GetLastError();
//			_ASSERTE(gpSrv->hInputPipe != INVALID_HANDLE_VALUE);
//			_printf("CreatePipe failed, ErrCode=0x%08X\n", dwErr);
//			Sleep(50);
//			//return 99;
//			continue;
//		}
//
//		// Wait for the client to connect; if it succeeds,
//		// the function returns a nonzero value. If the function
//		// returns zero, GetLastError returns ERROR_PIPE_CONNECTED.
//		fConnected = ConnectNamedPipe(gpSrv->hInputPipe, NULL) ?
//TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
//		MCHKHEAP;
//
//		if (fConnected)
//		{
//			//TODO:
//			DWORD cbBytesRead; //, cbWritten;
//			MSG64 imsg = {};
//
//			while(!gbQuit && (fSuccess = ReadFile(
//				gpSrv->hInputPipe,        // handle to pipe
//				&imsg,        // buffer to receive data
//				sizeof(imsg), // size of buffer
//				&cbBytesRead, // number of bytes read
//				NULL)) != FALSE)        // not overlapped I/O
//			{
//				// предусмотреть возможность завершения нити
//				if (gbQuit)
//					break;
//
//				MCHKHEAP;
//
//				if (imsg.message)
//				{
//#ifdef _DEBUG
//
//					switch(imsg.message)
//					{
//					case WM_KEYDOWN: case WM_SYSKEYDOWN: DEBUGSTRINPUTPIPE(L"ConEmuC: Recieved key down\n"); break;
//					case WM_KEYUP: case WM_SYSKEYUP: DEBUGSTRINPUTPIPE(L"ConEmuC: Recieved key up\n"); break;
//					default: DEBUGSTRINPUTPIPE(L"ConEmuC: Recieved input\n");
//					}
//
//#endif
//					INPUT_RECORD r;
//
//					// Некорректные события - отсеиваются,
//					// некоторые события (CtrlC/CtrlBreak) не пишутся в буферном режиме
//					if (ProcessInputMessage(imsg, r))
//					{
//						//SendConsoleEvent(&r, 1);
//						if (!WriteInputQueue(&r))
//						{
//							_ASSERTE(FALSE);
//							WARNING("Если буфер переполнен - ждать? Хотя если будем ждать здесь - может повиснуть GUI на записи в pipe...");
//						}
//					}
//
//					MCHKHEAP;
//				}
//
//				// next
//				memset(&imsg,0,sizeof(imsg));
//				MCHKHEAP;
//			}
//
//			SafeCloseHandle(gpSrv->hInputPipe);
//		}
//		else
//			// The client could not connect, so close the pipe.
//			SafeCloseHandle(gpSrv->hInputPipe);
//	}
//
//	MCHKHEAP;
//	return 1;
//}
