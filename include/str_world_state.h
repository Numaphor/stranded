#ifndef STR_WORLD_STATE_H
#define STR_WORLD_STATE_H

#include "bn_fixed_point.h"
#include "bn_vector.h"

namespace str
{
    // Structure to hold state for a specific world
    struct WorldState
    {
        int world_id;
        bn::fixed_point player_position;
        int player_health;
        // Could add more state like enemy positions, items collected, etc.
        bool is_saved;
        
        WorldState() : world_id(0), player_position(50, 100), player_health(100), is_saved(false) {}
        WorldState(int id) : world_id(id), player_position(50, 100), player_health(100), is_saved(false) {}
    };
    
    class WorldStateManager
    {
    public:
        static WorldStateManager& instance()
        {
            static WorldStateManager _instance;
            return _instance;
        }
        
        // Save current world state
        void save_world_state(int world_id, const bn::fixed_point& player_pos, int player_health);
        
        // Load world state
        WorldState load_world_state(int world_id);
        
        // Check if world has saved state
        bool has_saved_state(int world_id);
        
        // Get default spawn location for world
        bn::fixed_point get_default_spawn(int world_id);
        
    private:
        WorldStateManager() {}
        bn::vector<WorldState, 8> _saved_states;
        
        WorldState* _find_state(int world_id);
    };
}

#endif