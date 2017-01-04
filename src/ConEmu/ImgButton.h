
/*
Copyright (c) 2016-2017 Maximus5
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
#include "../common/MArray.h"

class CToolImg;

typedef void (*OnImgBtnClick_t)();
typedef CToolImg* (*ToolImgCreate_t)();

struct BtnInfo
{
	wchar_t   ResId[32];
	CToolImg* pImg;
	wchar_t*  pszHint;
	UINT      nCtrlId;

	// Calls ShellExecute
	OnImgBtnClick_t OnClick;
};

extern wchar_t gsDonatePage[]; // = CEDONATEPAGE;
extern wchar_t gsFlattrPage[]; // = CEFLATTRPAGE;

class CImgButtons
{
protected:
	HWND hDlg;
	HWND hwndTip;
	int AlignLeftId, AlignVCenterId;
	MArray<BtnInfo> m_Btns;

public:
	CImgButtons(HWND ahDlg, int aAlignLeftId, int aAlignVCenterId);
	~CImgButtons();

	void AddDonateButtons();

	bool Process(HWND ahDlg, UINT messg, WPARAM wParam, LPARAM lParam, INT_PTR& lResult);

protected:
	void Add(LPCWSTR asText, ToolImgCreate_t fnCreate, UINT nCtrlId, LPCWSTR asHint, OnImgBtnClick_t fnClick);
	void ImplementButtons();
	void RegisterTip();
	bool GetBtnInfo(UINT nCtrlId, BtnInfo** ppBtn);
};
