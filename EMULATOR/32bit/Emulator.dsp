# Microsoft Developer Studio Project File - Name="Emulator" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Emulator - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Emulator.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Emulator.mak" CFG="Emulator - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Emulator - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Emulator - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/TSP++ Library/EMULATOR/32bit", NKAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Emulator - Win32 Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 winmm.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "Emulator - Win32 Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 winmm.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Emulator - Win32 Release"
# Name "Emulator - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\About.cpp
# End Source File
# Begin Source File

SOURCE=.\Addrset.cpp
# End Source File
# Begin Source File

SOURCE=.\Baseprop.cpp
# End Source File
# Begin Source File

SOURCE=.\Call.cpp
# End Source File
# Begin Source File

SOURCE=.\Colorlb.cpp
# End Source File
# Begin Source File

SOURCE=.\Confgdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Ddx.cpp
# End Source File
# Begin Source File

SOURCE=.\Dial.cpp
# End Source File
# Begin Source File

SOURCE=.\drv.cpp
# End Source File
# Begin Source File

SOURCE=.\Emulator.cpp
# End Source File
# Begin Source File

SOURCE=.\Emulator.rc
# End Source File
# Begin Source File

SOURCE=.\Gendlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Gensetup.cpp
# End Source File
# Begin Source File

SOURCE=.\Gentone.cpp
# End Source File
# Begin Source File

SOURCE=.\Lamp.cpp
# End Source File
# Begin Source File

SOURCE=.\Objects.cpp
# End Source File
# Begin Source File

SOURCE=.\socket.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\Ttbutt.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\About.h
# End Source File
# Begin Source File

SOURCE=.\Addrset.h
# End Source File
# Begin Source File

SOURCE=.\Baseprop.h
# End Source File
# Begin Source File

SOURCE=.\Call.h
# End Source File
# Begin Source File

SOURCE=.\Colorlb.h
# End Source File
# Begin Source File

SOURCE=.\Confgdlg.h
# End Source File
# Begin Source File

SOURCE=.\controls.h
# End Source File
# Begin Source File

SOURCE=.\Dial.h
# End Source File
# Begin Source File

SOURCE=.\Emintf.h
# End Source File
# Begin Source File

SOURCE=.\Emulator.h
# End Source File
# Begin Source File

SOURCE=.\Gendlg.h
# End Source File
# Begin Source File

SOURCE=.\Gensetup.h
# End Source File
# Begin Source File

SOURCE=.\Gentone.h
# End Source File
# Begin Source File

SOURCE=.\Objects.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\socket.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\Res\Answer.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\Busy.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\Config.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\Dial.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\Digits.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Emulator.ico
# End Source File
# Begin Source File

SOURCE=.\res\Emulator.rc2
# End Source File
# Begin Source File

SOURCE=.\Res\Exit.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\Generate.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\Helpme.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\Hold.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\Incoming.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\Left.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\Release.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\Reset.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\Right.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\Ring1.wav
# End Source File
# Begin Source File

SOURCE=.\Res\Ring2.wav
# End Source File
# Begin Source File

SOURCE=.\Res\Ring3.wav
# End Source File
# End Group
# End Target
# End Project
