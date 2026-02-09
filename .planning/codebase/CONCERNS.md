# Codebase Concerns

**Analysis Date:** 2026-02-09

## Tech Debt

**Sword Background System:**
- Issue: Sword background system temporarily disabled with commented code
- Files: `src/core/world.cpp` (lines 121, 178-188)
- Impact: Missing visual sword effect, commented code blocks maintain complexity
- Fix approach: Either remove sword system completely or re-enable with proper implementation

**Manual Memory Management:**
- Issue: Mixed use of new/delete without RAII or smart pointers
- Files: `src/core/world.cpp` (Level, Minimap, MerchantNPC allocations)
- Impact: Potential memory leaks if exceptions occur or in error paths
- Fix approach: Consider bn::optional or RAII wrappers for dynamic allocations

## Known Bugs

**None explicitly documented** - No TODO/FIXME/HACK comments found in codebase

## Security Considerations

**Asset Security:**
- Risk: Embedded assets in ROM could be extracted
- Files: All graphics/audio assets in ROM binary
- Current mitigation: Standard GBA ROM protection
- Recommendation: Consider asset obfuscation if needed for commercial release

## Performance Bottlenecks

**Large World Update Loop:**
- Problem: World::execute() method is ~500+ lines with complex logic
- Files: `src/core/world.cpp` (main game loop)
- Cause: Single monolithic update function handling camera, zoom, entities, collision
- Improvement: Break down into smaller, focused update methods

**Complex Camera/Zoom System:**
- Problem: Zoom and affine transformation logic scattered throughout update loop
- Files: `src/core/world.cpp` (lines 208-238, 310-335, 403-468)
- Cause: Manual affine matrix management with complex conditional logic
- Improvement: Extract to dedicated CameraManager or ZoomSystem class

**Manual Sprite Position Updates:**
- Problem: Each sprite manually updated for zoom every frame
- Files: `src/core/world.cpp` (lines 406-441)
- Cause: Direct sprite manipulation in main loop
- Improvement: Create sprite group management system

## Fragile Areas

**World::execute() Method:**
- Files: `src/core/world.cpp`
- Why fragile: 500+ line method with many responsibilities and nested conditionals
- Safe modification: Extract camera, zoom, and entity update logic to separate methods
- Test coverage: Manual testing only - changes risk breaking core gameplay

**Memory Management in Constructors:**
- Files: `src/core/world.cpp` (constructor and destructor)
- Why fragile: Manual new/delete without exception safety
- Safe modification: Use RAII or bn::optional for dynamic allocations
- Test coverage: Limited - memory issues hard to test manually

**Affine Matrix Management:**
- Files: `src/core/world.cpp` (zoom system)
- Why fragile: Complex state management for multiple affine matrices
- Safe modification: Create dedicated zoom management class
- Test coverage: Manual testing of zoom transitions

## Scaling Limits

**Entity Count:**
- Current capacity: Limited by bn::vector sizes (e.g., 32 bullets)
- Limit: GBA sprite hardware limits (OAM entries)
- Scaling path: Implement entity pooling, optimize sprite usage

**Background Complexity:**
- Current capacity: 128x128 tile maps with random generation
- Limit: GBA VRAM and tile memory constraints
- Scaling path: Use big backgrounds or streaming for larger worlds

## Dependencies at Risk

**Butano Engine:**
- Risk: Submodule could diverge or have breaking changes
- Impact: Entire game depends on Butano APIs
- Migration: Keep butano submodule pinned, track upstream changes

**mGBA Emulator:**
- Risk: Version compatibility for testing
- Impact: Development workflow
- Migration: Update to newer mGBA versions as needed

## Missing Critical Features

**Automated Testing:**
- Problem: No unit tests or automated integration tests
- Blocks: Reliable refactoring, regression detection
- Impact: High risk for code changes

**Error Handling:**
- Problem: Limited error handling for edge cases
- Blocks: Robust gameplay experience
- Impact: Medium - crashes possible in edge cases

**Configuration System:**
- Problem: Hardcoded constants throughout codebase
- Blocks: Easy tweaking of game balance
- Impact: Low - constants consolidated in str_constants.h

## Test Coverage Gaps

**Core Game Mechanics:**
- What's not tested: Player movement, combat, collision detection
- Files: `src/actors/player.cpp`, `src/core/collision.cpp`
- Risk: High - gameplay bugs could go unnoticed
- Priority: High

**Scene Transitions:**
- What's not tested: Menu navigation, save/load, state persistence
- Files: `src/core/scenes.cpp`, `src/core/world_state.cpp`
- Risk: Medium - navigation bugs affect game flow
- Priority: Medium

**Performance Edge Cases:**
- What's not tested: Memory limits, sprite overflow, maximum entity counts
- Files: Memory allocation patterns throughout codebase
- Risk: High - GBA hardware constraints are strict
- Priority: High

---
*Concerns audit: 2026-02-09*