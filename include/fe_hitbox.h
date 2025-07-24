#ifndef FE_HITBOX_H
#define FE_HITBOX_H

#include "bn_fixed.h"
#include "bn_fixed_point.h"

namespace fe
{
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