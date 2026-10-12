param(
    [int]$BasePort = 39071,
    [int]$ClientsPerProtocol = 8,
    [int]$Rounds = 5,
    [string]$MsBuildPath = ""
)

$ErrorActionPreference = "Stop"

function Resolve-MsBuild {
    param([string]$ExplicitPath)

    if ($ExplicitPath) {
        if (-not (Test-Path -LiteralPath $ExplicitPath -PathType Leaf)) {
            throw "MSBuild does not exist: $ExplicitPath"
        }
        return (Resolve-Path -LiteralPath $ExplicitPath).Path
    }

    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path -LiteralPath $vswhere -PathType Leaf)) {
        throw "vswhere.exe was not found; specify MSBuild.exe with -MsBuildPath."
    }
    $resolved = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild `
        -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
    if (-not $resolved) {
        throw "MSBuild.exe was not found."
    }
    return $resolved
}

function Invoke-Build {
    param([string]$MsBuild, [string]$Solution)

    Write-Host "BUILD $Solution"
    & $MsBuild $Solution /t:Build /p:Configuration=Release /p:Platform=x64 /m /nologo
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed: $Solution"
    }
}

function Assert-SameFile {
    param([string]$Expected, [string]$Actual)

    if (-not (Test-Path -LiteralPath $Expected -PathType Leaf) -or
        -not (Test-Path -LiteralPath $Actual -PathType Leaf)) {
        throw "Dependency file is missing: $Expected or $Actual"
    }
    $expectedHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $Expected).Hash
    $actualHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $Actual).Hash
    if ($expectedHash -ne $actualHash) {
        throw "Output dependency is not the current build: $Actual"
    }
}

function Resolve-DumpBin {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    $resolved = & $vswhere -latest -products * `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -find "VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe" | Select-Object -First 1
    if (-not $resolved) {
        throw "dumpbin.exe was not found; PE imports cannot be verified."
    }
    return $resolved
}

function Assert-NoObsoleteRuntimeImport {
    param([string]$DumpBin, [string]$Binary)

    $imports = (& $DumpBin /nologo /dependents $Binary 2>&1 | Out-String)
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to read PE imports: $Binary"
    }
    if ($imports -match '(?i)libnsdk\.dll|libsnappy\.dll') {
        throw "Binary still imports nsdk/snappy dynamically: $Binary"
    }
}

function Invoke-DemoClient {
    param(
        [string]$Executable,
        [int]$Port,
        [int]$Clients,
        [int]$IterationCount
    )

    & $Executable "127.0.0.1" $Port $Clients $IterationCount
    if ($LASTEXITCODE -ne 0) {
        throw "demoClient integration failed with exit code $LASTEXITCODE."
    }
}

function Get-ServerResources {
    param([System.Diagnostics.Process]$Process)

    Start-Sleep -Milliseconds 500
    $Process.Refresh()
    return @{
        Handles = $Process.HandleCount
        PrivateBytes = $Process.PrivateMemorySize64
    }
}

if ($BasePort -lt 1 -or $BasePort -gt 65533) {
    throw "BasePort must be in 1..65533."
}
if ($ClientsPerProtocol -lt 1 -or $ClientsPerProtocol -gt 256) {
    throw "ClientsPerProtocol must be in 1..256."
}
if ($Rounds -lt 1 -or $Rounds -gt 100) {
    throw "Rounds must be in 1..100."
}

$cppRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..\..")).Path
$msbuild = Resolve-MsBuild -ExplicitPath $MsBuildPath
$dumpbin = Resolve-DumpBin
$socketServerSolution = Join-Path $cppRoot "SocketServer\project\vs2022\SocketServer\SocketServer.sln"
$socketClientSolution = Join-Path $cppRoot "SocketClient\project\vs2022\SocketClient\SocketClient.sln"
$echoSolution = Join-Path $cppRoot "demoServer\project\vs2022\SocketEchoServer\SocketEchoServer.sln"
$clientSolution = Join-Path $cppRoot "demoClient\project\vs2022\demoClient\demoClient.sln"

Invoke-Build -MsBuild $msbuild -Solution $socketServerSolution
Invoke-Build -MsBuild $msbuild -Solution $socketClientSolution
Invoke-Build -MsBuild $msbuild -Solution $echoSolution
Invoke-Build -MsBuild $msbuild -Solution $clientSolution

$serverOutput = Join-Path $cppRoot "demoServer\bin\socketecho\x64vc14"
$clientOutput = Join-Path $cppRoot "demoClient\bin\x64vc14"
$serverExe = Join-Path $serverOutput "SocketEchoServer.exe"
$clientExe = Join-Path $clientOutput "demoClient.exe"

Assert-SameFile `
    (Join-Path $cppRoot "SocketServer\lib\x64vc14\libSocketServer.dll") `
    (Join-Path $serverOutput "libSocketServer.dll")
Assert-SameFile `
    (Join-Path $cppRoot "SocketClient\lib\x64vc14\share\libSocketClient.dll") `
    (Join-Path $clientOutput "libSocketClient.dll")
Assert-SameFile `
    (Join-Path $cppRoot "SocketServer\vender\hpsocket\lib\x64vc14\HPSocket.dll") `
    (Join-Path $serverOutput "HPSocket.dll")
Assert-SameFile `
    (Join-Path $cppRoot "SocketClient\vender\hpsocket\lib\x64vc14\HPSocket.dll") `
    (Join-Path $clientOutput "HPSocket.dll")
Assert-SameFile `
    (Join-Path $cppRoot "SocketServer\vender\thread\lib\x64vc14\pthreadVC2.dll") `
    (Join-Path $serverOutput "pthreadVC2.dll")

foreach ($output in @($serverOutput, $clientOutput)) {
    foreach ($obsoleteDll in @("libnsdk.dll", "libsnappy.dll")) {
        if (Test-Path -LiteralPath (Join-Path $output $obsoleteDll) -PathType Leaf) {
            throw "Demo output still contains an obsolete DLL: $(Join-Path $output $obsoleteDll)"
        }
    }
}
foreach ($binary in @(
    $serverExe,
    $clientExe,
    (Join-Path $serverOutput "libSocketServer.dll"),
    (Join-Path $clientOutput "libSocketClient.dll"))) {
    Assert-NoObsoleteRuntimeImport -DumpBin $dumpbin -Binary $binary
}

$startInfo = [System.Diagnostics.ProcessStartInfo]::new()
$startInfo.FileName = $serverExe
$startInfo.Arguments = "$BasePort"
$startInfo.WorkingDirectory = $serverOutput
$startInfo.UseShellExecute = $false
$startInfo.CreateNoWindow = $true
$startInfo.RedirectStandardInput = $true
$startInfo.RedirectStandardOutput = $true
$startInfo.RedirectStandardError = $true
$server = [System.Diagnostics.Process]::new()
$server.StartInfo = $startInfo
$serverStarted = $false

try {
    if (-not $server.Start()) {
        throw "SocketEchoServer failed to start."
    }
    $serverStarted = $true
    $readyTask = $server.StandardOutput.ReadLineAsync()
    if (-not $readyTask.Wait(10000)) {
        throw "Timed out waiting for SocketEchoServer READY."
    }
    $readyLine = $readyTask.Result
    if ($readyLine -notmatch '^READY tcp=') {
        throw "SocketEchoServer did not become READY: $readyLine"
    }
    Write-Host $readyLine

    # Warm up lazy HP-Socket and CRT allocations before resource comparisons.
    Invoke-DemoClient -Executable $clientExe -Port $BasePort `
        -Clients $ClientsPerProtocol -IterationCount 1

    Invoke-DemoClient -Executable $clientExe -Port $BasePort `
        -Clients $ClientsPerProtocol -IterationCount $Rounds
    $firstLoad = Get-ServerResources -Process $server

    Invoke-DemoClient -Executable $clientExe -Port $BasePort `
        -Clients $ClientsPerProtocol -IterationCount $Rounds
    $secondLoad = Get-ServerResources -Process $server

    $maxPrivateGrowth = 64MB
    if ($secondLoad.Handles -gt $firstLoad.Handles + 16) {
        throw "Server handle growth: $($firstLoad.Handles) -> $($secondLoad.Handles)"
    }
    if ($secondLoad.PrivateBytes -gt $firstLoad.PrivateBytes + $maxPrivateGrowth) {
        throw "Server private-memory growth: $($firstLoad.PrivateBytes) -> $($secondLoad.PrivateBytes)"
    }
    Write-Host "Server resources stable: handles $($firstLoad.Handles) -> $($secondLoad.Handles), private bytes $($firstLoad.PrivateBytes) -> $($secondLoad.PrivateBytes)"

    $server.StandardInput.WriteLine()
    if (-not $server.WaitForExit(10000)) {
        throw "SocketEchoServer graceful shutdown timed out."
    }
    if ($server.ExitCode -ne 0) {
        $serverError = $server.StandardError.ReadToEnd()
        throw "SocketEchoServer exited with code $($server.ExitCode): $serverError"
    }
}
finally {
    if ($serverStarted -and -not $server.HasExited) {
        $server.Kill()
        $server.WaitForExit()
    }
    $server.Dispose()
}

Write-Host "Socket demo integration: PASS"
