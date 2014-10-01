
-- Place this file into your %FARPROFILE%\Macros\scripts


-- Show all windows list from all consoles

local ConEmu   = "4b675d80-1d4a-4ea9-8436-fdc23f2fc14b"

Macro {
  area="Dialog Editor"; key="ShiftF11"; flags=""; description="ConEmu: Show all panels/dirs from all consoles"; action = function()
    Plugin.Call(ConEmu,11)
  end
}
