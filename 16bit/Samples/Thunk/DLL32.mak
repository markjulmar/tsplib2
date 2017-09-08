# Microsoft Developer Studio Generated NMAKE File, Format Version 4.10
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=DLL32 - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to DLL32 - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "DLL32 - Win32 Release" && "$(CFG)" != "DLL32 - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "DLL32.mak" CFG="DLL32 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DLL32 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "DLL32 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "DLL32 - Win32 Debug"
CPP=cl.exe
RSC=rc.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "DLL32 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\DLL32.dll" ".\32to16.asm"

CLEAN : 
	-@erase "$(INTDIR)\dll32.res"
	-@erase "$(INTDIR)\dllmain.obj"
	-@erase "$(OUTDIR)\DLL32.dll"
	-@erase "$(OUTDIR)\DLL32.exp"
	-@erase "$(OUTDIR)\DLL32.lib"
	-@erase ".\32to16.asm"
	-@erase ".\32to16.obj"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/DLL32.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/dll32.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/DLL32.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/DLL32.pdb" /machine:I386 /def:".\dll32.def"\
 /out:"$(OUTDIR)/DLL32.dll" /implib:"$(OUTDIR)/DLL32.lib" 
DEF_FILE= \
	".\dll32.def"
LINK32_OBJS= \
	"$(INTDIR)\dll32.res" \
	"$(INTDIR)\dllmain.obj" \
	".\32to16.obj"

"$(OUTDIR)\DLL32.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build
InputPath=.\Release\DLL32.dll
SOURCE=$(InputPath)

BuildCmds= \
	thunk -t thk -o 32to16.asm 32to16.thk \
	ml /W3 /c /DIS_32 32to16.asm \
	

"32to16.asm" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"32to16.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "DLL32 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : 

CLEAN : 
	-@erase "$(INTDIR)\dll32.res"
	-@erase "$(INTDIR)\dllmain.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\DLL32.dll"
	-@erase "$(OUTDIR)\DLL32.exp"
	-@erase "$(OUTDIR)\DLL32.ilk"
	-@erase "$(OUTDIR)\DLL32.lib"
	-@erase "$(OUTDIR)\DLL32.pdb"
	-@erase ".\32to16.asm"
	-@erase ".\32to16.obj"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/DLL32.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/dll32.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/DLL32.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)/DLL32.pdb" /debug /machine:I386 /def:".\dll32.def"\
 /out:"$(OUTDIR)/DLL32.dll" /implib:"$(OUTDIR)/DLL32.lib" 
DEF_FILE= \
	".\dll32.def"
LINK32_OBJS= \
	"$(INTDIR)\dll32.res" \
	"$(INTDIR)\dllmain.obj" \
	".\32to16.obj"

"$(OUTDIR)\DLL32.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build
InputPath=.\Debug\DLL32.dll
SOURCE=$(InputPath)

BuildCmds= \
	thunk -t thk -o 32to16.asm 32to16.thk \
	ml /W3 /c /DIS_32 32to16.asm \
	

"32to16.asm" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"32to16.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "DLL32 - Win32 Release"
# Name "DLL32 - Win32 Debug"

!IF  "$(CFG)" == "DLL32 - Win32 Release"

!ELSEIF  "$(CFG)" == "DLL32 - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\dll32.h

!IF  "$(CFG)" == "DLL32 - Win32 Release"

!ELSEIF  "$(CFG)" == "DLL32 - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\dllmain.c

"$(INTDIR)\dllmain.obj" : $(SOURCE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\dll32.rc

"$(INTDIR)\dll32.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\dll32.def

!IF  "$(CFG)" == "DLL32 - Win32 Release"

!ELSEIF  "$(CFG)" == "DLL32 - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\32to16.asm

!IF  "$(CFG)" == "DLL32 - Win32 Release"

# Begin Custom Build
InputPath=.\32to16.asm

"32to16.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   ml /DIS_32 /c /W3 32to16.asm

# End Custom Build

!ELSEIF  "$(CFG)" == "DLL32 - Win32 Debug"

# Begin Custom Build
InputPath=.\32to16.asm

"32to16.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   ml /DIS_32 /c /W3 32to16.asm

# End Custom Build

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\32to16.thk

!IF  "$(CFG)" == "DLL32 - Win32 Release"

!ELSEIF  "$(CFG)" == "DLL32 - Win32 Debug"

# Begin Custom Build
InputPath=.\32to16.thk

"32to16.asm" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   thunk -t thk -o 32to16.asm 32to16.thk

# End Custom Build

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
