#include "str_scene_world.h"
#include "str_player.h"
#include "str_level.h"
#include "str_minimap.h"
#include "str_enemy.h"
#include "str_npc.h"
#include "str_collision.h"
#include "str_world_state.h"
#include "str_constants.h"
#include "str_direction_utils.h"
#include "str_chunk_manager.h"
#include "str_world_object.h"
#include "str_world_map_data.h"
#include "../validation/background/bg_validation.h"
#include "../validation/integration/system_validation.h"

#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_sprite_ptr.h"
#include "bn_optional.h"
#include "bn_span.h"
#include "bn_affine_bg_ptr.h"
#include "bn_affine_bg_map_ptr.h"
#include "bn_affine_bg_map_cell.h"
#include "bn_affine_bg_map_cell_info.h"
#include "bn_camera_ptr.h"
#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_size.h"
#include "bn_window.h"
#include "bn_rect_window.h"
#include "bn_bg_palettes.h"
#include "bn_bg_tiles.h"
#include "bn_bg_palette_ptr.h"
#include "bn_affine_bg_map_item.h"
#include "bn_affine_bg_tiles_ptr.h"
#include "bn_sprite_builder.h"
#include "bn_sprite_double_size_mode.h"
#include "bn_sprite_text_generator.h"

#include "bn_affine_bg_tiles_items_tiles_affine.h"
#include "bn_bg_palette_items_palette.h"
#include "bn_sprite_items_hero.h"
#include "common_variable_8x8_sprite_font.h"

#include <cstdint>

namespace str
{

    // =========================================================================
    // Helpers
    // =========================================================================

    namespace
    {
        // View buffer for chunk streaming (128x128 tiles, same as before)
        struct view_buffer
        {
            static const int columns = VIEW_BUFFER_TILES;  // 128
            static const int rows = VIEW_BUFFER_TILES;     // 128
            static const int cells_count = columns * rows; // 16384
            BN_DATA_EWRAM static bn::affine_bg_map_cell cells[cells_count];
            bn::affine_bg_map_item map_item;

            view_buffer() : map_item(cells[0], bn::size(columns, rows))
            {
                // Initialize with empty tiles - ChunkManager will fill it
                for (int i = 0; i < cells_count; ++i)
                {
                    cells[i] = bn::affine_bg_map_cell(1);
                }
            }
        };
        BN_DATA_EWRAM bn::affine_bg_map_cell view_buffer::cells[view_buffer::cells_count];

        struct procedural_world_context
        {
            int background_tile = 1;
            int variation_tile = 2;
            int feature_tile = 3;
            int world_seed = 1;
        };

        procedural_world_context g_world_context;

        [[nodiscard]] uint32_t hash_coordinates(int x, int y, int seed)
        {
            uint32_t value = static_cast<uint32_t>(x) * 73856093u;
            value ^= static_cast<uint32_t>(y) * 19349663u;
            value ^= static_cast<uint32_t>(seed) * 83492791u;
            value ^= value >> 13;
            value *= 1274126177u;
            value ^= value >> 16;
            return value;
        }

        bn::affine_bg_map_cell procedural_tile_provider(int tile_x, int tile_y, const void* context_ptr)
        {
            const auto* context = static_cast<const procedural_world_context*>(context_ptr);
            uint32_t hash = hash_coordinates(tile_x, tile_y, context->world_seed);
            uint32_t bucket = hash % 100u;

            int tile = context->background_tile;
            if (bucket < 5)
            {
                tile = context->feature_tile;
            }
            else if (bucket < 35)
            {
                tile = context->variation_tile;
            }

            return bn::affine_bg_map_cell(tile);
        }

        // Configure procedural world parameters for the requested world
        void generate_world_map(int world_id)
        {
            g_world_context.world_seed = 0xACE1 + world_id * 1315423911;
            if (world_id == 1)
            {
                g_world_context.background_tile = 2;
                g_world_context.variation_tile = 1;
                g_world_context.feature_tile = 3;
            }
            else
            {
                g_world_context.background_tile = 1;
                g_world_context.variation_tile = 2;
                g_world_context.feature_tile = 3;
            }
        }

        WorldMapData create_world_map_data()
        {
            WorldMapData data;
            data.provider = procedural_tile_provider;
            data.provider_context = &g_world_context;
            data.width_tiles = WORLD_WIDTH_TILES;
            data.height_tiles = WORLD_HEIGHT_TILES;
            return data;
        }
    }

    // =========================================================================
    // World Implementation
    // =========================================================================

    World::World() : _player(nullptr),
                     _level(nullptr),
                     _minimap(nullptr),
                     _merchant(nullptr),
                     _player_status_display(nullptr),
                     _camera(bn::nullopt),
                     _last_camera_direction(PlayerMovement::Direction::DOWN),
                     _direction_change_frames(0),
                     _current_world_id(0),
                     _use_chunked_world(false),
                     _chunk_manager(nullptr),
                     _player_world_position(0, 0),
                     _shake_frames(0),
                     _shake_intensity(0),
                     _continuous_fire_frames(0),
                     _zoomed_out(false),
                     _current_zoom_scale(ZOOM_NORMAL_SCALE),
                     _zoom_affine_mat(bn::nullopt),
                     _gun_affine_mat(bn::nullopt),
                     _player_affine_mat(bn::nullopt),
                     _vfx_affine_mat(bn::nullopt)
    {
        bn::sprite_builder builder(bn::sprite_items::hero);
        builder.set_bg_priority(1);
        _player = new Player(builder.release_build());
        _lookahead_current = bn::fixed_point(0, 0);
        _skip_camera_update = false;
        _lookahead_paused = false;
    }

    World::~World()
    {
        delete _chunk_manager;
        delete _player;
        delete _level;
        delete _minimap;
        delete _merchant;
    }

    str::Scene str::World::execute(bn::fixed_point spawn_location, int world_id)
    {
        _current_world_id = world_id;
        _use_chunked_world = true;  // Enable chunk system

        WorldStateManager &state_manager = WorldStateManager::instance();
        if (state_manager.has_saved_state(world_id))
        {
            WorldState saved_state = state_manager.load_world_state(world_id);
            spawn_location = saved_state.player_position;
        }

        // Generate the large world map
        generate_world_map(world_id);
        WorldMapData world_map_data = create_world_map_data();

        // Create view buffer and background
        view_buffer vb;
        bn::affine_bg_tiles_ptr tiles = bn::affine_bg_tiles_items::tiles_affine.create_tiles();
        bn::bg_palette_ptr palette = bn::bg_palette_items::palette.create_palette();
        bn::affine_bg_map_ptr bg_map_ptr = vb.map_item.create_map(tiles, palette);
        bn::affine_bg_ptr bg = bn::affine_bg_ptr::create(bg_map_ptr);

        // Initialize chunk manager
        if (_chunk_manager)
        {
            delete _chunk_manager;
        }
        _chunk_manager = new ChunkManager();
        _chunk_manager->init(world_map_data, view_buffer::cells);
        
        // Initialize background validation
        str::BgValidation::init();
        
        // Initialize system integration validation
        str::SystemValidation::init();

        // Start with simple approach: player and camera in buffer coordinates
        // World position = player_screen_pos + camera_pos + buffer_offset
        // Initial: player at (0,0), camera at (0,0), offset = 512
        _player_world_position = bn::fixed_point(BUFFER_HALF_SIZE, BUFFER_HALF_SIZE);

        bn::camera_ptr camera = bn::camera_ptr::create(0, 0);
        _camera = camera;
        bg.set_camera(camera);

        // Create level for collision (uses chunk manager)
        _level = new Level();
        _level->set_chunk_manager(_chunk_manager);

        // Player spawns at origin
        _player->spawn(bn::fixed_point(0, 0), camera);
        _camera_target_pos = bn::fixed_point(0, 0);
        _lookahead_current = bn::fixed_point(0, 0);

        // Force initial chunk loading - run multiple updates to fill visible area
        // With LOAD_RANGE=2 (5x5=25 chunks) and 25 chunks/frame max, we need 1 frame
        // But we use 20 iterations to ensure all chunks are loaded and committed
        // 25 chunks / 25 chunks per frame = 1 frame minimum; 20 provides safety margin
        constexpr int INITIAL_CHUNK_LOAD_ITERATIONS = 20;
        for (int i = 0; i < INITIAL_CHUNK_LOAD_ITERATIONS; ++i)
        {
            _chunk_manager->update(_player_world_position);
        }
        _chunk_manager->commit_to_vram(bg_map_ptr);
        
        // Start background validation session after initial setup
        str::BgValidation::start_validation_session();

        // Sword bg temporarily disabled for affine main bg
        // _sword_bg = bn::affine_bg_items::sword.create_bg(0, 0);
        // _sword_bg->set_visible(true);
        // _sword_bg->set_wrapping_enabled(false);
        // _sword_bg->set_camera(camera);

        // bn::window outside_window = bn::window::outside();
        // outside_window.set_show_bg(*_sword_bg, false);
        // bn::rect_window internal_window = bn::rect_window::internal();
        // internal_window.set_show_bg(*_sword_bg, true);
        // internal_window.set_boundaries(-SWORD_HALF_WIDTH, -SWORD_HALF_HEIGHT, SWORD_HALF_WIDTH, SWORD_HALF_HEIGHT);

        _minimap = new Minimap(bn::fixed_point(100, -80), bg_map_ptr, camera);
        _player->set_camera(camera);

        bn::sprite_text_generator text_generator(common::variable_8x8_sprite_font);
        _init_world_specific_content(world_id, camera, bg, text_generator);

        while (true)
        {
            // Update chunk streaming
            if (_chunk_manager)
            {
                // Store previous background position for artifact detection
                bn::fixed_point previous_bg_pos(bg.x(), bg.y());
                
                _chunk_manager->update(_player_world_position);
                _chunk_manager->commit_to_vram(bg_map_ptr);
                
                // Get current camera position for validation
                bn::fixed_point camera_pos = _camera ? bn::fixed_point(_camera->x(), _camera->y()) : bn::fixed_point(0, 0);
                bn::fixed_point expected_bg_pos = bn::fixed_point(-camera_pos.x(), -camera_pos.y());
                
                // Validate BG register synchronization
                str::BgValidation::test_bg_register_sync(bg, camera_pos, expected_bg_pos);
                
                // Validate affine background compatibility
                bn::fixed current_scale = _current_zoom_scale; // Use our tracked scale
                bn::fixed current_rotation = bg.rotation_angle();
                str::BgValidation::validate_affine_compatibility(bg, bg_map_ptr, camera_pos, 
                                                               current_scale, current_rotation);
                
                // Check rendering pipeline compatibility
                bool dma_in_progress = _chunk_manager->is_streaming();
                bool vblank_active = false; // We're in main loop, not VBlank
                str::BgValidation::check_rendering_pipeline(bg, 1, dma_in_progress, vblank_active);
                
                // Detect visual artifacts
                bool buffer_recentered = _chunk_manager->was_buffer_recentered_this_frame();
                bn::fixed_point current_bg_pos(bg.x(), bg.y());
                str::BgValidation::detect_visual_artifacts(current_bg_pos, previous_bg_pos, buffer_recentered);
                
                // Measure performance impact
                // Note: In a real implementation, you'd measure actual frame time here
                // For now, we'll use a reasonable estimate
                int estimated_frame_time_us = dma_in_progress ? 12000 : 8000; // μs
                int chunks_processed = _chunk_manager->get_chunks_processed_this_frame();
                int tiles_transferred = _chunk_manager->get_tiles_transferred_this_frame();
                str::BgValidation::measure_performance_impact(estimated_frame_time_us, chunks_processed, tiles_transferred);
                
                // Run system integration validation periodically
                static int integration_validation_counter = 0;
                integration_validation_counter++;
                if (integration_validation_counter >= 300) // Every 5 seconds at 60 FPS
                {
                    str::SystemValidation::run_category_tests(str::IntegrationTestCategory::PERFORMANCE);
                    integration_validation_counter = 0;
                }
            }

            bn::core::update();
            if (bn::keypad::select_held() && bn::keypad::a_pressed())
            {
                // End validation session before exiting
                str::BgValidation::end_validation_session();
                str::SystemValidation::shutdown();
                
                if (_merchant)
                    _merchant->set_is_hidden(true);
                _save_current_state();
                return str::Scene::MENU;
            }
            if (bn::keypad::select_pressed() && !bn::keypad::a_held() && !bn::keypad::b_held() && !bn::keypad::l_held() && !bn::keypad::r_held())
            {
                _recenter_camera();
                _zoomed_out = !_zoomed_out;
            }
            
            // Background validation stress test trigger
            if (bn::keypad::start_pressed() && bn::keypad::b_held())
            {
                // Run combined stress test for 5 seconds
                str::BgValidation::run_stress_test(bg, 3, STRESS_TEST_DURATION_FRAMES);
            }
            
            // System integration validation trigger
            if (bn::keypad::start_pressed() && bn::keypad::a_held())
            {
                // Run comprehensive integration validation
                str::SystemValidation::run_all_integration_tests();
            }
            
            // Collision system validation trigger
            if (bn::keypad::start_pressed() && bn::keypad::select_held())
            {
                // Run collision compatibility tests
                str::SystemValidation::run_category_tests(str::IntegrationTestCategory::COLLISION);
            }
            
            // Entity positioning validation trigger
            if (bn::keypad::start_pressed() && bn::keypad::r_held())
            {
                // Run entity positioning tests
                str::SystemValidation::run_category_tests(str::IntegrationTestCategory::ENTITIES);
            }
            bn::fixed t_sc = _zoomed_out ? ZOOM_OUT_SCALE : ZOOM_NORMAL_SCALE;
            if (_current_zoom_scale != t_sc)
            {
                bn::fixed d = t_sc - _current_zoom_scale;
                if (bn::abs(d) < ZOOM_TRANSITION_SPEED)
                    _current_zoom_scale = t_sc;
                else
                    _current_zoom_scale += d * ZOOM_TRANSITION_SPEED * 2;
            }
            if (_current_zoom_scale != ZOOM_NORMAL_SCALE)
            {
                if (!_zoom_affine_mat)
                    _zoom_affine_mat = bn::sprite_affine_mat_ptr::create();
                _zoom_affine_mat->set_scale(_current_zoom_scale);
                if (!_gun_affine_mat)
                    _gun_affine_mat = bn::sprite_affine_mat_ptr::create();
                _gun_affine_mat->set_scale(_current_zoom_scale);
                if (!_player_affine_mat)
                    _player_affine_mat = bn::sprite_affine_mat_ptr::create();
                _player_affine_mat->set_scale(_current_zoom_scale);
                if (!_vfx_affine_mat)
                    _vfx_affine_mat = bn::sprite_affine_mat_ptr::create();
                _vfx_affine_mat->set_scale(_current_zoom_scale);
            }
            else
            {
                _zoom_affine_mat.reset();
                _gun_affine_mat.reset();
                _player_affine_mat.reset();
                _vfx_affine_mat.reset();
            }
            
            bool alert_active = false;
            if (_merchant)
            {
                bool mwt = _merchant->is_talking();
                _merchant->update();
                str::ZoneManager::set_merchant_zone_center(_merchant->pos());
                str::ZoneManager::set_merchant_zone_enabled(!(_merchant->is_talking() || _player->listening()));
                _merchant->set_sprite_z_order(-_merchant->pos().y().integer());
                if (!_merchant->is_talking() && mwt)
                    _player->set_listening(0);
                if (str::Hitbox::is_in_merchant_interaction_zone(_player->pos(), _merchant->pos()))
                {
                    _merchant->set_near_player(1);
                    if (!_merchant->is_talking() && !_player->listening())
                        alert_active = true;

                    if (bn::keypad::a_pressed() && !mwt && !_player->listening())
                    {
                        _player->set_listening(1);
                        _merchant->talk();
                    }
                }
                else
                    _merchant->set_near_player(0);
            }
            _player->update();
            _player->update_gun_position(_player->facing_direction());
            if (_player->is_firing())
            {
                _continuous_fire_frames++;
                if (_player->bullet_just_fired())
                    _player->clear_bullet_fired_flag();
            }
            else
                _continuous_fire_frames = 0;
            _player->update_z_order();

            // Track player world position for chunk streaming
            // Player pos() returns position relative to camera (screen position)
            // So we need: player_screen_pos + camera_pos + buffer_offset
            bn::fixed cam_x = _camera ? _camera->x() : bn::fixed(0);
            bn::fixed cam_y = _camera ? _camera->y() : bn::fixed(0);
            _player_world_position.set_x(_player->pos().x() + cam_x + BUFFER_HALF_SIZE);
            _player_world_position.set_y(_player->pos().y() + cam_y + BUFFER_HALF_SIZE);

            if (!str::ZoneManager::is_position_valid(_player->pos()))
                _player->revert_position();
            if (_minimap)
                _minimap->update(_player->pos(), {0, 0}, _enemies);
            // Camera follows player (original logic)
            if (_skip_camera_update)
            {
                _skip_camera_update = false;
            }
            else
            {
                bool player_is_moving = _player && _player->is_moving();
                if (_lookahead_paused)
                {
                    if (player_is_moving)
                    {
                        _lookahead_paused = false;
                    }
                    else
                    {
                        _lookahead_current = bn::fixed_point(0, 0);
                    }
                }

                bn::fixed_point dl = {0, 0};
                if (!_lookahead_paused)
                {
                    PlayerMovement::Direction fdir = _player->facing_direction();
                    if (fdir == PlayerMovement::Direction::RIGHT)
                        dl = {CAMERA_LOOKAHEAD_X, 0};
                    else if (fdir == PlayerMovement::Direction::LEFT)
                        dl = {-CAMERA_LOOKAHEAD_X, 0};
                    else if (fdir == PlayerMovement::Direction::UP)
                        dl = {0, -CAMERA_LOOKAHEAD_Y};
                    else if (fdir == PlayerMovement::Direction::DOWN)
                        dl = {0, CAMERA_LOOKAHEAD_Y};
                }

                _lookahead_current += (dl - _lookahead_current) * CAMERA_LOOKAHEAD_SMOOTHING;
                bn::fixed_point cp = _camera ? bn::fixed_point(_camera->x(), _camera->y()) : bn::fixed_point(0, 0);
                bn::fixed_point ct = _player->pos() + _lookahead_current;
                bn::fixed_point ctt = ct - cp;
                bn::fixed nx = cp.x(), ny = cp.y();
                if (bn::abs(ctt.x()) > CAMERA_DEADZONE_X)
                    nx = ct.x() - (ctt.x() > 0 ? CAMERA_DEADZONE_X : -CAMERA_DEADZONE_X);
                if (bn::abs(ctt.y()) > CAMERA_DEADZONE_Y)
                    ny = ct.y() - (ctt.y() > 0 ? CAMERA_DEADZONE_Y : -CAMERA_DEADZONE_Y);
                if (_camera)
                    _camera->set_position(
                        bn::clamp(nx, bn::fixed(-WORLD_WIDTH_PIXELS + 120), bn::fixed(WORLD_WIDTH_PIXELS - 120)).integer(),
                        bn::clamp(ny, bn::fixed(-WORLD_HEIGHT_PIXELS + 80), bn::fixed(WORLD_HEIGHT_PIXELS - 80)).integer()
                    );
            }
            // Sword bg temporarily disabled
            // if (_sword_bg)
            // {
            //     bn::fixed_point sp = {0, 0}, cp2(camera.x(), camera.y()), scp = sp - cp2;
            //     internal_window.set_boundaries(scp.y() - SWORD_HALF_HEIGHT, scp.x() - SWORD_HALF_WIDTH, scp.y() + SWORD_HALF_HEIGHT, scp.x() + SWORD_HALF_WIDTH);
            //     _sword_bg->set_priority(_player->pos().y() > sp.y() + 8 ? 2 : 0);
            // }
            
            // Apply zoom to affine background
            // When zoomed, we need to handle camera and pivot carefully.
            // The BG position (with camera) and pivot interact during scaling.
            if (_current_zoom_scale != ZOOM_NORMAL_SCALE)
            {
                // Remove camera from BG and manually position it
                bg.remove_camera();
                bg.set_pivot_position(0, 0);
                // Position BG so camera position appears at screen center, scaled
                if (_camera)
                {
                    bn::fixed scaled_camera_x = _camera->x() * _current_zoom_scale;
                    bn::fixed scaled_camera_y = _camera->y() * _current_zoom_scale;
                    bg.set_position(-scaled_camera_x.integer(), -scaled_camera_y.integer());
                }
                else
                {
                    bg.set_position(0, 0);
                }
                bg.set_scale(_current_zoom_scale);
            }
            else
            {
                // Normal view - use camera
                bg.set_camera(camera);
                bg.set_pivot_position(0, 0);
                bg.set_position(0, 0);
                bg.set_scale(1);
            }
            for (int i = 0; i < _enemies.size();)
            {
                Enemy &e = _enemies[i];
                if (e.is_chasing())
                    alert_active = true;
                bool ignore = _player->listening() || _player->get_hp() <= 0;
                e.update(_player->pos(), *_level, ignore);
                if (!ignore && str::Collision::check_bb(_player->get_hitbox(), e.get_hitbox()) && !_player->is_state(PlayerMovement::State::ROLLING))
                {
                    _player->take_damage(1);
                    bn::fixed kx = (_player->pos().x() - e.get_position().x() > 0) ? 10 : -10;
                    _player->set_position(_player->pos() + bn::fixed_point(kx, 0));
                }
                if (_player->has_companion() && !_player->get_companion()->is_dead_independently() && str::Collision::check_bb({_player->get_companion()->pos().x() - COMPANION_HITBOX_SIZE / 2, _player->get_companion()->pos().y() - COMPANION_HITBOX_SIZE / 2, COMPANION_HITBOX_SIZE, COMPANION_HITBOX_SIZE}, e.get_hitbox()))
                    _player->kill_companion();
                for (const auto &b : _player->bullets())
                    if (b.is_active() && b.get_hitbox().collides_with(e.get_hitbox()))
                    {
                        if (b.position().x() < e.get_position().x())
                            e.damage_from_left(1);
                        else
                            e.damage_from_right(1);
                        const_cast<Bullet &>(b).deactivate();
                        break;
                    }
                if (_player->is_attacking() && _player->get_melee_hitbox().collides_with(e.get_hitbox()))
                {
                    if (_player->get_melee_hitbox().x() < e.get_position().x())
                        e.damage_from_left(1);
                    else
                        e.damage_from_right(1);
                }
                if (e.is_ready_for_removal())
                    _enemies.erase(_enemies.begin() + i);
                else
                    i++;
            }
            
            _player->get_hud().set_alert(alert_active);

            if (_player->is_reset_required())
            {
                _player->reset();
                _level->reset();
                _enemies.clear();
                delete _minimap;
                _minimap = new Minimap({100, -80}, bg_map_ptr, camera);
                _player->spawn(spawn_location, camera);
                _init_world_specific_content(_current_world_id, camera, bg, text_generator);
                camera.set_position(0, 0);
                continue;
            }
            // Sword bg zoom temporarily disabled
            // if (_sword_bg)
            // {
            //     if (_current_zoom_scale != ZOOM_NORMAL_SCALE)
            //     {
            //         _sword_bg->set_scale(_current_zoom_scale);
            //         bn::fixed_point cp3(camera.x(), camera.y()), off = bn::fixed_point(0, 0) - cp3;
            //         _sword_bg->set_position(cp3 + off * _current_zoom_scale);
            //     }
            //     else
            //     {
            //         _sword_bg->set_scale(1);
            //         _sword_bg->set_position(0, 0);
            //     }
            // }
            if (_zoom_affine_mat)
            {
                bn::fixed_point cp4(camera.x(), camera.y());
                auto sc = [&](auto *s, bn::fixed_point wp, bool f = 0)
                { if (!s) return; if (f) _player_affine_mat->set_horizontal_flip(_player->facing_direction() == PlayerMovement::Direction::LEFT); s->set_affine_mat(f ? *_player_affine_mat : *_zoom_affine_mat); s->set_double_size_mode(bn::sprite_double_size_mode::ENABLED); s->set_position(cp4 + (wp - cp4) * _current_zoom_scale); };
                sc(_player->sprite(), _player->pos() + bn::fixed_point(0, PLAYER_SPRITE_Y_OFFSET), 1);
                sc(_player->vfx_sprite(), _player->vfx_sprite() ? _player->vfx_sprite()->position() : bn::fixed_point(0, 0));
                if (_player->gun_sprite() && _gun_affine_mat)
                {
                    _gun_affine_mat->set_rotation_angle(player_constants::GUN_ANGLES[int(_player->facing_direction())]);
                    _player->gun_sprite()->set_affine_mat(*_gun_affine_mat);
                    _player->gun_sprite()->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                    _player->gun_sprite()->set_position(cp4 + (_player->pos() + (_player->gun_sprite()->position() - _player->pos()) - cp4) * _current_zoom_scale);
                }
                if (_player->has_companion() && _player->get_companion())
                {
                    auto *c = _player->get_companion();
                    auto s = c->get_sprite();
                    s.set_affine_mat(*_zoom_affine_mat);
                    s.set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                    s.set_position(cp4 + (c->pos() - cp4) * _current_zoom_scale);
                    if (auto *pb = c->get_progress_bar_sprite())
                    {
                        pb->set_affine_mat(*_zoom_affine_mat);
                        pb->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                        pb->set_position(s.position() + bn::fixed_point(0, -16) * _current_zoom_scale);
                    }
                }
                for (Bullet &b : _player->bullets_mutable())
                    if (b.is_active() && b.get_sprite())
                        sc(b.get_sprite(), b.position());
                for (Enemy &e : _enemies)
                {
                    sc(e.get_sprite(), e.get_position());
                    sc(e.get_health_bar_sprite(), e.get_position() + bn::fixed_point(0, -12));
                }
                if (_merchant)
                    sc(_merchant->get_sprite(), _merchant->pos());
            }
            else
            {
                if (_player->sprite())
                    _player->sprite()->remove_affine_mat();
                if (_player->vfx_sprite())
                    _player->vfx_sprite()->remove_affine_mat();
                if (_player->gun_sprite())
                {
                    _player->gun_sprite()->remove_affine_mat();
                    _player->update_gun_position(_player->facing_direction());
                }
                if (_player->has_companion() && _player->get_companion())
                {
                    auto *c = _player->get_companion();
                    c->get_sprite().remove_affine_mat();
                    if (auto *pb = c->get_progress_bar_sprite())
                        pb->remove_affine_mat();
                    for (auto &ts : c->get_text_sprites())
                        ts.remove_affine_mat();
                    c->reset_text_positions();
                }
                for (Enemy &e : _enemies)
                    if (auto *s = e.get_sprite())
                        s->remove_affine_mat();
                if (_merchant && _merchant->get_sprite())
                    _merchant->get_sprite()->remove_affine_mat();
            }
        }
    }

    void World::_recenter_camera()
    {
        if (!_camera)
        {
            return;
        }

        _lookahead_current = bn::fixed_point(0, 0);
        _skip_camera_update = true;
        _lookahead_paused = true;

        bn::fixed_point player_pos = _player ? _player->pos() : bn::fixed_point(0, 0);
        _camera_target_pos = player_pos;

        bn::fixed clamped_x = bn::clamp(player_pos.x(), bn::fixed(-MAP_OFFSET_X + 120), bn::fixed(MAP_OFFSET_X - 120));
        bn::fixed clamped_y = bn::clamp(player_pos.y(), bn::fixed(-MAP_OFFSET_Y + 80), bn::fixed(MAP_OFFSET_Y - 80));
        _camera->set_position(clamped_x, clamped_y);
    }

    void World::_init_world_specific_content(int world_id, bn::camera_ptr &camera, bn::affine_bg_ptr &bg, bn::sprite_text_generator &text_generator)
    {
        _enemies.clear();
        if (_merchant)
        {
            delete _merchant;
            _merchant = nullptr;
        }

        switch (world_id)
        {
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

    void World::_save_current_state()
    {
        if (_player)
        {
            WorldStateManager &state_manager = WorldStateManager::instance();
            state_manager.save_world_state(_current_world_id, _player->pos(), _player->get_hp());
        }
    }

    void World::_update_camera_shake()
    {
        if (_shake_frames > 0 && _camera)
        {
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

    void World::trigger_screen_shake(int frames, bn::fixed intensity)
    {
        _shake_frames = frames;
        _shake_intensity = intensity;
    }

} // namespace str
