#include "str_level.h"
#include "str_constants.h"
#include "str_chunk_manager.h"
#include "str_world_object.h"

#include "bn_affine_bg_map_ptr.h"
#include "bn_affine_bg_map_cell.h"
#include "bn_affine_bg_map_cell_info.h"
#include "bn_span.h"
#include "bn_fixed_point.h"

namespace str
{

    // =========================================================================
    // Level Implementation
    // =========================================================================

    Level::Level() :
        _chunk_manager(nullptr)
    {
        _zone_tiles.clear();
        _zone_tiles.push_back(COLLISION_ZONE_TILE_INDEX);
        _zone_tiles.push_back(INTERACTION_ZONE_TILE_INDEX);
    }

    Level::Level(bn::affine_bg_map_ptr bg) :
        _chunk_manager(nullptr)
    {
        _bg_map_ptr = bg;
        _floor_tiles = {};
        _zone_tiles.clear();
        _zone_tiles.push_back(COLLISION_ZONE_TILE_INDEX);
        _zone_tiles.push_back(INTERACTION_ZONE_TILE_INDEX);
        bn::span<const bn::affine_bg_map_cell> cells = bg.cells_ref().value();
        for (int i = 0; i < 32 && i < cells.size(); ++i)
        {
            if (cells.at(i) != 0)
            {
                _floor_tiles.push_back(cells.at(i));
            }
        }
    }

    void Level::set_chunk_manager(const ChunkManager* chunk_manager)
    {
        _chunk_manager = chunk_manager;
    }

    void Level::add_world_object(WorldObject* obj)
    {
        if (obj && !_world_objects.full())
        {
            _world_objects.push_back(obj);
        }
    }

    void Level::remove_world_object(WorldObject* obj)
    {
        for (int i = _world_objects.size() - 1; i >= 0; --i)
        {
            if (_world_objects[i] == obj)
            {
                _world_objects.erase(_world_objects.begin() + i);
                return;
            }
        }
    }

    void Level::clear_world_objects()
    {
        _world_objects.clear();
    }

    bn::vector<int, 32> Level::floor_tiles() { return _floor_tiles; }

    void Level::add_zone_tile(int tile_index)
    {
        if (_zone_tiles.size() < _zone_tiles.max_size())
        {
            _zone_tiles.push_back(tile_index);
        }
    }

    void Level::reset()
    {
        _zone_tiles.clear();
        _zone_tiles.push_back(4);
        _zone_tiles.push_back(4);
        _floor_tiles.clear();
        if (_bg_map_ptr.has_value())
        {
            bn::span<const bn::affine_bg_map_cell> cells = _bg_map_ptr->cells_ref().value();
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
        constexpr int tile_size = TILE_SIZE;
        constexpr int map_offset = MAP_OFFSET;
        const bn::fixed zone_left = SWORD_ZONE_TILE_LEFT * tile_size - map_offset;
        const bn::fixed zone_right = SWORD_ZONE_TILE_RIGHT * tile_size - map_offset;
        const bn::fixed zone_top = SWORD_ZONE_TILE_TOP * tile_size - map_offset;
        const bn::fixed zone_bottom = SWORD_ZONE_TILE_BOTTOM * tile_size - map_offset;
        return position.x() >= zone_left && position.x() < zone_right &&
               position.y() >= zone_top && position.y() < zone_bottom;
    }

    bool Level::is_in_merchant_interaction_zone(const bn::fixed_point &position) const
    {
        if (!_merchant_zone_center.has_value() || !_merchant_zone_enabled)
        {
            return false;
        }
        const bn::fixed_point &center = _merchant_zone_center.value();
        const bn::fixed zone_left = center.x() - MERCHANT_INTERACTION_ZONE_WIDTH / 2;
        const bn::fixed zone_right = center.x() + MERCHANT_INTERACTION_ZONE_WIDTH / 2;
        const bn::fixed zone_top = center.y() - MERCHANT_INTERACTION_ZONE_HEIGHT / 2;
        const bn::fixed zone_bottom = center.y() + MERCHANT_INTERACTION_ZONE_HEIGHT / 2;
        return position.x() >= zone_left && position.x() < zone_right &&
               position.y() >= zone_top && position.y() < zone_bottom;
    }

    bool Level::is_in_merchant_collision_zone(const bn::fixed_point &position) const
    {
        if (!_merchant_zone_center.has_value() || !_merchant_zone_enabled)
        {
            return false;
        }
        const bn::fixed_point &center = _merchant_zone_center.value();
        const bn::fixed zone_left = center.x() - MERCHANT_COLLISION_ZONE_WIDTH / 2;
        const bn::fixed zone_right = center.x() + MERCHANT_COLLISION_ZONE_WIDTH / 2;
        const bn::fixed zone_top = center.y() - MERCHANT_COLLISION_ZONE_HEIGHT / 2;
        const bn::fixed zone_bottom = center.y() + MERCHANT_COLLISION_ZONE_HEIGHT / 2;
        return position.x() >= zone_left && position.x() < zone_right &&
               position.y() >= zone_top && position.y() < zone_bottom;
    }

    void Level::set_merchant_zone(const bn::fixed_point &center) { _merchant_zone_center = center; }
    void Level::clear_merchant_zone() { _merchant_zone_center.reset(); }
    void Level::set_merchant_zone_enabled(bool enabled) { _merchant_zone_enabled = enabled; }

    bool Level::_collides_with_world_objects([[maybe_unused]] const bn::fixed_point &position) const
    {
        // TODO: Enable when world objects are properly initialized
        // For now, always return false to avoid potential issues
        // NOTE: Once enabled, this function should have test coverage to ensure
        // world object collision detection works correctly with the chunk system
        return false;

        /*
        // Skip if no world objects registered
        if (_world_objects.empty())
        {
            return false;
        }

        // Check collision with world objects (trees, sword, buildings)
        for (int i = 0; i < _world_objects.size(); ++i)
        {
            const WorldObject* obj = _world_objects[i];
            if (obj && obj->has_collision() && obj->collides_with_point(position))
            {
                return true;
            }
        }
        return false;
        */
    }

    bool Level::_is_position_valid_chunked(const bn::fixed_point &p) const
    {
        if (!_chunk_manager)
            return true;

        // Check merchant collision
        if (is_in_merchant_collision_zone(p))
            return false;

        // Check world object collision
        if (_collides_with_world_objects(p))
            return false;

        // Check collision points against world tiles
        bn::fixed_point pts[] = {
            {p.x() - PLAYER_HITBOX_REDUCED_WIDTH / 2, p.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET},
            {p.x() + PLAYER_HITBOX_REDUCED_WIDTH / 2 - 1, p.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET},
            {p.x() - PLAYER_HITBOX_REDUCED_WIDTH / 2, p.y() + PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET - 1},
            {p.x() + PLAYER_HITBOX_REDUCED_WIDTH / 2 - 1, p.y() + PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET - 1},
            {p.x(), p.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET},
            {p.x() - PLAYER_HITBOX_WIDTH / 4, p.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET},
            {p.x() + PLAYER_HITBOX_WIDTH / 4, p.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET}
        };

        for (auto &pt : pts)
        {
            // Convert camera-relative point to world coordinates before sampling tiles
            bn::fixed_point world_pt = _chunk_manager->buffer_to_world(pt);

            // Convert world position to tile coordinates
            int tile_x = world_pt.x().integer() / TILE_SIZE;
            int tile_y = world_pt.y().integer() / TILE_SIZE;

            // Check bounds
            if (tile_x < 0 || tile_x >= WORLD_WIDTH_TILES ||
                tile_y < 0 || tile_y >= WORLD_HEIGHT_TILES)
            {
                return false;
            }

            // Get tile from chunk manager
            int tidx = _chunk_manager->get_tile_at_world(tile_x, tile_y);

            // Check against zone tiles
            for (int z : _zone_tiles)
            {
                if (tidx == z && z != 3 && z != 4)
                    return false;
            }
        }
        return true;
    }

    bool Level::is_position_valid(const bn::fixed_point &p) const
    {
        // Use chunked collision if chunk manager is available
        if (_chunk_manager)
        {
            return _is_position_valid_chunked(p);
        }

        // Fall back to original behavior for non-chunked maps
        if (!_bg_map_ptr || is_in_merchant_collision_zone(p))
            return 0;

        // Check world object collision
        if (_collides_with_world_objects(p))
            return false;

        auto c = _bg_map_ptr->cells_ref().value();
        int w = _bg_map_ptr->dimensions().width(), h = _bg_map_ptr->dimensions().height();
        bn::fixed_point pts[] = {
            {p.x() - PLAYER_HITBOX_REDUCED_WIDTH / 2, p.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET},
            {p.x() + PLAYER_HITBOX_REDUCED_WIDTH / 2 - 1, p.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET},
            {p.x() - PLAYER_HITBOX_REDUCED_WIDTH / 2, p.y() + PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET - 1},
            {p.x() + PLAYER_HITBOX_REDUCED_WIDTH / 2 - 1, p.y() + PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET - 1},
            {p.x(), p.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET},
            {p.x() - PLAYER_HITBOX_WIDTH / 4, p.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET},
            {p.x() + PLAYER_HITBOX_WIDTH / 4, p.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET}
        };
        for (auto &pt : pts)
        {
            int cx = ((pt.x() + w * 4) / 8).integer(), cy = ((pt.y() + h * 4) / 8).integer();
            if (cx < 0 || cx >= w || cy < 0 || cy >= h)
                return 0;
            int idx = cy * w + cx;
            if (idx < 0 || idx >= c.size())
                return 0;
            int tidx = bn::affine_bg_map_cell_info(c.at(idx)).tile_index();
            for (int z : _zone_tiles)
                if (tidx == z && z != 3 && z != 4)
                    return 0;
        }
        return 1;
    }

} // namespace str
