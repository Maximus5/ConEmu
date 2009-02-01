#pragma once
#include <windows.h>

class CProgressBars
{
	HWND Progressbar1, Progressbar2, hWnd;
public:
	CProgressBars(HWND hWnd, HINSTANCE g_hInstance);
	void OnTimer();
	bool isCoping();
};