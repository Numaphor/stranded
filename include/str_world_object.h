#ifndef STR_WORLD_OBJECT_H
#define STR_WORLD_OBJECT_H

#include "bn_fixed_point.h"
#include "bn_sprite_ptr.h"
#include "bn_optional.h"
#include "bn_vector.h"
#include "bn_camera_ptr.h"
#include "str_world_map_data.h"

namespace str
{
    class ChunkManager;

    // Base class for world objects (trees, buildings, sword, etc.)
    class WorldObject
    {
    public:
        WorldObject(const bn::fixed_point& world_pos, WorldObjectType type, bool has_collision);
        virtual ~WorldObject() = default;

        // Update sprite position based on buffer coordinates
        virtual void update(const ChunkManager& chunks);

        // Set camera for sprite
        virtual void set_camera(bn::camera_ptr& camera);
        virtual void remove_camera();

        // Update z-order based on Y position (for depth sorting)
        void update_z_order();

        // Getters
        [[nodiscard]] const bn::fixed_point& world_position() const { return _world_position; }
        [[nodiscard]] WorldObjectType type() const { return _type; }
        [[nodiscard]] bool has_collision() const { return _has_collision; }
        [[nodiscard]] bool is_visible() const { return _visible; }

        // Collision hitbox (in world coordinates)
        [[nodiscard]] virtual bn::fixed collision_left() const;
        [[nodiscard]] virtual bn::fixed collision_right() const;
        [[nodiscard]] virtual bn::fixed collision_top() const;
        [[nodiscard]] virtual bn::fixed collision_bottom() const;
        [[nodiscard]] virtual int collision_width() const { return 32; }
        [[nodiscard]] virtual int collision_height() const { return 16; }

        // Check if a point collides with this object
        [[nodiscard]] bool collides_with_point(const bn::fixed_point& point) const;

        // Set visibility
        void set_visible(bool visible);

        // Chunk ownership
        int source_chunk_x() const { return _source_chunk_x; }
        int source_chunk_y() const { return _source_chunk_y; }
        void set_source_chunk(int chunk_x, int chunk_y);

    protected:
        bn::fixed_point _world_position;
        bn::fixed_point _buffer_position;
        WorldObjectType _type;
        bool _has_collision;
        bool _visible;
        int _source_chunk_x;
        int _source_chunk_y;

        // Single sprite for simple objects (trees, rocks)
        bn::optional<bn::sprite_ptr> _sprite;

        // Convert world position to buffer position
        void _update_buffer_position(const ChunkManager& chunks);
    };

    // Large composite object made of multiple sprites (for sword, large buildings)
    class CompositeWorldObject : public WorldObject
    {
    public:
        // grid_width and grid_height are in 64x64 sprite units
        CompositeWorldObject(const bn::fixed_point& world_pos, WorldObjectType type,
                            bool has_collision, int grid_width, int grid_height);

        void update(const ChunkManager& chunks) override;
        void set_camera(bn::camera_ptr& camera) override;
        void remove_camera() override;

        [[nodiscard]] int collision_width() const override;
        [[nodiscard]] int collision_height() const override;

        // Add a sprite to the composite
        void add_sprite(bn::sprite_ptr sprite, int grid_x, int grid_y);

    private:
        struct SpriteSlot
        {
            bn::optional<bn::sprite_ptr> sprite;
            int grid_x;  // Position in grid (0 to grid_width-1)
            int grid_y;  // Position in grid (0 to grid_height-1)
        };

        bn::vector<SpriteSlot, 16> _sprites;  // Max 4x4 = 16 sprites
        int _grid_width;   // Number of sprites horizontally
        int _grid_height;  // Number of sprites vertically
        int _sprite_size;  // Size of each sprite (typically 64)
    };

    // Factory function to create the sword object
    CompositeWorldObject* create_sword_object(const bn::fixed_point& world_pos);
}

#endif
