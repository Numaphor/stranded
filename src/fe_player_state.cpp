#include "fe_player.h"

namespace fe
{
    // PlayerState Implementation
    void PlayerState::set_listening(bool listening)
    {
        if (_listening && !listening)
        {
            // Dialog just ended, set cooldown
            _dialog_cooldown = 10; // 10 frame cooldown
        }
        _listening = listening;
    }

    void PlayerState::update_dialog_cooldown()
    {
        if (_dialog_cooldown > 0)
            _dialog_cooldown--;
    }

    void PlayerState::reset()
    {
        _invulnerable = false;
        _listening = false;
        _inv_timer = 0;
        _dialog_cooldown = 0;
    }
}
