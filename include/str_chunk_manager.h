#ifndef STR_CHUNK_MANAGER_H
#define STR_CHUNK_MANAGER_H

#include "bn_fixed_point.h"
#include "bn_affine_bg_map_ptr.h"
#include "bn_affine_bg_map_cell.h"
#include "bn_vector.h"
#include "bn_optional.h"
#include "str_constants.h"
#include "str_world_map_data.h"

namespace str
{
    // Forward declarations
    class Enemy;

    // Chunk state tracking
    enum class ChunkState : uint8_t
    {
        UNLOADED,
        LOADING,
        LOADED
    };

    // Info about a loaded chunk
    struct LoadedChunk
    {
        int chunk_x;              // Chunk coordinate in world
        int chunk_y;
        ChunkState state;
        int buffer_slot_x;        // Position in view buffer (0-3)
        int buffer_slot_y;
    };

    class ChunkManager
    {
    public:
        ChunkManager();

        // Initialize with world map data
        void init(const WorldMapData& world_map, bn::affine_bg_map_cell* view_buffer);

        // Core update - call each frame with player position
        // Returns true if streaming is in progress
        bool update(const bn::fixed_point& player_world_pos);

        // Commit changes to VRAM (call after update returns false, or periodically)
        void commit_to_vram(bn::affine_bg_map_ptr& bg_map);

        // Coordinate conversion
        [[nodiscard]] bn::fixed_point world_to_buffer(const bn::fixed_point& world_pos) const;
        [[nodiscard]] bn::fixed_point buffer_to_world(const bn::fixed_point& buffer_pos) const;

        // Get tile at world position (for collision)
        [[nodiscard]] int get_tile_at_world(int world_tile_x, int world_tile_y) const;

        // Check if a world position is within loaded chunks
        [[nodiscard]] bool is_position_loaded(const bn::fixed_point& world_pos) const;

        // Get the current buffer origin in world tiles
        [[nodiscard]] int buffer_origin_x() const { return _buffer_origin_tile_x; }
        [[nodiscard]] int buffer_origin_y() const { return _buffer_origin_tile_y; }

        // Get player's current chunk
        [[nodiscard]] int player_chunk_x() const { return _player_chunk_x; }
        [[nodiscard]] int player_chunk_y() const { return _player_chunk_y; }

        // Check if currently streaming
        [[nodiscard]] bool is_streaming() const { return _is_streaming; }
        
        // Get performance data for validation
        [[nodiscard]] int get_chunks_processed_this_frame() const { return _chunks_processed_this_frame; }
        [[nodiscard]] int get_tiles_transferred_this_frame() const { return _tiles_transferred_this_frame; }
        [[nodiscard]] bool was_buffer_recentered_this_frame() const { return _buffer_recentered_this_frame; }

    private:
        const WorldMapData* _world_map;
        bn::affine_bg_map_cell* _view_buffer;

        // Buffer origin in world tile coordinates
        int _buffer_origin_tile_x;
        int _buffer_origin_tile_y;

        // Player's current chunk
        int _player_chunk_x;
        int _player_chunk_y;

        // Loaded chunks tracking (need 121+ for 11x11 LOAD_RANGE, use 128 to be safe)
        bn::vector<LoadedChunk, 128> _loaded_chunks;

        // Streaming state
        bool _is_streaming;
        int _pending_chunk_x;
        int _pending_chunk_y;
        int _stream_progress;
        bool _needs_vram_update;
        
        // Performance tracking for validation
        int _chunks_processed_this_frame;
        int _tiles_transferred_this_frame;
        bool _buffer_recentered_this_frame;

        // Internal methods
        void _determine_needed_chunks(const bn::fixed_point& player_world_pos);
        void _stream_pending_chunk();
        void _load_chunk_immediately(int chunk_x, int chunk_y);
        void _recenter_buffer_if_needed(int center_chunk_x, int center_chunk_y, int load_range);
        bool _is_chunk_loaded(int chunk_x, int chunk_y) const;
        void _queue_chunk_for_loading(int chunk_x, int chunk_y);
        int _get_buffer_index(int buffer_tile_x, int buffer_tile_y) const;
        int _find_loaded_chunk_index(int chunk_x, int chunk_y) const;
    };
}

#endif
