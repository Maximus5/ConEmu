
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


#pragma once

#include <windows.h>

#include "../common/wcchars.h"

class CConEmuMain;
class CRealConsole;

enum AltCharAction
{
	eAltCharNone = 0, // We are not in Alt+Number sequence
	eAltCharAccept,   // Rely on OS
	eAltCharSkip,     // Processed internally
};

class CAltNumpad
{
protected:
	CConEmuMain* mp_ConEmu;
	AltCharAction m_WaitingForAltChar;
	bool mb_InAltNumber;
	bool mb_External; // Mode triggered by hotkey
	bool mb_AnsiCP; // If started by Alt+0ddd
	UINT mn_NumberBase; // 10 or 16
	u64  mn_AltNumber;
	UINT mn_SkipVkKeyUp;

public:
	CAltNumpad(CConEmuMain* apConEmu);
	~CAltNumpad();

public:
	// Methods
	bool OnKeyboard(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
	void StartCapture(UINT NumberBase, UINT Initial, bool External);

protected:
	void ClearAltNumber(bool bFull);
	void DumpChars(wchar_t* asChars);
	AltCharAction DumpAltNumber();
	bool isAltNumpad();
	void DumpStatus();
protected:
	ucs32 GetChars(wchar_t (&wszChars)[3]);
};
