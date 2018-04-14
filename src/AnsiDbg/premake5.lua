workspace "AnsiDbg"
  configurations { "Release", "Debug" }
  platforms { "Win32" }
  flags { "StaticRuntime" }

  filter "platforms:Win32"
    architecture "x32"
    defines { "WIN32", "_WIN32" }

  filter "action:vs2017"
    toolset "v141_xp"
  filter "action:vs2015"
    toolset "v140_xp"
  filter "action:vs2013"
    toolset "v120_xp"
  filter "action:vs2012"
    toolset "v110_xp"
--filter "action:vs*"
--  entrypoint "wmain"
  filter "action:gmake"
    entrypoint "main"

  filter "configurations:Release"
    defines { "NDEBUG" }
    optimize "Size"
    symbols "On"

  filter "configurations:Debug"
    defines { "_DEBUG" }
    symbols "On"

  filter{}


-- ############################### --
-- ############################### --
-- ############################### --
project "AnsiDbg"
  kind "ConsoleApp"
  language "C++"

  files {
    "AnsiDbg.cpp",
    "ReadMe.txt",
  }

  vpaths {
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { [""] = {"**.txt"} },
  }

  targetdir "."
-- end of "AnsiDbg"
