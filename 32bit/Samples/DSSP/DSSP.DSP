# Microsoft Developer Studio Project File - Name="DSSP" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=DSSP - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DSSP.MAK".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DSSP.MAK" CFG="DSSP - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DSSP - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "DSSP - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/First Party TSP++/32bit/Samples/DSSP", NJAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "DSSP - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W4 /WX /GX /O2 /I "..\..\INCLUDE" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /Yc"STDAFX.H" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winmm.lib wow32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib splib32.lib /nologo /subsystem:windows /dll /machine:I386 /out:"Release/DSSP32.tsp" /libpath:"..\..\LIB"

!ELSEIF  "$(CFG)" == "DSSP - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MTd /W4 /WX /GR /GX /Z7 /Od /I "..\..\INCLUDE" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /Yc"STDAFX.H" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winmm.lib wow32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib splib32d.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /out:".\DEBUG\DSSP32.TSP" /pdbtype:sept /libpath:"..\..\LIB"

!ENDIF 

# Begin Target

# Name "DSSP - Win32 Release"
# Name "DSSP - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CONFIG.CPP
# ADD CPP /YX"STDAFX.H"
# End Source File
# Begin Source File

SOURCE=.\DRV.CPP
# ADD CPP /YX"STDAFX.H"
# End Source File
# Begin Source File

SOURCE=.\DSSP.CPP
# ADD CPP /YX"STDAFX.H"
# End Source File
# Begin Source File

SOURCE=.\DSSP.DEF
# End Source File
# Begin Source File

SOURCE=.\DSSP.RC
# End Source File
# Begin Source File

SOURCE=.\INIT.CPP
# ADD CPP /YX"STDAFX.H"
# End Source File
# Begin Source File

SOURCE=.\line.cpp
# ADD CPP /YX"STDAFX.H"
# End Source File
# Begin Source File

SOURCE=.\LineReq.cpp
# ADD CPP /YX"STDAFX.H"
# End Source File
# Begin Source File

SOURCE=.\Phone.cpp
# ADD CPP /YX"STDAFX.H"
# End Source File
# Begin Source File

SOURCE=.\STDAFX.CPP
# ADD CPP /Yc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\_BldNum.h
# End Source File
# Begin Source File

SOURCE=.\Config.h
# End Source File
# Begin Source File

SOURCE=.\DSSP.H
# End Source File
# Begin Source File

SOURCE=.\EMINTF.H
# End Source File
# Begin Source File

SOURCE=.\STDAFX.H
# End Source File
# End Group
# Begin Group "External Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Inc32\Splib.h
# End Source File
# Begin Source File

SOURCE="C:\Program Files\DevStudio\Vc\include\Tapi.h"
# End Source File
# End Group
# Begin Group "Resource"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\phone.ico
# End Source File
# End Group
# End Target
# End Project
