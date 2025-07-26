#include "fe_player.h"
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

    void PlayerMovement::start_running()
    {
        if (_current_state == State::WALKING)
        {
            _current_state = State::RUNNING;
        }
    }

    void PlayerMovement::stop_running()
    {
        if (_current_state == State::RUNNING)
        {
            _current_state = State::WALKING;
        }
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
        {
            _current_state = State::WALKING;
        }
        else if (!is_moving && (_current_state == State::WALKING || _current_state == State::RUNNING))
        {
            _current_state = State::IDLE;
        }
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
    PlayerAnimation::PlayerAnimation(bn::sprite_ptr sprite) : _sprite(sprite)
    {
    }

    void PlayerAnimation::apply_state(PlayerMovement::State state, PlayerMovement::Direction direction)
    {
        if (!should_change_animation(state, direction))
            return;

        _sprite.set_horizontal_flip(direction == PlayerMovement::Direction::LEFT);

        // Animation frame mappings
        struct AnimData
        {
            int speed;
            int start;
            int end;
        };
        static const AnimData animations[12][4] = {
            // IDLE: UP, DOWN, LEFT, RIGHT (fixed order to match enum)
            {{12, 187, 198}, {12, 0, 12}, {12, 144, 155}, {12, 144, 155}},
            // WALKING
            {{12, 199, 206}, {12, 109, 116}, {12, 156, 163}, {12, 156, 163}},
            // RUNNING
            {{8, 207, 214}, {8, 117, 124}, {8, 164, 171}, {8, 164, 171}},
            // ROLLING
            {{8, 226, 233}, {8, 136, 143}, {8, 172, 177}, {8, 172, 177}},
            // SLASHING
            {{8, 219, 225}, {8, 129, 135}, {8, 178, 181}, {8, 178, 181}},
            // ATTACKING
            {{8, 219, 225}, {8, 129, 135}, {8, 182, 186}, {8, 182, 186}},
            // CHOPPING
            {{10, 215, 218}, {10, 125, 128}, {10, 178, 181}, {10, 178, 181}},
            // HEAL_BUFF
            {{4, 13, 36}, {4, 13, 36}, {4, 13, 36}, {4, 13, 36}},
            // DEFENCE_BUFF
            {{4, 37, 60}, {4, 37, 60}, {4, 37, 60}, {4, 37, 60}},
            // POWER_BUFF
            {{4, 61, 84}, {4, 61, 84}, {4, 61, 84}, {4, 61, 84}},
            // ENERGY_BUFF
            {{4, 85, 108}, {4, 85, 108}, {4, 85, 108}, {4, 85, 108}},
            // DEAD
            {{6, 234, 246}, {6, 234, 246}, {6, 234, 246}, {6, 234, 246}}};

        int state_idx = static_cast<int>(state);
        int dir_idx = static_cast<int>(direction);

        if (state_idx < 12 && dir_idx < 4)
        {
            const AnimData &anim = animations[state_idx][dir_idx];
            make_anim_range(anim.speed, anim.start, anim.end);
        }
    }

    bool PlayerAnimation::should_change_animation(PlayerMovement::State state, PlayerMovement::Direction direction)
    {
        if (!_animation.has_value())
            return true;

        // Check if direction flip changed
        bool flip_changed = _sprite.horizontal_flip() != (direction == PlayerMovement::Direction::LEFT);

        // Check if we need to change animation based on state
        // This is a simplified check - in a full implementation, you'd track current state
        static PlayerMovement::State _last_state = PlayerMovement::State::IDLE;
        static PlayerMovement::Direction _last_direction = PlayerMovement::Direction::DOWN;

        bool state_changed = (_last_state != state);
        bool direction_changed = (_last_direction != direction);

        _last_state = state;
        _last_direction = direction;

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
            update_timers();
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

        // Action inputs
        if (!performing_action)
        {
            handle_action_inputs();
        }

        // Movement inputs
        if (!performing_action)
        {
            handle_movement_inputs();
        }

        update_gun_if_active();
        _movement.apply_friction();
    }

    void Player::handle_action_inputs()
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
            handle_buff_inputs();
        }
    }

    void Player::handle_buff_inputs()
    {
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

    void Player::handle_movement_inputs()
    {
        bool should_run = !_is_strafing && _abilities.running_available();

        if (_is_strafing)
        {
            handle_strafe_movement();
        }
        else
        {
            handle_normal_movement();
        }

        if (should_run && _movement.is_moving())
        {
            _movement.start_running();
        }
        else if (!should_run && _movement.is_state(PlayerMovement::State::RUNNING))
        {
            _movement.stop_running();
        }
    }

    void Player::handle_normal_movement()
    {
        if (bn::keypad::right_held())
            _movement.move_direction(PlayerMovement::Direction::RIGHT);
        else if (bn::keypad::left_held())
            _movement.move_direction(PlayerMovement::Direction::LEFT);

        if (bn::keypad::up_held())
            _movement.move_direction(PlayerMovement::Direction::UP);
        else if (bn::keypad::down_held())
            _movement.move_direction(PlayerMovement::Direction::DOWN);
    }

    void Player::handle_strafe_movement()
    {
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
        _movement.update_movement_state(); // Make sure state is updated for strafe movement too
    }

    void Player::toggle_gun()
    {
        _gun_active = !_gun_active;

        if (_gun_active && !_gun_sprite.has_value())
        {
            _gun_sprite = bn::sprite_items::gun.create_sprite(pos().x(), pos().y());
            _gun_sprite->set_bg_priority(get_sprite()->bg_priority());
            _gun_sprite->set_z_order(get_sprite()->z_order() - 1);
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

        update_timers();
        handle_input();

        if (!_state.listening())
        {
            update_physics();
            check_collision();
        }

        update_action_completion(was_performing_action);
        update_animation_if_changed(old_state, old_direction);
        update_components();
        handle_invulnerability();
        update_companion();
    }

    void Player::update_timers()
    {
        _abilities.update_cooldowns();
        _state.update_dialog_cooldown();
        _movement.update_action_timer();
    }

    void Player::update_physics()
    {
        bn::fixed_point new_pos = pos() + bn::fixed_point(_movement.dx(), _movement.dy());

        if (_movement.current_state() == PlayerMovement::State::ROLLING)
        {
            bn::fixed_point roll_offset = get_roll_offset();
            new_pos += roll_offset;
        }

        set_position(new_pos);
    }

    bn::fixed_point Player::get_roll_offset() const
    {
        switch (_movement.facing_direction())
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

    void Player::check_collision()
    {
        if (_level && !Collision::check_hitbox_collision_with_level(get_hitbox(), pos(), directions::down, *_level))
        {
            revert_position();
            _movement.stop_movement();
        }
    }

    void Player::update_action_completion(bool was_performing_action)
    {
        if (was_performing_action && _movement.action_timer() <= 0)
        {
            _movement.stop_action();
            update_animation();
        }
    }

    void Player::update_animation_if_changed(PlayerMovement::State old_state, PlayerMovement::Direction old_direction)
    {
        if (old_state != _movement.current_state() || old_direction != _movement.facing_direction())
        {
            update_animation();
        }
    }

    void Player::update_components()
    {
        update_sprite_position();
        _animation.update();
        _healthbar.update();
        update_bullets();
    }

    void Player::handle_invulnerability()
    {
        if (!_state.invulnerable())
            return;

        int inv_timer = _state.inv_timer() - 1;
        _state.set_inv_timer(inv_timer);

        set_visible((inv_timer / 5) % 2 == 0);

        if (inv_timer <= 0)
        {
            _state.set_invulnerable(false);
            set_visible(true);
        }
    }

    void Player::update_companion()
    {
        if (!_companion.has_value())
            return;

        _companion->update(pos(), _hp <= 0);
        _companion->set_visible(_state.invulnerable() ? get_sprite()->visible() : true);
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

        const int idx = int(direction);
        _gun_sprite->set_horizontal_flip(player_constants::GUN_FLIPS[idx]);
        _gun_sprite->set_rotation_angle(player_constants::GUN_ANGLES[idx]);
        _gun_sprite->set_position(
            pos().x() + player_constants::GUN_OFFSET_X[idx],
            pos().y() + player_constants::GUN_OFFSET_Y[idx]);
    }

    void Player::fire_bullet(PlayerMovement::Direction direction)
    {
        if (!_gun_active || !_gun_sprite.has_value())
            return;

        const int idx = int(direction);
        bn::fixed_point bullet_pos(
            pos().x() + player_constants::BULLET_OFFSET_X[idx],
            pos().y() + player_constants::BULLET_OFFSET_Y[idx]);

        Direction bullet_dir = static_cast<Direction>(idx);
        _bullet_manager.fire_bullet(bullet_pos, bullet_dir);
    }

    Hitbox Player::get_attack_hitbox() const
    {
        if (!is_attacking())
            return get_hitbox();

        bn::fixed_point player_pos = pos();
        PlayerMovement::Direction facing_dir = _movement.facing_direction();

        bn::fixed_point base_pos(player_pos.x() - player_constants::HITBOX_WIDTH / 2,
                                 player_pos.y() - player_constants::HITBOX_HEIGHT / 2);

        switch (facing_dir)
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
            return get_hitbox();
        }
        return get_hitbox();
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

    // PlayerCompanion Implementation (simplified)
    PlayerCompanion::PlayerCompanion(bn::sprite_ptr sprite)
        : _sprite(bn::move(sprite)), _position(0, 0), _position_side(Position::RIGHT), _is_dead(false),
          _follow_delay(0), _target_offset(24, 0)
    {
        _sprite.set_z_order(0);
    }

    void PlayerCompanion::update(bn::fixed_point player_pos, bool player_is_dead)
    {
        if (player_is_dead != _is_dead)
        {
            _is_dead = player_is_dead;
            update_animation();
        }

        if (!_is_dead)
        {
            update_position(player_pos);
        }

        if (_animation && !_animation->done())
        {
            _animation->update();
        }
    }

    void PlayerCompanion::update_position(bn::fixed_point player_pos)
    {
        // Calculate target position based on current side
        bn::fixed_point target_pos = player_pos + _target_offset;

        // Calculate distance to target position
        bn::fixed_point diff = target_pos - _position;
        bn::fixed distance = bn::sqrt(diff.x() * diff.x() + diff.y() * diff.y());

        // Check if player is approaching the companion
        bn::fixed_point player_to_companion = _position - player_pos;
        bn::fixed distance_to_companion = bn::sqrt(player_to_companion.x() * player_to_companion.x() +
                                                   player_to_companion.y() * player_to_companion.y());

        // If player is very close to companion (within 30 units), wait for them to pass
        bool player_approaching = distance_to_companion < 30;

        // Only move if not waiting for player to pass
        if (!player_approaching && distance > 1) // Very small threshold to almost always follow
        {
            // Calculate normalized direction
            bn::fixed_point normalized_diff = diff / distance;

            // Base movement speed that scales with distance
            bn::fixed movement_speed = (distance * 0.08 < 1.2) ? distance * 0.08 : 1.2; // Cap max speed

            // Minimum speed to prevent stopping when close
            movement_speed = (movement_speed > 0.3) ? movement_speed : 0.3;

            // Apply smooth movement
            _position += normalized_diff * movement_speed;
        }

        // Check if we need to recalculate which side to be on
        // Only do this when companion is reasonably far and stable
        bn::fixed distance_to_player = bn::sqrt(player_to_companion.x() * player_to_companion.x() +
                                                player_to_companion.y() * player_to_companion.y());

        // Use a consistent low threshold for responsive switching in all cases
        bn::fixed switch_threshold = 15; // Low threshold for fast switching

        // Only recalculate side if companion is far enough from player
        if (distance_to_player > switch_threshold)
        {
            Position new_side = _position_side;

            // Determine side based on where companion currently is relative to player
            bn::fixed abs_x = bn::abs(player_to_companion.x());
            bn::fixed abs_y = bn::abs(player_to_companion.y());

            if (abs_x > abs_y + 10) // Increased bias to make side switching less sensitive
            {
                // Companion is more to the side than above/below
                new_side = player_to_companion.x() > 0 ? Position::RIGHT : Position::LEFT;
            }
            else if (player_to_companion.y() > 15) // Increased threshold
            {
                // Companion is clearly below player
                new_side = Position::BELOW;
            }
            else if (player_to_companion.y() < -15) // Player is below companion
            {
                // When player is below companion, choose left or right side based on X offset
                // This prevents the companion from getting stuck above the player
                new_side = player_to_companion.x() >= 0 ? Position::RIGHT : Position::LEFT;
            }

            // If companion is too close to center, keep current side
            set_position_side(new_side);
        }

        // Update the sprite position
        _sprite.set_position(_position);
    }

    bn::fixed_point PlayerCompanion::calculate_companion_offset() const
    {
        switch (_position_side)
        {
        case Position::RIGHT:
            return {16, 0}; // Right side of player (closer)
        case Position::LEFT:
            return {-16, 0}; // Left side of player (closer)
        case Position::BELOW:
            return {0, 12}; // Below player (closer)
        default:
            return {16, 0};
        }
    }

    void PlayerCompanion::update_animation()
    {
        if (_is_dead)
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
            _gun_sprite->set_z_order(z_order + 1);
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

    // PlayerCompanion method implementations
    void PlayerCompanion::spawn(bn::fixed_point pos, bn::camera_ptr camera)
    {
        // Start 16 pixels up and 16 pixels to the right of the player
        _position = pos + bn::fixed_point(16, -16);
        _target_offset = calculate_companion_offset();
        _sprite.set_camera(camera);
        update_animation();
    }

    void PlayerCompanion::set_flying(bool flying)
    {
        _is_flying = flying;
        update_animation();
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
}