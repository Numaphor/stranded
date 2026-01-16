#ifndef STR_MOVEMENT_H
#define STR_MOVEMENT_H

#include "bn_fixed.h"
#include "bn_fixed_point.h"

namespace str
{
    // Generic movement interface that can be used by Player, Enemy, and other entities
    class Movement
    {
    public:
        enum class State
        {
            IDLE,
            WALKING,
            HIT,
            DEAD
        };

        enum class Direction
        {
            UP,
            DOWN,
            LEFT,
            RIGHT
        };

        // Movement constants - can be overridden by derived classes
        static constexpr bn::fixed DEFAULT_ACC_CONST = 0.35;
        static constexpr bn::fixed DEFAULT_FRICTION_CONST = 0.65;
        static constexpr bn::fixed DEFAULT_MOVEMENT_THRESHOLD = 0.1;
        static constexpr bn::fixed DEFAULT_MAX_SPEED = 2;

        Movement();
        virtual ~Movement() = default;

        // Core movement methods
        virtual void move_right();
        virtual void move_left();
        virtual void move_up();
        virtual void move_down();
        virtual void apply_friction();
        virtual void reset();
        virtual void stop_movement();

        // Getters
        [[nodiscard]] virtual bn::fixed dx() const { return _dx; }
        [[nodiscard]] virtual bn::fixed dy() const { return _dy; }
        [[nodiscard]] virtual State current_state() const { return _current_state; }
        [[nodiscard]] virtual Direction facing_direction() const { return _facing_direction; }
        [[nodiscard]] virtual bool is_state(State state) const { return _current_state == state; }
        [[nodiscard]] virtual bool is_moving() const { return _dx != 0 || _dy != 0; }

        // Setters
        virtual void set_dx(bn::fixed dx) { _dx = dx; }
        virtual void set_dy(bn::fixed dy) { _dy = dy; }

        // Update movement state based on current velocity
        virtual void update_movement_state() { update_state(); }

        // Allow derived classes to customize movement constants
        virtual bn::fixed get_acc_const() const { return DEFAULT_ACC_CONST; }
        virtual bn::fixed get_friction_const() const { return DEFAULT_FRICTION_CONST; }
        virtual bn::fixed get_movement_threshold() const { return DEFAULT_MOVEMENT_THRESHOLD; }
        virtual bn::fixed get_max_speed() const { return DEFAULT_MAX_SPEED; }
        
        // Velocity management for Enemy integration
        virtual void set_velocity(bn::fixed_point velocity) { _dx = velocity.x(); _dy = velocity.y(); }
        virtual bn::fixed_point get_velocity() const { return bn::fixed_point(_dx, _dy); }
        virtual void update() { update_movement_state(); }

    protected:
        bn::fixed _dx;
        bn::fixed _dy;
        State _current_state;
        Direction _facing_direction;

        virtual void update_state();
        virtual void clamp_velocity();
    };


    // EnemyMovement class that can be used by Enemy entities
    class EnemyMovement : public Movement
    {
    public:
        // Enemy-specific movement constants (can be different from player)
        static constexpr bn::fixed acc_const = 0.25;
        static constexpr bn::fixed friction_const = 0.7;
        static constexpr bn::fixed movement_threshold = 0.05;
        static constexpr bn::fixed max_speed = 1.5;

        EnemyMovement();

        // Override movement constants for enemy-specific behavior
        bn::fixed get_acc_const() const override { return acc_const; }
        bn::fixed get_friction_const() const override { return friction_const; }
        bn::fixed get_movement_threshold() const override { return movement_threshold; }
        bn::fixed get_max_speed() const override { return max_speed; }
    };
}

#endif
