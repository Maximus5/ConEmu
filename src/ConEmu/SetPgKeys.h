
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

#include "SetDlgLists.h"
#include "SetPgBase.h"

struct ConEmuHotKey;

class CSetPgKeys
	: public CSetPgBase
{
public:
	static CSetPgBase* Create() { return new CSetPgKeys(); };
	static TabHwndIndex PageType() { return thi_Keys; };
	virtual TabHwndIndex GetPageType() override { return PageType(); };
public:
	CSetPgKeys();
	virtual ~CSetPgKeys();

public:
	// Methods
	virtual LRESULT OnInitDialog(HWND hDlg, bool abInitial) override;
	virtual INT_PTR PageDlgProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam) override;

	void FillHotKeysList(HWND hWnd2, bool abInitial);
	LRESULT OnHotkeysNotify(HWND hWnd2, WPARAM wParam, LPARAM lParam);
	static int CALLBACK HotkeysCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

	virtual INT_PTR OnButtonClicked(HWND hDlg, HWND hBtn, WORD nCtrlId) override;
	virtual LRESULT OnEditChanged(HWND hDlg, WORD nCtrlId) override;
	virtual INT_PTR OnComboBox(HWND hWnd2, WORD nCtrlId, WORD code) override;
	virtual bool QueryDialogCancel() override;

	static void ReInitHotkeys();

	void RefilterHotkeys(bool bReset = true);

	void SetHotkeyVkMod(ConEmuHotKey *pHK, DWORD VkMod);

protected:
	enum KeyListColumns
	{
		klc_Type = 0,
		klc_Hotkey,
		klc_Desc
	};

protected:
	// Members
	ConEmuHotKey *mp_ActiveHotKey;
	KeysFilterValues mn_LastShowType;
	bool mb_LastHideEmpties;
	CEStr ms_LastFilter;
};
