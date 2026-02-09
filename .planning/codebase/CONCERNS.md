# Codebase Concerns

**Analysis Date:** 2026-02-09

## Tech Debt

**Memory Management in World Class:**
- Issue: Manual new/delete usage without RAII or smart pointers
- Files: `src/core/world.cpp` (lines 140, 146-149, 172, 190, 381-382, 477, 484, 494, 501)
- Impact: Potential memory leaks if exceptions occur or during complex cleanup scenarios
- Fix approach: Replace raw pointers with `bn::unique_ptr` or implement proper RAII wrappers

**Large Monolithic Functions:**
- Issue: `World::execute()` function is over 300 lines long with complex nested logic
- Files: `src/core/world.cpp` (lines 152-470)
- Impact: Difficult to maintain, test, and understand. High cyclomatic complexity.
- Fix approach: Break into smaller focused methods for camera handling, enemy updates, input processing, etc.

**Hard-coded Magic Numbers:**
- Issue: Extensive use of hard-coded numerical values throughout codebase
- Files: All source files contain numerous hard-coded values (coordinates, timers, sizes)
- Impact: Difficult to tune game balance and maintain consistency
- Fix approach: Move more constants to `str_constants.h` or create configuration structs

## Known Bugs

**Sword Background Disabled:**
- Symptoms: Sword rendering functionality is commented out throughout world code
- Files: `src/core/world.cpp` (lines 121, 178-188, 299-302, 388-402)
- Trigger: Sword attacks don't show visual effects
- Workaround: None - feature is intentionally disabled

**Camera Zoom Sprite Positioning:**
- Symptoms: Complex affine matrix calculations may cause sprite positioning errors
- Files: `src/core/world.cpp` (lines 403-468)
- Trigger: During zoom transitions with multiple sprites
- Workaround: Avoid rapid zoom changes with many active sprites

## Security Considerations

**Input Handling Validation:**
- Risk: Complex input handling in Player class may miss edge cases
- Files: `src/actors/player.cpp` (lines 992-1240)
- Impact: Could lead to unintended game states or crashes
- Current mitigation: Basic state checks before processing input
- Recommendations: Add more comprehensive input validation and state consistency checks

## Performance Bottlenecks

**Frame-by-Frame Sprite Operations:**
- Problem: Every frame processes all sprites for zoom effects
- Files: `src/core/world.cpp` (lines 431-441)
- Cause: No dirty flag system to skip unchanged sprites
- Improvement path: Implement sprite dirty state tracking

**Vector Operations with Fixed Size Limits:**
- Problem: Linear searches in enemy and bullet vectors
- Files: `src/core/world.cpp` (lines 336-372, 431-441)
- Cause: No spatial partitioning or indexing
- Improvement path: Implement spatial grid for collision detection

**String Operations in UI:**
- Problem: Frequent string concatenation in menu systems
- Files: `src/core/scenes.cpp` (lines 47-52, 102-107)
- Cause: Building UI text every frame regardless of changes
- Improvement path: Cache UI text until state changes

## Fragile Areas

**World::execute() Main Loop:**
- Files: `src/core/world.cpp` (lines 196-469)
- Why fragile: Contains all game logic in one massive function, prone to breaking with small changes
- Safe modification: Extract focused helper methods one at a time
- Test coverage: No unit tests - relies on manual integration testing

**Player Input Handling:**
- Files: `src/actors/player.cpp` (lines 992-1240)
- Why fragile: Complex nested conditionals for combo detection and state transitions
- Safe modification: Create state machine for input handling with clear transitions
- Test coverage: No automated tests for edge cases

**Zoom and Camera System:**
- Files: `src/core/world.cpp` (lines 307-335, 403-468)
- Why fragile: Complex coordinate transformations during zoom transitions
- Safe modification: Separate zoom logic into dedicated class with clear interface
- Test coverage: Limited testing edge cases for sprite positioning

## Scaling Limits

**Enemy Count:**
- Current capacity: Limited by `bn::vector<Enemy, 16>` (hard-coded 16 enemy limit)
- Limit: Game will crash or behave unexpectedly with >16 enemies
- Scaling path: Make enemy capacity configurable with performance monitoring

**Sprite Memory:**
- Current capacity: Limited by GBA hardware sprite limits
- Limit: Sprite overflow causes clipping or disappearing sprites
- Scaling path: Implement sprite pooling and priority-based culling

**Background Map Size:**
- Current capacity: 128x128 tiles (1024x1024 pixels) - maximum for affine backgrounds
- Limit: Cannot increase world size without switching to different background type
- Scaling path: Consider big backgrounds or multi-screen tiling system

## Dependencies at Risk

**Butano Engine Version:**
- Risk: Butano is actively developed with potential breaking changes
- Impact: API changes could require significant code updates
- Migration plan: Pin to specific version, create compatibility layer

**DevkitARM/Wonderful Toolchain:**
- Risk: Toolchain updates may break build system
- Impact: Could prevent compilation and deployment
- Migration plan: Use containerized build environment, test toolchain updates in isolation

## Missing Critical Features

**Save Game System:**
- Problem: World state only saved in memory, lost on application restart
- Blocks: Persistent game progress, longer play sessions
- Current: `WorldStateManager` only handles session persistence

**Configuration System:**
- Problem: No way to tune game parameters without recompiling
- Blocks: Game balance adjustments, accessibility options
- Current: All values hard-coded in source

**Error Handling:**
- Problem: Limited error handling for out-of-memory or resource failures
- Blocks: Robust operation on hardware with limited resources
- Current: Assumes all allocations succeed

## Test Coverage Gaps

**Unit Tests:**
- What's not tested: Individual class methods, utility functions, state transitions
- Files: All source files lack unit test coverage
- Risk: Refactoring may introduce subtle bugs
- Priority: High - core game logic needs test coverage

**Integration Tests:**
- What's not tested: Scene transitions, save/load operations, complex input sequences
- Files: Scene system, world state management
- Risk: Critical game flows may break unnoticed
- Priority: Medium - manual testing catches major issues

**Performance Tests:**
- What's not tested: Memory usage, frame rate under load, scaling behavior
- Files: Performance-critical areas like sprite management
- Risk: Performance regressions go unnoticed until deployment
- Priority: Low - GBA hardware constraints make this critical

---

*Concerns audit: 2026-02-09*