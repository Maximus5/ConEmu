
/*
Copyright (c) 2014-2017 Maximus5
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
}

bool TermX::GetSubstitute(const KEY_EVENT_RECORD& k, wchar_t (&szSubst)[16])
{
	_ASSERTE(szSubst[0] == 0);

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

		void SetKey(wchar_t (&szSubst)[16], wchar_t c, wchar_t prefix=L'O')
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
		}

		void SetFKey(wchar_t (&szSubst)[16], UINT fn)
		{
			if (!Mods)
			{
				msprintf(szSubst, countof(szSubst), L"\033[%u~", fn);
			}
			else
			{
				msprintf(szSubst, countof(szSubst), L"\033[%u;%c~", fn, Mods+L'1');
			}
		}

		void SetTilde(wchar_t (&szSubst)[16], wchar_t c)
		{
			if (!Mods)
			{
				msprintf(szSubst, countof(szSubst), L"\033[%c~", c);
			}
			else
			{
				msprintf(szSubst, countof(szSubst), L"\033[%c;%c~", c, Mods+L'1');
			}
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
		Processor.SetKey(szSubst, ArrowCodes[(k.wVirtualKeyCode-VK_END)], AppCursorKeys ? L'O' : L'[');
		return true;

	case VK_F1: case VK_F2: case VK_F3: case VK_F4:
		Processor.SetKey(szSubst, F1F4Codes[(k.wVirtualKeyCode-VK_F1)]);
		return true;
	case VK_F5: case VK_F6: case VK_F7: case VK_F8:
	case VK_F9: case VK_F10: case VK_F11: case VK_F12: case VK_F13: case VK_F14: case VK_F15: case VK_F16:
	case VK_F17: case VK_F18: case VK_F19: case VK_F20: case VK_F21: case VK_F22: case VK_F23: case VK_F24:
		// "\033[11;*~" .. L"\033[15;*~", and so on: F1Codes[]
		Processor.SetFKey(szSubst, F1Codes[(k.wVirtualKeyCode-VK_F1)]);
		return true;

	case VK_INSERT:
		Processor.SetTilde(szSubst, L'2');
		return true;
	case VK_PRIOR:
		Processor.SetTilde(szSubst, L'5');
		return true;
	case VK_NEXT:
		Processor.SetTilde(szSubst, L'6');
		return true;
	case VK_DELETE:
		Processor.SetTilde(szSubst, L'3');
		return true;
	case VK_BACK:
		if ((Processor.Mods & xtc_Alt)) szSubst[0] = 0x1B;
		szSubst[(Processor.Mods == xtc_Alt) ? 1 : 0] = ((Processor.Mods & (xtc_Ctrl|xtc_Alt)) == (xtc_Ctrl|xtc_Alt)) ? 0x9F
			: (Processor.Mods & xtc_Ctrl) ? X_CTRL('_')/*0x1F*/ : X_CDEL/*0x7F*/;
		szSubst[(Processor.Mods == xtc_Alt) ? 2 : 1] = 0;
		return true;

	case VK_TAB:
		if (!(k.dwControlKeyState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED|LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED)))
		{
			wcscpy_c(szSubst, (k.dwControlKeyState & SHIFT_PRESSED) ? L"\033[Z" : L"\t");
		}
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
			//	Processor.SetKey(szSubst, L'l'); break;
			case L'.':
				szSubst[0] = 0x2E; break;
			//	Processor.SetKey(szSubst, L'n'); break;
			//case L'-':
			//	Processor.SetKey(szSubst, L'm'); break;
			//case L'0':
			//	Processor.SetKey(szSubst, L'p'); break;
			//case L'1':
			//	Processor.SetKey(szSubst, L'q'); break;
			// case L'2': // processed above
			//case L'3':
			//	Processor.SetKey(szSubst, L's'); break;
			//case L'4':
			//	Processor.SetKey(szSubst, L't'); break;
			//case L'5':
			//	Processor.SetKey(szSubst, L'u'); break;
			//case L'6':
			case L'~':
				szSubst[0] = 0x1E;
				break;
			//case L'7':
			//	Processor.SetKey(szSubst, L'w'); break;
			case L'8':
				szSubst[0] = 0x1B;
				if ((Processor.Mods == (xtc_Ctrl|xtc_Alt)) || (Processor.Mods == (xtc_Ctrl|xtc_Alt|xtc_Shift)))
					szSubst[1] = 0x7F;
				else
					szSubst[0] = 0x7F;
				break;
				//Processor.SetKey(szSubst, L'x'); break;
			//case L'9':
			//	Processor.SetKey(szSubst, L'y'); break;
			//case L'=':
			//	Processor.SetKey(szSubst, L'X'); break;
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
				return true;
		}
	}

	return false;
}

bool TermX::GetSubstitute(const MOUSE_EVENT_RECORD& m, TermMouseMode MouseMode, wchar_t (&szSubst)[16])
{
	if (m.dwEventFlags & MOUSE_WHEELED)
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
			return true;
		}
		else if (mods == SHIFT_PRESSED)
		{
			if (dir <= 0)
				wcscpy_c(szSubst, L"\033[64~"); // <S-MouseDown>
			else
				wcscpy_c(szSubst, L"\033[65~"); // <S-MouseUp>
			return true;
		}
	}

	if (!MouseMode)
		return false;

	BYTE NewBtns = (m.dwButtonState & 0x1F);
	if ((NewBtns != MouseButtons)
		|| ((LastMousePos != m.dwMousePosition)
			&& ((MouseMode & tmm_ANY)
				|| ((MouseMode & tmm_BTN) && NewBtns)))
		)
	{
		wcscpy_c(szSubst, L"\033[M");
		if (NewBtns & FROM_LEFT_1ST_BUTTON_PRESSED)
			szSubst[3] = 0; // MB1 pressed
		else if (NewBtns & FROM_LEFT_2ND_BUTTON_PRESSED)
			szSubst[3] = 1; // MB2 pressed
		else if (NewBtns & RIGHTMOST_BUTTON_PRESSED)
			szSubst[3] = 2; // MB3 pressed
		else if (NewBtns & FROM_LEFT_3RD_BUTTON_PRESSED)
			szSubst[3] = 64; // MB4 pressed
		else if (NewBtns & FROM_LEFT_4TH_BUTTON_PRESSED)
			szSubst[3] = 64 + 1; // MB5 pressed
		else
			szSubst[3] = 3; // release
		if (m.dwControlKeyState & SHIFT_PRESSED)
			szSubst[3] |= 4;
		if (m.dwControlKeyState & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED))
			szSubst[3] |= 8;
		if (m.dwControlKeyState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED))
			szSubst[3] |= 16;
		szSubst[3] += 32;

		// #XTERM_MOUSE Unfortunately, szSubst is too short to pass "missed" coordinates
		// Like we do for Far events, MouseMove with RBtn pressed in tmm_ANY|tmm_BTN
		// modes would send all intermediate coordinates between two events

		// And coords. Must be in "screen" coordinate space: (1,1) is upper left character position
		szSubst[4] = klMin(255, m.dwMousePosition.X + 1 + 32); // no way to pass coordinate
		szSubst[5] = klMin(255, m.dwMousePosition.Y + 1 + 32); // larger than (255 - 32)
		szSubst[6] = 0;

		MouseButtons = NewBtns;
		LastMousePos = m.dwMousePosition;
		return true;
	}

	return false;
}
