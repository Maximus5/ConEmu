
-- Place this file into your %FARPROFILE%\Macros\scripts


-- Several Far instances may be opened in ConEmu tabs.
-- This macro activates existing Editor/Viewer, wich
-- may be opened in then inactive instance of Far.

local ConEmu   = "4b675d80-1d4a-4ea9-8436-fdc23f2fc14b"

Macro {
  area="Shell Search"; key="F3 F4"; flags="NoPluginPanels"; description="ConEmu: Auto switch to Editor Tab"; action = function()

  k = akey(1,1);

  if   not APanel.Empty  and  (APanel.Current~="..") then 
    s=APanel.Path;
    s=s..mf.iif(mf.substr(s,mf.len(s)-1)=="\\","","\\")..APanel.Current;
    -- msgbox("Current item", %s)
    c = mf.substr(k,mf.len(k)-1,1);
    cmd = mf.iif(c=="4","FindEditor:","FindViewer:")..s;
    iRc=Plugin.Call(ConEmu,cmd);
    -- msgbox("EditOpen result", "callplugin="+%iRc+"\n<"+env("ConEmuMacroResult")+">")
    if  iRc~=0 then 
      r = mf.env("ConEmuMacroResult");
      if  mf.substr(r,0,5)=="Found" then 
        exit() -- OK
      else if  mf.substr(r,0,6)=="Active" then
        Keys("F12 "..mf.substr(r,7))
        exit() -- Ok
      else if  r=="Blocked" then 
        -- msgbox("ConEmu macro","Tab can't be activated now\n"+%s);
      end end end
    end
  end

  -- history.enable(0xF)
  if  k=="F4" then  Keys("F4") else Keys("F3") end

  -- As we get here, Far will try to open viewer or editor window.
  -- But, it may be already opened in the current Far instance.
  -- When it's exists, Far show messagebox with confirmation and buttons:
  -- { Current } [ New instance ] [ Reload ]
  -- Next macro line depress { Current } button
  if  Area.Dialog  and  Object.Title=="Editor"  and  Dlg.ItemCount==7  and  Dlg.CurPos==5  and  Dlg.ItemType==7 then  Keys("Enter") end

  end;
}
