
/*
Copyright (c) 2014-present Maximus5
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
#define SHOWDEBUGSTR

//#define HIDE_TODO

#include "Header.h"
#include "../common/DefTermBase.h"
#include "../common/Monitors.h"
#include "../common/MSetter.h"
#include "../common/WUser.h"

#include "ConEmu.h"
#include "ConEmuSize.h"
#include "DpiAware.h"
#include "DwmApi_Part.h"
#include "HooksUnlocker.h"
#include "Inside.h"
#include "Menu.h"
#include "OptionsClass.h"
#include "Status.h"
#include "TabBar.h"
#include "TaskBar.h"
#include "TrayIcon.h"
#include "VConGroup.h"
#include "VirtualConsole.h"

#define DEBUGSTRSIZE(s) //DEBUGSTR(s)
#define DEBUGSTRSTYLE(s) //DEBUGSTR(s)
#define DEBUGSTRWMODE(s) //DEBUGSTR(s)
#define DEBUGSTRDPI(s) //DEBUGSTR(s)
#define DEBUGSTRRGN(s) //DEBUGSTR(s)
#define DEBUGSTRPANEL2(s) //DEBUGSTR(s)
#define DEBUGSTRPANEL(s) //DEBUGSTR(s)

#define FIXPOSAFTERSTYLE_DELTA 2500


/// Helper class to call and automatically revert DisableThickFrame
CConEmuSize::ThickFrameDisabler::ThickFrameDisabler(CConEmuSize& _conemu, bool _disable /*= false*/)
	: conemu(_conemu), disabled(false)
{
	if (_disable)
		Disable();
}

CConEmuSize::ThickFrameDisabler::~ThickFrameDisabler()
{
	Enable();
}

void CConEmuSize::ThickFrameDisabler::Disable()
{
	if (disabled)
		return;
	disabled = true;
	conemu.DisableThickFrame(true);
}

void CConEmuSize::ThickFrameDisabler::Enable()
{
	if (!disabled)
		return;
	disabled = false;
	conemu.DisableThickFrame(false);
}


// ****************************************************************** //

/// Part of ConEmu GUI related to sizing functions
CConEmuSize::CConEmuSize()
	: mp_ConEmu(static_cast<CConEmuMain*>(this))
	, SizeInfo(static_cast<CConEmuMain*>(this))
{
	_ASSERTE(mp_ConEmu!=NULL);

	//mb_PostUpdateWindowSize = false;

	WndWidth = gpSet->wndWidth; WndHeight = gpSet->wndHeight;
	WndPos = MakePoint(gpSet->_wndX, gpSet->_wndY);
	m_Snapping.reset();
}

CConEmuSize::~CConEmuSize()
{
}

/*!!!static!!*/
// Функция расчитывает смещения (относительные)
// mg содержит битмаск, например (CEM_FRAMECAPTION|CEM_TAB|CEM_CLIENT)
RECT CConEmuSize::CalcMargins(DWORD/*enum ConEmuMargins*/ mg, ConEmuWindowMode wmNewMode /*= wmCurrent*/)
{
	_ASSERTE(this!=NULL);

	// -- функция должна работать только с главным окном
	// -- VCon тут влиять не должны
	//if (!apVCon)
	//	apVCon = mp_ConEmu->mp_ VActive;

	WARNING("CEM_SCROLL вообще удалить?");
	WARNING("Проверить, чтобы DC нормально центрировалось после удаления CEM_BACK");

	RECT rc = {};

	DEBUGTEST(FrameDrawStyle fdt = mp_ConEmu->DrawType());
	DEBUGTEST(ConEmuWindowMode wm = (wmNewMode == wmCurrent) ? WindowMode : wmNewMode);

	// Windows 10 breaks desktop coordinate system
	if ((mg & ((DWORD)CEM_WIN10FRAME)) && isWin10InvisibleFrame())
	{
		RECT rcHiddenFrame = CalcMargins_Win10Frame();
		AddMargins(rc, rcHiddenFrame, rcop_MathAdd);
	}

	// Difference between whole window size and its client area (frame + caption)
	if (mg & ((DWORD)CEM_FRAMECAPTION))
	{
		// CEM_FRAMEONLY|CEM_CAPTION
		RECT rcFrameCaption = CalcMargins_FrameCaption(mg, wmNewMode);
		AddMargins(rc, rcFrameCaption, rcop_MathAdd);
	}

	// Сколько занимают табы (по идее меняется только rc.top or rc.bottom)
	if (mg & ((DWORD)CEM_TAB))
	{
		RECT rcTab = CalcMargins_TabBar(mg);
		AddMargins(rc, rcTab, rcop_MathAdd);
	}

	if (mg & ((DWORD)CEM_PAD))
	{
		RECT rcPad = CalcMargins_Padding();
		AddMargins(rc, rcPad, rcop_MathAdd);
	}

	if (mg & ((DWORD)CEM_SCROLL))
	{
		RECT rcScroll = CalcMargins_Scrolling();
		AddMargins(rc, rcScroll, rcop_MathAdd);
	}

	if (mg & ((DWORD)CEM_STATUS))
	{
		RECT rcStatus = CalcMargins_StatusBar();
		AddMargins(rc, rcStatus, rcop_MathAdd);
	}

	if (mg & ((DWORD)CEM_VISIBLE_FRAME))
	{
		MBoxAssert(mg == CEM_VISIBLE_FRAME); // Nothing else yet allowed
		RECT rcFrameOnly = CalcMargins_VisibleFrame();
		AddMargins(rc, rcFrameOnly, rcop_MathAdd);
	}

	if (mg & ((DWORD)CEM_INVISIBLE_FRAME))
	{
		MBoxAssert(mg == CEM_INVISIBLE_FRAME); // Nothing else yet allowed
		RECT rcFrameOnly = CalcMargins_InvisibleFrame();
		AddMargins(rc, rcFrameOnly, rcop_MathAdd);
	}

	return rc;
}

RECT CConEmuSize::CalcMargins_Win10Frame() const
{
	RECT rc = {};

	if (isWin10InvisibleFrame())
	{
		const MonitorInfoCache mi = NearestMonitorInfo(NULL);
		const auto& fi = isCaptionHidden() ? mi.noCaption : mi.withCaption;
		rc = fi.Win10Stealthy;
	}

	return rc;
}

// CEM_FRAMECAPTION : Difference between whole window size and its client area (frame + caption)
// CEM_CAPTION      : only window title bar (height), if applicable
// CEM_FRAMEONLY    : only frame borders, if applicable
RECT CConEmuSize::CalcMargins_FrameCaption(DWORD/*enum ConEmuMargins*/ mg, ConEmuWindowMode wmNewMode /*= wmCurrent*/) const
{
	RECT rc = {};
	if (!(mg & ((DWORD)CEM_FRAMECAPTION)) || mp_ConEmu->mp_Inside)
		return rc;

	ConEmuWindowMode wm = (wmNewMode == wmCurrent) ? WindowMode : wmNewMode;
	// We remove all frames and caption in FullScreen mode
	if (wm == wmFullScreen)
		return rc;

	bool bHideCaption = isCaptionHidden(wmNewMode);
	// If there is no caption, nothing to evaluate
	if (bHideCaption && ((mg & ((DWORD)CEM_FRAMECAPTION)) == CEM_CAPTION))
		return rc;

	bool processed = false;

	const MonitorInfoCache mi = NearestMonitorInfo(NULL);

	if (!processed && bHideCaption)
	{
		_ASSERTE((mg & ((DWORD)CEM_FRAMECAPTION)) != CEM_CAPTION);
		if ((mg & ((DWORD)CEM_FRAMECAPTION)) & CEM_FRAMEONLY)
		{
			if (isSelfFrame())
			{
				UINT frame = mp_ConEmu->GetSelfFrameWidth();
				rc = RECT{(LONG)frame, (LONG)frame, (LONG)frame, (LONG)frame};
			}
			else
			{
				rc = mi.noCaption.FrameMargins;
			}
		}
		processed = true;
	}

	if (!processed)
	{
		DWORD dwStyle = mp_ConEmu->GetWindowStyle();
		if (wmNewMode != wmCurrent)
			dwStyle = mp_ConEmu->FixWindowStyle(dwStyle, wmNewMode);
		DWORD dwStyleEx = mp_ConEmu->GetWindowStyleEx();
		const int nTestWidth = 100, nTestHeight = 100;
		RECT rcTest = MakeRect(nTestWidth,nTestHeight);

		_ASSERTE(!bHideCaption);
		if (mp_ConEmu->AdjustWindowRectExForDpi(&rcTest, dwStyle, FALSE, dwStyleEx, mi.Ydpi))
		{
			if ((mg & ((DWORD)CEM_FRAMECAPTION)) == CEM_CAPTION)
			{
				rc.top  = (-rcTest.top) - (rcTest.bottom - nTestHeight);
			}
			else
			{
				rc.left = -rcTest.left;
				rc.right = rcTest.right - nTestWidth;
				rc.bottom = rcTest.bottom - nTestHeight;
				if ((mg & ((DWORD)CEM_CAPTION)) && !bHideCaption)
					rc.top = -rcTest.top;
				else
					rc.top = rc.bottom;
			}
			_ASSERTE(rc.top >= 0 && rc.left >= 0 && rc.right >= 0 && rc.bottom >= 0);
			processed = true;
		}
	}

	if (!processed)
	{
		_ASSERTE(FALSE && "Must not get here, our AdjustWindowRectExForDpi is expected to be OK");
		if ((mg & ((DWORD)CEM_FRAMECAPTION)) & (DWORD)CEM_FRAMEONLY)
		{
			rc.left = rc.right = rc.bottom = rc.top = mp_ConEmu->GetWinFrameWidth();
		}

		if ((mg & ((DWORD)CEM_CAPTION)) && !bHideCaption)
		{
			rc.top += mp_ConEmu->GetWinCaptionHeight();
		}
	}

	return rc;
}

RECT CConEmuSize::CalcMargins_TabBar(DWORD/*enum ConEmuMargins*/ mg) const
{
	RECT rc = {};

	// The place (height) used by TabBar. Only rc.top or rc.bottom may be set
	if (mg & ((DWORD)CEM_TAB))
	{
		#ifdef _DEBUG
		if (ghWnd)
		{
			const bool isActive = mp_ConEmu->isTabsShown();
			_ASSERTE(((mg & CEM_TAB_MASK) == CEM_TAB) || (isActive == ((mg & ((DWORD)CEM_TABACTIVATE)) == ((DWORD)CEM_TABACTIVATE))));
		}
		#endif

		// Check tabs condition - if they are enabled (1==always show) their size must be taken into account
		SizeInfo temp(*static_cast<const SizeInfo*>(this));
		temp.RequestSize(600, 300);
		// #SIZE_TODO get dpi and coordinates for destination monitor?
		RECT rcTabs = temp.RebarRect();
		switch (gpSet->nTabsLocation)
		{
		case 0: // top
			rc.top += RectHeight(rcTabs); break;
		case 1: // bottom
			rc.bottom = RectHeight(rcTabs); break;
		default:
			_ASSERTE(FALSE && "Unsupported location of TabBar!");
			rc = {};
		}
	}

	return rc;
}

RECT CConEmuSize::CalcMargins_StatusBar() const
{
	RECT rc = {};

	if (gpSet->isStatusBarShow)
	{
		TODO("Зависимость от темы/шрифта");
		rc.bottom += gpSet->StatusBarHeight();
	}

	return rc;
}

RECT CConEmuSize::CalcMargins_Padding() const
{
	RECT rc = {};

	if (gpSet->isTryToCenter)
	{
		rc.left += gpSet->nCenterConsolePad;
		rc.top += gpSet->nCenterConsolePad;
		rc.right += gpSet->nCenterConsolePad;
		rc.bottom += gpSet->nCenterConsolePad;
	}

	return rc;
}

RECT CConEmuSize::CalcMargins_Scrolling() const
{
	RECT rc = {};

	// TODO: Horizontal scroll
	if ((gpSet->isAlwaysShowScrollbar == 1))
	{
		rc.right += GetSystemMetrics(SM_CXVSCROLL);
	}

	return rc;
}

RECT CConEmuSize::CalcMargins_VisibleFrame(LPRECT prcFrame /*= NULL*/) const
{
	RECT rcFrameOnly = {};
	if (mp_ConEmu->isInside())
	{
		if (prcFrame)
			*prcFrame = rcFrameOnly;
		return rcFrameOnly;
	}

	int iVisibleFrameWidth = isSelfFrame() ? gpSet->HideCaptionAlwaysFrame() : -1;
	const RECT rcReal = CalcMargins_FrameCaption(CEM_FRAMEONLY);
	if (prcFrame)
		*prcFrame = rcReal;

	// 0 means we hide frame completely
	if (iVisibleFrameWidth != 0)
	{
		rcFrameOnly = rcReal;

		// Windows 10 DWM hides portions of the actual frame
		if (isWin10InvisibleFrame())
		{
			// BUGBUG: During m_ForceShowFrame DwmGetWindowAttribute returns yet old values (fixed frame), but we need estimated values for changes styles
			const RECT rcWin10 = CalcMargins_Win10Frame();
			MBoxAssert(rcWin10.left>0 && rcWin10.top>=0 && rcWin10.right>0 && rcWin10.bottom>0);
			MBoxAssert(rcWin10.left<=rcReal.left && rcWin10.top<=rcReal.top && rcWin10.right<=rcReal.right && rcWin10.bottom<=rcReal.bottom);
			AddMargins(rcFrameOnly, rcWin10, rcop_MathSub);
		}

		// If user asks to hide frame partially
		if (iVisibleFrameWidth > 0)
		{
			rcFrameOnly.left = std::min<int>(rcFrameOnly.left, iVisibleFrameWidth);
			rcFrameOnly.right = std::min<int>(rcFrameOnly.right, iVisibleFrameWidth);
			rcFrameOnly.top = std::min<int>(rcFrameOnly.top, iVisibleFrameWidth);
			rcFrameOnly.bottom = std::min<int>(rcFrameOnly.bottom, iVisibleFrameWidth);
		}

		// Quake - hides top edge
		if (gpSet->isQuakeStyle)
		{
			rcFrameOnly.top = 0;
		}
	}

	return rcFrameOnly;
}

RECT CConEmuSize::CalcMargins_InvisibleFrame() const
{
	RECT rcFrameOnly = {};
	if (mp_ConEmu->isInside())
		return rcFrameOnly;

	const RECT rcVisible = CalcMargins_VisibleFrame(&rcFrameOnly);
	RECT rcInvisible = rcFrameOnly;
	AddMargins(rcInvisible, rcVisible, rcop_MathSub);
	return rcInvisible;
}

RECT CConEmuSize::CalcRect(enum ConEmuRect tWhat, CVirtualConsole* pVCon /*= NULL*/)
{
	_ASSERTE(ghWnd!=NULL);
	RECT rcMain = {};
	WINDOWPLACEMENT wpl = {sizeof(wpl)};
	int nGetStyle = 0;

	bool bNeedCalc = (isIconic() || InMinimizing() || mp_ConEmu->mp_Menu->GetRestoreFromMinimized() || !IsWindowVisible(ghWnd));

	if (mp_ConEmu->mp_Inside)
	{
		if (!mp_ConEmu->mp_Inside->GetInsideRect(&rcMain))
		{
			_ASSERTE(FALSE && "GetInsideRect failed");
			rcMain = {};
			return rcMain;
		}
	}
	else
	if (bNeedCalc)
	{
		nGetStyle = 1;

		if (gpSet->isQuakeStyle)
		{
			rcMain = GetDefaultRect();
			nGetStyle = 2;
		}
		else
		{
			// Win 8 Bug? Offered screen coordinates, but return - desktop workspace coordinates.
			// Thats why only for zoomed modes (need to detect "monitor"), however, this may be buggy too?
			if ((WindowMode == wmMaximized) || (WindowMode == wmFullScreen))
			{
				// We need RESTORED(NORMAL) rect to be able to find monitor where the window must be maximized/fullscreened
				if (!IsRectMinimized(mrc_StoredNormalRect))
				{
					// Will be CalcRect(CER_MAXIMIZED/CER_FULLSCREEN,...) below
					rcMain = mrc_StoredNormalRect;
					nGetStyle = 3;
				}
				else
				{
					GetWindowPlacement(ghWnd, &wpl);
					// Will be CalcRect(CER_MAXIMIZED/CER_FULLSCREEN,...) below
					rcMain = wpl.rcNormalPosition;
					nGetStyle = 4;
				}
			}
			else
			{
				GetWindowRect(ghWnd, &rcMain);
				nGetStyle = 5;
			}

			_ASSERTE((mp_ConEmu->isIconic() == IsRectMinimized(rcMain)) || (changeFromWindowMode != wmNotChanging));

			// If rcMain still has invalid pos (got from iconic window placement)
			if (IsRectMinimized(rcMain))
			{
				// -- when we call "DefWindowProc(hWnd, WM_SYSCOMMAND, wParam, lParam)"
				// -- uxtheme lose wpl.rcNormalPosition at this point
				//_ASSERTE(FALSE && "wpl.rcNormalPosition contains 'iconic' size!");

				if (!IsRectMinimized(mrc_StoredNormalRect))
				{
					rcMain = mrc_StoredNormalRect;
					nGetStyle = 6;
				}
				else
				{
					_ASSERTE(FALSE && "mrc_StoredNormalRect was not saved yet, using GetDefaultRect()");

					rcMain = GetDefaultRect();
					nGetStyle = 7;
				}
			}

			// Если окно было свернуто из Maximized/FullScreen состояния
			if ((WindowMode == wmMaximized) || (WindowMode == wmFullScreen))
			{
				ConEmuRect t = (WindowMode == wmMaximized) ? CER_MAXIMIZED : CER_FULLSCREEN;
				rcMain = CalcRect(t, rcMain, t);
				nGetStyle += 10;
			}
		}
	}
	else
	{
		GetWindowRect(ghWnd, &rcMain);
	}

	if (tWhat == CER_MAIN)
	{
		return rcMain;
	}

	return CalcRect(tWhat, rcMain, CER_MAIN, pVCon);
}

// Для приблизительного расчета размеров - нужен только (размер главного окна)|(размер консоли)
// Для точного расчета размеров - требуется (размер главного окна) и (размер окна отрисовки) для корректировки
// на x64 возникают какие-то глюки с ",RECT rFrom,". Отладчик показывает мусор в rFrom,
// но тем не менее, после "RECT rc = rFrom;", rc получает правильные значения >:|
RECT CConEmuSize::CalcRect(enum ConEmuRect tWhat, const RECT &rFrom, enum ConEmuRect tFrom, CVirtualConsole* pVCon, enum ConEmuMargins tTabAction /*= CEM_TAB*/)
{
	_ASSERTE(this!=NULL);
	_ASSERTE(tWhat!=CER_MAINCLIENT && tWhat!=CER_TAB && tWhat!=CER_WORKSPACE);
	RECT rc = rFrom; // инициализация, если уж не получится...
	RECT rcShift = MakeRect(0,0);
	enum ConEmuRect tFromNow = tFrom;

	// tTabAction обязан включать и флаг CEM_TAB
	_ASSERTE((tTabAction & CEM_TAB)==CEM_TAB);

	WARNING("CEM_SCROLL вообще удалить?");
	WARNING("Проверить, чтобы DC нормально центрировалось после удаления CEM_BACK");

	//if (!pVCon)
	//	pVCon = mp_ConEmu->mp_ VActive;

	if (rFrom.left || rFrom.top)
	{
		if (rFrom.left >= rFrom.right || rFrom.top >= rFrom.bottom)
		{
			MBoxAssert(!(rFrom.left || rFrom.top));
		}
		else
		{
			// Это реально может произойти на втором мониторе.
			// Т.к. сюда передается Rect монитора,
			// полученный из CalcRect(CER_FULLSCREEN, MakeRect(0,0), CER_FULLSCREEN);
		}
	}

	switch (tFrom)
	{
		case CER_MAIN: // switch (tFrom)
			// Нужно отнять отступы рамки и заголовка!
		{
			// Это может быть, если сделали GetWindowRect для ghWnd, когда он isIconic!
			_ASSERTE(!IsRectMinimized(rc) && "Use CalcRect(CER_MAIN) instead of GetWindowRect() while IsIconic!");
			// Avoid wrong sized when resize is not finished yet
			SizeInfo temp(*static_cast<SizeInfo*>(this));
			temp.RequestSize(RectWidth(rFrom), RectHeight(rFrom));
			rc = temp.ClientRect();
			tFromNow = CER_MAINCLIENT;
		}
		break;
		// switch (tFrom)
		case CER_WORKSPACE:
		case CER_MAINCLIENT:
		{
			//
		}
		break;
		case CER_DC: // switch (tFrom)
		{
			// Размер окна отрисовки в пикселах!
			//MBoxAssert(!(rFrom.left || rFrom.top));
			TODO("DoubleView");

			switch (tWhat)
			{
				case CER_MAIN:
				{
					rcShift = CalcMargins(tTabAction|CEM_ALL_MARGINS);
					AddMargins(rc, rcShift, rcop_AddSize);
					WARNING("Неправильно получается при DoubleView");
					tFromNow = CER_MAINCLIENT;
				} break;
				case CER_MAINCLIENT:
				{
					rcShift = CalcMargins(tTabAction);
					AddMargins(rc, rcShift, rcop_AddSize);
					WARNING("Неправильно получается при DoubleView");
					tFromNow = CER_MAINCLIENT;
				} break;
				case CER_TAB:
				{
					_ASSERTE(tWhat!=CER_TAB); // вроде этого быть не должно?
					rcShift = CalcMargins(tTabAction);
					AddMargins(rc, rcShift, rcop_AddSize);
					WARNING("Неправильно получается при DoubleView");
					tFromNow = CER_MAINCLIENT;
				} break;
				case CER_WORKSPACE:
				{
					WARNING("Это tFrom. CER_WORKSPACE - не поддерживается (пока?)");
					_ASSERTE(tWhat!=CER_WORKSPACE);
				} break;
				case CER_BACK:
				{
					rcShift = CalcMargins(tTabAction|CEM_CLIENT_MARGINS);
					rc.top += rcShift.top; rc.bottom += rcShift.top;
					_ASSERTE(rcShift.left == 0 && rcShift.right == 0 && rcShift.bottom == 0);
					WARNING("Неправильно получается при DoubleView");
					tFromNow = CER_MAINCLIENT;
				} break;
				default:
					_ASSERTE(FALSE && "Unexpected tWhat");
					break;
			}
		}
		_ASSERTE(rc.right>rc.left && rc.bottom>rc.top);
		return rc;

		case CER_CONSOLE_ALL: // switch (tFrom)
		case CER_CONSOLE_CUR: // switch (tFrom)
		{
			// Тут считаем БЕЗОТНОСИТЕЛЬНО SplitScreen.
			// Т.к. вызов, например, может идти из интерфейса диалога "Settings" и CVConGroup::SetAllConsoleWindowsSize

			// Размер консоли в символах!
			//MBoxAssert(!(rFrom.left || rFrom.top));
			_ASSERTE(tWhat!=CER_CONSOLE_ALL && tWhat!=CER_CONSOLE_CUR);
			//if (gpSetCls->FontWidth()==0) {
			//    MBoxAssert(pVCon!=NULL);
			//    pVCon->InitDC(false, true); // инициализировать ширину шрифта по умолчанию
			//}
			// ЭТО размер окна отрисовки DC
			rc = MakeRect((rFrom.right-rFrom.left) * gpFontMgr->FontWidth(),
			              (rFrom.bottom-rFrom.top) * gpFontMgr->FontHeight());

			RECT rcScroll = CalcMargins(CEM_SCROLL|CEM_PAD);
			AddMargins(rc, rcScroll, rcop_AddSize);

			if (tWhat != CER_DC)
				rc = CalcRect(tWhat, rc, CER_DC, pVCon, tTabAction);

			// -- tFromNow = CER_BACK; -- менять не требуется, т.к. мы сразу на выход
		}
		_ASSERTE(rc.right>rc.left && rc.bottom>rc.top);
		return rc;

		case CER_FULLSCREEN: // switch (tFrom)
		case CER_MAXIMIZED: // switch (tFrom)
		case CER_RESTORE: // switch (tFrom)
		case CER_MONITOR: // switch (tFrom)
			// Например, таким способом можно получить размер развернутого окна на текущем мониторе
			// RECT rcMax = CalcRect(CER_MAXIMIZED, MakeRect(0,0), CER_MAXIMIZED);
		{
			switch (tFrom)
			{
			case CER_FULLSCREEN: // switch (tFrom)
			case CER_MAXIMIZED: // switch (tFrom)
				rcShift = CalcMargins(CEM_CAPTION);
				break;
			case CER_RESTORE: // switch (tFrom)
			case CER_MONITOR: // switch (tFrom)
				rcShift = CalcMargins(CEM_FRAMECAPTION);
				break;
			default:
				_ASSERTE((tFrom==tWhat) && "Unhandled tFrom/tWhat");
				rcShift = {};
			}
			AddMargins(rc, rcShift);
			tFromNow = CER_MAINCLIENT;
			break;
		}
		case CER_BACK: // switch (tFrom)
			break;
		default:
			_ASSERTE(tFrom==CER_MAINCLIENT); // для отладки, сюда вроде попадать не должны
			break;
	};

	//// Теперь rc должен соответствовать CER_MAINCLIENT
	//RECT rcAddShift = MakeRect(0,0);

	// если мы дошли сюда - значит tFrom==CER_MAINCLIENT

	switch (tWhat)
	{
		case CER_TAB: // switch (tWhat)
		{
			_ASSERTE(tFrom==CER_MAINCLIENT);

			#ifdef _DEBUG
			RECT rcNew = SizeInfo::RebarRect();
			RECT rcTest = rc;
			if (gpSet->nTabsLocation == 1) {
				int nStatusHeight = gpSet->isStatusBarShow ? gpSet->StatusBarHeight() : 0;
				rcTest.top = rcTest.bottom - nStatusHeight - SizeInfo::GetDefaultTabbarHeight();
			} else
				rcTest.bottom = rcTest.top + SizeInfo::GetDefaultTabbarHeight();
			_ASSERTE(rcNew == rcTest);
			#endif

			rc = SizeInfo::RebarRect();
		} break;
		case CER_WORKSPACE: // switch (tWhat)
		{
			rcShift = CalcMargins(tTabAction|CEM_CLIENT_MARGINS/*|CEM_SCROLL*/);
			AddMargins(rc, rcShift);
		} break;
		case CER_BACK: // switch (tWhat)
		case CER_SCROLL: // switch (tWhat)
		case CER_DC: // switch (tWhat)
		case CER_CONSOLE_ALL: // switch (tWhat)
		case CER_CONSOLE_CUR: // switch (tWhat)
		case CER_CONSOLE_NTVDMOFF: // switch (tWhat)
		{
			RECT rcRet = CVConGroup::CalcRect(tWhat, rc, tFromNow, pVCon, tTabAction);
			_ASSERTE(rcRet.right>rcRet.left && rcRet.bottom>rcRet.top);
			return rcRet;
		} break;

		case CER_MONITOR:    // switch (tWhat)
		case CER_FULLSCREEN: // switch (tWhat)
		case CER_MAXIMIZED:  // switch (tWhat)
		case CER_RESTORE:    // switch (tWhat)
			//case CER_CORRECTED:
		{
			//HMONITOR hMonitor = NULL;
			_ASSERTE(tFrom==tWhat || tFrom==CER_MAIN);

			DEBUGTEST(bool bIconic = isIconic());

			//if (tWhat != CER_CORRECTED)
			//    tFrom = tWhat;
			auto mi = (tFrom == CER_MAIN) ? NearestMonitorInfo(rFrom) : NearestMonitorInfo(NULL);

			{
				//switch (tFrom) // --было
				switch (tWhat)
				{
					case CER_MONITOR:
					{
						rc = mi.mi.rcWork;

					} break;
					case CER_FULLSCREEN:
					{
						rc = mi.mi.rcMonitor;

					} break;
					case CER_MAXIMIZED:
					{
						rc = mi.mi.rcWork;
						if (mi.isTaskbarHidden)
							AddMargins(rc, mi.rcTaskbarExcess);
						RECT rcFrame = CalcMargins(CEM_FRAMEONLY);
						// Скорректируем размер окна до видимого на мониторе (рамка при максимизации уезжает за пределы экрана)
						rc.left -= rcFrame.left;
						rc.right += rcFrame.right;
						rc.top -= rcFrame.top;
						rc.bottom += rcFrame.bottom;

					} break;
					case CER_RESTORE:
					{
						RECT rcNormal = {0};
						if (mp_ConEmu)
						{
							_ASSERTE(!IsRectMinimized(mrc_StoredNormalRect));
							rcNormal = mrc_StoredNormalRect;
						}
						int w = rcNormal.right - rcNormal.left;
						int h = rcNormal.bottom - rcNormal.top;
						if ((w > 0) && (h > 0))
						{
							rc = rcNormal;

							// Если после последней максимизации была изменена
							// конфигурация мониторов - нужно поправить видимую область
							if (((rc.right + 30) <= mi.mi.rcWork.left)
								|| ((rc.left + 30) >= mi.mi.rcWork.right))
							{
								rc.left = mi.mi.rcWork.left; rc.right = rc.left + w;
							}
							if (((rc.bottom + 30) <= mi.mi.rcWork.top)
								|| ((rc.top + 30) >= mi.mi.rcWork.bottom))
							{
								rc.top = mi.mi.rcWork.top; rc.bottom = rc.top + h;
							}
						}
						else
						{
							rc = mi.mi.rcWork;
						}
					} break;
					default:
					{
						_ASSERTE(tWhat==CER_FULLSCREEN || tWhat==CER_MAXIMIZED);
					}
				}
			}

			_ASSERTE(rc.right>rc.left && rc.bottom>rc.top);
			return rc;
		} break;
		default:
			break;
	}

	_ASSERTE(rc.right>rc.left && rc.bottom>rc.top);
	return rc; // Посчитали, возвращаем
}

/*!!!static!!*/
void CConEmuSize::AddMargins(RECT& rc, const RECT& rcAddShift, RectOperations rect_op /*= rcop_Shrink*/)
{
	MBoxAssert((rect_op==rcop_MathAdd || rect_op==rcop_MathSub) || (rcAddShift.left>=0 && rcAddShift.top>=0 && rcAddShift.right>=0 && rcAddShift.bottom>=0));

	// The code below has a lot of "if"s, they are intended for debugging purposes (breakpoints)

	switch (rect_op)
	{
	case rcop_Shrink:
		if (rcAddShift.left)
			rc.left += rcAddShift.left;

		if (rcAddShift.top)
			rc.top += rcAddShift.top;

		if (rcAddShift.right)
			rc.right -= rcAddShift.right;

		if (rcAddShift.bottom)
			rc.bottom -= rcAddShift.bottom;

		break;

	case rcop_Enlarge:
		if (rcAddShift.left)
			rc.left -= rcAddShift.left;

		if (rcAddShift.top)
			rc.top -= rcAddShift.top;

		if (rcAddShift.right)
			rc.right += rcAddShift.right;

		if (rcAddShift.bottom)
			rc.bottom += rcAddShift.bottom;

		break;

	case rcop_MathAdd:
		if (rcAddShift.left)
			rc.left += rcAddShift.left;

		if (rcAddShift.top)
			rc.top += rcAddShift.top;

		if (rcAddShift.right)
			rc.right += rcAddShift.right;

		if (rcAddShift.bottom)
			rc.bottom += rcAddShift.bottom;

		break;

	case rcop_MathSub:
		if (rcAddShift.left)
			rc.left -= rcAddShift.left;

		if (rcAddShift.top)
			rc.top -= rcAddShift.top;

		if (rcAddShift.right)
			rc.right -= rcAddShift.right;

		if (rcAddShift.bottom)
			rc.bottom -= rcAddShift.bottom;

		break;

	case rcop_AddSize:
		// change only right and bottom edges
		if (rcAddShift.right || rcAddShift.left)
			rc.right += rcAddShift.right + rcAddShift.left;

		if (rcAddShift.bottom || rcAddShift.top)
			rc.bottom += rcAddShift.bottom + rcAddShift.top;

		break;
	}
}

// bCells==true - вернуть расчетный консольный размер, bCells==false - размер в пикселях (только консоль, без рамок-отступов-табов-прочая)
// pSizeW и pSizeH указываются если нужно установить конкретные размеры
// hMon может быть указан при переносе окна между мониторами например
SIZE CConEmuSize::GetDefaultSize(bool bCells, const CESize* pSizeW /*= NULL*/, const CESize* pSizeH /*= NULL*/, HMONITOR hMon /*= NULL*/)
{
	NestedCallAssert(1);

	//WARNING! Function must NOT call CalcRect to avoid nested loops!

	_ASSERTE(mp_ConEmu->mp_Inside == NULL); // Must not be called in "Inside"?

	SetRequestedMonitor(hMon ? hMon : mh_RequestedMonitor);

	SIZE sz = {80,25}; // This has no matter unless fatal errors

	CESize sizeW = {WndWidth.Raw};
	if (pSizeW && pSizeW->Value)
		sizeW.Set(true, pSizeW->Style, pSizeW->Value);
	CESize sizeH = {WndHeight.Raw};
	if (pSizeH && pSizeH->Value)
		sizeH.Set(false, pSizeH->Style, pSizeH->Value);

	if (bCells && (sizeW.Style == ss_Standard) && (sizeH.Style == ss_Standard))
	{
		// If settings was set to exact size in cells - no need to evaluate the size in pixels
		sz.cx = sizeW.Value; sz.cy = sizeH.Value;
		return sz;
	}


	// Font size must be initialized already!
	int nFontWidth = gpFontMgr->FontWidth();
	int nFontHeight = gpFontMgr->FontHeight();
	if ((nFontWidth <= 0) || (nFontHeight <= 0))
	{
		Assert(nFontWidth>0 && nFontHeight>0);
		return sz;
	}


	COORD conSize = {80, 25}; // Used only with cell-sized options
	wchar_t szInfo[80];


	RECT rcFrameMargin = CalcMargins(CEM_FRAMECAPTION|CEM_STATUS);
	int nFrameX = rcFrameMargin.left + rcFrameMargin.right;
	int nFrameY = rcFrameMargin.top + rcFrameMargin.bottom;

	// When tabs as visible - add their sized to evaluate proper console size
	int nTabsX = 0, nTabsY = 0;
	bool bTabs = mp_ConEmu->isTabsShown();
	if (bTabs)
	{
		RECT temp = CalcMargins_TabBar(CEM_TAB);
		nTabsX = temp.left + temp.right;
		nTabsY = temp.top + temp.bottom;
	}
	_ASSERTE((gpSet->isTabs != 1) || (nTabsY > 0));

	// Consoles splitters, pads and scrolling
	RECT rcVConPad = CalcMargins(CEM_SCROLL|CEM_PAD);
	if (CVConGroup::isVConExists(0))
	{
		SIZE Splits = {};
		RECT rcMin = CVConGroup::AllTextRect(&Splits, true);
		// If any split exists
		if (Splits.cx > 0)
		{
			rcVConPad.left *= (Splits.cx+1);
			rcVConPad.right *= (Splits.cx+1);
		}
		if (Splits.cy > 0)
		{
			rcVConPad.top *= (Splits.cy+1);
			rcVConPad.bottom *= (Splits.cy+1);
		}
		// Splitter bars?
		if ((Splits.cx > 0) && (gpSet->nSplitWidth > 0))
		{
			rcVConPad.right += Splits.cx * gpSetCls->EvalSize(gpSet->nSplitWidth, esf_Horizontal|esf_CanUseDpi);
		}
		if ((Splits.cy > 0) && (gpSet->nSplitHeight > 0))
		{
			rcVConPad.bottom += Splits.cy * gpSetCls->EvalSize(gpSet->nSplitHeight, esf_Vertical|esf_CanUseDpi);
		}
	}
	int nConX = (rcVConPad.right + rcVConPad.left), nConY = (rcVConPad.bottom + rcVConPad.top);


	// Final padding
	int nShiftX = nFrameX + nTabsX + nConX;
	int nShiftY = nFrameY + nTabsY + nConY;


	ConEmuWindowMode wmCurMode = GetWindowMode();

	// Информация по монитору нам нужна только если размеры заданы в процентах
	MONITORINFO mi = {sizeof(mi)};
	if ((sizeW.Style == ss_Percents || sizeH.Style == ss_Percents)
		|| gpSet->isQuakeStyle
		|| (wmCurMode == wmMaximized || wmCurMode == wmFullScreen))
	{
		if (hMon != NULL)
		{
			GetMonitorInfoSafe(hMon, mi);
		}
		// по ghWnd - монитор не ищем (окно может быть свернуто), только по координатам
		else
		{
			hMon = FindInitialMonitor(&mi);
		}

		// was gh-472: One more fix for custom frame width (Quake) and ‘gaps’ (36d34ba)
		if (wmCurMode == wmNormal)
		{
			// Was CEM_FRAMECAPTION, but if user set 100% for Height,
			// the window become larger than working area on Title bar height
			RECT rcInvisible = CalcMargins_InvisibleFrame();
			AddMargins(mi.rcWork, rcInvisible, rcop_Enlarge);
		}
	}

	int nPixelWidth = 0, nPixelHeight = 0;
	{	// Calculation begins

	// We store size of whole window to avoid ambiguity

	// Well, there is some ambiguity in size storing.
	// * When user specify cells count on the "Size & Pos" page,
	//   they await this cells count in the created console
	// * When they set percentage of monitor working area,
	//   window size must not be larger than 100%, therefore percentage
	//   is treated as whole window size, but not a VCons size.
	// * Same with pixels. This is expected whole *window* size,
	//   VCon size is derivative and will be smaller...

	// Calculate width
	if (sizeW.Style == ss_Pixels)
	{
		// Read comment above about size storing
		nPixelWidth = sizeW.Value;
	}
	else if (sizeW.Style == ss_Percents)
	{
		// Read comment above about size storing
		nPixelWidth = ((mi.rcWork.right - mi.rcWork.left) * sizeW.Value / 100);
	}
	else if (sizeW.Value > 0)
	{
		// It's a cells count
		conSize.X = sizeW.Value;
	}

	if (nPixelWidth <= 0)
	{
		nPixelWidth = conSize.X * nFontWidth + (bCells ? 0 : nShiftX);
	}

	// Calculate height
	if (sizeH.Style == ss_Pixels)
	{
		// Read comment above about size storing
		nPixelHeight = sizeH.Value;
	}
	else if (sizeH.Style == ss_Percents)
	{
		// Read comment above about size storing
		nPixelHeight = ((mi.rcWork.bottom - mi.rcWork.top) * sizeH.Value / 100);
	}
	else if (sizeH.Value > 0)
	{
		// It's a cells count
		conSize.Y = sizeH.Value;
	}

	if (nPixelHeight <= 0)
	{
		nPixelHeight = conSize.Y * nFontHeight + (bCells ? 0 : nShiftY);
	}

	}	// Calculation ends


	// Quake size/pos will be corrected later in CheckQuakeRect method
	#ifdef _DEBUG
	// Maximized/fullscreen size must not exceed display rect
	if (!gpSet->isQuakeStyle && (wmCurMode == wmMaximized || wmCurMode == wmFullScreen))
	{
		if (mi.cbSize)
		{
			const RECT rcMon = (wmCurMode == wmFullScreen) ? mi.rcMonitor : mi.rcWork;
			int nMaxWidth  = rcMon.right - rcMon.left, nMaxHeight = rcMon.bottom - rcMon.top;
			// Maximized/Fullscreen - frame may come out of working area
			RECT rcFrameOnly = CalcMargins(CEM_FRAMEONLY);
			_ASSERTE(nFrameX>=0 && nFrameY>=0 && rcFrameOnly.left>=0 && rcFrameOnly.top>=0 && rcFrameOnly.right>=0 && rcFrameOnly.bottom>=0);
			// 131029 - Was nFrameX&nFrameY, which was less then required. Quake goes of the screen
			nMaxWidth += (rcFrameOnly.left + rcFrameOnly.right) - nShiftX;
			nMaxHeight += (rcFrameOnly.top + rcFrameOnly.bottom) - nShiftY;
			// Check it
			_ASSERTE((nPixelWidth <= nMaxWidth) && (nPixelHeight <= nMaxHeight));
		}
		else
		{
			_ASSERTE(mi.cbSize!=0);
		}
	}
	#endif


	// Prepare result
	if (bCells)
	{
		// nPixelXXX - VCon size
		sz.cx = nPixelWidth / nFontWidth;
		sz.cy = nPixelHeight / nFontHeight;
		// Debug purposes
		swprintf_c(szInfo, L"GetDefaultSize: {%i,%i} cells", sz.cx, sz.cy);
	}
	else
	{
		// nPixelXXX - whole window size
		sz.cx = nPixelWidth;
		sz.cy = nPixelHeight;
		// Debug purposes
		swprintf_c(szInfo, L"GetDefaultSize: {%i,%i} pixels", sz.cx, sz.cy);
	}
	DEBUGSTRSIZE(szInfo);

	return sz;
}

HMONITOR CConEmuSize::FindInitialMonitor(MONITORINFO* pmi /*= NULL*/)
{
	// Too large or too small values may cause wrong monitor detection if on the edge of screen
	const int kSizeDefault = 100;
	int iWidth, iHeight;

	TODO("CConEmuSize::GetInitialDpi -> ss_Percents");

	switch (WndWidth.Style)
	{
	case ss_Standard:
		iWidth = WndWidth.Value * (gpSet->FontSizeX ? gpSet->FontSizeX : (gpSet->FontSizeY / 2));
		break;
	case ss_Percents:
		iWidth = WndWidth.Value;
		break;
	default:
		iWidth = kSizeDefault;
	}

	switch (WndHeight.Style)
	{
	case ss_Standard:
		iHeight = WndHeight.Value * gpSet->FontSizeY;
		break;
	case ss_Percents:
		iHeight = WndHeight.Value;
		break;
	default:
		iHeight = kSizeDefault;
	}

	// No need to get strict coordinates, just find the default monitor
	RECT rcDef = {WndPos.x, WndPos.y, WndPos.x + iWidth, WndPos.y + iHeight};

	MONITORINFO mi = {sizeof(mi)};
	HMONITOR hMon = GetNearestMonitorInfo(&mi, NULL, &rcDef);

	if (pmi)
		*pmi = mi;

	return hMon;
}

int CConEmuSize::GetInitialDpi(DpiValue* pDpi)
{
	DpiValue dpi;

	mh_RequestedMonitor = FindInitialMonitor();

	int dpiY = CDpiAware::QueryDpiForMonitor(mh_RequestedMonitor, &dpi);

	SizeInfo::RequestDpi(dpi);

	if (pDpi)
		pDpi->SetDpi(dpi);

	return dpiY;
}

bool CConEmuSize::SetWindowPosSize(LPCWSTR asX, LPCWSTR asY, LPCWSTR asW, LPCWSTR asH)
{
	if (mp_ConEmu->mp_Inside)
	{
		_ASSERTE(mp_ConEmu->mp_Inside == NULL);
		return false;
	}

	int newX = 0, newY = 0;
	CESize newW, newH;

	HWND hDlg = gpSetCls->GetPage(thi_SizePos); // ‘Size & Pos’ settings page

	if (!asX || !*asX || !IntFromString(newX, asX))
		newX = WndPos.x;

	if (!asY || !*asY || !IntFromString(newY, asY))
		newY = WndPos.y;

	if (!asW || !*asW || !newW.SetFromString(true, asW))
		newW.Raw = WndWidth.Raw;

	if (!asH || !*asH || !newH.SetFromString(false, asH))
		newH.Raw = WndHeight.Raw;

	// Чтобы GetDefaultRect сработал правильно - сразу обновим значения
	if (!gpSet->wndCascade)
	{
		WndPos.x = newX;
		if (!gpSet->isQuakeStyle)
			WndPos.y = newY;
	}
	gpSet->_wndX = newX;
	if (!gpSet->isQuakeStyle)
		gpSet->_wndY = newY;
	mp_ConEmu->WndWidth.Set(true, newW.Style, newW.Value);
	mp_ConEmu->WndHeight.Set(false, newH.Style, newH.Value);
	gpSet->wndWidth.Set(true, newW.Style, newW.Value);
	gpSet->wndHeight.Set(true, newH.Style, newH.Value);

	if (gpSet->isQuakeStyle)
	{
		if (hDlg)
			SetFocus(GetDlgItem(hDlg, tWndWidth));
		RECT rcQuake = mp_ConEmu->GetDefaultRect();
		// And size/move!
		setWindowPos(NULL, rcQuake.left, rcQuake.top, rcQuake.right-rcQuake.left, rcQuake.bottom-rcQuake.top, SWP_NOZORDER);
	}
	else
	{
		if (hDlg)
			SetFocus(GetDlgItem(hDlg, rNormal));

		if (mp_ConEmu->isZoomed() || mp_ConEmu->isIconic() || mp_ConEmu->isFullScreen())
			mp_ConEmu->SetWindowMode(wmNormal);

		POINT wndPos = RealPosFromVisual(newX, newY);

		setWindowPos(NULL, wndPos.x, wndPos.y, 0,0, SWP_NOSIZE|SWP_NOZORDER);

		// Установить размер
		mp_ConEmu->SizeWindow(newW, newH);

		setWindowPos(NULL, wndPos.x, wndPos.y, 0,0, SWP_NOSIZE|SWP_NOZORDER);
	}

	// Запомнить "идеальный" размер окна, выбранный пользователем
	mp_ConEmu->StoreNormalRect(NULL);

	return true;
}

// Used while parsing command line switches
void CConEmuSize::SetWindowPosSizeParam(wchar_t acType, LPCWSTR asValue)
{
	wchar_t* pch;
	switch (acType)
	{
	case L'X':
		WndPos.x = wcstol(asValue, &pch, 10);
		if (gpSet->isUseCurrentSizePos)
			gpSet->_wndX = WndPos.x;
		break;
	case L'Y':
		WndPos.y = wcstol(asValue, &pch, 10);
		if (gpSet->isUseCurrentSizePos)
			gpSet->_wndY = WndPos.y;
		break;
	case L'W':
		if (WndWidth.SetFromString(true, asValue) && gpSet->isUseCurrentSizePos)
			gpSet->wndWidth.Raw = WndWidth.Raw;
		break;
	case L'H':
		if (WndHeight.SetFromString(false, asValue) && gpSet->isUseCurrentSizePos)
			gpSet->wndHeight.Raw = WndHeight.Raw;
		break;
	}
}

bool CConEmuSize::CheckQuakeRect(LPRECT prcWnd)
{
	RECT rcDesired = *prcWnd;
	RECT rcReal = *prcWnd;
	if (ghWnd)
		GetWindowRect(ghWnd, &rcReal);
	MONITORINFO mi = {sizeof(mi)};
	HMONITOR hMon = GetNearestMonitorInfo(&mi, NULL, &rcDesired);

	bool bChange = false;

	const RECT rcInvidisbleFrame = CalcMargins(CEM_INVISIBLE_FRAME);
	const RECT rcFrameOnly = CalcMargins(CEM_FRAMECAPTION);
	const auto& iLeftShift = rcInvidisbleFrame.left;
	const auto& iRightShift = rcInvidisbleFrame.right;

	// Если успешно - подгоняем по экрану
	if (mi.rcWork.right > mi.rcWork.left)
	{
		int nWidth = prcWnd->right - prcWnd->left;
		int nHeight = prcWnd->bottom - prcWnd->top;

		switch (gpSet->_WindowMode)
		{
			case wmMaximized:
			{
				prcWnd->left = mi.rcWork.left - rcFrameOnly.left;
				prcWnd->right = mi.rcWork.right + rcFrameOnly.right;
				prcWnd->top = mi.rcWork.top - rcFrameOnly.top;
				prcWnd->bottom = mi.rcWork.bottom + rcFrameOnly.bottom;

				bChange = true;
				break;
			}

			case wmFullScreen:
			{
				prcWnd->left = mi.rcMonitor.left - rcFrameOnly.left;
				prcWnd->right = mi.rcMonitor.right + rcFrameOnly.right;
				prcWnd->top = mi.rcMonitor.top - rcFrameOnly.top;
				prcWnd->bottom = mi.rcMonitor.bottom + rcFrameOnly.bottom;

				bChange = true;
				break;
			}

			case wmNormal:
			{
				// If "Fixed" mode was selected, let user set the left corner coordinate
				POINT pt = RealPosFromVisual(WndPos.x, WndPos.y);
				if (!gpSet->wndCascade)
					prcWnd->left = std::max((mi.rcWork.left - iLeftShift),std::min(pt.x,(mi.rcWork.right - nWidth + iRightShift)));
				else // Иначе - центрируется по монитору
					prcWnd->left = std::max((mi.rcWork.left - iLeftShift),((mi.rcWork.left + mi.rcWork.right - nWidth) / 2));
				prcWnd->right = std::min((mi.rcWork.right + iRightShift),(prcWnd->left + nWidth));
				prcWnd->top = mi.rcWork.top - rcFrameOnly.top;
				prcWnd->bottom = prcWnd->top + nHeight;

				bChange = true;
				break;
			}
		}
	}

	if (bChange)
	{
		ptFullScreenSize.x = prcWnd->right - prcWnd->left + 1;
		ptFullScreenSize.y = mi.rcMonitor.bottom - mi.rcMonitor.top + (rcFrameOnly.bottom + rcFrameOnly.top);

		DEBUGTEST(RECT rcCon = CalcRect(CER_CONSOLE_ALL, *prcWnd, CER_MAIN));
		//if (rcCon.right)
		//	this->wndWidth = rcCon.right;

		// Issue 1929: Quake position changed unexpectedly after Win+D
		//this->wndX = prcWnd->left;
		//this->wndY = prcWnd->top;
	}

	return bChange;
}

POINT CConEmuSize::VisualPosFromReal(const int x, const int y)
{
	RECT rcInvisible = CalcMargins_InvisibleFrame();
	MBoxAssert(rcInvisible.left>=0 && rcInvisible.top>=0);
	return MakePoint(x + rcInvisible.left, y + rcInvisible.top);
}

POINT CConEmuSize::RealPosFromVisual(const int x, const int y)
{
	RECT rcInvisible = CalcMargins_InvisibleFrame();
	MBoxAssert(rcInvisible.left>=0 && rcInvisible.top>=0);
	return MakePoint(x - rcInvisible.left, y - rcInvisible.top);
}

// Вызывается при старте программы, для вычисления mrc_Ideal - размера окна по умолчанию
RECT CConEmuSize::GetDefaultRect()
{
	NestedCallAssert(2);

	RECT rcWnd = {};

	if (mp_ConEmu->mp_Inside)
	{
		if (mp_ConEmu->mp_Inside->GetInsideRect(&rcWnd))
		{
			WindowMode = wmNormal;

			// #DPI What if parent window covers several monitors?
			SetRequestedMonitor(MonitorFromWindow(mp_ConEmu->mp_Inside->mh_InsideParentWND, MONITOR_DEFAULTTONEAREST));
			if (!IsRectEmpty(&rcWnd))
			{
				if (!mp_ConEmu->isCalculated())
					mp_ConEmu->RequestRect(rcWnd);
				this->WndPos = VisualPosFromReal(rcWnd.left, rcWnd.top);
				RECT rcCon = CalcRect(CER_CONSOLE_ALL, rcWnd, CER_MAIN);
				// In the "Inside" mode we are interested only in "cells"
				this->WndWidth.Set(true, ss_Standard, rcCon.right);
				this->WndHeight.Set(false, ss_Standard, rcCon.bottom);
			}

			OnMoving(&rcWnd);

			return rcWnd;
		}
	}

	if (gpSet->isQuakeStyle && !IsRectEmpty(&m_QuakePrevSize.PreSlidedSize))
	{
		rcWnd = m_QuakePrevSize.PreSlidedSize;
		return rcWnd;
	}

	// Расчет размеров
	SIZE szConPixels = GetDefaultSize(false);

	POINT wndPos = RealPosFromVisual(WndPos.x, WndPos.y);

	// >> rcWnd
	rcWnd.left = wndPos.x;
	rcWnd.top = wndPos.y;
	rcWnd.right = wndPos.x + szConPixels.cx;
	rcWnd.bottom = wndPos.y + szConPixels.cy;

	// Go
	if (gpSet->isQuakeStyle)
	{
		CheckQuakeRect(&rcWnd);
	}

	OnMoving(&rcWnd);

	// Debug purposes
	wchar_t szInfo[100];
	swprintf_c(szInfo, L"GetDefaultRect: {%i,%i} (%ix%i)", rcWnd.left, rcWnd.top, LOGRECTSIZE(rcWnd));
	DEBUGSTRSIZE(szInfo);

	return rcWnd;
}

void CConEmuSize::ReloadMonitorInfo()
{
	_ASSERTEX(isMainThread());

	struct Invoke
	{
		static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
		{
			CConEmuSize* p = reinterpret_cast<CConEmuSize*>(dwData);
			MonitorInfoCache mi = {hMonitor};
			mi.mi.cbSize = sizeof(mi.mi);
			GetMonitorInfoSafe(hMonitor, mi.mi);
			p->monitors.push_back(mi);
			return TRUE;
		};
	};

	MSectionLockSimple locks; locks.Lock(&mcs_monitors);

	bool is_log = LogString(L"ReloadMonitorInfo");

	monitors.clear();
	EnumDisplayMonitors(NULL, NULL, Invoke::MonitorEnumProc, reinterpret_cast<LPARAM>(this));
	if (monitors.empty())
	{
		MonitorInfoCache mi = {};
		mi.hMon = GetPrimaryMonitorInfo(&mi.mi);
		monitors.push_back(mi);
	}

	// Use WS_OVERLAPPEDWINDOW instead of GetWindowStyle(), we need to know frame width for "normal" window,
	// to emulate it in GetSelfFrameWidth() even if our window has no WS_THICKFRAME
	const DWORD style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_OVERLAPPEDWINDOW;
	const DWORD styleNoCaption = WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_OVERLAPPED | WS_THICKFRAME;
	const DWORD exStyle = WS_EX_LAYERED;

	int std_frame_width = GetSystemMetrics(SM_CXFRAME);
	for (INT_PTR i = 0; i < monitors.size(); ++i)
	{
		auto& mi = monitors[i];

		// default frame width (it must be corrected after AdjustWindowRectExForDpi)
		mi.withCaption.VisibleFrameWidth = mi.withCaption.FrameWidth = std_frame_width;
		mi.noCaption.VisibleFrameWidth = mi.noCaption.FrameWidth = std_frame_width;

		// Per-monitor DPI
		DpiValue dpi;
		CDpiAware::QueryDpiForMonitor(mi.hMon, &dpi);
		mi.Xdpi = dpi.Xdpi;
		mi.Ydpi = dpi.Ydpi;

		const int nTestWidth = 100, nTestHeight = 100;
		RECT rcTest = MakeRect(nTestWidth,nTestHeight);
		if (mp_ConEmu->AdjustWindowRectExForDpi(&rcTest, style, FALSE, exStyle, dpi.Ydpi))
		{
			RECT& rc = mi.withCaption.FrameMargins;
			rc.left = -rcTest.left;
			rc.right = rcTest.right - nTestWidth;
			rc.bottom = rcTest.bottom - nTestHeight;
			rc.top = -rcTest.top;
			mi.withCaption.VisibleFrameWidth = mi.withCaption.FrameWidth = rc.left;
		}
		rcTest = MakeRect(nTestWidth,nTestHeight);
		if (mp_ConEmu->AdjustWindowRectExForDpi(&rcTest, styleNoCaption, FALSE, exStyle, dpi.Ydpi))
		{
			RECT& rc = mi.noCaption.FrameMargins;
			rc.left = -rcTest.left;
			rc.right = rcTest.right - nTestWidth;
			rc.bottom = rcTest.bottom - nTestHeight;
			rc.top = -rcTest.top;
			mi.noCaption.VisibleFrameWidth = mi.noCaption.FrameWidth = rc.left;
		}

		// Issue 828: When taskbar is auto-hidden
		// Is task-bar found on current monitor?
		mi.rcTaskbarExcess = RECT{};
		if ((mi.isTaskbarHidden = IsTaskbarAutoHidden(&mi.mi.rcMonitor, (PUINT)&mi.taskbarLocation, &mi.hTaskbar)))
		{
			const int pixel = 2; // #TaskBar Do the same for standard window frame+caption, coz TaskBar obscures portion of ConEmu
			switch (mi.taskbarLocation)
			{
				case ABE_LEFT:   mi.rcTaskbarExcess.left   = pixel; break;
				case ABE_RIGHT:  mi.rcTaskbarExcess.right  = pixel; break;
				case ABE_TOP:    mi.rcTaskbarExcess.top    = pixel; break;
				case ABE_BOTTOM: mi.rcTaskbarExcess.bottom = pixel; break;
			}
		}
	}

	// *** Query information about invisible parts of Win10 frame ***
	if (IsWin10())
	{
		struct Win10Invoke
		{
			static LRESULT CALLBACK FrameTestProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
			{
				LRESULT result = 0;

				if (!gnWndSetHotkey && (messg == WM_SETHOTKEY))
				{
					gnWndSetHotkey = wParam;
				}

				switch (messg)
				{
				case WM_PAINT:
					{
						PAINTSTRUCT ps = {};
						BeginPaint(hWnd, &ps);
						EndPaint(hWnd, &ps);
					}
					result = 0;
					break;

				case WM_ERASEBKGND:
					result = TRUE;
					break;

				default:
					result = DefWindowProc(hWnd, messg, wParam, lParam);
				}

				return result;
			};
		};

		const wchar_t szFrameClass[] = L"ConEmuWin10FrameCheck";
		WNDCLASSEX wc = {sizeof(WNDCLASSEX), 0, Win10Invoke::FrameTestProc, 0, 0,
						 g_hInstance, hClassIcon, LoadCursor(NULL, IDC_ARROW),
						 NULL /*(HBRUSH)COLOR_BACKGROUND*/,
						 NULL, szFrameClass, hClassIconSm
						};// | CS_DROPSHADOW

		if (RegisterClassEx(&wc))
		{
			HWND hFrame = CreateWindowEx(exStyle, szFrameClass, L"", style, 100, 100, 400, 200, NULL, NULL, (HINSTANCE)g_hInstance, NULL);
			if (hFrame)
			{
				wchar_t szInfo[140];

				SetLayeredWindowAttributes(hFrame, 0, 0, LWA_ALPHA);
				ShowWindow(hFrame, SW_SHOWNA);

				for (int c = 0; c <= 1; ++c)
				{
					SetWindowLong(hFrame, GWL_STYLE, !c ? style : styleNoCaption);

					for (INT_PTR i = 0; i < monitors.size(); ++i)
					{
						MonitorInfoCache& mi = monitors[i];
						FrameInfoCache& fi = !c ? mi.withCaption : mi.noCaption;
						const RECT& rc = mi.mi.rcWork;

						MoveWindow(hFrame, rc.left+RectWidth(rc)/4, rc.top+RectHeight(rc)/4, RectWidth(rc)/2, RectHeight(rc)/2, FALSE);

						RECT rcReal = {}, rcVisible = {};
						if (GetWindowRect(hFrame, &rcReal)
							&& SUCCEEDED(mp_ConEmu->DwmGetWindowAttribute(hFrame, DWMWA_EXTENDED_FRAME_BOUNDS, &rcVisible, sizeof(rcVisible))))
						{
							// rcVisible is expected to be smaller than rcReal
							if (rcVisible.left > rcReal.left && rcVisible.right < rcReal.right
								&& rcVisible.right > rcVisible.left && rcVisible.bottom > rcVisible.top
								&& rcVisible.top >= rcReal.top && rcVisible.bottom <= rcReal.bottom)
							{
								fi.Win10Stealthy = MakeRect(
										(rcVisible.left - rcReal.left),
										(rcVisible.top - rcReal.top),
										(rcReal.right - rcVisible.right),
										(rcReal.bottom - rcVisible.bottom)
									);
								const RECT& rc = fi.Win10Stealthy;
								swprintf_c(szInfo, L"  DWMWA_EXTENDED_FRAME_BOUNDS Visible={%i,%i}-{%i,%i} Real={%i,%i}-{%i,%i} Diff={%i,%i}-{%i,%i}",
									LOGRECTCOORDS(rcVisible), LOGRECTCOORDS(rcReal), LOGRECTCOORDS(rc));

								#if 0
								// Correct client area shift for caption-less modes!
								if (c)
								{
									// otherwise we get a 7 pixel gap from the visible top-edge-frame to client area start
									fi.FrameMargins.top = std::max<int>(1, fi.FrameMargins.top - fi.Win10Stealthy.bottom);
								}
								#endif
							}
							else
							{
								_ASSERTE(rcVisible.left > rcReal.left && rcVisible.right < rcReal.right);
								_ASSERTE(rcVisible.right > rcVisible.left && rcVisible.bottom > rcVisible.top);
								_ASSERTE(rcVisible.top >= rcReal.top && rcVisible.bottom <= rcReal.bottom);
								swprintf_c(szInfo, L"  DWMWA_EXTENDED_FRAME_BOUNDS {FAIL} Visible={%i,%i}-{%i,%i} Real={%i,%i}-{%i,%i} Diff={%i,%i}-{%i,%i}",
									LOGRECTCOORDS(rcVisible), LOGRECTCOORDS(rcReal),
									(rcVisible.left - rcReal.left), (rcVisible.top - rcReal.top), (rcReal.right - rcVisible.right), (rcReal.bottom - rcVisible.bottom));
							}

							// Debug purposes
							LogString(szInfo);
						}
					}
				}
				DestroyWindow(hFrame);
			}
			UnregisterClass(szFrameClass, g_hInstance);
		}
	}

	if (IsWin10())
	{
		for (INT_PTR i = 0; i < monitors.size(); ++i)
		{
			MonitorInfoCache& mi = monitors[i];
			for (int c = 0; c <= 1; ++c)
			{
				FrameInfoCache& fi = !c ? mi.withCaption : mi.noCaption;
				// Default values for Win10 if for unknown reason we failed to retrieve them
				if (!fi.Win10Stealthy.left)
				{
					// We shall not get here, WinAPI MUST return proper values
					_ASSERTE(fi.Win10Stealthy.left && fi.Win10Stealthy.right && fi.Win10Stealthy.bottom);
					const int def_val = !c ? 7 : 6; // 6 for caption-less
					fi.Win10Stealthy = MakeRect(def_val, 0, def_val, def_val);
				}
				// Correct FrameWidth taking into account hidden frame parts
				fi.VisibleFrameWidth = std::max<int>(1, (fi.FrameMargins.left - fi.Win10Stealthy.left));
				// Due to the bug of DWM(?) we can't eliminate the top gap in caption-less mode
				// https://i.imgur.com/jxDPT3q.png
				// #SIZE_TODO Try DwmExtendFrameIntoClientArea to avoid bug?
				//// Correct top margin for caption-less window
				//if (c)
				//{
				//	_ASSERTE(fi.Win10Stealthy.top==0 && fi.VisibleFrameWidth>0);
				//	fi.FrameMargins.top = fi.VisibleFrameWidth + fi.Win10Stealthy.top;
				//}
			}
		}
	}

	if (is_log)
	{
		wchar_t szInfo[220];
		auto location = [](const MonitorInfoCache::TaskBarLocation& l) -> const wchar_t*
		{
			return (l == MonitorInfoCache::Left) ? L"Left"
				: (l == MonitorInfoCache::Right) ? L"Right"
				: (l == MonitorInfoCache::Top) ? L"Top"
				: (l == MonitorInfoCache::Bottom) ? L"Bottom"
				: L"None";
		};
		for (INT_PTR i = 0; i < monitors.size(); ++i)
		{
			auto& mi = monitors[i];
			swprintf_c(szInfo, L"  %i: #%08X dpi=%i; Full: {%i,%i}-{%i,%i} (%ix%i); Work: {%i,%i}-{%i,%i} (%ix%i)",
				(int)i, LODWORD(mi.hMon), mi.Ydpi, LOGRECTCOORDS(mi.mi.rcMonitor), LOGRECTSIZE(mi.mi.rcMonitor),
				LOGRECTCOORDS(mi.mi.rcWork), LOGRECTSIZE(mi.mi.rcWork));
			LogString(szInfo);
			swprintf_c(szInfo, L"     Caption: %i/%i {%i,%i}-{%i,%i}; NoCaption: %i/%i {%i,%i}-{%i,%i}; TaskBar: %s%s",
				mi.withCaption.FrameWidth, mi.withCaption.VisibleFrameWidth, LOGRECTCOORDS(mi.withCaption.FrameMargins),
				mi.noCaption.FrameWidth, mi.noCaption.VisibleFrameWidth, LOGRECTCOORDS(mi.noCaption.FrameMargins),
				location(mi.taskbarLocation), mi.isTaskbarHidden ? L"-Auto" : L"");
			LogString(szInfo);
		}
	}

	SizeInfo::RequestRecalc();
	mp_ConEmu->RecalculateFrameSizes();
}

void CConEmuSize::SetRequestedMonitor(HMONITOR hNewMon)
{
	_ASSERTE(isMainThread());
	mh_RequestedMonitor = hNewMon ? hNewMon : FindInitialMonitor();

	MonitorInfoCache mi = NearestMonitorInfo(mh_RequestedMonitor);
	gpSetCls->SetRequestedDpi(mi.Xdpi, mi.Ydpi);
	SizeInfo::RequestDpi({mi.Xdpi, mi.Ydpi});
}

CConEmuSize::MonitorInfoCache CConEmuSize::NearestMonitorInfo(const RECT& rcWnd) const
{
	HMONITOR hMon = MonitorFromRect(&rcWnd, MONITOR_DEFAULTTONEAREST);
	return NearestMonitorInfo(hMon);
}

// Pass hNewMon=NULL to get default/recommended monitor for our window
// Pass hNewMon=HMONITOR(-1) to get primary monitor
CConEmuSize::MonitorInfoCache CConEmuSize::NearestMonitorInfo(HMONITOR hNewMon) const
{
	// Must be filled already!
	if (monitors.empty())
	{
		_ASSERTEX(!monitors.empty());
		const_cast<CConEmuSize*>(this)->ReloadMonitorInfo();
		// Function always fills 'monitors' with at least one item
	}

	MSectionLockSimple locks; locks.Lock(&mcs_monitors);

	HMONITOR hTargetMon = hNewMon ? hNewMon : mh_RequestedMonitor;

	int iPrimary = 0;
	for (int i = 0; i < monitors.size(); ++i)
	{
		if (monitors[i].hMon == hTargetMon)
			return monitors[i];
		if (monitors[i].mi.dwFlags & MONITORINFOF_PRIMARY)
			iPrimary = i;
	}

	return monitors[iPrimary];
}

RECT CConEmuSize::GetVirtualScreenRect(bool abFullScreen)
{
	RECT rcScreen = MakeRect(800,600);
	int nMonitors = GetSystemMetrics(SM_CMONITORS);

	if (nMonitors > 1)
	{
		// Размер виртуального экрана по всем мониторам
		rcScreen.left = GetSystemMetrics(SM_XVIRTUALSCREEN); // may be <0
		rcScreen.top  = GetSystemMetrics(SM_YVIRTUALSCREEN);
		rcScreen.right = rcScreen.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
		rcScreen.bottom = rcScreen.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);

		// Хорошо бы исключить из рассмотрения Taskbar и прочие панели
		if (!abFullScreen)
		{
			RECT rcPrimaryWork;

			if (SystemParametersInfo(SPI_GETWORKAREA, 0, &rcPrimaryWork, 0))
			{
				if (rcPrimaryWork.left>0 && rcPrimaryWork.left<rcScreen.right)
					rcScreen.left = rcPrimaryWork.left;

				if (rcPrimaryWork.top>0 && rcPrimaryWork.top<rcScreen.bottom)
					rcScreen.top = rcPrimaryWork.top;

				if (rcPrimaryWork.right<rcScreen.right && rcPrimaryWork.right>rcScreen.left)
					rcScreen.right = rcPrimaryWork.right;

				if (rcPrimaryWork.bottom<rcScreen.bottom && rcPrimaryWork.bottom>rcScreen.top)
					rcScreen.bottom = rcPrimaryWork.bottom;
			}
		}
	}
	else
	{
		if (abFullScreen)
		{
			rcScreen = MakeRect(GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN));
		}
		else
		{
			SystemParametersInfo(SPI_GETWORKAREA, 0, &rcScreen, 0);
		}
	}

	if (rcScreen.right<=rcScreen.left || rcScreen.bottom<=rcScreen.top)
	{
		_ASSERTE(rcScreen.right>rcScreen.left && rcScreen.bottom>rcScreen.top);
		rcScreen = MakeRect(800,600);
	}

	return rcScreen;
}

RECT CConEmuSize::GetIdealRect()
{
	RECT rcIdeal = mrc_StoredNormalRect;
	if (mp_ConEmu->isInside() || IsRectMinimized(rcIdeal))
	{
		rcIdeal = GetDefaultRect();
	}

	if (!mp_ConEmu->isInside()
		&& (m_TileMode == cwc_TileLeft || m_TileMode == cwc_TileRight
			|| m_TileMode == cwc_TileHeight || m_TileMode == cwc_TileWidth))
	{
		auto mi = NearestMonitorInfo(rcIdeal);
		rcIdeal = GetTileRect(m_TileMode, mi.mi);
	}

	return rcIdeal;
}

void CConEmuSize::AutoSizeFont(RECT arFrom, enum ConEmuRect tFrom)
{
	if (!gpSet->isFontAutoSize)
		return;

	// В 16бит режиме - не заморачиваться пока
	if (mp_ConEmu->isNtvdm())
		return;

	if (!WndWidth.Value || !WndHeight.Value)
	{
		Assert(WndWidth.Value!=0 && WndHeight.Value!=0);
		return;
	}

	if ((WndWidth.Style != ss_Standard) || (WndHeight.Style != ss_Standard))
	{
		//TODO: Ниже WndWidth&WndHeight используется для расчета размера шрифта...
		Assert(WndWidth.Value!=0 && WndHeight.Value!=0);
		return;
	}

	//GO

	RECT rc = arFrom;

	if (tFrom == CER_MAIN)
	{
		rc = SizeInfo::WorkspaceRect();
	}
	else if (tFrom == CER_MAINCLIENT)
	{
		_ASSERTEX(FALSE && "Whole window size is preferred");
		RECT rcMain = CalcRect(CER_MAIN, arFrom, CER_MAINCLIENT);
		SizeInfo::RequestSize(RectWidth(rcMain), RectHeight(rcMain));
		rc = SizeInfo::WorkspaceRect();
	}
	else
	{
		MBoxAssert(tFrom==CER_MAINCLIENT || tFrom==CER_MAIN);
		return;
	}

	// !!! Для CER_DC размер в rc.right
	int nFontW = (rc.right - rc.left) / WndWidth.Value;

	if (nFontW < 5) nFontW = 5;

	int nFontH = (rc.bottom - rc.top) / WndHeight.Value;

	if (nFontH < 8) nFontH = 8;

	gpFontMgr->AutoRecreateFont(nFontW, nFontH);
}

void CConEmuSize::UpdateInsideRect(RECT rcNewPos)
{
	setWindowPos(HWND_TOP, rcNewPos.left, rcNewPos.top, RectWidth(rcNewPos), RectHeight(rcNewPos), 0);
}

// Return true, when rect was changed
bool CConEmuSize::FixWindowRect(RECT& rcWnd, ConEmuBorders nBorders, bool bPopupDlg /*= false*/)
{
	const RECT rcStore = rcWnd;
	RECT rcWork;

	// When we live inside parent window - size must be strict
	if (!bPopupDlg && mp_ConEmu->mp_Inside)
	{
		rcWork = GetDefaultRect();
		bool bChanged = (memcmp(&rcWork, &rcWnd, sizeof(rcWnd)) != 0);
		rcWnd = rcWork;
		return bChanged;
	}

	ConEmuWindowMode wndMode = GetWindowMode();

	if (!bPopupDlg && (wndMode == wmMaximized || wndMode == wmFullScreen))
	{
		nBorders |= CEB_TOP|CEB_LEFT|CEB_RIGHT|CEB_BOTTOM;
	}


	RECT rcFrame = CalcMargins_VisibleFrame();


	// We prefer nearest monitor
	RECT rcFind = {
		rcStore.left+(rcFrame.left+1)*2,
		rcStore.top+(rcFrame.top+1)*2,
		rcStore.right-(rcFrame.left+1)*2 /*rcStore.left+(rcFrame.left+1)*3*/,
		rcStore.bottom-(rcFrame.top+1)*2 /*rcStore.top+(rcFrame.top+1)*3*/
	};
	MONITORINFO mi; GetNearestMonitor(&mi, &rcFind);
	// Get work area (may be FullScreen)
	rcWork = (wndMode == wmFullScreen) ? mi.rcMonitor : mi.rcWork;

	// TODO: Проверить в Windows 10 с её невидимыми рамками
	RECT rcInvisible = CalcMargins_InvisibleFrame();
	AddMargins(rcWork, rcInvisible, rcop_Enlarge);

	RECT rcInter = rcWnd;
	BOOL bIn = IntersectRect(&rcInter, &rcWork, &rcStore);

	if (bIn && ((EqualRect(&rcInter, &rcStore)) || (nBorders & CEB_ALLOW_PARTIAL)))
	{
		// Nothing must be changed
		return false;
	}

	if (nBorders & CEB_ALLOW_PARTIAL)
	{
		RECT rcFind2 = {rcStore.right-rcFrame.left-1, rcStore.bottom-rcFrame.top-1, rcStore.right, rcStore.bottom};
		MONITORINFO mi2; GetNearestMonitor(&mi2, &rcFind2);
		RECT rcInter2, rcInter3;
		if (IntersectRect(&rcInter2, &mi2.rcMonitor, &rcStore) || IntersectRect(&rcInter3, &mi.rcMonitor, &rcStore))
		{
			// It's partially visible, OK
			return false;
		}
	}

	// Calculate border sizes (even though one of left/top/right/bottom requested)
	RECT rcMargins = (nBorders & CEB_ALL) ? rcFrame : MakeRect(0,0);
	if (!(nBorders & CEB_LEFT))
		rcMargins.left = 0;
	if (!(nBorders & CEB_TOP))
		rcMargins.top = 0;
	else if (IsWin10())
		rcMargins.top = std::min<LONG>(rcMargins.top, 1);
	if (!(nBorders & CEB_RIGHT))
		rcMargins.right = 0;
	if (!(nBorders & CEB_BOTTOM))
		rcMargins.bottom = 0;

	if (IsRectEmpty(&rcMargins))
	{
		// May be work rect will cover our window without margins?
		RECT rcTest = rcWork;
		AddMargins(rcTest, rcMargins, rcop_Enlarge);

		BOOL bIn2 = IntersectRect(&rcInter, &rcTest, &rcStore);
		if (bIn2 && EqualRect(&rcInter, &rcStore))
		{
			// Ok, it covers, nothing must be changed
			return false;
		}
	}

	bool bChanged = false;

	// We come here when at least part of ConEmu is "Out-of-screen"
	// "bIn == false" when ConEmu is totally "Out-of-screen"
	if (!bIn || !(nBorders & CEB_ALLOW_PARTIAL))
	{
		int nWidth = rcStore.right - rcStore.left - (rcInvisible.left + rcInvisible.right);
		int nHeight = rcStore.bottom - rcStore.top - (rcInvisible.top + rcInvisible.bottom);

		// All is bad. Windows is totally out of screen.
		bool overWidth = (rcStore.left < rcWork.left) || (rcStore.right > rcWork.right);
		bool overHeight = (rcStore.top < rcWork.top) || (rcStore.bottom > rcWork.bottom);

		rcWnd.left = std::max(rcWork.left-rcMargins.left,std::min(rcStore.left,rcWork.right-nWidth+rcMargins.right));
		rcWnd.top = std::max(rcWork.top-rcMargins.top,std::min(rcStore.top,rcWork.bottom-nHeight+rcMargins.bottom));

		// Add Width/Height. They may exceed monitor in wmNormal.
		if ((wndMode == wmNormal) && !IsCantExceedMonitor())
		{
			rcWnd.right = rcWnd.left+nWidth;
			rcWnd.bottom = rcWnd.top+nHeight;

			// Ideally, we must detects most right/bottom monitor to limit our window
			if (!PtInRect(&rcWork, MakePoint(rcWnd.right, rcWnd.bottom)))
			{
				// Yes, right-bottom corner is out-of-screen
				RECT rcTest = {rcWnd.right, rcWnd.bottom, rcWnd.right, rcWnd.bottom};

				MONITORINFO mi2; GetNearestMonitor(&mi2, &rcTest);
				RECT rcWork2 = (wndMode == wmFullScreen) ? mi2.rcMonitor : mi2.rcWork;

				rcWnd.right = std::min(rcWork2.right+rcMargins.right,rcWnd.right);
				rcWnd.bottom = std::min(rcWork2.bottom+rcMargins.bottom,rcWnd.bottom);
			}
		}
		else
		{
			// Maximized/FullScreen. Window CAN'T exceeds active monitor!
			rcWnd.right = std::min(rcWork.right+rcMargins.right,rcWnd.left+rcStore.right-rcStore.left);
			rcWnd.bottom = std::min(rcWork.bottom+rcMargins.bottom,rcWnd.top+rcStore.bottom-rcStore.top);
		}

		// Final corrections
		int newWidth = RectWidth(rcWnd), newHeight = RectHeight(rcWnd);
		if ((rcWnd.left < rcStore.left) && ((rcStore.left + newWidth) <= rcWork.right))
		{
			rcWnd.left = rcStore.left; rcWnd.right = rcWnd.left + newWidth;
		}
		if ((rcWnd.top < rcStore.top) && ((rcStore.top + newHeight) <= rcWork.bottom))
		{
			rcWnd.top = rcStore.top; rcWnd.bottom = rcWnd.top + newHeight;
		}

		// Done
		bChanged = (memcmp(&rcWnd, &rcStore, sizeof(rcWnd)) != 0);
	}

	return bChanged;
}

bool CConEmuSize::FixPosByStartupMonitor(const HMONITOR hStartMon)
{
	#define PSS_SKIP_PREFIX L"PatchSizeSettings skipping: "
	wchar_t szInfo[280];

	// If we haven't to change anything
	if (!hStartMon)
	{
		LogString(PSS_SKIP_PREFIX L"hStartMon is NULL");
		return false;
	}
	if (!mp_ConEmu->opt.Monitor.Exists && !gpSet->isRestore2ActiveMon)
	{
		LogString(PSS_SKIP_PREFIX L"Neither `-Monitor` switch nor `Restore to active monitor` option were specified");
		return false;
	}

	SetRequestedMonitor(hStartMon);

	// Perhaps, we shall not care of DPI in per-monitor-dpi systems
	// That is because we evaluate changed X/Y coordinates proportionally

	// NB. _wndX and _wndY may slightly exceed monitor rect.
	const RECT rcWnd = {gpSet->_wndX, gpSet->_wndY, gpSet->_wndX+500, gpSet->_wndY+300};
	RECT rcPrevMonitor = gpSet->LastMonRect;
	RECT rcInter = {};

	// Find HMONITOR where our window is supposed to be (due to settings)
	HMONITOR hCurMon = MonitorFromRect(&rcWnd, MONITOR_DEFAULTTONULL);

	// For new config (or old versions) rcPrevMonitor would be empty
	// So, evaluate it by rcWnd
	if (IsRectEmpty(&rcPrevMonitor) && hCurMon)
	{
		MONITORINFO miLast = {sizeof(miLast)};
		if (GetMonitorInfo(hCurMon, &miLast))
		{
			rcPrevMonitor = (gpSet->_WindowMode == wmFullScreen) ? miLast.rcMonitor : miLast.rcWork;
		}
	}

	// If rcWnd lays completely out of rcLastMon,
	// that would be an error in saved settings
	// We can't move window "properly"
	if (!IntersectRect(&rcInter, &rcWnd, &rcPrevMonitor))
	{
		swprintf_c(szInfo, PSS_SKIP_PREFIX L"Bad rects were stored: LastMon={%i,%i}-{%i,%i} Pos={%i,%i}", LOGRECTCOORDS(rcPrevMonitor), gpSet->_wndX, gpSet->_wndY);
		LogString(szInfo);
		return false;
	}


	// So, user asked to place our window to exact monitor?
	if (hStartMon == hCurMon)
	{
		LogString(PSS_SKIP_PREFIX L"Same monitor");
		return false; // already there
	}

	#ifdef _DEBUG
	// Find stored HMONITOR, if it exists. If NOT - move our window to the nearest
	HMONITOR hLastMon = MonitorFromRect(&rcPrevMonitor, MONITOR_DEFAULTTONEAREST);
	#endif

	// Retrieve information about monitor where use want to get our window
	MONITORINFO mi = {sizeof(mi)};
	if (!GetMonitorInfo(hStartMon, &mi))
	{
		LogString(PSS_SKIP_PREFIX L"GetMonitorInfo failed");
		return false;
	}
	RECT rcNewMon = (gpSet->_WindowMode == wmFullScreen) ? mi.rcMonitor : mi.rcWork;
	if (IsRectEmpty(&rcNewMon))
	{
		LogString(PSS_SKIP_PREFIX L"GetMonitorInfo failed (NULL RECT)");
		return false;
	}

	// Now, we are ready to reposition our window
	WndPos.x = rcNewMon.left + ((rcWnd.left - rcPrevMonitor.left) * RectWidth(rcNewMon) / RectWidth(rcPrevMonitor));
	WndPos.y = rcNewMon.top + ((rcWnd.top - rcPrevMonitor.top) * RectHeight(rcNewMon) / RectHeight(rcPrevMonitor));
	// Leave LastMonRect unchanged, because we change our internal WndPos instead
	// -- And update monitor rect
	// gpSet->LastMonRect = rcNewMon;

	// Log changes
	msprintf(szInfo, countof(szInfo), L"PatchSizeSettings changes: OldPos={%i,%i} NewPos={%i,%i} OldMon={%i,%i}-{%i,%i} NewMon=%08X:{%i,%i}-{%i,%i}",
		rcWnd.left, rcWnd.top, WndPos.x, WndPos.y, LOGRECTCOORDS(rcPrevMonitor), LODWORD(hStartMon), LOGRECTCOORDS(rcNewMon));
	LogString(szInfo);

	// WinPos was modified
	return true;
}

void CConEmuSize::StoreNormalRect(const RECT* prcWnd)
{
	mp_ConEmu->mouse.bCheckNormalRect = false;

	// Обновить координаты в gpSet, если требуется
	// Если сейчас окно в смене размера - игнорируем, размер запомнит SetWindowMode
	if ((GetWindowMode() == wmNormal) && !mp_ConEmu->isInside() && !isIconic())
	{
		if (prcWnd == NULL)
		{
			if (isFullScreen() || isZoomed() || isIconic())
				return;

			//131023 Otherwise, after tiling (Win+Left)
			//       and Lose/Get focus (Alt+Tab,Alt+Tab)
			//       StoreNormalRect will be called unexpectedly
			// Don't call Estimate here? Avoid excess calculations?
			ConEmuWindowCommand CurTile = GetTileMode(false/*Estimate*/);
			if (CurTile != cwc_Current)
				return;
		}

		RECT rcNormal = {};
		if (prcWnd)
		{
			ConEmuWindowCommand rect_tile = EvalTileMode(*prcWnd);
			if (rect_tile != cwc_Current)
			{
				_ASSERTE(rect_tile == cwc_Current); // Don't store tiled placement as "normal rect"
				return;
			}
			rcNormal = *prcWnd;
		}
		else
		{
			GetWindowRect(ghWnd, &rcNormal);
		}

		if (SizeInfo::IsRectMinimized(rcNormal))
			return;

		gpSetCls->UpdatePos(rcNormal.left, rcNormal.top);

		// 120720 - если окно сейчас тянут мышкой, то пока не обновлять mrc_StoredNormalRect,
		// иначе, если произошло Maximize при дотягивании до верхнего края экрана - то при
		// восстановлении окна получаем глюк позиционирования - оно прыгает заголовком за пределы.
		if (!isSizing())
		{
			if (mrc_StoredNormalRect != rcNormal)
			{
				wchar_t szLog[255];
				swprintf_c(szLog, L"UpdateNormalRect Cur={%i,%i}-{%i,%i} (%ix%i) New={%i,%i}-{%i,%i} (%ix%i)",
					LOGRECTCOORDS(mrc_StoredNormalRect), LOGRECTSIZE(mrc_StoredNormalRect),
					LOGRECTCOORDS(rcNormal), LOGRECTSIZE(rcNormal));
				LogString(szLog);
			}
			mrc_StoredNormalRect = rcNormal;
		}
		else
		{
			LogString(L"StoreNormalRect skipped due to `isSizing()`, continue to UpdateSize");
		}

		{
			// При ресайзе через окно настройки - mp_ VActive еще не перерисовался
			// так что и TextWidth/TextHeight не обновился
			//-- gpSetCls->UpdateSize(mp_ VActive->TextWidth, mp_ VActive->TextHeight);
			CESize w = {this->WndWidth.Raw};
			CESize h = {this->WndHeight.Raw};

			// Если хранятся размеры в ячейках - нужно позвать CVConGroup::AllTextRect()
			if ((w.Style == ss_Standard) || (h.Style == ss_Standard))
			{
				RECT rcAll;
				/* -- removed that because console size was not changed until resize was finished
				if (CVConGroup::isVConExists(0))
				{
					rcAll = CVConGroup::AllTextRect();
				}
				else
				*/
				{
					rcAll = CalcRect(CER_CONSOLE_ALL, rcNormal, CER_MAIN);
				}
				// And set new values to w & h
				if (w.Style == ss_Standard)
					w.Set(true, ss_Standard, rcAll.right);
				if (h.Style == ss_Standard)
					h.Set(false, ss_Standard, rcAll.bottom);
			}

			// Размеры в процентах от монитора?
			if ((w.Style == ss_Percents) || (h.Style == ss_Percents))
			{
				MONITORINFO mi;
				GetNearestMonitorInfo(&mi, NULL, &rcNormal);

				if (w.Style == ss_Percents)
					w.Set(true, ss_Percents, (rcNormal.right-rcNormal.left)*100/(mi.rcWork.right - mi.rcWork.left) );
				if (h.Style == ss_Percents)
					h.Set(false, ss_Percents, (rcNormal.bottom-rcNormal.top)*100/(mi.rcWork.bottom - mi.rcWork.top) );
			}

			if (w.Style == ss_Pixels)
				w.Set(true, ss_Pixels, rcNormal.right-rcNormal.left);
			if (h.Style == ss_Pixels)
				h.Set(false, ss_Pixels, rcNormal.bottom-rcNormal.top);

			gpSetCls->UpdateSize(w, h);
		}
	}
}

void CConEmuSize::CascadedPosFix()
{
	if (gpSet->wndCascade && (ghWnd == NULL) && (WindowMode == wmNormal) && IsSizePosFree(WindowMode))
	{
		// Сдвиг при каскаде
		int nShift = (int)(1.5*(GetSystemMetrics(SM_CYSIZEFRAME)+GetSystemMetrics(SM_CYCAPTION)));
		// Preferred window size
		int nDefWidth = 0, nDefHeight = 0;

		HWND hPrev = gpSetCls->isDontCascade ? NULL : FindWindow(VirtualConsoleClassMain, NULL);

		// Find first visible existing ConEmu window with normal state
		while (hPrev)
		{
			/*if (Is Iconic(hPrev) || Is Zoomed(hPrev)) {
				hPrev = FindWindowEx(NULL, hPrev, VirtualConsoleClassMain, NULL);
				continue;
			}*/
			WINDOWPLACEMENT wpl = {sizeof(WINDOWPLACEMENT)}; // Workspace coordinates!!!

			if (!GetWindowPlacement(hPrev, &wpl))
			{
				break;
			}

			// Screen coordinates!
			RECT rcWnd = {}; GetWindowRect(hPrev, &rcWnd);
			auto mi = NearestMonitorInfo(rcWnd);

			if (wpl.showCmd == SW_HIDE || !IsWindowVisible(hPrev)
			        || wpl.showCmd == SW_SHOWMINIMIZED || wpl.showCmd == SW_SHOWMAXIMIZED
			        /* Max в режиме скрытия заголовка */
			        || (wpl.rcNormalPosition.left<mi.mi.rcWork.left || wpl.rcNormalPosition.top<mi.mi.rcWork.top))
			{
				hPrev = FindWindowEx(NULL, hPrev, VirtualConsoleClassMain, NULL);
				continue;
			}

			if (!nDefWidth || !nDefHeight)
			{
				_ASSERTE(gpConEmu->GetStartupStage() <= CConEmuMain::ss_OnCreateCalled);
				const int minWidth = 160, minHeight = 16;
				SizeInfo::RequestDpi({mi.Xdpi, mi.Ydpi});
				SizeInfo::RequestPos((rcWnd.left + rcWnd.right)/2, (rcWnd.top + rcWnd.bottom)/2);
				SizeInfo::RequestSize(minWidth, minHeight);

				RECT rcDef = GetDefaultRect();
				nDefWidth = rcDef.right - rcDef.left;
				nDefHeight = rcDef.bottom - rcDef.top;
			}

			if (nDefWidth > 0 && nDefHeight > 0)
			{
				WndPos = VisualPosFromReal(
					std::min(rcWnd.left + nShift, mi.mi.rcWork.right - nDefWidth),
					std::min(rcWnd.top + nShift, mi.mi.rcWork.bottom - nDefHeight)
				);
			}
			else
			{
				WndPos = VisualPosFromReal(rcWnd.left + nShift, rcWnd.top + nShift);
			}
			break; // нашли, сдвинулись, выходим
		}
	}
}

void CConEmuSize::StorePreMinimizeMonitor()
{
	// Запомним, на каком мониторе мы были до минимизации
	if (!isIconic())
	{
		mh_MinFromMonitor = MonitorFromWindow(ghWnd, MONITOR_DEFAULTTONEAREST);
	}
}

HMONITOR CConEmuSize::GetNearestMonitor(MONITORINFO* pmi /*= NULL*/, LPCRECT prcWnd /*= NULL*/)
{
	NestedCallAssert(1);

	HMONITOR hMon = NULL;
	MONITORINFO mi = {sizeof(mi)};

	HWND hWndFrom = mp_ConEmu->mp_Inside ? mp_ConEmu->mp_Inside->GetParentRoot() : ghWnd;

	// #SIZE_TODO Utilize mh_MinFromMonitor during restore?

	if (prcWnd)
	{
		hMon = GetNearestMonitorInfo(&mi, NULL, prcWnd);
	}
	else if (!hWndFrom || (gpSet->isQuakeStyle && isIconic()))
	{
		_ASSERTE(WndWidth.Value>0 && WndHeight.Value>0);
		RECT rcEvalWnd = GetDefaultRect();
		hMon = GetNearestMonitorInfo(&mi, NULL, &rcEvalWnd);
	}
	else if (!mp_ConEmu->mp_Menu->GetRestoreFromMinimized() && !isIconic())
	{
		hMon = GetNearestMonitorInfo(&mi, NULL, NULL, hWndFrom);
	}
	else
	{
		_ASSERTEX(hWndFrom);
		// !! We can't use CalcRect, it may call GetNearestMonitor in turn !!
		//RECT rcDefault = CalcRect(CER_MAIN);
		WINDOWPLACEMENT wpl = {sizeof(wpl)};
		GetWindowPlacement(hWndFrom, &wpl);
		RECT rcDefault = wpl.rcNormalPosition;
		hMon = GetNearestMonitorInfo(&mi, mh_MinFromMonitor, &rcDefault);
	}

	// GetNearestMonitorInfo failed?
	_ASSERTE(hMon && mi.cbSize && !IsRectEmpty(&mi.rcMonitor) && !IsRectEmpty(&mi.rcWork));

	if (pmi)
	{
		*pmi = mi;
	}
	return hMon;
}

HMONITOR CConEmuSize::GetPrimaryMonitor(MONITORINFO* pmi /*= NULL*/)
{
	MONITORINFO mi = {sizeof(mi)};
	HMONITOR hMon = GetPrimaryMonitorInfo(&mi);

	// GetPrimaryMonitorInfo failed?
	_ASSERTE(hMon && mi.cbSize && !IsRectEmpty(&mi.rcMonitor) && !IsRectEmpty(&mi.rcWork));

	if (gpSet->isLogging(2))
	{
		wchar_t szInfo[255];
		swprintf_c(szInfo, L"  GetPrimaryMonitor=%u -> hMon=x%08X Work=({%i,%i}-{%i,%i}) Area=({%i,%i}-{%i,%i})",
			(hMon!=NULL), LODWORD(hMon), LOGRECTCOORDS(mi.rcWork), LOGRECTCOORDS(mi.rcMonitor));
		LogString(szInfo);
	}

	if (pmi)
	{
		*pmi = mi;
	}
	return hMon;
}

LRESULT CConEmuSize::OnGetMinMaxInfo(LPMINMAXINFO pInfo)
{
	bool bLog = RELEASEDEBUGTEST((gpSet->isLogging(2) >= 2),true);
	wchar_t szMinMax[255];
	RECT rcWnd;

	if (bLog)
	{
		GetWindowRect(ghWnd, &rcWnd);
		swprintf_c(szMinMax, L"OnGetMinMaxInfo[before] MaxSize={%i,%i}, MaxPos={%i,%i}, MinTrack={%i,%i}, MaxTrack={%i,%i}, Cur={%i,%i}-{%i,%i}\n",
	          pInfo->ptMaxSize.x, pInfo->ptMaxSize.y,
	          pInfo->ptMaxPosition.x, pInfo->ptMaxPosition.y,
	          pInfo->ptMinTrackSize.x, pInfo->ptMinTrackSize.y,
	          pInfo->ptMaxTrackSize.x, pInfo->ptMaxTrackSize.y,
			  LOGRECTCOORDS(rcWnd));
		if (gpSet->isLogging(2))
			LogString(szMinMax, true, false);
		DEBUGSTRSIZE(szMinMax);
	}


	auto mi = NearestMonitorInfo(NULL);
	auto prm = IsWin6() ? mi : NearestMonitorInfo(HMONITOR(-1));


	// *** Минимально допустимые размеры консоли
	RECT rcMin = MakeRect(MIN_CON_WIDTH,MIN_CON_HEIGHT);
	SIZE Splits = {};
	if (mp_ConEmu->isVConExists(0))
		rcMin = CVConGroup::AllTextRect(&Splits, true);
	RECT rcFrame = CalcRect(CER_MAIN, rcMin, CER_CONSOLE_ALL);
	if (Splits.cx > 0)
		rcFrame.right += Splits.cx * gpSetCls->EvalSize(gpSet->nSplitWidth, esf_Horizontal|esf_CanUseDpi);
	if (Splits.cy > 0)
		rcFrame.bottom += Splits.cy * gpSetCls->EvalSize(gpSet->nSplitHeight, esf_Vertical|esf_CanUseDpi);

	#if 0
	// This is only for testing purposes
	SizeInfo checker(dynamic_cast<const SizeInfo&>(*this));
	checker.RequestSize(RectWidth(rcFrame), RectHeight(rcFrame));
	RECT rcWork =
		checker.WorkspaceRect();
		//CalcRect(CER_WORKSPACE, rcFrame, CER_MAIN);
	#endif

	pInfo->ptMinTrackSize.x = rcFrame.right;
	pInfo->ptMinTrackSize.y = rcFrame.bottom;


	if (GetWindowMode() == wmFullScreen)
	{
		if (pInfo->ptMaxTrackSize.x < ptFullScreenSize.x)
			pInfo->ptMaxTrackSize.x = ptFullScreenSize.x;

		if (pInfo->ptMaxTrackSize.y < ptFullScreenSize.y)
			pInfo->ptMaxTrackSize.y = ptFullScreenSize.y;

		pInfo->ptMaxPosition.x = 0;
		pInfo->ptMaxPosition.y = 0;
		pInfo->ptMaxSize.x = std::max<int>(GetSystemMetrics(SM_CXFULLSCREEN), ptFullScreenSize.x);
		pInfo->ptMaxSize.y = std::max<int>(GetSystemMetrics(SM_CYFULLSCREEN), ptFullScreenSize.y);

		//if (pInfo->ptMaxSize.x < ptFullScreenSize.x)
		//	pInfo->ptMaxSize.x = ptFullScreenSize.x;

		//if (pInfo->ptMaxSize.y < ptFullScreenSize.y)
		//	pInfo->ptMaxSize.y = ptFullScreenSize.y;
	}
	else if (gpSet->isQuakeStyle)
	{
		_ASSERTE(::IsZoomed(ghWnd) == FALSE); // Стиль WS_MAXIMIZE для Quake выставляться НЕ ДОЛЖЕН!
		// Поэтому здесь нас интересует только "Normal"

		RECT rcWork;
		if (gpSet->_WindowMode == wmFullScreen)
		{
			rcWork = mi.mi.rcMonitor;
		}
		else
		{
			rcWork = mi.mi.rcWork;
		}

		if (pInfo->ptMaxTrackSize.x < (rcWork.right - rcWork.left))
			pInfo->ptMaxTrackSize.x = (rcWork.right - rcWork.left);

		if (pInfo->ptMaxTrackSize.y < (rcWork.bottom - rcWork.top))
			pInfo->ptMaxTrackSize.y = (rcWork.bottom - rcWork.top);
	}
	else //if (WindowMode == wmMaximized)
	{
		RECT rcShift = CalcMargins(CEM_FRAMEONLY, (WindowMode == wmNormal) ? wmMaximized : WindowMode);

		if (gnOsVer >= 0x600)
		{
			pInfo->ptMaxPosition.x = (mi.mi.rcWork.left - mi.mi.rcMonitor.left) - rcShift.left;
			pInfo->ptMaxPosition.y = (mi.mi.rcWork.top - mi.mi.rcMonitor.top) - rcShift.top;

			pInfo->ptMaxSize.x = mi.mi.rcWork.right - mi.mi.rcWork.left + (rcShift.left + rcShift.right);
			pInfo->ptMaxSize.y = mi.mi.rcWork.bottom - mi.mi.rcWork.top + (rcShift.top + rcShift.bottom);
		}
		else
		{
			// Issue 721: WinXP, TaskBar on top of screen. Assuming, mi.rcWork as {0,0,...}
			pInfo->ptMaxPosition.x = /*mi.rcWork.left*/ - rcShift.left;
			pInfo->ptMaxPosition.y = /*mi.rcWork.top*/ - rcShift.top;

			pInfo->ptMaxSize.x = prm.mi.rcWork.right - prm.mi.rcWork.left + (rcShift.left + rcShift.right);
			pInfo->ptMaxSize.y = prm.mi.rcWork.bottom - prm.mi.rcWork.top + (rcShift.top + rcShift.bottom);
		}
	}


	// Что получилось...
	if (bLog)
	{
		swprintf_c(szMinMax, L"OnGetMinMaxInfo[after]  MaxSize={%i,%i}, MaxPos={%i,%i}, MinTrack={%i,%i}, MaxTrack={%i,%i}, Cur={%i,%i}-{%i,%i}\n",
	          pInfo->ptMaxSize.x, pInfo->ptMaxSize.y,
	          pInfo->ptMaxPosition.x, pInfo->ptMaxPosition.y,
	          pInfo->ptMinTrackSize.x, pInfo->ptMinTrackSize.y,
	          pInfo->ptMaxTrackSize.x, pInfo->ptMaxTrackSize.y,
			  LOGRECTCOORDS(rcWnd));
		if (gpSet->isLogging(2))
			LogString(szMinMax, true, false);
		DEBUGSTRSIZE(szMinMax);
	}


	// If an application processes this message, it should return zero.
	return 0;
}

// Про IntegralSize - смотреть CConEmuSize::OnSizing
LRESULT CConEmuSize::OnWindowPosChanged(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0; // DefWindowProc зовется в середине функции

	WINDOWPOS *p = (WINDOWPOS*)lParam;

	if (hWnd != ghWnd)
	{
		_ASSERTE(hWnd == ghWnd);
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	wchar_t szInfo[255];
	if (RELEASEDEBUGTEST(gpSet->isLogging(), true))
	{
		RECT rcBox = {};
		HRGN hRgn = CreateRectRgn(0,0,0,0);
		int wRgn = GetWindowRgn(ghWnd, hRgn);
		int nRgn = (wRgn == SIMPLEREGION || wRgn == COMPLEXREGION) ? GetRgnBox(hRgn, &rcBox) : wRgn;
		swprintf_c(szInfo,
			nRgn
				? L"WM_WINDOWPOSCHANGED.begin: GetWindowRgn(0x%08X) = <%u> {%i,%i}-{%i,%i}"
				: L"WM_WINDOWPOSCHANGED.begin: GetWindowRgn(0x%08X) = NULL",
			ghWnd, nRgn, LOGRECTCOORDS(rcBox));
		if (!mp_ConEmu->LogWindowPos(szInfo)) { DEBUGSTRRGN(szInfo); }
		DeleteObject(hRgn);
	}

	// Если нужно поправить параметры DWM
	mp_ConEmu->ExtendWindowFrame();

	DWORD dwStyle = GetWindowLong(ghWnd, GWL_STYLE);
	DWORD dwStyleEx = GetWindowLong(ghWnd, GWL_EXSTYLE);

	DEBUGTEST(WINDOWPLACEMENT wpl = {sizeof(wpl)}; GetWindowPlacement(ghWnd, &wpl););
	DEBUGTEST(WINDOWPOS ps = *p);

	if (gpSet->isLogging(2))
	{
		wcscpy_c(szInfo, L"OnWindowPosChanged:");
		if (dwStyle & WS_MAXIMIZE) wcscat_c(szInfo, L" (zoomed)");
		if (dwStyle & WS_MINIMIZE) wcscat_c(szInfo, L" (iconic)");
		size_t cchLen = wcslen(szInfo);
		swprintf_c(szInfo+cchLen, countof(szInfo)-cchLen, L" x%08X x%08X (F:x%08X X:%i Y:%i W:%i H:%i)", dwStyle, dwStyleEx, p->flags, p->x, p->y, p->cx, p->cy);
		LogString(szInfo);
	}

	#ifdef _DEBUG
	static int cx, cy;

	if (!(p->flags & SWP_NOSIZE) && (cx != p->cx || cy != p->cy))
	{
		cx = p->cx; cy = p->cy;
	}

	// Отлов неожиданной установки "AlwaysOnTop"
	//static bool bWasTopMost = false;
	//_ASSERTE(((p->flags & SWP_NOZORDER) || (p->hwndInsertAfter!=HWND_TOPMOST)) && "OnWindowPosChanged");
	//_ASSERTE(((dwStyleEx & WS_EX_TOPMOST) == 0) && "OnWindowPosChanged");
	//bWasTopMost = ((dwStyleEx & WS_EX_TOPMOST)==WS_EX_TOPMOST);
	#endif

	wchar_t szDbg[128]; swprintf_c(szDbg, L"WM_WINDOWPOSCHANGED ({%i,%i}x{%i,%i} Flags=x%08X After=x%08X), style=0x%08X\n",
		p->x, p->y, p->cx, p->cy, p->flags, LODWORD(p->hwndInsertAfter), dwStyle);
	DEBUGSTRSIZE(szDbg);
	//WindowPosStackCount++;


	if (!gpSet->isAlwaysOnTop && ((dwStyleEx & WS_EX_TOPMOST) == WS_EX_TOPMOST))
	{
		CheckTopMostState();
		//_ASSERTE(((dwStyleEx & WS_EX_TOPMOST) == 0) && "Determined TopMost in OnWindowPosChanged");
		//SetWindowStyleEx(dwStyleEx & ~WS_EX_TOPMOST);
	}
	else if (gpSet->isDownShowExOnTopMessage && gpSet->isAlwaysOnTop && !(dwStyleEx & WS_EX_TOPMOST))
	{
		gpSet->isAlwaysOnTop = false;
	}

	if (!IsWindowModeChanging())
	{
		GetTileMode(true);
	}


	#ifdef _DEBUG
	if (isFullScreen() && !isZoomed())
	{
		_ASSERTE(GetTileMode(false)==cwc_Current);
	}
	#endif

	DEBUGTEST(WINDOWPOS ps1 = *p);

	// If monitor jump was triggered by OS but not ConEmu internals
	// По идее, смена DPI обработана в OnWindowPosChanging
	CheckDpiOnMoving(p);

	// Иначе могут не вызваться события WM_SIZE/WM_MOVE
	result = DefWindowProc(hWnd, uMsg, wParam, lParam);

	DEBUGTEST(WINDOWPOS ps2 = *p);

	//WindowPosStackCount--;

	if (hWnd == ghWnd /*&& ghOpWnd*/)  //2009-05-08 запоминать wndX/wndY всегда, а не только если окно настроек открыто
	{
		if (!mn_IgnoreSizeChange && !isIconic())
		{
			RECT rc = CalcRect(CER_MAIN);
			mp_ConEmu->mp_Status->OnWindowReposition(&rc);

			if ((changeFromWindowMode == wmNotChanging) && isWindowNormal())
			{
				//131023 Otherwise, after tiling (Win+Left)
				//       and Lose/Get focus (Alt+Tab,Alt+Tab)
				//       StoreNormalRect will be called unexpectedly
				// Don't call Estimate here? Avoid excess calculations?
				ConEmuWindowCommand CurTile = GetTileMode(false/*Estimate*/);
				if (CurTile == cwc_Current)
				{
					StoreNormalRect(&rc);
				}
			}

			if (mb_MonitorDpiChanged)
			{
				if (mp_ConEmu->mp_TabBar->IsTabsActive())
				{
					mp_ConEmu->mp_TabBar->Reposition();
				}

				OnSize();
			}

			if (mp_ConEmu->hPictureView)
			{
				mp_ConEmu->mrc_WndPosOnPicView = rc;
			}

			//else
			//{
			//	TODO("Доработать, когда будет ресайз PicView на лету");
			//	if (!(p->flags & SWP_NOSIZE))
			//	{
			//		RECT rcWnd = {0,0,p->cx,p->cy};
			//		ActiveCon()->RCon()->SyncConsole2Window(FALSE, &rcWnd);
			//	}
			//}
		}
		else
		{
			mp_ConEmu->mp_Status->OnWindowReposition(NULL);
		}
	}
	else if (mp_ConEmu->hPictureView)
	{
		GetWindowRect(ghWnd, &mp_ConEmu->mrc_WndPosOnPicView);
	}

	// Если окошко сворачивалось кнопками Win+Down (Win7) то SC_MINIMIZE не приходит
	if (gpSet->isMinToTray() && isIconic(true) && IsWindowVisible(ghWnd))
	{
		Icon.HideWindowToTray();
	}

	// Issue 878: ConEmu - Putty: Can't select in putty when ConEmu change display
	if (IsWindowVisible(ghWnd) && !isIconic() && !isSizing())
	{
		CVConGroup::NotifyChildrenWindows();
	}

	//Логирование, что получилось
	if (gpSet->isLogging())
	{
		mp_ConEmu->LogWindowPos(L"WM_WINDOWPOSCHANGED.end");
	}

	mb_MonitorDpiChanged = false;

	return result;
}

// Про IntegralSize - смотреть CConEmuSize::OnSizing
LRESULT CConEmuSize::OnWindowPosChanging(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	WINDOWPOS *p = (WINDOWPOS*)lParam;
	DEBUGTEST(WINDOWPOS ps = *p);

	if (hWnd != ghWnd)
	{
		_ASSERTE(hWnd == ghWnd);
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	mp_ConEmu->LogWindowPos(L"OnWindowPosChanging.begin");

	bool zoomed = _bool(::IsZoomed(ghWnd));
	bool iconic = _bool(::IsIconic(ghWnd));

	DWORD dwStyle = GetWindowLong(ghWnd, GWL_STYLE);
	DWORD dwStyleEx = GetWindowLong(ghWnd, GWL_EXSTYLE);

	if (m_JumpMonitor.bInJump)
	{
		if (m_JumpMonitor.bFullScreen || m_JumpMonitor.bMaximized)
			zoomed = true;
	}


	#ifdef _DEBUG
	DEBUGTEST(WINDOWPLACEMENT wpl = {sizeof(wpl)}; GetWindowPlacement(ghWnd, &wpl););
	_ASSERTE(zoomed == ((dwStyle & WS_MAXIMIZE) == WS_MAXIMIZE) || (zoomed && m_JumpMonitor.bInJump));
	_ASSERTE(iconic == ((dwStyle & WS_MINIMIZE) == WS_MINIMIZE));
	#endif

	// В процессе раскрытия/сворачивания могут меняться стили и вызывается ...Changing
	if (!gpSet->isQuakeStyle && (changeFromWindowMode != wmNotChanging))
	{
		if (!zoomed && ((WindowMode == wmMaximized) || (WindowMode == wmFullScreen)))
		{
			zoomed = true;
			// Это может быть, например, в случае смены стилей окна в процессе раскрытия (HideCaption)
			_ASSERTE(changeFromWindowMode==wmNormal || mp_ConEmu->InCreateWindow() || iconic);
		}
		else if (zoomed && (WindowMode == wmNormal))
		{
			_ASSERTE((changeFromWindowMode!=wmNormal) && "Must be not zoomed!");
			zoomed = false;
		}
		else if (WindowMode != wmNormal)
		{
			_ASSERTE(zoomed && "Need to check 'zoomed' state");
		}
	}

	wchar_t szInfo[280] = L"";
	{
		wcscpy_c(szInfo, L"OnWindowPosChanging.end:");
		if (zoomed) wcscat_c(szInfo, L" (zoomed)");
		if (iconic) wcscat_c(szInfo, L" (iconic)");
		size_t cchLen = wcslen(szInfo);
		swprintf_c(szInfo+cchLen, countof(szInfo)-cchLen/*#SECURELEN*/, L" x%08X x%08X (F:x%08X X:%i Y:%i W:%i H:%i)", dwStyle, dwStyleEx, p->flags, p->x, p->y, p->cx, p->cy);
	}


	#ifdef _DEBUG
	{
		wchar_t szDbg[128]; swprintf_c(szDbg, L"WM_WINDOWPOSCHANGING ({%i,%i}x{%i,%i} Flags=0x%08X) style=0x%08X%s\n", p->x, p->y, p->cx, p->cy, p->flags, dwStyle, zoomed ? L" (zoomed)" : L"");
		DEBUGSTRSIZE(szDbg);
		DWORD nCurTick = GetTickCount();
		static int cx, cy;
		static int wx, wy;

		// При реальной попытке смены положения/размера окна здесь можно поставить точку
		if ((!(p->flags & SWP_NOSIZE) && (cx != p->cx || cy != p->cy))
			|| (!(p->flags & SWP_NOMOVE) && (wx != p->x || wy != p->y)))
		{
			if (!(p->flags & SWP_NOSIZE)) { cx = p->cx; cy = p->cy; }
			if (!(p->flags & SWP_NOMOVE)) { wx = p->x;  wy = p->y;  }
		}

		// Отлов неожиданной установки "AlwaysOnTop"
		{
			#ifdef CATCH_TOPMOST_SET
			static bool bWasTopMost = false;
			_ASSERTE(((p->flags & SWP_NOZORDER) || (p->hwndInsertAfter!=HWND_TOPMOST)) && "OnWindowPosChanging");
			_ASSERTE(((dwStyleEx & WS_EX_TOPMOST) == 0) && "OnWindowPosChanging");
			_ASSERTE((bWasTopMost || gpSet->isAlwaysOnTop || ((dwStyleEx & WS_EX_TOPMOST)==0)) && "TopMost mode detected in OnWindowPosChanging");
			bWasTopMost = ((dwStyleEx & WS_EX_TOPMOST)==WS_EX_TOPMOST);
			#endif
		}
	}
	#endif

	if (mp_ConEmu->m_FixPosAfterStyle)
	{
		DWORD nCurTick = GetTickCount();

		if ((nCurTick - mp_ConEmu->m_FixPosAfterStyle) < FIXPOSAFTERSTYLE_DELTA)
		{
			#ifdef _DEBUG
			RECT rcDbgNow = {}; GetWindowRect(hWnd, &rcDbgNow);
			#endif
			p->flags &= ~(SWP_NOMOVE|SWP_NOSIZE);
			p->x = mp_ConEmu->mrc_FixPosAfterStyle.left;
			p->y = mp_ConEmu->mrc_FixPosAfterStyle.top;
			p->cx = RectWidth(mp_ConEmu->mrc_FixPosAfterStyle);
			p->cy = RectHeight(mp_ConEmu->mrc_FixPosAfterStyle);
		}
		mp_ConEmu->m_FixPosAfterStyle = 0;
	}

	//120821 - Размер мог быть изменен извне (Win+Up например)
	//120830 - только если не "Minimized"
	if (!iconic
		&& !mp_ConEmu->mb_InCaptionChange && !(p->flags & (SWP_NOSIZE|SWP_NOMOVE))
		&& (changeFromWindowMode == wmNotChanging) && !gpSet->isQuakeStyle)
	{
		// В этом случае нужно проверить, может быть требуется коррекция стилей окна?
		if (zoomed != (WindowMode == wmMaximized || WindowMode == wmFullScreen))
		{
			// В режимах Maximized/FullScreen должен быть WS_MAXIMIZED
			WindowMode = zoomed ? wmMaximized : wmNormal;
			// Установив WindowMode мы вызываем все необходимые действия
			// по смене стилей, обновлении регионов, и т.п.
			mp_ConEmu->RefreshWindowStyles();
		}
	}

	// -- // Если у нас режим скрытия заголовка (при максимизации/фулскрине)
	// При любой смене. Т.к. мы меняем WM_GETMINMAXINFO - нужно корректировать и размеры :(
	// Иначе возможны глюки
	if (!(p->flags & (SWP_NOSIZE|SWP_NOMOVE)) && !InMinimizing(p))
	{
		if (gpSet->isQuakeStyle)
		{
			if (!m_JumpMonitor.bInJump // gh - Quake is not moved to another monitor
				&& !mn_IgnoreSizeChange)
			{
				RECT rc = GetDefaultRect();

				p->y = rc.top;

				ConEmuWindowMode wmCurQuake = GetWindowMode();

				if (isSizing() && p->cx && (wmCurQuake == wmNormal))
				{
					// Если выбран режим "Fixed" - разрешим задавать левую координату, иначе - центрируется по монитору
					int iDifMul = gpSet->wndCascade ? 2 : 1;

					// Поскольку оно центрируется (wndCascade)... изменение ширину нужно множить на 2
					RECT rcQNow = {}; GetWindowRect(hWnd, &rcQNow);
					int width = rcQNow.right - rcQNow.left;
					int shift = (p->cx - width);

					if (shift && gpSet->wndCascade)
					{
						#ifdef _DEBUG
						wchar_t szQuake[200];
						swprintf_c(szQuake, L"QuakePosFix Shift=%i Old={%ix%i} Suggested={%ix%i} Fixed={%ix%i}",
							shift, rcQNow.left, width, p->x, p->cx, (p->x - shift), (p->cx + shift));
						DEBUGSTRSIZE(szQuake);
						#endif

						p->x -= shift;
						p->cx += shift;
					}
				}
				else
				{
					p->x = rc.left;
					p->cx = rc.right - rc.left; // + 1;
				}

				// We are in Quake style, update the coordinate
				_ASSERTE(gpSet->isQuakeStyle);
				POINT newPos = VisualPosFromReal(p->x, p->y);
				if (WndPos.x != newPos.x)
					WndPos.x = newPos.x;

				RECT rcFixup = {p->x, p->y, p->x + p->cx, p->y + p->cy};
				if (CheckQuakeRect(&rcFixup))
				{
					p->x = rcFixup.left;
					p->y = rcFixup.top;
					p->cx = rcFixup.right - rcFixup.left;
					p->cy = rcFixup.bottom - rcFixup.top;
				}
			}
		}
		else if (zoomed)
		{
			// Здесь может быть попытка перемещения на другой монитор. Нужно обрабатывать x/y/cx/cy!
			RECT rcNewMain = {p->x, p->y, p->x + p->cx, p->y + p->cy};
			RECT rc = CalcRect((WindowMode == wmFullScreen) ? CER_FULLSCREEN : CER_MAXIMIZED, rcNewMain, CER_MAIN);
			p->x = rc.left;
			p->y = rc.top;
			p->cx = rc.right - rc.left;
			p->cy = rc.bottom - rc.top;
		}
		else // normal only
		{
			_ASSERTE(GetWindowMode() == wmNormal);
			RECT rc = {p->x, p->y, p->x + p->cx, p->y + p->cy};
			if (FixWindowRect(rc, CEB_ALL|CEB_ALLOW_PARTIAL))
			{
				p->x = rc.left;
				p->y = rc.top;
				p->cx = rc.right - rc.left;
				p->cy = rc.bottom - rc.top;
			}
		}
	//#ifdef _DEBUG
	#if 0
		else if (isWindowNormal())
		{
			HMONITOR hMon = NULL;
			RECT rcNew = {}; GetWindowRect(ghWnd, &rcNew);
			if (!(p->flags & SWP_NOSIZE) && !(p->flags & SWP_NOMOVE))
			{
				// Меняются и размер и положение
				rcNew = MakeRect(p->x, p->y, p->x+p->cx, p->y+p->cy);
			}
			else if (!(p->flags & SWP_NOSIZE))
			{
				// Меняется только размер
				rcNew.right = rcNew.left + p->cx;
				rcNew.bottom = rcNew.top + p->cy;
			}
			else
			{
				// Меняется только положение
				rcNew = MakeRect(p->x, p->y, rcNew.right-rcNew.left, rcNew.bottom-rcNew.top);
			}

			hMon = MonitorFromRect(&rcNew, MONITOR_DEFAULTTONEAREST);
			MONITORINFO mi = {sizeof(mi)};
			if (GetMonitorInfo(hMon, &mi))
			{
				//if (WindowMode == wmFullScreen)
				//{
				//	p->x = mi.rcMonitor.left;
				//	p->y = mi.rcMonitor.top;
				//	p->cx = mi.rcMonitor.right-mi.rcMonitor.left;
				//	p->cy = mi.rcMonitor.bottom-mi.rcMonitor.top;
				//}
				//else
				//{
				//	p->x = mi.rcWork.left;
				//	p->y = mi.rcWork.top;
				//	p->cx = mi.rcWork.right-mi.rcWork.left;
				//	p->cy = mi.rcWork.bottom-mi.rcWork.top;
				//}
			}
			else
			{
				_ASSERTE(FALSE && "GetMonitorInfo failed");
			}

			//// Нужно скорректировать размеры, а то при смене разрешения монитора (в частности при повороте экрана) глюки лезут
			//p->flags |= (SWP_NOSIZE|SWP_NOMOVE);
			//// И обновить размер насильно
			//SetWindowMode(wmMaximized, TRUE);
		}
	#endif
	}
	else
	{
		#ifdef _DEBUG
		if (!(p->flags & SWP_NOMOVE) && (p->flags & SWP_NOSIZE))
		{
			DEBUGSTRSIZE(L"!!! Only position was changed !!!");
		}
		else if ((p->flags & SWP_NOMOVE) && !(p->flags & SWP_NOSIZE))
		{
			DEBUGSTRSIZE(L"!!! Only size was changed !!!");
		}
		#endif
	}

	//if (gpSet->isDontMinimize) {
	//	if ((p->flags & (0x8000|SWP_NOACTIVATE)) == (0x8000|SWP_NOACTIVATE)
	//		|| ((p->flags & (SWP_NOMOVE|SWP_NOSIZE)) == 0 && p->x < -30000 && p->y < -30000 )
	//		)
	//	{
	//		p->flags = SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE;
	//		p->hwndInsertAfter = HWND_BOTTOM;
	//		result = 0;
	//		if ((dwStyle & WS_MINIMIZE) == WS_MINIMIZE) {
	//			dwStyle &= ~WS_MINIMIZE;
	//			SetWindowStyle(dwStyle);
	//			mp_ConEmu->InvalidateAll();
	//		}
	//		break;
	//	}
	//}


	// If monitor jump was triggered by OS but not ConEmu internals
	// Per-monitor dpi? And moved to another monitor?
	// Чтобы правильно был рассчитан размер консоли
	bool bResized = CheckDpiOnMoving(p);

	// Add `SWP_NOSIZE` check because in that case {p->cx,p->cy} are 0
	// therefore OnSize CalcRect will return invalid result on empty src rect
	if (bResized && !(p->flags & SWP_NOSIZE) && (p->cx > 0) && (p->cy > 0))
	{
		//RECT rcWnd = {0,0,p->cx,p->cy};
		//CVConGroup::SyncAllConsoles2Window(rcWnd, CER_MAIN, true);
		//RECT rcClient = CalcRect(CER_MAINCLIENT, rcWnd, CER_MAIN);
		SizeInfo::RequestSize(p->cx, p->cy);
		OnSize(true, isZoomed() ? SIZE_MAXIMIZED : SIZE_RESTORED);
	}

#if 0
	if (!(p->flags & SWP_NOMOVE) && gpSet->isQuakeStyle && (p->y > -30))
	{
		int nDbg = 0;
	}
#endif

	/*
	-- DWM, Glass --
	dwmm.cxLeftWidth = 0;
	dwmm.cxRightWidth = 0;
	dwmm.cyTopHeight = kClientRectTopOffset;
	dwmm.cyBottomHeight = 0;
	DwmExtendFrameIntoClientArea(hwnd, &dwmm);
	-- DWM, Glass --
	*/
	// Иначе не вызовутся события WM_SIZE/WM_MOVE
	result = DefWindowProc(hWnd, uMsg, wParam, lParam);
	p = (WINDOWPOS*)lParam;

	size_t cchLen = wcslen(szInfo);
	swprintf_c(szInfo + cchLen, countof(szInfo) - cchLen/*#SECURELEN*/, L" --> (F:x%08X X:%i Y:%i W:%i H:%i)", p->flags, p->x, p->y, p->cx, p->cy);
	if (!LogString(szInfo)) { DEBUGSTRSIZE(szInfo); }

	return result;
}

// Returns true if dpi was changed and/or RealConsole recalc/resize is pending
// Called from OnWindowPosChanging and OnWindowPosChanged
bool CConEmuSize::CheckDpiOnMoving(WINDOWPOS *p)
{
	// Nothing was changed (neither pos nor size)?
	if ((p->flags & (SWP_NOMOVE|SWP_NOSIZE)) == (SWP_NOMOVE|SWP_NOSIZE))
		return false;

	// Currently minimizing?
	if (InMinimizing(p))
		return false;

	RECT rcOld = SizeInfo::m_size.rr.window, rcNew;
	if ((p->flags & SWP_NOMOVE))
		rcNew = RECT{rcOld.left, rcOld.top, rcOld.left + p->cx, rcOld.top + p->cy};
	else if ((p->flags & SWP_NOSIZE))
		rcNew = RECT{p->x, p->y, p->x + RectWidth(rcOld), p->y + RectHeight(rcOld)};
	else
		rcNew = RECT{p->x, p->y, p->x + p->cx, p->y + p->cy};

	bool bResized = (RectWidth(rcNew) != RectWidth(rcOld)) || (RectHeight(rcNew) != RectHeight(rcOld));

	// Lets go

	HMONITOR hMon = NULL;
	DpiValue dpi;

	#ifdef _DEBUG
	BOOL bVis = IsWindowVisible(ghWnd);
	#endif

	// If monitor jump was triggered by OS but not ConEmu internals
	// Per-monitor dpi? And moved to another monitor?
	if (CDpiAware::IsPerMonitorDpi())
	{
		if (CDpiAware::QueryDpiForRect(rcNew, &dpi) > 0)
		{
			// Это нужно делать до изменения размеров окна,
			// иначе будет неправильно рассчитан размер консоли
			if (OnDpiChanged(dpi.Xdpi, dpi.Ydpi, &rcNew, false, dcs_Internal))
			{
				// Вернуть флаг того, что DPI изменилось
				bResized = true;
				mb_MonitorDpiChanged = true;
			}
		}
	}

	return bResized;
}

LRESULT CConEmuSize::OnSize(bool bResizeRCon/*=true*/, WPARAM wParam/*=0*/)
{
	LRESULT result = 0;
#ifdef _DEBUG
	RECT rcDbgSize; GetWindowRect(ghWnd, &rcDbgSize);
	wchar_t szSize[255]; swprintf_c(szSize, L"OnSize(%u, %i) Current window size (X=%i, Y=%i, W=%i, H=%i)\n",
	                               (int)bResizeRCon, (DWORD)wParam,
	                               rcDbgSize.left, rcDbgSize.top, (rcDbgSize.right-rcDbgSize.left), (rcDbgSize.bottom-rcDbgSize.top));
	DEBUGSTRSIZE(szSize);
#endif

	if (wParam == SIZE_MINIMIZED || isIconic(true))
	{
		return 0;
	}

	if (mn_IgnoreSizeChange)
	{
		// на время обработки WM_SYSCOMMAND
		return 0;
	}

	if (!isMainThread())
	{
		//MBoxAssert(mn_MainThreadId == GetCurrentThreadId());
		PostMessage(ghWnd, WM_SIZE, MAKELPARAM(wParam,(bResizeRCon?1:2)), 0);
		return 0;
	}

	RECT rcRealClient = SizeInfo::RealClientRect();
	if (IsRectEmpty(&rcRealClient))
	{
		// If there are no space for anything - just exit
		return 0;
	}

	mp_ConEmu->InvalidateFrame();

	UpdateWindowRgn();

#ifdef _DEBUG
	DWORD_PTR dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE);
#endif
	//if (mb_InResize) {
	//	_ASSERTE(!mb_InResize);
	//	PostMessage(ghWnd, WM_SIZE, wParam, MAKELONG(newClientWidth,newClientHeight));
	//	return 0;
	//}
	mn_InResize++;


	//if (newClientWidth==(WORD)-1 || newClientHeight==(WORD)-1)
	//{
	//	RECT rcClient = ClientRect();
	//	newClientWidth = rcClient.right;
	//	newClientHeight = rcClient.bottom;
	//}

	//int nClientTop = 0;
	//#if defined(CONEMU_TABBAR_EX)
	//RECT rcFrame = CalcMargins(CEM_CLIENTSHIFT);
	//nClientTop = rcFrame.top;
	//#endif
	//RECT mainClient = MakeRect(0, nClientTop, newClientWidth, newClientHeight+nClientTop);
	//RECT main = CalcRect(CER_MAIN);
	//SizeInfo::RequestSize(RectWidth(main), RectHeight(main));
	RECT work = SizeInfo::WorkspaceRect();
	_ASSERTE(ghWndWork && GetParent(ghWndWork)==ghWnd); // пока рассчитано на дочерний режим
	MoveWindowRect(ghWndWork, work, TRUE);

	// Запомнить "идеальный" размер окна, выбранный пользователем
	if (isSizing() && isWindowNormal())
	{
		//GetWindowRect(ghWnd, &mrc_Ideal);
		//UpdateIdealRect();
		StoreNormalRect(NULL);
	}

	if (mp_ConEmu->mp_TabBar->IsTabsActive())
	{
		mp_ConEmu->mp_TabBar->Reposition();
		mp_ConEmu->mp_TabBar->RePaint();
	}

	// Background - должен занять все клиентское место под тулбаром
	// Там же ресайзится ScrollBar
	//ResizeChildren();

	BOOL lbIsPicView = mp_ConEmu->isPictureView();		UNREFERENCED_PARAMETER(lbIsPicView);

	if (bResizeRCon && (changeFromWindowMode == wmNotChanging) && (mn_InResize <= 1))
	{
		CVConGroup::SyncAllConsoles2Window(work, true);
	}

	if (CVConGroup::isVConExists(0))
	{
		CVConGroup::ReSizePanes(work);
	}

	mp_ConEmu->InvalidateGaps();

	if (mn_InResize>0)
		mn_InResize--;

	#ifdef _DEBUG
	dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE);
	#endif
	return result;
}

LRESULT CConEmuSize::OnSizing(WPARAM wParam, LPARAM lParam)
{
	DEBUGSTRSIZE(L"CConEmuSize::OnSizing");

	LRESULT result = true;

	#if defined(EXT_GNUC_LOG)
	wchar_t szDbg[255];
	wsprintf(szDbg, L"CConEmuSize::OnSizing(wParam=%i, L.Lo=%i, L.Hi=%i)\n",
	          wParam, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));
	if (gpSet->isLogging())
		mp_ConEmu->LogString(szDbg);
	#endif

	#if 0
	if (mp_ConEmu->isPictureView())
	{
		RECT *pRect = (RECT*)lParam; // с рамкой
		*pRect = mp_ConEmu->mrc_WndPosOnPicView;
		//pRect->right = pRect->left + (mrc_WndPosOnPicView.right-mrc_WndPosOnPicView.left);
		//pRect->bottom = pRect->top + (mrc_WndPosOnPicView.bottom-mrc_WndPosOnPicView.top);
	}
	else
	#endif

	if (mn_IgnoreSizeChange)
	{
		// на время обработки WM_SYSCOMMAND
	}
	else if (mp_ConEmu->isNtvdm())
	{
		// не менять для 16бит приложений
	}
	else if (isWindowNormal())
	{
		RECT srctWindow;
		RECT wndSizeRect, restrictRect, calcRect;
		RECT *pRect = (RECT*)lParam; // с рамкой
		RECT rcCurrent; GetWindowRect(ghWnd, &rcCurrent);

		if ((mp_ConEmu->mouse.state & (MOUSE_SIZING_BEGIN|MOUSE_SIZING_TODO))==MOUSE_SIZING_BEGIN
		        && isPressed(VK_LBUTTON))
		{
			SetSizingFlags(MOUSE_SIZING_TODO);
		}

		wndSizeRect = *pRect;
		#if 0
		// Для красивости рамки под мышкой
		LONG nWidth = gpFontMgr->FontWidth(), nHeight = gpFontMgr->FontHeight();

		if (nWidth && nHeight)
		{
			wndSizeRect.right += (nWidth-1)/2;
			wndSizeRect.bottom += (nHeight-1)/2;
		}
		#endif

		bool bNeedFixSize = gpSet->isIntegralSize();

		// Рассчитать желаемый размер консоли
		//srctWindow = ConsoleSizeFromWindow(&wndSizeRect, true /* frameIncluded */);
		AutoSizeFont(wndSizeRect, CER_MAIN);
		srctWindow = CalcRect(CER_CONSOLE_ALL, wndSizeRect, CER_MAIN);

		RECT rcMin = MakeRect(MIN_CON_WIDTH,MIN_CON_HEIGHT);
		SIZE Splits = {};
		if (mp_ConEmu->isVConExists(0))
			rcMin = CVConGroup::AllTextRect(&Splits, true);

		// Минимально допустимые размеры консоли
		if (srctWindow.right < rcMin.right)
		{
			srctWindow.right = rcMin.right;
			bNeedFixSize = true;
		}

		if (srctWindow.bottom < rcMin.bottom)
		{
			srctWindow.bottom = rcMin.bottom;
			bNeedFixSize = true;
		}


		if (bNeedFixSize)
		{
			calcRect = CalcRect(CER_MAIN, srctWindow, CER_CONSOLE_ALL);
			if (Splits.cx > 0)
				calcRect.right += Splits.cx * gpSetCls->EvalSize(gpSet->nSplitWidth, esf_Horizontal|esf_CanUseDpi);
			if (Splits.cy > 0)
				calcRect.bottom += Splits.cy * gpSetCls->EvalSize(gpSet->nSplitHeight, esf_Vertical|esf_CanUseDpi);
			#ifdef _DEBUG
			RECT rcRev = CalcRect(CER_CONSOLE_ALL, calcRect, CER_MAIN);
			_ASSERTE(rcRev.right==srctWindow.right && rcRev.bottom==srctWindow.bottom);
			#endif
			restrictRect.right = pRect->left + calcRect.right;
			restrictRect.bottom = pRect->top + calcRect.bottom;
			restrictRect.left = pRect->right - calcRect.right;
			restrictRect.top = pRect->bottom - calcRect.bottom;

			switch (wParam)
			{
				case WMSZ_RIGHT:
				case WMSZ_BOTTOM:
				case WMSZ_BOTTOMRIGHT:
					pRect->right = restrictRect.right;
					pRect->bottom = restrictRect.bottom;
					break;
				case WMSZ_LEFT:
				case WMSZ_TOP:
				case WMSZ_TOPLEFT:
					pRect->left = restrictRect.left;
					pRect->top = restrictRect.top;
					break;
				case WMSZ_TOPRIGHT:
					pRect->right = restrictRect.right;
					pRect->top = restrictRect.top;
					break;
				case WMSZ_BOTTOMLEFT:
					pRect->left = restrictRect.left;
					pRect->bottom = restrictRect.bottom;
					break;
			}
		}

		if (gpSet->isQuakeStyle)
		{
			switch (wParam)
			{
			case WMSZ_LEFT:
			case WMSZ_TOPLEFT:
			case WMSZ_BOTTOMLEFT:
				{
				// Save left coordinate if user drags left frame edge
				// We are in Quake style, update the coordinate
				POINT newPos = VisualPosFromReal(pRect->left, pRect->top);
				WndPos.x = newPos.x;
				break;
				}
			}
			// And final corrections (especially for centered mode)
			CheckQuakeRect(pRect);
		}

		// При смене размера (пока ничего не делаем)
		if ((pRect->right - pRect->left) != (rcCurrent.right - rcCurrent.left)
		        || (pRect->bottom - pRect->top) != (rcCurrent.bottom - rcCurrent.top))
		{
			// Сразу подресайзить консоль, чтобы при WM_PAINT можно было отрисовать уже готовые данные
			TODO("DoubleView");
			//ActiveCon()->RCon()->SyncConsole2Window(FALSE, pRect);
			#ifdef _DEBUG
			wchar_t szSize[255]; swprintf_c(szSize, L"New window size (X=%i, Y=%i, W=%i, H=%i); Current size (X=%i, Y=%i, W=%i, H=%i)\n",
			                               pRect->left, pRect->top, (pRect->right-pRect->left), (pRect->bottom-pRect->top),
			                               rcCurrent.left, rcCurrent.top, (rcCurrent.right-rcCurrent.left), (rcCurrent.bottom-rcCurrent.top));
			DEBUGSTRSIZE(szSize);
			#endif
		}
	}

	if (gpSet->isLogging())
		mp_ConEmu->LogWindowPos(L"OnSizing.end");

	return result;
}

LRESULT CConEmuSize::OnMoving(LPRECT prcWnd /*= NULL*/, bool bWmMove /*= false*/)
{
	BOOL bModified = FALSE;
	HMONITOR hMon = NULL;
	MONITORINFO mi = {sizeof(mi)};
	RECT rcCur = {};
	RECT rcShift;
	RECT rcUnshifted;
	RECT rcWnd;
	RECT rcWorkFix;
	int nMagnetSize;
	bool magnet_left, magnet_right, magnet_top, magnet_bottom;
	int nWidth, nHeight;
	POINT ptShift = {};
	DWORD SnappingFlags = smf_None;
	POINT ptCur = {};
	bool drag_by_mouse = false;

	if (!gpSet->isSnapToDesktopEdges && (m_TileMode == cwc_Current))
		goto wrap;

	if (isIconic() || isZoomed() || isFullScreen())
	{
		// TODO: Restore normal rect after exiting maximized mode by dragging the TabBar
		goto wrap;
	}

	if (bWmMove && isPressed(VK_LBUTTON))
	{
		// Prefer the monitor under mouse cursor
		drag_by_mouse = true;
		GetCursorPos(&ptCur);
		hMon = MonitorFromPoint(ptCur, MONITOR_DEFAULTTONEAREST);
	}
	else if (prcWnd)
	{
		hMon = MonitorFromRect(prcWnd, MONITOR_DEFAULTTONEAREST);
	}
	else
	{
		hMon = MonitorFromWindow(ghWnd, MONITOR_DEFAULTTONEAREST);
	}

	if (!GetMonitorInfo(hMon, &mi))
	{
		AssertMsg(L"GetMonitorInfo failed");
		goto wrap;
	}

	// Acquire window caption height and take into account DPI
	nMagnetSize = gpSetCls->EvalSize(GetSystemMetrics(SM_CYCAPTION)*2/3, esf_CanUseDpi|esf_Vertical);

	// Windows aero snap feature
	if (drag_by_mouse && prcWnd && IsWin7())
	{
		// We receive TWO OnMoving messages, first with proposed "aero snapped" rect,
		// and second with "real" window rect. We shall not reset snap flags on first.
		const RECT rc = mi.rcWork;
		// mouse cursor reaches the top of the screen --> maximize
		if (memcmp(prcWnd, &rc, sizeof(rc)) == 0)
			goto wrap_no_reset;
		// corners and left/right screen edges
		if ((ptCur.x >= (rc.right - nMagnetSize)) && (ptCur.x < rc.right))
		{
			if (prcWnd->right == mi.rcWork.right
				&& (prcWnd->top == mi.rcWork.top || prcWnd->bottom == mi.rcWork.bottom))
				goto wrap_no_reset;
		}
		if ((ptCur.x >= rc.left) && (ptCur.x < rc.left + nMagnetSize))
		{
			if (prcWnd->left == mi.rcWork.left
				&& (prcWnd->top == mi.rcWork.top || prcWnd->bottom == mi.rcWork.bottom))
				goto wrap_no_reset;
		}
	}

	rcShift = CalcMargins_InvisibleFrame();
	rcWorkFix = mi.rcWork;
	AddMargins(rcWorkFix, rcShift, rcop_Enlarge);

	if (prcWnd)
		rcCur = *prcWnd;
	else
		GetWindowRect(ghWnd, &rcCur);

	// Evaluate
	rcUnshifted = rcCur;
	if (m_Snapping.LastFlags)
	{
		rcUnshifted.left += m_Snapping.CorrectionX;
		rcUnshifted.right += m_Snapping.CorrectionX;
		rcUnshifted.top += m_Snapping.CorrectionY;
		rcUnshifted.bottom += m_Snapping.CorrectionY;
	}
	// TODO: current tile mode
	magnet_left = (_abs(rcWorkFix.left - rcUnshifted.left) <= nMagnetSize);
	magnet_right = (_abs(rcWorkFix.right - rcUnshifted.right) <= nMagnetSize);
	magnet_top = (_abs(rcWorkFix.top - rcUnshifted.top) <= nMagnetSize);
	magnet_bottom = (_abs(rcWorkFix.bottom - rcUnshifted.bottom) <= nMagnetSize);

	// If the window edges are far from monitor's working area
	if (!magnet_left && !magnet_right && !magnet_top && !magnet_bottom)
	{
		if (m_Snapping.LastFlags)
		{
			rcWnd = rcUnshifted;
			SnappingFlags = smf_None;
			goto modified;
		}
		goto wrap;
	}

	nWidth = rcCur.right - rcCur.left;
	nHeight = rcCur.bottom - rcCur.top;

	rcWnd = rcCur;
	if (magnet_left)
	{
		rcWnd.left = rcWorkFix.left;
		rcWnd.right = std::min<int>(rcWorkFix.right, rcWnd.left + nWidth);
		ptShift.x = rcUnshifted.left - rcWnd.left;
		SnappingFlags |= smf_Left;
	}
	// TODO: tile width
	else if (magnet_right)
	{
		rcWnd.right = rcWorkFix.right;
		rcWnd.left = std::max<int>(rcWorkFix.left, rcWorkFix.right - nWidth);
		ptShift.x = rcUnshifted.right - rcWnd.right;
		SnappingFlags |= smf_Right;
	}
	// no-mod horz
	if (!(SnappingFlags & smf_Horz) && (m_Snapping.LastFlags & smf_Horz))
	{
		rcWnd.left = rcUnshifted.left;
		rcWnd.right = rcUnshifted.right;
	}

	if (magnet_top)
	{
		rcWnd.top = rcWorkFix.top;
		rcWnd.bottom = std::min(rcWorkFix.bottom, rcWnd.top + nHeight);
		ptShift.y = rcUnshifted.top - rcWnd.top;
		SnappingFlags |= smf_Top;
	}
	// TODO: Tile height
	else if (magnet_bottom)
	{
		rcWnd.bottom = rcWorkFix.bottom;
		rcWnd.top = std::max<int>(rcWorkFix.top, rcWorkFix.bottom - nHeight);
		ptShift.y = rcUnshifted.bottom - rcWnd.bottom;
		SnappingFlags |= smf_Bottom;
	}
	// no-mod vert
	if (!(SnappingFlags & smf_Vert) && (m_Snapping.LastFlags & smf_Vert))
	{
		rcWnd.top = rcUnshifted.top;
		rcWnd.bottom = rcUnshifted.bottom;
	}

	if (memcmp(&rcWnd, &rcCur, sizeof(rcWnd)) == 0)
		goto wrap;

modified:
	bModified = TRUE;

	if (prcWnd == NULL)
	{
		// May get here from OnBtn_SnapToDesktopEdges if gpSet->isSnapToDesktopEdges
		setWindowPos(NULL, rcWnd.left, rcWnd.top, nWidth, nHeight, SWP_NOZORDER);
	}
	else
	{
		*prcWnd = rcWnd;
	}

wrap:
	m_Snapping.reset(SnappingFlags, ptShift.x, ptShift.y);
wrap_no_reset:
	if (gpSet->isLogging())
		mp_ConEmu->LogWindowPos(L"OnMoving.end", prcWnd);
	return bModified;
}

// Например при вызове из диалога "Settings..." и нажатия кнопки "Apply" (Size & Pos)
bool CConEmuSize::SizeWindow(const CESize sizeW, const CESize sizeH)
{
	if (mp_ConEmu->mp_Inside)
	{
		// В Inside - размер окна не меняется. Оно всегда подгоняется под родителя.
		return false;
	}

	ConEmuWindowMode wndMode = GetWindowMode();
	if (wndMode == wmMaximized || wndMode == wmFullScreen)
	{
		// Если окно развернуто - его размер таким образом (по консоли например) менять запрещается

		// Quake??? Разрешить?

		_ASSERTE(wndMode != wmMaximized && wndMode != wmFullScreen);
		return false;
	}

	// Set window size by evaluated values

	SIZE szPixelSize = GetDefaultSize(false, &sizeW, &sizeH);

	bool bSizeOK = false;

	WndWidth.Set(true, sizeW.Style, sizeW.Value);
	WndHeight.Set(false, sizeH.Style, sizeH.Value);

	if (setWindowPos(NULL, 0,0, szPixelSize.cx, szPixelSize.cy, SWP_NOMOVE|SWP_NOZORDER))
	{
		bSizeOK = true;
	}

	return bSizeOK;
}

void CConEmuSize::QuakePrevSize::Save(const CESize& awndWidth, const CESize& awndHeight, const LONG& awndX, const LONG& awndY, const BYTE& anFrame, const ConEmuWindowMode& aWindowMode, const RECT& arcStoredNormalRect, const bool& abAlwaysShowTrayIcon)
{
	wndWidth = awndWidth;
	wndHeight = awndHeight;
	wndX = awndX;
	wndY = awndY;
	nFrame = anFrame;
	WindowMode = aWindowMode;
	rcStoredNormalRect = arcStoredNormalRect;
	MinToTray = abAlwaysShowTrayIcon;
	//
	bWasSaved = true;
}

ConEmuWindowMode CConEmuSize::QuakePrevSize::Restore(CESize& rwndWidth, CESize& rwndHeight, LONG& rwndX, LONG& rwndY, BYTE& rnFrame, RECT& rrcStoredNormalRect, bool& rbAlwaysShowTrayIcon)
{
	rwndWidth = wndWidth;
	rwndHeight = wndHeight;
	rwndX = wndX;
	rwndY = wndY;
	_ASSERTE(nFrame == LOBYTE(nFrame));
	rnFrame = LOBYTE(nFrame);
	rrcStoredNormalRect = rcStoredNormalRect;
	rbAlwaysShowTrayIcon = MinToTray;
	return WindowMode;
}

// If Quake mode was loaded from settings and user wants to get back "standard" behavior
void CConEmuSize::QuakePrevSize::SetNonQuakeDefaults()
{
	gpSet->mb_MinToTray = false;
	gpSet->nHideCaptionAlwaysFrame = HIDECAPTIONALWAYSFRAME_DEF;
}

bool CConEmuSize::SetQuakeMode(BYTE NewQuakeMode, ConEmuWindowMode nNewWindowMode /*= wmNotChanging*/, bool bFromDlg /*= false*/)
{
	if (mb_InSetQuakeMode)
	{
		_ASSERTE(mb_InSetQuakeMode==false);
		return false;
	}

	mb_InSetQuakeMode = true;

	BYTE bPrevStyle = gpSet->isQuakeStyle;
	gpSet->isQuakeStyle = NewQuakeMode;

	if (gpSet->isQuakeStyle && bFromDlg)
	{
		gpSet->isTryToCenter = true;
		gpSet->SetMinToTray(true);
	}

	// Quake is incompatible with "Desktop mode", drop last one
	if (NewQuakeMode && mp_ConEmu->opt.DesktopMode)
	{
		mp_ConEmu->opt.DesktopMode.Clear();
		DoDesktopModeSwitch();
	}

	//ConEmuWindowMode nNewWindowMode =
	//	IsChecked(hWnd2, rMaximized) ? wmMaximized :
	//	IsChecked(hWnd2, rFullScreen) ? wmFullScreen :
	//	wmNormal;
	if (nNewWindowMode == wmNotChanging || nNewWindowMode == wmCurrent)
		nNewWindowMode = (ConEmuWindowMode)gpSet->_WindowMode;
	else
		gpSet->_WindowMode = nNewWindowMode;

	if (gpSet->isQuakeStyle && !bPrevStyle)
	{
		m_QuakePrevSize.Save(this->WndWidth, this->WndHeight, this->WndPos.x, this->WndPos.y, gpSet->nHideCaptionAlwaysFrame, this->WindowMode, mrc_StoredNormalRect, gpSet->mb_MinToTray);
	}
	else if (!gpSet->isQuakeStyle)
	{
		if (m_QuakePrevSize.bWasSaved)
			nNewWindowMode = m_QuakePrevSize.Restore(this->WndWidth, this->WndHeight, this->WndPos.x, this->WndPos.y, gpSet->nHideCaptionAlwaysFrame, mrc_StoredNormalRect, gpSet->mb_MinToTray);
		else
			m_QuakePrevSize.SetNonQuakeDefaults();
	}


	RECT rcWnd = this->GetDefaultRect();
	UNREFERENCED_PARAMETER(rcWnd);

	// При отключении Quake режима - сразу "подвинуть" окошко на "старое место"
	// Иначе оно уедет за границы экрана при вызове OnHideCaption
	if (!gpSet->isQuakeStyle && bPrevStyle)
	{
		m_QuakePrevSize.bWaitReposition = true;
		StoreNormalRect(&rcWnd);
	}

	if (ghWnd)
		this->SetWindowMode(nNewWindowMode, TRUE);

	if (m_QuakePrevSize.bWaitReposition)
		m_QuakePrevSize.bWaitReposition = false;

	CSetPgBase* pSetPg = gpSetCls->GetActivePageObj();
	if (pSetPg)
	{
		TabHwndIndex tt = pSetPg->GetPageType();
		if ((tt == thi_Appear)
			|| (tt == thi_Quake)
			|| (tt == thi_SizePos)
			)
		{
			pSetPg->OnInitDialog(pSetPg->Dlg(), false/*abInitial*/);
		}
	}

	// Save current rect, JIC
	if (ghWnd)
		StoreNormalRect(NULL);

	if (ghWnd)
		apiSetForegroundWindow(ghOpWnd ? ghOpWnd : ghWnd);

	mb_InSetQuakeMode = false;
	return true;
}

ConEmuWindowMode CConEmuSize::GetWindowMode() const
{
	ConEmuWindowMode wndMode = gpSet->isQuakeStyle ? ((ConEmuWindowMode)gpSet->_WindowMode) : WindowMode;

	#ifdef _DEBUG
	bool bZoomed = _bool(::IsZoomed(ghWnd));
	bool bIconic = _bool(::IsIconic(ghWnd));
	bool bCaptionHidden = isCaptionHidden(); // Auto-hidden TaskBar doesn't roll over maximized ConEmu in Win 8.1
	bool bInTransition = (changeFromWindowMode == wmNormal) || mp_ConEmu->InCreateWindow()
		|| (m_JumpMonitor.bInJump && (m_JumpMonitor.bFullScreen || m_JumpMonitor.bMaximized))
		|| !IsWindowVisible(ghWnd);
	_ASSERTE((WindowMode == wmNormal) || bZoomed || bCaptionHidden || bIconic || bInTransition);
	#endif

	return wndMode;
}

ConEmuWindowMode CConEmuSize::GetChangeFromWindowMode() const
{
	return changeFromWindowMode;
}

bool CConEmuSize::IsWindowModeChanging() const
{
	_ASSERTE(mn_IgnoreSizeChange>=0);
	return (changeFromWindowMode != wmNotChanging) || m_JumpMonitor.bInJump || mn_IgnoreSizeChange;
}

LPCWSTR CConEmuSize::FormatTileMode(ConEmuWindowCommand Tile, wchar_t* pchBuf, size_t cchBufMax)
{
	switch (Tile)
	{
	case cwc_TileLeft:
		_wcscpy_c(pchBuf, cchBufMax, L"cwc_TileLeft"); break;
	case cwc_TileRight:
		_wcscpy_c(pchBuf, cchBufMax, L"cwc_TileRight"); break;
	case cwc_TileHeight:
		_wcscpy_c(pchBuf, cchBufMax, L"cwc_TileHeight"); break;
	case cwc_TileWidth:
		_wcscpy_c(pchBuf, cchBufMax, L"cwc_TileWidth"); break;
	default:
		swprintf_c(pchBuf, cchBufMax/*#SECURELEN*/, L"%u", (UINT)Tile);
	}

	return pchBuf;
}

RECT CConEmuSize::SetNormalWindowSize()
{
	RECT rcNewWnd = GetDefaultRect();

	if (!ghWnd)
	{
		_ASSERTE(ghWnd!=NULL);
		return rcNewWnd;
	}

	if (m_TileMode != cwc_Current)
	{
		_ASSERTE(m_TileMode == cwc_Current);
		SetTileMode(cwc_Current);
	}

	HMONITOR hMon = NULL;

	if (!isIconic())
		hMon = MonitorFromWindow(ghWnd, MONITOR_DEFAULTTONEAREST);
	else
		hMon = MonitorFromRect(&rcNewWnd, MONITOR_DEFAULTTONEAREST);

	if (!hMon)
	{
		_ASSERTE(hMon!=NULL);
		setWindowPos(NULL, rcNewWnd.left, rcNewWnd.top, rcNewWnd.right-rcNewWnd.left, rcNewWnd.bottom-rcNewWnd.top, SWP_NOZORDER);
	}
	else
	{
		JumpNextMonitor(ghWnd, hMon, true/*без разницы, если указан монитор*/, rcNewWnd);
	}

	return rcNewWnd;
}

void CConEmuSize::SetTileMode(ConEmuWindowCommand Tile)
{
	if (m_TileMode != Tile)
	{
		wchar_t szInfo[64], szName[32], szOld[32];
		swprintf_c(szInfo, L"SetTileMode(%s) OldMode=%s", FormatTileMode(Tile,szName,countof(szName)), FormatTileMode(m_TileMode,szOld,countof(szOld)));
		mp_ConEmu->LogString(szInfo);

		m_TileMode = Tile;
	}
}

bool CConEmuSize::ChangeTileMode(ConEmuWindowCommand Tile)
{
	// While debugging - low-level keyboard hooks almost lock DevEnv
	HooksUnlocker;

	if (gpSet->isLogging())
	{
		wchar_t szInfo[64], szName[32];
		swprintf_c(szInfo, L"ChangeTileMode(%s)", FormatTileMode(Tile,szName,countof(szName)));
		mp_ConEmu->LogString(szInfo);
	}


	if (isIconic())
	{
		mp_ConEmu->LogString(L"ChangeTileMode SKIPPED due to isIconic");
		return false;
	}

	if (!IsSizePosFree())
	{
		_ASSERTE(FALSE && "Tiling not allowed in Quake and Inside modes");
		mp_ConEmu->LogString(L"ChangeTileMode SKIPPED due to Quake/Inside");
		return false;
	}

	if (!mp_ConEmu->isMeForeground(false, false))
	{
		mp_ConEmu->LogString(L"ChangeTileMode SKIPPED because ConEmu is not a foreground window");
		return false;
	}

	if (Tile != cwc_TileLeft && Tile != cwc_TileRight && Tile != cwc_TileHeight && Tile != cwc_TileWidth)
	{
		_ASSERTE(FALSE && "ChangeTileMode SKIPPED due to invalid mode");
		mp_ConEmu->LogString(L"ChangeTileMode SKIPPED due to invalid mode");
		return false;
	}

	MONITORINFO mi = {0};
	ConEmuWindowCommand CurTile = GetTileMode(true/*Estimate*/, &mi);
	ConEmuWindowMode wm = GetWindowMode();

	if (mi.cbSize)
	{
		bool bChange = false;
		RECT rcNewWnd = {};

		if ((wm == wmNormal) && (Tile == cwc_Current))
		{
			StoreNormalRect(NULL);
		}

		HMONITOR hMon = NULL;

		changeFromWindowMode = wmNormal;

		// When window is snapped to the right edge, and user press Win+Right
		// Same with left edge and Win+Left
		// ConEmu must jump to next monitor and set tile to Left
		if ((CurTile == Tile) && (CurTile == cwc_TileLeft || CurTile == cwc_TileRight))
		{
			MONITORINFO nxt = {0};
			hMon = GetNextMonitorInfo(&nxt, &mi.rcWork, (CurTile == cwc_TileRight)/*Next*/);
			//131022 - if there is only one monitor - just switch Left/Right tiling...
			if (!hMon || !nxt.cbSize)
				hMon = GetNearestMonitorInfo(&nxt, NULL, &mi.rcWork);
			if (hMon && nxt.cbSize /* && memcmp(&mi.rcWork, &nxt.rcWork, sizeof(nxt.rcWork))*/)
			{
				memmove(&mi, &nxt, sizeof(nxt));
				Tile = (CurTile == cwc_TileRight) ? cwc_TileLeft : cwc_TileRight;
			}
		}
		// Already snapped to right, Win+Left must "restore" window
		else if ((Tile == cwc_TileLeft) && (CurTile == cwc_TileRight))
		{
			SetTileMode(Tile = cwc_Current);
			rcNewWnd = SetNormalWindowSize();
			bChange = false;
		}
		// Already snapped to left, Win+Right must "restore" window
		else if ((Tile == cwc_TileRight) && (CurTile == cwc_TileLeft))
		{
			SetTileMode(Tile = cwc_Current);
			rcNewWnd = SetNormalWindowSize();
			bChange = false;
		}
		// Already maximized by width or height?
		else if (((Tile == cwc_TileWidth) || (Tile == cwc_TileHeight))
			&& ((CurTile == cwc_TileWidth) || (CurTile == cwc_TileHeight)))
		{
			SetTileMode(Tile = cwc_Current);
			rcNewWnd = SetNormalWindowSize();
			bChange = false;
		}

		switch (Tile)
		{
		case cwc_Current:
			// Вернуться из Tile-режима в нормальный
			rcNewWnd = GetDefaultRect();
			//bChange = true; -- already processed in SetNormalWindowSize
			break;

		case cwc_TileLeft:
		case cwc_TileRight:
		case cwc_TileHeight:
		case cwc_TileWidth:
			rcNewWnd = GetTileRect(Tile, mi);
			bChange = true;
			break;
		}

		if (bChange)
		{
			// Выйти из FullScreen/Maximized режима
			if (GetWindowMode() != wmNormal)
				SetWindowMode(wmNormal);

			// Сразу меняем, чтобы DefaultRect не слетел...
			SetTileMode(Tile);

			LogTileModeChange(L"ChangeTileMode", Tile, bChange, rcNewWnd, NULL, hMon);

			if (CDpiAware::IsPerMonitorDpi())
			{
				DpiValue dpi;
				if (CDpiAware::QueryDpiForRect(rcNewWnd, &dpi))
				{
					OnDpiChanged(dpi.Xdpi, dpi.Ydpi, &rcNewWnd, false, dcs_Snap);
				}
			}

			setWindowPos(NULL, rcNewWnd.left, rcNewWnd.top, rcNewWnd.right-rcNewWnd.left, rcNewWnd.bottom-rcNewWnd.top, SWP_NOZORDER);
		}

		changeFromWindowMode = wmNotChanging;

		RECT rc = {}; GetWindowRect(ghWnd, &rc);
		LogTileModeChange(L"result of ChangeTileMode", Tile, bChange, rcNewWnd, &rc, hMon);

		CVConGroup::SyncConsoleToWindow();

		mp_ConEmu->UpdateProcessDisplay(false);
	}

	return true;
}

RECT CConEmuSize::GetTileRect(ConEmuWindowCommand Tile, const MONITORINFO& mi) const
{
	RECT rcNewWnd;

	// gh-284
	RECT rcWin10 = CalcMargins_InvisibleFrame();

	switch (Tile)
	{
	case cwc_Current:
		// Вернуться из Tile-режима в нормальный
		rcNewWnd = const_cast<CConEmuSize*>(this)->GetDefaultRect();
		break;

	case cwc_TileLeft:
		rcNewWnd.left = mi.rcWork.left;
		rcNewWnd.top = mi.rcWork.top;
		rcNewWnd.bottom = mi.rcWork.bottom;
		rcNewWnd.right = (mi.rcWork.left + mi.rcWork.right) >> 1;
		AddMargins(rcNewWnd, rcWin10, rcop_Enlarge);
		break;

	case cwc_TileRight:
		rcNewWnd.right = mi.rcWork.right;
		rcNewWnd.top = mi.rcWork.top;
		rcNewWnd.bottom = mi.rcWork.bottom;
		rcNewWnd.left = (mi.rcWork.left + mi.rcWork.right) >> 1;
		AddMargins(rcNewWnd, rcWin10, rcop_Enlarge);
		break;

	case cwc_TileHeight:
		rcNewWnd = const_cast<CConEmuSize*>(this)->GetDefaultRect();
		rcNewWnd.top = mi.rcWork.top - rcWin10.top;
		rcNewWnd.bottom = mi.rcWork.bottom + rcWin10.bottom;
		break;

	case cwc_TileWidth:
		rcNewWnd = const_cast<CConEmuSize*>(this)->GetDefaultRect();
		rcNewWnd.left = mi.rcWork.left - rcWin10.left;
		rcNewWnd.right = mi.rcWork.right + rcWin10.right;
		break;

	default:
		AssertMsg(L"Must not get here");
		rcNewWnd = const_cast<CConEmuSize*>(this)->GetDefaultRect();
	}

	return rcNewWnd;
}

void CConEmuSize::LogTileModeChange(LPCWSTR asPrefix, ConEmuWindowCommand Tile, bool bChanged, const RECT& rcSet, LPRECT prcAfter, HMONITOR hMon)
{
	if (!gpSet->isLogging() && !IsDebuggerPresent())
		return;

	ConEmuWindowCommand NewTile = GetTileMode(false/*Estimate*/, NULL);
	wchar_t szInfo[200], szName[32], szNewTile[32], szAfter[64];

	if (prcAfter)
		swprintf_c(szAfter, L" -> %s {%i,%i}-{%i,%i}",
			FormatTileMode(NewTile,szNewTile,countof(szNewTile)),
			LOGRECTCOORDS(*prcAfter)
			);
	else
		szAfter[0] = 0;

	swprintf_c(szInfo, L"%s(%s) -> %u {%i,%i}-{%i,%i} x%08X%s",
		asPrefix,
		FormatTileMode(Tile,szName,countof(szName)),
		(UINT)bChanged,
		LOGRECTCOORDS(rcSet),
		(DWORD)(DWORD_PTR)hMon,
		szAfter);

	if (gpSet->isLogging())
		mp_ConEmu->LogString(szInfo);
	else
		DEBUGSTRSIZE(szInfo);
}

ConEmuWindowCommand CConEmuSize::EvalTileMode(const RECT& arcWnd, MONITORINFO* pmi /*= NULL*/)
{
	ConEmuWindowCommand CurTile = cwc_Current;

	// gh-284
	RECT rcWnd = arcWnd;
	RECT rcWin10 = CalcMargins(CEM_WIN10FRAME);
	AddMargins(rcWnd, rcWin10, rcop_Shrink);

	// Where we are
	MONITORINFO mi = {};
	GetNearestMonitorInfo(&mi, NULL, &rcWnd, ghWnd);
	if (pmi)
		*pmi = mi;

	// Is auto-hidden task-bar found on current monitor?
	UINT uEdge = (UINT)-1;
	bool bAutoHidden = IsTaskbarAutoHidden(&mi.rcMonitor, &uEdge);
	// Than we shall take it into account
	RECT rcAlt = mi.rcWork;
	if (bAutoHidden)
	{
		switch (uEdge)
		{
			case ABE_LEFT:   rcAlt.left   += 1; break;
			case ABE_RIGHT:  rcAlt.right  -= 1; break;
			case ABE_TOP:    rcAlt.top    += 1; break;
			case ABE_BOTTOM: rcAlt.bottom -= 1; break;
		}
	}

	// If the window covers whole working area,
	// that is not a "tile" mode, but sort of "Maximized"
	if (((rcWnd.left == mi.rcWork.left) || (rcWnd.left == rcAlt.left))
		&& ((rcWnd.right == mi.rcWork.right) || (rcWnd.right == rcAlt.right))
		&& ((rcWnd.top == mi.rcWork.top) || (rcWnd.top == rcAlt.top))
		&& ((rcWnd.bottom == mi.rcWork.bottom) || (rcWnd.bottom == rcAlt.bottom)))
	{
		goto wrap;
	}

	// _abs(x1-x2) <= 1 ?
	if (((rcWnd.right == mi.rcWork.right) || (rcWnd.right == rcAlt.right))
		&& ((rcWnd.top == mi.rcWork.top) || (rcWnd.top == rcAlt.top))
		&& ((rcWnd.bottom == mi.rcWork.bottom) || (rcWnd.bottom == rcAlt.bottom)))
	{
		int nCenter = ((mi.rcWork.right + mi.rcWork.left) >> 1) - rcWnd.left;
		if (_abs(nCenter) <= 4)
		{
			CurTile = cwc_TileRight;
			goto wrap;
		}
	}

	if (((rcWnd.left == mi.rcWork.left) || (rcWnd.left == rcAlt.left))
		&& ((rcWnd.top == mi.rcWork.top) || (rcWnd.top == rcAlt.top))
		&& ((rcWnd.bottom == mi.rcWork.bottom) || (rcWnd.bottom == rcAlt.bottom)))
	{
		int nCenter = ((mi.rcWork.right + mi.rcWork.left) >> 1) - rcWnd.right;
		if (_abs(nCenter) <= 4)
		{
			CurTile = cwc_TileLeft;
			goto wrap;
		}
	}

	if (((rcWnd.top == mi.rcWork.top) || (rcWnd.top == rcAlt.top))
		&& ((rcWnd.bottom == mi.rcWork.bottom) || (rcWnd.bottom == rcAlt.bottom)))
	{
		CurTile = cwc_TileHeight;
		goto wrap;
	}

	if (((rcWnd.left == mi.rcWork.left) || (rcWnd.left == rcAlt.left))
		&& ((rcWnd.right == mi.rcWork.right) || (rcWnd.right == rcAlt.right)))
	{
		CurTile = cwc_TileWidth;
		goto wrap;
	}

wrap:
	return CurTile;
}

ConEmuWindowCommand CConEmuSize::GetTileMode(bool Estimate, MONITORINFO* pmi/*=NULL*/)
{
	if (Estimate)
	{
		ConEmuWindowCommand CurTile = m_TileMode;

		if (isFullScreen() || isZoomed())
		{
			if (pmi)
				GetNearestMonitorInfo(pmi, NULL, NULL, ghWnd);
			CurTile = cwc_Current;
		}
		else if (IsSizeFree()  // gh-622: Quake wmNormal, but 100% width
			// && !isFullScreen() && !isZoomed() -- gh#142
			&& !isIconic()
			&& ((mp_ConEmu->GetStartupStage() < CConEmuMain::ss_Started)
				|| IsWindowVisible(ghWnd))
			)
		{
			RECT rcWnd = {};
			GetWindowRect(ghWnd, &rcWnd);
			_ASSERTE(IsWindowModeChanging() == false);
			CurTile = EvalTileMode(rcWnd, pmi);
		}

		if (m_TileMode != CurTile)
		{
			wchar_t szTile[255], szNewTile[32], szOldTile[32];
			swprintf_c(szTile, L"Tile mode was changed externally: Our=%s, New=%s",
				FormatTileMode(m_TileMode,szOldTile,countof(szOldTile)), FormatTileMode(CurTile,szNewTile,countof(szNewTile)));
			LogString(szTile);
			DEBUGSTRSIZE(szTile);

			SetTileMode(CurTile);

			mp_ConEmu->UpdateProcessDisplay(false);
		}
	}

	_ASSERTE(m_TileMode==cwc_Current || m_TileMode==cwc_TileLeft || m_TileMode==cwc_TileRight || m_TileMode==cwc_TileHeight || m_TileMode==cwc_TileWidth);
	return m_TileMode;
}

// В некоторых режимах менять положение/размер окна произвольно - нельзя,
// для Quake например окно должно быть прилеплено к верхней границе экрана
// в режиме Inside - размер окна вообще заблокирован и зависит от свободной области
bool CConEmuSize::IsSizeFree(ConEmuWindowMode CheckMode /*= wmFullScreen*/)
{
	// В некоторых режимах - нельзя
	if (mp_ConEmu->mp_Inside)
		return false;

	// FullScreen is not supported in "Desktop" mode
	if (mp_ConEmu->opt.DesktopMode && (CheckMode == wmFullScreen))
		return false;

	// В режиме "Quake" менять размер можно только в "Normal"
	if (gpSet->isQuakeStyle && (GetWindowMode() != wmNormal))
		return false;

	// Размер И положение можно менять произвольно
	return true;
}
bool CConEmuSize::IsSizePosFree(ConEmuWindowMode CheckMode /*= wmFullScreen*/)
{
	// В некоторых режимах - нельзя
	if (gpSet->isQuakeStyle || mp_ConEmu->mp_Inside)
		return false;

	// FullScreen is not supported in "Desktop" mode
	if (mp_ConEmu->opt.DesktopMode && (CheckMode == wmFullScreen))
		return false;

	// Размер И положение можно менять произвольно
	return true;
}
// В некоторых режимах - не должен выходить за пределы экрана
bool CConEmuSize::IsCantExceedMonitor()
{
	_ASSERTE(!mp_ConEmu->mp_Inside);
	if (gpSet->isQuakeStyle)
		return true;
	return false;
}
bool CConEmuSize::IsPosLocked()
{
	// В некоторых режимах - двигать окно юзеру вообще нельзя
	if (mp_ConEmu->mp_Inside)
		return true;

	// Положение - строго не ограничивается
	return false;
}
bool CConEmuSize::IsInResize()
{
	return (mn_InResize > 0);
}
bool CConEmuSize::IsInWindowModeChange()
{
	return (changeFromWindowMode != wmNotChanging);
}

bool CConEmuSize::JumpNextMonitor(bool Next)
{
	// While debugging - low-level keyboard hooks almost lock DevEnv
	HooksUnlocker;

	if (mp_ConEmu->mp_Inside || mp_ConEmu->opt.DesktopMode)
	{
		LogString(L"JumpNextMonitor skipped, not allowed in Inside/Desktop modes");
		return false;
	}

	wchar_t szInfo[255];
	RECT rcMain;

	HWND hJump = getForegroundWindow();
	if (!hJump)
	{
		Assert(hJump!=NULL);
		LogString(L"JumpNextMonitor skipped, no foreground window");
		return false;
	}
	DWORD nWndPID = 0; GetWindowThreadProcessId(hJump, &nWndPID);
	if (nWndPID != GetCurrentProcessId())
	{
		if (gpSet->isLogging())
		{
			swprintf_c(szInfo,
				L"JumpNextMonitor skipped, not our window PID=%u, HWND=x%08X",
				nWndPID, LODWORD(hJump));
			LogString(szInfo);
		}
		return false;
	}

	if (hJump == ghWnd)
	{
		rcMain = CalcRect(CER_MAIN);
	}
	else
	{
		GetWindowRect(hJump, &rcMain);
	}

	return JumpNextMonitor(hJump, NULL, Next, rcMain);
}

void CConEmuSize::EvalNewNormalPos(const MONITORINFO& miOld, HMONITOR hNextMon, const MONITORINFO& miNew, const RECT& rcOld, RECT& rcNew)
{
	#ifdef _DEBUG
	RECT rcPrevMon = miOld.rcWork;
	RECT rcNextMon = miNew.rcWork;
	#endif

	// Если мониторы различаются по разрешению или рабочей области - коррекция позиционирования
	int ShiftX = rcOld.left - miOld.rcWork.left;
	int ShiftY = rcOld.top - miOld.rcWork.top;

	int Width = (rcOld.right - rcOld.left);
	int Height = (rcOld.bottom - rcOld.top);

	// Если ширина или высота были заданы в процентах (от монитора)
	if ((WndWidth.Style == ss_Percents) || (WndHeight.Style == ss_Percents))
	{
		_ASSERTE(!gpSet->isQuakeStyle); // Quake?

		if ((WindowMode == wmNormal) && hNextMon)
		{
			SIZE szNewSize = GetDefaultSize(false, &WndWidth, &WndHeight, hNextMon);
			if ((szNewSize.cx > 0) && (szNewSize.cy > 0))
			{
				if (WndWidth.Style == ss_Percents)
					Width = szNewSize.cx;
				if (WndHeight.Style == ss_Percents)
					Height = szNewSize.cy;
			}
			else
			{
				_ASSERTE((szNewSize.cx > 0) && (szNewSize.cy > 0));
			}
		}
	}


	if (ShiftX > 0)
	{
		int SpaceX = ((miOld.rcWork.right - miOld.rcWork.left) - Width);
		int NewSpaceX = ((miNew.rcWork.right - miNew.rcWork.left) - Width);
		if (SpaceX > 0)
		{
			if (NewSpaceX <= 0)
				ShiftX = 0;
			else
				ShiftX = ShiftX * NewSpaceX / SpaceX;
		}
	}

	if (ShiftY > 0)
	{
		int SpaceY = ((miOld.rcWork.bottom - miOld.rcWork.top) - Height);
		int NewSpaceY = ((miNew.rcWork.bottom - miNew.rcWork.top) - Height);
		if (SpaceY > 0)
		{
			if (NewSpaceY <= 0)
				ShiftY = 0;
			else
				ShiftY = ShiftY * NewSpaceY / SpaceY;
		}
	}

	rcNew.left = miNew.rcWork.left + ShiftX;
	rcNew.top = miNew.rcWork.top + ShiftY;
	rcNew.right = rcNew.left + Width;
	rcNew.bottom = rcNew.top + Height;
}

bool CConEmuSize::JumpNextMonitor(HWND hJumpWnd, HMONITOR hJumpMon, bool Next, const RECT rcJumpWnd, LPRECT prcNewPos /*= NULL*/)
{
	wchar_t szInfo[255];
	RECT rcMain = {};
	bool bFullScreen = false;
	bool bMaximized = false;
	const DWORD nStyles = GetWindowLong(hJumpWnd, GWL_STYLE);

	rcMain = rcJumpWnd;
	if (hJumpWnd == ghWnd)
	{
		//-- rcMain = CalcRect(CER_MAIN);
		bFullScreen = isFullScreen();
		bMaximized = bFullScreen ? false : isZoomed();
	}
	else
	{
		//-- GetWindowRect(hJumpWnd, &rcMain);
	}

	MONITORINFO mi = {};
	HMONITOR hNext = hJumpMon ? (GetMonitorInfoSafe(hJumpMon, mi) ? hJumpMon : NULL) : GetNextMonitorInfo(&mi, &rcMain, Next);
	if (!hNext)
	{
		if (gpSet->isLogging())
		{
			swprintf_c(szInfo,
				L"JumpNextMonitor(%i) skipped, GetNextMonitorInfo({%i,%i}-{%i,%i}) returns NULL",
				(int)Next, LOGRECTCOORDS(rcMain));
			LogString(szInfo);
		}
		return false;
	}
	else if (gpSet->isLogging())
	{
		swprintf_c(szInfo,
			L"JumpNextMonitor(%i), GetNextMonitorInfo({%i,%i}-{%i,%i}) returns 0x%08X ({%i,%i}-{%i,%i})",
			(int)Next, LOGRECTCOORDS(rcMain),
			(DWORD)(DWORD_PTR)hNext, LOGRECTCOORDS(mi.rcMonitor));
		LogString(szInfo);
	}

	SetRequestedMonitor(hNext);

	MONITORINFO miCur;
	DEBUGTEST(HMONITOR hCurMonitor = )
		GetNearestMonitor(&miCur, &rcMain);

	RECT rcNewMain = rcMain;

	if ((bFullScreen || bMaximized) && (hJumpWnd == ghWnd))
	{
		_ASSERTE(hJumpWnd == ghWnd);
		rcNewMain = CalcRect(bFullScreen ? CER_FULLSCREEN : CER_MAXIMIZED, mi.rcMonitor, CER_MAIN);
	}
	else
	{
		_ASSERTE(WindowMode==wmNormal || hJumpWnd!=ghWnd);

		EvalNewNormalPos(miCur, hNext, mi, rcMain, rcNewMain);
	}


	// Поскольку кнопки мы перехватываем - нужно предусмотреть
	// прыжки и для других окон, например окна настроек
	if (hJumpWnd == ghWnd)
	{
		if (CDpiAware::IsPerMonitorDpi())
		{
			DpiValue dpi;
			if (CDpiAware::QueryDpiForMonitor(hNext, &dpi))
			{
				OnDpiChanged(dpi.Xdpi, dpi.Ydpi, &rcNewMain, false, dcs_Jump);
			}
		}

		// Коррекция (заранее)
		OnMoving(&rcNewMain);

		m_JumpMonitor.rcNewPos = rcNewMain;
		m_JumpMonitor.bInJump = true;
		m_JumpMonitor.bFullScreen = bFullScreen;
		m_JumpMonitor.bMaximized = bMaximized;

		#ifdef _DEBUG
		const DWORD nStylesDbg = GetWindowLong(ghWnd, GWL_STYLE);
		_ASSERTE(nStylesDbg == nStyles);
		#endif
		if (bFullScreen)
		{
			// Win8. Не хочет прыгать обратно
			mp_ConEmu->SetWindowStyle(ghWnd, nStyles & ~WS_MAXIMIZE);
		}

		AutoSizeFont(rcNewMain, CER_MAIN);
		// Рассчитать размер консоли по оптимальному WindowRect
		CVConGroup::PreReSize(WindowMode, rcNewMain);
	}

	if (prcNewPos)
	{
		*prcNewPos = rcNewMain;
	}

	// Move it
	if (hJumpWnd == ghWnd)
		setWindowPos(NULL, rcNewMain.left, rcNewMain.top, rcNewMain.right-rcNewMain.left, rcNewMain.bottom-rcNewMain.top, SWP_NOZORDER|SWP_NOCOPYBITS);
	else
		SetWindowPos(hJumpWnd, NULL, rcNewMain.left, rcNewMain.top, rcNewMain.right-rcNewMain.left, rcNewMain.bottom-rcNewMain.top, SWP_NOZORDER|SWP_NOCOPYBITS);

	// Almost done
	if (hJumpWnd == ghWnd)
	{
		if (bFullScreen)
		{
			// Restore styles
			mp_ConEmu->SetWindowStyle(ghWnd, nStyles);
			// Check it
			_ASSERTE(WindowMode == wmFullScreen);
		}

		m_JumpMonitor.bInJump = false;
	}

	return true;
}

// inMode: wmNormal, wmMaximized, wmFullScreen
bool CConEmuSize::SetWindowMode(ConEmuWindowMode inMode, bool abForce /*= false*/, bool abFirstShow /*= false*/)
{
	if (inMode != wmNormal && inMode != wmMaximized && inMode != wmFullScreen)
		inMode = wmNormal; // ошибка загрузки настроек?

	if (!isMainThread())
	{
		PostMessage(ghWnd, mp_ConEmu->mn_MsgSetWindowMode, inMode, 0);
		return false;
	}

	//if (gpSet->isQuakeStyle && !mb_InSetQuakeMode)
	//{
	//	bool bQuake = SetQuakeMode(gpSet->isQuakeStyle
	//}

	if (mp_ConEmu->opt.DesktopMode)
	{
		if (inMode == wmFullScreen)
			inMode = (WindowMode != wmNormal) ? wmNormal : wmMaximized; // FullScreen на Desktop-е невозможен
	}

	wchar_t szInfo[255];

	swprintf_c(szInfo, L"SetWindowMode begin: CurMode=%s inMode=%s", GetWindowModeName(GetWindowMode()), GetWindowModeName(inMode));
	if (!LogString(szInfo)) { DEBUGSTRWMODE(szInfo); }

	if ((inMode != wmFullScreen) && (WindowMode == wmFullScreen))
	{
		// Отмена vkForceFullScreen
		DoForcedFullScreen(false);
	}

	#ifdef _DEBUG
	DWORD_PTR dwStyleDbg1 = GetWindowLongPtr(ghWnd, GWL_STYLE);
	#endif

	if (mp_ConEmu->isPictureView())
	{
		#ifndef _DEBUG
		//2009-04-22 Если открыт PictureView - лучше не дергаться...
		return false;
		#else
		_ASSERTE(FALSE && "Change WindowMode will be skipped in Release: isPictureView()");
		#endif
	}

	SetCursor(LoadCursor(NULL,IDC_WAIT));
	mp_ConEmu->mp_Menu->SetPassSysCommand(true);

	//WindowPlacement -- использовать нельзя, т.к. он работает в координатах Workspace, а не Screen!
	RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);

	// Logging purposes
	auto mi = IsRectMinimized(rcWnd) ? NearestMonitorInfo(NULL) : NearestMonitorInfo(rcWnd);
	swprintf_c(szInfo, L"SetWindowMode info: Wnd={%i,%i}-{%i,%i} Mon=x%08X Work={%i,%i}-{%i,%i} Full={%i,%i}-{%i,%i}",
		LOGRECTCOORDS(rcWnd), LODWORD(mi.hMon), LOGRECTCOORDS(mi.mi.rcWork), LOGRECTCOORDS(mi.mi.rcMonitor));
	if (!LogString(szInfo)) { DEBUGSTRWMODE(szInfo); }

	//bool canEditWindowSizes = false;
	bool lbRc = false;
	static bool bWasSetFullscreen = false;
	// Сброс флагов ресайза мышкой
	ResetSizingFlags(MOUSE_SIZING_BEGIN|MOUSE_SIZING_TODO);

	bool bNewFullScreen = (inMode == wmFullScreen) || (gpSet->isQuakeStyle && (gpSet->_WindowMode == wmFullScreen));

	if (bWasSetFullscreen && !bNewFullScreen)
	{
		mp_ConEmu->Taskbar_MarkFullscreenWindow(ghWnd, FALSE);
		bWasSetFullscreen = false;
	}

	bool bIconic = isIconic();

	// When changing more to smth else than wmNormal, and current mode is wmNormal
	// save current window rect for further usage (when restoring from wmMaximized for example)
	if ((inMode != wmNormal) && isWindowNormal() && !bIconic && (GetTileMode(false) == cwc_Current))
		StoreNormalRect(&rcWnd);

	// Коррекция для Quake/Inside/Desktop
	ConEmuWindowMode NewMode = IsSizePosFree(inMode) ? inMode : wmNormal;

	// И сразу запоминаем _новый_ режим
	changeFromWindowMode = WindowMode;
	mp_ConEmu->mp_Menu->SetRestoreFromMinimized(bIconic);
	if (gpSet->isQuakeStyle)
	{
		gpSet->_WindowMode = inMode;
		WindowMode = wmNormal;
	}
	else
	{
		WindowMode = inMode;
	}


	CVConGuard VCon;
	mp_ConEmu->GetActiveVCon(&VCon);
	//CRealConsole* pRCon = gpSet->isLogging() ? (VCon.VCon() ? VCon.VCon()->RCon() : NULL) : NULL;

	swprintf_c(szInfo, L"SetWindowMode exec: NewMode=%s", GetWindowModeName(NewMode));
	if (!LogString(szInfo)) { DEBUGSTRWMODE(szInfo); }

	// 180130 - Moved after actual mode change, because this introduces drawing glitches during maximization (toolbar is painted in wrong position, etc.)
	//mp_ConEmu->RefreshWindowStyles(); // inMode из параметров убрал, т.к. WindowMode уже изменен

	DEBUGTEST(BOOL bIsVisible = IsWindowVisible(ghWnd););

	//!!!
	switch (NewMode)
	{
		case wmNormal:
		{
			DEBUGLOGFILE("SetWindowMode(wmNormal)\n");
			//RECT consoleSize;

			bool bTiled = (m_TileMode == cwc_TileLeft || m_TileMode == cwc_TileRight || m_TileMode == cwc_TileHeight || m_TileMode == cwc_TileWidth);
			RECT rcIdeal = (gpSet->isQuakeStyle || !bTiled) ? GetDefaultRect() : GetIdealRect();

			if (!bTiled && !mp_ConEmu->isInside())
			{
				POINT ptOffset;
				if (!IsRectMinimized(mrc_StoredNormalRect))
				{
					ptOffset = POINT{mrc_StoredNormalRect.left, mrc_StoredNormalRect.top};
				}
				else
				{
					ptOffset = POINT{mi.mi.rcWork.left, mi.mi.rcWork.top};
				}

				rcIdeal = RECT{ptOffset.x, ptOffset.y, ptOffset.x + RectWidth(rcIdeal), ptOffset.y + RectHeight(rcIdeal)};
				FixWindowRect(rcIdeal, CEB_ALL|CEB_ALLOW_PARTIAL);
			}

			AutoSizeFont(rcIdeal, CER_MAIN);

			// Рассчитать размер по оптимальному WindowRect
			if (!CVConGroup::PreReSize(inMode, rcIdeal))
			{
				mp_ConEmu->mp_Menu->SetPassSysCommand(false);
				goto wrap;
			}

			//consoleSize = MakeRect(this->wndWidth, this->wndHeight);

			// Тут именно "::IsZoomed(ghWnd)"
			if (isIconic() || ::IsZoomed(ghWnd))
			{
				//apiShow Window(ghWnd, SW_SHOWNORMAL); // WM_SYSCOMMAND использовать не хочется...
				MSetter lSet(&mn_IgnoreSizeChange);

				if (mp_ConEmu->opt.DesktopMode)
				{
					RECT rcNormal = CalcRect(CER_RESTORE, MakeRect(0,0), CER_RESTORE);
					DWORD_PTR dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE);
					if (dwStyle & WS_MAXIMIZE)
						mp_ConEmu->SetWindowStyle(dwStyle&~WS_MAXIMIZE);
					setWindowPos(HWND_TOP, rcNormal.left, rcNormal.top, rcNormal.right-rcNormal.left, rcNormal.bottom-rcNormal.top, SWP_NOCOPYBITS|SWP_SHOWWINDOW);
				}
				else if (IsWindowVisible(ghWnd))
				{
					LPCWSTR pszInfo = L"WM_SYSCOMMAND(SC_RESTORE)";
					if (!LogString(pszInfo)) { DEBUGSTRWMODE(pszInfo); }

					DefWindowProc(ghWnd, WM_SYSCOMMAND, SC_RESTORE, 0); //2009-04-22 Было SendMessage
				}
				else
				{
					LPCWSTR pszInfo = L"ShowMainWindow(SW_SHOWNORMAL)";
					if (!LogString(pszInfo)) { DEBUGSTRWMODE(pszInfo); }

					ShowMainWindow(SW_SHOWNORMAL, abFirstShow);
				}

				LPCWSTR pszInfo = L"OnSize(false).1";
				if (!LogString(pszInfo)) { DEBUGSTRWMODE(pszInfo); }

			}

			#ifdef _DEBUG
			WINDOWPLACEMENT wpl = {sizeof(wpl)}; GetWindowPlacement(ghWnd, &wpl);
			#endif

			mp_ConEmu->RefreshWindowStyles();

			setWindowPos(NULL, rcIdeal.left, rcIdeal.top, RectWidth(rcIdeal), RectHeight(rcIdeal), SWP_NOZORDER);

			#ifdef _DEBUG
			GetWindowPlacement(ghWnd, &wpl);
			#endif

			gpSetCls->UpdateWindowMode(wmNormal);

			if (!IsWindowVisible(ghWnd))
			{
				ShowMainWindow((abFirstShow && mp_ConEmu->WindowStartMinimized) ? SW_SHOWMINNOACTIVE : SW_SHOWNORMAL, abFirstShow);
			}

			#ifdef _DEBUG
			GetWindowPlacement(ghWnd, &wpl);
			#endif

			// On startup, before first ::ShowWindow, isIconic returns FALSE
			// And we need real IsZoomed (FullScreen is Zoomed too)
			if (!(abFirstShow && mp_ConEmu->WindowStartMinimized) && (isIconic() || ::IsZoomed(ghWnd)))
			{
				ShowMainWindow(SW_SHOWNORMAL, abFirstShow);
			}

			// Already restored, need to clear the flag to avoid incorrect sizing
			mp_ConEmu->mp_Menu->SetRestoreFromMinimized(false);

			UpdateWindowRgn();
			OnSize(false); // подровнять ТОЛЬКО дочерние окошки

			//#ifdef _DEBUG
			//GetWindowPlacement(ghWnd, &wpl);
			//UpdateWindow(ghWnd);
			//#endif
		} break; //wmNormal

		case wmMaximized:
		{
			DEBUGLOGFILE("SetWindowMode(wmMaximized)\n");
			_ASSERTE(gpSet->isQuakeStyle==0); // Must not get here for Quake mode

			bool resetSize = false;
			RECT rcMax = CalcRect(CER_MAXIMIZED);

			WARNING("Может обломаться из-за максимального размера консоли");
			// в этом случае хорошо бы установить максимально возможный и отцентрировать ее в ConEmu
			if (!CVConGroup::PreReSize(inMode, rcMax))
			{
				mp_ConEmu->mp_Menu->SetPassSysCommand(false);
				goto wrap;
			}

			//if (mb_MaximizedHideCaption || !::IsZoomed(ghWnd))
			if (bIconic || (changeFromWindowMode != wmMaximized))
			{
				MSetter lSet(&mn_IgnoreSizeChange);

				mp_ConEmu->InvalidateAll();

				if (!mp_ConEmu->opt.DesktopMode)
				{
					DEBUGTEST(WINDOWPLACEMENT wpl1 = {sizeof(wpl1)}; GetWindowPlacement(ghWnd, &wpl1););
					ShowMainWindow(SW_SHOWMAXIMIZED, abFirstShow);
					DEBUGTEST(WINDOWPLACEMENT wpl2 = {sizeof(wpl2)}; GetWindowPlacement(ghWnd, &wpl2););
					if (changeFromWindowMode == wmFullScreen)
					{
						setWindowPos(HWND_TOP,
							rcMax.left, rcMax.top,
							rcMax.right-rcMax.left, rcMax.bottom-rcMax.top,
							SWP_NOCOPYBITS|SWP_SHOWWINDOW);
					}
				}
				else
				{
					/*WINDOWPLACEMENT wpl = {sizeof(WINDOWPLACEMENT)};
					GetWindowPlacement(ghWnd, &wpl);
					wpl.flags = 0;
					wpl.showCmd = SW_SHOWMAXIMIZED;
					wpl.ptMaxPosition.x = rcMax.left;
					wpl.ptMaxPosition.y = rcMax.top;
					SetWindowPlacement(ghWnd, &wpl);*/
					DWORD_PTR dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE);
					mp_ConEmu->SetWindowStyle(dwStyle|WS_MAXIMIZE);
					setWindowPos(HWND_TOP, rcMax.left, rcMax.top, rcMax.right-rcMax.left, rcMax.bottom-rcMax.top, SWP_NOCOPYBITS|SWP_SHOWWINDOW);
				}
				lSet.Unlock();
				_ASSERTE(mn_IgnoreSizeChange>=0);

				//RePaint();
				mp_ConEmu->InvalidateAll();

				LPCWSTR pszInfo = L"OnSize(false).2";
				if (!LogString(pszInfo)) { DEBUGSTRWMODE(pszInfo); }

				//OnSize(false); // консоль уже изменила свой размер

				gpSetCls->UpdateWindowMode(wmMaximized);
			} // if (!gpSet->isHideCaption)
			else
			{
				resetSize = true;
			}

			_ASSERTE(mn_IgnoreSizeChange>=0);

			if (!IsWindowVisible(ghWnd))
			{
				MSetter lSet(&mn_IgnoreSizeChange);
				resetSize = true;
				ShowMainWindow((abFirstShow && mp_ConEmu->WindowStartMinimized) ? SW_SHOWMINNOACTIVE : SW_SHOWMAXIMIZED, abFirstShow);

				LPCWSTR pszInfo = L"OnSize(false).3";
				if (!LogString(pszInfo)) { DEBUGSTRWMODE(pszInfo); }
			}

			_ASSERTE(mn_IgnoreSizeChange>=0);

			mp_ConEmu->RefreshWindowStyles();

			if (resetSize)
			{
				setWindowPos(NULL, rcMax.left, rcMax.top, rcMax.right-rcMax.left, rcMax.bottom-rcMax.top, SWP_NOCOPYBITS|SWP_NOZORDER);
			}

			// Already restored, need to clear the flag to avoid incorrect sizing
			mp_ConEmu->mp_Menu->SetRestoreFromMinimized(false);

			OnSize(false); // консоль уже изменила свой размер

			UpdateWindowRgn();

		} break; //wmMaximized

		case wmFullScreen:
		{
			DEBUGLOGFILE("SetWindowMode(wmFullScreen)\n");
			_ASSERTE(gpSet->isQuakeStyle==0); // Must not get here for Quake mode

			RECT rcMax = CalcRect(CER_FULLSCREEN);

			WARNING("Может обломаться из-за максимального размера консоли");
			// в этом случае хорошо бы установить максимально возможный и отцентрировать ее в ConEmu
			if (!CVConGroup::PreReSize(inMode, rcMax))
			{
				mp_ConEmu->mp_Menu->SetPassSysCommand(false);
				goto wrap;
			}

			//mb_isFullScreen = true;
			//WindowMode = inMode; // Запомним!

			//120820 - т.к. FullScreen теперь SW_SHOWMAXIMIZED, то здесь нужно смотреть на WindowMode
			isWndNotFSMaximized = (changeFromWindowMode == wmMaximized);

			RECT rcShift = CalcMargins(CEM_FRAMEONLY);
			_ASSERTE(rcShift.left==0 && rcShift.right==0 && rcShift.top==0 && rcShift.bottom==0);

			//GetCWShift(ghWnd, &rcShift); // Обновить, на всякий случай
			// Умолчания
			ptFullScreenSize.x = GetSystemMetrics(SM_CXSCREEN)+rcShift.left+rcShift.right;
			ptFullScreenSize.y = GetSystemMetrics(SM_CYSCREEN)+rcShift.top+rcShift.bottom;
			// которые нужно уточнить для текущего монитора!
			MONITORINFO mi = {};
			HMONITOR hMon = GetNearestMonitor(&mi);

			if (hMon)
			{
				ptFullScreenSize.x = (mi.rcMonitor.right-mi.rcMonitor.left)+rcShift.left+rcShift.right;
				ptFullScreenSize.y = (mi.rcMonitor.bottom-mi.rcMonitor.top)+rcShift.top+rcShift.bottom;
			}

			//120820 - Тут нужно проверять реальный IsZoomed
			if (isIconic() || !::IsZoomed(ghWnd))
			{
				MSetter lSet(&mn_IgnoreSizeChange);
				//120820 для четкости, в FullScreen тоже ставим Maximized, а не Normal
				ShowMainWindow((abFirstShow && mp_ConEmu->WindowStartMinimized) ? SW_SHOWMINNOACTIVE : SW_SHOWMAXIMIZED, abFirstShow);

				//// Сбросить
				//if (mb_MaximizedHideCaption)
				//	mb_MaximizedHideCaption = FALSE;

				//120820 - лишняя перерисовка?
				//-- RePaint();
			}

			_ASSERTE(mn_IgnoreSizeChange>=0);

			bWasSetFullscreen = SUCCEEDED(mp_ConEmu->Taskbar_MarkFullscreenWindow(ghWnd, TRUE));

			mp_ConEmu->RefreshWindowStyles();

			// for virtual screens mi.rcMonitor. may contain negative values...

			// Already restored, need to clear the flag to avoid incorrect sizing
			mp_ConEmu->mp_Menu->SetRestoreFromMinimized(false);

			setWindowPos(NULL,
			                -rcShift.left+mi.rcMonitor.left,-rcShift.top+mi.rcMonitor.top,
			                ptFullScreenSize.x,ptFullScreenSize.y,
			                SWP_NOZORDER);

			OnSize(false); // Align children windows

			UpdateWindowRgn();

			gpSetCls->UpdateWindowMode(wmFullScreen);

			if (!IsWindowVisible(ghWnd))
			{
				MSetter lSet(&mn_IgnoreSizeChange);
				ShowMainWindow((abFirstShow && mp_ConEmu->WindowStartMinimized) ? SW_SHOWMINNOACTIVE : SW_SHOWNORMAL, abFirstShow);
				lSet.Unlock();
				//WindowMode = inMode; // Запомним!

				LPCWSTR pszInfo = L"OnSize(false).5";
				if (!LogString(pszInfo)) { DEBUGSTRWMODE(pszInfo); }

				OnSize(false); // подровнять ТОЛЬКО дочерние окошки
			}

			_ASSERTE(mn_IgnoreSizeChange>=0);

			#ifdef _DEBUG
			bool bZoomed = _bool(::IsZoomed(ghWnd));
			_ASSERTE(bZoomed);
			#endif

		} break; //wmFullScreen

	default:
		_ASSERTE(FALSE && "Unsupported mode");
	}

	LPCWSTR pszInfo = L"SetWindowMode done";
	if (!LogString(pszInfo)) { DEBUGSTRWMODE(pszInfo); }

	//WindowMode = inMode; // Запомним!

	//if (VCon.VCon())
	//{
	//	VCon->RCon()->SyncGui2Window();
	//}

	//if (gpSetCls->GetPage(thi_SizePos))
	//{
	//	bool canEditWindowSizes = (inMode == wmNormal);
	//	HWND hSizePos = gpSetCls->GetPage(thi_SizePos);
	//	EnableWindow(GetDlgItem(hSizePos, tWndWidth), canEditWindowSizes);
	//	EnableWindow(GetDlgItem(hSizePos, tWndHeight), canEditWindowSizes);
	//}

	if (gpSet->isLogging())
	{
		wchar_t szInfo[64];
		swprintf_c(szInfo, L"SetWindowMode(%u) end", inMode);
		if (!mp_ConEmu->LogWindowPos(szInfo)) { DEBUGSTRWMODE(szInfo); }
	}

	//Sync ConsoleToWindow(); 2009-09-10 А это вроде вообще не нужно - ресайз консоли уже сделан
	mp_ConEmu->mp_Menu->SetPassSysCommand(false);
	lbRc = true;
wrap:
	if (!lbRc)
	{
		LPCWSTR pszInfo = L"Failed to switch WindowMode";
		if (!LogString(pszInfo)) { DEBUGSTRWMODE(pszInfo); }
		_ASSERTE(lbRc && "Failed to switch WindowMode");
		WindowMode = changeFromWindowMode;
	}
	changeFromWindowMode = wmNotChanging;
	mp_ConEmu->mp_Menu->SetRestoreFromMinimized(false);

	// В случае облома изменения размера консоли - может слететь признак
	// полноэкранности у панели задач. Вернем его...
	if (mp_ConEmu->mp_TaskBar2)
	{
		bNewFullScreen = (WindowMode == wmFullScreen)
			|| (gpSet->isQuakeStyle && (gpSet->_WindowMode == wmFullScreen));
		if (bWasSetFullscreen != bNewFullScreen)
		{
			mp_ConEmu->Taskbar_MarkFullscreenWindow(ghWnd, bNewFullScreen);

			bWasSetFullscreen = bNewFullScreen;
		}
	}

	mp_ConEmu->mp_TabBar->OnWindowStateChanged();

	#ifdef _DEBUG
	DWORD_PTR dwStyleDbg2 = GetWindowLongPtr(ghWnd, GWL_STYLE);
	#endif

	// Transparency styles may be changed occasionally, check them
	mp_ConEmu->OnTransparent();

	TODO("Что-то курсор иногда остается APPSTARTING...");
	SetCursor(LoadCursor(NULL,IDC_ARROW));
	//PostMessage(ghWnd, WM_SETCURSOR, -1, -1);
	return lbRc;
}

// abCorrect2Ideal==TRUE приходит из TabBarClass::UpdatePosition(),
// это происходит при отображении/скрытии "автотабов"
// или при смене шрифта
void CConEmuSize::ReSize(bool abCorrect2Ideal /*= false*/)
{
	if (isIconic())
		return;

	RECT client = ClientRect();

	#ifdef _DEBUG
	DWORD_PTR dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE);
	#endif

	if (abCorrect2Ideal)
	{
		if (isWindowNormal())
		{
			// Вобщем-то интересует только X,Y
			RECT rcWnd = {}; GetWindowRect(ghWnd, &rcWnd);

			// Пользователи жалуются на смену размера консоли

			#if 0
			// -- не годится. при запуске новой консоли и автопоказе табов
			// -- размер "CVConGroup::AllTextRect()" --> {0x0}
			RECT rcConsole = {}, rcCompWnd = {};
			TODO("DoubleView: нужно с учетом видимых консолей");
			if (isVConExists(0))
			{
				rcConsole = CVConGroup::AllTextRect();
				rcCompWnd = CalcRect(CER_MAIN, rcConsole, CER_CONSOLE_ALL);
				//AutoSizeFont(rcCompWnd, CER_MAIN);
			}
			else
			{
				rcCompWnd = rcWnd; // не менять?
			}
			#endif

			// Do it always, because of abCorrect2Ideal, even if the size already match
			RECT rcIdeal = GetIdealRect();
			AutoSizeFont(rcIdeal, CER_MAIN);
			RECT rcConsole = CalcRect(CER_CONSOLE_ALL, rcIdeal, CER_MAIN);
			RECT rcCompWnd = CalcRect(CER_MAIN, rcConsole, CER_CONSOLE_ALL);

			// При показе/скрытии табов высота консоли может "прыгать"
			// Ее нужно скорректировать. Поскольку идет реальный ресайз
			// главного окна - OnSize вызовется автоматически
			_ASSERTE(isMainThread());

			CVConGroup::PreReSize(WindowMode, rcCompWnd, CER_MAIN, true);

			int iCompWidth = (rcCompWnd.right - rcCompWnd.left), iCompHeight = (rcCompWnd.bottom - rcCompWnd.top);
			setWindowPos(NULL, rcWnd.left, rcWnd.top, iCompWidth, iCompHeight, SWP_NOZORDER);
		}
		else
		{
			CVConGroup::PreReSize(WindowMode, client, CER_MAINCLIENT, true);
		}

		// Поправить! Может изменилось!
		client = ClientRect();
	}

	DEBUGTEST(dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE));

	OnSize(true, isZoomed() ? SIZE_MAXIMIZED : SIZE_RESTORED);

	if (abCorrect2Ideal)
	{
	}
}

void CConEmuSize::OnConsoleResize()
{
	TODO("На удаление. ConEmu не должен дергаться при смене размера ИЗ КОНСОЛИ");
	//MSetter lInConsoleResize(&mb_InConsoleResize);
	// Выполняться должно в нити окна, иначе можем повиснуть
	static bool lbPosted = false;

	if (!isMainThread())
	{
		if (gpSet->isLogging())
			LogString(L"OnConsoleResize(abPosted==false)", TRUE);

		if (!lbPosted)
		{
			lbPosted = true; // чтобы post не накапливались
			//#ifdef _DEBUG
			//int nCurConWidth = (int)CVConGroup::TextWidth();
			//int nCurConHeight = (int)CVConGroup::TextHeight();
			//#endif
			PostMessage(ghWnd, mp_ConEmu->mn_PostConsoleResize, 0,0);
		}

		return;
	}

	lbPosted = false;

	if (isIconic())
	{
		if (gpSet->isLogging())
			LogString(L"OnConsoleResize ignored, because of iconic");

		return; // если минимизировано - ничего не делать
	}

	// Было ли реальное изменение размеров?
	bool lbSizingToDo  = (mp_ConEmu->mouse.state & MOUSE_SIZING_TODO) == MOUSE_SIZING_TODO;
	bool lbIsSizing = isSizing();
	bool lbLBtnPressed = isPressed(VK_LBUTTON);

	if (lbIsSizing && !lbLBtnPressed)
	{
		// Сброс всех флагов ресайза мышкой
		ResetSizingFlags(MOUSE_SIZING_BEGIN|MOUSE_SIZING_TODO);
	}

	if (gpSet->isLogging())
	{
		wchar_t szInfo[255];
		swprintf_c(szInfo, L"OnConsoleResize: mouse.state=0x%08X, SizingToDo=%i, IsSizing=%i, LBtnPressed=%i, PostUpdateWindowSize=%i",
		                            mp_ConEmu->mouse.state, (int)lbSizingToDo, (int)lbIsSizing, (int)lbLBtnPressed, (int)0/*mb_PostUpdateWindowSize*/);
		LogString(szInfo, TRUE);
	}

	CVConGroup::OnConsoleResize(lbSizingToDo);
}

void CConEmuSize::OnSizePanels(COORD cr)
{
	INPUT_RECORD r;
	int nRepeat = 0;
	wchar_t szKey[32];
	bool bShifted = (mp_ConEmu->mouse.state & MOUSE_DRAGPANEL_BOTH) != MOUSE_DRAGPANEL_BOTH;
	CRealConsole* pRCon = NULL;
	CVConGuard VCon;
	if (mp_ConEmu->GetActiveVCon(&VCon) >= 0)
		pRCon = VCon->RCon();

	if (!pRCon)
	{
		mp_ConEmu->mouse.state &= ~MOUSE_DRAGPANEL_ALL;
		return; // Некорректно, консоли нет
	}

	int nConWidth = pRCon->TextWidth();
	// Поскольку реакция на CtrlLeft/Right... появляется с задержкой - то
	// получение rcPanel - просто для проверки ее наличия!
	{
		RECT rcPanel;

		if (!pRCon->GetPanelRect(((mp_ConEmu->mouse.state & (MOUSE_DRAGPANEL_RIGHT|MOUSE_DRAGPANEL_SPLIT)) != 0), &rcPanel, true))
		{
			// Во время изменения размера панелей соответствующий Rect может быть сброшен?
#ifdef _DEBUG
			if (mp_ConEmu->mouse.state & MOUSE_DRAGPANEL_SPLIT)
			{
				DEBUGSTRPANEL2(L"PanelDrag: Skip of NO right panel\n");
			}
			else
			{
				DEBUGSTRPANEL2((mp_ConEmu->mouse.state & MOUSE_DRAGPANEL_RIGHT) ? L"PanelDrag: Skip of NO right panel\n" : L"PanelDrag: Skip of NO left panel\n");
			}

#endif
			return;
		}
	}
	r.EventType = KEY_EVENT;
	r.Event.KeyEvent.dwControlKeyState = 0x128; // Потом добавить SHIFT_PRESSED, если нужно...
	r.Event.KeyEvent.wVirtualKeyCode = 0;

	// Сразу запомним изменение положения, чтобы не "колбасило" туда-сюда,
	// пока фар не отработает изменение положения

	if (mp_ConEmu->mouse.state & MOUSE_DRAGPANEL_SPLIT)
	{
		//FAR BUGBUG: При наличии часов в заголовке консоли и нечетной ширине окна
		// слетает заголовок правой панели, если она уже 11 символов. Поставим минимум 12
		if (cr.X >= (nConWidth-13))
			cr.X = std::max<int>((nConWidth-12), mp_ConEmu->mouse.LClkCon.X);

		//rcPanel.left = mp_ConEmu->mouse.LClkCon.X; -- делал для макро
		mp_ConEmu->mouse.LClkCon.Y = cr.Y;

		if (cr.X < mp_ConEmu->mouse.LClkCon.X)
		{
			r.Event.KeyEvent.wVirtualKeyCode = VK_LEFT;
			nRepeat = mp_ConEmu->mouse.LClkCon.X - cr.X;
			mp_ConEmu->mouse.LClkCon.X = cr.X; // std::max(cr.X, (mp_ConEmu->mouse.LClkCon.X-1));
			wcscpy_c(szKey, L"CtrlLeft");
		}
		else if (cr.X > mp_ConEmu->mouse.LClkCon.X)
		{
			r.Event.KeyEvent.wVirtualKeyCode = VK_RIGHT;
			nRepeat = cr.X - mp_ConEmu->mouse.LClkCon.X;
			mp_ConEmu->mouse.LClkCon.X = cr.X; // std::min(cr.X, (mp_ConEmu->mouse.LClkCon.X+1));
			wcscpy_c(szKey, L"CtrlRight");
		}
	}
	else
	{
		//rcPanel.bottom = mp_ConEmu->mouse.LClkCon.Y; -- делал для макро
		mp_ConEmu->mouse.LClkCon.X = cr.X;

		if (cr.Y < mp_ConEmu->mouse.LClkCon.Y)
		{
			r.Event.KeyEvent.wVirtualKeyCode = VK_UP;
			nRepeat = mp_ConEmu->mouse.LClkCon.Y - cr.Y;
			mp_ConEmu->mouse.LClkCon.Y = cr.Y; // std::max(cr.Y, (mp_ConEmu->mouse.LClkCon.Y-1));
			wcscpy_c(szKey, bShifted ? L"CtrlShiftUp" : L"CtrlUp");
		}
		else if (cr.Y > mp_ConEmu->mouse.LClkCon.Y)
		{
			r.Event.KeyEvent.wVirtualKeyCode = VK_DOWN;
			nRepeat = cr.Y - mp_ConEmu->mouse.LClkCon.Y;
			mp_ConEmu->mouse.LClkCon.Y = cr.Y; // std::min(cr.Y, (mp_ConEmu->mouse.LClkCon.Y+1));
			wcscpy_c(szKey, bShifted ? L"CtrlShiftDown" : L"CtrlDown");
		}

		if (bShifted)
		{
			// Тогда разделение на левую и правую панели
			TODO("Активировать нужную, иначе будет меняться размер активной панели, а хотели другую...");
			r.Event.KeyEvent.dwControlKeyState |= SHIFT_PRESSED;
		}
	}

	if (r.Event.KeyEvent.wVirtualKeyCode)
	{
		// Макрос вызывается после отпускания кнопки мышки
		if (gpSet->isDragPanel == 2)
		{
			if (pRCon->isFar(TRUE))
			{
				mp_ConEmu->mouse.LClkCon = cr;
				wchar_t szMacro[128]; szMacro[0] = L'@';

				if (pRCon->IsFarLua())
				{
					// "$Rep (%i)" -> "for RCounter= %i,1,-1 do Keys(\"%s\") end"
					if (nRepeat > 1)
						swprintf_c(szMacro+1, countof(szMacro)-1/*#SECURELEN*/, L"for RCounter= %i,1,-1 do Keys(\"%s\") end", nRepeat, szKey);
					else
						swprintf_c(szMacro+1, countof(szMacro)-1/*#SECURELEN*/, L"Keys(\"%s\")", szKey);
				}
				else
				{
					if (nRepeat > 1)
						swprintf_c(szMacro+1, countof(szMacro)-1/*#SECURELEN*/, L"$Rep (%i) %s $End", nRepeat, szKey);
					else
						_wcscpy_c(szMacro+1, countof(szMacro)-1, szKey);
				}

				pRCon->PostMacro(szMacro);
			}
		}
		else
		{
			#ifdef _DEBUG
			wchar_t szDbg[128]; swprintf_c(szDbg, L"PanelDrag: Sending '%s'\n", szKey);
			DEBUGSTRPANEL(szDbg);
			#endif
			// Макросом не будем - велика вероятность второго вызова, когда еще не закончилась обработка первого макро
			// Поехали
			r.Event.KeyEvent.wVirtualScanCode = MapVirtualKey(r.Event.KeyEvent.wVirtualKeyCode, 0/*MAPVK_VK_TO_VSC*/);
			r.Event.KeyEvent.wRepeatCount = nRepeat; //-- repeat - что-то глючит...
			//while (nRepeat-- > 0)
			{
				r.Event.KeyEvent.bKeyDown = TRUE;
				pRCon->PostConsoleEvent(&r);
				r.Event.KeyEvent.bKeyDown = FALSE;
				r.Event.KeyEvent.wRepeatCount = 1;
				r.Event.KeyEvent.dwControlKeyState = 0x120; // "Отожмем Ctrl|Shift"
				pRCon->PostConsoleEvent(&r);
			}
		}
	}
	else
	{
		DEBUGSTRPANEL2(L"PanelDrag: Skip of NO key selected\n");
	}
}

struct OnDpiChangedArg
{
	CConEmuSize* pObject;
	UINT DpiX, DpiY;
	RECT rcSuggested;
	bool bResizeWindow;
	bool bSuggested;
	DpiChangeSource src;

	OnDpiChangedArg(CConEmuSize* ap, UINT dx, UINT dy, LPRECT prc, bool abResize, DpiChangeSource asrc)
	{
		pObject = ap;
		DpiX = dx; DpiY = dy;
		bSuggested = (prc != NULL);
		if (prc)
			rcSuggested = *prc;
		else
			rcSuggested = {};
		bResizeWindow = abResize;
		src = asrc;
	};
};

LRESULT CConEmuSize::OnDpiChangedCall(LPARAM lParam)
{
	OnDpiChangedArg* p = (OnDpiChangedArg*)lParam;
	return p->pObject->OnDpiChanged(p->DpiX, p->DpiY, p->bSuggested ? &p->rcSuggested : NULL, p->bResizeWindow, p->src);
}

// Returns TRUE if DPI was really changed (compared with previous value)
LRESULT CConEmuSize::OnDpiChanged(UINT dpiX, UINT dpiY, LPRECT prcSuggested, bool bResizeWindow, DpiChangeSource src)
{
	LRESULT lRc = FALSE;

	wchar_t szInfo[255], szPrefix[40];
	RECT rc = {}; if (prcSuggested) rc = *prcSuggested;
	static UINT oldDpiX, oldDpiY;
	bool bChanged = (dpiX != oldDpiX) || (dpiY != oldDpiY);
	oldDpiX = dpiX; oldDpiY = dpiY;

	if (prcSuggested)
		SetRequestedMonitor(MonitorFromRect(prcSuggested, MONITOR_DEFAULTTONEAREST));

	swprintf_c(szPrefix, bChanged ? L"DpiChanged(%s)" : L"DpiNotChanged(%s)",
		(src == dcs_Api) ? L"Api" : (src == dcs_Macro) ? L"Mcr" : (src == dcs_Jump) ? L"Jmp" : (src == dcs_Snap) ? L"Snp" : L"Int");
	swprintf_c(szInfo, L"%s: dpi={%u,%u}, rect={%i,%i}-{%i,%i} (%ix%i)",
		szPrefix, dpiX, dpiY, LOGRECTCOORDS(rc), LOGRECTSIZE(rc));

	/*
	if (m_JumpMonitor.bInJump || mn_IgnoreSizeChange)
	{
		wcscat_c(szInfo, L" - SKIPPED because of moving");
		DEBUGSTRDPI(szInfo);
		LogString(szInfo, true);
		goto wrap;
	}
	*/

	// Если прыжок на другой монитор был сделан из ChangeTileMode (например)
	// то API может предложить некорректное значение в prcSuggested
	// Корректный размер уже установлен, на менять его
	if (IsWindowModeChanging())
	{
		prcSuggested = NULL;
		wcscat_c(szInfo, L" {SuggestedRect was dropped}");
	}

	// Работать нужно в основном потоке!
	if (!isMainThread())
	{
		OnDpiChangedArg args(this, dpiX, dpiY, prcSuggested, bResizeWindow, src);
		lRc = mp_ConEmu->CallMainThread(true, OnDpiChangedCall, (LPARAM)&args);
	}
	else
	{
		DEBUGSTRDPI(szInfo);
		LogString(szInfo, true);

		#ifdef _DEBUG
		if ((rc.bottom - rc.top) > 1200)
			_ASSERTE((rc.bottom - rc.bottom) == 0);
		#endif

		SizeInfo::RequestDpi(DpiValue(dpiX, dpiY));
		if (prcSuggested)
			SizeInfo::RequestRect(*prcSuggested);

		// it returns false if DPI was not changed
		if (gpFontMgr->RecreateFontByDpi(dpiX, dpiY, prcSuggested))
		{
			// Вернуть флажок
			lRc = TRUE;

			// Пересоздать тулбары для соответствия их размеров новому DPI
			RecreateControls(true/*bRecreateTabbar*/, true/*bRecreateStatus*/, bResizeWindow, prcSuggested);

			// Нельзя полагаться на то, что WM_SIZE вызовется сам
			OnSize();

			// Если вызывает наш внутренний Tile/Snap,
			// и новый режим "прилепленный" к краю экрана,
			// то не будет обновлен желаемый размер wmNormal консоли
			// 150816 - Called from JumpNextMonitor, EvalTileMode raises an assertion (IsWindowModeChanged)
			ConEmuWindowCommand Tile = (m_JumpMonitor.bInJump || IsRectEmpty(&rc))
				? m_TileMode
				: EvalTileMode(rc);
			if (Tile != cwc_Current)
			{
				RECT rcOld = IsRectMinimized(mrc_StoredNormalRect) ? GetDefaultRect() : mrc_StoredNormalRect;
				RECT rcNew = {}, rcNewFix = {};
				if (!IsRectEmpty(&rc))
					rcNew = rc;
				else
					GetWindowRect(ghWnd, &rcNew);
				MONITORINFO miOld = {}, miNew = {};
				HMONITOR hOld = GetNearestMonitorInfo(&miOld, NULL, &rcOld);
				HMONITOR hNew = GetNearestMonitorInfo(&miNew, NULL, &rcNew);
				if (hOld && hNew && (hOld != hNew))
				{
					// Сменился монитор, нужно обновить NormalRect
					EvalNewNormalPos(miOld, hNew, miNew, rcOld, rcNewFix);
					// И сохранить
					StoreNormalRect(&rcNewFix);
				}
			}
		}
	}

	return lRc;
}

LRESULT CConEmuSize::OnDisplayChanged(UINT bpp, UINT screenWidth, UINT screenHeight)
{
	wchar_t szInfo[255];
	swprintf_c(szInfo, L"WM_DISPLAYCHANGED: bpp=%u, size={%u,%u}\r\n",
		bpp, screenWidth, screenHeight);
	DEBUGSTRDPI(szInfo);
	LogString(szInfo, true, false);
	return 0;
}

void CConEmuSize::RecreateControls(bool bRecreateTabbar, bool bRecreateStatus, bool bResizeWindow, LPRECT prcSuggested /*= NULL*/)
{
	SizeInfo::RequestRecalc();

	if (bRecreateTabbar && mp_ConEmu->mp_TabBar)
		mp_ConEmu->mp_TabBar->Recreate();

	if (bRecreateStatus && mp_ConEmu->mp_Status)
		mp_ConEmu->mp_Status->UpdateStatusBar(true);

	if (bResizeWindow)
	{
		bool bNormal = IsSizePosFree() && !isZoomed() && !isFullScreen();
		if (bNormal)
		{
			// If Windows DWM sends new preferred RECT?
			if (prcSuggested && !IsRectEmpty(prcSuggested))
			{
				// Change window position if only prcSuggested
				// otherwise the size may be changed erroneously
				// during jumping between monitors with different
				// dpi properties (per-monitor-dpi)
				MoveWindowRect(ghWnd, *prcSuggested, TRUE);
			}
		}
		// Suggested size may be the same?
		OnSize();
		mp_ConEmu->InvalidateGaps();
	}
}

bool CConEmuSize::isIconic(bool abRaw /*= false*/)
{
	bool bIconic = _bool(::IsIconic(ghWnd));

	if (!bIconic && mp_ConEmu->mp_Inside)
		bIconic = mp_ConEmu->mp_Inside->isParentIconic();

	if (!abRaw && !bIconic)
	{
		bIconic = (gpSet->isQuakeStyle && isQuakeMinimized)
			// Don't assume "iconic" while creating window
			// otherwise "StoreNormalRect" will fail
			|| (!mp_ConEmu->InCreateWindow() && !IsWindowVisible(ghWnd));
		// Issue 1792: Win+D/D
		if (!bIconic)
		{
			RECT rc = {}; GetWindowRect(ghWnd, &rc);
			bIconic = IsRectMinimized(rc);
		}
	}
	return bIconic;
}

bool CConEmuSize::isWindowNormal()
{
	if ((GetWindowMode() != wmNormal) || !IsSizeFree())
		return false;
	if (isIconic())
		return false;

	return true;
}

bool CConEmuSize::isZoomed()
{
	if (GetWindowMode() != wmMaximized)
		return false;
	if (isIconic())
		return false;

	return true;
}

bool CConEmuSize::isFullScreen()
{
	if (GetWindowMode() != wmFullScreen)
		return false;
	if (isIconic())
		return false;

	return true;
}

bool CConEmuSize::setWindowPos(HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
	BOOL lbRc;

	bool bInCreate = mp_ConEmu->InCreateWindow();
	bool bQuake = gpSet->isQuakeStyle && !(mp_ConEmu->opt.DesktopMode || mp_ConEmu->mp_Inside);

	MSetter inSetWindowPos(&mn_InSetWindowPos);
	_ASSERTE(mn_InSetWindowPos<3);

	if (bInCreate)
	{
		if (mp_ConEmu->WindowStartTsa || (mp_ConEmu->WindowStartMinimized && gpSet->isMinToTray()))
		{
			uFlags |= SWP_NOACTIVATE;
			uFlags &= ~SWP_SHOWWINDOW;
		}

		if (bQuake)
		{
			SetIgnoreQuakeActivation(true);
		}
	}

	wchar_t szInfo[255];
	swprintf_c(szInfo, L"setWindowPos: {%i,%i} (%ix%i) Flags=x%X After=x%X", X, Y, cx, cy, uFlags, LODWORD(hWndInsertAfter));
	if (gpSet->isLogging())
	{
		LogString(szInfo);
	}
	else
	{
		DEBUGSTRSIZE(szInfo);
	}

	// Call API function, we are ready
	lbRc = ::SetWindowPos(ghWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);

	if (bInCreate && bQuake)
	{
		SetIgnoreQuakeActivation(false);
	}

	return (lbRc != FALSE);
}

void CConEmuSize::UpdateWindowRgn()
{
	if (mb_LockWindowRgn)
		return;

	// No sense to change WndRgn in Child mode
	if (mp_ConEmu->mp_Inside)
		return;

	HRGN hRgn = NULL;
	if (m_ForceShowFrame == fsf_Show && !mb_DisableThickFrame)
		hRgn = NULL; // allow nice themed buttons in the window caption (XP)
	else
		hRgn = CreateWindowRgn();

	if (hRgn)
	{
		mb_LastRgnWasNull = false;
	}
	else
	{
		bool lbPrev = mb_LastRgnWasNull;
		mb_LastRgnWasNull = true;

		if (lbPrev)
			return; // менять не нужно
	}

	bool do_log = RELEASEDEBUGTEST(gpSet->isLogging(), true);

	if (do_log)  // -V547
	{
		wchar_t szInfo[255];
		RECT rcBox = {};
		int nRgn = hRgn ? GetRgnBox(hRgn, &rcBox) : NULLREGION;
		swprintf_c(szInfo,
			nRgn ? L"SetWindowRgn(0x%08X, <%u> {%i,%i}-{%i,%i})" : L"SetWindowRgn(0x%08X, NULL)",
			ghWnd, nRgn, LOGRECTCOORDS(rcBox));
		if (!LogString(szInfo)) { DEBUGSTRRGN(szInfo); }
	}

	// Облом получается при двукратном SetWindowRgn(ghWnd, NULL, TRUE);
	// Причем после следующего ресайза - рамка приходит в норму
	bool b = mp_ConEmu->SetDontPreserveClient(true);
	BOOL bRc = SetWindowRgn(ghWnd, hRgn, TRUE);
	DWORD dwErr = bRc ? 0 : GetLastError();
	mp_ConEmu->SetDontPreserveClient(b);
	UNREFERENCED_PARAMETER(dwErr);
	UNREFERENCED_PARAMETER(bRc);
}

bool CConEmuSize::ShowMainWindow(int anCmdShow, bool abFirstShow /*= false*/)
{
	if (mb_LockShowWindow)
	{
		return false;
	}

	DWORD nAnimationMS = (((int)gpSet->nQuakeAnimation) >= 0) ? gpSet->nQuakeAnimation : QUAKEANIMATION_DEF;

	BOOL lbRc = FALSE;
	bool bUseApi = true;

	if (mp_ConEmu->InCreateWindow() && (mp_ConEmu->WindowStartTsa || (mp_ConEmu->WindowStartMinimized && gpSet->isMinToTray())))
	{
		_ASSERTE(anCmdShow == SW_SHOWMINNOACTIVE || anCmdShow == SW_HIDE);
		_ASSERTE(::IsWindowVisible(ghWnd) == FALSE);
		anCmdShow = SW_HIDE;
	}


	// In Quake mode - animation proceeds with SetWindowRgn
	//   yes, it is possible to use Slide, but ConEmu may hold
	//   many child windows (outprocess), which don't support
	//   WM_PRINTCLIENT. So, our choice - "trim" window with RGN
	// When Caption is visible - Windows animates window itself.
	// Also, animation brakes transparency
	DWORD nStyleEx = GetWindowLong(ghWnd, GWL_EXSTYLE);
	if (nAnimationMS && !gpSet->isQuakeStyle
		&& isCaptionHidden() && !(IsWin10() && !isSelfFrame())
		&& !(nStyleEx & WS_EX_LAYERED) )
	{
		// Well, Caption is hidden, Windows does not animates our window.
		if (anCmdShow == SW_HIDE)
		{
			bUseApi = false; // можно AnimateWindow
		}
		else if (anCmdShow == SW_SHOWMAXIMIZED)
		{
			if (::IsZoomed(ghWnd)) // тут нужно "RAW" значение zoomed
				bUseApi = false; // можно AnimateWindow
		}
		else if (anCmdShow == SW_SHOWNORMAL)
		{
			if (!::IsZoomed(ghWnd) && !::IsIconic(ghWnd)) // тут нужно "RAW" значение zoomed
				bUseApi = false; // можно AnimateWindow
		}
	}

	bool bOldShowMinimized = mb_InShowMinimized;
	mb_InShowMinimized = (anCmdShow == SW_SHOWMINIMIZED) || (anCmdShow == SW_SHOWMINNOACTIVE);

	if (bUseApi)
	{
		bool b, bAllowPreserve = false;
		if (mp_ConEmu->InCreateWindow())
		{
			bAllowPreserve = false;
		}
		else if (anCmdShow == SW_SHOWMAXIMIZED)
		{
			if (::IsZoomed(ghWnd)) // тут нужно "RAW" значение zoomed
				bAllowPreserve = true;
		}
		else if (anCmdShow == SW_SHOWNORMAL)
		{
			if (!::IsZoomed(ghWnd) && !::IsIconic(ghWnd)) // тут нужно "RAW" значение zoomed
				bAllowPreserve = true;
		}
		// Allowed?
		if (bAllowPreserve)
		{
			b = mp_ConEmu->SetAllowPreserveClient(true);
		}

		//GO
		lbRc = apiShowWindow(ghWnd, anCmdShow);

		if (abFirstShow
			&& (anCmdShow != SW_HIDE) && (anCmdShow != SW_SHOWMINIMIZED) && (anCmdShow != SW_MINIMIZE)
			&& (anCmdShow != SW_SHOWMINNOACTIVE) && (anCmdShow != SW_FORCEMINIMIZE))
		{
			::UpdateWindow(ghWnd);
		}
		else if (bAllowPreserve)
		{
			mp_ConEmu->SetAllowPreserveClient(b);
		}
	}
	else
	{
		// Use animation only when Caption is hidden.
		// Otherwise - artifacts comes: while animation - theming does not applies
		// (Win7, Glass - caption style changes while animation)
		_ASSERTE(isCaptionHidden());

		DWORD nFlags = AW_BLEND;
		if (anCmdShow == SW_HIDE)
			nFlags |= AW_HIDE;


		if (nAnimationMS > QUAKEANIMATION_MAX)
			nAnimationMS = QUAKEANIMATION_MAX;

		//if (nAnimationMS == 0)
		//	nAnimationMS = CONEMU_ANIMATE_DURATION;
		//else if (nAnimationMS > CONEMU_ANIMATE_DURATION_MAX)
		//	nAnimationMS = CONEMU_ANIMATE_DURATION_MAX;

		AnimateWindow(nAnimationMS, nFlags);
	}

	mb_InShowMinimized = bOldShowMinimized;

	return (lbRc != FALSE);
}

BOOL CConEmuSize::AnimateWindow(DWORD dwTime, DWORD dwFlags)
{
	HWND hNextWindow = NULL;
	if ((dwFlags & AW_HIDE) && mp_ConEmu->isMeForeground(true, true))
	{
		hNextWindow = ghWnd;
		while ((hNextWindow = GetNextWindow(hNextWindow, GW_HWNDNEXT)) != NULL)
		{
			if (!CVConGroup::isOurWindow(hNextWindow)
				&& ::IsWindowVisible(hNextWindow))
				break;
		}
	}

	bool bWasShown = false;
	if (gpSet->isQuakeStyle)
	{
		bWasShown = mp_ConEmu->mp_TabBar->ShowSearchPane(false, true);
	}

	ThickFrameDisabler disabler(*this);

	// during quake-style slide - we turn off ThickFrame,
	// because it looks really weird during animation
	if (dwFlags & AW_SLIDE)
	{
		disabler.Disable();
	}

	BOOL bRc = ::AnimateWindow(ghWnd, dwTime, dwFlags);

	// Restore ThickFrame if we show the window
	if ((dwFlags & AW_SLIDE) && !(dwFlags & AW_HIDE))
	{
		disabler.Enable();
	}

	if (bWasShown)
	{
		mp_ConEmu->mp_TabBar->ShowSearchPane(true, true);
	}

	if (hNextWindow && (hNextWindow != ghWnd))
	{
		SetForegroundWindow(hNextWindow);
	}

	return bRc;
}

//void CConEmuSize::SetPostUpdateWindowSize(bool bValue)
//{
//	WARNING("Не используется, удалить");
//	mb_PostUpdateWindowSize = false;
//}

bool CConEmuSize::isSizing(UINT nMouseMsg/*=0*/)
{
	// Юзер тащит мышкой рамку окна
	if ((mp_ConEmu->mouse.state & MOUSE_SIZING_BEGIN) != MOUSE_SIZING_BEGIN)
		return false;

	// могло не сброситься, проверим
	if ((nMouseMsg==WM_NCLBUTTONUP) || !isPressed(VK_LBUTTON))
	{
		EndSizing(nMouseMsg);
		return false;
	}

	return true;
}

void CConEmuSize::BeginSizing()
{
	// Перенес снизу, а то ассерты вылезают
	SetSizingFlags(MOUSE_SIZING_BEGIN);

	if (gpSet->isHideCaptionAlways())
	{
		mp_ConEmu->SetKillTimer(false, TIMER_CAPTION_DISAPPEAR_ID, 0);
		mp_ConEmu->SetKillTimer(false, TIMER_CAPTION_APPEAR_ID, 0);
		// -- если уж пришел WM_NCLBUTTONDOWN - значит рамка уже показана?
		//mp_ConEmu->OnTimer(TIMER_CAPTION_APPEAR_ID,0);
		//UpdateWindowRgn();
	}
}

void CConEmuSize::SetSizingFlags(DWORD nSetFlags /*= MOUSE_SIZING_BEGIN*/)
{
	_ASSERTE((nSetFlags & (~(MOUSE_SIZING_BEGIN|MOUSE_SIZING_TODO))) == 0); // Допустимые флаги

	#ifdef _DEBUG
	if ((nSetFlags & MOUSE_SIZING_BEGIN) && !(mp_ConEmu->mouse.state & MOUSE_SIZING_BEGIN))
	{
		DEBUGSTRSIZE(L"SetSizingFlags(MOUSE_SIZING_BEGIN)");
	}
	#endif

	#ifdef _DEBUG
	if (!(mp_ConEmu->mouse.state & nSetFlags)) // For debug purposes (breakpoints)
	#endif
	mp_ConEmu->mouse.state |= nSetFlags;
}

void CConEmuSize::ResetSizingFlags(DWORD nDropFlags /*= MOUSE_SIZING_BEGIN|MOUSE_SIZING_TODO*/)
{
	_ASSERTE((nDropFlags & (~(MOUSE_SIZING_BEGIN|MOUSE_SIZING_TODO))) == 0); // Допустимые флаги

	#ifdef _DEBUG
	if ((nDropFlags & MOUSE_SIZING_BEGIN) && (mp_ConEmu->mouse.state & MOUSE_SIZING_BEGIN))
	{
		DEBUGSTRSIZE(L"ResetSizingFlags(MOUSE_SIZING_BEGIN)");
	}
	#endif

	#ifdef _DEBUG
	if ((mp_ConEmu->mouse.state & nDropFlags)) // For debug purposes (breakpoints)
	#endif
	mp_ConEmu->mouse.state &= ~nDropFlags;
}

void CConEmuSize::EndSizing(UINT nMouseMsg/*=0*/)
{
	bool bApplyResize = false;

	if (mp_ConEmu->mouse.state & MOUSE_SIZING_TODO)
	{
		// Если тянули мышкой - изменение размера консоли могло быть "отложено" до ее отпускания
		bApplyResize = true;
	}

	ResetSizingFlags(MOUSE_SIZING_BEGIN|MOUSE_SIZING_TODO);

	if (bApplyResize)
	{
		LogString(L"EndSizing: Sync console sizes after resize");
		CVConGroup::SyncConsoleToWindow();
	}

	if (!isIconic())
	{
		if (m_TileMode != cwc_Current)
		{
			wchar_t szTile[255], szOldTile[32];
			swprintf_c(szTile, L"Tile mode was stopped on resizing: Was=%s, New=cwc_Current",
				FormatTileMode(m_TileMode,szOldTile,countof(szOldTile)));
			LogString(szTile);

			GetTileMode(true);

			mp_ConEmu->UpdateProcessDisplay(false);
		}

		// Сама разберется, что надо/не надо
		StoreNormalRect(NULL);
		// Теоретически, в некоторых случаях размер окна может (?) измениться с задержкой,
		// в следующем цикле таймера - проверить размер
		mp_ConEmu->mouse.bCheckNormalRect = true;
	}

	if (IsWindowVisible(ghWnd) && !isIconic())
	{
		CVConGroup::NotifyChildrenWindows();
	}
}

void CConEmuSize::CheckTopMostState()
{
	static bool bInCheck = false;
	if (bInCheck)
		return;

	bInCheck = true;

	DWORD dwStyleEx = GetWindowLong(ghWnd, GWL_EXSTYLE);

	if (!gpSet->isAlwaysOnTop && ((dwStyleEx & WS_EX_TOPMOST) == WS_EX_TOPMOST))
	{
		if (gpSet->isLogging())
		{
			wchar_t szInfo[255];
			RECT rcWnd = {}; GetWindowRect(ghWnd, &rcWnd);
			swprintf_c(szInfo,
				L"Some external program brought ConEmu OnTop: HWND=x%08X, StyleEx=x%08X, Rect={%i,%i}-{%i,%i}",
				LODWORD(ghWnd), dwStyleEx, LOGRECTCOORDS(rcWnd));
			LogString(szInfo);
		}

		if (!gpSet->isDownShowExOnTopMessage
			&& (IDYES == MsgBox(L"Some external program brought ConEmu OnTop\nRevert?", MB_SYSTEMMODAL|MB_ICONQUESTION|MB_YESNO))
			)
		{
			//SetWindowStyleEx(dwStyleEx & ~WS_EX_TOPMOST);
			DoAlwaysOnTopSwitch();
		}
		else
		{
			gpSet->isAlwaysOnTop = true;
		}
	}

	bInCheck = false;
}

void CConEmuSize::StartForceShowFrame()
{
	LPCWSTR pszLogMsg = L"StartForceShowFrame called";
	if (!mp_ConEmu->LogString(pszLogMsg)) { DEBUGSTRSTYLE(pszLogMsg); }
	_ASSERTE(gpConEmu->isSelfFrame());
	mp_ConEmu->SetKillTimer(false, TIMER_CAPTION_APPEAR_ID, 0);
	mp_ConEmu->SetKillTimer(false, TIMER_CAPTION_DISAPPEAR_ID, 0);
	m_ForceShowFrame = fsf_Show;
	mp_ConEmu->RefreshWindowStyles();
	//UpdateWindowRgn();
}

void CConEmuSize::StopForceShowFrame()
{
	LPCWSTR pszLogMsg = L"StopForceShowFrame called";
	if (!mp_ConEmu->LogString(pszLogMsg)) { DEBUGSTRSTYLE(pszLogMsg); }
	m_ForceShowFrame = fsf_Hide;
	mp_ConEmu->SetKillTimer(false, TIMER_CAPTION_APPEAR_ID, 0);
	mp_ConEmu->SetKillTimer(false, TIMER_CAPTION_DISAPPEAR_ID, 0);
	mp_ConEmu->RefreshWindowStyles();
	//UpdateWindowRgn();
}

void CConEmuSize::DisableThickFrame(bool flag)
{
	if (flag)
	{
		mb_DisableThickFrame = true;
		DEBUGSTRRGN(L"mb_DisableThickFrame = true");
		mp_ConEmu->StopForceShowFrame();
		//mp_ConEmu->UpdateWindowRgn(); // already called from StopForceShowFrame/RefreshWindowStyles
	}
	else
	{
		DEBUGSTRRGN(L"mb_DisableThickFrame = false");
		mb_DisableThickFrame = false;
		mp_ConEmu->RefreshWindowStyles();
		// mp_ConEmu->UpdateWindowRgn(); // already called from RefreshWindowStyles
	}
}

bool CConEmuSize::InMinimizing(WINDOWPOS *p /*= NULL*/)
{
	if (mb_InShowMinimized)
		return true;
	if (mp_ConEmu->mp_Menu && mp_ConEmu->mp_Menu->GetInScMinimize())
		return true;

	// Last chance to check (come from OnWindowPosChanging)
	if (p && IsRectMinimized(p->x, p->y))
		return true;

	if (mp_ConEmu->mp_Inside && mp_ConEmu->mp_Inside->inMinimizing(p))
		return true;

	#ifdef _DEBUG
	RECT rc = {}; GetWindowRect(ghWnd, &rc);
	if (IsRectMinimized(rc))
	{
		_ASSERTE((rc.left > -32000 && rc.top > -32000) || (p && (p->x > -32000 && p->y > -32000)));
	}
	#endif

	return false;
}

HRGN CConEmuSize::CreateWindowRgn()
{
	if (mp_ConEmu->isInside())
	{
		_ASSERTE(FALSE && "Don't set WndRgn in Inside modes");
		return NULL;
	}

	HRGN hRgn = NULL, hExclusion = NULL;

	TODO("DoubleView: Если видимы несколько консолей - нужно совместить регионы, или вообще отрубить, для простоты");

	hExclusion = CVConGroup::GetExclusionRgn();

	WARNING("Установка любого НЕ NULL региона сбивает XP темы при отрисовке кнопок в заголовке");

	// Ensure that parts of window frames will not be visible *under* the TaskBar or adjacent monitors
	if (!gpSet->isQuakeStyle
		&& ((WindowMode == wmFullScreen)
			// Условие именно такое (для isZoomed) - здесь регион ставится на весь монитор
			|| ((WindowMode == wmMaximized) && (mp_ConEmu->isSelfFrame() || hExclusion))))
	{
		auto mi = NearestMonitorInfo(NULL);
		RECT rcScreen = (WindowMode == wmFullScreen) ? mi.mi.rcMonitor : mi.mi.rcWork;
		RECT rcFrame = SizeInfo::FrameMargins();

		// rcFrame, т.к. регион ставится относительно верхнего левого угла ОКНА
		hRgn = CreateWindowRgn(false, rcFrame.left, rcFrame.top, rcScreen.right-rcScreen.left, rcScreen.bottom-rcScreen.top);
	}
	else
	{
		// Normal
		if (isSelfFrame() || mb_DisableThickFrame)
		{
			if ((mn_QuakePercent != 0)
				|| !mp_ConEmu->isMouseOverFrame()
				|| (gpSet->isQuakeStyle && (gpSet->HideCaptionAlwaysFrame() != 0)))
			{
				// Рамка невидима (мышка не над рамкой или заголовком)
				RECT rcClient = ClientRect();
				RECT rcFrame = FrameMargins();
				//_ASSERTE(!rcClient.left && !rcClient.top);

				bool bRoundTitle = (gOSVer.dwMajorVersion == 5) && gpSetCls->CheckTheming() && mp_ConEmu->isTabsShown();

				if (gpSet->isQuakeStyle)
				{
					if (mn_QuakePercent > 0)
					{
						int nPercent = (mn_QuakePercent > 100) ? 100 : (mn_QuakePercent == 1) ? 0 : mn_QuakePercent;
						int nQuakeHeight = (rcClient.bottom - rcClient.top + 1) * nPercent / 100;
						if (nQuakeHeight < 1)
						{
							nQuakeHeight = 1; // иначе регион не применится
							rcClient.right = rcClient.left + 1;
						}
						rcClient.bottom = std::min(rcClient.bottom, (rcClient.top+nQuakeHeight));
						bRoundTitle = false;
					}
					else
					{
						// Видимо все, но нужно "отрезать" верхнюю часть рамки
					}
				}


				// Take the value form gpSet because we need to apply it over calculated size
				int nFrame = gpSet->HideCaptionAlwaysFrame();
				bool bFullFrame = (nFrame < 0) && !mb_DisableThickFrame;
				if (bFullFrame)
				{
					nFrame = 0;
				}
				else
				{
					const int min_frame = std::min(rcFrame.left, rcFrame.top);
					if (nFrame > min_frame)
					{
						_ASSERTE(rcFrame.left>=nFrame);
						_ASSERTE(rcFrame.top>=nFrame);
						nFrame = min_frame;
					}
				}


				// We need coordinates relative to upper-top corner of WINDOW (not client area)
				int rgnX = bFullFrame ? 0 : (rcFrame.left - nFrame);
				int rgnY = bFullFrame ? 0 : (rcFrame.top - nFrame);
				int rgnX2 = (rcFrame.left - rgnX) + (bFullFrame ?  rcFrame.right : nFrame);
				int rgnY2 = (rcFrame.top - rgnY) + (bFullFrame ? rcFrame.bottom : nFrame);
				if (gpSet->isQuakeStyle)
				{
					if (gpSet->_WindowMode != wmNormal)
					{
						// ConEmu window is maximized to fullscreen or work area
						// Need to cut left/top/bottom frame edges
						rgnX = rcFrame.left;
						rgnX2 = rcFrame.left - rcFrame.right;
					}
					// top - cut always
					rgnY = rcFrame.top;
				}
				int rgnWidth = RectWidth(rcClient) + rgnX2;
				int rgnHeight = RectHeight(rcClient) + rgnY2;

				bool bCreateRgn = (nFrame >= 0) || mb_DisableThickFrame;


				if (gpSet->isQuakeStyle)
				{
					if (mn_QuakePercent)
					{
						if (mn_QuakePercent == 1)
						{
							rgnWidth = rgnHeight = 1;
						}
						else if (nFrame < 0)
						{
							nFrame = GetSystemMetrics(SM_CXSIZEFRAME);
						}
						bCreateRgn = true;
					}
					bRoundTitle = false;
				}


				if (bCreateRgn)
				{
					hRgn = CreateWindowRgn(bRoundTitle, rgnX, rgnY, rgnWidth, rgnHeight);
				}
				else
				{
					_ASSERTE(hRgn == NULL);
					hRgn = NULL;
				}
			}
		}

		if (!hRgn && hExclusion)
		{
			// Таки нужно создать...
			bool bTheming = gpSetCls->CheckTheming();
			RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);
			hRgn = CreateWindowRgn(bTheming,
			                       0,0, rcWnd.right-rcWnd.left, rcWnd.bottom-rcWnd.top);
		}
	}

	return hRgn;
}

HRGN CConEmuSize::CreateWindowRgn(bool abRoundTitle, int anX, int anY, int anWndWidth, int anWndHeight)
{
	HRGN hRgn = NULL, hExclusion = NULL, hCombine = NULL;

	TODO("DoubleView: Если видимы несколько консолей - нужно совместить регионы, или вообще отрубить, для простоты");

	CVConGuard VCon;

	// Консоли может реально не быть на старте
	// или если GUI не закрывается при закрытии последнего таба
	if (CVConGroup::GetActiveVCon(&VCon) >= 0)
	{
		hExclusion = CVConGroup::GetExclusionRgn();
	}

	if (abRoundTitle && anX>0 && anY>0)
	{
		int nPoint = 0;
		POINT ptPoly[20];
		ptPoly[nPoint++] = MakePoint(anX, anY+anWndHeight);
		ptPoly[nPoint++] = MakePoint(anX, anY+5);
		ptPoly[nPoint++] = MakePoint(anX+1, anY+3);
		ptPoly[nPoint++] = MakePoint(anX+3, anY+1);
		ptPoly[nPoint++] = MakePoint(anX+5, anY);
		ptPoly[nPoint++] = MakePoint(anX+anWndWidth-5, anY);
		ptPoly[nPoint++] = MakePoint(anX+anWndWidth-3, anY+1);
		ptPoly[nPoint++] = MakePoint(anX+anWndWidth-1, anY+4); //-V112
		ptPoly[nPoint++] = MakePoint(anX+anWndWidth, anY+6);
		ptPoly[nPoint++] = MakePoint(anX+anWndWidth, anY+anWndHeight);
		hRgn = CreatePolygonRgn(ptPoly, nPoint, WINDING);
	}
	else
	{
		hRgn = CreateRectRgn(anX, anY, anX+anWndWidth, anY+anWndHeight);
	}

	// Смотреть CombineRgn, OffsetRgn (для перемещения региона отрисовки в пространство окна)
	if (hExclusion)
	{
		if (hRgn)
		{
			//_ASSERTE(hRgn != NULL);
			//DeleteObject(hExclusion);
			//} else {
			//POINT ptShift = {0,0};
			//MapWindowPoints(ghWnd DC, NULL, &ptShift, 1);
			//RECT rcWnd = GetWindow
			RECT rcFrame = CalcMargins(CEM_FRAMECAPTION);

			POINT ptClient = {0,0};
			TODO("Будет глючить на SplitScreen?");
			MapWindowPoints(VCon->GetView(), ghWnd, &ptClient, 1);

			HRGN hOffset = CreateRectRgn(0,0,0,0);
			int n1 = CombineRgn(hOffset, hExclusion, NULL, RGN_COPY); UNREFERENCED_PARAMETER(n1);
			int n2 = OffsetRgn(hOffset, rcFrame.left+ptClient.x, rcFrame.top+ptClient.y); UNREFERENCED_PARAMETER(n2);
			hCombine = CreateRectRgn(0,0,1,1);
			CombineRgn(hCombine, hRgn, hOffset, RGN_DIFF);
			DeleteObject(hRgn);
			DeleteObject(hOffset); hOffset = NULL;
			hRgn = hCombine; hCombine = NULL;
		}
	}

	wchar_t szInfo[200];
	msprintf(szInfo, countof(szInfo), L"CreateWindowRgn: x,y={%i,%i} size={%i,%i} %s", anX, anY, anWndWidth, anWndHeight, hRgn ? L"Succeeded" : L"Failed");
	DEBUGSTRRGN(szInfo);

	return hRgn;
}

bool CConEmuSize::isCaptionHidden(ConEmuWindowMode wmNewMode /*= wmCurrent*/) const
{
	bool bCaptionHidden = gpSet->isHideCaptionAlways(); // <== Quake & UserScreen here.
	if (!bCaptionHidden)
	{
		if (wmNewMode == wmCurrent || wmNewMode == wmNotChanging)
			wmNewMode = mp_ConEmu->WindowMode;

		bCaptionHidden = (wmNewMode == wmFullScreen)
				|| ((wmNewMode == wmMaximized) && gpSet->isHideCaption);
	}
	return bCaptionHidden;
}

// Determines if our window has "invisible" portion of WS_THICKFRAME
// This was introduced in Windows 10 :(
bool CConEmuSize::isWin10InvisibleFrame(const ConEmuWindowMode wmNewMode /*= wmCurrent*/) const
{
	if (!IsWin10())
		return false;
	if (isSelfFrame(wmNewMode))
		return false;
	return true;
}

// Return true if WS_THICKFRAME is not used
bool CConEmuSize::isSelfFrame(const ConEmuWindowMode wmNewMode /*= wmCurrent*/) const
{
	if (mp_ConEmu->isInside())
		return true;
	const auto wm = (wmNewMode != wmCurrent) ? wmNewMode : GetWindowMode();
	// Use standard Windows frame when caption is ON
	if (!isCaptionHidden(wm))
		return false;
	// NO resizeable frame in FullScreen at all
	if (wm == wmFullScreen)
		return true;
	// Take into account user setting but don't allow frame larger than system-defined
	const int iFrame = gpSet->HideCaptionAlwaysFrame();
	if (iFrame >= 0)
		return true;
	// Even for standard frame (without caption) we shall hide it in maximized mode
	// to avoid artifacts and TaskBar rollout problems
	_ASSERTE(iFrame == -1);
	// gh-1539: Bypass Windows problem with region and hidden taskbar
	if (wm == wmMaximized)
	{
		const MonitorInfoCache mi = CConEmuSize::NearestMonitorInfo(NULL);
		if (mi.isTaskbarHidden)
			return true;
	}
	return false;
}

UINT CConEmuSize::GetSelfFrameWidth() const
{
	if (mp_ConEmu->isInside())
		return 0;
	_ASSERTE(isSelfFrame()); // Shall GetSelfFrameWidth be used only for self-drawn frame?
	// Take into account user setting but don't allow frame larger than system-defined
	int iFrame = gpSet->HideCaptionAlwaysFrame();
	const auto wm = GetWindowMode();
	const MonitorInfoCache mi = CConEmuSize::NearestMonitorInfo(NULL);
	// -1 means we should use standard Windows thick frame if possible
	// otherwise if window is maximized or fullscreen no sense to create hidden frame for sizing on mouse-over
	if ((iFrame < 0) || (mi.isTaskbarHidden && isCaptionHidden() && (wm == wmMaximized || wm == wmFullScreen)))
		return 0;
	// Reuse frame width from monitors cache
	int iStdFrame = mp_ConEmu->GetWinFrameWidth();
	int iDefFrame = isCaptionHidden() ? mi.noCaption.FrameWidth : mi.withCaption.FrameWidth;
	if (iFrame == 0 || iFrame > iDefFrame)
		iFrame = iDefFrame; // allow comfortable resize area on mouse hover
	return static_cast<UINT>(iFrame);
}

void CConEmuSize::SetIgnoreQuakeActivation(bool bNewValue)
{
	if (bNewValue)
		InterlockedIncrement(&mn_IgnoreQuakeActivation);
	else if (mn_IgnoreQuakeActivation > 0)
		InterlockedDecrement(&mn_IgnoreQuakeActivation);
	else
		_ASSERTE(mn_IgnoreQuakeActivation>0);
}

// размер в символах, используется в CVirtualConsole::Constructor
void CConEmuSize::EvalVConCreateSize(CVirtualConsole* apVCon, unsigned& rnTextWidth, unsigned& rnTextHeight)
{
	RECT rcConAll;
	if (WndWidth.Value && WndHeight.Value)
	{
		//if (wndWidth)  TextWidth = wndWidth;
		//if (wndHeight) TextHeight = wndHeight;

		rcConAll = CalcRect(CER_CONSOLE_ALL);
		if (rcConAll.right && rcConAll.bottom)
		{
			rnTextWidth = rcConAll.right;
			rnTextHeight = rcConAll.bottom;
		}
	}
	else
	{
		Assert(WndWidth.Value && WndHeight.Value);
	}
}

void CConEmuSize::DoBringHere()
{
	if (!IsSizePosFree())
	{
		LogString(L"CConEmuSize::DoBringHere() skipped due to !IsSizePosFree()");
		return;
	}

	LogString(L"CConEmuSize::DoBringHere()");

	if (!IsWindowVisible(ghWnd))
		Icon.RestoreWindowFromTray();

	POINT ptCur = {}; GetCursorPos(&ptCur);
	HMONITOR hMon = MonitorFromPoint(ptCur, MONITOR_DEFAULTTOPRIMARY);
	HMONITOR hWndMon = MonitorFromWindow(ghWnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi = {sizeof(mi)};
	bool bUseJump = (hWndMon != hMon);
	GetMonitorInfoSafe(hMon, mi);

	//if (!bUseJump)
	{
		setWindowPos(HWND_TOP, mi.rcWork.left, mi.rcWork.top, 0,0, SWP_NOSIZE);
	}
	//else
	//{
	//	RECT rcDefWnd = GetDefaultRect();
	//	JumpNextMonitor(ghWnd, hMon, true/*doesn't matter if hMon defined*/, rcDefWnd);
	//}
	UNREFERENCED_PARAMETER(bUseJump);
}

void CConEmuSize::DoFullScreen()
{
	if (mp_ConEmu->mp_Inside)
	{
		_ASSERTE(FALSE && "Must not change mode in the Inside mode");
		return;
	}

	ConEmuWindowMode wm = GetWindowMode();

	if (wm != wmFullScreen)
		mp_ConEmu->SetWindowMode(wmFullScreen);
	else if (mp_ConEmu->opt.DesktopMode && (wm != wmNormal))
		mp_ConEmu->SetWindowMode(wmNormal);
	else
		mp_ConEmu->SetWindowMode(mp_ConEmu->isWndNotFSMaximized ? wmMaximized : wmNormal);
}

void CConEmuSize::DoMaximizeRestore()
{
	// abPosted - removed, all calls was as TRUE

	if (mp_ConEmu->mp_Inside)
	{
		_ASSERTE(FALSE && "Must not change mode in the Inside mode");
		return;
	}

	// -- this will be done in SetWindowMode
	//StoreNormalRect(NULL); // Сама разберется, надо/не надо

	ConEmuWindowMode wm = GetWindowMode();

	mp_ConEmu->SetWindowMode((wm != wmMaximized) ? wmMaximized : wmNormal);
}

void CConEmuSize::LogMinimizeRestoreSkip(LPCWSTR asMsgFormat, DWORD nParm1, DWORD nParm2, DWORD nParm3)
{
	wchar_t szInfo[200];
	swprintf_c(szInfo, asMsgFormat, nParm1, nParm2, nParm3);
	LogFocusInfo(szInfo);
}

void CConEmuSize::DoMinimizeRestore(SingleInstanceShowHideType ShowHideType /*= sih_None*/)
{
	if (mp_ConEmu->mp_Inside)
		return;

	static bool bInFunction = false;
	if (bInFunction)
		return;

	HWND hCurForeground = GetForegroundWindow();
	bool bIsForeground = CVConGroup::isOurWindow(hCurForeground);
	bool bIsIconic = isIconic();
	BOOL bVis = IsWindowVisible(ghWnd);

	wchar_t szInfo[120];
	swprintf_c(szInfo, L"DoMinimizeRestore(%i) Fore=x%08X Our=%u Iconic=%u Vis=%u",
		ShowHideType, LODWORD(hCurForeground), bIsForeground, bIsIconic, bVis);
	LogFocusInfo(szInfo);

	if (ShowHideType == sih_QuakeShowHide)
	{
		if (bIsIconic)
		{
			if (mn_LastQuakeShowHide && gpSet->isMinimizeOnLoseFocus())
			{
				DWORD nDelay = GetTickCount() - mn_LastQuakeShowHide;
				if (nDelay < QUAKE_FOCUS_CHECK_TIMER_DELAY)
				{
					LogMinimizeRestoreSkip(L"DoMinimizeRestore skipped, delay was %u ms", nDelay);
					return;
				}
			}

			ShowHideType = sih_Show;
		}
		else
		{
			ShowHideType = sih_AutoHide; // дальше разберемся
		}
	}

	if (ShowHideType == sih_AutoHide)
	{
		if (gpSet->isQuakeStyle)
		{
			bool bMin2TSA = gpSet->isMinToTray();

			if (bMin2TSA)
				ShowHideType = sih_HideTSA;
			else
				ShowHideType = sih_Minimize;
		}
		else
		{
			ShowHideType = sih_Minimize;
		}
	}

	if ((ShowHideType == sih_HideTSA) || (ShowHideType == sih_Minimize))
	{
		// Some trick for clicks on taskbar and "Hide on focus lose"
		mp_ConEmu->mn_ForceTimerCheckLoseFocus = 0;

		if (bIsIconic || !bVis)
		{
			return;
		}
	}

	bInFunction = true;

	// 1. Функция вызывается при нажатии глобально регистрируемого хоткея
	//    Win+C  -->  ShowHideType = sih_None
	// 2. При вызове из другого приложения
	//    аргумент /single  --> ShowHideType = sih_Show
	// 3. При вызове из другого приложения
	//    аргумент /showhide     --> ShowHideType = sih_ShowMinimize
	//    аргумент /showhideTSA  --> ShowHideType = sih_ShowHideTSA
	SingleInstanceShowHideType cmd = sih_None;

	// gh#255, old#1065: Move window to "active" monitor
	if ((ShowHideType == sih_None) && gpSet->isRestore2ActiveMon && gpSet->isRestoreInactive)
	{
		POINT ptCur = {}; GetCursorPos(&ptCur);
		RECT rcCur = CalcRect(CER_MAIN, NULL);
		HMONITOR hMonCur = MonitorFromRect(&rcCur, MONITOR_DEFAULTTONEAREST);
		HMONITOR hMonAct = MonitorFromPoint(ptCur, MONITOR_DEFAULTTONEAREST);
		if (hMonCur && hMonAct && (hMonCur != hMonAct))
		{
			ShowHideType = sih_Show;
			bVis = false;
		}
	}

	// Choose default action otherwise
	if (ShowHideType == sih_None)
	{
		ShowHideType = gpSet->isMinToTray() ? sih_ShowHideTSA : sih_ShowMinimize;
	}

	// Go
	if (ShowHideType == sih_Show)
	{
		cmd = sih_Show;
	}
	else if ((ShowHideType == sih_ShowMinimize)
		|| (ShowHideType == sih_Minimize)
		|| (ShowHideType == sih_ShowHideTSA)
		|| (ShowHideType == sih_HideTSA))
	{
		if ((bVis && bIsForeground && !bIsIconic)
			|| (!bIsIconic && !gpSet->isRestoreInactive)
			|| (ShowHideType == sih_HideTSA) || (ShowHideType == sih_Minimize))
		{
			// если видимо - спрятать
			cmd = (ShowHideType == sih_HideTSA) ? sih_ShowHideTSA : ShowHideType;
		}
		else
		{
			// Иначе - показать
			cmd = sih_Show;
		}
	}
	else
	{
		_ASSERTE(ShowHideType == sih_SetForeground);
		// Иначе - показываем (в зависимости от текущей видимости)
		if (IsWindowVisible(ghWnd) && (bIsForeground || !bIsIconic)) // (!bIsIconic) - окошко развернуто, надо свернуть
			cmd = sih_SetForeground;
		else
			cmd = sih_Show;
	}

	// Поехали
	int nQuakeMin = 1;
	int nQuakeShift = 10;
	//int nQuakeDelay = gpSet->nQuakeAnimation / nQuakeShift; // 20;
	bool bUseQuakeAnimation = false; //, bNeedHideTaskIcon = false;
	bool bMinToTray = gpSet->isMinToTray();
	if (gpSet->isQuakeStyle != 0)
	{
		// Если есть дочерние GUI окна - в них могут быть глюки с отрисовкой
		if ((gpSet->nQuakeAnimation > 0) && !CVConGroup::isChildWindowVisible())
		{
			bUseQuakeAnimation = true;
		}
	}

	DEBUGTEST(static DWORD nLastQuakeHide);

	// Logging
	if (RELEASEDEBUGTEST(gpSet->isLogging(),true))
	{
		switch (cmd)
		{
		case sih_None:
			LogFocusInfo(L"DoMinimizeRestore(sih_None)"); break;
		case sih_ShowMinimize:
			LogFocusInfo(L"DoMinimizeRestore(sih_ShowMinimize)"); break;
		case sih_ShowHideTSA:
			LogFocusInfo(L"DoMinimizeRestore(sih_ShowHideTSA)"); break;
		case sih_Show:
			LogFocusInfo(L"DoMinimizeRestore(sih_Show)"); break;
		case sih_SetForeground:
			LogFocusInfo(L"DoMinimizeRestore(sih_SetForeground)"); break;
		case sih_HideTSA:
			LogFocusInfo(L"DoMinimizeRestore(sih_HideTSA)"); break;
		case sih_Minimize:
			LogFocusInfo(L"DoMinimizeRestore(sih_Minimize)"); break;
		case sih_AutoHide:
			LogFocusInfo(L"DoMinimizeRestore(sih_AutoHide)"); break;
		case sih_QuakeShowHide:
			LogFocusInfo(L"DoMinimizeRestore(sih_QuakeShowHide)"); break;
		default:
			LogFocusInfo(L"DoMinimizeRestore(Unknown cmd)");
		}
	}

	if (cmd == sih_SetForeground)
	{
		apiSetForegroundWindow(ghWnd);
	}
	else if (cmd == sih_Show)
	{
		HWND hWndFore = hCurForeground;
		DEBUGTEST(DWORD nHideShowDelay = GetTickCount() - nLastQuakeHide);
		// Если активация идет кликом по кнопке на таскбаре (Quake без скрытия) то на WinXP
		// может быть глюк - двойная отрисовка раскрытия (WM_ACTIVATE, WM_SYSCOMMAND)
		bool bNoQuakeAnimation = false;

		bool bPrevInRestore = false;
		if (bIsIconic)
			bPrevInRestore = mp_ConEmu->mp_Menu->SetRestoreFromMinimized(true);

		//apiSetForegroundWindow(ghWnd);

		if (gpSet->isFadeInactive)
		{
			CVConGuard VCon;
			if (mp_ConEmu->GetActiveVCon(&VCon) >= 0)
			{
				// Чтобы при "Fade inactive" все сразу красиво отрисовалось
				int iCur = mn_QuakePercent; mn_QuakePercent = 100;
				VCon->Update();
				mn_QuakePercent = iCur;
			}
		}

		DEBUGTEST(bool bWasQuakeIconic = isQuakeMinimized);

		ThickFrameDisabler disabler(*this);

		if (gpSet->isQuakeStyle /*&& !isMouseOverFrame()*/)
		{
			// Для Quake-style необходимо СНАЧАЛА сделать окно "невидимым" перед его разворачиваем или активацией
			if ((hWndFore == ghWnd) && !bIsIconic && bVis)
			{
				bNoQuakeAnimation = true;
				mn_QuakePercent = 0;
			}
			else
			{
				disabler.Disable();
				mn_QuakePercent = nQuakeMin;
				UpdateWindowRgn();
			}
		}
		else
			mn_QuakePercent = 0;

		if (Icon.isWindowInTray() || !IsWindowVisible(ghWnd))
		{
			mb_LockWindowRgn = true;
			bool b = mb_LockShowWindow; mb_LockShowWindow = bUseQuakeAnimation;
			Icon.RestoreWindowFromTray(false, bUseQuakeAnimation);
			mb_LockShowWindow = b;
			mb_LockWindowRgn = false;
			bIsIconic = isIconic();
			//bNeedHideTaskIcon = bUseQuakeAnimation;
		}

		// Здесь - интересует реальный IsIconic. Для isQuakeStyle может быть фейк
		if (::IsIconic(ghWnd))
		{
			MSetter lSet(&mn_IgnoreSizeChange);
			DEBUGTEST(WINDOWPLACEMENT wpl = {sizeof(wpl)}; GetWindowPlacement(ghWnd, &wpl););
			bool b = mp_ConEmu->mp_Menu->SetPassSysCommand(true);
			SendMessage(ghWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
			mp_ConEmu->mp_Menu->SetPassSysCommand(b);
		}

		_ASSERTE(mn_IgnoreSizeChange>=0);

		// Correct Quake position or Move window to "active monitor" (ref gh-255)
		if (gpSet->isQuakeStyle || gpSet->isRestore2ActiveMon)
		{
			RECT rcWnd = GetDefaultRect();

			if (gpSet->isRestore2ActiveMon)
			{
				HMONITOR hMonCur = MonitorFromRect(&rcWnd, MONITOR_DEFAULTTONEAREST);
				POINT ptCur = {}; GetCursorPos(&ptCur);
				HMONITOR hMonAct = MonitorFromPoint(ptCur, MONITOR_DEFAULTTONEAREST);
				if (hMonCur && hMonAct && (hMonCur != hMonAct))
				{
					if (JumpNextMonitor(ghWnd, hMonAct, true, rcWnd, &m_QuakePrevSize.PreSlidedSize))
					{
						RECT rcNew = GetDefaultRect();
						rcWnd = rcNew;
					}
				}
			}

			setWindowPos(NULL, rcWnd.left, rcWnd.top, 0,0, SWP_NOZORDER|SWP_NOSIZE);
		}

		isQuakeMinimized = false; // теперь можно сбросить

		apiSetForegroundWindow(ghWnd);

		OnSize();

		if (gpSet->isQuakeStyle /*&& !isMouseOverFrame()*/)
		{
			if (!bNoQuakeAnimation)
			{
				if (bUseQuakeAnimation)
				{
					DWORD nAnimationMS = gpSet->nQuakeAnimation; // (100 / nQuakeShift) * nQuakeDelay * 2;
					_ASSERTE(nAnimationMS > 0);
					//MinMax(nAnimationMS, CONEMU_ANIMATE_DURATION, CONEMU_ANIMATE_DURATION_MAX);
					MinMax(nAnimationMS, QUAKEANIMATION_MAX);

					DWORD nFlags = AW_SLIDE|AW_VER_POSITIVE|AW_ACTIVATE;

					// Need to hide window
					DEBUGTEST(RECT rcNow; ::GetWindowRect(ghWnd, &rcNow));
					RECT rcPlace = GetDefaultRect();
					if (::IsWindowVisible(ghWnd))
					{
						apiShowWindow(ghWnd, SW_HIDE);
					}
					// and place it "in place"
					int nWidth = rcPlace.right-rcPlace.left, nHeight = rcPlace.bottom-rcPlace.top;
					setWindowPos(NULL, rcPlace.left, rcPlace.top, nWidth, nHeight, SWP_NOZORDER);

					mn_QuakePercent = 100;
					UpdateWindowRgn();

					// to enable animation
					AnimateWindow(nAnimationMS, nFlags);
				}
				else
				{
					DWORD nStartTick = GetTickCount();

					disabler.Disable();

					// Begin of animation
					_ASSERTE(mn_QuakePercent==0 || mn_QuakePercent==nQuakeMin);

					if (gpSet->nQuakeAnimation > 0)
					{
						int nQuakeStepDelay = gpSet->nQuakeAnimation / nQuakeShift; // 20;

						while (mn_QuakePercent < 100)
						{
							mn_QuakePercent += nQuakeShift;
							UpdateWindowRgn();
							RedrawWindow(ghWnd, NULL, NULL, RDW_UPDATENOW);

							DWORD nCurTick = GetTickCount();
							int nQuakeDelay = nQuakeStepDelay - ((int)(nCurTick - nStartTick));
							if (nQuakeDelay > 0)
							{
								Sleep(nQuakeDelay);
							}

							nStartTick = GetTickCount();
						}
					}
				}
			}
			if (mn_QuakePercent != 100)
			{
				mn_QuakePercent = 100;
				UpdateWindowRgn();
			}
		}
		mn_QuakePercent = 0; // 0 - отключен
		m_QuakePrevSize.PreSlidedSize = {};

		disabler.Enable();

		CVConGuard VCon;
		if (mp_ConEmu->GetActiveVCon(&VCon) >= 0)
		{
			VCon->PostRestoreChildFocus();
			//mp_ConEmu->OnFocus(ghWnd, WM_ACTIVATEAPP, TRUE, 0, L"From DoMinimizeRestore(sih_Show)");
		}

		CVConGroup::EnumVCon(evf_Visible, CRealConsole::RefreshAfterRestore, 0);

		if (bIsIconic)
			mp_ConEmu->mp_Menu->SetRestoreFromMinimized(bPrevInRestore);
	} // if (cmd == sih_Show)
	else
	{
		// Минимизация
		_ASSERTE(((cmd == sih_ShowHideTSA) || (cmd == sih_ShowMinimize) || (cmd == sih_Minimize)) && "cmd must be determined!");

		isQuakeMinimized = true; // сразу включим, чтобы не забыть. Используется только при gpSet->isQuakeStyle

		if (bVis && !bIsIconic)
		{
			//UpdateIdealRect();
			StoreNormalRect(NULL);
		}

		if ((ghLastForegroundWindow != ghWnd) && (ghLastForegroundWindow != ghOpWnd))
		{
			// Фокус не там где надо - например, в дочернем GUI приложении
			if (CVConGroup::isOurWindow(ghLastForegroundWindow))
			{
				mp_ConEmu->setFocus();
				apiSetForegroundWindow(ghWnd);
			}
			//TODO: Тут наверное нужно выйти и позвать Post для DoMinimizeRestore(cmd)
			//TODO: иначе при сворачивании не активируется "следующее" окно, фокус ввода
			//TODO: остается в дочернем Notepad (ввод текста идет в него)
		}

		bool wasMinimized = false;
		ThickFrameDisabler disabler(*this);

		if (gpSet->isQuakeStyle /*&& !isMouseOverFrame()*/)
		{
			BOOL bVs1 = ::IsWindowVisible(ghWnd);
			RECT rc1 = {}; ::GetWindowRect(ghWnd, &rc1);
			if (bVs1)
			{
				m_QuakePrevSize.PreSlidedSize = rc1;
			}
			else
			{
				_ASSERTE(FALSE && "Quake must not be slided up from hidden state");
				m_QuakePrevSize.PreSlidedSize = {};
			}

			if (bUseQuakeAnimation)
			{
				DWORD nAnimationMS = gpSet->nQuakeAnimation; // (100 / nQuakeShift) * nQuakeDelay * 2;
				_ASSERTE(nAnimationMS > 0);
				//MinMax(nAnimationMS, CONEMU_ANIMATE_DURATION, CONEMU_ANIMATE_DURATION_MAX);
				MinMax(nAnimationMS, QUAKEANIMATION_MAX);

				DWORD nFlags = AW_SLIDE|AW_VER_NEGATIVE|AW_HIDE;

				DEBUGTEST(HWND hFore1 = GetForegroundWindow());

				AnimateWindow(nAnimationMS, nFlags);

				DEBUGTEST(BOOL bVs2 = ::IsWindowVisible(ghWnd));
				DEBUGTEST(RECT rc2; ::GetWindowRect(ghWnd, &rc2));
				DEBUGTEST((bVs1 == bVs2));  // -V607
				DEBUGTEST(HWND hFore2 = GetForegroundWindow());

				// 1. Если на таскбаре отображаются "табы", то после AnimateWindow(AW_HIDE) в Win8 иконка с таскбара не убирается
				// 2. Issue 1042: Return focus to window which was active before showing ConEmu
				StopForceShowFrame();
				mn_QuakePercent = 1;
				UpdateWindowRgn();

				if (!bMinToTray && (cmd != sih_ShowHideTSA))
				{
					// Если в трей не скрываем - то окошко нужно "вернуть на таскбар"
					apiShowWindow(ghWnd, SW_SHOWMINNOACTIVE);
				}
				else
				{
					apiShowWindow(ghWnd, SW_SHOWNOACTIVATE);
					apiShowWindow(ghWnd, SW_HIDE);
				}
				wasMinimized = true;
			}
			else
			{
				DWORD nStartTick = GetTickCount();
				mn_QuakePercent = (gpSet->nQuakeAnimation > 0) ? (100 + nQuakeMin - nQuakeShift) : nQuakeMin;
				StopForceShowFrame();

				if (gpSet->nQuakeAnimation > 0)
				{
					int nQuakeStepDelay = gpSet->nQuakeAnimation / nQuakeShift; // 20;

					disabler.Disable();

					while (mn_QuakePercent > 0)
					{
						UpdateWindowRgn();
						RedrawWindow(ghWnd, NULL, NULL, RDW_UPDATENOW);
						//Sleep(nQuakeDelay);
						mn_QuakePercent -= nQuakeShift;

						DWORD nCurTick = GetTickCount();
						int nQuakeDelay = nQuakeStepDelay - ((int)(nCurTick - nStartTick));
						if (nQuakeDelay > 0)
						{
							Sleep(nQuakeDelay);
						}

						nStartTick = GetTickCount();
					}
				}
				else
				{
					UpdateWindowRgn();
				}
			}
		}
		else
		{
			m_QuakePrevSize.PreSlidedSize = {};
		}
		mn_QuakePercent = 0; // 0 - отключен


		if (cmd == sih_ShowHideTSA)
		{
			mb_LockWindowRgn = true;
			// Явно попросили в TSA спрятать
			Icon.HideWindowToTray();
			mb_LockWindowRgn = false;
		}
		else if ((cmd == sih_ShowMinimize) || (ShowHideType == sih_Minimize))
		{
			if (!wasMinimized)
			{
				// SC_MINIMIZE сам обработает (gpSet->isMinToTray || mp_ConEmu->ForceMinimizeToTray)
				SendMessage(ghWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
			}
		}

		disabler.Enable();

		DEBUGTEST(nLastQuakeHide = GetTickCount());
	}

	mn_LastQuakeShowHide = GetTickCount();
	LogFocusInfo(L"DoMinimizeRestore finished");

	bInFunction = false;
 }

void CConEmuSize::DoForcedFullScreen(bool bSet /*= true*/)
{
	static bool bWasSetTopMost = false;

	// определить возможность перейти в текстовый FullScreen
	if (!bSet)
	{
		// Снять флаг "OnTop", вернуть нормальные приоритеты процессам
		if (bWasSetTopMost)
		{
			gpSet->isAlwaysOnTop = false;
			DoAlwaysOnTopSwitch();
			bWasSetTopMost = false;
		}
		return;
	}

	if (IsHwFullScreenAvailable())
	{

		CVConGuard VCon;
		if (CVConGroup::GetActiveVCon(&VCon) >= 0)
		{
			//BOOL WINAPI SetConsoleDisplayMode(HANDLE hConsoleOutput, DWORD dwFlags, PCOORD lpNewScreenBufferDimensions);
			//if (!isIconic())
			//	SendMessage(ghWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);

			::ShowWindow(ghWnd, SW_SHOWMINNOACTIVE);

			bool bFRc = VCon->RCon()->SetFullScreen();

			if (bFRc)
				return;

			SendMessage(ghWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
		}
	}

	if (mp_ConEmu->opt.DesktopMode)
	{
		DisplayLastError(L"Can't set FullScreen in DesktopMode", -1);
		return;
	}

	TODO("Поднять приоритет процессов");

	// Установить AlwaysOnTop
	if (!gpSet->isAlwaysOnTop)
	{
		gpSet->isAlwaysOnTop = true;
		DoAlwaysOnTopSwitch();
		bWasSetTopMost = true;
	}

	if (!mp_ConEmu->isMeForeground())
	{
		if (isIconic())
		{
			if (Icon.isWindowInTray() || !IsWindowVisible(ghWnd))
				Icon.RestoreWindowFromTray();
			else
				SendMessage(ghWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
		}
		//else
		//{
		//	//SendMessage(ghWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
		//	SendMessage(ghWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
		//}
	}

	SetWindowMode(wmFullScreen);

	if (!mp_ConEmu->isMeForeground())
	{
		SwitchToThisWindow(ghWnd, FALSE);
	}
}

void CConEmuSize::DoAlwaysOnTopSwitch()
{
	HWND hwndAfter = (gpSet->isAlwaysOnTop || mp_ConEmu->opt.DesktopMode) ? HWND_TOPMOST : HWND_NOTOPMOST;

	#ifdef CATCH_TOPMOST_SET
	_ASSERTE((hwndAfter!=HWND_TOPMOST) && "Setting TopMost mode - CConEmuMain::OnAlwaysOnTop()");
	#endif

	CheckMenuItem(mp_ConEmu->mp_Menu->GetSysMenu(), ID_ALWAYSONTOP, MF_BYCOMMAND |
	              (gpSet->isAlwaysOnTop ? MF_CHECKED : MF_UNCHECKED));
	setWindowPos(hwndAfter, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);

	if (ghOpWnd && gpSet->isAlwaysOnTop)
	{
		SetWindowPos(ghOpWnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
		apiSetForegroundWindow(ghOpWnd);
	}
}

void CConEmuSize::DoDesktopModeSwitch()
{
	if (!this) return;

	Icon.SettingsChanged();

	if (mp_ConEmu->WindowStartMinimized || mp_ConEmu->ForceMinimizeToTray)
		return;

#ifndef CHILD_DESK_MODE
	DWORD dwStyleEx = GetWindowLong(ghWnd, GWL_EXSTYLE);
	DWORD dwNewStyleEx = dwStyleEx;
	DWORD dwStyle = GetWindowLong(ghWnd, GWL_STYLE);
	DWORD dwNewStyle = dwStyle;

	if (mp_ConEmu->opt.DesktopMode)
	{
		dwNewStyleEx |= WS_EX_TOOLWINDOW;
		dwNewStyle |= WS_POPUP;
	}
	else
	{
		dwNewStyleEx &= ~WS_EX_TOOLWINDOW;
		dwNewStyle &= ~WS_POPUP;
	}

	if (dwNewStyleEx != dwStyleEx || dwNewStyle != dwStyle)
	{
		mp_ConEmu->SetWindowStyle(dwStyle);
		mp_ConEmu->SetWindowStyleEx(dwStyleEx);
		CVConGroup::SyncWindowToConsole(); // -- функция пустая, игнорируется
		UpdateWindowRgn();
	}

#endif
#ifdef CHILD_DESK_MODE
	HWND hDesktop = GetDesktopWindow();

	//HWND hProgman = FindWindowEx(hDesktop, NULL, L"Progman", L"Program Manager");
	//HWND hParent = NULL; // mp_ConEmu->opt.DesktopMode ?  : GetDesktopWindow();

	OnTaskbarSettingsChanged();

	if (mp_ConEmu->opt.DesktopMode)
	{
		// Shell windows is FindWindowEx(hDesktop, NULL, L"Progman", L"Program Manager");
		HWND hShellWnd = GetShellWindow();
		DWORD dwShellPID = 0;

		if (hShellWnd)
			GetWindowThreadProcessId(hShellWnd, &dwShellPID);

		// But in Win7 it is not a real desktop holder :(
		if (gOSVer.dwMajorVersion >= 6)  // Vista too?
		{
			// В каких-то случаях (на каких-то темах?) иконки создаются в "Progman", а в одном из "WorkerW" классов
			// Все эти окна принадлежат одному процессу explorer.exe
			HWND hShell = FindWindowEx(hDesktop, NULL, L"WorkerW", NULL);

			while (hShell)
			{
				// У него должны быть дочерние окна
				if (IsWindowVisible(hShell) && FindWindowEx(hShell, NULL, NULL, NULL))
				{
					// Теоретически, эти окна должны принадлежать одному процессу (Explorer.exe)
					if (dwShellPID)
					{
						DWORD dwTestPID;
						GetWindowThreadProcessId(hShell, &dwTestPID);

						if (dwTestPID != dwShellPID)
						{
							hShell = FindWindowEx(hDesktop, hShell, L"WorkerW", NULL);
							continue;
						}
					}

					break;
				}

				hShell = FindWindowEx(hDesktop, hShell, L"WorkerW", NULL);
			}

			if (hShell)
				hShellWnd = hShell;
		}

		if (gpSet->isQuakeStyle)  // этот режим с Desktop несовместим
		{
			SetQuakeMode(0, (WindowMode == wmFullScreen) ? wmMaximized : wmNormal);
		}

		if (WindowMode == wmFullScreen)  // этот режим с Desktop несовместим
		{
			SetWindowMode(wmMaximized);
		}

		if (!hShellWnd)
		{
			mp_ConEmu->opt.DesktopMode.Clear();

			HWND hExt = gpSetCls->GetPage(gpSetCls->thi_Features);

			if (ghOpWnd && hExt)
			{
				CheckDlgButton(hExt, cbDesktopMode, BST_UNCHECKED);
			}
		}
		else
		{
			mh_ShellWindow = hShellWnd;
			GetWindowThreadProcessId(mh_ShellWindow, &mn_ShellWindowPID);
			RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);
			MapWindowPoints(NULL, mh_ShellWindow, (LPPOINT)&rcWnd, 2);
			SetParent(mh_ShellWindow);

			setWindowPos(NULL, rcWnd.left,rcWnd.top,0,0, SWP_NOSIZE|SWP_NOZORDER);
			setWindowPos(HWND_TOPMOST, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE);

			#ifdef _DEBUG
			RECT rcNow; GetWindowRect(ghWnd, &rcNow);
			UNREFERENCED_PARAMETER(rcNow.left);
			#endif
		}
	}

	if (!mp_ConEmu->opt.DesktopMode)
	{
		//dwStyle |= WS_POPUP;
		RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);
		RECT rcVirtual = GetVirtualScreenRect(TRUE);
		setWindowPos(NULL, std::max(rcWnd.left,rcVirtual.left),std::max(rcWnd.top,rcVirtual.top),0,0, SWP_NOSIZE|SWP_NOZORDER);
		SetParent(hDesktop);
		setWindowPos(NULL, std::max(rcWnd.left,rcVirtual.left),std::max(rcWnd.top,rcVirtual.top),0,0, SWP_NOSIZE|SWP_NOZORDER);
		OnAlwaysOnTop();

		if (ghOpWnd && !gpSet->isAlwaysOnTop)
			apiSetForegroundWindow(ghOpWnd);
	}

	//SetWindowStyle(dwStyle);
#endif
}

HWND CConEmuSize::FindNextSiblingApp(bool bActivate)
{
	HWND hNext = NULL;
	wchar_t szClass[MAX_PATH] = L"";

	#ifdef _DEBUG
	DWORD nStyle;
	DWORD nPID = 0;
	wchar_t szTitle[MAX_PATH] = L"";
	#endif

	while ((hNext = FindWindowEx(NULL, hNext, NULL, NULL)) != NULL)
	{
		if (!IsWindowVisible(hNext) || !IsWindowEnabled(hNext))
			continue;
		if (CVConGroup::isOurWindow(hNext))
			continue;
		if (!GetClassName(hNext, szClass, countof(szClass)))
			continue;
		if (CDefTermBase::IsShellTrayWindowClass(szClass))
			continue;
		if (lstrcmpi(szClass, L"Button") == 0)
			continue; // Windows' Start button

		#ifdef _DEBUG
		nStyle = GetWindowLong(hNext, GWL_STYLE);
		GetWindowThreadProcessId(hNext, &nPID);
		GetWindowText(hNext, szTitle, countof(szTitle));
		#endif

		// Found
		if (bActivate)
			apiSetForegroundWindow(hNext);
		break;
	}

	return hNext;
}
