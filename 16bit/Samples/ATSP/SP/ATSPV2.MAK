# Microsoft Visual C++ generated build script - Do not modify

PROJ = ATSPV2
DEBUG = 0
PROGTYPE = 1
CALLER = 
ARGS = 
DLLS = 
D_RCDEFINES = /d_DEBUG 
R_RCDEFINES = /dNDEBUG 
ORIGIN = MSVC
ORIGIN_VER = 1.00
PROJPATH = G:\TSPLIB\SAMPLES\ATSP\SP\
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
CFLAGS_D_WDLL = /nologo /G2 /W4 /WX /Gf /Z7 /ALw /Od /D "_DEBUG" /I "..\..\..\include" /GD 
CFLAGS_R_WDLL = /nologo /Gs /G3 /W4 /WX /Gf /ALw /O1 /D "NDEBUG" /I "..\..\..\inc16" /GD /GEf 
LFLAGS_D_WDLL = /NOLOGO /NOD /NOE /PACKC:61440 /ALIGN:16 /ONERROR:NOEXE /CO /MAP /MAP:FULL
LFLAGS_R_WDLL = /NOLOGO /NOD /NOE /PACKC:61440 /ALIGN:16 /ONERROR:NOEXE /MAP /MAP:FULL
LIBS_D_WDLL = lafxdwd ..\..\..\lib\splibd oldnames libw ldllcew commdlg.lib shell.lib 
LIBS_R_WDLL = lafxdw ..\..\..\lib\splib oldnames libw ldllcew commdlg.lib shell.lib 
RCFLAGS = /nologo 
RESFLAGS = /nologo /31 
RUNFLAGS = 
DEFFILE = ATSP.DEF
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
		ATSP.SBR \
		TALKDROP.SBR \
		UI.SBR \
		REQUEST.SBR \
		LINE.SBR


STDAFX_DEP = g:\tsplib\samples\atsp\sp\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h


CONFIG_DEP = g:\tsplib\samples\atsp\sp\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h \
	g:\tsplib\samples\atsp\sp\atsp.h \
	g:\tsplib\samples\atsp\sp\config.h


ATSP_DEP = g:\tsplib\samples\atsp\sp\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h \
	g:\tsplib\samples\atsp\sp\atsp.h \
	g:\tsplib\samples\atsp\sp\atspint.h \
	g:\tsplib\samples\atsp\sp\config.h \
	g:\tsplib\samples\atsp\sp\talkdrop.h


TALKDROP_DEP = g:\tsplib\samples\atsp\sp\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h \
	g:\tsplib\samples\atsp\sp\atsp.h \
	g:\tsplib\samples\atsp\sp\talkdrop.h


ATSP_RCDEP = g:\tsplib\samples\atsp\sp\phone.ico \
	g:\tsplib\samples\atsp\sp\atsp.r2c


UI_DEP = g:\tsplib\samples\atsp\sp\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h \
	g:\tsplib\samples\atsp\sp\atsp.h \
	g:\tsplib\samples\atsp\sp\config.h


REQUEST_DEP = g:\tsplib\samples\atsp\sp\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h \
	g:\tsplib\samples\atsp\sp\atsp.h \
	g:\tsplib\samples\atsp\sp\atspint.h \
	g:\tsplib\samples\atsp\sp\talkdrop.h


LINE_DEP = g:\tsplib\samples\atsp\sp\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h \
	g:\tsplib\samples\atsp\sp\atsp.h \
	g:\tsplib\samples\atsp\sp\atspint.h \
	g:\tsplib\samples\atsp\sp\talkdrop.h


all:	$(PROJ).DLL

STDAFX.OBJ:	STDAFX.CPP $(STDAFX_DEP)
	$(CPP) $(CFLAGS) $(CPPCREATEPCHFLAG) /c STDAFX.CPP

CONFIG.OBJ:	CONFIG.CPP $(CONFIG_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c CONFIG.CPP

ATSP.OBJ:	ATSP.CPP $(ATSP_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c ATSP.CPP

TALKDROP.OBJ:	TALKDROP.CPP $(TALKDROP_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c TALKDROP.CPP

ATSP.RES:	ATSP.RC $(ATSP_RCDEP)
	$(RC) $(RCFLAGS) $(RCDEFINES) -r ATSP.RC

UI.OBJ:	UI.CPP $(UI_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c UI.CPP

REQUEST.OBJ:	REQUEST.CPP $(REQUEST_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c REQUEST.CPP

LINE.OBJ:	LINE.CPP $(LINE_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c LINE.CPP


$(PROJ).DLL::	ATSP.RES

$(PROJ).DLL::	STDAFX.OBJ CONFIG.OBJ ATSP.OBJ TALKDROP.OBJ UI.OBJ REQUEST.OBJ LINE.OBJ $(OBJS_EXT) $(DEFFILE)
	echo >NUL @<<$(PROJ).CRF
STDAFX.OBJ +
CONFIG.OBJ +
ATSP.OBJ +
TALKDROP.OBJ +
UI.OBJ +
REQUEST.OBJ +
LINE.OBJ +
$(OBJS_EXT)
$(PROJ).DLL
$(MAPFILE)
c:\progra~1\msvc\lib\+
c:\progra~1\msvc\mfc\lib\+
$(LIBS)
$(DEFFILE);
<<
	link $(LFLAGS) @$(PROJ).CRF
	$(RC) $(RESFLAGS) ATSP.RES $@
	@copy $(PROJ).CRF MSVC.BND
	implib /nowep $(PROJ).LIB $(PROJ).DLL

$(PROJ).DLL::	ATSP.RES
	if not exist MSVC.BND 	$(RC) $(RESFLAGS) ATSP.RES $@

run: $(PROJ).DLL
	$(PROJ) $(RUNFLAGS)


$(PROJ).BSC: $(SBRS)
	bscmake @<<
/o$@ $(SBRS)
<<
