# Microsoft Developer Studio Project File - Name="wdmcheck" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=wdmcheck - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "wdmcheck.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "wdmcheck.mak" CFG="wdmcheck - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "wdmcheck - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "wdmcheck - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "wdmcheck - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Ox /I "." /I "$(98DDK)\inc\win98" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_X86_" /D "BLD_COFF" /D "IS_32" /FR /Yu"stdvxd.h" /Fd"wdmcheck.pdb" /FD -Gs -Zipl /c
# ADD CPP /nologo /Gz /W3 /Ox /I "." /I "$(98DDK)\inc\win98" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_X86_" /D "BLD_COFF" /D "IS_32" /FR /Yu"stdvxd.h" /Fd"wdmcheck.pdb" /FD -Gs -Zipl /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "$(ddkpath)\inc" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 vxdwraps.clb wdmvxd.clb /nologo /subsystem:windows /pdb:"wdmcheck.pdb" /debug /machine:I386 /nodefaultlib /out:"release\wdmcheck.vxd" /libpath:"$(98DDK)\lib\i386\free" -ignore:4078 -ignore:4039 -ignore:4075 -merge:.CRT=_IDATA -merge:.xdata=_LDATA -vxd
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 vxdwraps.clb wdmvxd.clb /nologo /subsystem:windows /pdb:"wdmcheck.pdb" /map /debug /machine:I386 /nodefaultlib /out:"release\wdmcheck.vxd" /libpath:"$(98DDK)\lib\i386\free" -ignore:4078 -ignore:4039 -ignore:4075 -merge:.CRT=_IDATA -merge:.xdata=_LDATA -vxd
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Finishing up...
IntDir=.\Release
TargetPath=.\release\wdmcheck.vxd
TargetName=wdmcheck
InputPath=.\release\wdmcheck.vxd
SOURCE="$(InputPath)"

"$(IntDir)\$(TargetName).nms" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	$(98DDK)\bin\win98\bin16\rc -r -i $(ddkpath)\inc -Fo$(IntDir)\version.res version.rc 
	$(98DDK)\bin\win98\adrc2vxd $(TargetPath) $(IntDir)\version.res 
	$(SIWPATH)\nmsym -translate:source,package,always $(TargetPath) 
	copy $(TargetPath) $(WINDIR)\system 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "wdmcheck - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Od /I "." /I "$(98DDK)\inc\win98" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_X86_" /D "BLD_COFF" /D "IS_32" /D DEBLEVEL=1 /D "DEBUG" /FR /Yu"stdvxd.h" /Fd"wdmcheck.pdb" /FD -Gs -Zipl -Odb2 /c
# ADD CPP /nologo /Gz /W3 /Od /I "." /I "$(98DDK)\inc\win98" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_X86_" /D "BLD_COFF" /D "IS_32" /D DEBLEVEL=1 /D "DEBUG" /FR /Yu"stdvxd.h" /Fd"wdmcheck.pdb" /FD -Gs -Zipl -Odb2 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "$(ddkpath)\inc" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 vxdwraps.clb wdmvxd.clb /nologo /subsystem:windows /incremental:no /pdb:"wdmcheck.pdb" /debug /machine:I386 /nodefaultlib /out:"debug\wdmcheck.vxd" /pdbtype:sept /libpath:"$(98DDK)\lib\i386\free" -ignore:4078 -ignore:4039 -ignore:4075 -merge:.CRT=_IDATA -merge:.xdata=_LDATA -vxd
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 vxdwraps.clb wdmvxd.clb /nologo /subsystem:windows /incremental:no /pdb:"wdmcheck.pdb" /debug /machine:I386 /nodefaultlib /out:"debug\wdmcheck.vxd" /pdbtype:sept /libpath:"$(98DDK)\lib\i386\free" -ignore:4078 -ignore:4039 -ignore:4075 -merge:.CRT=_IDATA -merge:.xdata=_LDATA -vxd
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Finishing up...
IntDir=.\Debug
TargetPath=.\debug\wdmcheck.vxd
TargetName=wdmcheck
InputPath=.\debug\wdmcheck.vxd
SOURCE="$(InputPath)"

"$(IntDir)\$(TargetName).nms" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	$(98DDK)\bin\win98\bin16\rc -r -i $(ddkpath)\inc -Fo$(IntDir)\version.res version.rc 
	$(98DDK)\bin\win98\adrc2vxd $(TargetPath) $(IntDir)\version.res 
	$(SIWPATH)\nmsym -translate:source,package,always $(TargetPath) 
	copy $(TargetPath) $(WINDIR)\system 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "wdmcheck - Win32 Release"
# Name "wdmcheck - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\stdvxd.cpp
# ADD CPP /Yc"stdvxd.h"
# End Source File
# Begin Source File

SOURCE=.\version.rc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\wdmcheck.cpp
# End Source File
# Begin Source File

SOURCE=.\wdmcheck.def
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\crs.h
# End Source File
# Begin Source File

SOURCE=.\ioctls.h
# End Source File
# Begin Source File

SOURCE=.\stdvxd.h
# End Source File
# Begin Source File

SOURCE=.\version.h
# End Source File
# Begin Source File

SOURCE=..\..\..\98ddk\inc\win98\Vmm.h
# End Source File
# Begin Source File

SOURCE=..\..\..\98ddk\inc\win98\Vwin32.h
# End Source File
# Begin Source File

SOURCE=.\wdmcheck.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\DevCtrl.asm

!IF  "$(CFG)" == "wdmcheck - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Assembling...
IntDir=.\Release
InputPath=.\DevCtrl.asm
InputName=DevCtrl

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	$(98DDK)\bin\win98\ml -c -coff -DBLD_COFF -DIS_32 -DMASM6 -W2 -Cx -Fo$(IntDir)\$(InputName).obj -Zi -DDEBUG -DDEBLEVEL=1 -I$(98DDK)\inc\win98 $(InputPath) 
	editbin $(IntDir)\$(InputName).obj -section:_EBSS=.bss$1 -section:_INIT=.CRT$XCA -section:_EINIT=.CRT$XCZ 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "wdmcheck - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Assembling...
IntDir=.\Debug
InputPath=.\DevCtrl.asm
InputName=DevCtrl

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	$(98DDK)\bin\win98\ml -c -coff -DBLD_COFF -DIS_32 -DMASM6 -W2 -Cx -Fo$(IntDir)\$(InputName).obj -Zi -DDEBUG -DDEBLEVEL=1 -I$(98DDK)\inc\win98 $(InputPath) 
	editbin $(IntDir)\$(InputName).obj -section:_EBSS=.bss$1 -section:_INIT=.CRT$XCA -section:_EINIT=.CRT$XCZ 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\version.inc
# End Source File
# End Target
# End Project
