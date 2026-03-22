#include "str_scene_room_viewer.h"

#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_bg_palettes.h"
#include "bn_color.h"
#include "bn_sprites.h"
#include "bn_sprite_text_generator.h"
#include "bn_sprite_affine_mat_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_affine_mat_attributes.h"
#include "bn_fixed.h"
#include "bn_math.h"
#include "bn_algorithm.h"
#include "bn_assert.h"
#include "bn_string.h"
#include "bn_sstream.h"
#include "bn_log.h"
#include "bn_profiler.h"
#include "bn_point.h"
#include "bn_sprite_items_eris.h"
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

#ifndef STR_ROOM_VIEWER_AUTO_PROFILE_FRAMES
    #define STR_ROOM_VIEWER_AUTO_PROFILE_FRAMES 0
#endif

namespace {
    constexpr int iso_phi = 6400;
    constexpr int iso_theta = 59904;
    constexpr int iso_psi = 6400;
    constexpr int NUM_ROOMS = 6;
    constexpr int QUARTER_TURN_ANGLE = 16384;
    constexpr int CAMERA_BEHIND_OFFSET_ANGLE = 24576;
    constexpr int CAMERA_MIN_RETARGET_DELTA = 8192;
    constexpr bn::fixed CAMERA_FOLLOW_GAIN = bn::fixed(0.025);
    constexpr bn::fixed CAMERA_START_GAIN = bn::fixed(0.14);
    constexpr int CAMERA_FOLLOW_MAX_STEP = 128;
    constexpr int CAMERA_START_MAX_STEP = 640;
    constexpr int CAMERA_MIN_STEP = 4;
    constexpr int CAMERA_SNAP_EPSILON = 32;
    constexpr int CAMERA_START_BOOST_FRAMES = 10;
    constexpr int CAMERA_INITIAL_LOCK_FRAMES = 120;
    constexpr int CAMERA_RETARGET_COOLDOWN_FRAMES = 36;
    constexpr int CAMERA_IDLE_FOLLOW_DELAY_FRAMES = 60;
    constexpr int CAMERA_RENDER_UPDATE_ANGLE_STEP = 64;
    constexpr int PAINTING_MOTION_UPDATE_INTERVAL_FRAMES = 2;
    constexpr bn::fixed PAINTING_FACE_VISIBILITY_DOT_MIN = bn::fixed(8);
    constexpr int PAINTING_MIN_TRI_AREA2 = 220;
    constexpr int ADJACENT_ROOM_DEPTH_BIAS = 1500000;
    constexpr int TRANSITION_DECOR_DEPTH_BIAS = ADJACENT_ROOM_DEPTH_BIAS;
    constexpr int ROOM_PREVIEW_TARGET_FPS = 50;
    constexpr int ROOM_PREVIEW_FPS_HYSTERESIS = 3;
    constexpr int ROOM_PREVIEW_DOWNSHIFT_FPS = ROOM_PREVIEW_TARGET_FPS - ROOM_PREVIEW_FPS_HYSTERESIS;
    constexpr int ROOM_PREVIEW_UPSHIFT_FPS = ROOM_PREVIEW_TARGET_FPS + ROOM_PREVIEW_FPS_HYSTERESIS;
    constexpr int ROOM_PREVIEW_REQUIRED_SAMPLES = 2;
    constexpr int ROOM_PREVIEW_MODE_CHANGE_COOLDOWN_FRAMES = 180;
    constexpr bool ROOM_PREVIEW_AUTO_ADJUST = false;
    constexpr bool ENABLE_PAINTING_QUADS = true;
    constexpr bool ENABLE_NPC_SPRITES = true;
    constexpr bn::fixed NPC_FX = 20;
    constexpr bn::fixed NPC_FY = -15;
    constexpr bn::fixed NPC_FZ = -10;
    constexpr int NPC_ANIM_SPEED = 12;
    constexpr int NPC_FRAMES_PER_ANIM = 4;
    constexpr int NPC_PALETTE_HAT_INDEX_0 = 9;
    constexpr int NPC_PALETTE_HAT_INDEX_1 = 10;
    constexpr int NPC_PALETTE_HAT_INDEX_2 = 11;
    constexpr int BIG_ROOM_A = 1;
    constexpr int BIG_ROOM_B = 5;

    // NPC hat recolor targets per big room (derived from room floor colors)
    // Room 1 floor: bn::color(12,14,22) blue -> keep original blue hat
    // Room 5 floor: bn::color(8,18,20) teal -> recolor to teal/green
    constexpr bn::color npc_room5_hat_color_0(12, 28, 24);
    constexpr bn::color npc_room5_hat_color_1(6, 20, 18);
    constexpr bn::color npc_room5_hat_color_2(8, 12, 16);

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

    int quantize_heading_angle_8(int angle)
    {
        int normalized_angle = normalize_angle(angle);
        int bucket = (normalized_angle + 4096) / 8192;
        return normalize_angle(bucket * 8192);
    }

    int wrap_linear8(int linear)
    {
        return ((linear % 8) + 8) % 8;
    }

    int view_angle_steps_8(int angle)
    {
        return normalize_angle(angle) / 8192;
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
        bn::fixed sp = fr::sin(iso_phi), cp = fr::cos(iso_phi);
        bn::fixed st = fr::sin(iso_theta), ct = fr::cos(iso_theta);
        bn::fixed ss = fr::sin(iso_psi),  cs = fr::cos(iso_psi);

        bn::fixed c0x = cp * ct;
        bn::fixed c0y = cp * st * ss - sp * cs;
        bn::fixed c0z = cp * st * cs + sp * ss;
        bn::fixed c1x = sp * ct;
        bn::fixed c1y = sp * st * ss + cp * cs;
        bn::fixed c1z = sp * st * cs - cp * ss;
        bn::fixed c2x = -st;
        bn::fixed c2y = ct * ss;
        bn::fixed c2z = ct * cs;

        out[0] = { c0x, c0y, c0z,  c1x, c1y, c1z,  c2x, c2y, c2z };
        out[1] = { c0y, -c0x, c0z,  c1y, -c1x, c1z,  c2y, -c2x, c2z };
        out[2] = { -c0x, -c0y, c0z,  -c1x, -c1y, c1z,  -c2x, -c2y, c2z };
        out[3] = { -c0y, c0x, c0z,  -c1y, c1x, c1z,  -c2y, c2x, c2z };
    }

    constexpr bn::fixed BOOKS_FX = 24;
    constexpr bn::fixed BOOKS_FY = -12;
    constexpr bn::fixed POTTED_PLANT_FX = -28;
    constexpr bn::fixed POTTED_PLANT_FY = 24;
    // Textured-polygon painting quads (room-local coordinates):
    // A on back wall (Y = +58), B on right wall (X = +58).
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

    constexpr bn::fixed FLOOR_MIN = -55;
    constexpr bn::fixed FLOOR_MAX = 55;
    constexpr bn::fixed DOOR_HALF_WIDTH = 10;
    constexpr bn::fixed DOOR_APPROACH_EDGE_MARGIN = 18;
    constexpr bn::fixed DOOR_APPROACH_LANE_MARGIN = 12;
    // Movement is frame-compensated, so keep a lower per-step speed target.
    constexpr bn::fixed MOVE_SPEED = bn::fixed(0.5);
    constexpr int DOOR_TRANSITION_DURATION_FRAMES = 16;
    constexpr int SPAWN_CORNER_INDEX = 2;
    constexpr int SPAWN_PLAYER_DIR = 3;
    constexpr bool SPAWN_PLAYER_FACING_LEFT = false;
    constexpr int SPAWN_ROOM_ID = 1;
    constexpr int SHAPE_OAM_START_INDEX = 64;
    constexpr int SHAPE_RESERVED_HANDLES = fr::models_3d::required_reserved_sprite_handles();
    constexpr int ROOM_VIEWER_RESERVED_HANDLES = 0;
    static_assert(ROOM_VIEWER_RESERVED_HANDLES <= bn::hw::sprites::count(),
                  "Room viewer reserved handles overflow OAM");

    class textured_triangle
    {
    public:
        explicit textured_triangle(const bn::sprite_item& sprite_item) :
            _sprite(sprite_item.create_sprite(0, 0)),
            _affine_mat(bn::sprite_affine_mat_ptr::create()),
            _half_size(sprite_item.shape_size().width() / 2)
        {
            _sprite.set_affine_mat(_affine_mat);
            // Keep paintings above room HDMA sprites without needing manual OAM writes.
            _sprite.set_bg_priority(1);
            _sprite.set_visible(false);
        }

        void set_visible(bool visible)
        {
            _sprite.set_visible(visible);
        }

        void set_points(const bn::point& p0, const bn::point& p1, const bn::point& p2)
        {
            _p0 = p0;
            _p1 = p1;
            _p2 = p2;
            _update();
        }

    private:
        bn::sprite_ptr _sprite;
        bn::sprite_affine_mat_ptr _affine_mat;
        bn::point _p0;
        bn::point _p1;
        bn::point _p2;
        int _half_size;

        void _update()
        {
            switch(_half_size)
            {
                case 4:
                    _update_impl<4>();
                    break;

                case 8:
                    _update_impl<8>();
                    break;

                case 16:
                    _update_impl<16>();
                    break;

                case 32:
                    _update_impl<32>();
                    break;

                default:
                    BN_ERROR("Invalid textured triangle half size: ", _half_size);
                    break;
            }
        }

        template<int half_size>
        void _update_impl()
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
                return;
            }

            int pa = (-512 * half_size * y1 + 512 * half_size * y2 + 256 * y1 - 256 * y2) / affine_divisor;
            int pb = (256 * (2 * half_size * x1 - 2 * half_size * x2 - x1 + x2)) / affine_divisor;
            int pc = (-512 * half_size * y0 + 512 * half_size * y1 + 256 * y0 - 256 * y1) / affine_divisor;
            int pd = (256 * (2 * half_size * x0 - 2 * half_size * x1 - x0 + x1)) / affine_divisor;
            constexpr int max_affine_register = 32767;

            // Prevent affine register overflow/wrap when projected triangles become unstable at specific zooms.
            if(abs_value(pa) > max_affine_register || abs_value(pb) > max_affine_register ||
               abs_value(pc) > max_affine_register || abs_value(pd) > max_affine_register)
            {
                set_visible(false);
                return;
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

            // Keep off-screen/unstable projections from wrapping into visible garbage quads.
            if(abs_value(sprite_x) > 240 || abs_value(sprite_y) > 176)
            {
                set_visible(false);
                return;
            }

            _sprite.set_position(sprite_x, sprite_y);
            set_visible(true);
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

        void set_points(const bn::point& p0, const bn::point& p1, const bn::point& p2, const bn::point& p3)
        {
            _top.set_points(p2, p3, p0);
            _bottom.set_points(p0, p1, p2);
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
        decor_books,        // Room 0
        decor_books,        // Room 1
        decor_potted_plant, // Room 2
        decor_books,        // Room 3
        decor_potted_plant, // Room 4
        decor_potted_plant, // Room 5
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

    // Building layout: 2 columns x 3 rows
    //
    //   col 0   col 1
    //  +------+------+
    //  |  R0  |  R1  |  row 0
    //  +--||--+--||--+
    //  |  R2  |  R3  |  row 1
    //  +--||--+--||--+
    //  |  R4  |  R5  |  row 2
    //  +------+------+
    //
    // Each room is 120x120, player confined to [-55,55] (FLOOR_MIN/MAX).
    // Doors trigger when player crosses ??55 near center of a wall edge.

    constexpr inline bn::color room_viewer_colors[10] = {
        bn::color(22, 16, 8),   // 0 room0_floor
        bn::color(12, 14, 22),  // 1 room1_floor
        bn::color(10, 20, 10),  // 2 room2_floor
        bn::color(22, 10, 8),   // 3 room3_floor
        bn::color(16, 10, 20),  // 4 room4_floor
        bn::color(8, 18, 20),   // 5 room5_floor
        bn::color(28, 26, 22),  // 6 wall
        bn::color(16, 22, 28),  // 7 window
        bn::color(10, 8, 6),    // 8 door_frame
        bn::color(18, 12, 6)    // 9 furniture
    };

    // Door transition info: which direction, which neighbor room
    // Doors are at the edges of local room space:
    //   North door (y near -55): go to row-1 (same column)
    //   South door (y near +55): go to row+1 (same column)
    //   West door (x near -55):  go to col-1 (same row) -- but only col0->col1 via center wall
    //   East door (x near +55):  go to col+1 (same row) -- but only col0->col1 via center wall
    //
    // In our 2x3 grid (col, row): R0=(0,0), R1=(1,0), R2=(0,1), R3=(1,1), R4=(0,2), R5=(1,2)

    int room_col(int room_id) { return room_id % 2; }
    int room_row(int room_id) { return room_id / 2; }
    int room_id_from_grid(int col, int row) { return row * 2 + col; }

    int int_abs(int value)
    {
        return value >= 0 ? value : -value;
    }

    // Runtime room vertices are emitted with ROOM_SCALE=2 (generate_level_headers.py).
    // Keep placement centers in the same scaled engine-space units.
    constexpr bn::fixed room_center_x_values[NUM_ROOMS] = {
        bn::fixed(-60), bn::fixed(75), bn::fixed(-60), bn::fixed(60), bn::fixed(-60), bn::fixed(75)
    };
    constexpr bn::fixed room_center_y_values[NUM_ROOMS] = {
        bn::fixed(-120), bn::fixed(-135), bn::fixed(0), bn::fixed(0), bn::fixed(120), bn::fixed(135)
    };
    constexpr bn::fixed room_half_extent_x_values[NUM_ROOMS] = {
        bn::fixed(60), bn::fixed(75), bn::fixed(60), bn::fixed(60), bn::fixed(60), bn::fixed(75)
    };
    constexpr bn::fixed room_half_extent_y_values[NUM_ROOMS] = {
        bn::fixed(60), bn::fixed(75), bn::fixed(60), bn::fixed(60), bn::fixed(60), bn::fixed(75)
    };

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

    enum class door_direction
    {
        east,
        west,
        south,
        north
    };

    int neighbor_room_for_door(int room_id, door_direction direction)
    {
        int col = room_col(room_id);
        int row = room_row(room_id);

        switch(direction)
        {
            case door_direction::east:
                return col < 1 ? room_id_from_grid(col + 1, row) : -1;

            case door_direction::west:
                return col > 0 ? room_id_from_grid(col - 1, row) : -1;

            case door_direction::south:
                return row < 2 ? room_id_from_grid(col, row + 1) : -1;

            case door_direction::north:
                return row > 0 ? room_id_from_grid(col, row - 1) : -1;

            default:
                return -1;
        }
    }

    int preferred_neighbor_room_for_corner(int room_id, int corner_index)
    {
        switch(corner_index)
        {
            case 0:
                return neighbor_room_for_door(room_id, door_direction::north);

            case 1:
                return neighbor_room_for_door(room_id, door_direction::west);

            case 2:
                return neighbor_room_for_door(room_id, door_direction::south);

            default:
                return neighbor_room_for_door(room_id, door_direction::east);
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

        if(direction == door_direction::east || direction == door_direction::west)
        {
            bn::fixed shared_world_y = (room_center_y(room_id) + room_center_y(neighbor_room)) / 2;
            return shared_world_y - room_center_y(room_id);
        }

        bn::fixed shared_world_x = (room_center_x(room_id) + room_center_x(neighbor_room)) / 2;
        return shared_world_x - room_center_x(room_id);
    }

    // Check if a door transition should occur and return the new room id (-1 if no transition)
    // Also sets new_local_x/y for the player position in the new room
    int check_door_transition(int current_room, bn::fixed local_x, bn::fixed local_y,
                              bn::fixed& new_local_x, bn::fixed& new_local_y)
    {
        int east_room = neighbor_room_for_door(current_room, door_direction::east);
        bn::fixed east_door_center = aligned_door_center_offset(current_room, door_direction::east);
        if(east_room >= 0 && local_x >= FLOOR_MAX && bn::abs(local_y - east_door_center) <= DOOR_HALF_WIDTH)
        {
            new_local_x = FLOOR_MIN;
            bn::fixed shared_world_y = room_center_y(current_room) + local_y;
            new_local_y = bn::clamp(shared_world_y - room_center_y(east_room), FLOOR_MIN, FLOOR_MAX);
            return east_room;
        }

        int west_room = neighbor_room_for_door(current_room, door_direction::west);
        bn::fixed west_door_center = aligned_door_center_offset(current_room, door_direction::west);
        if(west_room >= 0 && local_x <= FLOOR_MIN && bn::abs(local_y - west_door_center) <= DOOR_HALF_WIDTH)
        {
            new_local_x = FLOOR_MAX;
            bn::fixed shared_world_y = room_center_y(current_room) + local_y;
            new_local_y = bn::clamp(shared_world_y - room_center_y(west_room), FLOOR_MIN, FLOOR_MAX);
            return west_room;
        }

        int south_room = neighbor_room_for_door(current_room, door_direction::south);
        bn::fixed south_door_center = aligned_door_center_offset(current_room, door_direction::south);
        if(south_room >= 0 && local_y >= FLOOR_MAX && bn::abs(local_x - south_door_center) <= DOOR_HALF_WIDTH)
        {
            bn::fixed shared_world_x = room_center_x(current_room) + local_x;
            new_local_x = bn::clamp(shared_world_x - room_center_x(south_room), FLOOR_MIN, FLOOR_MAX);
            new_local_y = FLOOR_MIN;
            return south_room;
        }

        int north_room = neighbor_room_for_door(current_room, door_direction::north);
        bn::fixed north_door_center = aligned_door_center_offset(current_room, door_direction::north);
        if(north_room >= 0 && local_y <= FLOOR_MIN && bn::abs(local_x - north_door_center) <= DOOR_HALF_WIDTH)
        {
            bn::fixed shared_world_x = room_center_x(current_room) + local_x;
            new_local_x = bn::clamp(shared_world_x - room_center_x(north_room), FLOOR_MIN, FLOOR_MAX);
            new_local_y = FLOOR_MAX;
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
           local_x >= FLOOR_MAX - DOOR_APPROACH_EDGE_MARGIN &&
           bn::abs(local_y - east_door_center) <= door_lane_half_width)
        {
            return true;
        }

        int west_room = neighbor_room_for_door(current_room, door_direction::west);
        bn::fixed west_door_center = aligned_door_center_offset(current_room, door_direction::west);
        if(west_room >= 0 &&
           local_x <= FLOOR_MIN + DOOR_APPROACH_EDGE_MARGIN &&
           bn::abs(local_y - west_door_center) <= door_lane_half_width)
        {
            return true;
        }

        int south_room = neighbor_room_for_door(current_room, door_direction::south);
        bn::fixed south_door_center = aligned_door_center_offset(current_room, door_direction::south);
        if(south_room >= 0 &&
           local_y >= FLOOR_MAX - DOOR_APPROACH_EDGE_MARGIN &&
           bn::abs(local_x - south_door_center) <= door_lane_half_width)
        {
            return true;
        }

        int north_room = neighbor_room_for_door(current_room, door_direction::north);
        bn::fixed north_door_center = aligned_door_center_offset(current_room, door_direction::north);
        if(north_room >= 0 &&
           local_y <= FLOOR_MIN + DOOR_APPROACH_EDGE_MARGIN &&
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
            case 2:  return str::model_3d_items::room_2;
            case 3:  return str::model_3d_items::room_3;
            case 4:  return str::model_3d_items::room_4;
            case 5:  return str::model_3d_items::room_5;
            default: return str::model_3d_items::room_0;
        }
    }

}

namespace str
{

RoomViewer::RoomViewer() {}

void RoomViewer::_update_player_anim_tiles(fr::sprite_3d_item& item, bool moving, int dir, int elapsed_frames)
{
    constexpr int idle_bases[] = {8, 12, 16, 20, 24};
    constexpr int walk_bases[] = {28, 32, 36, 40, 44};
    constexpr int frames_per_anim = 4;
    constexpr int anim_speed = 8;

    if(_player_moving != moving || _player_dir != dir)
    {
        _player_moving = moving;
        _player_dir = dir;
        _anim_frame_counter = 0;
    }

    int base_frame = moving ? walk_bases[dir] : idle_bases[dir];
    int frame_in_anim = (_anim_frame_counter / anim_speed) % frames_per_anim;
    int tile_index = base_frame + frame_in_anim;

    item.tiles().set_tiles_ref(bn::sprite_items::eris.tiles_item(), tile_index);

    _anim_frame_counter += elapsed_frames;
}

str::Scene RoomViewer::execute()
{
    bn::bg_palettes::set_transparent_color(bn::color(2, 2, 4));
    _models.set_shape_oam_start_index(SHAPE_OAM_START_INDEX);
    int previous_reserved_sprite_handles = bn::sprites::reserved_handles_count();
    int required_reserved_sprite_handles = ROOM_VIEWER_RESERVED_HANDLES;

    if(previous_reserved_sprite_handles < required_reserved_sprite_handles)
    {
        bn::sprites::set_reserved_handles_count(required_reserved_sprite_handles);
    }

    // Keep spawn deterministic for debugging: start from the painting-facing view.
    _corner_index = SPAWN_CORNER_INDEX;
    _player_dir = SPAWN_PLAYER_DIR;
    _player_facing_left = SPAWN_PLAYER_FACING_LEFT;
    _player_moving = false;

    int current_room = SPAWN_ROOM_ID;
    _models.load_colors(bn::span<const bn::color>(room_viewer_colors, 10));

    fr::model_3d* room_models[NUM_ROOMS] = {};
    fr::model_3d* books_ptr = nullptr;
    fr::model_3d* potted_plant_ptr = nullptr;
    fr::model_3d* transition_books_ptr = nullptr;
    fr::model_3d* transition_potted_plant_ptr = nullptr;

    fr::point_3d room_base_pos(0, 96, 16);
    bool player_sprite_created = false;

    corner_matrix all_corners[4];
    compute_corner_matrices(all_corners);
    const corner_matrix base_corner = all_corners[0];

    int current_view_angle = -_corner_index * QUARTER_TURN_ANGLE;
    int last_oriented_view_angle = current_view_angle;
    int target_view_angle = current_view_angle;
    int committed_heading_angle = normalize_angle(current_view_angle - CAMERA_BEHIND_OFFSET_ANGLE);
    bool has_committed_heading = false;
    int start_recenter_boost_frames = 0;
    int camera_initial_lock_frames = CAMERA_INITIAL_LOCK_FRAMES;
    int retarget_cooldown_frames = 0;
    int painting_update_cooldown_frames = 0;
    int idle_follow_timer = 0;
    int last_movement_heading = 0;
    bool has_last_movement_heading = false;
    int player_world_linear_dir =
        wrap_linear8(dir_to_linear8(_player_dir, _player_facing_left) - view_angle_steps_8(current_view_angle));
    bn::fixed world_anchor_x = room_center_x(current_room);
    bn::fixed world_anchor_y = room_center_y(current_room);
    bool door_transition_active = false;
    int door_transition_elapsed = 0;
    int door_transition_target_room = current_room;
    int door_transition_furniture_room = -1;
    room_preview_mode preview_mode = room_preview_mode::all_connected;
    bool debug_hide_room_models = false;
    int preview_mode_low_fps_samples = 0;
    int preview_mode_high_fps_samples = 0;
    int preview_mode_change_cooldown_frames = 0;
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

    str::Minimap* minimap = new str::Minimap();
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

        auto mark_preview_rooms = [&](int anchor_room) {
            should_exist[anchor_room] = true;

            if(preview_mode == room_preview_mode::off)
            {
                return;
            }

            if(preview_mode == room_preview_mode::preferred_only)
            {
                int preferred_neighbor = preferred_neighbor_room_for_corner(anchor_room, _corner_index);
                if(preferred_neighbor >= 0)
                {
                    should_exist[preferred_neighbor] = true;
                }

                return;
            }

            int east_neighbor = neighbor_room_for_door(anchor_room, door_direction::east);
            int west_neighbor = neighbor_room_for_door(anchor_room, door_direction::west);
            int south_neighbor = neighbor_room_for_door(anchor_room, door_direction::south);
            int north_neighbor = neighbor_room_for_door(anchor_room, door_direction::north);

            if(east_neighbor >= 0)
            {
                should_exist[east_neighbor] = true;
            }

            if(west_neighbor >= 0)
            {
                should_exist[west_neighbor] = true;
            }

            if(south_neighbor >= 0)
            {
                should_exist[south_neighbor] = true;
            }

            if(north_neighbor >= 0)
            {
                should_exist[north_neighbor] = true;
            }
        };

        mark_preview_rooms(current_room);

        if(door_transition_active)
        {
            mark_preview_rooms(door_transition_target_room);
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

        // Current room first, then adjacent rooms while budget allows.
        ensure_room_model(current_room);

        for(int room_id = 0; room_id < NUM_ROOMS; ++room_id)
        {
            if(room_id != current_room)
            {
                ensure_room_model(room_id);
            }
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

    bn::fixed cam_dist = 274;

    auto update_camera = [&]() {
        _camera.set_position(fr::point_3d(0, cam_dist, 0));
        paintings_need_update = ENABLE_PAINTING_QUADS;
    };

    _camera.set_phi(0);
    update_camera();

    _player_fx = -20;
    _player_fy = 20;
    _player_fz = -10;

    fr::sprite_3d_item player_sprite_item(bn::sprite_items::eris, 8);
    fr::sprite_3d& player_sprite = _models.create_sprite(player_sprite_item);
    player_sprite_created = true;
    player_sprite.set_scale(2);

    // --- NPC sprites for big rooms (Room 1 and Room 5) ---
    fr::sprite_3d_item npc_sprite_item_a(bn::sprite_items::villager, 0);
    fr::sprite_3d_item npc_sprite_item_b(bn::sprite_items::villager, 0);

    // Room 5 NPC gets unique palette recolored to teal/green
    npc_sprite_item_b.palette() = bn::sprite_items::villager.palette_item().create_new_palette();
    npc_sprite_item_b.palette().set_color(NPC_PALETTE_HAT_INDEX_0, npc_room5_hat_color_0);
    npc_sprite_item_b.palette().set_color(NPC_PALETTE_HAT_INDEX_1, npc_room5_hat_color_1);
    npc_sprite_item_b.palette().set_color(NPC_PALETTE_HAT_INDEX_2, npc_room5_hat_color_2);

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
            if(room_models[BIG_ROOM_A])
            {
                npc_sprite_a_ptr->set_position(
                    transform_global_point(cm,
                                           room_center_x(BIG_ROOM_A) + NPC_FX - world_anchor_x,
                                           room_center_y(BIG_ROOM_A) + NPC_FY - world_anchor_y,
                                           NPC_FZ));
            }
            else
            {
                npc_sprite_a_ptr->set_position(offscreen_pos);
            }
        }
        if(npc_sprite_b_ptr)
        {
            if(room_models[BIG_ROOM_B])
            {
                npc_sprite_b_ptr->set_position(
                    transform_global_point(cm,
                                           room_center_x(BIG_ROOM_B) + NPC_FX - world_anchor_x,
                                           room_center_y(BIG_ROOM_B) + NPC_FY - world_anchor_y,
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
        if(debug_hide_room_models || ! room_models[current_room])
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

            int area_bottom = tri_area2(p0, p1, p2);
            int area_top = tri_area2(p0, p2, p3);

            if(int_abs(area_bottom) < PAINTING_MIN_TRI_AREA2 || int_abs(area_top) < PAINTING_MIN_TRI_AREA2)
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
            painting_a_quad.set_points(a2_screen, a3_screen, a0_screen, a1_screen);
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
            painting_b_quad.set_points(b0_screen, b1_screen, b2_screen, b3_screen);
        }
        else
        {
            painting_b_quad.set_visible(false);
        }
    };

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
    int current_fps = 60;
    int fps_sample_updates = 0;
    int fps_sample_refreshes = 0;

    auto refresh_overlay_text = [&](int room_id, int fps, int vertices_count) {
        _text_sprites.clear();
        int yaw_degrees = (normalize_angle(current_view_angle) * 360 + 32768) / 65536;
        if(yaw_degrees > 180)
        {
            yaw_degrees -= 360;
        }

        auto generation_failed = [&]() {
            // Avoid hard asserts from sprite tile exhaustion when toggling overlay.
            _text_sprites.clear();
            text_debug_mode = _debug_mode;
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
        };

        if(_debug_mode)
        {
            int current_fx = _player_fx.integer();
            int current_fy = _player_fy.integer();

            bn::string<32> line1;
            bn::ostringstream stream1(line1);
            stream1 << "F:" << current_fx << "," << current_fy << " D:" << _player_dir;
            if(! tg.generate_optional(0, -72, line1, _text_sprites))
            {
                generation_failed();
                return;
            }

            bn::string<32> line2;
            bn::ostringstream stream2(line2);
            stream2 << "Room:" << room_id << " C:" << _corner_index << " Y:" << yaw_degrees;
            if(! tg.generate_optional(0, -60, line2, _text_sprites))
            {
                generation_failed();
                return;
            }

            bn::string<32> line3;
            bn::ostringstream stream3(line3);
            stream3 << "FPS:" << fps << " V:" << vertices_count << "/" << fr::models_3d::max_vertices();
            if(! tg.generate_optional(0, -48, line3, _text_sprites))
            {
                generation_failed();
                return;
            }

            bn::string<32> line4;
            bn::ostringstream stream4(line4);
            stream4 << "Prev:" << room_preview_mode_name(preview_mode)
                    << " A:" << (ROOM_PREVIEW_AUTO_ADJUST ? "ON" : "OFF")
                    << " RM:" << (debug_hide_room_models ? "OFF" : "ON");

            if(ROOM_PREVIEW_AUTO_ADJUST)
            {
                stream4 << " T:" << ROOM_PREVIEW_TARGET_FPS;
            }

            if(! tg.generate_optional(0, -36, line4, _text_sprites))
            {
                generation_failed();
                return;
            }

            #if BN_CFG_PROFILER_ENABLED
                if(! tg.generate_optional(0, 72, "SEL+START:Profiler B:Exit", _text_sprites))
                {
                    generation_failed();
                    return;
                }
            #else
                if(! tg.generate_optional(0, 72, "Profiler:OFF B:Exit", _text_sprites))
                {
                    generation_failed();
                    return;
                }
            #endif

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
        }
        else
        {
            if(! tg.generate_optional(0, -72, "ROOM VIEWER", _text_sprites))
            {
                generation_failed();
                return;
            }

            if(! tg.generate_optional(0, 72, "L/R:Zoom START:Recenter B:Exit", _text_sprites))
            {
                generation_failed();
                return;
            }
            text_fps = fps;
            text_vertices = vertices_count;
            text_preview_mode = int(preview_mode);
            text_hide_room_models = int(debug_hide_room_models);
        }

        text_debug_mode = _debug_mode;
    };

    BN_PROFILER_RESET();

    #if BN_CFG_PROFILER_ENABLED
        int auto_profile_frames_left = STR_ROOM_VIEWER_AUTO_PROFILE_FRAMES;
    #endif

    refresh_overlay_text(current_room, current_fps, _models.vertices_count());

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

        #if BN_CFG_PROFILER_ENABLED
            if(auto_profile_frames_left > 0)
            {
                --auto_profile_frames_left;

                if(auto_profile_frames_left == 0)
                {
                    bn::profiler::show();
                }
            }
        #endif

        int elapsed_frames = bn::clamp(frame_cost, 1, 4);

        if(camera_initial_lock_frames > 0)
        {
            camera_initial_lock_frames -= elapsed_frames;

            if(camera_initial_lock_frames < 0)
            {
                camera_initial_lock_frames = 0;
            }
        }

        if(retarget_cooldown_frames > 0)
        {
            retarget_cooldown_frames -= elapsed_frames;

            if(retarget_cooldown_frames < 0)
            {
                retarget_cooldown_frames = 0;
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

        if(bn::keypad::b_pressed())
        {
            delete minimap;
            minimap = nullptr;
            clear_room_decor_models(books_ptr, potted_plant_ptr);
            clear_room_decor_models(transition_books_ptr, transition_potted_plant_ptr);

            _models.destroy_sprite(player_sprite);

            for(int room_id = 0; room_id < NUM_ROOMS; ++room_id)
            {
                if(room_models[room_id])
                {
                    _models.destroy_dynamic_model(*room_models[room_id]);
                    room_models[room_id] = nullptr;
                }
            }

            #if BN_CFG_PROFILER_ENABLED && ! FR_DETAILED_PROFILE
                BN_PROFILER_START("room_models_update");
                _models.update(_camera);
                BN_PROFILER_STOP();
            #else
                _models.update(_camera);
            #endif

            if(bn::sprites::reserved_handles_count() != previous_reserved_sprite_handles)
            {
                bn::sprites::set_reserved_handles_count(previous_reserved_sprite_handles);
            }

            bn::core::update();
            return str::Scene::ROOM_VIEWER;
        }

        #if BN_CFG_PROFILER_ENABLED
            if(_debug_mode && bn::keypad::select_held() && bn::keypad::start_pressed())
            {
                bn::profiler::show();
            }
        #endif

        bool camera_unlocked = camera_initial_lock_frames == 0;
        bool door_approach_lock = near_door_approach(current_room, _player_fx, _player_fy);
        bool camera_steering_enabled = camera_unlocked && !door_approach_lock;

        if(!camera_steering_enabled)
        {
            start_recenter_boost_frames = 0;
            retarget_cooldown_frames = 0;
            idle_follow_timer = 0;
        }
        if(camera_steering_enabled && !door_transition_active && bn::keypad::start_pressed() && !bn::keypad::select_held())
        {
            int heading_for_recenter = has_committed_heading ?
                committed_heading_angle :
                normalize_angle(current_view_angle - CAMERA_BEHIND_OFFSET_ANGLE);
            target_view_angle = normalize_angle(heading_for_recenter + CAMERA_BEHIND_OFFSET_ANGLE);
            start_recenter_boost_frames = CAMERA_START_BOOST_FRAMES;
        }

        {
            bn::fixed old_dist = cam_dist;
            if(bn::keypad::l_held()) cam_dist = bn::max(bn::fixed(100), cam_dist - 3);
            else if(bn::keypad::r_held()) cam_dist = bn::min(bn::fixed(500), cam_dist + 3);
            if(cam_dist != old_dist)
            {
                update_camera();
            }
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
            else
            {
                _debug_mode = !_debug_mode;
            }
        }

        if(door_transition_active)
        {
            door_transition_elapsed += elapsed_frames;

            if(door_transition_elapsed >= DOOR_TRANSITION_DURATION_FRAMES)
            {
                door_transition_active = false;
                current_room = door_transition_target_room;
                _player_fx = door_transition_target_local_x;
                _player_fy = door_transition_target_local_y;
                world_anchor_x = door_transition_target_anchor_x;
                world_anchor_y = door_transition_target_anchor_y;
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

            for(int step = 0; step < elapsed_frames; ++step)
            {
                bn::fixed new_fx = bn::clamp(_player_fx + dfx, FLOOR_MIN, FLOOR_MAX);
                bn::fixed new_fy = bn::clamp(_player_fy + dfy, FLOOR_MIN, FLOOR_MAX);

                if(!overlaps_any_furniture(current_room, new_fx, new_fy))
                {
                    _player_fx = new_fx;
                    _player_fy = new_fy;
                }
                else
                {
                    bn::fixed slide_fx = bn::clamp(_player_fx + dfx, FLOOR_MIN, FLOOR_MAX);
                    bn::fixed slide_fy = _player_fy;
                    if(!overlaps_any_furniture(current_room, slide_fx, slide_fy))
                    {
                        _player_fx = slide_fx;
                    }
                    else
                    {
                        slide_fx = _player_fx;
                        slide_fy = bn::clamp(_player_fy + dfy, FLOOR_MIN, FLOOR_MAX);
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
                    break;
                }
            }

            actual_move_dx = _player_fx - old_player_fx;
            actual_move_dy = _player_fy - old_player_fy;
        }

        bool view_angle_changed = false;

        if(actual_move_dx != 0 || actual_move_dy != 0)
        {
            // While moving: track heading but do NOT update target_view_angle.
            last_movement_heading = quantize_heading_angle_8(
                bn::atan2(actual_move_dy.data(), actual_move_dx.data()).data());
            has_last_movement_heading = true;
            idle_follow_timer = 0;
        }
        else
        {
            idle_follow_timer += elapsed_frames;
        }

        if(camera_steering_enabled && !door_transition_active)
        {
            // Only retarget camera after player has been idle for ~1 second.
            if(has_last_movement_heading &&
               idle_follow_timer >= CAMERA_IDLE_FOLLOW_DELAY_FRAMES)
            {
                int new_target = normalize_angle(last_movement_heading + CAMERA_BEHIND_OFFSET_ANGLE);
                int committed_delta = int_abs(shortest_angle_delta(
                    has_committed_heading ? committed_heading_angle : last_movement_heading,
                    last_movement_heading));

                if(!has_committed_heading || committed_delta >= CAMERA_MIN_RETARGET_DELTA)
                {
                    committed_heading_angle = last_movement_heading;
                    has_committed_heading = true;
                    target_view_angle = new_target;
                    retarget_cooldown_frames = CAMERA_RETARGET_COOLDOWN_FRAMES;
                }

                has_last_movement_heading = false;
            }

            int angle_delta = shortest_angle_delta(current_view_angle, target_view_angle);
            if(angle_delta != 0)
            {
                bool start_boost = start_recenter_boost_frames > 0;
                bn::fixed gain = start_boost ? CAMERA_START_GAIN : CAMERA_FOLLOW_GAIN;
                int max_step = start_boost ? CAMERA_START_MAX_STEP : CAMERA_FOLLOW_MAX_STEP;
                int abs_angle_delta = int_abs(angle_delta);
                int step = (bn::fixed(abs_angle_delta) * gain).round_integer();
                step = bn::max(step, CAMERA_MIN_STEP);
                step = bn::min(step, max_step);

                if(step >= abs_angle_delta || abs_angle_delta <= CAMERA_SNAP_EPSILON)
                {
                    step = abs_angle_delta;
                }

                current_view_angle = normalize_angle(current_view_angle + (angle_delta > 0 ? step : -step));
                view_angle_changed = true;
            }

            if(start_recenter_boost_frames > 0)
            {
                start_recenter_boost_frames -= elapsed_frames;

                if(start_recenter_boost_frames < 0)
                {
                    start_recenter_boost_frames = 0;
                }
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

        player_sprite.set_horizontal_flip(facing_left);
        _player_facing_left = facing_left;
        _update_player_anim_tiles(player_sprite_item, moving || door_transition_active, dir, elapsed_frames);

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
        bool near_npc_a = !npc_dialog.is_active() && (current_room == BIG_ROOM_A) &&
            (bn::abs(_player_fx - NPC_FX) + bn::abs(_player_fy - NPC_FY) < NPC_INTERACT_DIST);
        bool near_npc_b = !npc_dialog.is_active() && (current_room == BIG_ROOM_B) &&
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

        bool text_dirty = _debug_mode != text_debug_mode;
        int current_vertices = _models.vertices_count();
        int current_yaw_deg = (normalize_angle(current_view_angle) * 360 + 32768) / 65536;
        if(current_yaw_deg > 180)
        {
            current_yaw_deg -= 360;
        }

        if(_debug_mode)
        {
            int current_fx = _player_fx.integer();
            int current_fy = _player_fy.integer();

            if(current_fx != text_fx || current_fy != text_fy ||
               _player_dir != text_dir || current_room != text_room || _corner_index != text_corner ||
               current_yaw_deg != text_yaw_deg ||
               int(preview_mode) != text_preview_mode ||
               int(debug_hide_room_models) != text_hide_room_models ||
               current_fps != text_fps || current_vertices != text_vertices)
            {
                text_dirty = true;
            }
        }

        if(text_dirty && !npc_dialog.is_active())
        {
            refresh_overlay_text(current_room, current_fps, current_vertices);
        }

        // --- Minimap update ---
        if(minimap)
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

        #if BN_CFG_PROFILER_ENABLED && ! FR_DETAILED_PROFILE
            BN_PROFILER_START("room_models_update");
            _models.update(_camera);
            BN_PROFILER_STOP();
        #else
            _models.update(_camera);
        #endif
        bn::core::update();
    }
}

}









