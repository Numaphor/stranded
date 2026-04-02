#include "str_scene_room_viewer.h"

#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_bg_palettes.h"
#include "bn_bg_tiles.h"
#include "bn_color.h"
#include "bn_memory.h"
#include "bn_sprites.h"
#include "bn_display.h"
#include "bn_sprite_text_generator.h"
#include "bn_sprite_affine_mat_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_affine_mat_attributes.h"
#include "bn_fixed.h"
#include "bn_math.h"
#include "bn_algorithm.h"
#include "bn_assert.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_map_item.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_map_cell.h"
#include "bn_regular_bg_map_cell_info.h"
#include "bn_regular_bg_ptr.h"
#include "bn_string.h"
#include "bn_sstream.h"
#include "bn_log.h"
#include "bn_profiler.h"
#include "bn_unordered_map.h"
#include "bn_point.h"
#include "bn_bg_palette_items_dialog_font_palette.h"
#include "bn_regular_bg_tiles_items_dialog_font_tiles.h"
#include "bn_sprite_items_player_idle.h"
#include "bn_sprite_items_player_walk.h"
#include "bn_sprite_items_villager.h"
#include "bn_sprite_palette_ptr.h"
#include "str_bg_dialog.h"
#include "bn_sprite_items_escaping_criticism_wall_bottom.h"
#include "bn_sprite_items_escaping_criticism_wall_top.h"
#include "bn_sprite_items_mr_and_mrs_andrews_wall_bottom.h"
#include "bn_sprite_items_mr_and_mrs_andrews_wall_top.h"

#include "common_variable_8x8_sprite_font.h"
#include "str_minimap.h"

#include "fr_sin_cos.h"
#include "fr_div_lut.h"
#include "fr_constants_3d.h"
#include "models/str_model_3d_items_room.h"
#include "models/str_model_3d_items_books.h"
#include "models/str_model_3d_items_potted_plant.h"
#include "../butano/butano/hw/include/bn_hw_sprites.h"

namespace {
    class overlay_resources
    {
    public:
        static constexpr int MAP_COLUMNS = 32;
        static constexpr int MAP_ROWS = 32;
        static constexpr int MAP_CELLS = MAP_COLUMNS * MAP_ROWS;
        static constexpr int VISIBLE_COL_LEFT = 1;
        static constexpr int VISIBLE_COL_RIGHT = 30;
        static constexpr int MENU_TOP_ROW = 6;
        static constexpr int MENU_BOTTOM_ROW = 25;

        static overlay_resources& instance()
        {
            static overlay_resources resources;
            return resources;
        }

        [[nodiscard]] bool ensure_visible()
        {
            if(_bg && _backdrop_bg && _bg_map)
            {
                _bg->set_visible(true);
                _backdrop_bg->set_visible(true);
                return true;
            }

            bn::regular_bg_map_item map_item(_cells[0], bn::size(MAP_COLUMNS, MAP_ROWS));
            bn::regular_bg_item bg_item(
                bn::regular_bg_tiles_items::dialog_font_tiles,
                bn::bg_palette_items::dialog_font_palette,
                map_item);

            bn::regular_bg_map_item backdrop_map_item(_backdrop_cells[0], bn::size(MAP_COLUMNS, MAP_ROWS));
            bn::regular_bg_item backdrop_bg_item(
                bn::regular_bg_tiles_item(
                    bn::span<const bn::tile>(_backdrop_tiles, BACKDROP_TILES_COUNT),
                    bn::bpp_mode::BPP_4),
                bn::bg_palette_item(
                    bn::span<const bn::color>(_backdrop_palette, BACKDROP_PALETTE_COLORS_COUNT),
                    bn::bpp_mode::BPP_4),
                backdrop_map_item);

            bool old_offset = bn::bg_tiles::allow_offset();
            bn::bg_tiles::set_allow_offset(false);
            _bg = bg_item.create_bg_optional(0, 0);
            _backdrop_bg = backdrop_bg_item.create_bg_optional(0, 0);
            bn::bg_tiles::set_allow_offset(old_offset);

            if(! _bg || ! _backdrop_bg)
            {
                hide();
                return false;
            }

            _bg->set_priority(0);
            _bg_map = _bg->map();
            _backdrop_bg->set_priority(1);
            _bg->set_visible(true);
            _backdrop_bg->set_visible(true);
            return true;
        }

        void hide()
        {
            _bg_map.reset();
            _bg.reset();
            _backdrop_bg.reset();
        }

        [[nodiscard]] bool active() const
        {
            return _bg_map.has_value();
        }

        void clear_cells()
        {
            bn::memory::clear(_cells);
        }

        void reload()
        {
            if(_bg_map.has_value())
            {
                _bg_map->reload_cells_ref();
            }
        }

        void write_text(int row, int col, const char* text, int len)
        {
            for(int index = 0; index < len; ++index)
            {
                int target_col = col + index;

                if(target_col < VISIBLE_COL_LEFT)
                {
                    continue;
                }

                if(target_col > VISIBLE_COL_RIGHT)
                {
                    break;
                }

                int ch = static_cast<unsigned char>(text[index]);
                int tile_index = ch >= 32 && ch <= 127 ? ch - 32 : 0;
                _set_cell(target_col, row, tile_index);
            }
        }

        void write_text_centered(int row, const char* text, int len)
        {
            int start_col = VISIBLE_COL_LEFT + ((VISIBLE_COL_RIGHT - VISIBLE_COL_LEFT + 1) - len) / 2;
            start_col = bn::max(start_col, VISIBLE_COL_LEFT);
            write_text(row, start_col, text, len);
        }

        void write_text_centered(int row, const char* text)
        {
            int len = 0;

            while(text[len] != '\0')
            {
                ++len;
            }

            write_text_centered(row, text, len);
        }

    private:
        static constexpr int BACKDROP_TILES_COUNT = 2;
        static constexpr int BACKDROP_PALETTE_COLORS_COUNT = 16;

        static constexpr bn::tile _backdrop_tiles[BACKDROP_TILES_COUNT] = {
            { { 0, 0, 0, 0, 0, 0, 0, 0 } },
            { { 0x11111111, 0x11111111, 0x11111111, 0x11111111,
                0x11111111, 0x11111111, 0x11111111, 0x11111111 } }
        };

        static constexpr bn::color _backdrop_palette[BACKDROP_PALETTE_COLORS_COUNT] = {
            bn::color(0, 0, 0), bn::color(2, 2, 4), bn::color(0, 0, 0), bn::color(0, 0, 0),
            bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0),
            bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0),
            bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0)
        };

        alignas(int) static BN_DATA_EWRAM bn::regular_bg_map_cell _cells[MAP_CELLS];
        alignas(int) static BN_DATA_EWRAM bn::regular_bg_map_cell _backdrop_cells[MAP_CELLS];

        bn::optional<bn::regular_bg_ptr> _bg;
        bn::optional<bn::regular_bg_ptr> _backdrop_bg;
        bn::optional<bn::regular_bg_map_ptr> _bg_map;

        overlay_resources()
        {
            bn::memory::clear(_cells);
            bn::memory::clear(_backdrop_cells);

            for(int row = MENU_TOP_ROW; row <= MENU_BOTTOM_ROW; ++row)
            {
                for(int col = VISIBLE_COL_LEFT; col <= VISIBLE_COL_RIGHT; ++col)
                {
                    _set_cell(col, row, 1);
                    _set_backdrop_cell(col, row, 1);
                }
            }
        }

        void _set_cell(int x, int y, int tile_index)
        {
            _set_cell_value(_cells, x, y, tile_index);
        }

        void _set_backdrop_cell(int x, int y, int tile_index)
        {
            _set_cell_value(_backdrop_cells, x, y, tile_index);
        }

        static void _set_cell_value(bn::regular_bg_map_cell* cells, int x, int y, int tile_index)
        {
            int index = y * MAP_COLUMNS + x;
            bn::regular_bg_map_cell_info cell_info(cells[index]);
            cell_info.set_tile_index(tile_index);
            cell_info.set_palette_id(0);
            cell_info.set_horizontal_flip(false);
            cell_info.set_vertical_flip(false);
            cells[index] = cell_info.cell();
        }
    };

    class profiler_menu
    {
    public:
        [[nodiscard]] bool visible() const
        {
            return _visible;
        }

        void set_visible(bool visible)
        {
            if(_visible == visible)
            {
                return;
            }

            if(visible)
            {
                if(! overlay_resources::instance().ensure_visible())
                {
                    _visible = false;
                    return;
                }

                _visible = true;
                _scroll_offset = 0;
                _frame_counter = 0;
                _refresh();
            }
            else
            {
                _visible = false;
                overlay_resources::instance().hide();
            }
        }

        void update()
        {
            if(! _visible)
            {
                return;
            }

            if(bn::keypad::a_pressed())
            {
                _show_total = ! _show_total;
                _scroll_offset = 0;
            }

            if(bn::keypad::start_pressed())
            {
                BN_PROFILER_RESET();
                _scroll_offset = 0;
            }

            if(bn::keypad::down_pressed())
            {
                ++_scroll_offset;
                _frame_counter = 0;
            }
            else if(bn::keypad::up_pressed())
            {
                --_scroll_offset;
                _frame_counter = 0;
            }

            ++_frame_counter;

            if(_frame_counter >= REFRESH_INTERVAL || bn::keypad::a_pressed() || bn::keypad::start_pressed() ||
               bn::keypad::down_pressed() || bn::keypad::up_pressed())
            {
                _frame_counter = 0;
                _refresh();
            }
        }

    private:
        struct entry_data
        {
            const char* id;
            int64_t total_ticks;
            int max_ticks;
        };

        static constexpr int TITLE_ROW = 7;
        static constexpr int SUMMARY_ROW = 8;
        static constexpr int CONTROLS_ROW_A = 9;
        static constexpr int CONTROLS_ROW_B = 10;
        static constexpr int ENTRIES_ROW = 12;
        static constexpr int VISIBLE_ENTRY_ROWS = overlay_resources::MENU_BOTTOM_ROW - ENTRIES_ROW;
        static constexpr int REFRESH_INTERVAL = 12;
        bool _visible = false;
        bool _show_total = true;
        int _scroll_offset = 0;
        int _frame_counter = 0;

        void _append_clipped_id(const char* id, int max_chars, bn::string<64>& output)
        {
            int id_len = 0;

            while(id[id_len] != '\0')
            {
                ++id_len;
            }

            if(id_len <= max_chars)
            {
                output.append(id, id_len);
                return;
            }

            int clipped_len = bn::max(0, max_chars - 3);
            output.append(id, clipped_len);
            output.append("...");
        }

        void _collect_entries(bn::vector<entry_data, BN_CFG_PROFILER_MAX_ENTRIES>& entries, int64_t& global_value) const
        {
            entries.clear();
            global_value = 0;

            const auto& ticks_per_entry = _bn::profiler::ticks_per_entry();

            for(const auto& ticks_per_entry_pair : ticks_per_entry)
            {
                const auto& ticks_entry = ticks_per_entry_pair.second;
                entry_data entry = {
                    ticks_per_entry_pair.first,
                    ticks_entry.total,
                    ticks_entry.max
                };
                entries.push_back(entry);
            }

            bn::sort(entries.begin(), entries.end(), [this](const entry_data& a, const entry_data& b) {
                return _show_total ? a.total_ticks > b.total_ticks : a.max_ticks > b.max_ticks;
            });

            for(const entry_data& entry : entries)
            {
                global_value += _show_total ? entry.total_ticks : entry.max_ticks;
            }
        }

        void _flush()
        {
            overlay_resources::instance().reload();
        }

        void _refresh()
        {
            if(! overlay_resources::instance().active())
            {
                return;
            }

            bn::vector<entry_data, BN_CFG_PROFILER_MAX_ENTRIES> entries;
            int64_t global_value = 0;
            _collect_entries(entries, global_value);

            int max_scroll = bn::max(0, entries.size() - VISIBLE_ENTRY_ROWS);
            _scroll_offset = bn::clamp(_scroll_offset, 0, max_scroll);

            overlay_resources& resources = overlay_resources::instance();
            resources.clear_cells();
            resources.write_text_centered(TITLE_ROW, _show_total ? "PROFILER TOTAL" : "PROFILER MAX");

            bn::string<64> summary;
            bn::ostringstream summary_stream(summary);
            summary_stream << "SUM:" << global_value << " ENTRIES:" << entries.size();
            resources.write_text_centered(SUMMARY_ROW, summary.data(), summary.size());
            resources.write_text_centered(CONTROLS_ROW_A, "A:MODE START:RESET");
            resources.write_text_centered(CONTROLS_ROW_B, "UP/DOWN:SCROLL B:CLOSE");

            if(entries.empty())
            {
                resources.write_text_centered(ENTRIES_ROW, "NO PROFILER DATA RECORDED");
                _flush();
                return;
            }

            int visible_entries = bn::min(VISIBLE_ENTRY_ROWS, entries.size() - _scroll_offset);

            for(int index = 0; index < visible_entries; ++index)
            {
                const entry_data& entry = entries[_scroll_offset + index];
                int64_t entry_value = _show_total ? entry.total_ticks : entry.max_ticks;

                bn::string<8> index_text;
                bn::ostringstream index_stream(index_text);
                index_stream << _scroll_offset + index + 1 << '.';

                bn::string<24> value_text;
                bn::ostringstream value_stream(value_text);
                value_stream << entry_value;

                bn::string<8> pct_text;
                if(global_value)
                {
                    bn::ostringstream pct_stream(pct_text);
                    pct_stream << int((entry_value * 100) / global_value) << '%';
                }

                int max_line_chars = overlay_resources::VISIBLE_COL_RIGHT - overlay_resources::VISIBLE_COL_LEFT + 1;
                int fixed_chars = index_text.size() + 1 + value_text.size();
                if(! pct_text.empty())
                {
                    fixed_chars += 1 + pct_text.size();
                }

                int max_id_chars = bn::max(4, max_line_chars - fixed_chars - 1);

                bn::string<64> line;
                line.append(index_text);
                line.append(" ");
                _append_clipped_id(entry.id, max_id_chars, line);
                line.append(" ");
                line.append(value_text);

                if(! pct_text.empty())
                {
                    line.append(" ");
                    line.append(pct_text);
                }

                resources.write_text(ENTRIES_ROW + index, overlay_resources::VISIBLE_COL_LEFT, line.data(), line.size());
            }

            if(_scroll_offset > 0)
            {
                resources.write_text(overlay_resources::MENU_TOP_ROW, overlay_resources::VISIBLE_COL_RIGHT, "^", 1);
            }

            if(_scroll_offset + VISIBLE_ENTRY_ROWS < entries.size())
            {
                resources.write_text(overlay_resources::MENU_BOTTOM_ROW, overlay_resources::VISIBLE_COL_RIGHT, "v", 1);
            }

            _flush();
        }
    };

    class debug_menu
    {
    public:
        struct state
        {
            int room_id = -1;
            int fps = -1;
            int vertices_count = -1;
            int hlines_count = -1;
            int room_x = 0;
            int room_y = 0;
            int dir = 0;
            int corner = 0;
            int yaw_degrees = 0;
            const char* preview_name = "";
            bool room_models_visible = true;
            int used_alloc_ewram = 0;
            int used_static_ewram = 0;
            int used_static_iwram = 0;
            int used_bg_tiles = 0;
            int available_bg_tiles = 0;
            int used_bg_blocks = 0;
            int available_bg_blocks = 0;

            [[nodiscard]] bool operator==(const state& other) const
            {
                return room_id == other.room_id &&
                       fps == other.fps &&
                       vertices_count == other.vertices_count &&
                       hlines_count == other.hlines_count &&
                       room_x == other.room_x &&
                       room_y == other.room_y &&
                       dir == other.dir &&
                       corner == other.corner &&
                       yaw_degrees == other.yaw_degrees &&
                       preview_name == other.preview_name &&
                       room_models_visible == other.room_models_visible &&
                       used_alloc_ewram == other.used_alloc_ewram &&
                       used_static_ewram == other.used_static_ewram &&
                       used_static_iwram == other.used_static_iwram &&
                       used_bg_tiles == other.used_bg_tiles &&
                       available_bg_tiles == other.available_bg_tiles &&
                       used_bg_blocks == other.used_bg_blocks &&
                       available_bg_blocks == other.available_bg_blocks;
            }
        };

        [[nodiscard]] bool visible() const
        {
            return _visible;
        }

        void set_visible(bool visible)
        {
            if(_visible == visible)
            {
                return;
            }

            if(visible)
            {
                if(! overlay_resources::instance().ensure_visible())
                {
                    _visible = false;
                    return;
                }

                _visible = true;
                _frame_counter = 0;
                _has_last_state = false;
            }
            else
            {
                _visible = false;
                overlay_resources::instance().hide();
            }
        }

        void update(const state& current_state)
        {
            if(! _visible || ! overlay_resources::instance().active())
            {
                return;
            }

            ++_frame_counter;
            bool should_refresh = ! _has_last_state || current_state != _last_state;

            if(! should_refresh && _frame_counter < REFRESH_INTERVAL)
            {
                return;
            }

            _frame_counter = 0;
            _last_state = current_state;
            _has_last_state = true;

            overlay_resources& resources = overlay_resources::instance();
            resources.clear_cells();
            resources.write_text_centered(TITLE_ROW, "DEBUG VIEW");

            bn::string<40> line0;
            bn::ostringstream line0_stream(line0);
            line0_stream << "POS:" << current_state.room_x << "," << current_state.room_y << " DIR:" << current_state.dir;
            resources.write_text_centered(INFO_ROW_0, line0.data(), line0.size());

            bn::string<40> line1;
            bn::ostringstream line1_stream(line1);
            line1_stream << "ROOM:" << current_state.room_id << " C:" << current_state.corner << " Y:" << current_state.yaw_degrees;
            resources.write_text_centered(INFO_ROW_1, line1.data(), line1.size());

            bn::string<40> line2;
            bn::ostringstream line2_stream(line2);
            line2_stream << "FPS:" << current_state.fps << " V:" << current_state.vertices_count
                         << "/" << fr::models_3d::max_vertices();
            resources.write_text_centered(INFO_ROW_2, line2.data(), line2.size());

            bn::string<40> line3;
            bn::ostringstream line3_stream(line3);
            line3_stream << "PREV:" << current_state.preview_name
                         << " RM:" << (current_state.room_models_visible ? "ON" : "OFF")
                         << " HL:" << current_state.hlines_count;
            resources.write_text_centered(INFO_ROW_3, line3.data(), line3.size());

            bn::string<40> line4;
            bn::ostringstream line4_stream(line4);
            line4_stream << "EWA:" << current_state.used_alloc_ewram
                         << " EWS:" << current_state.used_static_ewram;
            resources.write_text_centered(INFO_ROW_4, line4.data(), line4.size());

            bn::string<40> line5;
            bn::ostringstream line5_stream(line5);
            line5_stream << "IWS:" << current_state.used_static_iwram;
            resources.write_text_centered(INFO_ROW_5, line5.data(), line5.size());

            bn::string<40> line6;
            bn::ostringstream line6_stream(line6);
            line6_stream << "BT:" << current_state.used_bg_tiles << "/" << current_state.available_bg_tiles;
            resources.write_text_centered(INFO_ROW_6, line6.data(), line6.size());

            bn::string<40> line7;
            bn::ostringstream line7_stream(line7);
            line7_stream << "BB:" << current_state.used_bg_blocks << "/" << current_state.available_bg_blocks;
            resources.write_text_centered(INFO_ROW_7, line7.data(), line7.size());

            resources.write_text_centered(CONTROLS_ROW_0, "SELECT:CLOSE B:PROF");
            resources.write_text_centered(CONTROLS_ROW_1, "START:RECENTER L/R:ZOOM");
            resources.write_text_centered(CONTROLS_ROW_2, "SEL+L:TOGGLE RM");
            resources.reload();
        }

    private:
        static constexpr int TITLE_ROW = 7;
        static constexpr int INFO_ROW_0 = 9;
        static constexpr int INFO_ROW_1 = 10;
        static constexpr int INFO_ROW_2 = 11;
        static constexpr int INFO_ROW_3 = 12;
        static constexpr int INFO_ROW_4 = 14;
        static constexpr int INFO_ROW_5 = 15;
        static constexpr int INFO_ROW_6 = 16;
        static constexpr int INFO_ROW_7 = 17;
        static constexpr int CONTROLS_ROW_0 = 20;
        static constexpr int CONTROLS_ROW_1 = 21;
        static constexpr int CONTROLS_ROW_2 = 22;
        static constexpr int REFRESH_INTERVAL = 8;
        bool _visible = false;
        bool _has_last_state = false;
        int _frame_counter = 0;
        state _last_state;
    };

    alignas(int) BN_DATA_EWRAM bn::regular_bg_map_cell overlay_resources::_cells[overlay_resources::MAP_CELLS];
    alignas(int) BN_DATA_EWRAM bn::regular_bg_map_cell overlay_resources::_backdrop_cells[overlay_resources::MAP_CELLS];

    constexpr int NUM_ROOMS = 2;
    constexpr int QUARTER_TURN_ANGLE = 16384;
    constexpr int CAMERA_BEHIND_OFFSET_ANGLE = 24576;
    constexpr int CAMERA_INITIAL_LOCK_FRAMES = 120;
    constexpr int CAMERA_IDLE_RECENTER_DELAY_FRAMES = 60;
    constexpr bn::fixed CAMERA_CENTER_HOLD_HALF_EXTENT = 16;
    constexpr int CAMERA_TURN_MAX_STEP_ANGLE = 2048;
    constexpr bn::fixed CAMERA_TURN_GAIN = bn::fixed(0.35);
    constexpr bn::fixed CAMERA_TURN_RESPONSE = bn::fixed(0.65);
    constexpr int CAMERA_TURN_SNAP_EPSILON = 96;
    constexpr int CAMERA_RENDER_UPDATE_ANGLE_STEP = 64;
    constexpr bn::fixed CAMERA_AUTO_FIT_MIN_DIST = 100;
    constexpr bn::fixed CAMERA_AUTO_FIT_MAX_DIST = 500;
    constexpr int CAMERA_AUTO_FIT_MARGIN_X = 8;
    constexpr int CAMERA_AUTO_FIT_MARGIN_Y = 6;
    constexpr int CAMERA_AUTO_FIT_BINARY_SEARCH_STEPS = 10;
    constexpr bn::fixed CAMERA_AUTO_FIT_FILL_FACTOR = bn::fixed(0.82);
    constexpr int PAINTING_MOTION_UPDATE_INTERVAL_FRAMES = 2;
    constexpr bn::fixed PAINTING_FACE_VISIBILITY_DOT_MIN = bn::fixed(8);
    constexpr int PAINTING_MIN_TRI_AREA2 = 220;
    constexpr int PAINTING_MIN_TRI_SPAN = 4;
    constexpr int PAINTING_MAX_AFFINE_REGISTER = 16384;
    constexpr int ADJACENT_ROOM_DEPTH_BIAS = 1500000;
    constexpr int TRANSITION_DECOR_DEPTH_BIAS = ADJACENT_ROOM_DEPTH_BIAS;
    constexpr int ROOM_PREVIEW_TARGET_FPS = 50;
    constexpr int ROOM_PREVIEW_FPS_HYSTERESIS = 3;
    constexpr int ROOM_PREVIEW_DOWNSHIFT_FPS = ROOM_PREVIEW_TARGET_FPS - ROOM_PREVIEW_FPS_HYSTERESIS;
    constexpr int ROOM_PREVIEW_UPSHIFT_FPS = ROOM_PREVIEW_TARGET_FPS + ROOM_PREVIEW_FPS_HYSTERESIS;
    constexpr int ROOM_PREVIEW_REQUIRED_SAMPLES = 2;
    constexpr int ROOM_PREVIEW_MODE_CHANGE_COOLDOWN_FRAMES = 180;
    constexpr bool ROOM_PREVIEW_AUTO_ADJUST = false;
    constexpr bool ENABLE_RENDER_SWEEP_DEBUG = false;
    constexpr bool ENABLE_PAINTING_QUADS = true;
    constexpr bool ENABLE_NPC_SPRITES = true;
    constexpr bool ENABLE_MINIMAP = true;
    constexpr bn::fixed NPC_FX = 20;
    constexpr bn::fixed NPC_FY = -15;
    constexpr bn::fixed NPC_FZ = -10;
    constexpr int NPC_ANIM_SPEED = 12;
    constexpr int NPC_FRAMES_PER_ANIM = 4;
    constexpr int NPC_PALETTE_HAT_INDEX_0 = 9;
    constexpr int NPC_PALETTE_HAT_INDEX_1 = 10;
    constexpr int NPC_PALETTE_HAT_INDEX_2 = 11;
    constexpr int NPC_ROOM_A = 0;
    constexpr int NPC_ROOM_B = 1;
    constexpr int PLAYER_IDLE_FRAMES_PER_ANGLE = 17;
    constexpr int PLAYER_WALK_FRAMES_PER_ANGLE = 8;
    constexpr int PLAYER_ANIM_SPEED = 5;
    constexpr bn::fixed PLAYER_SPRITE_SCALE = bn::fixed(7) / 8;
    int player_angle_row(int dir, bool facing_left)
    {
        switch(dir)
        {
            case 0:
                return 0;

            case 1:
                return facing_left ? 1 : 7;

            case 2:
                return facing_left ? 2 : 6;

            case 3:
                return facing_left ? 3 : 5;

            case 4:
                return 4;

            default:
                return 0;
        }
    }

    // Give the second room's NPC a distinct hat tint so both villagers stay readable.
    constexpr bn::color npc_room_b_hat_color_0(12, 28, 24);
    constexpr bn::color npc_room_b_hat_color_1(6, 20, 18);
    constexpr bn::color npc_room_b_hat_color_2(8, 12, 16);

    // NPC dialog data
    constexpr int NPC_INTERACT_DIST = 30; // Manhattan distance threshold

    constexpr bn::string_view villager_a_greeting[] = {
        "Hello, traveler!",
        "These old halls hold many secrets.",
        "What brings you here?"
    };

    constexpr bn::string_view villager_a_opt0_resp[] = {
        "This gallery was built centuries ago.",
        "The paintings... they watch you."
    };
    constexpr bn::string_view villager_a_opt1_resp[] = {
        "Be careful in the dark rooms.",
        "Strange things lurk in the shadows."
    };

    constexpr bn::string_view villager_b_greeting[] = {
        "Ah, another visitor!",
        "Not many make it this far.",
        "Can I help you with something?"
    };

    constexpr bn::string_view villager_b_opt0_resp[] = {
        "I've been here longer than I remember.",
        "Time moves strangely in this place."
    };
    constexpr bn::string_view villager_b_opt1_resp[] = {
        "The exit? I think it's through",
        "the room with the tall painting."
    };

    struct corner_matrix
    {
        bn::fixed r00, r01, r02;
        bn::fixed r10, r11, r12;
        bn::fixed r20, r21, r22;
    };

    enum class room_preview_mode
    {
        all_connected,
        preferred_only,
        off
    };

    const char* room_preview_mode_name(room_preview_mode mode)
    {
        switch(mode)
        {
            case room_preview_mode::all_connected:
                return "ALL";

            case room_preview_mode::preferred_only:
                return "ONE";

            default:
                return "OFF";
        }
    }

    room_preview_mode downgraded_room_preview_mode(room_preview_mode mode)
    {
        switch(mode)
        {
            case room_preview_mode::all_connected:
                return room_preview_mode::preferred_only;

            case room_preview_mode::preferred_only:
                return room_preview_mode::off;

            default:
                return room_preview_mode::off;
        }
    }

    room_preview_mode upgraded_room_preview_mode(room_preview_mode mode)
    {
        switch(mode)
        {
            case room_preview_mode::off:
                return room_preview_mode::preferred_only;

            case room_preview_mode::preferred_only:
                return room_preview_mode::all_connected;

            default:
                return room_preview_mode::all_connected;
        }
    }

    int normalize_angle(int angle)
    {
        return int(unsigned(angle) & 0xFFFF);
    }

    int shortest_angle_delta(int from, int to)
    {
        int delta = normalize_angle(to - from);

        if(delta > 32767)
        {
            delta -= 65536;
        }

        return delta;
    }

    int corner_from_view_angle(int angle)
    {
        return ((normalize_angle(-angle) + 8192) / QUARTER_TURN_ANGLE) & 3;
    }

    int wrap_linear8(int linear)
    {
        return ((linear % 8) + 8) % 8;
    }

    int view_angle_steps_8(int angle)
    {
        return normalize_angle(angle) / 8192;
    }

    int heading_angle_from_linear8(int linear)
    {
        return normalize_angle(8192 - wrap_linear8(linear) * 8192);
    }

    int step_angle_toward_target(int current_angle, int target_angle, bn::fixed& turn_velocity)
    {
        int angle_delta = shortest_angle_delta(current_angle, target_angle);

        if(angle_delta == 0)
        {
            turn_velocity = 0;
            return current_angle;
        }

        int abs_angle_delta = angle_delta >= 0 ? angle_delta : -angle_delta;

        if(abs_angle_delta <= CAMERA_TURN_SNAP_EPSILON)
        {
            turn_velocity = 0;
            return target_angle;
        }

        bn::fixed max_step = CAMERA_TURN_MAX_STEP_ANGLE;
        bn::fixed desired_step = bn::fixed(angle_delta) * CAMERA_TURN_GAIN;
        desired_step = bn::max(-max_step, bn::min(desired_step, max_step));
        turn_velocity += (desired_step - turn_velocity) * CAMERA_TURN_RESPONSE;

        int step = turn_velocity.round_integer();

        if(step == 0)
        {
            step = angle_delta > 0 ? 1 : -1;
        }

        if(step > 0)
        {
            step = bn::min(step, abs_angle_delta);
        }
        else
        {
            step = -bn::min(-step, abs_angle_delta);
        }

        return normalize_angle(current_angle + step);
    }

    // Convert (dir, facing_left) to a linear 0-7 index and back.
    // Linear: 0=down 1=down-right 2=right 3=up-right 4=up 5=up-left 6=left 7=down-left
    int dir_to_linear8(int dir, bool facing_left)
    {
        if(!facing_left)
        {
            return dir; // 0-4 maps directly to 0=down..4=up
        }
        switch(dir)
        {
            case 1: return 7; // down-left
            case 2: return 6; // left
            case 3: return 5; // up-left
            default: return dir; // 0=down, 4=up unchanged
        }
    }

    void linear8_to_dir(int linear, int& out_dir, bool& out_facing_left)
    {
        linear = wrap_linear8(linear);
        switch(linear)
        {
            case 0: out_dir = 0; out_facing_left = false; break;
            case 1: out_dir = 1; out_facing_left = false; break;
            case 2: out_dir = 2; out_facing_left = false; break;
            case 3: out_dir = 3; out_facing_left = false; break;
            case 4: out_dir = 4; out_facing_left = false; break;
            case 5: out_dir = 3; out_facing_left = true;  break;
            case 6: out_dir = 2; out_facing_left = true;  break;
            case 7: out_dir = 1; out_facing_left = true;  break;
            default: out_dir = 0; out_facing_left = false; break;
        }
    }

    bn::fixed_point screen_to_room_delta(bn::fixed screen_dx, bn::fixed screen_dy, int view_angle)
    {
        bn::fixed base_dx = screen_dx + screen_dy;
        bn::fixed base_dy = screen_dy - screen_dx;
        int normalized_view_angle = normalize_angle(view_angle);
        bn::fixed s = fr::sin(normalized_view_angle);
        bn::fixed c = fr::cos(normalized_view_angle);
        return bn::fixed_point(base_dx * c - base_dy * s, base_dx * s + base_dy * c);
    }

    corner_matrix rotate_corner_matrix(const corner_matrix& base, int angle)
    {
        int normalized_angle = normalize_angle(angle);
        bn::fixed s = fr::sin(normalized_angle);
        bn::fixed c = fr::cos(normalized_angle);

        return {
            base.r00 * c - base.r01 * s, base.r00 * s + base.r01 * c, base.r02,
            base.r10 * c - base.r11 * s, base.r10 * s + base.r11 * c, base.r12,
            base.r20 * c - base.r21 * s, base.r20 * s + base.r21 * c, base.r22
        };
    }

    void compute_corner_matrices(corner_matrix out[4])
    {
        constexpr bn::fixed FLOOR_X_AXIS_X = bn::fixed(0.716816);
        constexpr bn::fixed FLOOR_X_AXIS_Y = bn::fixed(0.5984);
        constexpr bn::fixed FLOOR_X_AXIS_Z = bn::fixed(0.3579);
        constexpr bn::fixed FLOOR_Y_AXIS_X = bn::fixed(-0.697102);
        constexpr bn::fixed FLOOR_Y_AXIS_Y = bn::fixed(0.626033);
        constexpr bn::fixed FLOOR_Y_AXIS_Z = bn::fixed(0.349472);
        constexpr bn::fixed FLOOR_NORMAL_X = bn::fixed(-0.014934);
        constexpr bn::fixed FLOOR_NORMAL_Y = bn::fixed(-0.5);
        constexpr bn::fixed FLOOR_NORMAL_Z = bn::fixed(0.865897);

        // Keep the original room-viewer bearing, but retune the floor pitch to 60 degrees from above.
        const corner_matrix base = {
            FLOOR_X_AXIS_X, FLOOR_Y_AXIS_X, FLOOR_NORMAL_X,
            FLOOR_X_AXIS_Y, FLOOR_Y_AXIS_Y, FLOOR_NORMAL_Y,
            FLOOR_X_AXIS_Z, FLOOR_Y_AXIS_Z, FLOOR_NORMAL_Z
        };

        out[0] = base;
        out[1] = rotate_corner_matrix(base, QUARTER_TURN_ANGLE);
        out[2] = rotate_corner_matrix(base, QUARTER_TURN_ANGLE * 2);
        out[3] = rotate_corner_matrix(base, QUARTER_TURN_ANGLE * 3);
    }

    constexpr bn::fixed BOOKS_FX = 24;
    constexpr bn::fixed BOOKS_FY = -12;
    constexpr bn::fixed POTTED_PLANT_FX = -28;
    constexpr bn::fixed POTTED_PLANT_FY = 24;
    // Textured-polygon painting quads in room-local coordinates.
    // Their wall anchors are derived from the current room half extents.
    // A (back wall): mr_and_mrs_andrews (192x112, landscape)
    // B (right wall): escaping_criticism (80x100, portrait)
    constexpr bn::fixed PAINTING_A_HALF_WIDTH = bn::fixed(14.4);
    constexpr bn::fixed PAINTING_A_HALF_HEIGHT = bn::fixed(8.4);
    constexpr bn::fixed PAINTING_B_HALF_WIDTH = bn::fixed(8.0);
    constexpr bn::fixed PAINTING_B_HALF_HEIGHT = bn::fixed(10.0);
    constexpr bn::fixed PAINTING_Z_CENTER = -24;
    constexpr bn::fixed PAINTING_WALL_INSET = bn::fixed(0.2);
    constexpr bn::fixed PAINTING_A_CENTER_X = -38;
    constexpr bn::fixed PAINTING_B_CENTER_Y = -34;

    constexpr bn::fixed BOOKS_HW = 4;
    constexpr bn::fixed BOOKS_HD = 4;
    constexpr bn::fixed POTTED_PLANT_HW = 6;
    constexpr bn::fixed POTTED_PLANT_HD = 6;
    constexpr bn::fixed ROOM_WALL_TOP_Z = bn::fixed(-50);

    constexpr bn::fixed DOOR_HALF_WIDTH = 10;
    constexpr bn::fixed DOOR_APPROACH_EDGE_MARGIN = 18;
    constexpr bn::fixed DOOR_APPROACH_LANE_MARGIN = 12;
    constexpr int DOOR_TRANSITION_MAX_STEPS_PER_UPDATE = 2;
    constexpr int DOOR_TRANSITION_MAX_FRAME_BUDGET = 8;
    constexpr int PLAYER_MOVEMENT_MAX_STEPS_PER_UPDATE = 2;
    constexpr int PLAYER_MOVEMENT_MAX_FRAME_BUDGET = 8;
    // Keep player movement fixed-step per update so walk speed stays intuitive,
    // but carry a small catch-up budget across missed frames so brief heavy-room
    // spikes don't permanently shave movement distance.
    constexpr bn::fixed MOVE_SPEED = bn::fixed(0.5);
    constexpr int DOOR_TRANSITION_DURATION_FRAMES = 16;
    constexpr int SPAWN_CORNER_INDEX = 2;
    constexpr int SPAWN_PLAYER_DIR = 3;
    constexpr bool SPAWN_PLAYER_FACING_LEFT = false;
    constexpr int SPAWN_ROOM_ID = 0;
    constexpr int SHAPE_OAM_START_INDEX = 64;
    constexpr int SHAPE_RESERVED_HANDLES = fr::models_3d::required_reserved_sprite_handles();
    constexpr int ROOM_VIEWER_RESERVED_HANDLES = 0;
    static_assert(ROOM_VIEWER_RESERVED_HANDLES <= bn::hw::sprites::count(),
                  "Room viewer reserved handles overflow OAM");

    constexpr int degrees_to_angle(int degrees)
    {
        return (degrees * 65536) / 360;
    }

    struct render_sweep_preset
    {
        int fx;
        int fy;
        int view_angle;
        int dir;
        bool facing_left;
    };

    constexpr render_sweep_preset render_sweep_presets[] = {
        { -28, 28, degrees_to_angle(180), 4, false },
        { -12, 28, degrees_to_angle(180), 4, false },
        {   4, 28, degrees_to_angle(180), 4, false },
        {  20, 28, degrees_to_angle(180), 4, false },
        { -28, 16, degrees_to_angle(180), 4, false },
        { -12, 16, degrees_to_angle(180), 4, false },
        {   4, 16, degrees_to_angle(180), 4, false },
        {  20, 16, degrees_to_angle(180), 4, false },
        { -20, 10, degrees_to_angle(168), 4, false },
        {  -4, 10, degrees_to_angle(168), 4, false },
        {  12, 10, degrees_to_angle(168), 4, false },
        {  28, 10, degrees_to_angle(168), 4, false },
        { -20, 10, degrees_to_angle(192), 0, false },
        {  -4, 10, degrees_to_angle(192), 0, false },
        {  12, 10, degrees_to_angle(192), 0, false },
        {  28, 10, degrees_to_angle(192), 0, false },
    };

    constexpr int render_sweep_presets_count = sizeof(render_sweep_presets) / sizeof(render_sweep_presets[0]);

    class textured_triangle
    {
    public:
        explicit textured_triangle(const bn::sprite_item& sprite_item) :
            _sprite(sprite_item.create_sprite_optional(0, 0)),
            _affine_mat(bn::sprite_affine_mat_ptr::create()),
            _half_size(sprite_item.shape_size().width() / 2)
        {
            if(_sprite)
            {
                _sprite->set_affine_mat(_affine_mat);
                // Keep paintings above room HDMA sprites without needing manual OAM writes.
                _sprite->set_bg_priority(1);
                _sprite->set_visible(false);
            }
        }

        void set_visible(bool visible)
        {
            if(_sprite)
            {
                _sprite->set_visible(visible);
            }
        }

        bool set_points(const bn::point& p0, const bn::point& p1, const bn::point& p2)
        {
            if(! _sprite)
            {
                return false;
            }

            _p0 = p0;
            _p1 = p1;
            _p2 = p2;
            return _update();
        }

    private:
        bn::optional<bn::sprite_ptr> _sprite;
        bn::sprite_affine_mat_ptr _affine_mat;
        bn::point _p0;
        bn::point _p1;
        bn::point _p2;
        int _half_size;

        bool _update()
        {
            switch(_half_size)
            {
                case 4:
                    return _update_impl<4>();

                case 8:
                    return _update_impl<8>();

                case 16:
                    return _update_impl<16>();

                case 32:
                    return _update_impl<32>();

                default:
                    BN_ERROR("Invalid textured triangle half size: ", _half_size);
                    set_visible(false);
                    return false;
            }
        }

        template<int half_size>
        bool _update_impl()
        {
            auto abs_value = [](int value) {
                return value >= 0 ? value : -value;
            };
            int x0 = _p0.x();
            int y0 = _p0.y();
            int x1 = _p1.x();
            int y1 = _p1.y();
            int x2 = _p2.x();
            int y2 = _p2.y();
            int affine_divisor = x0 * y1 - x0 * y2 - x1 * y0 + x1 * y2 + x2 * y0 - x2 * y1;

            // Keep a stable triangle winding so the transparent half of the
            // texture doesn't flip into view when projected winding changes.
            if(affine_divisor > 0)
            {
                bn::swap(x1, x2);
                bn::swap(y1, y2);
                affine_divisor = x0 * y1 - x0 * y2 - x1 * y0 + x1 * y2 + x2 * y0 - x2 * y1;
            }

            // Very thin/skewed screen-space triangles explode affine params and cause scanline artifacts.
            int abs_affine_divisor = affine_divisor >= 0 ? affine_divisor : -affine_divisor;

            if(! affine_divisor || abs_affine_divisor < 32)
            {
                set_visible(false);
                return false;
            }

            int pa = (-512 * half_size * y1 + 512 * half_size * y2 + 256 * y1 - 256 * y2) / affine_divisor;
            int pb = (256 * (2 * half_size * x1 - 2 * half_size * x2 - x1 + x2)) / affine_divisor;
            int pc = (-512 * half_size * y0 + 512 * half_size * y1 + 256 * y0 - 256 * y1) / affine_divisor;
            int pd = (256 * (2 * half_size * x0 - 2 * half_size * x1 - x0 + x1)) / affine_divisor;
            // Reject very stretched affine triangles well before hardware register
            // overflow so they don't degrade into visible horizontal banding.
            if(abs_value(pa) > PAINTING_MAX_AFFINE_REGISTER || abs_value(pb) > PAINTING_MAX_AFFINE_REGISTER ||
               abs_value(pc) > PAINTING_MAX_AFFINE_REGISTER || abs_value(pd) > PAINTING_MAX_AFFINE_REGISTER)
            {
                set_visible(false);
                return false;
            }

            bn::affine_mat_attributes attributes;
            attributes.unsafe_set_register_values(pa, pb, pc, pd);
            _affine_mat.set_attributes(attributes);

            int u0 = -half_size;
            int v0 = half_size;
            int u1 = half_size;
            int v1 = half_size;
            int u2 = half_size;
            int v2 = -half_size;

            int u0v1 = u0 * v1;
            int u0v2 = u0 * v2;
            int u1v0 = u1 * v0;
            int u1v2 = u1 * v2;
            int u2v0 = u2 * v0;
            int u2v1 = u2 * v1;

            int delta_x = (u0v1 * x2) + (u2v0 * x1) + (x0 * u1v2) - (x0 * u2v1) - (u1v0 * x2) - (u0v2 * x1);
            int delta_y = (u0v1 * y2) + (y1 * u2v0) + (y0 * u1v2) - (y0 * u2v1) - (u1v0 * y2) - (u0v2 * y1);
            int position_divisor = u0v1 + u2v0 + u1v2 - u2v1 - u1v0 - u0v2;
            int sprite_x = delta_x / position_divisor;
            int sprite_y = delta_y / position_divisor;

            // Affine triangle centers can land well outside the visible screen
            // while the transformed triangle itself is still on screen, so only
            // reject clearly nonsensical wrapped positions.
            if(abs_value(sprite_x) > 1024 || abs_value(sprite_y) > 1024)
            {
                set_visible(false);
                return false;
            }

            _sprite->set_position(sprite_x, sprite_y);
            set_visible(true);
            return true;
        }
    };

    class textured_quad
    {
    public:
        textured_quad(const bn::sprite_item& top_item, const bn::sprite_item& bottom_item) :
            _top(top_item),
            _bottom(bottom_item)
        {
        }

        void set_visible(bool visible)
        {
            _top.set_visible(visible);
            _bottom.set_visible(visible);
        }

        bool set_points(const bn::point& p0, const bn::point& p1, const bn::point& p2, const bn::point& p3)
        {
            bool top_ok = _top.set_points(p2, p3, p0);
            bool bottom_ok = _bottom.set_points(p0, p1, p2);

            if(! top_ok || ! bottom_ok)
            {
                set_visible(false);
                return false;
            }

            return true;
        }

    private:
        textured_triangle _top;
        textured_triangle _bottom;
    };

    enum decor_flags
    {
        decor_none = 0,
        decor_books = 1 << 0,
        decor_potted_plant = 1 << 1,
    };

    constexpr int room_decor_flags[NUM_ROOMS] = {
        decor_none,   // Room 0
        decor_books,  // Room 1
    };

    bool overlaps_any_furniture(int room_id, bn::fixed px, bn::fixed py)
    {
        int decor = room_decor_flags[room_id];

        if(decor & decor_books)
        {
            if(bn::abs(px - BOOKS_FX) < BOOKS_HW && bn::abs(py - BOOKS_FY) < BOOKS_HD)
                return true;
        }

        if(decor & decor_potted_plant)
        {
            if(bn::abs(px - POTTED_PLANT_FX) < POTTED_PLANT_HW &&
               bn::abs(py - POTTED_PLANT_FY) < POTTED_PLANT_HD)
                return true;
        }

        return false;
    }

    // Two-room layout:
    //   [ Room 1: square side room ]
    //                |
    //   [ Room 0: long spawn room ]
    // The small room sits above-right of the spawn room and connects through
    // one doorway on the spawn room's long back wall.

    int int_abs(int value)
    {
        return value >= 0 ? value : -value;
    }

    // Runtime room vertices are emitted with ROOM_SCALE=2 (generate_level_headers.py).
    // Keep placement centers in the same scaled engine-space units.
    constexpr bn::fixed room_center_x_values[NUM_ROOMS] = {
        bn::fixed(0), bn::fixed(60)
    };
    constexpr bn::fixed room_center_y_values[NUM_ROOMS] = {
        bn::fixed(0), bn::fixed(120)
    };
    constexpr bn::fixed room_half_extent_x_values[NUM_ROOMS] = {
        bn::fixed(90), bn::fixed(60)
    };
    constexpr bn::fixed room_half_extent_y_values[NUM_ROOMS] = {
        bn::fixed(60), bn::fixed(60)
    };
    constexpr bn::fixed ROOM_PLAYABLE_EDGE_INSET = bn::fixed(5);
    constexpr bn::fixed SHARED_DOOR_WORLD_X = bn::fixed(45);

    bn::fixed room_center_x(int room_id)
    {
        return room_center_x_values[room_id];
    }

    bn::fixed room_center_y(int room_id)
    {
        return room_center_y_values[room_id];
    }

    bn::fixed room_half_extent_x(int room_id)
    {
        return room_half_extent_x_values[room_id];
    }

    bn::fixed room_half_extent_y(int room_id)
    {
        return room_half_extent_y_values[room_id];
    }

    bn::fixed room_floor_min_x(int room_id)
    {
        return -room_half_extent_x(room_id) + ROOM_PLAYABLE_EDGE_INSET;
    }

    bn::fixed room_floor_max_x(int room_id)
    {
        return room_half_extent_x(room_id) - ROOM_PLAYABLE_EDGE_INSET;
    }

    bn::fixed room_floor_min_y(int room_id)
    {
        return -room_half_extent_y(room_id) + ROOM_PLAYABLE_EDGE_INSET;
    }

    bn::fixed room_floor_max_y(int room_id)
    {
        return room_half_extent_y(room_id) - ROOM_PLAYABLE_EDGE_INSET;
    }

    enum class door_direction
    {
        east,
        west,
        south,
        north
    };

    int neighbor_room_for_door(int room_id, door_direction direction)
    {
        switch(direction)
        {
            case door_direction::south:
                return room_id == 0 ? 1 : -1;

            case door_direction::north:
                return room_id == 1 ? 0 : -1;

            case door_direction::east:
            case door_direction::west:
            default:
                return -1;
        }
    }

    // Keep doorway centers aligned between rooms with different shell sizes.
    bn::fixed aligned_door_center_offset(int room_id, door_direction direction)
    {
        int neighbor_room = neighbor_room_for_door(room_id, direction);
        if(neighbor_room < 0)
        {
            return 0;
        }

        if(direction == door_direction::north || direction == door_direction::south)
        {
            return SHARED_DOOR_WORLD_X - room_center_x(room_id);
        }

        bn::fixed shared_world_y = (room_center_y(room_id) + room_center_y(neighbor_room)) / 2;
        return shared_world_y - room_center_y(room_id);
    }

    // Check if a door transition should occur and return the new room id (-1 if no transition)
    // Also sets new_local_x/y for the player position in the new room
    int check_door_transition(int current_room, bn::fixed local_x, bn::fixed local_y,
                              bn::fixed& new_local_x, bn::fixed& new_local_y)
    {
        int east_room = neighbor_room_for_door(current_room, door_direction::east);
        bn::fixed east_door_center = aligned_door_center_offset(current_room, door_direction::east);
        if(east_room >= 0 && local_x >= room_floor_max_x(current_room) &&
           bn::abs(local_y - east_door_center) <= DOOR_HALF_WIDTH)
        {
            new_local_x = room_floor_min_x(east_room);
            bn::fixed shared_world_y = room_center_y(current_room) + local_y;
            new_local_y = bn::clamp(shared_world_y - room_center_y(east_room),
                                    room_floor_min_y(east_room), room_floor_max_y(east_room));
            return east_room;
        }

        int west_room = neighbor_room_for_door(current_room, door_direction::west);
        bn::fixed west_door_center = aligned_door_center_offset(current_room, door_direction::west);
        if(west_room >= 0 && local_x <= room_floor_min_x(current_room) &&
           bn::abs(local_y - west_door_center) <= DOOR_HALF_WIDTH)
        {
            new_local_x = room_floor_max_x(west_room);
            bn::fixed shared_world_y = room_center_y(current_room) + local_y;
            new_local_y = bn::clamp(shared_world_y - room_center_y(west_room),
                                    room_floor_min_y(west_room), room_floor_max_y(west_room));
            return west_room;
        }

        int south_room = neighbor_room_for_door(current_room, door_direction::south);
        bn::fixed south_door_center = aligned_door_center_offset(current_room, door_direction::south);
        if(south_room >= 0 && local_y >= room_floor_max_y(current_room) &&
           bn::abs(local_x - south_door_center) <= DOOR_HALF_WIDTH)
        {
            bn::fixed shared_world_x = room_center_x(current_room) + local_x;
            new_local_x = bn::clamp(shared_world_x - room_center_x(south_room),
                                    room_floor_min_x(south_room), room_floor_max_x(south_room));
            new_local_y = room_floor_min_y(south_room);
            return south_room;
        }

        int north_room = neighbor_room_for_door(current_room, door_direction::north);
        bn::fixed north_door_center = aligned_door_center_offset(current_room, door_direction::north);
        if(north_room >= 0 && local_y <= room_floor_min_y(current_room) &&
           bn::abs(local_x - north_door_center) <= DOOR_HALF_WIDTH)
        {
            bn::fixed shared_world_x = room_center_x(current_room) + local_x;
            new_local_x = bn::clamp(shared_world_x - room_center_x(north_room),
                                    room_floor_min_x(north_room), room_floor_max_x(north_room));
            new_local_y = room_floor_max_y(north_room);
            return north_room;
        }

        return -1;
    }

    bool near_door_approach(int current_room, bn::fixed local_x, bn::fixed local_y)
    {
        bn::fixed door_lane_half_width = DOOR_HALF_WIDTH + DOOR_APPROACH_LANE_MARGIN;

        int east_room = neighbor_room_for_door(current_room, door_direction::east);
        bn::fixed east_door_center = aligned_door_center_offset(current_room, door_direction::east);
        if(east_room >= 0 &&
           local_x >= room_floor_max_x(current_room) - DOOR_APPROACH_EDGE_MARGIN &&
           bn::abs(local_y - east_door_center) <= door_lane_half_width)
        {
            return true;
        }

        int west_room = neighbor_room_for_door(current_room, door_direction::west);
        bn::fixed west_door_center = aligned_door_center_offset(current_room, door_direction::west);
        if(west_room >= 0 &&
           local_x <= room_floor_min_x(current_room) + DOOR_APPROACH_EDGE_MARGIN &&
           bn::abs(local_y - west_door_center) <= door_lane_half_width)
        {
            return true;
        }

        int south_room = neighbor_room_for_door(current_room, door_direction::south);
        bn::fixed south_door_center = aligned_door_center_offset(current_room, door_direction::south);
        if(south_room >= 0 &&
           local_y >= room_floor_max_y(current_room) - DOOR_APPROACH_EDGE_MARGIN &&
           bn::abs(local_x - south_door_center) <= door_lane_half_width)
        {
            return true;
        }

        int north_room = neighbor_room_for_door(current_room, door_direction::north);
        bn::fixed north_door_center = aligned_door_center_offset(current_room, door_direction::north);
        if(north_room >= 0 &&
           local_y <= room_floor_min_y(current_room) + DOOR_APPROACH_EDGE_MARGIN &&
           bn::abs(local_x - north_door_center) <= door_lane_half_width)
        {
            return true;
        }

        return false;
    }

    const fr::model_3d_item& get_room_model(int room_id)
    {
        switch(room_id)
        {
            case 0:  return str::model_3d_items::room_0;
            case 1:  return str::model_3d_items::room_1;
            default: return str::model_3d_items::room_0;
        }
    }

}

namespace str
{

RoomViewer::RoomViewer() {}

void RoomViewer::_update_player_anim_tiles(fr::sprite_3d_item& item, bool moving, int dir, bool facing_left,
                                           int frame_advance)
{
    const bn::sprite_item& player_sprite_item =
        moving ? bn::sprite_items::player_walk : bn::sprite_items::player_idle;
    int frames_per_angle = moving ? PLAYER_WALK_FRAMES_PER_ANGLE : PLAYER_IDLE_FRAMES_PER_ANGLE;

    if(_player_moving != moving || _player_dir != dir || _player_facing_left != facing_left)
    {
        _player_moving = moving;
        _player_dir = dir;
        _player_facing_left = facing_left;
        _anim_frame_counter = 0;
        item.palette().set_colors(player_sprite_item.palette_item());
    }

    int base_frame = player_angle_row(dir, facing_left) * frames_per_angle;
    int frame_in_anim = (_anim_frame_counter / PLAYER_ANIM_SPEED) % frames_per_angle;
    int tile_index = base_frame + frame_in_anim;

    item.tiles().set_tiles_ref(player_sprite_item.tiles_item(), tile_index);

    _anim_frame_counter += frame_advance;
}

str::Scene RoomViewer::execute()
{
    bn::bg_palettes::set_transparent_color(bn::color(2, 2, 4));
    _models.set_shape_oam_start_index(SHAPE_OAM_START_INDEX);
    int required_reserved_sprite_handles = ROOM_VIEWER_RESERVED_HANDLES;

    if(bn::sprites::reserved_handles_count() < required_reserved_sprite_handles)
    {
        bn::sprites::set_reserved_handles_count(required_reserved_sprite_handles);
    }

    // Keep spawn deterministic for debugging: start from the painting-facing view.
    _corner_index = SPAWN_CORNER_INDEX;
    _player_dir = SPAWN_PLAYER_DIR;
    _player_facing_left = SPAWN_PLAYER_FACING_LEFT;
    _player_moving = false;
    _debug_mode = ENABLE_RENDER_SWEEP_DEBUG;

    int current_room = SPAWN_ROOM_ID;
    _models.load_colors(str::model_3d_items::room_model_colors);

    fr::model_3d* room_models[NUM_ROOMS] = {};
    fr::model_3d* books_ptr = nullptr;
    fr::model_3d* potted_plant_ptr = nullptr;
    fr::model_3d* transition_books_ptr = nullptr;
    fr::model_3d* transition_potted_plant_ptr = nullptr;

    fr::point_3d room_base_pos(0, 96, 8);
    bool player_sprite_created = false;

    corner_matrix all_corners[4];
    compute_corner_matrices(all_corners);
    const corner_matrix base_corner = all_corners[0];

    int current_view_angle = -_corner_index * QUARTER_TURN_ANGLE;
    int last_oriented_view_angle = current_view_angle;
    int target_view_angle = current_view_angle;
    bn::fixed camera_turn_velocity = 0;
    int camera_initial_lock_frames = CAMERA_INITIAL_LOCK_FRAMES;
    int painting_update_cooldown_frames = 0;
    int idle_recenter_timer = 0;
    int player_world_linear_dir =
        wrap_linear8(dir_to_linear8(_player_dir, _player_facing_left) - view_angle_steps_8(current_view_angle));
    bn::fixed world_anchor_x = room_center_x(current_room);
    bn::fixed world_anchor_y = room_center_y(current_room);
    bn::fixed camera_offset_x = 0;
    bn::fixed camera_offset_y = 0;
    bn::fixed lookahead_x = 0;
    bn::fixed lookahead_y = 0;
    bn::fixed prev_player_fx = _player_fx;
    bn::fixed prev_player_fy = _player_fy;
    bool camera_follow_moved = false;
    bool door_transition_active = false;
    int door_transition_elapsed = 0;
    int door_transition_target_room = current_room;
    int door_transition_furniture_room = -1;
    room_preview_mode preview_mode = room_preview_mode::off;
    bool debug_hide_room_models = false;
    int preview_mode_low_fps_samples = 0;
    int preview_mode_high_fps_samples = 0;
    int preview_mode_change_cooldown_frames = 0;
    int render_sweep_pose_index = ENABLE_RENDER_SWEEP_DEBUG ? 0 : -1;
    bn::fixed door_transition_start_global_x = 0;
    bn::fixed door_transition_start_global_y = 0;
    bn::fixed door_transition_target_global_x = 0;
    bn::fixed door_transition_target_global_y = 0;
    bn::fixed door_transition_current_global_x = 0;
    bn::fixed door_transition_current_global_y = 0;
    bn::fixed door_transition_start_anchor_x = world_anchor_x;
    bn::fixed door_transition_start_anchor_y = world_anchor_y;
    bn::fixed door_transition_target_anchor_x = world_anchor_x;
    bn::fixed door_transition_target_anchor_y = world_anchor_y;
    bn::fixed door_transition_target_local_x = 0;
    bn::fixed door_transition_target_local_y = 0;

    str::Minimap* minimap = ENABLE_MINIMAP ? new str::Minimap() : nullptr;
    auto set_model_rotation = [](fr::model_3d& m, const corner_matrix& cm) {
        m.set_rotation_matrix(
            cm.r00, cm.r01, cm.r02,
            cm.r10, cm.r11, cm.r12,
            cm.r20, cm.r21, cm.r22);
    };

    auto transform_global_point = [&](const corner_matrix& cm, bn::fixed gx, bn::fixed gy, bn::fixed gz) {
        bn::fixed rx = gx.unsafe_multiplication(cm.r00) +
                       gy.unsafe_multiplication(cm.r01) +
                       gz.unsafe_multiplication(cm.r02);
        bn::fixed ry = gx.unsafe_multiplication(cm.r10) +
                       gy.unsafe_multiplication(cm.r11) +
                       gz.unsafe_multiplication(cm.r12);
        bn::fixed rz = gx.unsafe_multiplication(cm.r20) +
                       gy.unsafe_multiplication(cm.r21) +
                       gz.unsafe_multiplication(cm.r22);
        return fr::point_3d(
            room_base_pos.x() + rx,
            room_base_pos.y() + ry,
            room_base_pos.z() + rz);
    };

    auto try_create_dynamic_model = [&](const fr::model_3d_item& model_item) -> fr::model_3d* {
        int reserved_vertices = player_sprite_created ? 0 : 1;
        int model_vertices_count = model_item.vertices().size();
        int model_faces_count = model_item.faces().size();

        if(_models.vertices_count() + model_vertices_count + reserved_vertices > fr::models_3d::max_vertices())
        {
            return nullptr;
        }

        if(_models.faces_count() + model_faces_count + reserved_vertices > fr::models_3d::max_faces())
        {
            return nullptr;
        }

        return &_models.create_dynamic_model(model_item);
    };

    auto sync_room_models = [&]() {
        bool changed = false;
        bool should_exist[NUM_ROOMS] = {};

        if(debug_hide_room_models)
        {
            for(int room_id = 0; room_id < NUM_ROOMS; ++room_id)
            {
                if(room_models[room_id])
                {
                    _models.destroy_dynamic_model(*room_models[room_id]);
                    room_models[room_id] = nullptr;
                    changed = true;
                }
            }

            return changed;
        }

        should_exist[current_room] = true;

        if(door_transition_active)
        {
            should_exist[door_transition_target_room] = true;
        }

        for(int room_id = 0; room_id < NUM_ROOMS; ++room_id)
        {
            if(!should_exist[room_id] && room_models[room_id])
            {
                _models.destroy_dynamic_model(*room_models[room_id]);
                room_models[room_id] = nullptr;
                changed = true;
            }
        }

        auto ensure_room_model = [&](int room_id) {
            if(!should_exist[room_id])
            {
                return;
            }

            fr::model_3d* room_model = room_models[room_id];

            if(!room_model)
            {
                room_model = try_create_dynamic_model(get_room_model(room_id));
                room_models[room_id] = room_model;

                if(room_model)
                {
                    changed = true;
                }
                else
                {
                    return;
                }
            }

            bool room_perspective_mode = room_id == current_room;
            room_model->set_mode(
                room_perspective_mode ? fr::model_3d::layering_mode::room_perspective :
                                        fr::model_3d::layering_mode::room_floor_only);
            room_model->set_depth_bias(
                room_id == current_room ? 0 : ADJACENT_ROOM_DEPTH_BIAS);
            // Preview rooms use floor-only mode; double-sided shell faces are only needed
            // for the current room shell.
            room_model->set_double_sided(room_perspective_mode);
        };

        // Keep only the active room loaded, plus the destination room while a door transition runs.
        ensure_room_model(current_room);

        if(door_transition_active && door_transition_target_room != current_room)
        {
            ensure_room_model(door_transition_target_room);
        }

        return changed;
    };

    auto configure_room_decor_model =
        [&](bool needed, const fr::model_3d_item& model_item, int depth_bias, fr::model_3d*& room_model_ptr) {
        if(needed)
        {
            if(!room_model_ptr)
            {
                room_model_ptr = try_create_dynamic_model(model_item);
            }

            if(room_model_ptr)
            {
                room_model_ptr->set_mode(fr::model_3d::layering_mode::none);
                room_model_ptr->set_depth_bias(depth_bias);
                room_model_ptr->set_double_sided(false);
            }
        }
        else if(room_model_ptr)
        {
            _models.destroy_dynamic_model(*room_model_ptr);
            room_model_ptr = nullptr;
        }
    };

    auto ensure_room_decor_models = [&](int room_id, int depth_bias, fr::model_3d*& room_books_ptr,
                                        fr::model_3d*& room_potted_plant_ptr) {
        int decor = room_decor_flags[room_id];
        configure_room_decor_model(
            decor & decor_books, str::model_3d_items::books, depth_bias, room_books_ptr);
        configure_room_decor_model(
            decor & decor_potted_plant, str::model_3d_items::potted_plant, depth_bias, room_potted_plant_ptr);
    };

    auto clear_room_decor_models = [&](fr::model_3d*& room_books_ptr, fr::model_3d*& room_potted_plant_ptr) {
        if(room_potted_plant_ptr)
        {
            _models.destroy_dynamic_model(*room_potted_plant_ptr);
            room_potted_plant_ptr = nullptr;
        }

        if(room_books_ptr)
        {
            _models.destroy_dynamic_model(*room_books_ptr);
            room_books_ptr = nullptr;
        }
    };

    auto ensure_decor_models = [&]() {
        if(debug_hide_room_models)
        {
            clear_room_decor_models(books_ptr, potted_plant_ptr);
            clear_room_decor_models(transition_books_ptr, transition_potted_plant_ptr);
            door_transition_furniture_room = -1;
            return;
        }

        ensure_room_decor_models(current_room, 0, books_ptr, potted_plant_ptr);

        if(door_transition_furniture_room >= 0 && door_transition_furniture_room != current_room)
        {
            ensure_room_decor_models(
                door_transition_furniture_room,
                TRANSITION_DECOR_DEPTH_BIAS,
                transition_books_ptr,
                transition_potted_plant_ptr);
        }
        else
        {
            clear_room_decor_models(transition_books_ptr, transition_potted_plant_ptr);
            door_transition_furniture_room = -1;
        }
    };

    fr::sprite_3d* npc_sprite_a_ptr = nullptr;
    fr::sprite_3d* npc_sprite_b_ptr = nullptr;

    auto update_all_orientations = [&]() {
        corner_matrix cm = rotate_corner_matrix(base_corner, current_view_angle);

        for(int room_id = 0; room_id < NUM_ROOMS; ++room_id)
        {
            fr::model_3d* room_model = room_models[room_id];

            if(room_model)
            {
                set_model_rotation(*room_model, cm);
                room_model->set_position(
                    transform_global_point(cm,
                                           room_center_x(room_id) - world_anchor_x,
                                           room_center_y(room_id) - world_anchor_y,
                                           0));
            }
        }

        if(books_ptr)
        {
            set_model_rotation(*books_ptr, cm);
            books_ptr->set_position(
                transform_global_point(cm,
                                       room_center_x(current_room) + BOOKS_FX - world_anchor_x,
                                       room_center_y(current_room) + BOOKS_FY - world_anchor_y,
                                       0));
        }

        if(potted_plant_ptr)
        {
            set_model_rotation(*potted_plant_ptr, cm);
            potted_plant_ptr->set_position(
                transform_global_point(cm,
                                       room_center_x(current_room) + POTTED_PLANT_FX - world_anchor_x,
                                       room_center_y(current_room) + POTTED_PLANT_FY - world_anchor_y,
                                       0));
        }

        if(door_transition_furniture_room >= 0)
        {
            if(transition_books_ptr)
            {
                set_model_rotation(*transition_books_ptr, cm);
                transition_books_ptr->set_position(
                    transform_global_point(cm,
                                           room_center_x(door_transition_furniture_room) + BOOKS_FX - world_anchor_x,
                                           room_center_y(door_transition_furniture_room) + BOOKS_FY - world_anchor_y,
                                           0));
            }

            if(transition_potted_plant_ptr)
            {
                set_model_rotation(*transition_potted_plant_ptr, cm);
                transition_potted_plant_ptr->set_position(
                    transform_global_point(cm,
                                           room_center_x(door_transition_furniture_room) + POTTED_PLANT_FX -
                                               world_anchor_x,
                                           room_center_y(door_transition_furniture_room) + POTTED_PLANT_FY -
                                               world_anchor_y,
                                           0));
            }

        }
    };

    bool paintings_need_update = ENABLE_PAINTING_QUADS;
    auto update_orientations_and_paintings = [&]() {
        update_all_orientations();
        last_oriented_view_angle = current_view_angle;
        paintings_need_update = ENABLE_PAINTING_QUADS;
    };

    sync_room_models();
    ensure_decor_models();
    update_orientations_and_paintings();

    auto transform_room_local_point = [&](const corner_matrix& cm, int room_id,
                                          bn::fixed anchor_x, bn::fixed anchor_y,
                                          bn::fixed local_x, bn::fixed local_y, bn::fixed local_z) {
        return transform_global_point(cm,
                                      room_center_x(room_id) + local_x - anchor_x,
                                      room_center_y(room_id) + local_y - anchor_y,
                                      local_z);
    };

    auto transform_room_local_vector = [&](const corner_matrix& cm,
                                           bn::fixed local_x, bn::fixed local_y, bn::fixed local_z) {
        return fr::point_3d(local_x * cm.r00 + local_y * cm.r01 + local_z * cm.r02,
                            local_x * cm.r10 + local_y * cm.r11 + local_z * cm.r12,
                            local_x * cm.r20 + local_y * cm.r21 + local_z * cm.r22);
    };

    auto project_point_for_camera_distance = [&](const fr::point_3d& point, bn::fixed camera_y, bn::point& output) {
        constexpr int focal_length_shift = fr::constants_3d::focal_length_shift;
        constexpr int near_plane = 24 * 256 * 16;

        bn::fixed vry = point.y() - camera_y;
        int vcz = -vry.data();

        if(vcz < near_plane)
        {
            return false;
        }

        bn::fixed vrx = point.x() / 16;
        bn::fixed vrz = point.z() / 16;
        int vcx = (vrx.unsafe_multiplication(_camera.u().x()) + vrz.unsafe_multiplication(_camera.u().z())).data();
        int vcy = -(vrx.unsafe_multiplication(_camera.v().x()) + vrz.unsafe_multiplication(_camera.v().z())).data();

        int scale = int((fr::div_lut_ptr[vcz >> 10] << (focal_length_shift - 8)) >> 6);
        output.set_x((vcx * scale) >> 16);
        output.set_y((vcy * scale) >> 16);
        return true;
    };

    auto room_side_is_visible = [&](const corner_matrix& cm, int room_id,
                                    bn::fixed anchor_x, bn::fixed anchor_y,
                                    bn::fixed camera_y,
                                    bn::fixed center_local_x, bn::fixed center_local_y,
                                    bn::fixed normal_local_x, bn::fixed normal_local_y) {
        fr::point_3d center = transform_room_local_point(
            cm, room_id, anchor_x, anchor_y, center_local_x, center_local_y, ROOM_WALL_TOP_Z / 2);
        fr::point_3d normal = transform_room_local_vector(cm, normal_local_x, normal_local_y, 0);
        fr::point_3d to_camera(-center.x(), camera_y - center.y(), -center.z());
        bn::fixed facing_dot = normal.x() * to_camera.x() +
                               normal.y() * to_camera.y() +
                               normal.z() * to_camera.z();
        return facing_dot > 0;
    };

    auto room_fits_camera_distance = [&](const corner_matrix& cm, int room_id,
                                         bn::fixed anchor_x, bn::fixed anchor_y,
                                         bn::fixed camera_y) {
        constexpr int max_abs_x = (bn::display::width() / 2) - CAMERA_AUTO_FIT_MARGIN_X;
        constexpr int max_abs_y = (bn::display::height() / 2) - CAMERA_AUTO_FIT_MARGIN_Y;

        auto point_fits = [&](bn::fixed local_x, bn::fixed local_y, bn::fixed local_z) {
            bn::point screen_point;
            fr::point_3d point = transform_room_local_point(
                cm, room_id, anchor_x, anchor_y, local_x, local_y, local_z);

            if(! project_point_for_camera_distance(point, camera_y, screen_point))
            {
                return false;
            }

            return int_abs(screen_point.x()) <= max_abs_x &&
                   int_abs(screen_point.y()) <= max_abs_y;
        };

        bn::fixed room_half_x = room_half_extent_x(room_id);
        bn::fixed room_half_y = room_half_extent_y(room_id);

        if(! point_fits(-room_half_x, -room_half_y, 0) ||
           ! point_fits(-room_half_x, room_half_y, 0) ||
           ! point_fits(room_half_x, -room_half_y, 0) ||
           ! point_fits(room_half_x, room_half_y, 0))
        {
            return false;
        }

        if(room_side_is_visible(cm, room_id, anchor_x, anchor_y, camera_y, 0, -room_half_y, 0, 1) &&
           (! point_fits(-room_half_x, -room_half_y, ROOM_WALL_TOP_Z) ||
            ! point_fits(room_half_x, -room_half_y, ROOM_WALL_TOP_Z)))
        {
            return false;
        }

        if(room_side_is_visible(cm, room_id, anchor_x, anchor_y, camera_y, 0, room_half_y, 0, -1) &&
           (! point_fits(-room_half_x, room_half_y, ROOM_WALL_TOP_Z) ||
            ! point_fits(room_half_x, room_half_y, ROOM_WALL_TOP_Z)))
        {
            return false;
        }

        if(room_side_is_visible(cm, room_id, anchor_x, anchor_y, camera_y, room_half_x, 0, -1, 0) &&
           (! point_fits(room_half_x, -room_half_y, ROOM_WALL_TOP_Z) ||
            ! point_fits(room_half_x, room_half_y, ROOM_WALL_TOP_Z)))
        {
            return false;
        }

        if(room_side_is_visible(cm, room_id, anchor_x, anchor_y, camera_y, -room_half_x, 0, 1, 0) &&
           (! point_fits(-room_half_x, -room_half_y, ROOM_WALL_TOP_Z) ||
            ! point_fits(-room_half_x, room_half_y, ROOM_WALL_TOP_Z)))
        {
            return false;
        }

        return true;
    };

    auto fitted_camera_distance_for_room = [&](const corner_matrix& cm, int room_id,
                                               bn::fixed anchor_x, bn::fixed anchor_y) {
        bn::fixed low = CAMERA_AUTO_FIT_MIN_DIST;
        bn::fixed high = CAMERA_AUTO_FIT_MAX_DIST;

        if(! room_fits_camera_distance(cm, room_id, anchor_x, anchor_y, high))
        {
            return high;
        }

        for(int index = 0; index < CAMERA_AUTO_FIT_BINARY_SEARCH_STEPS; ++index)
        {
            bn::fixed mid = (low + high) / 2;

            if(room_fits_camera_distance(cm, room_id, anchor_x, anchor_y, mid))
            {
                high = mid;
            }
            else
            {
                low = mid;
            }
        }

        return high;
    };

    bn::fixed cam_dist = 0;

    auto camera_distance_target = [&]() {
        corner_matrix cm = rotate_corner_matrix(base_corner, current_view_angle);
        bn::fixed target_dist =
            fitted_camera_distance_for_room(cm, current_room, world_anchor_x, world_anchor_y);

        if(door_transition_active)
        {
            target_dist = bn::max(
                target_dist,
                fitted_camera_distance_for_room(cm, door_transition_target_room, world_anchor_x, world_anchor_y));
        }

        target_dist = bn::max(CAMERA_AUTO_FIT_MIN_DIST, target_dist * CAMERA_AUTO_FIT_FILL_FACTOR);
        return target_dist;
    };

    auto update_camera = [&](bool allow_distance_change = true) {
        if(! allow_distance_change)
        {
            return;
        }

        bn::fixed target_dist = camera_distance_target();

        if(target_dist != cam_dist)
        {
            cam_dist = target_dist;
            _camera.set_position(fr::point_3d(0, cam_dist, 0));
            paintings_need_update = ENABLE_PAINTING_QUADS;
        }
    };

    auto update_camera_follow = [&]() {
        if(door_transition_active)
        {
            return;
        }

        bn::fixed vel_x = _player_fx - prev_player_fx;
        bn::fixed vel_y = _player_fy - prev_player_fy;
        bool player_moving = (vel_x != 0 || vel_y != 0);

        if(player_moving)
        {
            bn::fixed dir_x = vel_x > 0 ? bn::fixed(1) : (vel_x < 0 ? bn::fixed(-1) : bn::fixed(0));
            bn::fixed dir_y = vel_y > 0 ? bn::fixed(1) : (vel_y < 0 ? bn::fixed(-1) : bn::fixed(0));
            bn::fixed target_look_x = dir_x * CAMERA_LOOKAHEAD_X;
            bn::fixed target_look_y = dir_y * CAMERA_LOOKAHEAD_Y;
            lookahead_x += (target_look_x - lookahead_x) * CAMERA_LOOKAHEAD_SMOOTHING;
            lookahead_y += (target_look_y - lookahead_y) * CAMERA_LOOKAHEAD_SMOOTHING;
        }
        else
        {
            lookahead_x *= CAMERA_LOOKAHEAD_DECAY;
            lookahead_y *= CAMERA_LOOKAHEAD_DECAY;
            if(bn::abs(lookahead_x) < bn::fixed(0.5))
            {
                lookahead_x = 0;
            }
            if(bn::abs(lookahead_y) < bn::fixed(0.5))
            {
                lookahead_y = 0;
            }
        }

        bn::fixed raw_target_x = _player_fx + lookahead_x;
        bn::fixed raw_target_y = _player_fy + lookahead_y;

        bn::fixed diff_x = raw_target_x - camera_offset_x;
        bn::fixed diff_y = raw_target_y - camera_offset_y;
        bn::fixed camera_target_x = camera_offset_x;
        bn::fixed camera_target_y = camera_offset_y;

        if(bn::abs(diff_x) > CAMERA_DEADZONE_X)
        {
            camera_target_x = raw_target_x - (diff_x > 0 ? CAMERA_DEADZONE_X : -CAMERA_DEADZONE_X);
        }
        if(bn::abs(diff_y) > CAMERA_DEADZONE_Y)
        {
            camera_target_y = raw_target_y - (diff_y > 0 ? CAMERA_DEADZONE_Y : -CAMERA_DEADZONE_Y);
        }

        bn::fixed max_offset_x = room_half_extent_x(current_room) * bn::fixed(0.4);
        bn::fixed max_offset_y = room_half_extent_y(current_room) * bn::fixed(0.4);
        camera_target_x = bn::clamp(camera_target_x, -max_offset_x, max_offset_x);
        camera_target_y = bn::clamp(camera_target_y, -max_offset_y, max_offset_y);

        bn::fixed dist = bn::abs(camera_target_x - camera_offset_x) +
                         bn::abs(camera_target_y - camera_offset_y);
        bn::fixed speed;
        if(dist > 40)
        {
            speed = CAMERA_CATCH_UP_SPEED;
        }
        else if(!player_moving)
        {
            speed = CAMERA_SNAPBACK_SPEED;
        }
        else
        {
            speed = CAMERA_FOLLOW_SPEED;
        }

        bn::fixed old_offset_x = camera_offset_x;
        bn::fixed old_offset_y = camera_offset_y;
        camera_offset_x += (camera_target_x - camera_offset_x) * speed;
        camera_offset_y += (camera_target_y - camera_offset_y) * speed;

        camera_follow_moved = (camera_offset_x != old_offset_x || camera_offset_y != old_offset_y);

        world_anchor_x = room_center_x(current_room) + camera_offset_x;
        world_anchor_y = room_center_y(current_room) + camera_offset_y;

        prev_player_fx = _player_fx;
        prev_player_fy = _player_fy;
    };

    _camera.set_phi(0);
    update_camera();

    _player_fx = -20;
    _player_fy = 20;
    _player_fz = -10;

    fr::sprite_3d_item player_sprite_item(
        bn::sprite_items::player_idle,
        player_angle_row(SPAWN_PLAYER_DIR, SPAWN_PLAYER_FACING_LEFT) * PLAYER_IDLE_FRAMES_PER_ANGLE);
    fr::sprite_3d& player_sprite = _models.create_sprite(player_sprite_item);
    player_sprite_created = true;
    player_sprite.set_scale(PLAYER_SPRITE_SCALE);
    player_sprite.set_horizontal_flip(false);

    // --- NPC sprites for the two-room layout ---
    fr::sprite_3d_item npc_sprite_item_a(bn::sprite_items::villager, 0);
    fr::sprite_3d_item npc_sprite_item_b(bn::sprite_items::villager, 0);

    // The side-room NPC gets a distinct hat tint.
    npc_sprite_item_b.palette() = bn::sprite_items::villager.palette_item().create_new_palette();
    npc_sprite_item_b.palette().set_color(NPC_PALETTE_HAT_INDEX_0, npc_room_b_hat_color_0);
    npc_sprite_item_b.palette().set_color(NPC_PALETTE_HAT_INDEX_1, npc_room_b_hat_color_1);
    npc_sprite_item_b.palette().set_color(NPC_PALETTE_HAT_INDEX_2, npc_room_b_hat_color_2);

    npc_sprite_a_ptr = &_models.create_sprite(npc_sprite_item_a);
    npc_sprite_b_ptr = &_models.create_sprite(npc_sprite_item_b);
    npc_sprite_a_ptr->set_scale(2);
    npc_sprite_b_ptr->set_scale(2);
    int npc_anim_counter = 0;

    update_orientations_and_paintings();

    auto update_player_sprite_position = [&]() {
        corner_matrix cm = rotate_corner_matrix(base_corner, current_view_angle);
        bn::fixed player_global_x = room_center_x(current_room) + _player_fx - world_anchor_x;
        bn::fixed player_global_y = room_center_y(current_room) + _player_fy - world_anchor_y;

        if(door_transition_active)
        {
            player_global_x = door_transition_current_global_x - world_anchor_x;
            player_global_y = door_transition_current_global_y - world_anchor_y;
        }

        player_sprite.set_position(
            transform_global_point(cm, player_global_x, player_global_y, _player_fz));

        // --- NPC sprite positioning (every frame, with off-screen fallback) ---
        constexpr fr::point_3d offscreen_pos(0, -9999, 0);
        if(npc_sprite_a_ptr)
        {
            if(room_models[NPC_ROOM_A])
            {
                npc_sprite_a_ptr->set_position(
                    transform_global_point(cm,
                                           room_center_x(NPC_ROOM_A) + NPC_FX - world_anchor_x,
                                           room_center_y(NPC_ROOM_A) + NPC_FY - world_anchor_y,
                                           NPC_FZ));
            }
            else
            {
                npc_sprite_a_ptr->set_position(offscreen_pos);
            }
        }
        if(npc_sprite_b_ptr)
        {
            if(room_models[NPC_ROOM_B])
            {
                npc_sprite_b_ptr->set_position(
                    transform_global_point(cm,
                                           room_center_x(NPC_ROOM_B) + NPC_FX - world_anchor_x,
                                           room_center_y(NPC_ROOM_B) + NPC_FY - world_anchor_y,
                                           NPC_FZ));
            }
            else
            {
                npc_sprite_b_ptr->set_position(offscreen_pos);
            }
        }
    };

    textured_quad painting_a_quad(
        bn::sprite_items::mr_and_mrs_andrews_wall_top,
        bn::sprite_items::mr_and_mrs_andrews_wall_bottom);
    textured_quad painting_b_quad(
        bn::sprite_items::escaping_criticism_wall_top,
        bn::sprite_items::escaping_criticism_wall_bottom);

    auto project_point_to_screen = [&](const fr::point_3d& point, bn::point& output) {
        constexpr int focal_length_shift = fr::constants_3d::focal_length_shift;
        constexpr int near_plane = 24 * 256 * 16;

        const fr::point_3d& camera_position = _camera.position();
        bn::fixed vry = point.y() - camera_position.y();
        int vcz = -vry.data();

        if(vcz < near_plane)
        {
            return false;
        }

        bn::fixed vrx = (point.x() - camera_position.x()) / 16;
        bn::fixed vrz = (point.z() - camera_position.z()) / 16;
        int vcx = (vrx.unsafe_multiplication(_camera.u().x()) + vrz.unsafe_multiplication(_camera.u().z())).data();
        int vcy = -(vrx.unsafe_multiplication(_camera.v().x()) + vrz.unsafe_multiplication(_camera.v().z())).data();

        int scale = int((fr::div_lut_ptr[vcz >> 10] << (focal_length_shift - 8)) >> 6);
        output.set_x((vcx * scale) >> 16);
        output.set_y((vcy * scale) >> 16);
        return true;
    };

    auto update_painting_quads = [&]() {
        if(debug_hide_room_models || door_transition_active || ! room_models[current_room])
        {
            painting_a_quad.set_visible(false);
            painting_b_quad.set_visible(false);
            return;
        }

        corner_matrix cm = rotate_corner_matrix(base_corner, current_view_angle);
        bn::fixed room_half_x = room_half_extent_x(current_room);
        bn::fixed room_half_y = room_half_extent_y(current_room);
        bn::fixed a_center_x = PAINTING_A_CENTER_X;
        bn::fixed b_center_y = PAINTING_B_CENTER_Y;
        bn::fixed a_wall_y = room_half_y - PAINTING_WALL_INSET;
        bn::fixed b_wall_x = room_half_x - PAINTING_WALL_INSET;

        auto local_point_to_view = [&](bn::fixed local_x, bn::fixed local_y, bn::fixed local_z) {
            return transform_global_point(cm,
                                          room_center_x(current_room) + local_x - world_anchor_x,
                                          room_center_y(current_room) + local_y - world_anchor_y,
                                          local_z);
        };

        auto local_vector_to_view = [&](bn::fixed local_x, bn::fixed local_y, bn::fixed local_z) {
            return fr::point_3d(local_x * cm.r00 + local_y * cm.r01 + local_z * cm.r02,
                                local_x * cm.r10 + local_y * cm.r11 + local_z * cm.r12,
                                local_x * cm.r20 + local_y * cm.r21 + local_z * cm.r22);
        };

        auto is_front_facing = [&](const fr::point_3d& center, const fr::point_3d& normal) {
            fr::point_3d to_camera(_camera.position().x() - center.x(),
                                   _camera.position().y() - center.y(),
                                   _camera.position().z() - center.z());
            bn::fixed facing_dot = normal.x() * to_camera.x() +
                                   normal.y() * to_camera.y() +
                                   normal.z() * to_camera.z();
            // Require a stronger positive margin so near-grazing views don't leak through wall edges.
            return facing_dot > PAINTING_FACE_VISIBILITY_DOT_MIN;
        };

        auto quad_projection_is_stable = [&](const bn::point& p0, const bn::point& p1,
                                             const bn::point& p2, const bn::point& p3) {
            constexpr int max_abs_x = 168;
            constexpr int max_abs_y = 128;
            auto point_in_safe_range = [&](const bn::point& p) {
                return int_abs(p.x()) <= max_abs_x && int_abs(p.y()) <= max_abs_y;
            };

            if(! point_in_safe_range(p0) || ! point_in_safe_range(p1) ||
               ! point_in_safe_range(p2) || ! point_in_safe_range(p3))
            {
                return false;
            }

            auto tri_area2 = [](const bn::point& a, const bn::point& b, const bn::point& c) {
                return (b.x() - a.x()) * (c.y() - a.y()) - (b.y() - a.y()) * (c.x() - a.x());
            };
            auto tri_span_is_stable = [&](const bn::point& a, const bn::point& b, const bn::point& c) {
                int min_x = bn::min(a.x(), bn::min(b.x(), c.x()));
                int max_x = bn::max(a.x(), bn::max(b.x(), c.x()));
                int min_y = bn::min(a.y(), bn::min(b.y(), c.y()));
                int max_y = bn::max(a.y(), bn::max(b.y(), c.y()));
                return max_x - min_x >= PAINTING_MIN_TRI_SPAN &&
                       max_y - min_y >= PAINTING_MIN_TRI_SPAN;
            };

            int area_bottom = tri_area2(p0, p1, p2);
            int area_top = tri_area2(p0, p2, p3);

            if(int_abs(area_bottom) < PAINTING_MIN_TRI_AREA2 || int_abs(area_top) < PAINTING_MIN_TRI_AREA2)
            {
                return false;
            }

            if(! tri_span_is_stable(p0, p1, p2) || ! tri_span_is_stable(p0, p2, p3))
            {
                return false;
            }

            return (area_bottom > 0) == (area_top > 0);
        };

        bn::fixed a_z_top = PAINTING_Z_CENTER + PAINTING_A_HALF_HEIGHT;
        bn::fixed a_z_bottom = PAINTING_Z_CENTER - PAINTING_A_HALF_HEIGHT;
        bn::fixed b_z_top = PAINTING_Z_CENTER - PAINTING_B_HALF_HEIGHT;
        bn::fixed b_z_bottom = PAINTING_Z_CENTER + PAINTING_B_HALF_HEIGHT;

        // Painting A: on back wall (constant Y).
        bn::fixed a_left_x = a_center_x - PAINTING_A_HALF_WIDTH;
        bn::fixed a_right_x = a_center_x + PAINTING_A_HALF_WIDTH;

        fr::point_3d a0 = local_point_to_view(a_left_x, a_wall_y, a_z_bottom);
        fr::point_3d a1 = local_point_to_view(a_right_x, a_wall_y, a_z_bottom);
        fr::point_3d a2 = local_point_to_view(a_right_x, a_wall_y, a_z_top);
        fr::point_3d a3 = local_point_to_view(a_left_x, a_wall_y, a_z_top);

        fr::point_3d a_center = local_point_to_view(a_center_x, a_wall_y, PAINTING_Z_CENTER);
        fr::point_3d a_normal = local_vector_to_view(0, -1, 0);  // south wall interior normal

        bn::point a0_screen, a1_screen, a2_screen, a3_screen;
        bool a_visible = is_front_facing(a_center, a_normal) &&
                         project_point_to_screen(a0, a0_screen) &&
                         project_point_to_screen(a1, a1_screen) &&
                         project_point_to_screen(a2, a2_screen) &&
                         project_point_to_screen(a3, a3_screen) &&
                         quad_projection_is_stable(a0_screen, a1_screen, a2_screen, a3_screen);

        if(a_visible)
        {
            if(! painting_a_quad.set_points(a2_screen, a3_screen, a0_screen, a1_screen))
            {
                painting_a_quad.set_visible(false);
            }
        }
        else
        {
            painting_a_quad.set_visible(false);
        }

        // Painting B: on right wall (constant X).
        bn::fixed b_low_y = b_center_y - PAINTING_B_HALF_WIDTH;
        bn::fixed b_high_y = b_center_y + PAINTING_B_HALF_WIDTH;

        fr::point_3d b0 = local_point_to_view(b_wall_x, b_low_y, b_z_bottom);
        fr::point_3d b1 = local_point_to_view(b_wall_x, b_high_y, b_z_bottom);
        fr::point_3d b2 = local_point_to_view(b_wall_x, b_high_y, b_z_top);
        fr::point_3d b3 = local_point_to_view(b_wall_x, b_low_y, b_z_top);

        fr::point_3d b_center = local_point_to_view(b_wall_x, b_center_y, PAINTING_Z_CENTER);
        fr::point_3d b_normal = local_vector_to_view(-1, 0, 0);  // east wall interior normal

        bn::point b0_screen, b1_screen, b2_screen, b3_screen;
        bool b_visible = is_front_facing(b_center, b_normal) &&
                         project_point_to_screen(b0, b0_screen) &&
                         project_point_to_screen(b1, b1_screen) &&
                         project_point_to_screen(b2, b2_screen) &&
                         project_point_to_screen(b3, b3_screen) &&
                         quad_projection_is_stable(b0_screen, b1_screen, b2_screen, b3_screen);

        if(b_visible)
        {
            if(! painting_b_quad.set_points(b0_screen, b1_screen, b2_screen, b3_screen))
            {
                painting_b_quad.set_visible(false);
            }
        }
        else
        {
            painting_b_quad.set_visible(false);
        }
    };

    auto apply_render_sweep_preset = [&](int preset_index) {
        if(! ENABLE_RENDER_SWEEP_DEBUG)
        {
            return;
        }

        const render_sweep_preset& preset = render_sweep_presets[preset_index];

        current_room = SPAWN_ROOM_ID;
        _player_fx = preset.fx;
        _player_fy = preset.fy;
        _player_fz = -10;
        _player_dir = preset.dir;
        _player_facing_left = preset.facing_left;
        _player_moving = false;

        current_view_angle = normalize_angle(preset.view_angle);
        last_oriented_view_angle = current_view_angle;
        target_view_angle = current_view_angle;
        camera_turn_velocity = 0;
        camera_initial_lock_frames = 0;
        painting_update_cooldown_frames = 0;
        idle_recenter_timer = 0;
        player_world_linear_dir =
            wrap_linear8(dir_to_linear8(_player_dir, _player_facing_left) - view_angle_steps_8(current_view_angle));
        world_anchor_x = room_center_x(current_room);
        world_anchor_y = room_center_y(current_room);
        camera_offset_x = 0;
        camera_offset_y = 0;
        lookahead_x = 0;
        lookahead_y = 0;
        prev_player_fx = _player_fx;
        prev_player_fy = _player_fy;
        door_transition_active = false;
        door_transition_elapsed = 0;
        door_transition_target_room = current_room;
        door_transition_furniture_room = -1;
        door_transition_start_global_x = 0;
        door_transition_start_global_y = 0;
        door_transition_target_global_x = 0;
        door_transition_target_global_y = 0;
        door_transition_current_global_x = 0;
        door_transition_current_global_y = 0;
        door_transition_start_anchor_x = world_anchor_x;
        door_transition_start_anchor_y = world_anchor_y;
        door_transition_target_anchor_x = world_anchor_x;
        door_transition_target_anchor_y = world_anchor_y;
        door_transition_target_local_x = 0;
        door_transition_target_local_y = 0;

        sync_room_models();
        ensure_decor_models();
        bool camera_turn_in_progress = current_view_angle != target_view_angle;
        update_camera(! camera_turn_in_progress);
        update_orientations_and_paintings();
        update_player_sprite_position();
        update_painting_quads();
        paintings_need_update = false;
    };

    if(ENABLE_RENDER_SWEEP_DEBUG)
    {
        apply_render_sweep_preset(render_sweep_pose_index);
    }

    update_player_sprite_position();

    if(paintings_need_update)
    {
        update_painting_quads();
        paintings_need_update = false;
    }

    bn::sprite_text_generator tg(common::variable_8x8_sprite_font);
    tg.set_center_alignment();
    tg.set_bg_priority(0);

    // --- NPC dialog (BG-based, no sprite VRAM) ---
    str::BgDialog npc_dialog;
    debug_menu in_game_debug_menu;
    profiler_menu in_game_profiler;
    bool profiler_requested = false;
    bool minimap_visible = minimap != nullptr;
    auto sync_overlay_visibility = [&]() {
        if(npc_dialog.is_active())
        {
            in_game_debug_menu.set_visible(false);
            in_game_profiler.set_visible(false);
        }
        else if(profiler_requested)
        {
            in_game_debug_menu.set_visible(false);
            in_game_profiler.set_visible(true);
        }
        else if(_debug_mode)
        {
            in_game_profiler.set_visible(false);
            in_game_debug_menu.set_visible(true);
        }
        else
        {
            in_game_debug_menu.set_visible(false);
            in_game_profiler.set_visible(false);
        }

        if(! minimap)
        {
            return;
        }

        bool should_be_visible = ! _debug_mode && ! profiler_requested;

        if(should_be_visible != minimap_visible)
        {
            minimap->set_visible(should_be_visible);
            minimap_visible = should_be_visible;
        }
    };
    bool prev_near_npc = false;
    bool near_any_npc = false;

    bool text_debug_mode = !_debug_mode;
    int text_fx = 0;
    int text_fy = 0;
    int text_dir = 0;
    int text_room = -1;
    int text_corner = -1;
    int text_yaw_deg = 999;
    int text_preview_mode = -1;
    int text_hide_room_models = -1;
    int text_fps = -1;
    int text_vertices = -1;
    int text_hlines = -1;
    int text_pose_index = -2;
    int current_fps = 60;
    int fps_sample_updates = 0;
    int fps_sample_refreshes = 0;
    int door_transition_frame_budget = 0;
    int player_movement_frame_budget = 0;

    auto refresh_overlay_text = [&](int room_id, int fps, int vertices_count, int hlines_count) {
        _text_sprites.clear();
        int yaw_degrees = (normalize_angle(current_view_angle) * 360 + 32768) / 65536;
        if(yaw_degrees > 180)
        {
            yaw_degrees -= 360;
        }

        auto generation_failed = [&]() {
            // Avoid hard asserts from sprite tile exhaustion when toggling overlay.
            _text_sprites.clear();
            text_debug_mode = _debug_mode || profiler_requested;
            text_fx = _player_fx.integer();
            text_fy = _player_fy.integer();
            text_dir = _player_dir;
            text_room = room_id;
            text_corner = _corner_index;
            text_yaw_deg = yaw_degrees;
            text_preview_mode = int(preview_mode);
            text_hide_room_models = int(debug_hide_room_models);
            text_fps = fps;
            text_vertices = vertices_count;
            text_hlines = hlines_count;
            text_pose_index = render_sweep_pose_index;
        };

        if(_debug_mode || profiler_requested)
        {
            int current_fx = _player_fx.integer();
            int current_fy = _player_fy.integer();
            text_fx = current_fx;
            text_fy = current_fy;
            text_dir = _player_dir;
            text_room = room_id;
            text_corner = _corner_index;
            text_yaw_deg = yaw_degrees;
            text_preview_mode = int(preview_mode);
            text_hide_room_models = int(debug_hide_room_models);
            text_fps = fps;
            text_vertices = vertices_count;
            text_hlines = hlines_count;
            text_pose_index = render_sweep_pose_index;
            text_debug_mode = _debug_mode || profiler_requested;
            return;
        }

        if(! tg.generate_optional(0, -72, "ROOM VIEWER", _text_sprites))
        {
            generation_failed();
            return;
        }

        if(! tg.generate_optional(0, 72, "L/R:Zoom START:Recenter B:Profiler", _text_sprites))
        {
            generation_failed();
            return;
        }
        text_fps = fps;
        text_vertices = vertices_count;
        text_preview_mode = int(preview_mode);
        text_hide_room_models = int(debug_hide_room_models);
        text_hlines = hlines_count;
        text_pose_index = render_sweep_pose_index;

        text_debug_mode = _debug_mode || profiler_requested;
    };

    BN_PROFILER_RESET();
    sync_overlay_visibility();

    refresh_overlay_text(current_room, current_fps, _models.vertices_count(), _models.max_hlines_last_frame());

    while(true)
    {
        int frame_cost = bn::core::last_missed_frames() + 1;
        fps_sample_updates += 1;
        fps_sample_refreshes += frame_cost;
        bool fps_sample_ready = false;

        if(fps_sample_refreshes >= 60)
        {
            current_fps = (60 * fps_sample_updates + (fps_sample_refreshes / 2)) / fps_sample_refreshes;
            fps_sample_updates = 0;
            fps_sample_refreshes = 0;
            fps_sample_ready = true;
        }

        if(! npc_dialog.is_active() && bn::keypad::b_pressed())
        {
            profiler_requested = ! profiler_requested;

            if(profiler_requested)
            {
                _debug_mode = false;
            }

            sync_overlay_visibility();
            _text_sprites.clear();

            if(! profiler_requested)
            {
                refresh_overlay_text(current_room, current_fps,
                                     _models.vertices_count(), _models.max_hlines_last_frame());
            }
        }

        sync_overlay_visibility();

        if(in_game_profiler.visible())
        {
            in_game_profiler.update();
            bn::core::update();
            continue;
        }

        BN_PROFILER_START("viewer_update");

        int elapsed_frames = bn::clamp(frame_cost, 1, 4);
        int door_transition_frame_advance = 0;

        if(door_transition_active)
        {
            door_transition_frame_budget = bn::min(door_transition_frame_budget + elapsed_frames,
                                                   DOOR_TRANSITION_MAX_FRAME_BUDGET);
            door_transition_frame_advance =
                bn::min(door_transition_frame_budget, DOOR_TRANSITION_MAX_STEPS_PER_UPDATE);
        }

        int player_frame_advance = door_transition_frame_advance;

        if(camera_initial_lock_frames > 0)
        {
            camera_initial_lock_frames -= elapsed_frames;

            if(camera_initial_lock_frames < 0)
            {
                camera_initial_lock_frames = 0;
            }
        }

        if(ROOM_PREVIEW_AUTO_ADJUST)
        {
            if(preview_mode_change_cooldown_frames > 0)
            {
                preview_mode_change_cooldown_frames -= elapsed_frames;

                if(preview_mode_change_cooldown_frames < 0)
                {
                    preview_mode_change_cooldown_frames = 0;
                }
            }

            if(fps_sample_ready)
            {
                if(current_fps < ROOM_PREVIEW_DOWNSHIFT_FPS)
                {
                    ++preview_mode_low_fps_samples;
                    preview_mode_high_fps_samples = 0;
                }
                else if(current_fps > ROOM_PREVIEW_UPSHIFT_FPS)
                {
                    ++preview_mode_high_fps_samples;
                    preview_mode_low_fps_samples = 0;
                }
                else
                {
                    preview_mode_low_fps_samples = 0;
                    preview_mode_high_fps_samples = 0;
                }

                if(preview_mode_change_cooldown_frames == 0)
                {
                    if(preview_mode_low_fps_samples >= ROOM_PREVIEW_REQUIRED_SAMPLES)
                    {
                        room_preview_mode downgraded_mode = downgraded_room_preview_mode(preview_mode);

                        if(downgraded_mode != preview_mode)
                        {
                            preview_mode = downgraded_mode;
                            preview_mode_change_cooldown_frames = ROOM_PREVIEW_MODE_CHANGE_COOLDOWN_FRAMES;
                            sync_room_models();
                            update_orientations_and_paintings();
                        }

                        preview_mode_low_fps_samples = 0;
                        preview_mode_high_fps_samples = 0;
                    }
                    else if(preview_mode_high_fps_samples >= ROOM_PREVIEW_REQUIRED_SAMPLES)
                    {
                        room_preview_mode upgraded_mode = upgraded_room_preview_mode(preview_mode);

                        if(upgraded_mode != preview_mode)
                        {
                            preview_mode = upgraded_mode;
                            preview_mode_change_cooldown_frames = ROOM_PREVIEW_MODE_CHANGE_COOLDOWN_FRAMES;
                            sync_room_models();
                            update_orientations_and_paintings();
                        }

                        preview_mode_low_fps_samples = 0;
                        preview_mode_high_fps_samples = 0;
                    }
                }
            }
        }

        bool camera_unlocked = camera_initial_lock_frames == 0;
        bool door_approach_lock = near_door_approach(current_room, _player_fx, _player_fy);
        bool camera_steering_enabled = camera_unlocked && !door_approach_lock;

        if(!camera_steering_enabled)
        {
            idle_recenter_timer = 0;
            camera_turn_velocity = 0;
        }
        if(camera_steering_enabled && !door_transition_active && bn::keypad::start_pressed() && !bn::keypad::select_held())
        {
            target_view_angle =
                normalize_angle(heading_angle_from_linear8(player_world_linear_dir) + CAMERA_BEHIND_OFFSET_ANGLE);
            idle_recenter_timer = 0;
        }

        if(bn::keypad::select_pressed())
        {
            if(bn::keypad::l_held())
            {
                debug_hide_room_models = !debug_hide_room_models;
                sync_room_models();
                ensure_decor_models();
                update_orientations_and_paintings();
            }
            else if(ENABLE_RENDER_SWEEP_DEBUG && !bn::keypad::start_held() && !bn::keypad::r_held())
            {
                render_sweep_pose_index += 1;

                if(render_sweep_pose_index >= render_sweep_presets_count)
                {
                    render_sweep_pose_index = 0;
                }

                apply_render_sweep_preset(render_sweep_pose_index);
            }
            else
            {
                if(in_game_profiler.visible())
                {
                    profiler_requested = false;
                    in_game_profiler.set_visible(false);
                }

                _debug_mode = !_debug_mode;
                sync_overlay_visibility();
            }
        }

        if(door_transition_active)
        {
            door_transition_elapsed += door_transition_frame_advance;
            door_transition_frame_budget -= door_transition_frame_advance;

            if(door_transition_elapsed >= DOOR_TRANSITION_DURATION_FRAMES)
            {
                door_transition_active = false;
                door_transition_frame_budget = 0;
                current_room = door_transition_target_room;
                _player_fx = door_transition_target_local_x;
                _player_fy = door_transition_target_local_y;
                world_anchor_x = door_transition_target_anchor_x;
                world_anchor_y = door_transition_target_anchor_y;
                camera_offset_x = 0;
                camera_offset_y = 0;
                lookahead_x = 0;
                lookahead_y = 0;
                prev_player_fx = _player_fx;
                prev_player_fy = _player_fy;
                clear_room_decor_models(books_ptr, potted_plant_ptr);
                books_ptr = transition_books_ptr;
                potted_plant_ptr = transition_potted_plant_ptr;
                transition_books_ptr = nullptr;
                transition_potted_plant_ptr = nullptr;
                door_transition_furniture_room = -1;
                sync_room_models();
                ensure_decor_models();
                update_orientations_and_paintings();
            }
            else
            {
                bn::fixed transition_progress = bn::fixed(door_transition_elapsed) / DOOR_TRANSITION_DURATION_FRAMES;
                bn::fixed eased_progress = transition_progress * transition_progress *
                                           (bn::fixed(3) - bn::fixed(2) * transition_progress);
                door_transition_current_global_x = door_transition_start_global_x +
                    (door_transition_target_global_x - door_transition_start_global_x) * eased_progress;
                door_transition_current_global_y = door_transition_start_global_y +
                    (door_transition_target_global_y - door_transition_start_global_y) * eased_progress;
                world_anchor_x = door_transition_start_anchor_x +
                    (door_transition_target_anchor_x - door_transition_start_anchor_x) * eased_progress;
                world_anchor_y = door_transition_start_anchor_y +
                    (door_transition_target_anchor_y - door_transition_start_anchor_y) * eased_progress;
                update_orientations_and_paintings();
            }
        }

        bool moving = false;
        int dir = 0;
        bool facing_left = false;
        linear8_to_dir(player_world_linear_dir + view_angle_steps_8(current_view_angle), dir, facing_left);
        bn::fixed dfx = 0;
        bn::fixed dfy = 0;
        bn::fixed actual_move_dx = 0;
        bn::fixed actual_move_dy = 0;
        bn::fixed screen_dx = 0, screen_dy = 0;

        if(!door_transition_active && !npc_dialog.is_active() && bn::keypad::up_held())
        {
            screen_dy = -1;
            moving = true;
        }
        else if(!door_transition_active && !npc_dialog.is_active() && bn::keypad::down_held())
        {
            screen_dy = 1;
            moving = true;
        }

        if(!door_transition_active && !npc_dialog.is_active() && bn::keypad::left_held())
        {
            screen_dx = -1;
            moving = true;
        }
        else if(!door_transition_active && !npc_dialog.is_active() && bn::keypad::right_held())
        {
            screen_dx = 1;
            moving = true;
        }

        if(moving)
        {
            if(screen_dx == 0 && screen_dy == 1)       { dir = 0; facing_left = false; }
            else if(screen_dx == 0 && screen_dy == -1)  { dir = 4; facing_left = false; }
            else if(screen_dx == 1 && screen_dy == 0)   { dir = 2; facing_left = false; }
            else if(screen_dx == -1 && screen_dy == 0)  { dir = 2; facing_left = true;  }
            else if(screen_dx == 1 && screen_dy == 1)   { dir = 1; facing_left = false; }
            else if(screen_dx == -1 && screen_dy == 1)  { dir = 1; facing_left = true;  }
            else if(screen_dx == 1 && screen_dy == -1)  { dir = 3; facing_left = false; }
            else if(screen_dx == -1 && screen_dy == -1) { dir = 3; facing_left = true;  }

            player_world_linear_dir = wrap_linear8(
                dir_to_linear8(dir, facing_left) - view_angle_steps_8(current_view_angle));

            bn::fixed speed_factor = 1;
            if(screen_dx != 0 && screen_dy != 0)
            {
                speed_factor = bn::fixed(0.707);
            }
            screen_dx *= speed_factor;
            screen_dy *= speed_factor;

            bn::fixed_point room_delta = screen_to_room_delta(screen_dx, screen_dy, current_view_angle);
            dfx = room_delta.x();
            dfy = room_delta.y();

            dfx *= MOVE_SPEED;
            dfy *= MOVE_SPEED;
            bn::fixed old_player_fx = _player_fx;
            bn::fixed old_player_fy = _player_fy;
            player_movement_frame_budget = bn::min(player_movement_frame_budget + elapsed_frames,
                                                   PLAYER_MOVEMENT_MAX_FRAME_BUDGET);
            player_frame_advance = bn::min(player_movement_frame_budget, PLAYER_MOVEMENT_MAX_STEPS_PER_UPDATE);

            // Preserve the normal 60 FPS cadence, but let short missed-frame
            // bursts pay back over the next few updates instead of losing
            // ground permanently in heavier rooms.
            for(int step = 0; step < player_frame_advance; ++step)
            {
                bn::fixed new_fx = bn::clamp(_player_fx + dfx,
                                             room_floor_min_x(current_room), room_floor_max_x(current_room));
                bn::fixed new_fy = bn::clamp(_player_fy + dfy,
                                             room_floor_min_y(current_room), room_floor_max_y(current_room));

                if(!overlaps_any_furniture(current_room, new_fx, new_fy))
                {
                    _player_fx = new_fx;
                    _player_fy = new_fy;
                }
                else
                {
                    bn::fixed slide_fx = bn::clamp(_player_fx + dfx,
                                                   room_floor_min_x(current_room), room_floor_max_x(current_room));
                    bn::fixed slide_fy = _player_fy;
                    if(!overlaps_any_furniture(current_room, slide_fx, slide_fy))
                    {
                        _player_fx = slide_fx;
                    }
                    else
                    {
                        slide_fx = _player_fx;
                        slide_fy = bn::clamp(_player_fy + dfy,
                                             room_floor_min_y(current_room), room_floor_max_y(current_room));
                        if(!overlaps_any_furniture(current_room, slide_fx, slide_fy))
                        {
                            _player_fy = slide_fy;
                        }
                    }
                }

                // Check for room transition via doors.
                bn::fixed new_local_x, new_local_y;
                int next_room = check_door_transition(current_room, _player_fx, _player_fy,
                                                      new_local_x, new_local_y);
                if(next_room >= 0 && next_room != current_room)
                {
                    door_transition_furniture_room = next_room;
                    // Preload destination-room decor before camera interpolation starts.
                    ensure_decor_models();
                    // Avoid seeing current-room decor through walls during camera interpolation.
                    clear_room_decor_models(books_ptr, potted_plant_ptr);

                    update_orientations_and_paintings();

                    door_transition_active = true;
                    door_transition_elapsed = 0;
                    door_transition_frame_budget = 0;
                    door_transition_target_room = next_room;
                    door_transition_start_global_x = room_center_x(current_room) + _player_fx;
                    door_transition_start_global_y = room_center_y(current_room) + _player_fy;
                    door_transition_target_global_x = room_center_x(next_room) + new_local_x;
                    door_transition_target_global_y = room_center_y(next_room) + new_local_y;
                    door_transition_current_global_x = door_transition_start_global_x;
                    door_transition_current_global_y = door_transition_start_global_y;
                    door_transition_start_anchor_x = world_anchor_x;
                    door_transition_start_anchor_y = world_anchor_y;
                    door_transition_target_anchor_x = room_center_x(next_room);
                    door_transition_target_anchor_y = room_center_y(next_room);
                    door_transition_target_local_x = new_local_x;
                    door_transition_target_local_y = new_local_y;
                    // Keep destination neighborhood loaded during interpolation.
                    sync_room_models();
                }
            }

            player_movement_frame_budget -= player_frame_advance;
            actual_move_dx = _player_fx - old_player_fx;
            actual_move_dy = _player_fy - old_player_fy;
        }
        else if(!door_transition_active)
        {
            player_movement_frame_budget = 0;
        }

        update_camera_follow();

        bool view_angle_changed = false;

        if(camera_steering_enabled && !door_transition_active)
        {
            bool player_idle = actual_move_dx == 0 && actual_move_dy == 0;
            bool no_buttons_held = !bn::keypad::any_held();
            bool player_near_room_center =
                bn::abs(_player_fx) <= CAMERA_CENTER_HOLD_HALF_EXTENT &&
                bn::abs(_player_fy) <= CAMERA_CENTER_HOLD_HALF_EXTENT;

            if(player_idle && no_buttons_held && !player_near_room_center)
            {
                idle_recenter_timer += elapsed_frames;

                if(idle_recenter_timer > CAMERA_IDLE_RECENTER_DELAY_FRAMES)
                {
                    idle_recenter_timer = CAMERA_IDLE_RECENTER_DELAY_FRAMES;
                }
            }
            else
            {
                idle_recenter_timer = 0;
            }

            if(idle_recenter_timer >= CAMERA_IDLE_RECENTER_DELAY_FRAMES)
            {
                target_view_angle =
                    normalize_angle(heading_angle_from_linear8(player_world_linear_dir) + CAMERA_BEHIND_OFFSET_ANGLE);
            }

            if(current_view_angle != target_view_angle)
            {
                current_view_angle = step_angle_toward_target(current_view_angle, target_view_angle,
                                                              camera_turn_velocity);
                view_angle_changed = true;
            }
        }

        int quantized_corner = corner_from_view_angle(current_view_angle);
        if(quantized_corner != _corner_index)
        {
            _corner_index = quantized_corner;
            sync_room_models();
        }

        int render_angle_delta = int_abs(shortest_angle_delta(last_oriented_view_angle, current_view_angle));
        if(view_angle_changed && render_angle_delta >= CAMERA_RENDER_UPDATE_ANGLE_STEP)
        {
            update_orientations_and_paintings();
        }
        else if(!view_angle_changed && render_angle_delta > 0)
        {
            update_orientations_and_paintings();
        }
        else if(camera_follow_moved)
        {
            update_orientations_and_paintings();
        }

        update_camera();

        linear8_to_dir(player_world_linear_dir + view_angle_steps_8(current_view_angle), dir, facing_left);
        update_player_sprite_position();

        if(paintings_need_update)
        {
            bool high_motion = door_transition_active || view_angle_changed;

            if(high_motion)
            {
                painting_update_cooldown_frames -= elapsed_frames;
                if(painting_update_cooldown_frames <= 0)
                {
                    update_painting_quads();
                    paintings_need_update = false;
                    painting_update_cooldown_frames = PAINTING_MOTION_UPDATE_INTERVAL_FRAMES;
                }
            }
            else
            {
                update_painting_quads();
                paintings_need_update = false;
                painting_update_cooldown_frames = 0;
            }
        }

        player_sprite.set_horizontal_flip(false);
        _update_player_anim_tiles(player_sprite_item, moving || door_transition_active, dir, facing_left,
                                  player_frame_advance);

        // --- NPC idle animation with camera-relative facing ---
        {
            // NPC faces south (linear 0) in world space; rotate by camera angle
            int npc_linear = wrap_linear8(view_angle_steps_8(current_view_angle));
            int npc_dir = 0;
            bool npc_facing_left = false;
            linear8_to_dir(npc_linear, npc_dir, npc_facing_left);

            int npc_anim_frame = (npc_anim_counter / NPC_ANIM_SPEED) % NPC_FRAMES_PER_ANIM;
            int npc_tile_index = npc_dir * NPC_FRAMES_PER_ANIM + npc_anim_frame;

            if(npc_sprite_a_ptr)
            {
                npc_sprite_item_a.tiles().set_tiles_ref(
                    bn::sprite_items::villager.tiles_item(), npc_tile_index);
                npc_sprite_a_ptr->set_horizontal_flip(npc_facing_left);
            }

            if(npc_sprite_b_ptr)
            {
                npc_sprite_item_b.tiles().set_tiles_ref(
                    bn::sprite_items::villager.tiles_item(), npc_tile_index);
                npc_sprite_b_ptr->set_horizontal_flip(npc_facing_left);
            }

            ++npc_anim_counter;
        }

        // --- NPC proximity check ---
        bool near_npc_a = !npc_dialog.is_active() && (current_room == NPC_ROOM_A) &&
            (bn::abs(_player_fx - NPC_FX) + bn::abs(_player_fy - NPC_FY) < NPC_INTERACT_DIST);
        bool near_npc_b = !npc_dialog.is_active() && (current_room == NPC_ROOM_B) &&
            (bn::abs(_player_fx - NPC_FX) + bn::abs(_player_fy - NPC_FY) < NPC_INTERACT_DIST);
        near_any_npc = near_npc_a || near_npc_b;
        if(near_any_npc != prev_near_npc)
        {
            if(near_any_npc) { npc_dialog.show_prompt(); }
            else             { npc_dialog.hide_prompt(); }
            prev_near_npc = near_any_npc;
        }

        // --- NPC dialog interaction ---
        if(npc_dialog.is_active())
        {
            npc_dialog.update();
        }
        else
        {
            // Check if player presses A while near an NPC
            if(bn::keypad::a_pressed() && !door_transition_active)
            {
                if(near_npc_a)
                {
                    npc_dialog.set_greeting(villager_a_greeting);
                    str::BgDialog::DialogOption opts_a[] = {
                        {"Tell me about this place", villager_a_opt0_resp, false},
                        {"Any warnings?", villager_a_opt1_resp, false},
                        {"Goodbye", {}, true}
                    };
                    npc_dialog.set_options(opts_a);
                    npc_dialog.talk();
                }
                else if(near_npc_b)
                {
                    npc_dialog.set_greeting(villager_b_greeting);
                    str::BgDialog::DialogOption opts_b[] = {
                        {"Who are you?", villager_b_opt0_resp, false},
                        {"How do I get out?", villager_b_opt1_resp, false},
                        {"Goodbye", {}, true}
                    };
                    npc_dialog.set_options(opts_b);
                    npc_dialog.talk();
                }
            }
        }

        bool overlay_requested = _debug_mode || profiler_requested;
        bool text_dirty = overlay_requested != text_debug_mode;
        int current_vertices = _models.vertices_count();
        int current_hlines = _models.max_hlines_last_frame();
        int current_yaw_deg = (normalize_angle(current_view_angle) * 360 + 32768) / 65536;
        if(current_yaw_deg > 180)
        {
            current_yaw_deg -= 360;
        }

        if(_debug_mode && ! in_game_profiler.visible() && ! npc_dialog.is_active())
        {
            debug_menu::state debug_state;
            debug_state.room_id = current_room;
            debug_state.fps = current_fps;
            debug_state.vertices_count = current_vertices;
            debug_state.hlines_count = current_hlines;
            debug_state.room_x = _player_fx.integer();
            debug_state.room_y = _player_fy.integer();
            debug_state.dir = _player_dir;
            debug_state.corner = _corner_index;
            debug_state.yaw_degrees = current_yaw_deg;
            debug_state.preview_name = room_preview_mode_name(preview_mode);
            debug_state.room_models_visible = ! debug_hide_room_models;
            debug_state.used_alloc_ewram = bn::memory::used_alloc_ewram();
            debug_state.used_static_ewram = bn::memory::used_static_ewram();
            debug_state.used_static_iwram = bn::memory::used_static_iwram();
            debug_state.used_bg_tiles = bn::bg_tiles::used_tiles_count();
            debug_state.available_bg_tiles = bn::bg_tiles::available_tiles_count();
            debug_state.used_bg_blocks = bn::bg_tiles::used_blocks_count();
            debug_state.available_bg_blocks = bn::bg_tiles::available_blocks_count();
            in_game_debug_menu.update(debug_state);
        }

        if(! overlay_requested)
        {
            int current_fx = _player_fx.integer();
            int current_fy = _player_fy.integer();

            if(current_fx != text_fx || current_fy != text_fy ||
               _player_dir != text_dir || current_room != text_room || _corner_index != text_corner ||
               current_yaw_deg != text_yaw_deg ||
               int(preview_mode) != text_preview_mode ||
               int(debug_hide_room_models) != text_hide_room_models ||
               current_fps != text_fps || current_vertices != text_vertices ||
               current_hlines != text_hlines ||
               render_sweep_pose_index != text_pose_index)
            {
                text_dirty = true;
            }
        }

        if(text_dirty && !npc_dialog.is_active())
        {
            refresh_overlay_text(current_room, current_fps, current_vertices, current_hlines);
        }

        // --- Minimap update ---
        if(minimap && ! _debug_mode && ! profiler_requested)
        {
            bn::fixed_point player_world_pos;
            if(door_transition_active)
            {
                player_world_pos = bn::fixed_point(door_transition_current_global_x, door_transition_current_global_y);
            }
            else
            {
                player_world_pos = bn::fixed_point(room_center_x(current_room) + _player_fx, room_center_y(current_room) + _player_fy);
            }

            // Map room_viewer direction (0=down,1=down_side,2=side,3=up_side,4=up)
            // to minimap direction (UP=0,DOWN=1,LEFT=2,RIGHT=3)
            int minimap_dir;
            if(dir == 4)       { minimap_dir = 0; }  // up -> UP
            else if(dir == 3)  { minimap_dir = 0; }  // up_side -> UP
            else if(dir == 0)  { minimap_dir = 1; }  // down -> DOWN
            else if(dir == 1)  { minimap_dir = 1; }  // down_side -> DOWN
            else               { minimap_dir = facing_left ? 2 : 3; }  // side -> LEFT/RIGHT

            minimap->update(player_world_pos, minimap_dir);
        }

        BN_PROFILER_STOP();
        _models.update(_camera);
        bn::core::update();
    }
}

}






