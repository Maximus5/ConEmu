
#pragma once

// For ChildGui attach
extern RECT    grcConEmuClient;
// For ChildGui attach
extern BOOL    gbAttachGuiClient;
// For ChildGui attach: success
extern BOOL    gbGuiClientAttached;
// For ChildGui attach: is we need to show ChildGui outside of ConEmu tab
extern BOOL    gbGuiClientExternMode;
#ifdef _DEBUG
extern HHOOK   ghGuiClientRetHook;
#endif

enum GuiCui
{
	e_Alone    = 0,
	e_ChildGui = /*2*/ IMAGE_SUBSYSTEM_WINDOWS_GUI,
	e_ChildCui = /*3*/ IMAGE_SUBSYSTEM_WINDOWS_CUI,
};
// XxxPortable.exe starts xxx.exe which we have to attach (IMAGE_SUBSYSTEM_WINDOWS_[G|C]UI)
extern GuiCui  gnAttachPortableGuiCui;
// Remember shifts of ChildGui inside of ConEmu window
extern struct GuiStylesAndShifts gGuiClientStyles;
// Let's hook ShowWindow
extern HWND    ghAttachGuiClient;
// Last call of SetFocus form ChildGui
extern HWND    ghAttachGuiFocused;
// enum ATTACHGUIAPP_FLAGS
extern DWORD   gnAttachGuiClientFlags;
// ChildGui Window Style and StyleEx
extern DWORD   gnAttachGuiClientStyle, gnAttachGuiClientStyleEx;

bool CheckCanCreateWindow(LPCSTR lpClassNameA, LPCWSTR lpClassNameW, DWORD& dwStyle, DWORD& dwExStyle, HWND& hWndParent, BOOL& bAttachGui, BOOL& bStyleHidden);
void ReplaceGuiAppWindow(BOOL abStyleHidden);
void OnGuiWindowAttached(HWND hWindow, HMENU hMenu, LPCSTR asClassA, LPCWSTR asClassW, DWORD anStyle, DWORD anStyleEx, BOOL abStyleHidden, int anFromShowWindow=-1);
void OnShowGuiClientWindow(HWND hWnd, int &nCmdShow, BOOL &rbGuiAttach, BOOL &rbInactive);
void OnPostShowGuiClientWindow(HWND hWnd, int nCmdShow);
bool OnSetGuiClientWindowPos(HWND hWnd, HWND hWndInsertAfter, int &X, int &Y, int &cx, int &cy, UINT uFlags);
void SetGuiExternMode(BOOL abUseExternMode, LPRECT prcOldPos = nullptr);
void AttachGuiWindow(HWND hOurWindow);
RECT AttachGuiClientPos(DWORD anStyle = 0, DWORD anStyleEx = 0);
void SetConEmuHkWindows(HWND hDcWnd, HWND hBackWnd);
