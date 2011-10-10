
/*
Copyright (c) 2009-2011 Maximus5
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


#include "Header.h"
#include "../common/common.hpp"
#include "../common/WinObjects.h"
#include "ConEmu.h"
#include "ConEmuChild.h"
#include "Options.h"
#include "TabBar.h"
#include "VirtualConsole.h"
#include "RealConsole.h"

#if defined(__GNUC__)
#define EXT_GNUC_LOG
#endif

#define DEBUGSTRDRAW(s) //DEBUGSTR(s)
#define DEBUGSTRTABS(s) //DEBUGSTR(s)
#define DEBUGSTRLANG(s) //DEBUGSTR(s)

CConEmuChild::CConEmuChild()
{
	mn_MsgTabChanged = RegisterWindowMessage(CONEMUTABCHANGED);
	mn_MsgPostFullPaint = RegisterWindowMessage(L"CConEmuChild::PostFullPaint");
	mb_PostFullPaint = FALSE;
	mn_LastPostRedrawTick = 0;
	mb_IsPendingRedraw = FALSE;
	mb_RedrawPosted = FALSE;
	memset(&Caret, 0, sizeof(Caret));
	mb_DisableRedraw = FALSE;
	mh_WndDC = NULL;
}

CConEmuChild::~CConEmuChild()
{
	if (mh_WndDC)
	{
		DestroyWindow(mh_WndDC);
		mh_WndDC = NULL;
	}
}

HWND CConEmuChild::CreateView()
{
	if (!this)
	{
		_ASSERTE(this!=NULL);
		return NULL;
	}
	if (mh_WndDC)
	{
		_ASSERTE(mh_WndDC == NULL);
		return mh_WndDC;
	}

	if (!gpConEmu->isMainThread())
	{
		// Окно должно создаваться в главной нити!
		HWND hCreate = gpConEmu->PostCreateView(this);
		_ASSERTE(hCreate && (hCreate == mh_WndDC));
		return mh_WndDC;
	}

	// Имя класса - то же самое, что и у главного окна
	DWORD style = /*WS_VISIBLE |*/ WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	//RECT rc = gpConEmu->DCClientRect();
	RECT rcMain; GetClientRect(ghWnd, &rcMain);
	RECT rc = gpConEmu->CalcRect(CER_DC, rcMain, CER_MAINCLIENT);
	CVirtualConsole* pVCon = (CVirtualConsole*)this;
	_ASSERTE(pVCon);
	mh_WndDC = CreateWindow(szClassName, 0, style, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, ghWnd, NULL, (HINSTANCE)g_hInstance, pVCon);

	if (!mh_WndDC)
	{
		MBoxA(_T("Can't create DC window!"));
		return NULL; //
	}

	//SetClassLong('ghWnd DC', GCL_HBRBACKGROUND, (LONG)gpConEmu->m_Back->mh_BackBrush);
	SetWindowPos(mh_WndDC, HWND_TOP, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
	//gpConEmu->dcWindowLast = rc; //TODO!!!

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

BOOL CConEmuChild::ShowView(int nShowCmd)
{
	if (!this || !mh_WndDC)
		return FALSE;

	BOOL bRc = FALSE;
	DWORD nTID = 0, nPID = 0;

	// Должно быть создано в главной нити!
	nTID = GetWindowThreadProcessId(mh_WndDC, &nPID);

	#ifdef _DEBUG
	DWORD nMainThreadID = GetWindowThreadProcessId(ghWnd, &nPID);
	_ASSERTE(nTID==nMainThreadID);
	#endif

	// Если это "GUI" режим - могут возникать блокировки из-за дочернего окна
	HWND hChildGUI = ((CVirtualConsole*)this)->GuiWnd();

	if ((GetCurrentThreadId() != nTID) || (hChildGUI != NULL))
	{
		bRc = ShowWindowAsync(mh_WndDC, nShowCmd);
	}
	else
	{
		bRc = ShowWindow(mh_WndDC, nShowCmd);
	}
	return bRc;
}

LRESULT CConEmuChild::ChildWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	CVirtualConsole* pVCon = NULL;
	if (messg == WM_CREATE || messg == WM_NCCREATE)
	{
		LPCREATESTRUCT lp = (LPCREATESTRUCT)lParam;
		pVCon = (CVirtualConsole*)lp->lpCreateParams;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pVCon);
	}
	else
	{
		pVCon = (CVirtualConsole*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	}

	if (messg == WM_SYSCHAR)
	{
		// Чтобы не пищало
		result = TRUE;
		if (pVCon)
		{
			HWND hGuiWnd = pVCon->GuiWnd();
			if (hGuiWnd)
			{
				TODO("Послать Alt+<key> в прилепленное GUI приложение?");
				// pVCon->RCon()->PostMessage(hGuiWnd, messg, wParam, lParam);
			}
		}
		goto wrap;
	}

	if (!pVCon)
	{
		_ASSERTE(pVCon!=NULL);
		result = DefWindowProc(hWnd, messg, wParam, lParam);
		goto wrap;
	}

	switch (messg)
	{
		case WM_SETFOCUS:
			// Если в консоли работает "GUI" окно (GUI режим), то фокус нужно отдать туда.
			{
				// Фокус должен быть в главном окне! За исключением случая работы в GUI режиме.
				HWND hGuiWnd = pVCon->GuiWnd();
				SetFocus(hGuiWnd ? hGuiWnd : ghWnd);
			}
			return 0;
		case WM_ERASEBKGND:
			result = 0;
			break;
		case WM_PAINT:
			result = pVCon->OnPaint();
			break;
		case WM_SIZE:
			result = pVCon->OnSize(wParam, lParam);
			break;
		case WM_CREATE:
			break;
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_MOUSEWHEEL:
		case WM_ACTIVATE:
		case WM_ACTIVATEAPP:
			//case WM_MOUSEACTIVATE:
		case WM_KILLFOCUS:
			//case WM_SETFOCUS:
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
		case WM_VSCROLL:
			// Вся обработка в родителе
			{
				POINT pt = {LOWORD(lParam),HIWORD(lParam)};
				MapWindowPoints(hWnd, ghWnd, &pt, 1);
				lParam = MAKELONG(pt.x,pt.y);
				result = gpConEmu->WndProc(ghWnd, messg, wParam, lParam);
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
				WINDOWPOS* pwp = (WINDOWPOS*)lParam;
				result = DefWindowProc(hWnd, messg, wParam, lParam);
			}
			return result;
		case WM_WINDOWPOSCHANGED:
			{
				WINDOWPOS* pwp = (WINDOWPOS*)lParam;
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
					_ASSERT(FALSE);
					gpConEmu->mp_TabBar->Retrieve();
				}
			}
			else if (messg == pVCon->mn_MsgPostFullPaint)
			{
				pVCon->Redraw();
			}
			else if (messg)
			{
				result = DefWindowProc(hWnd, messg, wParam, lParam);
			}
	}

wrap:
	return result;
}

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
	//_ASSERT(FALSE);

	//2009-09-28 может так (autotabs)
	if (mb_DisableRedraw)
		return 0;

	mb_PostFullPaint = FALSE;

	if (gpSet->isAdvLogging>1)
		gpConEmu->ActiveCon()->RCon()->LogString("CConEmuChild::OnPaint");

	gpSet->Performance(tPerfBlt, FALSE);

	if (gpConEmu->isPictureView())
	{
		// если PictureView распахнуто не на все окно - отрисовать видимую часть консоли!
		RECT rcPic, rcClient, rcCommon;
		GetWindowRect(gpConEmu->hPictureView, &rcPic);
		GetClientRect(mh_WndDC, &rcClient); // Нам нужен ПОЛНЫЙ размер но ПОД тулбаром.
		MapWindowPoints(mh_WndDC, NULL, (LPPOINT)&rcClient, 2);

		#ifdef _DEBUG
		BOOL lbIntersect =
		#endif
		IntersectRect(&rcCommon, &rcClient, &rcPic);

		// Убрать из отрисовки прямоугольник PictureView
		MapWindowPoints(NULL, mh_WndDC, (LPPOINT)&rcPic, 2);
		ValidateRect(mh_WndDC, &rcPic);


		GetClientRect(gpConEmu->hPictureView, &rcPic);
		GetClientRect(mh_WndDC, &rcClient);

		// Если PicView занимает всю (почти? 95%) площадь окна
		//if (rcPic.right>=rcClient.right)
		if ((rcPic.right * rcPic.bottom) >= (rcClient.right * rcClient.bottom * 95 / 100))
		{
			//_ASSERTE(FALSE);
			lbSkipDraw = TRUE;
			// Типа "зальет цветом фона окна"?
			result = DefWindowProc(mh_WndDC, WM_PAINT, 0, 0);
		}
	}

	if (!lbSkipDraw)
	{
		CVirtualConsole* pVCon = (CVirtualConsole*)this;
		_ASSERTE(pVCon!=NULL);

		PAINTSTRUCT ps;
		#ifdef _DEBUG
		HDC hDc =
		#endif
		BeginPaint(mh_WndDC, &ps);

		RECT rcClient = {}; GetClientRect(mh_WndDC, &rcClient);
		pVCon->Paint(ps.hdc, rcClient);

		if (gpConEmu->isRightClickingPaint() && gpConEmu->isActive(pVCon))
		{
			// Нарисует кружочек, или сбросит таймер, если кнопку отпустили
			gpConEmu->RightClickingPaint(ps.hdc, pVCon);
		}

		EndPaint(mh_WndDC, &ps);
	}

	Validate();
	gpSet->Performance(tPerfBlt, TRUE);
	// Если открыто окно настроек - обновить системную информацию о размерах
	gpConEmu->UpdateSizes();
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
	RECT rcNewClient; GetClientRect(mh_WndDC,&rcNewClient);
#endif
	// Вроде это и не нужно. Ни для Ansi ни для Unicode версии плагина
	// Все равно в ConEmu запрещен ресайз во время видимости окошка PictureView
	//if (gpConEmu->isPictureView())
	//{
	//    if (gpConEmu->hPictureView) {
	//        lbIsPicView = TRUE;
	//        gpConEmu->isPiewUpdate = true;
	//        RECT rcClient; GetClientRect('ghWnd DC', &rcClient);
	//        //TODO: а ведь PictureView может и в QuickView активироваться...
	//        MoveWindow(gpConEmu->hPictureView, 0,0,rcClient.right,rcClient.bottom, 1);
	//        //INVALIDATE(); //InvalidateRect(hWnd, NULL, FALSE);
	//		Invalidate();
	//        //SetFocus(hPictureView); -- все равно на другой процесс фокус передать нельзя...
	//    }
	//}
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
		_ASSERTE(gpConEmu->isMainThread());
		Redraw();
	}
}

void CConEmuChild::Redraw()
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

	if (!gpConEmu->isMainThread())
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
	//RECT rcClient; GetClientRect(mh_WndDC, &rcClient);
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

	// Здесь нужно invalidate'ить и GAPS !!!
	// Иначе при запуске "conemu.exe /max" - gaps остаются зелеными, что означает потенциальную проблему

	//2009-06-22 Опять поперла непрорисовка. Причем если по экрану окошко повозить - изображение нормальное
	// Так что пока лучше два раза нарисуем...
	//if (mb_Invalidated) {
	//	DEBUGSTRDRAW(L" ### Warning! Invalidate on DC window will be duplicated\n");
	////	return;
	//}
	if (mh_WndDC)
	{
		DEBUGSTRDRAW(L" +++ Invalidate on DC window called\n");
		//RECT rcClient; GetClientRect('ghWnd DC', &rcClient);
		//MapWindowPoints('ghWnd DC', ghWnd, (LPPOINT)&rcClient, 2);
		//InvalidateRect(ghWnd, &rcClient, FALSE);

		//RECT rcMainClient; GetClientRect(ghWnd, &rcMainClient);
		//RECT rcClient = gpConEmu->CalcRect(CER_BACK, rcMainClient, CER_MAINCLIENT);
		//InvalidateRect(ghWnd, &rcClient, FALSE);

		InvalidateRect(mh_WndDC, NULL, FALSE);
	}
	else
	{
		_ASSERTE(mh_WndDC != NULL);
	}
}

void CConEmuChild::Validate()
{
	//mb_Invalidated = FALSE;
	//DEBUGSTRDRAW(L" +++ Validate on DC window called\n");
	//if ('ghWnd DC') ValidateRect(ghWnd, NULL);
}








WARNING("!!! На время скроллирования необходимо установить AutoScroll в TRUE, а при отпускании ползунка - вернуть старое значение!");
TODO("И вообще, скроллинг нужно передавать через pipe");

//#define SCROLLHIDE_TIMER_ID 1726
#define TIMER_SCROLL_SHOW         3201
#define TIMER_SCROLL_SHOW_DELAY   1000
#define TIMER_SCROLL_SHOW_DELAY2  500
#define TIMER_SCROLL_HIDE         3202
#define TIMER_SCROLL_HIDE_DELAY   1000
#define TIMER_SCROLL_CHECK        3203
#define TIMER_SCROLL_CHECK_DELAY  250
#define TIMER_SCROLL_CHECK_DELAY2 1000

CConEmuBack::CConEmuBack()
{
	mh_WndBack = NULL;
	mh_WndScroll = NULL;
	mh_BackBrush = NULL;
	mn_LastColor = -1;
	mn_ScrollWidth = 0;
	mb_ScrollVisible = FALSE; mb_Scroll2Visible = FALSE; /*mb_ScrollTimerSet = FALSE;*/ mb_ScrollAutoPopup = FALSE;
	//m_TScrollShow, m_TScrollHide, m_TScrollCheck
	memset(&mrc_LastClient, 0, sizeof(mrc_LastClient));
	mb_LastTabVisible = false; mb_LastAlwaysScroll = false;
	mb_VTracking = false;
#ifdef _DEBUG
	mn_ColorIdx = 1; // Blue
#else
	mn_ColorIdx = 0; // Black
#endif
	//mh_UxTheme = NULL; mh_ThemeData = NULL; mfn_OpenThemeData = NULL; mfn_CloseThemeData = NULL;
}

CConEmuBack::~CConEmuBack()
{
}

HWND CConEmuBack::Create()
{
	mn_LastColor = gpSet->GetColors()[mn_ColorIdx];
	mh_BackBrush = CreateSolidBrush(mn_LastColor);
	DWORD dwLastError = 0;
	WNDCLASS wc = {CS_OWNDC/*|CS_SAVEBITS*/, CConEmuBack::BackWndProc, 0, 0,
	               g_hInstance, NULL, LoadCursor(NULL, IDC_ARROW),
	               mh_BackBrush,
	               NULL, szClassNameBack
	              };

	if (!RegisterClass(&wc))
	{
		dwLastError = GetLastError();
		mh_WndBack = (HWND)-1; // чтобы родитель не ругался
		mh_WndScroll = (HWND)-1;
		MBoxA(_T("Can't register background window class!"));
		return NULL;
	}

	// Scroller
	wc.lpfnWndProc = CConEmuBack::ScrollWndProc;
	wc.lpszClassName = szClassNameScroll;

	if (!RegisterClass(&wc))
	{
		dwLastError = GetLastError();
		mh_WndBack = (HWND)-1; // чтобы родитель не ругался
		mh_WndScroll = (HWND)-1;
		MBoxA(_T("Can't register scroller window class!"));
		return NULL;
	}

	DWORD style = /*WS_VISIBLE |*/ WS_CHILD | WS_CLIPSIBLINGS ;
	//RECT rc = gpConEmu->ConsoleOffsetRect();
	RECT rcClient; GetClientRect(ghWnd, &rcClient);
	RECT rc = gpConEmu->CalcRect(CER_BACK, rcClient, CER_MAINCLIENT);
	mh_WndBack = CreateWindow(szClassNameBack, NULL, style,
	                          rc.left, rc.top,
	                          rcClient.right - rc.right - rc.left,
	                          rcClient.bottom - rc.bottom - rc.top,
	                          ghWnd, NULL, (HINSTANCE)g_hInstance, NULL);

	if (!mh_WndBack)
	{
		dwLastError = GetLastError();
		mh_WndBack = (HWND)-1; // чтобы родитель не ругался
		MBoxA(_T("Can't create background window!"));
		return NULL; //
	}

	// Прокрутка
	//style = SBS_RIGHTALIGN/*|WS_VISIBLE*/|SBS_VERT|WS_CHILD|WS_CLIPSIBLINGS;
	//mh_WndScroll = CreateWindowEx(0/*|WS_EX_LAYERED*/ /*WS_EX_TRANSPARENT*/, L"SCROLLBAR", NULL, style,
	//	rc.left, rc.top,
	//	rcClient.right - rc.right - rc.left,
	//	rcClient.bottom - rc.bottom - rc.top,
	//	ghWnd, NULL, (HINSTANCE)g_hInstance, NULL);
	style = WS_VSCROLL|WS_CHILD|WS_CLIPSIBLINGS;
	mn_ScrollWidth = GetSystemMetrics(SM_CXVSCROLL);
	mh_WndScroll = CreateWindowEx(0/*|WS_EX_LAYERED*/ /*WS_EX_TRANSPARENT*/, szClassNameScroll, NULL, style,
	                              rc.left - mn_ScrollWidth, rc.top,
	                              mn_ScrollWidth, rc.bottom - rc.top,
	                              ghWnd, NULL, (HINSTANCE)g_hInstance, NULL);

	if (!mh_WndScroll)
	{
		dwLastError = GetLastError();
		mh_WndScroll = (HWND)-1; // чтобы родитель не ругался
		MBoxA(_T("Can't create scrollbar window!"));
		return NULL; //
	}

	//GetWindowRect(mh_WndScroll, &rcClient);
	//mn_ScrollWidth = rcClient.right - rcClient.left;
	TODO("alpha-blended. похоже для WS_CHILD это не прокатит...");
	//BOOL lbRcLayered = SetLayeredWindowAttributes ( mh_WndScroll, 0, 100, LWA_ALPHA );
	//if (!lbRcLayered)
	//	dwLastError = GetLastError();
	//// Важно проверку делать после создания главного окна, иначе IsAppThemed будет возвращать FALSE
	//BOOL bAppThemed = FALSE, bThemeActive = FALSE;
	//FAppThemed pfnThemed = NULL;
	//mh_UxTheme = LoadLibrary ( L"UxTheme.dll" );
	//if (mh_UxTheme) {
	//	pfnThemed = (FAppThemed)GetProcAddress( mh_UxTheme, "IsAppThemed" );
	//	if (pfnThemed) bAppThemed = pfnThemed();
	//	pfnThemed = (FAppThemed)GetProcAddress( mh_UxTheme, "IsThemeActive" );
	//	if (pfnThemed) bThemeActive = pfnThemed();
	//}
	//if (!bAppThemed || !bThemeActive) {
	//	FreeLibrary(mh_UxTheme); mh_UxTheme = NULL;
	//} else {
	//	mfn_OpenThemeData = (FOpenThemeData)GetProcAddress( mh_UxTheme, "OpenThemeData" );
	//	mfn_OpenThemeDataEx = (FOpenThemeData)GetProcAddress( mh_UxTheme, "OpenThemeDataEx" );
	//	mfn_CloseThemeData = (FCloseThemeData)GetProcAddress( mh_UxTheme, "CloseThemeData" );
	//
	//	if (mfn_OpenThemeDataEx)
	//		mh_ThemeData = mfn_OpenThemeDataEx ( mh_WndScroll, L"Scrollbar", OTD_NONCLIENT );
	//}
	return mh_WndBack;
}

LRESULT CALLBACK CConEmuBack::BackWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	if (messg == WM_SYSCHAR)
		return TRUE;

	switch(messg)
	{
		case WM_CREATE:
			gpConEmu->m_Back->mh_WndBack = hWnd;
			break;
		case WM_DESTROY:
			//if (gpConEmu->m_Back->mh_ThemeData && gpConEmu->m_Back->mfn_CloseThemeData) {
			//	gpConEmu->m_Back->mfn_CloseThemeData ( gpConEmu->m_Back->mh_ThemeData );
			//	gpConEmu->m_Back->mh_ThemeData = NULL;
			//}
			//if (gpConEmu->m_Back->mh_UxTheme) {
			//	FreeLibrary(gpConEmu->m_Back->mh_UxTheme);
			//	gpConEmu->m_Back->mh_UxTheme = NULL;
			//}
			DeleteObject(gpConEmu->m_Back->mh_BackBrush);
			break;
		case WM_SETFOCUS:
			gpConEmu->setFocus(); // Фокус должен быть в главном окне!
			return 0;
		case WM_VSCROLL:
			//POSTMESSAGE(ghConWnd, messg, wParam, lParam, FALSE);
			// -- не должно вызываться вообще
			_ASSERTE(messg!=WM_VSCROLL);
			break;
		case WM_PAINT:
		{
			PAINTSTRUCT ps; memset(&ps, 0, sizeof(ps));
			HDC hDc = BeginPaint(hWnd, &ps);
#ifndef SKIP_ALL_FILLRECT

			if (!IsRectEmpty(&ps.rcPaint))
				FillRect(hDc, &ps.rcPaint, gpConEmu->m_Back->mh_BackBrush);

#endif
			EndPaint(hWnd, &ps);
		}
		return 0;
#ifdef _DEBUG
		case WM_SIZE:
		{
			UINT nW = LOWORD(lParam), nH = HIWORD(lParam);
			result = DefWindowProc(hWnd, messg, wParam, lParam);
		}
		return result;
		case WM_WINDOWPOSCHANGING:
		{
			WINDOWPOS* pwp = (WINDOWPOS*)lParam;
			result = DefWindowProc(hWnd, messg, wParam, lParam);
		}
		return result;
		case WM_WINDOWPOSCHANGED:
		{
			WINDOWPOS* pwp = (WINDOWPOS*)lParam;
			result = DefWindowProc(hWnd, messg, wParam, lParam);
		}
		return result;
#endif
	}

	result = DefWindowProc(hWnd, messg, wParam, lParam);
	return result;
}

LRESULT CALLBACK CConEmuBack::ScrollWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	if (messg == WM_SYSCHAR)
		return TRUE;

	switch(messg)
	{
		case WM_CREATE:
			gpConEmu->m_Back->mh_WndScroll = hWnd;
			gpConEmu->m_Back->m_TScrollShow.Init(hWnd, TIMER_SCROLL_SHOW, TIMER_SCROLL_SHOW_DELAY);
			gpConEmu->m_Back->m_TScrollHide.Init(hWnd, TIMER_SCROLL_HIDE, TIMER_SCROLL_HIDE_DELAY);
			#ifndef SKIP_HIDE_TIMER
			gpConEmu->m_Back->m_TScrollCheck.Init(hWnd, TIMER_SCROLL_CHECK, TIMER_SCROLL_CHECK_DELAY);
			#endif
			break;
		case WM_VSCROLL:

			//POSTMESSAGE(ghConWnd, messg, wParam, lParam, FALSE);
			if (LOWORD(wParam) == SB_THUMBTRACK)
				gpConEmu->m_Back->mb_VTracking = true;

			gpConEmu->ActiveCon()->RCon()->OnSetScrollPos(wParam);
			break;
		case WM_SETFOCUS:
			gpConEmu->setFocus(); // Фокус должен быть в главном окне!
			return 0;
		case WM_TIMER:

			switch(wParam)
			{
				#ifndef SKIP_HIDE_TIMER // Не будем прятать по таймеру - только по движению мышки
				case TIMER_SCROLL_CHECK:

					if (gpConEmu->m_Back->mb_Scroll2Visible)
					{
						if (!gpConEmu->m_Back->CheckMouseOverScroll())
						{
							gpConEmu->m_Back->HideScroll(FALSE/*abImmediate*/);
						}
					}

					break;
				#endif

				case TIMER_SCROLL_SHOW:

					if (gpConEmu->m_Back->CheckMouseOverScroll() || gpConEmu->m_Back->CheckScrollAutoPopup())
						gpConEmu->m_Back->ShowScroll(TRUE/*abImmediate*/);
					else
						gpConEmu->m_Back->mb_Scroll2Visible = FALSE;

					if (gpConEmu->m_Back->m_TScrollShow.IsStarted())
						gpConEmu->m_Back->m_TScrollShow.Stop();

					break;

				case TIMER_SCROLL_HIDE:

					if (!gpConEmu->m_Back->CheckMouseOverScroll())
						gpConEmu->m_Back->HideScroll(TRUE/*abImmediate*/);
					else
						gpConEmu->m_Back->mb_Scroll2Visible = TRUE;

					if (gpConEmu->m_Back->m_TScrollHide.IsStarted())
						gpConEmu->m_Back->m_TScrollHide.Stop();

					break;
			}

			break;
	}

	result = DefWindowProc(hWnd, messg, wParam, lParam);
	return result;
}

void CConEmuBack::Resize()
{
#if defined(EXT_GNUC_LOG)
	CVirtualConsole* pVCon = gpConEmu->ActiveCon();
#endif

	if (!mh_WndBack || !IsWindow(mh_WndBack))
	{
#if defined(EXT_GNUC_LOG)

		if (gpSet->isAdvLogging>1)
			pVCon->RCon()->LogString("  --  CConEmuBack::Resize() - exiting, mh_WndBack failed");

#endif
		return;
	}

	//RECT rc = gpConEmu->ConsoleOffsetRect();
	RECT rcClient; GetClientRect(ghWnd, &rcClient);
	bool bTabsShown = gpConEmu->mp_TabBar->IsShown();

	if (mb_LastTabVisible == bTabsShown && mb_LastAlwaysScroll == (gpSet->isAlwaysShowScrollbar == 1))
	{
		if (memcmp(&rcClient, &mrc_LastClient, sizeof(RECT))==0)
		{
#if defined(EXT_GNUC_LOG)
			char szDbg[255];
			wsprintfA(szDbg, "  --  CConEmuBack::Resize() - exiting, (%i,%i,%i,%i)==(%i,%i,%i,%i)",
			          rcClient.left, rcClient.top, rcClient.right, rcClient.bottom,
			          mrc_LastClient.left, mrc_LastClient.top, mrc_LastClient.right, mrc_LastClient.bottom);

			if (gpSet->isAdvLogging>1)
				pVCon->RCon()->LogString(szDbg);

#endif
			return; // ничего не менялось
		}
	}

	memmove(&mrc_LastClient, &rcClient, sizeof(RECT)); // сразу запомним
	mb_LastTabVisible = bTabsShown;
	mb_LastAlwaysScroll = (gpSet->isAlwaysShowScrollbar == 1);
	//RECT rcScroll; GetWindowRect(mh_WndScroll, &rcScroll);
	RECT rc = gpConEmu->CalcRect(CER_BACK, rcClient, CER_MAINCLIENT);
#ifdef _DEBUG
	RECT rcTest;
	GetClientRect(mh_WndBack, &rcTest);
#endif
#if defined(EXT_GNUC_LOG)
	char szDbg[255]; wsprintfA(szDbg, "  --  CConEmuBack::Resize() - X=%i, Y=%i, W=%i, H=%i", rc.left, rc.top, 	rc.right - rc.left,	rc.bottom - rc.top);

	if (gpSet->isAdvLogging>1)
		pVCon->RCon()->LogString(szDbg);

#endif
	// Это скрытое окно отрисовки. Оно должно соответствовать
	// размеру виртуальной консоли. На это окно ориентируются плагины!
	WARNING("DoubleView");
	MoveWindow(mh_WndBack,
	           rc.left, rc.top,
	           rc.right - rc.left,
	           rc.bottom - rc.top,
	           1);
	rc = gpConEmu->CalcRect(CER_SCROLL, rcClient, CER_MAINCLIENT);
#ifdef _DEBUG

	if (rc.bottom != rcClient.bottom || rc.right != rcClient.right)
	{
		_ASSERTE(rc.bottom == rcClient.bottom && rc.right == rcClient.right);
	}

#endif
	MoveWindow(mh_WndScroll,
	           rc.left, rc.top,
	           rc.right - rc.left,
	           rc.bottom - rc.top,
	           1);
	//MoveWindow(mh_WndScroll,
	//	rc.right - (rcScroll.right-rcScroll.left),
	//	rc.top,
	//	rcScroll.right-rcScroll.left,
	//	rc.bottom - rc.top,
	//	1);
#ifdef _DEBUG
	GetClientRect(mh_WndBack, &rcTest);
#endif
}

void CConEmuBack::Refresh()
{
	COLORREF* pcr = gpSet->GetColors(gpConEmu->isMeForeground());

	if (mn_LastColor == pcr[mn_ColorIdx])
		return;

	mn_LastColor = pcr[mn_ColorIdx];
	HBRUSH hNewBrush = CreateSolidBrush(mn_LastColor);
	SetClassLongPtr(mh_WndBack, GCLP_HBRBACKGROUND, (LONG_PTR)hNewBrush);
	DeleteObject(mh_BackBrush);
	mh_BackBrush = hNewBrush;
	//RECT rc; GetClientRect(mh_Wnd, &rc);
	//InvalidateRect(mh_Wnd, &rc, TRUE);
	Invalidate();
}

void CConEmuBack::RePaint()
{
	WARNING("mh_WndBack invisible, поэтому отрисовка смысла не имеет");

	if (mh_WndBack && mh_WndBack!=(HWND)-1)
	{
		Refresh();
		UpdateWindow(mh_WndBack);
		//if (mh_WndScroll) UpdateWindow(mh_WndScroll);
	}
}

void CConEmuBack::Invalidate()
{
	if (this && mh_WndBack)
	{
		InvalidateRect(mh_WndBack, NULL, FALSE);
		InvalidateRect(mh_WndScroll, NULL, FALSE);
	}
}

// Должна вернуть TRUE, если события мыши не нужно пропускать в консоль
BOOL CConEmuBack::TrackMouse()
{
	BOOL lbCapture = FALSE; // По умолчанию - мышь не перехватывать
	BOOL lbHided = FALSE;
	#ifdef _DEBUG
	CRealConsole* pRCon = gpConEmu->ActiveCon()->RCon();
	BOOL lbBufferMode = pRCon->isBufferHeight() && !pRCon->GuiWnd();
	#endif
	BOOL lbOverVScroll = CheckMouseOverScroll();

	if (lbOverVScroll || (gpSet->isAlwaysShowScrollbar == 1))
	{
		if (!mb_Scroll2Visible)
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
	else if (mb_Scroll2Visible)
	{
		_ASSERTE(gpSet->isAlwaysShowScrollbar != 1);
		mb_Scroll2Visible = FALSE;
		HideScroll(FALSE/*abImmediate*/); // Если gpSet->isAlwaysShowScrollbar==1 - сама разберется
	}

	lbCapture = (lbOverVScroll && mb_ScrollVisible);
	return lbCapture;
}

BOOL CConEmuBack::CheckMouseOverScroll()
{
	BOOL lbOverVScroll = FALSE;
	CVirtualConsole* pVCon = gpConEmu->ActiveCon();
	CRealConsole* pRCon = pVCon ? gpConEmu->ActiveCon()->RCon() : NULL;

	if (pRCon)
	{
		BOOL lbBufferMode = pRCon->isBufferHeight() && !pRCon->GuiWnd();

		if (lbBufferMode)
		{
			// Если прокрутку тащили мышкой и отпустили
			if (mb_VTracking && !isPressed(VK_LBUTTON))
			{
				// Сбросим флажок
				mb_VTracking = false;
			}

			// чтобы полоса не скрылась, когда ее тащат мышкой
			if (mb_VTracking)
			{
				lbOverVScroll = TRUE;
			}
			else // Теперь проверим, если мышь в над скроллбаром - показать его
			{
				POINT ptCur; RECT rcScroll;
				GetCursorPos(&ptCur);
				GetWindowRect(mh_WndScroll, &rcScroll);

				if (PtInRect(&rcScroll, ptCur))
				{
					// Если не проверять - не получится начать выделение с правого края окна
					//if (!gpSet->isSelectionModifierPressed())
					if (!(isPressed(VK_SHIFT) || isPressed(VK_CONTROL) || isPressed(VK_MENU) || isPressed(VK_LBUTTON)))
						lbOverVScroll = TRUE;
				}
			}
		}
	}

	return lbOverVScroll;
}

BOOL CConEmuBack::CheckScrollAutoPopup()
{
	return mb_ScrollAutoPopup;
}

void CConEmuBack::SetScroll(BOOL abEnabled, int anTop, int anVisible, int anHeight)
{
	int nCurPos = 0;
	//BOOL lbScrollRc = FALSE;
	SCROLLINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cbSize = sizeof(si);
	si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE; // | SIF_TRACKPOS;
	si.nMin = 0;

	if (!abEnabled)
	{
		si.nPos = 0;
		si.nPage = 1;
		si.nMax = 1;
	}
	else
	{
		si.nPos = anTop;
		si.nPage = anVisible - 1;
		si.nMax = anHeight;
	}

	//// Если режим "BufferHeight" включен - получить из консольного окна текущее состояние полосы прокрутки
	//if (con.bBufferHeight) {
	//    lbScrollRc = GetScrollInfo(hConWnd, SB_VERT, &si);
	//} else {
	//    // Сбросываем параметры так, чтобы полоса не отображалась (все на 0)
	//}
	//TODO("Нужно при необходимости 'всплыть' полосу прокрутки");
	nCurPos = SetScrollInfo(mh_WndScroll, SB_VERT, &si, true);

	if (!abEnabled)
	{
		mb_ScrollAutoPopup = FALSE;

		if ((gpSet->isAlwaysShowScrollbar == 1) && IsWindowEnabled(mh_WndScroll))
		{
			EnableScrollBar(mh_WndScroll, SB_VERT, ESB_DISABLE_BOTH);
			EnableWindow(mh_WndScroll, FALSE);
		}

		HideScroll((gpSet->isAlwaysShowScrollbar != 1));
	}
	else
	{
		if (!IsWindowEnabled(mh_WndScroll))
		{
			EnableWindow(mh_WndScroll, TRUE);
			EnableScrollBar(mh_WndScroll, SB_VERT, ESB_ENABLE_BOTH);
		}

		// Показать прокрутку, если например буфер скроллится с клавиатуры
		if ((si.nPos > 0) && (si.nPos < (si.nMax - (int)si.nPage - 1)) && gpSet->isAlwaysShowScrollbar)
		{
			mb_ScrollAutoPopup = (gpSet->isAlwaysShowScrollbar == 2);

			if (!mb_Scroll2Visible)
				ShowScroll((gpSet->isAlwaysShowScrollbar == 1));
		}
		else
		{
			mb_ScrollAutoPopup = FALSE;
		}
	}
}

void CConEmuBack::ShowScroll(BOOL abImmediate)
{
	bool bTShow = false, bTCheck = false;

	if (abImmediate || (gpSet->isAlwaysShowScrollbar == 1))
	{
		mb_ScrollVisible = TRUE; mb_Scroll2Visible = TRUE;

		if (!IsWindowVisible(mh_WndScroll))
			apiShowWindow(mh_WndScroll, SW_SHOWNOACTIVATE);

		SetWindowPos(mh_WndScroll, HWND_TOP, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE|SWP_SHOWWINDOW);

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

void CConEmuBack::HideScroll(BOOL abImmediate)
{
	bool bTHide = false;
	mb_ScrollAutoPopup = FALSE;

	if (gpSet->isAlwaysShowScrollbar == 1)
	{
		// Прокрутка всегда показывается! Скрывать нельзя!
	}
	else if (abImmediate)
	{
		mb_ScrollVisible = FALSE;
		mb_Scroll2Visible = FALSE;

		if (IsWindowVisible(mh_WndScroll))
			apiShowWindow(mh_WndScroll, SW_HIDE);

		RECT rcScroll; GetWindowRect(mh_WndScroll, &rcScroll);
		// вьюпорт невидимый, передернуть нужно основное окно
		MapWindowPoints(NULL, ghWnd, (LPPOINT)&rcScroll, 2);
		InvalidateRect(ghWnd, &rcScroll, FALSE);
	}
	else
	{
		mb_Scroll2Visible = FALSE;
		bTHide = true;
		//m_TScrollHide.Start();
	}

	#ifndef SKIP_HIDE_TIMER
	if (m_TScrollCheck.IsStarted())
		m_TScrollCheck.Stop();
	#endif

	//
	if (m_TScrollShow.IsStarted())
		m_TScrollShow.Stop();

	//
	if (bTHide)
		m_TScrollHide.Start();
	else if (m_TScrollHide.IsStarted())
		m_TScrollHide.Stop();
}
