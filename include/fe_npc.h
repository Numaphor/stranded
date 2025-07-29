#ifndef BF_NPC_H
#define BF_NPC_H

#include "bn_fixed_point.h"
#include "bn_sprite_ptr.h"
#include "bn_camera_ptr.h"
#include "bn_optional.h"
#include "bn_span.h"
#include "bn_vector.h"
#include "bn_display.h"
#include "bn_string.h"
#include "bn_string_view.h"
#include "bn_sprite_animate_actions.h"

#include "bn_sprite_text_generator.h"

#include "fe_npc_type.h"
#include "fe_entity.h"

namespace fe
{
    class NPC : public Entity
    {
    protected:
        NPC_TYPE _type;
        bn::camera_ptr &_camera;
        bn::optional<bn::sprite_animate_action<10>> _action;
        bool _is_talking = false;
        bool _is_near_player = false;
        bool _finished = false;
        bool _has_spoken_once = false;
        bool _hidden = false;

        bn::vector<bn::sprite_ptr, 32> _text_sprites;
        bn::sprite_text_generator &_text_generator;

        const bn::fixed _text_y_inc = 14;
        const bn::fixed _text_y_limit = (bn::display::height() / 2) - _text_y_inc;

        bn::span<bn::string_view> _lines;
        int _currentLine = 0;
        int _currentChar = 0;
        int _last_char_count = -1;
        bn::string_view _currentChars = "";

    public:
        NPC(bn::fixed_point pos, bn::camera_ptr &camera, NPC_TYPE type, bn::sprite_text_generator &text_generator);
        virtual ~NPC() = default;

        // Override Entity methods
        void update() override;
        void update_hitbox() override; // Override to center NPC hitbox properly

        // NPC-specific methods
        bool is_in_interaction_zone(bn::fixed_point player_pos);
        bool check_trigger(bn::fixed_point player_pos); // Legacy method that calls is_in_interaction_zone
        bool is_talking();
        void talk();
        bool finished_talking();
        void set_is_hidden(bool is_hidden);
        bool hidden();
        NPC_TYPE type() const { return _type; }

    private:
        // Private helper methods
        void end_conversation();

    protected:
        // Virtual methods for derived classes to override
        virtual void initialize_sprite() {}
        virtual void initialize_dialogue() {}
    };
}

#endif
