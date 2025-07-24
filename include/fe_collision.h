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
#include "fe_entity.h"
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
            // Treat hitbox x,y as top-left corner, not center
            // Calculate the actual bounds of each hitbox
            const bn::fixed left_A = boxA.x();
            const bn::fixed right_A = boxA.x() + boxA.width();
            const bn::fixed top_A = boxA.y();
            const bn::fixed bottom_A = boxA.y() + boxA.height();

            const bn::fixed left_B = boxB.x();
            const bn::fixed right_B = boxB.x() + boxB.width();
            const bn::fixed top_B = boxB.y();
            const bn::fixed bottom_B = boxB.y() + boxB.height();

            // Standard AABB collision detection
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

        // Generic entity collision detection using Entity base class
        [[nodiscard]] static bool check_entity_collision(const Entity &entityA, const Entity &entityB)
        {
            // Get the hitboxes for both entities
            Hitbox hitboxA = entityA.get_hitbox();
            Hitbox hitboxB = entityB.get_hitbox();

            // Get the positions
            bn::fixed_point posA = entityA.pos();
            bn::fixed_point posB = entityB.pos();

            // Calculate the half sizes for each hitbox
            bn::fixed halfWA = hitboxA.width() / 2;
            bn::fixed halfHA = hitboxA.height() / 2;
            bn::fixed halfWB = hitboxB.width() / 2;
            bn::fixed halfHB = hitboxB.height() / 2;

            // Check for overlap on both axes
            bool xOverlap = bn::abs(posA.x() - posB.x()) < (halfWA + halfWB);
            bool yOverlap = bn::abs(posA.y() - posB.y()) < (halfHA + halfHB);

            return xOverlap && yOverlap;
        }

        // Backward compatibility wrapper: Player-Enemy collision
        [[nodiscard]] static bool check_player_enemy(const Player &player, const Enemy &enemy)
        {
            // Get the hitboxes for both player and enemy
            Hitbox playerHitbox = player.get_hitbox();
            Hitbox enemyHitbox = enemy.get_hitbox();

            // Get the positions
            bn::fixed playerX = player.pos().x();
            bn::fixed playerY = player.pos().y();
            bn::fixed_point enemyPos = enemy.get_position();

            // Calculate the half sizes for each hitbox
            bn::fixed playerHalfW = playerHitbox.width() / 2;
            bn::fixed playerHalfH = playerHitbox.height() / 2;
            bn::fixed enemyHalfW = enemyHitbox.width() / 2;
            bn::fixed enemyHalfH = enemyHitbox.height() / 2;

            // Check for overlap on both axes
            bool xOverlap = bn::abs(playerX - enemyPos.x()) < (playerHalfW + enemyHalfW);
            bool yOverlap = bn::abs(playerY - enemyPos.y()) < (playerHalfH + enemyHalfH);

            return xOverlap && yOverlap;
        }

        // Backward compatibility wrapper: Player-NPC collision with custom sizing
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

        // Shared collision validation utilities
        [[nodiscard]] static bool validate_position_points(const bn::fixed_point points[4], const Level &level)
        {
            for (int i = 0; i < 4; ++i)
            {
                if (!level.is_position_valid(points[i]))
                {
                    return false;
                }
            }
            return true;
        }

        [[nodiscard]] static bool check_hitbox_collision_with_level(const Hitbox &hitbox, bn::fixed_point pos, fe::directions direction, const Level &level)
        {
            bn::fixed_point points[4];
            hitbox.get_collision_points(pos, direction, points);
            return validate_position_points(points, level);
        }
    };
}

#endif
