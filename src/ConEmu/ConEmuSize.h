
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

class CConEmuSize
{
private:
	CConEmuMain* mp_ConEmu;

protected:
	struct IdealRectInfo
	{
		// Current Ideal rect
		RECT  rcIdeal;
		// TODO: Reserved fields
		RECT  rcClientMargins; // (TabBar + StatusBar) at storing moment
		COORD crConsole;       // Console size in cells at storing moment
		SIZE  csFont;          // VCon Font size (Width, Height) at storing moment
	} mr_Ideal;

	bool mb_InSetQuakeMode;
	LONG mn_IgnoreQuakeActivation;
	DWORD mn_LastQuakeShowHide;
public:
	void SetIgnoreQuakeActivation(bool bNewValue);
protected:
	int mn_QuakePercent; // 0 - отключен, иначе (>0 && <=100) - идет анимация Quake

	struct QuakePrevSize {
		bool bWasSaved;
		bool bWaitReposition; // Требуется смена позиции при OnHideCaption
		CESize wndWidth, wndHeight; // Консоль
		LONG wndX, wndY; // GUI (Visual Position)
		DWORD nFrame; // it's BYTE, DWORD here for alignment
		ConEmuWindowMode WindowMode;
		IdealRectInfo rcIdealInfo;
		bool MinToTray;
		// Used in GetDefaultRect/GetDefaultSize after Quake was slided up (hidden)
		RECT PreSlidedSize;
		// helper methods
		void Save(const CESize& awndWidth, const CESize& awndHeight, const LONG& awndX, const LONG& awndY, const BYTE& anFrame, const ConEmuWindowMode& aWindowMode, const IdealRectInfo& arcIdealInfo, const bool& abMinToTray);
		ConEmuWindowMode Restore(CESize& rwndWidth, CESize& rwndHeight, LONG& rwndX, LONG& rwndY, BYTE& rnFrame, IdealRectInfo& rrcIdealInfo, bool& rbMinToTray);
		void SetNonQuakeDefaults();
	} m_QuakePrevSize;

	bool CheckQuakeRect(LPRECT prcWnd);

	ConEmuWindowCommand m_TileMode;

	struct {
		RECT rcNewPos;
		bool bInJump;
		bool bFullScreen, bMaximized;
	} m_JumpMonitor;

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
	} m_Snapping;

	LONG mn_InResize;
	RECT mrc_StoredNormalRect;

	POINT ptFullScreenSize; // size for GetMinMaxInfo in Fullscreen mode
	
	ConEmuWindowMode WindowMode;           // wmNormal/wmMaximized/wmFullScreen
	ConEmuWindowMode changeFromWindowMode; // wmNotChanging/rmNormal/rmMaximized/rmFullScreen
	
	bool isRestoreFromMinimized;
	bool isWndNotFSMaximized; // ставится в true, если при переходе в FullScreen - был Maximized
	bool isQuakeMinimized;    // изврат, для случая когда "Quake" всегда показывается на таскбаре

	friend class CSetPgSizePos;
	// The size of the main window (cells/pixels/percents)
	CESize WndWidth, WndHeight;
	// Visual position of the upper-left corner (in pixels)
	// May differ from its real one (obtained from GetWindowRect) due to invisible parts of the frame
	POINT WndPos;
	// Processing functions
	POINT VisualPosFromReal(const int x, const int y);
	POINT RealPosFromVisual(const int x, const int y);

	bool mb_LockWindowRgn;
	bool mb_LockShowWindow;
	LONG mn_IgnoreSizeChange;
	LONG mn_InSetWindowPos;

	HMONITOR mh_MinFromMonitor;

	enum {
		fsf_Hide = 0,     // Рамка и заголовок спрятаны
		fsf_WaitShow = 1, // Запущен таймер показа рамки
		fsf_Show = 2,     // Рамка показана
	} m_ForceShowFrame;
	void StartForceShowFrame();
	void StopForceShowFrame();

	bool mb_InShowMinimized; // true на время выполнения ShowWindow(SW_SHOWMIN...)

	bool mb_LastRgnWasNull;

	// -- Не используется по сути!!!
	//private:
	//bool mb_PostUpdateWindowSize;
	//public:
	//void SetPostUpdateWindowSize(bool bValue);
	//bool isPostUpdateWindowSize() { return mb_PostUpdateWindowSize; };

protected:
	struct MonitorInfoCache
	{
		HMONITOR hMon;
		MONITORINFO mi;
		// if DWMWA_EXTENDED_FRAME_BOUNDS succeeded
		bool HasWin10Frame;
		RECT Win10Frame;
		// Per-monitor DPI
		int Xdpi, Ydpi;
	};
	MArray<MonitorInfoCache> monitors;
public:
	void ReloadMonitorInfo();

public:
	CConEmuSize(CConEmuMain* pOwner);
	virtual ~CConEmuSize();

protected:
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
	RECT GetGuiClientRect();
	RECT GetIdealRect();
	void StoreIdealRect();
	void UpdateInsideRect(RECT rcNewPos);

	void RecreateControls(bool bRecreateTabbar, bool bRecreateStatus, bool bResizeWindow, LPRECT prcSuggested = NULL);
	bool SetQuakeMode(BYTE NewQuakeMode, ConEmuWindowMode nNewWindowMode = wmNotChanging, bool bFromDlg = false);

	static LPCWSTR FormatTileMode(ConEmuWindowCommand Tile, wchar_t* pchBuf, size_t cchBufMax);
	bool SetTileMode(ConEmuWindowCommand Tile);
	RECT GetTileRect(ConEmuWindowCommand Tile, const MONITORINFO& mi);
	ConEmuWindowMode GetWindowMode();
	ConEmuWindowMode GetChangeFromWindowMode();
	bool IsWindowModeChanging();
	bool SetWindowMode(ConEmuWindowMode inMode, bool abForce = false, bool abFirstShow = false);
	bool SetWindowPosSize(LPCWSTR asX, LPCWSTR asY, LPCWSTR asW, LPCWSTR asH);
	void SetWindowPosSizeParam(wchar_t acType, LPCWSTR asValue);
	ConEmuWindowCommand GetTileMode(bool Estimate, MONITORINFO* pmi = NULL);
	ConEmuWindowCommand EvalTileMode(const RECT& rcWnd, MONITORINFO* pmi = NULL);
	bool IsSizeFree(ConEmuWindowMode CheckMode = wmFullScreen);
	bool IsSizePosFree(ConEmuWindowMode CheckMode = wmFullScreen);
	bool IsCantExceedMonitor();
	bool IsPosLocked();
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

	void UpdateWindowRgn(int anX = -1, int anY = -1, int anWndWidth = -1, int anWndHeight = -1);
	bool ShowMainWindow(int anCmdShow, bool abFirstShow = false);
	void CheckTopMostState();
	bool SizeWindow(const CESize sizeW, const CESize sizeH);

	void EvalVConCreateSize(CVirtualConsole* apVCon, uint& rnTextWidth, uint& rnTextHeight);

	HWND FindNextSiblingApp(bool bActivate);

public:
	HMONITOR GetNearestMonitor(MONITORINFO* pmi = NULL, LPCRECT prcWnd = NULL);
	HMONITOR GetPrimaryMonitor(MONITORINFO* pmi = NULL);
	void StorePreMinimizeMonitor();

	LRESULT OnGetMinMaxInfo(LPMINMAXINFO pInfo);

	LRESULT OnSize(bool bResizeRCon = true, WPARAM wParam = 0, WORD newClientWidth = (WORD)-1, WORD newClientHeight = (WORD)-1);
	LRESULT OnSizing(WPARAM wParam, LPARAM lParam);
	LRESULT OnMoving(LPRECT prcWnd = NULL, bool bWmMove = false);
	LRESULT OnWindowPosChanged(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnWindowPosChanging(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnDpiChanged(UINT dpiX, UINT dpiY, LPRECT prcSuggested, bool bResizeWindow, DpiChangeSource src);
	LRESULT OnDisplayChanged(UINT bpp, UINT screenWidth, UINT screenHeight);
	void OnSizePanels(COORD cr);
	void OnConsoleResize(bool abPosted = FALSE);

	bool isSizing(UINT nMouseMsg = 0);
	void BeginSizing(bool bFromStatusBar);
	void SetSizingFlags(DWORD nSetFlags = MOUSE_SIZING_BEGIN);
	void ResetSizingFlags(DWORD nDropFlags = MOUSE_SIZING_BEGIN|MOUSE_SIZING_TODO);
	void EndSizing(UINT nMouseMsg = 0);

	bool InMinimizing(WINDOWPOS *p = NULL);

	HRGN CreateWindowRgn(bool abTestOnly = false);
	HRGN CreateWindowRgn(bool abTestOnly, bool abRoundTitle, int anX, int anY, int anWndWidth, int anWndHeight);

protected:
	RECT CalcMargins_Win10Frame();
	RECT CalcMargins_FrameCaption(DWORD/*enum ConEmuMargins*/ mg, ConEmuWindowMode wmNewMode = wmCurrent);
	RECT CalcMargins_TabBar(DWORD/*enum ConEmuMargins*/ mg);
	RECT CalcMargins_StatusBar();
	RECT CalcMargins_Padding();
	RECT CalcMargins_Scrolling();
	RECT CalcMargins_VisibleFrame(LPRECT prcFrame = NULL);
	RECT CalcMargins_InvisibleFrame();
	static LRESULT OnDpiChangedCall(LPARAM lParam);
	bool FixWindowRect(RECT& rcWnd, DWORD nBorders /* enum of ConEmuBorders */, bool bPopupDlg = false);
	bool FixPosByStartupMonitor(const HMONITOR hStartMon);
	RECT GetVirtualScreenRect(bool abFullScreen);
	void StoreNormalRect(RECT* prcWnd);
	void UpdateIdealRect(RECT rcNewIdeal);
	RECT SetNormalWindowSize();
	void LogTileModeChange(LPCWSTR asPrefix, ConEmuWindowCommand Tile, bool bChanged, const RECT& rcSet, LPRECT prcAfter, HMONITOR hMon);
	bool CheckDpiOnMoving(WINDOWPOS *p);
	void EvalNewNormalPos(const MONITORINFO& miOld, HMONITOR hNextMon, const MONITORINFO& miNew, const RECT& rcOld, RECT& rcNew);
	BOOL AnimateWindow(DWORD dwTime, DWORD dwFlags);

	friend class CSettings;
	friend class CSetDlgButtons;
	friend struct Settings;
};
