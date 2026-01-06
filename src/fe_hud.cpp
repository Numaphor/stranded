#include "fe_hud.h"
#include "fe_constants.h"
#include "bn_fixed.h"
#include "bn_sprite_ptr.h"
#include "bn_math.h"

#include "bn_regular_bg_items_health.h"
#include "bn_sprite_items_soul.h"
#include "bn_sprite_items_icon_gun.h"
#include "bn_sprite_items_soul_silver.h"
#include "bn_sprite_items_soul_silver_idle.h"
#include "bn_sprite_items_ammo.h"

namespace fe
{
    HUD::HUD()
        : _hp(HUD_MAX_HP)
        , _is_visible(true)
        , _weapon(WEAPON_TYPE::SWORD)
        , _weapon_sprite(bn::sprite_items::icon_gun.create_sprite(HUD_WEAPON_ICON_X, HUD_WEAPON_ICON_Y, 0))
        , _soul_sprite(bn::sprite_items::soul.create_sprite(HUD_SOUL_INITIAL_X, HUD_SOUL_INITIAL_Y, 0))
        , _soul_positioned(false)
        , _defense_buff_active(false)
        , _defense_buff_fading(false)
        , _silver_soul_active(false)
        , _silver_soul_reversing(false)
        , _silver_idle_timer(0)
        , _displayed_ammo(HUD_MAX_AMMO)
    {
        // Initialize health background
        _health_bg = bn::regular_bg_items::health.create_bg(
            HUD_HEALTH_BG_X, HUD_HEALTH_BG_Y, HUD_HEALTH_BG_MAP_INDEX);
        _health_bg->set_priority(HUD_BG_PRIORITY);
        _health_bg->set_z_order(HUD_BG_Z_ORDER);
        _health_bg->put_above();
        _health_bg->remove_camera();
        _health_bg->set_visible(true);

        // Configure HUD sprites (weapon, soul, ammo)
        _configure_hud_sprite(_weapon_sprite);
        _configure_hud_sprite(_soul_sprite);

        // Play initial soul animation (intro/spawn animation - frame 13 to 4)
        _soul_action = bn::create_sprite_animate_action_once(
            _soul_sprite, HUD_SOUL_ANIM_SPEED,
            bn::sprite_items::soul.tiles_item(),
            13, 12, 11, 10, 9, 8, 7, 6, 5, 4);

        // Initialize ammo display (hidden by default since we start with sword)
        _ammo_sprite = bn::sprite_items::ammo.create_sprite(HUD_AMMO_X, HUD_AMMO_Y, 0);
        _ammo_sprite->set_bg_priority(HUD_BG_PRIORITY);
        _ammo_sprite->remove_camera();
        _ammo_sprite->set_z_order(HUD_SPRITE_Z_ORDER);
        _ammo_sprite->set_visible(false);
    }

    void HUD::_configure_hud_sprite(bn::sprite_ptr& sprite)
    {
        sprite.set_bg_priority(HUD_BG_PRIORITY);
        sprite.remove_camera();
        sprite.set_visible(true);
        sprite.set_z_order(HUD_SPRITE_Z_ORDER);
    }

    int HUD::hp() const
    {
        return _hp;
    }

    void HUD::set_hp(int hp)
    {
        _hp = bn::max(0, bn::min(HUD_MAX_HP, hp));

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

        if (_ammo_sprite.has_value())
        {
            bool show_ammo = is_visible && _weapon == WEAPON_TYPE::GUN && _displayed_ammo > 0;
            _ammo_sprite->set_visible(show_ammo);
        }
    }

    void HUD::activate_soul_animation()
    {
        _defense_buff_active = true;

        // Animate soul sprite frames 1-4 for defense buff
        _soul_action = bn::create_sprite_animate_action_once(
            _soul_sprite, HUD_SOUL_ANIM_SPEED,
            bn::sprite_items::soul.tiles_item(),
            1, 2, 3, 4, 4, 4, 4, 4, 4, 4);
    }

    void HUD::play_soul_damage_animation()
    {
        // Play reverse soul animation when taking damage (4→3→2→1→0)
        _soul_action = bn::create_sprite_animate_action_once(
            _soul_sprite, HUD_SOUL_ANIM_SPEED,
            bn::sprite_items::soul.tiles_item(),
            4, 3, 2, 1, 0, 0, 0, 0, 0, 0);
    }

    void HUD::activate_silver_soul()
    {
        _silver_soul_active = true;
        _silver_idle_timer = 0;

        // Switch to silver soul sprite and play transformation animation
        _soul_sprite.set_item(bn::sprite_items::soul_silver);

        // Create frame sequence for transformation (0-7 with padding)
        bn::vector<uint16_t, 10> frames;
        for (int i = 0; i <= 7; ++i)
        {
            frames.push_back(i);
        }
        frames.push_back(7);
        frames.push_back(7);

        _soul_action = bn::sprite_animate_action<10>::once(
            _soul_sprite, HUD_SOUL_ANIM_SPEED,
            bn::sprite_items::soul_silver.tiles_item(),
            bn::span<const uint16_t>(frames.data(), frames.size()));
    }

    void HUD::deactivate_silver_soul()
    {
        if (!_silver_soul_active)
        {
            return;
        }

        // Create reverse frame sequence for transformation (7-0 with padding)
        bn::vector<uint16_t, 10> frames;
        for (int i = 7; i >= 0; --i)
        {
            frames.push_back(i);
        }
        frames.push_back(0);
        frames.push_back(0);

        _soul_action = bn::sprite_animate_action<10>::once(
            _soul_sprite, HUD_SOUL_ANIM_SPEED,
            bn::sprite_items::soul_silver.tiles_item(),
            bn::span<const uint16_t>(frames.data(), frames.size()));

        _silver_soul_active = false;
        _silver_soul_reversing = true;
        _silver_idle_timer = 0;
    }

    void HUD::deactivate_soul_animation()
    {
        if (!_defense_buff_active)
        {
            return;
        }

        // Play reversed animation (frames 4-3-2-1-0) to return to idle
        _soul_action = bn::create_sprite_animate_action_once(
            _soul_sprite, HUD_SOUL_ANIM_SPEED,
            bn::sprite_items::soul.tiles_item(),
            4, 3, 2, 1, 0, 0, 0, 0, 0, 0);

        _defense_buff_active = false;
        _defense_buff_fading = true;
    }

    void HUD::update()
    {
        _update_soul_position();
        _update_soul_animations();
    }

    void HUD::_update_soul_position()
    {
        if (_soul_positioned)
        {
            return;
        }

        _soul_sprite.set_position(HUD_SOUL_FINAL_X, HUD_SOUL_FINAL_Y);
        _soul_positioned = true;
    }

    void HUD::_update_soul_animations()
    {
        // Handle silver soul idle animations
        if (_silver_soul_active)
        {
            _silver_idle_timer++;

            // Check if transformation animation is done
            if (_soul_action.has_value() && _soul_action.value().done())
            {
                // Play silver idle animation every HUD_SOUL_IDLE_INTERVAL frames
                if (_silver_idle_timer % HUD_SOUL_IDLE_INTERVAL == 0)
                {
                    _soul_sprite.set_item(bn::sprite_items::soul_silver_idle);
                    _soul_action = bn::create_sprite_animate_action_once(
                        _soul_sprite, HUD_SOUL_IDLE_ANIM_SPEED,
                        bn::sprite_items::soul_silver_idle.tiles_item(),
                        0, 1, 2, 1, 0, 0, 0, 0, 0, 0);
                }
            }
        }

        // Update active soul animation
        if (_soul_action.has_value() && !_soul_action.value().done())
        {
            _soul_action.value().update();
        }

        // Handle reverse transformation completion
        if (_silver_soul_reversing && _soul_action.has_value() && _soul_action.value().done())
        {
            _soul_sprite.set_item(bn::sprite_items::soul);
            _soul_sprite.set_tiles(bn::sprite_items::soul.tiles_item().create_tiles(0));
            _soul_action.reset();
            _silver_soul_reversing = false;
        }

        // Handle defense buff fade-out completion
        if (_defense_buff_fading && _soul_action.has_value() && _soul_action.value().done())
        {
            _soul_sprite.set_tiles(bn::sprite_items::soul.tiles_item().create_tiles(0));
            _soul_action.reset();
            _defense_buff_fading = false;
        }
    }

    void HUD::set_weapon(WEAPON_TYPE weapon)
    {
        if (_weapon == weapon)
        {
            return;
        }

        _weapon = weapon;

        // Recreate weapon sprite (both types use icon_gun for now)
        _weapon_sprite = bn::sprite_items::icon_gun.create_sprite(HUD_WEAPON_ICON_X, HUD_WEAPON_ICON_Y, 0);
        _configure_hud_sprite(_weapon_sprite);

        _update_ammo_display();
    }

    void HUD::set_weapon_frame(int frame)
    {
        if (_weapon == WEAPON_TYPE::GUN)
        {
            _weapon_sprite.set_tiles(bn::sprite_items::icon_gun.tiles_item(), frame);
        }
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
        _displayed_ammo = bn::max(0, bn::min(ammo_count, HUD_MAX_AMMO));
        _update_ammo_display();
    }

    void HUD::_update_ammo_display()
    {
        if (!_ammo_sprite.has_value())
        {
            return;
        }

        bool show_ammo = (_weapon == WEAPON_TYPE::GUN);

        if (show_ammo)
        {
            // Invert frame: 10 ammo shows frame 0, 0 ammo shows frame 10
            int frame = HUD_MAX_AMMO - _displayed_ammo;
            _ammo_sprite->set_tiles(bn::sprite_items::ammo.tiles_item(), frame);
            _ammo_sprite->set_visible(_is_visible);
        }
        else
        {
            _ammo_sprite->set_visible(false);
        }
    }
}
