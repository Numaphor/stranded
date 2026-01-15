#include "fe_player.h"
#include "fe_constants.h"
#include "fe_player_companion.h"
#include "bn_keypad.h"
#include "bn_sprite_items_hero_sword.h"
#include "bn_sprite_animate_actions.h"
#include "fe_level.h"
#include "fe_collision.h"
#include "fe_bullet_manager.h"
#include "bn_sound_items.h"
#include "bn_log.h"

namespace fe
{
    extern Level *_level;

    // Direction utility functions
    namespace direction_utils
    {
        bn::fixed_point get_roll_offset(PlayerMovement::Direction dir, int frames_remaining, int total_frames);
        int get_gun_z_offset(PlayerMovement::Direction dir);
    }

    // Player Implementation
    Player::Player(bn::sprite_ptr sprite) : Entity(sprite), _animation(sprite), _gun_active(false)
    {
        // Set player bg_priority to 1 to allow going behind sword backgrounds (priority 0)
        // Both player and companion need same bg_priority for z_order comparison to work
        if (auto player_sprite = get_sprite())
        {
            player_sprite->set_bg_priority(1);
        }
        set_sprite_z_order(1);
        _hitbox = Hitbox(0, 0, PLAYER_HITBOX_WIDTH, PLAYER_HITBOX_HEIGHT);
        _hud.set_hp(_hp);
        _hud.set_ammo(_ammo_count); // Initialize ammo display

        // Don't create gun sprite initially - player starts with sword equipped
    }

    void Player::spawn(bn::fixed_point pos, bn::camera_ptr camera)
    {
        _hud.set_hp(_hp);
        _hud.set_ammo(_ammo_count); // Initialize ammo display
        set_position(pos);
        set_camera(camera);
        initialize_companion(camera);
        _vfx.initialize(camera);
        update_animation();
    }

    void Player::update()
    {
        auto old_state = _movement.current_state();
        auto old_direction = _movement.facing_direction();
        bool was_performing_action = _movement.is_performing_action();

        // Update timers
        _abilities.update_cooldowns();
        _state.update_dialog_cooldown();
        _movement.update_action_timer();

        // Only process input and movement when NOT listening to NPCs
        if (!_state.listening())
        {
            handle_input();

            // Update physics
            bn::fixed_point new_pos = pos() + bn::fixed_point(_movement.dx(), _movement.dy());
            if (_movement.current_state() == PlayerMovement::State::ROLLING)
            {
                new_pos += direction_utils::get_roll_offset(_movement.facing_direction(),
                                                            _movement.action_timer(),
                                                            PLAYER_ROLL_DURATION);
            }
            set_position(new_pos);

            // Collision handled by main game loop for more accurate checking
            // Movement system collision disabled to prevent double collision zones
        }
        else
        {
            // While listening, ensure all movement is stopped
            _movement.stop_movement();
        }

        // Handle action completion
        if (was_performing_action && _movement.action_timer() <= 0)
        {
            // End roll invulnerability when roll ends
            if (_movement.current_state() == PlayerMovement::State::ROLLING && _state.invulnerable())
            {
                _state.set_invulnerable(false);
                set_visible(true);

                // Reload weapon at end of roll if flagged
                if (_reload_on_roll_end)
                {
                    reload_ammo();
                    _hud.set_ammo(_ammo_count);
                    _reload_on_roll_end = false;
                }
            }

            _movement.stop_action();
            update_animation();
        }

        // Update animation if changed
        if (old_state != _movement.current_state() || old_direction != _movement.facing_direction())
        {
            update_animation();
        }

        // Update animation frame
        _animation.update();

        // Update HUD (for soul animations)
        _hud.update();

        // Update bullets
        update_bullets();

        // Handle invulnerability blinking
        if (_state.invulnerable() && _state.inv_timer() > 0)
        {
            _state.set_inv_timer(_state.inv_timer() - 1);
            if (_state.inv_timer() % 10 == 0)
            {
                set_visible(!get_sprite()->visible());
            }
            if (_state.inv_timer() == 0)
            {
                _state.set_invulnerable(false);
                set_visible(true);
            }
        }

        // Handle death animation - wait for soul animation to complete
        if (_movement.current_state() == PlayerMovement::State::DEAD)
        {
            // Play death sound at start of death (instead of halfway through timer)
            if (!_death_sound_played)
            {
                bn::sound_items::death.play();
                _death_sound_played = true;
            }
            
            // Check if soul animation is complete before allowing respawn
            if (_hud.is_soul_animation_complete())
            {
                _reset_required = true;
            }
        }

        // Update companion
        if (_companion.has_value())
        {
            _companion->update(pos(), _movement.current_state() == PlayerMovement::State::DEAD);

            // Check for companion revival
            if (_companion->is_dead_independently())
            {
                _companion->try_revive(pos(), bn::keypad::a_pressed(), bn::keypad::a_held());
            }

            // If companion died independently or is reviving, it should always be visible (even during player invulnerability)
            if (_companion->is_dead_independently() || _companion->is_reviving())
            {
                _companion->set_visible(true);
            }
        }

        // Update VFX
        _vfx.update(pos(), _movement.current_state(), _movement.facing_direction());

        // Update z-order for depth sorting
        update_z_order();
    }

    void Player::set_position(bn::fixed_point new_pos)
    {
        Entity::set_position(new_pos);

        bn::fixed_point hitbox_pos = Hitbox::calculate_centered_position(new_pos,
                                                                         PLAYER_HITBOX_WIDTH, PLAYER_HITBOX_HEIGHT);
        _hitbox.set_x(hitbox_pos.x());
        _hitbox.set_y(hitbox_pos.y());

        update_sprite_position();
    }

    void Player::update_sprite_position()
    {
        if (auto sprite = get_sprite())
        {
            bn::fixed_point pos = Entity::pos();
            // Offset sprite to align 32x32 sprite with 16px hitbox
            sprite->set_position(pos.x(), pos.y() + PLAYER_SPRITE_Y_OFFSET);
        }
    }

    void Player::revert_position()
    {
        set_position(_previous_pos);
    }

    void Player::set_sprite_z_order(int z_order)
    {
        if (auto sprite = get_sprite())
        {
            sprite->set_z_order(z_order);
        }
    }

    void Player::update_z_order()
    {
        int z_order = -pos().y().integer();
        set_sprite_z_order(z_order);

        if (_gun_sprite.has_value())
        {
            PlayerMovement::Direction gun_dir = _is_strafing ? _strafing_direction : _movement.facing_direction();
            int gun_z_offset = direction_utils::get_gun_z_offset(gun_dir);
            _gun_sprite->set_z_order(z_order + gun_z_offset);
        }

        if (_companion.has_value())
        {
            // Dynamic z-order based on player position relative to companion
            bn::fixed_point companion_pos = _companion->pos();
            bn::fixed player_y = pos().y();
            bn::fixed companion_y = companion_pos.y();

            // If player is 8+ pixels below companion, player should appear on top
            if (player_y >= companion_y + 8)
            {
                // Player appears on top (companion behind)
                _companion->set_z_order(z_order + 10);
            }
            else
            {
                // Companion appears on top (player behind)
                _companion->set_z_order(z_order - 10);
            }
        }
    }

    void Player::update_animation()
    {
        _animation.apply_state(_movement.current_state(), _movement.facing_direction());
    }

    void Player::take_damage(int damage)
    {
        if (!_state.invulnerable() && _hp > 0)
        {
            _hp -= damage;
            if (_hp <= 0)
            {
                _hp = 0;
                // Start death animation instead of immediately requiring reset
                _movement.set_state(PlayerMovement::State::DEAD);
                _movement.stop_movement(); // Stop all momentum
                _death_timer = PLAYER_DEATH_ANIMATION_DURATION;
                _death_sound_played = false; // Reset flag for death sound

                // Clear invulnerability during death to prevent blinking
                _state.set_invulnerable(false);
                _state.set_inv_timer(0);

                // Trigger death animation
                update_animation();
            }
            else
            {
                // Only set invulnerability if not dead
                _state.set_invulnerable(true);
                _state.set_inv_timer(60); // 1 second of invulnerability at 60 FPS

                // Visual feedback for taking damage (but not for death)
                set_visible(false);

                // Health transition animations are now handled by HUD::set_hp()
            }
            _hud.set_hp(_hp);
        }
    }

    void Player::heal(int amount)
    {
        if (_hp < 3 && _hp > 0)
        {
            _hp = bn::min(_hp + amount, 3);
            _hud.set_hp(_hp);

            // Health transition animations are now handled by HUD::set_hp()
            // Soul shield activation for reaching full health is handled by the new animation system

            _hud.update();
        }
    }

    void Player::reset()
    {
        _hp = 3;
        _reset_required = false;
        _death_timer = 0;
        _death_sound_played = false;
        _state.reset();
        _movement.reset();
        _abilities.reset();
        
        // Set resetting flag to prevent soul animations during reset
        _hud.set_resetting_health(true);
        _hud.set_hp(_hp);
        _hud.set_resetting_health(false);
        
        _hud.update();
        set_visible(true);
        _bullet_manager.clear_bullets();

        // Reset ammo to full
        _ammo_count = MAX_AMMO;
        _hud.set_ammo(_ammo_count);

        // Don't auto-revive companion if it died independently
        // It should stay dead until player comes close to revive it
        if (_companion.has_value() && !_companion->is_dead_independently())
        {
            _companion->set_visible(true);
        }
    }

    void Player::reset_movement()
    {
        _movement.reset();
    }

    void Player::add_ammo(int amount)
    {
        _ammo_count = bn::min(_ammo_count + amount, MAX_AMMO);
        _hud.set_ammo(_ammo_count);
    }

    void Player::reload_ammo()
    {
        _ammo_count = MAX_AMMO;
        _hud.set_ammo(_ammo_count);
    }

    bool Player::has_ammo() const
    {
        return _ammo_count > 0;
    }

    bool Player::is_attacking() const
    {
        return _movement.current_state() == PlayerMovement::State::CHOPPING ||
               _movement.current_state() == PlayerMovement::State::SLASHING ||
               _movement.current_state() == PlayerMovement::State::ATTACKING;
    }

    bool Player::can_start_attack() const
    {
        // Can only start new attack if not currently attacking and not in any other action
        return !is_attacking() && !_movement.is_performing_action();
    }

    Hitbox Player::get_melee_hitbox() const
    {
        if (!is_attacking())
        {
            // Return empty hitbox when not attacking
            return Hitbox(0, 0, 0, 0);
        }

        bn::fixed_point attack_pos = pos();
        PlayerMovement::Direction dir = _movement.facing_direction();
        
        // Melee attack range and size based on attack type
        bn::fixed range;
        if (_movement.is_state(PlayerMovement::State::SLASHING))
        {
            range = 24 * 1.1; // 10% increase for slash: 24 → 26.4
        }
        else if (_movement.is_state(PlayerMovement::State::CHOPPING))
        {
            range = 24 * 1.2; // 20% increase for chop: 24 → 28.8
        }
        else
        {
            range = 24; // Default range for other attacks
        }
        bn::fixed width = 32;  // Attack width
        bn::fixed height = 16; // Attack height
        
        bn::fixed hitbox_x = attack_pos.x();
        bn::fixed hitbox_y = attack_pos.y() + PLAYER_SPRITE_Y_OFFSET; // Align with sprite
        
        // Adjust position based on direction
        switch (dir)
        {
        case PlayerMovement::Direction::UP:
            hitbox_y -= range;
            hitbox_x -= width / 2;
            break;
        case PlayerMovement::Direction::DOWN:
            hitbox_y += range;
            hitbox_x -= width / 2;
            break;
        case PlayerMovement::Direction::LEFT:
            hitbox_x -= range;
            hitbox_y -= height / 2;
            break;
        case PlayerMovement::Direction::RIGHT:
            hitbox_x += range;
            hitbox_y -= height / 2;
            break;
        default:
            break;
        }
        
        return Hitbox(hitbox_x, hitbox_y, width, height);
    }

    // Direction utility function implementations
    namespace direction_utils
    {
        bn::fixed_point get_roll_offset(PlayerMovement::Direction dir, int frames_remaining, int total_frames)
        {
            // Use linear momentum decay for smoother animation sync
            bn::fixed momentum_factor = bn::fixed(frames_remaining) / bn::fixed(total_frames);
            // Start fast, end slower but not too dramatic
            momentum_factor = (momentum_factor * 0.7) + 0.3; // Range from 1.0 to 0.3
            bn::fixed current_speed = PLAYER_ROLL_SPEED * momentum_factor;

            switch (dir)
            {
            case PlayerMovement::Direction::UP:
                return bn::fixed_point(0, -current_speed);
            case PlayerMovement::Direction::DOWN:
                return bn::fixed_point(0, current_speed);
            case PlayerMovement::Direction::LEFT:
                return bn::fixed_point(-current_speed, 0);
            case PlayerMovement::Direction::RIGHT:
                return bn::fixed_point(current_speed, 0);
            default:
                return bn::fixed_point(0, 0);
            }
        }

        int get_gun_z_offset(PlayerMovement::Direction dir)
        {
            // Z-order offsets for gun sprite relative to player
            // Lower z-order = drawn on top (in front)
            // UP: gun behind player, DOWN: gun in front of player
            switch (dir)
            {
            case PlayerMovement::Direction::UP:
                return 5; // gun behind player
            case PlayerMovement::Direction::DOWN:
                return -5; // gun in front of player
            case PlayerMovement::Direction::LEFT:
            case PlayerMovement::Direction::RIGHT:
                return 0;
            default:
                return 0;
            }
        }
    }
}
#include "fe_player.h"

namespace fe
{
    // PlayerAbilities Implementation
    void PlayerAbilities::update_cooldowns()
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

    void PlayerAbilities::reset()
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
}
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

        // Use non-looping animation for death, rolling, and attack states, looping for all others
        if (state == PlayerMovement::State::DEAD || 
            state == PlayerMovement::State::ROLLING ||
            state == PlayerMovement::State::SLASHING ||
            state == PlayerMovement::State::ATTACKING ||
            state == PlayerMovement::State::CHOPPING)
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
#include "fe_player.h"
#include "fe_constants.h"
#include "fe_direction_utils.h"
#include "bn_sprite_items_companion.h"
#include "bn_keypad.h"

namespace fe
{
    // Gun setup function implementation
    namespace direction_utils
    {
        void setup_gun(bn::sprite_ptr &gun_sprite, Direction dir, bn::fixed_point pos)
        {
            const int idx = int(dir);
            gun_sprite.set_horizontal_flip(player_constants::GUN_FLIPS[idx]);
            gun_sprite.set_rotation_angle(player_constants::GUN_ANGLES[idx]);
            gun_sprite.set_position(
                pos.x() + player_constants::GUN_OFFSET_X[idx],
                pos.y() + player_constants::GUN_OFFSET_Y[idx] + PLAYER_SPRITE_Y_OFFSET);
        }
    }

    // Player combat methods
    void Player::update_gun_position(PlayerMovement::Direction direction)
    {
        if (!_gun_sprite)
            return;

        Direction dir = static_cast<Direction>(int(direction));
        direction_utils::setup_gun(*_gun_sprite, dir, pos());
    }

    void Player::fire_bullet(PlayerMovement::Direction direction)
    {
        if (!_gun_active || !_gun_sprite.has_value() || !has_ammo())
            return;

        // Only fire and consume ammo if the bullet manager can actually fire
        if (!_bullet_manager.can_fire())
            return;

        Direction bullet_dir = static_cast<Direction>(int(direction));
        bn::fixed_point bullet_pos = direction_utils::get_bullet_position(bullet_dir, pos());
        _bullet_manager.fire_bullet(bullet_pos, bullet_dir);

        // Consume ammo and update HUD only after successful bullet firing
        _ammo_count--;
        _hud.set_ammo(_ammo_count);

        // Set flag for screen shake
        _bullet_just_fired = true;
    }

    void Player::update_bullets()
    {
        _bullet_manager.update_bullets();
    }

    bool Player::is_firing() const
    {
        // Player is firing if A button is held, gun is active, and dialog cooldown is 0
        return bn::keypad::a_held() && _gun_active && _state.dialog_cooldown() == 0;
    }

    void Player::initialize_companion(bn::camera_ptr camera)
    {
        if (_companion_initialized)
        {
            // If companion is already initialized but died independently,
            // just update its camera without respawning
            if (_companion.has_value() && _companion->is_dead_independently())
            {
                _companion->set_camera(camera);
            }
            return;
        }

        bn::sprite_ptr companion_sprite = bn::sprite_items::companion.create_sprite(pos());

        // Set companion bg_priority to 0 to ensure it NEVER goes behind the sword background
        // The sword background uses priorities 0-2, so companion at priority 0
        // will always be visible (sprites cover backgrounds of same priority)
        companion_sprite.set_bg_priority(0);

        _companion = PlayerCompanion(bn::move(companion_sprite));
        _companion->spawn(pos(), camera);
        _companion->set_flying(true);
        _companion_initialized = true;
    }
}
#include "fe_player.h"
#include "fe_constants.h"
#include "fe_level.h"
#include "fe_direction_utils.h"
#include "bn_keypad.h"
#include "bn_sound_items.h"
#include "bn_log.h"
#include "bn_sprite_items_gun.h"

namespace fe
{
    extern Level *_level;

    // Shared weapon state variables
    namespace
    {
        static int shared_gun_frame = 0;
        static int shared_sword_frame = 0;
    }

    // Player input methods
    void Player::handle_input()
    {
        if (_state.listening() || _movement.current_state() == PlayerMovement::State::DEAD)
        {
            return;
        }

        // Check if we're currently reviving companion - if so, limit input to just the revival process
        bool reviving_companion = _companion.has_value() && _companion->is_revival_in_progress();

        bool performing_action = _movement.is_performing_action();

        // R button: tap to switch weapon, hold to reload
        // Track how long R is held to distinguish tap from hold
        if (bn::keypad::r_held())
        {
            _r_hold_frames++;

            // Auto-reload when holding R past the switch window (gun must be active)
            if (_r_hold_frames > WEAPON_SWITCH_WINDOW && _gun_active && !reviving_companion && !_hud.is_buff_menu_open())
            {
                // Start timer if it's not already running
                if (_auto_reload_timer == 0)
                {
                    _auto_reload_timer = AUTO_RELOAD_INTERVAL;
                }

                _auto_reload_timer--;
                if (_auto_reload_timer <= 0 && _ammo_count < MAX_AMMO)
                {
                    _ammo_count++;
                    _hud.set_ammo(_ammo_count);
                    _auto_reload_timer = AUTO_RELOAD_INTERVAL; // Reset timer for next reload
                }
            }
        }
        else
        {
            // R released - check if it was a quick tap (weapon switch)
            if (_r_hold_frames > 0 && _r_hold_frames <= WEAPON_SWITCH_WINDOW && !performing_action && !reviving_companion)
            {
                switch_weapon();
            }
            _r_hold_frames = 0;
        }

        // Gun sprite cycling with SELECT + B (only when gun is active)
        if (bn::keypad::select_held() && bn::keypad::b_pressed() && _gun_active && !reviving_companion)
        {
            cycle_gun_sprite();
        }

        // Sword sprite cycling with SELECT + B (only when sword is active and gun is not active)
        if (bn::keypad::select_held() && bn::keypad::b_pressed() && !_gun_active && _hud.get_weapon() == WEAPON_TYPE::SWORD && !reviving_companion)
        {
            cycle_sword_sprite();
        }

        // Cancel roll if opposite direction key is pressed while rolling
        if (_movement.current_state() == PlayerMovement::State::ROLLING)
        {
            bool should_cancel = false;
            PlayerMovement::Direction roll_dir = _movement.facing_direction();

            switch (roll_dir)
            {
            case PlayerMovement::Direction::RIGHT:
                should_cancel = bn::keypad::left_pressed();
                break;
            case PlayerMovement::Direction::LEFT:
                should_cancel = bn::keypad::right_pressed();
                break;
            case PlayerMovement::Direction::UP:
                should_cancel = bn::keypad::down_pressed();
                break;
            case PlayerMovement::Direction::DOWN:
                should_cancel = bn::keypad::up_pressed();
                break;
            }

            if (should_cancel)
            {
                _movement.stop_action();
                _state.set_invulnerable(false);
            }
        }

        // Action inputs (consolidated) - disabled while reviving companion or when buff menu is open
        if (!performing_action && !reviving_companion && !_hud.is_buff_menu_open())
        {
            if (bn::keypad::b_pressed() && !bn::keypad::select_held() && _abilities.rolling_available())
            {
                _movement.start_action(PlayerMovement::State::ROLLING, PLAYER_ROLL_DURATION);
                _abilities.set_roll_cooldown(90); // Reasonable cooldown - 1.5 seconds

                // Set invulnerability but don't start the blinking timer during roll
                _state.set_invulnerable(true);
                _state.set_inv_timer(0); // No blinking during roll

                // Mark for reload at end of roll if gun is active
                _reload_on_roll_end = _gun_active;

                // Play roll sound effect
                bn::sound_items::swipe.play();
            }
            // Gun slot 0 uses autofire (A held), all other guns use single shot (A pressed)
            else if (bn::keypad::a_held() && _state.dialog_cooldown() == 0 && _gun_active && shared_gun_frame == 0)
            {
                // Autofire for gun slot 0 when A is held
                fire_bullet(_is_strafing ? _strafing_direction : _movement.facing_direction());
            }
            else if (bn::keypad::a_pressed() && _state.dialog_cooldown() == 0)
            {
                if (_gun_active)
                {
                    // Only fire on press for guns 1-5 (single shot)
                    if (shared_gun_frame != 0)
                    {
                        fire_bullet(_is_strafing ? _strafing_direction : _movement.facing_direction());
                    }
                }
                else if ((_combo_ready && _abilities.chopping_available() && can_start_attack()) || (!_combo_ready && _abilities.slashing_available() && can_start_attack()))
                {
                    // Check if we should do chop attack (combo)
                    if (_combo_ready && (_frame_counter - _last_attack_time) <= COMBO_WINDOW)
                    {
                        // Perform CHOPPING attack (second hit in combo)
                        _movement.start_action(PlayerMovement::State::CHOPPING, PLAYER_CHOP_DURATION);
                        _abilities.set_chop_cooldown(30);
                        _combo_ready = false; // Reset combo
                    }
                    else
                    {
                        // Perform SLASHING attack (first hit in combo)
                        _movement.start_action(PlayerMovement::State::SLASHING, PLAYER_SLASH_DURATION);
                        _abilities.set_slash_cooldown(30);
                        _last_attack_time = _frame_counter; // Record attack time
                        _combo_ready = true; // Next attack should be chop
                    }
                }
            }
            else if (bn::keypad::select_held() && _abilities.buff_abilities_available())
            {
                // Legacy buff inputs with SELECT + D-Pad (remains fully functional alongside new menu)
                PlayerMovement::State buff_state = PlayerMovement::State::IDLE;
                if (bn::keypad::up_pressed())
                    buff_state = PlayerMovement::State::HEAL_BUFF;
                else if (bn::keypad::down_pressed())
                    buff_state = PlayerMovement::State::DEFENCE_BUFF;
                else if (bn::keypad::left_pressed())
                    buff_state = PlayerMovement::State::POWER_BUFF;
                else if (bn::keypad::right_pressed())
                    buff_state = PlayerMovement::State::ENERGY_BUFF;

                if (buff_state != PlayerMovement::State::IDLE)
                {
                    activate_buff(buff_state);
                }
            }
        }

        // Update buff menu cooldown animation
        _hud.update_buff_menu_cooldown();

        // Buff menu system (new approach with L button hold for 2 seconds)
        // Cannot open menu while on cooldown
        if (!performing_action && !reviving_companion && _abilities.buff_abilities_available() && !_hud.is_buff_menu_on_cooldown())
        {
            // Hold L button to open menu (when not holding SELECT to avoid weapon switch conflict)
            if (!bn::keypad::select_held())
            {
                if (!_hud.is_buff_menu_open())
                {
                    // Not in menu - handle hold-to-open
                    if (bn::keypad::l_pressed())
                    {
                        // Start hold timer
                        _hud.start_buff_menu_hold();
                    }
                    else if (bn::keypad::l_held() && _hud.is_buff_menu_holding())
                    {
                        // Continue holding - update animation
                        _hud.update_buff_menu_hold();

                        // Check if hold is complete
                        if (_hud.is_buff_menu_hold_complete())
                        {
                            _hud.cancel_buff_menu_hold(); // Reset hold state
                            _hud.toggle_buff_menu();      // Open the menu
                        }
                    }
                    else if (!bn::keypad::l_held() && _hud.is_buff_menu_holding())
                    {
                        // Released before hold complete - cancel
                        _hud.cancel_buff_menu_hold();
                    }
                }
                else
                {
                    // Menu is open - A or L confirms selection, B cancels
                    if (bn::keypad::a_pressed() || bn::keypad::l_pressed())
                    {
                        int selected = _hud.get_selected_buff();
                        PlayerMovement::State buff_state = PlayerMovement::State::IDLE;

                        // Map option index to buff type: 0=Heal, 1=Energy, 2=Power
                        switch (selected)
                        {
                        case 0:
                            buff_state = PlayerMovement::State::HEAL_BUFF;
                            break;
                        case 1:
                            buff_state = PlayerMovement::State::ENERGY_BUFF;
                            break;
                        case 2:
                            buff_state = PlayerMovement::State::POWER_BUFF;
                            break;
                        default:
                            // Keep buff_state as IDLE if selection is out of range
                            break;
                        }

                        activate_buff(buff_state);

                        // Close the menu and start cooldown
                        _hud.toggle_buff_menu();
                        _hud.start_buff_menu_cooldown();
                    }
                    else if (bn::keypad::b_pressed())
                    {
                        // Cancel - close menu without activating buff (no cooldown)
                        _hud.toggle_buff_menu();
                    }
                }
            }

            // Navigate buff menu with D-pad when menu is open (not during SELECT combos)
            if (_hud.is_buff_menu_open() && !bn::keypad::select_held())
            {
                if (bn::keypad::up_pressed())
                {
                    _hud.navigate_buff_menu_up();
                }
                else if (bn::keypad::down_pressed())
                {
                    _hud.navigate_buff_menu_down();
                }
                else if (bn::keypad::left_pressed())
                {
                    _hud.navigate_buff_menu_left();
                }
                else if (bn::keypad::right_pressed())
                {
                    _hud.navigate_buff_menu_right();
                }
            }
        }
        else if (_hud.is_buff_menu_holding())
        {
            // Cancel hold if conditions are no longer met
            _hud.cancel_buff_menu_hold();
        }

        // Debug controls for testing health animations (SELECT + START + D-Pad)
        if (bn::keypad::select_held() && bn::keypad::start_held())
        {
            if (bn::keypad::up_pressed())
            {
                // Test health gain 0->1
                if (get_hp() > 0) take_damage(get_hp()); // Set to 0
                heal(1); // Go to 1
            }
            else if (bn::keypad::right_pressed())
            {
                // Test health gain 1->2
                if (get_hp() > 1) take_damage(get_hp() - 1); // Set to 1
                heal(1); // Go to 2
            }
            else if (bn::keypad::down_pressed())
            {
                // Test health loss 2->1
                if (get_hp() < 2) heal(2 - get_hp()); // Set to 2
                take_damage(1); // Go to 1
            }
            else if (bn::keypad::left_pressed())
            {
                // Test health loss 1->0
                if (get_hp() < 1) heal(1 - get_hp()); // Set to 1
                take_damage(1); // Go to 0
            }
        }

        // Movement inputs (consolidated) - disabled while reviving companion or when buff menu is open
        // Note: Movement IS allowed while holding L to charge the menu
        if (!performing_action && !reviving_companion && !_hud.is_buff_menu_open())
        {
            bool should_run = !_is_strafing && _abilities.running_available();

            if (_is_strafing)
            {
                // Strafe movement
                bn::fixed dx = _movement.dx();
                bn::fixed dy = _movement.dy();

                // Track input directions for diagonal normalization
                bool horizontal_input = false;
                bool vertical_input = false;
                bn::fixed dx_delta = 0;
                bn::fixed dy_delta = 0;

                if (bn::keypad::right_held())
                {
                    dx_delta = PlayerMovement::acc_const;
                    horizontal_input = true;
                }
                else if (bn::keypad::left_held())
                {
                    dx_delta = -PlayerMovement::acc_const;
                    horizontal_input = true;
                }

                if (bn::keypad::up_held())
                {
                    dy_delta = -PlayerMovement::acc_const;
                    vertical_input = true;
                }
                else if (bn::keypad::down_held())
                {
                    dy_delta = PlayerMovement::acc_const;
                    vertical_input = true;
                }

                // Apply diagonal normalization if moving diagonally
                if (horizontal_input && vertical_input)
                {
                    dx_delta *= PlayerMovement::diagonal_factor;
                    dy_delta *= PlayerMovement::diagonal_factor;
                }

                // Apply the normalized deltas with clamping
                dx = bn::clamp(dx + dx_delta, -PlayerMovement::max_speed, PlayerMovement::max_speed);
                dy = bn::clamp(dy + dy_delta, -PlayerMovement::max_speed, PlayerMovement::max_speed);

                _movement.set_dx(dx);
                _movement.set_dy(dy);
                _movement.update_movement_state();
            }
            else
            {
                // Normal movement with diagonal normalization
                bn::fixed dx = _movement.dx();
                bn::fixed dy = _movement.dy();

                // Track input directions for diagonal normalization
                bool horizontal_input = false;
                bool vertical_input = false;
                bn::fixed dx_delta = 0;
                bn::fixed dy_delta = 0;
                PlayerMovement::Direction last_direction = _movement.facing_direction();

                if (bn::keypad::right_held())
                {
                    dx_delta = PlayerMovement::acc_const;
                    horizontal_input = true;
                    last_direction = PlayerMovement::Direction::RIGHT;
                }
                else if (bn::keypad::left_held())
                {
                    dx_delta = -PlayerMovement::acc_const;
                    horizontal_input = true;
                    last_direction = PlayerMovement::Direction::LEFT;
                }

                if (bn::keypad::up_held())
                {
                    dy_delta = -PlayerMovement::acc_const;
                    vertical_input = true;
                    last_direction = PlayerMovement::Direction::UP;
                }
                else if (bn::keypad::down_held())
                {
                    dy_delta = PlayerMovement::acc_const;
                    vertical_input = true;
                    last_direction = PlayerMovement::Direction::DOWN;
                }

                // Apply diagonal normalization if moving diagonally
                if (horizontal_input && vertical_input)
                {
                    dx_delta *= PlayerMovement::diagonal_factor;
                    dy_delta *= PlayerMovement::diagonal_factor;
                }

                // Apply the normalized deltas with clamping
                dx = bn::clamp(dx + dx_delta, -PlayerMovement::max_speed, PlayerMovement::max_speed);
                dy = bn::clamp(dy + dy_delta, -PlayerMovement::max_speed, PlayerMovement::max_speed);

                _movement.set_dx(dx);
                _movement.set_dy(dy);

                // Update facing direction and state
                if (horizontal_input || vertical_input)
                {
                    _movement.set_facing_direction(last_direction);
                }
                _movement.update_movement_state();
            }

            if (should_run && _movement.is_moving())
            {
                // Only allow running if currently in WALKING state
                if (_movement.is_state(PlayerMovement::State::WALKING))
                {
                    _movement.start_action(PlayerMovement::State::RUNNING, 0);
                }
            }
            else if (!should_run && _movement.is_state(PlayerMovement::State::RUNNING))
            {
                _movement.start_action(PlayerMovement::State::WALKING, 0);
            }
        }

        update_gun_if_active();
        _movement.apply_friction();
    }

    void Player::toggle_gun()
    {
        _gun_active = !_gun_active;

        if (_gun_active && !_gun_sprite.has_value())
        {
            // Use shared gun frame
            _gun_sprite = bn::sprite_items::gun.create_sprite(pos().x(), pos().y(), shared_gun_frame);
            _gun_sprite->set_bg_priority(get_sprite()->bg_priority());

            // Update HUD to show current gun frame
            if (_hud.get_weapon() == WEAPON_TYPE::GUN)
            {
                _hud.set_weapon_frame(shared_gun_frame);
            }

            // Set initial z_order based on facing direction
            PlayerMovement::Direction gun_dir = _is_strafing ? _strafing_direction : _movement.facing_direction();
            int gun_z_offset = direction_utils::get_gun_z_offset(static_cast<Direction>(gun_dir));
            _gun_sprite->set_z_order(get_sprite()->z_order() + gun_z_offset);

            if (get_sprite()->camera().has_value())
            {
                _gun_sprite->set_camera(get_sprite()->camera().value());
                _bullet_manager.set_camera(get_sprite()->camera().value());
            }
        }
        else if (!_gun_active)
        {
            _gun_sprite.reset();
        }
    }

    void Player::update_gun_if_active()
    {
        if (_gun_active && _gun_sprite.has_value())
        {
            PlayerMovement::Direction gun_dir = _is_strafing ? _strafing_direction : _movement.facing_direction();
            update_gun_position(gun_dir);
        }
    }

    void Player::switch_weapon()
    {
        // Reset combo system when switching weapons
        _combo_ready = false;
        _last_attack_time = 0;

        // Switch between SWORD and GUN
        if (_hud.get_weapon() == WEAPON_TYPE::GUN)
        {
            _hud.set_weapon(WEAPON_TYPE::SWORD);
            // Turn off gun when switching to sword
            if (_gun_active)
            {
                _gun_active = false;
                _gun_sprite.reset();
            }
            // Hide ammo display when switching to sword
            _hud.set_ammo(0); // This will hide all ammo sprites
        }
        else
        {
            _hud.set_weapon(WEAPON_TYPE::GUN);
            _hud.set_weapon_frame(shared_gun_frame); // Sync HUD icon frame
            // Turn on gun when switching to gun weapon, preserving the frame
            if (!_gun_active)
            {
                _gun_active = true;
                if (!_gun_sprite.has_value())
                {
                    // Use the preserved gun sprite frame
                    _gun_sprite = bn::sprite_items::gun.create_sprite(pos().x(), pos().y(), shared_gun_frame);
                    _gun_sprite->set_bg_priority(get_sprite()->bg_priority());

                    PlayerMovement::Direction gun_dir = _is_strafing ? _strafing_direction : _movement.facing_direction();
                    int gun_z_offset = direction_utils::get_gun_z_offset(static_cast<Direction>(gun_dir));
                    _gun_sprite->set_z_order(get_sprite()->z_order() + gun_z_offset);

                    if (get_sprite()->camera().has_value())
                    {
                        _gun_sprite->set_camera(get_sprite()->camera().value());
                        _bullet_manager.set_camera(get_sprite()->camera().value());
                    }
                }
            }
            // Show current ammo count when switching to gun
            _hud.set_ammo(_ammo_count);
        }
    }

    void Player::cycle_gun_sprite()
    {
        // Only cycle gun sprites when gun is active
        if (_gun_active && _gun_sprite.has_value())
        {
            // Use shared gun frame
            shared_gun_frame = (shared_gun_frame + 1) % 6;

            // Update gun sprite to next frame
            _gun_sprite->set_tiles(bn::sprite_items::gun.tiles_item(), shared_gun_frame);

            // Update HUD gun icon to match player's gun frame
            _hud.set_weapon_frame(shared_gun_frame);

            // Log for debugging
        }
    }

    void Player::cycle_sword_sprite()
    {
        // Placeholder for sword sprite cycling
        // Use shared sword frame
        shared_sword_frame = (shared_sword_frame + 1) % 6; // Assume 6 sword variants like gun

        // Log for debugging
    }

    void Player::activate_buff(PlayerMovement::State buff_state)
    {
        if (buff_state == PlayerMovement::State::IDLE)
        {
            return;
        }

        _movement.start_action(buff_state, PLAYER_BUFF_DURATION);
        _abilities.set_buff_cooldown(PLAYER_BUFF_DURATION);

        // Heal buff restores 1 health
        if (buff_state == PlayerMovement::State::HEAL_BUFF)
        {
            heal(1);
        }

        // TEMP: Soul effects disabled
        // // Trigger soul animation for defense buff
        // if (buff_state == PlayerMovement::State::DEFENCE_BUFF)
        // {
        //     _hud.activate_soul_animation();
        // }
        // // Trigger silver soul animation for energy buff
        // else if (buff_state == PlayerMovement::State::ENERGY_BUFF)
        // {
        //     _hud.activate_silver_soul();
        // }
        // // Deactivate both soul effects when healing
        // else if (buff_state == PlayerMovement::State::HEAL_BUFF)
        // {
        //     _hud.deactivate_silver_soul();
        //     _hud.deactivate_soul_animation();
        // }
    }
}
#include "fe_player.h"
#include "fe_constants.h"

namespace fe
{
    // PlayerMovement Implementation
    PlayerMovement::PlayerMovement() : _dx(0), _dy(0), _current_state(State::IDLE), _facing_direction(Direction::DOWN), _action_timer(0)
    {
    }

    void PlayerMovement::move_right()
    {
        _dx = bn::clamp(_dx + acc_const, -max_speed, max_speed);
        _facing_direction = Direction::RIGHT;
        update_state();
    }

    void PlayerMovement::move_left()
    {
        _dx = bn::clamp(_dx - acc_const, -max_speed, max_speed);
        _facing_direction = Direction::LEFT;
        update_state();
    }

    void PlayerMovement::move_up()
    {
        _dy = bn::clamp(_dy - acc_const, -max_speed, max_speed);
        _facing_direction = Direction::UP;
        update_state();
    }

    void PlayerMovement::move_down()
    {
        _dy = bn::clamp(_dy + acc_const, -max_speed, max_speed);
        _facing_direction = Direction::DOWN;
        update_state();
    }

    void PlayerMovement::move_direction(Direction dir)
    {
        switch (dir)
        {
        case Direction::RIGHT:
            _dx = bn::clamp(_dx + acc_const, -max_speed, max_speed);
            break;
        case Direction::LEFT:
            _dx = bn::clamp(_dx - acc_const, -max_speed, max_speed);
            break;
        case Direction::UP:
            _dy = bn::clamp(_dy - acc_const, -max_speed, max_speed);
            break;
        case Direction::DOWN:
            _dy = bn::clamp(_dy + acc_const, -max_speed, max_speed);
            break;
        default:
            break;
        }
        _facing_direction = dir;
        update_state();
    }

    void PlayerMovement::apply_friction()
    {
        _dx *= friction_const;
        _dy *= friction_const;

        if (bn::abs(_dx) < movement_threshold)
            _dx = 0;
        if (bn::abs(_dy) < movement_threshold)
            _dy = 0;
        update_state();
    }

    void PlayerMovement::reset()
    {
        _dx = 0;
        _dy = 0;
        _current_state = State::IDLE;
        _facing_direction = Direction::DOWN;
        _action_timer = 0;
    }

    void PlayerMovement::stop_movement()
    {
        _dx = 0;
        _dy = 0;
        update_state();
    }

    void PlayerMovement::start_action(State action, int timer)
    {
        _current_state = action;
        _action_timer = timer;
    }

    void PlayerMovement::stop_action()
    {
        _action_timer = 0;
        _current_state = (bn::abs(_dx) > movement_threshold || bn::abs(_dy) > movement_threshold) ? State::WALKING : State::IDLE;
        update_state();
    }

    void PlayerMovement::start_running()
    {
        if (_current_state == State::WALKING || _current_state == State::IDLE)
        {
            _current_state = State::RUNNING;
        }
    }

    void PlayerMovement::stop_running()
    {
        if (_current_state == State::RUNNING)
        {
            _current_state = (bn::abs(_dx) > movement_threshold || bn::abs(_dy) > movement_threshold) ? State::WALKING : State::IDLE;
        }
    }

    void PlayerMovement::start_rolling()
    {
        start_action(State::ROLLING, PLAYER_ROLL_DURATION);
    }

    void PlayerMovement::start_chopping()
    {
        start_action(State::CHOPPING, PLAYER_CHOP_DURATION);
    }

    void PlayerMovement::start_slashing()
    {
        start_action(State::SLASHING, PLAYER_SLASH_DURATION);
    }

    void PlayerMovement::start_attacking()
    {
        start_action(State::ATTACKING, PLAYER_ATTACK_DURATION);
    }

    void PlayerMovement::start_buff(State buff_type)
    {
        start_action(buff_type, PLAYER_BUFF_DURATION);
    }

    void PlayerMovement::update_state()
    {
        if (_action_timer > 0)
            return;

        bool is_moving = bn::abs(_dx) > movement_threshold || bn::abs(_dy) > movement_threshold;
        if (is_moving && (_current_state == State::IDLE || _current_state == State::WALKING || _current_state == State::RUNNING))
            _current_state = State::WALKING;
        else if (!is_moving && (_current_state == State::WALKING || _current_state == State::RUNNING))
            _current_state = State::IDLE;
    }
}
#include "fe_player.h"

namespace fe
{
    // PlayerState Implementation
    void PlayerState::set_listening(bool listening)
    {
        if (_listening && !listening)
        {
            // Dialog just ended, set cooldown
            _dialog_cooldown = 10; // 10 frame cooldown
        }
        _listening = listening;
    }

    void PlayerState::update_dialog_cooldown()
    {
        if (_dialog_cooldown > 0)
            _dialog_cooldown--;
    }

    void PlayerState::reset()
    {
        _invulnerable = false;
        _listening = false;
        _inv_timer = 0;
        _dialog_cooldown = 0;
    }
}
#include "fe_player.h"
#include "bn_sprite_items_hero_vfx.h"

namespace fe
{
    PlayerVFX::PlayerVFX() : _last_vfx_state(PlayerMovement::State::IDLE), _last_vfx_direction(PlayerMovement::Direction::DOWN)
    {
    }

    void PlayerVFX::initialize(bn::camera_ptr camera)
    {
        _camera = camera;
    }

    void PlayerVFX::update(bn::fixed_point player_pos, PlayerMovement::State state, PlayerMovement::Direction direction)
    {
        if (should_show_vfx(state))
        {
            if (!_vfx_sprite.has_value())
            {
                // Create VFX sprite using hero_vfx
                _vfx_sprite = bn::sprite_items::hero_vfx.create_sprite(0, 0);
                if (_camera.has_value())
                {
                    _vfx_sprite->set_camera(*_camera);
                }
                _vfx_sprite->set_bg_priority(0);
                _vfx_sprite->set_z_order(-32000);
            }

            if (should_change_vfx(state, direction))
            {
                apply_vfx_state(state, direction);
            }

            // Ensure sprite is visible
            _vfx_sprite->set_visible(true);

            // Update position to follow player with offset for up/down attacks
            bn::fixed_point vfx_pos = player_pos;
            bool is_attack = (state == PlayerMovement::State::SLASHING ||
                              state == PlayerMovement::State::ATTACKING ||
                              state == PlayerMovement::State::CHOPPING);
            if (is_attack && (direction == PlayerMovement::Direction::UP ||
                              direction == PlayerMovement::Direction::DOWN))
            {
                vfx_pos = bn::fixed_point(player_pos.x() + 8, player_pos.y() + PLAYER_SPRITE_Y_OFFSET);
            }
            else
            {
                vfx_pos = bn::fixed_point(player_pos.x(), player_pos.y() + PLAYER_SPRITE_Y_OFFSET);
            }
            _vfx_sprite->set_position(vfx_pos);

            // Update animation - check if done to avoid crash on completed "once" animations
            if (_vfx_animation.has_value())
            {
                if (_vfx_animation->done())
                {
                    hide_vfx();
                }
                else
                {
                    _vfx_animation->update();
                }
            }
        }
        else
        {
            hide_vfx();
        }

        _last_vfx_state = state;
        _last_vfx_direction = direction;
    }

    void PlayerVFX::apply_vfx_state(PlayerMovement::State state, PlayerMovement::Direction direction)
    {
        if (!_vfx_sprite.has_value())
            return;

        // Set horizontal flip for left direction
        _vfx_sprite->set_horizontal_flip(direction == PlayerMovement::Direction::LEFT);

        // Frame indices using row-based calculation (64x64 sprites, 24 columns per row)
        // Matching hero_sword animation layout:
        // Row 7: chop_down (168+), Row 8: slash_down (192+)
        // Row 14: lr_slash (336+), Row 15: lr_slash2 (360+)
        // Row 19: chop_up (456+), Row 20: attack_up (480+)
        switch (state)
        {
        case PlayerMovement::State::SLASHING:
            if (direction == PlayerMovement::Direction::UP)
                make_vfx_anim_range_once(4, 480, 486); // attack_up (row 20, 7 frames)
            else if (direction == PlayerMovement::Direction::DOWN)
                make_vfx_anim_range_once(4, 192, 198); // slash_down (row 8, 7 frames)
            else
                make_vfx_anim_range_once(4, 336, 339); // lr_slash (row 14, 4 frames)
            break;
        case PlayerMovement::State::ATTACKING:
            if (direction == PlayerMovement::Direction::UP)
                make_vfx_anim_range_once(4, 480, 486); // attack_up (row 20, 7 frames)
            else if (direction == PlayerMovement::Direction::DOWN)
                make_vfx_anim_range_once(4, 192, 198); // slash_down (row 8, 7 frames)
            else
                make_vfx_anim_range_once(4, 360, 364); // lr_slash2 (row 15, 5 frames)
            break;
        case PlayerMovement::State::CHOPPING:
            if (direction == PlayerMovement::Direction::UP)
                make_vfx_anim_range_once(5, 456, 459); // chop_up (row 19, 4 frames)
            else if (direction == PlayerMovement::Direction::DOWN)
                make_vfx_anim_range_once(5, 168, 171); // chop_down (row 7, 4 frames)
            else
                make_vfx_anim_range_once(5, 336, 339); // lr_slash (row 14, 4 frames)
            break;
        case PlayerMovement::State::HEAL_BUFF:
            make_vfx_anim_range(4, 24, 47); // Row 1
            break;
        case PlayerMovement::State::DEFENCE_BUFF:
            make_vfx_anim_range(4, 48, 71); // Row 2
            break;
        case PlayerMovement::State::POWER_BUFF:
            make_vfx_anim_range(4, 72, 95); // Row 3
            break;
        case PlayerMovement::State::ENERGY_BUFF:
            make_vfx_anim_range(4, 96, 119); // Row 4
            break;
        default:
            hide_vfx();
            break;
        }
    }

    void PlayerVFX::hide_vfx()
    {
        if (_vfx_sprite.has_value())
        {
            _vfx_sprite->set_visible(false);
        }
        _vfx_animation.reset();
    }

    bool PlayerVFX::should_show_vfx(PlayerMovement::State state) const
    {
        return state == PlayerMovement::State::SLASHING ||
               state == PlayerMovement::State::ATTACKING ||
               state == PlayerMovement::State::CHOPPING ||
               state == PlayerMovement::State::POWER_BUFF ||
               state == PlayerMovement::State::DEFENCE_BUFF ||
               state == PlayerMovement::State::HEAL_BUFF ||
               state == PlayerMovement::State::ENERGY_BUFF;
    }

    bool PlayerVFX::should_change_vfx(PlayerMovement::State state, PlayerMovement::Direction direction) const
    {
        return state != _last_vfx_state || direction != _last_vfx_direction;
    }

    void PlayerVFX::make_vfx_anim_range(int speed, int start_frame, int end_frame)
    {
        if (!_vfx_sprite.has_value())
            return;

        bn::vector<uint16_t, 32> frames;
        for (int i = start_frame; i <= end_frame; ++i)
        {
            frames.push_back(i);
        }

        _vfx_animation = bn::sprite_animate_action<32>::forever(
            *_vfx_sprite, speed, bn::sprite_items::hero_vfx.tiles_item(),
            bn::span<const uint16_t>(frames.data(), frames.size()));
        _vfx_sprite->set_visible(true);
    }

    void PlayerVFX::make_vfx_anim_range_once(int speed, int start_frame, int end_frame)
    {
        if (!_vfx_sprite.has_value())
            return;

        bn::vector<uint16_t, 32> frames;
        for (int i = start_frame; i <= end_frame; ++i)
        {
            frames.push_back(i);
        }

        _vfx_animation = bn::sprite_animate_action<32>::once(
            *_vfx_sprite, speed, bn::sprite_items::hero_vfx.tiles_item(),
            bn::span<const uint16_t>(frames.data(), frames.size()));
        _vfx_sprite->set_visible(true);
    }
}
#include "fe_player_companion.h"
#include "fe_constants.h"
#include "bn_sprite_items_companion.h"
#include "bn_sprite_items_companion_load.h"
#include "bn_sprite_animate_actions.h"
#include "common_variable_8x8_sprite_font.h"

namespace fe
{
    PlayerCompanion::PlayerCompanion(bn::sprite_ptr sprite)
        : _sprite(bn::move(sprite)),
          _position(0, 0),
          _position_side(Position::RIGHT),
          _is_dead(false),
          _is_flying(false),
          _player_too_close(false),
          _follow_delay(0),
          _target_offset(24, 0),
          _independent_death(false),
          _death_position(0, 0),
          _can_be_revived(false),
          _is_reviving(false),
          _revival_in_progress(false),
          _revival_timer(0)
    {
        // Don't set z_order here - let the dynamic z_order system handle it
        // The player's update_z_order() method will manage companion z_order based on position
    }

    void PlayerCompanion::spawn(bn::fixed_point pos, bn::camera_ptr camera)
    {
        // Don't override position if companion died independently and is staying at death position
        if (!_independent_death)
        {
            _position = pos + bn::fixed_point(8, -8);
        }
        _target_offset = calculate_companion_offset();
        _sprite.set_camera(camera);
        update_animation();

        // FOR TESTING: Spawn companion dead
        die_independently();
    }

    void PlayerCompanion::update(bn::fixed_point player_pos, bool player_is_dead)
    {
        // Handle player death affecting companion
        if (player_is_dead != _is_dead && !_independent_death && !_is_reviving)
        {
            _is_dead = player_is_dead;
            update_animation();
        }

        // If companion is reviving, handle revival animation
        if (_is_reviving)
        {
            // Stay at death position during revival
            _sprite.set_position(_death_position);

            // Check if revival animation is complete
            if (_animation && _animation->done())
            {
                // Revival complete - companion is now alive
                _is_reviving = false;
                _is_dead = false;
                _independent_death = false;
                _position = _death_position; // Start from death position
                update_animation();          // Switch to normal alive animation
            }
        }
        // If companion died independently, it should stay dead until revived
        else if (_independent_death)
        {
            // Stay at death position
            _sprite.set_position(_death_position);

            // Show/hide revival text based on player proximity and revival state
            if (_can_be_revived && !_revival_in_progress)
            {
                // Calculate distance to see if player is close enough
                bn::fixed_point diff = player_pos - _death_position;
                bn::fixed distance_sq = diff.x() * diff.x() + diff.y() * diff.y();
                bool player_in_range = (distance_sq <= COMPANION_REVIVE_DISTANCE * COMPANION_REVIVE_DISTANCE);

                if (player_in_range && _text_sprites.empty())
                {
                    show_revival_text();
                }
                else if (!player_in_range && !_text_sprites.empty())
                {
                    hide_revival_text();
                }
            }
            else if (_text_sprites.size() > 0)
            {
                hide_revival_text();
            }
        }
        else if (!_is_dead)
        {
            update_position(player_pos);
        }

        // Update animation - for death/revival animation, only update while not done
        if (_animation && (!_is_dead || !_animation->done() || _is_reviving))
        {
            _animation->update();

            // If death animation just finished and this was an independent death, mark as revivable
            if (_is_dead && _independent_death && _animation->done() && !_can_be_revived && !_is_reviving)
            {
                _can_be_revived = true;
            }
        }
    }

    void PlayerCompanion::update_position(bn::fixed_point player_pos)
    {
        // Define proximity thresholds for idle behavior

        // Calculate direct distance from companion to player
        bn::fixed_point companion_to_player = player_pos - _position;
        bn::fixed player_distance = bn::sqrt(companion_to_player.x() * companion_to_player.x() +
                                             companion_to_player.y() * companion_to_player.y());

        // Update proximity state with hysteresis to prevent oscillation
        if (!_player_too_close && player_distance < COMPANION_IDLE_DISTANCE)
        {
            _player_too_close = true;
        }
        else if (_player_too_close && player_distance > COMPANION_RESUME_DISTANCE)
        {
            _player_too_close = false;
        }

        // Only move if player is not too close
        if (!_player_too_close)
        {
            bn::fixed_point target_pos = player_pos + _target_offset;
            bn::fixed_point diff = target_pos - _position;
            bn::fixed distance = bn::sqrt(diff.x() * diff.x() + diff.y() * diff.y());

            if (distance > 1)
            {
                bn::fixed speed = (distance * 0.08 < 1.2) ? distance * 0.08 : 1.2;
                speed = (speed > 0.3) ? speed : 0.3;
                _position += (diff / distance) * speed;
            }
        }

        // Update position side more frequently for responsive sprite direction changes
        // Allow this even when not moving so companion can face the right direction
        if (player_distance > 8)
        {
            bn::fixed_point offset = _position - player_pos;
            Position new_side;

            // Check if companion is primarily above or below the player
            if (bn::abs(offset.y()) > bn::abs(offset.x()))
            {
                // Companion is primarily above or below player
                if (offset.y() < 0)
                {
                    // Companion is above player - should look down
                    // Use left/right animation based on horizontal offset
                    new_side = (offset.x() >= 0) ? Position::RIGHT : Position::LEFT;
                }
                else
                {
                    // Companion is below player
                    new_side = Position::BELOW;
                }
            }
            else
            {
                // Companion is primarily to the left or right of player
                new_side = (offset.x() > 0) ? Position::RIGHT : Position::LEFT;
            }

            set_position_side(new_side);
        }

        _sprite.set_position(_position);
    }

    bn::fixed_point PlayerCompanion::calculate_companion_offset() const
    {
        switch (_position_side)
        {
        case Position::RIGHT:
            return {16, 0};
        case Position::LEFT:
            return {-16, 0};
        case Position::BELOW:
            return {0, 12};
        default:
            return {16, 0};
        }
    }

    void PlayerCompanion::update_animation()
    {
        if (_is_reviving)
        {
            // Play death animation in reverse for revival
            _animation = bn::create_sprite_animate_action_once(
                _sprite, 8, bn::sprite_items::companion.tiles_item(),
                21, 20, 19, 18, 17, 16, 15, 14, 13, 12);
        }
        else if (_is_dead)
        {
            _animation = bn::create_sprite_animate_action_once(
                _sprite, 8, bn::sprite_items::companion.tiles_item(),
                12, 13, 14, 15, 16, 17, 18, 19, 20, 21);
        }
        else
        {
            int start_frame = static_cast<int>(_position_side) * 4;
            _animation = bn::create_sprite_animate_action_forever(
                _sprite, 12, bn::sprite_items::companion.tiles_item(),
                start_frame, start_frame + 1, start_frame + 2, start_frame + 3);
        }
    }

    void PlayerCompanion::set_flying(bool flying)
    {
        _is_flying = flying;
        update_animation();
    }

    void PlayerCompanion::set_camera(bn::camera_ptr camera)
    {
        _sprite.set_camera(camera);
    }

    void PlayerCompanion::set_position_side(Position side)
    {
        if (_position_side != side)
        {
            _position_side = side;
            _target_offset = calculate_companion_offset();
            update_animation();
        }
    }

    void PlayerCompanion::set_visible(bool visible)
    {
        _sprite.set_visible(visible);
    }

    void PlayerCompanion::set_z_order(int z_order)
    {
        _sprite.set_z_order(z_order);
    }

    void PlayerCompanion::start_death_animation()
    {
        _is_dead = true;
        update_animation();
    }

    void PlayerCompanion::die_independently()
    {
        if (!_is_dead) // Only die if not already dead
        {
            _is_dead = true;
            _independent_death = true;
            _death_position = _position; // Remember where we died
            _can_be_revived = false;     // Will be set to true when death animation completes

            // Cancel any revival in progress
            cancel_revival();

            update_animation();
        }
    }

    bool PlayerCompanion::try_revive(bn::fixed_point player_pos, bool a_pressed, bool a_held)
    {
        if (!_independent_death || !_can_be_revived)
        {
            return false;
        }

        // Calculate squared distance between player and dead companion
        bn::fixed_point diff = player_pos - _death_position;
        bn::fixed distance_sq = diff.x() * diff.x() + diff.y() * diff.y();

        bool player_in_range = (distance_sq <= COMPANION_REVIVE_DISTANCE * COMPANION_REVIVE_DISTANCE);

        if (!player_in_range)
        {
            // Player moved away, cancel any revival in progress
            if (_revival_in_progress)
            {
                cancel_revival();
            }
            return false;
        }

        // Player is in range
        if (!_revival_in_progress)
        {
            // Start revival process when A is first pressed
            if (a_pressed)
            {
                _revival_in_progress = true;
                _revival_timer = 0;

                // Create progress bar sprite above the dead companion
                _progress_bar_sprite = bn::sprite_items::companion_load.create_sprite(
                    _death_position.x(), _death_position.y(), 0); // Start with frame 0 (empty)

                if (_sprite.camera().has_value())
                {
                    _progress_bar_sprite->set_camera(_sprite.camera().value());
                }
                _progress_bar_sprite->set_z_order(_sprite.z_order() - 1); // In front of companion
            }
        }
        else
        {
            // Revival in progress
            if (a_held)
            {
                // Continue revival
                _revival_timer++;

                // Update progress bar frame (8 frames for 300 total ticks)
                int progress_frame = (_revival_timer * 8) / COMPANION_REVIVAL_DURATION;
                if (progress_frame > 7)
                    progress_frame = 7; // Clamp to max frame

                if (_progress_bar_sprite.has_value())
                {
                    _progress_bar_sprite->set_tiles(bn::sprite_items::companion_load.tiles_item(), progress_frame);
                    // Keep progress bar positioned above companion
                    _progress_bar_sprite->set_position(_death_position.x() + 12, _death_position.y());
                }

                // Check if revival is complete
                if (_revival_timer >= COMPANION_REVIVAL_DURATION)
                {
                    // Revival complete - start revival animation
                    _revival_in_progress = false;
                    _revival_timer = 0;
                    _is_reviving = true;
                    _can_be_revived = false;     // Prevent multiple revival attempts
                    _position = _death_position; // Start revival at death position

                    // Hide progress bar
                    _progress_bar_sprite.reset();

                    update_animation(); // Start reverse animation
                    return true;
                }
            }
            else
            {
                // Player released A button, cancel revival
                cancel_revival();
            }
        }

        return false;
    }

    void PlayerCompanion::cancel_revival()
    {
        _revival_in_progress = false;
        _revival_timer = 0;

        // Hide progress bar
        if (_progress_bar_sprite.has_value())
        {
            _progress_bar_sprite.reset();
        }

        // Hide revival text
        hide_revival_text();
    }

    void PlayerCompanion::show_revival_text()
    {
        if (!_text_sprites.empty())
            return; // Already showing

        bn::sprite_text_generator text_generator(common::variable_8x8_sprite_font);
        text_generator.set_center_alignment();

        // Create text sprites at position above the dead companion
        bn::fixed_point text_center = _death_position + bn::fixed_point(0, -20); // 20 pixels above companion
        text_generator.set_bg_priority(0);
        text_generator.generate(text_center, "Press A to revive", _text_sprites);

        // Store original offsets from text center and set camera
        _text_original_offsets.clear();
        for (bn::sprite_ptr &text_sprite : _text_sprites)
        {
            text_sprite.set_camera(_sprite.camera());
            text_sprite.set_z_order(-32767); // Ensure text is on top
            // Store offset from text center
            _text_original_offsets.push_back(text_sprite.position() - text_center);
        }
    }

    void PlayerCompanion::hide_revival_text()
    {
        _text_sprites.clear();
    }

    void PlayerCompanion::reset_text_positions()
    {
        if (_text_sprites.empty() || _text_original_offsets.empty())
            return;
        
        // Restore original positions from stored offsets
        bn::fixed_point text_center = _death_position + bn::fixed_point(0, -20);
        for (int i = 0; i < _text_sprites.size() && i < _text_original_offsets.size(); ++i)
        {
            _text_sprites[i].set_position(text_center + _text_original_offsets[i]);
        }
    }
}
