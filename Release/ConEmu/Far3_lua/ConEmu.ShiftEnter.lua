
-- Place this file into your %FARPROFILE%\Macros\scripts


-- Run 'File under cursor' or 'Command line' in new console of ConEmu.
-- 'Press enter to close console' will be displayed after command completion.
-- Note! If you want to disable this confirmation,
--       set 'DisableCloseConfirm = 1'

local DisableCloseConfirm = 0

-- AltEnter   - run command and activate new ConEmu tab
-- Note! You must enable "Alt+Enter" option in ConEmu Settings->Keys.

-- ShiftEnter - run command in background ConEmu tab
-- Note! If you want to activate new tab on ShiftEnter,
--       set 'UseBackgroundTab = 0'

local UseBackgroundTab = 1


-- While starting command in background tab, there is a flicker on panels.
-- If you want to disable flicker, set 'DisableFlicker = 1'.
-- Note! ConEmu plugin reqired for 'DisableFlicker = 1'.

local DisableFlicker = 0



--  Descr="Run <File under cursor> or <Command line> in new console of ConEmu" 
--  Key="ShiftEnter AltEnter"
--  Area="Shell Search ShellACompl"

Macro {
  area="Shell Search ShellAutoCompletion"; key="ShiftEnter AltEnter"; flags=""; description="ConEmu: Run <File under cursor> or <Command line> in new console of ConEmu"; action = function()

  -- history.enable(0xff)

  if  akey(1,1)=="ShiftEnter"  and  UseBackgroundTab then 
    add = " -new_console:b";
    if  DisableCloseConfirm then  add = add .. "n"; end
  else
    -- AltEnter creates foreground console(tab)
    add = " -new_console";
    if  DisableCloseConfirm then  add = add .. ":n"; end
  end

  oldcmd = "";
  if  Area.ShellAutoCompletion then
    Keys("Esc") -- close autocompletion
  else
    if  Area.Search then 
      -- Save and clear command line - about to execute panel(!) item
      oldcmd = CmdLine.Value; oldpos = CmdLine.CurPos;
      Keys("Esc Esc")  -- First - close search, second - clear command line
    end
  end
  
  if   not CmdLine.Empty then 
    if  Area.Current=="Shell.AutoCompletion" then 
      Keys("Esc") -- Close autocompletion
    end
    
    if  CmdLine.Value=="."  or  CmdLine.Value==".."  or  CmdLine.Value=="..." then 
      Keys("ShiftEnter")
      exit()
    else
      if  Area.Current=="Shell.AutoCompletion" then 
        Keys("Del") -- Remove autocompletion selection
      end
      -- CtrlEnd - fails, couse of AutoCompletion
      for RCounter=CmdLine.ItemCount,1,-1 do  Keys("CtrlD") end
      -- Append "-new_console" if not exists
      if  mf.index(CmdLine.Value,"-new_console")<0 then  print(add) end
    end
  else if  APanel.FilePanel  and   not APanel.Plugin  and   not APanel.Empty  and   not APanel.Folder  and  mf.len(APanel.Current)>4 then 
    exec = mf.ucase(mf.env("PATHEXT"));
    if  exec=="" then  exec = ".COM;.EXE;.BAT;.CMD"; end
    ext = mf.ucase(mf.fsplit(APanel.Current,8));
    if  ext~=""  and  mf.index(";"..exec..";",";"..ext..";")>=0 then 
      Keys("CtrlEnter")
      Keys("Del") -- Remove possible autocompletion selection
      -- Append "-new_console"
      print(add)
    else
      if  akey(1,1)=="ShiftEnter" then  Keys("ShiftEnter") end
      exit()
    end
  else
    if  akey(1,1)=="ShiftEnter" then  Keys("ShiftEnter") end
    exit()
  end end

  Keys("Del") -- Remove possible autocompletion selection
  
  if  DisableFlicker then 
    for RCounter=CmdLine.ItemCount,1,-1 do  Keys("CtrlS") end
    print("ConEmu:run:")
  end

  Keys("Enter") -- Execute
  
  -- Restore old command line state (running file from Panel in QSearch mode)
  if  oldcmd ~= "" then 
    print(oldcmd)
    if  oldpos>=1  and  oldpos<=CmdLine.ItemCount then  Keys("CtrlHome") for RCounter=oldpos-1,1,-1 do  Keys("CtrlD") end end
  end

  end
}
