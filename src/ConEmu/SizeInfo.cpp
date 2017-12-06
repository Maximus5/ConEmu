
/*
Copyright (c) 2017 Maximus5
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
#include "DpiAware.h"
#include "Options.h"
#include "OptionsClass.h"
#include "SizeInfo.h"

SizeInfo::SizeInfo(CConEmuMain* _ConEmu)
	: mp_ConEmu(_ConEmu)
	, mcs_lock(true)
{
}

SizeInfo::SizeInfo(const SizeInfo& src)
	: mp_ConEmu(src.mp_ConEmu)
	, mcs_lock(true)
	, m_opt(src.m_opt)
	, m_size(src.m_size)
{
}

SizeInfo::~SizeInfo()
{
}

bool SizeInfo::isCalculated() const
{
	return m_size.valid;
}

// Following functions deprecate current sizes, recalculation will be executed on next size request
void SizeInfo::RequestRecalc()
{
	m_size.valid = false;
}

void SizeInfo::LogRequest(LPCWSTR asFrom, LPCWSTR asMessage /*= nullptr*/)
{
	CEStr lsMsg(L"Size recalc requested", asFrom?L" from: ":nullptr, asFrom, asMessage?L" - ":nullptr, asMessage, L"\n");
	//OutputDebugStringW(lsMsg);
	mp_ConEmu->LogString(lsMsg);
}

void SizeInfo::LogRequest(const RECT& rcNew, LPCWSTR asFrom)
{
	wchar_t szInfo[120];
	msprintf(szInfo, countof(szInfo), L"OldRect={%i,%i}-{%i,%i} NewRect={%i,%i}-{%i,%i}", LOGRECTCOORDS(m_size.window), LOGRECTCOORDS(rcNew));
	LogRequest(asFrom, szInfo);
}

void SizeInfo::LogRequest(const int dpi, LPCWSTR asFrom)
{
	wchar_t szInfo[120];
	msprintf(szInfo, countof(szInfo), L"OldDpi=%i NewDpi=%i", m_opt.dpi, dpi);
	LogRequest(asFrom, szInfo);
}

// Change used DPI
void SizeInfo::RequestDpi(const DpiValue& _dpi)
{
	if (m_opt.dpi == _dpi.Ydpi)
		return;
	LogRequest(_dpi.Ydpi, L"RequestDpi");
	MSectionLockSimple lock; lock.Lock(&mcs_lock);
	m_opt.dpi = _dpi.Ydpi;
	RequestRecalc();
}

// Change whole window Rect (includes caption/frame and invisible part of Win10 DWM area)
void SizeInfo::RequestRect(RECT _window)
{
	if (m_size.window == _window)
		return;
	LogRequest(_window, L"RequestRect");
	MSectionLockSimple lock; lock.Lock(&mcs_lock);
	m_size.window = _window;
	RequestRecalc();
}

void SizeInfo::RequestSize(int _width, int _height)
{
	RECT rcCur = m_size.window;
	RECT rcNew = {rcCur.left, rcCur.top, rcCur.left + _width, rcCur.top + _height};
	if (rcNew == m_size.window)
		return;
	LogRequest(rcNew, L"RequestSize");
	MSectionLockSimple lock; lock.Lock(&mcs_lock);
	m_size.window = rcNew;
	RequestRecalc();
}

void SizeInfo::RequestPos(int _x, int _y)
{
	RECT rcCur = m_size.window;
	RECT rcNew = {_x, _y, _x + RectWidth(rcCur), _y + RectHeight(rcCur)};
	if (rcNew == m_size.window)
		return;
	LogRequest(rcNew, L"RequestPos");
	MSectionLockSimple lock; lock.Lock(&mcs_lock);
	m_size.window = rcNew;
	RequestRecalc();
}


// *** Relative to the upper-left corner of the screen or the parent (if Inside mode) ***

// The size and pos of the window
RECT SizeInfo::WindowRect()
{
	DoCalculate();
	return m_size.window;
}

// In Win10 it may be smaller than stored WindowRect
RECT SizeInfo::VisibleRect()
{
	DoCalculate();
	return m_size.visible;
}


// *** Relative to the upper-left corner of the client area ***

HRGN SizeInfo::CreateSelfFrameRgn()
{
	if (!mp_ConEmu->isCaptionHidden())
		return NULL;

	DoCalculate();

	if (m_size.client == m_size.real_client)
		return NULL;

	HRGN hWhole, hClient;
	hWhole = CreateRectRgn(0, 0, m_size.real_client.right, m_size.real_client.bottom);
	hClient = CreateRectRgn(m_size.client.left, m_size.client.top, m_size.client.right, m_size.client.bottom);
	int iRc = CombineRgn(hWhole, hWhole, hClient, RGN_DIFF);
	if (iRc != SIMPLEREGION && iRc != COMPLEXREGION)
	{
		_ASSERTEX(iRc == COMPLEXREGION);
		DeleteObject(hWhole);
		DeleteObject(hClient);
		return NULL;
	}
	DeleteObject(hClient);
	return hWhole;
}

// Client rectangle, may be simulated if we utilize some space for self-implemented borders
RECT SizeInfo::ClientRect()
{
	DoCalculate();
	return m_size.client;
}

// Factual client rectangle, as Windows knows about our window
RECT SizeInfo::RealClientRect()
{
	DoCalculate();
	return m_size.real_client;
}

// Contains TabBar, Search panel, ToolBar
RECT SizeInfo::RebarRect()
{
	DoCalculate();
	return m_size.rebar;
}

// StatusBar
RECT SizeInfo::StatusRect()
{
	DoCalculate();
	return m_size.status;
}

// Workspace (contains VCons with splitters)
RECT SizeInfo::WorkspaceRect()
{
	DoCalculate();
	return m_size.workspace;
}


// Calculates new rectangles, if required, based on current settings and options
void SizeInfo::DoCalculate()
{
	if (m_size.valid)
		return;

	MSectionLockSimple lock; lock.Lock(&mcs_lock);

	if (IsRectEmpty(&m_size.window))
		m_size.window = mp_ConEmu->GetDefaultRect();

	DpiValue dpi; dpi.SetDpi(m_opt.dpi, m_opt.dpi);
	bool caption_hidden = mp_ConEmu->isCaptionHidden();

	m_size.visible = m_size.window;
	if (!caption_hidden && IsWin10())
	{
		RECT rcWin10 = mp_ConEmu->CalcMargins(CEM_WIN10FRAME);
		m_size.visible.left += rcWin10.left;
		m_size.visible.top += rcWin10.top;
		m_size.visible.right -= rcWin10.right;
		m_size.visible.bottom += rcWin10.bottom;
	}

	RECT rcFrame = mp_ConEmu->CalcMargins(CEM_FRAMECAPTION);
	if (!caption_hidden)
	{
		m_size.real_client = m_size.client = RECT{0, 0, RectWidth(m_size.window) - rcFrame.left - rcFrame.right, RectHeight(m_size.window) - rcFrame.top - rcFrame.bottom};
	}
	else
	{
		m_size.real_client = RECT{0, 0, RectWidth(m_size.window), RectHeight(m_size.window)};
		m_size.client = RECT{rcFrame.left, rcFrame.top, RectWidth(m_size.window) - rcFrame.right, RectHeight(m_size.window) - rcFrame.bottom};
	}

	m_size.workspace = m_size.client;

	int newStatusHeight = 0;
	if (gpSet->isStatusBarShow)
		newStatusHeight = gpSet->StatusBarHeight();
	if (newStatusHeight > 0)
	{
		m_size.status = m_size.workspace;
		m_size.status.top = m_size.status.bottom - newStatusHeight;
		m_size.workspace.bottom = m_size.status.top;
	}
	else
	{
		m_size.status = RECT{};
	}

	int newTabHeight = 0;
	if (gpSet->isTabs != 0)
	{
		int lfHeight = gpSetCls->EvalSize(gpSet->nTabFontHeight, esf_Vertical|esf_CanUseDpi|esf_CanUseUnits, &dpi);
		newTabHeight = gpFontMgr->EvalFontHeight(gpSet->sTabFontFace, lfHeight, gpSet->nTabFontCharSet)
			+ gpSetCls->EvalSize((lfHeight < 0) ? 8 : 9, esf_Vertical, &dpi);
	}
	if (newTabHeight > 0)
	{
		m_size.rebar = m_size.workspace;
		switch (gpSet->nTabsLocation)
		{
		case 0: // top
			m_size.rebar.bottom = m_size.rebar.top + newTabHeight;
			m_size.workspace.top += newTabHeight;
			break;
		case 1: // bottom
			m_size.rebar.top = m_size.rebar.bottom - newTabHeight;
			m_size.workspace.bottom -= newTabHeight;
			break;
		default:
			m_size.rebar = RECT{};
		}
	}
	else
	{
		m_size.rebar = RECT{};
	}

	m_size.valid = true;
}
