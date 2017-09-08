# Microsoft Visual C++ generated build script - Do not modify

PROJ = DSSPEXE
DEBUG = 1
PROGTYPE = 0
CALLER = 
ARGS = 
DLLS = 
D_RCDEFINES = /d_DEBUG 
R_RCDEFINES = /dNDEBUG 
ORIGIN = MSVC
ORIGIN_VER = 1.00
PROJPATH = C:\BUILD\SAMPLES\DSSP\APP\
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
CFLAGS_D_WEXE = /nologo /Gs /G2 /Zp1 /W4 /WX /Gf /Zi /AL /Od /D "_DEBUG" /I "..\..\..\include" /I "..\..\..\include" /I "..\sp" /GA /Fd"EMULATOR.PDB" 
CFLAGS_R_WEXE = /nologo /Gs /G3 /Zp1 /W4 /WX /Gf /AL /O1 /D "NDEBUG" /I "g:\pinglib\include" /I "..\..\..\include" /I "..\sp" /GA 
LFLAGS_D_WEXE = /NOLOGO /NOD /NOE /PACKC:61440 /STACK:12405 /ALIGN:16 /ONERROR:NOEXE /CO /MAP /LINE  
LFLAGS_R_WEXE = /NOLOGO /NOD /NOE /PACKC:61440 /STACK:10240 /ALIGN:16 /ONERROR:NOEXE /MAP  
LIBS_D_WEXE = lafxcwd ..\sp\dssp.lib oldnames libw llibcew ..\sp\dssp.lib commdlg.lib mmsystem.lib shell.lib 
LIBS_R_WEXE = lafxcw g:\pinglib\lib\pinglib.lib oldnames libw llibcew ..\sp\dssp.lib commdlg.lib mmsystem.lib 
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
		ABOUT.SBR \
		CTL3D.SBR \
		DDX.SBR \
		DIAL.SBR \
		PROGBAR.SBR \
		TTBUTT.SBR


STDAFX_DEP = c:\build\samples\dssp\app\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	c:\build\samples\dssp\app\controls.h


EMULATOR_DEP = c:\build\samples\dssp\app\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	c:\build\samples\dssp\app\controls.h \
	c:\build\samples\dssp\app\colorlb.h \
	c:\build\samples\dssp\app\objects.h \
	c:\build\samples\dssp\app\baseprop.h \
	c:\build\samples\dssp\app\call.h \
	c:\build\samples\dssp\app\dial.h \
	c:\build\samples\dssp\app\confgdlg.h \
	c:\build\samples\dssp\app\gendlg.h \
	c:\build\samples\dssp\app\addrset.h \
	c:\build\samples\dssp\app\gensetup.h \
	c:\build\samples\dssp\app\gentone.h \
	c:\build\samples\dssp\app\about.h \
	c:\build\samples\dssp\app\emulator.h \
	c:\build\samples\dssp\sp\dsspint.h


LAMP_DEP = c:\build\samples\dssp\app\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	c:\build\samples\dssp\app\controls.h \
	c:\build\samples\dssp\app\objects.h


EMULATOR_RCDEP = c:\build\samples\dssp\app\res\hold.bmp \
	c:\build\samples\dssp\app\res\release.bmp \
	c:\build\samples\dssp\app\res\dial.bmp \
	c:\build\samples\dssp\app\res\incoming.bmp \
	c:\build\samples\dssp\app\res\busy.bmp \
	c:\build\samples\dssp\app\res\answer.bmp \
	c:\build\samples\dssp\app\res\generate.bmp \
	c:\build\samples\dssp\app\res\exit.bmp \
	c:\build\samples\dssp\app\res\config.bmp \
	c:\build\samples\dssp\app\res\reset.bmp \
	c:\build\samples\dssp\app\res\left.bmp \
	c:\build\samples\dssp\app\res\right.bmp \
	c:\build\samples\dssp\app\res\digits.bmp \
	c:\build\samples\dssp\app\res\helpme.bmp \
	c:\build\samples\dssp\app\res\emulator.ico \
	c:\build\samples\dssp\app\res\emulator.rc2


CALL_DEP = c:\build\samples\dssp\app\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	c:\build\samples\dssp\app\controls.h \
	c:\build\samples\dssp\app\call.h \
	c:\sdk\msvc\include\tapi.h


COLORLB_DEP = c:\build\samples\dssp\app\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	c:\build\samples\dssp\app\controls.h \
	c:\build\samples\dssp\app\colorlb.h


CONFGDLG_DEP = c:\build\samples\dssp\app\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	c:\build\samples\dssp\app\controls.h \
	c:\build\samples\dssp\app\objects.h \
	c:\build\samples\dssp\app\colorlb.h \
	c:\build\samples\dssp\app\emulator.h \
	c:\build\samples\dssp\app\confgdlg.h


GENDLG_DEP = c:\build\samples\dssp\app\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	c:\build\samples\dssp\app\controls.h \
	c:\build\samples\dssp\app\objects.h \
	c:\build\samples\dssp\app\colorlb.h \
	c:\build\samples\dssp\app\emulator.h \
	c:\build\samples\dssp\app\gendlg.h


ADDRSET_DEP = c:\build\samples\dssp\app\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	c:\build\samples\dssp\app\controls.h \
	c:\build\samples\dssp\app\objects.h \
	c:\build\samples\dssp\app\colorlb.h \
	c:\build\samples\dssp\app\emulator.h \
	c:\build\samples\dssp\app\addrset.h


GENSETUP_DEP = c:\build\samples\dssp\app\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	c:\build\samples\dssp\app\controls.h \
	c:\build\samples\dssp\app\objects.h \
	c:\build\samples\dssp\app\colorlb.h \
	c:\build\samples\dssp\app\emulator.h \
	c:\build\samples\dssp\app\gensetup.h


BASEPROP_DEP = c:\build\samples\dssp\app\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	c:\build\samples\dssp\app\controls.h \
	c:\build\samples\dssp\app\baseprop.h


OBJECTS_DEP = c:\build\samples\dssp\app\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	c:\build\samples\dssp\app\controls.h \
	c:\build\samples\dssp\app\colorlb.h \
	c:\build\samples\dssp\app\objects.h \
	c:\build\samples\dssp\app\emulator.h \
	c:\build\samples\dssp\app\gendlg.h


DRV_DEP = c:\build\samples\dssp\app\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	c:\build\samples\dssp\app\controls.h \
	c:\build\samples\dssp\app\colorlb.h \
	c:\build\samples\dssp\app\objects.h \
	c:\build\samples\dssp\app\baseprop.h \
	c:\build\samples\dssp\app\emulator.h \
	c:\build\samples\dssp\app\call.h \
	c:\build\samples\dssp\app\dial.h \
	c:\build\samples\dssp\app\confgdlg.h \
	c:\build\samples\dssp\app\gendlg.h \
	c:\build\samples\dssp\app\addrset.h \
	c:\build\samples\dssp\app\gensetup.h \
	c:\build\samples\dssp\sp\dsspint.h


GENTONE_DEP = c:\build\samples\dssp\app\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	c:\build\samples\dssp\app\controls.h \
	c:\build\samples\dssp\app\objects.h \
	c:\build\samples\dssp\app\colorlb.h \
	c:\build\samples\dssp\app\emulator.h \
	c:\build\samples\dssp\app\gentone.h


ABOUT_DEP = c:\build\samples\dssp\app\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	c:\build\samples\dssp\app\controls.h \
	c:\build\samples\dssp\app\colorlb.h \
	c:\build\samples\dssp\app\objects.h \
	c:\build\samples\dssp\app\baseprop.h \
	c:\build\samples\dssp\app\emulator.h \
	c:\build\samples\dssp\app\about.h


CTL3D_DEP = c:\build\samples\dssp\app\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	c:\build\samples\dssp\app\controls.h


DDX_DEP = c:\build\samples\dssp\app\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	c:\build\samples\dssp\app\controls.h


DIAL_DEP = c:\build\samples\dssp\app\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	c:\build\samples\dssp\app\controls.h \
	c:\build\samples\dssp\app\objects.h \
	c:\build\samples\dssp\app\colorlb.h \
	c:\build\samples\dssp\app\emulator.h \
	c:\build\samples\dssp\app\dial.h


PROGBAR_DEP = c:\build\samples\dssp\app\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	c:\build\samples\dssp\app\controls.h


TTBUTT_DEP = c:\build\samples\dssp\app\stdafx.h \
	c:\sdk\msvc\include\ctl3d.h \
	c:\build\samples\dssp\app\controls.h


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

CTL3D.OBJ:	CTL3D.CPP $(CTL3D_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c CTL3D.CPP

DDX.OBJ:	DDX.CPP $(DDX_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c DDX.CPP

DIAL.OBJ:	DIAL.CPP $(DIAL_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c DIAL.CPP

PROGBAR.OBJ:	PROGBAR.CPP $(PROGBAR_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c PROGBAR.CPP

TTBUTT.OBJ:	TTBUTT.CPP $(TTBUTT_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c TTBUTT.CPP


$(PROJ).EXE::	EMULATOR.RES

$(PROJ).EXE::	STDAFX.OBJ EMULATOR.OBJ LAMP.OBJ CALL.OBJ COLORLB.OBJ CONFGDLG.OBJ \
	GENDLG.OBJ ADDRSET.OBJ GENSETUP.OBJ BASEPROP.OBJ OBJECTS.OBJ DRV.OBJ GENTONE.OBJ \
	ABOUT.OBJ CTL3D.OBJ DDX.OBJ DIAL.OBJ PROGBAR.OBJ TTBUTT.OBJ $(OBJS_EXT) $(DEFFILE)
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
CTL3D.OBJ +
DDX.OBJ +
DIAL.OBJ +
PROGBAR.OBJ +
TTBUTT.OBJ +
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
