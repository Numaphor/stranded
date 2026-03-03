#pragma once

#include "bn_fixed_point.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"
#include "bn_fixed.h"
#include "bn_optional.h"
#include "str_constants.h"

namespace str {
    class Enemy;  // Forward declaration

    // Room state for minimap display
    enum class RoomState : int
    {
        UNVISITED = 0,  // Hollow outline only
        VISITED = 1,    // Dithered fill with beveled border
        CURRENT = 2     // Bright cyan with beveled highlight/shadow
    };

    class Minimap {
    private:
        // Background panel sprite
        bn::sprite_ptr _bg_panel;

        // Room tile sprites (one per room, 6 total)
        bn::sprite_ptr _room_sprites[MINIMAP_NUM_ROOMS];

        // Door connector sprites (between adjacent rooms)
        bn::vector<bn::sprite_ptr, 8> _door_sprites;

        // Player arrow sprite
        bn::sprite_ptr _player_arrow;

        // Enemy dot sprites (dynamic)
        struct EnemyDot {
            bn::sprite_ptr sprite;
            const Enemy* enemy;

            EnemyDot() = default;
            EnemyDot(const bn::sprite_ptr& s, const Enemy* e) : sprite(s), enemy(e) {}

            EnemyDot(EnemyDot&& other) noexcept = default;
            EnemyDot& operator=(EnemyDot&& other) noexcept = default;

            EnemyDot(const EnemyDot&) = delete;
            EnemyDot& operator=(const EnemyDot&) = delete;
        };
        bn::vector<EnemyDot, 16> _enemy_dots;

        // Room state tracking
        RoomState _room_states[MINIMAP_NUM_ROOMS];
        int _current_room;

        // Panel center position (screen coords)
        bn::fixed_point _panel_center;

        // Pulse animation counter for current room highlight
        int _pulse_counter;

        // Track last-applied graphics frame per room to avoid redundant set_tiles
        // (each set_tiles allocates VRAM; VRAM is extremely tight in 3D scenes)
        int _last_applied_frame[MINIMAP_NUM_ROOMS];

        // World-space room layout data (centers and half-extents)
        static constexpr bn::fixed _room_center_x[MINIMAP_NUM_ROOMS] = {
            bn::fixed(-60), bn::fixed(75), bn::fixed(-60),
            bn::fixed(60), bn::fixed(-60), bn::fixed(75)
        };
        static constexpr bn::fixed _room_center_y[MINIMAP_NUM_ROOMS] = {
            bn::fixed(-120), bn::fixed(-135), bn::fixed(0),
            bn::fixed(0), bn::fixed(120), bn::fixed(135)
        };
        static constexpr bn::fixed _room_half_x[MINIMAP_NUM_ROOMS] = {
            bn::fixed(60), bn::fixed(75), bn::fixed(60),
            bn::fixed(60), bn::fixed(60), bn::fixed(75)
        };
        static constexpr bn::fixed _room_half_y[MINIMAP_NUM_ROOMS] = {
            bn::fixed(60), bn::fixed(75), bn::fixed(60),
            bn::fixed(60), bn::fixed(60), bn::fixed(75)
        };

        // Door adjacency: pairs of (room_a, room_b) that are connected
        static constexpr int DOOR_COUNT = 7;
        static constexpr int _door_pairs[DOOR_COUNT][2] = {
            {0, 1}, {0, 2}, {1, 3}, {2, 3}, {2, 4}, {3, 5}, {4, 5}
        };

        // Which rooms are "big" (true = full 16x16 sprite, false = smaller 10x10 within 16x16)
        static constexpr bool _is_big_room[MINIMAP_NUM_ROOMS] = {
            false, true, false, false, false, true  // rooms 1 and 5 have bigger half-extents
        };

        // Helper: configure a sprite for HUD overlay
        void _configure_hud_sprite(bn::sprite_ptr& sprite, int z_order);

        // Helper: get minimap screen position for a room index
        bn::fixed_point _room_screen_pos(int room_id) const;

        // Helper: determine which room a world position is in (-1 if none)
        int _find_room(bn::fixed_point world_pos) const;

        // Room-viewer variant: keeps current room while inside overlap bounds to avoid jitter.
        int _find_room_room_viewer(bn::fixed_point world_pos) const;

        // Helper: map world position to minimap screen position within a room
        bn::fixed_point _world_to_minimap(bn::fixed_point world_pos, int room_id) const;

        // Room-viewer variant: maps door thresholds closer to connector sprites.
        bn::fixed_point _world_to_minimap_room_viewer(bn::fixed_point world_pos, int room_id) const;

        // Update room sprite frames based on current state
        void _update_room_visuals();

        // Create door connector sprites between adjacent rooms
        void _create_door_connectors();

        // Update pulse animation on current room
        void _update_pulse();

        // Reposition all room/door sprites based on scroll offset
        void _scroll_sprites(bn::fixed_point scroll_offset);

    public:
        Minimap();
        ~Minimap() = default;

        // Main update: call each frame with player world pos, facing direction, enemies
        void update(bn::fixed_point player_pos, int facing_direction, const bn::vector<Enemy, 16>& enemies);

        // Simplified update without enemies (for room viewer scene)
        void update(bn::fixed_point player_pos, int facing_direction);

        void set_visible(bool visible);
    };
}
