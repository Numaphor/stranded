#ifndef STR_LEVEL_H
#define STR_LEVEL_H

#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_vector.h"
#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "bn_size.h" // Add the size header
#include "str_constants.h"

namespace str
{
    class Level
    {
    private:
        bn::vector<int, 32> _floor_tiles;
        bn::vector<int, 8> _zone_tiles;                   // New vector to store zone tile indices
        bn::optional<bn::regular_bg_map_ptr> _bg_map_ptr; // Make it optional to allow default construction

        // Hardcoded sword zone area for collision (independent of visual tiles)
        bool is_in_sword_zone(const bn::fixed_point &position) const;

        // Merchant zones (independent of visual tiles)
        bn::optional<bn::fixed_point> _merchant_zone_center;
        bool _merchant_zone_enabled = true; // Allow disabling during conversations

    public:
        Level() = default; // Now this is valid since _bg_map_ptr is optional
        Level(bn::regular_bg_map_ptr bg);

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