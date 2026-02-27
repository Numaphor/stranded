---
name: stranded-windows-e2e-testing
description: Run local end-to-end validation for the Stranded GBA ROM on Windows ARM64 with mGBA Qt and screenshot-grounded evidence. Use when implementing or reviewing visual/gameplay changes, reproducing rendering bugs, or proving fixes with before/after emulator captures.
---

# Stranded Windows E2E Testing

## Execute Workflow

1. Build the ROM with `make -j4`.
2. Stop running mGBA if the ROM is locked: `Get-Process mGBA -ErrorAction SilentlyContinue | Stop-Process -Force`.
3. Launch `tools/mGBA-0.10.5-win64/mGBA.exe` and load the new ROM.
4. Use mGBA native `F12` screenshots as the validation evidence.
5. Run full screenshot-grounded verification before claiming visual fixes.
6. Report which screenshots demonstrate baseline state, reproduction, and fix verification.

## Enforce Testing Constraints

- Use mGBA Qt (`tools/mGBA-0.10.5-win64/mGBA.exe`) on Windows ARM64.
- Do not rely on `mgba-sdl.exe` automation in this repo.
- Prefer Win32 input automation (`SetForegroundWindow` + `keybd_event`) over `WScript.Shell.SendKeys`.
- Treat generated `stranded-<n>.png` files as source of truth.

## Load Reference

Read [references/windows-arm64-e2e.md](references/windows-arm64-e2e.md) for full command snippets, local key bindings, Room Viewer checks, and the stationary capture loop for artifact isolation.
