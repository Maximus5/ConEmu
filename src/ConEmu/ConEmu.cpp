
#include "Header.h"
#include <Tlhelp32.h>
#include <Shlobj.h>
extern "C" {
#include "../common/ConEmuCheck.h"
}

#define PROCESS_WAIT_START_TIME 1000

#define PTDIFFTEST(C,D) PtDiffTest(C, LOWORD(lParam), HIWORD(lParam), D)
//(((abs(C.x-LOWORD(lParam)))<D) && ((abs(C.y-HIWORD(lParam)))<D))


// COM TaskBarList interface support
#ifdef __GNUC__
	#include "ShObjIdl_Part.h"
	const CLSID CLSID_TaskbarList = {0x56FDF344, 0xFD6D, 0x11d0, {0x95, 0x8A, 0x00, 0x60, 0x97, 0xC9, 0xA0, 0x90}};
	const IID IID_ITaskbarList3 = {0xea1afb91, 0x9e28, 0x4b86, {0x90, 0xe9, 0x9e, 0x9f, 0x8a, 0x5e, 0xef, 0xaf}};
#else
	#include <ShObjIdl.h>
	#ifndef __ITaskbarList3_INTERFACE_DEFINED__
		#undef __shobjidl_h__
		#include "ShObjIdl_Part.h"
		const IID IID_ITaskbarList3 = {0xea1afb91, 0x9e28, 0x4b86, {0x90, 0xe9, 0x9e, 0x9f, 0x8a, 0x5e, 0xef, 0xaf}};
	#endif
#endif


#if defined(__GNUC__)
#define EXT_GNUC_LOG
#endif

CConEmuMain::CConEmuMain()
{
    mn_MainThreadId = GetCurrentThreadId();
    wcscpy(szConEmuVersion, L"?.?.?.?");
    WindowMode=rNormal; mb_PassSysCommand = false; change2WindowMode = -1;
    isWndNotFSMaximized = false;
    mb_StartDetached = FALSE;
    mb_SkipSyncSize = false;
    isPiewUpdate = false; //true; --Maximus5
    gbPostUpdateWindowSize = false;
    hPictureView = NULL;  mrc_WndPosOnPicView = MakeRect(0,0);
    bPicViewSlideShow = false; 
    dwLastSlideShowTick = 0;
    cursor.x=0; cursor.y=0; Rcursor=cursor;
    m_LastConSize = MakeCoord(0,0);
    mp_DragDrop = NULL;
    ProgressBars = NULL;
    //cBlinkShift=0;
    Title[0] = 0; TitleCmp[0] = 0; MultiTitle[0] = 0; mn_Progress = -1;
    mb_InTimer = FALSE;
    //mb_InClose = FALSE;
    //memset(m_ProcList, 0, 1000*sizeof(DWORD)); 
    m_ProcCount=0; //mn_ConmanPID = 0; //mh_Infis = NULL; ms_InfisPath[0] = 0;
    //mn_TopProcessID = 0; //ms_TopProcess[0] = 0; mb_FarActive = FALSE;
    //mn_ActiveStatus = 0; //m_ActiveConmanIDX = 0; //mn_NeedRetryName = 0;
    mb_ProcessCreated = FALSE; mn_StartTick = 0;
    //mh_ConMan = NULL;
    //ConMan_MainProc = NULL; ConMan_LookForKeyboard = NULL; ConMan_ProcessCommand = NULL; 
    mb_IgnoreSizeChange = false;
    //mn_CurrentKeybLayout = (DWORD)GetKeyboardLayout(0);
    mn_ServerThreadId = 0; mh_ServerThread = NULL; mh_ServerThreadTerminate = NULL;
    //mpsz_RecreateCmd = NULL;
	ZeroStruct(mrc_Ideal);
	mb_MouseCaptured = FALSE;
	mb_HotKeyRegistered = FALSE;

    memset(&mouse, 0, sizeof(mouse));
    mouse.lastMMW=-1;
    mouse.lastMML=-1;

    ms_ConEmuExe[0] = 0;
    wchar_t *pszSlash = NULL;
    if (!GetModuleFileName(NULL, ms_ConEmuExe, MAX_PATH) || !(pszSlash = wcsrchr(ms_ConEmuExe, L'\\'))) {
        DisplayLastError(L"GetModuleFileName failed");
        TerminateProcess(GetCurrentProcess(), 100);
        return;
    }
    LoadVersionInfo(ms_ConEmuExe);
    // Добавить в окружение переменную с папкой к ConEmu.exe
    *pszSlash = 0;
    SetEnvironmentVariable(L"ConEmuDir", ms_ConEmuExe);
    *pszSlash = L'\\';

	ms_ConEmuCExe[0] = 0;
	if (ms_ConEmuExe[0] == L'\\' || wcschr(ms_ConEmuExe, L' ') == NULL) // Сетевые пути не менять
		wcscpy(ms_ConEmuCExe, ms_ConEmuExe);
	else {
		wchar_t* pszShort = GetShortFileNameEx(ms_ConEmuExe);
		if (pszShort) {
			wcscpy(ms_ConEmuCExe, pszShort);
			free(pszShort);
		} else {
			wcscpy(ms_ConEmuCExe, ms_ConEmuExe);
		}
	}
	pszSlash = wcsrchr(ms_ConEmuCExe, L'\\');
	if (pszSlash) pszSlash++; else pszSlash = ms_ConEmuCExe;
	wcscpy(pszSlash, L"ConEmuC.exe");
    
    // Запомнить текущую папку (на момент запуска)
    DWORD nDirLen = GetCurrentDirectory(MAX_PATH, ms_ConEmuCurDir);
    if (!nDirLen || nDirLen>MAX_PATH)
    	ms_ConEmuCurDir[0] = 0;

    mh_WinHook = NULL;
	//mh_PopupHook = NULL;
    mp_TaskBar = NULL;

    pVCon = NULL;
    memset(mp_VCon, 0, sizeof(mp_VCon));
    
	UINT nAppMsg = WM_APP+10;
    mn_MsgPostCreate = ++nAppMsg;
    mn_MsgPostCopy = ++nAppMsg;
    mn_MsgMyDestroy = ++nAppMsg;
    mn_MsgUpdateSizes = ++nAppMsg;
    mn_MsgSetWindowMode = ++nAppMsg;
    mn_MsgUpdateTitle = ++nAppMsg;
    mn_MsgAttach = RegisterWindowMessage(CONEMUMSG_ATTACH);
    mn_MsgVConTerminated = ++nAppMsg;
    mn_MsgUpdateScrollInfo = ++nAppMsg;
    mn_MsgUpdateTabs = RegisterWindowMessage(CONEMUMSG_UPDATETABS);
    mn_MsgOldCmdVer = ++nAppMsg; mb_InShowOldCmdVersion = FALSE;
    mn_MsgTabCommand = ++nAppMsg;
    mn_MsgSheelHook = RegisterWindowMessage(L"SHELLHOOK");
	mn_ShellExecuteEx = ++nAppMsg;
}

BOOL CConEmuMain::Init()
{
    //#pragma message("Win2k: EVENT_CONSOLE_START_APPLICATION, EVENT_CONSOLE_END_APPLICATION")
    //Нас интересуют только START и END. Все остальные события приходят от ConEmuC через серверный пайп
    //#if defined(__GNUC__)
    //HMODULE hUser32 = GetModuleHandle(L"user32.dll");
    //FSetWinEventHook SetWinEventHook = (FSetWinEventHook)GetProcAddress(hUser32, "SetWinEventHook");
    //#endif
    mh_WinHook = SetWinEventHook(EVENT_CONSOLE_START_APPLICATION/*EVENT_CONSOLE_CARET*/,EVENT_CONSOLE_END_APPLICATION,
        NULL, (WINEVENTPROC)CConEmuMain::WinEventProc, 0,0, WINEVENT_OUTOFCONTEXT);
	
    //mh_PopupHook = SetWinEventHook(EVENT_SYSTEM_MENUPOPUPSTART,EVENT_SYSTEM_MENUPOPUPSTART,
    //    NULL, (WINEVENTPROC)CConEmuMain::WinEventProc, 0,0, WINEVENT_OUTOFCONTEXT);

    /*mh_Psapi = LoadLibrary(_T("psapi.dll"));
    if (mh_Psapi) {
        GetModuleFileNameEx = (FGetModuleFileNameEx)GetProcAddress(mh_Psapi, "GetModuleFileNameExW");
        if (GetModuleFileNameEx)
            return TRUE;
    }*/

    /*DWORD dwErr = GetLastError();
    TCHAR szErr[255];
    wsprintf(szErr, _T("Can't initialize psapi!\r\nLastError = 0x%08x"), dwErr);
    MBoxA(szErr);
    return FALSE;*/

    return TRUE;
}

BOOL CConEmuMain::CreateMainWindow()
{
	if (!Init())
		return FALSE; // Ошибка уже показана

	if (_tcscmp(VirtualConsoleClass,VirtualConsoleClassMain)) {
		MBoxA(_T("Error: class names must be equal!"));
		return FALSE;
	}

	// 2009-06-11 Возможно, что CS_SAVEBITS приводит к глюкам отрисовки
	// банально непрорисовываются некоторые части экрана (драйвер видюхи глючит?)
	WNDCLASSEX wc = {sizeof(WNDCLASSEX), CS_DBLCLKS|CS_OWNDC/*|CS_SAVEBITS*/, CConEmuMain::MainWndProc, 0, 16, 
		g_hInstance, hClassIcon, LoadCursor(NULL, IDC_ARROW), 
		NULL /*(HBRUSH)COLOR_BACKGROUND*/, 
		NULL, szClassNameParent, hClassIconSm};// | CS_DROPSHADOW
	if (!RegisterClassEx(&wc))
		return -1;

	DWORD style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	if (ghWndApp)
		style |= WS_POPUPWINDOW | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
	else
		style |= WS_OVERLAPPEDWINDOW;
	int nWidth=CW_USEDEFAULT, nHeight=CW_USEDEFAULT;

	// Расчет размеров окна в Normal режиме
	if (gSet.wndWidth && gSet.wndHeight)
	{
		MBoxAssert(gSet.FontWidth() && gSet.FontHeight());

		COORD conSize; conSize.X=gSet.wndWidth; conSize.Y=gSet.wndHeight;
		int nShiftX = GetSystemMetrics(SM_CXSIZEFRAME)*2;
		int nShiftY = GetSystemMetrics(SM_CYSIZEFRAME)*2 + GetSystemMetrics(SM_CYCAPTION);
		// Если табы показываются всегда - сразу добавим их размер, чтобы размер консоли был заказанным
		nWidth  = conSize.X * gSet.FontWidth() + nShiftX 
			+ ((gSet.isTabs == 1) ? (gSet.rcTabMargins.left+gSet.rcTabMargins.right) : 0);
		nHeight = conSize.Y * gSet.FontHeight() + nShiftY 
			+ ((gSet.isTabs == 1) ? (gSet.rcTabMargins.top+gSet.rcTabMargins.bottom) : 0);

		mrc_Ideal = MakeRect(gSet.wndX, gSet.wndY, gSet.wndX+nWidth, gSet.wndY+nHeight);
	}

	//if (gConEmu.WindowMode == rMaximized) style |= WS_MAXIMIZE;
	//style |= WS_VISIBLE;
	// cRect.right - cRect.left - 4, cRect.bottom - cRect.top - 4; -- все равно это было не правильно
	ghWnd = CreateWindow(szClassNameParent, gSet.GetCmd(), style, 
		gSet.wndX, gSet.wndY, nWidth, nHeight, ghWndApp, NULL, (HINSTANCE)g_hInstance, NULL);
	if (!ghWnd) {
		if (!ghWndDC) MBoxA(_T("Can't create main window!"));
		return FALSE;
	}
	//if (gConEmu.WindowMode == rFullScreen || gConEmu.WindowMode == rMaximized)
	//	gConEmu.SetWindowMode(gConEmu.WindowMode);
	return TRUE;
}

void CConEmuMain::Destroy()
{
    for (int i=0; i<MAX_CONSOLE_COUNT; i++) {
        if (mp_VCon[i]) {
            mp_VCon[i]->RCon()->StopSignal();
        }
    }

    if (ghWnd) {
        //HWND hWnd = ghWnd;
        //ghWnd = NULL;
        //DestroyWindow(hWnd); -- может быть вызывано из другой нити
        PostMessage(ghWnd, mn_MsgMyDestroy, GetCurrentThreadId(), 0);
    }
}

CConEmuMain::~CConEmuMain()
{
    for (int i=0; i<MAX_CONSOLE_COUNT; i++) {
        if (mp_VCon[i]) {
			CVirtualConsole* p = mp_VCon[i];
			mp_VCon[i] = NULL;
            delete p;
        }
    }

    /*if (mh_ConMan && mh_ConMan!=INVALID_HANDLE_VALUE)
        FreeLibrary(mh_ConMan);
    mh_ConMan = NULL;*/

    if (mh_WinHook) {
        UnhookWinEvent(mh_WinHook);
        mh_WinHook = NULL;
    }
	//if (mh_PopupHook) {
	//	UnhookWinEvent(mh_PopupHook);
	//	mh_PopupHook = NULL;
	//}

    if (mp_DragDrop) {
        delete mp_DragDrop;
        mp_DragDrop = NULL;
    }
    if (ProgressBars) {
        delete ProgressBars;
        ProgressBars = NULL;
    }
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
            //RECT cRect, wRect;
            //if (!ghWnd || IsIconic(ghWnd)) {
                rc.left = rc.right = GetSystemMetrics(SM_CXSIZEFRAME);
                rc.bottom = GetSystemMetrics(SM_CYSIZEFRAME);
                rc.top = rc.bottom + GetSystemMetrics(SM_CYCAPTION);
            /*} else {
                GetClientRect(ghWnd, &cRect); // The left and top members are zero. The right and bottom members contain the width and height of the window.
                MapWindowPoints(ghWnd, NULL, (LPPOINT)&cRect, 2);
                GetWindowRect(ghWnd, &wRect); // screen coordinates of the upper-left and lower-right corners of the window
                rc.top = cRect.top - wRect.top;          // >0
                rc.left = cRect.left - wRect.left;       // >0
                rc.bottom = wRect.bottom - cRect.bottom; // >0
                rc.right = wRect.right - cRect.right;    // >0
            }*/
        }   break;
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
        }   break;
        //case CEM_BACK:  // Отступы от краев окна фона (с прокруткой) до окна с отрисовкой (DC)
        //{
        //    if (gConEmu.pVCon && gConEmu.pVCon->RCon()->isBufferHeight()) { //TODO: а показывается ли сейчас прокрутка?
        //        rc = MakeRect(0,0,GetSystemMetrics(SM_CXVSCROLL),0);
        //    } else {
        //        rc = MakeRect(0,0); // раз прокрутки нет - значит дополнительные отступы не нужны
        //    }
        //}   break;
    default:
        _ASSERTE(mg==CEM_FRAME || mg==CEM_TAB);
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
            // Нужно отнять отступы рамки и заголовка!
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
                        //rcShift = CalcMargins(CEM_BACK);
                        //AddMargins(rc, rcShift, TRUE/*abExpand*/);
                        rcShift = CalcMargins(CEM_TAB);
                        AddMargins(rc, rcShift, TRUE/*abExpand*/);
                        rcShift = CalcMargins(CEM_FRAME);
                        AddMargins(rc, rcShift, TRUE/*abExpand*/);
                    } break;
                    case CER_MAINCLIENT:
                    {
                        //rcShift = CalcMargins(CEM_BACK);
                        //AddMargins(rc, rcShift, TRUE/*abExpand*/);
                        rcShift = CalcMargins(CEM_TAB);
                        AddMargins(rc, rcShift, TRUE/*abExpand*/);
                    } break;
                    case CER_TAB:
                    {
                        //rcShift = CalcMargins(CEM_BACK);
                        //AddMargins(rc, rcShift, TRUE/*abExpand*/);
                        rcShift = CalcMargins(CEM_TAB);
                        AddMargins(rc, rcShift, TRUE/*abExpand*/);
                    } break;
                    case CER_BACK:
                    {
                        //rcShift = CalcMargins(CEM_BACK);
                        //AddMargins(rc, rcShift, TRUE/*abExpand*/);
                    } break;
                    default:
                        break;
                }
            }
            return rc;
        case CER_CONSOLE:
            {   // Размер консоли в символах!
                MBoxAssert(!(rFrom.left || rFrom.top));
                MBoxAssert(tWhat!=CER_CONSOLE);
                
                if (gSet.FontWidth()==0) {
                    MBoxAssert(gConEmu.pVCon!=NULL);
                    gConEmu.pVCon->InitDC(false, true); // инициализировать ширину шрифта по умолчанию
                }
                
                // ЭТО размер окна отрисовки DC
                rc = MakeRect(rFrom.right * gSet.FontWidth(), rFrom.bottom * gSet.FontHeight());
                
                if (tWhat != CER_DC)
                    rc = CalcRect(tWhat, rc, CER_DC);
            }
            return rc;
        case CER_FULLSCREEN:
        case CER_MAXIMIZED:
            break;
        default:
            break;
    };

    RECT rcAddShift = MakeRect(0,0);
        
    if (prDC) {
        // Если передали реальный размер окна отрисовки - нужно посчитать дополнительные сдвиги
        RECT rcCalcDC = CalcRect(CER_DC, rFrom, CER_MAINCLIENT, NULL /*prDC*/);
        // расчетный НЕ ДОЛЖЕН быть меньше переданного
        #ifdef MSGLOGGER
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
			// Учесть высоту закладок (табов)
            rcShift = CalcMargins(CEM_TAB);
            AddMargins(rc, rcShift);

			// Для корректного деления на размер знакоместа...
            if (gSet.FontWidth()==0 || gSet.FontHeight()==0)
                gConEmu.pVCon->InitDC(false, true); // инициализировать ширину шрифта по умолчанию
			//rc.right ++;
			//int nShift = (gSet.FontWidth() - 1) / 2; if (nShift < 1) nShift = 1;
			//rc.right += nShift;
			// Если есть вкладки
			//if (rcShift.top || rcShift.bottom || )
			//nShift = (gSet.FontWidth() - 1) / 2; if (nShift < 1) nShift = 1;

            if (gConEmu.isNtvdm()) {
                // NTVDM устанавливает ВЫСОТУ экранного буфера... в 25/28/43/50 строк
                // путем округления текущей высоты (то есть если до запуска 16bit
                // было 27 строк, то скорее всего будет установлена высота в 28 строк)
                RECT rc1 = MakeRect(gConEmu.pVCon->TextWidth*gSet.FontWidth(), gConEmu.pVCon->TextHeight*gSet.FontHeight());
                	//gSet.ntvdmHeight/*pVCon->TextHeight*/*gSet.FontHeight());
                if (rc1.bottom > (rc.bottom - rc.top))
                	rc1.bottom = (rc.bottom - rc.top); // Если размер вылез за текущий - обрежем снизу :(
                    
                int nS = rc.right - rc.left - rc1.right;
                if (nS>=0) {
                    rcShift.left = nS / 2;
                    rcShift.right = nS - rcShift.left;
                } else {
                    rcShift.left = 0;
                    rcShift.right = -nS;
                }
                nS = rc.bottom - rc.top - rc1.bottom;
                if (nS>=0) {
                    rcShift.top = nS / 2;
                    rcShift.bottom = nS - rcShift.top;
                } else {
                    rcShift.top = 0;
                    rcShift.bottom = -nS;
                }
                AddMargins(rc, rcShift);
                //rc = rc1;
            }
                
            // Если нужен размер консоли в символах сразу делим и выходим
            if (tWhat == CER_CONSOLE) {
                //MBoxAssert((gSet.Log Font.lfWidth!=0) && (gSet.Log Font.lfHeight!=0));
                
				//2009-07-09 - ClientToConsole использовать нельзя, т.к. после его
				//  приближений высота может получиться больше Ideal, а ширина - меньше
                //COORD cr = gConEmu.pVCon->ClientToConsole( (rc.right - rc.left), (rc.bottom - rc.top) );
                //rc.right = (rc.right - rc.left) / gSet.Log Font.lfWidth;
                //rc.bottom = (rc.bottom - rc.top) / gSet.Log Font.lfHeight;
                //rc.right = cr.X; rc.bottom = cr.Y;
				rc.right = (rc.right - rc.left + 1) / gSet.FontWidth();
				rc.bottom = (rc.bottom - rc.top) / gSet.FontHeight();
                rc.left = 0; rc.top = 0;

#ifdef _DEBUG
                _ASSERT(rc.bottom>=5);
#endif
                
                return rc;
            }
        } break;
        case CER_FULLSCREEN:
        case CER_MAXIMIZED:
        case CER_CORRECTED:
        {
            HMONITOR hMonitor = NULL;

            if (ghWnd)
                hMonitor = MonitorFromWindow ( ghWnd, MONITOR_DEFAULTTOPRIMARY );
            else
                hMonitor = MonitorFromRect ( &rFrom, MONITOR_DEFAULTTOPRIMARY );

            if (tWhat != CER_CORRECTED)
                tFrom = tWhat;

            MONITORINFO mi; mi.cbSize = sizeof(mi);
            if (GetMonitorInfo(hMonitor, &mi)) {
                switch (tFrom)
                {
                case CER_FULLSCREEN:
                    rc = mi.rcMonitor;
                    break;
                case CER_MAXIMIZED:
                    rc = mi.rcWork;
                    break;
                default:
                    _ASSERTE(tFrom==CER_FULLSCREEN || tFrom==CER_MAXIMIZED);
                }
            } else {
                switch (tFrom)
                {
                case CER_FULLSCREEN:
                    rc = MakeRect(GetSystemMetrics(SM_CXFULLSCREEN),GetSystemMetrics(SM_CYFULLSCREEN));
                    break;
                case CER_MAXIMIZED:
                    rc = MakeRect(GetSystemMetrics(SM_CXMAXIMIZED),GetSystemMetrics(SM_CYMAXIMIZED));
                    break;
                default:
                    _ASSERTE(tFrom==CER_FULLSCREEN || tFrom==CER_MAXIMIZED);
                }
            }

            if (tWhat == CER_CORRECTED)
            {
                RECT rcMon = rc;
                rc = rFrom;
                int nX = GetSystemMetrics(SM_CXSIZEFRAME), nY = GetSystemMetrics(SM_CYSIZEFRAME);
                int nWidth = rc.right-rc.left;
                int nHeight = rc.bottom-rc.top;
                static bool bFirstCall = true;
                if (bFirstCall) {
                    if (gSet.wndCascade) {
                        BOOL lbX = ((rc.left+nWidth)>(rcMon.right+nX));
                        BOOL lbY = ((rc.top+nHeight)>(rcMon.bottom+nY));
                        {
                            if (lbX && lbY) {
                                rc = MakeRect(rcMon.left,rcMon.top,rcMon.left+nWidth,rcMon.top+nHeight);
                            } else if (lbX) {
                                rc.left = rcMon.right - nWidth; rc.right = rcMon.right;
                            } else if (lbY) {
                                rc.top = rcMon.bottom - nHeight; rc.bottom = rcMon.bottom;
                            }
                        }
                    }
                    bFirstCall = false;
                }
                if (rc.left<(rcMon.left-nX)) { rc.left=rcMon.left-nX; rc.right=rc.left+nWidth; }
                if (rc.top<(rcMon.top-nX)) { rc.top=rcMon.top-nX; rc.bottom=rc.top+nHeight; }
                if ((rc.left+nWidth)>(rcMon.right+nX)) {
                    rc.left = max((rcMon.left-nX),(rcMon.right-nWidth));
                    nWidth = min(nWidth, (rcMon.right-rc.left+2*nX));
                    rc.right = rc.left + nWidth;
                }
                if ((rc.top+nHeight)>(rcMon.bottom+nY)) {
                    rc.top = max((rcMon.top-nY),(rcMon.bottom-nHeight));
                    nHeight = min(nHeight, (rcMon.bottom-rc.top+2*nY));
                    rc.bottom = rc.top + nHeight;
                }
            }

            return rc;
        } break;
        default:
            break;
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
//  if (TabBar.IsActive())
//      rect = TabBar.GetMargins();
//
//  /*rect.top = TabBar.IsActive()?TabBar.Height():0;
//    rect.left = 0;
//    rect.bottom = 0;
//    rect.right = 0;*/
//
//  return rect;
//}

//// Положение дочернего окна отрисовки
//RECT CConEmuMain::DCClientRect(RECT* pClient/*=NULL*/)
//{
//    RECT rect;
//  if (pClient)
//      rect = *pClient;
//  else
//      GetClientRect(ghWnd, &rect);
//  if (TabBar.IsActive()) {
//      RECT mr = TabBar.GetMargins();
//      //rect.top += TabBar.Height();
//      rect.top += mr.top;
//      rect.left += mr.left;
//      rect.right -= mr.right;
//      rect.bottom -= mr.bottom;
//  }
//
//  if (pClient)
//      *pClient = rect;
//    return rect;
//}

//// returns console size in columns and lines calculated from current window size
//// rectInWindow - если true - с рамкой, false - только клиент
//COORD CConEmuMain::ConsoleSizeFromWindow(RECT* arect /*= NULL*/, bool frameIncluded /*= false*/, bool alreadyClient /*= false*/)
//{
//    COORD size;
//
//  if (!gSet.Log Font.lfWidth || !gSet.Log Font.lfHeight) {
//      MBoxAssert(FALSE);
//      // размер шрифта еще не инициализирован! вернем текущий размер консоли! TODO:
//      CONSOLE_SCREEN_BUFFER_INFO inf; memset(&inf, 0, sizeof(inf));
//      GetConsoleScreenBufferInfo(pVCon->hConOut(), &inf);
//      size = inf.dwSize;
//      return size; 
//  }
//
//    RECT rect, consoleRect;
//    if (arect == NULL)
//    {
//      frameIncluded = false;
//        GetClientRect(ghWnd, &rect);
//      consoleRect = ConsoleOffsetRect();
//    } 
//    else
//    {
//        rect = *arect;
//      if (alreadyClient)
//          memset(&consoleRect, 0, sizeof(consoleRect));
//      else
//          consoleRect = ConsoleOffsetRect();
//    }
//    
//    size.X = (rect.right - rect.left - (frameIncluded ? cwShift.x : 0) - consoleRect.left - consoleRect.right)
//      / gSet.LogFont.lfWidth;
//    size.Y = (rect.bottom - rect.top - (frameIncluded ? cwShift.y : 0) - consoleRect.top - consoleRect.bottom)
//      / gSet.LogFont.lfHeight;
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
//  if (clientOnly)
//      memset(&offsetRect, 0, sizeof(RECT));
//  else
//      offsetRect = ConsoleOffsetRect();
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
    // Это не совсем корректно... ntvdm.exe не выгружается после выхода из 16бит приложения
    if (isNtvdm()) {
        //if (size.X == 80 && size.Y>25 && lastSize1.X != size.X && size.Y == lastSize1.Y) {
            TODO("Ntvdm почему-то не всегда устанавливает ВЫСОТУ консоли в 25/28/50 символов...")
        //}
        return; // запрет изменения размеров консоли для 16бит приложений 
    }

    #ifdef MSGLOGGER
        char szDbg[100]; wsprintfA(szDbg, "SetConsoleWindowSize({%i,%i},%i)\n", size.X, size.Y, updateInfo);
        DEBUGLOGFILE(szDbg);
    #endif

    m_LastConSize = size;

    if (isPictureView()) {
        isPiewUpdate = true;
        return;
    }

    // update size info
    // !!! Это вроде делает консоль
    WARNING("updateInfo убить");
    /*if (updateInfo && !gSet.isFullScreen && !IsZoomed(ghWnd) && !IsIconic(ghWnd))
    {
        gSet.UpdateSize(size.X, size.Y);
    }*/

    RECT rcCon = MakeRect(size.X,size.Y);
    if (pVCon) {
        if (!pVCon->RCon()->SetConsoleSize(size))
            rcCon = MakeRect(pVCon->TextWidth,pVCon->TextHeight);
    }
    RECT rcWnd = CalcRect(CER_MAIN, rcCon, CER_CONSOLE);

    RECT wndR; GetWindowRect(ghWnd, &wndR); // текущий XY

    MOVEWINDOW ( ghWnd, wndR.left, wndR.top, rcWnd.right, rcWnd.bottom, 1);
}

// Изменить размер консоли по размеру окна (главного)
void CConEmuMain::SyncConsoleToWindow()
{
    if (mb_SkipSyncSize || isNtvdm() || !pVCon)
        return;

    #ifdef _DEBUG
    if (change2WindowMode!=(DWORD)-1) {
        _ASSERTE(change2WindowMode==(DWORD)-1);
    }
    #endif

    pVCon->RCon()->SyncConsole2Window();

    /*
    DEBUGLOGFILE("SyncConsoleToWindow\n");

    RECT rcClient; GetClientRect(ghWnd, &rcClient);

    // Посчитать нужный размер консоли
    RECT newCon = CalcRect(CER_CONSOLE, rcClient, CER_MAINCLIENT);

    pVCon->SetConsoleSize(MakeCoord(newCon.right, newCon.bottom));
    */

    /*if (!IsZoomed(ghWnd) && !IsIconic(ghWnd) && !gSet.isFullScreen)
        gSet.UpdateSize(pVCon->TextWidth, pVCon->TextHeight);*/

    /*
    // Посчитать нужный размер консоли
    //COORD newConSize = ConsoleSizeFromWindow();
    // Получить текущий размер консольного окна
    CONSOLE_SCREEN_BUFFER_INFO inf; memset(&inf, 0, sizeof(inf));
    //GetConsoleScreenBufferInfo(pVCon->hConOut(), &inf);

    // Если нужно менять - ...
    if (newCon.right != (inf.srWindow.Right-inf.srWindow.Left+1) ||
        newCon.bottom != (inf.srWindow.Bottom-inf.srWindow.Top+1))
    {
        SetConsoleWindowSize(MakeCoord(newCon.right,newCon.bottom), true);
        if (pVCon)
            pVCon->InitDC(false);
    }
    */
}

void CConEmuMain::SyncNtvdm()
{
    //COORD sz = {pVCon->TextWidth, pVCon->TextHeight};
    //SetConsoleWindowSize(sz, false);

    OnSize();
}

// Установить размер основного окна по текущему размеру pVCon
void CConEmuMain::SyncWindowToConsole()
{
    DEBUGLOGFILE("SyncWindowToConsole\n");

    if (mb_SkipSyncSize || !pVCon)
        return;

    #ifdef _DEBUG
    _ASSERT(GetCurrentThreadId() == mn_MainThreadId);
    #endif

    RECT rcDC = pVCon->GetRect();
        /*MakeRect(pVCon->Width, pVCon->Height);
    if (pVCon->Width == 0 || pVCon->Height == 0) {
        rcDC = MakeRect(pVCon->winSize.X, pVCon->winSize.Y);
    }*/

    //_ASSERT(rcDC.right>250 && rcDC.bottom>200);

    RECT rcWnd = CalcRect(CER_MAIN, rcDC, CER_DC); // размеры окна
    
    //GetCWShift(ghWnd, &cwShift);
    
    RECT wndR; GetWindowRect(ghWnd, &wndR); // текущий XY

    gSet.UpdateSize(pVCon->TextWidth, pVCon->TextHeight);

    MOVEWINDOW ( ghWnd, wndR.left, wndR.top, rcWnd.right, rcWnd.bottom, 1);
    
    //RECT consoleRect = ConsoleOffsetRect();

    //#ifdef MSGLOGGER
    //    char szDbg[100]; wsprintfA(szDbg, "   pVCon:Size={%i,%i}\n", pVCon->Width,pVCon->Height);
    //    DEBUGLOGFILE(szDbg);
    //#endif
    //
    //MOVEWINDOW ( ghWnd, wndR.left, wndR.top, pVCon->Width + cwShift.x + consoleRect.left + consoleRect.right, pVCon->Height + cwShift.y + consoleRect.top + consoleRect.bottom, 1);
}

bool CConEmuMain::SetWindowMode(uint inMode)
{
    if (!isMainThread()) {
        PostMessage(ghWnd, mn_MsgSetWindowMode, inMode, 0);
        return false;
    }

    //2009-04-22 Если открыт PictureView - лучше не дергаться...
    if (isPictureView())
        return false;

    SetCursor(LoadCursor(NULL,IDC_WAIT));

    mb_PassSysCommand = true;

    //WindowPlacement -- использовать нельзя, т.к. он работает в координатах Workspace, а не Screen!
    RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);
    RECT consoleSize = MakeRect(gSet.wndWidth, gSet.wndHeight);
    bool canEditWindowSizes = false;
    bool lbRc = false;

    change2WindowMode = inMode;

    //!!!
    switch(inMode)
    {
    case rNormal:
        {
            DEBUGLOGFILE("SetWindowMode(rNormal)\n");

			// Расчитать размер по оптимальному WindowRect
			RECT rcCon = CalcRect(CER_CONSOLE, mrc_Ideal, CER_MAIN);
			if (!rcCon.right || !rcCon.bottom) { rcCon.right = gSet.wndWidth; rcCon.bottom = gSet.wndHeight; }
            if (pVCon && !pVCon->RCon()->SetConsoleSize(MakeCoord(rcCon.right, rcCon.bottom))) {
                mb_PassSysCommand = false;
                goto wrap;
            }

            if (IsIconic(ghWnd) || IsZoomed(ghWnd)) {
                //ShowWindow(ghWnd, SW_SHOWNORMAL); // WM_SYSCOMMAND использовать не хочется...
                mb_IgnoreSizeChange = TRUE;
                if (IsWindowVisible(ghWnd))
                    DefWindowProc(ghWnd, WM_SYSCOMMAND, SC_RESTORE, 0); //2009-04-22 Было SendMessage
                else
                    ShowWindow(ghWnd, SW_SHOWNORMAL);
                //RePaint();
                mb_IgnoreSizeChange = FALSE;
                OnSize(-1); // подровнять ТОЛЬКО дочерние окошки
            }

            RECT rcNew = CalcRect(CER_MAIN, consoleSize, CER_CONSOLE);
            //int nWidth = rcNew.right-rcNew.left;
            //int nHeight = rcNew.bottom-rcNew.top;
            rcNew.left+=gSet.wndX; rcNew.top+=gSet.wndY;
            rcNew.right+=gSet.wndX; rcNew.bottom+=gSet.wndY;

            rcNew = CalcRect(CER_CORRECTED, rcNew, CER_MAXIMIZED);

            #ifdef _DEBUG
            WINDOWPLACEMENT wpl; memset(&wpl,0,sizeof(wpl)); wpl.length = sizeof(wpl);
            GetWindowPlacement(ghWnd, &wpl);
            #endif
            SetWindowPos(ghWnd, NULL, rcNew.left, rcNew.top, rcNew.right-rcNew.left, rcNew.bottom-rcNew.top, SWP_NOZORDER);
            #ifdef _DEBUG
            GetWindowPlacement(ghWnd, &wpl);
            #endif

            if (ghOpWnd)
                CheckRadioButton(gSet.hMain, rNormal, rFullScreen, rNormal);
            gSet.isFullScreen = false;

            if (!IsWindowVisible(ghWnd))
                ShowWindow(ghWnd, SW_SHOWNORMAL);
            #ifdef _DEBUG
            GetWindowPlacement(ghWnd, &wpl);
            #endif
            // Если это во время загрузки - то до первого ShowWindow - IsIconic возвращает FALSE
            if (IsIconic(ghWnd) || IsZoomed(ghWnd))
                ShowWindow(ghWnd, SW_SHOWNORMAL); // WM_SYSCOMMAND использовать не хочется...
            #ifdef _DEBUG
            GetWindowPlacement(ghWnd, &wpl);
            UpdateWindow(ghWnd);
            #endif
        } break;
    case rMaximized:
        {
            DEBUGLOGFILE("SetWindowMode(rMaximized)\n");

            // Обновить коордианты в gSet, если требуется
            if (!gSet.isFullScreen && !IsZoomed(ghWnd) && !IsIconic(ghWnd))
            {
                gSet.UpdatePos(rcWnd.left, rcWnd.top);
                if (pVCon)
                    gSet.UpdateSize(pVCon->TextWidth, pVCon->TextHeight);
            }

            RECT rcMax = CalcRect(CER_MAXIMIZED, MakeRect(0,0), CER_MAXIMIZED);
            rcMax.left -= GetSystemMetrics(SM_CXSIZEFRAME);
            rcMax.right += GetSystemMetrics(SM_CXSIZEFRAME);
            rcMax.top -= GetSystemMetrics(SM_CYSIZEFRAME);
            rcMax.bottom += GetSystemMetrics(SM_CYSIZEFRAME);
            RECT rcCon = CalcRect(CER_CONSOLE, rcMax, CER_MAIN);
            if (pVCon && !pVCon->RCon()->SetConsoleSize(MakeCoord(rcCon.right,rcCon.bottom))) {
                mb_PassSysCommand = false;
                goto wrap;
            }

            if (!IsZoomed(ghWnd)) {
                mb_IgnoreSizeChange = TRUE;
                InvalidateAll();
                ShowWindow(ghWnd, SW_SHOWMAXIMIZED);
                mb_IgnoreSizeChange = FALSE;
                RePaint();
                OnSize(-1); // консоль уже изменила свой размер
            }

            if (ghOpWnd)
                CheckRadioButton(gSet.hMain, rNormal, rFullScreen, rMaximized);
            gSet.isFullScreen = false;

            if (!IsWindowVisible(ghWnd)) {
                mb_IgnoreSizeChange = TRUE;
                ShowWindow(ghWnd, SW_SHOWMAXIMIZED);
                mb_IgnoreSizeChange = FALSE;
                OnSize(-1); // консоль уже изменила свой размер
            }
        } break;

    case rFullScreen:
        DEBUGLOGFILE("SetWindowMode(rFullScreen)\n");
        if (!gSet.isFullScreen)
        {
            // Обновить коордианты в gSet, если требуется
            if (!gSet.isFullScreen && !IsZoomed(ghWnd) && !IsIconic(ghWnd))
            {
                gSet.UpdatePos(rcWnd.left, rcWnd.top);
                if (pVCon)
                    gSet.UpdateSize(pVCon->TextWidth, pVCon->TextHeight);
            }

            RECT rcMax = CalcRect(CER_FULLSCREEN, MakeRect(0,0), CER_FULLSCREEN);
            RECT rcCon = CalcRect(CER_CONSOLE, rcMax, CER_MAINCLIENT);
            if (pVCon && !pVCon->RCon()->SetConsoleSize(MakeCoord(rcCon.right,rcCon.bottom))) {
                mb_PassSysCommand = false;
                goto wrap;
            }

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

            if (IsIconic(ghWnd) || IsZoomed(ghWnd)) {
                mb_IgnoreSizeChange = TRUE;
                ShowWindow(ghWnd, SW_SHOWNORMAL);
                mb_IgnoreSizeChange = FALSE;
                RePaint();
            }

            // for virtual screend mi.rcMonitor. may contains negative values...

            /* */ SetWindowPos(ghWnd, NULL,
                -rcShift.left+mi.rcMonitor.left,-rcShift.top+mi.rcMonitor.top,
                ptFullScreenSize.x,ptFullScreenSize.y,
                SWP_NOZORDER);

            if (ghOpWnd)
                CheckRadioButton(gSet.hMain, rNormal, rFullScreen, rFullScreen);
        }
        if (!IsWindowVisible(ghWnd)) {
            mb_IgnoreSizeChange = TRUE;
            ShowWindow(ghWnd, SW_SHOWNORMAL);
            mb_IgnoreSizeChange = FALSE;
            OnSize(-1);  // консоль уже изменила свой размер
        }
        break;
    }
    
    WindowMode = inMode; // Запомним!

    canEditWindowSizes = inMode == rNormal;
    if (ghOpWnd)
    {
        EnableWindow(GetDlgItem(ghOpWnd, tWndWidth), canEditWindowSizes);
        EnableWindow(GetDlgItem(ghOpWnd, tWndHeight), canEditWindowSizes);
    }
    //SyncConsoleToWindow(); 2009-09-10 А это вроде вообще не нужно - ресайз консоли уже сделан
    mb_PassSysCommand = false;
    lbRc = true;
wrap:
    change2WindowMode = -1;
    return lbRc;
}

void CConEmuMain::ForceShowTabs(BOOL abShow)
{
    //if (!pVCon)
    //  return;

    //2009-05-20 Раз это Force - значит на возможность получить табы из фара забиваем! Для консоли показывается "Console"
    BOOL lbTabsAllowed = abShow /*&& TabBar.IsAllowed()*/;

    if (abShow && !TabBar.IsShown() && gSet.isTabs && lbTabsAllowed)
    {
        TabBar.Activate();
        //ConEmuTab tab; memset(&tab, 0, sizeof(tab));
        //tab.Pos=0;
        //tab.Current=1;
        //tab.Type = 1;
        //TabBar.Update(&tab, 1);
        //pVCon->RCon()->SetTabs(&tab, 1);
        TabBar.Update();
        //gbPostUpdateWindowSize = true; // 2009-07-04 Resize выполняет сам TabBar
    } else if (!abShow) {
        TabBar.Deactivate();
        //gbPostUpdateWindowSize = true; // 2009-07-04 Resize выполняет сам TabBar
    }

	// 2009-07-04 Resize выполняет сам TabBar
    //if (gbPostUpdateWindowSize) { // значит мы что-то поменяли
    //    ReSize();
    //    /*RECT rcNewCon; GetClientRect(ghWnd, &rcNewCon);
    //    DCClientRect(&rcNewCon);
    //    MoveWindow(ghWndDC, rcNewCon.left, rcNewCon.top, rcNewCon.right - rcNewCon.left, rcNewCon.bottom - rcNewCon.top, 0);
    //    dcWindowLast = rcNewCon;
    //    
    //    if (gSet.LogFont.lfWidth)
    //    {
    //        SyncConsoleToWindow();
    //    }*/
    //}
}

void CConEmuMain::ReSize(BOOL abCorrect2Ideal /*= FALSE*/)
{
    if (IsIconic(ghWnd))
        return;

	RECT client; GetClientRect(ghWnd, &client);

	if (abCorrect2Ideal) {
		

		if (!IsZoomed(ghWnd) && !gSet.isFullScreen) {
			// Выполняем всегда, даже если размер уже соответсвует...
			RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);
			RECT rcConsole = CalcRect(CER_CONSOLE, mrc_Ideal, CER_MAIN);
			RECT rcCompWnd = CalcRect(CER_MAIN, rcConsole, CER_CONSOLE);
			// При показе/скрытии табов высота консоли может "прыгать"
			// Ее нужно скорректировать. Поскольку идет реальный ресайз
			// главного окна - OnSize вызовается автоматически
			_ASSERTE(isMainThread());

			m_Child.SetRedraw(FALSE);
			pVCon->RCon()->SetConsoleSize(MakeCoord(rcConsole.right, rcConsole.bottom), CECMD_SETSIZESYNC);
			m_Child.SetRedraw(TRUE);
			m_Child.Redraw();

			//#ifdef _DEBUG
			//DnDstep(L"...Sleeping");
			//Sleep(300);
			//DnDstep(NULL);
			//#endif

			MoveWindow(ghWnd, rcWnd.left, rcWnd.top, 
				(rcCompWnd.right - rcCompWnd.left), (rcCompWnd.bottom - rcCompWnd.top), 1);
		} else {
			RECT rcConsole = CalcRect(CER_CONSOLE, client, CER_MAINCLIENT);

			m_Child.SetRedraw(FALSE);
			pVCon->RCon()->SetConsoleSize(MakeCoord(rcConsole.right, rcConsole.bottom), CECMD_SETSIZESYNC);
			m_Child.SetRedraw(TRUE);
			m_Child.Redraw();
		}
	}

    OnSize(IsZoomed(ghWnd) ? SIZE_MAXIMIZED : SIZE_RESTORED,
        client.right, client.bottom);

	if (abCorrect2Ideal) {
		
	}
}

LRESULT CConEmuMain::OnSize(WPARAM wParam, WORD newClientWidth, WORD newClientHeight)
{
    LRESULT result = 0;
    
    #if defined(EXT_GNUC_LOG)
    char szDbg[255];
    wsprintfA(szDbg, "CConEmuMain::OnSize(wParam=%i, Width=%i, Height=%i, "
        "IsIconic=%i, IgnoreSizeChange=%i)\n",
        wParam, newClientWidth, newClientHeight, IsIconic(ghWnd), mb_IgnoreSizeChange);
    pVCon->RCon()->LogString(szDbg);
    #endif

    if (wParam == SIZE_MINIMIZED || IsIconic(ghWnd)) {
        return 0;
    }

    if (mb_IgnoreSizeChange) {
        // на время обработки WM_SYSCOMMAND
        return 0;
    }

    if (mn_MainThreadId != GetCurrentThreadId()) {
        //MBoxAssert(mn_MainThreadId == GetCurrentThreadId());
        PostMessage(ghWnd, WM_SIZE, wParam, MAKELONG(newClientWidth,newClientHeight));
        return 0;
    }

    if (newClientWidth==(WORD)-1 || newClientHeight==(WORD)-1) {
        RECT rcClient; GetClientRect(ghWnd, &rcClient);
        newClientWidth = rcClient.right;
        newClientHeight = rcClient.bottom;
        #if defined(EXT_GNUC_LOG)
        char szDbg[255];
        wsprintfA(szDbg, "  --  RealSize(Width=%i, Height=%i)", newClientWidth, newClientHeight);
        pVCon->RCon()->LogString(szDbg);
        #endif
    } else {
        #if defined(EXT_GNUC_LOG)
        pVCon->RCon()->LogString("  -- normal mode\n");
        #endif
    }

	// Запомнить "идеальный" размер окна, выбранный пользователем
	if (isSizing() && !gSet.isFullScreen && !IsZoomed(ghWnd) && !IsIconic(ghWnd)) {
		GetWindowRect(ghWnd, &mrc_Ideal);
	}

    if (TabBar.IsActive())
        TabBar.UpdateWidth();

    // Background - должен занять все клиентское место под тулбаром
    // Там же ресайзится ScrollBar
    m_Back.Resize();
    
    #ifdef _DEBUG
    BOOL lbIsPicView = 
    #endif
    isPictureView();

    if (wParam != (DWORD)-1 && change2WindowMode == (DWORD)-1) {
        #if defined(EXT_GNUC_LOG)
        pVCon->RCon()->LogString("  --  Executing SyncConsoleToWindow");
        #endif
        SyncConsoleToWindow();
    }
    
    RECT mainClient = MakeRect(newClientWidth,newClientHeight);
    #if defined(EXT_GNUC_LOG)
    wsprintfA(szDbg, "  --  mainClient=(%i,%i,%i,%i)", mainClient.left, mainClient.top, mainClient.right, mainClient.bottom);
    pVCon->RCon()->LogString(szDbg);
    #endif
    //RECT dcSize; GetClientRect(ghWndDC, &dcSize);
    RECT dcSize = CalcRect(CER_DC, mainClient, CER_MAINCLIENT);
    #if defined(EXT_GNUC_LOG)
    if (dcSize.top > 40) {
        pVCon->RCon()->LogString("  **  Error");
    }
    wsprintfA(szDbg, "  --  dcSize=(%i,%i,%i,%i)", dcSize.left, dcSize.top, dcSize.right, dcSize.bottom);
    pVCon->RCon()->LogString(szDbg);
    #endif
    //DCClientRect(&client);
    RECT client = CalcRect(CER_DC, mainClient, CER_MAINCLIENT, &dcSize);
    #if defined(EXT_GNUC_LOG)
    wsprintfA(szDbg, "  --  client=(%i,%i,%i,%i)", client.left, client.top, client.right, client.bottom);
    pVCon->RCon()->LogString(szDbg);
    #endif

    RECT rcNewCon; memset(&rcNewCon,0,sizeof(rcNewCon));
    if (pVCon && pVCon->Width && pVCon->Height) {
        if ((gSet.isTryToCenter && (IsZoomed(ghWnd) || gSet.isFullScreen))
			|| isNtvdm())
        {
            rcNewCon.left = (client.right+client.left-(int)pVCon->Width)/2;
            rcNewCon.top = (client.bottom+client.top-(int)pVCon->Height)/2;
        }

        if (rcNewCon.left<client.left) rcNewCon.left=client.left;
        if (rcNewCon.top<client.top) rcNewCon.top=client.top;

        rcNewCon.right = rcNewCon.left + pVCon->Width;
        rcNewCon.bottom = rcNewCon.top + pVCon->Height;
        
        if (rcNewCon.right>client.right) rcNewCon.right=client.right;
        if (rcNewCon.bottom>client.bottom) rcNewCon.bottom=client.bottom;
        
        #if defined(EXT_GNUC_LOG)
        wsprintfA(szDbg, "  --  rcNewCon=(%i,%i,%i,%i) pVCon=(%ix%i) client=(%i,%i,%i,%i)", 
            rcNewCon.left, rcNewCon.top, rcNewCon.right, rcNewCon.bottom, pVCon->Width, pVCon->Height,
            client.left, client.top, client.right, client.bottom);
        pVCon->RCon()->LogString(szDbg);
        #endif
    } else {
        rcNewCon = client;
        #if defined(EXT_GNUC_LOG)
        wsprintfA(szDbg, "  --  rcNewCon=client(%i,%i,%i,%i)", rcNewCon.left, rcNewCon.top, rcNewCon.right, rcNewCon.bottom);
        pVCon->RCon()->LogString(szDbg);
        #endif
    }
    // Двигаем/ресайзим окошко DC
    #if defined(EXT_GNUC_LOG)
    wsprintfA(szDbg, "  --  Moving DC window(X=%i, Y=%i, Width=%i, Height=%i)", rcNewCon.left, rcNewCon.top, rcNewCon.right - rcNewCon.left, rcNewCon.bottom - rcNewCon.top);
    pVCon->RCon()->LogString(szDbg);
    #endif
    MoveWindow(ghWndDC, rcNewCon.left, rcNewCon.top, rcNewCon.right - rcNewCon.left, rcNewCon.bottom - rcNewCon.top, 1);

    return result;
}

LRESULT CConEmuMain::OnSizing(WPARAM wParam, LPARAM lParam)
{
    LRESULT result = true;
    
    #if defined(EXT_GNUC_LOG)
    char szDbg[255];
    wsprintfA(szDbg, "CConEmuMain::OnSizing(wParam=%i, L.Lo=%i, L.Hi=%i)\n",
        wParam, LOWORD(lParam), HIWORD(lParam));
    pVCon->RCon()->LogString(szDbg);
    #endif


    if (isPictureView()) {
        RECT *pRect = (RECT*)lParam; // с рамкой
        *pRect = mrc_WndPosOnPicView;
        //pRect->right = pRect->left + (mrc_WndPosOnPicView.right-mrc_WndPosOnPicView.left);
        //pRect->bottom = pRect->top + (mrc_WndPosOnPicView.bottom-mrc_WndPosOnPicView.top);
    } else
    if (mb_IgnoreSizeChange) {
        // на время обработки WM_SYSCOMMAND
    } else
    if (isNtvdm()) {
        // не менять для 16бит приложений
    } else
    if (!gSet.isFullScreen && !IsZoomed(ghWnd)) {
        RECT srctWindow;
        RECT wndSizeRect, restrictRect;
        RECT *pRect = (RECT*)lParam; // с рамкой

        if ((mouse.state & (MOUSE_SIZING_BEGIN|MOUSE_SIZING_TODO))==MOUSE_SIZING_BEGIN 
            && isPressed(VK_LBUTTON))
        {
            mouse.state |= MOUSE_SIZING_TODO;
        }

        wndSizeRect = *pRect;
        // Для красивости рамки под мышкой
        LONG nWidth = gSet.FontWidth(), nHeight = gSet.FontHeight();
        if (nWidth && nHeight) {
            wndSizeRect.right += (nWidth-1)/2;
            wndSizeRect.bottom += (nHeight-1)/2;
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

BOOL CConEmuMain::Activate(CVirtualConsole* apVCon)
{
    if (!isValid(apVCon))
        return FALSE;
    BOOL lbRc = FALSE;
    for (int i=0; pVCon && i<MAX_CONSOLE_COUNT; i++) {
        if (mp_VCon[i] == apVCon) {
            ConActivate(i);
            lbRc = (pVCon == apVCon);
            break;
        }
    }
    return lbRc;
}

CVirtualConsole* CConEmuMain::ActiveCon()
{
    return pVCon;
    /*if (mn_ActiveCon >= MAX_CONSOLE_COUNT)
        mn_ActiveCon = -1;
    if (mn_ActiveCon < 0)
        return NULL;
    return mp_VCon[mn_ActiveCon];*/
}

// 0 - based
int CConEmuMain::ActiveConNum()
{
    int nActive = -1;
    for (int i=0; pVCon && i<MAX_CONSOLE_COUNT; i++) {
        if (mp_VCon[i] == pVCon) {
            nActive = i; break;
        }
    }
    return nActive;
}

LPARAM CConEmuMain::AttachRequested(HWND ahConWnd, DWORD anConemuC_PID)
{
    int i;
    CVirtualConsole* pCon = NULL;
    // Может быть какой-то VCon ждет аттача?
    for (i = 0; !pCon && i<MAX_CONSOLE_COUNT; i++) {
        if (mp_VCon[i] && mp_VCon[i]->RCon()->isDetached())
            pCon = mp_VCon[i];
    }
    // Если не нашли - определим, можно ли добавить новую консоль?
    if (!pCon) {
		RConStartArgs args; args.bDetached = TRUE;
        if ((pCon = CreateCon(&args)) == NULL)
            return 0;
    }

    // Пытаемся подцепить консоль
    if (!pCon->RCon()->AttachConemuC(ahConWnd, anConemuC_PID))
        return 0;

    // OK
    return (LPARAM)ghWndDC;
}

//void CConEmuMain::CheckProcessName(struct ConProcess &ConPrc, LPCTSTR asFullFileName)
//{
//    ConPrc.IsFar = lstrcmpi(ConPrc.Name, _T("far.exe"))==0;
//
//    ConPrc.IsNtvdm = lstrcmpi(ConPrc.Name, _T("ntvdm.exe"))==0;
//    //mn_ActiveStatus |= CES_NTVDM; - это делает WinHook
//
//    if (ConPrc.IsTelnet = lstrcmpi(ConPrc.Name, _T("telnet.exe"))==0)
//        mn_ActiveStatus |= CES_TELNETACTIVE;
//
//    if (ConPrc.IsConman = lstrcmpi(ConPrc.Name, _T("conman.exe"))==0) {
//        mn_ConmanPID = ConPrc.ProcessID;
//        mn_ActiveStatus |= CES_CONMANACTIVE;
//
//        /*if (mh_Infis==NULL && asFullFileName) {
//            _tcscpy(ms_InfisPath, asFullFileName);
//            TCHAR* pszSlash = _tcsrchr(ms_InfisPath, _T('\\'));
//            if (pszSlash) {
//                pszSlash++;
//                _tcscpy(pszSlash, _T("infis.dll"));
//            } else {
//                if (!SearchPath(NULL, _T("infis.dll"), NULL, MAX_PATH*2, ms_InfisPath, &pszSlash))
//                    _tcscpy(ms_InfisPath, _T("infis.dll"));
//            }
//        }*/
//    }
//    ConPrc.NameChecked = true;
//}

void CConEmuMain::CheckGuiBarsCreated()
{
    if (gSet.isGUIpb && !ProgressBars) {
    	ProgressBars = new CProgressBars(ghWnd, g_hInstance);
    }
}

// Вернуть общее количество процессов по всем консолям
DWORD CConEmuMain::CheckProcesses()
{
    DWORD dwAllCount = 0;
    //mn_ActiveStatus &= ~CES_PROGRAMS;
    for (int j=0; j<MAX_CONSOLE_COUNT; j++) {
        if (mp_VCon[j] == NULL) continue;

        int nCount = mp_VCon[j]->RCon()->GetProcesses(NULL);
        if (nCount)
            dwAllCount += nCount;
    }

    //if (pVCon) {
    //    mn_ActiveStatus |= pVCon->RCon()->GetProgramStatus();
    //}

    m_ProcCount = dwAllCount;
    return dwAllCount;
}

//DWORD CConEmuMain::CheckProcesses(DWORD nConmanIDX, BOOL bTitleChanged)
//{
//    // Высота буфера могла измениться после смены списка процессов
//    BOOL  lbProcessChanged = FALSE; //, lbAllowRetry = TRUE;
//    int   nTopIdx = 0;
//    DWORD dwProcList[2], nProcCount;
//    #pragma message (FILE_LINE "Win2k: GetConsoleProcessList")
//    // Для Win2k можно бежать по TH32CS_SNAPPROCESS и использовать th32ParentProcessID для определения
//    // наш ли это процесс. Учесть, что часть процессов цепочки может быть убита. Пример:
//    // {ConEmu} -> {FAR} -> {CMD - убит} -> {PID1} -> {PID2} ...
//    nProcCount = GetConsoleProcessList(dwProcList,2);
//
//    
//    // Смену списка процессов хорошо бы еще отслеживать по последнему Top процессу
//    // А то при пакетной компиляции процессы меняются, но количество
//    // может оставаться неизменным
//
//    // Дополнительные телодвижения делаем только если в консоли изменился
//    // список процессов
//    if (nProcCount != m_ProcCount && nProcCount > 1) {
//        /*if (ConMan_ProcessCommand) {
//            nConmanIDX = ConMan_ProcessCommand(46/ *GET_ACTIVENUM* /,0,0);
//        }*/
//
//        DWORD *dwFullProcList = new DWORD[nProcCount+10];
//        nProcCount = GetConsoleProcessList(dwFullProcList,nProcCount+10);
//        lbProcessChanged = TRUE;
//        //CES_EDITOR и др. устанавливаются в SetTitle, а он вызывается перед этой функцией!
//        //mn_ActiveStatus &= ~(CES_FARACTIVE|CES_NTVDM);
//        mn_ActiveStatus &= ~CES_PROGRAMS;
//        
//        int i, j;
//        std::vector<struct ConProcess>::iterator iter;
//        struct ConProcess ConPrc;
//        bool bActive;
//        DWORD dwPID, dwTopPID, nTopIdx = 0, dwCurPID;
//        
//        dwCurPID = GetCurrentProcessId();
//        // Какой процесс был запущен последним?
//        if (dwFullProcList[0] == dwCurPID)
//            nTopIdx ++;
//        dwTopPID = dwFullProcList[nTopIdx];
//        // Сначала пробежаться по тому, что мы уже запомнили, 
//        // и удалить то что завершилось, да и сбросить коды уже известных процессов
//        iter = m_Processes.begin();
//        while (iter != m_Processes.end())
//        {
//            bActive = false; dwPID = iter->ProcessID;
//            for (i=0; i<(int)nProcCount; i++) {
//                if (dwFullProcList[i] == dwPID) {
//                    bActive = true; j = i; break;
//                }
//            }
//            if (!bActive)
//                iter = m_Processes.erase(iter);
//            else {
//                iter ++; dwFullProcList[j] = 0; // чтобы не добавить дубль
//            }
//        }
//        // Теперь, добавить новые процессы (скорее всего это только dwTopPID)
//        BOOL bWasNewProcess = FALSE;
//        for (i=0; i<(int)nProcCount; i++)
//        {
//            if (!dwFullProcList[i]) continue; // это НЕ новый процесс
//            if (dwFullProcList[i] == dwCurPID) continue; // Это сам ConEmu
//
//            memset(&ConPrc, 0, sizeof(ConPrc));
//            ConPrc.ProcessID = dwFullProcList[i];
//            ////TODO: теоретически, индекс конмана для верхнего процесса может отличаться, если заголовок консоли мигает (конман создает новую консоль)
//            //ConPrc.ConmanIDX = nConmanIDX;
//            //// Определить имя exe-шника
//            //DWORD dwErr = 0;
//            //if (ConPrc.NameChecked = GetProcessFileName(ConPrc.ProcessID, ConPrc.Name, &dwErr)) {
//            //  CheckProcessName(ConPrc); //far, telnet, conman
//            //  // Теоретически - это может быть багом (если из одного фара запустили другой, но заголовок не поменялся?
//            //  if (nConmanIDX && (bTitleChanged || (nConmanIDX != m_ActiveConmanIDX)))
//            //      ConPrc.ConmanChecked = true;
//            //} else if (dwErr == 6) {
//            //  ConPrc.RetryName = true;
//            //  lbAllowRetry = FALSE;
//            //  mn_NeedRetryName ++;
//            //}
//            // Если это НЕ фар - наверное индекс конмана нас не интересует? интересует. а то как узнать, что фар команды выполняет...
//            //if (!ConPrc.IsFar /* && ConPrc.NameChecked */)
//            //  ConPrc.ConmanIDX = 0;
//            
//            // Запомнить
//            if (!bWasNewProcess) bWasNewProcess = TRUE;
//            m_Processes.push_back(ConPrc);
//        }
//        // Через Process32First/Process32Next получить информацию обо всех добавленных PID
//        if (bWasNewProcess) {
//            HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
//            if (h==INVALID_HANDLE_VALUE) {
//                DWORD dwErr = GetLastError();
//                iter = m_Processes.begin();
//                while (iter != m_Processes.end())
//                {
//                    if (iter->Name[0] == 0) {
//                        swprintf(iter->Name, L"CreateToolhelp32Snapshot failed. 0x%08X", dwErr);
//                    }
//                    iter ++;
//                }
//            } else {
//                PROCESSENTRY32 p; memset(&p, 0, sizeof(p)); p.dwSize = sizeof(p);
//                if (Process32First(h, &p)) 
//                {
//                    do {
//                        // Перебираем процессы - нужно сохранить имя
//                        iter = m_Processes.begin();
//                        while (iter != m_Processes.end())
//                        {
//                            if (!iter->NameChecked && iter->ProcessID == p.th32ProcessID)
//                            {
//                                TCHAR* pszSlash = _tcsrchr(p.szExeFile, _T('\\'));
//                                if (pszSlash) pszSlash++; else pszSlash=p.szExeFile;
//                                int nLen = _tcslen(pszSlash);
//                                if (nLen>=63) pszSlash[63]=0;
//                                _tcscpy(iter->Name, pszSlash);
//                                CheckProcessName(*iter, p.szExeFile); //far, telnet, conman
//                                // запомнить родителя
//                                iter->ParentPID = p.th32ParentProcessID;
//                            }
//                            iter ++;
//                        }
//                        // Следущий процесс
//                    } while (Process32Next(h, &p));
//                }
//                // Пометить процессы, которых уже нет (и когда успели?)
//                iter = m_Processes.begin();
//                while (iter != m_Processes.end())
//                {
//                    if (iter->Name[0] == 0) {
//                        swprintf(iter->Name, L"Process %i not found in snapshoot", iter->ProcessID);
//                    }
//                    iter ++;
//                }
//            }
//        }
//        // Осталось выяснить, какой процесс сейчас "сверху"
//        // Идем с конца списка, т.к. новые процессы консоли добавляются в конец
//        mn_TopProcessID = 0;
//        for (i=m_Processes.size()-1; i>=0; i--)
//        {
//            if (m_Processes[i].ConmanIDX == nConmanIDX) {
//                // скорее всего это искомый процесс
//                if (m_Processes[i].IsFar)
//                    mn_TopProcessID = m_Processes[i].ProcessID;
//                    
//                if (m_Processes[i].IsFar)
//                    mn_ActiveStatus |= CES_FARACTIVE;
//                if (m_Processes[i].IsTelnet)
//                    mn_ActiveStatus |= CES_TELNETACTIVE;
//                //if (nConmanIDX) mn_ActiveStatus |= CES_CONMANACTIVE;
//                break;
//            }
//        }
//        delete dwFullProcList;
//    }
//
//    //if (mn_NeedRetryName>0 && lbAllowRetry) {
//    //    for (int i=m_Processes.size()-1; i>=0; i--)
//    //    {
//    //      if (!m_Processes[i].RetryName) continue;
//    //      DWORD dwErr = 0;
//    //      if (m_Processes[i].NameChecked = GetProcessFileName(m_Processes[i].ProcessID, m_Processes[i].Name, &dwErr)) {
//    //          m_Processes[i].IsFar = lstrcmpi(m_Processes[i].Name, _T("far.exe"))==0;
//    //          if (m_Processes[i].IsTelnet = lstrcmpi(m_Processes[i].Name, _T("telnet.exe"))==0)
//    //              mn_ActiveStatus |= CES_TELNETACTIVE;
//    //          if (m_Processes[i].IsConman = lstrcmpi(m_Processes[i].Name, _T("conman.exe"))==0)
//    //              mn_ActiveStatus |= CES_CONMANACTIVE;
//    //          m_Processes[i].NameChecked = true;
//    //          m_Processes[i].RetryName = false;
//    //          mn_NeedRetryName --;
//    //      } else {
//    //          lbAllowRetry = FALSE;
//    //      }
//    //  }
//    //}
//
//    // попытаться обновить номер ConMan для необработанных КОРНЕВЫХ процессов
//    if (mn_ConmanPID && nConmanIDX && (/*bTitleChanged ||*/ (nConmanIDX != m_ActiveConmanIDX)))
//    {
//        lbProcessChanged = TRUE; // чтобы дальше "верхний" процесс определился
//        for (int i=m_Processes.size()-1; i>=0; i--)
//        {
//            // 0-консоль - типа сам Конман (или ConEmu)
//            if (m_Processes[i].IsConman /*|| !m_Processes[i].IsFar*/) continue;
//            if ((m_Processes[i].ParentPID == mn_ConmanPID) &&
//                (!m_Processes[i].ConmanChecked || !m_Processes[i].ConmanIDX))
//            {
//                // Обновляем. Скорее всего номер уже правильный
//                m_Processes[i].ConmanIDX = nConmanIDX;
//                m_Processes[i].ConmanChecked = true;
//            }
//        }
//    }
//    
//    // Получить номер Конмана для процессов второго уровня
//    if (mn_ConmanPID && lbProcessChanged) {
//        for (int i=m_Processes.size()-1; i>=0; i--)
//        {
//            // 0-консоль - типа сам Конман
//            if (m_Processes[i].IsConman ||
//                m_Processes[i].ConmanChecked ||
//                m_Processes[i].ParentPID == mn_ConmanPID ||
//                m_Processes[i].ParentPID == 0 /* ? */
//                )
//            continue; // эти процессы уже обработаны и нас не интересуют
//            
//            // Нужно найти корневой процесс
//            DWORD nParentPID = m_Processes[i].ParentPID;
//            std::vector<struct ConProcess>::iterator iter;
//            iter = m_Processes.begin();
//            while (iter != m_Processes.end()) {
//                if (iter->ProcessID == nParentPID) {
//                    nParentPID = iter->ParentPID;
//                    if (!nParentPID) break;
//                    if (nParentPID == mn_ConmanPID) {
//                        if (!iter->ConmanChecked || !iter->ConmanIDX)
//                            break; // Индекс конмана для корневого процесса пока не определен!
//                        // Обновляем 
//                        m_Processes[i].ConmanIDX = iter->ConmanIDX;
//                        m_Processes[i].ConmanChecked = true;
//                        break; // OK
//                    }
//                    iter = m_Processes.begin();
//                    continue;
//                }
//                iter ++;
//            }
//        }
//    }
//    
//    // Получить код текущего "верхнего" процесса
//    if (lbProcessChanged) {
//        mn_TopProcessID = 0;
//        for (int i=m_Processes.size()-1; !mn_TopProcessID && i>=0; i--)
//        {
//            // 0-консоль - типа сам Конман
//            if (m_Processes[i].IsConman /*|| !m_Processes[i].IsFar*/) continue;
//            
//            if (m_Processes[i].IsFar && (m_Processes[i].ConmanIDX == nConmanIDX)) {
//                mn_TopProcessID = m_Processes[i].ProcessID;
//                break;
//            }
//        }
//    }
//    if (mn_TopProcessID) {
//        mn_ActiveStatus |= CES_FARACTIVE;
//    }
//    
//    if (!lbProcessChanged)
//        lbProcessChanged = m_ActiveConmanIDX != nConmanIDX;
//        
//    if (lbProcessChanged && ghWnd) {
//        TabBar.Reset();
//        if (mn_TopProcessID) {
//            // Дернуть событие для этого процесса фара - он перешлет текущие табы
//            TabBar.Retrieve();
//            /*swprintf(temp, CONEMUREQTABS, mn_TopProcessID);
//            HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, temp);
//            if (hEvent) {
//                SetEvent(hEvent);
//                SafeCloseHandle(hEvent);
//            }*/
//        }
//    }
//
//    //TODO: Alternative console?
//    if (m_ActiveConmanIDX != nConmanIDX || lbProcessChanged) {
//        TabBar.OnConman ( m_ActiveConmanIDX, isConmanAlternative() );
//    }
//
//    m_ProcCount = nProcCount;
//    m_ActiveConmanIDX = nConmanIDX;
//    
//
//    if (ghOpWnd) {
//        UpdateProcessDisplay(lbProcessChanged);
//    }
//
//    TabBar.Refresh(mn_ActiveStatus & CES_FARACTIVE);
//    
//    
//    //if (m_ProcCount>=2) {
//    //    if (m_ProcList[0]==GetCurrentProcessId())
//    //      nTopIdx++;
//    //}
//    //if (m_ProcCount && (!gnLastProcessCount || m_ProcCount!=gnLastProcessCount))
//    //  lbProcessChanged = TRUE;
//    //else if (m_ProcCount && m_ProcList[nTopIdx]!=mn_TopProcessID)
//    //  lbProcessChanged = TRUE;
//    //gnLastProcessCount = m_ProcCount;
//    //
//    //if (lbProcessChanged) {
//    //  if (m_ProcList[nTopIdx]==mn_TopProcessID) {
//    //      // не менялось
//    //      mb_FarActive = _tcscmp(ms_TopProcess, _T("far.exe"))==0;
//    //  } else
//    //  if (m_ProcList[nTopIdx]!=GetCurrentProcessId())
//    //  {
//    //      // Получить информацию о верхнем процессе
//    //      DWORD dwErr = 0;
//    //      HANDLE hProcess = OpenProcess(PROCESS_VM_READ|PROCESS_QUERY_INFORMATION, FALSE, m_ProcList[nTopIdx]);
//    //      if (!hProcess)
//    //          dwErr = GetLastError();
//    //      else
//    //      {
//    //          TCHAR szFilePath[MAX_PATH+1];
//    //          if (!GetModuleFileNameEx(hProcess, 0, szFilePath, MAX_PATH))
//    //              dwErr = GetLastError();
//    //          else
//    //          {
//    //              TCHAR* pszSlash = _tcsrchr(szFilePath, _T('\\'));
//    //              if (pszSlash) pszSlash++; else pszSlash=szFilePath;
//    //              int nLen = _tcslen(pszSlash);
//    //              if (nLen>MAX_PATH) pszSlash[MAX_PATH]=0;
//    //              _tcscpy(ms_TopProcess, pszSlash);
//    //              _tcslwr(ms_TopProcess);
//    //              mb_FarActive = _tcscmp(ms_TopProcess, _T("far.exe"))==0;
//    //              mn_TopProcessID = m_ProcList[nTopIdx];
//    //          }
//    //          CloseHandle(hProcess); hProcess = NULL;
//    //      }
//    //
//    //  }
//    //  TabBar.Refresh(mb_FarActive);
//    //}
//
//    return m_ProcCount;
//}

bool CConEmuMain::ConActivateNext(BOOL abNext)
{
    //if (nCmd == CONMAN_NEXTCONSOLE || nCmd == CONMAN_PREVCONSOLE)

    int nActive = ActiveConNum(), i, j, n1, n2, n3;
    for (j=0; j<=1; j++) {
        if (abNext) {
            if (j == 0) {
                n1 = nActive+1; n2 = MAX_CONSOLE_COUNT; n3 = 1;
            } else {
                n1 = 0; n2 = nActive; n3 = 1;
            }
            if (n1>=n2) continue;
        } else {
            if (j == 0) {
                n1 = nActive-1; n2 = -1; n3 = -1;
            } else {
                n1 = MAX_CONSOLE_COUNT-1; n2 = nActive; n3 = -1;
            }
            if (n1<=n2) continue;
        }

        for (i=n1; i!=n2 && i>=0 && i<MAX_CONSOLE_COUNT; i+=n3) {
            if (mp_VCon[i]) {
                return ConActivate(i);
            }
        }
    }
    return false;
}

// nCon - zero-based index of console
bool CConEmuMain::ConActivate(int nCon)
{
    FLASHWINFO fl = {sizeof(FLASHWINFO)}; fl.dwFlags = FLASHW_STOP; fl.hwnd = ghWnd;
    FlashWindowEx(&fl); // При многократных созданиях мигать начинает...

    if (nCon>=0 && nCon<MAX_CONSOLE_COUNT) {
        CVirtualConsole* pCon = mp_VCon[nCon];
        if (pCon == NULL)
            return false; // консоль с этим номером не была создана!
        if (pCon == pVCon)
            return true; // уже
        bool lbSizeOK = true;
        int nOldConNum = ActiveConNum();

        // Спрятать PictureView, или еще чего...
        if (pVCon) pVCon->RCon()->OnDeactivate(nCon);
        
        // ПЕРЕД переключением на новую консоль - обновить ее размеры
        if (pVCon) {
            int nOldConWidth = pVCon->RCon()->TextWidth();
            int nOldConHeight = pVCon->RCon()->TextHeight();
            int nNewConWidth = pCon->RCon()->TextWidth();
            int nNewConHeight = pCon->RCon()->TextHeight();
            if (nOldConWidth != nNewConWidth || nOldConHeight != nNewConHeight) {
                lbSizeOK = pCon->RCon()->SetConsoleSize(MakeCoord(nOldConWidth,nOldConHeight));
            }
        }

        pVCon = pCon;

        pCon->RCon()->OnActivate(nCon, nOldConNum);
        
        if (!lbSizeOK)
            SyncWindowToConsole();
        m_Child.Invalidate();
    }
    return false;
}

CVirtualConsole* CConEmuMain::CreateCon(RConStartArgs *args)
{
    CVirtualConsole* pCon = NULL;
    for (int i=0; i<MAX_CONSOLE_COUNT; i++) {
        if (!mp_VCon[i]) {
            pCon = CVirtualConsole::CreateVCon(args);
            if (pCon) {
                if (pVCon) pVCon->RCon()->OnDeactivate(i);
                mp_VCon[i] = pCon;
                pVCon = pCon;
                pCon->RCon()->OnActivate(i, ActiveConNum());
                
                //mn_ActiveCon = i;

                //Update(true);
            }
            break;
        }
    }
    return pCon;
}

void CConEmuMain::EnableComSpec(BOOL abSwitch)
{
	for (int i = 0; i<MAX_CONSOLE_COUNT; i++) {
		if (mp_VCon[i] == NULL) continue;
		CRealConsole* pRCon = mp_VCon[i]->RCon();
		DWORD dwFarPID = pRCon->GetFarPID();
		if (!dwFarPID) continue;
		pRCon->EnableComSpec(dwFarPID, abSwitch);
	}
}

void CConEmuMain::DnDstep(LPCTSTR asMsg)
{
    if ((gSet.isDnDsteps && ghWnd) || !asMsg)
        SetWindowText(ghWnd, asMsg ? asMsg : Title);
}

DWORD CConEmuMain::GetActiveKeyboardLayout()
{
    DWORD dwActive = (DWORD)GetKeyboardLayout(mn_MainThreadId);
    return dwActive;
}

LPTSTR CConEmuMain::GetTitleStart()
{
    //mn_ActiveStatus &= ~CES_CONALTERNATIVE; // сброс флага альтернативной консоли
    return Title;
}

LRESULT CConEmuMain::GuiShellExecuteEx(SHELLEXECUTEINFO* lpShellExecute, BOOL abAllowAsync)
{
	LRESULT lRc = 0;

	if (!isMainThread()) {
		if (abAllowAsync)
			lRc = PostMessage(ghWnd, mn_ShellExecuteEx, abAllowAsync, (LPARAM)lpShellExecute);
		else
			lRc = SendMessage(ghWnd, mn_ShellExecuteEx, abAllowAsync, (LPARAM)lpShellExecute);
	} else {
		if (IsDebuggerPresent()) {
			BOOL b = gbDontEnable; gbDontEnable = TRUE;
			int nBtn = MessageBox(ghWnd, L"Debugger active!\nShellExecuteEx(runas) my fails\nContinue?", L"ConEmu", MB_ICONASTERISK|MB_YESNO|MB_DEFBUTTON2);
			gbDontEnable = b;
			if (nBtn != IDYES)
				return (FALSE);
		}
		lRc = ::ShellExecuteEx(lpShellExecute);
		
		if (abAllowAsync && lRc == 0) {
			pVCon->RCon()->CloseConsole();
		}
	}

	return lRc;
}

//BOOL CConEmuMain::HandlerRoutine(DWORD dwCtrlType)
//{
//    return (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT ? true : false);
//}

// Нужно вызывать после загрузки настроек!
void CConEmuMain::LoadIcons()
{
    if (hClassIcon)
        return; // Уже загружены

    TCHAR szIconPath[MAX_PATH] = {0};
    lstrcpyW(szIconPath, ms_ConEmuExe);
        
    TCHAR *lpszExt = _tcsrchr(szIconPath, _T('.'));
    if (!lpszExt)
        szIconPath[0] = 0;
    else {
        _tcscpy(lpszExt, _T(".ico"));
        DWORD dwAttr = GetFileAttributes(szIconPath);
        if (dwAttr==(DWORD)-1 || (dwAttr & FILE_ATTRIBUTE_DIRECTORY))
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
    //wchar_t* pDesc=NULL;
    
    const wchar_t WSFI[] = L"StringFileInfo";

    DWORD size = GetFileVersionInfoSizeW(pFullPath, &size);
    if(!size) return false;
    MCHKHEAP
    pBuffer = new BYTE[size];
    MCHKHEAP
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

void CConEmuMain::PostCopy(wchar_t* apszMacro, BOOL abRecieved/*=FALSE*/)
{
    if (!abRecieved) {
        PostMessage(ghWnd, mn_MsgPostCopy, 0, (LPARAM)apszMacro);
    } else {
        PostMacro(apszMacro);
        free(apszMacro);
    }
}

void CConEmuMain::PostMacro(LPCWSTR asMacro)
{
    if (!asMacro || !*asMacro)
        return;

    CConEmuPipe pipe(GetFarPID(), CONEMUREADYTIMEOUT);
    if (pipe.Init(_T("CConEmuMain::PostMacro"), TRUE))
    {
        //DWORD cbWritten=0;
        DnDstep(_T("Macro: Waiting for result (10 sec)"));
        pipe.Execute(CMD_POSTMACRO, asMacro, (wcslen(asMacro)+1)*2);
        DnDstep(NULL);
    }
}

bool CConEmuMain::PtDiffTest(POINT C, int aX, int aY, UINT D)
{
    //(((abs(C.x-LOWORD(lParam)))<D) && ((abs(C.y-HIWORD(lParam)))<D))
    int nX = C.x - aX;
    if (nX < 0) nX = -nX;
    if (nX > (int)D)
        return false;
    int nY = C.y - aY;
    if (nY < 0) nY = -nY;
    if (nY > (int)D)
        return false;
    return true;
}

void CConEmuMain::RegisterHotKeys()
{
	if (!mb_HotKeyRegistered) {
		if (RegisterHotKey(ghWnd, 0x201, MOD_CONTROL|MOD_WIN|MOD_ALT, VK_SPACE))
			mb_HotKeyRegistered = TRUE;
	}
}

void CConEmuMain::UnRegisterHotKeys()
{
	if (mb_HotKeyRegistered) {
		UnregisterHotKey(ghWnd, 0x201);
	}
	mb_HotKeyRegistered = FALSE;
}

void CConEmuMain::CtrlWinAltSpace()
{
    if (!pVCon) {
    	//MBox(L"CtrlWinAltSpace: pVCon==NULL");
    	return;
	}
    
    static DWORD dwLastSpaceTick = 0;
    if ((dwLastSpaceTick-GetTickCount())<1000) {
        //if (hWnd == ghWndDC) MBoxA(_T("Space bounce recieved from DC")) else
        //if (hWnd == ghWnd) MBoxA(_T("Space bounce recieved from MainWindow")) else
        //if (hWnd == gConEmu.m_Back.mh_WndBack) MBoxA(_T("Space bounce recieved from BackWindow")) else
        //if (hWnd == gConEmu.m_Back.mh_WndScroll) MBoxA(_T("Space bounce recieved from ScrollBar")) else
        MBoxA(_T("Space bounce recieved from unknown window"));
        return;
    }
    dwLastSpaceTick = GetTickCount();

    //MBox(L"CtrlWinAltSpace: Toggle");
    pVCon->RCon()->ShowConsole(-1); // Toggle visibility
}

void CConEmuMain::Recreate(BOOL abRecreate, BOOL abConfirm)
{
    FLASHWINFO fl = {sizeof(FLASHWINFO)}; fl.dwFlags = FLASHW_STOP; fl.hwnd = ghWnd;
    FlashWindowEx(&fl); // При многократных созданиях мигать начинает...

	RConStartArgs args;
	args.bRecreate = abRecreate;
    
    if (!abRecreate) {
        // Создать новую консоль
        BOOL lbSlotFound = FALSE;
        for (int i=0; i<MAX_CONSOLE_COUNT; i++) {
            if (!mp_VCon[i]) { lbSlotFound = TRUE; break; }
        }
        if (!lbSlotFound) {
        	static bool bBoxShowed = false;
        	if (!bBoxShowed) {
	        	bBoxShowed = true;
	        	
                FlashWindowEx(&fl); // При многократных созданиях мигать начинает...
	        	
    	    	MBoxA(L"Maximum number of consoles was reached.");
        		bBoxShowed = false;
        	}
        	FlashWindowEx(&fl); // При многократных созданиях мигать начинает...
        	return;
        }

        if (abConfirm) {
            BOOL b = gbDontEnable;
            gbDontEnable = TRUE;
            int nRc = DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_RESTART), ghWnd, RecreateDlgProc, (LPARAM)&args);
            gbDontEnable = b;
            if (nRc != IDC_START)
                return;
			m_Child.Redraw();
        }
        //Собственно, запуск
        CreateCon(&args);
        
    } else {
        // Restart or close console
        int nActive = ActiveConNum();
        if (nActive >=0) {
			args.bRunAsAdministrator = pVCon->RCon()->isAdministrator();

            BOOL b = gbDontEnable;
            gbDontEnable = TRUE;
            int nRc = DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_RESTART), ghWnd, RecreateDlgProc, (LPARAM)&args);
            gbDontEnable = b;
            if (nRc == IDC_TERMINATE) {
                pVCon->RCon()->CloseConsole();
                return;
            }
            if (nRc != IDC_START)
                return;
			m_Child.Redraw();
            // Собственно, Recreate
            pVCon->RCon()->RecreateProcess(&args);
        }
    }
    
    SafeFree(args.pszSpecialCmd);
}

int CConEmuMain::BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    if (uMsg==BFFM_INITIALIZED) {
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
    }
	return 0;
}

INT_PTR CConEmuMain::RecreateDlgProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
    switch (messg)
    {
    case WM_INITDIALOG:
        {
            BOOL lbRc = FALSE;
            //#ifdef _DEBUG
            //SetWindowPos(ghOpWnd, HWND_NOTOPMOST, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE);
            //#endif
            
            LPCWSTR pszCmd = gConEmu.ActiveCon()->RCon()->GetCmd();
            int nId = SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_FINDSTRINGEXACT, -1, (LPARAM)pszCmd);
            if (nId < 0) SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_INSERTSTRING, 0, (LPARAM)pszCmd);
            LPCWSTR pszSystem = gSet.GetCmd();
            if (pszSystem != pszCmd) {
                int nId = SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_FINDSTRINGEXACT, -1, (LPARAM)pszSystem);
                if (nId < 0) SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_INSERTSTRING, 0, (LPARAM)pszSystem);
            }
            SetDlgItemText(hDlg, IDC_RESTART_CMD, pszCmd);
            
            SetDlgItemText(hDlg, IDC_STARTUP_DIR, gConEmu.ms_ConEmuCurDir);
            EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_DIR), FALSE);
            #ifndef _DEBUG
            EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_DIR), FALSE);
            #endif
            

			RConStartArgs* pArgs = (RConStartArgs*)lParam;
			_ASSERTE(pArgs);
			SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)lParam);

			// Finally, if all you need is to determine whether or not you are currently elevated you can 
			// simply call the IsUserAnAdmin function. If you need more precision you can also use GetTokenInformation 
			// but in most cases that is overkill.
			TODO("Если GUI запущено под администратором - флажок включить и задизэблить!");

			if (pArgs && pArgs->bRunAsAdministrator) {
				CheckDlgButton(hDlg, cbRunAs, BST_CHECKED);
				RecreateDlgProc(hDlg, WM_COMMAND, cbRunAs, 0);
			}

            SetClassLongPtr(hDlg, GCLP_HICON, (LONG)hClassIcon);
            if (pArgs->bRecreate) {
                //GCC hack. иначе не собирается
                SetDlgItemTextA(hDlg, IDC_RESTART_MSG, "About to recreate console");
                SendDlgItemMessage(hDlg, IDC_RESTART_ICON, STM_SETICON, (WPARAM)LoadIcon(NULL,IDI_EXCLAMATION), 0);
                
                lbRc = TRUE;
            } else {
                //GCC hack. иначе не собирается
                SetDlgItemTextA(hDlg, IDC_RESTART_MSG, "Сreate new console");
                SendDlgItemMessage(hDlg, IDC_RESTART_ICON, STM_SETICON, (WPARAM)LoadIcon(NULL,IDI_QUESTION), 0);
                POINT pt = {0,0};
                MapWindowPoints(GetDlgItem(hDlg, IDC_TERMINATE), hDlg, &pt, 1);
                DestroyWindow(GetDlgItem(hDlg, IDC_TERMINATE));
                SetWindowPos(GetDlgItem(hDlg, IDC_START), NULL, pt.x, pt.y, 0,0, SWP_NOSIZE|SWP_NOZORDER);
                SetDlgItemText(hDlg, IDC_START, L"&Start");
                DestroyWindow(GetDlgItem(hDlg, IDC_WARNING));
                RECT rcBox; GetWindowRect(GetDlgItem(hDlg, cbRunAs), &rcBox);
                pt.x = pt.x - (rcBox.right - rcBox.left) - 5;
                MapWindowPoints(NULL, hDlg, (LPPOINT)&rcBox, 1);
                pt.y = rcBox.top;
                SetWindowPos(GetDlgItem(hDlg, cbRunAs), NULL, pt.x, pt.y, 0,0, SWP_NOSIZE|SWP_NOZORDER);
                
                SetFocus(GetDlgItem(hDlg, IDC_RESTART_CMD));
            }
            
            RECT rect;
            GetWindowRect(hDlg, &rect);
            RECT rcParent;
            GetWindowRect(ghWnd, &rcParent);
            MoveWindow(hDlg,
                (rcParent.left+rcParent.right-rect.right+rect.left)/2,
                (rcParent.top+rcParent.bottom-rect.bottom+rect.top)/2,
                rect.right - rect.left, rect.bottom - rect.top, false);
                
            return lbRc;
        }
        
    case WM_CTLCOLORSTATIC:
        if (GetDlgItem(hDlg, IDC_WARNING) == (HWND)lParam)
        {
            SetTextColor((HDC)wParam, 255);
            HBRUSH hBrush = GetSysColorBrush(COLOR_3DFACE);
            SetBkMode((HDC)wParam, TRANSPARENT);
            return (BOOL)hBrush;
        }
        break;

    case WM_GETICON:
        if (wParam==ICON_BIG) {
            /*SetWindowLong(hWnd2, DWL_MSGRESULT, (LRESULT)hClassIcon);
            return 1;*/
        } else {
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LRESULT)hClassIconSm);
            return 1;
        }
        return 0;

    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            switch (LOWORD(wParam)) {
                case IDC_CHOOSE:
                {
                    wchar_t *pszFilePath = NULL;
                    int nLen = MAX_PATH*2;
                    pszFilePath = (wchar_t*)calloc(nLen+1,2);
                    if (!pszFilePath) return 1;
                    
                    OPENFILENAME ofn; memset(&ofn,0,sizeof(ofn));
                    ofn.lStructSize=sizeof(ofn);
                    ofn.hwndOwner = hDlg;
                    ofn.lpstrFilter = _T("Executables (*.exe)\0*.exe\0\0");
                    ofn.nFilterIndex = 1;
                    ofn.lpstrFile = pszFilePath;
                    ofn.nMaxFile = nLen;
                    ofn.lpstrTitle = _T("Choose program to run");
                    ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
                            | OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST;
                    if (GetOpenFileName(&ofn))
                        SetDlgItemText(hDlg, IDC_RESTART_CMD, pszFilePath);

                    SafeFree(pszFilePath); 
                    return 1;
                }
                
                case IDC_CHOOSE_DIR:
                {
                	BROWSEINFO bi = {ghWnd};
                	wchar_t szFolder[MAX_PATH+1] = {0};
                	GetDlgItemText(hDlg, IDC_STARTUP_DIR, szFolder, countof(szFolder));
                	bi.pszDisplayName = szFolder;
                	wchar_t szTitle[100];
                	bi.lpszTitle = wcscpy(szTitle, L"Choose startup directory");
                	bi.ulFlags = BIF_EDITBOX | BIF_RETURNONLYFSDIRS | BIF_VALIDATE;
                	// bi.lpfn = int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
                	bi.lParam = (LPARAM)szFolder;
                	
                	LPITEMIDLIST pRc = SHBrowseForFolder(&bi);
                	if (pRc) {
                		CoTaskMemFree(pRc);
                		
                		SetDlgItemText(hDlg, IDC_STARTUP_DIR, szFolder);
                	}
                	return 1;
                }
                
                case cbRunAs:
                {
                	// BCM_SETSHIELD = 5644
                	if (gOSVer.dwMajorVersion >= 6) {
                		BOOL bRunAs = SendDlgItemMessage(hDlg, cbRunAs, BM_GETCHECK, 0, 0);
                		SendDlgItemMessage(hDlg, IDC_START, 5644/*BCM_SETSHIELD*/, 0, bRunAs);
                	}
                	return 1;
                }
                    
                case IDC_START:
                {
					RConStartArgs* pArgs = (RConStartArgs*)GetWindowLongPtr(hDlg, DWLP_USER);
					_ASSERTE(pArgs);
                    HWND hEdit = GetDlgItem(hDlg, IDC_RESTART_CMD);
                    int nLen = GetWindowTextLength(hEdit);
                    if (nLen > 0) {
                        pArgs->pszSpecialCmd = (wchar_t*)calloc(nLen+1,2);
                        if (pArgs->pszSpecialCmd)
                            GetWindowText(hEdit, pArgs->pszSpecialCmd, nLen+1);
                    }
					pArgs->bRunAsAdministrator = SendDlgItemMessage(hDlg, cbRunAs, BM_GETCHECK, 0, 0);
                    EndDialog(hDlg, IDC_START);
                    return 1;
                }
                case IDC_TERMINATE:
                    EndDialog(hDlg, IDC_TERMINATE);
                    return 1;
                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return 1;
            }
        }
        break;
    default:
        return 0;
    }
    return 0;
}

void CConEmuMain::ShowOldCmdVersion(DWORD nCmd, DWORD nVersion)
{
    if (!isMainThread()) {
        if (mb_InShowOldCmdVersion)
            return; // уже послано
        mb_InShowOldCmdVersion = TRUE;
        PostMessage(ghWnd, mn_MsgOldCmdVer, nCmd, nVersion);
        return;
    }

	static bool lbErrorShowed = false;
	if (lbErrorShowed) return;
	lbErrorShowed = true;

    wchar_t szMsg[255];
    wsprintf(szMsg, L"ConEmu received old version packet!\nCommandID: %i, Version: %i\nPlease check ConEmuC.exe and ConEmu.dll", nCmd, nVersion);
    MBox(szMsg);

    mb_InShowOldCmdVersion = FALSE; // теперь можно показать еще одно...
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

void CConEmuMain::UpdateProcessDisplay(BOOL abForce)
{
    if (!ghOpWnd)
        return;

    wchar_t szNo[32];
    DWORD nProgramStatus = pVCon->RCon()->GetProgramStatus();
    DWORD nFarStatus = pVCon->RCon()->GetFarStatus();
    CheckDlgButton(gSet.hInfo, cbsTelnetActive, (nProgramStatus&CES_TELNETACTIVE) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(gSet.hInfo, cbsNtvdmActive, (nProgramStatus&CES_NTVDM) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(gSet.hInfo, cbsFarActive, (nProgramStatus&CES_FARACTIVE) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(gSet.hInfo, cbsFilePanel, (nFarStatus&CES_FILEPANEL) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(gSet.hInfo, cbsEditor, (nFarStatus&CES_EDITOR) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(gSet.hInfo, cbsViewer, (nFarStatus&CES_VIEWER) ? BST_CHECKED : BST_UNCHECKED);
    SetDlgItemText(gSet.hInfo, tsTopPID, _itow(pVCon->RCon()->GetFarPID(), szNo, 10));
	CheckDlgButton(gSet.hInfo, cbsProgress, (nFarStatus&CES_WASPROGRESS) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(gSet.hInfo, cbsProgressError, (nFarStatus&CES_OPER_ERROR) ? BST_CHECKED : BST_UNCHECKED);

    if (!abForce)
        return;

    MCHKHEAP
    SendDlgItemMessage(gSet.hInfo, lbProcesses, LB_RESETCONTENT, 0, 0);
    wchar_t temp[MAX_PATH];
    for (int j=0; j<MAX_CONSOLE_COUNT; j++) {
        if (mp_VCon[j] == NULL) continue;

        ConProcess* pPrc = NULL;
        int nCount = mp_VCon[j]->RCon()->GetProcesses(&pPrc);
        for (int i=0; i<nCount; i++) {
            if (mp_VCon[j] == pVCon)
                _tcscpy(temp, _T("(*) "));
            else
                temp[0] = 0;

            swprintf(temp+_tcslen(temp), _T("[%i.%i] %s - PID:%i"),
                j+1, i, pPrc[i].Name, pPrc[i].ProcessID);
            SendDlgItemMessage(gSet.hInfo, lbProcesses, LB_ADDSTRING, 0, (LPARAM)temp);
        }
        if (pPrc) { free(pPrc); pPrc = NULL; }
    }
    MCHKHEAP
}

void CConEmuMain::UpdateSizes()
{
    if (!ghOpWnd)
        return;
    if (!isMainThread()) {
        PostMessage(ghWnd, mn_MsgUpdateSizes, 0, 0);
        return;
    }

    if (pVCon)
        pVCon->UpdateInfo();
    else {
        SetDlgItemText(gSet.hInfo, tConSizeChr, _T("?"));
        SetDlgItemText(gSet.hInfo, tConSizePix, _T("?"));
        SetDlgItemText(gSet.hInfo, tPanelLeft, _T("?"));
        SetDlgItemText(gSet.hInfo, tPanelRight, _T("?"));
    }
    RECT rcClient; GetClientRect(ghWndDC, &rcClient);
    TCHAR szSize[32]; wsprintf(szSize, _T("%ix%i"), rcClient.right, rcClient.bottom);
    SetDlgItemText(gSet.hInfo, tDCSize, szSize);
}

// !!!Warning!!! Никаких return. в конце функции вызывается необходимый CheckProcesses
void CConEmuMain::UpdateTitle(LPCTSTR asNewTitle)
{
    if (!asNewTitle)
        return;

    if (GetCurrentThreadId() != mn_MainThreadId) {
        /*if (TitleCmp != asNewTitle) -- можем наколоться на многопоточности. Лучше получим повторно
            wcscpy(TitleCmp, asNewTitle);*/
        PostMessage(ghWnd, mn_MsgUpdateTitle, 0, 0);
        return;
    }
    
    wcscpy(Title, asNewTitle);

    UpdateProgress(TRUE);

    //BOOL  lbTitleChanged = TRUE;

    //BOOL lbPrevAlt = (mn_ActiveStatus & (CES_CONMANACTIVE|CES_CONALTERNATIVE))==(CES_CONMANACTIVE|CES_CONALTERNATIVE);
    //LPTSTR pszTitle = GetTitleStart();
    //BOOL lbCurAlt = (mn_ActiveStatus & (CES_CONMANACTIVE|CES_CONALTERNATIVE))==(CES_CONMANACTIVE|CES_CONALTERNATIVE);
    
    //WARNING("Этот код нужно перенести в RealConsole");
    //mn_ActiveStatus &= CES_PROGRAMS2; // оставляем только флаги текущей программы
    //// далее идут взаимоисключающие флаги режимов текущей программы
    //if (_tcsncmp(pszTitle, _T("edit "), 5)==0 || _tcsncmp(pszTitle, ms_EditorRus, _tcslen(ms_EditorRus))==0)
    //    mn_ActiveStatus |= CES_EDITOR;
    //else if (_tcsncmp(pszTitle, _T("view "), 5)==0 || _tcsncmp(pszTitle, ms_ViewerRus, _tcslen(ms_ViewerRus))==0)
    //    mn_ActiveStatus |= CES_VIEWER;
    //else if (isFilePanel(true))
    //    mn_ActiveStatus |= CES_FILEPANEL;


    // Под конец - проверить список процессов консоли
    CheckProcesses();
}

// Если в текущей консоли есть проценты - отображаются они
// Иначе - отображается максимальное значение процентов из всех консолей
void CConEmuMain::UpdateProgress(BOOL abUpdateTitle)
{
    LPCWSTR psTitle = NULL;
    MultiTitle[0] = 0;
    
    short nProgress = -1, n;
    BOOL bActiveHasProgress = FALSE;
	BOOL bWasError = FALSE;
    if ((nProgress = pVCon->RCon()->GetProgress(&bWasError)) >= 0) {
        mn_Progress = nProgress;
        bActiveHasProgress = TRUE;
    }
	// нас интересует возможное наличие ошибки во всех остальных консолях
    for (UINT i = 0; i < MAX_CONSOLE_COUNT; i++) {
        if (mp_VCon[i]) {
            n = mp_VCon[i]->RCon()->GetProgress(&bWasError);
            if (!bActiveHasProgress && n > nProgress) nProgress = n;
        }
    }
	if (!bActiveHasProgress) {
        mn_Progress = min(nProgress,100);
    }

    static short nLastProgress = -1;
	static BOOL  bLastProgressError = FALSE;
    if (nLastProgress != mn_Progress  || bLastProgressError != bWasError) {
        HRESULT hr = S_OK;
        if (mp_TaskBar) {
            if (mn_Progress >= 0) {
                hr = mp_TaskBar->SetProgressValue(ghWnd, mn_Progress, 100);
                if (nLastProgress == -1 || bLastProgressError != bWasError)
					hr = mp_TaskBar->SetProgressState(ghWnd, bWasError ? TBPF_ERROR : TBPF_NORMAL);
            } else {
                hr = mp_TaskBar->SetProgressState(ghWnd, TBPF_NOPROGRESS);
            }
        }
        // Запомнить последнее
        nLastProgress = mn_Progress;
		bLastProgressError = bWasError;
    }
    if (mn_Progress >= 0 && !bActiveHasProgress) {
        psTitle = MultiTitle;
        wsprintf(MultiTitle+lstrlen(MultiTitle), L"{*%i%%} ", mn_Progress);
    }

    if (gSet.isMulti && !TabBar.IsShown()) {
        int nCur = 1, nCount = 0;
        for (int n=0; n<MAX_CONSOLE_COUNT; n++) {
            if (mp_VCon[n]) {
                nCount ++;
                if (pVCon == mp_VCon[n])
                    nCur = n+1;
            }
        }
        if (nCount > 1) {
            psTitle = MultiTitle;
            wsprintf(MultiTitle+lstrlen(MultiTitle), L"[%i/%i] ", nCur, nCount);
        }
    }
    
    if (psTitle)
        wcscat(MultiTitle, Title);
    else
        psTitle = Title;

    SetWindowText(ghWnd, psTitle);

    // Задел на будущее
    if (ghWndApp)
        SetWindowText(ghWndApp, psTitle);
}

VOID CConEmuMain::WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    _ASSERTE(hwnd!=NULL);
    // Процесс может запуститься и ДО того, как закончится инициализация хэндла окна!

	// Не помогает
	//if (event == EVENT_SYSTEM_MENUPOPUPSTART) {
	//	if (gConEmu.isMeForeground())
	//	{
	//		//SetForegroundWindow(hwnd);
	//		DWORD dwPID = 0;
	//		GetWindowThreadProcessId(hwnd, &dwPID);
	//		AllowSetForegroundWindow(dwPID);
	//		HWND hParent = GetAncestor(hwnd, GA_PARENT); // Это Desktop
	//		if (hParent) {
	//			GetWindowThreadProcessId(hParent, &dwPID);
	//			AllowSetForegroundWindow(dwPID);
	//		}
	//	}
	//	//hwnd это похоже окошко самого меню, ClassName=#32768 (диалог)
	//	//#ifdef _DEBUG
	//	//wchar_t szClass[128] = {0}, szTitle[128] = {0};
	//	//HWND hParent = GetAncestor(hwnd, GA_PARENT); // Это Desktop
	//	//GetClassName(hParent, szClass, 128); GetWindowText(hParent, szTitle, 128);
	//	//WCHAR szDbg[512]; wsprintfW(szDbg, L"EVENT_SYSTEM_MENUPOPUPSTART(HWND=0x%08X, object=0x%08X, child=0x%08X)\nClass: %s, Title: %s\n\n", hwnd, idObject, idChild, szClass, szTitle);
	//	//OutputDebugString(szDbg);
	//	//#endif
	//	return;
	//}

#ifdef _DEBUG
    switch(event)
    {
    case EVENT_CONSOLE_START_APPLICATION:
        //A new console process has started. 
        //The idObject parameter contains the process identifier of the newly created process. 
        //If the application is a 16-bit application, the idChild parameter is CONSOLE_APPLICATION_16BIT and idObject is the process identifier of the NTVDM session associated with the console.

        #ifdef _DEBUG
        WCHAR szDbg[128]; wsprintfW(szDbg, L"EVENT_CONSOLE_START_APPLICATION(HWND=0x%08X, PID=%i%s)\n", hwnd, idObject, (idChild == CONSOLE_APPLICATION_16BIT) ? L" 16bit" : L"");
        OutputDebugString(szDbg);
        #endif
        break;

    case EVENT_CONSOLE_END_APPLICATION:
        //A console process has exited. 
        //The idObject parameter contains the process identifier of the terminated process.

        #ifdef _DEBUG
        wsprintfW(szDbg, L"EVENT_CONSOLE_END_APPLICATION(HWND=0x%08X, PID=%i%s)\n", hwnd, idObject, (idChild == CONSOLE_APPLICATION_16BIT) ? L" 16bit" : L"");
        OutputDebugString(szDbg);
        #endif
        break;

    case EVENT_CONSOLE_UPDATE_REGION: // 0x4002 
        {
        //More than one character has changed. 
        //The idObject parameter is a COORD structure that specifies the start of the changed region. 
        //The idChild parameter is a COORD structure that specifies the end of the changed region.
        #ifdef _DEBUG
        COORD crStart, crEnd; memmove(&crStart, &idObject, sizeof(idObject)); memmove(&crEnd, &idChild, sizeof(idChild));
        WCHAR szDbg[128]; wsprintfW(szDbg, L"EVENT_CONSOLE_UPDATE_REGION({%i, %i} - {%i, %i})\n", crStart.X,crStart.Y, crEnd.X,crEnd.Y);
        OutputDebugString(szDbg);
        #endif
        } break;
    case EVENT_CONSOLE_UPDATE_SCROLL: //0x4004
        {
        //The console has scrolled.
        //The idObject parameter is the horizontal distance the console has scrolled. 
        //The idChild parameter is the vertical distance the console has scrolled.
        #ifdef _DEBUG
        WCHAR szDbg[128]; wsprintfW(szDbg, L"EVENT_CONSOLE_UPDATE_SCROLL(X=%i, Y=%i)\n", idObject, idChild);
        OutputDebugString(szDbg);
        #endif
        } break;
    case EVENT_CONSOLE_UPDATE_SIMPLE: //0x4003
        {
        //A single character has changed.
        //The idObject parameter is a COORD structure that specifies the character that has changed.
        //Warning! В писании от  микрософта тут ошибка!
        //The idChild parameter specifies the character in the low word and the character attributes in the high word.
        #ifdef _DEBUG
        COORD crWhere; memmove(&crWhere, &idObject, sizeof(idObject));
        WCHAR ch = (WCHAR)LOWORD(idChild); WORD wA = HIWORD(idChild);
        WCHAR szDbg[128]; wsprintfW(szDbg, L"EVENT_CONSOLE_UPDATE_SIMPLE({%i, %i} '%c'(\\x%04X) A=%i)\n", crWhere.X,crWhere.Y, ch, ch, wA);
        OutputDebugString(szDbg);
        #endif
        } break;
    case EVENT_CONSOLE_CARET: //0x4001
        {
        //Warning! WinXPSP3. Это событие проходит ТОЛЬКО если консоль в фокусе. 
        //         А с ConEmu она НИКОГДА не в фокусе, так что курсор не обновляется.
        //The console caret has moved.
        //The idObject parameter is one or more of the following values:
        //      CONSOLE_CARET_SELECTION or CONSOLE_CARET_VISIBLE.
        //The idChild parameter is a COORD structure that specifies the cursor's current position.
        #ifdef _DEBUG
        COORD crWhere; memmove(&crWhere, &idChild, sizeof(idChild));
        WCHAR szDbg[128]; wsprintfW(szDbg, L"EVENT_CONSOLE_CARET({%i, %i} Sel=%c, Vis=%c\n", crWhere.X,crWhere.Y, 
            ((idObject & CONSOLE_CARET_SELECTION)==CONSOLE_CARET_SELECTION) ? L'Y' : L'N',
            ((idObject & CONSOLE_CARET_VISIBLE)==CONSOLE_CARET_VISIBLE) ? L'Y' : L'N');
        OutputDebugString(szDbg);
        #endif
        } break;
    case EVENT_CONSOLE_LAYOUT: //0x4005
        {
        //The console layout has changed.
        OutputDebugString(L"EVENT_CONSOLE_LAYOUT\n");
        } break;
    }
#endif

    BOOL lbProcessed = FALSE;
    for (int i=0; i<MAX_CONSOLE_COUNT; i++) {
        if (!gConEmu.mp_VCon[i]) continue;
        if (!gConEmu.mp_VCon[i]->RCon()->ConWnd() || gConEmu.mp_VCon[i]->RCon()->ConWnd() != hwnd) continue;

        gConEmu.mp_VCon[i]->RCon()->OnWinEvent(event, hwnd, idObject, idChild, dwEventThread, dwmsEventTime);
        lbProcessed = TRUE;
        break;
    }

    // Если событие "Запущен процесс" пришло ДО того, как в VirtualConsole определился
    // хэндл консольного окна - передать событие в тот VirtualConsole, в котором
    // mn_ConEmuC_PID == idObject
    if (!lbProcessed && event == EVENT_CONSOLE_START_APPLICATION && idObject) {
        // Warning. В принципе, за время выполнения этой процедуры mp_VCon[i]->hConWnd мог уже проинициализироваться
        for (int i=0; i<MAX_CONSOLE_COUNT; i++) {
            if (!gConEmu.mp_VCon[i]) continue;
            if (gConEmu.mp_VCon[i]->RCon()->ConWnd() == hwnd ||
                gConEmu.mp_VCon[i]->RCon()->GetServerPID() == (DWORD)idObject)
            {
                gConEmu.mp_VCon[i]->RCon()->OnWinEvent(event, hwnd, idObject, idChild, dwEventThread, dwmsEventTime);
                lbProcessed = TRUE;
                break;
            }
        }
    }
        
    //switch(event)
    //{
    //case EVENT_CONSOLE_START_APPLICATION:
    //    //#pragma message("Win2k: CONSOLE_APPLICATION_16BIT")
    //    if (idChild == CONSOLE_APPLICATION_16BIT) {
    //        DWORD ntvdmPID = idObject;
    //        for (size_t i=0; i<gConEmu.m_Processes.size(); i++) {
    //            DWORD dwPID = gConEmu.m_Processes[i].ProcessID;
    //            if (dwPID == ntvdmPID) {
    //                gConEmu.mn_ActiveStatus |= CES_NTVDM;
    //                //TODO: их могут запускать и в разных консолях...
    //                SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    //            }
    //        }
    //    }
    //    break;
    //case EVENT_CONSOLE_END_APPLICATION:
    //    if (idChild == CONSOLE_APPLICATION_16BIT) {
    //        DWORD ntvdmPID = idObject;
    //        for (size_t i=0; i<gConEmu.m_Processes.size(); i++) {
    //            DWORD dwPID = gConEmu.m_Processes[i].ProcessID;
    //            if (dwPID == ntvdmPID) {
    //                gConEmu.gbPostUpdateWindowSize = true;
    //                gConEmu.mn_ActiveStatus &= ~CES_NTVDM;
    //                SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
    //            }
    //        }
    //    }
    //    break;
    //}
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
    m_Child.Invalidate();
    m_Back.Invalidate();
    TabBar.Invalidate();
}

LRESULT CConEmuMain::OnPaint(WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    PAINTSTRUCT ps;
    #ifdef _DEBUG
    HDC hDc = 
    #endif
    BeginPaint(ghWnd, &ps);

    //PaintGaps(hDc);

    EndPaint(ghWnd, &ps);
    result = DefWindowProc(ghWnd, WM_PAINT, wParam, lParam);

    return result;
}

void CConEmuMain::PaintCon()
{
    if (ProgressBars)
        ProgressBars->OnTimer();
    pVCon->Paint();
}

void CConEmuMain::RePaint()
{
    TabBar.RePaint();
    m_Back.RePaint();
    pVCon->Paint(); // если pVCon==NULL - будет просто выполнена заливка фоном.
}

void CConEmuMain::Update(bool isForce /*= false*/)
{
    if (isForce) {
        for (int i=0; i<MAX_CONSOLE_COUNT; i++) {
            if (mp_VCon[i])
                mp_VCon[i]->OnFontChanged();
        }
    }
    
    if (pVCon) {
        pVCon->Update(isForce);
        //InvalidateAll();
    }
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

DWORD CConEmuMain::GetFarPID()
{
    DWORD dwPID = 0;
    if (pVCon)
        dwPID = pVCon->RCon()->GetFarPID();
    return dwPID;
}

LPCTSTR CConEmuMain::GetTitle()
{
    if (!Title[0])
        return _T("ConEmu");
    return Title;
}

LPCTSTR CConEmuMain::GetTitle(int nIdx)
{
    if (nIdx<0 || nIdx>=MAX_CONSOLE_COUNT)
        return NULL;
    if (!mp_VCon[nIdx])
        return NULL;
    return mp_VCon[nIdx]->RCon()->GetTitle();
}

CVirtualConsole* CConEmuMain::GetVCon(int nIdx)
{
    if (nIdx<0 || nIdx>=MAX_CONSOLE_COUNT)
        return NULL;
    return mp_VCon[nIdx];
}

bool CConEmuMain::isActive(CVirtualConsole* apVCon)
{
    if (!this)
        return false;
        
    if (apVCon == pVCon)
        return true;
        
    return false;
}

bool CConEmuMain::isConSelectMode()
{
    //TODO: По курсору, что-ли попробовать определять?
    //return gb_ConsoleSelectMode;
    if (pVCon)
        return pVCon->RCon()->isConSelectMode();
    return false;
}

bool CConEmuMain::isDragging()
{
    return (mouse.state & (DRAG_L_STARTED | DRAG_R_STARTED)) != 0;
}

bool CConEmuMain::isFilePanel(bool abPluginAllowed/*=false*/)
{
    if (!pVCon) return false;
    return pVCon->RCon()->isFilePanel(abPluginAllowed);
}

bool CConEmuMain::isEditor()
{
    if (!pVCon) return false;
    return pVCon->RCon()->isEditor();
}

bool CConEmuMain::isFar()
{
    if (!pVCon) return false;
    return pVCon->RCon()->isFar();
}

bool CConEmuMain::isLBDown()
{
    return (mouse.state & DRAG_L_ALLOWED) == DRAG_L_ALLOWED;
}

bool CConEmuMain::isMainThread()
{
    DWORD dwTID = GetCurrentThreadId();
    return dwTID == mn_MainThreadId;
}

bool CConEmuMain::isMeForeground()
{
    if (!this) return false;
    
    static HWND hLastFore = NULL;
    static bool isMe = false;
    HWND h = GetForegroundWindow();
    if (h != hLastFore) {
        isMe = (h != NULL) && (h == ghWnd || h == ghOpWnd);
        if (h && !isMe) {
            for (int i=0; i<MAX_CONSOLE_COUNT; i++) {
                if (mp_VCon[i]) {
                    if (h == mp_VCon[i]->RCon()->ConWnd()) {
                        isMe = true; break;
                    }
                }
            }
        }
        hLastFore = h;
    }
    return isMe;
}

bool CConEmuMain::isNtvdm()
{
    if (!pVCon) return false;
    return pVCon->RCon()->isNtvdm();
}

bool CConEmuMain::isValid(CVirtualConsole* apVCon)
{
    if (!apVCon)
        return false;

    for (int i=0; i<MAX_CONSOLE_COUNT; i++) {
        if (apVCon == mp_VCon[i])
            return true;
    }
    
    return false;
}

bool CConEmuMain::isViewer()
{
    if (!pVCon) return false;
    return pVCon->RCon()->isViewer();
}

bool CConEmuMain::isWindowNormal()
{
    if (change2WindowMode != (DWORD)-1) {
        return (change2WindowMode == rNormal);
    }
    if (gSet.isFullScreen || IsZoomed(ghWnd) || IsIconic(ghWnd))
        return false;
    return true;
}

// Заголовок окна для PictureView вообще может пользователем настраиваться, так что
// рассчитывать на него при определения "Просмотра" - нельзя
bool CConEmuMain::isPictureView()
{
    bool lbRc = false;
    
    if (hPictureView && (!IsWindow(hPictureView) || !isFar())) {
        InvalidateAll();
        hPictureView = NULL;
    }

    bool lbPrevPicView = (hPictureView != NULL);

    if (pVCon && pVCon->RCon())
        hPictureView = pVCon->RCon()->isPictureView();
    else
        hPictureView = NULL;
    
    //if (!hPictureView) {
    //    hPictureView = FindWindowEx(ghWnd, NULL, L"FarPictureViewControlClass", NULL);
    //    if (!hPictureView)
    //        hPictureView = FindWindowEx(ghWndDC, NULL, L"FarPictureViewControlClass", NULL);
    //    if (!hPictureView) { // FullScreen?
    //        hPictureView = FindWindowEx(NULL, NULL, L"FarPictureViewControlClass", NULL);
    //    }
    //    if (hPictureView) {
    //        // Проверить на принадлежность фару
    //        DWORD dwPID, dwTID;
    //        dwTID = GetWindowThreadProcessId ( hPictureView, &dwPID );
    //        if (dwPID != pVCon->RCon()->GetFarPID())
    //            hPictureView = NULL;
    //    }
    //}

    lbRc = hPictureView!=NULL;

    // Если вызывали Help (F1) - окошко PictureView прячется
    if (hPictureView && !IsWindowVisible(hPictureView)) {
        lbRc = false;
        hPictureView = NULL;
    }
    if (bPicViewSlideShow && !hPictureView) {
        bPicViewSlideShow=false;
    }

    if (lbRc && !lbPrevPicView) {
        GetWindowRect(ghWnd, &mrc_WndPosOnPicView);
    } else if (!lbRc) {
        memset(&mrc_WndPosOnPicView, 0, sizeof(mrc_WndPosOnPicView));
    }

    return lbRc;
}

bool CConEmuMain::isSizing()
{
    // Юзер тащит мышкой рамку окна
    return (mouse.state & MOUSE_SIZING_BEGIN) == MOUSE_SIZING_BEGIN;
}

// Сюда может придти только LOWORD от HKL
void CConEmuMain::SwitchKeyboardLayout(DWORD dwNewKeybLayout)
{
    if ((gSet.isMonitorConsoleLang & 1) == 0)
        return;

    HKL hKeyb[10]; UINT nCount, i; BOOL lbFound = FALSE;
    nCount = GetKeyboardLayoutList ( countof(hKeyb), hKeyb );
    for (i = 0; !lbFound && i < nCount; i++) {
        if (hKeyb[i] == (HKL)dwNewKeybLayout)
            lbFound = TRUE;
    }
    for (i = 0; !lbFound && i < nCount; i++) {
        if ((LOWORD((DWORD)hKeyb[i]) == LOWORD(dwNewKeybLayout))) {
            lbFound = TRUE; dwNewKeybLayout = (DWORD)hKeyb[i];
        }
    }
    if (!lbFound && (dwNewKeybLayout == LOWORD(dwNewKeybLayout)))
        dwNewKeybLayout |= (dwNewKeybLayout << 16);

    // Может она сейчас и активна?
    if (dwNewKeybLayout != GetActiveKeyboardLayout()) {
        // Теперь переключаем раскладку
        PostMessage ( ghWnd, WM_INPUTLANGCHANGEREQUEST, 0, dwNewKeybLayout );
        PostMessage ( ghWnd, WM_INPUTLANGCHANGE, 0, dwNewKeybLayout );
    }
}

void CConEmuMain::TabCommand(UINT nTabCmd)
{
    if (!isMainThread()) {
        PostMessage(ghWnd, mn_MsgTabCommand, nTabCmd, 0);
        return;
    }

    switch (nTabCmd) {
        case 0:
            {
                if (TabBar.IsShown())
                    gSet.isTabs = 0;
                else
                    gSet.isTabs = 1;
                gConEmu.ForceShowTabs ( gSet.isTabs == 1 );
            } break;
        case 1:
            {
                TabBar.SwitchNext();
            } break;
        case 2:
            {
                TabBar.SwitchPrev();
            } break;
        case 3:
            {
                TabBar.SwitchCommit();
            } break;
    };
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

void CConEmuMain::OnBufferHeight(BOOL abBufferHeight)
{
    gConEmu.m_Back.TrackMouse(); // спрятать или показать прокрутку, если над ней мышка
    TabBar.OnBufferHeight(abBufferHeight);
}

TODO("И вообще, похоже это событие не вызывается");
LRESULT CConEmuMain::OnClose(HWND hWnd)
{
    _ASSERT(FALSE);
    //Icon.Delete(); - перенес в WM_DESTROY
    //mb_InClose = TRUE;
    if (ghConWnd) {
        pVCon->RCon()->CloseConsole();
    } else {
        Destroy();
    }
    //mb_InClose = FALSE;
    return 0;
}

//// Вызывается из ConEmuC, когда он запускается в режиме ComSpec
//LRESULT CConEmuMain::OnConEmuCmd(BOOL abStarted, HWND ahConWnd, DWORD anConEmuC_PID)
//{
//  for (int i=0; i<MAX_CONSOLE_COUNT; i++) {
//      if (mp_VCon[i] == NULL || mp_VCon[i]->RCon() == NULL) continue;
//      if (mp_VCon[i]->RCon()->hConWnd != ahConWnd) continue;
//
//      return mp_VCon[i]->RCon()->OnConEmuCmd(abStarted, anConEmuC_PID);
//  }
//
//  return 0;
//}

LRESULT CConEmuMain::OnCreate(HWND hWnd, LPCREATESTRUCT lpCreate)
{
    ghWnd = hWnd; // ставим сразу, чтобы функции могли пользоваться

	if (!mrc_Ideal.right) {
		// lpCreate->cx/cy может содержать CW_USEDEFAULT
		GetWindowRect(ghWnd, &mrc_Ideal);
	}


    Icon.LoadIcon(hWnd, gSet.nIconID/*IDI_ICON1*/);
    
    
    // Позволяет реагировать на запросы FlashWindow из фара и запуск приложений
    HMODULE hUser32 = GetModuleHandle(L"user32.dll");
    FRegisterShellHookWindow fnRegisterShellHookWindow = NULL;
    if (hUser32) fnRegisterShellHookWindow = (FRegisterShellHookWindow)GetProcAddress(hUser32, "RegisterShellHookWindow");
    if (fnRegisterShellHookWindow) fnRegisterShellHookWindow ( hWnd );
    
    
    // Чтобы можно было найти хэндл окна по хэндлу консоли
    SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)ghConWnd); // 31.03.2009 Maximus - только нихрена оно еще не создано!

    m_Back.Create();

    if (!m_Child.Create())
        return -1;

    mn_StartTick = GetTickCount();


    if (gSet.isGUIpb && !ProgressBars) {
    	ProgressBars = new CProgressBars(ghWnd, g_hInstance);
    }

    // Установить переменную среды с дескриптором окна
    SetConEmuEnvVar(ghWndDC);


    HMENU hwndMain = GetSystemMenu(ghWnd, FALSE), hDebug = NULL;
    InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_TOTRAY, _T("Hide to &tray"));
    InsertMenu(hwndMain, 0, MF_BYPOSITION, MF_SEPARATOR, 0);
    InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_HELP, _T("&About"));
    InsertMenu(hwndMain, 0, MF_BYPOSITION, MF_SEPARATOR, 0);
    hDebug = CreatePopupMenu();
    AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_CON_TOGGLE_VISIBLE, _T("&Real console"));
    AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_CONPROP, _T("&Properties..."));
    AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_DUMPCONSOLE, _T("&Dump..."));
    InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_POPUP | MF_ENABLED, (UINT_PTR)hDebug, _T("&Debug"));
    InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_CON_PASTE, _T("Paste"));
    InsertMenu(hwndMain, 0, MF_BYPOSITION, MF_SEPARATOR, 0);
    InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED | (gSet.AutoScroll ? MF_CHECKED : 0),
        ID_AUTOSCROLL, _T("Auto scro&ll..."));
    InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_SETTINGS, _T("S&ettings..."));


    if (gSet.isTabs==1) // "Табы всегда"
        ForceShowTabs(TRUE); // Показать табы

    //CreateCon();
    
    // Запустить серверную нить
    mh_ServerThreadTerminate = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (mh_ServerThreadTerminate) ResetEvent(mh_ServerThreadTerminate);
    mh_ServerThread = CreateThread(NULL, 0, ServerThread, (LPVOID)this, 0, &mn_ServerThreadId);
    
    return 0;
}

void CConEmuMain::PostCreate(BOOL abRecieved/*=FALSE*/)
{
    if (!abRecieved) {
        //if (gConEmu.WindowMode == rFullScreen || gConEmu.WindowMode == rMaximized) {
        #ifdef MSGLOGGER
        WINDOWPLACEMENT wpl; memset(&wpl, 0, sizeof(wpl)); wpl.length = sizeof(wpl);
        GetWindowPlacement(ghWnd, &wpl);
        #endif

        SetWindowMode(WindowMode);
        //} else {
        //ShowWindow(ghWnd, SW_SHOW);
        //UpdateWindow(ghWnd);
        //}
        
        PostMessage(ghWnd, mn_MsgPostCreate, 0, 0);
    } else {
        if (pVCon == NULL || !gConEmu.mb_StartDetached) { // Консоль уже может быть создана, если пришел Attach из ConEmuC
			RConStartArgs args; args.bDetached = gConEmu.mb_StartDetached;
            if (!CreateCon(&args)) {
                DisplayLastError(L"Can't create new virtual console!");
                Destroy();
                return;
            }
        }
        if (gConEmu.mb_StartDetached) gConEmu.mb_StartDetached = FALSE; // действует только на первую консоль

        HRESULT hr = S_OK;
        hr = OleInitialize (NULL); // как бы попробовать включать Ole только во время драга. кажется что из-за него глючит переключалка языка
        //CoInitializeEx(NULL, COINIT_MULTITHREADED);

         
        if (!mp_TaskBar) {
            hr = CoCreateInstance(CLSID_TaskbarList,NULL,CLSCTX_INPROC_SERVER,IID_ITaskbarList3,(void**)&mp_TaskBar);
            if (hr == S_OK && mp_TaskBar) {
                hr = mp_TaskBar->HrInit();
            }
            if (hr != S_OK && mp_TaskBar) {
                if (mp_TaskBar) mp_TaskBar->Release();
                mp_TaskBar = NULL;
            }
        }

        if (!mp_DragDrop) {
        	// было ghWndDC. Попробуем на главное окно, было бы удобно 
        	// "бросать" на таб (с автоматической активацией консоли)
            mp_DragDrop = new CDragDrop(ghWnd);
            if (!mp_DragDrop->Init()) {
            	CDragDrop *p = mp_DragDrop; mp_DragDrop = NULL;
            	delete p;
            }
        }
        //TODO terst
        WARNING("Если консоль не создана - handler не установится!")

        //SetConsoleCtrlHandler((PHANDLER_ROUTINE)CConEmuMain::HandlerRoutine, true);

        SetForegroundWindow(ghWnd);
        
        RegisterHotKeys();

        //SetParent(ghWnd, GetParent(GetShellWindow()));

        UINT n = SetTimer(ghWnd, 0, 500/*gSet.nMainTimerElapse*/, NULL);
        #ifdef _DEBUG
        DWORD dw = GetLastError();
        #endif
        n = 0;
    }
}

LRESULT CConEmuMain::OnDestroy(HWND hWnd)
{
	if (mb_MouseCaptured) {
		ReleaseCapture();
		mb_MouseCaptured = FALSE;
	}

    if (mh_ServerThread) {
        SetEvent(mh_ServerThreadTerminate);
        
        wchar_t szServerPipe[MAX_PATH];
        _ASSERTE(ghWnd!=NULL);
        wsprintf(szServerPipe, CEGUIPIPENAME, L".", (DWORD)ghWnd);
        
        HANDLE hPipe = CreateFile(szServerPipe,GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
        if (hPipe == INVALID_HANDLE_VALUE) {
            DEBUGSTR(L"All pipe instances closed?\n");
        } else {
            DEBUGSTR(L"Waiting server pipe thread\n");
            #ifdef _DEBUG
            DWORD dwWait = 
            #endif
            WaitForSingleObject(mh_ServerThread, 200); // пытаемся дождаться, пока нить завершится
            // Просто закроем пайп - его нужно было передернуть
            CloseHandle(hPipe);
            hPipe = INVALID_HANDLE_VALUE;
        }
        // Если нить еще не завершилась - прибить
        if (WaitForSingleObject(mh_ServerThread,0) != WAIT_OBJECT_0) {
            DEBUGSTR(L"### Terminating mh_ServerThread\n");
            TerminateThread(mh_ServerThread,0);
        }
        SafeCloseHandle(mh_ServerThread);
        SafeCloseHandle(mh_ServerThreadTerminate);
    }

    for (int i=0; i<MAX_CONSOLE_COUNT; i++) {
        if (mp_VCon[i]) {
            delete mp_VCon[i]; mp_VCon[i] = NULL;
        }
    }

    /*if (mh_ConMan && mh_ConMan!=INVALID_HANDLE_VALUE)
        FreeLibrary(mh_ConMan);
    mh_ConMan = NULL;*/

    if (mh_WinHook) {
        UnhookWinEvent(mh_WinHook);
        mh_WinHook = NULL;
    }
	//if (mh_PopupHook) {
	//	UnhookWinEvent(mh_PopupHook);
	//	mh_PopupHook = NULL;
	//}

    if (mp_DragDrop) {
        delete mp_DragDrop;
        mp_DragDrop = NULL;
    }
    if (ProgressBars) {
        delete ProgressBars;
        ProgressBars = NULL;
    }

    Icon.Delete();
    
    if (mp_TaskBar) {
        mp_TaskBar->Release();
        mp_TaskBar = NULL;
    }
    
    UnRegisterHotKeys();

    KillTimer(ghWnd, 0);

    PostQuitMessage(0);

    return 0;
}

LRESULT CConEmuMain::OnFocus(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
    BOOL lbSetFocus = FALSE;
    if (messg == WM_SETFOCUS)
        lbSetFocus = TRUE;
    else if (messg == WM_ACTIVATE)
        lbSetFocus = (LOWORD(wParam)==WA_ACTIVE) || (LOWORD(wParam)==WA_CLICKACTIVE);
    else if (messg == WM_ACTIVATEAPP)
        lbSetFocus = (wParam!=0);

    if (!lbSetFocus) {
        TabBar.SwitchRollback();
        
        UnRegisterHotKeys();
    }

    if (gSet.isSkipFocusEvents)
        return 0;
        
#ifdef MSGLOGGER
    /*if (messg == WM_ACTIVATE && wParam == WA_INACTIVE) {
        WCHAR szMsg[128]; wsprintf(szMsg, L"--Deactivating to 0x%08X\n", lParam);
        DEBUGSTR(szMsg);
    }
    switch (messg) {
        case WM_SETFOCUS: 
            {
                DEBUGSTR(L"--Get focus\n");
                //return 0;
            } break;
        case WM_KILLFOCUS: 
            {
                DEBUGSTR(L"--Loose focus\n");
            } break;
    }*/
#endif

    if (pVCon /*&& (messg == WM_SETFOCUS || messg == WM_KILLFOCUS)*/) {
        pVCon->RCon()->OnFocus(lbSetFocus);
    }
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

    //pInfo->ptMinTrackSize.x = srctWindow.X * (gSet.Log Font.lfWidth ? gSet.Log Font.lfWidth : 4)
    //  + p.x + shiftRect.left + shiftRect.right;

    //pInfo->ptMinTrackSize.y = srctWindow.Y * (gSet.Log Font.lfHeight ? gSet.Log Font.lfHeight : 6)
    //  + p.y + shiftRect.top + shiftRect.bottom;

    if (gSet.isFullScreen) {
        pInfo->ptMaxTrackSize = ptFullScreenSize;
        pInfo->ptMaxSize = ptFullScreenSize;
    }

    return result;
}

LRESULT CConEmuMain::OnKeyboard(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	wchar_t szTranslatedChars[11];
	int nTranslatedChars = 0;
	if (!GetKeyboardState(m_KeybStates)) {
		#ifdef _DEBUG
		DWORD dwErr = GetLastError();
		_ASSERTE(FALSE);
		#endif
		static bool sbErrShown = false;
		if (!sbErrShown) {
			sbErrShown = true;
			DisplayLastError(L"GetKeyboardState failed!");
		}
	} else {
		HKL hkl = (HKL)GetActiveKeyboardLayout();
		UINT nVK = wParam & 0xFFFF;
		UINT nSC = ((DWORD)lParam & 0xFF0000) >> 16;
		nTranslatedChars = ToUnicodeEx(nVK, nSC, m_KeybStates, szTranslatedChars, 10, 0, hkl);
		if (nTranslatedChars>0) szTranslatedChars[max(10,nTranslatedChars)] = 0; else szTranslatedChars[0] = 0;
	}
    //LRESULT result = 0;

  //  #ifdef _DEBUG
  //  {
  //      TCHAR szDbg[128];
  //      wsprintf(szDbg, _T("(msg=%i) %s - %c (%i)\n"),
        //  messg,
  //          ((messg == WM_KEYDOWN) ? _T("Dn") : _T("Up")),
  //          (TCHAR)wParam, wParam);
  //      //SetWindowText(ghWnd, szDbg);
        //OutputDebugString(szDbg);
  //  }
  //  #endif
  
    if (messg == WM_KEYDOWN && !mb_HotKeyRegistered)
    	RegisterHotKeys();

    if (messg == WM_KEYDOWN || messg == WM_KEYUP)
    {
        if (wParam == VK_PAUSE && !isPressed(VK_CONTROL)) {
            if (isPictureView()) {
                if (messg == WM_KEYUP) {
                    bPicViewSlideShow = !bPicViewSlideShow;
                    if (bPicViewSlideShow) {
                        if (gSet.nSlideShowElapse<=500) gSet.nSlideShowElapse=500;
                        //SetTimer(hWnd, 3, gSet.nSlideShowElapse, NULL);
                        dwLastSlideShowTick = GetTickCount() - gSet.nSlideShowElapse;
                    //} else {
                    //  KillTimer(hWnd, 3);
                    }
                }
                return 0;
            }
        } else if (bPicViewSlideShow) {
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
                bPicViewSlideShow = false; // отмена слайдшоу
            }
        }
    }

    // Прокрутка в "буферном" режиме
    if (gConEmu.pVCon->RCon()->isBufferHeight() && (messg == WM_KEYDOWN || messg == WM_KEYUP) &&
        (wParam == VK_DOWN || wParam == VK_UP || wParam == VK_NEXT || wParam == VK_PRIOR) &&
        isPressed(VK_CONTROL)
    ) {
        if (messg != WM_KEYDOWN || !pVCon)
            return 0;
            
        switch(wParam)
        {
        case VK_DOWN:
            return pVCon->RCon()->OnScroll(SB_LINEDOWN);
        case VK_UP:
            return pVCon->RCon()->OnScroll(SB_LINEUP);
        case VK_NEXT:
            return pVCon->RCon()->OnScroll(SB_PAGEDOWN);
        case VK_PRIOR:
            return pVCon->RCon()->OnScroll(SB_PAGEUP);
        }
        return 0;
    }
    
    //CtrlWinAltSpace
    WARNING("В висте, блин, не приходим сообщение на первое нажатие Space. Только на второе???");
    //TODO: Переделать на HotKey
    if (messg == WM_KEYDOWN && wParam == VK_SPACE && isPressed(VK_CONTROL) && isPressed(VK_LWIN) && isPressed(VK_MENU))
    {
    	CtrlWinAltSpace();
        
        return 0;
    }
    
    // Tabs
    if (/*gSet.isTabs &&*/ gSet.isTabSelf && /*TabBar.IsShown() &&*/
        (
         ((messg == WM_KEYDOWN || messg == WM_KEYUP) 
           && (wParam == VK_TAB 
               || (TabBar.IsInSwitch() 
                   && (wParam == VK_UP || wParam == VK_DOWN || wParam == VK_LEFT || wParam == VK_RIGHT))))
         //|| (messg == WM_CHAR && wParam == VK_TAB)
        ))
    {
        if (isPressed(VK_CONTROL) /*&& !isPressed(VK_MENU)*/ && !isPressed(VK_LWIN) && !isPressed(VK_LWIN))
        {
            if (TabBar.OnKeyboard(messg, wParam, lParam))
                return 0;
        }
    }
    // !!! Запрос на переключение мог быть инициирован из плагина
    if (messg == WM_KEYUP /*&& TabBar.IsShown()*/
        && (wParam == VK_CONTROL || wParam == VK_LCONTROL || wParam == VK_RCONTROL)
        && (gSet.isTabSelf || (gSet.isTabLazy && TabBar.IsInSwitch()))
       )
    {
        TabBar.SwitchCommit(); // Если переключения не было - ничего не делает
        // В фар отпускание кнопки таки пропустим
    }

    // Conman
    static bool sb_SkipConmanChar = false;
    static DWORD sn_SkipConmanVk[2] = {0,0};
    bool lbLWin = false, lbRWin = false;
    if (gSet.isMulti && wParam && ((lbLWin = isPressed(VK_LWIN)) || (lbRWin = isPressed(VK_RWIN)) || sb_SkipConmanChar)) {
        if (messg == WM_KEYDOWN && (lbLWin || lbRWin) && (wParam != VK_LWIN && wParam != VK_RWIN)) {
            if (wParam==gSet.icMultiNext || wParam==gSet.icMultiNew || (wParam>='0' && wParam<='9')
				|| ((lbLWin || lbRWin) && (wParam==VK_F11 || wParam==VK_F12)) // KeyDown для этого не проходит, но на всякий случай
                || wParam==gSet.icMultiRecreate)
            {
                // Запомнить, что не нужно пускать в консоль
                sb_SkipConmanChar = true;
                sn_SkipConmanVk[0] = lbLWin ? VK_LWIN : VK_RWIN;
                sn_SkipConmanVk[1] = lParam & 0xFF0000;
                
                // Теперь собственно обработка
                if (wParam>='1' && wParam<='9')
                    ConActivate(wParam - '1');
                    
                else if (wParam=='0')
                    ConActivate(9);
                    
                else if (wParam == gSet.icMultiNext)
                    ConActivateNext(isPressed(VK_SHIFT) ? FALSE : TRUE);
                    
                else if (wParam == gSet.icMultiNew) {
                    // Создать новую консоль
                    Recreate ( FALSE, gSet.isMultiNewConfirm );
                    
                } else if (wParam == gSet.icMultiRecreate) {
                    Recreate ( TRUE, TRUE );

                }
                return 0;
			}
        //} else if (messg == WM_CHAR) {
        //    if (sn_SkipConmanVk[1] == (lParam & 0xFF0000))
        //        return 0; // не пропускать букву в консоль
        } else if (messg == WM_KEYUP) {
			if ((lbLWin || lbRWin) && (wParam==VK_F11 || wParam==VK_F12)) {
				ConActivate(wParam - VK_F11 + 10);
				return 0;
			} else if (wParam == VK_LWIN || wParam == VK_RWIN) {
                if (sn_SkipConmanVk[0] == wParam) {
                    sn_SkipConmanVk[0] = 0;
                    sb_SkipConmanChar = (sn_SkipConmanVk[0] != 0) || (sn_SkipConmanVk[1] != 0);
                    return 0;
                }
            } else if (sn_SkipConmanVk[1] == (lParam & 0xFF0000)) {
                sn_SkipConmanVk[1] = 0;
                sb_SkipConmanChar = (sn_SkipConmanVk[0] != 0) || (sn_SkipConmanVk[1] != 0);
                return 0;
            }
        }
    }

    if (pVCon) {
        pVCon->RCon()->OnKeyboard(hWnd, messg, wParam, lParam, szTranslatedChars);
    }

    return 0;
}

LRESULT CConEmuMain::OnLangChange(UINT messg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 1;
    #ifdef MSGLOGGER
    {
        WCHAR szMsg[128];
        wsprintf(szMsg, L"%s(CP:%i, HKL:0x%08X)\n",
            (messg == WM_INPUTLANGCHANGE) ? L"WM_INPUTLANGCHANGE" : L"WM_INPUTLANGCHANGEREQUEST",
            wParam, lParam);
        DEBUGSTR(szMsg);

    }
    #endif
    
    //POSTMESSAGE(ghConWnd, messg, wParam, lParam, FALSE);

    //mn_CurrentKeybLayout = lParam;

    result = DefWindowProc(ghWnd, messg, wParam, lParam);

    pVCon->RCon()->SwitchKeyboardLayout(lParam);
    
  //  if (isFar() && gSet.isLangChangeWsPlugin)
  //  {
     //   //LONG lLastLang = GetWindowLong ( ghWndDC, GWL_LANGCHANGE );
     //   //SetWindowLong ( ghWndDC, GWL_LANGCHANGE, lParam );
     //   
     //   /*if (lLastLang == lParam)
        //    return result;*/
     //   
        //CConEmuPipe pipe(GetFarPID(), 10);
        //if (pipe.Init(_T("CConEmuMain::OnLangChange"), FALSE))
        //{
        //  if (pipe.Execute(CMD_LANGCHANGE, &lParam, sizeof(LPARAM)))
        //  {
        //      //gConEmu.DnDstep(_T("ConEmu: Switching language (1 sec)"));
        //      // Подождем немножко, проверим что плагин живой
        //      /*DWORD dwWait = WaitForSingleObject(pipe.hEventAlive, CONEMUALIVETIMEOUT);
        //      if (dwWait == WAIT_OBJECT_0)*/
        //          return result;
        //  }
        //}
  //  }

  //  //POSTMESSAGE(ghConWnd, messg, wParam, lParam, FALSE);
  //  //SENDMESSAGE(ghConWnd, messg, wParam, lParam);

  //  //if (messg == WM_INPUTLANGCHANGEREQUEST)
  //  {
  //      //wParam Specifies the character set of the new locale. 
  //      //lParam - HKL
  //      //ActivateKeyboardLayout((HKL)lParam, 0);

  //      //POSTMESSAGE(ghConWnd, messg, wParam, lParam, FALSE);
  //      //SENDMESSAGE(ghConWnd, messg, wParam, lParam);
  //  }

  //  if (messg == WM_INPUTLANGCHANGE)
  //  {
  //      //SENDMESSAGE(ghConWnd, WM_SETFOCUS, 0,0);
  //      //POSTMESSAGE(ghConWnd, WM_SETFOCUS, 0,0, TRUE);
  //      //POSTMESSAGE(ghWnd, WM_SETFOCUS, 0,0, TRUE);
  //  }

    return result;
}

LRESULT CConEmuMain::OnMouse(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	// кто его знает, в каких координатах оно пришло...
    //short winX = GET_X_LPARAM(lParam);
    //short winY = GET_Y_LPARAM(lParam);

	RECT conRect = {0}, dcRect = {0}; GetWindowRect(ghWndDC, &dcRect);
	POINT ptCur = {-1, -1}; GetCursorPos(&ptCur);

	//BOOL lbMouseWasCaptured = mb_MouseCaptured;
	if (!mb_MouseCaptured) {
		// Если клик
		if (isPressed(VK_LBUTTON) || isPressed(VK_RBUTTON) || isPressed(VK_MBUTTON)) {
			// В клиентской области (области отрисовки)
			if (PtInRect(&dcRect, ptCur)) {
				mb_MouseCaptured = TRUE;
				SetCapture(ghWndDC);
			}
		}
	} else {
		// Все кнопки мышки отпущены - release
		if (!isPressed(VK_LBUTTON) && !isPressed(VK_RBUTTON) && !isPressed(VK_MBUTTON)) {
			ReleaseCapture();
			mb_MouseCaptured = FALSE;

			if (pVCon->RCon()->isMouseButtonDown()) {
				if (ptCur.x < dcRect.left)
					ptCur.x = dcRect.left;
				else if (ptCur.x >= dcRect.right)
					ptCur.x = dcRect.right-1;

				if (ptCur.y < dcRect.top)
					ptCur.y = dcRect.top;
				else if (ptCur.y >= dcRect.bottom)
					ptCur.y = dcRect.bottom-1;
			}
		}
	}
	
    if (!pVCon)
        return 0;

    if (gSet.FontWidth()==0 || gSet.FontHeight()==0)
        return 0;

#ifdef MSGLOGGER
    if (messg != WM_MOUSEMOVE) {
        TODO("SetConsoleMode");
        /*DWORD mode = 0;
        BOOL lb = GetConsoleMode(pVCon->hConIn(), &mode);
        if (!(mode & ENABLE_MOUSE_INPUT)) {
            mode |= ENABLE_MOUSE_INPUT;
            DEBUGSTR(L"!!! Enabling mouse input in console\n");
            lb = SetConsoleMode(pVCon->hConIn(), mode);
        }*/
    }
#endif

    if ((messg==WM_LBUTTONUP || messg==WM_MOUSEMOVE) && (gConEmu.mouse.state & MOUSE_SIZING_DBLCKL)) {
        if (messg==WM_LBUTTONUP)
            gConEmu.mouse.state &= ~MOUSE_SIZING_DBLCKL;
        return 0; //2009-04-22 После DblCkl по заголовку в консоль мог проваливаться LBUTTONUP
    }

    if (messg == WM_MOUSEMOVE) {
        if (m_Back.TrackMouse())
            return 0;
    }

    // Иначе в консоль проваливаются щелчки по незанятому полю таба...
    //if (messg==WM_LBUTTONDOWN || messg==WM_RBUTTONDOWN || messg==WM_MBUTTONDOWN || 
    //    messg==WM_LBUTTONDBLCLK || messg==WM_RBUTTONDBLCLK || messg==WM_MBUTTONDBLCLK)
    if (messg != WM_MOUSEMOVE) // 04.06.2009 Maks - 
    {
        if (gConEmu.mouse.state & MOUSE_SIZING_DBLCKL)
            gConEmu.mouse.state &= ~MOUSE_SIZING_DBLCKL;

        if (!PtInRect(&dcRect, ptCur)) {
            DEBUGLOGFILE("Click outside of DC");
            return 0;
        }
    }

	// Переводим в клиентские координаты
	ptCur.x -= dcRect.left; ptCur.y -= dcRect.top;
    GetClientRect(ghConWnd, &conRect);

    COORD cr = pVCon->ClientToConsole(ptCur.x,ptCur.y);
    short conX = cr.X; //winX/gSet.Log Font.lfWidth;
    short conY = cr.Y; //winY/gSet.Log Font.lfHeight;

    if (conY<0 || conY<0) {
        DEBUGLOGFILE("Mouse outside of upper-left");
        return 0;
    }
    
    if (gSet.isMouseSkipMoving && GetForegroundWindow() != ghWnd) {
    	DEBUGLOGFILE("ConEmu is not foreground window, mouse event skipped");
    	return 0;
    }

    if ((gConEmu.mouse.nSkipEvents[0] && gConEmu.mouse.nSkipEvents[0] == messg)
        || (gConEmu.mouse.nSkipEvents[1] 
	        && (gConEmu.mouse.nSkipEvents[1] == messg || messg == WM_MOUSEMOVE)))
    {
		if (gConEmu.mouse.nSkipEvents[0] == messg) {
			gConEmu.mouse.nSkipEvents[0] = 0;
			DEBUGSTR(L"Skipping Mouse down\n");
		} else
		if (gConEmu.mouse.nSkipEvents[1] == messg) {
			gConEmu.mouse.nSkipEvents[1] = 0;
			DEBUGSTR(L"Skipping Mouse up\n");
		} 
		#ifdef _DEBUG
		else if (messg == WM_MOUSEMOVE) {
			DEBUGSTR(L"Skipping Mouse move\n");
		}
		#endif
    	DEBUGLOGFILE("ConEmu was not foreground window, mouse activation event skipped");
    	return 0;
    }
    
    if (mouse.nReplaceDblClk) {
		if (!gSet.isMouseSkipActivation) {
			mouse.nReplaceDblClk = 0;
		} else {
    		if (messg == WM_LBUTTONDOWN && mouse.nReplaceDblClk == WM_LBUTTONDBLCLK) {
    			mouse.nReplaceDblClk = 0;
    		} else
    		if (messg == WM_RBUTTONDOWN && mouse.nReplaceDblClk == WM_RBUTTONDBLCLK) {
    			mouse.nReplaceDblClk = 0;
    		} else
    		if (messg == WM_MBUTTONDOWN && mouse.nReplaceDblClk == WM_MBUTTONDBLCLK) {
    			mouse.nReplaceDblClk = 0;
    		} else
     		if (mouse.nReplaceDblClk == messg) {
        		switch (mouse.nReplaceDblClk) {
        		case WM_LBUTTONDBLCLK:
        			messg = WM_LBUTTONDOWN;
        			break;
        		case WM_RBUTTONDBLCLK:
        			messg = WM_RBUTTONDOWN;
        			break;
        		case WM_MBUTTONDBLCLK:
        			messg = WM_MBUTTONDOWN;
        			break;
        		}
        		mouse.nReplaceDblClk = 0;
    		}
		}
    }

    ///*&& isPressed(VK_LBUTTON)*/) && // Если этого не делать - при выделении мышкой консоль может самопроизвольно прокрутиться
    CRealConsole* pRCon = pVCon->RCon();
    // Только при скрытой консоли. Иначе мы ConEmu видеть вообще не будем
    if (pRCon && (/*pRCon->isBufferHeight() ||*/ !pRCon->isWindowVisible())
        && (messg == WM_LBUTTONDOWN || messg == WM_RBUTTONDOWN || messg == WM_LBUTTONDBLCLK || messg == WM_RBUTTONDBLCLK
            || (messg == WM_MOUSEMOVE && isPressed(VK_LBUTTON)))
        )
    {
       // buffer mode: cheat the console window: adjust its position exactly to the cursor
       RECT win;
       GetWindowRect(ghWnd, &win);
       short x = win.left + ptCur.x - MulDiv(ptCur.x, conRect.right, klMax<uint>(1, pVCon->Width));
       short y = win.top + ptCur.y - MulDiv(ptCur.y, conRect.bottom, klMax<uint>(1, pVCon->Height));
       RECT con;
       GetWindowRect(ghConWnd, &con);
       if (con.left != x || con.top != y)
           MOVEWINDOW(ghConWnd, x, y, con.right - con.left + 1, con.bottom - con.top + 1, TRUE);
    }
    
    if (!isFar()) {
        if (messg != WM_MOUSEMOVE) { DEBUGLOGFILE("FAR not active, all clicks forced to console"); }
        goto fin;
    }
    
    if (messg == WM_MOUSEMOVE)
    {
        // WM_MOUSEMOVE вроде бы слишком часто вызывается даже при условии что курсор не двигается...
        if (wParam==mouse.lastMMW && lParam==mouse.lastMML) {
            return 0;
        }
        mouse.lastMMW=wParam; mouse.lastMML=lParam;
        
        // 18.03.2009 Maks - Если уже тащим - мышь не слать
        if (isDragging()) {
            // может флажок случайно остался?
            if ((mouse.state & DRAG_L_STARTED) && ((wParam & MK_LBUTTON)==0))
                mouse.state &= ~(DRAG_L_STARTED | DRAG_L_ALLOWED);
            if ((mouse.state & DRAG_R_STARTED) && ((wParam & MK_RBUTTON)==0))
                mouse.state &= ~(DRAG_R_STARTED | DRAG_R_ALLOWED);

            if (mouse.state & (DRAG_L_STARTED | DRAG_R_STARTED))
                return 0;
        }


        //TODO: вроде бы иногда isSizing() не сбрасывается?
        //? может так: if (gSet.isDragEnabled & mouse.state)
        if ((gSet.isDragEnabled & (mouse.state & (DRAG_L_ALLOWED|DRAG_R_ALLOWED)))!=0)
        {
			if (mp_DragDrop==NULL) {
				DnDstep(_T("DnD: Drag-n-Drop is null"));
			} else {
				mouse.bIgnoreMouseMove = true;

				BOOL lbDiffTest = PTDIFFTEST(cursor,DRAG_DELTA); // TRUE - если курсор НЕ двигался (далеко)
				if (!lbDiffTest && !isDragging() && !isSizing())
				{
					// 2009-06-19 Выполнялось сразу после "if (gSet.isDragEnabled & (mouse.state & (DRAG_L_ALLOWED|DRAG_R_ALLOWED)))"
					if (mouse.state & MOUSE_R_LOCKED) // чтобы при RightUp не ушел APPS
						mouse.state &= ~MOUSE_R_LOCKED;

					BOOL lbLeftDrag = (mouse.state & DRAG_L_ALLOWED) == DRAG_L_ALLOWED;

					pVCon->RCon()->LogString(lbLeftDrag ? "Left drag about to start" : "Right drag about to start");

					// Если сначала фокус был на файловой панели, но после LClick он попал на НЕ файловую - отменить ShellDrag
					bool bFilePanel = isFilePanel();
					if (!bFilePanel) {
						DnDstep(_T("DnD: not file panel"));
						//isLBDown = false; 
						mouse.state &= ~(DRAG_L_ALLOWED | DRAG_L_STARTED | DRAG_R_ALLOWED | DRAG_R_STARTED);
						mouse.bIgnoreMouseMove = false;
						//POSTMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( newX, newY ), TRUE);     //посылаем консоли отпускание
						pVCon->RCon()->OnMouse(WM_LBUTTONUP, wParam, ptCur.x, ptCur.x);
						//ReleaseCapture(); --2009-03-14
						return 0;
					}
	                
					// Чтобы сам фар не дергался на MouseMove...
					//isLBDown = false;
					mouse.state &= ~(DRAG_L_ALLOWED | DRAG_L_STARTED | DRAG_R_STARTED); // флажок поставит сам CDragDrop::Drag() по наличию DRAG_R_ALLOWED
					// вроде валится, если перед Drag
					//POSTMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( newX, newY ), TRUE);     //посылаем консоли отпускание

					if (lbLeftDrag) { // сразу "отпустить" клавишу
						//POSTMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( mouse.LClkCon.X, mouse.LClkCon.Y ), TRUE);     //посылаем консоли отпускание
						pVCon->RCon()->OnMouse(WM_LBUTTONUP, wParam, mouse.LClkDC.X, mouse.LClkDC.Y);
					} else {
						//POSTMESSAGE(ghConWnd, WM_LBUTTONDOWN, wParam, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);     //посылаем консоли отпускание
						pVCon->RCon()->OnMouse(WM_LBUTTONDOWN, wParam, mouse.RClkDC.X, mouse.RClkDC.Y);
						//POSTMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);     //посылаем консоли отпускание
						pVCon->RCon()->OnMouse(WM_LBUTTONUP, wParam, mouse.RClkDC.X, mouse.RClkDC.Y);
					}

					pVCon->RCon()->FlushInputQueue();
	                
					// Иначе иногда срабатывает FAR'овский D'n'D
					//SENDMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( newX, newY ));     //посылаем консоли отпускание
					if (mp_DragDrop) {
						DnDstep(_T("DnD: Drag-n-Drop starting"));
						mp_DragDrop->Drag(); //сдвинулись при зажатой левой
						DnDstep(Title); // вернуть заголовок
					} else {
						_ASSERTE(mp_DragDrop); // должно быть обработано выше
						DnDstep(_T("DnD: Drag-n-Drop is null"));
					}
					mouse.bIgnoreMouseMove = false;
	                
					//#ifdef NEWMOUSESTYLE
					//newX = cursor.x; newY = cursor.y;
					//#else
					//newX = MulDiv(cursor.x, conRect.right, klMax<uint>(1, pVCon->Width));
					//newY = MulDiv(cursor.y, conRect.bottom, klMax<uint>(1, pVCon->Height));
					//#endif
					//if (lbLeftDrag)
					//  POSTMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( newX, newY ), TRUE);     //посылаем консоли отпускание
					//isDragProcessed=false; -- убрал, иначе при бросании в пассивную панель больших файлов дроп может вызваться еще раз???
					return 0;
				}
			}
        }
        else if (gSet.isRClickSendKey && (mouse.state & MOUSE_R_LOCKED))
        {
            //Если двинули мышкой, а была включена опция RClick - не вызывать
            //контекстное меню - просто послать правый клик
            if (!PTDIFFTEST(Rcursor, RCLICKAPPSDELTA))
            {
                //isRBDown=false;
                mouse.state &= ~(DRAG_R_ALLOWED | DRAG_R_STARTED | MOUSE_R_LOCKED);
                //POSTMESSAGE(ghConWnd, WM_RBUTTONDOWN, 0, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);
                pVCon->RCon()->OnMouse(WM_RBUTTONDOWN, 0, mouse.RClkDC.X, mouse.RClkDC.Y);
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
        
        if (mouse.bIgnoreMouseMove)
            return 0;
        
    } else {
        mouse.lastMMW=-1; mouse.lastMML=-1;

        if (messg == WM_LBUTTONDOWN)
        {
            //if (isLBDown()) ReleaseCapture(); // Вдруг сглючило? --2009-03-14
            //isLBDown = false;
            mouse.state &= ~(DRAG_L_ALLOWED | DRAG_L_STARTED);
            mouse.bIgnoreMouseMove = false;
            mouse.LClkCon = MakeCoord(conX,conY);
            mouse.LClkDC = MakeCoord(ptCur.x,ptCur.y);
            if (!isConSelectMode() && isFilePanel() && pVCon &&
				pVCon->RCon()->CoordInPanel(mouse.LClkCon))
            {
                //SetCapture(ghWndDC); --2009-03-14
                cursor.x = LOWORD(lParam);
                cursor.y = HIWORD(lParam); 
                //isLBDown=true;
                //isDragProcessed=false;
                if (gSet.isDragEnabled & DRAG_L_ALLOWED) {
                    if (!gSet.nLDragKey || isPressed(gSet.nLDragKey))
                        mouse.state = DRAG_L_ALLOWED;
                }
                //if (gSet.is DnD) mouse.bIgnoreMouseMove = true;
                //POSTMESSAGE(ghConWnd, messg, wParam, MAKELPARAM( newX, newY ), FALSE); // было SEND
                pVCon->RCon()->OnMouse(messg, wParam, ptCur.x, ptCur.y);
                return 0;
            }
        }
        else if (messg == WM_LBUTTONUP)
        {
            BOOL lbLDrag = (mouse.state & DRAG_L_STARTED) == DRAG_L_STARTED;
            mouse.state &= ~(DRAG_L_ALLOWED | DRAG_L_STARTED);
            if (lbLDrag)
                return 0; // кнопка уже "отпущена"
            if (mouse.bIgnoreMouseMove) {
                mouse.bIgnoreMouseMove = false;
                //#ifdef NEWMOUSESTYLE
                //newX = cursor.x; newY = cursor.y;
                //#else
                //newX = MulDiv(cursor.x, conRect.right, klMax<uint>(1, pVCon->Width));
                //newY = MulDiv(cursor.y, conRect.bottom, klMax<uint>(1, pVCon->Height));
                //#endif
                //POSTMESSAGE(ghConWnd, messg, wParam, MAKELPARAM( newX, newY ), FALSE);
                pVCon->RCon()->OnMouse(messg, wParam, cursor.x, cursor.y);
                return 0;
            }
        }
        else if (messg == WM_RBUTTONDOWN)
        {
            Rcursor.x = LOWORD(lParam);
            Rcursor.y = HIWORD(lParam);
            mouse.RClkCon = MakeCoord(conX,conY);
            mouse.RClkDC = MakeCoord(ptCur.x,ptCur.y);
            //isRBDown=false;
            mouse.state &= ~(DRAG_R_ALLOWED | DRAG_R_STARTED | MOUSE_R_LOCKED);
            mouse.bIgnoreMouseMove = false;

            //if (isFilePanel()) // Maximus5
            if (!isConSelectMode() && isFilePanel() && pVCon &&
				pVCon->RCon()->CoordInPanel(mouse.RClkCon))
            {
                if (gSet.isDragEnabled & DRAG_R_ALLOWED) {
                    if (!gSet.nRDragKey || isPressed(gSet.nRDragKey)) {
                        mouse.state = DRAG_R_ALLOWED;
                        return 0;
                    }
                }
                // Если ничего лишнего не нажато!
                if (gSet.isRClickSendKey && !(wParam&(MK_CONTROL|MK_LBUTTON|MK_MBUTTON|MK_SHIFT|MK_XBUTTON1|MK_XBUTTON2)))
                {
                    //заведем таймер на .3
                    //если больше - пошлем apps
                    mouse.state |= MOUSE_R_LOCKED; mouse.bSkipRDblClk = false;
                    //SetTimer(hWnd, 1, 300, 0); -- Maximus5, откажемся от таймера
                    mouse.RClkTick = GetTickCount();
                    return 0;
                }
            }
        }
        else if (messg == WM_RBUTTONUP)
        {
            if (gSet.isRClickSendKey && (mouse.state & MOUSE_R_LOCKED))
            {
                //isRBDown=false; // сразу сбросим!
                mouse.state &= ~(DRAG_R_ALLOWED | DRAG_R_STARTED | MOUSE_R_LOCKED);
                if (PTDIFFTEST(Rcursor,RCLICKAPPSDELTA))
                {
                    //держали зажатой <.3
                    //убьем таймер, кликнием правой кнопкой
                    //KillTimer(hWnd, 1); -- Maximus5, таймер более не используется
                    DWORD dwCurTick=GetTickCount();
                    DWORD dwDelta=dwCurTick-mouse.RClkTick;
                    // Если держали дольше .3с, но не слишком долго :)
                    if ((gSet.isRClickSendKey==1) ||
                        (dwDelta>RCLICKAPPSTIMEOUT && dwDelta<10000))
                    {
                        // Сначала выделить файл под курсором
                        //POSTMESSAGE(ghConWnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);
                        pVCon->RCon()->OnMouse(WM_LBUTTONDOWN, MK_LBUTTON, mouse.RClkDC.X, mouse.RClkDC.Y);
                        //POSTMESSAGE(ghConWnd, WM_LBUTTONUP, 0, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);
                        pVCon->RCon()->OnMouse(WM_LBUTTONUP, 0, mouse.RClkDC.X, mouse.RClkDC.Y);
                        
                        pVCon->RCon()->FlushInputQueue();
                    
                        //pVCon->Update(true);
                        //INVALIDATE(); //InvalidateRect(HDCWND, NULL, FALSE);

                        // А теперь можно и Apps нажать
                        mouse.bSkipRDblClk=true; // чтобы пока FAR думает в консоль не проскочило мышиное сообщение
                        //POSTMESSAGE(ghConWnd, WM_KEYDOWN, VK_APPS, 0, TRUE);

                        DWORD dwFarPID = pVCon->RCon()->GetFarPID();
                        if (dwFarPID) AllowSetForegroundWindow(dwFarPID);

                        if (gSet.sRClickMacro && *gSet.sRClickMacro) {
                            // Если юзер задал свой макрос - выполняем его
                            PostMacro(gSet.sRClickMacro);
                        } else {
							INPUT_RECORD r = {KEY_EVENT};
                            //pVCon->RCon()->On Keyboard(ghConWnd, WM_KEYDOWN, VK_APPS, (VK_APPS << 16) | (1 << 24));
                            //pVCon->RCon()->On Keyboard(ghConWnd, WM_KEYUP, VK_APPS, (VK_APPS << 16) | (1 << 24));
							r.Event.KeyEvent.bKeyDown = TRUE;
							r.Event.KeyEvent.wVirtualKeyCode = VK_APPS;
							r.Event.KeyEvent.wVirtualScanCode = /*28 на моей клавиатуре*/MapVirtualKey(VK_APPS, 0/*MAPVK_VK_TO_VSC*/);
							r.Event.KeyEvent.dwControlKeyState = 0x120;
							pVCon->RCon()->PostConsoleEvent(&r);

							//On Keyboard(hConWnd, WM_KEYUP, VK_RETURN, 0);
							r.Event.KeyEvent.bKeyDown = FALSE;
							r.Event.KeyEvent.dwControlKeyState = 0x120;
							pVCon->RCon()->PostConsoleEvent(&r);
                        }
                        return 0;
                    } else {
                        char szLog[100];
                        wsprintfA(szLog, "RightClicked, but condition failed (RCSK:%i, Delay:%i)", (int)gSet.isRClickSendKey, (int)dwDelta);
                        pVCon->RCon()->LogString(szLog);
                    }
                } else {
                    char szLog[100];
                    wsprintfA(szLog, "RightClicked, but mouse was moved {%i-%i}->{%i-%i}", Rcursor.x, Rcursor.y, LOWORD(lParam), HIWORD(lParam));
                    pVCon->RCon()->LogString(szLog);
                }
                // Иначе нужно сначала послать WM_RBUTTONDOWN
                //POSTMESSAGE(ghConWnd, WM_RBUTTONDOWN, wParam, MAKELPARAM( newX, newY ), TRUE);
                pVCon->RCon()->OnMouse(WM_RBUTTONDOWN, wParam, ptCur.x, ptCur.y);
            } else {
                char szLog[100];
                wsprintfA(szLog, "RightClicked, but condition failed (RCSK:%i, State:%u)", (int)gSet.isRClickSendKey, (DWORD)mouse.state);
                pVCon->RCon()->LogString(szLog);
            }
            //isRBDown=false; // чтобы не осталось случайно
            mouse.state &= ~(DRAG_R_ALLOWED | DRAG_R_STARTED | MOUSE_R_LOCKED);
        }
        else if (messg == WM_RBUTTONDBLCLK) {
            if (mouse.bSkipRDblClk) {
                mouse.bSkipRDblClk = false;
                return 0; // не обрабатывать, сейчас висит контекстное меню
            }
        }
    }

#ifdef MSGLOGGER
    if (messg == WM_MOUSEMOVE)
        messg = WM_MOUSEMOVE;
#endif
fin:
    // заменим даблклик вторым обычным
    //POSTMESSAGE(ghConWnd, messg == WM_RBUTTONDBLCLK ? WM_RBUTTONDOWN : messg, wParam, MAKELPARAM( newX, newY ), FALSE);
    pVCon->RCon()->OnMouse(messg == WM_RBUTTONDBLCLK ? WM_RBUTTONDOWN : messg, wParam, ptCur.x, ptCur.y);
    return 0;
}

LRESULT CConEmuMain::OnShellHook(WPARAM wParam, LPARAM lParam)
{
    /*
wParam lParam 
HSHELL_GETMINRECT A pointer to a SHELLHOOKINFO structure.  
HSHELL_WINDOWACTIVATEED The HWND handle of the activated window.  
HSHELL_RUDEAPPACTIVATEED The HWND handle of the activated window.  
HSHELL_WINDOWREPLACING The HWND handle of the window replacing the top-level window.  
HSHELL_WINDOWREPLACED The HWND handle of the window being replaced.  
HSHELL_WINDOWCREATED The HWND handle of the window being created.  
HSHELL_WINDOWDESTROYED The HWND handle of the top-level window being destroyed.  
HSHELL_ACTIVATESHELLWINDOW Not used.  
HSHELL_TASKMAN Can be ignored.  
HSHELL_REDRAW The HWND handle of the window that needs to be redrawn.  
HSHELL_FLASH The HWND handle of the window that needs to be flashed.  
HSHELL_ENDTASK The HWND handle of the window that should be forced to exit.  
HSHELL_APPCOMMAND The APPCOMMAND which has been unhandled by the application or other hooks. See WM_APPCOMMAND and use the GET_APPCOMMAND_LPARAM macro to retrieve this parameter.  
    */
    switch (wParam) {
    case HSHELL_FLASH:
        {
            HWND hCon = (HWND)lParam;
            if (!hCon) return 0;
            for (int i = 0; i<MAX_CONSOLE_COUNT; i++) {
                if (!mp_VCon[i]) continue;
                if (mp_VCon[i]->RCon()->ConWnd() == hCon) {
                    FLASHWINFO fl = {sizeof(FLASHWINFO)};
                    if (isMeForeground()) {
                    	if (mp_VCon[i] != pVCon) { // Только для неактивной консоли
                            fl.dwFlags = FLASHW_STOP; fl.hwnd = ghWnd;
                            FlashWindowEx(&fl); // Чтобы мигание не накапливалось
                    		fl.uCount = 4; fl.dwFlags = FLASHW_ALL; fl.hwnd = ghWnd;
                    		FlashWindowEx(&fl);
                    	}
                    } else {
                    	fl.dwFlags = FLASHW_ALL|FLASHW_TIMERNOFG; fl.hwnd = ghWnd;
                    	FlashWindowEx(&fl); // Помигать в GUI
                    }
                    
                    fl.dwFlags = FLASHW_STOP; fl.hwnd = hCon;
                    FlashWindowEx(&fl);
                    break;
                }
            }
        }
        break;
    case HSHELL_WINDOWCREATED:
        {
            if (isMeForeground()) {
                HWND hWnd = (HWND)lParam;
                if (!hWnd) return 0;
                DWORD dwPID = 0, dwParentPID = 0, dwFarPID = 0;
                GetWindowThreadProcessId(hWnd, &dwPID);
                if (dwPID && dwPID != GetCurrentProcessId()) {
                    AllowSetForegroundWindow(dwPID);
                    
                    if (IsWindowVisible(hWnd)) // ? оно успело ?
                    {
                    	// Получить PID родительского процесса этого окошка
                    	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
                        if (hSnap != INVALID_HANDLE_VALUE) {
                            PROCESSENTRY32 prc = {sizeof(PROCESSENTRY32)};
                            if (Process32First(hSnap, &prc)) {
                                do {
                                    if (prc.th32ProcessID == dwPID) {
                                        dwParentPID = prc.th32ParentProcessID;
                                        break;
                                    }
                                } while (Process32Next(hSnap, &prc));
                            }
                            CloseHandle(hSnap);
                        }
                    	
                    	for (int i = 0; i<MAX_CONSOLE_COUNT; i++) {
                    		if (mp_VCon[i] == NULL || mp_VCon[i]->RCon() == NULL) continue;
                    		dwFarPID = mp_VCon[i]->RCon()->GetFarPID();
                    		if (!dwFarPID) continue;
                    		
                    		if (dwPID == dwFarPID || dwParentPID == dwFarPID) { // MSDN Topics
                    			SetForegroundWindow(hWnd);
                    			break;
                    		}
                    	}
                    }
                
                    //#ifdef _DEBUG
                    //wchar_t szTitle[255], szClass[255], szMsg[1024];
                    //GetWindowText(hWnd, szTitle, 254); GetClassName(hWnd, szClass, 254);
                    //wsprintf(szMsg, L"Window was created:\nTitle: %s\nClass: %s\nPID: %i", szTitle, szClass, dwPID);
                    //MBox(szMsg);
                    //#endif
                }
            }
        }
        break;
    }
    return 0;
}

LRESULT CConEmuMain::OnSysCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;
    switch(LOWORD(wParam))
    {
    case ID_SETTINGS:
        if (ghOpWnd && IsWindow(ghOpWnd)) {
            if (!ShowWindow ( ghOpWnd, SW_SHOWNORMAL ))
                DisplayLastError(L"Can't show settings window");
            SetFocus ( ghOpWnd );
            break; // А то открывались несколько окон диалогов :)
        }
        //DialogBox((HINSTANCE)GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG1), 0, CSettings::wndOpProc);
        CSettings::Dialog();
        return 0;
        //break;
    case ID_CON_PASTE:
        pVCon->RCon()->Paste();
        return 0;
        //break;
    case ID_AUTOSCROLL:
        gSet.AutoScroll = !gSet.AutoScroll;
        CheckMenuItem(GetSystemMenu(ghWnd, FALSE), ID_AUTOSCROLL, MF_BYCOMMAND |
            (gSet.AutoScroll ? MF_CHECKED : MF_UNCHECKED));
        return 0;
        //break;
    case ID_DUMPCONSOLE:
        if (pVCon)
            pVCon->DumpConsole();
        return 0;
        //break;
    case ID_CON_TOGGLE_VISIBLE:
        if (pVCon)
            pVCon->RCon()->ShowConsole(-1); // Toggle visibility
        return 0;
    case ID_HELP:
        {
            WCHAR szTitle[255];
            #ifdef _DEBUG
            wsprintf(szTitle, L"About ConEmu (%s [DEBUG])", szConEmuVersion);
            #else
            wsprintf(szTitle, L"About ConEmu (%s)", szConEmuVersion);
            #endif
            BOOL b = gbDontEnable; gbDontEnable = TRUE;
            MessageBoxW(ghWnd, pHelp, szTitle, MB_ICONQUESTION);
            gbDontEnable = b;
        }
        return 0;
        //break;
    case ID_TOTRAY:
        Icon.HideWindowToTray();
        return 0;
        //break;
    case ID_CONPROP:
        #ifdef MSGLOGGER
        {
            HMENU hMenu = GetSystemMenu(ghConWnd, FALSE);
            MENUITEMINFO mii; TCHAR szText[255];
            for (int i=0; i<15; i++) {
                memset(&mii, 0, sizeof(mii));
                mii.cbSize = sizeof(mii); mii.dwTypeData=szText; mii.cch=255;
                mii.fMask = MIIM_ID|MIIM_STRING|MIIM_SUBMENU;
                if (GetMenuItemInfo(hMenu, i, TRUE, &mii)) {
                    mii.cbSize = sizeof(mii);
                    if (mii.hSubMenu) {
                        MENUITEMINFO mic;
                        for (int i=0; i<15; i++) {
                            memset(&mic, 0, sizeof(mic));
                            mic.cbSize = sizeof(mic); mic.dwTypeData=szText; mic.cch=255;
                            mic.fMask = MIIM_ID|MIIM_STRING;
                            if (GetMenuItemInfo(mii.hSubMenu, i, TRUE, &mic)) {
                                mic.cbSize = sizeof(mic);
                            } else {
                                break;
                            }
                        }
                    }
                } else
                    break;
            }
        }
        #endif
        POSTMESSAGE(ghConWnd, WM_SYSCOMMAND, SC_PROPERTIES_SECRET/*65527*/, 0, TRUE);
        return 0;
        //break;
    }

    switch(wParam)
    {
    case SC_MAXIMIZE_SECRET:
        SetWindowMode(rMaximized);
        break;
    case SC_RESTORE_SECRET:
        SetWindowMode(rNormal);
        break;
    case SC_CLOSE:
        //Icon.Delete();
        //SENDMESSAGE(ghConWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
        //SENDMESSAGE(ghConWnd ? ghConWnd : ghWnd, WM_CLOSE, 0, 0); // ?? фар не ловит сообщение, ExitFAR не вызываются
        {
            int nConCount = 0, nDetachedCount = 0;
            for (int i=(MAX_CONSOLE_COUNT-1); i>=0; i--) {
                if (mp_VCon[i] && mp_VCon[i]->RCon()) {
                    if (mp_VCon[i]->RCon()->isDetached()) {
                        nDetachedCount ++;
                        continue;
                    }
                    nConCount ++;
                    if (mp_VCon[i]->RCon()->ConWnd()) {
                        mp_VCon[i]->RCon()->CloseConsole();
                    }
                }
            }
            if (nConCount == 0) {
                if (nDetachedCount > 0) {
                    if (MessageBox(ghWnd, L"ConEmu is waiting for console attach.\nIt was started in 'Detached' mode.\nDo You want to cancel waiting?",
                        L"ConEmu", MB_YESNO|MB_ICONQUESTION) != IDYES)
                    return result;
                }
                Destroy();
            }
        }
        break;

    case SC_MAXIMIZE:
        if (!mb_PassSysCommand) {
            if (isPictureView())
                break;;
            SetWindowMode(rMaximized);
        } else {
            result = DefWindowProc(hWnd, WM_SYSCOMMAND, wParam, lParam);
        }
        break;
    case SC_RESTORE:
        if (!mb_PassSysCommand) {
            if (!IsIconic(ghWnd) && isPictureView())
                break;
            if (!SetWindowMode(IsIconic(ghWnd) ? WindowMode : rNormal))
                result = DefWindowProc(hWnd, WM_SYSCOMMAND, wParam, lParam);
        } else {
            result = DefWindowProc(hWnd, WM_SYSCOMMAND, wParam, lParam);
        }
        break;
    case SC_MINIMIZE:
        if (gSet.isMinToTray) {
            Icon.HideWindowToTray();
            break;
        }
        result = DefWindowProc(hWnd, WM_SYSCOMMAND, wParam, lParam);
        break;

    default:
        if (wParam != 0xF100)
        {
            // иначе это приводит к потере фокуса и активации невидимой консоли,
            // перехвате стрелок клавиатуры, и прочей фигни...
            if (wParam<0xF000) {
                POSTMESSAGE(ghConWnd, WM_SYSCOMMAND, wParam, lParam, FALSE);
            }
            result = DefWindowProc(hWnd, WM_SYSCOMMAND, wParam, lParam);
        }
    }
    return result;
}

WARNING("Частота хождения таймера в винде оставляет желать... нужно от него избавляться и по возможности переносить все в нити");
LRESULT CConEmuMain::OnTimer(WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;
    
    //if (mb_InTimer) return 0; // чтобы ненароком два раза в одно событие не вошел (хотя не должен)
    mb_InTimer = TRUE;
    //result = gConEmu.OnTimer(wParam, lParam);

    switch (wParam)
    {
    case 0:
        //Maximus5. Hack - если какая-то зараза задизеблила окно
        if (!gbDontEnable) {
            DWORD dwStyle = GetWindowLong(ghWnd, GWL_STYLE);
            if (dwStyle & WS_DISABLED)
                EnableWindow(ghWnd, TRUE);
        }

        CheckProcesses(); //m_ActiveConmanIDX, FALSE/*bTitleChanged*/);

        TODO("Теперь это условие не работает. 1 - раньше это был сам ConEmu.exe");
        if (m_ProcCount == 0) {
            // При ошибках запуска консольного приложения хотя бы можно будет увидеть, что оно написало...
            if (mb_ProcessCreated) {
                Destroy();
                break;
            }
        } else if (!mb_ProcessCreated && m_ProcCount>=1) {
            if ((GetTickCount() - mn_StartTick)>PROCESS_WAIT_START_TIME)
                mb_ProcessCreated = TRUE;
        }


        // TODO: поддержку SlideShow повесить на отдельный таймер
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

        
        //2009-04-22 - вроде не требуется
        /*if (lbIsPicView && !isPiewUpdate)
        {
            // чтобы принудительно обновиться после закрытия PicView
            isPiewUpdate = true; 
        }*/

        if (!lbIsPicView && isPiewUpdate)
        {   // После скрытия/закрытия PictureView нужно передернуть консоль - ее размер мог измениться
            isPiewUpdate = false;
            SyncConsoleToWindow();
            //INVALIDATE(); //InvalidateRect(HDCWND, NULL, FALSE);
            InvalidateAll();
        }


        if (!IsIconic(ghWnd))
        {
            // Было ли реальное изменение размеров?
            BOOL lbSizingToDo  = (mouse.state & MOUSE_SIZING_TODO) == MOUSE_SIZING_TODO;

            if (isSizing() && !isPressed(VK_LBUTTON)) {
                // Сборс всех флагов ресайза мышкой
                mouse.state &= ~(MOUSE_SIZING_BEGIN|MOUSE_SIZING_TODO);
            }

            TODO("возможно весь ресайз (кроме SyncNtvdm?) нужно перенести в нить консоли")

            //COORD c = ConsoleSizeFromWindow();
            RECT client; GetClientRect(ghWnd, &client);
            // Проверим, вдруг не отработал IsIconic
            if (client.bottom > 10 )
            {
                RECT c = CalcRect(CER_CONSOLE, client, CER_MAINCLIENT);
                // чтобы не насиловать консоль лишний раз - реальное измененение ее размеров только
                // при отпускании мышкой рамки окна
                BOOL lbSizeChanged = FALSE;
                if (pVCon) {
                    lbSizeChanged = (c.right != (int)pVCon->RCon()->TextWidth() 
                            || c.bottom != (int)pVCon->RCon()->TextHeight());
                }
                if (!isSizing() &&
                    (lbSizingToDo /*после реального ресайза мышкой*/ ||
                     gbPostUpdateWindowSize /*после появления/скрытия табов*/ || 
                     lbSizeChanged /*или размер в виртуальной консоли не совпадает с расчетным*/))
                {
                    gbPostUpdateWindowSize = false;
                    if (isNtvdm())
                        SyncNtvdm();
                    else {
                        if (!gSet.isFullScreen && !IsZoomed(ghWnd) && !lbSizingToDo)
                            SyncWindowToConsole();
                        else
                            SyncConsoleToWindow();
                        OnSize(0, client.right, client.bottom);
                    }
                    //_ASSERTE(pVCon!=NULL);
                    if (pVCon) {
                        m_LastConSize = MakeCoord(pVCon->TextWidth,pVCon->TextHeight);
                    }
					// Запомнить "идеальный" размер окна, выбранный пользователем
					if (lbSizingToDo && !gSet.isFullScreen && !IsZoomed(ghWnd) && !IsIconic(ghWnd)) {
						GetWindowRect(ghWnd, &mrc_Ideal);
					}
                }
                else if (pVCon 
                    && (m_LastConSize.X != (int)pVCon->TextWidth 
                        || m_LastConSize.Y != (int)pVCon->TextHeight))
                {
                    // По идее, сюда мы попадаем только для 16-бит приложений
                    if (isNtvdm())
                        SyncNtvdm();
                    m_LastConSize = MakeCoord(pVCon->TextWidth,pVCon->TextHeight);
                }
            }

            // update scrollbar
            pVCon->RCon()->UpdateScrollInfo();
        }
    }

    mb_InTimer = FALSE;

    return result;
}

// Вызовем UpdateScrollPos для АКТИВНОЙ консоли
LRESULT CConEmuMain::OnUpdateScrollInfo(BOOL abPosted/* = FALSE*/)
{
    if (!abPosted) {
        PostMessage(ghWnd, mn_MsgUpdateScrollInfo, 0, (LPARAM)mp_VCon);
        return 0;
    }

    if (!pVCon)
        return 0;
    pVCon->RCon()->UpdateScrollInfo();
    return 0;
}

LRESULT CConEmuMain::OnVConTerminated(CVirtualConsole* apVCon, BOOL abPosted /*= FALSE*/)
{
    _ASSERTE(apVCon);
    if (!apVCon)
        return 0;

    if (!abPosted) {
        PostMessage(ghWnd, mn_MsgVConTerminated, 0, (LPARAM)apVCon);
        return 0;
    }

    for (int i=0; i<MAX_CONSOLE_COUNT; i++) {
        if (mp_VCon[i] == apVCon) {
            mp_VCon[i] = NULL;

            WARNING("Вообще-то это нужно бы в CriticalSection закрыть. Несколько консолей может одновременно закрыться");
            if (pVCon == apVCon) {
                for (int j=(i-1); j>=0; j--) {
                    if (mp_VCon[j]) {
                        ConActivate(j);
                        break;
                    }
                }
                if (pVCon == apVCon) {
                    for (int j=(i+1); j<MAX_CONSOLE_COUNT; j++) {
                        if (mp_VCon[j]) {
                            ConActivate(j);
                            break;
                        }
                    }
                }
            }
            for (int j=(i+1); j<MAX_CONSOLE_COUNT; j++) {
                mp_VCon[j-1] = mp_VCon[j];
            }
			mp_VCon[MAX_CONSOLE_COUNT-1] = NULL;
            if (pVCon == apVCon)
                pVCon = NULL;
            delete apVCon;
            break;
        }
    }
    TODO("Alternative?");
    TabBar.OnConsoleActivated(ActiveConNum()+1/*, FALSE*/);
    return 0;
}

DWORD CConEmuMain::ServerThread(LPVOID lpvParam)
{ 
    BOOL fConnected = FALSE;
    DWORD dwErr = 0;
    HANDLE hPipe = NULL; 
    #ifdef _DEBUG
    DWORD dwTID = GetCurrentThreadId();
    #endif
    wchar_t szServerPipe[MAX_PATH];

    _ASSERTE(ghWnd!=NULL);
    wsprintf(szServerPipe, CEGUIPIPENAME, L".", (DWORD)ghWnd);

    // The main loop creates an instance of the named pipe and 
    // then waits for a client to connect to it. When the client 
    // connects, a thread is created to handle communications 
    // with that client, and the loop is repeated.
    

    // Пока окно не закрыто
    do {
        while (!fConnected)
        { 
            _ASSERTE(hPipe == NULL);

            hPipe = CreateNamedPipe( 
                szServerPipe,             // pipe name 
                PIPE_ACCESS_DUPLEX,       // read/write access 
                PIPE_TYPE_MESSAGE |       // message type pipe 
                PIPE_READMODE_MESSAGE |   // message-read mode 
                PIPE_WAIT,                // blocking mode 
                PIPE_UNLIMITED_INSTANCES, // max. instances  
                PIPEBUFSIZE,              // output buffer size 
                PIPEBUFSIZE,              // input buffer size 
                0,                        // client time-out 
                gpNullSecurity);          // default security attribute 

            _ASSERTE(hPipe != INVALID_HANDLE_VALUE);

            if (hPipe == INVALID_HANDLE_VALUE) 
            {
                //DisplayLastError(L"CreateNamedPipe failed"); 
                hPipe = NULL;
                Sleep(50);
                continue;
            }

            // Wait for the client to connect; if it succeeds, 
            // the function returns a nonzero value. If the function
            // returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 

            fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : ((dwErr = GetLastError()) == ERROR_PIPE_CONNECTED); 

            if ((dwErr = WaitForSingleObject ( gConEmu.mh_ServerThreadTerminate, 0 )) == WAIT_OBJECT_0) {
                SafeCloseHandle(hPipe);
                return 0; // GUI закрывается
            }
            
            if (!fConnected)
                SafeCloseHandle(hPipe);
        }

        if (fConnected) {
            // сразу сбросим, чтобы не забыть
            fConnected = FALSE;

            gConEmu.ServerThreadCommand ( hPipe ); // При необходимости - записывает в пайп результат сама
        }

        FlushFileBuffers(hPipe); 
        //DisconnectNamedPipe(hPipe); 
        SafeCloseHandle(hPipe);
    } // Перейти к открытию нового instance пайпа
    while (WaitForSingleObject ( gConEmu.mh_ServerThreadTerminate, 0 ) != WAIT_OBJECT_0);

    return 0; 
}

// Эта функция пайп не закрывает!
void CConEmuMain::ServerThreadCommand(HANDLE hPipe)
{
    CESERVER_REQ in={{0}}, *pIn=NULL;
    DWORD cbRead = 0, cbWritten = 0, dwErr = 0;
    BOOL fSuccess = FALSE;

    // Send a message to the pipe server and read the response. 
    fSuccess = ReadFile( 
        hPipe,            // pipe handle 
        &in,              // buffer to receive reply
        sizeof(in),       // size of read buffer
        &cbRead,          // bytes read
        NULL);            // not overlapped 

    if (!fSuccess && ((dwErr = GetLastError()) != ERROR_MORE_DATA)) 
    {
        _ASSERTE("ReadFile(pipe) failed"==NULL);
        //CloseHandle(hPipe);
        return;
    }
    if (in.hdr.nVersion != CESERVER_REQ_VER) {
        gConEmu.ShowOldCmdVersion(in.hdr.nCmd, in.hdr.nVersion);
        return;
    }
    _ASSERTE(in.hdr.nSize>=sizeof(CESERVER_REQ_HDR) && cbRead>=sizeof(CESERVER_REQ_HDR));
    if (cbRead < sizeof(CESERVER_REQ_HDR) || /*in.hdr.nSize < cbRead ||*/ in.hdr.nVersion != CESERVER_REQ_VER) {
        //CloseHandle(hPipe);
        return;
    }

    if (in.hdr.nSize <= cbRead) {
        pIn = &in; // выделение памяти не требуется
    } else {
        int nAllSize = in.hdr.nSize;
        pIn = (CESERVER_REQ*)calloc(nAllSize,1);
        _ASSERTE(pIn!=NULL);
        memmove(pIn, &in, cbRead);
        _ASSERTE(pIn->hdr.nVersion==CESERVER_REQ_VER);

        LPBYTE ptrData = ((LPBYTE)pIn)+cbRead;
        nAllSize -= cbRead;

        while(nAllSize>0)
        { 
            //_tprintf(TEXT("%s\n"), chReadBuf);

            // Break if TransactNamedPipe or ReadFile is successful
            if(fSuccess)
                break;

            // Read from the pipe if there is more data in the message.
            fSuccess = ReadFile( 
                hPipe,      // pipe handle 
                ptrData,    // buffer to receive reply 
                nAllSize,   // size of buffer 
                &cbRead,    // number of bytes read 
                NULL);      // not overlapped 

            // Exit if an error other than ERROR_MORE_DATA occurs.
            if( !fSuccess && ((dwErr = GetLastError()) != ERROR_MORE_DATA)) 
                break;
            ptrData += cbRead;
            nAllSize -= cbRead;
        }

        TODO("Может возникнуть ASSERT, если консоль была закрыта в процессе чтения");
        _ASSERTE(nAllSize==0);
        if (nAllSize>0) {
            //CloseHandle(hPipe);
            return; // удалось считать не все данные
        }
    }

    #ifdef _DEBUG
    UINT nDataSize = pIn->hdr.nSize - sizeof(CESERVER_REQ_HDR);
    #endif
    // Все данные из пайпа получены, обрабатываем команду и возвращаем (если нужно) результат
    if (pIn->hdr.nCmd == CECMD_NEWCMD) {
        DEBUGSTR(L"GUI recieved CECMD_NEWCMD\n");
    
        pIn->Data[0] = FALSE;
        pIn->hdr.nSize = sizeof(CESERVER_REQ_HDR) + 1;

        if (IsIconic(ghWnd))
            SendMessage(ghWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
        SetForegroundWindow(ghWnd);

		RConStartArgs args; args.pszSpecialCmd = pIn->NewCmd.szCommand;
        CVirtualConsole* pCon = CreateCon(&args);
		args.pszSpecialCmd = NULL;
        if (pCon) {
            pIn->Data[0] = TRUE;
        }
        
        // Отправляем
        fSuccess = WriteFile( 
            hPipe,        // handle to pipe 
            pIn,         // buffer to write from 
            pIn->hdr.nSize,  // number of bytes to write 
            &cbWritten,   // number of bytes written 
            NULL);        // not overlapped I/O 

    } else if (pIn->hdr.nCmd == CECMD_TABSCMD) {
        // 0: спрятать/показать табы, 1: перейти на следующую, 2: перейти на предыдущую, 3: commit switch
        DEBUGSTR(L"GUI recieved CECMD_TABSCMD\n");
        _ASSERTE(nDataSize>=1);
        DWORD nTabCmd = pIn->Data[0];
        TabCommand(nTabCmd);
    }

    // Освободить память
    if (pIn && (LPVOID)pIn != (LPVOID)&in) {
        free(pIn); pIn = NULL;
    }

    return;
}

LRESULT CConEmuMain::MainWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	if (messg == WM_CREATE) {
		if (ghWnd == NULL)
			ghWnd = hWnd; // ставим сразу, чтобы функции могли пользоваться
		else if (ghWndDC == NULL)
			ghWndDC = hWnd; // ставим сразу, чтобы функции могли пользоваться
	}

	if (hWnd == ghWnd)
		result = gConEmu.WndProc(hWnd, messg, wParam, lParam);
	else if (hWnd == ghWndDC)
		result = gConEmu.m_Child.ChildWndProc(hWnd, messg, wParam, lParam);
	else if (messg)
		result = DefWindowProc(hWnd, messg, wParam, lParam);

	return result;
}

LRESULT CConEmuMain::WndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    //MCHKHEAP

    #ifdef _DEBUG
    wchar_t szDbg[127]; wsprintfW(szDbg, L"WndProc(%i{x%03X},%i,%i)\n", messg, messg, wParam, lParam);
    //OutputDebugStringW(szDbg);
    #endif

    if (messg == WM_SYSCHAR) // Вернул. Для пересылки в консоль не используется, но чтобы не пищало - необходимо
        return TRUE;
    //if (messg == WM_CHAR)
    //  return TRUE;

    switch (messg)
    {
	case WM_CREATE:
		result = gConEmu.OnCreate(hWnd, (LPCREATESTRUCT)lParam);
		break;

    case WM_NOTIFY:
    {
        result = TabBar.OnNotify((LPNMHDR)lParam);
        break;
    }

    case WM_COMMAND:
    {
        TabBar.OnCommand(wParam, lParam);
        result = 0;
        break;
    }
    
    case WM_ERASEBKGND:
        return 0;
        
    case WM_PAINT:
        result = gConEmu.OnPaint(wParam, lParam);
        break;
    
    case WM_TIMER:
    //    if (mb_InTimer) break; // чтобы ненароком два раза в одно событие не вошел (хотя не должен)
    //    gSet.Performance(tPerfInterval, TRUE); // именно обратный отсчет. Мы смотрим на промежуток МЕЖДУ таймерами
    //    mb_InTimer = TRUE;
      result = gConEmu.OnTimer(wParam, lParam);
    //  mb_InTimer = FALSE;
    //  gSet.Performance(tPerfInterval, FALSE);
        break;

    case WM_SIZING:
        if (!IsIconic(ghWnd))
            result = gConEmu.OnSizing(wParam, lParam);
        break;

    case WM_SIZE:
        if (!IsIconic(ghWnd))
            result = gConEmu.OnSize(wParam, LOWORD(lParam), HIWORD(lParam));
        break;

    case WM_MOVE:
        // вызывается когда не надо...
        if (hWnd == ghWnd /*&& ghOpWnd*/) { //2009-05-08 запоминать wndX/wndY всегда, а не только если окно настроек открыто
            if (!gSet.isFullScreen && !IsZoomed(ghWnd) && !IsIconic(ghWnd))
            {
                RECT rc; GetWindowRect(ghWnd, &rc);
                gSet.UpdatePos(rc.left, rc.top);
                if (hPictureView)
                    mrc_WndPosOnPicView = rc;
            }
        } else if (hPictureView) {
            GetWindowRect(ghWnd, &mrc_WndPosOnPicView);
        }
        break;

    case WM_GETMINMAXINFO:
        {
            LPMINMAXINFO pInfo = (LPMINMAXINFO)lParam;
            result = gConEmu.OnGetMinMaxInfo(pInfo);
            break;
        }

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    //case WM_CHAR: -- убрал. Теперь пользуем ToUnicodeEx.
    //case WM_SYSCHAR:
        result = gConEmu.OnKeyboard(hWnd, messg, wParam, lParam);
        //if (messg == WM_SYSCHAR)
        //    return TRUE;
        break;

    case WM_ACTIVATE:
        #ifdef MSGLOGGER
        result = gConEmu.OnFocus(hWnd, messg, wParam, lParam);
        result = DefWindowProc(hWnd, messg, wParam, lParam);
        break;
        #endif
    case WM_ACTIVATEAPP:
        #ifdef MSGLOGGER
        result = gConEmu.OnFocus(hWnd, messg, wParam, lParam);
        result = DefWindowProc(hWnd, messg, wParam, lParam);
        break;
        #endif
    case WM_KILLFOCUS:
        #ifdef MSGLOGGER
        result = gConEmu.OnFocus(hWnd, messg, wParam, lParam);
        result = DefWindowProc(hWnd, messg, wParam, lParam);
        break;
        #endif
    case WM_SETFOCUS:
        result = gConEmu.OnFocus(hWnd, messg, wParam, lParam);
        result = DefWindowProc(hWnd, messg, wParam, lParam);
        break;

    case WM_MOUSEACTIVATE:
    	//return MA_ACTIVATEANDEAT; -- ест все подряд, а LBUTTONUP пропускает :(
		gConEmu.mouse.nSkipEvents[0] = 0;
		gConEmu.mouse.nSkipEvents[1] = 0;
		if (gSet.isMouseSkipActivation && LOWORD(lParam) == HTCLIENT && GetForegroundWindow() != ghWnd) {
			POINT ptMouse = {0}; GetCursorPos(&ptMouse);
			RECT  rcDC = {0}; GetWindowRect(ghWndDC, &rcDC);
			if (PtInRect(&rcDC, ptMouse)) {
            	if (HIWORD(lParam) == WM_LBUTTONDOWN) {
            		gConEmu.mouse.nSkipEvents[0] = WM_LBUTTONDOWN;
            		gConEmu.mouse.nSkipEvents[1] = WM_LBUTTONUP;
            		gConEmu.mouse.nReplaceDblClk = WM_LBUTTONDBLCLK;
            	} else if (HIWORD(lParam) == WM_RBUTTONDOWN) {
            		gConEmu.mouse.nSkipEvents[0] = WM_RBUTTONDOWN;
            		gConEmu.mouse.nSkipEvents[1] = WM_RBUTTONUP;
            		gConEmu.mouse.nReplaceDblClk = WM_RBUTTONDBLCLK;
            	} else if (HIWORD(lParam) == WM_MBUTTONDOWN) {
            		gConEmu.mouse.nSkipEvents[0] = WM_MBUTTONDOWN;
            		gConEmu.mouse.nSkipEvents[1] = WM_MBUTTONUP;
            		gConEmu.mouse.nReplaceDblClk = WM_MBUTTONDBLCLK;
            	}
            }
        }
    	result = DefWindowProc(hWnd, messg, wParam, lParam);
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
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_XBUTTONDBLCLK:
        result = gConEmu.OnMouse(hWnd, messg, wParam, lParam);
        break;

    case WM_CLOSE:
        result = gConEmu.OnClose(hWnd);
        break;

    case WM_SYSCOMMAND:
        result = gConEmu.OnSysCommand(hWnd, wParam, lParam);
        break;

    case WM_NCLBUTTONDOWN:
        // При ресайзе WM_NCRBUTTONUP к сожалению не приходит
        gConEmu.mouse.state |= MOUSE_SIZING_BEGIN;
        result = DefWindowProc(hWnd, messg, wParam, lParam);
        break;

    case WM_NCLBUTTONDBLCLK:
        gConEmu.mouse.state |= MOUSE_SIZING_DBLCKL;
        result = DefWindowProc(hWnd, messg, wParam, lParam);
        break;

    case WM_NCRBUTTONUP:
        Icon.HideWindowToTray();
        break;

    case WM_TRAYNOTIFY:
        result = Icon.OnTryIcon(hWnd, messg, wParam, lParam);
        break; 

	case WM_HOTKEY:
		if (wParam == 0x201) {
			CtrlWinAltSpace();
		}
		return 0;

    case WM_DESTROY:
        result = gConEmu.OnDestroy(hWnd);
        break;
    
    case WM_IME_NOTIFY:
        break;
    case WM_INPUTLANGCHANGE:
    case WM_INPUTLANGCHANGEREQUEST:
        if (hWnd == ghWnd)
            result = gConEmu.OnLangChange(messg, wParam, lParam);
        else
            break;
        break;
        
    default:
        if (messg == gConEmu.mn_MsgPostCreate) {
            gConEmu.PostCreate(TRUE);
            return 0;
        } else 
        if (messg == gConEmu.mn_MsgPostCopy) {
            gConEmu.PostCopy((wchar_t*)lParam, TRUE);
            return 0;
        } else
        if (messg == gConEmu.mn_MsgMyDestroy) {
            gConEmu.OnDestroy(hWnd);
            return 0;
        } else if (messg == gConEmu.mn_MsgUpdateSizes) {
            gConEmu.UpdateSizes();
            return 0;
        } else if (messg == gConEmu.mn_MsgSetWindowMode) {
            gConEmu.SetWindowMode(wParam);
            return 0;
        } else if (messg == gConEmu.mn_MsgUpdateTitle) {
            //gConEmu.UpdateTitle(TitleCmp);
            gConEmu.UpdateTitle(pVCon->RCon()->GetTitle());
            return 0;
        } else if (messg == gConEmu.mn_MsgAttach) {
            return gConEmu.AttachRequested ( (HWND)wParam, (DWORD)lParam );
        } else if (messg == gConEmu.mn_MsgVConTerminated) {
            return gConEmu.OnVConTerminated ( (CVirtualConsole*)lParam, TRUE );
        } else if (messg == gConEmu.mn_MsgUpdateScrollInfo) {
            return OnUpdateScrollInfo(TRUE);
        } else if (messg == gConEmu.mn_MsgUpdateTabs) {
            TabBar.Update(TRUE);
            return 0;
        } else if (messg == gConEmu.mn_MsgOldCmdVer) {
            gConEmu.ShowOldCmdVersion(wParam, lParam);
            return 0;
        } else if (messg == gConEmu.mn_MsgTabCommand) {
            gConEmu.TabCommand(wParam);
            return 0;
        } else if (messg == gConEmu.mn_MsgSheelHook) {
            gConEmu.OnShellHook(wParam, lParam);
            return 0;
		} else if (messg == gConEmu.mn_ShellExecuteEx) {
			return gConEmu.GuiShellExecuteEx((SHELLEXECUTEINFO*)lParam, wParam);
		}
        //else if (messg == gConEmu.mn_MsgCmdStarted || messg == gConEmu.mn_MsgCmdStopped) {
        //  return gConEmu.OnConEmuCmd( (messg == gConEmu.mn_MsgCmdStarted), (HWND)wParam, (DWORD)lParam);
        //}

        if (messg) result = DefWindowProc(hWnd, messg, wParam, lParam);
    }
    return result;
}
