@echo off
setlocal enabledelayedexpansion

set CL_CACHE=%~dp0ccl.cache.cmd

if exist %CL_CACHE% (
  call %CL_CACHE%
) else (
  set __VSCMD_ARG_NO_LOGO=1
  set VSCMD_SKIP_SENDTELEMETRY=1
  for /f "tokens=*" %%i in ('"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.VisualStudio.Workload.NativeDesktop -property installationPath') do set VS=%%i
  call "!VS!\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 -startdir=none -no_logo
  echo set PATH=!PATH!>        %CL_CACHE%
  echo set INCLUDE=!INCLUDE!>> %CL_CACHE%
  echo set LIB=!LIB!>>         %CL_CACHE%
)

set CL=%CL% -nologo
set LINK=%LINK% -INCREMENTAL:NO
cl.exe %*
