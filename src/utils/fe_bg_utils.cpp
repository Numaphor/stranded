#include "bn_core.h"
#include "bn_regular_bg_map_cell_info.h"
#include "bn_regular_bg_map_cell.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_fixed.h"
#include "bn_format.h"
#include "bn_log.h"
#include "bn_optional.h"
#include "bn_span.h"

namespace fe
{

int get_tile_index(const bn::regular_bg_ptr& bg, int x, int y, int columns)
{
    // Safety checks
    bn::span<const bn::regular_bg_map_cell> cells = bg.map().cells_ref().value();
    int total_cells = cells.size();
    int index = x + y * columns;

    // Check if coordinates are within bounds
    if (x < 0 || y < 0 || index < 0 || index >= total_cells)
    {
        BN_LOG("get_tile_index: Out of bounds - x=", x, " y=", y, " columns=", columns, " total_cells=", total_cells);
        return -1;  // Invalid tile index
    }

    bn::regular_bg_map_cell unique_id = cells[index];
    bn::regular_bg_map_cell_info cell_info(unique_id);
    return cell_info.tile_index();
}

bool is_tile_non_empty(const bn::regular_bg_ptr& bg, int x, int y, int columns)
{
    int tile_index = get_tile_index(bg, x, y, columns);
    
    // Check for invalid tile index
    if (tile_index < 0)
    {
        BN_LOG("is_tile_non_empty: Invalid tile index at x=", x, " y=", y);
        return false;
    }

    return tile_index != 0;  // Assuming 0 is an empty tile
}

void log_bg_tile_indices(const bn::regular_bg_ptr& bg, int columns, int max_rows = 39, int max_cols = 39)
{
    BN_LOG("Tile indices for background:");
    BN_LOG("");

    bn::span<const bn::regular_bg_map_cell> cells = bg.map().cells_ref().value();
    bn::regular_bg_map_cell_info cell_info;

    for (int y = 0; y < max_rows; ++y)
    {
        char _bn_string[BN_CFG_LOG_MAX_SIZE];
        bn::istring_base _bn_istring(_bn_string);
        bn::ostringstream _bn_string_stream(_bn_istring);

        for (int x = 0; x < max_cols; ++x)
        {
            cell_info.set_cell(cells[x + y * columns]);
            int tile_index = cell_info.tile_index();
            _bn_string_stream.append_args(tile_index, " ");
        }

        bn::log(_bn_istring);
    }
}

} // namespace fe
