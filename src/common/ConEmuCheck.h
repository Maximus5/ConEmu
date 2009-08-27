
#pragma once

#include "common.hpp"

#define VirtualConsoleClass L"VirtualConsoleClass"
#define VirtualConsoleClassMain L"VirtualConsoleClass"
#define VirtualConsoleClassApp L"VirtualConsoleClassApp"
#define VirtualConsoleClassBack L"VirtualConsoleClassBack"
#define VirtualConsoleClassScroll L"VirtualConsoleClassScroll"

// Service function
HWND AtoH(char *Str, int Len);


// 0 -- All OK (ConEmu found, Version OK)
// 1 -- NO ConEmu (simple console mode)
int ConEmuCheck(HWND* ahConEmuWnd);


// Returns HWND of Gui console DC window (abRoot==FALSE),
//              or Gui Main window (abRoot==TRUE)
HWND GetConEmuHWND(BOOL abRoot);

// hConEmuWnd - HWND с отрисовкой!
void SetConEmuEnvVar(HWND hConEmuWnd);

HANDLE ExecuteOpenPipe(const wchar_t* szPipeName, wchar_t* pszErr/*[MAX_PATH*2]*/, const wchar_t* szModule);
CESERVER_REQ* ExecuteNewCmd(DWORD nCmd, DWORD nSize);
CESERVER_REQ* ExecuteGuiCmd(HWND hConWnd, const CESERVER_REQ* pIn, HWND hOwner);
CESERVER_REQ* ExecuteSrvCmd(DWORD dwSrvPID, const CESERVER_REQ* pIn, HWND hOwner);
void ExecuteFreeResult(CESERVER_REQ* pOut);


HWND myGetConsoleWindow();

extern SECURITY_ATTRIBUTES* gpNullSecurity;
