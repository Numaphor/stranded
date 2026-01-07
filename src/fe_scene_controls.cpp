#include "fe_scene_controls.h"
#include "fe_constants.h"
#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_sprite_text_generator.h"
#include "bn_string.h"
#include "common_variable_8x8_sprite_font.h"
#include "bn_bg_palettes.h"

namespace fe
{
    Controls::Controls()
    {
    }

    Controls::~Controls()
    {
        // Sprites will be automatically cleaned up
    }

    void Controls::_update_display()
    {
        _text_sprites.clear();

        // Create text generator
        bn::sprite_text_generator text_generator(common::variable_8x8_sprite_font);
        text_generator.set_center_alignment();
        text_generator.set_bg_priority(0);

        // Title
        text_generator.generate(0, CONTROLS_TITLE_Y_POSITION, "CONTROLS", _text_sprites);

        // Controls list
        const char* controls[] = {
            "D-PAD: Move",
            "A: Interact/Confirm",
            "B: Attack/Back",
            "L: Switch Weapon",
            "R: Roll/Dodge",
            "SELECT+START: Debug",
            "SELECT+A: Level Select"
        };

        int y_pos = CONTROLS_LIST_START_Y;
        for (const char* control : controls)
        {
            text_generator.generate(0, y_pos, control, _text_sprites);
            y_pos += CONTROLS_LIST_SPACING;
        }

        // Instructions at bottom
        text_generator.generate(0, CONTROLS_INSTRUCTIONS_Y_POSITION, "Press B to return", _text_sprites);
    }

    fe::Scene Controls::execute()
    {
        // Set a simple background color
        bn::bg_palettes::set_transparent_color(bn::color(MENU_BG_COLOR_R, MENU_BG_COLOR_G, MENU_BG_COLOR_B));

        _update_display();

        while (true)
        {
            bn::core::update();

            // Handle back button
            if (bn::keypad::b_pressed())
            {
                // Return to start screen
                return fe::Scene::START;
            }
        }
    }
}
