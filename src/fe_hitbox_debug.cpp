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
        _merchant_zone_markers.clear();
        _sword_zone_markers.clear();

        // Don't reset zone cache - it should persist across frames for performance
        // Zone boundaries don't change during gameplay
    }

    void HitboxDebug::set_enabled(bool enabled)
    {
        _enabled = enabled;

        // Clear zone bounds cache when debug state changes to force re-scanning
        // This ensures zone tiles are properly detected when debug mode is toggled
        _zone_bounds_cached = false;

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
            _merchant_zone_markers.set_visible(false);
            _sword_zone_markers.set_visible(false);
        }
    }

    void HitboxDebug::_update_markers(const Hitbox &hitbox, HitboxMarkers &markers)
    {
        MarkerOffsetConfig standard_config(0, 0, 0, 0);
        _update_markers_with_config(hitbox, markers, standard_config);
    }

    void HitboxDebug::_update_player_markers(const Hitbox &hitbox, HitboxMarkers &markers)
    {
        MarkerOffsetConfig player_config(
            fe::hitbox_constants::PLAYER_MARKER_X_OFFSET,
            fe::hitbox_constants::PLAYER_MARKER_Y_OFFSET,
            -1, -1);
        _update_markers_with_config(hitbox, markers, player_config);
    }

    void HitboxDebug::_update_markers_with_config(const Hitbox &hitbox, HitboxMarkers &markers,
                                                  const MarkerOffsetConfig &config,
                                                  bool use_hitbox_markers,
                                                  bool enable_blending)
    {
        if (!_camera.has_value())
        {
            return;
        }

        // Calculate marker positions using the configuration
        bn::fixed_point top_left_pos = _calculate_top_left_marker_pos(hitbox, config.top_left_x, config.top_left_y);
        bn::fixed_point bottom_right_pos = _calculate_bottom_right_marker_pos(hitbox, config.bottom_right_x, config.bottom_right_y);

        // Determine which marker pointers to use
        bn::optional<bn::sprite_ptr> &tl_marker = use_hitbox_markers ? markers.hitbox_top_left : markers.top_left;
        bn::optional<bn::sprite_ptr> &br_marker = use_hitbox_markers ? markers.hitbox_bottom_right : markers.bottom_right;

        // Create or update top-left marker
        if (!tl_marker.has_value())
        {
            tl_marker = _create_marker(top_left_pos, false);
            if (enable_blending)
            {
                tl_marker->set_blending_enabled(true);
            }
        }
        else
        {
            tl_marker->set_position(top_left_pos);
            tl_marker->set_visible(_enabled);
        }

        // Create or update bottom-right marker (rotated 180 degrees)
        if (!br_marker.has_value())
        {
            br_marker = _create_marker(bottom_right_pos, true);
            if (enable_blending)
            {
                br_marker->set_blending_enabled(true);
            }
        }
        else
        {
            br_marker->set_position(bottom_right_pos);
            br_marker->set_visible(_enabled);
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

        // Sprite center positioning adjustments (same logic as sword and merchant zones)
        // Butano sprites are positioned by their center, not top-left corner
        constexpr bn::fixed SPRITE_CENTER_OFFSET = fe::hitbox_constants::MARKER_SPRITE_SIZE / 2; // 2 pixels

        // For zone tiles, we want markers positioned at the actual zone boundaries
        // The cached bounds already represent the exact tile boundaries, so we just need sprite center adjustment
        constexpr bn::fixed ZONE_TOP_LEFT_X_ADJUST = SPRITE_CENTER_OFFSET - 1;        // 2 (center) - 1 (inset) = 1
        constexpr bn::fixed ZONE_TOP_LEFT_Y_ADJUST = SPRITE_CENTER_OFFSET - 1;        // 2 (center) - 1 (inset) = 1
        constexpr bn::fixed ZONE_BOTTOM_RIGHT_X_ADJUST = -(SPRITE_CENTER_OFFSET - 1); // -(2 - 1) = -1
        constexpr bn::fixed ZONE_BOTTOM_RIGHT_Y_ADJUST = -(SPRITE_CENTER_OFFSET - 1); // -(2 - 1) = -1

        // Use cached zone bounds for marker positions with sprite center adjustments
        bn::fixed_point top_left_pos(_zone_min_x + ZONE_TOP_LEFT_X_ADJUST, _zone_min_y + ZONE_TOP_LEFT_Y_ADJUST);
        bn::fixed_point bottom_right_pos(_zone_max_x + ZONE_BOTTOM_RIGHT_X_ADJUST, _zone_max_y + ZONE_BOTTOM_RIGHT_Y_ADJUST);

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

    void HitboxDebug::update_merchant_zone(const Level &level)
    {
        if (!_camera.has_value() || !_enabled)
        {
            _merchant_zone_markers.set_visible(false);
            return;
        }

        // Get merchant zone information
        bn::optional<bn::fixed_point> merchant_center = level.get_merchant_zone_center();
        bool merchant_zone_enabled = level.is_merchant_zone_enabled();

        // Debug: Create a simple visual indicator when no merchant is found
        if (!merchant_center.has_value())
        {
            _merchant_zone_markers.set_visible(false);
            // Early return - no merchant zone center set (likely no merchant in this world)
            return;
        }

        if (!merchant_zone_enabled)
        {
            _merchant_zone_markers.set_visible(false);
            // Early return - merchant zone disabled (likely during conversation)
            return;
        }

        // Use the actual COLLISION zone dimensions: 24x24 pixel area
        // This matches what Level::is_in_hitbox_zone() uses (the actual collision zone, not interaction zone)
        const bn::fixed actual_collision_width = level.get_merchant_zone_width();   // 24 pixels
        const bn::fixed actual_collision_height = level.get_merchant_zone_height(); // 24 pixels
        const bn::fixed_point &center = merchant_center.value();

        // Calculate the EXACT collision boundaries as used in Level::is_in_hitbox_zone()
        // Collision uses: position.x() >= zone_left && position.x() < zone_right (same pattern as sword zone)
        // This means collision area is [zone_left, zone_right-1] inclusive
        const bn::fixed zone_left = center.x() - actual_collision_width / 2;
        const bn::fixed zone_right = center.x() + actual_collision_width / 2;
        const bn::fixed zone_top = center.y() - actual_collision_height / 2;
        const bn::fixed zone_bottom = center.y() + actual_collision_height / 2;

        // Adjust for exclusive upper bounds (same logic as sword zone)
        const bn::fixed collision_left = zone_left;
        const bn::fixed collision_right = zone_right - 1; // -1 because collision boundary is exclusive
        const bn::fixed collision_top = zone_top;
        const bn::fixed collision_bottom = zone_bottom - 1;                      // -1 because collision boundary is exclusive
        const bn::fixed collision_width = collision_right - collision_left + 1;  // +1 because we want inclusive width
        const bn::fixed collision_height = collision_bottom - collision_top + 1; // +1 because we want inclusive height

        // Create a hitbox representing the actual collision zone used by Level::is_in_hitbox_zone()
        // Note: NPCs don't have collision hitboxes - all collision is handled by Level tile-based system
        // - Collision zone (23x23 actual collision area): where player cannot walk through the merchant (tile-based)
        Hitbox merchant_collision_hitbox(collision_left, collision_top, collision_width, collision_height);

        // Use simple, known working values for merchant zone markers
        // Try the established pattern: small positive offsets for positioning
        MarkerOffsetConfig collision_config(4, 4, -1, -1);
        _update_markers_with_config(merchant_collision_hitbox, _merchant_zone_markers, collision_config, false, true);
    }

    void HitboxDebug::update_sword_zone(const Level &level)
    {
        if (!_camera.has_value() || !_enabled)
        {
            _sword_zone_markers.set_visible(false);
            return;
        }

        // Pre-calculated sword zone constants (these never change during gameplay)
        // Tile-based coordinates from Level::is_in_sword_zone()
        constexpr int sword_zone_tile_left = 147;
        constexpr int sword_zone_tile_right = 157; // exclusive upper bound
        constexpr int sword_zone_tile_top = 162;
        constexpr int sword_zone_tile_bottom = 166; // exclusive upper bound
        constexpr int tile_size = 8;
        constexpr int map_offset = 1280;

        // Calculate the EXACT collision boundaries as used in Level::is_in_sword_zone()
        // Collision uses: position.x() >= zone_left && position.x() < zone_right
        // This means collision area is [zone_left, zone_right-1] inclusive
        constexpr bn::fixed collision_left = sword_zone_tile_left * tile_size - map_offset;
        constexpr bn::fixed collision_right = sword_zone_tile_right * tile_size - map_offset;
        constexpr bn::fixed collision_top = sword_zone_tile_top * tile_size - map_offset;
        constexpr bn::fixed collision_bottom = sword_zone_tile_bottom * tile_size - map_offset; // exclusive upper bound handled by < zone_bottom
        constexpr bn::fixed collision_width = collision_right - collision_left;
        constexpr bn::fixed collision_height = collision_bottom - collision_top;

        // Sprite center positioning adjustments
        // Butano sprites are positioned by their center, not top-left corner
        // For 4x4 marker sprites, center is 2 pixels from top-left corner
        constexpr bn::fixed SPRITE_CENTER_OFFSET = fe::hitbox_constants::MARKER_SPRITE_SIZE / 2; // 2 pixels

        // Marker positioning: account for sprite center + visual fine-tuning
        // Original working values were +4,+4,-1,-1, now derived logically:
        constexpr bn::fixed TOP_LEFT_X_ADJUST = SPRITE_CENTER_OFFSET + 2;        // 2 (center) + 2 (alignment) = 4
        constexpr bn::fixed TOP_LEFT_Y_ADJUST = SPRITE_CENTER_OFFSET + 2;        // 2 (center) + 2 (alignment) = 4
        constexpr bn::fixed BOTTOM_RIGHT_X_ADJUST = -(SPRITE_CENTER_OFFSET - 1); // -(2 - 1) = -1
        constexpr bn::fixed BOTTOM_RIGHT_Y_ADJUST = -(SPRITE_CENTER_OFFSET - 1); // -(2 - 1) = -1

        // Create hitbox representing the exact collision area
        Hitbox sword_zone_hitbox(collision_left, collision_top, collision_width, collision_height);

        // Apply marker positioning that accounts for sprite center positioning
        MarkerOffsetConfig sword_config(TOP_LEFT_X_ADJUST, TOP_LEFT_Y_ADJUST,
                                        BOTTOM_RIGHT_X_ADJUST, BOTTOM_RIGHT_Y_ADJUST);
        _update_markers_with_config(sword_zone_hitbox, _sword_zone_markers, sword_config, false, true);
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
