#include "Header.h"

CConEmuChild::CConEmuChild()
{
}

CConEmuChild::~CConEmuChild()
{
}

LRESULT CALLBACK CConEmuChild::ChildWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;
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
    case WM_INPUTLANGCHANGE:
    case WM_INPUTLANGCHANGEREQUEST:
    case WM_IME_NOTIFY:
    case WM_VSCROLL:
        // ¬с€ обработка в родителе
        result = gConEmu.WndProc(hWnd, messg, wParam, lParam);
        return result;

    default:
        if (messg) result = DefWindowProc(hWnd, messg, wParam, lParam);
    }
    return result;
}

LRESULT CConEmuChild::OnPaint(WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
    //if (gbInPaint)
	//    break;

    if (gConEmu.isPictureView())
    {
		// TODO: если PictureView распахнуто не на все окно - отрисовать видимую часть консоли!
        result = DefWindowProc(ghWndDC, WM_PAINT, wParam, lParam);

	} else {
	    
		//gbInPaint = true;
		RECT client; GetClientRect(ghWndDC, &client);
		if (((ULONG)client.right)>pVCon->Width)
			client.right = pVCon->Width;
		if (((ULONG)client.bottom)>pVCon->Height)
			client.bottom = pVCon->Height;

		PAINTSTRUCT ps;
		HDC hDc = BeginPaint(ghWndDC, &ps);
		//HDC hDc = GetDC(hWnd);

		#ifndef _DEBUG
			// Release режим
			BitBlt(hDc, 0, 0, client.right, client.bottom, pVCon->hDC, 0, 0, SRCCOPY);
		#else
			if (!gbNoDblBuffer) {
				// ќбычный режим
				BitBlt(hDc, 0, 0, client.right, client.bottom, pVCon->hDC, 0, 0, SRCCOPY);
			} else {
				RECT rect;
				HBRUSH hBrush = CreateSolidBrush(gSet.Colors[0]);
				HBRUSH hOldBrush = (HBRUSH)SelectObject(hDc, hBrush);
				GetClientRect(ghWndDC, &rect);
				GdiSetBatchLimit(1); // отключить буферизацию вывода дл€ текущей нити
				FillRect(hDc, &rect, hBrush); // -- если захочетс€ на "чистую" рисовать
				SelectObject(hDc, hOldBrush);
				DeleteObject(hBrush);

				GdiFlush();
				// –исуем сразу на канвасе, без буферизации
				pVCon->Update(false, &hDc);
			}
		#endif

		EndPaint(ghWndDC, &ps);

		#ifdef _DEBUG
		if (gbNoDblBuffer) GdiSetBatchLimit(0); // вернуть стандартный режим
		#endif
	}

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
            //TODO: а ведь PictureView может и в QuickView активироватьс€...
            MoveWindow(gConEmu.hPictureView, 0,0,rcClient.right,rcClient.bottom, 1);
            INVALIDATE(); //InvalidateRect(hWnd, NULL, FALSE);
            //SetFocus(hPictureView); -- все равно на другой процесс фокус передать нельз€...
        }
    }

	return result;
}
