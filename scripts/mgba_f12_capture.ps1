param(
    [switch]$SkipBuild,
    [string]$RomPath = "",
    [int]$BuildJobs = 4,
    [int]$StartupDelayMs = 2500,
    [int]$PostF12DelayMs = 700
)

$ErrorActionPreference = "Stop"

function Resolve-RomPath
{
    param([string]$Root)

    $preferred = Join-Path $Root "stranded.gba"
    if(Test-Path $preferred)
    {
        return $preferred
    }

    $workspaceName = Split-Path $Root -Leaf
    $named = Join-Path $Root ($workspaceName + ".gba")
    if(Test-Path $named)
    {
        return $named
    }

    $candidates = Get-ChildItem -Path $Root -Filter "*.gba" -File -ErrorAction SilentlyContinue
    if($candidates.Count -eq 1)
    {
        return $candidates[0].FullName
    }

    throw "Unable to resolve ROM path. Expected stranded.gba, <workspace>.gba, or exactly one .gba in repo root."
}

$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = (Resolve-Path (Join-Path $scriptPath "..")).Path
Set-Location $repoRoot

$emulatorPath = Join-Path $repoRoot "tools/mGBA-0.10.5-win64/mGBA.exe"
if(-not (Test-Path $emulatorPath))
{
    throw "mGBA not found at $emulatorPath"
}

Get-Process mGBA -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 300

if(-not $SkipBuild)
{
    & make ("-j" + $BuildJobs)
    if($LASTEXITCODE -ne 0)
    {
        throw "Build failed with exit code $LASTEXITCODE"
    }
}

if($RomPath)
{
    if(-not (Test-Path $RomPath))
    {
        throw "Specified ROM path does not exist: $RomPath"
    }

    $romPath = (Resolve-Path $RomPath).Path
}
else
{
    $romPath = Resolve-RomPath -Root $repoRoot
}

$romDirectory = Split-Path -Parent $romPath
$romBaseName = [System.IO.Path]::GetFileNameWithoutExtension($romPath)
$screenshotPattern = $romBaseName + "-*.png"

$existingScreens = @{}
Get-ChildItem -Path $romDirectory -Filter $screenshotPattern -File -ErrorAction SilentlyContinue | ForEach-Object {
    $existingScreens[$_.Name] = $true
}

$process = Start-Process -FilePath $emulatorPath -ArgumentList @($romPath) -WorkingDirectory $romDirectory -PassThru

$windowHandle = [IntPtr]::Zero
for($attempt = 0; $attempt -lt 120; $attempt++)
{
    if($process.HasExited)
    {
        throw "mGBA exited before screenshot capture."
    }

    $process.Refresh()
    if($process.MainWindowHandle -ne 0)
    {
        $windowHandle = $process.MainWindowHandle
        break
    }

    Start-Sleep -Milliseconds 100
}

if($windowHandle -eq [IntPtr]::Zero)
{
    throw "mGBA window handle was not found."
}

Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;

public static class MgbaUser32
{
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool ShowWindowAsync(IntPtr hWnd, int nCmdShow);

    [DllImport("user32.dll")]
    public static extern void keybd_event(byte bVk, byte bScan, int dwFlags, int dwExtraInfo);
}
"@

[void][MgbaUser32]::ShowWindowAsync($windowHandle, 9)
[void][MgbaUser32]::SetForegroundWindow($windowHandle)
Start-Sleep -Milliseconds $StartupDelayMs

$captureStart = Get-Date
[MgbaUser32]::keybd_event(0x7B, 0, 0, 0)
Start-Sleep -Milliseconds 50
[MgbaUser32]::keybd_event(0x7B, 0, 0x0002, 0)
Start-Sleep -Milliseconds $PostF12DelayMs

function Get-CandidateScreens
{
    return Get-ChildItem -Path $romDirectory -Filter $screenshotPattern -File -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending
}

function Find-NewScreens
{
    param([object[]]$screens)

    $newByName = $screens | Where-Object { -not $existingScreens.ContainsKey($_.Name) }
    if($newByName)
    {
        return $newByName
    }

    return $screens | Where-Object { $_.LastWriteTime -ge $captureStart.AddSeconds(-1) }
}

$screens = Get-CandidateScreens
$newScreens = Find-NewScreens -screens $screens

if(-not $newScreens)
{
    Add-Type -AssemblyName System.Windows.Forms
    [void][MgbaUser32]::SetForegroundWindow($windowHandle)
    Start-Sleep -Milliseconds 120
    [System.Windows.Forms.SendKeys]::SendWait("{F12}")
    Start-Sleep -Milliseconds $PostF12DelayMs

    $screens = Get-CandidateScreens
    $newScreens = Find-NewScreens -screens $screens
}

if(-not $newScreens)
{
    throw "No new mGBA F12 screenshot was detected."
}

$latest = $newScreens | Select-Object -First 1
Write-Output $latest.FullName
