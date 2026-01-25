# Stranded Chunk Streaming Architecture Project

**Project Type:** Technical Architecture Improvement  
**Platform:** Game Boy Advance (GBA)  
**Engine:** Butano C++ Framework  
**Focus Area:** World Rendering & Chunk Loading System  

## What This Is

A targeted architectural improvement to eliminate buffer recentering visual artifacts in Stranded's chunk streaming system. The project focuses on implementing predictive buffer shifting to prevent the "wave effect" that occurs when chunks transition across buffer boundaries during player movement.

## The Problem

**Current Issue:** Buffer recentering wave effect  
- When player walks far enough in any direction, background renders in a visible wave pattern  
- Wave moves across screen (left→right when walking right, etc.)  
- Caused by `_recenter_buffer_if_needed()` function remapping all chunk positions simultaneously  
- Currently commented out to avoid visual artifacts, but limits effective buffer usage  

**Technical Root Cause:**
```cpp
// Current approach causes all chunks to remap at once:
void ChunkManager::_recenter_buffer_if_needed(int center_chunk_x, int center_chunk_y, int load_range)
{
    // When buffer origin shifts, ALL loaded chunks remap to new buffer positions
    // This causes visible "wave" of tile updates across the screen
}
```

**Hardware Constraints:**
- GBA has limited VRAM (96KB VRAM total)
- No VSync interrupt for smooth transitions  
- Affine background maps limited to 128x128 tiles
- Memory bandwidth constraints during tile streaming

## The Solution

**Predictive Buffer Shifting:** 
- Shift buffer BEFORE chunks reach edge positions (not after)
- Make transitions invisible by overlapping load/unload zones
- Use gradual tile streaming instead of bulk remapping
- Maintain 60 FPS during buffer recentering

**Key Architectural Changes:**
1. **Predictive Logic:** Detect when buffer shift will be needed (frames in advance)
2. **Overlap Zones:** Create smooth transition areas where old and new positions coexist
3. **Incremental Shifting:** Stream tiles gradually rather than remapping all at once
4. **Seamless Integration:** Work within existing ChunkManager architecture

## Success Criteria

### Must-Have (v1)
- **Zero visible wave artifacts** during buffer recentering at any player speed
- **Maintain 60 FPS** during predictive shifting operations  
- **Preserve existing functionality** - no regressions in chunk loading or collision
- **Memory efficient** - no additional VRAM allocation beyond current usage

### Nice-to-Have (v2)
- **Adaptive streaming budget** based on player movement speed
- **Visual debugging tools** to show buffer boundaries and shift zones
- **Performance monitoring** for chunk streaming metrics

### Out of Scope
- Complete rewrite of chunk system (we're improving, not replacing)
- Changes to world file format or data structures
- Modifications to entity systems or game logic

## Context

### Existing Architecture
- **Chunk Manager:** `include/str_chunk_manager.h`, `src/core/chunk_manager.cpp`
- **Buffer System:** 16x16 chunk circular buffer (128x128 tiles total)
- **Streaming Rate:** 64 tiles per frame (`TILES_PER_FRAME`)
- **Load Range:** 4 chunks in each direction (9x9 visible area)
- **World Size:** 128x128 chunks (8192x8192 pixels, 1024x1024 tiles)

### Current Constants
```cpp
constexpr int VIEW_BUFFER_CHUNKS = 16;           // 16x16 chunks in buffer
constexpr int TILES_PER_FRAME = 64;              // Streaming budget
constexpr int CHUNK_LOAD_DISTANCE = 2;            // Preload distance
// Buffer recentering currently DISABLED (line 169-170 in chunk_manager.cpp)
```

### Technical Constraints
- **GBA VRAM:** 96KB total (64KB for backgrounds, 32KB for sprites)
- **Butano Engine:** Affine background maps max 128x128 tiles  
- **Performance Target:** Consistent 60 FPS during all operations
- **Memory Model:** Fixed-point arithmetic, no dynamic allocation

## Key Decisions

| Decision | Rationale | Outcome |
|-----------|------------|---------|
| Predictive over post-facto shifting | Prevents visual artifacts by acting before they occur | — Pending |
| Incremental over bulk remapping | Maintains 60 FPS by spreading work across frames | — Pending |
| Integration over rewrite | Preserves existing stability and reduces risk | — Pending |
| Hardware-aware optimization | Works within GBA constraints rather than fighting them | — Pending |

## Existing Validated Capabilities

From codebase analysis:

- ✓ **Chunk streaming system** — 16x16 circular buffer with 64-tile/frame streaming
- ✓ **World collision detection** — Tile-based collision with world coordinate conversion  
- ✓ **Entity positioning system** — World-to-buffer coordinate transformations
- ✓ **Memory management** — Efficient chunk tracking with state machine
- ✓ **Background rendering** — Butano affine background integration
- ✓ **Performance budgeting** — Frame-based streaming limits to maintain 60 FPS

## Active Requirements

- **STREAM-01:** Implement predictive buffer shift detection before edge conditions
- **STREAM-02:** Create overlapping transition zones for seamless chunk remapping  
- **STREAM-03:** Develop incremental tile streaming during buffer recentering
- **STREAM-04:** Maintain 60 FPS performance during all shift operations
- **STREAM-05:** Preserve existing chunk loading and collision functionality
- **STREAM-06:** Add visual debugging tools for buffer boundaries (optional)

## Out of Scope

- **Complete chunk system rewrite** — Current architecture is solid, just needs recentering fix
- **World format changes** — Keep existing `WorldMapData` structure unchanged  
- **Entity system modifications** — Focus purely on rendering artifacts
- **Memory allocation changes** — Work within existing VRAM constraints
- **Multi-threading** — GBA is single-threaded, keep simple approach

---

*Last updated: 2026-01-24 after initialization*