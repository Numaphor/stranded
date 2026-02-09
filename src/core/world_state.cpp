#include "str_world_state.h"
#include "str_constants.h"

#include "bn_fixed_point.h"
#include "bn_vector.h"

namespace str
{

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

} // namespace str
