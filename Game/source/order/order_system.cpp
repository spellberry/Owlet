#include "order/order_system.hpp"

#include <imgui/crude_json.h>
#include <imgui/imgui.h>

#include <glm/fwd.hpp>

#include "actors/props/prop_manager_system.hpp"
#include "actors/props/resource_system.hpp"
#include "actors/selection_system.hpp"
#include "actors/structures/structure_manager_system.hpp"
#include "core/audio.hpp"
#include "actors/units/unit_order_type.hpp"
#include "ai/FiniteStateMachines/finite_state_machine.hpp"
#include "ai/ai_behavior_selection_system.hpp"
#include "ai/grid_navigation_system.hpp"
#include "ai_behaviors/structure_behaviors.hpp"
#include "ai_behaviors/unit_behaviors.hpp"
#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "core/input.hpp"
#include "core/transform.hpp"
#include "game_ui/in_game_ui.hpp"
#include "imgui/imgui_stdlib.h"
#include "level_editor/brushes/structure_brush.hpp"
#include "level_editor/terrain_system.hpp"
#include "tools/3d_utility_functions.hpp"
#include "tools/asset_explorer_system.hpp"
#include "tools/debug_metric.hpp"
#include "tools/steam_input_system.hpp"
#include "tools/tools.hpp"
#include "user_interface/user_interface.hpp"
#include "user_interface/user_interface_serializer.hpp"
#include "user_interface/user_interface_structs.hpp"

UI_FUNCTION(ToBuildBarracks, BuildBarracks,
            bee::Engine.ECS().GetSystem<OrderSystem>().SetCurrentOrder(OrderType::BuildBarracks););
UI_FUNCTION(ToBuildMageTower, BuildMageTower,
            bee::Engine.ECS().GetSystem<OrderSystem>().SetCurrentOrder(OrderType::BuildMageTower););

OrderSystem::OrderSystem()
{
    InitializeOrders();
    InitializeGoalEntity();
    InitializeAttackGoalEntity();
    Title = "Order system";
    DragInitialize();
    //this is commented out because of a bug, which prevents the supply to be reset upon restart
    //plus there is no point in doing it if we start with level 0 buildings
    /*const auto structureView = bee::Engine.ECS().Registry.view<AllyStructure, AttributesComponent>();

    for (auto entity : structureView)
    {
        auto& attributes = bee::Engine.ECS().Registry.get<AttributesComponent>(entity);

        if (attributes.HasAttribute(BaseAttributes::SwordsmenLimitIncrease))
        {
            swordsmenLimit += attributes.GetValue(BaseAttributes::SwordsmenLimitIncrease);
        }

        if (attributes.HasAttribute(BaseAttributes::MageLimitIncrease))
        {
            mageLimit += attributes.GetValue(BaseAttributes::MageLimitIncrease);
        }
    }*/
}

OrderSystem::~OrderSystem() {}

void OrderSystem::Update(float dt)
{
    System::Update(dt);
    DragUpdate();  // Check if we are dragging, and update if so
    InputUpdate(dt);
}

void OrderSystem::HandleLeftMouseClick()
{
    const bool build = m_currentOrder == OrderType::BuildBarracks || m_currentOrder == OrderType::BuildMageTower ||
                       m_currentOrder == OrderType::BuildWallStart || m_currentOrder == OrderType::BuildFenceStart;

    if (!build)
        SetCurrentOrder(OrderType::Move);

    else if (m_drag)
    {
        if (m_wallType) // Is the wall a fence?
        {
            SetCurrentOrder(OrderType::BuildFenceEnd);
        }

        else // The wall is a wall
        {
            SetCurrentOrder(OrderType::BuildWallEnd);
        }
    }

    m_orders[m_currentOrder].orderBody();
}

void OrderSystem::HandleRightMouseClick()
{
    const bool unit = m_currentOrder == OrderType::Attack || m_currentOrder == OrderType::Move;
    const bool build = m_currentOrder == OrderType::BuildBarracks || m_currentOrder == OrderType::BuildMageTower ||
                       m_currentOrder == OrderType::BuildWallStart || m_currentOrder == OrderType::BuildFenceStart;

    if (unit)
    {
        bee::Entity targetEntity{};
        const bool mouse_hit_response = bee::MouseHitResponse(targetEntity);

        if (mouse_hit_response)
            SetCurrentOrder(!bee::Engine.ECS().Registry.try_get<EnemyUnit>(targetEntity) ? OrderType::Move : OrderType::Attack);

        else
            SetCurrentOrder(OrderType::Move);
    }
    else if (m_drag || build)
    {
        // Clearing drag data just in case
        m_drag = false;
        m_dragPoints.clear();
        m_structureBrush.RemovePreviewModel();
        DragReset();
        SetCurrentOrder(OrderType::Move);
    }

    m_orders[m_currentOrder].orderBody();
}

void OrderSystem::PollWrapperInput()
{
    // Polling input wrapper short cuts
    for (auto order:buttons)
    {
        const crude_json::string itemName = magic_enum::enum_name(order).data();
        if (bee::Engine.InputWrapper().GetDigitalData(itemName).pressedOnce)
        {
            SetCurrentOrder(order);
        }
    }
    auto selectedUnitView = bee::Engine.ECS().Registry.view<Selected,AllyUnit,AttributesComponent>();
    auto& unitManager = bee::Engine.ECS().GetSystem<UnitManager>();
    auto currentOrder = OrderType::None;
    for (auto [entity,selected,unit,attributes]:selectedUnitView.each())
    {
        for (auto order : unitManager.GetUnitTemplate(attributes.GetEntityType()).availableOrders)
        {
            const crude_json::string itemName = magic_enum::enum_name(order).data();
            if (bee::Engine.InputWrapper().GetDigitalData(itemName).pressedOnce && currentOrder!=order)
            {
                currentOrder = order;
                SetCurrentOrder(order);
            }
        }
    }

     auto selectedStructureView = bee::Engine.ECS().Registry.view<Selected, AllyStructure, AttributesComponent>();
    auto& structureManager = bee::Engine.ECS().GetSystem<StructureManager>();
    for (auto [entity, selected, structure, attributes] : selectedStructureView.each())
    {
        for (auto order : structureManager.GetStructureTemplate(attributes.GetEntityType()).availableOrders)
        {
            const crude_json::string itemName = magic_enum::enum_name(order).data();
            if (bee::Engine.InputWrapper().GetDigitalData(itemName).pressedOnce && currentOrder != order)
            {
                currentOrder = order;
                SetCurrentOrder(order);
            }
        }
    }
}

void OrderSystem::InputUpdate(float dt)
{
    if (bee::Engine.InputWrapper().GetDigitalData("Selection").pressedOnce) HandleLeftMouseClick();

    if (bee::Engine.InputWrapper().GetDigitalData("Order").pressedOnce) HandleRightMouseClick();

    const glm::vec3 mouseWorldDirection = GetDirectionToMouse();
    const glm::vec3 cameraWorldPosition = GetCameraPosition();
    glm::vec3 hit{};
    const bool isPointInMap = GetTerrainRaycastPosition(mouseWorldDirection, cameraWorldPosition, hit);

    if (bee::IsMouseInGameWindow() && isPointInMap && m_brushActive)
    {
        m_structureBrush.Update(hit);
        m_structureBrush.Render(hit);
    }
    if (m_currentOrder == OrderType::Move)
    {
        m_brushActive = false;
        m_structureBrush.RemovePreviewModel();
    }
}

glm::vec3 OrderSystem::GetDirectionToMouse()
{
    auto& terrain = bee::Engine.ECS().GetSystem<lvle::TerrainSystem>();

    const auto view = bee::Engine.ECS().Registry.view<bee::Transform, bee::Camera>();
    const auto& input = bee::Engine.Input();
    for (auto& entity : view)
    {
        auto [transform, camera] = view.get(entity);

#ifdef STEAM_API_WINDOWS
        if (!bee::Engine.ECS().GetSystem<SelectionSystem>().ControllerInput())
            return bee::GetRayFromScreenToWorld(input.GetMousePosition(), camera, transform);
        else
        {
            const auto scrWidth = bee::Engine.Inspector().GetGameSize().x;
            const auto scrHeight = bee::Engine.Inspector().GetGameSize().y;
            glm::vec2 mousePos = {scrWidth * 0.5f + bee::Engine.Inspector().GetGamePos().x,
                                  scrHeight * 0.5f + bee::Engine.Inspector().GetGamePos().y};
            return bee::GetRayFromScreenToWorld(mousePos, camera, transform);
        }
#else
        return bee::GetRayFromScreenToWorld(input.GetMousePosition(), camera, transform);
#endif
    }
    return {};
}

glm::vec3 OrderSystem::GetCameraPosition()
{
    const auto screen = bee::Engine.ECS().Registry.view<bee::Transform, bee::Camera>();
    for (auto& entity : screen)
    {
        auto [transform, camera] = screen.get(entity);
        return transform.Translation;
    }
    return {};
}

void OrderSystem::SetCurrentOrder(const OrderType type)
{
    m_currentOrder = type;
    m_orders[type].onOrderSet();
}

void OrderSystem::Render() { System::Render(); }

bee::ui::Icon& OrderSystem::GetOrderTemplate(OrderType type) { return m_OrderIcons.at(type); }

std::pair<glm::vec2, bool> OrderSystem::GetOrderCost(OrderType orderType)
{
    m_orders[orderType].onOrderHover();
    return m_orderCost[orderType];
}

void OrderSystem::SaveOrderTemplates()
{
    std::unordered_map<OrderType, bee::ui::Icon>& OrderIcons = m_OrderIcons;

    std::ofstream os(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::UserInterface, "icons/orderTemplates.json"));
    cereal::JSONOutputArchive archive(os);
    archive(CEREAL_NVP(OrderIcons));
}

void OrderSystem::LoadOrderTemplates()
{
    if (bee::fileExists(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::UserInterface, "icons/orderTemplates.json")))
    {
        {
            std::ifstream is(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::UserInterface, "icons/orderTemplates.json"));
            cereal::JSONInputArchive archive(is);

            std::unordered_map<OrderType, bee::ui::Icon> OrderIcons;
            archive(CEREAL_NVP(OrderIcons));
            m_OrderIcons = OrderIcons;
        }
    }
    // if new enums are made emplace them automatically
    {
        std::unordered_map<OrderType, bee::ui::Icon>& OrderIcons = m_OrderIcons;

        std::underlying_type_t<OrderType> sum;
        magic_enum::enum_for_each<OrderType>(
            [&sum, &OrderIcons](auto val)
            {
                if (OrderIcons.count(val()) == 0)
                {
                    bee::ui::Icon icon;
                    constexpr std::underlying_type_t<OrderType> v = magic_enum::enum_integer(val());
                    sum += v;

                    OrderIcons.emplace(val(), icon);
                }
            });
        m_OrderIcons = OrderIcons;
        SaveOrderTemplates();
    }
}

#ifdef BEE_INSPECTOR
void OrderSystem::Inspect()
{
    auto& assetSystem = bee::Engine.ECS().GetSystem<bee::AssetExplorer>();

    System::Inspect();
    ImGui::Begin("Order system");

    auto& orderSystem = bee::Engine.ECS().GetSystem<OrderSystem>();

    std::underlying_type_t<OrderType> sum;
    magic_enum::enum_for_each<OrderType>(
        [&sum, &orderSystem, &assetSystem](auto val)
        {
            constexpr std::underlying_type_t<OrderType> v = magic_enum::enum_integer(val());
            sum += v;
            bee::ui::Icon& icon = orderSystem.GetOrderTemplate(val());
            ImGui::Text(std::string(magic_enum::enum_name(val())).c_str());
            if (ImGui::InputText(
                    std::string(std::string("Icon") + std::string(" ##Order") + std::string(magic_enum::enum_name(val())))
                        .c_str(),
                    &icon.iconPath))
            {
                if (bee::fileExists(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Asset, icon.iconPath)))
                {
                    float width = 0, height = 0;
                    bee::GetPngImageDimensions(icon.iconPath, width, height);
                    icon.iconTextureCoordinates.z = width;
                    icon.iconTextureCoordinates.w = height;
                }
            }
            std::filesystem::path path;
            if (assetSystem.SetDragDropTarget(path, {".png"}))
            {
                icon.iconPath = path.string();
                float w, h;
                bee::GetPngImageDimensions(icon.iconPath, w, h);
                icon.iconTextureCoordinates = glm::vec4(0.0f, 0.0f, w, h);
            }
            ImGui::InputFloat2(std::string(std::string("coordinates (in pixels)") + std::string(" ##Order") +
                                           std::string(magic_enum::enum_name(val())))
                                   .c_str(),
                               &icon.iconTextureCoordinates.x);
            ImGui::InputFloat2(std::string(std::string("size (in pixels)") + std::string(" ##Order") +
                                           std::string(magic_enum::enum_name(val())))
                                   .c_str(),
                               &icon.iconTextureCoordinates.z);
            ImGui::Separator();
        });

    if (ImGui::Button("Apply"))
    {
        orderSystem.SaveOrderTemplates();
    }
    ImGui::End();
}

void OrderSystem::Inspect(bee::Entity e) { System::Inspect(e); }
#endif

// ----------------------------------------
// Private Methods
// ----------------------------------------

bool OrderSystem::GetTerrainRaycastPosition(const glm::vec3 mouseWorldDirection, const glm::vec3 cameraWorldPosition,
                                            glm::vec3& point)
{
    auto& terrain_system = bee::Engine.ECS().GetSystem<lvle::TerrainSystem>();
    const bool result = terrain_system.FindRayMeshIntersection(cameraWorldPosition, mouseWorldDirection, point);
    return result;
}

// --- Initializing Unit orders

void OrderSystem::InitializeAttackOrder()
{
    Order attackOrder;
    attackOrder.orderBody = [&]()
    {
        bee::Entity targetEntity{};  // The entity that will be attacked

        const bool rightClicked = bee::Engine.InputWrapper().GetDigitalData("Order").pressedOnce;
        const auto unitView = bee::Engine.ECS().Registry.view<AllyUnit, Selected, bee::Transform, bee::ai::StateMachineAgent>();

        // Safety checks
        if (!bee::MouseHitResponse(targetEntity)) return;                          // No target selected
        if (!bee::Engine.ECS().Registry.try_get<EnemyUnit>(targetEntity)           // Target is not enemy unit
            && !bee::Engine.ECS().Registry.try_get<EnemyStructure>(targetEntity))  // Target is not enemy structure
            return;

        // Applying new order to all selected ally units
        for (const auto entity : unitView)
        {
            auto& stateMachineAgent = unitView.get<bee::ai::StateMachineAgent>(entity);
            if (stateMachineAgent.IsInState<DeadState>()) continue;
            stateMachineAgent.context.blackboard->SetData<bee::Entity>("TargetEntity", targetEntity);
            if (stateMachineAgent.fsm.StateExists<RangedAttackState>())
                stateMachineAgent.SetStateOfType<RangedAttackState>();
            else if (stateMachineAgent.fsm.StateExists<MeleeAttackState>())
                stateMachineAgent.SetStateOfType<MeleeAttackState>();

        }
        auto& hit = bee::Engine.ECS().Registry.get<bee::Transform>(targetEntity);
        auto& transform = bee::Engine.ECS().Registry.get<bee::Transform>(m_attackGoalEntity);
        transform.Translation = hit.Translation;
        auto& emitter = bee::Engine.ECS().Registry.get<bee::ParticleEmitter>(m_attackGoalEntity);
        emitter.Emit();
    };
    m_orders[OrderType::Attack] = attackOrder;
}

void OrderSystem::InitializeMoveOrder()
{
    Order moveOrder;
    moveOrder.orderBody = [&]()
    {
        const bool rightClicked = bee::Engine.InputWrapper().GetDigitalData("Order").pressedOnce;
        const auto unitView = bee::Engine.ECS().Registry.view<Selected, AllyUnit, bee::ai::StateMachineAgent, bee::Transform>();
        const auto spawnView = bee::Engine.ECS().Registry.view<Selected, SpawningStructure>();

        const glm::vec3 mouseWorldDirection = GetDirectionToMouse();
        const glm::vec3 cameraWorldPosition = GetCameraPosition();

        glm::vec3 hit{};  // Where to move to
        const bool result = GetTerrainRaycastPosition(mouseWorldDirection, cameraWorldPosition, hit);

        if (rightClicked && result)  // If valid location selected and ordered
        {
            // Selected units that are not dead or building, move to hit
            for (auto& entity : unitView)
            {
                auto& stateMachineAgent = unitView.get<bee::ai::StateMachineAgent>(entity);
                if (stateMachineAgent.IsInState<DeadState>()) continue;
                stateMachineAgent.context.blackboard->SetData("PositionToMoveTo", glm::vec2(hit.x, hit.y));
                stateMachineAgent.SetStateOfType<MoveToPointState>();
            }
            if (unitView.begin() != unitView.end())
            {
                auto& transform = bee::Engine.ECS().Registry.get<bee::Transform>(m_goalEntity);
                transform.Translation = hit;
                auto& emitter = bee::Engine.ECS().Registry.get<bee::ParticleEmitter>(m_goalEntity);
                emitter.Emit();
            }

            // Selected spawners send troops to hit
            for (auto& entity : spawnView)
            {
                auto& spawningStructure = spawnView.get<SpawningStructure>(entity);
                spawningStructure.rallyPoint = hit;
                bee::Engine.ECS().Registry.get<bee::Transform>(spawningStructure.flagEntity).Translation =
                    spawningStructure.rallyPoint;
            }
        }
    };
    m_orders[OrderType::Move] = moveOrder;

    LoadOrderTemplates();
}

void OrderSystem::InitializeOffensiveMoveOrder()
{
    Order offensiveAttackOrder;
    offensiveAttackOrder.orderBody = [&]()
    {
        bool rightClicked = bee::Engine.InputWrapper().GetDigitalData("Order").pressedOnce;
        const auto unitView = bee::Engine.ECS().Registry.view<Selected, AllyUnit, bee::ai::StateMachineAgent, bee::Transform>();

        const glm::vec3 mouseWorldDirection = GetDirectionToMouse();
        const glm::vec3 cameraWorldPosition = GetCameraPosition();

        glm::vec3 hit{};  // Where to move to
        const bool result = GetTerrainRaycastPosition(mouseWorldDirection, cameraWorldPosition, hit);

        if (!result) return;
        if (rightClicked && result)
        {
            // Selected units that are not dead or building, move to hit
            for (auto& entity : unitView)
            {
                auto& stateMachineAgent = unitView.get<bee::ai::StateMachineAgent>(entity);
                if (stateMachineAgent.IsInState<DeadState>()) continue;
                stateMachineAgent.context.blackboard->SetData("PositionToMoveTo", glm::vec2(hit.x, hit.y));
                stateMachineAgent.SetStateOfType<OffensiveMove>();
                //auto& transform = bee::Engine.ECS().Registry.get<bee::Transform>(m_attackGoalEntity);
                //transform.Translation = hit;
                //auto& emitter = bee::Engine.ECS().Registry.get<bee::ParticleEmitter>(m_attackGoalEntity);
                //emitter.Emit();
            }
        }
    };
    m_orders[OrderType::OffensiveMove] = offensiveAttackOrder;
}

void OrderSystem::InitializePatrolOrder()
{
    Order patrolOrder;
    patrolOrder.orderBody = [&]()
    {
        bool rightClicked = bee::Engine.InputWrapper().GetDigitalData("Order").pressedOnce;
        const auto unitView = bee::Engine.ECS().Registry.view<Selected, AllyUnit, bee::ai::StateMachineAgent, bee::Transform>();

        const glm::vec3 mouseWorldDirection = GetDirectionToMouse();
        const glm::vec3 cameraWorldPosition = GetCameraPosition();

        glm::vec3 hit{};  // Where to patrol back and forth from
        const bool result = GetTerrainRaycastPosition(mouseWorldDirection, cameraWorldPosition, hit);

        if (rightClicked && result)
        {
            // Selected units that are not dead or building, oscillate between current position and hit
            for (auto& entity : unitView)
            {
                auto& stateMachineAgent = unitView.get<bee::ai::StateMachineAgent>(entity);
                auto& transform = unitView.get<bee::Transform>(entity);
                if (stateMachineAgent.IsInState<DeadState>()) continue;
                stateMachineAgent.context.blackboard->SetData("PositionToMoveTo", glm::vec2(hit.x, hit.y));
                stateMachineAgent.context.blackboard->SetData<glm::vec3>("PatrolPoint1", transform.Translation);
                stateMachineAgent.context.blackboard->SetData<glm::vec3>("PatrolPoint2", glm::vec3(hit.x, hit.y, 0));
                stateMachineAgent.SetStateOfType<PatrolState>();
            }
        }
    };
    m_orders[OrderType::Patrol] = patrolOrder;
}

// --- Initializing Build/Spawn orders

void OrderSystem::InitializeBuildOrder(const std::string& buildingHandle, OrderType orderType)
{
    Order buildOrder;
    buildOrder.onOrderSet = [buildingHandle, &orderType, this]()
    {
        auto& structureManager = bee::Engine.ECS().GetSystem<StructureManager>();
        auto& structureTemplate = structureManager.GetStructureTemplate(buildingHandle);

        m_brushActive = true;
        m_structureBrush.Enable();
        m_structureBrush.SetPreviewModel(buildingHandle);
        m_structureBrush.SetObjectDimensions(structureTemplate.tileDimensions);
    };

    buildOrder.orderBody = [buildingHandle, &orderType, this]()
    {
        const std::string tempHandle = buildingHandle;
        const glm::vec3 mouseWorldDirection = GetDirectionToMouse();
        const glm::vec3 cameraWorldPosition = GetCameraPosition();

        glm::vec3 hit{};
        const bool result = GetTerrainRaycastPosition(mouseWorldDirection, cameraWorldPosition, hit);
        m_structureBrush.Update(hit, tempHandle);
        const bool leftClick = bee::Engine.InputWrapper().GetDigitalData("Selection").pressedOnce;

        // getting the cost of the building
        const auto& woodCost = bee::Engine.ECS()
                                   .GetSystem<StructureManager>()
                                   .GetStructureTemplate(tempHandle)
                                   .GetAttribute(BaseAttributes::WoodCost);
        const auto& stoneCost = bee::Engine.ECS()
                                    .GetSystem<StructureManager>()
                                    .GetStructureTemplate(tempHandle)
                                    .GetAttribute(BaseAttributes::StoneCost);

        auto& resourceSystem = bee::Engine.ECS().GetSystem<ResourceSystem>();

        if (leftClick && result)
        {
            if (resourceSystem.CanAfford(GameResourceType::Wood, woodCost) &&
                resourceSystem.CanAfford(GameResourceType::Stone, stoneCost))
            {
                resourceSystem.Spend(GameResourceType::Wood, woodCost);
                resourceSystem.Spend(GameResourceType::Stone, stoneCost);
                bee::Engine.Audio().PlaySoundW("audio/click.wav", 1.5f, false);

                const auto& structureEntity = m_structureBrush.PlaceObject(tempHandle, hit, Team::Ally);
                if (!structureEntity.has_value())
                {
                    m_structureBrush.Disable();
                    m_structureBrush.RemovePreviewModel();
                    return;
                }

                auto& attributes = bee::Engine.ECS().Registry.get<AttributesComponent>(structureEntity.value());

                if (attributes.HasAttribute(BaseAttributes::SwordsmenLimitIncrease))
                {
                    swordsmenLimit += attributes.GetValue(BaseAttributes::SwordsmenLimitIncrease);
                }

                if (attributes.HasAttribute(BaseAttributes::MageLimitIncrease))
                {
                    mageLimit += attributes.GetValue(BaseAttributes::MageLimitIncrease);
                }


                auto& fsmAgent = bee::Engine.ECS().Registry.get<bee::ai::StateMachineAgent>(structureEntity.value());
                fsmAgent.context.blackboard->SetData("PositionToPlaceBuilding", hit);
                fsmAgent.SetStateOfType<BeingBuilt>();
                bee::Engine.Audio().PlaySoundW("audio/building2.wav", 2.5f, true);

                bee::Engine.ECS().GetSystem<lvle::TerrainSystem>().UpdateTerrainDataComponent();
                bee::Engine.ECS().GetSystem<bee::ai::GridNavigationSystem>().UpdateFromTerrain();
                m_brushActive = false;
                m_structureBrush.Disable();
                m_structureBrush.RemovePreviewModel();
                m_currentOrder = OrderType::Move;
            }
            else
            {
                bee::Engine.Audio().PlaySoundW("audio/not_possible.wav", 1.5f, false);
                std::cout << "Not enough resources" << "\n";
            }
        }
    };

    buildOrder.onOrderHover = [buildingHandle, orderType, this]()
    {
        auto& building = bee::Engine.ECS().GetSystem<StructureManager>().GetStructureTemplate(buildingHandle);
        const auto& woodCost = building.GetAttribute(BaseAttributes::WoodCost);
        const auto& stoneCost = building.GetAttribute(BaseAttributes::StoneCost);
        this->m_orderCost[orderType] = std::make_pair(glm::vec2(woodCost, stoneCost),true);
    };
    m_orders[orderType] = buildOrder;
}

void OrderSystem::InitializeUpgradeOrder()
{
    Order upgradeOrder;
    upgradeOrder.onOrderSet = [this]()
    {
        // initializing the needed systems
        auto& structureManager = bee::Engine.ECS().GetSystem<StructureManager>();
        auto& resourceSystem = bee::Engine.ECS().GetSystem<ResourceSystem>();

        double townHallLevel = 0;
        // getting the town hall level
        const auto& townHallView =
            bee::Engine.ECS().Registry.view<AllyStructure, AttributesComponent, bee::ai::StateMachineAgent, bee::Transform>();
        for (auto [entity, ally, attributes, agent, transform] : townHallView.each())
        {
            if (structureManager.GetStructureTemplate(attributes.GetEntityType()).structureType == StructureTypes::TownHall)
            {
                townHallLevel = attributes.GetValue(BaseAttributes::BuildingLevel);
                break;
            }
        }

        // initializing the view
        const auto& view =
            bee::Engine.ECS()
                .Registry.view<Selected, AllyStructure, AttributesComponent, bee::ai::StateMachineAgent, bee::Transform>();
        auto inGameUI = bee::Engine.ECS().GetSystem<InGameUI>();
        for (auto [entity, selected, ally, attributes, agent, transform] : view.each())
        {
            // getting the handles of the current and upgrade structure
            auto currentStructure = structureManager.GetStructureTemplate(attributes.GetEntityType());
            auto pair = structureManager.FindTemplateOfLevel(currentStructure.GetAttribute(BaseAttributes::BuildingLevel) + 1,
                                                             currentStructure.structureType);

            if (!inGameUI.IsTutorialDone() && pair.second.GetAttribute(BaseAttributes::BuildingLevel)>1)
                continue;

            // preventing buildings to be upgraded if the town hall isn't a higher level than them
            if (pair.second.GetAttribute(BaseAttributes::BuildingLevel) > townHallLevel &&
                pair.second.structureType != StructureTypes::TownHall)
            {
                bee::Engine.Audio().PlaySoundW("audio/not_possible.wav", 1.0f, false);
                continue;
            }

            if (agent.IsInState<TrainUnitState>()) continue;
            // checking if the upgrade bulding exists
            if (structureManager.GetStructures().count(pair.first))
            {
                // Getting the upgrade structure and how much it would cost to build it
                auto& upgradeStructure = pair.second;
                const auto& woodCost = upgradeStructure.GetAttribute(BaseAttributes::WoodCost);
                const auto& stoneCost = upgradeStructure.GetAttribute(BaseAttributes::StoneCost);
                if (resourceSystem.CanAfford(GameResourceType::Wood, woodCost) &&
                    resourceSystem.CanAfford(GameResourceType::Stone, stoneCost))
                {
                    // Spending resources
                    resourceSystem.Spend(GameResourceType::Wood, woodCost);
                    resourceSystem.Spend(GameResourceType::Stone, stoneCost);

                    bee::Engine.Audio().PlaySoundW("audio/click.wav", 1.0f, false);

                    if (attributes.HasAttribute(BaseAttributes::MageLimitIncrease))
                    {
                        mageLimit -= attributes.GetValue(BaseAttributes::MageLimitIncrease);
                    }

                    if (attributes.HasAttribute(BaseAttributes::SwordsmenLimitIncrease))
                    {
                        swordsmenLimit -= attributes.GetValue(BaseAttributes::SwordsmenLimitIncrease);
                    }


                    if (upgradeStructure.HasAttribute(BaseAttributes::MageLimitIncrease))
                    {
                        mageLimit += upgradeStructure.GetAttribute(BaseAttributes::MageLimitIncrease);
                    }

                    if (upgradeStructure.HasAttribute(BaseAttributes::SwordsmenLimitIncrease))
                    {
                        swordsmenLimit += upgradeStructure.GetAttribute(BaseAttributes::SwordsmenLimitIncrease);
                    }

                    int numUnits = 0;
                    std::string unitType = "";

                    auto& fsmAgent = bee::Engine.ECS().Registry.get<bee::ai::StateMachineAgent>(entity);
                    if (fsmAgent.context.blackboard->HasKey<int>("NumUnits"))
                    {
                        numUnits = fsmAgent.context.blackboard->GetData<int>("NumUnits");
                    }

                    if (fsmAgent.context.blackboard->HasKey<std::string>("UnitToTrain"))
                    {
                        unitType = fsmAgent.context.blackboard->GetData<std::string>("UnitToTrain");
                    }

                    glm::vec3 rallyPointPosition = {};

                    auto spawningStructure = bee::Engine.ECS().Registry.try_get<SpawningStructure>(entity);
                    if (spawningStructure)
                    {
                        rallyPointPosition = spawningStructure->rallyPoint;
                    }

                    // Removing the current building
                    structureManager.RemoveStructure(entity);

                    // Spawning the new upgraded structure
                    const auto upgradeEntity = structureManager.SpawnStructure(pair.first, transform.Translation, attributes.smallGridIndex, attributes.flipped, Team::Ally);

                    if (bee::Engine.ECS().Registry.try_get<SpawningStructure>(upgradeEntity.value()))
                    {
                        auto& spawningStructureUpgraded = bee::Engine.ECS().Registry.get<SpawningStructure>(upgradeEntity.value());
                        spawningStructureUpgraded.rallyPoint = rallyPointPosition;
                        bee::Engine.ECS().Registry.get<bee::Transform>(spawningStructureUpgraded.flagEntity).Translation =spawningStructureUpgraded.rallyPoint;
                    }
                    bee::Engine.Audio().PlaySoundW("audio/building2.wav", 2.5f, true);

                    for (auto [entity, debugMetric] : bee::Engine.ECS().Registry.view<bee::DebugMetricData>().each())
                    {
                        if (debugMetric.resourcesSpentOn.find(attributes.GetEntityType()) == debugMetric.resourcesSpentOn.end())
                            debugMetric.resourcesSpentOn.insert(std::pair<std::string, int>(attributes.GetEntityType(), 1));
                        else
                            debugMetric.resourcesSpentOn[attributes.GetEntityType()]++;

                        std::string structureName = attributes.GetEntityType();
                        auto upgradeLvl =
                            static_cast<int>(structureName[structureName.size() - 1]) + 1;
                        structureName[structureName.size() - 1] = static_cast<char>(upgradeLvl);

                        debugMetric.upgradedBuildings[structureName]++;
                        if (debugMetric.buildingSpawned.find(structureName) !=
                            debugMetric.buildingSpawned.end())
                        {
                            debugMetric.buildingSpawned[structureName]--;
                        }
                    }

                    // Spawning the new upgraded structure
                    if (upgradeEntity.has_value())
                    {
                        // Setting the state "BeingBuilt" so that the structure builds from the ground instead of just
                        // spawning
                        auto& fsmAgent = bee::Engine.ECS().Registry.get<bee::ai::StateMachineAgent>(upgradeEntity.value());
                        fsmAgent.context.blackboard->SetData("PositionToPlaceBuilding", transform.Translation);
                        fsmAgent.context.blackboard->SetData("NumUnits", numUnits);
                        fsmAgent.context.blackboard->SetData("UnitToTrain", unitType);
                        fsmAgent.SetStateOfType<BeingBuilt>();
                    }
                }
                else if (!resourceSystem.CanAfford(GameResourceType::Wood, woodCost) ||
                    !resourceSystem.CanAfford(GameResourceType::Stone, stoneCost))
                {
                    bee::Engine.Audio().PlaySoundW("audio/not_possible.wav", 1.0f, false);
                }
            }
        }

        m_currentOrder = OrderType::Move;
    };

    upgradeOrder.onOrderHover = [this]()
    {
        // initializing the needed systems
        auto& structureManager = bee::Engine.ECS().GetSystem<StructureManager>();
        auto& resourceSystem = bee::Engine.ECS().GetSystem<ResourceSystem>();

        double townHallLevel = 0;
        // getting the town hall level
        const auto& townHallView = bee::Engine.ECS().Registry.view<AllyStructure, AttributesComponent>();
        for (auto [entity, ally, attributes] : townHallView.each())
        {
            if (structureManager.GetStructureTemplate(attributes.GetEntityType()).structureType == StructureTypes::TownHall)
            {
                townHallLevel = attributes.GetValue(BaseAttributes::BuildingLevel);
                break;
            }
        }

        // initializing the view
        const auto& view = bee::Engine.ECS().Registry.view<Selected, AllyStructure, AttributesComponent>();
        bool canUpgrade = true;
        for (auto [entity, selected, ally, attributes] : view.each())
        {
            // getting the handles of the current and upgrade structure
            auto currentStructure = structureManager.GetStructureTemplate(attributes.GetEntityType());
            auto pair = structureManager.FindTemplateOfLevel(currentStructure.GetAttribute(BaseAttributes::BuildingLevel) + 1,
                                                             currentStructure.structureType);

            // preventing buildings to be upgraded if the town hall isn't a higher level than them
            if (pair.second.GetAttribute(BaseAttributes::BuildingLevel) > townHallLevel &&
                pair.second.structureType != StructureTypes::TownHall)
            {
                canUpgrade = false;
            }

            // checking if the upgrade bulding exists
            if (structureManager.GetStructures().count(pair.first))
            {
                // Getting the upgrade structure and how much it would cost to build it
                auto& upgradeStructure = pair.second;
                const auto& woodCost = upgradeStructure.GetAttribute(BaseAttributes::WoodCost);
                const auto& stoneCost = upgradeStructure.GetAttribute(BaseAttributes::StoneCost);

                glm::vec2 currentCost = glm::vec2(woodCost, stoneCost);
                this->m_orderCost[OrderType::UpgradeBuilding] =std::pair( currentCost,canUpgrade);
            }
        }
    };
    m_orders[OrderType::UpgradeBuilding] = upgradeOrder;
}

bool OrderSystem::CanSpawnUnit(const std::string& unitHandle)
{
    if (unitHandle == "Warrior")
    {
        if (swordsmenLimit <= GetNumberOfSwordsmen()) return false;
    }

    if (unitHandle == "Mage")
    {
        if (mageLimit <= GetNumberOfMages()) return false;
    }

    return true;
}

void OrderSystem::InitializeTrainOrder(const std::string& unitHandle, OrderType orderType)
{
    Order buildOrder;
    buildOrder.onOrderSet = [unitHandle, this]()
    {
        m_currentOrder = OrderType::Move;
        const std::string tempHandle = unitHandle;
        const auto spawnView = bee::Engine.ECS()
                                   .Registry.view<Selected, AllyStructure, AttributesComponent, bee::ai::StateMachineAgent,
                                                  bee::Transform, SpawningStructure>();

        for (auto [entity, selected, ally, attributes, stateMachineAgent, transform, spawningStructure] : spawnView.each())
        {
            auto& resourceSystem = bee::Engine.ECS().GetSystem<ResourceSystem>();
            auto unitAttributes = bee::Engine.ECS().GetSystem<UnitManager>().GetUnitTemplate(unitHandle).GetAttributes();

            // getting the cost, and reducing it if depending on the CostReduction attribute of the tower
            double woodCost = unitAttributes[BaseAttributes::WoodCost];
            double stoneCost = unitAttributes[BaseAttributes::StoneCost];

            if (!CanSpawnUnit(unitHandle)) return;
            if (!resourceSystem.CanAfford(GameResourceType::Wood, static_cast<int>(woodCost)) ||
                !resourceSystem.CanAfford(GameResourceType::Stone, static_cast<int>(stoneCost)))
            {
                bee::Engine.Audio().PlaySoundW("audio/not_possible.wav", 1.0f, false);
                return;
            }
            else
            {
                bee::Engine.Audio().PlaySoundW("audio/click.wav", 1.0f, false);
            }

            if (unitAttributes[BaseAttributes::WoodCost] > 0)
            {
                woodCost -= attributes.GetValue(BaseAttributes::CostReduction);
            }
            if (unitAttributes[BaseAttributes::StoneCost] > 0)
            {
                stoneCost -= attributes.GetValue(BaseAttributes::CostReduction);
            }

            // getting the cost, and reducing it depending on the TimeReduction attribute of the tower
            unitAttributes[BaseAttributes::CreationTime] -= attributes.GetValue(BaseAttributes::TimeReduction);

            if (stateMachineAgent.context.blackboard->HasKey<int>("NumUnits"))
            {
                auto& numUnits = stateMachineAgent.context.blackboard->GetData<int>("NumUnits");
                if (numUnits >= spawningStructure.spawnLimit) return;
                numUnits = std::clamp(numUnits + 1, 0, spawningStructure.spawnLimit);
            }
            else
            {
                stateMachineAgent.context.blackboard->SetData<int>("NumUnits", 1);
            }

            stateMachineAgent.context.blackboard->SetData<std::string>("UnitToTrain", tempHandle);
            resourceSystem.Spend(GameResourceType::Wood, woodCost);
            resourceSystem.Spend(GameResourceType::Stone, stoneCost);
            for (auto [entity, debugMetric] : bee::Engine.ECS().Registry.view<bee::DebugMetricData>().each())
                if (debugMetric.resourcesSpentOn.find(tempHandle) == debugMetric.resourcesSpentOn.end())
                    debugMetric.resourcesSpentOn.insert(std::pair<std::string, int>(tempHandle, 1));
                else
                    debugMetric.resourcesSpentOn[tempHandle]++;

            stateMachineAgent.SetStateOfType<TrainUnitState>();
        }
    };

    buildOrder.onOrderHover = [unitHandle, orderType, this]()
    {
        auto attributes = bee::Engine.ECS().GetSystem<UnitManager>().GetUnitTemplate(unitHandle).GetAttributes();

        const double woodCost = attributes[BaseAttributes::WoodCost];
        const double stoneCost = attributes[BaseAttributes::StoneCost];
        const auto spawnView = bee::Engine.ECS()
                                   .Registry.view<Selected, AllyStructure, AttributesComponent, bee::ai::StateMachineAgent,
                                                  bee::Transform, SpawningStructure>();

        for (auto [entity, selected, ally, structureAttributes, stateMachineAgent, transform, spawningStructure] :
             spawnView.each())
        {
            glm::vec2 currentCost = glm::vec2(woodCost, stoneCost) -
                                    static_cast<float>(structureAttributes.GetValue(BaseAttributes::CostReduction));
            currentCost = glm::clamp(currentCost, glm::vec2(0.0f), glm::vec2(std::numeric_limits<float>::infinity()));
            this->m_orderCost[orderType] = std::pair(currentCost,true);
        }
    };

    buildOrder.orderBody = []() {};
    m_orders[orderType] = buildOrder;
}

// --- Initializing player orders

void OrderSystem::InitializeBuildWall(const std::string& wallHandle)
{
    Order playerBuildWallStart;
    OrderType type1 = OrderType::BuildWallStart;

    playerBuildWallStart.onOrderSet = [wallHandle, this]()
    {
        m_brushActive = true;
        m_structureBrush.Enable();
        auto& structureManager = bee::Engine.ECS().GetSystem<StructureManager>();
        auto& structureTemplate = structureManager.GetStructureTemplate(wallHandle);
        m_structureBrush.SetObjectDimensions(structureTemplate.tileDimensions);
        const glm::vec3 mouseWorldDirection = GetDirectionToMouse();
        const glm::vec3 cameraWorldPosition = GetCameraPosition();
        const bool resultCast = GetTerrainRaycastPosition(mouseWorldDirection, cameraWorldPosition, m_dragStart);
        m_structureBrush.Update(m_dragStart, wallHandle);
        m_structureBrush.SetPreviewModel(wallHandle);
        m_structureBrush.SetObjectDimensions(glm::vec2(1, 1));
    };

    playerBuildWallStart.orderBody = [wallHandle, type1, this]()
    {
        const bool leftClick = bee::Engine.InputWrapper().GetDigitalData("Selection").pressedOnce;

        if (leftClick)
        {
            m_wallType = false;

            // Testing if the ray cast is valid
            const glm::vec3 mouseWorldDirection = GetDirectionToMouse();
            const glm::vec3 cameraWorldPosition = GetCameraPosition();
            const bool resultCast = GetTerrainRaycastPosition(mouseWorldDirection, cameraWorldPosition, m_dragStart);

            // Testing if the tile if buildable
            m_structureBrush.Update(m_dragStart, wallHandle);
            const std::vector<int> hoveredTiles = m_structureBrush.GetBrushTiles();

            const auto& terrain = bee::Engine.ECS().GetSystem<lvle::TerrainSystem>();
            const bool resultBuild = terrain.CanBuildOnTile(hoveredTiles[0]);

            if (resultCast && resultBuild)
            {
                m_drag = true;
                m_dragPoints.push_back(m_dragStart);
                DragReset();
            }
        }
    };
    m_orders[OrderType::BuildWallStart] = playerBuildWallStart;
    // --- --- --- --- --- --- ---
    //
    // --- --- --- --- --- --- ---

    playerBuildWallStart.onOrderHover = [wallHandle, this]()
    {
        auto& structureManager = bee::Engine.ECS().GetSystem<StructureManager>();
        auto& structureTemplate = structureManager.GetStructureTemplate(wallHandle);
        const auto& woodCost = structureTemplate.GetAttribute(BaseAttributes::WoodCost);
        const auto& stoneCost = structureTemplate.GetAttribute(BaseAttributes::StoneCost);

        glm::vec2 currentCost = glm::vec2(woodCost, stoneCost);
        this->m_orderCost[OrderType::BuildWallStart] = std::pair(currentCost, true);
    };

    // --- --- --- --- --- --- ---
    //
    // --- --- --- --- --- --- ---

    Order playerBuildWallEnd;
    OrderType type2 = OrderType::BuildWallEnd;

    playerBuildWallEnd.onOrderSet = [wallHandle, this]() {};

    playerBuildWallEnd.orderBody = [wallHandle, type2, this]()
    {
        const bool leftClick = bee::Engine.InputWrapper().GetDigitalData("Selection").pressedOnce;

        if (leftClick)
        {
            // Building the fences
            m_structureBrush.PlaceMultipleObjects(wallHandle, m_dragPoints, m_currentOrientation, m_buildCurrent, GameResourceType::Stone);
            bee::Engine.ECS().GetSystem<lvle::TerrainSystem>().UpdateTerrainDataComponent();
            bee::Engine.ECS().GetSystem<bee::ai::GridNavigationSystem>().UpdateFromTerrain();
            m_brushActive = false;

            m_drag = false;
            m_structureBrush.RemovePreviewModel();
            m_structureBrush.Disable();

            // Clearing and Reseting
            m_dragPoints.clear();
            DragReset();
            SetCurrentOrder(OrderType::Move);
        }
    };
    m_orders[OrderType::BuildWallEnd] = playerBuildWallEnd;
}

void OrderSystem::InitializeBuildFence(const std::string& fenceHandle)
{
    Order playerBuildFenceStart;
    OrderType type1 = OrderType::BuildFenceStart;

    playerBuildFenceStart.onOrderSet = [fenceHandle, this]()
    {
        m_brushActive = true;
        m_structureBrush.Enable();
        auto& structureManager = bee::Engine.ECS().GetSystem<StructureManager>();
        auto& structureTemplate = structureManager.GetStructureTemplate(fenceHandle);
        m_structureBrush.SetObjectDimensions(structureTemplate.tileDimensions);
        const glm::vec3 mouseWorldDirection = GetDirectionToMouse();
        const glm::vec3 cameraWorldPosition = GetCameraPosition();
        const bool resultCast = GetTerrainRaycastPosition(mouseWorldDirection, cameraWorldPosition, m_dragStart);
        m_structureBrush.Update(m_dragStart, fenceHandle);
        m_structureBrush.SetPreviewModel(fenceHandle);
        m_structureBrush.SetObjectDimensions(glm::vec2(1, 1));
    };

    playerBuildFenceStart.orderBody = [fenceHandle, type1, this]()
    {
        const bool leftClick = bee::Engine.InputWrapper().GetDigitalData("Selection").pressedOnce;

        if (leftClick)
        {
            m_wallType = true;

            // Testing if the ray cast is valid
            const glm::vec3 mouseWorldDirection = GetDirectionToMouse();
            const glm::vec3 cameraWorldPosition = GetCameraPosition();
            const bool resultCast = GetTerrainRaycastPosition(mouseWorldDirection, cameraWorldPosition, m_dragStart);

            // Testing if the tile if buildable
            m_structureBrush.Update(m_dragStart, fenceHandle);
            const std::vector<int> hoveredTiles = m_structureBrush.GetBrushTiles();

            const auto& terrain = bee::Engine.ECS().GetSystem<lvle::TerrainSystem>();
            const bool resultBuild = terrain.CanBuildOnTile(hoveredTiles[0]);

            if (resultCast && resultBuild)
            {
                m_drag = true;
                m_dragPoints.push_back(m_dragStart);
                DragReset();
            }
        }
    };
    m_orders[OrderType::BuildFenceStart] = playerBuildFenceStart;

    // --- --- --- --- --- --- ---
    //
    // --- --- --- --- --- --- ---

    playerBuildFenceStart.onOrderHover = [fenceHandle, this]()
    {
        auto& structureManager = bee::Engine.ECS().GetSystem<StructureManager>();
        auto& structureTemplate = structureManager.GetStructureTemplate(fenceHandle);
        const auto& woodCost = structureTemplate.GetAttribute(BaseAttributes::WoodCost);
        const auto& stoneCost = structureTemplate.GetAttribute(BaseAttributes::StoneCost);

        glm::vec2 currentCost = glm::vec2(woodCost, stoneCost);
        this->m_orderCost[OrderType::BuildFenceStart] = std::pair(currentCost, true);
    };

    // --- --- --- --- --- --- ---
    //
    // --- --- --- --- --- --- ---

    Order playerBuildFenceEnd;
    OrderType type2 = OrderType::BuildFenceEnd;

    playerBuildFenceEnd.onOrderSet = [fenceHandle, this]() {};

    playerBuildFenceEnd.orderBody = [fenceHandle, type2, this]()
    {
        const bool leftClick = bee::Engine.InputWrapper().GetDigitalData("Selection").pressedOnce;

        if (leftClick)
        {
            // Building the Walls
            m_structureBrush.PlaceMultipleObjects(fenceHandle, m_dragPoints, m_currentOrientation, m_buildCurrent, GameResourceType::Wood);
            bee::Engine.ECS().GetSystem<lvle::TerrainSystem>().UpdateTerrainDataComponent();
            bee::Engine.ECS().GetSystem<bee::ai::GridNavigationSystem>().UpdateFromTerrain();
            m_brushActive = false;

            m_drag = false;
            m_structureBrush.RemovePreviewModel();
            m_structureBrush.Disable();

            // Clearing and Reseting
            m_dragPoints.clear();
            DragReset();
            SetCurrentOrder(OrderType::Move);
        }
    };
    m_orders[OrderType::BuildFenceEnd] = playerBuildFenceEnd;
}

// --- Other methods

void OrderSystem::DragInitialize()
{
    const auto& modelWall  = bee::Engine.Resources().Load<bee::Model>("models/structures/SM_Wall.glb");
    for (size_t i = 0; i < m_buildMax; i++)
    {
        auto entity = bee::Engine.ECS().CreateEntity();
        auto& transform = bee::Engine.ECS().CreateComponent<bee::Transform>(entity);
            transform.Name = "Semi-Transparent Wall";
            transform.Translation = glm::vec3(0.0f, 0.0f, -300.0f);
        auto& tag = bee::Engine.ECS().CreateComponent<DragStructure>(entity);
        bee::ConstantBufferData constantData;
            constantData.opacity = 0.1f;
        modelWall->Instantiate(entity, constantData);
        m_dragWalls.push_back(entity);
    }

    const auto& modelFence = bee::Engine.Resources().Load<bee::Model>("models/structures/SM_Fence.glb");
    for (size_t i = 0; i < m_buildMax; i++)
    {
        auto entity = bee::Engine.ECS().CreateEntity();
        auto& transform = bee::Engine.ECS().CreateComponent<bee::Transform>(entity);
            transform.Name = "Semi-Transparent Fence";
            transform.Translation = glm::vec3(0.0f, 0.0f, -300.0f);
        auto& tag = bee::Engine.ECS().CreateComponent<DragStructure>(entity);
        bee::ConstantBufferData constantData;
            constantData.opacity = 0.1f;
        modelFence->Instantiate(entity, constantData);
        m_dragFences.push_back(entity);
    }
}

void OrderSystem::DragUpdate()
{
    if (m_drag)
    {
        glm::vec2 mouseCurrentPosition = bee::Engine.InputWrapper().GetAnalogData("Mouse");

        // New wall path gets updated only when the cursor moves. This is done to avoid calculating A* each tick.
        if ((mouseCurrentPosition.x - m_mousePositionLast.x != 0.0f) ||
            (mouseCurrentPosition.y - m_mousePositionLast.y != 0.0f))
        {
            const glm::vec3 mouseWorldDirection = GetDirectionToMouse();
            const glm::vec3 cameraWorldPosition = GetCameraPosition();

            // Getting the latest mouse hitting tile
            glm::vec3 hit{};
            const bool result = GetTerrainRaycastPosition(mouseWorldDirection, cameraWorldPosition, hit);

            if (result)
            {
                // Determining the orientation
                const glm::vec3 finalHit = DragDetermineDirection(hit);

                // Fetching grid from navigation system
                const auto& grid = bee::Engine.ECS().GetSystem<bee::ai::GridNavigationSystem>().GetGrid();

                // Updating path for the line
                bee::ai::NavigationPath path = grid.ComputePathStraightLine(glm::vec2(m_dragStart.x, m_dragStart.y), glm::vec2(finalHit.x, finalHit.y));
                m_dragPoints.clear();
                m_dragPoints = path.GetPoints();

                // Updating drag counts
                DragDetermineValues();
            }
            m_mousePositionLast = mouseCurrentPosition;
        }
    }
    else
        PollWrapperInput();
}

void OrderSystem::DragReset()
{
    const auto view = bee::Engine.ECS().Registry.view<bee::Transform, DragStructure>();

    for (auto [entity, transform, tag] : view.each()) transform.Translation = glm::vec3(0.0f, 0.0f, -300.0f);
};

const glm::vec3 OrderSystem::DragDetermineDirection(const glm::vec3& hit)
{
    // Values will be used to determine which way to drag
    const glm::vec3 test = hit - m_dragStart;
    const float absoluteX = std::fabsf(test.x);
    const float absoluteY = std::fabsf(test.y);

    if (absoluteX >= absoluteY)
    {
        m_currentOrientation = m_vertical;
        m_structureBrush.SetFlipped(false);
        glm::vec3 value = glm::vec3(hit.x, m_dragStart.y, hit.z);
        return value;
    }
    else
    {
        m_structureBrush.SetFlipped(true);
        m_currentOrientation = m_horizontal;
        glm::vec3 value = glm::vec3(m_dragStart.x, hit.y, hit.z);
        return value;
    }
}

void OrderSystem::DragDetermineValues()
{
    // Used to access resource data
    auto& resources = bee::Engine.ECS().GetSystem<ResourceSystem>();

    const auto& view = bee::Engine.ECS().Registry.view<bee::Transform, bee::MeshRenderer, StructureModelTag>(entt::exclude<bee::Camera>);

    // Always iterate with the cap being the size of the smaller vector
    if (m_dragPoints.size() > m_buildMax)
    {
        if (m_wallType)  // Is the wall a fence
        {
            // Updating all fences because all are visible
            for (size_t i = 0; i < m_dragFences.size(); i++)
            {
                if (resources.CanAfford(GameResourceType::Wood, 2 + (2 * static_cast<int>(i))))
                {
                    auto& transform = view.get<bee::Transform>(m_dragFences[i]);
                    transform.Translation = m_dragPoints[i];
                    transform.Rotation = m_currentOrientation;
                }
            }
        }
        else // wall is a wall
        {
            // Updating all walls because all are visible
            for (size_t i = 0; i < m_dragWalls.size(); i++)
            {
                if (resources.CanAfford(GameResourceType::Stone, 2 + (2 * static_cast<int>(i))))
                {
                    auto& transform = view.get<bee::Transform>(m_dragWalls[i]);
                        transform.Translation = m_dragPoints[i];
                        transform.Rotation = m_currentOrientation;
                }
            }
        }

        // Updating the number of wall to actually build
        m_buildCurrent = m_buildMax;
    }
    else
    {
        if (m_wallType)  // Is the wall a fence
        {
            // Updating all visible walls
            for (size_t i = 0; i < m_dragPoints.size(); i++)
            {
                if (resources.CanAfford(GameResourceType::Wood, 2 + (2 * static_cast<int>(i))))
                {
                    auto& transform = view.get<bee::Transform>(m_dragFences[i]);
                        transform.Translation = m_dragPoints[i];
                        transform.Rotation = m_currentOrientation;
                }
            }

            // Hiding the rest from the player
            for (size_t i = m_dragPoints.size(); i < m_dragFences.size(); i++)
            {
                auto& transform = view.get<bee::Transform>(m_dragFences[i]);
                transform.Translation = glm::vec3(0.0f, 0.0f, -300.0f);
            }
        }
        else // wall is a wall
        {
            // Updating all visible walls
            for (size_t i = 0; i < m_dragPoints.size(); i++)
            {
                if (resources.CanAfford(GameResourceType::Stone, 2 + (2 * static_cast<int>(i))))
                {
                    auto& transform = view.get<bee::Transform>(m_dragWalls[i]);
                        transform.Translation = m_dragPoints[i];
                        transform.Rotation = m_currentOrientation;
                }
            }

            // Hiding the rest from the player
            for (size_t i = m_dragPoints.size(); i < m_dragWalls.size(); i++)
            {
                auto& transform = view.get<bee::Transform>(m_dragWalls[i]);
                transform.Translation = glm::vec3(0.0f, 0.0f, -300.0f);
            }
        }

        // Updating the number of wall to actually build
        m_buildCurrent = m_dragPoints.size();
    }
}

int OrderSystem::GetNumberOfSwordsmen()
{
    int toReturn = 0;
    const auto view = bee::Engine.ECS().Registry.view<AllyUnit, bee::ai::StateMachineAgent>();
    for (const auto entity : view)
    {
        auto& agent = view.get<bee::ai::StateMachineAgent>(entity);
        if (agent.fsm.GetStateIDsOfType<MeleeAttackState>().empty()) continue;
        toReturn++;
    }

    const auto buildingView = bee::Engine.ECS().Registry.view<bee::ai::StateMachineAgent, AllyStructure, SpawningStructure>();
    for (const auto entity : buildingView)
    {
        auto& agent = view.get<bee::ai::StateMachineAgent>(entity);
        if (!agent.context.blackboard->HasKey<int>("NumUnits")) continue;
        if (agent.context.blackboard->GetData<std::string>("UnitToTrain") == "Warrior")
        {
            toReturn += agent.context.blackboard->GetData<int>("NumUnits");
        }
    }

    return toReturn;
}

int OrderSystem::GetNumberOfMages()
{
    int toReturn = 0;
    const auto view = bee::Engine.ECS().Registry.view<AllyUnit, bee::ai::StateMachineAgent>();
    for (const auto entity : view)
    {
        auto& agent = view.get<bee::ai::StateMachineAgent>(entity);
        if (agent.fsm.GetStateIDsOfType<RangedAttackState>().empty()) continue;
        toReturn++;
    }

    const auto buildingView = bee::Engine.ECS().Registry.view<bee::ai::StateMachineAgent, AllyStructure, SpawningStructure>();
    for (const auto entity : buildingView)
    {
        auto& agent = view.get<bee::ai::StateMachineAgent>(entity);
        if (!agent.context.blackboard->HasKey<int>("NumUnits")) continue;
        if (agent.context.blackboard->GetData<std::string>("UnitToTrain") == "Mage")
        {
            toReturn += agent.context.blackboard->GetData<int>("NumUnits");
        }
    }

    return toReturn;
}

// --- Order wrapper

void OrderSystem::InitializeOrders()
{
    InitializeAttackOrder();
    InitializeMoveOrder();
    InitializeOffensiveMoveOrder();
    InitializePatrolOrder();
    InitializeUpgradeOrder();

    InitializeBuildOrder(std::string("Base"), OrderType::BuildBase);
    InitializeBuildOrder(std::string("SwordsmenTowerLvl1"), OrderType::BuildBarracks);
    InitializeBuildWall(std::string("Wall"));
    InitializeBuildFence(std::string("Fence"));
    InitializeBuildOrder(std::string("MageTowerLvl1"), OrderType::BuildMageTower);
    InitializeTrainOrder(std::string("Warrior"), OrderType::TrainWarrior);
    InitializeTrainOrder(std::string("Mage"), OrderType::TrainMage);
}

void OrderSystem::InitializeGoalEntity()
{
    m_goalEntity = bee::Engine.ECS().CreateEntity();
    bee::Engine.ECS().CreateComponent<bee::Transform>(m_goalEntity);
    auto& emitter = bee::Engine.ECS().CreateComponent<bee::ParticleEmitter>(m_goalEntity);
    bee::Engine.ECS().GetSystem<bee::ParticleSystem>().LoadEmitterFromTemplate(emitter, "effects/goalEmitter.pepitter");
    emitter.AssignEntity(m_goalEntity);
}

void OrderSystem::InitializeAttackGoalEntity()
{
    m_attackGoalEntity = bee::Engine.ECS().CreateEntity();
    bee::Engine.ECS().CreateComponent<bee::Transform>(m_attackGoalEntity);
    auto& emitter = bee::Engine.ECS().CreateComponent<bee::ParticleEmitter>(m_attackGoalEntity);
    bee::Engine.ECS().GetSystem<bee::ParticleSystem>().LoadEmitterFromTemplate(emitter, "effects/attackGoalEmitter.pepitter");
    emitter.AssignEntity(m_attackGoalEntity);
}
