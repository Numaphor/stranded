# E2E Testing Guide (Windows ARM64)

This is the default local E2E workflow for this repo.

## Environment and emulator

- Use `tools/mGBA-0.10.5-win64/mGBA.exe` (Qt frontend) on Windows ARM64.
- Do not rely on `mgba-sdl.exe` automation in this repo: `gba.input.SDLB` keys are unbound in `tools/mGBA-0.10.5-win64/config.ini`.
- Build with:
  - `make -j4`
- If ROM output is locked, close emulator first:
  - `Get-Process mGBA -ErrorAction SilentlyContinue | Stop-Process -Force`

## Screenshot-grounded validation

- Use mGBA native `F12` screenshots as source of truth.
- mGBA writes PNGs in repo root as `stranded-<n>.png`.
- Prefer Win32 input automation (`SetForegroundWindow` + `keybd_event`) over `WScript.Shell.SendKeys`.
- Do full E2E screenshot verification before claiming visual fixes.

## Local key bindings used by tests (Qt)

| Key | GBA button |
|-----|------------|
| X | A |
| Z | B |
| Arrow keys | D-Pad |
| Enter | Start |
| Backspace | Select |
| A | L |
| S | R |

## Room Viewer specifics

- Room Viewer is the default boot scene (no menu flow required for normal testing).
- `START` recenters camera (it does not toggle debug overlay).
- `SELECT` toggles the Room Viewer debug overlay.
- For artifact isolation, use stationary captures:
  1. Boot to Room Viewer and press `F12` baseline.
  2. Toggle `SELECT` on and press `F12`.
  3. Toggle `SELECT` off and press `F12`.
  4. Repeat a few times without movement.

## Existing local capture sets

- `e2e_f12_boot_direct/`
- `e2e_f12_start_static/`
- `e2e_f12_select_static/`
