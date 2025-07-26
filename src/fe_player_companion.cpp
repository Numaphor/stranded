#include "fe_player_companion.h"
#include "bn_sprite_items_companion.h"
#include "bn_sprite_animate_actions.h"

namespace fe
{
    PlayerCompanion::PlayerCompanion(bn::sprite_ptr sprite)
        : _sprite(bn::move(sprite)), _position(0, 0), _position_side(Position::RIGHT), _is_dead(false),
          _follow_delay(0), _target_offset(24, 0)
    {
        _sprite.set_z_order(0);
    }

    void PlayerCompanion::spawn(bn::fixed_point pos, bn::camera_ptr camera)
    {
        _position = pos + bn::fixed_point(16, -16);
        _target_offset = calculate_companion_offset();
        _sprite.set_camera(camera);
        update_animation();
    }

    void PlayerCompanion::update(bn::fixed_point player_pos, bool player_is_dead)
    {
        if (player_is_dead != _is_dead)
        {
            _is_dead = player_is_dead;
            update_animation();
        }

        if (!_is_dead)
        {
            update_position(player_pos);
        }

        if (_animation && !_animation->done())
        {
            _animation->update();
        }
    }

    void PlayerCompanion::update_position(bn::fixed_point player_pos)
    {
        bn::fixed_point target_pos = player_pos + _target_offset;
        bn::fixed_point diff = target_pos - _position;
        bn::fixed distance = bn::sqrt(diff.x() * diff.x() + diff.y() * diff.y());

        if (distance > 1)
        {
            bn::fixed speed = (distance * 0.08 < 1.2) ? distance * 0.08 : 1.2;
            speed = (speed > 0.3) ? speed : 0.3;
            _position += (diff / distance) * speed;
        }

        if (distance > 20)
        {
            bn::fixed_point offset = _position - player_pos;
            Position new_side = (bn::abs(offset.x()) > bn::abs(offset.y())) ? (offset.x() > 0 ? Position::RIGHT : Position::LEFT) : Position::BELOW;
            set_position_side(new_side);
        }

        _sprite.set_position(_position);
    }

    bn::fixed_point PlayerCompanion::calculate_companion_offset() const
    {
        switch (_position_side)
        {
        case Position::RIGHT:
            return {16, 0};
        case Position::LEFT:
            return {-16, 0};
        case Position::BELOW:
            return {0, 12};
        default:
            return {16, 0};
        }
    }

    void PlayerCompanion::update_animation()
    {
        if (_is_dead)
        {
            _animation = bn::create_sprite_animate_action_once(
                _sprite, 8, bn::sprite_items::companion.tiles_item(),
                12, 13, 14, 15, 16, 17, 18, 19, 20, 21);
        }
        else
        {
            int start_frame = static_cast<int>(_position_side) * 4;
            _animation = bn::create_sprite_animate_action_forever(
                _sprite, 12, bn::sprite_items::companion.tiles_item(),
                start_frame, start_frame + 1, start_frame + 2, start_frame + 3);
        }
    }

    void PlayerCompanion::set_flying(bool flying)
    {
        _is_flying = flying;
        update_animation();
    }

    void PlayerCompanion::set_position_side(Position side)
    {
        if (_position_side != side)
        {
            _position_side = side;
            _target_offset = calculate_companion_offset();
            update_animation();
        }
    }

    void PlayerCompanion::set_visible(bool visible)
    {
        _sprite.set_visible(visible);
    }

    void PlayerCompanion::set_z_order(int z_order)
    {
        _sprite.set_z_order(z_order);
    }

    void PlayerCompanion::start_death_animation()
    {
        _is_dead = true;
        update_animation();
    }
}
