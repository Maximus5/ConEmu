## About this Fork

If you use Vim (command line version) in Windows and you have problems to scroll
by using the mouse wheel, you can try this ConEmu Fork.


You have to add following to your vimrc file:

```
if !has("gui_running")
    "set mouse=a
    inoremap <F6> <C-x><C-Y>
    inoremap <F7> <C-x><C-e>
    nnoremap <F6> <C-Y>
    nnoremap <F7> <C-e>
endif
```

Thats all. 
Now you should be able to scroll inside Vim just like you do it in GVim.














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

<a href="http://www.fosshub.com/ConEmu.html">![Fosshub.com ConEmu mirror](https://github.com/Maximus5/ConEmu/wiki/Downloads.png)</a>

Take a look at [screencast](http://dotnetsurfers.com/blog/2013/12/15/developer-tools-screencast-7-conemu/) about ConEmu.

This fork grew up from ConEmu by Zoin.

### Some links
Release stages: http://code.google.com/p/conemu-maximus5/wiki/StableVsPreview  
Wiki: http://code.google.com/p/conemu-maximus5/wiki/ConEmu  
What's new: http://code.google.com/p/conemu-maximus5/wiki/Whats_New  
Donate this project: http://conemu.github.io/donate.html



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


## Installation

### If you are NOT a Far Manager user
* Unpack all files (from appropriate `ConEmuPack.\*.7z`)
	or install `ConEmuSetup.\*.exe` package to any folder your choice.
 	Subfolder `plugins` (Far Manager related) is not required in your case.
*  Run ConEmu.exe or ConEmu64.exe.

### If you ARE a Far Manager user
* Use of `ConEmuPack.\*.7z` and `ConEmuSetup.\*.exe` are slightly different
  * `ConEmuPack.\*.7z`: Unpack all files to the folder, containing `far.exe`
  * `ConEmuSetup.\*.exe`: On the `Features` page you must select destination
	for `Far Manager plugins` to the folder, containing `far.exe`.
* Import to the registry Far Manager macroses, related to ConEmu. Macro `*.reg`
	files are located in `ConEmu.Addons` directory. Each macro file (`*.reg`) has
	description in header. Just doubleclick chosen files in Windows Explorer
	to import them.
* By default (started without command line params), ConEmu runs `far.exe` from
	it's home folder, or `cmd.exe` if Far Manager not found.
	Alternatively, You may run any root command, specifying `/Cmd \<App with params\>`
	argument in ConEmu shortcut or command line.

## Building from sources
https://github.com/Maximus5/ConEmu/blob/master/src/HowToBuild.txt

 
## Screenshots
![Splits and tabs in ConEmu](https://github.com/Maximus5/ConEmu/wiki/ConEmuSplits.png)

![ConEmu+Powershell inside Windows Explorer pane](https://github.com/Maximus5/ConEmu/wiki/ConEmuInside.png)

[More screenshots](http://code.google.com/p/conemu-maximus5/wiki/Screenshots)
