#pragma once

#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_sprite_ptr.h"
#include "bn_regular_bg_map_cell.h"
#include "bn_regular_bg_map_item.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_ptr.h"

#include "str_constants.h"

namespace str
{

    enum class RoomState : int
    {
        UNVISITED = 0,
        VISITED = 1,
        CURRENT = 2
    };

    class MinimapCanvas
    {
    public:
        MinimapCanvas();
        ~MinimapCanvas() = default;

        void update(bn::fixed_point player_pos, int facing_direction);
        void set_visible(bool visible);

    private:
        static constexpr int MAP_COLUMNS = 32;
        static constexpr int MAP_ROWS = 32;
        static constexpr int MAP_CELLS = MAP_COLUMNS * MAP_ROWS;

        static constexpr int PANEL_X = 24;
        static constexpr int PANEL_Y = 7;
        static constexpr int PANEL_WIDTH = 5;
        static constexpr int PANEL_HEIGHT = 8;

        static constexpr int ROOM_TILE_SIZE = 2;
        static constexpr int ROOM_TILE_GAP = 1;
        static constexpr int ROOM_ORIGIN_X = PANEL_X;
        static constexpr int ROOM_ORIGIN_Y = PANEL_Y;

        static constexpr int TILE_EMPTY = 0;
        static constexpr int TILE_SOLID = 1;
        static constexpr int TILE_BORDER = 2;

        static constexpr int PALETTE_PANEL = 0;
        static constexpr int PALETTE_UNVISITED = 1;
        static constexpr int PALETTE_VISITED = 2;
        static constexpr int PALETTE_CURRENT = 3;
        static constexpr int PALETTE_PLAYER = 4;

        static constexpr bn::fixed _room_center_x[MINIMAP_NUM_ROOMS] = {
            bn::fixed(-60), bn::fixed(75), bn::fixed(-60),
            bn::fixed(60), bn::fixed(-60), bn::fixed(75)
        };

        static constexpr bn::fixed _room_center_y[MINIMAP_NUM_ROOMS] = {
            bn::fixed(-120), bn::fixed(-135), bn::fixed(0),
            bn::fixed(0), bn::fixed(120), bn::fixed(135)
        };

        static constexpr bn::fixed _room_half_x[MINIMAP_NUM_ROOMS] = {
            bn::fixed(60), bn::fixed(75), bn::fixed(60),
            bn::fixed(60), bn::fixed(60), bn::fixed(75)
        };

        static constexpr bn::fixed _room_half_y[MINIMAP_NUM_ROOMS] = {
            bn::fixed(60), bn::fixed(75), bn::fixed(60),
            bn::fixed(60), bn::fixed(60), bn::fixed(75)
        };

        alignas(int) static BN_DATA_EWRAM bn::regular_bg_map_cell _cells[MAP_CELLS];

        bn::regular_bg_map_item _map_item;
        bn::regular_bg_ptr _bg;
        bn::regular_bg_map_ptr _bg_map;

        RoomState _room_states[MINIMAP_NUM_ROOMS];
        int _current_room;
        bn::sprite_ptr _player_sprite;

        static bn::regular_bg_ptr _create_bg(const bn::regular_bg_map_item& map_item);
        static int _room_tile_x(int room_id);
        static int _room_tile_y(int room_id);
        static int _room_palette(RoomState room_state);
        static void _set_cell(int x, int y, int tile_index, int palette_id, bool hflip, bool vflip);

        void _draw_panel();
        void _draw_room(int room_id);
        void _clear_dynamic_tiles();
        int _find_room(bn::fixed_point world_pos) const;
        bn::fixed_point _world_to_minimap_pixel(bn::fixed_point world_pos, int room_id) const;
    };
}
