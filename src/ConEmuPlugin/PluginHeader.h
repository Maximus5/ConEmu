#pragma once

#if !defined(_MSC_VER)
#include <wchar.h>
#include <tchar.h>
#endif

#define SafeCloseHandle(h) { if ((h)!=NULL) { HANDLE hh = (h); (h) = NULL; if (hh!=INVALID_HANDLE_VALUE) CloseHandle(hh); } }

extern int lastModifiedStateW;
extern WCHAR gszDir1[CONEMUTABMAX], gszDir2[CONEMUTABMAX];
extern int maxTabCount, lastWindowCount;
extern ConEmuTab* tabs; //(ConEmuTab*) calloc(maxTabCount, sizeof(ConEmuTab));
extern HWND ConEmuHwnd;
extern HWND FarHwnd;
extern FarVersion gFarVersion;
extern int lastModifiedStateW;
extern HANDLE hEventCmd[MAXCMDCOUNT];
extern HANDLE hThread;

BOOL CreateTabs(int windowCount);

BOOL AddTab(int &tabCount, bool losingFocus, bool editorSave, 
			int Type, LPCWSTR Name, LPCWSTR FileName, int Current, int Modified);

void SendTabs(int tabCount, BOOL abForce=FALSE);

void InitHWND(HWND ahFarHwnd);

int ProcessEditorInputW684(LPCVOID Rec);
int ProcessEditorInputW757(LPCVOID Rec);
int ProcessEditorEventW684(int Event, void *Param);
int ProcessEditorEventW757(int Event, void *Param);
int ProcessViewerEventW684(int Event, void *Param);
int ProcessViewerEventW757(int Event, void *Param);
void StopThread(void);
void ExitFARW684(void);
void ExitFARW757(void);
void UpdateConEmuTabsW684(int event, bool losingFocus, bool editorSave);
void UpdateConEmuTabsW757(int event, bool losingFocus, bool editorSave);
void SetStartupInfoW684(void *aInfo);
void SetStartupInfoW757(void *aInfo);
void ProcessDragFrom684();
void ProcessDragFrom757();
void ProcessDragFromA();
void ProcessDragTo684();
void ProcessDragTo757();
void ProcessDragToA();

void CloseTabs();

HWND AtoH(WCHAR *Str, int Len);
void UpdateConEmuTabsW(int event, bool losingFocus, bool editorSave);

BOOL LoadFarVersion();

BOOL OutDataAlloc(DWORD anSize); // необязательно
BOOL OutDataWrite(LPVOID apData, DWORD anSize);

void CheckMacro();
int ShowMessage(int aiMsg, int aiButtons);
int ShowMessageA(int aiMsg, int aiButtons);
int ShowMessage684(int aiMsg, int aiButtons);
int ShowMessage757(int aiMsg, int aiButtons);
void ReloadMacroA();
void ReloadMacro684();
void ReloadMacro757();
