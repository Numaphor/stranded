#ifndef FE_PLAYER_COMPANION_H
#define FE_PLAYER_COMPANION_H

#include "bn_sprite_ptr.h"
#include "bn_camera_ptr.h"
#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"
#include "bn_math.h"
#include "fe_constants.h"

namespace fe
{
    class PlayerCompanion
    {
    public:
        enum class Position
        {
            RIGHT,
            LEFT,
            BELOW
        };

        explicit PlayerCompanion(bn::sprite_ptr sprite);
        void spawn(bn::fixed_point player_pos, bn::camera_ptr camera);
        void update(bn::fixed_point player_pos, bool player_is_dead);
        void set_visible(bool visible);
        void set_position_side(Position side);
        void update_position_side(bn::fixed_point player_pos);
        void set_z_order(int z_order);
        void set_flying(bool flying);
        void set_camera(bn::camera_ptr camera);                                   // Set camera without affecting position
        void die_independently();                                                 // Companion dies independently of player
        bool try_revive(bn::fixed_point player_pos, bool a_pressed, bool a_held); // Try to revive if player is close enough
        void cancel_revival();                                                    // Cancel current revival progress
        [[nodiscard]] bool is_revival_in_progress() const { return _revival_in_progress; }
        [[nodiscard]] int get_revival_progress() const { return _revival_timer; }
        [[nodiscard]] float get_revival_progress_percent() const { return float(_revival_timer) / float(COMPANION_REVIVAL_DURATION); }
        void show_revival_text(); // Show "Press A to revive" text
        void hide_revival_text(); // Hide revival text
        [[nodiscard]] Position get_position_side() const { return _position_side; }
        [[nodiscard]] bn::fixed_point pos() const { return _position; }
        [[nodiscard]] bool is_flying() const { return _is_flying; }
        [[nodiscard]] bool is_dead_independently() const { return _independent_death; }
        [[nodiscard]] bool is_reviving() const { return _is_reviving; }
        [[nodiscard]] bool can_be_revived() const { return _can_be_revived; }
        [[nodiscard]] bn::sprite_ptr get_sprite() const { return _sprite; }

    private:
        bn::sprite_ptr _sprite;
        bn::fixed_point _position;
        bn::optional<bn::sprite_animate_action<32>> _animation;
        Position _position_side = Position::RIGHT;
        bool _is_dead = false;
        bool _is_flying = false;
        bool _player_too_close = false;
        int _follow_delay = 0;
        bn::fixed_point _target_offset;

        // Independent death system
        bool _independent_death = false;                 // True if companion died independently (not with player)
        bn::fixed_point _death_position;                 // Position where companion died
        bool _can_be_revived = false;                    // True if companion can be revived by player proximity
        bool _is_reviving = false;                       // True if companion is currently playing revival animation
        bool _revival_in_progress = false;               // True if player is actively reviving companion
        int _revival_timer = 0;                          // Timer for revival progress (0-300 for 5 seconds)

        // Revival progress bar sprite
        bn::optional<bn::sprite_ptr> _progress_bar_sprite;

        // Revival text message
        bn::vector<bn::sprite_ptr, 16> _text_sprites;

        void update_animation();
        void update_position(bn::fixed_point player_pos);
        bn::fixed_point calculate_companion_offset() const;
        void start_death_animation();
    };
}

#endif // FE_PLAYER_COMPANION_H
