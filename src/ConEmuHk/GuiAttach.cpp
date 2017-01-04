
/*
Copyright (c) 2009-2017 Maximus5
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

#define HIDE_USE_EXCEPTION_INFO

#ifdef _DEBUG
#define DebugString(x) OutputDebugString(x)
#else
#define DebugString(x) //OutputDebugString(x)
#endif

#include <windows.h>
#include "GuiAttach.h"
#include "MainThread.h"
#include "../common/Common.h"
#include "../common/CmdLine.h"
#include "../common/ConEmuCheck.h"
#include "../common/WObjects.h"

extern HMODULE ghOurModule;
extern HWND    ghConEmuWnd;     // Root! ConEmu window
extern HWND    ghConEmuWndDC;   // ConEmu DC window
extern HWND    ghConEmuWndBack; // ConEmu Back window - holder for GUI client
extern DWORD   gnServerPID;

// Этот TID может отличаться от основного потока.
// Например, VLC создает окна не в главном, а в фоновом потоке.
DWORD  gnAttachGuiClientThreadId = 0;

RECT    grcConEmuClient = {}; // Для аттача гуевых окон
BOOL    gbAttachGuiClient = FALSE; // Для аттача гуевых окон
BOOL 	gbGuiClientAttached = FALSE; // Для аттача гуевых окон (успешно подключились)
BOOL 	gbGuiClientExternMode = FALSE; // Если нужно показать Gui-приложение вне вкладки ConEmu
GuiCui  gnAttachPortableGuiCui = e_Alone; // XxxPortable.exe запускает xxx.exe который уже и нужно цеплять (IMAGE_SUBSYSTEM_WINDOWS_[G|C]UI)
struct GuiStylesAndShifts gGuiClientStyles = {}; // Запомнить сдвиги окна внутри ConEmu
HWND 	ghAttachGuiClient = NULL;
HWND    ghAttachGuiFocused = NULL;
DWORD 	gnAttachGuiClientFlags = 0;
DWORD 	gnAttachGuiClientStyle = 0, gnAttachGuiClientStyleEx = 0;
BOOL  	gbAttachGuiClientStyleOk = FALSE;
BOOL 	gbForceShowGuiClient = FALSE; // --
//HMENU ghAttachGuiClientMenu = NULL;
RECT 	grcAttachGuiClientOrig = {0,0,0,0};

#ifdef _DEBUG
static HWND hMonitorWndPos = NULL;
HHOOK 	ghGuiClientRetHook = NULL;
HHOOK 	ghGuiClientCallHook = NULL;
HHOOK 	ghGuiClientMsgHook = NULL;
LRESULT CALLBACK GuiClientMsgHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		MSG* p = (MSG*)lParam;
		switch (p->message)
		{
		case WM_CREATE:
			break;
		}
	}
	return CallNextHookEx(ghGuiClientMsgHook, nCode, wParam, lParam);
}
LRESULT CALLBACK GuiClientCallHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		CWPSTRUCT* p = (CWPSTRUCT*)lParam;
		CREATESTRUCT* pc = NULL;
		wchar_t szDbg[200]; szDbg[0] = 0;
		wchar_t szClass[127]; szClass[0] = 0;
		switch (p->message)
		{
			case WM_DESTROY:
			{
				GetClassName(p->hwnd, szClass, countof(szClass));
				msprintf(szDbg, countof(szDbg), L"WM_DESTROY on 0x%08X (%s)\n", LODWORD(p->hwnd), szClass);
				break;
			}
			case WM_CREATE:
			{
				pc = (CREATESTRUCT*)p->lParam;
				break;
			}
			case WM_SIZE:
			case WM_MOVE:
			case WM_WINDOWPOSCHANGED:
			case WM_WINDOWPOSCHANGING:
			{
				WINDOWPOS *wp = (WINDOWPOS*)p->lParam;
				WORD x = LOWORD(p->lParam);
				WORD y = HIWORD(p->lParam);
				if (p->hwnd == ghAttachGuiClient || p->hwnd == hMonitorWndPos)
				{
					int nDbg = 0;
					if (p->message == WM_WINDOWPOSCHANGING || p->message == WM_WINDOWPOSCHANGED)
					{
						if ((wp->x > 0 || wp->y > 0) && !isPressed(VK_LBUTTON))
						{
							if (GetParent(p->hwnd) == ghConEmuWndBack)
							{
								//_ASSERTEX(!(wp->x > 0 || wp->y > 0));
								break;
							}
						}
					}
				}
				break;
			}
		}

		if (*szDbg)
		{
			DebugString(szDbg);
		}
	}

	return CallNextHookEx(ghGuiClientCallHook, nCode, wParam, lParam);
}
LRESULT CALLBACK GuiClientRetHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		CWPRETSTRUCT* p = (CWPRETSTRUCT*)lParam;
		CREATESTRUCT* pc = NULL;
		wchar_t szDbg[200]; szDbg[0] = 0;
		wchar_t szClass[127]; szClass[0] = 0;
		switch (p->message)
		{
			case WM_DESTROY:
			{
				GetClassName(p->hwnd, szClass, countof(szClass));
				msprintf(szDbg, countof(szDbg), L"WM_DESTROY on 0x%08X (%s)\n", LODWORD(p->hwnd), szClass);
				break;
			}
			case WM_CREATE:
			{
				pc = (CREATESTRUCT*)p->lParam;
				if (p->lResult == -1)
				{
					wcscpy_c(szDbg, L"WM_CREATE --FAILED--\n");
				}
				break;
			}
			case WM_SIZE:
			case WM_MOVE:
			case WM_WINDOWPOSCHANGED:
			case WM_WINDOWPOSCHANGING:
			{
				WINDOWPOS *wp = (WINDOWPOS*)p->lParam;
				WORD x = LOWORD(p->lParam);
				WORD y = HIWORD(p->lParam);
				if (p->hwnd == ghAttachGuiClient || p->hwnd == hMonitorWndPos)
				{
					int nDbg = 0;
					if (p->message == WM_WINDOWPOSCHANGING || p->message == WM_WINDOWPOSCHANGED)
					{
						if ((wp->x > 0 || wp->y > 0) && !isPressed(VK_LBUTTON))
						{
							if (GetParent(p->hwnd) == ghConEmuWndBack)
							{
								//_ASSERTEX(!(wp->x > 0 || wp->y > 0));
								break;
							}
						}
					}
				}
				break;
			}
		}

		if (*szDbg)
		{
			DebugString(szDbg);
		}
	}

	return CallNextHookEx(ghGuiClientRetHook, nCode, wParam, lParam);
}
#endif


bool CheckCanCreateWindow(LPCSTR lpClassNameA, LPCWSTR lpClassNameW, DWORD& dwStyle, DWORD& dwExStyle, HWND& hWndParent, BOOL& bAttachGui, BOOL& bStyleHidden)
{
	bAttachGui = FALSE;

#ifdef _DEBUG
	// "!dwStyle" добавил для shell32.dll!CExecuteApplication::_CreateHiddenDDEWindow()
	_ASSERTE(hWndParent==NULL || ghConEmuWnd == NULL || hWndParent!=ghConEmuWnd || !dwStyle);
	STARTUPINFO si = {sizeof(si)};
	GetStartupInfo(&si);

	bool lbAfxFrameOrView90 = false;
	if (lpClassNameA && (((DWORD_PTR)lpClassNameA) & ~0xFFFF))
	{
		lbAfxFrameOrView90 = lstrcmpA(lpClassNameA, "AfxFrameOrView90") == 0 || lstrcmpiA(lpClassNameA, "Xshell4:MainWnd") == 0;
	}
	else if (lpClassNameW && (((DWORD_PTR)lpClassNameW) & ~0xFFFF))
	{
		lbAfxFrameOrView90 = lstrcmpW(lpClassNameW, L"AfxFrameOrView90") == 0 || lstrcmpiW(lpClassNameW, L"Xshell4:MainWnd") == 0;
	}
	if (lbAfxFrameOrView90)
	{
		lbAfxFrameOrView90 = true;
	}
#endif

	if (gbAttachGuiClient && ghConEmuWndBack)
	{
		#ifdef _DEBUG
		WNDCLASS wc = {}; BOOL lbClass = FALSE;
		if ((lpClassNameW && ((DWORD_PTR)lpClassNameW) <= 0xFFFF))
		{
			lbClass = GetClassInfo((HINSTANCE)GetModuleHandle(NULL), lpClassNameW, &wc);
		}
		#endif

		DWORD nTID = GetCurrentThreadId();
		if ((nTID != gnHookMainThreadId) && (gnAttachGuiClientThreadId && nTID != gnAttachGuiClientThreadId))
		{
			_ASSERTEX(nTID==gnHookMainThreadId || !gnAttachGuiClientThreadId || (ghAttachGuiClient && IsWindow(ghAttachGuiClient)));
		}
		else
		{
			const DWORD dwNormalSized = (WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX); // Some applications can 'disable' Maximize button but they are still 'resizeable'
			const DWORD dwDlgSized = (WS_POPUP|WS_THICKFRAME);
			const DWORD dwSizedMask = (WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_POPUP|DS_MODALFRAME|WS_CHILDWINDOW);
			const DWORD dwNoCaptionSized = (WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX);
			// Lets check
			bool lbCanAttach =
							// Обычное окно с заголовком (0x00CF0000 or 0x00CE0000)
							((dwStyle & dwNormalSized) == dwNormalSized)
							// Диалог с ресайзом рамки (0x80040000)
							|| ((dwStyle & dwDlgSized) == dwDlgSized)
							// Обычное окно без заголовка (0xC0070080 : 0x00070000)
							|| ((dwStyle & dwSizedMask) == dwNoCaptionSized)
							;
			if (dwStyle & (DS_MODALFRAME|WS_CHILDWINDOW))
				lbCanAttach = false;
			else if ((dwStyle & WS_POPUP) && !(dwStyle & WS_THICKFRAME))
				lbCanAttach = false;
			else if (dwExStyle & (WS_EX_TOOLWINDOW|WS_EX_TOPMOST|WS_EX_DLGMODALFRAME|WS_EX_MDICHILD))
				lbCanAttach = false;

			// Disable attach of some window classes
			if (lbCanAttach && lpClassNameW && (((DWORD_PTR)lpClassNameW) & ~0xFFFF))
			{
				if (lstrcmpW(lpClassNameW, L"MozillaTempWindowClass") == 0)
					lbCanAttach = false;
			}

			if (lbCanAttach)
			{
				// Родительское окно - ConEmu DC
				// -- hWndParent = ghConEmuWndBack; // Надо ли его ставить сразу, если не включаем WS_CHILD?

				// WS_CHILDWINDOW перед созданием выставлять нельзя, т.к. например у WordPad.exe сносит крышу:
				// все его окна создаются нормально, но он показывает ошибку "Не удалось создать новый документ"
				//// Уберем рамку, меню и заголовок - оставим
				//dwStyle = (dwStyle | WS_CHILDWINDOW|WS_TABSTOP) & ~(WS_THICKFRAME/*|WS_CAPTION|WS_MINIMIZEBOX|WS_MAXIMIZEBOX*/);
				bStyleHidden = (dwStyle & WS_VISIBLE) == WS_VISIBLE;
				dwStyle &= ~WS_VISIBLE; // А вот видимость - точно сбросим
				////dwExStyle = dwExStyle & ~WS_EX_WINDOWEDGE;

				bAttachGui = TRUE;
				//gbAttachGuiClient = FALSE; // Только одно окно приложения -- сбросим после реального создания окна
				gbGuiClientAttached = TRUE; // Сразу взведем флажок режима

				#ifdef _DEBUG
				if (!ghGuiClientRetHook)
					ghGuiClientRetHook = SetWindowsHookExW(WH_CALLWNDPROCRET, GuiClientRetHook, NULL, GetCurrentThreadId());
				//if (!ghGuiClientCallHook)
				//	ghGuiClientCallHook = SetWindowsHookExW(WH_CALLWNDPROC, GuiClientCallHook, NULL, GetCurrentThreadId());
				//if (!ghGuiClientMsgHook)
				//	ghGuiClientMsgHook = SetWindowsHookExW(WH_GETMESSAGE, GuiClientMsgHook, NULL, GetCurrentThreadId());
				#endif

				//gnAttachGuiClientThreadId = nTID; -- перенес к "ghAttachGuiClient = hWindow;"

				RECT rcGui = AttachGuiClientPos(dwStyle, dwExStyle);
				if (hWndParent != ghConEmuWndBack)
				{
					MapWindowPoints(ghConEmuWndBack, hWndParent, (LPPOINT)&rcGui, 2);
				}
				grcConEmuClient = rcGui;
			}
			return true;
		}
	}

	if (gbGuiClientAttached /*ghAttachGuiClient*/)
	{
		return true; // В GUI приложениях - разрешено все
	}

#ifndef _DEBUG
	return true;
#else
	if (gnHookMainThreadId && gnHookMainThreadId != GetCurrentThreadId())
		return true; // Разрешено, отдается на откуп консольной программе/плагинам

	if ((dwStyle & (WS_POPUP|DS_MODALFRAME)) == (WS_POPUP|DS_MODALFRAME))
	{
		// Это скорее всего обычный диалог, разрешим, но пока для отладчика - assert
		_ASSERTE((dwStyle & WS_POPUP) == 0);
		return true;
	}

	if ((lpClassNameA && ((DWORD_PTR)lpClassNameA) <= 0xFFFF)
		|| (lpClassNameW && ((DWORD_PTR)lpClassNameW) <= 0xFFFF))
	{
		// Что-то системное
		return true;
	}

	// Окно на любой чих создается. dwStyle == 0x88000000.
	if ((lpClassNameW && lstrcmpW(lpClassNameW, L"CicMarshalWndClass") == 0)
		|| (lpClassNameA && lstrcmpA(lpClassNameA, "CicMarshalWndClass") == 0)
		)
	{
		return true;
	}

	// WiX
	if ((lpClassNameW && lstrcmpW(lpClassNameW, L"MsiHiddenWindow") == 0)
		)
	{
		return true;
	}

	#ifdef _DEBUG
	// В консоли нет обработчика сообщений, поэтому создание окон в главной
	// нити приводит к "зависанию" приложения - например, любые программы,
	// использующие DDE могут виснуть.
	wchar_t szModule[MAX_PATH] = {}; GetModuleFileName(ghOurModule, szModule, countof(szModule));
	//const wchar_t* pszSlash = PointToName(szModule);
	//if (lstrcmpi(pszSlash, L"far.exe")==0 || lstrcmpi(szModule, L"far64.exe")==0)
	if (IsFarExe(szModule))
	{
		_ASSERTE(dwStyle == 0 && FALSE);
	}
	//SetLastError(ERROR_THREAD_MODE_NOT_BACKGROUND);
	//return false;
	#endif

	// Разрешить? По настройке?
	return true;
#endif
}

void CheckOrigGuiClientRect()
{
	if (grcAttachGuiClientOrig.left==grcAttachGuiClientOrig.right && grcAttachGuiClientOrig.bottom==grcAttachGuiClientOrig.top)
	{
		_ASSERTEX(ghAttachGuiClient!=NULL);
		GetWindowRect(ghAttachGuiClient, &grcAttachGuiClientOrig);
	}
}

void ReplaceGuiAppWindow(BOOL abStyleHidden)
{
	if (!ghAttachGuiClient)
	{
		_ASSERTEX(ghAttachGuiClient!=NULL);
		return;
	}

	DWORD_PTR dwStyle = GetWindowLongPtr(ghAttachGuiClient, GWL_STYLE);
	DWORD_PTR dwStyleEx = GetWindowLongPtr(ghAttachGuiClient, GWL_EXSTYLE);

	if (!gbAttachGuiClientStyleOk)
	{
		gbAttachGuiClientStyleOk = TRUE;
		gnAttachGuiClientStyle = (DWORD)dwStyle;
		gnAttachGuiClientStyleEx = (DWORD)dwStyleEx;
	}

	// Позвать GetWindowRect(ghAttachGuiClient, &grcAttachGuiClientOrig); если надо
	CheckOrigGuiClientRect();

	if (!gbGuiClientExternMode)
	{
		// DotNet: если не включить WS_CHILD - не работают toolStrip & menuStrip
		// Native: если включить WS_CHILD - исчезает оконное меню
		DWORD_PTR dwNewStyle = dwStyle & ~(WS_MAXIMIZEBOX|WS_MINIMIZEBOX);
		if (gnAttachGuiClientFlags & agaf_WS_CHILD)
			dwNewStyle = (dwNewStyle|WS_CHILD/*|DS_CONTROL*/) & ~(WS_POPUP);

		if (dwStyle != dwNewStyle)
			SetWindowLongPtr(ghAttachGuiClient, GWL_STYLE, dwNewStyle);

		/*

		DWORD_PTR dwNewStyleEx = (dwStyleEx|WS_EX_CONTROLPARENT);
		if (dwStyleEx != dwNewStyleEx)
			SetWindowLongPtr(ghAttachGuiClient, GWL_EXSTYLE, dwNewStyleEx);
		*/

		HWND hCurParent = GetParent(ghAttachGuiClient);
		_ASSERTEX(ghConEmuWndBack && IsWindow(ghConEmuWndBack));
		if (hCurParent != ghConEmuWndBack)
		{
			SetParent(ghAttachGuiClient, ghConEmuWndBack);
		}

		RECT rcGui = AttachGuiClientPos();

		if (SetWindowPos(ghAttachGuiClient, HWND_TOP, rcGui.left,rcGui.top, rcGui.right-rcGui.left, rcGui.bottom-rcGui.top,
			SWP_DRAWFRAME | SWP_NOCOPYBITS | /*SWP_FRAMECHANGED |*/ (abStyleHidden ? SWP_SHOWWINDOW : 0)))
		{
			if (abStyleHidden && IsWindowVisible(ghAttachGuiClient))
				abStyleHidden = FALSE;
		}

		// !!! OnSetForegroundWindow не подходит - он дергает Cmd.
		SetForegroundWindow(ghConEmuWnd);

		PostMessageW(ghAttachGuiClient, WM_NCPAINT, 0, 0);
	}
}

UINT gnAttachMsgId = 0;
HHOOK ghAttachMsgHook = NULL;
struct AttachMsgArg
{
	UINT MsgId;
	UINT Result;
	HWND hConEmu;
	HWND hProcessed;
};

LRESULT CALLBACK AttachGuiWindowCallback(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		CWPSTRUCT* p = (CWPSTRUCT*)lParam;
		if (p->message == gnAttachMsgId)
		{
			_ASSERTEX(wParam==1 && p->wParam==gnAttachMsgId);
			AttachMsgArg* args = (AttachMsgArg*)p->lParam;
			_ASSERTEX(args->MsgId==gnAttachMsgId && args->hConEmu==ghConEmuWnd && args->hProcessed==p->hwnd);

			HWND hOurWindow = p->hwnd;
			HMENU hMenu = GetMenu(hOurWindow);
			wchar_t szClassName[255]; GetClassName(hOurWindow, szClassName, countof(szClassName));
			DWORD nCurStyle = (DWORD)GetWindowLongPtr(hOurWindow, GWL_STYLE);
			DWORD nCurStyleEx = (DWORD)GetWindowLongPtr(hOurWindow, GWL_EXSTYLE);
			//BOOL bWasVisible = IsWindowVisible(hOurWindow);
			_ASSERTEX(IsWindowVisible(hOurWindow));
			OnGuiWindowAttached(hOurWindow, hMenu, NULL, szClassName, nCurStyle, nCurStyleEx, FALSE, SW_SHOW);
			args->Result = gnAttachMsgId;
			//if (!bWasVisible) // Надо?
			//	ShowWindow(hOurWindow, SW_SHOW);
		}
	}

	return CallNextHookEx(ghAttachMsgHook, nCode, wParam, lParam);
}

void AttachGuiWindow(HWND hOurWindow)
{
	_ASSERTEX(gbAttachGuiClient); // Уже должен был быть установлен?
	gnAttachMsgId = RegisterWindowMessageW(L"ConEmu:Attach2Gui");
	if (gnAttachMsgId)
	{
		DWORD nWndTID = GetWindowThreadProcessId(hOurWindow, NULL);
		ghAttachMsgHook = SetWindowsHookExW(WH_CALLWNDPROC, AttachGuiWindowCallback, NULL, nWndTID);

		// Поскольку аттач хорошо бы выполнять в той нити, в которой крутится окно - то через хук
		AttachMsgArg args = {gnAttachMsgId, 0, ghConEmuWnd, hOurWindow};
		LRESULT lRc = SendMessageW(hOurWindow, gnAttachMsgId, gnAttachMsgId, (LPARAM)&args);
		_ASSERTEX(args.Result == gnAttachMsgId);
		UNREFERENCED_PARAMETER(lRc);

		UnhookWindowsHookEx(ghAttachMsgHook);
		ghAttachMsgHook = NULL;
	}
}

bool IsDotNetWindow(HWND hWindow)
{
	// С приложенями .Net - приходится работать как с WS_CHILD,
	// иначе в них "не нажимаются" тулбары и меню

	//Issue 624: mscoree может быть загружен и НЕ в .Net приложение
	//if (GetModuleHandle(L"mscoree.dll") != NULL) -- некорректно

	wchar_t szClass[255];
	const wchar_t szWindowsForms[] = L"WindowsForms";
	int nNeedLen = lstrlen(szWindowsForms);

	// WindowsForms10.Window.8.app.0.378734a
	int nLen = GetClassName(hWindow, szClass, countof(szClass));
	if (nLen > nNeedLen)
	{
		szClass[nNeedLen] = 0;
		if (lstrcmpi(szClass, szWindowsForms) == 0)
		{
			// Ну а теперь проверим, что это ".Net"
			HMODULE hCore = GetModuleHandle(L"mscoree.dll");
			if (hCore != NULL)
			{
				return true;
			}
		}
	}

	return false;
}

bool IsQtWindow(LPCSTR asClassA, LPCWSTR asClassW)
{
	if (asClassW && (((DWORD_PTR)asClassW) > 0xFFFF))
	{
		if (lstrcmp(asClassW, L"QWidget") == 0)
			return true;
	}
	else if (asClassA && (((DWORD_PTR)asClassA) > 0xFFFF))
	{
		if (lstrcmpA(asClassA, "QWidget") == 0)
			return true;
	}
	return false;
}

// Если (anFromShowWindow != -1), значит функу зовут из ShowWindow
void OnGuiWindowAttached(HWND hWindow, HMENU hMenu, LPCSTR asClassA, LPCWSTR asClassW, DWORD anStyle, DWORD anStyleEx, BOOL abStyleHidden, int anFromShowWindow/*=-1*/)
{
	DWORD nCurStyle = (DWORD)GetWindowLongPtr(hWindow, GWL_STYLE);
	DWORD nCurStyleEx = (DWORD)GetWindowLongPtr(hWindow, GWL_EXSTYLE);

	AllowSetForegroundWindow(ASFW_ANY);

	// VLC создает несколько "подходящих" окон, но ShowWindow зовет
	// только для одного из них. Поэтому фактический аттач делаем
	// только в том случае, если окно "видимое"
	if ((!(nCurStyle & WS_VISIBLE)) && (anFromShowWindow <= SW_HIDE))
	{
		// Значит потом, из ShowWindow
		return;
	}

	ghAttachGuiClient = hWindow;
	gnAttachGuiClientThreadId = GetWindowThreadProcessId(hWindow, NULL);
	gbForceShowGuiClient = TRUE;
	gbAttachGuiClient = FALSE; // Только одно окно приложения. Пока?

#if 0
	// Для WS_CHILDWINDOW меню нельзя указать при создании окна
	if (!hMenu && !ghAttachGuiClientMenu && (asClassA || asClassW))
	{
		BOOL lbRcClass;
		WNDCLASSEXA wca = {sizeof(WNDCLASSEXA)};
		WNDCLASSEXW wcw = {sizeof(WNDCLASSEXW)};
		if (asClassA)
		{
			lbRcClass = GetClassInfoExA(GetModuleHandle(NULL), asClassA, &wca);
			if (lbRcClass)
				ghAttachGuiClientMenu = LoadMenuA(wca.hInstance, wca.lpszMenuName);
		}
		else
		{
			lbRcClass = GetClassInfoExW(GetModuleHandle(NULL), asClassW, &wcw);
			if (lbRcClass)
				ghAttachGuiClientMenu = LoadMenuW(wca.hInstance, wcw.lpszMenuName);
		}
		hMenu = ghAttachGuiClientMenu;
	}
	if (hMenu)
	{
		// Для WS_CHILDWINDOW - не работает
		SetMenu(hWindow, hMenu);
		HMENU hSys = GetSystemMenu(hWindow, FALSE);
		TODO("Это в принципе прокатывает, но нужно транслировать WM_SYSCOMMAND -> WM_COMMAND, соответственно, перехватывать WndProc, или хук ставить");
		if (hSys)
		{
			TODO("Хотя, хорошо бы не все в Popup засоывать, а извлечь ChildPopups из hMenu");
			InsertMenu(hSys, 0, MF_BYPOSITION|MF_POPUP, (UINT_PTR)hMenu, L"Window menu");
			InsertMenu(hSys, 1, MF_BYPOSITION|MF_SEPARATOR, NULL, NULL);
		}
	}
#endif

	DWORD nSize = sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_ATTACHGUIAPP);
	CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_ATTACHGUIAPP, nSize);

	gnAttachGuiClientFlags = agaf_Success;
	// С приложенями .Net - приходится работать как с WS_CHILD,
	// иначе в них "не нажимаются" тулбары и меню
	if (IsDotNetWindow(hWindow))
	{
		gnAttachGuiClientFlags |= (agaf_DotNet|agaf_WS_CHILD);
	}
	// Если в окне нет меню - работаем с ним как с WS_CHILD
	// так не возникает проблем с активацией и т.д.
	else if (GetMenu(hWindow) == NULL)
	{
		if (IsQtWindow(asClassA, asClassW))
			gnAttachGuiClientFlags |= (agaf_NoMenu|agaf_QtWindow|agaf_WS_CHILD);
		else
			gnAttachGuiClientFlags |= (agaf_NoMenu|agaf_WS_CHILD);
	}
	pIn->AttachGuiApp.nFlags = gnAttachGuiClientFlags;
	pIn->AttachGuiApp.nPID = GetCurrentProcessId();
	pIn->AttachGuiApp.nServerPID = gnServerPID;
	pIn->AttachGuiApp.hAppWindow = hWindow;
	pIn->AttachGuiApp.Styles.nStyle = nCurStyle; // стили могли измениться после создания окна,
	pIn->AttachGuiApp.Styles.nStyleEx = nCurStyleEx; // поэтому получим актуальные
	GetWindowRect(hWindow, &pIn->AttachGuiApp.rcWindow);
	GetModuleFileName(NULL, pIn->AttachGuiApp.sAppFilePathName, countof(pIn->AttachGuiApp.sAppFilePathName));
	pIn->AttachGuiApp.hkl = (DWORD)(LONG)(LONG_PTR)GetKeyboardLayout(0);

	wchar_t szGuiPipeName[128];
	msprintf(szGuiPipeName, countof(szGuiPipeName), CEGUIPIPENAME, L".", LODWORD(ghConEmuWnd));


	// AttachThreadInput
	DWORD nConEmuTID = GetWindowThreadProcessId(ghConEmuWnd, NULL);
	DWORD nTID = GetCurrentThreadId();
	_ASSERTEX(nTID==gnHookMainThreadId || nTID==gnAttachGuiClientThreadId);
	BOOL bAttachRc = AttachThreadInput(nTID, nConEmuTID, TRUE);
	DWORD nAttachErr = GetLastError();
	UNREFERENCED_PARAMETER(bAttachRc); UNREFERENCED_PARAMETER(nAttachErr);

	HWND hPreFocus = GetFocus();


	CESERVER_REQ* pOut = ExecuteCmd(szGuiPipeName, pIn, 0/*Default timeout*/, NULL);

	ExecuteFreeResult(pIn);

	// abStyleHidden == TRUE, если окно при создании указало флаг WS_VISIBLE (т.е. не собиралось звать ShowWindow)

	if (pOut)
	{
		if (pOut->hdr.cbSize > sizeof(CESERVER_REQ_HDR))
		{
			_ASSERTE((pOut->AttachGuiApp.nFlags & agaf_Success) == agaf_Success);

			BOOL lbRc = FALSE;

			_ASSERTE(pOut->AttachGuiApp.hConEmuBack && pOut->AttachGuiApp.hConEmuDc && (HWND)pOut->AttachGuiApp.hConEmuDc!=(HWND)pOut->AttachGuiApp.hConEmuBack);
			_ASSERTE((ghConEmuWndBack==NULL) || (pOut->AttachGuiApp.hConEmuBack==ghConEmuWndBack));
			_ASSERTE(ghConEmuWnd && (ghConEmuWnd==pOut->AttachGuiApp.hConEmuWnd));
			ghConEmuWnd = pOut->AttachGuiApp.hConEmuWnd;
			SetConEmuHkWindows(pOut->AttachGuiApp.hConEmuDc, pOut->AttachGuiApp.hConEmuBack);

			//gbGuiClientHideCaption = pOut->AttachGuiApp.bHideCaption;
			gGuiClientStyles = pOut->AttachGuiApp.Styles;

			#ifdef _DEBUG
			HWND hFocus = GetFocus();
			DWORD nFocusPID = 0;

			if (hFocus)
			{
				GetWindowThreadProcessId(hFocus, &nFocusPID);
				DWORD nConEmuPID = 0; GetWindowThreadProcessId(ghConEmuWnd, &nConEmuPID);
				if (nFocusPID != GetCurrentProcessId() && nFocusPID != nConEmuPID)
				{
					// Допустимая ситуация, когда одновременно во вкладки цепляется
					// несколько ChildGui. Например, при запуске из bat-файла...
					hFocus = hWindow;
				}
			}
			#endif

			if (pOut->AttachGuiApp.hkl)
			{
				LONG_PTR hkl = (LONG_PTR)(LONG)pOut->AttachGuiApp.hkl;
				lbRc = ActivateKeyboardLayout((HKL)hkl, KLF_SETFORPROCESS) != NULL;
				UNREFERENCED_PARAMETER(lbRc);
			}

			//grcAttachGuiClientPos = pOut->AttachGuiApp.rcWindow;
			ReplaceGuiAppWindow(abStyleHidden);

			//if (hPreFocus)
			//{
			//	SetFocus(hPreFocus);
			//}
			UINT nMsgID = RegisterWindowMessageW(CONEMUMSG_RESTORECHILDFOCUS);
			PostMessageW(ghConEmuWndBack, nMsgID, 0,0);

			//// !!! OnSetForegroundWindow не подходит - он дергает Cmd.
			////SetForegroundWindow(ghConEmuWnd);
			//#if 0
			//wchar_t szClass[64] = {}; GetClassName(hFocus, szClass, countof(szClass));
			//MessageBox(NULL, szClass, L"WasFocused", MB_SYSTEMMODAL);
			//#endif
			////if (!(nCurStyle & WS_CHILDWINDOW))
			//{
			//	// Если ставить WS_CHILD - пропадет меню!
			//	//nCurStyle = (nCurStyle | WS_CHILDWINDOW|WS_TABSTOP); // & ~(WS_THICKFRAME/*|WS_CAPTION|WS_MINIMIZEBOX|WS_MAXIMIZEBOX*/);
			//	//SetWindowLongPtr(hWindow, GWL_STYLE, nCurStyle);
			//	if (gnAttachGuiClientFlags & agaf_DotNet)
			//	{
			//	}
			//	else
			//	{
			//		SetParent(hWindow, ghConEmuWndBack);
			//	}
			//}
			//
			//RECT rcGui = grcAttachGuiClientPos = pOut->AttachGuiApp.rcWindow;
			//if (SetWindowPos(hWindow, HWND_TOP, rcGui.left,rcGui.top, rcGui.right-rcGui.left, rcGui.bottom-rcGui.top,
			//	SWP_DRAWFRAME | /*SWP_FRAMECHANGED |*/ (abStyleHidden ? SWP_SHOWWINDOW : 0)))
			//{
			//	if (abStyleHidden)
			//		abStyleHidden = FALSE;
			//}
			//
			//// !!! OnSetForegroundWindow не подходит - он дергает Cmd.
			//SetForegroundWindow(ghConEmuWnd);
			////if (hFocus)
			////SetFocus(hFocus ? hFocus : hWindow); // hFocus==NULL, эффекта нет
			////OnSetForegroundWindow(hWindow);
			////PostMessageW(ghConEmuWnd, WM_NCACTIVATE, TRUE, 0);
			////PostMessageW(ghConEmuWnd, WM_NCPAINT, 0, 0);
			//PostMessageW(hWindow, WM_NCPAINT, 0, 0);
		}
		ExecuteFreeResult(pOut);
	}

	if (abStyleHidden)
	{
		ShowWindow(hWindow, SW_SHOW);
	}
}

void OnShowGuiClientWindow(HWND hWnd, int &nCmdShow, BOOL &rbGuiAttach, BOOL &rbInactive)
{
#ifdef _DEBUG
	STARTUPINFO si = {sizeof(si)};
	GetStartupInfo(&si);
#endif

	rbInactive = FALSE;

	//if (ghConEmuWnd)
	//{
	//	DWORD nConEmuExStyle = GetWindowLongPtr(ghConEmuWnd, GWL_EXSTYLE);
	//	if (nConEmuExStyle & WS_EX_TOPMOST)
	//	{
	//		DWORD nExtStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
	//		if (!(nExtStyle & WS_EX_TOPMOST))
	//		{
	//			nExtStyle |= WS_EX_TOPMOST;
	//			SetWindowLongPtr(hWnd, GWL_EXSTYLE, nExtStyle);
	//		}
	//	}
	//}

	if ((!ghAttachGuiClient) && gbAttachGuiClient && (nCmdShow >= SW_SHOWNORMAL))
	{
		// VLC создает несколько "подходящих" окон, но ShowWindow зовет
		// только для одного из них. Поэтому фактический аттач делаем
		// только в том случае, если окно "видимое"
		HMENU hMenu = GetMenu(hWnd);
		wchar_t szClassName[255]; GetClassName(hWnd, szClassName, countof(szClassName));
		DWORD nCurStyle = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
		DWORD nCurStyleEx = (DWORD)GetWindowLongPtr(hWnd, GWL_EXSTYLE);
		BOOL bAttachGui = TRUE;

		if (ghAttachGuiClient == NULL)
		{
			HWND hWndParent = ::GetParent(hWnd);
			DWORD dwStyle = nCurStyle, dwExStyle = nCurStyleEx;
			BOOL bStyleHidden;

			CheckCanCreateWindow(NULL, szClassName, dwStyle, dwExStyle, hWndParent, bAttachGui, bStyleHidden);
		}

		// Пробуем
		if (bAttachGui)
		{
			OnGuiWindowAttached(hWnd, hMenu, NULL, szClassName, nCurStyle, nCurStyleEx, FALSE, nCmdShow);
		}
	}

	if (gbForceShowGuiClient && (ghAttachGuiClient == hWnd))
	{
		//if (nCmdShow == SW_HIDE)
		//	nCmdShow = SW_SHOWNORMAL;

		HWND hCurParent = GetParent(hWnd);
		if (hCurParent != ghConEmuWndBack)
		{
			DWORD nCurStyle = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
			DWORD nCurStyleEx = (DWORD)GetWindowLongPtr(hWnd, GWL_EXSTYLE);

			DWORD nSize = sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_ATTACHGUIAPP);
			CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_ATTACHGUIAPP, nSize);

			_ASSERTE(gnServerPID!=0);
			pIn->AttachGuiApp.nServerPID = gnServerPID;
			pIn->AttachGuiApp.nFlags = gnAttachGuiClientFlags;
			pIn->AttachGuiApp.nPID = GetCurrentProcessId();
			pIn->AttachGuiApp.hAppWindow = hWnd;
			pIn->AttachGuiApp.Styles.nStyle = nCurStyle; // стили могли измениться после создания окна,
			pIn->AttachGuiApp.Styles.nStyleEx = nCurStyleEx; // поэтому получим актуальные
			GetWindowRect(hWnd, &pIn->AttachGuiApp.rcWindow);
			GetModuleFileName(NULL, pIn->AttachGuiApp.sAppFilePathName, countof(pIn->AttachGuiApp.sAppFilePathName));

			wchar_t szGuiPipeName[128];
			msprintf(szGuiPipeName, countof(szGuiPipeName), CEGUIPIPENAME, L".", LODWORD(ghConEmuWnd));

			CESERVER_REQ* pOut = ExecuteCmd(szGuiPipeName, pIn, 0/*Default timeout*/, NULL);
			ExecuteFreeResult(pIn);
			if (pOut)
			{
				if (pOut->hdr.cbSize > sizeof(CESERVER_REQ_HDR) && (pOut->AttachGuiApp.nFlags & agaf_Success))
				{
					//grcAttachGuiClientPos = pOut->AttachGuiApp.rcWindow;

					_ASSERTE((ghConEmuWndBack==NULL) || (pOut->AttachGuiApp.hConEmuBack==ghConEmuWndBack));
					_ASSERTE(ghConEmuWnd && (ghConEmuWnd==pOut->AttachGuiApp.hConEmuWnd));
					ghConEmuWnd = pOut->AttachGuiApp.hConEmuWnd;
					SetConEmuHkWindows(pOut->AttachGuiApp.hConEmuDc, pOut->AttachGuiApp.hConEmuBack);
					//gbGuiClientHideCaption = pOut->AttachGuiApp.bHideCaption;
					gGuiClientStyles = pOut->AttachGuiApp.Styles;
					//Если приложение создается в НЕ активной вкладке - фокус нужно вернуть в ConEmu
					rbInactive = (pOut->AttachGuiApp.nFlags & agaf_Inactive) == agaf_Inactive;
				}
				ExecuteFreeResult(pOut);
			}

			//OnSetParent(hWnd, ghConEmuWndBack);
		}

		ReplaceGuiAppWindow(FALSE);

		//RECT rcGui = grcAttachGuiClientPos;
		//SetWindowPos(hWnd, HWND_TOP, rcGui.left,rcGui.top, rcGui.right-rcGui.left, rcGui.bottom-rcGui.top,
		//	SWP_FRAMECHANGED);

		nCmdShow = SW_SHOWNORMAL;
		gbForceShowGuiClient = FALSE; // Один раз?
		rbGuiAttach = TRUE;
	}
}

void OnPostShowGuiClientWindow(HWND hWnd, int nCmdShow)
{
	CheckOrigGuiClientRect();

	RECT rcGui = {0,0,grcAttachGuiClientOrig.right-grcAttachGuiClientOrig.left,grcAttachGuiClientOrig.bottom-grcAttachGuiClientOrig.top};

	GetClientRect(hWnd, &rcGui);
	SendMessageW(hWnd, WM_SIZE, SIZE_RESTORED, MAKELONG((rcGui.right-rcGui.left),(rcGui.bottom-rcGui.top)));
}

void CorrectGuiChildRect(DWORD anStyle, DWORD anStyleEx, RECT& rcGui)
{
	RECT rcShift = {};

	if ((anStyle != gGuiClientStyles.nStyle) || (anStyleEx != gGuiClientStyles.nStyleEx))
	{
		DWORD nSize = sizeof(CESERVER_REQ_HDR)+sizeof(GuiStylesAndShifts);
		CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_GUICLIENTSHIFT, nSize);
		if (pIn)
		{
			pIn->GuiAppShifts.nStyle = anStyle;
			pIn->GuiAppShifts.nStyleEx = anStyleEx;
			wchar_t szOurExe[MAX_PATH*2] = L"";
			GetModuleFileName(NULL, szOurExe, countof(szOurExe));
			lstrcpyn(pIn->GuiAppShifts.szExeName, PointToName(szOurExe), countof(pIn->GuiAppShifts.szExeName));

			wchar_t szGuiPipeName[128];
			msprintf(szGuiPipeName, countof(szGuiPipeName), CEGUIPIPENAME, L".", LODWORD(ghConEmuWnd));

			CESERVER_REQ* pOut = ExecuteCmd(szGuiPipeName, pIn, 10000, NULL);
			if (pOut)
			{
				gGuiClientStyles = pOut->GuiAppShifts;
				ExecuteFreeResult(pOut);
			}
			ExecuteFreeResult(pIn);
		}
	}

	rcShift = gGuiClientStyles.Shifts;

	rcGui.left += rcShift.left; rcGui.top += rcShift.top;
	rcGui.right += rcShift.right; rcGui.bottom += rcShift.bottom;
}

RECT AttachGuiClientPos(DWORD anStyle /*= 0*/, DWORD anStyleEx /*= 0*/)
{
	//CheckOrigGuiClientRect();

	RECT rcPos = {}; // {0,0,grcAttachGuiClientOrig.right-grcAttachGuiClientOrig.left,grcAttachGuiClientOrig.bottom-grcAttachGuiClientOrig.top};

	if (ghConEmuWndBack)
	{
		GetClientRect(ghConEmuWndBack, &rcPos);
		// AttachGuiClientPos may be called from CheckCanCreateWindow
		// and ghAttachGuiClient is NULL at this point
		_ASSERTEX(ghAttachGuiClient!=NULL || (anStyle||anStyleEx));

		DWORD nStyle = (anStyle || anStyleEx) ? anStyle : (DWORD)GetWindowLongPtr(ghAttachGuiClient, GWL_STYLE);
		DWORD nStyleEx = (anStyle || anStyleEx) ? anStyleEx : (DWORD)GetWindowLongPtr(ghAttachGuiClient, GWL_EXSTYLE);

		CorrectGuiChildRect(nStyle, nStyleEx, rcPos);
	}
	else
	{
		_ASSERTEX(ghConEmuWndBack!=NULL);
		_ASSERTEX(ghAttachGuiClient!=NULL);
		GetWindowRect(ghConEmuWndBack, &rcPos);
	}

	return rcPos;
}

bool OnSetGuiClientWindowPos(HWND hWnd, HWND hWndInsertAfter, int &X, int &Y, int &cx, int &cy, UINT uFlags)
{
	bool lbChanged = false;
	// GUI приложениями запрещено самостоятельно двигаться внутри ConEmu
	if (ghAttachGuiClient && hWnd == ghAttachGuiClient)
	{
		// -- что-то GetParent возвращает NULL (для cifirica по крайней мере)
		//if (GetParent(hWnd) == ghConEmuWndBack)
		{
			RECT rcGui = AttachGuiClientPos();
			X = rcGui.left;
			Y = rcGui.top;
			cx = rcGui.right - X;
			cy = rcGui.bottom - Y;
			lbChanged = true;
		}
	}
	return lbChanged;
}

void SetGuiExternMode(BOOL abUseExternMode, LPRECT prcOldPos /*= NULL*/)
{
	if (gbGuiClientExternMode != abUseExternMode)
	{
		gbGuiClientExternMode = abUseExternMode;

		if (!abUseExternMode)
		{
			ReplaceGuiAppWindow(FALSE);
		}
		else
		{
			RECT rcGui;
			if (prcOldPos && (prcOldPos->right > prcOldPos->left && prcOldPos->bottom > prcOldPos->top))
				rcGui = *prcOldPos;
			else
				GetWindowRect(ghAttachGuiClient, &rcGui);

			if (gbAttachGuiClientStyleOk)
			{
				SetWindowLongPtr(ghAttachGuiClient, GWL_STYLE, gnAttachGuiClientStyle & ~WS_VISIBLE);
				SetWindowLongPtr(ghAttachGuiClient, GWL_EXSTYLE, gnAttachGuiClientStyleEx);
			}
			SetParent(ghAttachGuiClient, NULL);

			TODO("Вернуть старый размер?");
			SetWindowPos(ghAttachGuiClient, ghConEmuWnd, rcGui.left,rcGui.top, rcGui.right-rcGui.left, rcGui.bottom-rcGui.top,
				SWP_DRAWFRAME | SWP_SHOWWINDOW | SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE);
		}
	}
}
