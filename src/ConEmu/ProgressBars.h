#pragma once
#include <windows.h>

class CProgressBars
{
protected:
	HWND Progressbar1, Progressbar2, hWnd;
public:
	CProgressBars(HWND hWnd, HINSTANCE hInstance);
	~CProgressBars();
	void OnTimer();
	bool isCoping();
protected:
	typedef HRESULT (STDAPICALLTYPE *SetWindowThemeT)(HWND hwnd,LPCWSTR pszSubAppName,LPCWSTR pszSubIdList);
	SetWindowThemeT SetWindowThemeF;
	HMODULE mh_Uxtheme;
};