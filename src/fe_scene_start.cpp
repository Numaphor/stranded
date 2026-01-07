#include "fe_scene_start.h"
#include "fe_constants.h"
#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_sprite_text_generator.h"
#include "bn_string.h"
#include "common_variable_8x8_sprite_font.h"
#include "bn_bg_palettes.h"

namespace fe
{
    Start::Start() : _selected_index(0)
    {
    }

    Start::~Start()
    {
        // Sprites will be automatically cleaned up
    }

    void Start::_update_display()
    {
        _text_sprites.clear();

        // Create text generator
        bn::sprite_text_generator text_generator(common::variable_8x8_sprite_font);
        text_generator.set_center_alignment();
        text_generator.set_bg_priority(0);

        // Title
        text_generator.generate(0, START_TITLE_Y_POSITION, "STRANDED", _text_sprites);

        // Menu options
        const char* options[] = {"Play Game", "Controls"};
        for (int i = 0; i < 2; ++i)
        {
            int y_pos = START_OPTIONS_START_Y + (i * START_OPTIONS_SPACING);

            bn::string<64> line;
            if (i == _selected_index)
            {
                line = "> ";
                line += options[i];
                line += " <";
            }
            else
            {
                line = "  ";
                line += options[i];
            }

            text_generator.generate(0, y_pos, line, _text_sprites);
        }

        // Instructions
        text_generator.generate(0, START_INSTRUCTIONS_Y_POSITION, "UP/DOWN: Select  A: Confirm", _text_sprites);
    }

    void Start::_handle_input()
    {
        if (bn::keypad::up_pressed())
        {
            _selected_index = (_selected_index > 0) ? _selected_index - 1 : 1;
        }

        if (bn::keypad::down_pressed())
        {
            _selected_index = (_selected_index < 1) ? _selected_index + 1 : 0;
        }
    }

    fe::Scene Start::execute()
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
                if (_selected_index == 0)
                {
                    // Play Game -> Go to level selector (MENU)
                    return fe::Scene::MENU;
                }
                else if (_selected_index == 1)
                {
                    // Controls -> Show controls screen
                    return fe::Scene::CONTROLS;
                }
            }
        }
    }
}
