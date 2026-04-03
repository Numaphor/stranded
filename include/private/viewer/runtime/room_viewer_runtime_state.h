#ifndef STR_ROOM_VIEWER_RUNTIME_STATE_H
#define STR_ROOM_VIEWER_RUNTIME_STATE_H

#include "bn_color.h"
#include "str_bg_dialog.h"

namespace str::rv_runtime_state
{

constexpr int NPC_INTERACT_DIST = 30;
extern const bn::color npc_room_b_hat_color_0;
extern const bn::color npc_room_b_hat_color_1;
extern const bn::color npc_room_b_hat_color_2;
void begin_npc_dialog(str::BgDialog& dialog, int npc_index);

}

#endif
