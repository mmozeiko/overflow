@echo off
setlocal enabledelayedexpansion

rem For msvc have Visual Studio with desktop development C++ workload installed.
rem For clang have clang.exe available in path.

rem For linux/mingw/wasm builds have WSL with Ubuntu 24.04 installed: https://learn.microsoft.com/en-us/windows/wsl/install
rem Then install packages listed in xrun.sh file

set NAME=%1 && shift
set ARGS=%*

for /f "tokens=*" %%i in ('"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.VisualStudio.Workload.NativeDesktop -property installationPath') do set VS=%%~si
if "%VS%" equ "" (
  echo ERROR: Visual Studio installation not found
  exit /b 1
)
set VSDEVCMD=%VS%\Common7\Tools\VsDevCmd.bat
set WSL_DISTRO=Ubuntu-24.04

set WSL=wsl.exe -d %WSL_DISTRO%

if "%CLANG%" equ "" set CLANG=clang
if "%GCC%"   equ "" set GCC=gcc

if "%PROCESSOR_ARCHITECTURE%" equ "AMD64" set HOST_ARCH=x64
if "%PROCESSOR_ARCHITECTURE%" equ "ARM64" set HOST_ARCH=arm64

set CL_FLAGS=-O2 -W3 -WX

set CFLAGS=-O2 -Wall -Wextra -Werror -ffp-contract=off
set LDFLAGS=-static -lm

set CLANG_ARCH_arm64=arm64
set CLANG_ARCH_x64=x86_64

set OS_VALUES=windows linux mingw wasi
set ARCH_VALUES=x64 arm64 rv64 wasm32
set CC_VALUES=msvc clang gcc

call :xrun windows msvc  x64    || exit /b 1
call :xrun windows clang x64    || exit /b 1
call :xrun windows msvc  arm64  || exit /b 1
call :xrun windows clang arm64  || exit /b 1
call :xrun linux   gcc   x64    || exit /b 1
call :xrun linux   clang x64    || exit /b 1
call :xrun linux   gcc   arm64  || exit /b 1
call :xrun linux   clang arm64  || exit /b 1
call :xrun linux   gcc   rv64   || exit /b 1
call :xrun linux   clang rv64   || exit /b 1
call :xrun mingw   gcc   x64    || exit /b 1
call :xrun mingw   clang x64    || exit /b 1
call :xrun wasi    clang wasm32 || exit /b 1

echo === OK ===
goto :eof


:xrun
set OS=%1
set CC=%2
set ARCH=%3

for %%x in ("%NAME%") do set OUTPUT=%%~nx_%OS%_%CC%_%ARCH%.exe
set NORUN=0

for %%a in (%ARGS%) do for %%x in (%OS_VALUES%)   do  if "%%x" equ "%%~a" if "%%x" neq "%OS%"   goto :eof
for %%a in (%ARGS%) do for %%x in (%CC_VALUES%)   do  if "%%x" equ "%%~a" if "%%x" neq "%CC%"   goto :eof
for %%a in (%ARGS%) do for %%x in (%ARCH_VALUES%) do  if "%%x" equ "%%~a" if "%%x" neq "%ARCH%" goto :eof

echo ===== %OS% %ARCH% %CC% =====

if "%CC%" equ "msvc" (
  set BUILD=%CL_FLAGS%
  for %%x in (%ARGS%) do (
    set X=%%x
    if "!X!" equ "c"      set BUILD=!BUILD! -TC
    if "!X!" equ "c++"    set BUILD=!BUILD! -TP
    if "!X!" equ "sse4"   set BUILD=!BUILD! -arch:SSE4.2
    if "!X:~0,1!" equ "-" set BUILD=!BUILD! %%x
  )
) else (
  set BUILD=%CFLAGS%
  for %%x in (%ARGS%) do (
    set X=%%x
    if "!X!" equ "c"      set BUILD=!BUILD! -x c
    if "!X!" equ "c++"    set BUILD=!BUILD! -x c++
    if "!X!" equ "sse4"   set BUILD=!BUILD! -msse4.2
    if "!X:~0,1!" equ "-" set BUILD=!BUILD! %%x
  )
)
set BUILD=!BUILD! !NAME!

if "%OS%" equ "windows" (

  if "%CC%" equ "msvc" (
    set BUILD=cmd.exe /C "%VSDEVCMD% -arch=%ARCH% -host_arch=%HOST_ARCH% -no_logo -startdir=none && cl 2>&1 | findstr Version && cl.exe -nologo !BUILD! -Fe%OUTPUT% -link -incremental:no"
  ) else if "%CC%" equ "clang" (
    set BUILD=%CLANG%.exe -fuse-ld=lld -target !CLANG_ARCH_%ARCH%!-pc-windows-msvc !BUILD! -o %OUTPUT%
    %CLANG%.exe --version | findstr version
  )
  set RUN=%OUTPUT%

  for %%a in (%ARGS%) do if "%%a" equ "norun" set NORUN=1

  rem cannot run arm64 on x64
  if "%ARCH%_%PROCESSOR_ARCHITECTURE%" equ "arm64_AMD64" set NORUN=1

) else if "%OS%" equ "linux" (

  set ARCH_VALUE=
  if "%ARCH%" equ "x64"   set ARCH_VALUE=x86_64
  if "%ARCH%" equ "rv64"  set ARCH_VALUE=riscv64
  if "%ARCH%" equ "arm64" set ARCH_VALUE=aarch64

  set TARGET="!ARCH_VALUE!-linux-gnu"
  if "%CC%" equ "gcc"   set BUILD=!TARGET!-%GCC% !BUILD!
  if "%CC%" equ "clang" set BUILD=%CLANG% -fuse-ld=lld -target !TARGET! !BUILD!

  set RUN=
  if "%ARCH%" neq "%HOST_ARCH%" (
    set RUN=qemu-!ARCH_VALUE!-static
    if "%ARCH%" equ "rv64" set RUN=!RUN! -cpu rv64,v=true,vlen=128,elen=64,vext_spec=v1.0,rvv_ta_all_1s=true,rvv_ma_all_1s=true
    %WSL% bash -c "qemu-!ARCH_VALUE!-static --version | head -1" || exit / b1
  )

  for /f "tokens=1 delims= " %%a in ("!BUILD!") do %WSL% bash -c "%%a --version | head -1" || exit /b 1

  set BUILD=%WSL% !BUILD! !LDFLAGS! -o %OUTPUT%
  set RUN=%WSL% bash -c "!RUN! ./%OUTPUT%"

) else if "%OS%" equ "mingw" (

  if "%CC%" equ "gcc"   set BUILD=x86_64-w64-mingw32-%GCC% !BUILD!
  if "%CC%" equ "clang" set BUILD=%CLANG% -fuse-ld=lld -target x86_64-w64-mingw32 !BUILD!

  for /f "tokens=1 delims= " %%a in ("!BUILD!") do %WSL% bash -c "%%a --version | head -1" || exit /b 1

  set BUILD=%WSL% !BUILD! !LDFLAGS! -o %OUTPUT%
  set RUN=%OUTPUT%

) else if "%OS%" equ "wasi" (

  %WSL% bash -c "wasmtime --version" || exit /b 1
  for /f "tokens=1 delims= " %%a in ("!BUILD!") do %WSL% bash -c "%CLANG% --version | head -1" || exit /b 1

  set BUILD=%WSL% %CLANG% -fuse-ld=lld -target wasm32-wasi !BUILD! !LDFLAGS! -o %OUTPUT%
  set RUN=%WSL% wasmtime %OUTPUT%

)

!BUILD! || exit /b 1

if "!NORUN!" equ "1" (
  echo [compile only]
) else (
  %RUN% || exit /b 1
)

goto :eof
