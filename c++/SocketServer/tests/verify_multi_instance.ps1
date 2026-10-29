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
	$strVcVars, (Join-Path $strRoot "include"), $strTestObj, $strTestSource, (Join-Path $strRoot "lib\x64vc14\dynamic"), $strTestExe
cmd.exe /d /c $strCompile
if ($LASTEXITCODE -ne 0)
{
	throw "SocketServer multi-instance test build failed: $LASTEXITCODE"
}

# 静态封装库必须由真实消费程序完成最终链接，单独生成 .lib 不能证明头文件宏、CRT 和下层依赖完整。
# SocketServer 静态库统一使用 /MD；HP-Socket 本体仍使用官方 DLL，所以最终程序同时链接其导入库并部署 HPSocket.dll。
$strStaticObj = Join-Path $strBuildDir "SocketServerMultiInstanceStatic.obj"
$strStaticExe = Join-Path $strBuildDir "SocketServerMultiInstanceStatic.exe"
$strStaticCompile = 'call "{0}" && cl /nologo /MD /EHsc /std:c++14 /W4 /WX /DSOCKETSERVER_STATIC /I"{1}" /Fo"{2}" "{3}" /link /LTCG /LIBPATH:"{4}" /LIBPATH:"{5}" /LIBPATH:"{6}" libSocketServer_static.lib HPSocket.lib libssl.lib libcrypto.lib bcrypt.lib crypt32.lib secur32.lib ws2_32.lib /OUT:"{7}"' -f `
	$strVcVars, (Join-Path $strRoot "include"), $strStaticObj, $strTestSource, `
	(Join-Path $strRoot "lib\x64vc14\static"), (Join-Path $strRoot "vender\hpsocket\lib\x64vc14\static"), `
	(Join-Path $strRoot "vender\openssl\lib\x64vc14\dynamic"), $strStaticExe
cmd.exe /d /c $strStaticCompile
if ($LASTEXITCODE -ne 0)
{
	throw "SocketServer static consumer test build failed: $LASTEXITCODE"
}

# 动态与静态测试共用隔离运行目录；显式部署全部运行 DLL，避免从系统 PATH 混入其他版本。
$aRuntimeFiles = @(
	"$strRoot\lib\x64vc14\dynamic\libSocketServer.dll"
	"$strRoot\vender\hpsocket\lib\x64vc14\dynamic\HPSocket.dll"
	"$strRoot\vender\openssl\lib\x64vc14\dynamic\libssl-3-x64.dll"
	"$strRoot\vender\openssl\lib\x64vc14\dynamic\libcrypto-3-x64.dll"
)
Copy-Item -LiteralPath $aRuntimeFiles -Destination $strBuildDir -Force

$aRuntimePath = @(
	"$strRoot\lib\x64vc14\dynamic"
	"$strRoot\vender\hpsocket\lib\x64vc14\dynamic"
	"$strRoot\vender\openssl\lib\x64vc14\dynamic"
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
	& $strStaticExe
	if ($LASTEXITCODE -ne 0)
	{
		throw "SocketServer static consumer runtime test failed: $LASTEXITCODE"
	}
}
finally
{
	$env:PATH = $strOldPath
}

Write-Host "SocketServer dynamic and static multi-instance checks passed"
