#include "str_scene_room_viewer.h"

#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_bg_palettes.h"
#include "bn_bg_tiles.h"
#include "bn_color.h"
#include "bn_sprites.h"
#include "bn_display.h"
#include "bn_sprite_affine_mat_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_affine_mat_attributes.h"
#include "bn_fixed.h"
#include "bn_math.h"
#include "bn_algorithm.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_map_item.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_map_cell.h"
#include "bn_regular_bg_map_cell_info.h"
#include "bn_regular_bg_ptr.h"
#include "bn_point.h"
#include "bn_bg_palette_items_dialog_font_palette.h"
#include "bn_regular_bg_tiles_items_dialog_font_tiles.h"
#include "bn_sprite_items_player_idle.h"
#include "bn_sprite_items_player_walk.h"
#include "bn_sprite_items_villager.h"
#include "bn_sprite_palette_ptr.h"
#include "bn_sprite_items_escaping_criticism_wall_bottom.h"
#include "bn_sprite_items_escaping_criticism_wall_top.h"
#include "bn_sprite_items_mr_and_mrs_andrews_wall_bottom.h"
#include "bn_sprite_items_mr_and_mrs_andrews_wall_top.h"
#include "str_bg_dialog.h"
#include "str_minimap.h"

#include "fr_sin_cos.h"
#include "fr_div_lut.h"
#include "models/str_model_3d_items_room.h"
#include "models/str_model_3d_items_books.h"
#include "private/viewer/str_room_renderer.h"

namespace {
    namespace rv = str::viewer;

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

    struct dir_face
    {
        int dir;
        bool facing_left;
    };

    constexpr dir_face screen_dir_faces[3][3] = {
        { { 3, true }, { 4, false }, { 3, false } },
        { { 2, true }, { 0, false }, { 2, false } },
        { { 1, true }, { 0, false }, { 1, false } }
    };

    dir_face dir_face_from_screen_delta(int screen_dx, int screen_dy)
    {
        return screen_dir_faces[screen_dy + 1][screen_dx + 1];
    }

    int minimap_dir_from_player_dir(int dir, bool facing_left)
    {
        if(dir == 4 || dir == 3)
        {
            return 0;
        }

        if(dir == 0 || dir == 1)
        {
            return 1;
        }

        return facing_left ? 2 : 3;
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

    constexpr bn::fixed ROOM_WALL_TOP_Z = bn::fixed(-50);

    struct room_spec
    {
        const fr::model_3d_item& model;
        bn::fixed center_x;
        bn::fixed center_y;
        bn::fixed half_x;
        bn::fixed half_y;
        const fr::model_3d_item* decor_model;
        bn::fixed decor_x;
        bn::fixed decor_y;
        bn::fixed decor_half_width;
        bn::fixed decor_half_depth;
    };

    constexpr room_spec rooms[NUM_ROOMS] = {
        {
            str::model_3d_items::room_0,
            bn::fixed(0),
            bn::fixed(0),
            bn::fixed(90),
            bn::fixed(60),
            nullptr,
            0,
            0,
            0,
            0
        },
        {
            str::model_3d_items::room_1,
            bn::fixed(60),
            bn::fixed(120),
            bn::fixed(60),
            bn::fixed(60),
            &str::model_3d_items::books,
            bn::fixed(24),
            bn::fixed(-12),
            bn::fixed(4),
            bn::fixed(4)
        }
    };

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
    constexpr bn::fixed ROOM_PLAYABLE_EDGE_INSET = bn::fixed(5);
    constexpr bn::fixed SHARED_DOOR_WORLD_X = bn::fixed(45);

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

    int int_abs(int value)
    {
        return value >= 0 ? value : -value;
    }

    bn::fixed room_center_x(int room_id)
    {
        return rooms[room_id].center_x;
    }

    bn::fixed room_center_y(int room_id)
    {
        return rooms[room_id].center_y;
    }

    bn::fixed room_half_extent_x(int room_id)
    {
        return rooms[room_id].half_x;
    }

    bn::fixed room_half_extent_y(int room_id)
    {
        return rooms[room_id].half_y;
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

    bool room_has_decor(int room_id)
    {
        return rooms[room_id].decor_model;
    }

    const fr::model_3d_item& room_decor_model(int room_id)
    {
        return *rooms[room_id].decor_model;
    }

    bool overlaps_room_decor(int room_id, bn::fixed px, bn::fixed py)
    {
        const room_spec& room = rooms[room_id];

        if(! room.decor_model)
        {
            return false;
        }

        return bn::abs(px - room.decor_x) < room.decor_half_width &&
               bn::abs(py - room.decor_y) < room.decor_half_depth;
    }

    // Check if a door transition should occur and return the new room id (-1 if no transition)
    // Also sets new_local_x/y for the player position in the new room
    int check_door_transition(int current_room, bn::fixed local_x, bn::fixed local_y,
                              bn::fixed& new_local_x, bn::fixed& new_local_y)
    {
        bn::fixed door_center = SHARED_DOOR_WORLD_X - room_center_x(current_room);

        if(current_room == 0 &&
           local_y >= room_floor_max_y(current_room) &&
           bn::abs(local_x - door_center) <= DOOR_HALF_WIDTH)
        {
            bn::fixed shared_world_x = room_center_x(current_room) + local_x;
            new_local_x = bn::clamp(shared_world_x - room_center_x(1), room_floor_min_x(1), room_floor_max_x(1));
            new_local_y = room_floor_min_y(1);
            return 1;
        }

        if(current_room == 1 &&
           local_y <= room_floor_min_y(current_room) &&
           bn::abs(local_x - door_center) <= DOOR_HALF_WIDTH)
        {
            bn::fixed shared_world_x = room_center_x(current_room) + local_x;
            new_local_x = bn::clamp(shared_world_x - room_center_x(0), room_floor_min_x(0), room_floor_max_x(0));
            new_local_y = room_floor_max_y(0);
            return 0;
        }

        return -1;
    }

    bool near_door_approach(int current_room, bn::fixed local_x, bn::fixed local_y)
    {
        bn::fixed door_lane_half_width = DOOR_HALF_WIDTH + DOOR_APPROACH_LANE_MARGIN;
        bn::fixed door_center = SHARED_DOOR_WORLD_X - room_center_x(current_room);

        if(current_room == 0)
        {
            return local_y >= room_floor_max_y(current_room) - DOOR_APPROACH_EDGE_MARGIN &&
                   bn::abs(local_x - door_center) <= door_lane_half_width;
        }

        return local_y <= room_floor_min_y(current_room) + DOOR_APPROACH_EDGE_MARGIN &&
               bn::abs(local_x - door_center) <= door_lane_half_width;
    }

    const fr::model_3d_item& get_room_model(int room_id)
    {
        return rooms[room_id].model;
    }

}

namespace str
{

namespace
{

    struct player_anim_state
    {
        bool moving = false;
        int dir = SPAWN_PLAYER_DIR;
        bool facing_left = SPAWN_PLAYER_FACING_LEFT;
        int frame_counter = 0;
    };

    void update_player_anim_tiles(rv::SpriteItem& item, player_anim_state& state, bool moving,
                                  int dir, bool facing_left, int frame_advance)
    {
        const bn::sprite_item& player_sprite_item =
            moving ? bn::sprite_items::player_walk : bn::sprite_items::player_idle;
        int frames_per_angle = moving ? PLAYER_WALK_FRAMES_PER_ANGLE : PLAYER_IDLE_FRAMES_PER_ANGLE;

        if(state.moving != moving || state.dir != dir || state.facing_left != facing_left)
        {
            state.moving = moving;
            state.dir = dir;
            state.facing_left = facing_left;
            state.frame_counter = 0;
            item.palette().set_colors(player_sprite_item.palette_item());
        }

        int base_frame = player_angle_row(dir, facing_left) * frames_per_angle;
        int frame_in_anim = (state.frame_counter / PLAYER_ANIM_SPEED) % frames_per_angle;
        int tile_index = base_frame + frame_in_anim;

        item.tiles().set_tiles_ref(player_sprite_item.tiles_item(), tile_index);

        state.frame_counter += frame_advance;
    }
}

[[noreturn]] void run_room_viewer_runtime()
{
    rv::Camera _camera;
    rv::Renderer& _models = *new rv::Renderer();
    int _corner_index = SPAWN_CORNER_INDEX;
    bn::fixed _player_fx = 0;
    bn::fixed _player_fy = 0;
    bn::fixed _player_fz = 0;
    player_anim_state player_anim;

    bn::bg_palettes::set_transparent_color(bn::color(2, 2, 4));

    // Keep spawn deterministic for debugging: start from the painting-facing view.
    int current_room = SPAWN_ROOM_ID;
    _models.load_colors(str::model_3d_items::room_model_colors);

    rv::Model* room_models[NUM_ROOMS] = {};
    rv::Model* decor_ptr = nullptr;
    int decor_room = -1;

    fr::point_3d room_base_pos(0, 96, 8);

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
        wrap_linear8(dir_to_linear8(player_anim.dir, player_anim.facing_left) -
                     view_angle_steps_8(current_view_angle));
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
    auto set_model_rotation = [](rv::Model& m, const corner_matrix& cm) {
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

    auto sync_room_models = [&]() {
        bool should_exist[NUM_ROOMS] = {};

        should_exist[current_room] = true;

        if(door_transition_active)
        {
            should_exist[door_transition_target_room] = true;
        }

        for(int room_id = 0; room_id < NUM_ROOMS; ++room_id)
        {
            if(!should_exist[room_id] && room_models[room_id])
            {
                _models.destroy_model(*room_models[room_id]);
                room_models[room_id] = nullptr;
            }
        }

        auto ensure_room_model = [&](int room_id) {
            if(!should_exist[room_id])
            {
                return;
            }

            rv::Model* room_model = room_models[room_id];

            if(!room_model)
            {
                room_model = &_models.create_model(get_room_model(room_id));
                room_models[room_id] = room_model;
            }

            bool room_perspective_mode = room_id == current_room;
            room_model->set_layering_mode(
                room_perspective_mode ? rv::Model::LayeringMode::room_perspective :
                                        rv::Model::LayeringMode::room_floor_only);
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

        int next_decor_room = -1;

        if(door_transition_active)
        {
            if(room_has_decor(door_transition_target_room))
            {
                next_decor_room = door_transition_target_room;
            }
        }
        else if(room_has_decor(current_room))
        {
            next_decor_room = current_room;
        }

        if(next_decor_room != decor_room)
        {
            if(decor_ptr)
            {
                _models.destroy_model(*decor_ptr);
                decor_ptr = nullptr;
            }

            decor_room = next_decor_room;

            if(decor_room >= 0)
            {
                decor_ptr = &_models.create_model(room_decor_model(decor_room));
            }
        }

        if(decor_ptr)
        {
            decor_ptr->set_layering_mode(rv::Model::LayeringMode::none);
            decor_ptr->set_depth_bias(decor_room == current_room ? 0 : TRANSITION_DECOR_DEPTH_BIAS);
            decor_ptr->set_double_sided(false);
        }
    };

    rv::Sprite* npc_sprite_a_ptr = nullptr;
    rv::Sprite* npc_sprite_b_ptr = nullptr;

    auto update_all_orientations = [&]() {
        corner_matrix cm = rotate_corner_matrix(base_corner, current_view_angle);

        for(int room_id = 0; room_id < NUM_ROOMS; ++room_id)
        {
            rv::Model* room_model = room_models[room_id];

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

        if(decor_ptr)
        {
            const room_spec& decor = rooms[decor_room];
            set_model_rotation(*decor_ptr, cm);
            decor_ptr->set_position(
                transform_global_point(cm,
                                       room_center_x(decor_room) + decor.decor_x - world_anchor_x,
                                       room_center_y(decor_room) + decor.decor_y - world_anchor_y,
                                       0));
        }
    };

    bool paintings_need_update = ENABLE_PAINTING_QUADS;
    auto update_orientations_and_paintings = [&]() {
        update_all_orientations();
        last_oriented_view_angle = current_view_angle;
        paintings_need_update = ENABLE_PAINTING_QUADS;
    };

    sync_room_models();
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

    auto project_point = [&](const fr::point_3d& point,
                             bn::fixed camera_x, bn::fixed camera_y, bn::fixed camera_z,
                             bn::point& output) {
        constexpr int focal_length_shift = rv::focal_length_shift;
        constexpr int near_plane = 24 * 256 * 16;

        bn::fixed vry = point.y() - camera_y;
        int vcz = -vry.data();

        if(vcz < near_plane)
        {
            return false;
        }

        bn::fixed vrx = (point.x() - camera_x) / 16;
        bn::fixed vrz = (point.z() - camera_z) / 16;
        int vcx = (vrx.unsafe_multiplication(_camera.right_axis().x()) +
                   vrz.unsafe_multiplication(_camera.right_axis().z())).data();
        int vcy = -(vrx.unsafe_multiplication(_camera.up_axis().x()) +
                    vrz.unsafe_multiplication(_camera.up_axis().z())).data();

        int scale = int((fr::div_lut_ptr[vcz >> 10] << (focal_length_shift - 8)) >> 6);
        output.set_x((vcx * scale) >> 16);
        output.set_y((vcy * scale) >> 16);
        return true;
    };

    auto project_point_for_camera_distance = [&](const fr::point_3d& point, bn::fixed camera_y, bn::point& output) {
        return project_point(point, 0, camera_y, 0, output);
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

    _camera.set_yaw(0);
    update_camera();

    _player_fx = -20;
    _player_fy = 20;
    _player_fz = -10;

    rv::SpriteItem player_sprite_item(
        bn::sprite_items::player_idle,
        player_angle_row(SPAWN_PLAYER_DIR, SPAWN_PLAYER_FACING_LEFT) * PLAYER_IDLE_FRAMES_PER_ANGLE);
    rv::Sprite& player_sprite = _models.create_sprite(player_sprite_item);
    player_sprite.set_scale(PLAYER_SPRITE_SCALE);
    player_sprite.set_horizontal_flip(false);

    // --- NPC sprites for the two-room layout ---
    rv::SpriteItem npc_sprite_item_a(bn::sprite_items::villager, 0);
    rv::SpriteItem npc_sprite_item_b(bn::sprite_items::villager, 0);

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
        const fr::point_3d& camera_position = _camera.position();
        return project_point(point,
                             camera_position.x(),
                             camera_position.y(),
                             camera_position.z(),
                             output);
    };

    auto update_painting_quads = [&]() {
        if(door_transition_active || ! room_models[current_room])
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

    update_player_sprite_position();

    if(paintings_need_update)
    {
        update_painting_quads();
        paintings_need_update = false;
    }

    // --- NPC dialog (BG-based, no sprite VRAM) ---
    str::BgDialog npc_dialog;
    bool prev_near_npc = false;
    bool near_any_npc = false;

    int door_transition_frame_budget = 0;
    int player_movement_frame_budget = 0;

    while(true)
    {
        int frame_cost = bn::core::last_missed_frames() + 1;
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
                sync_room_models();
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
            int input_dx = screen_dx > 0 ? 1 : (screen_dx < 0 ? -1 : 0);
            int input_dy = screen_dy > 0 ? 1 : (screen_dy < 0 ? -1 : 0);
            dir_face movement_face = dir_face_from_screen_delta(input_dx, input_dy);
            dir = movement_face.dir;
            facing_left = movement_face.facing_left;

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

                if(!overlaps_room_decor(current_room, new_fx, new_fy))
                {
                    _player_fx = new_fx;
                    _player_fy = new_fy;
                }
                else
                {
                    bn::fixed slide_fx = bn::clamp(_player_fx + dfx,
                                                   room_floor_min_x(current_room), room_floor_max_x(current_room));
                    bn::fixed slide_fy = _player_fy;
                    if(!overlaps_room_decor(current_room, slide_fx, slide_fy))
                    {
                        _player_fx = slide_fx;
                    }
                    else
                    {
                        slide_fx = _player_fx;
                        slide_fy = bn::clamp(_player_fy + dfy,
                                             room_floor_min_y(current_room), room_floor_max_y(current_room));
                        if(!overlaps_room_decor(current_room, slide_fx, slide_fy))
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
                    update_orientations_and_paintings();
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
        update_player_anim_tiles(player_sprite_item, player_anim, moving || door_transition_active,
                                 dir, facing_left, player_frame_advance);

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

            minimap->update(player_world_pos, minimap_dir_from_player_dir(dir, facing_left));
        }

        _models.render(_camera);
        bn::core::update();
    }
}

}
