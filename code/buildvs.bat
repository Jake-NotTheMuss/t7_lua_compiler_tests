@echo off

setlocal
set COMPILECMD=cl /nologo /MD /O2 /W3 /c /D_CRT_SECURE_NO_DEPRECATE /DWITH_LOGGING
set LINKCMD=link /nologo

set EXITSTATUS=0

set ProgramFilesX86=%ProgramFiles(x86)%
if not exist "%ProgramFilesX86%" set ProgramFilesX86=%ProgramFiles%

set vs_where=%ProgramFilesX86%\Microsoft Visual Studio\Installer\vswhere.exe
if not exist "%vs_where%" goto FAIL

for /f "usebackq tokens=1* delims=: " %%i in (`"%vs_where%" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64`) do (
	if /i "%%i"=="installationPath" set VS_InstallDir=%%j
)

echo VS_InstallDir="%VS_InstallDir%"

if "%VS_InstallDir%"=="" (
	echo.
	echo Visual Studio is detected but no suitable installation was found.
	echo.
	goto FAIL
)

set VCVARS=%VS_InstallDir%\VC\Auxiliary\Build\vcvarsall.bat
if exist "%VCVARS%" (
	echo calling "%VCVARS%" x64
	call "%VCVARS%" x64
) else (
	echo "%VCVARS%" not found
	goto FAIL
)

REM try the c++ compiler
echo Testing for the C/C++ Compiler
cl 2> NUL 1>&2
if not errorlevel 0 (
	echo Visual Studio C/C++ Compiler not found
	goto FAIL
)

echo Visual Studio C/C++ Compiler found

REM build the DLL

set modroot=%cd%
pushd %~dp0\src

%COMPILECMD% compiler.c
if errorlevel 0 (
%LINKCMD% /DLL /out:"%modroot%\zone\T7LuaCompiler.dll" compiler.obj
)
if not errorlevel 0 set EXITSTATUS=1

del compiler.obj

popd

exit /b %EXITSTATUS%

:FAIL
exit /b 1
