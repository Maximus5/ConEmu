#pragma once
#include "kl_parts.h"
#include "../Common/common.hpp"

#define CES_CMDACTIVE 0x01
#define CES_TELNETACTIVE 0x02
#define CES_FARACTIVE 0x04
#define CES_CONALTERNATIVE 0x08
#define CES_PROGRAMS (0x0F) // - CES_CONMANACTIVE)

//#define CES_NTVDM 0x10 -- common.hpp
#define CES_PROGRAMS2 0xFF

#define CES_FILEPANEL      0x0001
#define CES_TEMPPANEL      0x0002
#define CES_PLUGINPANEL    0x0004
#define CES_EDITOR         0x0010
#define CES_VIEWER         0x0020
#define CES_COPYING        0x0040
#define CES_MOVING         0x0080
#define CES_FARFLAGS       0xFFFF
#define CES_MAYBEPANEL   0x010000
#define CES_WASPROGRESS  0x020000
#define CES_OPER_ERROR   0x040000
//... and so on

// Undocumented console message
#define WM_SETCONSOLEINFO           (WM_USER+201)
// and others
#define SC_RESTORE_SECRET 0x0000f122
#define SC_MAXIMIZE_SECRET 0x0000f032
#define SC_PROPERTIES_SECRET 0x0000fff7
#define SC_MARK_SECRET 0x0000fff2
#define SC_COPY_ENTER_SECRET 0x0000fff0
#define SC_PASTE_SECRET 0x0000fff1
#define SC_SELECTALL_SECRET 0x0000fff5
#define SC_SCROLL_SECRET 0x0000fff3
#define SC_FIND_SECRET 0x0000fff4

#define MAX_TITLE_SIZE 0x400

#pragma pack(push, 1)


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

#pragma pack(pop)

struct ConProcess {
    DWORD ProcessID, ParentPID;
    bool  IsFar;
    bool  IsTelnet; // может быть включен ВМЕСТЕ с IsFar, если удалось подцепится к фару через сетевой пайп
    bool  IsNtvdm;  // 16bit приложения
    bool  IsCmd;    // значит фар выполняет команду
    bool  NameChecked, RetryName;
    bool  Alive, inConsole;
    TCHAR Name[64]; // чтобы полная инфа об ошибке влезала
};

#define MAX_SERVER_THREADS 3
#define MAX_THREAD_PACKETS 100

class CVirtualConsole;

class CRealConsole
{
#ifdef _DEBUG
    friend class CVirtualConsole;
#endif
public:

    uint TextWidth();
    uint TextHeight();
    
private:
    HWND    hConWnd;

public:
	HWND    ConWnd();

    CRealConsole(CVirtualConsole* apVCon);
    ~CRealConsole();
    
    BOOL PreInit(BOOL abCreateBuffers=TRUE);
    void DumpConsole(HANDLE ahFile);

    BOOL SetConsoleSize(COORD size, DWORD anCmdID=CECMD_SETSIZE);
private:
    void SendConsoleEvent(INPUT_RECORD* piRec);
    DWORD mn_FlushIn, mn_FlushOut;
public:
    void PostConsoleEvent(INPUT_RECORD* piRec);
    BOOL FlushInputQueue(DWORD nTimeout = 500);
    void OnKeyboard(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
    void OnMouse(UINT messg, WPARAM wParam, int x, int y);
    void OnFocus(BOOL abFocused);
    
    void StopSignal();
    void StopThread(BOOL abRecreating=FALSE);
    BOOL isBufferHeight();
	HWND isPictureView();
	BOOL isWindowVisible();
    LPCTSTR GetTitle();
    void GetConsoleScreenBufferInfo(CONSOLE_SCREEN_BUFFER_INFO* sbi) { *sbi = con.m_sbi; };
    void GetConsoleSelectionInfo(CONSOLE_SELECTION_INFO *sel) { *sel = con.m_sel; };
    void GetConsoleCursorInfo(CONSOLE_CURSOR_INFO *ci) { *ci = con.m_ci; };
    DWORD GetConsoleCP() { return con.m_dwConsoleCP; };
    DWORD GetConsoleOutputCP() { return con.m_dwConsoleOutputCP; };
    DWORD GetConsoleMode() { return con.m_dwConsoleMode; };
    void SyncConsole2Window();
    void OnWinEvent(DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
    int  GetProcesses(ConProcess** ppPrc);
    DWORD GetFarPID();
    DWORD GetProgramStatus();
	DWORD GetFarStatus();
    DWORD GetServerPID();
    LRESULT OnScroll(int nDirection);
    BOOL isConSelectMode();
    BOOL isFar();
    void ShowConsole(int nMode); // -1 Toggle, 0 - Hide, 1 - Show
    BOOL isDetached();
    BOOL AttachConemuC(HWND ahConWnd, DWORD anConemuC_PID);
    BOOL RecreateProcess(RConStartArgs *args);
    void GetData(wchar_t* pChar, WORD* pAttr, int nWidth, int nHeight);
    void OnActivate(int nNewNum, int nOldNum);
    void OnDeactivate(int nNewNum);
    BOOL CheckBufferSize();
    //LRESULT OnConEmuCmd(BOOL abStarted, DWORD anConEmuC_PID);
    BOOL BufferHeightTurnedOn(CONSOLE_SCREEN_BUFFER_INFO* psbi);
    void UpdateScrollInfo();
    void SetTabs(ConEmuTab* tabs, int tabsCount);
    BOOL GetTab(int tabIdx, /*OUT*/ ConEmuTab* pTab);
    int GetTabCount();
    BOOL ActivateFarWindow(int anWndIndex);
    DWORD CanActivateFarWindow(int anWndIndex);
    void SwitchKeyboardLayout(DWORD dwNewKeybLayout);
    void CloseConsole();
    void Paste();
    void LogString(LPCSTR asText);
	bool isActive();
	bool isFilePanel(bool abPluginAllowed=false);
	bool isEditor();
	bool isViewer();
	bool isNtvdm();
	bool isPackets();
	LPCWSTR GetCmd();
	short GetProgress(BOOL* rpbError);
	void EnableComSpec(DWORD anFarPID, BOOL abSwitch);
	int CoordInPanel(COORD cr);
	void GetPanelRect(BOOL abRight, RECT* prc);
	bool isAdministrator();
	BOOL isMouseButtonDown();

public:
    // Вызываются из CVirtualConsole
    BOOL PreCreate(RConStartArgs *args);
		//(BOOL abDetached, LPCWSTR asNewCmd = NULL, BOOL abAsAdmin = FALSE);
    BOOL IsConsoleThread();
    void SetForceRead();
    //DWORD WaitEndUpdate(DWORD dwTimeout=1000);

protected:
    CVirtualConsole* mp_VCon; // соответствующая виртуальная консоль
    DWORD mn_ConEmuC_PID; HANDLE mh_ConEmuC, mh_ConEmuCInput;
    TCHAR ms_ConEmuC_Pipe[MAX_PATH], ms_ConEmuCInput_Pipe[MAX_PATH], ms_VConServer_Pipe[MAX_PATH];
    TCHAR Title[MAX_TITLE_SIZE+1], TitleCmp[MAX_TITLE_SIZE+1];
    short mn_Progress, mn_PreWarningProgress;

    BOOL AttachPID(DWORD dwPID);
    BOOL StartProcess();
    BOOL StartMonitorThread();
    BOOL mb_NeedStartProcess;

    // Нить наблюдения за консолью
    static DWORD WINAPI MonitorThread(LPVOID lpParameter);
    HANDLE mh_MonitorThread; DWORD mn_MonitorThreadID;
    // Для пересылки событий ввода в консоль
    static DWORD WINAPI InputThread(LPVOID lpParameter);
    HANDLE mh_InputThread; DWORD mn_InputThreadID;
    
    HANDLE mh_TermEvent, mh_MonitorThreadEvent; //, mh_Sync2WindowEvent;
    BOOL mb_FullRetrieveNeeded; //, mb_Detached;
	RConStartArgs m_Args;
    //wchar_t* ms_SpecialCmd;
	//BOOL mb_RunAsAdministrator;
    

    void Box(LPCTSTR szText);

    BOOL RetrieveConsoleInfo(/*BOOL bShortOnly,*/ UINT anWaitSize);
    BOOL InitBuffers(DWORD OneBufferSize);
private:
    // Эти переменные инициализируются в RetrieveConsoleInfo()
	MSection csCON; //DWORD ncsT;
    struct {
        CONSOLE_SELECTION_INFO m_sel;
        CONSOLE_CURSOR_INFO m_ci;
        DWORD m_dwConsoleCP, m_dwConsoleOutputCP, m_dwConsoleMode;
        CONSOLE_SCREEN_BUFFER_INFO m_sbi;
        USHORT nTopVisibleLine; // может отличаться от m_sbi.srWindow.Top, если прокрутка заблокирована
        wchar_t *pConChar;
        WORD  *pConAttr;
        int nTextWidth, nTextHeight;
        int nChange2TextWidth, nChange2TextHeight;
        BOOL bBufferHeight;
        DWORD nPacketIdx;
        DWORD dwKeybLayout;
        BOOL bRBtnDrag; // в консоль посылается драг правой кнопкой (выделение в FAR)
        COORD crRBtnDrag;
    } con;
    // 
    MSection csPRC; //DWORD ncsTPRC;
    std::vector<ConProcess> m_Processes;
    int mn_ProcessCount;
    //
    DWORD mn_FarPID, mn_LastSetForegroundPID;
    //
    ConEmuTab* mp_tabs;
    int mn_tabsCount, mn_ActiveTab;
    WCHAR ms_PanelTitle[CONEMUTABMAX];
    void CheckPanelTitle();
    //
    //void ProcessAdd(DWORD addPID);
    //void ProcessDelete(DWORD addPID);
    void ProcessUpdate(DWORD *apPID, UINT anCount);
    void ProcessUpdateFlags(BOOL abProcessChanged);
    void ProcessCheckName(struct ConProcess &ConPrc, LPWSTR asFullFileName);
    DWORD mn_ProgramStatus, mn_FarStatus;
	BOOL mb_IgnoreCmdStop; // При запуске 16bit приложения не возвращать размер консоли! Это сделает OnWinEvent
    BOOL isShowConsole;
    BOOL mb_ConsoleSelectMode;
    static DWORD WINAPI ServerThread(LPVOID lpvParam);
    HANDLE mh_ServerThreads[MAX_SERVER_THREADS], mh_ActiveServerThread;
    DWORD  mn_ServerThreadsId[MAX_SERVER_THREADS];
    HANDLE mh_ServerSemaphore, mh_GuiAttached;
    void SetBufferHeightMode(BOOL abBufferHeight, BOOL abLock=FALSE);
    BOOL mb_BuferModeChangeLocked;

    void ServerThreadCommand(HANDLE hPipe);
    void ApplyConsoleInfo(CESERVER_REQ* pInfo);
    void SetHwnd(HWND ahConWnd);
    WORD mn_LastVKeyPressed;
    BOOL GetConWindowSize(const CONSOLE_SCREEN_BUFFER_INFO& sbi, int& nNewWidth, int& nNewHeight, BOOL* pbBufferHeight=NULL);
    int mn_Focused; //-1 после запуска, 1 - в фокусе, 0 - не в фокусе
    DWORD mn_InRecreate; // Tick, когда начали пересоздание
    BOOL mb_ProcessRestarted;
    // Логи
    BYTE m_UseLogs;
    HANDLE mh_LogInput; wchar_t *mpsz_LogInputFile, *mpsz_LogPackets; UINT mn_LogPackets;
    void CreateLogFiles();
    void CloseLogFiles();
    void LogInput(INPUT_RECORD* pRec);
    void LogPacket(CESERVER_REQ* pInfo);
    BOOL RecreateProcessStart();
    // Прием и обработка пакетов
    //MSection csPKT; //DWORD ncsTPKT;
    DWORD mn_LastProcessedPkt; HANDLE mh_PacketArrived;
    //std::vector<CESERVER_REQ*> m_Packets;
    CESERVER_REQ* m_PacketQueue[(MAX_SERVER_THREADS+1)*MAX_THREAD_PACKETS];
    void PushPacket(CESERVER_REQ* pPkt);
    CESERVER_REQ* PopPacket();
	BOOL mb_DataChanged;
    //
    BOOL PrepareOutputFile(BOOL abUnicodeText, wchar_t* pszFilePathName);
    HANDLE PrepareOutputFileCreate(wchar_t* pszFilePathName);
    // фикс для dblclick в редакторе
    MOUSE_EVENT_RECORD m_LastMouse;
	//
	wchar_t ms_Editor[32], ms_EditorRus[32], ms_Viewer[32], ms_ViewerRus[32];
	wchar_t ms_TempPanel[32], ms_TempPanelRus[32];
	//
	BOOL mb_PluginDetected; DWORD mn_FarPID_PluginDetected;
	void CheckFarStates();
	void OnTitleChanged();
	DWORD mn_LastInvalidateTick;
	//
	HWND hPictureView; BOOL mb_PicViewWasHidden;
	// координаты панелей в символах
	RECT mr_LeftPanel, mr_RightPanel;
	void FindPanels(BOOL abResetOnly=FALSE);
	// 
	BOOL mb_MouseButtonDown;
};
