#pragma once

#define HDCWND ghWndDC


#define WM_TRAYNOTIFY WM_USER+1
#define ID_DUMPCONSOLE 0xABCB
#define ID_CONPROP 0xABCC
#define ID_SETTINGS 0xABCD
#define ID_HELP 0xABCE
#define ID_TOTRAY 0xABCF

#define SC_RESTORE_SECRET 0x0000f122
#define SC_MAXIMIZE_SECRET 0x0000f032
#define SC_PROPERTIES_SECRET 65527

#define IID_IShellLink IID_IShellLinkW 

#define GET_X_LPARAM(inPx) ((int)(short)LOWORD(inPx))
#define GET_Y_LPARAM(inPy) ((int)(short)HIWORD(inPy))

#define RCLICKAPPSTIMEOUT 300
#define RCLICKAPPSDELTA 3
#define DRAG_DELTA 5
#define MAX_TITLE_SIZE 0x400

typedef DWORD (WINAPI* FGetModuleFileNameEx)(HANDLE hProcess,HMODULE hModule,LPWSTR lpFilename,DWORD nSize);


class CConEmuChild;
class CConEmuBack;

enum ConEmuMargins {
	CEM_FRAME = 0, // Разница между размером всего окна и клиентской области окна (рамка + заголовок)
	// Далее все отступы считаются в клиентской части (дочерние окна)!
	CEM_TAB,       // Отступы от краев таба (если он видим) до окна фона (с прокруткой)
	CEM_BACK       // Отступы от краев окна фона (с прокруткой) до окна с отрисовкой (DC)
};

enum ConEmuRect {
	CER_MAIN = 0,   // Полный размер окна
	// Далее все координаты считаются относительно клиенсткой области {0,0}
	CER_MAINCLIENT, // клиентская область главного окна
	CER_TAB,        // положение контрола с закладками (всего)
	CER_BACK,       // положение окна с прокруткой
	CER_DC,         // положение окна отрисовки
	CER_CONSOLE     // !!! _ размер в символах _ !!!
};

class CConEmuMain
{
public:
	HMODULE mh_Psapi;
	FGetModuleFileNameEx GetModuleFileNameEx;
public:
	CConEmuChild m_Child;
	CConEmuBack  m_Back;
	//POINT cwShift; // difference between window size and client area size for main ConEmu window
	POINT ptFullScreenSize; // size for GetMinMaxInfo in Fullscreen mode
	//DWORD gnLastProcessCount;
	uint cBlinkNext;
	DWORD WindowMode;
	//HANDLE hPipe;
	//HANDLE hPipeEvent;
	bool isWndNotFSMaximized;
	bool isShowConsole;
	bool mb_FullWindowDrag;
	//bool isLBDown, /*isInDrag,*/ isDragProcessed, 
	//mb_InSizing, -> state&MOUSE_SIZING
	//mb_IgnoreMouseMove;
	//bool isRBDown, ibSkipRDblClk; DWORD dwRBDownTick;
	struct {
		BYTE  state;
		bool  bSkipRDblClk;
		bool  bIgnoreMouseMove;
		
		COORD LClkDC, LClkCon;
		DWORD LClkTick;
		COORD RClkDC, RClkCon;
		DWORD RClkTick;

		// Чтобы не слать в консоль бесконечные WM_MOUSEMOVE
		WPARAM lastMMW;
		LPARAM lastMML;
	} mouse;
	bool isPiewUpdate;
	bool gbPostUpdateWindowSize;
	HWND hPictureView; bool bPicViewSlideShow; DWORD dwLastSlideShowTick;
	bool gb_ConsoleSelectMode;
	bool setParent, setParent2;
	//BOOL mb_InClose;
	//int RBDownNewX, RBDownNewY;
	POINT cursor, Rcursor;
	//WPARAM lastMMW;
	//LPARAM lastMML;
	CDragDrop *DragDrop;
	CProgressBars *ProgressBars;
	COORD m_LastConSize; // console size after last resize (in columns and lines)
	bool mb_IgnoreSizeChange; // на время переключения в конмане...
	int BufferHeight; // в отличие от gSet - может изменяться текущей консольной программой
	//RECT  dcWindowLast; // Последний размер дочернего окна
	uint cBlinkShift; // cursor blink counter threshold
	TCHAR szConEmuVersion[32];
	//DWORD m_ProcList[1000], 
	DWORD m_ProcCount, m_ActiveConmanIDX, mn_ConmanPID;
	HMODULE mh_ConMan;
	//HMODULE mh_Infis; TCHAR ms_InfisPath[MAX_PATH*2];
	DWORD mn_ActiveStatus;
	DWORD mn_TopProcessID; 
	//BOOL mb_FarActive;
	//TCHAR ms_TopProcess[MAX_PATH+1];
	TCHAR ms_EditorRus[16], ms_ViewerRus[16], ms_TempPanel[32];
protected:
	TCHAR Title[MAX_TITLE_SIZE], TitleCmp[MAX_TITLE_SIZE];
	void UpdateTitle(LPCTSTR asNewTitle);
	struct ConProcess {
		DWORD ProcessID, ParentPID;
		BYTE  ConmanIDX;
		//bool  Alive;
		bool  IsConman;
		bool  IsFar;
		bool  IsTelnet; // может быть включен ВМЕСТЕ с IsFar, если удалось подцепится к фару через сетевой пайп
		bool  IsNtvdm; // 16bit приложения
		bool  NameChecked, ConmanChecked, RetryName;
		TCHAR Name[64]; // чтобы полная инфа об ошибке влезала
	};
	//int mn_NeedRetryName;
	std::vector<struct ConProcess> m_Processes;
	void CheckProcessName(struct ConProcess &ConPrc, LPCTSTR asFullFileName);
	LPTSTR GetTitleStart(DWORD* rnConmanIDX=NULL);
	//bool GetProcessFileName(DWORD dwPID, TCHAR* rsName/*[32]*/, DWORD *pdwErr);
	BOOL mb_InTimer;
	BOOL mb_ProcessCreated; DWORD mn_StartTick;
	HWINEVENTHOOK mh_WinHook;
	static VOID CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
	CVirtualConsole *mp_VCon[MAX_CONSOLE_COUNT], *pVCon;
	int mn_ActiveCon; // в планах - убить m_ActiveConmanIDX
	// Registered messages
	UINT mn_MsgPostCopy;

public:
	DWORD CheckProcesses(DWORD nConmanIDX, BOOL bTitleChanged);
	typedef int (_cdecl * ConMan_MainProc_t)(LPCWSTR asCommandLine, BOOL abStandalone);
	ConMan_MainProc_t ConMan_MainProc;
	typedef void (_cdecl * ConMan_LookForKeyboard_t)();
	ConMan_LookForKeyboard_t ConMan_LookForKeyboard;
	typedef int (_cdecl * ConMan_ProcessCommand_t)( DWORD nCmd, int Param1, int Param2 );
	ConMan_ProcessCommand_t ConMan_ProcessCommand;
	
public:
	LPCTSTR GetTitle();
	void UpdateProcessDisplay(BOOL abForce);

public:
	CConEmuMain();
	~CConEmuMain();

public:
	CVirtualConsole* ActiveCon();
	static void AddMargins(RECT& rc, RECT& rcAddShift, BOOL abExpand=FALSE);
	static RECT CalcMargins(enum ConEmuMargins mg);
	static RECT CalcRect(enum ConEmuRect tWhat, RECT rFrom, enum ConEmuRect tFrom, RECT* prDC=NULL);
	bool CheckBufferSize();
	CVirtualConsole* CreateCon();
	//COORD ConsoleSizeFromWindow(RECT* arect = NULL, bool frameIncluded = false, bool alreadyClient = false);
	//RECT ConsoleOffsetRect();
	void Destroy();
	void DnDstep(LPCTSTR asMsg);
	//RECT DCClientRect(RECT* pClient=NULL);
	void ForceShowTabs(BOOL abShow);
	//void GetCWShift(HWND inWnd, POINT *outShift);
	//void GetCWShift(HWND inWnd, RECT *outShift);
	//int GetBufferHeight();
	static BOOL WINAPI HandlerRoutine(DWORD dwCtrlType);
	BOOL Init();
	BOOL InitConMan(LPCWSTR asCommandLine);
	void InvalidateAll();
	bool isConman();
	bool isConSelectMode();
	bool isDragging();
	bool isEditor();
	bool isFar();
	bool isFilePanel(bool abPluginAllowed=false);
	bool isLBDown();
	bool isNtvdm();
	bool isPictureView();
	bool isSizing();
	bool isViewer();
	void LoadIcons();
	bool LoadVersionInfo(wchar_t* pFullPath);
	static RECT MapRect(RECT rFrom, BOOL bFrame2Client);
	void PaintCon();
	void PaintGaps(HDC hDC=NULL);
	void PostCopy(wchar_t* apszMacro, BOOL abRecieved=FALSE);
	void PostMacro(LPCWSTR asMacro);
	void PostCreate();
	void ReSize();
	void SetConParent();
	void SetConsoleWindowSize(const COORD& size, bool updateInfo);
	bool SetWindowMode(uint inMode);
	void ShowSysmenu(HWND Wnd, HWND Owner, int x, int y);
	void SyncConsoleToWindow();
	void SyncNtvdm();
	void SyncWindowToConsole();
	void Update(bool isForce = false);
	//RECT WindowSizeFromConsole(COORD consoleSize, bool rectInWindow = false, bool clientOnly = false);
	LRESULT CALLBACK WndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
public:
	LRESULT OnClose(HWND hWnd);
	LRESULT OnCopyData(PCOPYDATASTRUCT cds);
	LRESULT OnCreate(HWND hWnd);
	LRESULT OnDestroy(HWND hWnd);
	LRESULT OnFocus(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
	LRESULT OnGetMinMaxInfo(LPMINMAXINFO pInfo);
	LRESULT OnKeyboard(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
	LRESULT OnMouse(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
	LRESULT OnPaint(WPARAM wParam, LPARAM lParam);
	LRESULT OnSize(WPARAM wParam, WORD newClientWidth, WORD newClientHeight);
	LRESULT OnSizing(WPARAM wParam, LPARAM lParam);
	LRESULT OnSysCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnTimer(WPARAM wParam, LPARAM lParam);
};
