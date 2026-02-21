#include "str_scene_room_viewer.h"

#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_bg_palettes.h"
#include "bn_color.h"
#include "bn_sprite_text_generator.h"
#include "bn_fixed.h"
#include "bn_math.h"
#include "bn_algorithm.h"
#include "bn_assert.h"
#include "bn_string.h"
#include "bn_sstream.h"
#include "bn_log.h"
#include "bn_sprite_items_eris.h"

#include "common_variable_8x8_sprite_font.h"

#include "fr_sin_cos.h"
#include "fr_div_lut.h"
#include "fr_constants_3d.h"
#include "models/str_model_3d_items_room.h"
#include "models/str_model_3d_items_table.h"
#include "models/str_model_3d_items_chair.h"

namespace {
    constexpr int iso_phi = 6400;
    constexpr int iso_theta = 59904;
    constexpr int iso_psi = 6400;

    struct corner_matrix
    {
        bn::fixed r00, r01, r02;
        bn::fixed r10, r11, r12;
        bn::fixed r20, r21, r22;
    };

    void compute_corner_matrices(corner_matrix out[4])
    {
        bn::fixed sp = fr::sin(iso_phi), cp = fr::cos(iso_phi);
        bn::fixed st = fr::sin(iso_theta), ct = fr::cos(iso_theta);
        bn::fixed ss = fr::sin(iso_psi),  cs = fr::cos(iso_psi);

        bn::fixed c0x = cp * ct;
        bn::fixed c0y = cp * st * ss - sp * cs;
        bn::fixed c0z = cp * st * cs + sp * ss;
        bn::fixed c1x = sp * ct;
        bn::fixed c1y = sp * st * ss + cp * cs;
        bn::fixed c1z = sp * st * cs - cp * ss;
        bn::fixed c2x = -st;
        bn::fixed c2y = ct * ss;
        bn::fixed c2z = ct * cs;

        out[0] = { c0x, c0y, c0z,  c1x, c1y, c1z,  c2x, c2y, c2z };
        out[1] = { c0y, -c0x, c0z,  c1y, -c1x, c1z,  c2y, -c2x, c2z };
        out[2] = { -c0x, -c0y, c0z,  -c1x, -c1y, c1z,  -c2x, -c2y, c2z };
        out[3] = { -c0y, c0x, c0z,  -c1y, c1x, c1z,  -c2y, c2x, c2z };
    }

    constexpr bn::fixed TABLE_FX = 10;
    constexpr bn::fixed TABLE_FY = 0;
    constexpr bn::fixed CHAIR_FX = 10;
    constexpr bn::fixed CHAIR_FY = 16;

    constexpr bn::fixed TABLE_HW = 12;
    constexpr bn::fixed TABLE_HD = 8;
    constexpr bn::fixed CHAIR_HW = 6;
    constexpr bn::fixed CHAIR_HD = 6;

    constexpr bn::fixed FLOOR_MIN = -55;
    constexpr bn::fixed FLOOR_MAX = 55;

    struct floor_aabb
    {
        bn::fixed cx, cy, hw, hd;
    };

    constexpr floor_aabb furniture_boxes[] = {
        { TABLE_FX, TABLE_FY, TABLE_HW, TABLE_HD },
        { CHAIR_FX, CHAIR_FY, CHAIR_HW, CHAIR_HD },
    };

    bool overlaps_any_furniture(bn::fixed px, bn::fixed py)
    {
        for(const auto& box : furniture_boxes)
        {
            if(bn::abs(px - box.cx) < box.hw && bn::abs(py - box.cy) < box.hd)
            {
                return true;
            }
        }
        return false;
    }

    // Building layout: 2 columns x 3 rows
    //
    //   col 0   col 1
    //  +------+------+
    //  |  R0  |  R1  |  row 0
    //  +--||--+--||--+
    //  |  R2  |  R3  |  row 1
    //  +--||--+--||--+
    //  |  R4  |  R5  |  row 2
    //  +------+------+
    //
    // Each room is 120x120, local coords [-60,60] x [-60,60].
    // Doors are at room edges (player local coord near ±55).

    constexpr int NUM_ROOMS = 6;

    // Per-room color palettes (same structure as room_model_colors but with unique floor colors)
    constexpr inline bn::color room_palettes[NUM_ROOMS][9] = {
        { // Room 0 - warm brown floor (same as original)
            bn::color(22, 16, 8), bn::color(16, 10, 4),
            bn::color(28, 26, 22), bn::color(22, 20, 16),
            bn::color(16, 22, 28), bn::color(10, 8, 6),
            bn::color(18, 12, 6), bn::color(8, 12, 18), bn::color(20, 14, 8)
        },
        { // Room 1 - blue-gray floor
            bn::color(12, 14, 22), bn::color(8, 10, 16),
            bn::color(28, 26, 22), bn::color(22, 20, 16),
            bn::color(16, 22, 28), bn::color(10, 8, 6),
            bn::color(18, 12, 6), bn::color(8, 12, 18), bn::color(20, 14, 8)
        },
        { // Room 2 - green floor
            bn::color(10, 20, 10), bn::color(6, 14, 6),
            bn::color(28, 26, 22), bn::color(22, 20, 16),
            bn::color(16, 22, 28), bn::color(10, 8, 6),
            bn::color(18, 12, 6), bn::color(8, 12, 18), bn::color(20, 14, 8)
        },
        { // Room 3 - terra cotta floor
            bn::color(22, 10, 8), bn::color(16, 6, 4),
            bn::color(28, 26, 22), bn::color(22, 20, 16),
            bn::color(16, 22, 28), bn::color(10, 8, 6),
            bn::color(18, 12, 6), bn::color(8, 12, 18), bn::color(20, 14, 8)
        },
        { // Room 4 - purple floor
            bn::color(16, 10, 20), bn::color(10, 6, 14),
            bn::color(28, 26, 22), bn::color(22, 20, 16),
            bn::color(16, 22, 28), bn::color(10, 8, 6),
            bn::color(18, 12, 6), bn::color(8, 12, 18), bn::color(20, 14, 8)
        },
        { // Room 5 - teal floor
            bn::color(8, 18, 20), bn::color(4, 12, 14),
            bn::color(28, 26, 22), bn::color(22, 20, 16),
            bn::color(16, 22, 28), bn::color(10, 8, 6),
            bn::color(18, 12, 6), bn::color(8, 12, 18), bn::color(20, 14, 8)
        }
    };

    // Door transition info: which direction, which neighbor room
    // Doors are at the edges of local room space:
    //   North door (y near -55): go to row-1 (same column)
    //   South door (y near +55): go to row+1 (same column)
    //   West door (x near -55):  go to col-1 (same row) -- but only col0->col1 via center wall
    //   East door (x near +55):  go to col+1 (same row) -- but only col0->col1 via center wall
    //
    // In our 2x3 grid (col, row): R0=(0,0), R1=(1,0), R2=(0,1), R3=(1,1), R4=(0,2), R5=(1,2)

    int room_col(int room_id) { return room_id % 2; }
    int room_row(int room_id) { return room_id / 2; }
    int room_id_from_grid(int col, int row) { return row * 2 + col; }

    // Check if a door transition should occur and return the new room id (-1 if no transition)
    // Also sets new_local_x/y for the player position in the new room
    int check_door_transition(int current_room, bn::fixed local_x, bn::fixed local_y,
                              bn::fixed& new_local_x, bn::fixed& new_local_y)
    {
        int col = room_col(current_room);
        int row = room_row(current_room);

        // East door: player at x > 55, center of room (|y| < 10)
        // Only col 0 has an east neighbor (col 1)
        if(col == 0 && local_x > 55 && local_y > -10 && local_y < 10)
        {
            new_local_x = -55;
            new_local_y = local_y;
            return room_id_from_grid(1, row);
        }

        // West door: player at x < -55, center of room (|y| < 10)
        // Only col 1 has a west neighbor (col 0)
        if(col == 1 && local_x < -55 && local_y > -10 && local_y < 10)
        {
            new_local_x = 55;
            new_local_y = local_y;
            return room_id_from_grid(0, row);
        }

        // South door: player at y > 55, center of room (|x| < 10)
        // Only rows 0,1 have a south neighbor
        if(row < 2 && local_y > 55 && local_x > -10 && local_x < 10)
        {
            new_local_x = local_x;
            new_local_y = -55;
            return room_id_from_grid(col, row + 1);
        }

        // North door: player at y < -55, center of room (|x| < 10)
        // Only rows 1,2 have a north neighbor
        if(row > 0 && local_y < -55 && local_x > -10 && local_x < 10)
        {
            new_local_x = local_x;
            new_local_y = 55;
            return room_id_from_grid(col, row - 1);
        }

        return -1;
    }
}

namespace str
{

RoomViewer::RoomViewer() {}

void RoomViewer::_update_player_anim_tiles(fr::sprite_3d_item& item, bool moving, int dir)
{
    constexpr int idle_bases[] = {8, 12, 16, 20, 24};
    constexpr int walk_bases[] = {28, 32, 36, 40, 44};
    constexpr int frames_per_anim = 4;
    constexpr int anim_speed = 8;

    if(_player_moving != moving || _player_dir != dir)
    {
        _player_moving = moving;
        _player_dir = dir;
        _anim_frame_counter = 0;
    }

    int base_frame = moving ? walk_bases[dir] : idle_bases[dir];
    int frame_in_anim = (_anim_frame_counter / anim_speed) % frames_per_anim;
    int tile_index = base_frame + frame_in_anim;

    item.tiles().set_tiles_ref(bn::sprite_items::eris.tiles_item(), tile_index);

    ++_anim_frame_counter;
}

void RoomViewer::_rotate_player_dir(fr::sprite_3d& sprite)
{
    int eight_dir;
    if(_player_facing_left && _player_dir >= 1 && _player_dir <= 3)
    {
        eight_dir = 8 - _player_dir;
    }
    else
    {
        eight_dir = _player_dir;
    }

    eight_dir = (eight_dir + 6) % 8;

    static constexpr int dir_from_8[] = {0, 1, 2, 3, 4, 3, 2, 1};
    static constexpr bool flip_from_8[] = {false, false, false, false, false, true, true, true};

    _player_dir = dir_from_8[eight_dir];
    _player_facing_left = flip_from_8[eight_dir];

    sprite.set_horizontal_flip(_player_facing_left);
}

str::Scene RoomViewer::execute()
{
    bn::bg_palettes::set_transparent_color(bn::color(2, 2, 4));

    int current_room = 0;
    _models.load_colors(bn::span<const bn::color>(room_palettes[current_room], 9));

    fr::model_3d& room = _models.create_dynamic_model(str::model_3d_items::room);
    fr::model_3d& table = _models.create_dynamic_model(str::model_3d_items::table);
    fr::model_3d& chair = _models.create_dynamic_model(str::model_3d_items::chair);

    fr::point_3d room_base_pos(0, 96, 16);
    room.set_position(room_base_pos);
    room.set_depth_bias(1000000);

    corner_matrix all_corners[4];
    compute_corner_matrices(all_corners);

    auto set_model_rotation = [](fr::model_3d& m, const corner_matrix& cm) {
        m.set_rotation_matrix(
            cm.r00, cm.r01, cm.r02,
            cm.r10, cm.r11, cm.r12,
            cm.r20, cm.r21, cm.r22);
    };

    auto update_all_orientations = [&]() {
        const corner_matrix& cm = all_corners[_corner_index];
        set_model_rotation(room, cm);
        set_model_rotation(table, cm);
        set_model_rotation(chair, cm);

        table.set_position(room.transform(fr::vertex_3d(TABLE_FX, TABLE_FY, 0)));
        chair.set_position(room.transform(fr::vertex_3d(CHAIR_FX, CHAIR_FY, 0)));
    };

    update_all_orientations();

    bn::fixed cam_dist = 274;

    auto update_camera = [&]() {
        _camera.set_position(fr::point_3d(0, cam_dist, 0));
    };

    _camera.set_phi(0);
    update_camera();

    _player_fx = -20;
    _player_fy = 20;
    _player_fz = -10;

    fr::sprite_3d_item player_sprite_item(bn::sprite_items::eris, 8);
    fr::sprite_3d& player_sprite = _models.create_sprite(player_sprite_item);
    player_sprite.set_scale(2);
    player_sprite.set_position(room.transform(fr::vertex_3d(_player_fx, _player_fy, _player_fz)));

    bn::sprite_text_generator tg(common::variable_8x8_sprite_font);

    while(true)
    {
        if(bn::keypad::b_pressed())
        {
            _models.destroy_sprite(player_sprite);
            _models.destroy_dynamic_model(chair);
            _models.destroy_dynamic_model(table);
            _models.destroy_dynamic_model(room);
            _models.update(_camera);
            bn::core::update();
            return str::Scene::START;
        }

        if(bn::keypad::start_pressed())
        {
            _corner_index = (_corner_index + 1) % 4;
            update_all_orientations();
            _rotate_player_dir(player_sprite);
            player_sprite.set_position(room.transform(
                fr::vertex_3d(_player_fx, _player_fy, _player_fz)));
        }

        {
            bn::fixed old_dist = cam_dist;
            if(bn::keypad::l_held()) cam_dist = bn::max(bn::fixed(100), cam_dist - 3);
            else if(bn::keypad::r_held()) cam_dist = bn::min(bn::fixed(500), cam_dist + 3);
            if(cam_dist != old_dist)
            {
                update_camera();
            }
        }

        if(bn::keypad::select_pressed())
        {
            _debug_mode = !_debug_mode;
        }

        bool moving = false;
        int dir = _player_dir;
        bool facing_left = _player_facing_left;
        bn::fixed dfx = 0, dfy = 0;
        bn::fixed screen_dx = 0, screen_dy = 0;

        if(bn::keypad::up_held())
        {
            screen_dy = -1;
            moving = true;
        }
        else if(bn::keypad::down_held())
        {
            screen_dy = 1;
            moving = true;
        }

        if(bn::keypad::left_held())
        {
            screen_dx = -1;
            moving = true;
        }
        else if(bn::keypad::right_held())
        {
            screen_dx = 1;
            moving = true;
        }

        if(moving)
        {
            if(screen_dx == 0 && screen_dy == 1)       { dir = 0; facing_left = false; }
            else if(screen_dx == 0 && screen_dy == -1)  { dir = 4; facing_left = false; }
            else if(screen_dx == 1 && screen_dy == 0)   { dir = 2; facing_left = false; }
            else if(screen_dx == -1 && screen_dy == 0)  { dir = 2; facing_left = true;  }
            else if(screen_dx == 1 && screen_dy == 1)   { dir = 1; facing_left = false; }
            else if(screen_dx == -1 && screen_dy == 1)  { dir = 1; facing_left = true;  }
            else if(screen_dx == 1 && screen_dy == -1)  { dir = 3; facing_left = false; }
            else if(screen_dx == -1 && screen_dy == -1) { dir = 3; facing_left = true;  }

            bn::fixed speed_factor = 1;
            if(screen_dx != 0 && screen_dy != 0)
            {
                speed_factor = bn::fixed(0.707);
            }
            screen_dx *= speed_factor;
            screen_dy *= speed_factor;

            bn::fixed base_dx = screen_dx + screen_dy;
            bn::fixed base_dy = screen_dy - screen_dx;

            if(_corner_index == 0)      { dfx = base_dx;  dfy = base_dy;  }
            else if(_corner_index == 1) { dfx = base_dy;  dfy = -base_dx; }
            else if(_corner_index == 2) { dfx = -base_dx; dfy = -base_dy; }
            else if(_corner_index == 3) { dfx = -base_dy; dfy = base_dx;  }

            bn::fixed new_fx = bn::clamp(_player_fx + dfx, FLOOR_MIN, FLOOR_MAX);
            bn::fixed new_fy = bn::clamp(_player_fy + dfy, FLOOR_MIN, FLOOR_MAX);

            if(!overlaps_any_furniture(new_fx, new_fy))
            {
                _player_fx = new_fx;
                _player_fy = new_fy;
            }
            else
            {
                bn::fixed slide_fx = bn::clamp(_player_fx + dfx, FLOOR_MIN, FLOOR_MAX);
                bn::fixed slide_fy = _player_fy;
                if(!overlaps_any_furniture(slide_fx, slide_fy))
                {
                    _player_fx = slide_fx;
                }
                else
                {
                    slide_fx = _player_fx;
                    slide_fy = bn::clamp(_player_fy + dfy, FLOOR_MIN, FLOOR_MAX);
                    if(!overlaps_any_furniture(slide_fx, slide_fy))
                    {
                        _player_fy = slide_fy;
                    }
                }
            }

            // Check for room transition via doors
            bn::fixed new_local_x, new_local_y;
            int next_room = check_door_transition(current_room, _player_fx, _player_fy,
                                                   new_local_x, new_local_y);
            if(next_room >= 0 && next_room != current_room)
            {
                current_room = next_room;
                _player_fx = new_local_x;
                _player_fy = new_local_y;
                _models.load_colors(bn::span<const bn::color>(room_palettes[current_room], 9));
            }

            player_sprite.set_position(room.transform(
                fr::vertex_3d(_player_fx, _player_fy, _player_fz)));
        }

        player_sprite.set_horizontal_flip(facing_left);
        _player_facing_left = facing_left;
        _update_player_anim_tiles(player_sprite_item, moving, dir);

        _text_sprites.clear();
        tg.set_center_alignment();
        tg.set_bg_priority(0);
        if(_debug_mode)
        {
            bn::string<32> line1;
            bn::ostringstream stream1(line1);
            stream1 << "F:" << _player_fx.integer() << "," << _player_fy.integer() << " D:" << _player_dir;
            tg.generate(0, -72, line1, _text_sprites);

            bn::string<32> line2;
            bn::ostringstream stream2(line2);
            stream2 << "Room:" << current_room << " C:" << _corner_index;
            tg.generate(0, 72, line2, _text_sprites);
        }
        else
        {
            tg.generate(0, -72, "ROOM VIEWER", _text_sprites);
            tg.generate(0, 72, "L/R:Zoom START:Rotate B:Exit", _text_sprites);
        }

        _models.update(_camera);
        bn::core::update();
    }
}

}
