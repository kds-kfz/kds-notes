[CmdletBinding()]
param(
    [string]$Root,
    [string]$OutputPath,
    [string]$VersionBaselineRef
)

$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($Root)) {
    $Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
}
if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $OutputPath = Join-Path $Root '工程规范审计.json'
}
$excludedPathPattern = '\\(vender|vendor|build|bin|obj|\.vs|third[-_]party|IceGrid)\\'

function New-Finding([string]$Severity, [string]$Rule, [string]$Path, [string]$Message, [int]$Line = 0) {
    return [ordered]@{
        severity = $Severity
        rule = $Rule
        path = $Path
        line = $Line
        message = $Message
    }
}

$projects = @(Get-ChildItem -LiteralPath $Root -Recurse -Filter *.vcxproj -File |
    Where-Object { $_.FullName -notmatch $excludedPathPattern } |
    Sort-Object FullName)
$findings = [System.Collections.Generic.List[object]]::new()

foreach ($project in $projects) {
    $agentsPath = Join-Path $project.DirectoryName 'AGENTS.md'
    if (-not (Test-Path -LiteralPath $agentsPath)) {
        $findings.Add((New-Finding 'error' 'local-agents' $project.FullName '工程目录缺少 AGENTS.md。'))
    }

    if ($project.FullName -match '\\vs2022\\' -or $project.DirectoryName -match '\\tests$') {
        $content = Get-Content -LiteralPath $project.FullName -Raw
        $importsCommonRules = $content -match '<Import Project=.*\.props'
        if ($content -notmatch '<PlatformToolset>v143</PlatformToolset>') {
            $findings.Add((New-Finding 'error' 'vs2022-toolset' $project.FullName 'VS2022 工程必须使用 v143 工具集。'))
        }
        if (-not $importsCommonRules -and $content -notmatch '<WarningLevel>Level4</WarningLevel>') {
            $findings.Add((New-Finding 'error' 'warning-level' $project.FullName 'VS2022 工程必须启用 Level4，或导入统一属性表。'))
        }
        if (-not $importsCommonRules -and $content -notmatch '<LanguageStandard>stdcpp17</LanguageStandard>') {
            $findings.Add((New-Finding 'error' 'language-standard' $project.FullName 'VS2022 工程必须使用 C++17，或导入统一属性表。'))
        }
        if (-not $importsCommonRules -and $content -notmatch '<RuntimeLibrary>MultiThreadedDLL</RuntimeLibrary>') {
            $findings.Add((New-Finding 'error' 'runtime-library' $project.FullName 'VS2022 发布及兼容构建必须统一使用动态 CRT /MD。'))
        }
    }
}

$sourceExtensions = @('.c', '.cc', '.cpp', '.cxx', '.h', '.hh', '.hpp', '.hxx')
$sourceFiles = @(Get-ChildItem -LiteralPath $Root -Recurse -File |
    Where-Object { $_.FullName -notmatch $excludedPathPattern -and $_.Extension.ToLowerInvariant() -in $sourceExtensions })
$unsafePattern = '\b(strcpy|strcat|sprintf|vsprintf|gets|scanf)\s*\('
$memberDeclarationPattern = '^\s+(?:mutable\s+)?(?:[A-Za-z_][A-Za-z0-9_:<> ,*&]*\s+)+[A-Za-z][A-Za-z0-9]*_(?:\s*\{[^;]*\}|\s*=\s*[^;]+)?\s*;'
$memberScanExcludedPathPattern = '\\(vender|vendor|build|bin|obj|\.vs|third[-_]party|IceGrid|snappy|nsdk)\\'
$memberNamingFileCount = 0

foreach ($sourceFile in $sourceFiles) {
    $lineNumber = 0
    foreach ($line in (Get-Content -LiteralPath $sourceFile.FullName -ErrorAction SilentlyContinue)) {
        $lineNumber++
        if ($line -match $unsafePattern -and $line -notmatch '^\s*//') {
            # 历史源码可能同时承担旧编译器和 Linux ABI 兼容职责。先报告风险，
            # 由所属工程在具备边界测试时单独整改，不能由全局审计触发批量改写。
            $findings.Add((New-Finding 'warning' 'unsafe-crt' $sourceFile.FullName '发现无容量参数的字符串复制或格式化 API，需要所属工程结合缓冲区边界单独复核。' $lineNumber))
        }
    }

    $content = Get-Content -LiteralPath $sourceFile.FullName -Raw -ErrorAction SilentlyContinue
    if ($content -match '\b(DllMain|_tDllMain)\s*\(' -and $content -match '(CreateThread|std::thread|connect\s*\(|fopen\s*\(|CreateFile\s*\()') {
        $findings.Add((New-Finding 'error' 'loader-lock' $sourceFile.FullName 'DllMain 中疑似包含线程、网络或文件初始化，必须移到显式 Init。'))
    }
}

# 自研 Windows 封装层统一使用 m_ 成员前缀。snappy 是上游源码，nsdk 仍承担旧版
# Linux/Windows 跨平台 ABI，二者只能在所属工程的独立兼容改造中调整命名。
$memberFiles = @($sourceFiles | Where-Object {
    $_.Extension.ToLowerInvariant() -in @('.h', '.hh', '.hpp', '.hxx') -and
    $_.FullName -notmatch $memberScanExcludedPathPattern
})
$memberNamingFileCount = $memberFiles.Count
foreach ($memberFile in $memberFiles) {
    $lineNumber = 0
    foreach ($line in (Get-Content -LiteralPath $memberFile.FullName -ErrorAction SilentlyContinue)) {
        $lineNumber++
        if ($line -match $memberDeclarationPattern) {
            $findings.Add((New-Finding 'error' 'member-naming' $memberFile.FullName '成员变量禁止使用尾下划线命名，必须按工程规则使用 m_ 加区域/类型前缀。' $lineNumber))
        }
    }
}

# 版本说明是发布历史，所有二级版本段落必须按日期由旧到新排列；同一天带有
# 明确版本号的段落继续按版本号递增，防止恢复“最新版本写在文件顶部”的旧习惯。
$versionFiles = @(Get-ChildItem -LiteralPath $Root -Recurse -File -Filter '版本说明.md' |
    Where-Object { $_.FullName -notmatch $excludedPathPattern } |
    Sort-Object FullName)
$versionHeadingPattern = '(?m)^##(?!#)\s+.*$'
foreach ($versionFile in $versionFiles) {
    $content = [System.IO.File]::ReadAllText($versionFile.FullName)
    $headings = [regex]::Matches($content, $versionHeadingPattern)
    $previousDate = [datetime]::MinValue
    $previousVersion = $null
    $seenUnversionedOnDate = $false
    foreach ($headingMatch in $headings) {
        $heading = $headingMatch.Value
        $dateMatch = [regex]::Match($heading, '\d{4}-\d{2}-\d{2}')
        if (-not $dateMatch.Success) {
            # 历史文档中存在“记录信息、改动内容”等二级说明标题；它们不是版本节点，
            # 只跳过排序判断，不要求为了审计而重写原有文档层级。
            continue
        }

        $currentDate = [datetime]::ParseExact($dateMatch.Value, 'yyyy-MM-dd', [Globalization.CultureInfo]::InvariantCulture)
        $versionMatch = [regex]::Match($heading, '^##\s+(\d+\.\d+\.\d+(?:\.\d+)?)')
        $currentVersion = if ($versionMatch.Success) { [Version]$versionMatch.Groups[1].Value } else { $null }
        if ($currentDate -lt $previousDate) {
            $findings.Add((New-Finding 'error' 'version-history-order' $versionFile.FullName "版本日期倒序：$heading；旧记录必须在前，新记录只能追加到末尾。"))
        }
        elseif ($currentDate -gt $previousDate) {
            $previousVersion = $currentVersion
            $seenUnversionedOnDate = $null -eq $currentVersion
        }
        elseif ($null -ne $currentVersion) {
            if ($seenUnversionedOnDate -or ($null -ne $previousVersion -and $currentVersion -lt $previousVersion)) {
                $findings.Add((New-Finding 'error' 'version-history-order' $versionFile.FullName "同日版本号倒序：$heading；低版本必须写在高版本之前。"))
            }
            $previousVersion = $currentVersion
        }
        else {
            $seenUnversionedOnDate = $true
        }
        $previousDate = $currentDate
    }
}

# CI 或提交前可传入基线引用（例如 origin/main），进一步确认历史正文没有被覆盖、
# 删除或重排。完成本次一次性旧顺序归一化后，后续发布应始终启用该门禁。
if (-not [string]::IsNullOrWhiteSpace($VersionBaselineRef)) {
    $gitRoot = (& git -C $Root rev-parse --show-toplevel 2>$null | Select-Object -First 1)
    if ($LASTEXITCODE -eq 0 -and -not [string]::IsNullOrWhiteSpace($gitRoot)) {
        $gitRoot = [System.IO.Path]::GetFullPath($gitRoot.Trim())
        foreach ($versionFile in $versionFiles) {
            if (-not $versionFile.FullName.StartsWith($gitRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
                continue
            }
            $relativePath = $versionFile.FullName.Substring($gitRoot.Length).TrimStart('\', '/').Replace('\', '/')
            $baselineLines = @(& git -C $gitRoot show "${VersionBaselineRef}:$relativePath" 2>$null)
            if ($LASTEXITCODE -ne 0) {
                continue
            }
            $baseline = (($baselineLines -join "`n") -replace "`r`n?", "`n").TrimEnd("`r", "`n")
            $current = ([System.IO.File]::ReadAllText($versionFile.FullName) -replace "`r`n?", "`n")
            if (-not $current.StartsWith($baseline, [System.StringComparison]::Ordinal)) {
                $findings.Add((New-Finding 'error' 'version-history-append-only' $versionFile.FullName "相对基线 $VersionBaselineRef 的历史正文被覆盖、删除或重排；必须恢复旧记录，并把新记录追加到文件末尾。"))
            }
        }
    }
}

$summary = [ordered]@{
    projectCount = $projects.Count
    sourceFileCount = $sourceFiles.Count
    memberNamingFileCount = $memberNamingFileCount
    versionFileCount = $versionFiles.Count
    errorCount = @($findings | Where-Object { $_.severity -eq 'error' }).Count
    warningCount = @($findings | Where-Object { $_.severity -eq 'warning' }).Count
}

$report = [ordered]@{
    schemaVersion = 1
    fieldDescriptions = [ordered]@{
        root = '被审计的 C++ 工程根目录。'
        generatedAtUtc = '审计执行时间，使用 UTC ISO-8601 格式。'
        versionBaselineRef = '可选 Git 基线引用；设置后严格校验历史正文保持为当前文件的完整前缀。'
        summary = '项目数、源码数和问题数量汇总。'
        findings = '所有不符合项；空数组表示自动门禁全部通过。'
        severity = '问题等级，error 会使脚本返回非零。'
        rule = '稳定的规则标识，便于 CI 分类。'
        path = '问题文件绝对路径。'
        line = '问题行号；工程级问题为 0。'
        message = '整改要求说明。'
        projectCount = '纳入审计的 vcxproj 数量。'
        sourceFileCount = '纳入危险 API 与 DllMain 扫描的源码数量。'
        memberNamingFileCount = '纳入自研成员变量命名扫描的头文件数量；排除官方、第三方及需保持跨平台 ABI 的历史源码。'
        versionFileCount = '纳入追加式历史校验的版本说明文件数量。'
        sourceEncoding = '新增源码使用 UTF-8；历史源码保留原编码，编码迁移必须单独评审，不由审计脚本自动改写。'
        errorCount = '阻断构建或发布的问题数量。'
        warningCount = '需要人工复核但不阻断的问题数量。'
    }
    root = $Root
    versionBaselineRef = $VersionBaselineRef
    generatedAtUtc = [DateTime]::UtcNow.ToString('o')
    summary = $summary
    findings = @($findings)
}

[System.IO.File]::WriteAllText($OutputPath, ($report | ConvertTo-Json -Depth 8) + [Environment]::NewLine, [System.Text.UTF8Encoding]::new($false))
Write-Host ("审计完成：工程 {0} 个，源码 {1} 个，错误 {2} 个。" -f $summary.projectCount, $summary.sourceFileCount, $summary.errorCount)
Write-Host "报告：$OutputPath"

if ($summary.errorCount -gt 0) {
    $findings | Where-Object { $_.severity -eq 'error' } | ForEach-Object {
        Write-Error ("[{0}] {1}:{2} {3}" -f $_.rule, $_.path, $_.line, $_.message)
    }
    exit 1
}
