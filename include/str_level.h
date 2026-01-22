#ifndef STR_LEVEL_H
#define STR_LEVEL_H

#include "bn_affine_bg_ptr.h"
#include "bn_affine_bg_map_ptr.h"
#include "bn_vector.h"
#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "bn_size.h"
#include "str_constants.h"

namespace str
{
    // Forward declaration
    class ChunkManager;
    class WorldObject;

    class Level
    {
    private:
        bn::vector<int, 32> _floor_tiles;
        bn::vector<int, 8> _zone_tiles;                   // Zone tile indices for collision
        bn::optional<bn::affine_bg_map_ptr> _bg_map_ptr;  // For non-chunked mode

        // Chunk-based collision (for large worlds)
        const ChunkManager* _chunk_manager;
        bn::vector<WorldObject*, 8> _world_objects;  // Active world objects (small for now to save IWRAM)

        // Hardcoded sword zone area for collision (independent of visual tiles)
        bool is_in_sword_zone(const bn::fixed_point &position) const;

        // Merchant zones (independent of visual tiles)
        bn::optional<bn::fixed_point> _merchant_zone_center;
        bool _merchant_zone_enabled = true;

        // Internal collision check using world coordinates (for chunked mode)
        [[nodiscard]] bool _is_position_valid_chunked(const bn::fixed_point &position) const;

        // Check collision with world objects
        [[nodiscard]] bool _collides_with_world_objects(const bn::fixed_point &position) const;

    public:
        Level();
        Level(bn::affine_bg_map_ptr bg);

        // Set chunk manager for large world collision
        void set_chunk_manager(const ChunkManager* chunk_manager);

        // World object collision management
        void add_world_object(WorldObject* obj);
        void remove_world_object(WorldObject* obj);
        void clear_world_objects();

        [[nodiscard]] bn::vector<int, 32> floor_tiles();

        // Check if a position is valid (not colliding with zones or other obstacles)
        [[nodiscard]] bool is_position_valid(const bn::fixed_point &position) const;

        // Designate specific tiles as zone tiles
        void add_zone_tile(int tile_index);

        // Merchant zone management
        void set_merchant_zone(const bn::fixed_point &center);
        void clear_merchant_zone();
        void set_merchant_zone_enabled(bool enabled);

        // Merchant interaction zone check (25x25 tile-based)
        [[nodiscard]] bool is_in_merchant_interaction_zone(const bn::fixed_point &position) const;

        // Improved merchant collision zone check (20x20) - for physical blocking
        [[nodiscard]] bool is_in_merchant_collision_zone(const bn::fixed_point &position) const;

        // Reset level state
        void reset();
    };
}

#endif