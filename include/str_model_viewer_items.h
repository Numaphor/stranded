#ifndef STR_MODEL_VIEWER_ITEMS_H
#define STR_MODEL_VIEWER_ITEMS_H

#include "fr_model_viewer_item.h"
#include "models/fr_model_3d_items_player_car.h"
#include "models/str_model_3d_items_blaster.h"

namespace str
{

constexpr fr::model_viewer_item viewer_items[] = {
    fr::model_viewer_item(fr::model_3d_items::player_car, "Player car", 200, 28672, 0, 40704),
    fr::model_viewer_item(str::model_3d_items::blaster, "Blaster", 100, 28672, 0, 32768),
};

constexpr int viewer_item_count = sizeof(viewer_items) / sizeof(viewer_items[0]);

// Items with custom color palettes (not using fr::default_model_colors)
inline bool has_custom_colors(int index)
{
    return index == 1; // Blaster
}

}

#endif
