
/*
Copyright (c) 2009-present Maximus5
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

#include "Common.h"
#include "CEStr.h"


// GCC fixes
#ifndef KEY_WOW64_32KEY
#define KEY_WOW64_32KEY 0x0200
#endif
#ifndef KEY_WOW64_64KEY
#define KEY_WOW64_64KEY 0x0100
#endif

// Win console defines
typedef BOOL (WINAPI* AttachConsole_t)(DWORD dwProcessId);

// Some WinAPI related functions

/// <summary>
/// Searches for a <ref>lpFileName</ref> with optional <ref>lpExtension</ref>
/// in either specified <ref>lpPath</ref> or default search PATH defined in System.
/// Look full description in WinAPI SearchPath.
/// </summary>
/// <param name="lpPath">nullptr or directory to search.</param>
/// <param name="lpFileName">File name, extension is optional.</param>
/// <param name="lpExtension">nullptr or ".ext", should start with dot.</param>
/// <param name="rsPath">output CEStr</param>
/// <returns></returns>
int apiSearchPath(LPCWSTR lpPath, LPCWSTR lpFileName, LPCWSTR lpExtension, CEStr& rsPath);

int apiGetFullPathName(LPCWSTR lpFileName, CEStr& rsPath);

/// <summary>
/// Check if file or directory exists
/// </summary>
/// <param name="asFilePath">Full or relative path to the file or directory</param>
/// <param name="pnSize">pointer to uint64_t - for files it's filled with file size</param>
/// <returns>true - if file or directory exists</returns>
bool FileExists(const wchar_t* asFilePath, uint64_t* pnSize = nullptr);

bool FileSearchInDir(LPCWSTR asFilePath, CEStr& rsFound);

class CEnvRestorer;
typedef bool (*SearchAppPaths_t)(LPCWSTR asFilePath, CEStr& rsFound, bool abSetPath, CEnvRestorer* rpsPathRestore /*= NULL*/);
extern SearchAppPaths_t gfnSearchAppPaths /*= NULL*/;
bool FileExistsSearch(LPCWSTR asFilePath, CEStr& rsFound, bool abSetPath = true);

bool GetShortFileName(LPCWSTR asFullPath, int cchShortNameMax, wchar_t* rsShortName/*[MAX_PATH+1]-name only*/, BOOL abFavorLength=FALSE);
CEStr GetShortFileNameEx(LPCWSTR asLong, BOOL abFavorLength=TRUE);

DWORD GetModulePathName(HMODULE hModule, CEStr& lsPathName);
DWORD GetCurrentModulePathName(CEStr& lsPathName);

OSVERSIONINFOEXW MakeOsVersionEx(DWORD dwMajorVersion, DWORD dwMinorVersion);
bool _VerifyVersionInfo(LPOSVERSIONINFOEXW lpVersionInformation, DWORD dwTypeMask, DWORDLONG dwlConditionMask);
bool IsWinDBCS();
bool IsHwFullScreenAvailable();
bool GetOsVersionInformational(OSVERSIONINFO* pOsVer);
bool IsWinVerOrHigher(WORD osVerNum); // Ex: 0x0601, _WIN32_WINNT_WIN10, ...
bool IsWinVerEqual(WORD osVerNum);
bool IsWin2kEql();
bool IsWin5family();
bool IsWinXP();
bool IsWinXP(WORD servicePack);
bool IsWin6();
bool IsWin7();
bool IsWin7Eql();
bool IsWin8();
// ReSharper disable once CppInconsistentNaming
bool IsWin8_1();
bool IsWin10();
bool IsWindows64();
bool IsWine();
bool IsWinPE();

CEStr ExpandMacroValues(LPCWSTR pszFormat, LPCWSTR* pszValues, size_t nValCount);

CEStr GetComspec(const ConEmuComspec* pOpt);
LPCWSTR GetComspecFromEnvVar(wchar_t* pszComspec, DWORD cchMax, ComSpecBits Bits = csb_SameOS);

bool IsExportEnvVarAllowed(LPCWSTR szName);
void ApplyExportEnvVar(LPCWSTR asEnvNameVal);

bool CoordInSmallRect(const COORD& cr, const SMALL_RECT& rc);

UINT GetCpFromString(LPCWSTR asString, LPCWSTR* ppszEnd = NULL);

template <typename Func>
Func GetProcAddress(Func& fn, HMODULE module, const char* name)
{
	fn = reinterpret_cast<Func>(module ? ::GetProcAddress(module, name) : nullptr);
	return fn;
}

// Should not be used as time points, only for time differences.
DWORD GetTicks();
