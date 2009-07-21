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
	
	void SetCaret ( int Visible, UINT X=0, UINT Y=0, UINT nWidth=0, UINT nHeight=0 );

	void SetRedraw(BOOL abRedrawEnabled);

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
