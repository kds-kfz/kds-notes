param(
    [Parameter(Mandatory = $true)]
    [string]$SourceRoot,
    [string]$BuildRoot = (Join-Path $PSScriptRoot "..\build\xlnt"),
    [string]$VendorRoot = (Join-Path $PSScriptRoot "..\vender\xlnt")
)

$ErrorActionPreference = "Stop"

# 兼容传入 xlnt 仓库根目录或其外层目录，最终必须定位到 CMakeLists.txt。
$sourceDirectory = $SourceRoot
if (-not (Test-Path (Join-Path $sourceDirectory "CMakeLists.txt")))
{
    $sourceDirectory = Join-Path $SourceRoot "xlnt"
}
if (-not (Test-Path (Join-Path $sourceDirectory "CMakeLists.txt")))
{
    throw "未找到 xlnt CMakeLists.txt: $SourceRoot"
}

function Invoke-XlntBuild
{
    param(
        [string]$Mode,
        [bool]$Static
    )

    $buildDirectory = Join-Path $BuildRoot $Mode
    $staticOption = if ($Static) { "ON" } else { "OFF" }
    & cmake -S $sourceDirectory -B $buildDirectory -G "Visual Studio 17 2022" -A x64 `
        -DSTATIC=$staticOption -DSTATIC_CRT=OFF -DTESTS=OFF -DSAMPLES=OFF `
        -DBENCHMARKS=OFF -DPYTHON=OFF -DXLNT_CXX_LANG=17
    if ($LASTEXITCODE -ne 0) { throw "xlnt $Mode 配置失败" }

    & cmake --build $buildDirectory --config Release --target xlnt --parallel 8
    if ($LASTEXITCODE -ne 0) { throw "xlnt $Mode 构建失败" }

    $libraryDirectory = Join-Path $VendorRoot "lib\x64vc14\$Mode"
    New-Item -ItemType Directory -Path $libraryDirectory -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $buildDirectory "source\Release\xlnt.lib") `
        -Destination (Join-Path $libraryDirectory "xlnt.lib") -Force

    if (-not $Static)
    {
        $binaryDirectory = Join-Path $VendorRoot "bin\x64vc14\share"
        New-Item -ItemType Directory -Path $binaryDirectory -Force | Out-Null
        Copy-Item -LiteralPath (Join-Path $buildDirectory "source\Release\xlnt.dll") `
            -Destination (Join-Path $binaryDirectory "xlnt.dll") -Force
    }
}

# 两种模式从同一源码分别构建，禁止用导入库冒充静态库。
Invoke-XlntBuild -Mode "share" -Static $false
Invoke-XlntBuild -Mode "static" -Static $true
Write-Output "xlnt share/static 构建和发布完成"
