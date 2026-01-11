#include "fe_npc_derived.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_builder.h"

namespace fe
{
    // Initial greeting dialog
    bn::string_view MerchantNPC::_dialogue_lines[3] = {
        "Hello there, traveler!",
        "I'm a wandering merchant.",
        "What can I help you with?"};

    // Response for "Ask about past" option
    bn::string_view MerchantNPC::_past_response_lines[4] = {
        "Ah, my past... well,",
        "I've traveled far and wide,",
        "trading goods across the lands.",
        "Every journey has a story!"};

    // Response for "Ask for directions" option
    bn::string_view MerchantNPC::_directions_response_lines[3] = {
        "Looking for somewhere specific?",
        "Head north for the forest,",
        "or south to reach the desert."};

    // Response for "Goodbye" option
    bn::string_view MerchantNPC::_goodbye_response_lines[2] = {
        "Safe travels, friend!",
        "Come back anytime!"};

    MerchantNPC::MerchantNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator)
        : NPC(pos, camera, NPC_TYPE::MERCHANT, text_generator)
    {
        initialize_sprite();
        initialize_dialogue();
        initialize_dialog_options();
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

    void MerchantNPC::initialize_dialog_options()
    {
        _has_dialog_options = true;
        
        // Add "Ask about past" option (loops back to options)
        DialogOption past_option("Ask about his past", bn::span(_past_response_lines), false);
        _dialog_options.push_back(past_option);
        
        // Add "Ask for directions" option (loops back to options)
        DialogOption directions_option("Ask for directions", bn::span(_directions_response_lines), false);
        _dialog_options.push_back(directions_option);
        
        // Add "Goodbye" option (ends conversation)
        DialogOption goodbye_option("Goodbye", bn::span(_goodbye_response_lines), true);
        _dialog_options.push_back(goodbye_option);
    }
}
