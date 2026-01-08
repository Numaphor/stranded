#include "fe_player.h"
#include "bn_sprite_items_hero_sword.h"

namespace fe
{
    // PlayerAnimation Implementation
    PlayerAnimation::PlayerAnimation(bn::sprite_ptr sprite) : _sprite(sprite), _last_state(PlayerMovement::State::IDLE), _last_direction(PlayerMovement::Direction::DOWN)
    {
    }

    void PlayerAnimation::apply_state(PlayerMovement::State state, PlayerMovement::Direction direction)
    {
        if (!should_change_animation(state, direction))
            return;

        _sprite.set_horizontal_flip(direction == PlayerMovement::Direction::LEFT);

        // Animation data with individual frame counts per direction
        struct AnimData
        {
            int speed;
            int up_start;
            int up_count;
            int down_start;
            int down_count;
            int side_start;
            int side_count;
        };

        static const AnimData animations[] = {
            {12, 384, 12, 0, 12, 240, 12},  // IDLE: idle_up(row16), idle_down(row0, 12 frames), lr_idle(row10)
            {5, 408, 8, 120, 8, 264, 8},    // WALKING: move_up(row17), move_down(row5), lr_move(row11)
            {8, 432, 8, 144, 8, 288, 8},    // RUNNING: run_up(row18), run_down(row6), lr_run(row12)
            {8, 504, 8, 216, 8, 312, 6},    // ROLLING: roll_up(row21), roll_down(row9), lr_roll(row13)
            {8, 480, 7, 192, 7, 336, 4},    // SLASHING: attack_up(row20), slash_down(row8), lr_slash(row14)
            {8, 480, 7, 192, 7, 360, 5},    // ATTACKING: attack_up(row20), slash_down(row8), lr_slash2(row15)
            {10, 456, 4, 168, 4, 336, 4},   // CHOPPING: chop_up(row19), chop_down(row7), lr_slash(row14)
            {4, 24, 24, 24, 24, 24, 24},    // HEAL_BUFF: heal_buff(row1) all directions
            {4, 48, 24, 48, 24, 48, 24},    // DEFENCE_BUFF: defence_buff(row2) all directions
            {4, 72, 24, 72, 24, 72, 24},    // POWER_BUFF: power_buff(row3) all directions
            {4, 96, 24, 96, 24, 96, 24},    // ENERGY_BUFF: energy_buff(row4) all directions
            {6, 0, 13, 0, 13, 0, 13},       // HIT: use idle_down frames temporarily (row0) all directions
            {15, 528, 13, 528, 13, 528, 13} // DEAD: death(row22) all directions - much slower animation
        };

        int state_idx = static_cast<int>(state);
        constexpr int NUM_PLAYER_STATES = sizeof(animations) / sizeof(animations[0]);
        if (state_idx >= NUM_PLAYER_STATES) // Use array size for maintainability
            return;

        const auto &anim = animations[state_idx];
        int start_frame, frame_count;

        if (direction == PlayerMovement::Direction::UP)
        {
            start_frame = anim.up_start;
            frame_count = anim.up_count;
        }
        else if (direction == PlayerMovement::Direction::DOWN)
        {
            start_frame = anim.down_start;
            frame_count = anim.down_count;
        }
        else // LEFT or RIGHT
        {
            start_frame = anim.side_start;
            frame_count = anim.side_count;
        }

        // Use non-looping animation for death state, looping for all others
        if (state == PlayerMovement::State::DEAD)
        {
            make_anim_range_once(anim.speed, start_frame, start_frame + frame_count - 1);
        }
        else
        {
            make_anim_range(anim.speed, start_frame, start_frame + frame_count - 1);
        }

        _last_state = state;
        _last_direction = direction;
    }

    bool PlayerAnimation::should_change_animation(PlayerMovement::State state, PlayerMovement::Direction direction)
    {
        if (!_animation.has_value())
            return true;

        bool flip_changed = _sprite.horizontal_flip() != (direction == PlayerMovement::Direction::LEFT);
        bool state_changed = (_last_state != state);
        bool direction_changed = (_last_direction != direction);

        return flip_changed || state_changed || direction_changed;
    }

    void PlayerAnimation::make_anim_range(int speed, int start_frame, int end_frame)
    {
        bn::vector<uint16_t, 32> frames;
        for (int i = start_frame; i <= end_frame; ++i)
        {
            frames.push_back(i);
        }

        _animation = bn::sprite_animate_action<32>::forever(
            _sprite, speed, bn::sprite_items::hero_sword.tiles_item(),
            bn::span<const uint16_t>(frames.data(), frames.size()));
    }

    void PlayerAnimation::make_anim_range_once(int speed, int start_frame, int end_frame)
    {
        bn::vector<uint16_t, 32> frames;
        for (int i = start_frame; i <= end_frame; ++i)
        {
            frames.push_back(i);
        }

        _animation = bn::sprite_animate_action<32>::once(
            _sprite, speed, bn::sprite_items::hero_sword.tiles_item(),
            bn::span<const uint16_t>(frames.data(), frames.size()));
    }

    void PlayerAnimation::update()
    {
        if (_animation.has_value() && !_animation->done())
        {
            _animation->update();
        }
    }
}
