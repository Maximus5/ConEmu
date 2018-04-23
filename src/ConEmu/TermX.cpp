
/*
Copyright (c) 2014-present Maximus5
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
#include "../common/MStrSafe.h"
#include "ConEmu.h"
#include "TermX.h"

#define X_CTRL(c) ((c) & 0x1F)
#define X_CDEL 0x7F // alternative - X_CTRL('H') ?

TermX::TermX()
{
	Reset();
}

void TermX::Reset()
{
	AppCursorKeys = false;
	MouseButtons = 0;
	LastMousePos = MakeCoord(-1,-1);
	LastDeadCharVK = 0;
}

bool TermX::GetSubstitute(const KEY_EVENT_RECORD& k, CEStr& lsSubst)
{
	_ASSERTE(lsSubst.IsEmpty());

	// Bypass AltGr+keys to console intact
	if ((k.dwControlKeyState & (LEFT_CTRL_PRESSED|RIGHT_ALT_PRESSED)) == (LEFT_CTRL_PRESSED|RIGHT_ALT_PRESSED))
	{
		if (k.uChar.UnicodeChar)
		{
			wchar_t szData[3] = {k.uChar.UnicodeChar};
			if (LastDeadCharVK == k.wVirtualKeyCode)
				szData[1] = k.uChar.UnicodeChar;
			lsSubst.Set(szData);
			LastDeadCharVK = 0;
		}
		else
		{
			LastDeadCharVK = k.wVirtualKeyCode;
			lsSubst.Clear();
		}
		return true;
	}

	wchar_t szSubst[16] = L"";

	static UINT F1Codes[24] = {
		11, 12, 13, 14, 15, 17, 18, 19, 20, 21, 23, 24,
		25, 26, 28, 29, 31, 32, 33, 34, 42, 43, 44, 45
	};
	static wchar_t F1F4Codes[] = {L'P', L'Q', L'R', L'S'};

	static wchar_t ArrowCodes[] = {L'F', L'H', L'D', L'A', L'C', L'B'};

	typedef DWORD XTermCtrls;
	const XTermCtrls
		xtc_Shift = 1,
		xtc_Alt   = 2,
		xtc_Ctrl  = 4,
		xtc_None  = 0;
	struct processor
	{
		XTermCtrls Mods;
		wchar_t szSubst[16];

		void SetKey(CEStr& lsSubst, wchar_t c, wchar_t prefix=L'O')
		{
			if (!Mods)
			{
				//wcscpy_c(szSubst, L"\033O*A");
				msprintf(szSubst, countof(szSubst), L"\033%c%c", prefix, c);
			}
			else
			{
				msprintf(szSubst, countof(szSubst), L"\033[1;%c%c", Mods+L'1', c);
			}
			lsSubst.Set(szSubst);
		}

		void SetFKey(CEStr& lsSubst, UINT fn)
		{
			if (!Mods)
			{
				msprintf(szSubst, countof(szSubst), L"\033[%u~", fn);
			}
			else
			{
				msprintf(szSubst, countof(szSubst), L"\033[%u;%c~", fn, Mods+L'1');
			}
			lsSubst.Set(szSubst);
		}

		void SetTilde(CEStr& lsSubst, wchar_t c)
		{
			if (!Mods)
			{
				msprintf(szSubst, countof(szSubst), L"\033[%c~", c);
			}
			else
			{
				msprintf(szSubst, countof(szSubst), L"\033[%c;%c~", c, Mods+L'1');
			}
			lsSubst.Set(szSubst);
		}
	} Processor = {xtc_None};

	if (k.dwControlKeyState & (SHIFT_PRESSED))
		Processor.Mods |= xtc_Shift;
	if (k.dwControlKeyState & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED))
		Processor.Mods |= xtc_Alt;
	if (k.dwControlKeyState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED))
		Processor.Mods |= xtc_Ctrl;

	switch (k.wVirtualKeyCode)
	{
	case VK_END:  case VK_HOME:  case VK_LEFT:
	case VK_UP:   case VK_RIGHT: case VK_DOWN:
		Processor.SetKey(lsSubst, ArrowCodes[(k.wVirtualKeyCode-VK_END)], AppCursorKeys ? L'O' : L'[');
		return true;

	case VK_F1: case VK_F2: case VK_F3: case VK_F4:
		Processor.SetKey(lsSubst, F1F4Codes[(k.wVirtualKeyCode-VK_F1)]);
		return true;
	case VK_F5: case VK_F6: case VK_F7: case VK_F8:
	case VK_F9: case VK_F10: case VK_F11: case VK_F12: case VK_F13: case VK_F14: case VK_F15: case VK_F16:
	case VK_F17: case VK_F18: case VK_F19: case VK_F20: case VK_F21: case VK_F22: case VK_F23: case VK_F24:
		// "\033[11;*~" .. L"\033[15;*~", and so on: F1Codes[]
		Processor.SetFKey(lsSubst, F1Codes[(k.wVirtualKeyCode-VK_F1)]);
		return true;

	case VK_INSERT:
		Processor.SetTilde(lsSubst, L'2');
		return true;
	case VK_PRIOR:
		Processor.SetTilde(lsSubst, L'5');
		return true;
	case VK_NEXT:
		Processor.SetTilde(lsSubst, L'6');
		return true;
	case VK_DELETE:
		Processor.SetTilde(lsSubst, L'3');
		return true;
	case VK_BACK:
		if ((Processor.Mods & xtc_Alt)) szSubst[0] = 0x1B;
		szSubst[(Processor.Mods == xtc_Alt) ? 1 : 0] = ((Processor.Mods & (xtc_Ctrl|xtc_Alt)) == (xtc_Ctrl|xtc_Alt)) ? 0x9F
			: (Processor.Mods & xtc_Ctrl) ? X_CTRL('_')/*0x1F*/ : X_CDEL/*0x7F*/;
		szSubst[(Processor.Mods == xtc_Alt) ? 2 : 1] = 0;
		lsSubst.Set(szSubst);
		return true;

	case VK_TAB:
		if (!(k.dwControlKeyState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED|LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED)))
		{
			wcscpy_c(szSubst, (k.dwControlKeyState & SHIFT_PRESSED) ? L"\033[Z" : L"\t");
		}
		lsSubst.Set(szSubst);
		return true;

	/* NumPad with NumLock ON */
	case VK_NUMPAD0:
	case VK_NUMPAD1:
	case VK_NUMPAD2:
	case VK_NUMPAD3:
	case VK_NUMPAD4:
	case VK_NUMPAD5:
	case VK_NUMPAD6:
	case VK_NUMPAD7:
	case VK_NUMPAD8:
	case VK_NUMPAD9:
	case VK_DECIMAL: // VK_OEM_PERIOD}, // Actually, this may be comma
	case VK_DIVIDE:
	case VK_MULTIPLY:
	case VK_SUBTRACT:
	case VK_ADD:
		if (k.uChar.UnicodeChar)
		{
			// Just a digits '0'..'9' and symbols +-/*.
			szSubst[0] = k.uChar.UnicodeChar; szSubst[1] = 0;
		}
		lsSubst.Set(szSubst);
		return true;
	}

	// Alt+Char
	if ((Processor.Mods & xtc_Alt) && k.uChar.UnicodeChar
		// connector/gh#4: AltGr+Char
		&& (((k.dwControlKeyState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED|LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED))
				!= (LEFT_CTRL_PRESSED|RIGHT_ALT_PRESSED))
		// or just a Alt+Char
			|| !(Processor.Mods & xtc_Ctrl))
		)
	{
		szSubst[0] = L'\033'; szSubst[1] = k.uChar.UnicodeChar; szSubst[2] = 0;
		lsSubst.Set(szSubst);
		return true;
	}

	// Ctrl+Char: A-->1, ..., J-->10, ...
	if (Processor.Mods & (xtc_Alt | xtc_Ctrl))
	{
		wchar_t key_char = k.uChar.UnicodeChar;
		if (!key_char)
		{
			switch (k.wVirtualKeyCode)
			{
			case VK_OEM_2:
				key_char = L'?'; break; // Ctrl+/ -> \x1F
			case VK_OEM_3:
				key_char = L'~'; break;
			case VK_OEM_4:
				key_char = L'['; break;
			case VK_OEM_5:
				key_char = L'\\'; break;
			case VK_OEM_6:
				key_char = L']'; break;
			case VK_OEM_COMMA:
				key_char = L','; break;
			case VK_OEM_PERIOD:
				key_char = L'.'; break;
			case VK_OEM_MINUS:
				key_char = L'-'; break;
			case VK_OEM_PLUS:
				key_char = L'='; break;
			default:
				if (k.wVirtualKeyCode >= L'0' && k.wVirtualKeyCode <= L'9')
					key_char = k.wVirtualKeyCode;
				else if (k.wVirtualKeyCode >= L'A' && k.wVirtualKeyCode <= L'Z')
					key_char = k.wVirtualKeyCode;
			}
		}


		if (key_char)
		{
			szSubst[0] = szSubst[1] = szSubst[2] = szSubst[3] = 0;
			if ((Processor.Mods == xtc_Ctrl) && (key_char == L'2' || key_char == L' '))
			{
				// Must send '\x00' but it's not a valid ASCIIZ string, have to be processed in connector
				return false;
			}
			else if (key_char >= L'A' && key_char <= L'Z')
			{
				if (Processor.Mods == xtc_Ctrl)
				{
					// In vt220: If XOFF support is enabled, then CTRL-S is a "hold screen" local function and CTRL-Q is an "unhold screen" local function.
					szSubst[0] = (wchar_t)(1 + (key_char - L'A'));
				}
				else if ((Processor.Mods == (xtc_Ctrl|xtc_Alt))
					|| (Processor.Mods == (xtc_Ctrl|xtc_Alt|xtc_Shift)))
				{
					szSubst[0] = 0x1B;
					szSubst[1] = (wchar_t)(1 + (key_char - L'A'));
				}
				// doesn't work, "showkey -a" receives "<x1B><xC3><x82><x01>"
				//else if (Processor.Mods == (xtc_Ctrl|xtc_Alt|xtc_Shift))
				//{
				//	szSubst[0] = 0x1B; szSubst[1] = 0xC2;
				//	szSubst[2] = (wchar_t)(1 + (key_char - L'A'));
				//}
			}
			else if ((Processor.Mods == xtc_Ctrl) && (key_char == L'1' || key_char == L'9' || key_char == L'0'))
			{
				// Strangely Ctrl+1, Ctrl+9, Ctrl+0 generate only 1, 9 and 0.
				szSubst[0] = key_char;
			}
			else if ((Processor.Mods == xtc_Ctrl) && (key_char >= L'3' && key_char <= L'7'))
			{
				szSubst[0] = (wchar_t)(0x1B + (key_char - L'3'));
			}
			else switch (key_char)
			{
			case L',':
				szSubst[0] = 0x2C; break;
			//	Processor.SetKey(lsSubst, L'l'); break;
			case L'.':
				szSubst[0] = 0x2E; break;
			//	Processor.SetKey(lsSubst, L'n'); break;
			//case L'-':
			//	Processor.SetKey(lsSubst, L'm'); break;
			//case L'0':
			//	Processor.SetKey(lsSubst, L'p'); break;
			//case L'1':
			//	Processor.SetKey(lsSubst, L'q'); break;
			// case L'2': // processed above
			//case L'3':
			//	Processor.SetKey(lsSubst, L's'); break;
			//case L'4':
			//	Processor.SetKey(lsSubst, L't'); break;
			//case L'5':
			//	Processor.SetKey(lsSubst, L'u'); break;
			//case L'6':
			case L'~':
				szSubst[0] = 0x1E;
				break;
			//case L'7':
			//	Processor.SetKey(lsSubst, L'w'); break;
			case L'8':
				szSubst[0] = 0x1B;
				if ((Processor.Mods == (xtc_Ctrl|xtc_Alt)) || (Processor.Mods == (xtc_Ctrl|xtc_Alt|xtc_Shift)))
					szSubst[1] = 0x7F;
				else
					szSubst[0] = 0x7F;
				break;
				//Processor.SetKey(lsSubst, L'x'); break;
			//case L'9':
			//	Processor.SetKey(lsSubst, L'y'); break;
			//case L'=':
			//	Processor.SetKey(lsSubst, L'X'); break;
			case L'[':
			case L'\\':
			case L']':
			{
				wchar_t c = (key_char == L'[') ? 0x1B : (key_char == L'\\') ? 0x1C : 0x1D;
				szSubst[0] = 0x1B;
				if (Processor.Mods == xtc_Alt)
					szSubst[1] = 0x40 + c;
				else if ((Processor.Mods == (xtc_Ctrl|xtc_Alt)) || (Processor.Mods == (xtc_Ctrl|xtc_Alt|xtc_Shift)))
					szSubst[1] = c;
				else
					szSubst[0] = c;
				break;
			}
			case L'?':
				if (!(Processor.Mods & xtc_Alt))
					szSubst[0] = (Processor.Mods & xtc_Shift) ? 0x7F : 0x1F;
				else
				{
					szSubst[0] = 0x1B; szSubst[1] = (Processor.Mods & xtc_Shift) ? 0x3F : 0x2F;
				}
				break;
			}
			if (szSubst[0])
			{
				lsSubst.Set(szSubst);
				return true;
			}
		}
	}

	return false;
}

bool TermX::GetSubstitute(const MOUSE_EVENT_RECORD& m, TermMouseMode MouseMode, CEStr& lsSubst)
{
	_ASSERTE(lsSubst.IsEmpty());

	if (!MouseMode)
	{
		lsSubst.Clear();
		return false;
	}

	wchar_t szSubst[16] = L"";

	// Deprecated. Mouse events mimic xterm behavior now.
	#if 0
	// http://conemu.github.io/en/VimXterm.html#Vim-scrolling-using-mouse-Wheel
	// https://github.com/Maximus5/ConEmu/issues/1007
	if ((m.dwEventFlags & MOUSE_WHEELED)
		&& (MouseMode & tmm_VIM))
	{
		// If the high word of the dwButtonState member contains
		// a positive value, the wheel was rotated forward, away from the user.
		// Otherwise, the wheel was rotated backward, toward the user.
		short dir = (short)HIWORD(m.dwButtonState);

		// Ctrl/Alt/Shift
		DWORD mods = (m.dwControlKeyState & (LEFT_ALT_PRESSED|LEFT_CTRL_PRESSED|RIGHT_ALT_PRESSED|RIGHT_CTRL_PRESSED|SHIFT_PRESSED));

		if (mods == 0)
		{
			if (dir <= 0)
				wcscpy_c(szSubst, L"\033[62~"); // <MouseDown>
			else
				wcscpy_c(szSubst, L"\033[63~"); // <MouseUp>
		}
		else if (mods == SHIFT_PRESSED)
		{
			if (dir <= 0)
				wcscpy_c(szSubst, L"\033[64~"); // <S-MouseDown>
			else
				wcscpy_c(szSubst, L"\033[65~"); // <S-MouseUp>
		}
		else
			return false;
		lsSubst.Set(szSubst);
		return true;
	}
	#endif

	if ((m.dwEventFlags & MOUSE_WHEELED)
		&& (MouseMode & tmm_SCROLL))
	{
		KEY_EVENT_RECORD k = {TRUE, 1};
		short dir = (short)HIWORD(m.dwButtonState);
		DWORD mods = (m.dwControlKeyState & (LEFT_ALT_PRESSED|LEFT_CTRL_PRESSED|RIGHT_ALT_PRESSED|RIGHT_CTRL_PRESSED|SHIFT_PRESSED));
		UINT nCount = 1;
		if (mods == 0)
		{
			k.wVirtualKeyCode = (dir > 0) ? VK_UP : VK_DOWN;
			nCount = gpConEmu->mouse.GetWheelScrollLines();
		}
		else if (mods == SHIFT_PRESSED)
		{
			k.wVirtualKeyCode = (dir > 0) ? VK_PRIOR : VK_NEXT;
		}
		else
			return false;
		CEStr lsPart;
		if (!GetSubstitute(k, lsPart) || lsPart.IsEmpty())
			return false;
		if (nCount > 1)
		{
			INT_PTR l = lsPart.GetLen();
			wchar_t* ptr = lsSubst.GetBuffer(l*nCount);
			for (UINT i = 1; i <= nCount; ++i, ptr += l)
				wcscpy_s(ptr, l+1, lsPart);
		}
		else
		{
			lsSubst.Attach(lsPart.Detach());
		}
		return true;
	}
	if (!(MouseMode & ~(tmm_VIM|tmm_SCROLL)))
		return false;


	BYTE NewBtns = (m.dwButtonState & 0x1F);
	if ((NewBtns != MouseButtons)
		|| ((m.dwEventFlags & MOUSE_WHEELED) && HIWORD(m.dwButtonState))
		|| ((LastMousePos != m.dwMousePosition)
			&& ((MouseMode & tmm_ANY)
				|| ((MouseMode & tmm_BTN) && NewBtns)))
		)
	{
		// #XTERM_MOUSE Unfortunately, szSubst is too short to pass "missed" coordinates
		// Like we do for Far events, MouseMove with RBtn pressed in tmm_ANY|tmm_BTN
		// modes would send all intermediate coordinates between two events

		BYTE code; bool released = false;
		if (NewBtns & FROM_LEFT_1ST_BUTTON_PRESSED)
			code = 0; // MB1 pressed
		else if (NewBtns & FROM_LEFT_2ND_BUTTON_PRESSED)
			code = 1; // MB2 pressed
		else if (NewBtns & RIGHTMOST_BUTTON_PRESSED)
			code = 2; // MB3 pressed
		else if (NewBtns & FROM_LEFT_3RD_BUTTON_PRESSED)
			code = 64; // MB4 pressed
		else if (NewBtns & FROM_LEFT_4TH_BUTTON_PRESSED)
			code = 64 + 1; // MB5 pressed
		else if (m.dwEventFlags & MOUSE_WHEELED)
		{
			// #XTERM_MOUSE Post multiple events if dir contains multiple notches
			short dir = (short)HIWORD(m.dwButtonState);
			if (dir > 0)
				code = 64; // MB4
			else if (dir < 0)
				code = 64 + 1; // MB5
		}
		else
		{
			released = true;
			code = 3;
		}

		if (m.dwControlKeyState & SHIFT_PRESSED)
			code |= 4;
		if (m.dwControlKeyState & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED))
			code |= 8;
		if (m.dwControlKeyState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED))
			code |= 16;

		if ((m.dwEventFlags & MOUSE_MOVED)
			&& (MouseMode & (tmm_BTN|tmm_ANY)))
			code |= 32;

		// (1,1) is upper left character position
		SHORT coord[] = {std::max<SHORT>(0, m.dwMousePosition.X) + 1,
			std::max<SHORT>(0, m.dwMousePosition.Y) + 1};

		if (MouseMode & tmm_XTERM)
		{
			msprintf(szSubst, countof(szSubst), L"\033[<%u;%u;%u%c",
				code, coord[0], coord[1], released ? 'm' : 'M');
		}
		else if (MouseMode & tmm_URXVT)
		{
			msprintf(szSubst, countof(szSubst), L"\033[%u;%u;%uM", code + 0x20, coord[0], coord[1]);
		}
		else
		{
			wcscpy_c(szSubst, L"\033[M");
			size_t i = wcslen(szSubst);
			szSubst[i++] = code + 32;

			// And coords. Must be in "screen" coordinate space
			for (size_t s = 0; s < 2; ++s)
			{
				// (1,1) is upper left character position
				if (!(MouseMode & tmm_UTF8))
					szSubst[i++] = std::min<unsigned>(255, coord[s] + 32);
				else if (coord[s] < 0x80)
					szSubst[i++] = coord[s];
				else if (coord[s] < 0x800)
				{
					// xterm #262: positions from 96 to 2015 are encoded as a two-byte UTF-8 sequence
					szSubst[i++] = 0xC0 + (coord[s] >> 6);
					szSubst[i++] = 0x80 + (coord[s] & 0x3F);
				}
				else
				{
					// #XTERM_MOUSE Xterm reports out-of-range positions as a NUL byte.
					szSubst[i++] = 1; // It's impossible to post NUL byte ATM
				}
			}
			szSubst[i++] = 0;
		}

		MouseButtons = NewBtns;
		LastMousePos = m.dwMousePosition;
		lsSubst.Set(szSubst);
	}
	else
	{
		// If XTerm emulation for mouse was requested - don't try to send
		// unprocessed mouse events in Windows way
		lsSubst.Clear();
	}

	return true;
}
