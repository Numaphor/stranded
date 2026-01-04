#ifndef FE_COLLISION_H
#define FE_COLLISION_H

#include "bn_fixed.h"
#include "fe_hitbox.h"

namespace fe
{
    class Collision
    {
    public:
        [[nodiscard]] static bool check_bb(const Hitbox &boxA, const Hitbox &boxB)
        {
            const bn::fixed left_A = boxA.x();
            const bn::fixed right_A = boxA.x() + boxA.width();
            const bn::fixed top_A = boxA.y();
            const bn::fixed bottom_A = boxA.y() + boxA.height();

            const bn::fixed left_B = boxB.x();
            const bn::fixed right_B = boxB.x() + boxB.width();
            const bn::fixed top_B = boxB.y();
            const bn::fixed bottom_B = boxB.y() + boxB.height();

            return !(right_A <= left_B || left_A >= right_B || bottom_A <= top_B || top_A >= bottom_B);
        }
    };
}

#endif
