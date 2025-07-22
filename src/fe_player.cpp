#include "fe_player.h"
#include "bn_keypad.h"
#include "bn_sprite_items_hero.h"
#include "bn_sprite_items_gun.h"
#include "bn_math.h"
#include "fe_level.h"
#include "bn_log.h"
#include "fe_enemy.h"
#include "fe_collision.h"
#include "bn_sprite_palette_ptr.h"
#include "fe_bullet_manager.h"

extern fe::Level *_level;

namespace fe
{
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

    void PlayerMovement::apply_friction()
    {
        _dx *= friction_const;
        _dy *= friction_const;

        if (bn::abs(_dx) < movement_threshold)
        {
            _dx = 0;
        }
        if (bn::abs(_dy) < movement_threshold)
        {
            _dy = 0;
        }

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

    void PlayerMovement::update_state()
    {
        // Don't change state if we're performing an action with a timer
        if (_action_timer > 0)
        {
            return;
        }
        
        if (bn::abs(_dx) > movement_threshold || bn::abs(_dy) > movement_threshold)
        {
            // Default to walking if moving, can be overridden by other methods
            if (_current_state == State::IDLE || _current_state == State::WALKING || _current_state == State::RUNNING)
            {
                _current_state = State::WALKING;
            }
        }
        else
        {
            // Only return to idle if not performing an action
            if (_current_state == State::WALKING || _current_state == State::RUNNING)
            {
                _current_state = State::IDLE;
            }
        }
    }

    // New movement methods implementation
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
            update_state(); // Will set to WALKING or IDLE based on movement
        }
    }

    void PlayerMovement::start_rolling()
    {
        _current_state = State::ROLLING;
        _action_timer = 30; // 0.5 seconds at 60 FPS
    }

    void PlayerMovement::start_chopping()
    {
        _current_state = State::CHOPPING;
        _action_timer = 20; // ~0.33 seconds at 60 FPS
    }

    void PlayerMovement::start_slashing()
    {
        _current_state = State::SLASHING;
        _action_timer = 25; // ~0.4 seconds at 60 FPS
    }

    void PlayerMovement::start_attacking()
    {
        _current_state = State::ATTACKING;
        _action_timer = 30; // 0.5 seconds at 60 FPS
    }

    void PlayerMovement::start_buff(State buff_type)
    {
        if (buff_type == State::HEAL_BUFF || buff_type == State::DEFENCE_BUFF || 
            buff_type == State::POWER_BUFF || buff_type == State::ENERGY_BUFF)
        {
            _current_state = buff_type;
            _action_timer = 90; // 1.5 seconds at 60 FPS for buff animations
        }
    }

    void PlayerMovement::stop_action()
    {
        _action_timer = 0;
        
        // Reset to appropriate state based on movement
        if (bn::abs(_dx) > movement_threshold || bn::abs(_dy) > movement_threshold)
        {
            _current_state = State::WALKING;
        }
        else
        {
            _current_state = State::IDLE;
        }
        
        // Ensure the state is properly updated
        update_state();
    }

    // PlayerAnimation Implementation
    PlayerAnimation::PlayerAnimation(bn::sprite_ptr sprite) : _sprite(bn::move(sprite))
    {
    }

    void PlayerAnimation::update()
    {
        if (_animation.has_value())
        {
            _animation.value().update();
        }
    }

    void PlayerAnimation::apply_state(PlayerMovement::State state, PlayerMovement::Direction direction)
    {
        bool should_change = !_animation.has_value();

        if (!should_change && _animation)
        {
            const auto &indexes = _animation->graphics_indexes();
            int current_frame = indexes.front();
            
            // Check if we need to change animation based on new state and frame ranges
            switch (state)
            {
            case PlayerMovement::State::DEAD:
                should_change = (current_frame < 234 || current_frame > 246);
                break;
            case PlayerMovement::State::HIT:
                // Use a substitute animation for hit - could use a frame from another animation
                should_change = (current_frame < 0 || current_frame > 12); // Use idle_down frames for hit
                break;
            case PlayerMovement::State::IDLE:
                switch (direction)
                {
                case PlayerMovement::Direction::DOWN:
                    should_change = (current_frame < 0 || current_frame > 12);
                    break;
                case PlayerMovement::Direction::LEFT:
                case PlayerMovement::Direction::RIGHT:
                    should_change = (current_frame < 144 || current_frame > 155);
                    break;
                case PlayerMovement::Direction::UP:
                    should_change = (current_frame < 187 || current_frame > 198);
                    break;
                }
                break;
            case PlayerMovement::State::WALKING:
                switch (direction)
                {
                case PlayerMovement::Direction::DOWN:
                    should_change = (current_frame < 109 || current_frame > 116);
                    break;
                case PlayerMovement::Direction::LEFT:
                case PlayerMovement::Direction::RIGHT:
                    should_change = (current_frame < 156 || current_frame > 163);
                    break;
                case PlayerMovement::Direction::UP:
                    should_change = (current_frame < 199 || current_frame > 206);
                    break;
                }
                break;
            case PlayerMovement::State::RUNNING:
                switch (direction)
                {
                case PlayerMovement::Direction::DOWN:
                    should_change = (current_frame < 117 || current_frame > 124);
                    break;
                case PlayerMovement::Direction::LEFT:
                case PlayerMovement::Direction::RIGHT:
                    should_change = (current_frame < 164 || current_frame > 171);
                    break;
                case PlayerMovement::Direction::UP:
                    should_change = (current_frame < 207 || current_frame > 214);
                    break;
                }
                break;
            default:
                should_change = true; // Force change for new states
                break;
            }
        }

        if (!should_change)
            return;

        // Create animation based on state and direction using new frame ranges
        auto make_anim_range = [&](int speed, int start_frame, int end_frame)
        {
            // Calculate how many frames we need (up to 4 for the animation action)
            int frame_count = (end_frame - start_frame + 1);
            int f0 = start_frame;
            int f1 = (frame_count > 1) ? start_frame + 1 : start_frame;
            int f2 = (frame_count > 2) ? start_frame + 2 : f1;
            int f3 = (frame_count > 3) ? start_frame + 3 : f2;
            
            _animation = bn::create_sprite_animate_action_forever(
                _sprite, speed, bn::sprite_items::hero.tiles_item(), f0, f1, f2, f3);
        };

        switch (state)
        {
        case PlayerMovement::State::DEAD:
            make_anim_range(6, 234, 246); // death: 234-246
            break;
        case PlayerMovement::State::HIT:
            // Use first few frames of idle_down for hit effect
            make_anim_range(6, 0, 3);
            break;
        case PlayerMovement::State::HEAL_BUFF:
            make_anim_range(4, 13, 36); // heal_buff: 13-36
            break;
        case PlayerMovement::State::DEFENCE_BUFF:
            make_anim_range(4, 37, 60); // defence_buff: 37-60
            break;
        case PlayerMovement::State::POWER_BUFF:
            make_anim_range(4, 61, 84); // power_buff: 61-84
            break;
        case PlayerMovement::State::ENERGY_BUFF:
            make_anim_range(4, 85, 108); // energy_buff: 85-108
            break;
        case PlayerMovement::State::ROLLING:
            switch (direction)
            {
            case PlayerMovement::Direction::DOWN:
                make_anim_range(8, 136, 143); // roll_down: 136-143
                break;
            case PlayerMovement::Direction::UP:
                make_anim_range(8, 226, 233); // roll_up: 226-233
                break;
            case PlayerMovement::Direction::LEFT:
                _sprite.set_horizontal_flip(true);
                make_anim_range(8, 172, 177); // lr_roll: 172-177
                break;
            case PlayerMovement::Direction::RIGHT:
                _sprite.set_horizontal_flip(false);
                make_anim_range(8, 172, 177); // lr_roll: 172-177
                break;
            }
            break;
        case PlayerMovement::State::CHOPPING:
            switch (direction)
            {
            case PlayerMovement::Direction::DOWN:
                make_anim_range(10, 125, 128); // chop_down: 125-128
                break;
            case PlayerMovement::Direction::UP:
                make_anim_range(10, 215, 218); // chop_up: 215-218
                break;
            case PlayerMovement::Direction::LEFT:
            case PlayerMovement::Direction::RIGHT:
                // No left/right chop, use slash instead
                _sprite.set_horizontal_flip(direction == PlayerMovement::Direction::LEFT);
                make_anim_range(10, 178, 181); // lr_slash: 178-181
                break;
            }
            break;
        case PlayerMovement::State::SLASHING:
            switch (direction)
            {
            case PlayerMovement::Direction::DOWN:
                make_anim_range(8, 129, 135); // slash_down: 129-135
                break;
            case PlayerMovement::Direction::UP:
                make_anim_range(8, 219, 225); // attack_up: 219-225 (using as slash_up)
                break;
            case PlayerMovement::Direction::LEFT:
                _sprite.set_horizontal_flip(true);
                make_anim_range(8, 178, 181); // lr_slash: 178-181
                break;
            case PlayerMovement::Direction::RIGHT:
                _sprite.set_horizontal_flip(false);
                make_anim_range(8, 178, 181); // lr_slash: 178-181
                break;
            }
            break;
        case PlayerMovement::State::ATTACKING:
            switch (direction)
            {
            case PlayerMovement::Direction::DOWN:
                make_anim_range(8, 129, 135); // Use slash_down for attack_down
                break;
            case PlayerMovement::Direction::UP:
                make_anim_range(8, 219, 225); // attack_up: 219-225
                break;
            case PlayerMovement::Direction::LEFT:
                _sprite.set_horizontal_flip(true);
                make_anim_range(8, 182, 186); // lr_slash second variant: 182-186
                break;
            case PlayerMovement::Direction::RIGHT:
                _sprite.set_horizontal_flip(false);
                make_anim_range(8, 182, 186); // lr_slash second variant: 182-186
                break;
            }
            break;
        case PlayerMovement::State::RUNNING:
            switch (direction)
            {
            case PlayerMovement::Direction::DOWN:
                make_anim_range(8, 117, 124); // run_down: 117-124
                break;
            case PlayerMovement::Direction::UP:
                make_anim_range(8, 207, 214); // run_up: 207-214
                break;
            case PlayerMovement::Direction::LEFT:
                _sprite.set_horizontal_flip(true);
                make_anim_range(8, 164, 171); // lr_run: 164-171
                break;
            case PlayerMovement::Direction::RIGHT:
                _sprite.set_horizontal_flip(false);
                make_anim_range(8, 164, 171); // lr_run: 164-171
                break;
            }
            break;
        case PlayerMovement::State::WALKING:
            switch (direction)
            {
            case PlayerMovement::Direction::DOWN:
                make_anim_range(12, 109, 116); // move_down: 109-116
                break;
            case PlayerMovement::Direction::UP:
                make_anim_range(12, 199, 206); // move_up: 199-206
                break;
            case PlayerMovement::Direction::LEFT:
                _sprite.set_horizontal_flip(true);
                make_anim_range(12, 156, 163); // lr_move: 156-163
                break;
            case PlayerMovement::Direction::RIGHT:
                _sprite.set_horizontal_flip(false);
                make_anim_range(12, 156, 163); // lr_move: 156-163
                break;
            }
            break;
        default: // IDLE
            switch (direction)
            {
            case PlayerMovement::Direction::DOWN:
                make_anim_range(12, 0, 12); // idle_down: 0-12
                break;
            case PlayerMovement::Direction::UP:
                make_anim_range(12, 187, 198); // idle_up: 187-198
                break;
            case PlayerMovement::Direction::LEFT:
                _sprite.set_horizontal_flip(true);
                make_anim_range(12, 144, 155); // lr_idle: 144-155
                break;
            case PlayerMovement::Direction::RIGHT:
                _sprite.set_horizontal_flip(false);
                make_anim_range(12, 144, 155); // lr_idle: 144-155
                break;
            }
            break;
        }
    }

    // PlayerMovement Implementation
    PlayerMovement::PlayerMovement() : _dx(0), _dy(0), _current_state(State::IDLE), _facing_direction(Direction::DOWN), _action_timer(0)
    {
    }

    // Player Implementation

    fe::Player::Player(bn::sprite_ptr sprite)
        : Entity(sprite), _animation(sprite), _gun_active(false)
    {
        // Set player z-order to 1 by default
        set_sprite_z_order(1);

        // Initialize hitbox size for player (16x32)
        _hitbox = Hitbox(0, 0, 16, 32);

        // Initialize health
        _healthbar.set_hp(_hp);
    }

    void Player::spawn(bn::fixed_point pos, bn::camera_ptr camera)
    {
        _healthbar.set_hp(_hp);
        set_position(pos);
        set_camera(camera);
        _animation.apply_state(_movement.current_state(), _movement.facing_direction());
    }

    // Helper function to colorize a pixel in a sprite tile
    void colorize_sprite_pixel(bn::span<bn::tile> tile_span, int x, int y, uint8_t value)
    {
        int tile_index = (x / 8) + 2 * (y / 8);
        bn::tile &tile = tile_span[tile_index];
        int pixel_y = y % 8; // Get the y position within the tile
        int pixel_x = x % 8; // Get the x position within the tile

        // In 4bpp mode, 8 pixels are stored in each 32-bit row
        // Each pixel uses 4 bits (nibble)
        uint32_t row = tile.data[pixel_y];

        int shift_amount = (7 - pixel_x) * 4; // High nibble is leftmost
        row &= ~(0xF << shift_amount);
        row |= (value & 0xF) << shift_amount;

        tile.data[pixel_y] = row;
    }


    void Player::handle_input()
    {
        // Don't process movement input when listening to NPCs
        if (_state.listening())
        {
            // Update action timer and abilities cooldowns
            _movement.update_action_timer();
            _abilities.update_cooldowns();
        
            // Check if action is finished and return to normal state
            bool was_performing_action = _movement.is_performing_action();
            if (was_performing_action && _movement.action_timer() <= 0)
            {
                _movement.stop_action();
                // Force animation update after action completes
                _animation.apply_state(_movement.current_state(), _movement.facing_direction());
            }
            if (was_performing_action && _movement.action_timer() <= 0)
            {
                _movement.stop_action();
                // Force animation update after action completes
                _animation.apply_state(_movement.current_state(), _movement.facing_direction());
            }
            return;
        }

        // Don't process most inputs while performing actions
        bool performing_action = _movement.is_performing_action();

        bool was_moving = _movement.is_moving();
        auto old_direction = _movement.facing_direction();
        auto old_state = _movement.current_state();

        // R button toggles strafe mode
        if (bn::keypad::r_pressed() && !performing_action)
        {
            _is_strafing = !_is_strafing;
            if (_is_strafing) {
                _strafing_direction = _movement.facing_direction();
            }
        }

        // L button toggles gun equip/unequip (can be done during any action)
        if (bn::keypad::l_pressed())
        {
            bool new_gun_state = !_gun_active;
            
            // Only update if state is actually changing
            if (new_gun_state != _gun_active) {
                _gun_active = new_gun_state;
                
                if (_gun_active) {
                    // Equip gun
                    if (!_gun_sprite.has_value()) {
                        _gun_sprite = bn::sprite_items::gun.create_sprite(pos().x(), pos().y());
                        _gun_sprite->set_bg_priority(get_sprite()->bg_priority());
                        _gun_sprite->set_z_order(get_sprite()->z_order() - 1);
                        if (get_sprite()->camera().has_value()) {
                            _gun_sprite->set_camera(get_sprite()->camera().value());
                            _bullet_manager.set_camera(get_sprite()->camera().value());
                        }
                        update_gun_position(_movement.facing_direction());
                    }
                } else {
                    // Unequip gun
                    _gun_sprite.reset();
                }
                
                // Debug output
                BN_LOG("Gun toggled: ", _gun_active ? "ON" : "OFF");
            }
        }

        // New ability inputs (can be performed during movement but not during other actions)
        if (!performing_action)
        {
            // B button for rolling
            if (bn::keypad::b_pressed() && _abilities.rolling_available())
            {
                _movement.start_rolling();
                _abilities.set_roll_cooldown(120); // 2 seconds cooldown
                performing_action = true;
            }
            // A button for slashing/attacking (or shooting if gun is equipped)
            else if (bn::keypad::a_pressed())
            {
                if (_gun_active) {
                    // Shoot if gun is equipped
                    PlayerMovement::Direction bullet_dir = _is_strafing ? _strafing_direction : _movement.facing_direction();
                    fire_bullet(bullet_dir);
                } else if (_abilities.slashing_available()) {
                    // Slash if no gun
                    _movement.start_slashing();
                    _abilities.set_slash_cooldown(60); // 1 second cooldown
                    performing_action = true;
                }
            }

            // Buff abilities - Select + directional
            else if (bn::keypad::select_held() && _abilities.buff_abilities_available())
            {
                if (bn::keypad::up_pressed())
                {
                    _movement.start_buff(PlayerMovement::State::HEAL_BUFF);
                    _abilities.set_buff_cooldown(300); // 5 seconds cooldown
                    performing_action = true;
                }
                else if (bn::keypad::down_pressed())
                {
                    _movement.start_buff(PlayerMovement::State::DEFENCE_BUFF);
                    _abilities.set_buff_cooldown(300);
                    performing_action = true;
                }
                else if (bn::keypad::left_pressed())
                {
                    _movement.start_buff(PlayerMovement::State::POWER_BUFF);
                    _abilities.set_buff_cooldown(300);
                    performing_action = true;
                }
                else if (bn::keypad::right_pressed())
                {
                    _movement.start_buff(PlayerMovement::State::ENERGY_BUFF);
                    _abilities.set_buff_cooldown(300);
                    performing_action = true;
                }
            }
        }

        // Movement input (only when not performing actions)
        if (!performing_action)
        {
            // Store movement input
            bool moving_right = bn::keypad::right_held();
            bool moving_left = bn::keypad::left_held();
            bool moving_up = bn::keypad::up_held();
            bool moving_down = bn::keypad::down_held();

            // Running is now toggled by R button (when not in strafe mode)
            bool should_run = !_is_strafing && _abilities.running_available();

            // Apply movement based on strafing state
            if (_is_strafing)
            {
                // When strafing, use the stored direction for facing
                // but still allow movement in all directions
                if (moving_right)
                {
                    _movement.set_dx(bn::clamp(_movement.dx() + _movement.acc_const, -_movement.max_speed, _movement.max_speed));
                }
                else if (moving_left)
                {
                    _movement.set_dx(bn::clamp(_movement.dx() - _movement.acc_const, -_movement.max_speed, _movement.max_speed));
                }

                if (moving_up)
                {
                    _movement.set_dy(bn::clamp(_movement.dy() - _movement.acc_const, -_movement.max_speed, _movement.max_speed));
                }
                else if (moving_down)
                {
                    _movement.set_dy(bn::clamp(_movement.dy() + _movement.acc_const, -_movement.max_speed, _movement.max_speed));
                }

                // Update movement state but keep facing direction
                _movement.update_movement_state();
                if (should_run && _movement.is_moving())
                {
                    _movement.start_running();
                }
            }
            else
            {
                // Normal movement - changes facing direction
                if (moving_right)
                {
                    _movement.move_right();
                }
                else if (moving_left)
                {
                    _movement.move_left();
                }

                if (moving_up)
                {
                    _movement.move_up();
                }
                else if (moving_down)
                {
                    _movement.move_down();
                }

                // Set running state after movement
                if (should_run && _movement.is_moving())
                {
                    _movement.start_running();
                }
                else if (!should_run && _movement.is_state(PlayerMovement::State::RUNNING))
                {
                    _movement.stop_running();
                }
            }
        }


        // A button handles interaction when not attacking
        if (bn::keypad::a_pressed() && !_gun_active && !_abilities.slashing_available() && !_state.listening() && !performing_action)
        {
            // TODO: Implement interaction with objects/NPCs
            // This would trigger interaction with objects in front of the player
        }

        // Update gun position if active, regardless of input
        if (_gun_active && _gun_sprite.has_value())
        {
            // Use strafing direction if strafing, otherwise use movement direction
            PlayerMovement::Direction gun_dir = _is_strafing ? _strafing_direction : _movement.facing_direction();
            update_gun_position(gun_dir);
        }

        // Apply friction when not holding movement keys and not performing action
        if (!performing_action)
        {
            _movement.apply_friction();
        }

        // Update animation if any movement state changed
        if (was_moving != _movement.is_moving() ||
            old_direction != _movement.facing_direction() ||
            old_state != _movement.current_state())
        {
            _animation.apply_state(_movement.current_state(), _movement.facing_direction());
        }
    }

    void Player::update()
    {
        // Store the previous state before handling input
        PlayerMovement::State old_state = _movement.current_state();
        PlayerMovement::Direction old_direction = _movement.facing_direction();
        bool was_performing_action = _movement.is_performing_action();

        // Update ability cooldowns
        _abilities.update_cooldowns();

        handle_input();

        // Don't update physics when listening to NPCs
        if (!_state.listening())
        {
            update_physics();
            
            // Check for ground collision
            extern fe::Level* _level; // Forward declaration
            if (_level)
            {
                if (!fe::Collision::check_hitbox_collision_with_level(get_hitbox(), pos(), fe::directions::down, *_level))
                {
                    revert_position();
                    _movement.stop_movement();
                }
            }
        }

        // Update action timer
        _movement.update_action_timer();
        
        // Check if action just completed (timer reached zero)
        if (was_performing_action && _movement.action_timer() <= 0)
        {
            // Action completed, reset to appropriate state
            _movement.stop_action();
            // Force animation update to show new state
            _animation.apply_state(_movement.current_state(), _movement.facing_direction());
        }
        // Update animation if state or direction changed
        else if (old_state != _movement.current_state() || old_direction != _movement.facing_direction())
        {
            _animation.apply_state(_movement.current_state(), _movement.facing_direction());
        }

        update_sprite_position();
        _animation.update();
        _healthbar.update();

        // Update gun position if active
        if (_gun_active && _gun_sprite.has_value())
        {
            update_gun_position(_movement.facing_direction());
        }

        // Update bullets
        update_bullets();

        // Handle player invulnerability timer
        if (_state.invulnerable())
        {
            int inv_timer = _state.inv_timer() - 1;
            _state.set_inv_timer(inv_timer);

            // Flash the player sprite when invulnerable
            if ((inv_timer / 5) % 2 == 0)
            {
                set_visible(true);
            }
            else
            {
                set_visible(false);
            }

            // Reset invulnerability when timer reaches zero
            if (inv_timer <= 0)
            {
                _state.set_invulnerable(false);
                set_visible(true);
            }
        }
    }

    void Player::update_physics()
    {
        set_position(pos() + bn::fixed_point(_movement.dx(), _movement.dy()));
    }

    void Player::set_position(bn::fixed_point new_pos)
    {
        // Call base class to update position
        Entity::set_position(new_pos);
        
        // Update hitbox position (16x32 hitbox centered on the player)
        _hitbox.set_x(new_pos.x() - 8);  // Center horizontally (16/2)
        _hitbox.set_y(new_pos.y() - 16); // Center vertically (32/2)
        
        // Update sprite position with offset
        update_sprite_position();
    }
    
    void Player::update_sprite_position()
    {
        if (auto sprite = get_sprite())
        {
            // The hero sprite needs a 13-pixel offset to be centered correctly
            // When facing left (flipped), we need to subtract this offset
            // When facing right (not flipped), we add the offset
            constexpr bn::fixed HORIZONTAL_OFFSET = 13;
            
            // Get the current position from the base class
            bn::fixed_point pos = Entity::pos();
            
            // Adjust the offset based on the sprite's horizontal flip state
            bn::fixed x_offset = sprite->horizontal_flip() ? -HORIZONTAL_OFFSET : HORIZONTAL_OFFSET;
            
            // Apply the offset to the sprite position
            sprite->set_position(pos.x() + x_offset, pos.y());
            
            // Update z-order based on Y position for proper depth sorting
            sprite->set_z_order(pos.y().integer());
        }
    }

    void Player::revert_position()
    {
        Entity::revert_position();
        // Center hitbox on player - update the Entity's hitbox
        _hitbox.set_x(pos().x() - 8);
        _hitbox.set_y(pos().y() - 16);
        // Reset velocity to prevent continued movement in the same direction
        _movement.stop_movement();
    }

    void Player::update_gun_position(PlayerMovement::Direction direction)
    {
        if (!_gun_sprite)
            return;

        const int idx = int(direction);
        constexpr bn::fixed xs[4] = {0, 0, -8, 8};
        constexpr bn::fixed ys[4] = {-6, 6, 0, 0};
        constexpr bool flips[4] = {false, false, true, false};
        // Update angles to rotate gun 90 degrees when looking up or down
        constexpr int angles[4] = {90, 270, 0, 0};

        // Adjust z-order based on direction
        // When looking up (idx 0), player should be drawn over the gun
        if (idx == 0)
        {                                 // UP direction
            set_sprite_z_order(-10);      // Player in front
            _gun_sprite->set_z_order(-5); // Gun behind
        }
        else
        {
            _gun_sprite->set_z_order(-10); // Gun in front
            set_sprite_z_order(-5);       // Player behind
        }

        _gun_sprite->set_horizontal_flip(flips[idx]);
        _gun_sprite->set_rotation_angle(angles[idx]);
        _gun_sprite->set_position(pos().x() + xs[idx], pos().y() + ys[idx]);
    }

    void Player::fire_bullet(PlayerMovement::Direction direction)
    {
        if (!_gun_active || !_gun_sprite.has_value())
        {
            return;
        }

        // Calculate bullet starting position (gun tip)
        const int idx = int(direction);
        constexpr bn::fixed bullet_offsets_x[4] = {0, 0, -12, 12};
        constexpr bn::fixed bullet_offsets_y[4] = {-12, 12, 0, 0};

        bn::fixed_point bullet_pos(
            pos().x() + bullet_offsets_x[idx],
            pos().y() + bullet_offsets_y[idx]);

        fe::Direction bullet_dir;
        switch (direction)
        {
            case PlayerMovement::Direction::UP:
                bullet_dir = fe::Direction::UP;
                break;
            case PlayerMovement::Direction::DOWN:
                bullet_dir = fe::Direction::DOWN;
                break;
            case PlayerMovement::Direction::LEFT:
                bullet_dir = fe::Direction::LEFT;
                break;
            case PlayerMovement::Direction::RIGHT:
                bullet_dir = fe::Direction::RIGHT;
                break;
            default:
                bullet_dir = fe::Direction::UP;
                break;
        }

        _bullet_manager.fire_bullet(bullet_pos, bullet_dir);
    }

    void Player::update_bullets()
    {
        _bullet_manager.update_bullets();
    }
}
