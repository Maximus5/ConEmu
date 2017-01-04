
/*
Copyright (c) 2015-2017 Maximus5
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
	#define DebugString(x) //OutputDebugString(x)
	#define DefTermMsg(s) //MessageBox(NULL, s, L"ConEmuHk", MB_SYSTEMMODAL)
#else
	#define DebugString(x) //OutputDebugString(x)
	#define DefTermMsg(s) //MessageBox(NULL, s, L"ConEmuHk", MB_SYSTEMMODAL)
#endif

#include "../common/Common.h"

#include "GuiAttach.h"
#include "hkMessages.h"
#include "MainThread.h"

/* **************** */

bool CanSendMessage(HWND& hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT& lRc)
{
	if (ghConEmuWndDC && hWnd == ghConEmuWndDC)
	{
		switch (Msg)
		{
			case WM_SIZE:
			case WM_MOVE:
			case WM_SHOWWINDOW:
			case WM_SYSCOMMAND:
				// Эти сообщения - вообще игнорировать
				return false;
			case WM_INPUTLANGCHANGEREQUEST:
			case WM_INPUTLANGCHANGE:
			case WM_QUERYENDSESSION:
			case WM_ENDSESSION:
			case WM_QUIT:
			case WM_DESTROY:
			case WM_CLOSE:
			case WM_ENABLE:
			case WM_SETREDRAW:
			case WM_GETTEXT:
			case WM_GETTEXTLENGTH:
				// Эти сообщения - нужно посылать в RealConsole
				hWnd = ghConWnd;
				return true;
			case WM_SETICON:
				TODO("Можно бы передать иконку в ConEmu и показать ее на табе");
				break;
		}
	}
	return true; // разрешено
}

void PatchGuiMessage(bool bReceived, HWND& hWnd, UINT& Msg, WPARAM& wParam, LPARAM& lParam)
{
	if (!ghAttachGuiClient)
		return;

	switch (Msg)
	{
	case WM_MOUSEACTIVATE:
		if (wParam == (WPARAM)ghConEmuWnd)
		{
			wParam = (WPARAM)ghAttachGuiClient;
		}
		break;

	case WM_LBUTTONDOWN:
		if (bReceived && ghConEmuWnd && ghConWnd)
		{
			HWND hActiveCon = (HWND)GetWindowLongPtr(ghConEmuWnd, GWLP_USERDATA);
			if (hActiveCon != ghConWnd)
			{
				CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_ACTIVATECON, sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_ACTIVATECONSOLE));
				if (pIn)
				{
					pIn->ActivateCon.hConWnd = ghConWnd;
					CESERVER_REQ* pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
					ExecuteFreeResult(pIn);
					ExecuteFreeResult(pOut);
				}
			}
			#if 0
			// Если окно ChildGui активируется кликом - то ConEmu (holder)
			// может остаться "под" предыдущим окном бывшим в фокусе...
			// Победить пока не получается.
			DWORD nTID = 0, nPID = 0;
			HWND hFore = GetForegroundWindow();
			if (hFore && ((nTID = GetWindowThreadProcessId(hFore, &nPID)) != 0))
			{
				if ((nPID != GetCurrentProcessId()) && (nPID != gnGuiPID))
				{
					SetForegroundWindow(ghConEmuWnd);
				}
			}
			#endif
		}
		break;

	#ifdef _DEBUG
	case WM_DESTROY:
		if (hWnd == ghAttachGuiClient)
		{
			int iDbg = -1;
		}
		break;
	#endif
	}
}

/* **************** */

VOID WINAPI Onmouse_event(DWORD dwFlags, DWORD dx, DWORD dy, DWORD dwData, ULONG_PTR dwExtraInfo)
{
	//typedef VOID (WINAPI* Onmouse_event_t)(DWORD dwFlags, DWORD dx, DWORD dy, DWORD dwData, ULONG_PTR dwExtraInfo);
	ORIGINAL_EX(mouse_event);

	F(mouse_event)(dwFlags, dx, dy, dwData, dwExtraInfo);
}


UINT WINAPI OnSendInput(UINT nInputs, LPINPUT pInputs, int cbSize)
{
	//typedef UINT (WINAPI* OnSendInput_t)(UINT nInputs, LPINPUT pInputs, int cbSize);
	ORIGINAL_EX(SendInput);
	UINT nRc;

	nRc = F(SendInput)(nInputs, pInputs, cbSize);

	return nRc;
}


BOOL WINAPI OnPostMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	//typedef BOOL (WINAPI* OnPostMessageA_t)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	ORIGINAL_EX(PostMessageA);
	BOOL lRc = 0;
	LRESULT ll = 0;

	if (!CanSendMessage(hWnd, Msg, wParam, lParam, ll))
		return TRUE; // Обманем, это сообщение запрещено посылать в ConEmuDC

	if (ghAttachGuiClient)
		PatchGuiMessage(false, hWnd, Msg, wParam, lParam);

	if (F(PostMessageA))
		lRc = F(PostMessageA)(hWnd, Msg, wParam, lParam);

	return lRc;
}


BOOL WINAPI OnPostMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	//typedef BOOL (WINAPI* OnPostMessageW_t)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	ORIGINAL_EX(PostMessageW);
	BOOL lRc = 0;
	LRESULT ll = 0;

	if (!CanSendMessage(hWnd, Msg, wParam, lParam, ll))
		return TRUE; // Обманем, это сообщение запрещено посылать в ConEmuDC

	if (ghAttachGuiClient)
		PatchGuiMessage(false, hWnd, Msg, wParam, lParam);

	if (F(PostMessageW))
		lRc = F(PostMessageW)(hWnd, Msg, wParam, lParam);

	return lRc;
}


LRESULT WINAPI OnSendMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	//typedef LRESULT (WINAPI* OnSendMessageA_t)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	ORIGINAL_EX(SendMessageA);
	LRESULT lRc = 0;

	if (!CanSendMessage(hWnd, Msg, wParam, lParam, lRc))
		return lRc; // Обманем, это сообщение запрещено посылать в ConEmuDC

	if (ghAttachGuiClient)
		PatchGuiMessage(false, hWnd, Msg, wParam, lParam);

	if (F(SendMessageA))
		lRc = F(SendMessageA)(hWnd, Msg, wParam, lParam);

	return lRc;
}


LRESULT WINAPI OnSendMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	//typedef LRESULT (WINAPI* OnSendMessageW_t)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	ORIGINAL_EX(SendMessageW);
	LRESULT lRc = 0;

	if (!CanSendMessage(hWnd, Msg, wParam, lParam, lRc))
		return lRc; // Обманем, это сообщение запрещено посылать в ConEmuDC

	if (ghAttachGuiClient)
		PatchGuiMessage(false, hWnd, Msg, wParam, lParam);

	if (F(SendMessageW))
		lRc = F(SendMessageW)(hWnd, Msg, wParam, lParam);

	return lRc;
}


BOOL WINAPI OnGetMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
{
	//typedef BOOL (WINAPI* OnGetMessageA_t)(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
	ORIGINAL_EX(GetMessageA);
	BOOL lRc = 0;

	if (F(GetMessageA))
		lRc = F(GetMessageA)(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);

	if (lRc && ghAttachGuiClient)
		PatchGuiMessage(true, lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);

	return lRc;
}


BOOL WINAPI OnGetMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
{
	//typedef BOOL (WINAPI* OnGetMessageW_t)(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
	ORIGINAL_EX(GetMessageW);
	BOOL lRc = 0;

	if (F(GetMessageW))
		lRc = F(GetMessageW)(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);

	if (lRc && ghAttachGuiClient)
		PatchGuiMessage(true, lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);

	_ASSERTRESULT(TRUE);
	return lRc;
}


BOOL WINAPI OnPeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
{
	//typedef BOOL (WINAPI* OnPeekMessageA_t)(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
	ORIGINAL_EX(PeekMessageA);
	BOOL lRc = 0;

	if (F(PeekMessageA))
		lRc = F(PeekMessageA)(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);

	if (lRc && ghAttachGuiClient)
		PatchGuiMessage(true, lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);

	return lRc;
}


BOOL WINAPI OnPeekMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
{
	//typedef BOOL (WINAPI* OnPeekMessageW_t)(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
	ORIGINAL_EX(PeekMessageW);
	BOOL lRc = 0;

	if (F(PeekMessageW))
		lRc = F(PeekMessageW)(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);

	if (lRc && ghAttachGuiClient)
		PatchGuiMessage(true, lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);

	return lRc;
}
