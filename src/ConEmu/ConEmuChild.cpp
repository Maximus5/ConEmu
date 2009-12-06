
#include "Header.h"

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
}

CConEmuChild::~CConEmuChild()
{
}

HWND CConEmuChild::Create()
{
	// Имя класса - то же самое, что и у главного окна
	DWORD style = /*WS_VISIBLE |*/ WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	//RECT rc = gConEmu.DCClientRect();
	RECT rcMain; GetClientRect(ghWnd, &rcMain);
	RECT rc = gConEmu.CalcRect(CER_DC, rcMain, CER_MAINCLIENT);
	ghWndDC = CreateWindow(szClassName, 0, style, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, ghWnd, NULL, (HINSTANCE)g_hInstance, NULL);
	if (!ghWndDC) {
		ghWndDC = (HWND)-1; // чтобы родитель не ругался
		MBoxA(_T("Can't create DC window!"));
		return NULL; //
	}
	//SetClassLong(ghWndDC, GCL_HBRBACKGROUND, (LONG)gConEmu.m_Back.mh_BackBrush);
	SetWindowPos(ghWndDC, HWND_TOP, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
	//gConEmu.dcWindowLast = rc; //TODO!!!
	
	return ghWndDC;
}

LRESULT CConEmuChild::ChildWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

	if (messg == WM_SYSCHAR)
		return TRUE;

    switch (messg)
    {
	case WM_SETFOCUS:
		SetFocus(ghWnd); // Фокус должен быть в главном окне!
		return 0;

    case WM_ERASEBKGND:
		result = 0;
		break;
		
    case WM_PAINT:
		result = gConEmu.m_Child.OnPaint();
		break;

    case WM_SIZE:
		result = gConEmu.m_Child.OnSize(wParam, lParam);
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
			result = gConEmu.WndProc(hWnd, messg, wParam, lParam);
		}
        return result;

	case WM_IME_NOTIFY:
		break;
    case WM_INPUTLANGCHANGE:
    case WM_INPUTLANGCHANGEREQUEST:
	{
		#ifdef _DEBUG
		if (IsDebuggerPresent()) {
			WCHAR szMsg[128];
			wsprintf(szMsg, L"InChild %s(CP:%i, HKL:0x%08X)\n",
				(messg == WM_INPUTLANGCHANGE) ? L"WM_INPUTLANGCHANGE" : L"WM_INPUTLANGCHANGEREQUEST",
				wParam, lParam);
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
			return result;
#endif

    default:
		// Сообщение приходит из ConEmuPlugin
		if (messg == gConEmu.m_Child.mn_MsgTabChanged) {
			if (gSet.isTabs) {
				//изменились табы, их нужно перечитать
				#ifdef MSGLOGGER
					WCHAR szDbg[128]; swprintf(szDbg, L"Tabs:Notified(%i)\n", wParam);
					DEBUGSTRTABS(szDbg);
				#endif
				TODO("здесь хорошо бы вместо OnTimer реально обновить mn_TopProcessID")
				// иначе во время запуска PID фара еще может быть не известен...
				//gConEmu.OnTimer(0,0); не получилось. индекс конмана не менялся, из-за этого индекс активного фара так и остался 0
				WARNING("TabBar.Retrieve() ничего уже не делает вообще");
				_ASSERT(FALSE);
				TabBar.Retrieve();
			}
		} else if (messg == gConEmu.m_Child.mn_MsgPostFullPaint) {
			gConEmu.m_Child.Redraw();		
		} else if (messg) {
			result = DefWindowProc(hWnd, messg, wParam, lParam);
		}
    }
    return result;
}

LRESULT CConEmuChild::OnPaint()
{
	LRESULT result = 0;
	BOOL lbSkipDraw = FALSE;
    //if (gbInPaint)
	//    break;

	_ASSERT(FALSE);

	//2009-09-28 может так (autotabs)
	if (mb_DisableRedraw)
		return 0;
	
	mb_PostFullPaint = FALSE;

	if (gSet.isAdvLogging>1) 
		gConEmu.ActiveCon()->RCon()->LogString("CConEmuChild::OnPaint");

	gSet.Performance(tPerfBlt, FALSE);

    if (gConEmu.isPictureView())
    {
		// если PictureView распахнуто не на все окно - отрисовать видимую часть консоли!
		RECT rcPic, rcClient, rcCommon;

		GetWindowRect(gConEmu.hPictureView, &rcPic);
		GetClientRect(ghWnd, &rcClient); // Нам нужен ПОЛНЫЙ размер но ПОД тулбаром.
		MapWindowPoints(ghWnd, NULL, (LPPOINT)&rcClient, 2);
		#ifdef _DEBUG
		BOOL lbIntersect =
		#endif
		IntersectRect(&rcCommon, &rcClient, &rcPic);
		
		// Убрать из отрисовки прямоугольник PictureView
		MapWindowPoints(NULL, ghWndDC, (LPPOINT)&rcPic, 2);
		ValidateRect(ghWndDC, &rcPic);

		GetClientRect(gConEmu.hPictureView, &rcPic);
		GetClientRect(ghWndDC, &rcClient);
		
		if (rcPic.right>=rcClient.right) {
			lbSkipDraw = TRUE;
	        result = DefWindowProc(ghWndDC, WM_PAINT, 0, 0);
        }
	}
	if (!lbSkipDraw)
	{
		//gConEmu.PaintCon();
	}

	Validate();

	gSet.Performance(tPerfBlt, TRUE);

	// Если открыто окно настроек - обновить системную информацию о размерах
	gConEmu.UpdateSizes();

    return result;
}

LRESULT CConEmuChild::OnSize(WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	#ifdef _DEBUG
    BOOL lbIsPicView = FALSE;

	RECT rcNewClient; GetClientRect(ghWndDC,&rcNewClient);
	#endif

    // Вроде это и не нужно. Ни для Ansi ни для Unicode версии плагина
    // Все равно в ConEmu запрещен ресайз во время видимости окошка PictureView 
    
    //if (gConEmu.isPictureView())
    //{
    //    if (gConEmu.hPictureView) {
    //        lbIsPicView = TRUE;
    //        gConEmu.isPiewUpdate = true;
    //        RECT rcClient; GetClientRect(ghWndDC, &rcClient);
    //        //TODO: а ведь PictureView может и в QuickView активироваться...
    //        MoveWindow(gConEmu.hPictureView, 0,0,rcClient.right,rcClient.bottom, 1);
    //        //INVALIDATE(); //InvalidateRect(hWnd, NULL, FALSE);
	//		Invalidate();
    //        //SetFocus(hPictureView); -- все равно на другой процесс фокус передать нельзя...
    //    }
    //}

	return result;
}

void CConEmuChild::CheckPostRedraw()
{
	// Если был "Отмененный" Redraw, но 
	if (mb_IsPendingRedraw && mn_LastPostRedrawTick && ((GetTickCount() - mn_LastPostRedrawTick) >= CON_REDRAW_TIMOUT)) {
		mb_IsPendingRedraw = FALSE;
		_ASSERTE(gConEmu.isMainThread());

		Redraw();
	}
}

void CConEmuChild::Redraw()
{
	if (!gConEmu.isMainThread()) {
		if (mb_RedrawPosted ||
			(mn_LastPostRedrawTick && ((GetTickCount() - mn_LastPostRedrawTick) < CON_REDRAW_TIMOUT)))
		{
			mb_IsPendingRedraw = TRUE;
			return; // Блокируем СЛИШКОМ частые обновления
		} else {
			mn_LastPostRedrawTick = GetTickCount();
			mb_IsPendingRedraw = FALSE;
		}
		mb_RedrawPosted = TRUE; // чтобы не было кумулятивного эффекта
		PostMessage(ghWndDC, mn_MsgPostFullPaint, 0, 0);
		return;
	}

	if (mb_DisableRedraw) {
		DEBUGSTRDRAW(L" +++ RedrawWindow on DC window will be ignored!\n");
		return;
	} else {
		DEBUGSTRDRAW(L" +++ RedrawWindow on DC window called\n");
	}
	RECT rcClient; GetClientRect(ghWndDC, &rcClient);
	MapWindowPoints(ghWndDC, ghWnd, (LPPOINT)&rcClient, 2);
	InvalidateRect(ghWnd, &rcClient, FALSE);
	gConEmu.OnPaint(0,0);
	//#ifdef _DEBUG
	//BOOL lbRc =
	//#endif
	//RedrawWindow(ghWnd, NULL, NULL,
	//	RDW_INTERNALPAINT|RDW_NOERASE|RDW_UPDATENOW);

	mb_RedrawPosted = FALSE; // Чтобы другие нити могли сделать еще пост
}

void CConEmuChild::SetRedraw(BOOL abRedrawEnabled)
{
	mb_DisableRedraw = !abRedrawEnabled;
}

void CConEmuChild::Invalidate()
{
	//2009-06-22 Опять поперла непрорисовка. Причем если по экрану окошко повозить - изображение нормальное
	// Так что пока лучше два раза нарисуем...
	//if (mb_Invalidated) {
	//	DEBUGSTRDRAW(L" ### Warning! Invalidate on DC window will be duplicated\n");
	////	return;
	//}
	if (ghWndDC) {
		DEBUGSTRDRAW(L" +++ Invalidate on DC window called\n");
		//mb_Invalidated = TRUE;
		RECT rcClient; GetClientRect(ghWndDC, &rcClient);
		//InvalidateRect(ghWndDC, &rcClient, FALSE);
		MapWindowPoints(ghWndDC, ghWnd, (LPPOINT)&rcClient, 2);
		InvalidateRect(ghWnd, &rcClient, FALSE);

		/*if (!mb_Invalidated) {
			mb_Invalidated = TRUE;
			PostMessage(ghWndDC, WM_PAINT, 0,0);
		}*/
	}
}

void CConEmuChild::Validate()
{
	//mb_Invalidated = FALSE;
	//DEBUGSTRDRAW(L" +++ Validate on DC window called\n");
	//if (ghWndDC) ValidateRect(ghWnd, NULL);
}

void CConEmuChild::SetCaret ( int Visible, UINT X, UINT Y, UINT nWidth, UINT nHeight )
{
	if (Visible == -1) {
		if (Caret.bCreated) {
			DestroyCaret();
			Caret.bCreated = FALSE;
			Caret.bVisible = FALSE;
		}
		return;
	}
	
	if (Caret.bCreated && (nWidth != Caret.nWidth || nHeight != Caret.nHeight)) {
		DestroyCaret();
		Caret.bCreated = FALSE;
		Caret.bVisible = FALSE;
	}
	if (!Caret.bCreated) {
		//If second parameter is (HBITMAP)1 - the caret dotted
		Caret.bCreated = CreateCaret ( ghWndDC, (HBITMAP)0, nWidth, nHeight );
		Caret.nWidth = nWidth; Caret.nHeight = nHeight;
	}
	
	if (Visible == 0 && Caret.bVisible) {
		ShowCaret ( ghWndDC );
		Caret.bVisible = FALSE;
	}

	if (Caret.X != X || Caret.Y != Y) {	
		SetCaretPos ( X, Y );
		Caret.X = X; Caret.Y = Y;
	}
	
	if (Visible == 1 && !Caret.bVisible) {
		ShowCaret ( ghWndDC );
		Caret.bVisible = TRUE;
	}
}








WARNING("!!! На время скроллирования необходимо установить AutoScroll в TRUE, а при отпускании ползунка - вернуть старое значение!");
TODO("И вообще, скроллинг нужно передавать через pipe");

CConEmuBack::CConEmuBack()
{
	mh_WndBack = NULL;
	mh_WndScroll = NULL;
	mh_BackBrush = NULL;
	mn_LastColor = -1;
	mn_ScrollWidth = 0;
	mb_ScrollVisible = FALSE;
	memset(&mrc_LastClient, 0, sizeof(mrc_LastClient));
	mb_LastTabVisible = false;
#ifdef _DEBUG
	mn_ColorIdx = 1;
#else
	mn_ColorIdx = 0;
#endif
	//mh_UxTheme = NULL; mh_ThemeData = NULL; mfn_OpenThemeData = NULL; mfn_CloseThemeData = NULL;
}

CConEmuBack::~CConEmuBack()
{
}

HWND CConEmuBack::Create()
{
	mn_LastColor = gSet.Colors[mn_ColorIdx];
	mh_BackBrush = CreateSolidBrush(mn_LastColor);

	DWORD dwLastError = 0;
	WNDCLASS wc = {CS_OWNDC/*|CS_SAVEBITS*/, CConEmuBack::BackWndProc, 0, 0, 
			g_hInstance, NULL, LoadCursor(NULL, IDC_ARROW), 
			mh_BackBrush, 
			NULL, szClassNameBack};
	if (!RegisterClass(&wc)) {
		dwLastError = GetLastError();
		mh_WndBack = (HWND)-1; // чтобы родитель не ругался
		mh_WndScroll = (HWND)-1;
		MBoxA(_T("Can't register background window class!"));
		return NULL;
	}
	// Scroller
	wc.lpfnWndProc = CConEmuBack::ScrollWndProc;
	wc.lpszClassName = szClassNameScroll;
	if (!RegisterClass(&wc)) {
		dwLastError = GetLastError();
		mh_WndBack = (HWND)-1; // чтобы родитель не ругался
		mh_WndScroll = (HWND)-1;
		MBoxA(_T("Can't register scroller window class!"));
		return NULL;
	}

	DWORD style = /*WS_VISIBLE |*/ WS_CHILD | WS_CLIPSIBLINGS ;
	//RECT rc = gConEmu.ConsoleOffsetRect();
	RECT rcClient; GetClientRect(ghWnd, &rcClient);
	RECT rc = gConEmu.CalcRect(CER_BACK, rcClient, CER_MAINCLIENT);

	mh_WndBack = CreateWindow(szClassNameBack, NULL, style, 
		rc.left, rc.top,
		rcClient.right - rc.right - rc.left,
		rcClient.bottom - rc.bottom - rc.top,
		ghWnd, NULL, (HINSTANCE)g_hInstance, NULL);
	if (!mh_WndBack) {
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
	if (!mh_WndScroll) {
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

	switch (messg) {
		case WM_CREATE:
			gConEmu.m_Back.mh_WndBack = hWnd;
			break;
		case WM_DESTROY:
			//if (gConEmu.m_Back.mh_ThemeData && gConEmu.m_Back.mfn_CloseThemeData) {
			//	gConEmu.m_Back.mfn_CloseThemeData ( gConEmu.m_Back.mh_ThemeData );
			//	gConEmu.m_Back.mh_ThemeData = NULL;
			//}
			//if (gConEmu.m_Back.mh_UxTheme) {
			//	FreeLibrary(gConEmu.m_Back.mh_UxTheme);
			//	gConEmu.m_Back.mh_UxTheme = NULL;
			//}
			DeleteObject(gConEmu.m_Back.mh_BackBrush);
			break;
		case WM_SETFOCUS:
			SetFocus(ghWnd); // Фокус должен быть в главном окне!
			return 0;
	    case WM_VSCROLL:
	        POSTMESSAGE(ghConWnd, messg, wParam, lParam, FALSE);
	        break;
		case WM_PAINT:
			{
				PAINTSTRUCT ps; memset(&ps, 0, sizeof(ps));
				HDC hDc = BeginPaint(hWnd, &ps);
				#ifndef SKIP_ALL_FILLRECT
				if (!IsRectEmpty(&ps.rcPaint))
					FillRect(hDc, &ps.rcPaint, gConEmu.m_Back.mh_BackBrush);
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

	switch (messg) {
		case WM_CREATE:
			gConEmu.m_Back.mh_WndScroll = hWnd;
			break;
		case WM_VSCROLL:
			WARNING("Переделать в команду пайпа");
			POSTMESSAGE(ghConWnd, messg, wParam, lParam, FALSE);
			break;
		case WM_SETFOCUS:
			SetFocus(ghWnd); // Фокус должен быть в главном окне!
			return 0;
	}

	result = DefWindowProc(hWnd, messg, wParam, lParam);

	return result;
}

void CConEmuBack::Resize()
{
	#if defined(EXT_GNUC_LOG)
	CVirtualConsole* pVCon = gConEmu.ActiveCon();
	#endif
	
	if (!mh_WndBack || !IsWindow(mh_WndBack)) {
    	#if defined(EXT_GNUC_LOG)
    	if (gSet.isAdvLogging>1) 
        	pVCon->RCon()->LogString("  --  CConEmuBack::Resize() - exiting, mh_WndBack failed");
        #endif
		return;
	}

	//RECT rc = gConEmu.ConsoleOffsetRect();
	RECT rcClient; GetClientRect(ghWnd, &rcClient);

	bool bTabsShown = TabBar.IsShown();
	if (mb_LastTabVisible == bTabsShown) {
		if (memcmp(&rcClient, &mrc_LastClient, sizeof(RECT))==0) {
        	#if defined(EXT_GNUC_LOG)
        	char szDbg[255];
        	wsprintfA(szDbg, "  --  CConEmuBack::Resize() - exiting, (%i,%i,%i,%i)==(%i,%i,%i,%i)",
        		rcClient.left, rcClient.top, rcClient.right, rcClient.bottom,
        		mrc_LastClient.left, mrc_LastClient.top, mrc_LastClient.right, mrc_LastClient.bottom);
        	if (gSet.isAdvLogging>1) 
            	pVCon->RCon()->LogString(szDbg);
            #endif
			return; // ничего не менялось
		}
	}
	memmove(&mrc_LastClient, &rcClient, sizeof(RECT)); // сразу запомним
	mb_LastTabVisible = bTabsShown;

	RECT rcScroll; GetWindowRect(mh_WndScroll, &rcScroll);

	RECT rc = gConEmu.CalcRect(CER_BACK, rcClient, CER_MAINCLIENT);

	#ifdef _DEBUG
	GetClientRect(mh_WndBack, &rcClient);
	#endif

	#if defined(EXT_GNUC_LOG)
	char szDbg[255]; wsprintfA(szDbg, "  --  CConEmuBack::Resize() - X=%i, Y=%i, W=%i, H=%i", rc.left, rc.top, 	rc.right - rc.left,	rc.bottom - rc.top );
	if (gSet.isAdvLogging>1) 
    	pVCon->RCon()->LogString(szDbg);
    #endif
    
	MoveWindow(mh_WndBack, 
		rc.left, rc.top,
		rc.right - rc.left,
		rc.bottom - rc.top,
		1);
	MoveWindow(mh_WndScroll, 
		rc.right - (rcScroll.right-rcScroll.left),
		rc.top,
		rcScroll.right-rcScroll.left,
		rc.bottom - rc.top,
		1);

	#ifdef _DEBUG
	GetClientRect(mh_WndBack, &rcClient);
	#endif
}

void CConEmuBack::Refresh()
{
	if (mn_LastColor == gSet.Colors[mn_ColorIdx])
		return;

	mn_LastColor = gSet.Colors[mn_ColorIdx];
	HBRUSH hNewBrush = CreateSolidBrush(mn_LastColor);

	SetClassLongPtr(mh_WndBack, GCLP_HBRBACKGROUND, (LONG)hNewBrush);
	DeleteObject(mh_BackBrush);
	mh_BackBrush = hNewBrush;

	//RECT rc; GetClientRect(mh_Wnd, &rc);
	//InvalidateRect(mh_Wnd, &rc, TRUE);
	Invalidate();
}

void CConEmuBack::RePaint()
{
	if (mh_WndBack && mh_WndBack!=(HWND)-1)
	{
		Refresh();
		UpdateWindow(mh_WndBack);
		//if (mh_WndScroll) UpdateWindow(mh_WndScroll);
	}
}

void CConEmuBack::Invalidate()
{
	if (this && mh_WndBack) {
		InvalidateRect(mh_WndBack, NULL, FALSE);
		InvalidateRect(mh_WndScroll, NULL, FALSE);
	}
}

BOOL CConEmuBack::TrackMouse()
{
	BOOL lbRc = FALSE; // По умолчанию - мышь не перехватывать
	BOOL lbHided = FALSE;
	BOOL lbBufferMode = gConEmu.ActiveCon()->RCon()->isBufferHeight();
	if (/*!mb_ScrollVisible &&*/ lbBufferMode) {
		// Если мышь в над скроллбаром - показать его
		POINT ptCur; RECT rcScroll;
		GetCursorPos(&ptCur);
		GetWindowRect(mh_WndScroll, &rcScroll);
		if (PtInRect(&rcScroll, ptCur)) {
			if (!mb_ScrollVisible) {
				mb_ScrollVisible = TRUE;
				ShowWindow(mh_WndScroll, SW_SHOWNOACTIVATE);
				SetWindowPos(mh_WndScroll, HWND_TOP, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE|SWP_SHOWWINDOW);
			}
			lbRc = TRUE;
		} else if (mb_ScrollVisible) {
			mb_ScrollVisible = FALSE;
			ShowWindow(mh_WndScroll, SW_HIDE);
			lbHided = TRUE;
		}
	} else if (mb_ScrollVisible && !lbBufferMode) {
		mb_ScrollVisible = FALSE;
		ShowWindow(mh_WndScroll, SW_HIDE);
		lbHided = TRUE;
	}
	
	if (lbHided)
	{
		RECT rcScroll; GetWindowRect(mh_WndScroll, &rcScroll);
		if (IsWindowVisible(ghWndDC)) {
			MapWindowPoints(NULL, ghWndDC, (LPPOINT)&rcScroll, 2);
			InvalidateRect(ghWndDC, &rcScroll, FALSE);
		} else { // задел на будущее, когда viewport станет невидимым
			MapWindowPoints(NULL, ghWnd, (LPPOINT)&rcScroll, 2);
			InvalidateRect(ghWnd, &rcScroll, FALSE);
		}
	}
	return lbRc;
}
