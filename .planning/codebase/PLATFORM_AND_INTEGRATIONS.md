# Platform and Integrations

Last updated: 2026-03-06

## Runtime and Toolchain

- Target platform: Game Boy Advance.
- Primary language: C++ (project configured for modern C++ through Butano build system).
- Build entrypoint: `Makefile`.
- Cross-toolchain in use:
  - Primary: Wonderful Toolchain (`/opt/wonderful` with `wf-pacman` packages: `wf-tools`, `target-gba`, `blocksds-toolchain`)
  - Alternative: local devkitARM/devkitPro environments

## Core Dependencies

- Butano engine (git submodule under `butano/`).
- varooom-3d baseline under `butano/games/varooom-3d/`.
- Project 3D overrides in `include/fr_*.h` and runtime sources in `src/viewer/`.

## Build Configuration

Key Makefile settings:

- `ROMCODE = SBTP`, `ROMTITLE = ROM TITLE`
- `USERFLAGS = -flto` (link-time optimization enabled)
- `USERLDFLAGS = -flto`
- `STACKTRACE = true` (stack trace support enabled)
- `PYTHON = python` (required for content pipeline)
- Source directories: `src`, `src/core`, `src/actors`, `src/viewer`, `butano/common/src`
- Include order: `include`, `butano/common/include`, `butano/games/varooom-3d/include`
- Graphics: `graphics`, `graphics/bg`, `graphics/sprite`, subdirectories + `graphics/shape_group_textures`
- Audio: 12 specific .wav files (death, eek, growl, hello, hum, mutant_hit, slime, slime2, steps, swipe, tablet, teleport)
- DMG audio: `dmg_audio` + `butano/common/dmg_audio`

## Local Support Tooling

### Content Pipeline (`tools/`)

| Script | Purpose |
|--------|---------|
| `create_starter_level_obj.py` | Creates level .obj files from template parameters |
| `generate_level_headers.py` | Converts OBJ files to C++ level headers |
| `model_utils.py` | Shared OBJ/MTL parsing utilities |
| `obj_to_header.py` | Standalone OBJ-to-C++ header converter |
| `generate_bg_font.py` | Generates 8x8 BMP font atlas for BG-based text rendering |
| `convert_axulart_npc.py` | Converts axulart-style NPC sprite sheets |
| `generate_wall_painting_sprites.py` | Generates triangular wall painting sprites |

### Bundled Emulator

- `tools/mGBA-0.10.5-win64/`: Bundled Windows mGBA build for local testing.
- Can be launched via Windows interop from WSL (no `wine` required).

### Dev Scripts (`scripts/`)

- `launch_emulator.js`: Build-and-run script; reads `.vscode/settings.json` for emulator path override (`stranded.emulatorPath`).
- `mgba_f12_capture.ps1`: Canonical AI E2E helper (build, launch mGBA Qt, trigger native `F12`, print screenshot path).

### Emulator Setup

- Windows: bundled mGBA build under `tools/` (or override via `stranded.emulatorPath`).
- WSL: can launch the bundled Windows mGBA build via Windows interop.
- Linux/macOS (non-WSL): requires a native emulator install (e.g., `VisualBoyAdvance` or `mgba`) and/or setting `stranded.emulatorPath`.
- AI visual validation on Windows ARM64 should use `scripts/mgba_f12_capture.ps1` and mGBA native `F12` screenshots.

## AI Agent Configuration (`.agents/`)

- `.agents/rules/butano-gba.md`: Butano GBA development guide for AI agents.
- `.agents/rules/codebase-documentation.md`: How to use `.planning/` and project references.
- `.agents/skills/stranded-vision-e2e-testing/`: Vision-only E2E testing skill for Windows ARM64 with mGBA.

## Asset Organization

### Graphics (`graphics/`)

- `bg/`: Background images (BMP + JSON metadata).
- `sprite/`: Sprite sheets by category: `creature/`, `decor/`, `enemy/`, `hud/`, `item/`, `npc/`, `player/`, `vfx/`.
- `shape_group_textures/`: 80+ texture files for 3D shape group rendering.

### Audio (`audio/`)

12 sound effects (.wav and .it format): death, eek, growl, hello, hum, mutant_hit, slime, slime2, steps, swipe, tablet, teleport.

### DMG Audio (`dmg_audio/`)

Reserved for DMG-specific audio assets. Currently empty (directory created 2025-07-14).

## CI/CD Integrations

Workflows in `.github/workflows/`:

### `build.yml`

- Triggers: push to main, pull requests
- Checks out repo + submodules
- Installs Wonderful Toolchain dependencies
- Builds ROM
- Uploads ROM artifact
- Creates release on `main` when ROM hash changes
- Publishes dev container image to GHCR when release changes are detected

### `copilot-setup-steps.yml`

- Provisions baseline Wonderful Toolchain for Copilot/Codespaces setup runs.

## External Services

- GitHub Actions for CI.
- GitHub Releases for binary distribution (`stranded.gba`).
- GitHub Container Registry for `stranded-dev` images.

## Environment Notes

- CI sets `WONDERFUL_TOOLCHAIN=/opt/wonderful`.
- Local builds depend on host toolchain setup and Butano make integration.
- Keep submodules initialized for build correctness.

## Maintenance Guidance

- If workflow behavior changes, update this file and `.planning/codebase/QUALITY_AND_TESTING.md` together.
- Keep this file focused on infrastructure facts, not gameplay behavior.
