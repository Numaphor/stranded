#include "fe_enemy_states.h"
#include "fe_enemy.h"
#include "fe_enemy_type.h"
#include "fe_level.h"
#include "fe_enemy_state_machine.h"
#include "bn_log.h"
#include "bn_random.h"
#include "bn_memory.h"
#include "bn_math.h"

namespace fe
{
    constexpr bn::fixed ATTACK_DISTANCE = 20;

    // IdleState implementation
    void IdleState::enter(Enemy &enemy)
    {
        // Stop movement when entering idle
        enemy._target_dx = 0;
        enemy._target_dy = 0;
        enemy._dx = 0;
        enemy._dy = 0;
    }

    void IdleState::update(Enemy &enemy, bn::fixed_point player_pos, const Level &level, bool player_listening)
    {
        // Calculate distance to player for state transitions
        bn::fixed dist_x = player_pos.x() - enemy.pos().x();
        bn::fixed dist_y = player_pos.y() - enemy.pos().y();
        bn::fixed dist_sq = dist_x * dist_x + dist_y * dist_y;
        const bn::fixed follow_dist_sq = 48 * 48; // 6 tiles squared

        // Check if player should be chased (only if not listening to NPCs)
        if (!player_listening && dist_sq <= follow_dist_sq)
        {
            // Transition to chase state
            bn::unique_ptr<ChaseState> chase_state = bn::make_unique<ChaseState>();
            enemy._state_machine.transition_to(enemy, bn::move(chase_state));
            return;
        }

        // Check if idle time is up and should start patrolling (for non-guard enemies)
        if (enemy.type() != ENEMY_TYPE::SPEARGUARD && enemy._state_machine.get_state_timer() >= _idle_duration)
        {
            // Transition to patrol state
            bn::unique_ptr<PatrolState> patrol_state = bn::make_unique<PatrolState>();
            enemy._state_machine.transition_to(enemy, bn::move(patrol_state));
            return;
        }

        // Stay idle - maintain zero velocity
        enemy._target_dx = 0;
        enemy._target_dy = 0;
    }

    void IdleState::exit(Enemy &enemy)
    {
        // No cleanup needed for idle state
    }

    // PatrolState implementation
    void PatrolState::enter(Enemy &enemy)
    {
        _direction_set = false;
        _target_dx = 0;
        _target_dy = 0;
    }

    void PatrolState::update(Enemy &enemy, bn::fixed_point player_pos, const Level &level, bool player_listening)
    {
        // Calculate distance to player for state transitions
        bn::fixed dist_x = player_pos.x() - enemy.pos().x();
        bn::fixed dist_y = player_pos.y() - enemy.pos().y();
        bn::fixed dist_sq = dist_x * dist_x + dist_y * dist_y;
        const bn::fixed follow_dist_sq = 48 * 48; // 6 tiles squared

        // Check if player should be chased (only if not listening to NPCs)
        if (!player_listening && dist_sq <= follow_dist_sq)
        {
            // Transition to chase state
            bn::unique_ptr<ChaseState> chase_state = bn::make_unique<ChaseState>();
            enemy._state_machine.transition_to(enemy, bn::move(chase_state));
            return;
        }

        // Set random direction if not already set
        if (!_direction_set)
        {
            static bn::random random;
            int angle = random.get() % 360;
            bn::fixed radians = angle * 3.14159 / 180;
            _target_dx = 0.35 * bn::sin(radians);
            _target_dy = 0.35 * bn::cos(radians);
            _direction_set = true;
        }

        // Apply movement target
        enemy._target_dx = _target_dx;
        enemy._target_dy = _target_dy;

        // Check if patrol time is up
        if (enemy._state_machine.get_state_timer() >= _patrol_duration)
        {
            // Transition back to idle state
            static bn::random random;
            int idle_duration = 20 + (random.get() % 40);
            bn::unique_ptr<IdleState> idle_state = bn::make_unique<IdleState>(idle_duration);
            enemy._state_machine.transition_to(enemy, bn::move(idle_state));
            return;
        }
    }

    void PatrolState::exit(Enemy &enemy)
    {
        // Stop movement when exiting patrol
        enemy._target_dx = 0;
        enemy._target_dy = 0;
    }

    // ChaseState implementation
    void ChaseState::enter(Enemy &enemy)
    {
        // No special setup needed for chase
    }

    void ChaseState::update(Enemy &enemy, bn::fixed_point player_pos, const Level &level, bool player_listening)
    {
        // Calculate distance to player
        bn::fixed dist_x = player_pos.x() - enemy.pos().x();
        bn::fixed dist_y = player_pos.y() - enemy.pos().y();
        bn::fixed dist_sq = dist_x * dist_x + dist_y * dist_y;
        const bn::fixed unfollow_dist_sq = 64 * 64; // 8 tiles squared

        // Check if player is too far away or listening to NPCs
        if (dist_sq > unfollow_dist_sq || player_listening)
        {
            // Decide what to do based on enemy type
            if (enemy.type() == ENEMY_TYPE::SPEARGUARD)
            {
                // Spearguards return to their post
                bn::unique_ptr<ReturnToPostState> return_state = bn::make_unique<ReturnToPostState>();
                enemy._state_machine.transition_to(enemy, bn::move(return_state));
            }
            else
            {
                // Other enemies go idle
                static bn::random random;
                int idle_duration = 20 + (random.get() % 40);
                bn::unique_ptr<IdleState> idle_state = bn::make_unique<IdleState>(idle_duration);
                enemy._state_machine.transition_to(enemy, bn::move(idle_state));
            }
            return;
        }

        // Check if close enough to attack (spearguards only)
        if (enemy.type() == ENEMY_TYPE::SPEARGUARD && enemy._attack_timer <= 0)
        {
            // Spearguards should only attack when player is to their left or right
            // since the spear extends horizontally, not vertically
            bn::fixed abs_dist_x = bn::abs(dist_x);
            bn::fixed abs_dist_y = bn::abs(dist_y);

            // Only attack if the horizontal distance is significant relative to vertical distance
            // and the player is within reasonable attack range horizontally
            // Also ensure we're roughly Y-aligned (vertical distance is small)
            if (abs_dist_x <= ATTACK_DISTANCE && abs_dist_x >= abs_dist_y * 0.5 && abs_dist_y <= 16)
            {
                // Transition to attack state
                bn::unique_ptr<AttackState> attack_state = bn::make_unique<AttackState>();
                enemy._state_machine.transition_to(enemy, bn::move(attack_state));
                return;
            }
        }

        // Move toward player (spearguards prioritize getting to the same Y level for horizontal attacks)
        bn::fixed len = bn::sqrt(dist_sq);
        if (len > 0.1)
        {
            if (enemy.type() == ENEMY_TYPE::SPEARGUARD)
            {
                // Spearguards prioritize vertical alignment first, then horizontal positioning
                bn::fixed abs_dist_x = bn::abs(dist_x);
                bn::fixed abs_dist_y = bn::abs(dist_y);

                // If not Y-aligned, prioritize vertical movement
                if (abs_dist_y > 8) // Allow small Y tolerance
                {
                    enemy._target_dx = (dist_x / len) * _chase_speed * 0.3; // Slower horizontal movement
                    enemy._target_dy = (dist_y / len) * _chase_speed;       // Full vertical movement
                }
                else
                {
                    // Y-aligned, now move horizontally to attack position
                    enemy._target_dx = (dist_x / len) * _chase_speed;
                    enemy._target_dy = (dist_y / len) * _chase_speed * 0.3; // Maintain Y position
                }
            }
            else
            {
                // Other enemies move normally
                enemy._target_dx = (dist_x / len) * _chase_speed;
                enemy._target_dy = (dist_y / len) * _chase_speed;
            }
        }
        else
        {
            enemy._target_dx = 0;
            enemy._target_dy = 0;
        }
    }

    void ChaseState::exit(Enemy &enemy)
    {
        // No special cleanup needed
    }

    // AttackState implementation
    void AttackState::enter(Enemy &enemy)
    {
        // Start attack animation and stop movement
        enemy._attack_timer = _attack_duration;
        enemy._target_dx = 0;
        enemy._target_dy = 0;
    }

    void AttackState::update(Enemy &enemy, bn::fixed_point player_pos, const Level &level, bool player_listening)
    {
        // Keep enemy stationary during attack
        enemy._target_dx = 0;
        enemy._target_dy = 0;

        // Decrement attack timer
        if (enemy._attack_timer > 0)
        {
            enemy._attack_timer--;
        }

        // Check if attack is finished
        if (enemy._attack_timer <= 0)
        {
            // Check if player is still in range to continue chasing
            bn::fixed dist_x = player_pos.x() - enemy.pos().x();
            bn::fixed dist_y = player_pos.y() - enemy.pos().y();
            bn::fixed dist_sq = dist_x * dist_x + dist_y * dist_y;
            const bn::fixed follow_dist_sq = 48 * 48; // 6 tiles squared

            if (!player_listening && dist_sq <= follow_dist_sq)
            {
                // Continue chasing
                bn::unique_ptr<ChaseState> chase_state = bn::make_unique<ChaseState>();
                enemy._state_machine.transition_to(enemy, bn::move(chase_state));
            }
            else
            {
                // Player is too far or listening, decide what to do based on enemy type
                if (enemy.type() == ENEMY_TYPE::SPEARGUARD)
                {
                    // Return to post
                    bn::unique_ptr<ReturnToPostState> return_state = bn::make_unique<ReturnToPostState>();
                    enemy._state_machine.transition_to(enemy, bn::move(return_state));
                }
                else
                {
                    // Go idle
                    bn::unique_ptr<IdleState> idle_state = bn::make_unique<IdleState>();
                    enemy._state_machine.transition_to(enemy, bn::move(idle_state));
                }
            }
        }
    }

    void AttackState::exit(Enemy &enemy)
    {
        // Reset attack timer
        enemy._attack_timer = 0;
    }

    // ReturnToPostState implementation
    void ReturnToPostState::enter(Enemy &enemy)
    {
        // No special setup needed
    }

    void ReturnToPostState::update(Enemy &enemy, bn::fixed_point player_pos, const Level &level, bool player_listening)
    {
        // Calculate distance to original position
        bn::fixed dist_to_post_x = enemy._original_position.x() - enemy.pos().x();
        bn::fixed dist_to_post_y = enemy._original_position.y() - enemy.pos().y();
        bn::fixed dist_to_post_sq = dist_to_post_x * dist_to_post_x + dist_to_post_y * dist_to_post_y;

        // Check if close enough to original position
        if (dist_to_post_sq <= _threshold * _threshold)
        {
            // Snap to exact position and go idle
            enemy.set_position(enemy._original_position);
            bn::unique_ptr<IdleState> idle_state = bn::make_unique<IdleState>();
            enemy._state_machine.transition_to(enemy, bn::move(idle_state));
            return;
        }

        // Move toward original position
        bn::fixed len = bn::sqrt(dist_to_post_sq);
        if (len > 0.1)
        {
            enemy._target_dx = (dist_to_post_x / len) * _return_speed;
            enemy._target_dy = (dist_to_post_y / len) * _return_speed;
        }
        else
        {
            enemy._target_dx = 0;
            enemy._target_dy = 0;
        }

        // Check if player comes back in range while returning
        bn::fixed dist_x = player_pos.x() - enemy.pos().x();
        bn::fixed dist_y = player_pos.y() - enemy.pos().y();
        bn::fixed dist_sq = dist_x * dist_x + dist_y * dist_y;
        const bn::fixed follow_dist_sq = 48 * 48; // 6 tiles squared

        if (!player_listening && dist_sq <= follow_dist_sq)
        {
            // Player is back in range, start chasing again
            bn::unique_ptr<ChaseState> chase_state = bn::make_unique<ChaseState>();
            enemy._state_machine.transition_to(enemy, bn::move(chase_state));
            return;
        }
    }

    void ReturnToPostState::exit(Enemy &enemy)
    {
        // No special cleanup needed
    }

    // StunnedState implementation
    void StunnedState::enter(Enemy &enemy)
    {
        // Stop movement while stunned
        enemy._target_dx = 0;
        enemy._target_dy = 0;
    }

    void StunnedState::update(Enemy &enemy, bn::fixed_point player_pos, const Level &level, bool player_listening)
    {
        // Keep enemy stationary while stunned
        enemy._target_dx = 0;
        enemy._target_dy = 0;

        // Check if stun duration is over
        if (enemy._state_machine.get_state_timer() >= _stun_duration)
        {
            // Decide what state to return to based on player proximity
            bn::fixed dist_x = player_pos.x() - enemy.pos().x();
            bn::fixed dist_y = player_pos.y() - enemy.pos().y();
            bn::fixed dist_sq = dist_x * dist_x + dist_y * dist_y;
            const bn::fixed follow_dist_sq = 48 * 48; // 6 tiles squared

            if (!player_listening && dist_sq <= follow_dist_sq)
            {
                // Player is close, start chasing
                bn::unique_ptr<ChaseState> chase_state = bn::make_unique<ChaseState>();
                enemy._state_machine.transition_to(enemy, bn::move(chase_state));
            }
            else
            {
                // Player is far, go idle
                bn::unique_ptr<IdleState> idle_state = bn::make_unique<IdleState>();
                enemy._state_machine.transition_to(enemy, bn::move(idle_state));
            }
        }
    }

    void StunnedState::exit(Enemy &enemy)
    {
        // Clear stunned flag
        enemy._stunned = false;
    }
}