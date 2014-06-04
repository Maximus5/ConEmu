
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

#include <Windows.h>

enum CESwitchType
{
	sw_Simple,  // just a bool
	sw_Int,     // number, may be min/max specified
	sw_FnSet,   // "/log1" -> Int=1
	sw_Str,     // string, may be min/max length specified
	sw_EnvStr,  // expanded env string, may be min/max length specified
	sw_PathStr, // path, may contains env strings, may be min/max length specified
	sw_Cmd,     // "/cmd ..."
	sw_CmdList, // "/cmdlist ..."
};

struct CESwitch
{
	// The value
	union
	{
		bool     Bool;
		int      Int;
		wchar_t* Str;
	};
	// Type
	CESwitchType Type;
	// Was specified in command line?
	bool Exists;
	// CTOR
	CESwitch()
	{
		Str = NULL; Exists = false;
	};
	// Helpers
	void Disable()
	{
		Exists = false;
	};
	bool GetBool()
	{
		_ASSERTE(Type==sw_Simple || Type==sw_Revert);
		return (Exists && Bool);
	};
	void SetBool(bool NewVal)
	{
		Bool = NewVal;
		Exists = true;
		if (Type != sw_Simple && Type != sw_Revert)
		{
			_ASSERTE(Type == sw_Simple || Type == sw_Revert);
			Type = sw_Simple;
		}
	};
	operator bool()
	{
		return GetBool();
	};
	LPCWSTR GetStr()
	{
		return (Exists && (Type == sw_Str || Type == sw_EnvStr && Type == sw_PathStr)) ? Str : NULL;
	};
	int GetInt()
	{
		_ASSERTE(Type==sw_Int);
		return (Exists && (Type == sw_Int)) ? Int : 0;
	};
};

class CSettingsCur
{
public:
	CSettingsCur();
	~CSettingsCur();
protected:
	bool     isScript; // Список шеллов передан через '/cmdlist'
	bool     isSwitch; // Первый аргумент начинается с '/' (или '-')
	// т.е. комстрока такая "ConEmu.exe /cmd c:\tools\far.exe"
	// а не такая "ConEmu.exe c:\tools\far.exe"

	LPCWSTR cmdLinePtr;  // That is actually a value of GetCommandLine()
	LPCWSTR cmdLineRest; // The command line after '/cmd' or '/cmdline' switches

	wchar_t* cmdNew;
	wchar_t* psUnknown;

public:
	CESwitch AboutParm;
	CESwitch AnsiLogPathParm;
	CESwitch BasicParm;
	CESwitch BufferHeightParm;
	CESwitch ClearTypeParm; // {{CLEARTYPE_NATURAL_QUALITY}};
	CESwitch ConfigParm;
	CESwitch DisableAllHotkeysParm;
	CESwitch DisableAllMacroParm;
	CESwitch DisableAutoUpdateParm;
	CESwitch DisableCloseConfirmParm;
	CESwitch DisableKeybHooksParm;
	CESwitch DisableRegisterFontsParm;
	CESwitch DisableSetDefTermParm;
	CESwitch DontCascadeParm;
	CESwitch ExitAfterActionParm;
	CESwitch FindBugModeParm;
	CESwitch FontDirParm; // Может быть несколько, здесь остается только последний
	CESwitch FontFileParm; // Может быть несколько, здесь остается только последний
	CESwitch FontParm;
	CESwitch ForceMinTSAParm;
	CESwitch IconParm;
	CESwitch InsideParm;
	CESwitch LoadCfgFileParm;
	CESwitch LogParm;
	CESwitch MultiConParm;
	CESwitch NoMultiConParm;
	CESwitch NoSingleParm;
	CESwitch PaletteParm;
	CESwitch ResetDefaultParm;
	CESwitch ResetParm;
	CESwitch ReuseParm;
	CESwitch SaveCfgFileParm;
	CESwitch SessionParm;
	CESwitch SetUpDefTermParm;
	CESwitch ShowHideParm;
	CESwitch ShowHideTSAParm;
	CESwitch SingleParm;
	CESwitch SizeParm;
	CESwitch StartDetachedParm;
	CESwitch TitleParm; // Fixed text in ConEmu window title
	CESwitch UpdateJumpListParm;
	CESwitch UpdateSrcSetParm;
	CESwitch VisParm;
	CESwitch WindowMinParm;
	CESwitch WindowMinTSAParm;
	CESwitch WindowModeParm;
	CESwitch WindowStartTSAParm;
	CESwitch WorkDirParm;

public:
	bool HasMultiParms();
	bool HasResetSettings();
	bool HasSingleParms();
	bool HasWindowParms();
	bool HasWindowStartMinimized();
	bool HasWindowStartTSA();
	bool HasWindowStartNoClose();
public:
	bool IsMulti();
	bool IsSingleInstance();
public:
	bool LockedSingleInstance();
	bool LockedMulti();
public:
	SingleInstanceShowHideType SingleInstanceShowHide();
	RecreateActionParm GetDefaultCreateAction();
public:
	void ResetSingleInstanceShowHide();

private:
	static LPCWSTR SkipToCmdArg(LPCWSTR pszRest);
private:
	CESwitch AutoSetupParm;
	CESwitch ByPassParm;
	CESwitch DemoteParm;
private:
	static int Arg_AutoSetup(CESwitch& Sw, LPCWSTR pszArg, LPCWSTR& pszRest);
	static int Arg_ByPassDemote(CESwitch& Sw, LPCWSTR pszArg, LPCWSTR& pszRest);
	static int Arg_ClearType(CESwitch& Sw, LPCWSTR pszArg, LPCWSTR& pszRest);
	static int Arg_FontFileDir(CESwitch& Sw, LPCWSTR pszArg, LPCWSTR& pszRest);
	static int Arg_WindowMode(CESwitch& Sw, LPCWSTR pszArg, LPCWSTR& pszRest);
	static int Arg_Inside(CESwitch& Sw, LPCWSTR pszArg, LPCWSTR& pszRest);
	static int Arg_WorkDir(CESwitch& Sw, LPCWSTR pszArg, LPCWSTR& pszRest);
	static int Arg_Log(CESwitch& Sw, LPCWSTR pszArg, LPCWSTR& pszRest);
	static int Arg_Title(CESwitch& Sw, LPCWSTR pszArg, LPCWSTR& pszRest);
	static int Arg_About(CESwitch& Sw, LPCWSTR pszArg, LPCWSTR& pszRest);
public:
	static wchar_t* LoadAutorunValue(HKEY hkCmd, bool bClear);
	static int RegisterCmdAutorun(LPCWSTR pszBatchFile, bool bForceNewWnd, bool bForceClear/*if (!pszBatchFile)*/);
	static void ResetConman();

protected:
	friend int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
	int ParseCommandLine(LPCWSTR pszCmdLine);
};

extern CSettingsCur* gpSetCur;
