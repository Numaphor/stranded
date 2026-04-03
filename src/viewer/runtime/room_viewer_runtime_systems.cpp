#include "str_scene_room_viewer.h"
#include "private/viewer/runtime/room_viewer_runtime_state.h"
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
#include "private/viewer/runtime/room_viewer_runtime_systems_shared.h"
namespace str
{
[[noreturn]] void run_room_viewer()
{
    rv::Camera _camera;
    rv::Renderer& _models = *new rv::Renderer();
    int _corner_index = SPAWN_CORNER_INDEX;
    bn::fixed _player_fx = 0;
    bn::fixed _player_fy = 0;
    bn::fixed _player_fz = 0;
    player_anim_state player_anim;
    bn::bg_palettes::set_transparent_color(bn::color(2, 2, 4));
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
            room_model->set_double_sided(room_perspective_mode);
        };
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
    rv::SpriteItem npc_sprite_item_a(bn::sprite_items::villager, 0);
    rv::SpriteItem npc_sprite_item_b(bn::sprite_items::villager, 0);
    npc_sprite_item_b.palette() = bn::sprite_items::villager.palette_item().create_new_palette();
    npc_sprite_item_b.palette().set_color(NPC_PALETTE_HAT_INDEX_0, str::rv_runtime_state::npc_room_b_hat_color_0);
    npc_sprite_item_b.palette().set_color(NPC_PALETTE_HAT_INDEX_1, str::rv_runtime_state::npc_room_b_hat_color_1);
    npc_sprite_item_b.palette().set_color(NPC_PALETTE_HAT_INDEX_2, str::rv_runtime_state::npc_room_b_hat_color_2);
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
        {
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
        bool near_npc_a = !npc_dialog.is_active() && (current_room == NPC_ROOM_A) &&
            (bn::abs(_player_fx - NPC_FX) + bn::abs(_player_fy - NPC_FY) < str::rv_runtime_state::NPC_INTERACT_DIST);
        bool near_npc_b = !npc_dialog.is_active() && (current_room == NPC_ROOM_B) &&
            (bn::abs(_player_fx - NPC_FX) + bn::abs(_player_fy - NPC_FY) < str::rv_runtime_state::NPC_INTERACT_DIST);
        near_any_npc = near_npc_a || near_npc_b;
        if(near_any_npc != prev_near_npc)
        {
            if(near_any_npc) { npc_dialog.show_prompt(); }
            else             { npc_dialog.hide_prompt(); }
            prev_near_npc = near_any_npc;
        }
        if(npc_dialog.is_active())
        {
            npc_dialog.update();
        }
        else
        {
            if(bn::keypad::a_pressed() && !door_transition_active)
            {
                if(near_npc_a)
                {
                    str::rv_runtime_state::begin_npc_dialog(npc_dialog, 0);
                }
                else if(near_npc_b)
                {
                    str::rv_runtime_state::begin_npc_dialog(npc_dialog, 1);
                }
            }
        }
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
