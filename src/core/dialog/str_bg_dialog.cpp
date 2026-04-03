#include "str_bg_dialog.h"

#include "bn_regular_bg_tiles_items_dialog_font_tiles.h"
#include "bn_bg_palette_items_dialog_font_palette.h"
#include "bn_sprite_items_text_bg.h"

namespace str
{

// Static EWRAM allocation for BG map cells
alignas(int) BN_DATA_EWRAM bn::regular_bg_map_cell BgDialog::_cells[BgDialog::MAP_CELLS];
alignas(int) BN_DATA_EWRAM bn::regular_bg_map_cell BgDialog::_backdrop_cells[BgDialog::MAP_CELLS];

BgDialog::BgDialog() :
    _active(false),
    _prompt_visible(false),
    _state(STATE_IDLE),
    _current_line(0),
    _current_char(0),
    _hold_counter(0),
    _selected_option(0),
    _scroll_offset(0),
    _backdrop_visible(false)
{
    // Initialize all map cells to tile 0 (space = transparent)
    bn::memory::clear(_cells);

    // Build the BG item from our font tiles + palette + map
    bn::regular_bg_map_item map_item(_cells[0], bn::size(MAP_COLUMNS, MAP_ROWS));
    bn::regular_bg_item bg_item(
        bn::regular_bg_tiles_items::dialog_font_tiles,
        bn::bg_palette_items::dialog_font_palette,
        map_item);

    bool old_offset = bn::bg_tiles::allow_offset();
    bn::bg_tiles::set_allow_offset(false);
    _bg = bg_item.create_bg(0, 0);
    bn::bg_tiles::set_allow_offset(old_offset);

    _bg->set_priority(0);         // frontmost BG
    _bg->set_visible(false);      // hidden until dialog active
    _bg_map = _bg->map();

    auto configure_backdrop_sprite = [&](bn::sprite_ptr& sprite, bn::fixed horizontal_scale)
    {
        // Keep the backdrop behind dialog BG text but over room sprites.
        sprite.set_bg_priority(1);
        sprite.set_z_order(-32767);
        sprite.set_visible(false);

        if(horizontal_scale != 1)
        {
            sprite.set_horizontal_scale(horizontal_scale);
            sprite.set_vertical_scale(1);
            sprite.set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
        }
    };

    auto create_backdrop_set = [&](int segments, int start_x, int x_step, bn::fixed horizontal_scale) -> bool
    {
        _dialog_backdrop_sprites.clear();

        for(int index = 0; index < segments; ++index)
        {
            int backdrop_x = start_x + index * x_step;
            bn::optional<bn::sprite_ptr> backdrop_sprite =
                    bn::sprite_ptr::create_optional(backdrop_x, BACKDROP_Y, bn::sprite_items::text_bg);

            if(!backdrop_sprite)
            {
                _dialog_backdrop_sprites.clear();
                return false;
            }

            configure_backdrop_sprite(*backdrop_sprite, horizontal_scale);
            _dialog_backdrop_sprites.push_back(*backdrop_sprite);
        }

        return true;
    };

    // Try full fidelity first (3x64px). If VRAM is tight, degrade gracefully.
    if(! create_backdrop_set(3, -64, 64, 1))
    {
        if(! create_backdrop_set(2, -64, 128, 2))
        {
            create_backdrop_set(1, 0, 64, 1);
        }
    }

    if(_dialog_backdrop_sprites.empty())
    {
        _init_fallback_backdrop_bg();
    }
}

void BgDialog::set_greeting(bn::span<const bn::string_view> lines)
{
    _greeting_lines = lines;
}

void BgDialog::set_options(bn::span<const DialogOption> options)
{
    _options.clear();
    for(const DialogOption& opt : options)
    {
        _options.push_back(opt);
    }
}

void BgDialog::talk()
{
    _active = true;
    _state = STATE_GREETING;
    _current_lines = _greeting_lines;
    _current_line = 0;
    _current_char = 0;
    _hold_counter = 0;
    _selected_option = 0;
    _scroll_offset = 0;
    _clear_all_text();
    _bg->set_visible(true);
    hide_prompt();
    _refresh_backdrop_visibility();
}

void BgDialog::show_prompt()
{
    if(_prompt_visible || _active)
    {
        return;
    }
    _prompt_visible = true;
    _write_text_centered(PROMPT_ROW, "Press A to talk");
    _flush();
    _bg->set_visible(true);
    _refresh_backdrop_visibility();
}

void BgDialog::hide_prompt()
{
    if(!_prompt_visible)
    {
        return;
    }
    _prompt_visible = false;
    _clear_row(PROMPT_ROW);
    _flush();
    if(!_active)
    {
        _bg->set_visible(false);
    }
    _refresh_backdrop_visibility();
}

void BgDialog::update()
{
    if(!_active)
    {
        return;
    }

    if(_state == STATE_SHOWING_OPTIONS)
    {
        _handle_option_navigation();
        return;
    }

    // Typewriter text state (GREETING or SHOWING_RESPONSE)
    if(_current_line < _current_lines.size())
    {
        const bn::string_view& line = _current_lines[_current_line];
        int line_len = line.size();

        // Advance typewriter
        if(_current_char < line_len)
        {
            bool advance_held = bn::keypad::a_held() || bn::keypad::up_held();
            if(advance_held)
            {
                ++_hold_counter;
            }
            else
            {
                _hold_counter = 0;
            }

            // Speed up when held
            int speed = (_hold_counter >= 2) ? 3 : 1;
            _current_char += speed;
            if(_current_char > line_len)
            {
                _current_char = line_len;
            }

            // Write characters up to _current_char
            _clear_text_area();
            _write_wrapped_text_centered(TEXT_ROW, line.data(), line_len, _current_char);
            _flush();
        }
        else
        {
            // Line fully shown -- wait for A press to advance
            if(bn::keypad::a_pressed() || bn::keypad::up_pressed())
            {
                _current_line++;
                _current_char = 0;
                _hold_counter = 0;

                if(_current_line >= _current_lines.size())
                {
                    // Finished all lines
                    if(_state == STATE_GREETING && !_options.empty())
                    {
                        _state = STATE_SHOWING_OPTIONS;
                        _selected_option = 0;
                        _scroll_offset = 0;
                        _clear_text_area();
                        _render_options();
                    }
                    else
                    {
                        _end();
                    }
                }
            }
        }
    }

    // Start button ends dialog immediately
    if(_active && bn::keypad::start_pressed())
    {
        _end();
    }
}

void BgDialog::_end()
{
    _active = false;
    _state = STATE_IDLE;
    _current_line = 0;
    _current_char = 0;
    _hold_counter = 0;
    _clear_all_text();
    _bg->set_visible(false);
    _refresh_backdrop_visibility();
}

void BgDialog::_set_backdrop_visible(bool visible)
{
    if(_backdrop_visible == visible)
    {
        return;
    }

    _backdrop_visible = visible;
    for(bn::sprite_ptr& backdrop_sprite : _dialog_backdrop_sprites)
    {
        backdrop_sprite.set_visible(visible);
    }

    if(_backdrop_bg.has_value())
    {
        _backdrop_bg->set_visible(visible);
    }
}

void BgDialog::_refresh_backdrop_visibility()
{
    _set_backdrop_visible(_active || _prompt_visible);
}

void BgDialog::_init_fallback_backdrop_bg()
{
    bn::memory::clear(_backdrop_cells);

    for(int row = TEXT_AREA_TOP; row <= TEXT_AREA_BOTTOM; ++row)
    {
        for(int col = VISIBLE_COL_LEFT; col <= VISIBLE_COL_RIGHT; ++col)
        {
            int index = row * MAP_COLUMNS + col;
            bn::regular_bg_map_cell_info cell_info(_backdrop_cells[index]);
            cell_info.set_tile_index(1);
            cell_info.set_palette_id(0);
            cell_info.set_horizontal_flip(false);
            cell_info.set_vertical_flip(false);
            _backdrop_cells[index] = cell_info.cell();
        }
    }

    bn::regular_bg_map_item map_item(_backdrop_cells[0], bn::size(MAP_COLUMNS, MAP_ROWS));
    bn::regular_bg_item backdrop_bg_item(
        bn::regular_bg_tiles_item(
            bn::span<const bn::tile>(_fallback_backdrop_tiles, BACKDROP_FALLBACK_TILES_COUNT),
            bn::bpp_mode::BPP_4),
        bn::bg_palette_item(
            bn::span<const bn::color>(_fallback_backdrop_palette, BACKDROP_FALLBACK_PALETTE_COLORS_COUNT),
            bn::bpp_mode::BPP_4),
        map_item);

    bool old_offset = bn::bg_tiles::allow_offset();
    bn::bg_tiles::set_allow_offset(false);
    _backdrop_bg = backdrop_bg_item.create_bg_optional(0, 0);
    bn::bg_tiles::set_allow_offset(old_offset);

    if(_backdrop_bg.has_value())
    {
        _backdrop_bg->set_priority(1);
        _backdrop_bg->set_visible(false);
    }
}


void BgDialog::_render_options()
{
    _clear_text_area();

    int visible = _options.size() < VISIBLE_OPTIONS ?
                  _options.size() : VISIBLE_OPTIONS;
    int max_cols = VISIBLE_COL_RIGHT - VISIBLE_COL_LEFT + 1;
    int display_width = 0;

    for(int i = 0; i < visible; ++i)
    {
        int idx = _scroll_offset + i;
        if(idx >= _options.size())
        {
            break;
        }

        int line_len = _options[idx].option_text.size() + 2; // include selection prefix
        display_width = bn::max(display_width, bn::min(line_len, max_cols));
    }

    int block_start_col = VISIBLE_COL_LEFT + (max_cols - display_width) / 2;

    for(int i = 0; i < visible; ++i)
    {
        int idx = _scroll_offset + i;
        if(idx >= _options.size())
        {
            break;
        }

        int row = OPTIONS_ROW + i;
        bn::string<64> line_text;

        if(idx == _selected_option)
        {
            line_text.append("> ");
        }
        else
        {
            line_text.append("  ");
        }

        line_text.append(_options[idx].option_text);

        if(line_text.size() > max_cols)
        {
            bn::string<64> clipped_text;
            int keep = bn::max(0, max_cols - 3);
            clipped_text.append(bn::string_view(line_text.data(), keep));
            clipped_text.append("...");
            _write_text(row, block_start_col, clipped_text.data(), clipped_text.size());
        }
        else
        {
            _write_text(row, block_start_col, line_text.data(), line_text.size());
        }
    }
    _flush();
}

void BgDialog::_handle_option_navigation()
{
    bool changed = false;

    if(bn::keypad::down_pressed())
    {
        if(_selected_option < _options.size() - 1)
        {
            ++_selected_option;
            if(_selected_option >= _scroll_offset + VISIBLE_OPTIONS)
            {
                ++_scroll_offset;
            }
            changed = true;
        }
    }
    else if(bn::keypad::up_pressed())
    {
        if(_selected_option > 0)
        {
            --_selected_option;
            if(_selected_option < _scroll_offset)
            {
                --_scroll_offset;
            }
            changed = true;
        }
    }
    else if(bn::keypad::a_pressed())
    {
        _select_option(_selected_option);
        return;
    }
    else if(bn::keypad::start_pressed())
    {
        _end();
        return;
    }

    if(changed)
    {
        _render_options();
    }
}

void BgDialog::_select_option(int idx)
{
    const DialogOption& opt = _options[idx];

    if(opt.ends_conversation)
    {
        _end();
        return;
    }

    if(opt.response_lines.size() > 0)
    {
        _state = STATE_SHOWING_RESPONSE;
        _current_lines = opt.response_lines;
        _current_line = 0;
        _current_char = 0;
        _hold_counter = 0;
        _clear_text_area();
        _flush();
    }
    else
    {
        _end();
    }
}

} // namespace str
