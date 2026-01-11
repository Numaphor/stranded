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
#include "bn_affine_bg_ptr.h"
#include "bn_affine_bg_items_sword.h"
#include "bn_rect_window.h"
#include "bn_sprite_affine_mat_ptr.h"
#include "fe_constants.h"

#include "fe_scene.h"
#include "fe_player.h"
#include "fe_enemy.h"
#include "fe_level.h"
#include "fe_minimap.h"
#include "fe_npc_derived.h" // Include NPC derived classes
#include "fe_hitbox.h"
#include "fe_world_state.h"

namespace fe
{
    // Forward declaration for future feature
    class PlayerStatusDisplay;

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
        bn::optional<bn::affine_bg_ptr> _sword_bg;
        NPC *_merchant;                                   // Changed to base NPC pointer to allow different types
        PlayerStatusDisplay *_player_status_display;      // Future: Player status display (will be converted to unique_ptr)
        bn::optional<bn::camera_ptr> _camera;             // Camera for positioning
        PlayerMovement::Direction _last_camera_direction; // Track last direction for smooth direction changes
        int _direction_change_frames;                     // Counter for how many frames we've been changing direction
        int _current_world_id;                            // Track current world

        // Camera deadzone system
        bn::fixed_point _camera_target_pos; // Where the camera wants to be
        bn::fixed_point _lookahead_current; // Smoothed lookahead vector

        // Screen shake system
        int _shake_frames;           // Number of frames left to shake
        bn::fixed _shake_intensity;  // Current shake intensity
        int _continuous_fire_frames; // How many frames player has been firing continuously

        // Zoom system
        bool _zoomed_out;                                           // Whether camera is zoomed out
        bn::fixed _current_zoom_scale;                              // Current zoom scale (interpolates between normal and zoomed)
        bn::optional<bn::sprite_affine_mat_ptr> _zoom_affine_mat;   // Shared affine matrix for zoom scaling
        bn::optional<bn::sprite_affine_mat_ptr> _gun_affine_mat;    // Separate affine matrix for gun (needs its own rotation)
        bn::optional<bn::sprite_affine_mat_ptr> _player_affine_mat; // Separate affine matrix for player (needs its own flip)
        bn::optional<bn::sprite_affine_mat_ptr> _vfx_affine_mat;    // Separate affine matrix for VFX (needs its own flip)

        // Debug system
        bool _debug_enabled; // Whether hitbox debug visualization is enabled

        void _init_world_specific_content(int world_id, bn::camera_ptr &camera, bn::regular_bg_ptr &bg, bn::sprite_text_generator &text_generator);
        void _save_current_state();
        void _update_camera_shake();                                // Update screen shake effect
        void trigger_screen_shake(int frames, bn::fixed intensity); // Trigger screen shake
    };
}

#endif