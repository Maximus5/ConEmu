-- ConEmu setupper solution generator script
-- 1. Download premake5 from https://premake.github.io/download.html
-- 2. Run `premake5 vs2019`

workspace "Setupper"
  configurations { "Release" }
  platforms { "Win32" }

  staticruntime "On"
  filter "configurations:Release"
    flags { "NoIncrementalLink" }

  filter "platforms:Win32"
    architecture "x32"
    defines { "WIN32", "_WIN32" }

  filter "action:vs2019"
    toolset "v141_xp"
  filter "action:vs2017"
    toolset "v141_xp"
  filter "action:vs2015"
    toolset "v140_xp"
  filter "action:vs2013"
    toolset "v120_xp"
  filter "action:vs2012"
    toolset "v110_xp"

-- ############################### --
-- ############################### --
-- ############################### --
project "Setupper"
  kind "WindowedApp"
  language "C++"
  --exceptionhandling "Off"
  --entrypoint "WinMainCRTStartup"

  links {
    "user32",
    "shell32",
  }

  files {
    "*.h",
    "*.cpp",
    "*.rc",
    "*.rc2",
    "../../ConEmu/ConEmu.ico",
    "*.manifest",
  }

  vpaths {
    { ["Resources"] = {"*.rc", "*.rc2", "*.manifest", "*.bmp", "*.cur", "../../ConEmu/ConEmu.ico"} },
    { ["Headers"]   = {"*.h"} },
    { ["Sources"]   = {"*.cpp"} },
  }

  targetdir  "."
  objdir     "obj"
  implibdir  "%{cfg.objdir}"
  defines  { "NDEBUG", "HIDE_TODO", "NOMINMAX", "VSBUILD" }
  optimize   "Size"
  symbols    "On"
