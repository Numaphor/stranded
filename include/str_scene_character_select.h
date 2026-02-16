#ifndef STR_SCENE_CHARACTER_SELECT_H
#define STR_SCENE_CHARACTER_SELECT_H

#include "bn_sprite_ptr.h"
#include "bn_sprite_builder.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"
#include "bn_optional.h"

#include "str_scene.h"
#include "str_constants.h"

namespace str
{

class CharacterSelect
{
public:
    CharacterSelect();
    
    Scene execute(int& selected_character_id);

private:
    void _init_characters();
    void _update_display();
    void _handle_input();
    
    bn::vector<bn::sprite_ptr, 64> _text_sprites;
    bn::vector<bn::sprite_ptr, 8> _character_sprites;
    int _selected_index = 0;
};

}

#endif