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
        static constexpr bn::fixed CAMERA_DEADZONE_X = 16;                // Horizontal deadzone radius (reduced for more responsive camera)
        static constexpr bn::fixed CAMERA_DEADZONE_Y = 6;                 // Vertical deadzone radius (reduced for more responsive camera)
        static constexpr bn::fixed CAMERA_FOLLOW_SPEED = 0.05;            // Faster follow speed (5% per frame) for more responsive lookahead
        static constexpr bn::fixed CAMERA_DIRECTION_CHANGE_SPEED = 0.012; // Slower direction changes (1.2% per frame)
        static constexpr int CAMERA_DIRECTION_CHANGE_DURATION = 28;       // Longer direction change duration (0.47 seconds at 60fps)
        static constexpr bn::fixed CAMERA_LOOKAHEAD_X = 120;              // Increased horizontal lookahead for better visibility
        static constexpr bn::fixed CAMERA_LOOKAHEAD_Y = 100;              // Good vertical lookahead distance
        static constexpr bn::fixed CAMERA_CENTER_BIAS = 0.3;              // Less center bias (30%) to allow more lookahead
        static constexpr bn::fixed CAMERA_LOOKAHEAD_SMOOTHING = 0.7;      // More lookahead effect (70%)

        // Screen shake system
        int _shake_frames;                                             // Number of frames left to shake
        bn::fixed _shake_intensity;                                    // Current shake intensity
        int _continuous_fire_frames;                                   // How many frames player has been firing continuously
        static constexpr int GUNFIRE_SHAKE_FRAMES = 6;                 // Reduced duration (about 0.1 seconds at 60fps)
        static constexpr bn::fixed GUNFIRE_SHAKE_BASE_INTENSITY = 1.0; // Starting intensity for first shot
        static constexpr bn::fixed GUNFIRE_SHAKE_MAX_INTENSITY = 5.0;  // Maximum intensity after sustained fire
        static constexpr int GUNFIRE_BUILDUP_FRAMES = 120;             // Frames to reach max intensity (2 seconds at 60fps)

        void _init_world_specific_content(int world_id, bn::camera_ptr &camera, bn::regular_bg_ptr &bg, bn::sprite_text_generator &text_generator);
        void _save_current_state();
        void _update_camera_shake();                                // Update screen shake effect
        void trigger_screen_shake(int frames, bn::fixed intensity); // Trigger screen shake
    };
}

#endif