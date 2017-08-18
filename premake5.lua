workspace "premake-CE"
  configurations { "Release", "Debug", "Remote" }
  platforms { "Win32", "x64" }
  location "build"
  --basedir "%{cfg.location}"
  startproject "ConEmu"
  flags { "StaticRuntime" }

  filter "platforms:Win32"
    architecture "x32"
    defines { "WIN32", "_WIN32" }

  filter "platforms:x64"
    architecture "x64"
    defines { "WIN64", "_WIN64" }

  filter "action:vs2017"
    toolset "v141_xp"
  filter "action:vs2015"
    toolset "v140_xp"
  filter "action:vs2013"
    toolset "v120_xp"
  filter "action:vs2012"
    toolset "v110_xp"

  filter "configurations:Release"
    defines { "NDEBUG", "HIDE_TODO" }
    optimize "Size"
    symbols "On"

  filter "configurations:Debug"
    defines { "_DEBUG", "HIDE_TODO", "MSGLOGGER" }
    symbols "On"

  filter "configurations:Remote"
    defines { "_DEBUG", "HIDE_TODO", "MSGLOGGER" }
    symbols "On"

  filter{}

  local ConEmuDir = "%{cfg.buildcfg}/"


local common_remove = {
  "**/ExecPty.*",
  "**/ProcList.*",
  "**/Processes.*",
  "**/base64.*",
  "**/*.bak",
  "**/!*.*",
  "**/_*.*",
}

local common_kernel = {
  "src/common/CEStr.*",
  "src/common/CmdLine.*",
  "src/common/Common.cpp",
  "src/common/Common.h",
  "src/common/ConEmuCheck.*",
  "src/common/ConsoleMixAttr.*",
  "src/common/ConsoleRead.*",
  "src/common/Execute.*",
  "src/common/HandleKeeper.*",
  "src/common/HkFunc.*",
  "src/common/InQueue.*",
  "src/common/MAssert.*",
  "src/common/MConHandle.*",
  "src/common/Memory.*",
  "src/common/MFileLog.*",
  "src/common/MModule.*",
  "src/common/MProcess.*",
  "src/common/MProcessBits.*",
  "src/common/MRect.*",
  "src/common/MSection.*",
  "src/common/MSectionSimple.*",
  "src/common/MSecurity.*",
  "src/common/MStrDup.*",
  "src/common/MStrSafe.*",
  "src/common/RConStartArgs.*",
  "src/common/WCodePage.*",
  "src/common/WConsole.*",
  "src/common/WModuleCheck.*",
  "src/common/WObjects.*",
  "src/common/WThreads.*",

}


-- ############################### --
-- ############################### --
-- ############################### --
project "common-kernel"
  kind "StaticLib"
  language "C++"
  exceptionhandling "Off"

  files (common_kernel)

  removefiles (common_remove)

  vpaths {
    ["Headers"] = {"**.h"},
    ["Sources"] = {"**.cpp"},
  }

  implibdir "lib1"
  objdir "obj1"
  -- doesn't work
  --targetdir "%{cfg.objdir}"
  filter "platforms:Win32"
    targetsuffix "32"
  filter "platforms:x64"
    targetsuffix "64"
  filter {}
-- end of "common-kernel"




-- ############################### --
-- ############################### --
-- ############################### --
project "common-user"
  kind "StaticLib"
  language "C++"
  --exceptionhandling "Off"

  files {
    "src/common/*.cpp",
    "src/common/*.h",
  }

  removefiles (common_remove)
  removefiles (common_kernel)

  vpaths {
    ["Headers"] = {"**.h"},
    ["Sources"] = {"**.cpp"},
  }

  implibdir "lib1"
  -- doesn't work
  --targetdir "%{cfg.objdir}"
  filter "platforms:Win32"
    targetsuffix "32"
  filter "platforms:x64"
    targetsuffix "64"
  filter {}
-- end of "common-user"




-- ############################### --
-- ############################### --
-- ############################### --
project "ConEmu"
  kind "WindowedApp"
  language "C++"
  --exceptionhandling "Off"
  flags { "WinMain" }

  links {
    "common-kernel",
    "common-user",
    "comctl32",
    "shlwapi",
    "version",
    "gdiplus",
    "winmm",
    "netapi32",
  }

  -- we can't use pch because SHOWDEBUGSTR is defined or not per source file
  -- pchheader "Header.h"
  -- pchsource "src/ConEmu/Header.cpp"

  files {
    "src/common/*.h",
    "src/ConEmu/*.cpp",
    "src/ConEmu/*.h",
    "src/ConEmu/*.rc",
    "src/ConEmu/*.rc2",
    "src/ConEmu/*.bmp",
    "src/ConEmu/*.cur",
    "src/ConEmu/*.ico",
    "src/ConEmu/conemu.gcc.manifest",
  }
  removefiles {
    "**/ConEmuMinGW.rc",
    "**/ToolBarClass.*",
    "**/Theming.*",
    "**/FontRanges.*",
    "**/*-old.*",
    "**/RegExp.*",
    "**/!*.*",
  }

  vpaths {
    { ["Common"]    = {"src/common/*.h"} },
    { ["Resources"] = {"**/*.rc", "**/*.rc2", "**/*.manifest", "**/*.bmp", "**/*.cur", "**/*.ico"} },
    { ["Exports"]   = {"**.def"} },
    { ["Settings"]  = {"**/SetPg*.*"} },
    { ["Headers"]   = {"**/*.h"} },
    { ["Sources"]   = {"**/*.cpp"} },
  }

  targetdir (ConEmuDir)
  targetname "ConEmu"
  filter "platforms:x64"
    targetsuffix "64"
  filter {}
-- end of "ConEmu"




-- ############################### --
-- ############################### --
-- ############################### --
project "ConEmuC"
  kind "ConsoleApp"
  language "C++"
  exceptionhandling "Off"

  links {
    "common-kernel",
  }

  dependson {
    "ConEmuCD",
    "ConEmuHk",
  }

  files {
    "src/ConEmuC/*.cpp",
    "src/ConEmuC/*.h",
    "src/ConEmuC/*.rc",
    "src/ConEmuC/ConEmuC.exe.manifest",
  }

  vpaths {
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc", "**.rc2", "**.manifest"} },
    { ["Exports"]   = {"**.def"} },
  }

  targetdir (ConEmuDir.."ConEmu/")
  targetname "ConEmuC"
  filter "platforms:Win32"
    targetsuffix ""
  filter "platforms:x64"
    targetsuffix "64"
  filter {}
-- end of "ConEmuC"




-- ############################### --
-- ############################### --
-- ############################### --
project "ConEmuCD"
  kind "SharedLib"
  language "C++"
  --exceptionhandling "Off"

  configuration { "vs*" }
    linkoptions { "/DYNAMICBASE:NO", "/FIXED:NO", "/BASE:0x6F780000" }
  configuration { "gmake" }
    linkoptions { "--image-base=0x6F780000" }
  configuration {}

  links {
    "common-kernel",
    "common-user",
  }

  files {
    "src/common/Common.h",
    "src/ConEmuCD/*.cpp",
    "src/ConEmuCD/*.h",
    "src/ConEmuHk/Injects.*",
    "src/ConEmuCD/*.rc",
  }

  filter "action:vs*"
    files "src/ConEmuCD/export.def"
  filter "action:gmake"
    files "src/ConEmuCD/export.gcc.def"
  filter {}

  vpaths {
    { ["Interface"] = {"**/Common.h", "**/SrvCommands.*", "**/Queue.*", "**/SrvPipes.*"} },
    { ["Automation"] = {"**/Actions.*", "**/GuiMacro.*"} },
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc", "**.rc2", "**.manifest"} },
    { ["Exports"]   = {"**.def"} },
  }

  implibdir "lib1"
  targetdir (ConEmuDir.."ConEmu/")
  targetname "ConEmuCD"
  filter "platforms:Win32"
    targetsuffix ""
  filter "platforms:x64"
    targetsuffix "64"
  filter {}
-- end of "ConEmuCD"




-- ############################### --
-- ############################### --
-- ############################### --
project "ConEmuHk"
  kind "SharedLib"
  language "C++"
  exceptionhandling "Off"

  configuration { "vs*" }
    linkoptions { "/DYNAMICBASE:NO", "/FIXED:NO", "/BASE:0x7E110000" }
  configuration { "gmake" }
    linkoptions { "--image-base=0x7E110000" }
  configuration {}

  --filter { "configurations:Release" }
  --  optimize "Full"
  --filter {}

  defines { "CONEMU_MINIMAL" }

  links {
    "common-kernel",
  }

  files {
    "src/ConEmuHk/*.cpp",
    "src/ConEmuHk/*.h",
    "src/ConEmuHk/*.rc",
    "src/ConEmuHk/hde.c",
    "src/modules/minhook/src/**.c",
    "src/modules/minhook/src/**.h",
    "src/modules/minhook/include/*.h",
    "src/modules/minhook/LICENSE.txt",
  }

  removefiles {
    "**/CETaskBar_.*",
    "**/*-orig.*",
  }

  filter { "files:**/HDE/*.*" }
    flags {"ExcludeFromBuild"}
  flags {}

  filter "action:vs*"
    files "src/ConEmuHk/export.def"
  filter "action:gmake"
    files "src/ConEmuHk/export.gcc.def"
  filter {}

  vpaths {
    { ["MinHook/HDE"] = {"**/minhook/src/HDE/*.*"} },
    { ["MinHook/Trampoline"] = {"**/minhook/src/*.*"} },
    { ["MinHook"] = {"**/minhook/**.*", "**/hde.c"} },
    { ["Hooks"] = {"**/hk*.*"} },
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc", "**.rc2", "**.manifest"} },
    { ["Exports"]   = {"**.def"} },
  }

  implibdir "lib1"
  targetdir (ConEmuDir.."ConEmu/")
  targetname "ConEmuHk"
  filter "platforms:Win32"
    targetsuffix ""
  filter "platforms:x64"
    targetsuffix "64"
  filter {}
-- end of "ConEmuHk"




-- ############################### --
-- ############################### --
-- ############################### --
project "Far.ConEmuPlugin"
  kind "SharedLib"
  language "C++"
  --exceptionhandling "Off"

  links {
    "common-kernel",
    "common-user",
    "version",
  }

  files {
    "src/ConEmuPlugin/*.cpp",
    "src/ConEmuPlugin/*.h",
    "src/ConEmuPlugin/*.rc",
    "src/ConEmuPlugin/Lang.templ",
  }

  filter "action:vs*"
    files "src/ConEmuPlugin/export.def"
  filter "action:gmake"
    files "src/ConEmuPlugin/export.gcc.def"
  filter {}

  vpaths {
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc", "**.rc2", "**.manifest", "**.templ"} },
    { ["Exports"]   = {"**.def"} },
  }

  implibdir "lib1"
  targetdir (ConEmuDir.."plugins/ConEmu/")
  targetname "ConEmu"
  filter "platforms:Win32"
    targetsuffix ""
  filter "platforms:x64"
    targetsuffix ".x64"
  filter {}
-- end of "Far.ConEmuPlugin"




-- ############################### --
-- ############################### --
-- ############################### --
project "Far.ConEmuBg"
  kind "SharedLib"
  language "C++"
  --exceptionhandling "Off"

  links {
    "common-kernel",
    "common-user",
    "version",
  }

  files {
    "src/ConEmuBg/*.cpp",
    "src/ConEmuBg/*.h",
    "src/ConEmuBg/*.rc",
    "src/ConEmuBg/Lang.templ",
  }

  filter "action:vs*"
    files "src/ConEmuBg/export.def"
  filter "action:gmake"
    files "src/ConEmuBg/export.gcc.def"
  filter {}

  vpaths {
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc", "**.rc2", "**.manifest", "**.templ"} },
    { ["Exports"]   = {"**.def"} },
  }

  implibdir "lib1"
  targetdir (ConEmuDir.."plugins/ConEmu/Background/")
  targetname "ConEmuBg"
  filter "platforms:Win32"
    targetsuffix ""
  filter "platforms:x64"
    targetsuffix ".x64"
  filter {}
-- end of "Far.ConEmuBg"




-- ############################### --
-- ############################### --
-- ############################### --
project "Far.ConEmuLn"
  kind "SharedLib"
  language "C++"
  --exceptionhandling "Off"

  links {
    "common-kernel",
    "common-user",
    "version",
  }

  files {
    "src/ConEmuLn/*.cpp",
    "src/ConEmuLn/*.h",
    "src/ConEmuLn/*.rc",
    "src/ConEmuLn/Lang.templ",
  }

  filter "action:vs*"
    files "src/ConEmuLn/export.def"
  filter "action:gmake"
    files "src/ConEmuLn/export.gcc.def"
  filter {}

  vpaths {
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc", "**.rc2", "**.manifest", "**.templ"} },
    { ["Exports"]   = {"**.def"} },
  }

  implibdir "lib1"
  targetdir (ConEmuDir.."plugins/ConEmu/Lines/")
  targetname "ConEmuLn"
  filter "platforms:Win32"
    targetsuffix ""
  filter "platforms:x64"
    targetsuffix ".x64"
  filter {}
-- end of "Far.ConEmuLn"




-- ############################### --
-- ############################### --
-- ############################### --
project "Far.ConEmuTh"
  kind "SharedLib"
  language "C++"
  --exceptionhandling "Off"

  links {
    "common-kernel",
    "common-user",
    "version",
  }

  files {
    "src/ConEmuTh/*.cpp",
    "src/ConEmuTh/*.h",
    "src/ConEmuTh/*.rc",
    "src/ConEmuTh/Lang.templ",
  }

  filter "action:vs*"
    files "src/ConEmuTh/export.def"
  filter "action:gmake"
    files "src/ConEmuTh/export.gcc.def"
  filter {}

  vpaths {
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc", "**.rc2", "**.manifest", "**.templ", "**.templ"} },
    { ["Exports"]   = {"**.def"} },
  }

  implibdir "lib1"
  targetdir (ConEmuDir.."plugins/ConEmu/Thumbs/")
  targetname "ConEmuTh"
  filter "platforms:Win32"
    targetsuffix ""
  filter "platforms:x64"
    targetsuffix ".x64"
  filter {}
-- end of "Far.ConEmuTh"




-- ############################### --
-- ############################### --
-- ############################### --
project "Far.ConEmuTh.gdi+"
  kind "SharedLib"
  language "C++"
  exceptionhandling "Off"

  links {
    "common-kernel",
  }

  files {
    "src/ConEmuTh/Modules/gdip/*.cpp",
    "src/ConEmuTh/Modules/gdip/*.h",
    "src/ConEmuTh/Modules/gdip/*.rc",
    "src/ConEmuTh/Modules/gdip/gdip.def",
    "src/ConEmuTh/Modules/ThumbSDK.h",
    "src/ConEmuTh/Modules/MStream.h",
  }

  vpaths {
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc", "**.rc2", "**.manifest"} },
    { ["Exports"]   = {"**.def"} },
  }

  implibdir "lib1"
  targetdir (ConEmuDir.."plugins/ConEmu/Thumbs/")
  targetname "gdi+"
  filter "platforms:Win32"
    targetextension ".t32"
  filter "platforms:x64"
    targetextension ".t64"
  filter {}
-- end of "Far.ConEmuTh.gdi+"




-- ############################### --
-- ############################### --
-- ############################### --
project "Far.ConEmuTh.ico"
  kind "SharedLib"
  language "C++"
  exceptionhandling "Off"

  links {
    "common-kernel",
  }

  files {
    "src/ConEmuTh/Modules/ico/*.cpp",
    "src/ConEmuTh/Modules/ico/*.h",
    "src/ConEmuTh/Modules/ico/*.rc",
    "src/ConEmuTh/Modules/ico/ico.def",
    "src/ConEmuTh/Modules/ThumbSDK.h",
    "src/ConEmuTh/Modules/MStream.h",
  }

  removefiles {
    "**/!*.*",
  }

  vpaths {
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc", "**.rc2", "**.manifest"} },
    { ["Exports"]   = {"**.def"} },
  }

  implibdir "lib1"
  targetdir (ConEmuDir.."plugins/ConEmu/Thumbs/")
  targetname "ico"
  filter "platforms:Win32"
    targetextension ".t32"
  filter "platforms:x64"
    targetextension ".t64"
  filter {}
-- end of "Far.ConEmuTh.ico"




-- ############################### --
-- ############################### --
-- ############################### --
project "Far.ConEmuTh.pe"
  kind "SharedLib"
  language "C++"
  exceptionhandling "Off"

  links {
    "common-kernel",
  }

  files {
    "src/ConEmuTh/Modules/pe/*.cpp",
    "src/ConEmuTh/Modules/pe/*.h",
    "src/ConEmuTh/Modules/pe/*.rc",
    "src/ConEmuTh/Modules/pe/pe.def",
    "src/ConEmuTh/Modules/ThumbSDK.h",
  }

  vpaths {
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc", "**.rc2", "**.manifest"} },
    { ["Exports"]   = {"**.def"} },
  }

  implibdir "lib1"
  targetdir (ConEmuDir.."plugins/ConEmu/Thumbs/")
  targetname "pe"
  filter "platforms:Win32"
    targetextension ".t32"
  filter "platforms:x64"
    targetextension ".t64"
  filter {}
-- end of "Far.ConEmuTh.pe"




-- ############################### --
-- ############################### --
-- ############################### --
project "Far.ExtendedConsole"
  kind "SharedLib"
  language "C++"
  exceptionhandling "Off"

  links {
    "common-kernel",
  }

  files {
    "src/ConEmuDW/*.cpp",
    "src/ConEmuDW/*.h",
    "src/ConEmuDW/*.rc",
  }

  removefiles {
    "**/!*.*",
  }

  filter "action:vs*"
    files "src/ConEmuDW/export.def"
  filter "action:gmake"
    files "src/ConEmuDW/export.gcc.def"
  filter {}

  vpaths {
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc", "**.rc2", "**.manifest"} },
    { ["Exports"]   = {"**.def"} },
  }

  implibdir "lib1"
  targetdir (ConEmuDir.."ConEmu/")
  targetname "ExtendedConsole"
  filter "platforms:Win32"
    targetsuffix ""
  filter "platforms:x64"
    targetsuffix "64"
  filter {}
-- end of "Far.ExtendedConsole"
