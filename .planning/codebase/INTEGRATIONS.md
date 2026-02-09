# External Integrations

**Analysis Date:** 2026-02-09

## APIs & External Services

**Version Control:**
- GitHub - Source code hosting and CI/CD
  - SDK/Client: Git CLI
  - Auth: SSH/GitHub tokens
  - Features: Automated builds, releases, container publishing

## Data Storage

**Databases:**
- None (embedded game system - data stored in ROM)

**File Storage:**
- ROM file system - All assets compiled into binary
- GitHub Releases - ROM distribution
- GitHub Container Registry - Docker images

**Caching:**
- GitHub Actions cache - Toolchain dependencies caching

## Authentication & Identity

**Auth Provider:**
- GitHub Actions - CI/CD authentication
  - Implementation: GitHub tokens for release management
  - Container Registry: GitHub Container Registry with token auth

## Monitoring & Observability

**Error Tracking:**
- None (embedded system - debug via emulator logging)

**Logs:**
- Butano logging framework - Emulator console output
- Build process logs - GitHub Actions workflow logs

## CI/CD & Deployment

**Hosting:**
- GitHub - Source code and releases
- GitHub Container Registry - Docker development environment

**CI Pipeline:**
- GitHub Actions workflows
  - Build validation on push/PR
  - Automated ROM generation
  - Release creation with hash-based versioning
  - Docker environment publishing

## Environment Configuration

**Required env vars:**
- WONDERFUL_TOOLCHAIN - Toolchain installation path
- GITHUB_TOKEN - Release and container publishing auth

**Secrets location:**
- GitHub Actions secrets
- GitHub Container Registry

## Webhooks & Callbacks

**Incoming:**
- GitHub webhook triggers - Push/PR events
- None for runtime (embedded game)

**Outgoing:**
- GitHub Releases API - ROM publishing
- Container Registry API - Docker image publishing

## Development Toolchain

**Build Tools:**
- Wonderful Toolchain - ARM cross-compilation
- Butano Asset Tools - Graphics/audio processing
- mGBA - Development and testing emulator

**Asset Processing:**
- Butano graphics tool - BMP to GBA format conversion
- Butano audio tool - Module music processing
- Python PIL - Custom font generation

**Container Integration:**
- Docker development environment with pre-installed toolchain
- Automated container publishing on ROM changes
- Complete development environment encapsulation

---

*Integration audit: 2026-02-09*