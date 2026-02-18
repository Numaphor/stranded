#ifndef STR_SCENE_ROOM_VIEWER_H
#define STR_SCENE_ROOM_VIEWER_H

#include "bn_vector.h"
#include "bn_sprite_ptr.h"
#include "bn_optional.h"
#include "bn_sprite_animate_actions.h"
#include "bn_fixed_point.h"

#include "str_scene.h"
#include "fr_camera_3d.h"
#include "fr_models_3d.h"

namespace str
{

class RoomViewer
{
public:
    RoomViewer();
    str::Scene execute();

private:
    bn::vector<bn::sprite_ptr, 32> _text_sprites;
    fr::camera_3d _camera;
    fr::models_3d _models;

    // Player sprite
    bn::optional<bn::sprite_ptr> _player_sprite;
    bn::optional<bn::sprite_animate_action<4>> _player_anim;
    int _corner_index = 0;
    int _player_dir = 0;    // 0=down, 1=down_side, 2=side, 3=up_side, 4=up
    bool _player_moving = false;
    bool _player_facing_left = false;
    bool _debug_mode = false;

    // Player floor-space position (model coordinates on the z=0 floor plane)
    bn::fixed _player_fx = 0;
    bn::fixed _player_fy = 0;

    // R_iso rotation matrix (first two columns, for floor z=0 plane)
    bn::fixed _r00, _r01, _r10, _r11, _r20, _r21;

    void _update_hud();
    void _set_player_anim(bool moving, int dir);
    void _rotate_player_dir();
    bn::fixed_point _floor_to_screen(bn::fixed fx, bn::fixed fy, bn::fixed cam_y, int corner);
};

}

#endif
