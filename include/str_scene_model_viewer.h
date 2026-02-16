#ifndef STR_SCENE_MODEL_VIEWER_H
#define STR_SCENE_MODEL_VIEWER_H

#include "bn_vector.h"
#include "bn_sprite_ptr.h"

#include "str_scene.h"
#include "fr_camera_3d.h"
#include "fr_models_3d.h"

namespace str
{

class ModelViewer
{
public:
    ModelViewer();
    str::Scene execute();

private:
    bn::vector<bn::sprite_ptr, 64> _text_sprites;
    int _selected_index;
    fr::camera_3d _camera;
    fr::models_3d _models;

    void _update_menu_display();
    void _update_viewer_hud();
};

}

#endif
