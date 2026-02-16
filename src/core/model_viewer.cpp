#include "str_scene_model_viewer.h"
#include "str_constants.h"

#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_bg_palettes.h"
#include "bn_color.h"
#include "bn_sprite_text_generator.h"
#include "bn_string.h"

#include "common_variable_8x8_sprite_font.h"

#include "fr_model_colors.h"
#include "str_model_viewer_items.h"

namespace str
{

namespace
{
    constexpr int total_items = str::viewer_item_count;
    constexpr int max_visible = 8;
    constexpr int menu_start_y = -50;
    constexpr int menu_spacing = 12;
}

ModelViewer::ModelViewer() : _selected_index(0) {}

void ModelViewer::_update_menu_display()
{
    _text_sprites.clear();
    bn::sprite_text_generator tg(common::variable_8x8_sprite_font);
    tg.set_center_alignment();
    tg.set_bg_priority(0);

    tg.generate(0, -70, "3D VIEWER", _text_sprites);

    // Calculate visible window
    int start = _selected_index - (max_visible / 2);
    if(start < 0) start = 0;
    if(start > total_items - max_visible) start = total_items - max_visible;

    for(int i = 0; i < max_visible && (start + i) < total_items; ++i)
    {
        int item_index = start + i;
        const fr::model_viewer_item& item = str::viewer_items[item_index];
        int y = menu_start_y + (i * menu_spacing);

        bn::string<48> line;
        if(item_index == _selected_index)
        {
            line = "> ";
            line.append(item.name());
            line += " <";
        }
        else
        {
            line = "  ";
            line.append(item.name());
        }
        tg.generate(0, y, line, _text_sprites);
    }

    // Scroll indicator
    bn::string<32> counter;
    counter.append(bn::to_string<4>(_selected_index + 1));
    counter.append("/");
    counter.append(bn::to_string<4>(total_items));
    tg.generate(0, 55, counter, _text_sprites);

    tg.generate(0, 70, "UP/DOWN: Select  A: View  B: Back", _text_sprites);
}

void ModelViewer::_update_viewer_hud()
{
    _text_sprites.clear();
    bn::sprite_text_generator tg(common::variable_8x8_sprite_font);
    tg.set_center_alignment();
    tg.set_bg_priority(0);

    const fr::model_viewer_item& item = str::viewer_items[_selected_index];
    tg.generate(0, -72, item.name(), _text_sprites);
    tg.generate(0, 72, "D-PAD:Rotate  L/R:Roll  B:Back", _text_sprites);
}

str::Scene ModelViewer::execute()
{
    bn::bg_palettes::set_transparent_color(bn::color(MENU_BG_COLOR_R, MENU_BG_COLOR_G, MENU_BG_COLOR_B));

    _models.load_colors(fr::default_model_colors);

    // Menu mode: select a model
    bool menu_dirty = true;
    while(true)
    {
        if(menu_dirty)
        {
            _update_menu_display();
            menu_dirty = false;
        }
        bn::core::update();

        if(bn::keypad::up_pressed())
        {
            if(_selected_index > 0)
            {
                --_selected_index;
                menu_dirty = true;
            }
        }
        else if(bn::keypad::down_pressed())
        {
            if(_selected_index < total_items - 1)
            {
                ++_selected_index;
                menu_dirty = true;
            }
        }
        else if(bn::keypad::b_pressed())
        {
            return str::Scene::START;
        }
        else if(bn::keypad::a_pressed())
        {
            // Enter viewer mode for the selected model
            _text_sprites.clear();
            bn::core::update();

            // Load per-model color palette
            if(str::has_custom_colors(_selected_index))
            {
                _models.load_colors(str::model_3d_items::blaster_model_colors);
            }
            else
            {
                _models.load_colors(fr::default_model_colors);
            }

            const fr::model_viewer_item& item = str::viewer_items[_selected_index];
            const fr::model_3d_item& model_item = item.model_item();

            // Calculate vertical centering (matching fr_model_viewer_scene.cpp _load_model, lines 240-258)
            const bn::span<const fr::vertex_3d>& verts = model_item.vertices();
            bn::fixed bottom_y = verts[0].point().y();
            bn::fixed top_y = bottom_y;
            for(const fr::vertex_3d& v : verts)
            {
                bn::fixed vy = v.point().y();
                if(vy < bottom_y) bottom_y = vy;
                else if(vy > top_y) top_y = vy;
            }
            bn::fixed model_z = (top_y - bn::abs(bottom_y)) / 2;

            fr::model_3d& model = _models.create_dynamic_model(model_item);
            model.set_position(fr::point_3d(0, item.y(), model_z));
            model.set_phi(item.initial_phi());
            model.set_theta(item.initial_theta());
            model.set_psi(item.initial_psi());

            _update_viewer_hud();

            // Viewer loop - must match varooom-3d frame order:
            // all work first (including _models.update), THEN bn::core::update()
            // (matching fr_model_viewer_scene.cpp update, line 170)
            bool viewing = true;
            while(viewing)
            {
                if(bn::keypad::b_pressed())
                {
                    viewing = false;
                }
                else
                {
                    if(bn::keypad::left_held())
                        model.set_phi(model.phi() - 256);
                    else if(bn::keypad::right_held())
                        model.set_phi(model.phi() + 256);

                    if(bn::keypad::up_held())
                        model.set_psi(model.psi() - 256);
                    else if(bn::keypad::down_held())
                        model.set_psi(model.psi() + 256);

                    if(bn::keypad::l_held())
                        model.set_theta(model.theta() - 256);
                    else if(bn::keypad::r_held())
                        model.set_theta(model.theta() + 256);
                }

                _models.update(_camera);
                bn::core::update();
            }

            _models.destroy_dynamic_model(model);
            _models.update(_camera);
            bn::core::update();
            menu_dirty = true;
        }
    }
}

}
