#ifndef FE_COLLISION_H
#define FE_COLLISION_H

#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_log.h"
#include "bn_string.h"
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

        // Bounding box collision with point and dimensions
        [[nodiscard]] static bool check_bb(const Hitbox &boxA, bn::fixed x, bn::fixed y, bn::fixed w, bn::fixed h)
        {
            return boxA.x() - boxA.width() / 2 < x + w / 2 &&
                   boxA.x() + boxA.width() / 2 > x - w / 2 &&
                   boxA.y() - boxA.height() / 2 < y + h / 2 &&
                   boxA.y() + boxA.height() / 2 > y - h / 2;
        }

        // Logging collision details
        static void log_collision(const char *entityA, const char *entityB,
                                  bn::fixed_point posA, bn::fixed_point posB)
        {
            bn::string<100> collision_log;
            collision_log.append("COLLISION: ");
            collision_log.append(entityA);
            collision_log.append(" with ");
            collision_log.append(entityB);
            collision_log.append(" at A(X: ");
            collision_log.append(bn::to_string<20>(posA.x()));
            collision_log.append(", Y: ");
            collision_log.append(bn::to_string<20>(posA.y()));
            collision_log.append(") B(X: ");
            collision_log.append(bn::to_string<20>(posB.x()));
            collision_log.append(", Y: ");
            collision_log.append(bn::to_string<20>(posB.y()));
            collision_log.append(")");
            bn::log(collision_log);
        }
    };
}

#endif
