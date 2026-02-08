/**
 * Builds the GBA ROM and launches the emulator. Used by VS Code launch.json (F5).
 * Reads emulator path from .vscode/settings.json (stranded.emulatorPath).
 */
const path = require('path');
const fs = require('fs');
const { execSync, spawn } = require('child_process');

const workspaceRoot = process.cwd();
const settingsPath = path.join(workspaceRoot, '.vscode', 'settings.json');
const gbaPath = path.join(workspaceRoot, 'stranded.gba');

let emulatorPath = path.join(workspaceRoot, 'tools', 'mGBA-0.10.5-win64', 'mGBA.exe');
try {
  const raw = fs.readFileSync(settingsPath, 'utf8');
  const settings = JSON.parse(raw);
  if (settings['stranded.emulatorPath']) {
    emulatorPath = settings['stranded.emulatorPath']
      .replace(/\$\{workspaceFolder\}/g, workspaceRoot);
  }
} catch (_) {}

console.log('Building...');
execSync('make -j8', { cwd: workspaceRoot, stdio: 'inherit' });

console.log('Launching', emulatorPath, '(debugger stays connected until emulator closes)');
const child = spawn(emulatorPath, [gbaPath], { stdio: 'ignore' });
child.on('close', (code) => process.exit(code !== null ? code : 0));
child.on('error', (err) => {
  console.error(err);
  process.exit(1);
});
