#include "fe_player.h"
#include "bn_sprite_items_hero.h"

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
            {12, 187, 12, 0, 13, 144, 12},  // IDLE: idle_up(187-198=12), idle_down(0-12=13), lr_idle(144-155=12)
            {5, 199, 8, 109, 8, 156, 8},    // WALKING: move_up(199-206=8), move_down(109-116=8), lr_move(156-163=8) - 75ms per frame
            {8, 207, 8, 117, 8, 164, 8},    // RUNNING: run_up(207-214=8), run_down(117-124=8), lr_run(164-171=8)
            {8, 226, 8, 136, 8, 172, 6},    // ROLLING: roll_up(226-233=8), roll_down(136-143=8), lr_roll(172-177=6)
            {8, 219, 7, 129, 7, 178, 4},    // SLASHING: attack_up(219-225=7), slash_down(129-135=7), lr_slash(178-181=4)
            {8, 219, 7, 129, 7, 182, 5},    // ATTACKING: attack_up(219-225=7), slash_down(129-135=7), lr_slash(182-186=5)
            {10, 215, 4, 125, 4, 178, 4},   // CHOPPING: chop_up(215-218=4), chop_down(125-128=4), lr_slash(178-181=4)
            {4, 13, 24, 13, 24, 13, 24},    // HEAL_BUFF: heal_buff(13-36=24) all directions
            {4, 37, 24, 37, 24, 37, 24},    // DEFENCE_BUFF: defence_buff(37-60=24) all directions
            {4, 61, 24, 61, 24, 61, 24},    // POWER_BUFF: power_buff(61-84=24) all directions
            {4, 85, 24, 85, 24, 85, 24},    // ENERGY_BUFF: energy_buff(85-108=24) all directions
            {6, 0, 13, 0, 13, 0, 13},       // HIT: use idle_down frames temporarily (0-12=13) all directions
            {15, 234, 13, 234, 13, 234, 13} // DEAD: death(234-246=13) all directions - much slower animation
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
            _sprite, speed, bn::sprite_items::hero.tiles_item(),
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
            _sprite, speed, bn::sprite_items::hero.tiles_item(),
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
