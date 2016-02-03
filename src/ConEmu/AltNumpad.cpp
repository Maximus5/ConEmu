
/*
Copyright (c) 2015-2016 Maximus5
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
#include "../common/wcchars.h"
#include "AltNumpad.h"
#include "ConEmu.h"
#include "RealConsole.h"
#include "VConRelease.h"
#include "VirtualConsole.h"

#define DEBUGSTRCHARS(s) DEBUGSTR(s)

CAltNumpad::CAltNumpad(CConEmuMain* apConEmu)
	: mp_ConEmu(apConEmu)
	, m_WaitingForAltChar(eAltCharNone)
	, mb_InAltNumber(false)
	, mn_AltNumber(0)
	, mn_NumberBase(10)
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
	if (m_WaitingForAltChar && (messg == WM_CHAR))
	{
		AltCharAction action = m_WaitingForAltChar;
		m_WaitingForAltChar = eAltCharNone;

		// Received from system 16-bit codebase
		// It is not expected (yet) to process 32-bit codebases
		wchar_t wc = (wchar_t)LOWORD(wParam);
		_ASSERTE((WPARAM)wc == wParam);

		// 32-bit codebase
		// UCS defines 16 ‘planes’, so maximum codebase is 0x10FFFF, but
		// we need to use wider mask to include all codebases in range.
		ucs32 wc32 = (ucs32)(DWORD)(mn_AltNumber & 0x1FFFFF);

		wchar_t wszChars[3] = {wc, 0};

		#if 0
		// If hexadecimal mode was selected, process codebase internally
		if (mn_NumberBase == 16)
		{
			// Drop invalid codebases
			if (wc32 > 0x10FFFF)
			{
				wszChars[0] = 0;
			}
			// To avoid ambiguous surrogates processing, convert
			// codebases which get into high-surrogate range
			else if ((wc32 > 0xFFFF /* (1<<16)-1 */) || IS_HIGH_SURROGATE(wc32))
			{
				wchar_from_ucs32((ucs32)LODWORD(mn_AltNumber), wszChars);
			}
			else
			{
				wszChars[0] = (wchar_t)LOWORD(wc32);
				_ASSERTE(wszChars[1] == 0);
			}
		}
		#endif

		// Log received and to be posted data
		wchar_t szLog[80];
		//_wsprintf(szLog, SKIPCOUNT(szLog) L"Received by AltNumber: system=%c (%u) internal=%s (x%X)",
		//	wc ? wc : L'?', (UINT)wc, wszChars, wc32);
		_wsprintf(szLog, SKIPCOUNT(szLog) L"%s: OS=%s (%u/x%X) (internal=x%X)",
			(action == eAltCharAccept) ? L"Received by AltNumber" : L"Skipped by AltNumber",
			wszChars, (UINT)wc, (UINT)wc, wc32);
		LogString(szLog);

		// Send chars to console
		if (wszChars[0] && (action == eAltCharAccept))
			DumpChars(wszChars);
		return true;
	}

	//messg == WM_SYSKEYDOWN(UP), wParam == VK_NUMPAD0..VK_NUMPAD9, lParam == (ex. 0x20490001)

	if (!mb_InAltNumber)
	{
		// Reset flag first
		m_WaitingForAltChar = eAltCharNone;

		// The context code. The value is 1 if the ALT key is held down while the key is pressed
		if ((messg != WM_SYSKEYDOWN) || !(lParam & (1 << 29)))
			return false;

		// No sense to start checks if the key is not keypad number
		if (!((wParam >= VK_NUMPAD0 && wParam <= VK_NUMPAD9)
				|| (wParam == VK_ADD) /* enables hexadecimal mode */
			))
			return false;

		// No other modifiers are down
		if (!isAltNumpad())
			return false;

		// What mode user wants to use?
		if (wParam != VK_ADD)
		{
			// Standard, decimal, we rely on OS
			mn_AltNumber = (wParam - VK_NUMPAD0);
			mn_NumberBase = 10;
		}
		else
		{
			// Hexadecimal, we'll process it internally
			// to implement surrogates support
			mn_AltNumber = 0;
			mn_NumberBase = 16;
		}

		// Ready to continut
		mb_InAltNumber = true;
		return true; // Don't pass to console
	}
	else if ((messg == WM_KEYUP) && (wParam == VK_MENU || wParam == VK_LMENU/*JIC*/))
	{
		//_ASSERTE(mb_InAltNumber && mn_AltNumber);
		m_WaitingForAltChar = DumpAltNumber();
	}
	else if (messg == WM_SYSKEYDOWN)
	{
		if (((wParam >= VK_NUMPAD0) && (wParam <= VK_NUMPAD9))
			|| ((wParam >= 'A') && (wParam <= 'F'))
			|| ((wParam >= 'a') && (wParam <= 'f'))
			)
		{
			UINT nDigit = 0;
			if ((wParam >= VK_NUMPAD0) && (wParam <= VK_NUMPAD9))
				nDigit = (wParam - VK_NUMPAD0);
			else if ((wParam >= 'A') && (wParam <= 'F'))
				nDigit = (10 + (wParam - 'A'));
			else if ((wParam >= 'a') && (wParam <= 'f'))
				nDigit = (10 + (wParam - 'a'));
			mn_AltNumber = mn_AltNumber * mn_NumberBase + nDigit;
			return true; // Don't pass to console
		}
		else
		{
			// Dump collected codepoint before clearing
			wchar_t szLog[80];
			_wsprintf(szLog, SKIPCOUNT(szLog) L"Cleared by AltNumber: codepoint=x%X", LODWORD(mn_AltNumber));
			LogString(szLog);

			// Clear collected codepoint due to invalid input
			mn_AltNumber = 0;
		}
	}

	return false;
}

AltCharAction CAltNumpad::DumpAltNumber()
{
	if (!mb_InAltNumber)
		return eAltCharNone;

	// Reset it immediately
	mb_InAltNumber = false;
	if (!mn_AltNumber)
		return eAltCharNone;

	// Default action - accept input from OS
	AltCharAction rc = eAltCharAccept;

	// Result wchar-s
	wchar_t wszChars[3] = L"";


	// 32-bit codebase
	// UCS defines 16 ‘planes’, so maximum codebase is 0x10FFFF, but
	// we need to use wider mask to include all codebases in range.
	ucs32 wc32 = (ucs32)(DWORD)(mn_AltNumber & 0x1FFFFF);


	// If hexadecimal mode was selected, process codebase internally
	if (mn_NumberBase == 16)
	{
		rc = eAltCharSkip;

		// Drop invalid codebases
		if (wc32 > 0x10FFFF)
		{
			wszChars[0] = 0;
		}
		// To avoid ambiguous surrogates processing, convert
		// codebases which get into high-surrogate range
		else if ((wc32 > 0xFFFF /* (1<<16)-1 */) || IS_HIGH_SURROGATE(wc32))
		{
			wchar_from_ucs32((ucs32)LODWORD(mn_AltNumber), wszChars);
		}
		else
		{
			wszChars[0] = (wchar_t)LOWORD(wc32);
			wszChars[1] = 0;
		}
	}
	else
	{
		// Let use OEM codepage for beginning?
		// Actually, CP does not matter, because here we rely on OS
		// More info: https://conemu.github.io/en/AltNumpad.html
		char szChars[2] = {LOBYTE(wc32), 0};
		MultiByteToWideChar(CP_OEMCP, 0, szChars, -1, wszChars, countof(wszChars));
		_ASSERTE(rc == eAltCharAccept);
	}

	// Prepare data for log
	wchar_t szLog[120], szCodePoints[32];
	// Surrogate pair?
	if (wszChars[0] && wszChars[1])
		_wsprintf(szCodePoints, SKIPCOUNT(szCodePoints) L"x%X x%X", (UINT)(wszChars[0]), (UINT)(wszChars[1]));
	else
		_wsprintf(szCodePoints, SKIPCOUNT(szCodePoints) L"%u", (UINT)(wszChars[0]));
	// Log our chars collected internally
	_wsprintf(szLog, SKIPCOUNT(szLog) L"Gathered by AltNumber: char=%s (%s) (codepoint=x%X)",
		wszChars, szCodePoints, wc32);
	LogString(szLog);

	if ((rc == eAltCharSkip) && wszChars[0])
	{
		DumpChars(wszChars);
	}

	return rc;
}

void CAltNumpad::DumpChars(wchar_t* asChars)
{
	CVConGuard VCon;
	if (mp_ConEmu->GetActiveVCon(&VCon) < 0)
		return;
	VCon->RCon()->PostString(asChars, wcslen(asChars));
}
