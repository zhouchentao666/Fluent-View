# Deploys Qt runtime next to the built fluent-view.exe.
# Usage: pwsh scripts/deploy-windows.ps1 -QtDir "C:\Qt\6.8\msvc2022_64" -OutDir "build\deploy"
param(
    [Parameter(Mandatory = $true)][string]$QtDir,
    [Parameter(Mandatory = $true)][string]$OutDir
)

$ErrorActionPreference = "Stop"

$qtBin = Join-Path $QtDir "bin"
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

Write-Host "Deploying Qt from $qtBin to $OutDir"

# Core DLLs
$core = @(
    "Qt6Core.dll", "Qt6Gui.dll", "Qt6Widgets.dll", "Qt6Svg.dll",
    "Qt6Concurrent.dll", "Qt6Network.dll",
    "d3dcompiler_47.dll", "opengl32sw.dll"
)
foreach ($dll in $core) {
    $src = Join-Path $qtBin $dll
    if (Test-Path $src) { Copy-Item $src $OutDir -Force }
}

# MSVC runtime (host and target arch match the Qt dir chosen)
$vcRedist = @("msvcp140.dll", "vcruntime140.dll", "vcruntime140_1.dll", "concrt140.dll")
foreach ($dll in $vcRedist) {
    $src = Join-Path $qtBin $dll
    if (Test-Path $src) { Copy-Item $src $OutDir -Force }
}

# Plugins
$pluginMap = @{
    "platforms/qwindows.dll"                  = "platforms"
    "platforms/qdirect2d.dll"                 = "platforms"
    "styles/qmodernwindowsstyle.dll"          = "styles"
    "imageformats/qjpeg.dll"                  = "imageformats"
    "imageformats/qpng.dll"                   = "imageformats"   # built-in, harmless if missing
    "imageformats/qgif.dll"                   = "imageformats"
    "imageformats/qico.dll"                   = "imageformats"
    "imageformats/qtiff.dll"                  = "imageformats"
    "imageformats/qwebp.dll"                  = "imageformats"
    "imageformats/qwbmp.dll"                  = "imageformats"
    "imageformats/qmng.dll"                   = "imageformats"
    "imageformats/qsvg.dll"                   = "imageformats"
    "imageformats/qavif.dll"                  = "imageformats"
    "imageformats/qheif.dll"                  = "imageformats"
    "imageformats/qtga.dll"                   = "imageformats"
    "imageformats/qwbmp.dll"                  = "imageformats"
    "iconengines/qsvgicon.dll"                = "iconengines"
    "networkinformation/qnetworklistmanager.dll" = "networkinformation"
    "tls/qcertonlybackend.dll"                = "tls"
    "tls/qopensslbackend.dll"                 = "tls"
    "tls/qschannelbackend.dll"                = "tls"
}

foreach ($entry in $pluginMap.GetEnumerator()) {
    $src = Join-Path $qtBin $entry.Key
    if (-not (Test-Path $src)) { continue }
    $dest = Join-Path $OutDir $entry.Value
    New-Item -ItemType Directory -Force -Path $dest | Out-Null
    Copy-Item $src $dest -Force
}

Write-Host "Deployment complete."
