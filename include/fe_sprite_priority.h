#ifndef FE_SPRITE_PRIORITY_H
#define FE_SPRITE_PRIORITY_H

#include "bn_fixed_point.h"

namespace fe
{
    enum class SpriteLayer
    {
        UI = 0,
        EFFECTS = 100,
        COMPANION_FLYING = 200,
        COMPANION_GROUNDED = 250,
        PLAYER = 300,
        NPCS = 400,
        GROUND_OBJECTS = 500,
        BACKGROUND = 600
    };

    class SpritePriority
    {
    public:
        static int calculate_depth_z_order(bn::fixed_point position, SpriteLayer base_layer);
        static int get_layer_z_order(SpriteLayer layer);
        static int get_companion_z_order(bn::fixed_point companion_pos, bn::fixed_point player_pos, bool is_flying);
        static int get_npc_z_order(bn::fixed_point npc_pos);
        static int get_player_z_order(bn::fixed_point player_pos);
    };
}

#endif // FE_SPRITE_PRIORITY_H
