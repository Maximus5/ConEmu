#pragma once

extern HWND ghWnd, ghConWnd, ghWndDC;
extern bool gbUseChildWindow;
#define HDCWND (gbUseChildWindow ? ghWndDC : ghWnd)


#define WM_TRAYNOTIFY WM_USER+1
#define ID_SETTINGS 0xABCD
#define ID_HELP 0xABCE
#define ID_TOTRAY 0xABCF
#define ID_CONPROP 0xABCC

#define SC_RESTORE_SECRET 0x0000f122
#define SC_MAXIMIZE_SECRET 0x0000f032
#define SC_PROPERTIES_SECRET 65527

#define IID_IShellLink IID_IShellLinkW 

#define GET_X_LPARAM(inPx) ((int)(short)LOWORD(inPx))
#define GET_Y_LPARAM(inPy) ((int)(short)HIWORD(inPy))

void SyncConsoleToWindowRect(const RECT& rect);
void SetConsoleWindowSize(const COORD& size, bool updateInfo);
bool isPictureView();
LRESULT CALLBACK WndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ChildWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);