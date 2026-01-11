#ifndef FE_HITBOX_DEBUG_H
#define FE_HITBOX_DEBUG_H

#include "bn_sprite_ptr.h"
#include "bn_camera_ptr.h"
#include "bn_vector.h"
#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "fe_hitbox.h"

namespace fe
{
    // Forward declarations
    class Player;
    class Enemy;

    class HitboxDebug
    {
    public:
        HitboxDebug();
        
        void toggle() { _enabled = !_enabled; }
        void set_enabled(bool enabled) { _enabled = enabled; }
        [[nodiscard]] bool is_enabled() const { return _enabled; }
        
        // Update debug visualization for player and enemies
        void update(const Player* player, const bn::vector<Enemy, 16>& enemies, 
                   bn::camera_ptr camera, bn::fixed zoom_scale);
        
        // Clear all debug sprites
        void clear();
        
    private:
        bool _enabled;
        bn::vector<bn::sprite_ptr, 32> _debug_sprites;
        
        // Create a debug sprite for a hitbox
        void create_hitbox_sprite(const Hitbox& hitbox, bn::camera_ptr camera, 
                                 bn::fixed zoom_scale, int color_index);
        
        // Helper to create 4 corner sprites for a hitbox outline
        void create_hitbox_outline(bn::fixed x, bn::fixed y, bn::fixed width, bn::fixed height,
                                   bn::camera_ptr camera, bn::fixed zoom_scale, int color_index);
    };
}

#endif
