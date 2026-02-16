#include "str_scene_menu.h"
#include "str_scene_start.h"
#include "str_scene_controls.h"
#include "str_scene_character_select.h"
#include "str_constants.h"

#include "bn_sprite_items_hero.h"
#include "bn_sprite_items_soldier.h"
#include "bn_sprite_items_cursor.h"

#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_bg_palettes.h"
#include "bn_color.h"
#include "bn_sprite_text_generator.h"
#include "bn_string.h"

#include "common_variable_8x8_sprite_font.h"

namespace str
{

    // =========================================================================
    // Menu Implementation
    // =========================================================================

    Menu::Menu() : _selected_index(0) { _init_worlds(); }
    Menu::~Menu() {}

    void Menu::_init_worlds()
    {
        _worlds.clear();
        _worlds.push_back({0, "Main World", bn::fixed_point(MAIN_WORLD_SPAWN_X, MAIN_WORLD_SPAWN_Y), true});
        _worlds.push_back({1, "Forest Area", bn::fixed_point(FOREST_WORLD_SPAWN_X, FOREST_WORLD_SPAWN_Y), true});
    }

    void Menu::_update_display()
    {
        _text_sprites.clear();
        bn::sprite_text_generator tg(common::variable_8x8_sprite_font);
        tg.set_center_alignment();
        tg.set_bg_priority(0);
        tg.generate(0, MENU_TITLE_Y_POSITION, "WORLD SELECTION", _text_sprites);
        tg.generate(0, MENU_INSTRUCTIONS_Y_POSITION, "UP/DOWN: Select  A: Enter  B: Exit", _text_sprites);
        for (int i = 0; i < _worlds.size(); ++i)
        {
            int y = MENU_WORLD_LIST_START_Y + (i * MENU_WORLD_LIST_SPACING);
            if (!_worlds[i].is_unlocked)
                tg.generate(0, y, "??? LOCKED ???", _text_sprites);
            else
            {
                bn::string<64> l = (i == _selected_index ? "> " : "  ");
                l += _worlds[i].world_name;
                if (i == _selected_index)
                    l += " <";
                tg.generate(0, y, l, _text_sprites);
            }
        }
    }

    void Menu::_handle_input()
    {
        auto move = [&](int d)
        { _selected_index = (_selected_index + d + _worlds.size()) % _worlds.size(); while (!_worlds[_selected_index].is_unlocked) _selected_index = (_selected_index + d + _worlds.size()) % _worlds.size(); };
        if (bn::keypad::up_pressed())
            move(-1);
        if (bn::keypad::down_pressed())
            move(1);
    }

    str::Scene Menu::execute(int &wid, bn::fixed_point &sl, int &/*selected_character_id*/)
    {
        bn::bg_palettes::set_transparent_color(bn::color(MENU_BG_COLOR_R, MENU_BG_COLOR_G, MENU_BG_COLOR_B));
        while (1)
        {
            bn::core::update();
            _handle_input();
            _update_display();
            if (bn::keypad::a_pressed() && _worlds[_selected_index].is_unlocked)
            {
                wid = _worlds[_selected_index].world_id;
                sl = _worlds[_selected_index].spawn_location;
                // Pass through character_id (character was already selected)
                return str::Scene::WORLD;
            }
            if (bn::keypad::b_pressed())
                return str::Scene::START;
        }
    }

    // =========================================================================
    // Start Implementation
    // =========================================================================

    Start::Start() : _selected_index(0) {}
    Start::~Start() {}

    void Start::_update_display()
    {
        _text_sprites.clear();
        bn::sprite_text_generator tg(common::variable_8x8_sprite_font);
        tg.set_center_alignment();
        tg.set_bg_priority(0);
        tg.generate(0, START_TITLE_Y_POSITION, "STRANDED", _text_sprites);
        const char *opts[] = {"Play Game", "Controls", "3D Viewer"};
        for (int i = 0; i < 3; ++i)
        {
            bn::string<64> l = (i == _selected_index ? "> " : "  ");
            l += opts[i];
            if (i == _selected_index)
                l += " <";
            tg.generate(0, START_OPTIONS_START_Y + i * START_OPTIONS_SPACING, l, _text_sprites);
        }
        tg.generate(0, START_INSTRUCTIONS_Y_POSITION, "UP/DOWN: Select  A: Confirm", _text_sprites);
    }

    str::Scene Start::execute()
    {
        bn::bg_palettes::set_transparent_color(bn::color(MENU_BG_COLOR_R, MENU_BG_COLOR_G, MENU_BG_COLOR_B));
        while (1)
        {
            bn::core::update();
            if (bn::keypad::up_pressed())
                _selected_index = (_selected_index + 2) % 3;
            if (bn::keypad::down_pressed())
                _selected_index = (_selected_index + 1) % 3;
            _update_display();
            if (bn::keypad::a_pressed())
            {
                if (_selected_index == 0) return str::Scene::CHARACTER_SELECT;
                if (_selected_index == 1) return str::Scene::CONTROLS;
                return str::Scene::MODEL_VIEWER;
            }
        }
    }

    // =========================================================================
    // Controls Implementation
    // =========================================================================

    Controls::Controls() {}
    Controls::~Controls() {}

    void Controls::_update_display()
    {
        _text_sprites.clear();
        bn::sprite_text_generator tg(common::variable_8x8_sprite_font);
        tg.set_center_alignment();
        tg.set_bg_priority(0);
        tg.generate(0, CONTROLS_TITLE_Y_POSITION, "CONTROLS", _text_sprites);
        const char *c[] = {
            "D-PAD: Move",
            "A: Attack/Interact",
            "B: Roll (uses energy)",
            "R: Switch to Sword/Gun",
            "R hold: Reload",
            "L: Buff Menu",
            "L hold: Gun Select",
            "SELECT: Toggle Zoom",
            "SEL+A: Level Select",
            "SEL+Dir: Quick Buff",
            "A+Move: Strafe (Gun)"
        };
        for (int i = 0; i < 11; ++i)
            tg.generate(0, CONTROLS_LIST_START_Y + i * CONTROLS_LIST_SPACING, c[i], _text_sprites);
        tg.generate(0, CONTROLS_INSTRUCTIONS_Y_POSITION, "Press B to return", _text_sprites);
    }

    str::Scene Controls::execute()
    {
        bn::bg_palettes::set_transparent_color(bn::color(MENU_BG_COLOR_R, MENU_BG_COLOR_G, MENU_BG_COLOR_B));
        _update_display();
        while (1)
        {
            bn::core::update();
            if (bn::keypad::b_pressed())
                return str::Scene::START;
        }
    }

    // =========================================================================
    // Character Select Implementation
    // =========================================================================

    CharacterSelect::CharacterSelect() : _selected_index(0) 
    { 
        _init_characters(); 
    }


    void CharacterSelect::_init_characters()
    {
        _character_sprites.clear();
        // Create character preview sprites
        bn::sprite_builder hero_builder(bn::sprite_items::hero);
        hero_builder.set_position(-60, 0);
        hero_builder.set_bg_priority(1);
        _character_sprites.push_back(hero_builder.release_build());

        bn::sprite_builder soldier_builder(bn::sprite_items::soldier);
        soldier_builder.set_position(60, 0);
        soldier_builder.set_bg_priority(1);
        _character_sprites.push_back(soldier_builder.release_build());

        // Create cursor sprite (but don't use it for now to avoid issues)
        // _cursor_sprite.reset();
    }

    void CharacterSelect::_update_display()
    {
        _text_sprites.clear();
        bn::sprite_text_generator tg(common::variable_8x8_sprite_font);
        tg.set_center_alignment();
        tg.set_bg_priority(0);
        
        tg.generate(0, MENU_TITLE_Y_POSITION, "SELECT CHARACTER", _text_sprites);
        
        // Character descriptions
        tg.generate(-60, 50, "HERO", _text_sprites);
        tg.generate(-60, 70, "Balanced fighter", _text_sprites);
        tg.generate(-60, 85, "Sword & Gun", _text_sprites);
        
        tg.generate(60, 50, "SOLDIER", _text_sprites);
        tg.generate(60, 70, "Ranged specialist", _text_sprites);
        tg.generate(60, 85, "Heavy firepower", _text_sprites);
        
        // Selection indicator - show selection
        if (_selected_index == 0) {
            tg.generate(-60, -40, ">>> SELECT <<<", _text_sprites);
        } else {
            tg.generate(60, -40, ">>> SELECT <<<", _text_sprites);
        }
        
        tg.generate(0, MENU_INSTRUCTIONS_Y_POSITION, "LEFT/RIGHT: Select  A: Confirm  B: Back", _text_sprites);
    }

    void CharacterSelect::_handle_input()
    {
        if (bn::keypad::left_pressed())
            _selected_index = 0;
        if (bn::keypad::right_pressed())
            _selected_index = 1;
    }

    str::Scene CharacterSelect::execute(int& selected_character_id)
    {
        bn::bg_palettes::set_transparent_color(bn::color(MENU_BG_COLOR_R, MENU_BG_COLOR_G, MENU_BG_COLOR_B));
        
        while (1)
        {
            bn::core::update();
            _handle_input();
            _update_display();
            
            if (bn::keypad::a_pressed())
            {
                selected_character_id = _selected_index;
                return str::Scene::MENU;
            }
            if (bn::keypad::b_pressed())
                return str::Scene::START;
        }
    }

} // namespace str
