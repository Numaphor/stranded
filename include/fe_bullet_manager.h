#ifndef FE_BULLET_MANAGER_H
#define FE_BULLET_MANAGER_H

#include "bn_fixed_point.h"
#include "bn_sprite_ptr.h"
#include "bn_camera_ptr.h"
#include "bn_optional.h"
#include "bn_vector.h"
#include "fe_hitbox.h"
#include "fe_constants.h"

namespace fe
{
    // Forward declaration of Enemy class
    class Enemy;

    // Movement direction enum (copied from PlayerMovement for bullet direction)
    enum class Direction
    {
        UP,
        DOWN,
        LEFT,
        RIGHT
    };

    // Bullet class for projectiles
    class Bullet
    {
    public:
        Bullet(bn::fixed_point pos, bn::fixed_point velocity, bn::camera_ptr camera, Direction direction);

        void update();
        bool check_enemy_collision(Enemy &enemy);

        [[nodiscard]] bool is_active() const { return _active; }
        [[nodiscard]] bn::fixed_point position() const { return _pos; }
        [[nodiscard]] Hitbox get_hitbox() const { return _hitbox; }
        void deactivate()
        {
            _active = false;
            if (_sprite.has_value())
            {
                _sprite.reset();
            }
        }

        // Access to sprite for zoom scaling
        [[nodiscard]] bn::sprite_ptr* get_sprite() { return _sprite.has_value() ? &_sprite.value() : nullptr; }

    private:
        bn::fixed_point _pos;
        bn::fixed_point _velocity;
        bn::optional<bn::sprite_ptr> _sprite;
        bool _active;
        Hitbox _hitbox;

        int _lifetime;
    };

    // BulletManager class to handle bullet creation, updating, and management
    class BulletManager
    {
    public:
        BulletManager();

        void fire_bullet(bn::fixed_point pos, Direction direction);
        void update_bullets();
        void clear_bullets();
        void set_camera(bn::camera_ptr camera);
        [[nodiscard]] const bn::vector<Bullet, 32> &bullets() const { return _bullets; }

        [[nodiscard]] const bn::vector<Bullet, 32> &get_bullets() const { return _bullets; }
        [[nodiscard]] bool can_fire() const { return _shoot_cooldown <= 0; }

    private:
        bn::vector<Bullet, 32> _bullets;               // Store active bullets, limit to 32 max
        int _shoot_cooldown = 0;                       // Cooldown between shots
        bn::optional<bn::camera_ptr> _camera;          // Store camera reference for bullets

        bn::fixed_point calculate_bullet_velocity(Direction direction) const;
    };
}

#endif
