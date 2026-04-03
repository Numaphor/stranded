#ifndef STR_ROOM_VIEWER_RUNTIME_SYSTEMS_SHARED_H
#define STR_ROOM_VIEWER_RUNTIME_SYSTEMS_SHARED_H
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
            if(affine_divisor > 0)
            {
                bn::swap(x1, x2);
                bn::swap(y1, y2);
                affine_divisor = x0 * y1 - x0 * y2 - x1 * y0 + x1 * y2 + x2 * y0 - x2 * y1;
            }
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
}
#endif
