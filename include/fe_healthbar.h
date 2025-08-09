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
        GUN,
        SWORD
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
        bool _soul_effect_active = false;
        int _soul_effect_timer = 0;
        bool _soul_fade_out_active = false;
        bool _silver_soul_active = false;
        int _silver_soul_timer = 0;
        int _silver_idle_timer = 0;
        bool _silver_soul_reversing = false;

        bn::optional<bn::sprite_animate_action<8>> _soul_action;

    public:
        Healthbar();

        int hp();
        void set_hp(int hp);
        void set_visible(bool is_visible);
        void set_soul_position(int x, int y);
        void debug_soul_center();         // Center soul sprite for debugging
        void debug_soul_animate();        // Animate soul sprite for debugging
        void activate_soul_animation();   // Trigger soul animation for defense buff
        void deactivate_soul_animation(); // Deactivate defense buff when healing
        void activate_silver_soul();      // Trigger silver soul animation for energy buff
        void deactivate_silver_soul();    // Return to normal soul when healing
        void update();

        // Weapon management
        void set_weapon(WEAPON_TYPE weapon);
        void set_weapon_frame(int frame); // Update weapon sprite frame
        WEAPON_TYPE get_weapon() const;
        void cycle_weapon(); // Cycle between available weapons
    };
}

#endif