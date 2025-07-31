#include "fe_scene_world.h"
#include "fe_collision.h"
#include "fe_npc.h"
#include "fe_hitbox_debug.h"
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

    // Centralized sword dimensions
    constexpr int SWORD_WIDTH = 256; // Adjust as needed for your sword sprite size
    constexpr int SWORD_HEIGHT = 256;
    constexpr int SWORD_HALF_WIDTH = SWORD_WIDTH / 2;
    constexpr int SWORD_HALF_HEIGHT = SWORD_HEIGHT / 2;

    // Correct macro placement
    struct bg_map
    {
        static const int columns = 320;
        static const int rows = 320;
        static const int cells_count = columns * rows;

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
            int tile_index = visible ? 3 : _background_tile; // Use tile 3 for sword zone when visible (same as other hitbox zones), background tile when hidden

            // Use the EXACT same sword zone tile coordinates as defined in Level::is_in_sword_zone
            // This ensures perfect alignment between visual tiles and collision detection
            constexpr int sword_zone_tile_left = 147;
            constexpr int sword_zone_tile_right = 157; // exclusive upper bound
            constexpr int sword_zone_tile_top = 162;
            constexpr int sword_zone_tile_bottom = 166; // exclusive upper bound

            // Set tiles for the sword zone area using the exact same coordinates as collision
            for (int x = sword_zone_tile_left; x < sword_zone_tile_right; x++)
            {
                for (int y = sword_zone_tile_top; y < sword_zone_tile_bottom; y++)
                {
                    int cell_index = x + y * columns;
                    cells[cell_index] = bn::regular_bg_map_cell(tile_index);
                }
            }
        }

        // Method to show/hide merchant interaction zone tiles (100x100) for debug visualization only
        void set_merchant_interaction_zone_visible(bool visible, const bn::fixed_point &merchant_center)
        {
            int tile_index = visible ? 4 : _background_tile; // Use tile 4 for interaction zone

            // Use the same coordinate system as the hitbox debug system
            constexpr int tile_size = 8;
            const int map_offset_x = (columns * 4); // Same as hitbox debug: map_width * 4
            const int map_offset_y = (rows * 4);    // Same as hitbox debug: map_height * 4

            // Debug visualization: 100x100 pixels to match actual interaction zone
            const bn::fixed zone_width = 100;
            const bn::fixed zone_height = 100;

            // Calculate zone boundaries in world coordinates
            const bn::fixed zone_left = merchant_center.x() - zone_width / 2;
            const bn::fixed zone_right = merchant_center.x() + zone_width / 2;
            const bn::fixed zone_top = merchant_center.y() - zone_height / 2;
            const bn::fixed zone_bottom = merchant_center.y() + zone_height / 2;

            // Convert world coordinates to tile coordinates using hitbox debug formula
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
        }

        // Method to show/hide merchant hitbox zone tiles (24x24) for debug visualization only
        void set_merchant_hitbox_zone_visible(bool visible, const bn::fixed_point &merchant_center)
        {
            int tile_index = visible ? 3 : _background_tile; // Use tile 3 for hitbox zone

            // Use the same coordinate system as the hitbox debug system
            constexpr int tile_size = 8;
            const int map_offset_x = (columns * 4); // Same as hitbox debug: map_width * 4
            const int map_offset_y = (rows * 4);    // Same as hitbox debug: map_height * 4

            // Debug visualization: 24x24 pixels for tile-based collision zone (shown smaller for clarity)
            const bn::fixed zone_width = 24;
            const bn::fixed zone_height = 24;

            // Calculate zone boundaries in world coordinates
            const bn::fixed zone_left = merchant_center.x() - zone_width / 2;
            const bn::fixed zone_right = merchant_center.x() + zone_width / 2;
            const bn::fixed zone_top = merchant_center.y() - zone_height / 2;
            const bn::fixed zone_bottom = merchant_center.y() + zone_height / 2;

            // Convert world coordinates to tile coordinates using hitbox debug formula
            int tile_left = ((zone_left + map_offset_x) / tile_size).integer();
            int tile_right = ((zone_right + map_offset_x) / tile_size).integer();
            int tile_top = ((zone_top + map_offset_y) / tile_size).integer();
            int tile_bottom = ((zone_bottom + map_offset_y) / tile_size).integer();

            // Clamp to map bounds
            tile_left = bn::max(0, tile_left);
            tile_right = bn::min(columns - 1, tile_right);
            tile_top = bn::max(0, tile_top);
            tile_bottom = bn::min(rows - 1, tile_bottom);

            // Update tiles in the hitbox zone area
            for (int x = tile_left; x <= tile_right; x++)
            {
                for (int y = tile_top; y <= tile_bottom; y++)
                {
                    int cell_index = x + y * columns;
                    cells[cell_index] = bn::regular_bg_map_cell(tile_index);
                }
            }
        }

        // Method to refresh all zone tiles based on current debug state and active zones
        void refresh_all_zones(bool debug_enabled, bool has_merchant = false, bn::optional<bn::fixed_point> merchant_pos = bn::nullopt)
        {
            // First, reset all tiles to background
            for (int x = 0; x < columns; x++)
            {
                for (int y = 0; y < rows; y++)
                {
                    int cell_index = x + y * columns;
                    cells[cell_index] = bn::regular_bg_map_cell(_background_tile);
                }
            }

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

    // Initialize static cells array in EWRAM
    BN_DATA_EWRAM bn::regular_bg_map_cell bg_map::cells[bg_map::cells_count];

    World::World() : _player(nullptr),
                     _level(nullptr),
                     _minimap(nullptr),
                     _sword_bg(bn::nullopt),
                     _merchant(nullptr),
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

        bg_map bg_map_obj(world_id);
        bn::regular_bg_tiles_ptr tiles = bn::regular_bg_tiles_items::tiles.create_tiles();
        bn::bg_palette_ptr palette = bn::bg_palette_items::palette.create_palette();
        bn::regular_bg_map_ptr bg_map_ptr = bg_map_obj.map_item.create_map(tiles, palette);
        bn::regular_bg_ptr bg = bn::regular_bg_ptr::create(bg_map_ptr);
        bg.set_camera(camera);

        _level = new Level(bg_map_ptr);
        _player->spawn(spawn_location, camera);

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

        // Initialize world-specific content
        _init_world_specific_content(world_id, camera, bg, text_generator);

        // Initialize hitbox debug system
        _hitbox_debug.initialize(camera);

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
                bool new_debug_state = !_hitbox_debug.is_enabled();
                _hitbox_debug.set_enabled(new_debug_state);

                // Refresh all zone visibility for visual debugging (not collision)
                bn::optional<bn::fixed_point> merchant_pos = _merchant ? bn::optional<bn::fixed_point>(_merchant->pos()) : bn::nullopt;
                bg_map_obj.refresh_all_zones(new_debug_state, _merchant != nullptr, merchant_pos);

                // Reload the background to reflect changes
                bg_map_ptr.reload_cells_ref();
            }

            // Clear hitbox debug markers from previous frame
            if (_hitbox_debug.is_enabled())
            {
                _hitbox_debug.clear_all();
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
                _level->set_merchant_zone(_merchant->pos());

                // Update merchant zone visual tiles if debug mode is enabled
                if (_hitbox_debug.is_enabled())
                {
                    // Update merchant zone tiles efficiently when merchant moves
                    bn::optional<bn::fixed_point> merchant_pos = bn::optional<bn::fixed_point>(_merchant->pos());
                    if (bg_map_obj.update_merchant_zone(true, true, merchant_pos))
                    {
                        bg_map_ptr.reload_cells_ref();
                    }
                }

                // Disable merchant collision during conversations (but keep sprite visible)
                bool conversation_active = _merchant->is_talking() || _player->listening();
                _level->set_merchant_zone_enabled(!conversation_active); // Disable collision zone, not sprite

                // Update z-ordering for player, companion, and gun
                _player->update_z_order();

                // Update merchant's z-order based on Y position
                int merchant_z = -_merchant->pos().y().integer();
                _merchant->set_sprite_z_order(merchant_z);

                // Merchant uses tile-based collision system through Level class:
                // Zone 1: Interaction trigger (100x100) - uses Level::is_in_merchant_interaction_zone() tile-based
                // Zone 2: Physical collision zone (24x24) - handled by Level::is_in_hitbox_zone() for tile-based collision
                // Note: NPCs don't use sprite hitboxes - all collision/interaction handled by Level tile-based system
                // Debug visualization: tile 4 markers (100x100 interaction), tile 3 markers (24x24 collision)
            }
            else
            {
                // Update merchant zone visual tiles if debug mode is enabled (clear them when no merchant)
                if (_hitbox_debug.is_enabled())
                {
                    if (bg_map_obj.update_merchant_zone(true, false, bn::nullopt))
                    {
                        bg_map_ptr.reload_cells_ref();
                    }
                }
            }

            // Check for merchant interaction BEFORE player input
            if (_merchant && _level->is_in_merchant_interaction_zone(_player->pos()))
            {
                // Update merchant's near player flag for UI display (fixes missing interaction prompt)
                _merchant->set_near_player(true);

                if (bn::keypad::a_pressed() && !merchant_was_talking && !_player->listening())
                {
                    // Start conversation
                    _player->set_listening(true);
                    _merchant->talk();
                }
                else if (!_merchant->is_talking() && merchant_was_talking)
                {
                    // Conversation just ended
                    _player->set_listening(false);
                }
            }
            else
            {
                // Clear merchant's near player flag when not in interaction zone
                if (_merchant)
                {
                    _merchant->set_near_player(false);
                }

                if (_player->listening())
                {
                    // Clear listening state if player moves away
                    _player->set_listening(false);
                }
            }

            // Now update player (which includes input handling)
            _player->update();
            _player->update_gun_position(_player->facing_direction());

            // Update hitbox debug visualization for player
            if (_hitbox_debug.is_enabled())
            {
                _hitbox_debug.update_player_hitbox(*_player);

                // Update zone tiles visualization (now with performance optimization)
                _hitbox_debug.update_zone_tiles(*_level, bg);

                // Merchant uses tile-based collision and interaction systems through Level class
                // NPCs don't use hitboxes - all collision/interaction handled by Level tile-based system

                // Update sword zone visualization
                _hitbox_debug.update_sword_zone(*_level);

                // Update merchant zone visualization
                _hitbox_debug.update_merchant_zone(*_level);
            }

            // Update player position and check for collisions
            bn::fixed_point new_pos = _player->pos();

            // Check for zone collisions - prevent player from walking through zones
            bool position_valid = _level->is_position_valid(new_pos);

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

            // Precise camera tracking
            bn::fixed_point player_pos = _player->pos();

            // Interpolate camera position to match player
            camera.set_x(player_pos.x());
            camera.set_y(player_pos.y());

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
                enemy.update(_player->pos(), *_level, _player->listening());

                // Update hitbox debug visualization for this enemy
                if (_hitbox_debug.is_enabled())
                {
                    _hitbox_debug.update_enemy_hitbox(enemy);
                }

                // Check for collision with player
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
