#include "fe_sprite_priority.h"

namespace fe
{
    int SpritePriority::calculate_depth_z_order(bn::fixed_point position, SpriteLayer base_layer)
    {
        int base_z = static_cast<int>(base_layer);
        int depth_offset = -position.y().integer();
        return base_z + depth_offset;
    }

    int SpritePriority::get_layer_z_order(SpriteLayer layer)
    {
        return static_cast<int>(layer);
    }

    int SpritePriority::get_companion_z_order(bn::fixed_point companion_pos, bn::fixed_point player_pos, bool is_flying)
    {
        if (is_flying)
        {
            return calculate_depth_z_order(companion_pos, SpriteLayer::COMPANION_FLYING);
        }
        else
        {
            return calculate_depth_z_order(companion_pos, SpriteLayer::COMPANION_GROUNDED);
        }
    }

    int SpritePriority::get_npc_z_order(bn::fixed_point npc_pos)
    {
        return calculate_depth_z_order(npc_pos, SpriteLayer::NPCS);
    }

    int SpritePriority::get_player_z_order(bn::fixed_point player_pos)
    {
        return calculate_depth_z_order(player_pos, SpriteLayer::PLAYER);
    }
}
