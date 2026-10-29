param(
	[Parameter(Mandatory = $true)]
	[string]$PackageDir,

	[Parameter(Mandatory = $true)]
	[string]$VisualStudioRoot
)

$ErrorActionPreference = 'Stop'
$expectedVersion = '1.0.0.3'
$requiredFiles = @('MtAssistant.exe', 'MtAssistant.pdb', 'MtAssistant.xml')
$forbiddenImports = @('libnsdk.dll', 'pthreadVC2.dll')

$resolvedPackage = (Resolve-Path -LiteralPath $PackageDir).Path
$expectedLeaf = "MtAssistant_TAG_$expectedVersion"
if ((Split-Path -Leaf $resolvedPackage) -ne $expectedLeaf)
{
	throw "Unexpected package directory: $resolvedPackage"
}

foreach ($name in $requiredFiles)
{
	if (-not (Test-Path -LiteralPath (Join-Path $resolvedPackage $name) -PathType Leaf))
	{
		throw "Required package file is missing: $name"
	}
}

$actualFiles = @(Get-ChildItem -LiteralPath $resolvedPackage -File | Select-Object -ExpandProperty Name | Sort-Object)
$unexpectedFiles = @($actualFiles | Where-Object { $_ -notin $requiredFiles })
if ($unexpectedFiles.Count -ne 0)
{
	throw "Unexpected package files: $($unexpectedFiles -join ', ')"
}

$exePath = Join-Path $resolvedPackage 'MtAssistant.exe'
$fileVersion = (Get-Item -LiteralPath $exePath).VersionInfo.FileVersion
$productVersion = (Get-Item -LiteralPath $exePath).VersionInfo.ProductVersion
if ($fileVersion -ne $expectedVersion -or $productVersion -ne $expectedVersion)
{
	throw "Version mismatch: file=$fileVersion product=$productVersion expected=$expectedVersion"
}

$dumpbin = Get-ChildItem -LiteralPath (Join-Path $VisualStudioRoot 'VC\Tools\MSVC') -Filter dumpbin.exe -Recurse |
	Where-Object { $_.FullName -match '\\bin\\Hostx64\\x64\\dumpbin\.exe$' } |
	Sort-Object FullName -Descending |
	Select-Object -First 1
if ($null -eq $dumpbin)
{
	throw "dumpbin.exe was not found under $VisualStudioRoot"
}

$dependencies = & $dumpbin.FullName /DEPENDENTS $exePath 2>&1
if ($LASTEXITCODE -ne 0)
{
	throw "dumpbin /DEPENDENTS failed: $($dependencies -join [Environment]::NewLine)"
}
foreach ($forbidden in $forbiddenImports)
{
	if ($dependencies -match [regex]::Escape($forbidden))
	{
		throw "Forbidden runtime dependency remains: $forbidden"
	}
}

Write-Host "[PASS] Package file set, version and import table are valid."
Write-Host "[INFO] Package: $resolvedPackage"
Write-Host "[INFO] FileVersion/ProductVersion: $expectedVersion"
Write-Host "[INFO] libnsdk.dll and pthreadVC2.dll are not imported."
