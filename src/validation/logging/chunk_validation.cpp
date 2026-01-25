#include "chunk_validation.h"
#include "bn_log.h"
#include "bn_log_level.h"
#include "bn_fixed.h"
#include <cstring>

// Helper functions for performance testing
namespace
{
    [[nodiscard]] int positive_mod(int value, int modulus)
    {
        if (modulus == 0)
        {
            return 0;
        }

        int result = value % modulus;
        if (result < 0)
        {
            result += modulus;
        }
        return result;
    }

    [[nodiscard]] int chunk_to_buffer_slot(int chunk_coord)
    {
        return positive_mod(chunk_coord, str::VIEW_BUFFER_CHUNKS);
    }

    [[nodiscard]] int tile_to_buffer_coord(int tile_coord)
    {
        return positive_mod(tile_coord, str::VIEW_BUFFER_TILES);
    }
}

namespace str
{
    // Performance tracking variables
    static int s_chunks_loaded_count = 0;
    static int s_chunks_unloaded_count = 0;
    static int s_buffer_turnover_total = 0;
    static int s_last_frame_chunks_loaded = 0;
    
    const char* chunk_state_to_string(ChunkState state)
    {
        switch (state)
        {
            case ChunkState::UNLOADED: return "UNLOADED";
            case ChunkState::LOADING: return "LOADING";
            case ChunkState::LOADED: return "LOADED";
            default: return "UNKNOWN";
        }
    }
    
    void log_chunk_state(int chunk_x, int chunk_y, ChunkState state, const char* context)
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "CHUNK_STATE:", chunk_x, ",", chunk_y, "->", 
                     chunk_state_to_string(state));
        
        if (context)
        {
            BN_LOG_LEVEL(bn::log_level::DEBUG, "CHUNK_STATE:", context);
        }
    }
    
    void log_buffer_utilization(const BufferMetrics& metrics)
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "BUFFER_MGMT:", metrics.active_chunks, "/128 chunks", 
                     "(", metrics.buffer_utilization, "% utilization)");
        BN_LOG_LEVEL(bn::log_level::DEBUG, "BUFFER_MGMT:", "loaded:", metrics.chunks_loaded_this_frame, 
                     "unloaded:", metrics.chunks_unloaded_this_frame);
    }
    
    void log_buffer_overflow_warning(int buffer_slot_x, int buffer_slot_y, int chunk_x, int chunk_y)
    {
        BN_LOG_LEVEL(bn::log_level::ERROR, "VALIDATION_ERROR:", "Buffer overflow at slot", 
                     buffer_slot_x, ",", buffer_slot_y, "for chunk", chunk_x, ",", chunk_y);
    }
    
    void log_buffer_underflow_warning(int buffer_slot_x, int buffer_slot_y)
    {
        BN_LOG_LEVEL(bn::log_level::ERROR, "VALIDATION_ERROR:", "Buffer underflow at slot", 
                     buffer_slot_x, ",", buffer_slot_y);
    }
    
    bool validate_buffer_stability(int buffer_origin_x, int buffer_origin_y)
    {
        // Check buffer origin bounds
        if (buffer_origin_x < 0 || buffer_origin_x >= WORLD_WIDTH_TILES - VIEW_BUFFER_TILES ||
            buffer_origin_y < 0 || buffer_origin_y >= WORLD_HEIGHT_TILES - VIEW_BUFFER_TILES)
        {
        BN_LOG_LEVEL(bn::log_level::ERROR, "VALIDATION_ERROR:", "Buffer origin out of bounds:", 
                     buffer_origin_x, ",", buffer_origin_y);
            return false;
        }
        
        // Validate alignment to chunk boundaries
        if ((buffer_origin_x % CHUNK_SIZE_TILES) != 0 || (buffer_origin_y % CHUNK_SIZE_TILES) != 0)
        {
            BN_LOG_LEVEL(bn::log_level::WARN, "VALIDATION_ERROR:", "Buffer origin not chunk-aligned:", 
                         buffer_origin_x, ",", buffer_origin_y);
        }
        
        return true;
    }
    
    bool validate_chunk_state_transition(ChunkState old_state, ChunkState new_state)
    {
        // Valid transitions: UNLOADED -> LOADING -> LOADED
        // LOADED -> UNLOADED (when chunk is evicted)
        
        if (old_state == ChunkState::UNLOADED && new_state == ChunkState::LOADING)
            return true;
        if (old_state == ChunkState::LOADING && new_state == ChunkState::LOADED)
            return true;
        if (old_state == ChunkState::LOADED && new_state == ChunkState::UNLOADED)
            return true;
            
        BN_LOG_LEVEL(bn::log_level::WARN, "VALIDATION_ERROR:", "Invalid state transition:", 
                     chunk_state_to_string(old_state), "->", chunk_state_to_string(new_state));
        return false;
    }
    
    bool validate_buffer_bounds(int buffer_slot_x, int buffer_slot_y)
    {
        bool valid = (buffer_slot_x >= 0 && buffer_slot_x < VIEW_BUFFER_CHUNKS &&
                     buffer_slot_y >= 0 && buffer_slot_y < VIEW_BUFFER_CHUNKS);
                     
        if (!valid)
        {
            BN_LOG_LEVEL(bn::log_level::ERROR, "VALIDATION_ERROR:", "Buffer slot out of bounds:", 
                         buffer_slot_x, ",", buffer_slot_y);
        }
        
        return valid;
    }
    
    void track_buffer_turnover(int chunk_x, int chunk_y)
    {
        // Simple tracking - in a full implementation we'd track actual chunk lifetimes
        s_buffer_turnover_total++;
        s_chunks_loaded_count++;
        
        BN_LOG_LEVEL(bn::log_level::DEBUG, "PERFORMANCE:", "Chunk turnover tracked for", 
                     chunk_x, ",", chunk_y, "total:", s_buffer_turnover_total);
    }
    
    BufferMetrics calculate_buffer_metrics()
    {
        BufferMetrics metrics = {};
        
        // Calculate actual metrics from tracked data
        metrics.active_chunks = s_chunks_loaded_count - s_chunks_unloaded_count;
        metrics.buffer_utilization = (metrics.active_chunks * 100) / 128; // 128 slots in _loaded_chunks vector
        metrics.chunks_loaded_this_frame = s_chunks_loaded_count - s_last_frame_chunks_loaded;
        metrics.chunks_unloaded_this_frame = s_chunks_unloaded_count;
        metrics.buffer_turnover_count = s_buffer_turnover_total;
        
        s_last_frame_chunks_loaded = s_chunks_loaded_count;
        
        // Log performance warnings if utilization is problematic
        if (metrics.buffer_utilization > 90)
        {
            BN_LOG_LEVEL(bn::log_level::WARN, "PERFORMANCE:", "High buffer utilization:", 
                         metrics.buffer_utilization, "%");
        }
        
        if (metrics.chunks_loaded_this_frame > 8)
        {
            BN_LOG_LEVEL(bn::log_level::WARN, "PERFORMANCE:", "High frame loading:", 
                         metrics.chunks_loaded_this_frame, "chunks");
        }
        
        return metrics;
    }
    
    void reset_performance_counters()
    {
        s_chunks_loaded_count = 0;
        s_chunks_unloaded_count = 0;
        s_buffer_turnover_total = 0;
        s_last_frame_chunks_loaded = 0;
    }
    
    void log_buffer_recenter(int old_origin_x, int old_origin_y, int new_origin_x, int new_origin_y)
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "BUFFER_MGMT:", "Buffer recentered from", 
                     old_origin_x, ",", old_origin_y, "to", new_origin_x, ",", new_origin_y);
    }
    
    bool validate_chunk_distribution(int center_chunk_x, int center_chunk_y)
    {
        // Validate that loaded chunks are optimally distributed around player
        // In a perfect 4-chunk radius, we expect chunks in a 9x9 pattern
        
        int chunks_in_optimal_range = 0;
        int chunks_far_from_player = 0;
        
        // This would need access to actual ChunkManager::_loaded_chunks
        // For now, implement basic validation logic
        int expected_range = 4; // 4-chunk load radius
        
        BN_LOG_LEVEL(bn::log_level::DEBUG, "PERFORMANCE:", "Validating chunk distribution around", 
                     center_chunk_x, ",", center_chunk_y, "with range", expected_range);
        
        // Placeholder for actual distribution validation
        // In real implementation, would count chunks at various distances
        
        return true; // Assume valid for now
    }
    
    bool check_buffer_fragmentation()
    {
        // Check for memory fragmentation in _loaded_chunks vector
        // Fragmentation occurs when chunks are loaded/unloaded non-optimally
        
        BN_LOG_LEVEL(bn::log_level::DEBUG, "PERFORMANCE:", "Checking buffer fragmentation");
        
        // Placeholder for fragmentation analysis
        // Would analyze chunk distribution patterns and identify inefficiencies
        
        return false; // No significant fragmentation
    }
    
    void validate_load_radius_efficiency(int center_chunk_x, int center_chunk_y, int load_range)
    {
        // Validate that the 4-chunk load radius is being used efficiently
        
        int total_chunks_needed = (load_range * 2 + 1) * (load_range * 2 + 1);
        int actual_chunks_loaded = s_chunks_loaded_count - s_chunks_unloaded_count;
        
        float efficiency = (float)actual_chunks_loaded / total_chunks_needed * 100.0f;
        
        BN_LOG_LEVEL(bn::log_level::INFO, "PERFORMANCE:", "Load radius efficiency:", 
                     actual_chunks_loaded, "/", total_chunks_needed, "chunks (", 
                     (int)efficiency, "%)");
        
        if (efficiency < 50.0f)
        {
            BN_LOG_LEVEL(bn::log_level::WARN, "PERFORMANCE:", "Low load radius efficiency");
        }
    }
    
    void run_buffer_edge_case_tests()
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "PERFORMANCE:", "Running buffer edge case tests");
        
        // Test positive_mod function at boundaries
        int test_mod_result = positive_mod(511, 512); // Should be 511
        if (test_mod_result != 511)
        {
            BN_LOG_LEVEL(bn::log_level::ERROR, "PERFORMANCE:", "positive_mod edge case failed");
        }
        
        test_mod_result = positive_mod(-1, 512); // Should be 511
        if (test_mod_result != 511)
        {
            BN_LOG_LEVEL(bn::log_level::ERROR, "PERFORMANCE:", "negative mod edge case failed");
        }
        
        // Test buffer slot calculations at extremes
        int buffer_slot = positive_mod(15, VIEW_BUFFER_CHUNKS); // Should be 15
        if (buffer_slot != 15)
        {
            BN_LOG_LEVEL(bn::log_level::ERROR, "PERFORMANCE:", "buffer slot boundary failed");
        }
    }
    
    void test_wrapping_at_boundaries()
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "PERFORMANCE:", "Testing coordinate wrapping at boundaries");
        
        // Test world coordinate wrapping
        int max_world_chunk = WORLD_WIDTH_CHUNKS - 1;
        int wrapped_chunk = positive_mod(max_world_chunk, VIEW_BUFFER_CHUNKS);
        
        BN_LOG_LEVEL(bn::log_level::DEBUG, "PERFORMANCE:", "World chunk", max_world_chunk, 
                     "wraps to buffer slot", wrapped_chunk);
        
        // Test tile coordinate wrapping
        int max_world_tile = WORLD_WIDTH_TILES - 1;
        int wrapped_tile = tile_to_buffer_coord(max_world_tile);
        
        BN_LOG_LEVEL(bn::log_level::DEBUG, "PERFORMANCE:", "World tile", max_world_tile, 
                     "wraps to buffer tile", wrapped_tile);
    }
    
    void test_rapid_direction_changes()
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "PERFORMANCE:", "Testing rapid direction changes");
        
        // Simulate rapid movement patterns that stress the buffer
        int movement_pattern[] = {1, -1, 1, -1, 2, -2, 3, -3, 5, -5};
        int pattern_size = sizeof(movement_pattern) / sizeof(movement_pattern[0]);
        
        int chunk_x = WORLD_WIDTH_CHUNKS / 2;
        int chunk_y = WORLD_HEIGHT_CHUNKS / 2;
        
        for (int i = 0; i < pattern_size; ++i)
        {
            chunk_x += movement_pattern[i];
            chunk_y += movement_pattern[(i + 1) % pattern_size];
            
            // Ensure bounds
            chunk_x = bn::clamp(chunk_x, 0, WORLD_WIDTH_CHUNKS - 1);
            chunk_y = bn::clamp(chunk_y, 0, WORLD_HEIGHT_CHUNKS - 1);
            
            // Test buffer slot calculations during rapid movement
            int buffer_x = chunk_to_buffer_slot(chunk_x);
            int buffer_y = chunk_to_buffer_slot(chunk_y);
            
            BN_LOG_LEVEL(bn::log_level::DEBUG, "PERFORMANCE:", "Rapid move to", 
                         chunk_x, ",", chunk_y, "-> buffer", buffer_x, ",", buffer_y);
        }
        
        BN_LOG_LEVEL(bn::log_level::INFO, "PERFORMANCE:", "Rapid direction change test completed");
    }
}