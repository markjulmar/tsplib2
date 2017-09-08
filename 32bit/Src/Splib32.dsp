# Microsoft Developer Studio Project File - Name="SPLIB32" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=SPLIB32 - Win32 Demo
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Splib32.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Splib32.mak" CFG="SPLIB32 - Win32 Demo"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "SPLIB32 - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "SPLIB32 - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "SPLIB32 - Win32 Debug Unicode" (based on "Win32 (x86) Static Library")
!MESSAGE "SPLIB32 - Win32 Release Unicode" (based on "Win32 (x86) Static Library")
!MESSAGE "SPLIB32 - Win32 Demo" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/TSP++ Library/SRC/32bit", KBAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "SPLIB32 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W4 /WX /GX /O1 /I "..\INCLUDE" /D "NDEBUG" /D "_MBCS" /D "WIN32" /D "_WINDOWS" /D "STRICT" /YX"STDAFX.H" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\LIB\SPLIB32.lib"

!ELSEIF  "$(CFG)" == "SPLIB32 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MTd /W4 /WX /GX /Z7 /Od /Gf /I "..\INCLUDE" /D "_DEBUG" /D "_MBCS" /D "WIN32" /D "_WINDOWS" /D "STRICT" /YX"STDAFX.H" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\LIB\SPLIB32D.lib"

!ELSEIF  "$(CFG)" == "SPLIB32 - Win32 Debug Unicode"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\SPLIB32_"
# PROP BASE Intermediate_Dir ".\SPLIB32_"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\UNICODED"
# PROP Intermediate_Dir ".\UNICODED"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W4 /WX /GX /Zi /Od /Gf /I "..\INCLUDE" /I "..\TAPISDK\INCLUDE" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX"STDAFX.H" /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MTd /W4 /WX /GX /Z7 /Od /Gf /I "..\INCLUDE" /D "_DEBUG" /D "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "STRICT" /YX"STDAFX.H" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\LIB\SPLIB32D.lib"
# ADD LIB32 /nologo /out:"..\LIB\SPLIB32UD.LIB"

!ELSEIF  "$(CFG)" == "SPLIB32 - Win32 Release Unicode"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\SPLIB32_"
# PROP BASE Intermediate_Dir ".\SPLIB32_"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\UNICODER"
# PROP Intermediate_Dir ".\UNICODER"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W4 /WX /GX /O2 /I "..\INCLUDE..\TAPISDK\INCLUDE" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX"STDAFX.H" /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MT /W4 /WX /GX /O2 /I "..\INCLUDE" /D "NDEBUG" /D "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "STRICT" /YX"STDAFX.H" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\LIB\SPLIB32.lib"
# ADD LIB32 /nologo /out:"..\LIB\SPLIB32U.lib"

!ELSEIF  "$(CFG)" == "SPLIB32 - Win32 Demo"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "SPLIB32___Win32_Demo"
# PROP BASE Intermediate_Dir "SPLIB32___Win32_Demo"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Demo"
# PROP Intermediate_Dir "Demo"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W4 /WX /GX /Z7 /Od /Gf /I "..\INCLUDE" /D "_DEBUG" /D "_MBCS" /D "WIN32" /D "_WINDOWS" /D "STRICT" /YX"STDAFX.H" /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MTd /W4 /WX /GX /Od /Gf /I "..\INCLUDE" /D "_DEBUG" /D "_MBCS" /D "WIN32" /D "_WINDOWS" /D "STRICT" /D "_DEMO_VERSION" /YX"STDAFX.H" /FD /c
# SUBTRACT CPP /Z<none> /Fr
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\LIB\SPLIB32D.lib"
# ADD LIB32 /nologo /out:"..\DEMO\SPLIB32D.lib"

!ENDIF 

# Begin Target

# Name "SPLIB32 - Win32 Release"
# Name "SPLIB32 - Win32 Debug"
# Name "SPLIB32 - Win32 Debug Unicode"
# Name "SPLIB32 - Win32 Release Unicode"
# Name "SPLIB32 - Win32 Demo"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\Address.cpp
# End Source File
# Begin Source File

SOURCE=.\Buttinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\Call.cpp
# End Source File
# Begin Source File

SOURCE=.\Confcall.cpp
# End Source File
# Begin Source File

SOURCE=.\Conn.cpp
# End Source File
# Begin Source File

SOURCE=.\Device.cpp
# End Source File
# Begin Source File

SOURCE=.\Display.cpp
# End Source File
# Begin Source File

SOURCE=.\Lineconn.cpp
# End Source File
# Begin Source File

SOURCE=.\map_ds.cpp
# End Source File
# Begin Source File

SOURCE=.\Misc.cpp
# End Source File
# Begin Source File

SOURCE=.\Phonecon.cpp
# End Source File
# Begin Source File

SOURCE=.\Request.cpp
# End Source File
# Begin Source File

SOURCE=.\Sp.cpp
# End Source File
# Begin Source File

SOURCE=.\Spdll.cpp
# End Source File
# Begin Source File

SOURCE=.\Spline.cpp
# End Source File
# Begin Source File

SOURCE=.\Spphone.cpp
# End Source File
# Begin Source File

SOURCE=.\Spthread.cpp
# End Source File
# Begin Source File

SOURCE=.\STDAFX.H
# End Source File
# Begin Source File

SOURCE=.\uisupp.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=..\Include\Splib.h
# End Source File
# Begin Source File

SOURCE=..\Include\Splib.inl
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
