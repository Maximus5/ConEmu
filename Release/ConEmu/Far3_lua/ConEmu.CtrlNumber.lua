
-- Place this file into your %FARPROFILE%\Macros\scripts

-- When tab switching in ConEmu is configured to Ctrl+Number
-- this macro will help to switch Far Manager panel modes
-- LCtrl-Shift-1 to ‘Brief’, LCtrl-Shift-2 to ‘Medium’, etc.

Macro
{
  area="Common";
  key="/LCtrlShift[0-9]/";
  description="Far: Map all LCtrl+Shift+Digit to LCtrl+Digit";
action = function()
  -- BUG? Following is expected to be working, but not... Far Panels ignores it
  -- Keys("LCtrl"..akey(1):sub(-1))
  Keys("Ctrl"..akey(1):sub(-1))
end
}
