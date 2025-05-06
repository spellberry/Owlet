#include "core/engine.hpp"
#include "game_ui/main_menu.hpp"
#include "platform/pc/core/device_pc.hpp"
////

#include <platform/dx12/animation_system.hpp>

#include "actors/actor_wrapper.hpp"
#include "camera/camera_rts_system.hpp"
#include "leaderboard/leaderboard.hpp"
#include "level_editor/terrain_system.hpp"
#include "material_system/material_system.hpp"
#include "tools/asset_explorer_system.hpp"
#include "tools/debug_metric.hpp"
#include "user_interface/user_interface.hpp"
using namespace bee;

void InitializeSystems()
{
    bee::Engine.ECS().CreateSystem<bee::AssetExplorer>();
    bee::Engine.ECS().CreateSystem<bee::MaterialSystem>();
    bee::Engine.ECS().CreateSystem<bee::CameraSystemRTS>();
    auto& renderer = bee::Engine.ECS().CreateSystem<bee::RenderPipeline>();
    renderer.SetIsPausable(false);
    bee::Engine.ECS().CreateSystem<bee::AnimationSystem>();
    bee::Engine.ECS().CreateSystem<bee::ui::UserInterface>().SetIsPausable(false);
    // create scene
    auto& terrainSystem = bee::Engine.ECS().CreateSystem<lvle::TerrainSystem>();
    terrainSystem.UpdateTerrainDataComponent();
    bee::actors::CreateActorSystems();
    auto leaderboard = bee::Engine.ECS().CreateEntity();
    auto& leaderboardComponent = bee::Engine.ECS().CreateComponent<Leaderboard>(leaderboard);
    leaderboardComponent.LoadScores();
    auto debugMetricEntity = bee::Engine.ECS().CreateEntity();
    bee::Engine.ECS().CreateComponent<DebugMetricData>(debugMetricEntity);
}

#ifdef NO_CONSOLE
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main()
#endif
{
    Engine.Initialize();
    InitializeSystems();
    Engine.SetGame<MainMenu>();
    Engine.Device().SetFullScreen(true);
    Engine.Run();
    Engine.Shutdown();
}
