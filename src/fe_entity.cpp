#include "fe_entity.h"
#include "fe_constants.h"

namespace fe
{
    Entity::Entity() : _pos(0, 0), _previous_pos(0, 0), _hitbox(0, 0, DEFAULT_ENTITY_WIDTH, DEFAULT_ENTITY_HEIGHT)
    {
    }

    Entity::Entity(bn::fixed_point pos) : _pos(pos), _previous_pos(pos), _hitbox(pos.x(), pos.y(), DEFAULT_ENTITY_WIDTH, DEFAULT_ENTITY_HEIGHT)
    {
    }

    Entity::Entity(bn::sprite_ptr sprite) : _pos(sprite.x(), sprite.y()), _previous_pos(_pos), _sprite(sprite), _hitbox(_pos.x(), _pos.y(), DEFAULT_ENTITY_WIDTH, DEFAULT_ENTITY_HEIGHT)
    {
    }

    bn::fixed_point Entity::pos() const
    {
        return _pos;
    }

    bn::fixed_point Entity::previous_pos() const
    {
        return _previous_pos;
    }

    Hitbox Entity::get_hitbox() const
    {
        return _hitbox;
    }

    bool Entity::has_sprite() const
    {
        return _sprite.has_value();
    }

    void Entity::set_position(bn::fixed_point new_pos)
    {
        _previous_pos = _pos;
        _pos = new_pos;
        update_hitbox();
        update_sprite_position();
    }

    void Entity::revert_position()
    {
        _pos = _previous_pos;
        update_hitbox();
        update_sprite_position();
    }

    void Entity::set_sprite_z_order(int z_order)
    {
        if (_sprite)
        {
            _sprite->set_z_order(z_order);
        }
    }

    void Entity::set_visible(bool visible)
    {
        if (_sprite)
        {
            _sprite->set_visible(visible);
        }
    }

    void Entity::set_camera(bn::camera_ptr camera)
    {
        if (_sprite)
        {
            _sprite->set_camera(camera);
        }
    }

    void Entity::update_hitbox()
    {
        _hitbox.set_x(_pos.x());
        _hitbox.set_y(_pos.y());
    }

    void Entity::update_sprite_position()
    {
        if (_sprite)
        {
            _sprite->set_position(_pos);
        }
    }
}
