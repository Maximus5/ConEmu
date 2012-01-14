
/*
Copyright (c) 2009-2012 Maximus5
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

#pragma once
//#include "kl_parts.h"
//#include "../Common/common.hpp"
#include "../Common/ConsoleAnnotation.h"
#include "../Common/RgnDetect.h"
//#include "../Common/WinObjects.h"

#define DEFINE_EXIT_DESC
#include "../ConEmuCD/ExitCodes.h"

#define CES_CMDACTIVE 0x01
#define CES_TELNETACTIVE 0x02
#define CES_FARACTIVE 0x04
#define CES_CONALTERNATIVE 0x08
#define CES_PROGRAMS (0x0F)

//#define CES_NTVDM 0x10 -- common.hpp
#define CES_PROGRAMS2 0xFF

#define CES_FILEPANEL      0x0001
#define CES_FARPANELS      0x000F // на будущее, должен содержать все возможные флаги возможных панелей
//#define CES_TEMPPANEL      0x0002
//#define CES_PLUGINPANEL    0x0004
#define CES_EDITOR         0x0010
#define CES_VIEWER         0x0020
#define CES_COPYING        0x0040
#define CES_MOVING         0x0080
#define CES_NOTPANELFLAGS  0xFFF0
#define CES_FARFLAGS       0xFFFF
#define CES_MAYBEPANEL   0x010000
#define CES_WASPROGRESS  0x020000
#define CES_OPER_ERROR   0x040000
//... and so on

//// Undocumented console message
//#define WM_SETCONSOLEINFO           (WM_USER+201)
//// and others
//#define SC_RESTORE_SECRET 0x0000f122
//#define SC_MAXIMIZE_SECRET 0x0000f032
//#define SC_PROPERTIES_SECRET 0x0000fff7
//#define SC_MARK_SECRET 0x0000fff2
//#define SC_COPY_ENTER_SECRET 0x0000fff0
//#define SC_PASTE_SECRET 0x0000fff1
//#define SC_SELECTALL_SECRET 0x0000fff5
//#define SC_SCROLL_SECRET 0x0000fff3
//#define SC_FIND_SECRET 0x0000fff4

//#define MAX_TITLE_SIZE 0x400

#define FAR_ALIVE_TIMEOUT gpSet->nFarHourglassDelay //1000

#define CONSOLE_BLOCK_SELECTION 0x0100 // selecting text (standard mode)
#define CONSOLE_TEXT_SELECTION 0x0200 // selecting text (stream mode)
#define CONSOLE_DBLCLICK_SELECTION 0x0400 // двойным кликом выделено слово, пропустить WM_LBUTTONUP
#define CONSOLE_KEYMOD_MASK 0xFF000000 // Здесь хранится модификатор, которым начали выделение мышкой

#define PROCESS_WAIT_START_TIME 1000

#define SETSYNCSIZEAPPLYTIMEOUT 500
#define SETSYNCSIZEMAPPINGTIMEOUT 300
#define CONSOLEPROGRESSTIMEOUT 1500
#define CONSOLEPROGRESSWARNTIMEOUT 2000 // поставил 2с, т.к. при минимизации консоль обновляется раз в секунду
#define CONSOLEINACTIVERGNTIMEOUT 500
#define SERVERCLOSETIMEOUT 2000

/*#pragma pack(push, 1)


//
//  Structure to send console via WM_SETCONSOLEINFO
//
typedef struct _CONSOLE_INFO
{
    ULONG       Length;
    COORD       ScreenBufferSize;
    COORD       WindowSize;
    ULONG       WindowPosX;
    ULONG       WindowPosY;

    COORD       FontSize;
    ULONG       FontFamily;
    ULONG       FontWeight;
    WCHAR       FaceName[32];

    ULONG       CursorSize;
    ULONG       FullScreen;
    ULONG       QuickEdit;
    ULONG       AutoPosition;
    ULONG       InsertMode;

    USHORT      ScreenColors;
    USHORT      PopupColors;
    ULONG       HistoryNoDup;
    ULONG       HistoryBufferSize;
    ULONG       NumberOfHistoryBuffers;

    COLORREF    ColorTable[16];

    ULONG       CodePage;
    HWND        Hwnd;

    WCHAR       ConsoleTitle[0x100];

} CONSOLE_INFO;

#pragma pack(pop)*/

struct ConProcess
{
	DWORD ProcessID, ParentPID; //, InputTID;
	bool  IsFar, IsFarPlugin;
	bool  IsTelnet; // может быть включен ВМЕСТЕ с IsFar, если удалось подцепится к фару через сетевой пайп
	bool  IsNtvdm;  // 16bit приложения
	bool  IsCmd;    // значит фар выполняет команду
	bool  NameChecked, RetryName;
	bool  Alive, inConsole;
	wchar_t Name[64]; // чтобы полная инфа об ошибке влезала
};

//#include <pshpack1.h>
//typedef struct tag_CharAttr
//{
//	TODO("OPTIMIZE: Заменить бы битовые поля на один DWORD, в котором хранить некий общий ИД стиля, заполняемый при формировании буфера");
//	union {
//		// Собственно цвета/шрифты
//		struct {
//			unsigned int crForeColor : 24; // чтобы в ui64 поместился и nFontIndex
//			unsigned int nFontIndex : 8; // 0 - normal, 1 - bold, 2 - italic
//			unsigned int crBackColor : 32; // Старший байт зарезервируем, вдруг для прозрачности понадобится
//			unsigned int nForeIdx : 8;
//			unsigned int nBackIdx : 8; // может понадобиться для ExtendColors
//			unsigned int crOrigForeColor : 32;
//			unsigned int crOrigBackColor : 32; // Реальные цвета в консоли, crForeColor и crBackColor могут быть изменены колорером
//			// вспомогательные флаги
//			unsigned int bDialog : 1;
//			unsigned int bDialogVBorder : 1;
//			unsigned int bDialogCorner : 1;
//			unsigned int bSomeFilled : 1;
//			unsigned int bTransparent : 1; // UserScreen
//		};
//		// А это для сравнения (поиск изменений)
//		unsigned __int64 All;
//		// для сравнения, когда фон не важен
//		unsigned int ForeFont;
//	};
//	//
//	//DWORD dwAttrubutes; // может когда понадобятся дополнительные флаги...
//	//
//    ///**
//    // * Used exclusively by ConsoleView to append annotations to each character
//    // */
//    //AnnotationInfo annotationInfo;
//} CharAttr;
//#include <poppack.h>
//
//inline bool operator==(const CharAttr& s1, const CharAttr& s2)
//{
//    return s1.All == s2.All;
//}
//

#define DBGMSG_LOG_ID (WM_APP+100)
#define DBGMSG_LOG_SHELL_MAGIC 0xD73A34
#define DBGMSG_LOG_INPUT_MAGIC 0xD73A35
#define DBGMSG_LOG_CMD_MAGIC   0xD73A36
struct DebugLogShellActivity
{
	DWORD   nParentPID, nParentBits, nChildPID;
	wchar_t szFunction[32];
	wchar_t* pszAction;
	wchar_t* pszFile;
	wchar_t* pszParam;
	int     nImageSubsystem;
	int     nImageBits;
	DWORD   nShellFlags, nCreateFlags, nStartFlags, nShowCmd;
	BOOL    bDos;
	DWORD   hStdIn, hStdOut, hStdErr;
};

#define MAX_SERVER_THREADS 3
//#define MAX_THREAD_PACKETS 100

class CVirtualConsole;
class CRgnDetect;
class CRealBuffer;

enum RealBufferType
{
	rbt_Undefined = 0,
	rbt_Primary,
	rbt_Alternative,
	rbt_Selection,
	rbt_Find,
	rbt_DumpScreen,
};

class CRealConsole
{
#ifdef _DEBUG
		friend class CVirtualConsole;
#endif
	public:

		uint TextWidth();
		uint TextHeight();
		uint BufferHeight(uint nNewBufferHeight=0);

	private:
		HWND    hConWnd;
		HWND    hGuiWnd; // Если работаем в Gui-режиме (Notepad, Putty, ...)
		BOOL    mb_GuiExternMode; // FALSE если захотели показать GUI приложение вне вкладки ConEmu (Ctrl-Win-Alt-Space)
		RECT    rcPreGuiWndRect; // Положение окна ДО аттача
		BOOL    mb_InGuiAttaching;
		BOOL    mb_InSetFocus;
		DWORD   mn_GuiWndStyle, mn_GuiWndStylEx; // Исходные стили окна ДО подцепления в ConEmu
		DWORD   mn_GuiWndPID;
		BYTE    m_ConsoleKeyShortcuts;
		//HANDLE  hFileMapping;
		//CESERVER_REQ_CONINFO* pConsoleData;
		//void CloseMapping();

	public:
		HWND    ConWnd();  // HWND RealConsole
		HWND    GetView(); // HWND отрисовки

		// Если работаем в Gui-режиме (Notepad, Putty, ...)
		HWND    GuiWnd();  // HWND Gui приложения
		BOOL    isGuiVisible();
		BOOL    isGuiOverCon();
		void    SetGuiMode(DWORD anFlags, HWND ahGuiWnd, DWORD anStyle, DWORD anStyleEx, LPCWSTR asAppFileName, DWORD anAppPID, RECT arcPrev);
		void    CorrectGuiChildRect(DWORD anStyle, DWORD anStyleEx, RECT& rcGui);

		CRealConsole(CVirtualConsole* apVCon);
		~CRealConsole();

		BYTE GetConsoleKeyShortcuts() { return this ? m_ConsoleKeyShortcuts : 0; };

		BOOL PreInit(BOOL abCreateBuffers=TRUE);
		void DumpConsole(HANDLE ahFile);
		bool LoadDumpConsole(LPCWSTR asDumpFile);
		
		RealBufferType GetActiveBufferType();
		bool SetActiveBuffer(RealBufferType aBufferType);

		BOOL SetConsoleSize(USHORT sizeX, USHORT sizeY, USHORT sizeBuffer=0, DWORD anCmdID=CECMD_SETSIZESYNC);
	private:
		bool SetActiveBuffer(CRealBuffer* aBuffer);
		BOOL SetConsoleSizeSrv(USHORT sizeX, USHORT sizeY, USHORT sizeBuffer, DWORD anCmdID=CECMD_SETSIZESYNC);
	private:
		//void SendConsoleEvent(INPUT_RECORD* piRec);
		DWORD mn_FlushIn, mn_FlushOut;
	public:
		COORD ScreenToBuffer(COORD crMouse);
		void PostConsoleEvent(INPUT_RECORD* piRec);
		void PostKeyPress(WORD vkKey, DWORD dwControlState, wchar_t wch, int ScanCode = -1);
		void PostKeyUp(WORD vkKey, DWORD dwControlState, wchar_t wch, int ScanCode = -1);
		void PostLeftClickSync(COORD crDC);
		void PostConsoleEventPipe(MSG64 *pMsg);
	private:
		void PostMouseEvent(UINT messg, WPARAM wParam, COORD crMouse, bool abForceSend = false);
	public:
		BOOL OpenConsoleEventPipe();
		LRESULT PostConsoleMessage(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam);
		BOOL ShowOtherWindow(HWND hWnd, int swShow, BOOL abAsync=TRUE);
		BOOL SetOtherWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);
		BOOL SetOtherWindowFocus(HWND hWnd, BOOL abSetForeground);
		HWND SetOtherWindowParent(HWND hWnd, HWND hParent);
		BOOL SetOtherWindowRgn(HWND hWnd, int nRects, LPRECT prcRects, BOOL bRedraw);
		void PostMacro(LPCWSTR asMacro, BOOL abAsync = FALSE);
	private:
		struct PostMacroAnyncArg
		{
			CRealConsole* pRCon;
			BOOL    bPipeCommand;
			DWORD   nCmdSize;
			DWORD   nCmdID;
			union
			{
				wchar_t szMacro[1];
				BYTE    Data[1];
			};
		};
		static DWORD WINAPI PostMacroThread(LPVOID lpParameter);
		HANDLE mh_PostMacroThread; DWORD mn_PostMacroThreadID;
		void PostCommand(DWORD anCmdID, DWORD anCmdSize, LPCVOID ptrData);
	public:
		//BOOL FlushInputQueue(DWORD nTimeout = 500);
		void OnKeyboard(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, const wchar_t *pszChars);
		void OnKeyboardIme(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
		void OnMouse(UINT messg, WPARAM wParam, int x, int y, bool abForceSend = false);
		void OnFocus(BOOL abFocused);

		void StopSignal();
		void StopThread(BOOL abRecreating=FALSE);
		BOOL isBufferHeight();
		HWND isPictureView(BOOL abIgnoreNonModal=FALSE);
		BOOL isWindowVisible();
		LPCTSTR GetTitle();
		void GetConsoleScreenBufferInfo(CONSOLE_SCREEN_BUFFER_INFO* sbi);
		//void GetConsoleCursorPos(COORD *pcr);
		void GetConsoleCursorInfo(CONSOLE_CURSOR_INFO *ci);
		DWORD GetConsoleCP();
		DWORD GetConsoleOutputCP();
		DWORD GetConsoleMode();
		void SyncConsole2Window(BOOL abNtvdmOff=FALSE, LPRECT prcNewWnd=NULL);
		void SyncGui2Window(RECT* prcClient=NULL);
		//void OnWinEvent(DWORD anEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
		void OnServerStarted(HWND ahConWnd, DWORD anServerPID);
		void OnDosAppStartStop(enum StartStopType sst, DWORD anPID);
		int  GetProcesses(ConProcess** ppPrc);
		DWORD GetFarPID(BOOL abPluginRequired=FALSE);
		DWORD GetActivePID();
		DWORD GetProgramStatus();
		DWORD GetFarStatus();
		DWORD GetServerPID();
		bool isServerCreated();
		LRESULT OnScroll(int nDirection);
		LRESULT OnSetScrollPos(WPARAM wParam);
		bool GetConsoleSelectionInfo(CONSOLE_SELECTION_INFO *sel);
		BOOL isConSelectMode();
		bool isFarBufferSupported();
		bool isSelectionAllowed();
		bool isSelectionPresent();
		void StartSelection(BOOL abTextMode, SHORT anX=-1, SHORT anY=-1, BOOL abByMouse=FALSE);
		void ExpandSelection(SHORT anX=-1, SHORT anY=-1);
		bool DoSelectionCopy();
		void DoSelectionStop();
		BOOL isFar(BOOL abPluginRequired=FALSE);
		void ShowConsoleOrGuiClient(int nMode); // -1 Toggle, 0 - Hide, 1 - Show
		void ShowConsole(int nMode); // -1 Toggle, 0 - Hide, 1 - Show
		void ShowGuiClient(int nMode); // -1 Toggle, 0 - Hide, 1 - Show
		BOOL isDetached();
		BOOL AttachConemuC(HWND ahConWnd, DWORD anConemuC_PID, const CESERVER_REQ_STARTSTOP* rStartStop, CESERVER_REQ_STARTSTOPRET* pRet);
		BOOL RecreateProcess(RConStartArgs *args);
		void GetConsoleData(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight);
	private:
		BOOL GetConsoleLine(int nLine, wchar_t** pChar, /*CharAttr** pAttr,*/ int* pLen, MSectionLock* pcsData);
		//enum ExpandTextRangeType
		//{
		//	etr_None = 0,
		//	etr_Word = 1,
		//	etr_FileAndLine = 2,
		//};
		//ExpandTextRangeType ExpandTextRange(COORD& crFrom/*[In/Out]*/, COORD& crTo/*[Out]*/, ExpandTextRangeType etr, wchar_t* pszText = NULL, size_t cchTextMax = 0);
		bool IsFarHyperlinkAllowed();
		bool ProcessFarHyperlink(UINT messg, COORD crFrom);
		void UpdateTabFlags(/*IN|OUT*/ ConEmuTab* pTab);
	public:
		
		BOOL IsConsoleDataChanged();
		void OnActivate(int nNewNum, int nOldNum);
		void OnDeactivate(int nNewNum);
		void ShowHideViews(BOOL abShow);
		void OnGuiFocused(BOOL abFocus, BOOL abForceChild = FALSE);
		void UpdateServerActive(BOOL abActive);
		BOOL CheckBufferSize();
		//LRESULT OnConEmuCmd(BOOL abStarted, DWORD anConEmuC_PID);
		BOOL BufferHeightTurnedOn(CONSOLE_SCREEN_BUFFER_INFO* psbi);
		void UpdateScrollInfo();
		void SetTabs(ConEmuTab* tabs, int tabsCount);
		int GetTabCount(BOOL abVisibleOnly = FALSE);
		int GetActiveTab();
		BOOL GetTab(int tabIdx, /*OUT*/ ConEmuTab* pTab);
		int GetModifiedEditors();
		BOOL ActivateFarWindow(int anWndIndex);
		DWORD CanActivateFarWindow(int anWndIndex);
		void SwitchKeyboardLayout(WPARAM wParam,DWORD_PTR dwNewKeybLayout);
		void CloseConsole(BOOL abForceTerminate = FALSE);
		BOOL CanCloseTab(BOOL abPluginRequired = FALSE);
		void CloseTab();
		bool isConsoleClosing();
		void OnServerClosing(DWORD anSrvPID);
		void Paste();
		void LogString(LPCSTR asText, BOOL abShowTime = FALSE);
		void LogString(LPCWSTR asText, BOOL abShowTime = FALSE);
		bool isActive();
		bool isFilePanel(bool abPluginAllowed = false, bool abSkipEditViewCheck = false);
		bool isEditor();
		bool isEditorModified();
		bool isViewer();
		bool isVisible();
		bool isNtvdm();
		//bool isPackets();
		LPCWSTR GetCmd();
		LPCWSTR GetDir();
		BOOL GetUserPwd(const wchar_t** ppszUser, const wchar_t** ppszDomain, BOOL* pbRestricted);
		short GetProgress(BOOL* rpbError);
		void UpdateGuiInfoMapping(const ConEmuGuiMapping* apGuiInfo);
		void UpdateFarSettings(DWORD anFarPID=0);
		int CoordInPanel(COORD cr);
		BOOL GetPanelRect(BOOL abRight, RECT* prc, BOOL abFull = FALSE);
		bool isAdministrator();
		BOOL isMouseButtonDown();
		void OnConsoleLangChange(DWORD_PTR dwNewKeybLayout);
		DWORD GetConsoleStates();
		void ChangeBufferHeightMode(BOOL abBufferHeight); // Вызывается из TabBar->ConEmu
		//void RemoveFromCursor(); // -- заменено на перехват функции ScreenToClient
		bool isAlive();
		bool GetMaxConSize(COORD* pcrMaxConSize);
		int GetDetectedDialogs(int anMaxCount, SMALL_RECT* rc, DWORD* rf);
		const CRgnDetect* GetDetector();
		// Логирование Shell вызовов
		//void LogShellStartStop();
		bool IsLogShellStarted();
		wchar_t ms_LogShellActivity[MAX_PATH]; bool mb_ShellActivityLogged;
		int GetStatusLineCount(int nLeftPanelEdge);

	public:
		// Вызываются из CVirtualConsole
		BOOL PreCreate(RConStartArgs *args);
		//(BOOL abDetached, LPCWSTR asNewCmd = NULL, BOOL abAsAdmin = FALSE);
		BOOL IsConsoleThread();
		void SetForceRead();
		//DWORD WaitEndUpdate(DWORD dwTimeout=1000);
		LPCWSTR GetConStatus();
		static wchar_t ms_LastRConStatus[80];
		void UpdateCursorInfo();
		void Detach();
		void AdminDuplicate();
		const CEFAR_INFO_MAPPING *GetFarInfo(); // FarVer и прочее
		BOOL InCreateRoot();
		BOOL GuiAppAttachAllowed(LPCWSTR asAppFileName, DWORD anAppPID);
		//LPCWSTR GetLngNameTime();
		void ShowPropertiesDialog();

	protected:
		CVirtualConsole* mp_VCon; // соответствующая виртуальная консоль
		DWORD mn_ConEmuC_PID/*, mn_ConEmuC_Input_TID*/; HANDLE mh_ConEmuC, mh_ConEmuCInput;
		/*HANDLE mh_CreateRootEvent;*/ BOOL mb_InCreateRoot;
		BOOL mb_UseOnlyPipeInput;
		TCHAR ms_ConEmuC_Pipe[MAX_PATH], ms_ConEmuCInput_Pipe[MAX_PATH], ms_VConServer_Pipe[MAX_PATH];
		//TCHAR ms_ConEmuC_DataReady[64]; HANDLE mh_ConEmuC_DataReady;
		void InitNames();
		// Текущий заголовок консоли и его значение для сравнения (для определения изменений)
		WCHAR Title[MAX_TITLE_SIZE+1], TitleCmp[MAX_TITLE_SIZE+1];
		// А здесь содержится то, что отображается в ConEmu (может быть добавлено " (Admin)")
		WCHAR TitleFull[MAX_TITLE_SIZE+96], TitleAdmin[MAX_TITLE_SIZE+192];
		// Принудительно дернуть OnTitleChanged, например, при изменении процентов в консоли
		BOOL mb_ForceTitleChanged;
		// Здесь сохраняется заголовок окна (с панелями), когда FAR фокус с панелей уходит (переходит в редактор...).
		WCHAR ms_PanelTitle[CONEMUTABMAX];
		// Процентики
		short mn_Progress, mn_LastShownProgress;
		short mn_PreWarningProgress; DWORD mn_LastWarnCheckTick;
		short mn_ConsoleProgress, mn_LastConsoleProgress; DWORD mn_LastConProgrTick;
		short CheckProgressInTitle();
		//short CheckProgressInConsole(const wchar_t* pszCurLine);
		//void SetProgress(short anProgress); // установить переменную mn_Progress и mn_LastProgressTick

		BOOL AttachPID(DWORD dwPID);
		BOOL StartProcess();
		BOOL StartMonitorThread();
		BOOL mb_NeedStartProcess;

		// Нить наблюдения за консолью
		static DWORD WINAPI MonitorThread(LPVOID lpParameter);
		HANDLE mh_MonitorThread; DWORD mn_MonitorThreadID;
		// Для пересылки событий ввода в консоль
		//static DWORD WINAPI InputThread(LPVOID lpParameter);
		//HANDLE mh_InputThread; DWORD mn_InputThreadID;

		HANDLE mh_TermEvent, mh_MonitorThreadEvent, mh_ApplyFinished;
		BOOL mb_FullRetrieveNeeded; //, mb_Detached;
		RConStartArgs m_Args;
		//wchar_t* ms_SpecialCmd;
		//BOOL mb_RunAsAdministrator;


		static void Box(LPCTSTR szText);

		//BOOL RetrieveConsoleInfo(/*BOOL bShortOnly,*/ UINT anWaitSize);
		BOOL WaitConsoleSize(int anWaitSize, DWORD nTimeout);
		BOOL InitBuffers(DWORD OneBufferSize);
		BOOL LoadDataFromSrv(DWORD CharCount, CHAR_INFO* pData);
	private:
		friend class CRealBuffer;
		CRealBuffer* mp_RBuf; // Реальный буфер консоли
		CRealBuffer* mp_EBuf; // Сохранение данных после выполненной команды в Far
		CRealBuffer* mp_SBuf; // Временный буфер (полный) для блокирования содержимого (выделение/прокрутка/поиск)
		CRealBuffer* mp_ABuf; // Активный буфер консоли -- ссылка на один из mp_RBuf/mp_EBuf/mp_SBuf
		bool mb_ABufChaged; // Сменился активный буфер, обновить консоль
		
		int mn_DefaultBufferHeight;
		DWORD mn_LastInactiveRgnCheck;
		#ifdef _DEBUG
		BOOL mb_DebugLocked; // для отладки - заморозить все нити, чтобы не мешали отладчику, ставится по контектному меню
		#endif
		
		//// Эти переменные инициализируются в RetrieveConsoleInfo()
		//MSection csCON; //DWORD ncsT;
		//struct RConInfo
		//{
		//	CONSOLE_SELECTION_INFO m_sel;
		//	CONSOLE_CURSOR_INFO m_ci;
		//	DWORD m_dwConsoleCP, m_dwConsoleOutputCP, m_dwConsoleMode;
		//	CONSOLE_SCREEN_BUFFER_INFO m_sbi;
		//	COORD crMaxSize; // Максимальный размер консоли на текущем шрифте
		//	USHORT nTopVisibleLine; // может отличаться от m_sbi.srWindow.Top, если прокрутка заблокирована
		//	wchar_t *pConChar;
		//	WORD  *pConAttr;
		//	COORD mcr_FileLineStart, mcr_FileLineEnd; // Подсветка строк ошибок компиляторов
		//	//CESERVER_REQ_CONINFO_DATA *pCopy, *pCmp;
		//	CHAR_INFO *pDataCmp;
		//	int nTextWidth, nTextHeight, nBufferHeight;
		//	BOOL bLockChange2Text;
		//	int nChange2TextWidth, nChange2TextHeight;
		//	BOOL bBufferHeight; // TRUE, если есть прокрутка
		//	//DWORD nPacketIdx;
		//	DWORD_PTR dwKeybLayout;
		//	BOOL bRBtnDrag; // в консоль посылается драг правой кнопкой (выделение в FAR)
		//	COORD crRBtnDrag;
		//	BOOL bInSetSize; HANDLE hInSetSize;
		//	int DefaultBufferHeight;
		//	BOOL bConsoleDataChanged;
		//	DWORD nLastInactiveRgnCheck;
		//	#ifdef _DEBUG
		//	BOOL bDebugLocked; // для отладки - заморозить все нити, чтобы не мешали отладчику, ставится по контектному меню
		//	#endif
		//} con;
		
		BOOL mb_ThawRefreshThread;
		struct ServerClosing
		{
			DWORD  nServerPID;     // PID закрывающегося сервера
			DWORD  nRecieveTick;   // Tick, когда получено сообщение о закрытии
			HANDLE hServerProcess; // Handle процесса сервера
		} m_ServerClosing;
		//
		MSection csPRC; //DWORD ncsTPRC;
		std::vector<ConProcess> m_Processes;
		int mn_ProcessCount;
		DWORD m_FarPlugPIDs[128];
		UINT mn_FarPlugPIDsCount;
		BOOL mb_SkipFarPidChange;
		DWORD m_TerminatedPIDs[128]; UINT mn_TerminatedIdx;
		//
		DWORD mn_FarPID;
		DWORD mn_ActivePID;
		DWORD mn_LastSetForegroundPID; // PID процесса, которому в последний раз было разрешено AllowSetForegroundWindow
		//
		ConEmuTab* mp_tabs;
		MSection   msc_Tabs;
		int mn_tabsCount, mn_MaxTabs, mn_ActiveTab;
		BOOL mb_TabsWasChanged;
		void CheckPanelTitle();
		//
		//void ProcessAdd(DWORD addPID);
		//void ProcessDelete(DWORD addPID);
		BOOL ProcessUpdate(const DWORD *apPID, UINT anCount);
		BOOL ProcessUpdateFlags(BOOL abProcessChanged);
		void ProcessCheckName(struct ConProcess &ConPrc, LPWSTR asFullFileName);
		DWORD mn_ProgramStatus, mn_FarStatus;
		DWORD mn_Comspec4Ntvdm;
		BOOL mb_IgnoreCmdStop; // При запуске 16bit приложения не возвращать размер консоли! Это сделает OnWinEvent
		BOOL isShowConsole;
		//BOOL mb_FarGrabberActive; // бывший mb_ConsoleSelectMode
		WORD mn_SelectModeSkipVk; // пропустить "отпускание" клавиши Esc/Enter при выделении текста
		//bool OnMouseSelection(UINT messg, WPARAM wParam, int x, int y);
		//void UpdateSelection(); // обновить на экране
		static DWORD WINAPI RConServerThread(LPVOID lpvParam);
		HANDLE mh_RConServerThreads[MAX_SERVER_THREADS], mh_ActiveRConServerThread;
		DWORD  mn_RConServerThreadsId[MAX_SERVER_THREADS];
		HANDLE mh_ServerSemaphore, mh_GuiAttached;
		//void SetBufferHeightMode(BOOL abBufferHeight, BOOL abIgnoreLock=FALSE);
		//BOOL mb_BuferModeChangeLocked; -> mp_RBuf

		void ServerThreadCommand(HANDLE hPipe);
		CESERVER_REQ* cmdStartStop(HANDLE hPipe, CESERVER_REQ* pIn, UINT nDataSize);
		//CESERVER_REQ* cmdGetGuiHwnd(HANDLE hPipe, CESERVER_REQ* pIn, UINT nDataSize);
		CESERVER_REQ* cmdTabsChanged(HANDLE hPipe, CESERVER_REQ* pIn, UINT nDataSize);
		CESERVER_REQ* cmdGetOutputFile(HANDLE hPipe, CESERVER_REQ* pIn, UINT nDataSize);
		CESERVER_REQ* cmdGuiMacro(HANDLE hPipe, CESERVER_REQ* pIn, UINT nDataSize);
		CESERVER_REQ* cmdLangChange(HANDLE hPipe, CESERVER_REQ* pIn, UINT nDataSize);
		CESERVER_REQ* cmdTabsCmd(HANDLE hPipe, CESERVER_REQ* pIn, UINT nDataSize);
		CESERVER_REQ* cmdResources(HANDLE hPipe, CESERVER_REQ* pIn, UINT nDataSize);
		CESERVER_REQ* cmdSetForeground(HANDLE hPipe, CESERVER_REQ* pIn, UINT nDataSize);
		CESERVER_REQ* cmdFlashWindow(HANDLE hPipe, CESERVER_REQ* pIn, UINT nDataSize);
		CESERVER_REQ* cmdRegPanelView(HANDLE hPipe, CESERVER_REQ* pIn, UINT nDataSize);
		CESERVER_REQ* cmdSetBackground(HANDLE hPipe, CESERVER_REQ* pIn, UINT nDataSize);
		CESERVER_REQ* cmdActivateCon(HANDLE hPipe, CESERVER_REQ* pIn, UINT nDataSize);
		CESERVER_REQ* cmdOnCreateProc(HANDLE hPipe, CESERVER_REQ* pIn, UINT nDataSize);
		//CESERVER_REQ* cmdNewConsole(HANDLE hPipe, CESERVER_REQ* pIn, UINT nDataSize);
		CESERVER_REQ* cmdOnPeekReadInput(HANDLE hPipe, CESERVER_REQ* pIn, UINT nDataSize);
		CESERVER_REQ* cmdOnSetConsoleKeyShortcuts(HANDLE hPipe, CESERVER_REQ* pIn, UINT nDataSize);
		//CESERVER_REQ* cmdAssert(HANDLE hPipe, CESERVER_REQ* pIn, UINT nDataSize);
		
		//void ApplyConsoleInfo(CESERVER_REQ* pInfo);
		void SetHwnd(HWND ahConWnd, BOOL abForceApprove = FALSE);
		WORD mn_LastVKeyPressed;
		BOOL GetConWindowSize(const CONSOLE_SCREEN_BUFFER_INFO& sbi, int& nNewWidth, int& nNewHeight, BOOL* pbBufferHeight=NULL);
		int mn_Focused; //-1 после запуска, 1 - в фокусе, 0 - не в фокусе
		DWORD mn_InRecreate; // Tick, когда начали пересоздание
		BOOL mb_ProcessRestarted;
		BOOL mb_InCloseConsole;
		// Логи
		BYTE m_UseLogs;
		HANDLE mh_LogInput; wchar_t *mpsz_LogInputFile/*, *mpsz_LogPackets*/; //UINT mn_LogPackets;
		void CreateLogFiles();
		void CloseLogFiles();
		void LogInput(INPUT_RECORD* pRec);
		//void LogPacket(CESERVER_REQ* pInfo);
		BOOL RecreateProcessStart();
		// Прием и обработка пакетов
		//MSection csPKT; //DWORD ncsTPKT;
		//DWORD mn_LastProcessedPkt; //HANDLE mh_PacketArrived;
		//std::vector<CESERVER_REQ*> m_Packets;
		//CESERVER_REQ* m_PacketQueue[(MAX_SERVER_THREADS+1)*MAX_THREAD_PACKETS];
		//void PushPacket(CESERVER_REQ* pPkt);
		//CESERVER_REQ* PopPacket();
		//HANDLE mh_FileMapping, mh_FileMappingData,
		//HANDLE mh_FarFileMapping,
		//HANDLE mh_FarAliveEvent;
		MEvent m_FarAliveEvent;
		MPipe<CESERVER_REQ_HDR,CESERVER_REQ_HDR> m_GetDataPipe;
		MEvent m_ConDataChanged;
		//wchar_t ms_HeaderMapName[64], ms_DataMapName[64];
		//const CESERVER_CONSOLE_MAPPING_HDR *mp_ConsoleInfo;
		//const CESERVER_REQ_CONINFO_DATA *mp_ConsoleData; // Mapping
		MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> m_ConsoleMap;
		//const CEFAR_INFO_MAPPING *mp_FarInfo;
		CEFAR_INFO_MAPPING m_FarInfo; // FarVer и прочее
		MFileMapping<const CEFAR_INFO_MAPPING> m__FarInfo; // в коде напрямую не использовать, только через секцию!
		MSection ms_FarInfoCS;
		// Colorer Mapping
		//HANDLE mh_ColorMapping;
		//AnnotationHeader *mp_ColorHdr;
		MFileMapping<AnnotationHeader> m_TrueColorerMap;
		AnnotationHeader m_TrueColorerHeader;
		const AnnotationInfo *mp_TrueColorerData;
		DWORD mn_LastColorFarID;
		void CreateColorMapping(); // Открыть мэппинг колорера (по HWND)
		//void CheckColorMapping(DWORD dwPID); // Проверить валидность буфера - todo
		void CloseColorMapping();
		//
		DWORD mn_LastConsoleDataIdx, mn_LastConsolePacketIdx; //, mn_LastFarReadIdx;
		DWORD mn_LastFarReadTick;
		BOOL OpenFarMapData();
		void CloseFarMapData(MSectionLock* pCS = NULL);
		BOOL OpenMapHeader(BOOL abFromAttach=FALSE);
		//void CloseMapData();
		//BOOL ReopenMapData();
		void CloseMapHeader();
		BOOL ApplyConsoleInfo();
		BOOL mb_DataChanged;
		void OnServerStarted();
		//
		BOOL PrepareOutputFile(BOOL abUnicodeText, wchar_t* pszFilePathName);
		HANDLE PrepareOutputFileCreate(wchar_t* pszFilePathName);
		// фикс для dblclick в редакторе
		MOUSE_EVENT_RECORD m_LastMouse;
		POINT m_LastMouseGuiPos; // в пикселах пикселах
		BOOL mb_BtnClicked; COORD mrc_BtnClickPos;
		//
		wchar_t ms_Editor[32], ms_EditorRus[32], ms_Viewer[32], ms_ViewerRus[32];
		wchar_t ms_TempPanel[32], ms_TempPanelRus[32];
		//wchar_t ms_NameTitle[32];
		//
		//BOOL mb_PluginDetected;
		DWORD mn_FarPID_PluginDetected; //, mn_Far_PluginInputThreadId;
		void CheckFarStates();
		void OnTitleChanged();
		DWORD mn_LastInvalidateTick;
		//
		HWND hPictureView; BOOL mb_PicViewWasHidden;
		//
		BOOL mb_MouseButtonDown;
		COORD mcr_LastMouseEventPos;
		//
		SHELLEXECUTEINFO *mp_sei;
		//
		HWND FindPicViewFrom(HWND hFrom);
		//
		wchar_t ms_ConStatus[80];
		void SetConStatus(LPCWSTR asStatus);
		bool isCharBorderVertical(WCHAR inChar);
		bool isCharBorderLeftVertical(WCHAR inChar);
		bool isCharBorderHorizontal(WCHAR inChar);
		bool ConsoleRect2ScreenRect(const RECT &rcCon, RECT *prcScr);
		/* ****************************************** */
		/* Поиск диалогов и пометка "прозрачных" мест */
		/* ****************************************** */
		//CRgnDetect* mp_Rgn; DWORD mn_LastRgnFlags;
		//int mn_DetectCallCount;
		//void PrepareTransparent(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight);
		//void DetectDialog(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int nFromX, int nFromY, int *pnMostRight=NULL, int *pnMostBottom=NULL);
		//bool FindDialog_TopLeft(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, BOOL &bMarkBorder);
		//bool FindDialog_TopRight(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, BOOL &bMarkBorder);
		//bool FindDialog_Left(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, BOOL &bMarkBorder);
		//bool FindDialog_Right(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, BOOL &bMarkBorder);
		//bool FindDialog_Any(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, BOOL &bMarkBorder);
		//bool FindDialog_Inner(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY);
		//bool FindFrame_TopLeft(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nFrameX, int &nFrameY);
		//bool FindFrameTop_ByRight(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostTop);
		//bool FindFrameTop_ByLeft(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostTop);
		//bool FindFrameBottom_ByRight(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostBottom);
		//bool FindFrameBottom_ByLeft(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostBottom);
		//bool FindFrameRight_ByTop(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight);
		//bool FindFrameRight_ByBottom(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight);
		//bool FindFrameLeft_ByTop(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostLeft);
		//bool FindFrameLeft_ByBottom(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostLeft);
		//// Последний шанс
		//bool FindByBackground(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, BOOL &bMarkBorder);
		//// Сервисная
		//bool ExpandDialogFrame(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int nFrameX, int nFrameY, int &nMostRight, int &nMostBottom);
		//void MarkDialog(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int nX1, int nY1, int nX2, int nY2, BOOL bMarkBorder = FALSE, BOOL bFindExterior = TRUE);
		//#define MAX_DETECTED_DIALOGS 20
		//	struct {
		//		int Count;
		//		SMALL_RECT Rects[MAX_DETECTED_DIALOGS];
		//		bool bWasFrame[MAX_DETECTED_DIALOGS];
		//	} m_DetectedDialogs;
};
