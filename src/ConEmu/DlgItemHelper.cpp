
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


#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "Header.h"

#include "DlgItemHelper.h"

bool CDlgItemHelper::checkDlgButton(HWND hParent, WORD nCtrlId, UINT uCheck)
{
#ifdef _DEBUG
	HWND hCheckBox = NULL;
	if (!hParent)
	{
		_ASSERTE(hParent != NULL);
	}
	else
	{
		hCheckBox = GetDlgItem(hParent, nCtrlId);
		if (!hCheckBox)
		{
			//_ASSERTE(hCheckBox!=NULL && "Checkbox not found in hParent dlg");
			wchar_t szErr[128]; _wsprintf(szErr, SKIPLEN(countof(szErr)) L"checkDlgButton failed\nControlID %u not found in hParent dlg", nCtrlId);
			MsgBox(szErr, MB_SYSTEMMODAL | MB_ICONSTOP, L"ConEmu settings", ghOpWnd);
		}
		else
		{
			_ASSERTE(uCheck == BST_UNCHECKED || uCheck == BST_CHECKED || (uCheck == BST_INDETERMINATE && (BS_3STATE&GetWindowLong(hCheckBox, GWL_STYLE))));
		}
	}
#endif

	// Аналог CheckDlgButton
	BOOL bRc = CheckDlgButton(hParent, nCtrlId, uCheck);
	return (bRc != FALSE);
}

bool CDlgItemHelper::checkRadioButton(HWND hParent, int nIDFirstButton, int nIDLastButton, int nIDCheckButton)
{
#ifdef _DEBUG
	if (!hParent)
	{
		_ASSERTE(hParent != NULL);
	}
	else if (!GetDlgItem(hParent, nIDFirstButton) || !GetDlgItem(hParent, nIDLastButton) || !GetDlgItem(hParent, nIDCheckButton))
	{
		//_ASSERTE(GetDlgItem(hParent, nIDFirstButton) && "Checkbox not found in hParent dlg");
		//_ASSERTE(GetDlgItem(hParent, nIDLastButton) && "Checkbox not found in hParent dlg");
		//_ASSERTE(GetDlgItem(hParent, nIDCheckButton) && "Checkbox not found in hParent dlg");
		wchar_t szErr[128]; _wsprintf(szErr, SKIPLEN(countof(szErr)) L"checkRadioButton failed\nControlIDs %u,%u,%u not found in hParent dlg", nIDFirstButton, nIDLastButton, nIDCheckButton);
		MsgBox(szErr, MB_SYSTEMMODAL | MB_ICONSTOP, L"ConEmu settings", ghOpWnd);
	}
#endif

	// Аналог CheckRadioButton
	BOOL bRc = CheckRadioButton(hParent, nIDFirstButton, nIDLastButton, nIDCheckButton);
	return (bRc != FALSE);
}

void CDlgItemHelper::enableDlgItem(HWND hParent, WORD nCtrlId, bool bEnabled)
{
	UINT tempCtrlId = nCtrlId;
	enableDlgItems(hParent, &tempCtrlId, 1, bEnabled);
}

void CDlgItemHelper::enableDlgItems(HWND hParent, UINT* pnCtrlID, INT_PTR nCount, bool bEnabled)
{
	#if defined(_DEBUG)
	if (!hParent)
	{
		_ASSERTE(hParent != NULL);
	}
	#endif

	for (INT_PTR i = 0; i < nCount; i++)
	{
		HWND hItem = GetDlgItem(hParent, pnCtrlID[i]);
		if (!hItem)
		{
			#if defined(_DEBUG)
			//_ASSERTE(GetDlgItem(hParent, pnCtrlID[i]) != NULL);
			#endif
			wchar_t szErr[128]; _wsprintf(szErr, SKIPLEN(countof(szErr)) L"enableDlgItems failed\nControlID %u not found in hParent dlg", pnCtrlID[i]);
			MsgBox(szErr, MB_SYSTEMMODAL | MB_ICONSTOP, L"ConEmu settings", ghOpWnd);
			continue;
		}
		EnableWindow(hItem, bEnabled);
	}
}

// FALSE - выключена
// TRUE (BST_CHECKED) - включена
// BST_INDETERMINATE (2) - 3-d state
BYTE CDlgItemHelper::isChecked(HWND hParent, WORD nCtrlId)
{
#ifdef _DEBUG
	if (!hParent)
	{
		_ASSERTE(hParent != NULL);
	}
	else if ((nCtrlId != IDCANCEL) && !GetDlgItem(hParent, nCtrlId))
	{
		//_ASSERTE(hCheckBox!=NULL && "Checkbox not found in hParent dlg");
		wchar_t szErr[128]; _wsprintf(szErr, SKIPLEN(countof(szErr)) L"IsChecked failed\nControlID %u not found in hParent dlg", nCtrlId);
		MsgBox(szErr, MB_SYSTEMMODAL | MB_ICONSTOP, L"ConEmu settings", ghOpWnd);
	}
#endif

	// Аналог IsDlgButtonChecked
	int nChecked = SendDlgItemMessage(hParent, nCtrlId, BM_GETCHECK, 0, 0);
	_ASSERTE(nChecked == 0 || nChecked == 1 || nChecked == 2);

	if (nChecked != 0 && nChecked != 1 && nChecked != 2)
		nChecked = 0;

	return LOBYTE(nChecked);
}

bool CDlgItemHelper::isChecked2(HWND hParent, WORD nCtrlId)
{
	return (isChecked(hParent, nCtrlId) != BST_UNCHECKED);
}

void CDlgItemHelper::InvalidateCtrl(HWND hCtrl, BOOL bErase)
{
	::InvalidateRect(hCtrl, NULL, bErase);
}

int CDlgItemHelper::GetNumber(HWND hParent, WORD nCtrlId, int nMin /*= 0*/, int nMax /*= 0*/)
{
#ifdef _DEBUG
	HWND hChild = GetDlgItem(hParent, nCtrlId);
	_ASSERTE(hChild!=NULL);
#endif
	int nValue = 0;
	wchar_t szNumber[32] = {0};

	if (GetDlgItemText(hParent, nCtrlId, szNumber, countof(szNumber)))
	{
		if (!wcscmp(szNumber, L"None"))
			nValue = 255; // 0xFF для gpSet->AppStd.nFontNormalColor, gpSet->AppStd.nFontBoldColor, gpSet->AppStd.nFontItalicColor;
		else
			nValue = _wtoi((szNumber[0]==L' ') ? (szNumber+1) : szNumber);
		// Validation?
		if (nMin < nMax)
			nValue = min(nMax,max(nMin,nValue));
	}

	return nValue;
}

INT_PTR CDlgItemHelper::GetString(HWND hParent, WORD nCtrlId, wchar_t** ppszStr, LPCWSTR asNoDefault /*= NULL*/, bool abListBox /*= false*/)
{
	INT_PTR nSel = abListBox ? SendDlgItemMessage(hParent, nCtrlId, CB_GETCURSEL, 0, 0) : -1;

	INT_PTR nLen = abListBox
		? ((nSel >= 0) ? SendDlgItemMessage(hParent, nCtrlId, CB_GETLBTEXTLEN, nSel, 0) : 0)
		: SendDlgItemMessage(hParent, nCtrlId, WM_GETTEXTLENGTH, 0, 0);

	if (!ppszStr)
		return nLen;

	if (nLen<=0)
	{
		SafeFree(*ppszStr);
		return nLen;
	}

	wchar_t* pszNew = (TCHAR*)calloc(nLen+1, sizeof(TCHAR));
	if (!pszNew)
	{
		_ASSERTE(pszNew!=NULL);
	}
	else
	{
		if (abListBox)
		{
			if (nSel >= 0)
				SendDlgItemMessage(hParent, nCtrlId, CB_GETLBTEXT, nSel, (LPARAM)pszNew);
		}
		else
		{
			GetDlgItemText(hParent, nCtrlId, pszNew, nLen+1);
		}


		if (*ppszStr)
		{
			if (lstrcmp(*ppszStr, pszNew) == 0)
			{
				free(pszNew);
				return nLen; // Изменений не было
			}
		}

		// Значение "по умолчанию" не запоминаем
		if (asNoDefault && lstrcmp(pszNew, asNoDefault) == 0)
		{
			SafeFree(*ppszStr);
			SafeFree(pszNew);
			nLen = 0;
			// Reset (it is default value!)
			return nLen;
		}

		if (nLen > (*ppszStr ? (INT_PTR)_tcslen(*ppszStr) : 0))
		{
			if (*ppszStr) free(*ppszStr);
			*ppszStr = pszNew; pszNew = NULL;
		}
		else if (*ppszStr)
		{
			_wcscpy_c(*ppszStr, nLen+1, pszNew);
		}
		SafeFree(pszNew);
	}

	return nLen;
}

bool CDlgItemHelper::isHyperlinkCtrl(WORD nCtrlId)
{
	switch (nCtrlId)
	{
	case stHomePage:
	case stDefTermWikiLink:
	case stConEmuUrl:
	case stAnsiSecureExecUrl:
		return true;
	}
	return false;
}

bool CDlgItemHelper::ProcessHyperlinkCtrl(HWND hDlg, WORD nCtrlId)
{
	if (!isHyperlinkCtrl(nCtrlId))
	{
		return false;
	}

	CEStr lsUrl;

	switch (nCtrlId)
	{
	case stConEmuUrl:
		lsUrl = gsHomePage;
		break;
	case stHomePage:
		lsUrl = gsFirstStart;
		break;
	default:
		if (GetString(hDlg, nCtrlId, &lsUrl.ms_Val) <= 0)
			return false;
		if ((0 != wcsncmp(lsUrl, L"http://", 7))
			&& (0 != wcsncmp(lsUrl, L"https://", 8))
			) return false;
	}

	DWORD shellRc = (DWORD)(INT_PTR)ShellExecute(ghWnd, L"open", lsUrl, NULL, NULL, SW_SHOWNORMAL);
	if (shellRc <= 32)
	{
		DisplayLastError(L"ShellExecute failed", shellRc);
		return false;
	}

	return true;
}
