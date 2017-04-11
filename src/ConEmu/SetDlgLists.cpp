
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

#include "DlgItemHelper.h"
#include "OptionsClass.h"
#include "SetDlgLists.h"

const ListBoxItem  CSetDlgLists::BgOper[] = {{eUpLeft,L"UpLeft"}, {eUpRight,L"UpRight"}, {eDownLeft,L"DownLeft"}, {eDownRight,L"DownRight"}, {eCenter,L"Center"}, {eStretch,L"Stretch"}, {eFit,L"Stretch-Fit"}, {eFill,L"Stretch-Fill"}, {eTile,L"Tile"}};

const ListBoxItem  CSetDlgLists::Modifiers[] = {{0,L" "}, {VK_LWIN,L"Win"},  {VK_APPS,L"Apps"},  {VK_CONTROL,L"Ctrl"}, {VK_LCONTROL,L"LCtrl"}, {VK_RCONTROL,L"RCtrl"},           {VK_MENU,L"Alt"}, {VK_LMENU,L"LAlt"}, {VK_RMENU,L"RAlt"},     {VK_SHIFT,L"Shift"}, {VK_LSHIFT,L"LShift"}, {VK_RSHIFT,L"RShift"}};
const ListBoxItem  CSetDlgLists::KeysHot[] = {{0,L""}, {VK_ESCAPE,L"Esc"}, {VK_DELETE,L"Delete"}, {VK_TAB,L"Tab"}, {VK_RETURN,L"Enter"}, {VK_SPACE,L"Space"}, {VK_BACK,L"Backspace"}, {VK_CANCEL,L"Break"}, {VK_PAUSE,L"Pause/Break"}, {VK_APPS,L"Apps"}, {VK_WHEEL_UP,L"Wheel Up"}, {VK_WHEEL_DOWN,L"Wheel Down"}, {VK_WHEEL_LEFT,L"Wheel Left"}, {VK_WHEEL_RIGHT,L"Wheel Right"}, {VK_LBUTTON,L"LButton"}, {VK_RBUTTON,L"RButton"}, {VK_MBUTTON,L"MButton"}};
const ListBoxItem  CSetDlgLists::Keys[] = {{0,L"<None>"}, {VK_LCONTROL,L"Left Ctrl"}, {VK_RCONTROL,L"Right Ctrl"}, {VK_LMENU,L"Left Alt"}, {VK_RMENU,L"Right Alt"}, {VK_LSHIFT,L"Left Shift"}, {VK_RSHIFT,L"Right Shift"}};
const ListBoxItem  CSetDlgLists::KeysAct[] = {{0,L"<No-Mod>"}, {VK_CONTROL,L"Ctrl"}, {VK_LCONTROL,L"Left Ctrl"}, {VK_RCONTROL,L"Right Ctrl"}, {VK_MENU,L"Alt"}, {VK_LMENU,L"Left Alt"}, {VK_RMENU,L"Right Alt"}, {VK_SHIFT,L"Shift"}, {VK_LSHIFT,L"Left Shift"}, {VK_RSHIFT,L"Right Shift"}};
const ListBoxItem  CSetDlgLists::KeysFilter[] = {{hkfv_All,L"All hotkeys"}, {hkfv_User,L"User defined"}, {hkfv_System,L"System"}, {hkfv_Macros,L"Macros"}};

const DWORD  CSetDlgLists::FSizesY[] = {8, 9, 10, 11, 12, 13, 14, 16, 18, 19, 20, 24, 26, 28, 30, 32, 34, 36, 40, 46, 50, 52, 72};
const DWORD  CSetDlgLists::FSizesX[] = {0, 8, 9, 10, 11, 12, 13, 14, 16, 18, 19, 20, 24, 26, 28, 30, 32, 34, 36, 40, 46, 50, 52, 72};
const DWORD  CSetDlgLists::FSizesSmall[] = {5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16, 18, 19, 20, 24, 26, 28, 30, 32};
const ListBoxItem  CSetDlgLists::CharSets[] = {
	{0,L"ANSI"}, {178,L"Arabic"}, {186,L"Baltic"}, {136,L"Chinese Big 5"}, {1,L"Default"}, {238,L"East Europe"},
	{134,L"GB 2312"}, {161,L"Greek"}, {177,L"Hebrew"}, {129,L"Hangul"}, {130,L"Johab"}, {77,L"Mac"}, {255,L"OEM"}, {204,L"Russian"}, {128,L"Shiftjis"},
	{2,L"Symbol"}, {222,L"Thai"}, {162,L"Turkish"}, {163,L"Vietnamese"}};

const ListBoxItem  CSetDlgLists::ClipAct[] = {{0,L"<None>"}, {1,L"Copy"}, {2,L"Paste"}, {3,L"Auto"}};

const ListBoxItem  CSetDlgLists::ColorIdx[]   = {{0,L" 0"}, {1,L" 1"}, {2,L" 2"}, {3,L" 3"}, {4,L" 4"}, {5,L" 5"}, {6,L" 6"}, {7,L" 7"}, {8,L" 8"}, {9,L" 9"}, {10,L"10"}, {11,L"11"}, {12,L"12"}, {13,L"13"}, {14,L"14"}, {15,L"15"}, {16,L"None"}};
const ListBoxItem  CSetDlgLists::ColorIdx16[] = {{0,L" 0"}, {1,L" 1"}, {2,L" 2"}, {3,L" 3"}, {4,L" 4"}, {5,L" 5"}, {6,L" 6"}, {7,L" 7"}, {8,L" 8"}, {9,L" 9"}, {10,L"10"}, {11,L"11"}, {12,L"12"}, {13,L"13"}, {14,L"14"}, {15,L"15"}};
const ListBoxItem  CSetDlgLists::ColorIdxSh[] = {{0,L"# 0"}, {1,L"# 1"}, {2,L"# 2"}, {3,L"# 3"}, {4,L"# 4"}, {5,L"# 5"}, {6,L"# 6"}, {7,L"# 7"}, {8,L"# 8"}, {9,L"# 9"}, {10,L"#10"}, {11,L"#11"}, {12,L"#12"}, {13,L"#13"}, {14,L"#14"}, {15,L"#15"}};
const ListBoxItem  CSetDlgLists::ColorIdxTh[] = {{0,L"# 0"}, {1,L"# 1"}, {2,L"# 2"}, {3,L"# 3"}, {4,L"# 4"}, {5,L"# 5"}, {6,L"# 6"}, {7,L"# 7"}, {8,L"# 8"}, {9,L"# 9"}, {10,L"#10"}, {11,L"#11"}, {12,L"#12"}, {13,L"#13"}, {14,L"#14"}, {15,L"#15"}, {16,L"Auto"}};
const ListBoxItem  CSetDlgLists::ThumbMaxZoom[] = {{100,L"100%"},{200,L"200%"},{300,L"300%"},{400,L"400%"},{500,L"500%"},{600,L"600%"}};

const ListBoxItem  CSetDlgLists::CRLF[] = {{0,L"CR+LF"}, {1,L"LF"}, {2,L"CR"}};
const ListBoxItem  CSetDlgLists::CopyFormat[] = {{0,L"Copy plain text only"}, {1,L"Copy HTML format"}, {2,L"Copy as (RAW) HTML"}, {3,L"Copy ANSI sequences"}};

const DWORD CSetDlgLists::SizeCtrlId[] = {tWndWidth, stWndWidth, tWndHeight, stWndHeight};
const DWORD CSetDlgLists::TaskCtrlId[] = {tCmdGroupName, tCmdGroupKey, cbCmdGroupKey, tCmdGroupGuiArg, tCmdGroupCommands, stCmdTaskAdd, cbCmdGroupApp, cbCmdTasksDir, cbCmdTasksParm, cbCmdTasksActive, cbCmdGrpDefaultNew, cbCmdGrpDefaultCmd, cbCmdGrpTaskbar, cbCmdGrpToolbar};
const DWORD CSetDlgLists::StatusColorIds[] = {stStatusColorBack, tc35, c35, stStatusColorLight, tc36, c36, stStatusColorDark, tc37, c37};
const DWORD CSetDlgLists::ImgCtrls[] = {tBgImage, bBgImage};
const DWORD CSetDlgLists::ExtendFonts[] = {lbExtendFontBoldIdx, lbExtendFontItalicIdx, lbExtendFontNormalIdx};

const ListBoxItem CSetDlgLists::TabBtnDblClickActions[] =
{// gpSet->nTabBtnDblClickAction
	{ TabBtnDblClick::NoAction,         L"No action"          },
	{ TabBtnDblClick::MaxRestoreWindow, L"Max/restore window" },
	{ TabBtnDblClick::MaxRestorePane,   L"Max/restore pane"   },
	{ TabBtnDblClick::CloseTab,         L"Close tab"          },
	{ TabBtnDblClick::RestartTab,       L"Restart tab"        },
	{ TabBtnDblClick::DuplicateTab,     L"Duplicate tab"      },
	{ TabBtnDblClick::RenameTab,        L"Rename tab"         },
};
const ListBoxItem CSetDlgLists::TabBarDblClickActions[] =
{// gpSet->nTabBarDblClickAction
	{ TabBarDblClick::NoAction,         L"No action"          },
	{ TabBarDblClick::Auto,             L"Auto"               },
	{ TabBarDblClick::MaxRestoreWindow, L"Max/restore window" },
	{ TabBarDblClick::OpenNewShell,     L"Open new shell"     },
};


uint CSetDlgLists::GetListItems(eFillListBoxItems eWhat, const ListBoxItem*& pItems)
{
	uint nCount = 0;

	#undef  LST_ENUM
	#define LST_ENUM(x) case e##x: pItems = x; nCount = LOWORD(countof(x)); break;

	switch (eWhat)
	{
	LST_ENUM(BgOper);
	LST_ENUM(Modifiers);
	LST_ENUM(KeysHot);
	LST_ENUM(Keys);
	LST_ENUM(KeysAct);
	LST_ENUM(KeysFilter);
	LST_ENUM(CharSets);
	LST_ENUM(ClipAct);
	LST_ENUM(ColorIdx);
	LST_ENUM(ColorIdx16);
	LST_ENUM(ColorIdxSh);
	LST_ENUM(ColorIdxTh);
	LST_ENUM(ThumbMaxZoom);
	LST_ENUM(CRLF);
	LST_ENUM(CopyFormat);
	LST_ENUM(TabBtnDblClickActions);
	LST_ENUM(TabBarDblClickActions);
	default:
		pItems = NULL;
	}

	#undef  LST_ENUM
	_ASSERTE((pItems!=NULL) && (nCount>0) && "Unsupported code");
	return nCount;
}

uint CSetDlgLists::GetListItems(eWordItems eWhat, const DWORD*& pItems)
{
	uint nCount = 0;

	#undef  LST_ENUM
	#define LST_ENUM(x) case e##x: pItems = x; nCount = LOWORD(countof(x)); break;

	switch (eWhat)
	{
	LST_ENUM(FSizesY);
	LST_ENUM(FSizesX);
	LST_ENUM(FSizesSmall);
	LST_ENUM(SizeCtrlId);
	LST_ENUM(TaskCtrlId);
	LST_ENUM(StatusColorIds);
	LST_ENUM(ImgCtrls);
	LST_ENUM(ExtendFonts);
	default:
		pItems = NULL;
	}

	#undef  LST_ENUM
	_ASSERTE((pItems!=NULL) && (nCount>0) && "Unsupported code");
	return nCount;
}

void CSetDlgLists::FillListBox(HWND hList, WORD nCtrlId, eFillListBoxItems eWhat)
{
	const ListBoxItem* Items;
	uint nItems = GetListItems(eWhat, Items);

	SendMessage(hList, CB_RESETCONTENT, 0, 0);

	for (uint i = 0; i < nItems; i++)
	{
		SendMessage(hList, CB_ADDSTRING, 0, (LPARAM)Items[i].sValue); //-V108
	}
}

void CSetDlgLists::FillListBoxItems(HWND hList, eFillListBoxItems eWhat, UINT& nValue, bool abExact)
{
	const ListBoxItem* Items;
	uint nItems = GetListItems(eWhat, Items);

	_ASSERTE(hList!=NULL);
	int num = -1;
	wchar_t szNumber[32];

	SendMessage(hList, CB_RESETCONTENT, 0, 0);

	for (uint i = 0; i < nItems; i++)
	{
		SendMessage(hList, CB_ADDSTRING, 0, (LPARAM) Items[i].sValue); //-V108

		if (Items[i].nValue == nValue) num = i; //-V108
	}

	if ((num != -1) || !abExact)
	{
		// если код не валиден
		if (num == -1)
		{
			nValue = 0;
			num = 0;
		}
		SendMessage(hList, CB_SETCURSEL, num, 0);
	}
	else
	{
		_wsprintf(szNumber, SKIPLEN(countof(szNumber)) L"%i", nValue);
		SelectStringExact(hList, 0, szNumber);
	}
}

void CSetDlgLists::FillListBoxItems(HWND hList, eFillListBoxItems eWhat, BYTE& nValue, bool abExact)
{
	UINT dwValue = nValue;
	FillListBoxItems(hList, eWhat, dwValue, abExact);

	if (abExact && (dwValue != nValue))
	{
		_ASSERTE(LOBYTE(dwValue) == dwValue);
		nValue = LOBYTE(dwValue);
	}
}

void CSetDlgLists::FillListBoxItems(HWND hList, eFillListBoxItems eWhat, const BYTE& nValue)
{
	UINT dwValue = nValue;
	FillListBoxItems(hList, eWhat, dwValue, true);
}

void CSetDlgLists::FillListBoxItems(HWND hList, eFillListBoxItems eWhat, const UINT& nValue)
{
	UINT dwValue = nValue;
	FillListBoxItems(hList, eWhat, dwValue, true);
}

void CSetDlgLists::FillListBoxItems(HWND hList, eWordItems eWhat, UINT& nValue, bool abExact)
{
	const DWORD* pnValues;
	uint nItems = GetListItems(eWhat, pnValues);

	_ASSERTE(hList!=NULL);
	uint num = 0;
	wchar_t szNumber[32];

	SendMessage(hList, CB_RESETCONTENT, 0, 0);

	for (uint i = 0; i < nItems; i++)
	{
		_wsprintf(szNumber, SKIPLEN(countof(szNumber)) L"%u", pnValues[i]);
		SendMessage(hList, CB_ADDSTRING, 0, (LPARAM)szNumber);

		if (pnValues[i] == nValue)
			num = i;
	}

	if (abExact)
	{
		_wsprintf(szNumber, SKIPLEN(countof(szNumber)) L"%u", nValue);
		SelectStringExact(hList, 0, szNumber);
	}
	else
	{
		if (!num)
			nValue = 0;  // если код неизвестен?
		SendMessage(hList, CB_SETCURSEL, num, 0);
	}
}

bool CSetDlgLists::GetListBoxItem(HWND hWnd, WORD nCtrlId, eFillListBoxItems eWhat, UINT& nValue)
{
	bool bFound = false;
	const ListBoxItem* Items;
	uint nItems = GetListItems(eWhat, Items);

	HWND hList = nCtrlId ? GetDlgItem(hWnd, nCtrlId) : hWnd;
	_ASSERTE(hList!=NULL);

	INT_PTR num = SendMessage(hList, CB_GETCURSEL, 0, 0);

	//int nKeyCount = countof(CSetDlgLists::szKeys);
	if (num>=0 && num<(int)nItems)
	{
		nValue = Items[num].nValue; //-V108
		bFound = true;
	}
	else
	{
		nValue = Items[0].nValue;

		if (num)  // Invalid index?
			SendMessage(hList, CB_SETCURSEL, num=0, 0);
	}

	return bFound;
}

bool CSetDlgLists::GetListBoxItem(HWND hWnd, WORD nCtrlId, eFillListBoxItems eWhat, BYTE& nValue)
{
	UINT dwValue = nValue;
	bool bFound = GetListBoxItem(hWnd, nCtrlId, eWhat, dwValue);
	_ASSERTE(dwValue==LOBYTE(dwValue));
	nValue = LOBYTE(dwValue);
	return bFound;
}

bool CSetDlgLists::GetListBoxItem(HWND hWnd, WORD nCtrlId, eWordItems eWhat, UINT& nValue)
{
	bool bFound = false;
	const DWORD* pnValues;
	uint nItems = GetListItems(eWhat, pnValues);

	HWND hList = nCtrlId ? GetDlgItem(hWnd, nCtrlId) : hWnd;
	_ASSERTE(hList!=NULL);

	INT_PTR num = SendMessage(hList, CB_GETCURSEL, 0, 0);

	//int nKeyCount = countof(CSetDlgLists::szKeys);
	if (num>=0 && num<(int)nItems)
	{
		nValue = pnValues[num]; //-V108
		bFound = true;
	}
	else
	{
		nValue = pnValues[0];

		if (num)  // Invalid index?
			SendMessage(hList, CB_SETCURSEL, num=0, 0);
	}

	return bFound;
}

bool CSetDlgLists::GetListBoxItem(HWND hWnd, WORD nCtrlId, eWordItems eWhat, BYTE& nValue)
{
	UINT dwValue = nValue;
	bool bFound = GetListBoxItem(hWnd, nCtrlId, eWhat, dwValue);
	_ASSERTE(dwValue==LOBYTE(dwValue));
	nValue = LOBYTE(dwValue);
	return bFound;
}

INT_PTR CSetDlgLists::GetSelectedString(HWND hParent, WORD nListCtrlId, wchar_t** ppszStr)
{
	INT_PTR nCur = SendDlgItemMessage(hParent, nListCtrlId, CB_GETCURSEL, 0, 0);
	INT_PTR nLen = (nCur >= 0) ? SendDlgItemMessage(hParent, nListCtrlId, CB_GETLBTEXTLEN, nCur, 0) : -1;
	if (!ppszStr)
		return nLen;

	if (nLen<=0)
	{
		if (*ppszStr) {free(*ppszStr); *ppszStr = NULL;}
	}
	else
	{
		wchar_t* pszNew = (TCHAR*)calloc(nLen+1, sizeof(TCHAR));
		if (!pszNew)
		{
			_ASSERTE(pszNew!=NULL);
		}
		else
		{
			SendDlgItemMessage(hParent, nListCtrlId, CB_GETLBTEXT, nCur, (LPARAM)pszNew);

			if (*ppszStr)
			{
				if (lstrcmp(*ppszStr, pszNew) == 0)
				{
					free(pszNew);
					return nLen; // Изменений не было
				}
			}

			if (nLen > (*ppszStr ? (INT_PTR)_tcslen(*ppszStr) : 0))
			{
				if (*ppszStr) free(*ppszStr);
				*ppszStr = pszNew; pszNew = NULL;
			}
			else
			{
				_wcscpy_c(*ppszStr, nLen+1, pszNew);
				SafeFree(pszNew);
			}
		}
	}

	return nLen;
}

int CSetDlgLists::SelectString(HWND hParent, WORD nCtrlId, LPCWSTR asText)
{
	if (!hParent)  // был ghOpWnd. теперь может быть вызван и для других диалогов!
		return -1;

#ifdef _DEBUG
	HWND hChild = GetDlgItem(hParent, nCtrlId);
	_ASSERTE(hChild!=NULL);
#endif
	// Осуществляет поиск по _началу_ (!) строки
	int nIdx = (int)SendDlgItemMessage(hParent, nCtrlId, CB_SELECTSTRING, -1, (LPARAM)asText);
	return nIdx;
}

// Если nCtrlId==0 - hParent==hList
int CSetDlgLists::SelectStringExact(HWND hParent, WORD nCtrlId, LPCWSTR asText)
{
	if (!hParent)  // был ghOpWnd. теперь может быть вызван и для других диалогов!
		return -1;

	HWND hList = nCtrlId ? GetDlgItem(hParent, nCtrlId) : hParent;
	_ASSERTE(hList!=NULL);

	int nIdx = SendMessage(hList, CB_FINDSTRINGEXACT, -1, (LPARAM)asText);

	if (nIdx < 0)
	{
		int nCount = SendMessage(hList, CB_GETCOUNT, 0, 0);
		wchar_t* pszNumEnd = NULL;
		int nNewVal = wcstol(asText, &pszNumEnd, 10), nCurVal;
		bool bUseNumCmp = (pszNumEnd && *pszNumEnd) && ((nNewVal != 0) || (lstrcmp(pszNumEnd, L"0") == 0));

		// For `Font Sizes` generally
		if (bUseNumCmp)
		{
			wchar_t temp[MAX_PATH] = {};

			for (int i = 0; i < nCount; i++)
			{
				if (!SendMessage(hList, CB_GETLBTEXT, i, (LPARAM)temp))
					break;

				nCurVal = _wtol(temp);

				if (nCurVal == nNewVal)
				{
					nIdx = i;
					break;
				}
				else if (nCurVal > nNewVal)
				{
					nIdx = SendMessage(hList, CB_INSERTSTRING, i, (LPARAM)asText);
					break;
				}
			}
		}

		if (nIdx < 0)
		{
			nIdx = SendMessage(hList, CB_INSERTSTRING, 0, (LPARAM)asText);
		}
	}

	if (nIdx >= 0)
		SendMessage(hList, CB_SETCURSEL, nIdx, 0);
	else
		SetWindowText(hList, asText);

	return nIdx;
}

void CSetDlgLists::EnableDlgItems(HWND hParent, const DWORD* pnCtrlIds, size_t nCount, bool bEnabled)
{
	for (;nCount-- && *pnCtrlIds; ++pnCtrlIds)
	{
		CDlgItemHelper::enableDlgItem(hParent, *pnCtrlIds, bEnabled);
	}
}

void CSetDlgLists::EnableDlgItems(HWND hParent, eWordItems eWhat, bool bEnabled)
{
	const DWORD* pnValues;
	uint nItems = GetListItems(eWhat, pnValues);
	EnableDlgItems(hParent, pnValues, nItems, bEnabled);
}

//void CSetDlgLists::EnableSizeCtrlItems(HWND hParent, BOOL bEnabled)
//{
//	EnableDlgItems(hParent, nSizeCtrlId, countof(nSizeCtrlId), bEnabled);
//}

//void CSetDlgLists::EnableStatusColorItems(HWND hParent, BOOL bEnabled)
//{
//	EnableDlgItems(hParent, nStatusColorIds, countof(nStatusColorIds), bEnabled);
//}

//void CSetDlgLists::FillListBoxWord(HWND hList, eFillListBoxItems eWhat)
//{
//}

/* *********************** */

//void CSetDlgLists::FillListBox(hDlg,nDlgID,Items,Value)
//{
//	FillListBoxItems(GetDlgItem(hDlg,nDlgID), countof(Items), Items, Value);
//}

//void CSetDlgLists::FillListBoxInt(hDlg,nDlgID,Values,Value)
//{
//	FillListBoxItems(GetDlgItem(hDlg,nDlgID), countof(Values), Values, Value);
//}

//void CSetDlgLists::FillListBoxByte(hDlg,nDlgID,Items,Value)
//{ \
//	DWORD dwVal = Value; \
//	FillListBoxItems(GetDlgItem(hDlg,nDlgID), countof(Items), Items, dwVal); \
//	Value = dwVal;
//}

//void CSetDlgLists::FillListBoxCharSet(hDlg,nDlgID,Value)
//{ \
//	u8 num = 4; /*индекс DEFAULT_CHARSET*/ \
//	for (size_t i = 0; i < countof(SettingsNS::CharSets); i++) \
//	{ \
//		SendDlgItemMessageW(hDlg, nDlgID, CB_ADDSTRING, 0, (LPARAM)SettingsNS::CharSets[i].sValue); \
//		if (SettingsNS::CharSets[i].nValue == Value) num = i; \
//	} \
//	SendDlgItemMessage(hDlg, nDlgID, CB_SETCURSEL, num, 0); \
//}

//void CSetDlgLists::FillListBoxTabDefaultClickAction(tt,hDlg,nDlgID,Value)
//{ \
//	u8 num = Value;  \
//	for (size_t i = 0; i < countof(SettingsNS::tt##DblClickActions); i++) \
//	{ \
//	SendDlgItemMessageW(hDlg, nDlgID, CB_ADDSTRING, 0, (LPARAM)SettingsNS::tt##DblClickActions[i].name); \
//		if (SettingsNS::tt##DblClickActions[i].value == num) num = i; \
//	} \
//	SendDlgItemMessage(hDlg, nDlgID, CB_SETCURSEL, num, 0); \
//}

//void CSetDlgLists::GetListBox(hDlg,nDlgID,Items,Value)
//{
//	GetListBoxItem(GetDlgItem(hDlg,nDlgID), countof(Items), Items, Value)
//}

//void CSetDlgLists::GetListBoxByte(hDlg,nDlgID,Items,Value)
//{ \
//	DWORD dwVal = Value; \
//	GetListBoxItem(GetDlgItem(hDlg,nDlgID), countof(Items), Items, dwVal); \
//	Value = dwVal;
//}

// return selected item (item with cursor)
// -1 on error or if no one was selected
int CSetDlgLists::GetListboxCurSel(HWND hDlg, UINT nCtrlID, bool bSingleReq /*= true*/)
{
	HWND hCtrl = nCtrlID ? GetDlgItem(hDlg, nCtrlID) : hDlg;
	if (!hCtrl)
		return -1;

	LRESULT lRc, lCount = SendMessage(hCtrl, LB_GETSELCOUNT, 0, 0);
	if (lCount < 0 /* == LB_ERR*/)
	{
		// Single-selection list box
		lRc = SendMessage(hCtrl, LB_GETCURSEL, 0, 0);
		return lRc;
	}
	else if (lCount == 0)
	{
		return -1; // No one was selected
	}

	if (bSingleReq && (lCount > 1))
	{
		return -1; // More than one item - unexpected
	}

	// Now we must check what item has a `cursor`
	lRc = SendMessage(hCtrl, LB_GETCARETINDEX, 0, 0);
	// And we have to check if it's selected
	lCount = SendMessage(hCtrl, LB_GETSEL, lRc, 0);
	if (lCount > 0)
		return lRc;

	// No one is selected
	return -1;
}

// returns
//  -1 - on errors
//  count of selected items on success
// rItems must be freed by `delete[]`
int CSetDlgLists::GetListboxSelection(HWND hDlg, UINT nCtrlID, int*& rItems)
{
	if (rItems)
	{
		_ASSERTE(rItems == NULL);
		//delete[] rItems;
		rItems = NULL;
	}

	HWND hCtrl = nCtrlID ? GetDlgItem(hDlg, nCtrlID) : hDlg;
	if (!hCtrl)
		return -1;

	LRESULT lRc, lCount = SendMessage(hCtrl, LB_GETSELCOUNT, 0, 0);
	if (lCount < 0 /* == LB_ERR*/)
	{
		// Single-selection list box
		lRc = SendMessage(hCtrl, LB_GETCURSEL, 0, 0);
		if (lRc < 0)
			return 0;
		rItems = new int[1];
		rItems[0] = lRc;
		return 1;
	}
	else if (lCount == 0)
	{
		return 0; // No one was selected
	}

	rItems = new int[lCount];
	lCount = SendMessage(hCtrl, LB_GETSELITEMS, lCount, (LPARAM)rItems);
	if (lCount <= 0)
	{
		delete[] rItems;
		rItems = NULL;
		return 0;
	}
	return lCount;
}

// For single-selection list boxes use simple SendDlgItemMessage(hWnd2, lbCmdTasks, LB_SETCURSEL, iCur, 0)
void CSetDlgLists::ListBoxMultiSel(HWND hDlg, UINT nCtrlID, int nItem)
{
	HWND hCtrl = nCtrlID ? GetDlgItem(hDlg, nCtrlID) : hDlg;
	if (!hCtrl)
		return;

	DEBUGTEST(LRESULT lRc =)
	SendMessage(hCtrl, LB_SETSEL, FALSE, -1);

	DEBUGTEST(lRc =)
	SendMessage(hCtrl, LB_SETCARETINDEX, nItem, FALSE);

	DEBUGTEST(lRc =)
	SendMessage(hCtrl, LB_SETSEL, TRUE, nItem);
}

void CSetDlgLists::FillCBList(HWND hCombo, bool abInitial, LPCWSTR* ppszPredefined, LPCWSTR pszUser)
{
	bool bUser = (pszUser != NULL);

	if (abInitial)
	{
		// Variants
		if (ppszPredefined)
		{
			for (LPCWSTR* ppsz = ppszPredefined; *ppsz; ppsz++)
			{
				SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)*ppsz);
				if (pszUser && (lstrcmp(*ppsz, pszUser) == 0))
					bUser = false; // This is our predefined string
			}
		}

		// Add user defined string to list
		if (bUser)
		{
			SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)pszUser);
		}
	}

	if (pszUser && (abInitial || GetWindowTextLength(hCombo) == 0))
	{
		SetWindowText(hCombo, pszUser);
	}
}
