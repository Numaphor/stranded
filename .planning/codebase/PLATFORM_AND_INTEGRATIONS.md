# Platform and Integrations

Last updated: 2026-03-22

## Build and Runtime

- Target platform: Game Boy Advance.
- Primary build entrypoint: `Makefile`.
- Primary engine dependency: the Butano submodule.
- Current baseline uses the Wonderful Toolchain layout described in the repo
  build notes.

## Local Tooling

- `tools/mGBA-0.10.5-win64/` provides the bundled Windows emulator.
- `scripts/mgba_f12_capture.ps1` is the local helper for build/run/capture
  checks when a visual review is needed.
- `scripts/launch_debug.sh` supports debugger and logging workflows.

## Asset and Content Pipeline

- `graphics/` and `audio/` contain the inputs used by the surviving room-viewer
  build.
- `tools/` contains the local content helpers that still support the current
  asset pipeline.
- `dmg_audio/` is only needed if the build still references it.

## Notes

- This repository is validated locally.
- CI/workflow documentation has been intentionally removed from the planning
  baseline.
