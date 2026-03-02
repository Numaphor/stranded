#include "str_minimap_canvas.h"

#include "bn_algorithm.h"
#include "bn_bg_tiles.h"
#include "bn_memory.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_map_cell_info.h"
#include "bn_sprite_items_minimap_player.h"

#include "bn_bg_palette_items_hud_palette.h"
#include "bn_regular_bg_tiles_items_hud_tiles.h"

namespace str
{

    alignas(int) BN_DATA_EWRAM bn::regular_bg_map_cell MinimapCanvas::_cells[MinimapCanvas::MAP_CELLS];

    bn::regular_bg_ptr MinimapCanvas::_create_bg(const bn::regular_bg_map_item& map_item)
    {
        bn::bg_tiles::set_allow_offset(false);

        bn::regular_bg_item bg_item(
                bn::regular_bg_tiles_items::hud_tiles,
                bn::bg_palette_items::hud_palette,
                map_item);

        bn::regular_bg_ptr bg = bg_item.create_bg(0, 0);
        bn::bg_tiles::set_allow_offset(true);
        return bg;
    }

    int MinimapCanvas::_room_tile_x(int room_id)
    {
        int col = room_id % MINIMAP_ROOM_COLS;
        return ROOM_ORIGIN_X + (col * (ROOM_TILE_SIZE + ROOM_TILE_GAP));
    }

    int MinimapCanvas::_room_tile_y(int room_id)
    {
        int row = room_id / MINIMAP_ROOM_COLS;
        return ROOM_ORIGIN_Y + (row * (ROOM_TILE_SIZE + ROOM_TILE_GAP));
    }

    int MinimapCanvas::_room_palette(RoomState room_state)
    {
        switch(room_state)
        {
            case RoomState::UNVISITED:
                return PALETTE_UNVISITED;

            case RoomState::VISITED:
                return PALETTE_VISITED;

            case RoomState::CURRENT:
                return PALETTE_CURRENT;

            default:
                return PALETTE_UNVISITED;
        }
    }

    void MinimapCanvas::_set_cell(int x, int y, int tile_index, int palette_id, bool hflip, bool vflip)
    {
        int index = y * MAP_COLUMNS + x;
        bn::regular_bg_map_cell_info cell_info(_cells[index]);
        cell_info.set_tile_index(tile_index);
        cell_info.set_palette_id(palette_id);
        cell_info.set_horizontal_flip(hflip);
        cell_info.set_vertical_flip(vflip);
        _cells[index] = cell_info.cell();
    }

    MinimapCanvas::MinimapCanvas() :
        _map_item(_cells[0], bn::size(MAP_COLUMNS, MAP_ROWS)),
        _bg(_create_bg(_map_item)),
        _bg_map(_bg.map()),
        _current_room(-1),
        _player_sprite(bn::sprite_items::minimap_player.create_sprite(0, 0))
    {
        bn::memory::clear(_cells);

        for(int i = 0; i < MINIMAP_NUM_ROOMS; ++i)
        {
            _room_states[i] = RoomState::UNVISITED;
        }

        _bg.set_priority(0);
        _bg.set_visible(true);

        _player_sprite.set_bg_priority(0);
        _player_sprite.set_z_order(Z_ORDER_MINIMAP_PLAYER);
        _player_sprite.set_visible(false);

        _draw_panel();
        _clear_dynamic_tiles();
        _bg_map.reload_cells_ref();
    }

    void MinimapCanvas::_draw_panel()
    {
        for(int y = PANEL_Y; y < PANEL_Y + PANEL_HEIGHT; ++y)
        {
            for(int x = PANEL_X; x < PANEL_X + PANEL_WIDTH; ++x)
            {
                _set_cell(x, y, TILE_SOLID, PALETTE_PANEL, false, false);
            }
        }
    }

    void MinimapCanvas::_draw_room(int room_id)
    {
        int base_x = _room_tile_x(room_id);
        int base_y = _room_tile_y(room_id);
        RoomState room_state = _room_states[room_id];
        int palette = _room_palette(room_state);

        for(int y = 0; y < ROOM_TILE_SIZE; ++y)
        {
            for(int x = 0; x < ROOM_TILE_SIZE; ++x)
            {
                _set_cell(base_x + x, base_y + y, TILE_SOLID, palette, false, false);
            }
        }

    }

    void MinimapCanvas::_clear_dynamic_tiles()
    {
        for(int room_id = 0; room_id < MINIMAP_NUM_ROOMS; ++room_id)
        {
            _draw_room(room_id);
        }
    }

    int MinimapCanvas::_find_room(bn::fixed_point world_pos) const
    {
        for(int i = 0; i < MINIMAP_NUM_ROOMS; ++i)
        {
            bn::fixed dx = world_pos.x() - _room_center_x[i];
            bn::fixed dy = world_pos.y() - _room_center_y[i];

            if((dx >= -_room_half_x[i] && dx <= _room_half_x[i]) &&
               (dy >= -_room_half_y[i] && dy <= _room_half_y[i]))
            {
                return i;
            }
        }

        return -1;
    }

    bn::fixed_point MinimapCanvas::_world_to_minimap_pixel(bn::fixed_point world_pos, int room_id) const
    {
        // Normalize player position within room to [-1, 1]
        bn::fixed nx = -((world_pos.x() - _room_center_x[room_id]) / _room_half_x[room_id]);
        bn::fixed ny = -((world_pos.y() - _room_center_y[room_id]) / _room_half_y[room_id]);

        nx = bn::clamp(nx, bn::fixed(-1), bn::fixed(1));
        ny = bn::clamp(ny, bn::fixed(-1), bn::fixed(1));

        // Room center in screen pixel coordinates
        // BG is 256x256 pixels centered at (0,0), so tile (0,0) top-left is at (-128, -128)
        int room_tile_x = _room_tile_x(room_id);
        int room_tile_y = _room_tile_y(room_id);
        bn::fixed room_center_screen_x = bn::fixed(-128 + room_tile_x * 8 + ROOM_TILE_SIZE * 4);
        bn::fixed room_center_screen_y = bn::fixed(-128 + room_tile_y * 8 + ROOM_TILE_SIZE * 4);

        // Half-inner: usable area within room for positioning (1 pixel inset from edge)
        bn::fixed half_inner = bn::fixed(ROOM_TILE_SIZE * 4 - 1);

        bn::fixed screen_x = room_center_screen_x + nx * half_inner;
        bn::fixed screen_y = room_center_screen_y + ny * half_inner;

        return bn::fixed_point(screen_x, screen_y);
    }

    void MinimapCanvas::update(bn::fixed_point player_pos, int)
    {
        int new_room = _find_room(player_pos);

        if(new_room >= 0)
        {
            if(_current_room >= 0 && _current_room != new_room)
            {
                _room_states[_current_room] = RoomState::VISITED;
            }

            _current_room = new_room;
            _room_states[_current_room] = RoomState::CURRENT;
        }

        _clear_dynamic_tiles();

        if(_current_room >= 0)
        {
            bn::fixed_point pixel_pos = _world_to_minimap_pixel(player_pos, _current_room);
            _player_sprite.set_position(pixel_pos);
            _player_sprite.set_visible(true);
        }
        else
        {
            _player_sprite.set_visible(false);
        }

        _bg_map.reload_cells_ref();
    }

    void MinimapCanvas::set_visible(bool visible)
    {
        _bg.set_visible(visible);
        _player_sprite.set_visible(visible && _current_room >= 0);
    }
}
