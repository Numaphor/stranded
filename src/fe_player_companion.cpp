#include "fe_player_companion.h"
#include "bn_sprite_items_companion.h"
#include "bn_sprite_animate_actions.h"

namespace fe
{
    PlayerCompanion::PlayerCompanion(bn::sprite_ptr sprite)
        : _sprite(bn::move(sprite)), _position(0, 0), _position_side(Position::RIGHT), _is_dead(false),
          _follow_delay(0), _player_too_close(false), _target_offset(24, 0)
    {
        // Don't set z_order here - let the dynamic z_order system handle it
        // The player's update_z_order() method will manage companion z_order based on position
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
        // Define proximity thresholds for idle behavior
        constexpr bn::fixed IDLE_DISTANCE = 12;   // Stop moving when player gets this close
        constexpr bn::fixed RESUME_DISTANCE = 20; // Resume following when player moves this far away
        
        // Calculate direct distance from companion to player
        bn::fixed_point companion_to_player = player_pos - _position;
        bn::fixed player_distance = bn::sqrt(companion_to_player.x() * companion_to_player.x() + 
                                            companion_to_player.y() * companion_to_player.y());
        
        // Update proximity state with hysteresis to prevent oscillation
        if (!_player_too_close && player_distance < IDLE_DISTANCE)
        {
            _player_too_close = true;
        }
        else if (_player_too_close && player_distance > RESUME_DISTANCE)
        {
            _player_too_close = false;
        }
        
        // Only move if player is not too close
        if (!_player_too_close)
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
        }

        // Update position side more frequently for responsive sprite direction changes
        // Allow this even when not moving so companion can face the right direction
        if (player_distance > 8)
        {
            bn::fixed_point offset = _position - player_pos;
            Position new_side;
            
            // Check if companion is primarily above or below the player
            if (bn::abs(offset.y()) > bn::abs(offset.x()))
            {
                // Companion is primarily above or below player
                if (offset.y() < 0)
                {
                    // Companion is above player - should look down
                    // Use left/right animation based on horizontal offset
                    new_side = (offset.x() >= 0) ? Position::RIGHT : Position::LEFT;
                }
                else
                {
                    // Companion is below player
                    new_side = Position::BELOW;
                }
            }
            else
            {
                // Companion is primarily to the left or right of player
                new_side = (offset.x() > 0) ? Position::RIGHT : Position::LEFT;
            }
            
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
