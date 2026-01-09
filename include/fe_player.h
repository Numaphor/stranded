#ifndef FE_PLAYER_H
#define FE_PLAYER_H

#include "bn_sprite_ptr.h"
#include "bn_camera_ptr.h"
#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "bn_sprite_animate_actions.h"
#include "bn_vector.h"

#include "fe_hitbox.h"
#include "fe_hud.h"
#include "fe_entity.h"
#include "fe_bullet_manager.h"
#include "fe_player_companion.h"
#include "fe_constants.h"

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
        // Normal responsiveness values
        static constexpr bn::fixed acc_const = 0.35;         // acceleration per frame
        static constexpr bn::fixed friction_const = 0.65;    // only applied when no input
        static constexpr bn::fixed movement_threshold = 0.1; // minimal velocity considered moving
        static constexpr bn::fixed max_speed = 1;            // max velocity per axis (halved)
        static constexpr bn::fixed diagonal_factor = 0.707;  // 1/√2 for diagonal movement normalization

        PlayerMovement();

        void move_right();
        void move_left();
        void move_up();
        void move_down();
        void move_direction(Direction dir);
        void apply_friction();
        void reset();
        void stop_movement();
        void start_action(State action, int timer);
        void stop_action();
        // New movement methods for enhanced abilities
        void start_running();
        void stop_running();
        void start_rolling();
        void start_chopping();
        void start_slashing();
        void start_attacking();
        void start_buff(State buff_type);

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
        void set_state(State state) { _current_state = state; }
        void set_facing_direction(Direction direction) { _facing_direction = direction; }

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
        PlayerMovement::State _last_state;
        PlayerMovement::Direction _last_direction;

        // Helper method to create animation ranges
        void make_anim_range(int speed, int start_frame, int end_frame);
        void make_anim_range_once(int speed, int start_frame, int end_frame);
        bool should_change_animation(PlayerMovement::State state, PlayerMovement::Direction direction);
    };

    // PlayerVFX class for visual effects overlay
    class PlayerVFX
    {
    public:
        explicit PlayerVFX();

        void initialize(bn::camera_ptr camera);
        void update(bn::fixed_point player_pos, PlayerMovement::State state, PlayerMovement::Direction direction);
        void apply_vfx_state(PlayerMovement::State state, PlayerMovement::Direction direction);
        void hide_vfx();
        [[nodiscard]] bn::sprite_ptr *vfx_sprite() { return _vfx_sprite.has_value() ? &_vfx_sprite.value() : nullptr; }

    private:
        bn::optional<bn::sprite_ptr> _vfx_sprite;
        bn::optional<bn::sprite_animate_action<32>> _vfx_animation;
        PlayerMovement::State _last_vfx_state;
        PlayerMovement::Direction _last_vfx_direction;
        bn::optional<bn::camera_ptr> _camera;

        bool should_show_vfx(PlayerMovement::State state) const;
        bool should_change_vfx(PlayerMovement::State state, PlayerMovement::Direction direction) const;
        void make_vfx_anim_range(int speed, int start_frame, int end_frame);
        void make_vfx_anim_range_once(int speed, int start_frame, int end_frame);
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
        void set_listening(bool listening);
        [[nodiscard]] int inv_timer() const { return _inv_timer; }
        void set_inv_timer(int inv_timer) { _inv_timer = inv_timer; }
        [[nodiscard]] int dialog_cooldown() const { return _dialog_cooldown; }
        void update_dialog_cooldown();
        void reset();
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
        void update_cooldowns();
        void reset();
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
        [[nodiscard]] bn::fixed velocity_x() const { return _movement.dx(); }
        [[nodiscard]] bn::fixed velocity_y() const { return _movement.dy(); }
        [[nodiscard]] bool is_running() const { return _movement.is_state(PlayerMovement::State::RUNNING); }
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
        void kill_companion()
        {
            if (_companion.has_value())
                _companion->die_independently();
        }

        [[nodiscard]] int get_hp() const { return _hp; }
        void take_damage(int damage);

        [[nodiscard]] bool is_reset_required() const { return _reset_required; }

        // Reset player state
        void reset();

        // Reset player movement state (position remains unchanged)
        void reset_movement();

        // Returns list of active bullets for collision checking
        [[nodiscard]] const bn::vector<Bullet, 32> &bullets() const { return _bullet_manager.bullets(); }
        [[nodiscard]] bn::vector<Bullet, 32> &bullets_mutable() { return const_cast<bn::vector<Bullet, 32> &>(_bullet_manager.bullets()); }

        // Check if bullet was just fired this frame (for screen shake)
        [[nodiscard]] bool bullet_just_fired() const { return _bullet_just_fired; }
        void clear_bullet_fired_flag() { _bullet_just_fired = false; }
        [[nodiscard]] bool is_firing() const;

        // Hitbox for collision detection
        [[nodiscard]] Hitbox get_hitbox() const override { return Entity::get_hitbox(); }

        // Check if player is currently performing a melee attack
        [[nodiscard]] bool is_attacking() const;

        // Access to HUD for weapon management
        [[nodiscard]] fe::HUD &get_hud() { return _hud; }

        // Access to gun sprite for zoom scaling
        [[nodiscard]] bn::sprite_ptr *gun_sprite() { return _gun_sprite.has_value() ? &_gun_sprite.value() : nullptr; }

        // Ammo management
        [[nodiscard]] int get_ammo() const { return _ammo_count; }
        void add_ammo(int amount);
        void reload_ammo();
        [[nodiscard]] bool has_ammo() const;

        void update_gun_position(PlayerMovement::Direction direction);
        [[nodiscard]] bn::sprite_ptr *vfx_sprite() { return _vfx.vfx_sprite(); }

    private:
        PlayerMovement _movement;
        PlayerAnimation _animation;
        PlayerVFX _vfx;
        PlayerState _state;
        PlayerAbilities _abilities;
        int _hp = 2;
        bool _reset_required = false;
        int _death_timer = 0;
        bool _death_sound_played = false;
        fe::HUD _hud;

        // Gun sprite members
        bn::optional<bn::sprite_ptr> _gun_sprite;
        bool _gun_active = false;
        int _gun_sprite_frame = 0;   // Track current gun sprite frame (0-5)
        int _sword_sprite_frame = 0; // Track current sword sprite frame (for future sword variants)

        // Bullet management
        BulletManager _bullet_manager;
        bool _bullet_just_fired = false; // Flag for screen shake detection

        // Ammo system
        int _ammo_count = 10;
        static constexpr int MAX_AMMO = 10;
        bool _reload_on_roll_end = false;
        int _auto_reload_timer = 0;                     // Timer for automatic reload when holding L
        static constexpr int AUTO_RELOAD_INTERVAL = 30; // Reload every 0.5 seconds (30 frames at 60fps)

        // Strafing state
        bool _is_strafing = false;
        PlayerMovement::Direction _strafing_direction = PlayerMovement::Direction::DOWN;

        // Companion
        bn::optional<PlayerCompanion> _companion;
        bool _companion_initialized = false;

        void update_animation(); // Helper to update animation state
        void fire_bullet(PlayerMovement::Direction direction);
        void update_bullets();
        void initialize_companion(bn::camera_ptr camera);
        void handle_input();
        void toggle_gun();
        void switch_weapon();      // Switch between SWORD and GUN
        void cycle_gun_sprite();   // Cycle gun animation sprites
        void cycle_sword_sprite(); // Cycle sword animation sprites (placeholder)
        void update_gun_if_active();
        void activate_buff(PlayerMovement::State buff_state); // Helper to activate a buff with proper animations
    };

    // ... other members ...
};

#endif // FE_PLAYER_H
