# Stranded Chunk Streaming Roadmap

**Project:** Stranded GBA Game - Predictive Buffer Management  
**Objective:** Eliminate buffer recentering wave artifacts through architectural improvements  
**Platform:** Game Boy Advance (GBA) with Butano C++ Framework  
**Created:** 2026-01-24  

## Overview

This roadmap transforms Stranded's chunk streaming system from one with visible buffer recentering artifacts to a seamless, predictive system that maintains 60 FPS performance. The project delivers predictive buffer shifting that eliminates the "wave effect" while preserving all existing functionality within GBA's strict hardware constraints.

## Phase Structure

| Phase | Goal | Requirements | Dependencies |
|-------|------|--------------|-------------|
| 1 - Foundation Validation | Ensure existing chunk streaming foundation is stable and properly instrumented | STREAM-01 to STREAM-15 | None |
| 2 - Predictive Buffer Management | Implement predictive buffer shifting to eliminate wave artifacts | STREAM-16 to STREAM-24 | Phase 1 |
| 3 - Adaptive Streaming Performance | Add adaptive streaming rates based on movement patterns | STREAM-25 to STREAM-28 | Phase 2 |
| 4 - Integration Validation | Validate complete system integration and optimize performance | STREAM-29 to STREAM-37 | Phase 3 |

## Phase Details

### Phase 1: Foundation Validation

**Goal:** Ensure existing chunk streaming foundation is stable and properly instrumented for predictive enhancement

**Dependencies:** None

**Requirements:** STREAM-01, STREAM-02, STREAM-03, STREAM-04, STREAM-05, STREAM-06, STREAM-07, STREAM-08, STREAM-09, STREAM-10, STREAM-11, STREAM-12, STREAM-13, STREAM-14, STREAM-15

**Success Criteria:**
1. 16x16 circular buffer maintains stable chunk loading/unloading within 4-chunk radius
2. DMA transfers reliably update VRAM during VBlank with 64 tiles/frame bandwidth limit
3. World-to-buffer coordinate conversion accurately handles wrapping for all player positions
4. Background scrolling integrates smoothly with Butano affine background system
5. All existing collision detection and entity positioning remain fully functional

### Phase 2: Predictive Buffer Management

**Goal:** Implement predictive buffer shifting to eliminate wave artifacts during player movement

**Dependencies:** Phase 1 (Foundation Validation)

**Requirements:** STREAM-16, STREAM-17, STREAM-18, STREAM-19, STREAM-20, STREAM-21, STREAM-22, STREAM-23, STREAM-24

**Success Criteria:**
1. Buffer shifts are triggered before chunks reach edge positions, preventing wave artifacts
2. Overlapping transition zones enable seamless chunk remapping without visual disruptions
3. Incremental tile streaming during buffer changes maintains consistent 60 FPS performance
4. Double-buffering approach allows invisible buffer origin changes during VBlank periods
5. All existing chunk loading and collision functionality remain intact during predictive shifts

### Phase 3: Adaptive Streaming Performance

**Goal:** Add adaptive streaming rates that optimize performance based on player movement patterns

**Dependencies:** Phase 2 (Predictive Buffer Management)

**Requirements:** STREAM-25, STREAM-26, STREAM-27, STREAM-28

**Success Criteria:**
1. Streaming budget increases up to 128 tiles/frame when player moves quickly
2. Streaming budget reduces to 32 tiles/frame when player remains stationary
3. Frame time monitoring maintains consistent 60 FPS target across all movement speeds
4. Adaptive rate changes are smooth and do not cause visible loading interruptions

### Phase 4: Integration Validation

**Goal:** Validate complete system integration and optimize performance within hardware constraints

**Dependencies:** Phase 3 (Adaptive Streaming Performance)

**Requirements:** STREAM-29, STREAM-30, STREAM-31, STREAM-32, STREAM-33, STREAM-34, STREAM-35, STREAM-36, STREAM-37

**Success Criteria:**
1. Total memory usage remains within existing VRAM allocation patterns (no additional allocation)
2. Chunk state tracking operates within existing 128-slot vector limit without fragmentation
3. 60 FPS performance baseline is maintained across all system operations
4. All existing game systems (collision, entities, camera) remain fully compatible
5. System operates reliably within GBA's 16.78MHz CPU and 96KB VRAM constraints

## Progress Tracking

| Phase | Status | Progress | Notes |
|-------|--------|----------|-------|
| 1 - Foundation Validation | Pending | 0/5 | Ready to begin |
| 2 - Predictive Buffer Management | Pending | 0/5 | Depends on Phase 1 |
| 3 - Adaptive Streaming Performance | Pending | 0/4 | Depends on Phase 2 |
| 4 - Integration Validation | Pending | 0/5 | Depends on Phase 3 |

## Coverage Analysis

**Total v1 Requirements:** 37  
**Requirements Mapped:** 37/37 ✓  
**Orphaned Requirements:** 0  
**Duplicate Mappings:** 0

### Requirement Distribution

| Phase | Requirements | Focus Area |
|-------|--------------|------------|
| Phase 1 | 15 requirements | Foundation systems (buffer, DMA, coordinates) |
| Phase 2 | 9 requirements | Predictive buffer shifting (core innovation) |
| Phase 3 | 4 requirements | Adaptive streaming rates |
| Phase 4 | 9 requirements | Integration and performance validation |

## Quality Assurance

**Success Verification:**
- Wave artifacts must be completely eliminated during any player movement pattern
- 60 FPS must be maintained throughout all buffer operations and adaptive streaming
- All existing gameplay systems must remain fully functional with no regressions
- Memory usage must not exceed current VRAM allocation patterns

**Risk Mitigation:**
- Phase 1 validates foundation before implementing complex predictive logic
- Phase 2 focuses on core innovation before performance optimizations
- Phase 3 adds performance tuning after core functionality is proven
- Phase 4 ensures complete system integration and hardware constraint compliance

---

*Roadmap created: 2026-01-24*  
*Ready for phase planning:* `/gsd-plan-phase 1`