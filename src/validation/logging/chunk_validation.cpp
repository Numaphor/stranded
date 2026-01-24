#include "chunk_validation.h"
#include "bn_log.h"
#include <cstring>

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
        
        // These would be calculated from actual ChunkManager data
        // For now, provide placeholder values
        metrics.active_chunks = s_chunks_loaded_count - s_chunks_unloaded_count;
        metrics.buffer_utilization = (metrics.active_chunks * 100) / (VIEW_BUFFER_CHUNKS * VIEW_BUFFER_CHUNKS);
        metrics.chunks_loaded_this_frame = s_chunks_loaded_count - s_last_frame_chunks_loaded;
        metrics.chunks_unloaded_this_frame = s_chunks_unloaded_count;
        metrics.buffer_turnover_count = s_buffer_turnover_total;
        
        s_last_frame_chunks_loaded = s_chunks_loaded_count;
        
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
}