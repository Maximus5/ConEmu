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

1. Visual Studio solution (*.sln) files. Supported versions:

   * CE.sln    Visual C++ 2008 and Windows SDK 7.0 (required!)  
   * CE10.sln  Visual C++ 2010  
   * CE11.sln  Visual C++ 2012  
   * CE12.sln  Visual C++ 2013  
   * CE14.sln  Visual C++ 2015  

2. Visual C++ makefiles (current releases are build with Windows SDK 7.0)

   Example: `nmake /F makefile_all_vc`  
   ! This method requires libCRT.lib (libCRT64.lib),  
   ! placed in 'common' subfolder.  
   ! These files may be compiled from Far Manager sources.  
   ! http://farmanager.googlecode.com/svn/trunk/plugins  -->  common folder  
   ! Windows SDK 7.0 required, check version in registry:  
   ! HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Microsoft SDKs\Windows  
   ! HKEY_CURRENT_USER\SOFTWARE\Microsoft\Microsoft SDKs\Windows
        
3. Use GnuC (MinGW) and makefile_all_gcc.

   Example: `mingw32-make -f makefile_all_gcc WIDE=1`  
   ! This method was tested only for x86 builds.  
   ! Still has several limitations due to lack of MinGW headers.
