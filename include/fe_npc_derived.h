#ifndef FE_NPC_DERIVED_H
#define FE_NPC_DERIVED_H

#include "fe_npc.h"
#include "bn_sprite_items_merchant.h"

namespace fe
{
    // Merchant NPC
    class MerchantNPC : public NPC
    {
    public:
        MerchantNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator);

    protected:
        void initialize_sprite() override;
        void initialize_dialogue() override;
        void initialize_dialog_options() override;

    private:
        static bn::string_view _dialogue_lines[3];
        static bn::string_view _past_response_lines[4];
        static bn::string_view _directions_response_lines[3];
        static bn::string_view _goodbye_response_lines[2];
    };
}

#endif
