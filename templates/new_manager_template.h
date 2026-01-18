/**
 * MANAGER_PATTERN: New System Manager Template
 * - MUST provide update() method for frame loop
 * - MUST handle resource cleanup automatically
 * - NO direct game logic (coordinate between entities)
 * - MUST use singleton or centralized instance
 * - MANAGES: [Specific system responsibilities]
 */

#ifndef STR_NEW_MANAGER_H
#define STR_NEW_MANAGER_H

#include "bn_vector.h"
#include "bn_optional.h"
#include "bn_fixed_point.h"
#include "bn_sprite_ptr.h"

#include "str_constants.h"

namespace str
{
    // Forward declarations
    class Entity;
    
    class NewManager
    {
    public:
        /**
         * Get singleton instance
         * @return Reference to the manager instance
         */
        static NewManager& instance();
        
        /**
         * Main update method called each frame
         * Updates all managed entities and handles system logic
         */
        void update();
        
        /**
         * Add entity to manager for tracking
         * @param entity Pointer to entity to manage
         */
        void add_entity(Entity* entity);
        
        /**
         * Remove entity from manager
         * @param entity Pointer to entity to remove
         */
        void remove_entity(Entity* entity);
        
        /**
         * Clear all entities from manager
         * Called during level reset or game over
         */
        void clear_entities();
        
        /**
         * Get number of active entities
         * @return Count of managed entities
         */
        int get_entity_count() const;
        
        /**
         * Check if manager has any active entities
         * @return true if entities exist, false otherwise
         */
        bool has_entities() const;

    private:
        // Singleton pattern - private constructor
        NewManager();
        ~NewManager() = default;
        
        // Prevent copying
        NewManager(const NewManager&) = delete;
        NewManager& operator=(const NewManager&) = delete;
        
        // Entity storage
        bn::vector<Entity*, MAX_ENTITIES> _entities;
        
        // System state
        bool _is_active;
        int _update_timer;
        
        // Private helper methods
        void _update_entities();
        void _cleanup_inactive_entities();
        void _process_system_logic();
        void _handle_entity_interactions();
        
        // Resource management
        void _cleanup_resources();
        void _validate_entity_count();
    };
}

#endif // STR_NEW_MANAGER_H
