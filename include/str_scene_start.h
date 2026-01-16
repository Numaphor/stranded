#ifndef STR_SCENE_START_H
#define STR_SCENE_START_H

#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "str_scene.h"

namespace str
{
    class Start
    {
    public:
        Start();
        ~Start();

        str::Scene execute();

    private:
        int _selected_index;
        bn::vector<bn::sprite_ptr, 32> _text_sprites;

        void _update_display();
        void _handle_input();
    };
}

#endif
