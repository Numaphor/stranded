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

extern fe::Level *_level;

namespace fe
{

    PlayerMovement::PlayerMovement() : _dx(0),
                                       _dy(0),
                                       _current_state(State::IDLE),
                                       _facing_direction(Direction::DOWN)
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
    }

    void PlayerMovement::stop_movement()
    {
        _dx = 0;
        _dy = 0;
        update_state();
    }

    void PlayerMovement::update_state()
    {
        if (bn::abs(_dx) > movement_threshold || bn::abs(_dy) > movement_threshold)
        {
            _current_state = State::WALKING;
        }
        else
        {
            _current_state = State::IDLE;
        }
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
            switch (state)
            {
            case PlayerMovement::State::DEAD:
                should_change = indexes.front() != 28;
                break;
            case PlayerMovement::State::HIT:
                should_change = indexes.front() != 26;
                break;
            case PlayerMovement::State::WALKING:
                should_change = true;
                break;
            default:
                should_change = indexes.front() >= 4;
                break;
            }
        }

        if (!should_change)
            return;

        auto make_anim = [&](int speed, int f0, int f1, int f2, int f3)
        {
            _animation = bn::create_sprite_animate_action_forever(
                _sprite, speed, bn::sprite_items::hero.tiles_item(), f0, f1, f2, f3);
        };

        switch (state)
        {
        case PlayerMovement::State::DEAD:
            make_anim(6, 28, 28, 28, 29);
            break;
        case PlayerMovement::State::HIT:
            make_anim(6, 26, 26, 26, 27);
            break;
        case PlayerMovement::State::WALKING:
            switch (direction)
            {
            case PlayerMovement::Direction::UP:
                make_anim(12, 18, 19, 20, 21);
                break;
            case PlayerMovement::Direction::DOWN:
                make_anim(12, 10, 11, 12, 13);
                break;
            case PlayerMovement::Direction::LEFT:
                _sprite.set_horizontal_flip(false);
                make_anim(12, 6, 7, 8, 9);
                break;
            case PlayerMovement::Direction::RIGHT:
                _sprite.set_horizontal_flip(true);
                make_anim(12, 6, 7, 8, 9);
                break;
            default:
                make_anim(12, 10, 11, 12, 13);
                break;
            }
            break;
        default:
            make_anim(12, 2, 3, 4, 5);
            break;
        }
    }

    // Player Implementation

    fe::Player::Player(bn::sprite_ptr sprite)
        : _sprite(sprite), _animation(bn::move(sprite)), _hitbox(0, 0, 16, 32), _gun_active(false)
    {
        // Set player z-order to 1 by default
        _sprite.set_z_order(1);

        // Set hitbox to be centered on the sprite
        _hitbox.set_x(_sprite.x() - 8);
        _hitbox.set_y(_sprite.y() - 16);

        // Initialize health
        _healthbar.set_hp(_hp);
    }

    void Player::spawn(bn::fixed_point pos, bn::camera_ptr camera)
    {
        _healthbar.set_hp(_hp);
        _pos = pos;
        _previous_pos = pos;
        _sprite.set_position(pos);
        _sprite.set_camera(camera);
        _animation.apply_state(_movement.current_state(), _movement.facing_direction());
        // Center hitbox on player
        _hitbox.set_x(pos.x() - 8);
        _hitbox.set_y(pos.y() - 16);
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

    // Bullet implementation
    Bullet::Bullet(bn::fixed_point pos, bn::fixed_point velocity, bn::camera_ptr camera, PlayerMovement::Direction direction) : _pos(pos),
                                                                                                                                _velocity(velocity),
                                                                                                                                _active(true),
                                                                                                                                _hitbox(0, 0, 2, 2), // Small 2x2 hitbox for the bullet
                                                                                                                                _lifetime(BULLET_LIFETIME)
    {
        // Create a bullet sprite using the hero sprite
        _sprite = bn::sprite_items::hero.create_sprite(_pos.x(), _pos.y(), 0);
        _sprite->set_camera(camera);
        _sprite->set_z_order(-15); // Make sure bullets are in front of other sprites

        // Make the bullet very small
        _sprite->set_scale(0.15, 0.15);

        // Set rotation based on direction
        switch (direction)
        {
        case PlayerMovement::Direction::UP:
            _sprite->set_rotation_angle(0);
            break;
        case PlayerMovement::Direction::RIGHT:
            _sprite->set_rotation_angle(270);
            break;
        case PlayerMovement::Direction::DOWN:
            _sprite->set_rotation_angle(180);
            break;
        case PlayerMovement::Direction::LEFT:
            _sprite->set_rotation_angle(90);
            break;
        default:
            // Default to right direction if unknown
            _sprite->set_rotation_angle(270);
            break;
        }

        // Set a high priority to make it appear on top of other elements
        _sprite->set_bg_priority(0);

        // Center hitbox on bullet
        _hitbox.set_x(_pos.x() - 1);
        _hitbox.set_y(_pos.y() - 1);
    }

    void Bullet::update()
    {
        if (!_active || !_sprite.has_value())
        {
            return;
        }

        // Update position based on velocity
        _pos += _velocity;

        // Update sprite position
        _sprite->set_position(_pos);

        // Update hitbox
        _hitbox.set_x(_pos.x() - 1);
        _hitbox.set_y(_pos.y() - 1);

        // Decrease lifetime
        _lifetime--;

        // Deactivate if lifetime reaches zero
        if (_lifetime <= 0)
        {
            deactivate();
            _sprite.reset();
        }
    }

    bool Bullet::check_enemy_collision(Enemy &enemy)
    {
        if (!_active)
        {
            return false;
        }

        // Get enemy hitbox
        Hitbox enemy_hitbox = enemy.get_hitbox();

        // Check if bullet hitbox intersects with enemy hitbox
        if (_hitbox.collides_with(enemy_hitbox))
        {
            // Deactivate bullet
            deactivate();
            if (_sprite.has_value())
            {
                _sprite.reset();
            }
            return true;
        }

        return false;
    }

    void Player::handle_input()
    {
        // Don't process movement input when listening to NPCs
        if (_state.listening())
        {
            // Stop any existing movement when listening
            _movement.stop_movement();
            return;
        }

        bool was_moving = _movement.is_moving();
        auto old_direction = _movement.facing_direction();
        auto old_state = _movement.current_state();

        // Check if R key is pressed to start strafing
        if (bn::keypad::r_pressed())
        {
            _is_strafing = true;
            _strafing_direction = _movement.facing_direction();
        }
        // Check if R key is released to stop strafing
        if (bn::keypad::r_released())
        {
            _is_strafing = false;
        }

        // Store movement input
        bool moving_right = bn::keypad::right_held();
        bool moving_left = bn::keypad::left_held();
        bool moving_up = bn::keypad::up_held();
        bool moving_down = bn::keypad::down_held();

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
        }

        // Check A button for gun toggle
        if (bn::keypad::a_pressed())
        {
            _gun_active = !_gun_active;

            if (_gun_active)
            {
                // Create gun sprite with correct z-order
                _gun_sprite = bn::sprite_items::gun.create_sprite(_pos.x(), _pos.y());
                // In Butano, sprites sorting takes BG priority first, then z_order.
                // Match BG priority with player sprite so z_order determines layering.
                _gun_sprite->set_bg_priority(_sprite.bg_priority());
                _gun_sprite->set_z_order(_sprite.z_order() - 1);
                if (_sprite.camera().has_value())
                {
                    _gun_sprite->set_camera(_sprite.camera().value());
                    _camera = _sprite.camera();
                }
            }
            else
            {
                // Remove gun sprite when toggled off
                if (_gun_sprite.has_value())
                {
                    _gun_sprite.reset();
                }
            }
        }

        // Check B button for firing bullets when gun is active
        if (_gun_active && bn::keypad::b_held() && _shoot_cooldown <= 0 && _camera.has_value())
        {
            // Use strafing direction if strafing, otherwise use movement direction
            PlayerMovement::Direction bullet_dir = _is_strafing ? _strafing_direction : _movement.facing_direction();
            fire_bullet(bullet_dir);
            _shoot_cooldown = SHOOT_COOLDOWN_TIME;
        }

        // Update shoot cooldown
        if (_shoot_cooldown > 0)
        {
            _shoot_cooldown--;
        }

        // Update gun position if active, regardless of input
        if (_gun_active && _gun_sprite.has_value())
        {
            // Use strafing direction if strafing, otherwise use movement direction
            PlayerMovement::Direction gun_dir = _is_strafing ? _strafing_direction : _movement.facing_direction();
            update_gun_position(gun_dir);
        }

        // Apply friction when not holding movement keys
        _movement.apply_friction();

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
        _previous_pos = _pos;
        handle_input();

        // Don't update physics when listening to NPCs
        if (!_state.listening())
        {
            update_physics();
        }

        // --- Robust collision check for all corners ---
        extern fe::Level *_level; // Assuming _level is globally accessible as in fe_scene_world.cpp
        if (_level)
        {
            if (!fe::Collision::check_hitbox_collision_with_level(_hitbox, _pos, fe::directions::down, *_level))
            {
                revert_position();
                _movement.stop_movement();
            }
        }

        _sprite.set_position(_pos);
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
                _sprite.set_visible(true);
            }
            else
            {
                _sprite.set_visible(false);
            }

            // Reset invulnerability when timer reaches zero
            if (inv_timer <= 0)
            {
                _state.set_invulnerable(false);
                _sprite.set_visible(true);
            }
        }
    }

    void Player::update_physics()
    {
        _pos += bn::fixed_point(_movement.dx(), _movement.dy());
    }

    void Player::set_position(bn::fixed_point pos)
    {
        _previous_pos = _pos;
        _pos = pos;
        _sprite.set_position(pos);
        // Center hitbox on player
        _hitbox.set_x(pos.x() - 8);
        _hitbox.set_y(pos.y() - 16);
    }

    void Player::revert_position()
    {
        // Revert to previous valid position
        _pos = _previous_pos;
        _sprite.set_position(_pos);
        // Center hitbox on player
        _hitbox.set_x(_pos.x() - 8);
        _hitbox.set_y(_pos.y() - 16);
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
            _sprite.set_z_order(-10);     // Player in front
            _gun_sprite->set_z_order(-5); // Gun behind
        }
        else
        {
            _gun_sprite->set_z_order(-10); // Gun in front
            _sprite.set_z_order(-5);       // Player behind
        }

        _gun_sprite->set_horizontal_flip(flips[idx]);
        _gun_sprite->set_rotation_angle(angles[idx]);
        _gun_sprite->set_position(_pos.x() + xs[idx], _pos.y() + ys[idx]);
    }

    void Player::fire_bullet(PlayerMovement::Direction direction)
    {
        if (!_gun_active || !_gun_sprite.has_value() || !_camera.has_value())
        {
            return;
        }

        // Calculate bullet starting position (gun tip)
        const int idx = int(direction);
        constexpr bn::fixed bullet_offsets_x[4] = {0, 0, -12, 12};
        constexpr bn::fixed bullet_offsets_y[4] = {-12, 12, 0, 0};

        bn::fixed_point bullet_pos(
            _pos.x() + bullet_offsets_x[idx],
            _pos.y() + bullet_offsets_y[idx]);

        // Calculate bullet velocity based on direction
        constexpr bn::fixed bullet_speed = 3;
        constexpr bn::fixed bullet_vx[4] = {0, 0, -bullet_speed, bullet_speed};
        constexpr bn::fixed bullet_vy[4] = {-bullet_speed, bullet_speed, 0, 0};

        bn::fixed_point bullet_velocity(bullet_vx[idx], bullet_vy[idx]);

        // Create a new bullet and add it to the bullets vector
        // First check if we have space in the vector
        if (_bullets.size() < 32)
        {
            _bullets.push_back(Bullet(bullet_pos, bullet_velocity, _camera.value(), direction));
        }
        else
        {
            // If we've reached max bullets, find an inactive one to replace
            for (int i = 0; i < _bullets.size(); ++i)
            {
                if (!_bullets[i].is_active())
                {
                    _bullets[i] = Bullet(bullet_pos, bullet_velocity, _camera.value(), direction);
                    break;
                }
            }
        }
    }

    void Player::update_bullets()
    {
        // Update all active bullets
        for (auto &bullet : _bullets)
        {
            if (bullet.is_active())
            {
                bullet.update();
            }
        }
    }
}
