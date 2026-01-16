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

namespace fe {

    // =========================================================================
    // HUD Implementation
    // =========================================================================

    namespace {
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
        void set_soul_sprite_and_frame(bn::sprite_ptr& sprite, const bn::sprite_item& item, int frame) {
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
          _buff_menu_cooldown_timer(0) {

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

    void HUD::_configure_hud_sprite(bn::sprite_ptr &sprite) {
        sprite.set_bg_priority(HUD_BG_PRIORITY);
        sprite.remove_camera();
        sprite.set_visible(true);
        sprite.set_z_order(HUD_SPRITE_Z_ORDER);
    }

    int HUD::hp() const { return _hp; }

    void HUD::set_hp(int hp) {
        int old_hp = _hp;
        _hp = bn::max(0, bn::min(HUD_MAX_HP, hp));

        if (_health_bg.has_value()) {
            _health_bg->set_map(bn::regular_bg_items::healthbar.map_item(), _hp);
        }

        if (_hp > old_hp) {
            if (old_hp == 0 && _hp == 1) { play_health_gain_0_to_1(); }
            else if (old_hp == 1 && _hp == 2) { play_health_gain_1_to_2(); }
            else if (old_hp == 2 && _hp == 3) { play_health_gain_2_to_3(); }
            else if (old_hp < 3 && _hp == 3) {
                // Bulk heal to full
                if (!_resetting_health) {
                    play_health_gain_2_to_3(); // Animate final step
                } else {
                    // Reset case
                    set_soul_sprite_and_frame(_soul_sprite, bn::sprite_items::heart_normal_full, 0);
                    _soul_action.reset();
                    _defense_buff_active = false; // Reset buffs on full reset
                    _silver_soul_active = false;
                }
            }
        }
        else if (_hp < old_hp) {
            if (old_hp == 3 && _hp == 2) { play_health_loss_3_to_2(); }
            else if (old_hp == 2 && _hp == 1) { play_health_loss_2_to_1(); }
            else if (old_hp == 1 && _hp == 0) { play_health_loss_1_to_0(); }
            else { play_health_loss_animation(); }
        }
    }

    void HUD::set_resetting_health(bool resetting) { _resetting_health = resetting; }

    bool HUD::is_soul_animation_complete() const { return !_soul_action.has_value() || _soul_action.value().done(); }

    void HUD::set_position(int x, int y) {
        if (_health_bg.has_value()) {
            _health_bg->set_position(x, y);
            int soul_x = x + HUD_SOUL_OFFSET_X;
            int soul_y = y + HUD_SOUL_OFFSET_Y;
            _soul_sprite.set_position(soul_x, soul_y);
        }
    }

    void HUD::set_visible(bool is_visible) {
        _is_visible = is_visible;
        if (_health_bg.has_value()) { _health_bg->set_visible(is_visible); }
        _weapon_sprite.set_visible(is_visible);
        _soul_sprite.set_visible(is_visible);
        _buff_menu_base.set_visible(is_visible);

        if (_ammo_sprite.has_value()) {
            bool show_ammo = is_visible && _weapon == WEAPON_TYPE::GUN && _displayed_ammo > 0;
            _ammo_sprite->set_visible(show_ammo);
        } else {
            _buff_menu_state = BUFF_MENU_STATE::CLOSED;
            for (int i = 0; i < BUFF_MENU_OPTION_COUNT; ++i) {
                _buff_menu_option_sprites[i].reset();
            }
        }
    }

    void HUD::activate_soul_animation() {
        // Defense Buff (Golden)
        _defense_buff_active = true;
        
        // Transform based on current HP
        const bn::sprite_item& transform_item = (_hp >= 3) ? bn::sprite_items::heart_golden_transform_full 
                                                           : bn::sprite_items::heart_golden_transform_half;
        
        _soul_sprite.set_item(transform_item);
        _soul_action = bn::create_sprite_animate_action_once(
            _soul_sprite, HUD_SOUL_ANIM_SPEED,
            transform_item.tiles_item(),
            0, 1, 2, 3, 4, 5, 6, 7); // 8 frames
    }

    void HUD::play_soul_damage_animation() {
        // Generic damage animation (Blink)
        // Use Full or Half blink depending on current state (before damage)
        const bn::sprite_item& blink_item = (_hp >= 3) ? bn::sprite_items::heart_normal_blink_full 
                                                       : bn::sprite_items::heart_normal_blink_half;
        
        _soul_sprite.set_item(blink_item);
        _soul_action = bn::create_sprite_animate_action_once(
            _soul_sprite, HUD_SOUL_ANIM_SPEED,
            blink_item.tiles_item(),
            0, 1, 2, 1, 0); // Blink
    }

    void HUD::play_health_loss_animation() {
        play_soul_damage_animation();
    }

    void HUD::activate_silver_soul() {
        // Energy Buff (Silver)
        _silver_soul_active = true;
        _silver_idle_timer = 0;
        
        const bn::sprite_item& transform_item = (_hp >= 3) ? bn::sprite_items::heart_silver_transform_full 
                                                           : bn::sprite_items::heart_silver_transform_half;

        _soul_sprite.set_item(transform_item);
        _soul_action = bn::create_sprite_animate_action_once(
            _soul_sprite, HUD_SOUL_ANIM_SPEED,
            transform_item.tiles_item(),
            0, 1, 2, 3, 4, 5, 6, 7); // 8 frames
    }

    void HUD::deactivate_silver_soul() {
        if (!_silver_soul_active) { return; }
        
        // Reverse transform
        const bn::sprite_item& transform_item = (_hp >= 3) ? bn::sprite_items::heart_silver_transform_full 
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

    void HUD::set_weapon(WEAPON_TYPE weapon) {
        _weapon = weapon;
        _weapon_sprite = bn::sprite_items::icon_gun.create_sprite(HUD_WEAPON_ICON_X, HUD_WEAPON_ICON_Y, 0);
        _configure_hud_sprite(_weapon_sprite);
        _update_ammo_display();
    }

    void HUD::set_weapon_frame(int frame) {
        if (_weapon == WEAPON_TYPE::GUN) { _weapon_sprite.set_tiles(bn::sprite_items::icon_gun.tiles_item(), frame); }
    }

    WEAPON_TYPE HUD::get_weapon() const { return _weapon; }

    void HUD::cycle_weapon() {
        if (_weapon == WEAPON_TYPE::GUN) { set_weapon(WEAPON_TYPE::SWORD); }
        else { set_weapon(WEAPON_TYPE::GUN); }
    }

    void HUD::set_ammo(int ammo_count) {
        _displayed_ammo = bn::max(0, bn::min(ammo_count, HUD_MAX_AMMO));
        _update_ammo_display();
    }

    void HUD::_update_ammo_display() {
        if (!_ammo_sprite.has_value()) { return; }
        bool show_ammo = (_weapon == WEAPON_TYPE::GUN);
        if (show_ammo) {
            int frame = HUD_MAX_AMMO - _displayed_ammo;
            _ammo_sprite->set_tiles(bn::sprite_items::ammo.tiles_item(), frame);
            _ammo_sprite->set_visible(_is_visible);
        } else {
            _ammo_sprite->set_visible(false);
        }
    }

    void HUD::update() {
        _update_soul_position();
        _update_soul_animations();
        _update_buff_menu_sprites();
        update_buff_menu_hold();
        update_buff_menu_cooldown();
    }

    void HUD::_update_soul_position() {
        if (_health_bg.has_value()) {
            bn::fixed_point bg_pos = _health_bg->position();
            _soul_sprite.set_position(bg_pos.x() + HUD_SOUL_OFFSET_X, bg_pos.y() + HUD_SOUL_OFFSET_Y);
        }
    }

    void HUD::_update_soul_animations() {
        if (!_soul_action.has_value()) return;

        if (!_soul_action->done()) {
            _soul_action->update();
            return;
        }

        // Animation complete, reset to idle state
        const bn::sprite_item* target_item = &bn::sprite_items::heart_empty;
        int frame_index = 0;

        // Health Gain Completion
        if (_health_gain_anim_active) {
            if (_hp >= 3) {
                 if (_silver_soul_active) target_item = &bn::sprite_items::heart_silver_full;
                 else if (_defense_buff_active) target_item = &bn::sprite_items::heart_golden_full;
                 else { target_item = &bn::sprite_items::heart_normal_spawn_full; frame_index = 9; }
            } else if (_hp == 2) {
                 if (_silver_soul_active) target_item = &bn::sprite_items::heart_silver_half;
                 else if (_defense_buff_active) target_item = &bn::sprite_items::heart_golden_half;
                 else target_item = &bn::sprite_items::heart_normal_full;
            } else if (_hp == 1) {
                 target_item = &bn::sprite_items::heart_normal_half;
            } else {
                 // 0 HP -> Nothing
                 target_item = &bn::sprite_items::heart_empty_spawn;
                 frame_index = 0;
            }
            _health_gain_anim_active = false;
        }
        // Health Loss Completion
        else if (_health_loss_anim_active) {
            if (_hp >= 3) {
                 target_item = &bn::sprite_items::heart_normal_spawn_full; frame_index = 9; 
            } else if (_hp == 2) {
                 if (_silver_soul_active) target_item = &bn::sprite_items::heart_silver_half;
                 else if (_defense_buff_active) target_item = &bn::sprite_items::heart_golden_half;
                 else target_item = &bn::sprite_items::heart_normal_full;
            } else if (_hp == 1) {
                 target_item = &bn::sprite_items::heart_normal_half;
            } else {
                 // 0 HP -> Nothing
                 target_item = &bn::sprite_items::heart_empty_spawn;
                 frame_index = 0;
            }
            _health_loss_anim_active = false;
        }
        // Buff Transition/Activation Completion
        else if (_silver_soul_reversing || _defense_buff_fading) {
            if (_hp >= 3) { target_item = &bn::sprite_items::heart_normal_spawn_full; frame_index = 9; }
            else if (_hp == 2) target_item = &bn::sprite_items::heart_normal_half;
            else if (_hp == 1) target_item = &bn::sprite_items::heart_normal_half;
            else { target_item = &bn::sprite_items::heart_empty_spawn; frame_index = 0; }
            _silver_soul_reversing = false;
            _defense_buff_fading = false;
        }
        else if (_silver_soul_active) {
             if (_hp >= 3) target_item = &bn::sprite_items::heart_silver_full;
             else if (_hp == 2) target_item = &bn::sprite_items::heart_silver_half;
             else if (_hp == 1) target_item = &bn::sprite_items::heart_normal_half; // Use normal half for silver empty
             else { target_item = &bn::sprite_items::heart_empty_spawn; frame_index = 0; }
        } 
        else if (_defense_buff_active) {
             if (_hp >= 3) target_item = &bn::sprite_items::heart_golden_full;
             else if (_hp == 2) target_item = &bn::sprite_items::heart_golden_half;
             else if (_hp == 1) target_item = &bn::sprite_items::heart_normal_half; // Use normal half for golden empty
             else { target_item = &bn::sprite_items::heart_empty_spawn; frame_index = 0; }
        }
        else {
            // Default idle state
            if (_hp >= 3) { target_item = &bn::sprite_items::heart_normal_spawn_full; frame_index = 9; }
            else if (_hp == 2) target_item = &bn::sprite_items::heart_normal_half;
            else if (_hp == 1) target_item = &bn::sprite_items::heart_normal_half;
            else { target_item = &bn::sprite_items::heart_empty_spawn; frame_index = 0; }
        }

        set_soul_sprite_and_frame(_soul_sprite, *target_item, frame_index);
        _soul_action.reset();
    }

    void HUD::toggle_buff_menu() {
        if (is_buff_menu_on_cooldown()) return;

        if (_buff_menu_state == BUFF_MENU_STATE::CLOSED) {
            _buff_menu_state = BUFF_MENU_STATE::OPEN;
            _selected_buff_option = 0;
            // Initialize option sprites
            for (int i = 0; i < BUFF_MENU_OPTION_COUNT; ++i) {
                int sprite_x = HUD_BUFF_MENU_BASE_X + buff_menu_offsets_x[i];
                int sprite_y = HUD_BUFF_MENU_BASE_Y + buff_menu_offsets_y[i];
                _buff_menu_option_sprites[i] = bn::sprite_items::hud_icons.create_sprite(sprite_x, sprite_y, buff_menu_icon_frames[i]);
                _configure_hud_sprite(_buff_menu_option_sprites[i].value());
                if (i != _selected_buff_option) { 
                    _buff_menu_option_sprites[i]->set_blending_enabled(true); 
                }
            }
        } else {
            _buff_menu_state = BUFF_MENU_STATE::CLOSED;
            for (int i = 0; i < BUFF_MENU_OPTION_COUNT; ++i) {
                _buff_menu_option_sprites[i].reset();
            }
        }
    }

    void HUD::_play_health_transition_anim(const bn::sprite_item& sprite_item, const int* frames, int frame_count, bool is_gain) {
        _health_gain_anim_active = is_gain;
        _health_loss_anim_active = !is_gain;
        _soul_sprite.set_item(sprite_item);
        
        // Create animation based on frame count
        if (frame_count == 14) {
             _soul_action = bn::create_sprite_animate_action_once(
                _soul_sprite, HUD_SOUL_ANIM_SPEED, sprite_item.tiles_item(),
                frames[0], frames[1], frames[2], frames[3], frames[4], frames[5], 
                frames[6], frames[7], frames[8], frames[9], frames[10], frames[11], frames[12], frames[13]);
        } else if (frame_count == 10) {
             _soul_action = bn::create_sprite_animate_action_once(
                _soul_sprite, HUD_SOUL_ANIM_SPEED, sprite_item.tiles_item(),
                frames[0], frames[1], frames[2], frames[3], frames[4], frames[5], 
                frames[6], frames[7], frames[8], frames[9]);
        } else if (frame_count == 9) {
             _soul_action = bn::create_sprite_animate_action_once(
                _soul_sprite, HUD_SOUL_ANIM_SPEED, sprite_item.tiles_item(),
                frames[0], frames[1], frames[2], frames[3], frames[4], frames[5], frames[6], frames[7], frames[8]);
        } else if (frame_count == 7) {
             _soul_action = bn::create_sprite_animate_action_once(
                _soul_sprite, HUD_SOUL_ANIM_SPEED, sprite_item.tiles_item(),
                frames[0], frames[1], frames[2], frames[3], frames[4], frames[5], frames[6]);
        } else if (frame_count == 5) {
             _soul_action = bn::create_sprite_animate_action_once(
                _soul_sprite, HUD_SOUL_ANIM_SPEED, sprite_item.tiles_item(),
                frames[0], frames[1], frames[2], frames[3], frames[4]);
        } else if (frame_count == 4) {
             _soul_action = bn::create_sprite_animate_action_once(
                _soul_sprite, HUD_SOUL_ANIM_SPEED, sprite_item.tiles_item(),
                frames[0], frames[1], frames[2], frames[3]);
        } else {
             // Fallback for custom lengths, assumes 6 for now
             _soul_action = bn::create_sprite_animate_action_once(
                _soul_sprite, HUD_SOUL_ANIM_SPEED, sprite_item.tiles_item(),
                frames[0], frames[1], frames[2], frames[3], frames[4], frames[5]);
        }
    }

    void HUD::play_health_gain_0_to_1() {
        // Spawn Empty (0->1 HP)
        static constexpr int frames[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        _play_health_transition_anim(bn::sprite_items::heart_empty_spawn, frames, 10, true);
    }

    void HUD::play_health_gain_1_to_2() {
        // Spawn Half (1->2 HP)
        static constexpr int frames[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        _play_health_transition_anim(bn::sprite_items::heart_normal_spawn_half, frames, 10, true);
    }
    
    void HUD::play_health_gain_2_to_3() {
        // Spawn Full (2->3 HP)
        static constexpr int frames[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        _play_health_transition_anim(bn::sprite_items::heart_normal_spawn_full, frames, 10, true);
    }

    void HUD::play_health_loss_3_to_2() {
        // Full -> Half (3->2 HP)
        static constexpr int frames[] = {10, 11, 12, 13};
        _play_health_transition_anim(bn::sprite_items::heart_normal_spawn_full, frames, 4, false);
    }

    void HUD::play_health_loss_2_to_1() {
        // Half -> Empty (2->1 HP) - Use blink animation
        play_health_loss_animation();
    }

    void HUD::play_health_loss_1_to_0() {
        // Empty -> Nothing (1->0 HP)
        static constexpr int frames[] = {10, 11, 12, 13};
        _play_health_transition_anim(bn::sprite_items::heart_empty_spawn, frames, 4, false);
    }

    bool HUD::is_buff_menu_on_cooldown() const { return _buff_menu_cooldown_timer > 0; }

    void HUD::_update_buff_menu_sprites() {
        if (_buff_menu_state == BUFF_MENU_STATE::OPEN) {
            for (int i = 0; i < BUFF_MENU_OPTION_COUNT; ++i) {
                if (_buff_menu_option_sprites[i].has_value()) { 
                    _buff_menu_option_sprites[i]->set_visible(_is_visible); 
                }
            }
        }
    }

    void HUD::_update_selection(int new_selection) {
        if (new_selection == _selected_buff_option || new_selection < 0 || new_selection >= BUFF_MENU_OPTION_COUNT) { return; }
        if (_buff_menu_option_sprites[_selected_buff_option].has_value()) { 
            _buff_menu_option_sprites[_selected_buff_option]->set_blending_enabled(true); 
        }
        _selected_buff_option = new_selection;
        if (_buff_menu_option_sprites[_selected_buff_option].has_value()) { 
            _buff_menu_option_sprites[_selected_buff_option]->set_blending_enabled(false); 
        }
    }

    void HUD::navigate_buff_menu_up() {
        if (_buff_menu_state == BUFF_MENU_STATE::OPEN) {
            int new_sel = buff_menu_nav[_selected_buff_option][NAV_UP];
            if (new_sel != -1) _update_selection(new_sel); 
        }
    }

    void HUD::navigate_buff_menu_down() {
        if (_buff_menu_state == BUFF_MENU_STATE::OPEN) {
            int new_sel = buff_menu_nav[_selected_buff_option][NAV_DOWN];
            if (new_sel != -1) _update_selection(new_sel); 
        }
    }

    void HUD::navigate_buff_menu_left() {
        if (_buff_menu_state == BUFF_MENU_STATE::OPEN) {
            int new_sel = buff_menu_nav[_selected_buff_option][NAV_LEFT];
            if (new_sel != -1) _update_selection(new_sel); 
        }
    }

    void HUD::navigate_buff_menu_right() {
        if (_buff_menu_state == BUFF_MENU_STATE::OPEN) {
            int new_sel = buff_menu_nav[_selected_buff_option][NAV_RIGHT];
            if (new_sel != -1) _update_selection(new_sel); 
        }
    }

    bool HUD::is_buff_menu_open() const { return _buff_menu_state == BUFF_MENU_STATE::OPEN; }

    int HUD::get_selected_buff() const { return _selected_buff_option; }

    void HUD::start_buff_menu_hold() {
        if (_buff_menu_state == BUFF_MENU_STATE::CLOSED && _buff_menu_hold_timer == 0) {
            _buff_menu_hold_timer = 1;
            _buff_menu_base.set_tiles(bn::sprite_items::temptest.tiles_item(), 8);
        }
    }

    void HUD::update_buff_menu_hold() {
        if (_buff_menu_hold_timer > 0 && _buff_menu_state == BUFF_MENU_STATE::CLOSED) {
            _buff_menu_hold_timer++;
            int frame = 8 - (_buff_menu_hold_timer * 7) / HUD_BUFF_MENU_HOLD_FRAMES;
            if (frame < 1) { frame = 1; }
            _buff_menu_base.set_tiles(bn::sprite_items::temptest.tiles_item(), frame);
        }
    }

    void HUD::cancel_buff_menu_hold() {
        _buff_menu_hold_timer = 0;
        _buff_menu_base.set_tiles(bn::sprite_items::temptest.tiles_item(), 0); 
    }

    bool HUD::is_buff_menu_hold_complete() const { return _buff_menu_hold_timer >= HUD_BUFF_MENU_HOLD_FRAMES; }

    bool HUD::is_buff_menu_holding() const { return _buff_menu_hold_timer > 0; }

    void HUD::start_buff_menu_cooldown() {
        _buff_menu_cooldown_timer = 1;
        _buff_menu_base.set_tiles(bn::sprite_items::temptest.tiles_item(), 1); 
    }

    void HUD::update_buff_menu_cooldown() {
        if (_buff_menu_cooldown_timer > 0) {
            _buff_menu_cooldown_timer++;
            int frame = 1 + (_buff_menu_cooldown_timer * 7) / HUD_BUFF_MENU_COOLDOWN_FRAMES;
            if (frame > 8) { frame = 8; }
            _buff_menu_base.set_tiles(bn::sprite_items::temptest.tiles_item(), frame);
            if (_buff_menu_cooldown_timer >= HUD_BUFF_MENU_COOLDOWN_FRAMES) {
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
    
    void Hitbox::get_collision_points(bn::fixed_point pos, fe::directions direction, bn::fixed_point points[4]) const {
        bn::fixed left = pos.x();
        bn::fixed right = pos.x() + _width - HITBOX_EDGE_OFFSET;
        bn::fixed top = pos.y();
        bn::fixed bottom = pos.y() + _height - HITBOX_EDGE_OFFSET;
        bn::fixed middle_x = pos.x() + _width / 2;
        bn::fixed quarter_x = pos.x() + _width / 4;
        bn::fixed middle_y = pos.y() + _height / 2;
        bn::fixed quarter_y = pos.y() + _height / 4;
        
        switch (direction) {
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

    bool Hitbox::contains_point(const bn::fixed_point &position) const {
        return position.x() >= x() && position.x() < x() + width() &&
               position.y() >= y() && position.y() < y() + height(); 
    }

    bool Hitbox::is_in_sword_zone(const bn::fixed_point &position) {
        const bn::fixed zone_left = fe::SWORD_ZONE_TILE_LEFT * fe::TILE_SIZE - fe::MAP_OFFSET;
        const bn::fixed zone_right = fe::SWORD_ZONE_TILE_RIGHT * fe::TILE_SIZE - fe::MAP_OFFSET;
        const bn::fixed zone_top = fe::SWORD_ZONE_TILE_TOP * fe::TILE_SIZE - fe::MAP_OFFSET;
        const bn::fixed zone_bottom = fe::SWORD_ZONE_TILE_BOTTOM * fe::TILE_SIZE - fe::MAP_OFFSET;
        return position.x() >= zone_left && position.x() < zone_right &&
               position.y() >= zone_top && position.y() < zone_bottom; 
    }

    bool Hitbox::is_in_merchant_interaction_zone(const bn::fixed_point &position, const bn::fixed_point &merchant_center) {
        bn::fixed_point zone_position = calculate_centered_position(merchant_center, MERCHANT_INTERACTION_ZONE_WIDTH, MERCHANT_INTERACTION_ZONE_HEIGHT);
        return position.x() >= zone_position.x() && position.x() < zone_position.x() + MERCHANT_INTERACTION_ZONE_WIDTH &&
               position.y() >= zone_position.y() && position.y() < zone_position.y() + MERCHANT_INTERACTION_ZONE_HEIGHT;
    }

    bool Hitbox::is_in_merchant_collision_zone(const bn::fixed_point &position, const bn::fixed_point &merchant_center) {
        bn::fixed_point zone_position = calculate_centered_position(merchant_center, MERCHANT_COLLISION_ZONE_WIDTH, MERCHANT_COLLISION_ZONE_HEIGHT);
        return position.x() >= zone_position.x() && position.x() < zone_position.x() + MERCHANT_COLLISION_ZONE_WIDTH &&
               position.y() >= zone_position.y() && position.y() < zone_position.y() + MERCHANT_COLLISION_ZONE_HEIGHT;
    }

    Hitbox Hitbox::create_player_hitbox(bn::fixed_point position) {
        bn::fixed_point hitbox_pos = calculate_centered_position(position, PLAYER_HITBOX_WIDTH, PLAYER_HITBOX_HEIGHT);
        return Hitbox(hitbox_pos.x(), hitbox_pos.y(), PLAYER_HITBOX_WIDTH, PLAYER_HITBOX_HEIGHT, HitboxType::PLAYER);
    }

    Hitbox Hitbox::create_merchant_interaction_zone(bn::fixed_point center) {
        bn::fixed_point position = calculate_centered_position(center, MERCHANT_INTERACTION_ZONE_WIDTH, MERCHANT_INTERACTION_ZONE_HEIGHT);
        return Hitbox(position.x(), position.y(), MERCHANT_INTERACTION_ZONE_WIDTH, MERCHANT_INTERACTION_ZONE_HEIGHT, HitboxType::MERCHANT_INTERACTION);
    }

    Hitbox Hitbox::create_sword_zone() {
        const bn::fixed zone_left = fe::SWORD_ZONE_TILE_LEFT * fe::TILE_SIZE - fe::MAP_OFFSET;
        const bn::fixed zone_top = fe::SWORD_ZONE_TILE_TOP * fe::TILE_SIZE - fe::MAP_OFFSET;
        const bn::fixed width = (fe::SWORD_ZONE_TILE_RIGHT - fe::SWORD_ZONE_TILE_LEFT) * fe::TILE_SIZE;
        const bn::fixed height = (fe::SWORD_ZONE_TILE_BOTTOM - fe::SWORD_ZONE_TILE_TOP) * fe::TILE_SIZE;
        return Hitbox(zone_left, zone_top, width, height, HitboxType::SWORD_ZONE);
    }

    bn::optional<bn::fixed_point> ZoneManager::_merchant_zone_center;
    bool ZoneManager::_merchant_zone_enabled = false;

    void ZoneManager::set_merchant_zone_center(const bn::fixed_point &center) {
        _merchant_zone_center = center;
        _merchant_zone_enabled = true; 
    }

    void ZoneManager::clear_merchant_zone() {
        _merchant_zone_center.reset();
        _merchant_zone_enabled = false; 
    }

    void ZoneManager::set_merchant_zone_enabled(bool enabled) { _merchant_zone_enabled = enabled && _merchant_zone_center.has_value(); }

    bn::optional<bn::fixed_point> ZoneManager::get_merchant_zone_center() { return _merchant_zone_center; }

    bool ZoneManager::is_merchant_zone_enabled() { return _merchant_zone_enabled && _merchant_zone_center.has_value(); }

    bool ZoneManager::is_position_valid(const bn::fixed_point &position) {
        if (Hitbox::is_in_sword_zone(position))
            return false;
        if (is_merchant_zone_enabled() && _merchant_zone_center.has_value()) {
            if (Hitbox::is_in_merchant_collision_zone(position, _merchant_zone_center.value()))
                return false; 
        }
        return true; 
    }

    // =========================================================================
    // Level Implementation
    // =========================================================================

    Level::Level(bn::regular_bg_map_ptr bg) {
        _bg_map_ptr = bg;
        _floor_tiles = {};
        _zone_tiles.clear();
        _zone_tiles.push_back(COLLISION_ZONE_TILE_INDEX);
        _zone_tiles.push_back(INTERACTION_ZONE_TILE_INDEX);
        bn::span<const bn::regular_bg_map_cell> cells = bg.cells_ref().value();
        for (int i = 0; i < 32 && i < cells.size(); ++i) {
            if (cells.at(i) != 0) { _floor_tiles.push_back(cells.at(i)); }
        }
    }

    bn::vector<int, 32> Level::floor_tiles() { return _floor_tiles; }

    void Level::add_zone_tile(int tile_index) {
        if (_zone_tiles.size() < _zone_tiles.max_size()) { _zone_tiles.push_back(tile_index); }
    }

    void Level::reset() {
        _zone_tiles.clear();
        _zone_tiles.push_back(4);
        _zone_tiles.push_back(4);
        _floor_tiles.clear();
        if (_bg_map_ptr.has_value()) {
            bn::span<const bn::regular_bg_map_cell> cells = _bg_map_ptr->cells_ref().value();
            for (int i = 0; i < 32 && i < cells.size(); ++i) {
                if (cells.at(i) != 0) { _floor_tiles.push_back(cells.at(i)); }
            }
        }
    }

    bool Level::is_in_sword_zone(const bn::fixed_point &position) const {
        constexpr int tile_size = TILE_SIZE;
        constexpr int map_offset = MAP_OFFSET;
        const bn::fixed zone_left = SWORD_ZONE_TILE_LEFT * tile_size - map_offset;
        const bn::fixed zone_right = SWORD_ZONE_TILE_RIGHT * tile_size - map_offset;
        const bn::fixed zone_top = SWORD_ZONE_TILE_TOP * tile_size - map_offset;
        const bn::fixed zone_bottom = SWORD_ZONE_TILE_BOTTOM * tile_size - map_offset;
        return position.x() >= zone_left && position.x() < zone_right &&
               position.y() >= zone_top && position.y() < zone_bottom; 
    }

    bool Level::is_in_merchant_interaction_zone(const bn::fixed_point &position) const {
        if (!_merchant_zone_center.has_value() || !_merchant_zone_enabled) { return false; }
        const bn::fixed_point &center = _merchant_zone_center.value();
        const bn::fixed zone_left = center.x() - MERCHANT_INTERACTION_ZONE_WIDTH / 2;
        const bn::fixed zone_right = center.x() + MERCHANT_INTERACTION_ZONE_WIDTH / 2;
        const bn::fixed zone_top = center.y() - MERCHANT_INTERACTION_ZONE_HEIGHT / 2;
        const bn::fixed zone_bottom = center.y() + MERCHANT_INTERACTION_ZONE_HEIGHT / 2;
        return position.x() >= zone_left && position.x() < zone_right &&
               position.y() >= zone_top && position.y() < zone_bottom; 
    }

    bool Level::is_in_merchant_collision_zone(const bn::fixed_point &position) const {
        if (!_merchant_zone_center.has_value() || !_merchant_zone_enabled) { return false; }
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

    bool Level::is_position_valid(const bn::fixed_point &position) const {
        if (!_bg_map_ptr.has_value()) { return true; }
        bn::span<const bn::regular_bg_map_cell> cells = _bg_map_ptr.value().cells_ref().value();
        int map_width = _bg_map_ptr.value().dimensions().width();
        int map_height = _bg_map_ptr.value().dimensions().height();
        
        bn::fixed_point top_left(position.x() - PLAYER_HITBOX_REDUCED_WIDTH / 2, position.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET);
        bn::fixed_point top_right(position.x() + PLAYER_HITBOX_REDUCED_WIDTH / 2 - 1, position.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET);
        bn::fixed_point bottom_left(position.x() - PLAYER_HITBOX_REDUCED_WIDTH / 2, position.y() + PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET - 1);
        bn::fixed_point bottom_right(position.x() + PLAYER_HITBOX_REDUCED_WIDTH / 2 - 1, position.y() + PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET - 1);
        bn::fixed_point middle_top(position.x(), position.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET);
        bn::fixed_point quarter_top_left(position.x() - PLAYER_HITBOX_WIDTH / 4, position.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET);
        bn::fixed_point quarter_top_right(position.x() + PLAYER_HITBOX_WIDTH / 4, position.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET);
        
        bn::fixed_point check_points[] = {
            top_left, top_right, bottom_left, bottom_right,
            middle_top, quarter_top_left, quarter_top_right
        };
        
        if (is_in_merchant_collision_zone(position)) { return false; }
        
        const int map_offset_x = (map_width * 4);
        const int map_offset_y = (map_height * 4);
        
        for (const auto &point : check_points) {
            int cell_x = ((point.x() + map_offset_x) / 8).integer();
            int cell_y = ((point.y() + map_offset_y) / 8).integer();
            
            if (cell_x < 0 || cell_x >= map_width || cell_y < 0 || cell_y >= map_height) { return false; }
            int cell_index = cell_y * map_width + cell_x;
            if (cell_index < 0 || cell_index >= cells.size()) { return false; }
            
            bn::regular_bg_map_cell cell = cells.at(cell_index);
            int tile_index = bn::regular_bg_map_cell_info(cell).tile_index();
            
            for (int zone_tile : _zone_tiles) {
                if (tile_index == zone_tile && zone_tile != 3 && zone_tile != 4) { return false; }
            }
        }
        return true; 
    }

    // =========================================================================
    // Minimap Implementation
    // =========================================================================

    Minimap::Minimap(bn::fixed_point pos, bn::regular_bg_map_ptr map, bn::camera_ptr &camera) 
        : _player_dot(bn::sprite_items::minimap_player.create_sprite(pos.x(), pos.y() + MINIMAP_VERTICAL_OFFSET)),
          _position(bn::fixed_point(pos.x(), pos.y() + MINIMAP_VERTICAL_OFFSET)) {
        
        _player_dot.set_bg_priority(0);
        _player_dot.set_z_order(Z_ORDER_MINIMAP_PLAYER);
        _player_dot.set_visible(true);
        (void)map;
        (void)camera; 
    }

    void Minimap::update(bn::fixed_point player_pos, bn::fixed_point map_center, const bn::vector<Enemy, 16> &enemies) {
        bn::fixed rel_x = (player_pos.x() - map_center.x()) * MINIMAP_POSITION_SCALE;
        bn::fixed rel_y = (player_pos.y() - map_center.y()) * MINIMAP_POSITION_SCALE;
        _player_dot.set_position(_position.x() + rel_x, _position.y() + rel_y);
        
        for (int i = 0; i < enemies.size(); ++i) {
            const Enemy &enemy = enemies[i];
            bn::fixed_point enemy_pos = enemy.pos();
            
            if (i >= _enemy_dots.size()) {
                auto sprite = bn::sprite_items::minimap_enemy.create_sprite(0, 0);
                sprite.set_bg_priority(0);
                sprite.set_z_order(Z_ORDER_MINIMAP_ENEMY);
                sprite.set_visible(true);
                sprite.set_blending_enabled(true);
                bn::blending::set_transparency_alpha(0.5);
                _enemy_dots.push_back(EnemyDot(std::move(sprite), &enemy)); 
            }
            
            _enemy_dots[i].enemy = &enemy;
            bn::fixed enemy_rel_x = (enemy_pos.x() - map_center.x()) * MINIMAP_POSITION_SCALE;
            bn::fixed enemy_rel_y = (enemy_pos.y() - map_center.y()) * MINIMAP_POSITION_SCALE;
            _enemy_dots[i].sprite.set_position(
                _position.x() + enemy_rel_x,
                _position.y() + enemy_rel_y); 
        }
        
        while (_enemy_dots.size() > enemies.size()) { _enemy_dots.pop_back(); }
    }

    void Minimap::set_visible(bool visible) {
        _player_dot.set_visible(visible);
        for (EnemyDot &enemy_dot : _enemy_dots) { enemy_dot.sprite.set_visible(visible); }
    }

    // =========================================================================
    // Bullet Implementation
    // =========================================================================

    Bullet::Bullet(bn::fixed_point pos, bn::fixed_point velocity, bn::camera_ptr camera, Direction direction)
        : _pos(pos), _velocity(velocity), _active(true), _hitbox(pos.x(), pos.y(), 2, 2), _lifetime(BULLET_LIFETIME) {
        
        _sprite = bn::sprite_items::hero_sword.create_sprite(_pos.x(), _pos.y(), 0);
        _sprite->set_camera(camera);
        _sprite->set_z_order(Z_ORDER_BULLET);
        _sprite->set_scale(BULLET_SCALE, BULLET_SCALE);
        _sprite->set_bg_priority(0);
        
        switch (direction) {
            case Direction::UP: _sprite->set_rotation_angle(0); break;
            case Direction::RIGHT: _sprite->set_rotation_angle(270); break;
            case Direction::DOWN: _sprite->set_rotation_angle(180); break;
            case Direction::LEFT: _sprite->set_rotation_angle(90); break;
            default: _sprite->set_rotation_angle(270); break; 
        }
    }

    void Bullet::update() {
        if (!_active) return;
        _pos += _velocity;
        if (_sprite) { _sprite->set_position(_pos); }
        _hitbox.set_x(_pos.x());
        _hitbox.set_y(_pos.y());
        _lifetime--;
        if (_lifetime <= 0) { deactivate(); }
    }

    bool Bullet::check_enemy_collision(Enemy &enemy) {
        if (!_active) return false;
        Hitbox enemy_hitbox = enemy.get_hitbox();
        return _hitbox.collides_with(enemy_hitbox); 
    }

    BulletManager::BulletManager() {}

    void BulletManager::fire_bullet(bn::fixed_point pos, Direction direction) {
        if (_shoot_cooldown > 0 || !_camera) return;
        bn::fixed_point velocity = calculate_bullet_velocity(direction);
        _bullets.push_back(Bullet(pos, velocity, *_camera, direction));
        _shoot_cooldown = SHOOT_COOLDOWN_TIME; 
    }

    void BulletManager::update_bullets() {
        if (_shoot_cooldown > 0) { _shoot_cooldown--; }
        for (int i = 0; i < _bullets.size();) {
            _bullets[i].update();
            if (!_bullets[i].is_active()) { _bullets.erase(_bullets.begin() + i); }
            else { i++; }
        }
    }

    void BulletManager::clear_bullets() {
        _bullets.clear();
        _shoot_cooldown = 0; 
    }

    void BulletManager::set_camera(bn::camera_ptr camera) { _camera = camera; }

    bn::fixed_point BulletManager::calculate_bullet_velocity(Direction direction) const {
        switch (direction) {
            case Direction::UP: return bn::fixed_point(0, -BULLET_SPEED);
            case Direction::DOWN: return bn::fixed_point(0, BULLET_SPEED);
            case Direction::LEFT: return bn::fixed_point(-BULLET_SPEED, 0);
            case Direction::RIGHT: return bn::fixed_point(BULLET_SPEED, 0);
            default: return bn::fixed_point(0, -BULLET_SPEED); 
        }
    }

    // =========================================================================
    // Entity Implementation
    // =========================================================================

    Entity::Entity() : _pos(0, 0), _previous_pos(0, 0), _hitbox(0, 0, DEFAULT_ENTITY_WIDTH, DEFAULT_ENTITY_HEIGHT) {}

    Entity::Entity(bn::fixed_point pos) : _pos(pos), _previous_pos(pos), _hitbox(pos.x(), pos.y(), DEFAULT_ENTITY_WIDTH, DEFAULT_ENTITY_HEIGHT) {}

    Entity::Entity(bn::sprite_ptr sprite) : _pos(sprite.x(), sprite.y()), _previous_pos(_pos), _sprite(sprite), _hitbox(_pos.x(), _pos.y(), DEFAULT_ENTITY_WIDTH, DEFAULT_ENTITY_HEIGHT) {}

    bn::fixed_point Entity::pos() const { return _pos; }
    bn::fixed_point Entity::previous_pos() const { return _previous_pos; }
    Hitbox Entity::get_hitbox() const { return _hitbox; }
    bool Entity::has_sprite() const { return _sprite.has_value(); }

    void Entity::set_position(bn::fixed_point new_pos) {
        _previous_pos = _pos;
        _pos = new_pos;
        update_hitbox();
        update_sprite_position(); 
    }

    void Entity::revert_position() {
        _pos = _previous_pos;
        update_hitbox();
        update_sprite_position(); 
    }

    void Entity::set_sprite_z_order(int z_order) {
        if (_sprite) { _sprite->set_z_order(z_order); }
    }

    void Entity::set_visible(bool visible) {
        if (_sprite) { _sprite->set_visible(visible); }
    }

    void Entity::set_camera(bn::camera_ptr camera) {
        if (_sprite) { _sprite->set_camera(camera); }
    }

    void Entity::update_hitbox() {
        _hitbox.set_x(_pos.x());
        _hitbox.set_y(_pos.y()); 
    }

    void Entity::update_sprite_position() {
        if (_sprite) { _sprite->set_position(_pos); }
    }

    // =========================================================================
    // Movement Implementation
    // =========================================================================

    Movement::Movement() : _dx(0), _dy(0), _current_state(State::IDLE), _facing_direction(Direction::DOWN) {}

    void Movement::move_right() {
        _dx += get_acc_const();
        _facing_direction = Direction::RIGHT;
        clamp_velocity();
        update_state(); 
    }

    void Movement::move_left() {
        _dx -= get_acc_const();
        _facing_direction = Direction::LEFT;
        clamp_velocity();
        update_state(); 
    }

    void Movement::move_up() {
        _dy -= get_acc_const();
        _facing_direction = Direction::UP;
        clamp_velocity();
        update_state(); 
    }

    void Movement::move_down() {
        _dy += get_acc_const();
        _facing_direction = Direction::DOWN;
        clamp_velocity();
        update_state(); 
    }

    void Movement::apply_friction() {
        _dx *= get_friction_const();
        _dy *= get_friction_const();
        if (bn::abs(_dx) < get_movement_threshold()) { _dx = 0; }
        if (bn::abs(_dy) < get_movement_threshold()) { _dy = 0; }
        update_state(); 
    }

    void Movement::reset() {
        _dx = 0;
        _dy = 0;
        _current_state = State::IDLE;
        _facing_direction = Direction::DOWN; 
    }

    void Movement::stop_movement() {
        _dx = 0;
        _dy = 0;
        update_state(); 
    }

    void Movement::update_state() {
        if (_dx == 0 && _dy == 0) { _current_state = State::IDLE; }
        else { _current_state = State::WALKING; }
    }

    void Movement::clamp_velocity() {
        bn::fixed max_speed = get_max_speed();
        if (_dx > max_speed) { _dx = max_speed; }
        else if (_dx < -max_speed) { _dx = -max_speed; }
        if (_dy > max_speed) { _dy = max_speed; }
        else if (_dy < -max_speed) { _dy = -max_speed; }
    }

    EnemyMovement::EnemyMovement() : Movement() { }

    // =========================================================================
    // Direction Utils Implementation
    // =========================================================================

    namespace direction_utils {
        bn::fixed_point get_roll_offset(Direction dir, int frames_remaining, int total_frames) {
            bn::fixed momentum_factor = bn::fixed(frames_remaining) / bn::fixed(total_frames);
            momentum_factor = (momentum_factor * 0.7) + 0.3;
            bn::fixed current_speed = PLAYER_ROLL_SPEED * momentum_factor;
            switch (dir) {
                case Direction::UP: return bn::fixed_point(0, -current_speed);
                case Direction::DOWN: return bn::fixed_point(0, current_speed);
                case Direction::LEFT: return bn::fixed_point(-current_speed, 0);
                case Direction::RIGHT: return bn::fixed_point(current_speed, 0);
                default: return bn::fixed_point(0, 0); 
            }
        }

        int get_gun_z_offset(Direction dir) {
            switch (dir) {
                case Direction::UP: return -1;
                case Direction::DOWN: return 1;
                case Direction::LEFT:
                case Direction::RIGHT:
                default: return -1; 
            }
        }
    }


    // =========================================================================
    // Helpers
    // =========================================================================

    namespace {
        struct bg_map {
            static const int columns = MAP_COLUMNS;
            static const int rows = MAP_ROWS;
            static const int cells_count = MAP_CELLS_COUNT;
            BN_DATA_EWRAM static bn::regular_bg_map_cell cells[cells_count];
            bn::regular_bg_map_item map_item;
            int _background_tile;
            
            bg_map(int world_id = 0) : map_item(cells[0], bn::size(columns, rows)) {
                _background_tile = 1;
                if (world_id == 1) { _background_tile = 2; }
                for (int x = 0; x < columns; x++) {
                    for (int y = 0; y < rows; y++) {
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
                     _vfx_affine_mat(bn::nullopt) {
        bn::sprite_builder builder(bn::sprite_items::hero_sword);
        builder.set_bg_priority(1);
        _player = new Player(builder.release_build());
        _lookahead_current = bn::fixed_point(0, 0); 
    }

    World::~World() {
        delete _player;
        delete _level;
        delete _minimap;
        delete _merchant; 
    }

    fe::Scene fe::World::execute(bn::fixed_point spawn_location, int world_id) {
        _current_world_id = world_id;
        WorldStateManager &state_manager = WorldStateManager::instance();
        if (state_manager.has_saved_state(world_id)) {
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
        
        while (true) {
            bn::core::update();
            
            if (bn::keypad::select_held() && bn::keypad::a_pressed()) {
                if (_merchant) { _merchant->set_is_hidden(true); }
                _save_current_state();
                return fe::Scene::MENU; 
            }
            
            if (bn::keypad::select_pressed() &&
                !bn::keypad::a_held() && !bn::keypad::b_held() &&
                !bn::keypad::l_held() && !bn::keypad::r_held()) { _zoomed_out = !_zoomed_out; }
            
            bn::fixed target_scale = _zoomed_out ? ZOOM_OUT_SCALE : ZOOM_NORMAL_SCALE;
            if (_current_zoom_scale != target_scale) {
                bn::fixed diff = target_scale - _current_zoom_scale;
                if (bn::abs(diff) < ZOOM_TRANSITION_SPEED) { _current_zoom_scale = target_scale; }
                else { _current_zoom_scale += diff * ZOOM_TRANSITION_SPEED * 2; }
            }
            
            if (_current_zoom_scale != ZOOM_NORMAL_SCALE) {
                if (!_zoom_affine_mat.has_value()) { _zoom_affine_mat = bn::sprite_affine_mat_ptr::create(); }
                _zoom_affine_mat->set_scale(_current_zoom_scale);
                if (!_gun_affine_mat.has_value()) { _gun_affine_mat = bn::sprite_affine_mat_ptr::create(); }
                _gun_affine_mat->set_scale(_current_zoom_scale);
                if (!_player_affine_mat.has_value()) { _player_affine_mat = bn::sprite_affine_mat_ptr::create(); }
                _player_affine_mat->set_scale(_current_zoom_scale);
                if (!_vfx_affine_mat.has_value()) { _vfx_affine_mat = bn::sprite_affine_mat_ptr::create(); }
                _vfx_affine_mat->set_scale(_current_zoom_scale); 
            } else {
                _zoom_affine_mat.reset();
                _gun_affine_mat.reset();
                _player_affine_mat.reset();
                _vfx_affine_mat.reset(); 
            }
            
            bool merchant_was_talking = false;
            if (_merchant) {
                merchant_was_talking = _merchant->is_talking();
                _merchant->update();
                fe::ZoneManager::set_merchant_zone_center(_merchant->pos());
                bool conversation_active = _merchant->is_talking() || _player->listening();
                fe::ZoneManager::set_merchant_zone_enabled(!conversation_active);
                int merchant_z = -_merchant->pos().y().integer();
                _merchant->set_sprite_z_order(merchant_z); 
            }
            
            if (_merchant && !_merchant->is_talking() && merchant_was_talking) { _player->set_listening(false); }
            
            if (_merchant && fe::Hitbox::is_in_merchant_interaction_zone(_player->pos(), _merchant->pos())) {
                _merchant->set_near_player(true);
                if (bn::keypad::a_pressed() && !merchant_was_talking && !_player->listening()) {
                    _player->set_listening(true);
                    _merchant->talk(); 
                }
            } else {
                if (_merchant) { _merchant->set_near_player(false); }
            }
            
            _player->update();
            _player->update_gun_position(_player->facing_direction());
            
            if (_player->is_firing()) {
                _continuous_fire_frames++;
                if (_player->bullet_just_fired()) { _player->clear_bullet_fired_flag(); }
            } else { 
                _continuous_fire_frames = 0; 
            }
            
            _player->update_z_order();
            bn::fixed_point new_pos = _player->pos();
            if (!fe::ZoneManager::is_position_valid(new_pos)) { _player->revert_position(); }
            if (_minimap) { _minimap->update(_player->pos(), bn::fixed_point(0, 0), _enemies); }
            
            bn::fixed_point player_pos = _player->pos();
            PlayerMovement::Direction facing_dir = _player->facing_direction();
            bn::fixed_point desired_lookahead(0, 0);
            
            switch (facing_dir) {
                case PlayerMovement::Direction::RIGHT: desired_lookahead = bn::fixed_point(CAMERA_LOOKAHEAD_X, 0); break;
                case PlayerMovement::Direction::LEFT: desired_lookahead = bn::fixed_point(-CAMERA_LOOKAHEAD_X, 0); break;
                case PlayerMovement::Direction::UP: desired_lookahead = bn::fixed_point(0, -CAMERA_LOOKAHEAD_Y); break;
                case PlayerMovement::Direction::DOWN: desired_lookahead = bn::fixed_point(0, CAMERA_LOOKAHEAD_Y); break;
                default: break; 
            }
            
            _lookahead_current = _lookahead_current + (desired_lookahead - _lookahead_current) * CAMERA_LOOKAHEAD_SMOOTHING;
            bn::fixed_point camera_target = player_pos + _lookahead_current;
            bn::fixed_point current_camera_pos = _camera.has_value() ? bn::fixed_point(_camera->x(), _camera->y()) : bn::fixed_point(0, 0);
            bn::fixed_point camera_to_target = camera_target - current_camera_pos;
            bn::fixed new_camera_x = current_camera_pos.x();
            bn::fixed new_camera_y = current_camera_pos.y();
            
            if (bn::abs(camera_to_target.x()) > CAMERA_DEADZONE_X) { new_camera_x = camera_target.x() - (camera_to_target.x() > 0 ? CAMERA_DEADZONE_X : -CAMERA_DEADZONE_X); }
            if (bn::abs(camera_to_target.y()) > CAMERA_DEADZONE_Y) { new_camera_y = camera_target.y() - (camera_to_target.y() > 0 ? CAMERA_DEADZONE_Y : -CAMERA_DEADZONE_Y); }
            
            constexpr bn::fixed half_screen_x = 120;
            constexpr bn::fixed half_screen_y = 80;
            constexpr bn::fixed map_min_x = -MAP_OFFSET_X + half_screen_x;
            constexpr bn::fixed map_max_x = MAP_OFFSET_X - half_screen_x;
            constexpr bn::fixed map_min_y = -MAP_OFFSET_Y + half_screen_y;
            constexpr bn::fixed map_max_y = MAP_OFFSET_Y - half_screen_y;
            
            bn::fixed_point new_camera_pos(
                bn::clamp(new_camera_x, map_min_x, map_max_x),
                bn::clamp(new_camera_y, map_min_y, map_max_y));
            
            if (_camera.has_value()) {
                _camera->set_x(bn::fixed(new_camera_pos.x().integer()));
                _camera->set_y(bn::fixed(new_camera_pos.y().integer())); 
            }
            
            if (_sword_bg) {
                constexpr bn::fixed sword_sprite_x = 0;
                constexpr bn::fixed sword_sprite_y = 0;
                bn::rect_window &window_ref = internal_window;
                bn::fixed_point camera_pos(camera.x(), camera.y());
                bn::fixed_point sword_screen_pos = bn::fixed_point(sword_sprite_x, sword_sprite_y) - camera_pos;
                window_ref.set_boundaries(
                    sword_screen_pos.y() - SWORD_HALF_HEIGHT,
                    sword_screen_pos.x() - SWORD_HALF_WIDTH,
                    sword_screen_pos.y() + SWORD_HALF_HEIGHT,
                    sword_screen_pos.x() + SWORD_HALF_WIDTH
                );
                const int sword_priority = (player_pos.y() > sword_sprite_y + 8) ? 2 : 0;
                _sword_bg->set_priority(sword_priority); 
            }
            
            for (int i = 0; i < _enemies.size();) {
                Enemy &enemy = _enemies[i];
                bool player_should_be_ignored = _player->listening() || _player->get_hp() <= 0;
                enemy.update(_player->pos(), *_level, player_should_be_ignored);
                
                if (_player->get_hp() > 0 && !_player->listening()) {
                    Hitbox collision_hitbox = enemy.get_hitbox();
                    Hitbox player_hitbox = _player->get_hitbox();
                    if (fe::Collision::check_bb(player_hitbox, collision_hitbox)) {
                        if (!_player->is_state(PlayerMovement::State::ROLLING)) {
                            _player->take_damage(1);
                            bn::fixed_point knockback_vector = _player->pos() - enemy.get_position();
                            bn::fixed knockback_x = (knockback_vector.x() > 0) ? 10 : -10;
                            bn::fixed_point knockback(knockback_x, 0);
                            _player->set_position(_player->pos() + knockback); 
                        }
                    }
                }
                
                if (_player->has_companion() && !_player->get_companion()->is_dead_independently()) {
                    constexpr int COMPANION_HITBOX_HALF_SIZE = COMPANION_HITBOX_SIZE / 2;
                    PlayerCompanion *companion = _player->get_companion();
                    Hitbox enemy_hitbox = enemy.get_hitbox();
                    bn::fixed_point companion_pos = companion->pos();
                    Hitbox companion_hitbox(companion_pos.x() - COMPANION_HITBOX_HALF_SIZE, companion_pos.y() - COMPANION_HITBOX_HALF_SIZE, COMPANION_HITBOX_SIZE, COMPANION_HITBOX_SIZE);
                    if (fe::Collision::check_bb(companion_hitbox, enemy_hitbox)) { _player->kill_companion(); }
                }
                
                if (_player->bullets().size() > 0) {
                    const auto &bullets = _player->bullets();
                    Hitbox enemy_hitbox = enemy.get_hitbox();
                    for (const auto &bullet : bullets) {
                        if (bullet.is_active()) {
                            Hitbox bullet_hitbox = bullet.get_hitbox();
                            if (bullet_hitbox.collides_with(enemy_hitbox)) {
                                bool damage_from_left = bullet.position().x() < enemy.get_position().x();
                                if (damage_from_left) { enemy.damage_from_left(1); }
                                else { enemy.damage_from_right(1); }
                                const_cast<Bullet &>(bullet).deactivate();
                                break; 
                            }
                        }
                    }
                }
                
                if (_player->is_attacking()) {
                    Hitbox melee_hitbox = _player->get_melee_hitbox();
                    Hitbox enemy_hitbox = enemy.get_hitbox();
                    if (melee_hitbox.collides_with(enemy_hitbox)) {
                        bool damage_from_left = melee_hitbox.x() < enemy.get_position().x();
                        if (damage_from_left) { enemy.damage_from_left(1); }
                        else { enemy.damage_from_right(1); }
                    }
                }
                
                if (enemy.is_ready_for_removal()) { _enemies.erase(_enemies.begin() + i); }
                else { i++; }
            }
            
            if (_player->is_reset_required()) {
                _player->reset();
                _level->reset();
                _enemies.clear();
                delete _minimap;
                _minimap = new Minimap(bn::fixed_point(100, -80), bg_map_ptr, camera);
                _player->spawn(spawn_location, camera);
                _enemies.push_back(Enemy(0, -100, camera, bg, ENEMY_TYPE::SPEARGUARD, 3));
                _enemies.push_back(Enemy(50, -80, camera, bg, ENEMY_TYPE::SPEARGUARD, 3));
                _enemies.push_back(Enemy(-50, -120, camera, bg, ENEMY_TYPE::SPEARGUARD, 3));
                camera.set_position(0, 0);
                continue; 
            }
            
            if (_sword_bg && _current_zoom_scale != ZOOM_NORMAL_SCALE) {
                _sword_bg->set_scale(_current_zoom_scale);
                bn::fixed_point cam_pos = bn::fixed_point(camera.x(), camera.y());
                bn::fixed_point sword_world_pos = bn::fixed_point(0, 0);
                bn::fixed_point offset = sword_world_pos - cam_pos;
                bn::fixed_point scaled_pos = cam_pos + bn::fixed_point(offset.x() * _current_zoom_scale, offset.y() * _current_zoom_scale);
                _sword_bg->set_position(scaled_pos.x(), scaled_pos.y()); 
            } else if (_sword_bg) {
                _sword_bg->set_scale(1);
                _sword_bg->set_position(0, 0); 
            }
            
            if (_zoom_affine_mat.has_value()) {
                bn::fixed_point cam_pos = bn::fixed_point(camera.x(), camera.y());
                if (_player->sprite() && _player_affine_mat.has_value()) {
                    bool facing_left = _player->facing_direction() == PlayerMovement::Direction::LEFT;
                    _player_affine_mat->set_horizontal_flip(facing_left);
                    _player->sprite()->set_affine_mat(_player_affine_mat.value());
                    _player->sprite()->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                    bn::fixed_point player_world_pos = _player->pos() + bn::fixed_point(0, PLAYER_SPRITE_Y_OFFSET);
                    bn::fixed_point offset = player_world_pos - cam_pos;
                    bn::fixed_point scaled_pos = cam_pos + bn::fixed_point(offset.x() * _current_zoom_scale, offset.y() * _current_zoom_scale);
                    _player->sprite()->set_position(scaled_pos); 
                }
                
                if (_player->vfx_sprite() && _vfx_affine_mat.has_value()) {
                    bool facing_left = _player->facing_direction() == PlayerMovement::Direction::LEFT;
                    _vfx_affine_mat->set_horizontal_flip(facing_left);
                    _player->vfx_sprite()->set_affine_mat(_vfx_affine_mat.value());
                    _player->vfx_sprite()->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                    bn::fixed_point vfx_world_pos = _player->vfx_sprite()->position();
                    bn::fixed_point offset = vfx_world_pos - cam_pos;
                    bn::fixed_point scaled_pos = cam_pos + bn::fixed_point(offset.x() * _current_zoom_scale, offset.y() * _current_zoom_scale);
                    _player->vfx_sprite()->set_position(scaled_pos); 
                }
                
                if (_player->gun_sprite() && _gun_affine_mat.has_value()) {
                    int dir_idx = int(_player->facing_direction());
                    bn::fixed gun_rotation = player_constants::GUN_ANGLES[dir_idx];
                    _gun_affine_mat->set_rotation_angle(gun_rotation);
                    _player->gun_sprite()->set_affine_mat(_gun_affine_mat.value());
                    _player->gun_sprite()->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                    bn::fixed_point player_world_pos = _player->pos();
                    bn::fixed_point gun_screen_pos = _player->gun_sprite()->position();
                    bn::fixed_point gun_offset_from_player = gun_screen_pos - player_world_pos;
                    bn::fixed_point gun_world_pos = player_world_pos + gun_offset_from_player;
                    bn::fixed_point offset = gun_world_pos - cam_pos;
                    bn::fixed_point scaled_pos = cam_pos + bn::fixed_point(offset.x() * _current_zoom_scale, offset.y() * _current_zoom_scale);
                    _player->gun_sprite()->set_position(scaled_pos); 
                }
                
                if (_player->has_companion() && _player->get_companion()) {
                    PlayerCompanion *companion = _player->get_companion();
                    bn::sprite_ptr companion_sprite = companion->get_sprite();
                    companion_sprite.set_affine_mat(_zoom_affine_mat.value());
                    companion_sprite.set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                    bn::fixed_point comp_world_pos = companion->pos();
                    bn::fixed_point offset = comp_world_pos - cam_pos;
                    bn::fixed_point scaled_pos = cam_pos + bn::fixed_point(offset.x() * _current_zoom_scale, offset.y() * _current_zoom_scale);
                    companion_sprite.set_position(scaled_pos);
                    
                    bn::sprite_ptr *progress_bar = companion->get_progress_bar_sprite();
                    if (progress_bar) {
                        progress_bar->set_affine_mat(_zoom_affine_mat.value());
                        progress_bar->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                        bn::fixed_point pb_offset_from_companion = bn::fixed_point(0, -16);
                        bn::fixed_point scaled_pb_offset = bn::fixed_point(pb_offset_from_companion.x() * _current_zoom_scale, pb_offset_from_companion.y() * _current_zoom_scale);
                        progress_bar->set_position(scaled_pos + scaled_pb_offset);
                    }
                    
                    bn::vector<bn::sprite_ptr, 16> &text_sprites = companion->get_text_sprites();
                    const bn::vector<bn::fixed_point, 16> &original_offsets = companion->get_text_original_offsets();
                    if (!text_sprites.empty() && !original_offsets.empty()) {
                        bn::fixed_point text_center_world = companion->get_text_center();
                        bn::fixed_point text_center_offset = text_center_world - cam_pos;
                        bn::fixed_point scaled_text_center = cam_pos + bn::fixed_point(text_center_offset.x() * _current_zoom_scale, text_center_offset.y() * _current_zoom_scale);
                        for (int i = 0; i < text_sprites.size() && i < original_offsets.size(); ++i) {
                            text_sprites[i].set_affine_mat(_zoom_affine_mat.value());
                            text_sprites[i].set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                            bn::fixed_point scaled_offset = bn::fixed_point(original_offsets[i].x() * _current_zoom_scale, original_offsets[i].y() * _current_zoom_scale);
                            text_sprites[i].set_position(scaled_text_center + scaled_offset);
                        }
                    }
                }
                
                for (Bullet &bullet : _player->bullets_mutable()) {
                    if (bullet.is_active() && bullet.get_sprite()) {
                        bn::fixed_point bullet_world_pos = bullet.position();
                        bn::fixed_point offset = bullet_world_pos - cam_pos;
                        bn::fixed_point scaled_pos = cam_pos + bn::fixed_point(offset.x() * _current_zoom_scale, offset.y() * _current_zoom_scale);
                        bullet.get_sprite()->set_position(scaled_pos); 
                    }
                }
                
                for (Enemy &enemy : _enemies) {
                    if (enemy.has_sprite()) {
                        bn::sprite_ptr *enemy_sprite = enemy.get_sprite();
                        if (enemy_sprite) {
                            enemy_sprite->set_affine_mat(_zoom_affine_mat.value());
                            enemy_sprite->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                            bn::fixed_point enemy_world_pos = enemy.get_position();
                            bn::fixed_point offset = enemy_world_pos - cam_pos;
                            bn::fixed_point scaled_pos = cam_pos + bn::fixed_point(offset.x() * _current_zoom_scale, offset.y() * _current_zoom_scale);
                            enemy_sprite->set_position(scaled_pos); 
                        }
                    }
                    bn::sprite_ptr *health_bar = enemy.get_health_bar_sprite();
                    if (health_bar) {
                        bn::fixed_point enemy_world_pos = enemy.get_position();
                        bn::fixed_point hb_world_pos = enemy_world_pos + bn::fixed_point(0, -12);
                        bn::fixed_point offset = hb_world_pos - cam_pos;
                        bn::fixed_point scaled_pos = cam_pos + bn::fixed_point(offset.x() * _current_zoom_scale, offset.y() * _current_zoom_scale);
                        health_bar->set_position(scaled_pos); 
                    }
                }
                
                if (_merchant && _merchant->has_sprite()) {
                    bn::sprite_ptr *merchant_sprite = _merchant->get_sprite();
                    if (merchant_sprite) {
                        merchant_sprite->set_affine_mat(_zoom_affine_mat.value());
                        merchant_sprite->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                        bn::fixed_point merchant_world_pos = _merchant->pos();
                        bn::fixed_point offset = merchant_world_pos - cam_pos;
                        bn::fixed_point scaled_pos = cam_pos + bn::fixed_point(offset.x() * _current_zoom_scale, offset.y() * _current_zoom_scale);
                        merchant_sprite->set_position(scaled_pos); 
                    }
                }
            } else {
                if (_player->sprite() && _player->sprite()->affine_mat().has_value()) { _player->sprite()->remove_affine_mat(); }
                if (_player->vfx_sprite() && _player->vfx_sprite()->affine_mat().has_value()) { _player->vfx_sprite()->remove_affine_mat(); }
                if (_player->gun_sprite() && _player->gun_sprite()->affine_mat().has_value()) {
                    _player->gun_sprite()->remove_affine_mat();
                    _player->update_gun_position(_player->facing_direction()); 
                }
                
                if (_player->has_companion() && _player->get_companion()) {
                    PlayerCompanion *companion = _player->get_companion();
                    bn::sprite_ptr companion_sprite = companion->get_sprite();
                    if (companion_sprite.affine_mat().has_value()) { companion_sprite.remove_affine_mat(); }
                    bn::sprite_ptr *progress_bar = companion->get_progress_bar_sprite();
                    if (progress_bar && progress_bar->affine_mat().has_value()) { progress_bar->remove_affine_mat(); }
                    for (bn::sprite_ptr &text_sprite : companion->get_text_sprites()) {
                        if (text_sprite.affine_mat().has_value()) { text_sprite.remove_affine_mat(); }
                    }
                    companion->reset_text_positions(); 
                }
                
                for (Enemy &enemy : _enemies) {
                    if (enemy.has_sprite()) {
                        bn::sprite_ptr *enemy_sprite = enemy.get_sprite();
                        if (enemy_sprite && enemy_sprite->affine_mat().has_value()) { enemy_sprite->remove_affine_mat(); }
                    }
                }
                
                if (_merchant && _merchant->has_sprite()) {
                    bn::sprite_ptr *merchant_sprite = _merchant->get_sprite();
                    if (merchant_sprite && merchant_sprite->affine_mat().has_value()) { merchant_sprite->remove_affine_mat(); }
                }
            }
            if (_enemies.empty()) { } 
        }
    }

    void World::_init_world_specific_content(int world_id, bn::camera_ptr &camera, bn::regular_bg_ptr &bg, bn::sprite_text_generator &text_generator) {
        _enemies.clear();
        if (_merchant) {
            delete _merchant;
            _merchant = nullptr; 
        }
        
        switch (world_id) {
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

    void World::_save_current_state() {
        if (_player) {
            WorldStateManager &state_manager = WorldStateManager::instance();
            state_manager.save_world_state(_current_world_id, _player->pos(), _player->get_hp());
        }
    }

    void World::_update_camera_shake() {
        if (_shake_frames > 0 && _camera) {
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

    void World::trigger_screen_shake(int frames, bn::fixed intensity) {
        _shake_frames = frames;
        _shake_intensity = intensity; 
    }

    // =========================================================================
    // Menu Implementation
    // =========================================================================

    Menu::Menu() : _selected_index(0) { _init_worlds(); }
    Menu::~Menu() {}

    void Menu::_init_worlds() {
        _worlds.clear();
        _worlds.push_back({0, "Main World", bn::fixed_point(MAIN_WORLD_SPAWN_X, MAIN_WORLD_SPAWN_Y), true});
        _worlds.push_back({1, "Forest Area", bn::fixed_point(FOREST_WORLD_SPAWN_X, FOREST_WORLD_SPAWN_Y), true});
    }

    void Menu::_update_display() {
        _text_sprites.clear();
        bn::sprite_text_generator text_generator(common::variable_8x8_sprite_font);
        text_generator.set_center_alignment();
        text_generator.set_bg_priority(0);
        text_generator.generate(0, MENU_TITLE_Y_POSITION, "WORLD SELECTION", _text_sprites);
        text_generator.generate(0, MENU_INSTRUCTIONS_Y_POSITION, "UP/DOWN: Select  A: Enter  B: Exit", _text_sprites);
        for (int i = 0; i < _worlds.size(); ++i) {
            int y_pos = MENU_WORLD_LIST_START_Y + (i * MENU_WORLD_LIST_SPACING);
            if (!_worlds[i].is_unlocked) { text_generator.generate(0, y_pos, "??? LOCKED ???", _text_sprites); }
            else {
                bn::string<64> line;
                if (i == _selected_index) {
                    line = "> ";
                    line += _worlds[i].world_name;
                    line += " <"; }
                else {
                    line = "  ";
                    line += _worlds[i].world_name; }
                text_generator.generate(0, y_pos, line, _text_sprites); }
        }
    }

    void Menu::_handle_input() {
        if (bn::keypad::up_pressed()) {
            if (_selected_index > 0) {
                _selected_index--;
                while (_selected_index >= 0 && !_worlds[_selected_index].is_unlocked) { _selected_index--; }
                if (_selected_index < 0) {
                    _selected_index = _worlds.size() - 1;
                    while (_selected_index >= 0 && !_worlds[_selected_index].is_unlocked) { _selected_index--; }
                }
            }
            else {
                _selected_index = _worlds.size() - 1;
                while (_selected_index >= 0 && !_worlds[_selected_index].is_unlocked) { _selected_index--; }
            }
        }
        if (bn::keypad::down_pressed()) {
            if (_selected_index < _worlds.size() - 1) {
                _selected_index++;
                while (_selected_index < _worlds.size() && !_worlds[_selected_index].is_unlocked) { _selected_index++; }
                if (_selected_index >= _worlds.size()) {
                    _selected_index = 0;
                    while (_selected_index < _worlds.size() && !_worlds[_selected_index].is_unlocked) { _selected_index++; }
                }
            }
            else {
                _selected_index = 0;
                while (_selected_index < _worlds.size() && !_worlds[_selected_index].is_unlocked) { _selected_index++; }
            }
        }
    }

    fe::Scene Menu::execute(int &selected_world_id, bn::fixed_point &spawn_location) {
        bn::bg_palettes::set_transparent_color(bn::color(MENU_BG_COLOR_R, MENU_BG_COLOR_G, MENU_BG_COLOR_B));
        while (true) {
            bn::core::update();
            _handle_input();
            _update_display();
            if (bn::keypad::a_pressed()) {
                if (_selected_index >= 0 && _selected_index < _worlds.size() &&
                    _worlds[_selected_index].is_unlocked) {
                    selected_world_id = _worlds[_selected_index].world_id;
                    spawn_location = _worlds[_selected_index].spawn_location;
                    return fe::Scene::WORLD; }
            }
            if (bn::keypad::b_pressed()) { return fe::Scene::START; }
        }
    }

    // =========================================================================
    // Start Implementation
    // =========================================================================

    Start::Start() : _selected_index(0) {}
    Start::~Start() {}

    void Start::_update_display() {
        _text_sprites.clear();
        bn::sprite_text_generator text_generator(common::variable_8x8_sprite_font);
        text_generator.set_center_alignment();
        text_generator.set_bg_priority(0);
        text_generator.generate(0, START_TITLE_Y_POSITION, "STRANDED", _text_sprites);
        const char* options[] = {"Play Game", "Controls"};
        for (int i = 0; i < 2; ++i) {
            int y_pos = START_OPTIONS_START_Y + (i * START_OPTIONS_SPACING);
            bn::string<64> line;
            if (i == _selected_index) {
                line = "> ";
                line += options[i];
                line += " <"; }
            else {
                line = "  ";
                line += options[i]; }
            text_generator.generate(0, y_pos, line, _text_sprites); 
        }
        text_generator.generate(0, START_INSTRUCTIONS_Y_POSITION, "UP/DOWN: Select  A: Confirm", _text_sprites);
    }

    void Start::_handle_input() {
        if (bn::keypad::up_pressed()) { _selected_index = (_selected_index > 0) ? _selected_index - 1 : 1; }
        if (bn::keypad::down_pressed()) { _selected_index = (_selected_index < 1) ? _selected_index + 1 : 0; }
    }

    fe::Scene Start::execute() {
        bn::bg_palettes::set_transparent_color(bn::color(MENU_BG_COLOR_R, MENU_BG_COLOR_G, MENU_BG_COLOR_B));
        while (true) {
            bn::core::update();
            _handle_input();
            _update_display();
            if (bn::keypad::a_pressed()) {
                if (_selected_index == 0) { return fe::Scene::MENU; }
                else if (_selected_index == 1) { return fe::Scene::CONTROLS; }
            }
        }
    }

    // =========================================================================
    // Controls Implementation
    // =========================================================================

    Controls::Controls() {}
    Controls::~Controls() {}

    void Controls::_update_display() {
        _text_sprites.clear();
        bn::sprite_text_generator text_generator(common::variable_8x8_sprite_font);
        text_generator.set_center_alignment();
        text_generator.set_bg_priority(0);
        text_generator.generate(0, CONTROLS_TITLE_Y_POSITION, "CONTROLS", _text_sprites);
        const char* controls[] = {
            "D-PAD: Move",
            "A: Interact/Confirm",
            "B: Attack/Back",
            "L: Switch Weapon",
            "R: Roll/Dodge",
            "SELECT+START: Debug",
            "SELECT+A: Level Select"
        };
        int y_pos = CONTROLS_LIST_START_Y;
        for (const char* control : controls) {
            text_generator.generate(0, y_pos, control, _text_sprites);
            y_pos += CONTROLS_LIST_SPACING; 
        }
        text_generator.generate(0, CONTROLS_INSTRUCTIONS_Y_POSITION, "Press B to return", _text_sprites);
    }

    fe::Scene Controls::execute() {
        bn::bg_palettes::set_transparent_color(bn::color(MENU_BG_COLOR_R, MENU_BG_COLOR_G, MENU_BG_COLOR_B));
        _update_display();
        while (true) {
            bn::core::update();
            if (bn::keypad::b_pressed()) { return fe::Scene::START; }
        }
    }

    // =========================================================================
    // WorldStateManager Implementation
    // =========================================================================

    void WorldStateManager::save_world_state(int world_id, const bn::fixed_point& player_pos, int player_health) {
        WorldState* existing_state = _find_state(world_id);
        if(existing_state) {
            existing_state->player_position = player_pos;
            existing_state->player_health = player_health;
            existing_state->is_saved = true; 
        } else {
            WorldState new_state(world_id);
            new_state.player_position = player_pos;
            new_state.player_health = player_health;
            new_state.is_saved = true;
            _saved_states.push_back(new_state); 
        }
    }

    WorldState WorldStateManager::load_world_state(int world_id) {
        WorldState* existing_state = _find_state(world_id);
        if(existing_state && existing_state->is_saved) { return *existing_state; }
        else {
            WorldState default_state(world_id);
            default_state.player_position = get_default_spawn(world_id);
            return default_state; 
        }
    }

    bool WorldStateManager::has_saved_state(int world_id) {
        WorldState* existing_state = _find_state(world_id);
        return existing_state && existing_state->is_saved; 
    }

    bn::fixed_point WorldStateManager::get_default_spawn(int world_id) {
        switch(world_id) {
            case 0: return bn::fixed_point(50, 100);
            case 1: return bn::fixed_point(100, 50);
            case 2: return bn::fixed_point(0, 150);
            case 3: return bn::fixed_point(-50, 75);
            default: return bn::fixed_point(50, 100); 
        }
    }

    WorldState* WorldStateManager::_find_state(int world_id) {
        for(int i = 0; i < _saved_states.size(); ++i) {
            if(_saved_states[i].world_id == world_id) { return &_saved_states[i]; }
        }
        return nullptr; 
    }

} // namespace fe
