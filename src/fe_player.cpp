#include "fe_player.h"
#include "fe_constants.h"
#include "fe_level.h"
#include "fe_collision.h"
#include "fe_direction_utils.h"

#include "bn_keypad.h"
#include "bn_log.h"
#include "bn_sound_items.h"
#include "bn_sprite_items_hero_sword.h"
#include "bn_sprite_items_hero_vfx.h"
#include "bn_sprite_items_companion.h"
#include "bn_sprite_items_companion_load.h"
#include "bn_sprite_items_gun.h"
#include "common_variable_8x8_sprite_font.h"
#include "bn_sprite_text_generator.h"

namespace fe {

    namespace {
        static int shared_gun_frame = 0;
        static int shared_sword_frame = 0;
    }

    // =========================================================================
    // Direction Utils Overloads (PlayerMovement::Direction)
    // =========================================================================

    namespace direction_utils {
        bn::fixed_point get_roll_offset(PlayerMovement::Direction dir, int frames_remaining, int total_frames) {
            bn::fixed momentum_factor = bn::fixed(frames_remaining) / bn::fixed(total_frames);
            momentum_factor = (momentum_factor * 0.7) + 0.3;
            bn::fixed current_speed = PLAYER_ROLL_SPEED * momentum_factor;
            switch (dir) {
            case PlayerMovement::Direction::UP: return bn::fixed_point(0, -current_speed);
            case PlayerMovement::Direction::DOWN: return bn::fixed_point(0, current_speed);
            case PlayerMovement::Direction::LEFT: return bn::fixed_point(-current_speed, 0);
            case PlayerMovement::Direction::RIGHT: return bn::fixed_point(current_speed, 0);
            default:
                return bn::fixed_point(0, 0); 
            }
        }

        int get_gun_z_offset(PlayerMovement::Direction dir) {
            switch (dir) {
            case PlayerMovement::Direction::UP: return 5;
            case PlayerMovement::Direction::DOWN: return -5;
            case PlayerMovement::Direction::LEFT:
            case PlayerMovement::Direction::RIGHT: return 0;
            default:
                return 0; 
            }
        }

        void setup_gun(bn::sprite_ptr &gun_sprite, Direction dir, bn::fixed_point pos) {
            const int idx = int(dir);
            gun_sprite.set_horizontal_flip(player_constants::GUN_FLIPS[idx]);
            gun_sprite.set_rotation_angle(player_constants::GUN_ANGLES[idx]);
            gun_sprite.set_position(
                pos.x() + player_constants::GUN_OFFSET_X[idx],
                pos.y() + player_constants::GUN_OFFSET_Y[idx] + PLAYER_SPRITE_Y_OFFSET);
        }
    }

    // =========================================================================
    // PlayerAbilities Implementation
    // =========================================================================

    void PlayerAbilities::update_cooldowns() {
        if (_roll_cooldown > 0) _roll_cooldown--;
        if (_chop_cooldown > 0) _chop_cooldown--;
        if (_slash_cooldown > 0) _slash_cooldown--;
        if (_buff_cooldown > 0) _buff_cooldown--; 
    }

    void PlayerAbilities::reset() {
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

    // =========================================================================
    // PlayerAnimation Implementation
    // =========================================================================

    PlayerAnimation::PlayerAnimation(bn::sprite_ptr sprite) 
        : _sprite(sprite), _last_state(PlayerMovement::State::IDLE), _last_direction(PlayerMovement::Direction::DOWN) {}

    void PlayerAnimation::apply_state(PlayerMovement::State state, PlayerMovement::Direction direction) {
        if (!should_change_animation(state, direction))
            return;
        
        _sprite.set_horizontal_flip(direction == PlayerMovement::Direction::LEFT);
        
        struct AnimData {
            int speed;
            int up_start;
            int up_count;
            int down_start;
            int down_count;
            int side_start;
            int side_count;
        };
        
        static const AnimData animations[] = {
            {12, 384, 12, 0, 12, 240, 12},
            {5, 408, 8, 120, 8, 264, 8},
            {8, 432, 8, 144, 8, 288, 8},
            {8, 504, 8, 216, 8, 312, 6},
            {8, 480, 7, 192, 7, 336, 4},
            {8, 480, 7, 192, 7, 360, 5},
            {10, 456, 4, 168, 4, 336, 4},
            {4, 24, 24, 24, 24, 24, 24},
            {4, 48, 24, 48, 24, 48, 24},
            {4, 72, 24, 72, 24, 72, 24},
            {4, 96, 24, 96, 24, 96, 24},
            {6, 0, 13, 0, 13, 0, 13},
            {15, 528, 13, 528, 13, 528, 13}
        };
        
        int state_idx = static_cast<int>(state);
        constexpr int NUM_PLAYER_STATES = sizeof(animations) / sizeof(animations[0]);
        if (state_idx >= NUM_PLAYER_STATES)
            return;
            
        const auto &anim = animations[state_idx];
        int start_frame, frame_count;
        
        if (direction == PlayerMovement::Direction::UP) {
            start_frame = anim.up_start;
            frame_count = anim.up_count; 
        } else if (direction == PlayerMovement::Direction::DOWN) {
            start_frame = anim.down_start;
            frame_count = anim.down_count; 
        } else {
            start_frame = anim.side_start;
            frame_count = anim.side_count; 
        }
        
        if (state == PlayerMovement::State::DEAD ||
            state == PlayerMovement::State::ROLLING ||
            state == PlayerMovement::State::SLASHING ||
            state == PlayerMovement::State::ATTACKING ||
            state == PlayerMovement::State::CHOPPING) { 
            make_anim_range_once(anim.speed, start_frame, start_frame + frame_count - 1); 
        } else { 
            make_anim_range(anim.speed, start_frame, start_frame + frame_count - 1); 
        }
        
        _last_state = state;
        _last_direction = direction; 
    }

    bool PlayerAnimation::should_change_animation(PlayerMovement::State state, PlayerMovement::Direction direction) {
        if (!_animation.has_value())
            return true;
        bool flip_changed = _sprite.horizontal_flip() != (direction == PlayerMovement::Direction::LEFT);
        bool state_changed = (_last_state != state);
        bool direction_changed = (_last_direction != direction);
        return flip_changed || state_changed || direction_changed; 
    }

    void PlayerAnimation::make_anim_range(int speed, int start_frame, int end_frame) {
        bn::vector<uint16_t, 32> frames;
        for (int i = start_frame; i <= end_frame; ++i) { frames.push_back(i); }
        _animation = bn::sprite_animate_action<32>::forever(
            _sprite, speed, bn::sprite_items::hero_sword.tiles_item(),
            bn::span<const uint16_t>(frames.data(), frames.size())); 
    }

    void PlayerAnimation::make_anim_range_once(int speed, int start_frame, int end_frame) {
        bn::vector<uint16_t, 32> frames;
        for (int i = start_frame; i <= end_frame; ++i) { frames.push_back(i); }
        _animation = bn::sprite_animate_action<32>::once(
            _sprite, speed, bn::sprite_items::hero_sword.tiles_item(),
            bn::span<const uint16_t>(frames.data(), frames.size())); 
    }

    void PlayerAnimation::update() {
        if (_animation.has_value() && !_animation->done()) { _animation->update(); }
    }

    // =========================================================================
    // PlayerVFX Implementation
    // =========================================================================

    PlayerVFX::PlayerVFX() : _last_vfx_state(PlayerMovement::State::IDLE), _last_vfx_direction(PlayerMovement::Direction::DOWN) {}

    void PlayerVFX::initialize(bn::camera_ptr camera) { _camera = camera; }

    void PlayerVFX::update(bn::fixed_point player_pos, PlayerMovement::State state, PlayerMovement::Direction direction) {
        if (should_show_vfx(state)) {
            if (!_vfx_sprite.has_value()) {
                _vfx_sprite = bn::sprite_items::hero_vfx.create_sprite(0, 0);
                if (_camera.has_value()) { _vfx_sprite->set_camera(*_camera); }
                _vfx_sprite->set_bg_priority(0);
                _vfx_sprite->set_z_order(-32000); 
            }
            if (should_change_vfx(state, direction)) { apply_vfx_state(state, direction); }
            _vfx_sprite->set_visible(true);
            
            bn::fixed_point vfx_pos = player_pos;
            bool is_attack = (state == PlayerMovement::State::SLASHING ||
                              state == PlayerMovement::State::ATTACKING ||
                              state == PlayerMovement::State::CHOPPING);
            
            if (is_attack && (direction == PlayerMovement::Direction::UP ||
                              direction == PlayerMovement::Direction::DOWN)) { 
                vfx_pos = bn::fixed_point(player_pos.x() + 8, player_pos.y() + PLAYER_SPRITE_Y_OFFSET); 
            } else { 
                vfx_pos = bn::fixed_point(player_pos.x(), player_pos.y() + PLAYER_SPRITE_Y_OFFSET); 
            }
            
            _vfx_sprite->set_position(vfx_pos);
            if (_vfx_animation.has_value()) {
                if (_vfx_animation->done()) { hide_vfx(); }
                else { _vfx_animation->update(); }
            }
        } else { 
            hide_vfx(); 
        }
        _last_vfx_state = state;
        _last_vfx_direction = direction; 
    }

    void PlayerVFX::apply_vfx_state(PlayerMovement::State state, PlayerMovement::Direction direction) {
        if (!_vfx_sprite.has_value())
            return;
        _vfx_sprite->set_horizontal_flip(direction == PlayerMovement::Direction::LEFT);
        switch (state) {
        case PlayerMovement::State::SLASHING: 
            if (direction == PlayerMovement::Direction::UP) make_vfx_anim_range_once(4, 480, 486);
            else if (direction == PlayerMovement::Direction::DOWN) make_vfx_anim_range_once(4, 192, 198);
            else make_vfx_anim_range_once(4, 336, 339);
            break;
        case PlayerMovement::State::ATTACKING: 
            if (direction == PlayerMovement::Direction::UP) make_vfx_anim_range_once(4, 480, 486);
            else if (direction == PlayerMovement::Direction::DOWN) make_vfx_anim_range_once(4, 192, 198);
            else make_vfx_anim_range_once(4, 360, 364);
            break;
        case PlayerMovement::State::CHOPPING: 
            if (direction == PlayerMovement::Direction::UP) make_vfx_anim_range_once(5, 456, 459);
            else if (direction == PlayerMovement::Direction::DOWN) make_vfx_anim_range_once(5, 168, 171);
            else make_vfx_anim_range_once(5, 336, 339);
            break;
        case PlayerMovement::State::HEAL_BUFF: make_vfx_anim_range(4, 24, 47); break;
        case PlayerMovement::State::DEFENCE_BUFF: make_vfx_anim_range(4, 48, 71); break;
        case PlayerMovement::State::POWER_BUFF: make_vfx_anim_range(4, 72, 95); break;
        case PlayerMovement::State::ENERGY_BUFF: make_vfx_anim_range(4, 96, 119); break;
        default: hide_vfx(); break; 
        }
    }

    void PlayerVFX::hide_vfx() {
        if (_vfx_sprite.has_value()) { _vfx_sprite->set_visible(false); }
        _vfx_animation.reset(); 
    }

    bool PlayerVFX::should_show_vfx(PlayerMovement::State state) const {
        return state == PlayerMovement::State::SLASHING ||
               state == PlayerMovement::State::ATTACKING ||
               state == PlayerMovement::State::CHOPPING ||
               state == PlayerMovement::State::POWER_BUFF ||
               state == PlayerMovement::State::DEFENCE_BUFF ||
               state == PlayerMovement::State::HEAL_BUFF ||
               state == PlayerMovement::State::ENERGY_BUFF; 
    }

    bool PlayerVFX::should_change_vfx(PlayerMovement::State state, PlayerMovement::Direction direction) const { 
        return state != _last_vfx_state || direction != _last_vfx_direction; 
    }

    void PlayerVFX::make_vfx_anim_range(int speed, int start_frame, int end_frame) {
        if (!_vfx_sprite.has_value())
            return;
        bn::vector<uint16_t, 32> frames;
        for (int i = start_frame; i <= end_frame; ++i) { frames.push_back(i); }
        _vfx_animation = bn::sprite_animate_action<32>::forever(
            *_vfx_sprite, speed, bn::sprite_items::hero_vfx.tiles_item(),
            bn::span<const uint16_t>(frames.data(), frames.size()));
        _vfx_sprite->set_visible(true); 
    }

    void PlayerVFX::make_vfx_anim_range_once(int speed, int start_frame, int end_frame) {
        if (!_vfx_sprite.has_value())
            return;
        bn::vector<uint16_t, 32> frames;
        for (int i = start_frame; i <= end_frame; ++i) { frames.push_back(i); }
        _vfx_animation = bn::sprite_animate_action<32>::once(
            *_vfx_sprite, speed, bn::sprite_items::hero_vfx.tiles_item(),
            bn::span<const uint16_t>(frames.data(), frames.size()));
        _vfx_sprite->set_visible(true); 
    }

    // =========================================================================
    // PlayerCompanion Implementation
    // =========================================================================

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
          _revival_timer(0) {}

    void PlayerCompanion::spawn(bn::fixed_point pos, bn::camera_ptr camera) {
        if (!_independent_death) { _position = pos + bn::fixed_point(8, -8); }
        _target_offset = calculate_companion_offset();
        _sprite.set_camera(camera);
        update_animation();
        die_independently(); 
    }

    void PlayerCompanion::update(bn::fixed_point player_pos, bool player_is_dead) {
        if (player_is_dead != _is_dead && !_independent_death && !_is_reviving) {
            _is_dead = player_is_dead;
            update_animation(); 
        }
        
        if (_is_reviving) {
            _sprite.set_position(_death_position);
            if (_animation && _animation->done()) {
                _is_reviving = false;
                _is_dead = false;
                _independent_death = false;
                _position = _death_position;
                update_animation(); 
            }
        } else if (_independent_death) {
            _sprite.set_position(_death_position);
            if (_can_be_revived && !_revival_in_progress) {
                bn::fixed_point diff = player_pos - _death_position;
                bn::fixed distance_sq = diff.x() * diff.x() + diff.y() * diff.y();
                bool player_in_range = (distance_sq <= COMPANION_REVIVE_DISTANCE * COMPANION_REVIVE_DISTANCE);
                if (player_in_range && _text_sprites.empty()) { show_revival_text(); }
                else if (!player_in_range && !_text_sprites.empty()) { hide_revival_text(); }
            } else if (_text_sprites.size() > 0) { 
                hide_revival_text(); 
            }
        } else if (!_is_dead) { 
            update_position(player_pos); 
        }
        
        if (_animation && (!_is_dead || !_animation->done() || _is_reviving)) {
            _animation->update();
            if (_is_dead && _independent_death && _animation->done() && !_can_be_revived && !_is_reviving) { _can_be_revived = true; }
        }
    }

    void PlayerCompanion::update_position(bn::fixed_point player_pos) {
        bn::fixed_point companion_to_player = player_pos - _position;
        bn::fixed player_distance = bn::sqrt(companion_to_player.x() * companion_to_player.x() +
                                             companion_to_player.y() * companion_to_player.y());
                                             
        if (!_player_too_close && player_distance < COMPANION_IDLE_DISTANCE) { _player_too_close = true; }
        else if (_player_too_close && player_distance > COMPANION_RESUME_DISTANCE) { _player_too_close = false; }
        
        if (!_player_too_close) {
            bn::fixed_point target_pos = player_pos + _target_offset;
            bn::fixed_point diff = target_pos - _position;
            bn::fixed distance = bn::sqrt(diff.x() * diff.x() + diff.y() * diff.y());
            if (distance > 1) {
                bn::fixed speed = (distance * 0.08 < 1.2) ? distance * 0.08 : 1.2;
                speed = (speed > 0.3) ? speed : 0.3;
                _position += (diff / distance) * speed; 
            }
        }
        
        if (player_distance > 8) {
            bn::fixed_point offset = _position - player_pos;
            Position new_side;
            if (bn::abs(offset.y()) > bn::abs(offset.x())) {
                if (offset.y() < 0) { new_side = (offset.x() >= 0) ? Position::RIGHT : Position::LEFT; }
                else { new_side = Position::BELOW; }
            } else { 
                new_side = (offset.x() > 0) ? Position::RIGHT : Position::LEFT; 
            }
            set_position_side(new_side); 
        }
        _sprite.set_position(_position); 
    }

    bn::fixed_point PlayerCompanion::calculate_companion_offset() const {
        switch (_position_side) {
        case Position::RIGHT: return {16, 0};
        case Position::LEFT: return {-16, 0};
        case Position::BELOW: return {0, 12};
        default: return {16, 0}; 
        }
    }

    void PlayerCompanion::update_animation() {
        if (_is_reviving) {
            _animation = bn::create_sprite_animate_action_once(
                _sprite, 8, bn::sprite_items::companion.tiles_item(),
                21, 20, 19, 18, 17, 16, 15, 14, 13, 12); 
        } else if (_is_dead) {
            _animation = bn::create_sprite_animate_action_once(
                _sprite, 8, bn::sprite_items::companion.tiles_item(),
                12, 13, 14, 15, 16, 17, 18, 19, 20, 21); 
        } else {
            int start_frame = static_cast<int>(_position_side) * 4;
            _animation = bn::create_sprite_animate_action_forever(
                _sprite, 12, bn::sprite_items::companion.tiles_item(),
                start_frame, start_frame + 1, start_frame + 2, start_frame + 3);
        }
    }

    void PlayerCompanion::set_flying(bool flying) {
        _is_flying = flying;
        update_animation(); 
    }

    void PlayerCompanion::set_camera(bn::camera_ptr camera) { _sprite.set_camera(camera); }

    void PlayerCompanion::set_position_side(Position side) {
        if (_position_side != side) {
            _position_side = side;
            _target_offset = calculate_companion_offset();
            update_animation(); 
        }
    }

    void PlayerCompanion::set_visible(bool visible) { _sprite.set_visible(visible); }
    void PlayerCompanion::set_z_order(int z_order) { _sprite.set_z_order(z_order); }

    void PlayerCompanion::start_death_animation() {
        _is_dead = true;
        update_animation(); 
    }

    void PlayerCompanion::die_independently() {
        if (!_is_dead) {
            _is_dead = true;
            _independent_death = true;
            _death_position = _position;
            _can_be_revived = false;
            cancel_revival();
            update_animation(); 
        }
    }

    bool PlayerCompanion::try_revive(bn::fixed_point player_pos, bool a_pressed, bool a_held) {
        if (!_independent_death || !_can_be_revived) { return false; }
        bn::fixed_point diff = player_pos - _death_position;
        bn::fixed distance_sq = diff.x() * diff.x() + diff.y() * diff.y();
        bool player_in_range = (distance_sq <= COMPANION_REVIVE_DISTANCE * COMPANION_REVIVE_DISTANCE);
        
        if (!player_in_range) {
            if (_revival_in_progress) { cancel_revival(); }
            return false; 
        }
        
        if (!_revival_in_progress) {
            if (a_pressed) {
                _revival_in_progress = true;
                _revival_timer = 0;
                _progress_bar_sprite = bn::sprite_items::companion_load.create_sprite(
                    _death_position.x(), _death_position.y(), 0);
                if (_sprite.camera().has_value()) { _progress_bar_sprite->set_camera(_sprite.camera().value()); }
                _progress_bar_sprite->set_z_order(_sprite.z_order() - 1); 
            }
        } else {
            if (a_held) {
                _revival_timer++;
                int progress_frame = (_revival_timer * 8) / COMPANION_REVIVAL_DURATION;
                if (progress_frame > 7) progress_frame = 7;
                
                if (_progress_bar_sprite.has_value()) {
                    _progress_bar_sprite->set_tiles(bn::sprite_items::companion_load.tiles_item(), progress_frame);
                    _progress_bar_sprite->set_position(_death_position.x() + 12, _death_position.y());
                }
                
                if (_revival_timer >= COMPANION_REVIVAL_DURATION) {
                    _revival_in_progress = false;
                    _revival_timer = 0;
                    _is_reviving = true;
                    _can_be_revived = false;
                    _position = _death_position;
                    _progress_bar_sprite.reset();
                    update_animation();
                    return true; 
                }
            } else { 
                cancel_revival(); 
            }
        }
        return false; 
    }

    void PlayerCompanion::cancel_revival() {
        _revival_in_progress = false;
        _revival_timer = 0;
        if (_progress_bar_sprite.has_value()) { _progress_bar_sprite.reset(); }
        hide_revival_text(); 
    }

    void PlayerCompanion::show_revival_text() {
        if (!_text_sprites.empty()) return;
        bn::sprite_text_generator text_generator(common::variable_8x8_sprite_font);
        text_generator.set_center_alignment();
        bn::fixed_point text_center = _death_position + bn::fixed_point(0, -20);
        text_generator.set_bg_priority(0);
        text_generator.generate(text_center, "Press A to revive", _text_sprites);
        _text_original_offsets.clear();
        for (bn::sprite_ptr &text_sprite : _text_sprites) {
            text_sprite.set_camera(_sprite.camera());
            text_sprite.set_z_order(-32767);
            _text_original_offsets.push_back(text_sprite.position() - text_center);
        }
    }

    void PlayerCompanion::hide_revival_text() { _text_sprites.clear(); }

    void PlayerCompanion::reset_text_positions() {
        if (_text_sprites.empty() || _text_original_offsets.empty()) return;
        bn::fixed_point text_center = _death_position + bn::fixed_point(0, -20);
        for (int i = 0; i < _text_sprites.size() && i < _text_original_offsets.size(); ++i) { 
            _text_sprites[i].set_position(text_center + _text_original_offsets[i]); 
        }
    }

    // =========================================================================
    // PlayerMovement Implementation
    // =========================================================================

    PlayerMovement::PlayerMovement() : _dx(0), _dy(0), _current_state(State::IDLE), _facing_direction(Direction::DOWN), _action_timer(0) {}

    void PlayerMovement::move_right() {
        _dx = bn::clamp(_dx + acc_const, -max_speed, max_speed);
        _facing_direction = Direction::RIGHT;
        update_state(); 
    }

    void PlayerMovement::move_left() {
        _dx = bn::clamp(_dx - acc_const, -max_speed, max_speed);
        _facing_direction = Direction::LEFT;
        update_state(); 
    }

    void PlayerMovement::move_up() {
        _dy = bn::clamp(_dy - acc_const, -max_speed, max_speed);
        _facing_direction = Direction::UP;
        update_state(); 
    }

    void PlayerMovement::move_down() {
        _dy = bn::clamp(_dy + acc_const, -max_speed, max_speed);
        _facing_direction = Direction::DOWN;
        update_state(); 
    }

    void PlayerMovement::move_direction(Direction dir) {
        switch (dir) {
        case Direction::RIGHT: _dx = bn::clamp(_dx + acc_const, -max_speed, max_speed); break;
        case Direction::LEFT: _dx = bn::clamp(_dx - acc_const, -max_speed, max_speed); break;
        case Direction::UP: _dy = bn::clamp(_dy - acc_const, -max_speed, max_speed); break;
        case Direction::DOWN: _dy = bn::clamp(_dy + acc_const, -max_speed, max_speed); break;
        default: break; 
        }
        _facing_direction = dir;
        update_state(); 
    }

    void PlayerMovement::apply_friction() {
        _dx *= friction_const;
        _dy *= friction_const;
        if (bn::abs(_dx) < movement_threshold) _dx = 0;
        if (bn::abs(_dy) < movement_threshold) _dy = 0;
        update_state(); 
    }

    void PlayerMovement::reset() {
        _dx = 0;
        _dy = 0;
        _current_state = State::IDLE;
        _facing_direction = Direction::DOWN;
        _action_timer = 0; 
    }

    void PlayerMovement::stop_movement() {
        _dx = 0;
        _dy = 0;
        update_state(); 
    }

    void PlayerMovement::start_action(State action, int timer) {
        _current_state = action;
        _action_timer = timer; 
    }

    void PlayerMovement::stop_action() {
        _action_timer = 0;
        _current_state = (bn::abs(_dx) > movement_threshold || bn::abs(_dy) > movement_threshold) ? State::WALKING : State::IDLE;
        update_state(); 
    }

    void PlayerMovement::start_running() {
        if (_current_state == State::WALKING || _current_state == State::IDLE) { _current_state = State::RUNNING; }
    }

    void PlayerMovement::stop_running() {
        if (_current_state == State::RUNNING) { 
            _current_state = (bn::abs(_dx) > movement_threshold || bn::abs(_dy) > movement_threshold) ? State::WALKING : State::IDLE; 
        }
    }

    void PlayerMovement::start_rolling() { start_action(State::ROLLING, PLAYER_ROLL_DURATION); }
    void PlayerMovement::start_chopping() { start_action(State::CHOPPING, PLAYER_CHOP_DURATION); }
    void PlayerMovement::start_slashing() { start_action(State::SLASHING, PLAYER_SLASH_DURATION); }
    void PlayerMovement::start_attacking() { start_action(State::ATTACKING, PLAYER_ATTACK_DURATION); }
    void PlayerMovement::start_buff(State buff_type) { start_action(buff_type, PLAYER_BUFF_DURATION); }

    void PlayerMovement::update_state() {
        if (_action_timer > 0) return;
        bool is_moving = bn::abs(_dx) > movement_threshold || bn::abs(_dy) > movement_threshold;
        if (is_moving && (_current_state == State::IDLE || _current_state == State::WALKING || _current_state == State::RUNNING))
            _current_state = State::WALKING;
        else if (!is_moving && (_current_state == State::WALKING || _current_state == State::RUNNING))
            _current_state = State::IDLE; 
    }

    // =========================================================================
    // PlayerState Implementation
    // =========================================================================

    void PlayerState::set_listening(bool listening) {
        if (_listening && !listening) { _dialog_cooldown = 10; }
        _listening = listening; 
    }

    void PlayerState::update_dialog_cooldown() {
        if (_dialog_cooldown > 0) _dialog_cooldown--; 
    }

    void PlayerState::reset() {
        _invulnerable = false;
        _listening = false;
        _inv_timer = 0;
        _dialog_cooldown = 0; 
    }

    // =========================================================================
    // Player Implementation
    // =========================================================================

    Player::Player(bn::sprite_ptr sprite) : Entity(sprite), _animation(sprite), _gun_active(false) {
        if (auto player_sprite = get_sprite()) { player_sprite->set_bg_priority(1); }
        set_sprite_z_order(1);
        _hitbox = Hitbox(0, 0, PLAYER_HITBOX_WIDTH, PLAYER_HITBOX_HEIGHT);
        _hud.set_hp(_hp);
        _hud.set_ammo(_ammo_count); 
    }

    void Player::spawn(bn::fixed_point pos, bn::camera_ptr camera) {
        _hud.set_hp(_hp);
        _hud.set_ammo(_ammo_count);
        set_position(pos);
        set_camera(camera);
        initialize_companion(camera);
        _vfx.initialize(camera);
        update_animation(); 
    }

    void Player::update() {
        auto old_state = _movement.current_state();
        auto old_direction = _movement.facing_direction();
        bool was_performing_action = _movement.is_performing_action();
        
        _abilities.update_cooldowns();
        _state.update_dialog_cooldown();
        _movement.update_action_timer();
        
        if (!_state.listening()) {
            handle_input();
            bn::fixed_point new_pos = pos() + bn::fixed_point(_movement.dx(), _movement.dy());
            
            if (_movement.current_state() == PlayerMovement::State::ROLLING) {
                new_pos += direction_utils::get_roll_offset(_movement.facing_direction(),
                                                            _movement.action_timer(),
                                                            PLAYER_ROLL_DURATION);
            }
            set_position(new_pos); 
        } else { 
            _movement.stop_movement(); 
        }
        
        if (was_performing_action && _movement.action_timer() <= 0) {
            if (_movement.current_state() == PlayerMovement::State::ROLLING && _state.invulnerable()) {
                _state.set_invulnerable(false);
                set_visible(true);
                if (_reload_on_roll_end) {
                    reload_ammo();
                    _hud.set_ammo(_ammo_count);
                    _reload_on_roll_end = false; 
                }
            }
            _movement.stop_action();
            update_animation(); 
        }
        
        if (old_state != _movement.current_state() || old_direction != _movement.facing_direction()) { update_animation(); }
        
        _animation.update();
        _hud.update();
        update_bullets();
        
        if (_state.invulnerable() && _state.inv_timer() > 0) {
            _state.set_inv_timer(_state.inv_timer() - 1);
            if (_state.inv_timer() % 10 == 0) { set_visible(!get_sprite()->visible()); }
            if (_state.inv_timer() == 0) {
                _state.set_invulnerable(false);
                set_visible(true); 
            }
        }
        
        if (_movement.current_state() == PlayerMovement::State::DEAD) {
            if (!_death_sound_played) {
                bn::sound_items::death.play();
                _death_sound_played = true; 
            }
            if (_hud.is_soul_animation_complete()) { _reset_required = true; }
        }
        
        if (_companion.has_value()) {
            _companion->update(pos(), _movement.current_state() == PlayerMovement::State::DEAD);
            if (_companion->is_dead_independently()) { _companion->try_revive(pos(), bn::keypad::a_pressed(), bn::keypad::a_held()); }
            if (_companion->is_dead_independently() || _companion->is_reviving()) { _companion->set_visible(true); }
        }
        
        _vfx.update(pos(), _movement.current_state(), _movement.facing_direction());
        update_z_order(); 
    }

    void Player::set_position(bn::fixed_point new_pos) {
        Entity::set_position(new_pos);
        bn::fixed_point hitbox_pos = Hitbox::calculate_centered_position(new_pos,
                                                                         PLAYER_HITBOX_WIDTH, PLAYER_HITBOX_HEIGHT);
        _hitbox.set_x(hitbox_pos.x());
        _hitbox.set_y(hitbox_pos.y());
        update_sprite_position(); 
    }

    void Player::update_sprite_position() {
        if (auto sprite = get_sprite()) {
            bn::fixed_point pos = Entity::pos();
            sprite->set_position(pos.x(), pos.y() + PLAYER_SPRITE_Y_OFFSET); 
        }
    }

    void Player::revert_position() { set_position(_previous_pos); }

    void Player::set_sprite_z_order(int z_order) {
        if (auto sprite = get_sprite()) { sprite->set_z_order(z_order); }
    }

    void Player::update_z_order() {
        int z_order = -pos().y().integer();
        set_sprite_z_order(z_order);
        
        if (_gun_sprite.has_value()) {
            PlayerMovement::Direction gun_dir = _is_strafing ? _strafing_direction : _movement.facing_direction();
            int gun_z_offset = direction_utils::get_gun_z_offset(gun_dir);
            _gun_sprite->set_z_order(z_order + gun_z_offset); 
        }
        
        if (_companion.has_value()) {
            bn::fixed_point companion_pos = _companion->pos();
            bn::fixed player_y = pos().y();
            bn::fixed companion_y = companion_pos.y();
            if (player_y >= companion_y + 8) { _companion->set_z_order(z_order + 10); }
            else { _companion->set_z_order(z_order - 10); }
        }
    }

    void Player::update_animation() { _animation.apply_state(_movement.current_state(), _movement.facing_direction()); }

    void Player::take_damage(int damage) {
        if (!_state.invulnerable() && _hp > 0) {
            _hp -= damage;
            if (_hp <= 0) {
                _hp = 0;
                _movement.set_state(PlayerMovement::State::DEAD);
                _movement.stop_movement();
                _death_timer = PLAYER_DEATH_ANIMATION_DURATION;
                _death_sound_played = false;
                _state.set_invulnerable(false);
                _state.set_inv_timer(0);
                update_animation(); 
            } else {
                _state.set_invulnerable(true);
                _state.set_inv_timer(60);
                set_visible(false); 
            }
            _hud.set_hp(_hp); 
        }
    }

    void Player::heal(int amount) {
        if (_hp < 3 && _hp > 0) {
            _hp = bn::min(_hp + amount, 3);
            _hud.set_hp(_hp);
            _hud.update(); 
        }
    }

    void Player::reset() {
        _hp = 3;
        _reset_required = false;
        _death_timer = 0;
        _death_sound_played = false;
        _state.reset();
        _movement.reset();
        _abilities.reset();
        _hud.set_resetting_health(true);
        _hud.set_hp(_hp);
        _hud.set_resetting_health(false);
        _hud.update();
        set_visible(true);
        _bullet_manager.clear_bullets();
        _ammo_count = MAX_AMMO;
        _hud.set_ammo(_ammo_count);
        if (_companion.has_value() && !_companion->is_dead_independently()) { _companion->set_visible(true); }
    }

    void Player::reset_movement() { _movement.reset(); }

    void Player::add_ammo(int amount) {
        _ammo_count = bn::min(_ammo_count + amount, MAX_AMMO);
        _hud.set_ammo(_ammo_count); 
    }

    void Player::reload_ammo() {
        _ammo_count = MAX_AMMO;
        _hud.set_ammo(_ammo_count); 
    }

    bool Player::has_ammo() const { return _ammo_count > 0; }

    bool Player::is_attacking() const {
        return _movement.current_state() == PlayerMovement::State::CHOPPING ||
               _movement.current_state() == PlayerMovement::State::SLASHING ||
               _movement.current_state() == PlayerMovement::State::ATTACKING; 
    }

    bool Player::can_start_attack() const { return !is_attacking() && !_movement.is_performing_action(); }

    Hitbox Player::get_melee_hitbox() const {
        if (!is_attacking()) { return Hitbox(0, 0, 0, 0); }
        
        bn::fixed_point attack_pos = pos();
        PlayerMovement::Direction dir = _movement.facing_direction();
        bn::fixed range;
        
        if (_movement.is_state(PlayerMovement::State::SLASHING)) { range = 24 * 1.1; }
        else if (_movement.is_state(PlayerMovement::State::CHOPPING)) { range = 24 * 1.2; }
        else { range = 24; }
        
        bn::fixed width = 32;
        bn::fixed height = 16;
        bn::fixed hitbox_x = attack_pos.x();
        bn::fixed hitbox_y = attack_pos.y() + PLAYER_SPRITE_Y_OFFSET;
        
        switch (dir) {
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
        default: break; 
        }
        return Hitbox(hitbox_x, hitbox_y, width, height); 
    }

    void Player::update_gun_position(PlayerMovement::Direction direction) {
        if (!_gun_sprite) return;
        direction_utils::setup_gun(*_gun_sprite, static_cast<fe::Direction>(int(direction)), pos()); 
    }

    void Player::fire_bullet(PlayerMovement::Direction direction) {
        if (!_gun_active || !_gun_sprite.has_value() || !has_ammo()) return;
        if (!_bullet_manager.can_fire()) return;
        
        direction_utils::setup_gun(*_gun_sprite, static_cast<fe::Direction>(int(direction)), pos());
        
        // Convert PlayerMovement::Direction to fe::Direction for BulletManager
        // Assuming they map 1:1, or we need a converter.
        // fe::Direction and PlayerMovement::Direction both have UP, DOWN, LEFT, RIGHT in same order usually.
        // Let's check.
        // fe_bullet_manager.h: UP, DOWN, LEFT, RIGHT
        // fe_player.h: UP, DOWN, LEFT, RIGHT
        // So casting is safe.
        fe::Direction bullet_dir = static_cast<fe::Direction>(int(direction));
        
        // We need get_bullet_position. It's in fe_direction_utils.h and declared as inline.
        // Since fe_player.cpp includes fe_direction_utils.h, it should be available.
        // However, it takes fe::Direction.
        bn::fixed_point bullet_pos = fe::direction_utils::get_bullet_position(bullet_dir, pos());
        
        _bullet_manager.fire_bullet(bullet_pos, bullet_dir);
        _ammo_count--;
        _hud.set_ammo(_ammo_count);
        _bullet_just_fired = true; 
    }

    void Player::update_bullets() { _bullet_manager.update_bullets(); }

    bool Player::is_firing() const { return bn::keypad::a_held() && _gun_active && _state.dialog_cooldown() == 0; }

    void Player::initialize_companion(bn::camera_ptr camera) {
        if (_companion_initialized) {
            if (_companion.has_value() && _companion->is_dead_independently()) { _companion->set_camera(camera); }
            return; 
        }
        bn::sprite_ptr companion_sprite = bn::sprite_items::companion.create_sprite(pos());
        companion_sprite.set_bg_priority(0);
        _companion = PlayerCompanion(bn::move(companion_sprite));
        _companion->spawn(pos(), camera);
        _companion->set_flying(true);
        _companion_initialized = true; 
    }

    void Player::handle_input() {
        if (_state.listening() || _movement.current_state() == PlayerMovement::State::DEAD) { return; }
        
        bool reviving_companion = _companion.has_value() && _companion->is_revival_in_progress();
        bool performing_action = _movement.is_performing_action();
        
        if (bn::keypad::r_held()) {
            _r_hold_frames++;
            if (_r_hold_frames > WEAPON_SWITCH_WINDOW && _gun_active && !reviving_companion && !_hud.is_buff_menu_open()) {
                if (_auto_reload_timer == 0) { _auto_reload_timer = AUTO_RELOAD_INTERVAL; }
                _auto_reload_timer--;
                if (_auto_reload_timer <= 0 && _ammo_count < MAX_AMMO) {
                    _ammo_count++;
                    _hud.set_ammo(_ammo_count);
                    _auto_reload_timer = AUTO_RELOAD_INTERVAL; 
                }
            }
        } else {
            if (_r_hold_frames > 0 && _r_hold_frames <= WEAPON_SWITCH_WINDOW && !performing_action && !reviving_companion) { switch_weapon(); }
            _r_hold_frames = 0; 
        }
        
        if (bn::keypad::select_held() && bn::keypad::b_pressed() && _gun_active && !reviving_companion) { cycle_gun_sprite(); }
        if (bn::keypad::select_held() && bn::keypad::b_pressed() && !_gun_active && _hud.get_weapon() == WEAPON_TYPE::SWORD && !reviving_companion) { cycle_sword_sprite(); }
        
        if (_movement.current_state() == PlayerMovement::State::ROLLING) {
            bool should_cancel = false;
            PlayerMovement::Direction roll_dir = _movement.facing_direction();
            switch (roll_dir) {
            case PlayerMovement::Direction::RIGHT: should_cancel = bn::keypad::left_pressed(); break;
            case PlayerMovement::Direction::LEFT: should_cancel = bn::keypad::right_pressed(); break;
            case PlayerMovement::Direction::UP: should_cancel = bn::keypad::down_pressed(); break;
            case PlayerMovement::Direction::DOWN: should_cancel = bn::keypad::up_pressed(); break;
            default: break;
            }
            if (should_cancel) {
                _movement.stop_action();
                _state.set_invulnerable(false); 
            }
        }
        
        if (!performing_action && !reviving_companion && !_hud.is_buff_menu_open()) {
            if (bn::keypad::b_pressed() && !bn::keypad::select_held() && _abilities.rolling_available()) {
                _movement.start_action(PlayerMovement::State::ROLLING, PLAYER_ROLL_DURATION);
                _abilities.set_roll_cooldown(90);
                _state.set_invulnerable(true);
                _state.set_inv_timer(0);
                _reload_on_roll_end = _gun_active;
                bn::sound_items::swipe.play(); 
            }
            else if (bn::keypad::a_held() && _state.dialog_cooldown() == 0 && _gun_active && shared_gun_frame == 0) { 
                fire_bullet(_is_strafing ? _strafing_direction : _movement.facing_direction()); 
            }
            else if (bn::keypad::a_pressed() && _state.dialog_cooldown() == 0) {
                if (_gun_active) {
                    if (shared_gun_frame != 0) { fire_bullet(_is_strafing ? _strafing_direction : _movement.facing_direction()); }
                } else if ((_combo_ready && _abilities.chopping_available() && can_start_attack()) || (!_combo_ready && _abilities.slashing_available() && can_start_attack())) {
                    if (_combo_ready && (_frame_counter - _last_attack_time) <= COMBO_WINDOW) {
                        _movement.start_action(PlayerMovement::State::CHOPPING, PLAYER_CHOP_DURATION);
                        _abilities.set_chop_cooldown(30);
                        _combo_ready = false; 
                    } else {
                        _movement.start_action(PlayerMovement::State::SLASHING, PLAYER_SLASH_DURATION);
                        _abilities.set_slash_cooldown(30);
                        _last_attack_time = _frame_counter;
                        _combo_ready = true; 
                    }
                }
            }
            else if (bn::keypad::select_held() && _abilities.buff_abilities_available()) {
                PlayerMovement::State buff_state = PlayerMovement::State::IDLE;
                if (bn::keypad::up_pressed()) buff_state = PlayerMovement::State::HEAL_BUFF;
                else if (bn::keypad::down_pressed()) buff_state = PlayerMovement::State::DEFENCE_BUFF;
                else if (bn::keypad::left_pressed()) buff_state = PlayerMovement::State::POWER_BUFF;
                else if (bn::keypad::right_pressed()) buff_state = PlayerMovement::State::ENERGY_BUFF;
                
                if (buff_state != PlayerMovement::State::IDLE) { activate_buff(buff_state); }
            }
        }
        
        _hud.update_buff_menu_cooldown();
        
        if (!performing_action && !reviving_companion && _abilities.buff_abilities_available() && !_hud.is_buff_menu_on_cooldown()) {
            if (!bn::keypad::select_held()) {
                if (!_hud.is_buff_menu_open()) {
                    if (bn::keypad::l_pressed()) { _hud.start_buff_menu_hold(); }
                    else if (bn::keypad::l_held() && _hud.is_buff_menu_holding()) {
                        _hud.update_buff_menu_hold();
                        if (_hud.is_buff_menu_hold_complete()) {
                            _hud.cancel_buff_menu_hold();
                            _hud.toggle_buff_menu(); 
                        }
                    }
                    else if (!bn::keypad::l_held() && _hud.is_buff_menu_holding()) { _hud.cancel_buff_menu_hold(); }
                } else {
                    if (bn::keypad::a_pressed() || bn::keypad::l_pressed()) {
                        int selected = _hud.get_selected_buff();
                        PlayerMovement::State buff_state = PlayerMovement::State::IDLE;
                        switch (selected) {
                        case 0: buff_state = PlayerMovement::State::HEAL_BUFF; break;
                        case 1: buff_state = PlayerMovement::State::ENERGY_BUFF; break;
                        case 2: buff_state = PlayerMovement::State::POWER_BUFF; break;
                        default: break; 
                        }
                        activate_buff(buff_state);
                        _hud.toggle_buff_menu();
                        _hud.start_buff_menu_cooldown(); 
                    }
                    else if (bn::keypad::b_pressed()) { _hud.toggle_buff_menu(); }
                }
            }
            
            if (_hud.is_buff_menu_open() && !bn::keypad::select_held()) {
                if (bn::keypad::up_pressed()) { _hud.navigate_buff_menu_up(); }
                else if (bn::keypad::down_pressed()) { _hud.navigate_buff_menu_down(); }
                else if (bn::keypad::left_pressed()) { _hud.navigate_buff_menu_left(); }
                else if (bn::keypad::right_pressed()) { _hud.navigate_buff_menu_right(); }
            }
        } else if (_hud.is_buff_menu_holding()) { _hud.cancel_buff_menu_hold(); }
        
        if (bn::keypad::select_held() && bn::keypad::start_held()) {
            // Debug commands
            if (bn::keypad::up_pressed()) {
                if (get_hp() > 0) take_damage(get_hp());
                heal(1); 
            }
            else if (bn::keypad::right_pressed()) {
                if (get_hp() > 1) take_damage(get_hp() - 1);
                heal(1); 
            }
            else if (bn::keypad::down_pressed()) {
                if (get_hp() < 2) heal(2 - get_hp());
                take_damage(1); 
            }
            else if (bn::keypad::left_pressed()) {
                if (get_hp() < 1) heal(1 - get_hp());
                take_damage(1); 
            }
        }
        
        if (!performing_action && !reviving_companion && !_hud.is_buff_menu_open()) {
            bool should_run = !_is_strafing && _abilities.running_available();
            if (_is_strafing) {
                bn::fixed dx = _movement.dx();
                bn::fixed dy = _movement.dy();
                bool horizontal_input = false;
                bool vertical_input = false;
                bn::fixed dx_delta = 0;
                bn::fixed dy_delta = 0;
                
                if (bn::keypad::right_held()) {
                    dx_delta = PlayerMovement::acc_const;
                    horizontal_input = true; 
                } else if (bn::keypad::left_held()) {
                    dx_delta = -PlayerMovement::acc_const;
                    horizontal_input = true; 
                }
                
                if (bn::keypad::up_held()) {
                    dy_delta = -PlayerMovement::acc_const;
                    vertical_input = true; 
                } else if (bn::keypad::down_held()) {
                    dy_delta = PlayerMovement::acc_const;
                    vertical_input = true; 
                }
                
                if (horizontal_input && vertical_input) {
                    dx_delta *= PlayerMovement::diagonal_factor;
                    dy_delta *= PlayerMovement::diagonal_factor; 
                }
                
                dx = bn::clamp(dx + dx_delta, -PlayerMovement::max_speed, PlayerMovement::max_speed);
                dy = bn::clamp(dy + dy_delta, -PlayerMovement::max_speed, PlayerMovement::max_speed);
                _movement.set_dx(dx);
                _movement.set_dy(dy);
                _movement.update_movement_state(); 
            } else {
                bn::fixed dx = _movement.dx();
                bn::fixed dy = _movement.dy();
                bool horizontal_input = false;
                bool vertical_input = false;
                bn::fixed dx_delta = 0;
                bn::fixed dy_delta = 0;
                PlayerMovement::Direction last_direction = _movement.facing_direction();
                
                if (bn::keypad::right_held()) {
                    dx_delta = PlayerMovement::acc_const;
                    horizontal_input = true;
                    last_direction = PlayerMovement::Direction::RIGHT; 
                } else if (bn::keypad::left_held()) {
                    dx_delta = -PlayerMovement::acc_const;
                    horizontal_input = true;
                    last_direction = PlayerMovement::Direction::LEFT; 
                }
                
                if (bn::keypad::up_held()) {
                    dy_delta = -PlayerMovement::acc_const;
                    vertical_input = true;
                    last_direction = PlayerMovement::Direction::UP; 
                } else if (bn::keypad::down_held()) {
                    dy_delta = PlayerMovement::acc_const;
                    vertical_input = true;
                    last_direction = PlayerMovement::Direction::DOWN; 
                }
                
                if (horizontal_input && vertical_input) {
                    dx_delta *= PlayerMovement::diagonal_factor;
                    dy_delta *= PlayerMovement::diagonal_factor; 
                }
                
                dx = bn::clamp(dx + dx_delta, -PlayerMovement::max_speed, PlayerMovement::max_speed);
                dy = bn::clamp(dy + dy_delta, -PlayerMovement::max_speed, PlayerMovement::max_speed);
                _movement.set_dx(dx);
                _movement.set_dy(dy);
                if (horizontal_input || vertical_input) { _movement.set_facing_direction(last_direction); }
                _movement.update_movement_state(); 
            }
            
            if (should_run && _movement.is_moving()) {
                if (_movement.is_state(PlayerMovement::State::WALKING)) { _movement.start_action(PlayerMovement::State::RUNNING, 0); }
            } else if (!should_run && _movement.is_state(PlayerMovement::State::RUNNING)) { 
                _movement.start_action(PlayerMovement::State::WALKING, 0); 
            }
        }
        
        update_gun_if_active();
        _movement.apply_friction(); 
    }

    void Player::toggle_gun() {
        _gun_active = !_gun_active;
        if (_gun_active && !_gun_sprite.has_value()) {
            _gun_sprite = bn::sprite_items::gun.create_sprite(pos().x(), pos().y(), shared_gun_frame);
            _gun_sprite->set_bg_priority(get_sprite()->bg_priority());
            if (_hud.get_weapon() == WEAPON_TYPE::GUN) { _hud.set_weapon_frame(shared_gun_frame); }
            
            PlayerMovement::Direction gun_dir = _is_strafing ? _strafing_direction : _movement.facing_direction();
            int gun_z_offset = direction_utils::get_gun_z_offset(gun_dir);
            _gun_sprite->set_z_order(get_sprite()->z_order() + gun_z_offset);
            
            if (get_sprite()->camera().has_value()) {
                _gun_sprite->set_camera(get_sprite()->camera().value());
                _bullet_manager.set_camera(get_sprite()->camera().value()); 
            }
        } else if (!_gun_active) { 
            _gun_sprite.reset(); 
        }
    }

    void Player::update_gun_if_active() {
        if (_gun_active && _gun_sprite.has_value()) {
            PlayerMovement::Direction gun_dir = _is_strafing ? _strafing_direction : _movement.facing_direction();
            update_gun_position(gun_dir); 
        }
    }

    void Player::switch_weapon() {
        _combo_ready = false;
        _last_attack_time = 0;
        if (_hud.get_weapon() == WEAPON_TYPE::GUN) {
            _hud.set_weapon(WEAPON_TYPE::SWORD);
            if (_gun_active) {
                _gun_active = false;
                _gun_sprite.reset(); 
            }
            _hud.set_ammo(0); 
        } else {
            _hud.set_weapon(WEAPON_TYPE::GUN);
            _hud.set_weapon_frame(shared_gun_frame);
            if (!_gun_active) {
                _gun_active = true;
                if (!_gun_sprite.has_value()) {
                    _gun_sprite = bn::sprite_items::gun.create_sprite(pos().x(), pos().y(), shared_gun_frame);
                    _gun_sprite->set_bg_priority(get_sprite()->bg_priority());
                    
                    PlayerMovement::Direction gun_dir = _is_strafing ? _strafing_direction : _movement.facing_direction();
                    int gun_z_offset = direction_utils::get_gun_z_offset(gun_dir);
                    _gun_sprite->set_z_order(get_sprite()->z_order() + gun_z_offset);
                    
                    if (get_sprite()->camera().has_value()) {
                        _gun_sprite->set_camera(get_sprite()->camera().value());
                        _bullet_manager.set_camera(get_sprite()->camera().value());
                    }
                }
            }
            _hud.set_ammo(_ammo_count); 
        }
    }

    void Player::cycle_gun_sprite() {
        if (_gun_active && _gun_sprite.has_value()) {
            shared_gun_frame = (shared_gun_frame + 1) % 6;
            _gun_sprite->set_tiles(bn::sprite_items::gun.tiles_item(), shared_gun_frame);
            _hud.set_weapon_frame(shared_gun_frame); 
        }
    }

    void Player::cycle_sword_sprite() { shared_sword_frame = (shared_sword_frame + 1) % 6; }

    void Player::activate_buff(PlayerMovement::State buff_state) {
        if (buff_state == PlayerMovement::State::IDLE) { return; }
        _movement.start_action(buff_state, PLAYER_BUFF_DURATION);
        _abilities.set_buff_cooldown(PLAYER_BUFF_DURATION);
        if (buff_state == PlayerMovement::State::HEAL_BUFF) { heal(1); }
    }

} // namespace fe
