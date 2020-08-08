
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

#include "../common/defines.h"
#include "RunMode.h"

typedef DWORD AttachModeEnum;
const AttachModeEnum
	am_Simple  = 0x0001,    // As is
	am_Auto    = 0x0002,    // Same as am_Simple, but always return 0 as errorlevel
	am_Modes   = (am_Simple|am_Auto),
	am_Async   = 0x0010,    // "/AUTOATTACH" must be async to be able to call from cmd prompt
	am_DefTerm = 0x0020,    // "/
	am_Admin   = 0x1000,    // Special "attach" when ConEmu is run under "User" and console "As admin"
	am_None    = 0
;

struct ConsoleState final
{
	ConsoleState();
	~ConsoleState();
	// Not copyable!
	ConsoleState& operator=(const ConsoleState&) = delete;
	ConsoleState(const ConsoleState&) = delete;
	ConsoleState& operator=(ConsoleState&&) = delete;
	ConsoleState(ConsoleState&&) = delete;

	void DisableAutoConfirmExit(bool fromFarPlugin = false);

	/// Running mode: server, comspec, guimacro, attach, etc.
	RunMode runMode_ = RunMode::Undefined;

	/// If server was started <b>not</b> from ConEmu.exe,
	/// but from Far plugin, CmdAutoAttach, -new_console, -GuiAttach, -Admin
	AttachModeEnum attachMode_ = am_None;
	/// if true - Server is not an owner of the console (root process of this console window)
	bool alienMode_ = false;
	/// If true - we don't create a process in console. <br>
	/// It slightly differs from alienMode_ because is used in any run mode (comspec, etc.)
	bool noCreateProcess_ = false;

	/// If true - show "Press Enter or Esc to close..."
	bool alwaysConfirmExit_ = false;
	/// if true and root process worked enough time (10 sec) than alwaysConfirmExit_ will be reset
	bool autoDisableConfirmExit_ = false;
	/// Set to true when we are in ExitWaitForKey function
	bool inExitWaitForKey_ = false;
	/// true if root process worked less than CHECK_ROOT_OK_TIMEOUT (10 sec)
	// ReSharper disable once CppInconsistentNaming
	bool rootAliveLess10sec_ = false;
	/// if true - triggers bufferheight during sst_ServerStart
	bool rootIsCmdExe_ = true;

	/// Real console window handle
	HWND    gpState->realConWnd = nullptr;
	/// PID of ConEmu[64].exe (ghConEmuWnd)
	DWORD   gnConEmuPID = 0;
	/// Root! window
	HWND    ghConEmuWnd = nullptr;
	/// ConEmu DC window
	HWND    ghConEmuWndDC = nullptr;
	/// ConEmu Back window
	HWND    ghConEmuWndBack = nullptr;

};

extern ConsoleState* gpState;
