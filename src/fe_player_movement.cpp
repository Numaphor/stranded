#include "fe_player.h"
#include "fe_constants.h"

namespace fe
{
    // PlayerMovement Implementation
    PlayerMovement::PlayerMovement() : _dx(0), _dy(0), _current_state(State::IDLE), _facing_direction(Direction::DOWN), _action_timer(0)
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
            _current_state = (bn::abs(_dx) > movement_threshold || bn::abs(_dy) > movement_threshold) ? State::WALKING : State::IDLE;
        }
    }

    void PlayerMovement::start_rolling()
    {
        start_action(State::ROLLING, PLAYER_ROLL_DURATION);
    }

    void PlayerMovement::start_chopping()
    {
        start_action(State::CHOPPING, PLAYER_CHOP_DURATION);
    }

    void PlayerMovement::start_slashing()
    {
        start_action(State::SLASHING, PLAYER_SLASH_DURATION);
    }

    void PlayerMovement::start_attacking()
    {
        start_action(State::ATTACKING, PLAYER_ATTACK_DURATION);
    }

    void PlayerMovement::start_buff(State buff_type)
    {
        start_action(buff_type, PLAYER_BUFF_DURATION);
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
}
