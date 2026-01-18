/**
 * ACTOR_PATTERN: New Entity Actor Template
 * - MUST inherit from Entity base class
 * - MUST use State Machine pattern for behavior
 * - MUST use bn::fixed_point for positions
 * - MUST use Hitbox class for collision detection
 * - NO direct input handling (use input processors)
 * - ALL state changes through state machine
 * - MANAGES: [Specific responsibilities for this actor]
 */

#ifndef STR_NEW_ACTOR_H
#define STR_NEW_ACTOR_H

#include "bn_sprite_ptr.h"
#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "bn_sprite_animate_actions.h"

#include "str_entity.h"
#include "str_hitbox.h"
#include "str_constants.h"

namespace str
{
    class NewActor : public Entity
    {
    public:
        /**
         * Constructor for NewActor
         * @param initial_position Starting position for the actor
         * @param actor_type Type identifier for this actor
         */
        NewActor(bn::fixed_point initial_position, int actor_type);
        
        /**
         * Main update method called each frame
         * Handles state machine updates, animation, and position
         */
        void update() override;
        
        /**
         * Get current hitbox for collision detection
         * @return Hitbox representing actor's collision area
         */
        Hitbox get_hitbox() const override;
        
        /**
         * Handle collision with other entities
         * @param other Entity that collided with this actor
         */
        void handle_collision(Entity* other) override;
        
        /**
         * Check if actor is active/alive
         * @return true if actor is active, false if destroyed/inactive
         */
        bool is_active() const override;

    protected:
        // State machine implementation
        enum class State
        {
            IDLE,           // Default resting state
            ACTIVE,         // Main action state
            TRANSITION,     // Between states
            DISABLED        // Inactive/dead state
        };
        
        /**
         * Set current state and handle state transitions
         * @param new_state New state to transition to
         */
        void set_state(State new_state);
        
        /**
         * Get current state
         * @return Current actor state
         */
        State get_state() const;

    private:
        // Actor properties
        State _current_state;
        int _actor_type;
        bool _is_active;
        
        // Animation and sprites
        bn::sprite_ptr _sprite;
        bn::optional<bn::sprite_animate_action<16>> _animation_action;
        
        // Position and movement
        bn::fixed_point _position;
        bn::fixed_point _velocity;
        
        // State-specific timers and counters
        int _state_timer;
        int _animation_timer;
        
        // Private state update methods
        void _update_idle_state();
        void _update_active_state();
        void _update_transition_state();
        void _update_disabled_state();
        
        // Animation helpers
        void _start_animation(const bn::sprite_item& sprite_item, const int* frames, int frame_count);
        void _update_animation();
        
        // Movement helpers
        void _update_position();
        void _apply_velocity();
        
        // Collision helpers
        void _check_world_boundaries();
        void _handle_world_collision();
    };
}

#endif // STR_NEW_ACTOR_H
