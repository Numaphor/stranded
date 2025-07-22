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
#include "fe_entity.h"
#include "fe_movement.h"
#include "fe_enemy_state_machine.h"

namespace fe
{
    // Forward declarations
    class Player;
    class IdleState;
    class PatrolState;
    class ChaseState;
    class AttackState;
    class ReturnToPostState;
    class StunnedState;

    class Enemy : public Entity
    {
    public:

        friend class World;                                            // Allow World to access private members
        friend class Minimap;                                          // Allow Minimap to access private members
        friend bool check_collisions_bb(Player &player, Enemy &enemy); // Allow collision function to access _pos
        // Friend classes for new state machine
        friend class IdleState;
        friend class PatrolState;
        friend class ChaseState;
        friend class AttackState;
        friend class ReturnToPostState;
        friend class StunnedState;

    private:
        EnemyMovement _movement;
        EnemyStateMachine _state_machine;      // New state machine
        int _state_timer = 0;
        int _state_duration = 60;
        bn::fixed _target_dx = 0;
        bn::fixed _target_dy = 0;
        bn::fixed _dx = 0;
        bn::fixed _dy = 0;
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
        
        // Death animation timer
        int _death_timer = 0;
        static constexpr int DEATH_ANIMATION_DURATION = 150; // Frames to allow death animation to complete

        // Knockback state
        bn::fixed _knockback_dx = 0;
        bn::fixed _knockback_dy = 0;
        int _knockback_timer = 0;
        static constexpr int KNOCKBACK_DURATION = 10; // Frames of knockback
        int _sound_timer = 0;
        bool _spotted_player = false;
        bn::optional<bn::sprite_animate_action<16>> _action; // Increased to handle dead animation (16 frames)
        
        // Spearguard animation states
        enum class AnimationState {
            IDLE,
            RUN,
            ATTACK,
            DEAD
        };
        AnimationState _current_animation = AnimationState::IDLE;
        int _attack_timer = 0;
        static constexpr int ATTACK_DURATION = 60; // Frames for attack animation
        
        // Spearguard return-to-post behavior
        bn::fixed_point _original_position = bn::fixed_point(0, 0);
        bool _returning_to_post = false;
        static constexpr bn::fixed RETURN_THRESHOLD = 8; // Distance threshold to consider "at post"

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
        
        // Move constructor and assignment operator (needed for EnemyStateMachine)
        Enemy(Enemy&& other) noexcept;
        Enemy& operator=(Enemy&& other) noexcept;
        
        // Delete copy constructor and assignment operator
        Enemy(const Enemy&) = delete;
        Enemy& operator=(const Enemy&) = delete;
        void update_hitbox();
        void update() override { /* Default implementation */ }
        void update(bn::fixed_point player_pos, const Level &level, bool player_listening = false);
        [[nodiscard]] bn::fixed_point get_position() const
        {
            return pos();
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
        bool is_ready_for_removal(); // Check if dead enemy should be removed
        void _update_spearguard_animation();
        [[nodiscard]] Hitbox get_hitbox() const override
        {
            return Entity::get_hitbox();
        }
    };
}

#endif
