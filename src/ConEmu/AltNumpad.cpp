
/*
Copyright (c) 2015 Maximus5
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

#include "Header.h"
#include "AltNumpad.h"
#include "ConEmu.h"
#include "RealConsole.h"
#include "VConRelease.h"
#include "VirtualConsole.h"

#define DEBUGSTRCHARS(s) DEBUGSTR(s)

CAltNumpad::CAltNumpad(CConEmuMain* apConEmu)
	: mp_ConEmu(apConEmu)
	, mb_WaitingForAltChar(false)
	, mb_InAltNumber(false)
	, mn_AltNumber(0)
{
}

CAltNumpad::~CAltNumpad()
{
}

bool CAltNumpad::isAltNumpad()
{
	SHORT sNumLock = GetKeyState(VK_NUMLOCK);
	if (!(sNumLock & 1))
		return false;
	if (!isPressed(VK_LMENU))
		return false;
	if (isPressed(VK_RMENU)
		|| isPressed(VK_SHIFT) || isPressed(VK_CONTROL)
		|| isPressed(VK_LWIN) || isPressed(VK_RWIN))
		return false;
	return true;
}

bool CAltNumpad::OnKeyboard(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	if (mb_WaitingForAltChar && (messg == WM_CHAR))
	{
		mb_WaitingForAltChar = false;
		wchar_t wszChars[2] = {LOWORD(wParam), 0};
		wchar_t szLog[80];
		_wsprintf(szLog, SKIPCOUNT(szLog) L"Received by AltNumber: char=%s (%u) (internal=x%X)", wszChars, LODWORD(wParam), LODWORD(mn_AltNumber));
		LogString(szLog);
		DumpChars(wszChars);
		return true;
	}

	//messg == WM_SYSKEYDOWN(UP), wParam == VK_NUMPAD0..VK_NUMPAD9, lParam == (ex. 0x20490001)

	if (!mb_InAltNumber)
	{
		// The context code. The value is 1 if the ALT key is held down while the key is pressed
		if ((messg != WM_SYSKEYDOWN) || !(lParam & (1 << 29)))
			return false;
		// No sense to start checks if the key is not keypad number
		if (!(wParam >= VK_NUMPAD0 && wParam <= VK_NUMPAD9))
			return false;
		// No other modifiers are down
		if (!isAltNumpad())
			return false;
		mn_AltNumber = (wParam - VK_NUMPAD0);
		mb_InAltNumber = true;
		return true; // Don't pass to console
	}
	else if ((messg == WM_KEYUP) && (wParam == VK_MENU || wParam == VK_LMENU/*JIC*/))
	{
		//_ASSERTE(mb_InAltNumber && mn_AltNumber);
		mb_WaitingForAltChar = true;
		DumpAltNumber();
	}
	else if (messg == WM_SYSKEYDOWN)
	{
		if ((wParam >= VK_NUMPAD0) && (wParam <= VK_NUMPAD9))
		{
			mn_AltNumber = mn_AltNumber * 10 + (wParam - VK_NUMPAD0);
			return true; // Don't pass to console
		}
		else
		{
			DEBUGSTRCHARS(L"Have to dump catched codebase?");
			mn_AltNumber = 0;
		}
	}

	return false;
}

void CAltNumpad::DumpAltNumber()
{
	if (!mb_InAltNumber)
		return;

	// Reset it immediately
	mb_InAltNumber = false;
	if (!mn_AltNumber)
		return;

	wchar_t wszChars[8] = L"";
	char szChars[8] = "";

	// Let use OEM codepage for beginning?
	szChars[0] = LOBYTE(mn_AltNumber);
	MultiByteToWideChar(CP_OEMCP, 0, szChars, -1, wszChars, countof(wszChars));

	wchar_t szLog[80];
	_wsprintf(szLog, SKIPCOUNT(szLog) L"Gathered by AltNumber: char=%s (%u) (codepoint=x%X)", wszChars, (UINT)wszChars[0], LODWORD(mn_AltNumber));
	//DumpChars(wszChars);
	LogString(szLog);
}

void CAltNumpad::DumpChars(wchar_t* asChars)
{
	CVConGuard VCon;
	if (mp_ConEmu->GetActiveVCon(&VCon) < 0)
		return;
	VCon->RCon()->PostString(asChars, wcslen(asChars));
}
