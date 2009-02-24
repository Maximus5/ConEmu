#include "Header.h"

#define CES_CONMANACTIVE 0x01
#define CES_TELNETACTIVE 0x02
#define CES_FARACTIVE 0x04
#define CES_FILEPANEL 0x10
#define CES_TEMPPANEL 0x20
#define CES_PLUGINPANEL 0x40
#define CES_EDITOR 0x100
#define CES_VIEWER 0x200
#define CES_COPYING 0x400
#define CES_MOVING 0x800
//... and so on

CConEmuMain::CConEmuMain()
{
    wcscpy(szConEmuVersion, L"?.?.?.?");
	gnLastProcessCount=0;
	cBlinkNext=0;
	WindowMode=0;
	//hPipe=NULL;
	//hPipeEvent=NULL;
	mb_InSizing = false;
	isWndNotFSMaximized=false;
	isShowConsole=false;
	mb_FullWindowDrag=false;
	isLBDown=false;
	isDragProcessed=false;
	isRBDown=false;
	ibSkilRDblClk=false; 
	dwRBDownTick=0;
	isPiewUpdate = false; //true; --Maximus5
	gbPostUpdateWindowSize = false;
	hPictureView = NULL; 
	bPicViewSlideShow = false; 
	dwLastSlideShowTick = 0;
	gb_ConsoleSelectMode=false;
	setParent = false; setParent2 = false;
	RBDownNewX=0; RBDownNewY=0;
	cursor.x=0; cursor.y=0; Rcursor=cursor;
	lastMMW=-1;
	lastMML=-1;
	DragDrop=NULL;
	ProgressBars=NULL;
	cBlinkShift=0;
	Title[0]=0; TitleCmp[0]=0;
	//mb_InClose = FALSE;
	memset(m_ProcList, 0, 1000*sizeof(DWORD)); m_ProcCount=0;
	mn_TopProcessID = 0; ms_TopProcess[0] = 0; mb_FarActive = FALSE;
	mn_ActiveStatus = 0;

	mh_Psapi = NULL;
	GetModuleFileNameEx= NULL;

#ifdef _UNICODE
	MultiByteToWideChar(CP_ACP, 0, "редактирование ", -1, ms_EditorRus, 16);
	MultiByteToWideChar(CP_ACP, 0, "просмотр ", -1, ms_ViewerRus, 16);
#else
	strcpy(ms_EditorRus, "редактирование ");
	strcpy(ms_ViewerRus, "просмотр ");
#endif
}

BOOL CConEmuMain::Init()
{
	mh_Psapi = LoadLibrary(_T("psapi.dll"));
	if (mh_Psapi) {
		GetModuleFileNameEx = (FGetModuleFileNameEx)GetProcAddress(mh_Psapi, "GetModuleFileNameExW");
		if (GetModuleFileNameEx)
			return TRUE;
	}

	DWORD dwErr = GetLastError();
	TCHAR szErr[255];
	wsprintf(szErr, _T("Can't initialize psapi!\r\nLastError = 0x%08x"), dwErr);
	MBoxA(szErr);
	return FALSE;
}

void CConEmuMain::Destroy()
{
	if (ghWnd)
		DestroyWindow(ghWnd);
}

CConEmuMain::~CConEmuMain()
{
}




/* ****************************************************** */
/*                                                        */
/*                  Sizing methods                        */
/*                                                        */
/* ****************************************************** */

#ifdef colorer_func
	{
	Sizing_Methods() {};
	}
#endif

/*!!!static!!*/
void CConEmuMain::AddMargins(RECT& rc, RECT& rcAddShift, BOOL abExpand/*=FALSE*/)
{
	if (!abExpand)
	{
		if (rcAddShift.left)
			rc.left += rcAddShift.left;
		if (rcAddShift.top)
			rc.top += rcAddShift.top;
		if (rcAddShift.right)
			rc.right -= rcAddShift.right;
		if (rcAddShift.bottom)
			rc.bottom -= rcAddShift.bottom;
	} else {
		if (rcAddShift.right || rcAddShift.left)
			rc.right += rcAddShift.right + rcAddShift.left;
		if (rcAddShift.bottom || rcAddShift.top)
			rc.bottom += rcAddShift.bottom + rcAddShift.top;
	}
}


/*!!!static!!*/
// Функция расчитывает 
RECT CConEmuMain::CalcMargins(enum ConEmuMargins mg)
{
	RECT rc;
	
	switch (mg)
	{
		case CEM_FRAME: // Разница между размером всего окна и клиентской области окна (рамка + заголовок)
		{
			RECT cRect, wRect;
			if (!ghWnd) {
				rc.left = rc.right = GetSystemMetrics(SM_CXSIZEFRAME);
				rc.bottom = GetSystemMetrics(SM_CYSIZEFRAME);
				rc.top = rc.bottom + GetSystemMetrics(SM_CYCAPTION);
			} else {
			    GetClientRect(ghWnd, &cRect); // The left and top members are zero. The right and bottom members contain the width and height of the window.
			    MapWindowPoints(ghWnd, NULL, (LPPOINT)&cRect, 2);
			    GetWindowRect(ghWnd, &wRect); // screen coordinates of the upper-left and lower-right corners of the window
			    rc.top = cRect.top - wRect.top;          // >0
			    rc.left = cRect.left - wRect.left;       // >0
			    rc.bottom = wRect.bottom - cRect.bottom; // >0
			    rc.right = wRect.right - cRect.right;    // >0
			}
		}	break;
		// Далее все отступы считаются в клиентской части (дочерние окна)!
		case CEM_TAB:   // Отступы от краев таба (если он видим) до окна фона (с прокруткой)
		{
			if (ghWnd) {
				// Главное окно уже создано, наличие таба определено
				if (TabBar.IsActive()) { //TODO: + IsAllowed()?
					rc = TabBar.GetMargins();
				} else {
					rc = MakeRect(0,0); // раз таба нет - значит дополнительные отступы не нужны
				}
			} else {
				// Иначе нужно смотреть по настройкам
				if (gSet.isTabs == 1) {
					rc = gSet.rcTabMargins; // умолчательные отступы таба
					if (!gSet.isTabFrame) {
						// От таба остается только заголовок (закладки)
						rc.left=0; rc.right=0; rc.bottom=0;
					}
				} else {
					rc = MakeRect(0,0); // раз таба нет - значит дополнительные отступы не нужны
				}
			}
		}	break;
		case CEM_BACK:  // Отступы от краев окна фона (с прокруткой) до окна с отрисовкой (DC)
		{
			if (gSet.BufferHeight) { //TODO: а показывается ли сейчас прокрутка?
				rc = MakeRect(0, GetSystemMetrics(SM_CXVSCROLL));
			} else {
				rc = MakeRect(0,0); // раз прокрутки нет - значит дополнительные отступы не нужны
			}
		}	break;
	}
	
	return rc;
}

/*!!!static!!*/
// Для приблизительного расчета размеров - нужен только (размер главного окна)|(размер консоли)
// Для точного расчета размеров - требуется (размер главного окна) и (размер окна отрисовки) для корректировки
RECT CConEmuMain::CalcRect(enum ConEmuRect tWhat, RECT rFrom, enum ConEmuRect tFrom, RECT* prDC/*=NULL*/)
{
    RECT rc = rFrom; // инициализация, если уж не получится...
    RECT rcShift = MakeRect(0,0);
    
	switch (tFrom)
	{
		case CER_MAIN:
			// Нужно отныть отступы рамки и заголовка!
			{
				rcShift = CalcMargins(CEM_FRAME);
				rc.right = (rFrom.right-rFrom.left) - (rcShift.left+rcShift.right);
				rc.bottom = (rFrom.bottom-rFrom.top) - (rcShift.top+rcShift.bottom);
				rc.left = 0;
				rc.top = 0; // Получили клиентскую область
			}
			break;
		case CER_MAINCLIENT:
			{
				MBoxAssert(!(rFrom.left || rFrom.top));
			}
			break;
		case CER_DC:
			{   // Размер окна отрисовки в пикселах!
				MBoxAssert(!(rFrom.left || rFrom.top));
				
				switch (tWhat)
				{
					case CER_MAIN:
					{
						rcShift = CalcMargins(CEM_BACK);
						AddMargins(rc, rcShift, TRUE/*abExpand*/);
						rcShift = CalcMargins(CEM_TAB);
						AddMargins(rc, rcShift, TRUE/*abExpand*/);
						rcShift = CalcMargins(CEM_FRAME);
						AddMargins(rc, rcShift, TRUE/*abExpand*/);
					} break;
					case CER_MAINCLIENT:
					{
						rcShift = CalcMargins(CEM_BACK);
						AddMargins(rc, rcShift, TRUE/*abExpand*/);
						rcShift = CalcMargins(CEM_TAB);
						AddMargins(rc, rcShift, TRUE/*abExpand*/);
					} break;
					case CER_TAB:
					{
						rcShift = CalcMargins(CEM_BACK);
						AddMargins(rc, rcShift, TRUE/*abExpand*/);
						rcShift = CalcMargins(CEM_TAB);
						AddMargins(rc, rcShift, TRUE/*abExpand*/);
					} break;
					case CER_BACK:
					{
						rcShift = CalcMargins(CEM_BACK);
						AddMargins(rc, rcShift, TRUE/*abExpand*/);
					} break;
					default:
						MBoxAssert(FALSE); // Недопустимо
						return rc;
				}
			}
			return rc;
		case CER_CONSOLE:
			{   // Размер консоли в символах!
				MBoxAssert(!(rFrom.left || rFrom.top));
				MBoxAssert(tWhat!=CER_CONSOLE);
				
			    if (gSet.LogFont.lfWidth==0) {
				    MBoxAssert(pVCon!=NULL);
				    pVCon->InitDC(FALSE); // инициализировать ширину шрифта по умолчанию
				}
				
				// ЭТО размер окна отрисовки DC
				rc = MakeRect(rFrom.right * gSet.LogFont.lfWidth, rFrom.bottom * gSet.LogFont.lfHeight);
				
				if (tWhat != CER_DC)
					rc = CalcRect(tWhat, rc, CER_DC);
			}
			return rc;
		default:
			MBoxAssert(FALSE); // Недопустимо
			return rc;
	};

	RECT rcAddShift = MakeRect(0,0);
		
	if (prDC) {
		// Если передали реальный размер окна отрисовки - нужно посчитать дополнительные сдвиги
		RECT rcCalcDC = CalcRect(CER_DC, rFrom, CER_MAINCLIENT, NULL /*prDC*/);
		// расчетный НЕ ДОЛЖЕН быть меньше переданного
		#ifdef _DEBUG
		_ASSERTE((rcCalcDC.right - rcCalcDC.left)>=(prDC->right - prDC->left));
		_ASSERTE((rcCalcDC.bottom - rcCalcDC.top)>=(prDC->bottom - prDC->top));
		#endif
		// считаем доп.сдвиги. ТОЧНО
		if ((rcCalcDC.right - rcCalcDC.left)!=(prDC->right - prDC->left))
		{
			rcAddShift.left = (rcCalcDC.right - rcCalcDC.left - (prDC->right - prDC->left))/2;
			rcAddShift.right = rcCalcDC.right - rcCalcDC.left - rcAddShift.left;
		}
		if ((rcCalcDC.bottom - rcCalcDC.top)!=(prDC->bottom - prDC->top))
		{
			rcAddShift.top = (rcCalcDC.bottom - rcCalcDC.top - (prDC->bottom - prDC->top))/2;
			rcAddShift.bottom = rcCalcDC.bottom - rcCalcDC.top - rcAddShift.top;
		}
	}
	
	// если мы дошли сюда - значит tFrom==CER_MAINCLIENT
	
	switch (tWhat)
	{
		case CER_TAB:
		{
			// Отступы ДО таба могут появиться только от корректировки
		} break;
		case CER_BACK:
		{
			rcShift = CalcMargins(CEM_TAB);
			AddMargins(rc, rcShift);
		} break;
		case CER_DC:
		case CER_CONSOLE:
		{
			rcShift = CalcMargins(CEM_TAB);
			AddMargins(rc, rcShift);
			rcShift = CalcMargins(CEM_BACK);
			AddMargins(rc, rcShift);
			
			// Если нужен размер консоли в символах сразу делим и выходим
			if (tWhat == CER_CONSOLE) {
				MBoxAssert((gSet.LogFont.lfWidth!=0) && (gSet.LogFont.lfHeight!=0));
			    if (gSet.LogFont.lfWidth==0 || gSet.LogFont.lfHeight==0)
				    pVCon->InitDC(FALSE); // инициализировать ширину шрифта по умолчанию
				
			    rc.right = (rc.right - rc.left) / gSet.LogFont.lfWidth;
			    rc.bottom = (rc.bottom - rc.top) / gSet.LogFont.lfHeight;
			    rc.left = 0; rc.top = 0;
			    
			    return rc;
			}
		} break;
		default:
			MBoxAssert(FALSE); // Недопустимо
			return rc;
	}
	
	AddMargins(rc, rcAddShift);
	
	return rc; // Посчитали, возвращаем
}

/*!!!static!!*/
// Получить размер (правый нижний угол) окна по его клиентской области и наоборот
RECT CConEmuMain::MapRect(RECT rFrom, BOOL bFrame2Client)
{
	RECT rcShift = CalcMargins(CEM_FRAME);
	if (bFrame2Client) {
		//rFrom.left -= rcShift.left;
		//rFrom.top -= rcShift.top;
		rFrom.right -= (rcShift.right+rcShift.left);
		rFrom.bottom -= (rcShift.bottom+rcShift.top);
	} else {
		//rFrom.left += rcShift.left;
		//rFrom.top += rcShift.top;
		rFrom.right += (rcShift.right+rcShift.left);
		rFrom.bottom += (rcShift.bottom+rcShift.top);
	}
	return rFrom;
}

//// returns difference between window size and client area size of inWnd in outShift->x, outShift->y
//void CConEmuMain::GetCWShift(HWND inWnd, POINT *outShift)
//{
//    RECT cRect;
//    
//    GetCWShift ( inWnd, &cRect );
//    
//    outShift->x = cRect.right  - cRect.left;
//    outShift->y = cRect.bottom - cRect.top;
//}
//
//// returns margins between window frame and client area
//void CConEmuMain::GetCWShift(HWND inWnd, RECT *outShift)
//{
//    RECT cRect, wRect;
//    GetClientRect(inWnd, &cRect); // The left and top members are zero. The right and bottom members contain the width and height of the window.
//    MapWindowPoints(inWnd, NULL, (LPPOINT)&cRect, 2);
//    GetWindowRect(inWnd, &wRect); // screen coordinates of the upper-left and lower-right corners of the window
//    outShift->top = wRect.top - cRect.top;          // <0
//    outShift->left = wRect.left - cRect.left;       // <0
//    outShift->bottom = wRect.bottom - cRect.bottom; // >0
//    outShift->right = wRect.right - cRect.right;    // >0
//}

//// Вернуть отступы со всех сторон от краев клиентской части главного окна до краев окна отрисовки
//RECT CConEmuMain::ConsoleOffsetRect()
//{
//    RECT rect; memset(&rect, 0, sizeof(rect));
//
//	if (TabBar.IsActive())
//		rect = TabBar.GetMargins();
//
//	/*rect.top = TabBar.IsActive()?TabBar.Height():0;
//    rect.left = 0;
//    rect.bottom = 0;
//    rect.right = 0;*/
//
//	return rect;
//}

//// Положение дочернего окна отрисовки
//RECT CConEmuMain::DCClientRect(RECT* pClient/*=NULL*/)
//{
//    RECT rect;
//	if (pClient)
//		rect = *pClient;
//	else
//		GetClientRect(ghWnd, &rect);
//	if (TabBar.IsActive()) {
//		RECT mr = TabBar.GetMargins();
//		//rect.top += TabBar.Height();
//		rect.top += mr.top;
//		rect.left += mr.left;
//		rect.right -= mr.right;
//		rect.bottom -= mr.bottom;
//	}
//
//	if (pClient)
//		*pClient = rect;
//    return rect;
//}

// Установить размер основного окна по текущему размеру pVCon
void CConEmuMain::SyncWindowToConsole()
{
    DEBUGLOGFILE("SyncWindowToConsole\n");

	RECT rcDC = MakeRect(pVCon->Width, pVCon->Height);

	RECT rcWnd = CalcRect(CER_MAIN, rcDC, CER_DC); // размеры окна
    
    //GetCWShift(ghWnd, &cwShift);
    
    RECT wndR; GetWindowRect(ghWnd, &wndR); // текущий XY

	MOVEWINDOW ( ghWnd, wndR.left, wndR.top, rcWnd.right, rcWnd.bottom, 1);
    
    //RECT consoleRect = ConsoleOffsetRect();

    //#ifdef MSGLOGGER
    //    char szDbg[100]; wsprintfA(szDbg, "   pVCon:Size={%i,%i}\n", pVCon->Width,pVCon->Height);
    //    DEBUGLOGFILE(szDbg);
    //#endif
    //
    //MOVEWINDOW ( ghWnd, wndR.left, wndR.top, pVCon->Width + cwShift.x + consoleRect.left + consoleRect.right, pVCon->Height + cwShift.y + consoleRect.top + consoleRect.bottom, 1);
}

//// returns console size in columns and lines calculated from current window size
//// rectInWindow - если true - с рамкой, false - только клиент
//COORD CConEmuMain::ConsoleSizeFromWindow(RECT* arect /*= NULL*/, bool frameIncluded /*= false*/, bool alreadyClient /*= false*/)
//{
//    COORD size;
//
//	if (!gSet.LogFont.lfWidth || !gSet.LogFont.lfHeight) {
//		MBoxAssert(FALSE);
//		// размер шрифта еще не инициализирован! вернем текущий размер консоли! TODO:
//		CONSOLE_SCREEN_BUFFER_INFO inf; memset(&inf, 0, sizeof(inf));
//		GetConsoleScreenBufferInfo(pVCon->hConOut(), &inf);
//		size = inf.dwSize;
//		return size; 
//	}
//
//    RECT rect, consoleRect;
//    if (arect == NULL)
//    {
//		frameIncluded = false;
//        GetClientRect(ghWnd, &rect);
//	    consoleRect = ConsoleOffsetRect();
//    } 
//    else
//    {
//        rect = *arect;
//		if (alreadyClient)
//			memset(&consoleRect, 0, sizeof(consoleRect));
//		else
//			consoleRect = ConsoleOffsetRect();
//    }
//    
//    size.X = (rect.right - rect.left - (frameIncluded ? cwShift.x : 0) - consoleRect.left - consoleRect.right)
//		/ gSet.LogFont.lfWidth;
//    size.Y = (rect.bottom - rect.top - (frameIncluded ? cwShift.y : 0) - consoleRect.top - consoleRect.bottom)
//		/ gSet.LogFont.lfHeight;
//    #ifdef MSGLOGGER
//        char szDbg[100]; wsprintfA(szDbg, "   ConsoleSizeFromWindow={%i,%i}\n", size.X, size.Y);
//        DEBUGLOGFILE(szDbg);
//    #endif
//    return size;
//}

//// return window size in pixels calculated from console size
//RECT CConEmuMain::WindowSizeFromConsole(COORD consoleSize, bool rectInWindow /*= false*/, bool clientOnly /*= false*/)
//{
//    RECT rect;
//    rect.top = 0;   
//    rect.left = 0;
//    RECT offsetRect;
//	if (clientOnly)
//		memset(&offsetRect, 0, sizeof(RECT));
//	else
//		offsetRect = ConsoleOffsetRect();
//    rect.bottom = consoleSize.Y * gSet.LogFont.lfHeight + (rectInWindow ? cwShift.y : 0) + offsetRect.top + offsetRect.bottom;
//    rect.right = consoleSize.X * gSet.LogFont.lfWidth + (rectInWindow ? cwShift.x : 0) + offsetRect.left + offsetRect.right;
//    #ifdef MSGLOGGER
//        char szDbg[100]; wsprintfA(szDbg, "   WindowSizeFromConsole={%i,%i}\n", rect.right,rect.bottom);
//        DEBUGLOGFILE(szDbg);
//    #endif
//    return rect;
//}

// size in columns and lines
void CConEmuMain::SetConsoleWindowSize(const COORD& size, bool updateInfo)
{
    #ifdef MSGLOGGER
        static COORD lastSize1;
        if (lastSize1.Y>size.Y)
            lastSize1.Y=size.Y; //DEBUG
        lastSize1 = size;
        char szDbg[100]; wsprintfA(szDbg, "SetConsoleWindowSize({%i,%i},%i)\n", size.X, size.Y, updateInfo);
        DEBUGLOGFILE(szDbg);
    #endif

    if (isPictureView()) {
        isPiewUpdate = true;
        return;
    }

    // update size info
    if (updateInfo && !gSet.isFullScreen && !IsZoomed(ghWnd) && !IsIconic(ghWnd))
    {
		gSet.UpdateSize(size.X, size.Y);
    }

    // case: simple mode
    if (gSet.BufferHeight == 0)
    {
        MOVEWINDOW(ghConWnd, 0, 0, 1, 1, 1);
        SETCONSOLESCREENBUFFERSIZE(pVCon->hConOut(), size);
        MOVEWINDOW(ghConWnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 1);
        return;
    }

    // global flag of the first call which is:
    // *) after getting all the settings
    // *) before running the command
    static bool s_isFirstCall = true;

    // case: buffer mode: change buffer
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(pVCon->hConOut(), &csbi))
        return;
    csbi.dwSize.X = size.X;
    if (s_isFirstCall)
    {
        // first call: buffer height = from settings
        s_isFirstCall = false;
        csbi.dwSize.Y = max(gSet.BufferHeight, size.Y);
    }
    else
    {
        if (csbi.dwSize.Y == csbi.srWindow.Bottom - csbi.srWindow.Top + 1)
            // no y-scroll: buffer height = new window height
            csbi.dwSize.Y = size.Y;
        else
            // y-scroll: buffer height = old buffer height
            csbi.dwSize.Y = max(csbi.dwSize.Y, size.Y);
    }
    MOVEWINDOW(ghConWnd, 0, 0, 1, 1, 1);
    SETCONSOLESCREENBUFFERSIZE(pVCon->hConOut(), csbi.dwSize);
    
    // set console window
    if (!GetConsoleScreenBufferInfo(pVCon->hConOut(), &csbi))
        return;
    SMALL_RECT rect;
    rect.Top = csbi.srWindow.Top;
    rect.Left = csbi.srWindow.Left;
    rect.Right = rect.Left + size.X - 1;
    rect.Bottom = rect.Top + size.Y - 1;
    if (rect.Right >= csbi.dwSize.X)
    {
        int shift = csbi.dwSize.X - 1 - rect.Right;
        rect.Left += shift;
        rect.Right += shift;
    }
    if (rect.Bottom >= csbi.dwSize.Y)
    {
        int shift = csbi.dwSize.Y - 1 - rect.Bottom;
        rect.Top += shift;
        rect.Bottom += shift;
    }
    SetConsoleWindowInfo(pVCon->hConOut(), TRUE, &rect);
}

// Изменить размер консоли по размеру окна (главного)
void CConEmuMain::SyncConsoleToWindow()
{
	DEBUGLOGFILE("SyncConsoleToWindow\n");

	RECT rcClient; GetClientRect(ghWnd, &rcClient);

	// Посчитать нужный размер консоли
	RECT newCon = CalcRect(CER_CONSOLE, rcClient, CER_MAINCLIENT);

	// Посчитать нужный размер консоли
	//COORD newConSize = ConsoleSizeFromWindow();
	// Получить текущий размер консольного окна
    CONSOLE_SCREEN_BUFFER_INFO inf; memset(&inf, 0, sizeof(inf));
    GetConsoleScreenBufferInfo(pVCon->hConOut(), &inf);

	// Если нужно менять - ...
	if (newCon.right != (inf.srWindow.Right-inf.srWindow.Left+1) ||
		newCon.bottom != (inf.srWindow.Bottom-inf.srWindow.Top+1))
	{
		SetConsoleWindowSize(MakeCoord(newCon.right,newCon.bottom), true);
		if (pVCon)
			pVCon->InitDC();
	}
}

bool CConEmuMain::SetWindowMode(uint inMode)
{
	//WindowPlacement -- использовать нельзя, т.к. он работает в координатах Workspace, а не Screen!
	RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);
	RECT consoleSize = MakeRect(gSet.wndWidth, gSet.wndHeight);

    switch(inMode)
    {
    case rNormal:
		{
			DEBUGLOGFILE("SetWindowMode(rNormal)\n");

			if (IsIconic(ghWnd) || IsZoomed(ghWnd))
				ShowWindow(ghWnd, SW_SHOWNORMAL); // WM_SYSCOMMAND использовать не хочется...

			RECT rcNew = CalcRect(CER_MAIN, consoleSize, CER_CONSOLE);
			int nWidth = rcNew.right-rcNew.left;
			int nHeight = rcNew.bottom-rcNew.top;
			rcNew.left+=gSet.wndX; rcNew.top+=gSet.wndY;
			rcNew.right+=gSet.wndX; rcNew.bottom+=gSet.wndY;

			HMONITOR hMonitor = MonitorFromRect(&rcNew, MONITOR_DEFAULTTONEAREST);
			MONITORINFO mi; mi.cbSize = sizeof(mi);
			if (GetMonitorInfo(hMonitor, &mi)) {
				if (rcNew.left<mi.rcWork.left) rcNew.left=mi.rcWork.left;
				if (rcNew.top<mi.rcWork.top) rcNew.top=mi.rcWork.top;
				if ((rcNew.left+nWidth)>mi.rcWork.right) {
					rcNew.left = max(mi.rcWork.left,(mi.rcWork.right-nWidth));
					nWidth = min(nWidth, (mi.rcWork.right-rcNew.left));
				}
				if ((rcNew.top+nHeight)>mi.rcWork.bottom) {
					rcNew.top = max(mi.rcWork.top,(mi.rcWork.bottom-nHeight));
					nHeight = min(nHeight, (mi.rcWork.bottom-rcNew.top));
				}
			} else {
				if (rcNew.left<0) rcNew.left=0;
				if (rcNew.top<0) rcNew.top=0;
			}

			SetWindowPos(ghWnd, NULL, 
				rcNew.left, rcNew.top, nWidth, nHeight,
				SWP_NOZORDER);

            if (ghOpWnd)
				CheckRadioButton(gSet.hMain, rNormal, rFullScreen, rNormal);
			gSet.isFullScreen = false;

			if (!IsWindowVisible(ghWnd))
				ShowWindow(ghWnd, SW_SHOWNORMAL);
		} break;
    case rMaximized:
        {
	        DEBUGLOGFILE("SetWindowMode(rMaximized)\n");

			// Обновить коордианты в gSet, если требуется
			if (!gSet.isFullScreen && !IsZoomed(ghWnd) && !IsIconic(ghWnd))
				gSet.UpdatePos(rcWnd.left, rcWnd.top);

			if (!IsZoomed(ghWnd))
				ShowWindow(ghWnd, SW_SHOWMAXIMIZED);

            if (ghOpWnd)
				CheckRadioButton(gSet.hMain, rNormal, rFullScreen, rMaximized);
            gSet.isFullScreen = false;

			if (!IsWindowVisible(ghWnd))
				ShowWindow(ghWnd, SW_SHOWMAXIMIZED);
        } break;

    case rFullScreen:
        DEBUGLOGFILE("SetWindowMode(rFullScreen)\n");
        if (!gSet.isFullScreen)
        {
			// Обновить коордианты в gSet, если требуется
			if (!gSet.isFullScreen && !IsZoomed(ghWnd) && !IsIconic(ghWnd))
				gSet.UpdatePos(rcWnd.left, rcWnd.top);

            gSet.isFullScreen = true;
            isWndNotFSMaximized = IsZoomed(ghWnd);
            
            RECT rcShift = CalcMargins(CEM_FRAME);
            //GetCWShift(ghWnd, &rcShift); // Обновить, на всякий случай

			// Умолчания
			ptFullScreenSize.x = GetSystemMetrics(SM_CXSCREEN)+rcShift.left+rcShift.right;
			ptFullScreenSize.y = GetSystemMetrics(SM_CYSCREEN)+rcShift.top+rcShift.bottom;
			// которые нужно уточнить для текущего монитора!
			MONITORINFO mi; memset(&mi, 0, sizeof(mi)); mi.cbSize = sizeof(mi);
			HMONITOR hMon = MonitorFromWindow(ghWnd, MONITOR_DEFAULTTONEAREST);
			if (hMon) {
				if (GetMonitorInfo(hMon, &mi)) {
					ptFullScreenSize.x = (mi.rcMonitor.right-mi.rcMonitor.left)+rcShift.left+rcShift.right;
					ptFullScreenSize.y = (mi.rcMonitor.bottom-mi.rcMonitor.top)+rcShift.top+rcShift.bottom;
				}
			}

			if (IsIconic(ghWnd) || IsZoomed(ghWnd))
				ShowWindow(ghWnd, SW_SHOWNORMAL);

			// for virtual screend mi.rcMonitor. may contains negative values...

			SetWindowPos(ghWnd, NULL,
				-rcShift.left+mi.rcMonitor.left,-rcShift.top+mi.rcMonitor.top,ptFullScreenSize.x,ptFullScreenSize.y,
				SWP_NOZORDER);

            if (ghOpWnd)
				CheckRadioButton(gSet.hMain, rNormal, rFullScreen, rFullScreen);
        }
		if (!IsWindowVisible(ghWnd))
			ShowWindow(ghWnd, SW_SHOWNORMAL);
        break;
    }

    bool canEditWindowSizes = inMode == rNormal;
	if (ghOpWnd)
	{
		EnableWindow(GetDlgItem(ghOpWnd, tWndWidth), canEditWindowSizes);
		EnableWindow(GetDlgItem(ghOpWnd, tWndHeight), canEditWindowSizes);
	}
    SyncConsoleToWindow();
    return true;
}

void CConEmuMain::ForceShowTabs(BOOL abShow)
{
	if (!pVCon)
		return;

	BOOL lbTabsAllowed = abShow && TabBar.IsAllowed();

    if (abShow && !TabBar.IsShown() && gSet.isTabs && lbTabsAllowed)
    {
        TabBar.Activate();
		ConEmuTab tab; memset(&tab, 0, sizeof(tab));
		tab.Pos=0;
		tab.Current=1;
		tab.Type = 1;
		TabBar.Update(&tab, 1);
		gbPostUpdateWindowSize = true;
	} else if (!abShow) {
		TabBar.Deactivate();
		gbPostUpdateWindowSize = true;
	}

	if (gbPostUpdateWindowSize) { // значит мы что-то поменяли
		ReSize();
        /*RECT rcNewCon; GetClientRect(ghWnd, &rcNewCon);
		DCClientRect(&rcNewCon);
        MoveWindow(ghWndDC, rcNewCon.left, rcNewCon.top, rcNewCon.right - rcNewCon.left, rcNewCon.bottom - rcNewCon.top, 0);
        dcWindowLast = rcNewCon;
		
	    if (gSet.LogFont.lfWidth)
	    {
	        SyncConsoleToWindow();
		}*/
    }
}

void CConEmuMain::CheckBufferSize()
{
    CONSOLE_SCREEN_BUFFER_INFO inf; memset(&inf, 0, sizeof(inf));
    GetConsoleScreenBufferInfo(pVCon->hConOut(), &inf);
    if (inf.dwSize.X>(inf.srWindow.Right-inf.srWindow.Left+1)) {
        DEBUGLOGFILE("Wrong screen buffer width\n");
		// Окошко консоли почему-то схлопнулось по горизонтали
        MOVEWINDOW(ghConWnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 1);
    } else if ((gSet.BufferHeight == 0) && (inf.dwSize.Y>(inf.srWindow.Bottom-inf.srWindow.Top+1))) {
        DEBUGLOGFILE("Wrong screen buffer height\n");
		// Окошко консоли почему-то схлопнулось по вертикали
        MOVEWINDOW(ghConWnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 1);
    }

	// При выходе из FAR -> CMD с BufferHeight - смена QuickEdit режима
	DWORD mode = 0;
	BOOL lb = FALSE;
	if (gSet.BufferHeight) {
		lb = GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode);

		if (inf.dwSize.Y>(inf.srWindow.Bottom-inf.srWindow.Top+1)) {
			// Буфер больше высоты окна
			mode |= ENABLE_QUICK_EDIT_MODE|ENABLE_INSERT_MODE|ENABLE_EXTENDED_FLAGS;
		} else {
			// Буфер равен высоте окна (значит ФАР запустился)
			mode &= ~(ENABLE_QUICK_EDIT_MODE|ENABLE_INSERT_MODE);
			mode |= ENABLE_EXTENDED_FLAGS;
		}

		lb = SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);
	}
}

void CConEmuMain::ReSize()
{
	if (IsIconic(ghWnd))
		return;

	RECT client; GetClientRect(ghWnd, &client);

	OnSize(IsZoomed(ghWnd) ? SIZE_MAXIMIZED : SIZE_RESTORED,
		client.right, client.bottom);
}

LRESULT CConEmuMain::OnSize(WPARAM wParam, WORD newClientWidth, WORD newClientHeight)
{
	LRESULT result = 0;

	if (TabBar.IsActive())
        TabBar.UpdateWidth();
        
	m_Back.Resize();

	if (!gConEmu.mb_InSizing || gConEmu.mb_FullWindowDrag) {
		BOOL lbIsPicView = isPictureView();

		SyncConsoleToWindow();
	    
		RECT mainClient = MakeRect(newClientWidth,newClientHeight);
		//RECT dcSize; GetClientRect(ghWndDC, &dcSize);
		RECT dcSize = CalcRect(CER_DC, mainClient, CER_MAINCLIENT);
		//DCClientRect(&client);
		RECT client = CalcRect(CER_DC, mainClient, CER_MAINCLIENT, &dcSize);

		RECT rcNewCon; memset(&rcNewCon,0,sizeof(rcNewCon));
		if (pVCon && pVCon->Width && pVCon->Height) {
			if (gSet.isTryToCenter && (IsZoomed(ghWnd) || gSet.isFullScreen)) {
				rcNewCon.left = (client.right+client.left-(int)pVCon->Width)/2;
				rcNewCon.top = (client.bottom+client.top-(int)pVCon->Height)/2;
			}

			if (rcNewCon.left<client.left) rcNewCon.left=client.left;
			if (rcNewCon.top<client.top) rcNewCon.top=client.top;

			rcNewCon.right = rcNewCon.left + pVCon->Width;
				if (rcNewCon.right>client.right) rcNewCon.right=client.right;
			rcNewCon.bottom = rcNewCon.top + pVCon->Height;
				if (rcNewCon.bottom>client.bottom) rcNewCon.bottom=client.bottom;
		} else {
			rcNewCon = client;
		}
		MoveWindow(ghWndDC, rcNewCon.left, rcNewCon.top, rcNewCon.right - rcNewCon.left, rcNewCon.bottom - rcNewCon.top, 1);

		dcWindowLast = rcNewCon;
	}

	return result;
}

LRESULT CConEmuMain::OnSizing(WPARAM wParam, LPARAM lParam)
{
	LRESULT result = true;

	if (!gSet.isFullScreen && !IsZoomed(ghWnd)) {
		RECT srctWindow;
		RECT wndSizeRect, restrictRect;
		RECT *pRect = (RECT*)lParam; // с рамкой

		wndSizeRect = *pRect;
		// Для красивости рамки под мышкой
		if (gSet.LogFont.lfWidth && gSet.LogFont.lfHeight) {
			wndSizeRect.right += (gSet.LogFont.lfWidth-1)/2;
			wndSizeRect.bottom += (gSet.LogFont.lfHeight-1)/2;
		}

		// Рассчитать желаемый размер консоли
		//srctWindow = ConsoleSizeFromWindow(&wndSizeRect, true /* frameIncluded */);
		srctWindow = CalcRect(CER_CONSOLE, wndSizeRect, CER_MAIN);


		// Минимально допустимые размеры консоли
		if (srctWindow.right<28) srctWindow.right=28;
		if (srctWindow.bottom<9)  srctWindow.bottom=9;

		/*if ((srctWindowLast.X != srctWindow.X 
			|| srctWindowLast.Y != srctWindow.Y) 
			&& !mb_FullWindowDrag)
		{
			SetConsoleWindowSize(srctWindow, true);
			srctWindowLast = srctWindow;
		}*/

		//RECT consoleRect = ConsoleOffsetRect();
		//wndSizeRect = WindowSizeFromConsole(srctWindow, true /* rectInWindow */);
		wndSizeRect = CalcRect(CER_MAIN, srctWindow, CER_CONSOLE);

		restrictRect.right = pRect->left + wndSizeRect.right;
		restrictRect.bottom = pRect->top + wndSizeRect.bottom;
		restrictRect.left = pRect->right - wndSizeRect.right;
		restrictRect.top = pRect->bottom - wndSizeRect.bottom;
	    

		switch(wParam)
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

	return result;
}

void CConEmuMain::SetConParent()
{
    // set parent window of the console window:
    // *) it is used by ConMan and some FAR plugins, set it for standard mode or if /SetParent switch is set
    // *) do not set it by default for buffer mode because it causes unwanted selection jumps
    // WARP ItSelf опытным путем выяснил, что SetParent валит ConEmu в Windows7
    //if (!setParentDisabled && (setParent || gSet.BufferHeight == 0))
    
    //TODO: ConMan? попробуем на родительское окно SetParent делать
    if (setParent)
        SetParent(ghConWnd, setParent2 ? ghWnd : ghWndDC);
}








/* ****************************************************** */
/*                                                        */
/*                  System Routines                       */
/*                                                        */
/* ****************************************************** */

#ifdef colorer_func
	{
	System_Routines() {};
	}
#endif

DWORD CConEmuMain::CheckProcesses()
{
	// Высота буфера могла измениться после смены списка процессов
	BOOL  lbProcessChanged = FALSE;
	int   nTopIdx = 0;
    m_ProcCount = GetConsoleProcessList(m_ProcList,1000);
    if (m_ProcCount>=2) {
	    if (m_ProcList[0]==GetCurrentProcessId())
		    nTopIdx++;
    }
	if (m_ProcCount && (!gnLastProcessCount || m_ProcCount!=gnLastProcessCount))
		lbProcessChanged = TRUE;
	else if (m_ProcCount && m_ProcList[nTopIdx]!=mn_TopProcessID)
		lbProcessChanged = TRUE;
	gnLastProcessCount = m_ProcCount;
	
	if (lbProcessChanged) {
		if (m_ProcList[nTopIdx]==mn_TopProcessID) {
			// не менялось
			mb_FarActive = _tcscmp(ms_TopProcess, _T("far.exe"))==0;
		} else
		if (m_ProcList[nTopIdx]!=GetCurrentProcessId())
		{
			// Получить информацию о верхнем процессе
			DWORD dwErr = 0;
			HANDLE hProcess = OpenProcess(PROCESS_VM_READ|PROCESS_QUERY_INFORMATION, FALSE, m_ProcList[nTopIdx]);
			if (!hProcess)
				dwErr = GetLastError();
			else
			{
				TCHAR szFilePath[MAX_PATH+1];
				if (!GetModuleFileNameEx(hProcess, 0, szFilePath, MAX_PATH))
					dwErr = GetLastError();
				else
				{
					TCHAR* pszSlash = _tcsrchr(szFilePath, _T('\\'));
					if (pszSlash) pszSlash++; else pszSlash=szFilePath;
					int nLen = _tcslen(pszSlash);
					if (nLen>MAX_PATH) pszSlash[MAX_PATH]=0;
					_tcscpy(ms_TopProcess, pszSlash);
					_tcslwr(ms_TopProcess);
					mb_FarActive = _tcscmp(ms_TopProcess, _T("far.exe"))==0;
					mn_TopProcessID = m_ProcList[nTopIdx];
				}
				CloseHandle(hProcess); hProcess = NULL;
			}

		}
		TabBar.Refresh(mb_FarActive);
    }

	return m_ProcCount;
}

BOOL CConEmuMain::HandlerRoutine(DWORD dwCtrlType)
{
    return (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT ? true : false);
}

// Нужно вызывать после загрузки настроек!
void CConEmuMain::LoadIcons()
{
    if (hClassIcon)
	    return; // Уже загружены
	    
    if (GetModuleFileName(0, szIconPath, MAX_PATH))
    {
        this->LoadVersionInfo(szIconPath);
        
	    TCHAR *lpszExt = _tcsrchr(szIconPath, _T('.'));
	    if (!lpszExt)
		    szIconPath[0] = 0;
		else {
			_tcscpy(lpszExt, _T(".ico"));
	        DWORD dwAttr = GetFileAttributes(szIconPath);
	        if (dwAttr==-1 || (dwAttr & FILE_ATTRIBUTE_DIRECTORY))
	            szIconPath[0]=0;
	    }
    } else {
        szIconPath[0]=0;
    }
    
    if (szIconPath[0]) {
	    hClassIcon = (HICON)LoadImage(0, szIconPath, IMAGE_ICON, 
		    GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR|LR_LOADFROMFILE);
	    hClassIconSm = (HICON)LoadImage(0, szIconPath, IMAGE_ICON, 
		    GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR|LR_LOADFROMFILE);
	}
    if (!hClassIcon) {
	    szIconPath[0]=0;
	    
	    hClassIcon = (HICON)LoadImage(GetModuleHandle(0), 
		    MAKEINTRESOURCE(gSet.nIconID), IMAGE_ICON, 
		    GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
		    
	    if (hClassIconSm) DestroyIcon(hClassIconSm);
	    hClassIconSm = (HICON)LoadImage(GetModuleHandle(0), 
		    MAKEINTRESOURCE(gSet.nIconID), IMAGE_ICON, 
		    GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    }
}

bool CConEmuMain::LoadVersionInfo(wchar_t* pFullPath)
{
    LPBYTE pBuffer=NULL;
    wchar_t* pVersion=NULL;
    wchar_t* pDesc=NULL;
    
    const wchar_t WSFI[] = L"StringFileInfo";

    DWORD size = GetFileVersionInfoSizeW(pFullPath, &size);
    if(!size) return false;
    pBuffer = new BYTE[size];
    GetFileVersionInfoW((wchar_t*)pFullPath, 0, size, pBuffer);

    //Find StringFileInfo
    DWORD ofs;
    for(ofs = 92; ofs < size; ofs += *(WORD*)(pBuffer+ofs) )
        if(!lstrcmpiW((wchar_t*)(pBuffer+ofs+6), WSFI))
            break;
    if(ofs >= size) {
        delete pBuffer;
        return false;
    }
    TCHAR *langcode;
    langcode = (TCHAR*)(pBuffer + ofs + 42);

    TCHAR blockname[48];
    unsigned dsize;

    wsprintf(blockname, _T("\\%s\\%s\\FileVersion"), WSFI, langcode);
    if(!VerQueryValue(pBuffer, blockname, (void**)&pVersion, &dsize))
        pVersion = 0;
    else {
       if (dsize>=31) pVersion[31]=0;
       wcscpy(szConEmuVersion, pVersion);
       pVersion = wcsrchr(szConEmuVersion, L',');
       if (pVersion && wcscmp(pVersion, L", 0")==0)
	       *pVersion = 0;
    }
    
    delete pBuffer;
    
    return true;
}

void CConEmuMain::ShowSysmenu(HWND Wnd, HWND Owner, int x, int y)
{
    bool iconic = IsIconic(Wnd);
    bool zoomed = IsZoomed(Wnd);
    bool visible = IsWindowVisible(Wnd);
    int style = GetWindowLong(Wnd, GWL_STYLE);

    HMENU systemMenu = GetSystemMenu(Wnd, false);
    if (!systemMenu)
        return;

    EnableMenuItem(systemMenu, SC_RESTORE, MF_BYCOMMAND | (iconic || zoomed ? MF_ENABLED : MF_GRAYED));
    EnableMenuItem(systemMenu, SC_MOVE, MF_BYCOMMAND | (!(iconic || zoomed) ? MF_ENABLED : MF_GRAYED));
    EnableMenuItem(systemMenu, SC_SIZE, MF_BYCOMMAND | (!(iconic || zoomed) && (style & WS_SIZEBOX) ? MF_ENABLED : MF_GRAYED));
    EnableMenuItem(systemMenu, SC_MINIMIZE, MF_BYCOMMAND | (!iconic && (style & WS_MINIMIZEBOX)? MF_ENABLED : MF_GRAYED));
    EnableMenuItem(systemMenu, SC_MAXIMIZE, MF_BYCOMMAND | (!zoomed && (style & WS_MAXIMIZEBOX) ? MF_ENABLED : MF_GRAYED));
    EnableMenuItem(systemMenu, ID_TOTRAY, MF_BYCOMMAND | (visible ? MF_ENABLED : MF_GRAYED));

    SendMessage(Wnd, WM_INITMENU, (WPARAM)systemMenu, 0);
    SendMessage(Wnd, WM_INITMENUPOPUP, (WPARAM)systemMenu, MAKELPARAM(0, true));
    SetActiveWindow(Owner);

    int command = TrackPopupMenu(systemMenu, TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, x, y, 0, Owner, NULL);

    if (Icon.isWindowInTray)
        switch(command)
        {
        case SC_RESTORE:
        case SC_MOVE:
        case SC_SIZE:
        case SC_MINIMIZE:
        case SC_MAXIMIZE:
            SendMessage(Wnd, WM_TRAYNOTIFY, 0, WM_LBUTTONDOWN);
            break;
        }

    if (command)
        PostMessage(Wnd, WM_SYSCOMMAND, (WPARAM)command, 0);
}


/* ****************************************************** */
/*                                                        */
/*                      Painting                          */
/*                                                        */
/* ****************************************************** */

#ifdef colorer_func
	{
	Painting() {};
	}
#endif

void CConEmuMain::InvalidateAll()
{
	InvalidateRect(ghWnd, NULL, TRUE);
	InvalidateRect(ghWndDC, NULL, TRUE);
	InvalidateRect(m_Back.mh_Wnd, NULL, TRUE);
	TabBar.Invalidate();
}

LRESULT CConEmuMain::OnPaint(WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

    PAINTSTRUCT ps;
    HDC hDc = BeginPaint(ghWnd, &ps);

	PaintGaps(hDc);

	EndPaint(ghWnd, &ps);
	result = DefWindowProc(ghWnd, WM_PAINT, wParam, lParam);

	return result;
}

void CConEmuMain::PaintGaps(HDC hDC/*=NULL*/)
{
	//TODO: !!! Тут раньше были margins, а теперь DC rect
	BOOL lbOurDc = (hDC==NULL);
    
	if (hDC==NULL)
		hDC = GetDC(ghWnd); // Главное окно!

	HBRUSH hBrush = CreateSolidBrush(gSet.Colors[0]);

	RECT rcClient;
	GetClientRect(ghWnd, &rcClient); // Клиентская часть главного окна

	WINDOWPLACEMENT wpl; memset(&wpl, 0, sizeof(wpl)); wpl.length = sizeof(wpl);
	GetWindowPlacement(ghWndDC, &wpl); // Положение окна, в котором идет отрисовка

	//RECT offsetRect = ConsoleOffsetRect(); // смещение с учетом табов
	RECT dcRect; GetClientRect(ghWndDC, &dcRect);
	RECT offsetRect = dcRect;
	MapWindowPoints(ghWndDC, ghWnd, (LPPOINT)&offsetRect, 2);
	

	// paint gaps between console and window client area with first color

	RECT rect;

	//TODO:!!!
	// top
	rect = rcClient;
	rect.top += offsetRect.top;
	rect.bottom = wpl.rcNormalPosition.top;
	if (!IsRectEmpty(&rect))
		FillRect(hDC, &rect, hBrush);
#ifdef _DEBUG
	GdiFlush();
#endif

	// right
	rect.left = wpl.rcNormalPosition.right;
	rect.bottom = rcClient.bottom;
	if (!IsRectEmpty(&rect))
		FillRect(hDC, &rect, hBrush);
#ifdef _DEBUG
	GdiFlush();
#endif

	// left
	rect.left = 0;
	rect.right = wpl.rcNormalPosition.left;
	rect.bottom = rcClient.bottom;
	if (!IsRectEmpty(&rect))
		FillRect(hDC, &rect, hBrush);
#ifdef _DEBUG
	GdiFlush();
#endif

	// bottom
	rect.left = 0;
	rect.right = rcClient.right;
	rect.top = wpl.rcNormalPosition.bottom;
	rect.bottom = rcClient.bottom;
	if (!IsRectEmpty(&rect))
		FillRect(hDC, &rect, hBrush);
#ifdef _DEBUG
	GdiFlush();
#endif

	DeleteObject(hBrush);

	if (lbOurDc)
		ReleaseDC(ghWnd, hDC);
}


/* ****************************************************** */
/*                                                        */
/*                 Status functions                       */
/*                                                        */
/* ****************************************************** */

#ifdef colorer_func
	{
	Status_Functions() {};
	}
#endif

LPCTSTR CConEmuMain::GetTitle()
{
	if (!Title[0])
		return _T("ConEmu");
	return Title;
}

void CConEmuMain::UpdateTitle(LPCTSTR asNewTitle)
{
    wcscpy(Title, asNewTitle);
    SetWindowText(ghWnd, Title);
    if (ghWndApp)
		SetWindowText(ghWndApp, Title);

	LPTSTR pszTitle = GetTitleStart();
	mn_ActiveStatus = 0;

	// далее идут взаимоисключающие флаги
	if (_tcsncmp(pszTitle, _T("edit "), 5)==0 || _tcsncmp(pszTitle, ms_EditorRus, _tcslen(ms_EditorRus))==0)
		mn_ActiveStatus |= CES_EDITOR;
	else if (_tcsncmp(pszTitle, _T("view "), 5)==0 || _tcsncmp(pszTitle, ms_ViewerRus, _tcslen(ms_ViewerRus))==0)
		mn_ActiveStatus |= CES_VIEWER;
}

LPTSTR CConEmuMain::GetTitleStart()
{
    TCHAR* pszTitle=Title;
    if (Title[0]==_T('[') && isdigit(Title[1]) && (Title[2]==_T('/') || Title[3]==_T('/'))) {
	    // ConMan
	    pszTitle = _tcschr(Title, _T(']'));
	    if (!pszTitle)
		    return NULL;
		pszTitle++; // пропустить ']' и последующие пробелы
		while (*pszTitle==L' ' /*&& *pszTitle!=_T('{')*/)
			pszTitle++;
    }
	return pszTitle;
}

bool CConEmuMain::isConSelectMode()
{
    //TODO: По курсору, что-ли попробовать определять?
    return gb_ConsoleSelectMode;
}

bool CConEmuMain::isFilePanel(bool abPluginAllowed/*=false*/)
{
    TCHAR* pszTitle=GetTitleStart();
    if (!pszTitle) return false;

	if (abPluginAllowed) {
		if (isEditor() || isViewer())
			return false;
	}
    
    if ((abPluginAllowed && pszTitle[0]==_T('{')) ||
		(_tcsncmp(pszTitle, _T("{\\\\"), 3)==0) ||
	    (pszTitle[0] == _T('{') && isalpha(pszTitle[1]) && pszTitle[2] == _T(':') && pszTitle[3] == _T('\\')))
    {
	    TCHAR *Br = _tcsrchr(pszTitle, _T('}'));
	    if (Br && _tcscmp(Br, _T("} - Far"))==0)
		    return true;
    }
    //TCHAR *BrF = _tcschr(Title, '{'), *BrS = _tcschr(Title, '}'), *Slash = _tcschr(Title, '\\');
    //if (BrF && BrS && Slash && BrF == Title && (Slash == Title+1 || Slash == Title+3))
    //    return true;
    return false;
}

bool CConEmuMain::isEditor()
{
	return mn_ActiveStatus & CES_EDITOR;
}

bool CConEmuMain::isViewer()
{
	return mn_ActiveStatus & CES_VIEWER;
}

// Заголовок окна для PictureView вообще может пользователем настраиваться, так что
// рассчитывать на него при определения "Просмотра" - нельзя
bool CConEmuMain::isPictureView()
{
    bool lbRc = false;
    
	if (hPictureView && !IsWindow(hPictureView)) {
		InvalidateAll();
	    hPictureView = NULL;
	}
	
	if (!hPictureView) {
		hPictureView = FindWindowEx(ghWnd, NULL, L"FarPictureViewControlClass", NULL);
		if (!hPictureView)
			hPictureView = FindWindowEx(ghWndDC, NULL, L"FarPictureViewControlClass", NULL);
		if (!hPictureView) { // FullScreen?
			hPictureView = FindWindowEx(NULL, NULL, L"FarPictureViewControlClass", NULL);
			// Хорошо бы конечно процесс окна проверить... но он может не совпадать с нашим запущенным
		}
	}

	lbRc = hPictureView!=NULL;

    // Если вызывали Help (F1) - окошко PictureView прячется
    if (!IsWindowVisible(hPictureView)) {
        lbRc = false;
        hPictureView = NULL;
    }
    if (bPicViewSlideShow && !hPictureView) {
        bPicViewSlideShow=false;
    }

    return lbRc;
}







/* ****************************************************** */
/*                                                        */
/*                        EVENTS                          */
/*                                                        */
/* ****************************************************** */

#ifdef colorer_func
	{
	EVENTS() {};
	}
#endif

LRESULT CConEmuMain::OnClose(HWND hWnd)
{
    //Icon.Delete(); - перенес в WM_DESTROY
	//gConEmu.mb_InClose = TRUE;
    SENDMESSAGE(ghConWnd, WM_CLOSE, 0, 0);
	//gConEmu.mb_InClose = FALSE;
	return 0;
}

LRESULT CConEmuMain::OnCopyData(PCOPYDATASTRUCT cds)
{
	LRESULT result = 0;
    
	if (cds->dwData == 0) {
		BOOL lbInClose = FALSE;
		DWORD ProcList[2], ProcCount=0;
	    ProcCount = GetConsoleProcessList(ProcList,2);
		if (ProcCount<=2)
			lbInClose = TRUE;

		// хотя в последнем плагине и не приходит...
		if (!lbInClose) { // Чтобы табы не прыгали при щелчке по крестику
			// Приходит из плагина по ExitFAR
			ForceShowTabs(FALSE);

			//CONSOLE_SCREEN_BUFFER_INFO inf; memset(&inf, 0, sizeof(inf));
			//GetConsoleScreenBufferInfo(pVCon->hConOut(), &inf);
			if ((gSet.BufferHeight > 0) /*&& (inf.dwSize.Y==(inf.srWindow.Bottom-inf.srWindow.Top+1))*/)
			{
				DWORD mode = 0;
				BOOL lb = GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode);
				mode |= ENABLE_QUICK_EDIT_MODE|ENABLE_INSERT_MODE|ENABLE_EXTENDED_FLAGS;
				lb = SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);
			}
		}
	} else {
		BOOL lbNeedInval=FALSE;
		ConEmuTab* tabs = (ConEmuTab*)cds->lpData;
		// Если не активен ИЛИ появлились edit/view ИЛИ табы просили отображать всегда
		// Это сообщение посылает плагин ConEmu при изменениях и входе в FAR
		if (!TabBar.IsActive() && gSet.isTabs && (cds->dwData>1 || tabs[0].Type!=1/*WTYPE_PANELS*/ || gSet.isTabs==1))
		{
			if ((gSet.BufferHeight > 0) /*&& (inf.dwSize.Y==(inf.srWindow.Bottom-inf.srWindow.Top+1))*/)
			{
				//CONSOLE_SCREEN_BUFFER_INFO inf; memset(&inf, 0, sizeof(inf));
				//GetConsoleScreenBufferInfo(pVCon->hConOut(), &inf);
				DWORD mode = 0;
				BOOL lb = GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode);
				mode &= ~(ENABLE_QUICK_EDIT_MODE|ENABLE_INSERT_MODE);
				mode |= ENABLE_EXTENDED_FLAGS;
				lb = SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);
			}

			TabBar.Activate();
			lbNeedInval = TRUE;
		}
		TabBar.Update(tabs, cds->dwData);
		if (lbNeedInval)
		{
			SyncConsoleToWindow();
			//RECT rc = ConsoleOffsetRect();
			//rc.bottom = rc.top; rc.top = 0;
			InvalidateRect(ghWnd, NULL/*&rc*/, FALSE);
			if (!gSet.isFullScreen && !IsZoomed(ghWnd)) {
				//SyncWindowToConsole(); -- это делать нельзя, т.к. FAR еще не отработал изменение консоли!
				gbPostUpdateWindowSize = true;
			}
		}
	}

	return result;
}

LRESULT CConEmuMain::OnCreate(HWND hWnd)
{
	ghWnd = hWnd; // ставим сразу, чтобы функции могли пользоваться
	Icon.LoadIcon(hWnd, gSet.nIconID/*IDI_ICON1*/);
	
	// Чтобы можно было найти хэндл окна по хэндлу консоли
	SetWindowLong(hWnd, GWL_USERDATA, (LONG)ghConWnd);

	gConEmu.m_Back.Create();

	if (!gConEmu.m_Child.Create())
		return -1;

	return 0;
}

LRESULT CConEmuMain::OnDestroy(HWND hWnd)
{
    Icon.Delete();
    if (gConEmu.DragDrop) {
        delete gConEmu.DragDrop;
        gConEmu.DragDrop = NULL;
    }
    if (gConEmu.ProgressBars) {
        delete gConEmu.ProgressBars;
        gConEmu.ProgressBars = NULL;
    }
    PostQuitMessage(0);

	return 0;
}

LRESULT CConEmuMain::OnFocus(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	/*
    case WM_ACTIVATE:
    case WM_ACTIVATEAPP:
    case WM_KILLFOCUS:
    case WM_SETFOCUS:
	*/


    /*if (messg == WM_SETFOCUS) {
	    //TODO: проверка Options
		if (ghOpWnd) {
			TCHAR szClass[128], szTitle[255], szMsg[1024];

			GetClassName(hWnd, szClass, 64);
			GetWindowText(hWnd, szTitle, 255);

			wsprintf(szMsg, _T("WM_SETFOCUS to (HWND=0x%08x)\r\n%s - %s\r\n"),
				(DWORD)hWnd, szClass, szTitle);

			if (!wParam)
				wParam = (WPARAM)GetFocus();
			if (wParam) {
				GetClassName((HWND)wParam, szClass, 64);
				GetWindowText((HWND)wParam, szTitle, 255);
				wsprintf(szMsg+_tcslen(szMsg), _T("from (HWND=0x%08x)\r\n%s - %s\r\n"),
					(DWORD)hWnd, szClass, szTitle);
			}
			MBoxA(szMsg);
		}
		else if (ghWndDC && IsWindow(ghWndDC)) {
			SetFocus(ghWndDC);
		}
	}*/

    /*if (messg == WM_SETFOCUS || messg == WM_KILLFOCUS) {
        if (hPictureView && IsWindow(hPictureView)) {
            break; // в FAR не пересылать
        }
    }*/

    POSTMESSAGE(ghConWnd, messg, wParam, lParam, FALSE);
	return 0;
}

LRESULT CConEmuMain::OnGetMinMaxInfo(LPMINMAXINFO pInfo)
{
	LRESULT result = 0;

	//RECT rcFrame = CalcMargins(CEM_FRAME);
    //POINT p = cwShift;
    //RECT shiftRect = ConsoleOffsetRect();
	//RECT shiftRect = ConsoleOffsetRect();

    // Минимально допустимые размеры консоли
    //COORD srctWindow; srctWindow.X=28; srctWindow.Y=9;
	RECT rcFrame = CalcRect(CER_MAIN, MakeRect(28,9), CER_CONSOLE);

	pInfo->ptMinTrackSize.x = rcFrame.right;
	pInfo->ptMinTrackSize.y = rcFrame.bottom;

	//pInfo->ptMinTrackSize.x = srctWindow.X * (gSet.LogFont.lfWidth ? gSet.LogFont.lfWidth : 4)
	//	+ p.x + shiftRect.left + shiftRect.right;

	//pInfo->ptMinTrackSize.y = srctWindow.Y * (gSet.LogFont.lfHeight ? gSet.LogFont.lfHeight : 6)
	//	+ p.y + shiftRect.top + shiftRect.bottom;

	if (gSet.isFullScreen) {
		pInfo->ptMaxTrackSize = ptFullScreenSize;
		pInfo->ptMaxSize = ptFullScreenSize;
	}

	return result;
}

LRESULT CConEmuMain::OnKeyboard(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

    //#ifdef _DEBUG
    //{
    //    TCHAR szDbg[32];
    //    wsprintf(szDbg, _T("%s - %c (%i)"),
	//        ((messg == WM_KEYDOWN) ? _T("Dn") : _T("Up")),
	//        (TCHAR)wParam, wParam);
	//    SetWindowText(ghWnd, szDbg);
    //}
    //#endif

	if (messg == WM_KEYDOWN || messg == WM_KEYUP)
	{
		if (wParam == VK_PAUSE && !isPressed(VK_CONTROL)) {
			if (gConEmu.isPictureView()) {
				if (messg == WM_KEYUP) {
					gConEmu.bPicViewSlideShow = !gConEmu.bPicViewSlideShow;
					if (gConEmu.bPicViewSlideShow) {
						if (gSet.nSlideShowElapse<=500) gSet.nSlideShowElapse=500;
						//SetTimer(hWnd, 3, gSet.nSlideShowElapse, NULL);
						gConEmu.dwLastSlideShowTick = GetTickCount() - gSet.nSlideShowElapse;
					//} else {
					//  KillTimer(hWnd, 3);
					}
				}
				return 0;
			}
		} else if (gConEmu.bPicViewSlideShow) {
			//KillTimer(hWnd, 3);
			if (wParam==0xbd/* -_ */ || wParam==0xbb/* =+ */) {
				if (messg == WM_KEYDOWN) {
					if (wParam==0xbb)
						gSet.nSlideShowElapse = 1.2 * gSet.nSlideShowElapse;
					else {
						gSet.nSlideShowElapse = gSet.nSlideShowElapse / 1.2;
						if (gSet.nSlideShowElapse<=500) gSet.nSlideShowElapse=500;
					}
				}
				return 0;
			} else {
				gConEmu.bPicViewSlideShow = false; // отмена слайдшоу
			}
		}
	}


  // buffer mode: scroll with keys  -- NightRoman
    if (gSet.BufferHeight && messg == WM_KEYDOWN && isPressed(VK_CONTROL))
    {
        switch(wParam)
        {
        case VK_DOWN:
            POSTMESSAGE(ghConWnd, WM_VSCROLL, SB_LINEDOWN, NULL, FALSE);
            return 0;
        case VK_UP:
            POSTMESSAGE(ghConWnd, WM_VSCROLL, SB_LINEUP, NULL, FALSE);
            return 0;
        case VK_NEXT:
            POSTMESSAGE(ghConWnd, WM_VSCROLL, SB_PAGEDOWN, NULL, FALSE);
            return 0;
        case VK_PRIOR:
            POSTMESSAGE(ghConWnd, WM_VSCROLL, SB_PAGEUP, NULL, FALSE);
            return 0;
        }
    }

    if (messg == WM_KEYDOWN && wParam == VK_SPACE && isPressed(VK_CONTROL) && isPressed(VK_LWIN) && isPressed(VK_MENU))
    {
        if (!IsWindowVisible(ghConWnd))
        {
            gConEmu.isShowConsole = true;
            ShowWindow(ghConWnd, SW_SHOWNORMAL);
            //if (gConEmu.setParent) SetParent(ghConWnd, 0);
            EnableWindow(ghConWnd, true);
        }
        else
        {
            gConEmu.isShowConsole = false;
            if (!gSet.isConVisible) ShowWindow(ghConWnd, SW_HIDE);
            //if (gConEmu.setParent) SetParent(ghConWnd, HDCWND);
            if (!gSet.isConVisible) EnableWindow(ghConWnd, false);
        }
        return 0;
    }

    if (gConEmu.gb_ConsoleSelectMode && messg == WM_KEYDOWN && ((wParam == VK_ESCAPE) || (wParam == VK_RETURN)))
        gConEmu.gb_ConsoleSelectMode = false; //TODO: может как-то по другому определять?

    // Основная обработка 
    {
        if (messg == WM_SYSKEYDOWN) 
            if (wParam == VK_INSERT && lParam & 29)
                gConEmu.gb_ConsoleSelectMode = true;

        static bool isSkipNextAltUp = false;
        if (messg == WM_SYSKEYDOWN && wParam == VK_RETURN && lParam & 29)
        {
            if (gSet.isSentAltEnter)
            {
                POSTMESSAGE(ghConWnd, WM_KEYDOWN, VK_MENU, 0, TRUE);
                POSTMESSAGE(ghConWnd, WM_KEYDOWN, VK_RETURN, 0, TRUE);
                POSTMESSAGE(ghConWnd, WM_KEYUP, VK_RETURN, 0, TRUE);
                POSTMESSAGE(ghConWnd, WM_KEYUP, VK_MENU, 0, TRUE);
            }
            else
            {
                if (isPressed(VK_SHIFT))
                    return 0;

                if (!gSet.isFullScreen)
                    gConEmu.SetWindowMode(rFullScreen);
                else
                    gConEmu.SetWindowMode(gConEmu.isWndNotFSMaximized ? rMaximized : rNormal);

                isSkipNextAltUp = true;
                //POSTMESSAGE(ghConWnd, messg, wParam, lParam);
            }
        }
        else if (messg == WM_SYSKEYDOWN && wParam == VK_SPACE && lParam & 29 && !isPressed(VK_SHIFT))
        {
            RECT rect, cRect;
            GetWindowRect(ghWnd, &rect);
            GetClientRect(ghWnd, &cRect);
            WINDOWINFO wInfo;   GetWindowInfo(ghWnd, &wInfo);
            gConEmu.ShowSysmenu(ghWnd, ghWnd, rect.right - cRect.right - wInfo.cxWindowBorders, rect.bottom - cRect.bottom - wInfo.cyWindowBorders);
        }
        else if (messg == WM_KEYUP && wParam == VK_MENU && isSkipNextAltUp) isSkipNextAltUp = false;
        else if (messg == WM_SYSKEYDOWN && wParam == VK_F9 && lParam & 29 && !isPressed(VK_SHIFT))
			gConEmu.SetWindowMode((IsZoomed(ghWnd)||(gSet.isFullScreen&&isWndNotFSMaximized)) ? rNormal : rMaximized);
        else
            POSTMESSAGE(ghConWnd, messg, wParam, lParam, FALSE);
    }

	return 0;
}

LRESULT CConEmuMain::OnMouse(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
    short winX = GET_X_LPARAM(lParam);
    short winY = GET_Y_LPARAM(lParam);

    RECT conRect, consoleRect;
	POINT ptCur;

	// Иначе в консоль проваливаются щелчки по незанятому полю таба...
	if (messg==WM_LBUTTONDOWN || messg==WM_RBUTTONDOWN || messg==WM_MBUTTONDOWN || 
		messg==WM_LBUTTONDBLCLK || messg==WM_RBUTTONDBLCLK || messg==WM_MBUTTONDBLCLK)
	{
		GetCursorPos(&ptCur);
		GetWindowRect(ghWndDC, &consoleRect);
		if (!PtInRect(&consoleRect, ptCur))
			return 0;
	}

    GetClientRect(ghConWnd, &conRect);

	memset(&consoleRect, 0, sizeof(consoleRect));
    winX -= consoleRect.left;
    winY -= consoleRect.top;
    short newX = MulDiv(winX, conRect.right, klMax<uint>(1, pVCon->Width));
    short newY = MulDiv(winY, conRect.bottom, klMax<uint>(1, pVCon->Height));

    if (newY<0 || newX<0)
        return 0;

    if (gSet.BufferHeight)
    {
       // buffer mode: cheat the console window: adjust its position exactly to the cursor
       RECT win;
       GetWindowRect(ghWnd, &win);
       short x = win.left + winX - newX;
       short y = win.top + winY - newY;
       RECT con;
       GetWindowRect(ghConWnd, &con);
       if (con.left != x || con.top != y)
          MOVEWINDOW(ghConWnd, x, y, con.right - con.left + 1, con.bottom - con.top + 1, TRUE);
    }
    else if (messg == WM_MOUSEMOVE)
    {
        // WM_MOUSEMOVE вроде бы слишком часто вызывается даже при условии что курсор не двигается...
        if (wParam==gConEmu.lastMMW && lParam==gConEmu.lastMML) {
            return 0;
        }
        gConEmu.lastMMW=wParam; gConEmu.lastMML=lParam;

        /*if (isLBDown &&   (cursor.x-LOWORD(lParam)>DRAG_DELTA || 
                           cursor.x-LOWORD(lParam)<-DRAG_DELTA || 
                           cursor.y-HIWORD(lParam)>DRAG_DELTA || 
                           cursor.y-HIWORD(lParam)<-DRAG_DELTA))*/
        if (gConEmu.isLBDown && !PTDIFFTEST(gConEmu.cursor,DRAG_DELTA) 
			&& !gConEmu.isDragProcessed && !gConEmu.mb_InSizing)
        {
            // Если сначала фокус был на файловой панели, но после LClick он попал на НЕ файловую - отменить ShellDrag
            if (!gConEmu.isFilePanel()) {
	            gConEmu.isLBDown = false;
	            return 0;
            }
            // Иначе иногда срабатывает FAR'овский D'n'D
            //SENDMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( newX, newY ));     //посылаем консоли отпускание
			if (gConEmu.DragDrop)
				gConEmu.DragDrop->Drag(); //сдвинулись при зажатой левой
			//isDragProcessed=false; -- убрал, иначе при бросании в пассивную панель больших файлов дроп может вызваться еще раз???
            POSTMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( newX, newY ), TRUE);     //посылаем консоли отпускание
            return 0;
        }
        else if (gSet.isRClickSendKey && gConEmu.isRBDown)
        {
            //Если двинули мышкой, а была включена опция RClick - не вызывать
            //контекстное меню - просто послать правый клик
            if (!PTDIFFTEST(gConEmu.Rcursor, RCLICKAPPSDELTA))
            {
                gConEmu.isRBDown=false;
                POSTMESSAGE(ghConWnd, WM_RBUTTONDOWN, 0, MAKELPARAM( gConEmu.RBDownNewX, gConEmu.RBDownNewY ), TRUE);
            }
            return 0;
        }
        /*if (!isRBDown && (wParam==MK_RBUTTON)) {
            // Чтобы при выделении правой кнопкой файлы не пропускались
            if ((newY-RBDownNewY)>5) {// пока попробуем для режима сверху вниз
                for (short y=RBDownNewY;y<newY;y+=5)
                    POSTMESSAGE(ghConWnd, WM_MOUSEMOVE, wParam, MAKELPARAM( RBDownNewX, y ), TRUE);
            }
            RBDownNewX=newX; RBDownNewY=newY;
        }*/
    } else {
        gConEmu.lastMMW=-1; gConEmu.lastMML=-1;

        if (messg == WM_LBUTTONDOWN)
        {
            if (gConEmu.isLBDown) ReleaseCapture(); // Вдруг сглючило?
            gConEmu.isLBDown=false;
            if (!gConEmu.isConSelectMode() && gConEmu.isFilePanel())
            {
                SetCapture(ghWndDC);
                gConEmu.cursor.x = LOWORD(lParam);
                gConEmu.cursor.y = HIWORD(lParam); 
                gConEmu.isLBDown=true;
                gConEmu.isDragProcessed=false;
                POSTMESSAGE(ghConWnd, messg, wParam, MAKELPARAM( newX, newY ), FALSE); // было SEND
                return 0;
            }
        }
        else if (messg == WM_LBUTTONUP)
        {
            if (gConEmu.isLBDown) {
                gConEmu.isLBDown=false;
                ReleaseCapture();
            }
        }
        else if (messg == WM_RBUTTONDOWN)
        {
            gConEmu.Rcursor.x = LOWORD(lParam);
            gConEmu.Rcursor.y = HIWORD(lParam);
            gConEmu.RBDownNewX=newX;
            gConEmu.RBDownNewY=newY;
            gConEmu.isRBDown=false;

            // Если ничего лишнего не нажато!
            if (gSet.isRClickSendKey && !(wParam&(MK_CONTROL|MK_LBUTTON|MK_MBUTTON|MK_SHIFT|MK_XBUTTON1|MK_XBUTTON2)))
            {
                //TCHAR *BrF = _tcschr(Title, '{'), *BrS = _tcschr(Title, '}'), *Slash = _tcschr(Title, '\\');
                //if (BrF && BrS && Slash && BrF == Title && (Slash == Title+1 || Slash == Title+3))
                if (gConEmu.isFilePanel()) // Maximus5
                {
                    //заведем таймер на .3
                    //если больше - пошлем apps
                    gConEmu.isRBDown=true; gConEmu.ibSkilRDblClk=false;
                    //SetTimer(hWnd, 1, 300, 0); -- Maximus5, откажемся от таймера
                    gConEmu.dwRBDownTick = GetTickCount();
                    return 0;
                }
            }
        }
        else if (messg == WM_RBUTTONUP)
        {
            if (gSet.isRClickSendKey && gConEmu.isRBDown)
            {
                gConEmu.isRBDown=false; // сразу сбросим!
                if (PTDIFFTEST(gConEmu.Rcursor,RCLICKAPPSDELTA))
                {
                    //держали зажатой <.3
                    //убьем таймер, кликнием правой кнопкой
                    //KillTimer(hWnd, 1); -- Maximus5, таймер более не используется
                    DWORD dwCurTick=GetTickCount();
                    DWORD dwDelta=dwCurTick-gConEmu.dwRBDownTick;
                    // Если держали дольше .3с, но не слишком долго :)
                    if ((gSet.isRClickSendKey==1) ||
                        (dwDelta>RCLICKAPPSTIMEOUT && dwDelta<10000))
                    {
                        // Сначала выделить файл под курсором
                        POSTMESSAGE(ghConWnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM( gConEmu.RBDownNewX, gConEmu.RBDownNewY ), TRUE);
                        POSTMESSAGE(ghConWnd, WM_LBUTTONUP, 0, MAKELPARAM( gConEmu.RBDownNewX, gConEmu.RBDownNewY ), TRUE);
                    
                        pVCon->Update(true);
                        INVALIDATE(); //InvalidateRect(HDCWND, NULL, FALSE);

                        // А теперь можно и Apps нажать
						gConEmu.ibSkilRDblClk=true; // чтобы пока FAR думает в консоль не проскочило мышиное сообщение
                        POSTMESSAGE(ghConWnd, WM_KEYDOWN, VK_APPS, 0, TRUE);
                        return 0;
                    }
                }
                // Иначе нужно сначала послать WM_RBUTTONDOWN
                POSTMESSAGE(ghConWnd, WM_RBUTTONDOWN, wParam, MAKELPARAM( newX, newY ), TRUE);
            }
            gConEmu.isRBDown=false; // чтобы не осталось случайно
        }
		else if (messg == WM_RBUTTONDBLCLK) {
			if (gConEmu.ibSkilRDblClk) {
				gConEmu.ibSkilRDblClk = false;
				return 0; // не обрабатывать, сейчас висит контекстное меню
			}
		}
    }

	// заменим даблклик вторым обычным
    POSTMESSAGE(ghConWnd, messg == WM_RBUTTONDBLCLK ? WM_RBUTTONDOWN : messg, wParam, MAKELPARAM( newX, newY ), FALSE);
    return 0;
}

LRESULT CConEmuMain::OnSysCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
    switch(LOWORD(wParam))
    {
    case ID_SETTINGS:
        if (ghOpWnd && IsWindow(ghOpWnd)) {
	        ShowWindow ( ghOpWnd, SW_SHOWNORMAL );
	        SetFocus ( ghOpWnd );
	        break; // А то открывались несколько окон диалогов :)
	    }
		//DialogBox((HINSTANCE)GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG1), 0, CSettings::wndOpProc);
		CSettings::Dialog();
        break;
    case ID_DUMPCONSOLE:
	    if (pVCon)
		    pVCon->DumpConsole();
		break;
    case ID_HELP:
        {
	        WCHAR szTitle[255];
	        wsprintf(szTitle, L"About ConEmu (%s)...", gConEmu.szConEmuVersion);
            MessageBox(ghOpWnd, pHelp, szTitle, MB_ICONQUESTION);
        }
        break;
    case ID_TOTRAY:
        Icon.HideWindowToTray();
        break;
    case ID_CONPROP:
        #ifdef _DEBUG
        {
            HMENU hMenu = GetSystemMenu(ghConWnd, FALSE);
            MENUITEMINFO mii; TCHAR szText[255];
            for (int i=0; i<15; i++) {
                memset(&mii, 0, sizeof(mii));
                mii.cbSize = sizeof(mii); mii.dwTypeData=szText; mii.cch=255;
                mii.fMask = MIIM_ID|MIIM_STRING;
                if (GetMenuItemInfo(hMenu, i, TRUE, &mii)) {
                    mii.cbSize = sizeof(mii);
                } else
                    break;
            }
        }
        #endif
        POSTMESSAGE(ghConWnd, WM_SYSCOMMAND, 65527, 0, TRUE);
        break;
    }

    switch(wParam)
    {
    case SC_MAXIMIZE_SECRET:
        gConEmu.SetWindowMode(rMaximized);
        break;
    case SC_RESTORE_SECRET:
        gConEmu.SetWindowMode(rNormal);
        break;
    case SC_CLOSE:
        Icon.Delete();
		//SENDMESSAGE(ghConWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
        SENDMESSAGE(ghConWnd, WM_CLOSE, 0, 0); // ?? фар не ловит сообщение, ExitFAR не вызываются
        return 0;

    case SC_MAXIMIZE:
        if (wParam == SC_MAXIMIZE)
            CheckRadioButton(ghOpWnd, rNormal, rFullScreen, rMaximized);
    case SC_RESTORE:
        if (wParam == SC_RESTORE)
            CheckRadioButton(ghOpWnd, rNormal, rFullScreen, rNormal);

    default:
        if (wParam != 0xF100)
        {
            POSTMESSAGE(ghConWnd, WM_SYSCOMMAND, wParam, lParam, FALSE);
            result = DefWindowProc(hWnd, WM_SYSCOMMAND, wParam, lParam);
        }
    }
	return result;
}

LRESULT CConEmuMain::OnTimer(WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

    switch (wParam)
    {
    case 0:
        HWND foreWnd = GetForegroundWindow();
        if (!isShowConsole && !gSet.isConVisible)
        {
            /*if (foreWnd == ghConWnd)
                SetForegroundWindow(ghWnd);*/
            //#ifndef _DEBUG
            if (IsWindowVisible(ghConWnd))
                ShowWindow(ghConWnd, SW_HIDE);
            //#endif
        }

		//Maximus5. Hack - если какая-то зараза задизеблила окно
		DWORD dwStyle = GetWindowLong(ghWnd, GWL_STYLE);
		if (dwStyle & WS_DISABLED)
			EnableWindow(ghWnd, TRUE);

		CheckProcesses();
		if (gnLastProcessCount == 1) {
			Destroy();
			break;
		}


		BOOL lbIsPicView = isPictureView();
        if (bPicViewSlideShow) {
            DWORD dwTicks = GetTickCount();
            DWORD dwElapse = dwTicks - dwLastSlideShowTick;
            if (dwElapse > gSet.nSlideShowElapse)
            {
                if (IsWindow(hPictureView)) {
                    //
                    bPicViewSlideShow = false;
                    SendMessage(ghConWnd, WM_KEYDOWN, VK_NEXT, 0x01510001);
                    SendMessage(ghConWnd, WM_KEYUP, VK_NEXT, 0xc1510001);

                    // Окно могло измениться?
                    isPictureView();

                    dwLastSlideShowTick = GetTickCount();
                    bPicViewSlideShow = true;
                } else {
                    hPictureView = NULL;
                    bPicViewSlideShow = false;
                }
            }
        }

        if (cBlinkNext++ >= cBlinkShift)
        {
            cBlinkNext = 0;
            if (foreWnd == ghWnd || foreWnd == ghOpWnd
				#ifdef _DEBUG
				|| gbNoDblBuffer
				#endif
				)
                // switch cursor
                pVCon->Cursor.isVisible = !pVCon->Cursor.isVisible;
            else
                // turn cursor off
                pVCon->Cursor.isVisible = false;
        }

        GetWindowText(ghConWnd, TitleCmp, 1024);
        if (wcscmp(Title, TitleCmp))
			UpdateTitle(TitleCmp);

        TabBar.OnTimer();
        ProgressBars->OnTimer();

        
		if (mb_InSizing && !isPressed(VK_LBUTTON)) {
			mb_InSizing = FALSE;
			if (!mb_FullWindowDrag)
				ReSize();
		}

        if (lbIsPicView)
        {
            bool lbOK = true;
            if (!setParent) {
                // Проверка, может PictureView создался в консоли, а не в ConEmu?
                HWND hPicView = FindWindowEx(ghConWnd, NULL, L"FarPictureViewControlClass", NULL);
                if (!hPicView) {
                    lbOK = false; // смысла нет, все равно ничего не увидим
                }
            }
            if (lbOK) {
                isPiewUpdate = true;
                if (pVCon->Update(false))
                    INVALIDATE(); //InvalidateRect(HDCWND, NULL, FALSE);
                break;
            }
        } else 
        if (isPiewUpdate)
        {	// После скрытия/закрытия PictureView нужно передернуть консоль - ее размер мог измениться
            isPiewUpdate = false;
            SyncConsoleToWindow();
            //INVALIDATE(); //InvalidateRect(HDCWND, NULL, FALSE);
			InvalidateAll();
        }

        // Проверить, может в консоли размер съехал? (хрен его знает из-за чего...)
		CheckBufferSize();

        //if (!gbInvalidating && !gbInPaint)
        if (pVCon->Update(false/*gbNoDblBuffer*/))
        {
            //COORD c = ConsoleSizeFromWindow();
			RECT client; GetClientRect(ghWnd, &client);
			RECT c = CalcRect(CER_CONSOLE, client, CER_MAINCLIENT);
            if ((mb_FullWindowDrag || !mb_InSizing) &&
				(gbPostUpdateWindowSize || c.right != pVCon->TextWidth || c.bottom != pVCon->TextHeight))
            {
				gbPostUpdateWindowSize = false;
                if (!gSet.isFullScreen && !IsZoomed(ghWnd))
                    SyncWindowToConsole();
                else
                    SyncConsoleToWindow();
            }

            INVALIDATE(); //InvalidateRect(HDCWND, NULL, FALSE);

            // update scrollbar
            if (gSet.BufferHeight)
            {
                SCROLLINFO si;
                ZeroMemory(&si, sizeof(si));
                si.cbSize = sizeof(si);
                si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;
                if (GetScrollInfo(ghConWnd, SB_VERT, &si))
                    SetScrollInfo(HDCWND, SB_VERT, &si, true);
            }
      //} -- в сборке NightRoman (isPiewUpdate) проверяется всегда

            /*if (!lbIsPicView && isPiewUpdate)
            {
                isPiewUpdate = false;
                SyncConsoleToWindow();
                INVALIDATE(); //InvalidateRect(ghWnd, NULL, FALSE);
            }*/
        }
    }

	return result;
}

LRESULT CConEmuMain::WndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

	if (messg == WM_SYSCHAR) {
        #ifdef _DEBUG
        /*{
	        TCHAR szDbg[32];
	        wsprintf(szDbg, _T("SysChar - %c (%i)"),
		        (TCHAR)wParam, wParam);
		    SetWindowText(ghWnd, szDbg);
        }*/
        #endif
		return TRUE;
    } else if (messg == WM_CHAR) {
        #ifdef _DEBUG
        /*{
	        TCHAR szDbg[32];
	        wsprintf(szDbg, _T("Char - %c (%i)"),
		        (TCHAR)wParam, wParam);
		    SetWindowText(ghWnd, szDbg);
        }*/
        #endif
    }

    switch (messg)
    {
    case WM_NOTIFY:
    {
        result = TabBar.OnNotify((LPNMHDR)lParam);
        break;
    }

    case WM_COPYDATA:
    {
        PCOPYDATASTRUCT cds = PCOPYDATASTRUCT(lParam);
		result = OnCopyData(cds);
        break;
    }
    
    case WM_ERASEBKGND:
		return 0;
		
	case WM_PAINT:
		result = gConEmu.OnPaint(wParam, lParam);
	
    case WM_TIMER:
		result = gConEmu.OnTimer(wParam, lParam);
        break;

    case WM_SIZING:
		result = gConEmu.OnSizing(wParam, lParam);
        break;

	case WM_SIZE:
		result = gConEmu.OnSize(wParam, LOWORD(lParam), HIWORD(lParam));
        break;

	case WM_MOVE:
		// вызывается когда не надо...
		if (hWnd == ghWnd && ghOpWnd) {
			if (!gSet.isFullScreen && !IsZoomed(ghWnd) && !IsIconic(ghWnd))
			{
				RECT rc; GetWindowRect(ghWnd, &rc);
				gSet.UpdatePos(rc.left, rc.top);
			}
		}
		break;

	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO pInfo = (LPMINMAXINFO)lParam;
			result = OnGetMinMaxInfo(pInfo);
			break;
		}

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
		result = OnKeyboard(hWnd, messg, wParam, lParam);
		break;

    case WM_ACTIVATE:
    case WM_ACTIVATEAPP:
    case WM_KILLFOCUS:
    case WM_SETFOCUS:
		result = OnFocus(hWnd, messg, wParam, lParam);
		break;

    case WM_MOUSEMOVE:
    case WM_MOUSEWHEEL:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
		result = OnMouse(hWnd, messg, wParam, lParam);
		break;

    case WM_CLOSE:
		result = OnClose(hWnd);
        break;

    case WM_CREATE:
		result = OnCreate(hWnd);
        break;

    case WM_SYSCOMMAND:
		result = OnSysCommand(hWnd, wParam, lParam);
        break;

	case WM_NCLBUTTONDOWN:
		gConEmu.mb_InSizing = (messg==WM_NCLBUTTONDOWN);
		result = DefWindowProc(hWnd, messg, wParam, lParam);
		break;

    case WM_NCRBUTTONUP:
        Icon.HideWindowToTray();
        break;

    case WM_TRAYNOTIFY:
		result = Icon.OnTryIcon(hWnd, messg, wParam, lParam);
        break; 


    case WM_DESTROY:
		result = OnDestroy(hWnd);
        break;
    
    /*case WM_INPUTLANGCHANGE:
    case WM_INPUTLANGCHANGEREQUEST:
    case WM_IME_NOTIFY:*/
    case WM_VSCROLL:
        POSTMESSAGE(ghConWnd, messg, wParam, lParam, FALSE);
        
    default:
        if (messg) result = DefWindowProc(hWnd, messg, wParam, lParam);
    }
    return result;
}