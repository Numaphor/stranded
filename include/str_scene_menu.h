#ifndef STR_SCENE_MENU_H
#define STR_SCENE_MENU_H

#include "bn_fixed_point.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"
#include "bn_sprite_text_generator.h"
#include "bn_string.h"

#include "str_scene.h"

namespace str
{
    // World data structure to hold world state
    struct WorldData
    {
        int world_id;
        bn::string<32> world_name;
        bn::fixed_point spawn_location;
        bool is_unlocked;
        // Additional state data can be added here
        // e.g., enemies defeated, items collected, etc.

        WorldData() : world_id(0), world_name(""), spawn_location(50, 100), is_unlocked(true) {}
        WorldData(int id, const bn::string<32> &name, bn::fixed_point spawn, bool unlocked = true)
            : world_id(id), world_name(name), spawn_location(spawn), is_unlocked(unlocked) {}
    };

    class Menu
    {
    public:
        Menu();
        ~Menu();

        str::Scene execute(int &selected_world_id, bn::fixed_point &spawn_location);

    private:
        bn::vector<WorldData, 8> _worlds;
        int _selected_index;
        bn::vector<bn::sprite_ptr, 32> _text_sprites;

        void _init_worlds();
        void _update_display();
        void _handle_input();
    };
}

#endif