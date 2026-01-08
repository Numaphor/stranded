#include "fe_hitbox.h"
#include "fe_constants.h"

namespace fe
{
    Hitbox::Hitbox() : _pos(0, 0), _width(0), _height(0) {}

    Hitbox::Hitbox(bn::fixed x, bn::fixed y, bn::fixed width, bn::fixed height) : _pos(x, y), _width(width), _height(height)
    {
    }

    Hitbox::Hitbox(bn::fixed x, bn::fixed y, bn::fixed width, bn::fixed height, HitboxType type)
        : _pos(x, y), _width(width), _height(height), _type(type)
    {
    }

    void Hitbox::set_x(bn::fixed x)
    {
        _pos.set_x(x);
    }

    void Hitbox::set_y(bn::fixed y)
    {
        _pos.set_y(y);
    }

    void Hitbox::set_position(bn::fixed_point position)
    {
        _pos = position;
    }

    void Hitbox::get_collision_points(bn::fixed_point pos, fe::directions direction, bn::fixed_point points[4]) const
    {
        bn::fixed left = pos.x();
        bn::fixed right = pos.x() + _width - HITBOX_EDGE_OFFSET;
        bn::fixed top = pos.y();
        bn::fixed bottom = pos.y() + _height - HITBOX_EDGE_OFFSET;
        bn::fixed middle_x = pos.x() + _width / 2;
        bn::fixed quarter_x = pos.x() + _width / 4;
        bn::fixed middle_y = pos.y() + _height / 2;
        bn::fixed quarter_y = pos.y() + _height / 4;

        switch (direction)
        {
        case fe::directions::up:
            points[0] = bn::fixed_point(left, top);
            points[1] = bn::fixed_point(right, top);
            points[2] = bn::fixed_point(middle_x, top);
            points[3] = bn::fixed_point(quarter_x, top);
            break;
        case fe::directions::down:
            points[0] = bn::fixed_point(left, bottom);
            points[1] = bn::fixed_point(right, bottom);
            points[2] = bn::fixed_point(middle_x, bottom);
            points[3] = bn::fixed_point(quarter_x, bottom);
            break;
        case fe::directions::left:
            points[0] = bn::fixed_point(left, top);
            points[1] = bn::fixed_point(left, bottom);
            points[2] = bn::fixed_point(left, middle_y);
            points[3] = bn::fixed_point(left, quarter_y);
            break;
        case fe::directions::right:
            points[0] = bn::fixed_point(right, top);
            points[1] = bn::fixed_point(right, bottom);
            points[2] = bn::fixed_point(right, middle_y);
            points[3] = bn::fixed_point(right, quarter_y);
            break;
        default:
            points[0] = bn::fixed_point(left, top);
            points[1] = bn::fixed_point(right, top);
            points[2] = bn::fixed_point(left, bottom);
            points[3] = bn::fixed_point(right, bottom);
            break;
        }
    }

    bool Hitbox::contains_point(const bn::fixed_point &position) const
    {
        return position.x() >= x() && position.x() < x() + width() &&
               position.y() >= y() && position.y() < y() + height();
    }

    bool Hitbox::is_in_sword_zone(const bn::fixed_point &position)
    {
        const bn::fixed zone_left = fe::SWORD_ZONE_TILE_LEFT * fe::TILE_SIZE - fe::MAP_OFFSET;
        const bn::fixed zone_right = fe::SWORD_ZONE_TILE_RIGHT * fe::TILE_SIZE - fe::MAP_OFFSET;
        const bn::fixed zone_top = fe::SWORD_ZONE_TILE_TOP * fe::TILE_SIZE - fe::MAP_OFFSET;
        const bn::fixed zone_bottom = fe::SWORD_ZONE_TILE_BOTTOM * fe::TILE_SIZE - fe::MAP_OFFSET;

        return position.x() >= zone_left && position.x() < zone_right &&
               position.y() >= zone_top && position.y() < zone_bottom;
    }

    bool Hitbox::is_in_merchant_interaction_zone(const bn::fixed_point &position, const bn::fixed_point &merchant_center)
    {
        bn::fixed_point zone_position = calculate_centered_position(merchant_center, MERCHANT_INTERACTION_ZONE_WIDTH, MERCHANT_INTERACTION_ZONE_HEIGHT);
        return position.x() >= zone_position.x() && position.x() < zone_position.x() + MERCHANT_INTERACTION_ZONE_WIDTH &&
               position.y() >= zone_position.y() && position.y() < zone_position.y() + MERCHANT_INTERACTION_ZONE_HEIGHT;
    }

    bool Hitbox::is_in_merchant_collision_zone(const bn::fixed_point &position, const bn::fixed_point &merchant_center)
    {
        bn::fixed_point zone_position = calculate_centered_position(merchant_center, MERCHANT_COLLISION_ZONE_WIDTH, MERCHANT_COLLISION_ZONE_HEIGHT);
        return position.x() >= zone_position.x() && position.x() < zone_position.x() + MERCHANT_COLLISION_ZONE_WIDTH &&
               position.y() >= zone_position.y() && position.y() < zone_position.y() + MERCHANT_COLLISION_ZONE_HEIGHT;
    }

    Hitbox Hitbox::create_player_hitbox(bn::fixed_point position)
    {
        bn::fixed_point hitbox_pos = calculate_centered_position(position, PLAYER_HITBOX_WIDTH, PLAYER_HITBOX_HEIGHT);
        return Hitbox(hitbox_pos.x(), hitbox_pos.y(), PLAYER_HITBOX_WIDTH, PLAYER_HITBOX_HEIGHT, HitboxType::PLAYER);
    }

    Hitbox Hitbox::create_merchant_interaction_zone(bn::fixed_point center)
    {
        bn::fixed_point position = calculate_centered_position(center, MERCHANT_INTERACTION_ZONE_WIDTH, MERCHANT_INTERACTION_ZONE_HEIGHT);
        return Hitbox(position.x(), position.y(), MERCHANT_INTERACTION_ZONE_WIDTH, MERCHANT_INTERACTION_ZONE_HEIGHT, HitboxType::MERCHANT_INTERACTION);
    }

    Hitbox Hitbox::create_sword_zone()
    {
        const bn::fixed zone_left = fe::SWORD_ZONE_TILE_LEFT * fe::TILE_SIZE - fe::MAP_OFFSET;
        const bn::fixed zone_top = fe::SWORD_ZONE_TILE_TOP * fe::TILE_SIZE - fe::MAP_OFFSET;
        const bn::fixed width = (fe::SWORD_ZONE_TILE_RIGHT - fe::SWORD_ZONE_TILE_LEFT) * fe::TILE_SIZE;
        const bn::fixed height = (fe::SWORD_ZONE_TILE_BOTTOM - fe::SWORD_ZONE_TILE_TOP) * fe::TILE_SIZE;
        return Hitbox(zone_left, zone_top, width, height, HitboxType::SWORD_ZONE);
    }

    bn::optional<bn::fixed_point> ZoneManager::_merchant_zone_center;
    bool ZoneManager::_merchant_zone_enabled = false;

    void ZoneManager::set_merchant_zone_center(const bn::fixed_point &center)
    {
        _merchant_zone_center = center;
        _merchant_zone_enabled = true;
    }

    void ZoneManager::clear_merchant_zone()
    {
        _merchant_zone_center.reset();
        _merchant_zone_enabled = false;
    }

    void ZoneManager::set_merchant_zone_enabled(bool enabled)
    {
        _merchant_zone_enabled = enabled && _merchant_zone_center.has_value();
    }

    bn::optional<bn::fixed_point> ZoneManager::get_merchant_zone_center()
    {
        return _merchant_zone_center;
    }

    bool ZoneManager::is_merchant_zone_enabled()
    {
        return _merchant_zone_enabled && _merchant_zone_center.has_value();
    }

    bool ZoneManager::is_position_valid(const bn::fixed_point &position)
    {
        if (Hitbox::is_in_sword_zone(position))
            return false;

        if (is_merchant_zone_enabled() && _merchant_zone_center.has_value())
        {
            if (Hitbox::is_in_merchant_collision_zone(position, _merchant_zone_center.value()))
                return false;
        }

        return true;
    }
}
