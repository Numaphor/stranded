---
name: stranded-gba-emulator-testing-setup
description: Enables testing the Stranded GBA game using the web-based emulator with Playwright MCP
---

# Stranded GBA Emulator Testing Setup

## Overview
This skill enables Windsurf agents to test the Stranded GBA game using the web-based emulator with Playwright MCP.

## Prerequisites
- MCP Playwright server configured
- Emulator server running at `http://localhost:8080`

## Starting the Emulator

### Check if server is running
```bash
netstat -ano | findstr ":8080"
```

### Launch server (if not running)
Use `browser_preview` tool:
```
Url: http://localhost:8080
Name: Stranded GBA Emulator
```

Then navigate with `mcp0_browser_navigate`:
```
url: http://localhost:8080
```

## Keyboard Controls

| Keyboard Key | GBA Button | Action |
|--------------|------------|--------|
| Arrow Keys | D-Pad | Movement |
| Z | A | Confirm / Action |
| X | B | Cancel / Secondary |
| A | L | Left Trigger |
| S | R | Right Trigger |
| Enter | Start | Start / Pause |
| Shift | Select | Select |

## Game Navigation Sequence

### 1. Start Game
Click the "Start Game" button on the initial screen:
```javascript
await page.getByText('Start Game').click();
```
Wait for game data to download (~3 seconds).

### 2. Main Menu
Menu shows: "Play Game" and "Controls"
- Use Arrow Keys to navigate
- Press Z (A button) to confirm

### 3. World Selection
Options: "Main World", "Forest Area"
- Press Z to select highlighted world

### 4. In-Game Actions

#### Reviving Companion
When "Press A to revive" appears, hold Z for ~8 seconds:
```javascript
async (page) => {
  const canvas = await page.locator('canvas').first();
  await canvas.click();
  await page.waitForTimeout(200);
  await page.keyboard.down('z');
  await page.waitForTimeout(8000);
  await page.keyboard.up('z');
}
```

## Playwright MCP Patterns

### Focus Game Canvas Before Input
Always click canvas before sending keyboard input:
```javascript
const canvas = await page.locator('canvas').first();
await canvas.click();
await page.waitForTimeout(200);
```

### Press Key
```javascript
await page.keyboard.press('z');
```

### Hold Key
```javascript
await page.keyboard.down('z');
await page.waitForTimeout(3000);
await page.keyboard.up('z');
```

### Take Screenshot
Use `mcp0_browser_take_screenshot` with type `png` to capture game state.

### Run Custom Code
Use `mcp0_browser_run_code` for complex interactions:
```javascript
async (page) => {
  const canvas = await page.locator('canvas').first();
  await canvas.click();
  // Your actions here
  return 'Result message';
}
```

## Common Testing Workflow

1. **Launch**: Use `browser_preview` then `mcp0_browser_navigate`
2. **Start**: Click "Start Game", wait for download
3. **Menu**: Press Z to select "Play Game"
4. **World**: Press Z to select "Main World"
5. **Play**: Use arrow keys for movement, Z for actions
6. **Screenshot**: Capture state with `mcp0_browser_take_screenshot`

## Troubleshooting

- **Keys not responding**: Click canvas first to focus it
- **Menu not advancing**: Try pressing key multiple times or use `keyboard.type('z', {delay: 100})`
- **Long actions**: Use `keyboard.down()` / `keyboard.up()` pattern with `waitForTimeout`
