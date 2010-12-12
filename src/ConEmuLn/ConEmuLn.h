
/*
Copyright (c) 2010 Maximus5
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

#include "../common/common.hpp"
#include "../common/farcolor.hpp"
#include "ConEmuLn_Lang.h"

// X - меньшая, Y - большая
#define FAR_X_VER 995
#define FAR_Y_VER 995
#define FUNC_X(fn) fn##995
#define FUNC_Y(fn) fn##995



extern HMODULE ghPluginModule;
extern wchar_t* gszRootKey;
extern FarVersion gFarVersion;

//Settings
extern BOOL gbBackgroundEnabled;
extern COLORREF gcrLinesColor;
extern BOOL gbHilightPlugins;
extern COLORREF gcrHilightPlugBack;

BOOL LoadFarVersion();
int WINAPI PaintConEmuBackground(struct PaintBackgroundArg* pBk);
void StartPlugin(BOOL bConfigure);
void ExitPlugin(void);
void FUNC_X(ExitFARW)(void);
void FUNC_Y(ExitFARW)(void);
void FUNC_X(SetStartupInfoW)(void *aInfo);
void FUNC_Y(SetStartupInfoW)(void *aInfo);
int FUNC_X(ConfigureW)(int ItemNumber);
int FUNC_Y(ConfigureW)(int ItemNumber);
LPCWSTR GetMsgW(int aiMsg);
LPCWSTR FUNC_Y(GetMsgW)(int aiMsg);
LPCWSTR FUNC_X(GetMsgW)(int aiMsg);
