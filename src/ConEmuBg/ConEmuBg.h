
/*
Copyright (c) 2010-present Maximus5
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

#include "../common/Common.h"
#include "ConEmuBg_Lang.h"

// X - меньшая, Y - большая
#define FAR_X_VER 995
#define FAR_Y1_VER 1900
#define FAR_Y2_VER 2800
#define FUNC_X(fn) fn##995
#define FUNC_Y1(fn) fn##1900
#define FUNC_Y2(fn) fn##2800



extern HMODULE ghPluginModule;
//extern wchar_t* gszRootKey;
extern FarVersion gFarVersion;

//Settings
extern BOOL gbBackgroundEnabled;
extern wchar_t gsXmlConfigFile[MAX_PATH];
extern BOOL gbMonitorFileChange;
extern const wchar_t* szDefaultXmlName;
bool CheckXmlFile(bool abUpdateName = false);
bool WasXmlLoaded();
//extern COLORREF gcrLinesColor;
//extern int giHilightType; // 0 - линии, 1 - полосы
//extern BOOL gbHilightPlugins;
//extern COLORREF gcrHilightPlugBack;
struct ConEmuBgSettings
{
	LPCWSTR pszValueName;
	LPBYTE  pValue;
	DWORD   nValueType;
	DWORD   nValueSize;
};
extern ConEmuBgSettings gSettings[];
void SettingsLoad();
void SettingsLoadReg(LPCWSTR pszRegKey);
void SettingsLoadA();
void FUNC_X(SettingsLoadW)();
void FUNC_Y1(SettingsLoadW)();
void FUNC_Y2(SettingsLoadW)();
void SettingsSave();
void SettingsSaveReg(LPCWSTR pszRegKey);
void SettingsSaveA();
void FUNC_X(SettingsSaveW)();
void FUNC_Y1(SettingsSaveW)();
void FUNC_Y2(SettingsSaveW)();

void FUNC_X(GetPluginInfoW)(void *piv);
void FUNC_Y1(GetPluginInfoW)(void *piv);
void FUNC_Y2(GetPluginInfoW)(void *piv);

HANDLE OpenPluginWcmn(int OpenFrom,INT_PTR Item);
HANDLE WINAPI OpenPluginW1(int OpenFrom,INT_PTR Item);
HANDLE WINAPI OpenPluginW2(int OpenFrom,const GUID* Guid,INT_PTR Data);

BOOL LoadFarVersion();
int WINAPI PaintConEmuBackground(struct PaintBackgroundArg* pBk);
void StartPlugin(BOOL bConfigure);
void ExitPlugin(void);
void FUNC_X(ExitFARW)(void);
void FUNC_Y1(ExitFARW)(void);
void FUNC_Y2(ExitFARW)(void);
void FUNC_X(SetStartupInfoW)(void *aInfo);
void FUNC_Y1(SetStartupInfoW)(void *aInfo);
void FUNC_Y2(SetStartupInfoW)(void *aInfo);
int FUNC_X(ConfigureW)(int ItemNumber);
int FUNC_Y1(ConfigureW)(int ItemNumber);
int FUNC_Y2(ConfigureW)(int ItemNumber);
LPCWSTR GetMsgW(int aiMsg);
LPCWSTR FUNC_X(GetMsgW)(int aiMsg);
LPCWSTR FUNC_Y1(GetMsgW)(int aiMsg);
LPCWSTR FUNC_Y2(GetMsgW)(int aiMsg);
bool FMatchA(LPCSTR asMask, LPSTR asPath);
bool FUNC_X(FMatchW)(LPCWSTR asMask, LPWSTR asPath);
bool FUNC_Y1(FMatchW)(LPCWSTR asMask, LPWSTR asPath);
bool FUNC_Y2(FMatchW)(LPCWSTR asMask, LPWSTR asPath);
int GetMacroArea();
int FUNC_X(GetMacroAreaW)();
int FUNC_Y1(GetMacroAreaW)();
int FUNC_Y2(GetMacroAreaW)();

HANDLE WINAPI FUNC_Y1(OpenW)(const void* apInfo);
HANDLE WINAPI FUNC_Y2(OpenW)(const void* apInfo);
