#ifndef STR_CHUNK_VALIDATION_H
#define STR_CHUNK_VALIDATION_H

#include "bn_fixed_point.h"
#include "str_constants.h"
#include "str_chunk_manager.h"

// Forward declaration for logging
#include "bn_log.h"

namespace str
{
    // Log categories for chunk validation
    enum class ChunkLogCategory
    {
        CHUNK_STATE,
        BUFFER_MGMT,
        VALIDATION_ERROR,
        PERFORMANCE
    };

    // Performance metrics structure
    struct BufferMetrics
    {
        int active_chunks;
        int buffer_utilization;
        int chunks_loaded_this_frame;
        int chunks_unloaded_this_frame;
        int buffer_turnover_count;
    };

    // Main validation functions
    void log_chunk_state(int chunk_x, int chunk_y, ChunkState state, const char* context = nullptr);
    void log_buffer_utilization(const BufferMetrics& metrics);
    void log_buffer_overflow_warning(int buffer_slot_x, int buffer_slot_y, int chunk_x, int chunk_y);
    void log_buffer_underflow_warning(int buffer_slot_x, int buffer_slot_y);
    
    // Validation functions
    bool validate_buffer_stability(int buffer_origin_x, int buffer_origin_y);
    bool validate_chunk_state_transition(ChunkState old_state, ChunkState new_state);
    bool validate_buffer_bounds(int buffer_slot_x, int buffer_slot_y);
    
    // Performance tracking
    void track_buffer_turnover(int chunk_x, int chunk_y);
    BufferMetrics calculate_buffer_metrics();
    void reset_performance_counters();
    
    // Buffer efficiency validation
    bool validate_chunk_distribution(int center_chunk_x, int center_chunk_y);
    bool check_buffer_fragmentation();
    void validate_load_radius_efficiency(int center_chunk_x, int center_chunk_y, int load_range);
    
    // Performance stress testing
    void run_buffer_edge_case_tests();
    void test_wrapping_at_boundaries();
    void test_rapid_direction_changes();
    
    // Utility functions for logging
    const char* chunk_state_to_string(ChunkState state);
    void log_buffer_recenter(int old_origin_x, int old_origin_y, int new_origin_x, int new_origin_y);
}

#endif // STR_CHUNK_VALIDATION_H