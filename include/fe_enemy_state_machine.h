#ifndef FE_ENEMY_STATE_MACHINE_H
#define FE_ENEMY_STATE_MACHINE_H

#include "fe_enemy_state.h"
#include "bn_unique_ptr.h"
#include "bn_memory.h"

namespace fe
{
    // Forward declarations
    class Enemy;
    class Level;

    /**
     * State machine context class following Dependency Inversion Principle.
     * Manages state transitions without depending on concrete state implementations.
     */
    class EnemyStateMachine
    {
    private:
        bn::unique_ptr<EnemyState> _current_state;
        EnemyStateId _current_state_id;
        int _state_timer;
        
    public:
        EnemyStateMachine();
        ~EnemyStateMachine() = default;
        
        /**
         * Initialize the state machine with the initial state
         * @param initial_state Initial state for the enemy
         */
        void initialize(bn::unique_ptr<EnemyState> initial_state);
        
        /**
         * Update the current state
         * @param enemy The enemy this state machine belongs to
         * @param player_pos Current player position
         * @param level Current level data
         * @param player_listening Whether player is listening to NPCs
         */
        void update(Enemy& enemy, bn::fixed_point player_pos, const Level& level, bool player_listening);
        
        /**
         * Transition to a new state
         * @param enemy The enemy transitioning states
         * @param new_state The new state to transition to
         */
        void transition_to(Enemy& enemy, bn::unique_ptr<EnemyState> new_state);
        
        /**
         * Get the current state ID
         * @return Current state identifier
         */
        EnemyStateId get_current_state_id() const { return _current_state_id; }
        
        /**
         * Get the time spent in current state
         * @return Number of frames in current state
         */
        int get_state_timer() const { return _state_timer; }
        
        /**
         * Reset the state timer (useful for state transitions)
         */
        void reset_state_timer() { _state_timer = 0; }
    };
}

#endif