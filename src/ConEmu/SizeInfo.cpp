
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

#define DEBUGSTRSIZE(s) DEBUGSTR(s)

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
	, mb_temp(true)
{
}

SizeInfo::~SizeInfo()
{
}

bool SizeInfo::isCalculated() const
{
	return m_size.valid;
}

//static
bool SizeInfo::IsRectMinimized(const RECT& rc)
{
	return IsRectMinimized(rc.left, rc.top) || IsRectEmpty(&rc);
}

//static
bool SizeInfo::IsRectMinimized(const int x, const int y)
{
	return (x <= WINDOWS_ICONIC_POS || y <= WINDOWS_ICONIC_POS);
}

// Following functions deprecate current sizes, recalculation will be executed on next size request
void SizeInfo::RequestRecalc()
{
	m_size.valid = false;
}

void SizeInfo::LogRequest(LPCWSTR asFrom, LPCWSTR asMessage /*= nullptr*/)
{
	CEStr lsMsg(L"Size recalc requested",
		mb_temp ? L" (temp)" : L" (main)",
		asFrom ? L" from: " : nullptr, asFrom,
		asMessage?L" - ":nullptr, asMessage, L"\n");
	if (!gpSet->isLogging()) { DEBUGSTRSIZE(lsMsg); }
	mp_ConEmu->LogString(lsMsg);
}

void SizeInfo::LogRequest(const RECT& rcNew, LPCWSTR asFrom)
{
	wchar_t szInfo[120];
	msprintf(szInfo, countof(szInfo), L"OldRect={%i,%i}-{%i,%i} {%i*%i} NewRect={%i,%i}-{%i,%i} {%i*%i}",
		LOGRECTCOORDS(m_size.rr.window), LOGRECTSIZE(m_size.rr.window),
		LOGRECTCOORDS(rcNew), LOGRECTSIZE(rcNew));
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

void SizeInfo::RequestRectInt(const RECT& _window, LPCWSTR asFrom)
{
	if (_window == (m_size.valid ? m_size.rr.window : m_size.source_window))
		return;
	LogRequest(_window, asFrom);
	if (IsRectMinimized(_window))
		return;
	MSectionLockSimple lock; lock.Lock(&mcs_lock);
	m_size.source_window = _window;
	RequestRecalc();
}

// Change whole window Rect (includes caption/frame and invisible part of Win10 DWM area)
void SizeInfo::RequestRect(const RECT& _window)
{
	RequestRectInt(_window, L"RequestRect");
}

void SizeInfo::RequestSize(const int _width, const int _height)
{
	RECT rcCur = m_size.rr.window;
	RECT rcNew = {rcCur.left, rcCur.top, rcCur.left + _width, rcCur.top + _height};
	RequestRectInt(rcNew, L"RequestSize");
}

void SizeInfo::RequestPos(const int _x, const int _y)
{
	RECT rcCur = m_size.rr.window;
	RECT rcNew = {_x, _y, _x + RectWidth(rcCur), _y + RectHeight(rcCur)};
	if (rcNew == (m_size.valid ? m_size.rr.window : m_size.source_window))
		return;
	RequestRectInt(rcNew, L"RequestPos");
}


// *** Relative to the upper-left corner of the screen or the parent (if Inside mode) ***

// The size and pos of the window
RECT SizeInfo::WindowRect()
{
	DoCalculate();
	return m_size.rr.window;
}

// In Win10 it may be smaller than stored WindowRect
RECT SizeInfo::VisibleRect()
{
	DoCalculate();
	return m_size.rr.visible;
}


// *** Relative to the upper-left corner of the client area ***

HRGN SizeInfo::CreateSelfFrameRgn()
{
	if (!mp_ConEmu->isCaptionHidden())
		return NULL;

	DoCalculate();

	// When Windows standard frame is used - real_client has offset from window left/top
	// But with owner-draw frame - real_client starts from window left/top
	if (m_size.rr.real_client.left == 0 && m_size.rr.real_client.top == 0
		&& RectWidth(m_size.rr.client) == RectWidth(m_size.rr.real_client)
		&& RectHeight(m_size.rr.client) == RectHeight(m_size.rr.real_client))
		return NULL;

	HRGN hWhole, hClient;
	hWhole = CreateRectRgnIndirect(&m_size.rr.real_client);
	hClient = CreateRectRgnIndirect(&m_size.rr.client);
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

const SizeInfo::WindowRectangles& SizeInfo::GetRectState() const
{
	return m_size.rr;
}

// Client rectangle, may be simulated if we utilize some space for self-implemented borders
RECT SizeInfo::ClientRect()
{
	DoCalculate();
	return m_size.rr.client;
}

// Factual client rectangle, as Windows knows about our window
RECT SizeInfo::RealClientRect()
{
	DoCalculate();
	return m_size.rr.real_client;
}

// Contains TabBar, Search panel, ToolBar
RECT SizeInfo::RebarRect()
{
	DoCalculate();
	return m_size.rr.rebar;
}

// StatusBar
RECT SizeInfo::StatusRect()
{
	DoCalculate();
	return m_size.rr.status;
}

// Workspace (contains VCons with splitters)
RECT SizeInfo::WorkspaceRect()
{
	DoCalculate();
	return m_size.rr.workspace;
}


// Calculates new rectangles, if required, based on current settings and options
void SizeInfo::DoCalculate()
{
	if (m_size.valid)
		return;

	MSectionLockSimple lock; lock.Lock(&mcs_lock);

	if (IsRectMinimized(m_size.source_window))
	{
		// Inside mode?
		_ASSERTEX(RectWidth(m_size.source_window)>0 && RectHeight(m_size.source_window)>0);
		// m_size.rr = {}; // -- don't touch current values?
		m_size.valid = true;
		return;
	}

	// #SIZE_TODO Declare local "WindowRectangles rr" and fill it, apply the value at the end

	m_size.rr.window = m_size.source_window;

	DpiValue dpi; dpi.SetDpi(m_opt.dpi, m_opt.dpi);
	bool caption_hidden = mp_ConEmu->isCaptionHidden();

	m_size.rr.visible = m_size.rr.window;
	if (!caption_hidden && IsWin10())
	{
		RECT rcWin10 = mp_ConEmu->CalcMargins(CEM_WIN10FRAME);
		m_size.rr.visible.left += rcWin10.left;
		m_size.rr.visible.top += rcWin10.top;
		m_size.rr.visible.right -= rcWin10.right;
		m_size.rr.visible.bottom += rcWin10.bottom;
	}

	if (mp_ConEmu->isInside())
	{
		// No frame at all, client area covers all our window space
		m_size.rr.real_client = m_size.rr.client = RECT{0, 0, RectWidth(m_size.rr.window), RectHeight(m_size.rr.window)};
	}
	else
	{
		const auto mi = mp_ConEmu->NearestMonitorInfo(m_size.rr.window);
		RECT rcFrame = caption_hidden ? mi.noCaption.FrameMargins : mi.withCaption.FrameMargins;

		if (!mp_ConEmu->isSelfFrame())
		{
			_ASSERTE(!mp_ConEmu->isInside());
			m_size.rr.real_client = RECT{rcFrame.left, rcFrame.top, RectWidth(m_size.rr.window) - rcFrame.right, RectHeight(m_size.rr.window) - rcFrame.bottom};
			m_size.rr.client = RECT{0, 0, RectWidth(m_size.rr.window) - rcFrame.left - rcFrame.right, RectHeight(m_size.rr.window) - rcFrame.top - rcFrame.bottom};
		}
		else
		{
			int iFrame = gpSet->HideCaptionAlwaysFrame();
			if ((iFrame >= 0) && (iFrame < rcFrame.left))
				rcFrame = RECT{iFrame, iFrame, iFrame, iFrame};
			m_size.rr.real_client = RECT{0, 0, RectWidth(m_size.rr.window), RectHeight(m_size.rr.window)};
			m_size.rr.client = RECT{rcFrame.left, rcFrame.top, RectWidth(m_size.rr.window) - rcFrame.right, RectHeight(m_size.rr.window) - rcFrame.bottom};
		}
	}

	m_size.rr.workspace = m_size.rr.client;

	int newStatusHeight = 0;
	if (gpSet->isStatusBarShow)
		newStatusHeight = gpSet->StatusBarHeight();
	if (newStatusHeight > 0)
	{
		m_size.rr.status = m_size.rr.workspace;
		m_size.rr.status.top = m_size.rr.status.bottom - newStatusHeight;
		m_size.rr.workspace.bottom = m_size.rr.status.top;
	}
	else
	{
		m_size.rr.status = RECT{};
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
		m_size.rr.rebar = m_size.rr.workspace;
		switch (gpSet->nTabsLocation)
		{
		case 0: // top
			m_size.rr.rebar.bottom = m_size.rr.rebar.top + newTabHeight;
			m_size.rr.workspace.top += newTabHeight;
			break;
		case 1: // bottom
			m_size.rr.rebar.top = m_size.rr.rebar.bottom - newTabHeight;
			m_size.rr.workspace.bottom -= newTabHeight;
			break;
		default:
			m_size.rr.rebar = RECT{};
		}
	}
	else
	{
		m_size.rr.rebar = RECT{};
	}

	m_size.valid = true;
}
