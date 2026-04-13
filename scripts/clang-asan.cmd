@echo off

for /f "tokens=3  delims=. " %%v in ('clang -v 2^>^&1 ^| findstr version')      do set CLANG_VERSION=%%v
for /f "tokens=1* delims= "  %%v in ('clang -v 2^>^&1 ^| findstr InstalledDir') do set CLANG_DIR=%%w

set PATH=%CLANG_DIR%\..\lib\clang\%CLANG_VERSION%\lib\windows;%PATH%

%*
