#ifndef FE_ENEMY_STATES_H
#define FE_ENEMY_STATES_H

#include "fe_enemy_state.h"
#include "fe_constants.h"
#include "bn_fixed_point.h"
#include "bn_random.h"

namespace fe
{
    // Forward declarations
    class Enemy;
    class Level;

    /**
     * Idle state - enemy stands still and waits
     * Follows Single Responsibility Principle by only handling idle behavior
     */
    class IdleState : public EnemyState
    {
    private:
        int _idle_duration;
        
    public:
        IdleState(int duration = ENEMY_IDLE_DURATION_DEFAULT) : _idle_duration(duration) {}
        
        void enter(Enemy& enemy) override;
        void update(Enemy& enemy, bn::fixed_point player_pos, const Level& level, bool player_listening) override;
        void exit(Enemy& enemy) override;
        int get_state_id() const override { return static_cast<int>(EnemyStateId::IDLE); }
    };

    /**
     * Patrol state - enemy moves in a pattern or random direction
     * Handles wandering behavior for non-guard enemies
     */
    class PatrolState : public EnemyState
    {
    private:
        int _patrol_duration;
        bn::fixed _target_dx;
        bn::fixed _target_dy;
        bool _direction_set;
        
    public:
        PatrolState(int duration = ENEMY_PATROL_DURATION) : _patrol_duration(duration), _target_dx(0), _target_dy(0), _direction_set(false) {}
        
        void enter(Enemy& enemy) override;
        void update(Enemy& enemy, bn::fixed_point player_pos, const Level& level, bool player_listening) override;
        void exit(Enemy& enemy) override;
        int get_state_id() const override { return static_cast<int>(EnemyStateId::PATROL); }
    };

    /**
     * Chase state - enemy actively pursues the player
     * Handles aggressive behavior when player is detected
     */
    class ChaseState : public EnemyState
    {
    private:
        bn::fixed _chase_speed;
        
    public:
        ChaseState(bn::fixed speed = ENEMY_CHASE_SPEED) : _chase_speed(speed) {}
        
        void enter(Enemy& enemy) override;
        void update(Enemy& enemy, bn::fixed_point player_pos, const Level& level, bool player_listening) override;
        void exit(Enemy& enemy) override;
        int get_state_id() const override { return static_cast<int>(EnemyStateId::CHASE); }
    };

    /**
     * Attack state - enemy performs attack behavior
     * Handles combat actions for enemies that can attack
     */
    class AttackState : public EnemyState
    {
    private:
        int _attack_duration;
        
    public:
        AttackState(int duration = ENEMY_ATTACK_DURATION) : _attack_duration(duration) {}
        
        void enter(Enemy& enemy) override;
        void update(Enemy& enemy, bn::fixed_point player_pos, const Level& level, bool player_listening) override;
        void exit(Enemy& enemy) override;
        int get_state_id() const override { return static_cast<int>(EnemyStateId::ATTACK); }
    };

    /**
     * Return to post state - guard enemies return to their original position
     * Handles post-chase behavior for guard-type enemies
     */
    class ReturnToPostState : public EnemyState
    {
    private:
        bn::fixed _return_speed;
        bn::fixed _threshold;
        
    public:
        ReturnToPostState(bn::fixed speed = ENEMY_RETURN_SPEED, bn::fixed threshold = ENEMY_RETURN_THRESHOLD) 
            : _return_speed(speed), _threshold(threshold) {}
        
        void enter(Enemy& enemy) override;
        void update(Enemy& enemy, bn::fixed_point player_pos, const Level& level, bool player_listening) override;
        void exit(Enemy& enemy) override;
        int get_state_id() const override { return static_cast<int>(EnemyStateId::RETURN_TO_POST); }
    };

    /**
     * Stunned state - enemy is temporarily incapacitated
     * Handles knockback and recovery behavior
     */
    class StunnedState : public EnemyState
    {
    private:
        int _stun_duration;
        
    public:
        StunnedState(int duration = ENEMY_STUN_DURATION) : _stun_duration(duration) {}
        
        void enter(Enemy& enemy) override;
        void update(Enemy& enemy, bn::fixed_point player_pos, const Level& level, bool player_listening) override;
        void exit(Enemy& enemy) override;
        int get_state_id() const override { return static_cast<int>(EnemyStateId::STUNNED); }
    };
}

#endif