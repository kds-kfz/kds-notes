$ErrorActionPreference = "Stop"

$strRoot = Split-Path -Parent $PSScriptRoot
$strBuildDir = Join-Path $strRoot "tests\build"
New-Item -ItemType Directory -Force -Path $strBuildDir | Out-Null

$strVsWhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
$strVsRoot = & $strVsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if ([string]::IsNullOrWhiteSpace($strVsRoot))
{
	throw "Visual Studio C++ toolchain not found"
}

$strVcVars = Join-Path $strVsRoot "VC\Auxiliary\Build\vcvars64.bat"
$strTestSource = Join-Path $strRoot "tests\SocketServerMultiInstanceRuntime.cpp"
$strTestObj = Join-Path $strBuildDir "SocketServerMultiInstanceRuntime.obj"
$strTestExe = Join-Path $strBuildDir "SocketServerMultiInstanceRuntime.exe"
$strCompile = 'call "{0}" && cl /nologo /EHsc /std:c++14 /W4 /WX /I"{1}" /Fo"{2}" "{3}" /link /LIBPATH:"{4}" libSocketServer.lib ws2_32.lib /OUT:"{5}"' -f `
	$strVcVars, (Join-Path $strRoot "include"), $strTestObj, $strTestSource, (Join-Path $strRoot "lib\x64vc14"), $strTestExe
cmd.exe /d /c $strCompile
if ($LASTEXITCODE -ne 0)
{
	throw "SocketServer multi-instance test build failed: $LASTEXITCODE"
}

$aRuntimePath = @(
	(Join-Path $strRoot "lib\x64vc14"),
	(Join-Path $strRoot "vender\hpsocket\lib\x64vc14"),
	(Join-Path $strRoot "vender\nsdk\lib\x64vc14"),
	(Join-Path $strRoot "vender\thread\lib\x64vc14"),
	$env:PATH
)
$strOldPath = $env:PATH
try
{
	$env:PATH = $aRuntimePath -join ";"
	& $strTestExe
	if ($LASTEXITCODE -ne 0)
	{
		throw "SocketServer multi-instance runtime test failed: $LASTEXITCODE"
	}
}
finally
{
	$env:PATH = $strOldPath
}

Write-Host "SocketServer multi-instance checks passed"
