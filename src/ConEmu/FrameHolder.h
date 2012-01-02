
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

#include "Header.h"


class CFrameHolder
{
private:
	BOOL mb_Initialized;
	BOOL mb_WasGlassDraw;
public:
	BOOL mb_NcActive;
public:
	//TODO: Во время анимации Maximize/Restore/Minimize заголовок отрисовывается 
	//TODO: системой, в итоге мелькает текст и срезаются табы                    
	//TODO: Сделаем, пока, чтобы текст хотя бы не мелькал...                     
	BOOL mb_NcAnimate;
public:
	CFrameHolder();
	virtual ~CFrameHolder();
	void InitFrameHolder();
	// returns false if message not handled
	bool ProcessNcMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult);
public:
	// Next functions must me defined in the ancestor
	virtual void ShowSysmenu(int x, int y) = 0; // параметр HWND нафиг не нужен
	//virtual void ShowTabMenu(int anConsole, int anTab, int x, int y) = 0;
	virtual void OnPaintClient(HDC hdc, int width, int height) = 0;
	virtual void CalculateTabPosition(const RECT &rcWindow, const RECT &rcCaption, RECT* rcTabs);
	virtual void CalculateCaptionPosition(const RECT &rcWindow, RECT* rcCaption);
protected:
	// internal messages processing
	virtual LRESULT OnWindowPosChanged(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
	LRESULT OnDwmMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnNcHitTest(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//LRESULT OnNcLButtonDown(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnNcCalcSize(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnNcActivate(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnNcPaint(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnPaint(HWND hWnd, BOOL abForceGetDc);
	// NC helpers
	LRESULT NC_Wrapper(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	// NC Invalidating
	void NC_Redraw(HWND hWnd);
	int mn_RedrawLockCount;
	bool mb_RedrawRequested;
	void RedrawLock();
	void RedrawUnlock();
public:
	void RedrawTabPanel();
private:
	//OSVERSIONINFO m_OSVer;
#if 0
	void PaintFrame2k(HWND hWnd, HDC hdc, RECT &cr);
#endif
	int mn_WinCaptionHeight, mn_FrameWidth, mn_FrameHeight, mn_OurCaptionHeight, mn_TabsHeight;
public:
	void RecalculateFrameSizes();
	int GetFrameWidth(); // ширина рамки окна
	int GetFrameHeight(); // высота рамки окна
	int GetCaptionHeight(); // высота НАШЕГО заголовка (с учетом табов)
	int GetTabsHeight(); // высота табов
	int GetCaptionDragHeight(); // высота части заголовка, который оставляем для "таскания" окна
	int GetWinCaptionHeight(); // высота заголовка в окнах Windows по умолчанию (без учета табов)
	void GetIconShift(POINT& IconShift);
};
