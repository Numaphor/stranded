#ifndef FE_SCENE_WORLD_H
#define FE_SCENE_WORLD_H

#include "bn_fixed_point.h"
#include "bn_sprite_ptr.h"
#include "bn_camera_ptr.h"
#include "bn_vector.h"
#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_tiles_ptr.h"
#include "bn_bg_palette_ptr.h"
#include "bn_regular_bg_items_sword.h"
#include "bn_rect_window.h"

#include "fe_scene.h"
#include "fe_player.h"
#include "fe_enemy.h"
#include "fe_level.h"
#include "fe_minimap.h"
#include "fe_npc_derived.h" // Include NPC derived classes
#include "fe_hitbox_debug.h"
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
        MerchantNPC *_merchant;    // Add MerchantNPC as a member
        HitboxDebug _hitbox_debug; // Hitbox visualization system
        int _current_world_id;     // Track current world
        
        void _init_world_specific_content(int world_id, bn::camera_ptr& camera, bn::regular_bg_ptr& bg, bn::sprite_text_generator& text_generator);
        void _save_current_state();
    };
}

#endif