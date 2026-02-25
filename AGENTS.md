# AGENTS.md

## Cursor Cloud specific instructions

### Project overview

**Stranded** is a GBA homebrew game built with the **Butano** C++ engine. The build produces a `.gba` ROM file. There are no automated tests — validation is done by building the ROM and running it in an emulator. See `README.md` for full project structure and controls.

### Build

```bash
export WONDERFUL_TOOLCHAIN=/opt/wonderful
export PATH="/opt/wonderful/bin:/opt/wonderful/toolchain/gcc-arm-none-eabi/bin:$PATH"
make -j$(nproc)
```

The output ROM is named after the workspace directory (e.g. `workspace.gba` in Cloud, `stranded.gba` locally). Clean build: `make clean && make -j$(nproc)`.

### Running the ROM

- **Use `VisualBoyAdvance`** (`/usr/bin/VisualBoyAdvance`) — it works reliably in this headless VM.
- **Do NOT use `mgba-qt`** — it has an OpenGL compositing bug in this VM environment that renders a blank screen even with software fallback. The ROM loads (title bar shows "ROM TITLE" and FPS) but the display widget never paints.
- Launch: `DISPLAY=:1 /usr/bin/VisualBoyAdvance /workspace/workspace.gba &`
- ALSA audio errors are harmless (no sound card in the VM).

### Emulator key bindings (VisualBoyAdvance defaults)

| Key | GBA Button |
|-----|-----------|
| Z | A |
| X | B |
| Arrow keys | D-Pad |
| Enter | Start |
| Backspace | Select |
| A | L shoulder |
| S | R shoulder |

### Lint / static analysis

There is no linter or static analysis configured. The compiler warnings from `make` serve as the primary code quality check.

### Toolchain notes

- The Wonderful Toolchain lives at `/opt/wonderful` with env var `WONDERFUL_TOOLCHAIN` pointing to it.
- `wf-pacman` packages installed: `wf-tools`, `target-gba`, `blocksds-toolchain` (provides `mmutil` for audio).
- The `butano/` directory is a git submodule — run `git submodule update --init --recursive` if it's empty.
- `python` must be available (provided by `python-is-python3` package).
