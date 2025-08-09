#include "fe_hud.h"
#include "bn_fixed.h"
#include "bn_sprite_ptr.h"
#include "bn_log.h"
#include "bn_math.h"

#include "bn_regular_bg_items_health.h"
#include "bn_sprite_items_soul.h"
#include "bn_sprite_items_gun.h"
#include "bn_sprite_items_icon_gun.h"
#include "bn_sprite_items_soul_silver.h"
#include "bn_sprite_items_soul_silver_idle.h"
#include "bn_sprite_items_ammo.h"

namespace fe
{
    constexpr int OFFSCREEN_POSITION_X = -500; // Off-screen X position for weapon sprite
    constexpr int OFFSCREEN_POSITION_Y = -500; // Off-screen Y position for weapon sprite
    constexpr int MIN_Z_ORDER = -32767;        // Minimum z-order value for backgrounds/sprites

    HUD::HUD() : _weapon(WEAPON_TYPE::SWORD),
                 _weapon_sprite(bn::sprite_items::icon_gun.create_sprite(100, 66, 0)),
                 _soul_sprite(bn::sprite_items::soul.create_sprite(-200, -150, 0)) // More visible position for debugging
    {
        // Create GUI HUD with maximum priority
        _health_bg = bn::regular_bg_items::health.create_bg(-262, -215, 2);
        _health_bg->set_priority(0);
        _health_bg->set_z_order(MIN_Z_ORDER);
        _health_bg->put_above();
        _health_bg->remove_camera();
        _health_bg->set_visible(true);

        _weapon_sprite.set_bg_priority(0);
        _weapon_sprite.remove_camera();
        _weapon_sprite.set_visible(true);   // Make weapon sprite visible
        _weapon_sprite.set_z_order(-32000); // High priority to ensure visibility

        // Position soul sprite over HUD with sprite priority
        _soul_sprite.set_bg_priority(0);
        _soul_sprite.set_z_order(-32000); // Maximum priority for sprite
        _soul_sprite.remove_camera();
        _soul_sprite.set_visible(true); // Explicitly make visible

        // Initialize ammo display (single sprite with frames 0-10 for different ammo counts)
        _ammo_sprite = bn::sprite_items::ammo.create_sprite(100, 77, 0); // Frame 0 = full ammo (inverted)
        _ammo_sprite->set_bg_priority(0);
        _ammo_sprite->remove_camera();
        _ammo_sprite->set_z_order(-32000);
        _ammo_sprite->set_visible(false); // Start hidden since we start with sword

        // Log dimensions and positions for debugging
        BN_LOG("HUD bg position: (-262, -215)");
        BN_LOG("Soul sprite position: (-200, -150) - DEBUG POSITION");
        BN_LOG("Weapon sprite: Gun icon at (100, 66) - moved up 4 pixels");
        BN_LOG("Soul sprite size: 16x16 pixels (from json height: 16)");
        BN_LOG("Ammo sprite: Single sprite at (100, 77) with 11 frames (0-10 ammo states)");
    }

    int HUD::hp()
    {
        return _hp;
    }

    void HUD::set_hp(int hp)
    {
        _hp = bn::max(0, bn::min(2, hp));

        if (_health_bg.has_value())
        {
            _health_bg->set_map(bn::regular_bg_items::health.map_item(), _hp);
        }
    }

    void HUD::set_visible(bool is_visible)
    {
        _is_visible = is_visible;

        if (_health_bg.has_value())
        {
            _health_bg->set_visible(is_visible);
        }

        _weapon_sprite.set_visible(is_visible);
        _soul_sprite.set_visible(is_visible);

        // Update ammo sprite visibility
        if (_ammo_sprite.has_value())
        {
            _ammo_sprite->set_visible(is_visible && _weapon == WEAPON_TYPE::GUN && _displayed_ammo > 0);
        }
    }

    void HUD::set_soul_position(int x, int y)
    {
        _soul_sprite.set_position(x, y);
        BN_LOG("Soul sprite repositioned to: (", x, ", ", y, ")");
    }

    void HUD::debug_soul_center()
    {
        _soul_sprite.set_position(0, 0); // Center of screen
        _soul_sprite.set_z_order(-2000); // Very high priority
        _soul_sprite.set_visible(true);
        BN_LOG("Soul sprite moved to screen center (0, 0) for debugging");
    }

    void HUD::debug_soul_animate()
    {
        // Move soul sprite in a circle pattern to make it very visible
        static int frame_counter = 0;
        frame_counter++;

        // Use simple sin/cos calculation with Butano fixed point
        bn::fixed angle = frame_counter * 0.1;
        int x = (100 * bn::sin(angle)).integer();
        int y = (100 * bn::cos(angle)).integer() + 8; // Move circle down by 8 pixels

        _soul_sprite.set_position(x, y);
        _soul_sprite.set_z_order(-32000);
        _soul_sprite.set_visible(true);

        if (frame_counter % 60 == 0)
        {
            BN_LOG("Soul sprite animating at: (", x, ", ", y, ")");
        }
    }

    void HUD::activate_soul_animation()
    {
        // Start defense buff soul effect (permanent until heal, like silver soul)
        _soul_effect_active = true;
        _soul_effect_timer = -1; // Set to -1 to indicate permanent state

        // Animate soul sprite frames 1-4 for defense buff (SELECT + DOWN)
        _soul_action = bn::create_sprite_animate_action_once(
            _soul_sprite, 8, bn::sprite_items::soul.tiles_item(), 1, 2, 3, 4);
        BN_LOG("Soul animation triggered for defense buff - permanent until heal");
    }

    void HUD::activate_silver_soul()
    {
        // Start silver soul transformation (permanent until heal)
        _silver_soul_active = true;
        _silver_soul_timer = -1; // Set to -1 to indicate permanent state
        _silver_idle_timer = 0;  // Reset idle timer

        // Switch to silver soul sprite and play transformation animation with all 8 frames
        _soul_sprite.set_item(bn::sprite_items::soul_silver);

        // Create frame sequence for 8-frame transformation (0-7)
        bn::vector<uint16_t, 8> frames;
        for (int i = 0; i <= 7; ++i)
        {
            frames.push_back(i);
        }

        _soul_action = bn::sprite_animate_action<8>::once(
            _soul_sprite, 8, bn::sprite_items::soul_silver.tiles_item(),
            bn::span<const uint16_t>(frames.data(), frames.size()));
        BN_LOG("Silver soul transformation triggered for energy buff - permanent until heal");
    }

    void HUD::deactivate_silver_soul()
    {
        if (_silver_soul_active)
        {
            // Create reverse frame sequence for 8-frame transformation (7-0)
            bn::vector<uint16_t, 8> frames;
            for (int i = 7; i >= 0; --i)
            {
                frames.push_back(i);
            }

            // Play reverse transformation animation when healing
            _soul_action = bn::sprite_animate_action<8>::once(
                _soul_sprite, 8, bn::sprite_items::soul_silver.tiles_item(),
                bn::span<const uint16_t>(frames.data(), frames.size()));
            _silver_soul_active = false;
            _silver_soul_reversing = true; // Flag to indicate reverse animation is playing
            _silver_idle_timer = 0;
            BN_LOG("Silver soul deactivated - playing reverse transformation due to heal");
        }
    }

    void HUD::deactivate_soul_animation()
    {
        if (_soul_effect_active)
        {
            // Start fade-out with reversed animation (frames 4-3-2-1) to return to idle
            _soul_action = bn::create_sprite_animate_action_once(
                _soul_sprite, 8, bn::sprite_items::soul.tiles_item(), 4, 3, 2, 1);
            _soul_effect_active = false;
            _soul_fade_out_active = true;
            BN_LOG("Defence buff soul deactivated - playing fade-out animation due to heal");
        }
    }

    void HUD::update()
    {
        // Handle defence buff soul effect - now permanent like silver soul, no timer countdown
        if (_soul_effect_active)
        {
            // Defence buff stays active until heal - no timer check for deactivation
            // Animation will complete and stay on frame 4 until manually deactivated
        }

        // Handle silver soul effect duration and random idle animations
        if (_silver_soul_active)
        {
            _silver_idle_timer++;

            // Check if transformation animation is done and we should start looping idle animations
            if (_soul_action.has_value() && _soul_action.value().done())
            {
                // Play silver idle animation on loop every 120 frames (2 seconds)
                if (_silver_idle_timer % 120 == 0)
                {
                    _soul_sprite.set_item(bn::sprite_items::soul_silver_idle);
                    _soul_action = bn::create_sprite_animate_action_once(
                        _soul_sprite, 10, bn::sprite_items::soul_silver_idle.tiles_item(), 0, 1, 2, 1, 0);
                    BN_LOG("Playing looping silver soul idle animation");
                }
            }

            // Silver soul stays active until heal - no timer check for deactivation
        }

        // Update soul animation if active
        if (_soul_action.has_value() && !_soul_action.value().done())
        {
            _soul_action.value().update();
        }

        // Handle reverse transformation completion
        if (_silver_soul_reversing && _soul_action.has_value() && _soul_action.value().done())
        {
            // Reverse animation completed, switch back to normal soul sprite
            _soul_sprite.set_item(bn::sprite_items::soul);
            _soul_sprite.set_tiles(bn::sprite_items::soul.tiles_item().create_tiles(0));
            _soul_action.reset();
            _silver_soul_reversing = false;
            BN_LOG("Reverse silver soul transformation completed - switched to normal soul");
        }

        // Handle defence buff fade-out completion
        if (_soul_fade_out_active && _soul_action.has_value() && _soul_action.value().done())
        {
            // Fade-out animation completed, reset soul sprite to frame 0 (idle)
            _soul_sprite.set_tiles(bn::sprite_items::soul.tiles_item().create_tiles(0));
            _soul_action.reset();
            _soul_fade_out_active = false;
            BN_LOG("Defence buff fade-out completed - soul sprite reset to idle frame 0");
        }

        // Position soul sprite at the optimal HUD overlay position
        // Based on user feedback: from top of circle, first position to the left
        // This corresponds to 90 degrees (π/2 radians) in the circle
        static bool soul_positioned = false;
        if (!soul_positioned)
        {
            bn::fixed angle = 1.57;
            int x = (100 * bn::sin(angle)).integer() - 59;
            int y = (100 * bn::cos(angle)).integer() + 22;
            _soul_sprite.set_position(x, y);
            BN_LOG("Soul sprite positioned at optimal HUD location: (", x, ", ", y, ")");
            soul_positioned = true;
        }
    }

    void HUD::set_weapon(WEAPON_TYPE weapon)
    {
        if (_weapon != weapon)
        {
            _weapon = weapon;

            // Update weapon sprite based on type and place in bottom right of screen
            if (_weapon == WEAPON_TYPE::GUN)
            {
                _weapon_sprite = bn::sprite_items::icon_gun.create_sprite(100, 66, 0); // Bottom right of screen (moved up 4 pixels)
            }
            else if (_weapon == WEAPON_TYPE::SWORD)
            {
                // Use a dedicated placeholder sprite for sword until real sword sprite is available
                // Replace 'sword_placeholder' with the actual sword sprite item when ready
                _weapon_sprite = bn::sprite_items::icon_gun.create_sprite(100, 66, 0); // Bottom right of screen (moved up 4 pixels)
            }

            // Reapply sprite settings - make weapon sprite visible in bottom right
            _weapon_sprite.set_bg_priority(0);
            _weapon_sprite.remove_camera();
            _weapon_sprite.set_visible(true);   // Make weapon sprite visible
            _weapon_sprite.set_z_order(-32000); // High priority to ensure visibility

            // Update ammo display visibility based on weapon type
            update_ammo_display();
        }
    }

    void HUD::set_weapon_frame(int frame)
    {
        // Update the weapon sprite frame to match the player's current weapon frame
        if (_weapon == WEAPON_TYPE::GUN)
        {
            _weapon_sprite.set_tiles(bn::sprite_items::icon_gun.tiles_item(), frame);
            BN_LOG("Updated gun icon frame to: ", frame);
        }
        // Sword frame updates can be added here when sword sprite is available
    }

    WEAPON_TYPE HUD::get_weapon() const
    {
        return _weapon;
    }

    void HUD::cycle_weapon()
    {
        if (_weapon == WEAPON_TYPE::GUN)
        {
            set_weapon(WEAPON_TYPE::SWORD);
        }
        else
        {
            set_weapon(WEAPON_TYPE::GUN);
        }
    }

    void HUD::set_ammo(int ammo_count)
    {
        _displayed_ammo = bn::max(0, bn::min(ammo_count, 10));
        update_ammo_display();
    }

    void HUD::update_ammo_display()
    {
        if (!_ammo_sprite)
            return;

        // Show/hide ammo sprite based on weapon type and update frame
        bool show_ammo = (_weapon == WEAPON_TYPE::GUN);

        if (show_ammo)
        {
            // Invert frame: 10 ammo shows frame 0, 0 ammo shows frame 10
            int frame = 10 - _displayed_ammo;
            _ammo_sprite->set_tiles(bn::sprite_items::ammo.tiles_item(), frame);
            _ammo_sprite->set_visible(_is_visible);
        }
        else
        {
            _ammo_sprite->set_visible(false);
        }

        BN_LOG("Ammo display updated: ", _displayed_ammo, "/10 rounds, frame: ", (10 - _displayed_ammo), ", weapon: ",
               (_weapon == WEAPON_TYPE::GUN ? "GUN" : "SWORD"));
    }
}