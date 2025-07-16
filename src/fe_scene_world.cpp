#include "fe_scene_world.h"
#include "fe_collision.h"
#include "fe_npc.h"
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

            // Draw a 4x4 square in the middle of the screen
            for (int x = (columns / 2) - (4 / 2); x < (columns / 2) + (4 / 2); x++)
            {
                for (int y = (rows / 2) - (4 / 2); y < (rows / 2) + (4 / 2); y++)
                {
                    int cell_index = x + y * columns;
                    cells[cell_index] = bn::regular_bg_map_cell(2); // Set a different tile index
                }
            }
        }
    }; // Place macro AFTER the struct definition

    // Initialize static cells array in EWRAM
    BN_DATA_EWRAM bn::regular_bg_map_cell bg_map::cells[bg_map::cells_count];

    World::World() : _player(nullptr),
                     _level(nullptr),
                     _minimap(nullptr),
                     _sword_bg(bn::nullopt)
    {
        // Create player sprite with correct shape and size
        bn::sprite_builder builder(bn::sprite_items::hero);
        builder.set_bg_priority(1);
        _player = new Player(builder.release_build());
    }

    fe::Scene fe::World::execute(bn::fixed_point spawn_location)
    {
        bn::camera_ptr camera = bn::camera_ptr::create(0, 0);

        bg_map bg_map;
        bn::regular_bg_tiles_ptr tiles = bn::regular_bg_tiles_items::tiles.create_tiles();
        bn::bg_palette_ptr palette = bn::bg_palette_items::palette.create_palette();
        bn::regular_bg_map_ptr bg_map_ptr = bg_map.map_item.create_map(tiles, palette);
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
        _player->sprite().set_camera(camera);

        // Create text generator for NPCs
        bn::sprite_text_generator text_generator(common::variable_8x8_sprite_font);

        // Add some enemies
        _enemies.push_back(Enemy(0, -100, camera, bg, ENEMY_TYPE::SLIME, 3));
        _enemies.push_back(Enemy(50, -80, camera, bg, ENEMY_TYPE::SLIME, 3));
        _enemies.push_back(Enemy(-50, -120, camera, bg, ENEMY_TYPE::SLIME, 3));

        // Add merchant NPC with 10 frame idle animation
        NPC merchant(bn::fixed_point(100, -50), camera, NPC_TYPE::MERCHANT, text_generator);

        while (true)
        {
            bn::core::update();

            _player->update();
            _player->update_gun_position(_player->facing_direction());

            // Update player position and check for collisions
            bn::fixed_point new_pos = _player->pos();

            // Check for collision with sword using Hitbox system
            constexpr bn::fixed sword_x = -64;     // 64 pixels left of center
            constexpr bn::fixed sword_y = 20;      // 20 pixels down from center
            constexpr bn::fixed sword_width = 64;  // Width of 64
            constexpr bn::fixed sword_height = 20; // Reduced height by ~70% (from 64 to ~20)

            // Create sword and player hitboxes
            fe::Hitbox sword_hitbox(sword_x, sword_y, sword_width, sword_height);
            fe::Hitbox player_hitbox(new_pos.x(), new_pos.y(), 32, 32);

            // Check collision between player and sword using Hitbox
            bool colliding_with_sword = player_hitbox.collides_with(sword_hitbox);

            // Check collision between player and merchant using unified collision system
            bool colliding_with_merchant = !merchant.is_talking() && !_player->listening() &&
                                           fe::Collision::check_player_npc(*_player, merchant);

            // Check for zone collisions - prevent player from walking through zones
            // If new position is invalid or colliding with sword or merchant, revert position
            if (!_level->is_position_valid(new_pos) || colliding_with_sword || colliding_with_merchant)
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
                constexpr bn::fixed sword_x = 0;
                constexpr bn::fixed sword_y = 0;

                // Get the internal window and update its position
                bn::rect_window internal_window = bn::rect_window::internal();

                // Calculate the sword's position in screen coordinates
                bn::fixed_point camera_pos(camera.x(), camera.y());
                bn::fixed_point sword_screen_pos = bn::fixed_point(sword_x, sword_y) - camera_pos;

                // Update window boundaries to be centered on the sword
                internal_window.set_boundaries(
                    sword_screen_pos.y() - SWORD_HALF_HEIGHT, // top
                    sword_screen_pos.x() - SWORD_HALF_WIDTH,  // left
                    sword_screen_pos.y() + SWORD_HALF_HEIGHT, // bottom
                    sword_screen_pos.x() + SWORD_HALF_WIDTH   // right
                );

                // Set priority based on player's Y position relative to the sword
                // When player is above the sword (lower Y), they should be behind it (higher priority)
                // When player is below the sword (higher Y), they should be in front of it (lower priority)
                const int sword_priority = (player_pos.y() > sword_y) ? 2 : 0; // 0 = above player, 2 = below player
                _sword_bg->set_priority(sword_priority);
            }

            // Update all enemies and check for collisions with player and bullets
            for (int i = 0; i < _enemies.size();)
            {
                Enemy &enemy = _enemies[i];
                enemy.update(_player->pos(), *_level, _player->listening());

                // Check for collision with player
                if (fe::Collision::check_player_enemy(*_player, enemy))
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

                // Remove dead enemies
                if (enemy.hp() <= 0)
                {
                    _enemies.erase(_enemies.begin() + i);
                }
                else
                {
                    i++;
                }
            }

            // Update merchant NPC
            merchant.update();

            // Check for merchant interaction
            if (merchant.check_trigger(_player->pos()))
            {
                if (bn::keypad::a_pressed())
                {
                    _player->set_listening(true);
                    merchant.talk();
                }
                else if (!merchant.is_talking())
                {
                    _player->set_listening(false);
                }
            }
            else
            {
                _player->set_listening(false);
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
                _enemies.push_back(Enemy(0, -100, camera, bg, ENEMY_TYPE::SLIME, 3));
                _enemies.push_back(Enemy(50, -80, camera, bg, ENEMY_TYPE::SLIME, 3));
                _enemies.push_back(Enemy(-50, -120, camera, bg, ENEMY_TYPE::SLIME, 3));

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
    }
}
