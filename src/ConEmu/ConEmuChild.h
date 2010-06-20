
/*
Copyright (c) 2009-2010 Maximus5
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
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

//#define SKIP_ALL_FILLRECT
class CVirtualConsole;
class CRealConsole;

class CConEmuChild
{
public:
	CConEmuChild();
	~CConEmuChild();

	LRESULT ChildWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
public:
	LRESULT OnPaint();
	LRESULT OnSize(WPARAM wParam, LPARAM lParam);
	HWND Create();
	void Invalidate();
	void Validate();
	void Redraw();
	
	void SetRedraw(BOOL abRedrawEnabled);

	void CheckPostRedraw();

protected:
	UINT mn_MsgTabChanged;
	UINT mn_MsgPostFullPaint;
	BOOL mb_PostFullPaint;
	BOOL mb_DisableRedraw;
#ifdef _DEBUG
	friend class CVirtualConsole;
	friend class CRealConsole;
#endif
    struct tag_Caret {
    	UINT  X, Y;
    	UINT  nWidth, nHeight;
    	BOOL  bVisible;
    	BOOL  bCreated;
    } Caret;
    DWORD mn_LastPostRedrawTick;
    BOOL  mb_IsPendingRedraw, mb_RedrawPosted;
};

class CConEmuBack
{
public:
	CConEmuBack();
	~CConEmuBack();

	HWND mh_WndBack;
	HWND mh_WndScroll; UINT mn_ScrollWidth; BOOL mb_ScrollVisible;
	HBRUSH mh_BackBrush;
	COLORREF mn_LastColor;

	HWND Create();
	void Resize();
	void Refresh();
	void Invalidate();
	void RePaint();
	BOOL TrackMouse();

	static LRESULT CALLBACK BackWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK ScrollWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
protected:
	RECT mrc_LastClient;
	bool mb_LastTabVisible;
	int mn_ColorIdx;
	bool mb_VTracking;
	//WNDPROC mpfn_ScrollProc;
//protected:
//	// Theming
//    typedef HANDLE (WINAPI* FOpenThemeData)(HWND,LPCWSTR);
//    FOpenThemeData mfn_OpenThemeData;
//    typedef HANDLE (WINAPI* FOpenThemeDataEx)(HWND,LPCWSTR,DWORD);
//    FOpenThemeData mfn_OpenThemeDataEx;
//    typedef HRESULT (WINAPI* FCloseThemeData)(HANDLE);
//    FCloseThemeData mfn_CloseThemeData;
//    typedef BOOL (WINAPI* FAppThemed)();
//    HMODULE mh_UxTheme;
//    HANDLE  mh_ThemeData;
};
