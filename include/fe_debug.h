#ifndef FE_DEBUG_H
#define FE_DEBUG_H

#include "bn_sprite_ptr.h"
#include "bn_vector.h"
#include "bn_optional.h"
#include "bn_camera_ptr.h"
#include "bn_fixed_point.h"
#include "fe_hitbox.h"

namespace fe
{
    class DebugSystem
    {
    public:
        DebugSystem();
        
        void toggle_debug_mode();
        [[nodiscard]] bool is_debug_active() const { return _debug_active; }
        
        void clear_debug_markers();
        void add_hitbox_marker(const Hitbox& hitbox, bn::camera_ptr camera);
        void update();
        
    private:
        bool _debug_active;
        bn::vector<bn::sprite_ptr, 32> _debug_markers;
    };
}

#endif
