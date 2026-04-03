#include "str_bg_dialog.h"

namespace str
{

void BgDialog::_write_text(int row, int col, const char* text, int len)
{
    for(int i = 0; i < len; ++i)
    {
        int target_col = col + i;
        if(target_col < VISIBLE_COL_LEFT)
        {
            continue;
        }
        if(target_col > VISIBLE_COL_RIGHT)
        {
            break;
        }

        int ch = static_cast<unsigned char>(text[i]);
        int tile_index = 0;
        if(ch >= 32 && ch <= 127)
        {
            tile_index = ch - 32;
        }
        _set_cell(target_col, row, tile_index);
    }
}

void BgDialog::_write_text_centered(int row, const char* text, int len)
{
    int start_col = VISIBLE_COL_LEFT + ((VISIBLE_COL_RIGHT - VISIBLE_COL_LEFT + 1) - len) / 2;
    if(start_col < VISIBLE_COL_LEFT)
    {
        start_col = VISIBLE_COL_LEFT;
    }
    _write_text(row, start_col, text, len);
}

void BgDialog::_write_text_centered(int row, const char* text)
{
    int len = 0;
    while(text[len] != '\0')
    {
        ++len;
    }

    _write_text_centered(row, text, len);
}

void BgDialog::_write_wrapped_text_centered(int bottom_row, const char* text, int total_len, int visible_len)
{
    int max_cols = VISIBLE_COL_RIGHT - VISIBLE_COL_LEFT + 1;

    if(visible_len <= 0)
    {
        return;
    }

    if(visible_len > total_len)
    {
        visible_len = total_len;
    }

    if(total_len <= max_cols)
    {
        _write_text_centered(bottom_row, text, visible_len);
        return;
    }

    int split = max_cols;
    for(int index = max_cols; index > 0; --index)
    {
        if(text[index - 1] == ' ')
        {
            split = index - 1;
            break;
        }
    }

    if(split <= 0)
    {
        split = max_cols;
    }

    int second_start = split;
    while(second_start < total_len && text[second_start] == ' ')
    {
        ++second_start;
    }

    int first_visible = bn::min(visible_len, split);
    if(first_visible > 0)
    {
        _write_text_centered(bottom_row - 1, text, first_visible);
    }

    if(visible_len > second_start)
    {
        int second_visible = bn::min(visible_len - second_start, max_cols);
        if(second_visible > 0)
        {
            _write_text_centered(bottom_row, text + second_start, second_visible);
        }
    }
}

void BgDialog::_set_cell(int x, int y, int tile_index)
{
    int index = y * MAP_COLUMNS + x;
    bn::regular_bg_map_cell_info cell_info(_cells[index]);
    cell_info.set_tile_index(tile_index);
    cell_info.set_palette_id(0);
    cell_info.set_horizontal_flip(false);
    cell_info.set_vertical_flip(false);
    _cells[index] = cell_info.cell();
}

void BgDialog::_clear_row(int row)
{
    for(int col = 0; col < MAP_COLUMNS; ++col)
    {
        _set_cell(col, row, 0);
    }
}

void BgDialog::_clear_text_area()
{
    for(int row = TEXT_AREA_TOP; row <= TEXT_AREA_BOTTOM; ++row)
    {
        _clear_row(row);
    }
}

void BgDialog::_clear_all_text()
{
    for(int row = 0; row < MAP_ROWS; ++row)
    {
        _clear_row(row);
    }
    _flush();
}

void BgDialog::_flush()
{
    if(_bg_map.has_value())
    {
        _bg_map->reload_cells_ref();
    }
}

}
