
/*
Copyright (c) 2011-2017 Maximus5
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

#define FarRClickMacroDefault2 L"@$If (!CmdLine.Empty) %Flg_Cmd=1; %CmdCurPos=CmdLine.ItemCount-CmdLine.CurPos+1; %CmdVal=CmdLine.Value; Esc $Else %Flg_Cmd=0; $End $Text \"rclk_gui:\" Enter $If (%Flg_Cmd==1) $Text %CmdVal %Flg_Cmd=0; %Num=%CmdCurPos; $While (%Num!=0) %Num=%Num-1; CtrlS $End $End"
#define FarRClickMacroDefault3 L"@if not CmdLine.Empty then Flg_Cmd=1; CmdCurPos=CmdLine.ItemCount-CmdLine.CurPos+1; CmdVal=CmdLine.Value; Keys(\"Esc\") else Flg_Cmd=0; end print(\"rclk_gui:\") Keys(\"Enter\") if Flg_Cmd==1 then print(CmdVal) Flg_Cmd=0; Num=CmdCurPos; while Num~=0 do Num=Num-1; Keys(\"CtrlS\") end end"


// Autosave modified editor
#define FarSafeCloseMacroDefault2 L"@$while (Dialog||Editor||Viewer||Menu||Disks||MainMenu||UserMenu||Other||Help) $if (Editor) ShiftF10 $if (Dialog) $Exit $end $else Esc $end $end  Esc  $if (Shell) F10 $if (Dialog) Enter $end $Exit $end  F10"
#define FarSafeCloseMacroDefault3 L"@while Area.Dialog or Area.Editor or Area.Viewer or Area.Menu or Area.Disks or Area.MainMenu or Area.UserMenu or Area.Other or Area.Help do if Area.Editor then Keys(\"ShiftF10\") if Area.Dialog then exit() end else Keys(\"Esc\") end end Keys(\"Esc\") if Area.Shell then Keys(\"F10\") if Area.Dialog then Keys(\"Enter\") end exit() end Keys(\"F10\")"
// Don't autosave modified editor
#define FarSafeCloseMacroDefaultD2 L"@$while (Dialog||Editor||Viewer||Menu||Disks||MainMenu||UserMenu||Other||Help) $if (Editor) F10 $if (Dialog) $Exit $end $else Esc $end $end  Esc  $if (Shell) F10 $if (Dialog) Enter $end $Exit $end  F10"
#define FarSafeCloseMacroDefaultD3 L"@while Area.Dialog or Area.Editor or Area.Viewer or Area.Menu or Area.Disks or Area.MainMenu or Area.UserMenu or Area.Other or Area.Help do if Area.Editor then Keys(\"F10\") if Area.Dialog then exit() end else Keys(\"Esc\") end end Keys(\"Esc\") if Area.Shell then Keys(\"F10\") if Area.Dialog then Keys(\"Enter\") end exit() end Keys(\"F10\")"


#define FarTabCloseMacroDefault2 L"@$if (Shell) F10 $if (Dialog) Enter $end $else F10 $end"
#define FarTabCloseMacroDefault3 L"@if Area.Shell then Keys(\"F10\") if Area.Dialog then Keys(\"Enter\") end else Keys(\"F10\") end"


#define FarSaveAllMacroDefault2 L"@F2 $If (!Editor) $Exit $End %i0=-1; F12 %cur = CurPos; Home Down %s = Menu.Select(\" * \",3,2); $While (%s > 0) $If (%s == %i0) MsgBox(\"FAR SaveAll\",\"Asterisk in menuitem for already processed window\",0x10001) $Exit $End Enter $If (Editor) F2 $If (!Editor) $Exit $End $Else $If (!Viewer) $Exit $End $End %i0 = %s; F12 %s = Menu.Select(\" * \",3,2); $End $If (Menu && Title==\"Screens\") Home $Rep (%cur-1) Down $End Enter $End $Exit"
#define FarSaveAllMacroDefault3 L"@Keys(\"F2\") if not Area.Editor then exit() end i0=-1; Keys(\"F12\") cur = Object.CurPos; Keys(\"Home Down\") s = Menu.Select(\" * \",3,2); while s > 0 do if s == i0 then msgbox(\"FAR SaveAll\",\"Asterisk in menuitem for already processed window\",0x10001) exit() end Keys(\"Enter\") if Area.Editor then Keys(\"F2\") if not Area.Editor then exit() end else if not Area.Viewer then exit() end end i0 = s; Keys(\"F12\") s = Menu.Select(\" * \",3,2); end if Area.Menu and Object.Title==\"Screens\" then Keys(\"Home\") for RCounter= cur-1,1,-1 do Keys(\"Down\") end Keys(\"Enter\") end exit()"
