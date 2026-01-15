#include "fe_enemy.h"
#include "fe_constants.h"
#include "fe_enemy_states.h"
#include "fe_enemy_state_machine.h"
#include "fe_level.h"
#include "fe_enemy_type.h"
#include "fe_collision.h"

#include "bn_fixed_point.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_shape_size.h"
#include "bn_camera_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_optional.h"
#include "bn_span.h"
#include "bn_log.h"
#include "bn_size.h"
#include "bn_sprite_builder.h"
#include "bn_memory.h"
#include "bn_random.h"
#include "bn_math.h"

#include "bn_sprite_items_healthbar_enemy.h"
#include "bn_sprite_items_spearguard.h"

namespace fe {

    constexpr int HEALTHBAR_Z_ORDER = -1000;

    // =========================================================================
    // Enemy Implementation
    // =========================================================================

    Enemy::Enemy(int x, int y, bn::camera_ptr camera, bn::regular_bg_ptr map, ENEMY_TYPE type, int hp) 
        : Entity(bn::fixed_point(x, y)),
          _camera(camera),
          _type(type),
          _dir(0),
          _hp(hp),
          _max_hp(hp),
          _map(map),
          _map_cells(map.map().cells_ref().value()) {
        
        bn::sprite_builder builder(bn::sprite_items::spearguard);
        switch (_type) {
        case ENEMY_TYPE::SPEARGUARD: builder = bn::sprite_builder(bn::sprite_items::spearguard); break;
        case ENEMY_TYPE::SLIME: builder = bn::sprite_builder(bn::sprite_items::spearguard); break;
        case ENEMY_TYPE::MUTANT: builder = bn::sprite_builder(bn::sprite_items::spearguard); break;
        default: builder = bn::sprite_builder(bn::sprite_items::spearguard); break; 
        }
        
        builder.set_position(pos());
        builder.set_bg_priority(1);
        _sprite = builder.build();
        if (!_sprite.has_value()) { return; }
        
        set_camera(_camera);
        _hitbox = Hitbox(pos().x() - 4, pos().y() - 4, 8, 8);
        
        if (_type == ENEMY_TYPE::SPEARGUARD) { _original_position = pos(); }
        
        if (_type == ENEMY_TYPE::SPEARGUARD) {
            _action = bn::create_sprite_animate_action_forever(
                *_sprite,
                12,
                bn::sprite_items::spearguard.tiles_item(),
                0, 1, 2, 3, 4, 5
            );
            _current_animation = AnimationState::IDLE; 
        } else {
            _action = bn::create_sprite_animate_action_forever(
                *_sprite,
                8,
                bn::sprite_items::spearguard.tiles_item(),
                0, 1, 2, 3); 
        }
        
        bn::unique_ptr<IdleState> initial_state = bn::make_unique<IdleState>();
        _state_machine.initialize(bn::move(initial_state));
        
        _health_bar_sprite = bn::sprite_items::healthbar_enemy.create_sprite(pos().x(), pos().y() - 20, 0);
        _health_bar_sprite->set_camera(_camera);
        _health_bar_sprite->set_bg_priority(3);
        _health_bar_sprite->set_z_order(HEALTHBAR_Z_ORDER);
        _update_health_bar(); 
    }

    Enemy::Enemy(Enemy &&other) noexcept
        : Entity(bn::move(other)),
          _movement(bn::move(other._movement)),
          _state_machine(bn::move(other._state_machine)),
          _state_timer(other._state_timer),
          _state_duration(other._state_duration),
          _target_dx(other._target_dx),
          _target_dy(other._target_dy),
          _dx(other._dx),
          _dy(other._dy),
          _camera(other._camera),
          _type(other._type),
          _dir(other._dir),
          _hp(other._hp),
          _direction_timer(other._direction_timer),
          _invulnerable(other._invulnerable),
          _dead(other._dead),
          _grounded(other._grounded),
          _inv_timer(other._inv_timer),
          _stunned(other._stunned),
          _death_timer(other._death_timer),
          _knockback_dx(other._knockback_dx),
          _knockback_dy(other._knockback_dy),
          _knockback_timer(other._knockback_timer),
          _sound_timer(other._sound_timer),
          _spotted_player(other._spotted_player),
          _action(bn::move(other._action)),
          _health_bar_sprite(bn::move(other._health_bar_sprite)),
          _max_hp(other._max_hp),
          _current_animation(other._current_animation),
          _attack_timer(other._attack_timer),
          _original_position(other._original_position),
          _returning_to_post(other._returning_to_post),
          _target(other._target),
          _target_locked(other._target_locked),
          _map(other._map),
          _map_cells(other._map_cells),
          _level(other._level) {
        other._hp = 0;
        other._dead = true; 
    }

    Enemy &Enemy::operator=(Enemy &&other) noexcept {
        if (this != &other) {
            Entity::operator=(bn::move(other));
            _movement = bn::move(other._movement);
            _state_machine = bn::move(other._state_machine);
            _state_timer = other._state_timer;
            _state_duration = other._state_duration;
            _target_dx = other._target_dx;
            _target_dy = other._target_dy;
            _dx = other._dx;
            _dy = other._dy;
            _camera = other._camera;
            _type = other._type;
            _dir = other._dir;
            _hp = other._hp;
            _direction_timer = other._direction_timer;
            _invulnerable = other._invulnerable;
            _dead = other._dead;
            _grounded = other._grounded;
            _inv_timer = other._inv_timer;
            _stunned = other._stunned;
            _death_timer = other._death_timer;
            _knockback_dx = other._knockback_dx;
            _knockback_dy = other._knockback_dy;
            _knockback_timer = other._knockback_timer;
            _sound_timer = other._sound_timer;
            _spotted_player = other._spotted_player;
            _action = bn::move(other._action);
            _health_bar_sprite = bn::move(other._health_bar_sprite);
            _max_hp = other._max_hp;
            _current_animation = other._current_animation;
            _attack_timer = other._attack_timer;
            _original_position = other._original_position;
            _returning_to_post = other._returning_to_post;
            _target = other._target;
            _target_locked = other._target_locked;
            _map = other._map;
            _map_cells = other._map_cells;
            _level = other._level;
            other._hp = 0;
            other._dead = true; 
        }
        return *this; 
    }

    void Enemy::update_hitbox() {
        _hitbox.set_x(pos().x() - 4);
        _hitbox.set_y(pos().y() - 4); 
    }

    void Enemy::update(bn::fixed_point player_pos, const Level &level, bool player_listening) {
        if (_knockback_timer > 0) {
            _knockback_timer--;
            set_position(bn::fixed_point(pos().x() + _knockback_dx, pos().y() + _knockback_dy));
            _knockback_dx *= 0.9;
            _knockback_dy *= 0.9;
            if (_knockback_timer == 0) {
                _knockback_dx = 0;
                _knockback_dy = 0;
                _stunned = false;
                bn::unique_ptr<StunnedState> stunned_state = bn::make_unique<StunnedState>();
                _state_machine.transition_to(*this, bn::move(stunned_state)); 
            }
            update_hitbox();
            return; 
        }
        
        if (!_dead) {
            _state_machine.update(*this, player_pos, level, player_listening);
            const bn::fixed lerp = 0.1;
            _dx += (_target_dx - _dx) * lerp;
            _dy += (_target_dy - _dy) * lerp;
            _movement.set_velocity(bn::fixed_point(_dx, _dy));
            _movement.update();
            bn::fixed new_x = pos().x() + _dx;
            bn::fixed new_y = pos().y() + _dy;
            bn::fixed_point new_pos(new_x, new_y);
            
            // Unused currently but calculated
            /*
            directions check_direction = directions::down;
            if (bn::abs(_dx) > bn::abs(_dy)) { check_direction = _dx > 0 ? directions::right : directions::left; }
            else { check_direction = _dy > 0 ? directions::down : directions::up; }
            */
            
            set_position(new_pos); 
        } else {
            _dx = 0;
            _dy = 0;
            _target_dx = 0;
            _target_dy = 0;
            _movement.set_velocity(bn::fixed_point(0, 0));
            if (_death_timer > 0) { _death_timer--; }
        }
        
        if (_invulnerable && !_dead) {
            if (--_inv_timer <= 0) {
                _invulnerable = false;
                _inv_timer = 0;
                if (_sprite.has_value()) { _sprite->set_visible(true); }
            }
        } else if (_dead && _sprite.has_value()) { 
            _sprite->set_visible(true); 
        }
        
        _update_spearguard_animation();
        
        if (_sprite.has_value()) {
            _sprite->set_position(pos());
            _sprite->set_horizontal_flip(_dx < 0);
            if (_action.has_value()) {
                if (!_action->done()) { _action->update(); }
            }
        }
        _update_health_bar_position();
        update_hitbox(); 
    }

    void Enemy::set_pos(bn::fixed_point new_pos) { set_position(new_pos); }

    bool Enemy::_take_damage(int damage) {
        if (_invulnerable || _dead) { return false; }
        _hp -= damage;
        _invulnerable = true;
        _inv_timer = 30;
        _stunned = true;
        if (_type == ENEMY_TYPE::SPEARGUARD) { _aggroed = true; }
        _update_health_bar();
        if (_hp <= 0) {
            _dead = true;
            _death_timer = ENEMY_DEATH_ANIMATION_DURATION; 
        }
        return true; 
    }

    void Enemy::_apply_knockback(bn::fixed dx, bn::fixed dy) {
        _knockback_dx = dx * ENEMY_KNOCKBACK_STRENGTH;
        _knockback_dy = dy * ENEMY_KNOCKBACK_STRENGTH;
        _knockback_timer = ENEMY_KNOCKBACK_DURATION;
        _stunned = true; 
    }

    bool Enemy::damage_from_left(int damage) {
        if (_take_damage(damage)) {
            _apply_knockback(1.0, -0.5);
            return true; 
        }
        return false; 
    }

    bool Enemy::damage_from_right(int damage) {
        if (_take_damage(damage)) {
            _apply_knockback(-1.0, -0.5);
            return true; 
        }
        return false; 
    }

    bool Enemy::is_hit(Hitbox ) { return false; }
    bool Enemy::is_vulnerable() { return !_invulnerable; }

    void Enemy::set_visible(bool visibility) {
        if (_sprite.has_value()) { _sprite->set_visible(visibility); }
    }

    bool Enemy::spotted_player() { return _spotted_player; }
    int Enemy::hp() { return _hp; }
    ENEMY_TYPE Enemy::type() { return _type; }
    bool Enemy::is_ready_for_removal() { return _dead && _death_timer <= 0; }

    void Enemy::_update_spearguard_animation() {
        if (_type != ENEMY_TYPE::SPEARGUARD || !_sprite.has_value()) { return; }
        
        AnimationState desired_animation = AnimationState::IDLE;
        if (_dead) { desired_animation = AnimationState::DEAD; }
        else if (_attack_timer > 0) {
            desired_animation = AnimationState::ATTACK;
            _attack_timer--; 
        } else if (_state_machine.get_current_state_id() == EnemyStateId::CHASE ||
                 _state_machine.get_current_state_id() == EnemyStateId::PATROL ||
                 _state_machine.get_current_state_id() == EnemyStateId::RETURN_TO_POST) { 
            desired_animation = AnimationState::RUN; 
        } else { 
            desired_animation = AnimationState::IDLE; 
        }
        
        if (desired_animation != _current_animation) {
            _current_animation = desired_animation;
            switch (_current_animation) {
            case AnimationState::IDLE: 
                _action = bn::create_sprite_animate_action_forever(
                    *_sprite, 12, bn::sprite_items::spearguard.tiles_item(), 0, 1, 2, 3, 4, 5);
                break;
            case AnimationState::RUN: 
                _action = bn::create_sprite_animate_action_forever(
                    *_sprite, 8, bn::sprite_items::spearguard.tiles_item(), 6, 7, 8, 9);
                break;
            case AnimationState::ATTACK: 
                _action = bn::create_sprite_animate_action_forever(
                    *_sprite, 6, bn::sprite_items::spearguard.tiles_item(), 10, 11, 12, 13, 14);
                break;
            case AnimationState::DEAD: 
                _action = bn::create_sprite_animate_action_once(
                    *_sprite, 8, bn::sprite_items::spearguard.tiles_item(),
                    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30);
                break;
            default: break; 
            }
        }
    }

    void Enemy::_update_health_bar() {
        if (_health_bar_sprite.has_value()) {
            int frame;
            if (_hp <= 0 || _dead) { frame = 3; }
            else if (_hp >= _max_hp) { frame = 0; }
            else {
                int health_slots = (_hp * 3) / _max_hp;
                if (health_slots == 0 && _hp > 0) health_slots = 1;
                frame = 3 - health_slots; 
            }
            bn::sprite_tiles_ptr new_tiles = bn::sprite_items::healthbar_enemy.tiles_item().create_tiles(frame);
            _health_bar_sprite->set_tiles(bn::move(new_tiles));
            _health_bar_sprite->set_visible(true); 
        }
    }

    void Enemy::_update_health_bar_position() {
        if (_health_bar_sprite.has_value()) { _health_bar_sprite->set_position(pos().x() - 3, pos().y() - 12); }
    }

    // =========================================================================
    // EnemyStateMachine Implementation
    // =========================================================================

    EnemyStateMachine::EnemyStateMachine()
        : _current_state(nullptr), _current_state_id(EnemyStateId::IDLE), _state_timer(0) {}

    EnemyStateMachine::EnemyStateMachine(EnemyStateMachine&& other) noexcept
        : _current_state(bn::move(other._current_state)),
          _current_state_id(other._current_state_id),
          _state_timer(other._state_timer) {
        other._current_state_id = EnemyStateId::IDLE;
        other._state_timer = 0; 
    }

    EnemyStateMachine& EnemyStateMachine::operator=(EnemyStateMachine&& other) noexcept {
        if (this != &other) {
            _current_state = bn::move(other._current_state);
            _current_state_id = other._current_state_id;
            _state_timer = other._state_timer;
            other._current_state_id = EnemyStateId::IDLE;
            other._state_timer = 0; 
        }
        return *this; 
    }

    void EnemyStateMachine::initialize(bn::unique_ptr<EnemyState> initial_state) {
        if (initial_state) {
            _current_state = bn::move(initial_state);
            _current_state_id = static_cast<EnemyStateId>(_current_state->get_state_id());
            _state_timer = 0; 
        }
    }

    void EnemyStateMachine::update(Enemy& enemy, bn::fixed_point player_pos, const Level& level, bool player_listening) {
        if (_current_state) {
            _current_state->update(enemy, player_pos, level, player_listening);
            _state_timer++; 
        }
    }

    void EnemyStateMachine::transition_to(Enemy& enemy, bn::unique_ptr<EnemyState> new_state) {
        if (!new_state) return;
        if (_current_state) { _current_state->exit(enemy); }
        _current_state = bn::move(new_state);
        _current_state_id = static_cast<EnemyStateId>(_current_state->get_state_id());
        _state_timer = 0;
        _current_state->enter(enemy); 
    }

    // =========================================================================
    // Enemy States Implementation
    // =========================================================================

    // --- IdleState ---
    void IdleState::enter(Enemy &enemy) {
        enemy._target_dx = 0;
        enemy._target_dy = 0;
        enemy._dx = 0;
        enemy._dy = 0; 
    }

    void IdleState::update(Enemy &enemy, bn::fixed_point player_pos, const Level &level, bool player_listening) {
        (void)level; // Unused
        bn::fixed dist_x = player_pos.x() - enemy.pos().x();
        bn::fixed dist_y = player_pos.y() - enemy.pos().y();
        bn::fixed dist_sq = dist_x * dist_x + dist_y * dist_y;
        const bn::fixed follow_dist_sq = 48 * 48;
        
        if (!player_listening && dist_sq <= follow_dist_sq) {
            bn::unique_ptr<ChaseState> chase_state = bn::make_unique<ChaseState>();
            enemy._state_machine.transition_to(enemy, bn::move(chase_state));
            return; 
        }
        
        if (enemy.type() != ENEMY_TYPE::SPEARGUARD && enemy._state_machine.get_state_timer() >= _idle_duration) {
            bn::unique_ptr<PatrolState> patrol_state = bn::make_unique<PatrolState>();
            enemy._state_machine.transition_to(enemy, bn::move(patrol_state));
            return; 
        }
        enemy._target_dx = 0;
        enemy._target_dy = 0; 
    }

    void IdleState::exit(Enemy &enemy) { (void)enemy; }

    // --- PatrolState ---
    void PatrolState::enter(Enemy &enemy) {
        (void)enemy;
        _direction_set = false;
        _target_dx = 0;
        _target_dy = 0; 
    }

    void PatrolState::update(Enemy &enemy, bn::fixed_point player_pos, const Level &level, bool player_listening) {
        (void)level;
        bn::fixed dist_x = player_pos.x() - enemy.pos().x();
        bn::fixed dist_y = player_pos.y() - enemy.pos().y();
        bn::fixed dist_sq = dist_x * dist_x + dist_y * dist_y;
        const bn::fixed follow_dist_sq = 48 * 48;
        
        if (!player_listening && dist_sq <= follow_dist_sq) {
            bn::unique_ptr<ChaseState> chase_state = bn::make_unique<ChaseState>();
            enemy._state_machine.transition_to(enemy, bn::move(chase_state));
            return; 
        }
        
        if (!_direction_set) {
            static bn::random random;
            int angle = random.get() % 360;
            bn::fixed radians = angle * 3.14159 / 180;
            _target_dx = 0.35 * bn::sin(radians);
            _target_dy = 0.35 * bn::cos(radians);
            _direction_set = true; 
        }
        enemy._target_dx = _target_dx;
        enemy._target_dy = _target_dy;
        
        if (enemy._state_machine.get_state_timer() >= _patrol_duration) {
            static bn::random random;
            int idle_duration = 20 + (random.get() % 40);
            bn::unique_ptr<IdleState> idle_state = bn::make_unique<IdleState>(idle_duration);
            enemy._state_machine.transition_to(enemy, bn::move(idle_state));
            return; 
        }
    }

    void PatrolState::exit(Enemy &enemy) {
        enemy._target_dx = 0;
        enemy._target_dy = 0; 
    }

    // --- ChaseState ---
    void ChaseState::enter(Enemy &enemy) { (void)enemy; }

    void ChaseState::update(Enemy &enemy, bn::fixed_point player_pos, const Level &level, bool player_listening) {
        (void)level;
        bn::fixed dist_x = player_pos.x() - enemy.pos().x();
        bn::fixed dist_y = player_pos.y() - enemy.pos().y();
        bn::fixed dist_sq = dist_x * dist_x + dist_y * dist_y;
        bn::fixed unfollow_dist_sq;
        
        if (enemy.type() == ENEMY_TYPE::SPEARGUARD && enemy._aggroed) { unfollow_dist_sq = 128 * 128; }
        else { unfollow_dist_sq = 64 * 64; }
        
        if (dist_sq > unfollow_dist_sq || player_listening) {
            if (enemy.type() == ENEMY_TYPE::SPEARGUARD) {
                bn::unique_ptr<ReturnToPostState> return_state = bn::make_unique<ReturnToPostState>();
                enemy._state_machine.transition_to(enemy, bn::move(return_state));
            } else {
                static bn::random random;
                int idle_duration = 20 + (random.get() % 40);
                bn::unique_ptr<IdleState> idle_state = bn::make_unique<IdleState>(idle_duration);
                enemy._state_machine.transition_to(enemy, bn::move(idle_state));
            }
            return; 
        }
        
        if (enemy.type() == ENEMY_TYPE::SPEARGUARD && enemy._attack_timer <= 0) {
            bn::fixed abs_dist_x = bn::abs(dist_x);
            bn::fixed abs_dist_y = bn::abs(dist_y);
            if (abs_dist_x <= ENEMY_ATTACK_DISTANCE && abs_dist_x >= abs_dist_y * 0.5 && abs_dist_y <= 16) {
                bn::unique_ptr<AttackState> attack_state = bn::make_unique<AttackState>();
                enemy._state_machine.transition_to(enemy, bn::move(attack_state));
                return; 
            }
        }
        
        bn::fixed len = bn::sqrt(dist_sq);
        if (len > 0.1) {
            if (enemy.type() == ENEMY_TYPE::SPEARGUARD) {
                bn::fixed abs_dist_y = bn::abs(dist_y);
                if (abs_dist_y > 8) {
                    enemy._target_dx = (dist_x / len) * _chase_speed * 0.3;
                    enemy._target_dy = (dist_y / len) * _chase_speed; 
                } else {
                    enemy._target_dx = (dist_x / len) * _chase_speed;
                    enemy._target_dy = (dist_y / len) * _chase_speed * 0.3; 
                }
            } else {
                enemy._target_dx = (dist_x / len) * _chase_speed;
                enemy._target_dy = (dist_y / len) * _chase_speed; 
            }
        } else {
            enemy._target_dx = 0;
            enemy._target_dy = 0; 
        }
    }

    void ChaseState::exit(Enemy &enemy) { (void)enemy; }

    // --- AttackState ---
    void AttackState::enter(Enemy &enemy) {
        enemy._attack_timer = _attack_duration;
        enemy._target_dx = 0;
        enemy._target_dy = 0; 
    }

    void AttackState::update(Enemy &enemy, bn::fixed_point player_pos, const Level &level, bool player_listening) {
        (void)level;
        enemy._target_dx = 0;
        enemy._target_dy = 0;
        if (enemy._attack_timer > 0) { enemy._attack_timer--; }
        
        if (enemy._attack_timer <= 0) {
            bn::fixed dist_x = player_pos.x() - enemy.pos().x();
            bn::fixed dist_y = player_pos.y() - enemy.pos().y();
            bn::fixed dist_sq = dist_x * dist_x + dist_y * dist_y;
            const bn::fixed follow_dist_sq = 48 * 48;
            
            if (!player_listening && dist_sq <= follow_dist_sq) {
                bn::unique_ptr<ChaseState> chase_state = bn::make_unique<ChaseState>();
                enemy._state_machine.transition_to(enemy, bn::move(chase_state));
            } else {
                if (enemy.type() == ENEMY_TYPE::SPEARGUARD) {
                    bn::unique_ptr<ReturnToPostState> return_state = bn::make_unique<ReturnToPostState>();
                    enemy._state_machine.transition_to(enemy, bn::move(return_state));
                } else {
                    bn::unique_ptr<IdleState> idle_state = bn::make_unique<IdleState>();
                    enemy._state_machine.transition_to(enemy, bn::move(idle_state));
                }
            }
        }
    }

    void AttackState::exit(Enemy &enemy) { enemy._attack_timer = 0; }

    // --- ReturnToPostState ---
    void ReturnToPostState::enter(Enemy &enemy) { (void)enemy; }

    void ReturnToPostState::update(Enemy &enemy, bn::fixed_point player_pos, const Level &level, bool player_listening) {
        (void)level;
        bn::fixed dist_to_post_x = enemy._original_position.x() - enemy.pos().x();
        bn::fixed dist_to_post_y = enemy._original_position.y() - enemy.pos().y();
        bn::fixed dist_to_post_sq = dist_to_post_x * dist_to_post_x + dist_to_post_y * dist_to_post_y;
        
        if (dist_to_post_sq <= _threshold * _threshold) {
            enemy.set_position(enemy._original_position);
            if (enemy.type() == ENEMY_TYPE::SPEARGUARD) { enemy._aggroed = false; }
            bn::unique_ptr<IdleState> idle_state = bn::make_unique<IdleState>();
            enemy._state_machine.transition_to(enemy, bn::move(idle_state));
            return; 
        }
        
        bn::fixed len = bn::sqrt(dist_to_post_sq);
        if (len > 0.1) {
            enemy._target_dx = (dist_to_post_x / len) * _return_speed;
            enemy._target_dy = (dist_to_post_y / len) * _return_speed; 
        } else {
            enemy._target_dx = 0;
            enemy._target_dy = 0; 
        }
        
        bn::fixed dist_x = player_pos.x() - enemy.pos().x();
        bn::fixed dist_y = player_pos.y() - enemy.pos().y();
        bn::fixed dist_sq = dist_x * dist_x + dist_y * dist_y;
        const bn::fixed follow_dist_sq = 48 * 48;
        
        if (!player_listening && dist_sq <= follow_dist_sq) {
            bn::unique_ptr<ChaseState> chase_state = bn::make_unique<ChaseState>();
            enemy._state_machine.transition_to(enemy, bn::move(chase_state));
            return; 
        }
    }

    void ReturnToPostState::exit(Enemy &enemy) { (void)enemy; }

    // --- StunnedState ---
    void StunnedState::enter(Enemy &enemy) {
        enemy._target_dx = 0;
        enemy._target_dy = 0; 
    }

    void StunnedState::update(Enemy &enemy, bn::fixed_point player_pos, const Level &level, bool player_listening) {
        (void)level;
        enemy._target_dx = 0;
        enemy._target_dy = 0;
        
        if (enemy._state_machine.get_state_timer() >= _stun_duration) {
            if (enemy.type() == ENEMY_TYPE::SPEARGUARD) {
                bn::unique_ptr<ChaseState> chase_state = bn::make_unique<ChaseState>();
                enemy._state_machine.transition_to(enemy, bn::move(chase_state));
                return; 
            }
            
            bn::fixed dist_x = player_pos.x() - enemy.pos().x();
            bn::fixed dist_y = player_pos.y() - enemy.pos().y();
            bn::fixed dist_sq = dist_x * dist_x + dist_y * dist_y;
            const bn::fixed follow_dist_sq = 48 * 48;
            
            if (!player_listening && dist_sq <= follow_dist_sq) {
                bn::unique_ptr<ChaseState> chase_state = bn::make_unique<ChaseState>();
                enemy._state_machine.transition_to(enemy, bn::move(chase_state));
            } else {
                bn::unique_ptr<IdleState> idle_state = bn::make_unique<IdleState>();
                enemy._state_machine.transition_to(enemy, bn::move(idle_state));
            }
        }
    }

    void StunnedState::exit(Enemy &enemy) { enemy._stunned = false; }

} // namespace fe
