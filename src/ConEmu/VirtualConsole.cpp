
/*
Copyright (c) 2009-2010 Maximus5
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
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


#define SHOWDEBUGSTR

#include "Header.h"
#include <Tlhelp32.h>
#include "ScreenDump.h"
#include "../common/farcolor.hpp"

#ifdef _DEBUG
	//#define DEBUGDRAW_RCONPOS VK_SCROLL // -- при включенном ScrollLock отрисовать прямоугольник, соответствующий положению окна RealConsole
	#define DEBUGDRAW_DIALOGS VK_CAPITAL // -- при включенном Caps отрисовать прямоугольники, соответствующие найденным диалогам
	#define DEBUGDRAW_VERTICALS VK_SCROLL // -- при включенном ScrollLock отрисовать прямоугольники, соответсвующие граням диалогов (которые строго выравниваются по ячейкам)
#endif

#define DEBUGSTRDRAW(s) //DEBUGSTR(s)
#define DEBUGSTRCOORD(s) //DEBUGSTR(s)

// WARNING("не появляются табы во второй консоли");
WARNING("На предыдущей строке символы под курсором прыгают влево");
WARNING("При быстром наборе текста курсор часто 'замерзает' на одной из букв, но продолжает двигаться дальше");

WARNING("Глюк с частичной отрисовкой экрана часто появляется при Alt-F7, Enter. Особенно, если файла нет");
// Иногда не отрисовывается диалог поиска полностью - только бежит текущая сканируемая директория.
// Иногда диалог отрисовался, но часть до текста "..." отсутствует

//PRAGMA_ERROR("При попытке компиляции F:\\VCProject\\FarPlugin\\PPCReg\\compile.cmd - Enter в консоль не прошел");

WARNING("Может быть хватит ServerThread? А StartProcessThread зарезервировать только для запуска процесса, и сразу из него выйти");

WARNING("В каждой VCon создать буфер BYTE[256] для хранения распознанных клавиш (Ctrl,...,Up,PgDn,Add,и пр.");
WARNING("Нераспознанные можно помещать в буфер {VKEY,wchar_t=0}, в котором заполнять последний wchar_t по WM_CHAR/WM_SYSCHAR");
WARNING("При WM_(SYS)CHAR помещать wchar_t в начало, в первый незанятый VKEY");
WARNING("При нераспознном WM_KEYUP - брать(и убрать) wchar_t из этого буфера, послав в консоль UP");
TODO("А периодически - проводить процерку isKeyDown, и чистить буфер");
WARNING("при переключении на другую консоль (да наверное и в процессе просто нажатий - модификатор может быть изменен в самой программе) требуется проверка caps, scroll, num");
WARNING("а перед пересылкой символа/клавиши проверять нажат ли на клавиатуре Ctrl/Shift/Alt");

WARNING("Часто после разблокирования компьютера размер консоли изменяется (OK), но обновленное содержимое консоли не приходит в GUI - там остется обрезанная верхняя и нижняя строка");


//Курсор, его положение, размер консоли, измененный текст, и пр...

#define VCURSORWIDTH 2
#define HCURSORHEIGHT 2

#define Assert(V) if ((V)==FALSE) { TCHAR szAMsg[MAX_PATH*2]; wsprintf(szAMsg, _T("Assertion (%s) at\n%s:%i"), _T(#V), _T(__FILE__), __LINE__); Box(szAMsg); }

#ifdef _DEBUG
//#undef HEAPVAL
#define HEAPVAL //HeapValidate(mh_Heap, 0, NULL);
#define CURSOR_ALWAYS_VISIBLE
#else
#define HEAPVAL
#endif

#define ISBGIMGCOLOR(a) (gSet.nBgImageColors & (1 << a))

#ifndef CONSOLE_SELECTION_NOT_EMPTY
#define CONSOLE_SELECTION_NOT_EMPTY     0x0002   // non-null select rectangle
#endif

#ifdef _DEBUG
#define DUMPDC(f) if (mb_DebugDumpDC) DumpImage(hDC, Width, Height, f);
#else
#define DUMPDC(f)
#endif

#define isCharSpace(c) (c == ucSpace || c == ucNoBreakSpace || c == 0)



CVirtualConsole::PARTBRUSHES CVirtualConsole::m_PartBrushes[MAX_COUNT_PART_BRUSHES] = {{0}};
char CVirtualConsole::mc_Uni2Oem[0x10000];
// MAX_SPACES == 0x400
wchar_t CVirtualConsole::ms_Spaces[MAX_SPACES];
wchar_t CVirtualConsole::ms_HorzDbl[MAX_SPACES];
wchar_t CVirtualConsole::ms_HorzSingl[MAX_SPACES];
HMENU CVirtualConsole::mh_PopupMenu = NULL;

CVirtualConsole* CVirtualConsole::CreateVCon(RConStartArgs *args)
{
    CVirtualConsole* pCon = new CVirtualConsole();
    
    if (!pCon->mp_RCon->PreCreate(args)) {
        delete pCon;
        return NULL;
    }

    return pCon;
}

CVirtualConsole::CVirtualConsole(/*HANDLE hConsoleOutput*/)
{
    mp_RCon = new CRealConsole(this);

	gConEmu.OnVConCreated(this);

	WARNING("скорректировать размер кучи");
    SIZE_T nMinHeapSize = (1000 + (200 * 90 * 2) * 6 + MAX_PATH*2)*2 + 210*sizeof(*TextParts);
    mh_Heap = HeapCreate(HEAP_GENERATE_EXCEPTIONS, nMinHeapSize, 0);
   
    cinf.dwSize = 15; cinf.bVisible = TRUE;
    ZeroStruct(winSize); ZeroStruct(coord);
    TextLen = 0;
	mb_RequiredForceUpdate = true;
	mb_LastFadeFlag = false;

	_ASSERTE(sizeof(mh_FontByIndex) == sizeof(gSet.mh_Font));
	memmove(mh_FontByIndex, gSet.mh_Font, sizeof(mh_FontByIndex));

	memset(&TransparentInfo, 0, sizeof(TransparentInfo));

	isFade = false; isForeground = true;
	mp_Colors = gSet.GetColors();
	

	memset(&m_LeftPanelView, 0, sizeof(m_LeftPanelView));
	memset(&m_RightPanelView, 0, sizeof(m_RightPanelView));
	mn_ConEmuFadeMsg = mn_ConEmuSettingsMsg = 0;
	// Эти переменные устанавливаются в TRUE, если при следующем Redraw нужно обновить размер панелей
	mb_LeftPanelRedraw = mb_RightPanelRedraw = FALSE;
	mn_LastDialogsCount = 0;
	memset(mrc_LastDialogs, 0, sizeof(mrc_LastDialogs));
  
    //InitializeCriticalSection(&csDC); ncsTDC = 0; 
	mb_PaintRequested = FALSE; mb_PaintLocked = FALSE;
    //InitializeCriticalSection(&csCON); ncsTCON = 0;

	mb_InPaintCall = FALSE;

	nFontHeight = gSet.FontHeight();
	nFontWidth = gSet.FontWidth();
	nFontCharSet = gSet.FontCharSet();
	nLastNormalBack = 255;

	mb_ConDataChanged = FALSE;
	mh_TransparentRgn = NULL;

#ifdef _DEBUG
    mn_BackColorIdx = 2;
#else
    mn_BackColorIdx = 0;
#endif
    memset(&Cursor, 0, sizeof(Cursor));
    Cursor.nBlinkTime = GetCaretBlinkTime();

    TextWidth = TextHeight = Width = Height = nMaxTextWidth = nMaxTextHeight = 0;
    hDC = NULL; hBitmap = NULL;
    hSelectedFont = NULL; hOldFont = NULL;
    PointersInit();
    mb_IsForceUpdate = false;
    hBrush0 = NULL; hSelectedBrush = NULL; hOldBrush = NULL;
    isEditor = false;
    memset(&csbi, 0, sizeof(csbi)); mdw_LastError = 0;

    //nSpaceCount = 1000;
    //Spaces = (TCHAR*)Alloc(nSpaceCount,sizeof(TCHAR));
    //for (UINT i=0; i<nSpaceCount; i++) Spaces[i]=L' ';
    if (!ms_Spaces[0]) {
    	wmemset(ms_Spaces, L' ', MAX_SPACES);
    	wmemset(ms_HorzDbl, ucBoxDblHorz, MAX_SPACES);
    	wmemset(ms_HorzSingl, ucBoxSinglHorz, MAX_SPACES);
	}


    hOldBrush = NULL;
    hOldFont = NULL;
    
    if (gSet.wndWidth)
        TextWidth = gSet.wndWidth;
    if (gSet.wndHeight)
        TextHeight = gSet.wndHeight;
    

	#ifdef _DEBUG
	mb_DebugDumpDC = FALSE;
	#endif

    if (gSet.isShowBgImage)
        gSet.LoadImageFrom(gSet.sBgImage);

    if (gSet.isAdvLogging != 3) {
        mpsz_LogScreen = NULL;
    } else {
        mn_LogScreenIdx = 0;
        //DWORD dwErr = 0;
        wchar_t szFile[MAX_PATH+64], *pszDot;
		lstrcpyW(szFile, gConEmu.ms_ConEmuExe);
        if ((pszDot = wcsrchr(szFile, L'\\')) == NULL) {
            DisplayLastError(L"wcsrchr failed!");
            return; // ошибка
        }
        *pszDot = 0;

        mpsz_LogScreen = (wchar_t*)calloc(pszDot - szFile + 64, 2);
        wcscpy(mpsz_LogScreen, szFile);
        wcscpy(mpsz_LogScreen+wcslen(mpsz_LogScreen), L"\\ConEmu-VCon-%i-%i.con"/*, RCon()->GetServerPID()*/);
    }

    // InitDC звать бессмысленно - консоль еще не создана
}

CVirtualConsole::~CVirtualConsole()
{
    if (mp_RCon)
        mp_RCon->StopThread();

    HEAPVAL;
    if (hDC)
        { DeleteDC(hDC); hDC = NULL; }
    if (hBitmap)
        { DeleteObject(hBitmap); hBitmap = NULL; }
        
    PointersFree();

    // Куча больше не нужна
    if (mh_Heap) {
        HeapDestroy(mh_Heap);
        mh_Heap = NULL;
    }

    if (mpsz_LogScreen) {
        //wchar_t szMask[MAX_PATH*2]; wcscpy(szMask, mpsz_LogScreen);
        //wchar_t *psz = wcsrchr(szMask, L'%');
        //if (psz) {
        //    wcscpy(psz, L"*.*");
        //    psz = wcsrchr(szMask, L'\\'); 
        //    if (psz) {
        //        psz++;
        //        WIN32_FIND_DATA fnd;
        //        HANDLE hFind = FindFirstFile(szMask, &fnd);
        //        if (hFind != INVALID_HANDLE_VALUE) {
        //            do {
        //                if ((fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0) {
        //                    wcscpy(psz, fnd.cFileName);
        //                    DeleteFile(szMask);
        //                }
        //            } while (FindNextFile(hFind, &fnd));
        //            FindClose(hFind);
        //        }
        //    }
        //}
        free(mpsz_LogScreen); mpsz_LogScreen = NULL;
    }
    
    //DeleteCriticalSection(&csDC);
    //DeleteCriticalSection(&csCON);

	if (mh_TransparentRgn) {
		DeleteObject(mh_TransparentRgn);
		mh_TransparentRgn = NULL;
	}

    if (mp_RCon) {
        delete mp_RCon;
        mp_RCon = NULL;
    }
    
    //if (mh_PopupMenu) { -- static на все экземпляры
    //	DestroyMenu(mh_PopupMenu); mh_PopupMenu = NULL;
    //}
}

HWND CVirtualConsole::GetView()
{
	TODO("Доработать для DoubleView");
	HWND hView = ghWndDC;
	return hView;
}

void CVirtualConsole::PointersInit()
{
	mb_PointersAllocated = false;
    mpsz_ConChar = mpsz_ConCharSave = NULL;
    mpn_ConAttrEx = mpn_ConAttrExSave = NULL;
    ConCharX = ConCharDX = NULL; 
    tmpOem = NULL; 
    TextParts = NULL;
    BgParts = NULL;
    PolyText = NULL;
	pbLineChanged = pbBackIsPic = NULL;
	pnBackRGB = NULL;
}

void CVirtualConsole::PointersFree()
{
	#ifdef SafeFree
	#undef SafeFree
	#endif
	#define SafeFree(f) if (f) { Free(f); f = NULL; }
	HEAPVAL;
	
    SafeFree(mpsz_ConChar);
    SafeFree(mpsz_ConCharSave);
    SafeFree(mpn_ConAttrEx);
    SafeFree(mpn_ConAttrExSave);
    SafeFree(ConCharX);
    SafeFree(ConCharDX);
    SafeFree(tmpOem);
    SafeFree(TextParts);
    SafeFree(BgParts);
    SafeFree(PolyText);
	SafeFree(pbLineChanged);
	SafeFree(pbBackIsPic);
	SafeFree(pnBackRGB);
	
    HEAPVAL;
    mb_PointersAllocated = false;
}

bool CVirtualConsole::PointersAlloc()
{
	mb_PointersAllocated = false;
	
    #ifdef _DEBUG
    HeapValidate(mh_Heap, 0, NULL);
    #endif
    
    uint nWidthHeight = (nMaxTextWidth * nMaxTextHeight);
    
    #ifdef AllocArray
    #undef AllocArray
    #endif
    #define AllocArray(p,t,c) p = (t*)Alloc(c,sizeof(t)); if (!p) return false;
	
    AllocArray(mpsz_ConChar, TCHAR, nWidthHeight);
    
    AllocArray(mpsz_ConCharSave, TCHAR, nWidthHeight);
    
    AllocArray(mpn_ConAttrEx, CharAttr, nWidthHeight);
    
    AllocArray(mpn_ConAttrExSave, CharAttr, nWidthHeight);
    
    AllocArray(ConCharX, DWORD, nWidthHeight);
    
    AllocArray(ConCharDX, DWORD, nMaxTextWidth);
    
    AllocArray(tmpOem, char, nMaxTextWidth);
    
    AllocArray(TextParts, TEXTPARTS, (nMaxTextWidth + 1));
    
    AllocArray(BgParts, BGPARTS, nMaxTextWidth);
    
    AllocArray(PolyText, POLYTEXT, nMaxTextWidth);
    
	AllocArray(pbLineChanged, bool, nMaxTextHeight);
	AllocArray(pbBackIsPic, bool, nMaxTextHeight);
	AllocArray(pnBackRGB, COLORREF, nMaxTextHeight);
    
    HEAPVAL;
    return (mb_PointersAllocated = true);
}

void CVirtualConsole::PointersZero()
{
	uint nWidthHeight = (nMaxTextWidth * nMaxTextHeight);
	
    HEAPVAL;
    ZeroMemory(mpsz_ConChar, nWidthHeight*sizeof(*mpsz_ConChar));
    ZeroMemory(mpsz_ConCharSave, nWidthHeight*sizeof(*mpsz_ConChar));
    HEAPVAL;
    ZeroMemory(mpn_ConAttrEx, nWidthHeight*sizeof(*mpn_ConAttrEx));
    ZeroMemory(mpn_ConAttrExSave, nWidthHeight*sizeof(*mpn_ConAttrExSave));
    HEAPVAL;
    ZeroMemory(ConCharX, nWidthHeight*sizeof(*ConCharX));
    ZeroMemory(ConCharDX, nMaxTextWidth*sizeof(*ConCharDX));
    HEAPVAL;
    ZeroMemory(tmpOem, nMaxTextWidth*sizeof(*tmpOem));
    HEAPVAL;
    ZeroMemory(TextParts, (nMaxTextWidth + 1)*sizeof(*TextParts));
    ZeroMemory(BgParts, nMaxTextWidth*sizeof(*BgParts));
    ZeroMemory(PolyText, nMaxTextWidth*sizeof(*PolyText));
    HEAPVAL;
	ZeroMemory(pbLineChanged, nMaxTextHeight*sizeof(*pbLineChanged));
	ZeroMemory(pbBackIsPic, nMaxTextHeight*sizeof(*pbBackIsPic));
	ZeroMemory(pnBackRGB, nMaxTextHeight*sizeof(*pnBackRGB));
    HEAPVAL;
}


// InitDC вызывается только при критических изменениях (размеры, шрифт, и т.п.) когда нужно пересоздать DC и Bitmap
bool CVirtualConsole::InitDC(bool abNoDc, bool abNoWndResize)
{
    MSectionLock SCON; SCON.Lock(&csCON);
    BOOL lbNeedCreateBuffers = FALSE;
    
    uint rTextWidth = mp_RCon->TextWidth();
    uint rTextHeight = mp_RCon->TextHeight();
    if (!rTextWidth || !rTextHeight) {
        WARNING("Если тут ошибка - будет просто DC Initialization failed, что не понятно...");
        Assert(mp_RCon->TextWidth() && mp_RCon->TextHeight());
        return false;
    }


#ifdef _DEBUG
    if (mp_RCon->IsConsoleThread()) {
        //_ASSERTE(!mp_RCon->IsConsoleThread());
    }
    if (TextHeight == 24)
        TextHeight = 24;
    _ASSERT(TextHeight >= 5);
#endif



    // Буфер пересоздаем только если требуется его увеличение
    if (!mb_PointersAllocated ||
        (nMaxTextWidth * nMaxTextHeight) < (rTextWidth * rTextHeight) ||
        (nMaxTextWidth < rTextWidth) // а это нужно для TextParts & tmpOem
        )
        lbNeedCreateBuffers = TRUE;


    if (lbNeedCreateBuffers && mb_PointersAllocated) {
		DEBUGSTRDRAW(L"Relocking SCON exclusively\n");
        SCON.RelockExclusive();
		DEBUGSTRDRAW(L"Relocking SCON exclusively (done)\n");

        PointersFree();
    }


    // InitDC вызывается только при первой инициализации или смене размера
    mb_IsForceUpdate = true; // поэтому выставляем флажок Force
    
    TextWidth = rTextWidth;
    TextHeight = rTextHeight;



    if (lbNeedCreateBuffers) {
    	// Если увеличиваем размер - то с запасом, чтобы "два раза не ходить"...
        if (nMaxTextWidth < TextWidth)
            nMaxTextWidth = TextWidth+80;
        if (nMaxTextHeight < TextHeight)
            nMaxTextHeight = TextHeight+30;

		DEBUGSTRDRAW(L"Relocking SCON exclusively\n");
        SCON.RelockExclusive();
		DEBUGSTRDRAW(L"Relocking SCON exclusively (done)\n");
		
        if (!PointersAlloc()) {
	        WARNING("Если тут ошибка - будет просто DC Initialization failed, что не понятно...");
	        return false;
        }
    } else {
    	PointersZero();
    }

    SCON.Unlock();
    

    HEAPVAL

    hSelectedFont = NULL;

    // Это может быть, если отключена буферизация (debug) и вывод идет сразу на экран
    if (!abNoDc)
    {
		DEBUGSTRDRAW(L"*** Recreate DC\n");
        MSectionLock SDC;
        // Если в этой нити уже заблокирован - секция не дергается
        SDC.Lock(&csDC, TRUE, 200); // но по таймауту, чтобы не повисли ненароком

        if (hDC)
        { DeleteDC(hDC); hDC = NULL; }
        if (hBitmap)
        { DeleteObject(hBitmap); hBitmap = NULL; }

        const HDC hScreenDC = GetDC(0);
        if ((hDC = CreateCompatibleDC(hScreenDC)) != NULL)
        {
            Assert ( gSet.FontWidth() && gSet.FontHeight() );
			nFontHeight = gSet.FontHeight();
			nFontWidth = gSet.FontWidth();
			nFontCharSet = gSet.FontCharSet();

            #ifdef _DEBUG
            BOOL lbWasInitialized = TextWidth && TextHeight;
            #endif
            // Посчитать новый размер в пикселях
            Width = TextWidth * nFontWidth;
            Height = TextHeight * nFontHeight;
            _ASSERTE(Height <= 1200);

            if (ghOpWnd)
                gConEmu.UpdateSizes();

            //if (!lbWasInitialized) // если зовется InitDC значит размер консоли изменился
            if (!abNoWndResize) {
                if (gConEmu.isVisible(this))
                    gConEmu.OnSize(-1);
            }

            hBitmap = CreateCompatibleBitmap(hScreenDC, Width, Height);
            SelectObject(hDC, hBitmap);
        }
        ReleaseDC(0, hScreenDC);

        return hBitmap!=NULL;
    }

    return true;
}

void CVirtualConsole::DumpConsole()
{
    if (!this) return;
    
    OPENFILENAME ofn; memset(&ofn,0,sizeof(ofn));
    WCHAR temp[MAX_PATH+5];
    ofn.lStructSize=sizeof(ofn);
    ofn.hwndOwner = ghWnd;
    ofn.lpstrFilter = _T("ConEmu dumps (*.con)\0*.con\0\0");
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = temp; temp[0] = 0;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = L"Dump console...";
    ofn.lpstrDefExt = L"con";
    ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
            | OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
    if (!GetSaveFileName(&ofn))
        return;

    Dump(temp);
}

BOOL CVirtualConsole::Dump(LPCWSTR asFile)
{
    // Она сделает снимок нашего буфера (hDC) в png файл
    DumpImage(hDC, Width, Height, asFile);
        
    HANDLE hFile = CreateFile(asFile, GENERIC_WRITE, FILE_SHARE_READ,
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        DisplayLastError(_T("Can't create file!"));
        return FALSE;
    }
    DWORD dw;
    LPCTSTR pszTitle = gConEmu.GetTitle();
    WriteFile(hFile, pszTitle, _tcslen(pszTitle)*sizeof(*pszTitle), &dw, NULL);
    wchar_t temp[100];
	swprintf(temp, _T("\r\nSize: %ix%i   Cursor: %ix%i\r\n"), TextWidth, TextHeight, Cursor.x, Cursor.y);
    WriteFile(hFile, temp, _tcslen(temp)*sizeof(TCHAR), &dw, NULL);
    WriteFile(hFile, mpsz_ConChar, TextWidth * TextHeight * sizeof(*mpsz_ConChar), &dw, NULL);
    WriteFile(hFile, mpn_ConAttrEx, TextWidth * TextHeight * sizeof(*mpn_ConAttrEx), &dw, NULL);
    WriteFile(hFile, mpsz_ConCharSave, TextWidth * TextHeight * sizeof(*mpsz_ConCharSave), &dw, NULL);
    WriteFile(hFile, mpn_ConAttrExSave, TextWidth * TextHeight * sizeof(*mpn_ConAttrExSave), &dw, NULL);
    if (mp_RCon) {
        mp_RCon->DumpConsole(hFile);
    }
    CloseHandle(hFile);
	return TRUE;
}

// Это символы рамок и др. спец. символы
//#define isCharBorder(inChar) (inChar>=0x2013 && inChar<=0x266B)
bool CVirtualConsole::isCharBorder(WCHAR inChar)
{
	if (!gSet.isFixFarBorders)
		return false;
	return gSet.isCharBorder(inChar);

	//CSettings::CharRanges *pcr = gSet.icFixFarBorderRanges;
	//for (int i = 10; --i && pcr->bUsed; pcr++) {
	//	CSettings::CharRanges cr = *pcr;
	//	if (inChar>=cr.cBegin && inChar<=cr.cEnd)
	//		return true;
	//}

	//return false;

    //if (inChar>=0x2013 && inChar<=0x266B)
    //    return true;
    //else
    //    return false;

    //if (gSet.isFixFarBorders)
    //{
    //  //if (! (inChar > 0x2500 && inChar < 0x251F))
    //  if ( !(inChar > 0x2013/*En dash*/ && inChar < 0x266B/*Beamed Eighth Notes*/) /*&& inChar!=L'}'*/ )
    //      /*if (inChar != 0x2550 && inChar != 0x2502 && inChar != 0x2551 && inChar != 0x007D &&
    //      inChar != 0x25BC && inChar != 0x2593 && inChar != 0x2591 && inChar != 0x25B2 &&
    //      inChar != 0x2562 && inChar != 0x255F && inChar != 0x255A && inChar != 0x255D &&
    //      inChar != 0x2554 && inChar != 0x2557 && inChar != 0x2500 && inChar != 0x2534 && inChar != 0x2564) // 0x2520*/
    //      return false;
    //  else
    //      return true;
    //}
    //else
    //{
    //  if (inChar < 0x01F1 || inChar > 0x0400 && inChar < 0x045F || inChar > 0x2012 && inChar < 0x203D || /*? - not sure that optimal*/ inChar > 0x2019 && inChar < 0x2303 || inChar > 0x24FF && inChar < 0x266C)
    //      return false;
    //  else
    //      return true;
    //}
}

// А это только "рамочные" символы, в которых есть любая (хотя бы частичная) вертикальная черта + стрелки/штриховки
bool CVirtualConsole::isCharBorderVertical(WCHAR inChar)
{
    //if (inChar>=0x2502 && inChar<=0x25C4 && inChar!=0x2550)
	//2009-07-12 Для упрощения - зададим диапазон рамок за исключением горизонтальной линии
    //if (inChar==ucBoxSinglVert || inChar==0x2503 || inChar==0x2506 || inChar==0x2507
    //    || (inChar>=0x250A && inChar<=0x254B) || inChar==0x254E || inChar==0x254F
    //    || (inChar>=0x2551 && inChar<=0x25C5)) // По набору символов Arial Unicode MS
    TODO("ucBoxSinglHorz отсекать не нужно?");
	if (inChar != ucBoxDblHorz && (inChar >= ucBoxSinglVert && inChar <= ucBoxDblVertHorz))
        return true;
    else
        return false;
}

bool CVirtualConsole::isCharProgress(WCHAR inChar)
{
    bool isProgress = (inChar == ucBox25 || inChar == ucBox50 || inChar == ucBox75 || inChar == ucBox100);
    return isProgress;
}

bool CVirtualConsole::isCharScroll(WCHAR inChar)
{
	bool isScrollbar = (inChar == ucBox25 || inChar == ucBox50 || inChar == ucBox75 || inChar == ucBox100
		|| inChar == ucUpScroll || inChar == ucDnScroll);
	return isScrollbar;
}

void CVirtualConsole::BlitPictureTo(int inX, int inY, int inWidth, int inHeight)
{
	#ifdef _DEBUG
	BOOL lbDump = FALSE;
	if (lbDump) DumpImage(gSet.hBgDc, gSet.bgBmp.X, gSet.bgBmp.Y, L"F:\\bgtemp.png");
	#endif
    if (gSet.bgBmp.X>inX && gSet.bgBmp.Y>inY)
        BitBlt(hDC, inX, inY, inWidth, inHeight, gSet.hBgDc, inX, inY, SRCCOPY);
    if (gSet.bgBmp.X<(inX+inWidth) || gSet.bgBmp.Y<(inY+inHeight))
    {
        if (hBrush0 == NULL) {
            hBrush0 = CreateSolidBrush(mp_Colors[0]);
            SelectBrush(hBrush0);
        }

        RECT rect = {max(inX,gSet.bgBmp.X), inY, inX+inWidth, inY+inHeight};
        #ifndef SKIP_ALL_FILLRECT
        if (!IsRectEmpty(&rect))
            FillRect(hDC, &rect, hBrush0);
        #endif

        if (gSet.bgBmp.X>inX) {
            rect.left = inX; rect.top = max(inY,gSet.bgBmp.Y); rect.right = gSet.bgBmp.X;
            #ifndef SKIP_ALL_FILLRECT
            if (!IsRectEmpty(&rect))
                FillRect(hDC, &rect, hBrush0);
            #endif
        }

        //DeleteObject(hBrush);
    }
}

void CVirtualConsole::SelectFont(HFONT hNew)
{
    if (hNew == NULL) {
        if (hOldFont)
            SelectObject(hDC, hOldFont);
        hOldFont = NULL;
        hSelectedFont = NULL;
    } else
    if (hSelectedFont != hNew)
    {
        hSelectedFont = (HFONT)SelectObject(hDC, hNew);
        if (!hOldFont)
            hOldFont = hSelectedFont;
        hSelectedFont = hNew;
    }
}

void CVirtualConsole::SelectBrush(HBRUSH hNew)
{
    if (!hNew) {
        if (hOldBrush)
            SelectObject(hDC, hOldBrush);
        hOldBrush = NULL;
    } else
    if (hSelectedBrush != hNew)
    {
        hSelectedBrush = (HBRUSH)SelectObject(hDC, hNew);
        if (!hOldBrush)
            hOldBrush = hSelectedBrush;
        hSelectedBrush = hNew;
    }
}

bool CVirtualConsole::CheckSelection(const CONSOLE_SELECTION_INFO& select, SHORT row, SHORT col)
{
	TODO("Выделение пока не живет");
	return false;
    //if ((select.dwFlags & CONSOLE_SELECTION_NOT_EMPTY) == 0)
    //    return false;
    //if (row < select.srSelection.Top || row > select.srSelection.Bottom)
    //    return false;
    //if (col < select.srSelection.Left || col > select.srSelection.Right)
    //    return false;
    //return true;
}

#ifdef MSGLOGGER
class DcDebug {
public:
    DcDebug(HDC* ahDcVar, HDC* ahPaintDC) {
        mb_Attached=FALSE; mh_OrigDc=NULL; mh_DcVar=NULL; mh_Dc=NULL;
        if (!ahDcVar || !ahPaintDC) return;
        mh_DcVar = ahDcVar;
        mh_OrigDc = *ahDcVar;
        mh_Dc = *ahPaintDC;
        *mh_DcVar = mh_Dc;
    };
    ~DcDebug() {
        if (mb_Attached && mh_DcVar) {
            mb_Attached = FALSE;
            *mh_DcVar = mh_OrigDc;
        }
    };
protected:
    BOOL mb_Attached;
    HDC mh_OrigDc, mh_Dc;
    HDC* mh_DcVar;
};
#endif

// Преобразовать консольный цветовой атрибут (фон+текст) в цвета фона и текста
// (!) Количество цветов текста могут быть расширено за счет одного цвета фона
// atr принудительно в BYTE, т.к. верхние флаги нас не интересуют
//void CVirtualConsole::GetCharAttr(WORD atr, BYTE& foreColorNum, BYTE& backColorNum, HFONT* pFont)
//{
//	// Нас интересует только нижний байт
//    atr &= 0xFF;
//    
//    // быстрее будет создать заранее массив с цветами, и получять нужное по индексу
//    foreColorNum = m_ForegroundColors[atr];
//    backColorNum = m_BackgroundColors[atr];
//    if (pFont) *pFont = mh_FontByIndex[atr];
//
//    if (bExtendColors) {
//        if (backColorNum == nExtendColor) {
//            backColorNum = attrBackLast; // фон нужно заменить на обычный цвет из соседней ячейки
//            foreColorNum += 0x10;
//        } else {
//            attrBackLast = backColorNum; // запомним обычный цвет соседней ячейки
//        }
//    }
//}

void CVirtualConsole::CharABC(TCHAR ch, ABC *abc)
{
	BOOL lbCharABCOk;

	if (!gSet.CharABC[ch].abcB) {
		if (gSet.mh_Font2 && isCharBorder(ch)) {
			SelectFont(gSet.mh_Font2);
		} else {
			TODO("Тут надо бы деление по стилям сделать");
			SelectFont(gSet.mh_Font[0]);
		}

		//This function succeeds only with TrueType fonts
		lbCharABCOk = GetCharABCWidths(hDC, ch, ch, &gSet.CharABC[ch]);
		if (!lbCharABCOk) {
			// Значит шрифт не TTF/OTF
			gSet.CharABC[ch].abcB = CharWidth(ch);
			_ASSERTE(gSet.CharABC[ch].abcB);
			if (!gSet.CharABC[ch].abcB) gSet.CharABC[ch].abcB = 1;
			gSet.CharABC[ch].abcA = gSet.CharABC[ch].abcC = 0;
		}
	}

	*abc = gSet.CharABC[ch];
}

// Возвращает ширину символа, учитывает FixBorders
WORD CVirtualConsole::CharWidth(TCHAR ch)
{
	// Проверяем сразу, чтобы по условиям не бегать
	WORD nWidth = gSet.CharWidth[ch];
	if (nWidth)
		return nWidth;

	if (gSet.isMonospace 
		|| (gSet.isFixFarBorders && isCharBorder(ch))
		|| (gSet.isEnhanceGraphics && isCharProgress(ch))
		)
	{
		//2009-09-09 Это некорректно. Ширина шрифта рамки может быть больше знакоместа
		//return gSet.BorderFontWidth();
		gSet.CharWidth[ch] = nFontWidth;
		return nFontWidth;
	}

    //nWidth = nFontWidth;
    //bool isBorder = false; //, isVBorder = false;


	// Наверное все же нужно считать именно в том шрифте, которым будет идти отображение
	if (gSet.mh_Font2 && isCharBorder(ch)) {
		SelectFont(gSet.mh_Font2);
	} else {
		TODO("Тут надо бы деление по стилям сделать");
		SelectFont(gSet.mh_Font[0]);
	}
	//SelectFont(gSet.mh_Font[0]);
    SIZE sz;
    //This function succeeds only with TrueType fonts
	//#ifdef _DEBUG
	//ABC abc;
	//BOOL lb1 = GetCharABCWidths(hDC, ch, ch, &abc);
	//#endif

    if (GetTextExtentPoint32(hDC, &ch, 1, &sz) && sz.cx)
        nWidth = sz.cx;
    else
        nWidth = nFontWidth;

	if (!nWidth)
		nWidth = 1; // на всякий случай, чтобы деления на 0 не возникло

    gSet.CharWidth[ch] = nWidth;

    return nWidth;
}

char CVirtualConsole::Uni2Oem(wchar_t ch)
{
	if (mc_Uni2Oem[ch])
		return mc_Uni2Oem[ch];

	char c = '?';
	WideCharToMultiByte(CP_OEMCP, 0, &ch, 1, &c, 1, 0, 0);
	mc_Uni2Oem[ch] = c;
	return c;
}

bool CVirtualConsole::CheckChangedTextAttr()
{
    textChanged = 0!=memcmp(mpsz_ConChar, mpsz_ConCharSave, TextLen * sizeof(*mpsz_ConChar));
    attrChanged = 0!=memcmp(mpn_ConAttrEx, mpn_ConAttrExSave, TextLen * sizeof(*mpn_ConAttrEx));

//#ifdef MSGLOGGER
//    COORD ch;
//    if (textChanged) {
//        for (UINT i=0; i<TextLen; i++) {
//            if (mpsz_ConChar[i] != mpsz_ConCharSave[i]) {
//                ch.Y = i % TextWidth;
//                ch.X = i - TextWidth * ch.Y;
//                break;
//            }
//        }
//    }
//    if (attrChanged) {
//        for (UINT i=0; i<TextLen; i++) {
//            if (mpn_ConAttr[i] != mpn_ConAttrSave[i]) {
//                ch.Y = i % TextWidth;
//                ch.X = i - TextWidth * ch.Y;
//                break;
//            }
//        }
//    }
//#endif

    return textChanged || attrChanged;
}

bool CVirtualConsole::Update(bool isForce, HDC *ahDc)
{
    if (!this || !mp_RCon || !mp_RCon->ConWnd())
        return false;

	if (!gConEmu.isMainThread()) {
		if (isForce)
			mb_RequiredForceUpdate = true;

		//if (mb_RequiredForceUpdate || updateText || updateCursor)
		{
			if (gConEmu.isVisible(this)) {
				if (gSet.isAdvLogging>=3) mp_RCon->LogString("Invalidating from CVirtualConsole::Update.1");
				gConEmu.Invalidate(this);
			}

			return true;
		}

		return false;
	}


	/* -- 2009-06-20 переделал. графика теперь крутится ТОЛЬКО в MainThread
    if (!mp_RCon->IsConsoleThread()) {
        if (!ahDc) {
            mp_RCon->SetForceRead();
            mp_RCon->WaitEndUpdate(1000);
            //gConEmu.InvalidateAll(); -- зачем на All??? Update и так Invalidate зовет
            return false;
        }
    }
	*/

    // Рисуем актуальную информацию из CRealConsole. ЗДЕСЬ запросы в консоль не делаются.
    //RetrieveConsoleInfo();

    #ifdef MSGLOGGER
    DcDebug dbg(&hDC, ahDc); // для отладки - рисуем сразу на канвасе окна
    #endif


    HEAPVAL
    bool lRes = false;
    
    MSectionLock SCON; SCON.Lock(&csCON);
    MSectionLock SDC; //&csDC, &ncsTDC);
    //if (mb_PaintRequested) -- не должно быть. Эта функция работает ТОЛЬКО в консольной нити
    if (mb_PaintLocked) // Значит идет асинхронный Paint (BitBlt) - это может быть во время ресайза, или над окошком что-то протащили
        SDC.Lock(&csDC, TRUE, 200); // но по таймауту, чтобы не повисли ненароком


    mp_RCon->GetConsoleScreenBufferInfo(&csbi);

    // start timer before "Read Console Output*" calls, they do take time
    //gSet.Performance(tPerfRead, FALSE);

    //if (gbNoDblBuffer) isForce = TRUE; // Debug, dblbuffer

	isForeground = gConEmu.isMeForeground();
	if (isFade == isForeground && gSet.isFadeInactive)
		isForce = true;

    //------------------------------------------------------------------------
    ///| Read console output and cursor info... |/////////////////////////////
    //------------------------------------------------------------------------
    if (!UpdatePrepare(isForce, ahDc, &SDC)) {
        gConEmu.DebugStep(_T("DC initialization failed!"));
        return false;
    }
    
    //gSet.Performance(tPerfRead, TRUE);

    gSet.Performance(tPerfRender, FALSE);

    //------------------------------------------------------------------------
    ///| Drawing text (if there were changes in console) |////////////////////
    //------------------------------------------------------------------------
    bool updateText, updateCursor;
    if (isForce)
    {
        updateText = updateCursor = attrChanged = textChanged = true;
    }
    else
    {

        // Do we have to update changed text?
        updateText = CheckChangedTextAttr();

        // Do we have to update text under changed cursor?
        // Important: check both 'cinf.bVisible' and 'Cursor.isVisible',
        // because console may have cursor hidden and its position changed -
        // in this case last visible cursor remains shown at its old place.
        // Also, don't check foreground window here for the same reasons.
        // If position is the same then check the cursor becomes hidden.
        if (Cursor.x != csbi.dwCursorPosition.X || Cursor.y != csbi.dwCursorPosition.Y)
            // сменилась позиция. обновляем если курсор видим
            updateCursor = cinf.bVisible || Cursor.isVisible || Cursor.isVisiblePrevFromInfo;
        else
            updateCursor = Cursor.isVisiblePrevFromInfo && !cinf.bVisible;
    }

	mb_DialogsChanged = CheckDialogsChanged();

	// Если на панелях открыт ConEmuTh - причесать заголовки и
	// строку разделителя инф.части (там не должно оставаться уголовых элементов)
	// И обновить видимые регионы (скрыть части, которые находятся "под" диалогами)
	PolishPanelViews();
    
    //gSet.Performance(tPerfRender, FALSE);

    if (updateText /*|| updateCursor*/)
    {
        lRes = true;

        DEBUGSTRDRAW(L" +++ updateText detected in VCon\n");
        
        //gSet.Performance(tPerfRender, FALSE);

        //------------------------------------------------------------------------
        ///| Drawing modified text |//////////////////////////////////////////////
        //------------------------------------------------------------------------
        UpdateText(isForce);
        
        //gSet.Performance(tPerfRender, TRUE);


        //HEAPVAL
        //------------------------------------------------------------------------
        ///| Now, store data for further comparison |/////////////////////////////
        //------------------------------------------------------------------------
        memcpy(mpsz_ConCharSave, mpsz_ConChar, TextLen * sizeof(*mpsz_ConChar));
        memcpy(mpn_ConAttrExSave, mpn_ConAttrEx, TextLen * sizeof(*mpn_ConAttrEx));
    }

	// Если зарегистрированы панели (ConEmuTh) - обновить видимые регионы
	// Делать это нужно после UpdateText, потому что иначе ConCharX может быть обнулен
	if (m_LeftPanelView.bRegister || m_RightPanelView.bRegister) {
		if (mb_DialogsChanged) {
			UpdatePanelRgn(TRUE);
			UpdatePanelRgn(FALSE);
		}
	}


    //HEAPVAL
    //------------------------------------------------------------------------
    ///| Checking cursor |////////////////////////////////////////////////////
    //------------------------------------------------------------------------
    UpdateCursor(lRes);
    
    HEAPVAL
    
    //SDC.Leave();
    SCON.Unlock();

    // После успешного обновления внутренних буферов (символы/атрибуты)
    // 
    if (lRes && gConEmu.isVisible(this)) {
        if (mpsz_LogScreen && mp_RCon && mp_RCon->GetServerPID()) {
        	// Скинуть буферы в лог
            mn_LogScreenIdx++;
            wchar_t szLogPath[MAX_PATH]; wsprintfW(szLogPath, mpsz_LogScreen, mp_RCon->GetServerPID(), mn_LogScreenIdx);
            Dump(szLogPath);
        }

        // Если допустимо - передернуть обновление viewport'а
		if (!mb_InPaintCall) {
			// должен вызываться в основной нити
			_ASSERTE(gConEmu.isMainThread());
			mb_PaintRequested = TRUE;
			gConEmu.Invalidate(this);
			if (gSet.isAdvLogging>=3) mp_RCon->LogString("Invalidating from CVirtualConsole::Update.2");
			//09.06.13 а если так? быстрее изменения на экране не появятся?
			//UpdateWindow(ghWndDC); // оно посылает сообщение в окно, и ждет окончания отрисовки
			#ifdef _DEBUG
			//_ASSERTE(!gConEmu.m_Child.mb_Invalidated);
			#endif
			mb_PaintRequested = FALSE;
		}
    }

    gSet.Performance(tPerfRender, TRUE);

    /* ***************************************** */
    /*       Finalization, release objects       */
    /* ***************************************** */
    SelectBrush(NULL);
    if (hBrush0) { // создается в BlitPictureTo
        DeleteObject(hBrush0); hBrush0 = NULL;
    }
	/*
    for (UINT br=0; br<m_PartBrushes.size(); br++) {
        DeleteObject(m_PartBrushes[br].hBrush);
    }
    m_PartBrushes.clear();
	*/
    SelectFont(NULL);
    HEAPVAL
    return lRes;
}

BOOL CVirtualConsole::CheckTransparentRgn()
{
	BOOL lbRgnChanged = FALSE;

	if (mb_ConDataChanged) {
		mb_ConDataChanged = FALSE;

		// (пере)Создать регион
		if (mpsz_ConChar && mpn_ConAttrEx && TextWidth && TextHeight) {
			MSectionLock SCON; SCON.Lock(&csCON);

			CharAttr* pnAttr = mpn_ConAttrEx;

			int nFontHeight = gSet.FontHeight();
			int    nMaxRects = TextHeight*5;
#ifdef _DEBUG
			nMaxRects = 5;
#endif
			POINT *lpAllPoints = (POINT*)Alloc(nMaxRects*4,sizeof(POINT));
			//INT   *lpAllCounts = (INT*)Alloc(nMaxRects,sizeof(INT));
			HEAPVAL;
			int    nRectCount = 0;

			if (lpAllPoints /*&& lpAllCounts*/)
			{
				POINT *lpPoints = lpAllPoints;
				//INT   *lpCounts = lpAllCounts;

				for (uint nY = 0; nY < TextHeight; nY++) {
					uint nX = 0;
					int nYPix1 = nY * nFontHeight;

					while (nX < TextWidth) {
						// Найти первый прозрачный cell
						while (nX < TextWidth && !pnAttr[nX].bTransparent)
							nX++;
						if (nX >= TextWidth)
							break;
						// Найти конец прозрачного блока
						uint nTranStart = nX;
						while (++nX < TextWidth && pnAttr[nX].bTransparent)
							;
						// Сформировать соответствующий Rect
						int nXPix = 0;
						if (nRectCount>=nMaxRects) {
							nMaxRects += TextHeight;
							HEAPVAL;
							POINT *lpTmpPoints = (POINT*)Alloc(nMaxRects*4,sizeof(POINT));
							//INT   *lpTmpCounts = (INT*)Alloc(nMaxRects,sizeof(INT));
							HEAPVAL;
							if (!lpTmpPoints /*|| !lpTmpCounts*/) {
								_ASSERTE(/*lpTmpCounts &&*/ lpTmpPoints);
								//Free(lpAllCounts);
								Free(lpAllPoints);
								return FALSE;
							}
							memmove(lpTmpPoints, lpAllPoints, nRectCount*4*sizeof(POINT));
							//memmove(lpTmpCounts, lpAllCounts, nRectCount*sizeof(INT));
							HEAPVAL;
							lpPoints = lpTmpPoints + (lpPoints - lpAllPoints);
							//Free(lpAllCounts);
							Free(lpAllPoints);
							lpAllPoints = lpTmpPoints;
							//lpAllCounts = lpTmpCounts;
							HEAPVAL;
						}
						nRectCount++;
#ifdef _DEBUG
						if (nRectCount>=nMaxRects) {
							nRectCount = nRectCount;
						}
#endif
						//*(lpCounts++) = 4;
						lpPoints[0] = ConsoleToClient(nTranStart, nY);
						lpPoints[1] = ConsoleToClient(nX, nY);
							// x - 1?
							//if (lpPoints[1].x > lpPoints[0].x) lpPoints[1].x--;
						lpPoints[2] = lpPoints[1]; lpPoints[2].y += nFontHeight;
						lpPoints[3] = lpPoints[0]; lpPoints[3].y = lpPoints[2].y;
						lpPoints += 4;

						nX++;
						HEAPVAL;
					}
					pnAttr += TextWidth;
				}

				HEAPVAL;
				lbRgnChanged = (nRectCount != TransparentInfo.nRectCount);
				if (!lbRgnChanged && TransparentInfo.nRectCount) {
					_ASSERTE(TransparentInfo.pAllPoints && TransparentInfo.pAllCounts);
					lbRgnChanged = memcmp(TransparentInfo.pAllPoints, lpAllPoints, sizeof(POINT)*4*nRectCount)!=0;
				}

				HEAPVAL;
				if (lbRgnChanged) {
					if (mh_TransparentRgn) { DeleteObject(mh_TransparentRgn); mh_TransparentRgn = NULL; }

					INT   *lpAllCounts = NULL;
					if (nRectCount > 0) {
						lpAllCounts = (INT*)Alloc(nRectCount,sizeof(INT));
						INT   *lpCounts = lpAllCounts;
						for (int n = 0; n < nRectCount; n++)
							*(lpCounts++) = 4;

						mh_TransparentRgn = CreatePolyPolygonRgn(lpAllPoints, lpAllCounts, nRectCount, WINDING);
					}


					HEAPVAL;
					if (TransparentInfo.pAllCounts)
						Free(TransparentInfo.pAllCounts);
					HEAPVAL;
					TransparentInfo.pAllCounts = lpAllCounts; lpAllCounts = NULL;
					if (TransparentInfo.pAllPoints)
						Free(TransparentInfo.pAllPoints);
					HEAPVAL;
					TransparentInfo.pAllPoints = lpAllPoints; lpAllPoints = NULL;
					HEAPVAL;
					TransparentInfo.nRectCount = nRectCount;
				}

				HEAPVAL;
				//if (lpAllCounts) Free(lpAllCounts);
				if (lpAllPoints) Free(lpAllPoints);
				HEAPVAL;
			}
		}
	}

	return lbRgnChanged;
}

bool CVirtualConsole::UpdatePrepare(bool isForce, HDC *ahDc, MSectionLock *pSDC)
{
    MSectionLock SCON; SCON.Lock(&csCON);
    
    attrBackLast = 0;
    isEditor = gConEmu.isEditor();
	isViewer = gConEmu.isViewer();
    isFilePanel = gConEmu.isFilePanel(true);

	isFade = !isForeground && gSet.isFadeInactive;
	mp_Colors = gSet.GetColors(isFade);


	nFontHeight = gSet.FontHeight();
	nFontWidth = gSet.FontWidth();
	nFontCharSet = gSet.FontCharSet();
	
	bExtendColors = gSet.isExtendColors;
	nExtendColor = gSet.nExtendColor;
	
    bExtendFonts = gSet.isExtendFonts;
    nFontNormalColor = gSet.nFontNormalColor;
    nFontBoldColor = gSet.nFontBoldColor;
    nFontItalicColor = gSet.nFontItalicColor;
    
    //m_ForegroundColors[0x100], m_BackgroundColors[0x100];
    //TODO("В принципе, это можно делать не всегда, а только при изменениях");
    //int nColorIndex = 0;
    //for (int nBack = 0; nBack <= 0xF; nBack++) {
    //	for (int nFore = 0; nFore <= 0xF; nFore++, nColorIndex++) {
    //		m_ForegroundColors[nColorIndex] = nFore;
    //		m_BackgroundColors[nColorIndex] = nBack;
    //		mh_FontByIndex[nColorIndex] = gSet.mh_Font;
	//		if (bExtendFonts) {
	//			if (nBack == nFontBoldColor) { // nFontBoldColor may be -1, тогда мы сюда не попадаем
	//				if (nFontNormalColor != 0xFF)
	//					m_BackgroundColors[nColorIndex] = nFontNormalColor;
	//				mh_FontByIndex[nColorIndex] = gSet.mh_FontB;
	//			} else if (nBack == nFontItalicColor) { // nFontItalicColor may be -1, тогда мы сюда не попадаем
	//				if (nFontNormalColor != 0xFF)
	//					m_BackgroundColors[nColorIndex] = nFontNormalColor;
	//				mh_FontByIndex[nColorIndex] = gSet.mh_FontI;
	//			}
	//		}
	//	}
    //}


    //winSize.X = csbi.srWindow.Right - csbi.srWindow.Left + 1; winSize.Y = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    //if (winSize.X < csbi.dwSize.X)
    //    winSize.X = csbi.dwSize.X;
    winSize = MakeCoord(mp_RCon->TextWidth(),mp_RCon->TextHeight());

    //csbi.dwCursorPosition.X -= csbi.srWindow.Left; -- горизонтальная прокрутка игнорируется!
    csbi.dwCursorPosition.Y -= csbi.srWindow.Top;
    isCursorValid =
        csbi.dwCursorPosition.X >= 0 && csbi.dwCursorPosition.Y >= 0 &&
        csbi.dwCursorPosition.X < winSize.X && csbi.dwCursorPosition.Y < winSize.Y;

    if (mb_RequiredForceUpdate || mb_IsForceUpdate) {
        isForce = true;
        mb_RequiredForceUpdate = false;
    }

    // Первая инициализация, или смена размера
    if (isForce || !mb_PointersAllocated || TextWidth != (uint)winSize.X || TextHeight != (uint)winSize.Y) {
        if (pSDC && !pSDC->isLocked()) // Если секция еще не заблокирована (отпускает - вызывающая функция)
            pSDC->Lock(&csDC, TRUE, 200); // но по таймауту, чтобы не повисли ненароком
        if (!InitDC(ahDc!=NULL && !isForce/*abNoDc*/, false/*abNoWndResize*/))
            return false;

		isForce = true; // После сброса буферов и DC - необходим полный refresh...

		#ifdef _DEBUG
		if (TextWidth == 80 && !mp_RCon->isNtvdm()) {
			TextWidth = TextWidth;
		}
		#endif
        gConEmu.OnConsoleResize();
    }

    // Требуется полная перерисовка!
    if (mb_IsForceUpdate)
    {
        isForce = true;
        mb_IsForceUpdate = false;
    }
    // Отладочный режим. Двойная буферизация отключена, отрисовка идет сразу на ahDc
    if (ahDc)
        isForce = true;


    drawImage = (gSet.isShowBgImage == 1 || (gSet.isShowBgImage == 2 && !(isEditor || isViewer)) )
		&& gSet.isBackgroundImageValid;
    TextLen = TextWidth * TextHeight;
    coord.X = csbi.srWindow.Left; coord.Y = csbi.srWindow.Top;

    // скопировать данные из состояния консоли В mpn_ConAttrEx/mpsz_ConChar
	BOOL bConDataChanged = isForce || mp_RCon->IsConsoleDataChanged();
	if (bConDataChanged) {
		mb_ConDataChanged = TRUE; // В FALSE - НЕ сбрасывать
		
		gSet.Performance(tPerfData, FALSE);
		
		mp_RCon->GetConsoleData(mpsz_ConChar, mpn_ConAttrEx, TextWidth, TextHeight); //TextLen*2);

		gSet.Performance(tPerfData, TRUE);
	}

    HEAPVAL
 
    
    // Определить координаты панелей (Переехало в CRealConsole)

    // get cursor info
    mp_RCon->GetConsoleCursorInfo(/*hConOut(),*/ &cinf);

    HEAPVAL
    return true;
}

//enum CVirtualConsole::_PartType CVirtualConsole::GetCharType(TCHAR ch)
//{
//    enum _PartType cType = pNull;
//
//    if (ch == L' ')
//        cType = pSpace;
//    //else if (ch == L'_')
//    //  cType = pUnderscore;
//    else if (isCharBorder(ch)) {
//        if (isCharBorderVertical(ch))
//            cType = pVBorder;
//        else
//            cType = pBorder;
//    }
//    else if (isFilePanel && ch == L'}')
//        cType = pRBracket;
//    else
//        cType = pText;
//
//    return cType;
//}



// row - 0-based
//void CVirtualConsole::ParseLine(int row, TCHAR *ConCharLine, WORD *ConAttrLine)
//{
//    //UINT idx = 0;
//    struct _TextParts *pStart=TextParts, *pEnd=TextParts;
//    enum _PartType cType1, cType2;
//    UINT i1=0, i2=0;
//    
//    pEnd->partType = pNull; // сразу ограничим строку
//    
//    TCHAR ch1, ch2;
//    BYTE af1, ab1, af2, ab2;
//    DWORD pixels;
//    while (i1<TextWidth)
//    {
//        //GetCharAttr(ConCharLine[i1], ConAttrLine[i1], ch1, af1, ab1);
//		ch1 = ConCharLine[i1];
//		GetCharAttr(ConAttrLine[i1], af1, ab1, NULL);
//        cType1 = GetCharType(ch1);
//        if (cType1 == pRBracket) {
//            if (!(row>=2 && isCharBorderVertical(mpsz_ConChar[TextWidth+i1]))
//                && (((UINT)row)<=(TextHeight-4)))
//                cType1 = pText;
//        }
//        pixels = CharWidth(ch1);
//
//        i2 = i1+1;
//        // в режиме Force Monospace отрисовка идет по одному символу
//        if (!gSet.isForceMonospace && i2 < TextWidth && 
//            (cType1 != pVBorder && cType1 != pRBracket))
//        {
//            //GetCharAttr(ConCharLine[i2], ConAttrLine[i2], ch2, af2, ab2);
//			ch2 = ConCharLine[i2];
//			GetCharAttr(ConAttrLine[i2], af2, ab2, NULL);
//            // Получить блок символов с аналогичными цветами
//            while (i2 < TextWidth && af2 == af1 && ab2 == ab1) {
//                // если символ отличается от первого
//
//                cType2 = GetCharType(ch2);
//                if ((ch2 = ConCharLine[i2]) != ch1) {
//                    if (cType2 == pRBracket) {
//                        if (!(row>=2 && isCharBorderVertical(mpsz_ConChar[TextWidth+i2]))
//                            && (((UINT)row)<=(TextHeight-4)))
//                            cType2 = pText;
//                    }
//
//                    // и он вообще из другой группы
//                    if (cType2 != cType1)
//                        break; // то завершаем поиск
//                }
//                pixels += CharWidth(ch2); // добавить ширину символа в пикселях
//                i2++; // следующий символ
//                //GetCharAttr(ConCharLine[i2], ConAttrLine[i2], ch2, af2, ab2);
//				ch2 = ConCharLine[i2];
//				GetCharAttr(ConAttrLine[i2], af2, ab2, NULL);
//                if (cType2 == pRBracket) {
//                    if (!(row>=2 && isCharBorderVertical(mpsz_ConChar[TextWidth+i2]))
//                        && (((UINT)row)<=(TextHeight-4)))
//                        cType2 = pText;
//                }
//            }
//        }
//
//        // при разборе строки будем смотреть, если нашли pText,pSpace,pText то pSpace,pText добавить в первый pText
//        if (cType1 == pText && (pEnd - pStart) >= 2) {
//            if (pEnd[-1].partType == pSpace && pEnd[-2].partType == pText &&
//                pEnd[-1].attrBack == ab1 && pEnd[-1].attrFore == af1 &&
//                pEnd[-2].attrBack == ab1 && pEnd[-2].attrFore == af1
//                )
//            {   
//                pEnd -= 2;
//                pEnd->i2 = i2 - 1;
//                pEnd->iwidth = i2 - pEnd->i1;
//                pEnd->width += pEnd[1].width + pixels;
//                pEnd ++;
//                i1 = i2;
//                continue;
//            }
//        }
//        pEnd->i1 = i1; pEnd->i2 = i2 - 1; // конец "включая"
//        pEnd->partType = cType1;
//        pEnd->attrBack = ab1; pEnd->attrFore = af1;
//        pEnd->iwidth = i2 - i1;
//        pEnd->width = pixels;
//        if (gSet.isForceMonospace ||
//            (gSet.isProportional && (cType1 == pVBorder || cType1 == pRBracket)))
//        {
//            pEnd->x1 = i1 * gSet.FontWidth();
//        } else {
//            pEnd->x1 = -1;
//        }
//
//        pEnd ++; // блоков не может быть больше количества символов в строке, так что с размерностью все ОК
//        i1 = i2;
//    }
//    // пока поставим конец блоков, потом, если ширины не хватит - добавим pDummy
//    pEnd->partType = pNull;
//}

void CVirtualConsole::Update_CheckAndFill(bool isForce)
{
    // pointers
    wchar_t* ConCharLine = mpsz_ConChar;
    CharAttr* ConAttrLine = mpn_ConAttrEx;
    // previous
    const CharAttr* ConAttrLine2 = mpn_ConAttrExSave;
    const wchar_t* ConCharLine2 = mpsz_ConCharSave;
    
    // counters
    int pos = 0;
    int row = 0;
    int nMaxPos = Height - nFontHeight;
    
    // locals
    bool bRowChanged;

    // rows
    //BUGBUG: хорошо бы отрисовывать последнюю строку, даже если она чуть не влазит
    for (; pos <= nMaxPos; 
        ConCharLine += TextWidth, ConAttrLine += TextWidth,
        ConCharLine2 += TextWidth, ConAttrLine2 += TextWidth,
        pbLineChanged++, pbBackIsPic++, pnBackRGB++,
        pos += nFontHeight, row++)
    {
    	if (row >= (int)TextHeight) {
    		_ASSERTE(row<(int)TextHeight);
    		return;
    	}
    	
		bRowChanged = isForce
						|| (wmemcmp(ConCharLine, ConCharLine2, TextWidth) != 0)
						|| (memcmp(ConAttrLine, ConAttrLine2, TextWidth*sizeof(*ConAttrLine)) != 0);
    

        //// skip not changed symbols except the old cursor or selection
        //int j = 0, end = TextWidth;
		//// В режиме пропорциональных шрифтов или isForce==true - отрисовываем линию ЦЕЛИКОМ
		//// Для пропорциональных это обусловлено тем, что при перерисовке измененная часть
		//// может оказаться КОРОЧЕ предыдущего значения, в результате появятся артефакты
        //if (skipNotChanged)
        //{
        //    // Skip not changed head and tail symbols
		//	if (!FindChanges(j, end, ConCharLine, ConAttrLine, ConCharLine2, ConAttrLine2))
		//		continue;
        //} // if (skipNotChanged)
        
        *pbLineChanged = bRowChanged;
        
        if (bRowChanged) {
        	
        	if (drawImage && ISBGIMGCOLOR(ConAttrLine->nBackIdx)) {
        		*pbBackIsPic = true;
        		BlitPictureTo(0, pos, Width, nFontHeight);
        	} else {
        		*pbBackIsPic = false;
        		COLORREF cr = ConAttrLine->crBackColor;
        		*pnBackRGB = cr;
        		// Именно пробельный, чтобы не заморачиваться с хвостовыми пробелами отрезков
				HBRUSH hbr = PartBrush(L' ', cr, 0);
				RECT rect = MakeRect(0, pos, Width, pos + nFontHeight);
				FillRect(hDC, &rect, hbr);
        	}
        }
    }
}

// Разбор строки на составляющие (возвращает true, если есть ячейки с НЕ основным фоном)
// Функция также производит распределение (заполнение координат и DX)
bool CVirtualConsole::Update_ParseTextParts(uint row, const wchar_t* ConCharLine, const CharAttr* ConAttrLine)
{
	bool bHasAlternateBg = false;
	int j = 0, j0 = 0;
	TEXTPARTS *pTP = TextParts;
	
	bool bEnhanceGraphics = gSet.isEnhanceGraphics;
	//bool bProportional = gSet.isMonospace == 0;
	//bool bForceMonospace = gSet.isMonospace == 2;
	
    wchar_t c = 0;
    bool isUnicode = false, isProgress = false, isSpace = false, isUnicodeOrProgress = false;
    uint nPrevForeFont = 0, nForeFont;
	
    // Сначала - разбор фона
    BGPARTS *pBG = BgParts;
    uint nColorIdx = 0;
    uint nPrevColorIdx = ConAttrLine[0].nBackIdx;
    j = j0 = 0;
    while (j < (int)TextWidth) {
	    while (++j < (int)TextWidth) {
	    	if (nPrevColorIdx != ConAttrLine[j].nBackIdx) {
	    		nColorIdx = ConAttrLine[j].nBackIdx;
	    		break;
    		}
    	}
    	
	    pBG->i = j0;
	    pBG->n = j - j0;
	    if (!(pBG->bBackIsPic = (drawImage && ISBGIMGCOLOR(nPrevColorIdx))))
	    	pBG->nBackRGB = ConAttrLine[j0].crBackColor;
	    pBG++;
	    
	    nPrevColorIdx = nColorIdx;
	    j0 = j;
    }
    
    // *** Теперь - Foreground (text) ***
	// чтобы лишний раз не проверять - ветвимся
	j = 0;
	nPrevForeFont = ConAttrLine[0].ForeFont;
	while (j < (int)TextWidth) {
		j0 = j;
		const CharAttr* attr = ConAttrLine+j;
		nForeFont = attr->ForeFont;
		c = ConCharLine[j];
		isUnicode = isCharBorder(c);
        bool isProgress = false, isSpace = false, isUnicodeOrProgress = false;
        if (isUnicode || bEnhanceGraphics)
            isProgress = isCharProgress(c); // ucBox25 / ucBox50 / ucBox75 / ucBox100
		isUnicodeOrProgress = isUnicode || isProgress;
        if (!isUnicodeOrProgress)
            isSpace = isCharSpace(c);
        
        TODO("Учет 'вертикалей'");
        if (isProgress) {
            while (++j < (int)TextWidth) {
            	if (ConCharLine[j] != c || attr->All != ConAttrLine[j].All)
            		break; // до первой смены символа или цвета фона-текста
            }
        } else if (isSpace) {
            while (++j < (int)TextWidth) {
            	if (!isCharSpace(ConCharLine[j]))
            		break; // до первого непробельного символа
            }
		} else if (isUnicode) {
			COLORREF crFore = attr->crForeColor;
            while (++j < (int)TextWidth) {
            	if (!isCharBorder(ConCharLine[j]) || crFore != ConAttrLine[j].crForeColor)
            		break; // до первого нерамочного символа или смены цвета текста
            }
        } else {
        	//PRAGMA_ERROR("доделать ветку");
			while (++j < (int)TextWidth) {
	            attr = ConAttrLine+j;
	            
	            nForeFont = attr->ForeFont;
	            c = ConCharLine[j];
	            
	            // Если смена цвета фона или шрифта
	            if (nForeFont != nPrevForeFont)
	            	break;
	            
	            isUnicode = isCharBorder(c);
	            bool isProgress = false, isSpace = false, isUnicodeOrProgress = false;

	            if (isUnicode || bEnhanceGraphics) {
	                isProgress = isCharProgress(c); // ucBox25 / ucBox50 / ucBox75 / ucBox100
                }
				isUnicodeOrProgress = isUnicode || isProgress;
	            if (!isUnicodeOrProgress)
	                isSpace = isCharSpace(c);
			
			}
		}
		
		TODO("создать блок j0..j");
	}
	
	return bHasAlternateBg;
}

// Заливка ячеек с НЕ основным фоном, отрисовка прогрессов и рамок
void CVirtualConsole::Update_FillAlternate(uint row, uint nY)
{
}

// Вывод собственно текста (при необходимости Clipped)
void CVirtualConsole::Update_DrawText(uint row, uint nY)
{
}

void CVirtualConsole::UpdateText(bool isForce)
{
    //if (!updateText) {
    //    _ASSERTE(updateText);
    //    return;
    //}

#ifdef _DEBUG
    if (mp_RCon->IsConsoleThread()) {
        //_ASSERTE(!mp_RCon->IsConsoleThread());
    }
    //// Данные уже должны быть заполнены, и там не должно быть лажы
    //BOOL lbDataValid = TRUE; uint n = 0;
    //while (n<TextLen) {
    //    if (mpsz_ConChar[n] == 0) {
    //        lbDataValid = FALSE; //break;
    //        mpsz_ConChar[n] = L'¤';
    //        mpn_ConAttr[n] = 12;
    //    } else if (mpsz_ConChar[n] != L' ') {
    //        // 0 - может быть только для пробела. Иначе символ будет скрытым, чего по идее, быть не должно
    //        if (mpn_ConAttr[n] == 0) {
    //            lbDataValid = FALSE; //break;
    //            mpn_ConAttr[n] = 12;
    //        }
    //    }
    //    n++;
    //}
    ////_ASSERTE(lbDataValid);
#endif


	memmove(mh_FontByIndex, gSet.mh_Font, sizeof(mh_FontByIndex));
    SelectFont(mh_FontByIndex[0]);

    // pointers
    wchar_t* ConCharLine;
    CharAttr* ConAttrLine;
    DWORD* ConCharXLine;
    // counters
    int pos, row;
    {
        int i;

        i = 0; //TextLen - TextWidth; // TextLen = TextWidth/*80*/ * TextHeight/*25*/;
        pos = 0; //Height - nFontHeight; // Height = TextHeight * nFontHeight;
        row = 0; //TextHeight - 1;

		ConCharLine = mpsz_ConChar + i;
        ConAttrLine = mpn_ConAttrEx + i;
        ConCharXLine = ConCharX + i;
    }
    int nMaxPos = Height - nFontHeight;

    if (/*gSet.isForceMonospace ||*/ !drawImage)
        SetBkMode(hDC, OPAQUE);
    else
        SetBkMode(hDC, TRANSPARENT);

	int nDX[500];

    // rows
    // зачем в isForceMonospace принудительно перерисовывать все?
    // const bool skipNotChanged = !isForce /*&& !gSet.isForceMonospace*/;
    const bool skipNotChanged = !isForce && !((gSet.FontItalic() || gSet.FontClearType()));
	bool bEnhanceGraphics = gSet.isEnhanceGraphics;
	bool bProportional = gSet.isMonospace == 0;
	bool bForceMonospace = gSet.isMonospace == 2;
	bool bFixFarBorders = gSet.isFixFarBorders;
	//mh_FontByIndex[0] = gSet.mh_Font; mh_FontByIndex[1] = gSet.mh_FontB; mh_FontByIndex[2] = gSet.mh_FontI; mh_FontByIndex[3] = gSet.mh_FontBI;
	HFONT hFont = gSet.mh_Font[0];
	HFONT hFont2 = gSet.mh_Font2;
	//BUGBUG: хорошо бы отрисовывать последнюю строку, даже если она чуть не влазит
    for (; pos <= nMaxPos; 
        ConCharLine += TextWidth, ConAttrLine += TextWidth, ConCharXLine += TextWidth,
        pos += nFontHeight, row++)
    {
		//2009-09-25. Некоторые (старые?) программы умудряются засунуть в консоль символы (ASC<32)
		//            их нужно заменить на юникодные аналоги
		//{
		//	wchar_t* pszEnd = ConCharLine + TextWidth;
		//	for (wchar_t* pch = ConCharLine; pch < pszEnd; pch++) {
		//		if (((WORD)*pch) < 32) *pch = gszAnalogues[(WORD)*pch];
		//	}
		//}

        // the line
        const CharAttr* const ConAttrLine2 = mpn_ConAttrExSave + (ConAttrLine - mpn_ConAttrEx);
        const wchar_t* const ConCharLine2 = mpsz_ConCharSave + (ConCharLine - mpsz_ConChar);

        // skip not changed symbols except the old cursor or selection
        int j = 0, end = TextWidth;
		// В режиме пропорциональных шрифтов или isForce==true - отрисовываем линию ЦЕЛИКОМ
		// Для пропорциональных это обусловлено тем, что при перерисовке измененная часть
		// может оказаться КОРОЧЕ предыдущего значения, в результате появятся артефакты
        if (skipNotChanged)
        {
            // Skip not changed head and tail symbols
			if (!FindChanges(j, end, ConCharLine, ConAttrLine, ConCharLine2, ConAttrLine2))
				continue;
        } // if (skipNotChanged)
        
        if (Cursor.isVisiblePrev && row == Cursor.y
            && (j <= Cursor.x && Cursor.x <= end))
            Cursor.isVisiblePrev = false;

        // *) Now draw as much as possible in a row even if some symbols are not changed.
        // More calls for the sake of fewer symbols is slower, e.g. in panel status lines.
        int j2=j+1;
        for (; j < end; j = j2)
        {
			TODO("OPTIMIZE: вынести переменные из цикла");
			TODO("OPTIMIZE: если символ/атрибут совпадает с предыдущим (j>0 !) то не звать повторно CharWidth, isChar..., isUnicode, ...");

            const CharAttr attr = ConAttrLine[j];
            WCHAR c = ConCharLine[j];
            //BYTE attrForeIdx, attrBackIdx;
            //COLORREF attrForeClr, attrBackClr;
            bool isUnicode = isCharBorder(c/*ConCharLine[j]*/);
            bool isProgress = false, isSpace = false, isUnicodeOrProgress = false;
            bool lbS1 = false, lbS2 = false;
            int nS11 = 0, nS12 = 0, nS21 = 0, nS22 = 0;

			//GetCharAttr(c, attr, c, attrFore, attrBack);
			//GetCharAttr(attr, attrFore, attrBack, &hFont);
            //if (GetCharAttr(c, attr, c, attrFore, attrBack))
            //    isUnicode = true;
            hFont = mh_FontByIndex[attr.nFontIndex];

            if (isUnicode || bEnhanceGraphics)
                isProgress = isCharProgress(c); // ucBox25 / ucBox50 / ucBox75 / ucBox100
			isUnicodeOrProgress = isUnicode || isProgress;
            if (!isUnicodeOrProgress)
                isSpace = isCharSpace(c);
                

			TODO("При позиционировании (наверное в DistrubuteSpaces) пытаться ориентироваться по координатам предыдущей строки");
			// Иначе окантовка диалогов съезжает на пропорциональных шрифтах

            HEAPVAL
            // корректировка лидирующих пробелов и рамок
            if (bProportional && (c==ucBoxDblHorz || c==ucBoxSinglHorz))
            {
                lbS1 = true; nS11 = nS12 = j;
                while ((nS12 < end) && (ConCharLine[nS12+1] == c))
                    nS12 ++;
                // Посчитать сколько этих же символов ПОСЛЕ текста
                if (nS12<end) {
                    nS21 = nS12+1; // Это должен быть НЕ c 
                    // ищем первый "рамочный" символ
                    while ((nS21<end) && (ConCharLine[nS21] != c) && !isCharBorder(ConCharLine[nS21]))
                        nS21 ++;
                    if (nS21<end && ConCharLine[nS21]==c) {
                        lbS2 = true; nS22 = nS21;
                        while ((nS22 < end) && (ConCharLine[nS22+1] == c))
                            nS22 ++;
                    }
                }
            } HEAPVAL
            // а вот для пробелов - когда их более одного
            /*else if (c==L' ' && j<end && ConCharLine[j+1]==L' ')
            {
                lbS1 = true; nS11 = nS12 = j;
                while ((nS12 < end) && (ConCharLine[nS12+1] == c))
                    nS12 ++;
            }*/


            //SetTextColor(hDC, pColors[attrFore]);
            SetTextColor(hDC, attr.crForeColor);

            // корректировка положения вертикальной рамки (Coord.X>0)
            if (bProportional && j)
            {
                HEAPVAL;
                DWORD nPrevX = ConCharXLine[j-1];
				// Рамки (их вертикальные части) и полосы прокрутки;
				// Символом } фар помечает имена файлов вылезшие за пределы колонки...
				// Если сверху или снизу на той же позиции рамки (или тот же '}')
				// корректируем позицию
				bool bBord = isCharBorderVertical(c);
				TODO("Пока не будет учтено, что поверх рамок может быть другой диалог!");
				bool bFrame = false; // mpn_ConAttrEx[row*TextWidth+j].bDialogVBorder;
                if (bFrame || bBord || isCharScroll(c) || (isFilePanel && c==L'}')) {
                    //2009-04-21 было (j-1) * nFontWidth;
                    //ConCharXLine[j-1] = j * nFontWidth;
					bool bMayBeFrame = true;
					if (!bBord && !bFrame)
					{
						int R;
						wchar_t prevC;
						for (R = (row-1); bMayBeFrame && !bBord && R>=0; R--) {
							prevC = mpsz_ConChar[R*TextWidth+j];
							bBord = isCharBorderVertical(prevC);
							bMayBeFrame = (bBord || isCharScroll(prevC) || (isFilePanel && prevC==L'}'));
						}
						for (R = (row+1); bMayBeFrame && !bBord && R < (int)TextHeight; R++) {
							prevC = mpsz_ConChar[R*TextWidth+j];
							bBord = isCharBorderVertical(prevC);
							bMayBeFrame = (bBord || isCharScroll(prevC) || (isFilePanel && prevC==L'}'));
						}
					}
					if (bMayBeFrame)
						ConCharXLine[j-1] = j * nFontWidth;
                }
				//else if (isFilePanel && c==L'}') {
				//    if ((row>=2 && isCharBorderVertical(mpsz_ConChar[TextWidth+j]))
				//        && (((UINT)row)<=(TextHeight-4)))
				//        //2009-04-21 было (j-1) * nFontWidth;
				//        ConCharXLine[j-1] = j * nFontWidth;
				//    //row = TextHeight - 1;
				//}
				HEAPVAL;

                if (nPrevX < ConCharXLine[j-1]) {
                    // Требуется зарисовать пропущенную область. пробелами что-ли?

                    RECT rect;
                    rect.left = nPrevX;
                    rect.top = pos;
                    rect.right = ConCharXLine[j-1];
                    rect.bottom = rect.top + nFontHeight;

                    if (gbNoDblBuffer) GdiFlush();
                    // Если не отрисовка фона картинкой
                    if (! (drawImage && ISBGIMGCOLOR(attr.nBackIdx))) {

						//BYTE PrevAttrFore = attrFore, PrevAttrBack = attrBack;
						const CharAttr PrevAttr = ConAttrLine[j-1];
						WCHAR PrevC = ConCharLine[j-1];

						//GetCharAttr(PrevC, PrevAttr, PrevC, PrevAttrFore, PrevAttrBack);
						//GetCharAttr(PrevAttr, PrevAttrFore, PrevAttrBack, NULL);
						//if (GetCharAttr(PrevC, PrevAttr, PrevC, PrevAttrFore, PrevAttrBack))
						//	isUnicode = true;

						// Если текущий символ - вертикальная рамка, а предыдущий символ - рамка
						// нужно продлить рамку до текущего символа
						if (isCharBorderVertical(c) && isCharBorder(PrevC)) {
							//SetBkColor(hDC, pColors[attrBack]);
							SetBkColor(hDC, attr.crBackColor);
							wchar_t *pchBorder = (c == ucBoxDblDownLeft || c == ucBoxDblUpLeft 
								|| c == ucBoxSinglDownDblHorz || c == ucBoxSinglUpDblHorz || c == ucBoxDblVertLeft
								|| c == ucBoxDblVertHorz) ? ms_HorzDbl : ms_HorzSingl;
							int nCnt = ((rect.right - rect.left) / CharWidth(pchBorder[0]))+1;
							if (nCnt > MAX_SPACES) {
								_ASSERTE(nCnt<=MAX_SPACES);
								nCnt = MAX_SPACES;
							}
							//UINT nFlags = ETO_CLIPPED; // || ETO_OPAQUE;
							//ExtTextOut(hDC, rect.left, rect.top, nFlags, &rect, Spaces, min(nSpaceCount, nCnt), 0);
							//if (! (drawImage && ISBGIMGCOLOR(attr.nBackIdx)))
							//	SetBkColor(hDC, pColors[attrBack]);
							//else if (drawImage)
							//	BlitPictureTo(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
							UINT nFlags = ETO_CLIPPED | ETO_OPAQUE;
							//wmemset(Spaces, chBorder, nCnt);
							if (bFixFarBorders)
								SelectFont(hFont2);
							SetTextColor(hDC, PrevAttr.crForeColor);
							ExtTextOut(hDC, rect.left, rect.top, nFlags, &rect, pchBorder, nCnt, 0);
							SetTextColor(hDC, attr.crForeColor); // Вернуть

						} else {
							HBRUSH hbr = PartBrush(L' ', attr.crBackColor, attr.crForeColor);
							FillRect(hDC, &rect, hbr);

						}

                    } else if (drawImage) {
                        BlitPictureTo(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
                    } HEAPVAL
                    if (gbNoDblBuffer) GdiFlush();
                }
            }

            ConCharXLine[j] = (j ? ConCharXLine[j-1] : 0)+CharWidth(c);
            HEAPVAL


            if (bForceMonospace)
            {
                HEAPVAL
                //SetBkColor(hDC, pColors[attrBack]);
                SetBkColor(hDC, attr.crBackColor);

                j2 = j + 1;

                if (bFixFarBorders && isUnicode)
					SelectFont(hFont2);
				else
                    SelectFont(hFont);


                RECT rect;
                if (!bProportional)
                    rect = MakeRect(j * nFontWidth, pos, j2 * nFontWidth, pos + nFontHeight);
                else {
                    rect.left = j ? ConCharXLine[j-1] : 0;
                    rect.top = pos;
                    rect.right = (TextWidth>(UINT)j2) ? ConCharXLine[j2-1] : Width;
                    rect.bottom = rect.top + nFontHeight;
                }
                UINT nFlags = ETO_CLIPPED | ((drawImage && ISBGIMGCOLOR(attr.nBackIdx)) ? 0 : ETO_OPAQUE);
                int nShift = 0;

                HEAPVAL
                if (!isSpace && !isUnicodeOrProgress) {
                    ABC abc;
                    //Для НЕ TrueType вызывается wrapper (CharWidth)
                    CharABC(c, &abc);

					if (abc.abcA<0) {
                        // иначе символ наверное налезет на предыдущий?
                        nShift = -abc.abcA;

					} else if (abc.abcB < (UINT)nFontWidth)  {
						int nEdge = (nFontWidth - abc.abcB - 1) >> 1;
						if (abc.abcA < nEdge) {
                            // символ I, i, и др. очень тонкие - рисуем посередине
	                        nShift = nEdge - abc.abcA;
						}
                    }
                }

                HEAPVAL
                if (! (drawImage && ISBGIMGCOLOR(attr.nBackIdx))) {
                    //SetBkColor(hDC, pColors[attrBack]);
                    SetBkColor(hDC, attr.crBackColor);
					// В режиме ForceMonospace символы пытаемся рисовать по центру (если они уже знакоместа)
					// чтобы не оставалось мусора от предыдущей отрисовки - нужно залить знакоместо фоном
					if (nShift>0 && !isSpace && !isProgress) {
						HBRUSH hbr = PartBrush(L' ', attr.crBackColor, attr.crForeColor);
						FillRect(hDC, &rect, hbr);
					}
                } else if (drawImage) {
                    BlitPictureTo(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
                }

                if (nShift>0) {
                    rect.left += nShift;

					// если сделать так - фон вылезает за правую границу (nShift > 0)
					//rect.right += nShift;
                }

                if (gbNoDblBuffer) GdiFlush();
				if (isSpace || (isProgress && bEnhanceGraphics)) {
					HBRUSH hbr = PartBrush(c, attr.crBackColor, attr.crForeColor);
					FillRect(hDC, &rect, hbr);
				} else {
					// Это режим Force monospace
					if (nFontCharSet == OEM_CHARSET && !isUnicode) {
						char cOem = Uni2Oem(c);
						ExtTextOutA(hDC, rect.left, rect.top, nFlags, &rect, &cOem, 1, 0);
					} else {
						ExtTextOut(hDC, rect.left, rect.top, nFlags, &rect, &c, 1, 0);
					}
				}
                if (gbNoDblBuffer) GdiFlush();
                HEAPVAL

            } // end  - if (gSet.isForceMonospace)
			else
			{
				wchar_t* pszDraw = NULL;
				int      nDrawLen = -1;
				bool     bDrawReplaced = false;
				RECT rect;

				//else if (/*gSet.isProportional &&*/ (c==ucSpace || c==ucNoBreakSpace || c==0))
				if (isSpace)
				{
					j2 = j + 1; HEAPVAL;
					while(j2 < end && ConAttrLine[j2] == attr && isCharSpace(ConCharLine[j2]))
						j2++;
					DistributeSpaces(ConCharLine, ConAttrLine, ConCharXLine, j, j2, end);

				}
				else if (!isUnicodeOrProgress)
				{
					j2 = j + 1; HEAPVAL

					#ifndef DRAWEACHCHAR
					// Если этого не делать - в пропорциональных шрифтах буквы будут наезжать одна на другую
					TCHAR ch;
					int nLastNonSpace = -1;
					WARNING("*** сомнение в следующей строчке: (!gSet.isProportional || !isFilePanel || (ch != L'}' && ch!=L' '))");
					TODO("при поиске по строке - обновлять nLastNonSpace");
					while(j2 < end && ConAttrLine[j2] == attr && 
						!isCharBorder(ch = ConCharLine[j2]) 
						&& (!bProportional || !isFilePanel || (ch != L'}' && ch!=L' '))) // корректировка имен в колонках
					{
						ConCharXLine[j2] = (j2 ? ConCharXLine[j2-1] : 0)+CharWidth(ch);
						j2++;
					}
					#endif
					TODO("От хвоста - отбросить пробелы (если если есть nLastNonSpace)");

					SelectFont(hFont);
					HEAPVAL
				}
				else //Border and specials
				{
					j2 = j + 1; HEAPVAL

					if (bEnhanceGraphics && isProgress) {
						TCHAR ch;
						ch = c; // Графическая отрисовка прокрутки и прогресса
						while(j2 < end && ConAttrLine[j2] == attr && ch == ConCharLine[j2+1])
						{
							ConCharXLine[j2] = (j2 ? ConCharXLine[j2-1] : 0)+CharWidth(ch);
							j2++;
						}
					} else
					if (!bFixFarBorders)
					{
						TCHAR ch;
						while(j2 < end && ConAttrLine[j2] == attr && isCharBorder(ch = ConCharLine[j2]))
						{
							ConCharXLine[j2] = (j2 ? ConCharXLine[j2-1] : 0)+CharWidth(ch);
							j2++;
						}
					}
					else
					{
						//TCHAR ch;
						//WARNING("Тут обламывается на ucBoxDblVert в первой позиции. Ее ширину ставит в ...");

						// Для отрисовки рамок (ucBoxDblHorz) за один проход
						if (c == ucBoxDblHorz || c == ucBoxSinglHorz)
						{
							while(j2 < end && ConAttrLine[j2] == attr && c == ConCharLine[j2])
								j2++;
						}
						DistributeSpaces(ConCharLine, ConAttrLine, ConCharXLine, j, j2, end);
						int nBorderWidth = CharWidth(c);
						rect.left = j ? ConCharXLine[j-1] : 0;
						rect.right = (TextWidth>(UINT)j2) ? ConCharXLine[j2-1] : Width;
						int nCnt = (rect.right - rect.left + (nBorderWidth>>1)) / nBorderWidth;
						if (nCnt > (j2 - j)) {
							if (c==ucBoxDblHorz || c==ucBoxSinglHorz) {
								_ASSERTE(nCnt <= MAX_SPACES);
								if (nCnt > MAX_SPACES) nCnt = MAX_SPACES;
								bDrawReplaced = true;
								nDrawLen = nCnt;
								pszDraw = (c==ucBoxDblHorz) ? ms_HorzDbl : ms_HorzSingl;
							} else {
								#ifdef _DEBUG
								static bool bShowAssert = true;
								if (bShowAssert) {
									_ASSERTE(c==ucBoxDblHorz || c==ucBoxSinglHorz);
								}
								#endif
							}
						}
						//while(j2 < end && ConAttrLine[j2] == attr && 
						//    isCharBorder(ch = ConCharLine[j2]) && ch == ConCharLine[j2+1])
						//{
						//    ConCharXLine[j2] = (j2 ? ConCharXLine[j2-1] : 0)+CharWidth(ch);
						//    j2++;
						//}
					}

					SelectFont(bFixFarBorders ? hFont2 : hFont);
					HEAPVAL
				}


				if (!pszDraw) {
					pszDraw = ConCharLine + j;
					nDrawLen = j2 - j;
				}


				//if (!gSet.isProportional) {
				//	TODO("Что-то как-то... ведь положения уже вроде расчитали?");
				//  rect = MakeRect(j * nFontWidth, pos, j2 * nFontWidth, pos + nFontHeight);
				//} else {
                rect.left = j ? ConCharXLine[j-1] : 0;
                rect.top = pos;
                rect.right = (TextWidth>(UINT)j2) ? ConCharXLine[j2-1] : Width;
                rect.bottom = rect.top + nFontHeight;
                //}

                HEAPVAL
				BOOL lbImgDrawn = FALSE;
                if (! (drawImage && ISBGIMGCOLOR(attr.nBackIdx))) {
                    //SetBkColor(hDC, pColors[attrBack]);
                    SetBkColor(hDC, attr.crBackColor);
				} else if (drawImage) {
                    BlitPictureTo(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
					lbImgDrawn = TRUE;
				}

				WARNING("ETO_CLIPPED вроде вообще не нарисует символ, если его часть не влезает?");
                UINT nFlags = ETO_CLIPPED | ((drawImage && ISBGIMGCOLOR(attr.nBackIdx)) ? 0 : ETO_OPAQUE);

                HEAPVAL
                if (gbNoDblBuffer) GdiFlush();
                if (isProgress && bEnhanceGraphics) {
					HBRUSH hbr = PartBrush(c, attr.crBackColor, attr.crForeColor);
					FillRect(hDC, &rect, hbr);
                } else
                if (/*gSet.isProportional &&*/ isSpace/*c == ' '*/) {
                    //int nCnt = ((rect.right - rect.left) / CharWidth(L' '))+1;
                    //Ext Text Out(hDC, rect.left, rect.top, nFlags, &rect, Spaces, nCnt, 0);
					TODO("Проверить, что будет если картинка МЕНЬШЕ по ширине чем область отрисовки");
					if (!lbImgDrawn) {
						HBRUSH hbr = PartBrush(L' ', attr.crBackColor, attr.crForeColor);
						FillRect(hDC, &rect, hbr);
					}
				} else {

					// В целях оптимизации вынесем проверку bProportional за пределы цикла
					/*if (bProportional) {

						if (!isSpace && !isUnicodeOrProgress) {
							ABC abc;
							//Для НЕ TrueType вызывается wrapper (CharWidth)
							CharABC(c, &abc);

							if (abc.abcA<0) {
								// иначе символ наверное налезет на предыдущий?
								nShift = -abc.abcA;

							} else if (abc.abcB < (UINT)nFontWidth)  {
								int nEdge = (nFontWidth - abc.abcB - 1) >> 1;
								if (abc.abcA < nEdge) {
									// символ I, i, и др. очень тонкие - рисуем посередине
									nShift = nEdge - abc.abcA;
								}
							}
						}

					} else {
					}*/


					if (nFontCharSet == OEM_CHARSET && !isUnicode)
					{
						if (nDrawLen > (int)TextWidth) {
							_ASSERTE(nDrawLen <= (int)TextWidth);
							nDrawLen = TextWidth;
						}
						WideCharToMultiByte(CP_OEMCP, 0, pszDraw, nDrawLen, tmpOem, TextWidth, 0, 0);
						if (!bProportional)
							for (int idx = 0; idx < nDrawLen; idx++) {
								WARNING("BUGBUG: что именно нужно передавать для получения ширины OEM символа?");
								nDX[idx] = CharWidth(tmpOem[idx]);
							}
						ExtTextOutA(hDC, rect.left, rect.top, nFlags,
							&rect, tmpOem, nDrawLen, bProportional ? 0 : nDX);
					}
					else
					{
						if (nDrawLen == 1) // support visualizer change
						ExtTextOut(hDC, rect.left, rect.top, nFlags, &rect,
							&c/*ConCharLine + j*/, 1, 0);
						else {
							if (!bProportional)
								for (int idx = 0, n = nDrawLen; n; idx++, n--)
									nDX[idx] = CharWidth(pszDraw[idx]);
							// nDX это сдвиги до начала следующего символа, с начала предыдущего
							ExtTextOut(hDC, rect.left, rect.top, nFlags, &rect,
								pszDraw, nDrawLen, bProportional ? 0 : nDX);
						}
					}
				}
                if (gbNoDblBuffer) GdiFlush();
                HEAPVAL
            }

			DUMPDC(L"F:\\vcon.png");
        

			//-- во избежание проблем с пропорциональными шрифтами и ClearType
			//-- рисуем измененные строки целиком
			//// skip next not changed symbols again
			//if (skipNotChanged)
			//{
			//    HEAPVAL
			//	wchar_t ch;
			//    // skip the same except spaces
			//    while(j2 < end && (ch = ConCharLine[j2]) == ConCharLine2[j2] && ConAttrLine[j2] == ConAttrLine2[j2]
			//		&& (ch != ucSpace && ch != ucNoBreakSpace && ch != 0))
			//    {
			//        ++j2;
			//    }
			//}
        }
    }

	return;
}

void CVirtualConsole::ClearPartBrushes()
{
	_ASSERTE(gConEmu.isMainThread());
	for (UINT br=0; br<MAX_COUNT_PART_BRUSHES; br++) {
		DeleteObject(m_PartBrushes[br].hBrush);
	}
	memset(m_PartBrushes, 0, sizeof(m_PartBrushes));
}

HBRUSH CVirtualConsole::PartBrush(wchar_t ch, COLORREF nBackCol, COLORREF nForeCol)
{
	_ASSERTE(gConEmu.isMainThread());
    //std::vector<PARTBRUSHES>::iterator iter = m_PartBrushes.begin();
    //while (iter != m_PartBrushes.end()) {
	PARTBRUSHES *pbr = m_PartBrushes;
	for (UINT br=0; pbr->ch && br<MAX_COUNT_PART_BRUSHES; br++, pbr++) {
		if (pbr->ch == ch && pbr->nBackCol == nBackCol && pbr->nForeCol == nForeCol) {
			_ASSERTE(pbr->hBrush);
            return pbr->hBrush;
		}
    }

    MYRGB clrBack, clrFore, clrMy;
    clrBack.color = nBackCol;
    clrFore.color = nForeCol;

    clrMy.color = clrFore.color; // 100 %

    //#define   PART_75(f,b) ((((int)f) + ((int)b)*3) / 4)
    //#define   PART_50(f,b) ((((int)f) + ((int)b)) / 2)
    //#define   PART_25(f,b) (((3*(int)f) + ((int)b)) / 4)
    //#define   PART_75(f,b) (b + 0.80*(f-b))
    //#define   PART_50(f,b) (b + 0.75*(f-b))
    //#define   PART_25(f,b) (b + 0.50*(f-b))
    #define PART_75(f,b) (b + ((gSet.isPartBrush75*(f-b))>>8))
    #define PART_50(f,b) (b + ((gSet.isPartBrush50*(f-b))>>8))
    #define PART_25(f,b) (b + ((gSet.isPartBrush25*(f-b))>>8))

    if (ch == ucBox75 /* 75% */) {
        clrMy.rgbRed = PART_75(clrFore.rgbRed,clrBack.rgbRed);
        clrMy.rgbGreen = PART_75(clrFore.rgbGreen,clrBack.rgbGreen);
        clrMy.rgbBlue = PART_75(clrFore.rgbBlue,clrBack.rgbBlue);
        clrMy.rgbReserved = 0;
    } else if (ch == ucBox50 /* 50% */) {
        clrMy.rgbRed = PART_50(clrFore.rgbRed,clrBack.rgbRed);
        clrMy.rgbGreen = PART_50(clrFore.rgbGreen,clrBack.rgbGreen);
        clrMy.rgbBlue = PART_50(clrFore.rgbBlue,clrBack.rgbBlue);
        clrMy.rgbReserved = 0;
    } else if (ch == ucBox25 /* 25% */) {
        clrMy.rgbRed = PART_25(clrFore.rgbRed,clrBack.rgbRed);
        clrMy.rgbGreen = PART_25(clrFore.rgbGreen,clrBack.rgbGreen);
        clrMy.rgbBlue = PART_25(clrFore.rgbBlue,clrBack.rgbBlue);
        clrMy.rgbReserved = 0;
    } else if (ch == ucSpace || ch == ucNoBreakSpace /* Non breaking space */ || !ch) {
        clrMy.color = clrBack.color;
    }

    PARTBRUSHES pb;
    pb.ch = ch; pb.nBackCol = nBackCol; pb.nForeCol = nForeCol;
    pb.hBrush = CreateSolidBrush(clrMy.color);

    //m_PartBrushes.push_back(pb);
	*pbr = pb;
    return pb.hBrush;
}

void CVirtualConsole::UpdateCursorDraw(HDC hPaintDC, RECT rcClient, COORD pos, UINT dwSize)
{
    Cursor.x = csbi.dwCursorPosition.X;
    Cursor.y = csbi.dwCursorPosition.Y;

    int CurChar = pos.Y * TextWidth + pos.X;
    if (CurChar < 0 || CurChar>=(int)(TextWidth * TextHeight)) {
        return; // может быть или глюк - или размер консоли был резко уменьшен и предыдущая позиция курсора пропала с экрана
    }
    if (!ConCharX) {
        return; // защита
    }
    COORD pix;
    
    pix.X = pos.X * nFontWidth;
    pix.Y = pos.Y * nFontHeight;
    if (pos.X && ConCharX[CurChar-1])
        pix.X = ConCharX[CurChar-1];

        
    RECT rect;
    
    bool bForeground = gConEmu.isMeForeground();
    
	if (!bForeground && gSet.isCursorBlockInactive) {
		dwSize = 100;
        rect.left = pix.X; /*Cursor.x * nFontWidth;*/
        rect.right = pix.X + nFontWidth; /*(Cursor.x+1) * nFontWidth;*/ //TODO: а ведь позиция следующего символа известна!
        rect.bottom = (pos.Y+1) * nFontHeight;
        rect.top = (pos.Y * nFontHeight) /*+ 1*/;
		
	} else
    if (!gSet.isCursorV)
    {
        if (!gSet.isMonospace) {
            rect.left = pix.X; /*Cursor.x * nFontWidth;*/
            rect.right = pix.X + nFontWidth; /*(Cursor.x+1) * nFontWidth;*/ //TODO: а ведь позиция следующего символа известна!
        } else {
            rect.left = pos.X * nFontWidth;
            rect.right = (pos.X+1) * nFontWidth;
        }
        //rect.top = (Cursor.y+1) * nFontHeight - MulDiv(nFontHeight, cinf.dwSize, 100);
        rect.bottom = (pos.Y+1) * nFontHeight;
        rect.top = (pos.Y * nFontHeight) /*+ 1*/;
        //if (cinf.dwSize<50)
        int nHeight = 0;
        if (dwSize) {
            nHeight = MulDiv(nFontHeight, dwSize, 100);
            if (nHeight < HCURSORHEIGHT) nHeight = HCURSORHEIGHT;
        }
        //if (nHeight < HCURSORHEIGHT) nHeight = HCURSORHEIGHT;
        rect.top = max(rect.top, (rect.bottom-nHeight));
    }
    else
    {
        if (!gSet.isMonospace) {
            rect.left = pix.X; /*Cursor.x * nFontWidth;*/
            //rect.right = rect.left/*Cursor.x * nFontWidth*/ //TODO: а ведь позиция следующего символа известна!
            //  + klMax(1, MulDiv(nFontWidth, cinf.dwSize, 100) 
            //  + (cinf.dwSize > 10 ? 1 : 0));
        } else {
            rect.left = pos.X * nFontWidth;
            //rect.right = Cursor.x * nFontWidth
            //  + klMax(1, MulDiv(nFontWidth, cinf.dwSize, 100) 
            //  + (cinf.dwSize > 10 ? 1 : 0));
        }
        rect.top = pos.Y * nFontHeight;
        int nR = (!gSet.isMonospace && ConCharX[CurChar]) // правая граница
            ? ConCharX[CurChar] : ((pos.X+1) * nFontWidth);
        //if (cinf.dwSize>=50)
        //  rect.right = nR;
        //else
        //  rect.right = min(nR, (rect.left+VCURSORWIDTH));
        int nWidth = 0;
        if (dwSize) {
            nWidth = MulDiv((nR - rect.left), dwSize, 100);
            if (nWidth < VCURSORWIDTH) nWidth = VCURSORWIDTH;
        }
        rect.right = min(nR, (rect.left+nWidth));
        //rect.right = rect.left/*Cursor.x * nFontWidth*/ //TODO: а ведь позиция следующего символа известна!
        //      + klMax(1, MulDiv(nFontWidth, cinf.dwSize, 100) 
        //      + (cinf.dwSize > 10 ? 1 : 0));
        rect.bottom = (pos.Y+1) * nFontHeight;
    }
    
    
	//if (!gSet.isCursorBlink) {
	//	gConEmu.m_Child.SetCaret ( 1, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top );
	//	return;
	//} else {
	//	gConEmu.m_Child.SetCaret ( -1 ); // Если был создан системный курсор - он разрушится
	//}

	rect.left += rcClient.left;
	rect.right += rcClient.left;
	rect.top += rcClient.top;
	rect.bottom += rcClient.top;

	// Если курсор занимает более 40% площади - принудительно включим 
	// XOR режим, иначе (тем более при немигающем) курсор закрывает 
	// весь символ и его не видно
	bool bCursorColor = gSet.isCursorColor || (dwSize >= 40);

	// Теперь в rect нужно отобразить курсор (XOR'ом попробуем?)
	if (bCursorColor)
	{
		HBRUSH hBr = CreateSolidBrush(0xC0C0C0);
		HBRUSH hOld = (HBRUSH)SelectObject ( hPaintDC, hBr );

		if (bForeground || !gSet.isCursorBlockInactive) {
			BitBlt(hPaintDC, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, hDC, 0,0,
				PATINVERT);
		} else {
			// Когда CE не в фокусе - можно просто прямоугольник рисовать
			BitBlt(hPaintDC, rect.left, rect.top, 1, rect.bottom-rect.top, hDC, 0,0,
				PATINVERT);
			BitBlt(hPaintDC, rect.left, rect.top, rect.right-rect.left, 1, hDC, 0,0,
				PATINVERT);
			BitBlt(hPaintDC, rect.right-1, rect.top, 1, rect.bottom-rect.top, hDC, 0,0,
				PATINVERT);
			BitBlt(hPaintDC, rect.left, rect.bottom-1, rect.right-rect.left, 1, hDC, 0,0,
				PATINVERT);
		}

		SelectObject ( hPaintDC, hOld );
		DeleteObject ( hBr );

		return;
    }


	//lbDark = Cursor.foreColorNum < 5; // Было раньше
	//BOOL lbIsProgr = isCharProgress(Cursor.ch[0]);
    
	BOOL lbDark = FALSE;
	DWORD clr = (Cursor.ch != ucBox100 && Cursor.ch != ucBox75) ? Cursor.bgColor : Cursor.foreColor;
	BYTE R = (clr & 0xFF);
	BYTE G = (clr & 0xFF00) >> 8;
	BYTE B = (clr & 0xFF0000) >> 16;
	lbDark = (R <= 0xC0) && (G <= 0xC0) && (B <= 0xC0);
	clr = lbDark ? mp_Colors[15] : mp_Colors[0];

	HBRUSH hBr = CreateSolidBrush(clr);
	FillRect(hPaintDC, &rect, hBr);
	DeleteObject ( hBr );
}

void CVirtualConsole::UpdateCursor(bool& lRes)
{
    //------------------------------------------------------------------------
    ///| Drawing cursor |/////////////////////////////////////////////////////
    //------------------------------------------------------------------------
    Cursor.isVisiblePrevFromInfo = cinf.bVisible;

    BOOL lbUpdateTick = FALSE;
    bool bForeground = gConEmu.isMeForeground();
    bool bConActive = gConEmu.isActive(this); // а тут именно Active, т.к. курсор должен мигать в активной консоли
    //if (bConActive) {
    //	bForeground = gConEmu.isMeForeground();
    //}

    // Если курсор (в консоли) видим, и находится в видимой области (при прокрутке)
    if (cinf.bVisible && isCursorValid)
    {
		if (!gSet.isCursorBlink || !bForeground) {
            Cursor.isVisible = true; // Видим всегда (даже в неактивной консоли), не мигает
            if (Cursor.isPrevBackground == bForeground) {
            	lRes = true;
            }
		} else {
            // Смена позиции курсора - его нужно обновить, если окно активно
            if ((Cursor.x != csbi.dwCursorPosition.X) || (Cursor.y != csbi.dwCursorPosition.Y)) 
            {
                Cursor.isVisible = bConActive;
                if (Cursor.isVisible) lRes = true; //force, pos changed
                lbUpdateTick = TRUE;
            } else
            // Настало время мигания
            if ((GetTickCount() - Cursor.nLastBlink) > Cursor.nBlinkTime)
            {
            	if (Cursor.isPrevBackground && bForeground) {
            		// после "неактивного" курсора - сразу рисовать активный, только потом - мигать
            		Cursor.isVisible = true;
            		lRes = true;
            	} else {
                	Cursor.isVisible = bConActive && !Cursor.isVisible;
            	}
                lbUpdateTick = TRUE;
            }
		}
    } else {
        // Иначе - его нужно спрятать (курсор скрыт в консоли, или ушел за границы экрана)
        Cursor.isVisible = false;
    }

    // Смена видимости палки в ConEmu
    if (Cursor.isVisible != Cursor.isVisiblePrev) {
        lRes = true;
        lbUpdateTick = TRUE;
    }
    
    if (Cursor.isVisible)
    	Cursor.isPrevBackground = !bForeground;

    // Сохраним новое положение
    //Cursor.x = csbi.dwCursorPosition.X;
    //Cursor.y = csbi.dwCursorPosition.Y;
    Cursor.isVisiblePrev = Cursor.isVisible;

    if (lbUpdateTick)
        Cursor.nLastBlink = GetTickCount();
}


LPVOID CVirtualConsole::Alloc(size_t nCount, size_t nSize)
{
    #ifdef _DEBUG
    //HeapValidate(mh_Heap, 0, NULL);
    #endif
    size_t nWhole = nCount * nSize;
    LPVOID ptr = HeapAlloc ( mh_Heap, HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY, nWhole );
    #ifdef _DEBUG
    //HeapValidate(mh_Heap, 0, NULL);
    #endif
    return ptr;
}

void CVirtualConsole::Free(LPVOID ptr)
{
    if (ptr && mh_Heap) {
        #ifdef _DEBUG
        //HeapValidate(mh_Heap, 0, NULL);
        #endif
        HeapFree ( mh_Heap, 0, ptr );
        #ifdef _DEBUG
        //HeapValidate(mh_Heap, 0, NULL);
        #endif
    }
}


// hdc - это DC родительского окна (ghWnd)
// rcClient - это место, куда нужно "положить" битмап. может быть произвольным!
void CVirtualConsole::Paint(HDC hPaintDc, RECT rcClient)
{
    if (!ghWndDC)
        return;
	_ASSERTE(hPaintDc);
	_ASSERTE(rcClient.left!=rcClient.right && rcClient.top!=rcClient.bottom);

//#ifdef _DEBUG
//    if (this) {
//        if (!mp_RCon || !mp_RCon->isPackets()) {
//        	DEBUGSTRDRAW(L"*** Painting ***\n");
//        } else {
//            DEBUGSTRDRAW(L"*** Painting (!!! Non processed packets are queued !!!) ***\n");
//        }
//    }
//#endif
    
	BOOL lbSimpleBlack = FALSE;
	if (!this) 
		lbSimpleBlack = TRUE;
	else if (!mp_RCon)
		lbSimpleBlack = TRUE;
	else if (!mp_RCon->ConWnd())
		lbSimpleBlack = TRUE;

	//else if (!mpsz_ConChar || !mpn_ConAttrEx)
	//	lbSimpleBlack = TRUE;
    if (lbSimpleBlack) {
        // Залить цветом 0
        #ifdef _DEBUG
            int nBackColorIdx = 2;
        #else
            int nBackColorIdx = 0;
        #endif
		COLORREF *pColors = gSet.GetColors();
        HBRUSH hBr = CreateSolidBrush(pColors[nBackColorIdx]);
        //RECT rcClient; GetClientRect(ghWndDC, &rcClient);
        //PAINTSTRUCT ps;
        //HDC hPaintDc = BeginPaint(ghWndDC, &ps);
        #ifndef SKIP_ALL_FILLRECT
        FillRect(hPaintDc, &rcClient, hBr);
        #endif
		HFONT hOldF = (HFONT)SelectObject(hPaintDc, gSet.mh_Font[0]);
		LPCWSTR pszStarting = L"Initializing ConEmu.";
		if (this) {
			if (mp_RCon)
				pszStarting = mp_RCon->GetConStatus();
		}
		UINT nFlags = ETO_CLIPPED;
		SetTextColor(hPaintDc, pColors[7]);
		SetBkColor(hPaintDc, pColors[0]);
		ExtTextOut(hPaintDc, rcClient.left, rcClient.top, nFlags, &rcClient, pszStarting, wcslen(pszStarting), 0);
		SelectObject(hPaintDc, hOldF);
        DeleteObject(hBr);
        //EndPaint(ghWndDC, &ps);
        return;
    }
    
    if (gConEmu.isActive(this))
    	gSet.Performance(tPerfFPS, TRUE); // считается по своему

	mb_InPaintCall = TRUE;
	Update(mb_RequiredForceUpdate);
	mb_InPaintCall = FALSE;

    BOOL lbExcept = FALSE;
    RECT client = rcClient;
    //PAINTSTRUCT ps;
    //HDC hPaintDc = NULL;


    //GetClientRect(ghWndDC, &client);

    if (!gConEmu.isNtvdm()) {
        // после глюков с ресайзом могут быть проблемы с размером окна DC
        if ((client.right-client.left) < (LONG)Width || (client.bottom-client.top) < (LONG)Height)
		{
			WARNING("Зацикливается. Вызывает Paint, который вызывает OnSize. В итоге - 100% загрузки проц.");
            gConEmu.OnSize(-1); // Только ресайз дочерних окон
		}
    }
    
    MSectionLock S; //&csDC, &ncsTDC);

	//RECT rcUpdateRect = {0};
	//BOOL lbInval = GetUpdateRect(ghWndDC, &rcUpdateRect, FALSE);

	//if (lbInval)
	//  hPaintDc = BeginPaint(ghWndDC, &ps);
	//else
	//	hPaintDc = GetDC(ghWndDC);

    // Если окно больше готового DC - залить края (справа/снизу) фоновым цветом
    HBRUSH hBr = NULL;
    if (((ULONG)(client.right-client.left)) > Width) {
        if (!hBr) hBr = CreateSolidBrush(mp_Colors[mn_BackColorIdx]);
        RECT rcFill = MakeRect(client.left+Width, client.top, client.right, client.bottom);
        #ifndef SKIP_ALL_FILLRECT
        FillRect(hPaintDc, &rcFill, hBr);
        #endif
        client.right = client.left+Width;
    }
    if (((ULONG)(client.bottom-client.top)) > Height) {
        if (!hBr) hBr = CreateSolidBrush(mp_Colors[mn_BackColorIdx]);
        RECT rcFill = MakeRect(client.left, client.top+Height, client.right, client.bottom);
        #ifndef SKIP_ALL_FILLRECT
        FillRect(hPaintDc, &rcFill, hBr);
        #endif
        client.bottom = client.top+Height;
    }
    if (hBr) { DeleteObject(hBr); hBr = NULL; }


    BOOL lbPaintLocked = FALSE;
    if (!mb_PaintRequested) { // Если Paint вызыван НЕ из Update (а например ресайз, или над нашим окошком что-то протащили).
        if (S.Lock(&csDC, 200)) // но по таймауту, чтобы не повисли ненароком
            mb_PaintLocked = lbPaintLocked = TRUE;
    }
    
    //bool bFading = false;
	bool lbLeftExists = (m_LeftPanelView.hWnd && IsWindowVisible(m_LeftPanelView.hWnd));
	if (lbLeftExists && !mb_LeftPanelRedraw) {
		DWORD n = GetWindowLong(m_LeftPanelView.hWnd, 16*4);
		if (n != (isFade ? 2 : 1)) mb_LeftPanelRedraw = TRUE;
	}
	bool lbRightExists = (m_RightPanelView.hWnd && IsWindowVisible(m_RightPanelView.hWnd));
	if (lbRightExists && !mb_RightPanelRedraw) {
		DWORD n = GetWindowLong(m_RightPanelView.hWnd, 16*4);
		if (n != (isFade ? 2 : 1)) mb_RightPanelRedraw = TRUE;
	}


	if (mb_LeftPanelRedraw) {
		UpdatePanelView(TRUE); mb_LeftPanelRedraw = FALSE;
		if (lbLeftExists) {
			InvalidateRect(m_LeftPanelView.hWnd, NULL, FALSE);
		}
	}
	if (mb_RightPanelRedraw) {
		UpdatePanelView(FALSE); mb_RightPanelRedraw = FALSE;
		if (lbRightExists) {
			InvalidateRect(m_RightPanelView.hWnd, NULL, FALSE);
		}
	}

    // Собственно, копирование готового bitmap
    if (!gbNoDblBuffer) {
        // Обычный режим
		if (gSet.isAdvLogging>=3) mp_RCon->LogString("Blitting to Display");

		/*if (gSet.isFadeInactive && !gConEmu.isMeForeground()) {
			// Fade-effect когда CE не в фокусе
			DWORD dwFadeColor = gSet.nFadeInactiveMask; //0xC0C0C0;
			// пробовал средний между Colors[7] & Colors[8] - некрасиво
			
			bFading = true;
			
			HBRUSH hBr = CreateSolidBrush(dwFadeColor);
			HBRUSH hOld = (HBRUSH)SelectObject ( hPaintDc, hBr );
			DWORD dwEffect = MERGECOPY;

			BitBlt(hPaintDc, client.left, client.top, client.right-client.left, client.bottom-client.top, hDC, 0, 0,
				dwEffect);

			SelectObject ( hPaintDc, hOld );
			DeleteObject ( hBr );
				
		} else {*/

    	BitBlt(hPaintDc, client.left, client.top, client.right-client.left, client.bottom-client.top, hDC, 0, 0,
    		SRCCOPY);

        /*}*/

		if (gSet.isUserScreenTransparent) {
			if (CheckTransparentRgn())
				gConEmu.UpdateWindowRgn();
		}
        
    } else {
        GdiSetBatchLimit(1); // отключить буферизацию вывода для текущей нити

        GdiFlush();
        // Рисуем сразу на канвасе, без буферизации
		// Это для отладки отрисовки, поэтому недопустимы табы и прочее
		_ASSERTE(gSet.isTabs == 0);
        Update(true, &hPaintDc);
    }
    
    //mb_LastFadeFlag = bFading;

    S.Unlock();
    
    if (lbPaintLocked)
        mb_PaintLocked = FALSE;

        // Палку курсора теперь рисуем только в окне
        //UpdateCursor(hPaintDc);

        if (gbNoDblBuffer) GdiSetBatchLimit(0); // вернуть стандартный режим

        

        if (Cursor.isVisible && cinf.bVisible && isCursorValid)
        {
			if (mpsz_ConChar && mpsz_ConChar)
			{
				HFONT hOldFont = (HFONT)SelectObject(hPaintDc, gSet.mh_Font[0]);

				MSectionLock SCON; SCON.Lock(&csCON);

				if (mpsz_ConChar && mpn_ConAttrEx)
				{
					int CurChar = csbi.dwCursorPosition.Y * TextWidth + csbi.dwCursorPosition.X;
					//Cursor.ch[1] = 0;
					//CVirtualConsole* p = this;
					//GetCharAttr(mpsz_ConChar[CurChar], mpn_ConAttr[CurChar], Cursor.ch[0], Cursor.foreColorNum, Cursor.bgColorNum);
					Cursor.ch = mpsz_ConChar[CurChar];
					//GetCharAttr(mpn_ConAttr[CurChar], Cursor.foreColorNum, Cursor.bgColorNum, NULL);
					//Cursor.foreColor = pColors[Cursor.foreColorNum];
					//Cursor.bgColor = pColors[Cursor.bgColorNum];
					Cursor.foreColor = mpn_ConAttrEx[CurChar].crForeColor;
					Cursor.bgColor = mpn_ConAttrEx[CurChar].crBackColor;
				}

				UpdateCursorDraw(hPaintDc, rcClient, csbi.dwCursorPosition, cinf.dwSize);

				Cursor.isVisiblePrev = Cursor.isVisible;

				SelectObject(hPaintDc, hOldFont);

				SCON.Unlock();
			}
        }

#ifdef _DEBUG
	if (mp_RCon) {
		#ifdef DEBUGDRAW_RCONPOS
		if (GetKeyState(DEBUGDRAW_RCONPOS) & 1) {
			// Прямоугольник, соответвующий положения окна RealConsole
			HWND hConWnd = mp_RCon->hConWnd;
			RECT rcCon; GetWindowRect(hConWnd, &rcCon);
			MapWindowPoints(NULL, ghWndDC, (LPPOINT)&rcCon, 2);
			SelectObject(hPaintDc, GetStockObject(WHITE_PEN));
			SelectObject(hPaintDc, GetStockObject(HOLLOW_BRUSH));
			Rectangle(hPaintDc, rcCon.left, rcCon.top, rcCon.right, rcCon.bottom);
		}
		#endif

		#ifdef DEBUGDRAW_DIALOGS
		if (GetKeyState(DEBUGDRAW_DIALOGS) & 1) {
			// Прямоугольники найденных диалогов
			SMALL_RECT rcFound[MAX_DETECTED_DIALOGS]; DWORD nDlgFlags[MAX_DETECTED_DIALOGS];
			int nFound = mp_RCon->GetDetectedDialogs(MAX_DETECTED_DIALOGS, rcFound, nDlgFlags);
			HPEN hFrame = CreatePen(PS_SOLID, 1, RGB(255,0,255));
			HPEN hPanel = CreatePen(PS_SOLID, 2, RGB(255,255,0));
			HPEN hActiveMenu = (HPEN)GetStockObject(WHITE_PEN);
			HPEN hOldPen = (HPEN)SelectObject(hPaintDc, hFrame);
			HBRUSH hOldBr = (HBRUSH)SelectObject(hPaintDc, GetStockObject(HOLLOW_BRUSH));
			HPEN hRed = CreatePen(PS_SOLID, 1, 255);
			HPEN hDash = CreatePen(PS_DOT, 1, 0xFFFFFF);
			HFONT hSmall = CreateFont(-7,0,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,NONANTIALIASED_QUALITY,DEFAULT_PITCH,L"Small fonts");
			HFONT hOldFont = (HFONT)SelectObject(hPaintDc, hSmall);
			SetTextColor(hPaintDc, 0xFFFFFF);
			SetBkColor(hPaintDc, 0);
			for (int i = 0; i < nFound; i++) {
				int n = 1;
				DWORD nFlags = nDlgFlags[i];
				if (i == (DEBUGDRAW_DIALOGS-1))
					SelectObject(hPaintDc, hRed);
				else if ((nFlags & FR_ACTIVEMENUBAR) == FR_ACTIVEMENUBAR)
					SelectObject(hPaintDc, hActiveMenu);
				else if ((nFlags & (FR_LEFTPANEL|FR_RIGHTPANEL|FR_FULLPANEL)))
					SelectObject(hPaintDc, hPanel);
				else if ((nFlags & FR_HASBORDER))
					SelectObject(hPaintDc, hFrame);
				else {
					SelectObject(hPaintDc, hDash);
					n = 0;
				}
				//
				POINT pt[2];
				pt[0] = ConsoleToClient(rcFound[i].Left, rcFound[i].Top);
				pt[1] = ConsoleToClient(rcFound[i].Right+1, rcFound[i].Bottom+1);
				MapWindowPoints(ghWndDC, ghWnd, pt, 2);
				
				Rectangle(hPaintDc, pt[0].x+n, pt[0].y+n, pt[1].x-n, pt[1].y-n);
				wchar_t szCoord[32]; wsprintf(szCoord, L"%ix%i", rcFound[i].Left, rcFound[i].Top);
				TextOut(hPaintDc, pt[0].x+1, pt[0].y+1, szCoord, wcslen(szCoord));
			}
			SelectObject(hPaintDc, hOldBr);
			SelectObject(hPaintDc, hOldPen);
			SelectObject(hPaintDc, hOldFont);
			DeleteObject(hRed);
			DeleteObject(hPanel);
			DeleteObject(hDash);
			DeleteObject(hFrame);
			DeleteObject(hSmall);
		}
		#endif
	}
#endif

    
    if (lbExcept)
        Box(_T("Exception triggered in CVirtualConsole::Paint"));

  //  if (hPaintDc && ghWndDC) {
		//if (lbInval)
		//	EndPaint(ghWndDC, &ps);
		//else
		//	ReleaseDC(ghWndDC, hPaintDc);
  //  }
    
    //gSet.Performance(tPerfFPS, FALSE); // Обратный
}

void CVirtualConsole::UpdateInfo()
{
    if (!ghOpWnd || !this)
        return;

    if (!gConEmu.isMainThread()) {
        return;
    }

    TCHAR szSize[128];
	if (!mp_RCon) {
		SetDlgItemText(gSet.hInfo, tConSizeChr, L"(None)");
		SetDlgItemText(gSet.hInfo, tConSizePix, L"(None)");
		
		SetDlgItemText(gSet.hInfo, tPanelLeft, L"(None)");
		SetDlgItemText(gSet.hInfo, tPanelRight, L"(None)");
	} else {
		wsprintf(szSize, _T("%ix%i"), mp_RCon->TextWidth(), mp_RCon->TextHeight());
		SetDlgItemText(gSet.hInfo, tConSizeChr, szSize);
		wsprintf(szSize, _T("%ix%i"), Width, Height);
		SetDlgItemText(gSet.hInfo, tConSizePix, szSize);
		
		RECT rcPanel;
		RCon()->GetPanelRect(FALSE, &rcPanel);
		if (rcPanel.right>rcPanel.left)
			wsprintf(szSize, L"(%i, %i)-(%i, %i), %ix%i", rcPanel.left+1, rcPanel.top+1, rcPanel.right+1, rcPanel.bottom+1, rcPanel.right-rcPanel.left+1, rcPanel.bottom-rcPanel.top+1);
		else
			lstrcpy(szSize, L"<Absent>");
		SetDlgItemText(gSet.hInfo, tPanelLeft, szSize);
		RCon()->GetPanelRect(TRUE, &rcPanel);
		if (rcPanel.right>rcPanel.left)
			wsprintf(szSize, L"(%i, %i)-(%i, %i), %ix%i", rcPanel.left+1, rcPanel.top+1, rcPanel.right+1, rcPanel.bottom+1, rcPanel.right-rcPanel.left+1, rcPanel.bottom-rcPanel.top+1);
		else
			lstrcpy(szSize, L"<Absent>");
		SetDlgItemText(gSet.hInfo, tPanelRight, szSize);
	}
}

void CVirtualConsole::Box(LPCTSTR szText)
{
#ifdef _DEBUG
    _ASSERT(FALSE);
#endif
    MessageBox(NULL, szText, _T("ConEmu"), MB_ICONSTOP);
}

RECT CVirtualConsole::GetRect()
{
    RECT rc;
    if (Width == 0 || Height == 0) { // если консоль еще не загрузилась - прикидываем по размеру GUI окна
        //rc = MakeRect(winSize.X, winSize.Y);
        RECT rcWnd; GetClientRect(ghWnd, &rcWnd);
        RECT rcCon = gConEmu.CalcRect(CER_CONSOLE, rcWnd, CER_MAINCLIENT);
        RECT rcDC = gConEmu.CalcRect(CER_DC, rcCon, CER_CONSOLE);
        rc = MakeRect(rcDC.right, rcDC.bottom);
    } else {
        rc = MakeRect(Width, Height);
    }
    return rc;
}

void CVirtualConsole::OnFontChanged()
{
    if (!this) return;
    mb_RequiredForceUpdate = true;
	//ClearPartBrushes();
}

void CVirtualConsole::OnConsoleSizeChanged()
{
	// По идее эта функция вызывается при ресайзе окна GUI ConEmu
	BOOL lbLast = mb_InPaintCall;
	mb_InPaintCall = TRUE; // чтобы Invalidate лишний раз не дергался
	Update(mb_RequiredForceUpdate);
	mb_InPaintCall = lbLast;
}

POINT CVirtualConsole::ConsoleToClient(LONG x, LONG y)
{
	POINT pt = {0,0};

	if (!this) {
		pt.y = y*gSet.FontHeight();
		pt.x = x*gSet.FontWidth();
		return pt;
	}

	pt.y = y*nFontHeight;
	if (x>0) {
		if (ConCharX && y >= 0 && y < (int)TextHeight && x < (int)TextWidth) {
			pt.x = ConCharX[y*TextWidth + x-1];
			_ASSERTE(pt.x || x==0);
		} else {
			pt.x = x*nFontWidth;
		}
	}

	return pt;
}

// Функция живет здесь, а не в gSet, т.к. здесь мы может более точно опеределить знакоместо
COORD CVirtualConsole::ClientToConsole(LONG x, LONG y)
{
	COORD cr = {0,0};

	if (!this) {
		cr.Y = y/gSet.FontHeight();
		cr.X = x/gSet.FontWidth();
		return cr;
	}

    _ASSERTE(nFontWidth!=0 && nFontHeight!=0);
    
    // Сначала приблизительный пересчет по размерам шрифта
    if (nFontHeight)
        cr.Y = y/nFontHeight;
    if (nFontWidth)
        cr.X = x/nFontWidth;
    // А теперь, если возможно, уточним X координату
    if (x > 0) {
		x++; // иначе сбивается на один пиксел влево
    	if (ConCharX && cr.Y >= 0 && cr.Y < (int)TextHeight) {
    		DWORD* ConCharXLine = ConCharX + cr.Y * TextWidth;
			for (uint i = 0; i < TextWidth; i++, ConCharXLine++) {
				if (((int)*ConCharXLine) >= x) {
					if (cr.X != (int)i) {
						#ifdef _DEBUG
						wchar_t szDbg[120]; wsprintf(szDbg, L"Coord corrected from {%i-%i} to {%i-%i}",
							cr.X, cr.Y, i, cr.Y);
						DEBUGSTRCOORD(szDbg);
						#endif

						cr.X = i;
					}
					break;
				}
			}
    	}
    }
    return cr;
}

// На самом деле не только пробелы, но и ucBoxSinglHorz/ucBoxDblVertHorz
void CVirtualConsole::DistributeSpaces(wchar_t* ConCharLine, CharAttr* ConAttrLine, DWORD* ConCharXLine, const int j, const int j2, const int end)
{
	//WORD attr = ConAttrLine[j];
	//wchar_t ch, c;
	//
	//if ((c=ConCharLine[j]) == ucSpace || c == ucNoBreakSpace || c == 0)
	//{
	//	while(j2 < end && ConAttrLine[j2] == attr
	//		// также и для &nbsp; и 0x00
	//		&& ((ch=ConCharLine[j2]) == ucSpace || ch == ucNoBreakSpace || ch == 0))
	//		j2++;
	//} else
	//if ((c=ConCharLine[j]) == ucBoxSinglHorz || c == ucBoxDblVertHorz)
	//{
	//	while(j2 < end && ConAttrLine[j2] == attr
	//		&& ((ch=ConCharLine[j2]) == ucBoxSinglHorz || ch == ucBoxDblVertHorz))
	//		j2++;
	//} else
	//if (isCharProgress(c=ConCharLine[j]))
	//{
	//	while(j2 < end && ConAttrLine[j2] == attr && (ch=ConCharLine[j2]) == c)
	//		j2++;
	//}

	_ASSERTE(j2 > j && j >= 0);

	// Ширину каждого "пробела" (или символа рамки) будем считать пропорционально занимаемому ИМИ месту

	if (j2>=(int)TextWidth) { // конец строки
		ConCharXLine[j2-1] = Width;
	} else {
		//2009-09-09 Это некорректно. Ширина шрифта рамки может быть больше знакоместа
		//int nBordWidth = gSet.BorderFontWidth(); if (!nBordWidth) nBordWidth = nFontWidth;
		int nBordWidth = nFontWidth;

		// Определить координату конца последовательности
		if (isCharBorderVertical(ConCharLine[j2])) {
			//2009-09-09 а это соответственно не нужно (пока nFontWidth == nBordWidth)
			//ConCharXLine[j2-1] = (j2-1) * nFontWidth + nBordWidth; // или тут [j] должен быть?
			ConCharXLine[j2-1] = j2 * nBordWidth;
		} else {
			TODO("Для пропорциональных шрифтов надо делать как-то по другому!");
			//2009-09-09 а это соответственно не нужно (пока nFontWidth == nBordWidth)
			//ConCharXLine[j2-1] = (j2-1) * nFontWidth + nBordWidth; // или тут [j] должен быть?
			if (!gSet.isMonospace && j > 1) {
				//2009-12-31 нужно плясать от предыдущего символа!
				ConCharXLine[j2-1] = ConCharXLine[j-1] + (j2 - j) * nBordWidth;
			} else {
				ConCharXLine[j2-1] = j2 * nBordWidth;
			}
		}
	}

	if (j2 > (j + 1))
	{
		HEAPVAL
		DWORD n1 = (j ? ConCharXLine[j-1] : 0); // j? если j==0 то тут у нас 10 (правая граница 0го символа в строке)
		DWORD n3 = j2-j; // кол-во символов
		_ASSERTE(n3>0);
		DWORD n2 = ConCharXLine[j2-1] - n1; // выделенная на пробелы ширина
		HEAPVAL

		for (int k=j, l=1; k<(j2-1); k++, l++) {
			#ifdef _DEBUG
			DWORD nn = n1 + (n3 ? klMulDivU32(l, n2, n3) : 0);
			if (nn != ConCharXLine[k])
				ConCharXLine[k] = nn;
			#endif
			ConCharXLine[k] = n1 + (n3 ? klMulDivU32(l, n2, n3) : 0);
			//n1 + (n3 ? klMulDivU32(k-j, n2, n3) : 0);
			HEAPVAL
		}
	}
}

BOOL CVirtualConsole::FindChanges(int &j, int &end, const wchar_t* ConCharLine, const CharAttr* ConAttrLine, const wchar_t* ConCharLine2, const CharAttr* ConAttrLine2)
{
	// Default: j = 0, end = TextWidth;
	return TRUE; // пока строку целиком будем рисовать

	// Если основной шрифт в консоли курсивный, то
	// во избежение обрезания правой части буквы нужно рисовать строку целиком
	TODO("Возможно при включенном ClearType не нужно будет рисовать строку целиком, если переделаю на Transparent отрисовку текста");
	if (gSet.FontItalic() || gSet.FontClearType())
		return TRUE;

	// Если изменений в строке вообще нет
	if (wmemcmp(ConCharLine, ConCharLine2, end) == 0
		&& memcmp(ConAttrLine, ConAttrLine2, end*sizeof(*ConAttrLine)) == 0)
		return FALSE;

	// *) Skip not changed tail symbols. но только если шрифт моноширный
	if (gSet.isMonospace) {
		TODO("OPTIMIZE:");
		while(--end >= 0 && ConCharLine[end] == ConCharLine2[end] && ConAttrLine[end] == ConAttrLine2[end])
		{
			// Если есть стили шрифта (курсив/жирный), то 
			// во избежение обрезания правой части буквы нужно рисовать строку целиком
			if (ConAttrLine[end].nFontIndex) {
				end = TextWidth;
				return TRUE;
			}
		}
		// [%] ClearType, TTF fonts (isProportional может быть и отключен)
		if (end >= 0  // Если есть хоть какие-то изменения
			&& end < (int)(TextWidth - 1) // до конца строки
			&& (ConCharLine[end+1] == ucSpace || ConCharLine[end+1] == ucNoBreakSpace || isCharProgress(ConCharLine[end+1])) // будем отрисовывать все проблеы
			)
		{
			int n = TextWidth - 1;
			while ((end < n) 
				&& (ConCharLine[end+1] == ucSpace || ConCharLine[end+1] == ucNoBreakSpace || isCharProgress(ConCharLine[end+1])))
				end++;
		}
		if (end < j)
			return FALSE;
		++end;
	}

	// *) Skip not changed head symbols.
	while(j < end && ConCharLine[j] == ConCharLine2[j] && ConAttrLine[j] == ConAttrLine2[j])
	{
		// Если есть стили шрифта (курсив/жирный) то 
		// во избежение обрезания правой части буквы нужно рисовать строку целиком
		if (ConAttrLine[j].nFontIndex) {
			j = 0;
			end = TextWidth;
			return TRUE;
		}
		++j;
	}
	// [%] ClearType, proportional fonts
	if (j > 0 // если с начала строки
		&& (ConCharLine[j-1] == ucSpace || ConCharLine[j-1] == ucNoBreakSpace || isCharProgress(ConCharLine[j-1]) // есть пробелы
		|| (gSet.FontItalic() || ConAttrLine[j-1].nFontIndex) ) // Или сейчас курсив/жирный?
		)
	{
		if (gSet.FontItalic() || ConAttrLine[j-1].nFontIndex) {
			j = 0; // с частичной отрисовкой заморачиваться не будем, а то еще ClearType вылезет...
		} else {
			while ((j > 0)
				&& (ConCharLine[j-1] == ucSpace || ConCharLine[j-1] == ucNoBreakSpace || isCharProgress(ConCharLine[j-1])))
			{
				j--;
			}
		}
	}
	if (j >= end) {
		// Для пропорциональных шрифтов сравнение выполняется только с начала строки,
		// поэтому сюда мы вполне можем попасть - значит строка не менялась
		return FALSE;
	}

	return TRUE;
}

HRGN CVirtualConsole::GetExclusionRgn(bool abTestOnly/*=false*/)
{
	if (!gSet.isUserScreenTransparent)
		return NULL;

	if (mp_RCon->GetFarPID(TRUE) == 0)
		return NULL;

	// Возвращает mh_TransparentRgn
	// Сам mh_TransparentRgn формируется в CheckTransparentRgn,
	// который должен вызываться в CVirtualConsole::Paint
	if (abTestOnly && mh_TransparentRgn)
		return (HRGN)1;
	return mh_TransparentRgn;
}

COORD CVirtualConsole::FindOpaqueCell()
{
	COORD cr = {0,0};
	if (this) {
		// По умолчанию - сбросим
		cr.X = cr.Y = -1;
		if (mpsz_ConChar && mpn_ConAttrEx && TextWidth && TextHeight) {
			MSectionLock SCON; SCON.Lock(&csCON);
			CharAttr* pnAttr = mpn_ConAttrEx;

			// Поиск первого непрозрачного
			for (uint y = 0; y < TextHeight; y++) {
				for (uint x = 0; x < TextWidth; x++) {
					if (!pnAttr[x].bTransparent) {
						cr.X = x; cr.Y = y;
						return cr;
					}
				}
				pnAttr += TextWidth;
			}
		}
	}
	return cr;
}

// Показать контекстное меню для ТЕКУЩЕЙ закладки консоли
// ptCur - экранные координаты
void CVirtualConsole::ShowPopupMenu(POINT ptCur)
{
	if (!mh_PopupMenu)
		mh_PopupMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_TABMENU));

	HMENU hPopup = mh_PopupMenu ? GetSubMenu(mh_PopupMenu, 0) : NULL;
	if (!hPopup) {
		MBoxAssert(hPopup!=NULL);
		return;
	}
	
	// Некузяво. Может вслыть тултип под меню
	//ptCur.x++; ptCur.y++; // чтобы меню можно было сразу закрыть левым кликом.
	
	bool lbIsFar = mp_RCon->isFar(TRUE/* abPluginRequired */);
	bool lbIsPanels = lbIsFar && mp_RCon->isFilePanel(false/* abPluginAllowed */);
	bool lbIsEditorModified = lbIsFar && mp_RCon->isEditorModified();
	bool lbHaveModified = lbIsFar && mp_RCon->GetModifiedEditors();
	if (lbHaveModified) {
		if (!gSet.sSaveAllMacro || !*gSet.sSaveAllMacro)
			lbHaveModified = false;
	}
	
	EnableMenuItem(hPopup, IDM_CLOSE, MF_BYCOMMAND | (lbIsFar ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(hPopup, IDM_ADMIN_DUPLICATE, MF_BYCOMMAND | (lbIsPanels ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(hPopup, IDM_SAVE, MF_BYCOMMAND | (lbIsEditorModified ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(hPopup, IDM_SAVEALL, MF_BYCOMMAND | (lbHaveModified ? MF_ENABLED : MF_GRAYED));

	int nCmd = TrackPopupMenu(hPopup, 
            TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, 
            ptCur.x, ptCur.y, 0, ghWnd, NULL);
	if (!nCmd)
		return; // отмена
	
	switch (nCmd) {
	case IDM_CLOSE:
		mp_RCon->PostMacro(gSet.sTabCloseMacro ? gSet.sTabCloseMacro : L"F10");
		break;
	case IDM_DETACH:
		mp_RCon->Detach();
		gConEmu.OnVConTerminated(this);
		break;
	case IDM_TERMINATE:
		mp_RCon->CloseConsole();
		break;
	case IDM_RESTART:
	case IDM_RESTARTAS:
		if (gConEmu.isActive(this)) {
			gConEmu.Recreate(TRUE, FALSE, (nCmd==IDM_RESTARTAS));
		} else {
			MBoxAssert(gConEmu.isActive(this));
		}
		break;
	case IDM_NEW:
		gConEmu.Recreate(FALSE, gSet.isMultiNewConfirm);
		break;
	case IDM_ADMIN_DUPLICATE:
		mp_RCon->AdminDuplicate();
		break;
	case IDM_SAVE:
		mp_RCon->PostMacro(L"F2");
		break;
	case IDM_SAVEALL:
		mp_RCon->PostMacro(gSet.sSaveAllMacro);
		break;
	}
}

void CVirtualConsole::OnPanelViewSettingsChanged()
{
	if (!this) return;

	if (!mn_ConEmuSettingsMsg)
		mn_ConEmuSettingsMsg = RegisterWindowMessage(CONEMUMSG_PNLVIEWSETTINGS);

	DWORD nPID = GetCurrentProcessId();
	if (m_LeftPanelView.hWnd && IsWindow(m_LeftPanelView.hWnd)) {
		PostMessage(m_LeftPanelView.hWnd, mn_ConEmuSettingsMsg, nPID, 0);
	}
	if (m_RightPanelView.hWnd && IsWindow(m_RightPanelView.hWnd)) {
		PostMessage(m_RightPanelView.hWnd, mn_ConEmuSettingsMsg, nPID, 0);
	}
}

BOOL CVirtualConsole::IsPanelViews()
{
	if (!this) return FALSE;

	if (m_LeftPanelView.hWnd && IsWindow(m_LeftPanelView.hWnd))
		return TRUE;
	if (m_RightPanelView.hWnd && IsWindow(m_RightPanelView.hWnd))
		return TRUE;

	return FALSE;
}

BOOL CVirtualConsole::RegisterPanelView(PanelViewInit* ppvi)
{
	_ASSERTE(ppvi && ppvi->cbSize == sizeof(PanelViewInit));
	BOOL lbRc = FALSE;
	
	PanelViewInit* pp = (ppvi->bLeftPanel) ? &m_LeftPanelView : &m_RightPanelView;
	BOOL lbPrevRegistered = pp->bRegister;
	*pp = *ppvi;
	// Вернуть текущую палитру GUI
	COLORREF *pcrNormal = gSet.GetColors(FALSE);
	COLORREF *pcrFade = gSet.GetColors(TRUE);
	for (int i=0; i<16; i++) {
		// через FOR чтобы с BitMask не наколоться
		ppvi->crPalette[i] = (pcrNormal[i]) & 0xFFFFFF;
		ppvi->crFadePalette[i] = (pcrFade[i]) & 0xFFFFFF;
	}
	ppvi->bFadeColors = isFade;
	memmove(&(ppvi->ThSet), &(gSet.ThSet), sizeof(gSet.ThSet));
	
	if (ppvi->bRegister) {
		// При повторной регистрации - не дергаться
		if (!lbPrevRegistered) {
			//for (int i=0; i<20; i++) // на всякий случай - сначала все сбросим
			//	SetWindowLong(pp->hWnd, i*4, 0);
			SetParent((HWND)ppvi->hWnd, ghWnd);
			UpdatePanelRgn(ppvi->bLeftPanel, FALSE, TRUE);
			lbRc = UpdatePanelView(ppvi->bLeftPanel);
			// На панелях нужно "затереть" лишние части рамок
			Update(true);
		} else {
			lbRc = TRUE;
			if (ppvi->bLeftPanel)
				mb_LeftPanelRedraw = TRUE;
			else
				mb_RightPanelRedraw = TRUE;
		}
	} else {
		lbRc = TRUE;
	}
	
	return lbRc;
}

HRGN CVirtualConsole::CreateConsoleRgn(int x1, int y1, int x2, int y2, BOOL abTestOnly)
{
	POINT pt[2];
	if (abTestOnly) {
		// Интересует принципиальное пересечение прямоугольников, так что координаты не важны
		pt[0].x = x1 << 3; pt[0].y = y1 << 3;
		pt[1].x = x2 << 3; pt[1].y = y2 << 3;
	} else {
		pt[0] = ConsoleToClient(x1, y1);
		pt[1] = ConsoleToClient(x2+1, y2+1);
	}
	HRGN hRgn = CreateRectRgn(pt[0].x, pt[0].y, pt[1].x, pt[1].y);
	return hRgn;
}

BOOL CVirtualConsole::CheckDialogsChanged()
{
	BOOL lbChanged = FALSE;
	SMALL_RECT rcDlg[32];

	_ASSERTE(sizeof(mrc_LastDialogs) == sizeof(rcDlg));

	int nDlgCount = mp_RCon->GetDetectedDialogs(sizeofarray(rcDlg), rcDlg, NULL);
	if (mn_LastDialogsCount != nDlgCount) {
		lbChanged = TRUE;
	} else if (memcmp(rcDlg, mrc_LastDialogs, nDlgCount*sizeof(rcDlg[0]))!=0) {
		lbChanged = TRUE;
	}
	if (lbChanged) {
		memset(mrc_LastDialogs, 0, sizeof(mrc_LastDialogs));
		memmove(mrc_LastDialogs, rcDlg, nDlgCount*sizeof(rcDlg[0]));
		mn_LastDialogsCount = nDlgCount;
	}

	return lbChanged;
}

// Возвращает TRUE, если панель (или хотя бы часть ее ВИДИМА)
BOOL CVirtualConsole::UpdatePanelRgn(BOOL abLeftPanel, BOOL abTestOnly, BOOL abOnRegister)
{
	PanelViewInit* pp = abLeftPanel ? &m_LeftPanelView : &m_RightPanelView;
	if (!pp->hWnd || !IsWindow(pp->hWnd)) {
		if (pp->hWnd)
			pp->hWnd = NULL;
		return FALSE;
	}

	BOOL lbPartHidden = FALSE;
	BOOL lbPanelVisible = FALSE;
	SMALL_RECT rcDlg[32]; DWORD rnDlgFlags[32];

	_ASSERTE(sizeof(mrc_LastDialogs) == sizeof(rcDlg));

	int nDlgCount = mp_RCon->GetDetectedDialogs(sizeofarray(rcDlg), rcDlg, rnDlgFlags);
	if (!nDlgCount) {
		lbPartHidden = TRUE;
		if (!abTestOnly) {
			if (IsWindowVisible(pp->hWnd))
				ShowWindow(pp->hWnd, SW_HIDE);
		}
	} else {
		HRGN hRgn = NULL, hSubRgn = NULL, hCombine = NULL;
		
		for (int i = 0; i < nDlgCount; i++) {
			// Пропустить прямоугольник соответсвующий самой панели
			if (rcDlg[i].Left == pp->PanelRect.left &&
				rcDlg[i].Bottom == pp->PanelRect.bottom &&
				rcDlg[i].Right == pp->PanelRect.right)
			{
				// Учесть, что первая строка этого прямоугольника может быть скрыта строкой меню
				if (rcDlg[i].Top == pp->PanelRect.top
					|| (pp->PanelRect.top == 0 && rcDlg[i].Top == 1))
				{
					lbPanelVisible = TRUE;
					continue;
				}
			}
			if (!IntersectSmallRect(pp->WorkRect, rcDlg[i]))
				continue;
			// Все остальные прямоугольники вычитать из hRgn
			if (!hRgn)
				hRgn = CreateConsoleRgn(pp->WorkRect.left, pp->WorkRect.top, pp->WorkRect.right, pp->WorkRect.bottom, abTestOnly);
			hSubRgn = CreateConsoleRgn(rcDlg[i].Left, rcDlg[i].Top, rcDlg[i].Right, rcDlg[i].Bottom, abTestOnly);
			if (!hCombine)
				hCombine = CreateRectRgn(0,0,1,1);
			int nCRC = CombineRgn(hCombine, hRgn, hSubRgn, RGN_DIFF);
			if (nCRC) {
				// Замена переменных
				HRGN hTmp = hRgn; hRgn = hCombine; hCombine = hTmp;
				// А этот больше не нужен
				DeleteObject(hSubRgn); hSubRgn = NULL;
			}

			// Проверка, может регион уже пуст?
			if (nCRC == NULLREGION) {
				lbPanelVisible = FALSE; break;
			}
			//else if (nCRC == ERROR) {
			//	// Ошибка
			//	_ASSERTE(nCRC != ERROR);
			//	if (hRgn) { DeleteObject(hRgn); hRgn = NULL; }
			//	break;
			//}
		}

		if (abTestOnly) {
			if (hRgn) { DeleteObject(hRgn); hRgn = NULL; }
		}
		else
		if (lbPanelVisible)
		{
			lbPartHidden = (hRgn != NULL);
			if (hRgn) {
				POINT pt = ConsoleToClient(pp->WorkRect.left, pp->WorkRect.top);
				OffsetRgn(hRgn, -pt.x, -pt.y);
			}
			SetWindowRgn(pp->hWnd, hRgn, TRUE); hRgn = NULL;
			if (!abOnRegister) {
				if (!IsWindowVisible(pp->hWnd))
					ShowWindow(pp->hWnd, SW_SHOWNA);
			}
		}
		else
		{
			if (hRgn) { DeleteObject(hRgn); hRgn = NULL; }
			lbPartHidden = TRUE;
			if (IsWindowVisible(pp->hWnd))
				ShowWindow(pp->hWnd, SW_HIDE);
			SetWindowRgn(pp->hWnd, NULL, TRUE);
		}

		// чистка
		if (hCombine) { DeleteObject(hCombine); hCombine = NULL; }
	}

	return lbPanelVisible;
}

// Отсюда - Redraw не звать, только Invalidate!
BOOL CVirtualConsole::UpdatePanelView(BOOL abLeftPanel)
{
	PanelViewInit* pp = abLeftPanel ? &m_LeftPanelView : &m_RightPanelView;

	// Чтобы плагин знал, что поменялась палитра (это или Fade, или реальная перенастройка цветов).
	//for (int i=0; i<16; i++)
	//	SetWindowLong(pp->hWnd, i*4, mp_Colors[i]);
	if (!mn_ConEmuFadeMsg)
		mn_ConEmuFadeMsg = RegisterWindowMessage(CONEMUMSG_PNLVIEWFADE);
	DWORD nNewFadeValue = isFade ? 2 : 1;
	if (GetWindowLong(pp->hWnd, 16*4) != nNewFadeValue)
		PostMessage(pp->hWnd, mn_ConEmuFadeMsg, 100, nNewFadeValue);

	// Подготовить размеры
	POINT pt[2];
	int nTopShift = 1 + ((pp->nFarPanelSettings & 0x20/*FPS_SHOWCOLUMNTITLES*/) ? 1 : 0);
	int nBottomShift = ((pp->nFarPanelSettings & 0x40/*FPS_SHOWSTATUSLINE*/) ? 2 : 0);
	pp->WorkRect = MakeRect(
		pp->PanelRect.left+1, pp->PanelRect.top+nTopShift,
		pp->PanelRect.right, pp->PanelRect.bottom-nBottomShift);
	// лучше не зависеть от ConCharX - он может оказаться не инициализированным!
	pt[0] = MakePoint(pp->WorkRect.left*gSet.FontWidth(), pp->WorkRect.top*gSet.FontHeight());
	pt[1] = MakePoint(pp->WorkRect.right*gSet.FontWidth(), pp->WorkRect.bottom*gSet.FontHeight());
	//pt[0] = ConsoleToClient(pp->WorkRect.left, pp->WorkRect.top);
	//pt[1] = ConsoleToClient(pp->WorkRect.right, pp->WorkRect.bottom);
	
	TODO("Потребуется коррекция для DoubleView");
	MapWindowPoints(ghWndDC, ghWnd, pt, 2);
	
	//MoveWindow(ahView, pt[0].x,pt[0].y, pt[1].x-pt[0].x,pt[1].y-pt[0].y, FALSE);
	DWORD dwErr = 0;
	BOOL lbRc = SetWindowPos(pp->hWnd, HWND_TOP, 
		pt[0].x,pt[0].y, pt[1].x-pt[0].x,pt[1].y-pt[0].y, 
		SWP_ASYNCWINDOWPOS|SWP_DEFERERASE|SWP_NOREDRAW);
	if (!lbRc)
		dwErr = GetLastError();
	// И отрисовать
	//InvalidateRect(pp->hWnd, NULL, FALSE); -- не нужно, так получается двойной WM_PAINT
	
	return TRUE;
}

void CVirtualConsole::PolishPanelViews()
{
	if (!this) return;
	//mpsz_ConChar, mpn_ConAttrEx, TextWidth, TextHeight

	if (!m_LeftPanelView.bRegister && !m_RightPanelView.bRegister)
		return;

	for (int i=0; i<=1; i++) {
		PanelViewInit *pp = i ? &m_RightPanelView : &m_LeftPanelView;

		if (!pp->bRegister || !pp->hWnd)
			continue; // Панель не зарегистрирована

		if (mb_DialogsChanged) {
			if (!UpdatePanelRgn(i==0, TRUE))
				continue; // Панель стала невидимой
		}

		const CEFAR_INFO* pFarInfo = mp_RCon->GetFarInfo();

		if (!pFarInfo || !IsWindowVisible(pp->hWnd))
			continue; // Панель полностью закрыта

		/* Так, панель видима, нужно "поправить" заголовки и разделитель перед статусом */
		RECT rc = pp->PanelRect;
		if (rc.right > (LONG)TextWidth || rc.bottom > (LONG)TextHeight) {
			_ASSERTE(rc.right<=(LONG)TextWidth && rc.bottom<=(LONG)TextHeight);
			continue;
		}

		// Цвета фара
		BYTE btNamesColor = pFarInfo->nFarColors[COL_PANELCOLUMNTITLE];
		BYTE btPanelColor = pFarInfo->nFarColors[COL_PANELBOX];

		// 1. Заголовок панели
		int x;
		wchar_t *pszLine = mpsz_ConChar;
		CharAttr *pAttrs = mpn_ConAttrEx;
		int nFore = btPanelColor & 0xF;
		int nBack = (btPanelColor & 0xF0) >> 4;
		for (x = rc.left+1; x < rc.right && pszLine[x] != L' '; x++) {
			if (pszLine[x] == ucBoxSinglDownDblHorz && pAttrs[x].nForeIdx == nFore && pAttrs[x].nBackIdx == nBack)
				pszLine[x] = ucBoxDblHorz;
		}
		for (x = rc.right-1; x > rc.left && pszLine[x] != L' '; x--) {
			if (pszLine[x] == ucBoxSinglDownDblHorz && pAttrs[x].nForeIdx == nFore && pAttrs[x].nBackIdx == nBack)
				pszLine[x] = ucBoxDblHorz;
		}

		// 2. Строка с именами колонок
		pszLine = mpsz_ConChar+TextWidth;
		pAttrs = mpn_ConAttrEx+TextWidth;
		int nNFore = btNamesColor & 0xF;
		int nNBack = (btNamesColor & 0xF0) >> 4;
		if ((pp->nFarPanelSettings & 0x20/*FPS_SHOWCOLUMNTITLES*/)) {
			LPCWSTR pszNameTitle = mp_RCon->GetLngNameTime();
			int nNameLen = lstrlen(pszNameTitle);
			int nX1 = rc.left + ((pp->nFarPanelSettings & 0x800/*FPS_SHOWSORTMODELETTER*/) ? 2 : 1);
			int nLineLen = rc.right - nX1;
			//wmemset(pszLine+nX1, L' ', nLineLen);
			if (nNameLen > nLineLen) nNameLen = nLineLen;
			int nX3 = nX1 + ((nLineLen - nNameLen) >> 1);
			_ASSERTE(nX3<=rc.right);
			//wmemcpy(pszLine+nX3, mp_RCon->ms_NameTitle, nNameLen);
			//TODO("Возможно нужно будет и атрибуты (цвет) обновить");
			for (x = nX1; x < nX3; x++)
				if ((pszLine[x] != L' ' && pAttrs[x].nForeIdx == nNFore && pAttrs[x].nBackIdx == nNBack)
					|| (pszLine[x] == ucBoxSinglVert && pAttrs[x].nForeIdx == nFore && pAttrs[x].nBackIdx == nBack)
					) pszLine[x] = L' ';
			int nX4 = min(rc.right,(nX3+nNameLen));
			for (x = nX3; x < nX4; x++, pszNameTitle++)
				if ((pAttrs[x].nForeIdx == nNFore && pAttrs[x].nBackIdx == nNBack)
					|| (pszLine[x] == ucBoxSinglVert && pAttrs[x].nForeIdx == nFore && pAttrs[x].nBackIdx == nBack)
					) pszLine[x] = *pszNameTitle;
			for (x = nX4; x < rc.right; x++)
				if ((pszLine[x] != L' ' && pAttrs[x].nForeIdx == nNFore && pAttrs[x].nBackIdx == nNBack)
					|| (pszLine[x] == ucBoxSinglVert && pAttrs[x].nForeIdx == nFore && pAttrs[x].nBackIdx == nBack)
					) pszLine[x] = L' ';
		}

		// 3. Разделитель
		pszLine = mpsz_ConChar+TextWidth*(rc.bottom-2);
		pAttrs = mpn_ConAttrEx+TextWidth*(rc.bottom-2);
		if ((pp->nFarPanelSettings & 0x40/*FPS_SHOWSTATUSLINE*/)) {
			for (x = rc.left+1; x < rc.right; x++)
				if (pszLine[x] == ucBoxSinglUpHorz && pAttrs[x].nForeIdx == nFore && pAttrs[x].nBackIdx == nBack)
					pszLine[x] = ucBoxSinglHorz;
		}
	}
}
