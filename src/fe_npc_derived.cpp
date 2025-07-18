#include "fe_npc_derived.h"
#include "bn_sprite_animate_actions.h"

namespace fe
{
    bn::string_view GolemNPC::_dialogue_lines[14] = {
        ". . .",
        "* groans *",
        "S o m e o n e  t h e r e  ?",
        "Hello young one.",
        "* cough *",
        "Something troubles you?",
        ". . .",
        "Oh.",
        "Thats no good.",
        "I am gravely sorry.",
        "I don't know if they came passed me.",
        "I haven't awoken for many centuries now.",
        "You see..",
        "*they motion toward the sprawling vines*"
    };

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
        "The tortoise brigade is on the case."
    };

    bn::string_view Tortoise2NPC::_dialogue_lines[9] = {
        "Sorry boys",
        "This is a police investigation.",
        "Oh hey, it's you.",
        "We have locked down the crime scene",
        "And we are doing a thorough inverstigation",
        "...",
        "What? You know where they are?!?!",
        "No! Don't go there.",
        "The tortoise brigade is on the case."
    };

    bn::string_view PenguinNPC::_dialogue_lines[3] = {
        "Hurry!",
        "I saw them head down.",
        "They were heading for the caves!"
    };

    bn::string_view Penguin2NPC::_dialogue_lines[3] = {
        "Oh good!",
        "You found buddy.",
        "Let me know if there is anything I can do."
    };

    bn::string_view TabletNPC::_dialogue_lines[4] = {
        "You feel warm",
        "You feel energized",
        "Almost like the energy could burst from",
        ".. within you at any moment"
    };

    bn::string_view JeremyNPC::_dialogue_lines[15] = {
        "Damnit",
        "Those Bastards!",
        "What's the plan?",
        "...",
        "Well you know I can't do that",
        "No!",
        "Look. I want to help. I do.",
        "I would do anything.",
        "But I won't do that.",
        "Take the airship. But I can't come with you.",
        "...",
        "Good Luck boys!",
        "Oh..",
        "And here.. take some of my lives.",
        "You are going to need them more than me."
    };

    bn::string_view CageNPC::_dialogue_lines[7] = {
        "DAD!!!",
        "You found me!",
        "They took mum and the others",
        "We have to save them!",
        "But we are going to need reinforcements",
        "We should find Uncle Jeremy",
        "Come on Dad, Let's Go!"
    };

    bn::string_view FrogNPC::_dialogue_lines[4] = {
        "Ribbit",
        "Did you see who knocked over my crates?",
        "...",
        "Hm... Well if you do. Let me know."
    };

    bn::string_view GirlsNPC::_dialogue_lines[4] = {
        "My..",
        "My goodness..",
        "What are they doing to you?",
        "I have to get to the bottom of this"
    };

    bn::string_view LabPcNPC::_dialogue_lines[8] = {
        "They are..",
        "No.. They..",
        "...",
        "They are experimenting on cats.",
        "By modifying DNA, they are...",
        "No.. They can't..",
        "I have to get the kids out of here",
        "I have to find Eileen"
    };

    bn::string_view PotionNPC::_dialogue_lines[2] = {
        "Ew.. What is that?",
        "I should probably just leave it be."
    };

    bn::string_view ComputerStuffNPC::_dialogue_lines[1] = {
        "I wonder what they need all this for?"
    };

    bn::string_view PewPewNPC::_dialogue_lines[6] = {
        "They left this one unlocked",
        "Oooohh...",
        "It has Space Invaders",
        "pew pew",
        "pew pew ... pew",
        "pew"
    };

    bn::string_view FamNPC::_dialogue_lines[7] = {
        "Eileen!",
        "Thank goodness you are okay",
        "That was horrifying",
        "I was so scared that I had lost you",
        "All of you!",
        "...",
        "Let's get the others and get out of here"
    };

    bn::string_view MutantNPC::_dialogue_lines[6] = {
        "Eileen...",
        "Is that you?",
        "Kids?",
        "...",
        "...",
        "..."
    };

    bn::string_view MerchantNPC::_dialogue_lines[5] = {
        "Greetings, traveler!",
        "I have many wares for sale.",
        "Looking for anything specific today?",
        "The finest goods from across the realm!",
        "Come back anytime!"
    };

    GolemNPC::GolemNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator)
        : NPC(pos, camera, NPC_TYPE::GOLEM, text_generator)
    {
        initialize_sprite();
        initialize_dialogue();
    }

    void GolemNPC::initialize_sprite()
    {
        _sprite = bn::sprite_items::golem_sprite.create_sprite(_pos.x(), _pos.y());
        _action = bn::create_sprite_animate_action_forever(
            _sprite.value(), 120, bn::sprite_items::golem_sprite.tiles_item(), 0, 1);
        
        if (_sprite.has_value())
        {
            _sprite.value().set_camera(_camera);
            _sprite.value().set_bg_priority(1);
            _sprite.value().set_z_order(2);
        }
    }

    void GolemNPC::initialize_dialogue()
    {
        _lines = bn::span(_dialogue_lines);
    }

    TortoiseNPC::TortoiseNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator)
        : NPC(pos, camera, NPC_TYPE::TORTOISE, text_generator)
    {
        initialize_sprite();
        initialize_dialogue();
    }

    void TortoiseNPC::initialize_sprite()
    {
        _sprite = bn::sprite_items::tortoise_sprite.create_sprite(_pos.x(), _pos.y());
        _action = bn::create_sprite_animate_action_forever(
            _sprite.value(), 120, bn::sprite_items::tortoise_sprite.tiles_item(), 0, 1);
        
        if (_sprite.has_value())
        {
            _sprite.value().set_camera(_camera);
            _sprite.value().set_bg_priority(1);
            _sprite.value().set_z_order(2);
        }
    }

    void TortoiseNPC::initialize_dialogue()
    {
        _lines = bn::span(_dialogue_lines);
    }

    Tortoise2NPC::Tortoise2NPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator)
        : NPC(pos, camera, NPC_TYPE::TORTOISE2, text_generator)
    {
        initialize_sprite();
        initialize_dialogue();
    }

    void Tortoise2NPC::initialize_sprite()
    {
        _sprite = bn::sprite_items::tortoise_sprite.create_sprite(_pos.x(), _pos.y());
        _action = bn::create_sprite_animate_action_forever(
            _sprite.value(), 120, bn::sprite_items::tortoise_sprite.tiles_item(), 0, 1);
        
        if (_sprite.has_value())
        {
            _sprite.value().set_camera(_camera);
            _sprite.value().set_bg_priority(1);
            _sprite.value().set_z_order(2);
        }
    }

    void Tortoise2NPC::initialize_dialogue()
    {
        _lines = bn::span(_dialogue_lines);
    }

    PenguinNPC::PenguinNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator)
        : NPC(pos, camera, NPC_TYPE::PENGUIN, text_generator)
    {
        initialize_sprite();
        initialize_dialogue();
    }

    void PenguinNPC::initialize_sprite()
    {
        _sprite = bn::sprite_items::penguin_sprite.create_sprite(_pos.x(), _pos.y());
        _action = bn::create_sprite_animate_action_forever(
            _sprite.value(), 20, bn::sprite_items::penguin_sprite.tiles_item(), 0, 1);
        
        if (_sprite.has_value())
        {
            _sprite.value().set_camera(_camera);
            _sprite.value().set_bg_priority(1);
            _sprite.value().set_z_order(2);
        }
    }

    void PenguinNPC::initialize_dialogue()
    {
        _lines = bn::span(_dialogue_lines);
    }

    Penguin2NPC::Penguin2NPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator)
        : NPC(pos, camera, NPC_TYPE::PENGUIN2, text_generator)
    {
        initialize_sprite();
        initialize_dialogue();
    }

    void Penguin2NPC::initialize_sprite()
    {
        _sprite = bn::sprite_items::penguin_sprite.create_sprite(_pos.x(), _pos.y());
        _action = bn::create_sprite_animate_action_forever(
            _sprite.value(), 20, bn::sprite_items::penguin_sprite.tiles_item(), 0, 1);
        
        if (_sprite.has_value())
        {
            _sprite.value().set_camera(_camera);
            _sprite.value().set_bg_priority(1);
            _sprite.value().set_z_order(2);
        }
    }

    void Penguin2NPC::initialize_dialogue()
    {
        _lines = bn::span(_dialogue_lines);
    }

    TabletNPC::TabletNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator)
        : NPC(pos, camera, NPC_TYPE::TABLET, text_generator)
    {
        initialize_sprite();
        initialize_dialogue();
    }

    void TabletNPC::initialize_sprite()
    {
        _sprite = bn::sprite_items::stone_plaque.create_sprite(_pos.x(), _pos.y());
        _action = bn::create_sprite_animate_action_forever(
            _sprite.value(), 20, bn::sprite_items::stone_plaque.tiles_item(), 0, 1);
        
        if (_sprite.has_value())
        {
            _sprite.value().set_camera(_camera);
            _sprite.value().set_bg_priority(1);
            _sprite.value().set_z_order(2);
        }
    }

    void TabletNPC::initialize_dialogue()
    {
        _lines = bn::span(_dialogue_lines);
    }

    JeremyNPC::JeremyNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator)
        : NPC(pos, camera, NPC_TYPE::JEREMY, text_generator)
    {
        initialize_sprite();
        initialize_dialogue();
    }

    void JeremyNPC::initialize_sprite()
    {
        _sprite = bn::sprite_items::jeremy.create_sprite(_pos.x(), _pos.y());
        _action = bn::create_sprite_animate_action_forever(
            _sprite.value(), 120, bn::sprite_items::jeremy.tiles_item(), 0, 1);
        
        if (_sprite.has_value())
        {
            _sprite.value().set_camera(_camera);
            _sprite.value().set_bg_priority(1);
            _sprite.value().set_z_order(2);
        }
    }

    void JeremyNPC::initialize_dialogue()
    {
        _lines = bn::span(_dialogue_lines);
    }

    CageNPC::CageNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator)
        : NPC(pos, camera, NPC_TYPE::CAGE, text_generator)
    {
        initialize_sprite();
        initialize_dialogue();
    }

    void CageNPC::initialize_sprite()
    {
        _sprite = bn::sprite_items::cage.create_sprite(_pos.x(), _pos.y());
        _action = bn::create_sprite_animate_action_forever(
            _sprite.value(), 120, bn::sprite_items::cage.tiles_item(), 0, 1);
        
        if (_sprite.has_value())
        {
            _sprite.value().set_camera(_camera);
            _sprite.value().set_bg_priority(1);
            _sprite.value().set_z_order(2);
        }
    }

    void CageNPC::initialize_dialogue()
    {
        _lines = bn::span(_dialogue_lines);
    }

    FrogNPC::FrogNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator)
        : NPC(pos, camera, NPC_TYPE::FROG, text_generator)
    {
        initialize_sprite();
        initialize_dialogue();
    }

    void FrogNPC::initialize_sprite()
    {
        _sprite = bn::sprite_items::pot_frog.create_sprite(_pos.x(), _pos.y());
        _action = bn::create_sprite_animate_action_forever(
            _sprite.value(), 120, bn::sprite_items::pot_frog.tiles_item(), 0, 1);
        
        if (_sprite.has_value())
        {
            _sprite.value().set_camera(_camera);
            _sprite.value().set_bg_priority(1);
            _sprite.value().set_z_order(2);
        }
    }

    void FrogNPC::initialize_dialogue()
    {
        _lines = bn::span(_dialogue_lines);
    }

    FamNPC::FamNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator)
        : NPC(pos, camera, NPC_TYPE::FAM, text_generator)
    {
        initialize_sprite();
        initialize_dialogue();
    }

    void FamNPC::initialize_sprite()
    {
        _sprite = bn::sprite_items::fam.create_sprite(_pos.x(), _pos.y());
        _action = bn::create_sprite_animate_action_forever(
            _sprite.value(), 20, bn::sprite_items::fam.tiles_item(), 0, 1);
        
        if (_sprite.has_value())
        {
            _sprite.value().set_camera(_camera);
            _sprite.value().set_bg_priority(1);
            _sprite.value().set_z_order(2);
        }
    }

    void FamNPC::initialize_dialogue()
    {
        _lines = bn::span(_dialogue_lines);
    }

    MutantNPC::MutantNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator)
        : NPC(pos, camera, NPC_TYPE::MUTANT, text_generator)
    {
        initialize_sprite();
        initialize_dialogue();
    }

    void MutantNPC::initialize_sprite()
    {
        _sprite = bn::sprite_items::mutant.create_sprite(_pos.x(), _pos.y() - 4);
        _sprite.value().set_horizontal_flip(true);
        _action = bn::create_sprite_animate_action_forever(
            _sprite.value(), 20, bn::sprite_items::mutant.tiles_item(), 0, 0);
        
        if (_sprite.has_value())
        {
            _sprite.value().set_camera(_camera);
            _sprite.value().set_bg_priority(1);
            _sprite.value().set_z_order(2);
        }
    }

    void MutantNPC::initialize_dialogue()
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
        _sprite = bn::sprite_items::merchant.create_sprite(_pos.x(), _pos.y());
        _action = bn::create_sprite_animate_action_forever(
            _sprite.value(), 20, bn::sprite_items::merchant.tiles_item(), 0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
        
        if (_sprite.has_value())
        {
            _sprite.value().set_camera(_camera);
            _sprite.value().set_bg_priority(1);
            _sprite.value().set_z_order(2);
        }
    }

    void MerchantNPC::initialize_dialogue()
    {
        _lines = bn::span(_dialogue_lines);
    }

    GirlsNPC::GirlsNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator)
        : NPC(pos, camera, NPC_TYPE::GIRLS, text_generator)
    {
        initialize_sprite();
        initialize_dialogue();
    }

    void GirlsNPC::initialize_sprite()
    {
    }

    void GirlsNPC::initialize_dialogue()
    {
        _lines = bn::span(_dialogue_lines);
    }

    LabPcNPC::LabPcNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator)
        : NPC(pos, camera, NPC_TYPE::LAB_PC, text_generator)
    {
        initialize_sprite();
        initialize_dialogue();
    }

    void LabPcNPC::initialize_sprite()
    {
    }

    void LabPcNPC::initialize_dialogue()
    {
        _lines = bn::span(_dialogue_lines);
    }

    PotionNPC::PotionNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator)
        : NPC(pos, camera, NPC_TYPE::POTION, text_generator)
    {
        initialize_sprite();
        initialize_dialogue();
    }

    void PotionNPC::initialize_sprite()
    {
    }

    void PotionNPC::initialize_dialogue()
    {
        _lines = bn::span(_dialogue_lines);
    }

    ComputerStuffNPC::ComputerStuffNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator)
        : NPC(pos, camera, NPC_TYPE::COMPUTER_STUFF, text_generator)
    {
        initialize_sprite();
        initialize_dialogue();
    }

    void ComputerStuffNPC::initialize_sprite()
    {
    }

    void ComputerStuffNPC::initialize_dialogue()
    {
        _lines = bn::span(_dialogue_lines);
    }

    PewPewNPC::PewPewNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator)
        : NPC(pos, camera, NPC_TYPE::PEWPEW, text_generator)
    {
        initialize_sprite();
        initialize_dialogue();
    }

    void PewPewNPC::initialize_sprite()
    {
    }

    void PewPewNPC::initialize_dialogue()
    {
        _lines = bn::span(_dialogue_lines);
    }

    NPC* create_npc(bn::fixed_point pos, bn::camera_ptr &camera, NPC_TYPE type, bn::sprite_text_generator &text_generator)
    {
        switch (type)
        {
            case NPC_TYPE::GOLEM:
                return new GolemNPC(pos, camera, text_generator);
            case NPC_TYPE::TORTOISE:
                return new TortoiseNPC(pos, camera, text_generator);
            case NPC_TYPE::TORTOISE2:
                return new Tortoise2NPC(pos, camera, text_generator);
            case NPC_TYPE::PENGUIN:
                return new PenguinNPC(pos, camera, text_generator);
            case NPC_TYPE::PENGUIN2:
                return new Penguin2NPC(pos, camera, text_generator);
            case NPC_TYPE::TABLET:
                return new TabletNPC(pos, camera, text_generator);
            case NPC_TYPE::JEREMY:
                return new JeremyNPC(pos, camera, text_generator);
            case NPC_TYPE::CAGE:
                return new CageNPC(pos, camera, text_generator);
            case NPC_TYPE::FROG:
                return new FrogNPC(pos, camera, text_generator);
            case NPC_TYPE::GIRLS:
                return new GirlsNPC(pos, camera, text_generator);
            case NPC_TYPE::LAB_PC:
                return new LabPcNPC(pos, camera, text_generator);
            case NPC_TYPE::POTION:
                return new PotionNPC(pos, camera, text_generator);
            case NPC_TYPE::COMPUTER_STUFF:
                return new ComputerStuffNPC(pos, camera, text_generator);
            case NPC_TYPE::PEWPEW:
                return new PewPewNPC(pos, camera, text_generator);
            case NPC_TYPE::FAM:
                return new FamNPC(pos, camera, text_generator);
            case NPC_TYPE::MUTANT:
                return new MutantNPC(pos, camera, text_generator);
            case NPC_TYPE::MERCHANT:
                return new MerchantNPC(pos, camera, text_generator);
            default:
                return nullptr;
        }
    }
}
