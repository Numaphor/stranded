#include "str_enemy.h"
#include "str_enemy_states.h"
#include "str_enemy_state_machine.h"
#include "str_level.h"
#include "str_enemy_type.h"
#include "str_constants.h"

#include "bn_math.h"
#include "bn_sprite_builder.h"
#include "bn_sprite_animate_actions.h"
#include "bn_fixed_point.h"
#include "bn_camera_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_random.h"

#include "bn_sprite_items_healthbar_enemy.h"
#include "bn_sprite_items_spearguard.h"

namespace str
{

    // =========================================================================
    // Enemy Implementation
    // =========================================================================

    constexpr int HEALTHBAR_Z_ORDER = -1000;

    Enemy::Enemy(int x, int y, bn::camera_ptr camera, bn::regular_bg_ptr map, ENEMY_TYPE type, int hp)
        : Entity(bn::fixed_point(x, y)), _camera(camera), _type(type), _hp(hp), _max_hp(hp), _map(map), _map_cells(map.map().cells_ref().value())
    {
        bn::sprite_builder builder(bn::sprite_items::spearguard);
        builder.set_position(pos());
        builder.set_bg_priority(1);
        _sprite = builder.build();
        if (!_sprite)
            return;
        set_camera(_camera);
        _hitbox = Hitbox(pos().x() - 4, pos().y() - 4, 8, 8);
        if (_type == ENEMY_TYPE::SPEARGUARD)
            _original_position = pos();
        _action = bn::create_sprite_animate_action_forever(*_sprite, _type == ENEMY_TYPE::SPEARGUARD ? 12 : 8, bn::sprite_items::spearguard.tiles_item(), 0, 1, 2, 3, 4, 5);
        _state_machine.initialize(bn::make_unique<IdleState>());
        _health_bar_sprite = bn::sprite_items::healthbar_enemy.create_sprite(pos().x(), pos().y() - 20, 0);
        _health_bar_sprite->set_camera(_camera);
        _health_bar_sprite->set_bg_priority(3);
        _health_bar_sprite->set_z_order(HEALTHBAR_Z_ORDER);
        _update_health_bar();
    }

    Enemy::Enemy(Enemy &&other) noexcept = default;
    Enemy &Enemy::operator=(Enemy &&other) noexcept = default;

    void Enemy::update_hitbox()
    {
        _hitbox.set_x(pos().x() - 4);
        _hitbox.set_y(pos().y() - 4);
    }

    void Enemy::update(bn::fixed_point player_pos, const Level &level, bool player_listening)
    {
        if (_knockback_timer > 0)
        {
            _knockback_timer--;
            set_position(pos() + bn::fixed_point(_knockback_dx, _knockback_dy));
            _knockback_dx *= 0.9;
            _knockback_dy *= 0.9;
            if (_knockback_timer == 0)
            {
                _knockback_dx = _knockback_dy = 0;
                _stunned = false;
                _state_machine.transition_to(*this, bn::make_unique<StunnedState>());
            }
            update_hitbox();
            return;
        }
        if (!_dead)
        {
            _state_machine.update(*this, player_pos, level, player_listening);
            _dx = _target_dx;
            _dy = _target_dy;
            _movement.set_velocity(bn::fixed_point(_dx, _dy));
            _movement.update();
            set_position(pos() + bn::fixed_point(_dx, _dy));
        }
        else
        {
            _dx = _dy = _target_dx = _target_dy = 0;
            _movement.set_velocity({0, 0});
            if (_death_timer > 0)
                _death_timer--;
        }
        if (_invulnerable && !_dead)
        {
            if (--_inv_timer <= 0)
            {
                _invulnerable = false;
                _inv_timer = 0;
                if (_sprite)
                    _sprite->set_visible(true);
            }
        }
        else if (_dead && _sprite)
            _sprite->set_visible(true);
        _update_spearguard_animation();
        if (_sprite)
        {
            _sprite->set_position(pos());
            _sprite->set_horizontal_flip(_dx < 0);
            if (_action && !_action->done())
                _action->update();
        }
        _update_health_bar_position();
        update_hitbox();
    }

    bool Enemy::_take_damage(int damage)
    {
        if (_invulnerable || _dead)
            return false;
        _hp -= damage;
        _invulnerable = true;
        _inv_timer = 30;
        _stunned = true;
        if (_type == ENEMY_TYPE::SPEARGUARD)
            _aggroed = true;
        _update_health_bar();
        if (_hp <= 0)
        {
            _dead = true;
            _death_timer = ENEMY_DEATH_ANIMATION_DURATION;
        }
        return true;
    }

    void Enemy::_apply_knockback(bn::fixed dx, bn::fixed dy)
    {
        _knockback_dx = dx * ENEMY_KNOCKBACK_STRENGTH;
        _knockback_dy = dy * ENEMY_KNOCKBACK_STRENGTH;
        _knockback_timer = ENEMY_KNOCKBACK_DURATION;
        _stunned = true;
    }

    bool Enemy::damage_from_left(int damage)
    {
        if (_take_damage(damage))
        {
            _apply_knockback(1.0, -0.5);
            return true;
        }
        return false;
    }
    bool Enemy::damage_from_right(int damage)
    {
        if (_take_damage(damage))
        {
            _apply_knockback(-1.0, -0.5);
            return true;
        }
        return false;
    }

    bool Enemy::is_hit(Hitbox) { return false; }
    bool Enemy::is_vulnerable() { return !_invulnerable; }

    void Enemy::set_visible(bool visibility)
    {
        if (_sprite.has_value())
        {
            _sprite->set_visible(visibility);
        }
    }

    bool Enemy::spotted_player() { return _spotted_player; }
    int Enemy::hp() { return _hp; }
    ENEMY_TYPE Enemy::type() { return _type; }
    bool Enemy::is_ready_for_removal() { return _dead && _death_timer <= 0; }
    bool Enemy::is_chasing() const { return _state_machine.get_current_state_id() == EnemyStateId::CHASE; }

    void Enemy::_update_spearguard_animation()
    {
        if (_type != ENEMY_TYPE::SPEARGUARD || !_sprite)
            return;
        AnimationState da = AnimationState::IDLE;
        if (_dead)
            da = AnimationState::DEAD;
        else if (_attack_timer > 0)
        {
            da = AnimationState::ATTACK;
            _attack_timer--;
        }
        else
        {
            EnemyStateId sid = _state_machine.get_current_state_id();
            if (sid == EnemyStateId::CHASE || sid == EnemyStateId::PATROL || sid == EnemyStateId::RETURN_TO_POST)
                da = AnimationState::RUN;
        }
        if (da != _current_animation)
        {
            _current_animation = da;
            auto s = bn::sprite_items::spearguard.tiles_item();
            auto m = [&](int sp, auto... f)
            { _action = bn::create_sprite_animate_action_forever(*_sprite, sp, s, f...); };
            if (da == AnimationState::IDLE)
                m(12, 0, 1, 2, 3, 4, 5);
            else if (da == AnimationState::RUN)
                m(8, 6, 7, 8, 9);
            else if (da == AnimationState::ATTACK)
                m(6, 10, 11, 12, 13, 14);
            else if (da == AnimationState::DEAD)
                _action = bn::create_sprite_animate_action_once(*_sprite, 8, s, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30);
        }
    }

    void Enemy::_update_health_bar()
    {
        if (_health_bar_sprite.has_value())
        {
            int frame;
            if (_hp <= 0 || _dead)
            {
                frame = 3;
            }
            else if (_hp >= _max_hp)
            {
                frame = 0;
            }
            else
            {
                int health_slots = (_hp * 3) / _max_hp;
                if (health_slots == 0 && _hp > 0)
                    health_slots = 1;
                frame = 3 - health_slots;
            }
            bn::sprite_tiles_ptr new_tiles = bn::sprite_items::healthbar_enemy.tiles_item().create_tiles(frame);
            _health_bar_sprite->set_tiles(bn::move(new_tiles));
            _health_bar_sprite->set_visible(true);
        }
    }

    void Enemy::_update_health_bar_position()
    {
        if (_health_bar_sprite.has_value())
        {
            _health_bar_sprite->set_position(pos().x() - 3, pos().y() - 12);
        }
    }

    // =========================================================================
    // EnemyStateMachine Implementation
    // =========================================================================

    EnemyStateMachine::EnemyStateMachine()
        : _current_state(nullptr), _current_state_id(EnemyStateId::IDLE), _state_timer(0) {}

    EnemyStateMachine::EnemyStateMachine(EnemyStateMachine &&other) noexcept = default;
    EnemyStateMachine &EnemyStateMachine::operator=(EnemyStateMachine &&other) noexcept = default;

    void EnemyStateMachine::initialize(bn::unique_ptr<EnemyState> initial_state)
    {
        if (initial_state)
        {
            _current_state = bn::move(initial_state);
            _current_state_id = static_cast<EnemyStateId>(_current_state->get_state_id());
            _state_timer = 0;
        }
    }

    void EnemyStateMachine::update(Enemy &enemy, bn::fixed_point player_pos, const Level &level, bool player_listening)
    {
        if (_current_state)
        {
            _current_state->update(enemy, player_pos, level, player_listening);
            _state_timer++;
        }
    }

    void EnemyStateMachine::transition_to(Enemy &enemy, bn::unique_ptr<EnemyState> new_state)
    {
        if (!new_state)
            return;
        if (_current_state)
            _current_state->exit(enemy);
        _current_state = bn::move(new_state);
        _current_state_id = static_cast<EnemyStateId>(_current_state->get_state_id());
        _state_timer = 0;
        _current_state->enter(enemy);
    }

    // =========================================================================
    // Enemy States Implementation
    // =========================================================================

    // --- IdleState ---
    void IdleState::enter(Enemy &e) { e._target_dx = e._target_dy = e._dx = e._dy = 0; }
    void IdleState::update(Enemy &enemy, bn::fixed_point p_pos, const Level &, bool listening)
    {
        bn::fixed d_sq = (p_pos - enemy.pos()).x() * (p_pos - enemy.pos()).x() + (p_pos - enemy.pos()).y() * (p_pos - enemy.pos()).y();
        if (!listening && d_sq <= 48 * 48)
        {
            enemy._state_machine.transition_to(enemy, bn::make_unique<ChaseState>());
            return;
        }
        if (enemy.type() != ENEMY_TYPE::SPEARGUARD && enemy._state_machine.get_state_timer() >= _idle_duration)
        {
            enemy._state_machine.transition_to(enemy, bn::make_unique<PatrolState>());
            return;
        }
        enemy._target_dx = enemy._target_dy = 0;
    }
    void IdleState::exit(Enemy &) {}

    // --- PatrolState ---
    void PatrolState::enter(Enemy &)
    {
        _direction_set = 0;
        _target_dx = _target_dy = 0;
    }
    void PatrolState::update(Enemy &enemy, bn::fixed_point p_pos, const Level &, bool listening)
    {
        bn::fixed d_sq = (p_pos - enemy.pos()).x() * (p_pos - enemy.pos()).x() + (p_pos - enemy.pos()).y() * (p_pos - enemy.pos()).y();
        if (!listening && d_sq <= 48 * 48)
        {
            enemy._state_machine.transition_to(enemy, bn::make_unique<ChaseState>());
            return;
        }
        if (!_direction_set)
        {
            static bn::random r;
            int a = r.get() % 360;
            bn::fixed rad = a * 3.14159 / 180;
            _target_dx = 0.35 * bn::sin(rad);
            _target_dy = 0.35 * bn::cos(rad);
            _direction_set = 1;
        }
        enemy._target_dx = _target_dx;
        enemy._target_dy = _target_dy;
        if (enemy._state_machine.get_state_timer() >= _patrol_duration)
        {
            static bn::random r;
            enemy._state_machine.transition_to(enemy, bn::make_unique<IdleState>(20 + (r.get() % 40)));
        }
    }
    void PatrolState::exit(Enemy &e) { e._target_dx = e._target_dy = 0; }

    // --- ChaseState ---
    void ChaseState::enter(Enemy &) {}
    void ChaseState::update(Enemy &enemy, bn::fixed_point p_pos, const Level &, bool listening)
    {
        bn::fixed dx_to_player = p_pos.x() - enemy.pos().x();
        bn::fixed dy_to_player = p_pos.y() - enemy.pos().y();
        bn::fixed d_sq = dx_to_player * dx_to_player + dy_to_player * dy_to_player;
        bn::fixed uf_sq = (enemy.type() == ENEMY_TYPE::SPEARGUARD && enemy._aggroed) ? 128 * 128 : 64 * 64;

        if (d_sq > uf_sq || listening)
        {
            if (enemy.type() == ENEMY_TYPE::SPEARGUARD)
                enemy._state_machine.transition_to(enemy, bn::make_unique<ReturnToPostState>());
            else
            {
                static bn::random r;
                enemy._state_machine.transition_to(enemy, bn::make_unique<IdleState>(20 + (r.get() % 40)));
            }
            return;
        }

        if (enemy.type() == ENEMY_TYPE::SPEARGUARD && enemy._attack_timer <= 0)
        {
            if (bn::abs(dx_to_player) <= ENEMY_ATTACK_DISTANCE &&
                bn::abs(dx_to_player) >= bn::abs(dy_to_player) * 0.5 &&
                bn::abs(dy_to_player) <= 16)
            {
                enemy._state_machine.transition_to(enemy, bn::make_unique<AttackState>());
                return;
            }
        }

        bn::fixed len = bn::sqrt(d_sq);
        if (len > 0.1)
        {
            bn::fixed dir_x = dx_to_player / len;
            bn::fixed dir_y = dy_to_player / len;
            bn::fixed f = (enemy.type() == ENEMY_TYPE::SPEARGUARD && bn::abs(dy_to_player) > 8) ? bn::fixed(1.0) : bn::fixed(0.3);
            bn::fixed x_mult = (f == bn::fixed(1.0)) ? bn::fixed(0.3) : bn::fixed(1.0);
            enemy._target_dx = dir_x * _chase_speed * x_mult;
            enemy._target_dy = dir_y * _chase_speed * f;
        }
        else
        {
            enemy._target_dx = enemy._target_dy = 0;
        }
    }
    void ChaseState::exit(Enemy &) {}

    // --- AttackState ---
    void AttackState::enter(Enemy &e)
    {
        e._attack_timer = _attack_duration;
        e._target_dx = e._target_dy = 0;
    }
    void AttackState::update(Enemy &enemy, bn::fixed_point p_pos, const Level &, bool listening)
    {
        enemy._target_dx = enemy._target_dy = 0;
        if (enemy._attack_timer > 0)
            enemy._attack_timer--;
        if (enemy._attack_timer <= 0)
        {
            bn::fixed d_sq = (p_pos - enemy.pos()).x() * (p_pos - enemy.pos()).x() + (p_pos - enemy.pos()).y() * (p_pos - enemy.pos()).y();
            if (!listening && d_sq <= 48 * 48)
                enemy._state_machine.transition_to(enemy, bn::make_unique<ChaseState>());
            else
                enemy._state_machine.transition_to(enemy, enemy.type() == ENEMY_TYPE::SPEARGUARD ? (bn::unique_ptr<EnemyState>)bn::make_unique<ReturnToPostState>() : (bn::unique_ptr<EnemyState>)bn::make_unique<IdleState>());
        }
    }
    void AttackState::exit(Enemy &e) { e._attack_timer = 0; }

    // --- ReturnToPostState ---
    void ReturnToPostState::enter(Enemy &) {}
    void ReturnToPostState::update(Enemy &enemy, bn::fixed_point p_pos, const Level &, bool listening)
    {
        bn::fixed_point off = enemy._original_position - enemy.pos();
        bn::fixed d_sq = off.x() * off.x() + off.y() * off.y();
        if (d_sq <= _threshold * _threshold)
        {
            enemy.set_position(enemy._original_position);
            if (enemy.type() == ENEMY_TYPE::SPEARGUARD)
                enemy._aggroed = 0;
            enemy._state_machine.transition_to(enemy, bn::make_unique<IdleState>());
            return;
        }
        bn::fixed len = bn::sqrt(d_sq);
        if (len > 0.1)
        {
            enemy._target_dx = (off.x() / len) * _return_speed;
            enemy._target_dy = (off.y() / len) * _return_speed;
        }
        else
            enemy._target_dx = enemy._target_dy = 0;
        if (!listening && (p_pos - enemy.pos()).x() * (p_pos - enemy.pos()).x() + (p_pos - enemy.pos()).y() * (p_pos - enemy.pos()).y() <= 48 * 48)
            enemy._state_machine.transition_to(enemy, bn::make_unique<ChaseState>());
    }
    void ReturnToPostState::exit(Enemy &) {}

    // --- StunnedState ---
    void StunnedState::enter(Enemy &e) { e._target_dx = e._target_dy = 0; }
    void StunnedState::update(Enemy &enemy, bn::fixed_point p_pos, const Level &, bool listening)
    {
        enemy._target_dx = enemy._target_dy = 0;
        if (enemy._state_machine.get_state_timer() >= _stun_duration)
        {
            if (enemy.type() == ENEMY_TYPE::SPEARGUARD || (!listening && (p_pos - enemy.pos()).x() * (p_pos - enemy.pos()).x() + (p_pos - enemy.pos()).y() * (p_pos - enemy.pos()).y() <= 48 * 48))
                enemy._state_machine.transition_to(enemy, bn::make_unique<ChaseState>());
            else
                enemy._state_machine.transition_to(enemy, bn::make_unique<IdleState>());
        }
    }
    void StunnedState::exit(Enemy &e) { e._stunned = 0; }

} // namespace str
