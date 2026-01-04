#include "fe_minimap.h"
#include "fe_constants.h"
#include "bn_sprite_items_minimap_player.h"
#include "bn_sprite_items_minimap_enemy.h"
#include "fe_enemy.h"
#include "bn_blending.h"

namespace fe
{
    Minimap::Minimap(bn::fixed_point pos) : _player_dot(bn::sprite_items::minimap_player.create_sprite(pos.x(), pos.y() + MINIMAP_VERTICAL_OFFSET)), // Use player sprite
                                             _position(bn::fixed_point(pos.x(), pos.y() + MINIMAP_VERTICAL_OFFSET))                                   // Update position to be 16 pixels lower
    {
        // Configure player dot
        _player_dot.set_bg_priority(0);
        _player_dot.set_z_order(Z_ORDER_MINIMAP_PLAYER);
        _player_dot.set_visible(true);
    }

    void Minimap::update(bn::fixed_point player_pos, bn::fixed_point map_center, const bn::vector<Enemy, 16> &enemies)
    {
        // Calculate player position relative to map center with a smaller scale for slower movement
        // Reduced by 5x to make the minimap appear larger (dots move slower)

        // Update player dot
        bn::fixed rel_x = (player_pos.x() - map_center.x()) * MINIMAP_POSITION_SCALE;
        bn::fixed rel_y = (player_pos.y() - map_center.y()) * MINIMAP_POSITION_SCALE;
        _player_dot.set_position(_position.x() + rel_x, _position.y() + rel_y);

        // Update enemy dots
        for (int i = 0; i < enemies.size(); ++i)
        {
            const Enemy &enemy = enemies[i];

            // Get enemy position
            bn::fixed_point enemy_pos = enemy.pos();

            // Find or create enemy dot - optimized sprite reuse
            if (i >= _enemy_dots.size())
            {
                // Create new enemy dot with enemy sprite
                auto sprite = bn::sprite_items::minimap_enemy.create_sprite(0, 0);
                sprite.set_bg_priority(0);
                sprite.set_z_order(Z_ORDER_MINIMAP_ENEMY);
                sprite.set_visible(true);
                sprite.set_blending_enabled(true);

                // Set transparency for enemy dots
                bn::blending::set_transparency_alpha(0.5);

                // Create and add new enemy dot
                _enemy_dots.push_back(EnemyDot(std::move(sprite), &enemy));
            }
            
            // Update existing enemy dot (moved outside else to reduce code duplication)
            _enemy_dots[i].enemy = &enemy;

            // Update position of enemy dot
            bn::fixed enemy_rel_x = (enemy_pos.x() - map_center.x()) * MINIMAP_POSITION_SCALE;
            bn::fixed enemy_rel_y = (enemy_pos.y() - map_center.y()) * MINIMAP_POSITION_SCALE;
            _enemy_dots[i].sprite.set_position(
                _position.x() + enemy_rel_x,
                _position.y() + enemy_rel_y);
        }

        // Remove any extra enemy dots
        while (_enemy_dots.size() > enemies.size())
        {
            _enemy_dots.pop_back();
        }
    }

    void Minimap::set_visible(bool visible)
    {
        _player_dot.set_visible(visible);
        for (EnemyDot &enemy_dot : _enemy_dots)
        {
            enemy_dot.sprite.set_visible(visible);
        }
    }
}
