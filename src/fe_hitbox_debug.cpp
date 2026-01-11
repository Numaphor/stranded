#include "fe_hitbox_debug.h"
#include "fe_player.h"
#include "fe_enemy.h"
#include "fe_constants.h"
#include "bn_sprite_items_minimap_player.h"
#include "bn_sprite_items_minimap_enemy.h"
#include "bn_sprite_builder.h"
#include "bn_sprite_palette_ptr.h"

namespace fe
{
    HitboxDebug::HitboxDebug() : _enabled(false)
    {
    }
    
    void HitboxDebug::clear()
    {
        _debug_sprites.clear();
    }
    
    void HitboxDebug::update(const Player* player, const bn::vector<Enemy, 16>& enemies, 
                            bn::camera_ptr camera, bn::fixed zoom_scale)
    {
        // Clear existing debug sprites
        clear();
        
        if (!_enabled)
        {
            return;
        }
        
        // Create debug visualization for player hitbox (red/player color)
        if (player)
        {
            Hitbox player_hitbox = player->get_hitbox();
            create_hitbox_outline(player_hitbox.x(), player_hitbox.y(), 
                                player_hitbox.width(), player_hitbox.height(),
                                camera, zoom_scale, 0); // 0 = player color (red/orange)
        }
        
        // Create debug visualization for enemy hitboxes (enemy color)
        for (const Enemy& enemy : enemies)
        {
            Hitbox enemy_hitbox = enemy.get_hitbox();
            create_hitbox_outline(enemy_hitbox.x(), enemy_hitbox.y(),
                                enemy_hitbox.width(), enemy_hitbox.height(),
                                camera, zoom_scale, 1); // 1 = enemy color (blue/green)
        }
    }
    
    void HitboxDebug::create_hitbox_outline(bn::fixed x, bn::fixed y, bn::fixed width, bn::fixed height,
                                           bn::camera_ptr camera, bn::fixed zoom_scale, int color_index)
    {
        // Calculate hitbox center in world coordinates
        bn::fixed_point hitbox_center(x + width / 2, y + height / 2);
        
        // Get camera position
        bn::fixed_point cam_pos(camera.x(), camera.y());
        
        // Calculate scaled position relative to camera
        bn::fixed_point offset = hitbox_center - cam_pos;
        bn::fixed_point scaled_center = cam_pos + bn::fixed_point(offset.x() * zoom_scale, offset.y() * zoom_scale);
        
        // Scale the hitbox dimensions
        bn::fixed scaled_width = width * zoom_scale;
        bn::fixed scaled_height = height * zoom_scale;
        
        // Create corner markers (4 small sprites at each corner)
        bn::fixed half_width = scaled_width / 2;
        bn::fixed half_height = scaled_height / 2;
        
        // Choose sprite based on color_index
        bn::sprite_item sprite_item = (color_index == 0) ? 
            bn::sprite_items::minimap_player : bn::sprite_items::minimap_enemy;
        
        // Create 4 corner sprites
        bn::fixed_point corners[4] = {
            bn::fixed_point(scaled_center.x() - half_width, scaled_center.y() - half_height), // Top-left
            bn::fixed_point(scaled_center.x() + half_width, scaled_center.y() - half_height), // Top-right
            bn::fixed_point(scaled_center.x() - half_width, scaled_center.y() + half_height), // Bottom-left
            bn::fixed_point(scaled_center.x() + half_width, scaled_center.y() + half_height)  // Bottom-right
        };
        
        for (int i = 0; i < 4; i++)
        {
            bn::sprite_ptr sprite = sprite_item.create_sprite(corners[i].x(), corners[i].y());
            sprite.set_camera(camera);
            sprite.set_z_order(-32000); // Very high priority to appear on top
            _debug_sprites.push_back(sprite);
        }
        
        // Create center marker for reference
        bn::sprite_ptr center_sprite = sprite_item.create_sprite(scaled_center.x(), scaled_center.y());
        center_sprite.set_camera(camera);
        center_sprite.set_z_order(-32000);
        _debug_sprites.push_back(center_sprite);
    }
}
