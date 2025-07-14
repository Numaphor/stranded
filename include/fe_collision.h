#ifndef FE_COLLISION_H
#define FE_COLLISION_H

#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_span.h"
#include "bn_log.h"
#include "bn_size.h"
#include "bn_vector.h"
#include "bn_affine_bg_ptr.h"
#include "bn_affine_bg_map_cell.h"

#include "fe_extras.h"
#include "fe_hitbox.h"
#include "fe_level.h"
#include "fe_player.h"
#include "fe_enemy.h"
#include "fe_npc.h"

namespace fe
{
    class Collision
    {
    public:
        // Basic bounding box collision between two Hitboxes
        [[nodiscard]] static bool check_bb(const Hitbox &boxA, const Hitbox &boxB)
        {
            const bn::fixed centerX_A = boxA.x();
            const bn::fixed centerY_A = boxA.y();
            const bn::fixed centerX_B = boxB.x();
            const bn::fixed centerY_B = boxB.y();

            const bn::fixed halfWidth_A = boxA.width() / 2;
            const bn::fixed halfHeight_A = boxA.height() / 4; // Compressed vertical height
            const bn::fixed halfWidth_B = boxB.width() / 2;
            const bn::fixed halfHeight_B = boxB.height() / 4;

            const bn::fixed dx = bn::abs(centerX_A - centerX_B);
            const bn::fixed dy = bn::abs(centerY_A - centerY_B);

            return (dx / (halfWidth_A + halfWidth_B) + dy / (halfHeight_A + halfHeight_B)) <= 1;
        }

        // Bounding box collision with point and dimensions
        [[nodiscard]] static bool check_bb(const Hitbox &boxA, bn::fixed x, bn::fixed y, bn::fixed w, bn::fixed h)
        {
            return boxA.x() - boxA.width() / 2 < x + w / 2 &&
                   boxA.x() + boxA.width() / 2 > x - w / 2 &&
                   boxA.y() - boxA.height() / 2 < y + h / 2 &&
                   boxA.y() + boxA.height() / 2 > y - h / 2;
        }

        // Bounding box collision between Player and Enemy using their hitboxes
        [[nodiscard]] static bool check_player_enemy(const Player &player, const Enemy &enemy)
        {
            // Get the hitboxes for both player and enemy
            Hitbox playerHitbox = player.get_hitbox();
            Hitbox enemyHitbox = enemy.get_hitbox();

            // Get the positions
            bn::fixed playerX = player.pos().x();
            bn::fixed playerY = player.pos().y();
            bn::fixed enemyX = enemy.get_position().x();
            bn::fixed enemyY = enemy.get_position().y();

            // Calculate the half sizes for each hitbox
            bn::fixed playerHalfW = playerHitbox.width() / 2;
            bn::fixed playerHalfH = playerHitbox.height() / 2;
            bn::fixed enemyHalfW = enemyHitbox.width() / 2;
            bn::fixed enemyHalfH = enemyHitbox.height() / 2;

            // Check for overlap on both axes
            bool xOverlap = bn::abs(playerX - enemyX) < (playerHalfW + enemyHalfW);
            bool yOverlap = bn::abs(playerY - enemyY) < (playerHalfH + enemyHalfH);

            return xOverlap && yOverlap;
        }

        // Bounding box collision between Player and NPC (like merchant)
        [[nodiscard]] static bool check_player_npc(const Player &player, const NPC &npc)
        {
            // Get player position and size - match the world scene hitbox sizes
            bn::fixed_point player_pos = player.pos();
            constexpr bn::fixed player_half_width = 16; // Half of 32 to match world scene
            constexpr bn::fixed player_half_height = 4; // Half of 32 to match world scene

            // Get NPC position and size - reduced by 50% width, 75% height
            bn::fixed_point npc_pos = npc.pos();
            constexpr bn::fixed npc_half_width = 4;  // Half of 8 (50% of original 16)
            constexpr bn::fixed npc_half_height = 4; // Half of 8 (25% of original 32)

            // Check for overlap on both axes
            bool xOverlap = bn::abs(player_pos.x() - npc_pos.x()) < (player_half_width + npc_half_width);
            bool yOverlap = bn::abs(player_pos.y() - npc_pos.y()) < (player_half_height + npc_half_height);

            return xOverlap && yOverlap;
        }

        // Generic hitbox collision check
        [[nodiscard]] static bool check_hitbox_collision(const Hitbox &hitbox1, const Hitbox &hitbox2)
        {
            return hitbox1.collides_with(hitbox2);
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