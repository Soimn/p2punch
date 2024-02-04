@echo off
setlocal

if "%Platform%" neq "x64" (
    echo ERROR: Platform is not "x64" - please run this from the MSVC x64 native tools command prompt.
    exit /b 1
)

cd %~dp0
if not exist build mkdir build
cd build

if "%1" neq "" (
	echo Illegal number of arguments^, expected^: build
	goto end
)

cl /nologo /Od /Oi /Zo /Z7 /RTC1 /GS- /W3 ..\src\platform_layer.c /link /fixed /subsystem:windows /out:p2punch.exe /pdb:p2punch_player_layer.pdb /incremental:no /opt:icf /opt:ref libvcruntime.lib user32.lib gdi32.lib

:end
endlocal
