# Stages and packs the MSIX package.
# Usage: pwsh scripts/make-msix.ps1 -DeployDir "build\deploy" -Arch "x64" -OutFile "dist\Fluent-View-1.0.0-x64.msix"
param(
    [Parameter(Mandatory = $true)][string]$DeployDir,
    [Parameter(Mandatory = $true)][ValidateSet("x64", "arm64")][string]$Arch,
    [Parameter(Mandatory = $true)][string]$OutFile
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path "$PSScriptRoot/..").Path

$stage = Join-Path $env:TEMP ("fluentview-msix-" + [Guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Force -Path $stage | Out-Null

try {
    # app payload
    Copy-Item (Join-Path $DeployDir "*") $stage -Recurse -Force

    # manifest
    $manifestSrc = Join-Path $repoRoot "packaging/windows/msix/AppxManifest.xml"
    $manifest = Get-Content $manifestSrc -Raw
    $manifest = $manifest.Replace('${ARCH}', $Arch)
    Set-Content -Path (Join-Path $stage "AppxManifest.xml") -Value $manifest -Encoding UTF8

    # visual assets rendered from the app SVG
    $assets = Join-Path $stage "assets"
    New-Item -ItemType Directory -Force -Path $assets | Out-Null
    $svg = Join-Path $repoRoot "resources/icons/fluentview.svg"
    $magick = Get-Command magick -ErrorAction SilentlyContinue
    if ($magick) {
        & $magick.Path $svg -resize 44x44  (Join-Path $assets "Square44x44Logo.png")
        & $magick.Path $svg -resize 150x150 (Join-Path $assets "Square150x150Logo.png")
        & $magick.Path $svg -resize 50x50  (Join-Path $assets "StoreLogo.png")
    } else {
        Write-Warning "ImageMagick not found; generating placeholder assets."
        foreach ($size in @("Square44x44Logo", "Square150x150Logo", "StoreLogo")) {
            # 1x1 transparent PNG placeholder (base64)
            $png = [Convert]::FromBase64String("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNkYPhfDwAChwGA60e6kgAAAABJRU5ErkJggg==")
            [IO.File]::WriteAllBytes((Join-Path $assets "$size.png"), $png)
        }
    }

    # [Content_Types].xml
    $ctypes = Join-Path $stage "[Content_Types].xml"
    Set-Content -Path $ctypes -Value @'
<?xml version="1.0" encoding="utf-8"?>
<Types xmlns="http://schemas.microsoft.com/package/2012/03/content-type-extensions">
  <Default Extension="xml" ContentType="application/vnd.ms-appx.manifest"/>
  <Default Extension="png" ContentType="image/png"/>
  <Default Extension="dll" ContentType="application/octet-stream"/>
  <Default Extension="exe" ContentType="application/octet-stream"/>
</Types>
'@ -Encoding UTF8

    # pack
    $makeappx = Get-ChildItem "${env:ProgramFiles(x86)}\Windows Kits\10\bin" -Recurse -Filter "makeappx.exe" |
        Where-Object { $_.DirectoryName -like "*\x64" } |
        Select-Object -Last 1
    if (-not $makeappx) { throw "makeappx.exe not found in Windows SDK." }

    New-Item -ItemType Directory -Force -Path (Split-Path $OutFile) | Out-Null
    & $makeappx.FullName pack /o /d $stage /p $OutFile | Write-Host
    if ($LASTEXITCODE -ne 0) { throw "makeappx failed with $LASTEXITCODE" }

    Write-Host "MSIX written: $OutFile"
}
finally {
    Remove-Item $stage -Recurse -Force -ErrorAction SilentlyContinue
}
