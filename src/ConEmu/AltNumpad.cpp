
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
#include "../common/MStrDup.h"
#include "AltNumpad.h"
#include "ConEmu.h"
#include "LngRc.h"
#include "RealConsole.h"
#include "Status.h"
#include "VConRelease.h"
#include "VirtualConsole.h"

#define DEBUGSTRCHARS(s) DEBUGSTR(s)

CAltNumpad::CAltNumpad(CConEmuMain* apConEmu)
	: mp_ConEmu(apConEmu)
	, m_WaitingForAltChar(eAltCharNone)
	, mb_InAltNumber(false)
	, mb_External(false)
	, mb_AnsiCP(false)
	, mn_NumberBase(10)
	, mn_AltNumber(0)
	, mn_SkipVkKeyUp(0)
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
	//messg == WM_SYSKEYDOWN(UP), wParam == VK_NUMPAD0..VK_NUMPAD9, lParam == (ex. 0x20490001)

	bool bSkip = false;
	wchar_t szLog[80];

	if (((messg == WM_KEYUP) || (messg == WM_SYSKEYUP) || (messg == WM_CHAR) || (messg == WM_SYSCHAR))
		&& (mn_SkipVkKeyUp == wParam))
	{
		bSkip = true;
		if ((messg == WM_KEYUP) || (messg == WM_SYSKEYUP))
			mn_SkipVkKeyUp = 0;
		_wsprintf(szLog, SKIPCOUNT(szLog) L"AltNumber: KeyUp skipped: msg=%u wParam=%u",
			(messg == WM_KEYUP) ? L"KeyUp" :
			(messg == WM_SYSKEYUP) ? L"SysKeyUp" :
			(messg == WM_CHAR) ? L"Char" :
			(messg == WM_SYSCHAR) ? L"SysChar" : L"???",
			messg, LODWORD(wParam));
		LogString(szLog);
	}
	else if (m_WaitingForAltChar && (messg == WM_CHAR))
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

		// Reset internal buffer
		ClearAltNumber(true);

		wchar_t wszChars[3] = {wc, 0};

		// Log received and to be posted data
		//_wsprintf(szLog, SKIPCOUNT(szLog) L"Received by AltNumber: system=%c (%u) internal=%s (x%X)",
		//	wc ? wc : L'?', (UINT)wc, wszChars, wc32);
		_wsprintf(szLog, SKIPCOUNT(szLog) L"%s: OS=%s (%u/x%X) (internal=x%X)",
			(action == eAltCharAccept) ? L"AltNumber: Received" : L"AltNumber: Skipped",
			wszChars, (UINT)wc, (UINT)wc, wc32);
		LogString(szLog);

		// Send chars to console
		if (wszChars[0] && (action == eAltCharAccept))
			DumpChars(wszChars);

		bSkip = true; // Don't pass to console
	}
	else if (!mb_InAltNumber)
	{
		// Reset flag first
		m_WaitingForAltChar = eAltCharNone;

		// The context code. The value is 1 if the ALT key is held down while the key is pressed
		if ((messg != WM_SYSKEYDOWN) || !(lParam & (1 << 29)))
		{
			_ASSERTE(bSkip == false); // bypass to console
		}
		// No sense to start checks if the key is not keypad number
		else if (!(
					(wParam >= VK_NUMPAD0 && wParam <= VK_NUMPAD9)
					|| (wParam == VK_ADD) /* enables hexadecimal mode */
				))
		{
			_ASSERTE(bSkip == false); // bypass to console
		}
		// No other modifiers are down
		else if (!isAltNumpad())
		{
			_ASSERTE(bSkip == false); // bypass to console
		}
		else
		{
			// What mode user wants to use?

			if (wParam != VK_ADD)
			{
				// Standard, decimal, we rely on OS
				StartCapture(10, (wParam - VK_NUMPAD0), false);
			}
			else
			{
				// Hexadecimal, we'll process it internally
				// to implement surrogates support
				StartCapture(16, 0, false);
			}

			bSkip = true; // Don't pass to console
		}
	}
	// Finalize mode by releasing Alt key
	else if ((messg == WM_KEYUP) && (wParam == VK_MENU || wParam == VK_LMENU/*JIC*/))
	{
		//_ASSERTE(mb_InAltNumber && mn_AltNumber);
		m_WaitingForAltChar = DumpAltNumber();

		bSkip = true; // Don't pass to console
	}
	// We may miss Alt KeyUp event if we loose keyboard focus
	else if (!mb_External && !isAltNumpad())
	{
		DumpAltNumber();
	}
	// AltNumber mode triggered by user-defined hotkey
	else if (mb_External
		&& ((messg == WM_SYSKEYDOWN) || (messg == WM_KEYDOWN))
		&& ((wParam == VK_RETURN) || (wParam == VK_ESCAPE))
		)
	{
		if (wParam == VK_RETURN)
			DumpAltNumber();
		else
			ClearAltNumber(true);
		// Bypass next Enter/Esc *release*
		mn_SkipVkKeyUp = LODWORD(wParam);
		// Don't pass Enter/Esc to console
		bSkip = true;
	}
	else if ((messg == WM_SYSKEYDOWN)
		|| (mb_External && (messg == WM_KEYDOWN)) // if we start capture by some user-defined hotkey
		)
	{
		if (((wParam >= VK_NUMPAD0) && (wParam <= VK_NUMPAD9))
			|| ((mn_NumberBase == 16) && (wParam >= '0' && wParam <= '9'))
			|| ((wParam >= 'A') && (wParam <= 'F'))
			|| ((wParam >= 'a') && (wParam <= 'f'))
			)
		{
			UINT nDigit = 0;
			if ((wParam >= VK_NUMPAD0) && (wParam <= VK_NUMPAD9))
				nDigit = LOBYTE(wParam - VK_NUMPAD0);
			else if ((wParam >= 'A') && (wParam <= 'F'))
				nDigit = LOBYTE(10 + (wParam - 'A'));
			else if ((wParam >= 'a') && (wParam <= 'f'))
				nDigit = LOBYTE(10 + (wParam - 'a'));
			else if (wParam >= '0' && wParam <= '9')
				nDigit = LOBYTE(wParam - '0');
			mn_AltNumber = mn_AltNumber * mn_NumberBase + nDigit;
			DumpStatus();

			bSkip = true; // Don't pass to console
		}
		else if (wParam == VK_ADD)
		{
			// Push to console, if that was our internal mode
			m_WaitingForAltChar = DumpAltNumber();
			// And start new capture sequence
			StartCapture(16, 0, mb_External);

			mn_SkipVkKeyUp = LODWORD(wParam);
			bSkip = true; // Don't pass to console
		}
		else
		{
			// Dump collected codepoint before clearing
			_wsprintf(szLog, SKIPCOUNT(szLog) L"AltNumber: Cleared: codepoint=x%X", LODWORD(mn_AltNumber));
			LogString(szLog);

			// Clear collected codepoint due to invalid input
			mn_AltNumber = 0;
		}
	}
	else
	{
		bSkip = true; // Don't pass to console
	}

	return bSkip;
}

void CAltNumpad::StartCapture(UINT NumberBase, UINT Initial, bool External)
{
	if (!mp_ConEmu->isVConExists(0))
	{
		LogString(L"AltNumber: Skipped: No consoles!");
		return;
	}

	// If user-defined hotkey is "Alt+Key" - let's work in standard mode
	if (External && isAltNumpad())
	{
		LogString(L"AltNumber: Forcing standard mode");
		External = false;
	}

	wchar_t szLog[80];
	_wsprintf(szLog, SKIPCOUNT(szLog) L"AltNumber: Starting: Base=%u Initial=%u %s",
		NumberBase, Initial, External ? L"External" : L"Internal");
	LogString(szLog);

	mn_AltNumber = Initial;
	mn_NumberBase = NumberBase ? NumberBase : 10;

	// Ready to continue
	// mb_InAltNumber will be cleared by DumpAltNumber or ClearAltNumber
	mb_InAltNumber = true;
	mb_External = External;
	// GuiMacro: AltNumber(0) - ANSI, AltNumber(10) - OEM, AltNumber(16) - HEX
	// Std Alt+0ddd - ANSI
	mb_AnsiCP = (NumberBase == 0) || (!External && !Initial && (NumberBase == 10));

	DumpStatus();
}

void CAltNumpad::ClearAltNumber(bool bFull)
{
	// DO NOT RESET m_WaitingForAltChar HERE!

	wchar_t szLog[80];
	_wsprintf(szLog, SKIPCOUNT(szLog) L"AltNumber: Clearing(Full=%u): Active=%u Gathered=%u",
		(UINT)bFull, mb_InAltNumber ? mn_NumberBase : 0, LODWORD(mn_AltNumber));
	LogString(szLog);

	mb_InAltNumber = false;
	mb_External = false;
	mb_AnsiCP = false;

	if (bFull)
	{
		mn_AltNumber = 0;
		mn_NumberBase = 10;
	}

	DumpStatus();
}

AltCharAction CAltNumpad::DumpAltNumber()
{
	if (!mb_InAltNumber)
		return eAltCharNone;

	// Result wchar-s and ucs32
	wchar_t wszChars[3] = L"";
	ucs32 wc32 = GetChars(wszChars);
	// Action for expected WM_CHAR
	AltCharAction rc = (mb_External || (mn_NumberBase == 16))
		? eAltCharSkip    // we process codebase internally
		: eAltCharAccept; // accept input from OS

	// And reset mb_InAltNumber immediately (leave mn_AltNumber for debugging purposes)
	ClearAltNumber(false);

	if (!wc32)
		return eAltCharNone;

	// Prepare data for log
	wchar_t szLog[120], szCodePoints[32];
	// Surrogate pair?
	if (wszChars[0] && wszChars[1])
		_wsprintf(szCodePoints, SKIPCOUNT(szCodePoints) L"x%X x%X", (UINT)(wszChars[0]), (UINT)(wszChars[1]));
	else
		_wsprintf(szCodePoints, SKIPCOUNT(szCodePoints) L"%u", (UINT)(wszChars[0]));
	// Log our chars collected internally
	_wsprintf(szLog, SKIPCOUNT(szLog) L"AltNumber: Gathered: char=%s (%s) (codepoint=x%X)",
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

// Convert collected codebase to wchar_t sequence
// ucs32 wc32 = (ucs32)(DWORD)(mn_AltNumber & 0x1FFFFF);
ucs32 CAltNumpad::GetChars(wchar_t (&wszChars)[3])
{
	// Store collected data in local vars
	UINT NumberBase = mn_NumberBase;
	// 32-bit codebase
	// UCS defines 16 ‘planes’, so maximum codebase is 0x10FFFF, but
	// we need to use wider mask to include all codebases in range.
	ucs32 wc32 = (ucs32)(DWORD)(mn_AltNumber & 0x1FFFFF);
	// And reset mb_InAltNumber immediately (leave mn_AltNumber for debugging purposes)

	// If hexadecimal mode was selected, process codebase internally
	if (mb_External || (NumberBase == 16))
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
			wchar_from_ucs32(wc32, wszChars);
		}
		else
		{
			wszChars[0] = (wchar_t)LOWORD(wc32);
			wszChars[1] = 0;
		}
	}
	else if (mb_AnsiCP)
	{
		wc32 = LOBYTE(wc32);
		wszChars[0] = (wchar_t)wc32;
		wszChars[1] = 0;
	}
	else
	{
		wc32 = LOBYTE(wc32);
		// Actually, CP does not matter, because here we rely on OS
		// More info: https://conemu.github.io/en/AltNumpad.html
		char szChars[2] = {(char)wc32, 0};
		MultiByteToWideChar(CP_OEMCP, 0, szChars, -1, wszChars, countof(wszChars));
	}

	return wc32;
}

void CAltNumpad::DumpStatus()
{
	if (!mp_ConEmu->mp_Status)
		return;

	CEStr lsStatus;
	if (mb_InAltNumber)
	{
		if (mn_AltNumber == 0)
		{
			// OEM may be started by Alt+1..9 only
			_ASSERTE((mn_NumberBase == 16) || mb_AnsiCP);
			lsStatus = CLngRc::getRsrc(
				(mn_NumberBase == 16)
					? lng_AltNumberHex/*"Type UNICODE code point using hexadecimal numbers"*/
					: lng_AltNumberACP/*"Type ANSI code point using decimal numbers"*/
				);
		}
		else
		{
			wchar_t wszChars[3] = L"", szStatus[80];
			ucs32 wc32 = GetChars(wszChars);
			_wsprintf(szStatus, SKIPCOUNT(szStatus) L"Alt+Num: `%s` (%Xh/%u)", wszChars, wc32, wc32);
			lsStatus = lstrmerge(szStatus,
				CLngRc::getRsrc(
					mb_External ? lng_AltNumberExt/*" - <Enter> to paste, <Esc> to cancel"*/
					: (mn_NumberBase == 16) ? lng_AltNumberStdUCS/*" - release <Alt> to paste UNICODE code point"*/
					: mb_AnsiCP ? lng_AltNumberStdACP/*" - release <Alt> to paste ANSI code point"*/
					: lng_AltNumberStdOEM/*" - release <Alt> to paste OEM code point"*/));
		}
	}
	mp_ConEmu->mp_Status->SetStatus(lsStatus);
}
