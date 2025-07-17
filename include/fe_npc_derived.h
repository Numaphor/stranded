#ifndef FE_NPC_DERIVED_H
#define FE_NPC_DERIVED_H

#include "fe_npc.h"
#include "bn_sprite_items_golem_sprite.h"
#include "bn_sprite_items_tortoise_sprite.h"
#include "bn_sprite_items_penguin_sprite.h"
#include "bn_sprite_items_jeremy.h"
#include "bn_sprite_items_pot_frog.h"
#include "bn_sprite_items_cage.h"
#include "bn_sprite_items_fam.h"
#include "bn_sprite_items_mutant.h"
#include "bn_sprite_items_stone_plaque.h"
#include "bn_sprite_items_merchant.h"

namespace fe
{
    // Golem NPC
    class GolemNPC : public NPC
    {
    public:
        GolemNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator);

    protected:
        void initialize_sprite() override;
        void initialize_dialogue() override;

    private:
        static bn::string_view _dialogue_lines[14];
    };

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

    // Tortoise2 NPC
    class Tortoise2NPC : public NPC
    {
    public:
        Tortoise2NPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator);

    protected:
        void initialize_sprite() override;
        void initialize_dialogue() override;

    private:
        static bn::string_view _dialogue_lines[9];
    };

    // Penguin NPC
    class PenguinNPC : public NPC
    {
    public:
        PenguinNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator);

    protected:
        void initialize_sprite() override;
        void initialize_dialogue() override;

    private:
        static bn::string_view _dialogue_lines[3];
    };

    // Penguin2 NPC
    class Penguin2NPC : public NPC
    {
    public:
        Penguin2NPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator);

    protected:
        void initialize_sprite() override;
        void initialize_dialogue() override;

    private:
        static bn::string_view _dialogue_lines[3];
    };

    // Tablet NPC
    class TabletNPC : public NPC
    {
    public:
        TabletNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator);

    protected:
        void initialize_sprite() override;
        void initialize_dialogue() override;

    private:
        static bn::string_view _dialogue_lines[4];
    };

    // Jeremy NPC
    class JeremyNPC : public NPC
    {
    public:
        JeremyNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator);

    protected:
        void initialize_sprite() override;
        void initialize_dialogue() override;

    private:
        static bn::string_view _dialogue_lines[15];
    };

    // Cage NPC
    class CageNPC : public NPC
    {
    public:
        CageNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator);

    protected:
        void initialize_sprite() override;
        void initialize_dialogue() override;

    private:
        static bn::string_view _dialogue_lines[7];
    };

    // Frog NPC
    class FrogNPC : public NPC
    {
    public:
        FrogNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator);

    protected:
        void initialize_sprite() override;
        void initialize_dialogue() override;

    private:
        static bn::string_view _dialogue_lines[4];
    };

    // Girls NPC (dialogue only)
    class GirlsNPC : public NPC
    {
    public:
        GirlsNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator);

    protected:
        void initialize_sprite() override;
        void initialize_dialogue() override;

    private:
        static bn::string_view _dialogue_lines[4];
    };

    // Lab PC NPC (dialogue only)
    class LabPcNPC : public NPC
    {
    public:
        LabPcNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator);

    protected:
        void initialize_sprite() override;
        void initialize_dialogue() override;

    private:
        static bn::string_view _dialogue_lines[8];
    };

    // Potion NPC (dialogue only)
    class PotionNPC : public NPC
    {
    public:
        PotionNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator);

    protected:
        void initialize_sprite() override;
        void initialize_dialogue() override;

    private:
        static bn::string_view _dialogue_lines[2];
    };

    // Computer Stuff NPC (dialogue only)
    class ComputerStuffNPC : public NPC
    {
    public:
        ComputerStuffNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator);

    protected:
        void initialize_sprite() override;
        void initialize_dialogue() override;

    private:
        static bn::string_view _dialogue_lines[1];
    };

    // PewPew NPC (dialogue only)
    class PewPewNPC : public NPC
    {
    public:
        PewPewNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator);

    protected:
        void initialize_sprite() override;
        void initialize_dialogue() override;

    private:
        static bn::string_view _dialogue_lines[6];
    };

    // Fam NPC
    class FamNPC : public NPC
    {
    public:
        FamNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator);

    protected:
        void initialize_sprite() override;
        void initialize_dialogue() override;

    private:
        static bn::string_view _dialogue_lines[7];
    };

    // Mutant NPC
    class MutantNPC : public NPC
    {
    public:
        MutantNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator);

    protected:
        void initialize_sprite() override;
        void initialize_dialogue() override;

    private:
        static bn::string_view _dialogue_lines[6];
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

    // Factory function to create NPCs based on type
    NPC* create_npc(bn::fixed_point pos, bn::camera_ptr &camera, NPC_TYPE type, bn::sprite_text_generator &text_generator);
}

#endif
