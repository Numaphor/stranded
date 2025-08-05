#include "fe_healthbar.h"
#include "bn_fixed.h"
#include "bn_sprite_ptr.h"
#include "bn_log.h"
#include "bn_math.h"

#include "bn_regular_bg_items_health.h"
#include "bn_sprite_items_weapon_claw.h"
#include "bn_sprite_items_soul.h"

namespace fe
{
    constexpr int OFFSCREEN_POSITION_X = -500;           // Off-screen X position for weapon sprite
    constexpr int OFFSCREEN_POSITION_Y = -500;           // Off-screen Y position for weapon sprite
    constexpr int MIN_Z_ORDER = -32767;                  // Minimum z-order value for backgrounds/sprites
    const constexpr int weapon_x = OFFSCREEN_POSITION_X; // Move far off-screen to left
    const constexpr int weapon_y = OFFSCREEN_POSITION_Y; // Move far off-screen to top
    Healthbar::Healthbar() : _weapon(WEAPON_TYPE::CLAW),
                             _weapon_sprite(bn::sprite_items::weapon_claw.create_sprite(OFFSCREEN_POSITION_X - 6, OFFSCREEN_POSITION_Y, 0)),
                             _soul_sprite(bn::sprite_items::soul.create_sprite(-200, -150, 0)), // More visible position for debugging
                             _action()
    {
        // Create GUI healthbar with maximum priority
        _health_bg = bn::regular_bg_items::health.create_bg(-262, -215, 2);
        _health_bg->set_priority(0);
        _health_bg->set_z_order(MIN_Z_ORDER);
        _health_bg->put_above();
        _health_bg->remove_camera();
        _health_bg->set_visible(true);

        _weapon_sprite.set_bg_priority(0);
        _weapon_sprite.remove_camera();

        // Position soul sprite over healthbar with sprite priority
        _soul_sprite.set_bg_priority(0);
        _soul_sprite.set_z_order(-32000); // Maximum priority for sprite
        _soul_sprite.remove_camera();
        _soul_sprite.set_visible(true); // Explicitly make visible

        // Log dimensions and positions for debugging
        BN_LOG("Healthbar bg position: (-262, -215)");
        BN_LOG("Soul sprite position: (-200, -150) - DEBUG POSITION");
        BN_LOG("Soul sprite size: 16x16 pixels (from json height: 16)");
        BN_LOG("Soul sprite z-order: -32000, bg_priority: 0");
    }

    int Healthbar::hp()
    {
        return _hp;
    }

    void Healthbar::set_hp(int hp)
    {
        _hp = bn::max(0, bn::min(2, hp));

        if (_health_bg.has_value())
        {
            _health_bg->set_map(bn::regular_bg_items::health.map_item(), _hp);
        }
    }

    void Healthbar::set_visible(bool is_visible)
    {
        _is_visible = is_visible;

        if (_health_bg.has_value())
        {
            _health_bg->set_visible(is_visible);
        }

        _weapon_sprite.set_visible(is_visible);
        _soul_sprite.set_visible(is_visible);
    }

    void Healthbar::set_soul_position(int x, int y)
    {
        _soul_sprite.set_position(x, y);
        BN_LOG("Soul sprite repositioned to: (", x, ", ", y, ")");
    }

    void Healthbar::debug_soul_center()
    {
        _soul_sprite.set_position(0, 0); // Center of screen
        _soul_sprite.set_z_order(-2000); // Very high priority
        _soul_sprite.set_visible(true);
        BN_LOG("Soul sprite moved to screen center (0, 0) for debugging");
    }

    void Healthbar::debug_soul_animate()
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

    void Healthbar::activate_soul_animation()
    {
        // Start defense buff soul effect with 5-second duration
        _soul_effect_active = true;
        _soul_effect_timer = 300; // 5 seconds at 60 FPS
        
        // Animate soul sprite frames 1-4 for defense buff (SELECT + DOWN)
        _soul_action = bn::create_sprite_animate_action_once(
            _soul_sprite, 8, bn::sprite_items::soul.tiles_item(), 1, 2, 3, 4);
        BN_LOG("Soul animation triggered for defense buff - 5 second effect started");
    }

    void Healthbar::activate_glow()
    {
        _is_glowing = true;
        _action = bn::create_sprite_animate_action_once(
            _weapon_sprite, 15, bn::sprite_items::weapon_claw.tiles_item(), 0, 0, 8, 7, 6, 5, 4, 3, 2, 1, 0);
    }

    void Healthbar::update()
    {
        if (_action.has_value() && !_action.value().done())
        {
            _action.value().update();
        }

        // Handle soul effect duration and fade-out
        if (_soul_effect_active)
        {
            _soul_effect_timer--;
            
            // Check if it's time to start fade-out (reversed animation)
            if (_soul_effect_timer <= 0)
            {
                // Start fade-out with reversed animation (frames 4-3-2-1-0) to return to idle
                _soul_action = bn::create_sprite_animate_action_once(
                    _soul_sprite, 8, bn::sprite_items::soul.tiles_item(), 4, 3, 2, 1, 0);
                _soul_effect_active = false;
                BN_LOG("Soul effect fading out with reversed animation to idle frame");
            }
        }

        // Update soul animation if active
        if (_soul_action.has_value() && !_soul_action.value().done())
        {
            _soul_action.value().update();
        }

        // Position soul sprite at the optimal healthbar overlay position
        // Based on user feedback: from top of circle, first position to the left
        // This corresponds to 90 degrees (π/2 radians) in the circle
        static bool soul_positioned = false;
        if (!soul_positioned)
        {
            bn::fixed angle = 1.57;
            int x = (100 * bn::sin(angle)).integer() - 59;
            int y = (100 * bn::cos(angle)).integer() + 22;
            _soul_sprite.set_position(x, y);
            BN_LOG("Soul sprite positioned at optimal healthbar location: (", x, ", ", y, ")");
            soul_positioned = true;
        }
    }

    bool Healthbar::is_glow_ready()
    {
        return _action.value().done();
    }

    bool Healthbar::is_glow_active()
    {
        return _is_glowing;
    }
}