---
name: stranded-vision-e2e-testing
description: Run vision-only end-to-end validation for Stranded on Windows ARM64 using mGBA Qt native F12 screenshots. Use when reproducing rendering bugs, verifying visual fixes, checking launch/rendering setup, or reviewing emulator output. Capture through scripts/mgba_f12_capture.ps1 and evaluate screenshots by direct visual inspection only; never use file size, byte counts, metadata, hashes, or non-mGBA capture tools as visual evidence.
---

# Stranded Vision E2E Testing

## Run Canonical Capture

1. Run `powershell -ExecutionPolicy Bypass -File scripts/mgba_f12_capture.ps1`.
2. Run `powershell -ExecutionPolicy Bypass -File scripts/mgba_f12_capture.ps1 -SkipBuild` for repeat captures.
3. Use the printed absolute `stranded-<n>.png` path as the screenshot artifact.
4. Capture baseline, reproduction, and fix-verification screenshots for every visual bug report.

## Analyze With Vision Only

1. Open screenshots and inspect rendered pixels directly.
2. Compare baseline and fix screenshots by visible content only.
3. Report observed artifacts, layout errors, and regressions from visual inspection only.
4. Reject non-visual evidence such as file size, byte count, metadata, filename patterns, or image hashes.

## Enforce Capture Rules

- Use mGBA Qt (`tools/mGBA-0.10.5-win64/mGBA.exe`) with native `F12` capture only.
- Do not use browser screenshots, Playwright screenshots, desktop capture tools, or HTML/canvas snapshots.
- Do not use `mgba-sdl.exe` automation in this repository as visual evidence.
- Treat generated `stranded-<n>.png` screenshots as the single source of truth for rendering checks.

## Load Reference

Read [references/vision-only-e2e.md](references/vision-only-e2e.md) for key bindings, room-viewer capture loops, and reporting format.
