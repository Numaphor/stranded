#include "fe_hud.h"
#include "fe_constants.h"
#include "bn_fixed.h"
#include "bn_sprite_ptr.h"
#include "bn_math.h"

#include "bn_regular_bg_items_healthbar.h"
#include "bn_sprite_items_soul.h"
#include "bn_sprite_items_soul_half_1.h"
#include "bn_sprite_items_soul_half_2.h"
#include "bn_sprite_items_icon_gun.h"
#include "bn_sprite_items_soul_silver.h"
#include "bn_sprite_items_soul_silver_idle.h"
#include "bn_sprite_items_ammo.h"
#include "bn_sprite_items_temptest.h"
#include "bn_sprite_items_hud_icons.h"

namespace fe
{
    namespace
    {
        constexpr int BUFF_MENU_OPTION_COUNT = 3;
        constexpr int buff_menu_offsets_x[BUFF_MENU_OPTION_COUNT] = {HUD_BUFF_MENU_OPTION_HEAL_X, HUD_BUFF_MENU_OPTION_ENERGY_X, HUD_BUFF_MENU_OPTION_POWER_X};
        constexpr int buff_menu_offsets_y[BUFF_MENU_OPTION_COUNT] = {HUD_BUFF_MENU_OPTION_HEAL_Y, HUD_BUFF_MENU_OPTION_ENERGY_Y, HUD_BUFF_MENU_OPTION_POWER_Y};
        constexpr int buff_menu_icon_frames[BUFF_MENU_OPTION_COUNT] = {0, 1, 3};

        // Buff menu navigation table: [from_option][direction] = to_option (-1 = no change)
        constexpr int NAV_UP = 0, NAV_DOWN = 1, NAV_LEFT = 2, NAV_RIGHT = 3;
        constexpr int buff_menu_nav[BUFF_MENU_OPTION_COUNT][4] = {
            {-1, 2, 1, -1}, // Heal(0): up=none, down=Power(2), left=Energy(1), right=none
            {-1, 2, -1, 0}, // Energy(1): up=none, down=Power(2), left=none, right=Heal(0)
            {0, -1, 1, -1}  // Power(2): up=Heal(0), down=none, left=Energy(1), right=none
        };
    }

    namespace
    {
        void set_soul_sprite_and_frame(bn::sprite_ptr& sprite, const bn::sprite_item& item, int frame)
        {
            sprite.set_item(item);
            sprite.set_tiles(item.tiles_item().create_tiles(frame));
        }

        template<size_t N>
        bn::sprite_animate_action<10> create_soul_animation(bn::sprite_ptr& sprite, const bn::sprite_tiles_item& tiles, const int (&frames)[N])
        {
            return bn::create_sprite_animate_action_once(sprite, HUD_SOUL_ANIM_SPEED, tiles, frames[0], frames[1], frames[2], frames[3], frames[4], frames[5], frames[6], frames[7], frames[8], frames[9]);
        }
    }

    HUD::HUD()
        : _hp(HUD_MAX_HP), _is_visible(true), _weapon(WEAPON_TYPE::SWORD), _weapon_sprite(bn::sprite_items::icon_gun.create_sprite(HUD_WEAPON_ICON_X, HUD_WEAPON_ICON_Y, 0)), _soul_sprite(bn::sprite_items::soul.create_sprite(HUD_SOUL_INITIAL_X, HUD_SOUL_INITIAL_Y, 0)), _soul_positioned(false), _defense_buff_active(false), _defense_buff_fading(false), _silver_soul_active(false), _silver_soul_reversing(false), _silver_idle_timer(0), _health_gain_anim_active(false), _health_loss_anim_active(false), _resetting_health(false), _displayed_ammo(HUD_MAX_AMMO), _buff_menu_state(BUFF_MENU_STATE::CLOSED), _buff_menu_base(bn::sprite_items::temptest.create_sprite(HUD_BUFF_MENU_BASE_X, HUD_BUFF_MENU_BASE_Y, 0)), _selected_buff_option(0), _buff_menu_hold_timer(0), _buff_menu_cooldown_timer(0)
    {
        // Initialize healthbar background
        _health_bg = bn::regular_bg_items::healthbar.create_bg(
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

        // Initialize buff menu base sprite
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

    int HUD::hp() const
    {
        return _hp;
    }

    void HUD::set_hp(int hp)
    {
        int old_hp = _hp;
        _hp = bn::max(0, bn::min(HUD_MAX_HP, hp));

        if (_health_bg.has_value())
        {
            _health_bg->set_map(bn::regular_bg_items::healthbar.map_item(), _hp);
        }

        // Trigger health transition animations based on change
        if (_hp > old_hp)
        {
            // Health gained
            if (old_hp == 0 && _hp == 1)
            {
                play_health_gain_0_to_1();
            }
            else if (old_hp == 1 && _hp == 2)
            {
                play_health_gain_1_to_2();
            }
            else if (old_hp < 3 && _hp == 3)
            {
                // When reaching full health (3), activate soul shield (unless resetting)
                if (!_resetting_health)
                {
                    activate_soul_animation();
                }
                else
                {
                    // During reset, ensure soul is set to regular soul sprite with shield appearance
                    set_soul_sprite_and_frame(_soul_sprite, bn::sprite_items::soul, 4);
                    _soul_action.reset();
                    _defense_buff_active = true;
                }
            }
        }
        else if (_hp < old_hp)
        {
            // Health lost
            if (old_hp == 3 && _hp < 3)
            {
                // Play reverse soul animation when losing soul shield (3rd health slot)
                play_soul_damage_animation();
            }
            else if (old_hp == 2 && _hp == 1)
            {
                // Specific animation for 2->1 health loss
                play_health_loss_2_to_1();
            }
            else if (old_hp == 1 && _hp == 0)
            {
                // Specific animation for 1->0 health loss
                play_health_loss_1_to_0();
            }
            else
            {
                // Generic health loss animation for other cases
                play_health_loss_animation();
            }
        }
    }

    void HUD::set_resetting_health(bool resetting)
    {
        _resetting_health = resetting;
    }

    bool HUD::is_soul_animation_complete() const
    {
        // Soul animation is complete if there's no active animation or if the active animation is done
        return !_soul_action.has_value() || _soul_action.value().done();
    }

    void HUD::set_position(int x, int y)
    {
        if (_health_bg.has_value())
        {
            _health_bg->set_position(x, y);

            // Update soul position to follow healthbar
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

        // Update buff menu option sprites visibility
        if (_buff_menu_state == BUFF_MENU_STATE::OPEN)
        {
            for (int i = 0; i < BUFF_MENU_OPTION_COUNT; ++i)
            {
                if (_buff_menu_option_sprites[i].has_value())
                {
                    _buff_menu_option_sprites[i]->set_visible(is_visible);
                }
            }
        }
    }

    void HUD::activate_soul_animation()
    {
        _defense_buff_active = true;

        // Ensure we're using the regular soul sprite before playing shield animation
        _soul_sprite.set_item(bn::sprite_items::soul);

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
        _update_buff_menu_sprites();
    }

    void HUD::_update_soul_position()
    {
        if (_soul_positioned)
        {
            return;
        }

        // Position soul relative to healthbar
        if (_health_bg.has_value())
        {
            int soul_x = HUD_HEALTH_BG_X + HUD_SOUL_OFFSET_X;
            int soul_y = HUD_HEALTH_BG_Y + HUD_SOUL_OFFSET_Y;
            _soul_sprite.set_position(soul_x, soul_y);
            _soul_positioned = true;
        }
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
            set_soul_sprite_and_frame(_soul_sprite, bn::sprite_items::soul, 0);
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

        // Handle health gain animation completion - return to appropriate idle state
        if (_health_gain_anim_active && _soul_action.has_value() && _soul_action.value().done())
        {
            const bn::sprite_item& soul_item = (_hp == 1) ? bn::sprite_items::soul_half_1 : bn::sprite_items::soul_half_2;
            _soul_sprite.set_tiles(soul_item.tiles_item().create_tiles(0));
            _health_gain_anim_active = false;
        }

        // Handle health loss animation completion - return to appropriate idle state
        if (_health_loss_anim_active && _soul_action.has_value() && _soul_action.value().done())
        {
            // Return to appropriate soul sprite based on current health
            const bn::sprite_item& soul_item = (_hp <= 1) ? bn::sprite_items::soul_half_1 : bn::sprite_items::soul_half_2;
            set_soul_sprite_and_frame(_soul_sprite, soul_item, 0);
            _soul_action.reset();
            _health_loss_anim_active = false;
        }
    }

    void HUD::set_weapon(WEAPON_TYPE weapon)
    {
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

    void HUD::toggle_buff_menu()
    {
        if (_buff_menu_state == BUFF_MENU_STATE::CLOSED)
        {
            _buff_menu_state = BUFF_MENU_STATE::OPEN;

            // Create the 3 option sprites (Heal, Energy, and Power) using icons sprite
            for (int i = 0; i < BUFF_MENU_OPTION_COUNT; ++i)
            {
                int sprite_x = HUD_BUFF_MENU_BASE_X + buff_menu_offsets_x[i];
                int sprite_y = HUD_BUFF_MENU_BASE_Y + buff_menu_offsets_y[i];

                // Create sprite with the appropriate icon frame
                _buff_menu_option_sprites[i] = bn::sprite_items::hud_icons.create_sprite(sprite_x, sprite_y, buff_menu_icon_frames[i]);
                _configure_hud_sprite(_buff_menu_option_sprites[i].value());

                // Grey out non-selected options using blending
                if (i != _selected_buff_option)
                {
                    _buff_menu_option_sprites[i]->set_blending_enabled(true);
                }
            }
        }
        else
        {
            _buff_menu_state = BUFF_MENU_STATE::CLOSED;

            // Hide/destroy the option sprites
            for (int i = 0; i < BUFF_MENU_OPTION_COUNT; ++i)
            {
                _buff_menu_option_sprites[i].reset();
            }
        }
    }

    void HUD::_update_selection(int new_selection)
    {
        if (new_selection == _selected_buff_option || new_selection < 0 || new_selection >= BUFF_MENU_OPTION_COUNT)
        {
            return;
        }

        // Grey out previous selection (enable blending)
        if (_buff_menu_option_sprites[_selected_buff_option].has_value())
        {
            _buff_menu_option_sprites[_selected_buff_option]->set_blending_enabled(true);
        }

        _selected_buff_option = new_selection;

        // Highlight new selection (disable blending)
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
            if (new_sel != -1) _update_selection(new_sel);
        }
    }

    void HUD::navigate_buff_menu_down()
    {
        if (_buff_menu_state == BUFF_MENU_STATE::OPEN)
        {
            int new_sel = buff_menu_nav[_selected_buff_option][NAV_DOWN];
            if (new_sel != -1) _update_selection(new_sel);
        }
    }

    void HUD::navigate_buff_menu_left()
    {
        if (_buff_menu_state == BUFF_MENU_STATE::OPEN)
        {
            int new_sel = buff_menu_nav[_selected_buff_option][NAV_LEFT];
            if (new_sel != -1) _update_selection(new_sel);
        }
    }

    void HUD::navigate_buff_menu_right()
    {
        if (_buff_menu_state == BUFF_MENU_STATE::OPEN)
        {
            int new_sel = buff_menu_nav[_selected_buff_option][NAV_RIGHT];
            if (new_sel != -1) _update_selection(new_sel);
        }
    }

    bool HUD::is_buff_menu_open() const
    {
        return _buff_menu_state == BUFF_MENU_STATE::OPEN;
    }

    int HUD::get_selected_buff() const
    {
        return _selected_buff_option;
    }

    void HUD::start_buff_menu_hold()
    {
        if (_buff_menu_state == BUFF_MENU_STATE::CLOSED && _buff_menu_hold_timer == 0)
        {
            _buff_menu_hold_timer = 1; // Start hold timer
            // Set base icon to frame 8 (empty - start of hold animation)
            _buff_menu_base.set_tiles(bn::sprite_items::temptest.tiles_item(), 8);
        }
    }

    void HUD::update_buff_menu_hold()
    {
        if (_buff_menu_hold_timer > 0 && _buff_menu_state == BUFF_MENU_STATE::CLOSED)
        {
            _buff_menu_hold_timer++;

            // Calculate which animation frame to show (8-1 over the hold duration)
            // Frame 8 = empty, frames go to 1 = full (filling up as you hold)
            int frame = 8 - (_buff_menu_hold_timer * 7) / HUD_BUFF_MENU_HOLD_FRAMES;
            if (frame < 1)
            {
                frame = 1; // Clamp to full frame
            }
            _buff_menu_base.set_tiles(bn::sprite_items::temptest.tiles_item(), frame);
        }
    }

    void HUD::cancel_buff_menu_hold()
    {
        _buff_menu_hold_timer = 0;
        // Reset base icon to frame 0 (empty/idle)
        _buff_menu_base.set_tiles(bn::sprite_items::temptest.tiles_item(), 0);
    }

    bool HUD::is_buff_menu_hold_complete() const
    {
        return _buff_menu_hold_timer >= HUD_BUFF_MENU_HOLD_FRAMES;
    }

    bool HUD::is_buff_menu_holding() const
    {
        return _buff_menu_hold_timer > 0;
    }

    void HUD::start_buff_menu_cooldown()
    {
        _buff_menu_cooldown_timer = 1;
        // Start at frame 1 (full) and countdown to frame 8 (empty)
        _buff_menu_base.set_tiles(bn::sprite_items::temptest.tiles_item(), 1);
    }

    void HUD::update_buff_menu_cooldown()
    {
        if (_buff_menu_cooldown_timer > 0)
        {
            _buff_menu_cooldown_timer++;

            // Calculate which animation frame to show (1-8 over the cooldown duration)
            // Frame 1 = full, frames go to 8 = empty (draining as cooldown progresses)
            int frame = 1 + (_buff_menu_cooldown_timer * 7) / HUD_BUFF_MENU_COOLDOWN_FRAMES;
            if (frame > 8)
            {
                frame = 8;
            }
            _buff_menu_base.set_tiles(bn::sprite_items::temptest.tiles_item(), frame);

            // Check if cooldown is complete
            if (_buff_menu_cooldown_timer >= HUD_BUFF_MENU_COOLDOWN_FRAMES)
            {
                _buff_menu_cooldown_timer = 0;
                // Reset to idle frame 0 (empty)
                _buff_menu_base.set_tiles(bn::sprite_items::temptest.tiles_item(), 0);
            }
        }
    }

    bool HUD::is_buff_menu_on_cooldown() const
    {
        return _buff_menu_cooldown_timer > 0;
    }

    void HUD::_update_buff_menu_sprites()
    {
        // Ensure option sprites follow visibility state
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

    void HUD::_play_health_transition_anim(const bn::sprite_item& sprite_item, const int* frames, int frame_count, bool is_gain)
    {
        _health_gain_anim_active = is_gain;
        _health_loss_anim_active = !is_gain;
        
        _soul_sprite.set_item(sprite_item);
        
        // Create animation based on frame count
        if (frame_count == 6) {
            _soul_action = bn::create_sprite_animate_action_once(
                _soul_sprite, HUD_SOUL_ANIM_SPEED, sprite_item.tiles_item(),
                frames[0], frames[1], frames[2], frames[3], frames[4], frames[5]);
        } else {
            _soul_action = bn::create_sprite_animate_action_once(
                _soul_sprite, HUD_SOUL_ANIM_SPEED, sprite_item.tiles_item(),
                frames[0], frames[1], frames[2], frames[3], frames[4], frames[5], frames[6], frames[7], frames[8]);
        }
    }

    void HUD::play_health_gain_0_to_1()
    {
        static constexpr int frames[] = {5, 4, 3, 2, 1, 0};
        _play_health_transition_anim(bn::sprite_items::soul_half_1, frames, 6, true);
    }

    void HUD::play_health_gain_1_to_2()
    {
        static constexpr int frames[] = {5, 4, 3, 2, 1, 0};
        _play_health_transition_anim(bn::sprite_items::soul_half_2, frames, 6, true);
    }

    void HUD::play_health_loss_2_to_1()
    {
        static constexpr int frames[] = {0, 1, 2, 3, 4, 3, 2, 1, 0};
        _play_health_transition_anim(bn::sprite_items::soul_half_2, frames, 9, false);
    }

    void HUD::play_health_loss_1_to_0()
    {
        static constexpr int frames[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
        _play_health_transition_anim(bn::sprite_items::soul_half_1, frames, 9, false);
    }

    void HUD::play_health_loss_animation()
    {
        static constexpr int frames[] = {0, 1, 2, 3, 4, 3, 2, 1, 0};
        _play_health_transition_anim(bn::sprite_items::soul, frames, 9, false);
    }
}
#include "fe_hitbox.h"
#include "fe_constants.h"

namespace fe
{
    Hitbox::Hitbox() : _pos(0, 0), _width(0), _height(0) {}

    Hitbox::Hitbox(bn::fixed x, bn::fixed y, bn::fixed width, bn::fixed height) : _pos(x, y), _width(width), _height(height)
    {
    }

    Hitbox::Hitbox(bn::fixed x, bn::fixed y, bn::fixed width, bn::fixed height, HitboxType type)
        : _pos(x, y), _width(width), _height(height), _type(type)
    {
    }

    void Hitbox::set_x(bn::fixed x)
    {
        _pos.set_x(x);
    }

    void Hitbox::set_y(bn::fixed y)
    {
        _pos.set_y(y);
    }

    void Hitbox::set_position(bn::fixed_point position)
    {
        _pos = position;
    }

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

    void ZoneManager::set_merchant_zone_enabled(bool enabled)
    {
        _merchant_zone_enabled = enabled && _merchant_zone_center.has_value();
    }

    bn::optional<bn::fixed_point> ZoneManager::get_merchant_zone_center()
    {
        return _merchant_zone_center;
    }

    bool ZoneManager::is_merchant_zone_enabled()
    {
        return _merchant_zone_enabled && _merchant_zone_center.has_value();
    }

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
}
#include "fe_level.h"
#include "fe_constants.h"

#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_optional.h"
#include "bn_span.h"
#include "bn_log.h"
#include "bn_string.h"
#include "bn_regular_bg_map_cell_info.h"

namespace fe
{
    Level::Level(bn::regular_bg_map_ptr bg)
    {
        _bg_map_ptr = bg;
        _floor_tiles = {};
        // Initialize _zone_tiles and add default zone tiles using centralized constants
        _zone_tiles.clear();
        _zone_tiles.push_back(COLLISION_ZONE_TILE_INDEX);   // Tile index for all hitbox zones (sword zone, merchant collision)
        _zone_tiles.push_back(INTERACTION_ZONE_TILE_INDEX); // Tile index for all interaction zones (merchant interaction)

        bn::span<const bn::regular_bg_map_cell> cells = bg.cells_ref().value();

        for (int i = 0; i < 32 && i < cells.size(); ++i)
        {
            if (cells.at(i) != 0)
            {
                _floor_tiles.push_back(cells.at(i));
            }
        }
    }

    bn::vector<int, 32> Level::floor_tiles()
    {
        return _floor_tiles;
    }

    void Level::add_zone_tile(int tile_index)
    {
        if (_zone_tiles.size() < _zone_tiles.max_size())
        {
            _zone_tiles.push_back(tile_index);
        }
    }

    void Level::reset()
    {
        // Clear existing zone tiles
        _zone_tiles.clear();

        // Reinitialize with default zone tiles (4 and 4)
        _zone_tiles.push_back(4); // Tile index 4 is used for collision squares
        _zone_tiles.push_back(4); // Keep existing zone tile

        // Clear and repopulate floor tiles if we have a background map
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
        // Use centralized constants
        constexpr int tile_size = TILE_SIZE;
        constexpr int map_offset = MAP_OFFSET;

        // Calculate world coordinates from tile coordinates
        const bn::fixed zone_left = SWORD_ZONE_TILE_LEFT * tile_size - map_offset;
        const bn::fixed zone_right = SWORD_ZONE_TILE_RIGHT * tile_size - map_offset;
        const bn::fixed zone_top = SWORD_ZONE_TILE_TOP * tile_size - map_offset;
        const bn::fixed zone_bottom = SWORD_ZONE_TILE_BOTTOM * tile_size - map_offset;

        return position.x() >= zone_left && position.x() < zone_right &&
               position.y() >= zone_top && position.y() < zone_bottom;
    }

    bool Level::is_in_merchant_interaction_zone(const bn::fixed_point &position) const
    {
        // Return false if no merchant zone is set or zone is disabled (during conversations)
        if (!_merchant_zone_center.has_value() || !_merchant_zone_enabled)
        {
            return false;
        }

        const bn::fixed_point &center = _merchant_zone_center.value();

        // Calculate interaction zone boundaries (larger zone for conversation triggers)
        // Using same pattern as other zones: inclusive left/top, exclusive right/bottom
        const bn::fixed zone_left = center.x() - MERCHANT_INTERACTION_ZONE_WIDTH / 2;
        const bn::fixed zone_right = center.x() + MERCHANT_INTERACTION_ZONE_WIDTH / 2;
        const bn::fixed zone_top = center.y() - MERCHANT_INTERACTION_ZONE_HEIGHT / 2;
        const bn::fixed zone_bottom = center.y() + MERCHANT_INTERACTION_ZONE_HEIGHT / 2;

        return position.x() >= zone_left && position.x() < zone_right &&
               position.y() >= zone_top && position.y() < zone_bottom;
    }

    bool Level::is_in_merchant_collision_zone(const bn::fixed_point &position) const
    {
        // Return false if no merchant zone is set or zone is disabled (during conversations)
        if (!_merchant_zone_center.has_value() || !_merchant_zone_enabled)
        {
            return false;
        }

        const bn::fixed_point &center = _merchant_zone_center.value();

        // Calculate collision zone boundaries (smaller zone for physical blocking)
        // Using same pattern as other zones: inclusive left/top, exclusive right/bottom
        const bn::fixed zone_left = center.x() - MERCHANT_COLLISION_ZONE_WIDTH / 2;
        const bn::fixed zone_right = center.x() + MERCHANT_COLLISION_ZONE_WIDTH / 2;
        const bn::fixed zone_top = center.y() - MERCHANT_COLLISION_ZONE_HEIGHT / 2;
        const bn::fixed zone_bottom = center.y() + MERCHANT_COLLISION_ZONE_HEIGHT / 2;

        return position.x() >= zone_left && position.x() < zone_right &&
               position.y() >= zone_top && position.y() < zone_bottom;
    }

    void Level::set_merchant_zone(const bn::fixed_point &center)
    {
        _merchant_zone_center = center;
    }

    void Level::clear_merchant_zone()
    {
        _merchant_zone_center.reset();
    }

    void Level::set_merchant_zone_enabled(bool enabled)
    {
        _merchant_zone_enabled = enabled;
    }

    bool Level::is_position_valid(const bn::fixed_point &position) const
    {
        // If we don't have a background map, any position is valid
        if (!_bg_map_ptr.has_value())
        {
            return true;
        }

        // Get map dimensions from the cells span
        bn::span<const bn::regular_bg_map_cell> cells = _bg_map_ptr.value().cells_ref().value();
        int map_width = _bg_map_ptr.value().dimensions().width();
        int map_height = _bg_map_ptr.value().dimensions().height();

        // Define hitbox for collision - middle 32x16 pixels of 32x32 sprite
        // Starting 8 pixels from the top (since we're using middle portion)

        // Reduced width for left/right collision checks to allow getting closer

        // Check multiple points around the hitbox - with reduced width for left/right
        // Top-left corner
        bn::fixed_point top_left(position.x() - PLAYER_HITBOX_REDUCED_WIDTH / 2, position.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET);
        // Top-right corner
        bn::fixed_point top_right(position.x() + PLAYER_HITBOX_REDUCED_WIDTH / 2 - 1, position.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET);
        // Bottom-left corner
        bn::fixed_point bottom_left(position.x() - PLAYER_HITBOX_REDUCED_WIDTH / 2, position.y() + PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET - 1);
        // Bottom-right corner
        bn::fixed_point bottom_right(position.x() + PLAYER_HITBOX_REDUCED_WIDTH / 2 - 1, position.y() + PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET - 1);

        // Additional check for the middle-top point to prevent walking into zones from the north
        bn::fixed_point middle_top(position.x(), position.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET);

        // Additional points along the top edge for more accurate collision
        bn::fixed_point quarter_top_left(position.x() - PLAYER_HITBOX_WIDTH / 4, position.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET);
        bn::fixed_point quarter_top_right(position.x() + PLAYER_HITBOX_WIDTH / 4, position.y() - PLAYER_HITBOX_HEIGHT / 2 + PLAYER_HITBOX_VERTICAL_OFFSET);

        // Check each point for collision - optimized to reduce redundant calculations
        bn::fixed_point check_points[] = {
            top_left, top_right, bottom_left, bottom_right,
            middle_top, quarter_top_left, quarter_top_right};

        // All collision barriers removed - players can move freely through sword zones
        // Sword zone collision checking has been disabled

        // Improved merchant collision system - check for collision zone (separate from interaction zone)
        // Only block if merchant zone is enabled and player is in collision zone
        if (is_in_merchant_collision_zone(position))
        {
            return false; // Block movement - player collides with merchant
        }

        // Then check for other tile-based collisions (if any other zone tiles exist)
        const int map_offset_x = (map_width * 4);
        const int map_offset_y = (map_height * 4);

        for (const auto &point : check_points)
        {
            // Convert position to cell coordinates
            // Assuming each cell is 8x8 pixels and the map is centered on the screen
            int cell_x = ((point.x() + map_offset_x) / 8).integer();
            int cell_y = ((point.y() + map_offset_y) / 8).integer();

            // Check if the cell position is within map bounds
            if (cell_x < 0 || cell_x >= map_width || cell_y < 0 || cell_y >= map_height)
            {
                return false;
            }

            // Get the tile at the specified position
            int cell_index = cell_y * map_width + cell_x;

            // Check if the index is in range
            if (cell_index < 0 || cell_index >= cells.size())
            {
                return false;
            }

            bn::regular_bg_map_cell cell = cells.at(cell_index);
            int tile_index = bn::regular_bg_map_cell_info(cell).tile_index();

            // Only check for zone tiles other than 3 and 4 (since merchant zones are handled separately by is_in_hitbox_zone)
            for (int zone_tile : _zone_tiles)
            {
                if (tile_index == zone_tile && zone_tile != 3 && zone_tile != 4)
                {
                    // It's a non-merchant zone tile, so we can't move here
                    return false;
                }
            }
        }

        // Position is valid if we made it this far
        return true;
    }
}
#include "fe_minimap.h"
#include "fe_constants.h"
#include "bn_sprite_items_minimap_player.h"
#include "bn_sprite_items_minimap_enemy.h"
#include "fe_enemy.h"
#include "bn_blending.h"

namespace fe
{
    Minimap::Minimap(bn::fixed_point pos, bn::regular_bg_map_ptr map, bn::camera_ptr &camera) : _player_dot(bn::sprite_items::minimap_player.create_sprite(pos.x(), pos.y() + MINIMAP_VERTICAL_OFFSET)), // Use player sprite
                                                                                                _position(bn::fixed_point(pos.x(), pos.y() + MINIMAP_VERTICAL_OFFSET))                                   // Update position to be 16 pixels lower
    {
        // Configure player dot
        _player_dot.set_bg_priority(0);
        _player_dot.set_z_order(Z_ORDER_MINIMAP_PLAYER);
        _player_dot.set_visible(true);

        (void)map;    // Mark as unused
        (void)camera; // Mark as unused
    }

    void Minimap::update(bn::fixed_point player_pos, bn::fixed_point map_center, const bn::vector<Enemy, 16> &enemies)
    {
        // Calculate player position relative to map center with a smaller scale for slower movement
        // Reduced by 5x to make the minimap appear larger (dots move slower)

        // Update player dot
        bn::fixed rel_x = (player_pos.x() - map_center.x()) * MINIMAP_POSITION_SCALE;
        bn::fixed rel_y = (player_pos.y() - map_center.y()) * MINIMAP_POSITION_SCALE;
        _player_dot.set_position(_position.x() + rel_x, _position.y() + rel_y);

        // Update enemy dots
        for (int i = 0; i < enemies.size(); ++i)
        {
            const Enemy &enemy = enemies[i];

            // Get enemy position
            bn::fixed_point enemy_pos = enemy.pos();

            // Find or create enemy dot - optimized sprite reuse
            if (i >= _enemy_dots.size())
            {
                // Create new enemy dot with enemy sprite
                auto sprite = bn::sprite_items::minimap_enemy.create_sprite(0, 0);
                sprite.set_bg_priority(0);
                sprite.set_z_order(Z_ORDER_MINIMAP_ENEMY);
                sprite.set_visible(true);
                sprite.set_blending_enabled(true);

                // Set transparency for enemy dots
                bn::blending::set_transparency_alpha(0.5);

                // Create and add new enemy dot
                _enemy_dots.push_back(EnemyDot(std::move(sprite), &enemy));
            }
            
            // Update existing enemy dot (moved outside else to reduce code duplication)
            _enemy_dots[i].enemy = &enemy;

            // Update position of enemy dot
            bn::fixed enemy_rel_x = (enemy_pos.x() - map_center.x()) * MINIMAP_POSITION_SCALE;
            bn::fixed enemy_rel_y = (enemy_pos.y() - map_center.y()) * MINIMAP_POSITION_SCALE;
            _enemy_dots[i].sprite.set_position(
                _position.x() + enemy_rel_x,
                _position.y() + enemy_rel_y);
        }

        // Remove any extra enemy dots
        while (_enemy_dots.size() > enemies.size())
        {
            _enemy_dots.pop_back();
        }
    }

    void Minimap::set_visible(bool visible)
    {
        _player_dot.set_visible(visible);
        for (EnemyDot &enemy_dot : _enemy_dots)
        {
            enemy_dot.sprite.set_visible(visible);
        }
    }
}
#include "fe_bullet_manager.h"
#include "fe_constants.h"
#include "fe_enemy.h"
#include "bn_sprite_items_hero_sword.h"
#include "bn_log.h"

namespace fe
{
    Bullet::Bullet(bn::fixed_point pos, bn::fixed_point velocity, bn::camera_ptr camera, Direction direction)
        : _pos(pos), _velocity(velocity), _active(true), _hitbox(pos.x(), pos.y(), 2, 2), _lifetime(BULLET_LIFETIME)
    {
        _sprite = bn::sprite_items::hero_sword.create_sprite(_pos.x(), _pos.y(), 0);
        _sprite->set_camera(camera);
        _sprite->set_z_order(Z_ORDER_BULLET); // Match existing bullet z-order
        _sprite->set_scale(BULLET_SCALE, BULLET_SCALE); // Make bullet small like in player implementation
        _sprite->set_bg_priority(0);
        
        switch (direction)
        {
            case Direction::UP:
                _sprite->set_rotation_angle(0);
                break;
            case Direction::RIGHT:
                _sprite->set_rotation_angle(270);
                break;
            case Direction::DOWN:
                _sprite->set_rotation_angle(180);
                break;
            case Direction::LEFT:
                _sprite->set_rotation_angle(90);
                break;
            default:
                _sprite->set_rotation_angle(270); // Default to right
                break;
        }
    }

    void Bullet::update()
    {
        if (!_active) return;

        // Update position
        _pos += _velocity;

        if (_sprite)
        {
            _sprite->set_position(_pos);
        }

        _hitbox.set_x(_pos.x());
        _hitbox.set_y(_pos.y());

        _lifetime--;
        if (_lifetime <= 0)
        {
            deactivate();
        }
    }

    bool Bullet::check_enemy_collision(Enemy &enemy)
    {
        if (!_active) return false;
        
        Hitbox enemy_hitbox = enemy.get_hitbox();
        return _hitbox.collides_with(enemy_hitbox);
    }

    BulletManager::BulletManager()
    {
    }

    void BulletManager::fire_bullet(bn::fixed_point pos, Direction direction)
    {
        if (_shoot_cooldown > 0 || !_camera) return;

        bn::fixed_point velocity = calculate_bullet_velocity(direction);

        _bullets.push_back(Bullet(pos, velocity, *_camera, direction));

        _shoot_cooldown = SHOOT_COOLDOWN_TIME;
    }

    void BulletManager::update_bullets()
    {
        if (_shoot_cooldown > 0)
        {
            _shoot_cooldown--;
        }

        for (int i = 0; i < _bullets.size();)
        {
            _bullets[i].update();
            
            if (!_bullets[i].is_active())
            {
                _bullets.erase(_bullets.begin() + i);
            }
            else
            {
                i++;
            }
        }
    }

    void BulletManager::clear_bullets()
    {
        _bullets.clear();
        _shoot_cooldown = 0;
    }

    void BulletManager::set_camera(bn::camera_ptr camera)
    {
        _camera = camera;
    }

    bn::fixed_point BulletManager::calculate_bullet_velocity(Direction direction) const
    {
        switch (direction)
        {
            case Direction::UP:
                return bn::fixed_point(0, -BULLET_SPEED);
            case Direction::DOWN:
                return bn::fixed_point(0, BULLET_SPEED);
            case Direction::LEFT:
                return bn::fixed_point(-BULLET_SPEED, 0);
            case Direction::RIGHT:
                return bn::fixed_point(BULLET_SPEED, 0);
            default:
                return bn::fixed_point(0, -BULLET_SPEED); // Default to up
        }
    }
}
#include "fe_entity.h"
#include "fe_constants.h"

namespace fe
{
    Entity::Entity() : _pos(0, 0), _previous_pos(0, 0), _hitbox(0, 0, DEFAULT_ENTITY_WIDTH, DEFAULT_ENTITY_HEIGHT)
    {
    }

    Entity::Entity(bn::fixed_point pos) : _pos(pos), _previous_pos(pos), _hitbox(pos.x(), pos.y(), DEFAULT_ENTITY_WIDTH, DEFAULT_ENTITY_HEIGHT)
    {
    }

    Entity::Entity(bn::sprite_ptr sprite) : _pos(sprite.x(), sprite.y()), _previous_pos(_pos), _sprite(sprite), _hitbox(_pos.x(), _pos.y(), DEFAULT_ENTITY_WIDTH, DEFAULT_ENTITY_HEIGHT)
    {
    }

    bn::fixed_point Entity::pos() const
    {
        return _pos;
    }

    bn::fixed_point Entity::previous_pos() const
    {
        return _previous_pos;
    }

    Hitbox Entity::get_hitbox() const
    {
        return _hitbox;
    }

    bool Entity::has_sprite() const
    {
        return _sprite.has_value();
    }

    void Entity::set_position(bn::fixed_point new_pos)
    {
        _previous_pos = _pos;
        _pos = new_pos;
        update_hitbox();
        update_sprite_position();
    }

    void Entity::revert_position()
    {
        _pos = _previous_pos;
        update_hitbox();
        update_sprite_position();
    }

    void Entity::set_sprite_z_order(int z_order)
    {
        if (_sprite)
        {
            _sprite->set_z_order(z_order);
        }
    }

    void Entity::set_visible(bool visible)
    {
        if (_sprite)
        {
            _sprite->set_visible(visible);
        }
    }

    void Entity::set_camera(bn::camera_ptr camera)
    {
        if (_sprite)
        {
            _sprite->set_camera(camera);
        }
    }

    void Entity::update_hitbox()
    {
        _hitbox.set_x(_pos.x());
        _hitbox.set_y(_pos.y());
    }

    void Entity::update_sprite_position()
    {
        if (_sprite)
        {
            _sprite->set_position(_pos);
        }
    }
}
#include "fe_movement.h"
#include "bn_math.h"

namespace fe
{
    Movement::Movement() : _dx(0), _dy(0), _current_state(State::IDLE), _facing_direction(Direction::DOWN)
    {
    }

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
        {
            _dx = 0;
        }
        if (bn::abs(_dy) < get_movement_threshold())
        {
            _dy = 0;
        }

        update_state();
    }

    void Movement::reset()
    {
        _dx = 0;
        _dy = 0;
        _current_state = State::IDLE;
        _facing_direction = Direction::DOWN;
    }

    void Movement::stop_movement()
    {
        _dx = 0;
        _dy = 0;
        update_state();
    }

    void Movement::update_state()
    {
        if (_dx == 0 && _dy == 0)
        {
            _current_state = State::IDLE;
        }
        else
        {
            _current_state = State::WALKING;
        }
    }

    void Movement::clamp_velocity()
    {
        bn::fixed max_speed = get_max_speed();
        
        if (_dx > max_speed)
        {
            _dx = max_speed;
        }
        else if (_dx < -max_speed)
        {
            _dx = -max_speed;
        }

        if (_dy > max_speed)
        {
            _dy = max_speed;
        }
        else if (_dy < -max_speed)
        {
            _dy = -max_speed;
        }
    }


    EnemyMovement::EnemyMovement() : Movement()
    {
    }
}
#include "fe_direction_utils.h"

namespace fe
{
    namespace direction_utils
    {
        bn::fixed_point get_roll_offset(Direction dir, int frames_remaining, int total_frames)
        {
            // Use linear momentum decay for smoother animation sync
            bn::fixed momentum_factor = bn::fixed(frames_remaining) / bn::fixed(total_frames);
            // Start fast, end slower but not too dramatic
            momentum_factor = (momentum_factor * 0.7) + 0.3; // Range from 1.0 to 0.3
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
            // Z-order offsets for gun sprite relative to player
            // Lower z-order = drawn on top (in front)
            // UP: gun behind player (higher z), DOWN: gun in front (lower z)
            switch (dir)
            {
            case Direction::UP:
                return -1;  // gun behind player (drawn first)
            case Direction::DOWN:
                return 1;   // gun in front of player (drawn on top)
            case Direction::LEFT:
            case Direction::RIGHT:
            default:
                return -1;
            }
        }
    }
}
