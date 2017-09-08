# Microsoft Visual C++ generated build script - Do not modify

PROJ = INSTALL
DEBUG = 0
PROGTYPE = 0
CALLER = 
ARGS = 
DLLS = 
D_RCDEFINES = /d_DEBUG 
R_RCDEFINES = /dNDEBUG 
ORIGIN = MSVC
ORIGIN_VER = 1.00
PROJPATH = G:\TSPLIB\SAMPLES\INSTALL\
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
CFLAGS_D_WEXE = /nologo /G2 /W3 /Zi /AL /Od /D "_DEBUG" /FR /GA /Fd"INSTALL.PDB" 
CFLAGS_R_WEXE = /nologo /G3 /W4 /WX /Gf /AL /O1 /D "NDEBUG" /GA 
LFLAGS_D_WEXE = /NOLOGO /ONERROR:NOEXE /NOD /PACKC:61440 /CO /ALIGN:16 /STACK:10240
LFLAGS_R_WEXE = /NOLOGO /ONERROR:NOEXE /NOD /PACKC:61440 /ALIGN:16 /STACK:10240
LIBS_D_WEXE = lafxcwd oldnames libw commdlg shell olecli olesvr llibcew
LIBS_R_WEXE = lafxcw oldnames libw commdlg shell olecli olesvr llibcew
RCFLAGS = /nologo 
RESFLAGS = /nologo /k
RUNFLAGS = 
DEFFILE = INSTALL.DEF
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
		DIALOG.SBR


STDAFX_DEP = g:\tsplib\samples\install\stdafx.h \
	c:\progra~1\msvc\include\tapi.h \
	c:\progra~1\msvc\include\tspi.h


DIALOG_DEP = g:\tsplib\samples\install\stdafx.h \
	c:\progra~1\msvc\include\tapi.h \
	c:\progra~1\msvc\include\tspi.h \
	g:\tsplib\samples\install\dialog.h


INSTALL_RCDEP = g:\tsplib\samples\install\install.ico \
	g:\tsplib\samples\install\install.r2c


all:	$(PROJ).EXE

STDAFX.OBJ:	STDAFX.CPP $(STDAFX_DEP)
	$(CPP) $(CFLAGS) $(CPPCREATEPCHFLAG) /c STDAFX.CPP

DIALOG.OBJ:	DIALOG.CPP $(DIALOG_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c DIALOG.CPP

INSTALL.RES:	INSTALL.RC $(INSTALL_RCDEP)
	$(RC) $(RCFLAGS) $(RCDEFINES) -r INSTALL.RC


$(PROJ).EXE::	INSTALL.RES

$(PROJ).EXE::	STDAFX.OBJ DIALOG.OBJ $(OBJS_EXT) $(DEFFILE)
	echo >NUL @<<$(PROJ).CRF
STDAFX.OBJ +
DIALOG.OBJ +
$(OBJS_EXT)
$(PROJ).EXE
$(MAPFILE)
c:\progra~1\msvc\lib\+
c:\progra~1\msvc\mfc\lib\+
$(LIBS)
$(DEFFILE);
<<
	link $(LFLAGS) @$(PROJ).CRF
	$(RC) $(RESFLAGS) INSTALL.RES $@
	@copy $(PROJ).CRF MSVC.BND

$(PROJ).EXE::	INSTALL.RES
	if not exist MSVC.BND 	$(RC) $(RESFLAGS) INSTALL.RES $@

run: $(PROJ).EXE
	$(PROJ) $(RUNFLAGS)


$(PROJ).BSC: $(SBRS)
	bscmake @<<
/o$@ $(SBRS)
<<
