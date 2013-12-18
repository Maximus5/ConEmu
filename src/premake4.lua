-- Microsoft nmake:
--    T:\VCProject\Externals\Premake\src\bin\debug\premake4.exe /scripts=T:\VCProject\Externals\Premake\src\src nmake

local to = ".build/"..(_ACTION or "default")

--------------------------------------------------------------------------------
local function setup_cfg(cfg)
    configuration(cfg)
        targetdir(to.."/bin/"..cfg)
        objdir(to.."/obj/"..cfg)

--    configuration({cfg, "x32"})
--        targetsuffix("_x86")
--        defines("PLATFORM=x86")

--    configuration({cfg, "x64"})
--        targetsuffix("_x64")
--        defines("PLATFORM=x64")
end

--------------------------------------------------------------------------------

solution ("ConEmu")
    platforms({"x32","x64"})

	configuration "vs*"
		configurations({"Release","Debug","Debug_C","Debug_F3","Debug_R"})
	    setup_cfg("Release")
	    setup_cfg("Debug")
	    setup_cfg("Debug_C")
	    setup_cfg("Debug_F3")
	    setup_cfg("Debug_R")

	configuration "gmake"
		configurations({"release","debug"})
	    setup_cfg("release")
	    setup_cfg("debug")


    location(to)


    flags("Symbols")
    flags("StaticRuntime")

    configuration("release")
        flags("OptimizeSpeed")

    configuration("x32")
        defines("WIN32")

    configuration("x64")
        defines("WIN64")

--------------------------------------------------------------------------------
project("ConEmu")
    language("c++")
    kind("WindowedApp")
    flags("WinMain")

    files("ConEmu.rc")

    files({
        "ConEmu/Attach.cpp",
        "ConEmu/Background.cpp",
        "ConEmu/BaseDragDrops.cpp",
        "ConEmu/ConEmu.cpp",
        "ConEmu/ConEmuApp.cpp",
        "ConEmu/ConEmuCtrl.cpp",
        "ConEmu/ConEmuPipe.cpp",
        "ConEmu/ConfirmDlg.cpp",
        "ConEmu/CustomFonts.cpp",
        "ConEmu/DefaultTerm.cpp",
        "ConEmu/DragDrop.cpp",
        "ConEmu/DragDropData.cpp",
        "ConEmu/DwmHelper.cpp",
        "ConEmu/FindDlg.cpp",
        "ConEmu/FrameHolder.cpp",
        "ConEmu/GestureEngine.cpp",
        "ConEmu/GuiServer.cpp",
        "ConEmu/Hotkeys.cpp",
        "ConEmu/IconList.cpp",
        "ConEmu/Inside.cpp",
        "ConEmu/LoadImg.cpp",
        "ConEmu/Macro.cpp",
        "ConEmu/Menu.cpp",
        "ConEmu/Options.cpp",
        "ConEmu/OptionsClass.cpp",
        "ConEmu/OptionsFast.cpp",
        "ConEmu/OptionsHelp.cpp",
        "ConEmu/RealBuffer.cpp",
        "ConEmu/RealConsole.cpp",
        "ConEmu/RealServer.cpp",
        "ConEmu/RunQueue.cpp",
        "ConEmu/Recreate.cpp",
        "ConEmu/Registry.cpp",
        "ConEmu/ScreenDump.cpp",
        "ConEmu/Status.cpp",
        "ConEmu/TabBar.cpp",
        "ConEmu/TabID.cpp",
        "ConEmu/TaskBar.cpp",
        "ConEmu/TaskBarGhost.cpp",
        "ConEmu/TrayIcon.cpp",
        "ConEmu/Update.cpp",
        "ConEmu/UpdateSet.cpp",
        "ConEmu/VConChild.cpp",
        "ConEmu/VConGroup.cpp",
        "ConEmu/VConRelease.cpp",
        "ConEmu/VirtualConsole.cpp"})
    
    files({
        "common/CmdLine.cpp",
        "common/Common.cpp",
        "common/ConEmuCheck.cpp",
        "common/execute.cpp",
        "common/Memory.cpp",
        "common/Monitors.cpp",
        "common/MSecurity.cpp",
        "common/MStrSafe.cpp",
        "common/RConStartArgs.cpp",
        "common/RgnDetect.cpp",
        "common/SetEnvVar.cpp",
        "common/WinConsole.cpp",
        "common/WinObjects.cpp"})

--------------------------------------------------------------------------------
project("ConEmuC")
    language("c++")
    kind("ConsoleApp")
    targetname("ConEmuC")

    files({
        "ConEmuC/ConEmuC.cpp",
        "ConEmuC/ConEmuC.rc"})

--------------------------------------------------------------------------------
project("ConEmuCD")
    language("c++")
    kind("SharedLib")
    targetname("ConEmuCD")

    files({
    	"ConEmuCD/*.cpp","ConEmuCD/*.h"
    	})
    files({
    	"ConEmuCD/ConEmuCD.rc","ConEmu/afxres.h","ConEmu/version.h"
    	})
	files({
		"ConEmuHk/Injects.cpp",
		"common/CmdLine.cpp",
		"common/Common.cpp",
		"common/ConEmuCheck.cpp",
		"common/ConsoleRead.cpp",
		"common/ExecPty.cpp",
		"common/Execute.cpp",
		"common/InQueue.cpp",
		"common/MAssert.cpp",
		"common/Monitors.cpp",
		"common/MSecurity.cpp",
		"common/MStrSafe.cpp",
		"common/Memory.cpp",
		"common/RConStartArgs.cpp",
		"common/SetEnvVar.cpp",
		"common/WinConsole.cpp",
		"common/WinObjects.cpp"
		})

    excludes({
    	"ConEmuCD/CETaskBar.*"
    	})
    excludes({
    	"ConEmuCD/header.h",
    	"ConEmuCD/RE_*.*",
    	"ConEmuCD/REG_*.*",
    	"ConEmuCD/RegEditor_Lang.h",
    	"ConEmuCD/RegHooks.h"
    	})

--------------------------------------------------------------------------------
project("ConEmuHk")
    language("c++")
    kind("SharedLib")
    targetname("ConEmuHk")

    files({
		"ConEmuHk/Entry.*",
		"ConEmuHk/Ansi.*",
		"ConEmuHk/ConEmuHooks.*",
		"ConEmuHk/ExtConsole.*",
		"ConEmuHk/Injects.*",
		"ConEmuHk/RegHooks.*",
		"ConEmuHk/SetHook.*",
		"ConEmuHk/ShellProcessor.*",
		"ConEmuHk/GuiAttach.*",
		"ConEmuHk/UserImp.*"
		})
	files({
		"common/CmdLine.*",
		"common/Common.*",
		"common/ConEmuCheck.*",
		"common/ConsoleMixAttr.*",
		"common/Execute.*",
		"common/InQueue.*",
		"common/MAssert.*",
		"common/MSecurity.*",
		"common/MStrSafe.*",
		"common/Memory.*",
		"common/RConStartArgs.*",
		"common/WinConsole.*",
		"common/WinObjects.*"
		})
