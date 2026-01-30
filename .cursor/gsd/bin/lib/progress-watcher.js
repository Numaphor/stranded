#!/usr/bin/env node
// GSD Progress Watcher - Live autopilot progress display
// Part of Quick Task 001: Autopilot Progress Watcher
//
// Watches .planning/STATE.md and .planning/ralph.log for changes
// and displays formatted real-time progress updates.
// Zero API token consumption - pure file watching.

const fs = require('fs');
const path = require('path');
const os = require('os');

// ANSI color codes (respect NO_COLOR)
const NO_COLOR = process.env.NO_COLOR;
const COLORS = NO_COLOR ? {
  RED: '',
  GREEN: '',
  YELLOW: '',
  CYAN: '',
  BLUE: '',
  BOLD: '',
  DIM: '',
  RESET: ''
} : {
  RED: '\x1b[31m',
  GREEN: '\x1b[32m',
  YELLOW: '\x1b[33m',
  CYAN: '\x1b[36m',
  BLUE: '\x1b[34m',
  BOLD: '\x1b[1m',
  DIM: '\x1b[2m',
  RESET: '\x1b[0m'
};

// Clear screen
function clearScreen() {
  if (!NO_COLOR) {
    process.stdout.write('\x1b[2J\x1b[H');
  }
}

// Parse STATE.md for current position and progress
function parseState(stateContent) {
  const lines = stateContent.split('\n');
  const result = {
    phase: '',
    plan: '',
    status: '',
    progress: '',
    lastActivity: '',
    nextTask: ''
  };

  for (let i = 0; i < lines.length; i++) {
    const line = lines[i];

    if (line.startsWith('Phase:')) {
      result.phase = line.substring(6).trim();
    } else if (line.startsWith('Plan:')) {
      result.plan = line.substring(5).trim();
    } else if (line.startsWith('Status:')) {
      result.status = line.substring(7).trim();
      // Extract "ready for XX-YY" pattern to show next task
      const readyMatch = result.status.match(/ready for (\d{2}-\d{2})/);
      if (readyMatch) {
        result.nextTask = readyMatch[1];
      }
    } else if (line.startsWith('Last activity:')) {
      result.lastActivity = line.substring(14).trim();
    } else if (line.startsWith('Progress:')) {
      result.progress = line.substring(9).trim();
    } else if (line.startsWith('Description:')) {
      // Next Action section has Description with task ID
      const descMatch = line.match(/(\d{2}-\d{2})/);
      if (descMatch && !result.nextTask) {
        result.nextTask = descMatch[1];
      }
    }
  }

  return result;
}

// Parse ralph.log for iteration entries
function parseRalphLog(logContent) {
  const entries = [];
  const blocks = logContent.split('---\n').filter(b => b.trim());

  for (const block of blocks) {
    const lines = block.split('\n');
    const entry = {};

    for (const line of lines) {
      if (line.startsWith('Iteration:')) {
        entry.iteration = line.substring(10).trim();
      } else if (line.startsWith('Timestamp:')) {
        entry.timestamp = line.substring(10).trim();
      } else if (line.startsWith('Task:')) {
        entry.task = line.substring(5).trim();
      } else if (line.startsWith('Status:')) {
        entry.status = line.substring(7).trim();
      } else if (line.startsWith('Duration:')) {
        entry.duration = line.substring(9).trim();
      } else if (line.startsWith('Summary:')) {
        entry.summary = line.substring(8).trim();
      }
    }

    if (entry.iteration) {
      entries.push(entry);
    }
  }

  return entries;
}

// Format status with color
function formatStatus(status) {
  if (!status) return '';

  const upper = status.toUpperCase();
  if (upper === 'SUCCESS') {
    return `${COLORS.GREEN}✓ ${status}${COLORS.RESET}`;
  } else if (upper === 'FAILURE') {
    return `${COLORS.RED}✗ ${status}${COLORS.RESET}`;
  } else if (upper === 'RETRY') {
    return `${COLORS.YELLOW}⟳ ${status}${COLORS.RESET}`;
  } else if (upper === 'RUNNING') {
    return `${COLORS.CYAN}▸ ${status}${COLORS.RESET}`;
  } else if (upper === 'SKIPPED') {
    return `${COLORS.YELLOW}⊘ ${status}${COLORS.RESET}`;
  } else {
    return status;
  }
}

// Display current state
function displayProgress(stateFile, logFile, isPaused) {
  clearScreen();

  const now = new Date().toLocaleTimeString();
  console.log(`${COLORS.BOLD}${COLORS.CYAN}═══════════════════════════════════════════════════════════════════${COLORS.RESET}`);
  console.log(`${COLORS.BOLD}${COLORS.CYAN}  GSD Progress Watcher${COLORS.RESET}                        ${COLORS.DIM}${now}${COLORS.RESET}`);
  console.log(`${COLORS.BOLD}${COLORS.CYAN}═══════════════════════════════════════════════════════════════════${COLORS.RESET}\n`);

  // Read and display STATE.md
  if (fs.existsSync(stateFile)) {
    try {
      const stateContent = fs.readFileSync(stateFile, 'utf8');
      const state = parseState(stateContent);

      console.log(`${COLORS.BOLD}Current Position:${COLORS.RESET}`);
      console.log(`  Phase:  ${state.phase}`);
      console.log(`  Status: ${state.status}`);
      if (state.nextTask) {
        console.log(`  ${COLORS.YELLOW}Next:   ${state.nextTask}${COLORS.RESET}`);
      }
      if (state.lastActivity) {
        console.log(`  Last:   ${COLORS.DIM}${state.lastActivity}${COLORS.RESET}`);
      }
      console.log();

      if (state.progress) {
        console.log(`${COLORS.BOLD}Progress:${COLORS.RESET}`);
        console.log(`  ${state.progress}`);
        console.log();
      }
    } catch (err) {
      console.log(`${COLORS.RED}Error reading STATE.md: ${err.message}${COLORS.RESET}\n`);
    }
  } else {
    console.log(`${COLORS.DIM}Waiting for STATE.md...${COLORS.RESET}\n`);
  }

  // Read and display ralph.log
  if (fs.existsSync(logFile)) {
    try {
      const logContent = fs.readFileSync(logFile, 'utf8');
      const entries = parseRalphLog(logContent);

      // Check if last entry is RUNNING (active task)
      const lastEntry = entries[entries.length - 1];
      if (lastEntry && lastEntry.status && lastEntry.status.toUpperCase() === 'RUNNING') {
        // Show prominent "Currently Running" section
        const elapsed = Math.floor((Date.now() - new Date(lastEntry.timestamp).getTime()) / 1000);
        const elapsedStr = elapsed > 60 ? `${Math.floor(elapsed/60)}m ${elapsed%60}s` : `${elapsed}s`;
        console.log(`${COLORS.BOLD}${COLORS.CYAN}▸ Currently Running:${COLORS.RESET}`);
        console.log(`  ${COLORS.BOLD}Task ${lastEntry.task}${COLORS.RESET} (iteration #${lastEntry.iteration})`);
        console.log(`  ${COLORS.DIM}Elapsed: ${elapsedStr}${COLORS.RESET}`);
        console.log();
      }

      // Show recent completed iterations (exclude current RUNNING)
      const completedEntries = entries.filter(e => e.status && e.status.toUpperCase() !== 'RUNNING');
      const recentEntries = completedEntries.slice(-5); // Last 5 completed

      if (recentEntries.length > 0) {
        console.log(`${COLORS.BOLD}Recent Iterations:${COLORS.RESET}`);
        for (const entry of recentEntries) {
          const statusFormatted = formatStatus(entry.status);
          console.log(`  ${COLORS.CYAN}#${entry.iteration}${COLORS.RESET} ${statusFormatted}`);
          console.log(`      Task: ${COLORS.DIM}${entry.task}${COLORS.RESET}`);
          if (entry.summary) {
            console.log(`      ${entry.summary}`);
          }
          if (entry.duration && entry.duration !== '-') {
            console.log(`      ${COLORS.DIM}Duration: ${entry.duration}${COLORS.RESET}`);
          }
          console.log();
        }
      }
    } catch (err) {
      console.log(`${COLORS.RED}Error reading ralph.log: ${err.message}${COLORS.RESET}\n`);
    }
  }

  console.log(`${COLORS.DIM}───────────────────────────────────────────────────────────────────${COLORS.RESET}`);

  // Display mode and keyboard shortcuts
  const modeColor = isPaused ? COLORS.YELLOW : COLORS.GREEN;
  const modeText = isPaused ? 'PAUSED' : 'RUNNING';
  console.log(`Mode: ${modeColor}${COLORS.BOLD}[${modeText}]${COLORS.RESET}`);
  console.log(`${COLORS.DIM}Keys: [p]ause  [r]esume  [q]uit${COLORS.RESET}`);
}

// Main watch function
function watchProgress(projectRoot) {
  const stateFile = path.join(projectRoot, '.planning', 'STATE.md');
  const logFile = path.join(projectRoot, '.planning', 'ralph.log');
  const pauseFile = path.join(projectRoot, '.planning', '.pause');

  // Track pause state
  let isPaused = fs.existsSync(pauseFile);

  // Initial display
  displayProgress(stateFile, logFile, isPaused);

  // Enable keyboard input handling
  process.stdin.setRawMode(true);
  process.stdin.resume();
  process.stdin.on('data', (key) => {
    const keyStr = key.toString().toLowerCase();

    if (keyStr === 'p') {
      // Pause - create pause file
      try {
        fs.writeFileSync(pauseFile, '');
        isPaused = true;
        displayProgress(stateFile, logFile, isPaused);
      } catch (err) {
        console.error(`${COLORS.RED}Error creating pause file: ${err.message}${COLORS.RESET}`);
      }
    } else if (keyStr === 'r') {
      // Resume - delete pause file
      try {
        if (fs.existsSync(pauseFile)) {
          fs.unlinkSync(pauseFile);
        }
        isPaused = false;
        displayProgress(stateFile, logFile, isPaused);
      } catch (err) {
        console.error(`${COLORS.RED}Error deleting pause file: ${err.message}${COLORS.RESET}`);
      }
    } else if (keyStr === 'q' || key[0] === 3) {
      // Quit (q or Ctrl+C)
      cleanup();
    }
  });

  // Watch STATE.md
  let stateWatcher = null;
  if (fs.existsSync(path.dirname(stateFile))) {
    stateWatcher = fs.watch(path.dirname(stateFile), (eventType, filename) => {
      if (filename === 'STATE.md') {
        displayProgress(stateFile, logFile, isPaused);
      }
    });
  }

  // Watch ralph.log
  let logWatcher = null;
  if (fs.existsSync(path.dirname(logFile))) {
    logWatcher = fs.watch(path.dirname(logFile), (eventType, filename) => {
      if (filename === 'ralph.log') {
        displayProgress(stateFile, logFile, isPaused);
      }
    });
  }

  // Watch pause file to update state when changed externally
  let pauseWatcher = null;
  if (fs.existsSync(path.dirname(pauseFile))) {
    pauseWatcher = fs.watch(path.dirname(pauseFile), (eventType, filename) => {
      if (filename === '.pause') {
        isPaused = fs.existsSync(pauseFile);
        displayProgress(stateFile, logFile, isPaused);
      }
    });
  }

  // Graceful shutdown
  const cleanup = () => {
    console.log(`\n\n${COLORS.YELLOW}Stopping progress watcher...${COLORS.RESET}`);
    if (stateWatcher) stateWatcher.close();
    if (logWatcher) logWatcher.close();
    if (pauseWatcher) pauseWatcher.close();
    if (process.stdin.setRawMode) {
      process.stdin.setRawMode(false);
    }
    process.stdin.pause();
    process.exit(0);
  };

  process.on('SIGINT', cleanup);
  process.on('SIGTERM', cleanup);

  // Keep process alive with adaptive refresh rate
  // Faster refresh (2s) when task running, slower (10s) otherwise
  let refreshInterval = null;

  const scheduleRefresh = () => {
    // Check if a task is currently running
    let isRunning = false;
    if (fs.existsSync(logFile)) {
      try {
        const content = fs.readFileSync(logFile, 'utf8');
        const blocks = content.split('---\n').filter(b => b.trim());
        if (blocks.length > 0) {
          const lastBlock = blocks[blocks.length - 1];
          isRunning = lastBlock.includes('Status: RUNNING');
        }
      } catch (e) { /* ignore */ }
    }

    // Clear existing interval
    if (refreshInterval) clearInterval(refreshInterval);

    // Set new interval based on state
    const interval = isRunning ? 2000 : 10000;
    refreshInterval = setInterval(() => {
      isPaused = fs.existsSync(pauseFile);
      displayProgress(stateFile, logFile, isPaused);
      scheduleRefresh(); // Re-check and adjust interval
    }, interval);
  };

  scheduleRefresh();
}

// CLI
function showHelp() {
  console.log(`
${COLORS.BOLD}GSD Progress Watcher${COLORS.RESET}

Live progress display for autopilot execution.
Watches .planning/STATE.md and .planning/ralph.log for real-time updates.

${COLORS.BOLD}USAGE:${COLORS.RESET}
  progress-watcher.js [project-root]

${COLORS.BOLD}ARGUMENTS:${COLORS.RESET}
  project-root    Path to GSD project root (default: current directory)

${COLORS.BOLD}EXAMPLES:${COLORS.RESET}
  progress-watcher.js
  progress-watcher.js /path/to/project

${COLORS.BOLD}NOTES:${COLORS.RESET}
  - Press Ctrl+C to exit
  - Zero API token consumption (pure file watching)
  - Auto-launched when autopilot starts
  - Display updates on file changes + every 10 seconds
`);
}

// Entry point
if (require.main === module) {
  const args = process.argv.slice(2);

  if (args.includes('--help') || args.includes('-h')) {
    showHelp();
    process.exit(0);
  }

  const projectRoot = args[0] || process.cwd();

  if (!fs.existsSync(projectRoot)) {
    console.error(`${COLORS.RED}Error: Project root not found: ${projectRoot}${COLORS.RESET}`);
    process.exit(1);
  }

  watchProgress(projectRoot);
}

module.exports = { watchProgress };
