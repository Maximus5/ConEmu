
-- Place this file into your %FARPROFILE%\Macros\scripts


-- Show output of last console command
-- Checkbox 'Long console output' must be checked in
-- ConEmu Settings -> Features

-- OpenType=1 - Open in editor, OpenType=2 - Open in viewer
local OpenType = 1

local ConEmu   = "4b675d80-1d4a-4ea9-8436-fdc23f2fc14b"

Macro {
  area="Shell"; key="CtrlO"; flags=""; description="ConEmu: Show console output in editor"; action = function()
    Plugin.Call(ConEmu,OpenType) Keys("CtrlEnd")
  end;
}

--  Area="Editor.'*.tmp'"  !!!
--  Area="Viewer.'*.tmp'"  !!!

Macro {
  area="Editor Viewer"; key="CtrlO"; flags=""; description="ConEmu: Return to panels from console output"; action = function()

  if  Area.Editor then  f=Editor.FileName; else f=Viewer.FileName; end
  if  mf.lcase(mf.fsplit(f,8))~=".tmp"  or  mf.ucase(mf.substr(mf.fsplit(f,4),0,3))~="CEM" then 
    Keys("AKey")
    exit()
  end
  -- Was editor modified?
  m=0; if  Area.Editor then  if  band(Editor.State,0x40) then  m=1; end end
  -- Close
  Keys("Esc")
  -- Skip save confirmation
  if  m  and  Area.Dialog then  Keys("Right Enter") end

  end;
}

Macro {
  area="Shell"; key="CtrlShiftO"; flags=""; description="Hide/Show panels (standard FAR CtrlO)"; action = function()
    Keys("CtrlO")
  end;
}
