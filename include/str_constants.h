#ifndef STR_CONSTANTS_H
#define STR_CONSTANTS_H

#include "bn_fixed.h"

namespace str
{
    // Minimap layout
    constexpr int MINIMAP_NUM_ROOMS = 2;
    constexpr int MINIMAP_PANEL_X = 96;
    constexpr int MINIMAP_PANEL_Y = -56;
    constexpr bn::fixed MINIMAP_BORDER_SCALE = bn::fixed(0.726);
    constexpr int MINIMAP_ROOM_SIZE = 16;
    constexpr int MINIMAP_ROOM_GAP = 2;
    constexpr int MINIMAP_GRID_CELL = MINIMAP_ROOM_SIZE + MINIMAP_ROOM_GAP;

    // Z-orders for minimap layers
    constexpr int Z_ORDER_MINIMAP_BG = 15;
    constexpr int Z_ORDER_MINIMAP_ROOM = 13;
    constexpr int Z_ORDER_MINIMAP_DOOR = 12;
    constexpr int Z_ORDER_MINIMAP_PLAYER = 11;

    // Camera follow constants
    constexpr bn::fixed CAMERA_DEADZONE_X = 16;
    constexpr bn::fixed CAMERA_DEADZONE_Y = 10;
    constexpr bn::fixed CAMERA_FOLLOW_SPEED = 0.06;
    constexpr bn::fixed CAMERA_LOOKAHEAD_X = 36;
    constexpr bn::fixed CAMERA_LOOKAHEAD_Y = 24;
    constexpr bn::fixed CAMERA_SNAPBACK_SPEED = 0.03;
    constexpr bn::fixed CAMERA_CATCH_UP_SPEED = 0.12;
    constexpr bn::fixed CAMERA_LOOKAHEAD_SMOOTHING = 0.12;
    constexpr bn::fixed CAMERA_LOOKAHEAD_DECAY = 0.95;
}

#endif
