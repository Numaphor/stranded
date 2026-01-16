#include "str_scene_menu.h"
#include "str_scene_start.h"
#include "str_scene_controls.h"
#include "str_constants.h"

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

    str::Scene Menu::execute(int &wid, bn::fixed_point &sl)
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
        const char *opts[] = {"Play Game", "Controls"};
        for (int i = 0; i < 2; ++i)
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
                _selected_index = !_selected_index;
            if (bn::keypad::down_pressed())
                _selected_index = !_selected_index;
            _update_display();
            if (bn::keypad::a_pressed())
                return _selected_index ? str::Scene::CONTROLS : str::Scene::MENU;
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
        const char *c[] = {"D-PAD: Move", "A: Interact/Confirm", "B: Attack/Back", "L: Switch Weapon", "R: Roll/Dodge", "SELECT+START: Debug", "SELECT+A: Level Select"};
        for (int i = 0; i < 7; ++i)
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

} // namespace str
