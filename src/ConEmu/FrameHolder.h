
/*
Copyright (c) 2009-present Maximus5
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

class CConEmuMain;

class CFrameHolder
{
private:
	CConEmuMain* mp_ConEmu;
	bool mb_Initialized = false;
	#if defined(CONEMU_TABBAR_EX)
	bool mb_WasGlassDraw = false;
	#endif
	bool mb_AllowPreserveClient = false;// по возможности(!) - разрешить сохранение клиентской области (имеет приоритет перед mb_DontPreserveClient)
	bool mb_DontPreserveClient = false; // запретить сохранение клиентской области при ресайзе
	LONG mn_InNcPaint = 0;
	POINT ptLastNcClick = {};
	int mn_WinCaptionHeight = 0, mn_FrameWidth = 0, mn_FrameHeight = 0, mn_OurCaptionHeight = 0, mn_CaptionDragHeight = 0;
	RECT mrc_NcClientMargins = {};
	int mn_RedrawLockCount = 0;
	bool mb_RedrawRequested = false;

	#if 0
	void PaintFrame2k(HWND hWnd, HDC hdc, RECT &cr);
	#endif

public:
	bool mb_NcActive = true;
	//TODO: Во время анимации Maximize/Restore/Minimize заголовок отрисовывается
	//TODO: системой, в итоге мелькает текст и срезаются табы
	//TODO: Сделаем, пока, чтобы текст хотя бы не мелькал...
	bool mb_NcAnimate = false;

public:
	CFrameHolder();
	virtual ~CFrameHolder();
	void InitFrameHolder();
	// returns false if message not handled
	bool ProcessNcMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult);
	bool SetDontPreserveClient(bool abSwitch);
	bool SetAllowPreserveClient(bool abSwitch);
	bool isInNcPaint();
public:
	// Next functions must me defined in the ancestor
	//virtual void ShowSysmenu(int x=-32000, int y=-32000, bool bAlignUp = false) = 0;
	//virtual void ShowTabMenu(int anConsole, int anTab, int x, int y) = 0;
	//virtual void OnPaintClient(HDC hdc/*, int width, int height*/) = 0;
public:
	void RedrawFrame();
	void SetFrameActiveState(bool bActive);
public:
	void RecalculateFrameSizes();
	UINT GetWinFrameWidth(); // системная ширина/высота рамки окна
	UINT GetWinCaptionHeight(); // высота заголовка в окнах Windows по умолчанию (без учета табов)
	int GetCaptionHeight(); // высота НАШЕГО заголовка (с учетом табов)
	int GetCaptionDragHeight(); // высота части заголовка, который оставляем для "таскания" окна
	void GetIconShift(POINT& IconShift);

protected:
	// internal messages processing
	bool OnDwmCompositionChanged(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult);
	bool OnNcUahDrawFrame(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult);
	bool OnNcUahDrawCaption(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult);
	bool OnGetText(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult);
	bool OnKeyboardMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult);
	bool OnMouseMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult);
	bool OnNcLButtonDblClk(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult);
	bool OnNcMouseMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult);
	bool OnSysCommand(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult);
	bool OnEraseBkgnd(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult);
	bool OnNcHitTest(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult);
	bool OnNcCalcSize(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult);
	bool OnNcActivate(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult);
	bool OnNcPaint(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult);
	bool OnPaint(HWND hWnd, HDC hdc, UINT uMsg, LRESULT& lResult);
	// NC helpers
	LRESULT DoDwmMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT DoNcHitTest(const POINT& point, int width, int height, LPARAM def_hit_test = 0);
	LRESULT NC_Wrapper(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	// NC Invalidating
	void NC_Redraw();
	void RedrawLock();
	void RedrawUnlock();
};
