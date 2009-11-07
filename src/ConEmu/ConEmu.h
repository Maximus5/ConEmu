
#pragma once

#define HDCWND ghWndDC

#ifndef __ITaskbarList3_FWD_DEFINED__
#define __ITaskbarList3_FWD_DEFINED__
typedef interface ITaskbarList3 ITaskbarList3;
#endif 	/* __ITaskbarList3_FWD_DEFINED__ */

#define WM_TRAYNOTIFY WM_USER+1
#define ID_HELP 0xABC7
#define ID_CON_TOGGLE_VISIBLE 0xABC8
#define ID_CON_PASTE 0xABC9
#define ID_AUTOSCROLL 0xABCA
#define ID_DUMPCONSOLE 0xABCB
#define ID_CONPROP 0xABCC
#define ID_SETTINGS 0xABCD
#define ID_ABOUT 0xABCE
#define ID_TOTRAY 0xABCF

#define IID_IShellLink IID_IShellLinkW 

#define GET_X_LPARAM(inPx) ((int)(short)LOWORD(inPx))
#define GET_Y_LPARAM(inPy) ((int)(short)HIWORD(inPy))

#define RCLICKAPPSTIMEOUT 300
#define RCLICKAPPSDELTA 3
#define DRAG_DELTA 5

//typedef DWORD (WINAPI* FGetModuleFileNameEx)(HANDLE hProcess,HMODULE hModule,LPWSTR lpFilename,DWORD nSize);


class CConEmuChild;
class CConEmuBack;

WARNING("Проверить, чтобы DC нормально центрировалось после удаления CEM_BACK");
enum ConEmuMargins {
	CEM_FRAME = 0, // Разница между размером всего окна и клиентской области окна (рамка + заголовок)
	// Далее все отступы считаются в клиентской части (дочерние окна)!
	CEM_TAB,       // Отступы от краев таба (если он видим) до окна фона (с прокруткой)
	CEM_TABACTIVATE,
	CEM_TABDEACTIVATE,
	//2009-06-07 Это был только SM_CXVSCROLL, который сейчас всплывает и не занимает место!
	//CEM_BACK,      // Отступы от краев окна фона (с прокруткой) до окна с отрисовкой (DC)
	CEM_BACKFORCESCROLL,
	CEM_BACKFORCENOSCROLL
};

enum ConEmuRect {
	CER_MAIN = 0,   // Полный размер окна
	// Далее все координаты считаются относительно клиенсткой области {0,0}
	CER_MAINCLIENT, // клиентская область главного окна
	CER_TAB,        // положение контрола с закладками (всего)
	CER_BACK,       // положение окна с фоном
	CER_DC,         // положение окна отрисовки
	CER_CONSOLE,    // !!! _ размер в символах _ !!!
	CER_FULLSCREEN, // полный размер в pix текущего монитора (содержащего ghWnd)
	CER_MAXIMIZED,  // размер максимизированного окна на текущем мониторе (содержащего ghWnd)
	CER_CORRECTED   // скорректированное положение (чтобы окно было видно на текущем мониторе)
};

class CConEmuMain
{
public:
	//HMODULE mh_Psapi;
	//FGetModuleFileNameEx GetModuleFileNameEx;
	wchar_t ms_ConEmuExe[MAX_PATH+1];
	wchar_t ms_ConEmuChm[MAX_PATH+1];
	wchar_t ms_ConEmuCExe[MAX_PATH+1];
	wchar_t ms_ConEmuCurDir[MAX_PATH+1];
public:
	CConEmuChild m_Child;
	CConEmuBack  m_Back;
	//POINT cwShift; // difference between window size and client area size for main ConEmu window
	POINT ptFullScreenSize; // size for GetMinMaxInfo in Fullscreen mode
	//DWORD gnLastProcessCount;
	//uint cBlinkNext;
	DWORD WindowMode, change2WindowMode;
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
	struct {
		WORD  state;
		bool  bSkipRDblClk;
		bool  bIgnoreMouseMove;
		
		COORD LClkDC, LClkCon;
		DWORD LClkTick;
		COORD RClkDC, RClkCon;
		DWORD RClkTick;

		// Чтобы не слать в консоль бесконечные WM_MOUSEMOVE
		WPARAM lastMMW;
		LPARAM lastMML;
		
		// Пропустить клик мышкой (окно было неактивно)
		UINT nSkipEvents[2]; UINT nReplaceDblClk;
	} mouse;
	bool isPiewUpdate;
	bool gbPostUpdateWindowSize;
	HWND hPictureView; bool bPicViewSlideShow; DWORD dwLastSlideShowTick; RECT mrc_WndPosOnPicView;
	//bool gb_ConsoleSelectMode;
	//bool setParent, setParent2;
	//BOOL mb_InClose;
	//int RBDownNewX, RBDownNewY;
	POINT cursor, Rcursor;
	//WPARAM lastMMW;
	//LPARAM lastMML;
	COORD m_LastConSize; // console size after last resize (in columns and lines)
	bool mb_IgnoreSizeChange;
	TCHAR szConEmuVersion[32];
	DWORD m_ProcCount;
	//DWORD mn_ActiveStatus;
	//TCHAR ms_EditorRus[16], ms_ViewerRus[16], ms_TempPanel[32], ms_TempPanelRus[32];
protected:
	CDragDrop *mp_DragDrop;
	CProgressBars *ProgressBars;
	TCHAR Title[MAX_TITLE_SIZE], TitleCmp[MAX_TITLE_SIZE], MultiTitle[MAX_TITLE_SIZE+30];
	short mn_Progress;
	LPTSTR GetTitleStart();
	BOOL mb_InTimer;
	BOOL mb_ProcessCreated; DWORD mn_StartTick;
	HWINEVENTHOOK mh_WinHook; //, mh_PopupHook;
	static VOID CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
	CVirtualConsole *mp_VCon[MAX_CONSOLE_COUNT];
	CVirtualConsole *mp_VActive, *mp_VCon1, *mp_VCon2;
	bool mb_SkipSyncSize, mb_PassSysCommand;
	//wchar_t *mpsz_RecreateCmd;
	ITaskbarList3 *mp_TaskBar;
	typedef BOOL (WINAPI* FRegisterShellHookWindow)(HWND);
	RECT mrc_Ideal;
	BOOL mb_MouseCaptured;
	BYTE m_KeybStates[256];
	wchar_t ms_ConEmuAliveEvent[MAX_PATH+64];
	HANDLE mh_ConEmuAliveEvent; BOOL mb_ConEmuAliveOwned, mb_AliveInitialized;
	//
	BOOL mb_HotKeyRegistered;
	void RegisterHotKeys();
	void UnRegisterHotKeys();
	void CtrlWinAltSpace();
	//DWORD_PTR mn_CurrentKeybLayout;
	// Registered messages
	DWORD mn_MainThreadId;
	UINT mn_MsgPostCreate;
	UINT mn_MsgPostCopy;
	UINT mn_MsgMyDestroy;
	UINT mn_MsgUpdateSizes;
	UINT mn_MsgSetWindowMode;
	UINT mn_MsgUpdateTitle;
	UINT mn_MsgAttach;
	UINT mn_MsgSrvStarted;
	UINT mn_MsgVConTerminated;
	UINT mn_MsgUpdateScrollInfo;
	UINT mn_MsgUpdateTabs;
	UINT mn_MsgOldCmdVer; BOOL mb_InShowOldCmdVersion;
	UINT mn_MsgTabCommand;
	UINT mn_MsgSheelHook;
	UINT mn_ShellExecuteEx;
	UINT mn_PostConsoleResize;
	UINT mn_ConsoleLangChanged;
	UINT mn_MsgPostOnBufferHeight;
	UINT mn_MsgSetForeground;
	UINT mn_MsgFlashWindow;
	
	//
	static DWORD CALLBACK ServerThread(LPVOID lpvParam);
	void ServerThreadCommand(HANDLE hPipe);
	DWORD mn_ServerThreadId; HANDLE mh_ServerThread, mh_ServerThreadTerminate;

public:
	DWORD CheckProcesses();
	DWORD GetFarPID();
	
public:
	LPCTSTR GetTitle();
	LPCTSTR GetTitle(int nIdx);
	CVirtualConsole* GetVCon(int nIdx);
	void UpdateProcessDisplay(BOOL abForce);
	void UpdateSizes();

public:
	CConEmuMain();
	~CConEmuMain();

public:
	CVirtualConsole* ActiveCon();
	BOOL Activate(CVirtualConsole* apVCon);
	int ActiveConNum(); // 0-based
	static void AddMargins(RECT& rc, RECT& rcAddShift, BOOL abExpand=FALSE);
	LPARAM AttachRequested(HWND ahConWnd, DWORD anConemuC_PID);
	static RECT CalcMargins(enum ConEmuMargins mg);
	static RECT CalcRect(enum ConEmuRect tWhat, RECT rFrom, enum ConEmuRect tFrom, RECT* prDC=NULL, enum ConEmuMargins tTabAction=CEM_TAB);
	bool ConActivate(int nCon);
	bool ConActivateNext(BOOL abNext);
	void CheckGuiBarsCreated();
	CVirtualConsole* CreateCon(RConStartArgs *args);
	BOOL CreateMainWindow();
	void Destroy();
	void DebugStep(LPCTSTR asMsg);
	void EnableComSpec(BOOL abSwitch);
	void ForceShowTabs(BOOL abShow);
	DWORD_PTR GetActiveKeyboardLayout();
	RECT GetIdealRect() { return mrc_Ideal; };
	LRESULT GuiShellExecuteEx(SHELLEXECUTEINFO* lpShellExecute, BOOL abAllowAsync);
	BOOL Init();
	void Invalidate(CVirtualConsole* apVCon);
	void InvalidateAll();
	bool isActive(CVirtualConsole* apVCon);
	bool isConSelectMode();
	bool isDragging();
	bool isEditor();
	bool isFar();
	bool isFilePanel(bool abPluginAllowed=false);
	bool isFirstInstance();
	bool isIconic();
	bool isLBDown();
	bool isMainThread();
	bool isMeForeground();
	bool isNtvdm();
	bool isPictureView();
	bool isSizing();
	bool isValid(CVirtualConsole* apVCon);
	bool isViewer();
	bool isVisible(CVirtualConsole* apVCon);
	bool isWindowNormal();
	bool isZoomed();
	void LoadIcons();
	bool LoadVersionInfo(wchar_t* pFullPath);
	static RECT MapRect(RECT rFrom, BOOL bFrame2Client);
	void PaintCon();
	void PostCopy(wchar_t* apszMacro, BOOL abRecieved=FALSE);
	void PostMacro(LPCWSTR asMacro);
	void PostCreate(BOOL abRecieved=FALSE);
	bool PtDiffTest(POINT C, int aX, int aY, UINT D); //(((abs(C.x-LOWORD(lParam)))<D) && ((abs(C.y-HIWORD(lParam)))<D))
	void Recreate(BOOL abRecreate, BOOL abConfirm);
	static INT_PTR CALLBACK RecreateDlgProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam);
	static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
	void RePaint();
	void ReSize(BOOL abCorrect2Ideal = FALSE);
	BOOL RunSingleInstance();
	void SetConsoleWindowSize(const COORD& size, bool updateInfo);
	bool SetWindowMode(uint inMode);
	void ShowOldCmdVersion(DWORD nCmd, DWORD nVersion);
	void ShowSysmenu(HWND Wnd, HWND Owner, int x, int y);
	void SyncConsoleToWindow();
	void SyncNtvdm();
	void SyncWindowToConsole();
	void SwitchKeyboardLayout(DWORD_PTR dwNewKeybLayout);
	void TabCommand(UINT nTabCmd);
	void Update(bool isForce = false);
	void UpdateTitle(LPCTSTR asNewTitle);
	void UpdateProgress(BOOL abUpdateTitle);
	static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
	LRESULT WndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
public:
	void OnBufferHeight(); //BOOL abBufferHeight);
	LRESULT OnClose(HWND hWnd);
	void OnConsoleResize(BOOL abPosted=FALSE);
	LRESULT OnCreate(HWND hWnd, LPCREATESTRUCT lpCreate);
	LRESULT OnDestroy(HWND hWnd);
	LRESULT OnFlashWindow(DWORD nFlags, DWORD nCount, HWND hCon);
	LRESULT OnFocus(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
	LRESULT OnGetMinMaxInfo(LPMINMAXINFO pInfo);
	LRESULT OnKeyboard(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
	LRESULT OnLangChange(UINT messg, WPARAM wParam, LPARAM lParam);
	LRESULT OnLangChangeConsole(CVirtualConsole *apVCon, DWORD dwLayoutName);
	LRESULT OnMouse(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
	LRESULT OnPaint(WPARAM wParam, LPARAM lParam);
	LRESULT OnSize(WPARAM wParam=0, WORD newClientWidth=(WORD)-1, WORD newClientHeight=(WORD)-1);
	LRESULT OnSizing(WPARAM wParam, LPARAM lParam);
	LRESULT OnShellHook(WPARAM wParam, LPARAM lParam);
	LRESULT OnSysCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnTimer(WPARAM wParam, LPARAM lParam);
	LRESULT OnVConTerminated(CVirtualConsole* apVCon, BOOL abPosted = FALSE);
	LRESULT OnUpdateScrollInfo(BOOL abPosted = FALSE);
};
