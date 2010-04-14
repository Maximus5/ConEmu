
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
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
#include "kl_parts.h"
#include "../Common/common.hpp"
#include "../Common/ConsoleAnnotation.h"
#include "../Common/RgnDetect.h"

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

#define FAR_ALIVE_TIMEOUT gSet.nFarHourglassDelay //1000

#define CONSOLE_BLOCK_SELECTION 0x0100 // selecting text (standard mode)
#define CONSOLE_TEXT_SELECTION 0x0200 // selecting text (stream mode)

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

struct ConProcess {
    DWORD ProcessID, ParentPID; //, InputTID;
    bool  IsFar;
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


#define MAX_SERVER_THREADS 3
//#define MAX_THREAD_PACKETS 100

class CVirtualConsole;
class CRgnDetect;

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
	//HANDLE  hFileMapping;
	//CESERVER_REQ_CONINFO* pConsoleData;
	//void CloseMapping();

public:
	HWND    ConWnd();

    CRealConsole(CVirtualConsole* apVCon);
    ~CRealConsole();
    
    BOOL PreInit(BOOL abCreateBuffers=TRUE);
    void DumpConsole(HANDLE ahFile);

    BOOL SetConsoleSize(USHORT sizeX, USHORT sizeY, USHORT sizeBuffer=0, DWORD anCmdID=CECMD_SETSIZESYNC);
private:
	BOOL SetConsoleSizeSrv(USHORT sizeX, USHORT sizeY, USHORT sizeBuffer, DWORD anCmdID=CECMD_SETSIZESYNC);
private:
    //void SendConsoleEvent(INPUT_RECORD* piRec);
    DWORD mn_FlushIn, mn_FlushOut;
public:
    void PostConsoleEvent(INPUT_RECORD* piRec);
	void PostKeyPress(WORD vkKey, DWORD dwControlState, wchar_t wch, int ScanCode = -1);
	void PostKeyUp(WORD vkKey, DWORD dwControlState, wchar_t wch, int ScanCode = -1);
	void PostConsoleEventPipe(MSG *pMsg);
	BOOL OpenConsoleEventPipe();
	LRESULT PostConsoleMessage(UINT nMsg, WPARAM wParam, LPARAM lParam);
	void PostMacro(LPCWSTR asMacro);
    //BOOL FlushInputQueue(DWORD nTimeout = 500);
    void OnKeyboard(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, wchar_t *pszChars);
    void OnMouse(UINT messg, WPARAM wParam, int x, int y);
    void OnFocus(BOOL abFocused);
    
    void StopSignal();
    void StopThread(BOOL abRecreating=FALSE);
    BOOL isBufferHeight();
	HWND isPictureView(BOOL abIgnoreNonModal=FALSE);
	BOOL isWindowVisible();
    LPCTSTR GetTitle();
    void GetConsoleScreenBufferInfo(CONSOLE_SCREEN_BUFFER_INFO* sbi);
    void GetConsoleCursorPos(COORD *pcr);
    void GetConsoleCursorInfo(CONSOLE_CURSOR_INFO *ci);
    DWORD GetConsoleCP() { return con.m_dwConsoleCP; };
    DWORD GetConsoleOutputCP() { return con.m_dwConsoleOutputCP; };
    DWORD GetConsoleMode() { return con.m_dwConsoleMode; };
    void SyncConsole2Window();
    void OnWinEvent(DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
    int  GetProcesses(ConProcess** ppPrc);
    DWORD GetFarPID(BOOL abPluginRequired=FALSE);
    DWORD GetProgramStatus();
	DWORD GetFarStatus();
    DWORD GetServerPID();
    LRESULT OnScroll(int nDirection);
	LRESULT OnSetScrollPos(WPARAM wParam);
    bool GetConsoleSelectionInfo(CONSOLE_SELECTION_INFO *sel);
    BOOL isConSelectMode();
	bool isSelectionAllowed();
	bool isSelectionPresent();
	void StartSelection(BOOL abTextMode, SHORT anX=-1, SHORT anY=-1, BOOL abByMouse=FALSE);
	void ExpandSelection(SHORT anX=-1, SHORT anY=-1);
	bool DoSelectionCopy();
    BOOL isFar(BOOL abPluginRequired=FALSE);
    void ShowConsole(int nMode); // -1 Toggle, 0 - Hide, 1 - Show
    BOOL isDetached();
    BOOL AttachConemuC(HWND ahConWnd, DWORD anConemuC_PID, CONSOLE_SCREEN_BUFFER_INFO sbi, CESERVER_REQ_STARTSTOPRET* pRet);
    BOOL RecreateProcess(RConStartArgs *args);
    void GetConsoleData(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight);
	BOOL IsConsoleDataChanged();
    void OnActivate(int nNewNum, int nOldNum);
    void OnDeactivate(int nNewNum);
	void OnGuiFocused(BOOL abFocus);
    BOOL CheckBufferSize();
    //LRESULT OnConEmuCmd(BOOL abStarted, DWORD anConEmuC_PID);
    BOOL BufferHeightTurnedOn(CONSOLE_SCREEN_BUFFER_INFO* psbi);
    void UpdateScrollInfo();
    void SetTabs(ConEmuTab* tabs, int tabsCount);
    BOOL GetTab(int tabIdx, /*OUT*/ ConEmuTab* pTab);
    int GetTabCount();
    int GetModifiedEditors();
    BOOL ActivateFarWindow(int anWndIndex);
    DWORD CanActivateFarWindow(int anWndIndex);
    void SwitchKeyboardLayout(WPARAM wParam,DWORD_PTR dwNewKeybLayout);
    void CloseConsole();
    void Paste();
    void LogString(LPCSTR asText, BOOL abShowTime = FALSE);
	bool isActive();
	bool isFilePanel(bool abPluginAllowed=false);
	bool isEditor();
	bool isEditorModified();
	bool isViewer();
	bool isVisible();
	bool isNtvdm();
	//bool isPackets();
	LPCWSTR GetCmd();
	LPCWSTR GetDir();
	BOOL GetUserPwd(const wchar_t** ppszUser, const wchar_t** ppszPwd, BOOL* pbRestricted);
	short GetProgress(BOOL* rpbError);
	void UpdateFarSettings(DWORD anFarPID=0);
	int CoordInPanel(COORD cr);
	BOOL GetPanelRect(BOOL abRight, RECT* prc, BOOL abFull = FALSE);
	bool isAdministrator();
	BOOL isMouseButtonDown();
	void OnConsoleLangChange(DWORD_PTR dwNewKeybLayout);
	DWORD GetConsoleStates();
    void ChangeBufferHeightMode(BOOL abBufferHeight); // Вызывается из TabBar->ConEmu
	void RemoveFromCursor();
	bool isAlive();
	bool GetMaxConSize(COORD* pcrMaxConSize);
	int GetDetectedDialogs(int anMaxCount, SMALL_RECT* rc, DWORD* rf);

public:
    // Вызываются из CVirtualConsole
    BOOL PreCreate(RConStartArgs *args);
		//(BOOL abDetached, LPCWSTR asNewCmd = NULL, BOOL abAsAdmin = FALSE);
    BOOL IsConsoleThread();
    void SetForceRead();
    //DWORD WaitEndUpdate(DWORD dwTimeout=1000);
    LPCWSTR GetConStatus();
	void UpdateCursorInfo();
	void Detach();
	void AdminDuplicate();

protected:
    CVirtualConsole* mp_VCon; // соответствующая виртуальная консоль
    DWORD mn_ConEmuC_PID/*, mn_ConEmuC_Input_TID*/; HANDLE mh_ConEmuC, mh_ConEmuCInput;
	BOOL mb_UseOnlyPipeInput;
    TCHAR ms_ConEmuC_Pipe[MAX_PATH], ms_ConEmuCInput_Pipe[MAX_PATH], ms_VConServer_Pipe[MAX_PATH];
    // Текущий заголовок консоли и его значение для сравнения (для определения изменений)
    WCHAR Title[MAX_TITLE_SIZE+1], TitleCmp[MAX_TITLE_SIZE+1];
    // А здесь содержится то, что отображается в ConEmu (может быть добавлено " (Admin)")
    WCHAR TitleFull[MAX_TITLE_SIZE+96];
	// Принудительно дернуть OnTitleChanged, например, при изменении процентов в консоли
	BOOL mb_ForceTitleChanged;
    // Здесь сохраняется заголовок окна (с панелями), когда FAR фокус с панелей уходит (переходит в редактор...).
    WCHAR ms_PanelTitle[CONEMUTABMAX];
    // Процентики
    short mn_Progress, mn_PreWarningProgress, mn_ConsoleProgress;
    short CheckProgressInTitle();
    short CheckProgressInConsole(const wchar_t* pszCurLine);

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
    

    void Box(LPCTSTR szText);

    //BOOL RetrieveConsoleInfo(/*BOOL bShortOnly,*/ UINT anWaitSize);
	BOOL WaitConsoleSize(UINT anWaitSize, DWORD nTimeout);
    BOOL InitBuffers(DWORD OneBufferSize);
	BOOL LoadDataFromMap(DWORD CharCount);
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
		CESERVER_REQ_CONINFO_DATA *pCopy, *pCmp;
        int nTextWidth, nTextHeight, nBufferHeight;
        int nChange2TextWidth, nChange2TextHeight;
        BOOL bBufferHeight; // TRUE, если есть прокрутка
        //DWORD nPacketIdx;
        DWORD_PTR dwKeybLayout;
        BOOL bRBtnDrag; // в консоль посылается драг правой кнопкой (выделение в FAR)
        COORD crRBtnDrag;
		BOOL bInSetSize; HANDLE hInSetSize;
		int DefaultBufferHeight;
		BOOL bConsoleDataChanged;

		#ifdef _DEBUG
		BOOL bDebugLocked;
		#endif
    } con;
    // 
    MSection csPRC; //DWORD ncsTPRC;
    std::vector<ConProcess> m_Processes;
    int mn_ProcessCount;
    //
    DWORD mn_FarPID, /*mn_FarInputTID,*/ mn_LastSetForegroundPID;
    //
    ConEmuTab* mp_tabs;
	MSection   msc_Tabs;
    int mn_tabsCount, mn_MaxTabs, mn_ActiveTab;
	BOOL mb_TabsWasChanged;
    void CheckPanelTitle();
    //
    //void ProcessAdd(DWORD addPID);
    //void ProcessDelete(DWORD addPID);
    void ProcessUpdate(const DWORD *apPID, UINT anCount);
    void ProcessUpdateFlags(BOOL abProcessChanged);
    void ProcessCheckName(struct ConProcess &ConPrc, LPWSTR asFullFileName);
    DWORD mn_ProgramStatus, mn_FarStatus;
	BOOL mb_IgnoreCmdStop; // При запуске 16bit приложения не возвращать размер консоли! Это сделает OnWinEvent
    BOOL isShowConsole;
    BOOL mb_FarGrabberActive; // бывший mb_ConsoleSelectMode
    WORD mn_SelectModeSkipVk; // пропустить "отпускание" клавиши Esc/Enter при выделении текста
    bool OnMouseSelection(UINT messg, WPARAM wParam, int x, int y);
    void UpdateSelection(); // обновить на экране
    static DWORD WINAPI ServerThread(LPVOID lpvParam);
    HANDLE mh_ServerThreads[MAX_SERVER_THREADS], mh_ActiveServerThread;
    DWORD  mn_ServerThreadsId[MAX_SERVER_THREADS];
    HANDLE mh_ServerSemaphore, mh_GuiAttached;
    void SetBufferHeightMode(BOOL abBufferHeight, BOOL abLock=FALSE);
    BOOL mb_BuferModeChangeLocked;

    void ServerThreadCommand(HANDLE hPipe);
    //void ApplyConsoleInfo(CESERVER_REQ* pInfo);
    void SetHwnd(HWND ahConWnd);
    WORD mn_LastVKeyPressed;
    BOOL GetConWindowSize(const CONSOLE_SCREEN_BUFFER_INFO& sbi, int& nNewWidth, int& nNewHeight, BOOL* pbBufferHeight=NULL);
    int mn_Focused; //-1 после запуска, 1 - в фокусе, 0 - не в фокусе
    DWORD mn_InRecreate; // Tick, когда начали пересоздание
    BOOL mb_ProcessRestarted;
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
	HANDLE mh_FileMapping, mh_FileMappingData;
	wchar_t ms_HeaderMapName[64], ms_DataMapName[64];
	const CESERVER_REQ_CONINFO_HDR *mp_ConsoleInfo;
	const CESERVER_REQ_CONINFO_DATA *mp_ConsoleData; // Mapping
	// Colorer Mapping
	HANDLE mh_ColorMapping;
	AnnotationHeader *mp_ColorHdr;
	AnnotationInfo *mp_ColorData;
	DWORD mn_LastColorFarID;
	void OpenColorMapping(); // Открыть мэппинг колорера (по HWND)
	void CheckColorMapping(DWORD dwPID); // Проверить валидность буфера - todo
	void CloseColorMapping();
	//
	DWORD mn_LastConsoleDataIdx, mn_LastConsolePacketIdx, mn_LastFarReadIdx;
	DWORD mn_LastFarReadTick;
	BOOL OpenMapHeader();
	void CloseMapData();
	BOOL ReopenMapData();
	void CloseMapHeader();
	void ApplyConsoleInfo();
	BOOL mb_DataChanged;
	void OnServerStarted();
    //
    BOOL PrepareOutputFile(BOOL abUnicodeText, wchar_t* pszFilePathName);
    HANDLE PrepareOutputFileCreate(wchar_t* pszFilePathName);
    // фикс для dblclick в редакторе
    MOUSE_EVENT_RECORD m_LastMouse;
	//
	wchar_t ms_Editor[32], ms_EditorRus[32], ms_Viewer[32], ms_ViewerRus[32];
	wchar_t ms_TempPanel[32], ms_TempPanelRus[32];
	//
	BOOL mb_PluginDetected; DWORD mn_FarPID_PluginDetected; //, mn_Far_PluginInputThreadId;
	void CheckFarStates();
	void OnTitleChanged();
	DWORD mn_LastInvalidateTick;
	//
	HWND hPictureView; BOOL mb_PicViewWasHidden;
	// координаты панелей в символах
	RECT mr_LeftPanel, mr_RightPanel, mr_LeftPanelFull, mr_RightPanelFull; BOOL mb_LeftPanel, mb_RightPanel;
	void FindPanels();
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
	CRgnDetect* mp_Rgn;
	//int mn_DetectCallCount;
	void PrepareTransparent(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight);
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
