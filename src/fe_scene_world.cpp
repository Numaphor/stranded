#include "fe_scene_world.h"
#include "fe_collision.h"
#include "fe_npc.h"
#include "fe_hitbox.h"
#include "fe_level.h"
#include "fe_constants.h"
#include "fe_direction_utils.h"
#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_sprite_double_size_mode.h"
#include "bn_sprite_builder.h"
#include "bn_sprite_items_hero_sword.h"
#include "bn_bg_tiles.h"
#include "bn_unique_ptr.h"
#include "bn_bg_palette_items_palette.h"
#include "bn_regular_bg_tiles_items_tiles.h"
#include "bn_log.h"
#include "bn_string.h"
#include "bn_regular_bg_map_item.h"
#include "bn_regular_bg_map_cell.h"
#include "bn_size.h"
#include "bn_regular_bg_tiles_ptr.h"
#include "bn_bg_palette_ptr.h"
#include "bn_sprite_shape_size.h"
#include "bn_regular_bg_map_cell_info.h"
#include "bn_sprite_text_generator.h"
#include "common_variable_8x8_sprite_font.h"
#include <string>
#include "bn_memory.h"

namespace fe
{
    Level *_level = nullptr;

    // Correct macro placement
    struct bg_map
    {
        static const int columns = MAP_COLUMNS;
        static const int rows = MAP_ROWS;
        static const int cells_count = MAP_CELLS_COUNT;

        // Explicitly place in EWRAM
        BN_DATA_EWRAM static bn::regular_bg_map_cell cells[cells_count];

        bn::regular_bg_map_item map_item;
        int _background_tile; // Store the background tile for this world

        bg_map(int world_id = 0) : map_item(cells[0], bn::size(columns, rows))
        {
            // Choose background tile based on world_id
            _background_tile = 1; // Default tile
            if (world_id == 1)    // Forest Area (second world)
            {
                _background_tile = 2;
            }

            // Fill all squares with the appropriate background tile
            for (int x = 0; x < columns; x++)
            {
                for (int y = 0; y < rows; y++)
                {
                    int cell_index = x + y * columns;
                    cells[cell_index] = bn::regular_bg_map_cell(_background_tile);
                }
            }
        }
    }; // Place macro AFTER the struct definition

    // Initialize static cells array in EWRAM
    BN_DATA_EWRAM bn::regular_bg_map_cell bg_map::cells[bg_map::cells_count];

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
                     _vfx_affine_mat(bn::nullopt),
                     _hitbox_debug(nullptr)
    {
        // Create player sprite with correct shape and size
        bn::sprite_builder builder(bn::sprite_items::hero_sword);
        builder.set_bg_priority(1);
        _player = new Player(builder.release_build());
        // Initialize camera lookahead state
        _lookahead_current = bn::fixed_point(0, 0);
    }

    fe::Scene fe::World::execute(bn::fixed_point spawn_location, int world_id)
    {
        _current_world_id = world_id;

        // Load saved state if available
        WorldStateManager &state_manager = WorldStateManager::instance();
        if (state_manager.has_saved_state(world_id))
        {
            WorldState saved_state = state_manager.load_world_state(world_id);
            spawn_location = saved_state.player_position;
            // Could restore other state like healthbar here
        }

        bn::camera_ptr camera = bn::camera_ptr::create(0, 0);
        _camera = camera; // Store camera reference for debug hitbox marker positioning

        bg_map bg_map_obj(world_id);
        bn::regular_bg_tiles_ptr tiles = bn::regular_bg_tiles_items::tiles.create_tiles();
        bn::bg_palette_ptr palette = bn::bg_palette_items::palette.create_palette();
        bn::regular_bg_map_ptr bg_map_ptr = bg_map_obj.map_item.create_map(tiles, palette);
        bn::regular_bg_ptr bg = bn::regular_bg_ptr::create(bg_map_ptr);
        bg.set_camera(camera);

        _level = new Level(bg_map_ptr);
        _player->spawn(spawn_location, camera);

        // Initialize camera target position to player spawn location
        _camera_target_pos = spawn_location;
        camera.set_position(spawn_location.x(), spawn_location.y());
        // Reset lookahead on world start
        _lookahead_current = bn::fixed_point(0, 0);

        // Initialize sword background with camera (affine for scaling support)
        _sword_bg = bn::affine_bg_items::sword.create_bg(0, 0);
        _sword_bg->set_visible(true);
        _sword_bg->set_wrapping_enabled(false); // Disable wrapping to prevent repeating when zoomed
        _sword_bg->set_camera(camera);          // Make it follow the camera

        // Get the outside window and set it to hide the sword background by default
        bn::window outside_window = bn::window::outside();
        outside_window.set_show_bg(*_sword_bg, false);

        // Create a small window to show only the sword
        bn::rect_window internal_window = bn::rect_window::internal();
        internal_window.set_show_bg(*_sword_bg, true);
        internal_window.set_boundaries(-SWORD_HALF_WIDTH, -SWORD_HALF_HEIGHT, SWORD_HALF_WIDTH, SWORD_HALF_HEIGHT);

        // Priority will be set in the update loop based on player position

        // Initialize minimap in top-right corner
        _minimap = new Minimap(bn::fixed_point(100, -80), bg_map_ptr, camera);
        _player->set_camera(camera);

        // Create text generator for NPCs
        bn::sprite_text_generator text_generator(common::variable_8x8_sprite_font);

        // Initialize world-specific content
        _init_world_specific_content(world_id, camera, bg, text_generator);

        // Initialize debug system
        _hitbox_debug = new HitboxDebug(bg_map_ptr, bg_map::cells, bg_map_obj._background_tile);

        while (true)
        {
            bn::core::update();

            // Toggle debug mode with SELECT + START
            if (bn::keypad::select_held() && bn::keypad::start_pressed())
            {
                if (_hitbox_debug)
                {
                    _hitbox_debug->toggle();
                }
            }

            // Check for menu access - SELECT + A opens world selection menu
            if (bn::keypad::select_held() && bn::keypad::a_pressed())
            {
                // Hide merchant sprite before going to menu to prevent duplication
                if (_merchant)
                {
                    _merchant->set_is_hidden(true);
                }

                // Save current state before going to menu
                _save_current_state();
                return fe::Scene::MENU;
            }

            // Toggle zoom with SELECT pressed alone (not combined with action buttons)
            if (bn::keypad::select_pressed() &&
                !bn::keypad::a_held() && !bn::keypad::b_held() &&
                !bn::keypad::l_held() && !bn::keypad::r_held())
            {
                _zoomed_out = !_zoomed_out;
            }

            // Smoothly interpolate zoom scale
            bn::fixed target_scale = _zoomed_out ? ZOOM_OUT_SCALE : ZOOM_NORMAL_SCALE;
            if (_current_zoom_scale != target_scale)
            {
                bn::fixed diff = target_scale - _current_zoom_scale;
                if (bn::abs(diff) < ZOOM_TRANSITION_SPEED)
                {
                    _current_zoom_scale = target_scale;
                }
                else
                {
                    _current_zoom_scale += diff * ZOOM_TRANSITION_SPEED * 2;
                }
            }

            // Manage zoom affine matrix based on current scale
            if (_current_zoom_scale != ZOOM_NORMAL_SCALE)
            {
                // Create or update affine matrix when zoomed
                if (!_zoom_affine_mat.has_value())
                {
                    _zoom_affine_mat = bn::sprite_affine_mat_ptr::create();
                }
                _zoom_affine_mat->set_scale(_current_zoom_scale);

                // Create or update gun affine matrix (needs separate matrix for rotation)
                if (!_gun_affine_mat.has_value())
                {
                    _gun_affine_mat = bn::sprite_affine_mat_ptr::create();
                }
                _gun_affine_mat->set_scale(_current_zoom_scale);

                // Create or update player affine matrix (needs separate matrix for flip)
                if (!_player_affine_mat.has_value())
                {
                    _player_affine_mat = bn::sprite_affine_mat_ptr::create();
                }
                _player_affine_mat->set_scale(_current_zoom_scale);

                // Create or update VFX affine matrix (needs separate matrix for flip)
                if (!_vfx_affine_mat.has_value())
                {
                    _vfx_affine_mat = bn::sprite_affine_mat_ptr::create();
                }
                _vfx_affine_mat->set_scale(_current_zoom_scale);
            }
            else
            {
                // Clear affine matrices when at normal scale
                _zoom_affine_mat.reset();
                _gun_affine_mat.reset();
                _player_affine_mat.reset();
                _vfx_affine_mat.reset();
            }

            // Handle NPC interactions BEFORE player input processing
            // This ensures listening state is set before weapon equipping is checked
            bool merchant_was_talking = false;
            if (_merchant)
            {
                // Capture talking state BEFORE update to detect conversation ending
                merchant_was_talking = _merchant->is_talking();
                _merchant->update();

                // Update Level system with merchant position for tile-based collision
                fe::ZoneManager::set_merchant_zone_center(_merchant->pos());

                // Disable merchant collision during conversations (but keep sprite visible)
                bool conversation_active = _merchant->is_talking() || _player->listening();
                fe::ZoneManager::set_merchant_zone_enabled(!conversation_active);

                // Update merchant's z-order based on Y position
                int merchant_z = -_merchant->pos().y().integer();
                _merchant->set_sprite_z_order(merchant_z);
            }

            // Handle conversation end detection BEFORE interaction zone checks
            // This ensures conversation end is detected regardless of player position
            if (_merchant && !_merchant->is_talking() && merchant_was_talking)
            {
                // Conversation just ended
                _player->set_listening(false);
            }

            // Check for merchant interaction BEFORE player input
            if (_merchant && fe::Hitbox::is_in_merchant_interaction_zone(_player->pos(), _merchant->pos()))
            {
                // Update merchant's near player flag for UI display (fixes missing interaction prompt)
                _merchant->set_near_player(true);

                if (bn::keypad::a_pressed() && !merchant_was_talking && !_player->listening())
                {
                    // Start conversation
                    _player->set_listening(true);
                    _merchant->talk();
                }
            }
            else
            {
                // Clear merchant's near player flag when not in interaction zone
                if (_merchant)
                {
                    _merchant->set_near_player(false);
                }

                // NOTE: We don't automatically end conversations if player moves away
                // because player movement should be completely blocked during conversations.
                // Conversations should only end through normal dialog progression (A/UP/START keys).
            }

            // Now update player (which includes input handling)
            _player->update();
            _player->update_gun_position(_player->facing_direction());

            // Handle screen shake based on continuous firing - DISABLED
            // Screen shake has been completely disabled
            if (_player->is_firing())
            {
                // Player is currently firing - increase continuous fire counter
                _continuous_fire_frames++;

                // Calculate progressive intensity based on how long player has been firing
                bn::fixed intensity_multiplier = bn::min(
                    bn::fixed(_continuous_fire_frames) / GUNFIRE_BUILDUP_FRAMES,
                    bn::fixed(1.0));
                bn::fixed current_intensity = GUNFIRE_SHAKE_BASE_INTENSITY +
                                              (GUNFIRE_SHAKE_MAX_INTENSITY - GUNFIRE_SHAKE_BASE_INTENSITY) * intensity_multiplier;

                // Trigger shake if player just fired a bullet - DISABLED
                if (_player->bullet_just_fired())
                {
                    // trigger_screen_shake(GUNFIRE_SHAKE_FRAMES, current_intensity); // DISABLED
                    _player->clear_bullet_fired_flag();
                }
            }
            else
            {
                // Player stopped firing - reset continuous fire counter
                _continuous_fire_frames = 0;
            }

            // Update player z-order for proper sprite layering
            // This needs to happen every frame regardless of merchant presence
            // The player's update_z_order() method also handles companion and gun z-orders
            _player->update_z_order();

            // Update player position and check for collisions
            bn::fixed_point new_pos = _player->pos();

            // Check for zone collisions - prevent player from walking through zones
            if (!fe::ZoneManager::is_position_valid(new_pos))
            {
                _player->revert_position();
            }

            // Update minimap with player position and enemies
            if (_minimap)
            {
                _minimap->update(_player->pos(), bn::fixed_point(0, 0), _enemies);
            }

            // Camera system with look-ahead (shake disabled)
            bn::fixed_point player_pos = _player->pos();
            PlayerMovement::Direction facing_dir = _player->facing_direction();

            // Calculate desired lookahead based on player's facing direction
            bn::fixed_point desired_lookahead(0, 0);

            // Apply lookahead in the direction the player is facing
            switch (facing_dir)
            {
            case PlayerMovement::Direction::RIGHT:
                desired_lookahead = bn::fixed_point(CAMERA_LOOKAHEAD_X, 0);
                break;
            case PlayerMovement::Direction::LEFT:
                desired_lookahead = bn::fixed_point(-CAMERA_LOOKAHEAD_X, 0);
                break;
            case PlayerMovement::Direction::UP:
                desired_lookahead = bn::fixed_point(0, -CAMERA_LOOKAHEAD_Y);
                break;
            case PlayerMovement::Direction::DOWN:
                desired_lookahead = bn::fixed_point(0, CAMERA_LOOKAHEAD_Y);
                break;
            default:
                break;
            }

            // Smoothly interpolate current lookahead towards desired lookahead
            _lookahead_current = _lookahead_current + (desired_lookahead - _lookahead_current) * CAMERA_LOOKAHEAD_SMOOTHING;

            // Calculate target camera position with lookahead
            bn::fixed_point camera_target = player_pos + _lookahead_current;

            // Get current camera position
            bn::fixed_point current_camera_pos = _camera.has_value() ? bn::fixed_point(_camera->x(), _camera->y()) : bn::fixed_point(0, 0);
            bn::fixed_point camera_to_target = camera_target - current_camera_pos;

            bn::fixed new_camera_x = current_camera_pos.x();
            bn::fixed new_camera_y = current_camera_pos.y();

            // Apply deadzone - only move camera if target is outside deadzone
            if (bn::abs(camera_to_target.x()) > CAMERA_DEADZONE_X)
            {
                new_camera_x = camera_target.x() - (camera_to_target.x() > 0 ? CAMERA_DEADZONE_X : -CAMERA_DEADZONE_X);
            }
            if (bn::abs(camera_to_target.y()) > CAMERA_DEADZONE_Y)
            {
                new_camera_y = camera_target.y() - (camera_to_target.y() > 0 ? CAMERA_DEADZONE_Y : -CAMERA_DEADZONE_Y);
            }

            // Clamp camera to map boundaries (prevent showing outside world)
            // GBA screen is 240x160, so half is 120x80
            constexpr bn::fixed half_screen_x = 120;
            constexpr bn::fixed half_screen_y = 80;
            constexpr bn::fixed map_min_x = -MAP_OFFSET_X + half_screen_x;
            constexpr bn::fixed map_max_x = MAP_OFFSET_X - half_screen_x;
            constexpr bn::fixed map_min_y = -MAP_OFFSET_Y + half_screen_y;
            constexpr bn::fixed map_max_y = MAP_OFFSET_Y - half_screen_y;

            bn::fixed_point new_camera_pos(
                bn::clamp(new_camera_x, map_min_x, map_max_x),
                bn::clamp(new_camera_y, map_min_y, map_max_y));

            // Apply final camera position (shake is disabled, set to 0)
            // Round to integer pixels to prevent sub-pixel jittering
            if (_camera.has_value())
            {
                _camera->set_x(bn::fixed(new_camera_pos.x().integer()));
                _camera->set_y(bn::fixed(new_camera_pos.y().integer()));
            }

            // Update sword position and priority based on player position
            if (_sword_bg)
            {
                constexpr bn::fixed sword_sprite_x = 0;
                constexpr bn::fixed sword_sprite_y = 0;

                // Get the internal window and update its position
                bn::rect_window &window_ref = internal_window;

                // Calculate the sword's position in screen coordinates
                bn::fixed_point camera_pos(camera.x(), camera.y());
                bn::fixed_point sword_screen_pos = bn::fixed_point(sword_sprite_x, sword_sprite_y) - camera_pos;

                // Update window boundaries to be centered on the sword
                window_ref.set_boundaries(
                    sword_screen_pos.y() - SWORD_HALF_HEIGHT, // top
                    sword_screen_pos.x() - SWORD_HALF_WIDTH,  // left
                    sword_screen_pos.y() + SWORD_HALF_HEIGHT, // bottom
                    sword_screen_pos.x() + SWORD_HALF_WIDTH   // right
                );

                // Set priority based on player's Y position relative to the sword
                // When player is above the sword (lower Y), they should be behind it (higher priority)
                // When player is below the sword (higher Y), they should be in front of it (lower priority)
                // Threshold lowered by 16 pixels so player goes behind sword earlier
                const int sword_priority = (player_pos.y() > sword_sprite_y + 8) ? 2 : 0; // 0 = above player, 2 = below player
                _sword_bg->set_priority(sword_priority);
            }

            // Update all enemies and check for collisions with player and bullets
            for (int i = 0; i < _enemies.size();)
            {
                Enemy &enemy = _enemies[i];
                // Enemies should ignore player if listening to NPCs OR dead
                bool player_should_be_ignored = _player->listening() || _player->get_hp() <= 0;
                enemy.update(_player->pos(), *_level, player_should_be_ignored);

                // Check for collision with player (but not if player is dead or listening)
                if (_player->get_hp() > 0 && !_player->listening())
                {
                    // Use standard enemy hitbox for collision detection
                    Hitbox collision_hitbox = enemy.get_hitbox();
                    Hitbox player_hitbox = _player->get_hitbox();

                    if (fe::Collision::check_bb(player_hitbox, collision_hitbox))
                    {
                        // Player can roll through enemies without taking damage, but cannot walk through them
                        if (!_player->is_state(PlayerMovement::State::ROLLING))
                        {
                            // Player takes damage when colliding with enemy (only when not rolling)
                            _player->take_damage(1);

                            // Knockback effect
                            bn::fixed_point knockback_vector = _player->pos() - enemy.get_position();
                            // Simple knockback in the x direction based on relative positions
                            bn::fixed knockback_x = (knockback_vector.x() > 0) ? 10 : -10;
                            bn::fixed_point knockback(knockback_x, 0);
                            _player->set_position(_player->pos() + knockback);
                        }
                    }
                }

                // Check for collision with companion (if companion exists and is alive)
                if (_player->has_companion() && !_player->get_companion()->is_dead_independently())
                {
                    constexpr int COMPANION_HITBOX_HALF_SIZE = COMPANION_HITBOX_SIZE / 2;

                    PlayerCompanion *companion = _player->get_companion();
                    Hitbox enemy_hitbox = enemy.get_hitbox();

                    // Create a simple hitbox around companion position (16x16 like most sprites)
                    bn::fixed_point companion_pos = companion->pos();
                    Hitbox companion_hitbox(companion_pos.x() - COMPANION_HITBOX_HALF_SIZE, companion_pos.y() - COMPANION_HITBOX_HALF_SIZE, COMPANION_HITBOX_SIZE, COMPANION_HITBOX_SIZE);

                    if (fe::Collision::check_bb(companion_hitbox, enemy_hitbox))
                    {
                        // Companion dies when hit by enemy
                        _player->kill_companion();
                    }
                }

                // Check for collision with player's bullets - optimized collision detection
                if (_player->bullets().size() > 0)
                {
                    // Get reference to bullets vector to avoid copying
                    const auto &bullets = _player->bullets();

                    Hitbox enemy_hitbox = enemy.get_hitbox();

                    // Check each bullet for collision with this enemy
                    for (const auto &bullet : bullets)
                    {
                        if (bullet.is_active())
                        {
                            Hitbox bullet_hitbox = bullet.get_hitbox();

                            // Check for collision
                            if (bullet_hitbox.collides_with(enemy_hitbox))
                            {
                                // Bullet hit enemy - enemy takes 1 damage
                                bool damage_from_left = bullet.position().x() < enemy.get_position().x();

                                if (damage_from_left)
                                {
                                    enemy.damage_from_left(1);
                                }
                                else
                                {
                                    enemy.damage_from_right(1);
                                }

                                // Deactivate the bullet
                                const_cast<Bullet &>(bullet).deactivate();

                                // No need to check other bullets for this enemy
                                break;
                            }
                        }
                    }
                }

                // Remove dead enemies only after death animation completes
                if (enemy.is_ready_for_removal())
                {
                    _enemies.erase(_enemies.begin() + i);
                }
                else
                {
                    i++;
                }
            }

            // Check game over conditions and reset if needed
            if (_player->is_reset_required())
            {
                // Reset the game state
                _player->reset();
                _level->reset(); // Assuming Level has a reset method
                _enemies.clear();

                // Reset minimap
                delete _minimap;
                _minimap = new Minimap(bn::fixed_point(100, -80), bg_map_ptr, camera);

                // Reset player position
                _player->spawn(spawn_location, camera);

                // Reset enemies
                _enemies.push_back(Enemy(0, -100, camera, bg, ENEMY_TYPE::SPEARGUARD, 3));
                _enemies.push_back(Enemy(50, -80, camera, bg, ENEMY_TYPE::SPEARGUARD, 3));
                _enemies.push_back(Enemy(-50, -120, camera, bg, ENEMY_TYPE::SPEARGUARD, 3));

                // Reset camera
                camera.set_position(0, 0);

                continue; // Skip the rest of the loop for this frame
            }

            // Apply zoom scaling to affine backgrounds
            if (_sword_bg && _current_zoom_scale != ZOOM_NORMAL_SCALE)
            {
                _sword_bg->set_scale(_current_zoom_scale);
                // Scale sword position relative to camera
                bn::fixed_point cam_pos = bn::fixed_point(camera.x(), camera.y());
                bn::fixed_point sword_world_pos = bn::fixed_point(0, 0); // Sword is at world origin
                bn::fixed_point offset = sword_world_pos - cam_pos;
                bn::fixed_point scaled_pos = cam_pos + bn::fixed_point(offset.x() * _current_zoom_scale, offset.y() * _current_zoom_scale);
                _sword_bg->set_position(scaled_pos.x(), scaled_pos.y());
            }
            else if (_sword_bg)
            {
                _sword_bg->set_scale(1);
                _sword_bg->set_position(0, 0); // Reset to original position
            }

            // Apply zoom affine matrix and position scaling to all game sprites
            // Use camera as zoom center, scale positions relative to camera
            if (_zoom_affine_mat.has_value())
            {
                // Camera position is the zoom center (screen center)
                bn::fixed_point cam_pos = bn::fixed_point(camera.x(), camera.y());

                // Apply to player sprite - needs its own affine mat for horizontal flip
                if (_player->sprite() && _player_affine_mat.has_value())
                {
                    // Update player affine mat with current flip state
                    bool facing_left = _player->facing_direction() == PlayerMovement::Direction::LEFT;
                    _player_affine_mat->set_horizontal_flip(facing_left);

                    _player->sprite()->set_affine_mat(_player_affine_mat.value());
                    _player->sprite()->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                    bn::fixed_point player_world_pos = _player->pos();
                    bn::fixed_point offset = player_world_pos - cam_pos;
                    bn::fixed_point scaled_pos = cam_pos + bn::fixed_point(offset.x() * _current_zoom_scale, offset.y() * _current_zoom_scale);
                    _player->sprite()->set_position(scaled_pos);
                }

                // Apply to VFX sprite - needs its own affine mat for horizontal flip
                if (_player->vfx_sprite() && _vfx_affine_mat.has_value())
                {
                    // Update VFX affine mat with current flip state
                    bool facing_left = _player->facing_direction() == PlayerMovement::Direction::LEFT;
                    _vfx_affine_mat->set_horizontal_flip(facing_left);

                    _player->vfx_sprite()->set_affine_mat(_vfx_affine_mat.value());
                    _player->vfx_sprite()->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                    bn::fixed_point vfx_world_pos = _player->vfx_sprite()->position();
                    bn::fixed_point offset = vfx_world_pos - cam_pos;
                    bn::fixed_point scaled_pos = cam_pos + bn::fixed_point(offset.x() * _current_zoom_scale, offset.y() * _current_zoom_scale);
                    _player->vfx_sprite()->set_position(scaled_pos);
                }

                // Apply to gun sprite - use player world pos + gun offset
                // Gun needs its own affine matrix to combine zoom scale with gun rotation
                if (_player->gun_sprite() && _gun_affine_mat.has_value())
                {
                    // Get gun rotation from player's facing direction (not sprite, since affine mat overrides rotation)
                    // Note: Don't set horizontal_flip on affine mat - sprite's own flip is applied separately
                    int dir_idx = int(_player->facing_direction());
                    bn::fixed gun_rotation = player_constants::GUN_ANGLES[dir_idx];
                    _gun_affine_mat->set_rotation_angle(gun_rotation);

                    _player->gun_sprite()->set_affine_mat(_gun_affine_mat.value());
                    _player->gun_sprite()->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                    // Gun world position is player pos + relative gun offset
                    bn::fixed_point player_world_pos = _player->pos();
                    bn::fixed_point gun_screen_pos = _player->gun_sprite()->position();
                    // Gun offset from player in world space
                    bn::fixed_point gun_offset_from_player = gun_screen_pos - player_world_pos;
                    bn::fixed_point gun_world_pos = player_world_pos + gun_offset_from_player;
                    bn::fixed_point offset = gun_world_pos - cam_pos;
                    bn::fixed_point scaled_pos = cam_pos + bn::fixed_point(offset.x() * _current_zoom_scale, offset.y() * _current_zoom_scale);
                    _player->gun_sprite()->set_position(scaled_pos);
                }

                // Apply to companion sprite
                if (_player->has_companion() && _player->get_companion())
                {
                    PlayerCompanion *companion = _player->get_companion();
                    bn::sprite_ptr companion_sprite = companion->get_sprite();
                    companion_sprite.set_affine_mat(_zoom_affine_mat.value());
                    companion_sprite.set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                    bn::fixed_point comp_world_pos = companion->pos();
                    bn::fixed_point offset = comp_world_pos - cam_pos;
                    bn::fixed_point scaled_pos = cam_pos + bn::fixed_point(offset.x() * _current_zoom_scale, offset.y() * _current_zoom_scale);
                    companion_sprite.set_position(scaled_pos);

                    // Apply to companion revival progress bar - position relative to scaled companion pos
                    bn::sprite_ptr *progress_bar = companion->get_progress_bar_sprite();
                    if (progress_bar)
                    {
                        progress_bar->set_affine_mat(_zoom_affine_mat.value());
                        progress_bar->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                        // Progress bar is above companion - use scaled companion position + scaled offset
                        bn::fixed_point pb_offset_from_companion = bn::fixed_point(0, -16);
                        bn::fixed_point scaled_pb_offset = bn::fixed_point(pb_offset_from_companion.x() * _current_zoom_scale, pb_offset_from_companion.y() * _current_zoom_scale);
                        progress_bar->set_position(scaled_pos + scaled_pb_offset);
                    }

                    // Text sprites - scale visually and compact spacing using stored original offsets
                    bn::vector<bn::sprite_ptr, 16> &text_sprites = companion->get_text_sprites();
                    const bn::vector<bn::fixed_point, 16> &original_offsets = companion->get_text_original_offsets();
                    if (!text_sprites.empty() && !original_offsets.empty())
                    {
                        // Text center is at companion death position + text offset
                        bn::fixed_point text_center_world = companion->get_text_center();
                        bn::fixed_point text_center_offset = text_center_world - cam_pos;
                        bn::fixed_point scaled_text_center = cam_pos + bn::fixed_point(text_center_offset.x() * _current_zoom_scale, text_center_offset.y() * _current_zoom_scale);

                        for (int i = 0; i < text_sprites.size() && i < original_offsets.size(); ++i)
                        {
                            text_sprites[i].set_affine_mat(_zoom_affine_mat.value());
                            text_sprites[i].set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                            // Use stored original offset and scale it inward
                            bn::fixed_point scaled_offset = bn::fixed_point(original_offsets[i].x() * _current_zoom_scale, original_offsets[i].y() * _current_zoom_scale);
                            text_sprites[i].set_position(scaled_text_center + scaled_offset);
                        }
                    }
                }

                // Apply position scaling to bullets (no visual scaling)
                for (Bullet &bullet : _player->bullets_mutable())
                {
                    if (bullet.is_active() && bullet.get_sprite())
                    {
                        bn::fixed_point bullet_world_pos = bullet.position();
                        bn::fixed_point offset = bullet_world_pos - cam_pos;
                        bn::fixed_point scaled_pos = cam_pos + bn::fixed_point(offset.x() * _current_zoom_scale, offset.y() * _current_zoom_scale);
                        bullet.get_sprite()->set_position(scaled_pos);
                    }
                }

                // Apply to enemies
                for (Enemy &enemy : _enemies)
                {
                    if (enemy.has_sprite())
                    {
                        bn::sprite_ptr *enemy_sprite = enemy.get_sprite();
                        if (enemy_sprite)
                        {
                            enemy_sprite->set_affine_mat(_zoom_affine_mat.value());
                            enemy_sprite->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                            bn::fixed_point enemy_world_pos = enemy.get_position();
                            bn::fixed_point offset = enemy_world_pos - cam_pos;
                            bn::fixed_point scaled_pos = cam_pos + bn::fixed_point(offset.x() * _current_zoom_scale, offset.y() * _current_zoom_scale);
                            enemy_sprite->set_position(scaled_pos);
                        }
                    }
                    // Health bar - position it above the scaled enemy position
                    bn::sprite_ptr *health_bar = enemy.get_health_bar_sprite();
                    if (health_bar)
                    {
                        bn::fixed_point enemy_world_pos = enemy.get_position();
                        bn::fixed_point hb_world_pos = enemy_world_pos + bn::fixed_point(0, -12); // Health bar offset above enemy
                        bn::fixed_point offset = hb_world_pos - cam_pos;
                        bn::fixed_point scaled_pos = cam_pos + bn::fixed_point(offset.x() * _current_zoom_scale, offset.y() * _current_zoom_scale);
                        health_bar->set_position(scaled_pos);
                    }
                }

                // Apply to merchant
                if (_merchant && _merchant->has_sprite())
                {
                    bn::sprite_ptr *merchant_sprite = _merchant->get_sprite();
                    if (merchant_sprite)
                    {
                        merchant_sprite->set_affine_mat(_zoom_affine_mat.value());
                        merchant_sprite->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                        bn::fixed_point merchant_world_pos = _merchant->pos();
                        bn::fixed_point offset = merchant_world_pos - cam_pos;
                        bn::fixed_point scaled_pos = cam_pos + bn::fixed_point(offset.x() * _current_zoom_scale, offset.y() * _current_zoom_scale);
                        merchant_sprite->set_position(scaled_pos);
                    }
                }
            }
            else
            {
                // Remove affine matrix when not zooming
                if (_player->sprite() && _player->sprite()->affine_mat().has_value())
                {
                    _player->sprite()->remove_affine_mat();
                }
                if (_player->vfx_sprite() && _player->vfx_sprite()->affine_mat().has_value())
                {
                    _player->vfx_sprite()->remove_affine_mat();
                }
                if (_player->gun_sprite() && _player->gun_sprite()->affine_mat().has_value())
                {
                    _player->gun_sprite()->remove_affine_mat();
                    // Re-apply gun rotation/flip/position after removing affine matrix
                    // (setup_gun was called before this, but rotation is ignored when affine mat is attached)
                    _player->update_gun_position(_player->facing_direction());
                }
                // Remove from companion and its revival sprites
                if (_player->has_companion() && _player->get_companion())
                {
                    PlayerCompanion *companion = _player->get_companion();
                    bn::sprite_ptr companion_sprite = companion->get_sprite();
                    if (companion_sprite.affine_mat().has_value())
                    {
                        companion_sprite.remove_affine_mat();
                    }
                    bn::sprite_ptr *progress_bar = companion->get_progress_bar_sprite();
                    if (progress_bar && progress_bar->affine_mat().has_value())
                    {
                        progress_bar->remove_affine_mat();
                    }
                    // Remove affine matrices from text sprites and reset positions
                    for (bn::sprite_ptr &text_sprite : companion->get_text_sprites())
                    {
                        if (text_sprite.affine_mat().has_value())
                        {
                            text_sprite.remove_affine_mat();
                        }
                    }
                    companion->reset_text_positions();
                }
                for (Enemy &enemy : _enemies)
                {
                    if (enemy.has_sprite())
                    {
                        bn::sprite_ptr *enemy_sprite = enemy.get_sprite();
                        if (enemy_sprite && enemy_sprite->affine_mat().has_value())
                        {
                            enemy_sprite->remove_affine_mat();
                        }
                    }
                }
                if (_merchant && _merchant->has_sprite())
                {
                    bn::sprite_ptr *merchant_sprite = _merchant->get_sprite();
                    if (merchant_sprite && merchant_sprite->affine_mat().has_value())
                    {
                        merchant_sprite->remove_affine_mat();
                    }
                }
            }

            // Update debug visualization
            if (_hitbox_debug && _hitbox_debug->is_active())
            {
                _hitbox_debug->update(_player, _enemies, _merchant, camera);
            }

            // Check win condition
            if (_enemies.empty())
            {
            }
        }
    }

    World::~World()
    {
        delete _player;
        delete _level;
        delete _minimap;
        delete _merchant;
        delete _hitbox_debug;
        // _player_status_display is unique_ptr - automatically cleaned up
    }

    void World::_init_world_specific_content(int world_id, bn::camera_ptr &camera, bn::regular_bg_ptr &bg, bn::sprite_text_generator &text_generator)
    {
        // Clear existing enemies
        _enemies.clear();

        // Clean up existing merchant to prevent duplicates
        if (_merchant)
        {
            delete _merchant;
            _merchant = nullptr;
        }

        // Initialize different content based on world ID
        switch (world_id)
        {
        case 0: // Main World
            // Create merchant NPC
            _merchant = new MerchantNPC(bn::fixed_point(100, -50), camera, text_generator);

            // Spawn 3 spearguard enemies
            _enemies.push_back(Enemy(0, -100, camera, bg, ENEMY_TYPE::SPEARGUARD, 3));
            _enemies.push_back(Enemy(50, -80, camera, bg, ENEMY_TYPE::SPEARGUARD, 3));
            _enemies.push_back(Enemy(-50, -120, camera, bg, ENEMY_TYPE::SPEARGUARD, 3));
            break;

        case 1: // Forest Area
            // Spawn different enemy configuration
            _enemies.push_back(Enemy(-100, -50, camera, bg, ENEMY_TYPE::SPEARGUARD, 2));
            _enemies.push_back(Enemy(80, -100, camera, bg, ENEMY_TYPE::SPEARGUARD, 2));
            break;

        case 2: // Desert Zone
            // Desert-specific setup
            _merchant = new MerchantNPC(bn::fixed_point(-80, 100), camera, text_generator);

            // More challenging enemies
            _enemies.push_back(Enemy(0, 0, camera, bg, ENEMY_TYPE::SPEARGUARD, 4));
            _enemies.push_back(Enemy(100, 20, camera, bg, ENEMY_TYPE::SPEARGUARD, 4));
            _enemies.push_back(Enemy(-100, 40, camera, bg, ENEMY_TYPE::SPEARGUARD, 4));
            _enemies.push_back(Enemy(0, 80, camera, bg, ENEMY_TYPE::SPEARGUARD, 4));
            break;

        default: // Default to main world
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
            // Decrease shake frames and intensity
            _shake_frames--;
            _shake_intensity *= 0.85; // Slower decay to maintain intensity longer

            // Generate random shake offset using simpler method
            static int shake_seed = 1234;
            shake_seed = (shake_seed * 1664525 + 1013904223) % 32768;
            int shake_x_int = (shake_seed % 16) - 8; // Range: -8 to +7

            shake_seed = (shake_seed * 1664525 + 1013904223) % 32768;
            int shake_y_int = (shake_seed % 16) - 8; // Range: -8 to +7

            // Convert to fixed point and apply intensity
            bn::fixed shake_x = bn::fixed(shake_x_int) * _shake_intensity / 4;
            bn::fixed shake_y = bn::fixed(shake_y_int) * _shake_intensity / 4;

            // Apply shake offset to camera
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
}
