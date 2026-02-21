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
#include "models/str_model_3d_items_building.h"

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

    struct rect_bounds
    {
        bn::fixed min_x, max_x, min_y, max_y;
    };

    constexpr rect_bounds building_rooms[] = {
        { -117, -3, -177, -63 },  // Room 0
        {    3, 117, -177, -63 },  // Room 1
        { -117, -3,  -57,  57 },  // Room 2
        {    3, 117,  -57,  57 },  // Room 3
        { -117, -3,   63, 177 },  // Room 4
        {    3, 117,   63, 177 },  // Room 5
    };

    constexpr rect_bounds building_doors[] = {
        { -10,  10, -128, -112 },  // Door between Room 0 & 1
        { -10,  10,   -8,    8 },  // Door between Room 2 & 3
        { -10,  10,  112,  128 },  // Door between Room 4 & 5
        { -68, -52,  -63,  -57 },  // Door between Room 0 & 2
        {  52,  68,  -63,  -57 },  // Door between Room 1 & 3
        { -68, -52,   57,   63 },  // Door between Room 2 & 4
        {  52,  68,   57,   63 },  // Door between Room 3 & 5
    };

    int get_current_room_id(bn::fixed px, bn::fixed py)
    {
        for(int i = 0; i < 6; ++i)
        {
            const auto& r = building_rooms[i];
            if(px >= r.min_x && px <= r.max_x && py >= r.min_y && py <= r.max_y)
            {
                return i;
            }
        }
        return -1;
    }

    bool is_valid_building_position(bn::fixed px, bn::fixed py)
    {
        if(get_current_room_id(px, py) >= 0)
        {
            return true;
        }
        for(const auto& d : building_doors)
        {
            if(px >= d.min_x && px <= d.max_x && py >= d.min_y && py <= d.max_y)
            {
                return true;
            }
        }
        return false;
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

    _models.load_colors(str::model_3d_items::building_model_colors);

    fr::model_3d& building = _models.create_dynamic_model(str::model_3d_items::building);

    fr::point_3d building_base_pos(0, 96, 16);
    building.set_position(building_base_pos);
    building.set_depth_bias(1000000);

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
        set_model_rotation(building, cm);
    };

    update_all_orientations();

    bn::fixed cam_dist = 274;

    auto update_camera = [&]() {
        _camera.set_position(fr::point_3d(0, cam_dist, 0));
    };

    _camera.set_phi(0);
    update_camera();

    _player_fx = -60;
    _player_fy = 0;
    _player_fz = -10;

    fr::sprite_3d_item player_sprite_item(bn::sprite_items::eris, 8);
    fr::sprite_3d& player_sprite = _models.create_sprite(player_sprite_item);
    player_sprite.set_scale(2);
    player_sprite.set_position(building.transform(fr::vertex_3d(_player_fx, _player_fy, _player_fz)));

    bn::sprite_text_generator tg(common::variable_8x8_sprite_font);

    while(true)
    {
        if(bn::keypad::b_pressed())
        {
            _models.destroy_sprite(player_sprite);
            _models.destroy_dynamic_model(building);
            _models.update(_camera);
            bn::core::update();
            return str::Scene::START;
        }

        if(bn::keypad::start_pressed())
        {
            _corner_index = (_corner_index + 1) % 4;
            update_all_orientations();
            _rotate_player_dir(player_sprite);
            player_sprite.set_position(building.transform(
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

            bn::fixed new_fx = _player_fx + dfx;
            bn::fixed new_fy = _player_fy + dfy;

            if(is_valid_building_position(new_fx, new_fy))
            {
                _player_fx = new_fx;
                _player_fy = new_fy;
            }
            else
            {
                bn::fixed slide_fx = _player_fx + dfx;
                if(is_valid_building_position(slide_fx, _player_fy))
                {
                    _player_fx = slide_fx;
                }
                else
                {
                    bn::fixed slide_fy = _player_fy + dfy;
                    if(is_valid_building_position(_player_fx, slide_fy))
                    {
                        _player_fy = slide_fy;
                    }
                }
            }

            player_sprite.set_position(building.transform(
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
            stream2 << "Room:" << get_current_room_id(_player_fx, _player_fy) << " C:" << _corner_index;
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
