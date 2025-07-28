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
        // HORIZONTAL_OFFSET removed - new 32x32 sprite tightly fits around player
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

        int get_gun_z_offset(PlayerMovement::Direction dir)
        {
            // Gun should be behind player when looking up, in front for all other directions
            return (dir == PlayerMovement::Direction::UP) ? 1 : -1;
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
            {12, 187, 12, 0, 13, 144, 12}, // IDLE: idle_up(187-198=12), idle_down(0-12=13), lr_idle(144-155=12)
            {12, 199, 8, 109, 8, 156, 8},  // WALKING: move_up(199-206=8), move_down(109-116=8), lr_move(156-163=8)
            {8, 207, 8, 117, 8, 164, 8},   // RUNNING: run_up(207-214=8), run_down(117-124=8), lr_run(164-171=8)
            {8, 226, 8, 136, 8, 172, 6},   // ROLLING: roll_up(226-233=8), roll_down(136-143=8), lr_roll(172-177=6)
            {8, 219, 7, 129, 7, 178, 4},   // SLASHING: attack_up(219-225=7), slash_down(129-135=7), lr_slash(178-181=4)
            {8, 219, 7, 129, 7, 182, 5},   // ATTACKING: attack_up(219-225=7), slash_down(129-135=7), lr_slash(182-186=5)
            {10, 215, 4, 125, 4, 178, 4},  // CHOPPING: chop_up(215-218=4), chop_down(125-128=4), lr_slash(178-181=4)
            {4, 13, 24, 13, 24, 13, 24},   // HEAL_BUFF: heal_buff(13-36=24) all directions
            {4, 37, 24, 37, 24, 37, 24},   // DEFENCE_BUFF: defence_buff(37-60=24) all directions
            {4, 61, 24, 61, 24, 61, 24},   // POWER_BUFF: power_buff(61-84=24) all directions
            {4, 85, 24, 85, 24, 85, 24},   // ENERGY_BUFF: energy_buff(85-108=24) all directions
            {6, 234, 13, 234, 13, 234, 13} // DEAD: death(234-246=13) all directions
        };

        int state_idx = static_cast<int>(state);
        if (state_idx >= 12)
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

        make_anim_range(anim.speed, start_frame, start_frame + frame_count - 1);

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
        // Set player bg_priority to 1 to allow going behind sword backgrounds (priority 0)
        // Both player and companion need same bg_priority for z_order comparison to work
        if (auto player_sprite = get_sprite())
        {
            player_sprite->set_bg_priority(1);
        }
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

            // Set initial z_order based on facing direction
            PlayerMovement::Direction gun_dir = _is_strafing ? _strafing_direction : _movement.facing_direction();
            int gun_z_offset = direction_utils::get_gun_z_offset(gun_dir);
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
        update_z_order();
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
            // No offset needed since the new 32x32 sprite tightly fits around the player
            sprite->set_position(pos.x(), pos.y());
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

        // Set companion bg_priority to 0 to ensure it NEVER goes behind the sword background
        // The sword background uses priorities 0-2, so companion at priority 0
        // will always be visible (sprites cover backgrounds of same priority)
        companion_sprite.set_bg_priority(0);

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

    void Player::update_bullets()
    {
        _bullet_manager.update_bullets();
    }
}