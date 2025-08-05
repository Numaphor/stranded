#include "fe_healthbar.h"
#include "bn_fixed.h"
#include "bn_sprite_ptr.h"
#include "bn_log.h"

#include "bn_regular_bg_items_health.h"
#include "bn_sprite_items_weapon_claw.h"

namespace fe
{
    constexpr int OFFSCREEN_POSITION_X = -500;           // Off-screen X position for weapon sprite
    constexpr int OFFSCREEN_POSITION_Y = -500;           // Off-screen Y position for weapon sprite
    constexpr int MIN_Z_ORDER = -32767;                  // Minimum z-order value for backgrounds/sprites
    const constexpr int weapon_x = OFFSCREEN_POSITION_X; // Move far off-screen to left
    const constexpr int weapon_y = OFFSCREEN_POSITION_Y; // Move far off-screen to top
    Healthbar::Healthbar() : _weapon(WEAPON_TYPE::CLAW), _weapon_sprite(bn::sprite_items::weapon_claw.create_sprite(OFFSCREEN_POSITION_X - 6, OFFSCREEN_POSITION_Y, 0)), _action()
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