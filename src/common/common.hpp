
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


#ifndef _COMMON_HEADER_HPP_
#define _COMMON_HEADER_HPP_

#include "defines.h"

#define MIN_CON_WIDTH 28
#define MIN_CON_HEIGHT 7
#define GUI_ATTACH_TIMEOUT 5000

// with line number
#if !defined(_MSC_VER)

//#define CONSOLE_APPLICATION_16BIT 1

typedef struct _CONSOLE_SELECTION_INFO
{
	DWORD dwFlags;
	COORD dwSelectionAnchor;
	SMALL_RECT srSelection;
} CONSOLE_SELECTION_INFO, *PCONSOLE_SELECTION_INFO;

#endif



//#define MAXCONMAPCELLS      (600*400)
#define CES_NTVDM 0x10
#define CEC_INITTITLE       L"ConEmu"
//#define CE_CURSORUPDATE     L"ConEmuCursorUpdate%u" // ConEmuC_PID - изменился курсор (размер или выделение). положение курсора отслеживает GUI

// Pipe name formats
#define CESERVERPIPENAME    L"\\\\%s\\pipe\\ConEmuSrv%u"      // ConEmuC_PID
#define CESERVERINPUTNAME   L"\\\\%s\\pipe\\ConEmuSrvInput%u" // ConEmuC_PID
#define CESERVERQUERYNAME   L"\\\\%s\\pipe\\ConEmuSrvQuery%u" // ConEmuC_PID
#define CESERVERWRITENAME   L"\\\\%s\\pipe\\ConEmuSrvWrite%u" // ConEmuC_PID
#define CESERVERREADNAME    L"\\\\%s\\pipe\\ConEmuSrvRead%u"  // ConEmuC_PID
#define CEGUIPIPENAME       L"\\\\%s\\pipe\\ConEmuGui%u"      // GetConsoleWindow() // необходимо, чтобы плагин мог общаться с GUI
#define CEPLUGINPIPENAME    L"\\\\%s\\pipe\\ConEmuPlugin%u"   // Far_PID

#define CEINPUTSEMAPHORE    L"ConEmuInputSemaphore.%08X"      // GetConsoleWindow()

// Mapping name formats
#define CEGUIINFOMAPNAME    L"ConEmuGuiInfoMapping.%u" // --> ConEmuGuiMapping            ( % == dwGuiProcessId )
#define CECONMAPNAME        L"ConEmuFileMapping.%08X"  // --> CESERVER_CONSOLE_MAPPING_HDR ( % == (DWORD)ghConWnd )
#define CECONMAPNAME_A      "ConEmuFileMapping.%08X"   // --> CESERVER_CONSOLE_MAPPING_HDR ( % == (DWORD)ghConWnd ) simplifying ansi
#define CEFARMAPNAME        L"ConEmuFarMapping.%u"     // --> CEFAR_INFO_MAPPING               ( % == nFarPID )
#define CECONVIEWSETNAME    L"ConEmuViewSetMapping.%u" // --> PanelViewSetMapping
#ifdef _DEBUG
#define CEPANELDLGMAPNAME   L"ConEmuPanelViewDlgsMapping.%u" // -> DetectedDialogs     ( % == nFarPID )
#endif

#define CEDATAREADYEVENT    L"ConEmuSrvDataReady.%u"
#define CEFARALIVEEVENT     L"ConEmuFarAliveEvent.%u"
//#define CECONMAPNAMESIZE    (sizeof(CESERVER_REQ_CONINFO)+(MAXCONMAPCELLS*sizeof(CHAR_INFO)))
//#define CEGUIATTACHED       L"ConEmuGuiAttached.%u"
#define CEGUIRCONSTARTED    L"ConEmuGuiRConStarted.%u"
#define CEGUI_ALIVE_EVENT   L"ConEmuGuiStarted"
#define CEKEYEVENT_CTRL     L"ConEmuCtrlPressed.%u"
#define CEKEYEVENT_SHIFT    L"ConEmuShiftPressed.%u"
#define CEHOOKLOCKMUTEX     L"ConEmuHookMutex.%u"
#define CEHOOKDISABLEEVENT  L"ConEmuSkipHooks.%u"


//#define CONEMUMSG_ATTACH L"ConEmuMain::Attach"            // wParam == hConWnd, lParam == ConEmuC_PID
//WARNING("CONEMUMSG_SRVSTARTED нужно переделать в команду пайпа для GUI");
//#define CONEMUMSG_SRVSTARTED L"ConEmuMain::SrvStarted"    // wParam == hConWnd, lParam == ConEmuC_PID
//#define CONEMUMSG_SETFOREGROUND L"ConEmuMain::SetForeground"            // wParam == hConWnd, lParam == ConEmuC_PID
#define CONEMUMSG_FLASHWINDOW L"ConEmuMain::FlashWindow"
//#define CONEMUCMDSTARTED L"ConEmuMain::CmdStarted"    // wParam == hConWnd, lParam == ConEmuC_PID (as ComSpec)
//#define CONEMUCMDSTOPPED L"ConEmuMain::CmdTerminated" // wParam == hConWnd, lParam == ConEmuC_PID (as ComSpec)
//#define CONEMUMSG_LLKEYHOOK L"ConEmuMain::LLKeyHook"    // wParam == hConWnd, lParam == ConEmuC_PID
#define CONEMUMSG_ACTIVATECON L"ConEmuMain::ActivateCon"  // wParam == ConNumber (1..12)
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

typedef DWORD CECMD;
const CECMD
//	CECMD_CONSOLEINFO    = 1,
	CECMD_CONSOLEDATA    = 2,
//	CECMD_SETSIZE        = 3,
	CECMD_CMDSTARTSTOP   = 4, // CESERVER_REQ_STARTSTOP
	CECMD_GETGUIHWND     = 5,
//	CECMD_RECREATE       = 6,
	CECMD_TABSCHANGED    = 7,
	CECMD_CMDSTARTED     = 8, // == CECMD_SETSIZE + восстановить содержимое консоли (запустился comspec)
	CECMD_CMDFINISHED    = 9, // == CECMD_SETSIZE + сохранить содержимое консоли (завершился comspec)
	CECMD_GETOUTPUTFILE  = 10, // Записать вывод последней консольной программы во временный файл и вернуть его имя
	CECMD_GETOUTPUT      = 11,
	CECMD_LANGCHANGE     = 12,
	CECMD_NEWCMD         = 13, // Запустить в этом экземпляре новую консоль с переданной командой (используется при SingleInstance)
	CECMD_TABSCMD        = 14, // 0: спрятать/показать табы, 1: перейти на следующую, 2: перейти на предыдущую, 3: commit switch
	CECMD_RESOURCES      = 15, // Посылается плагином при инициализации (установка ресурсов)
	CECMD_GETNEWCONPARM  = 16, // Доп.аргументы для создания новой консоли (шрифт, размер,...)
	CECMD_SETSIZESYNC    = 17, // Синхронно, ждет (но недолго), пока FAR обработает изменение размера (то есть отрисуется)
	CECMD_ATTACH2GUI     = 18, // Выполнить подключение видимой (отключенной) консоли к GUI. Без аргументов
	CECMD_FARLOADED      = 19, // Посылается плагином в сервер
//	CECMD_SHOWCONSOLE    = 20, // В Win7 релизе нельзя скрывать окно консоли, запущенной в режиме администратора -- заменено на CECMD_POSTCONMSG & CECMD_SETWINDOWPOS
	CECMD_POSTCONMSG     = 21, // В Win7 релизе нельзя посылать сообщения окну консоли, запущенной в режиме администратора
//	CECMD_REQUESTCONSOLEINFO  = 22, // было CECMD_REQUESTFULLINFO
	CECMD_SETFOREGROUND  = 23,
	CECMD_FLASHWINDOW    = 24,
//	CECMD_SETCONSOLECP   = 25,
	CECMD_SAVEALIASES    = 26,
	CECMD_GETALIASES     = 27,
	CECMD_SETSIZENOSYNC  = 28, // Почти CECMD_SETSIZE. Вызывается из плагина.
	CECMD_SETDONTCLOSE   = 29,
	CECMD_REGPANELVIEW   = 30,
	CECMD_ONACTIVATION   = 31, // Для установки флажка ConsoleInfo->bConsoleActive
	CECMD_SETWINDOWPOS   = 32, // CESERVER_REQ_SETWINDOWPOS.
	CECMD_SETWINDOWRGN   = 33, // CESERVER_REQ_SETWINDOWRGN.
	CECMD_SETBACKGROUND  = 34, // CESERVER_REQ_SETBACKGROUND
	CECMD_ACTIVATECON    = 35, // CESERVER_REQ_ACTIVATECONSOLE
//	CECMD_ONSERVERCLOSE  = 35, // Посылается из ConEmuC*.exe перед закрытием в режиме сервера
	CECMD_DETACHCON      = 36,
	CECMD_GUIMACRO       = 38, // CESERVER_REQ_GUIMACRO. Найти в других консолях редактор/вьювер
	CECMD_ONCREATEPROC   = 39, // CESERVER_REQ_ONCREATEPROCESS
	CECMD_SRVSTARTSTOP   = 40, // {DWORD(1/101); DWORD(ghConWnd);}
/** Команды FAR плагина **/
	CMD_FIRST_FAR_CMD    = 200,
	CMD_DRAGFROM         = 200,
	CMD_DRAGTO           = 201,
	CMD_REQTABS          = 202,
	CMD_SETWINDOW        = 203,
	CMD_POSTMACRO        = 204, // Если первый символ макроса '@' и после него НЕ пробел - макрос выполняется в DisabledOutput
//	CMD_DEFFONT          = 205,
	CMD_LANGCHANGE       = 206,
	CMD_FARSETCHANGED    = 207, // Изменились настройки для фара (isFARuseASCIIsort, isShellNoZoneCheck, ...)
//	CMD_SETSIZE          = 208,
	CMD_EMENU            = 209,
	CMD_LEFTCLKSYNC      = 210,
	CMD_REDRAWFAR        = 211,
	CMD_FARPOST          = 212,
	CMD_CHKRESOURCES     = 213,
//	CMD_QUITFAR          = 214, // Дернуть завершение консоли (фара?)
	CMD_CLOSEQSEARCH     = 215,
//	CMD_LOG_SHELL        = 216,
	CMD_SET_CON_FONT     = 217, // CESERVER_REQ_SETFONT
	CMD_GUICHANGED       = 218, // CESERVER_REQ_GUICHANGED. изменились настройки GUI (шрифт), размер окна ConEmu, или еще что-то
	CMD_ACTIVEWNDTYPE    = 219, // ThreadSafe - получить информацию об активном окне (его типе) в Far
	CMD_LAST_FAR_CMD     = CMD_ACTIVEWNDTYPE;


// Версия интерфейса
#define CESERVER_REQ_VER    60

#define PIPEBUFSIZE 4096
#define DATAPIPEBUFSIZE 40000


//// Команды FAR плагина
//typedef DWORD FARCMD;
//const FARCMD
//	CMD_DRAGFROM       = 0,
//	CMD_DRAGTO         = 1,
//	CMD_REQTABS        = 2,
//	CMD_SETWINDOW      = 3,
//	CMD_POSTMACRO      = 4, // Если первый символ макроса '@' и после него НЕ пробел - макрос выполняется в DisabledOutput
////	CMD_DEFFONT        = 5,
//	CMD_LANGCHANGE     = 6,
//	CMD_FARSETCHANGED  = 7, // Изменились настройки для фара (isFARuseASCIIsort, isShellNoZoneCheck, ...)
////	CMD_SETSIZE        = 8,
//	CMD_EMENU          = 9,
//	CMD_LEFTCLKSYNC    = 10,
//	CMD_REDRAWFAR      = 11,
//	CMD_FARPOST        = 12,
//	CMD_CHKRESOURCES   = 13,
////	CMD_QUITFAR        = 14, // Дернуть завершение консоли (фара?)
//	CMD_CLOSEQSEARCH   = 15,
////	CMD_LOG_SHELL      = 16,
//	CMD_SET_CON_FONT   = 17, // CESERVER_REQ_SETFONT
//	CMD_GUICHANGED     = 18, // CESERVER_REQ_GUICHANGED. изменились настройки GUI (шрифт), размер окна ConEmu, или еще что-то
//	CMD_ACTIVEWNDTYPE  = 19; // ThreadSafe - получить информацию об активном окне (его типе) в Far
//// +2
//	//MAXCMDCOUNT        = 21;
//    //CMD_EXIT           = MAXCMDCOUNT-1;



//#pragma pack(push, 1)
#include <pshpack1.h>

typedef unsigned __int64 u64;

struct HWND2
{
	DWORD u;
	operator HWND() const
	{
		return (HWND)u;
	};
	operator DWORD() const
	{
		return (DWORD)u;
	};
	struct HWND2& operator=(HWND h)
	{
		u = (DWORD)h;
		return *this;
	};
};

struct HKEY2
{
	DWORD u;
	operator HKEY() const
	{
		return (HKEY)(DWORD_PTR)(LONG)u;
	};
	operator DWORD() const
	{
		return u;
	};
	struct HKEY2& operator=(HKEY h)
	{
		u = (DWORD)(LONG)(LONG_PTR)h;
		return *this;
	};
};

struct MSG64
{
	UINT  message;
	HWND2 hwnd;
	u64   wParam;
	u64   lParam;
};

struct ThumbColor
{
	union
	{
		struct
		{
			unsigned int   ColorRGB : 24;
			unsigned int   ColorIdx : 5; // 5 bits, to support value '16' - automatic use of panel color
			unsigned int   UseIndex : 1; // TRUE - Use ColorRef, FALSE - ColorIdx
		};
		DWORD RawColor;
	};
};

struct ThumbSizes
{
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
};


enum PanelViewMode
{
	pvm_None = 0,
	pvm_Thumbnails = 1,
	pvm_Tiles = 2,
	// следующий режим (если он будет) делать 4! (это bitmask)
};

/* Основной шрифт в GUI */
struct ConEmuMainFont
{
	wchar_t sFontName[32];
	DWORD nFontHeight, nFontWidth, nFontCellWidth;
	DWORD nFontQuality, nFontCharSet;
	BOOL Bold, Italic;
	wchar_t sBorderFontName[32];
	DWORD nBorderFontWidth;
};

struct PanelViewSetMapping
{
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

	/* Основной шрифт в GUI */
	struct ConEmuMainFont MainFont;

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
};


typedef BOOL (WINAPI* PanelViewInputCallback)(HANDLE hInput, PINPUT_RECORD lpBuffer, DWORD nBufSize, LPDWORD lpNumberOfEventsRead, BOOL* pbResult);
typedef union uPanelViewInputCallback
{
	u64 Reserved; // необходимо для выравнивания структур при x64 <--> x86
	PanelViewInputCallback f;
} PanelViewInputCallback_t;
typedef BOOL (WINAPI* PanelViewOutputCallback)(HANDLE hOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
typedef union uPanelViewOutputCallback
{
	u64 Reserved; // необходимо для выравнивания структур при x64 <--> x86
	PanelViewOutputCallback f;
} PanelViewOutputCallback_t;
struct PanelViewText
{
	// Флаг используемости, выравнивание текста, и др.
#define PVI_TEXT_NOTUSED 0
#define PVI_TEXT_LEFT    1
#define PVI_TEXT_CENTER  2
#define PVI_TEXT_RIGHT   4
#define PVI_TEXT_SKIPSORTMODE 0x100 // только для имени колонки (оставить ФАРовскую буковку сортировки с ^)
	DWORD nFlags;
	// используется только младший байт - индексы консольных цветов
	DWORD bConAttr;
	// собственно текст
	wchar_t sText[128];
};
struct PanelViewInit
{
	DWORD cbSize;
	BOOL  bRegister, bVisible;
	HWND2 hWnd;
	BOOL  bLeftPanel;
	// Flags
#define PVI_COVER_NORMAL         0x000 // как обычно, располагаем прямоугольником, в который ФАР выводит файлы
#define PVI_COVER_SCROLL_CLEAR   0x001 // при отрисовке GUI очистит полосу прокрутки (заменит на вертикальную-двойную)
#define PVI_COVER_SCROLL_OVER    0x002 // PVI_COVER_NORMAL + правая рамка (где полоса прокрутки). Плагин должен отрисовать прокрутку сам
#define PVI_COVER_COLTITLE_OVER  0x004 // PVI_COVER_NORMAL + строка с заголовками колонок (если она есть)
#define PVI_COVER_FULL_OVER      0x008 // Вся РАБОЧАЯ область панели (то есть панель - рамка)
#define PVI_COVER_FRAME_OVER     0x010 // Вся область панели (включая рамку)
#define PVI_COVER_2PANEL_OVER    0x020 // Обе панели (полноэкранная)
#define PVI_COVER_CONSOLE_OVER   0x040 // Консоль целиком
	DWORD nCoverFlags;
	// FAR settings
	DWORD nFarInterfaceSettings;
	DWORD nFarPanelSettings;
	// Координаты всей панели (левой, правой, или fullscreen)
	RECT  PanelRect;
	// Разнообразные текстовые затиралки
	PanelViewText tPanelTitle, tColumnTitle, tInfoLine[3];
	// Это координаты прямоугольника, в котором реально располагается View
	// Координаты определяются в GUI. Единицы - консольные.
	RECT  WorkRect;
	// Callbacks, используются только в плагинах
	PanelViewInputCallback_t pfnPeekPreCall, pfnPeekPostCall, pfnReadPreCall, pfnReadPostCall;
	PanelViewOutputCallback_t pfnWriteCall;
	/* out */
	//PanelViewSetMapping ThSet;
	BOOL bFadeColors;
};



// События, при которых вызывается функция субплагина для перерисовки фона
enum PaintBackgroundEvents
{
	pbe_Common = 0,              // при изменении размера окна ConEmu/размера шрифта/палитры ConEmu/...
	pbe_PanelDirectory = 1,      // при смене папки
	//TODO, by request
	//bue_LeftDirChanged = 1,      // на левой панели изменена текущая папка
	//bue_RightDirChanged = 2,     // на правой панели изменена текущая папка
	//bue_FocusChanged = 4,        // фокус переключился между левой/правой панелью
};

enum PaintBackgroundPlaces
{
	// Используются при регистрации И отрисовке
	pbp_Panels = 1,
	pbp_Editor = 2,
	pbp_Viewer = 4,
	// Используются ТОЛЬКО как [OUT] при отрисовке // Reserved
	pbp_UserScreen  = 8,
	pbp_CommandLine = 16,
	pbp_KeyBar      = 32,
	pbp_MenuBar     = 64, // Верхняя строка - падающее меню
	pbp_StatusLine  = 64, // Статусная строка - редактор/вьювер
};

#define BkPanelInfo_CurDirMax 32768
#define BkPanelInfo_FormatMax MAX_PATH
#define BkPanelInfo_HostFileMax 32768

struct PaintBackgroundArg
{
	DWORD cbSize;

	// указан при вызове RegisterBackground(rbc_Register)
	LPARAM lParam;

	// панели/редактор/вьювер
	enum PaintBackgroundPlaces Place;

	// Слой, в котором вызван плагин. По сути, это порядковый номер, 0-based
	// если (dwLevel > 0) плагин НЕ ДОЛЖЕН затирать фон целиком.
	DWORD dwLevel;
	// [Reserved] комбинация из enum PaintBackgroundEvents
	DWORD dwEventFlags;

	// Основной шрифт в ConEmu GUI
	struct ConEmuMainFont MainFont;
	// Палитра в ConEmu GUI
	COLORREF crPalette[16];
	// Reserved
	DWORD dwReserved[20];


	// DC для отрисовки фона. Изначально (для dwLevel==0) DC залит цветом фона crPalette[0]
	HDC hdc;
	// размер DC в пикселях
	int dcSizeX, dcSizeY;
	// Координаты панелей в DC (base 0x0)
	RECT rcDcLeft, rcDcRight;


	// Для облегчения жизни плагинам - текущие параметры FAR
	RECT rcConWorkspace; // Кооринаты рабочей области FAR. В FAR 2 с ключом /w верх может быть != {0,0}
	COORD conCursor; // положение курсора, или {-1,-1} если он не видим
	DWORD nFarInterfaceSettings; // ACTL_GETINTERFACESETTINGS
	DWORD nFarPanelSettings; // ACTL_GETPANELSETTINGS
	BYTE nFarColors[0x100]; // Массив цветов фара

	// Инфорация о панелях
	BOOL bPanelsAllowed;
	typedef struct tag_BkPanelInfo
	{
		BOOL bVisible; // Наличие панели
		BOOL bFocused; // В фокусе
		BOOL bPlugin;  // Плагиновая панель
		wchar_t *szCurDir/*[32768]*/;    // Текущая папка на панели
		wchar_t *szFormat/*[MAX_PATH]*/; // Доступно только в FAR2
		wchar_t *szHostFile/*[32768]*/;  // Доступно только в FAR2
		RECT rcPanelRect; // Консольные кооринаты панели. В FAR 2 с ключом /w верх может быть != {0,0}
	} BkPanelInfo;
	BkPanelInfo LeftPanel;
	BkPanelInfo RightPanel;

	// [OUT] Плагин должен указать, какие части консоли он "раскрасил" - enum PaintBackgroundPlaces
	DWORD dwDrawnPlaces;
};

typedef int (WINAPI* PaintConEmuBackground_t)(struct PaintBackgroundArg* pBk);

// Если функция вернет 0 - обновление фона пока не требуется
typedef int (WINAPI* BackgroundTimerProc_t)(LPARAM lParam);

enum RegisterBackgroundCmd
{
	rbc_Register   = 1, // Первичная регистрации "фонового" плагина
	rbc_Unregister = 2, // Убрать плагин из списка "фоновых"
	rbc_Redraw     = 3, // Если плагину требуется обновить фон - он зовет эту команду
};

// Аргумент для функции: int WINAPI RegisterBackground(RegisterBackgroundArg *pbk)
struct RegisterBackgroundArg
{
	DWORD cbSize;
	enum RegisterBackgroundCmd Cmd; // DWORD
	HMODULE hPlugin; // Instance плагина, содержащего PaintConEmuBackground

	// Для дерегистрации всех калбэков достаточно вызвать {sizeof(RegisterBackgroundArg), rbc_Unregister, hPlugin}

	// Что вызывать для обновления фона.
	// Требуется заполнять только для Cmd==rbc_Register,
	// в остальных случаях команды игнорируются

	// Плагин может зарегистрировать несколько различных пар {PaintConEmuBackground,lParam}
	PaintConEmuBackground_t PaintConEmuBackground; // Собственно калбэк
	LPARAM lParam; // lParam будет передан в PaintConEmuBackground

	DWORD  dwPlaces;     // bitmask of PaintBackgroundPlaces
	DWORD  dwEventFlags; // bitmask of PaintBackgroundEvents

	// 0 - плагин предпочитает отрисовывать фон первым. Чем больше nSuggestedLevel
	// тем позднее может быть вызван плагин при обновлении фона
	DWORD  dwSuggestedLevel;

	// Необязательный калбэк таймера
	BackgroundTimerProc_t BackgroundTimerProc;
	// Рекомендованная частота опроса (ms)
	DWORD nBackgroundTimerElapse;
};

typedef int (WINAPI* RegisterBackground_t)(RegisterBackgroundArg *pbk);

typedef void (WINAPI* SyncExecuteCallback_t)(LONG_PTR lParam);


struct FarVersion
{
	union
	{
		DWORD dwVer;
		struct
		{
			WORD dwVerMinor;
			WORD dwVerMajor;
		};
	};
	DWORD dwBuild;
};


// Аргумент для функции OnConEmuLoaded
struct ConEmuLoadedArg
{
	DWORD cbSize;    // размер структуры в байтах
	DWORD nBuildNo;  // {Номер сборки ConEmu*10} (i.e. 1012070). Это версия плагина. В принципе, версия GUI может отличаться
	HMODULE hConEmu; // conemu.dll / conemu.x64.dll
	HMODULE hPlugin; // для информации - Instance этого плагина, в котором вызывается OnConEmuLoaded
	BOOL bLoaded;    // TRUE - при загрузке conemu.dll, FALSE - при выгрузке
	BOOL bGuiActive; // TRUE - консоль запущена из-под ConEmu, FALSE - standalone

	/* ***************** */
	/* Сервисные функции */
	/* ***************** */
	HWND (WINAPI *GetFarHWND)();
	HWND (WINAPI *GetFarHWND2)(BOOL abConEmuOnly);
	void (WINAPI *GetFarVersion)(FarVersion* pfv);
	BOOL (WINAPI *IsTerminalMode)();
	BOOL (WINAPI *IsConsoleActive)();
	int (WINAPI *RegisterPanelView)(PanelViewInit *ppvi);
	int (WINAPI *RegisterBackground)(RegisterBackgroundArg *pbk);
	// Activate console tab in ConEmu
	int (WINAPI *ActivateConsole)();
	// Synchronous execute CallBack in Far main thread (FAR2 use ACTL_SYNCHRO).
	// SyncExecute does not returns, until CallBack finished.
	// ahModule must be module handle, wich contains CallBack
	int (WINAPI *SyncExecute)(HMODULE ahModule, SyncExecuteCallback_t CallBack, LONG_PTR lParam);
};

// другие плагины могут экспортировать функцию "OnConEmuLoaded"
// при активации плагина ConEmu (при запуске фара из-под ConEmu)
// эта функция ("OnConEmuLoaded") будет вызвана, в ней плагин может
// зарегистрировать калбэки для обновления Background-ов
// Внимание!!! Эта же функция вызывается при ВЫГРУЗКЕ conemu.dll,
// при выгрузке hConEmu и все функции == NULL
typedef void (WINAPI* OnConEmuLoaded_t)(struct ConEmuLoadedArg* pConEmuInfo);

enum GuiLoggingType
{
	glt_None = 0,
	glt_Processes = 1,
	// glt_Keyboard, glt_Files, ...
};

struct ConEmuGuiMapping
{
	DWORD    cbSize;
	DWORD    nProtocolVersion; // == CESERVER_REQ_VER
	HWND2    hGuiWnd; // основное (корневое) окно ConEmu
	
	DWORD    nLoggingType; // enum GuiLoggingType
	
	wchar_t  sConEmuExe[MAX_PATH+1]; // полный путь к ConEmu.exe (GUI)
	wchar_t  sConEmuDir[MAX_PATH+1]; // БЕЗ завершающего слеша. Папка содержит ConEmu.exe
	wchar_t  sConEmuBaseDir[MAX_PATH+1]; // БЕЗ завершающего слеша. Папка содержит ConEmuC.exe, ConEmuHk.dll, ConEmu.xml
	wchar_t  sConEmuArgs[MAX_PATH*2];

	/* Основной шрифт в GUI */
	struct ConEmuMainFont MainFont;
	
	// Перехват реестра
	BOOL    bHookRegistry;
	wchar_t sHiveFileName[MAX_PATH];
	HKEY2   hMountRoot;    // NULL для Vista+, для Win2k&XP здесь хранится корневой ключ (HKEY_USERS), в который загружен hive
	wchar_t sMountKey[64]; // Для Win2k&XP здесь хранится имя ключа, в который загружен hive
	
	// DosBox
	BOOL     bDosBox; // наличие DosBox
	wchar_t  sDosBoxExe[MAX_PATH+1]; // полный путь к DosBox.exe
	wchar_t  sDosBoxEnv[8192]; // команды загрузки (mount, и пр.)

	//DWORD    bUseInjects; // 0-off, 1-on. Далее могут быть доп.флаги (битмаск)? chcp, Hook HKCU\FAR[2] & HKLM\FAR and translate them to hive, ...
	//wchar_t  sInjectsDir[MAX_PATH+1]; // path to "conemu.dll" & "conemu.x64.dll"
	// Для облегчения Inject-ов наверное можно сразу пути в конкретным файлам заполнить.
	//wchar_t  sInjects32[MAX_PATH+16], sInjects64[MAX_PATH+16]; // path to "ConEmuHk.dll" & "ConEmuHk64.dll"
	// Kernel32 загружается по фиксированному адресу, НО
	// для 64-битной программы он один, а для 32-битной ест-но другой.
	// Поэтому в 64-битных системых НЕОБХОДИМО пользоваться 64-битной версией ConEmu.exe
	// которая сможет корректно определить адрес для 64-битного kernel,
	// а адрес 32-битного kernel сможет вытащить через его экспорты.
	//ULONGLONG ptrLoadLib32, ptrLoadLib64;
	//// Логирование CreateProcess, ShellExecute, и прочих запускающих функций
	//// Если пусто - не логируется
	//wchar_t  sLogCreateProcess[MAX_PATH];
};


//TODO("Restrict CONEMUTABMAX to 128 chars. Only filename, and may be ellipsed...");
#define CONEMUTABMAX 0x400
struct ConEmuTab
{
	int  Pos;
	int  Current;
	int  Type; // (Panels=1, Viewer=2, Editor=3) | (Elevated=0x100)
	int  Modified;
	int  EditViewId;
	wchar_t Name[CONEMUTABMAX];
	//  int  Modified;
	//  int isEditor;
};

struct CESERVER_REQ_CONEMUTAB
{
	DWORD nTabCount;
	BOOL  bMacroActive;
	BOOL  bMainThread;
	int   CurrentType; // WTYPE_PANELS / WTYPE_VIEWER / WTYPE_EDITOR
	int   CurrentIndex; // для удобства, индекс текущего окна в tabs
	ConEmuTab tabs[1];
};

struct CESERVER_REQ_CONEMUTAB_RET
{
	BOOL  bNeedPostTabSend;
	BOOL  bNeedResize;
	COORD crNewSize;
};

struct ForwardedPanelInfo
{
	RECT ActiveRect;
	RECT PassiveRect;
	int ActivePathShift; // сдвиг в этой структуре в байтах
	int PassivePathShift; // сдвиг в этой структуре в байтах
	union //x64 ready
	{
		WCHAR* pszActivePath/*[MAX_PATH+1]*/;
		u64 Reserved1;
	};
	union //x64 ready
	{
		WCHAR* pszPassivePath/*[MAX_PATH+1]*/;
		u64 Reserved2;
	};
};

struct ForwardedFileInfo
{
	WCHAR Path[MAX_PATH+1];
};


struct CESERVER_REQ_HDR
{
	DWORD   cbSize;
	CECMD   nCmd;
	DWORD   nVersion;
	DWORD   nSrcThreadId;
	DWORD   nSrcPID;
	DWORD   nCreateTick;
};


struct CESERVER_CHAR_HDR
{
	int   nSize;    // размер структуры динамический. Если 0 - значит прямоугольник is NULL
	COORD cr1, cr2; // WARNING: Это АБСОЛЮТНЫЕ координаты (без учета прокрутки), а не экранные.
};

struct CESERVER_CHAR
{
	CESERVER_CHAR_HDR hdr; // фиксированная часть
	WORD  data[2];  // variable(!) length
};

struct CESERVER_CONSAVE_HDR
{
	CESERVER_REQ_HDR hdr; // Заполняется только перед отсылкой в плагин
	CONSOLE_SCREEN_BUFFER_INFO sbi;
	DWORD cbMaxOneBufferSize;
};

struct CESERVER_CONSAVE
{
	CESERVER_CONSAVE_HDR hdr;
	wchar_t Data[1];
};



struct CESERVER_REQ_RGNINFO
{
	DWORD dwRgnInfoSize;
	CESERVER_CHAR RgnInfo;
};

struct CESERVER_REQ_FULLCONDATA
{
	DWORD dwRgnInfoSize_MustBe0; // must be 0
	DWORD dwOneBufferSize; // may be 0
	wchar_t Data[300]; // Variable length!!!
};

struct CEFAR_SHORT_PANEL_INFO
{
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
	unsigned __int64 Flags;
};

struct CEFAR_PANELTABS
{
	int   SeparateTabs; // если -1 - то умолчание
	int   ButtonColor;  // если -1 - то умолчание
};

struct CEFAR_INFO_MAPPING
{
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
	BOOL bViewerOrEditor; // для облегчения жизни RgnDetect
	DWORD nFarConsoleMode;
	BOOL bBufferSupport; // FAR2 с ключом /w ?
	CEFAR_PANELTABS PanelTabs; // Настройки плагина PanelTabs
	//DWORD nFarReadIdx;    // index, +1, когда фар в последний раз позвал (Read|Peek)ConsoleInput или GetConsoleInputCount
	// Далее идут строковые ресурсы, на которые в некоторых случаях ориентируется GUI
	wchar_t sLngEdit[64]; // "edit"
	wchar_t sLngView[64]; // "view"
	wchar_t sLngTemp[64]; // "{Temporary panel"
	//wchar_t sLngName[64]; // "Name"
	wchar_t sReserved[MAX_PATH]; // Чтобы случайно GetMsg из допустимого диапазона не вышел
};


struct CESERVER_CONSOLE_MAPPING_HDR
{
	DWORD cbSize;
	DWORD nLogLevel;
	COORD crMaxConSize;
	DWORD bDataReady;  // TRUE, после того как сервер успешно запустился и готов отдавать данные
	HWND2 hConWnd;     // !! положение hConWnd и nGuiPID не менять !!
	DWORD nServerPID;  //
	DWORD nGuiPID;     // !! на них ориентируется PicViewWrapper   !!
	//
	DWORD bConsoleActive;
	DWORD nProtocolVersion; // == CESERVER_REQ_VER
	DWORD bThawRefreshThread; // FALSE - увеличивает интервал опроса консоли (GUI теряет фокус)
	//
	DWORD nFarPID; // PID последнего активного фара
	//
	DWORD nServerInShutdown; // GetTickCount() начала закрытия сервера
	//
	//DWORD    bUseInjects; // 0-off, 1-on. Далее могут быть доп.флаги (битмаск)? chcp, Hook HKCU\FAR[2] & HKLM\FAR and translate them to hive, ...
	//wchar_t  sConEmuDir[MAX_PATH+1];  // здесь будет лежать собственно hive
	//wchar_t  sInjectsDir[MAX_PATH+1]; // path to "ConEmuHk.dll" & "ConEmuHk64.dll"
	DWORD nAltServerPID;  //

	// Root(!) ConEmu window
	HWND2 hConEmuRoot;
	// DC ConEmu window
	HWND2 hConEmuWnd;
	
	// Перехват реестра
	BOOL    bHookRegistry;
	wchar_t sHiveFileName[MAX_PATH];
	HKEY2   hMountRoot;    // NULL для Vista+, для Win2k&XP здесь хранится корневой ключ (HKEY_USERS), в который загружен hive
	wchar_t sMountKey[64]; // Для Win2k&XP здесь хранится имя ключа, в который загружен hive

	//// Логирование CreateProcess, ShellExecute, и прочих запускающих функций
	//// Если пусто - не логируется
	//wchar_t  sLogCreateProcess[MAX_PATH];
};

struct CESERVER_REQ_CONINFO_INFO
{
	CESERVER_REQ_HDR cmd;
	//DWORD cbSize;
	HWND2 hConWnd;
	//DWORD nGuiPID;
	//DWORD nCurDataMapIdx; // суффикс для текущего MAP файла с данными
	//DWORD nCurDataMaxSize; // Максимальный размер буфера nCurDataMapIdx
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
	COORD crWindow; // Для удобства - размер ОКНА (не буфера) при последнем ReadConsoleData
	COORD crMaxSize; // Максимальный размер консоли в символах (для текущего выбранного шрифта)
	DWORD nDataShift; // Для удобства - сдвиг начала данных (data) От начала info
	DWORD nDataCount; // Для удобства - количество ячеек (data)
	//// Информация о текущем FAR
	//DWORD nFarInfoIdx; // выносим из структуры CEFAR_INFO_MAPPING, т.к. ее копия хранится в плагине
	//CEFAR_INFO_MAPPING FarInfo;
};

//typedef struct tag_CESERVER_REQ_CONINFO_DATA {
//	CESERVER_REQ_HDR cmd;
//	COORD      crMaxSize;
//	CHAR_INFO  Buf[1];
//} CESERVER_REQ_CONINFO_DATA;

struct CESERVER_REQ_CONINFO_FULL
{
	DWORD cbMaxSize;    // размер всего буфера CESERVER_REQ_CONINFO_FULL (скорее всего будет меньше реальных данных)
	//DWORD cbActiveSize; // размер реальных данных CESERVER_REQ_CONINFO_FULL, а не всего буфера data
	//BOOL  bChanged;     // флаг того, что данные изменились с последней передачи в GUI
	BOOL  bDataChanged; // Выставляется в TRUE, при изменениях содержимого консоли (а не только положение курсора...)
	CESERVER_CONSOLE_MAPPING_HDR  hdr;
	CESERVER_REQ_CONINFO_INFO info;
	//CESERVER_REQ_CONINFO_DATA data;
	CHAR_INFO  data[1];
};

//typedef struct tag_CESERVER_REQ_CONINFO {
//	CESERVER_CONSOLE_MAPPING_HDR inf;
//    union {
//	/* 9*/DWORD dwRgnInfoSize;
//	      CESERVER_REQ_RGNINFO RgnInfo;
//    /*10*/CESERVER_REQ_FULLCONDATA FullData;
//	};
//} CESERVER_REQ_CONINFO;

struct CESERVER_REQ_SETSIZE
{
	USHORT nBufferHeight; // 0 или высота буфера (режим с прокруткой)
	COORD  size;
	SHORT  nSendTopLine;  // -1 или 0based номер строки зафиксированной в GUI (только для режима с прокруткой)
	SMALL_RECT rcWindow;  // координаты видимой области для режима с прокруткой
	DWORD  dwFarPID;      // Если передано - сервер должен сам достучаться до FAR'а и обновить его размер через плагин ПЕРЕД возвратом
};

struct CESERVER_REQ_OUTPUTFILE
{
	BOOL  bUnicode;
	WCHAR szFilePathName[MAX_PATH+1];
};

struct CESERVER_REQ_RETSIZE
{
	DWORD nNextPacketId;
	CONSOLE_SCREEN_BUFFER_INFO SetSizeRet;
};

struct CESERVER_REQ_NEWCMD
{
	wchar_t szCurDir[MAX_PATH];
	wchar_t szCommand[MAX_PATH]; // На самом деле - variable_size !!!
};

struct CESERVER_REQ_SETFONT
{
	DWORD cbSize; // страховка
	int inSizeY;
	int inSizeX;
	wchar_t sFontName[LF_FACESIZE];
};

// изменились настройки GUI (шрифт), размер окна ConEmu, или еще что-то
struct CESERVER_REQ_GUICHANGED
{
	DWORD cbSize; // страховка
	DWORD nGuiPID;
	HWND2 hLeftView, hRightView;
};

enum StartStopType
{
	sst_ServerStart  = 0,
	sst_ServerStop   = 1,
	sst_ComspecStart = 2,
	sst_ComspecStop  = 3,
	sst_AppStart     = 4,
	sst_AppStop      = 5,
	sst_App16Start   = 6,
	sst_App16Stop    = 7,
};
struct CESERVER_REQ_STARTSTOP
{
	DWORD nStarted; // StartStopType
	HWND2 hWnd; // при передаче В GUI - консоль, при возврате в консоль - GUI
	DWORD dwPID; //, dwInputTID;
	DWORD nSubSystem; // 255 для DOS программ, 0x100 - аттач из FAR плагина
	DWORD nImageBits; // 16/32/64
	BOOL  bRootIsCmdExe;
	BOOL  bUserIsAdmin;
	// А это приходит из консоли, вдруго консольная программа успела поменять размер буфера
	CONSOLE_SCREEN_BUFFER_INFO sbi;
	// Только ComSpec
	BOOL  bWasBufferHeight;
	// Только при аттаче. Может быть NULL-ом
	u64   hServerProcessHandle;
	// Для информации и удобства (GetModuleFileName(0))
	wchar_t sModuleName[MAX_PATH+1];
	// Reserved
	DWORD nReserved0[20];
};

// CECMD_ONCREATEPROC
struct CESERVER_REQ_ONCREATEPROCESS
{
	//BOOL    bUnicode;
	wchar_t sFunction[32];
	int     nImageSubsystem;
	int     nImageBits;
	DWORD   nShellFlags; // Флаги ShellExecuteEx
	DWORD   nCreateFlags, nStartFlags; // Флаги CreateProcess
	DWORD   nShowCmd;
	int     nActionLen;
	int     nFileLen;
	int     nParamLen;
	unsigned __int64 hStdIn, hStdOut, hStdErr;
	// Variable length tail
	wchar_t wsValue[1];
};
struct CESERVER_REQ_ONCREATEPROCESSRET
{
	BOOL  bContinue;
	//BOOL  bUnicode;
	BOOL  bForceBufferHeight;
	BOOL  bAllowDosbox;
	//int   nFileLen;
	//int   nBaseLen;
	//union
	//{
	//	wchar_t wsValue[1];
	//	char sValue[1];
	//};
};

// _ASSERTE(sizeof(CESERVER_REQ_STARTSTOPRET) <= sizeof(CESERVER_REQ_STARTSTOP));
struct CESERVER_REQ_STARTSTOPRET
{
	BOOL  bWasBufferHeight;
	HWND2 hWnd; // при возврате в консоль - GUI (главное окно)
	HWND2 hWndDC;
	DWORD dwPID; // при возврате в консоль - PID ConEmu.exe
	DWORD nBufferHeight, nWidth, nHeight;
	DWORD dwSrvPID;
	BOOL  bNeedLangChange;
	u64   NewConsoleLang;
	// Используется при CECMD_ATTACH2GUI
	CESERVER_REQ_SETFONT Font;
};

struct CESERVER_REQ_POSTMSG
{
	BOOL    bPost;
	HWND2   hWnd;
	UINT    nMsg;
	// Заложимся на унификацию x86 & x64
	u64     wParam, lParam;
};

struct CESERVER_REQ_FLASHWINFO
{
	BOOL  bSimple;
	HWND2 hWnd;
	BOOL  bInvert; // только если bSimple == TRUE
	DWORD dwFlags; // а это и далее, если bSimple == FALSE
	UINT  uCount;
	DWORD dwTimeout;
};

// CMD_FARCHANGED - FAR plugin
struct FAR_REQ_FARSETCHANGED
{
	BOOL    bFARuseASCIIsort;
	BOOL    bShellNoZoneCheck; // Затычка для SEE_MASK_NOZONECHECKS
	//wchar_t szEnv[1]; // Variable length: <Name>\0<Value>\0<Name2>\0<Value2>\0\0
};

struct CESERVER_REQ_SETCONCP
{
	BOOL    bSetOutputCP; // [IN], [Out]=result
	DWORD   nCP;          // [IN], [Out]=LastError
};

struct CESERVER_REQ_SETWINDOWPOS
{
	HWND2 hWnd;
	HWND2 hWndInsertAfter;
	int X;
	int Y;
	int cx;
	int cy;
	UINT uFlags;
};

struct CESERVER_REQ_SETWINDOWRGN
{
	HWND2 hWnd;
	int   nRectCount;  // если 0 - сбросить WindowRgn, иначе - обсчитать.
	BOOL  bRedraw;
	RECT  rcRects[20]; // [0] - основной окна, [
};

// CECMD_SETBACKGROUND - приходит из плагина "PanelColorer.dll"
// Warning! Structure has variable length. "bmp" field must be followed by bitmap data (same as in *.bmp files)
struct CESERVER_REQ_SETBACKGROUND
{
	int               nType;    // Reserved for future use. Must be 1
	BOOL              bEnabled; // TRUE - ConEmu use this image, FALSE - ConEmu use self background settings

	// какие части консоли раскрашены - enum PaintBackgroundPlaces
	DWORD dwDrawnPlaces;

	//int               nReserved1; // Must by 0. reserved for alpha
	//DWORD             nReserved2; // Must by 0. reserved for replaced colors
	//int               nReserved3; // Must by 0. reserved for background op
	//DWORD nReserved4; // Must by 0. reserved for flags (BmpIsTransparent, RectangleSpecified)
	//DWORD nReserved5; // Must by 0. reserved for level (Some plugins may want to draw small parts over common background)
	//RECT  rcReserved5; // Must by 0. reserved for filled rect (plugin may cover only one panel, or part of it)

	BITMAPFILEHEADER  bmp;
	BITMAPINFOHEADER  bi;
};

// ConEmu respond for CESERVER_REQ_SETBACKGROUND
typedef int SetBackgroundResult;
const SetBackgroundResult
	esbr_OK = 0,               // All OK
	esbr_InvalidArg = 1,       // Invalid *RegisterBackgroundArg
	esbr_PluginForbidden = 2,  // "Allow plugins" unchecked in ConEmu settings ("Main" page)
	esbr_ConEmuInShutdown = 3, // Console is closing. This is not an error, just information
	esbr_Unexpected = 4,       // Unexpected error in ConEmu
	esbr_InvalidArgSize = 5,   // Invalid RegisterBackgroundArg.cbSize
	esbr_InvalidArgProc = 6,   // Invalid RegisterBackgroundArg.PaintConEmuBackground
	esbr_LastErrorNo = esbr_InvalidArgProc;

struct CESERVER_REQ_SETBACKGROUNDRET
{
	SetBackgroundResult  nResult;
};

// CECMD_ACTIVATECON.
struct CESERVER_REQ_ACTIVATECONSOLE
{
	HWND2 hConWnd;
};

// CECMD_GUIMACRO
#define CEGUIMACRORETENVVAR L"ConEmuMacroResult"
struct CESERVER_REQ_GUIMACRO
{
	DWORD   nSucceeded;
	wchar_t sMacro[1];    // Variable length
};

struct CESERVER_REQ
{
	CESERVER_REQ_HDR hdr;
	union
	{
		BYTE    Data[1]; // variable(!) length
		WORD    wData[1];
		DWORD   dwData[1];
		u64     qwData[1];
		CESERVER_CONSOLE_MAPPING_HDR ConInfo;
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
		FAR_REQ_FARSETCHANGED FarSetChanged;
		CESERVER_REQ_SETCONCP SetConCP;
		CESERVER_REQ_SETWINDOWPOS SetWndPos;
		CESERVER_REQ_SETWINDOWRGN SetWndRgn;
		CESERVER_REQ_SETBACKGROUND Background;
		CESERVER_REQ_SETBACKGROUNDRET BackgroundRet;
		CESERVER_REQ_ACTIVATECONSOLE ActivateCon;
		PanelViewInit PVI;
		CESERVER_REQ_SETFONT Font;
		CESERVER_REQ_GUIMACRO GuiMacro;
		CESERVER_REQ_ONCREATEPROCESS OnCreateProc;
		CESERVER_REQ_ONCREATEPROCESSRET OnCreateProcRet;
	};
};


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

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
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

//#define MAX_INPUT_QUEUE_EMPTY_WAIT 1000

int NextArg(const wchar_t** asCmdLine, wchar_t (&rsArg)[MAX_PATH+1], const wchar_t** rsArgStart=NULL);
int NextArg(const char** asCmdLine, char (&rsArg)[MAX_PATH+1], const char** rsArgStart=NULL);
const wchar_t* SkipNonPrintable(const wchar_t* asParams);
BOOL PackInputRecord(const INPUT_RECORD* piRec, MSG64* pMsg);
BOOL UnpackInputRecord(const MSG64* piMsg, INPUT_RECORD* pRec);
SECURITY_ATTRIBUTES* NullSecurity();
SECURITY_ATTRIBUTES* LocalSecurity();
void CommonShutdown();


#ifndef _CRT_WIDE
#define __CRT_WIDE(_String) L ## _String
#define _CRT_WIDE(_String) __CRT_WIDE(_String)
#endif

#include "MAssert.h"

#endif // __cplusplus



// This must be the end line
#endif // _COMMON_HEADER_HPP_
