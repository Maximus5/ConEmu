
/*
Copyright (c) 2009-2010 Maximus5
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

#include "../ConEmuCD/ConsoleHelp.h"

//L"Console emulation program.\r\n" \
//L"Home page: http://conemu-maximus5.googlecode.com\r\n" \
//L"http://code.google.com/p/conemu-maximus5/wiki/Command_Line\r\n"\
//"\r\n" \

#define pCmdLine \
	L"Command line examples\r\n" \
	L"––––––––––––––––––––––\r\n" \
	L"ConEmu.exe /cmd Far.exe /w\r\n" \
	L"ConEmu.exe /font \"Lucida Console\" /size 16 /bufferheight 9999 /cmd powershell\r\n" \
	L"ConEmu.exe /config \"Hiew\" /cmd \"C:\\Tools\\HIEW32.EXE\"\r\n" \
	L"ConEmu.exe /cmd {Shells}\r\n" \
	L"ConEmu.exe /tsa /min /icon \"cmd.exe\" /cmd cmd /c dir c:\\ /s\r\n" \
	L"\r\n" \
	L"By default (started without args) this program launches \"Far.exe\", \"tcc.exe\" or \"cmd.exe\" (which can be found).\r\n" \
	L"\r\n" \
	L"Command line switches:\r\n" \
	L"/? - This help screen.\r\n" \
	L"/config <configname> - Use alternative named configuration.\r\n" \
	L"/fs | /max | /min - (Full screen), (Maximized) or (Minimized) mode.\r\n" \
	L"/tsa - Override (enable) minimize to taskbar status area.\r\n" \
	L"/icon <file> - Take icon from file (exe, dll, ico).\r\n" \
	L"/multi | /nomulti - Enable or disable multiconsole features.\r\n" \
	L"/single - New console will be started in new tab of existing ConEmu.\r\n" \
	L"/noupdate - Disable automatic checking for updates on startup\r\n" \
	L"/ct[0|1] - Anti-aliasing: /ct0 - off, /ct1 - standard, /ct - cleartype.\r\n" \
	L"/font <fontname> - Specify the font name.\r\n" \
	L"/size <fontsize> - Specify the font size.\r\n" \
	L"/fontfile <fontfilename> - Loads fonts from file.\r\n" \
	L"/BufferHeight <lines> - Set console buffer height.\r\n" \
	L"/Log[1|2] - Used to create debug log files.\r\n" \
	/* L"/Attach [PID] - intercept console of specified process\n" */ \
	L"/cmd <commandline>|@<taskfile>|{taskname} - Command line to start. This must be the last used switch.\r\n" \
	L"\r\n" \
	L"Read more online: http://code.google.com/p/conemu-maximus5/wiki/Command_Line\r\n"
	//L"\x00A9 2006-2008 Zoin (based on console emulator by SEt)\r\n" \
	//CECOPYRIGHTSTRING_W /*\x00A9 2009-2011 ConEmu.Maximus5@gmail.com*/ L"\r\n" \

#define pAboutContributors \
	L"Thanks to all testers and reporters! You help to make the ConEmu better.\r\n" \
	L"\r\n" \
	L"Special Thanks\r\n" \
	L"––––––––––––––––––––––\r\n" \
	L"thecybershadow: bdf support\r\n" \
	L"NightRoman: drawing optimization, BufferHeight and other fixes\r\n" \
	L"dolzenko_: windows switching via GUI tabs\r\n" \
	L"alex_itd: Drag'n'Drop, RightClick, AltEnter\r\n" \
	L"Mors: loading font from file."

#define pAboutTitle \
	L"Console emulation program. Visit home page:"
//#define pAboutUrl
//	L"http://conemu-maximus5.googlecode.com"

#define pAboutLicense \
	L"\x00A9 2006-2008 Zoin (based on console emulator by SEt)\r\n" \
	CECOPYRIGHTSTRING_W /*\x00A9 2009-2011 ConEmu.Maximus5@gmail.com*/ L"\r\n" \
	L"\r\n" \
	L"Permission to use, copy, modify, and distribute this software for any\r\n" \
	L"purpose with or without fee is hereby granted, provided that the above\r\n" \
	L"copyright notice and this permission notice appear in all copies.\r\n" \
	L"\r\n" \
	L"THE SOFTWARE IS PROVIDED 'AS IS' AND THE AUTHOR DISCLAIMS ALL WARRANTIES\r\n" \
	L"WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF\r\n" \
	L"MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR\r\n" \
	L"ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES\r\n" \
	L"WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN\r\n" \
	L"ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF\r\n" \
	L"OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.\r\n"


#define pAbout \
    L"ConEmu-Maximus5 is a Windows console emulator with tabs, which represents " \
    L"multiple consoles and simple GUI applications as one customizable GUI window " \
    L"with various features.\r\n" \
    L"\r\n" \
    L"Basic application - joint use with Far Manager (a console program for managing files and " \
    L"archives in Windows operating systems).\r\n" \
	L"\r\n" \
	L"By default this program launches \"Far.exe\" (if exists) or \"tcc.exe\"/\"cmd.exe\".\r\n" \
	L"\r\n" \
	L"\x00A9 2006-2008 Zoin (based on console emulator by SEt)\r\n" \
	CECOPYRIGHTSTRING_W /*\x00A9 2009-2011 ConEmu.Maximus5@gmail.com*/ L"\r\n" \
	L"\r\n" \
	L"Read more online: http://code.google.com/p/conemu-maximus5/wiki/ConEmu\r\n"


#define pConsoleHelpFull \
	pConsoleHelp

#define pDosBoxHelpFull \
	L"ConEmu supports DosBox, read more online:\r\n" \
	L"http://code.google.com/p/conemu-maximus5/wiki/DosBox\r\n" \
	L"––––––––––––––––––––––\r\n" \
	pDosBoxHelp

#define pGuiMacro \
	L"Sort of simple macro language, read more online:\r\n" \
	L"http://code.google.com/p/conemu-maximus5/wiki/GuiMacro\r\n" \
	L"––––––––––––––––––––––\r\n" \
    L"FindEditor(\"<FullEditFileName>\")\r\n" \
    L"FindViewer(\"<FullViewerFileName>\")\r\n" \
    L"FindFarWindow(<WindowType>,\"<WindowTitle>\")\r\n" \
    L"  - Returns \"Found:<index of RealConsole>\", \"Blocked\", or \"NotFound\"\r\n" \
    L"     <WindowType> is number from Far Manager enum WINDOWINFO_TYPE\r\n" \
    L"FontSetName(\"<FontFaceName>\"[,FontHeight[,FontWidth]])\r\n" \
    L"  - Change font face name and (optionally) height/width.\r\n" \
    L"FontSetSize(Relative,N)\r\n" \
    L"  - Change font height in the ConEmu window\r\n" \
    L"     Relative==0: N - required font height in points\r\n" \
    L"     Relative==1: N (+-1, +-2) - increase/decrease font height\r\n" \
    L"     returns - \"OK\", or \"InvalidArg\"\r\n" \
    L"IsConEmu\r\n" \
    L"  - Returns \"Yes\" when console was started under !ConEmu\r\n" \
    L"IsConsoleActive\r\n" \
    L"  - Check, is RealConsole active or not, \"Yes\"/\"No\"\r\n" \
    L"IsRealVisible\r\n" \
    L"  - Check, is RealConsole visible or not, \"Yes\"/\"No\"\r\n" \
    L"MsgBox(\"<Text>\",\"<Title>\",<ButtonType>)\r\n" \
    L"  - Show modal GUI MessageBox, returns clicked button name (\"Ok\", \"Cancel\", \"Retry\", etc.),\r\n" \
    L"     ButtonType (number) is from Windows SDK.\r\n" \
    L"Paste(<Cmd>[,\"<Text>\"])\r\n" \
    L"  - When <Text> is omitted - paste from Windows clipboard, otherwise - paste specified text\r\n" \
    L"     Cmd==0: paste all lines\r\n" \
    L"     Cmd==1: paste first line\r\n" \
    L"     Cmd==2: paste all lines, without confirmations\r\n" \
    L"     Cmd==3: paste first line, without confirmations\r\n" \
    L"Shell(\"<Verb>\",\"<File>\"[,\"<Parms>\"[,\"<Dir>\"[,<ShowCmd>]]])\r\n" \
    L"  - Verb can be \"open\", \"print\" and so on, or special value \"new_console\",\r\n" \
    L"     which starts File in the new tab of ConEmu window. Examples:\r\n" \
    L"        Shell(\"open\",\"cmd.exe\",\"/k\")\r\n" \
    L"        Shell(\"\",\"\",\"cmd.exe /k -new_console:b\"),\r\n" \
    L"        Shell(\"new_console:b\",\"\",\"cmd.exe /k\")\r\n" \
    L"Status(0[,<Parm>])\r\n" \
    L"  - Show/Hide status bar, Parm=1 - Show, Parm=2 - Hide\r\n" \
    L"Status(1[,\"<Text>\"])\r\n" \
    L"  - Set status bar text\r\n" \
    L"Tab(<Cmd>[,<Parm>])\r\n" \
    L"  - Control ConEmu tabs\r\n" \
    L"     Cmd==0 - show/hide tabs\r\n" \
    L"     Cmd==1 - commit lazy changes\r\n" \
    L"     Cmd==2 - switch next (eq. CtrlTab)\r\n" \
    L"     Cmd==3 - switch prev (eq. CtrlShiftTab)\r\n" \
    L"     Cmd==4 - switch tab direct (no recent mode), Parm=(1,-1)\r\n" \
    L"     Cmd==5 - switch tab recent, Parm=(1,-1)\r\n" \
    L"     Cmd==6 - switch console direct (no recent mode), Parm=(1,-1)\r\n" \
    L"     Cmd==7 - activate console by number, Parm=(1-based console index)\r\n" \
    L"     Cmd==8 - show tabs list menu (indiffirent Far/Not Far)\r\n" \
    L"Task(Index)\r\n" \
    L"  - start task with 1-based index\r\n" \
    L"Task(\"Name\")\r\n" \
	L"  - start task with specified name\r\n" \
    L"WindowFullscreen()\r\n" \
    L"  - Switch fullscreen window mode\r\n" \
    L"     returns previous window mode\r\n" \
    L"WindowMaximize()\r\n" \
    L"  - Switch maximized window mode\r\n" \
    L"     returns previous window mode\r\n" \
    L"WindowMinimize() or WindowMinimize(1)\r\n" \
    L"  - Minimize ConEmu window (\"1\" means \"minimize to the TSA\")\r\n" \
    L"     returns previous window mode\r\n" \
    L"WindowMode([\"<Mode>\"])\r\n" \
    L"  - Returns or set current window mode\r\n" \
    L"     \"NOR\", \"MAX\", \"FS\", \"MIN\", \"TSA\"\r\n" \
    L""
