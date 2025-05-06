#include "Starcraft.hpp"

#include <material_system/material_system.hpp>
#include <platform/dx12/animation_system.hpp>
#include <thread>

#include "actors/actor_wrapper.hpp"
#include "actors/buff_system.hpp"
#include "actors/projectile_system/projectile_system.hpp"
#include "actors/props/resource_system.hpp"
#include "actors/selection_system.hpp"
#include "actors/units/unit_manager_system.hpp"
#include "ai/ai_behavior_selection_system.hpp"
#include "ai/behavior_editor_system.hpp"
#include "ai/grid_navigation_system.hpp"
#include "ai/navigation_grid.hpp"
#include "ai_behaviors/wave_system.hpp"
#include "animation/animation_state.hpp"
#include "camera/camera_rts_system.hpp"
#include "core/audio.hpp"
#include "core/device.hpp"
#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "game_ui/in_game_ui.hpp"
#include "game_ui/main_menu.hpp"
#include "leaderboard/leaderboard.hpp"
#include "level_editor/brushes/foliage_brush.hpp"
#include "level_editor/terrain_system.hpp"
#include "light_system/light_system.hpp"
#include "order/order_system.hpp"
#include "particle_system/particle_system.hpp"
#include "physics/world.hpp"
#include "platform/opengl/render_gl.hpp"
#include "quest_system/quest_system.hpp"
#include "tools/3d_utility_functions.hpp"
#include "tools/debug_metric.hpp"
#include "tools/steam_input_system.hpp"
#include "user_interface/user_interface.hpp"
#include "user_interface/user_interface_serializer.hpp"
#include "leaderboard/text_field.hpp"

UI_FUNCTION(ToMainMenuMenu, StartMainMenu, bee::Engine.SetGame<MainMenu>(); bee::Engine.Audio().PlaySoundW("audio/click.wav", 1.5f, false););
UI_FUNCTION(ToBasic, BasicDebugMetricData, auto& UI = bee::Engine.ECS().GetSystem<bee::ui::UserInterface>();
            auto game = static_cast<Starcraft*>(&bee::Engine.Game());
            UI.SetDrawStateUIelement(game->GetDebugMetricBasic(), true);
            UI.SetDrawStateUIelement(game->GetDebugMetricUnits(), false);
            UI.SetDrawStateUIelement(game->GetDebugMetricStructure(), false););
UI_FUNCTION(ToUnits, UnitsDebugMetricData, auto& UI = bee::Engine.ECS().GetSystem<bee::ui::UserInterface>();
            auto game = static_cast<Starcraft*>(&bee::Engine.Game());
            UI.SetDrawStateUIelement(game->GetDebugMetricUnits(), true);
            UI.SetDrawStateUIelement(game->GetDebugMetricBasic(), false);
            UI.SetDrawStateUIelement(game->GetDebugMetricStructure(), false););
UI_FUNCTION(ToStructures, StructureDebugMetricData, auto& UI = bee::Engine.ECS().GetSystem<bee::ui::UserInterface>();
            auto game = static_cast<Starcraft*>(&bee::Engine.Game());
            UI.SetDrawStateUIelement(game->GetDebugMetricStructure(), true);
            UI.SetDrawStateUIelement(game->GetDebugMetricBasic(), false);
            UI.SetDrawStateUIelement(game->GetDebugMetricUnits(), false););
UI_FUNCTION(ToAppear, MakeMetricDataAppear, auto& UI = bee::Engine.ECS().GetSystem<bee::ui::UserInterface>();
            auto game = static_cast<Starcraft*>(&bee::Engine.Game()); game->SetDebugMetricState(!game->GetDebugMetricState());
            UI.SetDrawStateUIelement(game->GetDebugMetricStructure(), false);
            UI.SetDrawStateUIelement(game->GetDebugMetricBasic(), false);
            UI.SetDrawStateUIelement(game->GetDebugMetricUnits(), false);
            UI.SetDrawStateUIelement(game->GetDebugMetricButtons(), game->GetDebugMetricState());
            UI.SetDrawStateUIelement(game->GetDebugMetricBackground(), game->GetDebugMetricState()););

UI_FUNCTION(ToTextField, TextFieldButton, auto game = static_cast<Starcraft*>(&bee::Engine.Game());
            game->ChangeInputNameToggle(!game->IsInputNameEnabled());
            for (auto [ent, text] : bee::Engine.ECS().Registry.view<TextField>().each())
            {
            if (game->IsInputNameEnabled())
                text.StartKeyboardHook();
            else
                text.StopKeyboardHook();
            }
);

#include <material_system/material_system.hpp>

void Starcraft::Init()
{
    GameBase::Init();
    srand(time(nullptr));
    auto& UI = bee::Engine.ECS().GetSystem<bee::ui::UserInterface>();
    /*  bee::ui::UIElementID ID = UI.serialiser->LoadElement("test");
      const auto& item = UI.serialiser->getComponentItem<bee::ui::Text>(ID, "TextComponent");
      std::cout << item.text;*/

    {  // Terrain
        auto& terrain_system = bee::Engine.ECS().GetSystem<lvle::TerrainSystem>();
        terrain_system.LoadLevel(m_levelName);
        terrain_system.UpdateTerrainDataComponent();
        terrain_system.CalculateTerrainColliders();
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

    //creating the pause menu UI elemtns
    m_pauseMenu = UI.serialiser->LoadElement("PauseMenu");
    m_pauseMenuBackground = UI.serialiser->LoadElement("PauseMenuBackground");
    m_pauseMenuOpacity = UI.serialiser->LoadElement("PauseMenuOpacity");

    // making the UI elements invisible
    UI.SetDrawStateUIelement(m_pauseMenu, false);
    UI.SetDrawStateUIelement(m_pauseMenuBackground, false);
    UI.SetDrawStateUIelement(m_pauseMenuOpacity, false);
    // const auto& item = UI.serialiser->getComponentItem<bee::ui::Text>(ID, "TextComponent");
    // std::cout << item.text;
    bee::actors::LoadActorsTemplates(m_levelName);
    bee::actors::LoadActorsData(m_levelName);

    bee::Engine.ECS().CreateSystem<bee::physics::World>(0.02f);
    bee::Engine.ECS().CreateSystem<bee::ai::BehaviorEditorSystem>();
    bee::Engine.ECS().CreateSystem<ProjectileSystem>();
    bee::Engine.ECS().CreateSystem<bee::ParticleSystem>();

    auto& resourceSystem = bee::Engine.ECS().CreateSystem<ResourceSystem>();
    bee::Engine.ECS().CreateSystem<SelectionSystem>();
    // bee::Engine.ECS().CreateSystem<UnitPreview>();
    bee::Engine.ECS().CreateSystem<InGameUI>();

    resourceSystem.ChangeResourceCount(GameResourceType::Wood,
                                       resourceSystem.playerResourceData.resources[GameResourceType::Wood]);
    resourceSystem.ChangeResourceCount(GameResourceType::Stone,
                                       resourceSystem.playerResourceData.resources[GameResourceType::Stone]);
    bee::actors::CreateActorSystems();

    bee::Engine.ECS().CreateSystem<bee::ai::AIBehaviorSelectionSystem>(1.0f / 6.0f);

    bee::Engine.ECS().CreateSystem<bee::MaterialSystem>();

    auto& lightSystem = bee::Engine.ECS().CreateSystem<bee::LightSystem>();
    lightSystem.LoadLights(m_levelName);

    {
        lvle::FoliageBrush foliageBrush;
        foliageBrush.RemovePreviewModel();
        foliageBrush.Deactivate();
        foliageBrush.Disable();
        foliageBrush.LoadFoliageTemplates(m_levelName);
        foliageBrush.LoadFoliageInstances(m_levelName);
    }

    // fill our treasury yaharrrrr
    // resourceSystem.playerResourceData.resources[GameResourceType::Wood] = 10000;
    // resourceSystem.playerResourceData.resources[GameResourceType::Stone] = 10000;

    // auto& questSystem = bee::Engine.ECS().CreateSystem<bee::QuestSystem>();

    const unsigned int flags = bee::DebugCategory::General;

    {
        auto enemyAI = bee::Engine.ECS().Registry.create();
        auto fsm = bee::Engine.Resources().Load<bee::ai::FiniteStateMachine>("fsm/enemyAIFsm.json");
        auto& transform = bee::Engine.ECS().CreateComponent<bee::Transform>(enemyAI);
        transform.Name = "EnemyAI";
        auto& agent = bee::Engine.ECS().CreateComponent<bee::ai::StateMachineAgent>(enemyAI, *fsm);
        agent.context.entity = enemyAI;
        bee::Engine.DebugRenderer().SetCategoryFlags(flags);
        bee::Engine.ECS().CreateSystem<WaveSystem>().Pause(true);
        bee::Engine.ECS().CreateSystem<BuffSystem>();
    }

    auto camera = bee::Engine.ECS().GetSystem<bee::CameraSystemRTS>();
    camera.ResetCamera();
    bee::Engine.ECS().CreateSystem<OrderSystem>();
    for (auto [entity, debugMetric] : bee::Engine.ECS().Registry.view<bee::DebugMetricData>().each()) debugMetric.Clear();

    auto entityTextField = bee::Engine.ECS().CreateEntity();
    bee::Engine.ECS().CreateComponent<TextField>(entityTextField);
    

    bee::Engine.ECS().GetSystem<lvle::TerrainSystem>().UpdateTerrainDataComponent();
    bee::Engine.ECS().GetSystem<bee::ai::GridNavigationSystem>().UpdateFromTerrain();

    //Initializing all sounds
    InitializeSounds();
}

void Starcraft::ShutDown()
{
    GameBase::ShutDown();

    SaveDebugMetrics();
    SaveLeaderboardScore();

    auto& orderSystem = bee::Engine.ECS().GetSystem<OrderSystem>();
    orderSystem.mageLimit = 0;
    orderSystem.swordsmenLimit = 0;

    bee::Engine.ECS().RemoveAllSystemAfter<bee::ai::GridNavigationSystem>();
    bee::Engine.ECS().GetSystem<bee::ui::UserInterface>().Clean();

    for (auto entity : bee::Engine.ECS().Registry.storage<bee::Entity>().each())
    {
        if (bee::Engine.ECS().Registry.try_get<bee::Camera>(std::get<0>(entity)) != nullptr ||
            bee::Engine.ECS().Registry.try_get<bee::ui::internal::UIElement>(std::get<0>(entity)) != nullptr ||
            bee::Engine.ECS().Registry.try_get<lvle::TerrainDataComponent>(std::get<0>(entity)) != nullptr)
            continue;
        if (bee::Engine.ECS()
            .Registry.any_of<AttributesComponent, bee::Light, bee::physics::PolygonCollider, bee::physics::DiskCollider,
            Corpse, bee::Transform, TextField>(std::get<0>(entity)))
        {
            bee::Engine.ECS().DeleteEntity(std::get<0>(entity));
        }
    }

    for (auto system : bee::Engine.ECS().GetSystems<bee::System>())
    {
        system->Pause(false);
    }
    bee::Engine.Input().SetPaused(false);
    for (auto [ent, text] : bee::Engine.ECS().Registry.view<TextField>().each())
    {
        text.StopKeyboardHook();
        text.ResetTextInput();
    }
}

void Starcraft::Render()
{
    auto& terrain = bee::Engine.ECS().GetSystem<lvle::TerrainSystem>();
    terrain.DrawPathing();
    // terrain.DrawBigWireframe(glm::vec4(1.0, 1.0, 1.0, 1.0));
}

void Starcraft::Pause(bool pause)
{
    m_pause = pause;
    for (auto system : bee::Engine.ECS().GetSystems<bee::System>())
    {
        system->Pause(pause);
    }
    auto in_game_ui = bee::Engine.ECS().GetSystem<InGameUI>();
    bee::Engine.ECS().GetSystem<WaveSystem>().Pause(in_game_ui.IsWaveSystemPaused()?pause:true);

    bee::Engine.Input().SetPaused(pause);

    auto& UI = bee::Engine.ECS().GetSystem<bee::ui::UserInterface>();
    for (auto [entity,uiElement]:bee::Engine.ECS().Registry.view<bee::ui::internal::UIElement>().each())
    {
        UI.SetInputStateUIelement(uiElement.ID,( !pause&&UI.GetDrawStateUIelement(uiElement.ID)));
    }

    UI.SetDrawStateUIelement(m_pauseMenu, !m_endScreen?pause:false);
    UI.SetDrawStateUIelement(m_pauseMenuBackground, !m_endScreen ? pause : false);
    UI.SetDrawStateUIelement(m_pauseMenuOpacity, !m_endScreen ? pause : false);
}

void Starcraft::EndScreen()
{
    for (auto [entity, debugMetric] : bee::Engine.ECS().Registry.view<bee::DebugMetricData>().each())
    {
        for (auto upgrades : debugMetric.upgradedBuildings)
        {
            debugMetric.buildingSpawned.erase(upgrades.first);
        }
    }
    // Gets the window size
    const float height = static_cast<float>(bee::Engine.Device().GetHeight());
    const float width = static_cast<float>(bee::Engine.Device().GetWidth());
    const float norWidth = width / height;

    auto waveSystem = bee::Engine.ECS().GetSystem<WaveSystem>();

    auto leaderboardView = bee::Engine.ECS().Registry.view<Leaderboard>();
    for (auto [entity, leaderboardComponent] : leaderboardView.each())
    {
        for (auto [ent, text] : bee::Engine.ECS().Registry.view<TextField>().each())
        {
            m_playerScoreIndex =
                leaderboardComponent.AddScore(std::pair<std::string, int>(text.GetTextInput(), waveSystem.GetCurrentWave() - 1));
        }
        leaderboardComponent.ShowLeaderboard(m_playerScoreIndex);
    }
    auto& UI = bee::Engine.ECS().GetSystem<bee::ui::UserInterface>();
    UI.LoadFont(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Asset, "fonts/DroidSans.sff"));
    std::string str0 =
        bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Asset, "textures/UI/Game UI collection/png files/Bars-01.png");
    m_hover.Img = m_buttonBack.Img = UI.LoadTexture(str0);

    bee::ui::UIComponentID m_start;
    m_hoverid = UI.AddOverlayImage(m_hover);
    //
    // Pause Screen
    //
    {
        // m_element = UI.CreateUIElement(bee::ui::top, bee::ui::left);
        glm::vec2 middle = glm::vec2((norWidth / 2) - ((m_buttonBack.GetNormSize(m_elementSize).x) / 2),
                                     0.5f - (m_buttonBack.GetNormSize(m_elementSize).y) / 2);
        bee::ui::UIComponentID exitButt;

        m_background = UI.CreateUIElement(bee::ui::Alignment::top, bee::ui::Alignment::left);
        int mainMenuImage = UI.LoadTexture(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Asset, "textures/white.png"));
        UI.CreateTextureComponentFromAtlas(m_background, mainMenuImage, glm::vec4(0.0f, 0.0f, 4.0f, 4.0f), glm::vec2(0.f, 0.f),
                                           glm::vec2(norWidth, 1.0f));
        // Creates an semi transparent background for the pause screen
        UI.SetElementLayer(m_background, 3);
        UI.SetElementOpacity(m_background, 0.5f);

        m_element = UI.serialiser->LoadElement("EndScreen");
        std::string gameName = "Last Wave Survived: " + std::to_string(waveSystem.GetCurrentWave() - 1);
        UI.ReplaceString(UI.GetComponentID(m_element, "LastWaveText"), gameName);
    }
    m_debugMetricBackground = UI.CreateUIElement(bee::ui::Alignment::top, bee::ui::Alignment::left);
    int background = UI.LoadTexture(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Asset, "textures/UI/Loose_Scroll.png"));
    UI.CreateTextureComponentFromAtlas(m_debugMetricBackground, background, glm::vec4(0.0f, 0.0f, 1072, 1069), glm::vec2(0.f, 0.0f),
                                       glm::vec2(0.4f, 0.8f));
    UI.SetElementLayer(m_debugMetricBackground, 2);

    m_debugMetricButtons = UI.serialiser->LoadElement("Button");
    m_debugMetricBasic = UI.serialiser->LoadElement("Basic");
    m_debugMetricUnits = UI.serialiser->LoadElement("Units");
    m_debugMetricStructures = UI.serialiser->LoadElement("Structures");
    m_debugMetricData = UI.serialiser->LoadElement("DebugMetrics");
    for (auto [entity, debugMetric] : bee::Engine.ECS().Registry.view<bee::DebugMetricData>().each())
    {
        UI.ReplaceString(UI.GetComponentID(m_debugMetricBasic, "InGameTime"),
                         std::string("In game time: " + std::to_string(debugMetric.timeInsideGame)));
        UI.ReplaceString(UI.GetComponentID(m_debugMetricBasic, "DiedOnWave"),
                         std::string("Died on wave: " + std::to_string(debugMetric.diedOnWave)));
        UI.ReplaceString(UI.GetComponentID(m_debugMetricBasic, "Wood"),
                         std::string("Wood wasted: " + std::to_string(debugMetric.woodDespawned)));
        UI.ReplaceString(UI.GetComponentID(m_debugMetricBasic, "Stone"),
                         std::string("Stone wasted: " + std::to_string(debugMetric.stoneDespawned)));
        UI.ReplaceString(UI.GetComponentID(m_element, "Rank"), "Your rank: " + std::to_string(m_playerScoreIndex + 1));
        int typeIndex = 1;
        for (auto type : debugMetric.allyUnitsSpawned)
        {
            UI.ReplaceString(UI.GetComponentID(m_debugMetricUnits, std::string("Ally" + std::to_string(typeIndex) + "Spawned")),
                             std::string(type.first + " spawned: " + std::to_string(type.second)));
            typeIndex++;
        }
        while (typeIndex < 3)
        {
            UI.ReplaceString(UI.GetComponentID(m_debugMetricUnits, std::string("Ally" + std::to_string(typeIndex) + "Spawned")),
                             "");
            typeIndex++;
        }
        typeIndex = 1;
        for (auto type : debugMetric.allyUnitsKilled)
        {
            UI.ReplaceString(UI.GetComponentID(m_debugMetricUnits, std::string("Ally" + std::to_string(typeIndex) + "Killed")),
                             std::string(type.first + " died: " + std::to_string(type.second)));
            typeIndex++;
        }
        while (typeIndex < 3)
        {
            UI.ReplaceString(UI.GetComponentID(m_debugMetricUnits, std::string("Ally" + std::to_string(typeIndex) + "Killed")),
                             "");
            typeIndex++;
        }
        typeIndex = 1;
        for (auto type : debugMetric.enemyUnitsSpawned)
        {
            UI.ReplaceString(
                UI.GetComponentID(m_debugMetricUnits, std::string("Enemy" + std::to_string(typeIndex) + "Spawned")),
                std::string(type.first + " spawned: " + std::to_string(type.second)));
            typeIndex++;
        }
        while (typeIndex < 3)
        {
            UI.ReplaceString(
                UI.GetComponentID(m_debugMetricUnits, std::string("Enemy" + std::to_string(typeIndex) + "Spawned")), "");
            typeIndex++;
        }
        typeIndex = 1;
        for (auto type : debugMetric.enemyUnitsKilled)
        {
            UI.ReplaceString(UI.GetComponentID(m_debugMetricUnits, std::string("Enemy" + std::to_string(typeIndex) + "Killed")),
                             std::string(type.first + " killed: " + std::to_string(type.second)));
            typeIndex++;
        }
        while (typeIndex < 3)
        {
            UI.ReplaceString(UI.GetComponentID(m_debugMetricUnits, std::string("Enemy" + std::to_string(typeIndex) + "Killed")),
                             "");
            typeIndex++;
        }
        typeIndex = 1;
        for (auto type : debugMetric.buildingSpawned)
        {
            UI.ReplaceString(UI.GetComponentID(m_debugMetricStructures, std::string("Structure" + std::to_string(typeIndex))),
                             std::string(type.first + ": " + std::to_string(type.second)));
            typeIndex++;
        }
        while (typeIndex < 5)
        {
            UI.ReplaceString(UI.GetComponentID(m_debugMetricStructures, std::string("Structure" + std::to_string(typeIndex))),
                             "");
            typeIndex++;
        }
        for (auto type : debugMetric.upgradedBuildings)
        {
            UI.ReplaceString(UI.GetComponentID(m_debugMetricStructures, std::string("Structure" + std::to_string(typeIndex))),
                             std::string(type.first + ": " + std::to_string(type.second)));
            typeIndex++;
        }
        while (typeIndex < 17)
        {
            UI.ReplaceString(UI.GetComponentID(m_debugMetricStructures, std::string("Structure" + std::to_string(typeIndex))),
                             "");
            typeIndex++;
        }
    }
    UI.SetDrawStateUIelement(m_debugMetricBasic, false);
    UI.SetDrawStateUIelement(m_debugMetricUnits, false);
    UI.SetDrawStateUIelement(m_debugMetricStructures, false);
    UI.SetDrawStateUIelement(m_debugMetricBackground, false);
    UI.SetDrawStateUIelement(m_debugMetricButtons, false);
}

void Starcraft::SaveLeaderboardScore()
{
    auto leaderboardView = bee::Engine.ECS().Registry.view<Leaderboard>();
    for (auto [entity, leaderboardComponent] : leaderboardView.each())
    {
        for (auto [ent, text] : bee::Engine.ECS().Registry.view<TextField>().each())
        {
            if (text.GetTextInput().size() == 0) leaderboardComponent.UpdateUserName(m_playerScoreIndex, "Owlbert");
            leaderboardComponent.SaveScores();
        }
    }
}

void Starcraft::SaveDebugMetrics()
{
    for (auto [entity, debugMetric] : bee::Engine.ECS().Registry.view<bee::DebugMetricData>().each()) debugMetric.SaveData();
}

void Starcraft::InitializeSounds()
{
    bee::Engine.Audio().LoadSound("audio/building2.wav", false); // done
    bee::Engine.Audio().LoadSound("audio/click.wav", false);
    bee::Engine.Audio().LoadSound("audio/friend_spawn.wav", false); // done
    bee::Engine.Audio().LoadSound("audio/mage_shoot.wav", false); // done
    bee::Engine.Audio().LoadSound("audio/melee_hit3.wav", false); // done but sound needs rework
    bee::Engine.Audio().LoadSound("audio/not_possible.wav", false); 
    bee::Engine.Audio().LoadSound("audio/owl_death.wav", false); // done
    bee::Engine.Audio().LoadSound("audio/rock_death_variation.wav", false); //done
    bee::Engine.Audio().LoadSound("audio/horn_alert.wav", false); // done
    bee::Engine.Audio().LoadSound("audio/wood_death.wav", false); //done
}

void Starcraft::Update(float dt)
{
    if (bee::Engine.Input().GetKeyboardKeyOnce(bee::Input::KeyboardKey::Escape) && !m_endScreen)
    {
        Pause(!m_pause);
    }
    bool endGame = true;
    for (auto [entity, allyStructure, transform] : bee::Engine.ECS().Registry.view<AllyStructure, bee::Transform>().each())
    {
        if (transform.Name.find("TownHall") != std::string::npos) endGame = false;
    }
    if (endGame && !m_endScreen)
    {
        m_endScreen = true;

        auto& UI = bee::Engine.ECS().GetSystem<bee::ui::UserInterface>();
        // auto& resourceUI = bee::Engine.ECS().GetSystem<ResourceUI>();
        //  auto& preview = bee::Engine.ECS().GetSystem<UnitPreview>();
        // UI.SetInputStateUIelement(resourceUI.element, false);
        //  UI.SetInputStateUIelement(preview.element, false);
        //  UI.SetInputStateUIelement(preview.elementRight, false);
        //auto& resourceUI = bee::Engine.ECS().GetSystem<ResourceUI>();
        // auto& preview = bee::Engine.ECS().GetSystem<UnitPreview>();
        // UI.SetInputStateUIelement(resourceUI.element, false);
        for (auto [entity, debugMetric] : bee::Engine.ECS().Registry.view<bee::DebugMetricData>().each())
            debugMetric.diedOnWave = bee::Engine.ECS().GetSystem<WaveSystem>().GetCurrentWave();
        // UI.SetInputStateUIelement(preview.element, false);
        // UI.SetInputStateUIelement(preview.elementRight, false);
        auto& gameUI = bee::Engine.ECS().GetSystem<InGameUI>();
        gameUI.SetInputState(false);
        Pause(true);
        EndScreen();
    }
    if (!m_endScreen)
    {
        for (auto [entity, debugMetric] : bee::Engine.ECS().Registry.view<bee::DebugMetricData>().each())
            debugMetric.timeInsideGame += dt;
    }
    else
    {
        for (auto [ent, text] : bee::Engine.ECS().Registry.view<TextField>().each())
        {
            if (text.GetUpdateString())
            {
                if (!text.GetRunningState())
                {
                    text.StopKeyboardHook();
                    m_inputName = false;
                }
                text.SetUpdateString(false);
                auto& UI = bee::Engine.ECS().GetSystem<bee::ui::UserInterface>();
                UI.ReplaceString(UI.GetComponentID(m_element, "TextInput"), text.GetTextInput());
                auto leaderboardView = bee::Engine.ECS().Registry.view<Leaderboard>();
                for (auto [entity, leaderboardComponent] : leaderboardView.each())
                {
                    leaderboardComponent.UpdateUserName(m_playerScoreIndex, text.GetTextInput());
                }
            }
            const float height = static_cast<float>(bee::Engine.Device().GetHeight());
            const float width = static_cast<float>(bee::Engine.Device().GetWidth());
            const float norWidth = width / height;
            auto mousePos = bee::Engine.Input().GetMousePosition();
            mousePos = glm::vec2((mousePos.x / width) * norWidth, mousePos.y / height);
            glm::vec4 bounds = bee::Engine.ECS()
                                   .GetSystem<bee::ui::UserInterface>()
                                   .getComponentItem<bee::ui::sButton>(m_element, "TextField")
                                   .bounds;
            if (bee::Engine.Input().GetMouseButtonOnce(bee::Input::MouseButton::Left) &&
                !((mousePos.x >= bounds.x && mousePos.x <= bounds.z) && (mousePos.y >= bounds.y && mousePos.y <= bounds.w)))
            {
                text.StopKeyboardHook();
                m_inputName = false;
            }
        }
    }

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
}
