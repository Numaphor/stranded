# Vision-Only E2E Reference

## Environment And Emulator

- Use `tools/mGBA-0.10.5-win64/mGBA.exe` (Qt frontend) on Windows ARM64.
- Run canonical capture: `powershell -ExecutionPolicy Bypass -File scripts/mgba_f12_capture.ps1`.
- Run repeat capture without rebuild: `powershell -ExecutionPolicy Bypass -File scripts/mgba_f12_capture.ps1 -SkipBuild`.
- Use the printed absolute `stranded-<n>.png` path as the artifact path.

## Vision-Only Evidence Policy

- Inspect screenshot pixels directly and base conclusions on visible output only.
- Use before/after screenshot comparison for reproduction and fix validation.
- Never use file size, byte count, metadata, filename order, or checksums to claim rendering correctness.
- Never substitute browser/Playwright/desktop screenshots for mGBA native `F12` output.

## Local Key Bindings (mGBA Qt)

| Key | GBA Button |
| --- | --- |
| X | A |
| Z | B |
| Arrow keys | D-Pad |
| Enter | Start |
| Backspace | Select |
| A | L |
| S | R |

## Room Viewer Capture Loop

1. Boot to Room Viewer and press `F12` for baseline.
2. Toggle `SELECT` on and press `F12`.
3. Toggle `SELECT` off and press `F12`.
4. Repeat the same captures without movement to isolate rendering artifacts.

## Reporting Format

1. Reference screenshot paths for baseline, reproduction, and fix.
2. Describe only visible differences and remaining artifacts.
3. If a screenshot is unclear, capture again instead of inferring from non-visual data.
