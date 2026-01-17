#include "str_hud.h"
#include "str_constants.h"

#include "bn_fixed.h"
#include "bn_sprite_ptr.h"
#include "bn_optional.h"
#include "bn_span.h"
#include "bn_blending.h"
#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_sprite_builder.h"
#include "bn_sprite_text_generator.h"

#include "bn_regular_bg_items_healthbar.h"
#include "bn_sprite_items_heart_normal_full.h"
#include "bn_sprite_items_heart_normal_half.h"
#include "bn_sprite_items_heart_empty.h"
#include "bn_sprite_items_heart_normal_spawn_full.h"
#include "bn_sprite_items_heart_normal_spawn_half.h"
#include "bn_sprite_items_heart_empty_spawn.h"
#include "bn_sprite_items_heart_normal_blink_full.h"
#include "bn_sprite_items_heart_normal_blink_half.h"
#include "bn_sprite_items_heart_golden_full.h"
#include "bn_sprite_items_heart_golden_half.h"
#include "bn_sprite_items_heart_golden_transform_full.h"
#include "bn_sprite_items_heart_golden_transform_half.h"
#include "bn_sprite_items_heart_silver_full.h"
#include "bn_sprite_items_heart_silver_half.h"
#include "bn_sprite_items_heart_silver_transform_full.h"
#include "bn_sprite_items_heart_silver_transform_half.h"
#include "bn_sprite_items_icon_gun.h"
#include "bn_sprite_items_ammo.h"
#include "bn_sprite_items_temptest.h"
#include "bn_sprite_items_hud_icons.h"
#include "bn_sprite_items_energy.h"
#include "bn_sprite_items_health_slot_3.h"
#include "bn_sprite_items_alert.h"

namespace str
{

    // =========================================================================
    // HUD Implementation
    // =========================================================================

    namespace
    {
        constexpr int BUFF_MENU_OPTION_COUNT = 3;
        constexpr int buff_menu_offsets_x[BUFF_MENU_OPTION_COUNT] = {HUD_BUFF_MENU_OPTION_HEAL_X, HUD_BUFF_MENU_OPTION_ENERGY_X, HUD_BUFF_MENU_OPTION_POWER_X};
        constexpr int buff_menu_offsets_y[BUFF_MENU_OPTION_COUNT] = {HUD_BUFF_MENU_OPTION_HEAL_Y, HUD_BUFF_MENU_OPTION_ENERGY_Y, HUD_BUFF_MENU_OPTION_POWER_Y};
        constexpr int buff_menu_icon_frames[BUFF_MENU_OPTION_COUNT] = {0, 1, 3};

        constexpr int NAV_UP = 0, NAV_DOWN = 1, NAV_LEFT = 2, NAV_RIGHT = 3;
        constexpr int buff_menu_nav[BUFF_MENU_OPTION_COUNT][4] = {
            {-1, 2, 1, -1}, // Heal (0): Up->X, Down->Power, Left->Energy, Right->X
            {-1, 2, -1, 0}, // Energy (1): Up->X, Down->Power, Left->X, Right->Heal
            {0, -1, 1, -1}  // Power (2): Up->Heal, Down->X, Left->Energy, Right->X
        };

        // Helper to set sprite item and frame
        void set_soul_sprite_and_frame(bn::sprite_ptr &sprite, const bn::sprite_item &item, int frame)
        {
            sprite.set_item(item);
            sprite.set_tiles(item.tiles_item().create_tiles(frame));
        }
    }

    HUD::HUD()
        : _hp(HUD_MAX_HP),
          _is_visible(true),
          _weapon(WEAPON_TYPE::SWORD),
          _weapon_sprite(bn::sprite_items::icon_gun.create_sprite(HUD_WEAPON_ICON_X, HUD_WEAPON_ICON_Y, 0)),
          _soul_sprite(bn::sprite_items::heart_normal_full.create_sprite(HUD_SOUL_INITIAL_X, HUD_SOUL_INITIAL_Y, 0)),
          _soul_positioned(false),
          _defense_buff_active(false),
          _defense_buff_fading(false),
          _silver_soul_active(false),
          _silver_soul_reversing(false),
          _silver_idle_timer(0),
          _health_gain_anim_active(false),
          _health_loss_anim_active(false),
          _resetting_health(false),
          _displayed_ammo(HUD_MAX_AMMO),
          _buff_menu_state(BUFF_MENU_STATE::CLOSED),
          _buff_menu_base(bn::sprite_items::temptest.create_sprite(HUD_BUFF_MENU_BASE_X, HUD_BUFF_MENU_BASE_Y, 0)),
          _selected_buff_option(0),
          _buff_menu_hold_timer(0),
          _buff_menu_cooldown_timer(0)
    {

        _health_bg = bn::regular_bg_items::healthbar.create_bg(
            HUD_HEALTH_BG_X, HUD_HEALTH_BG_Y, HUD_HEALTH_BG_MAP_INDEX);
        _health_bg->set_priority(HUD_BG_PRIORITY);
        _health_bg->set_z_order(HUD_BG_Z_ORDER);
        _health_bg->put_above();
        _health_bg->remove_camera();
        _health_bg->set_visible(true);

        _configure_hud_sprite(_weapon_sprite);
        _configure_hud_sprite(_soul_sprite);

        _energy_sprite = bn::sprite_items::energy.create_sprite(HUD_SOUL_INITIAL_X, HUD_SOUL_INITIAL_Y);
        _configure_hud_sprite(_energy_sprite.value());

        _health_slot_3_sprite = bn::sprite_items::health_slot_3.create_sprite(HUD_SOUL_INITIAL_X, HUD_SOUL_INITIAL_Y);
        _configure_hud_sprite(_health_slot_3_sprite.value());

        _alert_sprite = bn::sprite_items::alert.create_sprite(HUD_SOUL_INITIAL_X, HUD_SOUL_INITIAL_Y);
        _configure_hud_sprite(_alert_sprite.value());

        // Initial animation (Spawn Full)
        _soul_sprite.set_item(bn::sprite_items::heart_normal_spawn_full);
        _soul_action = bn::create_sprite_animate_action_once(
            _soul_sprite, HUD_SOUL_ANIM_SPEED,
            bn::sprite_items::heart_normal_spawn_full.tiles_item(),
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9);

        _ammo_sprite = bn::sprite_items::ammo.create_sprite(HUD_AMMO_X, HUD_AMMO_Y, 0);
        _ammo_sprite->set_bg_priority(HUD_BG_PRIORITY);
        _ammo_sprite->remove_camera();
        _ammo_sprite->set_z_order(HUD_SPRITE_Z_ORDER);
        _ammo_sprite->set_visible(false);

        _configure_hud_sprite(_buff_menu_base);
        _buff_menu_base.set_horizontal_flip(true);
        _buff_menu_base.set_visible(true);
    }

    void HUD::_configure_hud_sprite(bn::sprite_ptr &sprite)
    {
        sprite.set_bg_priority(HUD_BG_PRIORITY);
        sprite.remove_camera();
        sprite.set_visible(true);
        sprite.set_z_order(HUD_SPRITE_Z_ORDER);
    }

    int HUD::hp() const { return _hp; }

    void HUD::set_hp(int hp)
    {
        int old_hp = _hp;
        _hp = bn::clamp(hp, 0, HUD_MAX_HP);
        if (_health_bg)
            _health_bg->set_map(bn::regular_bg_items::healthbar.map_item(), _hp);
        if (_hp > old_hp)
        {
            if (_resetting_health && _hp == 3)
            {
                set_soul_sprite_and_frame(_soul_sprite, bn::sprite_items::heart_normal_full, 0);
                _soul_action.reset();
                _defense_buff_active = _silver_soul_active = false;
            }
            else
            {
                static const int f[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
                if (old_hp == 0 && _hp == 1)
                    _play_health_transition_anim(bn::sprite_items::heart_empty_spawn, f, 10, 1);
                else if (old_hp == 1 && _hp == 2)
                    _play_health_transition_anim(bn::sprite_items::heart_normal_spawn_half, f, 10, 1);
                else if (_hp == 3)
                    _play_health_transition_anim(bn::sprite_items::heart_normal_spawn_full, f, 10, 1);
            }
        }
        else if (_hp < old_hp)
        {
            if (old_hp == 3 && _hp == 2)
            {
                static const int f[] = {10, 11, 12, 13};
                _play_health_transition_anim(bn::sprite_items::heart_normal_spawn_full, f, 4, 0);
            }
            else if (old_hp == 1 && _hp == 0)
            {
                static const int f[] = {10, 11, 12, 13};
                _play_health_transition_anim(bn::sprite_items::heart_empty_spawn, f, 4, 0);
            }
            else
                play_soul_damage_animation();
        }
    }

    void HUD::set_resetting_health(bool resetting) { _resetting_health = resetting; }

    bool HUD::is_soul_animation_complete() const { return !_soul_action.has_value() || _soul_action.value().done(); }

    void HUD::set_position(int x, int y)
    {
        if (_health_bg.has_value())
        {
            _health_bg->set_position(x, y);
            int soul_x = x + HUD_SOUL_OFFSET_X;
            int soul_y = y + HUD_SOUL_OFFSET_Y;
            _soul_sprite.set_position(soul_x, soul_y);
            
            if (_energy_sprite.has_value())
            {
                int energy_x = x + HUD_ENERGY_OFFSET_X;
                int energy_y = y + HUD_ENERGY_OFFSET_Y;
                _energy_sprite->set_position(energy_x, energy_y);
            }

            if (_health_slot_3_sprite.has_value())
            {
                int hs3_x = x + HUD_HEALTH_SLOT_3_OFFSET_X;
                int hs3_y = y + HUD_HEALTH_SLOT_3_OFFSET_Y;
                _health_slot_3_sprite->set_position(hs3_x, hs3_y);
            }

            if (_alert_sprite.has_value())
            {
                int alert_x = x + HUD_ALERT_OFFSET_X;
                int alert_y = y + HUD_ALERT_OFFSET_Y;
                _alert_sprite->set_position(alert_x, alert_y);
            }
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
        if (_energy_sprite.has_value())
            _energy_sprite->set_visible(is_visible);
        if (_health_slot_3_sprite.has_value())
            _health_slot_3_sprite->set_visible(is_visible);
        _buff_menu_base.set_visible(is_visible);

        if (_ammo_sprite.has_value())
        {
            bool show_ammo = is_visible && _weapon == WEAPON_TYPE::GUN && _displayed_ammo > 0;
            _ammo_sprite->set_visible(show_ammo);
        }
        else
        {
            _buff_menu_state = BUFF_MENU_STATE::CLOSED;
            for (int i = 0; i < BUFF_MENU_OPTION_COUNT; ++i)
            {
                _buff_menu_option_sprites[i].reset();
            }
        }
    }

    void HUD::activate_soul_animation()
    {
        // Defense Buff (Golden)
        _defense_buff_active = true;

        // Transform based on current HP
        const bn::sprite_item &transform_item = (_hp >= 3) ? bn::sprite_items::heart_golden_transform_full
                                                           : bn::sprite_items::heart_golden_transform_half;

        _soul_sprite.set_item(transform_item);
        _soul_action = bn::create_sprite_animate_action_once(
            _soul_sprite, HUD_SOUL_ANIM_SPEED,
            transform_item.tiles_item(),
            0, 1, 2, 3, 4, 5, 6, 7); // 8 frames
    }

    void HUD::play_soul_damage_animation()
    {
        // Generic damage animation (Blink)
        // Use Full or Half blink depending on current state (before damage)
        const bn::sprite_item &blink_item = (_hp >= 3) ? bn::sprite_items::heart_normal_blink_full
                                                       : bn::sprite_items::heart_normal_blink_half;

        _soul_sprite.set_item(blink_item);
        _soul_action = bn::create_sprite_animate_action_once(
            _soul_sprite, HUD_SOUL_ANIM_SPEED,
            blink_item.tiles_item(),
            0, 1, 2, 1, 0); // Blink
    }

    void HUD::play_health_loss_animation()
    {
        play_soul_damage_animation();
    }

    void HUD::activate_silver_soul()
    {
        // Energy Buff (Silver)
        _silver_soul_active = true;
        _silver_idle_timer = 0;

        const bn::sprite_item &transform_item = (_hp >= 3) ? bn::sprite_items::heart_silver_transform_full
                                                           : bn::sprite_items::heart_silver_transform_half;

        _soul_sprite.set_item(transform_item);
        _soul_action = bn::create_sprite_animate_action_once(
            _soul_sprite, HUD_SOUL_ANIM_SPEED,
            transform_item.tiles_item(),
            0, 1, 2, 3, 4, 5, 6, 7); // 8 frames
    }

    void HUD::deactivate_silver_soul()
    {
        if (!_silver_soul_active)
        {
            return;
        }

        // Reverse transform
        const bn::sprite_item &transform_item = (_hp >= 3) ? bn::sprite_items::heart_silver_transform_full
                                                           : bn::sprite_items::heart_silver_transform_half;

        _soul_sprite.set_item(transform_item);
        _soul_action = bn::create_sprite_animate_action_once(
            _soul_sprite, HUD_SOUL_ANIM_SPEED,
            transform_item.tiles_item(),
            7, 6, 5, 4, 3, 2, 1, 0);

        _silver_soul_active = false;
        _silver_soul_reversing = true;
        _silver_idle_timer = 0;
    }

    void HUD::set_weapon(WEAPON_TYPE weapon)
    {
        _weapon = weapon;
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

    WEAPON_TYPE HUD::get_weapon() const { return _weapon; }

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
            int frame = HUD_MAX_AMMO - _displayed_ammo;
            _ammo_sprite->set_tiles(bn::sprite_items::ammo.tiles_item(), frame);
            _ammo_sprite->set_visible(_is_visible);
        }
        else
        {
            _ammo_sprite->set_visible(false);
        }
    }

    void HUD::update()
    {
        _update_soul_position();
        _update_soul_animations();
        _update_buff_menu_sprites();
        update_buff_menu_hold();
        update_buff_menu_cooldown();
    }

    void HUD::_update_soul_position()
    {
        if (_health_bg.has_value())
        {
            bn::fixed_point bg_pos = _health_bg->position();
            _soul_sprite.set_position(bg_pos.x() + HUD_SOUL_OFFSET_X, bg_pos.y() + HUD_SOUL_OFFSET_Y);
            if (_energy_sprite.has_value())
                _energy_sprite->set_position(bg_pos.x() + HUD_ENERGY_OFFSET_X, bg_pos.y() + HUD_ENERGY_OFFSET_Y);
            if (_health_slot_3_sprite.has_value())
                _health_slot_3_sprite->set_position(bg_pos.x() + HUD_HEALTH_SLOT_3_OFFSET_X, bg_pos.y() + HUD_HEALTH_SLOT_3_OFFSET_Y);
            if (_alert_sprite.has_value())
                _alert_sprite->set_position(bg_pos.x() + HUD_ALERT_OFFSET_X, bg_pos.y() + HUD_ALERT_OFFSET_Y);
        }
    }

    void HUD::_update_soul_animations()
    {
        if (_energy_sprite.has_value())
        {
            // Energy has 4 frames (0-3).
            // Max Energy is 3.
            _energy_sprite->set_tiles(bn::sprite_items::energy.tiles_item(), _energy);
        }

        if (_health_slot_3_sprite.has_value())
        {
            // Health Slot 3 has 2 frames.
            // Frame 0: Empty
            // Frame 1: Active (Full) - when HP >= 3
            int frame = (_hp >= 3) ? 1 : 0;
            _health_slot_3_sprite->set_tiles(bn::sprite_items::health_slot_3.tiles_item(), frame);
        }

        if (_alert_sprite.has_value())
        {
             // Alert has 2 frames.
             // Frame 0: Empty
             // Frame 1: Active
             _alert_sprite->set_tiles(bn::sprite_items::alert.tiles_item(), _alert_active ? 1 : 0);
        }

        if (!_soul_action.has_value())
            return;

        if (!_soul_action->done())
        {
            _soul_action->update();
            return;
        }

        // Animation complete, reset to idle state
        const bn::sprite_item *target_item = &bn::sprite_items::heart_empty;
        int frame_index = 0;

        // Health Gain Completion
        if (_health_gain_anim_active)
        {
            if (_hp >= 3)
            {
                if (_silver_soul_active)
                    target_item = &bn::sprite_items::heart_silver_full;
                else if (_defense_buff_active)
                    target_item = &bn::sprite_items::heart_golden_full;
                else
                {
                    target_item = &bn::sprite_items::heart_normal_spawn_full;
                    frame_index = 9;
                }
            }
            else if (_hp == 2)
            {
                if (_silver_soul_active)
                    target_item = &bn::sprite_items::heart_silver_half;
                else if (_defense_buff_active)
                    target_item = &bn::sprite_items::heart_golden_half;
                else
                    target_item = &bn::sprite_items::heart_normal_full;
            }
            else if (_hp == 1)
            {
                target_item = &bn::sprite_items::heart_normal_half;
            }
            else
            {
                // 0 HP -> Nothing
                target_item = &bn::sprite_items::heart_empty_spawn;
                frame_index = 0;
            }
            _health_gain_anim_active = false;
        }
        // Health Loss Completion
        else if (_health_loss_anim_active)
        {
            if (_hp >= 3)
            {
                target_item = &bn::sprite_items::heart_normal_spawn_full;
                frame_index = 9;
            }
            else if (_hp == 2)
            {
                if (_silver_soul_active)
                    target_item = &bn::sprite_items::heart_silver_half;
                else if (_defense_buff_active)
                    target_item = &bn::sprite_items::heart_golden_half;
                else
                    target_item = &bn::sprite_items::heart_normal_full;
            }
            else if (_hp == 1)
            {
                target_item = &bn::sprite_items::heart_normal_half;
            }
            else
            {
                // 0 HP -> Nothing
                target_item = &bn::sprite_items::heart_empty_spawn;
                frame_index = 0;
            }
            _health_loss_anim_active = false;
        }
        // Buff Transition/Activation Completion
        else if (_silver_soul_reversing || _defense_buff_fading)
        {
            if (_hp >= 3)
            {
                target_item = &bn::sprite_items::heart_normal_spawn_full;
                frame_index = 9;
            }
            else if (_hp == 2)
                target_item = &bn::sprite_items::heart_normal_half;
            else if (_hp == 1)
                target_item = &bn::sprite_items::heart_normal_half;
            else
            {
                target_item = &bn::sprite_items::heart_empty_spawn;
                frame_index = 0;
            }
            _silver_soul_reversing = false;
            _defense_buff_fading = false;
        }
        else if (_silver_soul_active)
        {
            if (_hp >= 3)
                target_item = &bn::sprite_items::heart_silver_full;
            else if (_hp == 2)
                target_item = &bn::sprite_items::heart_silver_half;
            else if (_hp == 1)
                target_item = &bn::sprite_items::heart_normal_half; // Use normal half for silver empty
            else
            {
                target_item = &bn::sprite_items::heart_empty_spawn;
                frame_index = 0;
            }
        }
        else if (_defense_buff_active)
        {
            if (_hp >= 3)
                target_item = &bn::sprite_items::heart_golden_full;
            else if (_hp == 2)
                target_item = &bn::sprite_items::heart_golden_half;
            else if (_hp == 1)
                target_item = &bn::sprite_items::heart_normal_half; // Use normal half for golden empty
            else
            {
                target_item = &bn::sprite_items::heart_empty_spawn;
                frame_index = 0;
            }
        }
        else
        {
            // Default idle state
            if (_hp >= 3)
            {
                target_item = &bn::sprite_items::heart_normal_spawn_full;
                frame_index = 9;
            }
            else if (_hp == 2)
                target_item = &bn::sprite_items::heart_normal_half;
            else if (_hp == 1)
                target_item = &bn::sprite_items::heart_normal_half;
            else
            {
                target_item = &bn::sprite_items::heart_empty_spawn;
                frame_index = 0;
            }
        }

        set_soul_sprite_and_frame(_soul_sprite, *target_item, frame_index);
        _soul_action.reset();
    }

    void HUD::toggle_buff_menu()
    {
        if (is_buff_menu_on_cooldown())
            return;

        if (_buff_menu_state == BUFF_MENU_STATE::CLOSED)
        {
            _buff_menu_state = BUFF_MENU_STATE::OPEN;
            _selected_buff_option = 0;
            // Initialize option sprites
            for (int i = 0; i < BUFF_MENU_OPTION_COUNT; ++i)
            {
                int sprite_x = HUD_BUFF_MENU_BASE_X + buff_menu_offsets_x[i];
                int sprite_y = HUD_BUFF_MENU_BASE_Y + buff_menu_offsets_y[i];
                _buff_menu_option_sprites[i] = bn::sprite_items::hud_icons.create_sprite(sprite_x, sprite_y, buff_menu_icon_frames[i]);
                _configure_hud_sprite(_buff_menu_option_sprites[i].value());
                if (i != _selected_buff_option)
                {
                    _buff_menu_option_sprites[i]->set_blending_enabled(true);
                }
            }
        }
        else
        {
            _buff_menu_state = BUFF_MENU_STATE::CLOSED;
            for (int i = 0; i < BUFF_MENU_OPTION_COUNT; ++i)
            {
                _buff_menu_option_sprites[i].reset();
            }
        }
    }

    void HUD::_play_health_transition_anim(const bn::sprite_item &it, const int *f, int c, bool g)
    {
        _health_gain_anim_active = g;
        _health_loss_anim_active = !g;
        _soul_sprite.set_item(it);
        auto s = [&](auto... args)
        { _soul_action = bn::create_sprite_animate_action_once(_soul_sprite, HUD_SOUL_ANIM_SPEED, it.tiles_item(), args...); };
        if (c == 14)
            s(f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8], f[9], f[10], f[11], f[12], f[13]);
        else if (c == 10)
            s(f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8], f[9]);
        else if (c == 9)
            s(f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8]);
        else if (c == 7)
            s(f[0], f[1], f[2], f[3], f[4], f[5], f[6]);
        else if (c == 5)
            s(f[0], f[1], f[2], f[3], f[4]);
        else if (c == 4)
            s(f[0], f[1], f[2], f[3]);
        else
            s(f[0], f[1], f[2], f[3], f[4], f[5]);
    }

    bool HUD::is_buff_menu_on_cooldown() const { return _buff_menu_cooldown_timer > 0; }

    void HUD::_update_buff_menu_sprites()
    {
        if (_buff_menu_state == BUFF_MENU_STATE::OPEN)
        {
            for (int i = 0; i < BUFF_MENU_OPTION_COUNT; ++i)
            {
                if (_buff_menu_option_sprites[i].has_value())
                {
                    _buff_menu_option_sprites[i]->set_visible(_is_visible);
                }
            }
        }
    }

    void HUD::_update_selection(int new_selection)
    {
        if (new_selection == _selected_buff_option || new_selection < 0 || new_selection >= BUFF_MENU_OPTION_COUNT)
        {
            return;
        }
        if (_buff_menu_option_sprites[_selected_buff_option].has_value())
        {
            _buff_menu_option_sprites[_selected_buff_option]->set_blending_enabled(true);
        }
        _selected_buff_option = new_selection;
        if (_buff_menu_option_sprites[_selected_buff_option].has_value())
        {
            _buff_menu_option_sprites[_selected_buff_option]->set_blending_enabled(false);
        }
    }

    void HUD::navigate_buff_menu_up()
    {
        if (_buff_menu_state == BUFF_MENU_STATE::OPEN)
        {
            int new_sel = buff_menu_nav[_selected_buff_option][NAV_UP];
            if (new_sel != -1)
                _update_selection(new_sel);
        }
    }

    void HUD::navigate_buff_menu_down()
    {
        if (_buff_menu_state == BUFF_MENU_STATE::OPEN)
        {
            int new_sel = buff_menu_nav[_selected_buff_option][NAV_DOWN];
            if (new_sel != -1)
                _update_selection(new_sel);
        }
    }

    void HUD::navigate_buff_menu_left()
    {
        if (_buff_menu_state == BUFF_MENU_STATE::OPEN)
        {
            int new_sel = buff_menu_nav[_selected_buff_option][NAV_LEFT];
            if (new_sel != -1)
                _update_selection(new_sel);
        }
    }

    void HUD::navigate_buff_menu_right()
    {
        if (_buff_menu_state == BUFF_MENU_STATE::OPEN)
        {
            int new_sel = buff_menu_nav[_selected_buff_option][NAV_RIGHT];
            if (new_sel != -1)
                _update_selection(new_sel);
        }
    }

    bool HUD::is_buff_menu_open() const { return _buff_menu_state == BUFF_MENU_STATE::OPEN; }

    int HUD::get_selected_buff() const { return _selected_buff_option; }

    void HUD::set_energy(int energy)
    {
        _energy = bn::clamp(energy, 0, HUD_MAX_ENERGY);
    }

    void HUD::set_alert(bool active)
    {
        _alert_active = active;
    }

    void HUD::start_buff_menu_hold()
    {
        if (_buff_menu_state == BUFF_MENU_STATE::CLOSED && _buff_menu_hold_timer == 0)
        {
            _buff_menu_hold_timer = 1;
            _buff_menu_base.set_tiles(bn::sprite_items::temptest.tiles_item(), 8);
        }
    }

    void HUD::update_buff_menu_hold()
    {
        if (_buff_menu_hold_timer > 0 && _buff_menu_state == BUFF_MENU_STATE::CLOSED)
        {
            _buff_menu_hold_timer++;
            int frame = 8 - (_buff_menu_hold_timer * 7) / HUD_BUFF_MENU_HOLD_FRAMES;
            if (frame < 1)
            {
                frame = 1;
            }
            _buff_menu_base.set_tiles(bn::sprite_items::temptest.tiles_item(), frame);
        }
    }

    void HUD::cancel_buff_menu_hold()
    {
        _buff_menu_hold_timer = 0;
        _buff_menu_base.set_tiles(bn::sprite_items::temptest.tiles_item(), 0);
    }

    bool HUD::is_buff_menu_hold_complete() const { return _buff_menu_hold_timer >= HUD_BUFF_MENU_HOLD_FRAMES; }

    bool HUD::is_buff_menu_holding() const { return _buff_menu_hold_timer > 0; }

    void HUD::start_buff_menu_cooldown()
    {
        _buff_menu_cooldown_timer = 1;
        _buff_menu_base.set_tiles(bn::sprite_items::temptest.tiles_item(), 1);
    }

    void HUD::update_buff_menu_cooldown()
    {
        if (_buff_menu_cooldown_timer > 0)
        {
            _buff_menu_cooldown_timer++;
            int frame = 1 + (_buff_menu_cooldown_timer * 7) / HUD_BUFF_MENU_COOLDOWN_FRAMES;
            if (frame > 8)
            {
                frame = 8;
            }
            _buff_menu_base.set_tiles(bn::sprite_items::temptest.tiles_item(), frame);
            if (_buff_menu_cooldown_timer >= HUD_BUFF_MENU_COOLDOWN_FRAMES)
            {
                _buff_menu_cooldown_timer = 0;
                _buff_menu_base.set_tiles(bn::sprite_items::temptest.tiles_item(), 0);
            }
        }
    }

    // =========================================================================
    // Gun Selection Menu
    // =========================================================================

    void HUD::toggle_gun_menu()
    {
        if (_gun_menu_open)
        {
            // Close menu
            _gun_menu_open = false;
            for (int i = 0; i < 6; ++i)
            {
                _gun_menu_sprites[i].reset();
            }
            _gun_menu_cursor.reset();
        }
        else
        {
            // Open menu - create 2x3 grid of gun icons centered on screen
            _gun_menu_open = true;
            constexpr int GRID_COLS = 3;
            constexpr int GRID_SPACING = 20;
            constexpr int BASE_X = -GRID_SPACING; // Center the 3-column grid
            constexpr int BASE_Y = -10;           // Slightly above center

            for (int i = 0; i < 6; ++i)
            {
                int col = i % GRID_COLS;
                int row = i / GRID_COLS;
                int x = BASE_X + col * GRID_SPACING;
                int y = BASE_Y + row * GRID_SPACING;
                _gun_menu_sprites[i] = bn::sprite_items::icon_gun.create_sprite(x, y, i);
                _configure_hud_sprite(_gun_menu_sprites[i].value());
                // Dim non-selected options
                if (i != _selected_gun)
                {
                    _gun_menu_sprites[i]->set_blending_enabled(true);
                }
            }
        }
    }

    void HUD::navigate_gun_menu(int delta)
    {
        if (!_gun_menu_open)
            return;

        int old_selection = _selected_gun;
        int new_selection = _selected_gun + delta;

        // Wrap around in grid (3 columns, 2 rows)
        if (delta == 1 || delta == -1)
        {
            // Left/right navigation - wrap within row
            int row = _selected_gun / 3;
            int col = (_selected_gun % 3) + delta;
            if (col < 0)
                col = 2;
            else if (col > 2)
                col = 0;
            new_selection = row * 3 + col;
        }
        else
        {
            // Up/down navigation - wrap between rows
            new_selection = (_selected_gun + delta + 6) % 6;
        }

        if (new_selection != old_selection && new_selection >= 0 && new_selection < 6)
        {
            // Update visual selection
            if (_gun_menu_sprites[old_selection].has_value())
            {
                _gun_menu_sprites[old_selection]->set_blending_enabled(true);
            }
            _selected_gun = new_selection;
            if (_gun_menu_sprites[_selected_gun].has_value())
            {
                _gun_menu_sprites[_selected_gun]->set_blending_enabled(false);
            }
        }
    }

    bool HUD::is_gun_menu_open() const { return _gun_menu_open; }

    int HUD::get_selected_gun() const { return _selected_gun; }

} // namespace str
