#pragma once

#include "bn_camera_ptr.h"
#include "bn_fixed_point.h"
#include "bn_affine_bg_map_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_tiles_ptr.h"
#include "bn_sprite_palette_ptr.h"
#include "bn_vector.h"
#include "bn_fixed.h"
#include "bn_optional.h"
#include "bn_blending.h"

namespace str {
    class Enemy;  // Forward declaration

    class Minimap {
    private:
        bn::sprite_ptr _player_dot;
        bn::fixed_point _position;
        bn::fixed _scale;
        
        struct EnemyDot {
            bn::sprite_ptr sprite;
            const Enemy* enemy;
            
            EnemyDot() = default;
            EnemyDot(const bn::sprite_ptr& s, const Enemy* e) : sprite(s), enemy(e) {}
            
            // Add move constructor
            EnemyDot(EnemyDot&& other) noexcept = default;
            EnemyDot& operator=(EnemyDot&& other) noexcept = default;
            
            // Delete copy constructor and assignment
            EnemyDot(const EnemyDot&) = delete;
            EnemyDot& operator=(const EnemyDot&) = delete;
        };
        
        bn::vector<EnemyDot, 16> _enemy_dots;
        
    public:
        Minimap(bn::fixed_point pos, bn::affine_bg_map_ptr map, bn::camera_ptr& camera);
        void update(bn::fixed_point player_pos, bn::fixed_point map_center, const bn::vector<Enemy, 16>& enemies);
        void set_visible(bool visible);
    };
}
