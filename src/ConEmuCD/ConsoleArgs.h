
/*
Copyright (c) 2015-present Maximus5
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

#include "Actions.h"
#include "../common/CEStr.h"
#include "../common/defines.h"
#include "../common/RConStartArgs.h"

enum class ConsoleMainMode;

enum class SwitchType
{
	None,
	Simple,  // just a bool
	Int,     // number, may be min/max specified
	Str,     // string, may be min/max length specified
};

struct Switch
{
	/// Type
	// ReSharper disable once CppInconsistentNaming
	SwitchType switchType = SwitchType::None;

	/// Was specified in command line?
	// ReSharper disable once CppInconsistentNaming
	bool exists = false;

	explicit Switch(SwitchType aType = SwitchType::None);
	~Switch();

	// Not copyable!
	Switch& operator=(const Switch&) = delete;
	Switch(const Switch&) = delete;
	Switch& operator=(Switch&&) = delete;
	Switch(Switch&&) = delete;

	void Clear();
};

struct SwitchBool final : public Switch
{
	/// The bool value
	// ReSharper disable once CppInconsistentNaming
	bool value = false;

	explicit SwitchBool(bool defaultValue = false);
	~SwitchBool() = default;

	// Not copyable!
	SwitchBool& operator=(const SwitchBool&) = delete;
	SwitchBool(const SwitchBool&) = delete;
	SwitchBool& operator=(SwitchBool&&) = delete;
	SwitchBool(SwitchBool&&) = delete;

	void Clear();

	bool GetBool() const;
	void SetBool(bool newVal);
	SwitchBool& operator=(bool newVal);
	explicit operator bool() const;
};

struct SwitchInt final : public Switch
{
public:
	using ValueType_t = int64_t;
	/// The int value
	// ReSharper disable once CppInconsistentNaming
	ValueType_t value = 0;

	SwitchInt();
	~SwitchInt() = default;

	// Not copyable!
	SwitchInt& operator=(const SwitchInt&) = delete;
	SwitchInt(const SwitchInt&) = delete;
	SwitchInt& operator=(SwitchInt&&) = delete;
	SwitchInt(SwitchInt&&) = delete;


	ValueType_t GetInt() const;
	ValueType_t SetInt(ValueType_t newVal);
	ValueType_t SetInt(const wchar_t* newVal, int radix = 10);
	SwitchInt& operator=(ValueType_t newVal);

	void Clear();
	bool IsValid() const;
};

struct SwitchStr : public Switch
{
	/// The string value
	// ReSharper disable once CppInconsistentNaming
	wchar_t* value = nullptr;

	SwitchStr();
	~SwitchStr();

	// Not copyable!
	SwitchStr& operator=(const SwitchStr&) = delete;
	SwitchStr(const SwitchStr&) = delete;
	SwitchStr& operator=(SwitchStr&&) = delete;
	SwitchStr(SwitchStr&&) = delete;

	void Clear();

	bool IsEmpty() const;
	LPCWSTR GetStr() const;
	void SetStr(const wchar_t* newVal);
	void SetStr(wchar_t*&& newVal);
	SwitchStr& operator=(const wchar_t* newVal);
};


class ConsoleArgs final
{
public:
	ConsoleArgs();
	~ConsoleArgs();

	// Not copyable!
	ConsoleArgs& operator=(const ConsoleArgs&) = delete;
	ConsoleArgs(const ConsoleArgs&) = delete;
	ConsoleArgs& operator=(ConsoleArgs&&) = delete;
	ConsoleArgs(ConsoleArgs&&) = delete;

protected:
	/* Processing helpers */
	static bool GetCfgParam(LPCWSTR& cmdLineRest, SwitchStr& Val);
	void ShowAttachMsgBox(const CEStr& szArg) const;
	static void ShowConfigMsgBox(const CEStr& szArg, LPCWSTR cmdLineRest);
	static void ShowServerStartedMsgBox(LPCWSTR asCmdLine);
	static void ShowComspecStartedMsgBox(LPCWSTR asCmdLine);
	static void ShowInjectsMsgBox(ConEmuExecAction mode, const wchar_t* asCmdLine, const wchar_t* param);
	void AddConEmuArg(LPCWSTR asSwitch, LPCWSTR asValue);
public:
	int ParseCommandLine(LPCWSTR pszCmdLine, ConsoleMainMode anWorkMode);
	bool IsAutoAttachAllowed() const;
	bool IsForceHideConWnd() const;

public:
	/// All arguments (unstripped command line)
	CEStr fullCmdLine_;
	/// The shell command line or script. e.g. "cmd.exe" or "cmd ||| powershell"
	CEStr command_;
	/// If the command_ is a {Task} this field contains task contents (first command)
	CEStr taskCommand_;
	/// config and xml file to pass
	CEStr conemuAddArgs_;
	/// number of parsed arguments, informational
	int   params_ = 0;

	/// If something unknown appeared in command line
	SwitchStr unknownSwitch_;

	/// Perform the check (e.g. IsConEmu) and return result as errorlevel (exit code):
	/// CERR_CHKSTATE_ON if true and CERR_CHKSTATE_OFF if false.
	ConEmuStateCheck eStateCheck_ = ConEmuStateCheck::None;
	/// Perform the action (e.g. RegConFont, InjectDefTerm, etc.)
	ConEmuExecAction eExecAction_ = ConEmuExecAction::None;
	/// print to StdOut process exit code (RunMode::RM_COMSPEC mainly)
	SwitchBool printRetErrLevel_;
	/// don't print to console (GuiMacro and ExportXXX modes)
	SwitchBool preferSilentMode_;
	/// Set GuiMacro result to environment variable "ConEmuMacroResult" and export it
	SwitchBool macroExportResult_;
	/// Switch format "-GUIMACRO[:PID|0xHWND][:T<tab>][:S<split>]". <br>
	/// It should be parsed as ArgGuiMacro(m_guiMacro.GetStr(), m_macroInst). <br>
	/// The GuiMacro itself is stored in m_command.
	SwitchStr guiMacro_;

	/// Controls behavior of "Press Enter or Esc to close..." <br>
	/// eConfDefault | eConfAlways | eConfNever | eConfEmpty | eConfHalt
	RConStartArgs::CloseConfirm confirmExitParm_ = RConStartArgs::eConfDefault;

	/// if true - don't show warnings in console about (known) third-party processes loaded into ConEmu space
	SwitchBool skipHookSettersCheck_;

	/// ChildGui attach, should contains hex HWND
	SwitchInt attachGuiAppWnd_;
	/// PID of already created process which is console root
	SwitchInt rootPid_;
	/// may be set with '/ROOTEXE' switch if used with '/TRMPID'. full path to root exe
	SwitchStr rootExe_;
	/// PID of far.exe process <br>
	/// Should store for RunMode::RM_COMSPEC long output
	SwitchInt parentFarPid_;
	/// Used to "silent" creation of RealConsole window from *.vshost.exe
	SwitchBool creatingHiddenConsole_;
	/// Server was started from DefTerm application (*.vshost.exe), console could be hidden
	SwitchBool defTermCall_;
	/// We should create CEDEFAULTTERMHOOK event and CEINSIDEMAPNAMEP mapping
	SwitchBool inheritDefTerm_;
	/// Don't inject ConEmuHk.dll into root process
	SwitchBool doNotInjectConEmuHk_;
	/// Attach was initiated from Far Manager
	SwitchBool attachFromFar_;
	/// Attach to existing real console without ConEmuHk injection
	SwitchBool alternativeAttach_;
	/// Don't wait for created process termination. Used to run processes in background.
	SwitchBool asyncRun_;
	/// Use "cmd /k ..." to run commands
	SwitchBool cmdK_;

	/// PID to attach the debugger
	SwitchStr debugPidList_;
	/// Start debugger only for command_
	SwitchBool debugExe_;
	/// Start debugger for command_ and for its child processes
	SwitchBool debugTree_;
	/// Dump debugging process memory and ask user for dump type
	SwitchBool debugDump_;
	/// Dump debugging process memory using mini-dump
	SwitchBool debugMiniDump_;
	/// Dump debugging process memory using full-dump
	SwitchBool debugFullDump_;
	/// Dump debugging process memory using optional time interval
	SwitchStr debugAutoMini_;

	/// (ENABLE_QUICK_EDIT_MODE|ENABLE_INSERT_MODE) etc.
	SwitchInt consoleModeFlags_;
	/// Request to hide created console window
	SwitchBool forceHideConWnd_;

	/// Initial console visible rect width
	SwitchInt visibleWidth_;
	/// Initial console visible rect height
	SwitchInt visibleHeight_;
	/// Initial console scroll width
	SwitchInt bufferWidth_;
	/// Initial console scroll height
	SwitchInt bufferHeight_;
	/// Real console font name. Length is expected to be lesser than LF_FACESIZE.
	SwitchStr consoleFontName_;
	/// Width for real console font
	SwitchInt consoleFontWidth_;
	/// Height for real console font
	SwitchInt consoleFontHeight_;
	/// Text, Back, PopText & PopBack color indexes for new real console
	SwitchInt consoleColorIndexes_;
	/// CD to %USERPROFILE% before creating root process. ConEmu can't CD during RunAs.
	SwitchBool needCdToProfileDir_;

	/// Enable internal logging
	SwitchBool isLogging_;

	/// PID of ConEmu[64].exe (gState.conemuWnd_)
	SwitchInt conemuPid_;
	/// HWND of ConEmu instance (root window)
	SwitchInt conemuHwnd_;
	/// If we need new ConEmu window
	SwitchBool requestNewGuiWnd_;
	/// Config name (passed via -config to ConEmu)
	SwitchStr configName_;
	/// Config file (usually xml, passed as -LoadCfgFile to ConEmu)
	SwitchStr configFile_;

};

extern ConsoleArgs* gpConsoleArgs;
