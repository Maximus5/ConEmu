
/*
Copyright (c) 2014 Maximus5
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

bool TermX::GetSubstiture(const KEY_EVENT_RECORD& k, wchar_t (&szSubst)[16])
{
	_ASSERTE(szSubst[0] == 0);

	switch (k.wVirtualKeyCode)
	{
	case VK_UP:
		wcscpy_c(szSubst, L"\033O*A");
		return true;
	case VK_DOWN:
		wcscpy_c(szSubst, L"\033O*B");
		return true;
	case VK_RIGHT:
		wcscpy_c(szSubst, L"\033O*C");
		return true;
	case VK_LEFT:
		wcscpy_c(szSubst, L"\033O*D");
		return true;
	case VK_F1:
		wcscpy_c(szSubst, L"\033[11;*~");
		return true;
	case VK_F2:
		wcscpy_c(szSubst, L"\033[12;*~");
		return true;
	case VK_F3:
		wcscpy_c(szSubst, L"\033[13;*~");
		return true;
	case VK_F4:
		wcscpy_c(szSubst, L"\033[14;*~");
		return true;
	case VK_F5:
		wcscpy_c(szSubst, L"\033[15;*~");
		return true;
	case VK_F6:
		wcscpy_c(szSubst, L"\033[17;*~");
		return true;
	case VK_F7:
		wcscpy_c(szSubst, L"\033[18;*~");
		return true;
	case VK_F8:
		wcscpy_c(szSubst, L"\033[19;*~");
		return true;
	case VK_F9:
		wcscpy_c(szSubst, L"\033[20;*~");
		return true;
	case VK_F10:
		wcscpy_c(szSubst, L"\033[21;*~");
		return true;
	case VK_F11:
		wcscpy_c(szSubst, L"\033[23;*~");
		return true;
	case VK_F12:
		wcscpy_c(szSubst, L"\033[24;*~");
		return true;
	case VK_INSERT:
		wcscpy_c(szSubst, L"\033[2;*~");
		return true;
	case VK_HOME:
		wcscpy_c(szSubst, L"\033[1;*H");
		return true;
	case VK_END:
		wcscpy_c(szSubst, L"\033[1;*F");
		return true;
	case VK_PRIOR:
		wcscpy_c(szSubst, L"\033[5;*~");
		return true;
	case VK_NEXT:
		wcscpy_c(szSubst, L"\033[6;*~");
		return true;
	case VK_DELETE:
		wcscpy_c(szSubst, L"\033[3;*~"); // ???
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

	return false;
}
