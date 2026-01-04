#include "fe_player.h"
#include "bn_sprite_items_hero.h"

namespace fe
{
    PlayerVFX::PlayerVFX() : _last_vfx_state(PlayerMovement::State::IDLE), _last_vfx_direction(PlayerMovement::Direction::DOWN)
    {
    }

    void PlayerVFX::initialize(bn::camera_ptr camera)
    {
        _camera = camera;
    }

    void PlayerVFX::update(bn::fixed_point player_pos, PlayerMovement::State state, PlayerMovement::Direction direction)
    {
        if (should_show_vfx(state))
        {
            if (!_vfx_sprite.has_value())
            {
                // Create VFX sprite - using hero sprite for now
                _vfx_sprite = bn::sprite_items::hero.create_sprite(0, 0);
                if (_camera.has_value())
                {
                    _vfx_sprite->set_camera(*_camera);
                }
                _vfx_sprite->set_bg_priority(0);
                _vfx_sprite->set_z_order(-32000);
            }

            if (should_change_vfx(state, direction))
            {
                apply_vfx_state(state, direction);
            }

            // Update position to follow player
            _vfx_sprite->set_position(player_pos);

            // Update animation
            if (_vfx_animation.has_value())
            {
                _vfx_animation->update();
            }
        }
        else
        {
            hide_vfx();
        }

        _last_vfx_state = state;
        _last_vfx_direction = direction;
    }

    void PlayerVFX::apply_vfx_state(PlayerMovement::State state, PlayerMovement::Direction /*direction*/)
    {
        if (!_vfx_sprite.has_value())
            return;

        switch (state)
        {
            case PlayerMovement::State::POWER_BUFF:
                make_vfx_anim_range(12, 0, 5);
                break;
            case PlayerMovement::State::DEFENCE_BUFF:
                make_vfx_anim_range(8, 6, 11);
                break;
            case PlayerMovement::State::HEAL_BUFF:
                make_vfx_anim_range(10, 12, 17);
                break;
            case PlayerMovement::State::ENERGY_BUFF:
                make_vfx_anim_range(15, 18, 23);
                break;
            default:
                hide_vfx();
                break;
        }
    }

    void PlayerVFX::hide_vfx()
    {
        if (_vfx_sprite.has_value())
        {
            _vfx_sprite->set_visible(false);
        }
        _vfx_animation.reset();
    }

    bool PlayerVFX::should_show_vfx(PlayerMovement::State state) const
    {
        return state == PlayerMovement::State::POWER_BUFF ||
               state == PlayerMovement::State::DEFENCE_BUFF ||
               state == PlayerMovement::State::HEAL_BUFF ||
               state == PlayerMovement::State::ENERGY_BUFF;
    }

    bool PlayerVFX::should_change_vfx(PlayerMovement::State state, PlayerMovement::Direction direction) const
    {
        return state != _last_vfx_state || direction != _last_vfx_direction;
    }

    void PlayerVFX::make_vfx_anim_range(int speed, int start_frame, int end_frame)
    {
        if (!_vfx_sprite.has_value())
            return;

        _vfx_animation = bn::create_sprite_animate_action_forever(
            *_vfx_sprite, speed, bn::sprite_items::hero.tiles_item(), start_frame, end_frame);
    }

    void PlayerVFX::make_vfx_anim_range_once(int speed, int start_frame, int end_frame)
    {
        if (!_vfx_sprite.has_value())
            return;

        _vfx_animation = bn::create_sprite_animate_action_once(
            *_vfx_sprite, speed, bn::sprite_items::hero.tiles_item(), start_frame, end_frame);
    }
}
