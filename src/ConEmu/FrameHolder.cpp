
/*
Copyright (c) 2009-2012 Maximus5
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
#include <windows.h>
#include "DwmApi_Part.h"
#include <TCHAR.H>
//#ifdef _DEBUG
//#include <CRTDBG.H>
//#endif
#include "Header.h"
#include "TabBar.h"
#include "FrameHolder.h"
#include "ConEmu.h"
#include "Options.h"
#include "Status.h"

#ifdef _DEBUG
static int _nDbgStep = 0; wchar_t _szDbg[512];
#endif
#define DBGFUNCTION(s) //{ wsprintf(_szDbg, L"%i: %s", ++_nDbgStep, s); OutputDebugString(_szDbg); /*Sleep(1000);*/ }
#define DEBUGSTRSIZE(s) DEBUGSTR(s)

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
		lResult = TRUE; return true;
		//return false;

	case WM_PAINT:
		DBGFUNCTION(L"WM_PAINT \n");
		lResult = OnPaint(hWnd, FALSE); return true;

	case WM_NCPAINT:
		DBGFUNCTION(L"WM_NCPAINT \n");
		lResult = OnNcPaint(hWnd, uMsg, wParam, lParam); return true;

	case WM_NCACTIVATE:
		DBGFUNCTION(L"WM_NCACTIVATE \n");
		lResult = OnNcActivate(hWnd, uMsg, wParam, lParam); return true;

	case WM_NCCALCSIZE:
		DBGFUNCTION(L"WM_NCCALCSIZE \n");
		lResult = OnNcCalcSize(hWnd, uMsg, wParam, lParam); return true;

	case WM_NCHITTEST:
		DBGFUNCTION(L"WM_NCHITTEST \n");
		lResult = OnNcHitTest(hWnd, uMsg, wParam, lParam); return true;

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
			gpConEmu->isSizing(); // могло не сброситься, проверим

		RedrawLock();
		lbRc = gpConEmu->mp_TabBar->ProcessTabMouseEvent(hWnd, uMsg, wParam, lParam, lResult);
		RedrawUnlock();
		if (!lbRc)
		{
			if ((wParam == HTSYSMENU && uMsg == WM_NCLBUTTONDOWN)
				/*|| (wParam == HTCAPTION && uMsg == WM_NCRBUTTONDOWN)*/)
			{
				//if (uMsg == WM_NCRBUTTONDOWN)
				//	gpConEmu->ShowSysmenu((SHORT)LOWORD(lParam),(SHORT)HIWORD(lParam));
				//else

				DWORD nCurTick = GetTickCount();
				DWORD nOpenDelay = nCurTick - gpConEmu->mn_SysMenuOpenTick;
				DWORD nCloseDelay = nCurTick - gpConEmu->mn_SysMenuCloseTick;
				DWORD nDoubleTime = GetDoubleClickTime();

				if (gpConEmu->mn_SysMenuOpenTick && (nOpenDelay < nDoubleTime))
				{
					PostMessage(ghWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
				}
				else if (gpConEmu->mn_SysMenuCloseTick && (nCloseDelay < (nDoubleTime/2)))
				{
					// Пропустить - кликом закрыли меню
					#ifdef _DEBUG
					int nDbg = 0;
					#endif
				}
				else
				{
					gpConEmu->ShowSysmenu();
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
	//			PostMessage(ghWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
	//			lResult = 0;
	//			return true;
	//		}
	//	}
	//	return false;
		
	case WM_MOUSEMOVE:
		DBGFUNCTION(L"WM_MOUSEMOVE \n");
		RedrawLock();
		if (gpConEmu->mp_TabBar->GetHoverTab() != -1)
		{
			gpConEmu->mp_TabBar->HoverTab(-1);
		}
		// Ну и с кнопок убрать подсветку, если была
		gpConEmu->mp_TabBar->Toolbar_UnHover();
		RedrawUnlock();
		return false;
		
	case WM_NCLBUTTONDBLCLK:
		if (wParam == HTCAPTION)
		{
			mb_NcAnimate = TRUE;
		}
		
		if (wParam == HT_CONEMUTAB)
		{
			//RedrawLock(); -- чтобы отрисовать "клик" по кнопке
			lbRc = gpConEmu->mp_TabBar->ProcessTabMouseEvent(hWnd, uMsg, wParam, lParam, lResult);
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
		lResult = OnDwmMessage(hWnd, uMsg, wParam, lParam); return true;
	case 0xAF: /*WM_NCUAHDRAWFRAME*/
		lResult = OnDwmMessage(hWnd, uMsg, wParam, lParam); return true;
	case 0x31E: /*WM_DWMCOMPOSITIONCHANGED*/
		lResult = OnDwmMessage(hWnd, uMsg, wParam, lParam); return true;

	case WM_WINDOWPOSCHANGED:
		lResult = OnWindowPosChanged(hWnd, uMsg, wParam, lParam); return true;
		
	case WM_SYSCOMMAND:
		if (wParam == SC_MAXIMIZE || wParam == SC_MINIMIZE || wParam == SC_RESTORE)
		{
			mb_NcAnimate = TRUE;
			//GetWindowText(hWnd, ms_LastCaption, countof(ms_LastCaption));
			//SetWindowText(hWnd, L"");
		}
		lResult = gpConEmu->OnSysCommand(hWnd, wParam, lParam);
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
	if (gpSet->isTabsInCaption)
	{
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
			return (gpConEmu->DrawType()==fdt_Aero) ? DefWindowProc(hWnd, uMsg, wParam, lParam) : 0;
		}
	}
	
	// Unknown!
	_ASSERTE(uMsg == 0x31E || uMsg == 0xAE || uMsg == 0xAF);
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void CFrameHolder::NC_Redraw(HWND hWnd)
{
	mb_RedrawRequested = false;
	
	//TODO: При потере фокуса (клик мышкой по другому окну) лезут глюки отрисовки этого нового окна
	SetWindowPos(hWnd, 0, 0, 0, 0, 0,
		SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_DRAWFRAME);
	
	//SetWindowPos(ghWnd, NULL, 0, 0, 0, 0,
	//		SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	//RedrawWindow(ghWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
	//RedrawWindow(ghWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME);

	// В Aero отрисовка идет как бы на клиентской части
	if (gpConEmu->DrawType() == fdt_Aero)
	{
		InvalidateRect(hWnd, 0,0);
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
		NC_Redraw(ghWnd);
	}
}

void CFrameHolder::RedrawTabPanel()
{
	//TODO: табы/кнопки могут быть в клиентской области!
	if (mn_RedrawLockCount > 0)
	{
		mb_RedrawRequested = true;
	}
	else
	{
		NC_Redraw(ghWnd);
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
	SetWindowLong(hWnd, GWL_STYLE, dwStyle & ~WS_VISIBLE);
	
	//Do the default thing..
	ret = DefWindowProc(hWnd, uMsg, wParam, lParam);
	
	//Restore the original style
	SetWindowLong(hWnd, GWL_STYLE, dwStyle);

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

LRESULT CFrameHolder::OnPaint(HWND hWnd, BOOL abForceGetDc)
{
	HDC hdc;
	PAINTSTRUCT ps = {0};
	RECT wr, cr, tr;
	
	RecalculateFrameSizes();

	hdc = BeginPaint(hWnd, &ps);

	GetClientRect(hWnd, &wr);

	if (gpSet->isStatusBarShow)
	{
		int nHeight = gpSet->StatusBarHeight();
		if (nHeight < (wr.bottom - wr.top))
		{
			RECT rcStatus = {wr.left, wr.bottom - nHeight, wr.right, wr.bottom};
			gpConEmu->mp_Status->PaintStatus(hdc, rcStatus);
			wr.bottom = rcStatus.top;
		}
	}

	cr = wr;

	if (gpConEmu->DrawType() == fdt_Aero)
	{
		int nOffset = gpConEmu->GetDwmClientRectTopOffset();
		// "Рамка" расширена на клиентскую область, поэтому
		// нужно зарисовать заголовок черной кистью, иначе идет
		// искажение цвета для кнопок Min/Max/Close

		if (gpSet->isTabs && gpSet->isTabsInCaption)
		{
			RECT captrect;
			CalculateCaptionPosition(cr, &captrect);
			CalculateTabPosition(cr, captrect, &tr);

			gpConEmu->mp_TabBar->PaintTabs(hdc, captrect, tr);

			mb_WasGlassDraw = TRUE;
		}

		cr.top += nOffset;
	}


	int nWidth = (cr.right-cr.left);
	int nHeight = (cr.bottom-cr.top);

	WARNING("Пока табы рисуем не сами и ExtendDWM отсутствует - дополнительные изыски с временным DC не нужны");
	if (!gpSet->isTabsInCaption)
	{
		OnPaintClient(hdc, nWidth, nHeight);
	}
	else

	// Создадим временный DC, для удобства отрисовки в Glass-режиме и для фикса глюка DWM(?) см.ниже
	// В принципе, для режима Win2k/XP временный DC можно не создавать, если это будет тормозить
	{
		HDC hdcPaint = CreateCompatibleDC(hdc);
		HBITMAP hbmp = CreateCompatibleBitmap(hdc, nWidth, nHeight);
		HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcPaint, hbmp);

		OnPaintClient(hdcPaint, nWidth, nHeight);

		if ((gpConEmu->DrawType() == fdt_Aero) || !(mb_WasGlassDraw && gpConEmu->isZoomed()))
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

	EndPaint(hWnd, &ps);

	return 0;
}

void CFrameHolder::CalculateCaptionPosition(const RECT &rcWindow, RECT* rcCaption)
{
	if (gpConEmu->DrawType() == fdt_Aero)
	{
		// Почему тут не SM_CXFRAME не помню
		rcCaption->left = 0; //2; //GetSystemMetrics(SM_CXFRAME);
		rcCaption->right = (rcWindow.right - rcWindow.left); // - 4; //GetSystemMetrics(SM_CXFRAME);
		rcCaption->top = GetFrameHeight(); //6; //GetSystemMetrics(SM_CYFRAME);
		rcCaption->bottom = rcCaption->top + GetCaptionHeight()/*это наш*/; // (gpSet->isTabs ? (GetCaptionDragHeight()+GetTabsHeight()) : GetWinCaptionHeight()); //gpConEmu->GetDwmClientRectTopOffset() - 1;
	}
	else if (gpConEmu->DrawType() == fdt_Themed)
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

	rcTabs->left = rcCaption.left + GetSystemMetrics(SM_CXSMICON) + 3;
	rcTabs->bottom = rcCaption.bottom - 1;
	rcTabs->top = max((rcCaption.bottom - GetTabsHeight()/*nHeightIdeal*/), rcCaption.top);

	// TODO: определить ширину кнопок + Shift из настроек юзера

	if (gpConEmu->DrawType() == fdt_Aero)
	{
		RECT rcButtons = {0};
		HRESULT hr = gpConEmu->DwmGetWindowAttribute(ghWnd, DWMWA_CAPTION_BUTTON_BOUNDS, &rcButtons, sizeof(rcButtons));

		if (SUCCEEDED(hr))
			rcTabs->right = min((rcButtons.left - 4),(rcCaption.right - 3*nBtnWidth));
		else
			rcTabs->right = rcCaption.right - 3*nBtnWidth;
	}
	else if (gpConEmu->DrawType() == fdt_Themed)
	{
		// -- Не работает :(
		//HTHEME hTheme = gpConEmu->OpenThemeData(NULL, L"WINDOW");
		//RECT rcBtns, wr; GetWindowRect(ghWnd, &wr); OffsetRect(&wr, -wr.left, -wr.top);
		//HRESULT hr = gpConEmu->GetThemeBackgroundContentRect(hTheme, NULL, 15/*WP_MINBUTTON*/, 1/*MINBS_NORMAL*/, &wr, &rcBtns);
		//gpConEmu->CloseThemeData(hTheme);

		rcTabs->right = rcCaption.right - 3*nBtnWidth;
	}
	else
	{
		rcTabs->right = rcCaption.right - 3*nBtnWidth /*+ 2*/;
	}
}

LRESULT CFrameHolder::OnNcPaint(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RecalculateFrameSizes();

	if (!gpSet->isTabsInCaption)
	{
		LRESULT lRc = DefWindowProc(hWnd, uMsg, wParam, lParam);
		return lRc;
	}

	if (gpConEmu->DrawType() == fdt_Aero)
	{
		LRESULT lRc = DefWindowProc(hWnd, uMsg, wParam, lParam);
		//TODO: Может быть на "стекле" сразу рисовать, а не в WM_PAINT?
		return lRc;
	}
		
	if (!gpSet->isTabs)
	{
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}


	RECT dirty_box, dirty, wr, tr, cr, xorRect;
	BOOL fRegionOwner = FALSE;
	HDC hdc;
	HRGN hrgn = (HRGN)wParam;

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
	gpConEmu->mp_TabBar->PaintTabs(hdc, cr, tr);

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
	
	return 0;
}

LRESULT CFrameHolder::OnNcActivate(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	mb_NcActive = (wParam != 0);
	
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
	//NC_Redraw(hWnd);
	OnNcPaint(hWnd, WM_NCPAINT, 1, 0);
	return lRc;
}

LRESULT CFrameHolder::OnNcCalcSize(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RecalculateFrameSizes();

	LRESULT lRc = 0;
	FrameDrawStyle fdt = gpConEmu->DrawType();
	NCCALCSIZE_PARAMS* pParm = wParam ? ((NCCALCSIZE_PARAMS*)lParam) : NULL;
	LPRECT nccr = wParam
			? pParm->rgrc
			: (RECT*)lParam;
			
	if (!gpSet->isTabsInCaption)
	{
		lRc = DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	else
	{
		if (!gpSet->isTabs || !gpSet->isTabsInCaption)
		{
			lRc = DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
		else
		{
			RECT r[3];
			r[0] = *nccr;
			if (wParam)
			{
				r[1] = pParm->rgrc[1];
				r[2] = pParm->rgrc[2];
			}
				
			if (fdt == fdt_Aero)
			{
				// В Aero (Glass) важно, чтобы клиенская область начиналась с верхнего края окна,
				// иначе не получится "рисовать по стеклу"
				nccr->top = r->top; // нада !!!
				nccr->left = r->left + GetFrameWidth();
				nccr->right = r->right - GetFrameWidth();
				nccr->bottom = r->bottom - GetFrameHeight();
			}
			else
			{
				//TODO: Темы!!! В XP ширина/высота рамки может быть больше
				nccr->top = r->top + GetFrameHeight() + GetCaptionHeight();
				nccr->left = r->left + GetFrameWidth();
				nccr->right = r->right - GetFrameWidth();
				nccr->bottom = r->bottom - GetFrameHeight();
			}
		}

		// Наверное имеет смысл сбрасывать всегда, чтобы Win не пыталась
		// отрисовать невалидное содержимое консоли (она же размер меняет)
		if (wParam)
		{
			//pParm->rgrc[1] = *nccr;
			//pParm->rgrc[2] = r[2];
			memset(pParm->rgrc+1, 0, sizeof(RECT)*2);
		}
	}

	// При смене режимов (особенно при смене HideCaption/NotHideCaption)
	// требовать полную перерисовку клиентской области
	if (gpConEmu->changeFromWindowMode != wmNotChanging)
	{
		lRc = WVR_REDRAW;
	}

	return lRc;
}

LRESULT CFrameHolder::OnNcHitTest(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT l_result;

	// Обработает клики на кнопках Min/Max/Close
	if (gpConEmu->DrawType() == fdt_Aero)
	{
		//TODO: Проверить, чтобы за табы окошко НЕ таскалось!
		if (gpConEmu->DwmDefWindowProc(hWnd, WM_NCHITTEST, 0, lParam, &l_result))
			return l_result;
	}


	l_result = DefWindowProc(hWnd, WM_NCHITTEST, wParam, lParam);
	
	POINT point;
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
	if (gpConEmu->DrawType() == fdt_Aero)
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

	int nHeightIdeal = mn_WinCaptionHeight * 3 / 4;
	if (nHeightIdeal < 19) nHeightIdeal = 19;
	mn_TabsHeight = min(nHeightIdeal,mn_OurCaptionHeight);

	if (gpSet->isTabsInCaption && gpConEmu->mp_TabBar->IsTabsActive())
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
	_ASSERTE(mb_Initialized && mn_OurCaptionHeight > 0);
	return mn_OurCaptionHeight;
}

// высота табов
int CFrameHolder::GetTabsHeight()
{
	_ASSERTE(mb_Initialized && mn_TabsHeight > 0);
	return mn_TabsHeight;
}

// высота части заголовка, который оставляем для "таскания" окна
int CFrameHolder::GetCaptionDragHeight()
{
	_ASSERTE(mb_Initialized);
	//TODO: Хорошо бы учитывать текущее выставленное dpi для монитора?
	if (gpSet->ilDragHeight < 0)
	{
		_ASSERTE(gpSet->ilDragHeight>=0);
		gpSet->ilDragHeight = 0;
	}
	return gpSet->ilDragHeight;
}

// высота заголовка в окнах Windows по умолчанию (без учета табов)
int CFrameHolder::GetWinCaptionHeight()
{
	_ASSERTE(mb_Initialized);
	return mn_WinCaptionHeight;
}

void CFrameHolder::GetIconShift(POINT& IconShift)
{
	if (gpConEmu->DrawType() == fdt_Aero)
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
	else if (gpConEmu->DrawType() == fdt_Themed)
	{
		IconShift.x = 3;
		IconShift.y = gpConEmu->GetCaptionHeight() - 16;
		if (IconShift.y < 0)
			IconShift.y = 0;
		else if (IconShift.y > 4)
			IconShift.y = 4;
	}
	else // Win2k
	{
		IconShift.x = 2;
		IconShift.y = 1; //(gpSet->ilDragHeight ? 1 : 1);
	}
}
