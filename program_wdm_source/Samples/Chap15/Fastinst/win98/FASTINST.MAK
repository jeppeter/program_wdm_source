# Microsoft Visual C++ generated build script - Do not modify

PROJ = FASTINST
DEBUG = 0
PROGTYPE = 0
CALLER = 
ARGS = d:\wdmbook\chap8\wdmidle\sys\device.inf
DLLS = 
D_RCDEFINES = -d_DEBUG
R_RCDEFINES = -dNDEBUG
ORIGIN = MSVC
ORIGIN_VER = 1.00
PROJPATH = T:\NEWBOOK\CHAP15\FASTINST\WIN98\
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
CFLAGS_D_WEXE = /nologo /G2 /W3 /Zi /AM /Od /D "_DEBUG" /I "c:\98ddk\inc\win98\inc16" /I "c:\98ddk\inc\win98" /FR /GA /Fd"FASTINST.PDB" 
CFLAGS_R_WEXE = /nologo /W3 /AM /O1 /D "NDEBUG" /I "c:\98ddk\inc\win98\inc16" /I "c:\98ddk\inc\win98" /FR /GA 
LFLAGS_D_WEXE = /NOLOGO /NOD /PACKC:61440 /STACK:10240 /ALIGN:16 /ONERROR:NOEXE /CO  
LFLAGS_R_WEXE = /NOLOGO /NOD /PACKC:61440 /STACK:10240 /ALIGN:16 /ONERROR:NOEXE  
LIBS_D_WEXE = mafxcwd mlibcew oldnames libw setupx ctl3dv2 commdlg.lib olecli.lib olesvr.lib shell.lib 
LIBS_R_WEXE = mafxcw mlibcew oldnames libw setupx ctl3dv2 commdlg.lib olecli.lib olesvr.lib shell.lib 
RCFLAGS = /nologo
RESFLAGS = /nologo
RUNFLAGS = 
DEFFILE = FASTINST.DEF
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
		SETUP.SBR \
		FASTINST.SBR \
		GAUGE.SBR \
		PROGRESS.SBR


STDAFX_DEP = t:\newbook\chap15\fastinst\win98\stdafx.h


SETUP_DEP = t:\newbook\chap15\fastinst\win98\stdafx.h \
	t:\newbook\chap15\fastinst\win98\setup.h


FASTINST_DEP = t:\newbook\chap15\fastinst\win98\stdafx.h \
	t:\newbook\chap15\fastinst\win98\setup.h \
	t:\newbook\chap15\fastinst\win98\progress.h \
	t:\newbook\chap15\fastinst\win98\gauge.h \
	t:\newbook\chap15\fastinst\win98\ctl3d.h


GAUGE_DEP = t:\newbook\chap15\fastinst\win98\stdafx.h \
	t:\newbook\chap15\fastinst\win98\gauge.h


PROGRESS_DEP = t:\newbook\chap15\fastinst\win98\stdafx.h \
	t:\newbook\chap15\fastinst\win98\progress.h \
	t:\newbook\chap15\fastinst\win98\gauge.h


all:	$(PROJ).EXE $(PROJ).BSC

STDAFX.OBJ:	STDAFX.CPP $(STDAFX_DEP)
	$(CPP) $(CFLAGS) $(CPPCREATEPCHFLAG) /c STDAFX.CPP

SETUP.OBJ:	SETUP.CPP $(SETUP_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c SETUP.CPP

FASTINST.OBJ:	FASTINST.CPP $(FASTINST_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c FASTINST.CPP

GAUGE.OBJ:	GAUGE.CPP $(GAUGE_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c GAUGE.CPP

PROGRESS.OBJ:	PROGRESS.CPP $(PROGRESS_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c PROGRESS.CPP

FASTINST.RES:	FASTINST.RC $(FASTINST_RCDEP)
	$(RC) $(RCFLAGS) $(RCDEFINES) -r FASTINST.RC


$(PROJ).EXE::	FASTINST.RES

$(PROJ).EXE::	STDAFX.OBJ SETUP.OBJ FASTINST.OBJ GAUGE.OBJ PROGRESS.OBJ $(OBJS_EXT) $(DEFFILE)
	echo >NUL @<<$(PROJ).CRF
STDAFX.OBJ +
SETUP.OBJ +
FASTINST.OBJ +
GAUGE.OBJ +
PROGRESS.OBJ +
$(OBJS_EXT)
$(PROJ).EXE
$(MAPFILE)
c:\msvc15\lib\+
c:\msvc15\mfc\lib\+
c:\98ddk\lib\win98\+
$(LIBS)
$(DEFFILE);
<<
	link $(LFLAGS) @$(PROJ).CRF
	$(RC) $(RESFLAGS) FASTINST.RES $@
	@copy $(PROJ).CRF MSVC.BND

$(PROJ).EXE::	FASTINST.RES
	if not exist MSVC.BND 	$(RC) $(RESFLAGS) FASTINST.RES $@

run: $(PROJ).EXE
	$(PROJ) $(RUNFLAGS)


$(PROJ).BSC: $(SBRS)
	bscmake @<<
/o$@ $(SBRS)
<<
