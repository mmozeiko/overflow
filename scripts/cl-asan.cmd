@echo off

set CL_CACHE=%~dp0ccl.cache.cmd
set CCL=%~dp0ccl.cmd

if exist %CL_CACHE% (
  call %CL_CACHE%
) else (
  call %CCL% 1>nul 2>nul
)

%*
