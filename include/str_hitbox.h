#ifndef STR_HITBOX_H
#define STR_HITBOX_H

#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "str_constants.h"

namespace str
{
    enum class directions
    {
        up,
        down,
        left,
        right
    };

    enum class HitboxType
    {
        STANDARD,
        PLAYER,
        MERCHANT_COLLISION,
        MERCHANT_INTERACTION,
        SWORD_ZONE,
        ZONE_TILES
    };

    class Collision;

    class Hitbox
    {
    public:
        Hitbox();
        Hitbox(bn::fixed x, bn::fixed y, bn::fixed width, bn::fixed height);
        Hitbox(bn::fixed x, bn::fixed y, bn::fixed width, bn::fixed height, HitboxType type);

        friend class Collision;

        void get_collision_points(bn::fixed_point pos, directions direction, bn::fixed_point points[4]) const;

        [[nodiscard]] bool collides_with(const Hitbox &other) const
        {
            return !(x() + width() < other.x() ||
                     other.x() + other.width() < x() ||
                     y() + height() < other.y() ||
                     other.y() + other.height() < y());
        }

        [[nodiscard]] bn::fixed_point center() const
        {
            return bn::fixed_point(x() + width() / 2, y() + height() / 2);
        }

        [[nodiscard]] bn::fixed_point bottom_right() const
        {
            return bn::fixed_point(x() + width(), y() + height());
        }

        [[nodiscard]] static bn::fixed_point calculate_centered_position(bn::fixed_point center_point, bn::fixed width, bn::fixed height)
        {
            return bn::fixed_point(center_point.x() - width / 2, center_point.y() - height / 2);
        }

        [[nodiscard]] bool contains_point(const bn::fixed_point &position) const;

        [[nodiscard]] static bool is_in_sword_zone(const bn::fixed_point &position);
        [[nodiscard]] static bool is_in_merchant_interaction_zone(const bn::fixed_point &position, const bn::fixed_point &merchant_center);
        [[nodiscard]] static bool is_in_merchant_collision_zone(const bn::fixed_point &position, const bn::fixed_point &merchant_center);

        [[nodiscard]] static Hitbox create_player_hitbox(bn::fixed_point position);
        [[nodiscard]] static Hitbox create_merchant_interaction_zone(bn::fixed_point center);
        [[nodiscard]] static Hitbox create_sword_zone();

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

        void set_width(bn::fixed width);
        void set_height(bn::fixed height);
    };

    class ZoneManager
    {
    public:
        static void set_merchant_zone_center(const bn::fixed_point &center);
        static void clear_merchant_zone();
        static void set_merchant_zone_enabled(bool enabled);

        [[nodiscard]] static bn::optional<bn::fixed_point> get_merchant_zone_center();
        [[nodiscard]] static bool is_merchant_zone_enabled();
        [[nodiscard]] static bool is_position_valid(const bn::fixed_point &position);

    private:
        static bn::optional<bn::fixed_point> _merchant_zone_center;
        static bool _merchant_zone_enabled;
    };
}

#endif