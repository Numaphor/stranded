#ifndef FE_PLAYER_COMPANION_H
#define FE_PLAYER_COMPANION_H

#include "bn_sprite_ptr.h"
#include "bn_camera_ptr.h"
#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "bn_sprite_animate_actions.h"
#include "bn_math.h"

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
        void set_camera(bn::camera_ptr camera);      // Set camera without affecting position
        void die_independently();                    // Companion dies independently of player
        bool try_revive(bn::fixed_point player_pos); // Try to revive if player is close enough
        [[nodiscard]] Position get_position_side() const { return _position_side; }
        [[nodiscard]] bn::fixed_point pos() const { return _position; }
        [[nodiscard]] bool is_flying() const { return _is_flying; }
        [[nodiscard]] bool is_dead_independently() const { return _independent_death; }
        [[nodiscard]] bool is_reviving() const { return _is_reviving; }
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
        static constexpr bn::fixed REVIVE_DISTANCE = 24; // Distance at which player can revive companion

        void update_animation();
        void update_position(bn::fixed_point player_pos);
        bn::fixed_point calculate_companion_offset() const;
        void start_death_animation();
    };
}

#endif // FE_PLAYER_COMPANION_H
