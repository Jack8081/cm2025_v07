@echo off

::--------------------------------------------------------
::-- Input param
::-- %1: elf file
::-- %2: tool dir
::--------------------------------------------------------
set "ELF=%~dpnx1"
set "BIN=%~dp1\..\%~n1.bin"
set "LST=%~dp1\..\%~n1.lst"
set "TOOL=%~2\ARMCC\bin"
set "OUTDIR=%~dp1\..\..\..\"

:: Output bin
"%TOOL%\fromelf" --bin --output="%BIN%" "%ELF%"

:: Output list
"%TOOL%\fromelf" --text -a -c --output="%LST%" "%ELF%"

:: Copy bin
copy /B /Y "%BIN%" "%OUTDIR%"
