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
namespace str
{
class BgDialog
{
public:
    struct DialogOption
    {
        bn::string_view option_text;
        bn::span<const bn::string_view> response_lines;
        bool ends_conversation;
    };
    BgDialog();
    void set_greeting(bn::span<const bn::string_view> lines);
    void set_options(bn::span<const DialogOption> options);
    void talk();
    [[nodiscard]] bool is_active() const { return _active; }
    void show_prompt();
    void hide_prompt();
    void update();
private:
    static constexpr int MAP_COLUMNS = 32;
    static constexpr int MAP_ROWS = 32;
    static constexpr int MAP_CELLS = MAP_COLUMNS * MAP_ROWS;
    static constexpr int TEXT_ROW = 24;
    static constexpr int OPTIONS_ROW = 22;
    static constexpr int PROMPT_ROW = 24;
    static constexpr int VISIBLE_COL_LEFT = 1;
    static constexpr int VISIBLE_COL_RIGHT = 30;
    static constexpr int VISIBLE_OPTIONS = 3;
    static constexpr int BACKDROP_SEGMENTS = 3;
    static constexpr int BACKDROP_Y = 64;
    static constexpr int BACKDROP_FALLBACK_TILES_COUNT = 2;
    static constexpr int BACKDROP_FALLBACK_PALETTE_COLORS_COUNT = 16;
    static constexpr int TEXT_AREA_TOP = 22;
    static constexpr int TEXT_AREA_BOTTOM = 25;
    static constexpr bn::tile _fallback_backdrop_tiles[BACKDROP_FALLBACK_TILES_COUNT] = {
        { { 0, 0, 0, 0, 0, 0, 0, 0 } },
        { { 0x11111111, 0x11111111, 0x11111111, 0x11111111,
            0x11111111, 0x11111111, 0x11111111, 0x11111111 } }
    };
    static constexpr bn::color _fallback_backdrop_palette[BACKDROP_FALLBACK_PALETTE_COLORS_COUNT] = {
        bn::color(0, 0, 0),
        bn::color(2, 2, 4),
        bn::color(0, 0, 0),
        bn::color(0, 0, 0),
        bn::color(0, 0, 0),
        bn::color(0, 0, 0),
        bn::color(0, 0, 0),
        bn::color(0, 0, 0),
        bn::color(0, 0, 0),
        bn::color(0, 0, 0),
        bn::color(0, 0, 0),
        bn::color(0, 0, 0),
        bn::color(0, 0, 0),
        bn::color(0, 0, 0),
        bn::color(0, 0, 0),
        bn::color(0, 0, 0)
    };
    enum State
    {
        STATE_IDLE,
        STATE_GREETING,
        STATE_SHOWING_OPTIONS,
        STATE_SHOWING_RESPONSE
    };
    void _end();
    void _set_backdrop_visible(bool visible);
    void _refresh_backdrop_visibility();
    void _init_fallback_backdrop_bg();
    void _write_text(int row, int col, const char* text, int len);
    void _write_text_centered(int row, const char* text, int len);
    void _write_text_centered(int row, const char* text);
    void _write_wrapped_text_centered(int bottom_row, const char* text, int total_len, int visible_len);
    void _render_options();
    void _handle_option_navigation();
    void _select_option(int idx);
    void _set_cell(int x, int y, int tile_index);
    void _clear_row(int row);
    void _clear_text_area();
    void _clear_all_text();
    void _flush();
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
} // namespace str
#endif // STR_BG_DIALOG_H
