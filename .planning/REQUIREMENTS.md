# Stranded Chunk Streaming Requirements

**Project:** Stranded GBA Game - Predictive Buffer Management  
**Scope:** Eliminate buffer recentering wave artifacts through architectural improvements  
**Date:** 2026-01-24  

## v1 Requirements

### Foundation Requirements (Table Stakes)

#### Circular Buffer System
- **STREAM-01**: Maintain 16x16 chunk circular buffer (128x128 tiles total) for efficient VRAM usage
- **STREAM-02**: Preserve existing chunk loading/unloading logic based on player distance
- **STREAM-03**: Support chunk coordinate tracking with state management (UNLOADED/LOADING/LOADED)

#### Basic Distance-Based Loading
- **STREAM-04**: Load chunks within 4-chunk radius (9x9 visible area) of player position
- **STREAM-05**: Unload chunks outside 4-chunk radius to maintain memory efficiency
- **STREAM-06**: Track player chunk position for boundary detection

#### DMA Transfer Optimization
- **STREAM-07**: Use DMA channel 0 during VBlank for VRAM updates to maintain 60 FPS
- **STREAM-08**: Limit transfers to 64 tiles per frame within GBA hardware bandwidth
- **STREAM-09**: Batch tile transfers to minimize DMA setup overhead

#### World-to-Buffer Coordinate Conversion
- **STREAM-10**: Convert world coordinates to buffer-relative positions with wrapping arithmetic
- **STREAM-11**: Handle buffer coordinate wrapping for circular buffer behavior
- **STREAM-12**: Support buffer origin tracking for world-to-buffer transformations

#### Background Scrolling Support
- **STREAM-13**: Enable smooth background position updates via BGHOFS/BGVOFS registers
- **STREAM-14**: Integrate with Butano affine background map system
- **STREAM-15**: Maintain background layer compatibility with existing rendering pipeline

### Predictive Requirements (Key Differentiators)

#### Predictive Buffer Shifting
- **STREAM-16**: Detect when buffer recentering will be needed (before edge conditions)
- **STREAM-17**: Implement overlapping transition zones for seamless chunk remapping
- **STREAM-18**: Eliminate visible "wave" artifacts during buffer origin changes
- **STREAM-19**: Maintain 60 FPS performance during predictive shifting operations
- **STREAM-20**: Preserve existing chunk loading and collision functionality during shifts

#### Hot-Swapping Buffer Origins
- **STREAM-21**: Implement incremental buffer origin changes (not bulk recentering)
- **STREAM-22**: Stream tiles gradually during buffer transitions
- **STREAM-23**: Use double-buffering approach for invisible buffer shifts
- **STREAM-24**: Coordinate buffer shifts with VBlank periods for artifact-free updates

#### Adaptive Streaming Rate
- **STREAM-25**: Adjust streaming budget based on current performance needs
- **STREAM-26**: Increase tiles/frame when player moves quickly (up to 128 tiles max)
- **STREAM-27**: Reduce tiles/frame when player is stationary (down to 32 tiles min)
- **STREAM-28**: Monitor frame time to maintain consistent 60 FPS target

### Cross-Functional Requirements

#### Memory Management
- **STREAM-29**: Use existing VRAM allocation patterns (no additional memory beyond current usage)
- **STREAM-30**: Maintain chunk state tracking within existing 128-slot vector limit
- **STREAM-31**: Prevent memory fragmentation during buffer shifts

#### Performance Optimization
- **STREAM-32**: Preserve existing 60 FPS performance baseline
- **STREAM-33**: Optimize for GBA's 16.78MHz CPU constraints
- **STREAM-34**: Work within current 64 tiles/frame streaming budget baseline

#### Integration Compatibility
- **STREAM-35**: Maintain compatibility with existing collision detection system
- **STREAM-36**: Preserve current entity positioning and hitbox calculations
- **STREAM-37**: Integrate with existing camera and world coordinate systems

## v2 Requirements (Deferred)

### Advanced Predictive Features
- **STREAM-V2-01**: Prefetch on Movement Patterns - anticipate player direction changes
- **STREAM-V2-02**: Multi-Layer Coordination - synchronize chunk loading across BG layers
- **STREAM-V2-03**: Compression-Aware Loading - handle compressed chunks to save ROM space

### Enhanced Debugging & Monitoring
- **STREAM-V2-04**: Visual debugging tools for buffer boundaries and shift zones
- **STREAM-V2-05**: Performance monitoring for chunk streaming metrics
- **STREAM-V2-06**: Frame time analysis and streaming budget visualization

### Scaling Features
- **STREAM-V2-07**: Support larger chunk sizes (beyond current 8x8 tiles)
- **STREAM-V2-08**: Dynamic load range adjustment based on player speed
- **STREAM-V2-09**: Memory pool optimization for variable chunk sizes

## Out of Scope

### System Architecture Changes
- **Complete chunk system rewrite** - Current architecture is sound, just needs predictive enhancement
- **World format changes** - Keep existing `WorldMapData` structure unchanged
- **Entity system modifications** - Focus purely on rendering artifacts, not gameplay logic

### Hardware Limitations
- **Multi-threading implementation** - GBA is single-threaded, keep simple approach
- **Exceeding VRAM limitations** - Work within 96KB VRAM constraint
- **Bypassing Butano framework** - Leverage existing background management abstractions

### Performance Trade-offs
- **Resolution increases** - Maintain current GBA screen resolution (240x160)
- **Background layer count changes** - Preserve existing layer usage patterns
- **Sprite system modifications** - No changes needed to entity rendering

## Traceability

| Requirement | Phase | Success Criteria | Status |
|--------------|----------|------------------|---------|
| STREAM-01 | Phase 1 | 16x16 buffer maintained | Pending |
| STREAM-02 | Phase 1 | Distance-based loading preserved | Pending |
| STREAM-03 | Phase 1 | Chunk state tracking functional | Pending |
| STREAM-04 | Phase 1 | 9x9 area loading works | Pending |
| STREAM-05 | Phase 1 | Unloading beyond 4-chunk radius | Pending |
| STREAM-06 | Phase 1 | Player chunk tracking accurate | Pending |
| STREAM-07 | Phase 1 | DMA VBlank transfers working | Pending |
| STREAM-08 | Phase 1 | 64 tiles/frame limit respected | Pending |
| STREAM-09 | Phase 1 | Batch transfers optimized | Pending |
| STREAM-10 | Phase 1 | World-to-buffer conversion accurate | Pending |
| STREAM-11 | Phase 1 | Buffer wrapping handles correctly | Pending |
| STREAM-12 | Phase 1 | Buffer origin tracking maintained | Pending |
| STREAM-13 | Phase 1 | Background scrolling smooth | Pending |
| STREAM-14 | Phase 1 | Butano integration preserved | Pending |
| STREAM-15 | Phase 1 | Rendering pipeline compatible | Pending |
| STREAM-16 | Phase 2 | Edge detection triggers early | Pending |
| STREAM-17 | Phase 2 | Overlapping zones seamless | Pending |
| STREAM-18 | Phase 2 | Wave artifacts eliminated | Pending |
| STREAM-19 | Phase 2 | 60 FPS maintained during shifts | Pending |
| STREAM-20 | Phase 2 | Existing functionality preserved | Pending |
| STREAM-21 | Phase 2 | Incremental shifts implemented | Pending |
| STREAM-22 | Phase 2 | Gradual streaming during shifts | Pending |
| STREAM-23 | Phase 2 | Double-buffering functional | Pending |
| STREAM-24 | Phase 2 | VBlank synchronization working | Pending |
| STREAM-25 | Phase 3 | Rate adjusts to performance | Pending |
| STREAM-26 | Phase 3 | Fast movement increases budget | Pending |
| STREAM-27 | Phase 3 | Stationary reduces budget | Pending |
| STREAM-28 | Phase 3 | 60 FPS target maintained | Pending |
| STREAM-29 | Phase 4 | No additional VRAM used | Pending |
| STREAM-30 | Phase 4 | 128-slot limit respected | Pending |
| STREAM-31 | Phase 4 | Fragmentation prevented | Pending |
| STREAM-32 | Phase 4 | 60 FPS baseline preserved | Pending |
| STREAM-33 | Phase 4 | 16.78MHz constraints met | Pending |
| STREAM-34 | Phase 4 | 64 tiles/frame baseline kept | Pending |
| STREAM-35 | Phase 4 | Collision detection intact | Pending |
| STREAM-36 | Phase 4 | Entity positioning preserved | Pending |
| STREAM-37 | Phase 4 | Camera system compatible | Pending |

### Phase Coverage Summary

**Phase 1 - Foundation Validation:** 15 requirements (STREAM-01 to STREAM-15)  
**Phase 2 - Predictive Buffer Management:** 9 requirements (STREAM-16 to STREAM-24)  
**Phase 3 - Adaptive Streaming Performance:** 4 requirements (STREAM-25 to STREAM-28)  
**Phase 4 - Integration Validation:** 9 requirements (STREAM-29 to STREAM-37)  

**Total v1 Requirements:** 37/37 mapped ✓

## Quality Criteria

**Requirement Quality Standards:**
- **Specific and Testable:** "Eliminate wave artifacts during buffer recentering" (not "improve buffer management")
- **User-centric:** "Player experiences seamless background transitions" (not "system handles buffer shifts")
- **Atomic:** One capability per requirement (not "Implement predictive system with adaptive rates")
- **Independent:** Minimal dependencies between requirements for phased implementation

**Success Verification:**
- Wave artifacts must be completely eliminated during any player movement
- 60 FPS must be maintained throughout buffer recentering operations
- Existing gameplay and collision systems must remain fully functional
- Memory usage must not exceed current VRAM allocation patterns

---

*Requirements updated: 2026-01-24*