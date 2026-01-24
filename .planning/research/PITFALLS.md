# Domain Pitfalls

**Domain:** GBA chunk streaming systems
**Researched:** January 24, 2026
**Confidence:** MEDIUM

## Critical Pitfalls

Mistakes that cause rewrites or major issues.

### Pitfall 1: Buffer Recentering Synchronization Issues

**What goes wrong:**
When the circular buffer origin shifts to follow the player, all visible chunks can update simultaneously during a single frame, causing a visible "wave" of chunk updates sweeping across the screen. This happens because the entire buffer remapping occurs atomically from the hardware's perspective, but not from the player's visual perspective.

**Why it happens:**
Developers treat buffer recentering as a single atomic operation without considering the GBA's scanline-by-scanline rendering. The GBA PPU draws the screen progressively (154 scanlines, 144 active), so simultaneous chunk updates create visual discontinuities that appear as waves or tears moving across the screen during the recentering operation.

**Consequences:**
- Visible wave artifacts during player movement in any direction
- Loss of 60 FPS performance during buffer shifts
- Player disorientation when chunks visibly "pop" into new positions
- Inability to maintain smooth 60 FPS gameplay during buffer recentering

**Prevention:**
Implement predictive buffer shifting with staggered chunk updates:
1. Predict when buffer recentering will be needed (player approaching buffer edge)
2. Begin loading new chunks into buffer slots before they're visible
3. Update chunk mappings incrementally over multiple frames
4. Use double-buffering for chunk mappings separate from tile data

**Detection:**
- Visual wave artifacts when player moves more than 8 chunks from buffer origin
- Frame drops during buffer recentering operations
- All chunks updating simultaneously in debug visualization

**Phase to address:** Predictive Buffer Management Implementation

---

### Pitfall 2: VRAM Bandwidth Saturation During Chunk Loading

**What goes wrong:**
Loading too many tiles per frame exceeds VRAM bandwidth limits, causing frame pacing issues and potential DMA conflicts. The GBA has only 64 tiles/frame sustainable bandwidth before impacting frame timing, and loading all new chunks at once can saturate the memory bus.

**Why it happens:**
GBA VRAM transfers are limited by both DMA bandwidth and CPU cycle constraints. When buffer recentering requires loading multiple chunks, developers often try to load all required tiles immediately without considering the 64 tiles/frame budget, leading to memory bus contention and frame time overruns.

**Consequences:**
- Frame drops from 60 FPS to 30 FPS during chunk loading
- DMA conflicts causing visual corruption
- Audio stuttering due to CPU cycle theft from audio processing
- Input lag during heavy chunk loading periods

**Prevention:**
Implement bandwidth-aware chunk loading:
1. Never exceed 64 tiles/frame transfer budget (TILES_PER_FRAME)
2. Prioritize visible chunks over edge chunks for loading
3. Spread chunk loading across multiple frames when needed
4. Use DMA channel 3 for bulk transfers to avoid CPU overhead
5. Implement frame-time monitoring to detect bandwidth saturation

**Detection:**
- Profiler showing >100% frame time during chunk operations
- Frame drops coinciding with chunk loading
- Audio glitching during buffer shifts

**Phase to address:** Bandwidth Management Implementation

---

### Pitfall 3: Tile Mapping Race Conditions

**What goes wrong:**
Updating tile-to-VRAM mappings while the PPU is actively reading from those same memory locations causes corrupted tile data to be displayed, resulting in garbage visuals or incorrect chunk rendering.

**Why it happens:**
The GBA renders scanlines sequentially while allowing CPU access to VRAM during HBlank periods. If tile mapping updates occur mid-frame (during VDraw), the PPU may read partially updated tile data, displaying malformed chunks until the next frame.

**Consequences:**
- Corrupted chunk visuals appearing as random noise
- Tiles displaying incorrect data until next frame
- Inconsistent chunk rendering between frames
- Potential crashes if corrupted tile data causes illegal memory access

**Prevention:**
Implement safe tile update timing:
1. Only update tile mappings during VBlank periods
2. Use double-buffering for tile data arrays
3. Validate tile integrity before making visible
4. Implement atomic tile swap operations
5. Use DMA for tile data transfers to minimize timing windows

**Detection:**
- Visual corruption during chunk updates
- Inconsistent rendering between frames
- Debug builds showing partial tile updates

**Phase to address:** Safe Tile Mapping Implementation

---

### Pitfall 4: Memory Fragmentation in Chunk Pool

**What goes wrong:**
Frequent chunk allocation/deallocation without proper pooling causes memory fragmentation, eventually failing to allocate new chunks even when total memory is sufficient, leading to streaming failures.

**Why it happens:**
GBA WRAM is limited (256KB total, 32KB on-chip) and lacks automatic memory management. Without careful pool management, variable-size chunk allocations fragment memory over time, creating many small unusable gaps between allocated chunks.

**Consequences:**
- Chunk loading failures despite available total memory
- Gradual performance degradation as fragmentation increases
- Inability to load new chunks in critical path
- Game crashes when memory allocation fails unexpectedly

**Prevention:**
Implement fixed-size chunk pooling:
1. Use fixed-size chunk allocations (power-of-2 sizes preferred)
2. Maintain free chunk pools instead of general allocation
3. Implement chunk recycling with reference counting
4. Pre-allocate maximum expected chunks at startup
5. Use memory compaction during natural breaks (level transitions)

**Detection:**
- Allocation failures despite memory usage showing available space
- Increasing allocation times over time
- Memory usage patterns showing many small free gaps

**Phase to address:** Chunk Pool Architecture

---

## Moderate Pitfalls

Mistakes that cause delays or technical debt.

### Pitfall 1: Inefficient Chunk Coordinate Calculations

**What goes wrong:**
Using floating-point math or inefficient integer arithmetic for chunk coordinate calculations causes performance degradation and potential precision issues in chunk positioning.

**Why it happens:**
GBA lacks hardware floating-point support, making floating-point operations extremely expensive (software emulation). Developers often use coordinate systems that require frequent division or complex trigonometric calculations for chunk positioning.

**Consequences:**
- Reduced performance due to expensive math operations
- Chunk positioning drift over time
- Inconsistent chunk alignment causing visual seams
- Poor cache utilization due to complex calculation patterns

**Prevention:**
Implement fixed-point arithmetic for chunk calculations:
1. Use bn::fixed or custom fixed-point for all position calculations
2. Pre-calculate common coordinate transformations
3. Use bit shifts for power-of-2 operations
4. Implement lookup tables for complex trigonometric functions
5. Cache chunk coordinate calculations where possible

**Detection:**
- Profiler showing high CPU usage in coordinate calculation functions
- Chunk jitter or drift during movement
- Poor performance during smooth camera movement

**Phase to address:** Fixed-Point Coordinate System

---

### Pitfall 2: Suboptimal Chunk Loading Order

**What goes wrong:**
Loading chunks in inefficient order (e.g., farthest chunks first) causes player to see empty or loading chunks before needed ones are visible, breaking immersion.

**Why it happens:**
Developers implement chunk loading based on data structure order rather than player-centric priority, or load chunks in sweeping patterns that don't match actual player movement patterns.

**Consequences:**
- Player seeing unloaded/chunk-loading areas before content appears
- Unnecessary loading of chunks that won't be visible
- Wasted bandwidth on non-critical chunks
- Poor user experience during rapid movement

**Prevention:**
Implement player-centric chunk loading priority:
1. Load chunks based on distance to player position
2. Prioritize chunks in current movement direction
3. Implement predictive loading for likely destinations
4. Defer non-essential chunks until bandwidth available
5. Use circular buffer knowledge to anticipate movement

**Detection:**
- Player seeing empty chunks during movement
- Loading far chunks before near ones
- Bandwidth waste in chunk loading statistics

**Phase to address:** Priority-Based Chunk Loading

---

### Pitfall 3: Ignoring GBA Hardware Limits

**What goes wrong:**
Exceeding GBA hardware limits (4 background layers, 128 sprites, 1024 tiles per layer) causes unexpected behavior or crashes when assumptions about available resources are violated.

**Why it happens:**
 Developers design chunk systems without accounting for GBA's strict hardware limits, assuming unlimited resources or designing for more powerful platforms.

**Consequences:**
- Sprite/background display failures when limits exceeded
- Visual corruption when hardware limits are hit
- Game crashes during intensive scenes
- Inconsistent behavior across different GBA models

**Prevention:**
Design within GBA constraints from start:
1. Track resource usage against hardware limits
2. Implement fallbacks when limits are exceeded
3. Design chunk system for 4 BG layers maximum
4. Limit active sprites to 128 per frame
5. Use resource budgets in design phase

**Detection:**
- Visual issues when scene complexity increases
- Resource allocation failures
- Inconsistent behavior between test and production

**Phase to address:** Hardware Constraint Planning

---

## Minor Pitfalls

Mistakes that cause annoyance but are fixable.

### Pitfall 1: Chunk Boundary Visual Seams

**What goes wrong:**
Visible seams appear at chunk boundaries due to alignment issues or coordinate precision errors, breaking the illusion of continuous terrain.

**Why it happens:**
Chunk edge coordinates don't align perfectly due to floating-point precision errors or incorrect tile-to-chunk calculations, leaving 1-pixel gaps or overlaps between adjacent chunks.

**Consequences:**
- Visible lines or gaps at chunk boundaries
- Terrain discontinuities during movement
- Minor visual artifacts breaking immersion
- Collision detection issues at chunk seams

**Prevention:**
Ensure perfect chunk alignment:
1. Use integer arithmetic for all boundary calculations
2. Implement chunk edge smoothing/overlap
3. Validate chunk continuity in debug builds
4. Use conservative chunk boundaries
5. Test chunk alignment under all movement conditions

**Detection:**
- Visible lines at chunk boundaries
- Terrain height differences at chunk edges
- Visual artifacts during smooth camera movement

**Phase to address:** Chunk Boundary Alignment

---

### Pitfall 2: Inefficient Chunk Caching

**What goes wrong:**
Poor caching strategy causes redundant tile loading and excessive memory usage, reducing overall system performance.

**Why it happens:**
Implementing simple LRU or no caching strategy for chunks that have predictable access patterns, leading to repeated loading/unloading of frequently accessed chunks.

**Consequences:**
- Increased memory bandwidth usage
- Reduced performance due to redundant loads
- Larger memory footprint than necessary
- Battery life impact on real hardware

**Prevention:**
Implement intelligent chunk caching:
1. Use access pattern prediction for hot chunks
2. Implement multi-level caching (hot/warm/cold)
3. Pin frequently accessed chunks in memory
4. Use compression for cached chunk data
5. Implement cache warming for predictable areas

**Detection:**
- High cache miss rates in chunk access
- Redundant tile loading in profiles
- Memory usage higher than expected

**Phase to address:** Cache Strategy Optimization

---

## "Looks Done But Isn't" Checklist

Things that appear complete but are missing critical pieces.

- [ ] **Predictive Buffer Logic:** Often missing edge case handling — verify all player movement patterns covered
- [ ] **Bandwidth Budgeting:** Often missing frame-time monitoring — verify TILES_PER_FRAME never exceeded
- [ ] **Safe Tile Updates:** Often missing VBlank synchronization — verify all tile updates respect scanline timing
- [ ] **Memory Pool Design:** Often missing fragmentation prevention — verify fixed-size allocation strategy
- [ ] **Hardware Limit Tracking:** Often missing runtime monitoring — verify resource usage stays within GBA limits
- [ ] **Chunk Loading Priority:** Often missing player-centric ordering — verify distance-based loading works
- [ ] **Debug Visualization:** Often missing runtime chunk state visualization — verify debugging tools implemented

---

## Recovery Strategies

When pitfalls occur despite prevention, how to recover.

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Buffer Synchronization | HIGH | 1. Pause game updates<br>2. Reset buffer state<br>3. Implement staggered loading<br>4. Test with reduced bandwidth |
| VRAM Bandwidth | MEDIUM | 1. Reduce TILES_PER_FRAME<br>2. Implement frame pacing<br>3. Add bandwidth monitoring<br>4. Profile hot paths |
| Tile Mapping Race | HIGH | 1. Disable tile updates<br>2. Clear VRAM safely<br>3. Rebuild tile mappings<br>4. Add double-buffering |
| Memory Fragmentation | HIGH | 1. Defragment during natural break<br>2. Restart with fresh pools<br>3. Implement better pooling<br>4. Add fragmentation monitoring |
| Coordinate Precision | LOW | 1. Switch to fixed-point math<br>2. Recalculate all positions<br>3. Update coordinate system<br>4. Validate alignment |
| Loading Priority | MEDIUM | 1. Implement distance-based loading<br>2. Add predictive loading<br>3. Profile access patterns<br>4. Optimize for player movement |
| Hardware Limits | MEDIUM | 1. Add resource tracking<br>2. Implement graceful fallbacks<br>3. Redesign within constraints<br>4. Add limit monitoring |

---

## Pitfall-to-Phase Mapping

How roadmap phases should address these pitfalls.

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| Buffer Synchronization | Predictive Buffer Management | Implement staggered chunk updates and verify no wave artifacts |
| VRAM Bandwidth | Bandwidth Management Implementation | Monitor frame times and verify 60 FPS maintained |
| Tile Mapping Race | Safe Tile Mapping Implementation | Verify all updates occur during VBlank only |
| Memory Fragmentation | Chunk Pool Architecture | Implement fixed-size pools and verify allocation success |
| Coordinate Precision | Fixed-Point Coordinate System | Use bn::fixed and verify no drift over time |
| Loading Priority | Priority-Based Chunk Loading | Implement distance-based loading and verify player sees chunks before loading indicators |
| Hardware Limits | Hardware Constraint Planning | Track resource usage and verify within GBA limits |
| Chunk Boundary Alignment | Chunk Boundary Management | Verify seamless transitions between chunks |
| Cache Strategy | Cache System Optimization | Monitor cache hit rates and verify bandwidth efficiency |

---

## Sources

- [Game Boy Advance Resource Management - Rafael Baptista, Game Developer](https://www.gamedeveloper.com/programming/gameboy-advance-resource-management) (HIGH confidence - authoritative GBA memory management guide)
- [GBA Graphics Programming - Ian Finlayson](https://ianfinlayson.net/gba/02-graphics.html) (HIGH confidence - covers VBlank timing and double buffering)
- [GBATEK Technical Documentation](https://mgba-emu.github.io/gbatek/) (HIGH confidence - official GBA hardware specifications)
- [Pan Docs - Rendering Overview](https://gbdev.io/pandocs/Rendering.html) (HIGH confidence - GBA PPU rendering details)
- [Half Tile Offset Streaming World Grids - Demofox](https://blog.demofox.org/2017/09/30/half-tile-offset-streaming-world-grids/) (MEDIUM confidence - specific to streaming optimizations)
- [Tonc GBA Documentation](https://www.coranac.com/tonc/text/video.htm) (HIGH confidence - comprehensive GBA programming guide)
- [Butano Framework Documentation](https://github.com/GValiente/butano) (HIGH confidence - current GBA C++ framework patterns)

---
*Pitfalls research for: GBA chunk streaming systems*
*Researched: January 24, 2026*