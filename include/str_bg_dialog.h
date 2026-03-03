#ifndef STR_BG_DIALOG_H
#define STR_BG_DIALOG_H

#include <bn_algorithm.h>
#include <bn_bg_tiles.h>
#include <bn_memory.h>
#include <bn_regular_bg_item.h>
#include <bn_regular_bg_ptr.h>
#include <bn_regular_bg_map_item.h>
#include <bn_regular_bg_map_ptr.h>
#include <bn_regular_bg_map_cell.h>
#include <bn_regular_bg_map_cell_info.h>
#include <bn_string.h>
#include <bn_string_view.h>
#include <bn_span.h>
#include <bn_vector.h>
#include <bn_keypad.h>
#include <bn_optional.h>
#include <bn_sprite_ptr.h>
#include <bn_sprite_double_size_mode.h>

#include "bn_regular_bg_tiles_items_dialog_font_tiles.h"
#include "bn_bg_palette_items_dialog_font_palette.h"
#include "bn_sprite_items_text_bg.h"

namespace str
{

// Dialog system that renders text onto a BG layer using a bitmap font tileset.
// It also uses a sprite backdrop strip for improved readability.
// Follows the same state machine as the legacy sprite-based RoomDialog.
class BgDialog
{
public:
    struct DialogOption
    {
        bn::string_view option_text;
        bn::span<const bn::string_view> response_lines;
        bool ends_conversation;
    };

    BgDialog() :
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

    void set_greeting(bn::span<const bn::string_view> lines)
    {
        _greeting_lines = lines;
    }

    void set_options(bn::span<const DialogOption> options)
    {
        _options.clear();
        for(const DialogOption& opt : options)
        {
            _options.push_back(opt);
        }
    }

    void talk()
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

    [[nodiscard]] bool is_active() const
    {
        return _active;
    }

    // Show "Press A to talk" prompt at bottom of screen
    void show_prompt()
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

    void hide_prompt()
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
    // Call once per frame while active
    void update()
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

private:
    // Map dimensions (32x32 standard BG)
    static constexpr int MAP_COLUMNS = 32;
    static constexpr int MAP_ROWS = 32;
    static constexpr int MAP_CELLS = MAP_COLUMNS * MAP_ROWS;

    // Screen-to-map: BG centered means screen(0,0) = map(128,128) = tile(16,16)
    // Screen top-left (-120,-80) = map(8,48) = tile(1,6)
    // Visible area: tile cols 1-30, tile rows 6-25

    // Dialog text position (screen y ~= +32..+48)
    // screen y=32 → map_y = 32+128 = 160 → row 20
    static constexpr int TEXT_ROW = 24;

    // Options start a bit higher (screen y ~= +16)
    // screen y=16 → map_y = 16+128 = 144 → row 18
    static constexpr int OPTIONS_ROW = 22;

    // "Press A" prompt (screen y ~= +48)
    static constexpr int PROMPT_ROW = 24;

    static constexpr int VISIBLE_COL_LEFT = 1;
    static constexpr int VISIBLE_COL_RIGHT = 30;

    static constexpr int VISIBLE_OPTIONS = 3;
    static constexpr int BACKDROP_SEGMENTS = 3;
    static constexpr int BACKDROP_Y = 64;
    static constexpr int BACKDROP_FALLBACK_TILES_COUNT = 2;
    static constexpr int BACKDROP_FALLBACK_PALETTE_COLORS_COUNT = 16;

    // Text area rows to clear (rows 18-21 cover dialog + options area)
    static constexpr int TEXT_AREA_TOP = 22;
    static constexpr int TEXT_AREA_BOTTOM = 25;

    static constexpr bn::tile _fallback_backdrop_tiles[BACKDROP_FALLBACK_TILES_COUNT] = {
        { { 0, 0, 0, 0, 0, 0, 0, 0 } },
        { { 0x11111111, 0x11111111, 0x11111111, 0x11111111,
            0x11111111, 0x11111111, 0x11111111, 0x11111111 } }
    };

    static constexpr bn::color _fallback_backdrop_palette[BACKDROP_FALLBACK_PALETTE_COLORS_COUNT] = {
        bn::color(0, 0, 0),   // index 0 (transparent)
        bn::color(2, 2, 4),   // index 1 (dialog strip)
        bn::color(0, 0, 0),   // unused
        bn::color(0, 0, 0),   // unused
        bn::color(0, 0, 0),   // unused
        bn::color(0, 0, 0),   // unused
        bn::color(0, 0, 0),   // unused
        bn::color(0, 0, 0),   // unused
        bn::color(0, 0, 0),   // unused
        bn::color(0, 0, 0),   // unused
        bn::color(0, 0, 0),   // unused
        bn::color(0, 0, 0),   // unused
        bn::color(0, 0, 0),   // unused
        bn::color(0, 0, 0),   // unused
        bn::color(0, 0, 0),   // unused
        bn::color(0, 0, 0)    // unused
    };

    enum State
    {
        STATE_IDLE,
        STATE_GREETING,
        STATE_SHOWING_OPTIONS,
        STATE_SHOWING_RESPONSE
    };

    void _end()
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

    void _set_backdrop_visible(bool visible)
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

    void _refresh_backdrop_visibility()
    {
        _set_backdrop_visible(_active || _prompt_visible);
    }

    void _init_fallback_backdrop_bg()
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

    // Write ASCII text at map position
    void _write_text(int row, int col, const char* text, int len)
    {
        for(int i = 0; i < len; ++i)
        {
            int target_col = col + i;
            if(target_col < VISIBLE_COL_LEFT)
            {
                continue;
            }
            if(target_col > VISIBLE_COL_RIGHT)
            {
                break;
            }

            int ch = static_cast<unsigned char>(text[i]);
            int tile_index = 0;
            if(ch >= 32 && ch <= 127)
            {
                tile_index = ch - 32;
            }
            _set_cell(target_col, row, tile_index);
        }
    }

    // Write text centered on a row.
    void _write_text_centered(int row, const char* text, int len)
    {
        int start_col = VISIBLE_COL_LEFT + ((VISIBLE_COL_RIGHT - VISIBLE_COL_LEFT + 1) - len) / 2;
        if(start_col < VISIBLE_COL_LEFT)
        {
            start_col = VISIBLE_COL_LEFT;
        }
        _write_text(row, start_col, text, len);
    }

    // Write null-terminated text centered on a row.
    void _write_text_centered(int row, const char* text)
    {
        int len = 0;
        while(text[len] != '\0')
        {
            ++len;
        }

        _write_text_centered(row, text, len);
    }

    // Wrap a line into up to 2 centered rows, keeping wrapping stable while typewriting.
    void _write_wrapped_text_centered(int bottom_row, const char* text, int total_len, int visible_len)
    {
        int max_cols = VISIBLE_COL_RIGHT - VISIBLE_COL_LEFT + 1;

        if(visible_len <= 0)
        {
            return;
        }

        if(visible_len > total_len)
        {
            visible_len = total_len;
        }

        if(total_len <= max_cols)
        {
            _write_text_centered(bottom_row, text, visible_len);
            return;
        }

        int split = max_cols;
        for(int index = max_cols; index > 0; --index)
        {
            if(text[index - 1] == ' ')
            {
                split = index - 1;
                break;
            }
        }

        if(split <= 0)
        {
            split = max_cols;
        }

        int second_start = split;
        while(second_start < total_len && text[second_start] == ' ')
        {
            ++second_start;
        }

        int first_visible = bn::min(visible_len, split);
        if(first_visible > 0)
        {
            _write_text_centered(bottom_row - 1, text, first_visible);
        }

        if(visible_len > second_start)
        {
            int second_visible = bn::min(visible_len - second_start, max_cols);
            if(second_visible > 0)
            {
                _write_text_centered(bottom_row, text + second_start, second_visible);
            }
        }
    }

    void _render_options()
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

    void _handle_option_navigation()
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

    void _select_option(int idx)
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

    void _set_cell(int x, int y, int tile_index)
    {
        int index = y * MAP_COLUMNS + x;
        bn::regular_bg_map_cell_info cell_info(_cells[index]);
        cell_info.set_tile_index(tile_index);
        cell_info.set_palette_id(0);
        cell_info.set_horizontal_flip(false);
        cell_info.set_vertical_flip(false);
        _cells[index] = cell_info.cell();
    }

    void _clear_row(int row)
    {
        for(int col = 0; col < MAP_COLUMNS; ++col)
        {
            _set_cell(col, row, 0);  // tile 0 = space = transparent
        }
    }

    void _clear_text_area()
    {
        for(int row = TEXT_AREA_TOP; row <= TEXT_AREA_BOTTOM; ++row)
        {
            _clear_row(row);
        }
    }

    void _clear_all_text()
    {
        for(int row = 0; row < MAP_ROWS; ++row)
        {
            _clear_row(row);
        }
        _flush();
    }

    void _flush()
    {
        if(_bg_map.has_value())
        {
            _bg_map->reload_cells_ref();
        }
    }

    // Map cell data (EWRAM — too large for IWRAM stack)
    alignas(int) BN_DATA_EWRAM static bn::regular_bg_map_cell _cells[MAP_CELLS];
    alignas(int) BN_DATA_EWRAM static bn::regular_bg_map_cell _backdrop_cells[MAP_CELLS];

    bn::optional<bn::regular_bg_ptr> _bg;
    bn::optional<bn::regular_bg_map_ptr> _bg_map;
    bn::optional<bn::regular_bg_ptr> _backdrop_bg;
    bn::vector<DialogOption, 8> _options;
    bn::span<const bn::string_view> _greeting_lines;
    bn::span<const bn::string_view> _current_lines;
    bool _active;
    bool _prompt_visible;
    State _state;
    int _current_line;
    int _current_char;
    int _hold_counter;
    int _selected_option;
    int _scroll_offset;
    bool _backdrop_visible;
    bn::vector<bn::sprite_ptr, BACKDROP_SEGMENTS> _dialog_backdrop_sprites;
};

// Static EWRAM allocation for BG map cells
alignas(int) BN_DATA_EWRAM bn::regular_bg_map_cell BgDialog::_cells[BgDialog::MAP_CELLS];
alignas(int) BN_DATA_EWRAM bn::regular_bg_map_cell BgDialog::_backdrop_cells[BgDialog::MAP_CELLS];

} // namespace str

#endif // STR_BG_DIALOG_H
