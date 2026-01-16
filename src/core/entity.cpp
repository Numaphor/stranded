#include "str_entity.h"
#include "str_hitbox.h"

#include "bn_fixed_point.h"
#include "bn_sprite_ptr.h"
#include "bn_camera_ptr.h"

namespace str
{

    // =========================================================================
    // Entity Implementation
    // =========================================================================

    Entity::Entity() : _pos(0, 0), _previous_pos(0, 0), _hitbox(0, 0, DEFAULT_ENTITY_WIDTH, DEFAULT_ENTITY_HEIGHT) {}
    Entity::Entity(bn::fixed_point p) : _pos(p), _previous_pos(p), _hitbox(p.x(), p.y(), DEFAULT_ENTITY_WIDTH, DEFAULT_ENTITY_HEIGHT) {}
    Entity::Entity(bn::sprite_ptr s) : _pos(s.x(), s.y()), _previous_pos(_pos), _sprite(s), _hitbox(_pos.x(), _pos.y(), DEFAULT_ENTITY_WIDTH, DEFAULT_ENTITY_HEIGHT) {}
    bn::fixed_point Entity::pos() const { return _pos; }
    bn::fixed_point Entity::previous_pos() const { return _previous_pos; }
    Hitbox Entity::get_hitbox() const { return _hitbox; }
    bool Entity::has_sprite() const { return _sprite.has_value(); }
    void Entity::set_position(bn::fixed_point n)
    {
        _previous_pos = _pos;
        _pos = n;
        update_hitbox();
        update_sprite_position();
    }
    void Entity::revert_position()
    {
        _pos = _previous_pos;
        update_hitbox();
        update_sprite_position();
    }
    void Entity::set_sprite_z_order(int z)
    {
        if (_sprite)
            _sprite->set_z_order(z);
    }
    void Entity::set_visible(bool v)
    {
        if (_sprite)
            _sprite->set_visible(v);
    }
    void Entity::set_camera(bn::camera_ptr c)
    {
        if (_sprite)
            _sprite->set_camera(c);
    }
    void Entity::update_hitbox()
    {
        _hitbox.set_x(_pos.x());
        _hitbox.set_y(_pos.y());
    }
    void Entity::update_sprite_position()
    {
        if (_sprite)
            _sprite->set_position(_pos);
    }

} // namespace str
