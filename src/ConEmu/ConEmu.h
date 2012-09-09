
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

#define WM_TRAYNOTIFY WM_USER+1

#include "MenuIds.h"

#define IID_IShellLink IID_IShellLinkW

#define GET_X_LPARAM(inPx) ((int)(short)LOWORD(inPx))
#define GET_Y_LPARAM(inPy) ((int)(short)HIWORD(inPy))

//#define RCLICKAPPSTIMEOUT 600
//#define RCLICKAPPS_START 100 // начало отрисовки кружка вокруг курсора
//#define RCLICKAPPSTIMEOUT_MAX 10000
//#define RCLICKAPPSDELTA 3
#define DRAG_DELTA 5

//typedef DWORD (WINAPI* FGetModuleFileNameEx)(HANDLE hProcess,HMODULE hModule,LPWSTR lpFilename,DWORD nSize);

//typedef HRESULT(WINAPI* FDwmIsCompositionEnabled)(BOOL *pfEnabled);

class CConEmuChild;
class CConEmuBack;
class TabBarClass;
class CConEmuMacro;
class CAttachDlg;
class CRecreateDlg;
class CToolTip;
class CGestures;
class CVConGuard;
class CVConGroup;
class CStatus;
enum ConEmuWindowMode;


struct MsgSrvStartedArg
{
	HWND  hConWnd;
	DWORD nSrcPID;
	DWORD timeStart;
	DWORD timeRecv;
	DWORD timeFin;
	HWND  hWndDc;
	HWND  hWndBack;
};

#include "DwmHelper.h"
#include "TaskBar.h"
#include "FrameHolder.h"
#include "GuiServer.h"
#include "GestureEngine.h"
#include "ConEmuCtrl.h"


class CConEmuMain :
	public CDwmHelper,
	public CTaskBar,
	public CFrameHolder,
	public CGestures,
	public CConEmuCtrl
{
	public:
		//HMODULE mh_Psapi;
		//FGetModuleFileNameEx GetModuleFileNameEx;
		wchar_t ms_ConEmuDefTitle[32];          // Название с версией, например "ConEmu 110117 (32)"
		wchar_t ms_ConEmuBuild[16];             // номер сборки, например "110117"
		wchar_t ms_ConEmuExe[MAX_PATH+1];       // полный путь к ConEmu.exe (GUI)
		wchar_t ms_ConEmuExeDir[MAX_PATH+1];    // БЕЗ завершающего слеша. Папка содержит ConEmu.exe
		wchar_t ms_ConEmuBaseDir[MAX_PATH+1];   // БЕЗ завершающего слеша. Папка содержит ConEmuC.exe, ConEmuHk.dll, ConEmu.xml
		wchar_t ms_ComSpecInitial[MAX_PATH];
		wchar_t *mps_IconPath;
		BOOL mb_DosBoxExists;
		BOOL mb_MingwMode;   // ConEmu установлен как пакет MinGW
		BOOL mb_MSysStartup; // найден баш из msys: "%ConEmuDir%\..\msys\1.0\bin\sh.exe" (MinGW mode)
		// Portable Far Registry
		BOOL mb_PortableRegExist;
		wchar_t ms_PortableRegHive[MAX_PATH]; // полный путь к "Portable.S-x-x-..."
		wchar_t ms_PortableRegHiveOrg[MAX_PATH]; // путь к "Portable.S-x-x-..." (в ConEmu). этот файл мог быть скопирован в ms_PortableRegHive
		wchar_t ms_PortableReg[MAX_PATH]; // "Portable.reg"
		HKEY mh_PortableMountRoot; // Это HKEY_CURRENT_USER или HKEY_USERS
		wchar_t ms_PortableMountKey[MAX_PATH];
		BOOL mb_PortableKeyMounted;
		wchar_t ms_PortableTempDir[MAX_PATH];
		HKEY mh_PortableRoot; // Это открытый ключ
		bool PreparePortableReg();
		bool mb_UpdateJumpListOnStartup;
	public:
		// Режим интеграции. Запуститься как дочернее окно, например, в области проводника.
		enum {
			ii_None = 0,
			ii_Auto,
			ii_Explorer,
			ii_Simple,
		} m_InsideIntegration;
		bool  mb_InsideIntegrationShift;
		bool  mb_InsideSynchronizeCurDir;
		DWORD mn_InsideParentPID;  // PID "родительского" процесса режима интеграции
		HWND  mh_InsideParentWND; // Это окно используется как родительское в режиме интеграции
		HWND  mh_InsideParentRoot; // Корневое окно режима интеграции (для проверки isMeForeground)
		HWND  InsideFindParent();
	private:
		HWND  mh_InsideParentRel;  // Может быть NULL (ii_Simple). HWND относительно которого нужно позиционироваться
		HWND  mh_InsideParentPath; // Win7 Text = "Address: D:\MYDOC"
		HWND  mh_InsideParentCD;   // Edit для смены текущей папки, например -> "C:\USERS"
		RECT  mrc_InsideParent, mrc_InsideParentRel; // для сравнения, чтоб знать, что подвинуться нада
		wchar_t ms_InsideParentPath[MAX_PATH+1];
		static BOOL CALLBACK EnumInsideFindParent(HWND hwnd, LPARAM lParam);
		HWND  InsideFindConEmu(HWND hFrom);
		bool  InsideFindShellView(HWND hFrom);
		void  InsideParentMonitor();
		void  InsideUpdateDir();
		void  InsideUpdatePlacement();
	private:
		bool CheckBaseDir();
		BOOL CheckDosBoxExists();
		void CheckPortableReg();
		void FinalizePortableReg();
		wchar_t ms_ConEmuXml[MAX_PATH+1];       // полный путь к портабельным настройкам
		wchar_t ms_ConEmuIni[MAX_PATH+1];       // полный путь к портабельным настройкам
	public:
		LPWSTR ConEmuXml();
		LPWSTR ConEmuIni();
		wchar_t ms_ConEmuChm[MAX_PATH+1];       // полный путь к chm-файлу (help)
		wchar_t ms_ConEmuC32Full[MAX_PATH+12];  // полный путь к серверу (ConEmuC.exe) с длинными именами
		wchar_t ms_ConEmuC64Full[MAX_PATH+12];  // полный путь к серверу (ConEmuC64.exe) с длинными именами
		LPCWSTR ConEmuCExeFull(LPCWSTR asCmdLine=NULL);
		//wchar_t ms_ConEmuCExeName[32];        // имя сервера (ConEmuC.exe или ConEmuC64.exe) -- на удаление
		wchar_t ms_ConEmuCurDir[MAX_PATH+1];    // БЕЗ завершающего слеша. Папка запуска ConEmu.exe (GetCurrentDirectory)
		void RefreshConEmuCurDir();
		wchar_t *mpsz_ConEmuArgs;    // Аргументы
		void GetComSpecCopy(ConEmuComspec& ComSpec);
		void CreateGuiAttachMapping(DWORD nGuiAppPID);
	private:
		ConEmuGuiMapping m_GuiInfo;
		MFileMapping<ConEmuGuiMapping> m_GuiInfoMapping;
		MFileMapping<ConEmuGuiMapping> m_GuiAttachMapping;
		void FillConEmuMainFont(ConEmuMainFont* pFont);
		void UpdateGuiInfoMapping();
		int mn_QuakePercent; // 0 - отключен
	public:
		//CConEmuChild *m_Child;
		//CConEmuBack  *m_Back;
		//CConEmuMacro *m_Macro;
		TabBarClass *mp_TabBar;
		CStatus *mp_Status;
		CToolTip *mp_Tip;
		//POINT cwShift; // difference between window size and client area size for main ConEmu window
		POINT ptFullScreenSize; // size for GetMinMaxInfo in Fullscreen mode
		//DWORD gnLastProcessCount;
		//uint cBlinkNext;
		ConEmuWindowMode WindowMode;        // rNormal/rMaximized/rFullScreen
		DWORD wndWidth, wndHeight;
		int   wndX, wndY; // в пикселях
		ConEmuWindowMode change2WindowMode; // -1/rNormal/rMaximized/rFullScreen
		bool WindowStartMinimized, ForceMinimizeToTray;
		bool DisableAutoUpdate;
		bool DisableKeybHooks;
		bool mb_isFullScreen;
		BOOL mb_ExternalHidden;
		//HANDLE hPipe;
		//HANDLE hPipeEvent;
		bool isWndNotFSMaximized;
		BOOL mb_StartDetached;
		//bool isShowConsole;
		//bool mb_FullWindowDrag;
		//bool isLBDown, /*isInDrag,*/ isDragProcessed,
		//mb_InSizing, -> state&MOUSE_SIZING
		//mb_IgnoreMouseMove;
		//bool isRBDown, ibSkipRDblClk; DWORD dwRBDownTick;
		struct
		{
			WORD  state;
			bool  bSkipRDblClk;
			bool  bIgnoreMouseMove;
			bool  bCheckNormalRect;

			COORD LClkDC, LClkCon;
			POINT LDblClkDC; // заполняется в PatchMouseEvent
			DWORD LDblClkTick;
			COORD RClkDC, RClkCon;
			DWORD RClkTick;

			// Чтобы не слать в консоль бесконечные WM_MOUSEMOVE
			UINT   lastMsg;
			WPARAM lastMMW;
			LPARAM lastMML;

			// Пропустить клик мышкой (окно было неактивно)
			UINT nSkipEvents[2]; UINT nReplaceDblClk;
			// не пропускать следующий клик в консоль!
			BOOL bForceSkipActivation;

			// таскание окошка за клиентскую область
			POINT ptWndDragStart;
			RECT  rcWndDragStart;

			// настройки скролла мышкой (сколько линий/символов "за клик")
			UINT nWheelScrollChars, nWheelScrollLines;
			void  ReloadWheelScroll()
			{
				#ifndef SPI_GETWHEELSCROLLCHARS
				#define SPI_GETWHEELSCROLLCHARS   0x006C
				#endif
				if (!SystemParametersInfo(SPI_GETWHEELSCROLLCHARS, 0, &nWheelScrollChars, 0) || !nWheelScrollChars)
					nWheelScrollChars = 3;
				if (!SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &nWheelScrollLines, 0) || !nWheelScrollLines)
					nWheelScrollLines = 3;
			};
			UINT GetWheelScrollChars()
			{
				if (!nWheelScrollChars)
					ReloadWheelScroll();
				return nWheelScrollChars;
			};
			UINT GetWheelScrollLines()
			{
				if (!nWheelScrollLines)
					ReloadWheelScroll();
				return nWheelScrollLines;
			};
		} mouse;
		bool isPiewUpdate;
		bool gbPostUpdateWindowSize;
		HWND hPictureView; bool bPicViewSlideShow; DWORD dwLastSlideShowTick; RECT mrc_WndPosOnPicView;
		HWND mh_ShellWindow; // Окно Progman для Desktop режима
		DWORD mn_ShellWindowPID;
		BOOL mb_FocusOnDesktop;
		//bool gb_ConsoleSelectMode;
		//bool setParent, setParent2;
		//BOOL mb_InClose;
		//int RBDownNewX, RBDownNewY;
		POINT cursor, Rcursor;
		//WPARAM lastMMW;
		//LPARAM lastMML;
		//COORD m_LastConSize; // console size after last resize (in columns and lines)
		bool mb_IgnoreSizeChange;
		bool mb_InCaptionChange;
		DWORD m_FixPosAfterStyle;
		RECT mrc_FixPosAfterStyle;
		//bool mb_IgnoreStoreNormalRect;
		//TCHAR szConEmuVersion[32];
		DWORD m_ProcCount;
		//DWORD mn_ActiveStatus;
		//TCHAR ms_EditorRus[16], ms_ViewerRus[16], ms_TempPanel[32], ms_TempPanelRus[32];
		//OSVERSIONINFO m_osv;
		BOOL mb_IsUacAdmin;
		HCURSOR mh_CursorWait, mh_CursorArrow, mh_CursorAppStarting, mh_CursorMove;
		HCURSOR mh_SplitV, mh_SplitH;
		HCURSOR mh_DragCursor;
		CDragDrop *mp_DragDrop;
		bool mb_SkipOnFocus;
		bool mb_LastConEmuFocusState;
		DWORD mn_SysMenuOpenTick, mn_SysMenuCloseTick;
	protected:
		friend class CVConGroup;

		friend class CGuiServer;
		CGuiServer m_GuiServer;

		//CProgressBars *ProgressBars;
		HMENU mh_DebugPopup, mh_EditPopup, mh_ActiveVConPopup, mh_TerminateVConPopup, mh_VConListPopup, mh_HelpPopup; // Popup's для SystemMenu
		HMENU mh_InsideSysMenu;
		TCHAR Title[MAX_TITLE_SIZE+192]; //, TitleCmp[MAX_TITLE_SIZE+192]; //, MultiTitle[MAX_TITLE_SIZE+30];
		TCHAR TitleTemplate[128];
		short mn_Progress;
		//LPTSTR GetTitleStart();
		BOOL mb_InTimer;
		BOOL mb_ProcessCreated, mb_WorkspaceErasedOnClose; //DWORD mn_StartTick;
		bool mb_CloseGuiConfirmed;
		#ifndef _WIN64
		HWINEVENTHOOK mh_WinHook; //, mh_PopupHook;
		static VOID CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD anEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
		#endif
		//CVirtualConsole *mp_VCon[MAX_CONSOLE_COUNT];
		//CVirtualConsole *mp_VActive, *mp_VCon1, *mp_VCon2;
		CAttachDlg *mp_AttachDlg;
		CRecreateDlg *mp_RecreateDlg;
		bool mb_SkipSyncSize, mb_PassSysCommand;
		BOOL mb_WaitCursor;
		//BOOL mb_InTrackSysMenu; -> mn_TrackMenuPlace
		TrackMenuPlace mn_TrackMenuPlace;
		BOOL mb_LastRgnWasNull;
		BOOL mb_LockWindowRgn;
		enum {
			fsf_Hide = 0,     // Рамка и заголовок спрятаны
			fsf_WaitShow = 1, // Запущен таймер показа рамки
			fsf_Show = 2,     // Рамка показана
		} m_ForceShowFrame;
		void StartForceShowFrame();
		void StopForceShowFrame();
		//wchar_t *mpsz_RecreateCmd;
		//ITaskbarList3 *mp_TaskBar3;
		//ITaskbarList2 *mp_TaskBar2;
		typedef BOOL (WINAPI* FRegisterShellHookWindow)(HWND);
		RECT mrc_Ideal;
		BOOL mn_InResize;
		RECT mrc_StoredNormalRect;
		void StoreNormalRect(RECT* prcWnd);
		BOOL mb_MaximizedHideCaption; // в режиме HideCaption
		BOOL mb_InRestore; // во время восстановления из Maximized
		BOOL mb_MouseCaptured;
		//BYTE m_KeybStates[256];
		DWORD_PTR m_ActiveKeybLayout;
		struct
		{
			BOOL      bUsed;
			//wchar_t   klName[KL_NAMELENGTH/*==9*/];
			DWORD     klName;
			DWORD_PTR hkl;
		} m_LayoutNames[20];
		struct
		{
			wchar_t szTranslatedChars[16];
		} m_TranslatedChars[256];
		BYTE mn_LastPressedVK;
		bool mb_InImeComposition, mb_ImeMethodChanged;
		wchar_t ms_ConEmuAliveEvent[MAX_PATH+64];
		HANDLE mh_ConEmuAliveEvent; BOOL mb_ConEmuAliveOwned, mb_AliveInitialized;
		//
		BOOL mb_HotKeyRegistered;
		HHOOK mh_LLKeyHook;
		HMODULE mh_LLKeyHookDll;
		HWND* mph_HookedGhostWnd;
		HMODULE LoadConEmuCD();
		void RegisterHotKeys();
		void UnRegisterHotKeys(BOOL abFinal=FALSE);
		//int mn_MinRestoreRegistered; UINT mn_MinRestore_VK, mn_MinRestore_MOD;
		//HMODULE mh_DwmApi;
		//FDwmIsCompositionEnabled DwmIsCompositionEnabled;
		HBITMAP mh_RightClickingBmp; HDC mh_RightClickingDC;
		POINT m_RightClickingSize; // {384 x 16} 24 фрейма, считаем, что четверть отведенного времени прошла до начала показа
		int m_RightClickingFrames, m_RightClickingCurrent;
		BOOL mb_RightClickingPaint, mb_RightClickingLSent, mb_RightClickingRegistered;
		void StartRightClickingPaint();
		void StopRightClickingPaint();
		static LRESULT CALLBACK RightClickingProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
		HWND mh_RightClickingWnd;
		bool PatchMouseEvent(UINT messg, POINT& ptCurClient, POINT& ptCurScreen, WPARAM wParam, bool& isPrivate);
		void CheckTopMostState();
	public:
		wchar_t* LoadConsoleBatch(LPCWSTR asSource, wchar_t** ppszStartupDir = NULL);
	private:
		wchar_t* LoadConsoleBatch_File(LPCWSTR asSource);
		wchar_t* LoadConsoleBatch_Drops(LPCWSTR asSource);
		wchar_t* LoadConsoleBatch_Task(LPCWSTR asSource, wchar_t** ppszStartupDir = NULL);
	public:
		void RightClickingPaint(HDC hdcIntVCon, CVirtualConsole* apVCon);
		void RegisterMinRestore(bool abRegister);
		void RegisterHoooks();
		void UnRegisterHoooks(BOOL abFinal=FALSE);
		void OnWmHotkey(WPARAM wParam);
		void UpdateWinHookSettings();
		void CtrlWinAltSpace();
	protected:
		friend class CConEmuCtrl;
		//BOOL LowLevelKeyHook(UINT nMsg, UINT nVkKeyCode);
		//DWORD_PTR mn_CurrentKeybLayout;
		// Registered messages
		DWORD mn_MainThreadId;
		UINT mn_MsgPostCreate;
		UINT mn_MsgPostCopy;
		UINT mn_MsgMyDestroy;
		UINT mn_MsgUpdateSizes;
		UINT mn_MsgUpdateCursorInfo;
		UINT mn_MsgSetWindowMode;
		UINT mn_MsgUpdateTitle;
		//UINT mn_MsgAttach;
		UINT mn_MsgSrvStarted;
		UINT mn_MsgVConTerminated;
		UINT mn_MsgUpdateScrollInfo;
		UINT mn_MsgUpdateTabs; // = RegisterWindowMessage(CONEMUMSG_UPDATETABS);
		UINT mn_MsgOldCmdVer; BOOL mb_InShowOldCmdVersion;
		UINT mn_MsgTabCommand;
		UINT mn_MsgTabSwitchFromHook; /*BOOL mb_InWinTabSwitch;*/ // = RegisterWindowMessage(CONEMUMSG_SWITCHCON);
		//WARNING!!! mb_InWinTabSwitch - перенести в Keys!
		UINT mn_MsgWinKeyFromHook;
		//UINT mn_MsgConsoleHookedKey;
		UINT mn_MsgSheelHook;
		UINT mn_ShellExecuteEx;
		UINT mn_PostConsoleResize;
		UINT mn_ConsoleLangChanged;
		UINT mn_MsgPostOnBufferHeight;
		UINT mn_MsgPostAltF9;
		//UINT mn_MsgPostSetBackground;
		UINT mn_MsgInitInactiveDC;
		//UINT mn_MsgSetForeground;
		UINT mn_MsgFlashWindow; // = RegisterWindowMessage(CONEMUMSG_FLASHWINDOW);
		//UINT mn_MsgActivateCon; // = RegisterWindowMessage(CONEMUMSG_ACTIVATECON);
		UINT mn_MsgUpdateProcDisplay;
		//UINT wmInputLangChange;
		UINT mn_MsgAutoSizeFont;
		UINT mn_MsgDisplayRConError;
		UINT mn_MsgMacroFontSetName;
		UINT mn_MsgCreateViewWindow;
		UINT mn_MsgPostTaskbarActivate; BOOL mb_PostTaskbarActivate;
		UINT mn_MsgInitVConGhost;
		UINT mn_MsgCreateCon;
		UINT mn_MsgRequestUpdate;
		UINT mn_MsgTaskBarCreated;
		UINT mn_MsgPanelViewMapCoord;

		//
		virtual void OnUseGlass(bool abEnableGlass);
		virtual void OnUseTheming(bool abEnableTheming);
		virtual void OnUseDwm(bool abEnableDwm);

		//
		void InitCommCtrls();
		bool mb_CommCtrlsInitialized;
		HWND mh_AboutDlg;
		static INT_PTR CALLBACK aboutProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);

	public:
		DWORD CheckProcesses();
		DWORD GetFarPID(BOOL abPluginRequired=FALSE);

	public:
		LPCWSTR GetDefaultTitle(); // вернуть ms_ConEmuVer
		LPCTSTR GetLastTitle(bool abUseDefault=true);
		LPCTSTR GetVConTitle(int nIdx);
		void SetTitleTemplate(LPCWSTR asTemplate);
		int GetActiveVCon(CVConGuard* pVCon = NULL, int* pAllCount = NULL);
		CVirtualConsole* GetVCon(int nIdx, bool bFromCycle = false);
		int isVConValid(CVirtualConsole* apVCon);
		CVirtualConsole* GetVConFromPoint(POINT ptScreen);
		void UpdateCursorInfo(const CONSOLE_SCREEN_BUFFER_INFO* psbi, COORD crCursor, CONSOLE_CURSOR_INFO cInfo);
		void UpdateProcessDisplay(BOOL abForce);
		void UpdateSizes();

	public:
		CConEmuMain();
		virtual ~CConEmuMain();

	public:
		//CVirtualConsole* ActiveCon();
		BOOL Activate(CVirtualConsole* apVCon);
		int ActiveConNum(); // 0-based
		int GetConCount(); // количество открытых консолей
		static void AddMargins(RECT& rc, RECT& rcAddShift, BOOL abExpand=FALSE);
		void AskChangeBufferHeight();
		void AskChangeAlternative();
		BOOL AttachRequested(HWND ahConWnd, const CESERVER_REQ_STARTSTOP* pStartStop, CESERVER_REQ_STARTSTOPRET* pRet);
		CRealConsole* AttachRequestedGui(LPCWSTR asAppFileName, DWORD anAppPID);
		void AutoSizeFont(const RECT &rFrom, enum ConEmuRect tFrom);
		RECT CalcMargins(DWORD/*enum ConEmuMargins*/ mg /*, CVirtualConsole* apVCon=NULL*/);
		RECT CalcRect(enum ConEmuRect tWhat, CVirtualConsole* pVCon=NULL);
		RECT CalcRect(enum ConEmuRect tWhat, const RECT &rFrom, enum ConEmuRect tFrom, CVirtualConsole* pVCon=NULL, enum ConEmuMargins tTabAction=CEM_TAB);
		POINT CalcTabMenuPos(CVirtualConsole* apVCon);
		void CheckFocus(LPCWSTR asFrom);
		bool CheckRequiredFiles();
		void CheckUpdates(BOOL abShowMessages);
		enum DragPanelBorder CheckPanelDrag(COORD crCon);
		bool ConActivate(int nCon);
		bool ConActivateNext(BOOL abNext);
		//bool CorrectWindowPos(WINDOWPOS *wp);
		//void CheckGuiBarsCreated();
		CVirtualConsole* CreateCon(RConStartArgs *args, BOOL abAllowScripts = FALSE);
		CVirtualConsole* CreateConGroup(LPCWSTR apszScript, BOOL abForceAsAdmin = FALSE, LPCWSTR asStartupDir = NULL);
		void CreateGhostVCon(CVirtualConsole* apVCon);
		BOOL CreateMainWindow();
		BOOL CreateWorkWindow();
		HRGN CreateWindowRgn(bool abTestOnly=false);
		HRGN CreateWindowRgn(bool abTestOnly,bool abRoundTitle,int anX, int anY, int anWndWidth, int anWndHeight);
		void Destroy();
		void DebugStep(LPCWSTR asMsg, BOOL abErrorSeverity=FALSE);
		void ForceShowTabs(BOOL abShow);
		DWORD_PTR GetActiveKeyboardLayout();
		RECT GetDefaultRect();
		RECT GetGuiClientRect();
		RECT GetIdealRect() { return mrc_Ideal; };
		HMENU GetSysMenu(BOOL abInitial = FALSE);
	protected:
		void UpdateSysMenu(HMENU hSysMenu);
	public:
		RECT GetVirtualScreenRect(BOOL abFullScreen);
		DWORD GetWindowStyle();
		DWORD FixWindowStyle(DWORD dwExStyle, ConEmuWindowMode wmNewMode = wmCurrent);
		DWORD GetWindowStyleEx();
		DWORD GetWorkWindowStyle();
		DWORD GetWorkWindowStyleEx();
		LRESULT GuiShellExecuteEx(SHELLEXECUTEINFO* lpShellExecute, BOOL abAllowAsync, CVirtualConsole* apVCon);
		BOOL Init();
		void InitInactiveDC(CVirtualConsole* apVCon);
		void Invalidate(CVirtualConsole* apVCon);
		void InvalidateAll();
		void UpdateWindowChild(CVirtualConsole* apVCon);
		bool isActive(CVirtualConsole* apVCon, bool abAllowGroup = true);
		bool isChildWindow();
		bool isCloseConfirmed();
		bool isConSelectMode();
		bool isConsolePID(DWORD nPID);
		bool isDragging();
		bool isEditor();
		bool isFar(bool abPluginRequired=false);
		bool isFarExist(CEFarWindowType anWindowType=fwt_Any, LPWSTR asName=NULL, CVConGuard* rpVCon=NULL);
		bool isFilePanel(bool abPluginAllowed=false);
		bool isFirstInstance();
		bool isFullScreen();
		//bool IsGlass();		
		bool isIconic();		
		bool isInImeComposition();		
		bool isLBDown();
		bool isMainThread();
		bool isMeForeground(bool abRealAlso=false, bool abDialogsAlso=true);
		bool isMouseOverFrame(bool abReal=false);		
		bool isNtvdm(BOOL abCheckAllConsoles=FALSE);		
		bool isOurConsoleWindow(HWND hCon);		
		bool isPictureView();		
		bool isProcessCreated();		
		bool isRightClickingPaint();		
		bool isSizing();
		bool isValid(CRealConsole* apRCon);
		bool isValid(CVirtualConsole* apVCon);
		bool isVConExists(int nIdx);
		bool isVConHWND(HWND hChild, CVConGuard* pVCon = NULL);
		bool isViewer();
		bool isVisible(CVirtualConsole* apVCon);
		bool isWindowNormal();
		bool isZoomed();
		void LoadIcons();
		//bool LoadVersionInfo(wchar_t* pFullPath);
		RECT MapRect(RECT rFrom, BOOL bFrame2Client);
		void MoveActiveTab(CVirtualConsole* apVCon, bool bLeftward);
		//void PaintCon(HDC hPaintDC);
		void InvalidateGaps();
		void PaintGaps(HDC hDC);
		void PostAutoSizeFont(int nRelative/*0/1*/, int nValue/*для nRelative==0 - высота, для ==1 - +-1, +-2,...*/);
		void PostCopy(wchar_t* apszMacro, BOOL abRecieved=FALSE);
		void PostCreate(BOOL abRecieved=FALSE);
		void PostCreateCon(RConStartArgs *pArgs);
		HWND PostCreateView(CConEmuChild* pChild);
		void PostMacro(LPCWSTR asMacro);
		void PostMacroFontSetName(wchar_t* pszFontName, WORD anHeight /*= 0*/, WORD anWidth /*= 0*/, BOOL abPosted);
		void PostDisplayRConError(CRealConsole* apRCon, wchar_t* pszErrMsg);
		//void PostSetBackground(CVirtualConsole* apVCon, CESERVER_REQ_SETBACKGROUND* apImgData);
		bool PtDiffTest(POINT C, int aX, int aY, UINT D); //(((abs(C.x-LOWORD(lParam)))<D) && ((abs(C.y-HIWORD(lParam)))<D))
		void RecreateAction(RecreateActionParm aRecreate, BOOL abConfirm, BOOL abRunAs = FALSE);
		int RecreateDlg(RConStartArgs* apArg);
		void RePaint();
		bool ReportUpdateConfirmation();
		void ReportUpdateError();
		void RequestExitUpdate();
		void ReSize(BOOL abCorrect2Ideal = FALSE);
		//void ResizeChildren();
		BOOL RunSingleInstance(HWND hConEmuWnd = NULL, LPCWSTR apszCmd = NULL);
		bool ScreenToVCon(LPPOINT pt, CVirtualConsole** ppVCon);
		//void SetAllConsoleWindowsSize(const COORD& size, /*bool updateInfo,*/ bool bSetRedraw = false);
		void SetDragCursor(HCURSOR hCur);
		void SetSkipOnFocus(bool abSkipOnFocus);
		void SetWaitCursor(BOOL abWait);
		bool SetWindowMode(ConEmuWindowMode inMode, BOOL abForce = FALSE, BOOL abFirstShow = FALSE);
		void ShowMenuHint(HMENU hMenu, WORD nID, WORD nFlags);
		void ShowKeyBarHint(HMENU hMenu, WORD nID, WORD nFlags);
		BOOL ShowWindow(int anCmdShow);
		void ReportOldCmdVersion(DWORD nCmd, DWORD nVersion, int bFromServer, DWORD nFromProcess, u64 hFromModule, DWORD nBits);
		virtual void ShowSysmenu(int x=-32000, int y=-32000, bool bAlignUp = false);
		bool SetParent(HWND hNewParent);
		HMENU CreateDebugMenuPopup();
		HMENU CreateEditMenuPopup(CVirtualConsole* apVCon, HMENU ahExist = NULL);
		HMENU CreateHelpMenuPopup();
		HMENU CreateVConListPopupMenu(HMENU ahExist, BOOL abFirstTabOnly);
		HMENU CreateVConPopupMenu(CVirtualConsole* apVCon, HMENU ahExist, BOOL abAddNew, HMENU& hTerminate);
		void setFocus();
		void StartDebugLogConsole();
		void StartDebugActiveProcess();
		void MemoryDumpActiveProcess();
		//void StartLogCreateProcess();
		//void StopLogCreateProcess();
		//void UpdateLogCreateProcess();
		//wchar_t ms_LogCreateProcess[MAX_PATH]; bool mb_CreateProcessLogged;
		void SyncConsoleToWindow(LPRECT prcNewWnd=NULL);
		void SyncNtvdm();
		void SyncWindowToConsole();
		void SwitchKeyboardLayout(DWORD_PTR dwNewKeybLayout);
		//void TabCommand(UINT nTabCmd);
		BOOL TrackMouse();
		int trackPopupMenu(TrackMenuPlace place, HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, RECT *prcRect);
		void Update(bool isForce = false);
		void UpdateActiveGhost(CVirtualConsole* apVCon);
		void UpdateFarSettings();
		void UpdateIdealRect(BOOL abAllowUseConSize=FALSE);
	protected:
		void UpdateIdealRect(RECT rcNewIdeal);
	public:
		void UpdateTextColorSettings(BOOL ChangeTextAttr = TRUE, BOOL ChangePopupAttr = TRUE);
		void UpdateTitle(/*LPCTSTR asNewTitle*/);
		void UpdateProgress(/*BOOL abUpdateTitle*/);
		void UpdateWindowRgn(int anX=-1, int anY=-1, int anWndWidth=-1, int anWndHeight=-1);
		static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK WorkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		BOOL isDialogMessage(MSG &Msg);
		LPCWSTR MenuAccel(int DescrID, LPCWSTR asText);
		LRESULT WndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
	public:
		void OnAltEnter();
		void OnAltF9(BOOL abPosted=FALSE);
		void OnMinimizeRestore(SingleInstanceShowHideType ShowHideType = sih_None);
		void OnForcedFullScreen(bool bSet = true);
		void OnAlwaysOnTop();
		void OnAlwaysShowScrollbar(bool abSync = true);
		void OnBufferHeight();
		bool DoClose();
		BOOL OnCloseQuery();
		//BOOL mb_InConsoleResize;
		void OnConsoleKey(WORD vk, LPARAM Mods);
		void OnConsoleResize(BOOL abPosted=FALSE);
		void OnCopyingState();
		LRESULT OnCreate(HWND hWnd, LPCREATESTRUCT lpCreate);
		void OnDesktopMode();
		LRESULT OnDestroy(HWND hWnd);
		LRESULT OnFlashWindow(DWORD nFlags, DWORD nCount, HWND hCon);
		LRESULT OnFocus(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, LPCWSTR asMsgFrom = NULL);
		LRESULT OnGetMinMaxInfo(LPMINMAXINFO pInfo);
		void OnHideCaption(ConEmuWindowMode wmNewMode = wmCurrent);
		void OnInfo_About(LPCWSTR asPageName = NULL);
		void OnInfo_Help();
		void OnInfo_HomePage();
		void OnInfo_ReportBug();
		LRESULT OnInitMenuPopup(HWND hWnd, HMENU hMenu, LPARAM lParam);
		LRESULT OnKeyboard(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
		LRESULT OnKeyboardHook(DWORD VkMod);
		LRESULT OnKeyboardIme(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
		LRESULT OnLangChange(UINT messg, WPARAM wParam, LPARAM lParam);
		LRESULT OnLangChangeConsole(CVirtualConsole *apVCon, DWORD dwLayoutName);
		LRESULT OnMouse(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
		LRESULT OnMouse_Move(CVirtualConsole* pVCon, HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr);
		LRESULT OnMouse_LBtnDown(CVirtualConsole* pVCon, HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr);
		LRESULT OnMouse_LBtnUp(CVirtualConsole* pVCon, HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr);
		LRESULT OnMouse_LBtnDblClk(CVirtualConsole* pVCon, HWND hWnd, UINT& messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr);
		LRESULT OnMouse_RBtnDown(CVirtualConsole* pVCon, HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr);
		LRESULT OnMouse_RBtnUp(CVirtualConsole* pVCon, HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr);
		LRESULT OnMouse_RBtnDblClk(CVirtualConsole* pVCon, HWND hWnd, UINT& messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr);
		BOOL OnMouse_NCBtnDblClk(HWND hWnd, UINT& messg, WPARAM wParam, LPARAM lParam);
		//LRESULT OnNcMessage(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
		//LRESULT OnNcPaint(HRGN hRgn);
		//LRESULT OnPaint(WPARAM wParam, LPARAM lParam);
		virtual void OnPaintClient(HDC hdc, int width, int height);
		LRESULT OnSetCursor(WPARAM wParam=-1, LPARAM lParam=-1);
		LRESULT OnSize(bool bResizeRCon=true, WPARAM wParam=0, WORD newClientWidth=(WORD)-1, WORD newClientHeight=(WORD)-1);
		LRESULT OnSizing(WPARAM wParam, LPARAM lParam);
		LRESULT OnMoving(LPRECT prcWnd = NULL, bool bWmMove = false);
		virtual LRESULT OnWindowPosChanged(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		LRESULT OnWindowPosChanging(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		void OnSizePanels(COORD cr);
		LRESULT OnShellHook(WPARAM wParam, LPARAM lParam);
		LRESULT OnSysCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
		LRESULT OnTimer(WPARAM wParam, LPARAM lParam);
		void OnTransparent(bool abFromFocus = false, bool bSetFocus = true);
		void OnVConCreated(CVirtualConsole* apVCon, const RConStartArgs *args);
		LRESULT OnVConClosed(CVirtualConsole* apVCon, BOOL abPosted = FALSE);
		void OnAllVConClosed();
		void OnAllGhostClosed();
		void OnGhostCreated(CVirtualConsole* apVCon, HWND ahGhost);
		void OnRConStartedSuccess(CRealConsole* apRCon);
		LRESULT OnUpdateScrollInfo(BOOL abPosted = FALSE);
		void OnPanelViewSettingsChanged(BOOL abSendChanges=TRUE);
		void OnGlobalSettingsChanged();
		void OnTaskbarSettingsChanged();
		#ifdef __GNUC__
		AlphaBlend_t GdiAlphaBlend;
		#endif
		void OnActiveConWndStore(HWND hConWnd);

		// return true - when state was changes
		bool SetTransparent(HWND ahWnd, UINT anAlpha/*0..255*/, bool abColorKey = false, COLORREF acrColorKey = 0, bool abForceLayered = false);
		GetLayeredWindowAttributes_t _GetLayeredWindowAttributes;
		#ifdef __GNUC__
		SetLayeredWindowAttributes_t SetLayeredWindowAttributes;
		#endif
};

extern CConEmuMain *gpConEmu;
