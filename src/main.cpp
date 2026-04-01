#include "bn_core.h"
#include "str_scene_room_viewer.h"

int main()
{
    bn::core::init();

    while(true)
    {
        str::RoomViewer viewer;
        (void) viewer.execute();
        bn::core::update();
    }
}
