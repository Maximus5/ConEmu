
/*
Copyright (c) 2014-present Maximus5
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

#include "../common/MSectionSimple.h"

#include "SizeInfo.h"

class CConEmuMain;
struct DpiValue;

enum DpiChangeSource
{
	dcs_Api,
	dcs_Macro,
	dcs_Jump,
	dcs_Snap,
	dcs_Internal,
};

enum RectOperations
{
	rcop_Shrink,
	rcop_Enlarge,
	rcop_AddSize,
	rcop_MathAdd,
	rcop_MathSub,
};

class CConEmuSize : public SizeInfo
{
private:
	CConEmuMain* mp_ConEmu;

protected:
	bool mb_InSetQuakeMode = false;
	LONG mn_IgnoreQuakeActivation = 0;
	DWORD mn_LastQuakeShowHide = 0;
public:
	void SetIgnoreQuakeActivation(bool bNewValue);
protected:
	int mn_QuakePercent = 0; // 0 - no quake animation ATM; (>0 && <=100) - quake animation in progress
	bool mb_DisableThickFrame = false; // during quake animation (::AnimateWindow) we must turn off standard Windows frame to avoid glitches

	struct QuakePrevSize {
		bool bWasSaved;
		bool bWaitReposition; // Требуется смена позиции при OnHideCaption
		CESize wndWidth, wndHeight; // Консоль
		LONG wndX, wndY; // GUI (Visual Position)
		DWORD nFrame; // it's BYTE, DWORD here for alignment
		ConEmuWindowMode WindowMode;
		RECT rcStoredNormalRect;
		bool MinToTray;
		// Used in GetDefaultRect/GetDefaultSize after Quake was slided up (hidden)
		RECT PreSlidedSize;
		// helper methods
		void Save(const CESize& awndWidth, const CESize& awndHeight, const LONG& awndX, const LONG& awndY, const BYTE& anFrame, const ConEmuWindowMode& aWindowMode, const RECT& arcStoredNormalRect, const bool& abMinToTray);
		ConEmuWindowMode Restore(CESize& rwndWidth, CESize& rwndHeight, LONG& rwndX, LONG& rwndY, BYTE& rnFrame, RECT& rrcStoredNormalRect, bool& rbMinToTray);
		void SetNonQuakeDefaults();
	} m_QuakePrevSize = {};

	bool CheckQuakeRect(LPRECT prcWnd);

	ConEmuWindowCommand m_TileMode = cwc_Current;
	void SetTileMode(ConEmuWindowCommand Tile);

	struct {
		RECT rcNewPos;
		bool bInJump;
		bool bFullScreen, bMaximized;
	} m_JumpMonitor = {};

	enum SnappingMagnetFlags
	{
		smf_None    = 0,
		smf_Left    = 0x0001,
		smf_Right   = 0x0002,
		smf_Top     = 0x0004,
		smf_Bottom  = 0x0008,
		/* *** */
		smf_Horz    = (smf_Left|smf_Right),
		smf_Vert    = (smf_Top|smf_Bottom),
	};
	struct {
		// set of SnappingMagnetFlags
		DWORD LastFlags;
		// if window was magnetted, then contains last shift value
		int   CorrectionX, CorrectionY;
		// clear states
		void  reset(DWORD flags = smf_None, int newX = 0, int newY = 0)
		{
			LastFlags = flags;
			CorrectionX = (flags & smf_Horz) ? newX : 0;
			CorrectionY = (flags & smf_Vert) ? newY : 0;
		};
	} m_Snapping = {};

	LONG mn_InResize = 0;
	RECT mrc_StoredNormalRect = {};

	POINT ptFullScreenSize = {}; // size for GetMinMaxInfo in Fullscreen mode
	
	ConEmuWindowMode WindowMode = wmNormal;           // wmNormal/wmMaximized/wmFullScreen
	ConEmuWindowMode changeFromWindowMode = wmNotChanging; // wmNotChanging/rmNormal/rmMaximized/rmFullScreen
	
	bool isRestoreFromMinimized = false;
	bool isWndNotFSMaximized = false; // ставится в true, если при переходе в FullScreen - был Maximized
	bool isQuakeMinimized = false;    // изврат, для случая когда "Quake" всегда показывается на таскбаре

	friend class CSetPgSizePos;
	// The size of the main window (cells/pixels/percents)
	CESize WndWidth = {}, WndHeight = {};
	// Visual position of the upper-left corner (in pixels)
	// May differ from its real one (obtained from GetWindowRect) due to invisible parts of the frame
	POINT WndPos = {};
	// Processing functions
	POINT VisualPosFromReal(const int x, const int y);
	POINT RealPosFromVisual(const int x, const int y);

	bool mb_LockWindowRgn = false;
	bool mb_LockShowWindow = false;
	LONG mn_IgnoreSizeChange = 0;
	LONG mn_InSetWindowPos = 0;

	// If the frame is totally hidden (frame width = 0)
	// or self-frame is used (frame width > 0) than we need
	// to change window styles to allow resize with mouse
	enum {
		fsf_Hide = 0,     // Normal mode (frame is hidden)
		fsf_WaitShow = 1, // The timer to show frame (on mouse-hover) was started
		fsf_Show = 2,     // Resizable frame is ON
	} m_ForceShowFrame = fsf_Hide;

	bool mb_InShowMinimized = false; // true during execution ShowWindow(SW_SHOWMIN...)

	bool mb_LastRgnWasNull = true;

	// -- Не используется по сути!!!
	//private:
	//bool mb_PostUpdateWindowSize;
	//public:
	//void SetPostUpdateWindowSize(bool bValue);
	//bool isPostUpdateWindowSize() { return mb_PostUpdateWindowSize; };

public:
	struct FrameInfoCache
	{
		// if DWMWA_EXTENDED_FRAME_BOUNDS succeeded, we fill Win10Stealthy
		// with invisible parts of the real frame border (weird...)
		RECT Win10Stealthy;
		// Default *full* frame width for resizeable windows
		int FrameWidth;
		// Default *visible* frame width for resizeable windows (much smaller in Win10)
		int VisibleFrameWidth;
		// Default offsets for client area
		RECT FrameMargins;
	};
	struct MonitorInfoCache
	{
		HMONITOR hMon;
		MONITORINFO mi;
		// Per-monitor DPI
		int Xdpi, Ydpi;
		// For resizeable windows with caption
		FrameInfoCache withCaption;
		// And without caption
		FrameInfoCache noCaption;
		// TaskBar information
		HWND hTaskbar;
		BOOL isTaskbarHidden;
		RECT rcTaskbarExcess; // for auto-hidden taskbar
		enum TaskBarLocation : UINT
		{
			Left   = ABE_LEFT,
			Right  = ABE_RIGHT,
			Top    = ABE_TOP,
			Bottom = ABE_BOTTOM,
			Absent = (UINT)-1,
		} taskbarLocation;
	};
protected:
	mutable MSectionSimple mcs_monitors = MSectionSimple(true);
	MArray<MonitorInfoCache> monitors;
	// Save preferred monitor to restore
	HMONITOR mh_MinFromMonitor = NULL;
	// Updated during move operation (jump, etc.)
	HMONITOR mh_RequestedMonitor = NULL;
	// true during jump to monitor with different dpi
	bool mb_MonitorDpiChanged = false;
public:
	void ReloadMonitorInfo();
	void SetRequestedMonitor(HMONITOR hNewMon);
	MonitorInfoCache NearestMonitorInfo(HMONITOR hNewMon) const;
	MonitorInfoCache NearestMonitorInfo(const RECT& rcWnd) const;

public:
	CConEmuSize();
	virtual ~CConEmuSize();

public:
	static void AddMargins(RECT& rc, const RECT& rcAddShift, RectOperations rect_op = rcop_Shrink);

private:
	HMONITOR FindInitialMonitor(MONITORINFO* pmi = NULL);

public:
	void AutoSizeFont(RECT arFrom, enum ConEmuRect tFrom);
	RECT CalcMargins(DWORD/*enum ConEmuMargins*/ mg, ConEmuWindowMode wmNewMode = wmCurrent);
	RECT CalcRect(enum ConEmuRect tWhat, CVirtualConsole* pVCon = NULL);
	RECT CalcRect(enum ConEmuRect tWhat, const RECT &rFrom, enum ConEmuRect tFrom, CVirtualConsole* pVCon = NULL, enum ConEmuMargins tTabAction = CEM_TAB);
	void CascadedPosFix();
	SIZE GetDefaultSize(bool bCells, const CESize* pSizeW = NULL, const CESize* pSizeH = NULL, HMONITOR hMon = NULL);
	RECT GetDefaultRect();
	int  GetInitialDpi(DpiValue* pDpi);
	RECT GetIdealRect();
	void UpdateInsideRect(RECT rcNewPos);

	void RecreateControls(bool bRecreateTabbar, bool bRecreateStatus, bool bResizeWindow, LPRECT prcSuggested = NULL);
	bool SetQuakeMode(BYTE NewQuakeMode, ConEmuWindowMode nNewWindowMode = wmNotChanging, bool bFromDlg = false);

	static LPCWSTR FormatTileMode(ConEmuWindowCommand Tile, wchar_t* pchBuf, size_t cchBufMax);
	bool ChangeTileMode(ConEmuWindowCommand Tile);
	ConEmuWindowCommand GetTileMode(bool Estimate, MONITORINFO* pmi = NULL);
	ConEmuWindowCommand EvalTileMode(const RECT& rcWnd, MONITORINFO* pmi = NULL);
	RECT GetTileRect(ConEmuWindowCommand Tile, const MONITORINFO& mi) const;
	ConEmuWindowMode GetWindowMode() const;
	ConEmuWindowMode GetChangeFromWindowMode() const;
	bool IsWindowModeChanging() const;
	bool SetWindowMode(ConEmuWindowMode inMode, bool abForce = false, bool abFirstShow = false);
	bool SetWindowPosSize(LPCWSTR asX, LPCWSTR asY, LPCWSTR asW, LPCWSTR asH);
	void SetWindowPosSizeParam(wchar_t acType, LPCWSTR asValue);
	bool IsSizeFree(ConEmuWindowMode CheckMode = wmFullScreen);
	bool IsSizePosFree(ConEmuWindowMode CheckMode = wmFullScreen);
	bool IsCantExceedMonitor();
	bool IsPosLocked();
	bool IsInResize();
	bool IsInWindowModeChange();
	void LogMinimizeRestoreSkip(LPCWSTR asMsgFormat, DWORD nParm1 = 0, DWORD nParm2 = 0, DWORD nParm3 = 0);
	bool JumpNextMonitor(bool Next);
	bool JumpNextMonitor(HWND hJumpWnd, HMONITOR hJumpMon, bool Next, const RECT rcJumpWnd, LPRECT prcNewPos = NULL);
	void DoBringHere();
	void DoFullScreen();
	void DoMaximizeRestore();
	void DoMinimizeRestore(SingleInstanceShowHideType ShowHideType = sih_None);
	void DoForcedFullScreen(bool bSet = true);
	void DoAlwaysOnTopSwitch();
	void DoDesktopModeSwitch();

	void ReSize(bool abCorrect2Ideal = false);

	bool setWindowPos(HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);

	bool isFullScreen();
	bool isIconic(bool abRaw = false);
	bool isWindowNormal();
	bool isZoomed();

	void UpdateWindowRgn();
	bool ShowMainWindow(int anCmdShow, bool abFirstShow = false);
	void CheckTopMostState();
	bool SizeWindow(const CESize sizeW, const CESize sizeH);

	void EvalVConCreateSize(CVirtualConsole* apVCon, unsigned& rnTextWidth, unsigned& rnTextHeight);

	HWND FindNextSiblingApp(bool bActivate);

public:
	HMONITOR GetNearestMonitor(MONITORINFO* pmi = NULL, LPCRECT prcWnd = NULL);
	HMONITOR GetPrimaryMonitor(MONITORINFO* pmi = NULL);
	void StorePreMinimizeMonitor();

	LRESULT OnGetMinMaxInfo(LPMINMAXINFO pInfo);

	LRESULT OnSize(bool bResizeRCon = true, WPARAM wParam = 0);
	LRESULT OnSizing(WPARAM wParam, LPARAM lParam);
	LRESULT OnMoving(LPRECT prcWnd = NULL, bool bWmMove = false);
	LRESULT OnWindowPosChanged(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnWindowPosChanging(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnDpiChanged(UINT dpiX, UINT dpiY, LPRECT prcSuggested, bool bResizeWindow, DpiChangeSource src);
	LRESULT OnDisplayChanged(UINT bpp, UINT screenWidth, UINT screenHeight);
	void OnSizePanels(COORD cr);
	void OnConsoleResize();

	bool isSizing(UINT nMouseMsg = 0);
	void BeginSizing();
	void SetSizingFlags(DWORD nSetFlags = MOUSE_SIZING_BEGIN);
	void ResetSizingFlags(DWORD nDropFlags = MOUSE_SIZING_BEGIN|MOUSE_SIZING_TODO);
	void EndSizing(UINT nMouseMsg = 0);

	bool InMinimizing(WINDOWPOS *p = NULL);

	HRGN CreateWindowRgn();
	HRGN CreateWindowRgn(bool abRoundTitle, int anX, int anY, int anWndWidth, int anWndHeight);

	bool isCaptionHidden(ConEmuWindowMode wmNewMode = wmCurrent) const;
	bool isWin10InvisibleFrame(const ConEmuWindowMode wmNewMode = wmCurrent) const;
	bool isSelfFrame(const ConEmuWindowMode wmNewMode = wmCurrent) const;
	UINT GetSelfFrameWidth() const;
	void StartForceShowFrame();
	void StopForceShowFrame();
	void DisableThickFrame(bool flag);

	class ThickFrameDisabler
	{
		CConEmuSize& conemu;
		bool disabled = false;
	public:
		ThickFrameDisabler(CConEmuSize& _conemu, bool _disable = false);
		~ThickFrameDisabler();
		void Disable();
		void Enable();
	};

protected:
	RECT CalcMargins_Win10Frame() const;
	RECT CalcMargins_FrameCaption(DWORD/*enum ConEmuMargins*/ mg, ConEmuWindowMode wmNewMode = wmCurrent) const;
	RECT CalcMargins_TabBar(DWORD/*enum ConEmuMargins*/ mg) const;
	RECT CalcMargins_StatusBar() const;
	RECT CalcMargins_Padding() const;
	RECT CalcMargins_Scrolling() const;
	RECT CalcMargins_VisibleFrame(LPRECT prcFrame = NULL) const;
	RECT CalcMargins_InvisibleFrame() const;
	static LRESULT OnDpiChangedCall(LPARAM lParam);
	bool FixWindowRect(RECT& rcWnd, ConEmuBorders nBorders, bool bPopupDlg = false);
	bool FixPosByStartupMonitor(const HMONITOR hStartMon);
	RECT GetVirtualScreenRect(bool abFullScreen);
	void StoreNormalRect(const RECT* prcWnd);
	RECT SetNormalWindowSize();
	void LogTileModeChange(LPCWSTR asPrefix, ConEmuWindowCommand Tile, bool bChanged, const RECT& rcSet, LPRECT prcAfter, HMONITOR hMon);
	bool CheckDpiOnMoving(WINDOWPOS *p);
	void EvalNewNormalPos(const MONITORINFO& miOld, HMONITOR hNextMon, const MONITORINFO& miNew, const RECT& rcOld, RECT& rcNew);
	BOOL AnimateWindow(DWORD dwTime, DWORD dwFlags);

	friend class CSettings;
	friend class CSetDlgButtons;
	friend struct Settings;
};
