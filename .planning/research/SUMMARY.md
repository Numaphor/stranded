# Project Research Summary

**Project:** GBA Chunk Streaming with Predictive Buffer Management
**Domain:** Game Boy Advance development, chunk streaming systems
**Researched:** 2026-01-24
**Confidence:** HIGH

## Executive Summary

This project is a Game Boy Advance chunk streaming system designed to enable large, infinite-scrolling worlds within the strict hardware constraints of the GBA platform. Expert implementations use circular buffer management with DMA-optimized VRAM updates, frame-based bandwidth budgeting, and predictive loading to maintain smooth 60fps performance. The current Stranded implementation follows established patterns but lacks predictive buffer management, causing visible "wave" artifacts during buffer recentering operations.

The research strongly recommends implementing a predictive buffer manager that anticipates player movement and shifts the circular buffer before edge conditions occur. This approach, combined with the existing Butano C++ framework and DMA-optimized tile streaming, eliminates visual artifacts while maintaining the 64 tiles/frame bandwidth budget. Key risks include buffer synchronization issues during recentering and VRAM bandwidth saturation, both mitigated through staggered chunk updates and careful performance monitoring.

## Key Findings

### Recommended Stack

GBA development with Butano C++ framework provides the optimal foundation, offering built-in background map management, memory allocation without heap usage, and hardware-optimized abstractions. The stack includes devkitARM r42+ toolchain, DMA transfers for critical VRAM updates, and circular buffer patterns for infinite world streaming. Butano's bn::bg_map_ptr system and custom memory pools specifically address GBA's 16KB VRAM and 256KB WRAM constraints while maintaining 60fps performance.

**Core technologies:**
- Butano C++ Framework 21.0.0 — Modern GBA engine with background management and VRAM handling
- DMA Transfers (Hardware) — Critical VRAM updates during VBlank, 10x faster than CPU copies  
- Circular Buffer Pattern — Proven GBA pattern for infinite worlds with no allocation overhead
- Dual Buffer System — Allows background updates while displaying, eliminates wave artifacts

### Expected Features

The feature landscape separates essential table stakes from competitive differentiators. MVP requires circular buffer management, distance-based loading, and DMA optimization - all foundational to any GBA streaming system. Competitive advantages come from predictive buffer shifting that eliminates recentering artifacts and adaptive streaming rates that adjust based on performance needs.

**Must have (table stakes):**
- Circular Buffer System — Required for efficient memory usage on limited GBA VRAM
- Basic Distance-Based Loading — Must load chunks based on player position
- DMA Transfer Optimization — Required for performance on 16MHz GBA CPU
- Tile Reference Counting — Prevents memory leaks and duplicate loading

**Should have (competitive):**
- Predictive Buffer Shifting — Eliminates visual artifacts during recentering (key differentiator)
- Adaptive Streaming Rate — Adjusts loading based on performance needs
- Prefetch on Movement Patterns — Anticipates player direction changes

**Defer (v2+):**
- Multi-Layer Coordination — Complex but eliminates tearing between layers
- Hot-Swapping Buffer Origins — Core to eliminating wave artifacts but very complex

### Architecture Approach

Component-separated architecture with clear boundaries between game loop, predictive management, chunk streaming, and background rendering. The current system uses Butano's affine background management with a custom ChunkManager handling 16x16 circular buffers. The critical missing component is a PredictiveBufferManager that integrates with existing coordinate conversion and DMA patterns.

**Major components:**
1. Predictive Manager — Analyzes player movement to predict buffer shifts and preload chunks
2. ChunkManager — Manages 16x16 chunk circular buffer and streaming
3. Background System — Manages affine background rendering and VRAM updates via Butano
4. Memory Management — Handles OBJ VRAM, WRAM allocation, and DMA transfers

### Critical Pitfalls

Buffer recentering synchronization issues are the most critical, causing visible "wave" artifacts when all chunks update simultaneously. VRAM bandwidth saturation during chunk loading and tile mapping race conditions also pose significant risks that must be addressed through careful timing and bandwidth management.

1. **Buffer Recentering Synchronization Issues** — Implement predictive buffer shifting with staggered chunk updates
2. **VRAM Bandwidth Saturation** — Never exceed 64 tiles/frame transfer budget, prioritize visible chunks
3. **Tile Mapping Race Conditions** — Only update tile mappings during VBlank periods using DMA
4. **Memory Fragmentation in Chunk Pool** — Use fixed-size chunk allocations with reference counting

## Implications for Roadmap

Based on research, suggested phase structure:

### Phase 1: Predictive Buffer Management
**Rationale:** Addresses the most visible user-facing issue (wave artifacts) and builds on existing ChunkManager foundation
**Delivers:** PredictiveBufferManager component with lookahead logic and seamless buffer shifting
**Addresses:** Predictive Buffer Shifting (FEATURES.md), eliminates Buffer Synchronization Issues (PITFALLS.md)
**Avoids:** Visual artifacts during buffer recentering through proactive chunk loading

### Phase 2: Enhanced Streaming Pipeline  
**Rationale:** Improves loading performance after predictive system provides stable foundation
**Delivers:** Adaptive streaming rate with frame-based bandwidth management and priority-based loading
**Uses:** DMA Transfers, Circular Buffer Pattern from STACK.md
**Implements:** Chunk loading priority system and bandwidth monitoring

### Phase 3: Architecture Validation & Optimization
**Rationale:** Ensures clean integration and optimizes remaining performance bottlenecks
**Delivers:** Validated component boundaries, optimized coordinate system, and comprehensive testing
**Implements:** Fixed-point coordinate system and memory pool optimizations
**Addresses:** Remaining moderate pitfalls like coordinate precision and caching strategies

### Phase Ordering Rationale

The order prioritizes user experience first (eliminating visible artifacts), then performance optimization, and finally architectural cleanup. This follows the dependency structure where predictive systems require basic distance-based loading, and enhanced streaming builds on the stable predictive foundation. The grouping naturally separates concerns: Phase 1 fixes visual issues, Phase 2 optimizes performance, Phase 3 ensures long-term maintainability.

### Research Flags

Phases likely needing deeper research during planning:
- **Phase 1:** Complex integration with existing ChunkManager coordinate conversion system needs careful API research
- **Phase 2:** Adaptive streaming algorithms need performance testing on GBA hardware constraints

Phases with standard patterns (skip research-phase):
- **Phase 3:** Builds on well-documented Butano abstractions and established GBA memory management patterns

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | Butano framework and GBA hardware docs are authoritative sources |
| Features | MEDIUM | Feature differentiation based on community consensus and practical GBA patterns |
| Architecture | HIGH | Current Stranded implementation provides concrete foundation, patterns well-documented |
| Pitfalls | MEDIUM | Hardware-specific risks validated by multiple GBA development sources |

**Overall confidence:** HIGH

### Gaps to Address

- Predictive algorithm optimization: Current research provides patterns but optimal lookahead distance needs empirical testing
- Performance baseline: Adaptive streaming rates require benchmarking on actual GBA hardware for accurate thresholds
- Multi-layer coordination: Deferring complex layer synchronization due to limited documentation on Butano-specific implementation

## Sources

### Primary (HIGH confidence)
- Butano Framework Documentation — Background map management, memory allocation, and DMA patterns
- GBATEK Technical Reference — GBA hardware specifications, memory layout, and DMA capabilities  
- Game Boy Advance Resource Management (Rafael Baptista) — Memory allocation patterns and streaming optimization
- devkitARM Documentation — Official toolchain specifications and GBA development patterns

### Secondary (MEDIUM confidence)
- Current Stranded Implementation — Existing ChunkManager architecture and identified recentering artifacts
- GBA Development Community — Common predictive buffer patterns (needs validation for Stranded's specific use case)
- Game Developer Magazine Archives — Practical GBA optimization techniques and performance guidelines

### Tertiary (LOW confidence)
- Academic research on Predictive Buffer Management — Theoretical algorithms need GBA-specific validation
- General chunk streaming patterns from various game engines — Requires adaptation for GBA hardware constraints

---
*Research completed: 2026-01-24*
*Ready for roadmap: yes*