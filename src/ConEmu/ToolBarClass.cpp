
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

#include "header.h"
#include "ConEmu.h"
#include "ToolBarClass.h"
#include "TabBarEx.h"
#include "Options.h"
#include "../common/WObjects.h"
#include "resource.h"

#ifdef _DEBUG
	//#define DEBUG_SHOW_RECT
	#undef DEBUG_SHOW_RECT
#else
	#undef DEBUG_SHOW_RECT
#endif

CToolBarClass::CToolBarClass()
{
	InitializeCriticalSection(&m_CS);
	mb_Locked = false;
	mn_SeparatorWidth = 2;
	mn_PaddingWidth = 4;
	mn_DropArrowWidth = 11;
	mh_LitePen = /*mh_DarkPen =*/ NULL;
	mh_HoverBrush = mh_CheckedBrush = NULL;
	mn_ToolWidth = mn_ToolHeight = 14;
	m_Hover.nPane = m_Hover.nCmd = -1;
	mn_MaxPaneId = 1000;
	mn_LastCmdID = 1000;
	mh_PaneDC = NULL;
}

CToolBarClass::~CToolBarClass()
{
	EnterCriticalSection(&m_CS);

	// Освободить память и ресурсы
	for (INT_PTR i = m_Panels.size(); i--;)
	{
		PaneInfo* pane = m_Panels[i];
		if (!pane)
		{
			_ASSERTE(pane != NULL);
			continue;
		}

		if (pane->hBmp)
		{
			DeleteObject(pane->hBmp);
		}

		if (pane->pTool)
		{
			for (int t = 0; t < pane->nCount; t++)
			{
				if (pane->pTool[t].sTip)
					free(pane->pTool[t].sTip);
			}

			free(pane->pTool);
		}

		free(pane);
	}

	m_Panels.clear();

	LeaveCriticalSection(&m_CS);
	DeleteCriticalSection(&m_CS);
}

int CToolBarClass::GetWidth(int anMaxWidth /*= -1*/)
{
	int nWidth = 0;
	EnterCriticalSection(&m_CS);
	bool lbShown = false;

	for (INT_PTR i = 0; i < m_Panels.size(); i++)
	{
		if ((anMaxWidth >= 0) && (nWidth + mn_SeparatorWidth) > anMaxWidth)
			break; // больше некуда

		PaneInfo* pane = m_Panels[i];

		bool lbFit;
		if ((anMaxWidth >= 0) && pane->nCount && pane->pTool && pane->nTotalWidth && (pane->nStyles & TIS_VISIBLE))
			lbFit = ((nWidth + (lbShown ? mn_SeparatorWidth : 0) + pane->nTotalWidth) <= anMaxWidth);
		else
			lbFit = false;

		if (lbFit)
		{
			nWidth += (lbShown ? mn_SeparatorWidth : 0) + pane->nTotalWidth;
			lbShown = true;
		}
	}

	LeaveCriticalSection(&m_CS);
	return nWidth;
}

int CToolBarClass::GetHeight()
{
	return mn_ToolHeight+4;
}

void CToolBarClass::Lock()
{
	EnterCriticalSection(&m_CS);
	mb_Locked = true;
}

int CToolBarClass::CreatePane(int anPane, int anPriority, bool abShow, HBITMAP ahBmp, LPARAM lParam, ToolbarCommand_f OnToolbarCommand, ToolbarMenu_f OnToolbarMenu /*= NULL*/, ToolBarDraw_f OnToolBarDraw /*= NULL*/)
{
	TODO("anPriority is ignored still");

	if (!mb_Locked)
	{
		_ASSERTE(mb_Locked && "Must be locked before modifications");
		return -1;
	}

	_ASSERTE(OnToolbarCommand!=NULL);
	_ASSERTE(ahBmp!=NULL);

	if (anPane)
	{
		for (INT_PTR i = m_Panels.size(); i--;)
		{
			PaneInfo* pane = m_Panels[i];

			if (!pane)
			{
				_ASSERTE(pane != NULL);
				continue;
			}

			if (pane->nPane == anPane)
			{
				_ASSERTE(FALSE && "Pane with this ID already exists!");
				return -1;
			}
		}
	}
	else
	{
		anPane = ++mn_MaxPaneId;
	}

	PaneInfo* pane = (PaneInfo*)calloc(sizeof(*pane),1);
	if (!pane)
	{
		_ASSERTE(pane!=NULL);
		return -1;
	}

	pane->nPane = anPane;
	pane->nStyles = abShow ? TIS_VISIBLE : 0;
	pane->nPriority = anPriority;
	pane->hBmp = ahBmp;
	pane->nMax = 16;
	pane->pTool = (ToolInfo*)calloc(pane->nMax, sizeof(*pane->pTool));
	pane->nTotalWidth = mn_PaddingWidth;

	pane->lParam = lParam;
	pane->OnToolbarCommand = OnToolbarCommand;
	pane->OnToolbarMenu = OnToolbarMenu;
	pane->OnToolBarDraw = OnToolBarDraw;

	m_Panels.push_back(pane);

	return pane->nPane;
}

bool CToolBarClass::ShowPane(int anPane, bool abShow)
{
	if (!mb_Locked)
	{
		_ASSERTE(mb_Locked && "Must be locked before modifications");
		return false;
	}

	bool lbChanged = false;

	for (INT_PTR i = m_Panels.size(); i--;)
	{
		PaneInfo* pane = m_Panels[i];

		if (!pane)
		{
			_ASSERTE(pane != NULL);
			continue;
		}

		if (pane->nPane == anPane)
		{
			lbChanged = (((pane->nStyles & TIS_VISIBLE) == TIS_VISIBLE) != abShow);
			if (lbChanged)
			{
				if (abShow)
					pane->nStyles |= TIS_VISIBLE;
				else
					pane->nStyles &= ~TIS_VISIBLE;
			}
			break;
		}
	}

	return lbChanged;
}

bool CToolBarClass::DeletePane(int anPane)
{
	if (!mb_Locked)
	{
		_ASSERTE(mb_Locked && "Must be locked before modifications");
		return false;
	}

	bool lbFound = false;

	for (INT_PTR i = m_Panels.size(); i--;)
	{
		PaneInfo* pane = m_Panels[i];

		if (!pane)
		{
			_ASSERTE(pane != NULL);
			continue;
		}

		if (pane->nPane == anPane)
		{
			lbFound = true;

			if (pane->hBmp)
				DeleteObject(pane->hBmp);

			if (pane->pTool)
			{
				for (int t = 0; t < pane->nCount; t++)
				{
					if (pane->pTool[t].sTip)
						free(pane->pTool[t].sTip);
				}
				free(pane->pTool);
			}

			m_Panels.erase(i);
			free(pane);

			break;
		}
	}

	return lbFound;
}

// Returns CmdID
int CToolBarClass::AddTool(int anPane, int anCmd, const RECT& rcBmp, DWORD anFlags, LPCWSTR asTip)
{
	if (!mb_Locked)
	{
		_ASSERTE(mb_Locked && "Must be locked before modifications");
		return -1;
	}

	PaneInfo* p = GetPaneById(anPane);
	if (!p)
		return -1;

	if (anCmd)
	{
		ToolInfo* t = GetToolById(p, anCmd);
		if (t != NULL)
		{
			_ASSERTE(FALSE && "Button with this ID already exists!");
			return -1;
		}
	}
	else
	{
		anCmd = ++mn_LastCmdID;
	}

	ToolInfo* t = AddTool(p, anCmd, anFlags, rcBmp, asTip);
	if (!t)
		return -1;
	return t->ID.nCmd;
}

//int CToolBarClass::SetToolTip(int anPane, int anCmd, LPCWSTR asTip)
//{
//	if (!mb_Locked)
//	{
//		_ASSERTE(mb_Locked && "Must be locked before modifications");
//		return -1;
//	}
//	if (anCmd == -1)
//		return -1;
//
//	PaneInfo* p = GetPaneById(anPane);
//	if (!p)
//		return -1;
//
//	ToolInfo* t = GetToolById(p, anCmd);
//	if (t)
//	{
//		SetToolTip(t, asTip);
//		return t->ID.nCmd;
//	}
//
//	return -1;
//}

int CToolBarClass::CheckTool(int anPane, int anCmd, BOOL abChecked)
{
	if (!mb_Locked)
	{
		_ASSERTE(mb_Locked && "Must be locked before modifications");
		return -1;
	}

	PaneInfo* p = GetPaneById(anPane);
	if (!p)
		return -1;

	ToolInfo* t = GetToolById(p, anCmd);
	if (t)
	{
		if (abChecked)
			t->nFlags |= TIS_CHECKED;
		else
			t->nFlags &= ~TIS_CHECKED;
		return t->ID.nCmd;
	}

	return -1;
}

int CToolBarClass::SetToolBmp(int anPane, int anCmd, const RECT& rcBmp)
{
	if (!mb_Locked)
	{
		_ASSERTE(mb_Locked && "Must be locked before modifications");
		return -1;
	}

	PaneInfo* p = GetPaneById(anPane);
	if (!p)
		return -1;

	ToolInfo* t = GetToolById(p, anCmd);
	if (t)
	{
		t->rcBmp = rcBmp;
		return t->ID.nCmd;
	}

	return -1;
}

BOOL CToolBarClass::AddSeparator(int anPane)
{
	if (!mb_Locked)
	{
		_ASSERTE(mb_Locked && "Must be locked before modifications");
		return FALSE;
	}
	PaneInfo* p = GetPaneById(anPane);
	if (!p)
		return FALSE;

	RECT rcNil = {};
	AddTool(p, -1, TIS_SEPARATOR, rcNil, NULL);
	return TRUE;
}

void CToolBarClass::Commit()
{
	if (!mb_Locked)
	{
		_ASSERTE(mb_Locked && "Must be locked before modifications");
		return;
	}
	LeaveCriticalSection(&m_CS);

	// Если надо - обновиться/перерисоваться
	if (ghWnd && gpSet->isTabs && gpSet->isMultiShowButtons)
	{
		Invalidate();
	}
}

void CToolBarClass::Invalidate()
{
	TODO("Обновить Frame/Caption?");
	// -- gpConEmu->RedrawFrame();
	InvalidateRect(ghWnd, NULL, TRUE);
}

void CToolBarClass::Paint(const PaintDC& dc, const RECT& rcTB)
{
	if (!dc.hDC)
		return;

	_ASSERTE(gpConEmu->isMainThread());

	int nWidth = 0;
	int nMaxWidth = rcTB.right - rcTB.left + 1;
	if (nMaxWidth < 2)
		return;

	TODO("Toolbars otherwhere of tabs?");
	mb_Aero = (gpConEmu->DrawType() >= fdt_Aero);

	EnterCriticalSection(&m_CS);
	BOOL lbShown = FALSE;

	//mh_LitePen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));
	//mh_DarkPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));

	mh_LitePen = CreatePen(PS_SOLID, 1, RGB(122,177,232));
	mh_HoverBrush = CreateSolidBrush(RGB(213,231,248));
	mh_CheckedBrush = CreateSolidBrush(RGB(184,216,249));

	HPEN hOldPen = (HPEN)SelectObject(dc.hDC, mh_LitePen);
	HPEN hOldBrush = (HPEN)SelectObject(dc.hDC, mh_HoverBrush);

	#ifdef DEBUG_SHOW_RECT
		FillRect(dc.hDC, &rcTB, (HBRUSH)GetStockObject(WHITE_BRUSH));
		//MoveToEx(hdc, rcTB.left-1, rcTB.top-1, NULL);
		//LineTo(hdc, rcTB.right+1, rcTB.top-1);
		//LineTo(hdc, rcTB.right+1, rcTB.bottom+1);
		//LineTo(hdc, rcTB.left-1, rcTB.bottom+1);
		//LineTo(hdc, rcTB.left-1, rcTB.top-1);
	#endif

	for (INT_PTR i = 0; i < m_Panels.size(); i++)
	{
		if ((nWidth + mn_SeparatorWidth) > nMaxWidth)
			break; // больше некуда

		PaneInfo* pane = m_Panels[i];

		if (!pane->nCount || !pane->nTotalWidth || !pane->pTool || !(pane->nStyles & TIS_VISIBLE))
		{
			pane->bVisible = FALSE;
			continue;
		}

		BOOL lbFit = ((nWidth + (lbShown ? mn_SeparatorWidth : 0) + pane->nTotalWidth) <= nMaxWidth);
		if (lbFit)
		{
			if (lbShown)
			{
				PaintSeparator(dc, rcTB, rcTB.left+nWidth, rcTB.top, mn_ToolHeight);
				nWidth += mn_SeparatorWidth;
			}
			else
			{
				lbShown = TRUE;
			}
			pane->bVisible = TRUE;
			PaintPane(dc, rcTB, pane, rcTB.left+nWidth, rcTB.top);
			nWidth += pane->nTotalWidth;
		}
		else
		{
			pane->bVisible = FALSE;
		}

		//nWidth += mn_PaddingWidth; // ???
	}

	SelectObject(dc.hDC, hOldPen);
	SelectObject(dc.hDC, hOldBrush);

	//SelectObject(hdc, hOldPen);
	//DeleteObject(mh_DarkPen);
	//mh_DarkPen = NULL;
	DeleteObject(mh_LitePen);
	mh_LitePen = NULL;
	DeleteObject(mh_HoverBrush);
	mh_HoverBrush = NULL;
	DeleteObject(mh_CheckedBrush);
	mh_CheckedBrush = NULL;

	LeaveCriticalSection(&m_CS);
}

void CToolBarClass::PaintPane(const PaintDC& dc, const RECT& rcTB, PaneInfo* p, int x, int y)
{
	if (!p || (p->nCount < 1) || (p->nTotalWidth < 1) || !p->pTool)
	{
		_ASSERTE(!(!p || (p->nCount < 1) || (p->nTotalWidth < 1) || !p->pTool));
		return;
	}

	HBITMAP hOld = NULL;
	if (p->hBmp)
	{
		mh_PaneDC = CreateCompatibleDC(dc.hDC);
		hOld = (HBITMAP)SelectObject(mh_PaneDC, p->hBmp);
	}

	p->rcPane.left = x;
	p->rcPane.top = y;
	p->rcPane.bottom = y + mn_ToolHeight;

	x += mn_PaddingWidth;

	for (int t = 0; t < p->nCount; t++)
	{
		if (p->pTool[t].nFlags & TIS_SEPARATOR)
		{
			PaintSeparator(dc, rcTB, x, y, mn_ToolHeight);
			x += mn_SeparatorWidth + mn_PaddingWidth;
		}
		else
		{
			PaintTool(dc, rcTB, p, p->pTool+t, x, y);
			x += (p->pTool[t].rcBmp.right - p->pTool[t].rcBmp.left + 1) + mn_PaddingWidth
				+ ((p->pTool[t].nFlags & TIS_DROPDOWN) ? mn_DropArrowWidth : 0);
		}
	}

	p->rcPane.right = x;

	if (hOld)
		SelectObject(mh_PaneDC, hOld);
	DeleteDC(mh_PaneDC);
	mh_PaneDC = NULL;
}

void CToolBarClass::PaintTool(const PaintDC& dc, const RECT& rcTB, PaneInfo* p, ToolInfo* t, int x, int y)
{
	int x1 = t->rcTool.left = x;
	int y1 = t->rcTool.top = y;
	int x2 = t->rcTool.right = x + t->rcBmp.right-t->rcBmp.left+1;
	int y2 = t->rcTool.bottom = y + t->rcBmp.bottom-t->rcBmp.top+1;
	// Если нужно
	int x11, x21;
	if (t->nFlags & TIS_DROPDOWN)
	{
		x11 = x2 + 3;
		x21 = x2 + mn_DropArrowWidth;
		t->rcArrow.left = x2 /*+ 1*/;
		t->rcArrow.right = x21;
		t->rcArrow.top = y1;
		t->rcArrow.bottom = y2;
	}
	else
	{
		t->rcArrow.left = t->rcArrow.right = t->rcArrow.top = t->rcArrow.bottom = 0;
	}

	bool lbHover = (m_Hover.nID == t->ID.nID);
	bool lbClicked = lbHover && (m_Clicked.nID == t->ID.nID);
	bool lbChecked = ((t->nFlags & TIS_CHECKED) != FALSE);


	if (lbHover || lbChecked)
	{
		//POINT ptFrame[] = {
		//	{x1, y1-2}, {x2-1, y1-2}, {x2+1, y1},
		//	{x2+1, y2-1}, {x2-1, y2+1}, {x1, y2+1},
		//	{x1-2, y2-1}, {x1-2, y1}, {x1, y1-2}
		//};

		HPEN hp = (HPEN)SelectObject(dc.hDC, mh_LitePen /*lbClicked ? mh_LitePen : mh_DarkPen*/);
		HBRUSH hb = (HBRUSH)SelectObject(dc.hDC, lbChecked ? mh_CheckedBrush : mh_HoverBrush); // (lbChecked ? SelectObject(dc.hDC, GetSysColorBrush(COLOR_3DSHADOW)) : NULL);
		//if (lbChecked)
		//	Polygon(dc.hDC, ptFrame, countof(ptFrame));
		//else
		//	Polyline(dc.hDC, ptFrame, countof(ptFrame));
		::Rectangle(dc.hDC, x1-3, y1-3, x2+3, y2+3);

		RECT rcAlpha = {x1-3, y1-3, x2+4, y2+4};

		// Отрисовка рамки вокруг стрелки
		if (t->nFlags & TIS_DROPDOWN)
		{
			//POINT ptFrameDrop[] = {
			//	{x11, y1-2}, {x21-1, y1-2}, {x21+1, y1},
			//	{x21+1, y2-1}, {x21-1, y2+1}, {x11, y2+1},
			//	{x11-2, y2-1}, {x11-2, y1}, {x11, y1-2}
			//};
			//Polyline(dc.hDC, ptFrameDrop, countof(ptFrameDrop));
			::Rectangle(dc.hDC, x11-2, y1-3, x21+3, y2+3);
			rcAlpha.right = x21+4;
		}

		if (mb_Aero)
			gpConEmu->BufferedPaintSetAlpha(dc, &rcAlpha, 255);

		SelectObject(dc.hDC, hp);
		//if (lbChecked)
		SelectObject(dc.hDC, hb);
	}


	// Отрисовка стрелки
	if (t->nFlags & TIS_DROPDOWN)
	{
		int xs1 = x11+(lbClicked?2:1);
		int ys1 = y+(lbClicked?7:6);
		POINT ptArrow[] = {
			{xs1, ys1}, {xs1+4, ys1},
			{xs1+2, ys1+2}, {xs1, ys1}
		};

		HPEN hp = (HPEN)SelectObject(dc.hDC, GetStockObject(mb_Aero ? WHITE_PEN : BLACK_PEN));
		HBRUSH hb = (HBRUSH)SelectObject(dc.hDC, GetStockObject(mb_Aero ? WHITE_BRUSH : BLACK_BRUSH));

		Polygon(dc.hDC, ptArrow, countof(ptArrow));

		SelectObject(dc.hDC, hp);
		SelectObject(dc.hDC, hb);
	}


	// Отрисовка собственно картинки-кнопки
	if (t->nFlags & TIS_DRAWCALLBACK)
	{
		RECT rcPaint = {x+(((lbClicked&&!mb_ArrowClicked)||lbChecked)?1:0), y};
		rcPaint.right = rcPaint.left + t->rcBmp.right-t->rcBmp.left;
		rcPaint.bottom = rcPaint.top + t->rcBmp.bottom-t->rcBmp.top;
		if (p && p->OnToolBarDraw)
		{
			p->OnToolBarDraw(p->lParam, dc, rcPaint, t->ID.nPane, t->ID.nCmd,
				t->nFlags|((lbClicked&&!mb_ArrowClicked)?TIS_PRESSED:0));
		}
	}
	else if (mh_PaneDC)
	{
		BitBlt(dc.hDC, x+((lbClicked&&!mb_ArrowClicked)?1:0), y,
			t->rcBmp.right-t->rcBmp.left+1, t->rcBmp.bottom-t->rcBmp.top+1,
			mh_PaneDC, t->rcBmp.left, t->rcBmp.top, SRCCOPY);

		if (mb_Aero)
		{
			RECT rc = {x+((lbClicked&&!mb_ArrowClicked)?1:0), y};
			rc.right = rc.left + t->rcBmp.right-t->rcBmp.left+1;
			rc.bottom = rc.top + t->rcBmp.bottom-t->rcBmp.top+1;
			gpConEmu->BufferedPaintSetAlpha(dc, &rc, 255);
		}
	}
	else
	{
		_ASSERTE(mh_PaneDC!=NULL);
	}
}

void CToolBarClass::PaintSeparator(const PaintDC& dc, const RECT& rcTB, int x, int y, int h)
{
	/*
	HPEN hold = (HPEN)SelectObject(hdc, mh_DarkPen);
	MoveToEx(hdc, x, y, NULL);
	LineTo(hdc, x, y+h+1);
	SelectObject(hdc, mh_LitePen);
	MoveToEx(hdc, x+1, y, NULL);
	LineTo(hdc, x+1, y+h+1);
	SelectObject(hdc, hold);
	*/
}

CToolBarClass::ToolInfo* CToolBarClass::AddTool(PaneInfo* aPane, int anCmd, DWORD anFlags, const RECT& rcBmp, LPCWSTR asTip)
{
	if (!(anFlags & TIS_SEPARATOR))
	{
		if (rcBmp.right <= rcBmp.left || rcBmp.bottom <= rcBmp.top)
		{
			_ASSERTE(!(rcBmp.right <= rcBmp.left || rcBmp.bottom <= rcBmp.top));
			return NULL;
		}
	}

	if (aPane->nMax <= aPane->nCount || !aPane->pTool)
	{
		aPane->nMax = aPane->nMax + 16;
		ToolInfo* pt = (ToolInfo*)calloc(aPane->nMax, sizeof(*pt));
		if (!pt)
		{
			_ASSERTE(pt!=NULL);
			return NULL;
		}
		if (aPane->pTool && aPane->nCount > 0)
			memmove(pt, aPane->pTool, sizeof(ToolInfo)*aPane->nCount);
		free(aPane->pTool);
		aPane->pTool = pt;
	}

	ToolInfo ti = {{{{aPane->nPane, anCmd}}}, anFlags, rcBmp};
	if (asTip)
	{
		SetToolTip(&ti, asTip);
		//ti.nTipCchMax = lstrlen(ti.sTip)+1;
		//ti.sTip = (wchar_t*)malloc(ti.nTipCchMax*sizeof(wchar_t));
		//if (ti.sTip)
		//	wcscpy_c(ti.sTip, asTip);
	}
	aPane->nTotalWidth += mn_PaddingWidth
		+ ((anFlags & TIS_SEPARATOR) ? mn_SeparatorWidth : (rcBmp.right - rcBmp.left + 1))
		+ ((anFlags & TIS_DROPDOWN) ? mn_DropArrowWidth : 0);
	int nIdx = aPane->nCount++;
	aPane->pTool[nIdx] = ti;
	return (aPane->pTool+nIdx);
}

void CToolBarClass::SetToolTip(ToolInfo* apTool, LPCWSTR asTip)
{
	if (!asTip || !*asTip)
	{
		if (apTool->sTip)
			*apTool->sTip = 0;
		return;
	}
	int nNewCch = lstrlen(asTip)+1;
	if (nNewCch > apTool->nTipCchMax || !apTool->sTip)
	{
		if (apTool->sTip)
			free(apTool->sTip);
		apTool->sTip = (wchar_t*)calloc(nNewCch, sizeof(wchar_t));
		if (!apTool->sTip)
			return;
		apTool->nTipCchMax = nNewCch;
	}
	lstrcpy(apTool->sTip, asTip);
}

CToolBarClass::PaneInfo* CToolBarClass::GetPaneById(int anPane)
{
	for (INT_PTR i = m_Panels.size(); i--;)
	{
		PaneInfo* pane = m_Panels[i];

		if (pane->nPane == anPane)
		{
			return pane;
		}
	}
	return NULL;
}

CToolBarClass::ToolInfo* CToolBarClass::GetToolById(PaneInfo* aPane, int anCmd)
{
	if (!aPane || !aPane->pTool)
		return NULL;

	for (int t = 0; t < aPane->nCount; t++)
	{
		if (!(aPane->pTool[t].nFlags & TIS_SEPARATOR) && aPane->pTool[t].ID.nCmd == anCmd)
			return (aPane->pTool+t);
	}

	return NULL;
}

void CToolBarClass::UnHover()
{
	if (m_Hover.nID != -1 || m_Clicked.nID != -1)
	{
		m_Hover.nID /*= m_Clicked.nID*/ = -1;
		Invalidate();
	}
}

bool CToolBarClass::MouseEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, POINT ptClient, LRESULT &lResult)
{
	bool lbRedraw = false;

	POINT point = ptClient;
	bool lbArrow = false;
	//RECT wr; GetWindowRect(hWnd, &wr);
	//point.x = LOWORD(lParam) - wr.left - ((gpConEmu->DrawType() == fdt_Aero) ? gpConEmu->GetFrameWidth() : 0);
	//point.y = HIWORD(lParam) - wr.top;

	EnterCriticalSection(&m_CS);
	ToolId hover = {{{-1,-1}}};
	PaneInfo* hoverPane = NULL;
	POINT ptWhere = {};
	RECT rcTool = {};

	for (INT_PTR i = m_Panels.size(); i--;)
	{
		PaneInfo* pane = m_Panels[i];

		if (pane->bVisible && PtInRect(&pane->rcPane, point) && pane->pTool)
		{
			for (int t = 0; t < pane->nCount; t++)
			{
				if (!(pane->pTool[t].nFlags & TIS_SEPARATOR) &&
					(PtInRect(&pane->pTool[t].rcTool, point)
					|| ((pane->pTool[t].nFlags & TIS_DROPDOWN) && (lbArrow = (PtInRect(&pane->pTool[t].rcArrow, point)!=0)))))
				{
					hoverPane = pane;
					hover.nPane = pane->nPane;
					hover.nCmd = pane->pTool[t].ID.nCmd;

					rcTool = pane->pTool[t].rcTool;
					if (pane->pTool[t].nFlags & TIS_DROPDOWN)
						rcTool.right = pane->pTool[t].rcArrow.right;

					if (gpConEmu->DrawType() == fdt_Aero)
					{
						ptWhere.x = rcTool.left - 2;
						ptWhere.y = rcTool.bottom + 2;
						MapWindowPoints(ghWnd, NULL, &ptWhere, 1);
					}
					else
					{
						RECT wr = {};
						GetWindowRect(ghWnd, &wr);
						ptWhere.x = rcTool.left - 2 + wr.left;
						ptWhere.y = rcTool.bottom + 2 + wr.top;
					}

					_ASSERTE(pane->pTool[t].ID.nPane == pane->nPane);
					_ASSERTE(pane->pTool[t].ID.nID == hover.nID);
					break;
				}
			}
			break;
		}
	}
	LeaveCriticalSection(&m_CS);

	if (m_Hover.nID != hover.nID)
	{
		m_Hover = hover;
		lbRedraw = true;
	}

	if ((uMsg == WM_NCLBUTTONDOWN) || (uMsg == WM_LBUTTONDOWN))
	{
		mb_ArrowClicked = lbArrow;
		if (m_Clicked.nID != hover.nID)
		{
			m_Clicked = hover;
			lbRedraw = true;
		}
	}
	else if ((uMsg == WM_NCLBUTTONUP) || (uMsg == WM_LBUTTONUP))
	{
		if (m_Clicked.nID != -1)
		{
			if (hover.nID == m_Clicked.nID)
			{
				_ASSERTE(hoverPane!=NULL);
				//MapWindowPoints(ghWnd, NULL, &ptWhere, 1);
				if (hoverPane && hoverPane->OnToolbarCommand)
				{
					hoverPane->OnToolbarCommand(hoverPane->lParam, hover.nPane, hover.nCmd, mb_ArrowClicked, ptWhere, rcTool);
				}
			}
			m_Clicked.nID = -1;
			lbRedraw = true;
		}
	}
	else if ((uMsg == WM_NCLBUTTONDBLCLK) || (uMsg == WM_LBUTTONDBLCLK))
	{
		mb_ArrowClicked = lbArrow;
		if (hover.nID != -1)
		{
			m_Clicked = hover;
			Invalidate();
			_ASSERTE(hoverPane!=NULL);
			//MapWindowPoints(ghWnd, NULL, &ptWhere, 1);
			if (hoverPane && hoverPane->OnToolbarCommand)
			{
				hoverPane->OnToolbarCommand(hoverPane->lParam, hover.nPane, hover.nCmd, mb_ArrowClicked, ptWhere, rcTool);
			}
			m_Clicked.nID = -1;
			Invalidate();
			lbRedraw = false; // уже
		}
	}
	else if ((uMsg == WM_NCRBUTTONUP) || (uMsg == WM_RBUTTONUP))
	{
		if (hover.nID != -1)
		{
			m_Clicked = hover;
			Invalidate();
			if (hoverPane && hoverPane->OnToolbarMenu)
			{
				hoverPane->OnToolbarMenu(hoverPane->lParam, hover.nPane, hover.nCmd, ptWhere, rcTool);
			}
			m_Clicked.nID = -1;
			Invalidate();
			lbRedraw = false; // уже
		}
	}
	else if (m_Clicked.nID != -1)
	{
		if (!(GetKeyState(VK_LBUTTON) & 0x8000))
		{
			m_Clicked.nID = -1;
			lbRedraw = true;
		}
	}



	if (lbRedraw)
	{
		Invalidate();
	}
	// Продолжить обработку
	return false;
}

//void* CToolBarClass::ToolFromPoint(POINT pt)
//{
//	ToolInfo* p = NULL;
//	EnterCriticalSection(&m_CS);
//	HoverInfo hover = {-1,-1};
//	for (std::vector<PaneInfo>::iterator i = m_Panels.begin(); i != m_Panels.end(); i++)
//	{
//		PaneInfo& pane = *i;
//		if (pane.bVisible && PtInRect(&pane.rcPane, point) && pane.pTool)
//		{
//			for (int t = 0; t < pane.nCount; t++)
//			{
//				if (!(pane.pTool[t].nFlags & TIS_SEPARATOR) && PtInRect(&pane.pTool[t].rcTool, point))
//				{
//					p = pane.pTool+t;
//					break;
//				}
//			}
//			break;
//		}
//	}
//	LeaveCriticalSection(&m_CS);
//	return p;
//}
