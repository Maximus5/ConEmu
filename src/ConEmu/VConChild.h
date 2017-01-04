
/*
Copyright (c) 2009-2017 Maximus5
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

#define SKIP_HIDE_TIMER
//#define SKIP_ALL_FILLRECT
class CVirtualConsole;
class CRealConsole;

#include "../common/MTimer.h"

class CConEmuChild
{
	private:
		CVirtualConsole* mp_VCon;
	public:
		CConEmuChild(CVirtualConsole* pOwner);
	protected:
		virtual ~CConEmuChild();
		LRESULT OnSize(WPARAM wParam, LPARAM lParam);
		LRESULT OnMove(WPARAM wParam, LPARAM lParam);
	public:
		static LRESULT WINAPI ChildWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
		static LRESULT WINAPI BackWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
	public:
		bool isAlreadyDestroyed();
		void DoDestroyDcWindow();
		static void ProcessVConClosed(CVirtualConsole* apVCon, bool abPosted = false);

		LRESULT OnPaint();
		LRESULT OnPaintGaps(HDC hdc);
		HWND CreateView();
		HWND GetView();
		HWND GetBack();
		bool ShowView(int nShowCmd);
		void Invalidate();
		bool IsInvalidatePending();
	protected:
		LONG mn_InvalidateViewPending, mn_WmPaintCounter;
		void InvalidateView();
		void InvalidateBack();
	public:
		//void Validate();
		void Redraw(bool abRepaintNow = false);

		void SetRedraw(bool abRedrawEnabled);

		void SetVConSizePos(const RECT& arcBack, bool abReSize = true);
		void SetVConSizePos(const RECT& arcBack, const RECT& arcDC, bool abReSize = true);
		void OnVConSizePosChanged(); // Status bar columns

		void SetScroll(bool abEnabled, int anTop, int anVisible, int anHeight);
		bool InScroll();

		void CheckPostRedraw();

		bool TrackMouse();
		void OnAlwaysShowScrollbar(bool abSync = true);

		int IsDcLocked(RECT* CurrentConLockedRect);
		void LockDcRect(bool bLock, RECT* Rect = NULL);

		void SetAutoCopyTimer(bool bEnabled);

		void PostDetach(bool bSendCloseConsole /*= false*/);

		void PostRestoreChildFocus();
		void RestoreChildFocusPending(bool abSetPending);
		bool mb_RestoreChildFocusPending;

#ifdef _DEBUG
	public:
		HWND    hDlgTest;
		static  INT_PTR CALLBACK DbgChildDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
		void    CreateDbgDlg();
#endif

	protected:
		void PostOnVConClosed();
		virtual void OnDestroy() = 0; // WM_DESTROY
		DWORD mn_AlreadyDestroyed;

		#define CRITICAL_DCWND_STYLES (WS_CAPTION|WS_VSCROLL|WS_HSCROLL|WS_SYSMENU|WS_THICKFRAME|WS_GROUP|WS_TABSTOP)
		DWORD mn_WndDCStyle;
		DWORD mn_WndDCExStyle;
		HWND mh_WndDC;
		HWND mh_WndBack; // скроллинг и фон
		HWND mh_LastGuiChild;
		long mn_VConTerminatedPosted; // == 0, set when post destroing
		#ifdef _DEBUG
		UINT mn_MsgCreateDbgDlg;
		#endif
		UINT mn_MsgTabChanged;
		UINT mn_MsgPostFullPaint;
		UINT mn_MsgSavePaneSnapshot;
		UINT mn_MsgDetachPosted;
		UINT mn_MsgRestoreChildFocus;
		bool mb_PostFullPaint;
		bool mb_DisableRedraw;
#ifdef _DEBUG
		friend class CVirtualConsole;
		friend class CRealConsole;
#endif
		struct tag_Caret
		{
			UINT  X, Y;
			UINT  nWidth, nHeight;
			BOOL  bVisible;
			BOOL  bCreated;
		} Caret;
		DWORD mn_LastPostRedrawTick;
		bool  mb_IsPendingRedraw, mb_RedrawPosted;

		CTimer m_TAutoCopy; // TIMER_AUTOCOPY

		bool mb_ScrollDisabled, mb_ScrollVisible, mb_Scroll2Visible, mb_ScrollAutoPopup, mb_VTracking;
		CTimer m_TScrollShow; // TIMER_SCROLL_SHOW
		CTimer m_TScrollHide; // TIMER_SCROLL_HIDE
		#ifndef SKIP_HIDE_TIMER
		CTimer m_TScrollCheck; // TIMER_SCROLL_CHECK
		#endif

		BYTE m_LastAlwaysShowScrollbar;
		SCROLLINFO m_si;
	public:
		bool CheckMouseOverScroll(bool abCheckVisible = false);
	protected:
		bool CheckScrollAutoPopup();
		void ShowScroll(bool abImmediate);
		void HideScroll(bool abImmediate);
		void MySetScrollInfo(bool abSetEnabled, bool abEnableValue);
		bool mb_ScrollRgnWasSet;
		void UpdateScrollRgn(bool abForce = false);

		struct LockDcInfo
		{
			BOOL  bLocked;
			DWORD nLockTick;
			RECT  rcScreen;
			RECT  rcCon;
		} m_LockDc;
};
