# Microsoft Visual C++ generated build script - Do not modify

PROJ = EMULATOR
DEBUG = 0
PROGTYPE = 0
CALLER = 
ARGS = 
DLLS = 
D_RCDEFINES = /d_DEBUG 
R_RCDEFINES = /dNDEBUG 
ORIGIN = MSVC
ORIGIN_VER = 1.00
PROJPATH = G:\TSPLIB\SAMPLES\DSSP\EMULATOR\
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
CFLAGS_D_WEXE = /nologo /Gs /G2 /Zp1 /W4 /WX /Gf /Zi /AL /Od /D "_DEBUG" /I "g:\pinglib\include" /I "..\sp" /GA /Fd"EMULATOR.PDB" 
CFLAGS_R_WEXE = /nologo /Gs /G3 /Zp1 /W4 /WX /Gf /AL /O1 /D "NDEBUG" /I "g:\pinglib\include" /I "..\sp" /GA 
LFLAGS_D_WEXE = /NOLOGO /NOD /NOE /PACKC:61440 /STACK:10240 /ALIGN:16 /ONERROR:NOEXE /CO /MAP /LINE  
LFLAGS_R_WEXE = /NOLOGO /NOD /NOE /PACKC:61440 /STACK:10240 /ALIGN:16 /ONERROR:NOEXE /MAP  
LIBS_D_WEXE = lafxcwd g:\pinglib\lib\pinglibd.lib oldnames libw llibcew commdlg.lib mmsystem.lib shell.lib 
LIBS_R_WEXE = lafxcw g:\pinglib\lib\pinglib.lib oldnames libw llibcew commdlg.lib mmsystem.lib 
RCFLAGS = /z /ic:\projects\pinglib\include 
RESFLAGS = /t /31 
RUNFLAGS = 
DEFFILE = EMULATOR.DEF
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
		EMULATOR.SBR \
		LAMP.SBR \
		CALL.SBR \
		COLORLB.SBR \
		CONFGDLG.SBR \
		GENDLG.SBR \
		ADDRSET.SBR \
		GENSETUP.SBR \
		BASEPROP.SBR \
		OBJECTS.SBR \
		DRV.SBR \
		GENTONE.SBR \
		ABOUT.SBR


STDAFX_DEP = g:\tsplib\samples\dssp\emulator\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	g:\tsplib\samples\dssp\sp\emintf.h


EMULATOR_DEP = g:\tsplib\samples\dssp\emulator\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	g:\tsplib\samples\dssp\sp\emintf.h \
	g:\tsplib\samples\dssp\emulator\colorlb.h \
	g:\tsplib\samples\dssp\emulator\objects.h \
	g:\tsplib\samples\dssp\emulator\baseprop.h \
	g:\tsplib\samples\dssp\emulator\emulator.h \
	g:\tsplib\samples\dssp\emulator\call.h \
	g:\tsplib\samples\dssp\emulator\dial.h \
	g:\tsplib\samples\dssp\emulator\confgdlg.h \
	g:\tsplib\samples\dssp\emulator\gendlg.h \
	g:\tsplib\samples\dssp\emulator\addrset.h \
	g:\tsplib\samples\dssp\emulator\gensetup.h \
	g:\tsplib\samples\dssp\emulator\gentone.h \
	g:\tsplib\samples\dssp\emulator\about.h


LAMP_DEP = g:\tsplib\samples\dssp\emulator\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	g:\tsplib\samples\dssp\sp\emintf.h \
	g:\tsplib\samples\dssp\emulator\objects.h


EMULATOR_RCDEP = g:\tsplib\samples\dssp\emulator\res\hold.bmp \
	g:\tsplib\samples\dssp\emulator\res\release.bmp \
	g:\tsplib\samples\dssp\emulator\res\dial.bmp \
	g:\tsplib\samples\dssp\emulator\res\incoming.bmp \
	g:\tsplib\samples\dssp\emulator\res\busy.bmp \
	g:\tsplib\samples\dssp\emulator\res\answer.bmp \
	g:\tsplib\samples\dssp\emulator\res\generate.bmp \
	g:\tsplib\samples\dssp\emulator\res\exit.bmp \
	g:\tsplib\samples\dssp\emulator\res\config.bmp \
	g:\tsplib\samples\dssp\emulator\res\reset.bmp \
	g:\tsplib\samples\dssp\emulator\res\left.bmp \
	g:\tsplib\samples\dssp\emulator\res\right.bmp \
	g:\tsplib\samples\dssp\emulator\res\digits.bmp \
	g:\tsplib\samples\dssp\emulator\res\helpme.bmp \
	g:\tsplib\samples\dssp\emulator\res\emulator.ico \
	g:\tsplib\samples\dssp\emulator\res\emulator.rc2


CALL_DEP = g:\tsplib\samples\dssp\emulator\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	g:\tsplib\samples\dssp\sp\emintf.h \
	g:\tsplib\samples\dssp\emulator\call.h \
	c:\sdk\msvc\include\tapi.h


COLORLB_DEP = g:\tsplib\samples\dssp\emulator\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	g:\tsplib\samples\dssp\sp\emintf.h \
	g:\tsplib\samples\dssp\emulator\colorlb.h


CONFGDLG_DEP = g:\tsplib\samples\dssp\emulator\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	g:\tsplib\samples\dssp\sp\emintf.h \
	g:\tsplib\samples\dssp\emulator\objects.h \
	g:\tsplib\samples\dssp\emulator\colorlb.h \
	g:\tsplib\samples\dssp\emulator\emulator.h \
	g:\tsplib\samples\dssp\emulator\confgdlg.h


GENDLG_DEP = g:\tsplib\samples\dssp\emulator\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	g:\tsplib\samples\dssp\sp\emintf.h \
	g:\tsplib\samples\dssp\emulator\objects.h \
	g:\tsplib\samples\dssp\emulator\colorlb.h \
	g:\tsplib\samples\dssp\emulator\emulator.h \
	g:\tsplib\samples\dssp\emulator\gendlg.h


ADDRSET_DEP = g:\tsplib\samples\dssp\emulator\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	g:\tsplib\samples\dssp\sp\emintf.h \
	g:\tsplib\samples\dssp\emulator\objects.h \
	g:\tsplib\samples\dssp\emulator\colorlb.h \
	g:\tsplib\samples\dssp\emulator\emulator.h \
	g:\tsplib\samples\dssp\emulator\addrset.h


GENSETUP_DEP = g:\tsplib\samples\dssp\emulator\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	g:\tsplib\samples\dssp\sp\emintf.h \
	g:\tsplib\samples\dssp\emulator\objects.h \
	g:\tsplib\samples\dssp\emulator\colorlb.h \
	g:\tsplib\samples\dssp\emulator\emulator.h \
	g:\tsplib\samples\dssp\emulator\gensetup.h


BASEPROP_DEP = g:\tsplib\samples\dssp\emulator\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	g:\tsplib\samples\dssp\sp\emintf.h \
	g:\tsplib\samples\dssp\emulator\baseprop.h


OBJECTS_DEP = g:\tsplib\samples\dssp\emulator\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	g:\tsplib\samples\dssp\sp\emintf.h \
	g:\tsplib\samples\dssp\emulator\colorlb.h \
	g:\tsplib\samples\dssp\emulator\objects.h \
	g:\tsplib\samples\dssp\emulator\emulator.h \
	g:\tsplib\samples\dssp\emulator\gendlg.h


DRV_DEP = g:\tsplib\samples\dssp\emulator\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	g:\tsplib\samples\dssp\sp\emintf.h \
	g:\tsplib\samples\dssp\emulator\colorlb.h \
	g:\tsplib\samples\dssp\emulator\objects.h \
	g:\tsplib\samples\dssp\emulator\baseprop.h \
	g:\tsplib\samples\dssp\emulator\emulator.h \
	g:\tsplib\samples\dssp\emulator\call.h \
	g:\tsplib\samples\dssp\emulator\dial.h \
	g:\tsplib\samples\dssp\emulator\confgdlg.h \
	g:\tsplib\samples\dssp\emulator\gendlg.h \
	g:\tsplib\samples\dssp\emulator\addrset.h \
	g:\tsplib\samples\dssp\emulator\gensetup.h


GENTONE_DEP = g:\tsplib\samples\dssp\emulator\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	g:\tsplib\samples\dssp\sp\emintf.h \
	g:\tsplib\samples\dssp\emulator\objects.h \
	g:\tsplib\samples\dssp\emulator\colorlb.h \
	g:\tsplib\samples\dssp\emulator\emulator.h \
	g:\tsplib\samples\dssp\emulator\gentone.h


ABOUT_DEP = g:\tsplib\samples\dssp\emulator\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	g:\tsplib\samples\dssp\sp\emintf.h \
	g:\tsplib\samples\dssp\emulator\colorlb.h \
	g:\tsplib\samples\dssp\emulator\objects.h \
	g:\tsplib\samples\dssp\emulator\baseprop.h \
	g:\tsplib\samples\dssp\emulator\emulator.h \
	g:\tsplib\samples\dssp\emulator\about.h


all:	$(PROJ).EXE

STDAFX.OBJ:	STDAFX.CPP $(STDAFX_DEP)
	$(CPP) $(CFLAGS) $(CPPCREATEPCHFLAG) /c STDAFX.CPP

EMULATOR.OBJ:	EMULATOR.CPP $(EMULATOR_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c EMULATOR.CPP

LAMP.OBJ:	LAMP.CPP $(LAMP_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c LAMP.CPP

EMULATOR.RES:	EMULATOR.RC $(EMULATOR_RCDEP)
	$(RC) $(RCFLAGS) $(RCDEFINES) -r EMULATOR.RC

CALL.OBJ:	CALL.CPP $(CALL_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c CALL.CPP

COLORLB.OBJ:	COLORLB.CPP $(COLORLB_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c COLORLB.CPP

CONFGDLG.OBJ:	CONFGDLG.CPP $(CONFGDLG_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c CONFGDLG.CPP

GENDLG.OBJ:	GENDLG.CPP $(GENDLG_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c GENDLG.CPP

ADDRSET.OBJ:	ADDRSET.CPP $(ADDRSET_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c ADDRSET.CPP

GENSETUP.OBJ:	GENSETUP.CPP $(GENSETUP_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c GENSETUP.CPP

BASEPROP.OBJ:	BASEPROP.CPP $(BASEPROP_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c BASEPROP.CPP

OBJECTS.OBJ:	OBJECTS.CPP $(OBJECTS_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c OBJECTS.CPP

DRV.OBJ:	DRV.CPP $(DRV_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c DRV.CPP

GENTONE.OBJ:	GENTONE.CPP $(GENTONE_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c GENTONE.CPP

ABOUT.OBJ:	ABOUT.CPP $(ABOUT_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c ABOUT.CPP


$(PROJ).EXE::	EMULATOR.RES

$(PROJ).EXE::	STDAFX.OBJ EMULATOR.OBJ LAMP.OBJ CALL.OBJ COLORLB.OBJ CONFGDLG.OBJ \
	GENDLG.OBJ ADDRSET.OBJ GENSETUP.OBJ BASEPROP.OBJ OBJECTS.OBJ DRV.OBJ GENTONE.OBJ \
	ABOUT.OBJ $(OBJS_EXT) $(DEFFILE)
	echo >NUL @<<$(PROJ).CRF
STDAFX.OBJ +
EMULATOR.OBJ +
LAMP.OBJ +
CALL.OBJ +
COLORLB.OBJ +
CONFGDLG.OBJ +
GENDLG.OBJ +
ADDRSET.OBJ +
GENSETUP.OBJ +
BASEPROP.OBJ +
OBJECTS.OBJ +
DRV.OBJ +
GENTONE.OBJ +
ABOUT.OBJ +
$(OBJS_EXT)
$(PROJ).EXE
$(MAPFILE)
c:\sdk\msvc\lib\+
c:\sdk\msvc\mfc\lib\+
$(LIBS)
$(DEFFILE);
<<
	link $(LFLAGS) @$(PROJ).CRF
	$(RC) $(RESFLAGS) EMULATOR.RES $@
	@copy $(PROJ).CRF MSVC.BND

$(PROJ).EXE::	EMULATOR.RES
	if not exist MSVC.BND 	$(RC) $(RESFLAGS) EMULATOR.RES $@

run: $(PROJ).EXE
	$(PROJ) $(RUNFLAGS)


$(PROJ).BSC: $(SBRS)
	bscmake @<<
/o$@ $(SBRS)
<<
