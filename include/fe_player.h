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
            RUNNING,
            ROLLING,
            CHOPPING,
            SLASHING,
            ATTACKING,
            HEAL_BUFF,
            DEFENCE_BUFF,
            POWER_BUFF,
            ENERGY_BUFF,
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

        // New movement methods for enhanced abilities
        void start_running();
        void stop_running();
        void start_rolling();
        void start_chopping();
        void start_slashing();
        void start_attacking();
        void start_buff(State buff_type);
        void stop_action(); // Stop any special action and return to normal movement

        [[nodiscard]] bn::fixed dx() const { return _dx; }
        [[nodiscard]] bn::fixed dy() const { return _dy; }
        [[nodiscard]] State current_state() const { return _current_state; }
        [[nodiscard]] Direction facing_direction() const { return _facing_direction; }
        [[nodiscard]] bool is_state(State state) const { return _current_state == state; }
        [[nodiscard]] bool is_moving() const { return _dx != 0 || _dy != 0; }
        [[nodiscard]] bool is_performing_action() const
        {
            return _current_state == State::ROLLING || _current_state == State::CHOPPING ||
                   _current_state == State::SLASHING || _current_state == State::ATTACKING ||
                   _current_state == State::HEAL_BUFF || _current_state == State::DEFENCE_BUFF ||
                   _current_state == State::POWER_BUFF || _current_state == State::ENERGY_BUFF;
        }
        [[nodiscard]] int action_timer() const { return _action_timer; }

        void set_dx(bn::fixed dx) { _dx = dx; }
        void set_dy(bn::fixed dy) { _dy = dy; }
        void set_action_timer(int timer) { _action_timer = timer; }

        // Update movement state based on current velocity
        void update_movement_state() { update_state(); }

        // Update action timer
        void update_action_timer()
        {
            if (_action_timer > 0)
                _action_timer--;
        }

    private:
        bn::fixed _dx;
        bn::fixed _dy;
        State _current_state;
        Direction _facing_direction;
        int _action_timer = 0; // Timer for actions like rolling, chopping, etc.

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
        bn::optional<bn::sprite_animate_action<32>> _animation;

        // Helper method to create animation ranges
        void make_anim_range(int speed, int start_frame, int end_frame);
    };

    // PlayerState class (moved from fe_player_state.h)
    class PlayerState
    {
    private:
        bool _invulnerable = false;
        bool _listening = false;
        int _inv_timer = 0;
        int _dialog_cooldown = 0;

    public:
        [[nodiscard]] bool invulnerable() const { return _invulnerable; }
        void set_invulnerable(bool invulnerable) { _invulnerable = invulnerable; }
        [[nodiscard]] bool listening() const { return _listening; }
        void set_listening(bool listening)
        {
            if (_listening && !listening)
            {
                // Dialog just ended, set cooldown
                _dialog_cooldown = 10; // 10 frame cooldown
            }
            _listening = listening;
        }
        [[nodiscard]] int inv_timer() const { return _inv_timer; }
        void set_inv_timer(int inv_timer) { _inv_timer = inv_timer; }
        [[nodiscard]] int dialog_cooldown() const { return _dialog_cooldown; }
        void update_dialog_cooldown()
        {
            if (_dialog_cooldown > 0)
                _dialog_cooldown--;
        }
        void reset()
        {
            _invulnerable = false;
            _listening = false;
            _inv_timer = 0;
            _dialog_cooldown = 0;
        }
    };

    // PlayerAbilities class (moved from fe_player_abilities.h)
    class PlayerAbilities
    {
    private:
        // New abilities for hero sprite
        bool _running_available = true;
        bool _rolling_available = true;
        bool _chopping_available = true;
        bool _slashing_available = true;
        bool _buff_abilities_available = true;

        // Cooldowns for new abilities
        int _roll_cooldown = 0;
        int _chop_cooldown = 0;
        int _slash_cooldown = 0;
        int _buff_cooldown = 0;

    public:
        // New ability getters/setters
        [[nodiscard]] bool running_available() const { return _running_available; }
        void set_running_available(bool available) { _running_available = available; }
        [[nodiscard]] bool rolling_available() const { return _rolling_available && _roll_cooldown <= 0; }
        void set_rolling_available(bool available) { _rolling_available = available; }
        [[nodiscard]] bool chopping_available() const { return _chopping_available && _chop_cooldown <= 0; }
        void set_chopping_available(bool available) { _chopping_available = available; }
        [[nodiscard]] bool slashing_available() const { return _slashing_available && _slash_cooldown <= 0; }
        void set_slashing_available(bool available) { _slashing_available = available; }
        [[nodiscard]] bool buff_abilities_available() const { return _buff_abilities_available && _buff_cooldown <= 0; }
        void set_buff_abilities_available(bool available) { _buff_abilities_available = available; }

        // Cooldown management
        [[nodiscard]] int roll_cooldown() const { return _roll_cooldown; }
        void set_roll_cooldown(int cooldown) { _roll_cooldown = cooldown; }
        [[nodiscard]] int chop_cooldown() const { return _chop_cooldown; }
        void set_chop_cooldown(int cooldown) { _chop_cooldown = cooldown; }
        [[nodiscard]] int slash_cooldown() const { return _slash_cooldown; }
        void set_slash_cooldown(int cooldown) { _slash_cooldown = cooldown; }
        [[nodiscard]] int buff_cooldown() const { return _buff_cooldown; }
        void set_buff_cooldown(int cooldown) { _buff_cooldown = cooldown; }

        // Update cooldowns
        void update_cooldowns()
        {
            if (_roll_cooldown > 0)
                _roll_cooldown--;
            if (_chop_cooldown > 0)
                _chop_cooldown--;
            if (_slash_cooldown > 0)
                _slash_cooldown--;
            if (_buff_cooldown > 0)
                _buff_cooldown--;
        }

        void reset()
        {
            _running_available = true;
            _rolling_available = true;
            _chopping_available = true;
            _slashing_available = true;
            _buff_abilities_available = true;
            _roll_cooldown = 0;
            _chop_cooldown = 0;
            _slash_cooldown = 0;
            _buff_cooldown = 0;
        }
    };

    // Companion class for player companion
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
        void set_z_order(int z_order);
        [[nodiscard]] Position get_position_side() const { return _position_side; }
        [[nodiscard]] bn::fixed_point pos() const { return _position; }

    private:
        bn::sprite_ptr _sprite;
        bn::fixed_point _position;
        bn::optional<bn::sprite_animate_action<32>> _animation;
        Position _position_side = Position::RIGHT;
        bool _is_dead = false;
        int _follow_delay = 0;
        bn::fixed_point _target_offset;

        void update_animation();
        void update_position(bn::fixed_point player_pos);
        bn::fixed_point calculate_companion_offset() const;
        void start_death_animation();
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
        void update_sprite_position() override;
        void update_z_order();
        void revert_position() override;
        void set_sprite_z_order(int z_order) override;
        bn::sprite_ptr *sprite() { return get_sprite(); }

        // Companion accessors
        [[nodiscard]] bool has_companion() const { return _companion.has_value(); }
        [[nodiscard]] PlayerCompanion *get_companion() { return _companion.has_value() ? &(*_companion) : nullptr; }

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
            _abilities.reset();
            _healthbar.set_hp(_hp);
            _healthbar.update();
            set_visible(true);
            _bullet_manager.clear_bullets();

            // Reset companion if it exists
            if (_companion.has_value())
            {
                _companion->set_visible(true);
            }
        }

        // Reset player movement state (position remains unchanged)
        void reset_movement()
        {
            _movement.reset();
        }

        // Returns list of active bullets for collision checking
        [[nodiscard]] const bn::vector<Bullet, 32> &bullets() const { return _bullet_manager.bullets(); }

        // Hitbox for collision detection
        [[nodiscard]] Hitbox get_hitbox() const override { return Entity::get_hitbox(); }

        // Check if player is currently performing a melee attack
        [[nodiscard]] bool is_attacking() const
        {
            return _movement.current_state() == PlayerMovement::State::CHOPPING ||
                   _movement.current_state() == PlayerMovement::State::SLASHING ||
                   _movement.current_state() == PlayerMovement::State::ATTACKING;
        }

        // Get extended attack hitbox when performing melee attacks
        [[nodiscard]] Hitbox get_attack_hitbox() const;

        void update_gun_position(PlayerMovement::Direction direction);

    private:
        PlayerMovement _movement;
        PlayerAnimation _animation;
        PlayerState _state;
        PlayerAbilities _abilities;
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

        // Companion
        bn::optional<PlayerCompanion> _companion;
        bool _companion_initialized = false;

        void handle_input();
        void update_physics();
        void update_animation(); // Helper to update animation state
        void fire_bullet(PlayerMovement::Direction direction);
        void update_bullets();
        void initialize_companion(bn::camera_ptr camera);
    };

    // ... other members ...
};

#endif // FE_PLAYER_H
