#ifndef STR_COLLECTIBLE_H
#define STR_COLLECTIBLE_H

#include "bn_fixed_point.h"
#include "bn_sprite_ptr.h"
#include "bn_camera_ptr.h"
#include "bn_optional.h"

#include "str_hitbox.h"

namespace str
{
    enum class CollectibleType
    {
        HEALTH
    };

    /**
     * Simple world collectible (e.g., health pickup).
     * - Uses Hitbox for collision detection.
     * - Uses bn::sprite_ptr for visual representation.
     * - Managed by World (owned in a bn::vector).
     */
    class Collectible
    {
    public:
        Collectible();
        Collectible(bn::fixed_point position, CollectibleType type, bn::camera_ptr camera);

        void update();

        [[nodiscard]] const Hitbox& hitbox() const
        {
            return _hitbox;
        }

        [[nodiscard]] CollectibleType type() const
        {
            return _type;
        }

        [[nodiscard]] bool is_collected() const
        {
            return _collected;
        }

        void collect();

    private:
        bn::fixed_point _position;
        Hitbox _hitbox;
        CollectibleType _type;
        bool _collected;
        bn::optional<bn::sprite_ptr> _sprite;
    };
}

#endif

