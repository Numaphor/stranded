#include "str_soldier.h"
#include "bn_sprite_items_soldier.h"
#include "bn_sprite_items_soldier_vfx.h"
#include "bn_keypad.h"
#include "bn_random.h"

namespace str
{

Soldier::Soldier(bn::sprite_ptr sprite) 
    : Player(bn::move(sprite),
             bn::sprite_items::soldier.tiles_item(),
             PlayerAnimation::Profile::SOLDIER)
{
    // Soldier-specific initialization
    // Base class handles most initialization
}

void Soldier::update()
{
    // Update base player functionality
    Player::update();
    
    // Update soldier-specific abilities
    _update_soldier_abilities();
    _handle_burst_fire();
    _apply_tactical_buffs();
}

void Soldier::_update_soldier_abilities()
{
    // Update soldier-specific ability timers
    if (_accuracy_boost_timer > 0) {
        _accuracy_boost_timer--;
    }
    
    if (_damage_boost_timer > 0) {
        _damage_boost_timer--;
    }
}

void Soldier::_handle_burst_fire()
{
    // Handle burst fire mechanics for automatic fire capability
    if (is_state(PlayerMovement::State::ATTACKING)) {
        if (bn::keypad::a_held() && _burst_fire_count < 3) {
            // Continue burst fire
            _burst_fire_count++;
        }
    } else if (_burst_fire_count > 0) {
        // Reset burst count when not attacking
        _burst_fire_count = 0;
    }
}

void Soldier::_apply_tactical_buffs()
{
    // Apply tactical buffs based on button combinations
    if (bn::keypad::l_pressed() && bn::keypad::r_pressed()) {
        // Activate tactical accuracy boost
        if (_accuracy_boost_timer == 0) {
            _accuracy_boost_timer = 300; // 5 seconds at 60fps
            // VFX could be triggered here
        }
    }
    
    // Apply damage boost when low health (soldier's desperation ability)
    if (get_hp() <= 1 && _damage_boost_timer == 0) {
        _damage_boost_timer = 600; // 10 seconds
        // VFX for damage boost
    }
}

} // namespace str