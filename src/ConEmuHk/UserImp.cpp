
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

#include <windows.h>
#include "UserImp.h"
#include "../common/defines.h"
#include "../common/MAssert.h"

UserImp* user = NULL;

// Вызывать LoadLibrary из DllMain (откуда стартует UserImp::loadExports) нельзя
// Поэтому - только GetModuleHandle, и если он уже загружен в process space - грузим экспорты.
// Однако, чтобы при динамической линковке не обломаться, впоследствии нужно
// подтвердить (накрутить счетчик) использование user32 через LoadLibrary.
BOOL UserImp::loadExports(BOOL abAllowLoadLibrary)
{
	if (!hUser32)
	{
		if (abAllowLoadLibrary)
		{
			hUser32 = LoadLibrary(L"user32.dll");
			bUserLoaded = TRUE;
		}
		else
		{
			hUser32 = GetModuleHandle(L"user32.dll");
		}
		
		if (hUser32)
		{
			loadExportsFrom(hUser32);
		}
	}
	
	if (abAllowLoadLibrary && !bUserLoaded)
	{
		hUser32 = LoadLibrary(L"user32.dll");
	}
	
	return (hUser32 != NULL);
}

BOOL UserImp::loadExportsFrom(HMODULE hModule)
{
	// bUserLoaded - то что LoadLibrary(L"user32.dll") дернули
	// а вот функции - еще могли и не загружать
	if (bUserLoaded && isWindow_f)
		return TRUE;

	BOOL lbRc = FALSE;

	if (hModule && (hUser32 != hModule))
		hUser32 = hModule;

	if (hUser32)
	{
		allowSetForegroundWindow_f = (allowSetForegroundWindow_t)GetProcAddress(hUser32, "AllowSetForegroundWindow");
		setForegroundWindow_f = (setForegroundWindow_t)GetProcAddress(hUser32, "SetForegroundWindow");
		getForegroundWindow_f = (getForegroundWindow_t)GetProcAddress(hUser32, "GetForegroundWindow");
		getWindowThreadProcessId_f = (getWindowThreadProcessId_t)GetProcAddress(hUser32, "GetWindowThreadProcessId");
		setWindowPos_f = (setWindowPos_t)GetProcAddress(hUser32, "SetWindowPos");
		getWindowLongPtrW_f = (getWindowLongPtrW_t)GetProcAddress(hUser32, WIN3264TEST("GetWindowLongW","GetWindowLongPtrW"));
		setWindowLongPtrW_f = (setWindowLongPtrW_t)GetProcAddress(hUser32, WIN3264TEST("SetWindowLongW","SetWindowLongPtrW"));
		getParent_f = (getParent_t)GetProcAddress(hUser32, "GetParent");
		setParent_f = (setParent_t)GetProcAddress(hUser32, "SetParent");
		findWindowEx_f = (findWindowEx_t)GetProcAddress(hUser32, "FindWindowExW");
		getWindowRect_f = (getWindowRect_t)GetProcAddress(hUser32, "GetWindowRect");
		getSystemMetrics_f = (getSystemMetrics_t)GetProcAddress(hUser32, "GetSystemMetrics");
		systemParametersInfoW_f = (systemParametersInfoW_t)GetProcAddress(hUser32, "SystemParametersInfoW");
		setWindowTextW_f = (setWindowTextW_t)GetProcAddress(hUser32, "SetWindowTextW");
		endDialog_f = (endDialog_t)GetProcAddress(hUser32, "EndDialog");
		postMessageW_f = (postMessageW_t)GetProcAddress(hUser32, "PostMessageW");
		sendMessageW_f = (sendMessageW_t)GetProcAddress(hUser32, "SendMessageW");
		dialogBoxIndirectParamW_f = (dialogBoxIndirectParamW_t)GetProcAddress(hUser32, "DialogBoxIndirectParamW");
		getClassNameW_f = (getClassNameW_t)GetProcAddress(hUser32, "GetClassNameW");
		getClientRect_f = (getClientRect_t)GetProcAddress(hUser32, "GetClientRect");
		getMenu_f = (getMenu_t)GetProcAddress(hUser32, "GetMenu");
		attachThreadInput_f = (attachThreadInput_t)GetProcAddress(hUser32, "AttachThreadInput");
		getFocus_f = (getFocus_t)GetProcAddress(hUser32, "GetFocus");
		getWindowRect_f = (getWindowRect_t)GetProcAddress(hUser32, "GetWindowRect");
		isWindow_f = (isWindow_t)GetProcAddress(hUser32, "IsWindow");
		isWindowVisible_f = (isWindowVisible_t)GetProcAddress(hUser32, "IsWindowVisible");
		showWindow_f = (showWindow_t)GetProcAddress(hUser32, "ShowWindow");
		getKeyState_f = (getKeyState_t)GetProcAddress(hUser32, "GetKeyState");
		callNextHookEx_f = (callNextHookEx_t)GetProcAddress(hUser32, "CallNextHookEx");
		setWindowsHookExW_f = (setWindowsHookExW_t)GetProcAddress(hUser32, "SetWindowsHookExW");
		unhookWindowsHookEx_f = (unhookWindowsHookEx_t)GetProcAddress(hUser32, "UnhookWindowsHookEx");
		mapWindowPoints_f = (mapWindowPoints_t)GetProcAddress(hUser32, "MapWindowPoints");
		registerWindowMessageW_f = (registerWindowMessageW_t)GetProcAddress(hUser32, "RegisterWindowMessageW");
		_ASSERTEX(allowSetForegroundWindow_f && setForegroundWindow_f && getForegroundWindow_f && getWindowThreadProcessId_f);
		_ASSERTEX(setWindowPos_f && getWindowLongPtrW_f && setWindowLongPtrW_f && getParent_f && setParent_f && findWindowEx_f && getWindowRect_f);
		_ASSERTEX(getSystemMetrics_f && systemParametersInfoW_f && setWindowTextW_f && endDialog_f && postMessageW_f && sendMessageW_f);
		_ASSERTEX(dialogBoxIndirectParamW_f && getClassNameW_f && getClientRect_f && getMenu_f && attachThreadInput_f);
		_ASSERTEX(getFocus_f && getWindowRect_f && isWindow_f && isWindowVisible_f && showWindow_f && getKeyState_f);
		_ASSERTEX(callNextHookEx_f && setWindowsHookExW_f && unhookWindowsHookEx_f && mapWindowPoints_f && registerWindowMessageW_f);

		bUserLoaded = lbRc = (isWindow_f!=NULL);
	}

	return lbRc;
}

void UserImp::setAllowLoadLibrary()
{
	bAllowLoadLibrary = TRUE;
}

bool UserImp::isExportsLoaded()
{
	return (bUserLoaded!=0);
}

bool UserImp::isUser32(HMODULE hModule)
{
	return ((hModule != NULL) && (this != NULL) && (hModule == hUser32));
}

BOOL UserImp::allowSetForegroundWindow(DWORD dwProcessId)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return FALSE;
	}
	
	BOOL lbRc = FALSE;
	if (allowSetForegroundWindow_f)
	{
		lbRc = allowSetForegroundWindow_f(dwProcessId);
	}
	else
	{
		_ASSERTEX(allowSetForegroundWindow_f!=NULL);
	}
	return lbRc;
}

BOOL UserImp::setForegroundWindow(HWND hWnd)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return FALSE;
	}
	
	BOOL lbRc = FALSE;
	if (setForegroundWindow_f)
	{
		lbRc = setForegroundWindow_f(hWnd);
	}
	else
	{
		_ASSERTEX(setForegroundWindow_f!=NULL);
	}
	return lbRc;
}

HWND UserImp::getForegroundWindow()
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return NULL;
	}
	
	HWND hRc = NULL;
	if (getForegroundWindow_f)
	{
		hRc = getForegroundWindow_f();
	}
	else
	{
		_ASSERTEX(getForegroundWindow_f!=NULL);
	}
	return hRc;
}

DWORD UserImp::getWindowThreadProcessId(HWND hWnd, LPDWORD lpdwProcessId)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return 0;
	}
	
	DWORD lRc = 0;
	if (getWindowThreadProcessId_f)
	{
		lRc = getWindowThreadProcessId_f(hWnd, lpdwProcessId);
	}
	else
	{
		_ASSERTEX(getWindowThreadProcessId_f!=NULL);
	}
	return lRc;
}

BOOL UserImp::setWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return FALSE;
	}
	
	BOOL lbRc = FALSE;
	if (setWindowPos_f)
	{
		lbRc = setWindowPos_f(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
	}
	else
	{
		_ASSERTEX(setWindowPos_f!=NULL);
	}
	return lbRc;
}

LONG_PTR UserImp::getWindowLongPtrW(HWND hWnd, int nIndex)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return 0;
	}
	
	LONG_PTR lRc = 0;
	if (getWindowLongPtrW_f)
	{
		lRc = getWindowLongPtrW_f(hWnd, nIndex);
	}
	else
	{
		_ASSERTEX(getWindowLongPtrW_f!=NULL);
	}
	return lRc;
}

LONG_PTR UserImp::setWindowLongPtrW(HWND hWnd, int nIndex, LONG_PTR dwNewLong)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return 0;
	}
	
	LONG_PTR lRc = 0;
	if (setWindowLongPtrW_f)
	{
		lRc = setWindowLongPtrW_f(hWnd, nIndex, dwNewLong);
	}
	else
	{
		_ASSERTEX(setWindowLongPtrW_f!=NULL);
	}
	return lRc;
}

HWND UserImp::getParent(HWND hWnd)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return NULL;
	}
	
	HWND hRc = NULL;
	if (getParent_f)
	{
		hRc = getParent_f(hWnd);
	}
	else
	{
		_ASSERTEX(getParent_f!=NULL);
	}
	return hRc;
}

HWND UserImp::setParent(HWND hWndChild, HWND hWndNewParent)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return NULL;
	}
	
	HWND hRc = NULL;
	if (setParent_f)
	{
		hRc = setParent_f(hWndChild, hWndNewParent);
	}
	else
	{
		_ASSERTEX(setParent_f!=NULL);
	}
	return hRc;
}

HWND UserImp::findWindowEx(HWND hwndParent, HWND hwndChildAfter, LPCTSTR lpszClass, LPCTSTR lpszWindow)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return NULL;
	}
	
	HWND hRc = NULL;
	if (findWindowEx_f)
	{
		hRc = findWindowEx_f(hwndParent, hwndChildAfter, lpszClass, lpszWindow);
	}
	else
	{
		_ASSERTEX(findWindowEx_f!=NULL);
	}
	return hRc;
}

BOOL UserImp::getWindowRect(HWND hWnd, LPRECT lpRect)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return FALSE;
	}
	
	BOOL bRc = FALSE;
	if (getWindowRect_f)
	{
		bRc = getWindowRect_f(hWnd, lpRect);
	}
	else
	{
		_ASSERTEX(getWindowRect_f!=NULL);
	}
	return bRc;
}

BOOL UserImp::getSystemMetrics(int nIndex)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return FALSE;
	}
	
	int iRc = 0;
	if (getSystemMetrics_f)
	{
		iRc = getSystemMetrics_f(nIndex);
	}
	else
	{
		_ASSERTEX(getSystemMetrics_f!=NULL);
	}
	return iRc;
}

BOOL UserImp::systemParametersInfoW(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return FALSE;
	}
	
	BOOL bRc = FALSE;
	if (systemParametersInfoW_f)
	{
		bRc = systemParametersInfoW_f(uiAction, uiParam, pvParam, fWinIni);
	}
	else
	{
		_ASSERTEX(systemParametersInfoW_f!=NULL);
	}
	return bRc;
}

BOOL UserImp::setWindowTextW(HWND hWnd, LPCWSTR lpString)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return FALSE;
	}
	
	BOOL bRc = FALSE;
	if (setWindowTextW_f)
	{
		bRc = setWindowTextW_f(hWnd, lpString);
	}
	else
	{
		_ASSERTEX(setWindowTextW_f!=NULL);
	}
	return bRc;
}

BOOL UserImp::endDialog(HWND hDlg, INT_PTR nResult)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return FALSE;
	}
	
	BOOL bRc = FALSE;
	if (endDialog_f)
	{
		bRc = endDialog_f(hDlg, nResult);
	}
	else
	{
		_ASSERTEX(endDialog_f!=NULL);
	}
	return bRc;
}

BOOL UserImp::postMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return FALSE;
	}
	
	BOOL lbRc = FALSE;
	if (postMessageW_f)
	{
		lbRc = postMessageW_f(hWnd, Msg, wParam, lParam);
	}
	else
	{
		_ASSERTEX(postMessageW_f!=NULL);
	}
	return lbRc;
}

LRESULT UserImp::sendMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return 0;
	}
	
	LRESULT lRc = 0;
	if (sendMessageW_f)
	{
		lRc = sendMessageW_f(hWnd, Msg, wParam, lParam);
	}
	else
	{
		_ASSERTEX(sendMessageW_f!=NULL);
	}
	return lRc;
}

INT_PTR UserImp::dialogBoxIndirectParamW(HINSTANCE hInstance, LPCDLGTEMPLATE hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return 0;
	}
	
	INT_PTR nRc = 0;
	if (dialogBoxIndirectParamW_f)
	{
		nRc = dialogBoxIndirectParamW_f(hInstance, hDialogTemplate, hWndParent, lpDialogFunc, dwInitParam);
	}
	else
	{
		_ASSERTEX(dialogBoxIndirectParamW_f!=NULL);
	}
	return nRc;
}

int UserImp::getClassNameW(HWND hWnd, LPTSTR lpClassName, int nMaxCount)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return 0;
	}
	
	int lRc = 0;
	if (getClassNameW_f)
	{
		lRc = getClassNameW_f(hWnd, lpClassName, nMaxCount);
	}
	else
	{
		_ASSERTEX(getClassNameW_f!=NULL);
	}
	return lRc;
}

BOOL UserImp::getClientRect(HWND hWnd, LPRECT lpRect)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return 0;
	}
	
	BOOL lbRc = FALSE;
	if (getClientRect_f)
	{
		lbRc = getClientRect_f(hWnd, lpRect);
	}
	else
	{
		_ASSERTEX(getClientRect_f!=NULL);
	}
	return lbRc;
}

HMENU UserImp::getMenu(HWND hWnd)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return NULL;
	}
	
	HMENU hRc = NULL;
	if (getMenu_f)
	{
		hRc = getMenu_f(hWnd);
	}
	else
	{
		_ASSERTEX(getMenu_f!=NULL);
	}
	return hRc;
}

BOOL UserImp::attachThreadInput(DWORD idAttach, DWORD idAttachTo, BOOL fAttach)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return FALSE;
	}
	
	BOOL lbRc = FALSE;
	if (attachThreadInput_f)
	{
		lbRc = attachThreadInput_f(idAttach, idAttachTo, fAttach);
	}
	else
	{
		_ASSERTEX(attachThreadInput_f!=NULL);
	}
	return lbRc;
}

HWND UserImp::getFocus()
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return NULL;
	}
	
	HWND hRc = NULL;
	if (getFocus_f)
	{
		hRc = getFocus_f();
	}
	else
	{
		_ASSERTEX(getFocus_f!=NULL);
	}
	return hRc;
}

BOOL UserImp::isWindow(HWND hWnd)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		// Поскольку функция может использоваться при старте (проверка валидности дескриптора ConEmu)
		// нужно вернуть TRUE, если User32.dll еще не загружен в process space
		return TRUE;
	}
	
	BOOL lbRc = FALSE;
	if (isWindow_f)
	{
		lbRc = isWindow_f(hWnd);
	}
	else
	{
		_ASSERTEX(isWindow_f!=NULL);
	}
	return lbRc;
}

BOOL UserImp::isWindowVisible(HWND hWnd)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return FALSE;
	}
	
	BOOL lbRc = FALSE;
	if (isWindowVisible_f)
	{
		lbRc = isWindowVisible_f(hWnd);
	}
	else
	{
		_ASSERTEX(isWindowVisible_f!=NULL);
	}
	return lbRc;
}

BOOL UserImp::showWindow(HWND hWnd, int nCmdShow)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return FALSE;
	}
	
	BOOL lbRc = FALSE;
	if (showWindow_f)
	{
		lbRc = showWindow_f(hWnd, nCmdShow);
	}
	else
	{
		_ASSERTEX(showWindow_f!=NULL);
	}
	return lbRc;
}

SHORT UserImp::getKeyState(int nVirtKey)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return 0;
	}
	
	SHORT lRc = 0;
	if (getKeyState_f)
	{
		lRc = getKeyState_f(nVirtKey);
	}
	else
	{
		_ASSERTEX(getKeyState_f!=NULL);
	}
	return lRc;
}

LRESULT UserImp::callNextHookEx(HHOOK hhk, int nCode, WPARAM wParam, LPARAM lParam)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return 0;
	}

	LRESULT lRc = FALSE;
	if (callNextHookEx_f)
	{
		lRc = callNextHookEx_f(hhk, nCode, wParam, lParam);
	}
	else
	{
		_ASSERTEX(callNextHookEx_f!=NULL);
	}
	return lRc;
}

HHOOK UserImp::setWindowsHookExW(int idHook, HOOKPROC lpfn, HINSTANCE hMod, DWORD dwThreadId)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return NULL;
	}
	
	HHOOK hRc = NULL;
	if (setWindowsHookExW_f)
	{
		hRc = setWindowsHookExW_f(idHook, lpfn, hMod, dwThreadId);
	}
	else
	{
		_ASSERTEX(setWindowsHookExW_f!=NULL);
	}
	return hRc;
}

BOOL UserImp::unhookWindowsHookEx(HHOOK hhk)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return FALSE;
	}

	BOOL lbRc = FALSE;
	if (unhookWindowsHookEx_f)
	{
		lbRc = unhookWindowsHookEx_f(hhk);
	}
	else
	{
		_ASSERTEX(unhookWindowsHookEx_f!=NULL);
	}
	return lbRc;
}

BOOL UserImp::mapWindowPoints(HWND hWndFrom, HWND hWndTo, LPPOINT lpPoints, UINT cPoints)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return FALSE;
	}

	BOOL lbRc = FALSE;
	if (mapWindowPoints_f)
	{
		lbRc = mapWindowPoints_f(hWndFrom, hWndTo, lpPoints, cPoints);
	}
	else
	{
		_ASSERTEX(mapWindowPoints_f!=NULL);
	}
	return lbRc;
}

UINT UserImp::registerWindowMessageW(LPCWSTR lpString)
{
	if (!bUserLoaded && !loadExports(bAllowLoadLibrary))
	{
		_ASSERTEX(hUser32!=NULL);
		return FALSE;
	}

	UINT iMsg = 0;
	if (registerWindowMessageW_f)
	{
		iMsg = registerWindowMessageW_f(lpString);
	}
	else
	{
		_ASSERTEX(registerWindowMessageW_f!=NULL);
	}
	return iMsg;
}
