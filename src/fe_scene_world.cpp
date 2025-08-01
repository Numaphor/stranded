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

        bg_map() : map_item(cells[0], bn::size(columns, rows))
        {
            // Fill all squares with a default tile (index 1)
            for (int x = 0; x < columns; x++)
            {
                for (int y = 0; y < rows; y++)
                {
                    int cell_index = x + y * columns;
                    cells[cell_index] = bn::regular_bg_map_cell(1); // Set a default tile index
                }
            }
        }

        // Method to show/hide zone tiles for collision and debug visualization
        void set_zone_visible(bool visible)
        {
            int tile_index = visible ? 2 : 1; // Use tile 2 for zone when visible, tile 1 when hidden

            // Draw a 9x4 rectangle (width increased by 50% to the right) moved down-left by 64 pixels from center
            for (int x = (columns / 2) - (6 / 2) - 10; x < (columns / 2) + (15 / 2) - 10; x++)
            {
                for (int y = (rows / 2) - (4 / 2) + 4; y < (rows / 2) + (4 / 2) + 4; y++)
                {
                    int cell_index = x + y * columns;
                    cells[cell_index] = bn::regular_bg_map_cell(tile_index);
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
                     _merchant(nullptr)
    {
        // Create player sprite with correct shape and size
        bn::sprite_builder builder(bn::sprite_items::hero);
        builder.set_bg_priority(1);
        _player = new Player(builder.release_build());
    }

    fe::Scene fe::World::execute(bn::fixed_point spawn_location)
    {
        bn::camera_ptr camera = bn::camera_ptr::create(0, 0);

        bg_map bg_map_obj;
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

        // Create merchant NPC
        _merchant = new MerchantNPC(bn::fixed_point(100, -50), camera, text_generator);

        // Initialize hitbox debug system
        _hitbox_debug.initialize(camera);

        // Spawn 3 spearguard enemies
        _enemies.push_back(Enemy(0, -100, camera, bg, ENEMY_TYPE::SPEARGUARD, 3));
        _enemies.push_back(Enemy(50, -80, camera, bg, ENEMY_TYPE::SPEARGUARD, 3));
        _enemies.push_back(Enemy(-50, -120, camera, bg, ENEMY_TYPE::SPEARGUARD, 3));

        while (true)
        {
            bn::core::update();

            // Debug input handling - Toggle hitbox visualization with START + SELECT keys
            if (bn::keypad::select_held() && bn::keypad::start_pressed())
            {
                bool new_debug_state = !_hitbox_debug.is_enabled();
                _hitbox_debug.set_enabled(new_debug_state);

                // Also toggle zone visibility for visual debugging (not collision)
                bg_map_obj.set_zone_visible(new_debug_state);

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

                // Update z-ordering for player, companion, and gun
                _player->update_z_order();

                // Update merchant's z-order based on Y position
                int merchant_z = -_merchant->pos().y().integer();
                _merchant->set_sprite_z_order(merchant_z);

                // Update hitbox debug visualization for merchant
                if (_hitbox_debug.is_enabled())
                {
                    _hitbox_debug.update_npc_hitbox(*_merchant);
                }
            }

            // Check for merchant interaction BEFORE player input
            if (_merchant && _merchant->check_trigger(_player->pos()))
            {
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
            else if (_player->listening())
            {
                // Clear listening state if player moves away
                _player->set_listening(false);
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
            }

            // Update player position and check for collisions
            bn::fixed_point new_pos = _player->pos();

            // Check collision between player and merchant using unified collision system
            bool colliding_with_merchant = _merchant && !_merchant->is_talking() && !_player->listening() &&
                                           fe::Collision::check_player_npc(*_player, *_merchant);

            // Check for zone collisions - prevent player from walking through zones
            // If new position is invalid or colliding with merchant, revert position
            if (!_level->is_position_valid(new_pos) || colliding_with_merchant)
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
}
