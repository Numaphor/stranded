#ifndef STR_WORLD_MAP_DATA_H
#define STR_WORLD_MAP_DATA_H

#include "bn_affine_bg_map_cell.h"
#include "bn_fixed_point.h"
#include "str_constants.h"

namespace str
{
    // World map data stored in ROM
    struct WorldMapData
    {
        using tile_provider_fn = bn::affine_bg_map_cell (*)(int tile_x, int tile_y, const void* context);

        const bn::affine_bg_map_cell* cells = nullptr;  // Optional pointer to full map data in ROM
        tile_provider_fn provider = nullptr;            // Optional procedural provider callback
        const void* provider_context = nullptr;         // Context passed back to provider
        int width_tiles = 0;                            // Full map width in tiles (e.g., 1024)
        int height_tiles = 0;                           // Full map height in tiles (e.g., 1024)

        constexpr int width_pixels() const { return width_tiles * TILE_SIZE; }
        constexpr int height_pixels() const { return height_tiles * TILE_SIZE; }
        constexpr int total_cells() const { return width_tiles * height_tiles; }

        // Get cell at world tile coordinates
        bn::affine_bg_map_cell cell_at(int tile_x, int tile_y) const
        {
            if (tile_x < 0 || tile_x >= width_tiles || tile_y < 0 || tile_y >= height_tiles)
            {
                return bn::affine_bg_map_cell(0); // Out of bounds = empty
            }

            if (provider)
            {
                return provider(tile_x, tile_y, provider_context);
            }

            if (cells)
            {
                return cells[tile_y * width_tiles + tile_x];
            }

            return bn::affine_bg_map_cell(0);
        }
    };

    // Entity spawn data stored in ROM (per chunk)
    struct EntitySpawnData
    {
        int16_t local_x;          // Position within chunk (pixels)
        int16_t local_y;
        uint8_t entity_type;      // Enemy type or NPC type
        uint8_t initial_hp;
        uint8_t behavior_flags;
        uint8_t reserved;

        bn::fixed_point world_position(int chunk_x, int chunk_y) const
        {
            return bn::fixed_point(
                chunk_x * CHUNK_SIZE_PIXELS + local_x,
                chunk_y * CHUNK_SIZE_PIXELS + local_y
            );
        }
    };

    // World object spawn data (trees, buildings, etc.)
    enum class WorldObjectType : uint8_t
    {
        TREE_SMALL = 0,
        TREE_LARGE = 1,
        ROCK = 2,
        BUILDING = 3,
        SWORD = 4,  // The big sword object
        BUSH = 5
    };

    struct WorldObjectSpawnData
    {
        int16_t local_x;          // Position within chunk (pixels)
        int16_t local_y;
        WorldObjectType type;
        uint8_t variant;          // Visual variant
        uint8_t flags;            // Has collision, destructible, etc.
        uint8_t reserved;

        static constexpr uint8_t FLAG_HAS_COLLISION = 0x01;
        static constexpr uint8_t FLAG_DESTRUCTIBLE = 0x02;

        bool has_collision() const { return flags & FLAG_HAS_COLLISION; }
        bool is_destructible() const { return flags & FLAG_DESTRUCTIBLE; }

        bn::fixed_point world_position(int chunk_x, int chunk_y) const
        {
            return bn::fixed_point(
                chunk_x * CHUNK_SIZE_PIXELS + local_x,
                chunk_y * CHUNK_SIZE_PIXELS + local_y
            );
        }
    };

    // Chunk metadata stored in ROM
    struct ChunkData
    {
        const EntitySpawnData* entity_spawns;
        int entity_spawn_count;
        const WorldObjectSpawnData* object_spawns;
        int object_spawn_count;
    };
}

#endif
