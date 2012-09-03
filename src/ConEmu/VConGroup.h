
/*
Copyright (c) 2009-2012 Maximus5
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

#include "VConRelease.h"
#include "../common/RConStartArgs.h"

class CVirtualConsole;
class CVConGuard;

class CVConGroup
{
protected:
	CVirtualConsole* mp_Item;     // консоль, к которой привязан этот "Pane"
	RConStartArgs::SplitType m_SplitType; // eSplitNone/eSplitHorz/eSplitVert
	UINT mn_SplitPercent10; // (0.1% - 99.9%)*10
	CVConGroup *mp_Grp1, *mp_Grp2; // Ссылки на "дочерние" панели
	CVConGroup *mp_Parent; // Ссылка на "родительскую" панель
	bool mb_ResizeFlag; // взводится в true для корня, когда в группе что-то меняется
	void SetResizeFlags();

	CVConGroup* GetRootGroup();
	static CVConGroup* GetRootOfVCon(CVirtualConsole* apVCon);
	CVConGroup* GetAnotherGroup();
	void MoveToParent(CVConGroup* apParent);
	void RepositionVCon(RECT rcNewCon, bool bVisible);
	void CalcSplitRect(RECT rcNewCon, RECT& rcCon1, RECT& rcCon2);
	void CalcSplitRootRect(RECT rcAll, RECT& rcCon, CVConGroup* pTarget = NULL);
	void CalcSplitConSize(COORD size, COORD& sz1, COORD& sz2);
	void ShowAllVCon(int nShowCmd);
	static void ShowActiveGroup(CVirtualConsole* pOldActive);

	void GetAllTextSize(SIZE& sz, bool abMinimal = false);
	void SetConsoleSizes(const COORD& size, bool abSync);
	
protected:
	//static CVirtualConsole* mp_VCon[MAX_CONSOLE_COUNT];

	//static CVirtualConsole* mp_VActive;
	//static bool mb_CreatingActive, mb_SkipSyncSize;

	//static CRITICAL_SECTION mcs_VGroups;
	//static CVConGroup* mp_VGroups[MAX_CONSOLE_COUNT*2]; // на каждое разбиение добавляется +Parent

	//static CVirtualConsole* mp_GrpVCon[MAX_CONSOLE_COUNT];

	//static COORD m_LastConSize; // console size after last resize (in columns and lines)

private:
	static CVirtualConsole* CreateVCon(RConStartArgs *args, CVirtualConsole*& ppVConI);
	
	static CVConGroup* CreateVConGroup();
	CVConGroup* SplitVConGroup(RConStartArgs::SplitType aSplitType = RConStartArgs::eSplitHorz/*eSplitVert*/, UINT anPercent10 = 500);
	
	CVConGroup(CVConGroup *apParent);

public:
	~CVConGroup();

public:
	static void Initialize();
	static void Deinitialize();
	static CVirtualConsole* CreateCon(RConStartArgs *args, BOOL abAllowScripts = FALSE);
	static void OnVConDestroyed(CVirtualConsole* apVCon);

public:
	static bool isActive(CVirtualConsole* apVCon, bool abAllowGroup = true);
	static bool isVisible(CVirtualConsole* apVCon);
	static bool isValid(CRealConsole* apRCon);
	static bool isValid(CVirtualConsole* apVCon);
	static bool isVConExists(int nIdx);
	static bool isConSelectMode();
	static bool isFilePanel(bool abPluginAllowed=false);
	static bool isNtvdm(BOOL abCheckAllConsoles=FALSE);
	static bool isOurConsoleWindow(HWND hCon);
	static bool isChildWindow();
	static bool isPictureView();
	static bool isEditor();
	static bool isViewer();
	static bool isFar(bool abPluginRequired=false);
	static bool isFarExist(CEFarWindowType anWindowType=fwt_Any, LPWSTR asName=NULL, CVConGuard* rpVCon=NULL);
	static bool isVConHWND(HWND hChild, CVConGuard* rpVCon = NULL);
	static bool isConsolePID(DWORD nPID);
	static DWORD GetFarPID(BOOL abPluginRequired=FALSE);

	static int  GetActiveVCon(CVConGuard* pVCon = NULL, int* pAllCount = NULL);
	static int  GetVConIndex(CVirtualConsole* apVCon);
	static bool GetVCon(int nIdx, CVConGuard* pVCon = NULL);
	static bool GetVConFromPoint(POINT ptScreen, CVConGuard* pVCon = NULL);
	static bool GetProgressInfo(short* pnProgress, BOOL* pbActiveHasProgress, BOOL* pbWasError);

	static void StopSignalAll();
	static void DestroyAllVCon();
	static void OnAlwaysShowScrollbar(bool abSync = true);
	static void OnUpdateScrollInfo();
	static void OnUpdateFarSettings();
	static void OnUpdateTextColorSettings(BOOL ChangeTextAttr = TRUE, BOOL ChangePopupAttr = TRUE);
	static bool OnCloseQuery();
	static bool OnScClose();
	static void OnDestroyConEmu();
	static void OnVConClosed(CVirtualConsole* apVCon);
	static void OnUpdateProcessDisplay(HWND hInfo);
	static void OnDosAppStartStop(HWND hwnd, StartStopType sst, DWORD idChild);
	static void UpdateWindowChild(CVirtualConsole* apVCon);
	static void RePaint();
	static void Update(bool isForce = false);
	static HWND DoSrvCreated(DWORD nServerPID, HWND hWndCon, DWORD& t1, DWORD& t2, DWORD& t3, int& iFound, HWND& hWndBack);
	static void OnVConCreated(CVirtualConsole* apVCon, const RConStartArgs *args);
	static void OnGuiFocused(BOOL abFocus, BOOL abForceChild = FALSE);

	static bool Activate(CVirtualConsole* apVCon);
	static void MoveActiveTab(CVirtualConsole* apVCon, bool bLeftward);

	static void OnUpdateGuiInfoMapping(ConEmuGuiMapping* apGuiInfo);
	static void OnPanelViewSettingsChanged();
	static void OnTaskbarSettingsChanged();

	static void MoveAllVCon(CVirtualConsole* pVConCurrent, RECT rcNewCon);
	static HRGN GetExclusionRgn(bool abTestOnly = false);
	static bool ConActivate(int nCon);
	static bool ConActivateNext(BOOL abNext);
	static DWORD CheckProcesses();
	static CRealConsole* AttachRequestedGui(LPCWSTR asAppFileName, DWORD anAppPID);
	static BOOL AttachRequested(HWND ahConWnd, const CESERVER_REQ_STARTSTOP* pStartStop, CESERVER_REQ_STARTSTOPRET* pRet);
	static int GetConCount();
	static int ActiveConNum();

	static void LogString(LPCSTR asText, BOOL abShowTime = FALSE);
	static void LogString(LPCWSTR asText, BOOL abShowTime = FALSE);
	static void LogInput(UINT uMsg, WPARAM wParam, LPARAM lParam, LPCWSTR pszTranslatedChars = NULL);

	static RECT CalcRect(enum ConEmuRect tWhat, RECT rFrom, enum ConEmuRect tFrom, CVirtualConsole* pVCon, enum ConEmuMargins tTabAction=CEM_TAB);
	static bool PreReSize(uint WindowMode, RECT rcWnd, enum ConEmuRect tFrom = CER_MAIN, bool bSetRedraw = false);
	static void SyncWindowToConsole();
	static void SyncConsoleToWindow(LPRECT prcNewWnd=NULL);
	static void LockSyncConsoleToWindow(bool abLockSync);
	static void SetAllConsoleWindowsSize(const COORD& size, /*bool updateInfo,*/ bool bSetRedraw /*= false*/, bool bResizeConEmuWnd = false);
	static void SyncAllConsoles2Window(RECT rcWnd, enum ConEmuRect tFrom = CER_MAIN, bool bSetRedraw = false);
	static void OnConsoleResize(bool abSizingToDo);
	static void ReSizePanes(RECT mainClient);

	static void SetRedraw(bool abRedrawEnabled);
	static void Redraw();
	static void InvalidateGaps();
	static void PaintGaps(HDC hDC);
	static void InvalidateAll();

	static bool OnFlashWindow(DWORD nFlags, DWORD nCount, HWND hCon);

	// Это некие сводные размеры, соответствующие тому, как если бы была
	// только одна активная консоль, БЕЗ Split-screen
	static uint TextWidth();
	static uint TextHeight();

	static RECT AllTextRect(bool abMinimal = false);

//public:
//	bool ResizeConsoles(const RECT &rFrom, enum ConEmuRect tFrom);
//	bool ResizeViews(bool bResizeRCon=true, WPARAM wParam=0, WORD newClientWidth=(WORD)-1, WORD newClientHeight=(WORD)-1);
};
