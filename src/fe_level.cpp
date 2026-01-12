#include "fe_level.h"
#include "fe_constants.h"

#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_optional.h"
#include "bn_span.h"
#include "bn_log.h"
#include "bn_string.h"
#include "bn_regular_bg_map_cell_info.h"

namespace fe
{
    Level::Level(bn::regular_bg_map_ptr bg)
    {
        _bg_map_ptr = bg;
        _floor_tiles = {};
        // Initialize _zone_tiles and add default zone tiles using centralized constants
        _zone_tiles.clear();
        _zone_tiles.push_back(COLLISION_ZONE_TILE_INDEX);   // Tile index for all hitbox zones (sword zone, merchant collision)
        _zone_tiles.push_back(INTERACTION_ZONE_TILE_INDEX); // Tile index for all interaction zones (merchant interaction)

        bn::span<const bn::regular_bg_map_cell> cells = bg.cells_ref().value();

        for (int i = 0; i < 32 && i < cells.size(); ++i)
        {
            if (cells.at(i) != 0)
            {
                _floor_tiles.push_back(cells.at(i));
            }
        }
    }

    bn::vector<int, 32> Level::floor_tiles()
    {
        return _floor_tiles;
    }

    void Level::add_zone_tile(int tile_index)
    {
        if (_zone_tiles.size() < _zone_tiles.max_size())
        {
            _zone_tiles.push_back(tile_index);
        }
    }

    void Level::reset()
    {
        // Clear existing zone tiles
        _zone_tiles.clear();

        // Reinitialize with default zone tiles (4 and 4)
        _zone_tiles.push_back(4); // Tile index 4 is used for collision squares
        _zone_tiles.push_back(4); // Keep existing zone tile

        // Clear and repopulate floor tiles if we have a background map
        _floor_tiles.clear();
        if (_bg_map_ptr.has_value())
        {
            bn::span<const bn::regular_bg_map_cell> cells = _bg_map_ptr->cells_ref().value();
            for (int i = 0; i < 32 && i < cells.size(); ++i)
            {
                if (cells.at(i) != 0)
                {
                    _floor_tiles.push_back(cells.at(i));
                }
            }
        }
    }

    bool Level::is_in_sword_zone(const bn::fixed_point &position) const
    {
        // Use centralized constants
        constexpr int tile_size = TILE_SIZE;
        constexpr int map_offset = MAP_OFFSET;

        // Calculate world coordinates from tile coordinates
        const bn::fixed zone_left = SWORD_ZONE_TILE_LEFT * tile_size - map_offset;
        const bn::fixed zone_right = SWORD_ZONE_TILE_RIGHT * tile_size - map_offset;
        const bn::fixed zone_top = SWORD_ZONE_TILE_TOP * tile_size - map_offset;
        const bn::fixed zone_bottom = SWORD_ZONE_TILE_BOTTOM * tile_size - map_offset;

        return position.x() >= zone_left && position.x() < zone_right &&
               position.y() >= zone_top && position.y() < zone_bottom;
    }

    bool Level::is_in_merchant_interaction_zone(const bn::fixed_point &position) const
    {
        // Return false if no merchant zone is set or zone is disabled (during conversations)
        if (!_merchant_zone_center.has_value() || !_merchant_zone_enabled)
        {
            return false;
        }

        const bn::fixed_point &center = _merchant_zone_center.value();

        // Calculate interaction zone boundaries (larger zone for conversation triggers)
        // Using same pattern as other zones: inclusive left/top, exclusive right/bottom
        const bn::fixed zone_left = center.x() - MERCHANT_INTERACTION_ZONE_WIDTH / 2;
        const bn::fixed zone_right = center.x() + MERCHANT_INTERACTION_ZONE_WIDTH / 2;
        const bn::fixed zone_top = center.y() - MERCHANT_INTERACTION_ZONE_HEIGHT / 2;
        const bn::fixed zone_bottom = center.y() + MERCHANT_INTERACTION_ZONE_HEIGHT / 2;

        return position.x() >= zone_left && position.x() < zone_right &&
               position.y() >= zone_top && position.y() < zone_bottom;
    }

    bool Level::is_in_merchant_collision_zone(const bn::fixed_point &position) const
    {
        // Return false if no merchant zone is set or zone is disabled (during conversations)
        if (!_merchant_zone_center.has_value() || !_merchant_zone_enabled)
        {
            return false;
        }

        const bn::fixed_point &center = _merchant_zone_center.value();

        // Calculate collision zone boundaries (smaller zone for physical blocking)
        // Using same pattern as other zones: inclusive left/top, exclusive right/bottom
        const bn::fixed zone_left = center.x() - MERCHANT_COLLISION_ZONE_WIDTH / 2;
        const bn::fixed zone_right = center.x() + MERCHANT_COLLISION_ZONE_WIDTH / 2;
        const bn::fixed zone_top = center.y() - MERCHANT_COLLISION_ZONE_HEIGHT / 2;
        const bn::fixed zone_bottom = center.y() + MERCHANT_COLLISION_ZONE_HEIGHT / 2;

        return position.x() >= zone_left && position.x() < zone_right &&
               position.y() >= zone_top && position.y() < zone_bottom;
    }

    void Level::set_merchant_zone(const bn::fixed_point &center)
    {
        _merchant_zone_center = center;
    }

    void Level::clear_merchant_zone()
    {
        _merchant_zone_center.reset();
    }

    void Level::set_merchant_zone_enabled(bool enabled)
    {
        _merchant_zone_enabled = enabled;
    }

    bool Level::is_position_valid(const bn::fixed_point &position) const
    {
        // If we don't have a background map, any position is valid
        if (!_bg_map_ptr.has_value())
        {
            return true;
        }

        // Get map dimensions from the cells span
        bn::span<const bn::regular_bg_map_cell> cells = _bg_map_ptr.value().cells_ref().value();
        int map_width = _bg_map_ptr.value().dimensions().width();
        int map_height = _bg_map_ptr.value().dimensions().height();

        // Define hitbox for collision - middle 32x16 pixels of 32x32 sprite
        // Starting 8 pixels from the top (since we're using middle portion)

        // Reduced width for left/right collision checks to allow getting closer

        // Check multiple points around the hitbox - with reduced width for left/right
        // Top-left corner
        bn::fixed_point top_left(position.x() - PLAYER_HITBOX_REDUCED_WIDTH / 2, position.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET);
        // Top-right corner
        bn::fixed_point top_right(position.x() + PLAYER_HITBOX_REDUCED_WIDTH / 2 - 1, position.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET);
        // Bottom-left corner
        bn::fixed_point bottom_left(position.x() - PLAYER_HITBOX_REDUCED_WIDTH / 2, position.y() + PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET - 1);
        // Bottom-right corner
        bn::fixed_point bottom_right(position.x() + PLAYER_HITBOX_REDUCED_WIDTH / 2 - 1, position.y() + PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET - 1);

        // Additional check for the middle-top point to prevent walking into zones from the north
        bn::fixed_point middle_top(position.x(), position.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET);

        // Additional points along the top edge for more accurate collision
        bn::fixed_point quarter_top_left(position.x() - PLAYER_HITBOX_WIDTH / 4, position.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET);
        bn::fixed_point quarter_top_right(position.x() + PLAYER_HITBOX_WIDTH / 4, position.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET);

        // Check each point for collision - optimized to reduce redundant calculations
        bn::fixed_point check_points[] = {
            top_left, top_right, bottom_left, bottom_right,
            middle_top, quarter_top_left, quarter_top_right};

        // All collision barriers removed - players can move freely through sword zones
        // Sword zone collision checking has been disabled

        // Improved merchant collision system - check for collision zone (separate from interaction zone)
        // Only block if merchant zone is enabled and player is in collision zone
        if (is_in_merchant_collision_zone(position))
        {
            return false; // Block movement - player collides with merchant
        }

        // Then check for other tile-based collisions (if any other zone tiles exist)
        const int map_offset_x = (map_width * 4);
        const int map_offset_y = (map_height * 4);

        for (const auto &point : check_points)
        {
            // Convert position to cell coordinates
            // Assuming each cell is 8x8 pixels and the map is centered on the screen
            int cell_x = ((point.x() + map_offset_x) / 8).integer();
            int cell_y = ((point.y() + map_offset_y) / 8).integer();

            // Check if the cell position is within map bounds
            if (cell_x < 0 || cell_x >= map_width || cell_y < 0 || cell_y >= map_height)
            {
                return false;
            }

            // Get the tile at the specified position
            int cell_index = cell_y * map_width + cell_x;

            // Check if the index is in range
            if (cell_index < 0 || cell_index >= cells.size())
            {
                return false;
            }

            bn::regular_bg_map_cell cell = cells.at(cell_index);
            int tile_index = bn::regular_bg_map_cell_info(cell).tile_index();

            // Only check for zone tiles other than 3 and 4 (since merchant zones are handled separately by is_in_hitbox_zone)
            for (int zone_tile : _zone_tiles)
            {
                if (tile_index == zone_tile && zone_tile != 3 && zone_tile != 4)
                {
                    // It's a non-merchant zone tile, so we can't move here
                    return false;
                }
            }
        }

        // Position is valid if we made it this far
        return true;
    }
}
