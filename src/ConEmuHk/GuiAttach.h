
#pragma once

extern RECT    grcConEmuClient;   // Для аттача гуевых окон
extern BOOL    gbAttachGuiClient; // Для аттача гуевых окон
extern HWND    ghAttachGuiClient; // Чтобы ShowWindow перехватить

bool CheckCanCreateWindow(LPCSTR lpClassNameA, LPCWSTR lpClassNameW, DWORD& dwStyle, DWORD& dwExStyle, HWND& hWndParent, BOOL& bAttachGui, BOOL& bStyleHidden);
void ReplaceGuiAppWindow(BOOL abStyleHidden);
void OnGuiWindowAttached(HWND hWindow, HMENU hMenu, LPCSTR asClassA, LPCWSTR asClassW, DWORD anStyle, DWORD anStyleEx, BOOL abStyleHidden);
void OnShowGuiClientWindow(HWND hWnd, int &nCmdShow, BOOL &rbGuiAttach);
void OnPostShowGuiClientWindow(HWND hWnd, int nCmdShow);
bool OnSetGuiClientWindowPos(HWND hWnd, HWND hWndInsertAfter, int &X, int &Y, int &cx, int &cy, UINT uFlags);
