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
            momentum_factor = (momentum_factor * 0.7) + 0.3; // Range from 1.0 to 0.3
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
            // UP: -5 (in front), DOWN: 5 (behind), LEFT: 0, RIGHT: 0
            switch (dir)
            {
            case Direction::UP:
                return -5;
            case Direction::DOWN:
                return 5;
            case Direction::LEFT:
            case Direction::RIGHT:
                return 0;
            default:
                return 0;
            }
        }
    }
}
