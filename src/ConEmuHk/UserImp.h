
/*
Copyright (c) 2011 Maximus5
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

#ifndef LPCDLGTEMPLATEW
#endif

struct UserImp
{
public:
	BOOL     loadExports(BOOL abAllowLoadLibrary);
	BOOL     loadExportsFrom(HMODULE hModule);
	void     setAllowLoadLibrary();
	bool     isUser32(HMODULE hModule);
	bool     isExportsLoaded();

public:
	BOOL     allowSetForegroundWindow(DWORD dwProcessId);
	BOOL     setForegroundWindow(HWND hWnd);
	HWND     getForegroundWindow();
	DWORD    getWindowThreadProcessId(HWND hWnd, LPDWORD lpdwProcessId);
	BOOL     setWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);
	LONG_PTR getWindowLongPtrW(HWND hWnd, int nIndex);
	LONG_PTR setWindowLongPtrW(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
	HWND     getParent(HWND hWnd);
	HWND     setParent(HWND hWndChild, HWND hWndNewParent);
	HWND     findWindowEx(HWND hwndParent, HWND hwndChildAfter, LPCTSTR lpszClass, LPCTSTR lpszWindow);
	BOOL     getWindowRect(HWND hWnd, LPRECT lpRect);
	int      getSystemMetrics(int nIndex);
	BOOL     systemParametersInfoW(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni);
	BOOL     setWindowTextW(HWND hWnd, LPCWSTR lpString);
	BOOL     endDialog(HWND hDlg, INT_PTR nResult);
	BOOL     postMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	LRESULT  sendMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	INT_PTR  dialogBoxIndirectParamW(HINSTANCE hInstance, LPCDLGTEMPLATE hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
	int      getClassNameW(HWND hWnd, LPWSTR lpClassName, int nMaxCount);
	BOOL     getClientRect(HWND hWnd, LPRECT lpRect);
	HMENU    getMenu(HWND hWnd);
	BOOL     attachThreadInput(DWORD idAttach, DWORD idAttachTo, BOOL fAttach);
	HWND     getFocus();
	BOOL     isWindow(HWND hWnd);
	BOOL     isWindowVisible(HWND hWnd);
	BOOL     showWindow(HWND hWnd, int nCmdShow);
	SHORT    getKeyState(int nVirtKey);
	LRESULT  callNextHookEx(HHOOK hhk, int nCode, WPARAM wParam, LPARAM lParam);
	HHOOK    setWindowsHookExW(int idHook, HOOKPROC lpfn, HINSTANCE hMod, DWORD dwThreadId);
	BOOL     unhookWindowsHookEx(HHOOK hhk);
	int      mapWindowPoints(HWND hWndFrom, HWND hWndTo, LPPOINT lpPoints, UINT cPoints);
	UINT     registerWindowMessageW(LPCWSTR lpString);

	
private:
	HMODULE  hUser32;
	BOOL     bUserLoaded;
	BOOL     bAllowLoadLibrary;
private:
	typedef BOOL     (WINAPI* allowSetForegroundWindow_t)(DWORD dwProcessId);
	allowSetForegroundWindow_t allowSetForegroundWindow_f;
	typedef BOOL     (WINAPI* setForegroundWindow_t)(HWND hWnd);
	setForegroundWindow_t setForegroundWindow_f;
	typedef HWND     (WINAPI* getForegroundWindow_t)();
	getForegroundWindow_t getForegroundWindow_f;
	typedef DWORD    (WINAPI* getWindowThreadProcessId_t)(HWND hWnd, LPDWORD lpdwProcessId);
	getWindowThreadProcessId_t getWindowThreadProcessId_f;
	typedef BOOL     (WINAPI* setWindowPos_t)(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);
	setWindowPos_t setWindowPos_f;
	typedef LONG_PTR (WINAPI* getWindowLongPtrW_t)(HWND hWnd, int nIndex);
	getWindowLongPtrW_t getWindowLongPtrW_f;
	typedef LONG_PTR (WINAPI* setWindowLongPtrW_t)(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
	setWindowLongPtrW_t setWindowLongPtrW_f;
	typedef HWND     (WINAPI* getParent_t)(HWND hWnd);
	getParent_t getParent_f;
	typedef HWND     (WINAPI* setParent_t)(HWND hWndChild, HWND hWndNewParent);
	setParent_t setParent_f;
	typedef HWND     (WINAPI* findWindowEx_t)(HWND hwndParent, HWND hwndChildAfter, LPCTSTR lpszClass, LPCTSTR lpszWindow);
	findWindowEx_t findWindowEx_f;
	typedef BOOL     (WINAPI* getWindowRect_t)(HWND hWnd, LPRECT lpRect);
	getWindowRect_t getWindowRect_f;
	typedef int      (WINAPI* getSystemMetrics_t)(int nIndex);
	getSystemMetrics_t getSystemMetrics_f;
	typedef BOOL     (WINAPI* systemParametersInfoW_t)(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni);
	systemParametersInfoW_t systemParametersInfoW_f;
	typedef BOOL     (WINAPI* setWindowTextW_t)(HWND hWnd, LPCWSTR lpString);
	setWindowTextW_t setWindowTextW_f;
	typedef BOOL     (WINAPI* endDialog_t)(HWND hDlg, INT_PTR nResult);
	endDialog_t endDialog_f;
	typedef BOOL     (WINAPI* postMessageW_t)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	postMessageW_t postMessageW_f;
	typedef LRESULT  (WINAPI* sendMessageW_t)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	sendMessageW_t sendMessageW_f;
	typedef INT_PTR  (WINAPI* dialogBoxIndirectParamW_t)(HINSTANCE hInstance, LPCDLGTEMPLATE hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
	dialogBoxIndirectParamW_t dialogBoxIndirectParamW_f;
	typedef int      (WINAPI* getClassNameW_t)(HWND hWnd, LPWSTR lpClassName, int nMaxCount);
	getClassNameW_t getClassNameW_f;
	typedef BOOL     (WINAPI* getClientRect_t)(HWND hWnd, LPRECT lpRect);
	getClientRect_t getClientRect_f;
	typedef HMENU    (WINAPI* getMenu_t)(HWND hWnd);
	getMenu_t getMenu_f;
	typedef BOOL     (WINAPI* attachThreadInput_t)(DWORD idAttach, DWORD idAttachTo, BOOL fAttach);
	attachThreadInput_t attachThreadInput_f;
	typedef HWND     (WINAPI* getFocus_t)();
	getFocus_t getFocus_f;
	typedef BOOL     (WINAPI* isWindow_t)(HWND hWnd);
	isWindow_t isWindow_f;
	typedef BOOL     (WINAPI* isWindowVisible_t)(HWND hWnd);
	isWindowVisible_t isWindowVisible_f;
	typedef BOOL     (WINAPI* showWindow_t)(HWND hWnd, int nCmdShow);
	showWindow_t showWindow_f;
	typedef SHORT    (WINAPI* getKeyState_t)(int nVirtKey);
	getKeyState_t getKeyState_f;
	typedef LRESULT  (WINAPI* callNextHookEx_t)(HHOOK hhk, int nCode, WPARAM wParam, LPARAM lParam);
	callNextHookEx_t callNextHookEx_f;
	typedef HHOOK    (WINAPI* setWindowsHookExW_t)(int idHook, HOOKPROC lpfn, HINSTANCE hMod, DWORD dwThreadId);
	setWindowsHookExW_t setWindowsHookExW_f;
	typedef BOOL     (WINAPI* unhookWindowsHookEx_t)(HHOOK hhk);
	unhookWindowsHookEx_t unhookWindowsHookEx_f;
	typedef int      (WINAPI* mapWindowPoints_t)(HWND hWndFrom, HWND hWndTo, LPPOINT lpPoints, UINT cPoints);
	mapWindowPoints_t mapWindowPoints_f;
	typedef UINT     (WINAPI* registerWindowMessageW_t)(LPCWSTR lpString);
	registerWindowMessageW_t registerWindowMessageW_f;
};

extern UserImp* user;
