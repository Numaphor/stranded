#include "fe_player.h"

namespace fe
{
    // PlayerAbilities Implementation
    void PlayerAbilities::update_cooldowns()
    {
        if (_roll_cooldown > 0)
            _roll_cooldown--;
        if (_chop_cooldown > 0)
            _chop_cooldown--;
        if (_slash_cooldown > 0)
            _slash_cooldown--;
        if (_buff_cooldown > 0)
            _buff_cooldown--;
    }

    void PlayerAbilities::reset()
    {
        _running_available = true;
        _rolling_available = true;
        _chopping_available = true;
        _slashing_available = true;
        _buff_abilities_available = true;
        _roll_cooldown = 0;
        _chop_cooldown = 0;
        _slash_cooldown = 0;
        _buff_cooldown = 0;
    }
}
