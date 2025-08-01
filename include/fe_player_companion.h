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
        [[nodiscard]] Position get_position_side() const { return _position_side; }
        [[nodiscard]] bn::fixed_point pos() const { return _position; }
        [[nodiscard]] bool is_flying() const { return _is_flying; }
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

        void update_animation();
        void update_position(bn::fixed_point player_pos);
        bn::fixed_point calculate_companion_offset() const;
        void start_death_animation();
    };
}

#endif // FE_PLAYER_COMPANION_H
