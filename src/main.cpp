#include "bn_core.h"
#include "bn_fixed_point.h"

#include "fe_scene.h"
#include "fe_scene_world.h"
#include "fe_scene_menu.h"

int main()
{
    bn::core::init();

    fe::Scene next = fe::Scene::MENU;  // Start with menu instead of world
    bn::fixed_point spawn_location(50, 100);
    int selected_world_id = 0;

    while(true)
    {
        switch(next)
        {
            case fe::Scene::MENU:
            {
                fe::Menu menu;
                next = menu.execute(selected_world_id, spawn_location);
                break;
            }
            case fe::Scene::WORLD:
            {
                fe::World world;
                next = world.execute(spawn_location, selected_world_id);
                break;
            }
            default:
                next = fe::Scene::MENU;
                break;
        }
        bn::core::update();
    }
}
