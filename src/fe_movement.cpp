#include "fe_movement.h"
#include "bn_math.h"

namespace fe
{
    Movement::Movement() : _dx(0), _dy(0), _current_state(State::IDLE), _facing_direction(Direction::DOWN)
    {
    }

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
        {
            _dx = 0;
        }
        if (bn::abs(_dy) < get_movement_threshold())
        {
            _dy = 0;
        }

        update_state();
    }

    void Movement::reset()
    {
        _dx = 0;
        _dy = 0;
        _current_state = State::IDLE;
        _facing_direction = Direction::DOWN;
    }

    void Movement::stop_movement()
    {
        _dx = 0;
        _dy = 0;
        update_state();
    }

    void Movement::update_state()
    {
        if (_dx == 0 && _dy == 0)
        {
            _current_state = State::IDLE;
        }
        else
        {
            _current_state = State::WALKING;
        }
    }

    void Movement::clamp_velocity()
    {
        bn::fixed max_speed = get_max_speed();
        
        if (_dx > max_speed)
        {
            _dx = max_speed;
        }
        else if (_dx < -max_speed)
        {
            _dx = -max_speed;
        }

        if (_dy > max_speed)
        {
            _dy = max_speed;
        }
        else if (_dy < -max_speed)
        {
            _dy = -max_speed;
        }
    }


    EnemyMovement::EnemyMovement() : Movement()
    {
    }
}
