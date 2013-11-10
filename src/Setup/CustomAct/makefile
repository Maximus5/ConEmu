#!include params.mak

!if "$(CPU)" == "AMD64"
!if "$(PLATFORM)" != "x64"
!error "$(PLATFORM) != $(CPU)"
!endif
!else
!if "$(PLATFORM)" != "x86"
!error "$(PLATFORM) != $(CPU)"
!endif
!endif

CPPFLAGS = -nologo -D_WIN32_WINNT=0x0400
LINKFLAGS = -nologo -incremental:no -machine:$(PLATFORM)
CANDLEFLAGS = -nologo -dSourceDir="$(SOURCE_DIR)" -dBranch=$(VER_MAJOR) -dPlatform=$(PLATFORM) -dVersion=$(VER_MAJOR).$(VER_MINOR).$(VER_BUILD)
LIGHTFLAGS = -nologo -spdb -sval

!ifdef RELEASE
OUTDIR = Release
CPPFLAGS = $(CPPFLAGS) -O1 -DNDEBUG
LIGHTFLAGS = $(LIGHTFLAGS) -dcl:high
!else
OUTDIR = Debug
CPPFLAGS = $(CPPFLAGS) -Fd$(OUTDIR)\ -Zi
LINKFLAGS = $(LINKFLAGS) -debug
LIGHTFLAGS = $(LIGHTFLAGS) -cc $(OUTDIR) -reusecab -O2
!endif

OUTDIR = $(OUTDIR).$(PLATFORM).$(VER_MAJOR)
CPPFLAGS = -Fo$(OUTDIR)\ $(CPPFLAGS)
CANDLEFLAGS = -out $(OUTDIR)\ $(CANDLEFLAGS)

#WIXOBJ = $(OUTDIR)\installer.wixobj $(OUTDIR)\files.wixobj $(OUTDIR)\features.wixobj $(OUTDIR)\shortcuts.wixobj $(OUTDIR)\ui.wixobj
#!ifdef NIGHTLY
#MSI_NAME = $(SOURCE_DIR)\final.msi
#LIGHTFLAGS = $(LIGHTFLAGS) -sh
#!else
#MSI_NAME = $(OUTDIR)\far.msi
#!endif

#$(MSI_NAME): $(OUTDIR) $(OUTDIR)\1033.msi
#  copy /y $(OUTDIR)\1033.msi $(MSI_NAME)
#
#$(OUTDIR)\1033.msi: $(WIXOBJ) en-us.wxl ui_en-us.wxl $(OUTDIR)\customact.dll
#  light $(LIGHTFLAGS) -cultures:en-us -loc en-us.wxl -loc ui_en-us.wxl -out $@ $(WIXOBJ)
#
#$(OUTDIR)\1049.msi: $(WIXOBJ) ru-ru.wxl ui_ru-ru.wxl $(OUTDIR)\customact.dll
#  light $(LIGHTFLAGS) -cultures:ru-ru -loc ru-ru.wxl -loc ui_ru-ru.wxl -out $@ $(WIXOBJ)
#
#$(OUTDIR)\1049: $(OUTDIR)\1033.msi $(OUTDIR)\1049.msi
#  torch -nologo -t language $(OUTDIR)\1033.msi $(OUTDIR)\1049.msi -out $@

#$(OUTDIR)\CustomActions.dll: $(OUTDIR) customact.cpp customact.def $(OUTDIR)\customact.res
#  cl $(CPPFLAGS) -EHsc customact.cpp -link $(LINKFLAGS) -dll -out:$@ -def:customact.def $(OUTDIR)\customact.res msi.lib shell32.lib rpcrt4.lib
$(OUTDIR)\CustomActions.dll: $(OUTDIR) customact.cpp customact.def
  cl $(CPPFLAGS) -EHsc customact.cpp -link $(LINKFLAGS) -dll -out:$@ -def:customact.def msi.lib shell32.lib rpcrt4.lib Kernel32.lib Ole32.lib

#$(OUTDIR)\customact.res: customact.rc $(OUTDIR)\consize.exe
#  rc -DOUTDIR=$(OUTDIR) -fo$@ customact.rc

#$(OUTDIR)\consize.exe: consize.cpp
#  cl $(CPPFLAGS) -GS- consize.cpp -link $(LINKFLAGS) -nodefaultlib -entry:main -out:$@ kernel32.lib

#$(WIXOBJ): params.mak defines.wxi guids_$(VER_MAJOR)_$(PLATFORM).wxi

#.wxs{$(OUTDIR)}.wixobj::
#  candle $(CANDLEFLAGS) $<

$(OUTDIR):
  if not exist $(OUTDIR) mkdir $(OUTDIR)


clean:
#  if exist $(OUTDIR) rd /s /q $(OUTDIR)


.SUFFIXES: .wxs
.PHONY: clean
