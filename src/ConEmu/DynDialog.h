
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

#include <windows.h>
#include "../common/MArray.h"
#include "../common/MMap.h"

class CDynDialog
{
public:
	HWND   mh_Dlg;
	UINT   mn_DlgId;

public:
	CDynDialog(UINT nDlgId);
	virtual ~CDynDialog();

	// Modeless
	static CDynDialog* ShowDialog(UINT nDlgId, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
	// Modal
	static INT_PTR ExecuteDialog(UINT nDlgId, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);

	static CDynDialog* GetDlgClass(HWND hwndDlg);

	static bool DrawButton(WPARAM wID, DRAWITEMSTRUCT* pDraw);

	static void LocalizeDialog(HWND hDlg, UINT nTitleRsrcId = 0);
	static BOOL CALLBACK LocalizeControl(HWND hChild, LPARAM lParam);

public:
	// Methods
	#if 0
	INT_PTR ShowDialog(bool Modal, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
	#endif
	bool LoadTemplate();
	UINT GetFontPointSize();

	#ifdef _DEBUG
	static void UnitTests();
	#endif

public:
	static wchar_t Button[]; // = L"Button";
	static wchar_t Edit[]; // = L"Edit";
	static wchar_t Static[]; // = L"Static";
	static wchar_t ListBox[]; // = L"ListBox";
	static wchar_t ScrollBar[]; // = L"ScrollBar";
	static wchar_t ComboBox[]; // = L"ComboBox";
	static wchar_t Unknown[]; // = L"Unknown";

protected:
	// Dialog proc
	DLGPROC mp_DlgProc;
	static INT_PTR CALLBACK DynDialogBox(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static CDynDialog* mp_Creating;
	static MMap<HWND,CDynDialog*>* mp_DlgMap;
	void PrepareDlg(DLGPROC lpDialogFunc);

protected:
	// Types

	#if 0
	struct DLGTEMPLATE // Already defined in Win SDK
	{
		DWORD style;
		DWORD dwExtendedStyle;
		WORD  cdit;
		short x;
		short y;
		short cx;
		short cy;
		//sz_Or_Ord menu;
		//sz_Or_Ord windowClass;
		//WCHAR     title[titleLen];
		//#if (style & DS_SETFONT)
		//WORD      pointsize;
		//WCHAR     typeface[stringLen];
		//#endif
	};
	#endif

	struct DLGTEMPLATEEX
	{
		WORD        dlgVer;    // must be 0x0001
		WORD        signature; // must be 0xFFFF
		DWORD       helpID;
		DWORD       exStyle;
		DWORD       style;
		WORD        cDlgItems;
		short       x;
		short       y;
		short       cx;
		short       cy;
		//sz_Or_Ord menu;
		//sz_Or_Ord windowClass;
		//WCHAR     title[titleLen];
		//#if (style & (DS_SETFONT|DS_SHELLFONT))
		//WORD      pointsize;
		//WORD      weight;
		//BYTE      italic;
		//BYTE      charset;
		//WCHAR     typeface[stringLen];
		//#endif
	};

	#if 0
	// Each DLGITEMTEMPLATE structure must be aligned on DWORD boundary.
	struct DLGITEMTEMPLATE // Already defined in Win SDK
	{
		DWORD style;
		DWORD dwExtendedStyle;
		short x;
		short y;
		short cx;
		short cy;
		WORD  id;
	};
	#endif

	// Each DLGITEMTEMPLATEEX structure must be aligned on a DWORD boundary.
	struct DLGITEMTEMPLATEEX
	{
		DWORD       helpID;
		DWORD       exStyle;
		DWORD       style;
		short       x;
		short       y;
		short       cx;
		short       cy;
		DWORD       id;
		//sz_Or_Ord windowClass;
		//sz_Or_Ord title;
		//WORD      extraCount;
	};

	struct ItemInfo
	{
		UINT  ID;
		BOOL  ex;
		short *x, *y, *cx, *cy;
		LPCWSTR ItemType;
		LPDWORD pStyle;
		DWORD Style;
		union
		{
		DLGITEMTEMPLATE *ptr;
		DLGITEMTEMPLATEEX *ptrEx;
		};
	};

	// Members
	void* mlp_Template;
	DWORD mn_TemplateLength;
	LPWSTR mpsz_Title;
	LPWORD mp_pointsize;
	LPWSTR mpsz_TypeFace;
	MArray<ItemInfo> m_Items;

	// Methods
	bool ParseDialog(DWORD_PTR& data);
	bool ParseItem(DWORD_PTR& data);
	bool ParseDialogEx(DWORD_PTR& data);
	bool ParseItemEx(DWORD_PTR& data);
	LPWSTR SkipSz(DWORD_PTR& data);
	LPWSTR SkipSzOrOrd(DWORD_PTR& data);
	LPWSTR SkipSzOrAtom(DWORD_PTR& data);
	void FixupDialogSize();
	bool DrawButton(ItemInfo* pItem, DRAWITEMSTRUCT* pDraw);
};
