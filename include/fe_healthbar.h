#ifndef FE_HEALTHBAR_H
#define FE_HEALTHBAR_H

#include "bn_sprite_ptr.h"
#include "bn_optional.h"
#include "bn_sprite_animate_actions.h"
#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_map_ptr.h"

namespace fe
{
    enum class WEAPON_TYPE
    {
        CLAW
    };

    class Healthbar
    {
    private:
        bn::optional<bn::regular_bg_ptr> _health_bg;
        int _hp = 2;
        bool _is_visible = true;
        WEAPON_TYPE _weapon;
        bn::sprite_ptr _weapon_sprite;
        bn::sprite_ptr _soul_sprite;
        bool _is_glowing = false;
        bool _soul_effect_active = false;
        int _soul_effect_timer = 0;

        bn::optional<bn::sprite_animate_action<10>> _action;
        bn::optional<bn::sprite_animate_action<5>> _soul_action;

    public:
        Healthbar();

        int hp();
        void set_hp(int hp);
        void set_visible(bool is_visible);
        void set_soul_position(int x, int y);
        void debug_soul_center(); // Center soul sprite for debugging
        void debug_soul_animate(); // Animate soul sprite for debugging
        void activate_soul_animation(); // Trigger soul animation for defense buff
        void activate_glow();
        bool is_glow_active();
        void update();
        bool is_glow_ready();
    };
}

#endif