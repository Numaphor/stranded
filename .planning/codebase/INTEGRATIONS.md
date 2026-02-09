# External Integrations

**Analysis Date:** 2026-02-09

## APIs & External Services

**No external APIs detected** - This is an offline GBA game with no network connectivity.

## Data Storage

**Databases:**
- No databases - GBA games use SRAM for save data only
- SRAM integration: Via Butano's SRAM examples (butano/examples/sram/)

**File Storage:**
- Local filesystem only - All assets compiled into ROM binary
- BMP format images converted by Grit to GBA format
- MOD/XM/S3M/IT audio files converted by mmutil

**Caching:**
- Hardware-level caching - IWRAM (32KB fast) and EWRAM (256KB slow)
- bn::memory management for RAM monitoring

## Authentication & Identity

- None - Single-player offline game
- No user accounts or authentication required

## Monitoring & Observability

- Error Tracking: bn::core::log() for debug output to emulator
- Logging: Console-style logging via emulator debug windows
- Stack traces: Enabled via STACKTRACE := true in Makefile
- Profiling: Available via Butano profiler examples

## CI/CD & Deployment

- Hosting: GitHub Actions for build validation (.github/workflows/build.yml)
- CI Pipeline: Automated build testing
- Deployment: Manual ROM distribution for GBA hardware/emulators

## Environment Configuration

**Required env vars / config:**
- LIBBUTANO path in Makefile (relative to butano/butano/)
- ROMTITLE and ROMCODE for GBA header
- STACKTRACE := true for debugging
- USERFLAGS for custom compiler options

## Webhooks & Callbacks

- Incoming: None (offline embedded target)
- Outgoing: None (no network capabilities)

---
*Integration audit: 2026-02-09*