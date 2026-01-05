#include "fe_npc_derived.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_builder.h"

namespace fe
{
    bn::string_view TortoiseNPC::_dialogue_lines[12] = {
        "To be honest...",
        "I have no idea what happened here.",
        "It doesn't look like they took anything.",
        ". . .",
        "Oh.",
        "Oh my . . .",
        "You have kids and a wife?",
        "Glad they weren't here then.",
        ". . . ",
        "Oh dear, they were here?",
        "These bastards can't have gone far.",
        "The tortoise brigade is on the case."};

    bn::string_view MerchantNPC::_dialogue_lines[5] = {
        "Hello there, traveler!",
        "Would you like to see my wares?",
        "I have the finest goods in the land.",
        "Come back anytime!",
        "Safe travels!"};

    TortoiseNPC::TortoiseNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator)
        : NPC(pos, camera, NPC_TYPE::TORTOISE, text_generator)
    {
        initialize_sprite();
        initialize_dialogue();
    }

    void TortoiseNPC::initialize_sprite()
    {
        bn::sprite_builder builder(bn::sprite_items::tortoise_sprite);
        builder.set_position(pos());
        builder.set_bg_priority(1);
        builder.set_z_order(100);
        _sprite = builder.build();
        if (_sprite.has_value())
        {
            set_camera(_camera);
        }
    }

    void TortoiseNPC::initialize_dialogue()
    {
        _lines = bn::span(_dialogue_lines);
    }

    MerchantNPC::MerchantNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator)
        : NPC(pos, camera, NPC_TYPE::MERCHANT, text_generator)
    {
        initialize_sprite();
        initialize_dialogue();
    }

    void MerchantNPC::initialize_sprite()
    {
        bn::sprite_builder builder(bn::sprite_items::merchant);
        builder.set_position(pos());
        builder.set_bg_priority(1);
        builder.set_z_order(100);
        _sprite = builder.build();
        if (_sprite.has_value())
        {
            set_camera(_camera);
        }
    }

    void MerchantNPC::initialize_dialogue()
    {
        _lines = bn::span(_dialogue_lines);
    }
}
