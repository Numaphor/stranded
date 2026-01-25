# Feature Landscape

**Domain:** GBA Chunk Streaming Systems
**Researched:** 2026-01-24
**Confidence:** MEDIUM

## Table Stakes

Features users expect. Missing = product feels incomplete.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Circular Buffer System | Required for efficient memory usage on limited GBA VRAM | Low | 16x16 chunk buffer (128x128 tiles total) is standard |
| Basic Distance-Based Loading | Must load chunks based on player position | Low | Essential for any large world game |
| DMA Transfer Optimization | Required for performance on 16MHz GBA CPU | Medium | 6 cycles per 32-bit word, critical for 64 tiles/frame target |
| World-to-Buffer Coordinate Conversion | Mathematical necessity for circular buffers | Medium | Required for correct chunk indexing |
| Background Scrolling Support | Core functionality for moving through world | Low | Uses BGHOFS/BGVOFS registers |
| Tile Reference Counting | Prevents memory leaks and duplicate loading | Medium | Essential for VRAM efficiency |

## Differentiators

Features that set product apart. Not expected, but valued.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| Predictive Buffer Shifting | Eliminates visual artifacts during recentering | High | **Key differentiator** - solves "wave" artifacts |
| Adaptive Streaming Rate | Adjusts loading based on performance needs | Medium | Can vary from 32-128 tiles/frame dynamically |
| Prefetch on Movement Patterns | Anticipates player direction changes | High | Requires pattern recognition and prediction |
| Multi-Layer Coordination | Synchronizes chunk loading across BG layers | High | Complex but eliminates tearing between layers |
| Compression-Aware Loading | Handles compressed chunks to save ROM space | Medium | Must balance decompression cost vs streaming speed |
| Hot-Swapping Buffer Origins | Seamless transition without visual interruption | Very High | Core to eliminating "wave" artifacts |

## Anti-Features

Features to explicitly NOT build. Common mistakes in this domain.

| Anti-Feature | Why Avoid | What to Do Instead |
|--------------|-----------|-------------------|
| Frame-Complete Loading | Blocks entire frame during chunk loads | Use DMA during VBlank only, spread across frames |
| Static Buffer Origins | Causes visible recentering artifacts | Implement smooth buffer origin shifting |
| Single-Screenblock Copying | Wastes 4x VRAM and CPU every frame | Use row/column incremental updates |
| Hardcoded Memory Layout | Doesn't scale with different map sizes | Use dynamic allocators from GBA Resource Management |
| Synchronous Asset Loading | Stalls gameplay during transitions | Pipeline loading with prediction |
| Full-Map VRAM Allocation | Exceeds 64K BG VRAM limitation | Implement tile streaming with reference counting |

## Feature Dependencies

```
Basic Distance-Based Loading
    ├──requires──> World-to-Buffer Coordinate Conversion
    ├──requires──> Circular Buffer System
    └──requires──> Background Scrolling Support

Tile Reference Counting
    ├──requires──> Circular Buffer System
    └──enhances──> DMA Transfer Optimization

Predictive Buffer Shifting
    ├──requires──> Basic Distance-Based Loading
    ├──requires──> World-to-Buffer Coordinate Conversion
    ├──requires──> DMA Transfer Optimization
    └──enhances──> Adaptive Streaming Rate

Prefetch on Movement Patterns
    ├──requires──> Predictive Buffer Shifting
    └──enhances──> Adaptive Streaming Rate

Multi-Layer Coordination
    ├──requires──> Predictive Buffer Shifting
    └──enhances──> Hot-Swapping Buffer Origins
```

### Dependency Notes

- **Basic Distance-Based Loading requires World-to-Buffer Coordinate Conversion:** Mathematical foundation for determining which chunks to load based on player position
- **Tile Reference Counting enhances DMA Transfer Optimization:** Prevents redundant DMA transfers by tracking tile usage
- **Predictive Buffer Shifting requires Basic Distance-Based Loading:** Must understand current loading patterns before implementing prediction
- **Prefetch on Movement Patterns enhances Adaptive Streaming Rate:** Pattern recognition feeds into adaptive rate decisions
- **Multi-Layer Coordination enhances Hot-Swapping Buffer Origins:** Layer synchronization requires seamless buffer transitions

## MVP Recommendation

For MVP, prioritize:
1. **Circular Buffer System** - Foundation for all streaming
2. **Basic Distance-Based Loading** - Core functionality for any large world
3. **DMA Transfer Optimization** - Required for 64 tiles/frame target
4. **Tile Reference Counting** - Prevents memory issues on GBA

Defer to post-MVP:
- **Predictive Buffer Shifting** - Implement after basic streaming works reliably
- **Adaptive Streaming Rate** - Add once performance baseline established
- **Prefetch on Movement Patterns** - Advanced feature for polished experience

## Sources

- Game Developer Magazine: "Gameboy Advance Resource Management" (HIGH confidence)
- Pin Eight: "Managing Sprite Cel VRAM on the Game Boy Advance" (HIGH confidence)
- exelo.tl: "Things I've learned since 'Goodboy Advance'" (MEDIUM confidence)
- Academic research on Predictive Buffer Management (LOW confidence for GBA-specific application)
- General chunk streaming patterns from various game engines (MEDIUM confidence)