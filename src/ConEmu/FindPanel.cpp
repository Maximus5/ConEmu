
/*
Copyright (c) 2014 Maximus5
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
#include "ConEmu.h"
#include "FindPanel.h"
#include "Options.h"
#include "SearchCtrl.h"
#include "VConGroup.h"
#include "../common/MMap.h"

ATOM CFindPanel::mh_Class = NULL;
#define FindPanelClass L"ConEmuFindPanel"
#define SearchHint L"Search"

static MMap<HWND,CFindPanel*> g_FindMap = {};

CFindPanel::CFindPanel(CConEmuMain* apConEmu)
	: mp_ConEmu(apConEmu)
	, mh_Pane(NULL)
	, mh_Edit(NULL)
	, mh_Font(NULL)
{
	if (!g_FindMap.Initialized())
		g_FindMap.Init(16);
}

CFindPanel::~CFindPanel()
{
	OnDestroy();
}

HWND CFindPanel::GetHWND()
{
	return mh_Pane;
}

int CFindPanel::GetMinWidth()
{
	if (!this || !mh_Edit)
		return 0;
	RECT rcEdit = {};
	GetWindowRect(mh_Edit, &rcEdit);
	return max(80,6*(rcEdit.bottom - rcEdit.top));
}

HWND CFindPanel::CreatePane(HWND hParent, int nHeight)
{
	if (mh_Pane && IsWindow(mh_Pane))
		return mh_Pane;

	if (!RegisterPaneClass())
		return NULL;

	mh_Pane = CreateWindowEx(WS_EX_CONTROLPARENT,
		FindPanelClass, L"",
		WS_CHILD|WS_VISIBLE, 0,0,0,nHeight, hParent, NULL, NULL, this);

	return mh_Pane;
}

bool CFindPanel::RegisterPaneClass()
{
	if (mh_Class)
		return true;

	WNDCLASSEX wcFindPane = {
		sizeof(wcFindPane), 0,
		FindPaneProc, 0, 0,
		g_hInstance, NULL, LoadCursor(NULL, IDC_ARROW),
		(HBRUSH)(COLOR_BTNFACE+1), NULL,
		FindPanelClass
	};

	mh_Class = RegisterClassEx(&wcFindPane);

	return (mh_Class != NULL);
}

LRESULT CFindPanel::FindPaneProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR lRc = 0;
	CFindPanel* pPanel = NULL;
	RECT rcClient = {};

	if (uMsg == WM_CREATE)
	{
		if (lParam && ((CREATESTRUCT*)lParam)->lpCreateParams)
		{
			pPanel = (CFindPanel*)((CREATESTRUCT*)lParam)->lpCreateParams;
			pPanel->mh_Pane = hWnd;
		}
	}
	else
	{
		g_FindMap.Get(hWnd, &pPanel);
	}

	if (pPanel && EditIconHint_Process(hWnd, uMsg, wParam, lParam, lRc))
	{
		return lRc;
	}

	switch (uMsg)
	{
	case WM_CREATE:
		if (!pPanel || !((CFindPanel*)((CREATESTRUCT*)lParam)->lpCreateParams)->OnCreate((CREATESTRUCT*)lParam))
		{
			lRc = -1;
			goto wrap;
		}
		break;

	case WM_COMMAND:
		break;

	case WM_SIZE:
		if (pPanel)
			pPanel->OnSize();
		break;

	case WM_DESTROY:
		if (pPanel)
			pPanel->OnDestroy();
		break;

	case UM_SEARCH:
		if (pPanel)
			pPanel->OnSearch();
		break;
	}

	lRc = ::DefWindowProc(hWnd, uMsg, wParam, lParam);
wrap:
	return lRc;
}

LRESULT CFindPanel::EditCtrlProc(HWND hCtrl, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRc = 0;
	CFindPanel* pPanel = NULL;
	g_FindMap.Get(hCtrl, &pPanel);

	if (pPanel) switch (uMsg)
	{
	case WM_KEYDOWN:
	case WM_KEYUP:
		switch (LOWORD(wParam))
		{
		case VK_ESCAPE:
		case VK_F10:
			if (uMsg == WM_KEYUP)
				pPanel->StopSearch();
			goto wrap;
		case VK_RETURN:
		case VK_F3:
			if (uMsg == WM_KEYDOWN)
				gpConEmu->DoFindText(isPressed(VK_SHIFT) ? -1 : 1);
			goto wrap;
		}
		break;

	case WM_SYSKEYDOWN:
		lRc = TRUE;
		switch (LOWORD(wParam))
		{
		case 'W':
			gpSet->FindOptions.bMatchWholeWords = !gpSet->FindOptions.bMatchWholeWords;
			break;
		case 'C':
			gpSet->FindOptions.bMatchCase = !gpSet->FindOptions.bMatchCase;
			break;
		case 'F':
			gpSet->FindOptions.bFreezeConsole = !gpSet->FindOptions.bFreezeConsole;
			break;
		case 'A':
			gpSet->FindOptions.bHighlightAll = !gpSet->FindOptions.bHighlightAll;
			break;
		default:
			goto wrap;
		}
		gpConEmu->DoFindText(isPressed(VK_SHIFT) ? -1 : 1);
		goto wrap;
	case WM_SYSKEYUP:
		lRc = TRUE;
		goto wrap;
	}

	if (pPanel && pPanel->mfn_EditProc)
		lRc = ::CallWindowProc(pPanel->mfn_EditProc, hCtrl, uMsg, wParam, lParam);
	else
		lRc = ::DefWindowProc(hCtrl, uMsg, wParam, lParam);
wrap:
	return lRc;
}

bool CFindPanel::OnCreate(CREATESTRUCT* ps)
{
	mh_Edit = CreateWindowEx(WS_EX_CLIENTEDGE,
		L"EDIT", L"",
		WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,
		0,0,ps->cx,ps->cy, mh_Pane, (HMENU)1, NULL, NULL);
	if (!mh_Edit)
		return false;

	mfn_EditProc = (WNDPROC)SetWindowLongPtr(mh_Edit, GWLP_WNDPROC, (LONG_PTR)EditCtrlProc);

	g_FindMap.Set(mh_Pane, this);
	g_FindMap.Set(mh_Edit, this);

	SafeDeleteObject(mh_Font);

	// CreateFont
	LOGFONT lf = {};
	lf.lfWeight = FW_DONTCARE;
	lf.lfCharSet = gpSet->nTabFontCharSet;
	lf.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
	wcscpy_c(lf.lfFaceName, gpSet->sTabFontFace);
	lf.lfHeight = -max(6,ps->cy - 8);
	mh_Font = CreateFontIndirect(&lf);

	SendMessage(mh_Edit, WM_SETFONT, (WPARAM)mh_Font, FALSE);

	EditIconHint_Set(mh_Pane, mh_Edit, true, SearchHint, false, UM_SEARCH, 0);

	return true;
}

void CFindPanel::OnDestroy()
{
	g_FindMap.Del(mh_Pane);
	g_FindMap.Del(mh_Edit);
	SafeDeleteObject(mh_Font);
	mh_Pane = mh_Edit = NULL;
}

void CFindPanel::OnSize()
{
	RECT rcClient = {};
	if (mh_Edit)
	{
		GetClientRect(mh_Pane, &rcClient);
		MoveWindow(mh_Edit, 0, 0, rcClient.right-1, rcClient.bottom-2, TRUE);
	}
}

void CFindPanel::OnSearch()
{
	if (!CVConGroup::isVConExists(0))
		return;
	MyGetDlgItemText(mh_Edit, 0, gpSet->FindOptions.cchTextMax, gpSet->FindOptions.pszText);
	if (gpSet->FindOptions.pszText && *gpSet->FindOptions.pszText)
		gpConEmu->DoFindText(0);
}

void CFindPanel::StopSearch()
{
	if (mh_Edit)
		SetWindowText(mh_Edit, L"");
	//gpSet->SaveFindOptions();
	gpConEmu->setFocus();
	gpConEmu->DoEndFindText();
}
