# Testing Patterns

**Analysis Date:** 2026-02-09

## Test Framework

**Runner:**
- No project-specific test framework detected
- Uses Butano engine's built-in test framework for engine validation
- Manual testing through GBA emulators (mGBA, NanoBoyAdvance)

**Assertion Library:**
- Butano's `BN_ASSERT()` macro for runtime assertions
- Stack trace support enabled via `STACKTRACE := true` in Makefile
- Debug logging via `bn::core::log()`

**Run Commands:**
```bash
make clean && make -j$(nproc)     # Build ROM for testing
# ROM loaded manually in GBA emulator for manual testing
```

## Test File Organization

**Location:**
- No dedicated test directory in project structure
- Tests located in Butano library subdirectory: `butano/tests/`
- Manual testing through emulator with controller/keyboard input

**Naming:**
- Test files in Butano follow pattern: `*_tests.h` (e.g., `string_tests.h`, `memory_tests.h`)
- Project-specific tests not detected

**Structure:**
```
butano/tests/
├── general_tests/
│   ├── include/
│   │   ├── tests.h (base test class)
│   │   ├── string_tests.h
│   │   ├── memory_tests.h
│   │   └── [other_test_files].h
│   └── src/
└── [other_test_categories]/
```

## Test Structure

**Suite Organization:**
```
class tests
{
    virtual void run() = 0;
};

class specific_tests : public tests
{
    void run() override
    {
        // Test implementations
        BN_ASSERT(condition, "message");
    }
};
```

**Patterns:**
- Base `tests` class with virtual `run()` method
- Individual test classes inherit from base class
- `BN_ASSERT()` macro for assertions with optional error messages
- Test methods focus on specific functionality areas

**Setup/Teardown:**
- Constructor for test initialization
- Destructor for cleanup
- No explicit setUp/tearDown methods observed

## Mocking

**Framework:** No mocking framework detected

**Patterns:**
- Friend class pattern for controlled access to private members
- Forward declarations to avoid circular dependencies
- Dependency injection through constructor parameters
- State machines use dependency inversion principle

**What to Mock:**
- Not applicable in this low-level GBA development context
- Hardware interfaces abstracted through Butano engine

**What NOT to Mock:**
- Butano engine components (they provide the abstraction layer)
- Hardware-specific code handled by engine

## Fixtures and Factories

**Test Data:**
```
struct test_data
{
    // Test-specific data structures
};
```

**Location:**
- Test fixtures defined within individual test classes
- No external fixture files detected
- Test data created inline within test methods

## Coverage

**Requirements:** No coverage requirements enforced

**View Coverage:**
- No coverage tools configured
- Coverage likely measured through manual testing completeness
- Critical paths: scene transitions, collision detection, player movement

## Test Types

**Unit Tests:**
- Located in Butano engine tests directory
- Focus on engine core functionality (memory, strings, math, etc.)
- Test individual components in isolation

**Integration Tests:**
- Manual testing through emulator gameplay
- Test game mechanics, scene transitions, audio
- Validate complete game flow from start screen to gameplay

**E2E Tests:**
- Full gameplay testing in GBA emulator
- Manual test scenarios for game completion
- Performance testing on actual hardware constraints

## Common Patterns

**Manual Testing Workflow:**
```cpp
// Debug output for testing
bn::core::log("Debug message");

// Stack trace on assertion failure
BN_ASSERT(condition, "Error message");
```

**Error Testing:**
```cpp
// Stack trace configuration
STACKTRACE := true  // in Makefile

// Assertion-based testing
BN_ASSERT(memory_usage < limit, "Memory limit exceeded");
```

**Game State Testing:**
- Scene transition testing through manual navigation
- Collision detection testing in gameplay scenarios
- Save state testing through SRAM operations (if implemented)

## Development Testing Practices

**Debug Tools:**
- mGBA emulator with debug capabilities
- No$gba debug version for advanced debugging
- Stack trace logging enabled for crash analysis
- bn::core::log() for runtime debugging

**Hardware Testing:**
- ROM testing on actual GBA hardware via flash cart
- Performance validation under real hardware constraints
- Audio testing on GBA sound hardware
- SRAM persistence testing for save functionality

**CI Testing:**
- Automated ROM building and validation in GitHub Actions
- ROM hash comparison for release validation
- Build verification across toolchain versions
- Artifact generation for manual testing

---

*Testing analysis: 2026-02-09*