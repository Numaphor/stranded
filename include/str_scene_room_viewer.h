#ifndef STR_SCENE_ROOM_VIEWER_H
#define STR_SCENE_ROOM_VIEWER_H

#include "bn_vector.h"
#include "bn_sprite_ptr.h"
#include "bn_optional.h"
#include "bn_fixed_point.h"

#include "str_scene.h"
#include "fr_camera_3d.h"
#include "fr_model_3d.h"
#include "fr_models_3d.h"
#include "fr_sprite_3d.h"
#include "fr_sprite_3d_item.h"

namespace str
{

class RoomViewer
{
public:
    RoomViewer();
    str::Scene execute();

private:
    bn::vector<bn::sprite_ptr, 64> _text_sprites;
    fr::camera_3d _camera;
    fr::models_3d _models;

    int _corner_index = 0;
    int _player_dir = 0;    // 0=down, 1=down_side, 2=side, 3=up_side, 4=up
    bool _player_moving = false;
    bool _player_facing_left = false;
    bool _debug_mode = false;
    int _anim_frame_counter = 0;

    bn::fixed _player_fx = 0;
    bn::fixed _player_fy = 0;
    bn::fixed _player_fz = 0;

    void _update_player_anim_tiles(fr::sprite_3d_item& item, bool moving, int dir, int elapsed_frames);
    void _rotate_player_dir(fr::sprite_3d& sprite);
};

}

#endif
