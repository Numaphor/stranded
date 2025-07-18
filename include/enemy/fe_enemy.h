#ifndef BF_ENEMY_H
#define BF_ENEMY_H

#include "bn_fixed_point.h"
#include "bn_sprite_ptr.h"
#include "bn_camera_ptr.h"
#include "bn_optional.h"
#include "bn_span.h"
#include "bn_string.h"
#include "bn_random.h"
#include "bn_string_view.h"
#include "bn_sprite_animate_actions.h"
#include "bn_regular_bg_ptr.h"
#include "bn_span.h"
#include "bn_regular_bg_map_cell.h"

#include "fe_enemy_type.h"
#include "fe_hitbox.h"
#include "fe_level.h"

namespace fe
{
    // Forward declarations
    class Player;

    class Enemy
    {
    public:
        enum class EnemyState
        {
            IDLE,
            WALK,
            FOLLOW
        };

        friend class World;                                            // Allow World to access private members
        friend class Minimap;                                          // Allow Minimap to access private members
        friend bool check_collisions_bb(Player &player, Enemy &enemy); // Allow collision function to access _pos

    private:
        bn::fixed_point _pos;
        Hitbox _hitbox; // Persistent hitbox for enemy
        bn::fixed _dy = 0;
        bn::fixed _dx = 0;
        EnemyState _state = EnemyState::IDLE;
        int _state_timer = 0;
        int _state_duration = 60;
        bn::fixed _target_dx = 0;
        bn::fixed _target_dy = 0;
        bn::camera_ptr _camera;
        ENEMY_TYPE _type;
        int _dir;
        int _hp;
        int _direction_timer = 0;
        bool _invulnerable = false;
        bool _dead = false;
        bool _grounded = false;
        int _inv_timer = 0;
        bool _stunned = false;

        // Knockback state
        bn::fixed _knockback_dx = 0;
        bn::fixed _knockback_dy = 0;
        int _knockback_timer = 0;
        static constexpr int KNOCKBACK_DURATION = 10; // Frames of knockback
        int _sound_timer = 0;
        bool _spotted_player = false;
        bn::optional<bn::sprite_ptr> _sprite;
        bn::optional<bn::sprite_animate_action<4>> _action;

        bn::optional<bn::sprite_animate_action<10>> _mutant_action;

        bn::fixed_point _target = bn::fixed_point(0, 0);
        bool _target_locked = false;

        bn::regular_bg_ptr _map;
        bn::span<const bn::regular_bg_map_cell> _map_cells;
        Level _level;

        bool _will_hit_wall();
        bool _will_fall();
        bool _fall_check(bn::fixed x, bn::fixed y);

    private:
        bool _take_damage(int damage);
        void _apply_knockback(bn::fixed dx, bn::fixed dy);

    public:
        Enemy(int x, int y, bn::camera_ptr camera, bn::regular_bg_ptr map, ENEMY_TYPE type, int hp);
        void update_hitbox();
        void update(bn::fixed_point player_pos, const Level &level, bool player_listening = false);
        [[nodiscard]] bn::fixed_point get_position() const
        {
            return _pos;
        }
        void set_pos(bn::fixed_point pos);
        bool is_hit(Hitbox attack);
        bool is_vulnerable();
        void set_visible(bool visibility);
        void teleport();
        bool damage_from_left(int damage);
        bool damage_from_right(int damage);
        bool spotted_player();
        int hp();
        ENEMY_TYPE type();
        [[nodiscard]] Hitbox get_hitbox() const
        {
            return _hitbox;
        }
    };
}

#endif