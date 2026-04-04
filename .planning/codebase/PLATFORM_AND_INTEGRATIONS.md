# Platform and Integrations

Last updated: 2026-04-04

## Build and Runtime

- Target platform: Game Boy Advance.
- Primary build entrypoint: `Makefile`.
- Primary engine dependency: the Butano submodule.
- Current baseline uses the Wonderful Toolchain layout described in the repo
  build notes.
- Project include paths must include `butano/butano/hw/include` when code
  directly includes Butano hardware headers such as `bn_hw_sprites.h`.

## Local Tooling

- `tools/mGBA-0.10.5-win64/` provides the bundled Windows emulator.
- `scripts/mgba_f12_capture.ps1` is the local helper for build/run/capture
  checks when a visual review is needed.
- When `scripts/mgba_f12_capture.ps1` runs through Windows PowerShell against a
  WSL checkout, it must use the raw UNC provider path (`ProviderPath`) instead
  of the PowerShell `FileSystem::` display path so lock-file creation works.
- The same helper stages the bundled Windows mGBA build into
  `%LOCALAPPDATA%\Stranded\mGBA-0.10.5-win64\` before launch when the source
  emulator path lives on a `\\wsl.localhost\...` share; this avoids Windows
  "Open File - Security Warning" prompts during automated F12 capture runs.
- `scripts/launch_debug.sh` supports debugger and logging workflows.
- The VS Code C/C++ config currently relies on explicit `includePath` entries
  (including Butano HW and 3rd-party include roots) instead of depending on a
  frequently stale `build/compile_commands.json`.

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
