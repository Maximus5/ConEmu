workspace "premake-CE"
  configurations { "Release", "Debug", "Remote" }
  platforms { "Win32", "x64" }
  location "build"
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
    optimize "On"

  filter "configurations:Debug"
    defines { "_DEBUG", "HIDE_TODO", "MSGLOGGER" }
    symbols "On"

  filter "configurations:Remote"
    defines { "_DEBUG", "HIDE_TODO", "MSGLOGGER" }
    symbols "On"

  filter{}





-- ############################### --
-- ############################### --
-- ############################### --
project "common"
  kind "StaticLib"
  language "C++"

  files {
    "src/common/*.cpp",
    "src/common/*.h",
  }

  removefiles {
    "**/ExecPty.*",
    "**/ProcList.*",
    "**/Processes.*",
    "**/base64.*",
    "**/!*.*",
    "**/_*.*",
  }

  vpaths {
    ["Headers"] = {"**.h"},
    ["Sources"] = {"**.cpp"},
  }

  filter "platforms:Win32"
    targetsuffix "32"
  filter "platforms:x64"
    targetsuffix "64"
  filter {}
-- end of "common"




-- ############################### --
-- ############################### --
-- ############################### --
project "ConEmu"
  kind "WindowedApp"
  language "C++"
  flags { "WinMain" }

  links {
    "common",
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

  -- targetdir "../%{cfg.buildcfg}"

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

  links {
    "common",
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

  links {
    "common",
  }

  files {
    "src/common/Common.h",
    "src/ConEmuCD/*.cpp",
    "src/ConEmuCD/*.h",
    "src/ConEmuCD/*.rc",
  }

  filter "action:vs*"
    files "src/ConEmuCD/export.def"
  filter "action:gmake"
    files "src/ConEmuCD/export.gcc.def"
  filter {}

  vpaths {
    { ["Interface"] = {"**/Common.h", "**/SrvCommands.*"} },
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc", "**.rc2", "**.manifest"} },
    { ["Exports"]   = {"**.def"} },
  }

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

  links {
    "common",
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

  links {
    "common",
  }

  files {
    "src/ConEmuPlugin/*.cpp",
    "src/ConEmuPlugin/*.h",
    "src/ConEmuPlugin/*.rc",
  }

  filter "action:vs*"
    files "src/ConEmuPlugin/export.def"
  filter "action:gmake"
    files "src/ConEmuPlugin/export.gcc.def"
  filter {}

  vpaths {
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc", "**.rc2", "**.manifest"} },
    { ["Exports"]   = {"**.def"} },
  }

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

  links {
    "common",
  }

  files {
    "src/ConEmuBg/*.cpp",
    "src/ConEmuBg/*.h",
    "src/ConEmuBg/*.rc",
  }

  filter "action:vs*"
    files "src/ConEmuBg/export.def"
  filter "action:gmake"
    files "src/ConEmuBg/export.gcc.def"
  filter {}

  vpaths {
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc", "**.rc2", "**.manifest"} },
    { ["Exports"]   = {"**.def"} },
  }

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

  links {
    "common",
  }

  files {
    "src/ConEmuLn/*.cpp",
    "src/ConEmuLn/*.h",
    "src/ConEmuLn/*.rc",
  }

  filter "action:vs*"
    files "src/ConEmuLn/export.def"
  filter "action:gmake"
    files "src/ConEmuLn/export.gcc.def"
  filter {}

  vpaths {
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc", "**.rc2", "**.manifest"} },
    { ["Exports"]   = {"**.def"} },
  }

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

  links {
    "common",
  }

  files {
    "src/ConEmuTh/*.cpp",
    "src/ConEmuTh/*.h",
    "src/ConEmuTh/*.rc",
  }

  filter "action:vs*"
    files "src/ConEmuTh/export.def"
  filter "action:gmake"
    files "src/ConEmuTh/export.gcc.def"
  filter {}

  vpaths {
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc", "**.rc2", "**.manifest"} },
    { ["Exports"]   = {"**.def"} },
  }

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

  targetname "gdi+"
  filter "platforms:Win32"
    targetextension "t32"
  filter "platforms:x64"
    targetextension "t64"
  filter {}
-- end of "Far.ConEmuTh.gdi+"




-- ############################### --
-- ############################### --
-- ############################### --
project "Far.ConEmuTh.ico"
  kind "SharedLib"
  language "C++"

  files {
    "src/ConEmuTh/Modules/ico/*.cpp",
    "src/ConEmuTh/Modules/ico/*.h",
    "src/ConEmuTh/Modules/ico/*.rc",
    "src/ConEmuTh/Modules/ico/ico.def",
    "src/ConEmuTh/Modules/ThumbSDK.h",
    "src/ConEmuTh/Modules/MStream.h",
  }

  vpaths {
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc", "**.rc2", "**.manifest"} },
    { ["Exports"]   = {"**.def"} },
  }

  targetname "ico"
  filter "platforms:Win32"
    targetextension "t32"
  filter "platforms:x64"
    targetextension "t64"
  filter {}
-- end of "Far.ConEmuTh.ico"




-- ############################### --
-- ############################### --
-- ############################### --
project "Far.ConEmuTh.pe"
  kind "SharedLib"
  language "C++"

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

  targetname "pe"
  filter "platforms:Win32"
    targetextension "t32"
  filter "platforms:x64"
    targetextension "t64"
  filter {}
-- end of "Far.ConEmuTh.pe"




-- ############################### --
-- ############################### --
-- ############################### --
project "Far.ExtendedConsole"
  kind "SharedLib"
  language "C++"

  links {
    "common",
  }

  files {
    "src/ConEmuDW/*.cpp",
    "src/ConEmuDW/*.h",
    "src/ConEmuDW/*.rc",
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

  targetname "ExtendedConsole"
  filter "platforms:Win32"
    targetsuffix ""
  filter "platforms:x64"
    targetsuffix "64"
  filter {}
-- end of "Far.ExtendedConsole"
