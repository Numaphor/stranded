#ifndef STR_SOLDIER_H
#define STR_SOLDIER_H

#include "bn_sprite_ptr.h"
#include "bn_fixed_point.h"

#include "str_player.h"
#include "str_constants.h"

namespace str
{

/**
 * Soldier class - Ranged focused character with unique abilities
 * Inherits from Player but adds soldier-specific behaviors
 * - Heavy firepower with assault rifle
 * - Tactical buffs for accuracy and damage
 * - Different movement characteristics
 */
class Soldier : public Player
{
public:
    explicit Soldier(bn::sprite_ptr sprite);
    
    // Override update for soldier-specific behavior
    void update() override;
    
    // Soldier-specific public methods
    [[nodiscard]] bool has_accuracy_boost() const { return _accuracy_boost_timer > 0; }
    [[nodiscard]] bool has_damage_boost() const { return _damage_boost_timer > 0; }
    [[nodiscard]] int burst_fire_count() const { return _burst_fire_count; }
    
private:
    // Soldier-specific properties
    int _burst_fire_count = 0;
    int _accuracy_boost_timer = 0;
    int _damage_boost_timer = 0;
    
    // Soldier-specific ability methods
    void _update_soldier_abilities();
    void _handle_burst_fire();
    void _apply_tactical_buffs();
};

}

#endif