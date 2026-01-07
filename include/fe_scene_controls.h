#ifndef FE_SCENE_CONTROLS_H
#define FE_SCENE_CONTROLS_H

#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "fe_scene.h"

namespace fe
{
    class Controls
    {
    public:
        Controls();
        ~Controls();

        fe::Scene execute();

    private:
        bn::vector<bn::sprite_ptr, 128> _text_sprites;

        void _update_display();
    };
}

#endif
