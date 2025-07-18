#include "fe_bullet_manager.h"
#include "fe_enemy.h"
#include "bn_sprite_items_hero.h"
#include "bn_log.h"

namespace fe
{
    Bullet::Bullet(bn::fixed_point pos, bn::fixed_point velocity, bn::camera_ptr camera, Direction direction)
        : _pos(pos), _velocity(velocity), _active(true), _hitbox(pos.x(), pos.y(), 2, 2), _lifetime(BULLET_LIFETIME)
    {
        _sprite = bn::sprite_items::hero.create_sprite(_pos.x(), _pos.y(), 0);
        _sprite->set_camera(camera);
        _sprite->set_z_order(-15); // Match existing bullet z-order
        _sprite->set_scale(0.15, 0.15); // Make bullet small like in player implementation
        _sprite->set_bg_priority(0);
        
        switch (direction)
        {
            case Direction::UP:
                _sprite->set_rotation_angle(0);
                break;
            case Direction::RIGHT:
                _sprite->set_rotation_angle(270);
                break;
            case Direction::DOWN:
                _sprite->set_rotation_angle(180);
                break;
            case Direction::LEFT:
                _sprite->set_rotation_angle(90);
                break;
            default:
                _sprite->set_rotation_angle(270); // Default to right
                break;
        }
    }

    void Bullet::update()
    {
        if (!_active) return;

        // Update position
        _pos += _velocity;

        if (_sprite)
        {
            _sprite->set_position(_pos);
        }

        _hitbox.set_x(_pos.x());
        _hitbox.set_y(_pos.y());

        _lifetime--;
        if (_lifetime <= 0)
        {
            deactivate();
        }
    }

    bool Bullet::check_enemy_collision(Enemy &enemy)
    {
        if (!_active) return false;
        
        Hitbox enemy_hitbox = enemy.get_hitbox();
        return _hitbox.collides_with(enemy_hitbox);
    }

    BulletManager::BulletManager()
    {
    }

    void BulletManager::fire_bullet(bn::fixed_point pos, Direction direction)
    {
        if (_shoot_cooldown > 0 || !_camera) return;

        bn::fixed_point velocity = calculate_bullet_velocity(direction);

        _bullets.push_back(Bullet(pos, velocity, *_camera, direction));

        _shoot_cooldown = SHOOT_COOLDOWN_TIME;
    }

    void BulletManager::update_bullets()
    {
        if (_shoot_cooldown > 0)
        {
            _shoot_cooldown--;
        }

        for (int i = 0; i < _bullets.size();)
        {
            _bullets[i].update();
            
            if (!_bullets[i].is_active())
            {
                _bullets.erase(_bullets.begin() + i);
            }
            else
            {
                i++;
            }
        }
    }

    void BulletManager::clear_bullets()
    {
        _bullets.clear();
        _shoot_cooldown = 0;
    }

    void BulletManager::set_camera(bn::camera_ptr camera)
    {
        _camera = camera;
    }

    bn::fixed_point BulletManager::calculate_bullet_velocity(Direction direction) const
    {
        constexpr bn::fixed bullet_speed = 4;
        
        switch (direction)
        {
            case Direction::UP:
                return bn::fixed_point(0, -bullet_speed);
            case Direction::DOWN:
                return bn::fixed_point(0, bullet_speed);
            case Direction::LEFT:
                return bn::fixed_point(-bullet_speed, 0);
            case Direction::RIGHT:
                return bn::fixed_point(bullet_speed, 0);
            default:
                return bn::fixed_point(0, -bullet_speed); // Default to up
        }
    }
}
