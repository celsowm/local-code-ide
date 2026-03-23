# Build WiX Installer for LocalCodeIDE
param(
    [string]$Version = "0.10.1",
    [string]$BuildDir = "build",
    [string]$OutputDir = "build/installer"
)

$ErrorActionPreference = "Stop"

# Paths
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$StagingDir = Join-Path $OutputDir "staging"
$ReleaseDir = Join-Path $BuildDir "Release"
$InstallerDir = Join-Path $ProjectRoot "installer"
$AppIconSource = Join-Path $ProjectRoot "assets\local-code-ide-logo.ico"
$OutputMsi = Join-Path $OutputDir "LocalCodeIDE-${Version}.msi"

# WiX variables
$UpgradeCode = "C957FC5D-DD7C-48C4-AC7C-8900BD9AA945"

Write-Host "=== LocalCodeIDE WiX Installer Build ===" -ForegroundColor Green
Write-Host "Version: $Version"
Write-Host "Build Dir: $BuildDir"
Write-Host "Output: $OutputMsi"

# Add WiX to PATH
$wixPath = Join-Path $env:USERPROFILE ".dotnet\tools"
$env:PATH = "$wixPath;$env:PATH"

# Check if wix is installed
try {
    $wixExe = Get-Command wix -ErrorAction Stop
    Write-Host "WiX found: $($wixExe.Source)" -ForegroundColor Green
    $wixVersion = & wix --version
    Write-Host "WiX version: $wixVersion" -ForegroundColor Green
} catch {
    Write-Host "ERROR: WiX Toolset not found!" -ForegroundColor Red
    Write-Host "Installing WiX Toolset..." -ForegroundColor Yellow
    & dotnet tool install --global wix
    $env:PATH = "$wixPath;$env:PATH"
}

# Create output directory
if (!(Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir | Out-Null
}

# Copy deployed files to staging
Write-Host "`nCopying files to staging..." -ForegroundColor Cyan
if (Test-Path $StagingDir) {
    Remove-Item -Recurse -Force $StagingDir
}
Copy-Item -Recurse -Force $ReleaseDir $StagingDir
if (Test-Path $AppIconSource) {
    Copy-Item -Force $AppIconSource (Join-Path $StagingDir "local-code-ide-logo.ico")
} else {
    Write-Host "WARNING: App icon not found at $AppIconSource" -ForegroundColor Yellow
}

# Verify required files
$requiredFiles = @(
    "LocalCodeIDE.exe",
    "Qt6Core.dll",
    "Qt6Gui.dll",
    "Qt6Qml.dll",
    "Qt6Quick.dll",
    "Qt6Network.dll",
    "plugins\platforms\qwindows.dll",
    "local-code-ide-logo.ico",
    "language-packs\manifest.json",
    "language-packs\windows\rust-analyzer\rust-analyzer.cmd",
    "language-packs\linux\rust-analyzer\rust-analyzer.sh",
    "qt.conf"
)

$missingFiles = @()
foreach ($file in $requiredFiles) {
    $filePath = Join-Path $StagingDir $file
    if (!(Test-Path $filePath)) {
        $missingFiles += $file
    }
}

if ($missingFiles.Count -gt 0) {
    Write-Host "`nWARNING: Missing files in staging:" -ForegroundColor Yellow
    $missingFiles | ForEach-Object { Write-Host "  - $_" }
    Write-Host "`nMake sure to run 'python setup.py' first to build and deploy Qt." -ForegroundColor Yellow
    exit 1
}

# Build WiX project
Write-Host "`nBuilding MSI installer..." -ForegroundColor Cyan

# Get absolute path to staging directory
$StagingDirAbsolute = (Get-Item $StagingDir).FullName

$wixArgs = @(
    "build"
    (Join-Path $InstallerDir "Product.wxs")
    "-arch", "x64"
    "-out", $OutputMsi
    "-d", "ProductDir=$StagingDirAbsolute\"
    "-d", "Version=$Version"
    "-d", "UpgradeCode=$UpgradeCode"
)

Write-Host "Command: wix $($wixArgs -join ' ')" -ForegroundColor Gray
& wix @wixArgs

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n=== Installer built successfully! ===" -ForegroundColor Green
    Write-Host "Output: $OutputMsi" -ForegroundColor Green
    
    # Show file size
    $msiSize = (Get-Item $OutputMsi).Length / 1MB
    Write-Host ("Size: {0:N2} MB" -f $msiSize) -ForegroundColor Green
} else {
    Write-Host "`nERROR: Installer build failed!" -ForegroundColor Red
    exit $LASTEXITCODE
}
