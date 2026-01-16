#include "str_direction_utils.h"
#include "str_constants.h"

#include "bn_fixed.h"
#include "bn_fixed_point.h"

namespace str
{

    // =========================================================================
    // Direction Utils Implementation
    // =========================================================================

    namespace direction_utils
    {
        bn::fixed_point get_roll_offset(Direction dir, int frames_remaining, int total_frames)
        {
            bn::fixed momentum_factor = bn::fixed(frames_remaining) / bn::fixed(total_frames);
            momentum_factor = (momentum_factor * 0.7) + 0.3;
            bn::fixed current_speed = PLAYER_ROLL_SPEED * momentum_factor;
            switch (dir)
            {
            case Direction::UP:
                return bn::fixed_point(0, -current_speed);
            case Direction::DOWN:
                return bn::fixed_point(0, current_speed);
            case Direction::LEFT:
                return bn::fixed_point(-current_speed, 0);
            case Direction::RIGHT:
                return bn::fixed_point(current_speed, 0);
            default:
                return bn::fixed_point(0, 0);
            }
        }

        int get_gun_z_offset(Direction dir)
        {
            switch (dir)
            {
            case Direction::UP:
                return -1;
            case Direction::DOWN:
                return 1;
            case Direction::LEFT:
            case Direction::RIGHT:
            default:
                return -1;
            }
        }
    }

} // namespace str
