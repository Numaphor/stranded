#ifndef FE_HUD_H
#define FE_HUD_H

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

    class HUD
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

        bn::optional<bn::sprite_animate_action<10>> _soul_action;

        // Ammo display - single sprite with multiple frames (0-10 ammo states)
        bn::optional<bn::sprite_ptr> _ammo_sprite;
        int _displayed_ammo = 10;

    public:
        HUD();

        int hp();
        void set_hp(int hp);
        void set_visible(bool is_visible);
        void set_soul_position(int x, int y);

        // Ammo management
        void set_ammo(int ammo_count);
        void update_ammo_display();
        void activate_soul_animation();   // Trigger soul animation for defense buff
        void deactivate_soul_animation(); // Deactivate defense buff when healing
        void play_soul_damage_animation(); // Play reverse soul animation when taking damage
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