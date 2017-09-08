# Microsoft Visual C++ generated build script - Do not modify

PROJ = SPLIBD
DEBUG = 1
PROGTYPE = 4
CALLER = 
ARGS = 
DLLS = 
D_RCDEFINES = -d_DEBUG
R_RCDEFINES = -dNDEBUG
ORIGIN = MSVC
ORIGIN_VER = 1.00
PROJPATH = E:\TSPLIB\16BIT\SRC\
USEMFC = 1
CC = cl
CPP = cl
CXX = cl
CCREATEPCHFLAG = 
CPPCREATEPCHFLAG = 
CUSEPCHFLAG = 
CPPUSEPCHFLAG = 
FIRSTC =             
FIRSTCPP = CALL.CPP    
RC = rc
CFLAGS_D_LIB = /nologo /Gs /G2 /W4 /WX /Gf /Zd /ALw /Od /D "_DEBUG" /I "..\INCLUDE" /GD /GEf 
CFLAGS_R_LIB = /nologo /Gs /G3 /W4 /WX /ALw /Ox /D "NDEBUG" /GD /GEf 
RCFLAGS = /nologo
RESFLAGS = /nologo
RUNFLAGS = 
OBJS_EXT = 
LIBS_EXT = 
!if "$(DEBUG)" == "1"
CFLAGS = $(CFLAGS_D_LIB)
LFLAGS = 
LIBS = 
MAPFILE = nul
RCDEFINES = $(D_RCDEFINES)
!else
CFLAGS = $(CFLAGS_R_LIB)
LFLAGS = 
LIBS = 
MAPFILE = nul
RCDEFINES = $(R_RCDEFINES)
!endif
!if [if exist MSVC.BND del MSVC.BND]
!endif
SBRS = ADDRESS.SBR \
		BUTTINFO.SBR \
		CALL.SBR \
		CONFCALL.SBR \
		CONN.SBR \
		DEVICE.SBR \
		DISPLAY.SBR \
		LINECONN.SBR \
		MISC.SBR \
		PHONECON.SBR \
		REQUEST.SBR \
		SP.SBR \
		SPDLL.SBR \
		SPLINE.SBR \
		SPPHONE.SBR \
		SPTHREAD.SBR


ADDRESS_DEP = e:\tsplib\16bit\src\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h


BUTTINFO_DEP = e:\tsplib\16bit\src\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h


CALL_DEP = e:\tsplib\16bit\src\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h


CONFCALL_DEP = e:\tsplib\16bit\src\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h


CONN_DEP = e:\tsplib\16bit\src\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h


DEVICE_DEP = e:\tsplib\16bit\src\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h


DISPLAY_DEP = e:\tsplib\16bit\src\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h


LINECONN_DEP = e:\tsplib\16bit\src\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h


MISC_DEP = e:\tsplib\16bit\src\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h


PHONECON_DEP = e:\tsplib\16bit\src\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h


REQUEST_DEP = e:\tsplib\16bit\src\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h


SP_DEP = e:\tsplib\16bit\src\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h \
	c:\progra~1\msvc\mfc\src\auxdata.h


SPDLL_DEP = e:\tsplib\16bit\src\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h


SPLINE_DEP = e:\tsplib\16bit\src\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h


SPPHONE_DEP = e:\tsplib\16bit\src\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h


SPTHREAD_DEP = e:\tsplib\16bit\src\stdafx.h \
	c:\progra~1\msvc\include\tspi.h \
	c:\progra~1\msvc\include\tapi.h


all:	$(PROJ).LIB

ADDRESS.OBJ:	ADDRESS.CPP $(ADDRESS_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c ADDRESS.CPP

BUTTINFO.OBJ:	BUTTINFO.CPP $(BUTTINFO_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c BUTTINFO.CPP

CALL.OBJ:	CALL.CPP $(CALL_DEP)
	$(CPP) $(CFLAGS) $(CPPCREATEPCHFLAG) /c CALL.CPP

CONFCALL.OBJ:	CONFCALL.CPP $(CONFCALL_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c CONFCALL.CPP

CONN.OBJ:	CONN.CPP $(CONN_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c CONN.CPP

DEVICE.OBJ:	DEVICE.CPP $(DEVICE_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c DEVICE.CPP

DISPLAY.OBJ:	DISPLAY.CPP $(DISPLAY_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c DISPLAY.CPP

LINECONN.OBJ:	LINECONN.CPP $(LINECONN_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c LINECONN.CPP

MISC.OBJ:	MISC.CPP $(MISC_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c MISC.CPP

PHONECON.OBJ:	PHONECON.CPP $(PHONECON_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c PHONECON.CPP

REQUEST.OBJ:	REQUEST.CPP $(REQUEST_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c REQUEST.CPP

SP.OBJ:	SP.CPP $(SP_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c SP.CPP

SPDLL.OBJ:	SPDLL.CPP $(SPDLL_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c SPDLL.CPP

SPLINE.OBJ:	SPLINE.CPP $(SPLINE_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c SPLINE.CPP

SPPHONE.OBJ:	SPPHONE.CPP $(SPPHONE_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c SPPHONE.CPP

SPTHREAD.OBJ:	SPTHREAD.CPP $(SPTHREAD_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c SPTHREAD.CPP

$(PROJ).LIB::	ADDRESS.OBJ BUTTINFO.OBJ CALL.OBJ CONFCALL.OBJ CONN.OBJ DEVICE.OBJ \
	DISPLAY.OBJ LINECONN.OBJ MISC.OBJ PHONECON.OBJ REQUEST.OBJ SP.OBJ SPDLL.OBJ SPLINE.OBJ \
	SPPHONE.OBJ SPTHREAD.OBJ $(OBJS_EXT)
	echo >NUL @<<$(PROJ).CRF
$@ /PAGESIZE:64
y
+ADDRESS.OBJ &
+BUTTINFO.OBJ &
+CALL.OBJ &
+CONFCALL.OBJ &
+CONN.OBJ &
+DEVICE.OBJ &
+DISPLAY.OBJ &
+LINECONN.OBJ &
+MISC.OBJ &
+PHONECON.OBJ &
+REQUEST.OBJ &
+SP.OBJ &
+SPDLL.OBJ &
+SPLINE.OBJ &
+SPPHONE.OBJ &
+SPTHREAD.OBJ &
;
<<
	if exist $@ del $@
	lib @$(PROJ).CRF

$(PROJ).BSC: $(SBRS)
	bscmake @<<
/o$@ $(SBRS)
<<
