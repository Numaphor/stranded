#include "str_minimap.h"
#include "str_enemy.h"
#include "str_constants.h"

#include "bn_fixed_point.h"
#include "bn_camera_ptr.h"
#include "bn_affine_bg_map_ptr.h"
#include "bn_sprite_builder.h"
#include "bn_blending.h"

#include "bn_sprite_items_minimap_player.h"
#include "bn_sprite_items_minimap_enemy.h"

namespace str
{

    // =========================================================================
    // Minimap Implementation
    // =========================================================================

    Minimap::Minimap(bn::fixed_point pos, bn::affine_bg_map_ptr map, bn::camera_ptr &camera)
        : _player_dot(bn::sprite_items::minimap_player.create_sprite(pos.x(), pos.y() + MINIMAP_VERTICAL_OFFSET)),
          _position(bn::fixed_point(pos.x(), pos.y() + MINIMAP_VERTICAL_OFFSET))
    {

        _player_dot.set_bg_priority(0);
        _player_dot.set_z_order(Z_ORDER_MINIMAP_PLAYER);
        _player_dot.set_visible(true);
        (void)map;
        (void)camera;
    }

    void Minimap::update(bn::fixed_point p_pos, bn::fixed_point m_center, const bn::vector<Enemy, 16> &enemies)
    {
        bn::fixed rx = (p_pos.x() - m_center.x()) * MINIMAP_POSITION_SCALE, ry = (p_pos.y() - m_center.y()) * MINIMAP_POSITION_SCALE;
        _player_dot.set_position(_position.x() + rx, _position.y() + ry);
        for (int i = 0; i < enemies.size(); ++i)
        {
            bn::fixed_point ep = enemies[i].pos();
            if (i >= _enemy_dots.size())
            {
                auto s = bn::sprite_items::minimap_enemy.create_sprite(0, 0);
                s.set_bg_priority(0);
                s.set_z_order(Z_ORDER_MINIMAP_ENEMY);
                s.set_visible(1);
                s.set_blending_enabled(1);
                bn::blending::set_transparency_alpha(0.5);
                _enemy_dots.push_back(EnemyDot(std::move(s), &enemies[i]));
            }
            _enemy_dots[i].enemy = &enemies[i];
            _enemy_dots[i].sprite.set_position(_position.x() + (ep.x() - m_center.x()) * MINIMAP_POSITION_SCALE, _position.y() + (ep.y() - m_center.y()) * MINIMAP_POSITION_SCALE);
        }
        while (_enemy_dots.size() > enemies.size())
            _enemy_dots.pop_back();
    }

    void Minimap::set_visible(bool visible)
    {
        _player_dot.set_visible(visible);
        for (auto &ed : _enemy_dots)
            ed.sprite.set_visible(visible);
    }

} // namespace str
