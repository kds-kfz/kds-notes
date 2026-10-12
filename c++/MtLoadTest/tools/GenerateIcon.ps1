param(
    [string]$ProjectRoot = (Split-Path -Parent $PSScriptRoot)
)

# 生成与 TradingTerminal V3 服务图标一致的多尺寸应用图标。
# 输出 PNG 用于设计核对，ICO 由 MFC 资源脚本嵌入 EXE。
Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

Add-Type -AssemblyName System.Drawing

function New-RoundedRectanglePath {
    param([int]$Size, [int]$Radius)

    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $diameter = $Radius * 2
    $edge = $Size - 2
    $path.AddArc(1, 1, $diameter, $diameter, 180, 90)
    $path.AddArc($edge - $diameter, 1, $diameter, $diameter, 270, 90)
    $path.AddArc($edge - $diameter, $edge - $diameter, $diameter, $diameter, 0, 90)
    $path.AddArc(1, $edge - $diameter, $diameter, $diameter, 90, 90)
    $path.CloseFigure()
    return $path
}

function New-IconPngBytes {
    param([int]$Size)

    $bitmap = New-Object System.Drawing.Bitmap($Size, $Size,
        [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $graphics.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
    $graphics.Clear([System.Drawing.Color]::Transparent)

    $radius = [Math]::Max(2, [int][Math]::Round($Size * 0.16))
    $path = New-RoundedRectanglePath -Size $Size -Radius $radius
    $bounds = New-Object System.Drawing.Rectangle(0, 0, $Size, $Size)
    $top = [System.Drawing.Color]::FromArgb(255, 236, 72, 153)
    $bottom = [System.Drawing.Color]::FromArgb(255, 157, 23, 77)
    $fill = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
        $bounds, $top, $bottom, [System.Drawing.Drawing2D.LinearGradientMode]::Vertical)
    $graphics.FillPath($fill, $path)

    $borderWidth = [Math]::Max(1.0, $Size / 128.0)
    $border = New-Object System.Drawing.Pen(
        [System.Drawing.Color]::FromArgb(255, 131, 24, 67), $borderWidth)
    $graphics.DrawPath($border, $path)

    $fontSize = [single]($Size * 0.38)
    $font = New-Object System.Drawing.Font(
        "Microsoft YaHei UI", $fontSize,
        [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
    $format = New-Object System.Drawing.StringFormat
    $format.Alignment = [System.Drawing.StringAlignment]::Center
    $format.LineAlignment = [System.Drawing.StringAlignment]::Center
    $format.FormatFlags = [System.Drawing.StringFormatFlags]::NoWrap
    $textRect = New-Object System.Drawing.RectangleF(
        0, [single](-$Size * 0.015), $Size, $Size)
    $shadowRect = New-Object System.Drawing.RectangleF(
        [single]($Size * 0.012), [single]($Size * 0.005), $Size, $Size)
    $shadow = New-Object System.Drawing.SolidBrush(
        [System.Drawing.Color]::FromArgb(70, 80, 0, 35))
    $white = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::White)
    $graphics.DrawString("压测", $font, $shadow, $shadowRect, $format)
    $graphics.DrawString("压测", $font, $white, $textRect, $format)

    $stream = New-Object System.IO.MemoryStream
    $bitmap.Save($stream, [System.Drawing.Imaging.ImageFormat]::Png)
    $bytes = $stream.ToArray()

    $stream.Dispose()
    $white.Dispose()
    $shadow.Dispose()
    $format.Dispose()
    $font.Dispose()
    $border.Dispose()
    $fill.Dispose()
    $path.Dispose()
    $graphics.Dispose()
    $bitmap.Dispose()
    return $bytes
}

$resourceDirectory = Join-Path $ProjectRoot "res"
New-Item -ItemType Directory -Force -Path $resourceDirectory | Out-Null

$sizes = @(16, 20, 24, 32, 40, 48, 64, 128, 256)
$frames = @()
foreach ($size in $sizes) {
    $frames += [PSCustomObject]@{ Size = $size; Bytes = [byte[]](New-IconPngBytes -Size $size) }
}

[System.IO.File]::WriteAllBytes(
    (Join-Path $resourceDirectory "MtLoadTest.png"),
    [byte[]]($frames | Where-Object { $_.Size -eq 256 }).Bytes)

$iconPath = Join-Path $resourceDirectory "MtLoadTest.ico"
$file = [System.IO.File]::Open($iconPath, [System.IO.FileMode]::Create)
$writer = New-Object System.IO.BinaryWriter($file)
try {
    $writer.Write([uint16]0)
    $writer.Write([uint16]1)
    $writer.Write([uint16]$frames.Count)
    $offset = 6 + 16 * $frames.Count
    foreach ($frame in $frames) {
        $writer.Write([byte]($(if ($frame.Size -eq 256) { 0 } else { $frame.Size })))
        $writer.Write([byte]($(if ($frame.Size -eq 256) { 0 } else { $frame.Size })))
        $writer.Write([byte]0)
        $writer.Write([byte]0)
        $writer.Write([uint16]1)
        $writer.Write([uint16]32)
        $writer.Write([uint32]$frame.Bytes.Length)
        $writer.Write([uint32]$offset)
        $offset += $frame.Bytes.Length
    }
    foreach ($frame in $frames) {
        $writer.Write([byte[]]$frame.Bytes)
    }
}
finally {
    $writer.Dispose()
    $file.Dispose()
}

Write-Output "Generated: $iconPath"
