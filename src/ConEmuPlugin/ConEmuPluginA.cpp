/* ****************************************** 
   Changes history 
   Maximus5: попытка сделать одну dll'ку 
             под обе версии FAR
****************************************** */

//#include <stdio.h>
#include <windows.h>
//#include <windowsx.h>
//#include <string.h>
//#include <tchar.h>
#include "..\common\common.hpp"
#include "..\common\pluginA.hpp"
#include "PluginHeader.h"

#ifndef FORWARD_WM_COPYDATA
#define FORWARD_WM_COPYDATA(hwnd, hwndFrom, pcds, fn) \
    (BOOL)(UINT)(DWORD)(fn)((hwnd), WM_COPYDATA, (WPARAM)(hwndFrom), (LPARAM)(pcds))
#endif

extern BOOL LoadFarVersion();
extern FarVersion gFarVersion;

// minimal(?) FAR version 1.71 alpha 4 build 2470
int WINAPI _export GetMinFarVersion(void)
{
    // Однако, FAR2 до сборки 748 не понимал две версии плагина в одном файле
    BOOL bFar2=FALSE;
	if (!LoadFarVersion())
		bFar2 = TRUE;
	else
		bFar2 = gFarVersion.dwVerMajor>=2;

    if (bFar2) {
	    return MAKEFARVERSION(2,0,748);
    }
	return MAKEFARVERSION(1,71,2470);
}


/*static*/ struct PluginStartupInfo *InfoA=NULL;
struct FarStandardFunctions *FSFA=NULL;
extern HWND ConEmuHwnd;
extern BOOL bWasSetParent;
extern HWND FarHwnd;
extern HANDLE hPipe, hPipeEvent;
extern HANDLE hThread;
/*HWND ConEmuHwnd=NULL;
HWND FarHwnd=NULL;
HANDLE hPipe=NULL;
HANDLE hThread=NULL;*/


/*#define MessagesMax 5
WCHAR MessagesA[MessagesMax][64];*/

extern 
VOID CALLBACK ConEmuCheckTimerProc(
  HWND hwnd,         // handle to window
  UINT uMsg,         // WM_TIMER message
  UINT_PTR idEvent,  // timer identifier
  DWORD dwTime       // current system time
);

void UpdateConEmuTabsA(int event, bool losingFocus, bool editorSave);

/* const WCHAR *GetMsgA(int CompareLng)
{
	if (CompareLng<0 || CompareLng>=MessagesMax)
		return L"";

	const char* pszMsg = InfoA->GetMsg(InfoA->ModuleNumber, CompareLng);
	if (!pszMsg)
		MessagesA[CompareLng][0]=0;
	else {
		int nCh = MultiByteToWideChar(CP_OEMCP, 0, pszMsg, -1, MessagesA[CompareLng], 63);
		MessagesA[CompareLng][nCh] = 0;
	}

	return MessagesA[CompareLng];
}*/


DWORD WINAPI ThreadProcA(LPVOID lpParameter)
{
	while (true)
	{
		if (hPipeEvent && ConEmuHwnd) {
			DWORD dwWait = WaitForSingleObject(hPipeEvent, 1000);
			MCHKHEAP

		    if (ConEmuHwnd && FarHwnd) {
			    if (!IsWindow(ConEmuHwnd)) {
				    ConEmuHwnd = NULL;

				    //HWND hParent = GetAncestor(FarHwnd, GA_PARENT);
					//if (hParent == ConEmuHwnd) {
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
			/*case SetConEmuHwnd:
			{
				HWND hWnd = NULL;
				ReadFile(hPipe, &hWnd, sizeof(hWnd), &cout, NULL);
				if (hWnd && IsWindow(hWnd))
					ConEmuHwnd = hWnd;
				break;
			}*/
			case DragFrom:
			{
				MCHKHEAP
				WindowInfo WInfo;				
			    WInfo.Pos=0;
				InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETWINDOWINFO, (void*)&WInfo);
				if (!WInfo.Current)
				{
					int ItemsCount=0;
					WriteFile(hPipe, &ItemsCount, sizeof(int), &cout, NULL);				
					break;
				}
				MCHKHEAP

				PanelInfo PInfo;
				InfoA->Control(INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, &PInfo);
				MCHKHEAP
				if ((PInfo.PanelType == PTYPE_FILEPANEL || PInfo.PanelType == PTYPE_TREEPANEL) && PInfo.Visible)
				{
					int nDirLen=0, nDirNoSlash=0;
					if (PInfo.CurDir && *PInfo.CurDir)
					{
						nDirLen=lstrlenA(PInfo.CurDir);
						if (nDirLen>0)
							if (PInfo.CurDir[nDirLen-1]!='\\')
								nDirNoSlash=1;
					}
					MCHKHEAP

					//Maximus5 - новый формат передачи
					int nNull=0;
					WriteFile(hPipe, &nNull/*ItemsCount*/, sizeof(int), &cout, NULL);
					MCHKHEAP
					
					if (PInfo.SelectedItemsNumber>0)
					{
						int ItemsCount=PInfo.SelectedItemsNumber, i;

						int nMaxLen=MAX_PATH+1, nWholeLen=1;

						// сначала посчитать максимальную длину буфера
						for (i=0;i<ItemsCount;i++)
						{
							int nLen=nDirLen+nDirNoSlash;
							nLen += lstrlenA(PInfo.SelectedItems[i].FindData.cFileName);
							if (nLen>nMaxLen)
								nMaxLen = nLen;
							nWholeLen += (nLen+1);
						}
						WriteFile(hPipe, &nWholeLen, sizeof(int), &cout, NULL);
						MCHKHEAP

						WCHAR* Path=new WCHAR[nMaxLen+1];
						MCHKHEAP

						for (i=0;i<ItemsCount;i++)
						{
							//WCHAR Path[MAX_PATH+1];
							//ZeroMemory(Path, MAX_PATH+1);
							//Maximus5 - засада с корнем диска и возможностью overflow
							//wsprintf(Path, L"%s\\%s", PInfo.CurDir, PInfo.SelectedItems[i]->FindData.cFileName);
							Path[0]=0;

							int nLen=nDirLen+nDirNoSlash;
							nLen += lstrlenA(PInfo.SelectedItems[i].FindData.cFileName);
							MCHKHEAP

							if (nDirLen>0) {
								MultiByteToWideChar(CP_OEMCP/*???*/,0,
									PInfo.CurDir, nDirLen+1, Path, nDirLen+1);
								//lstrcpy(Path, PInfo.CurDir);
								if (nDirNoSlash) {
									Path[nDirLen]=L'\\';
									Path[nDirLen+1]=0;
								}
							}
							MCHKHEAP
							int nFLen = lstrlenA(PInfo.SelectedItems[i].FindData.cFileName)+1;
							MultiByteToWideChar(CP_OEMCP/*???*/, 0,
								PInfo.SelectedItems[i].FindData.cFileName,nFLen,Path+nDirLen+nDirNoSlash,nFLen);
							//lstrcpy(Path+nDirLen+nDirNoSlash, PInfo.SelectedItems[i].FindData.cFileName);
							MCHKHEAP

							nLen++;
							WriteFile(hPipe, &nLen, sizeof(int), &cout, NULL);
							WriteFile(hPipe, Path, sizeof(WCHAR)*nLen, &cout, NULL);
						}
						MCHKHEAP

						delete Path; Path=NULL;
						MCHKHEAP

						// Конец списка
						WriteFile(hPipe, &nNull/*ItemsCount*/, sizeof(int), &cout, NULL);
					}
				}
				else
				{
					int ItemsCount=0;
					WriteFile(hPipe, &ItemsCount, sizeof(int), &cout, NULL);
					WriteFile(hPipe, &ItemsCount, sizeof(int), &cout, NULL); // смена формата
					MCHKHEAP
				}
				break;
			}
			case DragTo:
			{
				WindowInfo WInfo;				
			    WInfo.Pos=0;
			    MCHKHEAP
				InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETWINDOWINFO, (void*)&WInfo);
				MCHKHEAP
				if (!WInfo.Current)
				{
					int ItemsCount=0;
					WriteFile(hPipe, &ItemsCount, sizeof(int), &cout, NULL);				
					break;
				}
				PanelInfo PAInfo, PPInfo;
				ForwardedPanelInfo *pfpi=NULL;
				MCHKHEAP
				int nStructSize = sizeof(ForwardedPanelInfo)+4; // потом увеличим на длину строк
				//ZeroMemory(&fpi, sizeof(fpi));
				BOOL lbAOK=FALSE, lbPOK=FALSE;
				MCHKHEAP
				
                //Maximus5 - к сожалению, В FAR2 FCTL_GETPANELSHORTINFO не возвращает CurDir :-(

                if (!(lbAOK=InfoA->Control(INVALID_HANDLE_VALUE, FCTL_GETPANELSHORTINFO, &PAInfo)))
                    lbAOK=InfoA->Control(INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, &PAInfo);
				if (lbAOK && PAInfo.CurDir)
					nStructSize += (lstrlenA(PAInfo.CurDir))*sizeof(WCHAR);
				MCHKHEAP

                if (!(lbPOK=InfoA->Control(INVALID_HANDLE_VALUE, FCTL_GETANOTHERPANELSHORTINFO, &PPInfo)))
                    lbPOK=InfoA->Control(INVALID_HANDLE_VALUE, FCTL_GETANOTHERPANELINFO, &PPInfo);
                MCHKHEAP
				if (lbPOK && PPInfo.CurDir)
					nStructSize += (lstrlenA(PPInfo.CurDir))*sizeof(WCHAR); // Именно WCHAR! не TCHAR
				MCHKHEAP

				pfpi = (ForwardedPanelInfo*)calloc(nStructSize,1);
				MCHKHEAP


				pfpi->ActivePathShift = sizeof(ForwardedPanelInfo);
				pfpi->pszActivePath = (WCHAR*)(((char*)pfpi)+pfpi->ActivePathShift);

				pfpi->PassivePathShift = pfpi->ActivePathShift+2; // если ActivePath заполнится - увеличим

				MCHKHEAP
				if (lbAOK)
				{
					pfpi->ActiveRect=PAInfo.PanelRect;
					if ((PAInfo.PanelType == PTYPE_FILEPANEL || PAInfo.PanelType == PTYPE_TREEPANEL) && PAInfo.Visible)
					{
						if (PAInfo.CurDir != NULL) {
							MCHKHEAP
							int nLen = lstrlenA(PAInfo.CurDir)+1;
							MultiByteToWideChar(CP_OEMCP/*??? проверить*/,0,
								PAInfo.CurDir, nLen, pfpi->pszActivePath, nLen);
							//lstrcpyW(pfpi->pszActivePath, PAInfo.CurDir);
							pfpi->PassivePathShift += (nLen-1)*2;
							MCHKHEAP
						}
					}
				}

				pfpi->pszPassivePath = (WCHAR*)(((char*)pfpi)+pfpi->PassivePathShift);
				if (lbPOK)
				{
					pfpi->PassiveRect=PPInfo.PanelRect;
					if ((PPInfo.PanelType == PTYPE_FILEPANEL || PPInfo.PanelType == PTYPE_TREEPANEL) && PPInfo.Visible)
					{
						if (PPInfo.CurDir != NULL)
						{
							int nLen = lstrlenA(PPInfo.CurDir)+1;
							MultiByteToWideChar(CP_OEMCP/*??? проверить*/,0,
								PPInfo.CurDir, nLen, pfpi->pszPassivePath, nLen);
							//lstrcpyW(pfpi->pszPassivePath, PPInfo.CurDir);
							MCHKHEAP
						}		
					}
				}

				// Собственно, пересылка информации
				WriteFile(hPipe, &nStructSize, sizeof(nStructSize), &cout, NULL);
				WriteFile(hPipe, pfpi, nStructSize, &cout, NULL);
				MCHKHEAP

				free(pfpi); pfpi=NULL;
				break;
			}
		}
		MCHKHEAP
		Sleep(1);
	}
}

extern HWND AtoH(WCHAR *Str, int Len);
/*{
  __int64 Ret=0;
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
}*/

void WINAPI _export SetStartupInfo(struct PluginStartupInfo *aInfo)
{
    MCHKHEAP
	::InfoA = (PluginStartupInfo*)calloc(sizeof(PluginStartupInfo),1);
	::FSFA = (FarStandardFunctions*)calloc(sizeof(FarStandardFunctions),1);
	*::InfoA = *aInfo;
	*::FSFA = *aInfo->FSF;
	::InfoA->FSF = ::FSFA;
	MCHKHEAP
	
//#ifdef _DEBUG
//	InfoA->Message(InfoA->ModuleNumber, FMSG_MB_OK|FMSG_ALLINONE, NULL, 
//			(const char *const *)"ConEmu plugin\nPlugin loaded as ANSI", 0, 0);
//#endif

	/*for (int i=0; i<=MessagesMax; i++)
		MessagesA[i][0]=0;*/

	ConEmuHwnd = NULL;
	FarHwnd = (HWND)InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETFARHWND, 0);
	ConEmuHwnd = GetAncestor(FarHwnd, GA_PARENT);
	if (ConEmuHwnd != NULL)
	{
		char className[100];
		GetClassNameA(ConEmuHwnd, (LPSTR)className, 100);
		if (FSFA->LStricmp(className, "VirtualConsoleClass") != 0 &&
			FSFA->LStricmp(className, "VirtualConsoleClassDC") != 0)
		{
			ConEmuHwnd = NULL;
		} else {
			bWasSetParent = TRUE;
		}
	}
	MCHKHEAP
	
	WCHAR *pipename=(WCHAR*)calloc(MAX_PATH,sizeof(WCHAR));

	if (!ConEmuHwnd) {
		//MessageBoxA(0,"Debug","Debug",0);
		if (GetEnvironmentVariable(L"ConEmuHWND", pipename, MAX_PATH)) {
			if (pipename[0]==L'0' && pipename[1]==L'x') {
				ConEmuHwnd = AtoH(pipename+2, 8);
				if (ConEmuHwnd) {
                    char className[100];
                    GetClassNameA(ConEmuHwnd, (LPSTR)className, 100);
                    if (FSFA->LStricmp(className, "VirtualConsoleClass") != 0 &&
						FSFA->LStricmp(className, "VirtualConsoleClassDC") != 0)
					{
						ConEmuHwnd = NULL;
					}
				}
			}
		}
	}
	MCHKHEAP
	
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
	
		hThread=CreateThread(NULL, 0, &ThreadProcA, 0, 0, 0);
	} else {
	#ifdef _DEBUG
		//char Msg[255]; lstrcpyA(Msg, InfoA->GetMsg(InfoA->ModuleNumber,4));
		//InfoA->Message(InfoA->ModuleNumber, FMSG_WARNING|FMSG_ERRORTYPE|FMSG_MB_OK|FMSG_ALLINONE, NULL, 
		//	(const char *const *)Msg, 0, 0);
	#endif
	}
	free(pipename);
	MCHKHEAP
}

void WINAPI _export GetPluginInfo(struct PluginInfo *pi)
{
	pi->StructSize = sizeof(struct PluginInfo);
	pi->Flags = PF_EDITOR | PF_VIEWER | PF_PRELOAD;
	pi->DiskMenuStrings = NULL;
	pi->DiskMenuNumbers = 0;
	pi->PluginMenuStrings = NULL;
	pi->PluginMenuStringsNumber =0;
	pi->PluginConfigStrings = NULL;
	pi->PluginConfigStringsNumber = 0;
	pi->CommandPrefix = NULL;
	pi->Reserved = 0;	
	MCHKHEAP
}

/*static*/ int lastModifiedStateA = -1;
// watch non-modified -> modified editor status change
int WINAPI _export ProcessEditorInput(const INPUT_RECORD *Rec)
{
	if (!ConEmuHwnd)
		return 0; // Если мы не под эмулятором - ничего
	// only key events with virtual codes > 0 are likely to cause status change (?)
	if (Rec->EventType == KEY_EVENT && Rec->Event.KeyEvent.wVirtualKeyCode > 0  && Rec->Event.KeyEvent.bKeyDown)
	{
		EditorInfo ei;
		InfoA->EditorControl(ECTL_GETINFO, &ei);
		int currentModifiedState = ei.CurState == ECSTATE_MODIFIED ? 1 : 0;
		if (lastModifiedStateA != currentModifiedState)
		{
			MCHKHEAP
			UpdateConEmuTabsA(0, false, false);
			lastModifiedStateA = currentModifiedState;
			MCHKHEAP
		}
	}
	return 0;
}

int WINAPI _export ProcessEditorEvent(int Event, void *Param)
{
	if (!ConEmuHwnd)
		return 0; // Если мы не под эмулятором - ничего
	switch (Event)
	{
	case EE_CLOSE:
	case EE_GOTFOCUS:
	case EE_KILLFOCUS:
	case EE_SAVE:
	case EE_READ:
		{
			MCHKHEAP
			UpdateConEmuTabsA(Event, Event == EE_KILLFOCUS, Event == EE_SAVE);
			MCHKHEAP
			break;
		}
	}
	return 0;
}

int WINAPI _export ProcessViewerEvent(int Event, void *Param)
{
	if (!ConEmuHwnd)
		return 0; // Если мы не под эмулятором - ничего
	switch (Event)
	{
	case VE_CLOSE:
	case VE_READ:
	case VE_KILLFOCUS:
	case VE_GOTFOCUS:
		{
			MCHKHEAP
			UpdateConEmuTabsA(Event, Event == VE_KILLFOCUS, false);
			MCHKHEAP
		}
	}

	return 0;
}

void UpdateConEmuTabsA(int event, bool losingFocus, bool editorSave)
{
	WindowInfo WInfo;
	WCHAR* pszName = (WCHAR*)calloc(CONEMUTABMAX, sizeof(WCHAR));

	int windowCount = (int)InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETWINDOWCOUNT, NULL);
	int maxTabCount = windowCount + 1;

	MCHKHEAP
	ConEmuTab* tabs = (ConEmuTab*) calloc(maxTabCount, sizeof(ConEmuTab));

	EditorInfo ei;
	WCHAR* pszFileName = NULL;
	if (editorSave)
	{
		InfoA->EditorControl(ECTL_GETINFO, &ei);
		pszFileName = (WCHAR*)calloc(CONEMUTABMAX, sizeof(WCHAR));
		if (ei.FileName)
			MultiByteToWideChar(CP_OEMCP, 0, ei.FileName, lstrlenA(ei.FileName)+1, pszFileName, CONEMUTABMAX);
	}
	MCHKHEAP

	int tabCount = 1;
	for (int i = 0; i < windowCount; i++)
	{
		WInfo.Pos = i;
		MCHKHEAP
		InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETWINDOWINFO, (void*)&WInfo);
		if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER || WInfo.Type == WTYPE_PANELS)
		{
			MultiByteToWideChar(CP_OEMCP, 0, WInfo.Name, lstrlenA(WInfo.Name)+1, pszName, CONEMUTABMAX);
			AddTab(tabs, tabCount, losingFocus, editorSave, 
				WInfo.Type, pszName, editorSave ? pszFileName : NULL, 
				WInfo.Current, WInfo.Modified);
			MCHKHEAP
		}
	}

	if (pszFileName) free(pszFileName);
	if (pszName) free(pszName);
	MCHKHEAP

	SendTabs(tabs, tabCount);
}

void   WINAPI _export ExitFAR(void)
{
    MCHKHEAP
	if (InfoA) {
		free(InfoA);
		InfoA=NULL;
	}
	if (FSFA) {
		free(FSFA);
		FSFA=NULL;
	}
	MCHKHEAP
}
