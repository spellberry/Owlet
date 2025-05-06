#include <string>

#include "camera/camera_test.h"
#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "samples/Empty/empty.hpp"
#include "samples/flat/flat.hpp"
#include "samples/fleet/fleet.hpp"
#include "samples/forest/forest_2d.hpp"
#include "samples/model_viewer.hpp"
#include "samples/platformer/platformer.hpp"
#include "samples/rts/rts.hpp"
#include "samples/terrain-test/terrain_test.hpp"

using namespace bee;

int main(int argc, char* argv[])
{
    if (argc != 2) return 1;
    std::string gameName = argv[1];

    Engine.Initialize();

    if (gameName == "rts")
        Engine.ECS().CreateSystem<RTS>();
    else if (gameName == "platformer")
        Engine.ECS().CreateSystem<platformer::Platformer>();
    else if (gameName == "model-viewer")
        Engine.ECS().CreateSystem<ModelViewer>();
    else if (gameName == "fleet")
        Engine.ECS().CreateSystem<flt::Fleet>();
    else if (gameName == "flat")
        Engine.ECS().CreateSystem<flt::Flat>();
    else if (gameName == "forest-2d")
        Engine.ECS().CreateSystem<f2d::Forest2D>();
    else if (gameName == "camera-test")
        Engine.ECS().CreateSystem<CameraTest>();
    else if (gameName == "terrain-test")
        Engine.ECS().CreateSystem<tt::TerrainTest>();
    else if (gameName == "empty")
        Engine.ECS().CreateSystem<Empty>();

    Engine.Run();
    Engine.Shutdown();
}
