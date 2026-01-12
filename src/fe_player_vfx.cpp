#include "fe_player.h"
#include "bn_sprite_items_hero_vfx.h"

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
                // Create VFX sprite using hero_vfx
                _vfx_sprite = bn::sprite_items::hero_vfx.create_sprite(0, 0);
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

            // Ensure sprite is visible
            _vfx_sprite->set_visible(true);

            // Update position to follow player with offset for up/down attacks
            bn::fixed_point vfx_pos = player_pos;
            bool is_attack = (state == PlayerMovement::State::SLASHING ||
                              state == PlayerMovement::State::ATTACKING ||
                              state == PlayerMovement::State::CHOPPING);
            if (is_attack && (direction == PlayerMovement::Direction::UP ||
                              direction == PlayerMovement::Direction::DOWN))
            {
                vfx_pos = bn::fixed_point(player_pos.x() + 8, player_pos.y() + PLAYER_SPRITE_Y_OFFSET);
            }
            else
            {
                vfx_pos = bn::fixed_point(player_pos.x(), player_pos.y() + PLAYER_SPRITE_Y_OFFSET);
            }
            _vfx_sprite->set_position(vfx_pos);

            // Update animation - check if done to avoid crash on completed "once" animations
            if (_vfx_animation.has_value())
            {
                if (_vfx_animation->done())
                {
                    hide_vfx();
                }
                else
                {
                    _vfx_animation->update();
                }
            }
        }
        else
        {
            hide_vfx();
        }

        _last_vfx_state = state;
        _last_vfx_direction = direction;
    }

    void PlayerVFX::apply_vfx_state(PlayerMovement::State state, PlayerMovement::Direction direction)
    {
        if (!_vfx_sprite.has_value())
            return;

        // Set horizontal flip for left direction
        _vfx_sprite->set_horizontal_flip(direction == PlayerMovement::Direction::LEFT);

        // Frame indices using row-based calculation (64x64 sprites, 24 columns per row)
        // Matching hero_sword animation layout:
        // Row 7: chop_down (168+), Row 8: slash_down (192+)
        // Row 14: lr_slash (336+), Row 15: lr_slash2 (360+)
        // Row 19: chop_up (456+), Row 20: attack_up (480+)
        switch (state)
        {
        case PlayerMovement::State::SLASHING:
            if (direction == PlayerMovement::Direction::UP)
                make_vfx_anim_range_once(4, 480, 486); // attack_up (row 20, 7 frames)
            else if (direction == PlayerMovement::Direction::DOWN)
                make_vfx_anim_range_once(4, 192, 198); // slash_down (row 8, 7 frames)
            else
                make_vfx_anim_range_once(4, 336, 339); // lr_slash (row 14, 4 frames)
            break;
        case PlayerMovement::State::ATTACKING:
            if (direction == PlayerMovement::Direction::UP)
                make_vfx_anim_range_once(4, 480, 486); // attack_up (row 20, 7 frames)
            else if (direction == PlayerMovement::Direction::DOWN)
                make_vfx_anim_range_once(4, 192, 198); // slash_down (row 8, 7 frames)
            else
                make_vfx_anim_range_once(4, 360, 364); // lr_slash2 (row 15, 5 frames)
            break;
        case PlayerMovement::State::CHOPPING:
            if (direction == PlayerMovement::Direction::UP)
                make_vfx_anim_range_once(5, 456, 459); // chop_up (row 19, 4 frames)
            else if (direction == PlayerMovement::Direction::DOWN)
                make_vfx_anim_range_once(5, 168, 171); // chop_down (row 7, 4 frames)
            else
                make_vfx_anim_range_once(5, 336, 339); // lr_slash (row 14, 4 frames)
            break;
        case PlayerMovement::State::HEAL_BUFF:
            make_vfx_anim_range(4, 24, 47); // Row 1
            break;
        case PlayerMovement::State::DEFENCE_BUFF:
            make_vfx_anim_range(4, 48, 71); // Row 2
            break;
        case PlayerMovement::State::POWER_BUFF:
            make_vfx_anim_range(4, 72, 95); // Row 3
            break;
        case PlayerMovement::State::ENERGY_BUFF:
            make_vfx_anim_range(4, 96, 119); // Row 4
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
        return state == PlayerMovement::State::SLASHING ||
               state == PlayerMovement::State::ATTACKING ||
               state == PlayerMovement::State::CHOPPING ||
               state == PlayerMovement::State::POWER_BUFF ||
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

        bn::vector<uint16_t, 32> frames;
        for (int i = start_frame; i <= end_frame; ++i)
        {
            frames.push_back(i);
        }

        _vfx_animation = bn::sprite_animate_action<32>::forever(
            *_vfx_sprite, speed, bn::sprite_items::hero_vfx.tiles_item(),
            bn::span<const uint16_t>(frames.data(), frames.size()));
        _vfx_sprite->set_visible(true);
    }

    void PlayerVFX::make_vfx_anim_range_once(int speed, int start_frame, int end_frame)
    {
        if (!_vfx_sprite.has_value())
            return;

        bn::vector<uint16_t, 32> frames;
        for (int i = start_frame; i <= end_frame; ++i)
        {
            frames.push_back(i);
        }

        _vfx_animation = bn::sprite_animate_action<32>::once(
            *_vfx_sprite, speed, bn::sprite_items::hero_vfx.tiles_item(),
            bn::span<const uint16_t>(frames.data(), frames.size()));
        _vfx_sprite->set_visible(true);
    }
}
