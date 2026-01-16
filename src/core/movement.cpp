#include "str_movement.h"
#include "str_constants.h"

#include "bn_fixed.h"
#include "bn_math.h"

namespace str
{

    // =========================================================================
    // Movement Implementation
    // =========================================================================

    Movement::Movement() : _dx(0), _dy(0), _current_state(State::IDLE), _facing_direction(Direction::DOWN) {}
    void Movement::move_right()
    {
        _dx += get_acc_const();
        _facing_direction = Direction::RIGHT;
        clamp_velocity();
        update_state();
    }
    void Movement::move_left()
    {
        _dx -= get_acc_const();
        _facing_direction = Direction::LEFT;
        clamp_velocity();
        update_state();
    }
    void Movement::move_up()
    {
        _dy -= get_acc_const();
        _facing_direction = Direction::UP;
        clamp_velocity();
        update_state();
    }
    void Movement::move_down()
    {
        _dy += get_acc_const();
        _facing_direction = Direction::DOWN;
        clamp_velocity();
        update_state();
    }
    void Movement::apply_friction()
    {
        _dx *= get_friction_const();
        _dy *= get_friction_const();
        if (bn::abs(_dx) < get_movement_threshold())
            _dx = 0;
        if (bn::abs(_dy) < get_movement_threshold())
            _dy = 0;
        update_state();
    }
    void Movement::reset()
    {
        _dx = _dy = 0;
        _current_state = State::IDLE;
        _facing_direction = Direction::DOWN;
    }
    void Movement::stop_movement()
    {
        _dx = _dy = 0;
        update_state();
    }
    void Movement::update_state() { _current_state = (_dx == 0 && _dy == 0) ? State::IDLE : State::WALKING; }
    void Movement::clamp_velocity()
    {
        bn::fixed m = get_max_speed();
        _dx = bn::clamp(_dx, -m, m);
        _dy = bn::clamp(_dy, -m, m);
    }

    EnemyMovement::EnemyMovement() : Movement() {}

} // namespace str
