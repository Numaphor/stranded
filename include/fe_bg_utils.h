#ifndef FE_BG_UTILS_H
#define FE_BG_UTILS_H

#include "bn_regular_bg_ptr.h"

namespace fe
{

/**
 * @brief Gets the tile index at the specified position in the background
 * @param bg The background to get the tile from
 * @param x X coordinate in tiles
 * @param y Y coordinate in tiles
 * @param columns Number of columns in the background
 * @return The tile index at the specified position
 */
int get_tile_index(const bn::regular_bg_ptr& bg, int x, int y, int columns);

/**
 * @brief Logs all tile indices in the background to the console
 * @param bg The background to log
 * @param columns Number of columns in the background
 * @param max_rows Maximum number of rows to log (default: 39)
 * @param max_cols Maximum number of columns to log (default: 39)
 */
void log_bg_tile_indices(const bn::regular_bg_ptr& bg, int columns, int max_rows = 39, int max_cols = 39);

/**
 * @brief Checks if a tile at the specified position is non-empty (has a non-zero tile index)
 * @param bg The background to check
 * @param x X coordinate in tiles
 * @param y Y coordinate in tiles
 * @param columns Number of columns in the background
 * @return True if the tile is non-empty, false otherwise
 */
bool is_tile_non_empty(const bn::regular_bg_ptr& bg, int x, int y, int columns);

} // namespace fe

#endif // FE_BG_UTILS_H
