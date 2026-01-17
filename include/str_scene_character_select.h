#ifndef STR_SCENE_CHARACTER_SELECT_H
#define STR_SCENE_CHARACTER_SELECT_H

#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "str_scene.h"

namespace str
{
    class CharacterSelect
    {
    public:
        CharacterSelect();
        ~CharacterSelect();

        str::Scene execute(CharacterType &selected_character);

    private:
        int _selected_index;
        bn::vector<bn::sprite_ptr, 32> _text_sprites;

        void _update_display();
        void _handle_input();
    };
}

#endif
