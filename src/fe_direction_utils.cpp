#include "fe_direction_utils.h"

namespace fe
{
    namespace direction_utils
    {
        bn::fixed_point get_roll_offset(Direction dir, int frames_remaining, int total_frames)
        {
            // Use linear momentum decay for smoother animation sync
            bn::fixed momentum_factor = bn::fixed(frames_remaining) / bn::fixed(total_frames);
            // Start fast, end slower but not too dramatic
            momentum_factor = (momentum_factor * ROLL_MOMENTUM_DECAY_RANGE) + ROLL_MOMENTUM_DECAY_MIN;
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
            // Z-order offsets for gun sprite relative to player
            // Lower z-order = drawn on top (in front)
            // UP: gun behind player (higher z), DOWN: gun in front (lower z)
            switch (dir)
            {
            case Direction::UP:
                return -1;  // gun behind player (drawn first)
            case Direction::DOWN:
                return 1;   // gun in front of player (drawn on top)
            case Direction::LEFT:
            case Direction::RIGHT:
            default:
                return -1;
            }
        }
    }
}
