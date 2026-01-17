#include "str_bullet_manager.h"
#include "str_enemy.h"
#include "str_hitbox.h"
#include "str_constants.h"

#include "bn_fixed_point.h"
#include "bn_camera_ptr.h"
#include "bn_sprite_builder.h"

#include "bn_sprite_items_hero.h"

namespace str
{

    // =========================================================================
    // Bullet Implementation
    // =========================================================================

    Bullet::Bullet(bn::fixed_point p, bn::fixed_point v, bn::camera_ptr c, Direction d) : _pos(p), _velocity(v), _active(1), _hitbox(p.x(), p.y(), 2, 2), _lifetime(BULLET_LIFETIME)
    {
        _sprite = bn::sprite_items::hero.create_sprite(p.x(), p.y(), 0);
        _sprite->set_camera(c);
        _sprite->set_z_order(Z_ORDER_BULLET);
        _sprite->set_scale(BULLET_SCALE);
        _sprite->set_bg_priority(0);
        _sprite->set_rotation_angle(d == Direction::UP ? 0 : (d == Direction::RIGHT ? 270 : (d == Direction::DOWN ? 180 : 90)));
    }

    void Bullet::update()
    {
        if (!_active)
            return;
        _pos += _velocity;
        if (_sprite)
            _sprite->set_position(_pos);
        _hitbox.set_x(_pos.x());
        _hitbox.set_y(_pos.y());
        if (--_lifetime <= 0)
            deactivate();
    }
    bool Bullet::check_enemy_collision(Enemy &e) { return _active && _hitbox.collides_with(e.get_hitbox()); }
    BulletManager::BulletManager() {}
    void BulletManager::fire_bullet(bn::fixed_point p, Direction d)
    {
        if (_shoot_cooldown > 0 || !_camera)
            return;
        _bullets.push_back(Bullet(p, calculate_bullet_velocity(d), *_camera, d));
        _shoot_cooldown = SHOOT_COOLDOWN_TIME;
    }
    void BulletManager::update_bullets()
    {
        if (_shoot_cooldown > 0)
            _shoot_cooldown--;
        for (int i = 0; i < _bullets.size();)
        {
            _bullets[i].update();
            if (!_bullets[i].is_active())
                _bullets.erase(_bullets.begin() + i);
            else
                i++;
        }
    }
    void BulletManager::clear_bullets()
    {
        _bullets.clear();
        _shoot_cooldown = 0;
    }
    void BulletManager::set_camera(bn::camera_ptr camera) { _camera = camera; }
    bn::fixed_point BulletManager::calculate_bullet_velocity(Direction d) const { return d == Direction::UP ? bn::fixed_point(0, -BULLET_SPEED) : (d == Direction::DOWN ? bn::fixed_point(0, BULLET_SPEED) : (d == Direction::LEFT ? bn::fixed_point(-BULLET_SPEED, 0) : bn::fixed_point(BULLET_SPEED, 0))); }

} // namespace str
