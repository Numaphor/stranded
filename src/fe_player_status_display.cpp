#include "fe_player_status_display.h"
#include "fe_constants.h"
#include "fe_player.h"
#include "bn_string.h"

namespace fe
{
    PlayerStatusDisplay::PlayerStatusDisplay(bn::sprite_text_generator &text_generator)
        : _text_generator(text_generator), _status_valid(false)
    {
        // Do not call _update_display() here; wait for first update_status call.
    }

    void PlayerStatusDisplay::update_status(const Player &player, bool near_merchant)
    {
        PLAYER_STATUS new_status = _determine_status(player, near_merchant);

        if (new_status != _current_status || !_status_valid)
        {
            _current_status = new_status;
            _status_valid = true;
            _update_display();
        }
    }

    void PlayerStatusDisplay::set_visible(bool is_visible)
    {
        _is_visible = is_visible;

        // If becoming visible, ensure status is valid before updating display
        if (is_visible)
        {
            if (!_status_valid)
            {
                _current_status = PLAYER_STATUS::IDLE;
                _status_valid = true;
            }
            _update_display();
        }
        else
        {
            // If becoming invisible, hide existing sprites
            for (bn::sprite_ptr &sprite : _text_sprites)
            {
                sprite.set_visible(false);
            }
        }
    }

    void PlayerStatusDisplay::_update_display()
    {
        if (!_is_visible)
            return;

        _text_sprites.clear();

        const char *status_text = _status_to_string(_current_status);

        // Use a fixed left-aligned position to avoid shifting
        _text_generator.set_left_alignment();
        _text_generator.generate(PLAYER_STATUS_X, PLAYER_STATUS_Y, status_text, _text_sprites);

        // Set high priority and ensure sprites stay fixed on screen
        for (bn::sprite_ptr &sprite : _text_sprites)
        {
            sprite.set_bg_priority(0);
            sprite.set_z_order(Z_ORDER_PLAYER_STATUS_HIGH_PRIORITY); // Very high priority
            sprite.remove_camera();    // Remove camera = fixed screen position
        }
    }

    PLAYER_STATUS PlayerStatusDisplay::_determine_status(const Player &player, bool near_merchant)
    {
        // Check for critical states first
        if (player.get_hp() <= 0)
        {
            return PLAYER_STATUS::DEAD;
        }

        if (player.listening())
        {
            return PLAYER_STATUS::LISTENING;
        }

        if (near_merchant)
        {
            return PLAYER_STATUS::NEAR_MERCHANT;
        }

        // Check movement state using is_state method
        if (player.is_state(PlayerMovement::State::CHOPPING) ||
            player.is_state(PlayerMovement::State::SLASHING) ||
            player.is_state(PlayerMovement::State::ATTACKING))
        {
            return PLAYER_STATUS::COMBAT;
        }

        if (player.is_state(PlayerMovement::State::HIT))
        {
            return PLAYER_STATUS::HIT;
        }

        if (player.is_state(PlayerMovement::State::WALKING) ||
            player.is_state(PlayerMovement::State::RUNNING) ||
            player.is_state(PlayerMovement::State::ROLLING))
        {
            return PLAYER_STATUS::MOVING;
        }

        // Default to idle for other states
        return PLAYER_STATUS::IDLE;
    }

    const char *PlayerStatusDisplay::_status_to_string(PLAYER_STATUS status)
    {
        switch (status)
        {
        case PLAYER_STATUS::IDLE:
            return "Idle";
        case PLAYER_STATUS::MOVING:
            return "Moving";
        case PLAYER_STATUS::COMBAT:
            return "Combat";
        case PLAYER_STATUS::LISTENING:
            return "Talking";
        case PLAYER_STATUS::NEAR_MERCHANT:
            return "Merchant";
        case PLAYER_STATUS::HIT:
            return "Hit";
        case PLAYER_STATUS::DEAD:
            return "Dead";
        default:
            return "Unknown";
        }
    }
}
