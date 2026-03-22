#pragma once

#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "str_constants.h"

namespace str
{
    enum class RoomState : int
    {
        UNVISITED = 0,
        VISITED = 1,
        CURRENT = 2
    };

    class Minimap
    {
    public:
        Minimap();
        ~Minimap() = default;

        void update(bn::fixed_point player_pos, int facing_direction);
        void set_visible(bool visible);

    private:
        bn::sprite_ptr _bg_panel;
        bn::sprite_ptr _room_sprites[MINIMAP_NUM_ROOMS];
        bn::vector<bn::sprite_ptr, 8> _door_sprites;
        bn::sprite_ptr _player_arrow;
        RoomState _room_states[MINIMAP_NUM_ROOMS];
        int _current_room;
        bn::fixed_point _panel_center;
        int _pulse_counter;
        int _last_applied_frame[MINIMAP_NUM_ROOMS];

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

        static constexpr int DOOR_COUNT = 7;
        static constexpr int _door_pairs[DOOR_COUNT][2] = {
            {0, 1}, {0, 2}, {1, 3}, {2, 3}, {2, 4}, {3, 5}, {4, 5}
        };

        static constexpr bool _is_big_room[MINIMAP_NUM_ROOMS] = {
            false, true, false, false, false, true
        };

        void _configure_hud_sprite(bn::sprite_ptr& sprite, int z_order);
        bn::fixed_point _room_screen_pos(int room_id) const;
        int _find_room(bn::fixed_point world_pos) const;
        int _find_room_room_viewer(bn::fixed_point world_pos) const;
        bn::fixed_point _world_to_minimap(bn::fixed_point world_pos, int room_id) const;
        bn::fixed_point _world_to_minimap_room_viewer(bn::fixed_point world_pos, int room_id) const;
        void _update_room_visuals();
        void _create_door_connectors();
        void _update_pulse();
        void _scroll_sprites(bn::fixed_point scroll_offset);
    };
}
