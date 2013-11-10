## About ConEmu
ConEmu-Maximus5 is a Windows console emulator with tabs, which represents
multiple consoles as one customizable GUI window with various features.

Initially, the program was created as a companion to
[Far Manager](http://en.wikipedia.org/wiki/FAR_Manager),
my favorite shell replacement - file and archive management,
command history and completion, powerful editor.

Today, ConEmu can be used with any other console application or simple GUI tools
(like PuTTY for example). ConEmu is an active project, open to
[suggestions](http://code.google.com/p/conemu-maximus5/issues/list).

Download: http://www.fosshub.com/ConEmu.html  
Source code (svn): http://conemu-maximus5.googlecode.com/svn/trunk/

This fork grew up from ConEmu by Zoin.


## Description
ConEmu starts a console program in hidden console window and provides
an alternative customizable GUI window with various features:

  * smooth window resizing;
  * tabs and splits (panes);
  * easy run old DOS applications (games) in Windows 7 or 64bit OS (DosBox required);
  * quake-style, normal, maximized and full screen window graphic modes;
  * window font anti-aliasing: standard, clear type, disabled;
  * window fonts: family, height, width, bold, italic, etc.;
  * using normal/bold/italic fonts for different parts of console simultaneously;
  * cursor: standard console (horisontal) or GUI (vertical);
  * and more, and more...

### Far Manager related features
  * tabs for editors, viewers, panels and consoles;
  * thumbnails and tiles;
  * show full output (1K+ lines) of last command in editor/viewer;
  * customizable right click behaviour (long click opens context menu);
  * drag and drop (explorer style);
  * and more, and more...

All settings are read from the registry or ConEmu.xml file, after which the
[command line parameters](http://code.google.com/p/conemu-maximus5/wiki/Command_Line)
are applied. You may easily use several named configurations (for different PCs for example).


## Requirements
  * Windows 2000 or later.


## Documentation
Wiki: http://code.google.com/p/conemu-maximus5/wiki/ConEmu  
What's new: http://code.google.com/p/conemu-maximus5/wiki/Whats_New


## Installation

### If you are NOT a Far Manager user
*. Unpack all files (from appropriate "ConEmuPack.*.7z") to any folder, or install "ConEmuSetup.*.exe" package. Subfolder "plugins" (Far Manager related) is not required in your case.
*. Run ConEmu.exe or ConEmu64.exe.

### If you ARE a Far Manager user
*. Unpack all files (from appropriate "ConEmuPack.*.7z") to the folder, containing
     "far.exe", or install "ConEmuSetup.*.exe" package.
     * Note, if You are not using FAR manager, You may unpack files to any folder.
*. Import to the registry Far Manager macroses, related to ConEmu. Macro `*.reg`
     files are located in `ConEmu.Addons` directory. Each macro file (`*.reg`) has
     description in header. Just doubleclick choosen files in Windows Explorer
     to import them.
*. By default (started without command line params), ConEmu runs "far.exe" from
     it's home folder, or "cmd.exe" if Far Manager not found.
     Alternatively, You may run any root command, specifying "/Cmd <App with params>"
     argument in ConEmu shortcut or command line.

 
## Screenshots
Splits and tabs in ConEmu
http://conemu-maximus5.googlecode.com/svn/files/ConEmuSplits.png

ConEmu Thumbnails & Tiles + FAR 2.0
http://conemu-maximus5.googlecode.com/svn/files/ConEmuPanelViews.png

FAR 3.0 + ConEmu (simple console mode)
http://conemu-maximus5.googlecode.com/svn/files/ConEmuVanilla.png

ConEmu + FAR 2.0*, using anti-aliased font
http://conemu-maximus5.googlecode.com/svn/files/ConEmu.png

ConEmu + FAR 2.0 Editor*, anti-aliasing disabled
http://conemu-maximus5.googlecode.com/svn/files/ConEmuItalic.png
