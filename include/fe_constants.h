#ifndef FE_CONSTANTS_H
#define FE_CONSTANTS_H

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
    // Physical collision zone (24x24) - UNUSED since merchant collision removed
    constexpr int MERCHANT_COLLISION_ZONE_WIDTH = 24;  // Kept for compatibility
    constexpr int MERCHANT_COLLISION_ZONE_HEIGHT = 24; // Kept for compatibility

    // Interaction zone (large, for triggering conversations)
    constexpr int MERCHANT_INTERACTION_ZONE_WIDTH = 100;
    constexpr int MERCHANT_INTERACTION_ZONE_HEIGHT = 100;

    // ===== DEBUG VISUALIZATION CONSTANTS =====
    // Tile indices used for debug visualization
    constexpr int COLLISION_ZONE_TILE_INDEX = 3;   // For collision zones (red)
    constexpr int INTERACTION_ZONE_TILE_INDEX = 4; // For interaction zones (blue)
    constexpr int LEFT_MARKER_TILE_INDEX = 5;      // For left markers (top)
    constexpr int RIGHT_MARKER_TILE_INDEX = 6;     // For right markers (right)
}

#endif
