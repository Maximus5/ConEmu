
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

#pragma once

struct ListBoxItem
{
	DWORD nValue;
	LPCWSTR sValue;
};

enum KeysFilterValues
{
	hkfv_All,
	hkfv_User,
	hkfv_System,
	hkfv_Macros,
};

class CSetDlgLists
{
public:
	enum eFillListBoxItems
	{
		eBgOper, eCharSets,
		eModifiers, eKeysHot, eKeys, eKeysAct, eKeysFilter,
		eClipAct, eColorIdx, eColorIdx16, eColorIdxSh, eColorIdxTh,
		eThumbMaxZoom, eCRLF, eCopyFormat,
		eTabBtnDblClickActions, eTabBarDblClickActions,
	};
	enum eWordItems
	{
		eFSizesY, eFSizesX, eFSizesSmall,
		eSizeCtrlId, eTaskCtrlId, eStatusColorIds,
		eImgCtrls, eExtendFonts,
	};

protected:
	static const ListBoxItem  BgOper[]; // {{eUpLeft,L"UpLeft"}, {eUpRight,L"UpRight"}, {eDownLeft,L"DownLeft"}, {eDownRight,L"DownRight"}, {eCenter,L"Center"}, {eStretch,L"Stretch"}, {eFit,L"Stretch-Fit"}, {eFill,L"Stretch-Fill"}, {eTile,L"Tile"}};

	static const ListBoxItem  Modifiers[]; // {{0,L" "}, {VK_LWIN,L"Win"},  {VK_APPS,L"Apps"},  {VK_CONTROL,L"Ctrl"}, {VK_LCONTROL,L"LCtrl"}, {VK_RCONTROL,L"RCtrl"},           {VK_MENU,L"Alt"}, {VK_LMENU,L"LAlt"}, {VK_RMENU,L"RAlt"},     {VK_SHIFT,L"Shift"}, {VK_LSHIFT,L"LShift"}, {VK_RSHIFT,L"RShift"}};
	static const ListBoxItem  KeysHot[]; // {{0,L""}, {VK_ESCAPE,L"Esc"}, {VK_DELETE,L"Delete"}, {VK_TAB,L"Tab"}, {VK_RETURN,L"Enter"}, {VK_SPACE,L"Space"}, {VK_BACK,L"Backspace"}, {VK_PAUSE,L"Pause"}, {VK_WHEEL_UP,L"Wheel Up"}, {VK_WHEEL_DOWN,L"Wheel Down"}, {VK_WHEEL_LEFT,L"Wheel Left"}, {VK_WHEEL_RIGHT,L"Wheel Right"}, {VK_LBUTTON,L"LButton"}, {VK_RBUTTON,L"RButton"}, {VK_MBUTTON,L"MButton"}};
	static const ListBoxItem  Keys[]; // {{0,L"<None>"}, {VK_LCONTROL,L"Left Ctrl"}, {VK_RCONTROL,L"Right Ctrl"}, {VK_LMENU,L"Left Alt"}, {VK_RMENU,L"Right Alt"}, {VK_LSHIFT,L"Left Shift"}, {VK_RSHIFT,L"Right Shift"}};
	static const ListBoxItem  KeysAct[]; // {{0,L"<Always>"}, {VK_CONTROL,L"Ctrl"}, {VK_LCONTROL,L"Left Ctrl"}, {VK_RCONTROL,L"Right Ctrl"}, {VK_MENU,L"Alt"}, {VK_LMENU,L"Left Alt"}, {VK_RMENU,L"Right Alt"}, {VK_SHIFT,L"Shift"}, {VK_LSHIFT,L"Left Shift"}, {VK_RSHIFT,L"Right Shift"}};
	static const ListBoxItem  KeysFilter[]; // {{0,L"All hotkeys"}, {1,L"User defined"}, {2,L"System"}, {3,L"Macros"}};

	static const DWORD  FSizesY[]; // {0, 8, 9, 10, 11, 12, 13, 14, 16, 18, 19, 20, 24, 26, 28, 30, 32, 34, 36, 40, 46, 50, 52, 72};
	static const DWORD  FSizesX[]; // {0, 8, 9, 10, 11, 12, 13, 14, 16, 18, 19, 20, 24, 26, 28, 30, 32, 34, 36, 40, 46, 50, 52, 72};
	static const DWORD  FSizesSmall[]; // {5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16, 18, 19, 20, 24, 26, 28, 30, 32};
	static const ListBoxItem  CharSets[]; // {{0,L"ANSI"}, {178,L"Arabic"}, {186,L"Baltic"}, {136,L"Chinese Big 5"}, {1,L"Default"}, {238,L"East Europe"},
		//{134,L"GB 2312"}, {161,L"Greek"}, {177,L"Hebrew"}, {129,L"Hangul"}, {130,L"Johab"}, {77,L"Mac"}, {255,L"OEM"}, {204,L"Russian"}, {128,L"Shiftjis"},
		//{2,L"Symbol"}, {222,L"Thai"}, {162,L"Turkish"}, {163,L"Vietnamese"}};

	static const ListBoxItem  ClipAct[]; // {{0,L"<None>"}, {1,L"Copy"}, {2,L"Paste"}, {3,L"Auto"}};

	static const ListBoxItem  ColorIdx[];   // {{0,L" 0"}, {1,L" 1"}, {2,L" 2"}, {3,L" 3"}, {4,L" 4"}, {5,L" 5"}, {6,L" 6"}, {7,L" 7"}, {8,L" 8"}, {9,L" 9"}, {10,L"10"}, {11,L"11"}, {12,L"12"}, {13,L"13"}, {14,L"14"}, {15,L"15"}, {16,L"None"}};
	static const ListBoxItem  ColorIdx16[]; // {{0,L" 0"}, {1,L" 1"}, {2,L" 2"}, {3,L" 3"}, {4,L" 4"}, {5,L" 5"}, {6,L" 6"}, {7,L" 7"}, {8,L" 8"}, {9,L" 9"}, {10,L"10"}, {11,L"11"}, {12,L"12"}, {13,L"13"}, {14,L"14"}, {15,L"15"}};
	static const ListBoxItem  ColorIdxSh[]; // {{0,L"# 0"}, {1,L"# 1"}, {2,L"# 2"}, {3,L"# 3"}, {4,L"# 4"}, {5,L"# 5"}, {6,L"# 6"}, {7,L"# 7"}, {8,L"# 8"}, {9,L"# 9"}, {10,L"#10"}, {11,L"#11"}, {12,L"#12"}, {13,L"#13"}, {14,L"#14"}, {15,L"#15"}};
	static const ListBoxItem  ColorIdxTh[]; // {{0,L"# 0"}, {1,L"# 1"}, {2,L"# 2"}, {3,L"# 3"}, {4,L"# 4"}, {5,L"# 5"}, {6,L"# 6"}, {7,L"# 7"}, {8,L"# 8"}, {9,L"# 9"}, {10,L"#10"}, {11,L"#11"}, {12,L"#12"}, {13,L"#13"}, {14,L"#14"}, {15,L"#15"}, {16,L"Auto"}};
	static const ListBoxItem  ThumbMaxZoom[]; // {{100,L"100%"},{200,L"200%"},{300,L"300%"},{400,L"400%"},{500,L"500%"},{600,L"600%"}};

	static const ListBoxItem  CRLF[]; // {{0,L"CR+LF"}, {1,L"LF"}, {2,L"CR"}};
	static const ListBoxItem  CopyFormat[]; // {{0,L"Copy plain text only"}, {1,L"Copy HTML format"}, {2,L"Copy as (RAW) HTML"}, {3,L"Copy ANSI sequences"}};

	static const DWORD SizeCtrlId[]; // {tWndWidth, stWndWidth, tWndHeight, stWndHeight};
	static const DWORD TaskCtrlId[]; // {tCmdGroupName, tCmdGroupKey, cbCmdGroupKey, tCmdGroupGuiArg, tCmdGroupCommands, stCmdTaskAdd, cbCmdGroupApp, cbCmdTasksDir, cbCmdTasksParm, cbCmdTasksActive};
	static const DWORD StatusColorIds[]; // {stStatusColorBack, tc35, c35, stStatusColorLight, tc36, c36, stStatusColorDark, tc37, c37};
	static const DWORD ImgCtrls[]; // {tBgImage, bBgImage};
	static const DWORD ExtendFonts[]; // {lbExtendFontBoldIdx, lbExtendFontItalicIdx, lbExtendFontNormalIdx};

	static const ListBoxItem TabBtnDblClickActions[];
	static const ListBoxItem TabBarDblClickActions[];

public:
	static uint GetListItems(eFillListBoxItems eWhat, const ListBoxItem*& pItems);
	static uint GetListItems(eWordItems eWhat, const DWORD*& pItems);

	static void FillListBox(HWND hList, WORD nCtrlId, eFillListBoxItems eWhat);
	static void FillListBoxItems(HWND hList, eFillListBoxItems eWhat, UINT& nValue, bool abExact);
	static void FillListBoxItems(HWND hList, eFillListBoxItems eWhat, BYTE& nValue, bool abExact);
	static void FillListBoxItems(HWND hList, eFillListBoxItems eWhat, const UINT& nValue);
	static void FillListBoxItems(HWND hList, eFillListBoxItems eWhat, const BYTE& nValue);
	static void FillListBoxItems(HWND hList, eWordItems eWhat, UINT& nValue, bool abExact);
	static void FillCBList(HWND hCombo, bool abInitial, LPCWSTR* ppszPredefined, LPCWSTR pszUser);

	static bool GetListBoxItem(HWND hWnd, WORD nCtrlId, eFillListBoxItems eWhat, UINT& nValue);
	static bool GetListBoxItem(HWND hWnd, WORD nCtrlId, eFillListBoxItems eWhat, BYTE& nValue);
	static bool GetListBoxItem(HWND hWnd, WORD nCtrlId, eWordItems eWhat, UINT& nValue);
	static bool GetListBoxItem(HWND hWnd, WORD nCtrlId, eWordItems eWhat, BYTE& nValue);

	static INT_PTR GetSelectedString(HWND hParent, WORD nListCtrlId, wchar_t** ppszStr);
	static int SelectString(HWND hParent, WORD nCtrlId, LPCWSTR asText);
	static int SelectStringExact(HWND hParent, WORD nCtrlId, LPCWSTR asText);

	static void EnableDlgItems(HWND hParent, const DWORD* pnCtrlIds, size_t nCount, bool bEnabled);
	static void EnableDlgItems(HWND hParent, eWordItems eWhat, bool bEnabled);

	// List box with extended selection styles
	static int GetListboxCurSel(HWND hDlg, UINT nCtrlID, bool bSingleReq = true);
	static int GetListboxSelection(HWND hDlg, UINT nCtrlID, int*& rItems);
	static void ListBoxMultiSel(HWND hDlg, UINT nCtrlID, int nItem);
};
