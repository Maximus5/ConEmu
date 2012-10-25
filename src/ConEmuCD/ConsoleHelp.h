
/*
Copyright (c) 2009-2012 Maximus5
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

#ifdef _DEBUG
#define _DBGHLP(x) x
#else
#define _DBGHLP(x)
#endif

// Used via SetWindowText && _wprintf. Need not escape of "%".
#define pConsoleHelp \
	    L"This is a console part of ConEmu product.\r\n" \
	    L"Usage: ConEmuC [switches] [/U | /A] /C <command line, passed to %COMSPEC%>\r\n" \
	    L"   or: ConEmuC [switches] /ROOT <program with arguments, far.exe for example>\r\n" \
	    L"   or: ConEmuC /ATTACH /NOCMD\r\n" \
		L"   or: ConEmuC /ATTACH /[FAR|CON]PID=<PID>\r\n" \
	    L"   or: ConEmuC /GUIMACRO <ConEmu GUI macro command>\r\n" \
		L"   or: ConEmuC /DEBUGPID=<Debugging PID> [/DUMP | /MINI | /FULL]\r\n" \
_DBGHLP(L"   or: ConEmuC /REGCONFONT=<FontName> -> RegisterConsoleFontHKLM\r\n") \
	    L"   or: ConEmuC /?\r\n" \
	    L"Switches:\r\n" \
	    L"     /[NO]CONFIRM    - [don't] confirm closing console on program termination\r\n" \
	    L"     /ATTACH         - auto attach to ConEmu GUI\r\n" \
	    L"     /NOCMD          - attach current (existing) console to GUI\r\n" \
		L"     /[FAR]PID=<PID> - use <PID> as root process\r\n" \
	    L"     /B{W|H|Z}       - define window width, height and buffer height\r\n" \
_DBGHLP(L"     /BW=<WndX> /BH=<WndY> /BZ=<BufY>\r\n") \
	    L"     /F{N|W|H}    - define console font name, width, height\r\n" \
_DBGHLP(L"     /FN=<ConFontName> /FH=<FontHeight> /FW=<FontWidth>\r\n") \
	    L"     /LOG[N]      - create (debug) log file, N is number from 0 to 3\r\n" \
_DBGHLP(L"     /CINMODE==<hex:gnConsoleModeFlags>\r\n") \
_DBGHLP(L"     /HIDE -> gbForceHideConWnd=TRUE\r\n") \
_DBGHLP(L"     /GID=<ConEmu.exe PID>\r\n") \
_DBGHLP(L"     /GHWND=<ConEmu.exe HWND>\r\n") \
_DBGHLP(L"     /SETHOOKS=HP{16},PID{10},HT{16},TID{10},ForceGui\r\n") \
_DBGHLP(L"     /INJECT=PID{10}\r\n") \
_DBGHLP(L"     /DOSBOX -> use DosBox\r\n") \
	    L"\r\n" \
	    L"When you run application from ConEmu console, you may use one or more\r\n" \
_DBGHLP(L"  Switch: -new_console[:abcd[:dir]h[N]rx[N]y[N]u[:user:pwd]]\r\n") \
		L"  Switch: -new_console[:abcd[:dir]h[N][s<...>(H|V)]ru[:user:pwd]]\r\n" \
        L"     a - RunAs shell verb (as Admin on Vista+, login/passw in Win2k and WinXP)\r\n" \
        L"     b - Create background tab\r\n" \
        L"     c - force enable 'Press Enter or Esc to close console' (default)\r\n" \
		L"     d - specify working directory, MUST BE LAST OPTION\r\n" \
        L"     h<height> - i.e., h0 - turn buffer off, h9999 - switch to 9999 lines\r\n" \
        L"     i - don't inject ConEmuHk into starting process\r\n" \
_DBGHLP(L"     l - lock console size, do not sync it to ConEmu window\r\n") \
        L"     n - disable 'Press Enter or Esc to close console'\r\n" \
        L"     o - don't enable 'Long console output' when starting command from Far Manager\r\n" \
        L"     r - run as restricted user\r\n" \
        L"     s[<SplitTab>T][<Percents>](H|V)\r\n" \
_DBGHLP(L"     x<width>, y<height> - change size of visible area, use with 'l'\r\n") \
        L"     u - ConEmu choose user dialog\r\n" \
        L"     u:<user>:<pwd> - specify user/pwd in args, MUST BE LAST OPTION\r\n" \
        L"  Warning: Option 'Inject ConEmuHk' must be enabled in ConEmu settings!\r\n" \
		L"  Example: dir \"-new_console:bh9999c\" \"-new_console:dC:\\\"  c:\\Users /s\r\n" \
		L"  Note: Some switches may be used in similar \"-cur_console:...\"\r\n"


#define pDosBoxHelp \
		L"Starting DosBox, You may use following default combinations in DosBox window\r\n" \
		L"ALT-ENTER     Switch to full screen and back.\r\n" \
		L"ALT-PAUSE     Pause emulation (hit ALT-PAUSE again to continue).\r\n" \
		L"CTRL-F1       Start the keymapper.\r\n" \
		L"CTRL-F4       Change between mounted floppy/CD images. Update directory cache \r\n" \
		L"              for all drives.\r\n" \
		L"CTRL-ALT-F5   Start/Stop creating a movie of the screen. (avi video capturing)\r\n" \
		L"CTRL-F5       Save a screenshot. (PNG format)\r\n" \
		L"CTRL-F6       Start/Stop recording sound output to a wave file.\r\n" \
		L"CTRL-ALT-F7   Start/Stop recording of OPL commands. (DRO format)\r\n" \
		L"CTRL-ALT-F8   Start/Stop the recording of raw MIDI commands.\r\n" \
		L"CTRL-F7       Decrease frameskip.\r\n" \
		L"CTRL-F8       Increase frameskip.\r\n" \
		L"CTRL-F9       Kill DOSBox.\r\n" \
		L"CTRL-F10      Capture/Release the mouse.\r\n" \
		L"CTRL-F11      Slow down emulation (Decrease DOSBox Cycles).\r\n" \
		L"CTRL-F12      Speed up emulation (Increase DOSBox Cycles).\r\n" \
		L"ALT-F12       Unlock speed (turbo button/fast forward).\r\n" \
		L"F11, ALT-F11  (machine=cga) change tint in NTSC output modes\r\n" \
		L"F11           (machine=hercules) cycle through amber, green, white colouring\r\n"
