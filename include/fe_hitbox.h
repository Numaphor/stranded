#ifndef FE_HITBOX_H
#define FE_HITBOX_H

#include "bn_fixed.h"
#include "bn_fixed_point.h"

namespace fe
{
    // Hitbox system constants
    namespace hitbox_constants
    {
        // Standard sprite dimensions
        constexpr bn::fixed PLAYER_HITBOX_WIDTH = 16;
        constexpr bn::fixed PLAYER_HITBOX_HEIGHT = 32;
        constexpr bn::fixed MARKER_SPRITE_SIZE = 4;
        
        // Hitbox centering offsets (half of dimensions)
        constexpr bn::fixed PLAYER_HALF_WIDTH = PLAYER_HITBOX_WIDTH / 2;   // 8
        constexpr bn::fixed PLAYER_HALF_HEIGHT = PLAYER_HITBOX_HEIGHT / 2; // 16
        
        // Visual adjustment offsets for debug markers
        constexpr bn::fixed PLAYER_MARKER_X_OFFSET = 4;
        constexpr bn::fixed PLAYER_MARKER_Y_OFFSET = 20;
        constexpr bn::fixed MERCHANT_BASE_OFFSET = 36;
        constexpr bn::fixed MERCHANT_X_ADJUSTMENT = 20; // 16 + 4 from original calculation
        constexpr bn::fixed MERCHANT_Y_ADJUSTMENT = 12; // 4 + 4 + 4 from original calculation
        constexpr bn::fixed MERCHANT_BR_X_OFFSET = 16;
        constexpr bn::fixed MERCHANT_BR_Y_OFFSET = 8;
    }

    enum class directions
    {
        up,
        down,
        left,
        right
    };

    class Collision;   // Forward declaration
    class HitboxDebug; // Forward declaration

    class Hitbox
    {
    public:
        Hitbox();
        Hitbox(bn::fixed x, bn::fixed y, bn::fixed width, bn::fixed height);

        friend class Collision;   // Allow Collision to access private members
        friend class HitboxDebug; // Allow HitboxDebug to access private members

    public:
        void get_collision_points(bn::fixed_point pos, fe::directions direction, bn::fixed_point points[4]) const;

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

    public:
        void set_x(bn::fixed x);
        void set_y(bn::fixed y);

    private:
        bn::fixed_point _pos;
        bn::fixed _width;
        bn::fixed _height;

        [[nodiscard]] bn::fixed x() const;
        [[nodiscard]] bn::fixed y() const;
        [[nodiscard]] bn::fixed width() const;
        [[nodiscard]] bn::fixed height() const;
        [[nodiscard]] bn::fixed_point pos() const;

        void set_width(bn::fixed width);
        void set_height(bn::fixed height);
    };
}

#endif