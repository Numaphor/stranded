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

        // Toggle controls (disabled while reviving companion)
        if (bn::keypad::r_pressed() && !performing_action && !reviving_companion)
        {
            _is_strafing = !_is_strafing;
            if (_is_strafing)
                _strafing_direction = _movement.facing_direction();
        }

        // Weapon switching moved to SELECT + L (SHIFT + L equivalent)
        if (bn::keypad::select_held() && bn::keypad::l_pressed() && !reviving_companion)
            switch_weapon();

        // Auto-reload when holding L with gun active (but not SELECT + L)
        if (bn::keypad::l_held() && !bn::keypad::select_held() && _gun_active && !reviving_companion)
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
        else
        {
            // Keep current timer value when not holding L (don't reset to 0)
            // This prevents spam-clicking L to get instant bullets
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
                else if (_abilities.slashing_available())
                {
                    _movement.start_action(PlayerMovement::State::SLASHING, PLAYER_SLASH_DURATION);
                    _abilities.set_slash_cooldown(60);
                }
            }
            else if (bn::keypad::select_held() && _abilities.buff_abilities_available())
            {
                // Buff inputs (consolidated) - LEGACY: replaced by buff menu system
                // Keeping for backwards compatibility if buff menu not used
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
                    _movement.start_action(buff_state, PLAYER_BUFF_DURATION);
                    _abilities.set_buff_cooldown(PLAYER_BUFF_DURATION);

                    // Trigger soul animation for defense buff
                    if (buff_state == PlayerMovement::State::DEFENCE_BUFF)
                    {
                        _hud.activate_soul_animation();
                    }
                    // Trigger silver soul animation for energy buff
                    else if (buff_state == PlayerMovement::State::ENERGY_BUFF)
                    {
                        _hud.activate_silver_soul();
                    }
                    // Deactivate both soul effects when healing
                    else if (buff_state == PlayerMovement::State::HEAL_BUFF)
                    {
                        _hud.deactivate_silver_soul();
                        _hud.deactivate_soul_animation();
                    }
                }
            }
        }

        // Buff menu system (new approach with L button)
        if (!performing_action && !reviving_companion && _abilities.buff_abilities_available())
        {
            // Toggle buff menu with L button (when not holding SELECT to avoid weapon switch conflict)
            if (bn::keypad::l_pressed() && !bn::keypad::select_held())
            {
                if (!_hud.is_buff_menu_open())
                {
                    // Open the buff menu
                    _hud.toggle_buff_menu();
                }
                else
                {
                    // Menu is open - activate selected buff and close menu
                    int selected = _hud.get_selected_buff();
                    PlayerMovement::State buff_state = PlayerMovement::State::IDLE;
                    
                    // Map option index to buff type: 0=Up(Heal), 1=Right(Energy), 2=Down(Power), 3=Left(Defence)
                    if (selected == 0)
                        buff_state = PlayerMovement::State::HEAL_BUFF;
                    else if (selected == 1)
                        buff_state = PlayerMovement::State::ENERGY_BUFF;
                    else if (selected == 2)
                        buff_state = PlayerMovement::State::POWER_BUFF;
                    else if (selected == 3)
                        buff_state = PlayerMovement::State::DEFENCE_BUFF;
                    
                    if (buff_state != PlayerMovement::State::IDLE)
                    {
                        _movement.start_action(buff_state, PLAYER_BUFF_DURATION);
                        _abilities.set_buff_cooldown(PLAYER_BUFF_DURATION);

                        // Trigger soul animation for defense buff
                        if (buff_state == PlayerMovement::State::DEFENCE_BUFF)
                        {
                            _hud.activate_soul_animation();
                        }
                        // Trigger silver soul animation for energy buff
                        else if (buff_state == PlayerMovement::State::ENERGY_BUFF)
                        {
                            _hud.activate_silver_soul();
                        }
                        // Deactivate both soul effects when healing
                        else if (buff_state == PlayerMovement::State::HEAL_BUFF)
                        {
                            _hud.deactivate_silver_soul();
                            _hud.deactivate_soul_animation();
                        }
                    }
                    
                    // Close the menu
                    _hud.toggle_buff_menu();
                }
            }
            
            // Navigate buff menu with A and B when menu is open
            if (_hud.is_buff_menu_open())
            {
                if (bn::keypad::a_pressed())
                {
                    _hud.navigate_buff_menu_next();
                }
                else if (bn::keypad::b_pressed())
                {
                    _hud.navigate_buff_menu_prev();
                }
            }
        }

        // Movement inputs (consolidated) - disabled while reviving companion or when buff menu is open
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
}
