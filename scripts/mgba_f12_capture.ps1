param(
    [switch]$SkipBuild,
    [ValidateSet('baseline', 'move_right', 'toggle_select', 'toggle_profiler', 'profiler_toggle_mode', 'profiler_reset')]
    [string]$Sequence = 'baseline'
)

$ErrorActionPreference = 'Stop'

$repoRootPath = Resolve-Path (Join-Path $PSScriptRoot '..')
$repoRoot = if($repoRootPath.ProviderPath) { $repoRootPath.ProviderPath } else { $repoRootPath.Path }
$romPath = Join-Path $repoRoot 'stranded.gba'
$captureLockPath = Join-Path $repoRoot '.mgba_f12_capture.lock'
$captureLockStream = $null

function Resolve-LaunchableEmulatorPath {
    param(
        [string]$CandidatePath
    )

    if($CandidatePath -notlike '\\*')
    {
        return $CandidatePath
    }

    $sourceDirectory = Split-Path -Parent $CandidatePath
    $stageDirectory = Join-Path $env:LOCALAPPDATA 'Stranded\mGBA-0.10.5-win64'
    $stagePath = Join-Path $stageDirectory (Split-Path -Leaf $CandidatePath)
    $sourceItem = Get-Item -LiteralPath $CandidatePath
    $stageItem = Get-Item -LiteralPath $stagePath -ErrorAction SilentlyContinue
    $needsSync = -not $stageItem -or
        $stageItem.Length -ne $sourceItem.Length -or
        $stageItem.LastWriteTimeUtc -lt $sourceItem.LastWriteTimeUtc

    if($needsSync)
    {
        New-Item -ItemType Directory -Path $stageDirectory -Force | Out-Null
        Get-ChildItem -LiteralPath $sourceDirectory -Force | Copy-Item -Destination $stageDirectory -Recurse -Force
        Unblock-File -LiteralPath $stagePath -ErrorAction SilentlyContinue
    }

    return $stagePath
}

try
{
    $captureLockStream = [System.IO.File]::Open(
        $captureLockPath,
        [System.IO.FileMode]::CreateNew,
        [System.IO.FileAccess]::ReadWrite,
        [System.IO.FileShare]::None)
}
catch [System.IO.IOException]
{
    throw "Another mGBA F12 capture is already running: $captureLockPath"
}
catch
{
    throw "Unable to create the mGBA capture lock at ${captureLockPath}: $($_.Exception.Message)"
}

try
{

if(-not $SkipBuild)
{
    Push-Location $repoRoot

    try
    {
        & make -j4
    }
    finally
    {
        Pop-Location
    }
}

if(-not (Test-Path -LiteralPath $romPath))
{
    throw "ROM not found at $romPath"
}

$emulatorCandidates = @(
    (Join-Path $repoRoot 'tools\mGBA-0.10.5-win64\mGBA.exe'),
    (Join-Path $HOME 'Downloads\mGBA-0.10.5-win64\mGBA.exe')
)

$emulatorPath = $emulatorCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1

if(-not $emulatorPath)
{
    throw 'Unable to locate mGBA.exe in tools/ or Downloads.'
}

$emulatorPath = Resolve-LaunchableEmulatorPath $emulatorPath

$screenshotDirectories = @($repoRoot, (Split-Path -Parent $emulatorPath)) |
    Select-Object -Unique

$existingShots = @{}
foreach($directory in $screenshotDirectories)
{
    Get-ChildItem -LiteralPath $directory -Filter 'stranded-*.png' -File -ErrorAction SilentlyContinue | ForEach-Object {
        $existingShots[$_.FullName] = $true
    }
}

Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;

public static class StrandedMgbaCapture
{
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool ShowWindowAsync(IntPtr hWnd, int nCmdShow);

    [DllImport("user32.dll")]
    public static extern void keybd_event(byte bVk, byte bScan, int dwFlags, int dwExtraInfo);
}
"@

function Press-Key {
    param(
        [byte]$VirtualKey,
        [int]$HoldMs = 50,
        [int]$AfterMs = 260
    )

    [StrandedMgbaCapture]::keybd_event($VirtualKey, 0, 0, 0)
    Start-Sleep -Milliseconds $HoldMs
    [StrandedMgbaCapture]::keybd_event($VirtualKey, 0, 2, 0)
    Start-Sleep -Milliseconds $AfterMs
}

function Get-NewScreenshotPath {
    $newShots = foreach($directory in $screenshotDirectories)
    {
        Get-ChildItem -LiteralPath $directory -Filter 'stranded-*.png' -File -ErrorAction SilentlyContinue
    }
    $newShots = $newShots |
        Where-Object { -not $existingShots.ContainsKey($_.FullName) } |
        Sort-Object LastWriteTime -Descending

    if(-not $newShots)
    {
        throw 'No new mGBA F12 screenshot was captured.'
    }

    $newShots | Select-Object -First 1 -ExpandProperty FullName
}

Get-Process mGBA -ErrorAction SilentlyContinue | Stop-Process -Force

$process = Start-Process -FilePath $emulatorPath -ArgumentList @($romPath) -WorkingDirectory $repoRoot -PassThru

$windowHandle = [IntPtr]::Zero
for($attempt = 0; $attempt -lt 120; ++$attempt)
{
    if($process.HasExited)
    {
        throw 'mGBA exited before a capture could be taken.'
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
    throw 'Unable to obtain the mGBA main window handle.'
}

[void][StrandedMgbaCapture]::ShowWindowAsync($windowHandle, 9)
[void][StrandedMgbaCapture]::SetForegroundWindow($windowHandle)
Start-Sleep -Milliseconds 3200

$shell = New-Object -ComObject WScript.Shell
if($shell.AppActivate('Unimplemented BIOS call'))
{
    Start-Sleep -Milliseconds 150
    $shell.SendKeys('{ENTER}')
    Start-Sleep -Milliseconds 2200
}
[void]$shell.AppActivate($process.Id)
Start-Sleep -Milliseconds 600

switch($Sequence)
{
    'baseline' {
    }
    'move_right' {
        for($i = 0; $i -lt 18; ++$i)
        {
            Press-Key 0x27 40 60
        }
    }
    'toggle_select' {
        Press-Key 0x08 50 350
    }
    'toggle_profiler' {
        Press-Key 0x5A 50 350
    }
    'profiler_toggle_mode' {
        Press-Key 0x5A 50 350
        Press-Key 0x58 50 350
    }
    'profiler_reset' {
        Press-Key 0x5A 50 350
        Press-Key 0x0D 50 350
    }
}

[void]$shell.AppActivate($process.Id)
Start-Sleep -Milliseconds 500
$shell.SendKeys('{F12}')
Start-Sleep -Milliseconds 1400

$screenshotPath = Get-NewScreenshotPath

    if(-not $process.HasExited)
    {
        Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
    }

    if($captureLockStream)
    {
        $captureLockStream.Dispose()
        $captureLockStream = $null
    }

    Remove-Item -LiteralPath $captureLockPath -Force -ErrorAction SilentlyContinue

    $screenshotPath
}
finally
{
    if($captureLockStream)
    {
        $captureLockStream.Dispose()
    }

    if($process -and -not $process.HasExited)
    {
        Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
    }

    Remove-Item -LiteralPath $captureLockPath -Force -ErrorAction SilentlyContinue
}
