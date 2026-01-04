#ifndef FE_CONSTANTS_H
#define FE_CONSTANTS_H

#include "bn_fixed.h"
#include "bn_fixed_point.h"

namespace fe
{
    constexpr int TILE_SIZE = 8;
    constexpr int MAP_COLUMNS = 320;
    constexpr int MAP_ROWS = 320;
    constexpr int MAP_CELLS_COUNT = MAP_COLUMNS * MAP_ROWS;

    constexpr int MAP_OFFSET_X = MAP_COLUMNS * 4;
    constexpr int MAP_OFFSET_Y = MAP_ROWS * 4;
    constexpr int MAP_OFFSET = MAP_OFFSET_X;

    constexpr int SWORD_ZONE_TILE_LEFT = 147;
    constexpr int SWORD_ZONE_TILE_RIGHT = 157;
    constexpr int SWORD_ZONE_TILE_TOP = 162;
    constexpr int SWORD_ZONE_TILE_BOTTOM = 166;

    constexpr int SWORD_WIDTH = 256;
    constexpr int SWORD_HEIGHT = 256;
    constexpr int SWORD_HALF_WIDTH = SWORD_WIDTH / 2;
    constexpr int SWORD_HALF_HEIGHT = SWORD_HEIGHT / 2;

    constexpr int MERCHANT_COLLISION_ZONE_WIDTH = 40;
    constexpr int MERCHANT_COLLISION_ZONE_HEIGHT = 40;
    constexpr int MERCHANT_INTERACTION_ZONE_WIDTH = 50;
    constexpr int MERCHANT_INTERACTION_ZONE_HEIGHT = 50;

    constexpr int COLLISION_ZONE_TILE_INDEX = 3;
    constexpr int INTERACTION_ZONE_TILE_INDEX = 4;
    constexpr int LEFT_MARKER_TILE_INDEX = 5;
    constexpr int RIGHT_MARKER_TILE_INDEX = 6;

    constexpr bn::fixed PLAYER_HITBOX_WIDTH = 32;
    constexpr bn::fixed PLAYER_HITBOX_HEIGHT = 16;
    constexpr int PLAYER_HITBOX_VERTICAL_OFFSET = 8;
    constexpr int PLAYER_HITBOX_REDUCED_WIDTH = 16;

    constexpr int DEFAULT_ENTITY_WIDTH = 32;
    constexpr int DEFAULT_ENTITY_HEIGHT = 32;

    constexpr bn::fixed BULLET_SPEED = 4;
    constexpr int BULLET_LIFETIME = 60;
    constexpr int SHOOT_COOLDOWN_TIME = 15;

    constexpr bn::fixed COMPANION_IDLE_DISTANCE = 12;
    constexpr bn::fixed COMPANION_RESUME_DISTANCE = 20;
    constexpr bn::fixed COMPANION_REVIVE_DISTANCE = 32;

    constexpr bn::fixed HITBOX_EDGE_OFFSET = 1;

    constexpr bn::fixed MINIMAP_POSITION_SCALE = bn::fixed(1) / 40;
    constexpr int MINIMAP_VERTICAL_OFFSET = 16;

    constexpr int MENU_TITLE_Y_POSITION = -60;
    constexpr int MENU_INSTRUCTIONS_Y_POSITION = 100;
    constexpr int MENU_WORLD_LIST_START_Y = -20;
    constexpr int MENU_WORLD_LIST_SPACING = 20;

    constexpr bn::fixed MAIN_WORLD_SPAWN_X = 50;
    constexpr bn::fixed MAIN_WORLD_SPAWN_Y = 100;
    constexpr bn::fixed FOREST_WORLD_SPAWN_X = 100;
    constexpr bn::fixed FOREST_WORLD_SPAWN_Y = 50;

    constexpr int Z_ORDER_BULLET = -15;
    constexpr int Z_ORDER_PLAYER_STATUS_HIGH_PRIORITY = -1000;
    constexpr int Z_ORDER_MINIMAP_PLAYER = 11;
    constexpr int Z_ORDER_MINIMAP_ENEMY = 10;

    constexpr bn::fixed BULLET_SCALE = 0.15;

    constexpr int MENU_BG_COLOR_R = 0;
    constexpr int MENU_BG_COLOR_G = 0;
    constexpr int MENU_BG_COLOR_B = 8;

    constexpr bn::fixed CAMERA_DEADZONE_X = 16;
    constexpr bn::fixed CAMERA_DEADZONE_Y = 6;
    constexpr bn::fixed CAMERA_FOLLOW_SPEED = 0.05;
    constexpr bn::fixed CAMERA_DIRECTION_CHANGE_SPEED = 0.012;
    constexpr int CAMERA_DIRECTION_CHANGE_DURATION = 28;
    constexpr bn::fixed CAMERA_LOOKAHEAD_X = 120;
    constexpr bn::fixed CAMERA_LOOKAHEAD_Y = 100;
    constexpr bn::fixed CAMERA_CENTER_BIAS = 0.3;

    constexpr bn::fixed PLAYER_STATUS_X = 76;
    constexpr bn::fixed PLAYER_STATUS_Y = 70;

    constexpr int GUNFIRE_SHAKE_FRAMES = 6;
    constexpr bn::fixed GUNFIRE_SHAKE_BASE_INTENSITY = 1.0;
    constexpr bn::fixed GUNFIRE_SHAKE_MAX_INTENSITY = 5.0;
    constexpr int GUNFIRE_BUILDUP_FRAMES = 120;
    constexpr bn::fixed CAMERA_LOOKAHEAD_SMOOTHING = 0.7;

    constexpr bn::fixed PLAYER_ROLL_SPEED = 3.75;
    constexpr int PLAYER_ROLL_DURATION = 64;
    constexpr int PLAYER_ROLL_IFRAME_DURATION = 30;
    constexpr int PLAYER_DEATH_ANIMATION_DURATION = 90;
    constexpr int PLAYER_CHOP_DURATION = 40;
    constexpr int PLAYER_SLASH_DURATION = 25;
    constexpr int PLAYER_ATTACK_DURATION = 40;
    constexpr int PLAYER_BUFF_DURATION = 96;

    constexpr bn::fixed ENEMY_KNOCKBACK_STRENGTH = 2.5;
    constexpr int ENEMY_DEATH_ANIMATION_DURATION = 150;
    constexpr int ENEMY_KNOCKBACK_DURATION = 10;
    constexpr int ENEMY_ATTACK_DURATION = 60;
    constexpr bn::fixed ENEMY_ATTACK_DISTANCE = 20;

    constexpr int COMPANION_HITBOX_SIZE = 16;
    constexpr int COMPANION_REVIVAL_DURATION = 300;
}

#endif
