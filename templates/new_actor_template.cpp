/**
 * ACTOR_PATTERN: New Entity Actor Implementation Template
 * - Follows the structure defined in new_actor_template.h
 * - Implements all required Entity interface methods
 * - Uses state machine for behavior management
 * - Handles animation, movement, and collision
 */

#include "new_actor_template.h"
#include "bn_sprite_items_new_actor.h"  // Replace with actual sprite item
#include "bn_math.h"

namespace str
{
    NewActor::NewActor(bn::fixed_point initial_position, int actor_type)
        : _current_state(State::IDLE)
        , _actor_type(actor_type)
        , _is_active(true)
        , _position(initial_position)
        , _velocity(0, 0)
        , _state_timer(0)
        , _animation_timer(0)
    {
        // Initialize sprite
        _sprite = bn::sprite_items::new_actor.create_sprite(_position.x(), _position.y());
        
        // Start idle animation
        static const int idle_frames[] = {0, 1, 2, 3};
        _start_animation(bn::sprite_items::new_actor, idle_frames, 4);
    }
    
    void NewActor::update()
    {
        if (!_is_active) return;
        
        // Update state machine
        switch (_current_state)
        {
            case State::IDLE:
                _update_idle_state();
                break;
            case State::ACTIVE:
                _update_active_state();
                break;
            case State::TRANSITION:
                _update_transition_state();
                break;
            case State::DISABLED:
                _update_disabled_state();
                break;
        }
        
        // Update animation
        _update_animation();
        
        // Update position
        _update_position();
        
        // Update sprite position
        _sprite.set_position(_position);
        
        // Check boundaries
        _check_world_boundaries();
        
        // Update timers
        if (_state_timer > 0) _state_timer--;
        if (_animation_timer > 0) _animation_timer--;
    }
    
    Hitbox NewActor::get_hitbox() const
    {
        // Return hitbox based on current position and actor size
        return Hitbox::create_entity_hitbox(_position, 16, 16); // 16x16 size
    }
    
    void NewActor::handle_collision(Entity* other)
    {
        if (!_is_active) return;
        
        // Handle collision based on entity type
        // Example: take damage from bullets, push away from player, etc.
        
        // Trigger state change if needed
        if (/* collision condition */ false)
        {
            set_state(State::TRANSITION);
        }
    }
    
    bool NewActor::is_active() const
    {
        return _is_active;
    }
    
    void NewActor::set_state(State new_state)
    {
        if (_current_state == new_state) return;
        
        // Handle state exit logic
        switch (_current_state)
        {
            case State::IDLE:
                // Cleanup idle state
                break;
            case State::ACTIVE:
                // Cleanup active state
                break;
            case State::TRANSITION:
                // Cleanup transition state
                break;
            case State::DISABLED:
                // Cleanup disabled state
                break;
        }
        
        _current_state = new_state;
        _state_timer = 0; // Reset timer for new state
        
        // Handle state entry logic
        switch (new_state)
        {
            case State::IDLE:
                // Start idle animation
                {
                    static const int idle_frames[] = {0, 1, 2, 3};
                    _start_animation(bn::sprite_items::new_actor, idle_frames, 4);
                }
                break;
            case State::ACTIVE:
                // Start active animation
                {
                    static const int active_frames[] = {4, 5, 6, 7};
                    _start_animation(bn::sprite_items::new_actor, active_frames, 4);
                }
                break;
            case State::TRANSITION:
                // Start transition animation
                {
                    static const int transition_frames[] = {8, 9, 10};
                    _start_animation(bn::sprite_items::new_actor, transition_frames, 3);
                }
                _state_timer = 30; // 0.5 seconds at 60 FPS
                break;
            case State::DISABLED:
                // Start disabled animation or hide sprite
                _sprite.set_visible(false);
                _is_active = false;
                break;
        }
    }
    
    NewActor::State NewActor::get_state() const
    {
        return _current_state;
    }
    
    // Private state update methods
    void NewActor::_update_idle_state()
    {
        // Idle behavior: wait for trigger, play idle animation
        // Example: check for player proximity, timer-based activation, etc.
        
        if (/* activation condition */ false)
        {
            set_state(State::ACTIVE);
        }
    }
    
    void NewActor::_update_active_state()
    {
        // Active behavior: main action, movement, interaction
        // Example: move towards target, attack pattern, special ability
        
        if (/* deactivation condition */ false)
        {
            set_state(State::IDLE);
        }
        else if (/* disable condition */ false)
        {
            set_state(State::DISABLED);
        }
    }
    
    void NewActor::_update_transition_state()
    {
        // Transition behavior: between states, temporary effects
        // Example: play transition animation, wait for timer
        
        if (_state_timer <= 0)
        {
            set_state(State::IDLE); // Default next state
        }
    }
    
    void NewActor::_update_disabled_state()
    {
        // Disabled behavior: no action, possibly hidden
        // Usually just wait for cleanup or respawn
    }
    
    // Animation helpers
    void NewActor::_start_animation(const bn::sprite_item& sprite_item, const int* frames, int frame_count)
    {
        _animation_action = bn::sprite_animate_action<16>::create(
            _sprite, 16, sprite_item, frames, frame_count
        );
        _animation_timer = frame_count * 16; // Total animation duration
    }
    
    void NewActor::_update_animation()
    {
        if (_animation_action && _animation_action->update())
        {
            // Animation completed
            if (_current_state == State::TRANSITION && _state_timer <= 0)
            {
                set_state(State::IDLE);
            }
        }
    }
    
    // Movement helpers
    void NewActor::_update_position()
    {
        _position += _velocity;
    }
    
    void NewActor::_apply_velocity()
    {
        // Apply acceleration, friction, or other movement logic
        // Example: _velocity *= 0.95; // Friction
    }
    
    // Collision helpers
    void NewActor::_check_world_boundaries()
    {
        // Keep actor within world bounds
        bn::fixed world_left = -MAP_OFFSET_X;
        bn::fixed world_right = (MAP_COLUMNS * TILE_SIZE) - MAP_OFFSET_X;
        bn::fixed world_top = -MAP_OFFSET_Y;
        bn::fixed world_bottom = (MAP_ROWS * TILE_SIZE) - MAP_OFFSET_Y;
        
        if (_position.x() < world_left) _position.set_x(world_left);
        if (_position.x() > world_right) _position.set_x(world_right);
        if (_position.y() < world_top) _position.set_y(world_top);
        if (_position.y() > world_bottom) _position.set_y(world_bottom);
    }
    
    void NewActor::_handle_world_collision()
    {
        // Handle collision with world geometry
        // Example: walls, obstacles, special zones
    }
}
