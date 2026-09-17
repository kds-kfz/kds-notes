$ErrorActionPreference = "Stop"

$strRoot = Split-Path -Parent $PSScriptRoot
$strHttpHeader = Get-Content -LiteralPath (Join-Path $strRoot "src\HttpAsynReqObj.h") -Raw -Encoding UTF8
$strHttpSource = Get-Content -LiteralPath (Join-Path $strRoot "src\HttpAsynReqObj.cpp") -Raw -Encoding UTF8
$strListener = Get-Content -LiteralPath (Join-Path $strRoot "src\HttpServerListerNet.cpp") -Raw -Encoding UTF8
$strPublicHeader = Get-Content -LiteralPath (Join-Path $strRoot "include\SocketServer.h") -Raw -Encoding UTF8

function Assert-Contains
{
	param([string]$p_strText, [string]$p_strMarker, [string]$p_strName)
	if (-not $p_strText.Contains($p_strMarker))
	{
		throw "$p_strName marker missing: $p_strMarker"
	}
}

Assert-Contains $strHttpHeader "std::vector<ST_HTTP_QUERY_PARAM> m_vecQueryParam" "contiguous query storage"
if ($strHttpHeader.Contains("m_mapQueryParam"))
{
	throw "query parser must not use std::map storage"
}
$nParseBegin = $strHttpSource.IndexOf("EN_HTTP_QUERY_PARSE_RESULT CHttpAsynReqObj::ParseQueryString")
if ($nParseBegin -lt 0)
{
	throw "single-pass query parser is missing"
}
$strParseBlock = $strHttpSource.Substring($nParseBegin)
foreach ($strForbidden in @(".substr(", "m_mapQueryParam"))
{
	if ($strParseBlock.Contains($strForbidden))
	{
		throw "query parser contains allocation-heavy marker: $strForbidden"
	}
}
Assert-Contains $strParseBlock "uiIndex <= m_strQueryString.size()" "single-pass state machine"
Assert-Contains $strListener "GetUrlField(dwConnID, HUF_QUERY)" "HP-Socket parsed query reuse"
Assert-Contains $strListener "HTTP_MAX_RAW_URL_BYTES = 16 * 1024" "raw URL limit"
Assert-Contains $strListener "HTTP_MAX_QUERY_PARAM_COUNT = 128" "query parameter limit"

$nMethodType = $strPublicHeader.IndexOf("virtual const char* GetMethodType() = 0;")
$nSendResponse = $strPublicHeader.IndexOf("virtual bool SendResponse(")
$nRawUrl = $strPublicHeader.IndexOf("virtual const char* GetRawUrl() = 0;")
if ($nMethodType -lt 0 -or $nSendResponse -le $nMethodType -or $nRawUrl -le $nSendResponse)
{
	throw "ABI 2 HTTP virtual method order is not preserved"
}
Assert-Contains $strPublicHeader "#define SOCKET_SERVER_ABI_VERSION 7U" "SocketServer ABI"
Assert-Contains $strPublicHeader "WebSockGetPendingDataLength" "WebSocket pending data length ABI"
Assert-Contains $strPublicHeader "SetSocketListenQueue" "Socket listen queue ABI"

$strBuildDir = Join-Path $strRoot "tests\build"
New-Item -ItemType Directory -Force -Path $strBuildDir | Out-Null
$strVsWhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
$strVsRoot = & $strVsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if ([string]::IsNullOrWhiteSpace($strVsRoot))
{
	throw "Visual Studio C++ toolchain not found"
}
$strVcVars = Join-Path $strVsRoot "VC\Auxiliary\Build\vcvars64.bat"
$strTestSource = Join-Path $strRoot "tests\SocketServerHttpQueryRuntime.cpp"
$strTestExe = Join-Path $strBuildDir "SocketServerHttpQueryRuntimeAbi4.exe"
$strCompile = 'call "{0}" && cl /nologo /EHsc /std:c++14 /I"{1}" "{2}" /link /LIBPATH:"{3}" libSocketServer.lib ws2_32.lib /OUT:"{4}"' -f `
	$strVcVars, (Join-Path $strRoot "include"), $strTestSource, (Join-Path $strRoot "lib\x64vc14"), $strTestExe
cmd.exe /d /c $strCompile
if ($LASTEXITCODE -ne 0)
{
	throw "HTTP query runtime test build failed: $LASTEXITCODE"
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
	$clStartInfo = New-Object System.Diagnostics.ProcessStartInfo
	$clStartInfo.FileName = $strTestExe
	$clStartInfo.UseShellExecute = $false
	$clStartInfo.CreateNoWindow = $true
	$clStartInfo.RedirectStandardOutput = $true
	$clStartInfo.RedirectStandardError = $true
	$clProcess = New-Object System.Diagnostics.Process
	$clProcess.StartInfo = $clStartInfo
	if (-not $clProcess.Start())
	{
		throw "HTTP query runtime test process failed to start"
	}
	# 异步读取两条输出流，避免进程输出超过管道缓冲区后互相等待。
	$clStdOutTask = $clProcess.StandardOutput.ReadToEndAsync()
	$clStdErrTask = $clProcess.StandardError.ReadToEndAsync()
	if (-not $clProcess.WaitForExit(30000))
	{
		$clProcess.Kill()
		$clProcess.WaitForExit()
		throw "HTTP query runtime test timed out`nstdout:`n$($clStdOutTask.Result)`nstderr:`n$($clStdErrTask.Result)"
	}
	$clProcess.WaitForExit()
	$strRuntimeOut = $clStdOutTask.Result
	$strRuntimeErr = $clStdErrTask.Result
	Write-Host $strRuntimeOut
	if ($clProcess.ExitCode -ne 0)
	{
		throw "HTTP query runtime test failed: $($clProcess.ExitCode)`n$strRuntimeErr"
	}
}
finally
{
	$env:PATH = $strOldPath
}

Write-Host "SocketServer HTTP query parser checks passed"
