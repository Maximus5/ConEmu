
#define SHOWDEBUGSTR

#include "Header.h"
#include <Tlhelp32.h>
#include <Shlobj.h>
#include "../common/ConEmuCheck.h"

#define DEBUGSTRSYS(s) //DEBUGSTR(s)
#define DEBUGSTRSIZE(s) //DEBUGSTR(s)
#define DEBUGSTRCONS(s) //DEBUGSTR(s)
#define DEBUGSTRTABS(s) //DEBUGSTR(s)
#define DEBUGSTRLANG(s) //DEBUGSTR(s)// ; Sleep(2000)
#define DEBUGSTRMOUSE(s) //DEBUGSTR(s)
#define DEBUGSTRSETCURSOR(s) //DEBUGSTR(s)

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
	mp_TabBar = NULL;
	ms_ConEmuAliveEvent[0] = 0;	mb_AliveInitialized = FALSE; mh_ConEmuAliveEvent = NULL; mb_ConEmuAliveOwned = FALSE;

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
    //ProgressBars = NULL;
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
    //mn_CurrentKeybLayout = (DWORD_PTR)GetKeyboardLayout(0);
    mn_ServerThreadId = 0; mh_ServerThread = NULL; mh_ServerThreadTerminate = NULL;
    //mpsz_RecreateCmd = NULL;
	ZeroStruct(mrc_Ideal);
	mn_InResize = 0;
	mb_MouseCaptured = FALSE;
	mb_HotKeyRegistered = FALSE;
	mb_WaitCursor = FALSE;
	mh_CursorWait = LoadCursor(NULL, IDC_WAIT);
	mh_CursorArrow = LoadCursor(NULL, IDC_ARROW);
	mh_CursorAppStarting = LoadCursor(NULL, IDC_APPSTARTING);

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
    lstrcpy(ms_ConEmuChm, ms_ConEmuExe); lstrcat(ms_ConEmuChm, L"\\ConEmu.chm");
    SetEnvironmentVariable(L"ConEmuDir", ms_ConEmuExe);
    *pszSlash = L'\\';
    
    DWORD dwAttr = GetFileAttributes(ms_ConEmuChm);
    if (dwAttr == (DWORD)-1 || (dwAttr & FILE_ATTRIBUTE_DIRECTORY))
    	ms_ConEmuChm[0] = 0;

	ms_ConEmuCExe[0] = 0;
	if (ms_ConEmuExe[0] == L'\\' /*|| wcschr(ms_ConEmuExe, L' ') == NULL*/) // Сетевые пути не менять
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
    	

    memset(&m_osv,0,sizeof(m_osv));	
    m_osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&m_osv);
	if (m_osv.dwMajorVersion >= 6) {
		mb_IsUacAdmin = IsUserAdmin(); // Чтобы знать, может мы уже запущены под UAC админом?
	} else {
		mb_IsUacAdmin = FALSE;
	}


    mh_WinHook = NULL;
	//mh_PopupHook = NULL;
    mp_TaskBar = NULL;

    mp_VActive = NULL; mp_VCon1 = NULL; mp_VCon2 = NULL;
    memset(mp_VCon, 0, sizeof(mp_VCon));
    
	UINT nAppMsg = WM_APP+10;
    mn_MsgPostCreate = ++nAppMsg;
    mn_MsgPostCopy = ++nAppMsg;
    mn_MsgMyDestroy = ++nAppMsg;
    mn_MsgUpdateSizes = ++nAppMsg;
    mn_MsgSetWindowMode = ++nAppMsg;
    mn_MsgUpdateTitle = ++nAppMsg;
    mn_MsgAttach = RegisterWindowMessage(CONEMUMSG_ATTACH);
	mn_MsgSrvStarted = RegisterWindowMessage(CONEMUMSG_SRVSTARTED);
    mn_MsgVConTerminated = ++nAppMsg;
    mn_MsgUpdateScrollInfo = ++nAppMsg;
    mn_MsgUpdateTabs = RegisterWindowMessage(CONEMUMSG_UPDATETABS);
    mn_MsgOldCmdVer = ++nAppMsg; mb_InShowOldCmdVersion = FALSE;
    mn_MsgTabCommand = ++nAppMsg;
    mn_MsgSheelHook = RegisterWindowMessage(L"SHELLHOOK");
	mn_ShellExecuteEx = ++nAppMsg;
	mn_PostConsoleResize = ++nAppMsg;
	mn_ConsoleLangChanged = ++nAppMsg;
	mn_MsgPostOnBufferHeight = ++nAppMsg;
	//mn_MsgSetForeground = RegisterWindowMessage(CONEMUMSG_SETFOREGROUND);
	mn_MsgFlashWindow = RegisterWindowMessage(CONEMUMSG_FLASHWINDOW);
}

BOOL CConEmuMain::Init()
{
	_ASSERTE(mp_TabBar == NULL);
	mp_TabBar = new TabBarClass();

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

	DWORD styleEx = 0;
	DWORD style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	if (ghWndApp)
		style |= WS_POPUPWINDOW | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
	else
		style |= WS_OVERLAPPEDWINDOW;
	if (gSet.nTransparent < 255)
		styleEx |= WS_EX_LAYERED;
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
	ghWnd = CreateWindowEx(styleEx, szClassNameParent, gSet.GetCmd(), style, 
		gSet.wndX, gSet.wndY, nWidth, nHeight, ghWndApp, NULL, (HINSTANCE)g_hInstance, NULL);
	if (!ghWnd) {
		if (!ghWndDC) MBoxA(_T("Can't create main window!"));
		return FALSE;
	}
	OnTransparent();
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

	CVirtualConsole::ClearPartBrushes();

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
    //if (ProgressBars) {
    //    delete ProgressBars;
    //    ProgressBars = NULL;
    //}
    
	if (mp_TabBar) {
		delete mp_TabBar;
		mp_TabBar = NULL;
	}

    CommonShutdown();
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

void CConEmuMain::AskChangeBufferHeight()
{
	CRealConsole *pRCon = ActiveCon()->RCon();
	if (!pRCon) return;
	BOOL lbBufferHeight = pRCon->isBufferHeight();
	BOOL b = gbDontEnable; gbDontEnable = TRUE;
	int nBtn = MessageBox(ghWnd, lbBufferHeight ? 
			L"Do You want to turn bufferheight OFF?" :
			L"Do You want to turn bufferheight ON?",
			L"ConEmu", MB_ICONQUESTION|MB_OKCANCEL);
	gbDontEnable = b;
	if (nBtn != IDOK) return;

#ifdef _DEBUG
	HANDLE hFarInExecuteEvent = NULL;
	if (!lbBufferHeight) {
		DWORD dwFarPID = pRCon->GetFarPID();
		if (dwFarPID) {
			// Это событие дергается в отладочной (мной поправленной) версии фара
			wchar_t szEvtName[64]; wsprintf(szEvtName, L"FARconEXEC:%08X", (DWORD)pRCon->ConWnd());
			hFarInExecuteEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, szEvtName);
			if (hFarInExecuteEvent) // Иначе ConEmuC вызовет _ASSERTE в отладочном режиме
				SetEvent(hFarInExecuteEvent);
		}
	}
#endif

	pRCon->ChangeBufferHeightMode(!lbBufferHeight);

#ifdef _DEBUG
	if (hFarInExecuteEvent)
		ResetEvent(hFarInExecuteEvent);
#endif

	OnBufferHeight();
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
            //if (!ghWnd || isIconic()) {
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
		case CEM_TABACTIVATE:
		case CEM_TABDEACTIVATE:
        {
            if (ghWnd) {
				bool lbTabActive = (mg == CEM_TAB) ? gConEmu.mp_TabBar->IsActive() : (mg == CEM_TABACTIVATE);
                // Главное окно уже создано, наличие таба определено
                if (lbTabActive) { //TODO: + IsAllowed()?
                    rc = gConEmu.mp_TabBar->GetMargins();
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
        //    if (gConEmu.mp_VActive && gConEmu.mp_VActive->RCon()->isBufferHeight()) { //TODO: а показывается ли сейчас прокрутка?
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
RECT CConEmuMain::CalcRect(enum ConEmuRect tWhat, RECT rFrom, enum ConEmuRect tFrom, RECT* prDC/*=NULL*/, enum ConEmuMargins tTabAction/*=CEM_TAB*/)
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
                        rcShift = CalcMargins(tTabAction);
                        AddMargins(rc, rcShift, TRUE/*abExpand*/);
                        rcShift = CalcMargins(CEM_FRAME);
                        AddMargins(rc, rcShift, TRUE/*abExpand*/);
                    } break;
                    case CER_MAINCLIENT:
                    {
                        //rcShift = CalcMargins(CEM_BACK);
                        //AddMargins(rc, rcShift, TRUE/*abExpand*/);
                        rcShift = CalcMargins(tTabAction);
                        AddMargins(rc, rcShift, TRUE/*abExpand*/);
                    } break;
                    case CER_TAB:
                    {
                        //rcShift = CalcMargins(CEM_BACK);
                        //AddMargins(rc, rcShift, TRUE/*abExpand*/);
                        rcShift = CalcMargins(tTabAction);
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
                    MBoxAssert(gConEmu.mp_VActive!=NULL);
                    gConEmu.mp_VActive->InitDC(false, true); // инициализировать ширину шрифта по умолчанию
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
            rcShift = CalcMargins(tTabAction);
            AddMargins(rc, rcShift);
        } break;
        case CER_DC:
        case CER_CONSOLE:
        {
			// Учесть высоту закладок (табов)
            rcShift = CalcMargins(tTabAction);
            AddMargins(rc, rcShift);

			// Для корректного деления на размер знакоместа...
            if (gSet.FontWidth()==0 || gSet.FontHeight()==0)
                gConEmu.mp_VActive->InitDC(false, true); // инициализировать ширину шрифта по умолчанию
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
                RECT rc1 = MakeRect(gConEmu.mp_VActive->TextWidth*gSet.FontWidth(), gConEmu.mp_VActive->TextHeight*gSet.FontHeight());
                	//gSet.ntvdmHeight/*mp_VActive->TextHeight*/*gSet.FontHeight());
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
                //COORD cr = gConEmu.mp_VActive->ClientToConsole( (rc.right - rc.left), (rc.bottom - rc.top) );
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
                    if (gSet.isHideCaption)
                    	rc.top -= GetSystemMetrics(SM_CYCAPTION);
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
                    if (gSet.isHideCaption)
                    	rc.top -= GetSystemMetrics(SM_CYCAPTION);
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
//  if (gConEmu.mp_TabBar->IsActive())
//      rect = gConEmu.mp_TabBar->GetMargins();
//
//  /*rect.top = gConEmu.mp_TabBar->IsActive()?gConEmu.mp_TabBar->Height():0;
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
//  if (gConEmu.mp_TabBar->IsActive()) {
//      RECT mr = gConEmu.mp_TabBar->GetMargins();
//      //rect.top += gConEmu.mp_TabBar->Height();
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
//      GetConsoleScreenBufferInfo(mp_VActive->hConOut(), &inf);
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
    /*if (updateInfo && !gSet.isFullScreen && !isZoomed() && !isIconic())
    {
        gSet.UpdateSize(size.X, size.Y);
    }*/

    RECT rcCon = MakeRect(size.X,size.Y);
    if (mp_VActive) {
        if (!mp_VActive->RCon()->SetConsoleSize(size.X,size.Y))
            rcCon = MakeRect(mp_VActive->TextWidth,mp_VActive->TextHeight);
    }
    RECT rcWnd = CalcRect(CER_MAIN, rcCon, CER_CONSOLE);

    RECT wndR; GetWindowRect(ghWnd, &wndR); // текущий XY

    MOVEWINDOW ( ghWnd, wndR.left, wndR.top, rcWnd.right, rcWnd.bottom, 1);
}

// Изменить размер консоли по размеру окна (главного)
void CConEmuMain::SyncConsoleToWindow()
{
    if (mb_SkipSyncSize || isNtvdm() || !mp_VActive)
        return;

	_ASSERTE(mn_InResize <= 1);

    #ifdef _DEBUG
    if (change2WindowMode!=(DWORD)-1) {
        _ASSERTE(change2WindowMode==(DWORD)-1);
    }
    #endif

    mp_VActive->RCon()->SyncConsole2Window();

    /*
    DEBUGLOGFILE("SyncConsoleToWindow\n");

    RECT rcClient; GetClientRect(ghWnd, &rcClient);

    // Посчитать нужный размер консоли
    RECT newCon = CalcRect(CER_CONSOLE, rcClient, CER_MAINCLIENT);

    mp_VActive->SetConsoleSize(MakeCoord(newCon.right, newCon.bottom));
    */

    /*if (!isZoomed() && !isIconic() && !gSet.isFullScreen)
        gSet.UpdateSize(mp_VActive->TextWidth, mp_VActive->TextHeight);*/

    /*
    // Посчитать нужный размер консоли
    //COORD newConSize = ConsoleSizeFromWindow();
    // Получить текущий размер консольного окна
    CONSOLE_SCREEN_BUFFER_INFO inf; memset(&inf, 0, sizeof(inf));
    //GetConsoleScreenBufferInfo(mp_VActive->hConOut(), &inf);

    // Если нужно менять - ...
    if (newCon.right != (inf.srWindow.Right-inf.srWindow.Left+1) ||
        newCon.bottom != (inf.srWindow.Bottom-inf.srWindow.Top+1))
    {
        SetConsoleWindowSize(MakeCoord(newCon.right,newCon.bottom), true);
        if (mp_VActive)
            mp_VActive->InitDC(false);
    }
    */
}

void CConEmuMain::SyncNtvdm()
{
    //COORD sz = {mp_VActive->TextWidth, mp_VActive->TextHeight};
    //SetConsoleWindowSize(sz, false);

    OnSize();
}

// Установить размер основного окна по текущему размеру mp_VActive
void CConEmuMain::SyncWindowToConsole()
{
    DEBUGLOGFILE("SyncWindowToConsole\n");

    if (mb_SkipSyncSize || !mp_VActive)
        return;

    #ifdef _DEBUG
    _ASSERT(GetCurrentThreadId() == mn_MainThreadId);
    #endif

    RECT rcDC = mp_VActive->GetRect();
        /*MakeRect(mp_VActive->Width, mp_VActive->Height);
    if (mp_VActive->Width == 0 || mp_VActive->Height == 0) {
        rcDC = MakeRect(mp_VActive->winSize.X, mp_VActive->winSize.Y);
    }*/

    //_ASSERT(rcDC.right>250 && rcDC.bottom>200);

    RECT rcWnd = CalcRect(CER_MAIN, rcDC, CER_DC); // размеры окна
    
    //GetCWShift(ghWnd, &cwShift);
    
    RECT wndR; GetWindowRect(ghWnd, &wndR); // текущий XY

    gSet.UpdateSize(mp_VActive->TextWidth, mp_VActive->TextHeight);

    MOVEWINDOW ( ghWnd, wndR.left, wndR.top, rcWnd.right, rcWnd.bottom, 1);
    
    //RECT consoleRect = ConsoleOffsetRect();

    //#ifdef MSGLOGGER
    //    char szDbg[100]; wsprintfA(szDbg, "   mp_VActive:Size={%i,%i}\n", mp_VActive->Width,mp_VActive->Height);
    //    DEBUGLOGFILE(szDbg);
    //#endif
    //
    //MOVEWINDOW ( ghWnd, wndR.left, wndR.top, mp_VActive->Width + cwShift.x + consoleRect.left + consoleRect.right, mp_VActive->Height + cwShift.y + consoleRect.top + consoleRect.bottom, 1);
}

bool CConEmuMain::SetWindowMode(uint inMode)
{
    if (!isMainThread()) {
        PostMessage(ghWnd, mn_MsgSetWindowMode, inMode, 0);
        return false;
    }

    #ifndef _DEBUG
    //2009-04-22 Если открыт PictureView - лучше не дергаться...
    if (isPictureView())
        return false;
    #endif

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
            if (mp_VActive && !mp_VActive->RCon()->SetConsoleSize(rcCon.right, rcCon.bottom)) {
                mb_PassSysCommand = false;
                goto wrap;
            }

            if (isIconic() || isZoomed()) {
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

            // Параметры именно такие, результат - просто подгонка rcNew под рабочую область текущего монитора
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
            // Если это во время загрузки - то до первого ShowWindow - isIconic возвращает FALSE
            if (isIconic() || isZoomed())
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
            if (!gSet.isFullScreen && !isZoomed() && !isIconic())
            {
                gSet.UpdatePos(rcWnd.left, rcWnd.top);
                if (mp_VActive)
                    gSet.UpdateSize(mp_VActive->TextWidth, mp_VActive->TextHeight);
            }

            RECT rcMax = CalcRect(CER_MAXIMIZED, MakeRect(0,0), CER_MAXIMIZED);
            // Скорректируем размер окна до видимого на мониторе (рамка при максимизации уезжает за пределы экрана)
            rcMax.left -= GetSystemMetrics(SM_CXSIZEFRAME);
            rcMax.right += GetSystemMetrics(SM_CXSIZEFRAME);
            rcMax.top -= GetSystemMetrics(SM_CYSIZEFRAME);
            rcMax.bottom += GetSystemMetrics(SM_CYSIZEFRAME);
            /*if (gSet.isHideCaption)
            	rcMax.top -= GetSystemMetrics(SM_CYCAPTION);*/
            RECT rcCon = CalcRect(CER_CONSOLE, rcMax, CER_MAIN);
            if (mp_VActive && !mp_VActive->RCon()->SetConsoleSize(rcCon.right,rcCon.bottom)) {
                mb_PassSysCommand = false;
                goto wrap;
            }

            if (!isZoomed()) {
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
        if (!gSet.isFullScreen || (isZoomed() || isIconic()))
        {
            // Обновить коордианты в gSet, если требуется
            if (!gSet.isFullScreen && !isZoomed() && !isIconic())
            {
                gSet.UpdatePos(rcWnd.left, rcWnd.top);
                if (mp_VActive)
                    gSet.UpdateSize(mp_VActive->TextWidth, mp_VActive->TextHeight);
            }

            RECT rcMax = CalcRect(CER_FULLSCREEN, MakeRect(0,0), CER_FULLSCREEN);
            RECT rcCon = CalcRect(CER_CONSOLE, rcMax, CER_MAINCLIENT);
            if (mp_VActive && !mp_VActive->RCon()->SetConsoleSize(rcCon.right,rcCon.bottom)) {
                mb_PassSysCommand = false;
                goto wrap;
            }

            gSet.isFullScreen = true;
            isWndNotFSMaximized = isZoomed();
            
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

            if (isIconic() || isZoomed()) {
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
	SetCursor(LoadCursor(NULL,IDC_ARROW));
    change2WindowMode = -1;
    return lbRc;
}

void CConEmuMain::ForceShowTabs(BOOL abShow)
{
    //if (!mp_VActive)
    //  return;

    //2009-05-20 Раз это Force - значит на возможность получить табы из фара забиваем! Для консоли показывается "Console"
    BOOL lbTabsAllowed = abShow /*&& gConEmu.mp_TabBar->IsAllowed()*/;

    if (abShow && !gConEmu.mp_TabBar->IsShown() && gSet.isTabs && lbTabsAllowed)
    {
        gConEmu.mp_TabBar->Activate();
        //ConEmuTab tab; memset(&tab, 0, sizeof(tab));
        //tab.Pos=0;
        //tab.Current=1;
        //tab.Type = 1;
        //gConEmu.mp_TabBar->Update(&tab, 1);
        //mp_VActive->RCon()->SetTabs(&tab, 1);
        gConEmu.mp_TabBar->Update();
        //gbPostUpdateWindowSize = true; // 2009-07-04 Resize выполняет сам TabBar
    } else if (!abShow) {
        gConEmu.mp_TabBar->Deactivate();
        //gbPostUpdateWindowSize = true; // 2009-07-04 Resize выполняет сам TabBar
    }

    // При отключенных табах нужно показать "[n/n] " а при выключенных - спрятать
    UpdateTitle(NULL); // сам перечитает

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

bool CConEmuMain::isIconic()
{
	bool bIconic = ::IsIconic(ghWnd);
	return bIconic;
}

bool CConEmuMain::isZoomed()
{
	bool bZoomed = ::IsZoomed(ghWnd);
	return bZoomed;
}

void CConEmuMain::ReSize(BOOL abCorrect2Ideal /*= FALSE*/)
{
    if (isIconic())
        return;

	RECT client; GetClientRect(ghWnd, &client);

	if (abCorrect2Ideal) {
		

		if (!isZoomed() && !gSet.isFullScreen) {
			// Выполняем всегда, даже если размер уже соответсвует...
			RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);
			RECT rcConsole = CalcRect(CER_CONSOLE, mrc_Ideal, CER_MAIN);
			RECT rcCompWnd = CalcRect(CER_MAIN, rcConsole, CER_CONSOLE);
			// При показе/скрытии табов высота консоли может "прыгать"
			// Ее нужно скорректировать. Поскольку идет реальный ресайз
			// главного окна - OnSize вызовается автоматически
			_ASSERTE(isMainThread());

			m_Child.SetRedraw(FALSE);
			mp_VActive->RCon()->SetConsoleSize(rcConsole.right, rcConsole.bottom, 0, CECMD_SETSIZESYNC);
			m_Child.SetRedraw(TRUE);
			m_Child.Redraw();

			//#ifdef _DEBUG
			//DebugStep(L"...Sleeping");
			//Sleep(300);
			//DebugStep(NULL);
			//#endif

			MoveWindow(ghWnd, rcWnd.left, rcWnd.top, 
				(rcCompWnd.right - rcCompWnd.left), (rcCompWnd.bottom - rcCompWnd.top), 1);
		} else {
			RECT rcConsole = CalcRect(CER_CONSOLE, client, CER_MAINCLIENT);

			m_Child.SetRedraw(FALSE);
			mp_VActive->RCon()->SetConsoleSize(rcConsole.right, rcConsole.bottom, 0, CECMD_SETSIZESYNC);
			m_Child.SetRedraw(TRUE);
			m_Child.Redraw();
		}
	}

    OnSize(isZoomed() ? SIZE_MAXIMIZED : SIZE_RESTORED,
        client.right, client.bottom);

	if (abCorrect2Ideal) {
		
	}
}

void CConEmuMain::OnConsoleResize(BOOL abPosted/*=FALSE*/)
{
	// Выполняться должно в нити окна, иначе можем повиснуть
	static bool lbPosted = false;
	if (!abPosted) {
		if (!lbPosted) {
			lbPosted = true; // чтобы post не накапливались
			PostMessage(ghWnd, mn_PostConsoleResize, 0,0);
		}
		return;
	}
	lbPosted = false;
	
	if (isIconic())
		return; // если минимизировано - ничего не делать

    // Было ли реальное изменение размеров?
    BOOL lbSizingToDo  = (mouse.state & MOUSE_SIZING_TODO) == MOUSE_SIZING_TODO;

    if (isSizing() && !isPressed(VK_LBUTTON)) {
        // Сборс всех флагов ресайза мышкой
        mouse.state &= ~(MOUSE_SIZING_BEGIN|MOUSE_SIZING_TODO);
    }

    //COORD c = ConsoleSizeFromWindow();
    RECT client; GetClientRect(ghWnd, &client);
    // Проверим, вдруг не отработал isIconic
    if (client.bottom > 10 )
    {
        RECT c = CalcRect(CER_CONSOLE, client, CER_MAINCLIENT);
        // чтобы не насиловать консоль лишний раз - реальное измененение ее размеров только
        // при отпускании мышкой рамки окна
        BOOL lbSizeChanged = FALSE;
        if (mp_VActive) {
            lbSizeChanged = (c.right != (int)mp_VActive->RCon()->TextWidth() 
                    || c.bottom != (int)mp_VActive->RCon()->TextHeight());
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
                if (!gSet.isFullScreen && !isZoomed() && !lbSizingToDo)
                    SyncWindowToConsole();
                else
                    SyncConsoleToWindow();
                OnSize(0, client.right, client.bottom);
            }
            //_ASSERTE(mp_VActive!=NULL);
            if (mp_VActive) {
                m_LastConSize = MakeCoord(mp_VActive->TextWidth,mp_VActive->TextHeight);
            }
			// Запомнить "идеальный" размер окна, выбранный пользователем
			if (lbSizingToDo && !gSet.isFullScreen && !isZoomed() && !isIconic()) {
				GetWindowRect(ghWnd, &mrc_Ideal);
			}
        }
        else if (mp_VActive 
            && (m_LastConSize.X != (int)mp_VActive->TextWidth 
                || m_LastConSize.Y != (int)mp_VActive->TextHeight))
        {
            // По идее, сюда мы попадаем только для 16-бит приложений
            if (isNtvdm())
                SyncNtvdm();
            m_LastConSize = MakeCoord(mp_VActive->TextWidth,mp_VActive->TextHeight);
        }
    }
}

LRESULT CConEmuMain::OnSize(WPARAM wParam, WORD newClientWidth, WORD newClientHeight)
{
    LRESULT result = 0;
    
    if (wParam == SIZE_MINIMIZED || isIconic()) {
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

	
	//if (mb_InResize) {
	//	_ASSERTE(!mb_InResize);
	//	PostMessage(ghWnd, WM_SIZE, wParam, MAKELONG(newClientWidth,newClientHeight));
	//	return 0;
	//}

	mn_InResize++;

    if (newClientWidth==(WORD)-1 || newClientHeight==(WORD)-1) {
        RECT rcClient; GetClientRect(ghWnd, &rcClient);
        newClientWidth = rcClient.right;
        newClientHeight = rcClient.bottom;
    }

	// Запомнить "идеальный" размер окна, выбранный пользователем
	if (isSizing() && !gSet.isFullScreen && !isZoomed() && !isIconic()) {
		GetWindowRect(ghWnd, &mrc_Ideal);
	}

    if (gConEmu.mp_TabBar->IsActive())
        gConEmu.mp_TabBar->UpdateWidth();

    // Background - должен занять все клиентское место под тулбаром
    // Там же ресайзится ScrollBar
    m_Back.Resize();
    
    #ifdef _DEBUG
    BOOL lbIsPicView = 
    #endif
    isPictureView();

    if (wParam != (DWORD)-1 && change2WindowMode == (DWORD)-1 && mn_InResize <= 1) {
        SyncConsoleToWindow();
    }
    
    RECT mainClient = MakeRect(newClientWidth,newClientHeight);

	RECT dcSize = CalcRect(CER_DC, mainClient, CER_MAINCLIENT);

	RECT client = CalcRect(CER_DC, mainClient, CER_MAINCLIENT, &dcSize);

    RECT rcNewCon; memset(&rcNewCon,0,sizeof(rcNewCon));
    if (mp_VActive && mp_VActive->Width && mp_VActive->Height) {
        if ((gSet.isTryToCenter && (isZoomed() || gSet.isFullScreen))
			|| isNtvdm())
        {
            rcNewCon.left = (client.right+client.left-(int)mp_VActive->Width)/2;
            rcNewCon.top = (client.bottom+client.top-(int)mp_VActive->Height)/2;
        }

        if (rcNewCon.left<client.left) rcNewCon.left=client.left;
        if (rcNewCon.top<client.top) rcNewCon.top=client.top;

        rcNewCon.right = rcNewCon.left + mp_VActive->Width;
        rcNewCon.bottom = rcNewCon.top + mp_VActive->Height;
        
        if (rcNewCon.right>client.right) rcNewCon.right=client.right;
        if (rcNewCon.bottom>client.bottom) rcNewCon.bottom=client.bottom;
        
    } else {
        rcNewCon = client;
    }
    // Двигаем/ресайзим окошко DC
    MoveWindow(ghWndDC, rcNewCon.left, rcNewCon.top, rcNewCon.right - rcNewCon.left, rcNewCon.bottom - rcNewCon.top, 1);
	m_Child.Invalidate();

	if (mn_InResize>0)
		mn_InResize--;

    return result;
}

LRESULT CConEmuMain::OnSizing(WPARAM wParam, LPARAM lParam)
{
    LRESULT result = true;
    
    #if defined(EXT_GNUC_LOG)
    char szDbg[255];
    wsprintfA(szDbg, "CConEmuMain::OnSizing(wParam=%i, L.Lo=%i, L.Hi=%i)\n",
        wParam, LOWORD(lParam), HIWORD(lParam));
    if (gSet.isAdvLogging>1) 
    	mp_VActive->RCon()->LogString(szDbg);
    #endif


    #ifndef _DEBUG
    if (isPictureView()) {
        RECT *pRect = (RECT*)lParam; // с рамкой
        *pRect = mrc_WndPosOnPicView;
        //pRect->right = pRect->left + (mrc_WndPosOnPicView.right-mrc_WndPosOnPicView.left);
        //pRect->bottom = pRect->top + (mrc_WndPosOnPicView.bottom-mrc_WndPosOnPicView.top);
    } else
    #endif
    if (mb_IgnoreSizeChange) {
        // на время обработки WM_SYSCOMMAND
    } else
    if (isNtvdm()) {
        // не менять для 16бит приложений
    } else
    if (!gSet.isFullScreen && !isZoomed()) {
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
    for (int i=0; mp_VActive && i<MAX_CONSOLE_COUNT; i++) {
        if (mp_VCon[i] == apVCon) {
            ConActivate(i);
            lbRc = (mp_VActive == apVCon);
            break;
        }
    }
    return lbRc;
}

CVirtualConsole* CConEmuMain::ActiveCon()
{
    return mp_VActive;
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
    for (int i=0; mp_VActive && i<MAX_CONSOLE_COUNT; i++) {
        if (mp_VCon[i] == mp_VActive) {
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

//void CConEmuMain::CheckGuiBarsCreated()
//{
//    if (gSet.isGUIpb && !ProgressBars) {
//    	ProgressBars = new CProgressBars(ghWnd, g_hInstance);
//    }
//}

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

    //if (mp_VActive) {
    //    mn_ActiveStatus |= mp_VActive->RCon()->GetProgramStatus();
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
//        gConEmu.mp_TabBar->Reset();
//        if (mn_TopProcessID) {
//            // Дернуть событие для этого процесса фара - он перешлет текущие табы
//            gConEmu.mp_TabBar->Retrieve();
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
//        gConEmu.mp_TabBar->OnConman ( m_ActiveConmanIDX, isConmanAlternative() );
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
//    gConEmu.mp_TabBar->Refresh(mn_ActiveStatus & CES_FARACTIVE);
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
//    //  gConEmu.mp_TabBar->Refresh(mb_FarActive);
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
        if (pCon == mp_VActive)
            return true; // уже
        bool lbSizeOK = true;
        int nOldConNum = ActiveConNum();

        // Спрятать PictureView, или еще чего...
        if (mp_VActive) mp_VActive->RCon()->OnDeactivate(nCon);
        
        // ПЕРЕД переключением на новую консоль - обновить ее размеры
        if (mp_VActive) {
            int nOldConWidth = mp_VActive->RCon()->TextWidth();
            int nOldConHeight = mp_VActive->RCon()->TextHeight();
            int nNewConWidth = pCon->RCon()->TextWidth();
            int nNewConHeight = pCon->RCon()->TextHeight();
            if (nOldConWidth != nNewConWidth || nOldConHeight != nNewConHeight) {
                lbSizeOK = pCon->RCon()->SetConsoleSize(nOldConWidth,nOldConHeight);
            }
        }

        mp_VActive = pCon;

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
                if (mp_VActive) mp_VActive->RCon()->OnDeactivate(i);
                mp_VCon[i] = pCon;
                mp_VActive = pCon;
                pCon->RCon()->OnActivate(i, ActiveConNum());
                
                //mn_ActiveCon = i;

                //Update(true);
            }
            break;
        }
    }
    return pCon;
}

void CConEmuMain::UpdateFarSettings()
{
	for (int i = 0; i<MAX_CONSOLE_COUNT; i++) {
		if (mp_VCon[i] == NULL) continue;
		CRealConsole* pRCon = mp_VCon[i]->RCon();
		if (pRCon)
			pRCon->UpdateFarSettings();
		//DWORD dwFarPID = pRCon->GetFarPID();
		//if (!dwFarPID) continue;
		//pRCon->EnableComSpec(dwFarPID, gSet.AutoBufferHeight);
	}
}

void CConEmuMain::DebugStep(LPCTSTR asMsg)
{
    if ((gSet.isDebugSteps && ghWnd) || !asMsg)
        SetWindowText(ghWnd, asMsg ? asMsg : Title);
}

DWORD_PTR CConEmuMain::GetActiveKeyboardLayout()
{
    DWORD_PTR dwActive = (DWORD_PTR)GetKeyboardLayout(mn_MainThreadId);
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
		/*if (IsDebuggerPresent()) { -- не требуется. был баг с памятью
			BOOL b = gbDontEnable; gbDontEnable = TRUE;
			int nBtn = MessageBox(ghWnd, L"Debugger active!\nShellExecuteEx(runas) my fails, when VC IDE\ncatches Microsoft C++ exceptions.\nContinue?", L"ConEmu", MB_ICONASTERISK|MB_YESNO|MB_DEFBUTTON2);
			gbDontEnable = b;
			if (nBtn != IDYES)
				return (FALSE);
		}*/
		lRc = ::ShellExecuteEx(lpShellExecute);
		
		if (abAllowAsync && lRc == 0) {
			mp_VActive->RCon()->CloseConsole();
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
        DebugStep(_T("Macro: Waiting for result (10 sec)"));
        pipe.Execute(CMD_POSTMACRO, asMacro, (wcslen(asMacro)+1)*2);
        DebugStep(NULL);
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
    if (!mp_VActive) {
    	//MBox(L"CtrlWinAltSpace: mp_VActive==NULL");
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
    mp_VActive->RCon()->ShowConsole(-1); // Toggle visibility
}

void CConEmuMain::Recreate(BOOL abRecreate, BOOL abConfirm)
{
    FLASHWINFO fl = {sizeof(FLASHWINFO)}; fl.dwFlags = FLASHW_STOP; fl.hwnd = ghWnd;
    FlashWindowEx(&fl); // При многократных созданиях мигать начинает...

	RConStartArgs args;
	args.bRecreate = abRecreate;

	if (!abConfirm && isPressed(VK_SHIFT))
		abConfirm = TRUE;
    
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
			args.bRunAsAdministrator = mp_VActive->RCon()->isAdministrator();

            BOOL b = gbDontEnable;
            gbDontEnable = TRUE;
            int nRc = DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_RESTART), ghWnd, RecreateDlgProc, (LPARAM)&args);
            gbDontEnable = b;
            if (nRc == IDC_TERMINATE) {
                mp_VActive->RCon()->CloseConsole();
                return;
            }
            if (nRc != IDC_START)
                return;
			m_Child.Redraw();
            // Собственно, Recreate
            mp_VActive->RCon()->RecreateProcess(&args);
        }
    }
    
    SafeFree(args.pszSpecialCmd);
}

BOOL CConEmuMain::RunSingleInstance()
{
	BOOL lbAccepted = FALSE;
	LPCWSTR lpszCmd = gSet.GetCmd();
	if (lpszCmd && *lpszCmd) {
		HWND ConEmuHwnd = FindWindowExW(NULL, NULL, VirtualConsoleClassMain, NULL);
		if (ConEmuHwnd) {
			CESERVER_REQ *pIn = NULL, *pOut = NULL;
			int nCmdLen = lstrlenW(lpszCmd);
			int nSize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_NEWCMD);
			if (nCmdLen >= MAX_PATH) {
				nSize += (nCmdLen - MAX_PATH + 2) * 2;
			}
			pIn = (CESERVER_REQ*)calloc(nSize,1);
			if (pIn) {
				ExecutePrepareCmd(pIn, CECMD_NEWCMD, nSize);
				lstrcpyW(pIn->NewCmd.szCommand, lpszCmd);

				DWORD dwPID = 0;
				if (GetWindowThreadProcessId(ConEmuHwnd, &dwPID))
					AllowSetForegroundWindow(dwPID);

				pOut = ExecuteGuiCmd(ConEmuHwnd, pIn, NULL);
				if (pOut && pOut->Data[0])
					lbAccepted = TRUE;
			}
			if (pIn) {free(pIn); pIn = NULL;}
			if (pOut) ExecuteFreeResult(pOut);
		}
	}
	return lbAccepted;
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
			pszSystem = gSet.HistoryGet();
			if (pszSystem) {
				while (*pszSystem) {
					int nId = SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_FINDSTRINGEXACT, -1, (LPARAM)pszSystem);
					if (nId < 0) SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_INSERTSTRING, -1, (LPARAM)pszSystem);
					pszSystem += lstrlen(pszSystem)+1;
				}
			}
            SetDlgItemText(hDlg, IDC_RESTART_CMD, pszCmd);
            
            SetDlgItemText(hDlg, IDC_STARTUP_DIR, gConEmu.ActiveCon()->RCon()->GetDir());
            //EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_DIR), FALSE);
            //#ifndef _DEBUG
            //EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_DIR), FALSE);
            //#endif
            

			RConStartArgs* pArgs = (RConStartArgs*)lParam;
			_ASSERTE(pArgs);
			SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)lParam);

	
			if (gConEmu.m_osv.dwMajorVersion < 6) {
				// В XP и ниже это просто RunAs - с возможностью ввода имени пользователя и пароля
				SetDlgItemTextA(hDlg, cbRunAs, "&Run as..."); //GCC hack. иначе не собирается
				// И уменьшить длину
                RECT rcBox; GetWindowRect(GetDlgItem(hDlg, cbRunAs), &rcBox);
                SetWindowPos(GetDlgItem(hDlg, cbRunAs), NULL, 0, 0, (rcBox.right-rcBox.left)/2, rcBox.bottom-rcBox.top,
                	SWP_NOMOVE|SWP_NOZORDER);
			}
			if (gConEmu.mb_IsUacAdmin || (pArgs && pArgs->bRunAsAdministrator)) {
				CheckDlgButton(hDlg, cbRunAs, BST_CHECKED);
				if (gConEmu.mb_IsUacAdmin) { // Только в Vista+ если GUI уже запущен под админом
					EnableWindow(GetDlgItem(hDlg, cbRunAs), FALSE);
				} else {
					RecreateDlgProc(hDlg, WM_COMMAND, cbRunAs, 0);
				}
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
                
                // Выровнять флажок по кнопке
				RECT rcBtn; GetWindowRect(GetDlgItem(hDlg, IDC_START), &rcBtn);
                RECT rcBox; GetWindowRect(GetDlgItem(hDlg, cbRunAs), &rcBox);
                pt.x = pt.x - (rcBox.right - rcBox.left) - 5;
                //MapWindowPoints(NULL, hDlg, (LPPOINT)&rcBox, 1);
                pt.y += ((rcBtn.bottom-rcBtn.top) - (rcBox.bottom-rcBox.top))/2;
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
                	bi.lpfn = BrowseCallbackProc;
                	bi.lParam = (LPARAM)szFolder;
                	
                	LPITEMIDLIST pRc = SHBrowseForFolder(&bi);
                	if (pRc) {
                		if (SHGetPathFromIDList(pRc, szFolder)) {
                			SetDlgItemText(hDlg, IDC_STARTUP_DIR, szFolder);
						}

                		CoTaskMemFree(pRc);
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
					// Command
                    HWND hEdit = GetDlgItem(hDlg, IDC_RESTART_CMD);
                    int nLen = GetWindowTextLength(hEdit);
                    if (nLen > 0) {
						_ASSERTE(pArgs->pszSpecialCmd==NULL);
                        pArgs->pszSpecialCmd = (wchar_t*)calloc(nLen+1,2);
						if (pArgs->pszSpecialCmd) {
                            GetWindowText(hEdit, pArgs->pszSpecialCmd, nLen+1);
							gSet.HistoryAdd(pArgs->pszSpecialCmd);
						}
                    }
					// StartupDir
					hEdit = GetDlgItem(hDlg, IDC_STARTUP_DIR);
					nLen = GetWindowTextLength(hEdit);
                    if (nLen > 0) {
						_ASSERTE(pArgs->pszStartupDir==NULL);
                        pArgs->pszStartupDir = (wchar_t*)calloc(nLen+1,2);
                        if (pArgs->pszStartupDir)
                            GetWindowText(hEdit, pArgs->pszStartupDir, nLen+1);
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

void CConEmuMain::ShowOldCmdVersion(DWORD nCmd, DWORD nVersion, int bFromServer)
{
    if (!isMainThread()) {
        if (mb_InShowOldCmdVersion)
            return; // уже послано
        mb_InShowOldCmdVersion = TRUE;
        PostMessage(ghWnd, mn_MsgOldCmdVer, (nCmd & 0xFFFF) | (((DWORD)(unsigned short)bFromServer) << 16), nVersion);
        return;
    }

	static bool lbErrorShowed = false;
	if (lbErrorShowed) return;
	lbErrorShowed = true;

    wchar_t szMsg[255];
    wsprintf(szMsg, L"ConEmu received old version packet!\nCommandID: %i, Version: %i, ReqVersion: %i\nPlease check %s",
    	nCmd, nVersion, CESERVER_REQ_VER,
    	(bFromServer==1) ? L"ConEmuC.exe" : (bFromServer==0) ? L"ConEmu.dll" : L"ConEmuC.exe and ConEmu.dll");
    MBox(szMsg);

    mb_InShowOldCmdVersion = FALSE; // теперь можно показать еще одно...
}

void CConEmuMain::ShowSysmenu(HWND Wnd, HWND Owner, int x, int y)
{
    bool iconic = isIconic();
    bool zoomed = isZoomed();
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

void CConEmuMain::StartDebugLogConsole()
{
	if (IsDebuggerPresent())
		return; // УЖЕ!

	// Create process, with flag /Attach GetCurrentProcessId()
	// Sleep for sometimes, try InitHWND(hConWnd); several times
	WCHAR  szExe[0x200] = {0};
	BOOL lbRc = FALSE;
	
	//DWORD nLen = 0;
	PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
	STARTUPINFO si; memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	DWORD dwSelfPID = GetCurrentProcessId();
	
	// "/ATTACH" - низя, а то заблокируемся при попытке подключения к "отлаживаемому" GUI
	wsprintf(szExe, L"\"%s\" /DEBUGPID=%i /BW=80 /BH=25 /BZ=1000", 
		ms_ConEmuCExe, dwSelfPID);
	
	if (!CreateProcess(NULL, szExe, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS|CREATE_NEW_CONSOLE, NULL,
			NULL, &si, &pi))
	{
		// Хорошо бы ошибку показать?
		DWORD dwErr = GetLastError();
		wchar_t szErr[128]; wsprintf(szErr, L"Can't create debugger console! ErrCode=0x%08X", dwErr);
		MBoxA(szErr);
	} else {
		gbDebugLogStarted = TRUE;
		lbRc = TRUE;
	}
}

void CConEmuMain::UpdateProcessDisplay(BOOL abForce)
{
    if (!ghOpWnd)
        return;

    wchar_t szNo[32];
    DWORD nProgramStatus = mp_VActive->RCon()->GetProgramStatus();
    DWORD nFarStatus = mp_VActive->RCon()->GetFarStatus();
    CheckDlgButton(gSet.hInfo, cbsTelnetActive, (nProgramStatus&CES_TELNETACTIVE) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(gSet.hInfo, cbsNtvdmActive, (nProgramStatus&CES_NTVDM) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(gSet.hInfo, cbsFarActive, (nProgramStatus&CES_FARACTIVE) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(gSet.hInfo, cbsFilePanel, (nFarStatus&CES_FILEPANEL) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(gSet.hInfo, cbsEditor, (nFarStatus&CES_EDITOR) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(gSet.hInfo, cbsViewer, (nFarStatus&CES_VIEWER) ? BST_CHECKED : BST_UNCHECKED);
    SetDlgItemText(gSet.hInfo, tsTopPID, _itow(mp_VActive->RCon()->GetFarPID(), szNo, 10));
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
            if (mp_VCon[j] == mp_VActive)
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

    if (mp_VActive)
        mp_VActive->UpdateInfo();
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
    //if (!asNewTitle)
    //    return;

    if (GetCurrentThreadId() != mn_MainThreadId) {
        /*if (TitleCmp != asNewTitle) -- можем наколоться на многопоточности. Лучше получим повторно
            wcscpy(TitleCmp, asNewTitle);*/
        PostMessage(ghWnd, mn_MsgUpdateTitle, 0, 0);
        return;
    }

    if (!asNewTitle)
    	if ((asNewTitle = mp_VActive->RCon()->GetTitle()) == NULL)
    		return;
    
    wcscpy(Title, asNewTitle);

    // Там же обновится L"[%i/%i] " если несколько консолей а табы отключены
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

    Icon.UpdateTitle();

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
    if ((nProgress = mp_VActive->RCon()->GetProgress(&bWasError)) >= 0) {
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

    if (gSet.isMulti && !gConEmu.mp_TabBar->IsShown()) {
        int nCur = 1, nCount = 0;
        for (int n=0; n<MAX_CONSOLE_COUNT; n++) {
            if (mp_VCon[n]) {
                nCount ++;
                if (mp_VActive == mp_VCon[n])
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
        wsprintfW(szDbg, L"EVENT_CONSOLE_END_APPLICATION(HWND=0x%08X, PID=%i%s)\n", hwnd, idObject, 
			(idChild == CONSOLE_APPLICATION_16BIT) ? L" 16bit" : L"");
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

void CConEmuMain::Invalidate(CVirtualConsole* apVCon)
{
	if (!this || !apVCon) return;
	TODO("После добавления второго viewport'а потребуется доработка");
	m_Child.Invalidate();
}

void CConEmuMain::InvalidateAll()
{
    InvalidateRect(ghWnd, NULL, TRUE);
    m_Child.Invalidate();
    m_Back.Invalidate();
    gConEmu.mp_TabBar->Invalidate();
}

LRESULT CConEmuMain::OnPaint(WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    PAINTSTRUCT ps;
    #ifdef _DEBUG
    HDC hDc = 
    #endif
    BeginPaint(ghWnd, &ps);
	//RECT rcClient; GetClientRect(ghWnd, &rcClient);
	//RECT rcTabMargins = CalcMargins(CEM_TAB);
	//AddMargins(rcClient, rcTabMargins, FALSE);

	//HDC hdc = GetDC(ghWnd);

    PaintGaps(ps.hdc);

	PaintCon(ps.hdc);
	//PaintCon(hdc);

    EndPaint(ghWnd, &ps);
    //result = DefWindowProc(ghWnd, WM_PAINT, wParam, lParam);

	//ReleaseDC(ghWnd, hdc);

	//ValidateRect(ghWnd, &rcClient);

    return result;
}

void CConEmuMain::PaintGaps(HDC hDC)
{
	if (hDC==NULL)
		hDC = GetDC(ghWnd); // Главное окно!

	int
	#ifdef _DEBUG
		mn_ColorIdx = 1;
	#else
		mn_ColorIdx = 0;
	#endif
	HBRUSH hBrush = CreateSolidBrush(gSet.Colors[mn_ColorIdx]);

	RECT rcClient; GetClientRect(ghWnd, &rcClient); // Клиентская часть главного окна
	RECT rcMargins = CalcMargins(CEM_TAB);
	AddMargins(rcClient, rcMargins, FALSE);

	RECT offsetRect; GetClientRect(ghWndDC, &offsetRect);
	//WINDOWPLACEMENT wpl; memset(&wpl, 0, sizeof(wpl)); wpl.length = sizeof(wpl);
	//GetWindowPlacement(ghWndDC, &wpl); // Положение окна, в котором идет отрисовка

	////RECT offsetRect = ConsoleOffsetRect(); // смещение с учетом табов
	//RECT dcRect; GetClientRect(ghWndDC, &dcRect);
	//RECT offsetRect = dcRect;
	MapWindowPoints(ghWndDC, ghWnd, (LPPOINT)&offsetRect, 2);


	// paint gaps between console and window client area with first color

	RECT rect;

	//TODO:!!!
	// top
	rect = rcClient;
	rect.bottom = offsetRect.top;
	if (!IsRectEmpty(&rect))
		FillRect(hDC, &rect, hBrush);
#ifdef _DEBUG
	GdiFlush();
#endif

	// right
	rect.left = offsetRect.right;
	rect.bottom = rcClient.bottom;
	if (!IsRectEmpty(&rect))
		FillRect(hDC, &rect, hBrush);
#ifdef _DEBUG
	GdiFlush();
#endif

	// left
	rect.left = 0;
	rect.right = offsetRect.left;
	rect.bottom = rcClient.bottom;
	if (!IsRectEmpty(&rect))
		FillRect(hDC, &rect, hBrush);
#ifdef _DEBUG
	GdiFlush();
#endif

	// bottom
	rect.left = 0;
	rect.right = rcClient.right;
	rect.top = offsetRect.bottom;
	rect.bottom = rcClient.bottom;
	if (!IsRectEmpty(&rect))
		FillRect(hDC, &rect, hBrush);
#ifdef _DEBUG
	GdiFlush();
#endif

	DeleteObject(hBrush);
}

void CConEmuMain::PaintCon(HDC hPaintDC)
{
    //if (ProgressBars)
    //    ProgressBars->OnTimer();

	RECT rcClient = {0};
	if (ghWndDC) {
		GetClientRect(ghWndDC, &rcClient);
		MapWindowPoints(ghWndDC, ghWnd, (LPPOINT)&rcClient, 2);
	}

	// если mp_VActive==NULL - будет просто выполнена заливка фоном.
    mp_VActive->Paint(hPaintDC, rcClient);

#ifdef _DEBUG
	if ((GetKeyState(VK_SCROLL) & 1) && (GetKeyState(VK_CAPITAL) & 1)) {
		DebugStep(L"ConEmu: Sleeping in PaintCon for 1s");
		Sleep(1000);
		DebugStep(NULL);
	}
#endif
}

void CConEmuMain::RePaint()
{
    gConEmu.mp_TabBar->RePaint();
    m_Back.RePaint();
	HDC hDc = GetDC(ghWnd);
    //mp_VActive->Paint(hDc); // если mp_VActive==NULL - будет просто выполнена заливка фоном.
	PaintCon(hDc);
	ReleaseDC(ghWnd, hDc);
}

void CConEmuMain::Update(bool isForce /*= false*/)
{
    if (isForce) {
        for (int i=0; i<MAX_CONSOLE_COUNT; i++) {
            if (mp_VCon[i])
                mp_VCon[i]->OnFontChanged();
        }
    }

	CVirtualConsole::ClearPartBrushes();
    
    if (mp_VActive) {
        mp_VActive->Update(isForce);
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
    if (mp_VActive)
        dwPID = mp_VActive->RCon()->GetFarPID();
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
    if (!this || !apVCon)
        return false;
        
    if (apVCon == mp_VActive)
        return true;
        
    return false;
}

bool CConEmuMain::isConSelectMode()
{
    //TODO: По курсору, что-ли попробовать определять?
    //return gb_ConsoleSelectMode;
    if (mp_VActive)
        return mp_VActive->RCon()->isConSelectMode();
    return false;
}

bool CConEmuMain::isDragging()
{
    return (mouse.state & (DRAG_L_STARTED | DRAG_R_STARTED)) != 0;
}

bool CConEmuMain::isFilePanel(bool abPluginAllowed/*=false*/)
{
    if (!mp_VActive) return false;
    return mp_VActive->RCon()->isFilePanel(abPluginAllowed);
}

bool CConEmuMain::isFirstInstance()
{
	if (!mb_AliveInitialized) {
		mb_AliveInitialized = TRUE;
		
		// создадим событие, чтобы не было проблем с ключем /SINGLE
		lstrcpy(ms_ConEmuAliveEvent, CEGUI_ALIVE_EVENT);
		DWORD nSize = MAX_PATH;
		// Добавим имя текущего юзера. Нам не нужны конфликты при наличии нескольких юзеров.
		GetUserName(ms_ConEmuAliveEvent+lstrlen(ms_ConEmuAliveEvent), &nSize);
		mh_ConEmuAliveEvent = CreateEvent(NULL, TRUE, TRUE, ms_ConEmuAliveEvent);
		nSize = GetLastError();
		// имя пользователя теоретически может содержать символы, которые недопустимы в имени Event
		if (!mh_ConEmuAliveEvent /* || nSize == ERROR_PATH_NOT_FOUND */) {
			lstrcpy(ms_ConEmuAliveEvent, CEGUI_ALIVE_EVENT);
			mh_ConEmuAliveEvent = CreateEvent(NULL, TRUE, TRUE, ms_ConEmuAliveEvent);
			nSize = GetLastError();
		}
		mb_ConEmuAliveOwned = mh_ConEmuAliveEvent && (nSize!=ERROR_ALREADY_EXISTS);
	}

	if (mh_ConEmuAliveEvent && !mb_ConEmuAliveOwned) {
		if (WaitForSingleObject(mh_ConEmuAliveEvent,0) == WAIT_TIMEOUT) {
			SetEvent(mh_ConEmuAliveEvent);
			mb_ConEmuAliveOwned = TRUE;
		}
	}

	return mb_ConEmuAliveOwned;
}

bool CConEmuMain::isEditor()
{
    if (!mp_VActive) return false;
    return mp_VActive->RCon()->isEditor();
}

bool CConEmuMain::isFar()
{
    if (!mp_VActive) return false;
    return mp_VActive->RCon()->isFar();
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
    if (!mp_VActive) return false;
    return mp_VActive->RCon()->isNtvdm();
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
    if (!mp_VActive) return false;
    return mp_VActive->RCon()->isViewer();
}

bool CConEmuMain::isVisible(CVirtualConsole* apVCon)
{
    if (!this || !apVCon)
        return false;
        
    TODO("После добавления второго viewport'а потребуется коррекция");
    if (apVCon == mp_VActive || apVCon == mp_VCon1 || apVCon == mp_VCon2)
        return true;
        
    return false;
}

bool CConEmuMain::isWindowNormal()
{
    if (change2WindowMode != (DWORD)-1) {
        return (change2WindowMode == rNormal);
    }
    if (gSet.isFullScreen || isZoomed() || isIconic())
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

    if (mp_VActive && mp_VActive->RCon())
        hPictureView = mp_VActive->RCon()->isPictureView();
    else
        hPictureView = NULL;
    

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
void CConEmuMain::SwitchKeyboardLayout(DWORD_PTR dwNewKeybLayout)
{
    if ((gSet.isMonitorConsoleLang & 1) == 0)
        return;

	#ifdef _DEBUG
	wchar_t szDbg[128]; wsprintfW(szDbg, L"CConEmuMain::SwitchKeyboardLayout(0x%08I64X)\n", (unsigned __int64)dwNewKeybLayout);
	DEBUGSTRLANG(szDbg);
	#endif

    HKL hKeyb[20]; UINT nCount, i; BOOL lbFound = FALSE;
    nCount = GetKeyboardLayoutList ( countof(hKeyb), hKeyb );
    for (i = 0; !lbFound && i < nCount; i++) {
        if (hKeyb[i] == (HKL)dwNewKeybLayout)
            lbFound = TRUE;
    }
    WARNING("Похоже с другими раскладками будет глючить. US Dvorak?");
    for (i = 0; !lbFound && i < nCount; i++) {
        if ((((DWORD_PTR)hKeyb[i]) & 0xFFFF) == (dwNewKeybLayout & 0xFFFF)) {
            lbFound = TRUE; dwNewKeybLayout = (DWORD_PTR)hKeyb[i];
        }
    }
    // Если не задана раскладка (только язык?) формируем по умолчанию
    if (!lbFound && (dwNewKeybLayout == (dwNewKeybLayout & 0xFFFF)))
        dwNewKeybLayout |= (dwNewKeybLayout << 16);

    // Может она сейчас и активна?
    if (dwNewKeybLayout != GetActiveKeyboardLayout()) {

		#ifdef _DEBUG
		wsprintfW(szDbg, L"CConEmuMain::SwitchKeyboardLayout change to(0x%08I64X)\n", (unsigned __int64)dwNewKeybLayout);
		DEBUGSTRLANG(szDbg);
		#endif

        // Теперь переключаем раскладку
        PostMessage ( ghWnd, WM_INPUTLANGCHANGEREQUEST, 0, (LPARAM)dwNewKeybLayout );
        PostMessage ( ghWnd, WM_INPUTLANGCHANGE, 0, (LPARAM)dwNewKeybLayout );
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
                if (gConEmu.mp_TabBar->IsShown())
                    gSet.isTabs = 0;
                else
                    gSet.isTabs = 1;
                gConEmu.ForceShowTabs ( gSet.isTabs == 1 );
            } break;
        case 1:
            {
                gConEmu.mp_TabBar->SwitchNext();
            } break;
        case 2:
            {
                gConEmu.mp_TabBar->SwitchPrev();
            } break;
        case 3:
            {
                gConEmu.mp_TabBar->SwitchCommit();
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

void CConEmuMain::OnBufferHeight() //BOOL abBufferHeight)
{
	if (!gConEmu.isMainThread()) {
		PostMessage(ghWnd, mn_MsgPostOnBufferHeight, 0, 0);
		return;
	}

	BOOL lbBufferHeight = mp_VActive->RCon()->isBufferHeight();
    gConEmu.m_Back.TrackMouse(); // спрятать или показать прокрутку, если над ней мышка
    gConEmu.mp_TabBar->OnBufferHeight(lbBufferHeight);
}

TODO("И вообще, похоже это событие не вызывается");
LRESULT CConEmuMain::OnClose(HWND hWnd)
{
    _ASSERT(FALSE);
    //Icon.Delete(); - перенес в WM_DESTROY
    //mb_InClose = TRUE;
    if (ghConWnd) {
        mp_VActive->RCon()->CloseConsole();
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


    //if (gSet.isGUIpb && !ProgressBars) {
    //	ProgressBars = new CProgressBars(ghWnd, g_hInstance);
    //}

    // Установить переменную среды с дескриптором окна
    SetConEmuEnvVar(ghWndDC);


    HMENU hwndMain = GetSystemMenu(ghWnd, FALSE), hDebug = NULL;
    InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_TOTRAY, _T("Hide to &tray"));
    InsertMenu(hwndMain, 0, MF_BYPOSITION, MF_SEPARATOR, 0);
    InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_ABOUT, _T("&About"));
    if (ms_ConEmuChm[0]) //Показывать пункт только если есть conemu.chm
    	InsertMenu(hwndMain, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_HELP, _T("&Help"));
    InsertMenu(hwndMain, 0, MF_BYPOSITION, MF_SEPARATOR, 0);
    hDebug = CreatePopupMenu();
    AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_CON_TOGGLE_VISIBLE, _T("&Real console"));
    AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_CONPROP, _T("&Properties..."));
    AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_DUMPCONSOLE, _T("&Dump..."));
    AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_DEBUGGUI, _T("&Debug log..."));
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
        if (mp_VActive == NULL || !gConEmu.mb_StartDetached) { // Консоль уже может быть создана, если пришел Attach из ConEmuC
        	BOOL lbCreated = FALSE;
        	LPCWSTR pszCmd = gSet.GetCmd();
        	if (*pszCmd == L'@' && !gConEmu.mb_StartDetached) {
        		// В качестве "команды" указан "пакетный файл" одновременного запуска нескольких консолей
        		HANDLE hFile = CreateFile(pszCmd+1, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
        		if (!hFile) {
        			DWORD dwErr = GetLastError();
        			wchar_t* pszErrMsg = (wchar_t*)calloc(lstrlen(pszCmd)+100,2);
        			lstrcpy(pszErrMsg, L"Can't open console batch file:\n\xAB"/*«*/); lstrcat(pszErrMsg, pszCmd+1); lstrcat(pszErrMsg, L"\xBB"/*»*/);
	                DisplayLastError(pszErrMsg, dwErr);
	                free(pszErrMsg);
	                Destroy();
	                return;
        		}
        		DWORD nSize = GetFileSize(hFile, NULL);
        		if (!nSize || nSize > (1<<20)) {
        			DWORD dwErr = GetLastError();
        			CloseHandle(hFile);
        			wchar_t* pszErrMsg = (wchar_t*)calloc(lstrlen(pszCmd)+100,2);
        			lstrcpy(pszErrMsg, L"Console batch file is too large or empty:\n\xAB"/*«*/); lstrcat(pszErrMsg, pszCmd+1); lstrcat(pszErrMsg, L"\xBB"/*»*/);
	                DisplayLastError(pszErrMsg, dwErr);
	                free(pszErrMsg);
	                Destroy();
	                return;
        		}
        		char* pszDataA = (char*)calloc(nSize+4,1);
        		_ASSERTE(pszDataA);
        		DWORD nRead = 0;
        		BOOL lbRead = ReadFile(hFile, pszDataA, nSize, &nRead, 0);
        		DWORD dwErr = GetLastError();
        		CloseHandle(hFile);
        		if (!lbRead || nRead != nSize) {
        			free(pszDataA);
        			wchar_t* pszErrMsg = (wchar_t*)calloc(lstrlen(pszCmd)+100,2);
        			lstrcpy(pszErrMsg, L"Reading console batch file failed:\n\xAB"/*«*/); lstrcat(pszErrMsg, pszCmd+1); lstrcat(pszErrMsg, L"\xBB"/*»*/);
	                DisplayLastError(pszErrMsg, dwErr);
	                free(pszErrMsg);
	                Destroy();
	                return;
        		}
        		// Опредлить код.страницу файла
        		wchar_t* pszDataW = NULL; BOOL lbNeedFreeW = FALSE;
        		if (pszDataA[0] == 0xEF && pszDataA[1] == 0xBB && pszDataA[2] == 0xBF) {
        			// UTF-8 BOM
        			pszDataW = (wchar_t*)calloc(nSize+2,2); lbNeedFreeW = TRUE;
        			_ASSERTE(pszDataW);
        			MultiByteToWideChar(CP_UTF8, 0, pszDataA+3, -1, pszDataW, nSize);
        		} else if (pszDataA[0] == 0xFF && pszDataA[1] == 0xFE) {
        			// CP-1200 BOM
        			pszDataW = (wchar_t*)(pszDataA+2);
        		} else {
        			// Plain ANSI
        			pszDataW = (wchar_t*)calloc(nSize+2,2); lbNeedFreeW = TRUE;
        			_ASSERTE(pszDataW);
        			MultiByteToWideChar(CP_ACP, 0, pszDataA, -1, pszDataW, nSize+1);
        		}
        		// Поехали
        		wchar_t *pszLine = pszDataW;
        		wchar_t *pszNewLine = wcschr(pszLine, L'\r');
        		CVirtualConsole *pSetActive = NULL, *pCon = NULL;
        		BOOL lbSetActive = FALSE, lbOneCreated = FALSE, lbRunAdmin = FALSE;
        		while (*pszLine) {
        			lbSetActive = lbRunAdmin = FALSE;
        			while (*pszLine == L'>' || *pszLine == L'*' || *pszLine == L' ' || *pszLine == L'\t') {
        				if (*pszLine == L'>') lbSetActive = TRUE;
        				if (*pszLine == L'*') lbRunAdmin = TRUE;
	        			pszLine++;
    	    		}
        			
        			if (pszNewLine) *pszNewLine = 0;
        			while (*pszLine == L' ') pszLine++;
        			
        			if (*pszLine) {
        				while (pszLine[0] == L'/') {
        					if (CSTR_EQUAL == CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE|SORT_STRINGSORT,
        							pszLine, 14, L"/bufferheight ", 14))
							{
								pszLine += 14;
								while (*pszLine == L' ') pszLine++;
								wchar_t* pszEnd = NULL;
								long lBufHeight = wcstol(pszLine, &pszEnd, 10);
								gSet.SetArgBufferHeight ( lBufHeight );
								if (pszEnd) pszLine = pszEnd;
							}
							
        					if (CSTR_EQUAL == CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE|SORT_STRINGSORT,
        							pszLine, 5, L"/cmd ", 5))
							{
								pszLine += 5;
							}
							
							while (*pszLine == L' ') pszLine++;
        				}

        				if (*pszLine) {
							RConStartArgs args;
							args.pszSpecialCmd = _wcsdup(pszLine);
							args.bRunAsAdministrator = lbRunAdmin;
							pCon = CreateCon(&args);
				            if (!pCon) {
				                DisplayLastError(L"Can't create new virtual console!");
				                if (!lbOneCreated) {
					                Destroy();
					                return;
				                }
				            } else {
				            	lbOneCreated = TRUE;
				            	if (lbSetActive && !pSetActive)
				            		pSetActive = pCon;
				            		
				            	if (GetVCon(MAX_CONSOLE_COUNT-1))
				            		break; // Больше создать не получится
		            		}
		            	}

						MSG Msg;
						while (PeekMessage(&Msg,0,0,0,PM_REMOVE)) {
							if (Msg.message == WM_QUIT)
								return;
							BOOL lbDlgMsg = FALSE;
							if (ghOpWnd) {
								if (IsWindow(ghOpWnd))
									lbDlgMsg = IsDialogMessage(ghOpWnd, &Msg);
							}
							if (!lbDlgMsg)
							{
								TranslateMessage(&Msg);
								DispatchMessage(&Msg);
							}
						}
						if (!ghWnd || !IsWindow(ghWnd))
							return;
	            	}
	            	
	            	pszLine = pszNewLine+1;
	            	if (!*pszLine) break;
	            	if (*pszLine == L'\n') pszLine++;
	            	pszNewLine = wcschr(pszLine, L'\r');
        		}
        		if (pSetActive) {
        			Activate(pSetActive);
        		}
        		if (pszDataW && lbNeedFreeW) free(pszDataW); pszDataW = NULL;
        		if (pszDataA) free(pszDataA); pszDataA = NULL;

				lbCreated = TRUE;
        	}
        	
        	if (!lbCreated)
        	{
				RConStartArgs args; args.bDetached = gConEmu.mb_StartDetached;
	            if (!CreateCon(&args)) {
	                DisplayLastError(L"Can't create new virtual console!");
	                Destroy();
	                return;
	            }
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
            mp_DragDrop = new CDragDrop();
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
        n = SetTimer(ghWnd, 1, CON_REDRAW_TIMOUT*2, NULL);
    }
}

LRESULT CConEmuMain::OnDestroy(HWND hWnd)
{
	if (mb_ConEmuAliveOwned && mh_ConEmuAliveEvent)
	{
		ResetEvent(mh_ConEmuAliveEvent); // Дадим другим процессам "завладеть" первенством
		SafeCloseHandle(mh_ConEmuAliveEvent);
		mb_ConEmuAliveOwned = FALSE;
	}

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
    //if (ProgressBars) {
    //    delete ProgressBars;
    //    ProgressBars = NULL;
    //}

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

LRESULT CConEmuMain::OnFlashWindow(DWORD nFlags, DWORD nCount, HWND hCon)
{
    if (!hCon) return 0;
    
    BOOL lbRc = FALSE;
    for (int i = 0; i<MAX_CONSOLE_COUNT; i++) {
        if (!mp_VCon[i]) continue;
        
        if (mp_VCon[i]->RCon()->ConWnd() == hCon) {
            FLASHWINFO fl = {sizeof(FLASHWINFO)};
            
            if (isMeForeground()) {
            	if (mp_VCon[i] != mp_VActive) { // Только для неактивной консоли
                    fl.dwFlags = FLASHW_STOP; fl.hwnd = ghWnd;
                    FlashWindowEx(&fl); // Чтобы мигание не накапливалось
                    
            		fl.uCount = 4; fl.dwFlags = FLASHW_ALL; fl.hwnd = ghWnd;
            		FlashWindowEx(&fl);
            	}
            } else {
            	fl.dwFlags = FLASHW_ALL|FLASHW_TIMERNOFG; fl.hwnd = ghWnd;
            	FlashWindowEx(&fl); // Помигать в GUI
            }
            
            //fl.dwFlags = FLASHW_STOP; fl.hwnd = hCon; -- не требуется, т.к. это хучится
            //FlashWindowEx(&fl);
            break;
        }
    }
            
	return lbRc;
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
        gConEmu.mp_TabBar->SwitchRollback();
        
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

    if (mp_VActive /*&& (messg == WM_SETFOCUS || messg == WM_KILLFOCUS)*/) {
        mp_VActive->RCon()->OnFocus(lbSetFocus);
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
    
    if (gSet.isHideCaption) {
    	pInfo->ptMaxPosition.y -= GetSystemMetrics(SM_CYCAPTION);
    	//pInfo->ptMaxSize.y += GetSystemMetrics(SM_CYCAPTION);
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
		WARNING("BUGBUG: похоже глючит в x64 на US-Dvorak");
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
    if (gConEmu.mp_VActive->RCon()->isBufferHeight() && (messg == WM_KEYDOWN || messg == WM_KEYUP) &&
        (wParam == VK_DOWN || wParam == VK_UP || wParam == VK_NEXT || wParam == VK_PRIOR) &&
        isPressed(VK_CONTROL)
    ) {
        if (messg != WM_KEYDOWN || !mp_VActive)
            return 0;
            
        switch(wParam)
        {
        case VK_DOWN:
            return mp_VActive->RCon()->OnScroll(SB_LINEDOWN);
        case VK_UP:
            return mp_VActive->RCon()->OnScroll(SB_LINEUP);
        case VK_NEXT:
            return mp_VActive->RCon()->OnScroll(SB_PAGEDOWN);
        case VK_PRIOR:
            return mp_VActive->RCon()->OnScroll(SB_PAGEUP);
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
    if (/*gSet.isTabs &&*/ gSet.isTabSelf && /*gConEmu.mp_TabBar->IsShown() &&*/
        (
         ((messg == WM_KEYDOWN || messg == WM_KEYUP) 
           && (wParam == VK_TAB 
               || (gConEmu.mp_TabBar->IsInSwitch() 
                   && (wParam == VK_UP || wParam == VK_DOWN || wParam == VK_LEFT || wParam == VK_RIGHT))))
         //|| (messg == WM_CHAR && wParam == VK_TAB)
        ))
    {
        if (isPressed(VK_CONTROL) /*&& !isPressed(VK_MENU)*/ && !isPressed(VK_LWIN) && !isPressed(VK_LWIN))
        {
            if (gConEmu.mp_TabBar->OnKeyboard(messg, wParam, lParam))
                return 0;
        }
    }
    // !!! Запрос на переключение мог быть инициирован из плагина
    if (messg == WM_KEYUP && (wParam == VK_CONTROL || wParam == VK_LCONTROL || wParam == VK_RCONTROL))
	{/*&& gConEmu.mp_TabBar->IsShown()*/
		if (gSet.isTabSelf || (gSet.isTabLazy && gConEmu.mp_TabBar->IsInSwitch()))
		{
			gConEmu.mp_TabBar->SwitchCommit(); // Если переключения не было - ничего не делает
			// В фар отпускание кнопки таки пропустим
		}
    }

    // Conman
    static bool sb_SkipConmanChar = false;
    static DWORD sn_SkipConmanVk[2] = {0,0};
    bool lbLWin = false, lbRWin = false;
    TODO("gSet.nMultiHotkeyModifier - байты содержат VK_CONTROL, VK_MENU, VK_SHIFT");
    TODO("gSet.icMultiBuffer - хоткей для включения-отключения режима буфера - AskChangeBufferHeight()");
    if (gSet.isMulti && wParam && ((lbLWin = isPressed(VK_LWIN)) || (lbRWin = isPressed(VK_RWIN)) || sb_SkipConmanChar)) {
        if (messg == WM_KEYDOWN && (lbLWin || lbRWin) && (wParam != VK_LWIN && wParam != VK_RWIN)) {
            if (wParam==gSet.icMultiNext || wParam==gSet.icMultiNew || wParam==gSet.icMultiRecreate
            	|| (wParam>='0' && wParam<='9')
				|| ((lbLWin || lbRWin) && (wParam==VK_F11 || wParam==VK_F12)) // KeyDown для этого не проходит, но на всякий случай
                )
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
                    
				else if (wParam == gSet.icMultiNext /* L'Q' */) { // Win-Q
                    ConActivateNext(isPressed(VK_SHIFT) ? FALSE : TRUE);
                    
				} else if (wParam == gSet.icMultiNew /* L'W' */) { // Win-W
                    // Создать новую консоль
                    Recreate ( FALSE, gSet.isMultiNewConfirm );
                    
                } else if (wParam == gSet.icMultiRecreate /* L'~' */) { // Win-~
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

	if (wParam == VK_ESCAPE) {
		if (mp_DragDrop->InDragDrop())
			return 0;
		static bool bEscPressed = false;
		if (messg != WM_KEYUP)
			bEscPressed = true;
		else if (!bEscPressed)
			return 0;
		else
			bEscPressed = false;
	}

    if (mp_VActive) {
		//#ifdef _DEBUG
		//if (wParam == VK_LEFT) {
		//	if (messg == WM_KEYDOWN)
		//		OutputDebugString(L"VK_LEFT pressed\n");
		//	else if (messg == WM_KEYUP)
		//		OutputDebugString(L"VK_LEFT released\n");
		//}
		//#endif

        mp_VActive->RCon()->OnKeyboard(hWnd, messg, wParam, lParam, szTranslatedChars);
    }

    return 0;
}

LRESULT CConEmuMain::OnLangChange(UINT messg, WPARAM wParam, LPARAM lParam)
{
	
	/*
	**********
	Вызовы идут в следующем порядке (WinXP SP3)
	**********
	En -> Ru : --> Слетает после этого!
	**********
	19:17:15.043(gui.4720) ConEmu: WM_INPUTLANGCHANGEREQUEST(CP:1, HKL:0x04190419)
	19:17:17.043(gui.4720) ConEmu: WM_INPUTLANGCHANGE(CP:204, HKL:0x04190419)
	19:17:19.043(gui.4720) ConEmu: GetKeyboardLayout(0) after DefWindowProc(WM_INPUTLANGCHANGE) = 0x04190419)
	19:17:21.043(gui.4720) ConEmu: GetKeyboardLayout(0) after DefWindowProc(WM_INPUTLANGCHANGEREQUEST) = 0x04190419)
	19:17:23.043(gui.4720) CRealConsole::SwitchKeyboardLayout(CP:1, HKL:0x04190419)
	19:17:25.044(gui.4720) RealConsole: WM_INPUTLANGCHANGEREQUEST, CP:1, HKL:0x04190419 via CmdExecute
	19:17:27.044(gui.3072) GUI recieved CECMD_LANGCHANGE
	19:17:29.044(gui.4720) ConEmu: GetKeyboardLayout(0) in OnLangChangeConsole after GetKeyboardLayout(0) = 0x04190419
	19:17:31.044(gui.4720) ConEmu: GetKeyboardLayout(0) in OnLangChangeConsole after GetKeyboardLayout(0) = 0x04190419
	--> Слетает после этого!
	'ConEmu.exe': Loaded 'C:\WINDOWS\system32\kbdru.dll'
	'ConEmu.exe': Unloaded 'C:\WINDOWS\system32\kbdru.dll'
	19:17:33.075(gui.4720) ConEmu: Calling GetKeyboardLayout(0)
	19:17:35.075(gui.4720) ConEmu: GetKeyboardLayout(0) after LoadKeyboardLayout = 0x04190419
	19:17:37.075(gui.4720) ConEmu: GetKeyboardLayout(0) after SwitchKeyboardLayout = 0x04190419
	**********
	Ru -> En : --> Слетает после этого!
	**********
	17:23:36.013(gui.3152) ConEmu: WM_INPUTLANGCHANGEREQUEST(CP:1, HKL:0x04090409)
	17:23:36.013(gui.3152) ConEmu: WM_INPUTLANGCHANGE(CP:0, HKL:0x04090409)
	17:23:36.013(gui.3152) ConEmu: GetKeyboardLayout(0) after DefWindowProc(WM_INPUTLANGCHANGE) = 0x04090409)
	17:23:36.013(gui.3152) ConEmu: GetKeyboardLayout(0) after DefWindowProc(WM_INPUTLANGCHANGEREQUEST) = 0x04090409)
	17:23:36.013(gui.3152) CRealConsole::SwitchKeyboardLayout(CP:1, HKL:0x04090409)
	17:23:36.013(gui.3152) RealConsole: WM_INPUTLANGCHANGEREQUEST, CP:1, HKL:0x04090409 via CmdExecute
	ConEmuC: PostMessage(WM_INPUTLANGCHANGEREQUEST, CP:1, HKL:0x04090409)
	The thread 'Win32 Thread' (0x3f0) has exited with code 1 (0x1).
	ConEmuC: InputLayoutChanged (GetConsoleKeyboardLayoutName returns) '00000409'
	17:23:38.013(gui.4460) GUI recieved CECMD_LANGCHANGE
	17:23:40.028(gui.4460) ConEmu: GetKeyboardLayout(0) on CECMD_LANGCHANGE after GetKeyboardLayout(0) = 0x04090409
	--> Слетает после этого!
	'ConEmu.exe': Loaded 'C:\WINDOWS\system32\kbdus.dll'
	'ConEmu.exe': Unloaded 'C:\WINDOWS\system32\kbdus.dll'
	17:23:42.044(gui.4460) ConEmu: Calling GetKeyboardLayout(0)
	17:23:44.044(gui.4460) ConEmu: GetKeyboardLayout(0) after LoadKeyboardLayout = 0x04090409
	17:23:46.044(gui.4460) ConEmu: GetKeyboardLayout(0) after SwitchKeyboardLayout = 0x04090409
	*/

    LRESULT result = 1;
    #ifdef _DEBUG
    WCHAR szMsg[255];
    wsprintf(szMsg, L"ConEmu: %s(CP:%i, HKL:0x%08I64X)\n",
        (messg == WM_INPUTLANGCHANGE) ? L"WM_INPUTLANGCHANGE" : L"WM_INPUTLANGCHANGEREQUEST",
        (DWORD)wParam, (unsigned __int64)(DWORD_PTR)lParam);
    DEBUGSTRLANG(szMsg);
    #endif
    
    //POSTMESSAGE(ghConWnd, messg, wParam, lParam, FALSE);

    //mn_CurrentKeybLayout = lParam;

    result = DefWindowProc(ghWnd, messg, wParam, lParam);

    #ifdef _DEBUG
    HKL hkl = GetKeyboardLayout(0);
    wsprintf(szMsg, L"ConEmu: GetKeyboardLayout(0) after DefWindowProc(%s) = 0x%08I64X)\n",
		(messg == WM_INPUTLANGCHANGE) ? L"WM_INPUTLANGCHANGE" : L"WM_INPUTLANGCHANGEREQUEST",
		(unsigned __int64)(DWORD_PTR)hkl);
    DEBUGSTRLANG(szMsg);
    #endif
	if (messg == WM_INPUTLANGCHANGEREQUEST) {
		mp_VActive->RCon()->SwitchKeyboardLayout(wParam,lParam);
	}
    
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
        //      //gConEmu.DebugStep(_T("ConEmu: Switching language (1 sec)"));
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

LRESULT CConEmuMain::OnLangChangeConsole(CVirtualConsole *apVCon, DWORD dwLayoutName)
{
	if ((gSet.isMonitorConsoleLang & 1) != 1)
		return 0;

	if (!isValid(apVCon))
		return 0;
	
	if (!isMainThread()) {
		PostMessage(ghWnd, mn_ConsoleLangChanged, dwLayoutName, (LPARAM)apVCon);
		return 0;
	}

	#ifdef _DEBUG
	//Sleep(2000);
	WCHAR szMsg[255];
	// --> Видимо именно это не "нравится" руслату. Нужно переправить Post'ом в основную нить
	HKL hkl = GetKeyboardLayout(0);
	wsprintf(szMsg, L"ConEmu: GetKeyboardLayout(0) in OnLangChangeConsole after GetKeyboardLayout(0) = 0x%08I64X\n",
		(unsigned __int64)(DWORD_PTR)hkl);
	DEBUGSTRLANG(szMsg);
	//Sleep(2000);
	#endif

	wchar_t szName[10]; wsprintf(szName, L"%08X", dwLayoutName);
	#ifdef _DEBUG
	DEBUGSTRLANG(szName);
	#endif
	// --> Тут слетает!
    DWORD_PTR dwNewKeybLayout = dwLayoutName; //(DWORD_PTR)LoadKeyboardLayout(szName, 0);

	HKL hKeyb[20]; UINT nCount, i; BOOL lbFound = FALSE;
	nCount = GetKeyboardLayoutList ( countof(hKeyb), hKeyb );
	for (i = 0; !lbFound && i < nCount; i++) {
		if (hKeyb[i] == (HKL)dwNewKeybLayout)
			lbFound = TRUE;
	}
	WARNING("Похоже с другими раскладками будет глючить. US Dvorak?");
	for (i = 0; !lbFound && i < nCount; i++) {
		if ((((DWORD_PTR)hKeyb[i]) & 0xFFFF) == (dwNewKeybLayout & 0xFFFF)) {
			lbFound = TRUE; dwNewKeybLayout = (DWORD_PTR)hKeyb[i];
		}
	}
	// Если не задана раскладка (только язык?) формируем по умолчанию
	if (!lbFound && (dwNewKeybLayout == (dwNewKeybLayout & 0xFFFF)))
		dwNewKeybLayout |= (dwNewKeybLayout << 16);


	#ifdef _DEBUG
	DEBUGSTRLANG(L"ConEmu: Calling GetKeyboardLayout(0)\n");
	//Sleep(2000);
	hkl = GetKeyboardLayout(0);
	wsprintf(szMsg, L"ConEmu: GetKeyboardLayout(0) after LoadKeyboardLayout = 0x%08I64X\n",
		(unsigned __int64)(DWORD_PTR)hkl);
	DEBUGSTRLANG(szMsg);
	//Sleep(2000);
	#endif

	if (isActive(apVCon))
		apVCon->RCon()->OnConsoleLangChange(dwNewKeybLayout);

	return 0;
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
				TODO("После скрытия ViewPort наверное SetCapture нужно будет делать на ghWnd");
				SetCapture(ghWndDC);
			}
		}
	} else {
		// Все кнопки мышки отпущены - release
		if (!isPressed(VK_LBUTTON) && !isPressed(VK_RBUTTON) && !isPressed(VK_MBUTTON)) {
			ReleaseCapture();
			mb_MouseCaptured = FALSE;

			if (mp_VActive->RCon()->isMouseButtonDown()) {
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
	
    if (!mp_VActive)
        return 0;

    if (gSet.FontWidth()==0 || gSet.FontHeight()==0)
        return 0;

#ifdef MSGLOGGER
    if (messg != WM_MOUSEMOVE) {
        TODO("SetConsoleMode");
        /*DWORD mode = 0;
        BOOL lb = GetConsoleMode(mp_VActive->hConIn(), &mode);
        if (!(mode & ENABLE_MOUSE_INPUT)) {
            mode |= ENABLE_MOUSE_INPUT;
            DEBUGSTR(L"!!! Enabling mouse input in console\n");
            lb = SetConsoleMode(mp_VActive->hConIn(), mode);
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

    COORD cr = mp_VActive->ClientToConsole(ptCur.x,ptCur.y);
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
			DEBUGSTRMOUSE(L"Skipping Mouse down\n");
		} else
		if (gConEmu.mouse.nSkipEvents[1] == messg) {
			gConEmu.mouse.nSkipEvents[1] = 0;
			DEBUGSTRMOUSE(L"Skipping Mouse up\n");
		} 
		#ifdef _DEBUG
		else if (messg == WM_MOUSEMOVE) {
			DEBUGSTRMOUSE(L"Skipping Mouse move\n");
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

	// Forwarding сообщений в нить драга
	if (mp_DragDrop->IsDragStarting()) {
		if (mp_DragDrop->ForwardMessage(hWnd, messg, wParam, lParam))
			return 0;
	}

    ///*&& isPressed(VK_LBUTTON)*/) && // Если этого не делать - при выделении мышкой консоль может самопроизвольно прокрутиться
    CRealConsole* pRCon = mp_VActive->RCon();
    // Только при скрытой консоли. Иначе мы ConEmu видеть вообще не будем
    if (pRCon && (/*pRCon->isBufferHeight() ||*/ !pRCon->isWindowVisible())
        && (messg == WM_LBUTTONDOWN || messg == WM_RBUTTONDOWN || messg == WM_LBUTTONDBLCLK || messg == WM_RBUTTONDBLCLK
            || (messg == WM_MOUSEMOVE && isPressed(VK_LBUTTON)))
        )
    {
       // buffer mode: cheat the console window: adjust its position exactly to the cursor
       RECT win;
       GetWindowRect(ghWnd, &win);
       short x = win.left + ptCur.x - MulDiv(ptCur.x, conRect.right, klMax<uint>(1, mp_VActive->Width));
       short y = win.top + ptCur.y - MulDiv(ptCur.y, conRect.bottom, klMax<uint>(1, mp_VActive->Height));
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
        if (gSet.isDragEnabled & ((mouse.state & (DRAG_L_ALLOWED|DRAG_R_ALLOWED))!=0)
			&& !isPictureView())
        {
			if (mp_DragDrop==NULL) {
				DebugStep(_T("DnD: Drag-n-Drop is null"));
			} else {
				mouse.bIgnoreMouseMove = true;

				BOOL lbDiffTest = PTDIFFTEST(cursor,DRAG_DELTA); // TRUE - если курсор НЕ двигался (далеко)
				if (!lbDiffTest && !isDragging() && !isSizing())
				{
					// 2009-06-19 Выполнялось сразу после "if (gSet.isDragEnabled & (mouse.state & (DRAG_L_ALLOWED|DRAG_R_ALLOWED)))"
					if (mouse.state & MOUSE_R_LOCKED) {
						// чтобы при RightUp не ушел APPS
						mouse.state &= ~MOUSE_R_LOCKED;
						
						//PtDiffTest(C, LOWORD(lParam), HIWORD(lParam), D)
		                char szLog[255];
		                wsprintfA(szLog, "Right drag started, MOUSE_R_LOCKED cleared: cursor={%i-%i}, Rcursor={%i-%i}, Now={%i-%i}, MinDelta=%i",
		                	(int)cursor.x, (int)cursor.y,
		                	(int)Rcursor.x, (int)Rcursor.y,
		                	(int)LOWORD(lParam), (int)HIWORD(lParam),
		                	(int)DRAG_DELTA);
		                mp_VActive->RCon()->LogString(szLog);
					}

					BOOL lbLeftDrag = (mouse.state & DRAG_L_ALLOWED) == DRAG_L_ALLOWED;

					mp_VActive->RCon()->LogString(lbLeftDrag ? "Left drag about to start" : "Right drag about to start");

					// Если сначала фокус был на файловой панели, но после LClick он попал на НЕ файловую - отменить ShellDrag
					bool bFilePanel = isFilePanel();
					if (!bFilePanel) {
						DebugStep(_T("DnD: not file panel"));
						//isLBDown = false; 
						mouse.state &= ~(DRAG_L_ALLOWED | DRAG_L_STARTED | DRAG_R_ALLOWED | DRAG_R_STARTED);
						mouse.bIgnoreMouseMove = false;
						//POSTMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( newX, newY ), TRUE);     //посылаем консоли отпускание
						mp_VActive->RCon()->OnMouse(WM_LBUTTONUP, wParam, ptCur.x, ptCur.x);
						//ReleaseCapture(); --2009-03-14
						return 0;
					}
	                
					// Чтобы сам фар не дергался на MouseMove...
					//isLBDown = false;
					mouse.state &= ~(DRAG_L_ALLOWED | DRAG_L_STARTED | DRAG_R_STARTED); // флажок поставит сам CDragDrop::Drag() по наличию DRAG_R_ALLOWED
					// вроде валится, если перед Drag
					//POSTMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( newX, newY ), TRUE);     //посылаем консоли отпускание

					//TODO("Тут бы не посылать мышь в очередь, а передавать через аргумент команды GetDragInfo в плагин");
					//if (lbLeftDrag) { // сразу "отпустить" клавишу
					//	//POSTMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( mouse.LClkCon.X, mouse.LClkCon.Y ), TRUE);     //посылаем консоли отпускание
					//	mp_VActive->RCon()->OnMouse(WM_LBUTTONUP, wParam, mouse.LClkDC.X, mouse.LClkDC.Y);
					//} else {
					//	//POSTMESSAGE(ghConWnd, WM_LBUTTONDOWN, wParam, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);     //посылаем консоли отпускание
					//	mp_VActive->RCon()->OnMouse(WM_LBUTTONDOWN, wParam, mouse.RClkDC.X, mouse.RClkDC.Y);
					//	//POSTMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);     //посылаем консоли отпускание
					//	mp_VActive->RCon()->OnMouse(WM_LBUTTONUP, wParam, mouse.RClkDC.X, mouse.RClkDC.Y);
					//}
					//mp_VActive->RCon()->FlushInputQueue();
	                
					// Иначе иногда срабатывает FAR'овский D'n'D
					//SENDMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( newX, newY ));     //посылаем консоли отпускание
					if (mp_DragDrop) {
						//COORD crMouse = mp_VActive->ClientToConsole(
						//	lbLeftDrag ? mouse.LClkDC.X : mouse.RClkDC.X,
						//	lbLeftDrag ? mouse.LClkDC.Y : mouse.RClkDC.Y);
						DebugStep(_T("DnD: Drag-n-Drop starting"));
						mp_DragDrop->Drag(!lbLeftDrag, lbLeftDrag ? mouse.LClkDC : mouse.RClkDC);
						DebugStep(Title); // вернуть заголовок
					} else {
						_ASSERTE(mp_DragDrop); // должно быть обработано выше
						DebugStep(_T("DnD: Drag-n-Drop is null"));
					}
					mouse.bIgnoreMouseMove = false;
	                
					//#ifdef NEWMOUSESTYLE
					//newX = cursor.x; newY = cursor.y;
					//#else
					//newX = MulDiv(cursor.x, conRect.right, klMax<uint>(1, mp_VActive->Width));
					//newY = MulDiv(cursor.y, conRect.bottom, klMax<uint>(1, mp_VActive->Height));
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
                
                char szLog[255];
                wsprintfA(szLog, "Mouse was moved, MOUSE_R_LOCKED cleared: Rcursor={%i-%i}, Now={%i-%i}, MinDelta=%i",
                	(int)Rcursor.x, (int)Rcursor.y,
                	(int)LOWORD(lParam), (int)HIWORD(lParam),
                	(int)RCLICKAPPSDELTA);
                mp_VActive->RCon()->LogString(szLog);
                
                //POSTMESSAGE(ghConWnd, WM_RBUTTONDOWN, 0, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);
                mp_VActive->RCon()->OnMouse(WM_RBUTTONDOWN, 0, mouse.RClkDC.X, mouse.RClkDC.Y);
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
            if (!isConSelectMode() && isFilePanel() && mp_VActive &&
				mp_VActive->RCon()->CoordInPanel(mouse.LClkCon))
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
                mp_VActive->RCon()->OnMouse(messg, wParam, ptCur.x, ptCur.y);
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
                //newX = MulDiv(cursor.x, conRect.right, klMax<uint>(1, mp_VActive->Width));
                //newY = MulDiv(cursor.y, conRect.bottom, klMax<uint>(1, mp_VActive->Height));
                //#endif
                //POSTMESSAGE(ghConWnd, messg, wParam, MAKELPARAM( newX, newY ), FALSE);
                mp_VActive->RCon()->OnMouse(messg, wParam, cursor.x, cursor.y);
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

            {
                char szLog[100];
                wsprintfA(szLog, "Right button down: Rcursor={%i-%i}", (int)Rcursor.x, (int)Rcursor.y);
                mp_VActive->RCon()->LogString(szLog);
            }
            
            //if (isFilePanel()) // Maximus5
            if (!isConSelectMode() && isFilePanel() && mp_VActive &&
				mp_VActive->RCon()->CoordInPanel(mouse.RClkCon))
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
                    
	                char szLog[100];
	                wsprintfA(szLog, "MOUSE_R_LOCKED was set: Rcursor={%i-%i}", (int)Rcursor.x, (int)Rcursor.y);
	                mp_VActive->RCon()->LogString(szLog);
                    
                    //SetTimer(hWnd, 1, 300, 0); -- Maximus5, откажемся от таймера
                    mouse.RClkTick = TimeGetTime(); //GetTickCount();
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
                    DWORD dwCurTick = TimeGetTime(); //GetTickCount();
                    DWORD dwDelta=dwCurTick-mouse.RClkTick;
                    // Если держали дольше .3с, но не слишком долго :)
                    if ((gSet.isRClickSendKey==1) ||
                        (dwDelta>RCLICKAPPSTIMEOUT && dwDelta<10000))
                    {
                        //// Сначала выделить файл под курсором
                        ////POSTMESSAGE(ghConWnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);
                        //mp_VActive->RCon()->OnMouse(WM_LBUTTONDOWN, MK_LBUTTON, mouse.RClkDC.X, mouse.RClkDC.Y);
                        ////POSTMESSAGE(ghConWnd, WM_LBUTTONUP, 0, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);
                        //mp_VActive->RCon()->OnMouse(WM_LBUTTONUP, 0, mouse.RClkDC.X, mouse.RClkDC.Y);
                        //
                        //mp_VActive->RCon()->FlushInputQueue();
                    
                        //mp_VActive->Update(true);
                        //INVALIDATE(); //InvalidateRect(HDCWND, NULL, FALSE);

                        // А теперь можно и Apps нажать
                        mouse.bSkipRDblClk=true; // чтобы пока FAR думает в консоль не проскочило мышиное сообщение
                        //POSTMESSAGE(ghConWnd, WM_KEYDOWN, VK_APPS, 0, TRUE);

                        DWORD dwFarPID = mp_VActive->RCon()->GetFarPID();
                        if (dwFarPID)
                        {
                        	AllowSetForegroundWindow(dwFarPID);

	                        //if (gSet.sRClickMacro && *gSet.sRClickMacro) {
		                    //    //// Сначала выделить файл под курсором
		                    //    ////POSTMESSAGE(ghConWnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);
		                    //    //mp_VActive->RCon()->OnMouse(WM_LBUTTONDOWN, MK_LBUTTON, mouse.RClkDC.X, mouse.RClkDC.Y);
		                    //    ////POSTMESSAGE(ghConWnd, WM_LBUTTONUP, 0, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);
		                    //    //mp_VActive->RCon()->OnMouse(WM_LBUTTONUP, 0, mouse.RClkDC.X, mouse.RClkDC.Y);
		                    //    //
		                    //    //mp_VActive->RCon()->FlushInputQueue();
		                    //
	                        //    // Если юзер задал свой макрос - выполняем его
	                        //    PostMacro(gSet.sRClickMacro);
	                        //} else {
                        
                        	COORD crMouse = mp_VActive->ClientToConsole(mouse.RClkDC.X, mouse.RClkDC.Y);
                        	
							CConEmuPipe pipe(GetFarPID(), CONEMUREADYTIMEOUT);
							if (pipe.Init(_T("CConEmuMain::EMenu"), TRUE))
							{
								//DWORD cbWritten=0;
								DebugStep(_T("EMenu: Waiting for result (10 sec)"));
								
								int nLen = 0;
								int nSize = sizeof(crMouse) + sizeof(wchar_t);
								if (gSet.sRClickMacro && *gSet.sRClickMacro) {
									nLen = lstrlen(gSet.sRClickMacro);
									nSize += nLen*2;
									mp_VActive->RCon()->RemoveFromCursor();
								}
								LPBYTE pcbData = (LPBYTE)calloc(nSize,1);
								_ASSERTE(pcbData);
								
								memmove(pcbData, &crMouse, sizeof(crMouse));
								if (nLen)
									lstrcpy((wchar_t*)(pcbData+sizeof(crMouse)), gSet.sRClickMacro);
								
								if (!pipe.Execute(CMD_EMENU, pcbData, nSize)) {
			                        mp_VActive->RCon()->LogString("RightClicked, but pipe.Execute(CMD_EMENU) failed");
			                    }
								DebugStep(NULL);
								
								free(pcbData);

							} else {
								mp_VActive->RCon()->LogString("RightClicked, but pipe.Init() failed");
							}

							//INPUT_RECORD r = {KEY_EVENT};
							////mp_VActive->RCon()->On Keyboard(ghConWnd, WM_KEYDOWN, VK_APPS, (VK_APPS << 16) | (1 << 24));
							////mp_VActive->RCon()->On Keyboard(ghConWnd, WM_KEYUP, VK_APPS, (VK_APPS << 16) | (1 << 24));
							//r.Event.KeyEvent.bKeyDown = TRUE;
							//r.Event.KeyEvent.wVirtualKeyCode = VK_APPS;
							//r.Event.KeyEvent.wVirtualScanCode = /*28 на моей клавиатуре*/MapVirtualKey(VK_APPS, 0/*MAPVK_VK_TO_VSC*/);
							//r.Event.KeyEvent.dwControlKeyState = 0x120;
							//mp_VActive->RCon()->PostConsoleEvent(&r);

							////On Keyboard(hConWnd, WM_KEYUP, VK_RETURN, 0);
							//r.Event.KeyEvent.bKeyDown = FALSE;
							//r.Event.KeyEvent.dwControlKeyState = 0x120;
							//mp_VActive->RCon()->PostConsoleEvent(&r);
                        } else {
	                        mp_VActive->RCon()->LogString("RightClicked, but FAR PID is 0");
                        }
                        return 0;
                    } else {
                        char szLog[255];
                        // if ((gSet.isRClickSendKey==1) || (dwDelta>RCLICKAPPSTIMEOUT && dwDelta<10000))
                        lstrcpyA(szLog, "RightClicked, but condition failed: ");
                        if (gSet.isRClickSendKey!=1) {
                        	wsprintfA(szLog+lstrlenA(szLog), "((isRClickSendKey=%i)!=1)", (UINT)gSet.isRClickSendKey);
                    	} else {
                    		wsprintfA(szLog+lstrlenA(szLog), "(isRClickSendKey==%i)", (UINT)gSet.isRClickSendKey);
                    	}
                        if (dwDelta <= RCLICKAPPSTIMEOUT) {
                        	wsprintfA(szLog+lstrlenA(szLog), ", ((Delay=%i)<=%i)", dwDelta, (int)RCLICKAPPSTIMEOUT);
                        } else if (dwDelta >= 10000) {
                        	wsprintfA(szLog+lstrlenA(szLog), ", ((Delay=%i)>=10000)", dwDelta);
                        } else {
                        	wsprintfA(szLog+lstrlenA(szLog), ", (Delay==%i)", dwDelta);
                        }
                        mp_VActive->RCon()->LogString(szLog);
                    }
                } else {
                    char szLog[100];
                    wsprintfA(szLog, "RightClicked, but mouse was moved abs({%i-%i}-{%i-%i})>%i", (int)Rcursor.x, (int)Rcursor.y, (int)LOWORD(lParam), (int)HIWORD(lParam), (int)RCLICKAPPSDELTA);
                    mp_VActive->RCon()->LogString(szLog);
                }
                // Иначе нужно сначала послать WM_RBUTTONDOWN
                //POSTMESSAGE(ghConWnd, WM_RBUTTONDOWN, wParam, MAKELPARAM( newX, newY ), TRUE);
                mp_VActive->RCon()->OnMouse(WM_RBUTTONDOWN, wParam, ptCur.x, ptCur.y);
            } else {
                char szLog[255];
                // if (gSet.isRClickSendKey && (mouse.state & MOUSE_R_LOCKED))
                //wsprintfA(szLog, "RightClicked, but condition failed (RCSK:%i, State:%u)", (int)gSet.isRClickSendKey, (DWORD)mouse.state);
                lstrcpyA(szLog, "RightClicked, but condition failed: ");
                if (gSet.isRClickSendKey==0) {
                	wsprintfA(szLog+lstrlenA(szLog), "((isRClickSendKey=%i)==0)", (UINT)gSet.isRClickSendKey);
            	} else {
            		wsprintfA(szLog+lstrlenA(szLog), "(isRClickSendKey==%i)", (UINT)gSet.isRClickSendKey);
            	}
                if ((mouse.state & MOUSE_R_LOCKED) == 0) {
                	wsprintfA(szLog+lstrlenA(szLog), ", (((state=0x%X)&MOUSE_R_LOCKED)==0)", (DWORD)mouse.state);
                } else {
                	wsprintfA(szLog+lstrlenA(szLog), ", (state==0x%X)", (DWORD)mouse.state);
                }
                mp_VActive->RCon()->LogString(szLog);
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
    mp_VActive->RCon()->OnMouse(messg == WM_RBUTTONDBLCLK ? WM_RBUTTONDOWN : messg, wParam, ptCur.x, ptCur.y);
    return 0;
}

void CConEmuMain::SetWaitCursor(BOOL abWait)
{
	mb_WaitCursor = abWait;
	if (mb_WaitCursor)
		SetCursor(mh_CursorWait);
	else
		SetCursor(mh_CursorArrow);
}

LRESULT CConEmuMain::OnSetCursor(WPARAM wParam, LPARAM lParam)
{
	if (lParam == (LPARAM)-1) {
		RECT rcWnd; GetWindowRect(ghWndDC, &rcWnd);
		POINT ptCur; GetCursorPos(&ptCur);
		if (!PtInRect(&rcWnd, ptCur))
			return FALSE;
		wParam = (WPARAM)ghWnd;
		lParam = HTCLIENT;
	}

	if (((HWND)wParam) != ghWnd || (LOWORD(lParam) != HTCLIENT))
		return FALSE;
	
	HCURSOR hCur = NULL;

	DEBUGSTRSETCURSOR(L"WM_SETCURSOR");

	if (mb_WaitCursor) {
		hCur = mh_CursorWait;
		DEBUGSTRSETCURSOR(L" ---> CursorWait\n");
	} else if (gSet.isFarHourglass) {
		CRealConsole *pRCon = mp_VActive->RCon();
		if (pRCon) {
			BOOL lbAlive = pRCon->isAlive();
			
			if (!lbAlive) {
				hCur = mh_CursorAppStarting;
				DEBUGSTRSETCURSOR(L" ---> AppStarting\n");
			}
		}
	}
	if (!hCur) {
		hCur = mh_CursorArrow;
		DEBUGSTRSETCURSOR(L" ---> Arrow\n");
	}

	SetCursor(hCur);

	return TRUE;
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
            //HWND hCon = (HWND)lParam;
            //if (!hCon) return 0;
            //for (int i = 0; i<MAX_CONSOLE_COUNT; i++) {
            //    if (!mp_VCon[i]) continue;
            //    if (mp_VCon[i]->RCon()->ConWnd() == hCon) {
            //        FLASHWINFO fl = {sizeof(FLASHWINFO)};
            //        if (isMeForeground()) {
            //        	if (mp_VCon[i] != mp_VActive) { // Только для неактивной консоли
            //                fl.dwFlags = FLASHW_STOP; fl.hwnd = ghWnd;
            //                FlashWindowEx(&fl); // Чтобы мигание не накапливалось
            //        		fl.uCount = 4; fl.dwFlags = FLASHW_ALL; fl.hwnd = ghWnd;
            //        		FlashWindowEx(&fl);
            //        	}
            //        } else {
            //        	fl.dwFlags = FLASHW_ALL|FLASHW_TIMERNOFG; fl.hwnd = ghWnd;
            //        	FlashWindowEx(&fl); // Помигать в GUI
            //        }
            //        
            //        fl.dwFlags = FLASHW_STOP; fl.hwnd = hCon;
            //        FlashWindowEx(&fl);
            //        break;
            //    }
            //}
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
	#ifdef _DEBUG
	wchar_t szDbg[128]; wsprintf(szDbg, L"OnSysCommand (%i(0x%X), %i)\n", wParam, wParam, lParam);
	DEBUGSTRSIZE(szDbg);
	#endif

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
        mp_VActive->RCon()->Paste();
        return 0;
        //break;
    case ID_AUTOSCROLL:
        gSet.AutoScroll = !gSet.AutoScroll;
        CheckMenuItem(GetSystemMenu(ghWnd, FALSE), ID_AUTOSCROLL, MF_BYCOMMAND |
            (gSet.AutoScroll ? MF_CHECKED : MF_UNCHECKED));
        return 0;
        //break;
    case ID_DUMPCONSOLE:
        if (mp_VActive)
            mp_VActive->DumpConsole();
        return 0;
        //break;
    case ID_DEBUGGUI:
    	StartDebugLogConsole();
    	return 0;
    case ID_CON_TOGGLE_VISIBLE:
        if (mp_VActive)
            mp_VActive->RCon()->ShowConsole(-1); // Toggle visibility
        return 0;
    case ID_HELP:
    	{
    		static HMODULE hhctrl = NULL;
    		if (!hhctrl) hhctrl = GetModuleHandle(L"hhctrl.ocx");
    		if (!hhctrl) hhctrl = LoadLibrary(L"hhctrl.ocx");
    		if (hhctrl) {
    			typedef BOOL (WINAPI* HTMLHelpW_t)(HWND hWnd, LPCWSTR pszFile, INT uCommand, INT dwData);
    			HTMLHelpW_t fHTMLHelpW = (HTMLHelpW_t)GetProcAddress(hhctrl, "HtmlHelpW");
    			if (fHTMLHelpW) {
    				wchar_t szHelpFile[MAX_PATH*2];
    				lstrcpy(szHelpFile, ms_ConEmuChm);
    				//wchar_t* pszSlash = wcsrchr(szHelpFile, L'\\');
    				//if (pszSlash) pszSlash++; else pszSlash = szHelpFile;
    				//lstrcpy(pszSlash, L"ConEmu.chm");
    				// lstrcat(szHelpFile, L::/Intro.htm");
    				
    				#define HH_HELP_CONTEXT 0x000F
    				#define HH_DISPLAY_TOC  0x0001
    				//fHTMLHelpW(NULL /*чтобы окно не блокировалось*/, szHelpFile, HH_HELP_CONTEXT, contextID);
    				fHTMLHelpW(NULL /*чтобы окно не блокировалось*/, szHelpFile, HH_DISPLAY_TOC, 0);
    			}
			}
    	}
    	return 0;
    case ID_ABOUT:
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
			int nEditors = 0, nProgress = 0, i;
			for (i=(MAX_CONSOLE_COUNT-1); i>=0; i--) {
				CRealConsole* pRCon = NULL;
				ConEmuTab tab = {0};
				if (mp_VCon[i] && (pRCon = mp_VCon[i]->RCon())!=NULL) {
					if (pRCon->GetProgress(NULL) != -1)
						nProgress ++;
					for (int j = 0; TRUE; j++) {
						if (!pRCon->GetTab(j, &tab))
							break;
						if (tab.Modified)
							nEditors ++;
					}
				}
			}
			if (nProgress || nEditors) {
				wchar_t szText[255], *pszText;
				lstrcpy(szText, L"Close confirmation.\r\n\r\n"); pszText = szText+lstrlen(szText);
				if (nProgress) { wsprintf(pszText, L"Incomplete operations: %i\r\n", nProgress); pszText += lstrlen(pszText); }
				if (nEditors) { wsprintf(pszText, L"Unsaved editor windows: %i\r\n", nEditors); pszText += lstrlen(pszText); }
				lstrcpy(pszText, L"\r\nProceed with shutdown?");
				int nBtn = MessageBoxW(ghWnd, szText, L"ConEmu", MB_OKCANCEL|MB_ICONEXCLAMATION);
				if (nBtn != IDOK)
					return 0; // не закрывать
			}

            for (i=(MAX_CONSOLE_COUNT-1); i>=0; i--) {
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
		DEBUGSTRSYS(L"OnSysCommand(SC_MAXIMIZE)\n");
        if (!mb_PassSysCommand) {
        	#ifndef _DEBUG
            if (isPictureView())
                break;
            #endif
            SetWindowMode(rMaximized);
        } else {
            result = DefWindowProc(hWnd, WM_SYSCOMMAND, wParam, lParam);
        }
        break;
    case SC_RESTORE:
		DEBUGSTRSYS(L"OnSysCommand(SC_RESTORE)\n");
        if (!mb_PassSysCommand) {
        	#ifndef _DEBUG
            if (!isIconic() && isPictureView())
                break;
            #endif
            if (!SetWindowMode(isIconic() ? WindowMode : rNormal))
                result = DefWindowProc(hWnd, WM_SYSCOMMAND, wParam, lParam);
        } else {
            result = DefWindowProc(hWnd, WM_SYSCOMMAND, wParam, lParam);
        }
        break;
    case SC_MINIMIZE:
		DEBUGSTRSYS(L"OnSysCommand(SC_MINIMIZE)\n");
        if (gSet.isMinToTray) {
            Icon.HideWindowToTray();
            break;
        }
        result = DefWindowProc(hWnd, WM_SYSCOMMAND, wParam, lParam);
        break;

    default:
        if (wParam != 0xF100)
        {
			#ifdef _DEBUG
			wchar_t szDbg[64]; wsprintf(szDbg, L"OnSysCommand(%i)\n", wParam);
			DEBUGSTRSYS(szDbg);
			#endif
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
	    {
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


	        if (!isIconic())
	        {
	            //// Было ли реальное изменение размеров?
	            //BOOL lbSizingToDo  = (mouse.state & MOUSE_SIZING_TODO) == MOUSE_SIZING_TODO;

	            //if (isSizing() && !isPressed(VK_LBUTTON)) {
	            //    // Сборс всех флагов ресайза мышкой
	            //    mouse.state &= ~(MOUSE_SIZING_BEGIN|MOUSE_SIZING_TODO);
	            //}

	            //TODO("возможно весь ресайз (кроме SyncNtvdm?) нужно перенести в нить консоли")
				//OnConsoleResize();

	            // update scrollbar
	            mp_VActive->RCon()->UpdateScrollInfo();
	        }

			if (mh_ConEmuAliveEvent && !mb_ConEmuAliveOwned)
				isFirstInstance(); // Заодно и проверит...

	    } break; // case 0:
	case 1:
		{
	        if (!isIconic())
	        {
	        	m_Child.CheckPostRedraw();
	        }
		} break; // case 1:
    }

    mb_InTimer = FALSE;

    return result;
}

void CConEmuMain::OnTransparent()
{
	UINT nTransparent = max(MIN_ALPHA_VALUE,gSet.nTransparent);
	DWORD dwExStyle = GetWindowLongPtr(ghWnd, GWL_EXSTYLE);
	if (nTransparent == 255) {
		// Прозрачность отключается (полностью непрозрачный)
		//SetLayeredWindowAttributes(ghWnd, 0, 255, LWA_ALPHA);
		if ((dwExStyle & WS_EX_LAYERED) == WS_EX_LAYERED) {
			dwExStyle &= ~WS_EX_LAYERED;
			SetLayeredWindowAttributes(ghWnd, 0, 255, LWA_ALPHA);
			SetWindowLongPtr(ghWnd, GWL_EXSTYLE, dwExStyle);
		}
	} else {
		if ((dwExStyle & WS_EX_LAYERED) == 0) {
			dwExStyle |= WS_EX_LAYERED;
			SetWindowLongPtr(ghWnd, GWL_EXSTYLE, dwExStyle);
		}
		SetLayeredWindowAttributes(ghWnd, 0, nTransparent, LWA_ALPHA);
	}
}

// Вызовем UpdateScrollPos для АКТИВНОЙ консоли
LRESULT CConEmuMain::OnUpdateScrollInfo(BOOL abPosted/* = FALSE*/)
{
    if (!abPosted) {
        PostMessage(ghWnd, mn_MsgUpdateScrollInfo, 0, (LPARAM)mp_VCon);
        return 0;
    }

    if (!mp_VActive)
        return 0;
    mp_VActive->RCon()->UpdateScrollInfo();
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

			// Сначала нужно обновить закладки, иначе в закрываемой консоли
			// может быть несколько вкладок и вместо активации другой консоли
			// будет попытка активировать другую вкладку закрываемой консоли
			//gConEmu.mp_TabBar->Update(TRUE); -- а и не сможет он другую активировать, т.к. RCon вернет FALSE

			// Эта комбинация должна активировать предыдущую консоль (если активна текущая)
			if (gSet.isTabRecent && apVCon == mp_VActive) {
				if (gConEmu.GetVCon(1)) {
					gConEmu.mp_TabBar->SwitchRollback();
					gConEmu.mp_TabBar->SwitchNext();
					gConEmu.mp_TabBar->SwitchCommit();
				}
			}

			// Теперь можно очистить переменную массива
			mp_VCon[i] = NULL;


            WARNING("Вообще-то это нужно бы в CriticalSection закрыть. Несколько консолей может одновременно закрыться");
            if (mp_VActive == apVCon) {
                for (int j=(i-1); j>=0; j--) {
                    if (mp_VCon[j]) {
                        ConActivate(j);
                        break;
                    }
                }
                if (mp_VActive == apVCon) {
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
            if (mp_VActive == apVCon)
                mp_VActive = NULL;
            delete apVCon;
            break;
        }
    }
    // Теперь перетряхнуть заголовок (табы могут быть отключены и в заголовке отображается количество консолей)
    UpdateTitle(NULL); // сам перечитает
    //
    gConEmu.mp_TabBar->Update(); // Иначе не будет обновлены закладки
	// А теперь можно обновить активную закладку
    gConEmu.mp_TabBar->OnConsoleActivated(ActiveConNum()+1/*, FALSE*/);
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

	MCHKHEAP;

    _ASSERTE(ghWnd!=NULL);
    wsprintf(szServerPipe, CEGUIPIPENAME, L".", (DWORD)ghWnd);

    // The main loop creates an instance of the named pipe and 
    // then waits for a client to connect to it. When the client 
    // connects, a thread is created to handle communications 
    // with that client, and the loop is repeated.
    
	MCHKHEAP;

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

			MCHKHEAP;

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
        gConEmu.ShowOldCmdVersion(in.hdr.nCmd, in.hdr.nVersion, -1);
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

        if (isIconic())
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
		if (gConEmu.mp_TabBar)
			result = gConEmu.mp_TabBar->OnNotify((LPNMHDR)lParam);
        break;
    }

    case WM_COMMAND:
    {
		if (gConEmu.mp_TabBar)
			gConEmu.mp_TabBar->OnCommand(wParam, lParam);
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
		{
			RECT* pRc = (RECT*)lParam;
			wchar_t szDbg[128]; wsprintf(szDbg, L"WM_SIZING (Edge%i, {%i-%i}-{%i-%i})\n", wParam, pRc->left, pRc->top, pRc->right, pRc->bottom);
			DEBUGSTRSIZE(szDbg);

			if (!isIconic())
				result = gConEmu.OnSizing(wParam, lParam);
		} break;

#ifdef _DEBUG
	case WM_WINDOWPOSCHANGING:
		{
			WINDOWPOS *p = (WINDOWPOS*)lParam;
			wchar_t szDbg[128]; wsprintf(szDbg, L"WM_WINDOWPOSCHANGING ({%i-%i}x{%i-%i} Flags=0x%08X)\n", p->x, p->y, p->cx, p->cy, p->flags);
			DEBUGSTRSIZE(szDbg);
			// Иначе могут не вызваться события WM_SIZE/WM_MOVE
			result = DefWindowProc(hWnd, messg, wParam, lParam);
		} break;

	//case WM_WINDOWPOSCHANGED:
	//	{
	//		WINDOWPOS *p = (WINDOWPOS*)lParam;
	//		wchar_t szDbg[128]; wsprintf(szDbg, L"WM_WINDOWPOSCHANGED ({%i-%i}x{%i-%i} Flags=0x%08X)\n", p->x, p->y, p->cx, p->cy, p->flags);
	//		DEBUGSTRSIZE(szDbg);
	//		result = 0; //DefWindowProc(hWnd, messg, wParam, lParam);
	//	} break;

	case WM_SHOWWINDOW:
		{
			wchar_t szDbg[128]; wsprintf(szDbg, L"WM_SHOWWINDOW (Show=%i, Status=%i)\n", wParam, lParam);
			DEBUGSTRSIZE(szDbg);
			result = DefWindowProc(hWnd, messg, wParam, lParam);
		} break;
#endif

    case WM_SIZE:
		{
			#ifdef _DEBUG
			wchar_t szDbg[128]; wsprintf(szDbg, L"WM_SIZE (Type:%i, {%i-%i})\n", wParam, LOWORD(lParam), HIWORD(lParam));
			DEBUGSTRSIZE(szDbg);
			#endif
			if (!isIconic())
				result = gConEmu.OnSize(wParam, LOWORD(lParam), HIWORD(lParam));
		} break;

	// WM_MOVE не зовется, возможно из-за WM_WINDOWPOSCHANGED
    case WM_MOVE:
		{
			#ifdef _DEBUG
			wchar_t szDbg[128]; wsprintf(szDbg, L"WM_MOVE ({%i-%i})\n", LOWORD(lParam), HIWORD(lParam));
			DEBUGSTRSIZE(szDbg);
			#endif
		} break;

	case WM_WINDOWPOSCHANGED:
		{
			WINDOWPOS *p = (WINDOWPOS*)lParam;
			wchar_t szDbg[128]; wsprintf(szDbg, L"WM_WINDOWPOSCHANGED ({%i-%i}x{%i-%i} Flags=0x%08X)\n", p->x, p->y, p->cx, p->cy, p->flags);
			DEBUGSTRSIZE(szDbg);
			// Иначе могут не вызваться события WM_SIZE/WM_MOVE
			result = DefWindowProc(hWnd, messg, wParam, lParam);

			if (hWnd == ghWnd /*&& ghOpWnd*/) { //2009-05-08 запоминать wndX/wndY всегда, а не только если окно настроек открыто
				if (!gSet.isFullScreen && !isZoomed() && !isIconic())
				{
					RECT rc; GetWindowRect(ghWnd, &rc);
					gSet.UpdatePos(rc.left, rc.top);
					if (hPictureView)
						mrc_WndPosOnPicView = rc;
				}
			} else if (hPictureView) {
				GetWindowRect(ghWnd, &mrc_WndPosOnPicView);
			}
		} break;

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
		if (messg == WM_SYSKEYUP || messg == WM_SYSKEYDOWN)
			result = TRUE;
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

	case WM_SETCURSOR:
		result = gConEmu.OnSetCursor(wParam, lParam);
		if (!result)
			result = DefWindowProc(hWnd, messg, wParam, lParam);
		MCHKHEAP;
		// If an application processes this message, it should return TRUE to halt further processing or FALSE to continue.
		return result;

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
            gConEmu.UpdateTitle(mp_VActive->RCon()->GetTitle());
            return 0;
        } else if (messg == gConEmu.mn_MsgAttach) {
            return gConEmu.AttachRequested ( (HWND)wParam, (DWORD)lParam );
		} else if (messg == gConEmu.mn_MsgSrvStarted) {
			gConEmu.WinEventProc(NULL, EVENT_CONSOLE_START_APPLICATION, (HWND)wParam, lParam, 0, 0, 0);
			return 0;
        } else if (messg == gConEmu.mn_MsgVConTerminated) {

			#ifdef _DEBUG
				wchar_t szDbg[200];
				lstrcpy(szDbg, L"OnVConTerminated");
				CVirtualConsole* pCon = (CVirtualConsole*)lParam;
				for (int i = 0; pCon && i < MAX_CONSOLE_COUNT; i++) {
					if (pCon == mp_VCon[i]) {
						ConEmuTab tab = {0};
						pCon->RCon()->GetTab(0, &tab);
						tab.Name[128] = 0; // чтобы не вылезло из szDbg
						wsprintf(szDbg+lstrlen(szDbg), L": #%i: %s", i+1, tab.Name);
						break;						
					}
				}
				lstrcat(szDbg, L"\n");
				DEBUGSTRCONS(szDbg);
			#endif

            return gConEmu.OnVConTerminated ( (CVirtualConsole*)lParam, TRUE );
        } else if (messg == gConEmu.mn_MsgUpdateScrollInfo) {
            return OnUpdateScrollInfo(TRUE);
        } else if (messg == gConEmu.mn_MsgUpdateTabs) {
			DEBUGSTRTABS(L"OnUpdateTabs\n");
            gConEmu.mp_TabBar->Update(TRUE);
            return 0;
        } else if (messg == gConEmu.mn_MsgOldCmdVer) {
            gConEmu.ShowOldCmdVersion(wParam & 0xFFFF, lParam, (SHORT)((wParam & 0xFFFF0000)>>16));
            return 0;
        } else if (messg == gConEmu.mn_MsgTabCommand) {
            gConEmu.TabCommand(wParam);
            return 0;
        } else if (messg == gConEmu.mn_MsgSheelHook) {
            gConEmu.OnShellHook(wParam, lParam);
            return 0;
		} else if (messg == gConEmu.mn_ShellExecuteEx) {
			return gConEmu.GuiShellExecuteEx((SHELLEXECUTEINFO*)lParam, wParam);
		} else if (messg == gConEmu.mn_PostConsoleResize) {
			gConEmu.OnConsoleResize(TRUE);
			return 0;
		} else if (messg == gConEmu.mn_ConsoleLangChanged) {
			gConEmu.OnLangChangeConsole((CVirtualConsole*)lParam, (DWORD)wParam);
			return 0;
		} else if (messg == gConEmu.mn_MsgPostOnBufferHeight) {
			gConEmu.OnBufferHeight();
			return 0;
		//} else if (messg == gConEmu.mn_MsgSetForeground) {
		//	SetForegroundWindow((HWND)lParam);
		//	return 0;
		} else if (messg == gConEmu.mn_MsgFlashWindow) {
			return OnFlashWindow((wParam & 0xFF000000) >> 24, wParam & 0xFFFFFF, (HWND)lParam);
		}
        //else if (messg == gConEmu.mn_MsgCmdStarted || messg == gConEmu.mn_MsgCmdStopped) {
        //  return gConEmu.OnConEmuCmd( (messg == gConEmu.mn_MsgCmdStarted), (HWND)wParam, (DWORD)lParam);
        //}

        if (messg) result = DefWindowProc(hWnd, messg, wParam, lParam);
    }
    return result;
}
