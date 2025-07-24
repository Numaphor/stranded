#include "fe_hitbox_debug.h"
#include "fe_player.h"
#include "fe_enemy.h"
#include "fe_npc.h"
#include "fe_level.h"
#include "bn_sprite_items_hitbox_marker.h"
#include "bn_regular_bg_map_cell_info.h"

namespace fe
{
    HitboxDebug::HitboxDebug() : _enabled(false), _zone_bounds_cached(false), _initialized(false)
    {
    }

    void HitboxDebug::initialize(bn::camera_ptr camera)
    {
        _camera = camera;
        _initialized = true;
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
            _update_merchant_markers(hitbox, *markers);
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

        // Reset zone cache when clearing all
        _zone_bounds_cached = false;
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

        // Calculate marker positions
        // Top-left marker at the top-left corner of the hitbox
        bn::fixed_point top_left_pos(hitbox.x(), hitbox.y());

        // Bottom-right marker at the bottom-right corner of the hitbox
        // Since we're using only the top-left 4x4 of the sprite, position it so the 4x4 marker
        // aligns with the bottom-right corner of the hitbox
        bn::fixed_point bottom_right_pos(hitbox.x() + hitbox.width() - 4,
                                         hitbox.y() + hitbox.height() - 4);

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

        // Calculate marker positions for player with adjusted top-left position
        // Top-left marker adjusted by +4x, +20y for better visual accuracy
        bn::fixed_point top_left_pos(hitbox.x() + 4, hitbox.y() + 20);

        // Bottom-right marker at the bottom-right corner of the hitbox (unchanged)
        bn::fixed_point bottom_right_pos(hitbox.x() + hitbox.width() - 4,
                                         hitbox.y() + hitbox.height() - 4);

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

    void HitboxDebug::_update_merchant_markers(const Hitbox &hitbox, HitboxMarkers &markers)
    {
        if (!_camera.has_value())
        {
            return;
        }

        // Calculate marker positions for merchant with adjusted top-left position for visibility
        // Move top-left marker up-left by ~36 pixels, then adjust right by 16 and down by 4, then further right by 4 and down by 4, then down by 4 more
        bn::fixed_point top_left_pos(hitbox.x() + 4 - 36 + 16 + 4, hitbox.y() + 20 - 36 + 4 + 4 + 4);

        // Bottom-right marker adjusted: move up-left by half tile (4 pixels each direction)
        // Previous: higher by 4 pixels (-4) and left by 12 pixels (-12)
        // New: up by 4 pixels (-4 from previous) and left by 4 pixels (-4 from previous)
        // Final: higher by 8 pixels (-8) and left by 16 pixels (-16)
        bn::fixed_point bottom_right_pos(hitbox.x() + hitbox.width() - 4 - 16,  // 16 pixels more to the left
                                         hitbox.y() + hitbox.height() - 4 - 8); // 8 pixels higher

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

    bn::sprite_ptr HitboxDebug::_create_marker(bn::fixed_point position, bool rotated)
    {
        // Create the marker sprite using the hitbox_marker sprite item
        bn::sprite_ptr marker = bn::sprite_items::hitbox_marker.create_sprite(position.x(), position.y());

        // Set camera if available
        if (_camera.has_value())
        {
            marker.set_camera(_camera.value());
        }

        // Set high z-order to ensure markers are drawn on top
        marker.set_z_order(-100);

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

        // Temporary test: Just show markers at fixed positions to see if they're visible
        // Marker offset: half of a 24x24 tile (12 pixels) to center the marker within the tile.
        constexpr int MARKER_HALF_TILE_OFFSET = 12; // Half of 24px tile size
        bn::fixed_point top_left_pos(-MARKER_HALF_TILE_OFFSET, -MARKER_HALF_TILE_OFFSET);
        bn::fixed_point bottom_right_pos(MARKER_HALF_TILE_OFFSET - 1, MARKER_HALF_TILE_OFFSET - 1);

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
}
