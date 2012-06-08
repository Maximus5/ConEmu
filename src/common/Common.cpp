
/*
Copyright (c) 2009-2012 Maximus5
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
#include <windows.h>
#include "common.hpp"


// Возвращает 0, если успешно, иначе - ошибка
int NextArg(const wchar_t** asCmdLine, wchar_t (&rsArg)[MAX_PATH+1], const wchar_t** rsArgStart/*=NULL*/)
{
	LPCWSTR psCmdLine = *asCmdLine, pch = NULL;
	wchar_t ch = *psCmdLine;
	size_t nArgLen = 0;
	bool lbQMode = false;

	while (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n') ch = *(++psCmdLine);

	if (ch == 0) return CERR_CMDLINEEMPTY;

	// аргумент начинается с "
	if (ch == L'"')
	{
		lbQMode = true;
		psCmdLine++;
		pch = wcschr(psCmdLine, L'"');

		if (!pch) return CERR_CMDLINE;

		while (pch[1] == L'"')
		{
			pch += 2;
			pch = wcschr(pch, L'"');

			if (!pch) return CERR_CMDLINE;
		}

		// Теперь в pch ссылка на последнюю "
	}
	else
	{
		// До конца строки или до первого пробела
		//pch = wcschr(psCmdLine, L' ');
		// 09.06.2009 Maks - обломался на: cmd /c" echo Y "
		pch = psCmdLine;

		// Ищем обычным образом (до пробела/кавычки)
		while (*pch && *pch!=L' ' && *pch!=L'"') pch++;

		//if (!pch) pch = psCmdLine + lstrlenW(psCmdLine); // до конца строки
	}

	nArgLen = pch - psCmdLine;

	if (nArgLen > MAX_PATH) return CERR_CMDLINE;

	// Вернуть аргумент
	memcpy(rsArg, psCmdLine, nArgLen*sizeof(*rsArg));

	if (rsArgStart) *rsArgStart = psCmdLine;

	rsArg[nArgLen] = 0;
	psCmdLine = pch;
	// Finalize
	ch = *psCmdLine; // может указывать на закрывающую кавычку

	if (lbQMode && ch == L'"') ch = *(++psCmdLine);

	while (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n') ch = *(++psCmdLine);

	*asCmdLine = psCmdLine;
	return 0;
}

int NextArg(const char** asCmdLine, char (&rsArg)[MAX_PATH+1], const char** rsArgStart/*=NULL*/)
{
	LPCSTR psCmdLine = *asCmdLine, pch = NULL;
	char ch = *psCmdLine;
	size_t nArgLen = 0;
	bool lbQMode = false;

	while (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') ch = *(++psCmdLine);

	if (ch == 0) return CERR_CMDLINEEMPTY;

	// аргумент начинается с "
	if (ch == '"')
	{
		lbQMode = true;
		psCmdLine++;
		pch = strchr(psCmdLine, '"');

		if (!pch) return CERR_CMDLINE;

		while (pch[1] == '"')
		{
			pch += 2;
			pch = strchr(pch, '"');

			if (!pch) return CERR_CMDLINE;
		}

		// Теперь в pch ссылка на последнюю "
	}
	else
	{
		// До конца строки или до первого пробела
		//pch = wcschr(psCmdLine, ' ');
		// 09.06.2009 Maks - обломался на: cmd /c" echo Y "
		pch = psCmdLine;

		// Ищем обычным образом (до пробела/кавычки)
		while (*pch && *pch!=' ' && *pch!='"') pch++;

		//if (!pch) pch = psCmdLine + lstrlenW(psCmdLine); // до конца строки
	}

	nArgLen = pch - psCmdLine;

	if (nArgLen > MAX_PATH) return CERR_CMDLINE;

	// Вернуть аргумент
	memcpy(rsArg, psCmdLine, nArgLen*sizeof(*rsArg));

	if (rsArgStart) *rsArgStart = psCmdLine;

	rsArg[nArgLen] = 0;
	psCmdLine = pch;
	// Finalize
	ch = *psCmdLine; // может указывать на закрывающую кавычку

	if (lbQMode && ch == '"') ch = *(++psCmdLine);

	while(ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') ch = *(++psCmdLine);

	*asCmdLine = psCmdLine;
	return 0;
}

BOOL PackInputRecord(const INPUT_RECORD* piRec, MSG64::MsgStr* pMsg)
{
	if (!pMsg || !piRec)
	{
		_ASSERTE(pMsg!=NULL && piRec!=NULL);
		return FALSE;
	}

	memset(pMsg, 0, sizeof(*pMsg));
	//pMsg->cbSize = sizeof(*pMsg);

	UINT nMsg = 0; WPARAM wParam = 0; LPARAM lParam = 0;

	if (piRec->EventType == KEY_EVENT)
	{
		nMsg = piRec->Event.KeyEvent.bKeyDown ? WM_KEYDOWN : WM_KEYUP;
		lParam |= (DWORD_PTR)(WORD)piRec->Event.KeyEvent.uChar.UnicodeChar;
		lParam |= ((DWORD_PTR)piRec->Event.KeyEvent.wVirtualKeyCode & 0xFF) << 16;
		lParam |= ((DWORD_PTR)piRec->Event.KeyEvent.wVirtualScanCode & 0xFF) << 24;
		wParam |= (DWORD_PTR)piRec->Event.KeyEvent.dwControlKeyState & 0xFFFF;
		wParam |= ((DWORD_PTR)piRec->Event.KeyEvent.wRepeatCount & 0xFF) << 16;
	}
	else if (piRec->EventType == MOUSE_EVENT)
	{
		switch(piRec->Event.MouseEvent.dwEventFlags)
		{
			case MOUSE_MOVED:
				nMsg = MOUSE_EVENT_MOVE;
				break;
			case 0:
				nMsg = MOUSE_EVENT_CLICK;
				break;
			case DOUBLE_CLICK:
				nMsg = MOUSE_EVENT_DBLCLICK;
				break;
			case MOUSE_WHEELED:
				nMsg = MOUSE_EVENT_WHEELED;
				break;
			case /*MOUSE_HWHEELED*/ 0x0008:
				nMsg = MOUSE_EVENT_HWHEELED;
				break;
			default:
				_ASSERT(FALSE);
		}

		lParam = ((DWORD_PTR)(WORD)piRec->Event.MouseEvent.dwMousePosition.X)
		         | (((DWORD_PTR)(WORD)piRec->Event.MouseEvent.dwMousePosition.Y) << 16);
		// max 0x0010/*FROM_LEFT_4ND_BUTTON_PRESSED*/
		wParam |= ((DWORD_PTR)piRec->Event.MouseEvent.dwButtonState) & 0xFF;
		// max - ENHANCED_KEY == 0x0100
		wParam |= (((DWORD_PTR)piRec->Event.MouseEvent.dwControlKeyState) & 0xFFFF) << 8;

		if (nMsg == MOUSE_EVENT_WHEELED || nMsg == MOUSE_EVENT_HWHEELED)
		{
			// HIWORD() - short (direction[1/-1])*count*120
			short nWheel = (short)((((DWORD)piRec->Event.MouseEvent.dwButtonState) & 0xFFFF0000) >> 16);
			char  nCount = nWheel / 120;
			wParam |= ((DWORD_PTR)(BYTE)nCount) << 24;
		}
	}
	else if (piRec->EventType == FOCUS_EVENT)
	{
		nMsg = piRec->Event.FocusEvent.bSetFocus ? WM_SETFOCUS : WM_KILLFOCUS;
	}
	else
	{
		_ASSERT(FALSE);
		return FALSE;
	}

	_ASSERTE(nMsg!=0);
	pMsg->message = nMsg;
	pMsg->wParam = wParam;
	pMsg->lParam = lParam;
	return TRUE;
}

BOOL UnpackInputRecord(const MSG64::MsgStr* piMsg, INPUT_RECORD* pRec)
{
	if (!piMsg || !pRec)
	{
		_ASSERTE(piMsg!=NULL && pRec!=NULL);
		return FALSE;
	}

	memset(pRec, 0, sizeof(INPUT_RECORD));

	if (piMsg->message == 0)
		return FALSE;

	if (piMsg->message == WM_KEYDOWN || piMsg->message == WM_KEYUP)
	{
		pRec->EventType = KEY_EVENT;
		// lParam
		pRec->Event.KeyEvent.bKeyDown = (piMsg->message == WM_KEYDOWN);
		pRec->Event.KeyEvent.uChar.UnicodeChar = (WCHAR)(piMsg->lParam & 0xFFFF);
		pRec->Event.KeyEvent.wVirtualKeyCode   = (((DWORD)piMsg->lParam) & 0xFF0000) >> 16;
		pRec->Event.KeyEvent.wVirtualScanCode  = (((DWORD)piMsg->lParam) & 0xFF000000) >> 24;
		// wParam. Пока что тут может быть max(ENHANCED_KEY==0x0100)
		pRec->Event.KeyEvent.dwControlKeyState = ((DWORD)piMsg->wParam & 0xFFFF);
		pRec->Event.KeyEvent.wRepeatCount = ((DWORD)piMsg->wParam & 0xFF0000) >> 16;
	}
	else if (piMsg->message >= MOUSE_EVENT_FIRST && piMsg->message <= MOUSE_EVENT_LAST)
	{
		pRec->EventType = MOUSE_EVENT;

		switch(piMsg->message)
		{
			case MOUSE_EVENT_MOVE:
				pRec->Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
				break;
			case MOUSE_EVENT_CLICK:
				pRec->Event.MouseEvent.dwEventFlags = 0;
				break;
			case MOUSE_EVENT_DBLCLICK:
				pRec->Event.MouseEvent.dwEventFlags = DOUBLE_CLICK;
				break;
			case MOUSE_EVENT_WHEELED:
				pRec->Event.MouseEvent.dwEventFlags = MOUSE_WHEELED;
				break;
			case MOUSE_EVENT_HWHEELED:
				pRec->Event.MouseEvent.dwEventFlags = /*MOUSE_HWHEELED*/ 0x0008;
				break;
		}

		pRec->Event.MouseEvent.dwMousePosition.X = LOWORD(piMsg->lParam);
		pRec->Event.MouseEvent.dwMousePosition.Y = HIWORD(piMsg->lParam);
		// max 0x0010/*FROM_LEFT_4ND_BUTTON_PRESSED*/
		pRec->Event.MouseEvent.dwButtonState = ((DWORD)piMsg->wParam) & 0xFF;
		// max - ENHANCED_KEY == 0x0100
		pRec->Event.MouseEvent.dwControlKeyState = (((DWORD)piMsg->wParam) & 0xFFFF00) >> 8;

		if (piMsg->message == MOUSE_EVENT_WHEELED || piMsg->message == MOUSE_EVENT_HWHEELED)
		{
			// HIWORD() - short (direction[1/-1])*count*120
			signed char nDir = (signed char)((((DWORD)piMsg->wParam) & 0xFF000000) >> 24);
			WORD wDir = nDir*120;
			pRec->Event.MouseEvent.dwButtonState |= wDir << 16;
		}
	}
	else if (piMsg->message == WM_SETFOCUS || piMsg->message == WM_KILLFOCUS)
	{
		pRec->EventType = FOCUS_EVENT;
		pRec->Event.FocusEvent.bSetFocus = (piMsg->message == WM_SETFOCUS);
	}
	else
	{
		return FALSE;
	}

	return TRUE;
}

//#ifdef CONEMU_MINIMAL
//#include "base64.h"
//#endif


BOOL gbInCommonShutdown = FALSE;
//HANDLE ghHookMutex = NULL;
//MSection* gpHookCS = NULL;
#ifdef _DEBUG
extern HANDLE ghInMyAssertTrap;
extern DWORD gnInMyAssertThread;
#endif

ShutdownConsole_t OnShutdownConsole = NULL;

void CommonShutdown()
{
	gbInCommonShutdown = TRUE;

	ShutdownSecurity();

	// Clean memory
	if (OnShutdownConsole)
		OnShutdownConsole();

	//if (ghHookMutex)
	//{
	//	CloseHandle(ghHookMutex);
	//	ghHookMutex = NULL;
	//}
	//if (gpHookCS)
	//{
	//	MSection *p = gpHookCS;
	//	gpHookCS = NULL;
	//	delete p;
	//}

#ifdef _DEBUG
	MyAssertShutdown();
#endif
}
