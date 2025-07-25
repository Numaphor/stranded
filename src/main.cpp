#include "bn_core.h"
#include "bn_fixed_point.h"

#include "fe_scene.h"
#include "fe_scene_world.h"

int main()
{
    bn::core::init();

    fe::Scene next = fe::Scene::WORLD;
    bn::fixed_point spawn_location(50, 100); // Changed from (0, 0) to (0, 100) to lower spawn position

    while(true)
    {
        switch(next)
        {
            case fe::Scene::WORLD:
            {
                fe::World world;
                next = world.execute(spawn_location);
                break;
            }
            default:
                next = fe::Scene::WORLD;
                break;
        }
        bn::core::update();
    }
}
