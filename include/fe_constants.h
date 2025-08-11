#ifndef FE_CONSTANTS_H
#define FE_CONSTANTS_H

#include "bn_fixed.h"
#include "bn_fixed_point.h"

namespace fe
{
    // ===== TILE SYSTEM CONSTANTS =====
    constexpr int TILE_SIZE = 8;
    constexpr int MAP_COLUMNS = 320;
    constexpr int MAP_ROWS = 320;
    constexpr int MAP_CELLS_COUNT = MAP_COLUMNS * MAP_ROWS;

    // Map offset calculations for coordinate transformations
    constexpr int MAP_OFFSET_X = MAP_COLUMNS * 4; // 1280
    constexpr int MAP_OFFSET_Y = MAP_ROWS * 4;    // 1280
    constexpr int MAP_OFFSET = MAP_OFFSET_X;      // Legacy name for compatibility

    // ===== SWORD ZONE CONSTANTS =====
    // Sword zone tile coordinates (for collision detection and debug visualization)
    constexpr int SWORD_ZONE_TILE_LEFT = 147;
    constexpr int SWORD_ZONE_TILE_RIGHT = 157; // exclusive upper bound
    constexpr int SWORD_ZONE_TILE_TOP = 162;
    constexpr int SWORD_ZONE_TILE_BOTTOM = 166; // exclusive upper bound

    // Sword sprite dimensions
    constexpr int SWORD_WIDTH = 256;
    constexpr int SWORD_HEIGHT = 256;
    constexpr int SWORD_HALF_WIDTH = SWORD_WIDTH / 2;
    constexpr int SWORD_HALF_HEIGHT = SWORD_HEIGHT / 2;

    // ===== MERCHANT ZONE CONSTANTS =====
    // Improved merchant collision system with separate zones
    // Collision zone (small, for physical blocking)
    constexpr int MERCHANT_COLLISION_ZONE_WIDTH = 40;
    constexpr int MERCHANT_COLLISION_ZONE_HEIGHT = 40;

    // Interaction zone (medium, for triggering conversations) - now 25x25 as requested
    constexpr int MERCHANT_INTERACTION_ZONE_WIDTH = 25;
    constexpr int MERCHANT_INTERACTION_ZONE_HEIGHT = 25;

    // ===== DEBUG VISUALIZATION CONSTANTS =====
    // Tile indices used for debug visualization
    constexpr int COLLISION_ZONE_TILE_INDEX = 3;   // For collision zones (red)
    constexpr int INTERACTION_ZONE_TILE_INDEX = 4; // For interaction zones (blue)
    constexpr int LEFT_MARKER_TILE_INDEX = 5;      // For left markers (top)
    constexpr int RIGHT_MARKER_TILE_INDEX = 6;     // For right markers (right)

    // ===== PLAYER HITBOX CONSTANTS =====
    // Player collision detection dimensions
    constexpr int PLAYER_HITBOX_WIDTH = 32;
    constexpr int PLAYER_HITBOX_HEIGHT = 16;
    constexpr int PLAYER_HITBOX_VERTICAL_OFFSET = 8; // Offset from top of sprite to start of hitbox
    constexpr int PLAYER_HITBOX_REDUCED_WIDTH = 16;  // Reduced from 32 to allow closer approach from sides

    // ===== ENTITY CONSTANTS =====
    // Default entity dimensions
    constexpr int DEFAULT_ENTITY_WIDTH = 32;
    constexpr int DEFAULT_ENTITY_HEIGHT = 32;

    // ===== BULLET SYSTEM CONSTANTS =====
    constexpr bn::fixed BULLET_SPEED = 4;
    constexpr int BULLET_LIFETIME = 60;           // Bullet disappears after 1 second (60 frames at 60 FPS)
    constexpr int SHOOT_COOLDOWN_TIME = 15;       // 15 frames between shots (4 shots/second at 60 FPS)

    // ===== COMPANION CONSTANTS =====
    constexpr bn::fixed COMPANION_IDLE_DISTANCE = 12;   // Stop moving when player gets this close
    constexpr bn::fixed COMPANION_RESUME_DISTANCE = 20; // Resume following when player moves this far away
    constexpr bn::fixed COMPANION_REVIVE_DISTANCE = 32; // Distance at which player can revive companion

    // ===== HITBOX DEBUG CONSTANTS =====
    constexpr bn::fixed HITBOX_EDGE_OFFSET = 1;          // Edge offset to stay within bounds
    constexpr bn::fixed HITBOX_SPRITE_CENTER_OFFSET = 2; // Half of marker sprite size (4/2)

    // ===== MINIMAP CONSTANTS =====
    constexpr bn::fixed MINIMAP_POSITION_SCALE = bn::fixed(1) / 40; // Scaling factor for minimap movement
    constexpr int MINIMAP_VERTICAL_OFFSET = 16; // Vertical offset for minimap positioning

    // ===== UI CONSTANTS =====
    // Menu positioning
    constexpr int MENU_TITLE_Y_POSITION = -60;
    constexpr int MENU_INSTRUCTIONS_Y_POSITION = 100;
    constexpr int MENU_WORLD_LIST_START_Y = -20;
    constexpr int MENU_WORLD_LIST_SPACING = 20;

    // World spawn locations
    constexpr bn::fixed MAIN_WORLD_SPAWN_X = 50;
    constexpr bn::fixed MAIN_WORLD_SPAWN_Y = 100;
    constexpr bn::fixed FOREST_WORLD_SPAWN_X = 100;
    constexpr bn::fixed FOREST_WORLD_SPAWN_Y = 50;

    // ===== Z-ORDER CONSTANTS =====
    constexpr int Z_ORDER_BULLET = -15;
    constexpr int Z_ORDER_PLAYER_STATUS_HIGH_PRIORITY = -1000;
    constexpr int Z_ORDER_MINIMAP_PLAYER = 11;
    constexpr int Z_ORDER_MINIMAP_ENEMY = 10;

    // ===== SPRITE SCALE CONSTANTS =====
    constexpr bn::fixed BULLET_SCALE = 0.15; // Scale for bullet sprites

    // ===== COLOR CONSTANTS =====
    // Menu background transparent color
    constexpr int MENU_BG_COLOR_R = 0;
    constexpr int MENU_BG_COLOR_G = 0;
    constexpr int MENU_BG_COLOR_B = 8;

    // ===== CAMERA SYSTEM CONSTANTS =====
    constexpr bn::fixed CAMERA_DEADZONE_X = 16;                // Horizontal deadzone radius (reduced for more responsive camera)
    constexpr bn::fixed CAMERA_DEADZONE_Y = 6;                 // Vertical deadzone radius (reduced for more responsive camera)
    constexpr bn::fixed CAMERA_FOLLOW_SPEED = 0.05;            // Faster follow speed (5% per frame) for more responsive lookahead
    constexpr bn::fixed CAMERA_DIRECTION_CHANGE_SPEED = 0.012; // Slower direction changes (1.2% per frame)
    constexpr int CAMERA_DIRECTION_CHANGE_DURATION = 28;       // Longer direction change duration (0.47 seconds at 60fps)
    constexpr bn::fixed CAMERA_LOOKAHEAD_X = 120;              // Increased horizontal lookahead for better visibility
    constexpr bn::fixed CAMERA_LOOKAHEAD_Y = 100;              // Good vertical lookahead distance
    constexpr bn::fixed CAMERA_CENTER_BIAS = 0.3;              // Less center bias (30%) to allow more lookahead

    // ===== UI POSITIONING CONSTANTS =====
    constexpr bn::fixed PLAYER_STATUS_X = 76; // X position for status display
    constexpr bn::fixed PLAYER_STATUS_Y = 70; // Y position for status display

    // ===== SCREEN SHAKE CONSTANTS =====
    constexpr int GUNFIRE_SHAKE_FRAMES = 6;                 // Reduced duration (about 0.1 seconds at 60fps)
    constexpr bn::fixed GUNFIRE_SHAKE_BASE_INTENSITY = 1.0; // Starting intensity for first shot
    constexpr bn::fixed GUNFIRE_SHAKE_MAX_INTENSITY = 5.0;  // Maximum intensity after sustained fire
    constexpr int GUNFIRE_BUILDUP_FRAMES = 120;             // Frames to reach max intensity (2 seconds at 60fps)
    constexpr bn::fixed CAMERA_LOOKAHEAD_SMOOTHING = 0.7;   // More lookahead effect (70%)

    // ===== PLAYER MOVEMENT CONSTANTS =====
    constexpr bn::fixed PLAYER_ROLL_SPEED = 3.75; // Roll speed (1.5x faster than normal movement)
    constexpr int PLAYER_ROLL_DURATION = 64;        // Increased to double total distance
    constexpr int PLAYER_ROLL_IFRAME_DURATION = 30; // Extended i-frames for longer roll
    constexpr bn::fixed PLAYER_HITBOX_WIDTH = 16;
    constexpr bn::fixed PLAYER_HITBOX_HEIGHT = 32;
    constexpr int PLAYER_DEATH_ANIMATION_DURATION = 90; // 1.5 seconds at 60 FPS

    // ===== ENEMY CONSTANTS =====
    constexpr bn::fixed ENEMY_KNOCKBACK_STRENGTH = 2.5; // Force applied during knockback
    constexpr int ENEMY_DEATH_ANIMATION_DURATION = 150; // Frames to allow death animation to complete
    constexpr int ENEMY_KNOCKBACK_DURATION = 10; // Frames of knockback
    constexpr int ENEMY_ATTACK_DURATION = 60;   // Frames for attack animation
    constexpr bn::fixed ENEMY_ATTACK_DISTANCE = 20; // Distance threshold for enemy attacks

    // ===== COMPANION HITBOX CONSTANTS =====
    constexpr int COMPANION_HITBOX_SIZE = 16; // Size of companion collision hitbox
    constexpr int COMPANION_REVIVAL_DURATION = 300; // 5 seconds at 60 FPS

    // ===== HITBOX DEBUG CONSTANTS =====
    constexpr bn::fixed HITBOX_MARKER_SPRITE_SIZE = 4;
}

#endif
