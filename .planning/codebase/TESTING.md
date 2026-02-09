# Testing Patterns

**Analysis Date:** 2026-02-09

## Test Framework

**Runner:** Manual testing via mGBA emulator
**Config:** No automated test configuration detected

**Run Commands:**
```bash
make                    # Build ROM
make clean && make     # Clean build
# Launch via scripts/launch_emulator.js or tools/mGBA-0.10.5-win64/mGBA.exe
```

## Test File Organization

- **Location:** No dedicated test files in project code
- **Naming:** No test-specific naming pattern detected
- **Structure:** Manual testing through emulator interaction

## Test Structure

**Manual testing approach:**
```cpp
// Debug builds with stack traces enabled
STACKTRACE := true

// Runtime logging to emulator console
bn::core::log("Debug message");
```

## Mocking

- **Framework:** None detected
- **What to mock:** No mocking pattern observed
- **Pattern:** Manual testing with real game entities

## Fixtures and Test Data

- **Location:** No test-specific fixtures
- **Pattern:** Real game assets and levels used for testing
- **Test scenarios:** Specific world configurations for different test cases

## Coverage

- **Requirements:** None enforced
- **Gaps:** No automated coverage measurement
- **Critical paths:** Manual testing focused on gameplay mechanics

## Test Types Present

- **Unit tests:** Not present - no automated unit testing framework
- **Integration tests:** Limited - manual testing of scene transitions and entity interactions
- **E2E tests:** Manual - complete gameplay testing in emulator
- **Manual testing:** Primary approach - mGBA emulator with real ROM

**Emulator Testing Setup:**
- mGBA 0.10.5 included in tools/ directory
- JavaScript launcher script available
- Debug logging via emulator console
- Stack trace support enabled in debug builds

**Manual Test Areas:**
- Scene navigation (start -> menu -> world)
- Player movement and combat
- Enemy AI and collision
- Save/load functionality
- Performance on real hardware constraints

---
*Testing analysis: 2026-02-09*