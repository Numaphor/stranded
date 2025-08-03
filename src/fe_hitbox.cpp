#include "fe_hitbox.h"
#include "bn_sprite_items_hitbox_marker.h"
#include "bn_blending.h"

namespace fe
{
    // === CONSTRUCTORS ===

    Hitbox::Hitbox() : _pos(0, 0), _width(0), _height(0) {}

    Hitbox::Hitbox(bn::fixed x, bn::fixed y, bn::fixed width, bn::fixed height) : _pos(x, y), _width(width), _height(height)
    {
    }

    Hitbox::Hitbox(bn::fixed x, bn::fixed y, bn::fixed width, bn::fixed height, HitboxType type)
        : _pos(x, y), _width(width), _height(height), _type(type)
    {
    }

    // === SETTERS AND BASIC OPERATIONS ===

    void Hitbox::set_x(bn::fixed x)
    {
        _pos.set_x(x);
    }

    void Hitbox::set_y(bn::fixed y)
    {
        _pos.set_y(y);
    }

    void Hitbox::set_position(bn::fixed_point position)
    {
        _pos = position;
    }

    void Hitbox::get_collision_points(bn::fixed_point pos, fe::directions direction, bn::fixed_point points[4]) const
    {
        // Edge offset to stay within bounds (one pixel inside the edge)
        constexpr bn::fixed EDGE_OFFSET = 1;

        // Calculate edge coordinates
        bn::fixed left = pos.x();
        bn::fixed right = pos.x() + _width - EDGE_OFFSET;
        bn::fixed top = pos.y();
        bn::fixed bottom = pos.y() + _height - EDGE_OFFSET;
        bn::fixed middle_x = pos.x() + _width / 2;
        bn::fixed quarter_x = pos.x() + _width / 4;
        bn::fixed middle_y = pos.y() + _height / 2;
        bn::fixed quarter_y = pos.y() + _height / 4;

        // Calculate points based on direction to check appropriate edges
        switch (direction)
        {
        case fe::directions::up:
            // Check top edge and corners when moving up
            points[0] = bn::fixed_point(left, top);      // Top-left
            points[1] = bn::fixed_point(right, top);     // Top-right
            points[2] = bn::fixed_point(middle_x, top);  // Top-middle
            points[3] = bn::fixed_point(quarter_x, top); // Top-quarter
            break;
        case fe::directions::down:
            // Check bottom edge and corners when moving down
            points[0] = bn::fixed_point(left, bottom);      // Bottom-left
            points[1] = bn::fixed_point(right, bottom);     // Bottom-right
            points[2] = bn::fixed_point(middle_x, bottom);  // Bottom-middle
            points[3] = bn::fixed_point(quarter_x, bottom); // Bottom-quarter
            break;
        case fe::directions::left:
            // Check left edge and corners when moving left
            points[0] = bn::fixed_point(left, top);       // Top-left
            points[1] = bn::fixed_point(left, bottom);    // Bottom-left
            points[2] = bn::fixed_point(left, middle_y);  // Middle-left
            points[3] = bn::fixed_point(left, quarter_y); // Quarter-left
            break;
        case fe::directions::right:
            // Check right edge and corners when moving right
            points[0] = bn::fixed_point(right, top);       // Top-right
            points[1] = bn::fixed_point(right, bottom);    // Bottom-right
            points[2] = bn::fixed_point(right, middle_y);  // Middle-right
            points[3] = bn::fixed_point(right, quarter_y); // Quarter-right
            break;
        default:
            // Default to all four corners
            points[0] = bn::fixed_point(left, top);     // Top-left
            points[1] = bn::fixed_point(right, top);    // Top-right
            points[2] = bn::fixed_point(left, bottom);  // Bottom-left
            points[3] = bn::fixed_point(right, bottom); // Bottom-right
            break;
        }
    }

    // === ZONE MANAGEMENT (from Level class) ===

    bool Hitbox::contains_point(const bn::fixed_point &position) const
    {
        return position.x() >= x() && position.x() < x() + width() &&
               position.y() >= y() && position.y() < y() + height();
    }

    bool Hitbox::is_in_sword_zone(const bn::fixed_point &position)
    {
        // Use centralized constants directly from fe namespace
        const bn::fixed zone_left = fe::SWORD_ZONE_TILE_LEFT * fe::TILE_SIZE - fe::MAP_OFFSET;
        const bn::fixed zone_right = fe::SWORD_ZONE_TILE_RIGHT * fe::TILE_SIZE - fe::MAP_OFFSET;
        const bn::fixed zone_top = fe::SWORD_ZONE_TILE_TOP * fe::TILE_SIZE - fe::MAP_OFFSET;
        const bn::fixed zone_bottom = fe::SWORD_ZONE_TILE_BOTTOM * fe::TILE_SIZE - fe::MAP_OFFSET;

        return position.x() >= zone_left && position.x() < zone_right &&
               position.y() >= zone_top && position.y() < zone_bottom;
    }

    bool Hitbox::is_in_merchant_collision_zone(const bn::fixed_point &position, const bn::fixed_point &merchant_center)
    {
        using namespace hitbox_constants;

        bn::fixed_point zone_position = calculate_centered_position(merchant_center, MERCHANT_COLLISION_WIDTH, MERCHANT_COLLISION_HEIGHT);

        return position.x() >= zone_position.x() && position.x() < zone_position.x() + MERCHANT_COLLISION_WIDTH &&
               position.y() >= zone_position.y() && position.y() < zone_position.y() + MERCHANT_COLLISION_HEIGHT;
    }

    bool Hitbox::is_in_merchant_interaction_zone(const bn::fixed_point &position, const bn::fixed_point &merchant_center)
    {
        using namespace hitbox_constants;

        bn::fixed_point zone_position = calculate_centered_position(merchant_center, MERCHANT_INTERACTION_WIDTH, MERCHANT_INTERACTION_HEIGHT);

        return position.x() >= zone_position.x() && position.x() < zone_position.x() + MERCHANT_INTERACTION_WIDTH &&
               position.y() >= zone_position.y() && position.y() < zone_position.y() + MERCHANT_INTERACTION_HEIGHT;
    }

    // === FACTORY METHODS ===

    Hitbox Hitbox::create_player_hitbox(bn::fixed_point position)
    {
        // Use hitbox constants from the nested namespace
        using namespace hitbox_constants;
        return Hitbox(position.x(), position.y(), PLAYER_HITBOX_WIDTH, PLAYER_HITBOX_HEIGHT, HitboxType::PLAYER);
    }

    Hitbox Hitbox::create_merchant_collision_zone(bn::fixed_point center)
    {
        // Use hitbox constants from the nested namespace
        using namespace hitbox_constants;
        bn::fixed_point position = calculate_centered_position(center, MERCHANT_COLLISION_WIDTH, MERCHANT_COLLISION_HEIGHT);
        return Hitbox(position.x(), position.y(), MERCHANT_COLLISION_WIDTH, MERCHANT_COLLISION_HEIGHT, HitboxType::MERCHANT_COLLISION);
    }

    Hitbox Hitbox::create_merchant_interaction_zone(bn::fixed_point center)
    {
        // Use hitbox constants from the nested namespace
        using namespace hitbox_constants;
        bn::fixed_point position = calculate_centered_position(center, MERCHANT_INTERACTION_WIDTH, MERCHANT_INTERACTION_HEIGHT);
        return Hitbox(position.x(), position.y(), MERCHANT_INTERACTION_WIDTH, MERCHANT_INTERACTION_HEIGHT, HitboxType::MERCHANT_INTERACTION);
    }

    Hitbox Hitbox::create_sword_zone()
    {
        // Use centralized constants directly from fe namespace
        const bn::fixed zone_left = fe::SWORD_ZONE_TILE_LEFT * fe::TILE_SIZE - fe::MAP_OFFSET;
        const bn::fixed zone_top = fe::SWORD_ZONE_TILE_TOP * fe::TILE_SIZE - fe::MAP_OFFSET;
        const bn::fixed width = (fe::SWORD_ZONE_TILE_RIGHT - fe::SWORD_ZONE_TILE_LEFT) * fe::TILE_SIZE;
        const bn::fixed height = (fe::SWORD_ZONE_TILE_BOTTOM - fe::SWORD_ZONE_TILE_TOP) * fe::TILE_SIZE;

        return Hitbox(zone_left, zone_top, width, height, HitboxType::SWORD_ZONE);
    }

    // === DEBUG VISUALIZATION ===

    Hitbox::MarkerOffsetConfig Hitbox::get_marker_config() const
    {
        // Use hitbox constants from the nested namespace
        using namespace hitbox_constants;

        switch (_type)
        {
        case HitboxType::PLAYER:
        case HitboxType::MERCHANT_COLLISION:
        case HitboxType::MERCHANT_INTERACTION:
        case HitboxType::SWORD_ZONE:
        case HitboxType::ZONE_TILES:
            return MarkerOffsetConfig(0, 0, 0, 0); // Zone-specific adjustments
        case HitboxType::STANDARD:
        default:
            return MarkerOffsetConfig(0, 0, 0, 0); // Standard adjustments
        }
    }

    bn::fixed_point Hitbox::calculate_top_left_marker_pos(bn::fixed x_offset, bn::fixed y_offset) const
    {
        constexpr bn::fixed SPRITE_CENTER_OFFSET = 2; // Half of marker sprite size (4/2)
        return bn::fixed_point(
            x() + x_offset - SPRITE_CENTER_OFFSET,
            y() + y_offset - SPRITE_CENTER_OFFSET);
    }

    bn::fixed_point Hitbox::calculate_bottom_right_marker_pos(bn::fixed x_offset, bn::fixed y_offset) const
    {
        constexpr bn::fixed SPRITE_CENTER_OFFSET = 2; // Half of marker sprite size (4/2)
        return bn::fixed_point(
            x() + width() + x_offset - SPRITE_CENTER_OFFSET,
            y() + height() + y_offset - SPRITE_CENTER_OFFSET);
    }

    bn::sprite_ptr Hitbox::create_marker(bn::fixed_point position, bool rotated) const
    {
        bn::sprite_ptr marker = bn::sprite_items::hitbox_marker.create_sprite(position);

        if (_camera.has_value())
        {
            marker.set_camera(*_camera);
        }

        marker.set_z_order(-32767); // Highest priority

        if (rotated)
        {
            marker.set_rotation_angle(180);
        }

        return marker;
    }

    void Hitbox::create_debug_markers(bn::camera_ptr camera, bool enabled)
    {
        _camera = camera;
        _debug_enabled = enabled;

        if (enabled)
        {
            update_debug_markers(true);
        }
    }

    void Hitbox::update_debug_markers(bool enabled)
    {
        _debug_enabled = enabled;

        if (!enabled)
        {
            clear_debug_markers();
            return;
        }

        MarkerOffsetConfig config = get_marker_config();

        // Handle dual-area entities (merchant zones with both collision and interaction areas)
        bool use_hitbox_markers = (_type == HitboxType::MERCHANT_COLLISION || _type == HitboxType::MERCHANT_INTERACTION);
        bool enable_blending = (_type == HitboxType::MERCHANT_INTERACTION);

        update_markers_with_config(config, use_hitbox_markers, enable_blending);
    }

    void Hitbox::update_debug_marker_positions()
    {
        if (!_debug_enabled || !_debug_markers.top_left.has_value())
        {
            return; // No markers to update
        }

        MarkerOffsetConfig config = get_marker_config();

        // Calculate main zone marker positions
        bn::fixed_point tl_pos = calculate_top_left_marker_pos(config.top_left_x, config.top_left_y);
        bn::fixed_point br_pos = calculate_bottom_right_marker_pos(config.bottom_right_x, config.bottom_right_y);

        // Calculate hitbox marker positions if they exist
        bn::optional<bn::fixed_point> hitbox_tl_pos;
        bn::optional<bn::fixed_point> hitbox_br_pos;

        if (_debug_markers.hitbox_top_left.has_value())
        {
            hitbox_tl_pos = calculate_top_left_marker_pos(0, 0);
            hitbox_br_pos = calculate_bottom_right_marker_pos(0, 0);
        }

        // Update positions efficiently without recreating sprites
        _debug_markers.update_positions(tl_pos, br_pos, hitbox_tl_pos, hitbox_br_pos);
    }

    void Hitbox::update_markers_with_config(const MarkerOffsetConfig &config, bool use_hitbox_markers, bool enable_blending)
    {
        // Clear existing markers
        _debug_markers.clear();

        // Create main zone markers
        bn::fixed_point tl_pos = calculate_top_left_marker_pos(config.top_left_x, config.top_left_y);
        bn::fixed_point br_pos = calculate_bottom_right_marker_pos(config.bottom_right_x, config.bottom_right_y);

        _debug_markers.top_left = create_marker(tl_pos, false);
        _debug_markers.bottom_right = create_marker(br_pos, true);

        // For merchant zones, add hitbox markers if needed
        if (use_hitbox_markers)
        {
            // Use different offsets for hitbox markers
            bn::fixed_point hitbox_tl_pos = calculate_top_left_marker_pos(0, 0);
            bn::fixed_point hitbox_br_pos = calculate_bottom_right_marker_pos(0, 0);

            _debug_markers.hitbox_top_left = create_marker(hitbox_tl_pos, false);
            _debug_markers.hitbox_bottom_right = create_marker(hitbox_br_pos, true);
        }

        // Apply blending for interaction zones
        if (enable_blending)
        {
            if (_debug_markers.top_left.has_value())
                _debug_markers.top_left->set_blending_enabled(true);
            if (_debug_markers.bottom_right.has_value())
                _debug_markers.bottom_right->set_blending_enabled(true);
        }

        _debug_markers.set_visible(_debug_enabled);
    }

    void Hitbox::clear_debug_markers()
    {
        _debug_markers.clear();
    }

    // === ZONE MANAGER IMPLEMENTATION ===

    // ZoneManager static member initialization for merchant zone tracking
    bn::optional<bn::fixed_point> ZoneManager::_merchant_zone_center;
    bool ZoneManager::_merchant_zone_enabled = false;

    void ZoneManager::set_merchant_zone_center(const bn::fixed_point &center)
    {
        _merchant_zone_center = center;
        _merchant_zone_enabled = true;
    }

    void ZoneManager::clear_merchant_zone()
    {
        _merchant_zone_center.reset();
        _merchant_zone_enabled = false;
    }

    void ZoneManager::set_merchant_zone_enabled(bool enabled)
    {
        _merchant_zone_enabled = enabled && _merchant_zone_center.has_value();
    }

    bn::optional<bn::fixed_point> ZoneManager::get_merchant_zone_center()
    {
        return _merchant_zone_center;
    }

    bool ZoneManager::is_merchant_zone_enabled()
    {
        return _merchant_zone_enabled && _merchant_zone_center.has_value();
    }

    bool ZoneManager::is_position_valid(const bn::fixed_point &position)
    {
        // Check sword zone collision
        if (Hitbox::is_in_sword_zone(position))
        {
            return false;
        }

        // Check merchant collision zone if enabled
        if (is_merchant_zone_enabled())
        {
            if (Hitbox::is_in_merchant_collision_zone(position, *_merchant_zone_center))
            {
                return false;
            }
        }

        return true;
    }
}
