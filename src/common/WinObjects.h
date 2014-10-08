
/*
Copyright (c) 2009-2014 Maximus5
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
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

#include <windows.h>
#include <wchar.h>
#include "common.hpp"
#include "MSecurity.h"
#include "ConEmuCheck.h"


// GCC fixes
#ifndef KEY_WOW64_32KEY
#define KEY_WOW64_32KEY 0x0200
#endif
#ifndef KEY_WOW64_64KEY
#define KEY_WOW64_64KEY 0x0100
#endif

// WinAPI wrappers
BOOL apiSetForegroundWindow(HWND ahWnd);
BOOL apiShowWindow(HWND ahWnd, int anCmdShow);
BOOL apiShowWindowAsync(HWND ahWnd, int anCmdShow);
void getWindowInfo(HWND ahWnd, wchar_t (&rsInfo)[1024], bool bProcessName = false, LPDWORD pnPID = NULL);

typedef BOOL (WINAPI* AttachConsole_t)(DWORD dwProcessId);


// Some WinAPI related functions
wchar_t* GetShortFileNameEx(LPCWSTR asLong, BOOL abFavorLength=TRUE);
bool FileCompare(LPCWSTR asFilePath1, LPCWSTR asFilePath2);
bool FileExists(LPCWSTR asFilePath, DWORD* pnSize = NULL);
bool FilesExists(LPCWSTR asDirectory, LPCWSTR asFileList, bool abAll = false, int anListCount = -1);
bool DirectoryExists(LPCWSTR asPath);
bool MyCreateDirectory(wchar_t* asPath);
bool IsFilePath(LPCWSTR asFilePath, bool abFullRequired = false);
bool IsPathNeedQuot(LPCWSTR asPath);
bool IsUserAdmin();
bool GetLogonSID (HANDLE hToken, wchar_t **ppszSID);
bool IsWine();
bool IsWinPE();
bool IsWin10();
bool IsDbcs();
bool IsWindows64();
bool IsHwFullScreenAvailable();
bool IsVsNetHostExe(LPCWSTR asFilePatName);
int GetProcessBits(DWORD nPID, HANDLE hProcess = NULL);
bool CheckCallbackPtr(HMODULE hModule, size_t ProcCount, FARPROC* CallBack, BOOL abCheckModuleInfo, BOOL abAllowNTDLL = FALSE, BOOL abTestVirtual = TRUE);
bool IsModuleValid(HMODULE module, BOOL abTestVirtual = TRUE);
typedef struct tagPROCESSENTRY32W PROCESSENTRY32W;
bool GetProcessInfo(DWORD nPID, PROCESSENTRY32W* Info);
bool isTerminalMode();

void RemoveOldComSpecC();
const wchar_t* PointToName(const wchar_t* asFullPath);
const char* PointToName(const char* asFileOrPath);
const wchar_t* PointToExt(const wchar_t* asFullPath);
const wchar_t* Unquote(wchar_t* asParm, bool abFirstQuote = false);
wchar_t* ExpandMacroValues(LPCWSTR pszFormat, LPCWSTR* pszValues, size_t nValCount);
wchar_t* ExpandEnvStr(LPCWSTR pszCommand);
wchar_t* GetFullPathNameEx(LPCWSTR asPath);
wchar_t* JoinPath(LPCWSTR asPath, LPCWSTR asPart1, LPCWSTR asPart2 = NULL);

//BOOL FindConEmuBaseDir(wchar_t (&rsConEmuBaseDir)[MAX_PATH+1], wchar_t (&rsConEmuExe)[MAX_PATH+1]);


COORD MyGetLargestConsoleWindowSize(HANDLE hConsoleOutput);

#ifndef CONEMU_MINIMAL
HANDLE DuplicateProcessHandle(DWORD anTargetPID);
void FindComspec(ConEmuComspec* pOpt, bool bCmdAlso = true); // используется в GUI при загрузке настроек
void UpdateComspec(ConEmuComspec* pOpt, bool DontModifyPath = false);
void SetEnvVarExpanded(LPCWSTR asName, LPCWSTR asValue);
#endif
wchar_t* GetEnvVar(LPCWSTR VarName);
LPCWSTR GetComspecFromEnvVar(wchar_t* pszComspec, DWORD cchMax, ComSpecBits Bits = csb_SameOS);
wchar_t* GetComspec(const ConEmuComspec* pOpt);

bool IsExportEnvVarAllowed(LPCWSTR szName);
void ApplyExportEnvVar(LPCWSTR asEnvNameVal);

#ifndef CONEMU_MINIMAL
int ReadTextFile(LPCWSTR asPath, DWORD cchMax, wchar_t*& rsBuffer, DWORD& rnChars, DWORD& rnErrCode, DWORD DefaultCP = 0);
int WriteTextFile(LPCWSTR asPath, const wchar_t* asBuffer, int anSrcLen = -1, DWORD OutCP = CP_UTF8, bool WriteBOM = true, LPDWORD rnErrCode = NULL);
#endif
