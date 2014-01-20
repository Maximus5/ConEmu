
/*
Copyright (c) 2009-2013 Maximus5
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

//L"Console emulation program.\r\n"
//L"Home page: http://conemu-maximus5.googlecode.com\r\n"
//L"http://code.google.com/p/conemu-maximus5/wiki/Command_Line\r\n"
//"\r\n"

#define pCmdLine \
	L"Command line examples\r\n" \
	VCGCCTEST(L"––––––––––––––––––––––\r\n",L"----------------------\r\n") \
	L"ConEmu.exe /cmd Far.exe /w\r\n" \
	L"ConEmu.exe /font \"Consolas\" /size 16 /bufferheight 9999 /cmd powershell\r\n" \
	L"ConEmu.exe /config \"Hiew\" /cmd \"C:\\Tools\\HIEW32.EXE\"\r\n" \
	L"ConEmu.exe /cmd {Shells}\r\n" \
	L"ConEmu.exe /tsa /min /icon \"cmd.exe\" /cmd cmd /c dir c:\\ /s\r\n" \
	L"\r\n"\
	L"By default (started without args) this program launches \"Far.exe\", \"tcc.exe\" or \"cmd.exe\" (which can be found).\r\n" \
	L"\r\n" \
	L"Command line switches (case insensitive):\r\n" \
	L"/? - This help screen.\r\n" \
	L"/Config <configname> - Use alternative named configuration.\r\n" \
	L"/Dir <workdir> - Set startup directory for ConEmu and consoles.\r\n" \
	L"/FS | /Max | /Min - (Full screen), (Maximized) or (Minimized) mode.\r\n" \
	L"/TSA - Override (enable) minimize to taskbar status area.\r\n" \
	L"/MinTSA - start minimized in taskbar status area on startup only.\r\n" \
	L"/Detached - start ConEmu without consoles.\r\n" \
	L"/Icon <file> - Take icon from file (exe, dll, ico).\r\n" \
	L"/Title <title> - Set fixed(!) title for ConEmu window. You may use environment variables in <title>.\r\n" \
	L"/Multi | /NoMulti - Enable or disable multiconsole features.\r\n" \
	L"/Single - New console will be started in new tab of existing ConEmu.\r\n" \
	L"/ShowHide | /ShowHideTSA - Works like \"Minimize/Restore\" global hotkey.\r\n" \
	L"/NoDefTerm - Don't start initialization procedure for setting up ConEmu as default terminal.\r\n" \
	L"/NoKeyHooks - Disable !SetWindowsHookEx and global hotkeys.\r\n" \
	L"/NoRegFonts - Disable auto register fonts (font files from ConEmu folder).\r\n" \
	L"/NoUpdate - Disable automatic checking for updates on startup\r\n" \
	L"/CT[0|1] - Anti-aliasing: /ct0 - off, /ct1 - standard, /ct - cleartype.\r\n" \
	L"/Font <fontname> - Specify the font name.\r\n" \
	L"/Size <fontsize> - Specify the font size.\r\n" \
	L"/FontFile <fontfilename> - Loads fonts from file.\r\n" \
	L"/BufferHeight <lines> - Set console buffer height.\r\n" \
	L"/Palette <name> - Choose named color palette.\r\n" \
	L"/Log[1|2] - Used to create debug log files.\r\n" \
	L"/Demote /cmd <command> - Run command de-elevated.\r\n" \
	L"/Reset - Don't load settings from registry/xml.\r\n" \
	L"/UpdateJumpList - Update Windows 7 taskbar jump list.\r\n" \
	L"/LoadCfgFile <file> - Use specified xml file as configuration storage.\r\n" \
	L"/SaveCfgFile <file> - Save configuration to the specified xml file.\r\n" \
	L"/SetDefTerm - Set ConEmu as default terminal, use with \"/Exit\" switch.\r\n" \
	L"/Exit - Don't create ConEmu window, exit after actions.\r\n" \
	/* L"/Attach [PID] - intercept console of specified process\n" */ \
	L"/cmd <commandline>|@<taskfile>|{taskname} - Command line to start. This must be the last used switch.\r\n" \
	L"\r\n" \
	L"Read more online: http://code.google.com/p/conemu-maximus5/wiki/Command_Line\r\n"
	//L"\x00A9 2006-2008 Zoin (based on console emulator by SEt)\r\n"
	//CECOPYRIGHTSTRING_W /*\x00A9 2009-2011 ConEmu.Maximus5@gmail.com*/ L"\r\n"

#define pAboutContributors \
	L"Thanks to all testers and reporters! You help to make the ConEmu better.\r\n" \
	L"\r\n" \
	L"Special Thanks\r\n" \
	VCGCCTEST(L"––––––––––––––––––––––\r\n",L"----------------------\r\n") \
	L"Rick Sladkey: bugfixes\r\n" \
	L"ForNeVeR: some ConEmuBg improvements\r\n" \
	L"thecybershadow: bdf support, Documentation\r\n" \
	L"NightRoman: drawing optimization, BufferHeight and other fixes\r\n" \
	L"dolzenko_: windows switching via GUI tabs\r\n" \
	L"alex_itd: Drag'n'Drop, RightClick, AltEnter\r\n" \
	L"Mors: loading font from file\r\n" \
	L"Grzegorz Kozub: new toolbar images"

#define pAboutTitle \
	L"Console emulation program. Visit home page:"
//#define pAboutUrl
//	L"http://conemu-maximus5.googlecode.com"

#define pAboutLicense \
	L"\x00A9 2006-2008 Zoin (based on console emulator by SEt)\r\n" \
	CECOPYRIGHTSTRING_W /*© 2009-2014 ConEmu.Maximus5@gmail.com*/ L"\r\n" \
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
	L"Online documentation: http://code.google.com/p/conemu-maximus5/wiki/ConEmu\r\n" \
	L"\r\n" \
	L"You can show your appreciation and support future development by donating. " \
	L"Donate (PayPal) button located on project website " \
	L"under ConEmu sketch (upper right of page)."


#define pConsoleHelpFull \
	pConsoleHelp \
	L"When you run application from ConEmu console, you may use one or more -new_console[:switches]\r\n" \
	L"Read about them on next page\r\n"

#define pNewConsoleHelpFull \
	pNewConsoleHelp


#define pDosBoxHelpFull \
	L"ConEmu supports DosBox, read more online:\r\n" \
	L"http://code.google.com/p/conemu-maximus5/wiki/DosBox\r\n" \
	VCGCCTEST(L"––––––––––––––––––––––\r\n",L"----------------------\r\n") \
	pDosBoxHelp

#define pGuiMacro \
	L"Sort of simple macro language, read more online:\r\n" \
	L"http://code.google.com/p/conemu-maximus5/wiki/GuiMacro\r\n" \
	VCGCCTEST(L"––––––––––––––––––––––\r\n",L"----------------------\r\n") \
	L"Close(<What>[,<Flags>])\r\n" \
	L"  - close current console (0), without confirmation (0,1),\r\n" \
	L"    terminate active process (1), without confirmation (1,1)\r\n" \
	L"    close ConEmu window (2), without confirmation (2,1)\r\n" \
	L"    close active tab (3)\r\n" \
	L"    close active group (4)\r\n" \
	L"    close all tabs but active (5)\r\n" \
	L"    close active tab or group (6)\r\n" \
	L"    close all active processes of the active group (7)\r\n" \
	L"    close all tabs (8)\r\n" \
	L"Copy(<What>[,<Format>])\r\n" \
	L"  - Copy active console contents\r\n" \
	L"    What==0: current selection\r\n" \
	L"    What==1: all buffer contents\r\n" \
	L"    Format==0: plain text, not formatting\r\n" \
	L"    Format==1: copy HTML format\r\n" \
	L"    Format==2: copy as HTML\r\n" \
	L"FindEditor(\"<FullEditFileName>\")\r\n" \
	L"FindViewer(\"<FullViewerFileName>\")\r\n" \
	L"FindFarWindow(<WindowType>,\"<WindowTitle>\")\r\n" \
	L"  - Returns \"Found:<index of RealConsole>\", \"Active:<Far window number>\",  \"Blocked\", or \"NotFound\"\r\n" \
	L"     <WindowType> is number from Far Manager enum WINDOWINFO_TYPE\r\n" \
	L"Flash(<Cmd>[,<Flags>[,<Count>]])\r\n" \
	L"  - Allows to flash taskbar icon and/or window caption\r\n" \
	L"    Flash(0) - Stop all flashing\r\n" \
	L"    Flash(0,1) - Simple flashing (see MSDN FlashWindow)\r\n" \
	L"Flash(1,<Flags>,<Count>)\r\n" \
	L"  - Special flashing (see MSDN FlashWindowEx)\r\n" \
	L"    Flags: 0 - stop, 1 - caption, 2 - taskbar, 3 - caption+taskbar, etc.\r\n" \
	L"    Count: the number of times to flash the window\r\n" \
	L"FontSetName(\"<FontFaceName>\"[,FontHeight[,FontWidth]])\r\n" \
	L"  - Change font face name and (optionally) height/width.\r\n" \
	L"FontSetSize(Relative,N)\r\n" \
	L"  - Change font height in the ConEmu window\r\n" \
	L"     Relative==0: N - required font height in points\r\n" \
	L"     Relative==1: N (+-1, +-2) - increase/decrease font height\r\n" \
	L"     returns - \"OK\", or \"InvalidArg\"\r\n" \
	L"HighlightMouse(<What>[,<Act>])\r\n" \
	L"  - change highlighting int the ACTIVE console only\r\n" \
	L"    What==0: switch off/row/col/row+col/off/...\r\n" \
	L"    What==1: row, What==2 - col, What==3 - row+col\r\n" \
	L"      Act==0 - off, Act==1 - on, Act==2 - switch (default)\r\n" \
	L"IsConEmu\r\n" \
	L"  - Returns \"Yes\" when console was started under !ConEmu\r\n" \
	L"IsConsoleActive\r\n" \
	L"  - Check, is RealConsole active or not, \"Yes\"/\"No\"\r\n" \
	L"IsRealVisible\r\n" \
	L"  - Check, is RealConsole visible or not, \"Yes\"/\"No\"\r\n" \
	L"MsgBox(\"<Text>\",\"<Title>\",<ButtonType>)\r\n" \
	L"  - Show modal GUI MessageBox, returns clicked button name (\"Ok\", \"Cancel\", \"Retry\", etc.),\r\n" \
	L"     ButtonType (number) is from Windows SDK.\r\n" \
	L"Palette([<Cmd>[,\"<NewPalette>\"|<NewPaletteIndex>]])\r\n" \
	L"  - Returns current palette name, change if <Cmd> specified\r\n" \
	L"     Cmd==0: return palette from ConEmu settings\r\n" \
	L"     Cmd==1: change palette in ConEmu settings, returns prev palette\r\n" \
	L"     Cmd==2: return palette from current console\r\n" \
	L"     Cmd==3: change palette in current console, returns prev palette\r\n" \
	L"     NewPalette: palette name (string)\r\n" \
	L"     NewPaletteIndex: 0-based index (number)\r\n" \
	L"Paste(<Cmd>[,\"<Text>])\r\n" \
	L"  - When <Text> is omitted - paste from Windows clipboard, otherwise - paste <Text>\r\n" \
	L"     Cmd==0: paste all lines\r\n" \
	L"     Cmd==1: paste first line\r\n" \
	L"     Cmd==2: paste all lines, without confirmations\r\n" \
	L"     Cmd==3: paste first line, without confirmations\r\n" \
	L"     Cmd==4: select and parse file pathname, Text - default\r\n" \
	L"     Cmd==5: select and parse folder pathname, Text - default\r\n" \
	L"     Cmd==6: select and parse cygwin file pathname, Text - default\r\n" \
	L"     Cmd==7: select and parse cygwin folder pathname, Text - default\r\n" \
	L"     Cmd==8: paste path from clipboard converted to CygWin style\r\n" \
	L"Print([\"<Text>\"])\r\n" \
	L"  - Alias for Paste(2,\"<Text>\")\r\n" \
	L"Progress(<Type>[,<Value>])\r\n" \
	L"  - Set progress state on taskbar and ConEmu title\r\n" \
	L"     Type=0: remove progress\r\n" \
	L"     Type=1: set progress value to <Value> (0-100)\r\n" \
	L"     Type=2: set error state in progress\r\n" \
	L"Rename(<Type>,[\"<Title>\"])\r\n" \
	L"  - Rename tab (Type=0) or console title (Type=1)\r\n" \
	L"SetOption(\"<Name>\",<Value>)\r\n" \
	L"  - Name=one of allowed for changing ConEmu options\r\n" \
	L"    \"AlwaysOnTop\": place ConEmu window above all non-topmost windows\r\n" \
	L"      Value: 2 - toggle, 1 - enable, 0 - disable\r\n" \
	L"    \"bgImageDarker\": darkening of background image\r\n" \
	L"      Value: 0 .. 255\r\n" \
	L"    \"QuakeAutoHide\": auto hide on focus lose, Quake mode\r\n" \
	L"      Value: 2 - switch auto-hide, 1 - enable, 0 - disable\r\n" \
	L"Shell(\"<Verb>\",\"<File>\"[,\"<Parms>\"[,\"<Dir>\"[,<ShowCmd>]]])\r\n" \
	L"  - Verb can be \"open\", \"print\" and so on, or special value \"new_console\",\r\n" \
	L"     which starts File in the new tab of ConEmu window. Examples:\r\n" \
	L"        Shell(\"open\",\"cmd.exe\",\"/k\")\r\n" \
	L"        Shell(\"\",\"\",\"cmd.exe /k -new_console:b\"),\r\n" \
	L"        Shell(\"new_console:b\",\"\",\"cmd.exe /k\")\r\n" \
	L"        Shell(\"new_console:sV\")\r\n" \
	L"Split(<Cmd>,<Horz>,<Vert>)\r\n" \
	L"  - Cmd=0, Horz=1..99, Vert=0: Duplicate active \"shell\" split to right\r\n" \
	L"    Cmd=0, Horz=0, Vert=1..99: Duplicate active \"shell\" split to bottom\r\n" \
	L"    Cmd=1, Horz=-1..1, Vert=-1..1: Move splitter between panes (aka resize panes)\r\n" \
	L"    Cmd=2, Horz=-1..1, Vert=-1..1: Put cursor to the nearest pane\r\n" \
	L"Status(0[,<Parm>])\r\n" \
	L"  - Show/Hide status bar, Parm=1 - Show, Parm=2 - Hide\r\n" \
	L"Status(1[,\"<Text>\"])\r\n" \
	L"  - Set status bar text\r\n" \
	L"Tab(<Cmd>[,<Parm>])\r\n" \
	L"  - Control ConEmu tabs\r\n" \
	L"     Cmd==0: show/hide tabs\r\n" \
	L"     Cmd==1: commit lazy changes\r\n" \
	L"     Cmd==2: switch next (eq. CtrlTab)\r\n" \
	L"     Cmd==3: switch prev (eq. CtrlShiftTab)\r\n" \
	L"     Cmd==4: switch tab direct (no recent mode), Parm=(1,-1)\r\n" \
	L"     Cmd==5: switch tab recent, Parm=(1,-1)\r\n" \
	L"     Cmd==6: switch console direct (no recent mode), Parm=(1,-1)\r\n" \
	L"     Cmd==7: activate console by number, Parm=(1-based console index)\r\n" \
	L"     Cmd==8: show tabs list menu (indiffirent Far/Not Far)\r\n" \
	L"     Cmd==9: close active tab, same as Close(3)\r\n" \
	L"     Cmd==10: switches visible split-panes, Parm=(1,-1)\r\n" \
	L"Task(Index[,\"Dir\"])\r\n" \
	L"  - start task with 1-based index\r\n" \
	L"Transparency(<Cmd>,<Value>)\r\n" \
	L"  - change ConEmu transparency level absolutely or relatively\r\n" \
	L"    Cmd==0, Value=40..255 (255==Opaque)\r\n" \
	L"    Cmd==1, Value=<relative inc/dec>\r\n" \
	L"Task(\"Name\"[,\"Dir\"])\r\n" \
	L"  - start task with specified name\r\n" \
	L"WindowFullscreen()\r\n" \
	L"  - Switch fullscreen window mode\r\n" \
	L"     returns previous window mode\r\n" \
	L"WindowMaximize([<Cmd>])\r\n" \
	L"  - Switch maximized window mode (Cmd==0)\r\n" \
	L"  - Maximize window by width (Cmd==1) or height (Cmd==2)\r\n" \
	L"     returns previous window mode\r\n" \
	L"WindowMinimize() or WindowMinimize(1)\r\n" \
	L"  - Minimize ConEmu window (\"1\" means \"minimize to the TSA\")\r\n" \
	L"     returns previous window mode\r\n" \
	L"WindowMode([\"<Mode>\"])\r\n" \
	L"  - Returns or set current window mode\r\n" \
	L"     \"NOR\", \"MAX\", \"FS\" (fullscreen), \"MIN\", \"TSA\",\r\n" \
    L"     \"TLEFT\", \"TRIGHT\" (tile to left/right), \"THEIGHT\",\r\n" \
    L"     \"MPREV\", \"MNEXT\" (move ConEmu to prev/next monitor)\r\n" \
	L""
