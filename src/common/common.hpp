
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


#ifndef _COMMON_HEADER_HPP_
#define _COMMON_HEADER_HPP_

#include <windows.h>
#include <wchar.h>
#if !defined(__GNUC__)
#include <crtdbg.h>
#else
#define _ASSERTE(f)
#endif

#include "usetodo.hpp"

//#define CONEMUPIPE      L"\\\\.\\pipe\\ConEmuPipe%u"
//#define CONEMUEVENTIN   L"ConEmuInEvent%u"
//#define CONEMUEVENTOUT  L"ConEmuOutEvent%u"
//#define CONEMUEVENTPIPE L"ConEmuPipeEvent%u"

#define MIN_CON_WIDTH 28
#define MIN_CON_HEIGHT 7
#define GUI_ATTACH_TIMEOUT 5000

// with line number
#if !defined(_MSC_VER)

    #define TODO(s)
    #define WARNING(s)
    #define PRAGMA_ERROR(s)

    //#define CONSOLE_APPLICATION_16BIT 1
    
    typedef struct _CONSOLE_SELECTION_INFO {
        DWORD dwFlags;
        COORD dwSelectionAnchor;
        SMALL_RECT srSelection;
    } CONSOLE_SELECTION_INFO, *PCONSOLE_SELECTION_INFO;

    #ifndef max
    #define max(a,b)            (((a) > (b)) ? (a) : (b))
    #endif

    #ifndef min
    #define min(a,b)            (((a) < (b)) ? (a) : (b))
    #endif

    #define _ASSERT(f)
    #define _ASSERTE(f)
    
#else

    #define STRING2(x) #x
    #define STRING(x) STRING2(x)
    #define FILE_LINE __FILE__ "(" STRING(__LINE__) "): "
    #ifdef HIDE_TODO
    #define TODO(s) 
    #define WARNING(s) 
    #else
    #define TODO(s) __pragma(message (FILE_LINE "TODO: " s))
    #define WARNING(s) __pragma(message (FILE_LINE "warning: " s))
    #endif
    #define PRAGMA_ERROR(s) __pragma(message (FILE_LINE "error: " s))

    #ifdef _DEBUG
    #include <crtdbg.h>
    #endif

#endif

#ifdef _WIN64
	#ifndef WIN64
		WARNING("WIN64 was not defined");
		#define WIN64
	#endif
#endif

#ifdef _DEBUG
	#define USE_SEH
#endif

#ifdef USE_SEH
	#if defined(_MSC_VER)
		#pragma message ("Compiling USING exception handler")
	#endif
	
	#define SAFETRY   __try
	#define SAFECATCH __except(EXCEPTION_EXECUTE_HANDLER)
#else
	#if defined(_MSC_VER)
		#pragma message ("Compiling NOT using exception handler")
	#endif

	#define SAFETRY   if (true)
	#define SAFECATCH else
#endif	


#define isPressed(inp) ((GetKeyState(inp) & 0x8000) == 0x8000)
#define countof(a) (sizeof((a))/(sizeof(*(a))))
#define ZeroStruct(s) memset(&(s), 0, sizeof(s))

#ifdef _DEBUG
extern wchar_t gszDbgModLabel[6];
#define CHEKCDBGMODLABEL if (gszDbgModLabel[0]==0) { \
	wchar_t szFile[MAX_PATH]; GetModuleFileName(NULL, szFile, MAX_PATH); \
	wchar_t* pszName = wcsrchr(szFile, L'\\'); \
	if (_wcsicmp(pszName, L"\\conemu.exe")==0) lstrcpyW(gszDbgModLabel, L"gui"); \
	else if (_wcsicmp(pszName, L"\\conemuc.exe")==0) lstrcpyW(gszDbgModLabel, L"srv"); \
	else lstrcpyW(gszDbgModLabel, L"dll"); \
}
#ifdef SHOWDEBUGSTR
	#define DEBUGSTR(s) { MCHKHEAP; CHEKCDBGMODLABEL; SYSTEMTIME st; GetLocalTime(&st); wchar_t szDEBUGSTRTime[40]; wsprintf(szDEBUGSTRTime, L"%i:%02i:%02i.%03i(%s.%i) ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, gszDbgModLabel, GetCurrentThreadId()); OutputDebugString(szDEBUGSTRTime); OutputDebugString(s); }
#else
	#ifndef DEBUGSTR
		#define DEBUGSTR(s)
	#endif
#endif
#else
	#ifndef DEBUGSTR
		#define DEBUGSTR(s)
	#endif
#endif


#define CES_NTVDM 0x10
#define CEC_INITTITLE       L"ConEmu"
//#define CE_CURSORUPDATE     L"ConEmuCursorUpdate%u" // ConEmuC_PID - изменился курсор (размер или выделение). положение курсора отслеживает GUI

#define CESERVERPIPENAME    L"\\\\%s\\pipe\\ConEmuSrv%u"      // ConEmuC_PID
#define CESERVERINPUTNAME   L"\\\\%s\\pipe\\ConEmuSrvInput%u" // ConEmuC_PID
#define CESERVERQUERYNAME   L"\\\\%s\\pipe\\ConEmuSrvQuery%u" // ConEmuC_PID
#define CESERVERWRITENAME   L"\\\\%s\\pipe\\ConEmuSrvWrite%u" // ConEmuC_PID
#define CEGUIPIPENAME       L"\\\\%s\\pipe\\ConEmuGui%u"      // GetConsoleWindow() // необходимо, чтобы плагин мог общаться с GUI
#define CEPLUGINPIPENAME    L"\\\\%s\\pipe\\ConEmuPlugin%u"   // Far_PID
//
//#define MAXCONMAPCELLS      (600*400)
#define CECONMAPNAME        L"ConEmuFileMapping.%08X"
#define CECONMAPNAME_A      "ConEmuFileMapping.%08X"
#define CEFARMAPNAME        L"ConEmuFarMapping.%08X"
#define CECONVIEWSETNAME    L"ConEmuViewSetMapping.%u"
#define CEFARALIVEEVENT     L"ConEmuFarAliveEvent.%u"
//#define CECONMAPNAMESIZE    (sizeof(CESERVER_REQ_CONINFO)+(MAXCONMAPCELLS*sizeof(CHAR_INFO)))
//#define CEGUIATTACHED       L"ConEmuGuiAttached.%u"
#define CEGUIRCONSTARTED    L"ConEmuGuiRConStarted.%u"
#define CEGUI_ALIVE_EVENT   L"ConEmuGuiStarted"


//#define CONEMUMSG_ATTACH L"ConEmuMain::Attach"            // wParam == hConWnd, lParam == ConEmuC_PID
WARNING("CONEMUMSG_SRVSTARTED нужно переделать в команду пайпа для GUI");
//#define CONEMUMSG_SRVSTARTED L"ConEmuMain::SrvStarted"    // wParam == hConWnd, lParam == ConEmuC_PID
//#define CONEMUMSG_SETFOREGROUND L"ConEmuMain::SetForeground"            // wParam == hConWnd, lParam == ConEmuC_PID
#define CONEMUMSG_FLASHWINDOW L"ConEmuMain::FlashWindow"
//#define CONEMUCMDSTARTED L"ConEmuMain::CmdStarted"    // wParam == hConWnd, lParam == ConEmuC_PID (as ComSpec)
//#define CONEMUCMDSTOPPED L"ConEmuMain::CmdTerminated" // wParam == hConWnd, lParam == ConEmuC_PID (as ComSpec)
#define CONEMUMSG_LLKEYHOOK L"ConEmuMain::LLKeyHook"    // wParam == hConWnd, lParam == ConEmuC_PID
#define CONEMUMSG_PNLVIEWFADE L"ConEmuTh::Fade"
#define CONEMUMSG_PNLVIEWSETTINGS L"ConEmuTh::Settings"

//#define CONEMUMAPPING    L"ConEmuPluginData%u"
//#define CONEMUDRAGFROM   L"ConEmuDragFrom%u"
//#define CONEMUDRAGTO     L"ConEmuDragTo%u"
//#define CONEMUREQTABS    L"ConEmuReqTabs%u"
//#define CONEMUSETWINDOW  L"ConEmuSetWindow%u"
//#define CONEMUPOSTMACRO  L"ConEmuPostMacro%u"
//#define CONEMUDEFFONT    L"ConEmuDefFont%u"
//#define CONEMULANGCHANGE L"ConEmuLangChange%u"
//#define CONEMUEXIT       L"ConEmuExit%u"
//#define CONEMUALIVE      L"ConEmuAlive%u"
//#define CONEMUREADY      L"ConEmuReady%u"
#define CONEMUTABCHANGED L"ConEmuTabsChanged"


//#define CESIGNAL_C          L"ConEmuC_C_Signal.%u"
//#define CESIGNAL_BREAK      L"ConEmuC_Break_Signal.%u"
//#define CECMD_GETSHORTINFO  1
#define CECMD_GETCONSOLEINFO   2 // было CECMD_GETFULLINFO
//#define CECMD_SETSIZE       3
#define CECMD_CMDSTARTSTOP  4 // 0 - ServerStart, 1 - ServerStop, 2 - ComspecStart, 3 - ComspecStop
#define CECMD_GETGUIHWND    5
//#define CECMD_RECREATE      6
#define CECMD_TABSCHANGED   7
#define CECMD_CMDSTARTED    8 // == CECMD_SETSIZE + восстановить содержимое консоли (запустился comspec)
#define CECMD_CMDFINISHED   9 // == CECMD_SETSIZE + сохранить содержимое консоли (завершился comspec)
#define CECMD_GETOUTPUTFILE 10 // Записать вывод последней консольной программы во временный файл и вернуть его имя
#define CECMD_GETOUTPUT     11
#define CECMD_LANGCHANGE    12
#define CECMD_NEWCMD        13 // Запустить в этом экземпляре новую консоль с переданной командой (используется при SingleInstance)
#define CECMD_TABSCMD       14 // 0: спрятать/показать табы, 1: перейти на следующую, 2: перейти на предыдущую, 3: commit switch
#define CECMD_RESOURCES     15 // Посылается плагином при инициализации (установка ресурсов)
#define CECMD_GETNEWCONPARM 16 // Доп.аргументы для создания новой консоли (шрифт, размер,...)
#define CECMD_SETSIZESYNC   17 // Синхронно, ждет (но недолго), пока FAR обработает изменение размера (то есть отрисуется)
#define CECMD_ATTACH2GUI    18 // Выполнить подключение видимой (отключенной) консоли к GUI. Без аргументов
#define CECMD_FARLOADED     19 // Посылается плагином в сервер
#define CECMD_SHOWCONSOLE   20 // В Win7 релизе нельзя скрывать окно консоли, запущенной в режиме администратора
#define CECMD_POSTCONMSG    21 // В Win7 релизе нельзя посылать сообщения окну консоли, запущенной в режиме администратора
#define CECMD_REQUESTCONSOLEINFO 22 // было CECMD_REQUESTFULLINFO
#define CECMD_SETFOREGROUND 23
#define CECMD_FLASHWINDOW   24
#define CECMD_SETCONSOLECP  25
#define CECMD_SAVEALIASES   26
#define CECMD_GETALIASES    27
#define CECMD_SETSIZENOSYNC 28 // Почти CECMD_SETSIZE. Вызывается из плагина.
#define CECMD_SETDONTCLOSE  29
#define CECMD_REGPANELVIEW  30
#define CECMD_ONACTIVATION  31 // Для установки флажка ConsoleInfo->bConsoleActive
#define CECMD_SETWINDOWPOS  32 // CESERVER_REQ_SETWINDOWPOS.
#define CECMD_SETWINDOWRGN  33 // CESERVER_REQ_SETWINDOWRGN.

// Версия интерфейса
#define CESERVER_REQ_VER    41

#define PIPEBUFSIZE 4096


// Команды FAR плагина
#define CMD_DRAGFROM     0
#define CMD_DRAGTO       1
#define CMD_REQTABS      2
#define CMD_SETWINDOW    3
#define CMD_POSTMACRO    4 // Если первый символ макроса '@' и после него НЕ пробел - макрос выполняется в DisabledOutput
//#define CMD_DEFFONT    5
#define CMD_LANGCHANGE   6
#define CMD_SETENVVAR    7 // Установить переменные окружения (FAR plugin)
//#define CMD_SETSIZE    8
#define CMD_EMENU        9
#define CMD_LEFTCLKSYNC  10
#define CMD_REDRAWFAR    11
#define CMD_FARPOST      12
#define CMD_CHKRESOURCES 13
#define CMD_QUITFAR      14 // Дернуть завершение консоли (фара?)
// +2
#define MAXCMDCOUNT      16
#define CMD_EXIT         MAXCMDCOUNT-1



//#pragma pack(push, 1)
#include <pshpack1.h>

#define u64 unsigned __int64
typedef struct tag_HWND2 {
	DWORD u;
	operator HWND() const {
		return (HWND)u;
	};
	operator DWORD() const {
		return (DWORD)u;
	};
	struct tag_HWND2& operator=(HWND h) {
		u = (DWORD)h;
		return *this;
	};
} HWND2;


typedef struct tag_ThumbColor {
	union {
		struct {
			unsigned int   ColorRGB : 24;
			unsigned int   ColorIdx : 5; // 5 bits, to support value '16' - automatic use of panel color
			unsigned int   UseIndex : 1; // TRUE - Use ColorRef, FALSE - ColorIdx
		};
		DWORD RawColor;
	};
} ThumbColor;

typedef struct tag_ThumbSizes {
	// сторона превьюшки или иконки
	int nImgSize; // Thumbs: 96, Tiles: 48
	// Сдвиг превьюшки/иконки вправо&вниз относительно полного поля файла
	int nSpaceX1, nSpaceY1; // Thumbs: 1x1, Tiles: 4x4
	// Размер "остатка" вправо&вниз после превьюшки/иконки. Здесь рисуется текст.
	int nSpaceX2, nSpaceY2; // Thumbs: 5x25, Tiles: 172x4
	// Расстояние между превьюшкой/иконкой и текстом
	int nLabelSpacing; // Thumbs: 0, Tiles: 4
	// Отступ текста от краев прямоугольника метки
	int nLabelPadding; // Thumbs: 0, Tiles: 1
	// Шрифт
	wchar_t sFontName[36]; // Tahoma
	int nFontHeight; // 14
} ThumbSizes;


typedef enum {
	pvm_None = 0,
	pvm_Thumbnails = 1,
	pvm_Tiles = 2,
	// следующий режим (если он будет) делать 4! (это bitmask)
} PanelViewMode;

typedef struct tag_PanelViewSettings {
	DWORD cbSize; // Struct size, на всякий случай

	/* Цвета и рамки */
	ThumbColor crBackground; // Фон превьюшки: RGB или Index
	
	int nPreviewFrame; // 1 (серая рамка вокруг превьюшки
	ThumbColor crPreviewFrame; // RGB или Index
	
	int nSelectFrame; // 1 (рамка вокруг текущего элемента)
	ThumbColor crSelectFrame; // RGB или Index

	/* антиалиасинг */
	DWORD nFontQuality;

	/* Теперь разнообразные размеры */
	ThumbSizes Thumbs;
	ThumbSizes Tiles;
	
	// Прочие параметры загрузки
	BYTE  bLoadPreviews; // bitmask of PanelViewMode {1=Thumbs, 2=Tiles}
	bool  bLoadFolders;  // true - load infolder previews (only for Thumbs)
	DWORD nLoadTimeout;  // 15 sec

	DWORD nMaxZoom; // 600%
	bool  bUsePicView2; // true
	bool  bRestoreOnStartup;


	// Цвета теперь живут здесь!	
	COLORREF crPalette[16], crFadePalette[16];
	

	//// Пока не используется
	//DWORD nCacheFolderType; // юзер/программа/temp/и т.п.
	//wchar_t sCacheFolder[MAX_PATH];
} PanelViewSettings;


typedef BOOL (WINAPI* PanelViewInputCallback)(HANDLE hInput, PINPUT_RECORD lpBuffer, DWORD nBufSize, LPDWORD lpNumberOfEventsRead, BOOL* pbResult);
typedef BOOL (WINAPI* PanelViewOutputCallback)(HANDLE hOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
typedef struct tag_PanelViewInit {
	DWORD cbSize;
	BOOL  bRegister;
	HWND2 hWnd;
	BOOL  bLeftPanel;
	BOOL  bPanelFullCover; // если TRUE - View закроет панель целиком (с рамкой), а не только рабочую область
	DWORD nFarInterfaceSettings;
	DWORD nFarPanelSettings;
	// Координаты всей панели (левой, правой, или fullscreen)
	RECT  PanelRect;
	// Это координаты прямоугольника, в котором реально располагается View
	// Координаты определяются в GUI. Единицы - консольные.
	RECT  WorkRect;
	// Callbacks, используются только в плагинах
	PanelViewInputCallback pfnPeekPreCall, pfnPeekPostCall, pfnReadPreCall, pfnReadPostCall;
	PanelViewOutputCallback pfnWriteCall;
	/* out */
	//PanelViewSettings ThSet;
	BOOL bFadeColors;
} PanelViewInit;



TODO("Restrict CONEMUTABMAX to 128 chars. Only filename, and may be ellipsed...");
#define CONEMUTABMAX 0x400
typedef struct tag_ConEmuTab {
	int  Pos;
	int  Current;
	int  Type; // (Panels=1, Viewer=2, Editor=3) | (Elevated=0x100)
	int  Modified;
	wchar_t Name[CONEMUTABMAX];
	//  int  Modified;
	//  int isEditor;
} ConEmuTab;

typedef struct tag_CESERVER_REQ_CONEMUTAB {
	DWORD nTabCount;
	BOOL  bMacroActive;
	BOOL  bMainThread;
	ConEmuTab tabs[1];
} CESERVER_REQ_CONEMUTAB;

typedef struct tag_CESERVER_REQ_CONEMUTAB_RET {
	BOOL  bNeedPostTabSend;
	BOOL  bNeedResize;
	COORD crNewSize;
} CESERVER_REQ_CONEMUTAB_RET;

typedef struct tag_ForwardedPanelInfo {
	RECT ActiveRect;
	RECT PassiveRect;
	int ActivePathShift; // сдвиг в этой структуре в байтах
	int PassivePathShift; // сдвиг в этой структуре в байтах
	union { //x64 ready
		WCHAR* pszActivePath/*[MAX_PATH+1]*/;
		u64 Reserved1;
	};
	union { //x64 ready
		WCHAR* pszPassivePath/*[MAX_PATH+1]*/;
		u64 Reserved2;
	};
} ForwardedPanelInfo;

typedef struct tag_FarVersion {
	union {
		DWORD dwVer;
		struct {
			WORD dwVerMinor;
			WORD dwVerMajor;
		};
	};
	DWORD dwBuild;
} FarVersion;

typedef struct tag_ForwardedFileInfo {
	WCHAR Path[MAX_PATH+1];
} ForwardedFileInfo;


typedef struct tag_CESERVER_REQ_HDR {
	DWORD   nSize;
	DWORD   nCmd;
	DWORD   nVersion;
	DWORD   nSrcThreadId;
	DWORD   nSrcPID;
	DWORD   nCreateTick;
} CESERVER_REQ_HDR;


typedef struct tag_CESERVER_CHAR_HDR {
	int   nSize;    // размер структуры динамический. Если 0 - значит прямоугольник is NULL
	COORD cr1, cr2; // WARNING: Это АБСОЛЮТНЫЕ координаты (без учета прокрутки), а не экранные.
} CESERVER_CHAR_HDR;

typedef struct tag_CESERVER_CHAR {
	CESERVER_CHAR_HDR hdr; // фиксированная часть
	WORD  data[2];  // variable(!) length
} CESERVER_CHAR;

typedef struct tag_CESERVER_CONSAVE_HDR {
	CESERVER_REQ_HDR hdr; // Заполняется только перед отсылкой в плагин
	CONSOLE_SCREEN_BUFFER_INFO sbi;
	DWORD cbMaxOneBufferSize;
} CESERVER_CONSAVE_HDR;

typedef struct tag_CESERVER_CONSAVE {
	CESERVER_CONSAVE_HDR hdr;
	wchar_t Data[1];
} CESERVER_CONSAVE;



typedef struct tag_CESERVER_REQ_RGNINFO {
	DWORD dwRgnInfoSize;
	CESERVER_CHAR RgnInfo;
} CESERVER_REQ_RGNINFO;

typedef struct tag_CESERVER_REQ_FULLCONDATA {
	DWORD dwRgnInfoSize_MustBe0; // must be 0
	DWORD dwOneBufferSize; // may be 0
	wchar_t Data[300]; // Variable length!!!
} CESERVER_REQ_FULLCONDATA;

typedef struct tag_CEFAR_SHORT_PANEL_INFO {
	int   PanelType;
	int   Plugin;
	RECT  PanelRect;
	int   ItemsNumber;
	int   SelectedItemsNumber;
	int   CurrentItem;
	int   TopPanelItem;
	int   Visible;
	int   Focus;
	int   ViewMode;
	int   ShortNames;
	int   SortMode;
	DWORD Flags;
} CEFAR_SHORT_PANEL_INFO;

typedef struct tag_CEFAR_INFO {
	DWORD cbSize;
	DWORD nFarInfoIdx;
	FarVersion FarVer;
	DWORD nProtocolVersion; // == CESERVER_REQ_VER
	DWORD nFarPID, nFarTID;
	BYTE nFarColors[0x100]; // Массив цветов фара
	DWORD nFarInterfaceSettings;
	DWORD nFarPanelSettings;
	DWORD nFarConfirmationSettings;
	BOOL  bFarPanelAllowed; // FCTL_CHECKPANELSEXIST
	// Информация собственно о панелях присутствовать не обязана
	BOOL bFarPanelInfoFilled;
	BOOL bFarLeftPanel, bFarRightPanel;   
	CEFAR_SHORT_PANEL_INFO FarLeftPanel, FarRightPanel; // FCTL_GETPANELSHORTINFO,...
	DWORD nFarConsoleMode;
	//DWORD nFarReadIdx;    // index, +1, когда фар в последний раз позвал (Read|Peek)ConsoleInput или GetConsoleInputCount
	// Далее идут строковые ресурсы, на которые в некоторых случаях ориентируется GUI
	wchar_t sLngEdit[64]; // "edit"
	wchar_t sLngView[64]; // "view"
	wchar_t sLngTemp[64]; // "{Temporary panel"
	wchar_t sLngName[64]; // "Name"
	wchar_t sReserved[MAX_PATH]; // Чтобы случайно GetMsg из допустимого диапазона не вышел
} CEFAR_INFO;


typedef struct tag_CESERVER_REQ_CONINFO_HDR {
	DWORD cbSize;
	DWORD nLogLevel;
	COORD crMaxConSize;
	DWORD cbExtraSize; // Размер данных, идущих за структурой (после cbSize), TODO на CESERVER_REQ_CONINFO_DATA[]
	HWND2 hConWnd;     // !! положение hConWnd и nGuiPID не менять !!
	DWORD nServerPID;  //
	DWORD nGuiPID;     // !! на них ориентируется PicViewWrapper   !!
	//
	DWORD bConsoleActive;
	DWORD nProtocolVersion; // == CESERVER_REQ_VER
	//
	DWORD nFarPID; // PID последнего фара, обновившего информацию о себе в этой структуре
	DWORD nCurDataMapIdx; // суффикс для текущего MAP файла с данными
	DWORD nCurDataMaxSize; // Максимальный размер буфера nCurDataMapIdx
	DWORD nPacketId;
	//DWORD nFarReadTick; // GetTickCount(), когда фар в последний раз считывал события из консоли
	//DWORD nFarUpdateTick0;// GetTickCount(), устанавливается в начале обновления консоли из фара (вдруг что свалится...)
	//DWORD nFarUpdateTick; // GetTickCount(), когда консоль была обновлена в последний раз из фара
	//DWORD nFarReadIdx;    // index, +1, когда фар в последний раз позвал (Read|Peek)ConsoleInput или GetConsoleInputCount
	DWORD nSrvUpdateTick; // GetTickCount(), когда консоль была считана в последний раз в сервере
	DWORD nReserved0; //DWORD nInputTID;
	DWORD nProcesses[20];
    DWORD dwCiSize;
	CONSOLE_CURSOR_INFO ci;
    DWORD dwConsoleCP;
	DWORD dwConsoleOutputCP;
	DWORD dwConsoleMode;
	DWORD dwSbiSize;
	CONSOLE_SCREEN_BUFFER_INFO sbi;
	//// Информация о текущем FAR
	//DWORD nFarInfoIdx; // выносим из структуры CEFAR_INFO, т.к. ее копия хранится в плагине
	//CEFAR_INFO FarInfo;
} CESERVER_REQ_CONINFO_HDR;

typedef struct tag_CESERVER_REQ_CONINFO_DATA {
	COORD      crBufSize;
	CHAR_INFO  Buf[1];
} CESERVER_REQ_CONINFO_DATA;

typedef struct tag_CESERVER_REQ_CONINFO_FULL {
	CESERVER_REQ_CONINFO_HDR  info;
	CESERVER_REQ_CONINFO_DATA data;
} CESERVER_REQ_CONINFO_FULL;

//typedef struct tag_CESERVER_REQ_CONINFO {
//	CESERVER_REQ_CONINFO_HDR inf;
//    union {
//	/* 9*/DWORD dwRgnInfoSize;
//	      CESERVER_REQ_RGNINFO RgnInfo;
//    /*10*/CESERVER_REQ_FULLCONDATA FullData;
//	};
//} CESERVER_REQ_CONINFO;

typedef struct tag_CESERVER_REQ_SETSIZE {
	USHORT nBufferHeight; // 0 или высота буфера (режим с прокруткой)
	COORD  size;
	SHORT  nSendTopLine;  // -1 или 0based номер строки зафиксированной в GUI (только для режима с прокруткой)
	SMALL_RECT rcWindow;  // координаты видимой области для режима с прокруткой
	DWORD  dwFarPID;      // Если передано - сервер должен сам достучаться до FAR'а и обновить его размер через плагин ПЕРЕД возвратом
} CESERVER_REQ_SETSIZE;

typedef struct tag_CESERVER_REQ_OUTPUTFILE {
	BOOL  bUnicode;
	WCHAR szFilePathName[MAX_PATH+1];
} CESERVER_REQ_OUTPUTFILE;

typedef struct tag_CESERVER_REQ_RETSIZE {
	DWORD nNextPacketId;
	CONSOLE_SCREEN_BUFFER_INFO SetSizeRet;
} CESERVER_REQ_RETSIZE;

typedef struct tag_CESERVER_REQ_NEWCMD {
	wchar_t szCurDir[MAX_PATH];
	wchar_t szCommand[MAX_PATH]; // На самом деле - variable_size !!!
} CESERVER_REQ_NEWCMD;

typedef struct tag_CESERVER_REQ_STARTSTOP {
	DWORD nStarted; // 0 - ServerStart, 1 - ServerStop, 2 - ComspecStart, 3 - ComspecStop
	HWND2 hWnd; // при передаче В GUI - консоль, при возврате в консоль - GUI
	DWORD dwPID; //, dwInputTID;
	DWORD nSubSystem; // 255 для DOS программ, 0x100 - аттач из FAR плагина
	BOOL  bRootIsCmdExe;
	BOOL  bUserIsAdmin;
	// А это приходит из консоли, вдруго консольная программа успела поменять размер буфера
	CONSOLE_SCREEN_BUFFER_INFO sbi;
	// Reserved
	DWORD nReserved0, nReserved1;
} CESERVER_REQ_STARTSTOP;

// _ASSERTE(sizeof(CESERVER_REQ_STARTSTOPRET) <= sizeof(CESERVER_REQ_STARTSTOP));
typedef struct tag_CESERVER_REQ_STARTSTOPRET {
	BOOL  bWasBufferHeight;
	HWND2 hWnd; // при возврате в консоль - GUI (главное окно)
	HWND2 hWndDC;
	DWORD dwPID; // при возврате в консоль - PID ConEmu.exe
	DWORD nBufferHeight, nWidth, nHeight;
	DWORD dwSrvPID;
	BOOL  bNeedLangChange;
	u64   NewConsoleLang;
} CESERVER_REQ_STARTSTOPRET;

typedef struct tag_CESERVER_REQ_POSTMSG {
	BOOL    bPost;
	HWND2   hWnd;
	UINT    nMsg;
	// Заложимся на унификацию x86 & x64
	u64     wParam, lParam;
} CESERVER_REQ_POSTMSG;

typedef struct tag_CESERVER_REQ_FLASHWINFO {
	BOOL  bSimple;
	HWND2 hWnd;
	BOOL  bInvert; // только если bSimple == TRUE
	DWORD dwFlags; // а это и далее, если bSimple == FALSE
	UINT  uCount;
	DWORD dwTimeout;
} CESERVER_REQ_FLASHWINFO;

// CMD_SETENVVAR - FAR plugin
typedef struct tag_FAR_REQ_SETENVVAR {
	BOOL    bFARuseASCIIsort;
	wchar_t szEnv[1]; // Variable length: <Name>\0<Value>\0<Name2>\0<Value2>\0\0
} FAR_REQ_SETENVVAR;

typedef struct tag_CESERVER_REQ_SETCONCP {
	BOOL    bSetOutputCP; // [IN], [Out]=result
	DWORD   nCP;          // [IN], [Out]=LastError
} CESERVER_REQ_SETCONCP;

typedef struct tag_CESERVER_REQ_SETWINDOWPOS {
	HWND2 hWnd;
	HWND2 hWndInsertAfter;
	int X;
	int Y;
	int cx;
	int cy;
	UINT uFlags;
} CESERVER_REQ_SETWINDOWPOS;

typedef struct tag_CESERVER_REQ_SETWINDOWRGN {
	HWND2 hWnd;
	int   nRectCount;  // если 0 - сбросить WindowRgn, иначе - обсчитать.
	BOOL  bRedraw;
	RECT  rcRects[20]; // [0] - основной окна, [
} CESERVER_REQ_SETWINDOWRGN;

typedef struct tag_CESERVER_REQ {
    CESERVER_REQ_HDR hdr;
	union {
		BYTE    Data[1]; // variable(!) length
		WORD    wData[1];
		DWORD   dwData[1];
		u64     qwData[1];
		CESERVER_REQ_CONINFO_HDR ConInfo;
		CESERVER_REQ_SETSIZE SetSize;
		CESERVER_REQ_RETSIZE SetSizeRet;
		CESERVER_REQ_OUTPUTFILE OutputFile;
		CESERVER_REQ_NEWCMD NewCmd;
		CESERVER_REQ_STARTSTOP StartStop;
		CESERVER_REQ_STARTSTOPRET StartStopRet;
		CESERVER_REQ_CONEMUTAB Tabs;
		CESERVER_REQ_CONEMUTAB_RET TabsRet;
		CESERVER_REQ_POSTMSG Msg;
		CESERVER_REQ_FLASHWINFO Flash;
		FAR_REQ_SETENVVAR SetEnvVar;
		CESERVER_REQ_SETCONCP SetConCP;
		CESERVER_REQ_SETWINDOWPOS SetWndPos;
		CESERVER_REQ_SETWINDOWRGN SetWndRgn;
		PanelViewInit PVI;
	};
} CESERVER_REQ;


//#pragma pack(pop)
#include <poppack.h>



//#define GWL_TABINDEX     0
//#define GWL_LANGCHANGE   4

#ifdef _DEBUG
    #define CONEMUALIVETIMEOUT INFINITE
    #define CONEMUREADYTIMEOUT INFINITE
    #define CONEMUFARTIMEOUT   120000 // Сколько ожидать, пока ФАР среагирует на вызов плагина
#else
    #define CONEMUALIVETIMEOUT 1000  // Живость плагина ждем секунду
    #define CONEMUREADYTIMEOUT 10000 // А на выполнение команды - 10s max
    #define CONEMUFARTIMEOUT   10000 // Сколько ожидать, пока ФАР среагирует на вызов плагина
#endif



/*enum PipeCmd
{
    SetTabs=0,
    DragFrom,
    DragTo
};*/

// ConEmu.dll экспортирует следующие функции
//HWND WINAPI GetFarHWND();
//void WINAPI _export GetFarVersion ( FarVersion* pfv );

//#if defined(__GNUC__)
////typedef DWORD   HWINEVENTHOOK;
//#define WINEVENT_OUTOFCONTEXT   0x0000  // Events are ASYNC
//// User32.dll
//typedef HWINEVENTHOOK (WINAPI* FSetWinEventHook)(DWORD eventMin, DWORD eventMax, HMODULE hmodWinEventProc, WINEVENTPROC pfnWinEventProc, DWORD idProcess, DWORD idThread, DWORD dwFlags);
//typedef BOOL (WINAPI* FUnhookWinEvent)(HWINEVENTHOOK hWinEventHook);
//#endif


#ifdef __cplusplus


#define CERR_CMDLINEEMPTY 200
#define CERR_CMDLINE      201

#define MOUSE_EVENT_MOVE      (WM_APP+10)
#define MOUSE_EVENT_CLICK     (WM_APP+11)
#define MOUSE_EVENT_DBLCLICK  (WM_APP+12)
#define MOUSE_EVENT_WHEELED   (WM_APP+13)
#define MOUSE_EVENT_HWHEELED  (WM_APP+14)
#define MOUSE_EVENT_FIRST MOUSE_EVENT_MOVE
#define MOUSE_EVENT_LAST MOUSE_EVENT_HWHEELED

//#define INPUT_THREAD_ALIVE_MSG (WM_APP+100)

#define MAX_INPUT_QUEUE_EMPTY_WAIT 100


int NextArg(const wchar_t** asCmdLine, wchar_t* rsArg/*[MAX_PATH+1]*/, const wchar_t** rsArgStart=NULL);
BOOL PackInputRecord(const INPUT_RECORD* piRec, MSG* pMsg);
BOOL UnpackInputRecord(const MSG* piMsg, INPUT_RECORD* pRec);
SECURITY_ATTRIBUTES* NullSecurity();
wchar_t* GetShortFileNameEx(LPCWSTR asLong);
void CommonShutdown();
BOOL IsUserAdmin();

//------------------------------------------------------------------------
///| Section |////////////////////////////////////////////////////////////
//------------------------------------------------------------------------
class MSectionLock;

class MSection
{
protected:
	CRITICAL_SECTION m_cs;
	DWORD mn_TID; // устанавливается только после EnterCriticalSection
	int mn_Locked;
	BOOL mb_Exclusive;
	HANDLE mh_ReleaseEvent;
	friend class MSectionLock;
	DWORD mn_LockedTID[10];
	int   mn_LockedCount[10];
public:
	MSection() {
		mn_TID = 0; mn_Locked = 0; mb_Exclusive = FALSE;
		ZeroStruct(mn_LockedTID); ZeroStruct(mn_LockedCount);
		InitializeCriticalSection(&m_cs);
		mh_ReleaseEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		_ASSERTE(mh_ReleaseEvent!=NULL);
		if (mh_ReleaseEvent) ResetEvent(mh_ReleaseEvent);
	};
	~MSection() {
		DeleteCriticalSection(&m_cs);
		if (mh_ReleaseEvent) {
			CloseHandle(mh_ReleaseEvent); mh_ReleaseEvent = NULL;
		}
	};
public:
	void ThreadTerminated(DWORD dwTID) {
		for (int i=1; i<10; i++) {
			if (mn_LockedTID[i] == dwTID) {
				mn_LockedTID[i] = 0;
				if (mn_LockedCount[i] != 0) {
					_ASSERTE(mn_LockedCount[i] == 0);
				}
				break;
			}
		}
	};
protected:
	void AddRef(DWORD dwTID) {
		mn_Locked ++; // увеличиваем счетчик nonexclusive locks
		_ASSERTE(mn_Locked>0);
		ResetEvent(mh_ReleaseEvent); // На всякий случай сбросим Event
		int j = -1; // будет -2, если ++ на существующий, иначе - +1 на пустой
		for (int i=1; i<10; i++) {
			if (mn_LockedTID[i] == dwTID) {
				mn_LockedCount[i] ++; 
				j = -2; 
				break;
			} else if (j == -1 && mn_LockedTID[i] == 0) {
				mn_LockedTID[i] = dwTID;
				mn_LockedCount[i] ++; 
				j = i;
				break;
			}
		}
		if (j == -1) { // Этого быть не должно
			_ASSERTE(j != -1);
		}
	};
	int ReleaseRef(DWORD dwTID) {
		_ASSERTE(mn_Locked>0);
		int nInThreadLeft = 0;
		if (mn_Locked > 0) mn_Locked --;
		if (mn_Locked == 0)
			SetEvent(mh_ReleaseEvent); // Больше nonexclusive locks не осталось
		for (int i=1; i<10; i++) {
			if (mn_LockedTID[i] == dwTID) {
				mn_LockedCount[i] --; 
				if ((nInThreadLeft = mn_LockedCount[i]) == 0)
					mn_LockedTID[i] = 0; // Иначе при динамически создаваемых нитях - 10 будут в момент использованы
				break;
			}
		}
		return nInThreadLeft;
	};
	void WaitUnlocked(DWORD dwTID, DWORD anTimeout) {
		DWORD dwStartTick = GetTickCount();
		int nSelfCount = 0;
		for (int i=1; i<10; i++) {
			if (mn_LockedTID[i] == dwTID) {
				nSelfCount = mn_LockedCount[i];
				break;
			}
		}
		while (mn_Locked > nSelfCount) {
			#ifdef _DEBUG
			DEBUGSTR(L"!!! Waiting for exclusive access\n");

			DWORD nWait = 
			#endif

			WaitForSingleObject(mh_ReleaseEvent, 10);

			DWORD dwCurTick = GetTickCount();
			DWORD dwDelta = dwCurTick - dwStartTick;

			if (anTimeout != (DWORD)-1) {
				if (dwDelta > anTimeout) {
					#ifndef CSECTION_NON_RAISE
					_ASSERTE(dwDelta<=anTimeout);
					#endif
					break;
				}
			}
			#ifdef _DEBUG
			else if (dwDelta > 3000) {
				#ifndef CSECTION_NON_RAISE
				_ASSERTE(dwDelta <= 3000);
				#endif
				break;
			}
			#endif
		}
	};
	bool MyEnterCriticalSection(DWORD anTimeout) {
		//EnterCriticalSection(&m_cs); 
		// дождаться пока секцию отпустят

		// НАДА. Т.к. может быть задан nTimeout (для DC)
		DWORD dwTryLockSectionStart = GetTickCount(), dwCurrentTick;

		if (!TryEnterCriticalSection(&m_cs)) {
			Sleep(10);
			while (!TryEnterCriticalSection(&m_cs)) {
				Sleep(10);
				DEBUGSTR(L"TryEnterCriticalSection failed!!!\n");

				dwCurrentTick = GetTickCount();
				if (anTimeout != (DWORD)-1) {
					if (((dwCurrentTick - dwTryLockSectionStart) > anTimeout)) {
						#ifndef CSECTION_NON_RAISE
						_ASSERTE((dwCurrentTick - dwTryLockSectionStart) <= anTimeout);
						#endif
						return false;
					}
				}
				#ifdef _DEBUG
				else if ((dwCurrentTick - dwTryLockSectionStart) > 3000) {
					#ifndef CSECTION_NON_RAISE
					_ASSERTE((dwCurrentTick - dwTryLockSectionStart) <= 3000);
					#endif
					dwTryLockSectionStart = GetTickCount();
				}
				#endif
			}
		}

		return true;
	}
	BOOL Lock(BOOL abExclusive, DWORD anTimeout=-1/*, BOOL abRelockExclusive=FALSE*/) {
		DWORD dwTID = GetCurrentThreadId();
		
		// Может эта нить уже полностью заблокирована?
		if (mb_Exclusive && dwTID == mn_TID) {
			return FALSE; // Уже, но Unlock делать не нужно!
		}
		
		if (!abExclusive) {
			// Быстрая блокировка, не запрещающая чтение другим нитям.
			// Запрещено только изменение (пересоздание буфера например)
			AddRef(dwTID);
			
			// Если другая нить уже захватила exclusive
			if (mb_Exclusive) {
				int nLeft = ReleaseRef(dwTID); // Иначе можем попасть на взаимную блокировку
				if (nLeft > 0) {
					_ASSERTE(nLeft == 0);
				}

				DEBUGSTR(L"!!! Failed non exclusive lock, trying to use CriticalSection\n");
				bool lbEntered = MyEnterCriticalSection(anTimeout); // дождаться пока секцию отпустят
				_ASSERTE(!mb_Exclusive); // После LeaveCriticalSection mb_Exclusive УЖЕ должен быть сброшен

				AddRef(dwTID); // Возвращаем блокировку

				// Но поскольку нам нужен только nonexclusive lock
				if (lbEntered)
					LeaveCriticalSection(&m_cs);
			}
		} else {
			// Требуется Exclusive Lock

			#ifdef _DEBUG
			if (mb_Exclusive) {
				// Этого надо стараться избегать
				DEBUGSTR(L"!!! Exclusive lock found in other thread\n");
			}
			#endif
			
			// Если есть ExclusiveLock (в другой нити) - дождется сама EnterCriticalSection
			#ifdef _DEBUG
			BOOL lbPrev = mb_Exclusive;
			DWORD nPrevTID = mn_TID;
			#endif
			mb_Exclusive = TRUE; // Сразу, чтобы в nonexclusive не нарваться
			TODO("Need to check, if MyEnterCriticalSection failed on timeout!\n");
			MyEnterCriticalSection(anTimeout);
			_ASSERTE(!(lbPrev && mb_Exclusive)); // После LeaveCriticalSection mb_Exclusive УЖЕ должен быть сброшен
			mb_Exclusive = TRUE; // Флаг могла сбросить другая нить, выполнившая Leave
			mn_TID = dwTID; // И запомним, в какой нити это произошло
			_ASSERTE(mn_LockedTID[0] == 0 && mn_LockedCount[0] == 0);
			mn_LockedTID[0] = dwTID;
			mn_LockedCount[0] ++;

			/*if (abRelockExclusive) {
				ReleaseRef(dwTID); // Если до этого был nonexclusive lock
			}*/

			// B если есть nonexclusive locks - дождаться их завершения
			if (mn_Locked) {
				//WARNING: Тут есть шанс наколоться, если сначала был NonExclusive, а потом в этой же нити - Exclusive
				// В таких случаях нужно вызывать с параметром abRelockExclusive
				WaitUnlocked(dwTID, anTimeout);
			}
		}
		
		return TRUE;
	};
	void Unlock(BOOL abExclusive) {
		DWORD dwTID = GetCurrentThreadId();
		if (abExclusive) {
			_ASSERTE(dwTID == dwTID && mb_Exclusive);
			_ASSERTE(mn_LockedTID[0] == dwTID);
			mb_Exclusive = FALSE; mn_TID = 0;
			mn_LockedTID[0] = 0; mn_LockedCount[0] --;
			LeaveCriticalSection(&m_cs);
		} else {
			ReleaseRef(dwTID);
		}
	};
};

class MSectionThread
{
protected:
	MSection* mp_S;
	DWORD mn_TID;
public:
	MSectionThread(MSection* apS) {
		mp_S = apS; mn_TID = GetCurrentThreadId();
	};
	~MSectionThread() {
		if (mp_S && mn_TID)
			mp_S->ThreadTerminated(mn_TID);
	};
};

class MSectionLock
{
protected:
	MSection* mp_S;
	BOOL mb_Locked, mb_Exclusive;
public:
	BOOL Lock(MSection* apS, BOOL abExclusive=FALSE, DWORD anTimeout=-1) {
		if (mb_Locked && apS == mp_S && (abExclusive == mb_Exclusive || mb_Exclusive))
			return FALSE; // уже заблокирован
		_ASSERTE(!mb_Locked);
		mb_Exclusive = abExclusive;
		mp_S = apS;
		mb_Locked = mp_S->Lock(mb_Exclusive, anTimeout);
		return mb_Locked;
	};
	BOOL RelockExclusive(DWORD anTimeout=-1) {
		if (mb_Locked && mb_Exclusive) return FALSE; // уже
		// Чистый ReLock делать нельзя. Виснут другие нити, которые тоже запросили ReLock
		Unlock();
		mb_Exclusive = TRUE;
		mb_Locked = mp_S->Lock(mb_Exclusive, anTimeout);
		return mb_Locked;
	};
	void Unlock() {
		if (mp_S && mb_Locked) {
			mp_S->Unlock(mb_Exclusive);
			mb_Locked = FALSE;
		}
	};
	BOOL isLocked() {
		return (mp_S!=NULL) && mb_Locked;
	};
public:
	MSectionLock() {
		mp_S = NULL; mb_Locked = FALSE; mb_Exclusive = FALSE;
	};
	~MSectionLock() {
		if (mb_Locked) Unlock();
	};
};


/* Console Handles */
class MConHandle {
private:
	wchar_t   ms_Name[10];
	HANDLE    mh_Handle;
	MSection  mcs_Handle;
	BOOL      mb_OpenFailed;
	DWORD     mn_LastError;
	DWORD     mn_StdMode;

public:
	operator const HANDLE();

public:
	void Close();

public:
	MConHandle(LPCWSTR asName);
	~MConHandle();
};


template <class T>
class MFileMapping
{
protected:
	HANDLE mh_Mapping;
	BOOL mb_WriteAllowed;
	int mn_Size;
	T* mp_Data; //WARNING!!! Доступ может быть только на чтение!
	wchar_t ms_MapName[MAX_PATH];
	DWORD mn_LastError;
	wchar_t ms_Error[MAX_PATH*2];
public:
	operator T*() {
		return mp_Data;
	};
	bool IsValid() {
		return (mp_Data!=NULL);
	};
	LPCWSTR GetErrorText() {
		return ms_Error;
	};
	bool SetFrom(const T* pSrc, int nSize=-1) {
		if (!IsValid() || !nSize) return false;
		if (nSize<0) nSize = sizeof(T);
		bool lbChanged = (memcmp(mp_Data, pSrc, nSize)!=0);
		memmove(mp_Data, pSrc, nSize);
		return lbChanged;
	}
	bool GetTo(T* pDst, int nSize=-1) {
		if (!IsValid() || !nSize) return false;
		if (nSize<0) nSize = sizeof(T);
		memmove(pDst, mp_Data, nSize);
		return true;
	}
public:
	void InitName(const wchar_t *aszTemplate,DWORD Parm1=0,DWORD Parm2=0) {
		wsprintfW(ms_MapName, aszTemplate, Parm1, Parm2);
	};
	void ClosePtr() {
		if (mp_Data) {
			UnmapViewOfFile(mp_Data);
			mp_Data = NULL;
		}
	};
	void CloseMap() {
		if (mp_Data) ClosePtr();
		if (mh_Mapping) {
			CloseHandle(mh_Mapping);
			mh_Mapping = NULL;
		}
		mh_Mapping = NULL; mb_WriteAllowed = FALSE; mp_Data = NULL; 
		mn_Size = -1; mn_LastError = 0;
	};
protected:
	T* InternalOpenCreate(BOOL abCreate,BOOL abReadWrite,int nSize) {
		if (mh_Mapping) CloseMap();
		mn_LastError = 0; ms_Error[0] = 0;
		_ASSERTE(mh_Mapping==NULL && mp_Data==NULL);
		_ASSERTE(nSize==-1 || nSize>=sizeof(T));

		if (ms_MapName[0] == 0) {
			_ASSERTE(ms_MapName[0]!=0);
			lstrcpyW (ms_Error, L"Internal error. Mapping file name was not specified.");
			return NULL;
		} else {
			mn_Size = (nSize<=0) ? sizeof(T) : nSize;
			mb_WriteAllowed = abCreate || abReadWrite;
			if (abCreate) {
				mh_Mapping = CreateFileMapping(INVALID_HANDLE_VALUE, 
					NullSecurity(), PAGE_READWRITE, 0, mn_Size, ms_MapName);
			} else {
				mh_Mapping = OpenFileMapping(FILE_MAP_READ, FALSE, ms_MapName);
			}
			if (!mh_Mapping) {
				mn_LastError = GetLastError();
				wsprintfW (ms_Error, L"Can't %s console data file mapping. ErrCode=0x%08X\n%s", 
						abCreate ? L"create" : L"open", mn_LastError, ms_MapName);
			} else {
				DWORD nFlags = mb_WriteAllowed ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ;
				mp_Data = (T*)MapViewOfFile(mh_Mapping, nFlags,0,0,0);
				if (!mp_Data) {
					mn_LastError = GetLastError();
					wsprintfW (ms_Error, L"Can't map console info (%s). ErrCode=0x%08X\n%s", 
							mb_WriteAllowed ? L"ReadWrite" : L"Read" ,mn_LastError, ms_MapName);
				}
			}
		}
		return mp_Data;
	};
public:
	T* Create(int nSize=-1) {
		_ASSERTE(nSize==-1 || nSize>=sizeof(T));
		return InternalOpenCreate(TRUE/*abCreate*/,TRUE/*abReadWrite*/,nSize);
	};
	T* Open(BOOL abReadWrite=FALSE/*FALSE - только Read*/,int nSize=-1) {
		_ASSERTE(nSize==-1 || nSize>=sizeof(T));
		return InternalOpenCreate(FALSE/*abCreate*/,abReadWrite,nSize);
	};
public:
	MFileMapping() {
		mh_Mapping = NULL; mb_WriteAllowed = FALSE; mp_Data = NULL; 
		mn_Size = -1; ms_MapName[0] = ms_Error[0] = 0; mn_LastError = 0;
	};
	~MFileMapping() {
		if (mh_Mapping) CloseMap();
	};
};

//class MPipe
//{
//};



#endif // __cplusplus



// This must be the end line
#endif // _COMMON_HEADER_HPP_
