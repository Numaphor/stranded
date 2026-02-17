#include "str_scene_room_viewer.h"

#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_bg_palettes.h"
#include "bn_color.h"
#include "bn_sprite_text_generator.h"
#include "bn_fixed.h"
#include "bn_math.h"
#include "bn_string.h"

#include "common_variable_8x8_sprite_font.h"

#include "models/str_model_3d_items_room.h"

namespace str
{

RoomViewer::RoomViewer() {}

void RoomViewer::_update_hud()
{
    _text_sprites.clear();
    bn::sprite_text_generator tg(common::variable_8x8_sprite_font);
    tg.set_center_alignment();
    tg.set_bg_priority(0);

    tg.generate(0, -72, "3D ROOM", _text_sprites);
    tg.generate(0, 72, "D-PAD:Orbit  L/R:Zoom  B:Back", _text_sprites);
}

str::Scene RoomViewer::execute()
{
    // Dark background for the void outside the room
    bn::bg_palettes::set_transparent_color(bn::color(2, 2, 4));

    _models.load_colors(str::model_3d_items::room_model_colors);

    fr::model_3d& room = _models.create_dynamic_model(str::model_3d_items::room);

    // Isometric view: all 3 Euler angles needed for symmetric diamond
    // phi=Rz(35°) rotates XY, theta=Ry(-31°) screen rotation, psi=Rx(35°) forward tilt
    bn::fixed room_y = 96;
    bn::fixed room_z = 16;
    room.set_position(fr::point_3d(0, room_y, room_z));

    bn::fixed model_phi = 6400;     // 35.16° Rz (25x256)
    bn::fixed model_theta = 59904;  // -31° Ry screen-plane diamond (234x256)
    bn::fixed model_psi = 6400;     // 35.16° Rx forward tilt (25x256)
    room.set_phi(model_phi);
    room.set_theta(model_theta);
    room.set_psi(model_psi);

    bn::fixed cam_y = 274;

    // Debug mode: SELECT toggles which axis D-PAD controls
    // 0 = phi (Left/Right) + psi (Up/Down)
    // 1 = theta (Left/Right) + cam_y (Up/Down)
    // 2 = room_y (Up/Down) + room_z (Left/Right)
    int mode = 0;

    bn::sprite_text_generator tg(common::variable_8x8_sprite_font);

    while(true)
    {
        if(bn::keypad::b_pressed())
        {
            _models.destroy_dynamic_model(room);
            _models.update(_camera);
            bn::core::update();
            return str::Scene::START;
        }

        // SELECT cycles mode
        if(bn::keypad::select_pressed())
        {
            mode = (mode + 1) % 3;
        }

        if(mode == 0)
        {
            // D-PAD: phi (L/R) + psi (U/D)
            if(bn::keypad::left_held()) model_phi -= 256;
            else if(bn::keypad::right_held()) model_phi += 256;
            if(bn::keypad::up_held()) model_psi -= 128;
            else if(bn::keypad::down_held()) model_psi += 128;
        }
        else if(mode == 1)
        {
            // D-PAD: theta (L/R) + cam_y (U/D)
            if(bn::keypad::left_held()) model_theta -= 256;
            else if(bn::keypad::right_held()) model_theta += 256;
            if(bn::keypad::up_held()) cam_y -= 3;
            else if(bn::keypad::down_held()) cam_y += 3;
            cam_y = bn::max(bn::fixed(100), bn::min(bn::fixed(500), cam_y));
        }
        else
        {
            // D-PAD: room_z (L/R) + room_y (U/D)
            if(bn::keypad::left_held()) room_z -= 1;
            else if(bn::keypad::right_held()) room_z += 1;
            if(bn::keypad::up_held()) room_y -= 2;
            else if(bn::keypad::down_held()) room_y += 2;
        }

        // L/R always zoom
        if(bn::keypad::l_held()) cam_y = bn::max(bn::fixed(100), cam_y - 3);
        else if(bn::keypad::r_held()) cam_y = bn::min(bn::fixed(500), cam_y + 3);

        // Wrap angles
        if(model_phi > 0xFFFF) model_phi -= 0xFFFF;
        if(model_phi < 0) model_phi += 0xFFFF;
        if(model_theta > 0xFFFF) model_theta -= 0xFFFF;
        if(model_theta < 0) model_theta += 0xFFFF;
        if(model_psi > 0xFFFF) model_psi -= 0xFFFF;
        if(model_psi < 0) model_psi += 0xFFFF;

        room.set_phi(model_phi);
        room.set_theta(model_theta);
        room.set_psi(model_psi);
        room.set_position(fr::point_3d(0, room_y, room_z));

        _camera.set_position(fr::point_3d(0, cam_y, 0));

        // HUD with live values
        _text_sprites.clear();
        tg.set_center_alignment();
        tg.set_bg_priority(0);

        const char* mode_names[] = {"PHI/PSI", "THETA/CAMY", "ROOMYZ"};
        bn::string<32> mode_str = "SEL:Mode [";
        mode_str += mode_names[mode];
        mode_str += "]";
        tg.generate(0, -72, mode_str, _text_sprites);

        bn::string<32> line1 = "P:";
        line1 += bn::to_string<8>(model_phi.right_shift_integer());
        line1 += " T:";
        line1 += bn::to_string<8>(model_theta.right_shift_integer());
        line1 += " S:";
        line1 += bn::to_string<8>(model_psi.right_shift_integer());
        tg.generate(0, -60, line1, _text_sprites);

        bn::string<32> line2 = "CY:";
        line2 += bn::to_string<8>(cam_y.right_shift_integer());
        line2 += " RY:";
        line2 += bn::to_string<8>(room_y.right_shift_integer());
        line2 += " RZ:";
        line2 += bn::to_string<8>(room_z.right_shift_integer());
        tg.generate(0, -48, line2, _text_sprites);

        tg.generate(0, 72, "D-PAD:Adjust L/R:Zoom B:Exit", _text_sprites);

        _models.update(_camera);
        bn::core::update();
    }
}

}
