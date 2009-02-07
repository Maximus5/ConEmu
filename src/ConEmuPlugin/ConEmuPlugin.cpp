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
#include "..\common\pluginW757.hpp"
#include "PluginHeader.h"

#ifndef FORWARD_WM_COPYDATA
#define FORWARD_WM_COPYDATA(hwnd, hwndFrom, pcds, fn) \
    (BOOL)(UINT)(DWORD)(fn)((hwnd), WM_COPYDATA, (WPARAM)(hwndFrom), (LPARAM)(pcds))
#endif
#define MAKEFARVERSION(major,minor,build) ( ((major)<<8) | (minor) | ((build)<<16))

// minimal(?) FAR version 2.0 alpha build 748
int WINAPI _export GetMinFarVersionW(void)
{
	return MAKEFARVERSION(2,0,748);
}


HWND ConEmuHwnd=NULL;
BOOL bWasSetParent=FALSE;
HWND FarHwnd=NULL;
HANDLE hPipe=NULL, hPipeEvent=NULL;
HANDLE hThread=NULL;
FarVersion gFarVersion;
WCHAR gszDir1[CONEMUTABMAX], gszDir2[CONEMUTABMAX];
//UINT_PTR uTimerID=0;
int maxTabCount = 0, lastWindowCount = 0;
ConEmuTab* tabs = NULL; //(ConEmuTab*) calloc(maxTabCount, sizeof(ConEmuTab));

BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
			{
				HWND hConWnd = GetConsoleWindow();
				InitHWND(hConWnd);
			}
			break;
	}
	return TRUE;
}

#if defined(__GNUC__)
#ifdef __cplusplus
extern "C"{
#endif
  BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved);
  HWND WINAPI GetFarHWND();
  void WINAPI GetFarVersion ( FarVersion* pfv );
  int  WINAPI ProcessEditorInputW(void* Rec);
  void WINAPI  SetStartupInfoW(void *aInfo);
#ifdef __cplusplus
};
#endif

BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
{
  /*(void) lpReserved;
  (void) dwReason;
  (void) hDll;*/
  return DllMain(hDll, dwReason,lpReserved);
}
#endif

HWND WINAPI _export GetFarHWND()
{
	if (ConEmuHwnd) {
		if (IsWindow(ConEmuHwnd))
			return ConEmuHwnd;
		ConEmuHwnd = NULL;
	}
	return FarHwnd;
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
			void *pVerData = calloc(dwSize, 1);
			VS_FIXEDFILEINFO *lvs = NULL;
			UINT nLen = sizeof(lvs);
			if (GetFileVersionInfo(FarPath, 0, dwSize, pVerData)) {
				if (VerQueryValue ((void*)pVerData, 
						L"\\", (void**)&lvs, &nLen)) {
					gFarVersion.dwVer = lvs->dwFileVersionMS;
					gFarVersion.dwBuild = lvs->dwFileVersionLS;
					lbRc = TRUE;
				}
			}
			free(pVerData);
		}
	}

	if (!lbRc) {
		gFarVersion.dwVerMajor = 2;
		gFarVersion.dwVerMinor = 0;
		gFarVersion.dwBuild = 757;
	}

	return lbRc;
}

VOID CALLBACK ConEmuCheckTimerProc(
  HWND hwnd,         // handle to window
  UINT uMsg,         // WM_TIMER message
  UINT_PTR idEvent,  // timer identifier
  DWORD dwTime       // current system time
);

void UpdateConEmuTabsW(int event, bool losingFocus, bool editorSave);

/*extern const WCHAR *GetMsgW684(int CompareLng);
extern const WCHAR *GetMsgW757(int CompareLng);
const WCHAR *GetMsgW(int CompareLng)
{
	if (gFarVersion.dwBuild>=757)
		return GetMsgW757(CompareLng);
	else
		return GetMsgW684(CompareLng);
}*/


DWORD WINAPI ThreadProcW(LPVOID lpParameter)
{
	while (true)
	{
		if (hPipeEvent && ConEmuHwnd) {
			DWORD dwWait = WaitForSingleObject(hPipeEvent, 1000);

		    if (ConEmuHwnd && FarHwnd) {
			    if (!IsWindow(ConEmuHwnd)) {
				    ConEmuHwnd = NULL;

					if (!IsWindow(FarHwnd))
					{
						MessageBox(0, L"ConEmu was abnormally termintated!\r\nExiting from FAR", L"ConEmu plugin", MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
						TerminateProcess(GetCurrentProcess(), 100);
					}
					else 
					{
						if (bWasSetParent) {
							SetParent(FarHwnd, NULL);
						}
					    
						ShowWindowAsync(FarHwnd, SW_SHOWNORMAL);
						EnableWindow(FarHwnd, true);
					}
				    
				    CloseHandle(hPipe); hPipe=NULL;
				    CloseHandle(hPipeEvent); hPipeEvent=NULL;
				    
				    return 0; // Эта нить более не требуется
			    }
		    }

			if (dwWait!=WAIT_OBJECT_0) {
				continue;
			}
		}

		if (hPipeEvent) ResetEvent(hPipeEvent);
	
		PipeCmd cmd;
		DWORD cout;
		ReadFile(hPipe, &cmd, sizeof(PipeCmd), &cout, NULL);
		switch (cmd)
		{
		    case SetTabs:
			    break;
			case DragFrom:
			{
				if (gFarVersion.dwBuild>=757)
					ProcessDragFrom757();
				else
					ProcessDragFrom684();
				break;
			}
			case DragTo:
			{
				if (gFarVersion.dwBuild>=757)
					ProcessDragTo757();
				else
					ProcessDragTo684();
				break;
			}
		}
		Sleep(1);
	}
}

HWND AtoH(WCHAR *Str, int Len)
{
  DWORD Ret=0;
  for (; Len && *Str; Len--, Str++)
  {
    if (*Str>=L'0' && *Str<=L'9')
      (Ret*=16)+=*Str-L'0';
    else if (*Str>=L'a' && *Str<=L'f')
      (Ret*=16)+=(*Str-L'a'+10);
    else if (*Str>=L'A' && *Str<=L'F')
      (Ret*=16)+=(*Str-L'A'+10);
  }
  return (HWND)Ret;
}


void WINAPI _export SetStartupInfoW(void *aInfo)
{
	LoadFarVersion();

	if (gFarVersion.dwBuild>=757)
		SetStartupInfoW757(aInfo);
	else
		SetStartupInfoW684(aInfo);
}

void InitHWND(HWND ahFarHwnd)
{
	ConEmuHwnd = NULL;
	FarHwnd = ahFarHwnd;
	ConEmuHwnd = GetAncestor(FarHwnd, GA_PARENT);
	if (ConEmuHwnd != NULL)
	{
		WCHAR className[100];
		GetClassNameW(ConEmuHwnd, className, 100);
		if (lstrcmpi(className, L"VirtualConsoleClass") != 0 &&
			lstrcmpi(className, L"VirtualConsoleClassMain") != 0)
		{
			ConEmuHwnd = NULL;
		} else {
			bWasSetParent = TRUE;
		}
	}

	WCHAR *pipename = gszDir1;

	if (!ConEmuHwnd) {
		if (GetEnvironmentVariable(L"ConEmuHWND", pipename, MAX_PATH)) {
			if (pipename[0]==L'0' && pipename[1]==L'x') {
				ConEmuHwnd = AtoH(pipename+2, 8);
				if (ConEmuHwnd) {
					WCHAR className[100];
					GetClassName(ConEmuHwnd, (LPWSTR)className, 100);
					if (lstrcmpiW(className, L"VirtualConsoleClass") != 0 &&
						lstrcmpiW(className, L"VirtualConsoleClassMain") != 0)
					{
						ConEmuHwnd = NULL;
					} else {
						// Нужно проверить, что консоль принадлежит процессу ConEmu
						DWORD dwEmuPID = 0;
						GetWindowThreadProcessId(ConEmuHwnd, &dwEmuPID);
						if (dwEmuPID == 0) {
							// Ошибка, не удалось получить код процесса ConEmu
						} else {
							DWORD *ProcList = (DWORD*)calloc(1000,sizeof(DWORD));
							DWORD dwCount = 0;
#if !defined(_MSC_VER)
							typedef DWORD (APIENTRY *FGetConsoleProcessList)(LPDWORD,DWORD);
							FGetConsoleProcessList GetConsoleProcessList = 
								(FGetConsoleProcessList)GetProcAddress(
									GetModuleHandle(L"kernel32.dll"),
									"GetConsoleProcessList");
	                        if (!GetConsoleProcessList)
		                        dwCount = 1001;
		                    else
#endif
                                dwCount = GetConsoleProcessList(ProcList,1000);
                                
							if(dwCount>1000) {
								// Ошибка, в консоли слишком много процессов
							} else {
								for (DWORD dw=0; dw<dwCount; dw++) {
									if (dwEmuPID == ProcList[dw]) {
										dwEmuPID = 0; break;
									}
								}
								if (dwEmuPID)
									ConEmuHwnd = NULL; // Консоль не принадлежит ConEmu
							}
							free(ProcList);
						}
					}
				}
			}
		}
	}
	
	// Если мы не под эмулятором - больше ничего делать не нужно
	if (ConEmuHwnd) {
		//Maximus5 - теперь имя пайпа - по хэндлу окна FAR'а
		//wsprintf(pipename, L"\\\\.\\pipe\\ConEmu%h", ConEmuHwnd);
		//DWORD dwFarProcID = GetCurrentProcessId();
		wsprintf(pipename, L"\\\\.\\pipe\\ConEmuP%u", FarHwnd);
		hPipe = CreateFile( 
			 pipename,   // pipe name 
			 GENERIC_READ |  // read and write access 
			 GENERIC_WRITE, 
			 0,              // no sharing 
			 NULL,           // default security attributes
			 OPEN_EXISTING,  // opens existing pipe 
			 0,              // default attributes 
			 NULL);          // no template file 
		if (hPipe && hPipe!=INVALID_HANDLE_VALUE)
		{
			wsprintf(pipename, L"ConEmuPEvent%u", /*hWnd*/ FarHwnd );
			hPipeEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, pipename);
			if (hPipeEvent==INVALID_HANDLE_VALUE) hPipeEvent=NULL;
		
			hThread=CreateThread(NULL, 0, &ThreadProcW, 0, 0, 0);
		}

		// дернуть табы, если они нужны
		int tabCount = 0;
		CreateTabs(1);
		AddTab(tabCount, true, false, WTYPE_PANELS, NULL, NULL, 0, 0);
		SendTabs(tabCount=1, TRUE);
	}

	//free(pipename);

#ifdef _DEBUG
	//MessageBox(ConEmuHwnd,L"Debug",L"ConEmu plugin",MB_SETFOREGROUND);
#endif

}

void UpdateConEmuTabsW(int event, bool losingFocus, bool editorSave)
{
	if (gFarVersion.dwBuild>=757)
		UpdateConEmuTabsW757(event, losingFocus, editorSave);
	else
		UpdateConEmuTabsW684(event, losingFocus, editorSave);
}

BOOL CreateTabs(int windowCount)
{
	if ((tabs==NULL) || (maxTabCount <= (windowCount + 1)))
	{
		maxTabCount = windowCount + 10; // с запасом
		if (tabs) {
			free(tabs); tabs = NULL;
		}
		
		tabs = (ConEmuTab*) calloc(maxTabCount, sizeof(ConEmuTab));
	}
	
	lastWindowCount = windowCount;
	
	return tabs!=NULL;
}

BOOL AddTab(int &tabCount, bool losingFocus, bool editorSave, 
			int Type, LPCWSTR Name, LPCWSTR FileName, int Current, int Modified)
{
    BOOL lbCh = FALSE;
    
	if (Type == WTYPE_PANELS) {
	    lbCh = (tabs[0].Current != (losingFocus ? 1 : 0)) ||
	           (tabs[0].Type != WTYPE_PANELS);
		tabs[0].Current = losingFocus ? 1 : 0;
		//lstrcpyn(tabs[0].Name, GetMsgW757(0), CONEMUTABMAX-1);
		tabs[0].Name[0] = 0;
		tabs[0].Pos = 0;
		tabs[0].Type = WTYPE_PANELS;
	} else
	if (Type == WTYPE_EDITOR || Type == WTYPE_VIEWER)
	{
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

		int nLen=min(lstrlen(Name),(CONEMUTABMAX-1));
		lstrcpyn(tabs[tabCount].Name, Name, nLen+1);
		tabs[tabCount].Name[nLen]=0;

		tabs[tabCount].Pos = tabCount;
		tabCount++;
	}
	
	return lbCh;
}

void SendTabs(int &tabCount, BOOL abForce/*=FALSE*/)
{
	if (ConEmuHwnd && IsWindow(ConEmuHwnd)) {
		COPYDATASTRUCT cds;
		if (tabs[0].Type == WTYPE_PANELS) {
			cds.dwData = tabCount;
			cds.lpData = tabs;
		} else {
			// Панелей нет - фар был открыт в режиме редактора!
			cds.dwData = --tabCount;
			cds.lpData = tabs+1;
		}
		if (tabCount || abForce) {
			cds.cbData = tabCount * sizeof(ConEmuTab);
			FORWARD_WM_COPYDATA(ConEmuHwnd, FarHwnd, &cds, SendMessage);
		}
	}
	//free(tabs); - освобождается в ExitFARW
}

// watch non-modified -> modified editor status change

int lastModifiedStateW = -1;

int WINAPI _export ProcessEditorInputW(void* Rec)
{
	if (!ConEmuHwnd)
		return 0; // Если мы не под эмулятором - ничего
	if (gFarVersion.dwBuild>=757)
		return ProcessEditorInputW757((LPCVOID)Rec);
	else
		return ProcessEditorInputW684((LPCVOID)Rec);
}

int WINAPI _export ProcessEditorEventW(int Event, void *Param)
{
	if (!ConEmuHwnd)
		return 0; // Если мы не под эмулятором - ничего
	if (gFarVersion.dwBuild>=757)
		return ProcessEditorEventW757(Event,Param);
	else
		return ProcessEditorEventW684(Event,Param);
}

int WINAPI _export ProcessViewerEventW(int Event, void *Param)
{
	if (!ConEmuHwnd)
		return 0; // Если мы не под эмулятором - ничего
	if (gFarVersion.dwBuild>=757)
		return ProcessViewerEventW757(Event,Param);
	else
		return ProcessViewerEventW684(Event,Param);
}

void   WINAPI _export ExitFARW(void)
{
	CloseTabs();

    if (tabs) {
	    free(tabs);
	    tabs = NULL;
    }
    
	if (gFarVersion.dwBuild>=757)
		return ExitFARW757();
	else
		return ExitFARW684();
}

void CloseTabs()
{
	if (ConEmuHwnd && IsWindow(ConEmuHwnd)) {
		COPYDATASTRUCT cds;
		cds.dwData = 0;
		cds.lpData = &cds.dwData;
		cds.cbData = sizeof(cds.dwData);
		SendMessage(ConEmuHwnd, WM_COPYDATA, (WPARAM)FarHwnd, (LPARAM)&cds);
	}
}
