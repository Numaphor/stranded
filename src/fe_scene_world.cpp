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

        // Use regular allocation instead of EWRAM to avoid memory issues
        static bn::regular_bg_map_cell cells[cells_count];

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

            // Initialize all cells to background tile (32x32 = 1,024 cells, minimum required by Butano)
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
                    // Additional bounds check to ensure we stay within map
                    if (x >= 0 && x < columns && y >= 0 && y < rows)
                    {
                        int cell_index = x + y * columns;
                        cells[cell_index] = bn::regular_bg_map_cell(tile_index);
                    }
                }
            }
        }

        // Method to show/hide merchant interaction zone tiles for debug visualization only
        void set_merchant_interaction_zone_visible(bool visible, const bn::fixed_point &merchant_center)
        {
            if (!visible)
                return; // Don't need to clear since refresh_all_zones handles clearing

            // Convert world coordinates to tile coordinates
            constexpr int tile_size = TILE_SIZE;
            const int map_offset_x = MAP_OFFSET_X;
            const int map_offset_y = MAP_OFFSET_Y;

            // Calculate the merchant's tile position
            int merchant_tile_x = ((merchant_center.x() + map_offset_x) / tile_size).integer();
            int merchant_tile_y = ((merchant_center.y() + map_offset_y) / tile_size).integer();

            // Calculate interaction zone bounds in tiles (32x32 pixels = 4x4 tiles)
            const int zone_tile_half_width = MERCHANT_INTERACTION_ZONE_WIDTH / (2 * tile_size);
            const int zone_tile_half_height = MERCHANT_INTERACTION_ZONE_HEIGHT / (2 * tile_size);

            int zone_tile_left = merchant_tile_x - zone_tile_half_width;
            int zone_tile_right = merchant_tile_x + zone_tile_half_width;
            int zone_tile_top = merchant_tile_y - zone_tile_half_height;
            int zone_tile_bottom = merchant_tile_y + zone_tile_half_height;

            // Only draw tiles that are within the 32x32 map bounds
            zone_tile_left = bn::max(0, zone_tile_left);
            zone_tile_right = bn::min(columns, zone_tile_right);
            zone_tile_top = bn::max(0, zone_tile_top);
            zone_tile_bottom = bn::min(rows, zone_tile_bottom);

            // Update tiles in the interaction zone area
            for (int x = zone_tile_left; x < zone_tile_right; x++)
            {
                for (int y = zone_tile_top; y < zone_tile_bottom; y++)
                {
                    int cell_index = x + y * columns;
                    cells[cell_index] = bn::regular_bg_map_cell(INTERACTION_ZONE_TILE_INDEX);
                }
            }
        }

        // Method to show/hide merchant hitbox zone tiles for debug visualization only
        void set_merchant_hitbox_zone_visible(bool visible, const bn::fixed_point &merchant_center)
        {
            if (!visible)
                return; // Don't need to clear since refresh_all_zones handles clearing

            // Convert world coordinates to tile coordinates
            constexpr int tile_size = TILE_SIZE;
            const int map_offset_x = MAP_OFFSET_X;
            const int map_offset_y = MAP_OFFSET_Y;

            // Calculate the merchant's tile position
            int merchant_tile_x = ((merchant_center.x() + map_offset_x) / tile_size).integer();
            int merchant_tile_y = ((merchant_center.y() + map_offset_y) / tile_size).integer();

            // Calculate collision zone bounds in tiles (16x16 pixels = 2x2 tiles)
            const int zone_tile_half_width = MERCHANT_COLLISION_ZONE_WIDTH / (2 * tile_size);
            const int zone_tile_half_height = MERCHANT_COLLISION_ZONE_HEIGHT / (2 * tile_size);

            int zone_tile_left = merchant_tile_x - zone_tile_half_width;
            int zone_tile_right = merchant_tile_x + zone_tile_half_width;
            int zone_tile_top = merchant_tile_y - zone_tile_half_height;
            int zone_tile_bottom = merchant_tile_y + zone_tile_half_height;

            // Only draw tiles that are within the 32x32 map bounds
            zone_tile_left = bn::max(0, zone_tile_left);
            zone_tile_right = bn::min(columns, zone_tile_right);
            zone_tile_top = bn::max(0, zone_tile_top);
            zone_tile_bottom = bn::min(rows, zone_tile_bottom);

            // Update tiles in the collision zone area (drawn on top of interaction zone)
            for (int x = zone_tile_left; x < zone_tile_right; x++)
            {
                for (int y = zone_tile_top; y < zone_tile_bottom; y++)
                {
                    int cell_index = x + y * columns;
                    cells[cell_index] = bn::regular_bg_map_cell(COLLISION_ZONE_TILE_INDEX);
                }
            }
        }

        // Method to show/hide hitbox markers (top-left and bottom-right tiles) for debug visualization
        void set_hitbox_markers_visible(bool visible, const bn::fixed_point &top_left_pos, const bn::fixed_point &bottom_right_pos)
        {
            // Use centralized constants for coordinate system calculations
            constexpr int tile_size = TILE_SIZE;
            const int map_offset_x = MAP_OFFSET_X;
            const int map_offset_y = MAP_OFFSET_Y;

            // Convert world coordinates to tile coordinates
            int top_left_tile_x = ((top_left_pos.x() + map_offset_x) / tile_size).integer();
            int top_left_tile_y = ((top_left_pos.y() + map_offset_y) / tile_size).integer();
            int bottom_right_tile_x = ((bottom_right_pos.x() + map_offset_x) / tile_size).integer();
            int bottom_right_tile_y = ((bottom_right_pos.y() + map_offset_y) / tile_size).integer();

            // Clamp to map bounds
            top_left_tile_x = bn::max(0, bn::min(columns - 1, top_left_tile_x));
            top_left_tile_y = bn::max(0, bn::min(rows - 1, top_left_tile_y));
            bottom_right_tile_x = bn::max(0, bn::min(columns - 1, bottom_right_tile_x));
            bottom_right_tile_y = bn::max(0, bn::min(rows - 1, bottom_right_tile_y));

            // Set top-left marker
            int top_left_index = top_left_tile_x + top_left_tile_y * columns;
            int top_left_tile_index = visible ? LEFT_MARKER_TILE_INDEX : _background_tile;
            cells[top_left_index] = bn::regular_bg_map_cell(top_left_tile_index);

            // Set bottom-right marker
            int bottom_right_index = bottom_right_tile_x + bottom_right_tile_y * columns;
            int bottom_right_tile_index = visible ? RIGHT_MARKER_TILE_INDEX : _background_tile;
            cells[bottom_right_index] = bn::regular_bg_map_cell(bottom_right_tile_index);
        }

        // Tracking for current hitbox markers to enable efficient clearing
        struct HitboxMarkerPosition
        {
            int tile_x, tile_y;
            HitboxMarkerPosition(int x, int y) : tile_x(x), tile_y(y) {}
        };
        bn::vector<HitboxMarkerPosition, 64> _current_markers; // Track current markers
        bool _markers_need_reload = false;                     // Track if background needs reloading

        // Clear all current hitbox markers
        void clear_hitbox_markers()
        {
            if (!_current_markers.empty())
            {
                for (const auto &marker : _current_markers)
                {
                    int cell_index = marker.tile_x + marker.tile_y * columns;
                    cells[cell_index] = bn::regular_bg_map_cell(_background_tile);
                }
                _current_markers.clear();
                _markers_need_reload = true;
            }
        }

        // Check if background reload is needed and reset flag
        bool should_reload_background()
        {
            if (_markers_need_reload)
            {
                _markers_need_reload = false;
                return true;
            }
            return false;
        }

        // Set hitbox markers and track positions for efficient clearing
        void set_hitbox_markers_tracked(const bn::fixed_point &top_left_pos, const bn::fixed_point &bottom_right_pos)
        {
            // Use centralized constants for coordinate system calculations
            constexpr int tile_size = TILE_SIZE;
            const int map_offset_x = MAP_OFFSET_X;
            const int map_offset_y = MAP_OFFSET_Y;

            // Convert world coordinates to tile coordinates
            int top_left_tile_x = ((top_left_pos.x() + map_offset_x) / tile_size).integer();
            int top_left_tile_y = ((top_left_pos.y() + map_offset_y) / tile_size).integer();
            int bottom_right_tile_x = ((bottom_right_pos.x() + map_offset_x) / tile_size).integer();
            int bottom_right_tile_y = ((bottom_right_pos.y() + map_offset_y) / tile_size).integer();

            // Clamp to map bounds
            top_left_tile_x = bn::max(0, bn::min(columns - 1, top_left_tile_x));
            top_left_tile_y = bn::max(0, bn::min(rows - 1, top_left_tile_y));
            bottom_right_tile_x = bn::max(0, bn::min(columns - 1, bottom_right_tile_x));
            bottom_right_tile_y = bn::max(0, bn::min(rows - 1, bottom_right_tile_y));

            // Set top-left marker
            int top_left_index = top_left_tile_x + top_left_tile_y * columns;
            cells[top_left_index] = bn::regular_bg_map_cell(LEFT_MARKER_TILE_INDEX);
            _current_markers.push_back(HitboxMarkerPosition(top_left_tile_x, top_left_tile_y));

            // Set bottom-right marker (only if different from top-left)
            if (top_left_tile_x != bottom_right_tile_x || top_left_tile_y != bottom_right_tile_y)
            {
                int bottom_right_index = bottom_right_tile_x + bottom_right_tile_y * columns;
                cells[bottom_right_index] = bn::regular_bg_map_cell(RIGHT_MARKER_TILE_INDEX);
                _current_markers.push_back(HitboxMarkerPosition(bottom_right_tile_x, bottom_right_tile_y));
            }

            _markers_need_reload = true;
        }

        // Method to refresh all zone tiles based on current debug state and active zones
        void refresh_all_zones(bool debug_enabled, bool has_merchant = false, bn::optional<bn::fixed_point> merchant_pos = bn::nullopt)
        {
            // Reset all tiles to background (32x32 = 1,024 cells, minimum required by Butano)
            for (int x = 0; x < columns; x++)
            {
                for (int y = 0; y < rows; y++)
                {
                    int cell_index = x + y * columns;
                    cells[cell_index] = bn::regular_bg_map_cell(_background_tile);
                }
            }

            // Also clear hitbox markers by resetting their vector
            _current_markers.clear();
            _markers_need_reload = true;

            // Then set zones visible if debug is enabled
            if (debug_enabled)
            {
                // Always show bg sword zone
                set_zone_visible(true);

                // Show merchant zones if merchant is present - draw interaction zone first (tile 4), then hitbox zone on top (tile 3)
                if (has_merchant && merchant_pos.has_value())
                {
                    set_merchant_interaction_zone_visible(true, merchant_pos.value()); // Tile 4 - interaction zone (100x100) - drawn first
                    set_merchant_hitbox_zone_visible(true, merchant_pos.value());      // Tile 3 - hitbox zone (24x24) - drawn on top
                }
            }
        }

        // Method to update merchant zone tiles without affecting other zones
        bool update_merchant_zone(bool debug_enabled, bool has_merchant, bn::optional<bn::fixed_point> merchant_pos = bn::nullopt)
        {
            static bn::optional<bn::fixed_point> last_merchant_pos = bn::nullopt;
            static bool last_had_merchant = false;
            bool changes_made = false;

            // If merchant status or position changed, we need to update
            bool position_changed = (!last_merchant_pos.has_value() && merchant_pos.has_value()) ||
                                    (last_merchant_pos.has_value() && !merchant_pos.has_value()) ||
                                    (last_merchant_pos.has_value() && merchant_pos.has_value() &&
                                     last_merchant_pos.value() != merchant_pos.value());

            if (debug_enabled && (position_changed || last_had_merchant != has_merchant))
            {
                // Clear old merchant zones if there was one
                if (last_had_merchant && last_merchant_pos.has_value())
                {
                    set_merchant_interaction_zone_visible(false, last_merchant_pos.value());
                    set_merchant_hitbox_zone_visible(false, last_merchant_pos.value());
                    changes_made = true;
                }

                // Set new merchant zones if there is one - draw interaction zone first (tile 4), then hitbox zone on top (tile 3)
                if (has_merchant && merchant_pos.has_value())
                {
                    set_merchant_interaction_zone_visible(true, merchant_pos.value()); // Tile 4 - interaction zone (100x100) - drawn first
                    set_merchant_hitbox_zone_visible(true, merchant_pos.value());      // Tile 3 - hitbox zone (24x24) - drawn on top
                    changes_made = true;
                }
            }

            // Update tracking variables
            last_merchant_pos = merchant_pos;
            last_had_merchant = has_merchant;

            return changes_made;
        }
    }; // Place macro AFTER the struct definition

    // Initialize static cells array in regular memory
    bn::regular_bg_map_cell bg_map::cells[bg_map::cells_count];

    World::World() : _player(nullptr),
                     _level(nullptr),
                     _minimap(nullptr),
                     _sword_bg(bn::nullopt),
                     _merchant(nullptr),
                     _player_status_display(nullptr),
                     _debug_enabled(false),
                     _camera(bn::nullopt),
                     _player_debug_hitbox(),
                     _sword_zone_debug_hitbox(),
                     _merchant_collision_debug_hitbox(),
                     _merchant_interaction_debug_hitbox(),
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

        // Remove the debug zones background entirely - it was causing illegal instruction
        // We'll use the original tile-based approach but with better zone positioning
        // _debug_zones_bg = ... (removed)

        // Priority will be set in the update loop based on player position

        // Initialize minimap in top-right corner
        _minimap = new Minimap(bn::fixed_point(100, -80), bg_map_ptr, camera);
        _player->set_camera(camera);

        // Create text generator for NPCs
        bn::sprite_text_generator text_generator(common::variable_8x8_sprite_font);

        // Initialize player status display
        _player_status_display = bn::make_unique<PlayerStatusDisplay>(text_generator);
        _player_status_display->set_visible(true);

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

                // Use the original tile-based approach with the fixed merchant zone positioning
                bn::optional<bn::fixed_point> merchant_pos = _merchant ? bn::optional<bn::fixed_point>(_merchant->pos()) : bn::nullopt;
                bg_map_obj.refresh_all_zones(new_debug_state, _merchant != nullptr, merchant_pos);
                bg_map_ptr.reload_cells_ref();
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

                // Update debug zones background if debug mode is enabled
                if (_debug_enabled)
                {
                    bn::optional<bn::fixed_point> merchant_pos = bn::optional<bn::fixed_point>(_merchant->pos());
                    if (bg_map_obj.update_merchant_zone(true, true, merchant_pos))
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
                    if (bg_map_obj.update_merchant_zone(true, false, bn::nullopt))
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
            // NOTE: Hitbox markers now use tile-based visualization (tiles 5 & 6) instead of sprites
            if (_debug_enabled)
            {
                // Clear previous hitbox markers before setting new ones
                bg_map_obj.clear_hitbox_markers();

                // Create/update player debug hitbox - now using tile-based markers
                _player_debug_hitbox = fe::Hitbox::create_player_hitbox(_player->pos());
                bn::fixed_point player_top_left = _player_debug_hitbox.pos();
                bn::fixed_point player_bottom_right = _player_debug_hitbox.bottom_right();
                bg_map_obj.set_hitbox_markers_tracked(player_top_left, player_bottom_right);

                // Create/update sword zone debug hitbox - now using tile-based markers
                _sword_zone_debug_hitbox = fe::Hitbox::create_sword_zone();
                bn::fixed_point sword_top_left = _sword_zone_debug_hitbox.pos();
                bn::fixed_point sword_bottom_right = _sword_zone_debug_hitbox.bottom_right();
                bg_map_obj.set_hitbox_markers_tracked(sword_top_left, sword_bottom_right);

                // Create/update merchant zone debug hitboxes if merchant is present - now using tile-based markers
                if (_merchant)
                {
                    _merchant_collision_debug_hitbox = fe::Hitbox::create_merchant_collision_zone(_merchant->pos());
                    bn::fixed_point merchant_collision_top_left = _merchant_collision_debug_hitbox.pos();
                    bn::fixed_point merchant_collision_bottom_right = _merchant_collision_debug_hitbox.bottom_right();
                    bg_map_obj.set_hitbox_markers_tracked(merchant_collision_top_left, merchant_collision_bottom_right);

                    _merchant_interaction_debug_hitbox = fe::Hitbox::create_merchant_interaction_zone(_merchant->pos());
                    bn::fixed_point merchant_interaction_top_left = _merchant_interaction_debug_hitbox.pos();
                    bn::fixed_point merchant_interaction_bottom_right = _merchant_interaction_debug_hitbox.bottom_right();
                    bg_map_obj.set_hitbox_markers_tracked(merchant_interaction_top_left, merchant_interaction_bottom_right);
                }

                // Maintain a pool of reusable enemy debug hitboxes
                size_t required_hitboxes = _enemies.size();
                if (_enemy_debug_hitboxes.size() < required_hitboxes)
                {
                    // Add new hitboxes to the pool if needed
                    _enemy_debug_hitboxes.resize(required_hitboxes);
                }
                else if (_enemy_debug_hitboxes.size() > required_hitboxes)
                {
                    // No need to clear debug markers for tile-based system - tiles are reset during refresh_all_zones
                    _enemy_debug_hitboxes.resize(required_hitboxes);
                }

                // Add enemy hitbox markers - now using tile-based markers
                for (size_t i = 0; i < _enemies.size() && i < _enemy_debug_hitboxes.size(); ++i)
                {
                    // Create enemy hitbox with standard type
                    _enemy_debug_hitboxes[i] = fe::Hitbox(_enemies[i].pos().x(), _enemies[i].pos().y(),
                                                          _enemies[i].get_hitbox().width(), _enemies[i].get_hitbox().height(),
                                                          fe::HitboxType::STANDARD);
                    bn::fixed_point enemy_top_left = _enemy_debug_hitboxes[i].pos();
                    bn::fixed_point enemy_bottom_right = _enemy_debug_hitboxes[i].bottom_right();
                    bg_map_obj.set_hitbox_markers_tracked(enemy_top_left, enemy_bottom_right);
                }

                // Reload the background only if hitbox markers have changed
                if (bg_map_obj.should_reload_background())
                {
                    bg_map_ptr.reload_cells_ref();
                }
            }
            else
            {
                // Clear all debug markers when debug is disabled - this is handled by refresh_all_zones when debug is disabled
                // refresh_all_zones resets all tiles to background, which clears hitbox markers
            }

            // Update debug hitbox positions if debug mode is enabled
            if (_debug_enabled)
            {
                // For tile-based system, positions are updated on each frame in the debug visualization section above
                // No need for separate position updates since we recalculate markers each frame

                // Update enemy debug hitbox positions efficiently
                for (size_t i = 0; i < _enemies.size() && i < _enemy_debug_hitboxes.size(); ++i)
                {
                    _enemy_debug_hitboxes[i].set_position(_enemies[i].pos());
                    _enemy_debug_hitboxes[i].update_debug_marker_positions();
                }
            }

            // Update player position and check for collisions
            bn::fixed_point new_pos = _player->pos();

            // Check for zone collisions - prevent player from walking through zones
            bool position_valid = fe::ZoneManager::is_position_valid(new_pos);

            // Merchant collision is handled entirely through Level's tile-based collision system

            if (!position_valid)
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

                // Check for collision with player (but not if player is dead or listening)
                if (_player->get_hp() > 0 && !_player->listening())
                {
                    // Use extended attack hitbox for spearguards during attack
                    Hitbox collision_hitbox = enemy.is_attacking() ? enemy.get_attack_hitbox() : enemy.get_hitbox();
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

                // Check for collision with player's melee attacks
                if (_player->is_attacking())
                {
                    Hitbox player_attack_hitbox = _player->get_attack_hitbox();
                    Hitbox enemy_hitbox = enemy.get_hitbox();

                    // Check for collision between player attack hitbox and enemy
                    if (player_attack_hitbox.collides_with(enemy_hitbox))
                    {
                        fe::Collision::log_collision("Player Attack", "Enemy",
                                                     _player->pos(), enemy.get_position());

                        // Determine damage direction based on player position relative to enemy
                        bool damage_from_left = _player->pos().x() < enemy.get_position().x();

                        if (damage_from_left)
                        {
                            enemy.damage_from_left(1);
                        }
                        else
                        {
                            enemy.damage_from_right(1);
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
