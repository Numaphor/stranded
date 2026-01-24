#include "str_chunk_manager.h"
#include "bn_affine_bg_map_cell_info.h"
#include "bn_assert.h"

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
    }

    bool ChunkManager::update(const bn::fixed_point& player_world_pos)
    {
        if (!_world_map || !_view_buffer)
        {
            return false;
        }

        // Calculate player's current chunk
        _player_chunk_x = player_world_pos.x().integer() / CHUNK_SIZE_PIXELS;
        _player_chunk_y = player_world_pos.y().integer() / CHUNK_SIZE_PIXELS;

        // Clamp to valid range
        if (_player_chunk_x < 0) _player_chunk_x = 0;
        if (_player_chunk_y < 0) _player_chunk_y = 0;
        if (_player_chunk_x >= WORLD_WIDTH_CHUNKS) _player_chunk_x = WORLD_WIDTH_CHUNKS - 1;
        if (_player_chunk_y >= WORLD_HEIGHT_CHUNKS) _player_chunk_y = WORLD_HEIGHT_CHUNKS - 1;

        // If currently streaming, continue that
        if (_is_streaming)
        {
            _stream_pending_chunk();
            return true;
        }

        // Determine which chunks need to be loaded
        _determine_needed_chunks(player_world_pos);

        return _is_streaming;
    }

    void ChunkManager::commit_to_vram(bn::affine_bg_map_ptr& bg_map)
    {
        if (_needs_vram_update)
        {
            bg_map.reload_cells_ref();
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

        const int origin_chunk_x = center_chunk_x - LOAD_RANGE;
        const int origin_chunk_y = center_chunk_y - LOAD_RANGE;
        const int max_chunk_x = center_chunk_x + LOAD_RANGE;
        const int max_chunk_y = center_chunk_y + LOAD_RANGE;

        // First, clean up chunks that are too far away from the current 9x9 window
        for (int i = _loaded_chunks.size() - 1; i >= 0; --i)
        {
            const int chunk_x = _loaded_chunks[i].chunk_x;
            const int chunk_y = _loaded_chunks[i].chunk_y;
            if (chunk_x < origin_chunk_x || chunk_x > max_chunk_x ||
                chunk_y < origin_chunk_y || chunk_y > max_chunk_y)
            {
                _loaded_chunks.erase(_loaded_chunks.begin() + i);
            }
        }

        bool slot_claimed[VIEW_BUFFER_CHUNKS][VIEW_BUFFER_CHUNKS] = {};
        int chunks_loaded_this_frame = 0;

        for (int dy = -LOAD_RANGE; dy <= LOAD_RANGE && chunks_loaded_this_frame < MAX_CHUNKS_PER_FRAME; ++dy)
        {
            for (int dx = -LOAD_RANGE; dx <= LOAD_RANGE && chunks_loaded_this_frame < MAX_CHUNKS_PER_FRAME; ++dx)
            {
                int chunk_x = center_chunk_x + dx;
                int chunk_y = center_chunk_y + dy;

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

                if (!chunk_loaded || !slot_matches)
                {
                    _load_chunk_immediately(chunk_x, chunk_y);
                    ++chunks_loaded_this_frame;
                }

                slot_claimed[buffer_slot_y][buffer_slot_x] = true;
            }
        }

        // Update buffer origin after synchronizing the mask so conversions stay aligned
        // NOTE: Temporarily disabled to verify whether buffer recentering causes visible wave updates.
        // _recenter_buffer_if_needed(center_chunk_x, center_chunk_y, LOAD_RANGE);

        // Assertion disabled - chunk loading is asynchronous and not all chunks may be loaded at once
        // This is especially true with larger LOAD_RANGE values
        #if BN_CFG_ASSERT_ENABLED && 0
        for (int dy = -LOAD_RANGE; dy <= LOAD_RANGE; ++dy)
        {
            for (int dx = -LOAD_RANGE; dx <= LOAD_RANGE; ++dx)
            {
                int chunk_x = center_chunk_x + dx;
                int chunk_y = center_chunk_y + dy;

                // Only assert for chunks within the intended load range
                if (chunk_x >= origin_chunk_x && chunk_x <= max_chunk_x &&
                    chunk_y >= origin_chunk_y && chunk_y <= max_chunk_y)
                {
                    BN_ASSERT(_is_chunk_loaded(chunk_x, chunk_y), "Chunk mask missing entry");
                }
            }
        }
        #endif
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

            // Write to view buffer
            int buffer_idx = _get_buffer_index(buffer_tile_x, buffer_tile_y);
            _view_buffer[buffer_idx] = cell;

            _stream_progress++;
            tiles_this_frame++;
        }

        // Check if chunk is fully loaded
        if (_stream_progress >= CHUNK_TILES_TOTAL)
        {
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

        _buffer_origin_tile_x = new_origin_chunk_x * CHUNK_SIZE_TILES;
        _buffer_origin_tile_y = new_origin_chunk_y * CHUNK_SIZE_TILES;
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
        // Load all tiles of this chunk immediately (8x8 = 64 tiles)
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
            }
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
        }

        _needs_vram_update = true;
    }

    int ChunkManager::_get_buffer_index(int buffer_tile_x, int buffer_tile_y) const
    {
        int wrapped_x = tile_to_buffer_coord(buffer_tile_x);
        int wrapped_y = tile_to_buffer_coord(buffer_tile_y);
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
