# Microsoft Visual C++ generated build script - Do not modify

PROJ = DSSP
DEBUG = 0
PROGTYPE = 1
CALLER = 
ARGS = 
DLLS = 
D_RCDEFINES = /d_DEBUG 
R_RCDEFINES = /dNDEBUG 
ORIGIN = MSVC
ORIGIN_VER = 1.00
PROJPATH = E:\TSPLIB\16BIT\SAMPLES\DSSP\SP\
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
CFLAGS_D_WDLL = /nologo /G2 /W4 /WX /Gf /Z7 /ALw /Od /D "_DEBUG" /I "..\..\INCLUDE" /GD 
CFLAGS_R_WDLL = /nologo /Gs /G3 /W4 /WX /Gf /ALw /O1 /D "NDEBUG" /I "..\..\..\INCLUDE" /GD /GEf 
LFLAGS_D_WDLL = /NOLOGO /NOD /NOE /PACKC:61440 /ALIGN:16 /ONERROR:NOEXE /CO /MAP /MAP:FULL
LFLAGS_R_WDLL = /NOLOGO /NOD /NOE /PACKC:61440 /ALIGN:16 /ONERROR:NOEXE /MAP /MAP:FULL
LIBS_D_WDLL = lafxdwd ..\..\..\lib\splibd oldnames libw ldllcew commdlg.lib shell.lib 
LIBS_R_WDLL = lafxdw ..\..\..\lib\splib oldnames libw ldllcew commdlg.lib mmsystem.lib shell.lib 
RCFLAGS = /nologo 
RESFLAGS = /nologo /31 
RUNFLAGS = 
DEFFILE = DSSP.DEF
OBJS_EXT = 
LIBS_EXT = 
!if "$(DEBUG)" == "1"
CFLAGS = $(CFLAGS_D_WDLL)
LFLAGS = $(LFLAGS_D_WDLL)
LIBS = $(LIBS_D_WDLL)
MAPFILE = nul
RCDEFINES = $(D_RCDEFINES)
!else
CFLAGS = $(CFLAGS_R_WDLL)
LFLAGS = $(LFLAGS_R_WDLL)
LIBS = $(LIBS_R_WDLL)
MAPFILE = nul
RCDEFINES = $(R_RCDEFINES)
!endif
!if [if exist MSVC.BND del MSVC.BND]
!endif
SBRS = STDAFX.SBR \
		CONFIG.SBR \
		DRV.SBR \
		DSSP.SBR \
		INIT.SBR \
		LINE.SBR \
		LINEREQ.SBR \
		MISC.SBR \
		PHONE.SBR


STDAFX_DEP = e:\tsplib\16bit\samples\dssp\sp\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h


CONFIG_DEP = e:\tsplib\16bit\samples\dssp\sp\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h \
	e:\tsplib\16bit\samples\dssp\sp\dssp.h \
	e:\tsplib\16bit\samples\dssp\sp\emintf.h


DRV_DEP = e:\tsplib\16bit\samples\dssp\sp\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h \
	e:\tsplib\16bit\samples\dssp\sp\dssp.h \
	e:\tsplib\16bit\samples\dssp\sp\emintf.h \
	e:\tsplib\16bit\samples\dssp\sp\dsspint.h


DSSP_DEP = e:\tsplib\16bit\samples\dssp\sp\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h \
	e:\tsplib\16bit\samples\dssp\sp\dssp.h \
	e:\tsplib\16bit\samples\dssp\sp\emintf.h


INIT_DEP = e:\tsplib\16bit\samples\dssp\sp\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h \
	e:\tsplib\16bit\samples\dssp\sp\dssp.h \
	e:\tsplib\16bit\samples\dssp\sp\emintf.h \
	e:\tsplib\16bit\samples\dssp\sp\dsspint.h


LINE_DEP = 

LINEREQ_DEP = 

MISC_DEP = e:\tsplib\16bit\samples\dssp\sp\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h \
	e:\tsplib\16bit\samples\dssp\sp\dssp.h \
	e:\tsplib\16bit\samples\dssp\sp\emintf.h


PHONE_DEP = 

DSSP_RCDEP = 

all:	$(PROJ).DLL

STDAFX.OBJ:	STDAFX.CPP $(STDAFX_DEP)
	$(CPP) $(CFLAGS) $(CPPCREATEPCHFLAG) /c STDAFX.CPP

CONFIG.OBJ:	CONFIG.CPP $(CONFIG_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c CONFIG.CPP

DRV.OBJ:	DRV.CPP $(DRV_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c DRV.CPP

DSSP.OBJ:	DSSP.CPP $(DSSP_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c DSSP.CPP

INIT.OBJ:	INIT.CPP $(INIT_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c INIT.CPP

LINE.OBJ:	LINE.CPP $(LINE_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c LINE.CPP

LINEREQ.OBJ:	LINEREQ.CPP $(LINEREQ_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c LINEREQ.CPP

MISC.OBJ:	MISC.CPP $(MISC_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c MISC.CPP

PHONE.OBJ:	PHONE.CPP $(PHONE_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c PHONE.CPP

DSSP.RES:	DSSP.RC $(DSSP_RCDEP)
	$(RC) $(RCFLAGS) $(RCDEFINES) -r DSSP.RC


$(PROJ).DLL::	DSSP.RES

$(PROJ).DLL::	STDAFX.OBJ CONFIG.OBJ DRV.OBJ DSSP.OBJ INIT.OBJ LINE.OBJ LINEREQ.OBJ \
	MISC.OBJ PHONE.OBJ $(OBJS_EXT) $(DEFFILE)
	echo >NUL @<<$(PROJ).CRF
STDAFX.OBJ +
CONFIG.OBJ +
DRV.OBJ +
DSSP.OBJ +
INIT.OBJ +
LINE.OBJ +
LINEREQ.OBJ +
MISC.OBJ +
PHONE.OBJ +
$(OBJS_EXT)
$(PROJ).DLL
$(MAPFILE)
c:\progra~1\msvc\lib\+
c:\progra~1\msvc\mfc\lib\+
$(LIBS)
$(DEFFILE);
<<
	link $(LFLAGS) @$(PROJ).CRF
	$(RC) $(RESFLAGS) DSSP.RES $@
	@copy $(PROJ).CRF MSVC.BND
	implib /nowep $(PROJ).LIB $(PROJ).DLL

$(PROJ).DLL::	DSSP.RES
	if not exist MSVC.BND 	$(RC) $(RESFLAGS) DSSP.RES $@

run: $(PROJ).DLL
	$(PROJ) $(RUNFLAGS)


$(PROJ).BSC: $(SBRS)
	bscmake @<<
/o$@ $(SBRS)
<<
