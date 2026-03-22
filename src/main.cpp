#include "bn_core.h"
#include "str_scene_room_viewer.h"

int main()
{
    bn::core::init();
    str::RoomViewer* viewer = new str::RoomViewer();

    while(true)
    {
        (void) viewer->execute();
        bn::core::update();
    }
}
