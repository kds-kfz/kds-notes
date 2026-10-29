@echo off
setlocal EnableExtensions

set "ROOT=%~dp0"
set "VSROOT=D:\vs2022az\Microsoft Visual Studio\2022\Community"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if exist "%VSWHERE%" (
	for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do set "VSROOT=%%I"
)

set "MSBUILD=%VSROOT%\MSBuild\Current\Bin\MSBuild.exe"
if not exist "%MSBUILD%" (
	echo [ERROR] MSBuild was not found: %MSBUILD%
	exit /b 1
)

echo [INFO] Building and running thread lifecycle tests...
"%MSBUILD%" "%ROOT%tests\ThreadLifecycleTests\ThreadLifecycleTests.vcxproj" /t:Rebuild /p:Configuration=Release /p:Platform=x64 /m
if errorlevel 1 exit /b 1
"%ROOT%Temp\Tests\x64\Release\ThreadLifecycleTests.exe"
if errorlevel 1 exit /b 1

echo [INFO] Building MtAssistant Release x64...
"%MSBUILD%" "%ROOT%project\vs2022\MtAssistant\MtAssistant.sln" /t:Rebuild /p:Configuration=Release /p:Platform=x64 /m
if errorlevel 1 exit /b 1

set "PACKAGE=%ROOT%Tags\MtAssistant_TAG_1.0.0.3"
if not exist "%PACKAGE%" mkdir "%PACKAGE%"
copy /Y "%ROOT%bin\x64vc14\MtAssistant.exe" "%PACKAGE%\MtAssistant.exe" >nul
if errorlevel 1 exit /b 1
copy /Y "%ROOT%bin\x64vc14\MtAssistant.pdb" "%PACKAGE%\MtAssistant.pdb" >nul
if errorlevel 1 exit /b 1
copy /Y "%ROOT%bin\x64vc14\MtAssistant.xml" "%PACKAGE%\MtAssistant.xml" >nul
if errorlevel 1 exit /b 1

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%ROOT%tools\verify_package.ps1" -PackageDir "%PACKAGE%" -VisualStudioRoot "%VSROOT%"
if errorlevel 1 exit /b 1

echo [SUCCESS] MtAssistant 1.0.0.3 was built, tested and packaged.
exit /b 0
