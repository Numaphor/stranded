#include "fe_hitbox_debug.h"
#include "fe_player.h"
#include "fe_enemy.h"
#include "fe_npc.h"
#include "fe_level.h"
#include "bn_sprite_items_hitbox_marker.h"
#include "bn_sprite_palette_ptr.h"
#include "bn_regular_bg_map_cell_info.h"
#include "bn_colors.h"
#include "bn_blending.h"

namespace fe
{
    HitboxDebug::HitboxDebug() : _enabled(false), _zone_bounds_cached(false), _initialized(false)
    {
    }

    void HitboxDebug::initialize(bn::camera_ptr camera)
    {
        _camera = camera;
        _initialized = true;

        // Set up blending for visual distinction between marker types
        bn::blending::set_transparency_alpha(0.7); // Make blended sprites semi-transparent
    }

    void HitboxDebug::update_player_hitbox(const Player &player)
    {
        if (!_enabled || !_initialized)
        {
            return;
        }

        // Get the player's hitbox
        Hitbox hitbox = player.get_hitbox();
        _update_player_markers(hitbox, _player_markers);
    }

    void HitboxDebug::update_enemy_hitbox(const Enemy &enemy)
    {
        if (!_enabled || !_initialized)
        {
            return;
        }

        // For now, we'll use the first available slot in the enemy markers vector
        // In a more sophisticated system, we'd track enemies by ID
        if (_enemy_markers.full())
        {
            return; // No more slots available
        }

        // Get the enemy's normal hitbox
        Hitbox hitbox = enemy.get_hitbox();

        // Check if enemy has an extended attack hitbox
        Hitbox attack_hitbox = enemy.get_attack_hitbox();

        // Use attack hitbox if it's different from normal hitbox (enemy is attacking)
        if (attack_hitbox.x() != hitbox.x() || attack_hitbox.y() != hitbox.y() ||
            attack_hitbox.width() != hitbox.width() || attack_hitbox.height() != hitbox.height())
        {
            hitbox = attack_hitbox;
        }

        // Find an empty slot or create a new one
        HitboxMarkers *markers = nullptr;
        for (auto &marker_pair : _enemy_markers)
        {
            if (!marker_pair.top_left.has_value())
            {
                markers = &marker_pair;
                break;
            }
        }

        if (!markers)
        {
            // Add a new marker pair if we have space
            _enemy_markers.push_back(HitboxMarkers());
            markers = &_enemy_markers.back();
        }

        _update_markers(hitbox, *markers);
    }

    void HitboxDebug::update_npc_hitbox(const NPC &npc)
    {
        if (!_enabled || !_initialized)
        {
            return;
        }

        // Get the NPC's hitbox
        Hitbox hitbox = npc.get_hitbox();

        // Find an empty slot or create a new one
        HitboxMarkers *markers = nullptr;
        for (auto &marker_pair : _npc_markers)
        {
            if (!marker_pair.top_left.has_value())
            {
                markers = &marker_pair;
                break;
            }
        }

        if (!markers)
        {
            // Add a new marker pair if we have space
            _npc_markers.push_back(HitboxMarkers());
            markers = &_npc_markers.back();
        }

        // Check if this is a merchant NPC and use specialized positioning
        if (npc.type() == NPC_TYPE::MERCHANT)
        {
            // Show both action radius and actual hitbox for merchants
            _update_merchant_action_radius_markers(hitbox, *markers);
            _update_merchant_hitbox_markers(hitbox, *markers);
        }
        else
        {
            _update_markers(hitbox, *markers);
        }
    }

    void HitboxDebug::clear_all()
    {
        _player_markers.clear();

        for (auto &markers : _enemy_markers)
        {
            markers.clear();
        }
        _enemy_markers.clear();

        for (auto &markers : _npc_markers)
        {
            markers.clear();
        }
        _npc_markers.clear();

        _zone_markers.clear();

        // Don't reset zone cache - it should persist across frames for performance
        // Zone boundaries don't change during gameplay
    }

    void HitboxDebug::set_enabled(bool enabled)
    {
        _enabled = enabled;

        if (!enabled)
        {
            // Hide all markers when disabled
            _player_markers.set_visible(false);

            for (auto &markers : _enemy_markers)
            {
                markers.set_visible(false);
            }

            for (auto &markers : _npc_markers)
            {
                markers.set_visible(false);
            }

            _zone_markers.set_visible(false);
        }
    }

    void HitboxDebug::_update_markers(const Hitbox &hitbox, HitboxMarkers &markers)
    {
        if (!_camera.has_value())
        {
            return;
        }

        // Calculate marker positions using helper functions
        bn::fixed_point top_left_pos = _calculate_top_left_marker_pos(hitbox);
        bn::fixed_point bottom_right_pos = _calculate_bottom_right_marker_pos(hitbox);

        // Create or update top-left marker
        if (!markers.top_left.has_value())
        {
            markers.top_left = _create_marker(top_left_pos, false);
        }
        else
        {
            markers.top_left->set_position(top_left_pos);
            markers.top_left->set_visible(_enabled);
        }

        // Create or update bottom-right marker (rotated 180 degrees)
        if (!markers.bottom_right.has_value())
        {
            markers.bottom_right = _create_marker(bottom_right_pos, true);
        }
        else
        {
            markers.bottom_right->set_position(bottom_right_pos);
            markers.bottom_right->set_visible(_enabled);
        }
    }

    void HitboxDebug::_update_player_markers(const Hitbox &hitbox, HitboxMarkers &markers)
    {
        if (!_camera.has_value())
        {
            return;
        }

        // Calculate marker positions for player with visual adjustments
        bn::fixed_point top_left_pos = _calculate_top_left_marker_pos(hitbox,
                                                                      fe::hitbox_constants::PLAYER_MARKER_X_OFFSET,
                                                                      fe::hitbox_constants::PLAYER_MARKER_Y_OFFSET);
        constexpr bn::fixed PLAYER_MARKER_BR_X_OFFSET = -1;
        constexpr bn::fixed PLAYER_MARKER_BR_Y_OFFSET = -1;
        bn::fixed_point bottom_right_pos = _calculate_bottom_right_marker_pos(hitbox, PLAYER_MARKER_BR_X_OFFSET, PLAYER_MARKER_BR_Y_OFFSET);

        // Create or update top-left marker
        if (!markers.top_left.has_value())
        {
            markers.top_left = _create_marker(top_left_pos, false);
        }
        else
        {
            markers.top_left->set_position(top_left_pos);
            markers.top_left->set_visible(_enabled);
        }

        // Create or update bottom-right marker (rotated 180 degrees)
        if (!markers.bottom_right.has_value())
        {
            markers.bottom_right = _create_marker(bottom_right_pos, true);
        }
        else
        {
            markers.bottom_right->set_position(bottom_right_pos);
            markers.bottom_right->set_visible(_enabled);
        }
    }

    void HitboxDebug::_update_merchant_action_radius_markers(const Hitbox &hitbox, HitboxMarkers &markers)
    {
        if (!_camera.has_value())
        {
            return;
        }

        // Calculate merchant action radius marker positions using simplified offsets
        // Top-left marker: base position adjusted for merchant action radius visibility, moved 2 pixels up-left
        bn::fixed top_left_x_offset = fe::hitbox_constants::PLAYER_MARKER_X_OFFSET -
                                      fe::hitbox_constants::MERCHANT_BASE_OFFSET +
                                      fe::hitbox_constants::MERCHANT_X_ADJUSTMENT - 2;
        bn::fixed top_left_y_offset = fe::hitbox_constants::PLAYER_MARKER_Y_OFFSET -
                                      fe::hitbox_constants::MERCHANT_BASE_OFFSET +
                                      fe::hitbox_constants::MERCHANT_Y_ADJUSTMENT - 2;

        bn::fixed_point top_left_pos = _calculate_top_left_marker_pos(hitbox, top_left_x_offset, top_left_y_offset);

        // Bottom-right marker: standard position with merchant-specific offsets
        bn::fixed_point bottom_right_pos = _calculate_bottom_right_marker_pos(hitbox,
                                                                              -fe::hitbox_constants::MERCHANT_BR_X_OFFSET,
                                                                              -fe::hitbox_constants::MERCHANT_BR_Y_OFFSET);

        // Create or update top-left marker
        if (!markers.top_left.has_value())
        {
            markers.top_left = _create_marker(top_left_pos, false);
        }
        else
        {
            markers.top_left->set_position(top_left_pos);
            markers.top_left->set_visible(_enabled);
        }

        // Create or update bottom-right marker (rotated 180 degrees)
        if (!markers.bottom_right.has_value())
        {
            markers.bottom_right = _create_marker(bottom_right_pos, true);
        }
        else
        {
            markers.bottom_right->set_position(bottom_right_pos);
            markers.bottom_right->set_visible(_enabled);
        }
    }

    void HitboxDebug::_update_merchant_hitbox_markers(const Hitbox &hitbox, HitboxMarkers &markers)
    {
        if (!_camera.has_value())
        {
            return;
        }

        // Use the same base positioning as action radius markers but create smaller 16x16 area
        // Calculate the action radius center point first
        bn::fixed action_top_left_x_offset = fe::hitbox_constants::PLAYER_MARKER_X_OFFSET -
                                             fe::hitbox_constants::MERCHANT_BASE_OFFSET +
                                             fe::hitbox_constants::MERCHANT_X_ADJUSTMENT - 2;
        bn::fixed action_top_left_y_offset = fe::hitbox_constants::PLAYER_MARKER_Y_OFFSET -
                                             fe::hitbox_constants::MERCHANT_BASE_OFFSET +
                                             fe::hitbox_constants::MERCHANT_Y_ADJUSTMENT - 2;

        // Get the action radius marker positions to find the center
        bn::fixed_point action_top_left = _calculate_top_left_marker_pos(hitbox, action_top_left_x_offset, action_top_left_y_offset);
        bn::fixed_point action_bottom_right = _calculate_bottom_right_marker_pos(hitbox,
                                                                                 -fe::hitbox_constants::MERCHANT_BR_X_OFFSET,
                                                                                 -fe::hitbox_constants::MERCHANT_BR_Y_OFFSET);

        // Calculate center of action radius area
        bn::fixed action_center_x = (action_top_left.x() + action_bottom_right.x() + fe::hitbox_constants::MARKER_SPRITE_SIZE) / 2;
        bn::fixed action_center_y = (action_top_left.y() + action_bottom_right.y() + fe::hitbox_constants::MARKER_SPRITE_SIZE) / 2;
        
        // Create 16x16 hitbox area centered within the action radius
        bn::fixed_point hitbox_top_left_pos(action_center_x - 8, action_center_y - 8);
        bn::fixed_point hitbox_bottom_right_pos(
            action_center_x + 8 - fe::hitbox_constants::MARKER_SPRITE_SIZE,
            action_center_y + 8 - fe::hitbox_constants::MARKER_SPRITE_SIZE
        );

        // Create or update hitbox top-left marker with blending effect
        if (!markers.hitbox_top_left.has_value())
        {
            markers.hitbox_top_left = _create_marker(hitbox_top_left_pos, false);
            // Enable blending to distinguish hitbox markers from action radius markers
            markers.hitbox_top_left->set_blending_enabled(true);
        }
        else
        {
            markers.hitbox_top_left->set_position(hitbox_top_left_pos);
            markers.hitbox_top_left->set_visible(_enabled);
        }

        // Create or update hitbox bottom-right marker (rotated 180 degrees) with blending effect
        if (!markers.hitbox_bottom_right.has_value())
        {
            markers.hitbox_bottom_right = _create_marker(hitbox_bottom_right_pos, true);
            // Enable blending to distinguish hitbox markers from action radius markers
            markers.hitbox_bottom_right->set_blending_enabled(true);
        }
        else
        {
            markers.hitbox_bottom_right->set_position(hitbox_bottom_right_pos);
            markers.hitbox_bottom_right->set_visible(_enabled);
        }
    }

    bn::sprite_ptr HitboxDebug::_create_marker(bn::fixed_point position, bool rotated)
    {
        // Create the marker sprite using the hitbox_marker sprite item
        bn::sprite_ptr marker = bn::sprite_items::hitbox_marker.create_sprite(position.x(), position.y());

        // Set camera if available
        if (_camera.has_value())
        {
            marker.set_camera(_camera.value());
        }

        // Set highest z-order to ensure markers are always drawn on top of everything including sword
        marker.set_z_order(-32767); // Maximum negative z-order value for highest priority
        marker.set_bg_priority(0);  // Must be 0 to appear above background layers with priority 0-3

        // Ensure marker is not affected by windows (sword uses window system)
        marker.set_window_enabled(false);

        // Rotate 180 degrees if requested
        if (rotated)
        {
            marker.set_rotation_angle(180);
        }

        // Set visibility based on enabled state
        marker.set_visible(_enabled);

        return marker;
    }

    void HitboxDebug::update_zone_tiles(const Level &level, const bn::regular_bg_ptr &bg)
    {
        if (!_camera.has_value() || !_enabled)
        {
            return;
        }

        // Get zone tiles
        const auto &zone_tiles = level.zone_tiles();
        if (zone_tiles.empty())
        {
            _zone_markers.set_visible(false);
            return;
        }

        // Use cached zone bounds if available to avoid expensive map scanning every frame
        if (!_zone_bounds_cached)
        {
            // Only scan the background map once to find zone boundaries
            const bn::span<const bn::regular_bg_map_cell> cells = bg.map().cells_ref().value();
            int map_width = bg.map().dimensions().width();
            int map_height = bg.map().dimensions().height();

            // Find min/max cell coordinates for zone tiles
            int min_cell_x = map_width, max_cell_x = -1;
            int min_cell_y = map_height, max_cell_y = -1;
            bool found_zone = false;

            for (int y = 0; y < map_height; ++y)
            {
                for (int x = 0; x < map_width; ++x)
                {
                    int cell_index = y * map_width + x;
                    if (cell_index < cells.size())
                    {
                        bn::regular_bg_map_cell cell = cells.at(cell_index);
                        int tile_index = bn::regular_bg_map_cell_info(cell).tile_index();

                        // Check if this tile is one of our zone tiles
                        for (int zone_tile : zone_tiles)
                        {
                            if (tile_index == zone_tile)
                            {
                                found_zone = true;
                                min_cell_x = bn::min(min_cell_x, x);
                                max_cell_x = bn::max(max_cell_x, x);
                                min_cell_y = bn::min(min_cell_y, y);
                                max_cell_y = bn::max(max_cell_y, y);
                                break;
                            }
                        }
                    }
                }
            }

            if (!found_zone)
            {
                _zone_markers.set_visible(false);
                return;
            }

            // Convert cell coordinates to world positions and cache them
            // Each cell is 8x8 pixels, map is offset by (map_width * 4, map_height * 4)
            const int map_offset_x = (map_width * 4);
            const int map_offset_y = (map_height * 4);

            // Calculate zone bounds in world coordinates and cache them
            _zone_min_x = min_cell_x * 8 - map_offset_x;
            _zone_max_x = (max_cell_x + 1) * 8 - map_offset_x - 1;
            _zone_min_y = min_cell_y * 8 - map_offset_y;
            _zone_max_y = (max_cell_y + 1) * 8 - map_offset_y - 1;

            _zone_bounds_cached = true;
        }

        // Use cached zone bounds for marker positions
        // Move markers inward by 4x4 pixels for better visibility
        bn::fixed_point top_left_pos(_zone_min_x + 4, _zone_min_y + 4);
        bn::fixed_point bottom_right_pos(_zone_max_x - 4, _zone_max_y - 4);

        // Create or update top-left marker
        if (!_zone_markers.top_left.has_value())
        {
            _zone_markers.top_left = _create_marker(top_left_pos, false);
        }
        else
        {
            _zone_markers.top_left->set_position(top_left_pos);
            _zone_markers.top_left->set_visible(_enabled);
        }

        // Create or update bottom-right marker (rotated 180 degrees)
        if (!_zone_markers.bottom_right.has_value())
        {
            _zone_markers.bottom_right = _create_marker(bottom_right_pos, true);
        }
        else
        {
            _zone_markers.bottom_right->set_position(bottom_right_pos);
            _zone_markers.bottom_right->set_visible(_enabled);
        }
    }

    bn::fixed_point HitboxDebug::_calculate_top_left_marker_pos(const Hitbox &hitbox, bn::fixed x_offset, bn::fixed y_offset) const
    {
        return bn::fixed_point(hitbox.x() + x_offset, hitbox.y() + y_offset);
    }

    bn::fixed_point HitboxDebug::_calculate_bottom_right_marker_pos(const Hitbox &hitbox, bn::fixed x_offset, bn::fixed y_offset) const
    {
        // Position marker so its top-left corner aligns with the hitbox's bottom-right corner
        // Subtract marker size to align properly, then apply additional offsets
        return bn::fixed_point(
            hitbox.x() + hitbox.width() - fe::hitbox_constants::MARKER_SPRITE_SIZE + x_offset,
            hitbox.y() + hitbox.height() - fe::hitbox_constants::MARKER_SPRITE_SIZE + y_offset);
    }
}
