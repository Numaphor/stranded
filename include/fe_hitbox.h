#ifndef FE_HITBOX_H
#define FE_HITBOX_H

#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "fe_constants.h"

namespace fe
{
    // Comprehensive hitbox system constants - using centralized constants from fe_constants.h
    namespace hitbox_constants
    {
        // Zone dimensions (using centralized constants from fe_constants.h)
        // Improved merchant system with separate collision and interaction zones
        constexpr bn::fixed MERCHANT_COLLISION_WIDTH = fe::MERCHANT_COLLISION_ZONE_WIDTH;
        constexpr bn::fixed MERCHANT_COLLISION_HEIGHT = fe::MERCHANT_COLLISION_ZONE_HEIGHT;
        constexpr bn::fixed MERCHANT_INTERACTION_WIDTH = fe::MERCHANT_INTERACTION_ZONE_WIDTH;
        constexpr bn::fixed MERCHANT_INTERACTION_HEIGHT = fe::MERCHANT_INTERACTION_ZONE_HEIGHT;

        // Note: Tile system constants are accessed directly from fe:: namespace
        // (SWORD_ZONE_TILE_*, TILE_SIZE, MAP_OFFSET, etc.)
        // Player hitbox dimensions are now in fe:: namespace (PLAYER_HITBOX_WIDTH, PLAYER_HITBOX_HEIGHT)
    }

    enum class directions
    {
        up,
        down,
        left,
        right
    };

    enum class HitboxType
    {
        STANDARD,             // Regular entity hitbox
        PLAYER,               // Player hitbox with special marker positioning
        MERCHANT_COLLISION,   // Merchant 24x24 collision zone
        MERCHANT_INTERACTION, // Merchant 100x100 interaction zone
        SWORD_ZONE,           // Sword zone (tile-based)
        ZONE_TILES            // General zone tiles
    };

    // Forward declarations
    class Collision;

    class Hitbox
    {
    public:
        // Constructors
        Hitbox();
        Hitbox(bn::fixed x, bn::fixed y, bn::fixed width, bn::fixed height);
        Hitbox(bn::fixed x, bn::fixed y, bn::fixed width, bn::fixed height, HitboxType type);

        // Friend classes for access to private members
        friend class Collision;

        // === CORE HITBOX FUNCTIONALITY ===
        void get_collision_points(bn::fixed_point pos, directions direction, bn::fixed_point points[4]) const;

        // Check if this hitbox collides with another hitbox
        [[nodiscard]] bool collides_with(const Hitbox &other) const
        {
            // Simple AABB collision detection
            return !(x() + width() < other.x() ||
                     other.x() + other.width() < x() ||
                     y() + height() < other.y() ||
                     other.y() + other.height() < y());
        }

        // Helper functions for common hitbox calculations
        [[nodiscard]] bn::fixed_point center() const
        {
            return bn::fixed_point(x() + width() / 2, y() + height() / 2);
        }

        [[nodiscard]] bn::fixed_point bottom_right() const
        {
            return bn::fixed_point(x() + width(), y() + height());
        }

        // Calculate hitbox position centered on a given point
        [[nodiscard]] static bn::fixed_point calculate_centered_position(bn::fixed_point center_point, bn::fixed width, bn::fixed height)
        {
            return bn::fixed_point(center_point.x() - width / 2, center_point.y() - height / 2);
        }

        // === ZONE MANAGEMENT (from Level class) ===

        // Check if a position is within this hitbox zone
        [[nodiscard]] bool contains_point(const bn::fixed_point &position) const;

        // Static zone collision methods
        [[nodiscard]] static bool is_in_sword_zone(const bn::fixed_point &position);
        [[nodiscard]] static bool is_in_merchant_interaction_zone(const bn::fixed_point &position, const bn::fixed_point &merchant_center);
        // Improved merchant collision system - separate collision zone for physical blocking
        [[nodiscard]] static bool is_in_merchant_collision_zone(const bn::fixed_point &position, const bn::fixed_point &merchant_center);



        // Static factory methods for common hitbox types
        [[nodiscard]] static Hitbox create_player_hitbox(bn::fixed_point position);
        // Merchant collision zone factory method removed - only interaction zones remain
        [[nodiscard]] static Hitbox create_merchant_interaction_zone(bn::fixed_point center);
        [[nodiscard]] static Hitbox create_sword_zone();

        // Getters and setters
        void set_x(bn::fixed x);
        void set_y(bn::fixed y);
        void set_position(bn::fixed_point position);
        void set_type(HitboxType type) { _type = type; }

        [[nodiscard]] HitboxType get_type() const { return _type; }
        [[nodiscard]] bn::fixed x() const { return _pos.x(); }
        [[nodiscard]] bn::fixed y() const { return _pos.y(); }
        [[nodiscard]] bn::fixed width() const { return _width; }
        [[nodiscard]] bn::fixed height() const { return _height; }
        [[nodiscard]] bn::fixed_point pos() const { return _pos; }

    private:
        bn::fixed_point _pos;
        bn::fixed _width;
        bn::fixed _height;
        HitboxType _type = HitboxType::STANDARD;

        // Private methods
        void set_width(bn::fixed width);
        void set_height(bn::fixed height);
    };

    // === ZONE MANAGEMENT STATIC METHODS (replacing Level functionality) ===

    class ZoneManager
    {
    public:
        // Merchant zone management
        static void set_merchant_zone_center(const bn::fixed_point &center);
        static void clear_merchant_zone();
        static void set_merchant_zone_enabled(bool enabled);

        [[nodiscard]] static bn::optional<bn::fixed_point> get_merchant_zone_center();
        [[nodiscard]] static bool is_merchant_zone_enabled();

        // Zone validation
        [[nodiscard]] static bool is_position_valid(const bn::fixed_point &position);

    private:
        static bn::optional<bn::fixed_point> _merchant_zone_center;
        static bool _merchant_zone_enabled;
    };
}

#endif // FE_HITBOX_H