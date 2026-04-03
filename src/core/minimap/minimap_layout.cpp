#include "str_minimap.h"
#include "str_constants.h"

#include "bn_blending.h"
#include "bn_math.h"

#include "bn_sprite_items_minimap_door.h"

namespace str
{

void Minimap::_configure_hud_sprite(bn::sprite_ptr& sprite, int z_order)
{
    sprite.set_bg_priority(0);
    sprite.remove_camera();
    sprite.set_z_order(z_order);
    sprite.set_visible(true);
}

bn::fixed_point Minimap::_room_screen_pos(int room_id) const
{
    switch(room_id)
    {
        case 0:
            return bn::fixed_point(0, MINIMAP_GRID_CELL / 2);

        case 1:
            return bn::fixed_point(0, -MINIMAP_GRID_CELL / 2);

        default:
            return bn::fixed_point(0, 0);
    }
}

int Minimap::_find_room(bn::fixed_point world_pos) const
{
    int best_room = -1;
    bn::fixed best_score;

    for(int i = 0; i < MINIMAP_NUM_ROOMS; ++i)
    {
        bn::fixed dx = world_pos.x() - _room_center_x[i];
        bn::fixed dy = world_pos.y() - _room_center_y[i];

        if(bn::abs(dx) <= _room_half_x[i] && bn::abs(dy) <= _room_half_y[i])
        {
            bn::fixed score = (bn::abs(dx) / _room_half_x[i]) + (bn::abs(dy) / _room_half_y[i]);

            if(best_room < 0 || score < best_score || (score == best_score && i == _current_room))
            {
                best_room = i;
                best_score = score;
            }
        }
    }

    return best_room;
}

int Minimap::_find_room_room_viewer(bn::fixed_point world_pos) const
{
    if(_current_room >= 0)
    {
        bn::fixed current_dx = bn::abs(world_pos.x() - _room_center_x[_current_room]);
        bn::fixed current_dy = bn::abs(world_pos.y() - _room_center_y[_current_room]);

        if(current_dx <= _room_half_x[_current_room] && current_dy <= _room_half_y[_current_room])
        {
            return _current_room;
        }
    }

    return _find_room(world_pos);
}

bn::fixed_point Minimap::_world_to_minimap_room_viewer(bn::fixed_point world_pos, int room_id) const
{
    bn::fixed room_viewer_playable_half_x = _room_half_x[room_id] - _playable_edge_inset;
    bn::fixed room_viewer_playable_half_y = _room_half_y[room_id] - _playable_edge_inset;
    bn::fixed room_viewer_half_inner = _is_big_room[room_id] ? bn::fixed(6) : bn::fixed(5);

    bn::fixed_point room_logical = _room_screen_pos(room_id);
    bn::fixed nx = (world_pos.x() - _room_center_x[room_id]) / room_viewer_playable_half_x;
    bn::fixed ny = (world_pos.y() - _room_center_y[room_id]) / room_viewer_playable_half_y;
    nx = bn::clamp(nx, bn::fixed(-1), bn::fixed(1));
    ny = bn::clamp(ny, bn::fixed(-1), bn::fixed(1));

    bn::fixed sx = room_logical.x() + nx * room_viewer_half_inner;
    bn::fixed sy = room_logical.y() + ny * room_viewer_half_inner;
    return bn::fixed_point(sx, sy);
}

void Minimap::_update_room_visuals()
{
    for(int i = 0; i < MINIMAP_NUM_ROOMS; ++i)
    {
        _room_sprites[i].set_visible(_room_states[i] != RoomState::UNVISITED);
    }
}

void Minimap::_create_door_connectors()
{
    for(int d = 0; d < DOOR_COUNT; ++d)
    {
        int room_a = _door_pairs[d][0];
        int room_b = _door_pairs[d][1];
        bn::sprite_ptr door = bn::sprite_items::minimap_door.create_sprite(0, 0);
        _configure_hud_sprite(door, Z_ORDER_MINIMAP_DOOR);

        bn::fixed_point pos_a = _room_screen_pos(room_a);
        bn::fixed_point pos_b = _room_screen_pos(room_b);

        if(bn::abs(pos_a.x() - pos_b.x()) > bn::abs(pos_a.y() - pos_b.y()))
        {
            door.set_rotation_angle(90);
        }

        door.set_blending_enabled(true);
        _door_sprites.push_back(bn::move(door));
    }
}

void Minimap::_scroll_sprites(bn::fixed_point scroll_offset)
{
    bn::fixed panel_half = bn::fixed(32) * MINIMAP_BORDER_SCALE;
    bn::fixed room_half = MINIMAP_ROOM_SIZE / 2;
    bn::fixed room_clip = panel_half - room_half;
    bn::fixed door_clip = panel_half - 4;

    for(int i = 0; i < MINIMAP_NUM_ROOMS; ++i)
    {
        bn::fixed_point logical = _room_screen_pos(i);
        bn::fixed sx = logical.x() + scroll_offset.x();
        bn::fixed sy = logical.y() + scroll_offset.y();
        _room_sprites[i].set_position(sx, sy);

        bn::fixed dx = bn::abs(sx - _panel_center.x());
        bn::fixed dy = bn::abs(sy - _panel_center.y());
        bool in_bounds = dx <= room_clip && dy <= room_clip;
        _room_sprites[i].set_visible(in_bounds && _room_states[i] != RoomState::UNVISITED);
    }

    for(int d = 0; d < DOOR_COUNT && d < _door_sprites.size(); ++d)
    {
        int room_a = _door_pairs[d][0];
        int room_b = _door_pairs[d][1];

        bn::fixed_point pos_a = _room_screen_pos(room_a);
        bn::fixed_point pos_b = _room_screen_pos(room_b);
        bn::fixed mid_x = (pos_a.x() + pos_b.x()) / 2 + scroll_offset.x();
        bn::fixed mid_y = (pos_a.y() + pos_b.y()) / 2 + scroll_offset.y();
        _door_sprites[d].set_position(mid_x, mid_y);

        bool room_a_revealed = _room_states[room_a] != RoomState::UNVISITED;
        bool room_b_revealed = _room_states[room_b] != RoomState::UNVISITED;
        bn::fixed dx = bn::abs(mid_x - _panel_center.x());
        bn::fixed dy = bn::abs(mid_y - _panel_center.y());
        _door_sprites[d].set_visible(room_a_revealed && room_b_revealed && dx <= door_clip && dy <= door_clip);
    }
}

void Minimap::_update_pulse()
{
    _pulse_counter = (_pulse_counter + 1) % 60;
    int half = _pulse_counter > 30 ? 60 - _pulse_counter : _pulse_counter;
    bn::fixed alpha = bn::fixed(55 + half * 15 / 10) / 100;
    bn::blending::set_transparency_alpha(alpha);
}

}
