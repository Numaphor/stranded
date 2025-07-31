#ifndef FE_LEVEL_H
#define FE_LEVEL_H

#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_vector.h"
#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "bn_size.h" // Add the size header

namespace fe
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

        // Physical collision zone (24x24)
        bn::fixed _merchant_zone_width = 24;  // Small physical collision zone
        bn::fixed _merchant_zone_height = 24; // Small physical collision zone
        bool _merchant_zone_enabled = true;   // Allow disabling during conversations
        bool is_in_hitbox_zone(const bn::fixed_point &position) const;

        // Interaction zone (100x100) - tile-based like collision zone
        bn::fixed _merchant_interaction_zone_width = 100;  // Large interaction trigger zone
        bn::fixed _merchant_interaction_zone_height = 100; // Large interaction trigger zone

    public:
        Level() = default; // Now this is valid since _bg_map_ptr is optional
        Level(bn::regular_bg_map_ptr bg);

        [[nodiscard]] bn::vector<int, 32> floor_tiles();
        [[nodiscard]] const bn::vector<int, 8> &zone_tiles() const { return _zone_tiles; }

        // Check if a position is valid (not colliding with zones or other obstacles)
        [[nodiscard]] bool is_position_valid(const bn::fixed_point &position) const;

        // Designate specific tiles as zone tiles
        void add_zone_tile(int tile_index);

        // Merchant zone management
        void set_merchant_zone(const bn::fixed_point &center);
        void clear_merchant_zone();
        void set_merchant_zone_enabled(bool enabled);

        // Merchant interaction zone check (100x100 tile-based)
        [[nodiscard]] bool is_in_merchant_interaction_zone(const bn::fixed_point &position) const;

        // Merchant zone debug access
        [[nodiscard]] bn::optional<bn::fixed_point> get_merchant_zone_center() const { return _merchant_zone_center; }
        [[nodiscard]] bool is_merchant_zone_enabled() const { return _merchant_zone_enabled; }
        [[nodiscard]] bn::fixed get_merchant_zone_width() const { return _merchant_zone_width; }
        [[nodiscard]] bn::fixed get_merchant_zone_height() const { return _merchant_zone_height; }
        [[nodiscard]] bn::fixed get_merchant_interaction_zone_width() const { return _merchant_interaction_zone_width; }
        [[nodiscard]] bn::fixed get_merchant_interaction_zone_height() const { return _merchant_interaction_zone_height; }

        // Reset level state
        void reset();
    };
}

#endif