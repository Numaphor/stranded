#ifndef FE_NPC_DERIVED_H
#define FE_NPC_DERIVED_H

#include "fe_npc.h"
#include "bn_sprite_items_merchant.h"

namespace fe
{
    // Tortoise NPC
    class TortoiseNPC : public NPC
    {
    public:
        TortoiseNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator);

    protected:
        void initialize_sprite() override;
        void initialize_dialogue() override;

    private:
        static bn::string_view _dialogue_lines[12];
    };

    // Merchant NPC
    class MerchantNPC : public NPC
    {
    public:
        MerchantNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator);

    protected:
        void initialize_sprite() override;
        void initialize_dialogue() override;

    private:
        static bn::string_view _dialogue_lines[5];
    };
}

#endif
