#!/usr/bin/env node
// Terminal Launcher - Cross-platform terminal window spawning
// Part of Phase 11: Terminal Launcher
//
// Detects platform, finds available terminal emulator, launches ralph.sh
// in a new window as a detached process for execution isolation.

const { spawn } = require('child_process');
const path = require('path');
const os = require('os');
const fs = require('fs');

// Get the installed ralph.sh location (relative to this file)
// When installed: ~/.claude/get-shit-done/bin/lib/terminal-launcher.js
// ralph.sh is at: ~/.claude/get-shit-done/bin/ralph.sh
const RALPH_SCRIPT = path.join(__dirname, '..', 'ralph.sh');
const PROGRESS_WATCHER_SCRIPT = path.join(__dirname, 'progress-watcher.js');

// Convert Windows path to Git Bash path (C:/Users/... → /c/Users/...)
function toGitBashPath(windowsPath) {
  const normalized = windowsPath.replace(/\\/g, '/');
  // Match drive letter pattern like C:/ or D:/
  const match = normalized.match(/^([A-Za-z]):\//);
  if (match) {
    return '/' + match[1].toLowerCase() + normalized.slice(2);
  }
  return normalized;
}

// Common Git Bash installation locations (order: most common first)
const GIT_BASH_CANDIDATES = [
  'C:\\Program Files\\Git\\bin\\bash.exe',           // Standard 64-bit
  'C:\\Program Files (x86)\\Git\\bin\\bash.exe',     // Standard 32-bit
  'C:\\Git\\bin\\bash.exe',                          // Custom root install
  path.join(process.env.USERPROFILE || '', 'scoop', 'apps', 'git', 'current', 'bin', 'bash.exe'), // Scoop user
  'C:\\ProgramData\\scoop\\apps\\git\\current\\bin\\bash.exe'  // Scoop global
];

/**
 * Find Git Bash executable by checking multiple candidate locations
 * @returns {string|null} Path to bash.exe if found, null otherwise
 */
function findGitBash() {
  for (const candidate of GIT_BASH_CANDIDATES) {
    try {
      if (fs.existsSync(candidate)) {
        return candidate;
      }
    } catch (e) {
      continue;
    }
  }
  return null;
}

// command-exists for checking terminal availability
let commandExistsSync;
try {
  commandExistsSync = require('command-exists').sync;
} catch (e) {
  // Fallback if command-exists not installed - use native detection
  const { execSync } = require('child_process');
  commandExistsSync = (command) => {
    // On Windows, cmd.exe and powershell.exe always exist
    if (process.platform === 'win32') {
      if (command === 'cmd.exe' || command === 'powershell.exe') {
        return true;
      }
      // For other commands, use 'where' to check PATH
      try {
        execSync(`where ${command}`, { stdio: 'ignore' });
        return true;
      } catch {
        return false;
      }
    }
    // On Unix, use 'which'
    try {
      execSync(`which ${command}`, { stdio: 'ignore' });
      return true;
    } catch {
      return false;
    }
  };
}

// Terminal configurations by platform
// Order matters - first available terminal is used
const TERMINAL_CONFIG = {
  win32: [
    { name: 'wt.exe', launcher: launchWindowsTerminal, nodeLauncher: launchWindowsTerminalNode },
    { name: 'cmd.exe', launcher: launchCmd, nodeLauncher: launchCmdNode },
    { name: 'powershell.exe', launcher: launchPowerShell, nodeLauncher: launchPowerShellNode },
    { name: 'bash.exe', launcher: launchGitBash, nodeLauncher: launchGitBashNode }
  ],
  darwin: [
    { name: 'osascript', launcher: launchMacTerminal, nodeLauncher: launchMacTerminalNode }
  ],
  linux: [
    { name: 'gnome-terminal', launcher: launchGnomeTerminal, nodeLauncher: launchGnomeTerminalNode },
    { name: 'xterm', launcher: launchXterm, nodeLauncher: launchXtermNode },
    { name: 'x-terminal-emulator', launcher: launchXtermEmulator, nodeLauncher: launchXtermEmulatorNode }
  ]
};

function findTerminal(platform) {
  const terminals = TERMINAL_CONFIG[platform] || [];

  for (const terminal of terminals) {
    try {
      if (commandExistsSync(terminal.name)) {
        return terminal;
      }
    } catch (err) {
      continue;
    }
  }

  return null;
}

function launchCmd(scriptPath, windowTitle = 'GSD') {
  const cwd = process.cwd();
  // cmd.exe spawns Git Bash which needs /c/Users/... format
  const bashCwd = toGitBashPath(cwd);
  const bashScript = toGitBashPath(scriptPath);

  return spawn('cmd.exe', ['/c', 'start', windowTitle, 'cmd', '/k', `bash -c "cd '${bashCwd}' && bash '${bashScript}'"`], {
    detached: true,
    stdio: 'ignore',
    cwd: cwd,
    shell: false
  });
}

function launchCmdNode(scriptPath, windowTitle = 'GSD') {
  const cwd = process.cwd();
  // cmd.exe spawns Git Bash which needs /c/Users/... format
  const bashCwd = toGitBashPath(cwd);
  const bashScript = toGitBashPath(scriptPath);

  return spawn('cmd.exe', ['/c', 'start', windowTitle, 'cmd', '/k', `bash -c "cd '${bashCwd}' && node '${bashScript}' '${bashCwd}'"`], {
    detached: true,
    stdio: 'ignore',
    cwd: cwd,
    shell: false
  });
}

function launchPowerShell(scriptPath, windowTitle = 'GSD') {
  const cwd = process.cwd();
  // PowerShell spawns Git Bash which needs /c/Users/... format
  const bashCwd = toGitBashPath(cwd);
  const bashScript = toGitBashPath(scriptPath);

  return spawn('powershell.exe', [
    '-Command', 'Start-Process', 'powershell',
    '-ArgumentList', `"-NoExit", "-Command", "cd '${cwd.replace(/'/g, "''")}'; bash -c 'cd \\"${bashCwd}\\" && bash \\"${bashScript}\\"'"`
  ], {
    detached: true,
    stdio: 'ignore',
    cwd: cwd,
    shell: false
  });
}

function launchPowerShellNode(scriptPath, windowTitle = 'GSD') {
  const cwd = process.cwd();
  // PowerShell spawns Git Bash which needs /c/Users/... format
  const bashCwd = toGitBashPath(cwd);
  const bashScript = toGitBashPath(scriptPath);

  return spawn('powershell.exe', [
    '-Command', 'Start-Process', 'powershell',
    '-ArgumentList', `"-NoExit", "-Command", "cd '${cwd.replace(/'/g, "''")}'; bash -c 'cd \\"${bashCwd}\\" && node \\"${bashScript}\\" \\"${bashCwd}\\"'"`
  ], {
    detached: true,
    stdio: 'ignore',
    cwd: cwd,
    shell: false
  });
}

function launchWindowsTerminal(scriptPath, windowTitle = 'GSD') {
  const gitBashPath = findGitBash();

  if (!gitBashPath) {
    // Git Bash not found - signal to findTerminal() to try next terminal
    return null;
  }

  const cwd = process.cwd();
  const bashScript = toGitBashPath(scriptPath);
  const bashCwd = toGitBashPath(cwd);

  return spawn('wt.exe', [
    '--title', windowTitle,
    gitBashPath, '--login', '-c', `cd "${bashCwd}" && bash "${bashScript}"`
  ], {
    detached: true,
    stdio: 'ignore',
    cwd: cwd,
    shell: false
  });
}

function launchWindowsTerminalNode(scriptPath, windowTitle = 'GSD') {
  const gitBashPath = findGitBash();

  if (!gitBashPath) {
    // Git Bash not found - signal to findTerminal() to try next terminal
    return null;
  }

  const cwd = process.cwd();
  const bashScript = toGitBashPath(scriptPath);
  const bashCwd = toGitBashPath(cwd);

  return spawn('wt.exe', [
    '--title', windowTitle,
    gitBashPath, '--login', '-c', `cd "${bashCwd}" && node "${bashScript}" "${bashCwd}"`
  ], {
    detached: true,
    stdio: 'ignore',
    cwd: cwd,
    shell: false
  });
}

function launchGitBash(scriptPath, windowTitle = 'GSD') {
  const cwd = process.cwd();
  // Git Bash needs /c/Users/... format
  const bashCwd = toGitBashPath(cwd);
  const bashScript = toGitBashPath(scriptPath);

  return spawn('cmd.exe', [
    '/c', 'start', windowTitle, 'bash', '--login', '-i', '-c',
    `cd "${bashCwd}" && bash "${bashScript}"`
  ], {
    detached: true,
    stdio: 'ignore',
    cwd: cwd,
    shell: false
  });
}

function launchGitBashNode(scriptPath, windowTitle = 'GSD') {
  const cwd = process.cwd();
  // Git Bash needs /c/Users/... format
  const bashCwd = toGitBashPath(cwd);
  const bashScript = toGitBashPath(scriptPath);

  return spawn('cmd.exe', [
    '/c', 'start', windowTitle, 'bash', '--login', '-i', '-c',
    `cd "${bashCwd}" && node "${bashScript}" "${bashCwd}"`
  ], {
    detached: true,
    stdio: 'ignore',
    cwd: cwd,
    shell: false
  });
}

function launchMacTerminal(scriptPath, windowTitle = 'GSD') {
  const cwd = process.cwd();

  // Escape for AppleScript
  const escapedCwd = cwd.replace(/"/g, '\\"');
  const escapedScript = scriptPath.replace(/"/g, '\\"');

  const appleScript = `tell application "Terminal"
    do script "cd \\"${escapedCwd}\\" && \\"${escapedScript}\\""
    activate
end tell`;

  return spawn('osascript', ['-e', appleScript], {
    detached: true,
    stdio: 'ignore',
    cwd: cwd
  });
}

function launchMacTerminalNode(scriptPath, windowTitle = 'GSD') {
  const cwd = process.cwd();

  // Escape for AppleScript
  const escapedCwd = cwd.replace(/"/g, '\\"');
  const escapedScript = scriptPath.replace(/"/g, '\\"');

  const appleScript = `tell application "Terminal"
    do script "cd \\"${escapedCwd}\\" && node \\"${escapedScript}\\" \\"${escapedCwd}\\""
    activate
end tell`;

  return spawn('osascript', ['-e', appleScript], {
    detached: true,
    stdio: 'ignore',
    cwd: cwd
  });
}

function launchGnomeTerminal(scriptPath, windowTitle = 'GSD') {
  const cwd = process.cwd();

  return spawn('gnome-terminal', [
    '--window',
    `--title=${windowTitle}`,
    '--',
    'bash', '-c', `cd "${cwd}" && "${scriptPath}"; exec bash`
  ], {
    detached: true,
    stdio: 'ignore',
    cwd: cwd
  });
}

function launchGnomeTerminalNode(scriptPath, windowTitle = 'GSD') {
  const cwd = process.cwd();

  return spawn('gnome-terminal', [
    '--window',
    `--title=${windowTitle}`,
    '--',
    'bash', '-c', `cd "${cwd}" && node "${scriptPath}" "${cwd}"; exec bash`
  ], {
    detached: true,
    stdio: 'ignore',
    cwd: cwd
  });
}

function launchXterm(scriptPath, windowTitle = 'GSD') {
  const cwd = process.cwd();

  return spawn('xterm', [
    '-hold',
    '-title', windowTitle,
    '-e', `cd "${cwd}" && "${scriptPath}"`
  ], {
    detached: true,
    stdio: 'ignore',
    cwd: cwd
  });
}

function launchXtermNode(scriptPath, windowTitle = 'GSD') {
  const cwd = process.cwd();

  return spawn('xterm', [
    '-hold',
    '-title', windowTitle,
    '-e', `cd "${cwd}" && node "${scriptPath}" "${cwd}"`
  ], {
    detached: true,
    stdio: 'ignore',
    cwd: cwd
  });
}

function launchXtermEmulator(scriptPath, windowTitle = 'GSD') {
  // x-terminal-emulator is a Debian alternatives symlink
  // Use same approach as xterm
  return launchXterm(scriptPath, windowTitle);
}

function launchXtermEmulatorNode(scriptPath, windowTitle = 'GSD') {
  // x-terminal-emulator is a Debian alternatives symlink
  // Use same approach as xterm
  return launchXtermNode(scriptPath, windowTitle);
}

function showManualInstructions(platform) {
  const cwd = process.cwd();
  const script = RALPH_SCRIPT;

  console.log('\n==========================================');
  console.log(' TERMINAL LAUNCH UNAVAILABLE');
  console.log('==========================================\n');
  console.log('Could not detect a supported terminal emulator.\n');
  console.log('To run autopilot manually:\n');
  console.log('  1. Open a new terminal window');
  console.log(`  2. cd ${cwd}`);

  if (platform === 'win32') {
    console.log(`  3. bash ${script}\n`);
    console.log('Supported terminals on Windows:');
    console.log('  - Windows Terminal (wt.exe)');
    console.log('  - Command Prompt (cmd.exe)');
    console.log('  - PowerShell');
    console.log('  - Git Bash');
  } else if (platform === 'darwin') {
    console.log(`  3. ${script}\n`);
    console.log('Supported terminals on macOS:');
    console.log('  - Terminal.app');
  } else {
    console.log(`  3. ${script}\n`);
    console.log('Supported terminals on Linux:');
    console.log('  - gnome-terminal');
    console.log('  - xterm');
    console.log('  - x-terminal-emulator');
  }

  console.log('\n==========================================\n');
}

/**
 * Launch progress watcher in a new terminal window
 *
 * @returns {Object} Result object with success/failure info
 */
function launchProgressWatcher() {
  const platform = process.platform;
  const terminal = findTerminal(platform);

  if (!terminal || !terminal.nodeLauncher) {
    // Silently skip if no terminal found - progress watcher is optional
    return { success: false, reason: 'no_terminal_found' };
  }

  try {
    // Resolve absolute path - each launcher handles its own path format conversion
    const watcherPath = path.join(os.homedir(), '.claude', 'get-shit-done', 'bin', 'lib', 'progress-watcher.js');
    const subprocess = terminal.nodeLauncher(watcherPath, 'GSD Progress');
    subprocess.unref(); // Critical: allow parent to exit independently

    console.log(`Launched progress watcher in new ${terminal.name} window`);

    return {
      success: true,
      terminal: terminal.name,
      pid: subprocess.pid
    };
  } catch (err) {
    // Non-critical failure - just log and continue
    console.log(`Note: Could not launch progress watcher (${err.message})`);
    return {
      success: false,
      reason: 'launch_failed',
      error: err.message
    };
  }
}

/**
 * Launch ralph.sh in a new terminal window
 *
 * @returns {Object} Result object with:
 *   - success: boolean - whether terminal was launched
 *   - terminal: string - terminal name used (if success)
 *   - pid: number - process ID (if success)
 *   - watcherPid: number - progress watcher PID (if launched)
 *   - reason: string - failure reason (if !success)
 *   - error: string - error message (if launch failed)
 */
function launchTerminal() {
  const platform = process.platform;
  const terminal = findTerminal(platform);

  if (!terminal) {
    showManualInstructions(platform);
    return { success: false, reason: 'no_terminal_found' };
  }

  try {
    // Resolve absolute path - each launcher handles its own path format conversion
    const ralphPath = path.join(os.homedir(), '.claude', 'get-shit-done', 'bin', 'ralph.sh');
    const subprocess = terminal.launcher(ralphPath, 'GSD Ralph');

    // If launcher returns null (e.g., wt.exe when Git Bash not found), try next terminal
    if (subprocess === null) {
      // Find next available terminal after current one
      const terminals = TERMINAL_CONFIG[platform] || [];
      const currentIndex = terminals.findIndex(t => t.name === terminal.name);

      for (let i = currentIndex + 1; i < terminals.length; i++) {
        try {
          if (commandExistsSync(terminals[i].name)) {
            const fallbackSubprocess = terminals[i].launcher(ralphPath, 'GSD Ralph');
            if (fallbackSubprocess !== null) {
              fallbackSubprocess.unref();
              console.log(`\nLaunched ralph.sh in new ${terminals[i].name} window (fallback from ${terminal.name})`);

              // Launch progress watcher in second terminal
              const watcherResult = launchProgressWatcher();

              console.log('You can now close this Claude session - ralph.sh will continue running.\n');

              return {
                success: true,
                terminal: terminals[i].name,
                pid: fallbackSubprocess.pid,
                watcherPid: watcherResult.success ? watcherResult.pid : null,
                fallbackFrom: terminal.name
              };
            }
          }
        } catch (fallbackErr) {
          continue;
        }
      }

      // No fallback terminal worked
      console.log(`\n${terminal.name} unavailable (Git Bash not found) and no fallback terminal available\n`);
      showManualInstructions(platform);
      return {
        success: false,
        reason: 'launcher_returned_null',
        attemptedTerminal: terminal.name
      };
    }

    subprocess.unref(); // Critical: allow parent to exit independently

    console.log(`\nLaunched ralph.sh in new ${terminal.name} window`);

    // Launch progress watcher in second terminal
    const watcherResult = launchProgressWatcher();

    console.log('You can now close this Claude session - ralph.sh will continue running.\n');

    return {
      success: true,
      terminal: terminal.name,
      pid: subprocess.pid,
      watcherPid: watcherResult.success ? watcherResult.pid : null
    };
  } catch (err) {
    console.error(`\nFailed to launch ${terminal.name}: ${err.message}\n`);
    showManualInstructions(platform);
    return {
      success: false,
      reason: 'launch_failed',
      error: err.message
    };
  }
}

module.exports = {
  launchTerminal,
  findTerminal,  // Exported for testing
  findGitBash,   // Exported for testing
  showManualInstructions  // Exported for direct use if needed
};

// CLI support - run directly for testing
if (require.main === module) {
  console.log('Terminal Launcher Test');
  console.log('Platform:', process.platform);

  const terminal = findTerminal(process.platform);
  if (terminal) {
    console.log('Found terminal:', terminal.name);
    console.log('\nLaunching terminal in 3 seconds...');
    setTimeout(() => {
      const result = launchTerminal();
      console.log('Result:', JSON.stringify(result, null, 2));
      process.exit(result.success ? 0 : 1);
    }, 3000);
  } else {
    console.log('No terminal found');
    showManualInstructions(process.platform);
    process.exit(1);
  }
}
