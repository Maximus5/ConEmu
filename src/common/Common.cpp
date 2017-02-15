
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
#include "Common.h"



void STRPTR2::Set(wchar_t* RVAL_REF ptrSrc, int cch /*= -1*/)
{
	_ASSERTE(!cchCount && !bMangled && !psz);
	if (cch < 0)
		cch = (lstrlen(ptrSrc)+1);
	cchCount = cch;
	psz = ptrSrc;
}

LPBYTE STRPTR2::Mangle(LPBYTE ptrDst)
{
	_ASSERTE(!bMangled);
	if (!bMangled)
	{
		_ASSERTE(!cchCount || psz);

		if (cchCount && psz)
		{
			wchar_t* ptrSrc = psz;
			offset = ptrDst - (LPBYTE)this;
			// ptrDst is expected to be inside memory block allocated for CESERVER_REQ
			_ASSERTE((((INT_PTR)offset) > 0) && (offset < 0x100000));
			// Copy our string to testination
			memmove(ptrDst, ptrSrc, cchCount*sizeof(ptrSrc[0]));
			ptrDst += cchCount*sizeof(ptrSrc[0]);
			// Release previously allocated pointer
			SafeFree(ptrSrc);
		}
		else
		{
			cchCount = 0;
		}

		bMangled = TRUE;
	}
	return ptrDst;
}

LPWSTR STRPTR2::Demangle()
{
	if (!cchCount)
		return NULL;
	if (!bMangled)
		return psz;
	_ASSERTE(offset >= sizeof(STRPTR2));
	psz = (wchar_t*)(((LPBYTE)this) + offset);
	return psz;
}

STRPTR2::operator LPCWSTR()
{
	return Demangle();
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
		_ASSERTE(piRec->Event.KeyEvent.wRepeatCount<=255); // can't pack more repeats
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

void TranslateKeyPress(WORD vkKey, DWORD dwControlState, wchar_t wch, int ScanCode, INPUT_RECORD* rDown, INPUT_RECORD* rUp)
{
	// Может приходить запрос на отсылку даже если текущий буфер НЕ rbt_Primary,
	// например, при начале выделения и автоматическом переключении на альтернативный буфер

	if (!vkKey && !dwControlState && wch)
	{
		USHORT vk = VkKeyScan(wch);
		if (vk && (vk != 0xFFFF))
		{
			vkKey = (vk & 0xFF);
			vk = vk >> 8;
			if ((vk & 7) == 6)
			{
				// For keyboard layouts that use the right-hand ALT key as a shift
				// key (for example, the French keyboard layout), the shift state is
				// represented by the value 6, because the right-hand ALT key is
				// converted internally into CTRL+ALT.
				dwControlState |= SHIFT_PRESSED;
			}
			else
			{
				if (vk & 1)
					dwControlState |= SHIFT_PRESSED;
				if (vk & 2)
					dwControlState |= LEFT_CTRL_PRESSED;
				if (vk & 4)
					dwControlState |= ((vk & 6) == 6) ? RIGHT_ALT_PRESSED : LEFT_ALT_PRESSED;
			}
		}
	}

	if (ScanCode == -1)
		ScanCode = MapVirtualKey(vkKey, 0/*MAPVK_VK_TO_VSC*/);

	INPUT_RECORD r = {KEY_EVENT};
	r.Event.KeyEvent.bKeyDown = TRUE;
	r.Event.KeyEvent.wRepeatCount = 1;
	r.Event.KeyEvent.wVirtualKeyCode = vkKey;
	r.Event.KeyEvent.wVirtualScanCode = ScanCode;
	r.Event.KeyEvent.uChar.UnicodeChar = wch;
	r.Event.KeyEvent.dwControlKeyState = dwControlState
		| ((vkKey == VK_SHIFT || vkKey == VK_LSHIFT || vkKey == VK_RSHIFT) ? SHIFT_PRESSED : 0)
		| ((vkKey == VK_CONTROL || vkKey == VK_LCONTROL) ? LEFT_CTRL_PRESSED : 0)
		| ((vkKey == VK_RCONTROL) ? RIGHT_CTRL_PRESSED : 0)
		| ((vkKey == VK_RMENU) ? RIGHT_ALT_PRESSED : 0)
		;
	*rDown = r;

	TODO("Может нужно в dwControlKeyState применять модификатор, если он и есть vkKey?");

	r.Event.KeyEvent.bKeyDown = FALSE;
	r.Event.KeyEvent.dwControlKeyState = dwControlState;
	*rUp = r;
}


/* *** struct CESERVER_REQ_NEWCMD -- CECMD_NEWCMD *** */
wchar_t* CESERVER_REQ_NEWCMD::GetCommand()
{
	static wchar_t szNil[1] = L"";
	return (this && cchCommand) ? (wchar_t*)&ptrDataStart : szNil;
}

wchar_t* CESERVER_REQ_NEWCMD::GetEnvStrings()
{
	return (this && cchEnvStrings) ? ((wchar_t*)&ptrDataStart)+cchCommand : NULL;
}

void CESERVER_REQ_NEWCMD::SetCommand(LPCWSTR asCommand)
{
	if (!this) return;
	int iLen = asCommand ? lstrlen(asCommand) : 0;
	if (iLen > 0)
	{
		cchCommand = iLen+1;
		memmove(GetCommand(), asCommand, cchCommand*sizeof(*asCommand));
	}
	else
	{
		cchCommand = 0;
	}
}

void CESERVER_REQ_NEWCMD::SetEnvStrings(LPCWSTR asStrings, DWORD cchLenZZ)
{
	if (!this) return;
	if (cchLenZZ && asStrings)
	{
		cchEnvStrings = cchLenZZ;
		// Acquire shifted pointer to dest storage
		wchar_t* ptrString = this->GetEnvStrings();
		if (ptrString)
		{
			wchar_t* ptr = ptrString;
			LPCWSTR pszSrc = asStrings;
			// ZZ-terminated pairs
			while (*pszSrc)
			{
				// Pairs "Name=Value\0"
				LPCWSTR pszName = pszSrc;
				LPCWSTR pszNext = pszName + lstrlen(pszName) + 1;
				// Next pair
				pszSrc = pszNext;

				LPCWSTR pszEqnSign = NULL;
				if (!IsEnvBlockVariableValid(pszName, pszEqnSign))
					continue;
				_ASSERTE(pszEqnSign!=NULL);

				size_t cchCur = pszNext - pszName;
				memmove(ptr, pszName, cchCur*sizeof(*pszName));
				ptr[(pszEqnSign-pszName)] = 0; // Make our storage "Name\0Value\0" like
				ptr += cchCur;
			}
			// Copied size?
			size_t cchAll = (ptr - ptrString);
			_ASSERTE(cchAll < cchLenZZ);
			if (cchAll > 0)
			{
				cchEnvStrings = (cchAll + 1); // ZZ-terminated
			}
			else
			{
				_ASSERTE(FALSE && "Nothing was copied, strange, env.block is empty?");
				cchEnvStrings = 0;
			}
		}
	}
	else
	{
		cchEnvStrings = 0;
	}
}
/* *** struct CESERVER_REQ_NEWCMD -- CECMD_NEWCMD *** */

bool IsEnvBlockVariableValid(LPCWSTR asName, LPCWSTR& rszEqnSign)
{
	if (!asName || !*asName)
		return false;

	// Skip lines like "=C:=C:\\" and other invalids
	rszEqnSign = wcschr(asName, L'=');
	if (!rszEqnSign || rszEqnSign == asName)
		return false;

	return IsEnvBlockVariableValid(asName);
}

bool IsEnvBlockVariableValid(LPCWSTR asName)
{
	if (!asName || !*asName)
		return false;

	// Skip all our internal vars, they must be set from prompt specially
	// But let user a chance with different case. E.g. "set conemuhk=OFF"
	// So, use case-sensitive comparison
	if (wcsncmp(asName, L"ConEmu", 6) == 0)
		return false;

	return true;
}

//#ifdef CONEMU_MINIMAL
//#include "base64.h"
//#endif


BOOL gbInCommonShutdown = FALSE;
#ifdef _DEBUG
extern HANDLE ghInMyAssertTrap;
extern DWORD gnInMyAssertThread;
#endif


void CommonShutdown()
{
	gbInCommonShutdown = TRUE;

	ShutdownSecurity();

#ifdef _DEBUG
	MyAssertShutdown();
#endif
}
