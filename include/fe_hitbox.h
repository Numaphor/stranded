#ifndef FE_HITBOX_H
#define FE_HITBOX_H

#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_sprite_ptr.h"
#include "bn_camera_ptr.h"
#include "bn_optional.h"
#include "bn_vector.h"
#include "fe_constants.h"

namespace fe
{
    // Comprehensive hitbox system constants - using centralized constants from fe_constants.h
    namespace hitbox_constants
    {
        // Standard sprite dimensions (these are hitbox-specific, not in fe_constants.h)
        constexpr bn::fixed PLAYER_HITBOX_WIDTH = 8;
        constexpr bn::fixed PLAYER_HITBOX_HEIGHT = 16;
        constexpr bn::fixed MARKER_SPRITE_SIZE = 4;

        // Zone dimensions (using centralized constants from fe_constants.h)
        constexpr bn::fixed MERCHANT_COLLISION_WIDTH = MERCHANT_COLLISION_ZONE_WIDTH;
        constexpr bn::fixed MERCHANT_COLLISION_HEIGHT = MERCHANT_COLLISION_ZONE_HEIGHT;
        constexpr bn::fixed MERCHANT_INTERACTION_WIDTH = MERCHANT_INTERACTION_ZONE_WIDTH;
        constexpr bn::fixed MERCHANT_INTERACTION_HEIGHT = MERCHANT_INTERACTION_ZONE_HEIGHT;

        // Note: Tile system constants are accessed directly from fe:: namespace
        // (SWORD_ZONE_TILE_*, TILE_SIZE, MAP_OFFSET, etc.)
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
        [[nodiscard]] static bool is_in_merchant_collision_zone(const bn::fixed_point &position);
        [[nodiscard]] static bool is_in_merchant_interaction_zone(const bn::fixed_point &position, const bn::fixed_point &merchant_center);

        // === DEBUG VISUALIZATION ===

        struct MarkerOffsetConfig
        {
            bn::fixed top_left_x;
            bn::fixed top_left_y;
            bn::fixed bottom_right_x;
            bn::fixed bottom_right_y;

            MarkerOffsetConfig(bn::fixed tl_x, bn::fixed tl_y, bn::fixed br_x, bn::fixed br_y)
                : top_left_x(tl_x), top_left_y(tl_y), bottom_right_x(br_x), bottom_right_y(br_y) {}
        };

        struct DebugMarkers
        {
            bn::optional<bn::sprite_ptr> top_left;
            bn::optional<bn::sprite_ptr> bottom_right;
            bn::optional<bn::sprite_ptr> hitbox_top_left;     // For dual-area entities
            bn::optional<bn::sprite_ptr> hitbox_bottom_right; // For dual-area entities

            void clear()
            {
                top_left.reset();
                bottom_right.reset();
                hitbox_top_left.reset();
                hitbox_bottom_right.reset();
            }

            void set_visible(bool visible)
            {
                if (top_left.has_value())
                    top_left->set_visible(visible);
                if (bottom_right.has_value())
                    bottom_right->set_visible(visible);
                if (hitbox_top_left.has_value())
                    hitbox_top_left->set_visible(visible);
                if (hitbox_bottom_right.has_value())
                    hitbox_bottom_right->set_visible(visible);
            }

            void update_positions(bn::fixed_point tl_pos, bn::fixed_point br_pos,
                                  bn::optional<bn::fixed_point> hitbox_tl_pos = bn::nullopt,
                                  bn::optional<bn::fixed_point> hitbox_br_pos = bn::nullopt)
            {
                if (top_left.has_value())
                    top_left->set_position(tl_pos);
                if (bottom_right.has_value())
                    bottom_right->set_position(br_pos);
                if (hitbox_top_left.has_value() && hitbox_tl_pos.has_value())
                    hitbox_top_left->set_position(*hitbox_tl_pos);
                if (hitbox_bottom_right.has_value() && hitbox_br_pos.has_value())
                    hitbox_bottom_right->set_position(*hitbox_br_pos);
            }
        };

        // Debug visualization methods
        void create_debug_markers(bn::camera_ptr camera, bool enabled = true);
        void update_debug_markers(bool enabled);
        void update_debug_marker_positions(); // Efficient position update without recreating sprites
        void clear_debug_markers();
        [[nodiscard]] MarkerOffsetConfig get_marker_config() const;

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

        // Debug visualization
        DebugMarkers _debug_markers;
        bn::optional<bn::camera_ptr> _camera;
        bool _debug_enabled = false;

        // Private methods
        void set_width(bn::fixed width);
        void set_height(bn::fixed height);

        // Debug marker calculation methods
        [[nodiscard]] bn::fixed_point calculate_top_left_marker_pos(bn::fixed x_offset = 0, bn::fixed y_offset = 0) const;
        [[nodiscard]] bn::fixed_point calculate_bottom_right_marker_pos(bn::fixed x_offset = 0, bn::fixed y_offset = 0) const;
        [[nodiscard]] bn::sprite_ptr create_marker(bn::fixed_point position, bool rotated = false) const;
        void update_markers_with_config(const MarkerOffsetConfig &config, bool use_hitbox_markers = false, bool enable_blending = false);
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