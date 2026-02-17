#ifndef STR_SCENE_ROOM_VIEWER_H
#define STR_SCENE_ROOM_VIEWER_H

#include "bn_vector.h"
#include "bn_sprite_ptr.h"

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

    void _update_hud();
};

}

#endif
