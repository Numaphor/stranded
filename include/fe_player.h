#ifndef FE_PLAYER_H
#define FE_PLAYER_H

#include "bn_sprite_ptr.h"
#include "bn_camera_ptr.h"
#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "bn_sprite_animate_actions.h"
#include "bn_vector.h"

#include "fe_hitbox.h"
#include "fe_healthbar.h"
#include "fe_entity.h"
#include "fe_bullet_manager.h"

namespace fe
{
    // PlayerMovement class (moved from fe_player_movement.h)
    class PlayerMovement
    {
    public:
        enum class State
        {
            IDLE,
            WALKING,
            HIT,
            DEAD
        };

        enum class Direction
        {
            UP,
            DOWN,
            LEFT,
            RIGHT
        };

        // Movement constants
        static constexpr bn::fixed acc_const = 0.35;
        static constexpr bn::fixed friction_const = 0.65;
        static constexpr bn::fixed movement_threshold = 0.1;
        static constexpr bn::fixed max_speed = 2;

        PlayerMovement();

        void move_right();
        void move_left();
        void move_up();
        void move_down();
        void apply_friction();
        void reset();
        void stop_movement();

        [[nodiscard]] bn::fixed dx() const { return _dx; }
        [[nodiscard]] bn::fixed dy() const { return _dy; }
        [[nodiscard]] State current_state() const { return _current_state; }
        [[nodiscard]] Direction facing_direction() const { return _facing_direction; }
        [[nodiscard]] bool is_state(State state) const { return _current_state == state; }
        [[nodiscard]] bool is_moving() const { return _dx != 0 || _dy != 0; }

        void set_dx(bn::fixed dx) { _dx = dx; }
        void set_dy(bn::fixed dy) { _dy = dy; }

        // Update movement state based on current velocity
        void update_movement_state() { update_state(); }

    private:
        bn::fixed _dx;
        bn::fixed _dy;
        State _current_state;
        Direction _facing_direction;

        void update_state();
    };

    // PlayerAnimation class (moved from fe_player_animation.h)
    class PlayerAnimation
    {
    public:
        explicit PlayerAnimation(bn::sprite_ptr sprite);
        void update();
        void apply_state(PlayerMovement::State state, PlayerMovement::Direction direction);

    private:
        bn::sprite_ptr _sprite;
        bn::optional<bn::sprite_animate_action<4>> _animation;
    };

    // PlayerState class (moved from fe_player_state.h)
    class PlayerState
    {
    private:
        bool _invulnerable = false;
        bool _listening = false;
        int _inv_timer = 0;

    public:
        [[nodiscard]] bool invulnerable() const { return _invulnerable; }
        void set_invulnerable(bool invulnerable) { _invulnerable = invulnerable; }
        [[nodiscard]] bool listening() const { return _listening; }
        void set_listening(bool listening) { _listening = listening; }
        [[nodiscard]] int inv_timer() const { return _inv_timer; }
        void set_inv_timer(int inv_timer) { _inv_timer = inv_timer; }
        void reset()
        {
            _invulnerable = false;
            _listening = false;
            _inv_timer = 0;
        }
    };

    // PlayerAbilities class (moved from fe_player_abilities.h)
    class PlayerAbilities
    {
    private:
        bool _double_jump_available = true;
        bool _dash_attack_available = true;
        int _dash_attack_cooldown = 0;

    public:
        [[nodiscard]] bool double_jump_available() const { return _double_jump_available; }
        void set_double_jump_available(bool available) { _double_jump_available = available; }
        [[nodiscard]] bool dash_attack_available() const { return _dash_attack_available; }
        void set_dash_attack_available(bool available) { _dash_attack_available = available; }
        [[nodiscard]] int dash_attack_cooldown() const { return _dash_attack_cooldown; }
        void set_dash_attack_cooldown(int cooldown) { _dash_attack_cooldown = cooldown; }
        void reset()
        {
            _double_jump_available = true;
            _dash_attack_available = true;
            _dash_attack_cooldown = 0;
        }
    };

    // Forward declaration of Enemy class
    class Enemy;

    // Main Player class (moved from fe_player.h)
    class Player : public Entity
    {
    public:
        explicit Player(bn::sprite_ptr sprite);

        void spawn(bn::fixed_point pos, bn::camera_ptr camera);
        void update();

        [[nodiscard]] bn::fixed_point pos() const override { return Entity::pos(); }
        [[nodiscard]] bool is_moving() const { return _movement.is_moving(); }
        [[nodiscard]] bool is_state(PlayerMovement::State state) const { return _movement.is_state(state); }
        [[nodiscard]] PlayerMovement::Direction facing_direction() const { return _movement.facing_direction(); }
        [[nodiscard]] bool listening() const { return _state.listening(); }
        void set_listening(bool listening) { _state.set_listening(listening); }

        void set_position(bn::fixed_point pos) override;
        void revert_position() override;
        void set_sprite_z_order(int z_order) override { Entity::set_sprite_z_order(z_order); }
        bn::sprite_ptr* sprite() { return get_sprite(); }

        [[nodiscard]] int get_hp() const { return _hp; }
        void take_damage(int damage)
        {
            if (!_state.invulnerable())
            {
                _hp -= damage;
                if (_hp <= 0)
                {
                    _hp = 0;
                    _reset_required = true;
                }
                _healthbar.set_hp(_hp);
                _healthbar.update();
                _state.set_invulnerable(true);
                _state.set_inv_timer(60); // 1 second of invulnerability at 60 FPS

                // Visual feedback for taking damage
                set_visible(false);
            }
        }

        [[nodiscard]] bool is_reset_required() const { return _reset_required; }

        // Reset player state
        void reset()
        {
            _hp = 3;
            _reset_required = false;
            _state.reset();
            _movement.reset();
            _healthbar.set_hp(_hp);
            _healthbar.update();
            set_visible(true);
            _bullet_manager.clear_bullets();
        }

        // Returns list of active bullets for collision checking
        [[nodiscard]] const bn::vector<Bullet, 32> &bullets() const { return _bullet_manager.bullets(); }

        // Hitbox for collision detection
        [[nodiscard]] Hitbox get_hitbox() const override { return Entity::get_hitbox(); }

        void update_gun_position(PlayerMovement::Direction direction);

    private:
        PlayerMovement _movement;
        PlayerAnimation _animation;
        PlayerState _state;
        int _hp = 3;
        bool _reset_required = false;
        fe::Healthbar _healthbar;

        // Gun sprite members
        bn::optional<bn::sprite_ptr> _gun_sprite;
        bool _gun_active = false;

        // Bullet management
        BulletManager _bullet_manager;

        // Strafing state
        bool _is_strafing = false;
        PlayerMovement::Direction _strafing_direction = PlayerMovement::Direction::DOWN;

        void handle_input();
        void update_physics();
        void fire_bullet(PlayerMovement::Direction direction);
        void update_bullets();
    };

    // ... other members ...
};

#endif // FE_PLAYER_H
