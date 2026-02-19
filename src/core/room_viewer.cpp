#include "str_scene_room_viewer.h"

#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_bg_palettes.h"
#include "bn_color.h"
#include "bn_sprite_text_generator.h"
#include "bn_fixed.h"
#include "bn_math.h"
#include "bn_string.h"
#include "bn_sstream.h"
#include "bn_sprite_items_eris.h"
#include "bn_sprite_animate_actions.h"

#include "common_variable_8x8_sprite_font.h"

#include "fr_sin_cos.h"
#include "models/str_model_3d_items_room.h"

namespace {
    constexpr int iso_phi = 6400;
    constexpr int iso_theta = 59904;
    constexpr int iso_psi = 6400;
}

namespace str
{

RoomViewer::RoomViewer() {}

void RoomViewer::_compute_rotation(int phi)
{
    bn::fixed sp = fr::sin(phi), cp = fr::cos(phi);
    bn::fixed st = fr::sin(iso_theta), ct = fr::cos(iso_theta);
    bn::fixed ss = fr::sin(iso_psi),  cs = fr::cos(iso_psi);

    _r00 = cp * ct;
    _r01 = cp * st * ss - sp * cs;
    _r10 = sp * ct;
    _r11 = sp * st * ss + cp * cs;
    _r20 = -st;
    _r21 = ct * ss;
}

void RoomViewer::_set_player_anim(bool moving, int dir)
{
    if(_player_moving == moving && _player_dir == dir && _player_anim.has_value())
    {
        return; // no change needed
    }

    _player_moving = moving;
    _player_dir = dir;

    // Frame indices in combined eris sheet (4 frames each):
    // Idle: down=8, down_side=12, side=16, up_side=20, up=24
    // Walk: down=28, down_side=32, side=36, up_side=40, up=44
    // dir: 0=down, 1=down_side, 2=side, 3=up_side, 4=up
    constexpr int idle_bases[] = {8, 12, 16, 20, 24};
    constexpr int walk_bases[] = {28, 32, 36, 40, 44};
    int base_frame = moving ? walk_bases[dir] : idle_bases[dir];

    const bn::sprite_tiles_item& tiles = bn::sprite_items::eris.tiles_item();

    _player_anim = bn::create_sprite_animate_action_forever(
        *_player_sprite, 8, tiles,
        base_frame, base_frame + 1, base_frame + 2, base_frame + 3);
}

void RoomViewer::_rotate_player_dir()
{
    // Convert (dir, facing_left) to 8-direction index (CW from down)
    // 0=down, 1=down-right, 2=right, 3=up-right, 4=up, 5=up-left, 6=left, 7=down-left
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

    _player_sprite->set_horizontal_flip(_player_facing_left);
    _player_anim.reset();
    _set_player_anim(_player_moving, _player_dir);
}

bn::fixed_point RoomViewer::_floor_to_screen(bn::fixed fx, bn::fixed fy, bn::fixed cam_y)
{
    bn::fixed world_x = _r00 * fx + _r01 * fy;
    bn::fixed world_y = _r10 * fx + _r11 * fy + 96;
    bn::fixed world_z = _r20 * fx + _r21 * fy + 16;

    bn::fixed depth = cam_y - world_y;
    if(depth <= 0) depth = 1;

    // Camera phi=0: vcx = world_x/16, vcy = world_z/16
    bn::fixed vrx = world_x / 16;
    bn::fixed vrz = world_z / 16;

    return bn::fixed_point(vrx * 256 / depth * 16, vrz * 256 / depth * 16);
}

void RoomViewer::_update_hud()
{
    _text_sprites.clear();
    bn::sprite_text_generator tg(common::variable_8x8_sprite_font);
    tg.set_center_alignment();
    tg.set_bg_priority(0);

    tg.generate(0, -72, "3D ROOM", _text_sprites);
    tg.generate(0, 72, "D-PAD:Move  L/R:Zoom  B:Back", _text_sprites);
}

str::Scene RoomViewer::execute()
{
    bn::bg_palettes::set_transparent_color(bn::color(2, 2, 4));

    _models.load_colors(str::model_3d_items::room_model_colors);

    fr::model_3d& room = _models.create_dynamic_model(str::model_3d_items::room);

    room.set_position(fr::point_3d(0, 96, 16));

    int effective_phi = iso_phi + _corner_index * 16384;
    _compute_rotation(effective_phi);

    room.set_phi(effective_phi);
    room.set_theta(iso_theta);
    room.set_psi(iso_psi);

    bn::fixed cam_dist = 274;

    auto get_cam_pos = [&]() {
        return fr::point_3d(0, cam_dist, 0);
    };

    auto update_camera = [&]() {
        _camera.set_position(get_cam_pos());
    };

    _camera.set_phi(0);
    update_camera();

    _player_fx = 0;
    _player_fy = 0;

    bn::fixed_point initial_pos = _floor_to_screen(_player_fx, _player_fy, cam_dist);
    _player_sprite = bn::sprite_items::eris.create_sprite(initial_pos.x(), initial_pos.y());
    _player_sprite->set_bg_priority(0);
    _set_player_anim(false, 0);

    bn::sprite_text_generator tg(common::variable_8x8_sprite_font);

    while(true)
    {
        if(bn::keypad::b_pressed())
        {
            _player_anim.reset();
            _player_sprite.reset();
            _models.destroy_dynamic_model(room);
            _models.update(_camera);
            bn::core::update();
            return str::Scene::START;
        }

        if(bn::keypad::start_pressed())
        {
            _corner_index = (_corner_index + 1) % 4;

            int eff_phi = iso_phi + _corner_index * 16384;
            room.set_phi(eff_phi);
            _compute_rotation(eff_phi);

            bn::fixed new_fx = _player_fy;
            bn::fixed new_fy = -_player_fx;
            _player_fx = new_fx;
            _player_fy = new_fy;

            _rotate_player_dir();

            bn::fixed_point pos = _floor_to_screen(_player_fx, _player_fy, cam_dist);
            _player_sprite->set_position(pos);
        }

        {
            bn::fixed old_dist = cam_dist;
            if(bn::keypad::l_held()) cam_dist = bn::max(bn::fixed(100), cam_dist - 3);
            else if(bn::keypad::r_held()) cam_dist = bn::min(bn::fixed(500), cam_dist + 3);
            if(cam_dist != old_dist)
            {
                update_camera();
                bn::fixed_point pos = _floor_to_screen(_player_fx, _player_fy, cam_dist);
                _player_sprite->set_position(pos);
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

        if(bn::keypad::down_held())
        {
            dfy += 1;
            dir = 0;
            moving = true;
        }
        else if(bn::keypad::up_held())
        {
            dfy -= 1;
            dir = 4;
            moving = true;
        }

        if(bn::keypad::left_held())
        {
            dfx -= 1;
            facing_left = true;
            if(!moving) dir = 2;
            else if(dir == 0) dir = 1;
            else dir = 3;
            moving = true;
        }
        else if(bn::keypad::right_held())
        {
            dfx += 1;
            facing_left = false;
            if(!moving) dir = 2;
            else if(dir == 0) dir = 1;
            else dir = 3;
            moving = true;
        }

        if(moving)
        {
            constexpr bn::fixed FLOOR_MIN = -55;
            constexpr bn::fixed FLOOR_MAX = 55;
            _player_fx = bn::min(bn::max(_player_fx + dfx, FLOOR_MIN), FLOOR_MAX);
            _player_fy = bn::min(bn::max(_player_fy + dfy, FLOOR_MIN), FLOOR_MAX);
            
            bn::fixed_point pos = _floor_to_screen(_player_fx, _player_fy, cam_dist);
            _player_sprite->set_position(pos);
        }

        _player_sprite->set_horizontal_flip(facing_left);
        _player_facing_left = facing_left;
        _set_player_anim(moving, dir);

        if(_player_anim.has_value())
        {
            _player_anim->update();
        }

        _text_sprites.clear();
        tg.set_center_alignment();
        tg.set_bg_priority(0);
        if(_debug_mode)
        {
            bn::string<32> line1;
            bn::ostringstream stream1(line1);
            stream1 << "F:" << _player_fx.integer() << "," << _player_fy.integer();
            tg.generate(0, -72, line1, _text_sprites);

            bn::string<32> line2;
            bn::ostringstream stream2(line2);
            stream2 << "C:" << _corner_index << " D:" << cam_dist.integer();
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
