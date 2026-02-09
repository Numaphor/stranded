#include "str_collectible.h"
#include "str_constants.h"

#include "bn_sprite_items_heart_normal_full.h"

namespace str
{

    Collectible::Collectible() :
        _position(0, 0),
        _hitbox(0, 0, 0, 0),
        _type(CollectibleType::HEALTH),
        _collected(true),
        _sprite()
    {
    }

    Collectible::Collectible(bn::fixed_point position, CollectibleType type, bn::camera_ptr camera) :
        _position(position),
        _hitbox(Hitbox::calculate_centered_position(position, bn::fixed(16), bn::fixed(16)).x(),
                Hitbox::calculate_centered_position(position, bn::fixed(16), bn::fixed(16)).y(),
                bn::fixed(16),
                bn::fixed(16)),
        _type(type),
        _collected(false),
        _sprite()
    {
        if(_type == CollectibleType::HEALTH)
        {
            bn::sprite_ptr sprite = bn::sprite_items::heart_normal_full.create_sprite(_position.x(), _position.y(), 0);
            sprite.set_bg_priority(1);
            sprite.set_z_order(-_position.y().integer());
            sprite.set_camera(camera);
            _sprite = bn::move(sprite);
        }
    }

    void Collectible::update()
    {
        if(_collected)
        {
            return;
        }

        // Keep sprite and hitbox aligned to position.
        if(_sprite)
        {
            _sprite->set_position(_position);
            _sprite->set_z_order(-_position.y().integer());
        }

        // Hitbox stays centered on _position.
        bn::fixed_point hit_pos = Hitbox::calculate_centered_position(_position, _hitbox.width(), _hitbox.height());
        _hitbox.set_x(hit_pos.x());
        _hitbox.set_y(hit_pos.y());
    }

    void Collectible::collect()
    {
        if(_collected)
        {
            return;
        }

        _collected = true;

        if(_sprite)
        {
            _sprite->set_visible(false);
        }
    }

}

