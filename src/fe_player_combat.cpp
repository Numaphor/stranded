#include "fe_player.h"
#include "fe_constants.h"
#include "fe_direction_utils.h"
#include "bn_sprite_items_companion.h"
#include "bn_keypad.h"

namespace fe
{
    // Gun setup function implementation
    namespace direction_utils
    {
        void setup_gun(bn::sprite_ptr &gun_sprite, Direction dir, bn::fixed_point pos)
        {
            const int idx = int(dir);
            gun_sprite.set_horizontal_flip(player_constants::GUN_FLIPS[idx]);
            gun_sprite.set_rotation_angle(player_constants::GUN_ANGLES[idx]);
            gun_sprite.set_position(
                pos.x() + player_constants::GUN_OFFSET_X[idx],
                pos.y() + player_constants::GUN_OFFSET_Y[idx]);
        }
    }

    // Player combat methods
    void Player::update_gun_position(PlayerMovement::Direction direction)
    {
        if (!_gun_sprite)
            return;

        Direction dir = static_cast<Direction>(int(direction));
        direction_utils::setup_gun(*_gun_sprite, dir, pos());
    }

    void Player::fire_bullet(PlayerMovement::Direction direction)
    {
        if (!_gun_active || !_gun_sprite.has_value() || !has_ammo())
            return;

        // Only fire and consume ammo if the bullet manager can actually fire
        if (!_bullet_manager.can_fire())
            return;

        Direction bullet_dir = static_cast<Direction>(int(direction));
        bn::fixed_point bullet_pos = direction_utils::get_bullet_position(bullet_dir, pos());
        _bullet_manager.fire_bullet(bullet_pos, bullet_dir);

        // Consume ammo and update HUD only after successful bullet firing
        _ammo_count--;
        _hud.set_ammo(_ammo_count);

        // Set flag for screen shake
        _bullet_just_fired = true;
    }

    void Player::update_bullets()
    {
        _bullet_manager.update_bullets();
    }

    bool Player::is_firing() const
    {
        // Player is firing if A button is held, gun is active, and dialog cooldown is 0
        return bn::keypad::a_held() && _gun_active && _state.dialog_cooldown() == 0;
    }

    void Player::initialize_companion(bn::camera_ptr camera)
    {
        if (_companion_initialized)
        {
            // If companion is already initialized but died independently,
            // just update its camera without respawning
            if (_companion.has_value() && _companion->is_dead_independently())
            {
                _companion->set_camera(camera);
            }
            return;
        }

        bn::sprite_ptr companion_sprite = bn::sprite_items::companion.create_sprite(pos());

        // Set companion bg_priority to 0 to ensure it NEVER goes behind the sword background
        // The sword background uses priorities 0-2, so companion at priority 0
        // will always be visible (sprites cover backgrounds of same priority)
        companion_sprite.set_bg_priority(0);

        _companion = PlayerCompanion(bn::move(companion_sprite));
        _companion->spawn(pos(), camera);
        _companion->set_flying(true);
        _companion_initialized = true;
    }
}
