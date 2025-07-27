#include "fe_world_state.h"

namespace fe
{
    void WorldStateManager::save_world_state(int world_id, const bn::fixed_point& player_pos, int player_health)
    {
        WorldState* existing_state = _find_state(world_id);
        
        if(existing_state)
        {
            // Update existing state
            existing_state->player_position = player_pos;
            existing_state->player_health = player_health;
            existing_state->is_saved = true;
        }
        else
        {
            // Create new state
            WorldState new_state(world_id);
            new_state.player_position = player_pos;
            new_state.player_health = player_health;
            new_state.is_saved = true;
            _saved_states.push_back(new_state);
        }
    }
    
    WorldState WorldStateManager::load_world_state(int world_id)
    {
        WorldState* existing_state = _find_state(world_id);
        
        if(existing_state && existing_state->is_saved)
        {
            return *existing_state;
        }
        else
        {
            // Return default state for this world
            WorldState default_state(world_id);
            default_state.player_position = get_default_spawn(world_id);
            return default_state;
        }
    }
    
    bool WorldStateManager::has_saved_state(int world_id)
    {
        WorldState* existing_state = _find_state(world_id);
        return existing_state && existing_state->is_saved;
    }
    
    bn::fixed_point WorldStateManager::get_default_spawn(int world_id)
    {
        switch(world_id)
        {
            case 0: return bn::fixed_point(50, 100);    // Main World
            case 1: return bn::fixed_point(100, 50);    // Forest Area  
            case 2: return bn::fixed_point(0, 150);     // Desert Zone
            case 3: return bn::fixed_point(-50, 75);    // Ocean Side
            default: return bn::fixed_point(50, 100);   // Default
        }
    }
    
    WorldState* WorldStateManager::_find_state(int world_id)
    {
        for(int i = 0; i < _saved_states.size(); ++i)
        {
            if(_saved_states[i].world_id == world_id)
            {
                return &_saved_states[i];
            }
        }
        return nullptr;
    }
}