# Windows ARM64 E2E Reference

## Environment And Emulator

- Use `tools/mGBA-0.10.5-win64/mGBA.exe` (Qt frontend) on Windows ARM64.
- Avoid `mgba-sdl.exe` automation in this repo because `gba.input.SDLB` keys are unbound in `tools/mGBA-0.10.5-win64/config.ini`.
- Build with `make -j4`.
- If ROM output is locked, close emulator first:
  - `Get-Process mGBA -ErrorAction SilentlyContinue | Stop-Process -Force`

## Screenshot-Grounded Validation

- Use mGBA native `F12` screenshots as source of truth.
- mGBA writes screenshots in repo root as `stranded-<n>.png`.
- Prefer Win32 input automation (`SetForegroundWindow` + `keybd_event`) over `WScript.Shell.SendKeys`.
- Complete full E2E screenshot verification before claiming visual fixes.

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

## Room Viewer Specifics

- Use Room Viewer as the default boot scene (no menu flow required for standard checks).
- Use `START` to recenter camera (do not treat it as debug overlay toggle).
- Use `SELECT` to toggle Room Viewer debug overlay.

## Artifact Isolation Capture Loop

1. Boot to Room Viewer and press `F12` for baseline capture.
2. Toggle `SELECT` on and press `F12`.
3. Toggle `SELECT` off and press `F12`.
4. Repeat without movement to isolate rendering artifacts.

## Existing Local Capture Sets

- `e2e_f12_boot_direct/`
- `e2e_f12_start_static/`
- `e2e_f12_select_static/`
