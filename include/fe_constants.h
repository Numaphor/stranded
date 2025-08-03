#ifndef FE_CONSTANTS_H
#define FE_CONSTANTS_H

namespace fe
{
    // ===== TILE SYSTEM CONSTANTS =====
    constexpr int TILE_SIZE = 8;
    constexpr int MAP_COLUMNS = 32;                         // Minimum required by Butano (must be >= 32 and % 32 == 0)
    constexpr int MAP_ROWS = 32;                            // Minimum required by Butano (must be >= 32 and % 32 == 0)
    constexpr int MAP_CELLS_COUNT = MAP_COLUMNS * MAP_ROWS; // Now 1,024 cells

    // Map offset calculations for coordinate transformations
    constexpr int MAP_OFFSET_X = MAP_COLUMNS * 4; // 128
    constexpr int MAP_OFFSET_Y = MAP_ROWS * 4;    // 128
    constexpr int MAP_OFFSET = MAP_OFFSET_X;      // Legacy name for compatibility

    // ===== SWORD ZONE CONSTANTS =====
    // Sword zone tile coordinates (for collision detection and debug visualization)
    // Updated to fit within 32x32 map (centered around 16,16)
    constexpr int SWORD_ZONE_TILE_LEFT = 14;
    constexpr int SWORD_ZONE_TILE_RIGHT = 18; // exclusive upper bound
    constexpr int SWORD_ZONE_TILE_TOP = 14;
    constexpr int SWORD_ZONE_TILE_BOTTOM = 18; // exclusive upper bound

    // Sword sprite dimensions
    constexpr int SWORD_WIDTH = 256;
    constexpr int SWORD_HEIGHT = 256;
    constexpr int SWORD_HALF_WIDTH = SWORD_WIDTH / 2;
    constexpr int SWORD_HALF_HEIGHT = SWORD_HEIGHT / 2;

    // ===== MERCHANT ZONE CONSTANTS =====
    // Physical collision zone (small, for tile-based collision)
    constexpr int MERCHANT_COLLISION_ZONE_WIDTH = 16;  // Reduced for 32x32 map
    constexpr int MERCHANT_COLLISION_ZONE_HEIGHT = 16; // Reduced for 32x32 map

    // Interaction zone (medium, for triggering conversations)
    constexpr int MERCHANT_INTERACTION_ZONE_WIDTH = 32;  // Reduced for 32x32 map
    constexpr int MERCHANT_INTERACTION_ZONE_HEIGHT = 32; // Reduced for 32x32 map

    // ===== DEBUG VISUALIZATION CONSTANTS =====
    // Tile indices used for debug visualization
    constexpr int COLLISION_ZONE_TILE_INDEX = 3;   // For collision zones (red)
    constexpr int INTERACTION_ZONE_TILE_INDEX = 4; // For interaction zones (blue)
    constexpr int LEFT_MARKER_TILE_INDEX = 5;      // For left markers (top)
    constexpr int RIGHT_MARKER_TILE_INDEX = 6;     // For right markers (right)
}

#endif
