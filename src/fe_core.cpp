#include "fe_hud.h"
#include "fe_hitbox.h"
#include "fe_level.h"
#include "fe_minimap.h"
#include "fe_bullet_manager.h"
#include "fe_entity.h"
#include "fe_movement.h"
#include "fe_direction_utils.h"
#include "fe_constants.h"
#include "fe_enemy.h"
#include "fe_scene.h"
#include "fe_scene_world.h"
#include "fe_scene_menu.h"
#include "fe_scene_start.h"
#include "fe_scene_controls.h"
#include "fe_world_state.h"
#include "fe_collision.h"
#include "fe_npc.h"

#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_sprite_ptr.h"
#include "bn_math.h"
#include "bn_log.h"
#include "bn_string.h"
#include "bn_optional.h"
#include "bn_span.h"
#include "bn_blending.h"
#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_map_cell_info.h"
#include "bn_camera_ptr.h"
#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_memory.h"
#include "bn_unique_ptr.h"
#include "bn_size.h"
#include "bn_window.h"
#include "bn_rect_window.h"
#include "bn_bg_palettes.h"
#include "bn_bg_tiles.h"
#include "bn_bg_palette_ptr.h"
#include "bn_regular_bg_map_item.h"
#include "bn_regular_bg_map_cell.h"
#include "bn_regular_bg_tiles_ptr.h"
#include "bn_sprite_builder.h"
#include "bn_sprite_double_size_mode.h"
#include "bn_sprite_shape_size.h"
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
#include "bn_sprite_items_minimap_player.h"
#include "bn_sprite_items_minimap_enemy.h"
#include "bn_sprite_items_hero_sword.h"
#include "bn_bg_palette_items_palette.h"
#include "bn_regular_bg_tiles_items_tiles.h"
#include "bn_affine_bg_items_sword.h"
#include "common_variable_8x8_sprite_font.h"

namespace fe
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
        }
    }

    void HUD::_update_soul_animations()
    {
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
    // Hitbox Implementation
    // =========================================================================

    Hitbox::Hitbox() : _pos(0, 0), _width(0), _height(0) {}

    Hitbox::Hitbox(bn::fixed x, bn::fixed y, bn::fixed width, bn::fixed height)
        : _pos(x, y), _width(width), _height(height) {}

    Hitbox::Hitbox(bn::fixed x, bn::fixed y, bn::fixed width, bn::fixed height, HitboxType type)
        : _pos(x, y), _width(width), _height(height), _type(type) {}

    void Hitbox::set_x(bn::fixed x) { _pos.set_x(x); }
    void Hitbox::set_y(bn::fixed y) { _pos.set_y(y); }
    void Hitbox::set_position(bn::fixed_point position) { _pos = position; }

    void Hitbox::get_collision_points(bn::fixed_point pos, fe::directions direction, bn::fixed_point points[4]) const
    {
        bn::fixed left = pos.x();
        bn::fixed right = pos.x() + _width - HITBOX_EDGE_OFFSET;
        bn::fixed top = pos.y();
        bn::fixed bottom = pos.y() + _height - HITBOX_EDGE_OFFSET;
        bn::fixed middle_x = pos.x() + _width / 2;
        bn::fixed quarter_x = pos.x() + _width / 4;
        bn::fixed middle_y = pos.y() + _height / 2;
        bn::fixed quarter_y = pos.y() + _height / 4;

        switch (direction)
        {
        case fe::directions::up:
            points[0] = bn::fixed_point(left, top);
            points[1] = bn::fixed_point(right, top);
            points[2] = bn::fixed_point(middle_x, top);
            points[3] = bn::fixed_point(quarter_x, top);
            break;
        case fe::directions::down:
            points[0] = bn::fixed_point(left, bottom);
            points[1] = bn::fixed_point(right, bottom);
            points[2] = bn::fixed_point(middle_x, bottom);
            points[3] = bn::fixed_point(quarter_x, bottom);
            break;
        case fe::directions::left:
            points[0] = bn::fixed_point(left, top);
            points[1] = bn::fixed_point(left, bottom);
            points[2] = bn::fixed_point(left, middle_y);
            points[3] = bn::fixed_point(left, quarter_y);
            break;
        case fe::directions::right:
            points[0] = bn::fixed_point(right, top);
            points[1] = bn::fixed_point(right, bottom);
            points[2] = bn::fixed_point(right, middle_y);
            points[3] = bn::fixed_point(right, quarter_y);
            break;
        default:
            points[0] = bn::fixed_point(left, top);
            points[1] = bn::fixed_point(right, top);
            points[2] = bn::fixed_point(left, bottom);
            points[3] = bn::fixed_point(right, bottom);
            break;
        }
    }

    bool Hitbox::contains_point(const bn::fixed_point &position) const
    {
        return position.x() >= x() && position.x() < x() + width() &&
               position.y() >= y() && position.y() < y() + height();
    }

    bool Hitbox::is_in_sword_zone(const bn::fixed_point &position)
    {
        const bn::fixed zone_left = fe::SWORD_ZONE_TILE_LEFT * fe::TILE_SIZE - fe::MAP_OFFSET;
        const bn::fixed zone_right = fe::SWORD_ZONE_TILE_RIGHT * fe::TILE_SIZE - fe::MAP_OFFSET;
        const bn::fixed zone_top = fe::SWORD_ZONE_TILE_TOP * fe::TILE_SIZE - fe::MAP_OFFSET;
        const bn::fixed zone_bottom = fe::SWORD_ZONE_TILE_BOTTOM * fe::TILE_SIZE - fe::MAP_OFFSET;
        return position.x() >= zone_left && position.x() < zone_right &&
               position.y() >= zone_top && position.y() < zone_bottom;
    }

    bool Hitbox::is_in_merchant_interaction_zone(const bn::fixed_point &position, const bn::fixed_point &merchant_center)
    {
        bn::fixed_point zone_position = calculate_centered_position(merchant_center, MERCHANT_INTERACTION_ZONE_WIDTH, MERCHANT_INTERACTION_ZONE_HEIGHT);
        return position.x() >= zone_position.x() && position.x() < zone_position.x() + MERCHANT_INTERACTION_ZONE_WIDTH &&
               position.y() >= zone_position.y() && position.y() < zone_position.y() + MERCHANT_INTERACTION_ZONE_HEIGHT;
    }

    bool Hitbox::is_in_merchant_collision_zone(const bn::fixed_point &position, const bn::fixed_point &merchant_center)
    {
        bn::fixed_point zone_position = calculate_centered_position(merchant_center, MERCHANT_COLLISION_ZONE_WIDTH, MERCHANT_COLLISION_ZONE_HEIGHT);
        return position.x() >= zone_position.x() && position.x() < zone_position.x() + MERCHANT_COLLISION_ZONE_WIDTH &&
               position.y() >= zone_position.y() && position.y() < zone_position.y() + MERCHANT_COLLISION_ZONE_HEIGHT;
    }

    Hitbox Hitbox::create_player_hitbox(bn::fixed_point position)
    {
        bn::fixed_point hitbox_pos = calculate_centered_position(position, PLAYER_HITBOX_WIDTH, PLAYER_HITBOX_HEIGHT);
        return Hitbox(hitbox_pos.x(), hitbox_pos.y(), PLAYER_HITBOX_WIDTH, PLAYER_HITBOX_HEIGHT, HitboxType::PLAYER);
    }

    Hitbox Hitbox::create_merchant_interaction_zone(bn::fixed_point center)
    {
        bn::fixed_point position = calculate_centered_position(center, MERCHANT_INTERACTION_ZONE_WIDTH, MERCHANT_INTERACTION_ZONE_HEIGHT);
        return Hitbox(position.x(), position.y(), MERCHANT_INTERACTION_ZONE_WIDTH, MERCHANT_INTERACTION_ZONE_HEIGHT, HitboxType::MERCHANT_INTERACTION);
    }

    Hitbox Hitbox::create_sword_zone()
    {
        const bn::fixed zone_left = fe::SWORD_ZONE_TILE_LEFT * fe::TILE_SIZE - fe::MAP_OFFSET;
        const bn::fixed zone_top = fe::SWORD_ZONE_TILE_TOP * fe::TILE_SIZE - fe::MAP_OFFSET;
        const bn::fixed width = (fe::SWORD_ZONE_TILE_RIGHT - fe::SWORD_ZONE_TILE_LEFT) * fe::TILE_SIZE;
        const bn::fixed height = (fe::SWORD_ZONE_TILE_BOTTOM - fe::SWORD_ZONE_TILE_TOP) * fe::TILE_SIZE;
        return Hitbox(zone_left, zone_top, width, height, HitboxType::SWORD_ZONE);
    }

    bn::optional<bn::fixed_point> ZoneManager::_merchant_zone_center;
    bool ZoneManager::_merchant_zone_enabled = false;

    void ZoneManager::set_merchant_zone_center(const bn::fixed_point &center)
    {
        _merchant_zone_center = center;
        _merchant_zone_enabled = true;
    }

    void ZoneManager::clear_merchant_zone()
    {
        _merchant_zone_center.reset();
        _merchant_zone_enabled = false;
    }

    void ZoneManager::set_merchant_zone_enabled(bool enabled) { _merchant_zone_enabled = enabled && _merchant_zone_center.has_value(); }

    bn::optional<bn::fixed_point> ZoneManager::get_merchant_zone_center() { return _merchant_zone_center; }

    bool ZoneManager::is_merchant_zone_enabled() { return _merchant_zone_enabled && _merchant_zone_center.has_value(); }

    bool ZoneManager::is_position_valid(const bn::fixed_point &position)
    {
        if (Hitbox::is_in_sword_zone(position))
            return false;
        if (is_merchant_zone_enabled() && _merchant_zone_center.has_value())
        {
            if (Hitbox::is_in_merchant_collision_zone(position, _merchant_zone_center.value()))
                return false;
        }
        return true;
    }

    // =========================================================================
    // Level Implementation
    // =========================================================================

    Level::Level(bn::regular_bg_map_ptr bg)
    {
        _bg_map_ptr = bg;
        _floor_tiles = {};
        _zone_tiles.clear();
        _zone_tiles.push_back(COLLISION_ZONE_TILE_INDEX);
        _zone_tiles.push_back(INTERACTION_ZONE_TILE_INDEX);
        bn::span<const bn::regular_bg_map_cell> cells = bg.cells_ref().value();
        for (int i = 0; i < 32 && i < cells.size(); ++i)
        {
            if (cells.at(i) != 0)
            {
                _floor_tiles.push_back(cells.at(i));
            }
        }
    }

    bn::vector<int, 32> Level::floor_tiles() { return _floor_tiles; }

    void Level::add_zone_tile(int tile_index)
    {
        if (_zone_tiles.size() < _zone_tiles.max_size())
        {
            _zone_tiles.push_back(tile_index);
        }
    }

    void Level::reset()
    {
        _zone_tiles.clear();
        _zone_tiles.push_back(4);
        _zone_tiles.push_back(4);
        _floor_tiles.clear();
        if (_bg_map_ptr.has_value())
        {
            bn::span<const bn::regular_bg_map_cell> cells = _bg_map_ptr->cells_ref().value();
            for (int i = 0; i < 32 && i < cells.size(); ++i)
            {
                if (cells.at(i) != 0)
                {
                    _floor_tiles.push_back(cells.at(i));
                }
            }
        }
    }

    bool Level::is_in_sword_zone(const bn::fixed_point &position) const
    {
        constexpr int tile_size = TILE_SIZE;
        constexpr int map_offset = MAP_OFFSET;
        const bn::fixed zone_left = SWORD_ZONE_TILE_LEFT * tile_size - map_offset;
        const bn::fixed zone_right = SWORD_ZONE_TILE_RIGHT * tile_size - map_offset;
        const bn::fixed zone_top = SWORD_ZONE_TILE_TOP * tile_size - map_offset;
        const bn::fixed zone_bottom = SWORD_ZONE_TILE_BOTTOM * tile_size - map_offset;
        return position.x() >= zone_left && position.x() < zone_right &&
               position.y() >= zone_top && position.y() < zone_bottom;
    }

    bool Level::is_in_merchant_interaction_zone(const bn::fixed_point &position) const
    {
        if (!_merchant_zone_center.has_value() || !_merchant_zone_enabled)
        {
            return false;
        }
        const bn::fixed_point &center = _merchant_zone_center.value();
        const bn::fixed zone_left = center.x() - MERCHANT_INTERACTION_ZONE_WIDTH / 2;
        const bn::fixed zone_right = center.x() + MERCHANT_INTERACTION_ZONE_WIDTH / 2;
        const bn::fixed zone_top = center.y() - MERCHANT_INTERACTION_ZONE_HEIGHT / 2;
        const bn::fixed zone_bottom = center.y() + MERCHANT_INTERACTION_ZONE_HEIGHT / 2;
        return position.x() >= zone_left && position.x() < zone_right &&
               position.y() >= zone_top && position.y() < zone_bottom;
    }

    bool Level::is_in_merchant_collision_zone(const bn::fixed_point &position) const
    {
        if (!_merchant_zone_center.has_value() || !_merchant_zone_enabled)
        {
            return false;
        }
        const bn::fixed_point &center = _merchant_zone_center.value();
        const bn::fixed zone_left = center.x() - MERCHANT_COLLISION_ZONE_WIDTH / 2;
        const bn::fixed zone_right = center.x() + MERCHANT_COLLISION_ZONE_WIDTH / 2;
        const bn::fixed zone_top = center.y() - MERCHANT_COLLISION_ZONE_HEIGHT / 2;
        const bn::fixed zone_bottom = center.y() + MERCHANT_COLLISION_ZONE_HEIGHT / 2;
        return position.x() >= zone_left && position.x() < zone_right &&
               position.y() >= zone_top && position.y() < zone_bottom;
    }

    void Level::set_merchant_zone(const bn::fixed_point &center) { _merchant_zone_center = center; }
    void Level::clear_merchant_zone() { _merchant_zone_center.reset(); }
    void Level::set_merchant_zone_enabled(bool enabled) { _merchant_zone_enabled = enabled; }

    bool Level::is_position_valid(const bn::fixed_point &p) const
    {
        if (!_bg_map_ptr || is_in_merchant_collision_zone(p))
            return 0;
        auto c = _bg_map_ptr->cells_ref().value();
        int w = _bg_map_ptr->dimensions().width(), h = _bg_map_ptr->dimensions().height();
        bn::fixed_point pts[] = {{p.x() - PLAYER_HITBOX_REDUCED_WIDTH / 2, p.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET}, {p.x() + PLAYER_HITBOX_REDUCED_WIDTH / 2 - 1, p.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET}, {p.x() - PLAYER_HITBOX_REDUCED_WIDTH / 2, p.y() + PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET - 1}, {p.x() + PLAYER_HITBOX_REDUCED_WIDTH / 2 - 1, p.y() + PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET - 1}, {p.x(), p.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET}, {p.x() - PLAYER_HITBOX_WIDTH / 4, p.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET}, {p.x() + PLAYER_HITBOX_WIDTH / 4, p.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET}};
        for (auto &pt : pts)
        {
            int cx = ((pt.x() + w * 4) / 8).integer(), cy = ((pt.y() + h * 4) / 8).integer();
            if (cx < 0 || cx >= w || cy < 0 || cy >= h)
                return 0;
            int idx = cy * w + cx;
            if (idx < 0 || idx >= c.size())
                return 0;
            int tidx = bn::regular_bg_map_cell_info(c.at(idx)).tile_index();
            for (int z : _zone_tiles)
                if (tidx == z && z != 3 && z != 4)
                    return 0;
        }
        return 1;
    }

    // =========================================================================
    // Minimap Implementation
    // =========================================================================

    Minimap::Minimap(bn::fixed_point pos, bn::regular_bg_map_ptr map, bn::camera_ptr &camera)
        : _player_dot(bn::sprite_items::minimap_player.create_sprite(pos.x(), pos.y() + MINIMAP_VERTICAL_OFFSET)),
          _position(bn::fixed_point(pos.x(), pos.y() + MINIMAP_VERTICAL_OFFSET))
    {

        _player_dot.set_bg_priority(0);
        _player_dot.set_z_order(Z_ORDER_MINIMAP_PLAYER);
        _player_dot.set_visible(true);
        (void)map;
        (void)camera;
    }

    void Minimap::update(bn::fixed_point p_pos, bn::fixed_point m_center, const bn::vector<Enemy, 16> &enemies)
    {
        bn::fixed rx = (p_pos.x() - m_center.x()) * MINIMAP_POSITION_SCALE, ry = (p_pos.y() - m_center.y()) * MINIMAP_POSITION_SCALE;
        _player_dot.set_position(_position.x() + rx, _position.y() + ry);
        for (int i = 0; i < enemies.size(); ++i)
        {
            bn::fixed_point ep = enemies[i].pos();
            if (i >= _enemy_dots.size())
            {
                auto s = bn::sprite_items::minimap_enemy.create_sprite(0, 0);
                s.set_bg_priority(0);
                s.set_z_order(Z_ORDER_MINIMAP_ENEMY);
                s.set_visible(1);
                s.set_blending_enabled(1);
                bn::blending::set_transparency_alpha(0.5);
                _enemy_dots.push_back(EnemyDot(std::move(s), &enemies[i]));
            }
            _enemy_dots[i].enemy = &enemies[i];
            _enemy_dots[i].sprite.set_position(_position.x() + (ep.x() - m_center.x()) * MINIMAP_POSITION_SCALE, _position.y() + (ep.y() - m_center.y()) * MINIMAP_POSITION_SCALE);
        }
        while (_enemy_dots.size() > enemies.size())
            _enemy_dots.pop_back();
    }

    void Minimap::set_visible(bool visible)
    {
        _player_dot.set_visible(visible);
        for (auto &ed : _enemy_dots)
            ed.sprite.set_visible(visible);
    }

    // =========================================================================
    // Bullet Implementation
    // =========================================================================

    Bullet::Bullet(bn::fixed_point p, bn::fixed_point v, bn::camera_ptr c, Direction d) : _pos(p), _velocity(v), _active(1), _hitbox(p.x(), p.y(), 2, 2), _lifetime(BULLET_LIFETIME)
    {
        _sprite = bn::sprite_items::hero_sword.create_sprite(p.x(), p.y(), 0);
        _sprite->set_camera(c);
        _sprite->set_z_order(Z_ORDER_BULLET);
        _sprite->set_scale(BULLET_SCALE);
        _sprite->set_bg_priority(0);
        _sprite->set_rotation_angle(d == Direction::UP ? 0 : (d == Direction::RIGHT ? 270 : (d == Direction::DOWN ? 180 : 90)));
    }

    void Bullet::update()
    {
        if (!_active)
            return;
        _pos += _velocity;
        if (_sprite)
            _sprite->set_position(_pos);
        _hitbox.set_x(_pos.x());
        _hitbox.set_y(_pos.y());
        if (--_lifetime <= 0)
            deactivate();
    }
    bool Bullet::check_enemy_collision(Enemy &e) { return _active && _hitbox.collides_with(e.get_hitbox()); }
    BulletManager::BulletManager() {}
    void BulletManager::fire_bullet(bn::fixed_point p, Direction d)
    {
        if (_shoot_cooldown > 0 || !_camera)
            return;
        _bullets.push_back(Bullet(p, calculate_bullet_velocity(d), *_camera, d));
        _shoot_cooldown = SHOOT_COOLDOWN_TIME;
    }
    void BulletManager::update_bullets()
    {
        if (_shoot_cooldown > 0)
            _shoot_cooldown--;
        for (int i = 0; i < _bullets.size();)
        {
            _bullets[i].update();
            if (!_bullets[i].is_active())
                _bullets.erase(_bullets.begin() + i);
            else
                i++;
        }
    }
    void BulletManager::clear_bullets()
    {
        _bullets.clear();
        _shoot_cooldown = 0;
    }
    void BulletManager::set_camera(bn::camera_ptr camera) { _camera = camera; }
    bn::fixed_point BulletManager::calculate_bullet_velocity(Direction d) const { return d == Direction::UP ? bn::fixed_point(0, -BULLET_SPEED) : (d == Direction::DOWN ? bn::fixed_point(0, BULLET_SPEED) : (d == Direction::LEFT ? bn::fixed_point(-BULLET_SPEED, 0) : bn::fixed_point(BULLET_SPEED, 0))); }

    // =========================================================================
    // Entity Implementation
    // =========================================================================

    Entity::Entity() : _pos(0, 0), _previous_pos(0, 0), _hitbox(0, 0, DEFAULT_ENTITY_WIDTH, DEFAULT_ENTITY_HEIGHT) {}
    Entity::Entity(bn::fixed_point p) : _pos(p), _previous_pos(p), _hitbox(p.x(), p.y(), DEFAULT_ENTITY_WIDTH, DEFAULT_ENTITY_HEIGHT) {}
    Entity::Entity(bn::sprite_ptr s) : _pos(s.x(), s.y()), _previous_pos(_pos), _sprite(s), _hitbox(_pos.x(), _pos.y(), DEFAULT_ENTITY_WIDTH, DEFAULT_ENTITY_HEIGHT) {}
    bn::fixed_point Entity::pos() const { return _pos; }
    bn::fixed_point Entity::previous_pos() const { return _previous_pos; }
    Hitbox Entity::get_hitbox() const { return _hitbox; }
    bool Entity::has_sprite() const { return _sprite.has_value(); }
    void Entity::set_position(bn::fixed_point n)
    {
        _previous_pos = _pos;
        _pos = n;
        update_hitbox();
        update_sprite_position();
    }
    void Entity::revert_position()
    {
        _pos = _previous_pos;
        update_hitbox();
        update_sprite_position();
    }
    void Entity::set_sprite_z_order(int z)
    {
        if (_sprite)
            _sprite->set_z_order(z);
    }
    void Entity::set_visible(bool v)
    {
        if (_sprite)
            _sprite->set_visible(v);
    }
    void Entity::set_camera(bn::camera_ptr c)
    {
        if (_sprite)
            _sprite->set_camera(c);
    }
    void Entity::update_hitbox()
    {
        _hitbox.set_x(_pos.x());
        _hitbox.set_y(_pos.y());
    }
    void Entity::update_sprite_position()
    {
        if (_sprite)
            _sprite->set_position(_pos);
    }

    // =========================================================================
    // Movement Implementation
    // =========================================================================

    Movement::Movement() : _dx(0), _dy(0), _current_state(State::IDLE), _facing_direction(Direction::DOWN) {}
    void Movement::move_right()
    {
        _dx += get_acc_const();
        _facing_direction = Direction::RIGHT;
        clamp_velocity();
        update_state();
    }
    void Movement::move_left()
    {
        _dx -= get_acc_const();
        _facing_direction = Direction::LEFT;
        clamp_velocity();
        update_state();
    }
    void Movement::move_up()
    {
        _dy -= get_acc_const();
        _facing_direction = Direction::UP;
        clamp_velocity();
        update_state();
    }
    void Movement::move_down()
    {
        _dy += get_acc_const();
        _facing_direction = Direction::DOWN;
        clamp_velocity();
        update_state();
    }
    void Movement::apply_friction()
    {
        _dx *= get_friction_const();
        _dy *= get_friction_const();
        if (bn::abs(_dx) < get_movement_threshold())
            _dx = 0;
        if (bn::abs(_dy) < get_movement_threshold())
            _dy = 0;
        update_state();
    }
    void Movement::reset()
    {
        _dx = _dy = 0;
        _current_state = State::IDLE;
        _facing_direction = Direction::DOWN;
    }
    void Movement::stop_movement()
    {
        _dx = _dy = 0;
        update_state();
    }
    void Movement::update_state() { _current_state = (_dx == 0 && _dy == 0) ? State::IDLE : State::WALKING; }
    void Movement::clamp_velocity()
    {
        bn::fixed m = get_max_speed();
        _dx = bn::clamp(_dx, -m, m);
        _dy = bn::clamp(_dy, -m, m);
    }

    EnemyMovement::EnemyMovement() : Movement() {}

    // =========================================================================
    // Direction Utils Implementation
    // =========================================================================

    namespace direction_utils
    {
        bn::fixed_point get_roll_offset(Direction dir, int frames_remaining, int total_frames)
        {
            bn::fixed momentum_factor = bn::fixed(frames_remaining) / bn::fixed(total_frames);
            momentum_factor = (momentum_factor * 0.7) + 0.3;
            bn::fixed current_speed = PLAYER_ROLL_SPEED * momentum_factor;
            switch (dir)
            {
            case Direction::UP:
                return bn::fixed_point(0, -current_speed);
            case Direction::DOWN:
                return bn::fixed_point(0, current_speed);
            case Direction::LEFT:
                return bn::fixed_point(-current_speed, 0);
            case Direction::RIGHT:
                return bn::fixed_point(current_speed, 0);
            default:
                return bn::fixed_point(0, 0);
            }
        }

        int get_gun_z_offset(Direction dir)
        {
            switch (dir)
            {
            case Direction::UP:
                return -1;
            case Direction::DOWN:
                return 1;
            case Direction::LEFT:
            case Direction::RIGHT:
            default:
                return -1;
            }
        }
    }

    // =========================================================================
    // Helpers
    // =========================================================================

    namespace
    {
        struct bg_map
        {
            static const int columns = MAP_COLUMNS;
            static const int rows = MAP_ROWS;
            static const int cells_count = MAP_CELLS_COUNT;
            BN_DATA_EWRAM static bn::regular_bg_map_cell cells[cells_count];
            bn::regular_bg_map_item map_item;
            int _background_tile;

            bg_map(int world_id = 0) : map_item(cells[0], bn::size(columns, rows))
            {
                _background_tile = 1;
                if (world_id == 1)
                {
                    _background_tile = 2;
                }
                for (int x = 0; x < columns; x++)
                {
                    for (int y = 0; y < rows; y++)
                    {
                        int cell_index = x + y * columns;
                        cells[cell_index] = bn::regular_bg_map_cell(_background_tile);
                    }
                }
            }
        };
        BN_DATA_EWRAM bn::regular_bg_map_cell bg_map::cells[bg_map::cells_count];
    }

    // =========================================================================
    // World Implementation
    // =========================================================================

    World::World() : _player(nullptr),
                     _level(nullptr),
                     _minimap(nullptr),
                     _sword_bg(bn::nullopt),
                     _merchant(nullptr),
                     _player_status_display(nullptr),
                     _camera(bn::nullopt),
                     _last_camera_direction(PlayerMovement::Direction::DOWN),
                     _direction_change_frames(0),
                     _current_world_id(0),
                     _shake_frames(0),
                     _shake_intensity(0),
                     _continuous_fire_frames(0),
                     _zoomed_out(false),
                     _current_zoom_scale(ZOOM_NORMAL_SCALE),
                     _zoom_affine_mat(bn::nullopt),
                     _gun_affine_mat(bn::nullopt),
                     _player_affine_mat(bn::nullopt),
                     _vfx_affine_mat(bn::nullopt)
    {
        bn::sprite_builder builder(bn::sprite_items::hero_sword);
        builder.set_bg_priority(1);
        _player = new Player(builder.release_build());
        _lookahead_current = bn::fixed_point(0, 0);
    }

    World::~World()
    {
        delete _player;
        delete _level;
        delete _minimap;
        delete _merchant;
    }

    fe::Scene fe::World::execute(bn::fixed_point spawn_location, int world_id)
    {
        _current_world_id = world_id;
        WorldStateManager &state_manager = WorldStateManager::instance();
        if (state_manager.has_saved_state(world_id))
        {
            WorldState saved_state = state_manager.load_world_state(world_id);
            spawn_location = saved_state.player_position;
        }

        bn::camera_ptr camera = bn::camera_ptr::create(0, 0);
        _camera = camera;

        bg_map bg_map_obj(world_id);
        bn::regular_bg_tiles_ptr tiles = bn::regular_bg_tiles_items::tiles.create_tiles();
        bn::bg_palette_ptr palette = bn::bg_palette_items::palette.create_palette();
        bn::regular_bg_map_ptr bg_map_ptr = bg_map_obj.map_item.create_map(tiles, palette);
        bn::regular_bg_ptr bg = bn::regular_bg_ptr::create(bg_map_ptr);
        bg.set_camera(camera);

        _level = new Level(bg_map_ptr);
        _player->spawn(spawn_location, camera);
        _camera_target_pos = spawn_location;
        camera.set_position(spawn_location.x(), spawn_location.y());
        _lookahead_current = bn::fixed_point(0, 0);

        _sword_bg = bn::affine_bg_items::sword.create_bg(0, 0);
        _sword_bg->set_visible(true);
        _sword_bg->set_wrapping_enabled(false);
        _sword_bg->set_camera(camera);

        bn::window outside_window = bn::window::outside();
        outside_window.set_show_bg(*_sword_bg, false);
        bn::rect_window internal_window = bn::rect_window::internal();
        internal_window.set_show_bg(*_sword_bg, true);
        internal_window.set_boundaries(-SWORD_HALF_WIDTH, -SWORD_HALF_HEIGHT, SWORD_HALF_WIDTH, SWORD_HALF_HEIGHT);

        _minimap = new Minimap(bn::fixed_point(100, -80), bg_map_ptr, camera);
        _player->set_camera(camera);

        bn::sprite_text_generator text_generator(common::variable_8x8_sprite_font);
        _init_world_specific_content(world_id, camera, bg, text_generator);

        while (true)
        {
            bn::core::update();
            if (bn::keypad::select_held() && bn::keypad::a_pressed())
            {
                if (_merchant)
                    _merchant->set_is_hidden(true);
                _save_current_state();
                return fe::Scene::MENU;
            }
            if (bn::keypad::select_pressed() && !bn::keypad::a_held() && !bn::keypad::b_held() && !bn::keypad::l_held() && !bn::keypad::r_held())
                _zoomed_out = !_zoomed_out;
            bn::fixed t_sc = _zoomed_out ? ZOOM_OUT_SCALE : ZOOM_NORMAL_SCALE;
            if (_current_zoom_scale != t_sc)
            {
                bn::fixed d = t_sc - _current_zoom_scale;
                if (bn::abs(d) < ZOOM_TRANSITION_SPEED)
                    _current_zoom_scale = t_sc;
                else
                    _current_zoom_scale += d * ZOOM_TRANSITION_SPEED * 2;
            }
            if (_current_zoom_scale != ZOOM_NORMAL_SCALE)
            {
                if (!_zoom_affine_mat)
                    _zoom_affine_mat = bn::sprite_affine_mat_ptr::create();
                _zoom_affine_mat->set_scale(_current_zoom_scale);
                if (!_gun_affine_mat)
                    _gun_affine_mat = bn::sprite_affine_mat_ptr::create();
                _gun_affine_mat->set_scale(_current_zoom_scale);
                if (!_player_affine_mat)
                    _player_affine_mat = bn::sprite_affine_mat_ptr::create();
                _player_affine_mat->set_scale(_current_zoom_scale);
                if (!_vfx_affine_mat)
                    _vfx_affine_mat = bn::sprite_affine_mat_ptr::create();
                _vfx_affine_mat->set_scale(_current_zoom_scale);
            }
            else
            {
                _zoom_affine_mat.reset();
                _gun_affine_mat.reset();
                _player_affine_mat.reset();
                _vfx_affine_mat.reset();
            }
            if (_merchant)
            {
                bool mwt = _merchant->is_talking();
                _merchant->update();
                fe::ZoneManager::set_merchant_zone_center(_merchant->pos());
                fe::ZoneManager::set_merchant_zone_enabled(!(_merchant->is_talking() || _player->listening()));
                _merchant->set_sprite_z_order(-_merchant->pos().y().integer());
                if (!_merchant->is_talking() && mwt)
                    _player->set_listening(0);
                if (fe::Hitbox::is_in_merchant_interaction_zone(_player->pos(), _merchant->pos()))
                {
                    _merchant->set_near_player(1);
                    if (bn::keypad::a_pressed() && !mwt && !_player->listening())
                    {
                        _player->set_listening(1);
                        _merchant->talk();
                    }
                }
                else
                    _merchant->set_near_player(0);
            }
            _player->update();
            _player->update_gun_position(_player->facing_direction());
            if (_player->is_firing())
            {
                _continuous_fire_frames++;
                if (_player->bullet_just_fired())
                    _player->clear_bullet_fired_flag();
            }
            else
                _continuous_fire_frames = 0;
            _player->update_z_order();
            if (!fe::ZoneManager::is_position_valid(_player->pos()))
                _player->revert_position();
            if (_minimap)
                _minimap->update(_player->pos(), {0, 0}, _enemies);
            PlayerMovement::Direction fdir = _player->facing_direction();
            bn::fixed_point dl = {0, 0};
            if (fdir == PlayerMovement::Direction::RIGHT)
                dl = {CAMERA_LOOKAHEAD_X, 0};
            else if (fdir == PlayerMovement::Direction::LEFT)
                dl = {-CAMERA_LOOKAHEAD_X, 0};
            else if (fdir == PlayerMovement::Direction::UP)
                dl = {0, -CAMERA_LOOKAHEAD_Y};
            else if (fdir == PlayerMovement::Direction::DOWN)
                dl = {0, CAMERA_LOOKAHEAD_Y};
            _lookahead_current += (dl - _lookahead_current) * CAMERA_LOOKAHEAD_SMOOTHING;
            bn::fixed_point cp = _camera ? bn::fixed_point(_camera->x(), _camera->y()) : bn::fixed_point(0, 0), ct = _player->pos() + _lookahead_current, ctt = ct - cp;
            bn::fixed nx = cp.x(), ny = cp.y();
            if (bn::abs(ctt.x()) > CAMERA_DEADZONE_X)
                nx = ct.x() - (ctt.x() > 0 ? CAMERA_DEADZONE_X : -CAMERA_DEADZONE_X);
            if (bn::abs(ctt.y()) > CAMERA_DEADZONE_Y)
                ny = ct.y() - (ctt.y() > 0 ? CAMERA_DEADZONE_Y : -CAMERA_DEADZONE_Y);
            if (_camera)
                _camera->set_position(bn::clamp(nx, bn::fixed(-MAP_OFFSET_X + 120), bn::fixed(MAP_OFFSET_X - 120)).integer(), bn::clamp(ny, bn::fixed(-MAP_OFFSET_Y + 80), bn::fixed(MAP_OFFSET_Y - 80)).integer());
            if (_sword_bg)
            {
                bn::fixed_point sp = {0, 0}, cp2(camera.x(), camera.y()), scp = sp - cp2;
                internal_window.set_boundaries(scp.y() - SWORD_HALF_HEIGHT, scp.x() - SWORD_HALF_WIDTH, scp.y() + SWORD_HALF_HEIGHT, scp.x() + SWORD_HALF_WIDTH);
                _sword_bg->set_priority(_player->pos().y() > sp.y() + 8 ? 2 : 0);
            }
            for (int i = 0; i < _enemies.size();)
            {
                Enemy &e = _enemies[i];
                bool ignore = _player->listening() || _player->get_hp() <= 0;
                e.update(_player->pos(), *_level, ignore);
                if (!ignore && fe::Collision::check_bb(_player->get_hitbox(), e.get_hitbox()) && !_player->is_state(PlayerMovement::State::ROLLING))
                {
                    _player->take_damage(1);
                    bn::fixed kx = (_player->pos().x() - e.get_position().x() > 0) ? 10 : -10;
                    _player->set_position(_player->pos() + bn::fixed_point(kx, 0));
                }
                if (_player->has_companion() && !_player->get_companion()->is_dead_independently() && fe::Collision::check_bb({_player->get_companion()->pos().x() - COMPANION_HITBOX_SIZE / 2, _player->get_companion()->pos().y() - COMPANION_HITBOX_SIZE / 2, COMPANION_HITBOX_SIZE, COMPANION_HITBOX_SIZE}, e.get_hitbox()))
                    _player->kill_companion();
                for (const auto &b : _player->bullets())
                    if (b.is_active() && b.get_hitbox().collides_with(e.get_hitbox()))
                    {
                        if (b.position().x() < e.get_position().x())
                            e.damage_from_left(1);
                        else
                            e.damage_from_right(1);
                        const_cast<Bullet &>(b).deactivate();
                        break;
                    }
                if (_player->is_attacking() && _player->get_melee_hitbox().collides_with(e.get_hitbox()))
                {
                    if (_player->get_melee_hitbox().x() < e.get_position().x())
                        e.damage_from_left(1);
                    else
                        e.damage_from_right(1);
                }
                if (e.is_ready_for_removal())
                    _enemies.erase(_enemies.begin() + i);
                else
                    i++;
            }
            if (_player->is_reset_required())
            {
                _player->reset();
                _level->reset();
                _enemies.clear();
                delete _minimap;
                _minimap = new Minimap({100, -80}, bg_map_ptr, camera);
                _player->spawn(spawn_location, camera);
                _init_world_specific_content(_current_world_id, camera, bg, text_generator);
                camera.set_position(0, 0);
                continue;
            }
            if (_sword_bg)
            {
                if (_current_zoom_scale != ZOOM_NORMAL_SCALE)
                {
                    _sword_bg->set_scale(_current_zoom_scale);
                    bn::fixed_point cp3(camera.x(), camera.y()), off = bn::fixed_point(0, 0) - cp3;
                    _sword_bg->set_position(cp3 + off * _current_zoom_scale);
                }
                else
                {
                    _sword_bg->set_scale(1);
                    _sword_bg->set_position(0, 0);
                }
            }
            if (_zoom_affine_mat)
            {
                bn::fixed_point cp4(camera.x(), camera.y());
                auto sc = [&](auto *s, bn::fixed_point wp, bool f = 0)
                { if (!s) return; if (f) _player_affine_mat->set_horizontal_flip(_player->facing_direction() == PlayerMovement::Direction::LEFT); s->set_affine_mat(f ? *_player_affine_mat : *_zoom_affine_mat); s->set_double_size_mode(bn::sprite_double_size_mode::ENABLED); s->set_position(cp4 + (wp - cp4) * _current_zoom_scale); };
                sc(_player->sprite(), _player->pos() + bn::fixed_point(0, PLAYER_SPRITE_Y_OFFSET), 1);
                sc(_player->vfx_sprite(), _player->vfx_sprite() ? _player->vfx_sprite()->position() : bn::fixed_point(0, 0));
                if (_player->gun_sprite() && _gun_affine_mat)
                {
                    _gun_affine_mat->set_rotation_angle(player_constants::GUN_ANGLES[int(_player->facing_direction())]);
                    _player->gun_sprite()->set_affine_mat(*_gun_affine_mat);
                    _player->gun_sprite()->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                    _player->gun_sprite()->set_position(cp4 + (_player->pos() + (_player->gun_sprite()->position() - _player->pos()) - cp4) * _current_zoom_scale);
                }
                if (_player->has_companion() && _player->get_companion())
                {
                    auto *c = _player->get_companion();
                    auto s = c->get_sprite();
                    s.set_affine_mat(*_zoom_affine_mat);
                    s.set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                    s.set_position(cp4 + (c->pos() - cp4) * _current_zoom_scale);
                    if (auto *pb = c->get_progress_bar_sprite())
                    {
                        pb->set_affine_mat(*_zoom_affine_mat);
                        pb->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                        pb->set_position(s.position() + bn::fixed_point(0, -16) * _current_zoom_scale);
                    }
                }
                for (Bullet &b : _player->bullets_mutable())
                    if (b.is_active() && b.get_sprite())
                        sc(b.get_sprite(), b.position());
                for (Enemy &e : _enemies)
                {
                    sc(e.get_sprite(), e.get_position());
                    sc(e.get_health_bar_sprite(), e.get_position() + bn::fixed_point(0, -12));
                }
                if (_merchant)
                    sc(_merchant->get_sprite(), _merchant->pos());
            }
            else
            {
                if (_player->sprite())
                    _player->sprite()->remove_affine_mat();
                if (_player->vfx_sprite())
                    _player->vfx_sprite()->remove_affine_mat();
                if (_player->gun_sprite())
                {
                    _player->gun_sprite()->remove_affine_mat();
                    _player->update_gun_position(_player->facing_direction());
                }
                if (_player->has_companion() && _player->get_companion())
                {
                    auto *c = _player->get_companion();
                    c->get_sprite().remove_affine_mat();
                    if (auto *pb = c->get_progress_bar_sprite())
                        pb->remove_affine_mat();
                    for (auto &ts : c->get_text_sprites())
                        ts.remove_affine_mat();
                    c->reset_text_positions();
                }
                for (Enemy &e : _enemies)
                    if (auto *s = e.get_sprite())
                        s->remove_affine_mat();
                if (_merchant && _merchant->get_sprite())
                    _merchant->get_sprite()->remove_affine_mat();
            }
        }
    }

    void World::_init_world_specific_content(int world_id, bn::camera_ptr &camera, bn::regular_bg_ptr &bg, bn::sprite_text_generator &text_generator)
    {
        _enemies.clear();
        if (_merchant)
        {
            delete _merchant;
            _merchant = nullptr;
        }

        switch (world_id)
        {
        case 0:
            _merchant = new MerchantNPC(bn::fixed_point(100, -50), camera, text_generator);
            _enemies.push_back(Enemy(0, -100, camera, bg, ENEMY_TYPE::SPEARGUARD, 3));
            _enemies.push_back(Enemy(50, -80, camera, bg, ENEMY_TYPE::SPEARGUARD, 3));
            _enemies.push_back(Enemy(-50, -120, camera, bg, ENEMY_TYPE::SPEARGUARD, 3));
            break;
        case 1:
            _enemies.push_back(Enemy(-100, -50, camera, bg, ENEMY_TYPE::SPEARGUARD, 2));
            _enemies.push_back(Enemy(80, -100, camera, bg, ENEMY_TYPE::SPEARGUARD, 2));
            break;
        case 2:
            _merchant = new MerchantNPC(bn::fixed_point(-80, 100), camera, text_generator);
            _enemies.push_back(Enemy(0, 0, camera, bg, ENEMY_TYPE::SPEARGUARD, 4));
            _enemies.push_back(Enemy(100, 20, camera, bg, ENEMY_TYPE::SPEARGUARD, 4));
            _enemies.push_back(Enemy(-100, 40, camera, bg, ENEMY_TYPE::SPEARGUARD, 4));
            _enemies.push_back(Enemy(0, 80, camera, bg, ENEMY_TYPE::SPEARGUARD, 4));
            break;
        default:
            _merchant = new MerchantNPC(bn::fixed_point(100, -50), camera, text_generator);
            _enemies.push_back(Enemy(0, -100, camera, bg, ENEMY_TYPE::SPEARGUARD, 3));
            break;
        }
    }

    void World::_save_current_state()
    {
        if (_player)
        {
            WorldStateManager &state_manager = WorldStateManager::instance();
            state_manager.save_world_state(_current_world_id, _player->pos(), _player->get_hp());
        }
    }

    void World::_update_camera_shake()
    {
        if (_shake_frames > 0 && _camera)
        {
            _shake_frames--;
            _shake_intensity *= 0.85;
            static int shake_seed = 1234;
            shake_seed = (shake_seed * 1664525 + 1013904223) % 32768;
            int shake_x_int = (shake_seed % 16) - 8;
            shake_seed = (shake_seed * 1664525 + 1013904223) % 32768;
            int shake_y_int = (shake_seed % 16) - 8;
            bn::fixed shake_x = bn::fixed(shake_x_int) * _shake_intensity / 4;
            bn::fixed shake_y = bn::fixed(shake_y_int) * _shake_intensity / 4;
            bn::fixed current_x = _camera->x();
            bn::fixed current_y = _camera->y();
            _camera->set_position(current_x + shake_x, current_y + shake_y);
        }
    }

    void World::trigger_screen_shake(int frames, bn::fixed intensity)
    {
        _shake_frames = frames;
        _shake_intensity = intensity;
    }

    // =========================================================================
    // Menu Implementation
    // =========================================================================

    Menu::Menu() : _selected_index(0) { _init_worlds(); }
    Menu::~Menu() {}

    void Menu::_init_worlds()
    {
        _worlds.clear();
        _worlds.push_back({0, "Main World", bn::fixed_point(MAIN_WORLD_SPAWN_X, MAIN_WORLD_SPAWN_Y), true});
        _worlds.push_back({1, "Forest Area", bn::fixed_point(FOREST_WORLD_SPAWN_X, FOREST_WORLD_SPAWN_Y), true});
    }

    void Menu::_update_display()
    {
        _text_sprites.clear();
        bn::sprite_text_generator tg(common::variable_8x8_sprite_font);
        tg.set_center_alignment();
        tg.set_bg_priority(0);
        tg.generate(0, MENU_TITLE_Y_POSITION, "WORLD SELECTION", _text_sprites);
        tg.generate(0, MENU_INSTRUCTIONS_Y_POSITION, "UP/DOWN: Select  A: Enter  B: Exit", _text_sprites);
        for (int i = 0; i < _worlds.size(); ++i)
        {
            int y = MENU_WORLD_LIST_START_Y + (i * MENU_WORLD_LIST_SPACING);
            if (!_worlds[i].is_unlocked)
                tg.generate(0, y, "??? LOCKED ???", _text_sprites);
            else
            {
                bn::string<64> l = (i == _selected_index ? "> " : "  ");
                l += _worlds[i].world_name;
                if (i == _selected_index)
                    l += " <";
                tg.generate(0, y, l, _text_sprites);
            }
        }
    }

    void Menu::_handle_input()
    {
        auto move = [&](int d)
        { _selected_index = (_selected_index + d + _worlds.size()) % _worlds.size(); while (!_worlds[_selected_index].is_unlocked) _selected_index = (_selected_index + d + _worlds.size()) % _worlds.size(); };
        if (bn::keypad::up_pressed())
            move(-1);
        if (bn::keypad::down_pressed())
            move(1);
    }

    fe::Scene Menu::execute(int &wid, bn::fixed_point &sl)
    {
        bn::bg_palettes::set_transparent_color(bn::color(MENU_BG_COLOR_R, MENU_BG_COLOR_G, MENU_BG_COLOR_B));
        while (1)
        {
            bn::core::update();
            _handle_input();
            _update_display();
            if (bn::keypad::a_pressed() && _worlds[_selected_index].is_unlocked)
            {
                wid = _worlds[_selected_index].world_id;
                sl = _worlds[_selected_index].spawn_location;
                return fe::Scene::WORLD;
            }
            if (bn::keypad::b_pressed())
                return fe::Scene::START;
        }
    }

    // =========================================================================
    // Start Implementation
    // =========================================================================

    Start::Start() : _selected_index(0) {}
    Start::~Start() {}

    void Start::_update_display()
    {
        _text_sprites.clear();
        bn::sprite_text_generator tg(common::variable_8x8_sprite_font);
        tg.set_center_alignment();
        tg.set_bg_priority(0);
        tg.generate(0, START_TITLE_Y_POSITION, "STRANDED", _text_sprites);
        const char *opts[] = {"Play Game", "Controls"};
        for (int i = 0; i < 2; ++i)
        {
            bn::string<64> l = (i == _selected_index ? "> " : "  ");
            l += opts[i];
            if (i == _selected_index)
                l += " <";
            tg.generate(0, START_OPTIONS_START_Y + i * START_OPTIONS_SPACING, l, _text_sprites);
        }
        tg.generate(0, START_INSTRUCTIONS_Y_POSITION, "UP/DOWN: Select  A: Confirm", _text_sprites);
    }

    fe::Scene Start::execute()
    {
        bn::bg_palettes::set_transparent_color(bn::color(MENU_BG_COLOR_R, MENU_BG_COLOR_G, MENU_BG_COLOR_B));
        while (1)
        {
            bn::core::update();
            if (bn::keypad::up_pressed())
                _selected_index = !_selected_index;
            if (bn::keypad::down_pressed())
                _selected_index = !_selected_index;
            _update_display();
            if (bn::keypad::a_pressed())
                return _selected_index ? fe::Scene::CONTROLS : fe::Scene::MENU;
        }
    }

    // =========================================================================
    // Controls Implementation
    // =========================================================================

    Controls::Controls() {}
    Controls::~Controls() {}

    void Controls::_update_display()
    {
        _text_sprites.clear();
        bn::sprite_text_generator tg(common::variable_8x8_sprite_font);
        tg.set_center_alignment();
        tg.set_bg_priority(0);
        tg.generate(0, CONTROLS_TITLE_Y_POSITION, "CONTROLS", _text_sprites);
        const char *c[] = {"D-PAD: Move", "A: Interact/Confirm", "B: Attack/Back", "L: Switch Weapon", "R: Roll/Dodge", "SELECT+START: Debug", "SELECT+A: Level Select"};
        for (int i = 0; i < 7; ++i)
            tg.generate(0, CONTROLS_LIST_START_Y + i * CONTROLS_LIST_SPACING, c[i], _text_sprites);
        tg.generate(0, CONTROLS_INSTRUCTIONS_Y_POSITION, "Press B to return", _text_sprites);
    }

    fe::Scene Controls::execute()
    {
        bn::bg_palettes::set_transparent_color(bn::color(MENU_BG_COLOR_R, MENU_BG_COLOR_G, MENU_BG_COLOR_B));
        _update_display();
        while (1)
        {
            bn::core::update();
            if (bn::keypad::b_pressed())
                return fe::Scene::START;
        }
    }

    // =========================================================================
    // WorldStateManager Implementation
    // =========================================================================

    void WorldStateManager::save_world_state(int world_id, const bn::fixed_point &player_pos, int player_health)
    {
        WorldState *existing_state = _find_state(world_id);
        if (existing_state)
        {
            existing_state->player_position = player_pos;
            existing_state->player_health = player_health;
            existing_state->is_saved = true;
        }
        else
        {
            WorldState new_state(world_id);
            new_state.player_position = player_pos;
            new_state.player_health = player_health;
            new_state.is_saved = true;
            _saved_states.push_back(new_state);
        }
    }

    WorldState WorldStateManager::load_world_state(int world_id)
    {
        WorldState *existing_state = _find_state(world_id);
        if (existing_state && existing_state->is_saved)
        {
            return *existing_state;
        }
        else
        {
            WorldState default_state(world_id);
            default_state.player_position = get_default_spawn(world_id);
            return default_state;
        }
    }

    bool WorldStateManager::has_saved_state(int world_id)
    {
        WorldState *existing_state = _find_state(world_id);
        return existing_state && existing_state->is_saved;
    }

    bn::fixed_point WorldStateManager::get_default_spawn(int world_id)
    {
        switch (world_id)
        {
        case 0:
            return bn::fixed_point(50, 100);
        case 1:
            return bn::fixed_point(100, 50);
        case 2:
            return bn::fixed_point(0, 150);
        case 3:
            return bn::fixed_point(-50, 75);
        default:
            return bn::fixed_point(50, 100);
        }
    }

    WorldState *WorldStateManager::_find_state(int world_id)
    {
        for (int i = 0; i < _saved_states.size(); ++i)
        {
            if (_saved_states[i].world_id == world_id)
            {
                return &_saved_states[i];
            }
        }
        return nullptr;
    }

} // namespace fe
