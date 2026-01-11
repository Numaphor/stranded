#ifndef FE_HITBOX_DEBUG_H
#define FE_HITBOX_DEBUG_H

#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_map_cell.h"
#include "bn_camera_ptr.h"
#include "bn_fixed_point.h"
#include "bn_vector.h"
#include "fe_hitbox.h"
#include "fe_player.h"
#include "fe_enemy.h"
#include "fe_npc.h"

namespace fe
{
    class HitboxDebug
    {
    public:
        HitboxDebug(bn::regular_bg_map_ptr &bg_map, bn::regular_bg_map_cell *cells, int background_tile);
        ~HitboxDebug();

        // Toggle debug visualization on/off
        void toggle();
        
        // Check if debug mode is currently active
        [[nodiscard]] bool is_active() const { return _active; }

        // Update debug visualization
        void update(
            const Player *player,
            const bn::vector<Enemy, 16> &enemies,
            const NPC *merchant,
            const bn::camera_ptr &camera
        );

    private:
        bn::regular_bg_map_ptr &_bg_map;
        bn::regular_bg_map_cell *_cells;
        int _background_tile;
        bool _active;
        bn::vector<int, 512> _modified_cells; // Track modified cell indices for efficient clearing

        // Draw a rectangular hitbox on the background map
        void _draw_hitbox(const Hitbox &hitbox, int tile_id);
        
        // Draw a point marker on the background map
        void _draw_point(const bn::fixed_point &point, int tile_id);
        
        // Clear all debug visualization
        void _clear_debug_tiles();
        
        // Helper to convert world position to tile coordinates
        static void _world_to_tile(const bn::fixed_point &world_pos, int &tile_x, int &tile_y);
    };
}

#endif
