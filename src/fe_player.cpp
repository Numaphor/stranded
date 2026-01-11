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

        // Handle death animation
        if (_movement.current_state() == PlayerMovement::State::DEAD && _death_timer > 0)
        {
            _death_timer--;
            if (_death_timer == PLAYER_DEATH_ANIMATION_DURATION / 2 && !_death_sound_played)
            {
                bn::sound_items::death.play();
                _death_sound_played = true;
            }
            if (_death_timer == 0)
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
            // No offset needed since the new 32x32 sprite tightly fits around the player
            sprite->set_position(pos.x(), pos.y());
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

                // Play reverse soul animation when taking damage
                _hud.play_soul_damage_animation();
            }
            _hud.set_hp(_hp);
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
        _hud.set_hp(_hp);
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