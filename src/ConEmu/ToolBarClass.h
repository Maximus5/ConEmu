
/*
Copyright (c) 2012-2017 Maximus5
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

#include "Header.h"
#include <commctrl.h> // tooltips
#include "../common/MArray.h"

enum ToolInfoStyles
{
	TIS_SEPARATOR    = 0x0001,
	TIS_DROPDOWN     = 0x0002,
	TIS_CHECKED      = 0x0004,
	TIS_DRAWCALLBACK = 0x0008,
	TIS_TIPCALLBACK  = 0x0010,
	TIS_PRESSED      = 0x0020, // Передается только в OnToolBarDraw если кнопка сейчас нажата мышкой
	TIS_VISIBLE      = 0x0040, // Pane-only style
};

typedef void (*ToolbarCommand_f)(LPARAM lParam, int anPaneID, int anCmd, bool abArrow, POINT ptWhere, RECT rcBtnRect);
typedef void (*ToolbarMenu_f)(LPARAM lParam, int anPaneID, int anCmd, POINT ptWhere, RECT rcBtnRect);
typedef void (*ToolBarDraw_f)(LPARAM lParam, const PaintDC& dc, const RECT& rc, int nPane, int nCmd, DWORD nFlags);

class CToolBarClass
{
private:
	struct ToolId
	{
		union {
			struct {
				int       nPane; // ИД панели
				int       nCmd;  // ID кнопки
			};
			__int64 nID; // для удобства
		};
	};
	struct ToolInfo
	{
		ToolId    ID;     // ИД панели/кнопки
		DWORD     nFlags; // enum ToolInfoStyles
		RECT      rcBmp;  // координаты кнопки в hBmp;
		wchar_t*  sTip;   // Tooltip
		int       nTipCchMax;
		// Далее - используется при отрисовке
		RECT      rcTool;
		RECT      rcArrow;
	};
	struct PaneInfo
	{
		int       nPane;        // ИД панели
		DWORD     nStyles;      // Показывать или спрятать эту панель (TIS_VISIBLE)
		int       nPriority;    // При нехватке места - остаются имеющие высший nPriority. Пока не реализовано.
		int       nTotalWidth;  // общая ширина "панели"
		int       nCount, nMax; // Количество кнопок, и размер выделенных ячеек
		HBITMAP   hBmp;
		ToolInfo* pTool;
		// Далее - используется при отрисовке
		BOOL      bVisible;
		RECT      rcPane;
		// Callbacks
		LPARAM    lParam;
		ToolbarCommand_f OnToolbarCommand;
		ToolbarMenu_f OnToolbarMenu;
		ToolBarDraw_f OnToolBarDraw;
	};
	int mn_LastCmdID;
	ToolId m_Hover, m_Clicked;
	bool mb_ArrowClicked;
	int mn_MaxPaneId;
	int mn_SeparatorWidth;
	int mn_PaddingWidth;
	int mn_DropArrowWidth; // размер поля со стрелкой
	int mn_ToolWidth, mn_ToolHeight;
	MArray<PaneInfo*> m_Panels;
	MSectionSimple m_CS;
	bool mb_Locked;

	HPEN mh_LitePen; //, mh_DarkPen;
	HBRUSH mh_HoverBrush, mh_CheckedBrush;
	HDC mh_PaneDC;
	bool mb_Aero;
	void PaintPane(const PaintDC& dc, const RECT& rcTB, PaneInfo* p, int x, int y);
	void PaintTool(const PaintDC& dc, const RECT& rcTB, PaneInfo* p, ToolInfo* t, int x, int y);
	void PaintSeparator(const PaintDC& dc, const RECT& rcTB, int x, int y, int h);
	ToolInfo* AddTool(PaneInfo* aPane, int anCmd, DWORD anFlags, const RECT& rcBmp, LPCWSTR asTip);
	void SetToolTip(ToolInfo* apTool, LPCWSTR asTip);
	PaneInfo* GetPaneById(int anPane);
	ToolInfo* GetToolById(PaneInfo* aPane, int anCmd);

	void Invalidate();

public:
	CToolBarClass();
	virtual ~CToolBarClass();

	int GetWidth(int anMaxWidth = -1);
	int GetHeight();

	void Lock();
	int CreatePane(int anPane, int anPriority, bool abShow, HBITMAP ahBmp, LPARAM lParam, ToolbarCommand_f OnToolbarCommand, ToolbarMenu_f OnToolbarMenu = NULL, ToolBarDraw_f OnToolBarDraw = NULL);
	bool ShowPane(int anPane, bool abShow);
	bool DeletePane(int anPane);
	int AddTool(int anPane, int anCmd, const RECT& rcBmp, DWORD anFlags, LPCWSTR asTip); // Returns new CmdID
	int SetToolBmp(int anPane, int anCmd, const RECT& rcBmp);
	//int SetToolTip(int anPane, int anCmd, LPCWSTR asTip);
	int CheckTool(int anPane, int anCmd, BOOL abChecked);
	BOOL AddSeparator(int anPane);
	void Commit();

	void Paint(const PaintDC& dc, const RECT& rcTB);

	void UnHover();
	bool MouseEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, POINT ptClient, LRESULT &lResult);
	//void* ToolFromPoint(POINT pt);
};
