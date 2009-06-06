#include "Header.h"

CConEmuChild::CConEmuChild()
{
	mn_MsgTabChanged = RegisterWindowMessage(CONEMUTABCHANGED);
}

CConEmuChild::~CConEmuChild()
{
}

HWND CConEmuChild::Create()
{
	//WNDCLASS wc = {CS_OWNDC|CS_DBLCLKS/*|CS_SAVEBITS*/, CConEmuChild::ChildWndProc, 0, 0, 
	//		g_hInstance, NULL, LoadCursor(NULL, IDC_ARROW), 
	//		NULL /*(HBRUSH)COLOR_BACKGROUND*/, 
	//		NULL, szClassName};// | CS_DROPSHADOW
	//if (!RegisterClass(&wc)) {
	//	ghWndDC = (HWND)-1; // чтобы родитель не ругался
	//	MBoxA(_T("Can't register DC window class!"));
	//	return NULL;
	//}
	DWORD style = WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS /*| WS_CLIPCHILDREN | (gSet.Buffer Height ? WS_VSCROLL : 0)*/;
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

LRESULT CALLBACK CConEmuChild::ChildWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

	if (messg == WM_SYSCHAR)
		return TRUE;

    switch (messg)
    {
    case WM_COPYDATA:
		// если уж пришло сюда - передадим куда надо
		result = gConEmu.WndProc ( ghWnd, messg, wParam, lParam );
		break;

    case WM_ERASEBKGND:
		result = 0;
		break;
		
    case WM_PAINT:
		result = gConEmu.m_Child.OnPaint(wParam, lParam);
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
    case WM_KILLFOCUS:
    case WM_SETFOCUS:
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
    case WM_VSCROLL:
        // Вся обработка в родителе
        result = gConEmu.WndProc(hWnd, messg, wParam, lParam);
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
			DEBUGSTR(szMsg);

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
		if (messg == mn_MsgTabChanged) {
			if (gSet.isTabs) {
				//изменились табы, их нужно перечитать
				#ifdef MSGLOGGER
					WCHAR szDbg[128]; swprintf(szDbg, L"Tabs:Notified(%i)\n", wParam);
					DEBUGSTR(szDbg);
				#endif
				TODO("здесь хорошо бы вместо OnTimer реально обновить mn_TopProcessID")
				// иначе во время запуска PID фара еще может быть не известен...
				//gConEmu.OnTimer(0,0); не получилось. индекс конмана не менялся, из-за этого индекс активного фара так и остался 0
				TabBar.Retrieve();
			}
		}
        if (messg) result = DefWindowProc(hWnd, messg, wParam, lParam);
    }
    return result;
}

LRESULT CConEmuChild::OnPaint(WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	BOOL lbSkipDraw = FALSE;
    //if (gbInPaint)
	//    break;

	gSet.Performance(tPerfBlt, FALSE);

    if (gConEmu.isPictureView())
    {
		// если PictureView распахнуто не на все окно - отрисовать видимую часть консоли!
		RECT rcPic, rcClient;
		GetClientRect(gConEmu.hPictureView, &rcPic);
		GetClientRect(ghWndDC, &rcClient);
		
		if (rcPic.right>=rcClient.right) {
			lbSkipDraw = TRUE;
	        result = DefWindowProc(ghWndDC, WM_PAINT, wParam, lParam);
        }
	}
	if (!lbSkipDraw)
	{
		gConEmu.PaintCon();
	}

	gSet.Performance(tPerfBlt, TRUE);

	// Если открыто окно настроек - обновить системную информацию о размерах
	gConEmu.UpdateSizes();

    return result;
}

LRESULT CConEmuChild::OnSize(WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
    BOOL lbIsPicView = FALSE;

	RECT rcNewClient; GetClientRect(ghWndDC,&rcNewClient);
    
    if (gConEmu.isPictureView())
    {
        if (gConEmu.hPictureView) {
            lbIsPicView = TRUE;
            gConEmu.isPiewUpdate = true;
            RECT rcClient; GetClientRect(ghWndDC, &rcClient);
            //TODO: а ведь PictureView может и в QuickView активироваться...
            MoveWindow(gConEmu.hPictureView, 0,0,rcClient.right,rcClient.bottom, 1);
            //INVALIDATE(); //InvalidateRect(hWnd, NULL, FALSE);
			Invalidate();
            //SetFocus(hPictureView); -- все равно на другой процесс фокус передать нельзя...
        }
    }

	return result;
}

void CConEmuChild::Invalidate()
{
	if (ghWndDC)
		InvalidateRect(ghWndDC, NULL, FALSE);
}









WARNING("!!! На время скроллирования необходимо установить AutoScroll в TRUE, а при отпускании ползунка - вернуть старое значение!");
TODO("И вообще, скроллинг нужно передавать через pipe");

CConEmuBack::CConEmuBack()
{
	mh_WndBack = NULL;
	mh_WndScroll = NULL;
	mh_BackBrush = NULL;
	mn_LastColor = -1;
#ifdef _DEBUG
	mn_ColorIdx = 1;
#else
	mn_ColorIdx = 0;
#endif
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
	DWORD style = WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS ;
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
	style = SBS_RIGHTALIGN/*|WS_VISIBLE*/|SBS_VERT|WS_CHILD|WS_CLIPSIBLINGS;
	mh_WndScroll = CreateWindowEx(0/*|WS_EX_LAYERED*/ /*WS_EX_TRANSPARENT*/, L"SCROLLBAR", NULL, style,
		rc.left, rc.top,
		rcClient.right - rc.right - rc.left,
		rcClient.bottom - rc.bottom - rc.top,
		ghWnd, NULL, (HINSTANCE)g_hInstance, NULL);
	if (!mh_WndScroll) {
		dwLastError = GetLastError();
		mh_WndScroll = (HWND)-1; // чтобы родитель не ругался
		MBoxA(_T("Can't create scrollbar window!"));
		return NULL; //
	}
	TODO("alpha-blended. похоже для WS_CHILD это не прокатит...");
	//BOOL lbRcLayered = SetLayeredWindowAttributes ( mh_WndScroll, 0, 100, LWA_ALPHA );
	//if (!lbRcLayered)
	//	dwLastError = GetLastError();

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
			DeleteObject(gConEmu.m_Back.mh_BackBrush);
			break;
		//case WM_SETFOCUS:
		//
		//	if (messg == WM_SETFOCUS) {
		//		if (ghWndDC && IsWindow(ghWndDC))
		//			SetFocus(ghWndDC);
		//	}
		//	return 0;
	    case WM_VSCROLL:
	        POSTMESSAGE(ghConWnd, messg, wParam, lParam, FALSE);
	        break;
		case WM_PAINT:
			{
				PAINTSTRUCT ps; memset(&ps, 0, sizeof(ps));
				HDC hDc = BeginPaint(hWnd, &ps);
				if (!IsRectEmpty(&ps.rcPaint))
					FillRect(hDc, &ps.rcPaint, gConEmu.m_Back.mh_BackBrush);
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

void CConEmuBack::Resize()
{
	if (!mh_WndBack || !IsWindow(mh_WndBack)) 
		return;

	//RECT rc = gConEmu.ConsoleOffsetRect();
	RECT rcClient; GetClientRect(ghWnd, &rcClient);
	RECT rcScroll; GetWindowRect(mh_WndScroll, &rcScroll);

	RECT rc = gConEmu.CalcRect(CER_BACK, rcClient, CER_MAINCLIENT);

	#ifdef _DEBUG
	GetClientRect(mh_WndBack, &rcClient);
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

	SetClassLong(mh_WndBack, GCL_HBRBACKGROUND, (LONG)hNewBrush);
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
