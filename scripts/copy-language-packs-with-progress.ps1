param(
    [Parameter(Mandatory = $true)]
    [string]$ResourcesSource,
    [Parameter(Mandatory = $true)]
    [string]$PacksSource,
    [Parameter(Mandatory = $true)]
    [string]$DestinationRoot
)

$ErrorActionPreference = "Stop"

function Resolve-ExistingPath([string]$PathValue, [string]$Label) {
    if ([string]::IsNullOrWhiteSpace($PathValue)) {
        throw "[copy-packs] $Label path is empty."
    }
    $resolved = Resolve-Path -LiteralPath $PathValue -ErrorAction Stop
    return $resolved.Path
}

function Collect-Files([string]$SourceRoot) {
    return Get-ChildItem -LiteralPath $SourceRoot -Recurse -File -ErrorAction Stop
}

function Get-RelativePathCompat([string]$BasePath, [string]$FilePath) {
    $baseFull = [System.IO.Path]::GetFullPath($BasePath)
    if (-not $baseFull.EndsWith([System.IO.Path]::DirectorySeparatorChar)) {
        $baseFull += [System.IO.Path]::DirectorySeparatorChar
    }
    $baseUri = New-Object System.Uri($baseFull)
    $fileUri = New-Object System.Uri([System.IO.Path]::GetFullPath($FilePath))
    $relativeUri = $baseUri.MakeRelativeUri($fileUri)
    return [System.Uri]::UnescapeDataString($relativeUri.ToString()).Replace('/', [System.IO.Path]::DirectorySeparatorChar)
}

function Ensure-Directory([string]$PathValue) {
    if ([string]::IsNullOrWhiteSpace($PathValue)) {
        return
    }
    if (-not (Test-Path -LiteralPath $PathValue -PathType Container)) {
        New-Item -Path $PathValue -ItemType Directory -Force | Out-Null
    }
}

$resourcesRoot = Resolve-ExistingPath -PathValue $ResourcesSource -Label "resources source"
$packsRoot = Resolve-ExistingPath -PathValue $PacksSource -Label "packs source"
Ensure-Directory -PathValue $DestinationRoot
$destination = (Resolve-Path -LiteralPath $DestinationRoot).Path

$sources = @(
    @{ Name = "resources/language-packs"; Root = $resourcesRoot; Files = @(Collect-Files -SourceRoot $resourcesRoot) },
    @{ Name = "language-packs"; Root = $packsRoot; Files = @(Collect-Files -SourceRoot $packsRoot) }
)

$total = 0
foreach ($entry in $sources) {
    $total += $entry.Files.Count
}

if ($total -eq 0) {
    Write-Host "[copy-packs] No files found to copy."
    exit 0
}

$copied = 0
$lastPercent = -1
foreach ($entry in $sources) {
    Write-Host ("[copy-packs] Copying {0} ({1} files)..." -f $entry.Name, $entry.Files.Count)
    foreach ($file in $entry.Files) {
        $copied += 1
        $relative = Get-RelativePathCompat -BasePath $entry.Root -FilePath $file.FullName
        $target = Join-Path $destination $relative
        $targetDir = Split-Path -Parent $target
        Ensure-Directory -PathValue $targetDir

        Copy-Item -LiteralPath $file.FullName -Destination $target -Force -ErrorAction Stop

        $percent = [int](($copied * 100) / $total)
        if ($percent -ne $lastPercent -or $copied -eq $total) {
            $lastPercent = $percent
            Write-Host ("[copy-packs] {0,3}% ({1}/{2}) {3}" -f $percent, $copied, $total, $relative)
        }
    }
}

Write-Host ("[copy-packs] Completed: {0} files copied to {1}" -f $total, $destination)
