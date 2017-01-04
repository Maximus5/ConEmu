
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

#define BST(v) (int)(v & (BST_CHECKED|BST_INDETERMINATE)) // BST_UNCHECKED/BST_CHECKED/BST_INDETERMINATE

class CDlgItemHelper
{
public:
	static bool checkDlgButton(HWND hParent, WORD nCtrlId, UINT uCheck);
	static bool checkRadioButton(HWND hParent, int nIDFirstButton, int nIDLastButton, int nIDCheckButton);
	static void enableDlgItem(HWND hParent, WORD nCtrlId, bool bEnabled);
	static void enableDlgItems(HWND hParent, UINT* pnCtrlID, INT_PTR nCount, bool bEnabled);
	static BYTE isChecked(HWND hParent, WORD nCtrlId);
	static bool isChecked2(HWND hParent, WORD nCtrlId);
	static void InvalidateCtrl(HWND hCtrl, BOOL bErase);
	static int GetNumber(HWND hParent, WORD nCtrlId, int nMin = 0, int nMax = 0);
	static INT_PTR GetString(HWND hParent, WORD nCtrlId, wchar_t** ppszStr, LPCWSTR asNoDefault = NULL, bool abListBox = false);
	static bool isHyperlinkCtrl(WORD nCtrlId);
	static bool ProcessHyperlinkCtrl(HWND hDlg, WORD nCtrlId);
};
