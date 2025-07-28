#include "fe_level.h"

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
        // Initialize _zone_tiles and add default zone tiles (4 and 4)
        _zone_tiles.clear();
        _zone_tiles.push_back(4); // Tile index 4 is used for collision squares
        _zone_tiles.push_back(4); // Keep existing zone tile

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
        // Define sword zone tile coordinates and map offset
        constexpr int sword_zone_tile_left = 147;
        constexpr int sword_zone_tile_right = 157; // exclusive upper bound
        constexpr int sword_zone_tile_top = 162;
        constexpr int sword_zone_tile_bottom = 166; // exclusive upper bound
        constexpr int tile_size = 8;
        constexpr int map_offset = 1280;

        // Calculate world coordinates from tile coordinates
        const bn::fixed zone_left = sword_zone_tile_left * tile_size - map_offset;
        const bn::fixed zone_right = sword_zone_tile_right * tile_size - map_offset;
        const bn::fixed zone_top = sword_zone_tile_top * tile_size - map_offset;
        const bn::fixed zone_bottom = sword_zone_tile_bottom * tile_size - map_offset;

        return position.x() >= zone_left && position.x() < zone_right &&
               position.y() >= zone_top && position.y() < zone_bottom;
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
        const int hitbox_width = 32;
        const int hitbox_height = 16;
        const int vertical_offset = 8; // Offset from top of sprite to start of hitbox

        // Reduced width for left/right collision checks to allow getting closer
        const int reduced_width = 16; // Reduced from 32 to allow closer approach from sides

        // Check multiple points around the hitbox - with reduced width for left/right
        // Top-left corner
        bn::fixed_point top_left(position.x() - reduced_width / 2, position.y() - hitbox_height / 2 + vertical_offset);
        // Top-right corner
        bn::fixed_point top_right(position.x() + reduced_width / 2 - 1, position.y() - hitbox_height / 2 + vertical_offset);
        // Bottom-left corner
        bn::fixed_point bottom_left(position.x() - reduced_width / 2, position.y() + hitbox_height / 2 + vertical_offset - 1);
        // Bottom-right corner
        bn::fixed_point bottom_right(position.x() + reduced_width / 2 - 1, position.y() + hitbox_height / 2 + vertical_offset - 1);

        // Additional check for the middle-top point to prevent walking into zones from the north
        bn::fixed_point middle_top(position.x(), position.y() - hitbox_height / 2 + vertical_offset);

        // Additional points along the top edge for more accurate collision
        bn::fixed_point quarter_top_left(position.x() - hitbox_width / 4, position.y() - hitbox_height / 2 + vertical_offset);
        bn::fixed_point quarter_top_right(position.x() + hitbox_width / 4, position.y() - hitbox_height / 2 + vertical_offset);

        // Check each point for collision - optimized to reduce redundant calculations
        bn::fixed_point check_points[] = {
            top_left, top_right, bottom_left, bottom_right,
            middle_top, quarter_top_left, quarter_top_right};

        // First check for hardcoded sword zone collision (independent of visual tiles)
        for (const auto &point : check_points)
        {
            if (is_in_sword_zone(point))
            {
                return false; // Collision with sword zone
            }
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

            // Only check for zone tiles other than 4 (since sword zone is handled separately)
            for (int zone_tile : _zone_tiles)
            {
                if (tile_index == zone_tile && zone_tile != 4)
                {
                    // It's a non-sword zone tile, so we can't move here
                    return false;
                }
            }
        }

        // Position is valid if we made it this far
        return true;
    }
}
