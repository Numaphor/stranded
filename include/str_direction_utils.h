#ifndef STR_DIRECTION_UTILS_H
#define STR_DIRECTION_UTILS_H

#include "bn_fixed_point.h"
#include "str_constants.h"
#include "str_bullet_manager.h"

namespace str
{
    namespace player_constants
    {
        constexpr bn::fixed GUN_OFFSET_X[4] = {0, 0, -8, 8};
        constexpr bn::fixed GUN_OFFSET_Y[4] = {-6, 6, 0, 0};
        constexpr bool GUN_FLIPS[4] = {false, false, true, false};
        constexpr int GUN_ANGLES[4] = {90, 270, 0, 0};
        
        constexpr bn::fixed BULLET_OFFSET_X[4] = {1, -1, -12, 11};
        constexpr bn::fixed BULLET_OFFSET_Y[4] = {-9, 9, -3, 1};
    }

    namespace direction_utils
    {
        bn::fixed_point get_roll_offset(Direction dir, int frames_remaining, int total_frames);
        
        inline bn::fixed_point get_bullet_position(Direction dir, bn::fixed_point position)
        {
            const int idx = int(dir);
            return {position.x() + player_constants::BULLET_OFFSET_X[idx],
                    position.y() + player_constants::BULLET_OFFSET_Y[idx] + PLAYER_SPRITE_Y_OFFSET};
        }
        
        int get_gun_z_offset(Direction dir);
        void setup_gun(bn::sprite_ptr &gun_sprite, Direction dir, bn::fixed_point pos);
    }
}

#endif
