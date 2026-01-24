#ifndef STR_DMA_VALIDATION_H
#define STR_DMA_VALIDATION_H

#include <cstdint>
#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_log_level.h"

namespace str
{
    // DMA Channel constants for GBA hardware
    constexpr int DMA_CHANNEL_0 = 0;  // Highest priority, for background updates
    constexpr int DMA_CHANNEL_1 = 1;  // Medium priority
    constexpr int DMA_CHANNEL_2 = 2;  // Medium priority  
    constexpr int DMA_CHANNEL_3 = 3;  // Lowest priority, general purpose

    // DMA Transfer modes
    constexpr int DMA_16BIT = 0;     // 16-bit transfers
    constexpr int DMA_32BIT = 1;     // 32-bit transfers (optimal for tiles)

    // DMA Timing constants based on research
    constexpr int DMA_CYCLES_PER_32BIT_WORD = 6;  // ROM-to-VRAM: 6 cycles/word from research
    constexpr int VBLANK_SCANLINES = 280;         // Approximate VBlank window in scanlines
    constexpr int SCANLINE_CYCLES = 1232;         // Cycles per scanline at 16.78MHz
    constexpr int VBLANK_CYCLES_BUDGET = VBLANK_SCANLINES * SCANLINE_CYCLES;

    // Bandwidth limits from constants
    constexpr int TILES_PER_FRAME = 64;           // GBA hardware bandwidth limit
    constexpr int BYTES_PER_TILE = 32;             // 8x8 tiles = 32 bytes at 4bpp
    constexpr int WORDS_PER_TILE = 8;             // 32 bytes / 4 bytes per word
    constexpr int MAX_WORDS_PER_FRAME = TILES_PER_FRAME * WORDS_PER_TILE;  // 512 words
    constexpr int MAX_DMA_CYCLES = MAX_WORDS_PER_FRAME * DMA_CYCLES_PER_32BIT_WORD;  // 3072 cycles

    // Performance measurement structures
    struct DmaPerformanceMetrics
    {
        int tiles_transferred;
        int cycles_taken;
        int words_transferred;
        bool within_vblank;
        bool bandwidth_limit_respected;
        bn::fixed transfer_rate;  // tiles per cycle
        int bandwidth_utilization; // percentage of 64 tiles limit
    };

    // Batch transfer testing structures
    struct BatchEfficiencyResult
    {
        int batch_size;
        int setup_overhead_cycles;
        int transfer_cycles;
        int total_cycles;
        bn::fixed efficiency_ratio;  // transfer_cycles / total_cycles
        bool is_optimal;
    };

    // DMA validation functions
    void measure_dma_performance();
    void validate_vblank_timing();
    void test_batch_efficiency();
    void run_dma_stress_tests();
    void validate_hardware_compliance();
    void run_comprehensive_batch_tests();
    void test_batch_transfer_scenarios();
    
    // Performance measurement functions
    DmaPerformanceMetrics benchmark_dma_transfer(int tile_count, bool force_vblank = true);
    BatchEfficiencyResult measure_batch_efficiency(int batch_size);
    bool validate_transfer_within_vblank(int transfer_cycles);
    bool validate_bandwidth_limits(int tiles_transferred);
    
    // Logging functions for mGBA integration
    void log_dma_performance(const DmaPerformanceMetrics& metrics);
    void log_batch_efficiency(const BatchEfficiencyResult& result);
    void log_dma_error(const char* error_type, const char* details);
    
    // Validation utilities
    bool is_in_vblank();
    void wait_for_vblank_start();
    int measure_transfer_cycles(const void* source, void* dest, int word_count);
    bool validate_dma_alignment(const void* ptr);
}

#endif