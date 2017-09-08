# Microsoft Visual C++ generated build script - Do not modify

PROJ = ATV2EXE
DEBUG = 0
PROGTYPE = 0
CALLER = 
ARGS = 
DLLS = 
D_RCDEFINES = -d_DEBUG
R_RCDEFINES = -dNDEBUG
ORIGIN = MSVC
ORIGIN_VER = 1.00
PROJPATH = G:\TSPLIB\SAMPLES\ATSP\APP\
USEMFC = 1
CC = cl
CPP = cl
CXX = cl
CCREATEPCHFLAG = 
CPPCREATEPCHFLAG = /YcSTDAFX.H
CUSEPCHFLAG = 
CPPUSEPCHFLAG = /YuSTDAFX.H
FIRSTC =             
FIRSTCPP = STDAFX.CPP  
RC = rc
CFLAGS_D_WEXE = /nologo /G2 /W4 /WX /Gf /Zi /AL /Od /D "_DEBUG" /I "..\..\..\include" /GA /Fd"ATV2EXE.PDB" 
CFLAGS_R_WEXE = /nologo /Gs /G2 /W4 /WX /Gf /AL /O1 /D "NDEBUG" /I "..\..\..\inc16" /GA 
LFLAGS_D_WEXE = /NOLOGO /NOD /PACKC:61440 /STACK:10240 /ALIGN:16 /ONERROR:NOEXE /CO  
LFLAGS_R_WEXE = /NOLOGO /NOD /PACKC:61440 /STACK:10240 /ALIGN:16 /ONERROR:NOEXE  
LIBS_D_WEXE = lafxcwd oldnames libw llibcew ..\sp\atspv2.lib commdlg.lib shell.lib 
LIBS_R_WEXE = lafxcw oldnames libw llibcew ..\sp\atspv2.lib commdlg.lib shell.lib 
RCFLAGS = /nologo
RESFLAGS = /nologo
RUNFLAGS = 
DEFFILE = ATV2EXE.DEF
OBJS_EXT = 
LIBS_EXT = 
!if "$(DEBUG)" == "1"
CFLAGS = $(CFLAGS_D_WEXE)
LFLAGS = $(LFLAGS_D_WEXE)
LIBS = $(LIBS_D_WEXE)
MAPFILE = nul
RCDEFINES = $(D_RCDEFINES)
!else
CFLAGS = $(CFLAGS_R_WEXE)
LFLAGS = $(LFLAGS_R_WEXE)
LIBS = $(LIBS_R_WEXE)
MAPFILE = nul
RCDEFINES = $(R_RCDEFINES)
!endif
!if [if exist MSVC.BND del MSVC.BND]
!endif
SBRS = STDAFX.SBR \
		ATV2EXE.SBR


STDAFX_DEP = g:\tsplib\samples\atsp\app\stdafx.h


ATV2EXE_DEP = g:\tsplib\samples\atsp\app\stdafx.h \
	g:\tsplib\samples\atsp\app\atv2exe.h \
	g:\tsplib\samples\atsp\sp\atspint.h


ATV2EXE_RCDEP = g:\tsplib\samples\atsp\app\atv2exe.ico


all:	$(PROJ).EXE

STDAFX.OBJ:	STDAFX.CPP $(STDAFX_DEP)
	$(CPP) $(CFLAGS) $(CPPCREATEPCHFLAG) /c STDAFX.CPP

ATV2EXE.OBJ:	ATV2EXE.CPP $(ATV2EXE_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c ATV2EXE.CPP

ATV2EXE.RES:	ATV2EXE.RC $(ATV2EXE_RCDEP)
	$(RC) $(RCFLAGS) $(RCDEFINES) -r ATV2EXE.RC


$(PROJ).EXE::	ATV2EXE.RES

$(PROJ).EXE::	STDAFX.OBJ ATV2EXE.OBJ $(OBJS_EXT) $(DEFFILE)
	echo >NUL @<<$(PROJ).CRF
STDAFX.OBJ +
ATV2EXE.OBJ +
$(OBJS_EXT)
$(PROJ).EXE
$(MAPFILE)
c:\progra~1\msvc\lib\+
c:\progra~1\msvc\mfc\lib\+
$(LIBS)
$(DEFFILE);
<<
	link $(LFLAGS) @$(PROJ).CRF
	$(RC) $(RESFLAGS) ATV2EXE.RES $@
	@copy $(PROJ).CRF MSVC.BND

$(PROJ).EXE::	ATV2EXE.RES
	if not exist MSVC.BND 	$(RC) $(RESFLAGS) ATV2EXE.RES $@

run: $(PROJ).EXE
	$(PROJ) $(RUNFLAGS)


$(PROJ).BSC: $(SBRS)
	bscmake @<<
/o$@ $(SBRS)
<<
