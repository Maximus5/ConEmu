
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

#include <CommCtrl.h>
#include "LngDataEnum.h"
#include "Options.h"
#include "DlgItemHelper.h"
#include "SetDlgButtons.h"

//HWND hMain, hExt, hFar, hKeys, hTabs, hColors, hCmdTasks, hViews, hInfo, hDebug, hUpdate, hSelection;
enum TabHwndIndex
{
	thi_Fonts = 0,    // "Main"
	thi_SizePos,      //   "Size & Pos"
	thi_Appear,       //   "Appearance"
	thi_Quake,        //   "Quake style"
	thi_Backgr,       //   "Background"
	thi_Tabs,         //   "Tabs"
	thi_Confirm,      //   "Confirmations"
	thi_Taskbar,      //   "Task bar"
	thi_Update,       //   "Update"
	thi_Startup,      // "Startup"
	thi_Tasks,        //   "Tasks"
	thi_Environment,  //   "Environment"
	thi_Features,     // "Features"
	thi_Cursor,       //   "Text cursor"
	thi_Colors,       //   "Colors"
	thi_Transparent,  //   "Transparency"
	thi_Status,       //   "Status bar"
	thi_Apps,         //   "App distinct"
	thi_Integr,       // "Integration"
	thi_DefTerm,      //   "Default terminal"
	thi_Comspec,      //   "ComSpec"
	thi_ChildGui,     //   "Children GUI"
	thi_ANSI,         //   "ANSI execution"
	thi_Keys,         // "Keys & Macro"
	thi_Keyboard,     //   "Keyboard"
	thi_Mouse,        //   "Mouse"
	thi_MarkCopy,     //   "Mark & Copy"
	thi_Paste,        //   "Paste"
	thi_Hilight,      //   "Highlight"
	thi_Far,          // "Far Manager"
	thi_FarMacro,     //   "Far macros"
	thi_Views,        //   "Views"
	thi_Info,         // "Info"
	thi_Debug,        //   "Debug"
					  //
	thi_Last
};

#if 0
enum ConEmuSetupItemType
{
	sit_Bool = 1,
	sit_Byte = 2,
	sit_Char = 3,
	sit_Long = 4,
	sit_ULong = 5,
	sit_Rect = 6,
	sit_FixString = 7,
	sit_VarString = 8,
	sit_MSZString = 9,
	sit_FixData = 10,
	sit_Fonts = 11,
	sit_FontsAndRaster = 12,
};
struct ConEmuSetupItem
{
	//DWORD nDlgID; // ID диалога
	DWORD nItemID; // ID контрола в диалоге, 0 - последний элемент в списке
	ConEmuSetupItemType nDataType; // Тип данных

	void* pData; // Ссылка на элемент в gpSet
	size_t nDataSize; // Размер или maxlen, если pData фиксированный

	ConEmuSetupItemType nListType; // Тип данных в pListData
	const void* pListData; // Для DDLB - можно задать список
	size_t nListItems; // количество элементов в списке

	#ifdef _DEBUG
	BOOL bFound; // для отладки корректности настройки
	#endif

	//wchar_t sItemName[32]; // Имя элемента в настройке (reg/xml)
};
#endif

class CDpiForDialog;
class CDynDialog;
class CSetPgBase;

struct ConEmuSetupPages
{
	int              DialogID;     // Page Dialog ID (IDD_SPG_FONTS, ...)
	int              Level;        // 0, 1
	LngResources     PageNameRsrc; // Label in treeview
	TabHwndIndex     PageIndex;    // thi_Fonts, thi_SizePos, etc.
	CSetPgBase*    (*CreateObj)();
	bool             Collapsed;
	// Filled after creation
	HTREEITEM        hTI;
	#if 0
	ConEmuSetupItem* pItems;
	#endif
	HWND             hPage;
	wchar_t          PageName[64]; // Label in treeview
	CSetPgBase*      pPage;
};

class CSetPgBase
	: public CSetDlgButtons
{
protected:
	HWND mh_Dlg;
	HWND mh_Parent;
	bool mb_SkipSelChange;
	bool mb_DpiChanged;
	UINT mn_ActivateTabMsg;
	CDpiForDialog* mp_DpiAware;
	CDynDialog* mp_DynDialog;
	const CDpiForDialog* mp_ParentDpi;
	ConEmuSetupPages* mp_InfoPtr;

public:
	static bool mb_IgnoreEditChanged;

public:
	CSetPgBase();
	virtual ~CSetPgBase();

	virtual void InitObject(HWND ahParent, UINT nActivateTabMsg, const CDpiForDialog* apParentDpi, ConEmuSetupPages* apInfoPtr);

public:
	static INT_PTR CALLBACK pageOpProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam);
	static HWND CreatePage(ConEmuSetupPages* p, HWND ahParent, UINT anActivateTabMsg, const CDpiForDialog* apParentDpi);
	HWND Dlg() { return mh_Dlg; };

public:
	// Methods
	virtual LRESULT OnInitDialog(HWND hDlg, bool abInitial) = 0;
	virtual void OnPostLocalize(HWND hDlg) {};
	virtual TabHwndIndex GetPageType() = 0;
	virtual void ProcessDpiChange(const CDpiForDialog* apDpi);
	// Optional page procedure
	virtual INT_PTR PageDlgProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam) { return 0; };
	// Events
	virtual void OnDpiChanged(const CDpiForDialog* apDpi);
	virtual LRESULT OnEditChanged(HWND hDlg, WORD nCtrlId) { return 0; };
	virtual INT_PTR OnComboBox(HWND hDlg, WORD nCtrlId, WORD code);
	virtual INT_PTR OnCtlColorStatic(HWND hDlg, HDC hdc, HWND hCtrl, WORD nCtrlId);
	virtual INT_PTR OnButtonClicked(HWND hDlg, HWND hBtn, WORD nCtrlId);
	virtual INT_PTR OnSetCursor(HWND hDlg, HWND hCtrl, WORD nCtrlId, WORD nHitTest, WORD nMouseMsg);
	virtual bool QueryDialogCancel() { return true; };
	virtual bool SelectNextItem(bool bNext, bool bProcess) { return false; };

public:
	// Members
	static void setHotkeyCheckbox(HWND hDlg, WORD nCtrlId, int iHotkeyId, LPCWSTR pszFrom, LPCWSTR pszTo, UINT uChecked);
	static void setCtrlTitleByHotkey(HWND hDlg, WORD nCtrlId, int iHotkeyId, LPCWSTR pszFrom, LPCWSTR pszTo);
};
