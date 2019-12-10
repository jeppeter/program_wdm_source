@echo off
if "%OS%" == "Windows_NT" goto :win2k

rem -----------------------------------------------------------

rem Special coding for Win98/Me systems. The version of rc.exe
rem supplied with the DDK will not run on back-level systems,
rem so we have to use one from an earlier DDK.

rem VCVARS32 is normally installed along with Visual Studio and
rem sets the path for the IDE. Modify or delete the following
rem line to match your own computer setup

call \vcvars32

set path=%wdmbook%\win98;%ddkpath%\bin\x86;%path%
goto :common

rem -----------------------------------------------------------

rem Special coding for Win2K/XP systems. The Visual Studio
rem setup program normally sets the path to include the IDE.
rem If you didn't let that happen, you'll have to adjust the
rem path at this point to include the IDE.

:win2k
set path=%ddkpath%\bin\x86;%path%

rem -----------------------------------------------------------

:common
set include=%ddkpath%\inc\wxp;%ddkpath%\inc\crt;%include%
set lib=%ddkpath%\lib\wxp\i386;%lib%
set basedir=%ddkpath%
msdev /useenv %1
exit
