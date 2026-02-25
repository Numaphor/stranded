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
- VBA opens **fullscreen by default** (configured in `~/.vba/VisualBoyAdvance.cfg` with `fullScreen=1`).
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

### GDB debugging

VBA supports remote GDB debugging via its `-G tcp` flag. A convenience script `gba-debug.gdb` is provided in the project root.

```bash
# Terminal 1: launch emulator with GDB server on port 55555
DISPLAY=:1 /usr/bin/VisualBoyAdvance -G tcp /workspace/workspace.gba &

# Terminal 2: connect debugger using the convenience script
gdb-multiarch -x gba-debug.gdb /workspace/workspace.elf

# Or connect manually
gdb-multiarch -ex "set architecture arm" -ex "target remote localhost:55555" /workspace/workspace.elf
```

- The ELF file with debug symbols is at `/workspace/workspace.elf` (produced by `make`).
- **"Unknown packet vCont?" warnings are harmless** — VBA's GDB stub is older and doesn't support all modern GDB protocol extensions. Breakpoints, single-stepping (`stepi`), register/memory inspection, disassembly, and backtraces all work.
- **`continue` does not work in batch mode** — VBA's stub starts the target running on connect, so `continue` reports "Cannot execute this command while the target is running." Use `stepi` for batch-mode debugging. In interactive mode, `continue` works normally after an `interrupt`.
- mGBA also has `-g` for GDB on port 2345 and works for basic connections, but has protocol incompatibilities with GDB 15+. Use VBA for debugging.

### Lint / static analysis

There is no linter or static analysis configured. The compiler warnings from `make` serve as the primary code quality check.

### Toolchain notes

- The Wonderful Toolchain lives at `/opt/wonderful` with env var `WONDERFUL_TOOLCHAIN` pointing to it.
- `wf-pacman` packages installed: `wf-tools`, `target-gba`, `blocksds-toolchain` (provides `mmutil` for audio).
- The `butano/` directory is a git submodule — run `git submodule update --init --recursive` if it's empty.
- `python` must be available (provided by `python-is-python3` package).
