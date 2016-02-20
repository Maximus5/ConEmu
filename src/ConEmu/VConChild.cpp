
/*
Copyright (c) 2009-2015 Maximus5
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
#include "Header.h"
#include "../common/Common.h"
#include "../common/MMap.h"
#include "../common/SetEnvVar.h"
#include "ConEmu.h"
#include "VConChild.h"
#include "Options.h"
#include "OptionsClass.h"
#include "Status.h"
#include "TabBar.h"
#include "VirtualConsole.h"
#include "RealConsole.h"
#include "VConGroup.h"
#include "GestureEngine.h"

#if defined(__GNUC__)
#define EXT_GNUC_LOG
#endif

#define DEBUGSTRDRAW(s) //DEBUGSTR(s)
#define DEBUGSTRTABS(s) //DEBUGSTR(s)
#define DEBUGSTRLANG(s) //DEBUGSTR(s)
#define DEBUGSTRCONS(s) //DEBUGSTR(s)
#define DEBUGSTRPAINTGAPS(s) //DEBUGSTR(s)
#define DEBUGSTRPAINTVCON(s) //DEBUGSTR(s)
#define DEBUGSTRSIZE(s) //DEBUGSTR(s)
#define DEBUGSTRDESTROY(s) DEBUGSTR(s)
#define DEBUGSTRTEXTSEL(s) DEBUGSTR(s)

//#define SCROLLHIDE_TIMER_ID 1726
#define TIMER_SCROLL_SHOW         3201
#define TIMER_SCROLL_SHOW_DELAY   gpSet->nScrollBarAppearDelay /*1000*/
#define TIMER_SCROLL_SHOW_DELAY2  (gpSet->nScrollBarAppearDelay/2) /*500*/
#define TIMER_SCROLL_HIDE         3202
#define TIMER_SCROLL_HIDE_DELAY   gpSet->nScrollBarDisappearDelay /*1000*/
#define TIMER_SCROLL_CHECK        3203
#define TIMER_SCROLL_CHECK_DELAY  250
//#define TIMER_SCROLL_CHECK_DELAY2 1000
#define LOCK_DC_RECT_TIMEOUT      2000
#define TIMER_AUTOCOPY_DELAY      (GetDoubleClickTime()*10/9)
#define TIMER_AUTOCOPY            3204

extern MMap<HWND,CVirtualConsole*> gVConDcMap;
extern MMap<HWND,CVirtualConsole*> gVConBkMap;
static HWND ghDcInDestroing = NULL;
static HWND ghBkInDestroing = NULL;

static UINT gn_MsgVConTerminated = 0; // gpConEmu->GetRegisteredMessage("VConTerminated");

CConEmuChild::CConEmuChild(CVirtualConsole* pOwner)
{
	mp_VCon = pOwner;

	mn_AlreadyDestroyed = 0;

	mn_MsgVConTerminated = 0; // Set when destroying pended
	if (!gn_MsgVConTerminated)
		gn_MsgVConTerminated = gpConEmu->GetRegisteredMessage("VConTerminated");
	mn_MsgTabChanged = gpConEmu->GetRegisteredMessage("CONEMUTABCHANGED",CONEMUTABCHANGED);
	mn_MsgPostFullPaint = gpConEmu->GetRegisteredMessage("CConEmuChild::PostFullPaint");
	mn_MsgSavePaneSnapshot = gpConEmu->GetRegisteredMessage("CConEmuChild::SavePaneSnapshot");
	mn_MsgDetachPosted = gpConEmu->GetRegisteredMessage("CConEmuChild::Detach");
	mn_MsgRestoreChildFocus = gpConEmu->GetRegisteredMessage("CONEMUMSG_RESTORECHILDFOCUS",CONEMUMSG_RESTORECHILDFOCUS);
	#ifdef _DEBUG
	mn_MsgCreateDbgDlg = gpConEmu->GetRegisteredMessage("CConEmuChild::MsgCreateDbgDlg");
	hDlgTest = NULL;
	#endif

	mb_RestoreChildFocusPending = false;

	mb_PostFullPaint = FALSE;
	mn_LastPostRedrawTick = 0;
	mb_IsPendingRedraw = FALSE;
	mb_RedrawPosted = FALSE;
	ZeroStruct(Caret);
	mb_DisableRedraw = FALSE;
	mn_WndDCStyle = mn_WndDCExStyle = 0;
	mh_WndDC = NULL;
	mh_WndBack = NULL;
	mn_InvalidateViewPending = 0; mn_WmPaintCounter = 0;
	mh_LastGuiChild = NULL;
	mb_ScrollVisible = FALSE; mb_Scroll2Visible = FALSE; /*mb_ScrollTimerSet = FALSE;*/ mb_ScrollAutoPopup = FALSE;
	mb_VTracking = FALSE;

	ZeroStruct(m_si);
	m_si.cbSize = sizeof(m_si);
	m_si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE /*| SIF_DISABLENOSCROLL*/;
	mb_ScrollDisabled = FALSE;
	m_LastAlwaysShowScrollbar = gpSet->isAlwaysShowScrollbar;
	mb_ScrollRgnWasSet = false;

	ZeroStruct(m_LockDc);
}

CConEmuChild::~CConEmuChild()
{
	if (mh_WndDC || mh_WndBack)
	{
		DoDestroyDcWindow();
	}
}

bool CConEmuChild::isAlreadyDestroyed()
{
	return (mn_AlreadyDestroyed!=0);
}

void CConEmuChild::DoDestroyDcWindow()
{
	// Set flag immediately
	mn_AlreadyDestroyed = GetTickCount();

	// Go
	ghDcInDestroing = mh_WndDC;
	ghBkInDestroing = mh_WndBack;
	// Remove from MMap before DestroyWindow, because pVCon is no longer Valid
	if (mh_WndDC)
	{
		gVConDcMap.Del(mh_WndDC);
		DestroyWindow(mh_WndDC);
		mh_WndDC = NULL;
	}
	if (mh_WndBack)
	{
		gVConBkMap.Del(mh_WndBack);
		DestroyWindow(mh_WndBack);
		mh_WndBack = NULL;
	}
	ghDcInDestroing = NULL;
	ghBkInDestroing = NULL;
}

void CConEmuChild::PostOnVConClosed()
{
	// Must be called FOR VALID objects ONLY (guared from outside)!
	CVirtualConsole* pVCon = mp_VCon;
	if (CVConGroup::isValid(pVCon)
		&& !InterlockedExchange(&this->mn_MsgVConTerminated, gn_MsgVConTerminated))
	{
		ShutdownGuiStep(L"ProcessVConClosed - repost");
		PostMessage(this->mh_WndDC, gn_MsgVConTerminated, 0, (LPARAM)pVCon);
	}

#ifdef _DEBUG
	// Must be guarded and thats why valid...
	int iRef = pVCon->RefCount();
	_ASSERTE(CVConGroup::isValid(pVCon) || (iRef>=1 && iRef<10));
#endif
}

void CConEmuChild::ProcessVConClosed(CVirtualConsole* apVCon, BOOL abPosted /*= FALSE*/)
{
	_ASSERTE(apVCon);

	if (!CVConGroup::isValid(apVCon))
		return;

	if (!abPosted)
	{
		apVCon->PostOnVConClosed();
		//PostMessage(ghWnd, mn_MsgVConTerminated, 0, (LPARAM)apVCon);
		return;
	}

	CVConGroup::OnVConClosed(apVCon);

	// Передернуть главный таймер, а то GUI долго думает, если ни одной консоли уже не осталось
	if (!CVConGroup::isVConExists(0))
		gpConEmu->OnTimer(TIMER_MAIN_ID, 0);

	ShutdownGuiStep(L"ProcessVConClosed - done");
}

HWND CConEmuChild::CreateView()
{
	if (!this)
	{
		_ASSERTE(this!=NULL);
		return NULL;
	}
	if (mh_WndDC || mh_WndBack)
	{
		_ASSERTE(mh_WndDC == NULL && mh_WndBack == NULL);
		return mh_WndDC;
	}

	if (!isMainThread())
	{
		// Окно должно создаваться в главной нити!
		HWND hCreate = gpConEmu->PostCreateView(this); UNREFERENCED_PARAMETER(hCreate);
		_ASSERTE(hCreate && (hCreate == mh_WndDC));
		return mh_WndDC;
	}

	CVirtualConsole* pVCon = mp_VCon;
	_ASSERTE(pVCon!=NULL);
	//-- тут консоль только создается, guard не нужен
	//CVConGuard guard(pVCon);

	TODO("Заменить ghWnd на ghWndWork");
	HWND hParent = ghWnd;
	// Имя класса - то же самое, что и у главного окна
	DWORD style = /*WS_VISIBLE |*/ WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	DWORD styleEx = 0;

	RECT rcBack = gpConEmu->CalcRect(CER_BACK, pVCon);
	RECT rc = gpConEmu->CalcRect(CER_DC, rcBack, CER_BACK, pVCon);

	mh_WndBack = CreateWindowEx(styleEx, gsClassNameBack, L"BackWND", style,
		rcBack.left, rcBack.top, rcBack.right - rcBack.left, rcBack.bottom - rcBack.top, hParent, NULL, (HINSTANCE)g_hInstance, pVCon);

	mn_WndDCStyle = style;
	mn_WndDCExStyle = styleEx;
	mh_WndDC = CreateWindowEx(styleEx, gsClassName, L"DrawWND", style,
		rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, hParent, NULL, (HINSTANCE)g_hInstance, pVCon);

	if (!mh_WndDC || !mh_WndBack)
	{
		WarnCreateWindowFail(L"DC window", hParent, GetLastError());
		return NULL; //
	}

	SetWindowPos(mh_WndDC, HWND_TOP, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);

	// Установить переменную среды с дескриптором окна
	SetConEmuEnvVar(mh_WndDC);

	return mh_WndDC;
}

HWND CConEmuChild::GetView()
{
	if (!this)
	{
		_ASSERTE(this!=NULL);
		return NULL;
	}
	return mh_WndDC;
}

HWND CConEmuChild::GetBack()
{
	if (!this)
	{
		_ASSERTE(this!=NULL);
		return NULL;
	}
	return mh_WndBack;
}

BOOL CConEmuChild::ShowView(int nShowCmd)
{
	if (!this || !mh_WndDC)
		return FALSE;

	BOOL bRc = FALSE;
	DWORD nTID = 0, nPID = 0;
	wchar_t sInfo[200];

	// Должно быть создано в главной нити!
	nTID = GetWindowThreadProcessId(mh_WndDC, &nPID);

	#ifdef _DEBUG
	DWORD nMainThreadID = GetWindowThreadProcessId(ghWnd, &nPID);
	_ASSERTE(nTID==nMainThreadID);
	#endif

	// Если это "GUI" режим - могут возникать блокировки из-за дочернего окна
	CVirtualConsole* pVCon = mp_VCon;
	_ASSERTE(pVCon!=NULL);
	CVConGuard guard(pVCon);

	HWND hChildGUI = pVCon->GuiWnd();
	BOOL bGuiVisible = (hChildGUI && nShowCmd) ? pVCon->RCon()->isGuiVisible() : FALSE;
	DWORD nDcShowCmd = nShowCmd;


	if (gpSetCls->isAdvLogging)
	{
		if (hChildGUI != NULL)
			_wsprintf(sInfo, SKIPLEN(countof(sInfo)) L"ShowView: Back=x%08X, DC=x%08X, ChildGUI=x%08X, ShowCMD=%u, ChildVisible=%u",
				LODWORD(mh_WndBack), LODWORD(mh_WndDC), LODWORD(hChildGUI), nShowCmd, bGuiVisible);
		else
			_wsprintf(sInfo, SKIPLEN(countof(sInfo)) L"ShowView: Back=x%08X, DC=x%08X, ShowCMD=%u",
				LODWORD(mh_WndBack), LODWORD(mh_WndDC), nShowCmd);
		gpConEmu->LogString(sInfo);
	}


	if (hChildGUI || (GetCurrentThreadId() != nTID))
	{
		// Только Async, иначе можно получить dead-lock
		bRc = ShowWindowAsync(mh_WndBack, nShowCmd);
		if (bGuiVisible && !mp_VCon->RCon()->isGuiForceConView())
			nDcShowCmd = SW_HIDE;
		bRc = ShowWindowAsync(mh_WndDC, nDcShowCmd);
	}
	else
	{
		bRc = ShowWindow(mh_WndBack, nShowCmd);
		bRc = ShowWindow(mh_WndDC, nShowCmd);
		if (nShowCmd)
		{
			SetWindowPos(mh_WndDC, HWND_TOP, 0, 0, 0,0, SWP_NOSIZE|SWP_NOMOVE);
			SetWindowPos(mh_WndBack, mh_WndDC, 0, 0, 0,0, SWP_NOSIZE|SWP_NOMOVE);
		}
	}

	if (nShowCmd && bGuiVisible)
	{
		// Если активируется таб с ChildGui
		if (pVCon->isActive(false))
		{
			PostRestoreChildFocus();
		}
	}

	return bRc;
}

LRESULT CConEmuChild::ChildWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	// Logger
	MSG msgStr = {hWnd, messg, wParam, lParam};
	ConEmuMsgLogger::Log(msgStr, ConEmuMsgLogger::msgCanvas);

	if (gpSetCls->isAdvLogging >= 4)
	{
		gpConEmu->LogMessage(hWnd, messg, wParam, lParam);
	}

	CVConGuard guard;
	CVirtualConsole* pVCon = NULL;
	if (messg == WM_CREATE || messg == WM_NCCREATE)
	{
		LPCREATESTRUCT lp = (LPCREATESTRUCT)lParam;
		guard = (CVirtualConsole*)lp->lpCreateParams;
		pVCon = guard.VCon();
		if (pVCon)
		{
			gVConDcMap.Set(hWnd, pVCon);

			pVCon->m_TAutoCopy.Init(hWnd, TIMER_AUTOCOPY, TIMER_AUTOCOPY_DELAY);

			pVCon->m_TScrollShow.Init(hWnd, TIMER_SCROLL_SHOW, TIMER_SCROLL_SHOW_DELAY);
			pVCon->m_TScrollHide.Init(hWnd, TIMER_SCROLL_HIDE, TIMER_SCROLL_HIDE_DELAY);
			#ifndef SKIP_HIDE_TIMER
			pVCon->m_TScrollCheck.Init(hWnd, TIMER_SCROLL_CHECK, TIMER_SCROLL_CHECK_DELAY);
			#endif
		}
	}
	else if (hWnd != ghDcInDestroing)
	{
		if (!gVConDcMap.Get(hWnd, &pVCon) || !guard.Attach(pVCon))
			pVCon = NULL;
	}


	if (messg == WM_SYSCHAR)
	{
		_ASSERTE(FALSE); // по идее, фокуса тут быть не должно
		// Чтобы не пищало
		result = TRUE;
		goto wrap;
	}

	if (!pVCon)
	{
		_ASSERTE(pVCon!=NULL || hWnd==ghDcInDestroing);
		result = DefWindowProc(hWnd, messg, wParam, lParam);
		goto wrap;
	}

	switch (messg)
	{
		case WM_DESTROY:
			DEBUGSTRDESTROY(L"WM_DESTROY: VCon");
			result = DefWindowProc(hWnd, messg, wParam, lParam);
			break;
		case WM_SHOWWINDOW:
			{
				// Из-за многопоточности может случиться так,
				// что m_ChildGui.hGuiWnd инициализируется после
				// того, как был вызван ShowView, но до того
				// как было получено WM_SHOWWINDOW
				HWND hGui = pVCon->GuiWnd();
				if (hGui)
				{
					//_ASSERTE(((wParam==0) || pVCon->RCon()->isGuiForceConView()) && "Show DC while GuiWnd exists");
					if (wParam && !pVCon->RCon()->isGuiForceConView())
					{
						wParam = SW_HIDE;
					}
				}
				result = DefWindowProc(hWnd, messg, wParam, lParam);
				break;
			}
		case WM_SETFOCUS:
			// Если в консоли работает "GUI" окно (GUI режим), то фокус нужно отдать туда.
			{
				// Фокус должен быть в главном окне! За исключением случая работы в GUI режиме.
				pVCon->setFocus();
			}
			return 0;
		case WM_ERASEBKGND:
			result = 0;
			break;
		case WM_PAINT:
			pVCon->mn_WmPaintCounter++;
			if (gpSetCls->isAdvLogging >= 2)
			{
				wchar_t szInfo[80];
				_wsprintf(szInfo, SKIPCOUNT(szInfo) L"VCon[%i] WM_PAINT %u times, %u pending", pVCon->Index(), pVCon->mn_WmPaintCounter, pVCon->mn_InvalidateViewPending);
				LogString(szInfo);
			}
			pVCon->mn_InvalidateViewPending = 0;
			#ifdef _DEBUG
			{
				wchar_t szPos[80]; RECT rcScreen = {}; GetWindowRect(hWnd, &rcScreen);
				_wsprintf(szPos, SKIPCOUNT(szPos) L"PaintClient VCon[%i] at {%i,%i}-{%i,%i} screen coords", pVCon->Index(), LOGRECTCOORDS(rcScreen));
				DEBUGSTRPAINTVCON(szPos);
			}
			#endif
			result = pVCon->OnPaint();
			break;
		case WM_PRINTCLIENT:
			if (wParam && (lParam & PRF_CLIENT))
			{
				#ifdef _DEBUG
				{
					wchar_t szPos[80]; RECT rcScreen = {}; GetWindowRect(hWnd, &rcScreen);
					_wsprintf(szPos, SKIPCOUNT(szPos) L"PrintClient VCon[%i] at {%i,%i}-{%i,%i} screen coords", pVCon->Index(), LOGRECTCOORDS(rcScreen));
					DEBUGSTRPAINTVCON(szPos);
				}
				#endif
				pVCon->PrintClient((HDC)wParam, false, NULL);
			}
			break;
		case WM_SIZE:
			#ifdef _DEBUG
			{
				RECT rc; GetClientRect(hWnd, &rc);
				short cx = LOWORD(lParam);
				rc.left = rc.left;
			}
			#endif
			result = pVCon->OnSize(wParam, lParam);
			break;
		case WM_MOVE:
			result = pVCon->OnMove(wParam, lParam);
			break;
		case WM_CREATE:
			break;
		case WM_MOUSEWHEEL:
		case WM_MOUSEHWHEEL:
		case WM_MOUSEMOVE:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
		case WM_XBUTTONDBLCLK:
			if (pVCon->RCon()->isGuiOverCon())
				break;
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_ACTIVATE:
		case WM_ACTIVATEAPP:
			//case WM_MOUSEACTIVATE:
		case WM_KILLFOCUS:
			//case WM_SETFOCUS:
		case WM_VSCROLL:
			// Вся обработка в родителе
			{
				switch (messg)
				{
					case WM_VSCROLL:
						switch (LOWORD(wParam))
						{
						case SB_THUMBTRACK:
						case SB_THUMBPOSITION:
							pVCon->mb_VTracking = TRUE;
							break;
						case SB_ENDSCROLL:
							pVCon->mb_VTracking = FALSE;
							break;
						}
						pVCon->RCon()->DoScroll(LOWORD(wParam), HIWORD(wParam));
						break;

					case WM_LBUTTONUP:
						pVCon->mb_VTracking = FALSE;
						break;
				}

				TODO("Обработка ghWndWork");
				HWND hParent = ghWnd;
				static bool bInFixStyle = false;
				if (!bInFixStyle)
				{
					hParent = GetParent(hWnd);
					if (hParent != ghWnd)
					{
						// Неправомерные действия плагинов фара?
						bInFixStyle = true;
						_ASSERTE(GetParent(hWnd)==ghWnd);
						SetParent(hWnd, ghWnd);
						bInFixStyle = false;
						hParent = ghWnd;
					}

					DWORD curStyle = GetWindowLong(hWnd, GWL_STYLE);

					if ((curStyle & CRITICAL_DCWND_STYLES) != (pVCon->mn_WndDCStyle & CRITICAL_DCWND_STYLES))
					{
						// DC window styles was changed externally!
						bInFixStyle = true;
						_ASSERTEX(((curStyle & CRITICAL_DCWND_STYLES) != (pVCon->mn_WndDCStyle & CRITICAL_DCWND_STYLES)));
						SetWindowLongPtr(hWnd, GWL_STYLE, (LONG_PTR)(DWORD_PTR)pVCon->mn_WndDCStyle);
						bInFixStyle = false;
					}
				}

				if (messg >= WM_MOUSEFIRST && messg <= WM_MOUSELAST)
				{
					POINT pt = {LOWORD(lParam),HIWORD(lParam)};
					MapWindowPoints(hWnd, hParent, &pt, 1);
					lParam = MAKELONG(pt.x,pt.y);
				}

				result = gpConEmu->WndProc(hParent, messg, wParam, lParam);
			}
			break;
		case WM_IME_NOTIFY:
			break;
		case WM_INPUTLANGCHANGE:
		case WM_INPUTLANGCHANGEREQUEST:
			{
				#ifdef _DEBUG
				if (IsDebuggerPresent())
				{
					WCHAR szMsg[128];
					_wsprintf(szMsg, SKIPLEN(countof(szMsg)) L"InChild %s(CP:%i, HKL:0x%08X)\n",
							  (messg == WM_INPUTLANGCHANGE) ? L"WM_INPUTLANGCHANGE" : L"WM_INPUTLANGCHANGEREQUEST",
							  (DWORD)wParam, (DWORD)lParam);
					DEBUGSTRLANG(szMsg);
				}
				#endif
				result = DefWindowProc(hWnd, messg, wParam, lParam);
			} break;

		case WM_WINDOWPOSCHANGING:
			{
				WINDOWPOS* pwp = (WINDOWPOS*)lParam;
				result = DefWindowProc(hWnd, messg, wParam, lParam);
			} break;
		case WM_WINDOWPOSCHANGED:
			{
				WINDOWPOS* pwp = (WINDOWPOS*)lParam;
				result = DefWindowProc(hWnd, messg, wParam, lParam);
				// Refresh status columns
				pVCon->OnVConSizePosChanged();
			} break;

		case WM_SETCURSOR:
			{
				gpConEmu->WndProc(hWnd, messg, wParam, lParam);

				//if (!result)
				//	result = DefWindowProc(hWnd, messg, wParam, lParam);
			}
			// If an application processes this message, it should return TRUE to halt further processing or FALSE to continue.
			break;

		case WM_SYSCOMMAND:
			// -- лишние ограничения, похоже
			result = DefWindowProc(hWnd, messg, wParam, lParam);
			//if (wParam >= SC_SIZE && wParam <= SC_CONTEXTHELP/*0xF180*/)
			//{
			//	// Изменение размеров/максимизация/и т.п. окна консоли - запрещена
			//	_ASSERTE(!(wParam >= SC_SIZE && wParam <= SC_CONTEXTHELP));
			//}
			//else
			//{
			//	// По идее, сюда ничего приходить больше не должно
			//	_ASSERTE(FALSE);
			//}
			break;

		case WM_TIMER:
			{
				switch(wParam)
				{
				#ifndef SKIP_HIDE_TIMER // Не будем прятать по таймеру - только по движению мышки
				case TIMER_SCROLL_CHECK:

					if (pVCon->mb_Scroll2Visible)
					{
						if (!pVCon->CheckMouseOverScroll())
						{
							pVCon->HideScroll(FALSE/*abImmediate*/);
						}
					}

					break;
				#endif

				case TIMER_SCROLL_SHOW:

					if (pVCon->CheckMouseOverScroll() || pVCon->CheckScrollAutoPopup())
						pVCon->ShowScroll(TRUE/*abImmediate*/);
					else
						pVCon->mb_Scroll2Visible = FALSE;

					if (pVCon->m_TScrollShow.IsStarted())
						pVCon->m_TScrollShow.Stop();

					break;

				case TIMER_SCROLL_HIDE:

					if (!pVCon->CheckMouseOverScroll())
						pVCon->HideScroll(TRUE/*abImmediate*/);
					else
						pVCon->mb_Scroll2Visible = TRUE;

					if (pVCon->m_TScrollHide.IsStarted())
						pVCon->m_TScrollHide.Stop();

					break;

				case TIMER_AUTOCOPY:
					pVCon->SetAutoCopyTimer(false);
					if (!isPressed(VK_LBUTTON))
					{
						pVCon->RCon()->AutoCopyTimer();
					}
					break;
				}
				break;
			} // case WM_TIMER:

		case WM_GESTURENOTIFY:
		case WM_GESTURE:
			{
				gpConEmu->ProcessGestureMessage(hWnd, messg, wParam, lParam, result);
				break;
			} // case WM_GESTURE, WM_GESTURENOTIFY

		default:

			// Сообщение приходит из ConEmuPlugin
			if (messg == pVCon->mn_MsgTabChanged)
			{
				if (gpSet->isTabs)
				{
					//изменились табы, их нужно перечитать
#ifdef MSGLOGGER
					WCHAR szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"Tabs:Notified(%i)\n", (DWORD)wParam);
					DEBUGSTRTABS(szDbg);
#endif
					TODO("здесь хорошо бы вместо OnTimer реально обновить mn_TopProcessID")
					// иначе во время запуска PID фара еще может быть не известен...
					//gpConEmu->OnTimer(0,0); не получилось. индекс конмана не менялся, из-за этого индекс активного фара так и остался 0
					WARNING("gpConEmu->mp_TabBar->Retrieve() ничего уже не делает вообще");
					_ASSERTE(FALSE);
					gpConEmu->mp_TabBar->Retrieve();
				}
			}
			else if (messg == pVCon->mn_MsgPostFullPaint)
			{
				pVCon->Redraw();
			}
			else if (messg == pVCon->mn_MsgSavePaneSnapshot)
			{
				pVCon->SavePaneSnapshot();
			}
			else if (messg == pVCon->mn_MsgDetachPosted)
			{
				pVCon->RCon()->Detach(true, (lParam == 1));
			}
			else if (messg == gn_MsgVConTerminated)
			{
				CVirtualConsole* pVCon = (CVirtualConsole*)lParam;

				#ifdef _DEBUG
				_ASSERTE(pVCon == guard.VCon());
				int i = -100;
				wchar_t szDbg[200];
				wchar_t szTmp[128];
				{
					lstrcpy(szDbg, L"gn_MsgVConTerminated");

					i = CVConGroup::GetVConIndex(pVCon);
					if (i >= 0)
					{
						CTab tab(__FILE__,__LINE__);
						if (pVCon->RCon()->GetTab(0, tab))
							lstrcpyn(szTmp, pVCon->RCon()->GetTabTitle(tab), countof(szTmp)); // чтобы не вылезло из szDbg
						else
							wcscpy_c(szTmp, L"<GetTab(0) failed>");
						wsprintf(szDbg+_tcslen(szDbg), L": #%i: %s", i, szTmp);
					}

					lstrcat(szDbg, L"\n");
					DEBUGSTRCONS(szDbg);
				}
				#endif

				// Do not "Guard" lParam here, validation will be made in ProcessVConClosed
				CConEmuChild::ProcessVConClosed(pVCon, TRUE);
				return 0;
			}
			#ifdef _DEBUG
			else if (messg == pVCon->mn_MsgCreateDbgDlg)
			{
				pVCon->CreateDbgDlg();
			}
			#endif
			else if (messg)
			{
				result = DefWindowProc(hWnd, messg, wParam, lParam);
			}
	}

wrap:
	return result;
}

void CConEmuChild::PostRestoreChildFocus()
{
	if (!gpConEmu->CanSetChildFocus())
	{
		_ASSERTE(FALSE && "Must not get here?");
		return;
	}

	PostMessage(mh_WndBack, mn_MsgRestoreChildFocus, 0, 0);
}

void CConEmuChild::RestoreChildFocusPending(bool abSetPending)
{
	mb_RestoreChildFocusPending = abSetPending && mp_VCon->isActive(false);
}

LRESULT CConEmuChild::BackWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	// Logger
	MSG msgStr = {hWnd, messg, wParam, lParam};
	ConEmuMsgLogger::Log(msgStr, ConEmuMsgLogger::msgBack);

	if (gpSetCls->isAdvLogging >= 4)
	{
		if (gpConEmu) gpConEmu->LogMessage(hWnd, messg, wParam, lParam);
	}

	if (gpConEmu)
		gpConEmu->PreWndProc(messg);

	CVConGuard guard;
	CVirtualConsole* pVCon = NULL;
	if (messg == WM_CREATE || messg == WM_NCCREATE)
	{
		LPCREATESTRUCT lp = (LPCREATESTRUCT)lParam;
		guard = (CVirtualConsole*)lp->lpCreateParams;
		pVCon = guard.VCon();
		if (pVCon)
			gVConBkMap.Set(hWnd, pVCon);
	}
	else if (hWnd != ghBkInDestroing)
	{
		if (!gVConBkMap.Get(hWnd, &pVCon) || !guard.Attach(pVCon))
			pVCon = NULL;
	}

	if (messg == WM_SYSCHAR)
	{
		_ASSERTE(FALSE); // по идее, фокуса тут быть не должно
		// Чтобы не пищало
		result = TRUE;
		goto wrap;
	}

	if (!pVCon)
	{
		_ASSERTE(pVCon!=NULL || hWnd==ghBkInDestroing);
		result = DefWindowProc(hWnd, messg, wParam, lParam);
		goto wrap;
	}

	switch (messg)
	{
		case WM_SHOWWINDOW:
			if (wParam)
			{
				HWND hView = pVCon->GetView();
				SetWindowPos(hView, HWND_TOP, 0, 0, 0,0, SWP_NOSIZE|SWP_NOMOVE);
				SetWindowPos(hWnd, hView, 0, 0, 0,0, SWP_NOSIZE|SWP_NOMOVE);
			}
			break; // DefaultProc
		case WM_SETFOCUS:
			// Если в консоли работает "GUI" окно (GUI режим), то фокус нужно отдать туда.
			{
				// Фокус должен быть в главном окне! За исключением случая работы в GUI режиме.
				pVCon->setFocus();
			}
			return 0;
		case WM_ERASEBKGND:
			result = 0;
			break;
		case WM_PAINT:
			_ASSERTE(hWnd == pVCon->mh_WndBack);
			pVCon->OnPaintGaps(NULL/*use BeginPaint/EndPaint*/);
			break;
		case WM_PRINTCLIENT:
			if (wParam && (lParam & PRF_CLIENT))
			{
				pVCon->OnPaintGaps((HDC)wParam);
			}
			break;
		case WM_MOUSEWHEEL:
		case WM_MOUSEHWHEEL:
		case WM_MOUSEMOVE:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
		case WM_XBUTTONDBLCLK:
			if (pVCon->RCon()->isGuiOverCon())
				break;
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_ACTIVATE:
		case WM_ACTIVATEAPP:
			//case WM_MOUSEACTIVATE:
		case WM_KILLFOCUS:
			//case WM_SETFOCUS:
		case WM_VSCROLL:
			// Вся обработка в родителе
			{
				switch (messg)
				{
					case WM_VSCROLL:
						switch (LOWORD(wParam))
						{
						case SB_THUMBTRACK:
						case SB_THUMBPOSITION:
							pVCon->mb_VTracking = TRUE;
							break;
						case SB_ENDSCROLL:
							pVCon->mb_VTracking = FALSE;
							break;
						}
						pVCon->RCon()->DoScroll(LOWORD(wParam), HIWORD(wParam));
						break;

					case WM_LBUTTONUP:
						pVCon->mb_VTracking = FALSE;
						break;
				}

				TODO("Обработка ghWndWork");
				HWND hParent = ghWnd;
				_ASSERTE(GetParent(hWnd)==ghWnd);

				if (messg >= WM_MOUSEFIRST && messg <= WM_MOUSELAST)
				{
					POINT pt = {LOWORD(lParam),HIWORD(lParam)};
					MapWindowPoints(hWnd, hParent, &pt, 1);
					lParam = MAKELONG(pt.x,pt.y);
				}

				result = gpConEmu->WndProc(hParent, messg, wParam, lParam);
			}
			break;
		case WM_IME_NOTIFY:
			break;
		case WM_INPUTLANGCHANGE:
		case WM_INPUTLANGCHANGEREQUEST:
			{
				#ifdef _DEBUG
				if (IsDebuggerPresent())
				{
					WCHAR szMsg[128];
					_wsprintf(szMsg, SKIPLEN(countof(szMsg)) L"InChild %s(CP:%i, HKL:0x%08X)\n",
							  (messg == WM_INPUTLANGCHANGE) ? L"WM_INPUTLANGCHANGE" : L"WM_INPUTLANGCHANGEREQUEST",
							  (DWORD)wParam, (DWORD)lParam);
					DEBUGSTRLANG(szMsg);
				}
				#endif
				result = DefWindowProc(hWnd, messg, wParam, lParam);
			} break;

#ifdef _DEBUG
		case WM_WINDOWPOSCHANGING:
			{
				WINDOWPOS* p = (WINDOWPOS*)lParam;
				wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_WINDOWPOSCHANGED.BACK ({%i,%i}x{%i,%i} Flags=0x%08X)\n", p->x, p->y, p->cx, p->cy, p->flags);
				DEBUGSTRSIZE(szDbg);
				result = DefWindowProc(hWnd, messg, wParam, lParam);
			}
			return result;
		case WM_WINDOWPOSCHANGED:
			{
				WINDOWPOS* p = (WINDOWPOS*)lParam;
				wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"WM_WINDOWPOSCHANGED.BACK ({%i,%i}x{%i,%i} Flags=0x%08X)\n", p->x, p->y, p->cx, p->cy, p->flags);
				DEBUGSTRSIZE(szDbg);
				result = DefWindowProc(hWnd, messg, wParam, lParam);
			}
			break;
#endif
		case WM_SETCURSOR:
			{
				gpConEmu->WndProc(hWnd, messg, wParam, lParam);

				//if (!result)
				//	result = DefWindowProc(hWnd, messg, wParam, lParam);
			}
			// If an application processes this message, it should return TRUE to halt further processing or FALSE to continue.
			break;

		case WM_SYSCOMMAND:
			// -- лишние ограничения, похоже
			result = DefWindowProc(hWnd, messg, wParam, lParam);
			//if (wParam >= SC_SIZE && wParam <= SC_CONTEXTHELP/*0xF180*/)
			//{
			//	// Изменение размеров/максимизация/и т.п. окна консоли - запрещена
			//	_ASSERTE(!(wParam >= SC_SIZE && wParam <= SC_CONTEXTHELP));
			//}
			//else
			//{
			//	// По идее, сюда ничего приходить больше не должно
			//	_ASSERTE(FALSE);
			//}
			break;

		case WM_GESTURENOTIFY:
		case WM_GESTURE:
			{
				gpConEmu->ProcessGestureMessage(hWnd, messg, wParam, lParam, result);
				break;
			} // case WM_GESTURE, WM_GESTURENOTIFY

		default:

			if (pVCon && (messg == pVCon->mn_MsgRestoreChildFocus))
			{
				if (!gpConEmu->CanSetChildFocus())
				{
					// Клик по иконке открывает системное меню
					//_ASSERTE(FALSE && "Must not get here?");
				}
				else
				{
					CRealConsole* pRCon = pVCon->RCon();
					if (pVCon->isActive(false))
					{
						pRCon->GuiWndFocusRestore();
					}

					if (pRCon->GuiWnd())
					{
						pRCon->StoreGuiChildRect(NULL);
					}
				}
			}
			else
			{
				result = DefWindowProc(hWnd, messg, wParam, lParam);
			}
	}

wrap:
	return result;
}

#ifdef _DEBUG
void CConEmuChild::CreateDbgDlg()
{
	return;

	if (!hDlgTest)
	{
		if (!isMainThread())
		{
			PostMessage(mh_WndDC, mn_MsgCreateDbgDlg, 0, (LPARAM)this);
		}
		else
		{
			hDlgTest = CreateDialog(g_hInstance, MAKEINTRESOURCE(IDD_SPG_MAIN), mh_WndDC, DbgChildDlgProc);
			if (hDlgTest)
			{
				SetWindowPos(hDlgTest, NULL, 100,100, 0,0, SWP_NOSIZE|SWP_NOZORDER);
				ShowWindow(hDlgTest, SW_SHOW);
			}
		}
	}
}

INT_PTR CConEmuChild::DbgChildDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return 0;
}
#endif

LRESULT CConEmuChild::OnPaintGaps(HDC hdc)
{
	CVConGuard VCon(mp_VCon);
	if (!VCon.VCon())
	{
		_ASSERTE(VCon.VCon()!=NULL);
		return 0;
	}

	int nColorIdx = RELEASEDEBUGTEST(0/*Black*/,1/*Blue*/);
	COLORREF* clrPalette = VCon->GetColors();
	if (!clrPalette)
	{
		_ASSERTE(clrPalette!=NULL);
		return 0;
	}

	CRealConsole* pRCon = VCon->RCon();
	if (pRCon)
	{
		nColorIdx = pRCon->GetDefaultBackColorIdx();
	}

	#ifdef _DEBUG
	{
		wchar_t szPos[80]; RECT rcScreen = {}; GetWindowRect(mh_WndBack, &rcScreen);
		_wsprintf(szPos, SKIPCOUNT(szPos) L"PaintGaps VCon[%i] at {%i,%i}-{%i,%i} screen coords", mp_VCon->Index(), LOGRECTCOORDS(rcScreen));
		DEBUGSTRPAINTGAPS(szPos);
	}
	#endif

	PAINTSTRUCT ps = {};
	if (hdc)
	{
		ps.hdc = hdc;
		GetClientRect(mh_WndBack, &ps.rcPaint);
	}
	else
	{
		BeginPaint(mh_WndBack, &ps);
	}

	HBRUSH hBrush = CreateSolidBrush(clrPalette[nColorIdx]);
	if (hBrush)
	{
		if (!hdc)
		{
			FillRect(ps.hdc, &ps.rcPaint, hBrush);
		}
		else
		{
			HRGN hrgn = CreateRectRgnIndirect(&ps.rcPaint);
			RECT rcVCon = {}; GetClientRect(mh_WndDC, &rcVCon);
			MapWindowPoints(mh_WndDC, mh_WndBack, (LPPOINT)&rcVCon, 2);
			HRGN hrgnVCon = CreateRectRgnIndirect(&rcVCon);
			int iRgn = CombineRgn(hrgn, hrgn, hrgnVCon, RGN_DIFF);
			if (iRgn == SIMPLEREGION || iRgn == COMPLEXREGION)
			{
				FillRgn(ps.hdc, hrgn, hBrush);
			}
			SafeDeleteObject(hrgn);
			SafeDeleteObject(hrgnVCon);
		}

		DeleteObject(hBrush);
	}

	if (!hdc)
	{
		EndPaint(mh_WndBack, &ps);
	}

	return 0;
}

// Utilizes BeginPaint/EndPaint for obtaining HDC
LRESULT CConEmuChild::OnPaint()
{
	if (!this)
	{
		_ASSERTE(this!=NULL);
		return 0;
	}
	LRESULT result = 0;
	BOOL lbSkipDraw = FALSE;
	//if (gbInPaint)
	//    break;
	//_ASSERTE(FALSE);

	DEBUGSTRDRAW(L"CConEmuChild::OnPaint()\n");

	//2009-09-28 может так (autotabs)
	if (mb_DisableRedraw)
		return 0;

	CVConGuard VCon(mp_VCon);

	mb_PostFullPaint = FALSE;

	if (gpSetCls->isAdvLogging>2)
		VCon->RCon()->LogString("CConEmuChild::OnPaint");

	//gpSetCls->Performance(tPerfBlt, FALSE);

	if (gpConEmu->isPictureView())
	{
		// если PictureView распахнуто не на все окно - отрисовать видимую часть консоли!
		RECT rcPic, rcVRect, rcCommon;
		GetWindowRect(gpConEmu->hPictureView, &rcPic);
		GetWindowRect(mh_WndDC, &rcVRect); // Нам нужен ПОЛНЫЙ размер но ПОД тулбаром.
		//MapWindowPoints(mh_WndDC, NULL, (LPPOINT)&rcClient, 2);

		BOOL lbIntersect = IntersectRect(&rcCommon, &rcVRect, &rcPic);
		UNREFERENCED_PARAMETER(lbIntersect);

		// Убрать из отрисовки прямоугольник PictureView
		MapWindowPoints(NULL, mh_WndDC, (LPPOINT)&rcPic, 2);
		ValidateRect(mh_WndDC, &rcPic);

		MapWindowPoints(NULL, mh_WndDC, (LPPOINT)&rcVRect, 2);

		//Get ClientRect(gpConEmu->hPictureView, &rcPic);
		//Get ClientRect(mh_WndDC, &rcClient);

		// Если PicView занимает всю (почти? 95%) площадь окна
		//if (rcPic.right>=rcClient.right)
		if ((rcPic.right * rcPic.bottom) >= (rcVRect.right * rcVRect.bottom * 95 / 100))
		{
			//_ASSERTE(FALSE);
			lbSkipDraw = TRUE;

			VCon->CheckTransparent();

			// Типа "зальет цветом фона окна"?
			result = DefWindowProc(mh_WndDC, WM_PAINT, 0, 0);
		}
	}

	if (!lbSkipDraw)
	{
		if (mh_LastGuiChild)
		{
			if (!IsWindow(mh_LastGuiChild))
			{
				mh_LastGuiChild = NULL;
				Invalidate();
			}
		}
		else
		{
			mh_LastGuiChild = VCon->RCon() ? VCon->RCon()->GuiWnd() : NULL;
		}

		bool bRightClickingPaint = gpConEmu->isRightClickingPaint() && VCon->isActive(false);
		if (bRightClickingPaint)
		{
			// Скрыть окошко с "кружочком"
			gpConEmu->RightClickingPaint((HDC)INVALID_HANDLE_VALUE, VCon.VCon());
		}

		PAINTSTRUCT ps;
		HDC hDc = BeginPaint(mh_WndDC, &ps);
		UNREFERENCED_PARAMETER(hDc);

		//RECT rcClient = VCon->GetDcClientRect();
		VCon->PaintVCon(ps.hdc);

		if (bRightClickingPaint)
		{
			// Нарисует кружочек, или сбросит таймер, если кнопку отпустили
			gpConEmu->RightClickingPaint(VCon->GetIntDC()/*ps.hdc*/, VCon.VCon());
		}

		EndPaint(mh_WndDC, &ps);
	}

	//Validate();
	//gpSetCls->Performance(tPerfBlt, TRUE);
	// Если открыто окно настроек - обновить системную информацию о размерах
	gpConEmu->UpdateSizes();

	_ASSERTE(CVConGroup::isValid(VCon.VCon()));
	return result;
}

LRESULT CConEmuChild::OnSize(WPARAM wParam, LPARAM lParam)
{
	if (!this)
	{
		_ASSERTE(this!=NULL);
		return 0;
	}

	LRESULT result = 0;

	#ifdef _DEBUG
	BOOL lbIsPicView = FALSE;
	RECT rcNewClient; ::GetClientRect(mh_WndDC, &rcNewClient);
	RECT rcNewWnd;    ::GetWindowRect(mh_WndDC, &rcNewWnd);
	#endif

	if (gpSetCls->isAdvLogging)
	{
		char szInfo[128];
		_wsprintfA(szInfo, SKIPLEN(countof(szInfo)) "VCon(0x%08X).OnSize(%ux%u)", LODWORD(mh_WndDC), (UINT)LOWORD(lParam), (UINT)HIWORD(lParam));
		gpConEmu->LogString(szInfo);
	}

	// Вроде это и не нужно. Ни для Ansi ни для Unicode версии плагина
	// Все равно в ConEmu запрещен ресайз во время видимости окошка PictureView
	//if (gpConEmu->isPictureView())
	//{
	//    if (gpConEmu->hPictureView) {
	//        lbIsPicView = TRUE;
	//        gpConEmu->isPiewUpdate = true;
	//        RECT rcClient; Get ClientRect('ghWnd DC', &rcClient);
	//        //TODO: а ведь PictureView может и в QuickView активироваться...
	//        MoveWindow(gpConEmu->hPictureView, 0,0,rcClient.right,rcClient.bottom, 1);
	//        //INVALIDATE(); //InvalidateRect(hWnd, NULL, FALSE);
	//		Invalidate();
	//        //SetFocus(hPictureView); -- все равно на другой процесс фокус передать нельзя...
	//    }
	//}
	return result;
}

LRESULT CConEmuChild::OnMove(WPARAM wParam, LPARAM lParam)
{
	if (!this)
	{
		_ASSERTE(this!=NULL);
		return 0;
	}

	LRESULT result = 0;

	#ifdef _DEBUG
	RECT rcNewClient; ::GetClientRect(mh_WndDC, &rcNewClient);
	RECT rcNewWnd;    ::GetWindowRect(mh_WndDC, &rcNewWnd);
	#endif

	if (gpSetCls->isAdvLogging)
	{
		char szInfo[128];
		_wsprintfA(szInfo, SKIPLEN(countof(szInfo)) "VCon(0x%08X).OnMove(%ux%u)", LODWORD(mh_WndDC), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));
		gpConEmu->LogString(szInfo);
	}

	return result;
}

void CConEmuChild::CheckPostRedraw()
{
	if (!this)
	{
		_ASSERTE(this!=NULL);
		return;
	}
	// Если был "Отмененный" Redraw, но
	if (mb_IsPendingRedraw && mn_LastPostRedrawTick && ((GetTickCount() - mn_LastPostRedrawTick) >= CON_REDRAW_TIMOUT))
	{
		mb_IsPendingRedraw = FALSE;
		_ASSERTE(isMainThread());
		Redraw();
	}
}

void CConEmuChild::Redraw(bool abRepaintNow /*= false*/)
{
	if (!this)
	{
		_ASSERTE(this!=NULL);
		return;
	}
	if (mb_DisableRedraw)
	{
		DEBUGSTRDRAW(L" +++ RedrawWindow on DC window will be ignored!\n");
		return;
	}

	if (!isMainThread())
	{
		if (mb_RedrawPosted ||
		        (mn_LastPostRedrawTick && ((GetTickCount() - mn_LastPostRedrawTick) < CON_REDRAW_TIMOUT)))
		{
			mb_IsPendingRedraw = TRUE;
			return; // Блокируем СЛИШКОМ частые обновления
		}
		else
		{
			mn_LastPostRedrawTick = GetTickCount();
			mb_IsPendingRedraw = FALSE;
		}

		mb_RedrawPosted = TRUE; // чтобы не было кумулятивного эффекта
		PostMessage(mh_WndDC, mn_MsgPostFullPaint, 0, 0);
		return;
	}

	DEBUGSTRDRAW(L" +++ RedrawWindow on DC window called\n");
	//RECT rcClient; Get ClientRect(mh_WndDC, &rcClient);
	//MapWindowPoints(mh_WndDC, ghWnd, (LPPOINT)&rcClient, 2);
	InvalidateRect(mh_WndDC, NULL, FALSE);
	// Из-за этого - возникает двойная перерисовка
	//gpConEmu->OnPaint(0,0);
	//#ifdef _DEBUG
	//BOOL lbRc =
	//#endif
	//RedrawWindow(ghWnd, NULL, NULL,
	//	RDW_INTERNALPAINT|RDW_NOERASE|RDW_UPDATENOW);
	mb_RedrawPosted = FALSE; // Чтобы другие нити могли сделать еще пост

	if (abRepaintNow)
	{
		RECT rcClient = {};
		if (GetClientRect(mh_WndDC, &rcClient))
		{
			RedrawWindow(mh_WndDC, &rcClient, NULL, RDW_INTERNALPAINT|RDW_NOERASE|RDW_NOFRAME|RDW_UPDATENOW|RDW_VALIDATE);
		}
	}
}

// Вызывается из VConGroup::RepositionVCon
void CConEmuChild::SetVConSizePos(const RECT& arcBack, bool abReSize /*= true*/)
{
	CVirtualConsole* pVCon = mp_VCon;

	RECT rcDC = CVConGroup::CalcRect(CER_DC, arcBack, CER_BACK, pVCon);

	SetVConSizePos(arcBack, rcDC, abReSize);
}

// Вызывается из CVirtualConsole::UpdatePrepare и CConEmuChild::SetVConSizePos
void CConEmuChild::SetVConSizePos(const RECT& arcBack, const RECT& arcDC, bool abReSize /*= true*/)
{
	if (abReSize)
	{
		_ASSERTE((arcBack.right > arcBack.left) && (arcBack.bottom > arcBack.top));
		_ASSERTE((arcDC.right > arcDC.left) && (arcDC.bottom > arcDC.top));

		// Двигаем/ресайзим окошко DC
		DEBUGTEST(RECT rc1; GetClientRect(mh_WndDC, &rc1););
		SetWindowPos(mh_WndDC, HWND_TOP, arcDC.left, arcDC.top, arcDC.right - arcDC.left, arcDC.bottom - arcDC.top, 0);
		DEBUGTEST(RECT rc2; GetClientRect(mh_WndDC, &rc2););

		SetWindowPos(mh_WndBack, mh_WndDC, arcBack.left, arcBack.top, arcBack.right - arcBack.left, arcBack.bottom - arcBack.top, 0);

		Invalidate();
	}
	else
	{
		// Двигаем окошко DC
		SetWindowPos(mh_WndDC, HWND_TOP, arcDC.left, arcDC.top, 0,0, SWP_NOSIZE);
		SetWindowPos(mh_WndBack, mh_WndDC, arcBack.left, arcBack.top, 0,0, SWP_NOSIZE);
	}

	// Обновить регион скролла, если он есть
	UpdateScrollRgn(true);
}

// Refresh StatusBar columns
void CConEmuChild::OnVConSizePosChanged()
{
	if (gpSet->isStatusColumnHidden[csi_WindowBack] && gpSet->isStatusColumnHidden[csi_WindowDC])
		return;
	if (!mp_VCon->isActive(false))
		return;

	mp_VCon->mp_ConEmu->mp_Status->OnWindowReposition(NULL);
}

void CConEmuChild::SetRedraw(BOOL abRedrawEnabled)
{
	if (!this)
	{
		_ASSERTE(this!=NULL);
		return;
	}
	mb_DisableRedraw = !abRedrawEnabled;
}

void CConEmuChild::Invalidate()
{
	if (!this)
	{
		_ASSERTE(this!=NULL);
		return;
	}
	if (mb_DisableRedraw)
		return; // Иначе, при автотабах начинаются глюки

	CVirtualConsole* pVCon = mp_VCon;

	InvalidateView();

	InvalidateBack();

	UNREFERENCED_PARAMETER(pVCon);
}

bool CConEmuChild::IsInvalidatePending()
{
	return (this && (mn_InvalidateViewPending > 0));
}

void CConEmuChild::InvalidateView()
{
	if (mh_WndDC)
	{
		DEBUGSTRDRAW(L" +++ Invalidate on DC window called\n");
		LONG l = InterlockedIncrement(&mn_InvalidateViewPending);

		if (!m_LockDc.bLocked)
		{
			InvalidateRect(mh_WndDC, NULL, FALSE);
		}
		else
		{
			RECT rcClient; GetClientRect(mh_WndDC, &rcClient);
			HRGN hRgn = CreateRectRgn(0, 0, rcClient.right, rcClient.bottom);
			HRGN hLock = CreateRectRgn(m_LockDc.rcScreen.left, m_LockDc.rcScreen.top, m_LockDc.rcScreen.right, m_LockDc.rcScreen.bottom);
			int iRc = CombineRgn(hRgn, hRgn, hLock, RGN_DIFF);

			if (iRc == ERROR)
				InvalidateRect(mh_WndDC, NULL, FALSE);
			else if (iRc != NULLREGION)
				InvalidateRgn(mh_WndDC, hRgn, FALSE);

			DeleteObject(hLock);
			DeleteObject(hRgn);
		}

		if ((l == 1) && (gpSetCls->isAdvLogging >= 2))
		{
			wchar_t szInfo[80];
			_wsprintf(szInfo, SKIPCOUNT(szInfo) L"VCon[%i] invalidate called", mp_VCon->Index());
			LogString(szInfo);
		}
	}
	else
	{
		_ASSERTE(mh_WndDC != NULL);
	}
}

void CConEmuChild::InvalidateBack()
{
	if (mh_WndBack)
	{
		InvalidateRect(mh_WndBack, NULL, FALSE);
	}
	else
	{
		_ASSERTE(mh_WndBack!=NULL);
	}
}

//void CConEmuChild::Validate()
//{
//	//mb_Invalidated = FALSE;
//	//DEBUGSTRDRAW(L" +++ Validate on DC window called\n");
//	//if ('ghWnd DC') ValidateRect(ghWnd, NULL);
//}

void CConEmuChild::OnAlwaysShowScrollbar(bool abSync /*= true*/)
{
	if (m_LastAlwaysShowScrollbar != gpSet->isAlwaysShowScrollbar)
	{
		CVConGuard VCon(mp_VCon);

		if (gpSet->isAlwaysShowScrollbar == 1)
			ShowScroll(TRUE);
		else if (!m_si.nMax)
			HideScroll(TRUE);
		else
			TrackMouse();

		if (abSync && VCon->isVisible())
		{
			if (gpConEmu->GetWindowMode() != wmNormal)
				VCon->RCon()->SyncConsole2Window();
			else
				CVConGroup::SyncWindowToConsole(); // -- функция пустая, игнорируется
		}

		m_LastAlwaysShowScrollbar = gpSet->isAlwaysShowScrollbar;
	}
}

// Должна вернуть TRUE, если события мыши не нужно пропускать в консоль
BOOL CConEmuChild::TrackMouse()
{
	_ASSERTE(this);
	BOOL lbCapture = FALSE; // По умолчанию - мышь не перехватывать

	CVirtualConsole* pVCon = mp_VCon;
	CVConGuard guard(pVCon);

	pVCon->WasHighlightRowColChanged();

	#ifdef _DEBUG
	CRealConsole* pRCon = pVCon->RCon();
	BOOL lbBufferMode = pRCon->isBufferHeight() && !pRCon->GuiWnd();
	#endif

	BOOL lbOverVScroll = CheckMouseOverScroll();

	if (pVCon->RCon()->isGuiVisible())
	{
		if (mb_ScrollVisible || mb_Scroll2Visible)
			HideScroll(TRUE);
	}
	else if (gpSet->isAlwaysShowScrollbar == 1)
	{
		// Если полоса прокрутки показывается всегда - то она и не прячется
		if (!mb_ScrollVisible)
			ShowScroll(TRUE);
	}
	else if (lbOverVScroll)
	{
		if (!mb_Scroll2Visible && gpConEmu->isMeForeground(false, true))
		{
			mb_Scroll2Visible = TRUE;
			ShowScroll(FALSE/*abImmediate*/); // Если gpSet->isAlwaysShowScrollbar==1 - сама разберется
		}
		#ifndef SKIP_HIDE_TIMER
		else if (mb_ScrollVisible && (gpSet->isAlwaysShowScrollbar != 1) && !m_TScrollCheck.IsStarted())
		{
			m_TScrollCheck.Start();
		}
		#endif
	}
	else if (mb_Scroll2Visible || mb_ScrollVisible)
	{
		_ASSERTE(gpSet->isAlwaysShowScrollbar != 1);
		mb_Scroll2Visible = FALSE;
		// Запустить таймер скрытия
		HideScroll(FALSE/*abImmediate*/);
	}

	lbCapture = (lbOverVScroll && mb_ScrollVisible);
	return lbCapture;
}

bool CConEmuChild::CheckMouseOverScroll(bool abCheckVisible /*= false*/)
{
	if (abCheckVisible)
	{
		if (gpSet->isAlwaysShowScrollbar == 0)
		{
			return false; // не показывается вообще
		}
		else if ((gpSet->isAlwaysShowScrollbar != 1) // 1 -- показывать всегда
			&& !mb_ScrollVisible)
		{
			return false; // не показывается сейчас
		}
	}

	bool lbOverVScroll = false;

	CVirtualConsole* pVCon = mp_VCon;
	CVConGuard guard(pVCon);

	// Вроде бы в активной? Или в this?
	CVConGuard VCon;
	CRealConsole* pRCon = (gpConEmu->GetActiveVCon(&VCon) >= 0) ? VCon->RCon() : NULL;

	if (pRCon)
	{
		BOOL lbBufferMode = pRCon->isBufferHeight() && !pRCon->GuiWnd();

		if (lbBufferMode)
		{
			// Если прокрутку тащили мышкой и отпустили
			if (mb_VTracking && !isPressed(VK_LBUTTON))
			{
				// Сбросим флажок
				mb_VTracking = FALSE;
			}

			// чтобы полоса не скрылась, когда ее тащат мышкой
			if (mb_VTracking)
			{
				lbOverVScroll = true;
			}
			else // Теперь проверим, если мышь в над скроллбаром - показать его
			{
				POINT ptCur; RECT rcScroll, rcClient;
				GetCursorPos(&ptCur);
				GetWindowRect(mh_WndBack, &rcClient);
				//GetWindowRect(mh_WndScroll, &rcScroll);
				rcScroll = rcClient;
				rcScroll.left = rcScroll.right - GetSystemMetrics(SM_CXVSCROLL);

				if (PtInRect(&rcScroll, ptCur))
				{
					// Если прокрутка УЖЕ видна - то мышку в консоль не пускать! Она для прокрутки!
					if (mb_ScrollVisible)
						lbOverVScroll = true;
					// Если не проверять - не получится начать выделение с правого края окна
					//if (!gpSet->isSelectionModifierPressed())
					else if (!(isPressed(VK_SHIFT) || isPressed(VK_CONTROL) || isPressed(VK_MENU) || isPressed(VK_LBUTTON)))
						lbOverVScroll = true;
				}
			}
		}
	}

	return lbOverVScroll;
}

BOOL CConEmuChild::CheckScrollAutoPopup()
{
	return mb_ScrollAutoPopup;
}

bool CConEmuChild::InScroll()
{
	if (mb_VTracking)
	{
		if (!isPressed(VK_LBUTTON))
			mb_VTracking = FALSE;
	}

	return (mb_VTracking != FALSE);
}

void CConEmuChild::SetScroll(BOOL abEnabled, int anTop, int anVisible, int anHeight)
{
	//int nCurPos = 0;
	//BOOL lbScrollRc = FALSE;
	//SCROLLINFO si;
	//ZeroMemory(&si, sizeof(si));
	m_si.cbSize = sizeof(m_si);
	m_si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE; // | SIF_TRACKPOS;
	m_si.nMin = 0;

	if (!abEnabled)
	{
		m_si.nPos = 0;
		m_si.nPage = 0;
		m_si.nMax = 0; //(gpSet->isAlwaysShowScrollbar == 1) ? 1 : 0;
		//m_si.fMask |= SIF_DISABLENOSCROLL;
	}
	else
	{
		m_si.nPos = anTop;
		m_si.nPage = anVisible - 1;
		m_si.nMax = anHeight;
	}

	//// Если режим "BufferHeight" включен - получить из консольного окна текущее состояние полосы прокрутки
	//if (con.bBufferHeight) {
	//    lbScrollRc = GetScrollInfo(hConWnd, SB_VERT, &si);
	//} else {
	//    // Сбросываем параметры так, чтобы полоса не отображалась (все на 0)
	//}
	//TODO("Нужно при необходимости 'всплыть' полосу прокрутки");
	//nCurPos = SetScrollInfo(mh_WndDC/*mh_WndScroll*/, SB_VERT, &m_si, true);

	if (!abEnabled)
	{
		mb_ScrollAutoPopup = FALSE;

		if (gpSet->isAlwaysShowScrollbar == 1)
		{
			if (!mb_ScrollVisible)
			{
				ShowScroll(TRUE);
			}
			else
			{
				MySetScrollInfo(TRUE, FALSE);
				//SetScrollInfo(mh_WndDC/*mh_WndScroll*/, SB_VERT, &m_si, TRUE);
				//if (!mb_ScrollDisabled)
				//{
				//	EnableScrollBar(mh_WndDC/*mh_WndScroll*/, SB_VERT, ESB_DISABLE_BOTH);
				//	mb_ScrollDisabled = TRUE;
				//}
			}
		}
		else
		{
			HideScroll(TRUE/*сразу!*/);
		}
	}
	else
	{
		//if (!IsWindowEnabled(mh_WndScroll))
		//{
		//	//EnableWindow(mh_WndScroll, TRUE);
		//	EnableScrollBar(mh_WndDC/*mh_WndScroll*/, SB_VERT, ESB_ENABLE_BOTH);
		//}

		// Показать прокрутку, если например буфер скроллится с клавиатуры
		if ((m_si.nPos > 0) && (m_si.nPos < (m_si.nMax - (int)m_si.nPage - 1)) && gpSet->isAlwaysShowScrollbar)
		{
			mb_ScrollAutoPopup = (gpSet->isAlwaysShowScrollbar == 2);

			if (!mb_Scroll2Visible)
			{
				ShowScroll((gpSet->isAlwaysShowScrollbar == 1));
			}
		}
		else
		{
			mb_ScrollAutoPopup = FALSE;
		}

		if (mb_ScrollVisible)
		{
			MySetScrollInfo(TRUE, TRUE);
			//SetScrollInfo(mh_WndDC/*mh_WndScroll*/, SB_VERT, &m_si, TRUE);
			//if (mb_ScrollDisabled)
			//{
			//	EnableScrollBar(mh_WndDC/*mh_WndScroll*/, SB_VERT, ESB_ENABLE_BOTH);
			//	mb_ScrollDisabled = FALSE;
			//}
		}
	}
}

void CConEmuChild::MySetScrollInfo(BOOL abSetEnabled, BOOL abEnableValue)
{
	SCROLLINFO si = m_si;

	if (/*!mb_ScrollVisible &&*/ !m_si.nMax && (gpSet->isAlwaysShowScrollbar == 1))
	{
		ShowScrollBar(mh_WndBack, SB_VERT, TRUE);
		// Прокрутка всегда показывается! Скрывать нельзя!
		si.nPage = 1;
		si.nMax = 100;
	}

	si.fMask |= SIF_PAGE|SIF_POS|SIF_RANGE/*|SIF_DISABLENOSCROLL*/;

	SetScrollInfo(mh_WndBack, SB_VERT, &si, TRUE);

	if (abSetEnabled)
	{
		if (abEnableValue)
		{
			EnableScrollBar(mh_WndBack/*mh_WndScroll*/, SB_VERT, ESB_ENABLE_BOTH);
			mb_ScrollDisabled = FALSE;
		}
		else
		{
			EnableScrollBar(mh_WndBack/*mh_WndScroll*/, SB_VERT, ESB_DISABLE_BOTH);
			mb_ScrollDisabled = TRUE;
		}
	}
}

void CConEmuChild::UpdateScrollRgn(bool abForce /*= false*/)
{
	bool bNeedRgn = (mb_ScrollVisible && (gpSet->isAlwaysShowScrollbar == 2));

	if ((bNeedRgn == mb_ScrollRgnWasSet) && !abForce)
		return;

	HRGN hRgn = NULL;
	if (mb_ScrollVisible && (gpSet->isAlwaysShowScrollbar == 2))
	{
		RECT rcDc = {}; GetClientRect(mh_WndDC, &rcDc);
		RECT rcScroll = {}; GetWindowRect(mh_WndBack, &rcScroll);
		MapWindowPoints(NULL, mh_WndDC, (LPPOINT)&rcScroll, 2);
		rcScroll.left = rcScroll.right - GetSystemMetrics(SM_CXVSCROLL);
		TODO("Horizontal scrolling");
		hRgn = CreateRectRgn(rcDc.left, rcDc.top, rcDc.right, rcDc.bottom);
		HRGN hScrlRgn = CreateRectRgn(rcScroll.left, rcScroll.top, rcScroll.right, rcScroll.bottom);
		int iRc = CombineRgn(hRgn, hRgn, hScrlRgn, RGN_DIFF);
		DeleteObject(hScrlRgn);
		UNREFERENCED_PARAMETER(iRc);
	}

	SetWindowRgn(mh_WndDC, hRgn, TRUE);

	mb_ScrollRgnWasSet = (hRgn != NULL);

	if (hRgn)
	{
		InvalidateRect(mh_WndBack, NULL, FALSE);
	}
}

void CConEmuChild::ShowScroll(BOOL abImmediate)
{
	bool bTShow = false, bTCheck = false;

	if (abImmediate || (gpSet->isAlwaysShowScrollbar == 1))
	{
		if (!mb_ScrollVisible && !m_si.nMax)
		{
			// Прокрутка всегда показывается! Скрывать нельзя!
			//m_si.nMax = (gpSet->isAlwaysShowScrollbar == 1) ? 1 : 0;
			SCROLLINFO si = {sizeof(si), SIF_PAGE|SIF_POS|SIF_RANGE/*|SIF_DISABLENOSCROLL*/, 0, 100, 1};
			SetScrollInfo(mh_WndBack, SB_VERT, &si, TRUE);
		}

		mb_ScrollVisible = TRUE; mb_Scroll2Visible = TRUE;

		#ifdef _DEBUG
		SCROLLINFO si = {sizeof(si), SIF_PAGE|SIF_POS|SIF_RANGE};
		GetScrollInfo(mh_WndBack, SB_VERT, &si);
		#endif

		//int nCurPos = -1;
		if (m_si.nMax || (gpSet->isAlwaysShowScrollbar == 1))
		{
			MySetScrollInfo(FALSE, FALSE);
			//nCurPos = SetScrollInfo(mh_WndDC/*mh_WndScroll*/, SB_VERT, &m_si, TRUE); UNREFERENCED_PARAMETER(nCurPos);
		}

		if (mb_ScrollDisabled && m_si.nMax > 1)
		{
			EnableScrollBar(mh_WndBack/*mh_WndScroll*/, SB_VERT, ESB_ENABLE_BOTH);
			mb_ScrollDisabled = FALSE;
		}
		else if (!mb_ScrollDisabled && !m_si.nMax)
		{
			EnableScrollBar(mh_WndBack/*mh_WndScroll*/, SB_VERT, ESB_DISABLE_BOTH);
			mb_ScrollDisabled = TRUE;
		}

		ShowScrollBar(mh_WndBack, SB_VERT, TRUE);

		#ifdef _DEBUG
		GetScrollInfo(mh_WndBack, SB_VERT, &si);
		#endif

		TODO("Scroll: Horizontal");

		#ifdef _DEBUG
		DWORD dwStyle = GetWindowLong(mh_WndBack, GWL_STYLE);
		//if (!(dwStyle & WS_VSCROLL))
		//	SetWindowLong(mh_WndDC, GWL_STYLE, dwStyle | WS_VSCROLL);
		#endif

		//if (!IsWindowVisible(mh_WndScroll))
		//	apiShowWindow(mh_WndScroll, SW_SHOWNOACTIVATE);

		//SetWindowPos(mh_WndScroll, HWND_TOP, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE|SWP_SHOWWINDOW);

		if ((gpSet->isAlwaysShowScrollbar != 1))
		{
			bTCheck = true;
		}
	}
	else
	{
		mb_Scroll2Visible = TRUE;
		bTShow = true;
	}

	UpdateScrollRgn();

	#ifndef SKIP_HIDE_TIMER
	if (bTCheck)
	{
		m_TScrollCheck.Start(mb_ScrollAutoPopup ? TIMER_SCROLL_CHECK_DELAY2 : TIMER_SCROLL_CHECK_DELAY);
	}
	else if (m_TScrollCheck.IsStarted())
	{
		m_TScrollCheck.Stop();
	}
	#endif

	//
	if (bTShow)
		m_TScrollShow.Start(mb_ScrollAutoPopup ? TIMER_SCROLL_SHOW_DELAY2 : TIMER_SCROLL_SHOW_DELAY);
	else if (m_TScrollShow.IsStarted())
		m_TScrollShow.Stop();

	//
	if (m_TScrollHide.IsStarted())
		m_TScrollHide.Stop();
}

void CConEmuChild::HideScroll(BOOL abImmediate)
{
	bool bTHide = false;
	mb_ScrollAutoPopup = FALSE;

	CVirtualConsole* pVCon = mp_VCon;

	if ((gpSet->isAlwaysShowScrollbar == 1) && !pVCon->GuiWnd())
	{
		// Прокрутка всегда показывается! Скрывать нельзя!
		SCROLLINFO si = {sizeof(si), SIF_PAGE|SIF_POS|SIF_RANGE/*|SIF_DISABLENOSCROLL*/, 0, 100, 1};
		SetScrollInfo(mh_WndBack, SB_VERT, &si, TRUE);
		if (!mb_ScrollDisabled)
		{
			EnableScrollBar(mh_WndBack/*mh_WndScroll*/, SB_VERT, ESB_DISABLE_BOTH);
			mb_ScrollDisabled = TRUE;
		}
		Invalidate();
	}
	else if (abImmediate)
	{
		mb_ScrollVisible = FALSE;
		mb_Scroll2Visible = FALSE;

		SCROLLINFO si = {sizeof(si), SIF_PAGE|SIF_POS|SIF_RANGE};
		int nCurPos = SetScrollInfo(mh_WndBack, SB_VERT, &si, TRUE);  UNREFERENCED_PARAMETER(nCurPos);

		TODO("Scroll: Horizontal");
		#ifdef _DEBUG
		DWORD dwStyle = GetWindowLong(mh_WndBack, GWL_STYLE);
		//if (dwStyle & WS_VSCROLL)
		//	SetWindowLong(mh_WndBack, GWL_STYLE, dwStyle & ~WS_VSCROLL);
		#endif

		Invalidate();

		//if (IsWindowVisible(mh_WndScroll))
		//	apiShowWindow(mh_WndScroll, SW_HIDE);
		//RECT rcScroll; GetWindowRect(mh_WndScroll, &rcScroll);
		//// вьюпорт невидимый, передернуть нужно основное окно
		//MapWindowPoints(NULL, ghWnd, (LPPOINT)&rcScroll, 2);
		//InvalidateRect(ghWnd, &rcScroll, FALSE);
	}
	else
	{
		mb_Scroll2Visible = FALSE;
		bTHide = true;
		//m_TScrollHide.Start(TIMER_SCROLL_HIDE_DELAY);
	}

	UpdateScrollRgn();

	#ifndef SKIP_HIDE_TIMER
	if (m_TScrollCheck.IsStarted())
		m_TScrollCheck.Stop();
	#endif

	//
	if (m_TScrollShow.IsStarted())
		m_TScrollShow.Stop();

	//
	if (bTHide)
		m_TScrollHide.Start(TIMER_SCROLL_HIDE_DELAY);
	else if (m_TScrollHide.IsStarted())
		m_TScrollHide.Stop();
}

// 0 - блокировок нет
// 1 - заблокировано, игнорировать заполнение консоли пробелами (таймаут обновления буферов)
// 2 - заблокировано, при любом чихе в этот прямоугольник - снимать блокировку
int CConEmuChild::IsDcLocked(RECT* CurrentConLockedRect)
{
	if (!m_LockDc.bLocked)
		return 0;

	if (CurrentConLockedRect)
		*CurrentConLockedRect = m_LockDc.rcCon;
	_ASSERTE(!(m_LockDc.rcCon.left < 0 || m_LockDc.rcCon.top < 0 || m_LockDc.rcCon.right < 0 || m_LockDc.rcCon.bottom < 0));

	DWORD nDelta = GetTickCount() - m_LockDc.nLockTick;

	return (nDelta <= LOCK_DC_RECT_TIMEOUT) ? 1 : 2;
}

// Это было сделано для обработки в хуках OnStretchDIBits
// Плагин фара пытался рисовать прямо на канвасе VCon и безуспешно
void CConEmuChild::LockDcRect(bool bLock, RECT* Rect)
{
	if (!bLock)
	{
		if (m_LockDc.bLocked)
		{
			m_LockDc.bLocked = FALSE;
			Invalidate();
		}
	}
	else
	{
		if (!Rect)
		{
			_ASSERTE(Rect!=NULL);
			return;
		}

		CVirtualConsole* pVCon = mp_VCon;
		CVConGuard guard(pVCon);

		TODO("Хорошо бы здесь запомнить в CompatibleBitmap то, что нарисовали. Это нужно делать в MainThread!!");

		m_LockDc.rcScreen = *Rect;
		COORD cr = pVCon->ClientToConsole(m_LockDc.rcScreen.left+1, m_LockDc.rcScreen.top+1, true);
		m_LockDc.rcCon.left = cr.X; m_LockDc.rcCon.top = cr.Y;
		cr = pVCon->ClientToConsole(m_LockDc.rcScreen.right-1, m_LockDc.rcScreen.bottom-1, true);
		m_LockDc.rcCon.right = cr.X; m_LockDc.rcCon.bottom = cr.Y;

		if (m_LockDc.rcCon.left < 0 || m_LockDc.rcCon.top < 0 || m_LockDc.rcCon.right < 0 || m_LockDc.rcCon.bottom < 0)
		{
			_ASSERTE(!(m_LockDc.rcCon.left < 0 || m_LockDc.rcCon.top < 0 || m_LockDc.rcCon.right < 0 || m_LockDc.rcCon.bottom < 0));
			LockDcRect(false);
			return;
		}

		m_LockDc.nLockTick = GetTickCount();
		m_LockDc.bLocked = TRUE;
		ValidateRect(mh_WndDC, &m_LockDc.rcScreen);
	}
}

void CConEmuChild::SetAutoCopyTimer(bool bEnabled)
{
	if (bEnabled)
	{
		DEBUGSTRTEXTSEL(L"CConEmuChild::SetAutoCopyTimer(true)");
		m_TAutoCopy.Stop();
		m_TAutoCopy.Start(TIMER_AUTOCOPY_DELAY);
	}
	else
	{
		DEBUGSTRTEXTSEL(L"CConEmuChild::SetAutoCopyTimer(false)");
		m_TAutoCopy.Stop();
	}
}

void CConEmuChild::PostDetach(bool bSendCloseConsole /*= false*/)
{
	PostMessage(mh_WndDC, mn_MsgDetachPosted, 0, (LPARAM)bSendCloseConsole);
}
