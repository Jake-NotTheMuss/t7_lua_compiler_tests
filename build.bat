@echo off

setlocal

cd "%~dp0"

set ME=%0

set EXITSTATUS=0

set BUILD_MOD=0
set BUILD_DLL=0

REM parse arguments
:argv_loop
if not "%1" == "" (
	if "%1" == "help" (
		echo Usage: %ME% [target]
		echo.
		echo Targets:
		echo   mod       Only build the mod
		echo   dll       Only build the DLL
		echo.
		echo If no target is specified, all targets are built
		goto EOF
	) else if "%1" == "mod" (
		set BUILD_MOD=1
	) else if "%1" == "dll" (
		set BUILD_DLL=1
	) else (
		echo Command "%1" unknown, aborting!
		goto ERR
	)
	shift /1
	goto argv_loop
)

if "%BUILD_MOD%"=="0" (
	if "%BUILD_DLL%"=="0" (
		set BUILD_MOD=1
		set BUILD_DLL=1
	)
)

if "%BUILD_MOD%"=="0" goto DLL

for %%i in (.) do set fs_game=%%~nxi

echo Linking mod "%fs_game%"
echo.

"%TA_TOOLS_PATH%/bin/linker_modtools.exe" -language english ^
-fs_game %fs_game% -modsource core_mod

echo.

if not errorlevel 0 goto FAIL

:DLL

if "%BUILD_DLL%"=="0" goto SUCCESS

echo Building DLL
echo.

call "%~dp0\code\buildvs.bat"

echo.

if not errorlevel 0 goto FAIL

goto SUCCESS

:FAIL
echo Build failed
:ERR
set EXITSTATUS=1
goto EOF

:SUCCESS
echo Build succeeded

:EOF
echo %CMDCMDLINE% | findstr /i /c:"%~nx0" && set dopause=1
if DEFINED dopause pause
exit /b %EXITSTATUS%
