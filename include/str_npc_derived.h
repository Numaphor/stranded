#ifndef STR_NPC_DERIVED_H
#define STR_NPC_DERIVED_H

#include "str_npc.h"
#include "str_quest.h"
#include "bn_sprite_items_merchant.h"
#include "bn_string.h"
#include "bn_string_view.h"

namespace str
{
    // Merchant NPC; optional QuestManager for dynamic quest options (Accept / How's my quest? / Turn in).
    class MerchantNPC : public NPC
    {
    public:
        MerchantNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator, QuestManager* quest_manager = nullptr);

        void set_quest_manager(QuestManager* qm) { _quest_manager = qm; }

    protected:
        void initialize_sprite() override;
        void initialize_dialogue() override;
        void initialize_dialog_options() override;
        void on_dialog_option_selected(int option_index) override;
        void talk() override;

    private:
        void refresh_dialog_options();

        static bn::string_view _dialogue_lines[3];
        static bn::string_view _past_response_lines[4];
        static bn::string_view _directions_response_lines[3];
        static bn::string_view _goodbye_response_lines[2];
        static bn::string_view _turn_in_response_lines[2];
        static bn::string_view _accept_collect_response[2];
        static bn::string_view _accept_slay_response[2];

        QuestManager* _quest_manager = nullptr;
        int _option_index_accept = -1;
        QuestID _option_quest_accept = QuestID::_COUNT;
        int _option_index_turn_in = -1;
        QuestID _option_quest_turn_in = QuestID::_COUNT;

        bn::string<48> _progress_line;
        bn::string_view _progress_line_view;
        bn::string<64> _accept_option_text;
        bn::string<64> _turn_in_option_text;
    };
}

#endif
