[CmdletBinding()]
param(
	[ValidateRange(1, 1000)]
	[int]$Cycles = 8,
	[ValidateRange(0, 256)]
	[int]$HttpWorkers = 12,
	[ValidateRange(0, 100000)]
	[int]$HttpRequestsPerWorker = 100,
	[ValidateRange(0, 256)]
	[int]$TcpWorkers = 8,
	[ValidateRange(0, 100000)]
	[int]$TcpMessagesPerWorker = 200,
	[ValidateRange(0, 256)]
	[int]$WebWorkers = 8,
	[ValidateRange(0, 100000)]
	[int]$WebMessagesPerWorker = 100
)

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
$strSource = Join-Path $strRoot "tests\SocketServerStabilityRuntime.cpp"
$strObject = Join-Path $strBuildDir "SocketServerStabilityRuntime.obj"
$strExe = Join-Path $strBuildDir "SocketServerStabilityRuntime.exe"
$strCompile = 'call "{0}" && cl /nologo /MD /O2 /EHsc /std:c++14 /utf-8 /W4 /WX /I"{1}" /Fo"{2}" "{3}" /link /LIBPATH:"{4}" libSocketServer.lib ws2_32.lib psapi.lib /OUT:"{5}"' -f `
	$strVcVars, (Join-Path $strRoot "include"), $strObject, $strSource, `
	(Join-Path $strRoot "lib\x64vc14"), $strExe
cmd.exe /d /c $strCompile
if ($LASTEXITCODE -ne 0)
{
	throw "SocketServer stability test build failed: $LASTEXITCODE"
}

$strOldPath = $env:PATH
try
{
	$env:PATH = @(
		(Join-Path $strRoot "lib\x64vc14"),
		(Join-Path $strRoot "vender\hpsocket\lib\x64vc14"),
		(Join-Path $strRoot "vender\thread\lib\x64vc14"),
		$strOldPath
	) -join ";"
	& $strExe $Cycles $HttpWorkers $HttpRequestsPerWorker `
		$TcpWorkers $TcpMessagesPerWorker $WebWorkers $WebMessagesPerWorker
	if ($LASTEXITCODE -ne 0)
	{
		throw "SocketServer stability runtime test failed: $LASTEXITCODE"
	}
}
finally
{
	$env:PATH = $strOldPath
}

Write-Host "SocketServer stability and resource-release checks passed"
