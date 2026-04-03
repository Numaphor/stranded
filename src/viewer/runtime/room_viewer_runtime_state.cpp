#include "private/viewer/runtime/room_viewer_runtime_state.h"
namespace str::rv_runtime_state
{
const bn::color npc_room_b_hat_color_0(12, 28, 24);
const bn::color npc_room_b_hat_color_1(6, 20, 18);
const bn::color npc_room_b_hat_color_2(8, 12, 16);
namespace
{
constexpr bn::string_view villager_a_greeting[] = {
    "Hello, traveler!",
    "These old halls hold many secrets.",
    "What brings you here?"
};
constexpr bn::string_view villager_a_opt0_resp[] = {
    "This gallery was built centuries ago.",
    "The paintings... they watch you."
};
constexpr bn::string_view villager_a_opt1_resp[] = {
    "Be careful in the dark rooms.",
    "Strange things lurk in the shadows."
};
constexpr bn::string_view villager_b_greeting[] = {
    "Ah, another visitor!",
    "Not many make it this far.",
    "Can I help you with something?"
};
constexpr bn::string_view villager_b_opt0_resp[] = {
    "I've been here longer than I remember.",
    "Time moves strangely in this place."
};
constexpr bn::string_view villager_b_opt1_resp[] = {
    "The exit? I think it's through",
    "the room with the tall painting."
};
constexpr str::BgDialog::DialogOption villager_a_options[] = {
    {"Tell me about this place", villager_a_opt0_resp, false},
    {"Any warnings?", villager_a_opt1_resp, false},
    {"Goodbye", {}, true}
};
constexpr str::BgDialog::DialogOption villager_b_options[] = {
    {"Who are you?", villager_b_opt0_resp, false},
    {"How do I get out?", villager_b_opt1_resp, false},
    {"Goodbye", {}, true}
};
}
void begin_npc_dialog(str::BgDialog& dialog, int npc_index)
{
    if(npc_index == 0)
    {
        dialog.set_greeting(villager_a_greeting);
        dialog.set_options(villager_a_options);
    }
    else
    {
        dialog.set_greeting(villager_b_greeting);
        dialog.set_options(villager_b_options);
    }
    dialog.talk();
}
}
