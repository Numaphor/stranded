#include "bn_core.h"
#include "str_scene_room_viewer.h"
int main()
{
    bn::core::init();
    while(true)
    {
        str::run_room_viewer();
    }
}
