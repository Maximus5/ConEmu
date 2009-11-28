
#ifdef _DEBUG
//  Раскомментировать, чтобы сразу после загрузки плагина показать MessageBox, чтобы прицепиться дебаггером
//  #define SHOW_STARTED_MSGBOX
#endif

//#include <stdio.h>
#include <windows.h>
//#include <windowsx.h>
//#include <string.h>
//#include <tchar.h>
#include "..\common\common.hpp"
#include "..\common\pluginW1007.hpp"
#include "PluginHeader.h"
#include <Tlhelp32.h>
#include <vector>

WARNING("Перевести ghCommandThreads с vector<> на обычный класс. Это позволит собрать с минимальным размером файла");

#include "../common/ConEmuCheck.h"

#define Free free
#define Alloc calloc

#define MAKEFARVERSION(major,minor,build) ( ((major)<<8) | (minor) | ((build)<<16))

#define ConEmu_SysID 0x43454D55 // 'CEMU'
#define SETWND_CALLPLUGIN_SENDTABS 100
#define SETWND_CALLPLUGIN_BASE (SETWND_CALLPLUGIN_SENDTABS+1)

#ifdef _DEBUG
wchar_t gszDbgModLabel[6] = {0};
#endif

#if defined(__GNUC__)
extern "C"{
  BOOL WINAPI DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved );
  HWND WINAPI GetFarHWND();
  HWND WINAPI GetFarHWND2(BOOL abConEmuOnly);
  void WINAPI GetFarVersion ( FarVersion* pfv );
  int  WINAPI ProcessEditorInputW(void* Rec);
  void WINAPI SetStartupInfoW(void *aInfo);
  int  WINAPI ProcessSynchroEventW(int Event,void *Param);
  BOOL WINAPI IsTerminalMode();
  BOOL WINAPI IsConsoleActive();
};
#endif


HMODULE ghPluginModule = NULL; // ConEmu.dll - сам плагин
HWND ConEmuHwnd = NULL; // Содержит хэндл окна отрисовки. Это ДОЧЕРНЕЕ окно.
DWORD gdwServerPID = 0;
BOOL TerminalMode = FALSE;
HWND FarHwnd = NULL;
HANDLE ghConIn = NULL;
DWORD gnMainThreadId = 0;
//HANDLE hEventCmd[MAXCMDCOUNT], hEventAlive=NULL, hEventReady=NULL;
HANDLE ghMonitorThread = NULL; DWORD gnMonitorThreadId = 0;
HANDLE ghInputThread = NULL; DWORD gnInputThreadId = 0;
HANDLE ghSetWndSendTabsEvent = NULL;
FarVersion gFarVersion;
WCHAR gszDir1[CONEMUTABMAX], gszDir2[CONEMUTABMAX];
WCHAR gszRootKey[MAX_PATH*2];
int maxTabCount = 0, lastWindowCount = 0, gnCurTabCount = 0;
CESERVER_REQ* gpTabs = NULL; //(ConEmuTab*) Alloc(maxTabCount, sizeof(ConEmuTab));
LPBYTE gpData = NULL, gpCursor = NULL;
CESERVER_REQ* gpCmdRet = NULL;
DWORD  gnDataSize=0;
//HANDLE ghMapping = NULL;
DWORD gnReqCommand = -1;
int gnPluginOpenFrom = -1;
HANDLE ghInputSynchroExecuted = NULL;
BOOL gbCmdCallObsolete = FALSE;
LPVOID gpReqCommandData = NULL;
HANDLE ghReqCommandEvent = NULL;
UINT gnMsgTabChanged = 0;
CRITICAL_SECTION csData;
MSection csTabs;
WCHAR gcPlugKey=0;
BOOL  gbPlugKeyChanged=FALSE;
HKEY  ghRegMonitorKey=NULL; HANDLE ghRegMonitorEvt=NULL;
HMODULE ghFarHintsFix = NULL;
WCHAR gszPluginServerPipe[MAX_PATH];
#define MAX_SERVER_THREADS 3
//HANDLE ghServerThreads[MAX_SERVER_THREADS] = {NULL,NULL,NULL};
//HANDLE ghActiveServerThread = NULL;
HANDLE ghServerThread = NULL;
DWORD  gnServerThreadId = 0;
std::vector<HANDLE> ghCommandThreads;
//DWORD  gnServerThreadsId[MAX_SERVER_THREADS] = {0,0,0};
HANDLE ghServerTerminateEvent = NULL;
HANDLE ghPluginSemaphore = NULL;
wchar_t gsFarLang[64] = {0};
BOOL FindServerCmd(DWORD nServerCmd, DWORD &dwServerPID);
BOOL gbNeedPostTabSend = FALSE;
DWORD gnNeedPostTabSendTick = 0;
#define NEEDPOSTTABSENDDELTA 100
wchar_t gsMonitorEnvVar[0x1000];
bool gbMonitorEnvVar = false;
#define MONITORENVVARDELTA 1000
void UpdateEnvVar(const wchar_t* pszList);
BOOL StartupHooks();
BOOL gbFARuseASCIIsort = FALSE; // попытаться перехватить строковую сортировку в FAR


// minimal(?) FAR version 2.0 alpha build FAR_X_VER
int WINAPI _export GetMinFarVersionW(void)
{
	// Необходимо наличие Synchro
#if FAR_X_VER<1007
	return MAKEFARVERSION(2,0,1007);
#else
	return MAKEFARVERSION(2,0,FAR_X_VER);
#endif
}

/* COMMON - пока структуры не различаются */
void WINAPI _export GetPluginInfoW(struct PluginInfo *pi)
{
    static WCHAR *szMenu[1], szMenu1[255];
    szMenu[0]=szMenu1; //lstrcpyW(szMenu[0], L"[&\x2560] ConEmu"); -> 0x2584
    //szMenu[0][1] = L'&';
    //szMenu[0][2] = 0x2560;

	// Проверить, не изменилась ли горячая клавиша плагина, и если да - пересоздать макросы
	//IsKeyChanged(TRUE); -- в FAR2 устарело, используем Synchro
	//if (gcPlugKey) szMenu1[0]=0; else lstrcpyW(szMenu1, L"[&\x2584] ");
	//lstrcpynW(szMenu1+lstrlenW(szMenu1), GetMsgW(2), 240);
	lstrcpynW(szMenu1, GetMsgW(2), 240);


	pi->StructSize = sizeof(struct PluginInfo);
	pi->Flags = PF_EDITOR | PF_VIEWER | PF_DIALOG | PF_PRELOAD;
	pi->DiskMenuStrings = NULL;
	pi->DiskMenuNumbers = 0;
	pi->PluginMenuStrings = szMenu;
	pi->PluginMenuStringsNumber = 1;
	pi->PluginConfigStrings = NULL;
	pi->PluginConfigStringsNumber = 0;
	pi->CommandPrefix = NULL;
	pi->Reserved = ConEmu_SysID; // 'CEMU'
}


BOOL gbInfoW_OK = FALSE;
HANDLE WINAPI _export OpenPluginW(int OpenFrom,INT_PTR Item)
{
	if (!gbInfoW_OK)
		return INVALID_HANDLE_VALUE;

	if (gnReqCommand != (DWORD)-1) {
		gnPluginOpenFrom = (OpenFrom && 0xFFFF);
		ProcessCommand(gnReqCommand, FALSE/*bReqMainThread*/, gpReqCommandData);
	} else {
		if (!gbCmdCallObsolete) {
			INT_PTR nID = -1; // выбор из меню
			if ((OpenFrom & OPEN_FROMMACRO) == OPEN_FROMMACRO)
			{
				if (Item >= 1 && Item <= 8)
				{
					nID = Item - 1; // Будет сразу выполнена команда
					
				} else if (Item >= SETWND_CALLPLUGIN_BASE) {
					gnPluginOpenFrom = OPEN_PLUGINSMENU;
					DWORD nTab = (DWORD)(Item - SETWND_CALLPLUGIN_BASE);
					ProcessCommand(CMD_SETWINDOW, FALSE, &nTab);
					SetEvent(ghSetWndSendTabsEvent);
					return INVALID_HANDLE_VALUE;
					
				} else if (Item == SETWND_CALLPLUGIN_SENDTABS) {
					// Force Send tabs to ConEmu
					//MSectionLock SC; SC.Lock(&csTabs, TRUE);
					//SendTabs(gnCurTabCount, TRUE);
					//SC.Unlock();
					SetEvent(ghSetWndSendTabsEvent);
					return INVALID_HANDLE_VALUE;
				}
			}
			ShowPluginMenu((int)nID);
		} else {
			gbCmdCallObsolete = FALSE;
		}
	}
	return INVALID_HANDLE_VALUE;
}

int WINAPI _export ProcessSynchroEventW(int Event,void *Param)
{
	if (Event == SE_COMMONSYNCHRO) {
    	if (!gbInfoW_OK) {
    		if (Param) free(Param);
    		return 0;
		}

		SynchroArg *pArg = (SynchroArg*)Param;
		_ASSERTE(pArg!=NULL);

		// Если предыдущая активация отвалилась по таймауту - не выполнять
		if (pArg->Obsolete) {
			free(pArg);
			return 0;
		}

		if (pArg->SynchroType == SynchroArg::eCommand) {
    		if (gnReqCommand != (DWORD)-1) {
    			_ASSERTE(gnReqCommand==(DWORD)-1);
    		} else {
    			TODO("Определить текущую область... (panel/editor/viewer/menu/...");
    			gnPluginOpenFrom = 0;
    			gnReqCommand = (DWORD)pArg->Param1;
				gpReqCommandData = (LPVOID)pArg->Param2;
				
				if (gnReqCommand == CMD_SETWINDOW) {
					// Необходимо быть в panel/editor/viewer
					wchar_t szMacro[255];
					DWORD nTabShift = SETWND_CALLPLUGIN_BASE + *((DWORD*)gpReqCommandData);
					
					// Если панели-редактор-вьювер - сменить окно. Иначе - отослать в GUI табы
					wsprintf(szMacro, L"$if (Shell||Viewer||Editor) callplugin(0x%08X,%i) $else callplugin(0x%08X,%i) $end",
						ConEmu_SysID, nTabShift, ConEmu_SysID, SETWND_CALLPLUGIN_SENDTABS);
						
					gnReqCommand = -1;
					gpReqCommandData = NULL;
					PostMacro(szMacro);
					// Done
				} else {
	    			ProcessCommand(gnReqCommand, FALSE/*bReqMainThread*/, gpReqCommandData);
    			}
    		}
		} else if (pArg->SynchroType == SynchroArg::eInput) {
			INPUT_RECORD *pRec = (INPUT_RECORD*)(pArg->Param1);
			UINT nCount = (UINT)pArg->Param2;

			if (nCount>0) {
				DWORD cbWritten = 0;
				_ASSERTE(ghConIn);
				BOOL fSuccess = WriteConsoleInput(ghConIn, pRec, nCount, &cbWritten);
				if (!fSuccess || cbWritten != nCount) {
					_ASSERTE(fSuccess && cbWritten==nCount);
				}
			}
		}

		if (pArg->hEvent)
			SetEvent(pArg->hEvent);
		//pArg->Processed = TRUE;
		//pArg->Executed = TRUE;
		free(pArg);
	}
	return 0;
}

/* COMMON - end */


//#if defined(__GNUC__)
//typedef HWND (APIENTRY *FGetConsoleWindow)();
//FGetConsoleWindow GetConsoleWindow = NULL;
//#endif
//extern void SetConsoleFontSizeTo(HWND inConWnd, int inSizeX, int inSizeY);


BOOL WINAPI DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
			{
				ghPluginModule = (HMODULE)hModule;
				
				_ASSERTE(FAR_X_VER<=FAR_Y_VER);
				#ifdef SHOW_STARTED_MSGBOX
				if (!IsDebuggerPresent()) MessageBoxA(NULL, "ConEmu*.dll loaded", "ConEmu plugin", 0);
				#endif
				//#if defined(__GNUC__)
				//GetConsoleWindow = (FGetConsoleWindow)GetProcAddress(GetModuleHandle(L"kernel32.dll"),"GetConsoleWindow");
				//#endif
				
				gpNullSecurity = NullSecurity();
				
				HWND hConWnd = GetConsoleWindow();
				gnMainThreadId = GetCurrentThreadId();
				InitHWND(hConWnd);
				
				if (!StartupHooks()) {
					_ASSERT(FALSE);
					OutputDebugString(L"!!! Can't install injects!!!\n");
				}
				
			    // Check Terminal mode
			    TCHAR szVarValue[MAX_PATH];
			    szVarValue[0] = 0;
			    if (GetEnvironmentVariable(L"TERM", szVarValue, 63)) {
				    TerminalMode = TRUE;
			    }
			    
			    if (!TerminalMode) {
					// FarHints fix for multiconsole mode...
					if (GetModuleFileName((HMODULE)hModule, szVarValue, MAX_PATH)) {
						WCHAR *pszSlash = wcsrchr(szVarValue, L'\\');
						if (pszSlash) pszSlash++; else pszSlash = szVarValue;
						lstrcpyW(pszSlash, L"infis.dll");
						ghFarHintsFix = LoadLibrary(szVarValue);
					}
			    }
			}
			break;
		case DLL_PROCESS_DETACH:
			if (ghFarHintsFix) {
				FreeLibrary(ghFarHintsFix);
				ghFarHintsFix = NULL;
			}
			break;
	}
	return TRUE;
}

BOOL WINAPI IsConsoleActive()
{
	if (ConEmuHwnd) {
		if (IsWindow(ConEmuHwnd)) {
			HWND hParent = GetParent(ConEmuHwnd);
			if (hParent) {
				HWND hTest = (HWND)GetWindowLongPtr(hParent, GWLP_USERDATA);
				return (hTest == FarHwnd);
			}
		}
	}
	return TRUE;
}

HWND WINAPI GetFarHWND2(BOOL abConEmuOnly)
{
	if (ConEmuHwnd) {
		if (IsWindow(ConEmuHwnd))
			return ConEmuHwnd;
		ConEmuHwnd = NULL;
		//
		SetConEmuEnvVar(NULL);
	}
	if (abConEmuOnly)
		return NULL;
	return FarHwnd;
}

HWND WINAPI _export GetFarHWND()
{
    return GetFarHWND2(FALSE);
}

BOOL WINAPI IsTerminalMode()
{
    return TerminalMode;
}

void WINAPI _export GetFarVersion ( FarVersion* pfv )
{
	if (!pfv)
		return;

	*pfv = gFarVersion;
}

BOOL LoadFarVersion()
{
    BOOL lbRc=FALSE;
    WCHAR FarPath[MAX_PATH+1];
    if (GetModuleFileName(0,FarPath,MAX_PATH)) {
		DWORD dwRsrvd = 0;
		DWORD dwSize = GetFileVersionInfoSize(FarPath, &dwRsrvd);
		if (dwSize>0) {
			void *pVerData = Alloc(dwSize, 1);
			if (pVerData) {
				VS_FIXEDFILEINFO *lvs = NULL;
				UINT nLen = sizeof(lvs);
				if (GetFileVersionInfo(FarPath, 0, dwSize, pVerData)) {
					TCHAR szSlash[3]; lstrcpyW(szSlash, L"\\");
					if (VerQueryValue ((void*)pVerData, szSlash, (void**)&lvs, &nLen)) {
						gFarVersion.dwVer = lvs->dwFileVersionMS;
						gFarVersion.dwBuild = lvs->dwFileVersionLS;
						lbRc = TRUE;
					}
				}
				Free(pVerData);
			}
		}
	}

	if (!lbRc) {
		gFarVersion.dwVerMajor = 2;
		gFarVersion.dwVerMinor = 0;
		gFarVersion.dwBuild = FAR_X_VER;
	}

	return lbRc;
}

BOOL IsKeyChanged(BOOL abAllowReload)
{
	BOOL lbKeyChanged = FALSE;
	if (ghRegMonitorEvt) {
		if (WaitForSingleObject(ghRegMonitorEvt, 0) == WAIT_OBJECT_0) {
			lbKeyChanged = CheckPlugKey();
			if (lbKeyChanged) gbPlugKeyChanged = TRUE;
		}
	}

	if (abAllowReload && gbPlugKeyChanged) {
		// Вообще-то его бы вызывать в главной нити...
		CheckMacro(TRUE);
		gbPlugKeyChanged = FALSE;
	}
	return lbKeyChanged;
}


BOOL ActivatePluginA(DWORD nCmd, LPVOID pCommandData)
{
	BOOL lbRc = FALSE;
	gnReqCommand = nCmd; gnPluginOpenFrom = -1;
	gpReqCommandData = pCommandData;
	ResetEvent(ghReqCommandEvent);

	// Проверить, не изменилась ли горячая клавиша плагина, и если да - пересоздать макросы
	IsKeyChanged(TRUE);

	INPUT_RECORD evt[10];
	DWORD dwStartWait, dwCur, dwTimeout, dwInputs = 0, dwInputsFirst = 0;
	//BOOL  lbInputs = FALSE;
	HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);

	BOOL lbInput = PeekConsoleInput(hInput, evt, 10, &dwInputsFirst);

	// Нужен вызов плагина в остновной нити
	WARNING("Переделать на WriteConsoleInput");
	gbCmdCallObsolete = FALSE;
	SendMessage(FarHwnd, WM_KEYDOWN, VK_F14, 0);
	SendMessage(FarHwnd, WM_KEYUP, VK_F14, (LPARAM)(3<<30));

	
	DWORD dwWait;
	HANDLE hEvents[2];
	hEvents[0] = ghReqCommandEvent;
	hEvents[1] = ghServerTerminateEvent;

	dwTimeout = CONEMUFARTIMEOUT;
	dwStartWait = GetTickCount();
	//DuplicateHandle(GetCurrentProcess(), ghReqCommandEvent, GetCurrentProcess(), hEvents, 0, 0, DUPLICATE_SAME_ACCESS);
	do {
		dwWait = WaitForMultipleObjects ( 2, hEvents, FALSE, 200 );
		if (dwWait == WAIT_TIMEOUT) {
			lbInput = PeekConsoleInput(hInput, evt, 10, &dwInputs);
			if (lbInput && dwInputs == 0) {
				//Раз из буфера ввода убралось "Отпускание F14" - значит ловить уже нечего, выходим
				gbCmdCallObsolete = TRUE; // иначе может всплыть меню, когда не надо 
				break;
			}

			dwCur = GetTickCount();
			if ((dwCur - dwStartWait) > dwTimeout)
				break;
		}
	} while (dwWait == WAIT_TIMEOUT);
	if (dwWait)
		ResetEvent(ghReqCommandEvent); // Сразу сбросим, вдруг не дождались?
	else
		lbRc = TRUE;

	gpReqCommandData = NULL;
	gnReqCommand = -1; gnPluginOpenFrom = -1;

	return lbRc;
}

BOOL ActivatePluginW(DWORD nCmd, LPVOID pCommandData, DWORD nTimeout = CONEMUFARTIMEOUT)
{
	BOOL lbRc = FALSE;
	gnReqCommand = -1; gnPluginOpenFrom = -1; gpReqCommandData = NULL;
	ResetEvent(ghReqCommandEvent);

	// Нужен вызов плагина в остновной нити
	gbCmdCallObsolete = FALSE;

	SynchroArg *Param = (SynchroArg*)calloc(sizeof(SynchroArg),1);
	Param->SynchroType = SynchroArg::eCommand;
	Param->Param1 = nCmd;
	Param->Param2 = (LPARAM)pCommandData;
	Param->hEvent = ghReqCommandEvent;
	Param->Obsolete = FALSE;

	lbRc = CallSynchro995(Param, nTimeout);
	
	if (!lbRc)
		ResetEvent(ghReqCommandEvent); // Сразу сбросим, вдруг не дождались?

	gpReqCommandData = NULL;
	gnReqCommand = -1; gnPluginOpenFrom = -1;

	return lbRc;
}

typedef HANDLE (WINAPI *OpenPlugin_t)(int OpenFrom,INT_PTR Item);

WARNING("Обязательно сделать возможность отваливаться по таймауту, если плагин не удалось активировать");
// Проверку можно сделать чтением буфера ввода - если там еще есть событие отпускания F11 - значит
// меню плагинов еще загружается. Иначе можно еще чуть-чуть подождать, и отваливаться - активироваться не получится
CESERVER_REQ* ProcessCommand(DWORD nCmd, BOOL bReqMainThread, LPVOID pCommandData)
{
	CESERVER_REQ* pCmdRet = NULL;
	if (gpCmdRet) Free(gpCmdRet);
	gpCmdRet = NULL; gpData = NULL; gpCursor = NULL;

	WARNING("Тут нужно сделать проверку содержимого консоли");
	// Если отображено меню - плагин не запустится
	// Не перепутать меню с пустым экраном (Ctrl-O)

	if (bReqMainThread && (gnMainThreadId != GetCurrentThreadId())) {
		_ASSERTE(ghPluginSemaphore!=NULL);
		_ASSERTE(ghServerTerminateEvent!=NULL);

		// Некоторые команды можно выполнить сразу
		if (nCmd == CMD_SETSIZE) {
			DWORD nHILO = *((DWORD*)pCommandData);
			SHORT nWidth = LOWORD(nHILO);
			SHORT nHeight = HIWORD(nHILO);
			MConHandle hConOut ( L"CONOUT$" );
			CONSOLE_SCREEN_BUFFER_INFO csbi = {{0,0}};
			BOOL lbRc = GetConsoleScreenBufferInfo(hConOut, &csbi);
			hConOut.Close();
			if (lbRc) {
				// Если размер консоли менять вообще не нужно
				if (csbi.dwSize.X == nWidth && csbi.dwSize.Y == nHeight) {
					OutDataAlloc(sizeof(nHILO));
					OutDataWrite(&nHILO, sizeof(nHILO));
					return gpCmdRet;
				}
			}
		}

		// Засемафорить, чтобы несколько команд одновременно не пошли...
		HANDLE hEvents[2] = {ghServerTerminateEvent, ghPluginSemaphore};
		DWORD dwWait = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
		if (dwWait == WAIT_OBJECT_0) {
			// Плагин завершается
			return NULL;
		}

		if (gFarVersion.dwVerMajor == 2 /*&& gFarVersion.dwBuild >= 1007*/)
		//	&& nCmd != CMD_SETWINDOW)
		{
			// Пока ProcessSynchroEventW не научится определять текущую макрообласть - переключение окон по старому
			ActivatePluginW(nCmd, pCommandData);
		} else {
			ActivatePluginA(nCmd, pCommandData);
		}

		ReleaseSemaphore(ghPluginSemaphore, 1, NULL);
		
		if (nCmd == CMD_LEFTCLKSYNC) {
			DWORD nTestEvents = 0, dwTicks = GetTickCount();
			GetNumberOfConsoleInputEvents(ghConIn, &nTestEvents);
			while (nTestEvents > 0 && (dwTicks - GetTickCount()) < 300) {
				Sleep(10);
				GetNumberOfConsoleInputEvents(ghConIn, &nTestEvents);
			}
		}

		//gpReqCommandData = NULL;
		//gnReqCommand = -1; gnPluginOpenFrom = -1;
		return gpCmdRet; // Результат выполнения команды
	}
	
	/*if (gbPlugKeyChanged) {
		gbPlugKeyChanged = FALSE;
		CheckMacro(TRUE);
		gbPlugKeyChanged = FALSE;
	}*/

	EnterCriticalSection(&csData);

	switch (nCmd)
	{
		case (CMD_DRAGFROM):
		{
			if (gFarVersion.dwVerMajor==1) {
				ProcessDragFromA();
				ProcessDragToA();
			} else if (gFarVersion.dwBuild>=FAR_Y_VER) {
				FUNC_Y(ProcessDragFrom)();
				FUNC_Y(ProcessDragTo)();
			} else {
				FUNC_X(ProcessDragFrom)();
				FUNC_X(ProcessDragTo)();
			}
			break;
		}
		case (CMD_DRAGTO):
		{
			if (gFarVersion.dwVerMajor==1)
				ProcessDragToA();
			else if (gFarVersion.dwBuild>=FAR_Y_VER)
				FUNC_Y(ProcessDragTo)();
			else
				FUNC_X(ProcessDragTo)();
			break;
		}
		case (CMD_SETWINDOW):
		{
			int nTab = 0;
			// Окно мы можем сменить только если:
			if (gnPluginOpenFrom == OPEN_VIEWER || gnPluginOpenFrom == OPEN_EDITOR 
				|| gnPluginOpenFrom == OPEN_PLUGINSMENU)
			{
				_ASSERTE(pCommandData!=NULL);
				if (pCommandData!=NULL)
					nTab = *((DWORD*)pCommandData);

				if (gFarVersion.dwVerMajor==1) {
					SetWindowA(nTab);
				} else {
					if (gFarVersion.dwBuild>=FAR_Y_VER)
						FUNC_Y(SetWindow)(nTab);
					else
						FUNC_X(SetWindow)(nTab);
				}
			}
			//SendTabs(gnCurTabCount, FALSE); // Обновить размер передаваемых данных
			pCmdRet = gpTabs;
			break;
		}
		case (CMD_POSTMACRO):
		{
			_ASSERTE(pCommandData!=NULL);
			if (pCommandData!=NULL)
				PostMacro((wchar_t*)pCommandData);
			break;
		}
		case (CMD_LEFTCLKSYNC):
		{
			COORD *crMouse = (COORD *)pCommandData;
			
			INPUT_RECORD clk[2] = {{MOUSE_EVENT},{MOUSE_EVENT}};
			clk[0].Event.MouseEvent.dwButtonState = FROM_LEFT_1ST_BUTTON_PRESSED;
			clk[0].Event.MouseEvent.dwMousePosition = *crMouse;
			clk[1].Event.MouseEvent.dwMousePosition = *crMouse;
			
			DWORD cbWritten = 0;
			if (!ghConIn)
				ghConIn  = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
					0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
			_ASSERTE(ghConIn);
			BOOL fSuccess = WriteConsoleInput(ghConIn, clk, 2, &cbWritten);
			if (!fSuccess || cbWritten != 2) {
				_ASSERTE(fSuccess && cbWritten==2);
			}
			break;
		}
		case (CMD_EMENU): //RMENU
		{
			COORD *crMouse = (COORD *)pCommandData;
			const wchar_t *pszUserMacro = (wchar_t*)(crMouse+1);
			
			// Чтобы на чистой системе менюшка всплывала под курсором и не выскакивало сообщение ""
			HKEY hRClkKey = NULL;
			DWORD disp = 0;
			WCHAR szEMenuKey[MAX_PATH*2+64];
			lstrcpyW(szEMenuKey, gszRootKey);
			lstrcatW(szEMenuKey, L"\\Plugins\\RightClick");
			// Ключа может и не быть, если настройки ни разу не сохранялись
			if (0 == RegCreateKeyExW(HKEY_CURRENT_USER, szEMenuKey, 0, 0, 0, KEY_ALL_ACCESS, 0, &hRClkKey, &disp))
			{
				if (disp == REG_CREATED_NEW_KEY) {
					RegSetValueExW(hRClkKey, L"WaitToContinue", 0, REG_DWORD, (LPBYTE)&(disp = 0), sizeof(disp));
					RegSetValueExW(hRClkKey, L"GuiPos", 0, REG_DWORD, (LPBYTE)&(disp = 0), sizeof(disp));
				}
				RegCloseKey(hRClkKey);
			}

			// Иначе в некторых случаях (Win7 & FAR2x64) не отрисовывается сменившийся курсор
			RedrawAll();
			
			//PostMacro((wchar_t*)L"@F11 %N=Menu.Select(\"EMenu\",0); $if (%N==0) %N=Menu.Select(\"EMenu\",2); $end $if(%N>0) Enter $while (Menu) Enter $end $else $MMode 1 MsgBox(\"ConEmu\",\"EMenu not found in F11\",0x00010001) $end");
			const wchar_t* pszMacro = L"@$If (!CmdLine.Empty) %Flg_Cmd=1; %CmdCurPos=CmdLine.ItemCount-CmdLine.CurPos+1; %CmdVal=CmdLine.Value; Esc $Else %Flg_Cmd=0; $End $Text \"rclk_gui:\" Enter $If (%Flg_Cmd==1) $Text %CmdVal %Flg_Cmd=0; %Num=%CmdCurPos; $While (%Num!=0) %Num=%Num-1; CtrlS $End $End";
			if (*pszUserMacro)
				pszMacro = pszUserMacro;
				
			PostMacro((wchar_t*)pszMacro);
			
			//// Чтобы GUI не дожидался окончания всплытия EMenu
			//LeaveCriticalSection(&csData);
			//SetEvent(ghReqCommandEvent);
			////
			//HMODULE hEMenu = GetModuleHandle(L"emenu.dll");
			//if (!hEMenu)
			//{
			//	if (gFarVersion.dwVerMajor==2) {
			//		TCHAR temp[NM*5];
			//		ExpandEnvironmentStringsW(L"%FARHOME%\\Plugins\\emenu\\EMenu.dll",temp,countof(temp));
			//		if (gFarVersion.dwBuild>=FAR_Y_VER) {
			//			FUNC_Y(LoadPlugin)(temp);
			//		} else {
			//			FUNC_X(LoadPlugin)(temp);
			//		}
			//		// Фактически FAR НЕ загружает длл-ку к сожалению, так что тут мы обломаемся
			//		hEMenu = GetModuleHandle(L"emenu.dll");
			//	}
			//
			//	if (!hEMenu) {
			//		PostMacro((wchar_t*)L"@F11 %N=Menu.Select(\"EMenu\",0); $if (%N==0) %N=Menu.Select(\"EMenu\",2); $end $if(%N>0) Enter $while (Menu) Enter $end $else $MMode 1 MsgBox(\"ConEmu\",\"EMenu not found in F11\",0x00010001) $end");
			//		break; // уже все что мог - сделал макрос
			//	}
			//}
			//if (hEMenu)
			//{
			//	OpenPlugin_t fnOpenPluginW = (OpenPlugin_t)GetProcAddress(hEMenu, (gFarVersion.dwVerMajor==1) ? "OpenPlugin" : "OpenPluginW");
			//	_ASSERTE(fnOpenPluginW);
			//	if (fnOpenPluginW) {
			//		if (gFarVersion.dwVerMajor==1) {
			//			fnOpenPluginW(OPEN_COMMANDLINE, (INT_PTR)"rclk_gui:");
			//		} else {
			//			fnOpenPluginW(OPEN_COMMANDLINE, (INT_PTR)L"rclk_gui:");
			//		}
			//	} else {
			//		// Ругнуться?
			//	}
			//}
			//return NULL;
			break;
		}
		case (CMD_SETSIZE):
		{
			_ASSERTE(pCommandData!=NULL);
			BOOL lbNeedChange = TRUE;
			DWORD nHILO = *((DWORD*)pCommandData);
			SHORT nWidth = LOWORD(nHILO);
			SHORT nHeight = HIWORD(nHILO);

			BOOL lbRc = SetConsoleSize(nWidth, nHeight);

			MConHandle hConOut ( L"CONOUT$" );
			CONSOLE_SCREEN_BUFFER_INFO csbi = {{0,0}};
			lbRc = GetConsoleScreenBufferInfo(hConOut, &csbi);
			hConOut.Close();
			if (lbRc) {
				OutDataAlloc(sizeof(nHILO));
				nHILO = ((WORD)csbi.dwSize.X) | (((DWORD)(WORD)csbi.dwSize.Y) << 16);
				OutDataWrite(&nHILO, sizeof(nHILO));
			}

			//REDRAWALL
		}
	}

	LeaveCriticalSection(&csData);

#ifdef _DEBUG
	_ASSERTE(_CrtCheckMemory());
#endif

	if (ghReqCommandEvent)
		SetEvent(ghReqCommandEvent);

	if (!pCmdRet)
		pCmdRet = gpCmdRet;

	return pCmdRet;
}

BOOL SetConsoleSize(SHORT nNewWidth, SHORT nNewHeight)
{
#ifdef _DEBUG
	if (GetCurrentThreadId() != gnMainThreadId) {
		_ASSERTE(GetCurrentThreadId() == gnMainThreadId);
	}
#endif

	BOOL lbRc = FALSE, lbNeedChange = TRUE;
	SHORT nWidth = nNewWidth; if (nWidth</*4*/MIN_CON_WIDTH) nWidth = /*4*/MIN_CON_WIDTH;
	SHORT nHeight = nNewHeight; if (nHeight</*3*/MIN_CON_HEIGHT) nHeight = /*3*/MIN_CON_HEIGHT;
	MConHandle hConOut ( L"CONOUT$" );
	COORD crMax = GetLargestConsoleWindowSize(hConOut);
	if (crMax.X && nWidth > crMax.X) nWidth = crMax.X;
	if (crMax.Y && nHeight > crMax.Y) nHeight = crMax.Y;

	CONSOLE_SCREEN_BUFFER_INFO csbi = {{0,0}};
	if (GetConsoleScreenBufferInfo(hConOut, &csbi)) {
		if (csbi.dwSize.X == nWidth && csbi.dwSize.Y == nHeight
			&& csbi.srWindow.Top == 0 && csbi.srWindow.Left == 0
			&& csbi.srWindow.Bottom == (nWidth-1) 
			&& csbi.srWindow.Bottom == (nHeight-1))
		{
			lbNeedChange = FALSE;
		}
	}

	if (lbNeedChange) {
		DWORD dwErr = 0;

		// Если этого не сделать - размер консоли нельзя УМЕНЬШИТЬ
		RECT rcConPos = {0}; GetWindowRect(FarHwnd, &rcConPos);
		MoveWindow(FarHwnd, rcConPos.left, rcConPos.top, 1, 1, 1);

		//specified width and height cannot be less than the width and height of the console screen buffer's window
		COORD crNewSize = {nWidth, nHeight};
		lbRc = SetConsoleScreenBufferSize(hConOut, crNewSize);
		if (!lbRc) dwErr = GetLastError();

		SMALL_RECT rNewRect = {0,0,nWidth-1,nHeight-1};
		SetConsoleWindowInfo(hConOut, TRUE, &rNewRect);

		RedrawAll();
	}

	return lbRc;
}

void EmergencyShow()
{
	SetWindowPos(FarHwnd, HWND_TOP, 50,50,0,0, SWP_NOSIZE);
	ShowWindowAsync(FarHwnd, SW_SHOWNORMAL);
	EnableWindow(FarHwnd, true);
}

// Эту нить нужно оставить, чтобы была возможность отобразить консоль при падении ConEmu
DWORD WINAPI MonitorThreadProcW(LPVOID lpParameter)
{
	//DWORD dwProcId = GetCurrentProcessId();

	DWORD dwStartTick = GetTickCount();
	DWORD dwMonitorTick = dwStartTick;
	BOOL lbStartedNoConEmu = (ConEmuHwnd == NULL);
	//_ASSERTE(ConEmuHwnd!=NULL); -- ConEmu может подцепиться позднее!

	while (true)
	{
		DWORD dwWait = 0;
		DWORD dwTimeout = 500;
		/*#ifdef _DEBUG
		dwTimeout = INFINITE;
		#endif*/

		//dwWait = WaitForMultipleObjects(MAXCMDCOUNT, hEventCmd, FALSE, dwTimeout);
		dwWait = WaitForSingleObject(ghServerTerminateEvent, dwTimeout);
		if (dwWait == WAIT_OBJECT_0)
			break; // завершение плагина

		if (lbStartedNoConEmu && ConEmuHwnd == NULL && FarHwnd != NULL) {
			DWORD dwCurTick = GetTickCount();
			DWORD dwDelta = dwCurTick - dwStartTick;
			if (dwDelta > GUI_ATTACH_TIMEOUT) {
				lbStartedNoConEmu = FALSE;
				if (!TerminalMode && !IsWindowVisible(FarHwnd)) {
					EmergencyShow();
				}
			}
		}

		// Теоретически, нить обработки может запуститься и без ConEmuHwnd (под телнетом)
	    if (ConEmuHwnd && FarHwnd && (dwWait>=(WAIT_OBJECT_0+MAXCMDCOUNT))) {

			// Мог быть сделан Detach! (CtrlAltTab)
			HWND hConWnd = GetConsoleWindow();
		    if (!IsWindow(ConEmuHwnd) || hConWnd!=FarHwnd) {
			    ConEmuHwnd = NULL;
			    //
			    SetConEmuEnvVar(NULL);

				if (!TerminalMode && !IsWindowVisible(FarHwnd)) {
					EmergencyShow();
				}

				if (hConWnd!=FarHwnd || !IsWindow(FarHwnd))
				{
					if (hConWnd != FarHwnd && IsWindow(hConWnd)) {
						FarHwnd = hConWnd;
						//int nBtn = ShowMessage(1, 2);
						//if (nBtn == 0) {
						//	// Create process, with flag /Attach GetCurrentProcessId()
						//	// Sleep for sometimes, try InitHWND(hConWnd); several times
						//	WCHAR  szExe[0x200];
						//	WCHAR  szCurArgs[0x600];
						//	
						//	PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
						//	STARTUPINFO si; memset(&si, 0, sizeof(si));
						//	si.cb = sizeof(si);
						//	
						//	//TODO: ConEmu.exe
						//	int nLen = 0;
						//	if ((nLen=GetModuleFileName(0, szExe, 0x190))==0) {
						//		goto closethread;
						//	}
						//	WCHAR* pszSlash = szExe+nLen-1;
			   //             while (pszSlash>szExe && *(pszSlash-1)!=L'\\') pszSlash--;
			   //             lstrcpyW(pszSlash, L"ConEmu.exe");
			   //             
						//	DWORD dwPID = GetCurrentProcessId();
						//	wsprintf(szCurArgs, L"\"%s\" /Attach %i ", szExe, dwPID);
						//	
						//	GetEnvironmentVariableW(L"ConEmuArgs", szCurArgs+lstrlenW(szCurArgs), 0x380);
			   //             
						//	
						//	if (!CreateProcess(NULL, szCurArgs, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL,
						//			NULL, &si, &pi))
						//	{
						//		// Хорошо бы ошибку показать?
						//		goto closethread;
						//	}
						//	
						//	// Ждем
						//	while (TRUE)
						//	{
						//		dwWait = WaitForSingleObject(hEventCmd[CMD_EXIT], 200);
						//		// ФАР закрывается
						//		if (dwWait == WAIT_OBJECT_0) {
						//			CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
						//			goto closethread;
						//		}
						//		if (!GetExitCodeProcess(pi.hProcess, &dwPID) || dwPID!=STILL_ACTIVE) {
						//			CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
						//			goto closethread;
						//		}
						//		InitHWND(hConWnd);
						//		if (ConEmuHwnd) {
						//			// Справились, но шрифт ConEmu скорее всего поменять не смог...
						//			SetConsoleFontSizeTo(FarHwnd, 4, 6);
						//			MoveWindow(FarHwnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 1); // чтобы убрать возможные полосы прокрутки...
						//			break;
						//		}
						//	}
						//	
						//	// Закрыть хэндлы
						//	CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
						//	// Хорошо бы сразу Табы обновить...
						//	Send Tabs(gnCurTabCount, TRUE);
						//	continue;
						//} else {
						//	// Пользователь отказался, выходим из нити обработки
						//	goto closethread;
						//}
					} else {
						// hConWnd не валидно
						MessageBox(0, L"ConEmu was abnormally termintated!\r\nExiting from FAR", L"ConEmu plugin", MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
						TerminateProcess(GetCurrentProcess(), 100);
					}
				}
				else 
				{
					//if (bWasSetParent) { -- все равно уже не поможет, если она была дочерней - сдохла
					//	SetParent(FarHwnd, NULL);
					//}
				    
					EmergencyShow();
				}
			    
				//goto closethread;
		    }
	    }

		//if (dwWait == (WAIT_OBJECT_0+CMD_EXIT))
		//{
		//	goto closethread;
		//}

		//if (dwWait>=(WAIT_OBJECT_0+MAXCMDCOUNT))
		//{
		//	continue;
		//}

		if (!ConEmuHwnd) {
			// ConEmu могло подцепиться
			//int nChk = 0;
			ConEmuHwnd = GetConEmuHWND ( FALSE/*abRoot*/  /*, &nChk*/ );
			if (ConEmuHwnd) {
                SetConEmuEnvVar(ConEmuHwnd);
			
				InitResources();
			}
		}

		if (ConEmuHwnd && gbMonitorEnvVar && gsMonitorEnvVar[0]
			&& (GetTickCount() - dwMonitorTick) > MONITORENVVARDELTA)
		{
			UpdateEnvVar(gsMonitorEnvVar);

			dwMonitorTick = GetTickCount();
		}

		if (gbNeedPostTabSend) {
			DWORD nDelta = GetTickCount() - gnNeedPostTabSendTick;
			if (nDelta > NEEDPOSTTABSENDDELTA) {
				if (IsMacroActive()) {
					gnNeedPostTabSendTick = GetTickCount();
				} else {
					// Force Send tabs to ConEmu
					MSectionLock SC; SC.Lock(&csTabs, TRUE);
					SendTabs(gnCurTabCount, TRUE);
					SC.Unlock();
				}
			}
		}


		//SafeCloseHandle(ghMapping);
		//// Поставим флажок, что мы приступили к обработке
		//// Хоть табы и не загружаются из фара, а пересылаются в текущем виде, но делается это обычным образом
		////if (dwWait != (WAIT_OBJECT_0+CMD_REQTABS)) // исключение - запрос табов. он асинхронный
		//SetEvent(hEventAlive);


		//switch (dwWait)
		//{
		//	case (WAIT_OBJECT_0+CMD_REQTABS):
		//	{
		//		/*if (!gnCurTabCount || !gpTabs) {
		//			CreateTabs(1);
		//			int nTabs=0; --низзя! это теперь идет через CriticalSection
		//			AddTab(nTabs, false, false, WTYPE_PANELS, 0,0,1,0); 
		//		}*/
		//		Send Tabs(gnCurTabCount, TRUE);
		//		// пересылка тоже обычным образом
		//		//// исключение - запрос табов. он асинхронный
		//		//continue;
		//		break;
		//	}
		//	
		//	case (WAIT_OBJECT_0+CMD_DEFFONT):
		//	{
		//		// исключение - асинхронный, результат не требуется
		//		ResetEvent(hEventAlive);
		//		SetConsoleFontSizeTo(FarHwnd, 4, 6);
		//		MoveWindow(FarHwnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 1); // чтобы убрать возможные полосы прокрутки...
		//		continue;
		//	}
		//	
		//	case (WAIT_OBJECT_0+CMD_LANGCHANGE):
		//	{
		//		// исключение - асинхронный, результат не требуется
		//		ResetEvent(hEventAlive);
		//		HKL hkl = (HKL)GetWindowLong(ConEmuHwnd, GWL_LANGCHANGE);
		//		if (hkl) {
		//			DWORD dwLastError = 0;
		//			WCHAR szLoc[10]; wsprintf(szLoc, L"%08x", (DWORD)(((DWORD)hkl) & 0xFFFF));
		//			HKL hkl1 = LoadKeyboardLayout(szLoc, KLF_ACTIVATE|KLF_REORDER|KLF_SUBSTITUTE_OK|KLF_SETFORPROCESS);
		//			HKL hkl2 = ActivateKeyboardLayout(hkl1, KLF_SETFORPROCESS|KLF_REORDER);
		//			if (!hkl2) {
		//				dwLastError = GetLastError();
		//			}
		//			dwLastError = dwLastError;
		//		}
		//		continue;
		//	}
		//	
		//	default:
		//	// Все остальные команды нужно выполнять в нити FAR'а
		//	ProcessCommand(dwWait/*nCmd*/, TRUE/*bReqMainThread*/, NULL);
		//}

		//// Подготовиться к передаче данных
		//Enter CriticalSection(&csData);
		//wsprintf(gszDir1, CONEMUMAPPING, dwProcId);
		//gnDataSize = gpData ? (gpCursor - gpData) : 0;
		//#ifdef _DEBUG
		//int nSize = gnDataSize; // чего-то там происходит...
		//#endif
		//SetLastError(0);
		//ghMapping = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, gnDataSize+4, gszDir1);
		//#ifdef _DEBUG
		//DWORD dwCreateError = GetLastError();
		//#endif
		//if (ghMapping) {
		//	LPBYTE lpMap = (LPBYTE)MapViewOfFile(ghMapping, FILE_MAP_ALL_ACCESS, 0,0,0);
		//	if (!lpMap) {
		//		// Ошибка
		//		SafeCloseHandle(ghMapping);
		//	} else {
		//		// copy data
		//		if (gpData && gnDataSize) {
		//			*((DWORD*)lpMap) = gnDataSize+4;
		//			#ifdef _DEBUG
		//			LPBYTE dst=lpMap+4; LPBYTE src=gpData;
		//			for (DWORD n=gnDataSize;n>0;n--)
		//				*(dst++) = *(src++);
		//			#else
		//			memcpy(lpMap+4, gpData, gnDataSize);
		//			#endif
		//		} else {
		//			*((DWORD*)lpMap) = 0;
		//		}

		//		//unmaps a mapped view of a file from the calling process's address space
		//		UnmapViewOfFile(lpMap);
		//	}
		//}
		//if (gpData) {
		//	Free(gpCmdRet);
		//	gpCmdRet = NULL; gpData = NULL; gpCursor = NULL;
		//}
		//LeaveCriticalSection(&csData);

		// Поставим флажок, что плагин жив и закончил
		//SetEvent(hEventReady);

		//Sleep(1);
	}


//closethread:
	// Закрываем все хэндлы и выходим
	//for (int i=0; i<MAXCMDCOUNT; i++)
	//	SafeCloseHandle(hEventCmd[i]);
	//SafeCloseHandle(hEventAlive);
	//SafeCloseHandle(hEventReady);
	//SafeCloseHandle(ghMapping);

	return 0;
}

BOOL SendConsoleEvent(INPUT_RECORD* pr, UINT nCount)
{
	_ASSERTE(nCount>0);
	BOOL fSuccess = FALSE;

	if (!ghConIn) {
		ghConIn  = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
			0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (ghConIn == INVALID_HANDLE_VALUE) {
			#ifdef _DEBUG
			DWORD dwErr = GetLastError();
			_ASSERTE(ghConIn!=INVALID_HANDLE_VALUE);
			#endif
			ghConIn = NULL;
			return FALSE;
		}
	}
	if (!ghInputSynchroExecuted)
		ghInputSynchroExecuted = CreateEvent(NULL, FALSE, FALSE, NULL);

	if (gFarVersion.dwVerMajor>1 && (gFarVersion.dwVerMinor>0 || gFarVersion.dwBuild>=1006))
	{
		static SynchroArg arg = {SynchroArg::eInput};
		arg.hEvent = ghInputSynchroExecuted;
		arg.Param1 = (LPARAM)pr;
		arg.Param2 = nCount;

		if (gFarVersion.dwBuild>=FAR_Y_VER)
			fSuccess = FUNC_Y(CallSynchro)(&arg,DEFAULT_SYNCHRO_TIMEOUT);
		else
			fSuccess = FUNC_X(CallSynchro)(&arg,DEFAULT_SYNCHRO_TIMEOUT);
	} 
	
	if (!fSuccess)
	{
		DWORD nCurInputCount = 0, cbWritten = 0;
		INPUT_RECORD irDummy[2] = {{0},{0}};

		// 27.06.2009 Maks - If input queue is not empty - wait for a while, to avoid conflicts with FAR reading queue
		if (PeekConsoleInput(ghConIn, irDummy, 1, &(nCurInputCount = 0)) && nCurInputCount > 0) {
			DWORD dwStartTick = GetTickCount();
			WARNING("Do NOT wait, but place event in Cyclic queue");
			do {
				Sleep(5);
				if (!PeekConsoleInput(ghConIn, irDummy, 1, &(nCurInputCount = 0)))
					nCurInputCount = 0;
			} while ((nCurInputCount > 0) && ((GetTickCount() - dwStartTick) < MAX_INPUT_QUEUE_EMPTY_WAIT));
		}

		fSuccess = WriteConsoleInput(ghConIn, pr, nCount, &cbWritten);
		_ASSERTE(fSuccess && cbWritten==nCount);
	}

	return fSuccess;
} 

DWORD WINAPI InputThreadProcW(LPVOID lpParameter)
{
	MSG msg;
	static INPUT_RECORD recs[10] = {{0}}; // переменная должна быть глобальной? SynchoApi...

	while (GetMessage(&msg,0,0,0)) {
		if (msg.message == WM_QUIT) return 0;
		if (ghServerTerminateEvent) {
			if (WaitForSingleObject(ghServerTerminateEvent, 0) == WAIT_OBJECT_0) return 0;
		}
		if (msg.message == 0) continue;

		if (msg.message == INPUT_THREAD_ALIVE_MSG) {
			//pRCon->mn_FlushOut = msg.wParam;
			TODO("INPUT_THREAD_ALIVE_MSG");
			continue;

		} else {

			INPUT_RECORD *pRec = recs;
			int nCount = 0, nMaxCount = countof(recs);
			memset(recs, 0, sizeof(recs));

			do {
				if (UnpackInputRecord(&msg, pRec)) {
					TODO("Сделать обработку пачки сообщений, вдруг они накопились в очереди?");

					if (pRec->EventType == KEY_EVENT && pRec->Event.KeyEvent.bKeyDown &&
						(pRec->Event.KeyEvent.wVirtualKeyCode == 'C' || pRec->Event.KeyEvent.wVirtualKeyCode == VK_CANCEL)
						)
					{
						#define ALL_MODIFIERS (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED|LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED|SHIFT_PRESSED)
						#define CTRL_MODIFIERS (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)

						BOOL lbRc = FALSE;
						DWORD dwEvent = (pRec->Event.KeyEvent.wVirtualKeyCode == 'C') ? CTRL_C_EVENT : CTRL_BREAK_EVENT;
						//&& (srv.dwConsoleMode & ENABLE_PROCESSED_INPUT)

						//The SetConsoleMode function can disable the ENABLE_PROCESSED_INPUT mode for a console's input buffer, 
						//so CTRL+C is reported as keyboard input rather than as a signal. 
						// CTRL+BREAK is always treated as a signal
						if ( // Удерживается ТОЛЬКО Ctrl
							(pRec->Event.KeyEvent.dwControlKeyState & CTRL_MODIFIERS) &&
							((pRec->Event.KeyEvent.dwControlKeyState & ALL_MODIFIERS) 
							== (pRec->Event.KeyEvent.dwControlKeyState & CTRL_MODIFIERS))
							)
						{
							// Вроде работает, Главное не запускать процесс с флагом CREATE_NEW_PROCESS_GROUP
							// иначе у микрософтовской консоли (WinXP SP3) сносит крышу, и она реагирует
							// на Ctrl-Break, но напрочь игнорирует Ctrl-C
							lbRc = GenerateConsoleCtrlEvent(dwEvent, 0);

							// Это событие (Ctrl+C) в буфер помещается(!) иначе до фара не дойдет собственно клавиша C с нажатым Ctrl
						}
					}
					nCount++; pRec++;
				}
				// Если в буфере есть еще сообщения, а recs еще не полностью заполнен
				if (nCount < nMaxCount)
				{
					if (!PeekMessage(&msg, 0,0,0, PM_REMOVE))
						break;
					if (msg.message == 0) continue;
					if (msg.message == WM_QUIT) return 0;
					if (ghServerTerminateEvent) {
						if (WaitForSingleObject(ghServerTerminateEvent, 0) == WAIT_OBJECT_0) return 0;
					}
				}
			} while (nCount < nMaxCount);
			if (nCount > 0)
				SendConsoleEvent(recs, nCount);
		}
	}

	return 0;
}

void WINAPI _export SetStartupInfoW(void *aInfo)
{
	if (!gFarVersion.dwVerMajor) LoadFarVersion();

	if (gFarVersion.dwBuild>=FAR_Y_VER)
		FUNC_Y(SetStartupInfoW)(aInfo);
	else
		FUNC_X(SetStartupInfoW)(aInfo);

	gbInfoW_OK = TRUE;

	// в FAR2 устарело - Synchro
	//CheckMacro(TRUE);

	CheckResources();
}

//#define CREATEEVENT(fmt,h) 
//		wsprintf(szEventName, fmt, dwCurProcId ); 
//		h = CreateEvent(NULL, FALSE, FALSE, szEventName); 
//		if (h==INVALID_HANDLE_VALUE) h=NULL;
	
void InitHWND(HWND ahFarHwnd)
{
	gsFarLang[0] = 0;
	//InitializeCriticalSection(&csTabs);
	InitializeCriticalSection(&csData);
	LoadFarVersion(); // пригодится уже здесь!

	// начальная инициализация. в SetStartupInfo поправим
	wsprintfW(gszRootKey, L"Software\\%s", (gFarVersion.dwVerMajor==2) ? L"FAR2" : L"FAR");
	// Нужно учесть, что FAR мог запуститься с ключом /u (выбор конфигурации)
	wchar_t* pszUserSlash = gszRootKey+wcslen(gszRootKey);
	lstrcpyW(pszUserSlash, L"\\Users\\");
	wchar_t* pszUserAdd = pszUserSlash+wcslen(pszUserSlash);
	if (GetEnvironmentVariable(L"FARUSER", pszUserAdd, MAX_PATH) == 0)
		*pszUserSlash = 0;

	ConEmuHwnd = NULL;
	FarHwnd = ahFarHwnd;

	//memset(hEventCmd, 0, sizeof(HANDLE)*MAXCMDCOUNT);
	
	//int nChk = 0;
	ConEmuHwnd = GetConEmuHWND ( FALSE/*abRoot*/  /*, &nChk*/ );

    SetConEmuEnvVar(ConEmuHwnd);
	
	gnMsgTabChanged = RegisterWindowMessage(CONEMUTABCHANGED);

	if (!ghSetWndSendTabsEvent) ghSetWndSendTabsEvent = CreateEvent(0,0,0,0);

	// Даже если мы не в ConEmu - все равно запустить нить, т.к. в ConEmu теперь есть возможность /Attach!
	//WCHAR szEventName[128];
	DWORD dwCurProcId = GetCurrentProcessId();

	if (!ghReqCommandEvent) {
		ghReqCommandEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
		_ASSERTE(ghReqCommandEvent!=NULL);
	}

	// Запустить сервер команд
	wsprintf(gszPluginServerPipe, CEPLUGINPIPENAME, L".", dwCurProcId);
	ghServerTerminateEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	_ASSERTE(ghServerTerminateEvent!=NULL);
	if (ghServerTerminateEvent) ResetEvent(ghServerTerminateEvent);
	ghPluginSemaphore = CreateSemaphore(NULL, 1, 1, NULL);
    gnServerThreadId = 0;
    ghServerThread = CreateThread(NULL, 0, ServerThread, (LPVOID)NULL, 0, &gnServerThreadId);
    _ASSERTE(ghServerThread!=NULL);


	ghMonitorThread = CreateThread(NULL, 0, MonitorThreadProcW, 0, 0, &gnMonitorThreadId);

	ghInputThread = CreateThread(NULL, 0, InputThreadProcW, 0, 0, &gnInputThreadId);


	// Если мы не под эмулятором - больше ничего делать не нужно
	if (ConEmuHwnd) {
		//
		DWORD dwPID, dwThread;
		dwThread = GetWindowThreadProcessId(ConEmuHwnd, &dwPID);
		typedef BOOL (WINAPI* AllowSetForegroundWindowT)(DWORD);
		HMODULE hUser32 = GetModuleHandle(L"user32.dll");
		if (hUser32) {
			AllowSetForegroundWindowT AllowSetForegroundWindowF = (AllowSetForegroundWindowT)GetProcAddress(hUser32, "AllowSetForegroundWindow");
			if (AllowSetForegroundWindowF) AllowSetForegroundWindowF(dwPID);
		}
		// дернуть табы, если они нужны
		int tabCount = 0;
		MSectionLock SC; SC.Lock(&csTabs);
		CreateTabs(1);
		AddTab(tabCount, true, false, WTYPE_PANELS, NULL, NULL, 0, 0);
		SendTabs(tabCount=1, TRUE);
		SC.Unlock();
	}
}

void NotifyChangeKey()
{
	if (ghRegMonitorKey) { RegCloseKey(ghRegMonitorKey); ghRegMonitorKey = NULL; }
	if (ghRegMonitorEvt) ResetEvent(ghRegMonitorEvt);
	
	WCHAR szKeyName[MAX_PATH*2];
	lstrcpyW(szKeyName, gszRootKey);
	lstrcatW(szKeyName, L"\\PluginHotkeys");
	// Ключа может и не быть, если ни для одного плагина не было зарегистрировано горячей клавиши
	if (0 == RegOpenKeyEx(HKEY_CURRENT_USER, szKeyName, 0, KEY_NOTIFY, &ghRegMonitorKey)) {
		if (!ghRegMonitorEvt) ghRegMonitorEvt = CreateEvent(NULL,FALSE,FALSE,NULL);
		RegNotifyChangeKeyValue(ghRegMonitorKey, TRUE, REG_NOTIFY_CHANGE_LAST_SET, ghRegMonitorEvt, TRUE);
		return;
	}
	// Если их таки нет - пробуем подцепиться к корневому ключу
	if (0 == RegOpenKeyEx(HKEY_CURRENT_USER, gszRootKey, 0, KEY_NOTIFY, &ghRegMonitorKey)) {
		if (!ghRegMonitorEvt) ghRegMonitorEvt = CreateEvent(NULL,FALSE,FALSE,NULL);
		RegNotifyChangeKeyValue(ghRegMonitorKey, TRUE, REG_NOTIFY_CHANGE_LAST_SET, ghRegMonitorEvt, TRUE);
		return;
	}
}

//abCompare=TRUE вызывается после загрузки плагина, если юзер изменил горячую клавишу...
BOOL CheckPlugKey()
{
	WCHAR cCurKey = gcPlugKey;
	gcPlugKey = 0;
	BOOL lbChanged = FALSE;
	HKEY hkey=NULL;
	WCHAR szMacroKey[2][MAX_PATH], szCheckKey[32];
	
	//Прочитать назначенные плагинам клавиши, и если для ConEmu*.dll указана клавиша активации - запомнить ее
	wsprintfW(szMacroKey[0], L"%s\\PluginHotkeys", gszRootKey/*, szCheckKey*/);
	if (0==RegOpenKeyExW(HKEY_CURRENT_USER, szMacroKey[0], 0, KEY_READ, &hkey))
	{
		DWORD dwIndex = 0, dwSize; FILETIME ft;
		while (0==RegEnumKeyEx(hkey, dwIndex++, szMacroKey[1], &(dwSize=MAX_PATH), NULL, NULL, NULL, &ft)) {
			WCHAR* pszSlash = szMacroKey[1]+lstrlenW(szMacroKey[1])-1;
			while (pszSlash>szMacroKey[1] && *pszSlash!=L'/') pszSlash--;
			#if !defined(__GNUC__)
			#pragma warning(disable : 6400)
			#endif
			if (lstrcmpiW(pszSlash, L"/conemu.dll")==0 || lstrcmpiW(pszSlash, L"/conemu.x64.dll")==0) {
				WCHAR lsFullPath[MAX_PATH*2];
				lstrcpy(lsFullPath, szMacroKey[0]);
				lstrcat(lsFullPath, L"\\");
				lstrcat(lsFullPath, szMacroKey[1]);

				RegCloseKey(hkey); hkey=NULL;

				if (0==RegOpenKeyExW(HKEY_CURRENT_USER, lsFullPath, 0, KEY_READ, &hkey)) {
					dwSize = sizeof(szCheckKey);
					if (0==RegQueryValueExW(hkey, L"Hotkey", NULL, NULL, (LPBYTE)szCheckKey, &dwSize)) {
						gcPlugKey = szCheckKey[0];
					}
				}
				//
				//
				break;
			}
		}

		// Закончили
		if (hkey) {RegCloseKey(hkey); hkey=NULL;}
	}
	
	lbChanged = (gcPlugKey != cCurKey);
	
	return lbChanged;
}

void CheckMacro(BOOL abAllowAPI)
{
	// и не под эмулятором нужно проверять макросы, иначе потом активация не сработает...
	//// Если мы не под эмулятором - больше ничего делать не нужно
	//if (!ConEmuHwnd) return;


	// Проверка наличия макроса
	BOOL lbMacroAdded = FALSE, lbNeedMacro = FALSE;
	HKEY hkey=NULL;
	#define MODCOUNT 4
	int n;
	char szValue[1024];
	WCHAR szMacroKey[MODCOUNT][MAX_PATH], szCheckKey[32];
	DWORD dwSize = 0;
	//bool lbMacroDontCheck = false;

	//Прочитать назначенные плагинам клавиши, и если для ConEmu*.dll указана клавиша активации - запомнить ее
	CheckPlugKey();


	for (n=0; n<MODCOUNT; n++) {
		switch(n){
			case 0: lstrcpyW(szCheckKey, L"F14"); break;
			case 1: lstrcpyW(szCheckKey, L"CtrlF14"); break;
			case 2: lstrcpyW(szCheckKey, L"AltF14"); break;
			case 3: lstrcpyW(szCheckKey, L"ShiftF14"); break;
		}
		wsprintfW(szMacroKey[n], L"%s\\KeyMacros\\Common\\%s", gszRootKey, szCheckKey);
	}
	if (gFarVersion.dwVerMajor==1) {
		lstrcpyW(szCheckKey, L"F11  "); //TODO: для ANSI может другой код по умолчанию?
		szCheckKey[4] = (wchar_t)(gcPlugKey ? gcPlugKey : ((gFarVersion.dwVerMajor==1) ? 0x42C/*0xDC - аналог для OEM*/ : 0x2584));
	} else {
		// Пока можно так. (пока GUID не появились)
		wsprintfW(szCheckKey, L"callplugin(0x%08X,0)", ConEmu_SysID);
	}

	//if (!lbMacroDontCheck)
	for (n=0; n<MODCOUNT && !lbNeedMacro; n++)
	{
		if (0==RegOpenKeyExW(HKEY_CURRENT_USER, szMacroKey[n], 0, KEY_READ, &hkey))
		{
			/*if (gFarVersion.dwVerMajor==1) {
				if (0!=RegQueryValueExA(hkey, "Sequence", 0, 0, (LPBYTE)szValue, &(dwSize=1022))) {
					lbNeedMacro = TRUE; // Значение отсутсвует
				} else {
					lbNeedMacro = lstrcmpA(szValue, (char*)szCheckKey)!=0;
				}
			} else {*/
				if (0!=RegQueryValueExW(hkey, L"Sequence", 0, 0, (LPBYTE)szValue, &(dwSize=1022))) {
					lbNeedMacro = TRUE; // Значение отсутсвует
				} else {
					//TODO: проверить, как себя ведет VC & GCC на 2х байтовых символах?
					lbNeedMacro = lstrcmpW((WCHAR*)szValue, szCheckKey)!=0;
				}
			//}
			//	szValue[dwSize]=0;
			//	#pragma message("ERROR: нужна проверка. В Ansi и Unicode это разные строки!")
			//	//if (strcmpW(szValue, "F11 \xCC")==0)
			//		lbNeedMacro = TRUE; // Значение некорректное
			//}
			RegCloseKey(hkey); hkey=NULL;
		} else {
			lbNeedMacro = TRUE;
		}
	}

	if (lbNeedMacro) {
		//int nBtn = ShowMessage(0, 3);
		//if (nBtn == 1) { // Don't disturb
		//	DWORD disp=0;
		//	lbMacroDontCheck = true;
		//	if (0==RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\ConEmu", 0, 0, 
		//		0, KEY_ALL_ACCESS, 0, &hkey, &disp))
		//	{
		//		RegSetValueExA(hkey, szCheckKey, 0, REG_BINARY, (LPBYTE)&lbMacroDontCheck, (dwSize=sizeof(lbMacroDontCheck)));
		//		RegCloseKey(hkey); hkey=NULL;
		//	}
		//} else if (nBtn == 0) 
		for (n=0; n<MODCOUNT; n++)
		{
			DWORD disp=0;
			lbMacroAdded = TRUE;
			if (0==RegCreateKeyExW(HKEY_CURRENT_USER, szMacroKey[n], 0, 0, 
				0, KEY_ALL_ACCESS, 0, &hkey, &disp))
			{
				lstrcpyA(szValue, "ConEmu support");
				RegSetValueExA(hkey, "", 0, REG_SZ, (LPBYTE)szValue, (dwSize=(DWORD)strlen(szValue)+1));

				//lstrcpyA(szValue, 
				//	"$If (Shell || Info || QView || Tree || Viewer || Editor) F12 $Else waitkey(100) $End");
				//RegSetValueExA(hkey, "Sequence", 0, REG_SZ, (LPBYTE)szValue, (dwSize=strlen(szValue)+1));
				
				/*if (gFarVersion.dwVerMajor==1) {
					RegSetValueExA(hkey, "Sequence", 0, REG_SZ, (LPBYTE)szCheckKey, (dwSize=strlen((char*)szCheckKey)+1));
				} else {*/
					RegSetValueExW(hkey, L"Sequence", 0, REG_SZ, (LPBYTE)szCheckKey, (dwSize=2*((DWORD)lstrlenW((WCHAR*)szCheckKey)+1)));
				//}

				lstrcpyA(szValue, "For ConEmu - plugin activation from listening thread");
				RegSetValueExA(hkey, "Description", 0, REG_SZ, (LPBYTE)szValue, (dwSize=(DWORD)strlen(szValue)+1));

				RegSetValueExA(hkey, "DisableOutput", 0, REG_DWORD, (LPBYTE)&(disp=1), (dwSize=4));

				RegCloseKey(hkey); hkey=NULL;
			}
		}
	}


	// Перечитать макросы в FAR?
	if (lbMacroAdded && abAllowAPI) {
		if (gFarVersion.dwVerMajor==1)
			ReloadMacroA();
		else if (gFarVersion.dwBuild>=FAR_Y_VER)
			FUNC_Y(ReloadMacro)();
		else
			FUNC_X(ReloadMacro)();
	}

	// First call
	if (abAllowAPI) {
		NotifyChangeKey();
	}
}

void UpdateConEmuTabsW(int event, bool losingFocus, bool editorSave, void* Param/*=NULL*/)
{
	if (!gbInfoW_OK)
		return;

	if (ConEmuHwnd && FarHwnd)
		CheckResources();

	MSectionLock SC; SC.Lock(&csTabs);

	if (gFarVersion.dwBuild>=FAR_Y_VER)
		FUNC_Y(UpdateConEmuTabsW)(event, losingFocus, editorSave, Param);
	else
		FUNC_X(UpdateConEmuTabsW)(event, losingFocus, editorSave, Param);

	SC.Unlock();
}

BOOL CreateTabs(int windowCount)
{
	if (gpTabs && maxTabCount > (windowCount + 1)) {
		// пересоздавать не нужно, секцию не трогаем. только запомним последнее кол-во окон
		lastWindowCount = windowCount;
		return TRUE; 
	}

	//Enter CriticalSection(&csTabs);

	if ((gpTabs==NULL) || (maxTabCount <= (windowCount + 1)))
	{
		MSectionLock SC; SC.Lock(&csTabs, TRUE);

		maxTabCount = windowCount + 20; // с запасом
		if (gpTabs) {
			Free(gpTabs); gpTabs = NULL;
		}
		
		gpTabs = (CESERVER_REQ*) Alloc(sizeof(CESERVER_REQ_HDR) + maxTabCount*sizeof(ConEmuTab), 1);
	}
	
	lastWindowCount = windowCount;
	
	//if (!gpTabs) LeaveCriticalSection(&csTabs);
	return gpTabs!=NULL;
}


	#ifndef max
	#define max(a,b)            (((a) > (b)) ? (a) : (b))
	#endif

	#ifndef min
	#define min(a,b)            (((a) < (b)) ? (a) : (b))
	#endif

BOOL AddTab(int &tabCount, bool losingFocus, bool editorSave, 
			int Type, LPCWSTR Name, LPCWSTR FileName, int Current, int Modified)
{
    BOOL lbCh = FALSE;
	DEBUGSTR(L"--AddTab\n");

	if (Type == WTYPE_PANELS) {
	    lbCh = (gpTabs->Tabs.tabs[0].Current != (losingFocus ? 1 : 0)) ||
	           (gpTabs->Tabs.tabs[0].Type != WTYPE_PANELS);
		gpTabs->Tabs.tabs[0].Current = losingFocus ? 1 : 0;
		//lstrcpyn(gpTabs->Tabs.tabs[0].Name, FUNC_Y(GetMsgW)(0), CONEMUTABMAX-1);
		gpTabs->Tabs.tabs[0].Name[0] = 0;
		gpTabs->Tabs.tabs[0].Pos = 0;
		gpTabs->Tabs.tabs[0].Type = WTYPE_PANELS;
		gpTabs->Tabs.tabs[0].Modified = 0; // Иначе GUI может ошибочно считать, что есть несохраненные редакторы
		if (!tabCount) tabCount++;
	} else
	if (Type == WTYPE_EDITOR || Type == WTYPE_VIEWER)
	{
		// Первое окно - должно быть панели. Если нет - значит фар открыт в режиме редактора
		if (tabCount == 1) {
			// 04.06.2009 Maks - Не, чего-то не то... при открытии редактора из панелей - он заменяет панели
			//gpTabs->Tabs.tabs[0].Type = Type;
		}

		// when receiving saving event receiver is still reported as modified
		if (editorSave && lstrcmpi(FileName, Name) == 0)
			Modified = 0;
	
	    lbCh = (gpTabs->Tabs.tabs[tabCount].Current != (losingFocus ? 0 : Current)) ||
	           (gpTabs->Tabs.tabs[tabCount].Type != Type) ||
	           (gpTabs->Tabs.tabs[tabCount].Modified != Modified) ||
	           (lstrcmp(gpTabs->Tabs.tabs[tabCount].Name, Name) != 0);
	
		// when receiving losing focus event receiver is still reported as current
		gpTabs->Tabs.tabs[tabCount].Type = Type;
		gpTabs->Tabs.tabs[tabCount].Current = losingFocus ? 0 : Current;
		gpTabs->Tabs.tabs[tabCount].Modified = Modified;

		if (gpTabs->Tabs.tabs[tabCount].Current != 0)
		{
			lastModifiedStateW = Modified != 0 ? 1 : 0;
		}
		//else
		//{
		//	lastModifiedStateW = -1; //2009-08-17 при наличии более одного редактора - сносит крышу
		//}

		int nLen = min(lstrlen(Name),(CONEMUTABMAX-1));
		lstrcpyn(gpTabs->Tabs.tabs[tabCount].Name, Name, nLen+1);
		gpTabs->Tabs.tabs[tabCount].Name[nLen]=0;

		gpTabs->Tabs.tabs[tabCount].Pos = tabCount;
		tabCount++;
	}
	
	return lbCh;
}

void SendTabs(int tabCount, BOOL abForceSend/*=FALSE*/)
{
	MSectionLock SC; SC.Lock(&csTabs);

	if (!gpTabs) {
		_ASSERTE(gpTabs!=NULL);
		return;
	}

	gnCurTabCount = tabCount; // сразу запомним!, А то при ретриве табов количество еще старым будет...

	gpTabs->Tabs.nTabCount = tabCount;
	gpTabs->hdr.nSize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_CONEMUTAB) 
		+ sizeof(ConEmuTab) * ((tabCount > 1) ? (tabCount - 1) : 0);

	// Обновляем структуру сразу, чтобы она была готова к отправке в любой момент
	ExecutePrepareCmd(gpTabs, CECMD_TABSCHANGED, gpTabs->hdr.nSize);

	// Это нужно делать только если инициировано ФАРОМ. Если запрос прислал ConEmu - не посылать...
	if (tabCount && ConEmuHwnd && IsWindow(ConEmuHwnd) && abForceSend)
	{
		gpTabs->Tabs.bMacroActive = IsMacroActive();
		gpTabs->Tabs.bMainThread = (GetCurrentThreadId() == gnMainThreadId);
		// Если выполняется макрос и отложенная отсылка (по окончанию) уже запрошена
		if (gpTabs->Tabs.bMacroActive && gbNeedPostTabSend) {
			gnNeedPostTabSendTick = GetTickCount(); // Обновить тик
			return;
		}

		gbNeedPostTabSend = FALSE;

		CESERVER_REQ* pOut =
			ExecuteGuiCmd(FarHwnd, gpTabs, FarHwnd);
		if (pOut) {
			if (pOut->hdr.nSize >= (sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_CONEMUTAB_RET))) {
				if (gpTabs->Tabs.bMacroActive && pOut->TabsRet.bNeedPostTabSend) {
					// Отослать после того, как макрос завершится
					gbNeedPostTabSend = TRUE;
					gnNeedPostTabSendTick = GetTickCount();
				} else if (pOut->TabsRet.bNeedResize) {
					// Если это отложенная отсылка табов после выполнения макросов
					if (GetCurrentThreadId() == gnMainThreadId) {
						SetConsoleSize(pOut->TabsRet.crNewSize.X, pOut->TabsRet.crNewSize.Y);
					}
				}
			}
			ExecuteFreeResult(pOut);
		}
	}
    
	SC.Unlock();
}

// watch non-modified -> modified editor status change

int lastModifiedStateW = -1;
bool gbHandleOneRedraw = false; //, gbHandleOneRedrawCh = false;

int WINAPI _export ProcessEditorInputW(void* Rec)
{
	// Даже если мы не под эмулятором - просто запомним текущее состояние
	//if (!ConEmuHwnd) return 0; // Если мы не под эмулятором - ничего
	if (gFarVersion.dwBuild>=FAR_Y_VER)
		return FUNC_Y(ProcessEditorInputW)((LPCVOID)Rec);
	else
		return FUNC_X(ProcessEditorInputW)((LPCVOID)Rec);
}

int WINAPI _export ProcessEditorEventW(int Event, void *Param)
{
	// Даже если мы не под эмулятором - просто запомним текущее состояние
	//if (!ConEmuHwnd) return 0; // Если мы не под эмулятором - ничего
	/*if (gFarVersion.dwBuild>=FAR_Y_VER)
		return FUNC_Y(ProcessEditorEventW)(Event,Param);
	else
		return FUNC_X(ProcessEditorEventW)(Event,Param);*/
	//static bool sbEditorReading = false;

	if (gbNeedPostTabSend && Event == EE_REDRAW) {
		if (!IsMacroActive())
			gbHandleOneRedraw = true;
	}

	// Вроде коды событий не различаются, да и от ANSI не отличаются...
	switch (Event)
	{
	case EE_READ: // в этот момент количество окон еще не изменилось
		gbHandleOneRedraw = true;
		//gbHandleOneRedrawCh = false;
		return 0;
	case EE_REDRAW:
		if (!gbHandleOneRedraw)
			return 0;
		//if (!gbHandleOneRedrawCh)//2009-08-17 - сбрасываем в UpdateConEmuTabsW т.к. на Input не сразу * появляется
		//	gbHandleOneRedraw = false; //2009-08-17 - сбрасываем в UpdateConEmuTabsW т.к. на Input не сразу * появляется
		gbHandleOneRedraw = false;
		if (lastModifiedStateW == GetEditorModifiedState())
			return 0;
		OUTPUTDEBUGSTRING(L"EE_REDRAW(HandleOneRedraw)\n");
		break;
	case EE_CLOSE:
		OUTPUTDEBUGSTRING(L"EE_CLOSE\n"); break;
	case EE_GOTFOCUS:
		OUTPUTDEBUGSTRING(L"EE_GOTFOCUS\n"); break;
	case EE_KILLFOCUS:
		OUTPUTDEBUGSTRING(L"EE_KILLFOCUS\n"); break;
	case EE_SAVE:
		gbHandleOneRedraw = true;
		OUTPUTDEBUGSTRING(L"EE_SAVE\n"); break;
	default:
		return 0;
	}
	// !!! Именно UpdateConEmuTabsW, без версии !!!
	//2009-06-03 EE_KILLFOCUS при закрытии редактора не приходит. Только EE_CLOSE
	bool loosingFocus = (Event == EE_KILLFOCUS) || (Event == EE_CLOSE);
	UpdateConEmuTabsW(Event+100, loosingFocus, Event == EE_SAVE);
	return 0;
}

int WINAPI _export ProcessViewerEventW(int Event, void *Param)
{
	// Даже если мы не под эмулятором - просто запомним текущее состояние
	//if (!ConEmuHwnd) return 0; // Если мы не под эмулятором - ничего
	/*if (gFarVersion.dwBuild>=FAR_Y_VER)
		return FUNC_Y(ProcessViewerEventW)(Event,Param);
	else
		return FUNC_X(ProcessViewerEventW)(Event,Param);*/
	// Вроде коды событий не различаются, да и от ANSI не отличаются...
	switch (Event)
	{
	case VE_CLOSE:
		OUTPUTDEBUGSTRING(L"VE_CLOSE"); break;
	//case VE_READ:
	//	OUTPUTDEBUGSTRING(L"VE_CLOSE"); break;
	case VE_KILLFOCUS:
		OUTPUTDEBUGSTRING(L"VE_KILLFOCUS"); break;
	case VE_GOTFOCUS:
		OUTPUTDEBUGSTRING(L"VE_GOTFOCUS"); break;
	default:
		return 0;
	}
	// !!! Именно UpdateConEmuTabsW, без версии !!!
	//2009-06-03 VE_KILLFOCUS при закрытии редактора не приходит. Только VE_CLOSE
	bool loosingFocus = (Event == VE_KILLFOCUS || Event == VE_CLOSE);
	UpdateConEmuTabsW(Event+200, loosingFocus, false, Param);
	return 0;
}

void StopThread(void)
{
	CloseTabs();

	//if (hEventCmd[CMD_EXIT])
	//	SetEvent(hEventCmd[CMD_EXIT]); // Завершить нить

	if (ghServerTerminateEvent) {
		SetEvent(ghServerTerminateEvent);
	}
	if (gnInputThreadId) {
		PostThreadMessage(gnInputThreadId, WM_QUIT, 0, 0);
	}

	if (ghServerThread) {
		HANDLE hPipe = INVALID_HANDLE_VALUE;
		DWORD dwWait = 0;
		// Передернуть пайп, чтобы нить сервера завершилась
		OutputDebugString(L"Plugin: Touching our server pipe\n");
		hPipe = CreateFile(gszPluginServerPipe,GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
		if (hPipe == INVALID_HANDLE_VALUE) {
			OutputDebugString(L"Plugin: All pipe instances closed?\n");
		} else {
			OutputDebugString(L"Plugin: Waiting server pipe thread\n");
			dwWait = WaitForSingleObject(ghServerThread, 300); // пытаемся дождаться, пока нить завершится
			// Просто закроем пайп - его нужно было передернуть
			CloseHandle(hPipe);
			hPipe = INVALID_HANDLE_VALUE;
		}
		dwWait = WaitForSingleObject(ghServerThread, 0);
		if (dwWait != WAIT_OBJECT_0) {
			#if !defined(__GNUC__)
			#pragma warning (disable : 6258)
			#endif
			TerminateThread(ghServerThread, 100);
		}
		SafeCloseHandle(ghServerThread);
	}
	SafeCloseHandle(ghPluginSemaphore);

	if (ghMonitorThread) { // подождем чуть-чуть, или принудительно прибъем нить ожидания
		if (WaitForSingleObject(ghMonitorThread,1000)) {
			#if !defined(__GNUC__)
			#pragma warning (disable : 6258)
			#endif
			TerminateThread(ghMonitorThread, 100);
		}
		SafeCloseHandle(ghMonitorThread);
	}

	if (ghInputThread) { // подождем чуть-чуть, или принудительно прибъем нить ожидания
		if (WaitForSingleObject(ghInputThread,1000)) {
			#if !defined(__GNUC__)
			#pragma warning (disable : 6258)
			#endif
			TerminateThread(ghInputThread, 100);
		}
		SafeCloseHandle(ghInputThread);
	}

    if (gpTabs) {
	    Free(gpTabs);
	    gpTabs = NULL;
    }
    
    if (ghReqCommandEvent) {
	    CloseHandle(ghReqCommandEvent); ghReqCommandEvent = NULL;
    }

	//DeleteCriticalSection(&csTabs); memset(&csTabs,0,sizeof(csTabs));
	DeleteCriticalSection(&csData); memset(&csData,0,sizeof(csData));
	
	if (ghRegMonitorKey) { RegCloseKey(ghRegMonitorKey); ghRegMonitorKey = NULL; }
	SafeCloseHandle(ghRegMonitorEvt);
	SafeCloseHandle(ghServerTerminateEvent);
	SafeCloseHandle(ghConIn);
	SafeCloseHandle(ghInputSynchroExecuted);
	SafeCloseHandle(ghSetWndSendTabsEvent);
}

void   WINAPI _export ExitFARW(void)
{
	UnsetAllHooks();
	
	StopThread();

	if (gFarVersion.dwBuild>=FAR_Y_VER)
		FUNC_Y(ExitFARW)();
	else
		FUNC_X(ExitFARW)();
}

void CheckResources()
{
	if (gsFarLang[0]) {
		static DWORD dwLastTickCount = GetTickCount();
		DWORD dwCurTick = GetTickCount();
		if ((dwCurTick - dwLastTickCount) < 5000)
			return;
		dwLastTickCount = dwCurTick;
	}

	wchar_t szLang[64];
	GetEnvironmentVariable(L"FARLANG", szLang, 63);
	if (lstrcmpW(szLang, gsFarLang)) {
		wchar_t szTitle[1024] = {0};
		GetConsoleTitleW(szTitle, 1024);
		SetConsoleTitleW(L"ConEmuC: CheckResources started");

		InitResources();
		
		DWORD dwServerPID = 0;
		FindServerCmd(CECMD_FARLOADED, dwServerPID);
		gdwServerPID = dwServerPID;

		SetConsoleTitleW(szTitle);
	}
}

// Передать в ConEmu строки с ресурсами
void InitResources()
{
	if (!ConEmuHwnd || !FarHwnd) return;
	// В ConEmu нужно передать следущие ресурсы
	//
	int nSize = sizeof(CESERVER_REQ) + 2*sizeof(DWORD) 
		+ 3*(MAX_PATH+1)*2; // + 3 строковых ресурса
	CESERVER_REQ *pIn = (CESERVER_REQ*)Alloc(nSize,1);;
	if (pIn) {
		ExecutePrepareCmd(pIn, CECMD_RESOURCES, nSize);
		pIn->dwData[0] = GetCurrentProcessId();
		pIn->dwData[1] = gnInputThreadId;
		wchar_t* pszRes = (wchar_t*)&(pIn->dwData[2]);
		if (gFarVersion.dwVerMajor==1) {
			GetMsgA(10, pszRes); pszRes += lstrlenW(pszRes)+1;
			GetMsgA(11, pszRes); pszRes += lstrlenW(pszRes)+1;
			GetMsgA(12, pszRes); pszRes += lstrlenW(pszRes)+1;
		} else {
			lstrcpyW(pszRes, GetMsgW(10)); pszRes += lstrlenW(pszRes)+1;
			lstrcpyW(pszRes, GetMsgW(11)); pszRes += lstrlenW(pszRes)+1;
			lstrcpyW(pszRes, GetMsgW(12)); pszRes += lstrlenW(pszRes)+1;
		}
		// Поправить nSize (он должен быть меньше)
		_ASSERTE(pIn->hdr.nSize >= (DWORD)(((LPBYTE)pszRes) - ((LPBYTE)pIn)));
		pIn->hdr.nSize = (DWORD)(((LPBYTE)pszRes) - ((LPBYTE)pIn));
		CESERVER_REQ* pOut = ExecuteGuiCmd(FarHwnd, pIn, FarHwnd);
		if (pOut) ExecuteFreeResult(pOut);
		Free(pIn);

		GetEnvironmentVariable(L"FARLANG", gsFarLang, 63);
	}
}

void CloseTabs()
{
	if (ConEmuHwnd && IsWindow(ConEmuHwnd) && FarHwnd) {
		CESERVER_REQ in; // Пустая команда - значит FAR закрывается
		ExecutePrepareCmd(&in, CECMD_TABSCHANGED, sizeof(CESERVER_REQ_HDR));

		CESERVER_REQ* pOut = ExecuteGuiCmd(FarHwnd, &in, FarHwnd);
		if (pOut) ExecuteFreeResult(pOut);
	}
}

// Если не вызывать - буфер увеличивается автоматически. Размер в БАЙТАХ
// Возвращает FALSE при ошибках выделения памяти
BOOL OutDataAlloc(DWORD anSize)
{
	_ASSERTE(gpCmdRet==NULL);
	// + размер заголовка gpCmdRet
	gpCmdRet = (CESERVER_REQ*)Alloc(sizeof(CESERVER_REQ_HDR)+anSize,1);
	if (!gpCmdRet)
		return FALSE;

	// Код команды пока не известен - установит вызывающая функция
	ExecutePrepareCmd(gpCmdRet, 0, anSize+sizeof(CESERVER_REQ_HDR));

	gpData = gpCmdRet->Data;
	gnDataSize = anSize;
	gpCursor = gpData;

	return TRUE;
}

// Размер в БАЙТАХ. вызывается автоматически из OutDataWrite
// Возвращает FALSE при ошибках выделения памяти
BOOL OutDataRealloc(DWORD anNewSize)
{
	if (!gpCmdRet)
		return OutDataAlloc(anNewSize);

	if (anNewSize < gnDataSize)
		return FALSE; // нельзя выделять меньше памяти, чем уже есть

	// realloc иногда не работает, так что даже и не пытаемся
	CESERVER_REQ* lpNewCmdRet = (CESERVER_REQ*)Alloc(sizeof(CESERVER_REQ_HDR)+anNewSize,1);
	if (!lpNewCmdRet)
		return FALSE;

	ExecutePrepareCmd(lpNewCmdRet, gpCmdRet->hdr.nCmd, anNewSize+sizeof(CESERVER_REQ_HDR));

	LPBYTE lpNewData = lpNewCmdRet->Data;
	if (!lpNewData)
		return FALSE;

	// скопировать существующие данные
	memcpy(lpNewData, gpData, gnDataSize);
	// запомнить новую позицию курсора
	gpCursor = lpNewData + (gpCursor - gpData);
	// И новый буфер с размером
	Free(gpCmdRet);
	gpCmdRet = lpNewCmdRet;
	gpData = lpNewData;
	gnDataSize = anNewSize;

	return TRUE;
}

// Размер в БАЙТАХ
// Возвращает FALSE при ошибках выделения памяти
BOOL OutDataWrite(LPVOID apData, DWORD anSize)
{
	if (!gpData) {
		if (!OutDataAlloc(max(1024, (anSize+128))))
			return FALSE;
	} else if (((gpCursor-gpData)+anSize)>gnDataSize) {
		if (!OutDataRealloc(gnDataSize+max(1024, (anSize+128))))
			return FALSE;
	}

	// Скопировать данные
	memcpy(gpCursor, apData, anSize);
	gpCursor += anSize;

	return TRUE;
}

int ShowMessage(int aiMsg, int aiButtons)
{
	if (gFarVersion.dwVerMajor==1)
		return ShowMessageA(aiMsg, aiButtons);
	else if (gFarVersion.dwBuild>=FAR_Y_VER)
		return FUNC_Y(ShowMessage)(aiMsg, aiButtons);
	else
		return FUNC_X(ShowMessage)(aiMsg, aiButtons);
}

LPCWSTR GetMsgW(int aiMsg)
{
	if (gFarVersion.dwVerMajor==1)
		return L"";
	else if (gFarVersion.dwBuild>=FAR_Y_VER)
		return FUNC_Y(GetMsg)(aiMsg);
	else
		return FUNC_X(GetMsg)(aiMsg);
}

void PostMacro(wchar_t* asMacro)
{
	if (!asMacro || !*asMacro)
		return;
		
	if (gFarVersion.dwVerMajor==1) {
		int nLen = lstrlenW(asMacro);
		char* pszMacro = (char*)Alloc(nLen+1,1);
		if (pszMacro) {
			WideCharToMultiByte(CP_OEMCP,0,asMacro,nLen+1,pszMacro,nLen+1,0,0);
			PostMacroA(pszMacro);
			Free(pszMacro);
		}
	} else if (gFarVersion.dwBuild>=FAR_Y_VER) {
		FUNC_Y(PostMacro)(asMacro);
	} else {
		FUNC_X(PostMacro)(asMacro);
	}
}

DWORD WINAPI ServerThread(LPVOID lpvParam) 
{ 
    BOOL fConnected = FALSE;
    DWORD dwErr = 0;
    HANDLE hPipe = NULL; 
    //HANDLE hWait[2] = {NULL,NULL};
	//DWORD dwTID = GetCurrentThreadId();
	std::vector<HANDLE>::iterator iter;

    _ASSERTE(gszPluginServerPipe[0]!=0);
    //_ASSERTE(ghServerSemaphore!=NULL);

    // The main loop creates an instance of the named pipe and 
    // then waits for a client to connect to it. When the client 
    // connects, a thread is created to handle communications 
    // with that client, and the loop is repeated. 
    
    //hWait[0] = ghServerTerminateEvent;
    //hWait[1] = ghServerSemaphore;

    // Пока не затребовано завершение консоли
    do {
		fConnected = FALSE; // Новый пайп
        while (!fConnected)
        { 
            _ASSERTE(hPipe == NULL);

			// Проверить, может какие-то командные нити уже завершились
			iter = ghCommandThreads.begin();
			while (iter != ghCommandThreads.end()) {
				HANDLE hThread = *iter; dwErr = 0;
				if (WaitForSingleObject(hThread, 0) == WAIT_OBJECT_0) {
					CloseHandle ( hThread );
					iter = ghCommandThreads.erase(iter);
				} else {
					iter++;
				}
			}

            //// Дождаться разрешения семафора, или закрытия консоли
            //dwErr = WaitForMultipleObjects ( 2, hWait, FALSE, INFINITE );
            //if (dwErr == WAIT_OBJECT_0) {
            //    return 0; // Консоль закрывается
            //}

			//for (int i=0; i<MAX_SERVER_THREADS; i++) {
			//	if (gnServerThreadsId[i] == dwTID) {
			//		ghActiveServerThread = ghServerThreads[i]; break;
			//	}
			//}

            hPipe = CreateNamedPipe( gszPluginServerPipe, PIPE_ACCESS_DUPLEX, 
				PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES,
                PIPEBUFSIZE, PIPEBUFSIZE, 0, gpNullSecurity);

            _ASSERTE(hPipe != INVALID_HANDLE_VALUE);

            if (hPipe == INVALID_HANDLE_VALUE) 
            {
                //DisplayLastError(L"CreateNamedPipe failed"); 
                hPipe = NULL;
                Sleep(50);
                continue;
            }

            // Wait for the client to connect; if it succeeds, 
            // the function returns a nonzero value. If the function
            // returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 

            fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : ((dwErr = GetLastError()) == ERROR_PIPE_CONNECTED); 

            // Консоль закрывается!
            if (WaitForSingleObject ( ghServerTerminateEvent, 0 ) == WAIT_OBJECT_0) {
                //FlushFileBuffers(hPipe); -- это не нужно, мы ничего не возвращали
                //DisconnectNamedPipe(hPipe); 
				//ReleaseSemaphore(ghServerSemaphore, 1, NULL);
                SafeCloseHandle(hPipe);
                goto wrap;
            }

            if (!fConnected)
                SafeCloseHandle(hPipe);
        }

        if (fConnected) {
            // сразу сбросим, чтобы не забыть
            //fConnected = FALSE;
            // разрешить другой нити принять вызов
            //ReleaseSemaphore(ghServerSemaphore, 1, NULL);
            
            //ServerThreadCommand ( hPipe ); // При необходимости - записывает в пайп результат сама
			DWORD dwThread = 0;
			HANDLE hThread = CreateThread(NULL, 0, ServerThreadCommand, (LPVOID)hPipe, 0, &dwThread);
			_ASSERTE(hThread!=NULL);
			if (hThread==NULL) {
				// Раз не удалось запустить нить - можно попробовать так обработать...
				ServerThreadCommand((LPVOID)hPipe);
			} else {
				ghCommandThreads.push_back ( hThread );
			}
			hPipe = NULL; // закрывает ServerThreadCommand
        }

		if (hPipe) {
			if (hPipe != INVALID_HANDLE_VALUE) {
				FlushFileBuffers(hPipe); 
				//DisconnectNamedPipe(hPipe); 
				CloseHandle(hPipe);
			}
			hPipe = NULL;
		}
    } // Перейти к открытию нового instance пайпа
    while (WaitForSingleObject ( ghServerTerminateEvent, 0 ) != WAIT_OBJECT_0);

wrap:
	// Прибивание всех запущенных нитей
	iter = ghCommandThreads.begin();
	while (iter != ghCommandThreads.end()) {
		HANDLE hThread = *iter; dwErr = 0;
		if (WaitForSingleObject(hThread, 100) != WAIT_OBJECT_0) {
			TerminateThread(hThread, 100);
		}
		CloseHandle ( hThread );
		iter = ghCommandThreads.erase(iter);
	}

    return 0; 
}

DWORD WINAPI ServerThreadCommand(LPVOID ahPipe)
{
	HANDLE hPipe = (HANDLE)ahPipe;
    CESERVER_REQ *pIn=NULL;
	BYTE cbBuffer[64]; // Для большей части команд нам хватит
    DWORD cbRead = 0, cbWritten = 0, dwErr = 0;
    BOOL fSuccess = FALSE;

	MSectionThread SCT(&csTabs);

    // Send a message to the pipe server and read the response. 
    fSuccess = ReadFile( hPipe, cbBuffer, sizeof(cbBuffer), &cbRead, NULL);
	dwErr = GetLastError();

    if (!fSuccess && (dwErr != ERROR_MORE_DATA)) 
    {
        _ASSERTE("ReadFile(pipe) failed"==NULL);
        CloseHandle(hPipe);
        return 0;
    }
	pIn = (CESERVER_REQ*)cbBuffer; // Пока cast, если нужно больше - выделим память
    _ASSERTE(pIn->hdr.nSize>=sizeof(CESERVER_REQ_HDR) && cbRead>=sizeof(CESERVER_REQ_HDR));
    _ASSERTE(pIn->hdr.nVersion == CESERVER_REQ_VER);
    if (cbRead < sizeof(CESERVER_REQ_HDR) || /*in.nSize < cbRead ||*/ pIn->hdr.nVersion != CESERVER_REQ_VER) {
        CloseHandle(hPipe);
        return 0;
    }

    int nAllSize = pIn->hdr.nSize;
    pIn = (CESERVER_REQ*)Alloc(nAllSize,1);
    _ASSERTE(pIn!=NULL);
	if (!pIn) {
		CloseHandle(hPipe);
		return 0;
	}
    memmove(pIn, cbBuffer, cbRead);
    _ASSERTE(pIn->hdr.nVersion==CESERVER_REQ_VER);

    LPBYTE ptrData = ((LPBYTE)pIn)+cbRead;
    nAllSize -= cbRead;

    while(nAllSize>0)
    { 
        //_tprintf(TEXT("%s\n"), chReadBuf);

        // Break if TransactNamedPipe or ReadFile is successful
        if(fSuccess)
            break;

        // Read from the pipe if there is more data in the message.
        fSuccess = ReadFile( 
            hPipe,      // pipe handle 
            ptrData,    // buffer to receive reply 
            nAllSize,   // size of buffer 
            &cbRead,    // number of bytes read 
            NULL);      // not overlapped 

        // Exit if an error other than ERROR_MORE_DATA occurs.
        if( !fSuccess && ((dwErr = GetLastError()) != ERROR_MORE_DATA)) 
            break;
        ptrData += cbRead;
        nAllSize -= cbRead;
    }

    TODO("Может возникнуть ASSERT, если консоль была закрыта в процессе чтения");
    _ASSERTE(nAllSize==0);
    if (nAllSize>0) {
		if (((LPVOID)cbBuffer) != ((LPVOID)pIn))
			Free(pIn);
        CloseHandle(hPipe);
        return 0; // удалось считать не все данные
    }

	UINT nDataSize = pIn->hdr.nSize - sizeof(CESERVER_REQ_HDR);

    // Все данные из пайпа получены, обрабатываем команду и возвращаем (если нужно) результат
	//fSuccess = WriteFile( hPipe, pOut, pOut->nSize, &cbWritten, NULL);

	if (pIn->hdr.nCmd == CMD_LANGCHANGE) {
		_ASSERTE(nDataSize>=4);
		// LayoutName: "00000409", "00010409", ...
		// А HKL от него отличается, так что передаем DWORD
		// HKL в x64 выглядит как: "0x0000000000020409", "0xFFFFFFFFF0010409"
		DWORD hkl = pIn->dwData[0];
		if (hkl) {
			DWORD dwLastError = 0;
			WCHAR szLoc[10]; wsprintf(szLoc, L"%08x", hkl);
			HKL hkl1 = LoadKeyboardLayout(szLoc, KLF_ACTIVATE|KLF_REORDER|KLF_SUBSTITUTE_OK|KLF_SETFORPROCESS);
			HKL hkl2 = ActivateKeyboardLayout(hkl1, KLF_SETFORPROCESS|KLF_REORDER);
			if (!hkl2) {
				dwLastError = GetLastError();
			}
			dwLastError = dwLastError;
		}

	//} else if (pIn->hdr.nCmd == CMD_DEFFONT) {
	//	// исключение - асинхронный, результат не требуется
	//	SetConsoleFontSizeTo(FarHwnd, 4, 6);
	//	MoveWindow(FarHwnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 1); // чтобы убрать возможные полосы прокрутки...

	} else if (pIn->hdr.nCmd == CMD_REQTABS || pIn->hdr.nCmd == CMD_SETWINDOW) {
		MSectionLock SC; SC.Lock(&csTabs, FALSE, 1000);

		if (pIn->hdr.nCmd == CMD_SETWINDOW) {
			ResetEvent(ghSetWndSendTabsEvent);

			ProcessCommand(pIn->hdr.nCmd, TRUE/*bReqMainThread*/, pIn->Data);

			if (gFarVersion.dwVerMajor == 2) {
				WaitForSingleObject(ghSetWndSendTabsEvent, 2000);
			}
		}

		if (gpTabs) { 
			fSuccess = WriteFile( hPipe, gpTabs, gpTabs->hdr.nSize, &cbWritten, NULL);
		}

		SC.Unlock();

	} else if (pIn->hdr.nCmd == CMD_SETENVVAR) {
		// Установить переменные окружения
		// Плагин это получает в ответ на CECMD_RESOURCES, посланное в GUI при загрузке плагина
		_ASSERTE(nDataSize>=8);
		//wchar_t *pszName  = (wchar_t*)pIn->Data;
		
	    FAR_REQ_SETENVVAR *pSetEnvVar = (FAR_REQ_SETENVVAR*)pIn->Data;
	    wchar_t *pszName = pSetEnvVar->szEnv;
	    
	    gbFARuseASCIIsort = pSetEnvVar->bFARuseASCIIsort;
		

		_ASSERTE(nDataSize<sizeof(gsMonitorEnvVar));
		gbMonitorEnvVar = false;
		// Плагин FarCall "нарушает" COMSPEC (копирует содержимое запускаемого процесса)
		bool lbOk = false;
		if (nDataSize<sizeof(gsMonitorEnvVar)) {
			memcpy(gsMonitorEnvVar, pszName, nDataSize);
			lbOk = true;
		}

		UpdateEnvVar(pszName);

		//while (*pszName && *pszValue) {
		//	const wchar_t* pszChanged = pszValue;
		//	// Для ConEmuOutput == AUTO выбирается по версии ФАРа
		//	if (!lstrcmpi(pszName, L"ConEmuOutput") && !lstrcmp(pszChanged, L"AUTO")) {
		//		if (gFarVersion.dwVerMajor==1)
		//			pszChanged = L"ANSI";
		//		else
		//			pszChanged = L"UNICODE";
		//	}
		//	// Если в pszValue пустая строка - удаление переменной
		//	SetEnvironmentVariableW(pszName, (*pszChanged != 0) ? pszChanged : NULL);
		//	//
		//	pszName = pszValue + lstrlenW(pszValue) + 1;
		//	if (*pszName == 0) break;
		//	pszValue = pszName + lstrlenW(pszName) + 1;
		//}

		gbMonitorEnvVar = lbOk;
		
	} else if (pIn->hdr.nCmd == CMD_EMENU) {
		#ifdef _DEBUG
		COORD *crMouse = (COORD *)pIn->Data;
		const wchar_t *pszUserMacro = (wchar_t*)(crMouse+1);
		#endif

		// Выделить файл под курсором
		ProcessCommand(CMD_LEFTCLKSYNC, TRUE/*bReqMainThread*/, pIn->Data);
		
		// А теперь, собственно вызовем меню
		ProcessCommand(pIn->hdr.nCmd, TRUE/*bReqMainThread*/, pIn->Data);


	} else {
		CESERVER_REQ* pCmdRet = ProcessCommand(pIn->hdr.nCmd, TRUE/*bReqMainThread*/, pIn->Data);

		if (pCmdRet) {
			fSuccess = WriteFile( hPipe, pCmdRet, pCmdRet->hdr.nSize, &cbWritten, NULL);
		}
		if (gpCmdRet && gpCmdRet == pCmdRet) {
			Free(gpCmdRet);
			gpCmdRet = NULL; gpData = NULL; gpCursor = NULL;
		}
	}


    // Освободить память
	if (((LPVOID)cbBuffer) != ((LPVOID)pIn))
		Free(pIn);

    CloseHandle(hPipe);
    return 0;
}

void ShowPluginMenu(int nID /*= -1*/)
{
	int nItem = -1;
	if (!FarHwnd)
		return;

	if (nID != -1) {
		nItem = nID;
		if (nItem >= 2) nItem++; //Separator
		if (nItem >= 7) nItem++; //Separator
		if (nItem >= 9) nItem++; //Separator
	} else if (gFarVersion.dwVerMajor==1)
		nItem = ShowPluginMenuA();
	else if (gFarVersion.dwBuild>=FAR_Y_VER)
		nItem = FUNC_Y(ShowPluginMenu)();
	else
		nItem = FUNC_X(ShowPluginMenu)();

	if (nItem < 0)
		return;

	switch (nItem) {
		case 0: case 1:
		{ // Открыть в редакторе вывод последней консольной программы
			CESERVER_REQ* pIn = (CESERVER_REQ*)calloc(sizeof(CESERVER_REQ_HDR)+4,1);
			if (!pIn) return;
			CESERVER_REQ* pOut = NULL;
			ExecutePrepareCmd(pIn, CECMD_GETOUTPUTFILE, sizeof(CESERVER_REQ_HDR)+4);
			pIn->OutputFile.bUnicode = (gFarVersion.dwVerMajor>=2);
			pOut = ExecuteGuiCmd(FarHwnd, pIn, FarHwnd);
			if (pOut) {
				if (pOut->OutputFile.szFilePathName[0]) {
					BOOL lbRc = FALSE;
					if (gFarVersion.dwVerMajor==1)
						lbRc = EditOutputA(pOut->OutputFile.szFilePathName, (nItem==1));
					else if (gFarVersion.dwBuild>=FAR_Y_VER)
						lbRc = FUNC_Y(EditOutput)(pOut->OutputFile.szFilePathName, (nItem==1));
					else
						lbRc = FUNC_X(EditOutput)(pOut->OutputFile.szFilePathName, (nItem==1));
					if (!lbRc) {
						DeleteFile(pOut->OutputFile.szFilePathName);
					}
				}
				ExecuteFreeResult(pOut);
			}
			free(pIn);
		} break;
		case 3: // Показать/спрятать табы
		case 4: case 5: case 6:
		{
			CESERVER_REQ in, *pOut = NULL;
			ExecutePrepareCmd(&in, CECMD_TABSCMD, sizeof(CESERVER_REQ_HDR)+1);
			in.Data[0] = nItem - 3;
			pOut = ExecuteGuiCmd(FarHwnd, &in, FarHwnd);
			if (pOut) ExecuteFreeResult(pOut);
		} break;
		case 8: // Attach to GUI (если FAR был CtrlAltTab)
		{
			if (TerminalMode) break; // низзя
			if (ConEmuHwnd && IsWindow(ConEmuHwnd)) break; // Мы и так подключены?
			Attach2Gui();
		} break;
		case 10: // Start "ConEmuC.exe /DEBUGPID="
		{
			if (TerminalMode) break; // низзя
			StartDebugger();
		}
	}
}

BOOL FindServerCmd(DWORD nServerCmd, DWORD &dwServerPID)
{
	BOOL lbRc = FALSE;
	DWORD nProcessCount = 0, nProcesses[100] = {0};
	
	dwServerPID = 0;
	
	typedef DWORD (WINAPI* FGetConsoleProcessList)(LPDWORD lpdwProcessList, DWORD dwProcessCount);
	FGetConsoleProcessList pfnGetConsoleProcessList = NULL;
    HMODULE hKernel = GetModuleHandleW (L"kernel32.dll");
    
    if (hKernel) {
        pfnGetConsoleProcessList = (FGetConsoleProcessList)GetProcAddress (hKernel, "GetConsoleProcessList");
    }

    if (pfnGetConsoleProcessList) {
    	nProcessCount = pfnGetConsoleProcessList(nProcesses, countof(nProcesses));
    	if (nProcessCount && nProcessCount > countof(nProcesses)) {
    		_ASSERTE(nProcessCount <= countof(nProcesses));
    		nProcessCount = 0;
    	}
    }
	
	if (nProcessCount >= 2) {
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
		if (hSnap != INVALID_HANDLE_VALUE) {
			PROCESSENTRY32 prc = {sizeof(PROCESSENTRY32)};
			DWORD nSelfPID = GetCurrentProcessId();
			if (Process32First(hSnap, &prc)) {
				do {
            		for (UINT i = 0; i < nProcessCount; i++) {
            			if (prc.th32ProcessID != nSelfPID
            				&& prc.th32ProcessID == nProcesses[i])
        				{
        					if (lstrcmpiW(prc.szExeFile, L"conemuc.exe")==0) {
        						CESERVER_REQ* pIn = ExecuteNewCmd(nServerCmd, sizeof(CESERVER_REQ_HDR)+sizeof(DWORD));
        						pIn->dwData[0] = GetCurrentProcessId();
        						CESERVER_REQ* pOut = ExecuteSrvCmd(prc.th32ProcessID, pIn, FarHwnd);
        						if (pOut) dwServerPID = prc.th32ProcessID;
        						ExecuteFreeResult(pIn); ExecuteFreeResult(pOut);
        						// Если команда успешно выполнена - выходим
        						if (dwServerPID) {
        							lbRc = TRUE;
        							break;
    							}
        					}
            			}
            		}
				} while (!dwServerPID && Process32Next(hSnap, &prc));
			}
			CloseHandle(hSnap);
		}
	}
	
	return lbRc;
}
	
BOOL Attach2Gui()
{
	DWORD dwServerPID = 0;
	
	if (FindServerCmd(CECMD_ATTACH2GUI, dwServerPID) && dwServerPID != 0) {
		// "Server was already started. PID=%i. Exiting...\n", dwServerPID
		gdwServerPID = dwServerPID;
		return TRUE;
	}
	gdwServerPID = 0;
	
	// Create process, with flag /Attach GetCurrentProcessId()
	// Sleep for sometimes, try InitHWND(hConWnd); several times
	WCHAR  szExe[0x200] = {0};
	BOOL lbRc = FALSE;
	
	DWORD nLen = 0;
	PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
	STARTUPINFO si; memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	DWORD dwSelfPID = GetCurrentProcessId();
	
	szExe[0] = L'"';
	if ((nLen = GetEnvironmentVariableW(L"ConEmuDir", szExe+1, MAX_PATH)) > 0) {
		if (szExe[nLen] != L'\\') { szExe[nLen+1] = L'\\'; szExe[nLen+2] = 0; }
	} else if ((nLen=GetModuleFileName(0, szExe+1, MAX_PATH)) > 0) {
		wchar_t* pszSlash = wcsrchr ( szExe, L'\\' );
		if (pszSlash) pszSlash[1] = 0;
	}
	
	wsprintf(szExe+wcslen(szExe), L"ConEmuC.exe\" /ATTACH /PID=%i", dwSelfPID);
	
	
	if (!CreateProcess(NULL, szExe, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL,
			NULL, &si, &pi))
	{
		// Хорошо бы ошибку показать?
	} else {
		gdwServerPID = pi.dwProcessId;
		lbRc = TRUE;
	}
	
	return lbRc;
}

BOOL StartDebugger()
{
	if (IsDebuggerPresent()) {
		ShowMessage(15,1); // "ConEmu plugin\nDebugger is already attached to current process\nOK"
		return FALSE; // Уже
	}
	if (IsTerminalMode()) {
		ShowMessage(16,1); // "ConEmu plugin\nDebugger is not available in terminal mode\nOK"
		return FALSE; // Уже
	}
		
	DWORD dwServerPID = 0;
	
	// Create process, with flag /Attach GetCurrentProcessId()
	// Sleep for sometimes, try InitHWND(hConWnd); several times
	WCHAR  szExe[0x200] = {0};
	BOOL lbRc = FALSE;
	
	DWORD nLen = 0;
	PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
	STARTUPINFO si; memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	DWORD dwSelfPID = GetCurrentProcessId();
	
	szExe[0] = L'"';
	if ((nLen = GetEnvironmentVariableW(L"ConEmuDir", szExe+1, MAX_PATH)) > 0) {
		if (szExe[nLen] != L'\\') { szExe[nLen+1] = L'\\'; szExe[nLen+2] = 0; }
	} else if ((nLen=GetModuleFileName(0, szExe+1, MAX_PATH)) > 0) {
		wchar_t* pszSlash = wcsrchr ( szExe, L'\\' );
		if (pszSlash) pszSlash[1] = 0;
	}
	
	wsprintf(szExe+wcslen(szExe), L"ConEmuC.exe\" %s /DEBUGPID=%i /BW=80 /BH=25 /BZ=1000", 
		ConEmuHwnd ? L"/ATTACH" : L"", dwSelfPID);
	
	if (ConEmuHwnd) {
		si.dwFlags |= STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
	}
	
	if (!CreateProcess(NULL, szExe, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS|CREATE_NEW_CONSOLE, NULL,
			NULL, &si, &pi))
	{
		// Хорошо бы ошибку показать?
		DWORD dwErr = GetLastError();
		ShowMessage(17,1); // "ConEmu plugin\nНе удалось запустить процесс отладчика\nOK"
	} else {
		lbRc = TRUE;
	}
	
	return lbRc;
}


BOOL IsMacroActive()
{
	if (!FarHwnd) return FALSE;

	BOOL lbActive = FALSE;
	if (gFarVersion.dwVerMajor==1)
		lbActive = IsMacroActiveA();
	else if (gFarVersion.dwBuild>=FAR_Y_VER)
		lbActive = FUNC_Y(IsMacroActive)();
	else
		lbActive = FUNC_X(IsMacroActive)();

	return lbActive;
}


void RedrawAll()
{
	if (!FarHwnd) return;

	if (gFarVersion.dwVerMajor==1)
		RedrawAllA();
	else if (gFarVersion.dwBuild>=FAR_Y_VER)
		FUNC_Y(RedrawAll)();
	else
		FUNC_X(RedrawAll)();
}

DWORD GetEditorModifiedState()
{
	if (gFarVersion.dwVerMajor==1)
		return GetEditorModifiedStateA();
	else if (gFarVersion.dwBuild>=FAR_Y_VER)
		return FUNC_Y(GetEditorModifiedState)();
	else
		return FUNC_X(GetEditorModifiedState)();
}



// <Name>\0<Value>\0<Name2>\0<Value2>\0\0
void UpdateEnvVar(const wchar_t* pszList)
{
	const wchar_t *pszName  = (wchar_t*)pszList;
	const wchar_t *pszValue = pszName + lstrlenW(pszName) + 1;

	while (*pszName && *pszValue) {
		const wchar_t* pszChanged = pszValue;
		// Для ConEmuOutput == AUTO выбирается по версии ФАРа
		if (!lstrcmpi(pszName, L"ConEmuOutput") && !lstrcmp(pszChanged, L"AUTO")) {
			if (gFarVersion.dwVerMajor==1)
				pszChanged = L"ANSI";
			else
				pszChanged = L"UNICODE";
		}
		// Если в pszValue пустая строка - удаление переменной
		SetEnvironmentVariableW(pszName, (*pszChanged != 0) ? pszChanged : NULL);
		//
		pszName = pszValue + lstrlenW(pszValue) + 1;
		if (*pszName == 0) break;
		pszValue = pszName + lstrlenW(pszName) + 1;
	}
}

/* Используются как extern в ConEmuCheck.cpp */
LPVOID _calloc(size_t nCount,size_t nSize) {
	return calloc(nCount,nSize);
}
LPVOID _malloc(size_t nCount) {
	return malloc(nCount);
}
void   _free(LPVOID ptr) {
	free(ptr);
}
