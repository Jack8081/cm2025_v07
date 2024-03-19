@echo off
setlocal ENABLEDELAYEDEXPANSION

set ROOT_DIR=%~dp0
set SUBMOD_ROOT=^
zephyr,^
framework\bluetooth,^
framework\media

@echo;
@echo --== git cleanup ==--
@echo;
west forall -c "git reset --hard"
west forall -c "git clean -d -f -x"

@echo;
@echo --== git update ==--
@echo;
git fetch
git rebase
west update

@echo;
@echo --== submodule update ==--
@echo;
:loop_submod
for /f "tokens=1,* delims=," %%a in ("%SUBMOD_ROOT%") do (
	set SUBMOD_ROOT=%%b
	set subdir=%ROOT_DIR%\..\%%a
	cd !subdir!
	@echo;
	@echo --== !subdir! update ==--
	@echo;
	git submodule init
	git submodule update 2>nul
	goto :loop_submod
)

cd %ROOT_DIR%
