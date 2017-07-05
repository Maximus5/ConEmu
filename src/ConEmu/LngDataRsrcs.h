
/*
Copyright (c) 2016-2017 Maximus5
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

static LngPredefined gsDataRsrcs[] = {
	{ lng_AboutAppName,             L"Console Emulation program (local terminal)" },
	{ lng_AltNumberACP,             L"Type ANSI code point using decimal numbers" },
	{ lng_AltNumberExt,             L" - <Enter> to paste, <Esc> to cancel" },
	{ lng_AltNumberHex,             L"Type UNICODE code point using hexadecimal numbers" },
	{ lng_AltNumberStdACP,          L" - release <Alt> to paste ANSI code point" },
	{ lng_AltNumberStdOEM,          L" - release <Alt> to paste OEM code point" },
	{ lng_AltNumberStdUCS,          L" - release <Alt> to paste UNICODE code point" },
	{ lng_AssertAbortDescr,         L"Press <Abort> to throw exception, ConEmu will be terminated!" },
	{ lng_AssertIgnoreDescr,        L"Press <Ignore> to continue your work." },
	{ lng_AssertRetryDescr,         L"Press <Retry> to copy text information to clipboard\r\nand report a bug (open project web page)." },
	{ lng_BasicSettings,            L"Basic settings" },
	{ lng_CurClrScheme,             L"<Current color scheme>" },
	{ lng_DetachConConfirm,         L"Detach console from ConEmu?" },
	{ lng_DetachGuiConfirm,         L"Detach GUI application from ConEmu?" },
	{ lng_DlgAbout,                 L"About" },
	{ lng_DlgAffinity,              L"Set active console processes affinity and priority" },
	{ lng_DlgAttach,                L"Attach: Choose window or console application" },
	{ lng_DlgCreateNewConsole,      L"Create new console" },
	{ lng_DlgFastCfg,               L"fast configuration" },
	{ lng_DlgHotkey,                L"Choose hotkey" },
	{ lng_DlgRenameTab,             L"Rename tab" },
	{ lng_DlgRestartConsole,        L"Restart console" },
	{ lng_DlgSettings,              L"Settings" },
	{ lng_ExitCode,                 L"exit code" },
	{ lng_HotkeyDuplicated,         L"Hotkey <%s> is not unique" },
	{ lng_HotkeysAll,               L"All hotkeys" },
	{ lng_HotkeysFilter,            L"Filter hotkeys (Alt+F)" },
	{ lng_HotkeysMacro,             L"Macros" },
	{ lng_HotkeysSystem,            L"System" },
	{ lng_HotkeysUser,              L"User defined" },
	{ lng_JumpListUpdated,          L"Taskbar jump list was updated successfully" },
	{ lng_KeyNoMod,                 L"<No-Mod>" },
	{ lng_KeyNone,                  L"<None>" },
	{ lng_PasteEnterConfirm,        L"Warning!\n\nPasting multi-line text involves\nsending <Enter> keypresses to console!\n\nContinue?" },
	{ lng_PasteLongConfirm,         L"Warning!\n\nPasting long text (%u chars) may make the console\nnon-responsive until the paste finishes!\n\nContinue?" },
	{ lng_PleaseCheckManually,      L"Please check for updates manually" },
	{ lng_RasterAutoError,          L"Font auto size is not allowed for a fixed raster font size. Select ‘Terminal’ instead of ‘[Raster Fonts ...]’" },
	{ lng_Search,                   L"Search" },
	{ lng_SearchCtrlF,              L"Search (Ctrl+F)" },
	{ lng_SelBlock,                 L" block selection" },
	{ lng_SelChars,                 L" chars " },
	{ lng_SelStream,                L" stream selection" },
	{ lng_SizeRadioCentered,        L"Centered" },
	{ lng_SizeRadioFree,            L"Free" },
	{ lng_SpgANSI,                  L"ANSI execution" },
	{ lng_SpgAppDistinct,           L"App distinct" },
	{ lng_SpgAppear,                L"Appearance" },
	{ lng_SpgBackgr,                L"Background" },
	{ lng_SpgChildGui,              L"Children GUI" },
	{ lng_SpgColors,                L"Colors" },
	{ lng_SpgComSpec,               L"ComSpec" },
	{ lng_SpgConfirm,               L"Confirm" },
	{ lng_SpgCursor,                L"Text cursor" },
	{ lng_SpgDebug,                 L"Debug" },
	{ lng_SpgDefTerm,               L"Default term" },
	{ lng_SpgEnvironment,           L"Environment" },
	{ lng_SpgFarMacros,             L"Far macros" },
	{ lng_SpgFarManager,            L"Far Manager" },
	{ lng_SpgFarViews,              L"Panel Views" },
	{ lng_SpgFeatures,              L"Features" },
	{ lng_SpgFonts,                 L"Main" },
	{ lng_SpgHighlight,             L"Highlight" },
	{ lng_SpgInfo,                  L"Info" },
	{ lng_SpgIntegration,           L"Integration" },
	{ lng_SpgKeyboard,              L"Keyboard" },
	{ lng_SpgKeys,                  L"Keys & Macro" },
	{ lng_SpgMarkCopy,              L"Mark/Copy" },
	{ lng_SpgMouse,                 L"Mouse" },
	{ lng_SpgPaste,                 L"Paste" },
	{ lng_SpgQuake,                 L"Quake style" },
	{ lng_SpgSizePos,               L"Size & Pos" },
	{ lng_SpgStartup,               L"Startup" },
	{ lng_SpgStatusBar,             L"Status bar" },
	{ lng_SpgTabBar,                L"Tab bar" },
	{ lng_SpgTaskBar,               L"Task bar" },
	{ lng_SpgTasks,                 L"Tasks" },
	{ lng_SpgTransparent,           L"Transparency" },
	{ lng_SpgUpdate,                L"Update" },
	{ lng_UnknownSwitch1,           L"Unknown switch specified:" },
	{ lng_UnknownSwitch2,           L"Visit website to get thorough switches description:" },
	{ lng_UnknownSwitch3,           L"Or run ‘ConEmu.exe -?’ to get the brief." },
	{ lng_UnknownSwitch4,           L"Switch ‘-new_console’ must be specified *after* ‘-cmd’ or ‘-cmdlist’!" },
	{ lng_UpdateCloseMsg1,          L"Please, close all ConEmu instances before continuing" },
	{ lng_UpdateCloseMsg2,          L"ConEmu still running:" },
	{ /* empty trailing item for patch convenience */ }
};
