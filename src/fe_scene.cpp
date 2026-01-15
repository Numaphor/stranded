#include "fe_scene.h"
#include "fe_scene_world.h"
#include "fe_scene_menu.h"
#include "fe_scene_start.h"
#include "fe_scene_controls.h"
#include "fe_world_state.h"

#include "fe_collision.h"
#include "fe_npc.h"
#include "fe_hitbox.h"
#include "fe_level.h"
#include "fe_constants.h"
#include "fe_direction_utils.h"

#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_log.h"
#include "bn_string.h"
#include "bn_memory.h"
#include "bn_unique_ptr.h"
#include "bn_size.h"
#include "bn_window.h"
#include "bn_rect_window.h"
#include "bn_bg_palettes.h"
#include "bn_bg_tiles.h"
#include "bn_bg_palette_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_map_item.h"
#include "bn_regular_bg_map_cell.h"
#include "bn_regular_bg_map_cell_info.h"
#include "bn_regular_bg_tiles_ptr.h"
#include "bn_sprite_builder.h"
#include "bn_sprite_double_size_mode.h"
#include "bn_sprite_shape_size.h"
#include "bn_sprite_text_generator.h"

#include "bn_sprite_items_hero_sword.h"
#include "bn_bg_palette_items_palette.h"
#include "bn_regular_bg_tiles_items_tiles.h"
#include "bn_affine_bg_items_sword.h"
#include "common_variable_8x8_sprite_font.h"

namespace fe {

    // =========================================================================
    // Helpers
    // =========================================================================

    namespace {
        struct bg_map {
            static const int columns = MAP_COLUMNS;
            static const int rows = MAP_ROWS;
            static const int cells_count = MAP_CELLS_COUNT;
            BN_DATA_EWRAM static bn::regular_bg_map_cell cells[cells_count];
            bn::regular_bg_map_item map_item;
            int _background_tile;
            
            bg_map(int world_id = 0) : map_item(cells[0], bn::size(columns, rows)) {
                _background_tile = 1;
                if (world_id == 1) { _background_tile = 2; }
                for (int x = 0; x < columns; x++) {
                    for (int y = 0; y < rows; y++) {
                        int cell_index = x + y * columns;
                        cells[cell_index] = bn::regular_bg_map_cell(_background_tile);
                    }
                }
            }
        };
        BN_DATA_EWRAM bn::regular_bg_map_cell bg_map::cells[bg_map::cells_count];
    }

    // =========================================================================
    // World Implementation
    // =========================================================================

    World::World() : _player(nullptr),
                     _level(nullptr),
                     _minimap(nullptr),
                     _sword_bg(bn::nullopt),
                     _merchant(nullptr),
                     _player_status_display(nullptr),
                     _camera(bn::nullopt),
                     _last_camera_direction(PlayerMovement::Direction::DOWN),
                     _direction_change_frames(0),
                     _current_world_id(0),
                     _shake_frames(0),
                     _shake_intensity(0),
                     _continuous_fire_frames(0),
                     _zoomed_out(false),
                     _current_zoom_scale(ZOOM_NORMAL_SCALE),
                     _zoom_affine_mat(bn::nullopt),
                     _gun_affine_mat(bn::nullopt),
                     _player_affine_mat(bn::nullopt),
                     _vfx_affine_mat(bn::nullopt) {
        bn::sprite_builder builder(bn::sprite_items::hero_sword);
        builder.set_bg_priority(1);
        _player = new Player(builder.release_build());
        _lookahead_current = bn::fixed_point(0, 0); 
    }

    World::~World() {
        delete _player;
        delete _level;
        delete _minimap;
        delete _merchant; 
    }

    fe::Scene fe::World::execute(bn::fixed_point spawn_location, int world_id) {
        _current_world_id = world_id;
        WorldStateManager &state_manager = WorldStateManager::instance();
        if (state_manager.has_saved_state(world_id)) {
            WorldState saved_state = state_manager.load_world_state(world_id);
            spawn_location = saved_state.player_position; 
        }
        
        bn::camera_ptr camera = bn::camera_ptr::create(0, 0);
        _camera = camera;
        
        bg_map bg_map_obj(world_id);
        bn::regular_bg_tiles_ptr tiles = bn::regular_bg_tiles_items::tiles.create_tiles();
        bn::bg_palette_ptr palette = bn::bg_palette_items::palette.create_palette();
        bn::regular_bg_map_ptr bg_map_ptr = bg_map_obj.map_item.create_map(tiles, palette);
        bn::regular_bg_ptr bg = bn::regular_bg_ptr::create(bg_map_ptr);
        bg.set_camera(camera);
        
        _level = new Level(bg_map_ptr);
        _player->spawn(spawn_location, camera);
        _camera_target_pos = spawn_location;
        camera.set_position(spawn_location.x(), spawn_location.y());
        _lookahead_current = bn::fixed_point(0, 0);
        
        _sword_bg = bn::affine_bg_items::sword.create_bg(0, 0);
        _sword_bg->set_visible(true);
        _sword_bg->set_wrapping_enabled(false);
        _sword_bg->set_camera(camera);
        
        bn::window outside_window = bn::window::outside();
        outside_window.set_show_bg(*_sword_bg, false);
        bn::rect_window internal_window = bn::rect_window::internal();
        internal_window.set_show_bg(*_sword_bg, true);
        internal_window.set_boundaries(-SWORD_HALF_WIDTH, -SWORD_HALF_HEIGHT, SWORD_HALF_WIDTH, SWORD_HALF_HEIGHT);
        
        _minimap = new Minimap(bn::fixed_point(100, -80), bg_map_ptr, camera);
        _player->set_camera(camera);
        
        bn::sprite_text_generator text_generator(common::variable_8x8_sprite_font);
        _init_world_specific_content(world_id, camera, bg, text_generator);
        
        while (true) {
            bn::core::update();
            
            if (bn::keypad::select_held() && bn::keypad::a_pressed()) {
                if (_merchant) { _merchant->set_is_hidden(true); }
                _save_current_state();
                return fe::Scene::MENU; 
            }
            
            if (bn::keypad::select_pressed() &&
                !bn::keypad::a_held() && !bn::keypad::b_held() &&
                !bn::keypad::l_held() && !bn::keypad::r_held()) { _zoomed_out = !_zoomed_out; }
            
            bn::fixed target_scale = _zoomed_out ? ZOOM_OUT_SCALE : ZOOM_NORMAL_SCALE;
            if (_current_zoom_scale != target_scale) {
                bn::fixed diff = target_scale - _current_zoom_scale;
                if (bn::abs(diff) < ZOOM_TRANSITION_SPEED) { _current_zoom_scale = target_scale; }
                else { _current_zoom_scale += diff * ZOOM_TRANSITION_SPEED * 2; }
            }
            
            if (_current_zoom_scale != ZOOM_NORMAL_SCALE) {
                if (!_zoom_affine_mat.has_value()) { _zoom_affine_mat = bn::sprite_affine_mat_ptr::create(); }
                _zoom_affine_mat->set_scale(_current_zoom_scale);
                if (!_gun_affine_mat.has_value()) { _gun_affine_mat = bn::sprite_affine_mat_ptr::create(); }
                _gun_affine_mat->set_scale(_current_zoom_scale);
                if (!_player_affine_mat.has_value()) { _player_affine_mat = bn::sprite_affine_mat_ptr::create(); }
                _player_affine_mat->set_scale(_current_zoom_scale);
                if (!_vfx_affine_mat.has_value()) { _vfx_affine_mat = bn::sprite_affine_mat_ptr::create(); }
                _vfx_affine_mat->set_scale(_current_zoom_scale); 
            } else {
                _zoom_affine_mat.reset();
                _gun_affine_mat.reset();
                _player_affine_mat.reset();
                _vfx_affine_mat.reset(); 
            }
            
            bool merchant_was_talking = false;
            if (_merchant) {
                merchant_was_talking = _merchant->is_talking();
                _merchant->update();
                fe::ZoneManager::set_merchant_zone_center(_merchant->pos());
                bool conversation_active = _merchant->is_talking() || _player->listening();
                fe::ZoneManager::set_merchant_zone_enabled(!conversation_active);
                int merchant_z = -_merchant->pos().y().integer();
                _merchant->set_sprite_z_order(merchant_z); 
            }
            
            if (_merchant && !_merchant->is_talking() && merchant_was_talking) { _player->set_listening(false); }
            
            if (_merchant && fe::Hitbox::is_in_merchant_interaction_zone(_player->pos(), _merchant->pos())) {
                _merchant->set_near_player(true);
                if (bn::keypad::a_pressed() && !merchant_was_talking && !_player->listening()) {
                    _player->set_listening(true);
                    _merchant->talk(); 
                }
            } else {
                if (_merchant) { _merchant->set_near_player(false); }
            }
            
            _player->update();
            _player->update_gun_position(_player->facing_direction());
            
            if (_player->is_firing()) {
                _continuous_fire_frames++;
                bn::fixed intensity_multiplier = bn::min(
                    bn::fixed(_continuous_fire_frames) / GUNFIRE_BUILDUP_FRAMES,
                    bn::fixed(1.0));
                // gun shake not used? calculate anyway or TODO remove if not used in camera shake
                // There is _shake_frames logic later, maybe this sets it?
                // The original code calculated 'current_intensity' but didn't use it except to maybe set _shake_intensity?
                // Ah, looking at original code:
                // bn::fixed current_intensity = ...
                // It was marked as unused variable warning!
                // So I will just increment frames and handle recoil if needed, but remove unused var.
                 if (_player->bullet_just_fired()) { _player->clear_bullet_fired_flag(); }
            } else { 
                _continuous_fire_frames = 0; 
            }
            
            _player->update_z_order();
            bn::fixed_point new_pos = _player->pos();
            if (!fe::ZoneManager::is_position_valid(new_pos)) { _player->revert_position(); }
            if (_minimap) { _minimap->update(_player->pos(), bn::fixed_point(0, 0), _enemies); }
            
            bn::fixed_point player_pos = _player->pos();
            PlayerMovement::Direction facing_dir = _player->facing_direction();
            bn::fixed_point desired_lookahead(0, 0);
            
            switch (facing_dir) {
                case PlayerMovement::Direction::RIGHT: desired_lookahead = bn::fixed_point(CAMERA_LOOKAHEAD_X, 0); break;
                case PlayerMovement::Direction::LEFT: desired_lookahead = bn::fixed_point(-CAMERA_LOOKAHEAD_X, 0); break;
                case PlayerMovement::Direction::UP: desired_lookahead = bn::fixed_point(0, -CAMERA_LOOKAHEAD_Y); break;
                case PlayerMovement::Direction::DOWN: desired_lookahead = bn::fixed_point(0, CAMERA_LOOKAHEAD_Y); break;
                default: break; 
            }
            
            _lookahead_current = _lookahead_current + (desired_lookahead - _lookahead_current) * CAMERA_LOOKAHEAD_SMOOTHING;
            bn::fixed_point camera_target = player_pos + _lookahead_current;
            bn::fixed_point current_camera_pos = _camera.has_value() ? bn::fixed_point(_camera->x(), _camera->y()) : bn::fixed_point(0, 0);
            bn::fixed_point camera_to_target = camera_target - current_camera_pos;
            bn::fixed new_camera_x = current_camera_pos.x();
            bn::fixed new_camera_y = current_camera_pos.y();
            
            if (bn::abs(camera_to_target.x()) > CAMERA_DEADZONE_X) { new_camera_x = camera_target.x() - (camera_to_target.x() > 0 ? CAMERA_DEADZONE_X : -CAMERA_DEADZONE_X); }
            if (bn::abs(camera_to_target.y()) > CAMERA_DEADZONE_Y) { new_camera_y = camera_target.y() - (camera_to_target.y() > 0 ? CAMERA_DEADZONE_Y : -CAMERA_DEADZONE_Y); }
            
            constexpr bn::fixed half_screen_x = 120;
            constexpr bn::fixed half_screen_y = 80;
            constexpr bn::fixed map_min_x = -MAP_OFFSET_X + half_screen_x;
            constexpr bn::fixed map_max_x = MAP_OFFSET_X - half_screen_x;
            constexpr bn::fixed map_min_y = -MAP_OFFSET_Y + half_screen_y;
            constexpr bn::fixed map_max_y = MAP_OFFSET_Y - half_screen_y;
            
            bn::fixed_point new_camera_pos(
                bn::clamp(new_camera_x, map_min_x, map_max_x),
                bn::clamp(new_camera_y, map_min_y, map_max_y));
            
            if (_camera.has_value()) {
                _camera->set_x(bn::fixed(new_camera_pos.x().integer()));
                _camera->set_y(bn::fixed(new_camera_pos.y().integer())); 
            }
            
            if (_sword_bg) {
                constexpr bn::fixed sword_sprite_x = 0;
                constexpr bn::fixed sword_sprite_y = 0;
                bn::rect_window &window_ref = internal_window;
                bn::fixed_point camera_pos(camera.x(), camera.y());
                bn::fixed_point sword_screen_pos = bn::fixed_point(sword_sprite_x, sword_sprite_y) - camera_pos;
                window_ref.set_boundaries(
                    sword_screen_pos.y() - SWORD_HALF_HEIGHT,
                    sword_screen_pos.x() - SWORD_HALF_WIDTH,
                    sword_screen_pos.y() + SWORD_HALF_HEIGHT,
                    sword_screen_pos.x() + SWORD_HALF_WIDTH
                );
                const int sword_priority = (player_pos.y() > sword_sprite_y + 8) ? 2 : 0;
                _sword_bg->set_priority(sword_priority); 
            }
            
            for (int i = 0; i < _enemies.size();) {
                Enemy &enemy = _enemies[i];
                bool player_should_be_ignored = _player->listening() || _player->get_hp() <= 0;
                enemy.update(_player->pos(), *_level, player_should_be_ignored);
                
                if (_player->get_hp() > 0 && !_player->listening()) {
                    Hitbox collision_hitbox = enemy.get_hitbox();
                    Hitbox player_hitbox = _player->get_hitbox();
                    if (fe::Collision::check_bb(player_hitbox, collision_hitbox)) {
                        if (!_player->is_state(PlayerMovement::State::ROLLING)) {
                            _player->take_damage(1);
                            bn::fixed_point knockback_vector = _player->pos() - enemy.get_position();
                            bn::fixed knockback_x = (knockback_vector.x() > 0) ? 10 : -10;
                            bn::fixed_point knockback(knockback_x, 0);
                            _player->set_position(_player->pos() + knockback); 
                        }
                    }
                }
                
                if (_player->has_companion() && !_player->get_companion()->is_dead_independently()) {
                    constexpr int COMPANION_HITBOX_HALF_SIZE = COMPANION_HITBOX_SIZE / 2;
                    PlayerCompanion *companion = _player->get_companion();
                    Hitbox enemy_hitbox = enemy.get_hitbox();
                    bn::fixed_point companion_pos = companion->pos();
                    Hitbox companion_hitbox(companion_pos.x() - COMPANION_HITBOX_HALF_SIZE, companion_pos.y() - COMPANION_HITBOX_HALF_SIZE, COMPANION_HITBOX_SIZE, COMPANION_HITBOX_SIZE);
                    if (fe::Collision::check_bb(companion_hitbox, enemy_hitbox)) { _player->kill_companion(); }
                }
                
                if (_player->bullets().size() > 0) {
                    const auto &bullets = _player->bullets();
                    Hitbox enemy_hitbox = enemy.get_hitbox();
                    for (const auto &bullet : bullets) {
                        if (bullet.is_active()) {
                            Hitbox bullet_hitbox = bullet.get_hitbox();
                            if (bullet_hitbox.collides_with(enemy_hitbox)) {
                                bool damage_from_left = bullet.position().x() < enemy.get_position().x();
                                if (damage_from_left) { enemy.damage_from_left(1); }
                                else { enemy.damage_from_right(1); }
                                const_cast<Bullet &>(bullet).deactivate();
                                break; 
                            }
                        }
                    }
                }
                
                if (_player->is_attacking()) {
                    Hitbox melee_hitbox = _player->get_melee_hitbox();
                    Hitbox enemy_hitbox = enemy.get_hitbox();
                    if (melee_hitbox.collides_with(enemy_hitbox)) {
                        bool damage_from_left = melee_hitbox.x() < enemy.get_position().x();
                        if (damage_from_left) { enemy.damage_from_left(1); }
                        else { enemy.damage_from_right(1); }
                    }
                }
                
                if (enemy.is_ready_for_removal()) { _enemies.erase(_enemies.begin() + i); }
                else { i++; }
            }
            
            if (_player->is_reset_required()) {
                _player->reset();
                _level->reset();
                _enemies.clear();
                delete _minimap;
                _minimap = new Minimap(bn::fixed_point(100, -80), bg_map_ptr, camera);
                _player->spawn(spawn_location, camera);
                _enemies.push_back(Enemy(0, -100, camera, bg, ENEMY_TYPE::SPEARGUARD, 3));
                _enemies.push_back(Enemy(50, -80, camera, bg, ENEMY_TYPE::SPEARGUARD, 3));
                _enemies.push_back(Enemy(-50, -120, camera, bg, ENEMY_TYPE::SPEARGUARD, 3));
                camera.set_position(0, 0);
                continue; 
            }
            
            if (_sword_bg && _current_zoom_scale != ZOOM_NORMAL_SCALE) {
                _sword_bg->set_scale(_current_zoom_scale);
                bn::fixed_point cam_pos = bn::fixed_point(camera.x(), camera.y());
                bn::fixed_point sword_world_pos = bn::fixed_point(0, 0);
                bn::fixed_point offset = sword_world_pos - cam_pos;
                bn::fixed_point scaled_pos = cam_pos + bn::fixed_point(offset.x() * _current_zoom_scale, offset.y() * _current_zoom_scale);
                _sword_bg->set_position(scaled_pos.x(), scaled_pos.y()); 
            } else if (_sword_bg) {
                _sword_bg->set_scale(1);
                _sword_bg->set_position(0, 0); 
            }
            
            if (_zoom_affine_mat.has_value()) {
                bn::fixed_point cam_pos = bn::fixed_point(camera.x(), camera.y());
                if (_player->sprite() && _player_affine_mat.has_value()) {
                    bool facing_left = _player->facing_direction() == PlayerMovement::Direction::LEFT;
                    _player_affine_mat->set_horizontal_flip(facing_left);
                    _player->sprite()->set_affine_mat(_player_affine_mat.value());
                    _player->sprite()->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                    bn::fixed_point player_world_pos = _player->pos() + bn::fixed_point(0, PLAYER_SPRITE_Y_OFFSET);
                    bn::fixed_point offset = player_world_pos - cam_pos;
                    bn::fixed_point scaled_pos = cam_pos + bn::fixed_point(offset.x() * _current_zoom_scale, offset.y() * _current_zoom_scale);
                    _player->sprite()->set_position(scaled_pos); 
                }
                
                if (_player->vfx_sprite() && _vfx_affine_mat.has_value()) {
                    bool facing_left = _player->facing_direction() == PlayerMovement::Direction::LEFT;
                    _vfx_affine_mat->set_horizontal_flip(facing_left);
                    _player->vfx_sprite()->set_affine_mat(_vfx_affine_mat.value());
                    _player->vfx_sprite()->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                    bn::fixed_point vfx_world_pos = _player->vfx_sprite()->position();
                    bn::fixed_point offset = vfx_world_pos - cam_pos;
                    bn::fixed_point scaled_pos = cam_pos + bn::fixed_point(offset.x() * _current_zoom_scale, offset.y() * _current_zoom_scale);
                    _player->vfx_sprite()->set_position(scaled_pos); 
                }
                
                if (_player->gun_sprite() && _gun_affine_mat.has_value()) {
                    int dir_idx = int(_player->facing_direction());
                    bn::fixed gun_rotation = player_constants::GUN_ANGLES[dir_idx];
                    _gun_affine_mat->set_rotation_angle(gun_rotation);
                    _player->gun_sprite()->set_affine_mat(_gun_affine_mat.value());
                    _player->gun_sprite()->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                    bn::fixed_point player_world_pos = _player->pos();
                    bn::fixed_point gun_screen_pos = _player->gun_sprite()->position();
                    bn::fixed_point gun_offset_from_player = gun_screen_pos - player_world_pos;
                    bn::fixed_point gun_world_pos = player_world_pos + gun_offset_from_player;
                    bn::fixed_point offset = gun_world_pos - cam_pos;
                    bn::fixed_point scaled_pos = cam_pos + bn::fixed_point(offset.x() * _current_zoom_scale, offset.y() * _current_zoom_scale);
                    _player->gun_sprite()->set_position(scaled_pos); 
                }
                
                if (_player->has_companion() && _player->get_companion()) {
                    PlayerCompanion *companion = _player->get_companion();
                    bn::sprite_ptr companion_sprite = companion->get_sprite();
                    companion_sprite.set_affine_mat(_zoom_affine_mat.value());
                    companion_sprite.set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                    bn::fixed_point comp_world_pos = companion->pos();
                    bn::fixed_point offset = comp_world_pos - cam_pos;
                    bn::fixed_point scaled_pos = cam_pos + bn::fixed_point(offset.x() * _current_zoom_scale, offset.y() * _current_zoom_scale);
                    companion_sprite.set_position(scaled_pos);
                    
                    bn::sprite_ptr *progress_bar = companion->get_progress_bar_sprite();
                    if (progress_bar) {
                        progress_bar->set_affine_mat(_zoom_affine_mat.value());
                        progress_bar->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                        bn::fixed_point pb_offset_from_companion = bn::fixed_point(0, -16);
                        bn::fixed_point scaled_pb_offset = bn::fixed_point(pb_offset_from_companion.x() * _current_zoom_scale, pb_offset_from_companion.y() * _current_zoom_scale);
                        progress_bar->set_position(scaled_pos + scaled_pb_offset);
                    }
                    
                    bn::vector<bn::sprite_ptr, 16> &text_sprites = companion->get_text_sprites();
                    const bn::vector<bn::fixed_point, 16> &original_offsets = companion->get_text_original_offsets();
                    if (!text_sprites.empty() && !original_offsets.empty()) {
                        bn::fixed_point text_center_world = companion->get_text_center();
                        bn::fixed_point text_center_offset = text_center_world - cam_pos;
                        bn::fixed_point scaled_text_center = cam_pos + bn::fixed_point(text_center_offset.x() * _current_zoom_scale, text_center_offset.y() * _current_zoom_scale);
                        for (int i = 0; i < text_sprites.size() && i < original_offsets.size(); ++i) {
                            text_sprites[i].set_affine_mat(_zoom_affine_mat.value());
                            text_sprites[i].set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                            bn::fixed_point scaled_offset = bn::fixed_point(original_offsets[i].x() * _current_zoom_scale, original_offsets[i].y() * _current_zoom_scale);
                            text_sprites[i].set_position(scaled_text_center + scaled_offset);
                        }
                    }
                }
                
                for (Bullet &bullet : _player->bullets_mutable()) {
                    if (bullet.is_active() && bullet.get_sprite()) {
                        bn::fixed_point bullet_world_pos = bullet.position();
                        bn::fixed_point offset = bullet_world_pos - cam_pos;
                        bn::fixed_point scaled_pos = cam_pos + bn::fixed_point(offset.x() * _current_zoom_scale, offset.y() * _current_zoom_scale);
                        bullet.get_sprite()->set_position(scaled_pos); 
                    }
                }
                
                for (Enemy &enemy : _enemies) {
                    if (enemy.has_sprite()) {
                        bn::sprite_ptr *enemy_sprite = enemy.get_sprite();
                        if (enemy_sprite) {
                            enemy_sprite->set_affine_mat(_zoom_affine_mat.value());
                            enemy_sprite->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                            bn::fixed_point enemy_world_pos = enemy.get_position();
                            bn::fixed_point offset = enemy_world_pos - cam_pos;
                            bn::fixed_point scaled_pos = cam_pos + bn::fixed_point(offset.x() * _current_zoom_scale, offset.y() * _current_zoom_scale);
                            enemy_sprite->set_position(scaled_pos); 
                        }
                    }
                    bn::sprite_ptr *health_bar = enemy.get_health_bar_sprite();
                    if (health_bar) {
                        bn::fixed_point enemy_world_pos = enemy.get_position();
                        bn::fixed_point hb_world_pos = enemy_world_pos + bn::fixed_point(0, -12);
                        bn::fixed_point offset = hb_world_pos - cam_pos;
                        bn::fixed_point scaled_pos = cam_pos + bn::fixed_point(offset.x() * _current_zoom_scale, offset.y() * _current_zoom_scale);
                        health_bar->set_position(scaled_pos); 
                    }
                }
                
                if (_merchant && _merchant->has_sprite()) {
                    bn::sprite_ptr *merchant_sprite = _merchant->get_sprite();
                    if (merchant_sprite) {
                        merchant_sprite->set_affine_mat(_zoom_affine_mat.value());
                        merchant_sprite->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                        bn::fixed_point merchant_world_pos = _merchant->pos();
                        bn::fixed_point offset = merchant_world_pos - cam_pos;
                        bn::fixed_point scaled_pos = cam_pos + bn::fixed_point(offset.x() * _current_zoom_scale, offset.y() * _current_zoom_scale);
                        merchant_sprite->set_position(scaled_pos); 
                    }
                }
            } else {
                if (_player->sprite() && _player->sprite()->affine_mat().has_value()) { _player->sprite()->remove_affine_mat(); }
                if (_player->vfx_sprite() && _player->vfx_sprite()->affine_mat().has_value()) { _player->vfx_sprite()->remove_affine_mat(); }
                if (_player->gun_sprite() && _player->gun_sprite()->affine_mat().has_value()) {
                    _player->gun_sprite()->remove_affine_mat();
                    _player->update_gun_position(_player->facing_direction()); 
                }
                
                if (_player->has_companion() && _player->get_companion()) {
                    PlayerCompanion *companion = _player->get_companion();
                    bn::sprite_ptr companion_sprite = companion->get_sprite();
                    if (companion_sprite.affine_mat().has_value()) { companion_sprite.remove_affine_mat(); }
                    bn::sprite_ptr *progress_bar = companion->get_progress_bar_sprite();
                    if (progress_bar && progress_bar->affine_mat().has_value()) { progress_bar->remove_affine_mat(); }
                    for (bn::sprite_ptr &text_sprite : companion->get_text_sprites()) {
                        if (text_sprite.affine_mat().has_value()) { text_sprite.remove_affine_mat(); }
                    }
                    companion->reset_text_positions(); 
                }
                
                for (Enemy &enemy : _enemies) {
                    if (enemy.has_sprite()) {
                        bn::sprite_ptr *enemy_sprite = enemy.get_sprite();
                        if (enemy_sprite && enemy_sprite->affine_mat().has_value()) { enemy_sprite->remove_affine_mat(); }
                    }
                }
                
                if (_merchant && _merchant->has_sprite()) {
                    bn::sprite_ptr *merchant_sprite = _merchant->get_sprite();
                    if (merchant_sprite && merchant_sprite->affine_mat().has_value()) { merchant_sprite->remove_affine_mat(); }
                }
            }
            if (_enemies.empty()) { } 
        }
    }

    void World::_init_world_specific_content(int world_id, bn::camera_ptr &camera, bn::regular_bg_ptr &bg, bn::sprite_text_generator &text_generator) {
        _enemies.clear();
        if (_merchant) {
            delete _merchant;
            _merchant = nullptr; 
        }
        
        switch (world_id) {
        case 0: 
            _merchant = new MerchantNPC(bn::fixed_point(100, -50), camera, text_generator);
            _enemies.push_back(Enemy(0, -100, camera, bg, ENEMY_TYPE::SPEARGUARD, 3));
            _enemies.push_back(Enemy(50, -80, camera, bg, ENEMY_TYPE::SPEARGUARD, 3));
            _enemies.push_back(Enemy(-50, -120, camera, bg, ENEMY_TYPE::SPEARGUARD, 3));
            break;
        case 1: 
            _enemies.push_back(Enemy(-100, -50, camera, bg, ENEMY_TYPE::SPEARGUARD, 2));
            _enemies.push_back(Enemy(80, -100, camera, bg, ENEMY_TYPE::SPEARGUARD, 2));
            break;
        case 2: 
            _merchant = new MerchantNPC(bn::fixed_point(-80, 100), camera, text_generator);
            _enemies.push_back(Enemy(0, 0, camera, bg, ENEMY_TYPE::SPEARGUARD, 4));
            _enemies.push_back(Enemy(100, 20, camera, bg, ENEMY_TYPE::SPEARGUARD, 4));
            _enemies.push_back(Enemy(-100, 40, camera, bg, ENEMY_TYPE::SPEARGUARD, 4));
            _enemies.push_back(Enemy(0, 80, camera, bg, ENEMY_TYPE::SPEARGUARD, 4));
            break;
        default:
            _merchant = new MerchantNPC(bn::fixed_point(100, -50), camera, text_generator);
            _enemies.push_back(Enemy(0, -100, camera, bg, ENEMY_TYPE::SPEARGUARD, 3));
            break; 
        }
    }

    void World::_save_current_state() {
        if (_player) {
            WorldStateManager &state_manager = WorldStateManager::instance();
            state_manager.save_world_state(_current_world_id, _player->pos(), _player->get_hp());
        }
    }

    void World::_update_camera_shake() {
        if (_shake_frames > 0 && _camera) {
            _shake_frames--;
            _shake_intensity *= 0.85;
            static int shake_seed = 1234;
            shake_seed = (shake_seed * 1664525 + 1013904223) % 32768;
            int shake_x_int = (shake_seed % 16) - 8;
            shake_seed = (shake_seed * 1664525 + 1013904223) % 32768;
            int shake_y_int = (shake_seed % 16) - 8;
            bn::fixed shake_x = bn::fixed(shake_x_int) * _shake_intensity / 4;
            bn::fixed shake_y = bn::fixed(shake_y_int) * _shake_intensity / 4;
            bn::fixed current_x = _camera->x();
            bn::fixed current_y = _camera->y();
            _camera->set_position(current_x + shake_x, current_y + shake_y); 
        }
    }

    void World::trigger_screen_shake(int frames, bn::fixed intensity) {
        _shake_frames = frames;
        _shake_intensity = intensity; 
    }

    // =========================================================================
    // Menu Implementation
    // =========================================================================

    Menu::Menu() : _selected_index(0) { _init_worlds(); }
    Menu::~Menu() {}

    void Menu::_init_worlds() {
        _worlds.clear();
        _worlds.push_back({0, "Main World", bn::fixed_point(MAIN_WORLD_SPAWN_X, MAIN_WORLD_SPAWN_Y), true});
        _worlds.push_back({1, "Forest Area", bn::fixed_point(FOREST_WORLD_SPAWN_X, FOREST_WORLD_SPAWN_Y), true});
    }

    void Menu::_update_display() {
        _text_sprites.clear();
        bn::sprite_text_generator text_generator(common::variable_8x8_sprite_font);
        text_generator.set_center_alignment();
        text_generator.set_bg_priority(0);
        text_generator.generate(0, MENU_TITLE_Y_POSITION, "WORLD SELECTION", _text_sprites);
        text_generator.generate(0, MENU_INSTRUCTIONS_Y_POSITION, "UP/DOWN: Select  A: Enter  B: Exit", _text_sprites);
        for (int i = 0; i < _worlds.size(); ++i) {
            int y_pos = MENU_WORLD_LIST_START_Y + (i * MENU_WORLD_LIST_SPACING);
            if (!_worlds[i].is_unlocked) { text_generator.generate(0, y_pos, "??? LOCKED ???", _text_sprites); }
            else {
                bn::string<64> line;
                if (i == _selected_index) {
                    line = "> ";
                    line += _worlds[i].world_name;
                    line += " <"; }
                else {
                    line = "  ";
                    line += _worlds[i].world_name; }
                text_generator.generate(0, y_pos, line, _text_sprites); }
        }
    }

    void Menu::_handle_input() {
        if (bn::keypad::up_pressed()) {
            if (_selected_index > 0) {
                _selected_index--;
                while (_selected_index >= 0 && !_worlds[_selected_index].is_unlocked) { _selected_index--; }
                if (_selected_index < 0) {
                    _selected_index = _worlds.size() - 1;
                    while (_selected_index >= 0 && !_worlds[_selected_index].is_unlocked) { _selected_index--; }
                }
            }
            else {
                _selected_index = _worlds.size() - 1;
                while (_selected_index >= 0 && !_worlds[_selected_index].is_unlocked) { _selected_index--; }
            }
        }
        if (bn::keypad::down_pressed()) {
            if (_selected_index < _worlds.size() - 1) {
                _selected_index++;
                while (_selected_index < _worlds.size() && !_worlds[_selected_index].is_unlocked) { _selected_index++; }
                if (_selected_index >= _worlds.size()) {
                    _selected_index = 0;
                    while (_selected_index < _worlds.size() && !_worlds[_selected_index].is_unlocked) { _selected_index++; }
                }
            }
            else {
                _selected_index = 0;
                while (_selected_index < _worlds.size() && !_worlds[_selected_index].is_unlocked) { _selected_index++; }
            }
        }
    }

    fe::Scene Menu::execute(int &selected_world_id, bn::fixed_point &spawn_location) {
        bn::bg_palettes::set_transparent_color(bn::color(MENU_BG_COLOR_R, MENU_BG_COLOR_G, MENU_BG_COLOR_B));
        while (true) {
            bn::core::update();
            _handle_input();
            _update_display();
            if (bn::keypad::a_pressed()) {
                if (_selected_index >= 0 && _selected_index < _worlds.size() &&
                    _worlds[_selected_index].is_unlocked) {
                    selected_world_id = _worlds[_selected_index].world_id;
                    spawn_location = _worlds[_selected_index].spawn_location;
                    return fe::Scene::WORLD; }
            }
            if (bn::keypad::b_pressed()) { return fe::Scene::START; }
        }
    }

    // =========================================================================
    // Start Implementation
    // =========================================================================

    Start::Start() : _selected_index(0) {}
    Start::~Start() {}

    void Start::_update_display() {
        _text_sprites.clear();
        bn::sprite_text_generator text_generator(common::variable_8x8_sprite_font);
        text_generator.set_center_alignment();
        text_generator.set_bg_priority(0);
        text_generator.generate(0, START_TITLE_Y_POSITION, "STRANDED", _text_sprites);
        const char* options[] = {"Play Game", "Controls"};
        for (int i = 0; i < 2; ++i) {
            int y_pos = START_OPTIONS_START_Y + (i * START_OPTIONS_SPACING);
            bn::string<64> line;
            if (i == _selected_index) {
                line = "> ";
                line += options[i];
                line += " <"; }
            else {
                line = "  ";
                line += options[i]; }
            text_generator.generate(0, y_pos, line, _text_sprites); 
        }
        text_generator.generate(0, START_INSTRUCTIONS_Y_POSITION, "UP/DOWN: Select  A: Confirm", _text_sprites);
    }

    void Start::_handle_input() {
        if (bn::keypad::up_pressed()) { _selected_index = (_selected_index > 0) ? _selected_index - 1 : 1; }
        if (bn::keypad::down_pressed()) { _selected_index = (_selected_index < 1) ? _selected_index + 1 : 0; }
    }

    fe::Scene Start::execute() {
        bn::bg_palettes::set_transparent_color(bn::color(MENU_BG_COLOR_R, MENU_BG_COLOR_G, MENU_BG_COLOR_B));
        while (true) {
            bn::core::update();
            _handle_input();
            _update_display();
            if (bn::keypad::a_pressed()) {
                if (_selected_index == 0) { return fe::Scene::MENU; }
                else if (_selected_index == 1) { return fe::Scene::CONTROLS; }
            }
        }
    }

    // =========================================================================
    // Controls Implementation
    // =========================================================================

    Controls::Controls() {}
    Controls::~Controls() {}

    void Controls::_update_display() {
        _text_sprites.clear();
        bn::sprite_text_generator text_generator(common::variable_8x8_sprite_font);
        text_generator.set_center_alignment();
        text_generator.set_bg_priority(0);
        text_generator.generate(0, CONTROLS_TITLE_Y_POSITION, "CONTROLS", _text_sprites);
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
        for (const char* control : controls) {
            text_generator.generate(0, y_pos, control, _text_sprites);
            y_pos += CONTROLS_LIST_SPACING; 
        }
        text_generator.generate(0, CONTROLS_INSTRUCTIONS_Y_POSITION, "Press B to return", _text_sprites);
    }

    fe::Scene Controls::execute() {
        bn::bg_palettes::set_transparent_color(bn::color(MENU_BG_COLOR_R, MENU_BG_COLOR_G, MENU_BG_COLOR_B));
        _update_display();
        while (true) {
            bn::core::update();
            if (bn::keypad::b_pressed()) { return fe::Scene::START; }
        }
    }

    // =========================================================================
    // WorldStateManager Implementation
    // =========================================================================

    void WorldStateManager::save_world_state(int world_id, const bn::fixed_point& player_pos, int player_health) {
        WorldState* existing_state = _find_state(world_id);
        if(existing_state) {
            existing_state->player_position = player_pos;
            existing_state->player_health = player_health;
            existing_state->is_saved = true; 
        } else {
            WorldState new_state(world_id);
            new_state.player_position = player_pos;
            new_state.player_health = player_health;
            new_state.is_saved = true;
            _saved_states.push_back(new_state); 
        }
    }

    WorldState WorldStateManager::load_world_state(int world_id) {
        WorldState* existing_state = _find_state(world_id);
        if(existing_state && existing_state->is_saved) { return *existing_state; }
        else {
            WorldState default_state(world_id);
            default_state.player_position = get_default_spawn(world_id);
            return default_state; 
        }
    }

    bool WorldStateManager::has_saved_state(int world_id) {
        WorldState* existing_state = _find_state(world_id);
        return existing_state && existing_state->is_saved; 
    }

    bn::fixed_point WorldStateManager::get_default_spawn(int world_id) {
        switch(world_id) {
            case 0: return bn::fixed_point(50, 100);
            case 1: return bn::fixed_point(100, 50);
            case 2: return bn::fixed_point(0, 150);
            case 3: return bn::fixed_point(-50, 75);
            default: return bn::fixed_point(50, 100); 
        }
    }

    WorldState* WorldStateManager::_find_state(int world_id) {
        for(int i = 0; i < _saved_states.size(); ++i) {
            if(_saved_states[i].world_id == world_id) { return &_saved_states[i]; }
        }
        return nullptr; 
    }

} // namespace fe
