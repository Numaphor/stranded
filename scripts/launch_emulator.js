/**
 * Builds the GBA ROM and launches the emulator. Used by VS Code launch.json (F5).
 * Reads emulator path from .vscode/settings.json (stranded.emulatorPath).
 */
const path = require('path');
const fs = require('fs');
const { execSync, spawn, spawnSync } = require('child_process');

const workspaceRoot = process.cwd();
const settingsPath = path.join(workspaceRoot, '.vscode', 'settings.json');

function _is_wsl()
{
  if(process.platform !== 'linux')
  {
    return false;
  }

  if(process.env.WSL_INTEROP || process.env.WSL_DISTRO_NAME)
  {
    return true;
  }

  try
  {
    const version = fs.readFileSync('/proc/version', 'utf8').toLowerCase();
    return version.includes('microsoft');
  }
  catch (_)
  {
    return false;
  }
}

function _wslpath_windows(posixPath)
{
  const result = spawnSync('wslpath', ['-w', posixPath], { encoding: 'utf8' });
  if(result.status === 0)
  {
    const resolved = (result.stdout || '').trim();
    return resolved.length ? resolved : null;
  }

  return null;
}

function _which(cmd)
{
  if(process.platform === 'win32')
  {
    return null;
  }

  const result = spawnSync('which', [cmd], { encoding: 'utf8' });
  if(result.status === 0)
  {
    const resolved = (result.stdout || '').trim();
    return resolved.length ? resolved : null;
  }

  return null;
}

function _resolve_gba_path()
{
  const stranded = path.join(workspaceRoot, 'stranded.gba');
  if(fs.existsSync(stranded))
  {
    return stranded;
  }

  const basename = path.basename(workspaceRoot);
  const named = path.join(workspaceRoot, `${basename}.gba`);
  if(fs.existsSync(named))
  {
    return named;
  }

  const candidates = fs.readdirSync(workspaceRoot)
    .filter((name) => name.toLowerCase().endsWith('.gba'))
    .map((name) => path.join(workspaceRoot, name));
  if(candidates.length === 1)
  {
    return candidates[0];
  }

  return stranded;
}

const gbaPath = _resolve_gba_path();

const isWsl = _is_wsl();

const defaultWindowsEmulatorPath = path.join(workspaceRoot, 'tools', 'mGBA-0.10.5-win64', 'mGBA.exe');
let emulatorPath = (process.platform === 'win32' || isWsl) ? defaultWindowsEmulatorPath : 'VisualBoyAdvance';
try {
  const raw = fs.readFileSync(settingsPath, 'utf8');
  const settings = JSON.parse(raw);
  if (settings['stranded.emulatorPath']) {
    emulatorPath = settings['stranded.emulatorPath']
      .replace(/\$\{workspaceFolder\}/g, workspaceRoot);
  }
} catch (_) {}

let emulatorCommand = emulatorPath;
let emulatorArgs = [gbaPath];
const childEnv = { ...process.env };

if(process.platform !== 'win32')
{
  const configured_is_windows_exe = emulatorPath.toLowerCase().endsWith('.exe');
  if(configured_is_windows_exe)
  {
    if(isWsl)
    {
      const cmdPath = _which('cmd.exe') || (fs.existsSync('/mnt/c/Windows/System32/cmd.exe') ? '/mnt/c/Windows/System32/cmd.exe' : null);
      const emulatorWinPath = _wslpath_windows(emulatorPath);
      const gbaWinPath = _wslpath_windows(gbaPath);

      if(!cmdPath || !emulatorWinPath || !gbaWinPath)
      {
        console.error('WSL detected, but failed to launch Windows emulator.');
        console.error('Ensure wslpath and Windows interop are available, or set stranded.emulatorPath to a native Linux emulator binary.');
        process.exit(1);
      }

      emulatorCommand = cmdPath;
      emulatorArgs = ['/c', 'start', '""', '/wait', emulatorWinPath, gbaWinPath];
    }
    else
    {
      const winePath = _which('wine');
      if(winePath)
      {
        emulatorCommand = winePath;
        emulatorArgs = [emulatorPath, gbaPath];
      }
      else
      {
        const vbaPath = _which('VisualBoyAdvance') || (fs.existsSync('/usr/bin/VisualBoyAdvance') ? '/usr/bin/VisualBoyAdvance' : null);
        const mgbaPath = _which('mgba');

        if(vbaPath)
        {
          console.warn('Configured stranded.emulatorPath points to a Windows .exe; falling back to VisualBoyAdvance.');
          emulatorCommand = vbaPath;
          emulatorArgs = [gbaPath];
        }
        else if(mgbaPath)
        {
          console.warn('Configured stranded.emulatorPath points to a Windows .exe; falling back to mgba.');
          emulatorCommand = mgbaPath;
          emulatorArgs = [gbaPath];
        }
        else
        {
          console.error('Configured stranded.emulatorPath points to a Windows .exe, but wine is not installed.');
          console.error('Install a native emulator (VisualBoyAdvance or mgba), or set stranded.emulatorPath to a native binary.');
          process.exit(1);
        }
      }
    }
  }

  if(!emulatorCommand.includes(path.sep) && !_which(emulatorCommand))
  {
    console.error('Emulator not found in PATH:', emulatorCommand);
    console.error('Install a native emulator (VisualBoyAdvance or mgba), or set stranded.emulatorPath in .vscode/settings.json.');
    process.exit(1);
  }

  if(!childEnv.DISPLAY)
  {
    childEnv.DISPLAY = ':1';
  }
}

if((emulatorCommand.includes(path.sep) || emulatorCommand.includes('/')))
{
  if(!fs.existsSync(emulatorCommand))
  {
    console.error('Emulator not found:', emulatorCommand);
    console.error('Set .vscode/settings.json stranded.emulatorPath to a native emulator binary for your OS.');
    process.exit(1);
  }
}

console.log('Building...');
execSync('make -j8', { cwd: workspaceRoot, stdio: 'inherit' });

console.log('Launching', emulatorCommand, emulatorArgs.join(' '));
const child = spawn(emulatorCommand, emulatorArgs, { stdio: 'inherit', env: childEnv });
child.on('close', (code) => process.exit(code !== null ? code : 0));
child.on('error', (err) => {
  console.error(err);
  process.exit(1);
});
