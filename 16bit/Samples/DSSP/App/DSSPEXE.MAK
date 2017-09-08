# Microsoft Visual C++ generated build script - Do not modify

PROJ = DSSPEXE
DEBUG = 0
PROGTYPE = 0
CALLER = 
ARGS = 
DLLS = 
D_RCDEFINES = -d_DEBUG
R_RCDEFINES = -dNDEBUG
ORIGIN = MSVC
ORIGIN_VER = 1.00
PROJPATH = G:\TSPLIB\SAMPLES\DSSP\APP\
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
CFLAGS_D_WEXE = /nologo /G2 /W4 /WX /Gf /Zi /AL /Od /D "_DEBUG" /I "..\..\..\include" /I "..\..\..\tapisdk\include" /GA /Fd"TBREXE.PDB" 
CFLAGS_R_WEXE = /nologo /Gs /G2 /W4 /WX /Gf /AL /O1 /D "NDEBUG" /I "..\..\..\inc16" /GA 
LFLAGS_D_WEXE = /NOLOGO /NOD /PACKC:61440 /STACK:10240 /ALIGN:16 /ONERROR:NOEXE /CO /MAP:FULL
LFLAGS_R_WEXE = /NOLOGO /NOD /PACKC:61440 /STACK:10240 /ALIGN:16 /ONERROR:NOEXE /MAP  
LIBS_D_WEXE = lafxcwd oldnames libw llibcew ..\sp\dssp.lib commdlg.lib shell.lib 
LIBS_R_WEXE = lafxcw oldnames libw llibcew ..\sp\dssp.lib commdlg.lib shell.lib 
RCFLAGS = /nologo
RESFLAGS = /nologo
RUNFLAGS = 
DEFFILE = DSSPEXE.DEF
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
		DSSPEXE.SBR


STDAFX_DEP = g:\tsplib\samples\dssp\app\stdafx.h


DSSPEXE_DEP = g:\tsplib\samples\dssp\app\stdafx.h \
	g:\tsplib\samples\dssp\app\dsspexe.h \
	g:\tsplib\samples\dssp\sp\dsspint.h \
	g:\tsplib\samples\dssp\sp\emintf.h


DSSPEXE_RCDEP = g:\tsplib\samples\dssp\app\dssp.ico


all:	$(PROJ).EXE

STDAFX.OBJ:	STDAFX.CPP $(STDAFX_DEP)
	$(CPP) $(CFLAGS) $(CPPCREATEPCHFLAG) /c STDAFX.CPP

DSSPEXE.OBJ:	DSSPEXE.CPP $(DSSPEXE_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c DSSPEXE.CPP

DSSPEXE.RES:	DSSPEXE.RC $(DSSPEXE_RCDEP)
	$(RC) $(RCFLAGS) $(RCDEFINES) -r DSSPEXE.RC


$(PROJ).EXE::	DSSPEXE.RES

$(PROJ).EXE::	STDAFX.OBJ DSSPEXE.OBJ $(OBJS_EXT) $(DEFFILE)
	echo >NUL @<<$(PROJ).CRF
STDAFX.OBJ +
DSSPEXE.OBJ +
$(OBJS_EXT)
$(PROJ).EXE
$(MAPFILE)
c:\progra~1\msvc\lib\+
c:\progra~1\msvc\mfc\lib\+
$(LIBS)
$(DEFFILE);
<<
	link $(LFLAGS) @$(PROJ).CRF
	$(RC) $(RESFLAGS) DSSPEXE.RES $@
	@copy $(PROJ).CRF MSVC.BND

$(PROJ).EXE::	DSSPEXE.RES
	if not exist MSVC.BND 	$(RC) $(RESFLAGS) DSSPEXE.RES $@

run: $(PROJ).EXE
	$(PROJ) $(RUNFLAGS)


$(PROJ).BSC: $(SBRS)
	bscmake @<<
/o$@ $(SBRS)
<<
