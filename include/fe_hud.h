#ifndef FE_HUD_H
#define FE_HUD_H

#include "bn_sprite_ptr.h"
#include "bn_optional.h"
#include "bn_sprite_animate_actions.h"
#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_map_ptr.h"

namespace fe
{
    /**
     * @brief Weapon types available to the player
     */
    enum class WEAPON_TYPE
    {
        GUN,
        SWORD
    };

    /**
     * @brief Buff menu states
     */
    enum class BUFF_MENU_STATE
    {
        CLOSED,
        OPEN
    };

    /**
     * @brief Buff menu option indices for the 2-buff menu
     */
    enum class BUFF_OPTION
    {
        ENERGY = 0, // Energy buff
        POWER = 1   // Power buff
    };

    /**
     * @brief Heads-Up Display manager for the game
     *
     * Manages the on-screen HUD elements including:
     * - Healthbar display (background-based with multiple map states)
     * - Soul indicator (animated sprite showing buff states)
     * - Weapon icon (current equipped weapon)
     * - Ammo counter (displayed when gun is equipped)
     * - Buff menu (temptest sprites for buff selection)
     *
     * All HUD elements are positioned in screen-space (not affected by camera)
     */
    class HUD
    {
    public:
        HUD();

        // Healthbar management
        [[nodiscard]] int hp() const;
        void set_hp(int hp);
        void set_position(int x, int y);                       // Set healthbar position (soul follows)
        void set_resetting_health(bool resetting);             // Prevent soul animations during reset
        [[nodiscard]] bool is_soul_animation_complete() const; // Check if soul animation is done

        // Visibility control
        void set_visible(bool is_visible);

        // Soul animation (buff indicators)
        void activate_soul_animation();    // Defense buff visual
        void deactivate_soul_animation();  // Return to idle state
        void play_soul_damage_animation(); // Visual feedback when taking damage
        void activate_silver_soul();       // Energy buff visual
        void deactivate_silver_soul();     // Return from energy buff

        // Health transition animations
        void play_health_gain_0_to_1();    // Animation for 0->1 health gain
        void play_health_gain_1_to_2();    // Animation for 1->2 health gain
        void play_health_gain_2_to_3();    // Animation for 2->3 health gain
        void play_health_loss_3_to_2();    // Animation for 3->2 health loss
        void play_health_loss_2_to_1();    // Animation for 2->1 health loss
        void play_health_loss_1_to_0();    // Animation for 1->0 health loss
        void play_health_loss_animation(); // Generic health loss animation

        // Main update loop
        void update();

        // Weapon management
        void set_weapon(WEAPON_TYPE weapon);
        void set_weapon_frame(int frame);
        [[nodiscard]] WEAPON_TYPE get_weapon() const;
        void cycle_weapon();

        // Ammo management
        void set_ammo(int ammo_count);

        // Buff menu management
        void toggle_buff_menu();
        void navigate_buff_menu_up();
        void navigate_buff_menu_down();
        void navigate_buff_menu_left();
        void navigate_buff_menu_right();
        [[nodiscard]] bool is_buff_menu_open() const;
        [[nodiscard]] int get_selected_buff() const;

        // Buff menu hold activation
        void start_buff_menu_hold();                           // Start hold animation
        void update_buff_menu_hold();                          // Update hold progress and animation
        void cancel_buff_menu_hold();                          // Cancel hold and reset animation
        [[nodiscard]] bool is_buff_menu_hold_complete() const; // Check if hold duration reached
        [[nodiscard]] bool is_buff_menu_holding() const;       // Check if currently holding

        // Buff menu cooldown (after buff activation)
        void start_buff_menu_cooldown();                     // Start cooldown after buff activation
        void update_buff_menu_cooldown();                    // Update cooldown animation (call in update())
        [[nodiscard]] bool is_buff_menu_on_cooldown() const; // Check if menu is on cooldown

    private:
        // Healthbar display
        bn::optional<bn::regular_bg_ptr> _health_bg;
        int _hp;

        // Global visibility state
        bool _is_visible;

        // Weapon display
        WEAPON_TYPE _weapon;
        bn::sprite_ptr _weapon_sprite;

        // Soul indicator
        bn::sprite_ptr _soul_sprite;
        bn::optional<bn::sprite_animate_action<16>> _soul_action;
        bool _soul_positioned;

        // Soul buff states
        bool _defense_buff_active;
        bool _defense_buff_fading;
        bool _silver_soul_active;
        bool _silver_soul_reversing;
        int _silver_idle_timer;

        // Health transition states
        bool _health_gain_anim_active;
        bool _health_loss_anim_active;
        bool _resetting_health;

        // Ammo display
        bn::optional<bn::sprite_ptr> _ammo_sprite;
        int _displayed_ammo;

        // Buff menu system
        BUFF_MENU_STATE _buff_menu_state;
        bn::sprite_ptr _buff_menu_base;
        bn::optional<bn::sprite_ptr> _buff_menu_option_sprites[3]; // Heal, Energy, Power
        int _selected_buff_option;                                 // 0-2 for the three options
        int _buff_menu_hold_timer;                                 // Hold timer for L button activation
        int _buff_menu_cooldown_timer;                             // Cooldown timer after buff activation

        // Private helper methods
        void _configure_hud_sprite(bn::sprite_ptr &sprite);
        void _update_soul_animations();
        void _update_soul_position();
        void _update_ammo_display();
        void _update_buff_menu_sprites();
        void _update_selection(int new_selection);
        void _play_health_transition_anim(const bn::sprite_item &sprite_item, const int *frames, int frame_count, bool is_gain);
    };
}

#endif