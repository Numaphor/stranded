#include "str_chunk_manager.h"
#include "bn_affine_bg_map_cell_info.h"
#include "bn_assert.h"
#include "bn_log.h"
#include "bn_log_level.h"
#include "../validation/logging/chunk_validation.h"
#include "../validation/logging/distance_validation.h"
#include "../validation/dma/dma_validation.h"
#include <climits>

namespace
{
    [[nodiscard]] int positive_mod(int value, int modulus)
    {
        if (modulus == 0)
        {
            BN_LOG_LEVEL(bn::log_level::ERROR, "VALIDATION_ERROR:", "positive_mod called with modulus 0");
            return 0;
        }

        // Handle edge cases for maximum safety
        if (value == INT_MIN && modulus == -1)
        {
            BN_LOG_LEVEL(bn::log_level::WARN, "VALIDATION_ERROR:", "positive_mod edge case: INT_MIN % -1");
            return 0;
        }

        int result = value % modulus;
        if (result < 0)
        {
            result += modulus;
        }
        
        // Final validation
        if (result < 0 || result >= modulus)
        {
            BN_LOG_LEVEL(bn::log_level::ERROR, "VALIDATION_ERROR:", "positive_mod failed:",
                         value, "%", modulus, "=", result);
            return 0;
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
    ChunkManager::ChunkManager() :
        _world_map(nullptr),
        _view_buffer(nullptr),
        _buffer_origin_tile_x(0),
        _buffer_origin_tile_y(0),
        _player_chunk_x(0),
        _player_chunk_y(0),
        _is_streaming(false),
        _pending_chunk_x(0),
        _pending_chunk_y(0),
        _stream_progress(0),
        _needs_vram_update(false)
    {
    }

    void ChunkManager::init(const WorldMapData& world_map, bn::affine_bg_map_cell* view_buffer)
    {
        _world_map = &world_map;
        _view_buffer = view_buffer;
        _loaded_chunks.clear();

        // Initialize buffer with empty tiles
        for (int i = 0; i < VIEW_BUFFER_TILES * VIEW_BUFFER_TILES; ++i)
        {
            _view_buffer[i] = bn::affine_bg_map_cell(0);
        }

        // Validate initial buffer state
        validate_buffer_stability(_buffer_origin_tile_x, _buffer_origin_tile_y);
        reset_performance_counters();
        
        // Run initial performance tests
        run_buffer_edge_case_tests();
        test_wrapping_at_boundaries();
        
        // Initialize distance validation testing
        validate_world_boundary_distances();
        validate_manhattan_distance_calculations();
        benchmark_distance_calculations();
        
        // Run comprehensive performance testing
        run_comprehensive_performance_tests();
    }

    bool ChunkManager::update(const bn::fixed_point& player_world_pos)
    {
        if (!_world_map || !_view_buffer)
        {
            return false;
        }

        // Store old player chunk for tracking
        int old_player_chunk_x = _player_chunk_x;
        int old_player_chunk_y = _player_chunk_y;

        // Calculate player's current chunk
        _player_chunk_x = player_world_pos.x().integer() / CHUNK_SIZE_PIXELS;
        _player_chunk_y = player_world_pos.y().integer() / CHUNK_SIZE_PIXELS;

        // Clamp to valid range
        if (_player_chunk_x < 0) _player_chunk_x = 0;
        if (_player_chunk_y < 0) _player_chunk_y = 0;
        if (_player_chunk_x >= WORLD_WIDTH_CHUNKS) _player_chunk_x = WORLD_WIDTH_CHUNKS - 1;
        if (_player_chunk_y >= WORLD_HEIGHT_CHUNKS) _player_chunk_y = WORLD_HEIGHT_CHUNKS - 1;

        // Track player chunk changes for distance validation
        track_player_chunk(old_player_chunk_x, old_player_chunk_y, _player_chunk_x, _player_chunk_y);

        // If currently streaming, continue that
        if (_is_streaming)
        {
            _stream_pending_chunk();
            return true;
        }

        // Determine which chunks need to be loaded
        _determine_needed_chunks(player_world_pos);

        // Log buffer utilization metrics
        BufferMetrics metrics = calculate_buffer_metrics();
        log_buffer_utilization(metrics);
        
        // Validate chunk distribution efficiency periodically
        static int frame_counter = 0;
        frame_counter++;
        if (frame_counter % 300 == 0) // Every 5 seconds at 60 FPS
        {
            validate_load_radius_efficiency(_player_chunk_x, _player_chunk_y, 4);
            check_buffer_fragmentation();
            validate_chunk_distribution(_player_chunk_x, _player_chunk_y);
        }

        return _is_streaming;
    }

    void ChunkManager::commit_to_vram(bn::affine_bg_map_ptr& bg_map)
    {
        if (_needs_vram_update)
        {
            // Measure DMA performance before transfer
            str::DmaPerformanceMetrics metrics = str::benchmark_dma_transfer(TILES_PER_FRAME, true);
            
            // Log DMA transfer performance with chunk manager context
            BN_LOG_LEVEL(bn::log_level::INFO, "CHUNK_DMA: commit_to_vram transferring ", metrics.tiles_transferred, " tiles");
            BN_LOG_LEVEL(bn::log_level::INFO, "CHUNK_DMA: bandwidth utilization ", metrics.bandwidth_utilization, "%");
            BN_LOG_LEVEL(bn::log_level::INFO, "CHUNK_DMA: VBlank timing ", metrics.within_vblank ? "OK" : "VIOLATION");
            
            // Validate constraints before transfer
            if (!metrics.bandwidth_limit_respected)
            {
                BN_LOG_LEVEL(bn::log_level::ERROR, "CHUNK_DMA_ERROR: Bandwidth limit exceeded - ", 
                             metrics.tiles_transferred, " > ", TILES_PER_FRAME, " tiles");
            }
            
            if (!metrics.within_vblank)
            {
                BN_LOG_LEVEL(bn::log_level::ERROR, "CHUNK_DMA_ERROR: Transfer outside VBlank period");
            }
            
            // Validate VBlank timing constraint
            if (!str::validate_transfer_within_vblank(metrics.cycles_taken))
            {
                BN_LOG_LEVEL(bn::log_level::ERROR, "CHUNK_DMA_ERROR: Transfer exceeds VBlank window - ", 
                             metrics.cycles_taken, " cycles > ", str::VBLANK_CYCLES_BUDGET, " budget");
            }
            
            // Perform the actual VRAM update using Butano's optimized method
            // This internally uses DMA transfers aligned with VBlank periods
            bg_map.reload_cells_ref();
            
            // Log successful completion
            BN_LOG_LEVEL(bn::log_level::INFO, "CHUNK_DMA: VRAM update completed successfully");
            
            _needs_vram_update = false;
        }
    }

    void ChunkManager::_determine_needed_chunks(const bn::fixed_point& player_world_pos)
    {
        (void)player_world_pos;
        constexpr int LOAD_RANGE = 4;  // Balanced: 9x9 chunks (81 total) - good safe zone, better performance
        constexpr int MAX_CHUNKS_PER_FRAME = 8;  // Conservative limit to maintain 60 FPS

        const int center_chunk_x = _player_chunk_x;
        const int center_chunk_y = _player_chunk_y;

        // Use distance validation constants for consistency
        static_assert(LOAD_RANGE == CHUNK_LOAD_DISTANCE, "Load range must match CHUNK_LOAD_DISTANCE");

        // First, clean up chunks that are outside the 4-chunk radius using distance validation
        for (int i = _loaded_chunks.size() - 1; i >= 0; --i)
        {
            const int chunk_x = _loaded_chunks[i].chunk_x;
            const int chunk_y = _loaded_chunks[i].chunk_y;
            
            // Use distance validation for precise boundary detection
            if (!is_chunk_within_load_distance(center_chunk_x, center_chunk_y, chunk_x, chunk_y))
            {
                // Log chunk unloading with distance information
                int distance = calculate_manhattan_distance(center_chunk_x, center_chunk_y, chunk_x, chunk_y);
                log_load_radius(center_chunk_x, center_chunk_y, chunk_x, chunk_y, distance);
                log_chunk_state(chunk_x, chunk_y, ChunkState::UNLOADED, "_determine_needed_chunks distance cleanup");
                _loaded_chunks.erase(_loaded_chunks.begin() + i);
            }
        }

        bool slot_claimed[VIEW_BUFFER_CHUNKS][VIEW_BUFFER_CHUNKS] = {};
        int chunks_loaded_this_frame = 0;

        // Process chunks within load range with distance-validated logic
        for (int dy = -LOAD_RANGE; dy <= LOAD_RANGE && chunks_loaded_this_frame < MAX_CHUNKS_PER_FRAME; ++dy)
        {
            for (int dx = -LOAD_RANGE; dx <= LOAD_RANGE && chunks_loaded_this_frame < MAX_CHUNKS_PER_FRAME; ++dx)
            {
                int chunk_x = center_chunk_x + dx;
                int chunk_y = center_chunk_y + dy;

                // Ensure chunk coordinates are within world bounds
                chunk_x = bn::clamp(chunk_x, 0, WORLD_WIDTH_CHUNKS - 1);
                chunk_y = bn::clamp(chunk_y, 0, WORLD_HEIGHT_CHUNKS - 1);

                // Calculate distance and validate loading logic
                int distance = calculate_manhattan_distance(center_chunk_x, center_chunk_y, chunk_x, chunk_y);
                bool should_load = is_chunk_within_load_distance(center_chunk_x, center_chunk_y, chunk_x, chunk_y);
                
                // Validate distance logic consistency
                validate_distance_logic(center_chunk_x, center_chunk_y, chunk_x, chunk_y);

                const int buffer_slot_x = chunk_to_buffer_slot(chunk_x);
                const int buffer_slot_y = chunk_to_buffer_slot(chunk_y);

                if (slot_claimed[buffer_slot_y][buffer_slot_x])
                {
                    continue;
                }

                const int loaded_index = _find_loaded_chunk_index(chunk_x, chunk_y);
                const bool chunk_loaded = loaded_index >= 0 &&
                    _loaded_chunks[loaded_index].state == ChunkState::LOADED;

                bool slot_matches = false;
                if (chunk_loaded)
                {
                    const LoadedChunk& loaded_chunk = _loaded_chunks[loaded_index];
                    slot_matches = (loaded_chunk.buffer_slot_x == buffer_slot_x &&
                                    loaded_chunk.buffer_slot_y == buffer_slot_y);
                }

                // Only load chunks that should be loaded according to distance validation
                if (should_load && (!chunk_loaded || !slot_matches))
                {
                    // Validate buffer slot before loading
                    if (!validate_buffer_bounds(buffer_slot_x, buffer_slot_y))
                    {
                        BN_LOG_LEVEL(bn::log_level::ERROR, "VALIDATION_ERROR:", "Skipping load for invalid slot:",
                                     buffer_slot_x, ",", buffer_slot_y);
                        continue;
                    }
                    
                    // Log loading with distance information
                    log_load_radius(center_chunk_x, center_chunk_y, chunk_x, chunk_y, distance);
                    _load_chunk_immediately(chunk_x, chunk_y);
                    ++chunks_loaded_this_frame;
                }
                else if (chunk_loaded && !should_load)
                {
                    // This shouldn't happen due to cleanup above, but add safety check
                    BN_LOG_LEVEL(bn::log_level::WARN, "DISTANCE_CALC:", "Loaded chunk outside radius:",
                                 chunk_x, ",", chunk_y, "dist:", distance);
                    log_chunk_state(chunk_x, chunk_y, ChunkState::UNLOADED, "_determine_needed_chunks radius violation");
                    _loaded_chunks.erase(_loaded_chunks.begin() + loaded_index);
                }

                slot_claimed[buffer_slot_y][buffer_slot_x] = true;
            }
        }

        // Update buffer origin after synchronizing the mask so conversions stay aligned
        // NOTE: Temporarily disabled to verify whether buffer recentering causes visible wave updates.
        // _recenter_buffer_if_needed(center_chunk_x, center_chunk_y, LOAD_RANGE);

        // Log distance-based loading efficiency
        LoadRadiusMetrics metrics = calculate_load_radius_metrics(center_chunk_x, center_chunk_y, LOAD_RANGE);
        log_load_radius_efficiency(metrics);
    }

    void ChunkManager::_stream_pending_chunk()
    {
        constexpr int CHUNK_TILES_TOTAL = CHUNK_SIZE_TILES * CHUNK_SIZE_TILES; // 1024

        int tiles_this_frame = 0;
        while (tiles_this_frame < TILES_PER_FRAME && _stream_progress < CHUNK_TILES_TOTAL)
        {
            int local_x = _stream_progress % CHUNK_SIZE_TILES;
            int local_y = _stream_progress / CHUNK_SIZE_TILES;

            // Calculate world tile position
            int world_tile_x = _pending_chunk_x * CHUNK_SIZE_TILES + local_x;
            int world_tile_y = _pending_chunk_y * CHUNK_SIZE_TILES + local_y;

            // Calculate buffer position (wrapping)
            int buffer_tile_x = tile_to_buffer_coord(world_tile_x);
            int buffer_tile_y = tile_to_buffer_coord(world_tile_y);

            // Get tile from world map
            bn::affine_bg_map_cell cell = _world_map->cell_at(world_tile_x, world_tile_y);

            // Write to view buffer with bounds checking
            int buffer_idx = _get_buffer_index(buffer_tile_x, buffer_tile_y);
            if (buffer_idx >= 0 && buffer_idx < VIEW_BUFFER_TILES * VIEW_BUFFER_TILES)
            {
                _view_buffer[buffer_idx] = cell;
            }
            else
            {
                BN_LOG_LEVEL(bn::log_level::ERROR, "VALIDATION_ERROR:", "Stream buffer index out of bounds:",
                             buffer_idx, "at tile", local_x, ",", local_y);
            }

            _stream_progress++;
            tiles_this_frame++;
        }

        // Check if chunk is fully loaded
        if (_stream_progress >= CHUNK_TILES_TOTAL)
        {
            // Validate DMA bandwidth for this frame's transfers
            if (tiles_this_frame > 0)
            {
                str::DmaPerformanceMetrics frame_metrics = str::benchmark_dma_transfer(tiles_this_frame, true);
                if (!frame_metrics.bandwidth_limit_respected)
                {
                    BN_LOG_LEVEL(bn::log_level::WARN, "CHUNK_STREAM_WARN: Frame bandwidth usage ", 
                                 frame_metrics.bandwidth_utilization, "% exceeds recommended limit");
                }
                
                // Log streaming performance
                BN_LOG_LEVEL(bn::log_level::INFO, "CHUNK_STREAM: Loaded ", _pending_chunk_x, ",", _pending_chunk_y, 
                             " with ", tiles_this_frame, " tiles this frame (", frame_metrics.bandwidth_utilization, "% bandwidth)");
            }
            
            // Mark chunk as loaded
            LoadedChunk loaded;
            loaded.chunk_x = _pending_chunk_x;
            loaded.chunk_y = _pending_chunk_y;
            loaded.state = ChunkState::LOADED;
            loaded.buffer_slot_x = chunk_to_buffer_slot(_pending_chunk_x);
            loaded.buffer_slot_y = chunk_to_buffer_slot(_pending_chunk_y);

            // Remove old chunk in same slot if exists
            for (int i = _loaded_chunks.size() - 1; i >= 0; --i)
            {
                if (_loaded_chunks[i].buffer_slot_x == loaded.buffer_slot_x &&
                    _loaded_chunks[i].buffer_slot_y == loaded.buffer_slot_y)
                {
                    _loaded_chunks.erase(_loaded_chunks.begin() + i);
                }
            }

            if (!_loaded_chunks.full())
            {
                _loaded_chunks.push_back(loaded);
            }

            _is_streaming = false;
            _needs_vram_update = true;
        }
    }

    void ChunkManager::_recenter_buffer_if_needed(int center_chunk_x, int center_chunk_y, int load_range)
    {
        const int load_diameter = load_range * 2 + 1;
        // Use a much smaller buffer radius to delay recentering - only recenter when very close to edge
        const int buffer_load_radius = 1;  // Only recenter when 1 chunk from buffer edge, not load_range

        int origin_chunk_x = _buffer_origin_tile_x / CHUNK_SIZE_TILES;
        int origin_chunk_y = _buffer_origin_tile_y / CHUNK_SIZE_TILES;

        int delta_chunks_x = 0;
        if (center_chunk_x - origin_chunk_x < buffer_load_radius)
        {
            delta_chunks_x = center_chunk_x - origin_chunk_x - buffer_load_radius;
        }
        else if ((origin_chunk_x + VIEW_BUFFER_CHUNKS - 1) - center_chunk_x < buffer_load_radius)
        {
            delta_chunks_x = center_chunk_x + buffer_load_radius - (origin_chunk_x + VIEW_BUFFER_CHUNKS - 1);
        }

        int delta_chunks_y = 0;
        if (center_chunk_y - origin_chunk_y < buffer_load_radius)
        {
            delta_chunks_y = center_chunk_y - origin_chunk_y - buffer_load_radius;
        }
        else if ((origin_chunk_y + VIEW_BUFFER_CHUNKS - 1) - center_chunk_y < buffer_load_radius)
        {
            delta_chunks_y = center_chunk_y + buffer_load_radius - (origin_chunk_y + VIEW_BUFFER_CHUNKS - 1);
        }

        if (delta_chunks_x == 0 && delta_chunks_y == 0)
        {
            return;
        }

        int new_origin_chunk_x = origin_chunk_x + delta_chunks_x;
        int new_origin_chunk_y = origin_chunk_y + delta_chunks_y;

        new_origin_chunk_x = bn::clamp(new_origin_chunk_x, 0, WORLD_WIDTH_CHUNKS - VIEW_BUFFER_CHUNKS);
        new_origin_chunk_y = bn::clamp(new_origin_chunk_y, 0, WORLD_HEIGHT_CHUNKS - VIEW_BUFFER_CHUNKS);

        int old_origin_x = _buffer_origin_tile_x;
        int old_origin_y = _buffer_origin_tile_y;
        
        _buffer_origin_tile_x = new_origin_chunk_x * CHUNK_SIZE_TILES;
        _buffer_origin_tile_y = new_origin_chunk_y * CHUNK_SIZE_TILES;
        
        // Log buffer recentering for validation
        log_buffer_recenter(old_origin_x, old_origin_y, _buffer_origin_tile_x, _buffer_origin_tile_y);
    }

    bool ChunkManager::_is_chunk_loaded(int chunk_x, int chunk_y) const
    {
        for (const auto& chunk : _loaded_chunks)
        {
            if (chunk.chunk_x == chunk_x && chunk.chunk_y == chunk_y &&
                chunk.state == ChunkState::LOADED)
            {
                return true;
            }
        }
        return false;
    }

    void ChunkManager::_queue_chunk_for_loading(int chunk_x, int chunk_y)
    {
        _pending_chunk_x = chunk_x;
        _pending_chunk_y = chunk_y;
        _stream_progress = 0;
        _is_streaming = true;
    }

    void ChunkManager::_load_chunk_immediately(int chunk_x, int chunk_y)
    {
        // Calculate distance for logging and validation
        int distance = calculate_manhattan_distance(_player_chunk_x, _player_chunk_y, chunk_x, chunk_y);
        
        // Log chunk state transition with distance information
        log_load_radius(_player_chunk_x, _player_chunk_y, chunk_x, chunk_y, distance);
        log_chunk_state(chunk_x, chunk_y, ChunkState::LOADING, "_load_chunk_immediately");
        
        // Load all tiles of this chunk immediately (8x8 = 64 tiles)
        // Track tiles loaded for DMA bandwidth validation
        int tiles_loaded = 0;
        for (int local_y = 0; local_y < CHUNK_SIZE_TILES; ++local_y)
        {
            for (int local_x = 0; local_x < CHUNK_SIZE_TILES; ++local_x)
            {
                int world_tile_x = chunk_x * CHUNK_SIZE_TILES + local_x;
                int world_tile_y = chunk_y * CHUNK_SIZE_TILES + local_y;

                // Calculate buffer position (wrapping)
                int buffer_tile_x = tile_to_buffer_coord(world_tile_x);
                int buffer_tile_y = tile_to_buffer_coord(world_tile_y);

                // Get tile from world map
                bn::affine_bg_map_cell cell = _world_map->cell_at(world_tile_x, world_tile_y);

                // Write to view buffer
                int buffer_idx = _get_buffer_index(buffer_tile_x, buffer_tile_y);
                _view_buffer[buffer_idx] = cell;
                tiles_loaded++;
            }
        }
        
        // Validate immediate load against DMA bandwidth limits
        str::DmaPerformanceMetrics immediate_load_metrics = str::benchmark_dma_transfer(tiles_loaded, false);
        BN_LOG_LEVEL(bn::log_level::INFO, "CHUNK_IMMEDIATE: Loaded ", chunk_x, ",", chunk_y, 
                     " with ", tiles_loaded, " tiles (", immediate_load_metrics.bandwidth_utilization, "% bandwidth)");
        
        if (tiles_loaded > TILES_PER_FRAME)
        {
            BN_LOG_LEVEL(bn::log_level::WARN, "CHUNK_IMMEDIATE_WARN: Immediate load ", 
                         tiles_loaded, " tiles exceeds frame budget of ", TILES_PER_FRAME);
        }

        // Mark chunk as loaded
        LoadedChunk loaded;
        loaded.chunk_x = chunk_x;
        loaded.chunk_y = chunk_y;
        loaded.state = ChunkState::LOADED;
        loaded.buffer_slot_x = chunk_to_buffer_slot(chunk_x);
        loaded.buffer_slot_y = chunk_to_buffer_slot(chunk_y);

        // Remove old chunk in same slot if exists
        for (int i = _loaded_chunks.size() - 1; i >= 0; --i)
        {
            if (_loaded_chunks[i].buffer_slot_x == loaded.buffer_slot_x &&
                _loaded_chunks[i].buffer_slot_y == loaded.buffer_slot_y)
            {
                _loaded_chunks.erase(_loaded_chunks.begin() + i);
            }
        }

        if (!_loaded_chunks.full())
        {
            _loaded_chunks.push_back(loaded);
            // Log successful chunk loading
            log_chunk_state(chunk_x, chunk_y, ChunkState::LOADED, "_load_chunk_immediately");
            track_buffer_turnover(chunk_x, chunk_y);
        }
        else
        {
            log_buffer_overflow_warning(loaded.buffer_slot_x, loaded.buffer_slot_y, chunk_x, chunk_y);
            
            // Emergency cleanup: remove oldest chunks to make space
            if (_loaded_chunks.size() > 100) // Emergency threshold
            {
                BN_LOG_LEVEL(bn::log_level::WARN, "BUFFER_MGMT:", "Emergency cleanup - removing oldest chunks");
                for (int i = 0; i < 10 && !_loaded_chunks.empty(); ++i)
                {
                    const LoadedChunk& old_chunk = _loaded_chunks[0];
                    log_chunk_state(old_chunk.chunk_x, old_chunk.chunk_y, ChunkState::UNLOADED, "emergency cleanup");
                    _loaded_chunks.erase(_loaded_chunks.begin());
                }
                
                // Retry adding the new chunk
                if (!_loaded_chunks.full())
                {
                    _loaded_chunks.push_back(loaded);
                    log_chunk_state(chunk_x, chunk_y, ChunkState::LOADED, "_load_chunk_immediately retry");
                    track_buffer_turnover(chunk_x, chunk_y);
                }
            }
        }

        _needs_vram_update = true;
    }

    int ChunkManager::_get_buffer_index(int buffer_tile_x, int buffer_tile_y) const
    {
        // Enhanced bounds checking with validation
        if (!validate_buffer_bounds(buffer_tile_x / CHUNK_SIZE_TILES, buffer_tile_y / CHUNK_SIZE_TILES))
        {
            log_buffer_underflow_warning(buffer_tile_x / CHUNK_SIZE_TILES, buffer_tile_y / CHUNK_SIZE_TILES);
            return 0; // Fallback to prevent crash
        }
        
        int wrapped_x = tile_to_buffer_coord(buffer_tile_x);
        int wrapped_y = tile_to_buffer_coord(buffer_tile_y);
        
        // Validate wrapping results
        if (wrapped_x < 0 || wrapped_x >= VIEW_BUFFER_TILES || wrapped_y < 0 || wrapped_y >= VIEW_BUFFER_TILES)
        {
            BN_LOG_LEVEL(bn::log_level::ERROR, "VALIDATION_ERROR:", "Wrapped coordinates out of bounds:",
                         wrapped_x, ",", wrapped_y, "from", buffer_tile_x, ",", buffer_tile_y);
            return 0; // Fallback
        }
        
        return wrapped_y * VIEW_BUFFER_TILES + wrapped_x;
    }

    int ChunkManager::_find_loaded_chunk_index(int chunk_x, int chunk_y) const
    {
        for (int i = 0; i < _loaded_chunks.size(); ++i)
        {
            if (_loaded_chunks[i].chunk_x == chunk_x && _loaded_chunks[i].chunk_y == chunk_y)
            {
                return i;
            }
        }
        return -1;
    }

    bn::fixed_point ChunkManager::world_to_buffer(const bn::fixed_point& world_pos) const
    {
        // Convert world position to buffer-relative position
        bn::fixed buffer_x = world_pos.x() - bn::fixed(_buffer_origin_tile_x * TILE_SIZE);
        bn::fixed buffer_y = world_pos.y() - bn::fixed(_buffer_origin_tile_y * TILE_SIZE);

        // Handle wrapping within the buffer (buffer is 0 to VIEW_BUFFER_TILES * TILE_SIZE)
        const bn::fixed buffer_size = bn::fixed(VIEW_BUFFER_TILES * TILE_SIZE);
        
        // Normalize to [0, buffer_size) range
        while (buffer_x < 0)
        {
            buffer_x += buffer_size;
        }
        while (buffer_x >= buffer_size)
        {
            buffer_x -= buffer_size;
        }
        
        while (buffer_y < 0)
        {
            buffer_y += buffer_size;
        }
        while (buffer_y >= buffer_size)
        {
            buffer_y -= buffer_size;
        }

        // Convert to centered coordinates (buffer center is 0,0)
        const bn::fixed half_size = buffer_size / 2;
        buffer_x -= half_size;
        buffer_y -= half_size;

        return bn::fixed_point(buffer_x, buffer_y);
    }

    bn::fixed_point ChunkManager::buffer_to_world(const bn::fixed_point& buffer_pos) const
    {
        // Convert from centered buffer coordinates to buffer-relative coordinates
        const bn::fixed half_size = bn::fixed(VIEW_BUFFER_TILES * TILE_SIZE / 2);
        bn::fixed buffer_relative_x = buffer_pos.x() + half_size;
        bn::fixed buffer_relative_y = buffer_pos.y() + half_size;

        // Add the current buffer origin in world space
        bn::fixed world_x = buffer_relative_x + bn::fixed(_buffer_origin_tile_x * TILE_SIZE);
        bn::fixed world_y = buffer_relative_y + bn::fixed(_buffer_origin_tile_y * TILE_SIZE);

        return bn::fixed_point(world_x, world_y);
    }

    int ChunkManager::get_tile_at_world(int world_tile_x, int world_tile_y) const
    {
        if (!_world_map)
        {
            return 0;
        }

        // Check bounds
        if (world_tile_x < 0 || world_tile_x >= _world_map->width_tiles ||
            world_tile_y < 0 || world_tile_y >= _world_map->height_tiles)
        {
            return 0;
        }

        // Get from world map (ROM)
        bn::affine_bg_map_cell cell = _world_map->cell_at(world_tile_x, world_tile_y);
        bn::affine_bg_map_cell_info info(cell);
        return info.tile_index();
    }

    bool ChunkManager::is_position_loaded(const bn::fixed_point& world_pos) const
    {
        int chunk_x = world_pos.x().integer() / CHUNK_SIZE_PIXELS;
        int chunk_y = world_pos.y().integer() / CHUNK_SIZE_PIXELS;
        return _is_chunk_loaded(chunk_x, chunk_y);
    }
}
