#ifndef FE_ENEMY_STATE_H
#define FE_ENEMY_STATE_H

#include "bn_fixed_point.h"

namespace fe
{
    // Forward declarations
    class Enemy;
    class Level;

    /**
     * Abstract base class for enemy states following Single Responsibility Principle.
     * Each concrete state handles one specific behavior (idle, patrol, chase, etc.)
     */
    class EnemyState
    {
    public:
        virtual ~EnemyState() = default;
        
        /**
         * Called when the state is entered
         * @param enemy The enemy entering this state
         */
        virtual void enter(Enemy& enemy) = 0;
        
        /**
         * Called every frame while the state is active
         * @param enemy The enemy in this state
         * @param player_pos Current player position
         * @param level Current level data
         * @param player_listening Whether player is listening to NPCs
         */
        virtual void update(Enemy& enemy, bn::fixed_point player_pos, const Level& level, bool player_listening) = 0;
        
        /**
         * Called when the state is exited
         * @param enemy The enemy exiting this state
         */
        virtual void exit(Enemy& enemy) = 0;
        
        /**
         * Get the unique identifier for this state type
         * @return State type identifier
         */
        virtual int get_state_id() const = 0;
    };

    /**
     * State identifiers for different enemy states
     */
    enum class EnemyStateId
    {
        IDLE = 0,
        PATROL = 1,
        CHASE = 2,
        ATTACK = 3,
        RETURN_TO_POST = 4,
        STUNNED = 5
    };
}

#endif