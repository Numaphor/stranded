#include "bn_core.h"
#include "bn_fixed_point.h"

#include "str_scene.h"
#include "str_scene_start.h"
#include "str_scene_menu.h"
#include "str_scene_controls.h"
#include "str_scene_world.h"

int main()
{
    bn::core::init();

    str::Scene next = str::Scene::START;  // Start with the start screen
    bn::fixed_point spawn_location(50, 100);
    int selected_world_id = 0;

    while(true)
    {
        switch(next)
        {
            case str::Scene::START:
            {
                str::Start start;
                next = start.execute();
                break;
            }
            case str::Scene::CONTROLS:
            {
                str::Controls controls;
                next = controls.execute();
                break;
            }
            case str::Scene::MENU:
            {
                str::Menu menu;
                next = menu.execute(selected_world_id, spawn_location);
                break;
            }
            case str::Scene::WORLD:
            {
                str::World world;
                next = world.execute(spawn_location, selected_world_id);
                break;
            }
            default:
                next = str::Scene::START;
                break;
        }
        bn::core::update();
    }
}
