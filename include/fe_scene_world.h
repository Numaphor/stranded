#ifndef FE_SCENE_WORLD_H
#define FE_SCENE_WORLD_H

#include "bn_fixed_point.h"
#include "bn_sprite_ptr.h"
#include "bn_camera_ptr.h"
#include "bn_optional.h"
#include "bn_vector.h"
#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_tiles_ptr.h"
#include "bn_bg_palette_ptr.h"
#include "bn_regular_bg_items_sword.h"
#include "bn_rect_window.h"

#include "fe_scene.h"
#include "fe_player.h"
#include "fe_player_status_display.h"
#include "fe_enemy.h"
#include "fe_level.h"
#include "fe_minimap.h"
#include "fe_npc_derived.h" // Include NPC derived classes
#include "fe_hitbox.h"
#include "fe_world_state.h"

namespace fe
{
    class World
    {
    public:
        World();
        ~World();

        fe::Scene execute(bn::fixed_point spawn_location, int world_id = 0);

    private:
        Player *_player;
        Level *_level;
        bn::vector<Enemy, 16> _enemies;
        Minimap *_minimap;
        bn::optional<bn::regular_bg_ptr> _sword_bg;
        NPC *_merchant;                                             // Changed to base NPC pointer to allow different types
        bn::unique_ptr<PlayerStatusDisplay> _player_status_display; // Player status display
        bool _debug_enabled;                                        // Track debug state for centralized hitbox system
        bn::optional<bn::camera_ptr> _camera;                       // Camera for debug marker positioning
        fe::Hitbox _player_debug_hitbox;                            // Player debug hitbox
        bn::vector<fe::Hitbox, 16> _enemy_debug_hitboxes;           // Enemy debug hitboxes
        PlayerMovement::Direction _last_camera_direction;           // Track last direction for smooth direction changes
        int _direction_change_frames;                               // Counter for how many frames we've been changing direction
        int _current_world_id;                                      // Track current world

        // Camera deadzone system
        bn::fixed_point _camera_target_pos;                               // Where the camera wants to be
        static constexpr bn::fixed CAMERA_DEADZONE_X = 24;                // Horizontal deadzone radius (increased 50% for more stable camera)
        static constexpr bn::fixed CAMERA_DEADZONE_Y = 9;                 // Vertical deadzone radius (increased 50% for more stable camera)
        static constexpr bn::fixed CAMERA_FOLLOW_SPEED = 0.02;            // How fast camera catches up (0.02 = 2% per frame - much slower and smoother)
        static constexpr bn::fixed CAMERA_DIRECTION_CHANGE_SPEED = 0.012; // Slower speed when changing directions (1.2% per frame)
        static constexpr int CAMERA_DIRECTION_CHANGE_DURATION = 30;       // Frames to use slow speed after direction change (0.5 seconds at 60fps)
        static constexpr bn::fixed CAMERA_LOOKAHEAD_X = 100;              // How far ahead to look horizontally (increased for much better forward vision)
        static constexpr bn::fixed CAMERA_LOOKAHEAD_Y = 80;               // How far ahead to look vertically (increased for much better forward vision)

        void _init_world_specific_content(int world_id, bn::camera_ptr &camera, bn::regular_bg_ptr &bg, bn::sprite_text_generator &text_generator);
        void _save_current_state();
    };
}

#endif