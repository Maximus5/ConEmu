/* ****************************************** 
   Changes history 
   Maximus5: убрал все static
   2009-01-31 Maximus5
	*) Переделать все вызовы InfoW.Control() – они радикально поменялись в последних юникодных версиях.
	*) ACTL_GETWINDOWINFO заменить на более эффективную ACTL_GETSHORTWINDOWINFO, тогда и не надо FREE вызывать. Это там, где строковые данные об окне не используются. А где используются, после ACTL_GETWINDOWINFO надо освобождать.
	*) После ECTL_GETINFO надо вызывать ECTL_FREEINFO.
****************************************** */

//#include <stdio.h>
#include <windows.h>
//#include <windowsx.h>
//#include <string.h>
//#include <tchar.h>
#include "..\common\common.hpp"
#include "..\common\pluginW1007.hpp"
#include "PluginHeader.h"
#include <vector>

extern "C" {
#include "../common/ConEmuCheck.h"
}

#define Free free
#define Alloc calloc

WARNING("Подозреваю, что в gszRootKey не учитывается имя пользователя/конфигурации");

#define MAKEFARVERSION(major,minor,build) ( ((major)<<8) | (minor) | ((build)<<16))

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



// minimal(?) FAR version 2.0 alpha build FAR_X_VER
int WINAPI _export GetMinFarVersionW(void)
{
	return MAKEFARVERSION(2,0,FAR_X_VER);
}

/* COMMON - пока структуры не различаются */
void WINAPI _export GetPluginInfoW(struct PluginInfo *pi)
{
    static WCHAR *szMenu[1], szMenu1[255];
    szMenu[0]=szMenu1; //lstrcpyW(szMenu[0], L"[&\x2560] ConEmu"); -> 0x2584
    //szMenu[0][1] = L'&';
    //szMenu[0][2] = 0x2560;

	// Проверить, не изменилась ли горячая клавиша плагина, и если да - пересоздать макросы
	IsKeyChanged(TRUE);

	if (gcPlugKey) szMenu1[0]=0; else lstrcpyW(szMenu1, L"[&\x2584] ");
	lstrcpynW(szMenu1+lstrlenW(szMenu1), GetMsgW(2), 240);


	pi->StructSize = sizeof(struct PluginInfo);
	pi->Flags = PF_EDITOR | PF_VIEWER | PF_DIALOG | PF_PRELOAD;
	pi->DiskMenuStrings = NULL;
	pi->DiskMenuNumbers = 0;
	pi->PluginMenuStrings = szMenu;
	pi->PluginMenuStringsNumber = 1;
	pi->PluginConfigStrings = NULL;
	pi->PluginConfigStringsNumber = 0;
	pi->CommandPrefix = NULL;
	pi->Reserved = 0;	
}


BOOL gbInfoW_OK = FALSE;
HANDLE WINAPI _export OpenPluginW(int OpenFrom,INT_PTR Item)
{
	if (!gbInfoW_OK)
		return INVALID_HANDLE_VALUE;

	if (gnReqCommand != (DWORD)-1) {
		gnPluginOpenFrom = OpenFrom;
		ProcessCommand(gnReqCommand, FALSE/*bReqMainThread*/, gpReqCommandData);
	} else {
		if (!gbCmdCallObsolete)
			ShowPluginMenu();
		else
			gbCmdCallObsolete = FALSE;
	}
	return INVALID_HANDLE_VALUE;
}

int WINAPI _export ProcessSynchroEventW(int Event,void *Param)
{
	if (Event == SE_COMMONSYNCHRO) {
    	if (!gbInfoW_OK)
    		return 0;

    	if (gnReqCommand != (DWORD)-1) {
    		TODO("Определить текущую область... (panel/editor/viewer/menu/...");
    		gnPluginOpenFrom = 0;
    		ProcessCommand(gnReqCommand, FALSE/*bReqMainThread*/, gpReqCommandData);
    	}
	}
	return 0;
}

/* COMMON - end */


HWND ConEmuHwnd=NULL; // Содержит хэндл окна отрисовки. Это ДОЧЕРНЕЕ окно.
BOOL TerminalMode = FALSE;
HWND FarHwnd=NULL;
DWORD gnMainThreadId = 0;
//HANDLE hEventCmd[MAXCMDCOUNT], hEventAlive=NULL, hEventReady=NULL;
HANDLE hThread=NULL;
FarVersion gFarVersion;
WCHAR gszDir1[CONEMUTABMAX], gszDir2[CONEMUTABMAX];
WCHAR gszRootKey[MAX_PATH*2];
int maxTabCount = 0, lastWindowCount = 0, gnCurTabCount = 0;
ConEmuTab* tabs = NULL; //(ConEmuTab*) Alloc(maxTabCount, sizeof(ConEmuTab));
LPBYTE gpData = NULL, gpCursor = NULL;
CESERVER_REQ* gpCmdRet = NULL;
DWORD  gnDataSize=0;
//HANDLE ghMapping = NULL;
DWORD gnReqCommand = -1;
int gnPluginOpenFrom = -1;
BOOL gbCmdCallObsolete = FALSE;
LPVOID gpReqCommandData = NULL;
HANDLE ghReqCommandEvent = NULL;
UINT gnMsgTabChanged = 0;
CRITICAL_SECTION csTabs, csData;
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
wchar_t gsFarLang[64];

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
				_ASSERTE(FAR_X_VER<FAR_Y_VER);
				#ifdef _DEBUG
				//if (!IsDebuggerPresent()) MessageBoxA(GetForegroundWindow(), "ConEmu.dll loaded", "ConEmu plugin", 0);
				#endif
				//#if defined(__GNUC__)
				//GetConsoleWindow = (FGetConsoleWindow)GetProcAddress(GetModuleHandle(L"kernel32.dll"),"GetConsoleWindow");
				//#endif
				HWND hConWnd = GetConsoleWindow();
				gnMainThreadId = GetCurrentThreadId();
				InitHWND(hConWnd);
				
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

WARNING("Обязательно сделать возможность отваливаться по таймауту, если плагин не удалось активировать");
// Проверку можно сделать чтением буфера ввода - если там еще есть событие отпускания F11 - значит
// меню плагинов еще загружается. Иначе можно еще чуть-чуть подождать, и отваливаться - активироваться не получится
void ProcessCommand(DWORD nCmd, BOOL bReqMainThread, LPVOID pCommandData)
{
	if (gpCmdRet) Free(gpCmdRet);
	gpCmdRet = NULL; gpData = NULL; gpCursor = NULL;

	WARNING("Тут нужно сделать проверку содержимого консоли");
	// Если отображено меню - плагин не запустится
	// Не перепутать меню с пустым экраном (Ctrl-O)

	if (bReqMainThread && (gnMainThreadId != GetCurrentThreadId())) {
		_ASSERTE(ghPluginSemaphore!=NULL);
		_ASSERTE(ghServerTerminateEvent!=NULL);
		// Засемафорить, чтобы несколько команд одновременно не пошли...
		HANDLE hEvents[2] = {ghServerTerminateEvent, ghPluginSemaphore};
		DWORD dwWait = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
		if (dwWait == WAIT_OBJECT_0) {
			// Плагин завершается
			return;
		}

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

		
		//HANDLE hEvents[2] = {ghReqCommandEvent, hEventCmd[CMD_EXIT]};
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
		if (dwWait) ResetEvent(ghReqCommandEvent); // Сразу сбросим, вдруг не дождались?

		ReleaseSemaphore(ghPluginSemaphore, 1, NULL);

		gpReqCommandData = NULL;
		gnReqCommand = -1; gnPluginOpenFrom = -1;
		return;
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

				if (gFarVersion.dwVerMajor==1)
					SetWindowA(nTab);
				else if (gFarVersion.dwBuild>=FAR_Y_VER)
					FUNC_Y(SetWindow)(nTab);
				else
					FUNC_X(SetWindow)(nTab);
			}
			SendTabs(gnCurTabCount, TRUE, TRUE);
			break;
		}
		case (CMD_POSTMACRO):
		{
			_ASSERTE(pCommandData!=NULL);
			if (pCommandData!=NULL)
				PostMacro((wchar_t*)pCommandData);
			break;
		}
	}

	LeaveCriticalSection(&csData);

	if (ghReqCommandEvent)
		SetEvent(ghReqCommandEvent);
}

// Эту нить нужно оставить, чтобы была возможность отобразить консоль при падении ConEmu
DWORD WINAPI ThreadProcW(LPVOID lpParameter)
{
	//DWORD dwProcId = GetCurrentProcessId();

	DWORD dwStartTick = GetTickCount();
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
					ShowWindowAsync(FarHwnd, SW_SHOWNORMAL);
					EnableWindow(FarHwnd, true);
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
					ShowWindowAsync(FarHwnd, SW_SHOWNORMAL);
					EnableWindow(FarHwnd, true);
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
						//	SendTabs(gnCurTabCount, TRUE);
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
				    
					ShowWindowAsync(FarHwnd, SW_SHOWNORMAL);
					EnableWindow(FarHwnd, true);
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

		//SafeCloseHandle(ghMapping);
		//// Поставим флажок, что мы приступили к обработке
		//// Хоть табы и не загружаются из фара, а пересылаются в текущем виде, но делается это обычным образом
		////if (dwWait != (WAIT_OBJECT_0+CMD_REQTABS)) // исключение - запрос табов. он асинхронный
		//SetEvent(hEventAlive);


		//switch (dwWait)
		//{
		//	case (WAIT_OBJECT_0+CMD_REQTABS):
		//	{
		//		/*if (!gnCurTabCount || !tabs) {
		//			CreateTabs(1);
		//			int nTabs=0; --низзя! это теперь идет через CriticalSection
		//			AddTab(nTabs, false, false, WTYPE_PANELS, 0,0,1,0); 
		//		}*/
		//		SendTabs(gnCurTabCount, TRUE);
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
		//EnterCriticalSection(&csData);
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

void WINAPI _export SetStartupInfoW(void *aInfo)
{
	if (!gFarVersion.dwVerMajor) LoadFarVersion();

	if (gFarVersion.dwBuild>=FAR_Y_VER)
		FUNC_Y(SetStartupInfoW)(aInfo);
	else
		FUNC_X(SetStartupInfoW)(aInfo);

	gbInfoW_OK = TRUE;

	CheckMacro(TRUE);

	CheckResources();
}

#define CREATEEVENT(fmt,h) \
		wsprintf(szEventName, fmt, dwCurProcId ); \
		h = CreateEvent(NULL, FALSE, FALSE, szEventName); \
		if (h==INVALID_HANDLE_VALUE) h=NULL;
	
void InitHWND(HWND ahFarHwnd)
{
	gsFarLang[0] = 0;
	InitializeCriticalSection(&csTabs);
	InitializeCriticalSection(&csData);
	LoadFarVersion(); // пригодится уже здесь!

	// начальная инициализация. в SetStartupInfo поправим
	wsprintfW(gszRootKey, L"Software\\%s", (gFarVersion.dwVerMajor==2) ? L"FAR2" : L"FAR");
	// Нужно учесть, что FAR мог запуститься с ключом /u (выбор конфигурации)
	wchar_t* pszUserSlash = gszRootKey+wcslen(gszRootKey);
	wcscpy(pszUserSlash, L"\\Users\\");
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



	
	//CREATEEVENT(CONEMUDRAGFROM, hEventCmd[CMD_DRAGFROM]);
	//CREATEEVENT(CONEMUDRAGTO, hEventCmd[CMD_DRAGTO]);
	//CREATEEVENT(CONEMUREQTABS, hEventCmd[CMD_REQTABS]);
	//CREATEEVENT(CONEMUSETWINDOW, hEventCmd[CMD_SETWINDOW]);
	//CREATEEVENT(CONEMUPOSTMACRO, hEventCmd[CMD_POSTMACRO]);
	//CREATEEVENT(CONEMUDEFFONT, hEventCmd[CMD_DEFFONT]);
	//CREATEEVENT(CONEMULANGCHANGE, hEventCmd[CMD_LANGCHANGE]);
	//CREATEEVENT(CONEMUEXIT, hEventCmd[CMD_EXIT]);
	//CREATEEVENT(CONEMUALIVE, hEventAlive);
	//CREATEEVENT(CONEMUREADY, hEventReady);

	hThread=CreateThread(NULL, 0, &ThreadProcW, 0, 0, 0);



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
		CreateTabs(1);
		AddTab(tabCount, true, false, WTYPE_PANELS, NULL, NULL, 0, 0);
		SendTabs(tabCount=1);
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
	
	//Прочитать назначенные плагинам клавиши, и если для ConEmu.dll указана клавиша активации - запомнить ее
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
			if (lstrcmpiW(pszSlash, L"/conemu.dll")==0) {
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

	//Прочитать назначенные плагинам клавиши, и если для ConEmu.dll указана клавиша активации - запомнить ее
	CheckPlugKey();
	//wsprintfW(szMacroKey[0], L"%s\\PluginHotkeys",
	//		gszRootKey, szCheckKey);
	//if (0==RegOpenKeyExW(HKEY_CURRENT_USER, szMacroKey[0], 0, KEY_READ, &hkey))
	//{
	//	DWORD dwIndex = 0, dwSize; FILETIME ft;
	//	while (0==RegEnumKeyEx(hkey, dwIndex++, szMacroKey[1], &(dwSize=MAX_PATH), NULL, NULL, NULL, &ft)) {
	//		WCHAR* pszSlash = szMacroKey[1]+lstrlenW(szMacroKey[1])-1;
	//		while (pszSlash>szMacroKey[1] && *pszSlash!=L'/') pszSlash--;
	//		if (lstrcmpiW(pszSlash, L"/conemu.dll")==0) {
	//			WCHAR lsFullPath[MAX_PATH*2];
	//			lstrcpy(lsFullPath, szMacroKey[0]);
	//			lstrcat(lsFullPath, L"\\");
	//			lstrcat(lsFullPath, szMacroKey[1]);
	//
	//			RegCloseKey(hkey); hkey=NULL;
	//
	//			if (0==RegOpenKeyExW(HKEY_CURRENT_USER, lsFullPath, 0, KEY_READ, &hkey)) {
	//				dwSize = sizeof(szCheckKey);
	//				if (0==RegQueryValueExW(hkey, L"Hotkey", NULL, NULL, (LPBYTE)szCheckKey, &dwSize)) {
	//					if (gFarVersion.dwVerMajor==1) {
	//						char cAnsi; // чтобы не возникло проблем с приведением к WCHAR
	//						WideCharToMultiByte(CP_OEMCP, 0, szCheckKey, 1, &cAnsi, 1, 0,0);
	//						gcPlugKey = cAnsi;
	//					} else {
	//						gcPlugKey = szCheckKey[0];
	//					}
	//				}
	//			}
	//			//
	//			//
	//			break;
	//		}
	//	}
	//
	//	// Закончили
	//	if (hkey) {RegCloseKey(hkey); hkey=NULL;}
	//}


	for (n=0; n<MODCOUNT; n++) {
		switch(n){
			case 0: lstrcpyW(szCheckKey, L"F14"); break;
			case 1: lstrcpyW(szCheckKey, L"CtrlF14"); break;
			case 2: lstrcpyW(szCheckKey, L"AltF14"); break;
			case 3: lstrcpyW(szCheckKey, L"ShiftF14"); break;
		}
		wsprintfW(szMacroKey[n], L"%s\\KeyMacros\\Common\\%s", gszRootKey, szCheckKey);
	}
	//lstrcpyA(szCheckKey, "F14DontCheck2");

	//if (0==RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\ConEmu", 0, KEY_ALL_ACCESS, &hkey))
	//{
	//
	//	if (RegQueryValueExA(hkey, szCheckKey, 0, 0, (LPBYTE)&lbMacroDontCheck, &(dwSize=sizeof(lbMacroDontCheck))))
	//		lbMacroDontCheck = false;
	//	RegCloseKey(hkey); hkey=NULL;
	//}

	/*if (gFarVersion.dwVerMajor==1) {
		lstrcpyA((char*)szCheckKey, "F11  ");
		((char*)szCheckKey)[4] = (char)((gcPlugKey ? gcPlugKey : 0xCC) & 0xFF);
		//lstrcpyW((wchar_t*)szCheckKey, L"F11  ");
		//szCheckKey[4] = (wchar_t)(gcPlugKey ? gcPlugKey : 0xCC);
	} else {*/
	lstrcpyW((wchar_t*)szCheckKey, L"F11  "); //TODO: для ANSI может другой код по умолчанию?
	szCheckKey[4] = (wchar_t)(gcPlugKey ? gcPlugKey : ((gFarVersion.dwVerMajor==1) ? 0x42C/*0xDC - аналог для OEM*/ : 0x2584));
	//}

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
				RegSetValueExA(hkey, "", 0, REG_SZ, (LPBYTE)szValue, (dwSize=strlen(szValue)+1));

				//lstrcpyA(szValue, 
				//	"$If (Shell || Info || QView || Tree || Viewer || Editor) F12 $Else waitkey(100) $End");
				//RegSetValueExA(hkey, "Sequence", 0, REG_SZ, (LPBYTE)szValue, (dwSize=strlen(szValue)+1));
				
				/*if (gFarVersion.dwVerMajor==1) {
					RegSetValueExA(hkey, "Sequence", 0, REG_SZ, (LPBYTE)szCheckKey, (dwSize=strlen((char*)szCheckKey)+1));
				} else {*/
					RegSetValueExW(hkey, L"Sequence", 0, REG_SZ, (LPBYTE)szCheckKey, (dwSize=2*(lstrlenW((WCHAR*)szCheckKey)+1)));
				//}

				lstrcpyA(szValue, "For ConEmu - plugin activation from listening thread");
				RegSetValueExA(hkey, "Description", 0, REG_SZ, (LPBYTE)szValue, (dwSize=strlen(szValue)+1));

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

	CheckResources();

	if (gFarVersion.dwBuild>=FAR_Y_VER)
		FUNC_Y(UpdateConEmuTabsW)(event, losingFocus, editorSave, Param);
	else
		FUNC_X(UpdateConEmuTabsW)(event, losingFocus, editorSave, Param);
}

BOOL CreateTabs(int windowCount)
{
	EnterCriticalSection(&csTabs);

	if ((tabs==NULL) || (maxTabCount <= (windowCount + 1)))
	{
		maxTabCount = windowCount + 10; // с запасом
		if (tabs) {
			Free(tabs); tabs = NULL;
		}
		
		tabs = (ConEmuTab*) Alloc(maxTabCount, sizeof(ConEmuTab));
	}
	
	lastWindowCount = windowCount;
	
	if (!tabs)
		LeaveCriticalSection(&csTabs);
	return tabs!=NULL;
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
	    lbCh = (tabs[0].Current != (losingFocus ? 1 : 0)) ||
	           (tabs[0].Type != WTYPE_PANELS);
		tabs[0].Current = losingFocus ? 1 : 0;
		//lstrcpyn(tabs[0].Name, FUNC_Y(GetMsgW)(0), CONEMUTABMAX-1);
		tabs[0].Name[0] = 0;
		tabs[0].Pos = 0;
		tabs[0].Type = WTYPE_PANELS;
		if (!tabCount) tabCount++;
	} else
	if (Type == WTYPE_EDITOR || Type == WTYPE_VIEWER)
	{
		// Первое окно - должно быть панели. Если нет - значит фар открыт в режиме редактора
		if (tabCount == 1) {
			// 04.06.2009 Maks - Не, чего-то не то... при открытии редактора из панелей - он заменяет панели
			//tabs[0].Type = Type;
		}

		// when receiving saving event receiver is still reported as modified
		if (editorSave && lstrcmpi(FileName, Name) == 0)
			Modified = 0;
	
	    lbCh = (tabs[tabCount].Current != (losingFocus ? 0 : Current)) ||
	           (tabs[tabCount].Type != Type) ||
	           (tabs[tabCount].Modified != Modified) ||
	           (lstrcmp(tabs[tabCount].Name, Name) != 0);
	
		// when receiving losing focus event receiver is still reported as current
		tabs[tabCount].Type = Type;
		tabs[tabCount].Current = losingFocus ? 0 : Current;
		tabs[tabCount].Modified = Modified;

		if (tabs[tabCount].Current != 0)
		{
			lastModifiedStateW = Modified != 0 ? 1 : 0;
		}
		else
		{
			lastModifiedStateW = -1;
		}

		int nLen = min(lstrlen(Name),(CONEMUTABMAX-1));
		lstrcpyn(tabs[tabCount].Name, Name, nLen+1);
		tabs[tabCount].Name[nLen]=0;

		tabs[tabCount].Pos = tabCount;
		tabCount++;
	}
	
	return lbCh;
}

void SendTabs(int tabCount, BOOL abFillDataOnly/*=FALSE*/, BOOL abForceSend/*=FALSE*/)
{
	//if (abWritePipe) { //2009-06-01 Секцию вроде всегда блокировать нужно...
	EnterCriticalSection(&csTabs);
	BOOL bReleased = FALSE;
	//}
#ifdef _DEBUG
	WCHAR szDbg[100]; wsprintf(szDbg, L"-SendTabs(%i,%s), prev=%i\n", tabCount, 
		abFillDataOnly ? L"FillDataOnly" : L"Post", gnCurTabCount);
	OUTPUTDEBUGSTRING(szDbg);
#endif
	if (tabs 
		&& (abFillDataOnly || (ConEmuHwnd && IsWindow(ConEmuHwnd)))
		)
	{
		COPYDATASTRUCT cds;
		//if (tabs[0].Type == WTYPE_PANELS) {
			cds.dwData = tabCount;
			cds.lpData = tabs;
		//} else {
		//	// Панелей нет - фар был открыт в режиме редактора!
		//	cds.dwData = --tabCount; //2009-06-04 наверное больше одного редактора в таком случае быть не должно
		//	cds.lpData = tabs+1;
		//}
		// Если abFillDataOnly - данные подготавливаются для записи в Pipe - иначе процедура отсылает данные сама

		cds.cbData = cds.dwData * sizeof(ConEmuTab);
		EnterCriticalSection(&csData);
		if (gpCmdRet) { Free(gpCmdRet); gpCmdRet = NULL; gpData = NULL; gpCursor = NULL; }
		OutDataAlloc(sizeof(cds.dwData) + cds.cbData);
		OutDataWrite(&cds.dwData, sizeof(cds.dwData));
		OutDataWrite(cds.lpData, cds.cbData);
		LeaveCriticalSection(&csData);

		LeaveCriticalSection(&csTabs); bReleased = TRUE;

		//TODO: возможно, что при переключении окон командой из конэму нужно сразу переслать табы?
		_ASSERT(gpCmdRet!=NULL);
		if (!abFillDataOnly && tabCount && gnReqCommand==(DWORD)-1) {
			// Это нужно делать только если инициировано ФАРОМ. Если запрос прислал ConEmu - не посылать...
			//if (gnCurTabCount != tabCount /*|| tabCount > 1*/)
			if (ConEmuHwnd && (abForceSend || gnCurTabCount != tabCount))
			{
				gnCurTabCount = tabCount; // сразу запомним!, А то при ретриве табов количество еще старым будет...
				//PostMessage(ConEmuHwnd, gnMsgTabChanged, tabCount, 0);
				gpCmdRet->hdr.nCmd = CECMD_TABSCHANGED;
				gpCmdRet->hdr.nSrcThreadId = GetCurrentThreadId();
				CESERVER_REQ* pOut =
					ExecuteGuiCmd(FarHwnd, gpCmdRet);
				if (pOut) ExecuteFreeResult(pOut);
			}
		}
	}
	//Free(tabs); - освобождается в ExitFARW
    gnCurTabCount = tabCount;
	if (!bReleased) {
		LeaveCriticalSection(&csTabs); bReleased = TRUE;
	}
}

// watch non-modified -> modified editor status change

int lastModifiedStateW = -1;
bool gbHandleOneRedraw = false;

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
	// Вроде коды событий не различаются, да и от ANSI не отличаются...
	switch (Event)
	{
	case EE_READ: // в этот момент количество окон еще не изменилось
		gbHandleOneRedraw = true;
		return 0;
	case EE_REDRAW:
		if (!gbHandleOneRedraw)
			return 0;
		gbHandleOneRedraw = false;
		OUTPUTDEBUGSTRING(L"EE_REDRAW(first)\n");
		break;
	case EE_CLOSE:
		OUTPUTDEBUGSTRING(L"EE_CLOSE\n"); break;
	case EE_GOTFOCUS:
		OUTPUTDEBUGSTRING(L"EE_GOTFOCUS\n"); break;
	case EE_KILLFOCUS:
		OUTPUTDEBUGSTRING(L"EE_KILLFOCUS\n"); break;
	case EE_SAVE:
		OUTPUTDEBUGSTRING(L"EE_SAVE\n"); break;
	default:
		return 0;
	}
	// !!! Именно UpdateConEmuTabsW, без версии !!!
	//2009-06-03 EE_KILLFOCUS при закрытии редактора не приходит. Только EE_CLOSE
	UpdateConEmuTabsW(Event+100, (Event == EE_KILLFOCUS || Event == EE_CLOSE), Event == EE_SAVE);
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
	UpdateConEmuTabsW(Event+200, (Event == VE_KILLFOCUS || Event == VE_CLOSE), false, Param);
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

	if (hThread) { // подождем чуть-чуть, или принудительно прибъем нить ожидания
		if (WaitForSingleObject(hThread,1000)) {
			#if !defined(__GNUC__)
			#pragma warning (disable : 6258)
			#endif
			TerminateThread(hThread, 100);
		}
		CloseHandle(hThread); hThread = NULL;
	}
	
    if (tabs) {
	    Free(tabs);
	    tabs = NULL;
    }
    
    if (ghReqCommandEvent) {
	    CloseHandle(ghReqCommandEvent); ghReqCommandEvent = NULL;
    }

	DeleteCriticalSection(&csTabs); memset(&csTabs,0,sizeof(csTabs));
	DeleteCriticalSection(&csData); memset(&csData,0,sizeof(csData));
	
	if (ghRegMonitorKey) { RegCloseKey(ghRegMonitorKey); ghRegMonitorKey = NULL; }
	SafeCloseHandle(ghRegMonitorEvt);
	SafeCloseHandle(ghServerTerminateEvent);
}

void   WINAPI _export ExitFARW(void)
{
	StopThread();

	if (gFarVersion.dwBuild>=FAR_Y_VER)
		FUNC_Y(ExitFARW)();
	else
		FUNC_X(ExitFARW)();
}

void CheckResources()
{
	wchar_t szLang[64];
	GetEnvironmentVariable(L"FARLANG", szLang, 63);
	if (lstrcmpW(szLang, gsFarLang))
		InitResources();
}

// Передать в ConEmu строки с ресурсами
void InitResources()
{
	if (!ConEmuHwnd || !FarHwnd) return;
	// В ConEmu нужно передать следущие ресурсы
	//
	int nSize = sizeof(CESERVER_REQ) + sizeof(DWORD) 
		+ 3*(MAX_PATH+1)*2; // + 3 строковых ресурса
	CESERVER_REQ *pIn = (CESERVER_REQ*)Alloc(nSize,1);;
	if (pIn) {
		pIn->hdr.nCmd = CECMD_RESOURCES;
		pIn->hdr.nSrcThreadId = GetCurrentThreadId();
		pIn->hdr.nVersion = CESERVER_REQ_VER;
		pIn->dwData[0] = GetCurrentProcessId();
		wchar_t* pszRes = (wchar_t*)&(pIn->dwData[1]);
		if (gFarVersion.dwVerMajor==1) {
			GetMsgA(10, pszRes); pszRes += lstrlenW(pszRes)+1;
			GetMsgA(11, pszRes); pszRes += lstrlenW(pszRes)+1;
			GetMsgA(12, pszRes); pszRes += lstrlenW(pszRes)+1;
		} else {
			lstrcpyW(pszRes, GetMsgW(10)); pszRes += lstrlenW(pszRes)+1;
			lstrcpyW(pszRes, GetMsgW(11)); pszRes += lstrlenW(pszRes)+1;
			lstrcpyW(pszRes, GetMsgW(12)); pszRes += lstrlenW(pszRes)+1;
		}
		pIn->hdr.nSize = ((LPBYTE)pszRes) - ((LPBYTE)pIn);
		CESERVER_REQ* pOut = ExecuteGuiCmd(FarHwnd, pIn);
		if (pOut) ExecuteFreeResult(pOut);
		Free(pIn);

		GetEnvironmentVariable(L"FARLANG", gsFarLang, 63);
	}
}

void CloseTabs()
{
	if (ConEmuHwnd && IsWindow(ConEmuHwnd) && FarHwnd) {
		CESERVER_REQ in = {{sizeof(CESERVER_REQ_HDR)}};
		in.hdr.nCmd = CECMD_TABSCHANGED;
		in.hdr.nSrcThreadId = GetCurrentThreadId();
		in.hdr.nVersion = CESERVER_REQ_VER;
		CESERVER_REQ* pOut = ExecuteGuiCmd(FarHwnd, &in);
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

	gpCmdRet->hdr.nSize = anSize+sizeof(CESERVER_REQ_HDR);
	gpCmdRet->hdr.nVersion = CESERVER_REQ_VER;
	gpCmdRet->hdr.nSrcThreadId = GetCurrentThreadId();

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
	lpNewCmdRet->hdr.nCmd = gpCmdRet->hdr.nCmd;
	lpNewCmdRet->hdr.nSize = anNewSize+sizeof(CESERVER_REQ_HDR);
	lpNewCmdRet->hdr.nVersion = gpCmdRet->hdr.nVersion;
	lpNewCmdRet->hdr.nSrcThreadId = GetCurrentThreadId();

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
                PIPEBUFSIZE, PIPEBUFSIZE, 0, NULL);

            _ASSERTE(hPipe != INVALID_HANDLE_VALUE);

            if (hPipe == INVALID_HANDLE_VALUE) 
            {
                //DisplayLastError(L"CreatePipe failed"); 
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

    #ifdef _DEBUG
	UINT nDataSize = pIn->hdr.nSize - sizeof(CESERVER_REQ) + 1;
	#endif

    // Все данные из пайпа получены, обрабатываем команду и возвращаем (если нужно) результат
	//fSuccess = WriteFile( hPipe, pOut, pOut->nSize, &cbWritten, NULL);

	if (pIn->hdr.nCmd == CMD_LANGCHANGE) {
		_ASSERTE(nDataSize>=4);
		HKL hkl = 0;
		memmove(&hkl, pIn->Data, 4);
		if (hkl) {
			DWORD dwLastError = 0;
			WCHAR szLoc[10]; wsprintf(szLoc, L"%08x", (DWORD)(((DWORD)hkl) & 0xFFFF));
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

	} else if (pIn->hdr.nCmd == CMD_REQTABS) {
		SendTabs(gnCurTabCount, TRUE, TRUE);

	} else if (pIn->hdr.nCmd == CMD_SETENVVAR) {
		// Установить переменные окружения
		_ASSERTE(nDataSize>=4);
		wchar_t *pszName  = (wchar_t*)pIn->Data;
		wchar_t *pszValue = pszName + lstrlenW(pszName) + 1;
		while (*pszName && *pszValue) {
			// Если в pszValue пустая строка - удаление переменной
			SetEnvironmentVariableW(pszName, (*pszValue != 0) ? pszValue : NULL);
			//
			pszName = pszValue + lstrlenW(pszValue) + 1;
			if (*pszName == 0) break;
			pszValue = pszName + lstrlenW(pszName) + 1;
		}

	} else {
		ProcessCommand(pIn->hdr.nCmd, TRUE/*bReqMainThread*/, pIn->Data);

		if (gpCmdRet) {
			fSuccess = WriteFile( hPipe, gpCmdRet, gpCmdRet->hdr.nSize, &cbWritten, NULL);
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

void ShowPluginMenu()
{
	int nItem = -1;
	if (!FarHwnd)
		return;

	if (gFarVersion.dwVerMajor==1)
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
			pIn->hdr.nSize = sizeof(CESERVER_REQ_HDR)+4;
			pIn->hdr.nCmd = CECMD_GETOUTPUTFILE;
			pIn->hdr.nSrcThreadId = GetCurrentThreadId();
			pIn->hdr.nVersion = CESERVER_REQ_VER;
			pIn->OutputFile.bUnicode = (gFarVersion.dwVerMajor>=2);
			pOut = ExecuteGuiCmd(FarHwnd, pIn);
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
			in.hdr.nSize = sizeof(CESERVER_REQ_HDR)+1;
			in.hdr.nCmd = CECMD_TABSCMD;
			in.hdr.nSrcThreadId = GetCurrentThreadId();
			in.hdr.nVersion = CESERVER_REQ_VER;
			in.Data[0] = nItem - 3;
			pOut = ExecuteGuiCmd(FarHwnd, &in);
			if (pOut) ExecuteFreeResult(pOut);
		} break;
		case 8: // Attach to GUI (если FAR был CtrlAltTab)
		{
			if (TerminalMode) break; // низзя
			if (ConEmuHwnd && IsWindow(ConEmuHwnd)) break; // Мы и так подключены?
			Attach2Gui();
		} break;
	}
}

BOOL Attach2Gui()
{
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
		if (szExe[nLen] != L'\\') wcscat(szExe, L"\\");
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
		lbRc = TRUE;
	}
	
	return lbRc;
}
