#ifndef STR_ENTITY_H
#define STR_ENTITY_H

#include "bn_fixed_point.h"
#include "bn_sprite_ptr.h"
#include "bn_camera_ptr.h"
#include "bn_optional.h"
#include "str_hitbox.h"

namespace str
{
    class Entity
    {
    public:
        Entity();
        Entity(bn::fixed_point pos);
        Entity(bn::sprite_ptr sprite);
        virtual ~Entity() = default;

        virtual void update() = 0;

        [[nodiscard]] virtual bn::fixed_point pos() const;
        [[nodiscard]] virtual bn::fixed_point previous_pos() const;
        [[nodiscard]] virtual Hitbox get_hitbox() const;
        [[nodiscard]] virtual bool has_sprite() const;

        virtual void set_position(bn::fixed_point new_pos);
        virtual void revert_position();
        virtual void set_sprite_z_order(int z_order);
        virtual void set_visible(bool visible);
        virtual void set_camera(bn::camera_ptr camera);
        bn::sprite_ptr* get_sprite() { return _sprite.has_value() ? &_sprite.value() : nullptr; }

    protected:
        bn::fixed_point _pos;
        bn::fixed_point _previous_pos;
        bn::optional<bn::sprite_ptr> _sprite;
        Hitbox _hitbox;

        virtual void update_hitbox();
        virtual void update_sprite_position();
    };
}

#endif
