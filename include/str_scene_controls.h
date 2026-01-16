#ifndef STR_SCENE_CONTROLS_H
#define STR_SCENE_CONTROLS_H

#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "str_scene.h"

namespace str
{
    class Controls
    {
    public:
        Controls();
        ~Controls();

        str::Scene execute();

    private:
        bn::vector<bn::sprite_ptr, 128> _text_sprites;

        void _update_display();
    };
}

#endif
