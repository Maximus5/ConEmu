#pragma once

//#define SKIP_ALL_FILLRECT
class CVirtualConsole;
class CRealConsole;

class CConEmuChild
{
public:
	CConEmuChild();
	~CConEmuChild();

	LRESULT CALLBACK ChildWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
public:
	LRESULT OnPaint(WPARAM wParam, LPARAM lParam);
	LRESULT OnSize(WPARAM wParam, LPARAM lParam);
	HWND Create();
	void Invalidate();
	void Validate();

protected:
	UINT mn_MsgTabChanged;
	BOOL mb_Invalidated;
#ifdef _DEBUG
	friend class CVirtualConsole;
	friend class CRealConsole;
#endif
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
	WNDPROC mpfn_ScrollProc;
};
