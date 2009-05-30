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

#define CES_FILEPANEL 0x100
#define CES_TEMPPANEL 0x200
#define CES_PLUGINPANEL 0x400
#define CES_EDITOR 0x1000
#define CES_VIEWER 0x2000
#define CES_COPYING 0x4000
#define CES_MOVING 0x8000
#define CES_FARFLAGS 0xFFFF00
//... and so on

// Undocumented console message
#define WM_SETCONSOLEINFO			(WM_USER+201)
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
//	Structure to send console via WM_SETCONSOLEINFO
//
typedef struct _CONSOLE_INFO
{
	ULONG		Length;
	COORD		ScreenBufferSize;
	COORD		WindowSize;
	ULONG		WindowPosX;
	ULONG		WindowPosY;

	COORD		FontSize;
	ULONG		FontFamily;
	ULONG		FontWeight;
	WCHAR		FaceName[32];

	ULONG		CursorSize;
	ULONG		FullScreen;
	ULONG		QuickEdit;
	ULONG		AutoPosition;
	ULONG		InsertMode;
	
	USHORT		ScreenColors;
	USHORT		PopupColors;
	ULONG		HistoryNoDup;
	ULONG		HistoryBufferSize;
	ULONG		NumberOfHistoryBuffers;
	
	COLORREF	ColorTable[16];

	ULONG		CodePage;
	HWND		Hwnd;

	WCHAR		ConsoleTitle[0x100];

} CONSOLE_INFO;

#pragma pack(pop)

struct ConProcess {
	DWORD ProcessID, ParentPID;
	bool  IsFar;
	bool  IsTelnet; // может быть включен ВМЕСТЕ с IsFar, если удалось подцепится к фару через сетевой пайп
	bool  IsNtvdm;  // 16bit приложения
	bool  IsCmd;    // значит фар выполняет команду
	bool  NameChecked, RetryName;
	bool  Alive;
	TCHAR Name[64]; // чтобы полная инфа об ошибке влезала
};

#define MAX_SERVER_THREADS 3

class CVirtualConsole;

class CRealConsole
{
public:

	uint TextWidth() { return con.nTextWidth; };
	uint TextHeight() { return con.nTextHeight; };
	friend class CVirtualConsole;

public:
	HWND    hConWnd;


	CRealConsole(CVirtualConsole* apVCon);
	~CRealConsole();
	
	void OnFocus(BOOL abFocused);
	BOOL PreInit(BOOL abCreateBuffers=TRUE);
	void DumpConsole(HANDLE ahFile);

	bool SetConsoleSize(COORD size);
	void SendMouseEvent(UINT messg, WPARAM wParam, int x, int y);
	void SendConsoleEvent(INPUT_RECORD* piRec);
	void StopSignal();
	void StopThread(BOOL abRecreating=FALSE);
	BOOL isBufferHeight();
	LPCTSTR GetTitle();
	void GetConsoleScreenBufferInfo(CONSOLE_SCREEN_BUFFER_INFO* sbi) { *sbi = con.m_sbi; };
	void GetConsoleSelectionInfo(CONSOLE_SELECTION_INFO *sel) {	*sel = con.m_sel; };
	void GetConsoleCursorInfo(CONSOLE_CURSOR_INFO *ci) { *ci = con.m_ci; };
	DWORD GetConsoleCP() { return con.m_dwConsoleCP; };
	DWORD GetConsoleOutputCP() { return con.m_dwConsoleOutputCP; };
	DWORD GetConsoleMode() { return con.m_dwConsoleMode; };
	void SyncConsole2Window();
	void OnWinEvent(DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
	int  GetProcesses(ConProcess** ppPrc);
	DWORD GetFarPID();
	DWORD GetActiveStatus();
	DWORD GetServerPID();
	LRESULT OnKeyboard(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
	LRESULT OnScroll(int nDirection);
	bool isConSelectMode();
	bool isFar();
	void ShowConsole(int nMode); // -1 Toggle, 0 - Hide, 1 - Show
	BOOL isDetached();
	BOOL AttachConemuC(HWND ahConWnd, DWORD anConemuC_PID);
	bool RecreateProcess();
	void GetData(wchar_t* pChar, WORD* pAttr, uint nSize);
	void OnActivate(int nNewNum, int nOldNum);
	bool CheckBufferSize();
	LRESULT OnConEmuCmd(BOOL abStarted, DWORD anConEmuC_PID);

protected:
	BOOL PreCreate(BOOL abDetached);
	CVirtualConsole* mp_VCon; // соответствующая виртуальная консоль
	DWORD mn_ConEmuC_PID; HANDLE mh_ConEmuC, mh_ConEmuCInput;
	TCHAR ms_ConEmuC_Pipe[MAX_PATH], ms_ConEmuCInput_Pipe[MAX_PATH], ms_VConServer_Pipe[MAX_PATH];
	TCHAR Title[MAX_TITLE_SIZE+1], TitleCmp[MAX_TITLE_SIZE+1];

	BOOL AttachPID(DWORD dwPID);
	BOOL StartProcess();
	BOOL StartMonitorThread();

	static DWORD WINAPI MonitorThread(LPVOID lpParameter);
	HANDLE mh_VConServerThread;
	HANDLE mh_TermEvent, mh_ForceReadEvent, mh_EndUpdateEvent, mh_Sync2WindowEvent, mh_ConChanged, mh_CursorChanged;
	BOOL mb_FullRetrieveNeeded, mb_Detached;
	HANDLE mh_MonitorThread; DWORD mn_MonitorThreadID;

	void Box(LPCTSTR szText);

	BOOL RetrieveConsoleInfo(BOOL bShortOnly);
	BOOL InitBuffers(DWORD OneBufferSize);
private:
	// Эти переменные инициализируются в RetrieveConsoleInfo()
	struct {
		CRITICAL_SECTION cs; DWORD ncsT;
		CONSOLE_SELECTION_INFO m_sel;
		CONSOLE_CURSOR_INFO m_ci;
		DWORD m_dwConsoleCP, m_dwConsoleOutputCP, m_dwConsoleMode;
		CONSOLE_SCREEN_BUFFER_INFO m_sbi;
		wchar_t *pConChar;
		WORD  *pConAttr;
		int nTextWidth, nTextHeight;
		size_t nPacketIdx;
	} con;
	// 
	CRITICAL_SECTION csPRC; DWORD ncsTPRC;
	std::vector<ConProcess> m_Processes;
	int mn_ProcessCount;
	//
	DWORD mn_FarPID;
	//
	void ProcessAdd(DWORD addPID);
	void ProcessDelete(DWORD addPID);
	void ProcessUpdateFlags(BOOL abProcessChanged);
	void ProcessCheckName(struct ConProcess &ConPrc, LPWSTR asFullFileName);
	DWORD mn_ActiveStatus;
	bool isShowConsole;
	bool mb_ConsoleSelectMode;
	int BufferHeight;
	static DWORD WINAPI ServerThread(LPVOID lpvParam);
	HANDLE mh_ServerThreads[MAX_SERVER_THREADS], mh_ActiveServerThread;
	DWORD  mn_ServerThreadsId[MAX_SERVER_THREADS];
	HANDLE mh_ServerSemaphore, mh_GuiAttached;

	void ServerThreadCommand(HANDLE hPipe);
	void ApplyConsoleInfo(CESERVER_REQ* pInfo);
	void SetHwnd(HWND ahConWnd);
	WORD mn_LastVKeyPressed;
	DWORD mn_LastConReadTick;
	BOOL GetConWindowSize(const CONSOLE_SCREEN_BUFFER_INFO& sbi, int& nNewWidth, int& nNewHeight);
	int mn_Focused; //-1 после запуска, 1 - в фокусе, 0 - не в фокусе
	DWORD mn_InRecreate; // Tick, когда начали пересоздание
	BOOL mb_ProcessRestarted;
	// Логи
	BOOL mb_UseLogs;
	HANDLE mh_LogInput; wchar_t *mpsz_LogInputFile, *mpsz_LogPackets; UINT mn_LogPackets;
	void CreateLogFiles();
	void CloseLogFiles();
	void LogInput(INPUT_RECORD* pRec);
	void LogPacket(CESERVER_REQ* pInfo);
	bool RecreateProcessStart();
};
