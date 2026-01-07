#include "fe_scene_menu.h"
#include "fe_constants.h"
#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_sprite_text_generator.h"
#include "bn_string.h"
#include "common_variable_8x8_sprite_font.h"
#include "bn_bg_palettes.h"

namespace fe
{
    Menu::Menu() : _selected_index(0)
    {
        _init_worlds();
    }

    Menu::~Menu()
    {
        // Sprites will be automatically cleaned up
    }

    void Menu::_init_worlds()
    {
        // Initialize available worlds
        _worlds.clear();

        _worlds.push_back({0, "Main World", bn::fixed_point(MAIN_WORLD_SPAWN_X, MAIN_WORLD_SPAWN_Y), true});
        _worlds.push_back({1, "Forest Area", bn::fixed_point(FOREST_WORLD_SPAWN_X, FOREST_WORLD_SPAWN_Y), true});
    }

    void Menu::_update_display()
    {
        _text_sprites.clear();

        // Create text generator
        bn::sprite_text_generator text_generator(common::variable_8x8_sprite_font);
        text_generator.set_center_alignment();
        text_generator.set_bg_priority(0);

        // Title
        text_generator.generate(0, MENU_TITLE_Y_POSITION, "WORLD SELECTION", _text_sprites);

        // Instructions
        text_generator.generate(0, MENU_INSTRUCTIONS_Y_POSITION, "UP/DOWN: Select  A: Enter  B: Exit", _text_sprites);

        // World list
        for (int i = 0; i < _worlds.size(); ++i)
        {
            int y_pos = MENU_WORLD_LIST_START_Y + (i * MENU_WORLD_LIST_SPACING);

            if (!_worlds[i].is_unlocked)
            {
                // Show locked worlds differently
                text_generator.generate(0, y_pos, "??? LOCKED ???", _text_sprites);
            }
            else
            {
                // Show selection indicator
                bn::string<64> line;
                if (i == _selected_index)
                {
                    line = "> ";
                    line += _worlds[i].world_name;
                    line += " <";
                }
                else
                {
                    line = "  ";
                    line += _worlds[i].world_name;
                }

                text_generator.generate(0, y_pos, line, _text_sprites);
            }
        }
    }

    void Menu::_handle_input()
    {
        if (bn::keypad::up_pressed())
        {
            if (_selected_index > 0)
            {
                _selected_index--;
                // Skip locked worlds
                while (_selected_index >= 0 && !_worlds[_selected_index].is_unlocked)
                {
                    _selected_index--;
                }
                if (_selected_index < 0)
                {
                    // Wrap to last unlocked world
                    _selected_index = _worlds.size() - 1;
                    while (_selected_index >= 0 && !_worlds[_selected_index].is_unlocked)
                    {
                        _selected_index--;
                    }
                }
            }
            else
            {
                // Wrap to last unlocked world
                _selected_index = _worlds.size() - 1;
                while (_selected_index >= 0 && !_worlds[_selected_index].is_unlocked)
                {
                    _selected_index--;
                }
            }
        }

        if (bn::keypad::down_pressed())
        {
            if (_selected_index < _worlds.size() - 1)
            {
                _selected_index++;
                // Skip locked worlds
                while (_selected_index < _worlds.size() && !_worlds[_selected_index].is_unlocked)
                {
                    _selected_index++;
                }
                if (_selected_index >= _worlds.size())
                {
                    // Wrap to first unlocked world
                    _selected_index = 0;
                    while (_selected_index < _worlds.size() && !_worlds[_selected_index].is_unlocked)
                    {
                        _selected_index++;
                    }
                }
            }
            else
            {
                // Wrap to first unlocked world
                _selected_index = 0;
                while (_selected_index < _worlds.size() && !_worlds[_selected_index].is_unlocked)
                {
                    _selected_index++;
                }
            }
        }
    }

    fe::Scene Menu::execute(int &selected_world_id, bn::fixed_point &spawn_location)
    {
        // Set a simple background color
        bn::bg_palettes::set_transparent_color(bn::color(MENU_BG_COLOR_R, MENU_BG_COLOR_G, MENU_BG_COLOR_B));

        while (true)
        {
            bn::core::update();

            _handle_input();
            _update_display();

            // Handle selection
            if (bn::keypad::a_pressed())
            {
                if (_selected_index >= 0 && _selected_index < _worlds.size() &&
                    _worlds[_selected_index].is_unlocked)
                {
                    selected_world_id = _worlds[_selected_index].world_id;
                    spawn_location = _worlds[_selected_index].spawn_location;
                    return fe::Scene::WORLD;
                }
            }

            // Exit menu - go back to start screen
            if (bn::keypad::b_pressed())
            {
                return fe::Scene::START;
            }
        }
    }
}