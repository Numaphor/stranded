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
#include "fe_constants.h"

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
        
        // Yoshi's Island-style camera system
        bn::fixed_point _camera_pos;                               // Current camera position
        bn::fixed_point _camera_target_pos;                        // Target position for camera movement
        bn::fixed_point _player_velocity;                          // Player velocity for lookahead calculation
        bool _special_camera_event_active;                         // Flag for special camera events
        bn::fixed_point _special_event_position;                   // Target position for special events
        
        // Legacy camera variables (to be removed)
        PlayerMovement::Direction _last_camera_direction;           // Track last direction for smooth direction changes
        int _direction_change_frames;                               // Counter for how many frames we've been changing direction
        int _current_world_id;                                      // Track current world

        // Camera deadzone system (legacy - now part of Yoshi's Island system)
        // bn::fixed_point _camera_target_pos;                               // Where the camera wants to be

        // Screen shake system
        int _shake_frames;                                             // Number of frames left to shake
        bn::fixed _shake_intensity;                                    // Current shake intensity
        int _continuous_fire_frames;                                   // How many frames player has been firing continuously

        void _init_world_specific_content(int world_id, bn::camera_ptr &camera, bn::regular_bg_ptr &bg, bn::sprite_text_generator &text_generator);
        void _save_current_state();
        void _update_camera_shake();                                // Update screen shake effect
        void _update_camera(bn::fixed dt);                          // Update Yoshi's Island-style camera
        bn::fixed _clamp(bn::fixed value, bn::fixed min_val, bn::fixed max_val); // Utility clamp function
        bn::fixed _sign(bn::fixed value);                           // Utility sign function
        bn::fixed _lerp(bn::fixed a, bn::fixed b, bn::fixed t);     // Utility lerp function
        void trigger_screen_shake(int frames, bn::fixed intensity); // Trigger screen shake
        void trigger_special_camera_event(bn::fixed_point target_pos); // Trigger special camera movement
        void disable_special_camera_event(); // Disable special camera movement
    };
}

#endif