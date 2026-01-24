# Architecture Research: GBA Chunk Streaming Systems

**Domain:** Game Boy Advance chunk streaming for large worlds
**Researched:** 2026-01-24
**Confidence:** HIGH

## Executive Summary

GBA chunk streaming systems follow well-established architectural patterns centered around the constraints of the GBA hardware. Research confirms that successful implementations use circular buffer management, DMA-driven VRAM updates, predictive loading strategies, and careful memory management to maintain 60fps performance. The current Stranded implementation follows these patterns but lacks predictive buffer management, causing visual artifacts during buffer recentering.

## Key Findings

**Stack:** GBA development with Butano C++ framework, 16x16 chunk circular buffers with DMA VRAM updates
**Architecture:** Component-separated streaming with background, chunk, and predictive management layers  
**Critical pitfall:** Buffer recentering causes "wave" artifacts due to simultaneous chunk remapping

## Implications for Roadmap

Based on research, suggested phase structure:

1. **Predictive Buffer Manager** - Addresses core visual artifact issue
   - Implements lookahead logic to shift buffer before edge conditions
   - Should integrate with existing ChunkManager coordinate conversion system

2. **Enhanced Streaming Pipeline** - Improves loading performance
   - Extends current frame-based budgeting with better predictive algorithms
   - Should leverage existing DMA transfer patterns in Butano

3. **Architecture Validation** - Ensures component boundaries are clear
   - Verifies that predictive system doesn't break existing data flows
   - Tests integration with Butano's affine background system

**Phase ordering rationale:**
- Predictive system first (fixes user-visible artifacts)
- Enhanced pipeline second (builds on working foundation)
- Architecture validation third (ensures clean integration)

**Research flags for phases:**
- Phase 1: Standard patterns, but requires careful integration with existing ChunkManager
- Phase 2: Medium confidence - predictive algorithms need performance testing
- Phase 3: Low risk - builds on existing Butano abstractions

## Recommended Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                    Game Loop & Scene Management                      │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────────┐  ┌─────────────────┐  │
│  │ Input Manager │  │ Player Manager  │  │ Camera Manager  │  │ Predictive        │
│  │               │  │                 │  │                  │  │
│  └─────────────┘  └─────────────────┘  └─────────────────┘  │
├────────────────────────────────────────────────────────────────┤
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      Core Rendering System (Butano)                      │  │
│  │  ┌─────────────┐  ┌──────────────┐  ┌───────────────────┐  │
│  │  │ Affine BG     │  │ Regular BG      │  │  │
│  │  │ Management      │  │ Management       │  │  │
│  │  └─────────────┘  └──────────────┘  └───────────────────┘  │
│  │  │ Sprite Manager │  │ OAM Manager     │  │  │
│  │  └─────────────┘  └──────────────┘  └───────────────────┘  │
│  └───────────────────────────────────────────────────────────────────┘  │
│  │                  Audio & Music System                             │
│  └───────────────────────────────────────────────────────────┘  │
│                                                             │
├────────────────────────────────────────────────────────────────────┤
│  ┌─────────────────────────────────────────────────────────────┐  │
│  │                      Memory Management (Butano)                       │
│  │  ┌─────────────────────────────────────────────────────┐  │
│  │  │ OBJ VRAM Mgr  │  │ WRAM Mgr       │  │  │
│  │  │ (32KB total)   │  │ (256KB total)    │  │  │
│  │  └─────────────┘  └──────────────┘  └───────────────────┘  │
└───────────────────────────────────────────────────────────────────────────┘

Data Flow: Player → Camera → Predictive → ChunkManager → Background → VRAM → Screen
```

### Component Boundaries

| Component | Responsibility | Communicates With |
|-----------|-------------|-------------------|
| **Game Loop** | Orchestrates main game loop, timing, and scene transitions | Camera Manager, Player Manager, Scene Manager |
| **Input Manager** | Handles keyboard input and converts to game actions | Player Manager, Game Loop |
| **Player Manager** | Manages player state, position, and game logic | Camera Manager, ChunkManager, Collision System |
| **Camera Manager** | Follows player with deadzone and lookahead, provides world coordinates | Background System, Player Manager |
| **Predictive Manager** | **NEW:** Analyzes player movement to predict buffer shifts and preload chunks | ChunkManager, Background System |
| **ChunkManager** | Manages 16x16 chunk circular buffer and streaming | Predictive Manager, World Data, Background System |
| **Background System** | Manages affine background rendering and VRAM updates | ChunkManager, Core Rendering |
| **World Data** | Provides ROM tile data and collision information | ChunkManager, Collision System |
| **Memory Management** | Handles OBJ VRAM allocation, WRAM allocation, and DMA transfers | All components (via Butano) |

### Data Flow

```
Player Movement → Predictive Analysis → Buffer Shift Decision → Chunk Loading → VRAM Update → Screen Render
     │                    │                    │                    │                    │                    │
     │                    │                    │                    │
     └─► DMA transfers (64 tiles/frame budget) ──► VRAM commit (bg_map.reload_cells_ref()) ──► Next frame
```

**Key flows:**
1. **Chunk Loading:** World ROM → ChunkManager buffer → Butano BG system → VRAM
2. **Coordinate Conversion:** World coordinates ↔ Buffer coordinates ↔ Screen coordinates (existing system works well)
3. **Predictive Logic:** Player velocity + position → Buffer edge detection → Proactive loading

## Architectural Patterns

### Pattern 1: Circular Buffer Management

**What:** 16x16 chunk circular buffer with wrapping arithmetic
**When:** Essential for any large GBA world exceeding 128x128 tile map limit
**Example:**
```cpp
// Current Stranded implementation
int buffer_slot_x = chunk_to_buffer_slot(chunk_x);
int buffer_tile_x = tile_to_buffer_coord(world_tile_x);

// Wrapping handles overflow seamlessly
buffer[buffer_tile_y * VIEW_BUFFER_TILES + buffer_tile_x] = tile_data;
```

**Why:** GBA affine backgrounds are limited to 128x128 tiles, but worlds can be much larger. Circular buffers with modulo arithmetic allow infinite scrolling within hardware limits.

### Pattern 2: Frame-Based Streaming Budget

**What:** Distribute tile loading across multiple frames to maintain 60fps
**When:** Critical for performance on GBA's limited CPU bandwidth
**Example:**
```cpp
// Current implementation
constexpr int TILES_PER_FRAME = 64;
constexpr int MAX_CHUNKS_PER_FRAME = 8;

if (tiles_this_frame < TILES_PER_FRAME) {
    stream_pending_chunk();
}
```

**Why:** GBA CPU runs at 16.78MHz with VRAM bandwidth constraints. Spreading loading prevents frame drops and maintains smooth gameplay.

### Pattern 3: DMA-Optimized VRAM Updates

**What:** Use DMA transfers and batch VRAM updates
**When:** Essential for GBA performance and avoiding tearing
**Example:**
```cpp
// Current Butano approach
void commit_to_vram(bn::affine_bg_map_ptr& bg_map) {
    if (_needs_vram_update) {
        bg_map.reload_cells_ref();  // DMA transfer
        _needs_vram_update = false;
    }
}
```

**Why:** Direct VRAM writes during active display cause visual artifacts. DMA transfers during V-blank ensure atomic updates.

### Pattern 4: Predictive Buffer Management

**What:** **NEW COMPONENT** - Analyze player movement to predict when buffer shifts will be needed
**When:** Essential for eliminating recentering artifacts
**Example:**
```cpp
// Proposed architecture
class PredictiveBufferManager {
private:
    bn::fixed_point _player_velocity;
    int _player_predicted_x;
    int _buffer_origin_chunk_x;
    
    void update_player_prediction(const bn::fixed_point& player_pos, const bn::fixed_point& player_vel) {
        // Simple prediction: player_x + velocity * prediction_frames
        _player_predicted_x = player_pos.x().integer() + player_vel.x().integer() * PREDICTION_FRAMES;
    }
    
    bool should_shift_buffer(const bn::fixed_point& player_pos) {
        int current_chunk_x = player_pos.x().integer() / CHUNK_SIZE_TILES;
        int distance_to_edge = _buffer_origin_chunk_x - current_chunk_x;
        
        // Shift when approaching edge with margin
        return distance_to_edge <= PREDICTIVE_SHIFT_THRESHOLD;
    }
};
```

**Why:** Current Stranded system shifts buffer reactively when player reaches edges, causing all chunks to remap simultaneously and creating visible "wave" artifacts. Predictive shifting loads chunks before they're needed, eliminating visual disruption.

## Anti-Patterns to Avoid

### Anti-Pattern 1: Synchronous Buffer Updates

**What people do:** Update entire buffer at once when recentering
**Why it's bad:** All 16x16 chunks remap simultaneously, causing visible "wave" artifacts across entire screen
**Instead:** Use incremental updates or stagger chunk transitions

### Anti-Pattern 2: Static Buffer Origin

**What people do:** Keep buffer origin fixed and load new chunks beyond current buffer
**Why it's bad:** Complex coordinate management and poor use of circular buffer benefits
**Instead:** Leverage circular buffer wrapping for seamless infinite scrolling

### Anti-Pattern 3: Direct Memory Access

**What people do:** Access VRAM directly during active rendering
**Why it's bad:** Causes tearing and visual artifacts due to GBA's rasterization timing
**Instead:** Use DMA transfers during V-blank or double buffering

## Scalability Considerations

| Scale | Streaming Approach | Memory Usage | Performance Impact |
|-------|-------------------|-------------|-------------------|
| **Current (9x9 chunks)** | 56KB loaded chunks | Smooth 60fps | Minimal CPU usage |
| **Large world (64x64 chunks)** | 224KB loaded chunks | Requires more predictive management | Higher CPU usage but manageable |
| **Full 8192x8192 world** | Full 1GB streaming | Requires sophisticated caching | Approaches GBA hardware limits |

**Scaling priorities:**
1. **First:** Optimize predictive algorithms within current 9x9 load range
2. **Second:** Expand to 16x16 buffer with better predictive management
3. **Third:** Implement multi-tier caching for very large worlds

## Integration Points for Predictive System

### 1. ChunkManager Integration

```cpp
class ChunkManager {
private:
    PredictiveBufferManager _predictive_manager;
    
    void update(const bn::fixed_point& player_world_pos) override {
        // Update predictive logic first
        _predictive_manager.update_player_prediction(player_world_pos);
        
        // Check if buffer should shift proactively
        if (_predictive_manager.should_shift_buffer()) {
            _recenter_buffer_proactively();
        }
        
        // Continue with existing chunk loading logic
        return _determine_needed_chunks_and_load(player_world_pos);
    }
    
    void _recenter_buffer_proactively() {
        // Shift buffer origin incrementally
        int shift_chunks = _predictive_manager.get_recommended_shift();
        _buffer_origin_tile_x += shift_chunks * CHUNK_SIZE_TILES;
        _buffer_origin_tile_y += shift_chunks * CHUNK_SIZE_TILES;
        
        // Update loaded chunks to new buffer coordinates
        _update_chunk_coordinates_for_buffer_shift();
    }
};
```

### 2. Coordinate System Extension

```cpp
// Extend existing coordinate conversion to handle predictive shifts
bn::fixed_point ChunkManager::world_to_buffer(const bn::fixed_point& world_pos) const override {
    // Account for predictive buffer origin
    bn::fixed adjusted_world_x = world_pos.x() - bn::fixed(_buffer_origin_tile_x * TILE_SIZE);
    bn::fixed adjusted_world_y = world_pos.y() - bn::fixed(_buffer_origin_tile_y * TILE_SIZE);
    
    // Use existing wrapping logic
    return _existing_world_to_buffer_logic(adjusted_world_pos);
}
```

### 3. Performance Considerations

- **Predictive calculations should be simple:** Player velocity × small lookahead multiplier
- **Shift during V-blank:** Buffer recentering should happen when display is not updating
- **Maintain frame budget:** Predictive shifts count as chunk loading, not additional work
- **Preserve DMA efficiency:** Batch coordinate updates with VRAM commits

## Sources

- [HIGH] Butano Framework Documentation - Core rendering, memory management, and background systems
- [HIGH] GBATEK Technical Reference - GBA hardware specifications, memory layout, and DMA capabilities  
- [HIGH] Rafael Baptista's Resource Management - Memory allocation patterns, tile management, and streaming optimization
- [MEDIUM] Current Stranded Implementation - Existing ChunkManager architecture and identified recentering artifacts
- [LOW] GBA Development Community - Common predictive buffer patterns (need validation for Stranded's specific use case)

---

*Architecture research for GBA chunk streaming systems with focus on predictive buffer management integration*