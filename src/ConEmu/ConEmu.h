
/*
Copyright (c) 2009-2013 Maximus5
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

#if !defined(IID_IShellLink)
#define IID_IShellLink IID_IShellLinkW
#endif

#define GET_X_LPARAM(inPx) ((int)(short)LOWORD(inPx))
#define GET_Y_LPARAM(inPy) ((int)(short)HIWORD(inPy))

//#define RCLICKAPPSTIMEOUT 600
//#define RCLICKAPPS_START 100 // начало отрисовки кружка вокруг курсора
//#define RCLICKAPPSTIMEOUT_MAX 10000
//#define RCLICKAPPSDELTA 3
//#define DRAG_DELTA 5

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
class CDefaultTerminal;
class CConEmuMenu;
class CConEmuInside;
class CRunQueue;
struct CEFindDlg;
union CESize;


struct MsgSrvStartedArg
{
	HWND  hConWnd;
	DWORD nSrcPID;
	DWORD dwKeybLayout;
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
#include "../common/MArray.h"
#include "../common/MMap.h"

// IME support (WinXP or later)
typedef BOOL (WINAPI* ImmSetCompositionFontW_t)(HIMC hIMC, LPLOGFONT lplf);
typedef BOOL (WINAPI* ImmSetCompositionWindow_t)(HIMC hIMC, LPCOMPOSITIONFORM lpCompForm);
typedef HIMC (WINAPI* ImmGetContext_t)(HWND hWnd);


//struct GuiShellExecuteExArg
//{
//	CVirtualConsole* pVCon;
//	SHELLEXECUTEINFO* lpShellExecute;
//	HANDLE hReadyEvent;
//	BOOL bInProcess;
//	BOOL bResult;
//	DWORD dwErrCode;
//};


typedef DWORD ConEmuInstallMode;
const ConEmuInstallMode
	cm_Normal       = 0x0000,
	cm_MinGW        = 0x0001, // ConEmu установлен как пакет MinGW
	cm_PortableApps = 0x0002, // ConEmu установлен как пакет PortableApps.com
	cm_MSysStartup  = 0x1000  // найден баш из msys: "%ConEmuDir%\..\msys\1.0\bin\sh.exe" (MinGW mode)
	;


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
		wchar_t ms_ConEmuBuild[16];             // номер сборки, например "110117" или "131129dbg"
		wchar_t ms_ConEmuExe[MAX_PATH+1];       // полный путь к ConEmu.exe (GUI)
		wchar_t ms_ConEmuExeDir[MAX_PATH+1];    // БЕЗ завершающего слеша. Папка содержит ConEmu.exe
		wchar_t ms_ConEmuBaseDir[MAX_PATH+1];   // БЕЗ завершающего слеша. Папка содержит ConEmuC.exe, ConEmuHk.dll, ConEmu.xml
		wchar_t ms_ConEmuWorkDir[MAX_PATH+1];    // БЕЗ завершающего слеша. Папка запуска ConEmu.exe (GetCurrentDirectory)
		void StoreWorkDir(LPCWSTR asNewCurDir = NULL);
		LPCWSTR WorkDir(LPCWSTR asOverrideCurDir = NULL);
		wchar_t ms_ComSpecInitial[MAX_PATH];
		wchar_t *mps_IconPath;
		void SetWindowIcon(LPCWSTR asNewIcon);
		BOOL mb_DosBoxExists;
		//BOOL mb_MingwMode;   // ConEmu установлен как пакет MinGW
		//BOOL mb_MSysStartup; // найден баш из msys: "%ConEmuDir%\..\msys\1.0\bin\sh.exe" (MinGW mode)
		ConEmuInstallMode m_InstallMode;
		bool isMingwMode();
		bool isMSysStartup();
		bool isUpdateAllowed();
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
		bool mb_FindBugMode;
	private:
		struct
		{
			DWORD nLastCrashReported;
			bool  bBlockChildrenDebuggers;
		} m_DbgInfo;
	private:
		bool CheckBaseDir();
		BOOL CheckDosBoxExists();
		void CheckPortableReg();
		void FinalizePortableReg();
		wchar_t ms_ConEmuXml[MAX_PATH+1];       // полный путь к портабельным настройкам
		wchar_t ms_ConEmuIni[MAX_PATH+1];       // полный путь к портабельным настройкам
	public:
		bool SetConfigFile(LPCWSTR asFilePath, bool abWriteReq = false);
		LPWSTR ConEmuXml();
		LPWSTR ConEmuIni();
		wchar_t ms_ConEmuChm[MAX_PATH+1];       // полный путь к chm-файлу (help)
		wchar_t ms_ConEmuC32Full[MAX_PATH+12];  // полный путь к серверу (ConEmuC.exe) с длинными именами
		wchar_t ms_ConEmuC64Full[MAX_PATH+12];  // полный путь к серверу (ConEmuC64.exe) с длинными именами
		LPCWSTR ConEmuCExeFull(LPCWSTR asCmdLine=NULL);
		wchar_t *mpsz_ConEmuArgs;    // Аргументы
		void GetComSpecCopy(ConEmuComspec& ComSpec);
		void CreateGuiAttachMapping(DWORD nGuiAppPID);
		void InitComSpecStr(ConEmuComspec& ComSpec);
		bool IsResetBasicSettings();
		bool IsFastSetupDisabled();
		bool IsAllowSaveSettingsOnExit();
	private:
		ConEmuGuiMapping m_GuiInfo;
		MFileMapping<ConEmuGuiMapping> m_GuiInfoMapping;
		MFileMapping<ConEmuGuiMapping> m_GuiAttachMapping;
		void FillConEmuMainFont(ConEmuMainFont* pFont);
		void UpdateGuiInfoMapping();
		void UpdateGuiInfoMappingActive(bool bActive);
		int mn_QuakePercent; // 0 - отключен, иначе (>0 && <=100) - идет анимация Quake
		bool mb_InCreateWindow;
		HMONITOR mh_MinFromMonitor;
		bool mb_InShowMinimized; // true на время выполнения ShowWindow(SW_SHOWMIN...)
		bool mb_LastTransparentFocused; // нужно для проверки gpSet->isTransparentSeparate
	public:
		bool InCreateWindow();
		bool InQuakeAnimation();
		UINT IsQuakeVisible();
		bool InMinimizing();
	protected:
		struct WindowsOverQuake
		{
			RECT rcWnd;
			HRGN hRgn;
			int  iRc;
		};
		static BOOL CALLBACK EnumWindowsOverQuake(HWND hWnd, LPARAM lpData);
	public:
		//CConEmuChild *m_Child;
		//CConEmuBack  *m_Back;
		//CConEmuMacro *m_Macro;
		CConEmuMenu *mp_Menu;
		TabBarClass *mp_TabBar;
		CConEmuInside *mp_Inside;
		CStatus *mp_Status;
		CToolTip *mp_Tip;
		MFileLog *mp_Log; CRITICAL_SECTION mcs_Log; // mcs_Log - для создания
		CDefaultTerminal *mp_DefTrm;
		CEFindDlg *mp_Find;
		CRunQueue *mp_RunQueue;

		bool CreateLog();
		void LogString(LPCWSTR asInfo, bool abWriteTime = true, bool abWriteLine = true);
		void LogString(LPCSTR asInfo, bool abWriteTime = true, bool abWriteLine = true);
		void LogMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		void LogWindowPos(LPCWSTR asPrefix);
		//POINT cwShift; // difference between window size and client area size for main ConEmu window
		POINT ptFullScreenSize; // size for GetMinMaxInfo in Fullscreen mode

	private:
		bool mb_InSetQuakeMode;
	public:
		ConEmuWindowMode WindowMode;           // wmNormal/wmMaximized/wmFullScreen
		ConEmuWindowMode changeFromWindowMode; // wmNotChanging/rmNormal/rmMaximized/rmFullScreen
		bool isRestoreFromMinimized;
		bool isWndNotFSMaximized; // ставится в true, если при переходе в FullScreen - был Maximized
		bool isQuakeMinimized;    // изврат, для случая когда "Quake" всегда показывается на таскбаре
		HMONITOR GetNearestMonitor(MONITORINFO* pmi = NULL, LPCRECT prcWnd = NULL);
		HMONITOR GetPrimaryMonitor(MONITORINFO* pmi = NULL);
		void StorePreMinimizeMonitor();

		CESize WndWidth, WndHeight;  // в символах/пикселях/процентах
		int    wndX, wndY;           // в пикселях

		bool  WindowStartMinimized; // ключик "/min" или "Свернуть" в свойствах ярлыка
		bool  WindowStartTSA;       // ключик "/mintsa"
		bool  ForceMinimizeToTray;  // ключики "/tsa" или "/tray"
		bool  DisableAutoUpdate;    // ключик "/noupdate"
		bool  DisableKeybHooks;     // ключик "/nokeyhook"
		bool  DisableAllMacro;      // ключик "/nomacro"
		bool  DisableAllHotkeys;    // ключик "/nohotkey"
		bool  DisableSetDefTerm;    // ключик "/nodeftrm"
		bool  DisableRegisterFonts; // ключик "/noregfont"

		BOOL  mb_ExternalHidden;
		
		BOOL  mb_StartDetached;

		enum StartupStage {
			ss_Starting,
			ss_PostCreate1Called,
			ss_PostCreate1Finished,
			ss_PostCreate2Called,
			ss_PostCreate2Finished,
			ss_Started = ss_PostCreate2Finished
		} mn_StartupFinished;

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

			// Для обработки gpSet->isActivateSplitMouseOver
			POINT  ptLastSplitOverCheck;

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
				_ASSERTE(nWheelScrollChars<=3);
				return nWheelScrollChars;
			};
			UINT GetWheelScrollLines()
			{
				if (!nWheelScrollLines)
					ReloadWheelScroll();
				return nWheelScrollLines;
			};
		} mouse;
		struct SessionInfo
		{
			HMODULE hWtsApi;
			WPARAM  wState;     // session state change event
			LPARAM  lSessionID; // session ID

			bool Connected()
			{
				return (wState!=7/*WTS_SESSION_LOCK*/);
			}

			LRESULT SessionChanged(WPARAM State, LPARAM SessionID)
			{
				wState = State;
				lSessionID = SessionID;
				return 0;
			}

			void SetSessionNotification(bool bSwitch)
			{
				if (((hWtsApi!=NULL) == bSwitch) || !IsWindowsXP)
					return;
				if (bSwitch)
				{
					typedef BOOL (WINAPI* WTSRegisterSessionNotification_t)(HWND hWnd, DWORD dwFlags);

					wState = (WPARAM)-1;
					lSessionID = (LPARAM)-1;

					hWtsApi = LoadLibrary(L"Wtsapi32.dll");
					WTSRegisterSessionNotification_t pfn = hWtsApi ? (WTSRegisterSessionNotification_t)GetProcAddress(hWtsApi, "WTSRegisterSessionNotification") : NULL;
					if (!pfn || !pfn(ghWnd, 0/*NOTIFY_FOR_THIS_SESSION*/))
					{
						if (hWtsApi)
						{
							FreeLibrary(hWtsApi);
							hWtsApi = NULL;
						}
						return;
					}
				}
				else if (hWtsApi)
				{
					typedef BOOL (WINAPI* WTSUnRegisterSessionNotification_t)(HWND hWnd);

					WTSUnRegisterSessionNotification_t pfn = (WTSUnRegisterSessionNotification_t)GetProcAddress(hWtsApi, "WTSUnRegisterSessionNotification");
					if (pfn && ghWnd)
					{
						pfn(ghWnd);
					}

					FreeLibrary(hWtsApi);
					hWtsApi = NULL;
				}
			}
		} session;
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
		BOOL mb_IsUacAdmin; // ConEmu itself is started elevated
		bool IsActiveConAdmin();
		HCURSOR mh_CursorWait, mh_CursorArrow, mh_CursorAppStarting, mh_CursorMove, mh_CursorIBeam;
		HCURSOR mh_SplitV, mh_SplitH;
		HCURSOR mh_DragCursor;
		CDragDrop *mp_DragDrop;
		bool mb_SkipOnFocus;
		bool mb_LastConEmuFocusState;
		DWORD mn_ForceTimerCheckLoseFocus; // GetTickCount()
		bool mb_IgnoreQuakeActivation;
		bool mb_AllowAutoChildFocus;
	public:
		void OnOurDialogOpened();
		void OnOurDialogClosed();
		void CheckAllowAutoChildFocus(DWORD nDeactivatedTID);
		bool CanSetChildFocus();
		void SetScClosePending(bool bFlag);
		bool OnScClose();
	protected:
		bool mb_ScClosePending; // Устанавливается в TRUE в CVConGroup::CloseQuery
	protected:
		DWORD mn_LastQuakeShowHide;

		friend class CVConGroup;

		friend class CGuiServer;
		CGuiServer m_GuiServer;

		//CProgressBars *ProgressBars;
		//HMENU mh_DebugPopup, mh_EditPopup, mh_ActiveVConPopup, mh_TerminateVConPopup, mh_VConListPopup, mh_HelpPopup; // Popup's для SystemMenu
		//HMENU mh_InsideSysMenu;
		TCHAR Title[MAX_TITLE_SIZE+192]; //, TitleCmp[MAX_TITLE_SIZE+192]; //, MultiTitle[MAX_TITLE_SIZE+30];
		TCHAR TitleTemplate[128];
		short mn_Progress;
		//LPTSTR GetTitleStart();
		bool mb_InTimer;
		bool mb_ProcessCreated;
		bool mb_WorkspaceErasedOnClose;
		#ifndef _WIN64
		HWINEVENTHOOK mh_WinHook; //, mh_PopupHook;
		static VOID CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD anEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
		#endif
		//CVirtualConsole *mp_VCon[MAX_CONSOLE_COUNT];
		//CVirtualConsole *mp_VActive, *mp_VCon1, *mp_VCon2;
		CAttachDlg *mp_AttachDlg;
		CRecreateDlg *mp_RecreateDlg;
		bool mb_SkipSyncSize;
		BOOL mb_WaitCursor;
		//BOOL mb_InTrackSysMenu; -> mn_TrackMenuPlace
		//TrackMenuPlace mn_TrackMenuPlace;
		BOOL mb_LastRgnWasNull;
		BOOL mb_LockWindowRgn;
		BOOL mb_LockShowWindow;
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
		struct IdealRectInfo
		{
			// Current Ideal rect
			RECT  rcIdeal;
			// TODO: Reserved fields
			RECT  rcClientMargins; // (TabBar + StatusBar) at storing moment
			COORD crConsole;       // Console size in cells at storing moment
			SIZE  csFont;          // VCon Font size (Width, Height) at storing moment
		} mr_Ideal;
	public:
		void StoreIdealRect();
		RECT GetIdealRect();
		void OnTabbarActivated(bool bTabbarVisible);
	protected:
		BOOL mn_InResize;
		//bool mb_InScMinimize;
		RECT mrc_StoredNormalRect;
		void StoreNormalRect(RECT* prcWnd);
		//BOOL mb_MaximizedHideCaption; // в режиме HideCaption
		//BOOL mb_InRestore; // во время восстановления из Maximized
		BOOL mb_MouseCaptured;
		//BYTE m_KeybStates[256];
		void CheckActiveLayoutName();
		void AppendHKL(wchar_t* szInfo, size_t cchInfoMax, HKL* hKeyb, int nCount);
		void AppendRegisteredLayouts(wchar_t* szInfo, size_t cchInfoMax);
		void StoreLayoutName(int iIdx, DWORD dwLayout, HKL hkl);
		DWORD_PTR m_ActiveKeybLayout;
		struct LayoutNames
		{
			BOOL      bUsed;
			//wchar_t   klName[KL_NAMELENGTH/*==9*/];
			DWORD     klName;
			DWORD_PTR hkl;
		} m_LayoutNames[20];
		struct TranslatedCharacters
		{
			wchar_t szTranslatedChars[16];
		} m_TranslatedChars[256];
		BYTE mn_LastPressedVK;
		bool mb_InImeComposition, mb_ImeMethodChanged;
		wchar_t ms_ConEmuAliveEvent[MAX_PATH];
		BOOL mb_AliveInitialized;
		HANDLE mh_ConEmuAliveEvent; bool mb_ConEmuAliveOwned; DWORD mn_ConEmuAliveEventErr;
		HANDLE mh_ConEmuAliveEventNoDir; bool mb_ConEmuAliveOwnedNoDir; DWORD mn_ConEmuAliveEventErrNoDir;
		//
		bool mb_HotKeyRegistered;
		HHOOK mh_LLKeyHook;
		HMODULE mh_LLKeyHookDll;
		HWND* mph_HookedGhostWnd;
		HMODULE LoadConEmuCD();
		void RegisterHotKeys();
		void RegisterGlobalHotKeys(bool bRegister);
	public:
		void GlobalHotKeyChanged();
	protected:
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
		void RegisterHooks();
		void UnRegisterHooks(BOOL abFinal=FALSE);
		void OnWmHotkey(WPARAM wParam);
		void UpdateWinHookSettings();
		void CtrlWinAltSpace();
		void DeleteVConMainThread(CVirtualConsole* apVCon);
		UINT RegisterMessage(LPCSTR asLocal, LPCWSTR asGlobal = NULL);
		UINT GetRegisteredMessage(LPCSTR asLocal);
	protected:
		friend class CConEmuCtrl;
		friend class CRunQueue;
		COLORREF mcr_BackBrush;
		HBRUSH mh_BackBrush;
		DWORD mn_MainThreadId;
		// Registered messages
		UINT mn__FirstAppMsg;
		MMap<UINT,LPCSTR> m__AppMsgs;
		UINT mn_MsgPostCreate;
		UINT mn_MsgPostCopy;
		UINT mn_MsgMyDestroy;
		UINT mn_MsgUpdateSizes;
		UINT mn_MsgUpdateCursorInfo;
		UINT mn_MsgSetWindowMode;
		UINT mn_MsgUpdateTitle;
		//UINT mn_MsgAttach;
		UINT mn_MsgSrvStarted;
		//UINT mn_MsgVConTerminated;
		UINT mn_MsgUpdateScrollInfo;
		UINT mn_MsgUpdateTabs; // = RegisterWindowMessage(CONEMUMSG_UPDATETABS);
		UINT mn_MsgOldCmdVer; BOOL mb_InShowOldCmdVersion;
		UINT mn_MsgTabCommand;
		UINT mn_MsgTabSwitchFromHook; /*BOOL mb_InWinTabSwitch;*/ // = RegisterWindowMessage(CONEMUMSG_SWITCHCON);
		//WARNING!!! mb_InWinTabSwitch - перенести в Keys!
		UINT mn_MsgWinKeyFromHook;
		//UINT mn_MsgConsoleHookedKey;
		UINT mn_MsgSheelHook;
		//UINT mn_ShellExecuteEx;
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
		UINT mn_MsgTaskBarBtnCreated;
		UINT mn_MsgRequestRunProcess;
		UINT mn_MsgDeleteVConMainThread;

		void SetRunQueueTimer(bool bSet, UINT uElapse);

		//
		virtual void OnUseGlass(bool abEnableGlass);
		virtual void OnUseTheming(bool abEnableTheming);
		virtual void OnUseDwm(bool abEnableDwm);

		//
		void InitCommCtrls();
		bool mb_CommCtrlsInitialized;
		HWND mh_AboutDlg;
		static INT_PTR CALLBACK aboutProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);

		//
		//CRITICAL_SECTION mcs_ShellExecuteEx;
		//MArray<GuiShellExecuteExArg*> m_ShellExecuteQueue;
		//void GuiShellExecuteExQueue();
		//bool mb_InShellExecuteQueue;

		bool ExecuteProcessPrepare();
		void ExecuteProcessFinished(bool bOpt);

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
		static void AddMargins(RECT& rc, const RECT& rcAddShift, BOOL abExpand=FALSE);
		void AskChangeBufferHeight();
		void AskChangeAlternative();
		void AttachToDialog();
		BOOL AttachRequested(HWND ahConWnd, const CESERVER_REQ_STARTSTOP* pStartStop, CESERVER_REQ_STARTSTOPRET* pRet);
		CRealConsole* AttachRequestedGui(LPCWSTR asAppFileName, DWORD anAppPID);
		void AutoSizeFont(RECT arFrom, enum ConEmuRect tFrom);
		RECT CalcMargins(DWORD/*enum ConEmuMargins*/ mg, ConEmuWindowMode wmNewMode = wmCurrent);
		RECT CalcRect(enum ConEmuRect tWhat, CVirtualConsole* pVCon=NULL);
		RECT CalcRect(enum ConEmuRect tWhat, const RECT &rFrom, enum ConEmuRect tFrom, CVirtualConsole* pVCon=NULL, enum ConEmuMargins tTabAction=CEM_TAB);
		bool FixWindowRect(RECT& rcWnd, DWORD nBorders /* enum of ConEmuBorders */, bool bPopupDlg = false);
		//POINT CalcTabMenuPos(CVirtualConsole* apVCon);
		void CheckFocus(LPCWSTR asFrom);
		bool CheckRequiredFiles();
		void CheckUpdates(BOOL abShowMessages);
		enum DragPanelBorder CheckPanelDrag(COORD crCon);
		bool ConActivate(int nCon);
		bool ConActivateNext(BOOL abNext);
		//bool CorrectWindowPos(WINDOWPOS *wp);
		//void CheckGuiBarsCreated();
		bool CreateWnd(RConStartArgs *args);
		CVirtualConsole* CreateCon(RConStartArgs *args, bool abAllowScripts = false, bool abForceCurConsole = false);
		CVirtualConsole* CreateConGroup(LPCWSTR apszScript, bool abForceAsAdmin = false, LPCWSTR asStartupDir = NULL, const RConStartArgs *apDefArgs = NULL);
		LPCWSTR ParseScriptLineOptions(LPCWSTR apszLine, bool* rpbAsAdmin, bool* rpbSetActive, size_t cchNameMax=0, wchar_t* rsName/*[MAX_RENAME_TAB_LEN]*/=NULL);
		void CreateGhostVCon(CVirtualConsole* apVCon);
		BOOL CreateMainWindow();
		BOOL CreateWorkWindow();
		HRGN CreateWindowRgn(bool abTestOnly=false);
		HRGN CreateWindowRgn(bool abTestOnly,bool abRoundTitle,int anX, int anY, int anWndWidth, int anWndHeight);
		void DebugStep(LPCWSTR asMsg, BOOL abErrorSeverity=FALSE);
		void ForceShowTabs(BOOL abShow);
		DWORD_PTR GetActiveKeyboardLayout();
		SIZE GetDefaultSize(bool bCells, const CESize* pSizeW=NULL, const CESize* pSizeH=NULL, HMONITOR hMon=NULL);
		RECT GetDefaultRect();
		RECT GetGuiClientRect();
		bool isTabsShown();
		bool SizeWindow(const CESize sizeW, const CESize sizeH);
		//HMENU GetSysMenu(BOOL abInitial = FALSE);

	public:
		void Destroy();
	private:
		#ifdef _DEBUG
		bool mb_DestroySkippedInAssert;
		#endif

	//protected:
	//	void UpdateSysMenu(HMENU hSysMenu);
	public:
		RECT GetVirtualScreenRect(BOOL abFullScreen);
		DWORD GetWindowStyle();
		DWORD FixWindowStyle(DWORD dwExStyle, ConEmuWindowMode wmNewMode = wmCurrent);
		void SetWindowStyle(DWORD anStyle);
		void SetWindowStyle(HWND ahWnd, DWORD anStyle);
		DWORD GetWindowStyleEx();
		void SetWindowStyleEx(DWORD anStyleEx);
		void SetWindowStyleEx(HWND ahWnd, DWORD anStyleEx);
		DWORD GetWorkWindowStyle();
		DWORD GetWorkWindowStyleEx();
		//LRESULT GuiShellExecuteEx(SHELLEXECUTEINFO* lpShellExecute, CVirtualConsole* apVCon);
		BOOL Init();
		void InitInactiveDC(CVirtualConsole* apVCon);
		void Invalidate(LPRECT lpRect, BOOL bErase = TRUE);
		void InvalidateAll();
		void UpdateWindowChild(CVirtualConsole* apVCon);
		void UpdateInsideRect(RECT rcNewPos);
		bool isActive(CVirtualConsole* apVCon, bool abAllowGroup = true);
		//bool isChildWindowVisible();
		bool isCloseConfirmed();
		bool isDestroyOnClose(bool ScCloseOnEmpty = false);
		bool isConSelectMode();
		bool isConsolePID(DWORD nPID);
		bool isDragging();
		bool isFirstInstance(bool bFolderIgnore = false);
		bool isFullScreen();
		//bool IsGlass();		
		bool isIconic(bool abRaw = false);
		bool isInImeComposition();		
		bool isLBDown();
		bool isMainThread();
		bool isMeForeground(bool abRealAlso=false, bool abDialogsAlso=true, HWND* phFore=NULL);
		bool isMouseOverFrame(bool abReal=false);		
		bool isNtvdm(BOOL abCheckAllConsoles=FALSE);		
		bool isOurConsoleWindow(HWND hCon);		
		bool isPictureView();		
		bool isProcessCreated();		
		bool isRightClickingPaint();		
		bool isSizing(UINT nMouseMsg=0);
		void BeginSizing(bool bFromStatusBar);
		void SetSizingFlags(DWORD nSetFlags = MOUSE_SIZING_BEGIN);
		void ResetSizingFlags(DWORD nDropFlags = MOUSE_SIZING_BEGIN|MOUSE_SIZING_TODO);
		void EndSizing(UINT nMouseMsg=0);
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
		//RECT MapRect(RECT rFrom, BOOL bFrame2Client);
		void MoveActiveTab(CVirtualConsole* apVCon, bool bLeftward);
		//void PaintCon(HDC hPaintDC);
		void InvalidateGaps();
		//void PaintGaps(HDC hDC);
		void PostAutoSizeFont(int nRelative/*0/1*/, int nValue/*для nRelative==0 - высота, для ==1 - +-1, +-2,...*/);
		void PostDragCopy(BOOL abMove, BOOL abReceived=FALSE);
		void PostCreate(BOOL abReceived=FALSE);
		void PostCreateCon(RConStartArgs *pArgs);
		HWND PostCreateView(CConEmuChild* pChild);
		void PostMacro(LPCWSTR asMacro);
		void PostMacroFontSetName(wchar_t* pszFontName, WORD anHeight /*= 0*/, WORD anWidth /*= 0*/, BOOL abPosted);
		void PostDisplayRConError(CRealConsole* apRCon, wchar_t* pszErrMsg);
		//void PostSetBackground(CVirtualConsole* apVCon, CESERVER_REQ_SETBACKGROUND* apImgData);
		bool PtDiffTest(POINT C, int aX, int aY, UINT D); //(((abs(C.x-LOWORD(lParam)))<D) && ((abs(C.y-HIWORD(lParam)))<D))
		void RecreateAction(RecreateActionParm aRecreate, BOOL abConfirm, BOOL abRunAs = FALSE);
		int RecreateDlg(RConStartArgs* apArg);
		//void RePaint();
		bool ReportUpdateConfirmation();
		void ReportUpdateError();
		void RequestExitUpdate();
		void RequestPostUpdateTabs();
		void ReSize(BOOL abCorrect2Ideal = FALSE);
		//void ResizeChildren();
		BOOL RunSingleInstance(HWND hConEmuWnd = NULL, LPCWSTR apszCmd = NULL);
		bool ScreenToVCon(LPPOINT pt, CVirtualConsole** ppVCon);
		//void SetAllConsoleWindowsSize(const COORD& size, /*bool updateInfo,*/ bool bSetRedraw = false);
		void SetDragCursor(HCURSOR hCur);
		bool SetSkipOnFocus(bool abSkipOnFocus);
		void SetWaitCursor(BOOL abWait);
		ConEmuWindowMode GetWindowMode();
		bool SetWindowMode(ConEmuWindowMode inMode, BOOL abForce = FALSE, BOOL abFirstShow = FALSE);
		bool SetQuakeMode(BYTE NewQuakeMode, ConEmuWindowMode nNewWindowMode = wmNotChanging, bool bFromDlg = false);
		static LPCWSTR FormatTileMode(ConEmuWindowCommand Tile, wchar_t* pchBuf, size_t cchBufMax);
		bool SetTileMode(ConEmuWindowCommand Tile);
		ConEmuWindowCommand GetTileMode(bool Estimate, MONITORINFO* pmi=NULL);
		bool IsSizeFree(ConEmuWindowMode CheckMode = wmFullScreen);
		bool IsSizePosFree(ConEmuWindowMode CheckMode = wmFullScreen);
		bool IsCantExceedMonitor();
		bool IsPosLocked();
		bool JumpNextMonitor(bool Next);
		bool JumpNextMonitor(HWND hJumpWnd, HMONITOR hJumpMon, bool Next, const RECT rcJumpWnd);
	private:
		struct QuakePrevSize {
			bool bWasSaved;
			bool bWaitReposition; // Требуется смена позиции при OnHideCaption
			CESize wndWidth, wndHeight; // Консоль
			int wndX, wndY; // GUI
			DWORD nFrame; // it's BYTE, DWORD here for alignment
			ConEmuWindowMode WindowMode;
			IdealRectInfo rcIdealInfo;
			// helper methods
			void Save(const CESize& awndWidth, const CESize& awndHeight, const int& awndX, const int& awndY, const BYTE& anFrame, const ConEmuWindowMode& aWindowMode, const IdealRectInfo& arcIdealInfo);
			ConEmuWindowMode Restore(CESize& rwndWidth, CESize& rwndHeight, int& rwndX, int& rwndY, BYTE& rnFrame, IdealRectInfo& rrcIdealInfo);
		} m_QuakePrevSize;
		ConEmuWindowCommand m_TileMode;
		struct {
			RECT rcNewPos;
			bool bInJump;
			bool bFullScreen, bMaximized;
		} m_JumpMonitor;
	public:
		//void ShowMenuHint(HMENU hMenu, WORD nID, WORD nFlags);
		//void ShowKeyBarHint(HMENU hMenu, WORD nID, WORD nFlags);
		BOOL ShowWindow(int anCmdShow, DWORD nAnimationMS = (DWORD)-1);
		void ReportOldCmdVersion(DWORD nCmd, DWORD nVersion, int bFromServer, DWORD nFromProcess, u64 hFromModule, DWORD nBits);
		//virtual void ShowSysmenu(int x=-32000, int y=-32000, bool bAlignUp = false);
		bool SetParent(HWND hNewParent);
		//HMENU CreateDebugMenuPopup();
		//HMENU CreateEditMenuPopup(CVirtualConsole* apVCon, HMENU ahExist = NULL);
		//HMENU CreateHelpMenuPopup();
		//HMENU CreateVConListPopupMenu(HMENU ahExist, BOOL abFirstTabOnly);
		//HMENU CreateVConPopupMenu(CVirtualConsole* apVCon, HMENU ahExist, BOOL abAddNew, HMENU& hTerminate);
		void setFocus();
		bool StartDebugLogConsole();
		bool StartDebugActiveProcess();
		bool MemoryDumpActiveProcess();
		//void StartLogCreateProcess();
		//void StopLogCreateProcess();
		//void UpdateLogCreateProcess();
		//wchar_t ms_LogCreateProcess[MAX_PATH]; bool mb_CreateProcessLogged;
		void SyncNtvdm();
		void SwitchKeyboardLayout(DWORD_PTR dwNewKeybLayout);
		//void TabCommand(UINT nTabCmd);
		BOOL TrackMouse();
		void SetSkipMouseEvent(UINT nMsg1, UINT nMsg2, UINT nReplaceDblClk);
		//int trackPopupMenu(TrackMenuPlace place, HMENU hMenu, UINT uFlags, int x, int y, HWND hWnd, RECT *prcRect = NULL);
		void Update(bool isForce = false);
		void UpdateActiveGhost(CVirtualConsole* apVCon);
		void UpdateFarSettings();
		//void UpdateIdealRect(BOOL abAllowUseConSize=FALSE);
	protected:
		void UpdateIdealRect(RECT rcNewIdeal);
		void UpdateImeComposition();
	public:
		void UpdateTextColorSettings(BOOL ChangeTextAttr = TRUE, BOOL ChangePopupAttr = TRUE);
		void CheckNeedUpdateTitle(LPCWSTR asRConTitle);
		void UpdateTitle(/*LPCTSTR asNewTitle*/);
		void UpdateProgress(/*BOOL abUpdateTitle*/);
		void UpdateWindowRgn(int anX=-1, int anY=-1, int anWndWidth=-1, int anWndHeight=-1);
		static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK WorkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		BOOL isDialogMessage(MSG &Msg);
		//LPCWSTR MenuAccel(int DescrID, LPCWSTR asText);
		BOOL setWindowPos(HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);
		LRESULT WndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
	public:
		void DoFullScreen();
		void DoMaximizeRestore();
		bool DoMaximizeWidthHeight(bool bWidth, bool bHeight);
		void DoMinimizeRestore(SingleInstanceShowHideType ShowHideType = sih_None);
		void DoForcedFullScreen(bool bSet = true);
		void OnSwitchGuiFocus(SwitchGuiFocusOp FocusOp);
		void OnAlwaysOnTop();
		void OnAlwaysShowScrollbar(bool abSync = true);
		void OnBufferHeight();
		//bool DoClose();
		//BOOL mb_InConsoleResize;
		void OnConsoleKey(WORD vk, LPARAM Mods);
		void OnConsoleResize(BOOL abPosted=FALSE);
		void OnCopyingState();
		LRESULT OnCreate(HWND hWnd, LPCREATESTRUCT lpCreate);
		void OnDesktopMode();
		LRESULT OnDestroy(HWND hWnd);
		LRESULT OnFlashWindow(DWORD nFlags, DWORD nCount, HWND hCon);
		LRESULT OnFocus(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, LPCWSTR asMsgFrom = NULL, BOOL abForceChild = FALSE);
		LRESULT OnGetMinMaxInfo(LPMINMAXINFO pInfo);
		void OnHideCaption();
		void OnInfo_About(LPCWSTR asPageName = NULL);
		void OnInfo_Donate();
		void OnInfo_WhatsNew(bool bLocal);
		void OnInfo_Help();
		void OnInfo_HomePage();
		void OnInfo_ReportBug();
		void OnInfo_ReportCrash(LPCWSTR asDumpWasCreatedMsg);
		void OnInfo_ThrowTrapException(bool bMainThread);
		LRESULT OnKeyboard(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
		LRESULT OnKeyboardHook(DWORD VkMod);
		LRESULT OnKeyboardIme(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
		LRESULT OnLangChange(UINT messg, WPARAM wParam, LPARAM lParam);
		LRESULT OnLangChangeConsole(CVirtualConsole *apVCon, const DWORD adwLayoutName);
		LRESULT OnMouse(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
		LRESULT OnMouse_Move(CVirtualConsole* pVCon, HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr);
		LRESULT OnMouse_LBtnDown(CVirtualConsole* pVCon, HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr);
		LRESULT OnMouse_LBtnUp(CVirtualConsole* pVCon, HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr);
		LRESULT OnMouse_LBtnDblClk(CVirtualConsole* pVCon, HWND hWnd, UINT& messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr);
		LRESULT OnMouse_RBtnDown(CVirtualConsole* pVCon, HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr);
		LRESULT OnMouse_RBtnUp(CVirtualConsole* pVCon, HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr);
		LRESULT OnMouse_RBtnDblClk(CVirtualConsole* pVCon, HWND hWnd, UINT& messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr);
		BOOL OnMouse_NCBtnDblClk(HWND hWnd, UINT& messg, WPARAM wParam, LPARAM lParam);
		//virtual void OnPaintClient(HDC hdc/*, int width, int height*/);
		LRESULT OnSetCursor(WPARAM wParam=-1, LPARAM lParam=-1);
		LRESULT OnSize(bool bResizeRCon=true, WPARAM wParam=0, WORD newClientWidth=(WORD)-1, WORD newClientHeight=(WORD)-1);
		LRESULT OnSizing(WPARAM wParam, LPARAM lParam);
		LRESULT OnMoving(LPRECT prcWnd = NULL, bool bWmMove = false);
		virtual LRESULT OnWindowPosChanged(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		LRESULT OnWindowPosChanging(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		void OnSizePanels(COORD cr);
		LRESULT OnShellHook(WPARAM wParam, LPARAM lParam);
		//LRESULT OnSysCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
		UINT_PTR SetKillTimer(bool bEnable, UINT nTimerID, UINT nTimerElapse);
		LRESULT OnTimer(WPARAM wParam, LPARAM lParam);
		void OnTimer_Main(CVirtualConsole* pVCon);
		void OnTimer_ConRedraw(CVirtualConsole* pVCon);
		void OnTimer_FrameAppearDisappear(WPARAM wParam);
		void OnTimer_RClickPaint();
		void OnTimer_AdmShield();
		void OnTimer_QuakeFocus();
		void OnTransparent();
		void OnTransparent(bool abFromFocus/* = false*/, bool bSetFocus/* = false*/);
		void OnTransparentSeparate(bool bSetFocus);
		void OnVConCreated(CVirtualConsole* apVCon, const RConStartArgs *args);
		void OnAllVConClosed();
		void OnAllGhostClosed();
		void OnGhostCreated(CVirtualConsole* apVCon, HWND ahGhost);
		void OnRConStartedSuccess(CRealConsole* apRCon);
		LRESULT OnUpdateScrollInfo(BOOL abPosted = FALSE);
		void OnPanelViewSettingsChanged(BOOL abSendChanges=TRUE);
		void OnGlobalSettingsChanged();
		void OnTaskbarCreated();
		void OnTaskbarButtonCreated();
		void OnTaskbarSettingsChanged();
		void OnDefaultTermChanged();
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

		// IME support (WinXP or later)
		HMODULE mh_Imm32;
		ImmSetCompositionFontW_t _ImmSetCompositionFont;
		ImmSetCompositionWindow_t _ImmSetCompositionWindow;
		ImmGetContext_t _ImmGetContext;
};

#ifndef __GNUC__
#include <intrin.h>
#else
#define _InterlockedIncrement InterlockedIncrement
#endif

// Message Logger
// Originally from http://preshing.com/20120522/lightweight-in-memory-logging
namespace ConEmuMsgLogger
{
	enum Source {
		msgCommon,
		msgMain,
		msgWork,
		msgApp,
		msgCanvas,
		msgBack,
		msgGhost,
	};
	static const int BUFFER_SIZE = RELEASEDEBUGTEST(0x1000,0x1000);   // Must be a power of 2
	extern MSG g_msg[BUFFER_SIZE];
	extern LONG g_msgidx;

	static const int BUFFER_POS_SIZE = RELEASEDEBUGTEST(0x100,0x100);   // Must be a power of 2
	struct Event {
		short x, y, w, h;
		enum {
			Empty,
			WindowPosChanging,
			WindowPosChanged,
			Sizing,
			Size,
			Moving,
			Move,
		} msg;
		DWORD time;
		HWND hwnd;
	};
	extern Event g_pos[BUFFER_POS_SIZE];
	extern LONG g_posidx;
	extern void LogPos(const MSG& msg, Source from);
 
	inline void Log(const MSG& msg, Source from)
	{
		// Get next message index
		LONG i = _InterlockedIncrement(&g_msgidx);
		// Write a message at this index
		g_msg[i & (BUFFER_SIZE - 1)] = msg; // Wrap to buffer size
		switch (msg.message)
		{
		case WM_WINDOWPOSCHANGING:
		case WM_WINDOWPOSCHANGED:
		case WM_MOVE:
		case WM_SIZE:
		case WM_SIZING:
		case WM_MOVING:
			LogPos(msg, from);
			break;
		}
	}
}

extern CConEmuMain *gpConEmu;
