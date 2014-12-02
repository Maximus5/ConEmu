﻿
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

#define HIDE_USE_EXCEPTION_INFO
#include "header.h"
#include "TaskBarGhost.h"
#include "ConEmu.h"
#include "VirtualConsole.h"
#include "RealConsole.h"
#include "../common/ConEmuCheck.h"
#include "../common/WUser.h"

//#ifdef __GNUC__
#include "DwmApi_Part.h"
//#endif

#define UPDATE_DELTA 1000

ATOM CTaskBarGhost::mh_Class = 0;

CTaskBarGhost::CTaskBarGhost(CVirtualConsole* apVCon)
{
	mp_VCon = apVCon;

	mh_Ghost = NULL;
	memset(&m_TabSize, 0, sizeof(m_TabSize));
	mh_Snap = NULL;
	mn_LastUpdate = 0;

	mb_SimpleBlack = TRUE;

	mn_MsgUpdateThumbnail = RegisterWindowMessage(L"ConEmu::TaskBarGhost");

	mh_SkipActivateEvent = NULL;
	mb_WasSkipActivate = false;
	ms_LastTitle[0] = 0;
	ZeroMemory(&mbmi_Snap, sizeof(mbmi_Snap));
	mpb_DS = NULL;
}

CTaskBarGhost::~CTaskBarGhost()
{
	if (mh_Snap)
	{
		DeleteObject(mh_Snap);
		mh_Snap = NULL;
	}
	if (mh_Ghost && IsWindow(mh_Ghost))
	{
		//_ASSERTE(mh_Ghost==NULL);
		DestroyWindow(mh_Ghost);
	}

	if (mh_SkipActivateEvent)
	{
		CloseHandle(mh_SkipActivateEvent);
		mh_SkipActivateEvent = NULL;
	}
}

CTaskBarGhost* CTaskBarGhost::Create(CVirtualConsole* apVCon)
{
	_ASSERTE(isMainThread());

	if (mh_Class == 0)
	{
		WNDCLASSEX wcex = {0};
		wcex.cbSize         = sizeof(wcex);
		wcex.lpfnWndProc    = GhostStatic;
		wcex.hInstance      = g_hInstance;
		wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
		wcex.lpszClassName  = VirtualConsoleClassGhost;

		mh_Class = ::RegisterClassEx(&wcex);
	}

	CTaskBarGhost* pGhost = new CTaskBarGhost(apVCon);

	pGhost->UpdateTabSnapshoot(TRUE);

#if 0
	if (!pGhost->CreateTabSnapshoot(...))
	{
		delete pGhost;
		return NULL;
	}
#endif

	DWORD dwStyle, dwStyleEx;
	if (IsWindows7)
	{
		dwStyleEx = WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_ACCEPTFILES;
		dwStyle = WS_POPUP | WS_BORDER | WS_SYSMENU | WS_CAPTION;
	}
	else
	{
		dwStyleEx = WS_EX_NOACTIVATE | WS_EX_APPWINDOW | WS_EX_ACCEPTFILES;
		dwStyle = WS_OVERLAPPED | WS_BORDER | WS_SYSMENU | WS_CAPTION;
		if ((gpConEmu->isVConValid(apVCon) > 0) /*&& (gpConEmu->GetVCon(1) == NULL)*/)
			dwStyle |= WS_VISIBLE;
	}

	pGhost->mh_Ghost = ::CreateWindowEx(dwStyleEx,
			VirtualConsoleClassGhost, pGhost->CheckTitle(),
			dwStyle,
			-32000, -32000, 10, 10,
			ghWnd, NULL, g_hInstance, (LPVOID)pGhost);

	if (!pGhost->mh_Ghost)
	{
		delete pGhost;
		return NULL;
	}

	return pGhost;
}

HWND CTaskBarGhost::GhostWnd()
{
	if (!this)
	{
		_ASSERTE(this!=NULL);
		return NULL;
	}
	return mh_Ghost;
}

void CTaskBarGhost::UpdateGhostSize()
{
	HWND hView = mp_VCon->GetView();
	RECT rcClient = {}; GetClientRect(hView, &rcClient);
	m_TabSize.VConSize.x = rcClient.right;
	m_TabSize.VConSize.y = rcClient.bottom;

	if (mh_Ghost)
	{
		int nShowWidth = 0, nShowHeight = 0;
		if (CalcThumbnailSize(200, 150, nShowWidth, nShowHeight))
		{
			RECT rcGhost = {}; GetWindowRect(mh_Ghost, &rcGhost);
			if ((rcGhost.right - rcGhost.left) != nShowWidth || (rcGhost.bottom - rcGhost.top) != nShowHeight
				|| rcGhost.left != -32000 || rcGhost.top != -32000)
			{
				SetWindowPos(mh_Ghost, NULL, -32000, -32000, nShowWidth, nShowHeight, SWP_NOZORDER|SWP_NOACTIVATE);
			}
		}
	}
}

BOOL CTaskBarGhost::UpdateTabSnapshoot(BOOL abSimpleBlack, BOOL abNoSnapshoot)
{
	if (!isMainThread())
	{
		PostMessage(mh_Ghost, mn_MsgUpdateThumbnail, abSimpleBlack, abNoSnapshoot);
		return FALSE;
	}

	mb_SimpleBlack = abSimpleBlack;

	CheckTitle();

	UpdateGhostSize();

	if (!abNoSnapshoot && NeedSnapshootCache())
	{
		if (mp_VCon->isVisible())
			CreateTabSnapshoot();
	}

	gpConEmu->DwmInvalidateIconicBitmaps(mh_Ghost);
	return TRUE;
}

BOOL CTaskBarGhost::CreateTabSnapshoot()
{
	CheckTitle();

	if (!gpConEmu->Taskbar_GhostSnapshootRequired())
		return FALSE; // Сразу выйдем.

	if (!gpConEmu->isValid(mp_VCon))
		return FALSE;

	POINT PtOffset = {}, PtViewOffset = {}, PtSize = {}, PtViewSize = {};
	GetPreviewPosSize(&PtOffset, &PtViewOffset, &PtSize, &PtViewSize);
	if (PtSize.x<=0 || PtSize.y<=0 || PtViewSize.x<=0 || PtViewSize.y<=0)
	{
		_ASSERTE(!(PtSize.x<=0 || PtSize.y<=0 || PtViewSize.x<=0 || PtViewSize.y<=0));
		return FALSE;
	}

	if (mh_Snap)
	{
		bool lbChanged = false;
		if ((m_TabSize.BitmapSize.x != PtSize.x || m_TabSize.BitmapSize.y != PtSize.y)
			|| (m_TabSize.VConSize.x != PtViewSize.x || m_TabSize.VConSize.y != PtViewSize.y))
		{
			lbChanged = true;
		}
		else
		{
			BITMAP bi = {};
			GetObject(mh_Snap, sizeof(bi), &bi);
			if ((bi.bmWidth != PtSize.x) || (bi.bmHeight != PtSize.y))
				lbChanged = true;
		}

		if (lbChanged)
		{
			DeleteObject(mh_Snap); mh_Snap = NULL;
		}
		//abForce = TRUE;
	}

	//if (!abForce)
	//{
	//	DWORD dwDelta = GetTickCount() - mn_LastUpdate;
	//	if (dwDelta < UPDATE_DELTA)
	//		return FALSE;
	//}

	m_TabSize.VConSize = PtViewSize;
	m_TabSize.BitmapSize = PtSize;
	m_TabSize.UsedRect = MakeRect(PtViewOffset.x, PtViewOffset.y, min(PtSize.x,(PtViewOffset.x+PtViewSize.x)), min(PtSize.y,(PtViewOffset.y+PtViewSize.y)));

	HDC hdcMem = CreateCompatibleDC(NULL);
	if (hdcMem != NULL)
	{
		if (mh_Snap == NULL)
		{
			ZeroMemory(&mbmi_Snap.bmiHeader, sizeof(BITMAPINFOHEADER));
			mbmi_Snap.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			mbmi_Snap.bmiHeader.biWidth = PtSize.x;
			mbmi_Snap.bmiHeader.biHeight = -PtSize.y;  // Use a top-down DIB
			mbmi_Snap.bmiHeader.biPlanes = 1;
			mbmi_Snap.bmiHeader.biBitCount = 32;

			mh_Snap = CreateDIBSection(hdcMem, &mbmi_Snap, DIB_RGB_COLORS, (VOID**)&mpb_DS, NULL, 0);

			// Чтобы в созданном mh_Snap гарантировано не было "мусора"
			if (mh_Snap && mpb_DS)
			{
				_ASSERTE(sizeof(COLORREF)==4);
				memset(mpb_DS, 0, PtSize.x*PtSize.y*sizeof(COLORREF));
			}
		}

		if (mh_Snap != NULL)
		{
			HBITMAP hOld = (HBITMAP)SelectObject(hdcMem, mh_Snap);

			// Если в "консоли" есть графические окна (GUI Application, PicView, MMView, PanelViews, и т.п.)
			// то делать snapshoot по тексту консоли некорректно - нужно "фотографировать" содержимое окна
			if (NeedSnapshootCache())
			{
				HWND hView = mp_VCon->GetView();
				RECT rcSnap = {}; GetWindowRect(hView, &rcSnap);

				bool bLockSnapshoot = false;
				if (gpConEmu->IsDwm())
				{
					// Если уже всплыл "DWM-ное" окошко с иконками...
					HWND hFind = FindWindowEx(NULL, NULL, L"TaskListOverlayWnd", NULL);
					if (hFind)
					{
						RECT rcDwm = {}; GetWindowRect(hFind, &rcDwm);
						RECT rcInter = {};
						if (IntersectRect(&rcInter, &rcSnap, &rcDwm))
							bLockSnapshoot = true;
					}
				}

				if (!bLockSnapshoot)
				{
					WARNING("TODO: Use WM_PRINT & WM_PRINTCLIENT?");
					// Нужно фото экрана
					#if 1
					// Этот метод вроде срабатывает, но
					// - если снапшот запрашивается в момент отображения DWM-иконок
					//   то он (снапшот) захватит и само плавающее окно DWM для иконок
					HDC hdcScrn = GetDC(NULL);
					BitBlt(hdcMem, PtViewOffset.x,PtViewOffset.y, PtViewSize.x,PtViewSize.y, hdcScrn, rcSnap.left,rcSnap.top, SRCCOPY);
					ReleaseDC(NULL, hdcScrn);
					#else
					HDC hdcScrn;
					// -- Этот метод не захватывает содержимое других окон (PicView, GUI-apps, и т.п.)
					hdcScrn = GetDC(hView);
					//hdcScrn = GetDCEx(hView, NULL, 0);
					BitBlt(hdcMem, PtViewOffset.x,PtViewOffset.y, PtViewSize.x,PtViewSize.y, hdcScrn, 0,0, SRCCOPY);
					ReleaseDC(hView, hdcScrn);
					#endif
				}
			}
			else
			{
				// Просто отрисуем "консоль" на нашем DC
				RECT rcPaint = m_TabSize.UsedRect;
				mp_VCon->PrintClient(hdcMem, true, &rcPaint);
			}

			SelectObject(hdcMem, hOld);
		}

		if (hdcMem) DeleteDC(hdcMem);
	}

	mn_LastUpdate = GetTickCount();
	return (mh_Snap != NULL);
}

bool CTaskBarGhost::CalcThumbnailSize(int nWidth, int nHeight, int &nShowWidth, int &nShowHeight)
{
	if (nWidth && nHeight && m_TabSize.VConSize.x && m_TabSize.VConSize.y)
	{
		if ((nWidth / (double)nHeight) > (m_TabSize.VConSize.x / (double)m_TabSize.VConSize.y))
		{
			nShowHeight = nHeight;
			nShowWidth = nHeight * m_TabSize.VConSize.x / m_TabSize.VConSize.y;
		}
		else
		{
			nShowWidth = nWidth;
			nShowHeight = nWidth * m_TabSize.VConSize.y / m_TabSize.VConSize.x;
		}
		return true;
	}
	return false;
}

// Если в "консоли" есть графические окна (GUI Application, PicView, MMView, PanelViews, и т.п.)
// то делать snapshoot по тексту консоли некорректно - нужно "фотографировать" содержимое окна
bool CTaskBarGhost::NeedSnapshootCache()
{
	if (!gpConEmu->IsDwm())
	{
		return false;
	}

	HWND hView = mp_VCon->GetView();
	HWND hChild = FindWindowEx(hView, NULL, NULL, NULL);
	DWORD nCurPID = GetCurrentProcessId();
	DWORD nPID, nStyle;
	while (hChild)
	{
		if (GetWindowThreadProcessId(hChild, &nPID) && nPID != nCurPID)
		{
			nStyle = GetWindowLong(hChild, GWL_STYLE);
			if (nStyle & WS_VISIBLE)
				return true;
		}
		hChild = FindWindowEx(hView, hChild, NULL, NULL);
	}
	return false;
}

HBITMAP CTaskBarGhost::CreateThumbnail(int nWidth, int nHeight)
{
	HBITMAP hbm = NULL;
	HDC hdcMem = CreateCompatibleDC(NULL);
	if (hdcMem != NULL)
	{
		int nShowWidth = nWidth, nShowHeight = nHeight;
		int nX = 0, nY = 0;

		if (CalcThumbnailSize(nWidth, nHeight, nShowWidth, nShowHeight))
		{
			#if 0
			nX = (nWidth - nShowWidth) / 2;
			nY = (nHeight - nShowHeight) / 2;
			#else
			nWidth = nShowWidth;
			nHeight = nShowHeight;
			#endif
		}

		BITMAPINFO bmi;
		ZeroMemory(&bmi.bmiHeader, sizeof(BITMAPINFOHEADER));
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = nWidth;
		bmi.bmiHeader.biHeight = -nHeight;  // Use a top-down DIB
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;

		PBYTE pbDS = NULL;
		hbm = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, (VOID**)&pbDS, NULL, 0);
		if (hbm != NULL)
		{
			HBITMAP hOldMem = (HBITMAP)SelectObject(hdcMem, hbm);

			if (nWidth && nHeight && m_TabSize.VConSize.x && m_TabSize.VConSize.y)
			{
				memset(pbDS, 0, nWidth*nHeight*4);

				if (!mb_SimpleBlack)
				{
					if (NeedSnapshootCache())
					{
						// При наличии дочерних GUI окон - вероятно, snapshoot-ы имеют смысл только когда окна видимы
						if (mp_VCon->isVisible())
							CreateTabSnapshoot();

						if (mh_Snap)
						{
							HDC hdcSrc = CreateCompatibleDC(NULL);
							BITMAP bi = {}; GetObject(mh_Snap, sizeof(bi), &bi);
							HBITMAP hOldSrc = (HBITMAP)SelectObject(hdcSrc, mh_Snap);
							SetStretchBltMode(hdcMem, HALFTONE);
							StretchBlt(hdcMem, nX,nY,nShowWidth,nShowHeight, hdcSrc, 0,0,bi.bmWidth,bi.bmHeight, SRCCOPY);
							SelectObject(hdcSrc, hOldSrc);
							DeleteDC(hdcSrc);
						}
					}
					else
					{
						if (!mp_VCon->isVisible())
							mp_VCon->Update(true);
						if (mp_VCon->RCon()->isConsoleReady())
							mp_VCon->StretchPaint(hdcMem, nX, nY, nShowWidth, nShowHeight);
					}
				}

				#if 0
				if (mh_Snap)
				{
					HDC hdcSheet = CreateCompatibleDC(NULL);
					HBITMAP hOld = (HBITMAP)SelectObject(hdcSheet, mh_Snap);
					SetStretchBltMode(hdcMem, HALFTONE);
					StretchBlt(hdcMem, nX,nY,nShowWidth,nShowHeight, hdcSheet, 0,0,m_TabSize.VConSize.x, m_TabSize.VConSize.y, SRCCOPY);
					SelectObject(hdcSheet, hOld);
					DeleteDC(hdcSheet);
				}
				#endif

				// Apply Alpha Channel
				PBYTE pbRow = NULL;
				if (nWidth > nShowWidth)
				{
					for (int y = 0; y < nShowHeight; y++)
					{
						pbRow = pbDS + (y*nWidth+nX)*4;
						for (int x = 0; x < nShowWidth; x++)
						{
							pbRow[3] = 255;
							pbRow += 4;
						}
					}
				}
				else
				{
					for (int y = 0; y < nShowHeight; y++)
					{
						pbRow = pbDS + (y+nY)*4*nWidth;
						for (int x = 0; x < nWidth; x++)
						{
							pbRow[3] = 255;
							pbRow += 4;
						}
					}
				}
			}
			else
			{
				RECT rc = {0,0,nShowWidth,nShowHeight};
				FillRect(hdcMem, &rc, (HBRUSH)GetSysColorBrush(COLOR_WINDOW));
			}

			SelectObject(hdcMem, hOldMem);
		}

		DeleteDC(hdcMem);
	}

	return hbm;
}

LPCWSTR CTaskBarGhost::CheckTitle(BOOL abSkipValidation /*= FALSE*/)
{
	LPCWSTR pszTitle = NULL;
	TODO("Разбивка по табам консоли");
	if (mp_VCon && (abSkipValidation || gpConEmu->isValid(mp_VCon)) && mp_VCon->RCon())
	{
		pszTitle = mp_VCon->RCon()->GetTitle(true);
	}
	if (!pszTitle)
	{
		pszTitle = gpConEmu->GetDefaultTitle();
		_ASSERTE(pszTitle!=NULL);
	}

	if (mh_Ghost)
	{
		if (wcsncmp(ms_LastTitle, pszTitle, countof(ms_LastTitle)) != 0)
		{
			lstrcpyn(ms_LastTitle, pszTitle, countof(ms_LastTitle));
			SetWindowText(mh_Ghost, pszTitle);
		}
	}

#ifdef _DEBUG
	wchar_t szInfo[1024];
	getWindowInfo(mh_Ghost, szInfo);
#endif

	return pszTitle;
}

void CTaskBarGhost::ActivateTaskbar()
{
	gpConEmu->Taskbar_SetActiveTab(mh_Ghost);
#if 0
	// -- смена родителя (owner) на WinXP не срабатывает
	if (!IsWindows7)
	{
		HWND hParent = GetParent(ghWnd);
		if (hParent != mh_Ghost)
			gpConEmu->SetParent(mh_Ghost);
	}
#endif
}
LRESULT CALLBACK CTaskBarGhost::GhostStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult = 0;

	// Logger
	MSG msgStr = {hWnd, message, wParam, lParam};
	ConEmuMsgLogger::Log(msgStr, ConEmuMsgLogger::msgGhost);

	CTaskBarGhost *pWnd = (CTaskBarGhost*)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (pWnd == NULL && message == WM_NCCREATE)
	{
		LPCREATESTRUCTW lpcs = (LPCREATESTRUCTW)lParam;
		pWnd = (CTaskBarGhost*)lpcs->lpCreateParams;
		pWnd->mh_Ghost = hWnd;
		::SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pWnd);
		lResult = ::DefWindowProcW(hWnd, message, wParam, lParam);
	}
	else if (pWnd != NULL)
	{
		lResult = pWnd->GhostProc(message, wParam, lParam);
	}
	else
	{
		lResult = ::DefWindowProcW(hWnd, message, wParam, lParam);
	}

	return lResult;
}

LRESULT CTaskBarGhost::OnCreate()
{
	SetTimer(mh_Ghost, 101, 2500, NULL);

	wchar_t szEvtName[64];
	_wsprintf(szEvtName, SKIPLEN(countof(szEvtName)) CEGHOSTSKIPACTIVATE, (DWORD)mh_Ghost);
	SafeCloseHandle(mh_SkipActivateEvent);
	mh_SkipActivateEvent = CreateEvent(NULL, FALSE, FALSE, szEvtName);

	UpdateGhostSize();

	// Set DWM window attributes to indicate we'll provide the iconic bitmap, and
	// to always render the thumbnail using the iconic bitmap.
	gpConEmu->ForceSetIconic(mh_Ghost);

	// Win7 и выше!
	if (IsWindows7)
	{
		// Tell the taskbar about this tab window
		gpConEmu->Taskbar_RegisterTab(mh_Ghost, mp_VCon->isActive(false));
	}

	#if 0
	// -- смена родителя (owner) на WinXP не срабатывает, а для других систем (Vista+) вообще не требуется
	else if (gpConEmu->isActive(mp_VCon))
	{
		HWND hParent = GetParent(ghWnd);
		if (hParent != mh_Ghost)
			gpConEmu->SetParent(mh_Ghost);
	}
	#endif

	gpConEmu->OnGhostCreated(mp_VCon, mh_Ghost);

	OnDwmSendIconicThumbnail(200,150);

	return 0;
}

LRESULT CTaskBarGhost::OnTimer(WPARAM TimerID)
{
	if (!gpConEmu->isValid(mp_VCon))
	{
		DestroyWindow(mh_Ghost);
	}
	else
	{
		if (mp_VCon->isVisible() && NeedSnapshootCache())
		{
			UpdateTabSnapshoot(FALSE, FALSE);
		}
	}
	return 0;
}

LRESULT CTaskBarGhost::OnActivate(WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult = 0;
	HRESULT hr = S_FALSE;

	// The taskbar will activate this window, so pass along the activation
	// to the tab window outer frame.
	if (LOWORD(wParam) == WA_ACTIVE)
	{
		mb_WasSkipActivate = false;
		if (!IsWindows7 && mh_SkipActivateEvent)
		{
			// Чтобы не было глюков при Alt-Tab, Alt-Tab
			DWORD nSkipActivate = WaitForSingleObject(mh_SkipActivateEvent, 0);
			if (nSkipActivate == WAIT_OBJECT_0)
			{
				mb_WasSkipActivate = true;
				return 0;
			}
		}

		if (gpConEmu->isIconic())
			SendMessage(ghWnd, WM_SYSCOMMAND, SC_RESTORE, 0);

		apiSetForegroundWindow(ghWnd);

		// Update taskbar
		hr = gpConEmu->Taskbar_SetActiveTab(mh_Ghost);
		//hr = gpConEmu->DwmInvalidateIconicBitmaps(mh_Ghost); -- need?

		// Activate tab.
		// Чтобы при клике по миниатюре табы НЕ итерировались (gpSet->isMultiIterate)
		// сначала проверяем активный таб
		CVConGuard guard(mp_VCon);
		CVConGuard VCon;
		gpConEmu->GetActiveVCon(&VCon);
		if (VCon.VCon() != mp_VCon)
			gpConEmu->Activate(mp_VCon);

		// Forward message
		SendMessage(ghWnd, WM_ACTIVATE, wParam, lParam);
	}

	UNREFERENCED_PARAMETER(hr);
	return lResult;
}

LRESULT CTaskBarGhost::OnSysCommand(WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult = 0;
	// All syscommands except for close will be passed along to the tab window
	// outer frame. This allows functions such as move/size to occur properly.
	if (wParam != SC_CLOSE)
	{
		if (wParam == SC_RESTORE || wParam == SC_MINIMIZE || wParam == SC_MAXIMIZE)
		{
			if (gpConEmu->isIconic())
				wParam = (wParam == SC_MAXIMIZE) ? SC_MAXIMIZE : SC_MINIMIZE;
			else if (gpConEmu->isZoomed() || gpConEmu->isFullScreen())
				wParam = (wParam == SC_RESTORE) ? SC_RESTORE : SC_MINIMIZE;
			else
				wParam = (wParam == SC_MAXIMIZE) ? SC_MAXIMIZE : SC_MINIMIZE;
		}
		SendMessage(ghWnd, WM_SYSCOMMAND, wParam, lParam);
	}
	else
	{
		lResult = ::DefWindowProc(mh_Ghost, WM_SYSCOMMAND, wParam, lParam);
	}
	return lResult;
}

HICON CTaskBarGhost::OnGetIcon(WPARAM anIconType)
{
	HICON lResult = NULL;

	TODO("Получить иконку активного приложения в консоли");
	//lResult = SendMessage(ghWnd, message, wParam, lParam);
	if (anIconType == ICON_BIG)
		lResult = hClassIcon;
	else
		lResult = hClassIconSm;

	return lResult;
}

LRESULT CTaskBarGhost::OnClose()
{
	// The taskbar (or system menu) is asking this tab window to close. Ask the
	// tab window outer frame to destroy this tab.

	//SetWindowLongPtr(_hwnd, GWLP_USERDATA, 0);
	//DestroyWindow(mh_Ghost);

	SetForegroundWindow(ghWnd);

	mp_VCon->RCon()->CloseConsole(false, true);

	return 0;
}

LRESULT CTaskBarGhost::OnDwmSendIconicThumbnail(short anWidth, short anHeight)
{
	// This tab window is being asked to provide its iconic bitmap. This indicates
	// a thumbnail is being drawn.

	HRESULT hr = S_FALSE;
	HBITMAP hThumb = CreateThumbnail(anWidth, anHeight);
	if (hThumb)
	{
#ifdef _DEBUG
		BITMAP bi = {};
		GetObject(hThumb, sizeof(bi), &bi);
#endif
		hr = gpConEmu->DwmSetIconicThumbnail(mh_Ghost, hThumb);
		DeleteObject(hThumb);
	}

	UNREFERENCED_PARAMETER(hr);
	return 0;
}

// pPtOffset		Смещение левого верхнего угла сформированного LiveBitmap
//					относительно левого верхнего угла окна ghWnd.
//					Чаще всего, это разница между GetWindowRect(hView) и GetWindowRect(ghWnd)
//					Но! В дальнейшем, при появлении групп табов все может стать сложнее...
// pPtViewOffset	Если размер GetClientRect(hView) меньше рабочей области ghWnd - центрирование
//					Смещение - относительно формируемого LiveBitmap
// pPtSize			размер рабочей области (размер формируемого LiveBitmap)
// pPtViewSize		размер окна отрисовки
void CTaskBarGhost::GetPreviewPosSize(POINT* pPtOffset, POINT* pPtViewOffset, POINT* pPtSize, POINT* pPtViewSize)
{
	POINT ptOffset = {};

	//RECT rcMain = gpConEmu->CalcRect(CER_MAIN);
	//// размер всей рабочей области (но БЕЗ табов, прокруток, статусов)
	//RECT rcWork = gpConEmu->CalcRect(CER_WORKSPACE, rcMain, CER_MAIN, mp_VCon/*хотя VCon и не важен, нужен полный размер*/);
	RECT rcWork = gpConEmu->CalcRect(CER_WORKSPACE, mp_VCon/*хотя VCon и не важен, нужен полный размер*/);
	_ASSERTE(rcWork.right>rcWork.left && rcWork.bottom>rcWork.top);

	//RECT rcView = {0};
	#ifdef _DEBUG
	HWND hView = mp_VCon->GetView();
	#endif
	POINT ptViewSize = MakePoint(mp_VCon->Width, mp_VCon->Height);
	//if (hView)
	//{
	//	GetWindowRect(hView, &rcView);
	//}
	//else
	//{
	//	_ASSERTE(hView!=NULL && "mp_VCon->GetView() must returns DC window");
	//	// Если View не сформирован - получить размер всей рабочей области (но БЕЗ табов, прокруток, статусов)
	//	rcView = rcWork;
	//}

	WARNING("'+1'?")
	POINT szSize = MakePoint(rcWork.right-rcWork.left, rcWork.bottom-rcWork.top);
	//POINT ptViewOffset = MakePoint(szSize.x - (rcView.right-rcView.left), szSize.y - (rcView.bottom-rcView.top));
	POINT ptViewOffset = MakePoint(max(0,(szSize.x - ptViewSize.x)), max(0,(szSize.y - ptViewSize.y)));

	ptOffset = MakePoint(rcWork.left, rcWork.top);
	ConEmuWindowMode wm = gpConEmu->GetWindowMode();
	if ((wm == rFullScreen)
		|| ((wm == rMaximized) && (gpSet->isHideCaptionAlways() || gpSet->isHideCaption)))
	{
		// Финт ушами, т.к. в этих режимах идет принудительный сдвиг
		ptOffset.x += GetSystemMetrics(SM_CXFRAME);
		ptOffset.y += GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFRAME);
	}
	//if (!gpConEmu->isIconic())
	//{
	//	//	ptOffset.x += GetSystemMetrics(SM_CXFRAME);
	//	//	ptOffset.y += GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFRAME);
	//}
	////TODO("Проверить, учитывается ли DWM и прочая фигня с шириной рамок");
	//ptOffset.x = rcWork.left - rcMain.left;
	//ptOffset.y = rcWork.top - rcMain.top;
	//if (!gpConEmu->isIconic())
	//{
	//	ptOffset.x -= GetSystemMetrics(SM_CXFRAME);
	//	ptOffset.y -= GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFRAME);
	//}

	if (pPtOffset)
		*pPtOffset = ptOffset;
	if (pPtViewOffset)
		*pPtViewOffset = ptViewOffset;
	if (pPtSize)
		*pPtSize = szSize;
	if (pPtViewSize)
		*pPtViewSize = ptViewSize;
}

LRESULT CTaskBarGhost::OnDwmSendIconicLivePreviewBitmap()
{
	HRESULT hr = S_FALSE;

	// This tab window is being asked to provide a bitmap to show in live preview.
	// This indicates the tab's thumbnail in the taskbar is being previewed.
	//_SendLivePreviewBitmap();
	if (CreateTabSnapshoot())
	{
		POINT ptOffset = {};
		GetPreviewPosSize(&ptOffset, NULL, NULL, NULL);
		//HWND hView = mp_VCon->GetView();
		//if (hView)
		//{
		//	RECT rcMain = {0}; GetWindowRect(ghWnd, &rcMain);
		//	RECT rcView = {0}; GetWindowRect(hView, &rcView);
		//	TODO("Проверить, учитывается ли DWM и прочая фигня с шириной рамок");
		//	ptOffset.x = rcView.left - rcMain.left;
		//	ptOffset.y = rcView.top - rcMain.top;
		//	if (!gpConEmu->isIconic())
		//	{
		//		ptOffset.x -= GetSystemMetrics(SM_CXFRAME);
		//		ptOffset.y -= GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFRAME);
		//	}
		//}

		BITMAP bi = {};
		GetObject(mh_Snap, sizeof(bi), &bi);

		PBYTE pbRow = NULL;
		int nHeight = (bi.bmHeight < 0) ? -bi.bmHeight : bi.bmHeight;
		for (int y = 0; y < nHeight; y++)
		{
			pbRow = mpb_DS + (y*bi.bmWidth)*4;
			for (int x = 0; x < bi.bmWidth; x++)
			{
				pbRow[3] = 255;
				pbRow += 4;
			}
		}

#if 0
		//defined(_DEBUG)
		HDC hdc = GetDC(NULL);
		HDC hdcComp = CreateCompatibleDC(hdc);
		HBITMAP hOld = (HBITMAP)SelectObject(hdcComp, mh_Snap);
		BitBlt(hdc, 0,0,200,150, hdcComp, 0,0, SRCCOPY);
		SelectObject(hdcComp, hOld);
		DeleteDC(hdcComp);
		ReleaseDC(NULL, hdc);
#endif

		hr = gpConEmu->DwmSetIconicLivePreviewBitmap(mh_Ghost, mh_Snap, &ptOffset);
	}

	UNREFERENCED_PARAMETER(hr);
	return 0;
}

LRESULT CTaskBarGhost::OnDestroy()
{
	KillTimer(mh_Ghost, 101);

	#if 0
	HWND hParent = GetParent(ghWnd);
	if (hParent == mh_Ghost)
	{
		gpConEmu->SetParent(NULL);
	}
	#endif

	if (IsWindows7)
	{
		gpConEmu->Taskbar_UnregisterTab(mh_Ghost);
	}

	return 0;
}

LRESULT CTaskBarGhost::GhostProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult = 0;

#ifdef _DEBUG
	// Для уменьшения мусора в DebugLog
	if (message == WM_SETTEXT)
	{
		bool lbDbg = false;
	}
	else if (message == WM_WINDOWPOSCHANGING)
	{
		bool lbDbg = false;
	}
	else if (message != 0xAE/*WM_NCUAHDRAWCAPTION*/ && message != WM_GETTEXT && message != WM_GETMINMAXINFO
		&& message != WM_GETICON && message != WM_TIMER)
	{
		//wchar_t szDbg[127]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"GhostProc(%i{x%03X},%i,%i)\n", message, message, (DWORD)wParam, (DWORD)lParam);
		//OutputDebugStringW(szDbg);
	}
#endif

	switch (message)
	{
		case WM_CREATE:
			lResult = OnCreate();
			break;

		case WM_TIMER:
			lResult = OnTimer(wParam);
			break;

		case WM_ACTIVATE:
			lResult = OnActivate(wParam, lParam);
			break;

		case WM_SYSCOMMAND:
			lResult = OnSysCommand(wParam, lParam);
			break;

		case WM_GETICON:
			lResult = (LRESULT)OnGetIcon(wParam);
			break;

		case WM_CLOSE:
			lResult = OnClose();
			break;

		case WM_DWMSENDICONICTHUMBNAIL:
			lResult = OnDwmSendIconicThumbnail((short)HIWORD(lParam), (short)LOWORD(lParam));
			break;

		case WM_DWMSENDICONICLIVEPREVIEWBITMAP:
			lResult = OnDwmSendIconicLivePreviewBitmap();
			break;

		case WM_DESTROY:
			lResult = OnDestroy();
			break;

		//case WM_SYSKEYUP:
		case WM_NCACTIVATE:
			//if (wParam == VK_MENU)
			if (wParam)
			{
				if (mb_WasSkipActivate)
				{
					ResetEvent(mh_SkipActivateEvent);
					mb_WasSkipActivate = false;
					OnActivate(WA_ACTIVE, 0);
				}
			}
			lResult = ::DefWindowProcW(mh_Ghost, message, wParam, lParam);
			break;

		default:
			if (message == mn_MsgUpdateThumbnail)
			{
				lResult = UpdateTabSnapshoot(wParam!=0, lParam!=0);
			}
			else
			{
				lResult = ::DefWindowProcW(mh_Ghost, message, wParam, lParam);
			}
			break;
	}

	return lResult;
}
