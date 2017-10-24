# Sources location

If you are reading this file, probably you already have sources.

https://conemu.github.io/en/Source.html


## Checkout sources from GitHub

https://github.com/Maximus5/ConEmu  
Use following command to checkout the project sources:

    git clone --recursive https://github.com/Maximus5/ConEmu.git

Submodule ‘minhook’ is available here:
  
    https://github.com/Maximus5/minhook.git

originally forked from here:
  
    https://github.com/RaMMicHaeL/minhook.git


## How to build

1. [Premake 5.0](https://premake.github.io/) is used to generate project files.

2. Visual Studio solution `src/CE.sln` is generated for ‘Visual C++ 2017’.

3. Also you may use GnuC (MinGW) and makefile_all_gcc.

   Example: `mingw32-make -f makefile_all_gcc WIDE=1`  
   ! This method was tested only for x86 builds.  
   ! Still has several limitations due to lack of MinGW headers.
