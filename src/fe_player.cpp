#include "fe_player.h"
#include "fe_player_companion.h"
#include "bn_keypad.h"
#include "bn_sprite_items_hero.h"
#include "bn_sprite_items_gun.h"
#include "bn_sprite_items_companion.h"
#include "bn_sprite_animate_actions.h"
#include "fe_level.h"
#include "fe_collision.h"
#include "fe_bullet_manager.h"
#include "fe_sprite_priority.h"

namespace fe
{
    extern Level *_level;

    namespace player_constants
    {
        constexpr bn::fixed ROLL_SPEED = 1.2;
        constexpr bn::fixed HORIZONTAL_OFFSET = 13;
        constexpr bn::fixed ATTACK_REACH = 20;
        constexpr bn::fixed HITBOX_WIDTH = 16;
        constexpr bn::fixed HITBOX_HEIGHT = 32;

        constexpr bn::fixed GUN_OFFSET_X[4] = {0, 0, -8, 8};
        constexpr bn::fixed GUN_OFFSET_Y[4] = {-6, 6, 0, 0};
        constexpr bn::fixed BULLET_OFFSET_X[4] = {1, -1, -12, 11};
        constexpr bn::fixed BULLET_OFFSET_Y[4] = {-9, 9, -3, 1};
        constexpr bool GUN_FLIPS[4] = {false, false, true, false};
        constexpr int GUN_ANGLES[4] = {90, 270, 0, 0};
    }

    // Direction utility functions
    namespace direction_utils
    {
        bn::fixed_point get_roll_offset(PlayerMovement::Direction dir)
        {
            switch (dir)
            {
            case PlayerMovement::Direction::UP:
                return {0, -player_constants::ROLL_SPEED};
            case PlayerMovement::Direction::DOWN:
                return {0, player_constants::ROLL_SPEED};
            case PlayerMovement::Direction::LEFT:
                return {-player_constants::ROLL_SPEED, 0};
            case PlayerMovement::Direction::RIGHT:
                return {player_constants::ROLL_SPEED, 0};
            default:
                return {0, 0};
            }
        }

        void setup_gun(bn::sprite_ptr &gun_sprite, PlayerMovement::Direction dir, bn::fixed_point pos)
        {
            const int idx = int(dir);
            gun_sprite.set_horizontal_flip(player_constants::GUN_FLIPS[idx]);
            gun_sprite.set_rotation_angle(player_constants::GUN_ANGLES[idx]);
            gun_sprite.set_position(
                pos.x() + player_constants::GUN_OFFSET_X[idx],
                pos.y() + player_constants::GUN_OFFSET_Y[idx]);
        }

        bn::fixed_point get_bullet_position(PlayerMovement::Direction dir, bn::fixed_point pos)
        {
            const int idx = int(dir);
            return {pos.x() + player_constants::BULLET_OFFSET_X[idx],
                    pos.y() + player_constants::BULLET_OFFSET_Y[idx]};
        }

        Hitbox get_attack_hitbox(PlayerMovement::Direction dir, bn::fixed_point pos)
        {
            bn::fixed_point base_pos(pos.x() - player_constants::HITBOX_WIDTH / 2,
                                     pos.y() - player_constants::HITBOX_HEIGHT / 2);

            switch (dir)
            {
            case PlayerMovement::Direction::UP:
                return Hitbox(base_pos.x(), base_pos.y() - player_constants::ATTACK_REACH,
                              player_constants::HITBOX_WIDTH, player_constants::HITBOX_HEIGHT + player_constants::ATTACK_REACH);
            case PlayerMovement::Direction::DOWN:
                return Hitbox(base_pos.x(), base_pos.y(),
                              player_constants::HITBOX_WIDTH, player_constants::HITBOX_HEIGHT + player_constants::ATTACK_REACH);
            case PlayerMovement::Direction::LEFT:
                return Hitbox(base_pos.x() - player_constants::ATTACK_REACH, base_pos.y(),
                              player_constants::HITBOX_WIDTH + player_constants::ATTACK_REACH, player_constants::HITBOX_HEIGHT);
            case PlayerMovement::Direction::RIGHT:
                return Hitbox(base_pos.x(), base_pos.y(),
                              player_constants::HITBOX_WIDTH + player_constants::ATTACK_REACH, player_constants::HITBOX_HEIGHT);
            default:
                return Hitbox(base_pos.x(), base_pos.y(), player_constants::HITBOX_WIDTH, player_constants::HITBOX_HEIGHT);
            }
        }
    }

    // PlayerMovement Implementation
    PlayerMovement::PlayerMovement() : _dx(0), _dy(0), _current_state(State::IDLE), _facing_direction(Direction::DOWN), _action_timer(0)
    {
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

    // PlayerAnimation Implementation
    PlayerAnimation::PlayerAnimation(bn::sprite_ptr sprite) : _sprite(sprite), _last_state(PlayerMovement::State::IDLE), _last_direction(PlayerMovement::Direction::DOWN)
    {
    }

    void PlayerAnimation::apply_state(PlayerMovement::State state, PlayerMovement::Direction direction)
    {
        if (!should_change_animation(state, direction))
            return;

        _sprite.set_horizontal_flip(direction == PlayerMovement::Direction::LEFT);

        // Simplified animation data: speed, base_frame, frame_count
        struct AnimData
        {
            int speed;
            int up_start;
            int down_start;
            int side_start;
            int frame_count;
        };

        static const AnimData animations[] = {
            {12, 187, 0, 144, 12},  // IDLE
            {12, 199, 109, 156, 8}, // WALKING
            {8, 207, 117, 164, 8},  // RUNNING
            {8, 226, 136, 172, 8},  // ROLLING
            {8, 219, 129, 178, 7},  // SLASHING
            {8, 219, 129, 182, 5},  // ATTACKING
            {10, 215, 125, 178, 4}, // CHOPPING
            {4, 13, 13, 13, 24},    // HEAL_BUFF
            {4, 37, 37, 37, 24},    // DEFENCE_BUFF
            {4, 61, 61, 61, 24},    // POWER_BUFF
            {4, 85, 85, 85, 24},    // ENERGY_BUFF
            {6, 234, 234, 234, 13}  // DEAD
        };

        int state_idx = static_cast<int>(state);
        if (state_idx >= 12)
            return;

        const auto &anim = animations[state_idx];
        int start_frame = (direction == PlayerMovement::Direction::UP) ? anim.up_start : (direction == PlayerMovement::Direction::DOWN) ? anim.down_start
                                                                                                                                        : anim.side_start;

        make_anim_range(anim.speed, start_frame, start_frame + anim.frame_count - 1);

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
            _sprite, speed, bn::sprite_items::hero.tiles_item(),
            bn::span<const uint16_t>(frames.data(), frames.size()));
    }

    void PlayerAnimation::update()
    {
        if (_animation.has_value())
        {
            _animation->update();
        }
    }

    // Player Implementation
    Player::Player(bn::sprite_ptr sprite) : Entity(sprite), _animation(sprite), _gun_active(false)
    {
        set_sprite_z_order(1);
        _hitbox = Hitbox(0, 0, player_constants::HITBOX_WIDTH, player_constants::HITBOX_HEIGHT);
        _healthbar.set_hp(_hp);
    }

    void Player::spawn(bn::fixed_point pos, bn::camera_ptr camera)
    {
        _healthbar.set_hp(_hp);
        set_position(pos);
        set_camera(camera);
        initialize_companion(camera);
        update_animation();
    }

    void Player::handle_input()
    {
        if (_state.listening())
        {
            return;
        }

        bool performing_action = _movement.is_performing_action();

        // Toggle controls
        if (bn::keypad::r_pressed() && !performing_action)
        {
            _is_strafing = !_is_strafing;
            if (_is_strafing)
                _strafing_direction = _movement.facing_direction();
        }

        if (bn::keypad::l_pressed())
            toggle_gun();

        // Action inputs (consolidated)
        if (!performing_action)
        {
            if (bn::keypad::b_pressed() && _abilities.rolling_available())
            {
                _movement.start_action(PlayerMovement::State::ROLLING, 64);
                _abilities.set_roll_cooldown(64);
            }
            else if (bn::keypad::a_pressed() && _state.dialog_cooldown() == 0)
            {
                if (_gun_active)
                {
                    fire_bullet(_is_strafing ? _strafing_direction : _movement.facing_direction());
                }
                else if (_abilities.slashing_available())
                {
                    _movement.start_action(PlayerMovement::State::SLASHING, 25);
                    _abilities.set_slash_cooldown(60);
                }
            }
            else if (bn::keypad::select_held() && _abilities.buff_abilities_available())
            {
                // Buff inputs (consolidated)
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
                    _movement.start_action(buff_state, 96);
                    _abilities.set_buff_cooldown(96);
                }
            }
        }

        // Movement inputs (consolidated)
        if (!performing_action)
        {
            bool should_run = !_is_strafing && _abilities.running_available();

            if (_is_strafing)
            {
                // Strafe movement
                bn::fixed dx = _movement.dx();
                bn::fixed dy = _movement.dy();

                if (bn::keypad::right_held())
                    dx = bn::clamp(dx + PlayerMovement::acc_const, -PlayerMovement::max_speed, PlayerMovement::max_speed);
                else if (bn::keypad::left_held())
                    dx = bn::clamp(dx - PlayerMovement::acc_const, -PlayerMovement::max_speed, PlayerMovement::max_speed);

                if (bn::keypad::up_held())
                    dy = bn::clamp(dy - PlayerMovement::acc_const, -PlayerMovement::max_speed, PlayerMovement::max_speed);
                else if (bn::keypad::down_held())
                    dy = bn::clamp(dy + PlayerMovement::acc_const, -PlayerMovement::max_speed, PlayerMovement::max_speed);

                _movement.set_dx(dx);
                _movement.set_dy(dy);
                _movement.update_movement_state();
            }
            else
            {
                // Normal movement
                if (bn::keypad::right_held())
                    _movement.move_direction(PlayerMovement::Direction::RIGHT);
                else if (bn::keypad::left_held())
                    _movement.move_direction(PlayerMovement::Direction::LEFT);

                if (bn::keypad::up_held())
                    _movement.move_direction(PlayerMovement::Direction::UP);
                else if (bn::keypad::down_held())
                    _movement.move_direction(PlayerMovement::Direction::DOWN);
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
            _gun_sprite = bn::sprite_items::gun.create_sprite(pos().x(), pos().y());
            _gun_sprite->set_bg_priority(get_sprite()->bg_priority());
            _gun_sprite->set_z_order(get_sprite()->z_order() - 1); // Gun in front of player
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

    void Player::update()
    {
        auto old_state = _movement.current_state();
        auto old_direction = _movement.facing_direction();
        bool was_performing_action = _movement.is_performing_action();

        // Update timers
        _abilities.update_cooldowns();
        _state.update_dialog_cooldown();
        _movement.update_action_timer();

        handle_input();

        if (!_state.listening())
        {
            // Update physics
            bn::fixed_point new_pos = pos() + bn::fixed_point(_movement.dx(), _movement.dy());
            if (_movement.current_state() == PlayerMovement::State::ROLLING)
            {
                new_pos += direction_utils::get_roll_offset(_movement.facing_direction());
            }
            set_position(new_pos);

            // Check collision
            if (fe::_level && !Collision::check_hitbox_collision_with_level(get_hitbox(), pos(), fe::directions::down, *fe::_level))
            {
                revert_position();
                _movement.stop_movement();
            }
        }

        // Handle action completion
        if (was_performing_action && _movement.action_timer() <= 0)
        {
            _movement.stop_action();
            update_animation();
        }

        // Update animation if changed
        if (old_state != _movement.current_state() || old_direction != _movement.facing_direction())
        {
            update_animation();
        }

        // Update components
        update_sprite_position();
        _animation.update();
        _healthbar.update();
        update_bullets();

        // Handle invulnerability
        if (_state.invulnerable())
        {
            int inv_timer = _state.inv_timer() - 1;
            _state.set_inv_timer(inv_timer);
            set_visible((inv_timer / 5) % 2 == 0);

            if (inv_timer <= 0)
            {
                _state.set_invulnerable(false);
                set_visible(true);
            }
        }

        // Update companion
        if (_companion.has_value())
        {
            _companion->update(pos(), _hp <= 0);
            _companion->set_visible(_state.invulnerable() ? get_sprite()->visible() : true);
        }
    }

    void Player::set_position(bn::fixed_point new_pos)
    {
        Entity::set_position(new_pos);

        bn::fixed_point hitbox_pos = Hitbox::calculate_centered_position(new_pos,
                                                                         player_constants::HITBOX_WIDTH, player_constants::HITBOX_HEIGHT);
        _hitbox.set_x(hitbox_pos.x());
        _hitbox.set_y(hitbox_pos.y());

        update_sprite_position();
    }

    void Player::update_sprite_position()
    {
        if (auto sprite = get_sprite())
        {
            bn::fixed_point pos = Entity::pos();
            bn::fixed x_offset = sprite->horizontal_flip() ? -player_constants::HORIZONTAL_OFFSET : player_constants::HORIZONTAL_OFFSET;
            sprite->set_position(pos.x() + x_offset, pos.y());
        }
    }

    void Player::update_gun_position(PlayerMovement::Direction direction)
    {
        if (!_gun_sprite)
            return;

        direction_utils::setup_gun(*_gun_sprite, direction, pos());
    }

    void Player::fire_bullet(PlayerMovement::Direction direction)
    {
        if (!_gun_active || !_gun_sprite.has_value())
            return;

        bn::fixed_point bullet_pos = direction_utils::get_bullet_position(direction, pos());
        Direction bullet_dir = static_cast<Direction>(int(direction));
        _bullet_manager.fire_bullet(bullet_pos, bullet_dir);
    }

    Hitbox Player::get_attack_hitbox() const
    {
        if (!is_attacking())
            return get_hitbox();

        return direction_utils::get_attack_hitbox(_movement.facing_direction(), pos());
    }

    void Player::initialize_companion(bn::camera_ptr camera)
    {
        if (_companion_initialized)
            return;

        bn::sprite_ptr companion_sprite = bn::sprite_items::companion.create_sprite(pos());

        // Ensure companion has same bg_priority as player
        if (auto player_sprite = get_sprite())
        {
            companion_sprite.set_bg_priority(player_sprite->bg_priority());
        }

        _companion = PlayerCompanion(bn::move(companion_sprite));
        _companion->spawn(pos(), camera);
        _companion->set_flying(true);
        _companion_initialized = true;
    }

    // Additional Player method implementations
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
            _gun_sprite->set_z_order(z_order - 1); // Gun in front of player
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

    void Player::update_bullets()
    {
        _bullet_manager.update_bullets();
    }
}