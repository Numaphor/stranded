/**
 * MANAGER_PATTERN: New System Manager Implementation Template
 * - Follows the structure defined in new_manager_template.h
 * - Implements singleton pattern for centralized management
 * - Handles entity lifecycle and system coordination
 */

#include "new_manager_template.h"
#include "str_entity.h"
#include "bn_assert.h"

namespace str
{
    NewManager& NewManager::instance()
    {
        static NewManager instance;
        return instance;
    }
    
    NewManager::NewManager()
        : _is_active(true)
        , _update_timer(0)
    {
        // Initialize entity storage
        _entities.reserve(MAX_ENTITIES);
    }
    
    void NewManager::update()
    {
        if (!_is_active) return;
        
        // Update system timer
        _update_timer++;
        
        // Update all managed entities
        _update_entities();
        
        // Clean up inactive entities
        _cleanup_inactive_entities();
        
        // Process system-specific logic
        _process_system_logic();
        
        // Handle interactions between entities
        _handle_entity_interactions();
        
        // Validate entity count for debugging
        _validate_entity_count();
    }
    
    void NewManager::add_entity(Entity* entity)
    {
        BN_ASSERT(entity, "Cannot add null entity to manager");
        BN_ASSERT(_entities.size() < MAX_ENTITIES, "Too many entities in manager");
        
        // Check if entity already exists
        for (Entity* existing : _entities)
        {
            if (existing == entity)
            {
                return; // Entity already managed
            }
        }
        
        _entities.push_back(entity);
    }
    
    void NewManager::remove_entity(Entity* entity)
    {
        BN_ASSERT(entity, "Cannot remove null entity from manager");
        
        // Find and remove entity
        for (auto it = _entities.begin(); it != _entities.end(); ++it)
        {
            if (*it == entity)
            {
                _entities.erase(it);
                break;
            }
        }
    }
    
    void NewManager::clear_entities()
    {
        _entities.clear();
        _cleanup_resources();
    }
    
    int NewManager::get_entity_count() const
    {
        return _entities.size();
    }
    
    bool NewManager::has_entities() const
    {
        return !_entities.empty();
    }
    
    // Private helper methods
    void NewManager::_update_entities()
    {
        for (Entity* entity : _entities)
        {
            if (entity && entity->is_active())
            {
                entity->update();
            }
        }
    }
    
    void NewManager::_cleanup_inactive_entities()
    {
        // Remove inactive entities
        auto it = _entities.begin();
        while (it != _entities.end())
        {
            Entity* entity = *it;
            if (!entity || !entity->is_active())
            {
                it = _entities.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    
    void NewManager::_process_system_logic()
    {
        // System-specific logic goes here
        // Examples:
        // - Spawn new entities based on conditions
        // - Update system-wide parameters
        // - Handle level progression
        // - Manage difficulty scaling
        
        // Example: Spawn new entity every 300 frames (5 seconds at 60 FPS)
        if (_update_timer % 300 == 0 && _entities.size() < MAX_ENTITIES / 2)
        {
            // Entity spawning logic would go here
            // Entity* new_entity = new Entity(spawn_position);
            // add_entity(new_entity);
        }
    }
    
    void NewManager::_handle_entity_interactions()
    {
        // Handle interactions between managed entities
        // Examples:
        // - Collision detection between entities
        // - Entity grouping or formation
        // - System-wide effects (explosions, area damage)
        
        for (size_t i = 0; i < _entities.size(); ++i)
        {
            Entity* entity_a = _entities[i];
            if (!entity_a || !entity_a->is_active()) continue;
            
            for (size_t j = i + 1; j < _entities.size(); ++j)
            {
                Entity* entity_b = _entities[j];
                if (!entity_b || !entity_b->is_active()) continue;
                
                // Check collision between entities
                if (entity_a->get_hitbox().intersects(entity_b->get_hitbox()))
                {
                    entity_a->handle_collision(entity_b);
                    entity_b->handle_collision(entity_a);
                }
            }
        }
    }
    
    void NewManager::_cleanup_resources()
    {
        // Clean up system resources
        // Examples:
        // - Reset timers
        // - Clear temporary data
        // - Release pooled objects
        
        _update_timer = 0;
        _is_active = true;
    }
    
    void NewManager::_validate_entity_count()
    {
        // Debug validation to ensure entity count is reasonable
        BN_ASSERT(_entities.size() <= MAX_ENTITIES, "Entity count exceeded maximum");
        
        #ifdef DEBUG
        // Additional debug checks in debug builds
        int active_count = 0;
        for (Entity* entity : _entities)
        {
            if (entity && entity->is_active())
            {
                active_count++;
            }
        }
        
        // Log warning if too many inactive entities
        if (active_count < _entities.size() / 2)
        {
            // Could add debug logging here
            // bn::log::warning("Many inactive entities in manager");
        }
        #endif
    }
}
