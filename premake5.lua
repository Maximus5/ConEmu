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

  pchheader "Header.h"
  pchsource "Header.cpp"

  files {
    "src/ConEmu/*.cpp",
    "src/ConEmu/*.h",
    "src/ConEmu/*.rc",
    "src/ConEmu/*.rc2",
    "src/ConEmu/*.bmp",
    "src/ConEmu/*.cur",
    "src/ConEmu/*.ico",
    "src/ConEmu/conemu.manifest",
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
    ["Resources"] = {"**/*.rc", "**/*.manifest", "**/*.bmp", "**/*.cur", "**/*.ico"},
    ["Settings"]  = {"**/SetPg*.*"},
    ["Headers"]   = {"**/*.h"},
    ["Sources"]   = {"**/*.cpp"},
  }

  -- targetdir "../%{cfg.buildcfg}"

  targetname "ConEmu"
  filter "platforms:x64"
    targetsuffix "64"

-- end of "ConEmu"
