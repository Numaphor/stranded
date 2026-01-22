#include "str_world_object.h"
#include "str_chunk_manager.h"
#include "str_constants.h"

namespace str
{
    // =========================================================================
    // WorldObject Implementation
    // =========================================================================

    WorldObject::WorldObject(const bn::fixed_point& world_pos, WorldObjectType type, bool has_collision) :
        _world_position(world_pos),
        _buffer_position(0, 0),
        _type(type),
        _has_collision(has_collision),
        _visible(true),
        _source_chunk_x(0),
        _source_chunk_y(0)
    {
    }

    void WorldObject::update(const ChunkManager& chunks)
    {
        _update_buffer_position(chunks);
        update_z_order();

        if (_sprite.has_value())
        {
            _sprite->set_position(_buffer_position);
        }
    }

    void WorldObject::set_camera(bn::camera_ptr& camera)
    {
        if (_sprite.has_value())
        {
            _sprite->set_camera(camera);
        }
    }

    void WorldObject::remove_camera()
    {
        if (_sprite.has_value())
        {
            _sprite->remove_camera();
        }
    }

    void WorldObject::update_z_order()
    {
        // Y-based depth sorting: higher Y = rendered in front
        // Use the bottom of the object for depth calculation
        int z = _world_position.y().integer() + collision_height() / 2;

        if (_sprite.has_value())
        {
            _sprite->set_z_order(z);
        }
    }

    bn::fixed WorldObject::collision_left() const
    {
        return _world_position.x() - collision_width() / 2;
    }

    bn::fixed WorldObject::collision_right() const
    {
        return _world_position.x() + collision_width() / 2;
    }

    bn::fixed WorldObject::collision_top() const
    {
        return _world_position.y() - collision_height() / 2;
    }

    bn::fixed WorldObject::collision_bottom() const
    {
        return _world_position.y() + collision_height() / 2;
    }

    bool WorldObject::collides_with_point(const bn::fixed_point& point) const
    {
        if (!_has_collision)
        {
            return false;
        }

        return point.x() >= collision_left() && point.x() < collision_right() &&
               point.y() >= collision_top() && point.y() < collision_bottom();
    }

    void WorldObject::set_visible(bool visible)
    {
        _visible = visible;
        if (_sprite.has_value())
        {
            _sprite->set_visible(visible);
        }
    }

    void WorldObject::set_source_chunk(int chunk_x, int chunk_y)
    {
        _source_chunk_x = chunk_x;
        _source_chunk_y = chunk_y;
    }

    void WorldObject::_update_buffer_position(const ChunkManager& chunks)
    {
        _buffer_position = chunks.world_to_buffer(_world_position);
    }

    // =========================================================================
    // CompositeWorldObject Implementation
    // =========================================================================

    CompositeWorldObject::CompositeWorldObject(const bn::fixed_point& world_pos, WorldObjectType type,
                                               bool has_collision, int grid_width, int grid_height) :
        WorldObject(world_pos, type, has_collision),
        _grid_width(grid_width),
        _grid_height(grid_height),
        _sprite_size(64)  // Standard sprite size
    {
    }

    void CompositeWorldObject::update(const ChunkManager& chunks)
    {
        _update_buffer_position(chunks);

        // Calculate z-order based on bottom of object
        int base_z = _world_position.y().integer() + collision_height() / 2;

        // Update each sprite in the composite
        for (auto& slot : _sprites)
        {
            if (slot.sprite.has_value())
            {
                // Calculate this sprite's position relative to center
                int offset_x = (slot.grid_x - _grid_width / 2) * _sprite_size + _sprite_size / 2;
                int offset_y = (slot.grid_y - _grid_height / 2) * _sprite_size + _sprite_size / 2;

                bn::fixed_point sprite_pos(
                    _buffer_position.x() + offset_x,
                    _buffer_position.y() + offset_y
                );

                slot.sprite->set_position(sprite_pos);
                slot.sprite->set_z_order(base_z);
            }
        }
    }

    void CompositeWorldObject::set_camera(bn::camera_ptr& camera)
    {
        for (auto& slot : _sprites)
        {
            if (slot.sprite.has_value())
            {
                slot.sprite->set_camera(camera);
            }
        }
    }

    void CompositeWorldObject::remove_camera()
    {
        for (auto& slot : _sprites)
        {
            if (slot.sprite.has_value())
            {
                slot.sprite->remove_camera();
            }
        }
    }

    int CompositeWorldObject::collision_width() const
    {
        return _grid_width * _sprite_size;
    }

    int CompositeWorldObject::collision_height() const
    {
        return _grid_height * _sprite_size;
    }

    void CompositeWorldObject::add_sprite(bn::sprite_ptr sprite, int grid_x, int grid_y)
    {
        if (!_sprites.full())
        {
            SpriteSlot slot;
            slot.sprite = sprite;
            slot.grid_x = grid_x;
            slot.grid_y = grid_y;
            _sprites.push_back(slot);
        }
    }

    // =========================================================================
    // Factory Functions
    // =========================================================================

    // Note: This needs sprite assets to be created for the sword
    // The sword.bmp needs to be split into 64x64 sprite sheets
    CompositeWorldObject* create_sword_object(const bn::fixed_point& world_pos)
    {
        // The sword is 256x256 = 4x4 grid of 64x64 sprites
        CompositeWorldObject* sword = new CompositeWorldObject(
            world_pos,
            WorldObjectType::SWORD,
            true,   // has collision
            4,      // 4 sprites wide
            4       // 4 sprites tall
        );

        // TODO: Add sprites when sword sprite assets are created
        // For now, the sword object exists but has no visuals
        // After creating sword sprite assets:
        // sword->add_sprite(bn::sprite_items::sword_0_0.create_sprite(0, 0), 0, 0);
        // sword->add_sprite(bn::sprite_items::sword_1_0.create_sprite(0, 0), 1, 0);
        // ... etc for all 16 sprites

        return sword;
    }
}
