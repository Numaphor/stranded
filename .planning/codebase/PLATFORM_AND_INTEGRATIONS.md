# Platform and Integrations

Last updated: 2026-02-22

## Runtime and Toolchain

- Target platform: Game Boy Advance.
- Primary language: C++ (project configured for modern C++ through Butano build system).
- Build entrypoint: `Makefile`.
- Cross-toolchain in use:
  - local environments may use devkitARM/devkitPro
  - CI uses Wonderful Toolchain bootstrap + `wf-pacman` packages

## Core Dependencies

- Butano engine (git submodule under `butano/`).
- varooom-3d baseline under `butano/games/varooom-3d/`.
- Project 3D overrides in `include/fr_*.h` and runtime sources in `src/viewer/`.

## Local Support Tooling

- Python scripts in `tools/` for content/header generation.
- Node script `scripts/launch_emulator.js` for local emulator workflow.
- mGBA used for emulator-based validation.

## CI/CD Integrations

Workflows in `.github/workflows/`:

- `build.yml`
  - checks out repo + submodules
  - installs toolchain dependencies
  - builds ROM
  - uploads ROM artifact
  - creates release on `main` when ROM hash changes
  - publishes dev container image to GHCR when release changes are detected
- `copilot-setup-steps.yml`
  - provisions baseline toolchain for Copilot setup runs

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
