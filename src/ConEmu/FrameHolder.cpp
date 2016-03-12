
/*
Copyright (c) 2009-2016 Maximus5
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

#define SHOWDEBUGSTR

#define HIDE_USE_EXCEPTION_INFO
#include "Header.h"
#include "ConEmu.h"
#include "DwmApi_Part.h"
#include "DwmHelper.h"
#include "FrameHolder.h"
#include "Menu.h"
#include "Options.h"
#include "OptionsClass.h"
#include "Status.h"
#include "TabBar.h"
#include "TrayIcon.h"
#include "../common/MSetter.h"

#ifdef _DEBUG
static int _nDbgStep = 0; wchar_t _szDbg[512];
#endif
#define DBGFUNCTION(s) //{ wsprintf(_szDbg, L"%i: %s", ++_nDbgStep, s); OutputDebugString(_szDbg); /*Sleep(1000);*/ }
#define DEBUGSTRSIZE(s) DEBUGSTR(s)
#define DEBUGSTRPAINT(s) //DEBUGSTR(s)

#ifdef _DEBUG
	//#define RED_CLIENT_FILL
	#undef RED_CLIENT_FILL
#else
	#undef RED_CLIENT_FILL
#endif

extern HICON hClassIconSm;

CFrameHolder::CFrameHolder()
{
	mb_NcActive = TRUE;
	mb_NcAnimate = FALSE;
	mb_Initialized = FALSE;
	mn_TabsHeight = 0;
	mb_WasGlassDraw = FALSE;
	mn_RedrawLockCount = 0;
	mb_RedrawRequested = false;
	mb_AllowPreserveClient = false;
	mb_DontPreserveClient = false;
	mn_WinCaptionHeight = 0;
	mn_FrameWidth = 0;
	mn_FrameHeight = 0;
	mn_OurCaptionHeight = 0;
	mn_CaptionDragHeight = 0;
	mn_InNcPaint = 0;
}

CFrameHolder::~CFrameHolder()
{
	mb_NcActive = FALSE;
}

void CFrameHolder::InitFrameHolder()
{
	mb_Initialized = TRUE;
	RecalculateFrameSizes();
}

// returns false if message not handled
bool CFrameHolder::ProcessNcMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult)
{
#ifdef _DEBUG
	if (mb_NcAnimate)
	{
		wchar_t szDbg[1024], szMsg[128], szInfo[255]; szInfo[0] = 0;
		switch (uMsg)
		{
		case WM_ERASEBKGND:
			lstrcpy(szMsg, L"WM_ERASEBKGND"); break;
		case WM_PAINT:
			lstrcpy(szMsg, L"WM_PAINT"); break;
		case WM_NCPAINT:
			lstrcpy(szMsg, L"WM_NCPAINT"); break;
		case WM_NCACTIVATE:
			lstrcpy(szMsg, L"WM_NCACTIVATE"); break;
		case WM_NCCALCSIZE:
			lstrcpy(szMsg, L"WM_NCCALCSIZE"); break;
		case WM_NCHITTEST:
			lstrcpy(szMsg, L"WM_NCHITTEST"); break;
		case WM_NCLBUTTONDOWN:
			lstrcpy(szMsg, L"WM_NCLBUTTONDOWN"); break;
		case WM_NCMOUSEMOVE:
			lstrcpy(szMsg, L"WM_NCMOUSEMOVE"); break;
		case WM_NCMOUSELEAVE:
			lstrcpy(szMsg, L"WM_NCMOUSELEAVE"); break;
		case WM_NCMOUSEHOVER:
			lstrcpy(szMsg, L"WM_NCMOUSEHOVER"); break;
		case WM_NCLBUTTONDBLCLK:
			lstrcpy(szMsg, L"WM_NCLBUTTONDBLCLK"); break;
		case 0xAE: /*WM_NCUAHDRAWCAPTION*/
			lstrcpy(szMsg, L"WM_NCUAHDRAWCAPTION"); break;
		case 0xAF: /*WM_NCUAHDRAWFRAME*/
			lstrcpy(szMsg, L"WM_NCUAHDRAWFRAME"); break;
		case 0x31E: /*WM_DWMCOMPOSITIONCHANGED*/
			lstrcpy(szMsg, L"WM_DWMCOMPOSITIONCHANGED"); break;
		case WM_WINDOWPOSCHANGED:
			lstrcpy(szMsg, L"WM_WINDOWPOSCHANGED"); break;
		case WM_SYSCOMMAND:
			lstrcpy(szMsg, L"WM_SYSCOMMAND"); break;
		case WM_GETTEXT:
			lstrcpy(szMsg, L"WM_GETTEXT"); break;
		case WM_PRINT:
			lstrcpy(szMsg, L"WM_PRINT"); break;
		case WM_PRINTCLIENT:
			lstrcpy(szMsg, L"WM_PRINTCLIENT"); break;
		case WM_GETMINMAXINFO:
			lstrcpy(szMsg, L"WM_GETMINMAXINFO"); break;
		case WM_WINDOWPOSCHANGING:
			lstrcpy(szMsg, L"WM_WINDOWPOSCHANGING"); break;
		case WM_MOVE:
			lstrcpy(szMsg, L"WM_MOVE");
			wsprintf(szInfo, L"{%ix%i}", (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));
			break;
		case WM_SIZE:
			lstrcpy(szMsg, L"WM_SIZE");
			wsprintf(szInfo, L"%s {%ix%i}",
				(wParam==SIZE_MAXHIDE) ? L"SIZE_MAXHIDE" :
				(wParam==SIZE_MAXIMIZED) ? L"SIZE_MAXIMIZED" :
				(wParam==SIZE_MAXSHOW) ? L"SIZE_MAXSHOW" :
				(wParam==SIZE_MINIMIZED) ? L"SIZE_MINIMIZED" :
				(wParam==SIZE_RESTORED) ? L"SIZE_RESTORED" : L"???",
				(int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));
			break;
		default:
			wsprintf(szMsg, L"%u=x%X", uMsg, uMsg);
		}

		wsprintf(szDbg, L"MsgInAnimage(%s, %u, %u%s%s)\n", szMsg, (DWORD)wParam, (DWORD)lParam,
			szInfo[0] ? L" - " : L"", szInfo);
		OutputDebugString(szDbg);
	}
#endif

	bool lbRc;
	static POINT ptLastNcClick = {};

	switch (uMsg)
	{
	case WM_ERASEBKGND:
		DBGFUNCTION(L"WM_ERASEBKGND \n");
		lResult = TRUE;
		return true;

	case WM_PAINT:
		DBGFUNCTION(L"WM_PAINT \n");
		lResult = OnPaint(hWnd, NULL/*use BeginPaint,EndPaint*/, WM_PAINT);
		return true;

	case WM_NCPAINT:
	{
		DBGFUNCTION(L"WM_NCPAINT \n");
		if (gpSetCls->isAdvLogging >= 2) LogString(L"WM_NCPAINT - begin");
		MSetter inNcPaint(&mn_InNcPaint);
		lResult = OnNcPaint(hWnd, uMsg, wParam, lParam);
		if (gpSetCls->isAdvLogging >= 2) LogString(L"WM_NCPAINT - end");
		return true;
	}

	case WM_NCACTIVATE:
		DBGFUNCTION(L"WM_NCACTIVATE \n");
		lResult = OnNcActivate(hWnd, uMsg, wParam, lParam);
		return true;

	case WM_NCCALCSIZE:
		DBGFUNCTION(L"WM_NCCALCSIZE \n");
		lResult = OnNcCalcSize(hWnd, uMsg, wParam, lParam);
		return true;

	case WM_NCHITTEST:
		DBGFUNCTION(L"WM_NCHITTEST \n");
		lResult = OnNcHitTest(hWnd, uMsg, wParam, lParam);
		return true;

	case WM_NCLBUTTONDOWN:
	case WM_NCLBUTTONUP:
	case WM_NCRBUTTONDOWN:
	case WM_NCRBUTTONUP:
	case WM_NCMBUTTONDOWN:
	case WM_NCMBUTTONUP:
	case WM_NCMOUSEMOVE:
	case WM_NCMOUSELEAVE:
	case WM_NCMOUSEHOVER:
		#ifdef _DEBUG
		switch (uMsg)
		{
		case WM_NCLBUTTONDOWN:
			DBGFUNCTION(L"WM_NCLBUTTONDOWN \n"); break;
		case WM_NCLBUTTONUP:
			DBGFUNCTION(L"WM_NCLBUTTONUP \n"); break;
		case WM_NCRBUTTONDOWN:
			DBGFUNCTION(L"WM_NCRBUTTONDOWN \n"); break;
		case WM_NCRBUTTONUP:
			DBGFUNCTION(L"WM_NCRBUTTONUP \n"); break;
		case WM_NCMBUTTONDOWN:
			DBGFUNCTION(L"WM_NCMBUTTONDOWN \n"); break;
		case WM_NCMBUTTONUP:
			DBGFUNCTION(L"WM_NCMBUTTONUP \n"); break;
		case WM_NCMOUSEMOVE:
			DBGFUNCTION(L"WM_NCMOUSEMOVE \n"); break;
		case WM_NCMOUSELEAVE:
			DBGFUNCTION(L"WM_NCMOUSELEAVE \n"); break;
		case WM_NCMOUSEHOVER:
			DBGFUNCTION(L"WM_NCMOUSEHOVER \n"); break;
		}
		#endif

		ptLastNcClick = MakePoint(LOWORD(lParam),HIWORD(lParam));

		if ((uMsg == WM_NCMOUSEMOVE) || (uMsg == WM_NCLBUTTONUP))
			gpConEmu->isSizing(uMsg); // могло не сброситься, проверим

		if (gpSet->isTabsInCaption)
		{
			//RedrawLock();
			lbRc = gpConEmu->mp_TabBar->ProcessNcTabMouseEvent(hWnd, uMsg, wParam, lParam, lResult);
			//RedrawUnlock();
		}
		else
		{
			// Табов чисто в заголовке - нет
			lbRc = false;
		}

		if (!lbRc)
		{
			if ((wParam == HTSYSMENU) && (uMsg == WM_NCLBUTTONDOWN))
			{
				gpConEmu->mp_Menu->OnNcIconLClick();
				lResult = 0;
				lbRc = true;
			}
			else if ((wParam == HTSYSMENU || wParam == HTCAPTION)
				&& (uMsg == WM_NCRBUTTONDOWN || uMsg == WM_NCRBUTTONUP))
			{
				if (uMsg == WM_NCRBUTTONUP)
				{
					LogString(L"ShowSysmenu called from (WM_NCRBUTTONUP)");
					gpConEmu->mp_Menu->ShowSysmenu((short)LOWORD(lParam), (short)HIWORD(lParam));
				}
				lResult = 0;
				lbRc = true;
			}
			else if ((uMsg == WM_NCRBUTTONDOWN || uMsg == WM_NCRBUTTONUP)
				&& (wParam == HTCLOSE || wParam == HTMINBUTTON || wParam == HTMAXBUTTON))
			{
				switch (LOWORD(wParam))
				{
				case HTMINBUTTON:
				case HTCLOSE:
					Icon.HideWindowToTray();
					break;
				case HTMAXBUTTON:
					gpConEmu->DoFullScreen();
					break;
				}
				lResult = 0;
				lbRc = true;
			}
		}
		return lbRc;

	//case WM_LBUTTONDBLCLK:
	//	{
	//		// Глюк? DblClick по иконке приводит к WM_LBUTTONDBLCLK вместо WM_NCLBUTTONDBLCLK
	//		POINT pt = MakePoint(LOWORD(lParam),HIWORD(lParam));
	//		if (gpConEmu->PtDiffTest(pt, ptLastNcClick.x, ptLastNcClick.y, 4))
	//		{
	//			PostScClose();
	//			lResult = 0;
	//			return true;
	//		}
	//	}
	//	return false;

	case WM_MOUSEMOVE:
		DBGFUNCTION(L"WM_MOUSEMOVE \n");
		// Табов чисто в заголовке - нет
		#if 0
		RedrawLock();
		if (gpConEmu->mp_TabBar->GetHoverTab() != -1)
		{
			gpConEmu->mp_TabBar->HoverTab(-1);
		}
		#if defined(USE_CONEMU_TOOLBAR)
		// Ну и с кнопок убрать подсветку, если была
		gpConEmu->mp_TabBar->Toolbar_UnHover();
		#endif
		RedrawUnlock();
		#endif
		return false;

	case WM_NCLBUTTONDBLCLK:
		if (wParam == HTCAPTION)
		{
			mb_NcAnimate = TRUE;
		}

		if (wParam == HT_CONEMUTAB)
		{
			_ASSERTE(gpSet->isTabsInCaption && "There is not tabs in 'Caption'");
			//RedrawLock(); -- чтобы отрисовать "клик" по кнопке
			lbRc = gpConEmu->mp_TabBar->ProcessNcTabMouseEvent(hWnd, uMsg, wParam, lParam, lResult);
			//RedrawUnlock();
		}
		else if (gpConEmu->OnMouse_NCBtnDblClk(hWnd, uMsg, wParam, lParam))
		{
			lResult = 0; // DblClick на рамке - ресайз по ширине/высоте рабочей области экрана
		}
		else
		{
			lResult = DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		if (wParam == HTCAPTION)
		{
			mb_NcAnimate = FALSE;
		}
		return true;

	case WM_KEYDOWN:
	case WM_KEYUP:
		if (gpSet->isTabSelf && (wParam == VK_TAB || gpConEmu->mp_TabBar->IsInSwitch()))
		{
			if (isPressed(VK_CONTROL) && !isPressed(VK_MENU) && !isPressed(VK_LWIN) && !isPressed(VK_RWIN))
			{
				if (gpConEmu->mp_TabBar->ProcessTabKeyboardEvent(hWnd, uMsg, wParam, lParam, lResult))
				{
					return true;
				}
			}
		}
		return false;

	//case WM_NCCREATE: gpConEmu->CheckGlassAttribute(); return false;

	case 0xAE: /*WM_NCUAHDRAWCAPTION*/
	{
		if (gpSetCls->isAdvLogging >= 2) LogString(L"WM_NCUAHDRAWCAPTION - begin");
		MSetter inNcPaint(&mn_InNcPaint);
		lResult = OnDwmMessage(hWnd, uMsg, wParam, lParam);
		if (gpSetCls->isAdvLogging >= 2) LogString(L"WM_NCUAHDRAWCAPTION - end");
		return true;
	}
	case 0xAF: /*WM_NCUAHDRAWFRAME*/
	{
		if (gpSetCls->isAdvLogging >= 2) LogString(L"WM_NCUAHDRAWFRAME - begin");
		MSetter inNcPaint(&mn_InNcPaint);
		lResult = OnDwmMessage(hWnd, uMsg, wParam, lParam);
		if (gpSetCls->isAdvLogging >= 2) LogString(L"WM_NCUAHDRAWFRAME - end");
		return true;
	}
	case 0x31E: /*WM_DWMCOMPOSITIONCHANGED*/
	{
		if (gpSetCls->isAdvLogging >= 2) LogString(L"WM_DWMCOMPOSITIONCHANGED - begin");
		lResult = OnDwmMessage(hWnd, uMsg, wParam, lParam);
		if (gpSetCls->isAdvLogging >= 2) LogString(L"WM_DWMCOMPOSITIONCHANGED - end");
		return true;
	}

	case WM_SYSCOMMAND:
		if (wParam == SC_MAXIMIZE || wParam == SC_MINIMIZE || wParam == SC_RESTORE)
		{
			// Win 10 build 9926 bug?
			if ((wParam == SC_MAXIMIZE) && IsWin10())
			{
				if (!gpConEmu->isMeForeground(false,false))
				{
					return true;
				}
			}
			mb_NcAnimate = TRUE;
			//GetWindowText(hWnd, ms_LastCaption, countof(ms_LastCaption));
			//SetWindowText(hWnd, L"");
		}
		lResult = gpConEmu->mp_Menu->OnSysCommand(hWnd, wParam, lParam, WM_SYSCOMMAND);
		if (wParam == SC_MAXIMIZE || wParam == SC_MINIMIZE || wParam == SC_RESTORE)
		{
			mb_NcAnimate = FALSE;
			//SetWindowText(hWnd, ms_LastCaption);
		}
		return true;

	case WM_GETTEXT:
		//TODO: Во время анимации Maximize/Restore/Minimize заголовок отрисовывается
		//TODO: системой, в итоге мелькает текст и срезаются табы
		//TODO: Сделаем, пока, чтобы текст хотя бы не мелькал...
		if (mb_NcAnimate && gpSet->isTabsInCaption)
		{
			_ASSERTE(!IsWindows7); // Проверить на XP и ниже
			if (wParam && lParam)
			{
				*(wchar_t*)lParam = 0;
			}
			lResult = 0;
			return true;
		}
		break;

	default:
		break;
	}

	return false;
}

LRESULT CFrameHolder::OnDwmMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
#if 0
	if (gpSet->isTabsInCaption)
	{
		FrameDrawStyle dt = gpConEmu->DrawType();
		switch (uMsg)
		{
		case 0x31E: // WM_DWMCOMPOSITIONCHANGED:
			gpConEmu->CheckGlassAttribute();
			return 0; // ??? позвать DefWindowProc?

			//0xAE посылается при необходимости перерисовки заголовка окна, например
			//после вызова SetWindowText(ghWnd, ...).
			//Как минимум, вызывается при включенных темах (WinXP).
		case 0xAE: // WM_NCUAHDRAWCAPTION:
		case 0xAF: // WM_NCUAHDRAWFRAME:
			return (dt==fdt_Aero || dt==fdt_Win8) ? DefWindowProc(hWnd, uMsg, wParam, lParam) : 0;
		}
	}
#endif

	// Unknown!
	_ASSERTE(uMsg == 0x31E || uMsg == 0xAE || uMsg == 0xAF);
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void CFrameHolder::NC_Redraw()
{
	mb_RedrawRequested = false;

	//TODO: При потере фокуса (клик мышкой по другому окну) лезут глюки отрисовки этого нового окна
	SetWindowPos(ghWnd, 0, 0, 0, 0, 0,
		SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_DRAWFRAME);

	//SetWindowPos(ghWnd, NULL, 0, 0, 0, 0,
	//		SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	//RedrawWindow(ghWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
	//RedrawWindow(ghWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME);

	// В Aero отрисовка идет как бы на клиентской части
	if (gpConEmu->DrawType() == fdt_Aero)
	{
		gpConEmu->Invalidate(NULL, FALSE);
	}
}

void CFrameHolder::RedrawLock()
{
	mn_RedrawLockCount++;
}

void CFrameHolder::RedrawUnlock()
{
	mn_RedrawLockCount = 0;
	if (mb_RedrawRequested)
	{
		NC_Redraw();
	}
}

void CFrameHolder::RedrawFrame()
{
	//TODO: табы/кнопки могут быть в клиентской области!
	if (mn_RedrawLockCount > 0)
	{
		mb_RedrawRequested = true;
	}
	else
	{
		NC_Redraw();
	}
}

//
//	This is a generic message handler used by WM_SETTEXT and WM_NCACTIVATE.
//	It works by turning off the WS_VISIBLE style, calling
//	the original window procedure, then turning WS_VISIBLE back on.
//
//	This prevents the original wndproc from redrawing the caption.
//	Last of all, we paint the caption ourselves with the inserted buttons
//
LRESULT CFrameHolder::NC_Wrapper(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UINT ret;
	DWORD dwStyle;

	dwStyle = GetWindowLong(hWnd, GWL_STYLE);

	//Turn OFF WS_VISIBLE, so that WM_NCACTIVATE does not
	//paint our window caption...
	gpConEmu->SetWindowStyle(hWnd, dwStyle & ~WS_VISIBLE);

	//Do the default thing..
	ret = DefWindowProc(hWnd, uMsg, wParam, lParam);

	//Restore the original style
	gpConEmu->SetWindowStyle(hWnd, dwStyle);

	//Paint the whole window frame + caption
	//Caption_NCPaint(ctp, hWnd, (HRGN)1);
	//if (msg == WM_NCPAINT)
	//{
	//	RECT tr; GetWindowRect(hWnd, &tr);
	//	OffsetRect(&tr, -tr.left, -tr.top);
	//
	//	tr.right -= 120; //TODO:
	//	tr.left += 26; //TODO:
	//	if (IsZoomed(hWnd))
	//		tr.top += GetSystemMetrics(SM_CYFRAME)+1;
	//	else
	//		tr.top += 3;
	//	tr.bottom = tr.top+GetSystemMetrics(SM_CYFRAME)+GetSystemMetrics(SM_CYCAPTION);
	//
	//	HDC hdc = GetWindowDC(hWnd);
	//	mp_Tabs->PaintHeader(hdc, tr);
	//	ReleaseDC(hWnd, hdc);
	//}
	return ret;
}

#if 0
void CFrameHolder::PaintFrame2k(HWND hWnd, HDC hdc, RECT &cr)
{
	//RECT cr;
	//GetWindowRect(hWnd, &cr);
	//OffsetRect(&cr, -cr.left, -cr.top);

	//TODO: Обработка XP
	HPEN hOldPen = (HPEN)SelectObject(hdc, (HPEN)GetStockObject(BLACK_PEN));
	HBRUSH hOldBr = (HBRUSH)SelectObject(hdc, (HBRUSH)GetStockObject(BLACK_BRUSH));

	HPEN hPenHilight = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));
	HPEN hPenLight = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DLIGHT));
	HPEN hPenShadow = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
	HPEN hPenDkShadow = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DDKSHADOW));
	HPEN hPenFace = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DFACE));
	//TODO: разобраться с шириной рамки, активной/неактивной
	HPEN hPenBorder = CreatePen(PS_SOLID, 1, GetSysColor(mb_NcActive ? COLOR_ACTIVEBORDER : COLOR_INACTIVEBORDER));
	//TODO: градиент
	HBRUSH hBrCaption = GetSysColorBrush(mb_NcActive ? COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION);
	//
	int nCaptionY = GetSystemMetrics(SM_CYCAPTION);

	SelectObject(hdc, hPenLight);
	MoveToEx(hdc, cr.left, cr.bottom-1, NULL); LineTo(hdc, cr.left, 0); LineTo(hdc, cr.right-1, 0);
	SelectObject(hdc, hPenHilight);
	MoveToEx(hdc, cr.left+1, cr.bottom-2, NULL); LineTo(hdc, cr.left+1, 1); LineTo(hdc, cr.right-2, 1);
	SelectObject(hdc, hPenDkShadow);
	MoveToEx(hdc, cr.left, cr.bottom-1, NULL); LineTo(hdc, cr.right-1, cr.bottom-1); LineTo(hdc, cr.right-1, -1);
	SelectObject(hdc, hPenShadow);
	MoveToEx(hdc, cr.left+1, cr.bottom-2, NULL); LineTo(hdc, cr.right-2, cr.bottom-2); LineTo(hdc, cr.right-2, 0);
	// рамка. обычно это 1 пиксел цвета кнопки
	SelectObject(hdc, hPenBorder); //TODO: но может быть и более одного пиксела
	Rectangle(hdc, cr.left+2, cr.top+2, cr.right-2, cr.bottom-2);

	//TODO: Заливка заголовка через GradientFill
	SelectObject(hdc, hPenFace); SelectObject(hdc, hBrCaption);
	Rectangle(hdc, cr.left+3, cr.top+3, cr.right-3, cr.top+nCaptionY+4);
	//что осталось
	SelectObject(hdc, hPenFace);
	//Rectangle(hdc, cr.left+3, cr.top+3+nCaptionY, cr.right-3, cr.bottom-3);
	MoveToEx(hdc, cr.left+3, cr.top+3+nCaptionY, 0);
	LineTo(hdc, cr.left+3, cr.bottom-3);
	LineTo(hdc, cr.right-3, cr.bottom-3);
	LineTo(hdc, cr.right-3, cr.top+3+nCaptionY);


	SelectObject(hdc, hOldPen);
	SelectObject(hdc, hOldBr);

	DeleteObject(hPenHilight); DeleteObject(hPenLight); DeleteObject(hPenShadow); DeleteObject(hPenDkShadow);
	DeleteObject(hPenFace); DeleteObject(hPenBorder);

	cr.left += 4; cr.right -= 4; cr.bottom -= 4; cr.top += 4+nCaptionY;
}
#endif

LRESULT CFrameHolder::OnPaint(HWND hWnd, HDC hdc, UINT uMsg)
{
	if (hdc == NULL)
	{
		LRESULT lRc = 0;
		PAINTSTRUCT ps = {0};
		hdc = BeginPaint(hWnd, &ps);

		if (hdc != NULL)
		{
			lRc = OnPaint(hWnd, hdc, uMsg);

			EndPaint(hWnd, &ps);
		}
		else
		{
			_ASSERTE(hdc != NULL);
		}

		return lRc;
	}

	#ifdef _DEBUG
	RECT rcClientReal = {}; GetClientRect(hWnd, &rcClientReal);
	MapWindowPoints(hWnd, NULL, (LPPOINT)&rcClientReal, 2);
	#endif

	// Если "завис" PostUpdate
	if (gpConEmu->mp_TabBar->NeedPostUpdate())
		gpConEmu->mp_TabBar->Update();

	// Go

	RECT wr, cr;

	RecalculateFrameSizes();

	wr = gpConEmu->GetGuiClientRect();

	#ifdef _DEBUG
	wchar_t szPaint[140];
	_wsprintf(szPaint, SKIPCOUNT(szPaint) L"MainClient %s at {%i,%i}-{%i,%i} screen coords, size (%ix%i) calc (%ix%i)",
		(uMsg == WM_PAINT) ? L"WM_PAINT" : (uMsg == WM_PRINTCLIENT) ? L"WM_PRINTCLIENT" : L"UnknownMsg",
		LOGRECTCOORDS(rcClientReal), LOGRECTSIZE(rcClientReal), LOGRECTSIZE(wr));
	DEBUGSTRPAINT(szPaint);
	#endif

#if defined(CONEMU_TABBAR_EX)
#ifdef RED_CLIENT_FILL
	HBRUSH h = CreateSolidBrush(RGB(255,0,0));
	FillRect(hdc, &wr, h);
	DeleteObject(h);
	return 0;
#endif
#endif

	if (gpSet->isStatusBarShow)
	{
		int nHeight = gpSet->StatusBarHeight();
		if (nHeight < (wr.bottom - wr.top))
		{
			RECT rcStatus = {wr.left, wr.bottom - nHeight, wr.right, wr.bottom};
			gpConEmu->mp_Status->PaintStatus(hdc, &rcStatus);
			wr.bottom = rcStatus.top;
		}
	}

	cr = wr;

	DEBUGTEST(FrameDrawStyle dt = gpConEmu->DrawType());


#if defined(CONEMU_TABBAR_EX)
	RECT tr = {};

	if (!gpSet->isTabsInCaption)
	{
		_ASSERTE(gpConEmu->GetDwmClientRectTopOffset() == 0); // CheckIt, must be zero

		if (gpSet->isTabs)
		{
			RECT captrect = gpConEmu->CalcRect(CER_TAB, wr, CER_MAINCLIENT);
			//CalculateCaptionPosition(cr, &captrect);
			CalculateTabPosition(cr, captrect, &tr);

			PaintDC dc = {false};
			RECT pr = {captrect.left, 0, captrect.right, captrect.bottom};
			gpConEmu->BeginBufferedPaint(hdc, pr, dc);

			gpConEmu->mp_TabBar->PaintTabs(dc, captrect, tr);

			gpConEmu->EndBufferedPaint(dc, TRUE);
		}

	}
	else if (dt == fdt_Aero || dt == fdt_Win8)
	{
		_ASSERTE(gpSet->isTabsInCaption);

		int nOffset = gpConEmu->GetDwmClientRectTopOffset();
		// "Рамка" расширена на клиентскую область, поэтому
		// нужно зарисовать заголовок черной кистью, иначе идет
		// искажение цвета для кнопок Min/Max/Close

		if (gpSet->isTabs)
		{
			RECT captrect = gpConEmu->CalcRect(CER_TAB, wr, CER_MAINCLIENT);
			//CalculateCaptionPosition(cr, &captrect);
			CalculateTabPosition(cr, captrect, &tr);

			PaintDC dc = {false};
			RECT pr = {captrect.left, 0, captrect.right, captrect.bottom};
			gpConEmu->BeginBufferedPaint(hdc, pr, dc);

			gpConEmu->mp_TabBar->PaintTabs(dc, captrect, tr);

			gpConEmu->EndBufferedPaint(dc, TRUE);

			// There is no "Glass" in Win8
			mb_WasGlassDraw = IsWindows7 && !IsWindows8;
		}

		cr.top += nOffset;
	}
#endif

	#ifdef _DEBUG
	int nWidth = (cr.right-cr.left);
	int nHeight = (cr.bottom-cr.top);
	#endif

	WARNING("Пока табы рисуем не сами и ExtendDWM отсутствует - дополнительные изыски с временным DC не нужны");
#if 0
	if (!gpSet->isTabsInCaption)
	{
		//OnPaintClient(hdc/*, nWidth, nHeight*/);
	}
	else
	// Создадим временный DC, для удобства отрисовки в Glass-режиме и для фикса глюка DWM(?) см.ниже
	// В принципе, для режима Win2k/XP временный DC можно не создавать, если это будет тормозить
	{
		//_ASSERTE(FALSE && "Need to be rewritten");

		HDC hdcPaint = CreateCompatibleDC(hdc);
		HBITMAP hbmp = CreateCompatibleBitmap(hdc, nWidth, nHeight);
		HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcPaint, hbmp);

		//OnPaintClient(hdcPaint/*, nWidth, nHeight*/);

		if ((dt == fdt_Aero) || !(mb_WasGlassDraw && gpConEmu->isZoomed()))
		{
			BitBlt(hdc, cr.left, cr.top, nWidth, nHeight, hdcPaint, 0, 0, SRCCOPY);
		}
		else
		{
			//mb_WasGlassDraw = FALSE;
			// Какой-то странный глюк DWM. При отключении Glass несколько верхних строк
			// клиентской области оказываются "разрушенными" - у них остается атрибут "прозрачности"
			// хотя прозрачность (Glass) уже отключена. В результате эти строки - белесые

			BITMAPINFOHEADER bi = {sizeof(BITMAPINFOHEADER)};
			bi.biWidth = cr.right-cr.left+1;
			bi.biHeight = GetFrameHeight()+1;
			bi.biPlanes = 1;
			bi.biBitCount = 32;
			COLORREF *pPixels = NULL;
			HDC hdcTmp = CreateCompatibleDC(hdc);
			HBITMAP hTmp = CreateDIBSection(hdcTmp, (BITMAPINFO*)&bi, DIB_RGB_COLORS, (void**)&pPixels, NULL, 0);
			if (hTmp == NULL)
			{
				_ASSERTE(hTmp == NULL);
				BitBlt(hdc, cr.left, cr.top, nWidth, nHeight, hdcPaint, 0, 0, SRCCOPY);
			}
			else
			{
				HBITMAP hOldTmp = (HBITMAP)SelectObject(hdcTmp, hTmp);

				BitBlt(hdcTmp, 0, 0, bi.biWidth, bi.biHeight, hdcPaint, 0, 0, SRCCOPY);

				int i = 0;
				for (int y = 0; y < bi.biHeight; y++)
				{
					for (int x = 0; x < bi.biWidth; x++)
					{
						pPixels[i++] |= 0xFF000000;
					}
				}

				BitBlt(hdc, cr.left, cr.top, bi.biWidth, bi.biHeight, hdcTmp, 0, 0, SRCCOPY);
				if (nHeight > bi.biHeight)
					BitBlt(hdc, cr.left, cr.top+bi.biHeight, nWidth, nHeight-bi.biHeight, hdcPaint, 0, bi.biHeight, SRCCOPY);

				SelectObject(hdcTmp, hOldTmp);
				DeleteObject(hbmp);
			}
			DeleteDC(hdcTmp);
		}

		SelectObject(hdcPaint, hOldBmp);
		DeleteObject(hbmp);
		DeleteDC(hdcPaint);
	}
#endif

	return 0;
}

void CFrameHolder::CalculateCaptionPosition(const RECT &rcWindow, RECT* rcCaption)
{
	//_ASSERTE(FALSE && "CFrameHolder::CalculateCaptionPosition MUST be refactored!");

	FrameDrawStyle dt = gpConEmu->DrawType();

	if (dt == fdt_Aero || dt == fdt_Win8)
	{
		// Почему тут не SM_CXFRAME не помню
		rcCaption->left = 0; //2; //GetSystemMetrics(SM_CXFRAME);
		rcCaption->right = (rcWindow.right - rcWindow.left); // - 4; //GetSystemMetrics(SM_CXFRAME);
		rcCaption->top = GetFrameHeight(); //6; //GetSystemMetrics(SM_CYFRAME);
		if (gpSet->isTabsInCaption)
			rcCaption->bottom = rcCaption->top + GetCaptionHeight()/*это наш*/; // (gpSet->isTabs ? (GetCaptionDragHeight()+GetTabsHeight()) : GetWinCaptionHeight()); //gpConEmu->GetDwmClientRectTopOffset() - 1;
		else
			rcCaption->bottom = rcCaption->top + GetCaptionHeight()/*это наш*/; // (gpSet->isTabs ? (GetCaptionDragHeight()+GetTabsHeight()) : GetWinCaptionHeight()); //gpConEmu->GetDwmClientRectTopOffset() - 1;
	}
	else if (dt == fdt_Themed)
	{
		rcCaption->left = GetFrameWidth();
		rcCaption->right = (rcWindow.right - rcWindow.left) - GetFrameWidth() - 1;
		rcCaption->top = GetFrameHeight();
		//rcCaption->bottom = rcCaption->top + GetSystemMetrics(SM_CYCAPTION) + (gSet.isTabs ? (GetSystemMetrics(SM_CYCAPTION)/2) : 0);
		rcCaption->bottom = rcCaption->top + GetCaptionHeight()/*это наш*/; // (gpSet->isTabs ? (GetCaptionDragHeight()+GetTabsHeight()) : GetWinCaptionHeight());
	}
	else
	{
		rcCaption->left = GetFrameWidth();
		rcCaption->right = (rcWindow.right - rcWindow.left) - GetFrameWidth() - 1;
		rcCaption->top = GetFrameHeight();
		//rcCaption->bottom = rcCaption->top + GetSystemMetrics(SM_CYCAPTION) - 1 + (gpSet->isTabs ? (GetSystemMetrics(SM_CYCAPTION)/2) : 0);
		rcCaption->bottom = rcCaption->top + GetCaptionHeight()/*это наш*/; // (gpSet->isTabs ? (GetCaptionDragHeight()+GetTabsHeight()) : GetWinCaptionHeight());
	}
}

void CFrameHolder::CalculateTabPosition(const RECT &rcWindow, const RECT &rcCaption, RECT* rcTabs)
{
	int nBtnWidth = GetSystemMetrics(SM_CXSIZE);

	bool bCaptionHidden = gpSet->isCaptionHidden();

	FrameDrawStyle fdt = gpConEmu->DrawType();

	rcTabs->left = rcCaption.left;
	if (bCaptionHidden || gpSet->isTabsInCaption)
		rcTabs->left += GetSystemMetrics(SM_CXSMICON) + mn_FrameWidth;

	if (gpSet->nTabsLocation == 0)
	{
		// Tabs on Top
		if (gpSet->isTabsInCaption && (fdt >= fdt_Aero))
			rcTabs->bottom = rcCaption.bottom - 2;
		else if (fdt == fdt_Aero)
			rcTabs->bottom = rcCaption.bottom - 7;
		else
			rcTabs->bottom = rcCaption.bottom - 1;
		rcTabs->top = max((rcCaption.bottom - GetTabsHeight()/*nHeightIdeal*/), rcCaption.top);
	}
	else
	{
		// Tabs on Bottom
		rcTabs->top = rcCaption.top;
		rcTabs->bottom = rcCaption.bottom - 1;// min(rcCaption.bottom, (rcCaption.top + GetTabsHeight()));
	}

	if (!bCaptionHidden)
	{
		rcTabs->right = rcCaption.right - 1;
	}
	// TODO: определить ширину кнопок + Shift из настроек юзера
	else if (gpConEmu->DrawType() == fdt_Aero)
	{
		RECT rcButtons = {0};
		HRESULT hr = gpConEmu->DwmGetWindowAttribute(ghWnd, DWMWA_CAPTION_BUTTON_BOUNDS, &rcButtons, sizeof(rcButtons));

		if (SUCCEEDED(hr))
			rcTabs->right = min((rcButtons.left - 4),(rcCaption.right - 3*nBtnWidth)) - 1;
		else
			rcTabs->right = rcCaption.right - 3*nBtnWidth - 1;
	}
	else if (gpConEmu->DrawType() == fdt_Themed)
	{
		// -- Не работает :(
		//HTHEME hTheme = gpConEmu->OpenThemeData(NULL, L"WINDOW");
		//RECT rcBtns, wr; GetWindowRect(ghWnd, &wr); OffsetRect(&wr, -wr.left, -wr.top);
		//HRESULT hr = gpConEmu->GetThemeBackgroundContentRect(hTheme, NULL, 15/*WP_MINBUTTON*/, 1/*MINBS_NORMAL*/, &wr, &rcBtns);
		//gpConEmu->CloseThemeData(hTheme);

		rcTabs->right = rcCaption.right - 3*nBtnWidth - 1;
	}
	else
	{
		rcTabs->right = rcCaption.right - 3*nBtnWidth /*+ 2*/ - 1;
	}
}

bool CFrameHolder::isInNcPaint()
{
	_ASSERTE(mn_InNcPaint>=0);
	return (mn_InNcPaint > 0);
}

LRESULT CFrameHolder::OnNcPaint(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRc = 0;
	FrameDrawStyle fdt;
	RECT dirty_box, dirty, wr = {}, tr = {}, cr = {}, xorRect;
	BOOL fRegionOwner = FALSE;
	HDC hdc;
	HRGN hrgn = (HRGN)wParam;
	PaintDC dc = {};

	RecalculateFrameSizes();

	if (!gpSet->isTabsInCaption)
	{
		lRc = DefWindowProc(hWnd, uMsg, wParam, lParam);
		goto wrap;
	}

	fdt = gpConEmu->DrawType();

	if (fdt == fdt_Aero || fdt == fdt_Win8)
	{
		lRc = ::DefWindowProc(hWnd, uMsg, wParam, lParam);
		//TODO: Может быть на "стекле" сразу рисовать, а не в WM_PAINT?
		goto wrap;
	}

	if (!gpSet->isTabs)
	{
		lRc = ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}


	GetWindowRect(hWnd, &wr);
	CalculateCaptionPosition(wr, &cr);
	CalculateTabPosition(wr, cr, &tr);

	// --- Регион формируем всегда сами, иначе в некоторых случаях (XP+ Theming)
	// --- при выезжании окна из-за пределов экрана (обратно в видимую область)
	// --- сильно мелькает текст в заголовке
	//Create a region which covers the whole window. This
	//must be in screen coordinates
	//if(TRUE || hrgn == (HRGN)1 || hrgn == NULL)
	{
		hrgn = CreateRectRgnIndirect(&wr);
		dirty_box = wr;
		fRegionOwner = TRUE;
		//#ifdef _DEBUG
		//	wchar_t szDbg[255];
		//	wsprintf(szDbg, L"CFrameHolder::OnNcPaint - New region({%ix%i}-{%ix%i})\n", wr.left,wr.top,wr.right,wr.bottom);
		//	OutputDebugStringW(szDbg);
		//#endif
	}
	//else
	//{
	//	GetRgnBox((HRGN)wParam, &dirty_box);
	//	#ifdef _DEBUG
	//		wchar_t szDbg[255];
	//		wsprintf(szDbg, L"CFrameHolder::OnNcPaint - Existing region({%ix%i}-{%ix%i})\n", dirty_box.left,dirty_box.top,dirty_box.right,dirty_box.bottom);
	//		OutputDebugStringW(szDbg);
	//	#endif
	//}



	xorRect = tr;
	xorRect.top = cr.top;
	if (gpConEmu->DrawType() == fdt_Aero)
	{
	}
	else if (gpConEmu->DrawType() == fdt_Themed)
	{
		xorRect.left = cr.left;
		xorRect.right = cr.right;
	}
	else
	{
		xorRect.left = cr.left;
		//xorRect.right = cr.right;
	}
	OffsetRect(&xorRect, wr.left, wr.top);


	if (IntersectRect(&dirty, &dirty_box, &xorRect))
	{
		// This must be in screen coordinates
		HRGN hrgn1 = CreateRectRgnIndirect(&xorRect);

		//Cut out a button-shaped hole
		CombineRgn(hrgn, hrgn, hrgn1, RGN_XOR);

		DeleteObject(hrgn1);
	}
	//#ifdef _DEBUG
	//else
	//{
	//	OutputDebugStringW(L"CFrameHolder::OnNcPaint --- IntersectRect failed\n");
	//}
	//#endif

	//if (!mb_NcAnimate)
	DefWindowProc(hWnd, uMsg, (WPARAM)hrgn, lParam);


//#ifdef _DEBUG
//	Sleep(150);
//#endif

	// Собственно отрисовка табов
	hdc = GetWindowDC(hWnd);

	//HRGN hdcrgn = CreateRectRgn(cr.left, cr.top, cr.right, tr.bottom);
	//hdc = GetDCEx(hWnd, hdcrgn, DCX_INTERSECTRGN);

	gpConEmu->BeginBufferedPaint(hdc, cr, dc);

	gpConEmu->mp_TabBar->PaintTabs(dc, cr, tr);

	gpConEmu->EndBufferedPaint(dc, TRUE);

	//if (mb_WasGlassDraw && gpConEmu->isZoomed())
	//{
	//	//mb_WasGlassDraw = FALSE;
	//	// Какой-то странный глюк DWM. При отключении Glass несколько верхних строк
	//	// клиентской области оказываются "разрушенными" - у них остается атрибут "прозрачности"
	//	// хотя прозрачность (Glass) уже отключена. В результате эти строки - белесые

	//	BITMAPINFOHEADER bi = {sizeof(BITMAPINFOHEADER)};
	//	bi.biWidth = cr.right-cr.left+1;
	//	bi.biHeight = GetFrameHeight()+1;
	//	bi.biPlanes = 1;
	//	bi.biBitCount = 32;
	//	COLORREF *pPixels = NULL;
	//	HDC hdcPaint = CreateCompatibleDC(hdc);
	//	HBITMAP hbmp = CreateDIBSection(hdcPaint, (BITMAPINFO*)&bi, DIB_RGB_COLORS, (void**)&pPixels, NULL, 0);
	//	HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcPaint, hbmp);

	//	//memset(pPixels, 0xFF, bi.biWidth*bi.biHeight*4);
	//	int i = 0;
	//	for (int y = 0; y < bi.biHeight; y++)
	//	{
	//		for (int x = 0; x < bi.biWidth; x++)
	//		{
	//			pPixels[i++] = 0xFF000000;
	//		}
	//	}

	//	BitBlt(hdc, cr.left, tr.bottom, bi.biWidth, bi.biHeight, hdcPaint, 0, 0, SRCCOPY);

	//	SelectObject(hdcPaint, hOldBmp);
	//	DeleteObject(hbmp);
	//	DeleteDC(hdcPaint);
	//}

	ReleaseDC(hWnd, hdc);

	if(fRegionOwner)
		DeleteObject(hrgn);

	lRc = 0;
wrap:
	return lRc;
}

LRESULT CFrameHolder::OnNcActivate(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	mb_NcActive = (wParam != 0);

	if (gpSetCls->isAdvLogging)
	{
		wchar_t szInfo[100];
		_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"CFrameHolder::OnNcActivate(%u,x%X)", (DWORD)wParam, (DWORD)(DWORD_PTR)lParam);
		gpConEmu->LogString(szInfo);
	}

	if (gpConEmu->IsGlass() || !gpSet->isTabsInCaption)
		return DefWindowProc(hWnd, uMsg, wParam, lParam);

	//return DefWindowProc(hWnd, uMsg, wParam, lParam);

		// We can get WM_NCACTIVATE before we're actually visible. If we're not
		// visible, no need to paint.
		if (!IsWindowVisible(hWnd))
		{
		return 0;
	}

	// Force paint our non-client area otherwise Windows will paint its own.
	//RedrawWindow(hWnd, NULL, NULL, RDW_UPDATENOW);
	//OnPaint(TRUE);

	LRESULT lRc;
	//if (!mb_NcActive)
	//{
	//	// При потере фокуса (клик мышкой по другому окну) лезут глюки отрисовки этого нового окна
	//	lRc = DefWindowProc(hWnd, uMsg, wParam, lParam);
	//}
	//else
	//{
		lRc = NC_Wrapper(hWnd, uMsg, wParam, lParam);
	//}
	//lRc = DefWindowProc(hWnd, uMsg, wParam, lParam);
	//NC_Redraw();
	OnNcPaint(hWnd, WM_NCPAINT, 1, 0);
	return lRc;
}

bool CFrameHolder::SetDontPreserveClient(bool abSwitch)
{
	bool b = mb_DontPreserveClient;
	mb_DontPreserveClient = abSwitch;
	return b;
}

bool CFrameHolder::SetAllowPreserveClient(bool abSwitch)
{
	bool b = mb_AllowPreserveClient;
	mb_AllowPreserveClient = abSwitch;
	return b;
}

LRESULT CFrameHolder::OnNcCalcSize(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RecalculateFrameSizes();

	LRESULT lRc = 0, lRcDef = 0;

	// В Aero (Glass) важно, чтобы клиентская область начиналась с верхнего края окна,
	// иначе не получится "рисовать по стеклу"
	FrameDrawStyle fdt = gpConEmu->DrawType();

	// Если nCaption == 0, то при fdt_Aero текст в заголовке окна не отрисовывается
	DEBUGTEST(int nCaption = GetCaptionHeight());

	bool bCallDefProc = true;

	#if defined(CONEMU_TABBAR_EX)
	// В режиме Aero/Glass (хоть он и почти выпилен в Win8)
	// мы расширяем клиентскую область на заголовок
	if (gpSet->isTabsInCaption && (fdt == fdt_Aero || fdt == fdt_Win8))
	{
		nCaption = 0;
		bCallDefProc = false;
		// Must be "glassed" or "themed", otherwise system will not draw window caption
		_ASSERTE(gpConEmu->IsGlass() || gpConEmu->IsThemed());
	}
	#endif

	if (wParam)
	{
		// lParam points to an NCCALCSIZE_PARAMS structure that contains information
		// an application can use to calculate the new size and position of the client rectangle.
		// The application should indicate which part of the client area contains valid information!
		NCCALCSIZE_PARAMS* pParm = (NCCALCSIZE_PARAMS*)lParam;
		if (!pParm)
		{
			_ASSERTE(pParm!=NULL);
			return 0;
		}
		// r[0] contains the new coordinates of a window that has been moved or resized,
		//      that is, it is the proposed new window coordinates
		// r[1] contains the coordinates of the window before it was moved or resized
		// r[2] contains the coordinates of the window's client area before the window was moved or resized
		RECT r[3] = {pParm->rgrc[0], pParm->rgrc[1], pParm->rgrc[2]};
		bool bAllowPreserveClient = mb_AllowPreserveClient && (memcmp(r, r+1, sizeof(*r)) == 0);

		// We need to call this, otherwise some parts of window may be broken
		// If don't - system will not draw window caption when theming is off
		if (bCallDefProc)
		{
			lRcDef = ::DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		//RECT rcWnd = {0,0, r[0].right-r[0].left, r[0].bottom-r[0].top};
		RECT rcClient; // = gpConEmu->CalcRect(CER_MAINCLIENT, rcWnd, CER_MAIN);
		//_ASSERTE(rcClient.left==0 && rcClient.top==0);
		RECT rcMargins = gpConEmu->CalcMargins(CEM_FRAMECAPTION);

		#if defined(CONEMU_TABBAR_EX)
		if (gpSet->isTabsInCaption)
		{
			_ASSERTE(nCaption==0);
			rcClient = MakeRect(r[0].left+rcMargins.left, r[0].top, r[0].right-rcMargins.right, r[0].bottom-rcMargins.bottom);
		}
		else
		#endif
		{
			// Need screen coordinates!
			rcClient = MakeRect(r[0].left+rcMargins.left, r[0].top+rcMargins.top, r[0].right-rcMargins.right, r[0].bottom-rcMargins.bottom);
			//int nDX = ((rcWnd.right - rcClient.right) >> 1);
			//int nDY = ((rcWnd.bottom - rcClient.bottom - nCaption) >> 1);
			//// Need screen coordinates!
			//OffsetRect(&rcClient, r[0].left + nDX, r[0].top + nDY + nCaption);
		}

		if (!gpSet->isTabsInCaption)
		{
		// pParm->rgrc[0] contains the coordinates of the new client rectangle resulting from the move or resize
		// pParm->rgrc[1] rectangle contains the valid destination rectangle
		// pParm->rgrc[2] rectangle contains the valid source rectangle
		pParm->rgrc[0] = rcClient;
		//TODO:
		#if 0
		if (!bAllowPreserveClient)
		{
			pParm->rgrc[1] = MakeRect(rcClient.left, rcClient.top, rcClient.left, rcClient.top);
			pParm->rgrc[2] = MakeRect(r[2].left, r[2].top, r[2].left, r[2].top);
		}
		else
		#endif
		{
			pParm->rgrc[1] = rcClient; // Mark as valid - only client area. Let system to redraw the frame and caption.
			pParm->rgrc[2] = r[2];
		}
		}

		if (bAllowPreserveClient)
		{
			lRc = WVR_VALIDRECTS;
		}
		// При смене режимов (особенно при смене HideCaption/NotHideCaption)
		// требовать полную перерисовку клиентской области
		else if (mb_DontPreserveClient || (gpConEmu->GetChangeFromWindowMode() != wmNotChanging))
		{
			lRc = WVR_REDRAW;
		}
	}
	else
	{
		// lParam points to a RECT structure. On entry, the structure contains the proposed window
		// rectangle for the window. On exit, the structure should contain the screen coordinates
		// of the corresponding window client area.
		LPRECT nccr = (LPRECT)lParam;
		if (!nccr)
		{
			_ASSERTE(nccr!=NULL);
			return 0;
		}
		RECT rc = *nccr;
		//RECT rcWnd = {0,0, rc.right-rc.left, rc.bottom-rc.top};
		RECT rcClient; // = gpConEmu->CalcRect(CER_MAINCLIENT, rcWnd, CER_MAIN);
		//_ASSERTE(rcClient.left==0 && rcClient.top==0);
		RECT rcMargins = gpConEmu->CalcMargins(CEM_FRAMECAPTION);

		if (bCallDefProc)
		{
			// Call default function JIC
			lRcDef = ::DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		#if defined(CONEMU_TABBAR_EX)
		if (gpSet->isTabsInCaption)
		{
			_ASSERTE(nCaption==0);
			rcClient = MakeRect(rc.left+rcMargins.left, rc.top, rc.right-rcMargins.right, rc.bottom-rcMargins.bottom);
			//int nDX = ((rcWnd.right - rcClient.right) >> 1);
			//int nDY = ((rcWnd.bottom - rcClient.bottom /*- nCaption*/) >> 1);
			//*nccr = MakeRect(rc.left+nDX, rc.top+nDY, rc.right-nDX, rc.bottom-nDY);
		}
		else
		#endif
		{
			// Need screen coordinates!
			rcClient = MakeRect(rc.left+rcMargins.left, rc.top+rcMargins.top, rc.right-rcMargins.right, rc.bottom-rcMargins.bottom);
			//int nDX = ((rcWnd.right - rcClient.right) >> 1);
			//int nDY = ((rcWnd.bottom - rcClient.bottom - nCaption) >> 1);
			//OffsetRect(&rcClient, rc.left + nDX, rc.top + nDY + nCaption);
		}

		*nccr = rcClient;
	}

	//if (!gpSet->isTabsInCaption)
	//{
	//	lRc = DefWindowProc(hWnd, uMsg, wParam, lParam);
	//}
	//else
	//{
	//	if (!gpSet->isTabs || !gpSet->isTabsInCaption)
	//	{
	//		lRc = DefWindowProc(hWnd, uMsg, wParam, lParam);
	//	}
	//	else
	//	{
	//		RECT r[3];
	//		r[0] = *nccr;
	//		if (wParam)
	//		{
	//			r[1] = pParm->rgrc[1];
	//			r[2] = pParm->rgrc[2];
	//		}
	//
	//		if (fdt == fdt_Aero)
	//		{
	//			// В Aero (Glass) важно, чтобы клиентская область начиналась с верхнего края окна,
	//			// иначе не получится "рисовать по стеклу"
	//			nccr->top = r->top; // нада !!!
	//			nccr->left = r->left + GetFrameWidth();
	//			nccr->right = r->right - GetFrameWidth();
	//			nccr->bottom = r->bottom - GetFrameHeight();
	//		}
	//		else
	//		{
	//			//TODO: Темы!!! В XP ширина/высота рамки может быть больше
	//			nccr->top = r->top + GetFrameHeight() + GetCaptionHeight();
	//			nccr->left = r->left + GetFrameWidth();
	//			nccr->right = r->right - GetFrameWidth();
	//			nccr->bottom = r->bottom - GetFrameHeight();
	//		}
	//	}

	//	// Наверное имеет смысл сбрасывать всегда, чтобы Win не пыталась
	//	// отрисовать невалидное содержимое консоли (она же размер меняет)
	//	if (wParam)
	//	{
	//		//pParm->rgrc[1] = *nccr;
	//		//pParm->rgrc[2] = r[2];
	//		memset(pParm->rgrc+1, 0, sizeof(RECT)*2);
	//	}
	//}

	UNREFERENCED_PARAMETER(lRcDef);
	UNREFERENCED_PARAMETER(fdt);
	return lRc;
}

LRESULT CFrameHolder::OnNcHitTest(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT l_result = 0;

	// Обработает клики на кнопках Min/Max/Close
	if (gpConEmu->DrawType() >= fdt_Aero)
	{
		//TODO: Проверить, чтобы за табы окошко НЕ таскалось!
		if (gpConEmu->DwmDefWindowProc(hWnd, WM_NCHITTEST, 0, lParam, &l_result))
			return l_result;
	}


	l_result = DefWindowProc(hWnd, WM_NCHITTEST, wParam, lParam);

	POINT point; // Coordinates, relative to UpperLeft corner of window
	RECT wr; GetWindowRect(hWnd, &wr);
	point.x = (int)(short)LOWORD(lParam) - wr.left;
	point.y = (int)(short)HIWORD(lParam) - wr.top;
	//MapWindowPoints(NULL, hWnd, &point, 1);

	// При скрытии окна заголовка убирается стиль WS_CAPTION,
	// но чтобы можно было оставить возможность ресайза за рамку -
	// ставится стиль WS_DLGFRAME а не WS_THICKFRAME
	// Ресайз "сам" не заработает, коррекция l_result в Win8 не помогает
	#if 0
	if (l_result == HTBORDER)
	{
		int nFrame = GetSystemMetrics(SM_CYDLGFRAME);
		int nShift = GetSystemMetrics(SM_CXSMICON);
		int nWidth = wr.right - wr.left;
		int nHeight = wr.bottom - wr.top;

		if (point.x <= nFrame && point.y <= nShift)
			l_result = HTTOPLEFT;
		else if (point.y <= nFrame && point.x <= nShift)
			l_result = HTTOPLEFT;
		else if (point.x >= (nWidth-nFrame) && point.y <= nShift)
			l_result = HTTOPRIGHT;
		else if (point.x >= (nWidth-nFrame-nShift) && point.y <= nFrame)
			l_result = HTTOPRIGHT;
		else if (point.x <= nFrame)
			l_result = HTLEFT;
		else if (point.x >= (nWidth-nFrame))
			l_result = HTRIGHT;
		else if (point.y <= nFrame)
			l_result = HTTOP;
		else if (point.y >= (nHeight-nFrame))
			l_result = HTBOTTOM;

	}
	else
	#endif
	if (gpConEmu->DrawType() >= fdt_Aero)
	{
		if (point.y < gpConEmu->GetDwmClientRectTopOffset())
		{
			int nShift = GetSystemMetrics(SM_CXSMICON);
			int nFrame = 2;
			//RECT wr; GetWindowRect(hWnd, &wr);
			int nWidth = wr.right - wr.left;
			if (point.x <= nFrame && point.y <= nShift)
				l_result = HTTOPLEFT;
			else if (point.y <= nFrame && point.x <= nShift)
				l_result = HTTOPLEFT;
			else if (point.x >= (nWidth-nFrame) && point.y <= nShift)
				l_result = HTTOPRIGHT;
			else if (point.x >= (nWidth-nFrame-nShift) && point.y <= nFrame)
				l_result = HTTOPRIGHT;
			else if (point.x > nFrame && point.x <= (nFrame+GetSystemMetrics(SM_CXSMICON)))
				l_result = HTSYSMENU;
			else if (point.x <= nFrame)
				l_result = HTLEFT;
			else if (point.x >= (nWidth-nFrame))
				l_result = HTRIGHT;
			else if (point.y <= nFrame)
				l_result = HTTOP;
			else
				l_result = HTCAPTION;
		}
	}

	// Чтобы окно нельзя было "таскать" за табы
	if (l_result == HTCAPTION && gpSet->isTabs)
	{
		if (gpConEmu->mp_TabBar->TabFromCursor(point) >= 0 || gpConEmu->mp_TabBar->TabBtnFromCursor(point) >= 0)
		{
			l_result = HT_CONEMUTAB;
		}
	}

	if (l_result == HTTOP && gpSet->isHideCaptionAlways() && !gpConEmu->mp_TabBar->IsTabsShown())
		l_result = HTCAPTION;

	//if ((l_result == HTCLIENT) && gpSet->isStatusBarShow)
	//{
	//	RECT rcStatus = {};
	//	if (gpConEmu->mp_Status->GetStatusBarItemRect(csi_SizeGrip, &rcStatus))
	//	{
	//		RECT rcFrame = gpConEmu->CalcMargins(CEM_FRAMECAPTION);
	//		POINT ptOver = {point.x - rcFrame.left, point.y - rcFrame.top};
	//		if (PtInRect(&rcStatus, ptOver))
	//		{
	//			l_result = HTBOTTOMRIGHT;
	//		}
	//	}
	//}

	return l_result;
}

//LRESULT CFrameHolder::OnNcLButtonDown(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
//{
//	LRESULT l_result = 0;
//
//	if (wParam == HTBORDER /*HTCAPTION*/)
//	{
//		POINT point;
//		RECT wr; GetWindowRect(hWnd, &wr);
//		point.x = (int)(short)LOWORD(lParam) - wr.left;
//		point.y = (int)(short)HIWORD(lParam) - wr.top;
//		//MapWindowPoints(NULL, hWnd, &point, 1);
//
//		int nTab = gpConEmu->TabFromCursor(point);
//		if (nTab >= 0)
//		{
//			gpConEmu->SelectTab(nTab);
//		}
//	}
//
//	l_result = DefWindowProc(hWnd, uMsg, wParam, lParam);
//
//	return l_result;
//}

void CFrameHolder::RecalculateFrameSizes()
{
	_ASSERTE(mb_Initialized==TRUE); // CConEmuMain должен позвать из своего конструктора InitFrameHolder()

	//mn_WinCaptionHeight, mn_FrameWidth, mn_FrameHeight, mn_OurCaptionHeight, mn_TabsHeight;
	mn_WinCaptionHeight = GetSystemMetrics(SM_CYCAPTION);
	mn_FrameWidth = GetSystemMetrics(SM_CXFRAME);
	mn_FrameHeight = GetSystemMetrics(SM_CYFRAME);

	//int nHeightIdeal = mn_WinCaptionHeight * 3 / 4;
	//if (nHeightIdeal < 19) nHeightIdeal = 19;
	mn_TabsHeight = GetTabsHeight(); // min(nHeightIdeal,mn_OurCaptionHeight);

	//int nCaptionDragHeight = 10;
	int nCaptionDragHeight = GetSystemMetrics(SM_CYSIZE); // - mn_FrameHeight;
	if (gpConEmu->DrawType() >= fdt_Themed)
	{
	//	//SIZE sz = {}; RECT tmpRc = MakeRect(600,400);
	//	//HANDLE hTheme = gpConEmu->OpenThemeData(NULL, L"WINDOW");
	//	//HRESULT hr = gpConEmu->GetThemePartSize(hTheme, NULL/*dc*/, 18/*WP_CLOSEBUTTON*/, 1/*CBS_NORMAL*/, &tmpRc, 2/*TS_DRAW*/, &sz);
	//	//if (SUCCEEDED(hr))
	//	//	nCaptionDragHeight = max(sz.cy - mn_FrameHeight,10);
	//	//gpConEmu->CloseThemeData(hTheme);
		nCaptionDragHeight -= mn_FrameHeight;
	}
	mn_CaptionDragHeight = max(10,nCaptionDragHeight);

	if (gpSet->isCaptionHidden())
	{
		mn_OurCaptionHeight = 0;
	}
#if defined(CONEMU_TABBAR_EX)
	else if (gpSet->isTabsInCaption && gpConEmu->mp_TabBar && gpConEmu->mp_TabBar->IsTabsActive())
	{
		if ((GetCaptionDragHeight() == 0) && (mn_TabsHeight > mn_WinCaptionHeight))
		{
			// Если дополнительную высоту в заголовке не просили -
			// нужно строго уместиться в стандартную высоту заголовка
			mn_TabsHeight = mn_WinCaptionHeight;
		}
		int nCHeight = mn_TabsHeight + GetCaptionDragHeight();
		mn_OurCaptionHeight = max(mn_WinCaptionHeight,nCHeight);
	}
#endif
	else
	{
		mn_OurCaptionHeight = mn_WinCaptionHeight;
	}


	//if (gpConEmu->DrawType() == fdt_Aero)
	//{
	//	rcCaption->left = 2; //GetSystemMetrics(SM_CXFRAME);
	//	rcCaption->right = (rcWindow.right - rcWindow.left) - GetSystemMetrics(SM_CXFRAME);
	//	rcCaption->top = 6; //GetSystemMetrics(SM_CYFRAME);
	//	rcCaption->bottom = gpConEmu->GetDwmClientRectTopOffset() - 1;
	//}
	//else if (gpConEmu->DrawType() == fdt_Themed)
	//{
	//	rcCaption->left = GetSystemMetrics(SM_CXFRAME);
	//	rcCaption->right = (rcWindow.right - rcWindow.left) - GetSystemMetrics(SM_CXFRAME) - 1;
	//	rcCaption->top = GetSystemMetrics(SM_CYFRAME);
	//	rcCaption->bottom = rcCaption->top + GetSystemMetrics(SM_CYCAPTION) + (gpSet->isTabs ? (GetSystemMetrics(SM_CYCAPTION)/2) : 0);
	//}
	//else
	//{
	//	rcCaption->left = GetSystemMetrics(SM_CXFRAME);
	//	rcCaption->right = (rcWindow.right - rcWindow.left) - GetSystemMetrics(SM_CXFRAME) - 1;
	//	rcCaption->top = GetSystemMetrics(SM_CYFRAME);
	//	rcCaption->bottom = rcCaption->top + GetSystemMetrics(SM_CYCAPTION) - 1 + (gpSet->isTabs ? (GetSystemMetrics(SM_CYCAPTION)/2) : 0);
	//}
}

// ширина рамки окна
int CFrameHolder::GetFrameWidth()
{
	_ASSERTE(mb_Initialized && mn_FrameWidth > 0);
	return mn_FrameWidth;
}

// высота рамки окна
int CFrameHolder::GetFrameHeight()
{
	_ASSERTE(mb_Initialized && mn_FrameHeight > 0);
	return mn_FrameHeight;
}

// высота НАШЕГО заголовка (с учетом табов)
int CFrameHolder::GetCaptionHeight()
{
	_ASSERTE(mb_Initialized && ((mn_OurCaptionHeight > 0) || gpSet->isCaptionHidden()));
	return mn_OurCaptionHeight;
}

// высота табов
int CFrameHolder::GetTabsHeight()
{
	mn_TabsHeight = (gpSet->isTabs!=0) ? gpConEmu->mp_TabBar->GetTabbarHeight() : 0;
	return mn_TabsHeight;

	//#ifndef CONEMU_TABBAR_EX
	//	return gpConEmu->mp_TabBar->GetTabbarHeight();
	//#else
	//	_ASSERTE(mb_Initialized && mn_TabsHeight > 0);
	//	return mn_TabsHeight;
	//#endif
}

// высота части заголовка, который оставляем для "таскания" окна
int CFrameHolder::GetCaptionDragHeight()
{
	_ASSERTE(mb_Initialized && (mn_CaptionDragHeight >= 0));
	////TODO: Хорошо бы учитывать текущее выставленное dpi для монитора?
	//if (gpSet->ilDragHeight < 0)
	//{
	//	_ASSERTE(gpSet->ilDragHeight>=0);
	//	gpSet->ilDragHeight = 0;
	//}
	//return gpSet->ilDragHeight;
	return mn_CaptionDragHeight;
}

// высота заголовка в окнах Windows по умолчанию (без учета табов)
int CFrameHolder::GetWinCaptionHeight()
{
	_ASSERTE(mb_Initialized);
	return mn_WinCaptionHeight;
}

void CFrameHolder::GetIconShift(POINT& IconShift)
{
	switch (gpConEmu->DrawType())
	{
	case fdt_Win8:
		{
			IconShift.x = 2;
			IconShift.y = 0;
		}
		break;

	case fdt_Aero:
		{
			IconShift.x = 3;
			//IconShift.y = (gpConEmu->isZoomed()?4:0)+(gpSet->ilDragHeight ? 4 : 1);
			IconShift.y = gpConEmu->GetCaptionHeight() - 16;
			if (IconShift.y < 0)
				IconShift.y = 0;
			else if (IconShift.y > 4)
				IconShift.y = 4;
			if (gpConEmu->isZoomed())
				IconShift.y += 4;
		}
		break;

	case fdt_Themed:
		{
			IconShift.x = 3;
			IconShift.y = gpConEmu->GetCaptionHeight() - 16;
			if (IconShift.y < 0)
				IconShift.y = 0;
			else if (IconShift.y > 4)
				IconShift.y = 4;
		}
		break;

	default:
		// Win2k
		{
			IconShift.x = 2;
			IconShift.y = 1; //(gpSet->ilDragHeight ? 1 : 1);
		}
	}
}

void CFrameHolder::SetFrameActiveState(bool bActive)
{
	SendMessage(ghWnd, WM_NCACTIVATE, bActive, 0);
}
