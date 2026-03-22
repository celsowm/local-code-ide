param(
    [string]$OutputPath = "",
    [string]$ProcessName = "LocalCodeIDE",
    [string]$WindowTitleContains = "LocalCodeIDE",
    [int]$ProcessId = 0,
    [int]$TimeoutSeconds = 12,
    [switch]$LaunchIfMissing,
    [string]$ExecutablePath = "build/Release/LocalCodeIDE.exe",
    [switch]$FullScreen,
    [switch]$FallbackToFullScreen,
    [switch]$ActivateWindow
)

$ErrorActionPreference = "Stop"

Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms

Add-Type @"
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

public struct RECT {
    public int Left;
    public int Top;
    public int Right;
    public int Bottom;
}

public static class NativeMethods {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern int GetWindowTextLength(IntPtr hWnd);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll")]
    public static extern bool ShowWindowAsync(IntPtr hWnd, int nCmdShow);

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool IsIconic(IntPtr hWnd);
}
"@

function Resolve-OutputPath {
    param([string]$PathValue)

    if (-not [string]::IsNullOrWhiteSpace($PathValue)) {
        return [System.IO.Path]::GetFullPath($PathValue)
    }

    $timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $defaultDir = [System.IO.Path]::GetFullPath("build/screenshots")
    return [System.IO.Path]::Combine($defaultDir, "localcodeide-$timestamp.png")
}

function Get-TopLevelWindows {
    $windows = New-Object System.Collections.Generic.List[Object]

    $callback = [NativeMethods+EnumWindowsProc]{
        param([IntPtr]$hWnd, [IntPtr]$lParam)

        if (-not [NativeMethods]::IsWindowVisible($hWnd)) {
            return $true
        }

        $length = [NativeMethods]::GetWindowTextLength($hWnd)
        if ($length -le 0) {
            return $true
        }

        $builder = New-Object System.Text.StringBuilder ($length + 1)
        [void][NativeMethods]::GetWindowText($hWnd, $builder, $builder.Capacity)
        $title = $builder.ToString()
        if ([string]::IsNullOrWhiteSpace($title)) {
            return $true
        }

        [uint32]$procId = 0
        [void][NativeMethods]::GetWindowThreadProcessId($hWnd, [ref]$procId)
        if ($procId -le 0) {
            return $true
        }

        try {
            $proc = Get-Process -Id $procId -ErrorAction Stop
        } catch {
            return $true
        }

        $windows.Add([PSCustomObject]@{
            Handle = $hWnd
            ProcessId = [int]$procId
            ProcessName = $proc.ProcessName
            Title = $title
            Minimized = [NativeMethods]::IsIconic($hWnd)
            StartTime = $proc.StartTime
        }) | Out-Null

        return $true
    }

    [void][NativeMethods]::EnumWindows($callback, [IntPtr]::Zero)
    return $windows
}

function Get-WindowMatch {
    param(
        [string]$Name,
        [string]$TitleNeedle,
        [int]$TargetProcessId
    )

    $windows = Get-TopLevelWindows |
        Where-Object { $_.Minimized -eq $false } |
        Sort-Object StartTime -Descending

    if ($TargetProcessId -gt 0) {
        $windows = $windows | Where-Object { $_.ProcessId -eq $TargetProcessId }
    } elseif (-not [string]::IsNullOrWhiteSpace($Name)) {
        $windows = $windows | Where-Object { $_.ProcessName -ieq $Name }
    }

    if (-not [string]::IsNullOrWhiteSpace($TitleNeedle)) {
        $filtered = $windows | Where-Object { $_.Title -like "*$TitleNeedle*" }
        if ($filtered) {
            return $filtered | Select-Object -First 1
        }
    }

    $filtered = $windows
    if ($filtered) {
        return $filtered | Select-Object -First 1
    }

    return $null
}

function Wait-ForWindow {
    param(
        [string]$Name,
        [string]$TitleNeedle,
        [int]$TargetProcessId,
        [int]$TimeoutS
    )

    $deadline = (Get-Date).AddSeconds($TimeoutS)
    while ((Get-Date) -lt $deadline) {
        $win = Get-WindowMatch -Name $Name -TitleNeedle $TitleNeedle -TargetProcessId $TargetProcessId
        if ($null -ne $win) {
            return $win
        }
        Start-Sleep -Milliseconds 250
    }
    return $null
}

function Capture-RectangleToFile {
    param(
        [int]$Left,
        [int]$Top,
        [int]$Width,
        [int]$Height,
        [string]$Path
    )

    if ($Width -le 0 -or $Height -le 0) {
        throw "Invalid capture size: ${Width}x${Height}."
    }

    $dir = Split-Path -Parent $Path
    if (-not [string]::IsNullOrWhiteSpace($dir)) {
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
    }

    $bitmap = New-Object System.Drawing.Bitmap($Width, $Height)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.CopyFromScreen($Left, $Top, 0, 0, $bitmap.Size)
        $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
}

$resolvedOutput = Resolve-OutputPath -PathValue $OutputPath

if ($FullScreen) {
    $bounds = [System.Windows.Forms.SystemInformation]::VirtualScreen
    Capture-RectangleToFile -Left $bounds.Left -Top $bounds.Top -Width $bounds.Width -Height $bounds.Height -Path $resolvedOutput
    Write-Host "Screenshot saved: $resolvedOutput"
    exit 0
}

$targetWindow = Get-WindowMatch -Name $ProcessName -TitleNeedle $WindowTitleContains -TargetProcessId $ProcessId
if ($null -eq $targetWindow -and $LaunchIfMissing) {
    $exeFullPath = [System.IO.Path]::GetFullPath($ExecutablePath)
    if (-not (Test-Path -LiteralPath $exeFullPath)) {
        throw "Executable not found: $exeFullPath"
    }

    $started = Start-Process -FilePath $exeFullPath -PassThru
    try {
        [void]$started.WaitForInputIdle(5000)
    } catch {
        # Some app types do not support WaitForInputIdle.
    }
    $targetWindow = Wait-ForWindow -Name $ProcessName -TitleNeedle $WindowTitleContains -TargetProcessId $started.Id -TimeoutS $TimeoutSeconds
}

if ($null -eq $targetWindow) {
    if ($FallbackToFullScreen) {
        $bounds = [System.Windows.Forms.SystemInformation]::VirtualScreen
        Capture-RectangleToFile -Left $bounds.Left -Top $bounds.Top -Width $bounds.Width -Height $bounds.Height -Path $resolvedOutput
        Write-Host "Window not found. Fullscreen screenshot saved: $resolvedOutput"
        exit 0
    }
    throw "No visible window found for process '$ProcessName'. Use -LaunchIfMissing or -FullScreen."
}

$hWnd = $targetWindow.Handle
if ($ActivateWindow) {
    [NativeMethods]::ShowWindowAsync($hWnd, 5) | Out-Null
    [NativeMethods]::SetForegroundWindow($hWnd) | Out-Null
    Start-Sleep -Milliseconds 150
}

$rect = New-Object RECT
if (-not [NativeMethods]::GetWindowRect($hWnd, [ref]$rect)) {
    throw "Failed to query window bounds for handle $hWnd."
}

$width = $rect.Right - $rect.Left
$height = $rect.Bottom - $rect.Top
Capture-RectangleToFile -Left $rect.Left -Top $rect.Top -Width $width -Height $height -Path $resolvedOutput

Write-Host "Screenshot saved: $resolvedOutput"
