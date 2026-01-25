# Codebase Concerns

**Analysis Date:** 2026-01-24

## Tech Debt

**Memory Management with Raw Pointers:**
- Issue: Manual memory management using raw `new`/`delete` pointers throughout codebase
- Files: `src/core/world.cpp`, `src/core/world_object.cpp`, `src/actors/enemy.cpp`
- Impact: High risk of memory leaks, double-free, and crashes on GBA's limited memory
- Fix approach: Replace raw pointers with Butano's smart pointers (`bn::unique_ptr`) and automatic memory management

**Disabled Collision System:**
- Issue: World object collision detection is completely disabled with TODO comment
- Files: `src/core/level.cpp` (line 152-156)
- Impact: Players can clip through world objects, breaking game mechanics
- Fix approach: Enable collision system once world objects are properly initialized and add comprehensive test coverage

**Incomplete Asset Implementation:**
- Issue: Sword object exists but has no visual sprites, commented out with TODO
- Files: `src/core/world_object.cpp` (line 213-218)
- Impact: Critical game object is invisible to players
- Fix approach: Create sword sprite assets and uncomment sprite creation code

## Performance Bottlenecks

**Large Monolithic Files:**
- Issue: Several files exceed 1000 lines, indicating complexity and potential performance issues
- Files: `src/actors/player.cpp` (1359 lines), `src/core/hud.cpp` (844 lines), `src/core/world.cpp` (678 lines)
- Impact: Slower compilation, potential GBA cache issues, difficult maintenance
- Improvement path: Break into smaller, focused modules with clear responsibilities

**Static Data in EWRAM:**
- Issue: Large static buffers allocated in EWRAM for world map data
- Files: `src/core/world.cpp` (line 61, 73) - 16,384 cell buffer
- Impact: Consumes precious EWRAM (256KB total), may limit other features
- Improvement path: Consider streaming approaches or compression for large static data

## GBA Hardware Constraints

**Memory Layout Concerns:**
- Issue: No `.bn_iwram.cpp` files for performance-critical code in IWRAM
- Files: All source files use default memory sections
- Impact: Critical functions may run slower than optimal, wasting IWRAM benefits
- Fix approach: Identify performance-critical functions and move to IWRAM sections

**No Memory Section Optimization:**
- Issue: Code doesn't leverage GBA's memory hierarchy (IWRAM/EWRAM optimizations)
- Files: Project source files
- Impact: Suboptimal performance on hardware with tight memory constraints
- Fix approach: Profile hot paths and strategically place code in fast memory sections

## Scalability Issues

**Hardcoded World Limits:**
- Issue: World size hardcoded to 1024x1024 tiles (8192x8192 pixels)
- Files: `include/str_constants.h` (lines 205-210)
- Impact: Cannot support larger worlds without extensive refactoring
- Scaling path: Implement dynamic world sizing or multiple world zones

**Fixed View Buffer:**
- Issue: View buffer hardcoded to 128x128 tiles matching GBA affine BG limits
- Files: `include/str_constants.h` (lines 202-203)
- Impact: No flexibility for different rendering approaches
- Scaling path: Abstract view buffer management to support multiple configurations

## Maintenance Challenges

**TODO Comments Without Tracking:**
- Issue: Two TODO comments indicate incomplete features but no issue tracking
- Files: `src/core/level.cpp`, `src/core/world_object.cpp`
- Impact: Incomplete features may be forgotten, technical debt accumulation
- Fix approach: Link TODOs to actual issue tracker or use Butano's task system

**Complex Constants File:**
- Issue: Single constants file with 217 lines mixing different concerns
- Files: `include/str_constants.h`
- Impact: Hard to maintain, risk of unrelated constant conflicts
- Fix approach: Split into focused header files by system (HUD, world, player, etc.)

## Fragile Areas

**Chunk Streaming System:**
- Files: `src/core/chunk_manager.cpp` (462 lines)
- Why fragile: Complex streaming logic with state management, buffer coordination, and error conditions
- Safe modification: Add comprehensive logging, create unit tests for edge cases
- Test coverage: Only assertion-based validation, no automated tests

**World State Management:**
- Files: `src/core/world_state.cpp`
- Why fragile: Global state coordination between multiple subsystems
- Safe modification: Use state machines or command patterns to encapsulate transitions
- Test coverage: Unknown, likely minimal due to complexity

## Security Considerations

**No Input Validation:**
- Risk: Player input and game state modifications lack validation
- Files: Input handling throughout player movement and interaction code
- Current mitigation: Butano framework provides basic validation
- Recommendations: Add bounds checking for all player position and game state changes

**Save Game Integrity:**
- Risk: Save game data could be corrupted or manipulated
- Files: Save/load functionality (not directly visible in core files)
- Current mitigation: Butano's SRAM abstraction
- Recommendations: Implement checksum validation for save data

## Platform-Specific Limitations and Workarounds

**GBA Audio Constraints:**
- Issue: Complex audio system but GBA has limited sound channels
- Files: Audio configuration in Makefile, butano audio integration
- Impact: May hit channel limits or have to prioritize sounds
- Workarounds: Implement sound priority system, channel management

**Sprite Limit Handling:**
- Risk: No visible sprite count management despite GBA's sprite limits
- Files: Sprite management scattered across multiple systems
- Impact: Could exceed hardware sprite limits causing visual artifacts
- Recommendations: Implement sprite pooling and priority-based rendering

**ROM Size Concerns:**
- Issue: Large graphics assets and audio may exceed GBA ROM limits
- Files: Graphics directories contain many asset files
- Impact: May need compression or asset reduction
- Scaling path: Implement asset streaming or compression system

---

*Concerns audit: 2026-01-24*