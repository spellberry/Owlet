#include "TowerDefense.hpp"

#include <platform/dx12/animation_system.hpp>

#include "actors/actor_wrapper.hpp"
#include "actors/projectile_system/projectile_system.hpp"
#include "actors/props/resource_system.hpp"
#include "actors/selection_system.hpp"
#include "actors/units/unit_manager_system.hpp"
#include "ai/ai_behavior_selection_system.hpp"
#include "ai/behavior_editor_system.hpp"
#include "ai/grid_navigation_system.hpp"
#include "ai/navigation_grid.hpp"
#include "ai_behaviors/unit_behaviors.hpp"
#include "camera/camera_rts_system.hpp"
#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "level_editor/terrain_system.hpp"
#include "order/order_system.hpp"
#include "physics/world.hpp"
#include "platform/opengl/render_gl.hpp"
#include "quest_system/quest_system.hpp"
#include "user_interface/font_handler.hpp"
#include "user_interface/user_interface.hpp"

void TowerDefense::Init()
{
    GameBase::Init();
    if (!bee::Engine.WasPreInitialized())
    {
        bee::Engine.ECS().CreateSystem<bee::CameraSystemRTS>();
        bee::Engine.ECS().CreateSystem<bee::RenderPipeline>();
        bee::Engine.ECS().CreateSystem<bee::AnimationSystem>();
        auto& UI = bee::Engine.ECS().CreateSystem<bee::ui::UserInterface>();
        UI.LoadFont(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Asset, "fonts/Droidsans.sff"));

        bee::Engine.ECS().CreateSystem<lvle::TerrainSystem>();
    }

    bee::Engine.ECS().CreateSystem<SelectionSystem>();
    bee::Engine.ECS().CreateSystem<OrderSystem>();
    bee::Engine.ECS().CreateSystem<bee::physics::World>(0.02f);
    bee::Engine.ECS().CreateSystem<bee::ai::BehaviorEditorSystem>();
    bee::Engine.ECS().CreateSystem<ProjectileSystem>();
    bee::Engine.ECS().CreateSystem<ResourceSystem>();

    bee::actors::CreateActorSystems();

    bee::Engine.ECS().CreateSystem<bee::ai::AIBehaviorSelectionSystem>(1.0f / 6.0f);

    {  // Terrain
        auto& terrain_system = bee::Engine.ECS().GetSystem<lvle::TerrainSystem>();
        terrain_system.LoadLevel(m_levelName);
    }

    {  // Grid Navigation System
        // This loop should only execute ONCE if everything is done correctly.
        auto view = bee::Engine.ECS().Registry.view<lvle::TerrainDataComponent>();
        for (auto entity : view)
        {
            auto [data] = view.get(entity);
            bee::ai::NavigationGrid nav_grid(data.m_tiles[0].centralPos, data.m_step, data.m_width, data.m_height);
            auto& grid_nav_system = bee::Engine.ECS().CreateSystem<bee::ai::GridNavigationSystem>(0.1f, nav_grid);
            grid_nav_system.UpdateFromTerrain();
        }
    }

    {
        // Create key directional light
        auto lightEntity = bee::Engine.ECS().CreateEntity();
        auto& light = bee::Engine.ECS().CreateComponent<bee::Light>(lightEntity, glm::vec3(1.f), 100.f, 100.f,
                                                                    bee::Light::Type::Directional);
        light.ShadowExtent = 60.f;
        auto& transform = bee::Engine.ECS().CreateComponent<bee::Transform>(lightEntity);
        transform.Rotation = glm::quatLookAt(glm::normalize(glm::vec3(-1, -2, -3)), glm::vec3(0, 0, 1));
        transform.Name = "Main Light";
    }

    {
        // Create fill light
        auto lightEntity = bee::Engine.ECS().CreateEntity();
        auto& light = bee::Engine.ECS().CreateComponent<bee::Light>(lightEntity, glm::vec3(0.4f, 0.4f, 0.5f), 100.f, 100.f,
                                                                    bee::Light::Type::Directional);
        light.ShadowExtent = 0.f;
        auto& transform = bee::Engine.ECS().CreateComponent<bee::Transform>(lightEntity);
        transform.Rotation = glm::quatLookAt(glm::normalize(glm::vec3(1, 2, -3)), glm::vec3(0, 0, 1));
        transform.Name = "Fill Light";
    }

    bee::actors::LoadActorsTemplates(m_levelName);
    bee::actors::LoadActorsData(m_levelName);

    auto& questSystem = bee::Engine.ECS().CreateSystem<bee::QuestSystem>();

    auto& quest1 = questSystem.CreateQuest("Main Quest");
    auto& stage1 = questSystem.CreateStage(quest1, "Defend your base!");
    glm::vec3 allyBasePos;
    auto view = bee::Engine.ECS().Registry.view<bee::Transform, AttributesComponent, AllyStructure>();
    for (auto [entity, transform, attributes, enemyTeam] : view.each())
    {
        if (attributes.GetEntityType() == "Base")
        {
            allyBasePos = transform.Translation;
        }
    }
    questSystem.CreateObjective(stage1, "Do not let the enemies in the base", bee::ObjectiveType::Location, 20, "Warrior",
                                Team::Enemy, allyBasePos, 10.0f);

    quest1.Initialize(questSystem.GetUIElementID());
    quest1.StartQuest();

    const unsigned int flags = bee::DebugCategory::General | bee::DebugCategory::Gameplay | bee::DebugCategory::Physics |
                               bee::DebugCategory::Rendering | bee::DebugCategory::AIDecision |
                               bee::DebugCategory::AccelStructs;

    bee::Engine.DebugRenderer().SetCategoryFlags(flags);
}

void TowerDefense::ShutDown()
{
    GameBase::ShutDown();
    //   m_atlases.clear();
}

void TowerDefense::Update(float dt)
{
#ifdef STEAM_API_WINDOWS
    auto& unitPreview = bee::Engine.ECS().GetSystem<UnitPreview>();
    if (bee::Engine.InputWrapper().GetDigitalData("Press_All").pressedOnce)
    {
        if (orderUnitToggle)
        {
            unitPreview.DeActivateSelectOrder();
            unitPreview.ActivateSelectUnits();
        }
        else
        {
            unitPreview.DeActivateSelectUnits();
            unitPreview.ActivateSelectOrder();
        }
        orderUnitToggle = !orderUnitToggle;
    }
#endif
    GameBase::Update(dt);
    m_spawnTimer += dt;
    if (m_spawnTimer >= m_spawnCooldown)
    {
        m_spawnTimer -= m_spawnCooldown;
        SpawnWarrior();
    }
}

void TowerDefense::Render()
{
    auto& terrain = bee::Engine.ECS().GetSystem<lvle::TerrainSystem>();
    terrain.DrawPathing();
    // terrain.DrawBigWireframe(glm::vec4(1.0, 1.0, 1.0, 1.0));
}

void TowerDefense::SpawnWarrior()
{
    bee::Log::Info("Spawned Enemy Warrior");
    m_enemySpawnCount++;
    if (m_enemySpawnCount > 2 && m_spawnCooldown > 0.5f)
    {
        m_spawnCooldown -= 0.5f;
        m_enemySpawnCount = 0;
    }
    auto& unitManager = bee::Engine.ECS().GetSystem<UnitManager>();

    glm::vec3 posToSpawn;
    auto enemyView = bee::Engine.ECS().Registry.view<bee::Transform, AttributesComponent, EnemyStructure>();
    for (auto [entity, transform, attributes, enemyTeam] : enemyView.each())
    {
        posToSpawn = transform.Translation + glm::vec3(5.0f, 0.0f, 0.0f);
    }
    auto unitEntity = unitManager.SpawnUnit("Warrior", posToSpawn, Team::Enemy);
    auto [unitGridAgent, unitStateMachineAgent] =
        bee::Engine.ECS().Registry.get<bee::ai::GridAgent, bee::ai::StateMachineAgent>(unitEntity.value());
    glm::vec3 posToGo;
    auto allyView = bee::Engine.ECS().Registry.view<bee::Transform, AttributesComponent, AllyStructure>();
    for (auto [entity, transform, attributes, enemyTeam] : allyView.each())
    {
        posToGo = transform.Translation;
    }
    // unitGridAgent.SetGoal(posToGo);
    unitStateMachineAgent.context.blackboard->SetData("PositionToMoveTo", glm::vec2(posToGo.x, posToGo.y));
    unitStateMachineAgent.SetStateOfType<MoveToPointState>();
}
