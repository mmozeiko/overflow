@echo off
setlocal enabledelayedexpansion

if exist %~dp0ccl.cache.cmd call %~dp0ccl.cache.cmd

where /Q dumpbin.exe || (
  echo dumpbin.exe not found
  exit /b 1
)

for /F "usebackq tokens=1 delims= " %%i in (`dumpbin.exe /headers %1 ^| find "size of image"`) do set SIZE=%%i
for /F "usebackq tokens=1 delims= " %%i in (`dumpbin.exe /headers %1 ^| find "date stamp"`) do set DATESTAMP=%%i
for /F "usebackq tokens=8-10 delims={}, " %%i in (`dumpbin.exe /headers %1 ^| find "RSDS"`) do (
  set GUID=%%i
  set VERSION=%%j
  set PDB=%%~nxk
)

rem TODO
rem timestamp must be exactly 8 digits, with leading 0'es
rem size must be without leading 0'es
rem all hex in uppercase

rem to uncompress:
rem makecab.exe /D CompressionType=LZX /D CompressionMemory=21 file.pdb
rem expand.exe file.pd_ file.pdb

rem dump pdb guid:
rem "C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\dbh.exe" file.pdb info | findstr PdbSig70

echo https://msdl.microsoft.com/download/symbols/%~nx1/%DATESTAMP%%SIZE%/%~nx1
echo https://msdl.microsoft.com/download/symbols/%PDB%/%GUID:-=%%VERSION%/%PDB%
