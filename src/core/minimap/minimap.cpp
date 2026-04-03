#include "str_minimap.h"

#include "bn_fixed_point.h"
#include "bn_sprite_builder.h"
#include "bn_blending.h"

#include "bn_sprite_items_minimap_room.h"
#include "bn_sprite_items_minimap_room_big.h"
#include "bn_sprite_items_minimap_bg.h"
#include "bn_sprite_items_minimap_arrow.h"
#include "str_constants.h"

namespace str
{

Minimap::Minimap() :
    _bg_panel(bn::sprite_items::minimap_bg.create_sprite(MINIMAP_PANEL_X, MINIMAP_PANEL_Y)),
    _room_sprites{
        bn::sprite_items::minimap_room_big.create_sprite(0, 0, 0),
        bn::sprite_items::minimap_room.create_sprite(0, 0, 0)
    },
    _player_arrow(bn::sprite_items::minimap_arrow.create_sprite(MINIMAP_PANEL_X, MINIMAP_PANEL_Y, 0)),
    _current_room(-1),
    _panel_center(bn::fixed_point(MINIMAP_PANEL_X, MINIMAP_PANEL_Y)),
    _pulse_counter(0)
{
    for(int i = 0; i < MINIMAP_NUM_ROOMS; ++i)
    {
        _room_states[i] = RoomState::UNVISITED;
        _last_applied_frame[i] = 0;
    }

    _configure_hud_sprite(_bg_panel, Z_ORDER_MINIMAP_BG);
    _bg_panel.set_horizontal_scale(MINIMAP_BORDER_SCALE);
    _bg_panel.set_vertical_scale(MINIMAP_BORDER_SCALE);
    _bg_panel.set_blending_enabled(true);
    bn::blending::set_transparency_alpha(0.7);

    for(int i = 0; i < MINIMAP_NUM_ROOMS; ++i)
    {
        _configure_hud_sprite(_room_sprites[i], Z_ORDER_MINIMAP_ROOM);
    }

    _configure_hud_sprite(_player_arrow, Z_ORDER_MINIMAP_PLAYER);
    _create_door_connectors();

    bn::fixed_point initial_logical = _room_screen_pos(0);
    bn::fixed_point scroll_offset(_panel_center.x() - initial_logical.x(),
                                  _panel_center.y() - initial_logical.y());
    _scroll_sprites(scroll_offset);
}

void Minimap::update(bn::fixed_point player_pos, int /*facing_direction*/)
{
    int new_room = _find_room_room_viewer(player_pos);

    if(new_room >= 0)
    {
        bool room_changed = _current_room != new_room;

        if(_current_room >= 0 && room_changed)
        {
            _room_states[_current_room] = RoomState::VISITED;
        }

        if(room_changed)
        {
            _current_room = new_room;
            _room_states[_current_room] = RoomState::CURRENT;
            _update_room_visuals();
        }

        bn::fixed_point player_logical = _world_to_minimap_room_viewer(player_pos, _current_room);
        bn::fixed_point scroll_offset(_panel_center.x() - player_logical.x(),
                                      _panel_center.y() - player_logical.y());
        _scroll_sprites(scroll_offset);
        _player_arrow.set_position(_panel_center);
        _player_arrow.set_visible(true);
    }
    else
    {
        _player_arrow.set_visible(false);
    }

    _update_pulse();
}

void Minimap::set_visible(bool visible)
{
    _bg_panel.set_visible(visible);

    for(int i = 0; i < MINIMAP_NUM_ROOMS; ++i)
    {
        _room_sprites[i].set_visible(visible);
    }

    _player_arrow.set_visible(visible);

    for(auto& door : _door_sprites)
    {
        door.set_visible(visible);
    }
}

}
