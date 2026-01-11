#include "fe_hitbox_debug.h"
#include "fe_constants.h"
#include "bn_regular_bg_map_cell.h"
#include "bn_log.h"

namespace fe
{
    HitboxDebug::HitboxDebug(bn::regular_bg_map_ptr &bg_map, bn::regular_bg_map_cell *cells, int background_tile)
        : _bg_map(bg_map), _cells(cells), _background_tile(background_tile), _active(false)
    {
        _modified_cells.clear();
    }

    HitboxDebug::~HitboxDebug()
    {
        // Clear debug visualization when destroyed
        if (_active)
        {
            _clear_debug_tiles();
        }
    }

    void HitboxDebug::toggle()
    {
        _active = !_active;
        
        if (!_active)
        {
            // Clear all debug tiles when turning off
            _clear_debug_tiles();
        }
    }

    void HitboxDebug::update(
        const Player *player,
        const bn::vector<Enemy, 16> &enemies,
        const NPC *merchant,
        const bn::camera_ptr &camera)
    {
        if (!_active)
        {
            return;
        }

        // Clear previous frame's debug tiles
        _clear_debug_tiles();

        // Draw player hitbox (tile 5 for player - white/bright color)
        if (player)
        {
            Hitbox player_hitbox = player->get_hitbox();
            _draw_hitbox(player_hitbox, 5);
        }

        // Draw enemy hitboxes (tile 6 for enemies - red/danger color)
        for (const Enemy &enemy : enemies)
        {
            Hitbox enemy_hitbox = enemy.get_hitbox();
            _draw_hitbox(enemy_hitbox, 6);
        }

        // Draw merchant zones if merchant exists
        if (merchant)
        {
            bn::fixed_point merchant_pos = merchant->pos();
            
            // Draw interaction zone (tile 4 - interaction)
            Hitbox interaction_zone = Hitbox::create_merchant_interaction_zone(merchant_pos);
            _draw_hitbox(interaction_zone, 4);
            
            // Draw collision zone (tile 3 - collision)
            // Merchant collision zone is 24x24 centered on merchant
            constexpr bn::fixed collision_half_size = 12;
            Hitbox collision_zone(
                merchant_pos.x() - collision_half_size,
                merchant_pos.y() - collision_half_size,
                collision_half_size * 2,
                collision_half_size * 2
            );
            _draw_hitbox(collision_zone, 3);
        }

        // Draw sword zone (tile 7 - special zone)
        Hitbox sword_zone = Hitbox::create_sword_zone();
        _draw_hitbox(sword_zone, 7);

        // Reload the background map to show changes
        _bg_map.reload_cells_ref();
    }

    void HitboxDebug::_draw_hitbox(const Hitbox &hitbox, int tile_id)
    {
        // Get hitbox bounds in world coordinates
        bn::fixed left = hitbox.x();
        bn::fixed top = hitbox.y();
        bn::fixed right = hitbox.x() + hitbox.width();
        bn::fixed bottom = hitbox.y() + hitbox.height();

        // Convert to tile coordinates
        int tile_left, tile_top, tile_right, tile_bottom;
        _world_to_tile(bn::fixed_point(left, top), tile_left, tile_top);
        _world_to_tile(bn::fixed_point(right, bottom), tile_right, tile_bottom);

        // Draw the outline of the hitbox
        for (int x = tile_left; x <= tile_right; x++)
        {
            for (int y = tile_top; y <= tile_bottom; y++)
            {
                // Only draw the border (not filled)
                bool is_border = (x == tile_left || x == tile_right || y == tile_top || y == tile_bottom);
                
                if (is_border)
                {
                    // Check if tile is within map bounds
                    if (x >= 0 && x < MAP_COLUMNS && y >= 0 && y < MAP_ROWS)
                    {
                        int cell_index = x + y * MAP_COLUMNS;
                        _cells[cell_index] = bn::regular_bg_map_cell(tile_id);
                        
                        // Track this cell for efficient clearing
                        if (_modified_cells.size() < _modified_cells.max_size())
                        {
                            _modified_cells.push_back(cell_index);
                        }
                    }
                }
            }
        }
    }

    void HitboxDebug::_draw_point(const bn::fixed_point &point, int tile_id)
    {
        int tile_x, tile_y;
        _world_to_tile(point, tile_x, tile_y);

        if (tile_x >= 0 && tile_x < MAP_COLUMNS && tile_y >= 0 && tile_y < MAP_ROWS)
        {
            int cell_index = tile_x + tile_y * MAP_COLUMNS;
            _cells[cell_index] = bn::regular_bg_map_cell(tile_id);
            
            // Track this cell for efficient clearing
            if (_modified_cells.size() < _modified_cells.max_size())
            {
                _modified_cells.push_back(cell_index);
            }
        }
    }

    void HitboxDebug::_clear_debug_tiles()
    {
        // Restore only the modified tiles to background tile (efficient clearing)
        for (int cell_index : _modified_cells)
        {
            if (cell_index >= 0 && cell_index < MAP_CELLS_COUNT)
            {
                _cells[cell_index] = bn::regular_bg_map_cell(_background_tile);
            }
        }
        
        // Clear the list of modified cells for next frame
        _modified_cells.clear();
        
        // Reload to apply changes
        _bg_map.reload_cells_ref();
    }

    void HitboxDebug::_world_to_tile(const bn::fixed_point &world_pos, int &tile_x, int &tile_y)
    {
        // Convert world coordinates to tile coordinates
        // World coordinates are centered around (0, 0)
        // Tile coordinates start at (0, 0) in top-left
        // Each tile is 8x8 pixels
        
        // Offset by map offset to get tile coordinates
        bn::fixed offset_x = world_pos.x() + MAP_OFFSET_X;
        bn::fixed offset_y = world_pos.y() + MAP_OFFSET_Y;
        
        // Divide by 8 to get tile coordinates (each tile is 8x8 pixels)
        tile_x = (offset_x / 8).integer();
        tile_y = (offset_y / 8).integer();
    }
}
