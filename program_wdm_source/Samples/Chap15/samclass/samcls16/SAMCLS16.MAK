# Microsoft Visual C++ generated build script - Do not modify

PROJ = SAMCLS16
DEBUG = 0
PROGTYPE = 1
CALLER = 
ARGS = 
DLLS = 
D_RCDEFINES = -d_DEBUG
R_RCDEFINES = -dNDEBUG
ORIGIN = MSVC
ORIGIN_VER = 1.00
PROJPATH = T:\NEWBOOK\CHAP15\SAMCLASS\SAMCLS16\
USEMFC = 0
CC = cl
CPP = cl
CXX = cl
CCREATEPCHFLAG = 
CPPCREATEPCHFLAG = 
CUSEPCHFLAG = 
CPPUSEPCHFLAG = 
FIRSTC =             
FIRSTCPP = SAMCLS16.CPP
RC = rc
CFLAGS_D_WDLL = /nologo /G2 /W3 /Zi /ALw /Od /D "_DEBUG" /I "c:\98ddk\inc\win98\inc16" /I "c:\98ddk\inc\win98" /FR /GD /Fd"SAMCLS16.PDB"
CFLAGS_R_WDLL = /nologo /W3 /ALw /O1 /D "NDEBUG" /I "c:\98ddk\inc\win98\inc16" /I "c:\98ddk\inc\win98" /FR /GD 
LFLAGS_D_WDLL = /NOLOGO /NOD /NOE /PACKC:61440 /ALIGN:16 /ONERROR:NOEXE /CO /MAP:FULL
LFLAGS_R_WDLL = /NOLOGO /NOD /NOE /PACKC:61440 /ALIGN:16 /ONERROR:NOEXE /MAP:FULL
LIBS_D_WDLL = oldnames libw ldllcew commctrl commdlg.lib olecli.lib olesvr.lib shell.lib setupx.lib
LIBS_R_WDLL = oldnames libw ldllcew commctrl commdlg.lib olecli.lib olesvr.lib shell.lib setupx.lib
RCFLAGS = /nologo
RESFLAGS = /nologo
RUNFLAGS = 
DEFFILE = SAMCLS16.DEF
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
SBRS = SAMCLS16.SBR


SAMCLS16_RCDEP = t:\newbook\chap15\samclass\samcls16\samicon.ico


all:	$(PROJ).DLL $(PROJ).BSC

SAMCLS16.OBJ:	SAMCLS16.CPP $(SAMCLS16_DEP)
	$(CPP) $(CFLAGS) $(CPPCREATEPCHFLAG) /c SAMCLS16.CPP

SAMCLS16.RES:	SAMCLS16.RC $(SAMCLS16_RCDEP)
	$(RC) $(RCFLAGS) $(RCDEFINES) -r SAMCLS16.RC


$(PROJ).DLL::	SAMCLS16.RES

$(PROJ).DLL::	SAMCLS16.OBJ $(OBJS_EXT) $(DEFFILE)
	echo >NUL @<<$(PROJ).CRF
SAMCLS16.OBJ +
$(OBJS_EXT)
$(PROJ).DLL
$(MAPFILE)
c:\msvc15\lib\+
c:\msvc15\mfc\lib\+
c:\98ddk\lib\win98\+
$(LIBS)
$(DEFFILE);
<<
	link $(LFLAGS) @$(PROJ).CRF
	$(RC) $(RESFLAGS) SAMCLS16.RES $@
	@copy $(PROJ).CRF MSVC.BND
	implib /nowep $(PROJ).LIB $(PROJ).DLL

$(PROJ).DLL::	SAMCLS16.RES
	if not exist MSVC.BND 	$(RC) $(RESFLAGS) SAMCLS16.RES $@

run: $(PROJ).DLL
	$(PROJ) $(RUNFLAGS)


$(PROJ).BSC: $(SBRS)
	bscmake @<<
/o$@ $(SBRS)
<<
