
/*
Copyright (c) 2017-present Maximus5
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

#include "../common/defines.h"

#include "../common/MSectionSimple.h"

#define WINDOWS_ICONIC_POS -32000

struct DpiValue;
class CConEmuMain;

class SizeInfo
{
private:
	CConEmuMain* mp_ConEmu;
	bool mb_temp = false;
public:
	SizeInfo(CConEmuMain* _ConEmu);
	SizeInfo(const SizeInfo& src);
	virtual ~SizeInfo();

	SizeInfo& operator=(const SizeInfo&) = delete;
	SizeInfo& operator=(SizeInfo&&) = delete;
	SizeInfo(SizeInfo&&) = delete;

public:
	// *** Relative to the upper-left corner of the screen or the parent (if Inside mode) ***

	// The size and pos of the window
	RECT WindowRect();
	// In Win10 it may be smaller than stored WindowRect
	RECT VisibleRect();

	// *** Relative to the upper-left corner of the client area ***

	HRGN CreateSelfFrameRgn();

	// The Frame with Caption (if visible)
	RECT FrameMargins();
	// Client rectangle, may be simulated if we utilize some space for self-implemented borders
	RECT ClientRect();
	// Factual client rectangle, as Windows knows about our window
	RECT RealClientRect();
	// Contains TabBar, Search panel, ToolBar
	RECT RebarRect();
	// StatusBar
	RECT StatusRect();
	// Workspace (contains VCons with splitters)
	RECT WorkspaceRect();

public:
	// Rectangles
	struct WindowRectangles
	{
		RECT window = {};
		// All rects below are calculated from m_size.window
		RECT visible = {};
		RECT frame = {};
		RECT real_client = {};
		RECT client = {};
		RECT rebar = {};
		RECT status = {};
		RECT workspace = {};
	};

public:
	// These functions DOES NOT do size recalculation!!!
	const WindowRectangles& GetRectState() const;
	int GetDefaultTabbarHeight() const;

	// Check if sizes are up to date
	bool isCalculated() const;
	// Following functions deprecate current sizes, recalculation will be executed on next size request
	void RequestRecalc();
	// Change used DPI
	void RequestDpi(const DpiValue& _dpi);
	// Change whole window Rect (includes caption/frame and invisible part of Win10 DWM area)
	void RequestRect(const RECT& _window);
	void RequestSize(const int _width, const int _height);
	void RequestPos(const int _x, const int _y);

	static bool IsRectMinimized(const RECT& rc);
	static bool IsRectMinimized(const int x, const int y);

protected:
	// Calculates new rectangles, if required, based on current settings and options
	void DoCalculate();
	void LogRequest(LPCWSTR asFrom, LPCWSTR asMessage = nullptr);
	void LogRequest(const RECT& rcNew, LPCWSTR asFrom);
	void LogRequest(const int dpi, LPCWSTR asFrom);
	void RequestRectInt(const RECT& _window, LPCWSTR asFrom);

protected:
	MSectionSimple mcs_lock;

	struct
	{
		/*
		enum CalcSource
		{
			// mrc_window as source
			eWindowRect,
			// use sizes from options (cells, pixels or percentage of monitor space)
			eOptionSize,
		} m_calc_source = CalcSource::eWindowRect;
		*/

		int dpi = 96;
		int tabbar_font = 0;
		int statusbar_font = 0;
	} m_opt;

	struct
	{
		// If it's false - recalculation is required
		bool valid = false;

		// Source rectangle for calculation
		RECT source_window = {};

		// Rectangles
		WindowRectangles rr;
	} m_size;
};
