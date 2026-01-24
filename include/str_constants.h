#ifndef STR_CONSTANTS_H
#define STR_CONSTANTS_H

#include "bn_fixed.h"
#include "bn_fixed_point.h"

namespace str
{
    constexpr int TILE_SIZE = 8;
    // Max non-big affine map size is 128x128 tiles (1024x1024 pixels)
    // Big maps don't support scaling properly
    constexpr int MAP_COLUMNS = 128;
    constexpr int MAP_ROWS = 128;
    constexpr int MAP_CELLS_COUNT = MAP_COLUMNS * MAP_ROWS;

    constexpr int MAP_OFFSET_X = MAP_COLUMNS * 4;
    constexpr int MAP_OFFSET_Y = MAP_ROWS * 4;
    constexpr int MAP_OFFSET = MAP_OFFSET_X;

    constexpr int SWORD_ZONE_TILE_LEFT = 145;
    constexpr int SWORD_ZONE_TILE_RIGHT = 157;
    constexpr int SWORD_ZONE_TILE_TOP = 159;
    constexpr int SWORD_ZONE_TILE_BOTTOM = 165;

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

    constexpr bn::fixed PLAYER_HITBOX_WIDTH = 32;
    constexpr bn::fixed PLAYER_HITBOX_HEIGHT = 16;
    constexpr int PLAYER_HITBOX_VERTICAL_OFFSET = 8;
    constexpr int PLAYER_HITBOX_REDUCED_WIDTH = 16;
    constexpr int PLAYER_SPRITE_Y_OFFSET = 0; // Offset removed as sprites are now centered

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

    constexpr int START_TITLE_Y_POSITION = -60;
    constexpr int START_OPTIONS_START_Y = -10;
    constexpr int START_OPTIONS_SPACING = 20;
    constexpr int START_INSTRUCTIONS_Y_POSITION = 60;

    constexpr int CONTROLS_TITLE_Y_POSITION = -70;
    constexpr int CONTROLS_LIST_START_Y = -50;
    constexpr int CONTROLS_LIST_SPACING = 9;
    constexpr int CONTROLS_INSTRUCTIONS_Y_POSITION = 70;

    constexpr bn::fixed MAIN_WORLD_SPAWN_X = 50;
    constexpr bn::fixed MAIN_WORLD_SPAWN_Y = 100;
    constexpr bn::fixed FOREST_WORLD_SPAWN_X = 100;
    constexpr bn::fixed FOREST_WORLD_SPAWN_Y = 50;

    constexpr int Z_ORDER_BULLET = -15;
    constexpr int Z_ORDER_MINIMAP_PLAYER = 11;
    constexpr int Z_ORDER_MINIMAP_ENEMY = 10;

    constexpr bn::fixed BULLET_SCALE = 0.075;

    constexpr int MENU_BG_COLOR_R = 0;
    constexpr int MENU_BG_COLOR_G = 0;
    constexpr int MENU_BG_COLOR_B = 8;

    constexpr bn::fixed CAMERA_DEADZONE_X = 16;
    constexpr bn::fixed CAMERA_DEADZONE_Y = 10;
    constexpr bn::fixed CAMERA_FOLLOW_SPEED = 0.06;
    constexpr bn::fixed CAMERA_DIRECTION_CHANGE_SPEED = 0.03;
    constexpr int CAMERA_DIRECTION_CHANGE_DURATION = 15;
    constexpr bn::fixed CAMERA_LOOKAHEAD_X = 36;
    constexpr bn::fixed CAMERA_LOOKAHEAD_Y = 24;
    constexpr bn::fixed CAMERA_CENTER_BIAS = 0.2;

    // Velocity-based lookahead multipliers
    constexpr bn::fixed CAMERA_RUNNING_LOOKAHEAD_BOOST = 1.5; // Extra lookahead when running
    constexpr bn::fixed CAMERA_SNAPBACK_SPEED = 0.03;         // Slower return to center when idle
    constexpr bn::fixed CAMERA_CATCH_UP_SPEED = 0.12;         // Faster catch-up when player far from camera

    constexpr bn::fixed GUNFIRE_SHAKE_BASE_INTENSITY = 1.0;
    constexpr bn::fixed GUNFIRE_SHAKE_MAX_INTENSITY = 5.0;
    constexpr int GUNFIRE_BUILDUP_FRAMES = 120;
    constexpr bn::fixed CAMERA_LOOKAHEAD_SMOOTHING = 0.12; // How fast lookahead builds up
    constexpr bn::fixed CAMERA_LOOKAHEAD_DECAY = 0.95;     // Slower decay for smooth return when stopping

    constexpr bn::fixed PLAYER_ROLL_SPEED = 3.75;
    constexpr int PLAYER_ROLL_DURATION = 64;
    constexpr int PLAYER_DEATH_ANIMATION_DURATION = 180;
    constexpr int PLAYER_CHOP_DURATION = 25;
    constexpr int PLAYER_SLASH_DURATION = 15;
    constexpr int PLAYER_ATTACK_DURATION = 25;
    constexpr int PLAYER_BUFF_DURATION = 96;

    constexpr bn::fixed ENEMY_KNOCKBACK_STRENGTH = 3.5;
    constexpr int ENEMY_DEATH_ANIMATION_DURATION = 150;
    constexpr int ENEMY_KNOCKBACK_DURATION = 10;
    constexpr bn::fixed ENEMY_ATTACK_DISTANCE = 20;

    constexpr int COMPANION_HITBOX_SIZE = 16;
    constexpr int COMPANION_REVIVAL_DURATION = 300;

    // HUD Layout Constants
    // Screen coordinates (GBA screen is 240x160, center is 0,0)
    constexpr int HUD_HEALTH_BG_X = -258;
    constexpr int HUD_HEALTH_BG_Y = -215;
    constexpr int HUD_HEALTH_BG_MAP_INDEX = 2;

    // Soul position relative to healthbar (offsets)
    constexpr int HUD_SOUL_OFFSET_X = 161; // Soul X offset from healthbar X position
    constexpr int HUD_SOUL_OFFSET_Y = 148; // Soul Y offset from healthbar Y position

    constexpr int HUD_ENERGY_OFFSET_X = 201; // Energy X offset from healthbar X position
    constexpr int HUD_ENERGY_OFFSET_Y = 160; // Energy Y offset from healthbar Y position

    constexpr int HUD_HEALTH_SLOT_3_OFFSET_X = 241; // Health Slot 3 X offset from healthbar X position (161 + 80)
    constexpr int HUD_HEALTH_SLOT_3_OFFSET_Y = 160; // Health Slot 3 Y offset from healthbar Y position

    constexpr int HUD_ALERT_OFFSET_X = 161; // Alert X offset from healthbar X position (161 + 120)
    constexpr int HUD_ALERT_OFFSET_Y = 160; // Alert Y offset from healthbar Y position

    constexpr int HUD_WEAPON_ICON_X = 100;
    constexpr int HUD_WEAPON_ICON_Y = 66;

    constexpr int HUD_AMMO_X = 100;
    constexpr int HUD_AMMO_Y = 77;

    constexpr int HUD_SOUL_INITIAL_X = -200;
    constexpr int HUD_SOUL_INITIAL_Y = -150;
    // Final soul position is calculated relative to healthbar:
    // X = HUD_HEALTH_BG_X + HUD_SOUL_OFFSET_X = -258 + 161 = -97
    // Y = HUD_HEALTH_BG_Y + HUD_SOUL_OFFSET_Y = -215 + 148 = -67

    constexpr int HUD_BG_PRIORITY = 0;
    constexpr int HUD_SPRITE_Z_ORDER = -32000;
    constexpr int HUD_BG_Z_ORDER = -32767;

    constexpr int HUD_SOUL_ANIM_SPEED = 8;
    constexpr int HUD_SOUL_IDLE_ANIM_SPEED = 10; // Slower animation speed for idle soul animations
    constexpr int HUD_SOUL_IDLE_INTERVAL = 120;  // Frames between idle animations (2 seconds at 60fps)

    constexpr int HUD_MAX_HP = 3;
    constexpr int HUD_MAX_AMMO = 10;
    constexpr int HUD_MAX_ENERGY = 3;

    constexpr int PLAYER_ENERGY_REGEN_FRAMES = 360; // 6 seconds at 60fps

    // Buff menu constants
    // Base temptest position (bottom-left, opposite to gun icon at 100, 66)
    constexpr int HUD_BUFF_MENU_BASE_X = -100;
    constexpr int HUD_BUFF_MENU_BASE_Y = 66;

    // Hold duration to open buff menu (2 seconds = 120 frames at 60fps)
    constexpr int HUD_BUFF_MENU_HOLD_FRAMES = 120;
    // Cooldown duration after buff activation (10 seconds = 600 frames at 60fps)
    constexpr int HUD_BUFF_MENU_COOLDOWN_FRAMES = 600;
    constexpr int HUD_BUFF_MENU_ANIM_FRAMES = 9; // Animation has 9 frames

    // 3 options: Heal, Energy, and Power - positioned vertically from bottom to top
    constexpr int HUD_BUFF_MENU_VERTICAL_SPACING = 24; // Vertical spacing between options
    constexpr int HUD_BUFF_MENU_OPTION_HEAL_X = 0;     // Heal: top (above Energy)
    constexpr int HUD_BUFF_MENU_OPTION_HEAL_Y = -HUD_BUFF_MENU_VERTICAL_SPACING * 3;
    constexpr int HUD_BUFF_MENU_OPTION_ENERGY_X = 0; // Energy: middle (above Power)
    constexpr int HUD_BUFF_MENU_OPTION_ENERGY_Y = -HUD_BUFF_MENU_VERTICAL_SPACING * 2;
    constexpr int HUD_BUFF_MENU_OPTION_POWER_X = 0; // Power: bottom (above base)
    constexpr int HUD_BUFF_MENU_OPTION_POWER_Y = -HUD_BUFF_MENU_VERTICAL_SPACING;

    // Zoom constants
    // Note: In GBA affine, scale < 1 makes sprites appear larger, scale > 1 makes them smaller
    // For zoom out effect (seeing more), we want sprites to appear smaller, so scale > 1
    constexpr bn::fixed ZOOM_NORMAL_SCALE = 1;
    constexpr bn::fixed ZOOM_OUT_SCALE = bn::fixed(0.6); // Scale of 0.6 makes sprites appear at 60% size (zoomed out)
    constexpr bn::fixed ZOOM_TRANSITION_SPEED = 0.1;     // How fast zoom transitions

    // Chunk loading system constants
    constexpr int CHUNK_SIZE_TILES = 8;                               // 8x8 tiles per chunk (64px)
    constexpr int CHUNK_SIZE_PIXELS = CHUNK_SIZE_TILES * TILE_SIZE;
    constexpr int VIEW_BUFFER_CHUNKS = 16;                            // 16x16 chunks in view buffer
    constexpr int VIEW_BUFFER_TILES = VIEW_BUFFER_CHUNKS * CHUNK_SIZE_TILES; // 128 tiles (matches affine BG limit)
    constexpr int BUFFER_HALF_SIZE = VIEW_BUFFER_TILES * TILE_SIZE / 2; // 512 pixels (half of view buffer)
    constexpr int TILES_PER_FRAME = 64;                               // Streaming budget per frame
    constexpr int WORLD_WIDTH_CHUNKS = 128;                           // 1024 tiles / 8 = 128 chunks wide (8192px)
    constexpr int WORLD_HEIGHT_CHUNKS = 128;                          // 1024 tiles / 8 = 128 chunks tall (8192px)
    constexpr int WORLD_WIDTH_TILES = WORLD_WIDTH_CHUNKS * CHUNK_SIZE_TILES;   // 1024 tiles total
    constexpr int WORLD_HEIGHT_TILES = WORLD_HEIGHT_CHUNKS * CHUNK_SIZE_TILES; // 1024 tiles total
    constexpr int WORLD_WIDTH_PIXELS = WORLD_WIDTH_TILES * TILE_SIZE;   // 8192 pixels total width
    constexpr int WORLD_HEIGHT_PIXELS = WORLD_HEIGHT_TILES * TILE_SIZE; // 8192 pixels total height

    // Chunk streaming thresholds
    constexpr int CHUNK_LOAD_DISTANCE = 2; // Load chunks 2 chunks away from visible area (smaller chunks need more preloading)
}

#endif
