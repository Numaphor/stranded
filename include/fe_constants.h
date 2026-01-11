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

    constexpr bn::fixed PLAYER_HITBOX_WIDTH = 32;
    constexpr bn::fixed PLAYER_HITBOX_HEIGHT = 16;
    constexpr int PLAYER_HITBOX_VERTICAL_OFFSET = 8;
    constexpr int PLAYER_HITBOX_REDUCED_WIDTH = 16;
    constexpr int PLAYER_SPRITE_Y_OFFSET = 12; // Offset to align 32x32 sprite with 16px hitbox

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
    constexpr int CONTROLS_LIST_START_Y = -45;
    constexpr int CONTROLS_LIST_SPACING = 12;
    constexpr int CONTROLS_INSTRUCTIONS_Y_POSITION = 70;

    constexpr bn::fixed MAIN_WORLD_SPAWN_X = 50;
    constexpr bn::fixed MAIN_WORLD_SPAWN_Y = 100;
    constexpr bn::fixed FOREST_WORLD_SPAWN_X = 100;
    constexpr bn::fixed FOREST_WORLD_SPAWN_Y = 50;

    constexpr int Z_ORDER_BULLET = -15;
    constexpr int Z_ORDER_MINIMAP_PLAYER = 11;
    constexpr int Z_ORDER_MINIMAP_ENEMY = 10;

    constexpr bn::fixed BULLET_SCALE = 0.15;

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
    constexpr int PLAYER_DEATH_ANIMATION_DURATION = 90;
    constexpr int PLAYER_CHOP_DURATION = 40;
    constexpr int PLAYER_SLASH_DURATION = 25;
    constexpr int PLAYER_ATTACK_DURATION = 40;
    constexpr int PLAYER_BUFF_DURATION = 96;

    constexpr bn::fixed ENEMY_KNOCKBACK_STRENGTH = 2.5;
    constexpr int ENEMY_DEATH_ANIMATION_DURATION = 150;
    constexpr int ENEMY_KNOCKBACK_DURATION = 10;
    constexpr bn::fixed ENEMY_ATTACK_DISTANCE = 20;

    // Enemy hitbox constants
    // Standard enemy hitbox is 8x8 pixels centered on the sprite
    constexpr int ENEMY_HITBOX_SIZE = 8;
    constexpr int ENEMY_HITBOX_HALF_SIZE = ENEMY_HITBOX_SIZE / 2;

    // Enemy health bar positioning
    // Health bar appears above the enemy sprite
    constexpr int ENEMY_HEALTHBAR_OFFSET_X = -3;   // X offset from enemy center
    constexpr int ENEMY_HEALTHBAR_OFFSET_Y = -12;  // Y offset (negative = above)
    constexpr int ENEMY_HEALTHBAR_SPAWN_Y = -20;   // Initial spawn Y offset for health bar
    constexpr int ENEMY_HEALTHBAR_Z_ORDER = -1000; // Z-order for healthbar sprite

    // Enemy invulnerability frames after taking damage
    constexpr int ENEMY_INVULNERABILITY_FRAMES = 30;

    // Enemy state machine constants
    // Detection/chase range: 6 tiles (48 pixels)
    constexpr int ENEMY_FOLLOW_DISTANCE_TILES = 6;
    constexpr int ENEMY_FOLLOW_DISTANCE = ENEMY_FOLLOW_DISTANCE_TILES * TILE_SIZE;
    constexpr int ENEMY_FOLLOW_DISTANCE_SQ = ENEMY_FOLLOW_DISTANCE * ENEMY_FOLLOW_DISTANCE;

    // Normal unfollow range: 8 tiles (64 pixels)
    constexpr int ENEMY_UNFOLLOW_DISTANCE_TILES = 8;
    constexpr int ENEMY_UNFOLLOW_DISTANCE = ENEMY_UNFOLLOW_DISTANCE_TILES * TILE_SIZE;
    constexpr int ENEMY_UNFOLLOW_DISTANCE_SQ = ENEMY_UNFOLLOW_DISTANCE * ENEMY_UNFOLLOW_DISTANCE;

    // Aggroed spearguard chase range: 16 tiles (128 pixels)
    constexpr int ENEMY_AGGRO_CHASE_DISTANCE_TILES = 16;
    constexpr int ENEMY_AGGRO_CHASE_DISTANCE = ENEMY_AGGRO_CHASE_DISTANCE_TILES * TILE_SIZE;
    constexpr int ENEMY_AGGRO_CHASE_DISTANCE_SQ = ENEMY_AGGRO_CHASE_DISTANCE * ENEMY_AGGRO_CHASE_DISTANCE;

    // Y-alignment tolerance for spearguard horizontal attacks
    constexpr int ENEMY_Y_ALIGNMENT_TOLERANCE = 16;
    constexpr int ENEMY_Y_ALIGNMENT_TOLERANCE_SMALL = 8;

    // Enemy movement interpolation factor
    constexpr bn::fixed ENEMY_MOVEMENT_LERP = bn::fixed(0.1);

    // Patrol state constants
    constexpr bn::fixed ENEMY_PATROL_SPEED = bn::fixed(0.35);
    constexpr int ENEMY_PATROL_DURATION = 60;    // Default patrol duration in frames
    constexpr int ENEMY_IDLE_DURATION_MIN = 20;  // Minimum idle frames
    constexpr int ENEMY_IDLE_DURATION_RANGE = 40; // Random range added to minimum

    // Chase state speeds
    constexpr bn::fixed ENEMY_CHASE_SPEED = bn::fixed(0.8);
    constexpr bn::fixed ENEMY_CHASE_SPEED_REDUCED = bn::fixed(0.3); // For secondary axis movement
    constexpr bn::fixed ENEMY_RETURN_SPEED = bn::fixed(0.6);

    // Attack state constants
    constexpr int ENEMY_ATTACK_DURATION = 30;    // Attack animation frames
    constexpr bn::fixed ENEMY_SPEAR_REACH = 16;  // Spear reach distance in pixels
    constexpr bn::fixed ENEMY_RETURN_THRESHOLD = 8; // Distance threshold to consider "at post"

    // Stunned state duration in frames
    constexpr int ENEMY_STUN_DURATION = 20;

    // Animation speeds (frames per animation step)
    constexpr int ENEMY_ANIM_SPEED_IDLE = 12;
    constexpr int ENEMY_ANIM_SPEED_RUN = 8;
    constexpr int ENEMY_ANIM_SPEED_ATTACK = 6;
    constexpr int ENEMY_ANIM_SPEED_DEATH = 8;

    // Knockback decay factor (applied per frame)
    constexpr bn::fixed ENEMY_KNOCKBACK_DECAY = bn::fixed(0.9);

    constexpr int COMPANION_HITBOX_SIZE = 16;
    constexpr int COMPANION_REVIVAL_DURATION = 300;

    // Companion positioning offsets (relative to player)
    constexpr bn::fixed COMPANION_SPAWN_OFFSET_X = 8;
    constexpr bn::fixed COMPANION_SPAWN_OFFSET_Y = -8;
    constexpr bn::fixed COMPANION_OFFSET_RIGHT_X = 16;
    constexpr bn::fixed COMPANION_OFFSET_LEFT_X = -16;
    constexpr bn::fixed COMPANION_OFFSET_BELOW_Y = 12;

    // Companion movement parameters
    constexpr bn::fixed COMPANION_MAX_FOLLOW_SPEED = bn::fixed(1.2);
    constexpr bn::fixed COMPANION_MIN_FOLLOW_SPEED = bn::fixed(0.3);
    constexpr bn::fixed COMPANION_FOLLOW_ACCELERATION = bn::fixed(0.08);
    constexpr bn::fixed COMPANION_FACING_THRESHOLD = 8;  // Pixels distance before updating facing direction

    // Companion revival UI
    constexpr int COMPANION_REVIVAL_TEXT_OFFSET_Y = -20;     // Y offset for "Press A to revive" text above companion
    constexpr int COMPANION_REVIVAL_PROGRESS_OFFSET_X = 12;  // X offset for progress bar
    constexpr int COMPANION_REVIVAL_PROGRESS_FRAMES = 8;     // Number of animation frames in progress bar
    constexpr int COMPANION_TEXT_Z_ORDER = -32767;           // Text sprites on top of everything

    // Companion animation frame indices
    constexpr int COMPANION_ANIM_FRAMES_PER_DIRECTION = 4;
    constexpr int COMPANION_DEATH_FRAME_START = 12;
    constexpr int COMPANION_DEATH_FRAME_END = 21;
    constexpr int COMPANION_ANIM_SPEED_IDLE = 12;
    constexpr int COMPANION_ANIM_SPEED_DEATH = 8;

    // HUD Layout Constants
    // Screen coordinates (GBA screen is 240x160, center is 0,0)
    constexpr int HUD_HEALTH_BG_X = -258;
    constexpr int HUD_HEALTH_BG_Y = -215;
    constexpr int HUD_HEALTH_BG_MAP_INDEX = 2;

    // Soul position relative to healthbar (offsets)
    constexpr int HUD_SOUL_OFFSET_X = 161; // Soul X offset from healthbar X position
    constexpr int HUD_SOUL_OFFSET_Y = 148; // Soul Y offset from healthbar Y position

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

    // Buff menu constants
    // Base temptest position (bottom-left, opposite to gun icon at 100, 66)
    constexpr int HUD_BUFF_MENU_BASE_X = -100;
    constexpr int HUD_BUFF_MENU_BASE_Y = 66;

    // Hold duration to open buff menu (2 seconds = 120 frames at 60fps)
    constexpr int HUD_BUFF_MENU_HOLD_FRAMES = 120;
    // Cooldown duration after buff activation (10 seconds = 600 frames at 60fps)
    constexpr int HUD_BUFF_MENU_COOLDOWN_FRAMES = 600;
    constexpr int HUD_BUFF_MENU_ANIM_FRAMES = 9; // Animation has 9 frames

    // 3 options: Heal, Energy, and Power - positioned top-right, up, and right from base
    constexpr int HUD_BUFF_MENU_OPTION_OFFSET = 24;                          // Base distance from center sprite
    constexpr int HUD_BUFF_MENU_OPTION_HEAL_X = HUD_BUFF_MENU_OPTION_OFFSET; // Heal: top right (diagonal)
    constexpr int HUD_BUFF_MENU_OPTION_HEAL_Y = -HUD_BUFF_MENU_OPTION_OFFSET;
    constexpr int HUD_BUFF_MENU_OPTION_ENERGY_X = 0; // Energy: straight up
    constexpr int HUD_BUFF_MENU_OPTION_ENERGY_Y = -HUD_BUFF_MENU_OPTION_OFFSET;
    constexpr int HUD_BUFF_MENU_OPTION_POWER_X = HUD_BUFF_MENU_OPTION_OFFSET; // Power: straight right
    constexpr int HUD_BUFF_MENU_OPTION_POWER_Y = 0;

    // Zoom constants
    // Note: In GBA affine, scale < 1 makes sprites appear larger, scale > 1 makes them smaller
    // For zoom out effect (seeing more), we want sprites to appear smaller, so scale > 1
    constexpr bn::fixed ZOOM_NORMAL_SCALE = 1;
    constexpr bn::fixed ZOOM_OUT_SCALE = bn::fixed(0.6); // Scale of 0.6 makes sprites appear at 60% size (zoomed out)
    constexpr bn::fixed ZOOM_TRANSITION_SPEED = 0.1;     // How fast zoom transitions

    // Screen dimensions (GBA native resolution)
    // GBA screen is 240x160 pixels, with center at (0,0) in Butano coordinate system
    constexpr int SCREEN_WIDTH = 240;
    constexpr int SCREEN_HEIGHT = 160;
    constexpr int SCREEN_HALF_WIDTH = SCREEN_WIDTH / 2;   // 120
    constexpr int SCREEN_HALF_HEIGHT = SCREEN_HEIGHT / 2; // 80

    // Player collision/knockback constants
    constexpr bn::fixed PLAYER_KNOCKBACK_DISTANCE = 10;   // Knockback distance when hit by enemy
    constexpr int PLAYER_INVULNERABILITY_FRAMES = 60;     // Invulnerability duration in frames (1 second at 60fps)
    constexpr int PLAYER_INVULNERABILITY_BLINK_RATE = 10; // Blink visibility every N frames during invulnerability

    // Player Z-order calculation
    // Z-order offset for companion relative to player (for depth sorting)
    constexpr int Z_ORDER_COMPANION_OFFSET = 10;
    // Threshold for companion appearing behind/in front of player
    constexpr int COMPANION_BEHIND_PLAYER_Y_THRESHOLD = 8;

    // Gun Z-order offsets relative to player
    constexpr int GUN_Z_OFFSET_BEHIND = 5;  // Gun behind player when facing up
    constexpr int GUN_Z_OFFSET_FRONT = -5;  // Gun in front when facing down

    // Player ability cooldowns (in frames)
    constexpr int PLAYER_ROLL_COOLDOWN = 90;    // 1.5 seconds at 60fps
    constexpr int PLAYER_SLASH_COOLDOWN = 60;   // 1 second at 60fps

    // Weapon switching/reloading constants
    constexpr int WEAPON_GUN_FRAMES = 6;        // Number of gun sprite variants
    constexpr int WEAPON_SWORD_FRAMES = 6;      // Number of sword sprite variants

    // Roll momentum decay constants
    constexpr bn::fixed ROLL_MOMENTUM_DECAY_MIN = bn::fixed(0.3);  // Minimum momentum at end of roll
    constexpr bn::fixed ROLL_MOMENTUM_DECAY_RANGE = bn::fixed(0.7); // Range from min to full momentum

    // Minimap position
    constexpr int MINIMAP_X = 100;
    constexpr int MINIMAP_Y = -80;

    // World-specific constants
    constexpr int WORLD_ID_MAIN = 0;
    constexpr int WORLD_ID_FOREST = 1;
    constexpr int WORLD_ID_DESERT = 2;
    constexpr int WORLD_ID_OCEAN = 3;

    // Default background tiles by world
    constexpr int BG_TILE_DEFAULT = 1;
    constexpr int BG_TILE_FOREST = 2;

    // Enemy spawn HP values by world
    constexpr int ENEMY_HP_MAIN_WORLD = 3;
    constexpr int ENEMY_HP_FOREST_WORLD = 2;
    constexpr int ENEMY_HP_DESERT_WORLD = 4;

    // Camera shake random seed constants
    constexpr int SHAKE_SEED_MULTIPLIER = 1664525;
    constexpr int SHAKE_SEED_INCREMENT = 1013904223;
    constexpr int SHAKE_SEED_MODULUS = 32768;
    constexpr int SHAKE_RANGE = 16;
    constexpr int SHAKE_HALF_RANGE = SHAKE_RANGE / 2;  // For offset calculation
    constexpr bn::fixed SHAKE_DECAY = bn::fixed(0.85); // Shake intensity decay per frame
    constexpr bn::fixed SHAKE_DIVISOR = 4;             // Divides shake offset for smoother effect
}

#endif
