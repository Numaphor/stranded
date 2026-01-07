#ifndef FE_SCENE_START_H
#define FE_SCENE_START_H

#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "fe_scene.h"

namespace fe
{
    class Start
    {
    public:
        Start();
        ~Start();

        fe::Scene execute();

    private:
        int _selected_index;
        bn::vector<bn::sprite_ptr, 32> _text_sprites;

        void _update_display();
        void _handle_input();
    };
}

#endif
