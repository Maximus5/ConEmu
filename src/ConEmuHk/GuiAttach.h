
#pragma once

extern RECT    grcConEmuClient;   // Для аттача гуевых окон
extern BOOL    gbAttachGuiClient; // Для аттача гуевых окон
extern BOOL    gbGuiClientAttached; // Для аттача гуевых окон (успешно подключились)
extern BOOL    gbGuiClientExternMode; // Если нужно показать Gui-приложение вне вкладки ConEmu
enum GuiCui
{
	e_Alone    = 0,
	e_ChildGui = /*2*/ IMAGE_SUBSYSTEM_WINDOWS_GUI,
	e_ChildCui = /*3*/ IMAGE_SUBSYSTEM_WINDOWS_CUI,
};
extern GuiCui  gnAttachPortableGuiCui; // XxxPortable.exe запускает xxx.exe который уже и нужно цеплять (IMAGE_SUBSYSTEM_WINDOWS_[G|C]UI)
extern struct GuiStylesAndShifts gGuiClientStyles; // Запомнить сдвиги окна внутри ConEmu
extern HWND    ghAttachGuiClient; // Чтобы ShowWindow перехватить
extern HWND    ghAttachGuiFocused; // Последний SetFocus из ChildGui
extern DWORD   gnAttachGuiClientFlags; // enum ATTACHGUIAPP_FLAGS
extern DWORD   gnAttachGuiClientStyle, gnAttachGuiClientStyleEx;

bool CheckCanCreateWindow(LPCSTR lpClassNameA, LPCWSTR lpClassNameW, DWORD& dwStyle, DWORD& dwExStyle, HWND& hWndParent, BOOL& bAttachGui, BOOL& bStyleHidden);
void ReplaceGuiAppWindow(BOOL abStyleHidden);
void OnGuiWindowAttached(HWND hWindow, HMENU hMenu, LPCSTR asClassA, LPCWSTR asClassW, DWORD anStyle, DWORD anStyleEx, BOOL abStyleHidden, int anFromShowWindow=-1);
void OnShowGuiClientWindow(HWND hWnd, int &nCmdShow, BOOL &rbGuiAttach, BOOL &rbInactive);
void OnPostShowGuiClientWindow(HWND hWnd, int nCmdShow);
bool OnSetGuiClientWindowPos(HWND hWnd, HWND hWndInsertAfter, int &X, int &Y, int &cx, int &cy, UINT uFlags);
void SetGuiExternMode(BOOL abUseExternMode, LPRECT prcOldPos = NULL);
void AttachGuiWindow(HWND hOurWindow);
RECT AttachGuiClientPos(DWORD anStyle = 0, DWORD anStyleEx = 0);
void SetConEmuHkWindows(HWND hDcWnd, HWND hBackWnd);
