@echo off
setlocal enabledelayedexpansion

set CL_CACHE=%~dp0ccl.cache.cmd

if exist %CL_CACHE% (
  call %CL_CACHE%
) else (
  set VSCMD_SKIP_SENDTELEMETRY=1
  call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
  echo set PATH=!PATH!>        %CL_CACHE%
  echo set INCLUDE=!INCLUDE!>> %CL_CACHE%
  echo set LIB=!LIB!>>         %CL_CACHE%
)

set LINK=%LINK% -INCREMENTAL:NO
cl.exe -nologo %*
