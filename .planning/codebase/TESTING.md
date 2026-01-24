# Testing Patterns

**Analysis Date:** 2026-01-24

## Test Framework

**Runner:**
- No formal unit testing framework detected
- Manual testing through game builds
- Butano's built-in test framework available but not used by project

**Assertion Library:**
- `bn::assert` for runtime assertions
- Compile-time `static_assert` for constraints

**Run Commands:**
```bash
make                    # Build game ROM
make clean              # Clean build artifacts
make run                # Build and run in default emulator
```

## Test File Organization

**Location:**
- No separate test directory structure
- Tests integrated into development workflow
- Templates located in `templates/` directory

**Naming:**
- No formal test naming conventions
- Test sprites use `temptest` prefix

**Structure:**
```
testing approach:
├── Manual build-and-test workflow
├── Emulator testing (mGBA)
├── Real hardware testing
└── Debug features in-game
```

## Test Structure

**Manual Testing Process:**
1. Build after every code edit
2. Test in mGBA emulator
3. Verify functionality works as expected
4. Check performance (60 FPS target)
5. Test on real hardware when possible

**Development Workflow Tests:**
```cpp
// From STR_DEVELOPMENT_GUIDE.md:
// 1. Build and test after every edit
// 2. Use mGBA emulator for testing
// 3. Check for stack traces in debug
// 4. Test on real hardware when possible
// 5. Profile performance regularly
```

## Game Testing Strategies

**Functional Testing:**
- Manual playthrough of game features
- Input validation testing
- State machine transition testing
- Collision detection verification
- Audio/visual consistency checks

**Platform-Specific Testing:**
- **Emulator Testing**: Primary development testing on mGBA
- **Hardware Testing**: Periodic testing on actual GBA hardware
- **Timing Verification**: Ensure 60 FPS performance target

**Debug Features for Testing:**
- Hitbox visualization: SELECT + START to toggle
- Camera zoom: Hold SELECT for zoom out
- Debug controls: Various button combinations for testing
- Stack trace logging: Enabled via `STACKTRACE=true` in Makefile

## Performance Testing

**Frame Rate Monitoring:**
- Target: 60 FPS (16.67ms per frame)
- Monitor using emulator debug tools
- Check for frame drops during complex scenes

**Memory Usage Testing:**
- Monitor sprite count (max 128 sprites)
- Check memory allocation in debug builds
- Validate `bn::vector` capacity limits
- Profile RAM usage patterns

**Resource Limits Testing:**
- Sprite limit verification
- Audio channel testing
- Tile memory usage checks
- Palette limitations validation

## Build Verification

**Compilation Testing:**
- Clean build verification (`make clean && make`)
- Cross-platform compatibility checks
- Resource compilation verification
- Asset processing validation

**ROM Generation:**
- `.gba` ROM file generation
- `.elf` debug file generation
- Asset embedding verification
- ROM header validation

## Emulator Testing

**Primary Emulator: mGBA**
- Most accurate GBA emulation
- Good debugging features
- Real-time performance monitoring
- Memory usage tracking

**Alternative Emulators:**
- No$GBA: For debugger backend testing
- VBA-M: Additional compatibility testing

**Emulator Features:**
- Save state testing
- Cheat code verification
- Video recording for bug reports
- Input recording for testing sequences

## Debugging Tools and Techniques

**In-Game Debug Features:**
```cpp
// Debug features documented in STR_DEVELOPMENT_GUIDE.md:
// - Hitbox visualization: SELECT + START to toggle
// - Camera zoom: Hold SELECT for zoom out
// - Debug controls: Various button combinations for testing
```

**Logging:**
- `bn::log` for emulator console output
- Stack trace logging enabled in debug builds
- Performance profiling through emulator tools

**Memory Debugging:**
- Stack trace analysis on crashes
- Memory usage monitoring
- Buffer overflow detection
- Resource leak tracking

## Quality Assurance Practices

**Code Review Checklist:**
- Pattern header compliance verification
- Memory management validation
- Performance impact assessment
- GBA constraint adherence

**Pre-Commit Testing:**
- Build must succeed
- No new compiler warnings
- Performance impact measured
- Memory usage within limits

**Release Testing:**
- Full gameplay testing
- Multi-emulator verification
- Real hardware validation
- Save/load functionality testing

## Automated Testing Opportunities

**Missing Automation:**
- No unit test framework integration
- No automated build pipeline
- No continuous integration setup
- No automated regression testing

**Potential Improvements:**
- Unit tests for utility functions
- Automated build verification
- Performance regression testing
- Memory usage automation

## Test Data Management

**Test Assets:**
- `temptest` sprites for UI testing
- Debug sprites and graphics
- Test audio files
- Sample save states

**Configuration Testing:**
- Debug vs release builds
- Different compiler optimization levels
- Various memory configurations

## GBA-Specific Testing Considerations

**Hardware Constraints Testing:**
- Screen boundary testing (240x160 pixels)
- Sprite overflow testing (128 sprite limit)
- Audio channel exhaustion
- Memory fragmentation testing

**Timing Verification:**
- VBlank synchronization testing
- Frame rate consistency
- Input latency measurement
- Animation timing validation

**Compatibility Testing:**
- Different GBA models
- Various flash carts
- Emulator accuracy verification
- Save type compatibility

## Regression Testing

**Areas Requiring Regular Testing:**
- Player movement and collision
- Enemy AI behavior
- Bullet physics and interaction
- HUD functionality
- Save/load system
- Audio synchronization

**Test Scenarios:**
- New game start flow
- Combat system stress testing
- Memory pressure scenarios
- Long play sessions (memory leaks)
- Edge case input combinations

## Performance Monitoring

**Key Metrics:**
- Frame rate consistency
- Memory usage patterns
- Sprite count optimization
- CPU usage profiling

**Tools:**
- mGBA performance overlay
- Butano profiler integration
- Custom performance counters
- Memory usage tracking

---

*Testing analysis: 2026-01-24*