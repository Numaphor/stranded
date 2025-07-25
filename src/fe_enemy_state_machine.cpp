#include "fe_enemy_state_machine.h"
#include "fe_enemy.h"
#include "bn_memory.h"

namespace fe
{
    EnemyStateMachine::EnemyStateMachine() 
        : _current_state(nullptr), _current_state_id(EnemyStateId::IDLE), _state_timer(0)
    {
    }

    EnemyStateMachine::EnemyStateMachine(EnemyStateMachine&& other) noexcept
        : _current_state(bn::move(other._current_state)), 
          _current_state_id(other._current_state_id), 
          _state_timer(other._state_timer)
    {
        // Reset the moved-from object
        other._current_state_id = EnemyStateId::IDLE;
        other._state_timer = 0;
    }

    EnemyStateMachine& EnemyStateMachine::operator=(EnemyStateMachine&& other) noexcept
    {
        if (this != &other)
        {
            _current_state = bn::move(other._current_state);
            _current_state_id = other._current_state_id;
            _state_timer = other._state_timer;
            
            // Reset the moved-from object
            other._current_state_id = EnemyStateId::IDLE;
            other._state_timer = 0;
        }
        return *this;
    }

    void EnemyStateMachine::initialize(bn::unique_ptr<EnemyState> initial_state)
    {
        if (initial_state)
        {
            _current_state = bn::move(initial_state);
            _current_state_id = static_cast<EnemyStateId>(_current_state->get_state_id());
            _state_timer = 0;
        }
    }

    void EnemyStateMachine::update(Enemy& enemy, bn::fixed_point player_pos, const Level& level, bool player_listening)
    {
        if (_current_state)
        {
            _current_state->update(enemy, player_pos, level, player_listening);
            _state_timer++;
        }
    }

    void EnemyStateMachine::transition_to(Enemy& enemy, bn::unique_ptr<EnemyState> new_state)
    {
        if (!new_state)
            return;

        // Exit current state
        if (_current_state)
        {
            _current_state->exit(enemy);
        }

        // Transition to new state
        _current_state = bn::move(new_state);
        _current_state_id = static_cast<EnemyStateId>(_current_state->get_state_id());
        _state_timer = 0;

        // Enter new state
        _current_state->enter(enemy);
    }
}