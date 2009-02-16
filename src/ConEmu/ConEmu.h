#pragma once

#define HDCWND ghWndDC


#define WM_TRAYNOTIFY WM_USER+1
#define ID_SETTINGS 0xABCD
#define ID_HELP 0xABCE
#define ID_TOTRAY 0xABCF
#define ID_CONPROP 0xABCC

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

class CConEmuMain
{
public:
	HMODULE mh_Psapi;
	FGetModuleFileNameEx GetModuleFileNameEx;
public:
	CConEmuChild m_Child;
	CConEmuBack  m_Back;
	POINT cwShift; // difference between window size and client area size for main ConEmu window
	POINT cwConsoleShift; // difference between window size and client area size for ghConWnd
	POINT ptFullScreenSize; // size for GetMinMaxInfo in Fullscreen mode
	DWORD gnLastProcessCount;
	uint cBlinkNext;
	DWORD WindowMode;
	//HANDLE hPipe;
	//HANDLE hPipeEvent;
	bool isWndNotFSMaximized;
	bool isShowConsole;
	bool mb_FullWindowDrag;
	bool isLBDown, /*isInDrag,*/ isDragProcessed, mb_InSizing;
	bool isRBDown, ibSkilRDblClk; DWORD dwRBDownTick;
	bool isPiewUpdate;
	bool gbPostUpdateWindowSize;
	HWND hPictureView; bool bPicViewSlideShow; DWORD dwLastSlideShowTick;
	bool gb_ConsoleSelectMode;
	bool setParent, setParent2;
	//BOOL mb_InClose;
	int RBDownNewX, RBDownNewY;
	POINT cursor, Rcursor;
	WPARAM lastMMW;
	LPARAM lastMML;
	CDragDrop *DragDrop;
	CProgressBars *ProgressBars;
	TCHAR Title[MAX_TITLE_SIZE], TitleCmp[MAX_TITLE_SIZE];
	COORD srctWindowLast; // console size after last resize (in columns and lines)
	RECT  dcWindowLast; // Последний размер дочернего окна
	uint cBlinkShift; // cursor blink counter threshold
	TCHAR szConEmuVersion[32];
	DWORD m_ProcList[1000], m_ProcCount;
	DWORD mn_TopProcessID; BOOL mb_FarActive;
	TCHAR ms_TopProcess[MAX_PATH+1];

public:
	CConEmuMain();
	~CConEmuMain();
	BOOL Init();
	void Destroy();
	void InvalidateAll();

public:
	void SetConsoleWindowSize(const COORD& size, bool updateInfo);
	bool isPictureView();
	LRESULT CALLBACK WndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
	void ForceShowTabs(BOOL abShow);

	RECT WindowSizeFromConsole(COORD consoleSize, bool rectInWindow = false, bool clientOnly = false);
	COORD ConsoleSizeFromWindow(RECT* arect = NULL, bool frameIncluded = false, bool alreadyClient = false);
	void PaintGaps(HDC hDC=NULL);
	DWORD CheckProcesses();
	void CheckBufferSize();
	RECT DCClientRect(RECT* pClient=NULL);
	void SyncWindowToConsole();
	static BOOL WINAPI HandlerRoutine(DWORD dwCtrlType);
	void LoadIcons();
	void GetCWShift(HWND inWnd, POINT *outShift);
	void GetCWShift(HWND inWnd, RECT *outShift);
	void ShowSysmenu(HWND Wnd, HWND Owner, int x, int y);
	RECT ConsoleOffsetRect();
	bool SetWindowMode(uint inMode);
	void SyncConsoleToWindow();
	bool isFilePanel();
	bool isConSelectMode();
	bool LoadVersionInfo(wchar_t* pFullPath);
	void SetConParent();
public:
	LRESULT OnPaint(WPARAM wParam, LPARAM lParam);
	LRESULT OnSize(WPARAM wParam, WORD newClientWidth, WORD newClientHeight);
	LRESULT OnSizing(WPARAM wParam, LPARAM lParam);
	LRESULT OnTimer(WPARAM wParam, LPARAM lParam);
	LRESULT OnCopyData(PCOPYDATASTRUCT cds);
	LRESULT OnGetMinMaxInfo(LPMINMAXINFO pInfo);
	void ReSize();
	LRESULT OnCreate(HWND hWnd);
	LRESULT OnClose(HWND hWnd);
	LRESULT OnDestroy(HWND hWnd);
	LRESULT OnMouse(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
	LRESULT OnKeyboard(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
	LRESULT OnFocus(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSysCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
};
