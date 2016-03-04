
/*
Copyright (c) 2009-2016 Maximus5
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
#include "../common/ConsoleAnnotation.h"
#include "../common/RgnDetect.h"
#include "../common/MArray.h"
#include "../common/MEvent.h"
#include "../common/MMap.h"
#include "../common/MPipe.h"
#include "../common/MSection.h"
#include "../common/MFileMapping.h"

#define DEFINE_EXIT_DESC
#include "../ConEmuCD/ExitCodes.h"

#define SB_HALFPAGEUP   32
#define SB_HALFPAGEDOWN 33
#define SB_GOTOCURSOR   34

#define CES_CMDACTIVE      0x01
#define CES_TELNETACTIVE   0x02
#define CES_FARACTIVE      0x04
#define CES_FARINSTACK     0x08
//#define CES_CONALTERNATIVE 0x08
//#define CES_PROGRAMS (0x0F)

//#define CES_NTVDM 0x10 -- Common.h
//#define CES_PROGRAMS2 0xFF

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
#define CONSOLE_LEFT_ANCHOR 0x0800 // If selection was started rightward
#define CONSOLE_RIGHT_ANCHOR 0x1000 // If selection was started leftward
#define CONSOLE_KEYMOD_MASK 0xFF000000 // Здесь хранится модификатор, которым начали выделение мышкой

#define PROCESS_WAIT_START_TIME RELEASEDEBUGTEST(1000,1000)

#define SETSYNCSIZEAPPLYTIMEOUT 500
#define SETSYNCSIZEMAPPINGTIMEOUT 300
#define CONSOLEPROGRESSTIMEOUT 1500
#define CONSOLEPROGRESSWARNTIMEOUT 2000 // поставил 2с, т.к. при минимизации консоль обновляется раз в секунду
#define CONSOLEINACTIVERGNTIMEOUT 500
#define SERVERCLOSETIMEOUT 2000
#define UPDATESERVERACTIVETIMEOUT 2500

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

//#include "../common/ProcList.h"

struct ConProcess
{
	DWORD ProcessID, ParentPID; //, InputTID;
	bool  IsMainSrv; // Root ConEmuC
	bool  IsTermSrv; // cygwin/msys connector
	bool  IsConHost; // conhost.exe (Win7 и выше)
	bool  IsFar, IsFarPlugin;
	bool  IsTelnet;  // может быть включен ВМЕСТЕ с IsFar, если удалось подцепиться к фару через сетевой пайп
	bool  IsNtvdm;   // 16bit приложения
	bool  IsCmd;     // значит фар выполняет команду
	bool  NameChecked, RetryName;
	bool  Alive, inConsole;
	int   Bits;
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

//#define MAX_SERVER_THREADS 3
//#define MAX_THREAD_PACKETS 100

class CVirtualConsole;
class CRgnDetect;
class CRealBuffer;
class CDpiForDialog;
class CDynDialog;
class MFileLog;
class CRConFiles;
class CAltNumpad;
struct AppSettings;
struct TermX;

enum RealBufferType
{
	rbt_Undefined = 0,
	rbt_Primary,
	rbt_Alternative,
	rbt_Selection,
	rbt_Find,
	rbt_DumpScreen,
};

enum LoadAltMode
{
	lam_Default = 0,    // режим выбирается автоматически
	lam_LastOutput = 1, // имеет смысл только для фара и "Long console output"
	lam_FullBuffer = 2, // снимок экрана - полный буфер с прокруткой
	lam_Primary = 3,    // TODO: для быстрого начала выделения - копия буфера rbt_Primary
};

enum StartDebugType
{
	sdt_DumpMemory,
	sdt_DumpMemoryTree,
	sdt_DebugActiveProcess,
	sdt_DebugProcessTree,
};

struct ConEmuHotKey;
struct ConsoleInfoArg;

#include "HotkeyChord.h"
#include "RealServer.h"
#include "TabID.h"

class CRealConsole
{
#ifdef _DEBUG
		friend class CVirtualConsole;
#endif
	protected:
		CVirtualConsole* mp_VCon; // соответствующая виртуальная консоль
		CConEmuMain*     mp_ConEmu;
		bool mb_ConstuctorFinished;

	public:

		uint TextWidth();
		uint TextHeight();
		uint BufferHeight(uint nNewBufferHeight=0);
		uint BufferWidth();
		void OnBufferHeight();

	private:
		HWND    hConWnd;
		BYTE    m_ConsoleKeyShortcuts;
		BYTE    mn_TextColorIdx, mn_BackColorIdx, mn_PopTextColorIdx, mn_PopBackColorIdx;
		HKEY    PrepareConsoleRegistryKey(LPCWSTR asSubKey);
		void    PrepareDefaultColors(BYTE& nTextColorIdx, BYTE& nBackColorIdx, BYTE& nPopTextColorIdx, BYTE& nPopBackColorIdx, bool bUpdateRegistry = false, HKEY hkConsole = NULL);
	public:
		void    PrepareDefaultColors();
	private:
		// ChildGui related
		struct {
			HWND    hGuiWnd;            // ChildGui mode (Notepad, Putty, ...)
			RECT    rcLastGuiWnd;       // Screen coordinates!
			HWND    hGuiWndFocusStore;
			DWORD   nGuiAttachInputTID;
			DWORD   nGuiAttachFlags;    // Stored in SetGuiMode
			RECT    rcPreGuiWndRect;    // Window coordinates before attach into ConEmu
			bool    bGuiExternMode;     // FALSE user asked to show ChildGui outside of ConEmu, temporarily detached (Ctrl-Win-Alt-Space).
			bool    bGuiForceConView;   // TRUE if user asked to hide ChildGui and show our VirtualConsole (with current console contents).
			bool    bChildConAttached;  // TRUE if ChildGui started CONSOLE application (CommandPromptPortable.exe). Don't confuse with Putty/plink-proxy.
			bool    bInGuiAttaching;
			bool    bInSetFocus;
			DWORD   nGuiWndStyle, nGuiWndStylEx; // Исходные стили окна ДО подцепления в ConEmu
			ConProcess Process;
			CESERVER_REQ_PORTABLESTARTED paf;
			// some helpers
			bool    isGuiWnd() { return (hGuiWnd && (hGuiWnd != (HWND)INVALID_HANDLE_VALUE)); };
		} m_ChildGui;
		void setGuiWndPID(HWND ahGuiWnd, DWORD anPID, int anBits, LPCWSTR asProcessName);
		void setGuiWnd(HWND ahGuiWnd);
		static  BOOL CALLBACK FindChildGuiWindowProc(HWND hwnd, LPARAM lParam);

	public:
		enum ConStatusOption
		{
			cso_Default             = 0x0000,
			cso_ResetOnConsoleReady = 0x0001,
			cso_DontUpdate          = 0x0002, // Не нужно обновлять статусную строку сразу
			cso_Critical            = 0x0004,
		};
		LPCWSTR GetConStatus();
		void SetConStatus(LPCWSTR asStatus, DWORD/*enum ConStatusOption*/ Options = cso_Default);
	private:
		struct {
			DWORD   Options; /*enum ConStatusOption*/
			wchar_t szText[80];
		} m_ConStatus;

	public:
		HWND    ConWnd();  // HWND RealConsole
		HWND    GetView(); // HWND отрисовки

		// Если работаем в Gui-режиме (Notepad, Putty, ...)
		HWND    GuiWnd();  // HWND Gui приложения
		DWORD   GuiWndPID();  // HWND Gui приложения
		bool    isGuiForceConView(); // mb_GuiForceConView
		bool    isGuiExternMode(); // bGuiExternMode
		bool    isGuiEagerFocus(); // ставить фокус в ChildGui при попадании оного в ConEmu
		void    GuiNotifyChildWindow();
		void    GuiWndFocusStore();
		void    GuiWndFocusRestore(bool bForce = false);
	private:
		void    GuiWndFocusThread(HWND hSetFocus, BOOL& bAttached, BOOL& bAttachCalled, DWORD& nErr);
	public:
		BOOL    isGuiVisible();
		BOOL    isGuiOverCon();
		void    StoreGuiChildRect(LPRECT prcNewPos);
		void    SetGuiMode(DWORD anFlags, HWND ahGuiWnd, DWORD anStyle, DWORD anStyleEx, LPCWSTR asAppFileName, DWORD anAppPID, int anBits, RECT arcPrev);
		static void CorrectGuiChildRect(DWORD anStyle, DWORD anStyleEx, RECT& rcGui, LPCWSTR pszExeName);
		static bool CanCutChildFrame(LPCWSTR pszExeName);

		CRealConsole(CVirtualConsole* pVCon, CConEmuMain* pOwner);
		bool Construct(CVirtualConsole* apVCon, RConStartArgs *args);
		~CRealConsole();

		CVirtualConsole* VCon();

		BYTE GetConsoleKeyShortcuts() { return this ? m_ConsoleKeyShortcuts : 0; };
		BYTE GetDefaultTextColorIdx() { return this ? (mn_TextColorIdx & 0xF) : 7; };
		BYTE GetDefaultBackColorIdx() { return this ? (mn_BackColorIdx & 0xF) : 0; };

		bool PreInit();
		void DumpConsole(HANDLE ahFile);
		bool LoadDumpConsole(LPCWSTR asDumpFile);

		RealBufferType GetActiveBufferType();
		bool SetActiveBuffer(RealBufferType aBufferType);

		void DoLockUnlock(bool bLock);

		BOOL SetConsoleSize(SHORT sizeX, SHORT sizeY, USHORT sizeBuffer=0, DWORD anCmdID=CECMD_SETSIZESYNC);
	private:
		bool SetActiveBuffer(CRealBuffer* aBuffer, bool abTouchMonitorEvent = true);
		bool LoadAlternativeConsole(LoadAltMode iMode = lam_Default);
	private:
		//void SendConsoleEvent(INPUT_RECORD* piRec);
		DWORD mn_FlushIn, mn_FlushOut;
	public:
		COORD ScreenToBuffer(COORD crMouse);
		COORD BufferToScreen(COORD crMouse, bool bFixup = true, bool bVertOnly = false);
		bool PostCtrlBreakEvent(DWORD nEvent, DWORD nGroupId);
		bool PostConsoleEvent(INPUT_RECORD* piRec, bool bFromIME = false);
		bool PostKeyPress(WORD vkKey, DWORD dwControlState, wchar_t wch, int ScanCode = -1);
		bool DeleteWordKeyPress(bool bTestOnly = false);
		bool PostKeyUp(WORD vkKey, DWORD dwControlState, wchar_t wch, int ScanCode = -1);
		bool PostLeftClickSync(COORD crDC);
		bool PostConsoleEventPipe(MSG64 *pMsg, size_t cchCount = 1);
		void ShowKeyBarHint(WORD nID);
		bool PostPromptCmd(bool CD, LPCWSTR asCmd);
		void OnKeysSending();
	protected:
		friend class CAltNumpad;
		bool PostString(wchar_t* pszChars, size_t cchCount);
	private:
		void PostMouseEvent(UINT messg, WPARAM wParam, COORD crMouse, bool abForceSend = false);
	public:
		BOOL OpenConsoleEventPipe();
		bool PostConsoleMessage(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam);
		bool SetFullScreen();
		BOOL ShowOtherWindow(HWND hWnd, int swShow, BOOL abAsync=TRUE);
		BOOL SetOtherWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);
		BOOL SetOtherWindowFocus(HWND hWnd, BOOL abSetForeground);
		HWND SetOtherWindowParent(HWND hWnd, HWND hParent);
		BOOL SetOtherWindowRgn(HWND hWnd, int nRects, LPRECT prcRects, BOOL bRedraw);
		void PostDragCopy(BOOL abMove);
		void PostMacro(LPCWSTR asMacro, BOOL abAsync = FALSE);
		wchar_t* PostponeMacro(wchar_t* RVAL_REF asMacro);
		bool GetFarVersion(FarVersion* pfv);
		bool IsFarLua();
		bool StartDebugger(StartDebugType sdt);
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
		wchar_t* mpsz_PostCreateMacro;
		void ProcessPostponedMacro();
		static DWORD WINAPI PostMacroThread(LPVOID lpParameter);
		HANDLE mh_PostMacroThread; DWORD mn_PostMacroThreadID;
		void PostCommand(DWORD anCmdID, DWORD anCmdSize, LPCVOID ptrData);
		DWORD mn_InPostDeadChar;
		void OnKeyboardInt(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, const wchar_t *pszChars, const MSG* pDeadCharMsg);
		struct KeyboardIntArg
		{
			HWND hWnd; UINT messg; WPARAM wParam; LPARAM lParam; const wchar_t *pszChars; const MSG* pDeadCharMsg;
		};
		static bool OnKeyboardBackCall(CVirtualConsole* pVCon, LPARAM lParam);
	public:
		//BOOL FlushInputQueue(DWORD nTimeout = 500);
		void OnKeyboard(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, const wchar_t *pszChars, const MSG* pDeadCharMsg);
		const ConEmuHotKey* ProcessSelectionHotKey(const ConEmuChord& VkState, bool bKeyDown, const wchar_t *pszChars);
		TermEmulationType GetTermType();
		BOOL GetBracketedPaste();
		bool ProcessXtermSubst(const INPUT_RECORD& r);
		void ProcessKeyboard(UINT messg, WPARAM wParam, LPARAM lParam, const wchar_t *pszChars);
		void OnKeyboardIme(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
		bool OnMouse(UINT messg, WPARAM wParam, int x, int y, bool abForceSend = false);
		bool OnMouseSelection(UINT messg, WPARAM wParam, int x, int y);
		void OnScroll(UINT messg, WPARAM wParam, int x, int y, bool abFromTouch = false);
		void OnFocus(BOOL abFocused);
		void OnConsoleDataChanged();

		void StopSignal();
		void StopThread(BOOL abRecreating=FALSE);
		void StartStopXTerm(DWORD nPID, bool xTerm);
		void StartStopBracketedPaste(DWORD nPID, bool bUseBracketedPaste);
		void StartStopAppCursorKeys(DWORD nPID, bool bAppCursorKeys);
		void PortableStarted(CESERVER_REQ_PORTABLESTARTED* pStarted);
		bool InScroll();
		BOOL isBufferHeight();
		BOOL isAlternative();
		HWND isPictureView(BOOL abIgnoreNonModal=FALSE);
		BOOL isWindowVisible();
		LPCTSTR GetTitle(bool abGetRenamed=false);
		LPCWSTR GetPanelTitle();
		LPCWSTR GetTabTitle(CTab& tab);
		void GetConsoleScreenBufferInfo(CONSOLE_SCREEN_BUFFER_INFO* sbi);
		void GetConsoleInfo(ConsoleInfoArg* pInfo);
		//void GetConsoleCursorPos(COORD *pcr);
		bool QueryPromptStart(COORD *cr);
		void GetConsoleCursorInfo(CONSOLE_CURSOR_INFO *ci, COORD *cr = NULL);
		DWORD GetConsoleCP();
		DWORD GetConsoleOutputCP();
		void GetConsoleModes(WORD& nConInMode, WORD& nConOutMode, TermEmulationType& Term, BOOL& bBracketedPaste);
		void SyncConsole2Window(BOOL abNtvdmOff=FALSE, LPRECT prcNewWnd=NULL);
		void SyncGui2Window(const RECT rcVConBack);
		//void OnWinEvent(DWORD anEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
		HWND OnServerStarted(const HWND ahConWnd, const DWORD anServerPID, const DWORD dwKeybLayout, CESERVER_REQ_SRVSTARTSTOPRET& pRet);
		void OnDosAppStartStop(enum StartStopType sst, DWORD anPID);
		bool isProcessExist(DWORD anPID);
		int  GetProcesses(ConProcess** ppPrc, bool ClientOnly = false);
		DWORD GetFarPID(bool abPluginRequired=false);
		void SetFarPID(DWORD nFarPID);
		void SetFarPluginPID(DWORD nFarPluginPID);
		void SetProgramStatus(DWORD nDrop, DWORD nSet);
		void SetFarStatus(DWORD nNewFarStatus);
		bool GetProcessInformation(DWORD nPID, ConProcess* rpProcess = NULL);
		LPCWSTR GetConsoleInfo(LPCWSTR asWhat, CEStr& rsInfo);
		LPCWSTR GetActiveProcessInfo(CEStr& rsInfo);
		DWORD GetActivePID(ConProcess* rpProcess = NULL);
		DWORD GetInteractivePID();
		DWORD GetLoadedPID();
		DWORD GetRunningPID();
		LPCWSTR GetActiveProcessName();
		CEActiveAppFlags GetActiveAppFlags();
		int GetActiveAppSettingsId(bool bReload = false);
	private:
		int GetDefaultAppSettingsId();
	public:
		void ResetActiveAppSettingsId();
		DWORD GetProgramStatus();
		DWORD GetFarStatus();
		bool isServerAlive();
		DWORD GetServerPID(bool bMainOnly = false);
		DWORD GetTerminalPID();
		DWORD GetMonitorThreadID();
		bool isServerCreated(bool bFullRequired = false);
		bool isServerAvailable();
		bool isServerClosing(bool bStrict = false);
		void ResetTopLeft();
		LRESULT DoScroll(int nDirection, UINT nCount = 1);
		bool GetConsoleSelectionInfo(CONSOLE_SELECTION_INFO *sel);
		bool isConSelectMode();
		bool isCygwinMsys();
		bool isFar(bool abPluginRequired=false);
		bool isFarBufferSupported();
		bool isSendMouseAllowed();
		bool isFarInStack();
		bool isFarKeyBarShown();
		bool isInternalScroll();
		bool isSelectionAllowed();
		bool isSelectionPresent();
		bool isMouseSelectionPresent();
		bool isPaused();
		CEPauseCmd Pause(CEPauseCmd cmd);
		void AutoCopyTimer(); // Чтобы разрулить "Auto Copy" & "Double click - select word"
		void StartSelection(BOOL abTextMode, SHORT anX=-1, SHORT anY=-1, BOOL abByMouse=FALSE, DWORD anAnchorFlag=0);
		void ChangeSelectionByKey(UINT vkKey, bool bCtrl, bool bShift);
		void ExpandSelection(SHORT anX, SHORT anY);
		bool DoSelectionCopy(CECopyMode CopyMode = cm_CopySel, BYTE nFormat = CTSFormatDefault, LPCWSTR pszDstFile = NULL);
		void DoSelectionStop();
		void OnSelectionChanged();
		void DoFindText(int nDirection);
		void DoEndFindText();
		void PasteExplorerPath(bool bDoCd = true, bool bSetFocus = true);
		void CtrlWinAltSpace();
		void ShowConsoleOrGuiClient(int nMode); // -1 Toggle, 0 - Hide, 1 - Show
		void ShowConsole(int nMode); // -1 Toggle, 0 - Hide, 1 - Show
		void ShowGuiClientExt(int nMode, BOOL bDetach = FALSE); // -1 Toggle, 0 - Hide, 1 - Show
		void ShowGuiClientInt(bool bShow);
		void ChildSystemMenu();
		bool isDetached();
		BOOL AttachConemuC(HWND ahConWnd, DWORD anConemuC_PID, const CESERVER_REQ_STARTSTOP* rStartStop, CESERVER_REQ_SRVSTARTSTOPRET& pRet);
		void QueryStartStopRet(CESERVER_REQ_SRVSTARTSTOPRET& pRet);
		void SetInitEnvCommands(CESERVER_REQ_SRVSTARTSTOPRET& pRet);
		BOOL RecreateProcess(RConStartArgs *args);
		void GetConsoleData(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, ConEmuTextRange& etr);
		void ResetHighlightHyperlinks();
		ExpandTextRangeType GetLastTextRangeType();
		bool IsFarHyperlinkAllowed(bool abFarRequired);
	private:
		bool PreCreate(RConStartArgs *args);

		BOOL GetConsoleLine(int nLine, wchar_t** pChar, /*CharAttr** pAttr,*/ int* pLen, MSectionLock* pcsData);

		CDpiForDialog* mp_RenameDpiAware;
		static INT_PTR CALLBACK renameProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam);
	public:
		BOOL IsConsoleDataChanged();
		void OnActivate(int nNewNum, int nOldNum);
		void OnDeactivate(int nNewNum);
		void ShowHideViews(BOOL abShow);
		void OnGuiFocused(BOOL abFocus, BOOL abForceChild = FALSE);
		void UpdateServerActive(BOOL abImmediate = FALSE);
		BOOL CheckBufferSize();
		//LRESULT OnConEmuCmd(BOOL abStarted, DWORD anConEmuC_PID);
		BOOL BufferHeightTurnedOn(CONSOLE_SCREEN_BUFFER_INFO* psbi);
		void UpdateScrollInfo();
		void SetTabs(ConEmuTab* apTabs, int anTabsCount, DWORD anFarPID);
		void DoRenameTab();
		bool DuplicateRoot(bool bSkipMsg = false, bool bRunAsAdmin = false, LPCWSTR asNewConsole = NULL, LPCWSTR asApp = NULL, LPCWSTR asParm = NULL);
		void RenameTab(LPCWSTR asNewTabText = NULL);
		void RenameWindow(LPCWSTR asNewWindowText = NULL);
		int GetTabCount(BOOL abVisibleOnly = FALSE);
		int GetRootProcessIcon();
		LPCWSTR GetRootProcessName();
		void NeedRefreshRootProcessIcon();
		int GetActiveTab();
		CEFarWindowType GetActiveTabType();
		bool GetTab(int tabIdx, /*OUT*/ CTab& rTab);
		int GetModifiedEditors();
		bool ActivateFarWindow(int anWndIndex);
		DWORD CanActivateFarWindow(int anWndIndex);
		bool IsSwitchFarWindowAllowed();
		LPCWSTR GetActivateFarWindowError(wchar_t* pszBuffer, size_t cchBufferMax);
		void OnConsoleKeyboardLayout(DWORD dwNewLayout);
		void SwitchKeyboardLayout(WPARAM wParam,DWORD_PTR dwNewKeybLayout);
		void CloseConsole(bool abForceTerminate, bool abConfirm, bool abAllowMacro = true);
		void CloseConsoleWindow(bool abConfirm);
		bool TerminateAllButShell(bool abConfirm);
		bool TerminateActiveProcess(bool abConfirm, DWORD nPID);
		bool TerminateActiveProcessConfirm(DWORD nPID);
		bool ChangeAffinityPriority(LPCWSTR asAffinity = NULL, LPCWSTR asPriority = NULL);
		bool isCloseTabConfirmed(CEFarWindowType TabType, LPCWSTR asConfirmation, bool bForceAsk = false);
		void CloseConfirmReset();
		bool CanCloseTab(bool abPluginRequired = false);
		void CloseTab();
		bool isConsoleClosing();
		bool isConsoleReady();
		void OnServerClosing(DWORD anSrvPID, int* pnShellExitCode);
		void Paste(CEPasteMode PasteMode = pm_Standard, LPCWSTR asText = NULL, bool abNoConfirm = false, bool abCygWin = false);
		bool Write(LPCWSTR pszText, int nLen = -1, DWORD* pnWritten = NULL);
		void LogString(LPCSTR asText);
		void LogString(LPCWSTR asText);
		bool isActive(bool abAllowGroup);
		bool isInFocus();
		bool isFarPanelAllowed();
		bool isFilePanel(bool abPluginAllowed = false, bool abSkipEditViewCheck = false, bool abSkipDialogCheck = false);
		bool isEditor();
		bool isEditorModified();
		bool isHighlighted();
		bool isViewer();
		bool isVisible();
		bool isNtvdm();
		bool isFixAndCenter(COORD* lpcrConSize = NULL);
		//bool isPackets();
		const RConStartArgs& GetArgs();
		void SetPaletteName(LPCWSTR asPaletteName);
		LPCWSTR GetCmd(bool bThisOnly = false);
		LPCWSTR GetStartupDir();
		wchar_t* CreateCommandLine(bool abForTasks = false);
		bool GetUserPwd(const wchar_t** ppszUser, const wchar_t** ppszDomain, bool* pbRestricted);
		short GetProgress(int* rpnState/*1-error,2-ind*/, BOOL* rpbNotFromTitle = NULL);
		bool SetProgress(short nState, short nValue, LPCWSTR pszName = NULL);
		void UpdateGuiInfoMapping(const ConEmuGuiMapping* apGuiInfo);
		void UpdateFarSettings(DWORD anFarPID = 0, FAR_REQ_FARSETCHANGED* rpSetEnvVar = NULL);
		void UpdateTextColorSettings(BOOL ChangeTextAttr = TRUE, BOOL ChangePopupAttr = TRUE, const AppSettings* apDistinct = NULL);
		int CoordInPanel(COORD cr, BOOL abIncludeEdges = FALSE);
		BOOL GetPanelRect(BOOL abRight, RECT* prc, BOOL abFull = FALSE, BOOL abIncludeEdges = FALSE);
		bool isAdministrator();
		BOOL isMouseButtonDown();
		void OnConsoleLangChange(DWORD_PTR dwNewKeybLayout);
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
		void GetStartTime(SYSTEMTIME& st);
		LPCWSTR GetConsoleStartDir(CEStr& szDir);
		LPCWSTR GetFileFromConsole(LPCWSTR asSrc, CEStr& szFull);
		LPCWSTR GetConsoleCurDir(CEStr& szDir);
		void GetPanelDirs(CEStr& szActiveDir, CEStr& szPassive);

	public:
		BOOL IsConsoleThread();
		void SetForceRead();
		void UpdateCursorInfo();
		bool isNeedCursorDraw();
		bool Detach(bool bPosted = false, bool bSendCloseConsole = false);
		void Unfasten();
		void AdminDuplicate();
		const CEFAR_INFO_MAPPING *GetFarInfo(); // FarVer и прочее
		bool InCreateRoot();
		bool InRecreate();
		BOOL GuiAppAttachAllowed(DWORD anServerPID, LPCWSTR asAppFileName, DWORD anAppPID);
		//LPCWSTR GetLngNameTime();
		void ShowPropertiesDialog();
		void LogInput(UINT uMsg, WPARAM wParam, LPARAM lParam, LPCWSTR pszTranslatedChars = NULL);

		//static void Box(LPCTSTR szText, DWORD nBtns = 0);

		void OnStartProcessAllowed();
		void OnTimerCheck();

		static bool RefreshAfterRestore(CVirtualConsole* pVCon, LPARAM lParam);

	public:
		void MonitorAssertTrap();
	private:
		bool mb_MonitorAssertTrap;

	protected:
		void SetMainSrvPID(DWORD anMainSrvPID, HANDLE ahMainSrv);
		void SetAltSrvPID(DWORD anAltSrvPID/*, HANDLE ahAltSrv*/);
		void SetTerminalPID(DWORD anTerminalPID);
		// Сервер и альтернативный сервер
		DWORD mn_MainSrv_PID; HANDLE mh_MainSrv;
		u64 mn_ProcessAffinity; DWORD mn_ProcessPriority;
		CDpiForDialog* mp_PriorityDpiAware;
		void RepositionDialogWithTab(HWND hDlg);
		static INT_PTR CALLBACK priorityProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam);
		DWORD mn_CheckFreqLock;
		DWORD mn_ConHost_PID;
		MMap<DWORD,BOOL>* mp_ConHostSearch;
		void ConHostSearchPrepare();
		DWORD ConHostSearch(bool bFinal);
		void ConHostSetPID(DWORD nConHostPID);
		bool  mb_MainSrv_Ready; // Сервер готов принимать команды?
		DWORD mn_ActiveLayout;
		DWORD mn_AltSrv_PID;  //HANDLE mh_AltSrv;
		DWORD mn_Terminal_PID; // cygwin/msys connector
		HANDLE mh_SwitchActiveServer, mh_ActiveServerSwitched;
		bool mb_SwitchActiveServer;
		enum SwitchActiveServerEvt { eDontChange, eSetEvent, eResetEvent };
		void SetSwitchActiveServer(bool bSwitch, SwitchActiveServerEvt eCall, SwitchActiveServerEvt eResult);
		bool InitAltServer(DWORD nAltServerPID/*, HANDLE hAltServer*/);
		bool ReopenServerPipes();

		// Пайп консольного ввода
		wchar_t ms_ConEmuCInput_Pipe[MAX_PATH];
		HANDLE mh_ConInputPipe; // wsprintf(ms_ConEmuCInput_Pipe, CESERVERINPUTNAME, L".", mn_ConEmuC_PID)

		BOOL mb_InCreateRoot;
		BOOL mb_UseOnlyPipeInput;
		TCHAR ms_ConEmuC_Pipe[MAX_PATH], ms_MainSrv_Pipe[MAX_PATH], ms_VConServer_Pipe[MAX_PATH];
		//TCHAR ms_ConEmuC_DataReady[64]; HANDLE mh_ConEmuC_DataReady;
		void InitNames();
		// Текущий заголовок консоли и его значение для сравнения (для определения изменений)
		WCHAR Title[MAX_TITLE_SIZE+1], TitleCmp[MAX_TITLE_SIZE+1];
		// А здесь содержится то, что отображается в ConEmu (может быть добавлено " (Admin)")
		WCHAR TitleFull[MAX_TITLE_SIZE+96], TitleAdmin[MAX_TITLE_SIZE+192];
		// Буфер для CRealConsole::GetTitle
		wchar_t TempTitleRenamed[MAX_RENAME_TAB_LEN/*128*/];
		// Принудительно дернуть OnTitleChanged, например, при изменении процентов в консоли
		BOOL mb_ForceTitleChanged;
		// Здесь сохраняется заголовок окна (с панелями), когда FAR фокус с панелей уходит (переходит в редактор...).
		WCHAR ms_PanelTitle[CONEMUTABMAX];
		// Процентики
		struct {
			short Progress, LastShownProgress;
			short PreWarningProgress; DWORD LastWarnCheckTick;
			short ConsoleProgress, LastConsoleProgress; DWORD LastConProgrTick;
			short AppProgressState, AppProgress; // Может быть задан из консоли (Ansi codes, Macro)
		} m_Progress;
		// a-la properties
		void setProgress(short value);
		void setLastShownProgress(short value);
		void setPreWarningProgress(short value);
		void setConsoleProgress(short value);
		void setLastConsoleProgress(short value, bool UpdateTick);
		void setAppProgress(short AppProgressState, short AppProgress);
		void logProgress(LPCWSTR asFormat, int V1, int V2 = 0);
		// method
		short CheckProgressInTitle();
		//short CheckProgressInConsole(const wchar_t* pszCurLine);
		//void SetProgress(short anProgress); // установить переменную mn_Progress и mn_LastProgressTick

#if 0
		BOOL AttachPID(DWORD dwPID); //120714 - аналогичные параметры работают в ConEmuC.exe, а в GUI они и не работали. убрал пока
#endif
		BOOL StartProcess();
		static BOOL CreateOrRunAs(CRealConsole* pRCon, RConStartArgs& Args, LPWSTR psCurCmd, LPCWSTR& lpszWorkDir, STARTUPINFO& si, PROCESS_INFORMATION& pi, SHELLEXECUTEINFO*& pp_sei, DWORD& dwLastError, bool bExternal = false);
		private:
		BOOL StartProcessInt(LPCWSTR& lpszCmd, wchar_t*& psCurCmd, LPCWSTR& lpszWorkDir, bool bNeedConHostSearch, HWND hSetForeground, DWORD& nCreateBegin, DWORD& nCreateEnd, DWORD& nCreateDuration, BYTE nTextColorIdx /*= 7*/, BYTE nBackColorIdx /*= 0*/, BYTE nPopTextColorIdx /*= 5*/, BYTE nPopBackColorIdx /*= 15*/, STARTUPINFO& si, PROCESS_INFORMATION& pi, DWORD& dwLastError);
		void ResetVarsOnStart();
		protected:
		BOOL StartMonitorThread();
		void SetMonitorThreadEvent();
		BOOL mb_NeedStartProcess;

		// Нить наблюдения за консолью
		static DWORD WINAPI MonitorThread(LPVOID lpParameter);
		DWORD MonitorThreadWorker(bool bDetached, bool& rbChildProcessCreated);
		static int WorkerExFilter(unsigned int code, struct _EXCEPTION_POINTERS *ep, LPCTSTR szFile, UINT nLine);
		HANDLE mh_MonitorThread; DWORD mn_MonitorThreadID; BOOL mb_WasForceTerminated;
		HANDLE mh_MonitorThreadEvent;
		HANDLE mh_UpdateServerActiveEvent;
		DWORD mn_ServerActiveTick1, mn_ServerActiveTick2;
		//BOOL mb_UpdateServerActive;
		DWORD mn_LastUpdateServerActive;
		// Для пересылки событий ввода в консоль
		//static DWORD WINAPI InputThread(LPVOID lpParameter);
		//HANDLE mh_InputThread; DWORD mn_InputThreadID;

		DWORD mn_TermEventTick;
		HANDLE mh_TermEvent, mh_ApplyFinished;
		HANDLE mh_StartExecuted;
		BOOL mb_StartResult, mb_WaitingRootStartup;
		BOOL mb_FullRetrieveNeeded; //, mb_Detached;
		RConStartArgs m_Args;
		SYSTEMTIME m_StartTime;
		CEStr ms_DefTitle;
		CEStr ms_StartWorkDir;
		CEStr ms_CurWorkDir;
		CEStr ms_CurPassiveDir;
		MSectionSimple* mpcs_CurWorkDir;
		void StoreCurWorkDir(CESERVER_REQ_STORECURDIR* pNewCurDir);
		bool ReloadFarWorkDir();
		//wchar_t ms_ProfilePathTemp[MAX_PATH+1]; -- commented code
		bool mb_WasStartDetached;
		SYSTEMTIME mst_ServerStartingTime;
		void SetRootProcessName(LPCWSTR asProcessName);
		wchar_t ms_RootProcessName[MAX_PATH];
		int mn_RootProcessIcon;
		bool mb_NeedLoadRootProcessIcon;
		CESERVER_ROOT_INFO m_RootInfo;
		void UpdateRootInfo(const CESERVER_ROOT_INFO& RootInfo);
		// Replace in asCmd some env.vars (!ConEmuBackHWND! and so on)
		//wchar_t* ParseConEmuSubst(LPCWSTR asCmd);
		//wchar_t* mpsz_CmdBuffer;
		//BOOL mb_AdminShieldChecked;
		//wchar_t* ms_SpecialCmd;
		//BOOL mb_RunAsAdministrator;

		//BOOL RetrieveConsoleInfo(/*BOOL bShortOnly,*/ UINT anWaitSize);
		BOOL WaitConsoleSize(int anWaitSize, DWORD nTimeout);
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
		BOOL mb_DebugLocked; // для отладки - заморозить все нити, чтобы не мешали отладчику, ставится по контекстному меню
		#endif


		//BOOL mb_ThawRefreshThread;
		struct ServerClosing
		{
			DWORD  nServerPID;     // PID закрывающегося сервера
			DWORD  nRecieveTick;   // Tick, когда получено сообщение о закрытии
			HANDLE hServerProcess; // Handle процесса сервера
			BOOL   bBackActivated; // Main server was activated back, when AltServer was closed
		} m_ServerClosing;
		//
		MSection csPRC; //DWORD ncsTPRC;
		MArray<ConProcess> m_Processes;
		int mn_ProcessCount, mn_ProcessClientCount;
		DWORD m_FarPlugPIDs[128];
		UINT mn_FarPlugPIDsCount;
		BOOL mb_SkipFarPidChange;
		DWORD m_TerminatedPIDs[128]; UINT mn_TerminatedIdx;
		//
		DWORD mn_FarPID;
		int   mn_FarNoPanelsCheck; // "Far /e ..."
		ConProcess m_ActiveProcess;
		ConProcess m_AppDistinctProcess;
		bool mb_ForceRefreshAppId;
		wchar_t ms_LastActiveProcess[64]; // Used internally by GetProcessInformation
		void SetActivePID(const ConProcess* apProcess);
		void SetAppDistinctPID(const ConProcess* apProcess);
		DWORD mn_LastSetForegroundPID; // PID процесса, которому в последний раз было разрешено AllowSetForegroundWindow
		DWORD mn_LastProcessNamePID;
		int mn_LastAppSettingsId;
		//
		struct _TabsInfo
		{
			CTabStack m_Tabs;
			CTab* mp_ActiveTab;
			int  mn_tabsCount; // Число текущих табов. Может отличаться (в меньшую сторону) от m_Tabs.GetCount()
			bool mb_WasInitialized; // Информационно, чтобы ассертов не было
			bool mb_TabsWasChanged;
			bool mb_HasModalWindow; // Far Manager modal editor/viewer
			CEFarWindowType nActiveType;
			int  nActiveIndex;
			int  nActiveFarWindow;
			bool bConsoleDataChanged;
			LONG nFlashCounter;
			wchar_t sTabActivationErr[128];
			void StoreActiveTab(int anActiveIndex, CTab& ActiveTab);
			bool RefreshFarPID(DWORD nNewPID);
		} tabs;
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

		friend class CRealServer;
		friend class CGuiServer;
		CRealServer m_RConServer;

		//void ApplyConsoleInfo(CESERVER_REQ* pInfo);
		void SetHwnd(HWND ahConWnd, BOOL abForceApprove = FALSE);
		void CheckVConRConPointer(bool bForceSet);
		WORD mn_LastVKeyPressed;
		BOOL GetConWindowSize(const CONSOLE_SCREEN_BUFFER_INFO& sbi, int& nNewWidth, int& nNewHeight, BOOL* pbBufferHeight=NULL);
		int mn_Focused; //-1 после запуска, 1 - в фокусе, 0 - не в фокусе
		DWORD mn_InRecreate; // Tick, когда начали пересоздание
		BOOL mb_RecreateFailed;
		DWORD mn_StartTick; // для определения GetRunTime()
		DWORD mn_DeactivateTick; // чтобы не мигать сразу после "cmd -new_console" из промпта
		DWORD mn_RunTime; // для информации
		DWORD GetRunTime();
		bool mb_WasVisibleOnce;
		BOOL mb_ProcessRestarted;
		BOOL mb_InCloseConsole;
		DWORD mn_CloseConfirmedTick;
		bool mb_CloseFarMacroPosted;
		// Логи
		BYTE m_UseLogs;
		//HANDLE mh_LogInput; wchar_t *mpsz_LogInputFile/*, *mpsz_LogPackets*/; //UINT mn_LogPackets;
		MFileLog *mp_Log;
		void CreateLogFiles();
		void CloseLogFiles();
		void LogInput(INPUT_RECORD* pRec);
		//void LogPacket(CESERVER_REQ* pInfo);
		BOOL RecreateProcessStart();
		void RequestStartup(bool bForce = false);
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
		// CECONMAPNAME
		MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> m_ConsoleMap;
		// CECONAPPMAPNAME
		MFileMapping<CESERVER_CONSOLE_APP_MAPPING> m_AppMap;
		// CEFARMAPNAME: FarVer and others
		CEFAR_INFO_MAPPING m_FarInfo;
		// Don't use directly, but via ms_FarInfoCS only!
		MFileMapping<const CEFAR_INFO_MAPPING> m__FarInfo;
		// CS Lock for m__FarInfo
		MSection ms_FarInfoCS;

		// Colorer Mapping
		//HANDLE mh_ColorMapping;
		//AnnotationHeader *mp_ColorHdr;
		MFileMapping<AnnotationHeader> m_TrueColorerMap;
		AnnotationHeader m_TrueColorerHeader;
		const AnnotationInfo *mp_TrueColorerData;
		int mn_TrueColorMaxCells;
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
		bool mb_DataChanged;
		void OnServerStarted(DWORD anServerPID, HANDLE ahServerHandle, DWORD dwKeybLayout, BOOL abFromAttach = FALSE);
		void OnStartedSuccess();
		BOOL mb_RConStartedSuccess;
		//
		struct TermEmulation
		{
			DWORD nCallTermPID; // PID процесса запросившего эмуляцию терминала
			TermEmulationType Term;
			BOOL  bBracketedPaste; // All "pasted" text will be wrapped in `\e[200~ ... \e[201~`
		} m_Term;
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
		// для Far Manager: облегчить "попадание" тапами
		BOOL mb_MouseTapChanged;
		COORD mcr_MouseTapReal, mcr_MouseTapChanged;
		//
		SHELLEXECUTEINFO *mp_sei, *mp_sei_dbg;
		//
		HWND FindPicViewFrom(HWND hFrom);
		//
		bool isCharBorderVertical(WCHAR inChar);
		bool isCharBorderLeftVertical(WCHAR inChar);
		bool isCharBorderHorizontal(WCHAR inChar);
		bool ConsoleRect2ScreenRect(const RECT &rcCon, RECT *prcScr);

		bool mb_InPostCloseMacro;
		bool mb_WasMouseSelection; // Useful to know when processing LBtnUp

		// Searching for files on the console surface (hyperlinking)
		CRConFiles* mp_Files;

		// XTerm keyboard substitutions
		TermX* mp_XTerm;
};

//#define Assert(V) if ((V)==FALSE) { wchar_t szAMsg[MAX_PATH*2]; _wsprintf(szAMsg, SKIPLEN(countof(szAMsg)) L"Assertion (%s) at\n%s:%i\n\nPress <Retry> to report a bug (web page)", _T(#V), _T(__FILE__), __LINE__); CRealConsole::Box(szAMsg, MB_RETRYCANCEL); }
