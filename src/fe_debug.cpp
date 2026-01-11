#include "fe_debug.h"
#include "bn_sprite_items_bullet.h"
#include "bn_log.h"

namespace fe
{
    DebugSystem::DebugSystem() : _debug_active(false)
    {
    }
    
    void DebugSystem::toggle_debug_mode()
    {
        _debug_active = !_debug_active;
        BN_LOG("Debug mode: ", _debug_active ? "ON" : "OFF");
        
        if (!_debug_active)
        {
            clear_debug_markers();
        }
    }
    
    void DebugSystem::clear_debug_markers()
    {
        _debug_markers.clear();
    }
    
    void DebugSystem::add_hitbox_marker(const Hitbox& hitbox, bn::camera_ptr camera)
    {
        if (!_debug_active || _debug_markers.full())
        {
            return;
        }
        
        // Use bullet sprite as a simple debug marker (small red square)
        bn::fixed_point center = hitbox.center();
        bn::sprite_ptr marker = bn::sprite_items::bullet.create_sprite(center.x(), center.y());
        marker.set_camera(camera);
        marker.set_z_order(-32000); // High priority to show above everything
        
        _debug_markers.push_back(bn::move(marker));
    }
    
    void DebugSystem::update()
    {
        // Debug markers are automatically managed by Butano sprite system
        // This method is here for future expansion
    }
}
