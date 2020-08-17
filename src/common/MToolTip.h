﻿
/*
Copyright (c) 2009-present Maximus5
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

#include "defines.h"
#include <commctrl.h>

class CToolTip
{
public:
	CToolTip();
	virtual ~CToolTip();
public:
	void ShowTip(HWND ahOwner, HWND ahControl, LPCWSTR asText, BOOL abBalloon, POINT pt, HINSTANCE hInstance = NULL/*g_hInstance*/);
	void HideTip();
	//virtual bool OnMenuSelected(HMENU hMenu, WORD nID, WORD nFlags) { return false; };
private:
	HWND mh_Tip, mh_Ball;
	wchar_t *mpsz_LastTip;
	int mn_LastTipCchMax;
	TOOLINFO mti_Tip, mti_Ball;
	int mb_LastTipBalloon;
};
