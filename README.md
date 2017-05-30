Download binary for 32 and 64bit Windows: (http://xumbu.org/downloads/opensource/prebuild/ConEmu_VimScrollPack.zip)


## About this Fork

If you use Vim in Windows cmd.exe and you have problems to scroll
by using the mouse wheel, you can try this ConEmu Fork.


You have to add following lines to your vimrc file:

```
if !has("gui_running")
    set mouse=a
    inoremap <F6> <C-x><C-Y>
    inoremap <F7> <C-x><C-e>
    nnoremap <F6> <C-Y>
    nnoremap <F7> <C-e>

    snoremap <F6> <C-Y>
    snoremap <F7> <C-e>
   
    vnoremap <F6> <C-Y> 
    vnoremap <F7> <C-e  
endif
```

Thats all.
Now you should be able to scroll inside Vim just like you do it in GVim.

________






## About ConEmu
[ConEmu-Maximus5](https://conemu.github.io) is a Windows console emulator with tabs, which represents
multiple consoles as one customizable GUI window with various features.

Initially, the program was created as a companion to
[Far Manager](http://en.wikipedia.org/wiki/FAR_Manager),
my favorite shell replacement - file and archive management,
command history and completion, powerful editor.

Today, ConEmu can be used with any other console application or simple GUI tools
(like PuTTY for example). ConEmu is an active project, open to
[suggestions](https://github.com/Maximus5/ConEmu/issues).

<a href="http://www.fosshub.com/ConEmu.html">![Fosshub.com ConEmu mirror](https://github.com/Maximus5/ConEmu/wiki/Downloads.png)</a>

Take a look at [screencast](http://dotnetsurfers.com/blog/2013/12/15/developer-tools-screencast-7-conemu/) about ConEmu.

This fork grew up from ConEmu by Zoin.


## License (BSD 3-clause)
    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.

See [Release/ConEmu/License.txt](https://github.com/Maximus5/ConEmu/blob/master/Release/ConEmu/License.txt) for the full license text.


## Some links
Wiki: https://conemu.github.io/en/TableOfContents.html  
What's new: https://conemu.github.io/en/Whats_New.html  
Release stages: https://conemu.github.io/en/StableVsPreview.html  
Donate this project: <a ref="https://conemu.github.io/donate.html" rel="nofollow">https://conemu.github.io/donate.html</a>



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
  * cursor: standard console (horizontal) or GUI (vertical);
  * and more, and more...

### Far Manager related features
  * tabs for editors, viewers, panels and consoles;
  * thumbnails and tiles;
  * show full output (1K+ lines) of last command in editor/viewer;
  * customizable right click behaviour (long click opens context menu);
  * drag and drop (explorer style);
  * and more, and more...

All settings are read from the registry or ConEmu.xml file, after which the
[command line parameters](https://conemu.github.io/en/CommandLine.html)
are applied. You may easily use several named configurations (for different PCs for example).


## Requirements
  * Windows 2000 or later.


## Installation
In general, ConEmu installation is easy.
Just unpack or install to any folder and run `ConEmu.exe`.

Read [Installation wiki](https://conemu.github.io/en/Installation.html)
about release stages, distro packets, some warnings and much more...


## Building from sources
https://github.com/Maximus5/ConEmu/blob/master/src/HowToBuild.md
 
## Screenshots
![Splits and tabs in ConEmu](https://github.com/Maximus5/ConEmu/wiki/ConEmuSplits.png)

![ConEmu+Powershell inside Windows Explorer pane](https://github.com/Maximus5/ConEmu/wiki/ConEmuInside.png)

[More screenshots](https://conemu.github.io/en/Screenshots.html)
