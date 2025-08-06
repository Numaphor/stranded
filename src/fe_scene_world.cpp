#include "fe_scene_world.h"
#include "fe_collision.h"
#include "fe_npc.h"
#include "fe_hitbox.h"
#include "fe_level.h"
#include "fe_constants.h"
#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_sprite_builder.h"
#include "bn_sprite_items_hero.h"
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
#include "bn_regular_bg_items_strandedobjects.h"
#include "fe_bg_utils.h"
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

        // Method to show/hide zone tiles for collision and debug visualization
        void set_zone_visible(bool visible)
        {
            int tile_index = visible ? COLLISION_ZONE_TILE_INDEX : _background_tile; // Use centralized tile index for collision zones

            // Use the centralized sword zone tile coordinates
            // This ensures perfect alignment between visual tiles and collision detection
            constexpr int sword_zone_tile_left = SWORD_ZONE_TILE_LEFT;
            constexpr int sword_zone_tile_right = SWORD_ZONE_TILE_RIGHT; // exclusive upper bound
            constexpr int sword_zone_tile_top = SWORD_ZONE_TILE_TOP;
            constexpr int sword_zone_tile_bottom = SWORD_ZONE_TILE_BOTTOM; // exclusive upper bound

            // Set tiles for the sword zone area using the centralized coordinates
            for (int x = sword_zone_tile_left; x < sword_zone_tile_right; x++)
            {
                for (int y = sword_zone_tile_top; y < sword_zone_tile_bottom; y++)
                {
                    int cell_index = x + y * columns;
                    cells[cell_index] = bn::regular_bg_map_cell(tile_index);
                }
            }

            // Add zone markers at corners if visible
            if (visible)
            {
                // Add left marker at top-left corner
                int tl_cell_index = sword_zone_tile_left + sword_zone_tile_top * columns;
                cells[tl_cell_index] = bn::regular_bg_map_cell(LEFT_MARKER_TILE_INDEX);

                // Add right marker at bottom-right corner (exclusive bounds, so subtract 1)
                int br_x = sword_zone_tile_right - 1;
                int br_y = sword_zone_tile_bottom - 1;
                int br_cell_index = br_x + br_y * columns;
                cells[br_cell_index] = bn::regular_bg_map_cell(RIGHT_MARKER_TILE_INDEX);
            }
        }

        // Method to show/hide merchant interaction zone tiles (100x100) for debug visualization only
        void set_merchant_interaction_zone_visible(bool visible, const bn::fixed_point &merchant_center)
        {
            int tile_index = visible ? INTERACTION_ZONE_TILE_INDEX : _background_tile; // Use centralized tile index for interaction zones

            // Use centralized constants for coordinate system calculations
            constexpr int tile_size = TILE_SIZE;
            const int map_offset_x = MAP_OFFSET_X;
            const int map_offset_y = MAP_OFFSET_Y;

            // Debug visualization: use centralized zone dimensions
            const bn::fixed zone_width = MERCHANT_INTERACTION_ZONE_WIDTH;
            const bn::fixed zone_height = MERCHANT_INTERACTION_ZONE_HEIGHT;

            // Calculate zone boundaries in world coordinates
            const bn::fixed zone_left = merchant_center.x() - zone_width / 2;
            const bn::fixed zone_right = merchant_center.x() + zone_width / 2;
            const bn::fixed zone_top = merchant_center.y() - zone_height / 2;
            const bn::fixed zone_bottom = merchant_center.y() + zone_height / 2;

            // Convert world coordinates to tile coordinates - use same formula as sword zone but in reverse
            // Sword zone: world = tile * TILE_SIZE - MAP_OFFSET
            // So: tile = (world + MAP_OFFSET) / TILE_SIZE
            int tile_left = ((zone_left + map_offset_x) / tile_size).integer();
            int tile_right = ((zone_right + map_offset_x) / tile_size).integer();
            int tile_top = ((zone_top + map_offset_y) / tile_size).integer();
            int tile_bottom = ((zone_bottom + map_offset_y) / tile_size).integer();

            // Clamp to map bounds
            tile_left = bn::max(0, tile_left);
            tile_right = bn::min(columns - 1, tile_right);
            tile_top = bn::max(0, tile_top);
            tile_bottom = bn::min(rows - 1, tile_bottom);

            // Update tiles in the interaction zone area
            for (int x = tile_left; x <= tile_right; x++)
            {
                for (int y = tile_top; y <= tile_bottom; y++)
                {
                    int cell_index = x + y * columns;
                    cells[cell_index] = bn::regular_bg_map_cell(tile_index);
                }
            }

            // Add zone markers at corners if visible
            if (visible)
            {
                // Add left marker at top-left corner
                int tl_cell_index = tile_left + tile_top * columns;
                cells[tl_cell_index] = bn::regular_bg_map_cell(LEFT_MARKER_TILE_INDEX);

                // Add right marker at bottom-right corner
                int br_cell_index = tile_right + tile_bottom * columns;
                cells[br_cell_index] = bn::regular_bg_map_cell(RIGHT_MARKER_TILE_INDEX);
            }
        }

        // Centralized zone management - handles all zone visibility and updates
        bool manage_zones(bool debug_enabled, bool has_merchant = false, bn::optional<bn::fixed_point> merchant_pos = bn::nullopt, bool force_refresh = false)
        {
            static bn::optional<bn::fixed_point> last_merchant_pos = bn::nullopt;
            static bool last_had_merchant = false;
            static bool last_debug_state = false;
            bool changes_made = false;

            // Check if we need to update
            bool position_changed = (!last_merchant_pos.has_value() && merchant_pos.has_value()) ||
                                    (last_merchant_pos.has_value() && !merchant_pos.has_value()) ||
                                    (last_merchant_pos.has_value() && merchant_pos.has_value() &&
                                     last_merchant_pos.value() != merchant_pos.value());

            bool state_changed = (last_debug_state != debug_enabled) || (last_had_merchant != has_merchant);

            if (force_refresh || state_changed || (debug_enabled && position_changed))
            {
                if (force_refresh || state_changed)
                {
                    // Full refresh - reset all tiles to background
                    for (int x = 0; x < columns; x++)
                    {
                        for (int y = 0; y < rows; y++)
                        {
                            int cell_index = x + y * columns;
                            cells[cell_index] = bn::regular_bg_map_cell(_background_tile);
                        }
                    }

                    // Set zones visible if debug is enabled
                    if (debug_enabled)
                    {
                        set_zone_visible(true); // Sword zone
                        if (has_merchant && merchant_pos.has_value())
                        {
                            set_merchant_interaction_zone_visible(true, merchant_pos.value());
                        }
                    }
                    changes_made = true;
                }
                else if (debug_enabled && position_changed)
                {
                    // Incremental update - only merchant zones changed
                    if (last_had_merchant && last_merchant_pos.has_value())
                    {
                        set_merchant_interaction_zone_visible(false, last_merchant_pos.value());
                    }
                    if (has_merchant && merchant_pos.has_value())
                    {
                        set_merchant_interaction_zone_visible(true, merchant_pos.value());
                    }
                    changes_made = true;
                }
            }

            // Update tracking variables
            last_merchant_pos = merchant_pos;
            last_had_merchant = has_merchant;
            last_debug_state = debug_enabled;

            return changes_made;
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
                     _debug_enabled(false),
                     _camera(bn::nullopt),
                     _player_debug_hitbox(),
                     _last_camera_direction(PlayerMovement::Direction::DOWN),
                     _direction_change_frames(0),
                     _current_world_id(0)
    {
        // Create player sprite with correct shape and size
        bn::sprite_builder builder(bn::sprite_items::hero);
        builder.set_bg_priority(1);
        _player = new Player(builder.release_build());
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
            // Could restore other state like health here
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

        // Initialize sword background with camera
        _sword_bg = bn::regular_bg_items::sword.create_bg(0, 0);
        _sword_bg->set_visible(true);
        _sword_bg->set_camera(camera); // Make it follow the camera

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

        // Initialize player status display
        _player_status_display = bn::make_unique<PlayerStatusDisplay>(text_generator);
        _player_status_display->set_visible(false);  // Only visible in debug mode

        // Initialize world-specific content
        _init_world_specific_content(world_id, camera, bg, text_generator);

        // Initialize hitbox debug system
        _debug_enabled = false;

        while (true)
        {
            bn::core::update();

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

            // Debug input handling - Toggle hitbox visualization with START + SELECT keys
            if (bn::keypad::select_held() && bn::keypad::start_pressed())
            {
                bool new_debug_state = !_debug_enabled;
                _debug_enabled = new_debug_state;

                // Update player status display visibility based on debug state
                if (_player_status_display)
                {
                    _player_status_display->set_visible(new_debug_state);
                }

                // Refresh all zone visibility for visual debugging (not collision)
                bn::optional<bn::fixed_point> merchant_pos = _merchant ? bn::optional<bn::fixed_point>(_merchant->pos()) : bn::nullopt;
                if (bg_map_obj.manage_zones(new_debug_state, _merchant != nullptr, merchant_pos, true))
                {
                    bg_map_ptr.reload_cells_ref();
                }
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

                // Update merchant zone visual tiles if debug mode is enabled
                if (_debug_enabled)
                {
                    // Update merchant zone tiles efficiently when merchant moves
                    bn::optional<bn::fixed_point> merchant_pos = bn::optional<bn::fixed_point>(_merchant->pos());
                    if (bg_map_obj.manage_zones(true, true, merchant_pos))
                    {
                        bg_map_ptr.reload_cells_ref();
                    }
                }

                // Disable merchant collision during conversations (but keep sprite visible)
                bool conversation_active = _merchant->is_talking() || _player->listening();
                fe::ZoneManager::set_merchant_zone_enabled(!conversation_active);

                // Update merchant's z-order based on Y position
                int merchant_z = -_merchant->pos().y().integer();
                _merchant->set_sprite_z_order(merchant_z);
            }
            else
            {
                // Update merchant zone visual tiles if debug mode is enabled (clear them when no merchant)
                if (_debug_enabled)
                {
                    if (bg_map_obj.manage_zones(true, false, bn::nullopt))
                    {
                        bg_map_ptr.reload_cells_ref();
                    }
                }
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

            // Update player status display
            if (_player_status_display)
            {
                bool near_merchant = _merchant && _merchant->is_in_interaction_zone(_player->pos());
                _player_status_display->update_status(*_player, near_merchant);
            }

            // Update player z-order for proper sprite layering
            // This needs to happen every frame regardless of merchant presence
            // The player's update_z_order() method also handles companion and gun z-orders
            _player->update_z_order();

            // Update hitbox debug visualization for player
            if (_debug_enabled)
            {
                // Create/update player debug hitbox
                _player_debug_hitbox = fe::Hitbox::create_player_hitbox(_player->pos());
                _player_debug_hitbox.create_debug_markers(*_camera, true);

                // Maintain a pool of reusable enemy debug hitboxes
                int required_hitboxes = static_cast<int>(_enemies.size());
                if (_enemy_debug_hitboxes.size() < required_hitboxes)
                {
                    // Add new hitboxes to the pool if needed
                    _enemy_debug_hitboxes.resize(required_hitboxes);
                }
                else if (_enemy_debug_hitboxes.size() > required_hitboxes)
                {
                    // Clear debug markers for unused hitboxes
                    for (int i = required_hitboxes; i < _enemy_debug_hitboxes.size(); ++i)
                    {
                        _enemy_debug_hitboxes[i].clear_debug_markers();
                    }
                    _enemy_debug_hitboxes.resize(required_hitboxes);
                }
            }
            else
            {
                // Clear all debug markers when debug is disabled
                _player_debug_hitbox.clear_debug_markers();
                for (auto &enemy_hitbox : _enemy_debug_hitboxes)
                {
                    enemy_hitbox.clear_debug_markers();
                }
                // Do not clear the vector, keep the pool for reuse
            }

            // Update debug hitbox positions if debug mode is enabled
            if (_debug_enabled)
            {
                // Update player debug hitbox marker positions efficiently
                // Note: Player debug hitbox marker positions are updated here each frame
                _player_debug_hitbox.update_debug_marker_positions();

                // Update enemy debug hitbox positions efficiently
                for (int i = 0; i < _enemies.size() && i < _enemy_debug_hitboxes.size(); ++i)
                {
                    _enemy_debug_hitboxes[i].set_position(_enemies[i].pos());
                    _enemy_debug_hitboxes[i].update_debug_marker_positions();
                }
            }

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

            // Proper deadzone camera system
            bn::fixed_point player_pos = _player->pos();
            bn::fixed_point current_camera_pos = bn::fixed_point(camera.x(), camera.y());

            // Check if player is actively moving
            bool player_is_moving = _player->is_moving();

            // Calculate distance from player to camera
            bn::fixed_point player_to_camera = player_pos - current_camera_pos;

            // Check if player is outside deadzone
            bool outside_deadzone_x = bn::abs(player_to_camera.x()) > CAMERA_DEADZONE_X;
            bool outside_deadzone_y = bn::abs(player_to_camera.y()) > CAMERA_DEADZONE_Y;
            bool outside_deadzone = outside_deadzone_x || outside_deadzone_y;

            // Determine target position
            if (player_is_moving && outside_deadzone)
            {
                // Player is moving and outside deadzone - apply look-ahead in facing direction
                auto player_direction = _player->facing_direction();

                // Check if direction changed
                if (player_direction != _last_camera_direction)
                {
                    _direction_change_frames = CAMERA_DIRECTION_CHANGE_DURATION; // Start slow period
                    _last_camera_direction = player_direction;
                }

                // Calculate look-ahead offset based on movement direction
                bn::fixed_point lookahead_offset(0, 0);
                switch (player_direction)
                {
                case PlayerMovement::Direction::UP:
                    lookahead_offset.set_y(-CAMERA_LOOKAHEAD_Y);
                    break;
                case PlayerMovement::Direction::DOWN:
                    lookahead_offset.set_y(CAMERA_LOOKAHEAD_Y);
                    break;
                case PlayerMovement::Direction::LEFT:
                    lookahead_offset.set_x(-CAMERA_LOOKAHEAD_X);
                    break;
                case PlayerMovement::Direction::RIGHT:
                    lookahead_offset.set_x(CAMERA_LOOKAHEAD_X);
                    break;
                default:
                    // No offset for unknown direction
                    break;
                }

                // Set target to player position + lookahead offset
                _camera_target_pos = player_pos + lookahead_offset;
            }
            else if (!player_is_moving)
            {
                // Player is idle - smoothly interpolate camera target towards player position
                // This prevents snapping when stopping after look-ahead
                _camera_target_pos = _camera_target_pos + (player_pos - _camera_target_pos) * CAMERA_FOLLOW_SPEED;
            }
            // If player is moving but inside deadzone, don't change camera target (stay put)

            // Decrease direction change counter
            if (_direction_change_frames > 0)
            {
                _direction_change_frames--;
            }

            // Choose interpolation speed based on whether we're in direction change period
            bool in_direction_change = (_direction_change_frames > 0);
            bn::fixed interpolation_speed = in_direction_change ? CAMERA_DIRECTION_CHANGE_SPEED : CAMERA_FOLLOW_SPEED;

            // Smoothly interpolate camera towards target position
            bn::fixed_point camera_offset = _camera_target_pos - current_camera_pos;
            bn::fixed_point new_camera_pos = current_camera_pos + (camera_offset * interpolation_speed);

            camera.set_x(new_camera_pos.x());
            camera.set_y(new_camera_pos.y());

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

                // Update hitbox debug visualization for this enemy
                if (_debug_enabled && i < _enemy_debug_hitboxes.size())
                {
                    // Create enemy hitbox with standard type
                    _enemy_debug_hitboxes[i] = fe::Hitbox(enemy.pos().x(), enemy.pos().y(),
                                                          enemy.get_hitbox().width(), enemy.get_hitbox().height(),
                                                          fe::HitboxType::STANDARD);
                    _enemy_debug_hitboxes[i].create_debug_markers(*_camera, true);
                }

                // Check for collision with player (but not if player is dead or listening)
                if (_player->get_hp() > 0 && !_player->listening())
                {
                    // Use standard enemy hitbox for collision detection
                    Hitbox collision_hitbox = enemy.get_hitbox();
                    Hitbox player_hitbox = _player->get_hitbox();

                    if (fe::Collision::check_bb(player_hitbox, collision_hitbox))
                    {
                        fe::Collision::log_collision("Player", "Enemy",
                                                     _player->pos(), enemy.get_position());

                        // Player takes damage when colliding with enemy
                        _player->take_damage(1);

                        // Knockback effect
                        bn::fixed_point knockback_vector = _player->pos() - enemy.get_position();
                        // Simple knockback in the x direction based on relative positions
                        bn::fixed knockback_x = (knockback_vector.x() > 0) ? 10 : -10;
                        bn::fixed_point knockback(knockback_x, 0);
                        _player->set_position(_player->pos() + knockback);
                    }
                }

                // Check for collision with companion (if companion exists and is alive)
                if (_player->has_companion() && !_player->get_companion()->is_dead_independently())
                {
                    constexpr int COMPANION_HITBOX_SIZE = 16;
                    constexpr int COMPANION_HITBOX_HALF_SIZE = COMPANION_HITBOX_SIZE / 2;

                    PlayerCompanion *companion = _player->get_companion();
                    Hitbox enemy_hitbox = enemy.get_hitbox();

                    // Create a simple hitbox around companion position (16x16 like most sprites)
                    bn::fixed_point companion_pos = companion->pos();
                    Hitbox companion_hitbox(companion_pos.x() - COMPANION_HITBOX_HALF_SIZE, companion_pos.y() - COMPANION_HITBOX_HALF_SIZE, COMPANION_HITBOX_SIZE, COMPANION_HITBOX_SIZE);

                    if (fe::Collision::check_bb(companion_hitbox, enemy_hitbox))
                    {
                        fe::Collision::log_collision("Companion", "Enemy",
                                                     companion_pos, enemy.get_position());

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
            _merchant = new TortoiseNPC(bn::fixed_point(-80, 100), camera, text_generator);

            // More challenging enemies
            _enemies.push_back(Enemy(0, 0, camera, bg, ENEMY_TYPE::SPEARGUARD, 4));
            _enemies.push_back(Enemy(100, 20, camera, bg, ENEMY_TYPE::SPEARGUARD, 4));
            _enemies.push_back(Enemy(-100, 40, camera, bg, ENEMY_TYPE::SPEARGUARD, 4));
            _enemies.push_back(Enemy(0, 80, camera, bg, ENEMY_TYPE::SPEARGUARD, 4));
            break;

        case 3: // Ocean Side
            // Ocean-specific setup
            _merchant = new PenguinNPC(bn::fixed_point(-60, 40), camera, text_generator);

            // Fewer but stronger enemies
            _enemies.push_back(Enemy(-80, 0, camera, bg, ENEMY_TYPE::SPEARGUARD, 5));
            _enemies.push_back(Enemy(80, 100, camera, bg, ENEMY_TYPE::SPEARGUARD, 5));
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
}
